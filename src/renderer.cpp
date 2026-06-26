#include "renderer.h"
#include "raylib.h"
#include <algorithm>

Renderer::Renderer(const AppConfig& config) : config(config) {}

void Renderer::draw_frame(const GameState& state) {
    BeginDrawing();
    ClearBackground(DARKGRAY);
    draw_map(state);
    draw_range_overlay(state);
    draw_target_highlights(state);
    draw_units(state);
    draw_floating_texts(state);
    draw_attack_preview(state);
    draw_game_over(state);
    draw_hud(state);
    EndDrawing();
}

void Renderer::draw_map(const GameState& state) {
    const auto& tiles     = state.map.get_tiles();
    int         tile_size = config.tile_size;
    for (int row = 0; row < (int)tiles.size(); row++) {
        for (int col = 0; col < (int)tiles[row].size(); col++) {
            int          x = col * tile_size;
            int          y = row * tile_size;
            const Tile&  t = tiles[row][col];
            switch (t.type) {
                case TILE_WALL:    DrawRectangle(x, y, tile_size, tile_size, GRAY);      break;
                case TILE_BARREL:  DrawRectangle(x, y, tile_size, tile_size, BROWN);     break;
                case TILE_RAILING: DrawRectangle(x, y, tile_size, tile_size, DARKBROWN); break;
                default: break;
            }
            DrawRectangleLines(x, y, tile_size, tile_size, BLACK);
        }
    }
}

void Renderer::draw_range_overlay(const GameState& state) {
    if (state.mode != ActionMode::NONE) return;
    if (state.player.get_actions() <= 0) return;

    int tile_size = config.tile_size;
    int cols      = state.map.getCols();
    int rows      = state.map.getRows();
    int px        = state.player.get_x_pos();
    int py        = state.player.get_y_pos();
    int mv        = state.player.get_movement();

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int dist = std::max(abs(col - px), abs(row - py));
            if (dist <= mv && dist > 0)
                DrawRectangleLines(col * tile_size, row * tile_size,
                                   tile_size, tile_size, Fade(YELLOW, 0.8f));
        }
    }
}

void Renderer::draw_target_highlights(const GameState& state) {
    int tile_size = config.tile_size;

    if (state.mode == ActionMode::SHOOT) {
        for (const auto& e : state.enemies) {
            if (!e.is_alive()) continue;
            int dist = std::max(abs(e.get_x_pos() - state.player.get_x_pos()),
                                abs(e.get_y_pos() - state.player.get_y_pos()));
            if (dist <= state.player.get_shoot_range())
                DrawRectangle(e.get_x_pos() * tile_size, e.get_y_pos() * tile_size,
                              tile_size, tile_size, Fade(RED, 0.4f));
        }
    }

    if (state.mode == ActionMode::MELEE) {
        for (const auto& e : state.enemies) {
            if (!e.is_alive()) continue;
            int dist = std::max(abs(e.get_x_pos() - state.player.get_x_pos()),
                                abs(e.get_y_pos() - state.player.get_y_pos()));
            if (dist <= 1)
                DrawRectangle(e.get_x_pos() * tile_size, e.get_y_pos() * tile_size,
                              tile_size, tile_size, Fade(ORANGE, 0.5f));
        }
    }
}

void Renderer::draw_units(const GameState& state) {
    int tile_size = config.tile_size;

    for (int i = 0; i < (int)state.enemies.size(); i++) {
        if (!state.enemies[i].is_alive()) continue;
        if (!state.spotted[i]) continue;  // not spotted — don't draw

        const enemy& e = state.enemies[i];
        int x = e.get_x_pos() * tile_size;
        int y = e.get_y_pos() * tile_size;
        DrawRectangle(x, y, tile_size, tile_size, BLUE);

        for (int j = 0; j < e.get_max_hp(); j++) {
            Color pip = j < e.get_hp() ? GREEN : DARKGRAY;
            DrawRectangle(x + 4 + j * 10, y - 10, 8, 6, pip);
        }
    }

    DrawRectangle(state.player.get_x_pos() * tile_size,
                  state.player.get_y_pos() * tile_size,
                  tile_size, tile_size, RED);
}

void Renderer::draw_floating_texts(const GameState& state) {
    int tile_size = config.tile_size;
    for (const auto& ft : state.floating_texts.get_all()) {
        float alpha  = 1.0f - (ft.timer / ft.duration);
        float rise   = ft.timer * 30.0f;
        Color c      = Fade(ft.color, alpha);
        int   text_x = ft.col * tile_size + tile_size / 2
                     - MeasureText(ft.text.c_str(), 16) / 2;
        int   text_y = (int)(ft.row * tile_size - rise);
        DrawText(ft.text.c_str(), text_x, text_y, 16, c);
    }
}

