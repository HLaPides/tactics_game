#include "renderer.h"
#include "raylib.h"
#include <algorithm>

Renderer::Renderer(const AppConfig& cfg) : config(cfg) {
    camera.zoom     = 1.0f;
    camera.rotation = 0.0f;
}

void Renderer::update_camera(int player_x, int player_y, int map_w) {
    float target_x = player_x * config.tile_size + config.tile_size / 2.0f;
    float half_w   = config.screen_w / 2.0f;
    target_x = std::max(half_w, std::min(target_x, (float)map_w - half_w));

    camera.target = { target_x, config.grid_h / 2.0f };
    camera.offset = { config.screen_w / 2.0f, config.grid_h / 2.0f };
}

const Camera2D& Renderer::get_camera() const {
    return camera;
}

void Renderer::draw_frame(const GameState& state) {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    BeginMode2D(camera);
    draw_map(state);
    draw_range_overlay(state);
    draw_target_highlights(state);
    draw_units(state);
    draw_floating_texts(state);
    EndMode2D();

    draw_attack_preview(state);
    draw_hud(state);
    draw_game_over(state);

    EndDrawing();
}

void Renderer::draw_map(const GameState& state) {
    const auto& tiles     = state.map.get_tiles();
    int         tile_size = config.tile_size;
    for (int row = 0; row < (int)tiles.size(); row++) {
        for (int col = 0; col < (int)tiles[row].size(); col++) {
            int         x = col * tile_size;
            int         y = row * tile_size;
            const Tile& t = tiles[row][col];
            switch (t.type) {
                case TILE_WALL:    DrawRectangle(x, y, tile_size, tile_size, GRAY);      break;
                case TILE_BARREL:  DrawRectangle(x, y, tile_size, tile_size, BROWN);     break;
                case TILE_RAILING: DrawRectangle(x, y, tile_size, tile_size, DARKBROWN); break;
                default:
                    if (t.is_objective)
                        DrawRectangle(x, y, tile_size, tile_size, Fade(GOLD, 0.4f));
                    break;
            }
            DrawRectangleLines(x, y, tile_size, tile_size, BLACK);
        }
    }
}

void Renderer::draw_range_overlay(const GameState& state) {
    if (state.players.empty()) return;
    if (state.mode != ActionMode::NONE) return;

    const unit& active = state.players[state.selected_player];
    if (active.get_actions() <= 0) return;

    int tile_size = config.tile_size;

    std::vector<std::pair<int,int>> blocked;
    for (auto& e : state.enemies)
        if (e.is_alive()) blocked.push_back({e.get_x_pos(), e.get_y_pos()});
    for (int i = 0; i < (int)state.players.size(); i++) {
        if (i == state.selected_player) continue;
        if (state.players[i].is_alive())
            blocked.push_back({state.players[i].get_x_pos(), state.players[i].get_y_pos()});
    }

    auto reachable = get_reachable_tiles(
        active.get_x_pos(), active.get_y_pos(),
        active.get_movement(), state.map, blocked);

    for (auto& [col, row] : reachable)
        DrawRectangleLines(col * tile_size, row * tile_size,
                           tile_size, tile_size, Fade(YELLOW, 0.8f));
}

