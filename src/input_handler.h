#pragma once
#include "types.h"
#include "ability.h"
#include "game_state.h"
#include "raylib.h"
#include <optional>
#include <vector>

class InputHandler {
public:
    InputHandler(const AppConfig& config);
    std::optional<Intent> poll(GameState& state);
    void update_preview(GameState& state);
private:
    const AppConfig& config;

    int  mouse_tile_x(Vector2 mouse, const Camera2D& cam) const;
    int  mouse_tile_y(Vector2 mouse, const Camera2D& cam) const;
    bool mouse_on_grid(Vector2 mouse) const;
    bool clicked_end_turn(Vector2 mouse) const;
    int  clicked_ability(Vector2 mouse, const unit& active) const;
    std::optional<IntentType> mode_to_intent(ActionMode mode) const;
    std::vector<int> get_valid_targets(const GameState& state) const;

    static const int BTN_W             = 80;
    static const int BTN_H             = 60;
    static const int BTN_Y_OFFSET      = 10;
    static const int BTN_START_X       = 148;
    static const int BTN_GAP           = 8;
    static const int END_TURN_W        = 100;
    static const int END_TURN_H        = 28;
    static const int END_TURN_X_OFFSET = 112;
    static const int END_TURN_Y_OFFSET = 30;
};