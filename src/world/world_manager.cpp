#include "world_manager.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

WorldManager::WorldManager() {}

// ─── init ───────────────────────────────────────────────────────────────────

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

// ─── ship spawning ────────────────────────────────────────────────────────────

void WorldManager::spawn_ship(WorldState& state, FactionType faction, ShipState role,
                                int home_col, int home_row) {
    WorldShip s;
    s.id            = faction == FactionType::PIRATE ? "pirate_ship" : "faction_ship";
    s.faction       = faction;
    s.col           = home_col;
    s.row           = home_row;
    s.is_player     = false;
    s.state         = role;
    s.home_col      = home_col;
    s.home_row      = home_row;
    s.wander_radius = 4;
    s.target_col    = home_col;
    s.target_row    = home_row;
    s.detect_radius = 3;

    state.ships.push_back(s);
}

void WorldManager::place_wandering_ships(WorldState& state) {
    state.ships.clear();

    for (auto& port : state.ports) {
        if (port.faction == FactionType::NEUTRAL) continue;

        for (int i = 0; i < SHIPS_PER_FACTION; i++) {
            int offset_col = 2 + (rand() % 3);   // 2-4 tiles away
            int offset_row = 2 + (rand() % 3);
            int sign_c = (rand() % 2 == 0) ? 1 : -1;
            int sign_r = (rand() % 2 == 0) ? 1 : -1;

            int spawn_col = std::max(0, std::min(state.grid_cols - 1,
                              port.col + offset_col * sign_c));
            int spawn_row = std::max(0, std::min(state.grid_rows - 1,
                              port.row + offset_row * sign_r));

            spawn_ship(state, port.faction, ShipState::WANDER, port.col, port.row);
            state.ships.back().col        = spawn_col;
            state.ships.back().row        = spawn_row;
            state.ships.back().target_col = spawn_col;
            state.ships.back().target_row = spawn_row;
        }
    }
}

std::string WorldManager::spawn_contract_ship(WorldState& state, FactionType faction,
                                                int near_col, int near_row) {
    static int contract_ship_counter = 0;
    std::string id = "contract_ship_" + std::to_string(contract_ship_counter++);

    int offset_col = 3 + (rand() % 4);
    int offset_row = 3 + (rand() % 4);
    int sign_c = (rand() % 2 == 0) ? 1 : -1;
    int sign_r = (rand() % 2 == 0) ? 1 : -1;

    int spawn_col = std::max(0, std::min(state.grid_cols - 1, near_col + offset_col * sign_c));
    int spawn_row = std::max(0, std::min(state.grid_rows - 1, near_row + offset_row * sign_r));

    spawn_ship(state, faction, ShipState::WANDER, near_col, near_row);
    WorldShip& s   = state.ships.back();
    s.id           = id;
    s.col          = spawn_col;
    s.row          = spawn_row;
    s.target_col   = spawn_col;
    s.target_row   = spawn_row;

    return id;
}

void WorldManager::pick_new_wander_target(WorldShip& s) {
    int dx = (rand() % (s.wander_radius * 2 + 1)) - s.wander_radius;
    int dy = (rand() % (s.wander_radius * 2 + 1)) - s.wander_radius;
    s.target_col = s.home_col + dx;
    s.target_row = s.home_row + dy;
}

// ─── fog ──────────────────────────────────────────────────────────────────────

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

// ─── player movement ──────────────────────────────────────────────────────────

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

int WorldManager::ship_index_at(const WorldState& state, int col, int row) const {
    for (int i = 0; i < (int)state.ships.size(); i++) {
        if (state.ships[i].is_player) continue;
        if (state.ships[i].col == col && state.ships[i].row == row) return i;
    }
    return -1;
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
    state.port_tab      = PortTab::TAVERN;
    generate_tavern_contracts(state, port_index);
    generate_town_hall_contracts(state, port_index);
}

void WorldManager::leave_port(WorldState& state) {
    state.current_port = -1;
}

// ─── contracts ────────────────────────────────────────────────────────────────

static Contract::Type random_contract_type() {
    return (rand() % 2 == 0) ? Contract::Type::HIT : Contract::Type::SINK;
}

void WorldManager::generate_tavern_contracts(WorldState& state, int port_index) {
    if (port_index < 0 || port_index >= (int)state.ports.size()) return;
    const auto& port = state.ports[port_index];

    state.contracts.erase(
        std::remove_if(state.contracts.begin(), state.contracts.end(),
            [](const Contract& c) { return !c.is_town_hall; }),
        state.contracts.end());

    int count = 2 + (rand() % 2); // 2-3
    for (int i = 0; i < count; i++) {
        Contract c;
        c.type           = random_contract_type();
        c.target         = "Wandering vessel";
        c.target_ship_id = "";       // no ship yet — spawned only on accept
        c.origin_port    = port_index;
        c.gold_reward    = 100 + (rand() % 150) + state.day * 2;
        c.renown_reward  = 5 + (rand() % 10);
        c.expiry_day     = state.day + 10 + (rand() % 10);
        c.faction        = port.faction;
        c.is_town_hall   = false;
        state.contracts.push_back(c);
    }
}

