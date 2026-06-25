#include "raylib.h"
#include "map.h"
#include "units/unit.h"
#include "units/enemy.h"
#include "hud.h"
#include "combat.h"
#include "resource_dir.h"
#include <algorithm>
#include <vector>

int main() {
    const int SCREEN_W   = 1280;
    const int SCREEN_H   = 720;
    const int GRID_H     = 640;

    InitWindow(SCREEN_W, SCREEN_H, "Xcom Knockoff");

    const char* levels[] = { "levels/ship_01.txt" };
    int level_count      = sizeof(levels) / sizeof(levels[0]);
    int chosen           = GetRandomValue(0, level_count - 1);

    map game_map = map(32);
    if (!game_map.load(levels[chosen])) {
        CloseWindow();
        return 1;
    }

    int tile_size = game_map.get_tile_size();
    int cols      = game_map.getCols();
    int rows      = game_map.getRows();

    auto player_spawns = game_map.get_player_spawns();
    auto enemy_spawns  = game_map.get_enemy_spawns();

    for (int i = (int)enemy_spawns.size() - 1; i > 0; i--) {
        int j = GetRandomValue(0, i);
        std::swap(enemy_spawns[i], enemy_spawns[j]);
    }

    unit player = unit(player_spawns[0].col, player_spawns[0].row, 4, 10, 75, 15, 6, 2, 3);

    std::vector<enemy> enemies;
    int enemy_count = std::min(6, (int)enemy_spawns.size());
    for (int i = 0; i < enemy_count; i++) {
        enemies.push_back(enemy(enemy_spawns[i].col, enemy_spawns[i].row, 2, 5, 60, 10, 3, 1, 1));
    }

    GameState  state        = PLAYER_TURN;
    ActionMode mode         = MODE_NONE;
    hud        game_hud     = hud(SCREEN_W, SCREEN_H);

    int         enemy_index = 0;
    float       enemy_timer = 0.0f;
    const float ENEMY_DELAY = 0.4f;

    // hover preview state
    enemy*       hovered_enemy  = nullptr;
    AttackResult hover_result   = {};

    while (!WindowShouldClose()) {

        float   dt            = GetFrameTime();
        Vector2 mouse         = GetMousePosition();
        int     mx            = (int)(mouse.x / tile_size);
        int     my            = (int)(mouse.y / tile_size);
        bool    mouse_on_grid = mouse.y < GRID_H;

        // update hover preview
        hovered_enemy = nullptr;
        if ((mode == MODE_SHOOT || mode == MODE_MELEE) && mouse_on_grid) {
            for (auto& e : enemies) {
                if (!e.is_alive()) continue;
                if (e.get_x_pos() == mx && e.get_y_pos() == my) {
                    int dist = std::max(abs(mx - player.get_x_pos()),
                                        abs(my - player.get_y_pos()));
                    int range = (mode == MODE_SHOOT) ? player.get_shoot_range() : 1;
                    if (dist <= range) {
                        hovered_enemy = &e;
                        hover_result  = resolve_attack(player, e, game_map,
                                            mode == MODE_SHOOT ? player.get_shoot_damage()
                                                               : player.get_melee_damage());
                    }
                    break;
                }
            }
        }

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
                            AttackResult result = resolve_attack(player, e, game_map, player.get_shoot_damage());
                            e.take_damage(result.damage);
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
                            AttackResult result = resolve_attack(player, e, game_map, player.get_melee_damage());
                            e.take_damage(result.damage);
                            player.use_action();
                            mode = MODE_NONE;
                            break;
                        }
                    }
                } else {
                    int  dist         = std::max(abs(mx - player.get_x_pos()),
                                                 abs(my - player.get_y_pos()));
                    bool tile_blocked = false;
                    for (auto& e : enemies) {
                        if (e.is_alive() && e.get_x_pos() == mx && e.get_y_pos() == my) {
                            tile_blocked = true;
                            break;
                        }
                    }
                    if (dist <= player.get_movement() && player.get_actions() > 0
                        && !tile_blocked && game_map.is_walkable(mx, my)) {
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

            if (!player.is_alive()) {
                mode        = MODE_NONE;
                state       = ENEMY_TURN;
                enemy_index = (int)enemies.size();
            }
        }

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
                    if (player.is_alive()) {
                        player.reset_actions();
                        state = PLAYER_TURN;
                    }
                }
            }
        }

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

        // draw hover preview
        if (hovered_enemy != nullptr)
            game_hud.draw_attack_preview(*hovered_enemy, hover_result, tile_size);

        if (!player.is_alive())
            DrawText("GAME OVER", SCREEN_W/2 - MeasureText("GAME OVER", 40)/2, SCREEN_H/2 - 60, 40, RED);

        game_hud.draw(player, state, mode);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}