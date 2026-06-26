#include "game.h"
#include "combat.h"
#include "raylib.h"
#include <algorithm>
#include <vector>
#include <string>

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

    auto player_spawns = state.map.get_player_spawns();
    auto enemy_spawns  = state.map.get_enemy_spawns();

    if (player_spawns.empty()) return false;

    for (int i = (int)enemy_spawns.size() - 1; i > 0; i--) {
        int j = GetRandomValue(0, i);
        std::swap(enemy_spawns[i], enemy_spawns[j]);
    }

    state.player = unit(player_spawns[0].col, player_spawns[0].row,
                        UnitStats{ 4, 10, 75, 15, 6, 2, 3, 10 });

    int enemy_count = std::min(6, (int)enemy_spawns.size());
    for (int i = 0; i < enemy_count; i++) {
        state.enemies.push_back(
            enemy(enemy_spawns[i].col, enemy_spawns[i].row,
                  UnitStats{ 2, 5, 60, 10, 3, 1, 1, 6 }));
    }

    // initialise spotted — all false
    state.spotted.assign(state.enemies.size(), false);

    // run initial visibility check
    update_visibility();

    return true;
}

void game::update_visibility() {
    int px = state.player.get_x_pos();
    int py = state.player.get_y_pos();
    int sr = state.player.get_sight_range();

    for (int i = 0; i < (int)state.enemies.size(); i++) {
        if (state.spotted[i]) continue;  // already spotted — stays visible
        if (!state.enemies[i].is_alive()) continue;

        int ex   = state.enemies[i].get_x_pos();
        int ey   = state.enemies[i].get_y_pos();
        int dist = std::max(abs(ex - px), abs(ey - py));

        if (dist <= sr && has_los(px, py, ex, ey, state.map))
            state.spotted[i] = true;
    }
}

void game::update(float dt) {
    state.floating_texts.update(dt);
    input.update_preview(state);

    if (state.phase == GamePhase::PLAYER_TURN) {
        auto intent = input.poll(state);
        if (intent.has_value())
            turns.apply_player_intent(intent.value(), state);
        update_visibility();
    }

    if (state.phase == GamePhase::ENEMY_TURN) {
        turns.update_enemy_turn(dt, state, ai);
        update_visibility();
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