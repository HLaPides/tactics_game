#include "renderer.h"
#include "raylib.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstdio>

// ─── tooltip data ─────────────────────────────────────────────────────────────

struct TooltipData {
    const char* description;
    const char* flavor;
};

static TooltipData get_tooltip(const std::string& id) {
    if (id == "shoot")       return { "Fire at an enemy in range.",
                                      "A pirate's answer to most problems." };
    if (id == "melee")       return { "Strike an adjacent enemy.",
                                      "They're even uglier up close." };
    if (id == "heal")        return { "Restore HP to an adjacent ally.",
                                      "It does the job." };
    if (id == "rush")        return { "Move without spending an action.",
                                      "The best offense is a good defense." };
    if (id == "dirty_trick") return { "Reduce target's aim by 30 for one turn.",
                                      "If it works it works." };
    if (id == "aimed_shot")  return { "2 actions; improved aim and damage.",
                                      "One shot. Make it count." };
    return { "", "" };
}

// ─── constructor / destructor ─────────────────────────────────────────────────

Renderer::Renderer(const AppConfig& cfg) : config(cfg) {
    camera.zoom     = 1.0f;
    camera.rotation = 0.0f;

    hud_texture = LoadTexture("src/assets/icons/hud.png");

    icons.load("shoot",       "src/assets/icons/shoot.png");
    icons.load("melee",       "src/assets/icons/melee.png");
    icons.load("aimed_shot",  "src/assets/icons/aimed_shot.png");
    icons.load("rush",        "src/assets/icons/rush.png");
    icons.load("heal",        "src/assets/icons/heal.png");
    icons.load("dirty_trick", "src/assets/icons/dirty_trick.png");

    tileset = LoadTexture("levels/tileset_03.png");

    load_portraits();
    load_sprites();
}

Renderer::~Renderer() {
    if (hud_texture.id != 0) UnloadTexture(hud_texture);
    if (tileset.id != 0)     UnloadTexture(tileset);
    for (auto& [name, tex] : portraits)
        if (tex.id != 0) UnloadTexture(tex);
    for (auto& [name, tex] : unit_sprites)
        if (tex.id != 0) UnloadTexture(tex);
    for (auto& [name, tex] : enemy_sprites)
        if (tex.id != 0) UnloadTexture(tex);
}

void Renderer::load_portraits() {
    auto load = [&](const std::string& name, const std::string& path) {
        Texture2D tex = LoadTexture(path.c_str());
        if (tex.id != 0) portraits[name] = tex;
    };
    load("Swashbuckler", "src/assets/portraits/pirate_01.png");
    load("Bosun",        "src/assets/portraits/pirate_02.png");
    load("Sharpshooter", "src/assets/portraits/pirate_03.png");
    load("Medic",        "src/assets/portraits/pirate_04.png");
}

void Renderer::load_sprites() {
    auto load = [&](std::unordered_map<std::string, Texture2D>& map,
                    const std::string& key, const std::string& path) {
        Texture2D tex = LoadTexture(path.c_str());
        if (tex.id != 0) map[key] = tex;
    };
    load(unit_sprites,  "Bosun",        "src/assets/sprites/bosun_sprite.png");
    load(unit_sprites,  "Sharpshooter", "src/assets/sprites/sharpshooter_sprite.png");
    load(unit_sprites,  "Medic",        "src/assets/sprites/medic_sprite.png");
    load(unit_sprites,  "Swashbuckler", "src/assets/sprites/swashbuckler_sprite.png");
    load(enemy_sprites, "soldier",    "src/assets/sprites/soldier.png");
    load(enemy_sprites, "guard",      "src/assets/sprites/guard.png");
    load(enemy_sprites, "capt_vane",  "src/assets/sprites/capt_vane.png");
}

// ─── portrait ─────────────────────────────────────────────────────────────────