void WorldManager::generate_town_hall_contracts(WorldState& state, int port_index) {
    if (port_index < 0 || port_index >= (int)state.ports.size()) return;
    const auto& port = state.ports[port_index];
    if (!port.has_town_hall) return;
    if (state.reputation < port.rep_requirement) return;

    state.contracts.erase(
        std::remove_if(state.contracts.begin(), state.contracts.end(),
            [](const Contract& c) { return c.is_town_hall; }),
        state.contracts.end());

    int count = 1 + (rand() % 2); // 1-2
    for (int i = 0; i < count; i++) {
        Contract c;
        c.type           = random_contract_type();
        c.target         = "Marked vessel";
        c.target_ship_id = "";
        c.origin_port    = port_index;
        c.gold_reward    = 300 + (rand() % 300) + state.day * 4;
        c.renown_reward  = 15 + (rand() % 20);
        c.expiry_day     = state.day + 14 + (rand() % 14);
        c.faction        = port.faction;
        c.is_town_hall   = true;
        state.contracts.push_back(c);
    }
}

int WorldManager::board_contract_index(const WorldState& state, bool town_hall, int displayed_index) const {
    int count = -1;
    for (int i = 0; i < (int)state.contracts.size(); i++) {
        if (state.contracts[i].is_town_hall != town_hall) continue;
        count++;
        if (count == displayed_index) return i;
    }
    return -1;
}

bool WorldManager::accept_contract(WorldState& state, int contract_index) {
    if (contract_index < 0 || contract_index >= (int)state.contracts.size())
        return false;

    Contract c = state.contracts[contract_index];

    if (c.target_ship_id.empty() && c.type != Contract::Type::ESCORT &&
        c.origin_port >= 0 && c.origin_port < (int)state.ports.size()) {
        const auto& port = state.ports[c.origin_port];
        c.target_ship_id = spawn_contract_ship(state, port.faction, port.col, port.row);
    }

    state.active_contracts.push_back(c);
    state.contracts.erase(state.contracts.begin() + contract_index);
    return true;
}


void WorldManager::check_contract_expiry(WorldState& state) {
    // when an active contract expires, its dedicated ship stops being
    // a marked target — simplest correct behavior is to despawn it
    // along with the contract.
    for (auto it = state.active_contracts.begin(); it != state.active_contracts.end(); ) {
        if (it->expiry_day <= state.day) {
            std::string dead_id = it->target_ship_id;
            state.ships.erase(
                std::remove_if(state.ships.begin(), state.ships.end(),
                    [&](const WorldShip& s) { return s.id == dead_id; }),
                state.ships.end());
            it = state.active_contracts.erase(it);
        } else {
            ++it;
        }
    }
}

int WorldManager::find_active_contract_for_ship(const WorldState& state, const std::string& ship_id) const {
    if (ship_id.empty()) return -1;
    for (int i = 0; i < (int)state.active_contracts.size(); i++) {
        if (state.active_contracts[i].target_ship_id == ship_id) return i;
    }
    return -1;
}

void WorldManager::resolve_contract(WorldState& state, int active_contract_index) {
    if (active_contract_index < 0 || active_contract_index >= (int)state.active_contracts.size())
        return;

    const Contract& c = state.active_contracts[active_contract_index];
    state.gold       += c.gold_reward;
    state.reputation += c.renown_reward;

    std::string dead_id = c.target_ship_id;
    state.ships.erase(
        std::remove_if(state.ships.begin(), state.ships.end(),
            [&](const WorldShip& s) { return s.id == dead_id; }),
        state.ships.end());

    state.active_contracts.erase(state.active_contracts.begin() + active_contract_index);
}

// ─── port services ─────────────────────────────────────────────────────────────

void WorldManager::buy_supplies(WorldState& state, int amount, int cost_per_unit) {
    int total = amount * cost_per_unit;
    if (state.gold < total) return;
    state.gold     -= total;
    state.supplies += amount;
}

void WorldManager::repair_ship(WorldState& state, int cost_per_hp) {
    int missing = state.ship.max_hull - state.ship.hull;
    if (missing <= 0) return;
    int total = missing * cost_per_hp;
    if (state.gold < total) return;
    state.gold      -= total;
    state.ship.hull  = state.ship.max_hull;
}

// ─── wandering ship AI ─────────────────────────────────────────────────────────

static int step_toward(int cur, int target) {
    if (cur < target) return cur + 1;
    if (cur > target) return cur - 1;
    return cur;
}

static bool at_target(const WorldShip& s) {
    return s.col == s.target_col && s.row == s.target_row;
}

void WorldManager::update_wandering_ships(WorldState& state) {
    for (auto& s : state.ships) {
        if (s.is_player) continue;

        switch (s.state) {
            case ShipState::WANDER:
                if (at_target(s)) pick_new_wander_target(s);
                break;

            case ShipState::FLEE:
                s.target_col = s.col + (s.col - state.ship_col);
                s.target_row = s.row + (s.row - state.ship_row);
                break;
        }

        int next_col = step_toward(s.col, s.target_col);
        int next_row = step_toward(s.row, s.target_row);

        bool blocked_by_land = next_col < 0 || next_col >= state.grid_cols ||
                               next_row < 0 || next_row >= state.grid_rows ||
                               state.terrain[next_row][next_col] == TerrainType::LAND;

        bool blocked_by_player = next_col == state.ship_col && next_row == state.ship_row;

        if (!blocked_by_land && !blocked_by_player) {
            s.col = next_col;
            s.row = next_row;
        }
    }
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