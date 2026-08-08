#include "port_input.h"

PortInput::PortInput(const AppConfig& cfg) : config(cfg) {}

std::optional<PortAction> PortInput::poll(const WorldState& state) {
    if (IsKeyPressed(KEY_ESCAPE)) return PortAction{ PortIntent::LEAVE_PORT };
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return std::nullopt;

    Vector2 mouse = GetMousePosition();

    const PortTab tabs[4] = { PortTab::SHOP, PortTab::TAVERN,
                              PortTab::SHIPYARD, PortTab::TOWN_HALL };
    for (int i = 0; i < 4; i++) {
        int tx = 20 + i * (TAB_W + 8);
        if (mouse.x >= tx && mouse.x <= tx + TAB_W &&
            mouse.y >= TAB_Y && mouse.y <= TAB_Y + TAB_H) {
            return PortAction{ PortIntent::SWITCH_TAB, tabs[i] };
        }
    }

    if (state.port_tab == PortTab::TAVERN) {
        int count = 0;
        for (auto& c : state.contracts) {
            if (c.is_town_hall) continue;
            int ry = LIST_Y + count * ROW_H;
            if (mouse.x >= LIST_X && mouse.x <= LIST_X + 500 &&
                mouse.y >= ry && mouse.y <= ry + ROW_H - 8) {
                return PortAction{ PortIntent::ACCEPT_TAVERN_CONTRACT, PortTab::TAVERN, count };
            }
            count++;
        }
    } else if (state.port_tab == PortTab::TOWN_HALL) {
        int count = 0;
        for (auto& c : state.contracts) {
            if (!c.is_town_hall) continue;
            int ry = LIST_Y + count * ROW_H;
            if (mouse.x >= LIST_X && mouse.x <= LIST_X + 500 &&
                mouse.y >= ry && mouse.y <= ry + ROW_H - 8) {
                return PortAction{ PortIntent::ACCEPT_TOWN_HALL_CONTRACT, PortTab::TOWN_HALL, count };
            }
            count++;
        }
    } else if (state.port_tab == PortTab::SHOP) {
        Rectangle btn{ (float)LIST_X, (float)LIST_Y, 220, 50 };
        if (mouse.x >= btn.x && mouse.x <= btn.x + btn.width &&
            mouse.y >= btn.y && mouse.y <= btn.y + btn.height) {
            return PortAction{ PortIntent::BUY_SUPPLIES };
        }
    } else if (state.port_tab == PortTab::SHIPYARD) {
        Rectangle btn{ (float)LIST_X, (float)LIST_Y, 220, 50 };
        if (mouse.x >= btn.x && mouse.x <= btn.x + btn.width &&
            mouse.y >= btn.y && mouse.y <= btn.y + btn.height) {
            return PortAction{ PortIntent::REPAIR_SHIP };
        }
    }

    return std::nullopt;
}