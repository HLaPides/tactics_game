#include "game.h"
#include "combat.h"
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

    // player unit lookup — add new types by extending this table
    std::unordered_map<char, std::pair<UnitStats, std::string>> player_types = {
        { '1', { UnitPresets::Bosun(),        "Bosun"        } },
        { '2', { UnitPresets::Sharpshooter(), "Sharpshooter" } },
        { '3', { UnitPresets::Medic(),        "Medic"        } },
        { '4', { UnitPresets::Swashbuckler(), "Swashbuckler" } },
    };

    // enemy unit lookup — add new types by extending this table
    std::unordered_map<char, UnitStats> enemy_types = {
        { 'E', UnitPresets::Soldier() },
        { 'G', UnitPresets::Guard()   },
        { 'C', UnitPresets::Captain() },
    };

    auto player_spawns = state.map.get_player_spawns();
    auto enemy_spawns  = state.map.get_enemy_spawns();

    if (player_spawns.empty()) return false;

    // sort player spawns by type digit so unit 1 always spawns before 2 etc.
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

    for (auto& sp : enemy_spawns) {
        auto it = enemy_types.find(sp.type);
        if (it == enemy_types.end()) continue;
        state.enemies.push_back(enemy(sp.col, sp.row, it->second));
    }

    if (state.players.empty()) return false;

    state.spotted.assign(state.enemies.size(), false);
    state.selected_player = 0;
    update_visibility();
    return true;
}

void game::update_visibility() {
    if (state.players.empty()) return;
    unit& active = state.players[state.selected_player];
    int px = active.get_x_pos();
    int py = active.get_y_pos();
    int sr = active.get_sight_range();

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

void game::check_win_conditions() {
    if (state.win_state != WinState::ONGOING) return;

    // defeat — all players dead
    bool any_alive = false;
    for (auto& p : state.players) {
        if (p.is_alive()) { any_alive = true; break; }
    }
    if (!any_alive) {
        state.win_state = WinState::DEFEAT;
        return;
    }

    // victory — any living player on objective tile
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
        if (intent.has_value())
            turns.apply_player_intent(intent.value(), state);
        update_visibility();
        check_win_conditions();
    }

    if (state.phase == GamePhase::ENEMY_TURN) {
        turns.update_enemy_turn(dt, state, ai);
        update_visibility();
        check_win_conditions();
    }

    if (state.mode == ActionMode::SHOOT || state.mode == ActionMode::MELEE)
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