void Renderer::draw_portrait(const GameState& state, int bar_y) {
    if (state.players.empty()) return;

    const std::string& pname = state.selected_player < (int)state.player_names.size()
                             ? state.player_names[state.selected_player]
                             : "";

    int port_h = BAR_HEIGHT;
    int port_w = port_h;
    int port_x = PORTRAIT_X;
    int port_y = bar_y;

    auto it = portraits.find(pname);
    if (it != portraits.end() && it->second.id != 0) {
        Texture2D& tex = it->second;
        DrawTexturePro(tex,
            { 0, 0, (float)tex.width, (float)tex.height },
            { (float)port_x, (float)port_y, (float)port_w, (float)port_h },
            { 0, 0 }, 0.0f, WHITE);
    } else {
        DrawRectangle(port_x, port_y, port_w, port_h, Color{80, 30, 30, 255});
        const char* init = pname.empty() ? "?" : pname.substr(0,1).c_str();
        DrawText(init, port_x + port_w/2 - MeasureText(init, 32)/2,
                 port_y + port_h/2 - 16, 32, WHITE);
    }

    DrawRectangleLines(port_x, port_y, port_w, port_h, Color{160, 130, 60, 255});

    int text_x = port_x + port_w + 12;
    const char* name = pname.empty() ? "Unit" : pname.c_str();
    DrawText(name, text_x, bar_y + 8, 22, Color{60, 35, 15, 255});
}

// ─── tooltip ──────────────────────────────────────────────────────────────────

void Renderer::draw_tooltip(const std::string& ability_id, int bx, int by) const {
    TooltipData tip = get_tooltip(ability_id);
    if (tip.description[0] == '\0') return;

    int panel_w = 220;
    int panel_h = 56;
    int px      = bx + BTN_W / 2 - panel_w / 2;
    int py      = by - panel_h - 6;

    if (px < 4) px = 4;
    if (px + panel_w > config.screen_w - 4) px = config.screen_w - panel_w - 4;
    if (py < 4) py = by + BTN_H + 6;

    DrawRectangle(px, py, panel_w, panel_h,
                  ColorFromNormalized({0.08f, 0.05f, 0.02f, 0.95f}));
    DrawRectangleLines(px, py, panel_w, panel_h, Color{140, 110, 70, 255});

    DrawText(tip.description, px + 8, py + 8,  12, Color{220, 200, 160, 255});
    DrawText(tip.flavor,      px + 8, py + 30, 11, Color{140, 110, 70,  255});
}

// ─── objectives ───────────────────────────────────────────────────────────────

void Renderer::draw_objectives(const GameState& state) const {
    const auto& objectives = state.map.get_objectives();
    if (objectives.empty()) return;

    auto is_met = [&](const Objective& obj) -> bool {
        if (obj.type == Objective::Type::KILL_UNIT) {
            for (auto& e : state.enemies) {
                if (!e.is_alive()) continue;
                if (e.get_type_id() == obj.target) return false;
            }
            return true;
        }
        if (obj.type == Objective::Type::HOLD_TILE) {
            for (auto& p : state.players) {
                if (!p.is_alive()) continue;
                if (state.map.is_objective(p.get_x_pos(), p.get_y_pos()))
                    return true;
            }
            return false;
        }
        return false;
    };

    int panel_x = 8;
    int panel_y = 8;
    int panel_w = 200;
    int panel_h = 16 + (int)objectives.size() * 20 + 8;

    DrawRectangle(panel_x, panel_y, panel_w, panel_h,
                  ColorFromNormalized({0.08f, 0.05f, 0.02f, 0.88f}));
    DrawRectangleLines(panel_x, panel_y, panel_w, panel_h,
                       Color{140, 110, 70, 255});

    DrawText("Objectives", panel_x + 8, panel_y + 6, 13,
             Color{200, 170, 100, 255});

    int line_y = panel_y + 26;
    for (auto& obj : objectives) {
        bool        met   = is_met(obj);
        Color       c     = met ? Color{80, 180, 80, 255} : Color{180, 180, 180, 255};
        const char* check = met ? "[x]" : "[ ]";
        DrawText(check,             panel_x + 8,  line_y, 12, c);
        DrawText(obj.label.c_str(), panel_x + 34, line_y, 12, c);
        line_y += 20;
    }
}

