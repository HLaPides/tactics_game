#include "raylib.h"
#include "map.h"
#include "units/unit.h"
#include "units/enemy.h"
#include "hud.h"
#include "resource_dir.h"
#include <algorithm>
#include <vector>

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    const int SCREEN_W = 640;
    const int SCREEN_H = 720;
    InitWindow(SCREEN_W, SCREEN_H, "Xcom Knockoff");

    map game_map = map(640, 640, 64);
    int tile_size = game_map.get_tile_size();
    int cols      = game_map.getCols();
    int rows      = game_map.getRows();

    //                        x  y  mv hp  sr  sd  md
    unit player = unit       (1, 2,  3, 10,  6,  2,  3);
    std::vector<enemy> enemies;
    enemies.push_back(enemy  (7, 5,  2, 5,  3,  1,  1));
    enemies.push_back(enemy  (6, 2,  2, 5,  3,  1,  1));

    GameState  state        = PLAYER_TURN;
    ActionMode mode         = MODE_NONE;
    hud        game_hud     = hud(SCREEN_W, SCREEN_H);

    int         enemy_index = 0;
    float       enemy_timer = 0.0f;
    const float ENEMY_DELAY = 0.4f;

    while (!WindowShouldClose()) {

        float dt           = GetFrameTime();
        Vector2 mouse      = GetMousePosition();
        int mx             = (int)(mouse.x / tile_size);
        int my             = (int)(mouse.y / tile_size);
        bool mouse_on_grid = mouse.y < 640;

        // --- INPUT ---
        if (state == PLAYER_TURN) {

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !mouse_on_grid) {
                if (game_hud.clicked_shoot(mouse) && player.get_actions() > 0)
                    mode = (mode == MODE_SHOOT) ? MODE_NONE : MODE_SHOOT;

                if (game_hud.clicked_melee(mouse) && player.get_actions() > 0)
                    mode = (mode == MODE_MELEE) ? MODE_NONE : MODE_MELEE;

                if (game_hud.clicked_end_turn(mouse)) {
                    mode        = MODE_NONE;
                    state       = ENEMY_TURN;
                    enemy_index = 0;
                    enemy_timer = 0.0f;
                }
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mouse_on_grid) {
                if (mode == MODE_SHOOT) {
                    for (auto& e : enemies) {
                        if (!e.is_alive()) continue;
                        int dist = std::max(abs(mx - player.get_x_pos()),
                                            abs(my - player.get_y_pos()));
                        if (e.get_x_pos() == mx && e.get_y_pos() == my && dist <= player.get_shoot_range()) {
                            e.take_damage(player.get_shoot_damage());
                            player.use_action();
                            mode = MODE_NONE;
                            break;
                        }
                    }
                } else if (mode == MODE_MELEE) {
                    for (auto& e : enemies) {
                        if (!e.is_alive()) continue;
                        int dist = std::max(abs(mx - player.get_x_pos()),
                                            abs(my - player.get_y_pos()));
                        if (e.get_x_pos() == mx && e.get_y_pos() == my && dist <= 1) {
                            e.take_damage(player.get_melee_damage());
                            player.use_action();
                            mode = MODE_NONE;
                            break;
                        }
                    }
                } else {
                    int dist          = std::max(abs(mx - player.get_x_pos()),
                                                 abs(my - player.get_y_pos()));
                    bool tile_blocked = false;
                    for (auto& e : enemies) {
                        if (e.is_alive() && e.get_x_pos() == mx && e.get_y_pos() == my) {
                            tile_blocked = true;
                            break;
                        }
                    }
                    if (dist <= player.get_movement() && player.get_actions() > 0 && !tile_blocked) {
                        player.set_position(mx, my);
                        player.use_action();
                    }
                }
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_ESCAPE))
                mode = MODE_NONE;

            if (player.get_actions() <= 0 || IsKeyPressed(KEY_SPACE)) {
                mode        = MODE_NONE;
                state       = ENEMY_TURN;
                enemy_index = 0;
                enemy_timer = 0.0f;
            }

            // player death — freeze game, game over screen later
            if (!player.is_alive()) {
                mode        = MODE_NONE;
                state       = ENEMY_TURN;
                enemy_index = (int)enemies.size();
            }
        }

        // enemy turn — one enemy acts per delay tick
        if (state == ENEMY_TURN) {
            enemy_timer += dt;
            if (enemy_timer >= ENEMY_DELAY) {
                enemy_timer = 0.0f;

                while (enemy_index < (int)enemies.size() && !enemies[enemy_index].is_alive())
                    enemy_index++;

                if (enemy_index < (int)enemies.size()) {
                    enemies[enemy_index].act(player, enemies);
                    enemy_index++;
                } else {
                    // all enemies have acted
                    if (player.is_alive()) {
                        player.reset_actions();
                        state = PLAYER_TURN;
                    }
                    // if player is dead we stay in ENEMY_TURN indefinitely — game over
                }
            }
        }

        // cursor
        if (mode == MODE_SHOOT || mode == MODE_MELEE)
            SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
        else
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);

        // --- DRAW ---
        BeginDrawing();
        ClearBackground(DARKGRAY);
        game_map.draw_map();

        if (mode == MODE_NONE)
            player.draw_range(tile_size, cols, rows);

        if (mode == MODE_SHOOT) {
            for (auto& e : enemies) {
                if (!e.is_alive()) continue;
                int dist = std::max(abs(e.get_x_pos() - player.get_x_pos()),
                                    abs(e.get_y_pos() - player.get_y_pos()));
                if (dist <= player.get_shoot_range())
                    DrawRectangle(e.get_x_pos() * tile_size, e.get_y_pos() * tile_size,
                                  tile_size, tile_size, Fade(RED, 0.4f));
            }
        }

        if (mode == MODE_MELEE) {
            for (auto& e : enemies) {
                if (!e.is_alive()) continue;
                int dist = std::max(abs(e.get_x_pos() - player.get_x_pos()),
                                    abs(e.get_y_pos() - player.get_y_pos()));
                if (dist <= 1)
                    DrawRectangle(e.get_x_pos() * tile_size, e.get_y_pos() * tile_size,
                                  tile_size, tile_size, Fade(ORANGE, 0.5f));
            }
        }

        for (auto& e : enemies) {
            e.draw(tile_size);
            e.draw_hp(tile_size);
        }
        player.draw(tile_size);

        // game over overlay
        if (!player.is_alive())
            DrawText("GAME OVER", SCREEN_W/2 - MeasureText("GAME OVER", 40)/2, SCREEN_H/2 - 60, 40, RED);

        game_hud.draw(player, state, mode);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}