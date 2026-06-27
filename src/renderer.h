#pragma once
#include "game_state.h"
#include "combat.h"
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

    static const int BAR_HEIGHT      = 80;
    static const int BTN_W           = 80;
    static const int BTN_H           = 60;
    static const int BTN_Y_OFFSET    = 10;
    static const int BTN_START_X     = 148;
    static const int BTN_GAP         = 8;
    static const int END_TURN_W      = 100;
    static const int END_TURN_H      = 28;
    static const int END_TURN_X_OFF  = 112;
    static const int END_TURN_Y_OFF  = 30;
};