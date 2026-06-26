#pragma once
#include "game_state.h"
#include "types.h"

class Renderer {
public:
    Renderer(const AppConfig& config);
    void draw_frame(const GameState& state);
private:
    const AppConfig& config;

    void draw_map(const GameState& state);
    void draw_range_overlay(const GameState& state);
    void draw_target_highlights(const GameState& state);
    void draw_units(const GameState& state);
    void draw_floating_texts(const GameState& state);
    void draw_attack_preview(const GameState& state);
    void draw_hud(const GameState& state);
    void draw_game_over(const GameState& state);

    // HUD layout constants
    static const int BAR_HEIGHT   = 80;
    static const int BTN_W        = 80;
    static const int BTN_H        = 60;
    static const int BTN_Y_OFFSET = 10;
    static const int BTN_START_X  = 148;
    static const int BTN_GAP      = 8;
};