void Renderer::draw_target_highlights(const GameState& state) {
    if (state.players.empty()) return;
    const unit& active    = state.players[state.selected_player];
    int         tile_size = config.tile_size;

    if (state.mode == ActionMode::SHOOT || state.mode == ActionMode::AIMED_SHOT) {
        for (int i = 0; i < (int)state.enemies.size(); i++) {
            if (!state.enemies[i].is_alive()) continue;
            if (!state.spotted[i]) continue;
            const auto& e = state.enemies[i];
            int dist = std::max(abs(e.get_x_pos() - active.get_x_pos()),
                                abs(e.get_y_pos() - active.get_y_pos()));
            if (dist <= active.get_sight_range() &&
                has_los(active.get_x_pos(), active.get_y_pos(),
                        e.get_x_pos(), e.get_y_pos(), state.map))
                DrawRectangle(e.get_x_pos() * tile_size, e.get_y_pos() * tile_size,
                              tile_size, tile_size, Fade(RED, 0.4f));
        }
    }

    if (state.mode == ActionMode::MELEE) {
        for (const auto& e : state.enemies) {
            if (!e.is_alive()) continue;
            int dist = std::max(abs(e.get_x_pos() - active.get_x_pos()),
                                abs(e.get_y_pos() - active.get_y_pos()));
            if (dist <= 1)
                DrawRectangle(e.get_x_pos() * tile_size, e.get_y_pos() * tile_size,
                              tile_size, tile_size, Fade(ORANGE, 0.5f));
        }
    }

    if (state.mode == ActionMode::DIRTY_TRICK) {
        for (int i = 0; i < (int)state.enemies.size(); i++) {
            if (!state.enemies[i].is_alive()) continue;
            if (!state.spotted[i]) continue;
            const auto& e = state.enemies[i];
            int dist = std::max(abs(e.get_x_pos() - active.get_x_pos()),
                                abs(e.get_y_pos() - active.get_y_pos()));
            if (dist <= active.get_sight_range() &&
                has_los(active.get_x_pos(), active.get_y_pos(),
                        e.get_x_pos(), e.get_y_pos(), state.map))
                DrawRectangle(e.get_x_pos() * tile_size, e.get_y_pos() * tile_size,
                              tile_size, tile_size, Fade(PURPLE, 0.4f));
        }
    }

    if (state.mode == ActionMode::HEAL) {
        for (int i = 0; i < (int)state.players.size(); i++) {
            const auto& p = state.players[i];
            if (!p.is_alive()) continue;
            int dist = std::max(abs(p.get_x_pos() - active.get_x_pos()),
                                abs(p.get_y_pos() - active.get_y_pos()));
            if (dist <= 1)
                DrawRectangle(p.get_x_pos() * tile_size, p.get_y_pos() * tile_size,
                              tile_size, tile_size, Fade(GREEN, 0.4f));
        }
    }

    if (state.mode == ActionMode::RUSH) {
        std::vector<std::pair<int,int>> blocked;
        for (auto& e : state.enemies)
            if (e.is_alive()) blocked.push_back({e.get_x_pos(), e.get_y_pos()});
        for (int i = 0; i < (int)state.players.size(); i++) {
            if (i == state.selected_player) continue;
            if (state.players[i].is_alive())
                blocked.push_back({state.players[i].get_x_pos(), state.players[i].get_y_pos()});
        }
        auto reachable = get_reachable_tiles(
            active.get_x_pos(), active.get_y_pos(),
            active.get_movement(), state.map, blocked);
        for (auto& [col, row] : reachable)
            DrawRectangle(col * tile_size, row * tile_size,
                          tile_size, tile_size, Fade(ORANGE, 0.25f));
    }
}

void Renderer::draw_units(const GameState& state) {
    int tile_size = config.tile_size;

    for (int i = 0; i < (int)state.enemies.size(); i++) {
        if (!state.enemies[i].is_alive()) continue;
        if (!state.spotted[i]) continue;

        const enemy& e = state.enemies[i];
        int x = e.get_x_pos() * tile_size;
        int y = e.get_y_pos() * tile_size;
        DrawRectangle(x, y, tile_size, tile_size, BLUE);

        if (i == state.target_index && state.mode != ActionMode::HEAL)
            DrawRectangleLines(x - 2, y - 2, tile_size + 4, tile_size + 4, YELLOW);

        for (int j = 0; j < e.get_max_hp(); j++) {
            Color pip = j < e.get_hp() ? GREEN : DARKGRAY;
            DrawRectangle(x + 4 + j * 10, y - 10, 8, 6, pip);
        }
    }

    for (int i = 0; i < (int)state.players.size(); i++) {
        if (!state.players[i].is_alive()) continue;
        const unit& p           = state.players[i];
        int         x           = p.get_x_pos() * tile_size;
        int         y           = p.get_y_pos() * tile_size;
        bool        is_selected = (i == state.selected_player);

        Color unit_color = is_selected ? RED : Color{150, 30, 30, 255};
        DrawRectangle(x, y, tile_size, tile_size, unit_color);

        if (is_selected)
            DrawRectangleLines(x, y, tile_size, tile_size, WHITE);

        if (state.mode == ActionMode::HEAL && i == state.target_index)
            DrawRectangleLines(x - 2, y - 2, tile_size + 4, tile_size + 4, GREEN);

        if (p.is_on_overwatch())
            DrawText("OW", x + 8, y + 8, 12, SKYBLUE);

        for (int j = 0; j < p.get_max_hp(); j++) {
            Color pip = j < p.get_hp() ? GREEN : DARKGRAY;
            DrawRectangle(x + 4 + j * 10, y - 10, 8, 6, pip);
        }
    }
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

    const AttackResult& result = state.preview.result;
    const unit*         target = state.preview.target;

    Vector2 world_pos  = {
        (float)(target->get_x_pos() * config.tile_size + config.tile_size / 2),
        (float)(target->get_y_pos() * config.tile_size)
    };
    Vector2 screen_pos = GetWorldToScreen2D(world_pos, camera);

    int panel_w = 200;
    int panel_h = 110;
    int px      = (int)screen_pos.x - panel_w / 2;
    int py      = (int)screen_pos.y - panel_h - 8;

    if (px < 4) px = 4;
    if (px + panel_w > config.screen_w - 4) px = config.screen_w - panel_w - 4;
    if (py < 4) py = (int)screen_pos.y + config.tile_size + 8;
    if (py + panel_h > config.grid_h - 4) py = config.grid_h - panel_h - 4;

    DrawRectangle(px, py, panel_w, panel_h,
                  ColorFromNormalized({0.1f, 0.1f, 0.1f, 0.92f}));
    DrawRectangleLines(px, py, panel_w, panel_h, GRAY);

    Color hit_color = result.hit_chance >= 70 ? GREEN :
                      result.hit_chance >= 40 ? YELLOW : RED;
    DrawText(TextFormat("%d%% to hit", result.hit_chance), px + 8, py + 6,  14, hit_color);
    DrawText(TextFormat("%d%% crit",   result.crit_chance), px + 8, py + 22, 11, ORANGE);

    int       line_y = py + 38;
    const int LINE_H = 12;

    DrawText(TextFormat("aim      %+d", result.aim_component),     px + 8, line_y, 10, LIGHTGRAY);
    line_y += LINE_H;
    if (result.flank_bonus > 0) {
        DrawText(TextFormat("flank    %+d", result.flank_bonus),   px + 8, line_y, 10, GREEN);
        line_y += LINE_H;
    }
    if (result.range_penalty > 0) {
        DrawText(TextFormat("range    -%d", result.range_penalty), px + 8, line_y, 10, RED);
        line_y += LINE_H;
    }
    DrawText(TextFormat("defense  -%d", result.defense_penalty),   px + 8, line_y, 10, LIGHTGRAY);
    line_y += LINE_H;
    if (result.cover_penalty > 0) {
        DrawText(TextFormat("cover    -%d", result.cover_penalty), px + 8, line_y, 10, SKYBLUE);
        line_y += LINE_H;
    }
    if (result.is_flanking)
        DrawText("FLANKED", px + 8, line_y, 10, GREEN);
}