void Renderer::draw_attack_preview(const GameState& state) {
    if (!state.preview.active || state.preview.target == nullptr) return;

    const AttackResult& result    = state.preview.result;
    const unit*         target    = state.preview.target;
    int                 tile_size = config.tile_size;

    int tx = target->get_x_pos() * tile_size;
    int ty = target->get_y_pos() * tile_size;

    int panel_w = 200;
    int panel_h = 100;
    int px      = tx - panel_w / 2 + tile_size / 2;
    int py      = ty - panel_h - 4;

    if (px < 0) px = 0;
    if (px + panel_w > config.screen_w) px = config.screen_w - panel_w;
    if (py < 0) py = ty + tile_size + 4;

    DrawRectangle(px, py, panel_w, panel_h,
                  ColorFromNormalized({0.1f, 0.1f, 0.1f, 0.9f}));
    DrawRectangleLines(px, py, panel_w, panel_h, GRAY);

    Color hit_color = result.hit_chance >= 70 ? GREEN :
                      result.hit_chance >= 40 ? YELLOW : RED;
    DrawText(TextFormat("%d%% to hit", result.hit_chance), px + 8, py + 6,  14, hit_color);
    DrawText(TextFormat("%d%% crit",   result.crit_chance), px + 8, py + 22, 11, ORANGE);

    int line_y      = py + 38;
    const int LINE_H = 12;

    DrawText(TextFormat("aim      %+d", result.aim_component),    px + 8, line_y, 10, LIGHTGRAY);
    line_y += LINE_H;
    if (result.flank_bonus > 0) {
        DrawText(TextFormat("flank    %+d", result.flank_bonus),  px + 8, line_y, 10, GREEN);
        line_y += LINE_H;
    }
    if (result.range_penalty > 0) {
        DrawText(TextFormat("range    -%d", result.range_penalty),px + 8, line_y, 10, RED);
        line_y += LINE_H;
    }
    DrawText(TextFormat("defense  -%d", result.defense_penalty),  px + 8, line_y, 10, LIGHTGRAY);
    line_y += LINE_H;
    if (result.cover_penalty > 0) {
        DrawText(TextFormat("cover    -%d", result.cover_penalty),px + 8, line_y, 10, SKYBLUE);
        line_y += LINE_H;
    }
    if (result.is_flanking)
        DrawText("FLANKED", px + 8, line_y, 10, GREEN);
}

void Renderer::draw_hud(const GameState& state) {
    int bar_y = config.screen_h - BAR_HEIGHT;

    DrawRectangle(0, bar_y, config.screen_w, BAR_HEIGHT,
                  ColorFromNormalized({0.13f, 0.13f, 0.13f, 1.0f}));
    DrawLine(0, bar_y, config.screen_w, bar_y, GRAY);

    // unit info
    DrawText("Bosun", 12, bar_y + 10, 16, WHITE);
    for (int i = 0; i < 2; i++) {
        Color pip = i < state.player.get_actions() ? SKYBLUE : DARKGRAY;
        DrawCircle(12 + i * 16, bar_y + 36, 5, pip);
    }
    DrawText("HP", 12, bar_y + 50, 12, GRAY);
    DrawRectangle(30, bar_y + 52, 60, 8, DARKGRAY);
    float hp_frac  = (float)state.player.get_hp() / state.player.get_max_hp();
    Color hp_color = hp_frac > 0.5f ? GREEN : (hp_frac > 0.25f ? ORANGE : RED);
    DrawRectangle(30, bar_y + 52, (int)(60 * hp_frac), 8, hp_color);
    DrawText(TextFormat("%d/%d", state.player.get_hp(), state.player.get_max_hp()),
             96, bar_y + 50, 12, GRAY);
    DrawLine(130, bar_y + 8, 130, bar_y + BAR_HEIGHT - 8, GRAY);

    // action buttons
    const char* btn_labels[] = { "Shoot", "Melee" };
    ActionMode  btn_modes[]  = { ActionMode::SHOOT, ActionMode::MELEE };
    for (int i = 0; i < 2; i++) {
        int bx = BTN_START_X + i * (BTN_W + BTN_GAP);
        int by = bar_y + BTN_Y_OFFSET;

        bool active     = (state.mode == btn_modes[i]);
        bool no_actions = (state.player.get_actions() <= 0);

        Color border = active ? SKYBLUE : (no_actions ? DARKGRAY : GRAY);
        Color label  = active ? SKYBLUE : (no_actions ? DARKGRAY : WHITE);
        Color bg     = active ? ColorFromNormalized({0.1f, 0.3f, 0.5f, 1.0f})
                              : ColorFromNormalized({0.18f, 0.18f, 0.18f, 1.0f});

        DrawRectangle(bx, by, BTN_W, BTN_H, bg);
        DrawRectangleLines(bx, by, BTN_W, BTN_H, border);
        DrawText(btn_labels[i], bx + BTN_W/2 - MeasureText(btn_labels[i], 14)/2,
                 by + 10, 14, label);
        DrawText("1 action", bx + BTN_W/2 - MeasureText("1 action", 10)/2,
                 by + 44, 10, DARKGRAY);
    }

    DrawLine(config.screen_w - 120, bar_y + 8,
             config.screen_w - 120, bar_y + BAR_HEIGHT - 8, GRAY);

    // turn info + end turn
    const char* turn_label = state.phase == GamePhase::PLAYER_TURN ? "Player turn" : "Enemy turn";
    DrawText(turn_label, config.screen_w - 112, bar_y + 10, 13, GRAY);

    Color end_col = (state.phase == GamePhase::PLAYER_TURN) ? WHITE : DARKGRAY;
    DrawRectangleLines(config.screen_w - 112, bar_y + 30, 100, 28, end_col);
    DrawText("End turn", config.screen_w - 112 + 10, bar_y + 37, 13, end_col);
}

void Renderer::draw_game_over(const GameState& state) {
    if (state.player.is_alive()) return;
    DrawText("GAME OVER",
             config.screen_w / 2 - MeasureText("GAME OVER", 40) / 2,
             config.screen_h / 2 - 60, 40, RED);
}