#pragma once
#include "raylib.h"
#include "units/unit.h"

enum GameState  { PLAYER_TURN, ENEMY_TURN };
enum ActionMode { MODE_NONE, MODE_SHOOT, MODE_MELEE };

class hud {
public:
    hud(int screen_w, int screen_h);
    void draw(unit& selected_unit, GameState state, ActionMode mode);
    bool clicked_shoot(Vector2 mouse);
    bool clicked_melee(Vector2 mouse);
    bool clicked_end_turn(Vector2 mouse);
private:
    int screen_w;
    int screen_h;
    int bar_y;

    static const int BAR_HEIGHT   = 80;
    static const int BTN_W        = 80;
    static const int BTN_H        = 60;
    static const int BTN_Y_OFFSET = 10;

    void draw_unit_info(unit& u);
    void draw_buttons(int player_actions, ActionMode mode);
    void draw_turn_info(GameState state);
};