// ─── camera ───────────────────────────────────────────────────────────────────

void Renderer::update_camera(int player_x, int player_y, int map_w) {
    float target_x = player_x * config.tile_size + config.tile_size / 2.0f;
    float half_w   = config.screen_w / 2.0f;
    target_x = std::max(half_w, std::min(target_x, (float)map_w - half_w));

    camera.target = { target_x, config.grid_h / 2.0f };
    camera.offset = { config.screen_w / 2.0f, config.grid_h / 2.0f };
}

const Camera2D& Renderer::get_camera() const { return camera; }

// ─── draw frame ───────────────────────────────────────────────────────────────

void Renderer::draw_frame(const GameState& state) {
    BeginDrawing();

    if (state.show_title) {
        draw_title_screen();
        EndDrawing();
        return;
    }

    ClearBackground(DARKGRAY);

    BeginMode2D(camera);
    draw_map(state);
    draw_range_overlay(state);
    draw_target_highlights(state);
    draw_units(state);
    draw_floating_texts(state);
    EndMode2D();

    draw_attack_preview(state);
    draw_objectives(state);
    draw_hud(state);
    draw_game_over(state);

    EndDrawing();
}

// ─── draw map ─────────────────────────────────────────────────────────────────

void Renderer::draw_map(const GameState& state) {
    const auto& tiles     = state.map.get_tiles();
    int         tile_size = config.tile_size;

    for (int row = 0; row < (int)tiles.size(); row++) {
        for (int col = 0; col < (int)tiles[row].size(); col++) {
            int         x = col * tile_size;
            int         y = row * tile_size;
            const Tile& t = tiles[row][col];

            if (tileset.id != 0 && t.tile_id > 0) {
                draw_tile(t.tile_id, x, y);
            } else {
                switch (t.type) {
                    case TILE_WALL:
                        DrawRectangle(x, y, tile_size, tile_size, GRAY); break;
                    case TILE_BARREL:
                        DrawRectangle(x, y, tile_size, tile_size, BROWN); break;
                    case TILE_RAILING:
                        DrawRectangle(x, y, tile_size, tile_size, DARKBROWN); break;
                    case TILE_OBJECTIVE:
                        DrawRectangle(x, y, tile_size, tile_size, Fade(GOLD, 0.4f)); break;
                    default:
                        DrawRectangle(x, y, tile_size, tile_size, Color{50, 35, 20, 255}); break;
                }
            }
        }
    }
}

void Renderer::draw_tile(int tile_id, int x, int y) {
    if (tile_id <= 0 || tileset.id == 0) return;
    int idx   = tile_id - 1;
    int src_x = idx * 32;
    DrawTexturePro(tileset,
        { (float)src_x, 0, 32, 32 },
        { (float)x, (float)y, (float)config.tile_size, (float)config.tile_size },
        { 0, 0 }, 0.0f, WHITE);
}

// ─── overlays ─────────────────────────────────────────────────────────────────

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
            if (!has_los(active.get_x_pos(), active.get_y_pos(),
                         e.get_x_pos(), e.get_y_pos(), state.map)) continue;

            CoverResult cover = get_cover(active, state.enemies[i], state.map);
            Color highlight   = cover.flanked ? Fade(GOLD, 0.5f) : Fade(RED, 0.4f);
            DrawRectangle(e.get_x_pos() * tile_size, e.get_y_pos() * tile_size,
                          tile_size, tile_size, highlight);
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
            if (!has_los(active.get_x_pos(), active.get_y_pos(),
                         e.get_x_pos(), e.get_y_pos(), state.map)) continue;

            CoverResult cover = get_cover(active, state.enemies[i], state.map);
            Color highlight   = cover.flanked ? Fade(GOLD, 0.5f) : Fade(PURPLE, 0.4f);
            DrawRectangle(e.get_x_pos() * tile_size, e.get_y_pos() * tile_size,
                          tile_size, tile_size, highlight);
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

