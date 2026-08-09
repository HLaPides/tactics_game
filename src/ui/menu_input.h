#pragma once
#include "../world/world_state.h"
#include "../core/campaign_state.h"
#include "../core/types.h"
#include "raylib.h"
#include <optional>

enum class MenuIntent {
    NONE,
    SWITCH_TAB,
    TOGGLE_CREW_EXPAND,
    CLOSE_MENU
};

struct MenuAction {
    MenuIntent intent = MenuIntent::NONE;
    MenuTab    tab     = MenuTab::OVERVIEW;
    int        index   = 0;
};

class MenuInput {
public:
    MenuInput(const AppConfig& config);
    std::optional<MenuAction> poll(const CampaignState& campaign, const WorldState& world);

private:
    const AppConfig& config;

    static const int TAB_Y              = 20;
    static const int TAB_H              = 40;
    static const int TAB_W              = 140;
    static const int TAB_GAP            = 8;
    static const int LIST_X             = 60;
    static const int LIST_Y             = 90;
    static const int LIST_W             = 600;
    static const int ROW_H_COLLAPSED    = 64;
    static const int ROW_H_EXPANDED     = 200;
    static const int ROW_GAP            = 8;
};