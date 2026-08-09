#pragma once
#include <vector>
#include <string>

enum class TerrainType {
    DEEP_SEA,
    SHALLOW_SEA,
    LAND,
    PORT
};

enum class FactionType {
    NEUTRAL,
    SPANISH,
    ENGLISH,
    FRENCH,
    PIRATE
};

enum class PortTab {
    SHOP,
    TAVERN,
    SHIPYARD,
    TOWN_HALL
};

struct PortLocation {
    std::string  name;
    FactionType  faction;
    int          col;
    int          row;
    int          rep_requirement; // min reputation to access town hall
    bool         has_town_hall;
};

enum class ShipState {
    WANDER,
    FLEE
};

struct WorldShip {
    std::string  id;
    FactionType  faction;
    int          col;
    int          row;
    bool         is_player;

    ShipState    state         = ShipState::WANDER;
    int          home_col      = 0;
    int          home_row      = 0;
    int          wander_radius = 4;
    int          target_col    = 0;
    int          target_row    = 0;
    int          detect_radius = 3;
};

struct Contract {
    enum class Type { HIT, SINK, ESCORT };
    Type        type;
    std::string target;         // display label
    std::string target_ship_id; // set only once accepted; empty while just an offer
    std::string destination;    // for escort
    int         gold_reward;
    int         renown_reward;
    int         expiry_day;
    FactionType faction;
    bool        is_town_hall;
    int         origin_port = -1;
};

struct PlayerShip {
    std::string name        = "";
    int         hull        = 10;
    int         max_hull    = 10;
    int         speed       = 1;   // tiles per turn
    int         cargo       = 10;
    int         crew_cap    = 6;
    std::string flag        = "pirate";
    std::string figurehead  = "default";
};

struct WorldState {
    // grid
    int                                    grid_cols = 80;
    int                                    grid_rows = 40;
    std::vector<std::vector<TerrainType>> terrain;
    std::vector<std::vector<bool>>        fog;

    // player
    int         ship_col    = 0;
    int         ship_row    = 0;
    int         gold        = 500;
    int         supplies    = 20;
    int         day         = 1;
    int         reputation  = 0;
    PlayerShip  ship;

    // faction relations — indexed by FactionType
    int faction_relations[5] = { 0, 0, 0, 0, 50 }; // neutral with all, friendly with pirates

    // locations
    std::vector<PortLocation> ports;

    // wandering ships
    std::vector<WorldShip> ships;

    // contract board (offers currently displayed at the port you're in)
    std::vector<Contract> contracts;

    // contracts the player has accepted and is actively working
    std::vector<Contract> active_contracts;

    // which port the player is currently in (-1 = at sea)
    int     current_port          = -1;
    PortTab port_tab               = PortTab::TAVERN;
    int     pending_engage_ship    = -1;
    int     pending_contract_index = -1; // index into active_contracts; -1 = none
};