// ─── units ────────────────────────────────────────────────────────────────────

void Renderer::draw_units(const GameState& state) {
    int tile_size = config.tile_size;

    struct DrawCall { int row; bool is_player; int index; };
    std::vector<DrawCall> calls;

    for (int i = 0; i < (int)state.enemies.size(); i++) {
        if (!state.enemies[i].is_alive()) continue;
        if (!state.spotted[i]) continue;
        calls.push_back({state.enemies[i].get_y_pos(), false, i});
    }
    for (int i = 0; i < (int)state.players.size(); i++) {
        if (!state.players[i].is_alive()) continue;
        calls.push_back({state.players[i].get_y_pos(), true, i});
    }
    std::sort(calls.begin(), calls.end(), [](const DrawCall& a, const DrawCall& b) {
        return a.row < b.row;
    });

    for (auto& call : calls) {
        if (!call.is_player) {
            const enemy& e  = state.enemies[call.index];
            int          tx = e.get_x_pos() * tile_size;
            int          ty = e.get_y_pos() * tile_size;

            // sprite key comes directly from the enemy def
            std::string ekey = e.get_sprite_key();

            auto it = enemy_sprites.find(ekey);
            if (it != enemy_sprites.end() && it->second.id != 0) {
                int dx = tx - (SPR_W - tile_size) / 2;
                int dy = ty - (SPR_H - tile_size);
                DrawTexturePro(it->second,
                    { 0, 0, (float)it->second.width, (float)it->second.height },
                    { (float)dx, (float)dy, (float)SPR_W, (float)SPR_H },
                    { 0, 0 }, 0.0f, WHITE);
            } else {
                DrawRectangle(tx, ty, tile_size, tile_size, BLUE);
            }

            if (call.index == state.target_index && state.mode != ActionMode::HEAL)
                DrawRectangleLines(tx - 2, ty - 2, tile_size + 4, tile_size + 4, YELLOW);

            int pip_y = ty - (SPR_H - tile_size) - 8;
            for (int j = 0; j < e.get_max_hp(); j++) {
                Color pip = j < e.get_hp() ? GREEN : DARKGRAY;
                DrawRectangle(tx + 2 + j * 5, pip_y, 4, 4, pip);
            }

        } else {
            const unit& p    = state.players[call.index];
            int tx = p.get_x_pos() * tile_size;
            int ty = p.get_y_pos() * tile_size;
            bool is_selected = (call.index == state.selected_player);

            const std::string& pname = call.index < (int)state.player_names.size()
                                     ? state.player_names[call.index] : "";
            auto it = unit_sprites.find(pname);

            if (it != unit_sprites.end() && it->second.id != 0) {
                int   dx   = tx - (SPR_W - tile_size) / 2;
                int   dy   = ty - (SPR_H - tile_size);
                Color tint = is_selected ? WHITE : Color{200, 200, 200, 255};
                DrawTexturePro(it->second,
                    { 0, 0, (float)it->second.width, (float)it->second.height },
                    { (float)dx, (float)dy, (float)SPR_W, (float)SPR_H },
                    { 0, 0 }, 0.0f, tint);
            } else {
                DrawRectangle(tx, ty, tile_size, tile_size,
                              is_selected ? RED : Color{150, 30, 30, 255});
            }

            if (is_selected)
                DrawRectangleLines(tx - 1, ty - 1, tile_size + 2, tile_size + 2, WHITE);

            if (state.mode == ActionMode::HEAL && call.index == state.target_index)
                DrawRectangleLines(tx - 2, ty - 2, tile_size + 4, tile_size + 4, GREEN);

            int pip_y = ty - (SPR_H - tile_size) - 8;
            for (int j = 0; j < p.get_max_hp(); j++) {
                Color pip = j < p.get_hp() ? GREEN : DARKGRAY;
                DrawRectangle(tx + 2 + j * 5, pip_y, 4, 4, pip);
            }
        }
    }
}

