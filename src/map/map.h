#pragma once
#include <string>
#include <vector>

enum TileType  { TILE_FLOOR, TILE_WALL, TILE_BARREL, TILE_RAILING, TILE_OBJECTIVE };
enum CoverType { COVER_NONE, COVER_HALF, COVER_FULL };

struct CoverFaces { bool north, south, east, west; };

struct Tile {
    TileType   type         = TILE_FLOOR;
    CoverType  cover        = COVER_NONE;
    CoverFaces faces        = {false, false, false, false};
    bool       walkable     = true;
    bool       is_objective = false;
    int        tile_id      = 0;
};

struct SpawnPoint {
    int  col, row;
    char type;
};

struct Objective {
    enum class Type { KILL_UNIT, HOLD_TILE };
    Type        type;
    std::string target;  // for KILL_UNIT — "captain", "soldier", etc.
    std::string label;   // display text in objectives panel
    int         col = -1;
    int         row = -1;
};

class GameMap {
public:
    GameMap();
    GameMap(int tile_size);
    bool load(const std::string& filepath);
    int  get_tile_size()   const;
    int  get_x_dimension() const;
    int  get_y_dimension() const;
    int  getCols()         const;
    int  getRows()         const;
    Tile get_tile(int col, int row) const;
    bool is_walkable(int col, int row) const;
    bool is_objective(int col, int row) const;
    const std::vector<std::vector<Tile>>& get_tiles() const;
    const std::vector<Objective>&         get_objectives() const;
    std::vector<SpawnPoint> get_player_spawns();
    std::vector<SpawnPoint> get_enemy_spawns();
private:
    int tile_size;
    int cols;
    int rows;
    std::vector<std::vector<Tile>> tiles;
    std::vector<SpawnPoint>        player_spawns;
    std::vector<SpawnPoint>        enemy_spawns;
    std::vector<Objective>         objectives;

    Tile tile_from_id(int id) const;

    static std::string      read_file(const std::string& path);
    static int              json_int(const std::string& src, const std::string& key, int def = 0);
    static std::string      json_string(const std::string& src, const std::string& key, const std::string& def = "");
    static std::vector<int> json_int_array(const std::string& src, const std::string& key);
};