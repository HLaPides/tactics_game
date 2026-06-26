#pragma once
#include <string>
#include <vector>

enum TileType  { TILE_FLOOR, TILE_WALL, TILE_BARREL, TILE_RAILING };
enum CoverType { COVER_NONE, COVER_HALF, COVER_FULL };

struct CoverFaces { bool north, south, east, west; };

struct Tile {
    TileType   type         = TILE_FLOOR;
    CoverType  cover        = COVER_NONE;
    CoverFaces faces        = {false, false, false, false};
    bool       walkable     = true;
    bool       is_objective = false;
};

struct SpawnPoint {
    int  col, row;
    char type;
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
    Tile get_tile(int col, int row);
    bool is_walkable(int col, int row);
    bool is_objective(int col, int row);
    const std::vector<std::vector<Tile>>& get_tiles() const;
    std::vector<SpawnPoint> get_player_spawns();
    std::vector<SpawnPoint> get_enemy_spawns();
private:
    int tile_size;
    int cols;
    int rows;
    std::vector<std::vector<Tile>> tiles;
    std::vector<SpawnPoint>        player_spawns;
    std::vector<SpawnPoint>        enemy_spawns;
    static Tile make_tile(char c);
};