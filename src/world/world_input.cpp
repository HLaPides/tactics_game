#include "world_input.h"

WorldInput::WorldInput(const AppConfig& cfg) : config(cfg) {}

int WorldInput::mouse_world_col(Vector2 mouse, const Camera2D& cam) const {
    return (int)(GetScreenToWorld2D(mouse, cam).x / config.world_tile_size);
}

int WorldInput::mouse_world_row(Vector2 mouse, const Camera2D& cam) const {
    return (int)(GetScreenToWorld2D(mouse, cam).y / config.world_tile_size);
}

std::optional<WorldAction> WorldInput::poll(const WorldState& state,
                                             const Camera2D& cam) {
    if (IsKeyPressed(KEY_SPACE)) return WorldAction{ WorldIntent::END_TURN };

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return std::nullopt;

    Vector2 mouse = GetMousePosition();
    int col = mouse_world_col(mouse, cam);
    int row = mouse_world_row(mouse, cam);

    for (int i = 0; i < (int)state.ports.size(); i++) {
        if (state.ports[i].col == col && state.ports[i].row == row) {
            if (state.ship_col == col && state.ship_row == row)
                return WorldAction{ WorldIntent::ENTER_PORT, col, row, i };
        }
    }

    return WorldAction{ WorldIntent::MOVE_SHIP, col, row };
}