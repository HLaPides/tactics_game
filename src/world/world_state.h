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
    HUNT,
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
    std::string target;       // enemy id or location name
    std::string destination;  // for escort
    int         gold_reward;
    int         renown_reward;
    int         expiry_day;
    FactionType faction;
    bool        is_town_hall; // false = tavern
};

struct PlayerShip {
    std::string name        = "The Mutiny";
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

    // active contracts
    std::vector<Contract> contracts;

    // which port the player is currently in (-1 = at sea)
    int  current_port        = -1;
    bool pending_ambush       = false;
    int  pending_engage_ship  = -1;
};