// ─── floating texts ───────────────────────────────────────────────────────────

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

// ─── attack preview ───────────────────────────────────────────────────────────

void Renderer::draw_attack_preview(const GameState& state) {
    if (!state.preview.active || state.preview.target == nullptr) return;

    const AttackResult& result = state.preview.result;
    const unit*         target = state.preview.target;

    Vector2 world_pos  = {
        (float)(target->get_x_pos() * config.tile_size + config.tile_size / 2),
        (float)(target->get_y_pos() * config.tile_size)
    };
    Vector2 screen_pos = GetWorldToScreen2D(world_pos, camera);

    int panel_w = 220;
    int panel_h = 130;
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
    DrawText(TextFormat("%d%% to hit", result.hit_chance), px + 8, py + 6,  16, hit_color);
    DrawText(TextFormat("%d%% crit",   result.crit_chance), px + 8, py + 26, 13, ORANGE);

    int       line_y = py + 46;
    const int LINE_H = 14;

    DrawText(TextFormat("aim      %+d", result.aim_component),     px + 8, line_y, 12, LIGHTGRAY);
    line_y += LINE_H;
    if (result.flank_bonus > 0) {
        DrawText(TextFormat("flank    %+d", result.flank_bonus),   px + 8, line_y, 12, GREEN);
        line_y += LINE_H;
    }
    if (result.range_penalty > 0) {
        DrawText(TextFormat("range    -%d", result.range_penalty), px + 8, line_y, 12, RED);
        line_y += LINE_H;
    }
    DrawText(TextFormat("defense  -%d", result.defense_penalty),   px + 8, line_y, 12, LIGHTGRAY);
    line_y += LINE_H;
    if (result.cover_penalty > 0) {
        DrawText(TextFormat("cover    -%d", result.cover_penalty), px + 8, line_y, 12, SKYBLUE);
        line_y += LINE_H;
    }
    if (result.is_flanking)
        DrawText("FLANKED", px + 8, line_y, 12, GREEN);
}

// ─── HUD ──────────────────────────────────────────────────────────────────────

