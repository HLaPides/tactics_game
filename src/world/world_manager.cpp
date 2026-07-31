#include "world_manager.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>

WorldManager::WorldManager() {}

void WorldManager::init(WorldState& state, const std::string& start_location) {
    build_caribbean(state);
    build_ports(state);
    place_wandering_ships(state);

    state.ship_col = 40;
    state.ship_row = 18;
    for (auto& port : state.ports) {
        if (port.name == start_location) {
            state.ship_col = port.col;
            state.ship_row = port.row;
            break;
        }
    }

    reveal_fog(state, state.ship_col, state.ship_row, 4);
}

void WorldManager::build_caribbean(WorldState& state) {
    state.terrain.assign(state.grid_rows,
        std::vector<TerrainType>(state.grid_cols, TerrainType::DEEP_SEA));

    // shallow water around coasts — placeholder
    // full geography defined when art is ready
}

void WorldManager::build_ports(WorldState& state) {
    state.ports.clear();

    // format: name, faction, col, row, rep_requirement, has_town_hall
    state.ports.push_back({ "Nassau",        FactionType::PIRATE,   40, 18, 0,  true  });
    state.ports.push_back({ "Havana",        FactionType::SPANISH,  30, 16, 50, true  });
    state.ports.push_back({ "Port au Prince",FactionType::FRENCH,   45, 20, 50, true  });
    state.ports.push_back({ "Tortuga",       FactionType::PIRATE,   44, 17, 0,  false });
    state.ports.push_back({ "Port Royal",    FactionType::ENGLISH,  42, 22, 30, true  });
    state.ports.push_back({ "Santiago",      FactionType::SPANISH,  36, 20, 30, true  });
    state.ports.push_back({ "Veracruz",      FactionType::SPANISH,  18, 18, 40, true  });
    state.ports.push_back({ "New Orleans",   FactionType::FRENCH,   22, 10, 20, true  });
    state.ports.push_back({ "Galveston",     FactionType::NEUTRAL,  16, 12, 0,  false });
    state.ports.push_back({ "Bridgetown",    FactionType::ENGLISH,  58, 26, 30, true  });
}

void WorldManager::place_wandering_ships(WorldState& state) {
    // placeholder — will populate with faction patrol ships
}

void WorldManager::reveal_fog(WorldState& state, int col, int row, int radius) {
    for (int r = row - radius; r <= row + radius; r++) {
        for (int c = col - radius; c <= col + radius; c++) {
            if (r < 0 || r >= state.grid_rows) continue;
            if (c < 0 || c >= state.grid_cols) continue;
            int dist = std::max(abs(c - col), abs(r - row));
            if (dist <= radius) state.fog[r][c] = false;
        }
    }
}

bool WorldManager::move_ship(WorldState& state, int target_col, int target_row) {
    int dist = std::max(abs(target_col - state.ship_col),
                        abs(target_row - state.ship_row));
    if (dist > state.ship.speed) return false;
    if (target_col < 0 || target_col >= state.grid_cols) return false;
    if (target_row < 0 || target_row >= state.grid_rows) return false;
    if (state.terrain[target_row][target_col] == TerrainType::LAND) return false;

    state.ship_col = target_col;
    state.ship_row = target_row;

    reveal_fog(state, target_col, target_row, 3);
    advance_day(state);

    return true;
}

void WorldManager::advance_day(WorldState& state) {
    state.day++;
    state.supplies = std::max(0, state.supplies - 1);
    check_contract_expiry(state);
    update_wandering_ships(state);
}

void WorldManager::enter_port(WorldState& state, int port_index) {
    if (port_index < 0 || port_index >= (int)state.ports.size()) return;
    state.current_port = port_index;
}

void WorldManager::leave_port(WorldState& state) {
    state.current_port = -1;
}

void WorldManager::generate_tavern_contracts(WorldState& state, int port_index) {
    // placeholder — generate 2-3 low tier contracts
}

void WorldManager::generate_town_hall_contracts(WorldState& state, int port_index) {
    // placeholder — generate 1-2 high tier faction contracts
}

bool WorldManager::accept_contract(WorldState& state, int contract_index) {
    if (contract_index < 0 || contract_index >= (int)state.contracts.size())
        return false;
    return true;
}

void WorldManager::check_contract_expiry(WorldState& state) {
    state.contracts.erase(
        std::remove_if(state.contracts.begin(), state.contracts.end(),
            [&](const Contract& c) { return c.expiry_day <= state.day; }),
        state.contracts.end());
}

void WorldManager::update_wandering_ships(WorldState& state) {
    // placeholder — move enemy ships one tile toward patrol routes
}

bool WorldManager::check_ship_encounter(WorldState& state) {
    for (auto& s : state.ships) {
        if (s.is_player) continue;
        if (s.col == state.ship_col && s.row == state.ship_row)
            return true;
    }
    return false;
}

void WorldManager::update(float dt, WorldState& state) {
    // placeholder — handle timed events
}