void Renderer::draw_hud(const GameState& state) {
    if (state.players.empty()) return;
    const unit& active = state.players[state.selected_player];
    int         bar_y  = config.screen_h - BAR_HEIGHT;

    DrawRectangle(0, bar_y, config.screen_w, BAR_HEIGHT,
                  ColorFromNormalized({0.13f, 0.13f, 0.13f, 1.0f}));
    DrawLine(0, bar_y, config.screen_w, bar_y, GRAY);

    // unit name + overwatch indicator
    const char* name = state.selected_player < (int)state.player_names.size()
                     ? state.player_names[state.selected_player].c_str()
                     : "Unit";
    DrawText(name, 12, bar_y + 10, 16, WHITE);

    if (active.is_on_overwatch()) {
        DrawText("[OVERWATCH]", 12 + MeasureText(name, 16) + 8,
                 bar_y + 12, 12, SKYBLUE);
    }

    // action pips
    for (int i = 0; i < 2; i++) {
        Color pip = i < active.get_actions() ? SKYBLUE : DARKGRAY;
        DrawCircle(12 + i * 16, bar_y + 36, 5, pip);
    }

    // HP bar
    DrawText("HP", 12, bar_y + 50, 12, GRAY);
    DrawRectangle(30, bar_y + 52, 60, 8, DARKGRAY);
    float hp_frac  = (float)active.get_hp() / active.get_max_hp();
    Color hp_color = hp_frac > 0.5f ? GREEN : (hp_frac > 0.25f ? ORANGE : RED);
    DrawRectangle(30, bar_y + 52, (int)(60 * hp_frac), 8, hp_color);
    DrawText(TextFormat("%d/%d", active.get_hp(), active.get_max_hp()),
             96, bar_y + 50, 12, GRAY);

    DrawLine(130, bar_y + 8, 130, bar_y + BAR_HEIGHT - 8, GRAY);

    // ability buttons
    const auto& abilities  = active.get_abilities();
    bool        no_actions = (active.get_actions() <= 0);

    for (int i = 0; i < (int)abilities.size(); i++) {
        const Ability& ab = abilities[i];
        int bx = BTN_START_X + i * (BTN_W + BTN_GAP);
        int by = bar_y + BTN_Y_OFFSET;

        bool is_active   = (state.mode == ab.get_mode());
        bool on_cooldown = !ab.is_ready();
        bool cant_afford = (active.get_actions() < ab.get_cost() && ab.get_cost() > 0);
        bool unavailable = no_actions || on_cooldown || cant_afford;

        Color border = is_active   ? SKYBLUE
                     : unavailable ? DARKGRAY : GRAY;
        Color label  = is_active   ? SKYBLUE
                     : unavailable ? DARKGRAY : WHITE;
        Color bg     = is_active   ? ColorFromNormalized({0.1f, 0.3f, 0.5f, 1.0f})
                                   : ColorFromNormalized({0.18f, 0.18f, 0.18f, 1.0f});

        DrawRectangle(bx, by, BTN_W, BTN_H, bg);
        DrawRectangleLines(bx, by, BTN_W, BTN_H, border);

        const char* lbl = ab.get_label().c_str();
        DrawText(lbl, bx + BTN_W/2 - MeasureText(lbl, 13)/2, by + 8, 13, label);

        if (on_cooldown) {
            const char* cd = TextFormat("CD: %d", ab.get_cooldown());
            DrawText(cd, bx + BTN_W/2 - MeasureText(cd, 10)/2, by + 44, 10, RED);
        } else if (ab.get_cost() == 0) {
            DrawText("free", bx + BTN_W/2 - MeasureText("free", 10)/2,
                     by + 44, 10, GREEN);
        } else {
            const char* cost = TextFormat("%d action%s", ab.get_cost(),
                                          ab.get_cost() > 1 ? "s" : "");
            DrawText(cost, bx + BTN_W/2 - MeasureText(cost, 10)/2,
                     by + 44, 10, DARKGRAY);
        }
    }

    // right side — turn counter, phase, end turn
    DrawLine(config.screen_w - 120, bar_y + 8,
             config.screen_w - 120, bar_y + BAR_HEIGHT - 8, GRAY);

    const char* turn_label = state.phase == GamePhase::PLAYER_TURN
                           ? "Player turn" : "Enemy turn";
    DrawText(turn_label, config.screen_w - 112, bar_y + 10, 13, GRAY);

    // turn counter
    const char* turn_num = TextFormat("Turn %d", state.turn_count + 1);
    DrawText(turn_num, config.screen_w - 112, bar_y + 26, 11, DARKGRAY);

    Color end_col = (state.phase == GamePhase::PLAYER_TURN) ? WHITE : DARKGRAY;
    DrawRectangleLines(config.screen_w - END_TURN_W - 12, bar_y + END_TURN_Y_OFF,
                       END_TURN_W, END_TURN_H, end_col);
    DrawText("End turn",
             config.screen_w - END_TURN_W - 12 + 10,
             bar_y + END_TURN_Y_OFF + 7, 13, end_col);
}

