#pragma once
#include <string>
#include <vector>

enum TileType  { TILE_FLOOR, TILE_WALL, TILE_BARREL, TILE_RAILING };
enum CoverType { COVER_NONE, COVER_HALF, COVER_FULL };

struct CoverFaces { bool north, south, east, west; };

struct Tile {
    TileType   type;
    CoverType  cover;
    CoverFaces faces;
    bool       walkable;
};

struct SpawnPoint { int col, row; };

class map {
public:
    map(int tile_size);
    bool load(const std::string& filepath);
    void draw_map();
    int  get_tile_size();
    int  get_x_dimension();
    int  get_y_dimension();
    int  getCols();
    int  getRows();
    Tile get_tile(int col, int row);
    bool is_walkable(int col, int row);
    std::vector<SpawnPoint> get_player_spawns();
    std::vector<SpawnPoint> get_enemy_spawns();
private:
    int tile_size;
    int cols;
    int rows;
    std::vector<std::vector<Tile>> tiles;
    std::vector<SpawnPoint>        player_spawns;
    std::vector<SpawnPoint>        enemy_spawns;
    Tile make_tile(char c);
};