#include "game.h"
#include "../combat/combat.h"
#include "../units/unit_factory.h"
#include "raylib.h"
#include <algorithm>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

game::game(const std::string& level_dir, const AppConfig& cfg)
    : config(cfg)
    , campaign()
    , state()
    , input(cfg)
    , turns()
    , ai()
    , renderer(cfg)
    , level_dir(level_dir)
{
    init_campaign();
    start_mission();
}

void game::init_campaign() {
    struct RosterEntry {
        std::function<unit(int,int)> factory;
        std::string                  name;
    };

    std::vector<RosterEntry> entries = {
        { UnitFactory::make_bosun,        "Bosun"        },
        { UnitFactory::make_sharpshooter, "Sharpshooter" },
        { UnitFactory::make_medic,        "Medic"        },
        { UnitFactory::make_swashbuckler, "Swashbuckler" },
    };

    for (auto& e : entries) {
        campaign.roster.push_back(e.factory(0, 0));
        campaign.names.push_back(e.name);
        campaign.permanently_dead.push_back(false);
    }
}

void game::start_mission() {
    reset_mission_state();

    std::vector<std::string> levels = {
    level_dir + "/mutiny_map.tmj"
    };

    std::string path = levels[campaign.mission_index % (int)levels.size()];
    if (!load_level(path)) return;
}

bool game::load_level(const std::string& path) {
    if (!state.map.load(path)) return false;

    auto player_spawns = state.map.get_player_spawns();
    auto enemy_spawns  = state.map.get_enemy_spawns();

    if (player_spawns.empty()) return false;

    std::sort(player_spawns.begin(), player_spawns.end(),
              [](const SpawnPoint& a, const SpawnPoint& b) {
                  return a.type < b.type;
              });

    using EnemyFactory = std::function<enemy(int,int)>;
    std::unordered_map<char, EnemyFactory> enemy_types = {
        { 'E', UnitFactory::make_soldier },
        { 'G', UnitFactory::make_guard   },
        { 'C', UnitFactory::make_captain },
    };

    for (auto& sp : player_spawns) {
        int roster_idx = sp.type - '1';
        if (roster_idx < 0 || roster_idx >= (int)campaign.roster.size()) continue;
        if (campaign.permanently_dead[roster_idx]) continue;

        unit u = campaign.roster[roster_idx];
        u.set_position(sp.col, sp.row);
        u.reset_hp();

        state.players.push_back(u);
        state.player_names.push_back(campaign.names[roster_idx]);
        state.from_roster.push_back(true);
    }

    for (auto& sp : enemy_spawns) {
        auto it = enemy_types.find(sp.type);
        if (it == enemy_types.end()) continue;
        state.enemies.push_back(it->second(sp.col, sp.row));
    }

    if (state.players.empty()) return false;

    state.spotted.assign(state.enemies.size(), false);
    state.selected_player = 0;

    // init camera via renderer
    if (!state.players.empty()) {
        const unit& active = state.players[0];
        renderer.update_camera(
            active.get_x_pos(),
            active.get_y_pos(),
            state.map.getCols() * config.tile_size);
    }

    update_visibility();
    return true;
}

void game::end_mission() {
    write_back_to_roster();
    campaign.mission_index++;
}

void game::write_back_to_roster() {
    int roster_slot = 0;
    for (int i = 0; i < (int)state.players.size(); i++) {
        if (!state.from_roster[i]) continue;

        while (roster_slot < (int)campaign.permanently_dead.size() &&
               campaign.permanently_dead[roster_slot])
            roster_slot++;

        if (roster_slot >= (int)campaign.roster.size()) break;

        if (!state.players[i].is_alive()) {
            campaign.permanently_dead[roster_slot] = true;
        } else {
            campaign.roster[roster_slot] = state.players[i];
        }

        roster_slot++;
    }
}

void game::reset_mission_state() {
    state = GameState{};
}

void game::update_visibility() {
    if (state.players.empty()) return;

    for (auto& player : state.players) {
        if (!player.is_alive()) continue;
        int px = player.get_x_pos();
        int py = player.get_y_pos();
        int sr = player.get_sight_range();

        for (int i = 0; i < (int)state.enemies.size(); i++) {
            if (state.spotted[i]) continue;
            if (!state.enemies[i].is_alive()) continue;

            int ex   = state.enemies[i].get_x_pos();
            int ey   = state.enemies[i].get_y_pos();
            int dist = std::max(abs(ex - px), abs(ey - py));

            if (dist <= sr && has_los(px, py, ex, ey, state.map))
                state.spotted[i] = true;
        }
    }
}

void game::check_win_conditions() {
    if (state.win_state != WinState::ONGOING) return;

    bool any_alive = false;
    for (auto& p : state.players) {
        if (p.is_alive()) { any_alive = true; break; }
    }
    if (!any_alive) {
        state.win_state = WinState::DEFEAT;
        return;
    }

    for (auto& p : state.players) {
        if (!p.is_alive()) continue;
        if (state.map.is_objective(p.get_x_pos(), p.get_y_pos())) {
            state.win_state = WinState::VICTORY;
            end_mission();
            return;
        }
    }
}

void game::update(float dt) {
    if (state.win_state != WinState::ONGOING) {
        if (state.win_state == WinState::DEFEAT && IsKeyPressed(KEY_R)) {
            campaign = CampaignState{};
            init_campaign();
            start_mission();
        }
        return;
    }

    state.floating_texts.update(dt);

    const Camera2D& cam = renderer.get_camera();
    input.update_preview(state, cam);

    if (state.phase == GamePhase::PLAYER_TURN) {
        auto intent = input.poll(state, cam);
        if (intent.has_value()) {
            if (intent->type == IntentType::SelectUnit) {
                state.selected_player = intent->value;
                state.mode            = ActionMode::NONE;
                state.preview         = {};
            } else {
                turns.apply_player_intent(intent.value(), state);
            }
        }

        // update camera to follow selected player
        if (!state.players.empty()) {
            const unit& active = state.players[state.selected_player];
            renderer.update_camera(
                active.get_x_pos(),
                active.get_y_pos(),
                state.map.getCols() * config.tile_size);
        }

        update_visibility();
        check_win_conditions();
    }

    if (state.phase == GamePhase::ENEMY_TURN) {
        turns.update_enemy_turn(dt, state, ai);

        if (!state.players.empty()) {
            const unit& active = state.players[state.selected_player];
            renderer.update_camera(
                active.get_x_pos(),
                active.get_y_pos(),
                state.map.getCols() * config.tile_size);
        }

        update_visibility();
        check_win_conditions();
    }

    if (state.mode == ActionMode::SHOOT  ||
        state.mode == ActionMode::MELEE  ||
        state.mode == ActionMode::HEAL   ||
        state.mode == ActionMode::DIRTY_TRICK)
        SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
    else
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

void game::draw() {
    renderer.draw_frame(state);
}

void game::run() {
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        update(dt);
        draw();
    }
}