void Renderer::draw_hud(const GameState& state) {
    if (state.players.empty()) return;
    const unit& active = state.players[state.selected_player];
    int         bar_y  = config.screen_h - BAR_HEIGHT;

    if (hud_texture.id != 0) {
        DrawTexturePro(hud_texture,
            { 0, 0, (float)hud_texture.width, (float)hud_texture.height },
            { 0, (float)bar_y, (float)config.screen_w, (float)BAR_HEIGHT },
            { 0, 0 }, 0.0f, WHITE);
    } else {
        DrawRectangle(0, bar_y, config.screen_w, BAR_HEIGHT,
                      ColorFromNormalized({0.13f, 0.13f, 0.13f, 1.0f}));
    }
    DrawLine(0, bar_y, config.screen_w, bar_y, Color{80, 60, 40, 255});

    draw_portrait(state, bar_y);

    DrawLine(PORTRAIT_X + BAR_HEIGHT + 10, bar_y + 8,
             PORTRAIT_X + BAR_HEIGHT + 10, bar_y + BAR_HEIGHT - 8,
             Color{120, 90, 50, 255});

    int stat_x   = PORTRAIT_X + BAR_HEIGHT + 35;
    int bar_w    = 130;
    int bar_h    = 14;
    int hp_bar_y = bar_y + 40;
    int ac_bar_y = bar_y + 74;

    float hp_frac = (float)active.get_hp() / (float)active.get_max_hp();
    Color hp_fill = hp_frac > 0.5f ? Color{45, 120, 45, 255}
                  : hp_frac > 0.25f ? Color{180, 120, 30, 255}
                  : Color{160, 40, 40, 255};

    DrawText("HP", stat_x - 24, hp_bar_y + 1, 12, Color{80, 55, 30, 255});
    DrawRectangle(stat_x, hp_bar_y, bar_w, bar_h, Color{40, 25, 10, 255});
    int hp_fill_w = (int)(bar_w * hp_frac);
    if (hp_fill_w > 0) {
        DrawRectangle(stat_x, hp_bar_y, hp_fill_w, bar_h, hp_fill);
        DrawRectangle(stat_x, hp_bar_y, hp_fill_w, 2, Color{80, 200, 80, 120});
    }
    for (int j = 1; j < active.get_max_hp(); j++) {
        int seg_x = stat_x + (int)(bar_w * j / (float)active.get_max_hp());
        DrawLine(seg_x, hp_bar_y, seg_x, hp_bar_y + bar_h, Color{20, 12, 5, 160});
    }
    DrawRectangleLines(stat_x, hp_bar_y, bar_w, bar_h, Color{140, 110, 50, 255});
    DrawText(TextFormat("%d/%d", active.get_hp(), active.get_max_hp()),
             stat_x + bar_w + 6, hp_bar_y + 1, 12, Color{80, 55, 30, 255});

    DrawText("AP", stat_x - 24, ac_bar_y + 1, 12, Color{80, 55, 30, 255});
    DrawRectangle(stat_x, ac_bar_y, bar_w, bar_h, Color{20, 20, 40, 255});
    int ac_fill_w = (int)(bar_w * active.get_actions() / 2.0f);
    if (ac_fill_w > 0) {
        DrawRectangle(stat_x, ac_bar_y, ac_fill_w, bar_h, Color{40, 80, 180, 255});
        DrawRectangle(stat_x, ac_bar_y, ac_fill_w, 2, Color{80, 140, 220, 120});
    }
    DrawLine(stat_x + bar_w / 2, ac_bar_y,
             stat_x + bar_w / 2, ac_bar_y + bar_h,
             Color{20, 12, 5, 160});
    DrawRectangleLines(stat_x, ac_bar_y, bar_w, bar_h, Color{140, 110, 50, 255});
    DrawText(TextFormat("%d/2", active.get_actions()),
             stat_x + bar_w + 6, ac_bar_y + 1, 12, Color{80, 55, 30, 255});

    DrawLine(stat_x + bar_w + 40, bar_y + 8,
             stat_x + bar_w + 40, bar_y + BAR_HEIGHT - 8,
             Color{120, 90, 50, 255});

    const auto& abilities  = active.get_abilities();
    bool        no_actions = (active.get_actions() <= 0);

    int     hovered_btn = -1;
    Vector2 mouse       = GetMousePosition();

    for (int i = 0; i < (int)abilities.size(); i++) {
        const Ability& ab = abilities[i];
        int bx = BTN_START_X + i * (BTN_W + BTN_GAP);
        int by = bar_y + BTN_Y_OFFSET;

        bool is_active   = (state.mode == ab.get_mode());
        bool on_cooldown = !ab.is_ready();
        bool cant_afford = (active.get_actions() < ab.get_cost() && ab.get_cost() > 0);
        bool unavailable = no_actions || on_cooldown || cant_afford;

        if (state.phase == GamePhase::PLAYER_TURN &&
            mouse.x >= bx && mouse.x <= bx + BTN_W &&
            mouse.y >= by && mouse.y <= by + BTN_H)
            hovered_btn = i;

        Color bg = is_active
                 ? ColorFromNormalized({0.1f, 0.25f, 0.45f, 0.85f})
                 : ColorFromNormalized({0.15f, 0.10f, 0.05f, 0.55f});
        DrawRectangle(bx, by, BTN_W, BTN_H, bg);

        Color border = is_active   ? Color{80, 140, 220, 255}
                     : unavailable ? Color{100, 80, 55, 255}
                                   : Color{140, 110, 70, 255};
        DrawRectangleLines(bx, by, BTN_W, BTN_H, border);

        if (icons.has(ab.get_id())) {
            Texture2D tex       = icons.get(ab.get_id());
            int       icon_size = 56;
            int       icon_x    = bx + BTN_W / 2 - icon_size / 2;
            int       icon_y    = by + 2;
            Color     tint      = unavailable ? Color{120, 120, 120, 180} : WHITE;
            DrawTexturePro(tex,
                { 0, 0, (float)tex.width, (float)tex.height },
                { (float)icon_x, (float)icon_y, (float)icon_size, (float)icon_size },
                { 0, 0 }, 0.0f, tint);

            Color label = is_active   ? Color{80, 160, 220, 255}
                        : unavailable ? Color{100, 80, 55, 255}
                                      : Color{60, 35, 15, 255};
            const char* lbl = ab.get_label().c_str();
            DrawText(lbl, bx + BTN_W/2 - MeasureText(lbl, 13)/2, by + 60, 13, label);
        } else {
            Color label = is_active   ? Color{80, 160, 220, 255}
                        : unavailable ? Color{100, 80, 55, 255}
                                      : Color{60, 35, 15, 255};
            const char* lbl = ab.get_label().c_str();
            DrawText(lbl, bx + BTN_W/2 - MeasureText(lbl, 16)/2, by + 10, 16, label);
        }

        if (on_cooldown) {
            const char* cd = TextFormat("CD: %d", ab.get_cooldown());
            DrawText(cd, bx + BTN_W/2 - MeasureText(cd, 13)/2, by + 74, 13,
                     Color{200, 60, 60, 255});
        } else if (ab.get_cost() == 0) {
            DrawText("free", bx + BTN_W/2 - MeasureText("free", 13)/2,
                     by + 74, 13, Color{60, 180, 60, 255});
        } else {
            const char* cost = TextFormat("%d action%s", ab.get_cost(),
                                           ab.get_cost() > 1 ? "s" : "");
            DrawText(cost, bx + BTN_W/2 - MeasureText(cost, 13)/2,
                     by + 74, 13, Color{40, 20, 5, 255});
        }
    }

    if (hovered_btn >= 0 && hovered_btn < (int)abilities.size()) {
        int bx = BTN_START_X + hovered_btn * (BTN_W + BTN_GAP);
        int by = bar_y + BTN_Y_OFFSET;
        draw_tooltip(abilities[hovered_btn].get_id(), bx, by);
    }

    DrawLine(config.screen_w - 150, bar_y + 8,
             config.screen_w - 150, bar_y + BAR_HEIGHT - 8,
             Color{120, 90, 50, 255});

    const char* turn_num = TextFormat("Turn %d", state.turn_count + 1);
    DrawText(turn_num, config.screen_w - 142, bar_y + 8, 16,
             Color{100, 75, 45, 255});

    const char* turn_label = state.phase == GamePhase::PLAYER_TURN
                           ? "Player turn" : "Enemy turn";
    DrawText(turn_label, config.screen_w - 142, bar_y + 28, 16,
             Color{80, 55, 30, 255});

    Color end_border = (state.phase == GamePhase::PLAYER_TURN)
                     ? Color{140, 110, 70, 255}
                     : Color{100, 80, 55, 255};
    Color end_text   = (state.phase == GamePhase::PLAYER_TURN)
                     ? Color{60, 35, 15, 255}
                     : Color{100, 80, 55, 255};
    DrawRectangle(config.screen_w - 142, bar_y + 52, 130, 36,
                  ColorFromNormalized({0.15f, 0.10f, 0.05f, 0.45f}));
    DrawRectangleLines(config.screen_w - 142, bar_y + 52, 130, 36, end_border);
    DrawText("End turn",
             config.screen_w - 142 + 16, bar_y + 62, 16, end_text);
}

