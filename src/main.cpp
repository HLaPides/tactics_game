#include "raylib.h"
#include "map.h"
#include "units/unit.h"
#include "units/enemy.h"
#include "resource_dir.h"
#include <algorithm>

enum GameState { PLAYER_TURN, ENEMY_TURN };

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 800, "Xcom Knockoff");

    map game_map = map(640, 640, 64);
    int tile_size = game_map.get_tile_size();
    int cols = game_map.getCols();   // tile counts, not pixel dimensions
    int rows = game_map.getRows();

    unit player = unit(1, 2, 3);
    enemy enemy1 = enemy(7, 5, 2);

    GameState state = PLAYER_TURN;

    while (!WindowShouldClose()) {

        // --- INPUT ---
        if (state == PLAYER_TURN) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 mouse_pos = GetMousePosition();
                int clicked_col = mouse_pos.x / tile_size;
                int clicked_row = mouse_pos.y / tile_size;
                int dist = std::max(abs(clicked_col - player.get_x_pos()),
                                    abs(clicked_row - player.get_y_pos()));
                if (dist <= player.get_movement() && player.get_actions() > 0) {
                    player.set_position(clicked_col, clicked_row);
                    player.use_action();
                }
            }

            // end turn manually with SPACE, or auto-end when out of actions
            if (IsKeyPressed(KEY_SPACE) || player.get_actions() <= 0) {
                state = ENEMY_TURN;
            }
        }

        if (state == ENEMY_TURN) {
            // dummy enemy: does nothing, immediately hands back
            player.reset_actions();
            state = PLAYER_TURN;
        }

        // --- DRAW ---
        BeginDrawing();
        ClearBackground(DARKGRAY);
        game_map.draw_map();
        enemy1.draw(tile_size);
        player.draw(tile_size);
        player.draw_range(tile_size, cols, rows);

        // HUD
        DrawText(state == PLAYER_TURN ? "PLAYER TURN" : "ENEMY TURN", 10, 10, 20, WHITE);
        DrawText(TextFormat("Actions: %d", player.get_actions()), 10, 35, 20, WHITE);
        DrawText("SPACE = End Turn", 10, 760, 16, GRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}