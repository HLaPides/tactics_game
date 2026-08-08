#pragma once
#include "world_state.h"
#include "../core/types.h"
#include "raylib.h"
#include <optional>

enum class PortIntent {
    NONE,
    SWITCH_TAB,
    ACCEPT_TAVERN_CONTRACT,
    ACCEPT_TOWN_HALL_CONTRACT,
    BUY_SUPPLIES,
    REPAIR_SHIP,
    LEAVE_PORT
};

struct PortAction {
    PortIntent intent = PortIntent::NONE;
    PortTab    tab     = PortTab::SHOP;
    int        index   = 0;
};

class PortInput {
public:
    PortInput(const AppConfig& config);
    std::optional<PortAction> poll(const WorldState& state);

private:
    const AppConfig& config;

    static const int TAB_W = 140;
    static const int TAB_H = 40;
    static const int TAB_Y = 20;
    static const int LIST_X = 60;
    static const int LIST_Y = 100;
    static const int ROW_H  = 70;
};