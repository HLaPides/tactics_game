#include "hud.h"

hud::hud(int screen_w, int screen_h) {
    this->screen_w = screen_w;
    this->screen_h = screen_h;
    bar_y = screen_h - BAR_HEIGHT;
}

void hud::draw(unit& selected_unit, GameState state, ActionMode mode) {
    DrawRectangle(0, bar_y, screen_w, BAR_HEIGHT, ColorFromNormalized({0.13f, 0.13f, 0.13f, 1.0f}));
    DrawLine(0, bar_y, screen_w, bar_y, GRAY);
    draw_unit_info(selected_unit);
    draw_buttons(selected_unit.get_actions(), mode);
    draw_turn_info(state);
}

void hud::draw_unit_info(unit& u) {
    DrawText("Bosun", 12, bar_y + 10, 16, WHITE);

    for (int i = 0; i < 2; i++) {
        Color pip = i < u.get_actions() ? SKYBLUE : DARKGRAY;
        DrawCircle(12 + i * 16, bar_y + 36, 5, pip);
    }

    DrawText("HP", 12, bar_y + 50, 12, GRAY);
    DrawRectangle(30, bar_y + 52, 60, 8, DARKGRAY);
    float hp_frac = (float)u.get_hp() / u.get_max_hp();
    Color hp_color = hp_frac > 0.5f ? GREEN : (hp_frac > 0.25f ? ORANGE : RED);
    DrawRectangle(30, bar_y + 52, (int)(60 * hp_frac), 8, hp_color);
    DrawText(TextFormat("%d/%d", u.get_hp(), u.get_max_hp()), 96, bar_y + 50, 12, GRAY);

    DrawLine(130, bar_y + 8, 130, bar_y + BAR_HEIGHT - 8, GRAY);
}

void hud::draw_buttons(int player_actions, ActionMode mode) {
    const char* btn_labels[] = { "Shoot", "Melee" };
    ActionMode  btn_modes[]  = { MODE_SHOOT, MODE_MELEE };
    int btn_start_x = 148;

    for (int i = 0; i < 2; i++) {
        int bx = btn_start_x + i * (BTN_W + 8);
        int by = bar_y + BTN_Y_OFFSET;

        bool active     = (mode == btn_modes[i]);
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

    DrawLine(screen_w - 120, bar_y + 8, screen_w - 120, bar_y + BAR_HEIGHT - 8, GRAY);
}

void hud::draw_turn_info(GameState state) {
    const char* turn_label = state == PLAYER_TURN ? "Player turn" : "Enemy turn";
    DrawText(turn_label, screen_w - 112, bar_y + 10, 13, GRAY);

    Color end_col = (state == PLAYER_TURN) ? WHITE : DARKGRAY;
    DrawRectangleLines(screen_w - 112, bar_y + 30, 100, 28, end_col);
    DrawText("End turn", screen_w - 112 + 10, bar_y + 37, 13, end_col);
}

bool hud::clicked_shoot(Vector2 mouse) {
    int bx = 148;
    int by = bar_y + BTN_Y_OFFSET;
    return mouse.x >= bx && mouse.x < bx + BTN_W &&
           mouse.y >= by && mouse.y < by + BTN_H;
}

bool hud::clicked_melee(Vector2 mouse) {
    int bx = 148 + BTN_W + 8;
    int by = bar_y + BTN_Y_OFFSET;
    return mouse.x >= bx && mouse.x < bx + BTN_W &&
           mouse.y >= by && mouse.y < by + BTN_H;
}

bool hud::clicked_end_turn(Vector2 mouse) {
    return mouse.x >= screen_w - 112 && mouse.x < screen_w - 12 &&
           mouse.y >= bar_y + 30 && mouse.y < bar_y + 58;
}