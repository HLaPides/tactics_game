#include "game.h"
#include "raylib.h"
#include <algorithm>
#include <vector>
#include <string>

game::game(const std::string& level_dir, const AppConfig& config)
    : config(config)
    , state()
    , input(config)
    , turns()
    , ai()
    , renderer(config)
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
                        UnitStats{ 4, 10, 75, 15, 6, 2, 3 });

    int enemy_count = std::min(6, (int)enemy_spawns.size());
    for (int i = 0; i < enemy_count; i++) {
        state.enemies.push_back(
            enemy(enemy_spawns[i].col, enemy_spawns[i].row,
                  UnitStats{ 2, 5, 60, 10, 3, 1, 1 }));
    }

    return true;
}

void game::update(float dt) {
    state.floating_texts.update(dt);
    input.update_preview(state);

    if (state.phase == GamePhase::PLAYER_TURN) {
        auto intent = input.poll(state);
        if (intent.has_value())
            turns.apply_player_intent(intent.value(), state);
    }

    if (state.phase == GamePhase::ENEMY_TURN)
        turns.update_enemy_turn(dt, state, ai);

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