// ─── game over ────────────────────────────────────────────────────────────────

void Renderer::draw_game_over(const GameState& state) {
    if (state.win_state == WinState::ONGOING) return;

    DrawRectangle(0, 0, config.screen_w, config.screen_h,
                  ColorFromNormalized({0.0f, 0.0f, 0.0f, 0.75f}));

    int cx = config.screen_w / 2;
    int cy = config.screen_h / 2;

    if (state.win_state == WinState::VICTORY) {
        const char* title = "MUTINY SUCCESSFUL";
        DrawText(title, cx - MeasureText(title, 48) / 2, cy - 130, 48, GOLD);
        const char* sub = "The gold and ship are yours.";
        DrawText(sub, cx - MeasureText(sub, 20) / 2, cy - 72, 20, LIGHTGRAY);
        const char* demo = "Thanks for playing the demo.";
        DrawText(demo, cx - MeasureText(demo, 16) / 2, cy - 44, 16,
                Color{160, 130, 60, 255});
    } else {
        const char* title = "MUTINY FAILED";
        DrawText(title, cx - MeasureText(title, 48) / 2, cy - 130, 48, RED);
        const char* sub = "It was worth a shot.";
        DrawText(sub, cx - MeasureText(sub, 20) / 2, cy - 72, 20, LIGHTGRAY);
    }

    // survived text — push down so it doesn't overlap
    const char* turns = TextFormat("Survived %d turn%s",
                                    state.turn_count,
                                    state.turn_count == 1 ? "" : "s");
    DrawText(turns, cx - MeasureText(turns, 18) / 2, cy - 16, 18, GRAY);

    int  y_offset = cy + 10;
    bool any_dead = false;
    for (int i = 0; i < (int)state.players.size(); i++) {
        if (state.players[i].is_alive()) continue;
        if (!any_dead) {
            DrawText("Killed in action:", cx - 100, y_offset, 16, GRAY);
            y_offset += 24;
            any_dead = true;
        }
        const char* pname = i < (int)state.player_names.size()
                          ? state.player_names[i].c_str() : "Unknown";
        DrawText(TextFormat("  %s", pname), cx - 100, y_offset, 16, RED);
        y_offset += 22;
    }

    if (!any_dead && state.win_state == WinState::VICTORY) {
        DrawText("Full crew survived.",
                 cx - MeasureText("Full crew survived.", 16) / 2,
                 y_offset, 16, GREEN);
        y_offset += 24;
    }

    if (state.win_state == WinState::DEFEAT) {
        const char* restart = "Press R to try again";
        DrawText(restart, cx - MeasureText(restart, 20) / 2,
                 cy + 90, 20, WHITE);
    }
}


