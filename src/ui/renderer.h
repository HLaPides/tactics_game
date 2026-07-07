#pragma once
#include "../core/game_state.h"
#include "../combat/combat.h"
#include "../core/types.h"
#include "icon_registry.h"
#include "raylib.h"

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

    void draw_map(const GameState& state);
    void draw_range_overlay(const GameState& state);
    void draw_target_highlights(const GameState& state);
    void draw_units(const GameState& state);
    void draw_floating_texts(const GameState& state);
    void draw_attack_preview(const GameState& state);
    void draw_hud(const GameState& state);
    void draw_game_over(const GameState& state);

    static const int BAR_HEIGHT     = 100;
    static const int BTN_W          = 90;
    static const int BTN_H          = 90;
    static const int BTN_Y_OFFSET   = 5;
    static const int BTN_START_X    = 160;
    static const int BTN_GAP        = 10;
    static const int END_TURN_W     = 110;
    static const int END_TURN_H     = 32;
    static const int END_TURN_X_OFF = 124;
    static const int END_TURN_Y_OFF = 34;
};