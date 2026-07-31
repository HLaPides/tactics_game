#pragma once
#include "world_state.h"
#include "../core/types.h"
#include <string>

class WorldManager {
public:
    WorldManager();
    void init(WorldState& state, const std::string& start_location);
    void update(float dt, WorldState& state);

    // player actions
    bool move_ship(WorldState& state, int target_col, int target_row);
    void enter_port(WorldState& state, int port_index);
    void leave_port(WorldState& state);

    // time
    void advance_day(WorldState& state);

    // contracts
    void generate_tavern_contracts(WorldState& state, int port_index);
    void generate_town_hall_contracts(WorldState& state, int port_index);
    bool accept_contract(WorldState& state, int contract_index);
    void check_contract_expiry(WorldState& state);

    // wandering ships
    void update_wandering_ships(WorldState& state);
    bool check_ship_encounter(WorldState& state);

private:
    void build_caribbean(WorldState& state);
    void build_ports(WorldState& state);
    void place_wandering_ships(WorldState& state);
    void reveal_fog(WorldState& state, int col, int row, int radius);
};