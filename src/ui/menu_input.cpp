#include "menu_input.h"

MenuInput::MenuInput(const AppConfig& cfg) : config(cfg) {}

std::optional<MenuAction> MenuInput::poll(const CampaignState& campaign, const WorldState& world) {
    if (IsKeyPressed(KEY_ESCAPE)) return MenuAction{ MenuIntent::CLOSE_MENU };
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return std::nullopt;

    Vector2 mouse = GetMousePosition();

    const MenuTab tabs[5] = {
        MenuTab::OVERVIEW, MenuTab::INVENTORY, MenuTab::CREW,
        MenuTab::CONTRACTS, MenuTab::QUESTS
    };
    for (int i = 0; i < 5; i++) {
        int tx = 20 + i * (TAB_W + TAB_GAP);
        if (mouse.x >= tx && mouse.x <= tx + TAB_W &&
            mouse.y >= TAB_Y && mouse.y <= TAB_Y + TAB_H) {
            return MenuAction{ MenuIntent::SWITCH_TAB, tabs[i] };
        }
    }

    if (world.menu_tab == MenuTab::CREW) {
        int y = LIST_Y;
        for (int i = 0; i < (int)campaign.roster.size(); i++) {
            bool expanded = (world.menu_expanded_crew == i);
            int  row_h    = expanded ? ROW_H_EXPANDED : ROW_H_COLLAPSED;

            if (mouse.x >= LIST_X && mouse.x <= LIST_X + LIST_W &&
                mouse.y >= y && mouse.y <= y + row_h) {
                return MenuAction{ MenuIntent::TOGGLE_CREW_EXPAND, MenuTab::CREW, i };
            }

            y += row_h + ROW_GAP;
        }
    }

    return std::nullopt;
}