#pragma once
#include "../core/game_state.h"
#include "../combat/combat.h"
#include "../core/types.h"
#include "icon_registry.h"
#include "raylib.h"
#include <unordered_map>
#include <numeric>
#include <string>

class Renderer {
public:
    Renderer(const AppConfig& config);
    ~Renderer();
    void update_camera(int player_x, int player_y, int map_w);
    void draw_frame(const GameState& state);
    const Camera2D& get_camera() const;
private:
    const AppConfig& config;
    Camera2D         camera = { {0,0}, {0,0}, 0.0f, 1.0f };
    IconRegistry     icons;
    Texture2D        hud_texture = {};
    Texture2D        tileset     = {};

    std::unordered_map<std::string, Texture2D> portraits;
    std::unordered_map<std::string, Texture2D> unit_sprites;
    std::unordered_map<std::string, Texture2D> enemy_sprites;

    void load_portraits();
    void load_sprites();
    void draw_portrait(const GameState& state, int bar_y);
    void draw_tile(int tile_id, int x, int y);

    void draw_map(const GameState& state);
    void draw_range_overlay(const GameState& state);
    void draw_target_highlights(const GameState& state);
    void draw_units(const GameState& state);
    void draw_floating_texts(const GameState& state);
    void draw_attack_preview(const GameState& state);
    void draw_hud(const GameState& state);
    void draw_game_over(const GameState& state);

    static const int BAR_HEIGHT    = 100;
    static const int BTN_W         = 90;
    static const int BTN_H         = 90;
    static const int BTN_Y_OFFSET  = 5;
    static const int BTN_START_X   = 350;
    static const int BTN_GAP       = 10;
    static const int PORTRAIT_SIZE = 84;
    static const int PORTRAIT_X    = 8;
    static const int SPR_W         = 48;
    static const int SPR_H         = 96;
};