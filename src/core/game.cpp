#include "game.h"
#include "combat/combat.h"
#include "abilities/ability_defs.h"
#include "units/enemy.h"
#include "raylib.h"
#include <algorithm>
#include <vector>
#include <string>
#include <unordered_map>
#include <utility>

game::game(const std::string& level_dir, const AppConfig& cfg)
    : config(cfg)
    , state()
    , input(cfg)
    , turns()
    , ai()
    , renderer(cfg)
{
    load_level(level_dir);
}

bool game::load_level(const std::string& level_dir) {
    std::vector<std::string> levels = {
        level_dir + "/ship_01.txt"
    };

    int chosen = GetRandomValue(0, (int)levels.size() - 1);
    if (!state.map.load(levels[chosen])) return false;

    std::unordered_map<char, std::pair<UnitStats, std::string>> player_types = {
        { '1', { UnitPresets::Bosun(),        "Bosun"        } },
        { '2', { UnitPresets::Sharpshooter(), "Sharpshooter" } },
        { '3', { UnitPresets::Medic(),        "Medic"        } },
        { '4', { UnitPresets::Swashbuckler(), "Swashbuckler" } },
    };

    std::unordered_map<char, std::pair<UnitStats, EnemyType>> enemy_types = {
        { 'E', { UnitPresets::Soldier(), EnemyType::SOLDIER  } },
        { 'G', { UnitPresets::Guard(),   EnemyType::GUARD    } },
        { 'C', { UnitPresets::Captain(), EnemyType::CAPTAIN  } },
    };

    auto player_spawns = state.map.get_player_spawns();
    auto enemy_spawns  = state.map.get_enemy_spawns();

    if (player_spawns.empty()) return false;

    std::sort(player_spawns.begin(), player_spawns.end(),
              [](const SpawnPoint& a, const SpawnPoint& b) {
                  return a.type < b.type;
              });

    for (auto& sp : player_spawns) {
        auto it = player_types.find(sp.type);
        if (it == player_types.end()) continue;
        state.players.push_back(unit(sp.col, sp.row, it->second.first));
        state.player_names.push_back(it->second.second);
    }

    // assign starting abilities based on unit type
    for (int i = 0; i < (int)state.players.size(); i++) {
        unit&              p    = state.players[i];
        const std::string& name = state.player_names[i];

        // all units get shoot and melee
        p.add_ability(AbilityDefs::make_shoot());
        p.add_ability(AbilityDefs::make_melee());

        if (name == "Bosun") {
            p.add_ability(AbilityDefs::make_rush());
        } else if (name == "Sharpshooter") {
            p.add_ability(AbilityDefs::make_aimed_shot());
            p.add_ability(AbilityDefs::make_overwatch());
        } else if (name == "Medic") {
            p.add_ability(AbilityDefs::make_heal());
        } else if (name == "Swashbuckler") {
            p.add_ability(AbilityDefs::make_dirty_trick());
        }
    }

    for (auto& sp : enemy_spawns) {
        auto it = enemy_types.find(sp.type);
        if (it == enemy_types.end()) continue;
        state.enemies.push_back(enemy(sp.col, sp.row, it->second.first, it->second.second));
    }

    if (state.players.empty()) return false;

    state.spotted.assign(state.enemies.size(), false);
    state.selected_player = 0;

    state.camera.zoom     = 1.0f;
    state.camera.rotation = 0.0f;
    update_camera();
    update_visibility();
    return true;
}

void game::update_camera() {
    if (state.players.empty()) return;

    const unit& active    = state.players[state.selected_player];
    int         tile_size = config.tile_size;
    int         map_w     = state.map.getCols() * tile_size;

    float target_x = active.get_x_pos() * tile_size + tile_size / 2.0f;
    float half_w   = config.screen_w / 2.0f;
    target_x = std::max(half_w, std::min(target_x, (float)map_w - half_w));

    state.camera.target = { target_x, config.grid_h / 2.0f };
    state.camera.offset = { config.screen_w / 2.0f, config.grid_h / 2.0f };
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
            return;
        }
    }
}

void game::update(float dt) {
    if (state.win_state != WinState::ONGOING) return;

    state.floating_texts.update(dt);
    input.update_preview(state);

    if (state.phase == GamePhase::PLAYER_TURN) {
        auto intent = input.poll(state);
        if (intent.has_value()) {
            if (intent->type == IntentType::SelectUnit) {
                state.selected_player = intent->value;
                state.mode    = ActionMode::NONE;
                state.preview = {};
                update_camera();
            } else {
                turns.apply_player_intent(intent.value(), state);
            }
        }
        update_camera();
        update_visibility();
        check_win_conditions();
    }

    if (state.phase == GamePhase::ENEMY_TURN) {
        turns.update_enemy_turn(dt, state, ai);
        update_camera();
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