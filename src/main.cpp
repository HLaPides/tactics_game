#include "raylib.h"
#include "map.h"
#include "units/unit.h"
#include "units/enemy.h"
#include "resource_dir.h"
#include <algorithm>
#include <vector>

enum GameState  { PLAYER_TURN, ENEMY_TURN };
enum ActionMode { MODE_NONE, MODE_SHOOT, MODE_MELEE };

// --- action bar layout constants ---
const int BAR_HEIGHT    = 80;
const int BTN_W         = 80;
const int BTN_H         = 60;
const int BTN_Y_OFFSET  = 10;

void draw_action_bar(int screen_w, int screen_h, int player_hp, int player_max_hp,
                     int player_actions, GameState state, ActionMode mode) {
    int bar_y = screen_h - BAR_HEIGHT;

    // bar background
    DrawRectangle(0, bar_y, screen_w, BAR_HEIGHT, ColorFromNormalized({0.13f, 0.13f, 0.13f, 1.0f}));
    DrawLine(0, bar_y, screen_w, bar_y, GRAY);

    // --- unit info (left) ---
    DrawText("Bosun", 12, bar_y + 10, 16, WHITE);

    // action pips
    for (int i = 0; i < 2; i++) {
        Color pip = i < player_actions ? SKYBLUE : DARKGRAY;
        DrawCircle(12 + i * 16, bar_y + 36, 5, pip);
    }

    // HP bar
    DrawText("HP", 12, bar_y + 50, 12, GRAY);
    DrawRectangle(30, bar_y + 52, 60, 8, DARKGRAY);
    float hp_frac = (float)player_hp / player_max_hp;
    Color hp_color = hp_frac > 0.5f ? GREEN : (hp_frac > 0.25f ? ORANGE : RED);
    DrawRectangle(30, bar_y + 52, (int)(60 * hp_frac), 8, hp_color);
    DrawText(TextFormat("%d/%d", player_hp, player_max_hp), 96, bar_y + 50, 12, GRAY);

    // divider
    DrawLine(130, bar_y + 8, 130, bar_y + BAR_HEIGHT - 8, GRAY);

    // --- action buttons (middle) ---
    const char* btn_labels[]  = { "Shoot",  "Melee"  };
    ActionMode  btn_modes[]   = { MODE_SHOOT, MODE_MELEE };
    int btn_start_x = 148;

    for (int i = 0; i < 2; i++) {
        int bx = btn_start_x + i * (BTN_W + 8);
        int by = bar_y + BTN_Y_OFFSET;

        bool active   = (mode == btn_modes[i]);
        bool no_actions = (player_actions <= 0);

        Color border = active ? SKYBLUE : (no_actions ? DARKGRAY : GRAY);
        Color label  = active ? SKYBLUE : (no_actions ? DARKGRAY : WHITE);
        Color bg     = active ? ColorFromNormalized({0.1f, 0.3f, 0.5f, 1.0f})
                              : ColorFromNormalized({0.18f, 0.18f, 0.18f, 1.0f});

        DrawRectangle(bx, by, BTN_W, BTN_H, bg);
        DrawRectangleLines(bx, by, BTN_W, BTN_H, border);
        DrawText(btn_labels[i], bx + BTN_W/2 - MeasureText(btn_labels[i], 14)/2, by + 10, 14, label);
        DrawText("1 action", bx + BTN_W/2 - MeasureText("1 action", 10)/2, by + 44, 10, DARKGRAY);
    }

    // divider
    DrawLine(screen_w - 120, bar_y + 8, screen_w - 120, bar_y + BAR_HEIGHT - 8, GRAY);

    // --- turn info + end turn (right) ---
    const char* turn_label = state == PLAYER_TURN ? "Player turn" : "Enemy turn";
    DrawText(turn_label, screen_w - 112, bar_y + 10, 13, GRAY);

    Color end_col = (state == PLAYER_TURN) ? WHITE : DARKGRAY;
    DrawRectangleLines(screen_w - 112, bar_y + 30, 100, 28, end_col);
    DrawText("End turn", screen_w - 112 + 10, bar_y + 37, 13, end_col);
}

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    const int SCREEN_W = 640;
    const int SCREEN_H = 640;
    InitWindow(SCREEN_W, SCREEN_H, "Xcom Knockoff");

    map game_map = map(640, 640, 64);
    int tile_size = game_map.get_tile_size();
    int cols      = game_map.getCols();
    int rows      = game_map.getRows();

    unit player   = unit(1, 2, 3, 5);

    // store enemies in a vector so dead ones can be skipped easily
    std::vector<enemy> enemies;
    enemies.push_back(enemy(7, 5, 2, 5));
    enemies.push_back(enemy(6, 2, 2, 5));

    GameState  state = PLAYER_TURN;
    ActionMode mode  = MODE_NONE;

    const int SHOOT_DAMAGE = 1;
    const int SHOOT_RANGE  = 6;  // in tiles, Chebyshev
    const int MELEE_RANGE  = 1;

    while (!WindowShouldClose()) {

        // --- INPUT ---
        if (state == PLAYER_TURN) {

            Vector2 mouse = GetMousePosition();
            int mx = (int)(mouse.x / tile_size);
            int my = (int)(mouse.y / tile_size);
            bool mouse_on_grid = mouse.y < (SCREEN_H - BAR_HEIGHT);

            // button clicks
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !mouse_on_grid) {
                int bar_y     = SCREEN_H - BAR_HEIGHT;
                int btn_start = 148;

                // shoot button
                if (mouse.x >= btn_start && mouse.x < btn_start + BTN_W &&
                    mouse.y >= bar_y + BTN_Y_OFFSET &&
                    mouse.y < bar_y + BTN_Y_OFFSET + BTN_H &&
                    player.get_actions() > 0) {
                    mode = (mode == MODE_SHOOT) ? MODE_NONE : MODE_SHOOT;
                }

                // melee button
                int bx2 = btn_start + BTN_W + 8;
                if (mouse.x >= bx2 && mouse.x < bx2 + BTN_W &&
                    mouse.y >= bar_y + BTN_Y_OFFSET &&
                    mouse.y < bar_y + BTN_Y_OFFSET + BTN_H &&
                    player.get_actions() > 0) {
                    mode = (mode == MODE_MELEE) ? MODE_NONE : MODE_MELEE;
                }

                // end turn button
                int end_x = SCREEN_W - 112;
                if (mouse.x >= end_x && mouse.x < end_x + 100 &&
                    mouse.y >= bar_y + 30 && mouse.y < bar_y + 58) {
                    mode  = MODE_NONE;
                    state = ENEMY_TURN;
                }
            }

            // grid clicks
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mouse_on_grid) {
                if (mode == MODE_SHOOT) {
                    for (auto& e : enemies) {
                        if (!e.is_alive()) continue;
                        int dist = std::max(abs(mx - player.get_x_pos()),
                                            abs(my - player.get_y_pos()));
                        if (e.get_x_pos() == mx && e.get_y_pos() == my && dist <= SHOOT_RANGE) {
                            e.take_damage(SHOOT_DAMAGE);
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
                        if (e.get_x_pos() == mx && e.get_y_pos() == my && dist <= MELEE_RANGE) {
                            e.take_damage(2);  // melee hits harder
                            player.use_action();
                            mode = MODE_NONE;
                            break;
                        }
                    }
                } else {
                    // normal move
                    int dist = std::max(abs(mx - player.get_x_pos()),
                                        abs(my - player.get_y_pos()));
                    if (dist <= player.get_movement() && player.get_actions() > 0) {
                        player.set_position(mx, my);
                        player.use_action();
                    }
                }
            }

            // cancel targeting with right-click or Escape
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_ESCAPE)) {
                mode = MODE_NONE;
            }

            // auto end turn when out of actions
            if (player.get_actions() <= 0) {
                mode  = MODE_NONE;
                state = ENEMY_TURN;
            }

            // SPACE to end turn manually
            if (IsKeyPressed(KEY_SPACE)) {
                mode  = MODE_NONE;
                state = ENEMY_TURN;
            }
        }

        if (state == ENEMY_TURN) {
            player.reset_actions();
            state = PLAYER_TURN;
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

        // draw movement range only in default mode
        if (mode == MODE_NONE)
            player.draw_range(tile_size, cols, rows);

        // highlight valid shoot targets in red
        if (mode == MODE_SHOOT) {
            for (auto& e : enemies) {
                if (!e.is_alive()) continue;
                int dist = std::max(abs(e.get_x_pos() - player.get_x_pos()),
                                    abs(e.get_y_pos() - player.get_y_pos()));
                if (dist <= SHOOT_RANGE) {
                    DrawRectangle(e.get_x_pos() * tile_size, e.get_y_pos() * tile_size,
                                  tile_size, tile_size, Fade(RED, 0.4f));
                }
            }
        }

        // highlight valid melee targets
        if (mode == MODE_MELEE) {
            for (auto& e : enemies) {
                if (!e.is_alive()) continue;
                int dist = std::max(abs(e.get_x_pos() - player.get_x_pos()),
                                    abs(e.get_y_pos() - player.get_y_pos()));
                if (dist <= MELEE_RANGE) {
                    DrawRectangle(e.get_x_pos() * tile_size, e.get_y_pos() * tile_size,
                                  tile_size, tile_size, Fade(ORANGE, 0.5f));
                }
            }
        }

        for (auto& e : enemies) {
            e.draw(tile_size);
            e.draw_hp(tile_size);
        }
        player.draw(tile_size);

        draw_action_bar(SCREEN_W, SCREEN_H, player.get_hp(), player.get_max_hp(),
                        player.get_actions(), state, mode);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}