void Renderer::draw_game_over(const GameState& state) {
    if (state.win_state == WinState::ONGOING) return;

    // dark overlay
    DrawRectangle(0, 0, config.screen_w, config.screen_h,
                  ColorFromNormalized({0.0f, 0.0f, 0.0f, 0.75f}));

    int cx = config.screen_w / 2;
    int cy = config.screen_h / 2;

    if (state.win_state == WinState::VICTORY) {
        const char* title = "MUTINY SUCCESSFUL";
        DrawText(title, cx - MeasureText(title, 40) / 2, cy - 120, 40, GOLD);
        const char* sub = "The gold is yours. Vane never saw it coming.";
        DrawText(sub, cx - MeasureText(sub, 16) / 2, cy - 68, 16, LIGHTGRAY);
    } else {
        const char* title = "MUTINY FAILED";
        DrawText(title, cx - MeasureText(title, 40) / 2, cy - 120, 40, RED);
        const char* sub = "Vane keeps the gold. You keep nothing.";
        DrawText(sub, cx - MeasureText(sub, 16) / 2, cy - 68, 16, LIGHTGRAY);
    }

    // turn count
    const char* turns = TextFormat("Survived %d turn%s",
                                    state.turn_count,
                                    state.turn_count == 1 ? "" : "s");
    DrawText(turns, cx - MeasureText(turns, 14) / 2, cy - 40, 14, GRAY);

    // casualties
    int y_offset = cy - 10;
    bool any_dead = false;
    for (int i = 0; i < (int)state.players.size(); i++) {
        if (state.players[i].is_alive()) continue;
        if (!any_dead) {
            DrawText("Lost in action:", cx - 80, y_offset, 13, GRAY);
            y_offset += 20;
            any_dead = true;
        }
        const char* pname = i < (int)state.player_names.size()
                          ? state.player_names[i].c_str() : "Unknown";
        DrawText(TextFormat("  %s", pname), cx - 80, y_offset, 13, RED);
        y_offset += 18;
    }

    if (!any_dead && state.win_state == WinState::VICTORY) {
        DrawText("Full crew survived.", cx - MeasureText("Full crew survived.", 13) / 2,
                 y_offset, 13, GREEN);
        y_offset += 20;
    }

    // restart prompt
    const char* restart = "Press R to try again";
    DrawText(restart, cx - MeasureText(restart, 16) / 2,
             cy + 80, 16, WHITE);
}