//Title screen
void Renderer::draw_title_screen() const {
    int cx = config.screen_w / 2;
    int cy = config.screen_h / 2;

    ClearBackground(Color{10, 8, 5, 255});

    const char* title = "MUTINY";
    DrawText(title, cx - MeasureText(title, 72) / 2, cy - 180, 72,
             Color{200, 160, 60, 255});

    DrawLine(cx - 200, cy - 90, cx + 200, cy - 90, Color{100, 80, 40, 255});

    int line_y = cy - 70;
    const int LINE_H = 32;
    Color     text_c = Color{180, 160, 120, 255};
    int       size   = 20;

    auto draw_centered = [&](const char* text, int y, int font_size, Color c) {
        DrawText(text, cx - MeasureText(text, font_size) / 2, y, font_size, c);
    };

    draw_centered("Captain Vane has become a liability.", line_y, size, text_c);
    line_y += LINE_H;
    draw_centered("The crew is starving, his promises are worthless,", line_y, size, text_c);
    line_y += LINE_H;
    draw_centered("and every voyage costs more lives than gold.", line_y, size, text_c);
    line_y += LINE_H * 2;
    draw_centered("Tonight, a few desperate pirates have decided", line_y, size, text_c);
    line_y += LINE_H;
    draw_centered("to settle the matter the old fashioned way.", line_y, size, text_c);
    line_y += LINE_H * 2;

    DrawLine(cx - 200, cy + 120, cx + 200, cy + 120, Color{100, 80, 40, 255});

    draw_centered("-- Press Enter --", cy + 140, 14, Color{140, 110, 60, 255});
}