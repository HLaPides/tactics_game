#include "input_handler.h"
#include "combat.h"
#include "raylib.h"
#include <algorithm>

InputHandler::InputHandler(const AppConfig& cfg) : config(cfg) {}

int  InputHandler::mouse_tile_x(Vector2 mouse) const { return (int)(mouse.x / config.tile_size); }
int  InputHandler::mouse_tile_y(Vector2 mouse) const { return (int)(mouse.y / config.tile_size); }
bool InputHandler::mouse_on_grid(Vector2 mouse) const { return mouse.y < config.grid_h; }

bool InputHandler::clicked_shoot(Vector2 mouse) const {
    int bar_y = config.screen_h - 80;
    int bx    = BTN_START_X;
    int by    = bar_y + BTN_Y_OFFSET;
    return mouse.x >= bx && mouse.x < bx + BTN_W &&
           mouse.y >= by && mouse.y < by + BTN_H;
}

bool InputHandler::clicked_melee(Vector2 mouse) const {
    int bar_y = config.screen_h - 80;
    int bx    = BTN_START_X + BTN_W + BTN_GAP;
    int by    = bar_y + BTN_Y_OFFSET;
    return mouse.x >= bx && mouse.x < bx + BTN_W &&
           mouse.y >= by && mouse.y < by + BTN_H;
}

bool InputHandler::clicked_end_turn(Vector2 mouse) const {
    int bar_y = config.screen_h - 80;
    int ex    = config.screen_w - END_TURN_X_OFFSET;
    int ey    = bar_y + END_TURN_Y_OFFSET;
    return mouse.x >= ex && mouse.x < ex + END_TURN_W &&
           mouse.y >= ey && mouse.y < ey + END_TURN_H;
}

std::optional<Intent> InputHandler::poll(const GameState& state) {
    Vector2 mouse = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_ESCAPE)) {
        if (state.mode != ActionMode::NONE)
            return Intent{ IntentType::Cancel };
    }

    if (IsKeyPressed(KEY_SPACE) && state.phase == GamePhase::PLAYER_TURN)
        return Intent{ IntentType::EndTurn };

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return std::nullopt;

    if (!mouse_on_grid(mouse)) {
        if (state.phase != GamePhase::PLAYER_TURN) return std::nullopt;
        if (clicked_end_turn(mouse))
            return Intent{ IntentType::EndTurn };
        return std::nullopt;
    }

    int mx = mouse_tile_x(mouse);
    int my = mouse_tile_y(mouse);

    if (state.phase != GamePhase::PLAYER_TURN) return std::nullopt;

    if (state.mode == ActionMode::SHOOT) return Intent{ IntentType::Shoot, mx, my };
    if (state.mode == ActionMode::MELEE) return Intent{ IntentType::Melee, mx, my };
    return Intent{ IntentType::Move, mx, my };
}

void InputHandler::update_preview(GameState& state) {
    Vector2 mouse = GetMousePosition();
    state.preview = {};

    // handle mode toggle on HUD button click
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !mouse_on_grid(mouse)
        && state.phase == GamePhase::PLAYER_TURN && state.player.get_actions() > 0) {
        if (clicked_shoot(mouse))
            state.mode = (state.mode == ActionMode::SHOOT) ? ActionMode::NONE : ActionMode::SHOOT;
        else if (clicked_melee(mouse))
            state.mode = (state.mode == ActionMode::MELEE) ? ActionMode::NONE : ActionMode::MELEE;
        return;
    }

    if (!mouse_on_grid(mouse) || state.mode == ActionMode::NONE) return;

    int mx = mouse_tile_x(mouse);
    int my = mouse_tile_y(mouse);

    for (auto& e : state.enemies) {
        if (!e.is_alive()) continue;
        if (e.get_x_pos() != mx || e.get_y_pos() != my) continue;

        int dist  = std::max(abs(mx - state.player.get_x_pos()),
                             abs(my - state.player.get_y_pos()));
        int range = (state.mode == ActionMode::SHOOT) ? state.player.get_shoot_range() : 1;

        if (dist <= range) {
            int base_dmg = (state.mode == ActionMode::SHOOT)
                         ? state.player.get_shoot_damage()
                         : state.player.get_melee_damage();
            state.preview.active = true;
            state.preview.target = &e;
            // calculate_odds — no random rolls wasted on preview
            state.preview.result = calculate_odds(state.player, e, state.map, base_dmg);
        }
        break;
    }
}