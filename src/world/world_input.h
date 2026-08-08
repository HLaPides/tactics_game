#pragma once
#include "world_state.h"
#include "../core/types.h"
#include "raylib.h"
#include <optional>

enum class WorldIntent {
    NONE,
    MOVE_SHIP,
    ENTER_PORT,
    OPEN_TAVERN,
    OPEN_SHOP,
    OPEN_SHIPYARD,
    OPEN_TOWN_HALL,
    LEAVE_PORT,
    ACCEPT_CONTRACT,
    CONFIRM_ENGAGE,
    CANCEL_ENGAGE,
    END_TURN
};

struct WorldAction {
    WorldIntent intent = WorldIntent::NONE;
    int         col    = 0;
    int         row    = 0;
    int         index  = 0;
};

class WorldInput {
public:
    WorldInput(const AppConfig& config);
    std::optional<WorldAction> poll(const WorldState& state, const Camera2D& cam);

private:
    const AppConfig& config;
    int mouse_world_col(Vector2 mouse, const Camera2D& cam) const;
    int mouse_world_row(Vector2 mouse, const Camera2D& cam) const;
};