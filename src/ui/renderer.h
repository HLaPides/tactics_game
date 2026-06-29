#pragma once
#include "../core/game_state.h"
#include "../combat/combat.h"
#include "../core/types.h"
#include "raylib.h"

class Renderer {
public:
    Renderer(const AppConfig& config);
    void update_camera(int player_x, int player_y, int map_w);
    void draw_frame(const GameState& state);
    const Camera2D& get_camera() const;
private:
    const AppConfig& config;
    Camera2D         camera = { {0,0}, {0,0}, 0.0f, 1.0f };

    void draw_map(const GameState& state);
    void draw_range_overlay(const GameState& state);
    void draw_target_highlights(const GameState& state);
    void draw_units(const GameState& state);
    void draw_floating_texts(const GameState& state);
    void draw_attack_preview(const GameState& state);
    void draw_hud(const GameState& state);
    void draw_game_over(const GameState& state);

    static const int BAR_HEIGHT     = 80;
    static const int BTN_W          = 80;
    static const int BTN_H          = 60;
    static const int BTN_Y_OFFSET   = 10;
    static const int BTN_START_X    = 148;
    static const int BTN_GAP        = 8;
    static const int END_TURN_W     = 100;
    static const int END_TURN_H     = 28;
    static const int END_TURN_X_OFF = 112;
    static const int END_TURN_Y_OFF = 30;
};