#include "map.h"
#include <fstream>
#include <string>

GameMap::GameMap() : GameMap(32) {}

GameMap::GameMap(int tile_size) {
    this->tile_size = tile_size;
    cols            = 0;
    rows            = 0;
}

Tile GameMap::make_tile(char c) {
    Tile t;
    t.is_objective = false;
    switch (c) {
        case 'W':
            t.type     = TILE_WALL;
            t.cover    = COVER_FULL;
            t.faces    = {true, true, true, true};
            t.walkable = false;
            break;
        case 'B':
            t.type     = TILE_BARREL;
            t.cover    = COVER_HALF;
            t.faces    = {true, true, true, true};
            t.walkable = false;
            break;
        case 'R':
            t.type     = TILE_RAILING;
            t.cover    = COVER_HALF;
            t.faces    = {true, true, false, false};
            t.walkable = false;
            break;
        default:
            t.type     = TILE_FLOOR;
            t.cover    = COVER_NONE;
            t.faces    = {false, false, false, false};
            t.walkable = true;
            break;
    }
    return t;
}

bool GameMap::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    tiles.clear();
    player_spawns.clear();
    enemy_spawns.clear();

    std::string line;
    int row_idx = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::vector<Tile> row;
        for (int col_idx = 0; col_idx < (int)line.size(); col_idx++) {
            char c = line[col_idx];
            Tile t = make_tile('.');

            if (c >= '1' && c <= '9') {
                // player spawn — digit encodes unit type
                player_spawns.push_back({col_idx, row_idx, c});
            } else if (c == 'E' || c == 'G' || c == 'C') {
                // enemy spawn — letter encodes enemy type
                enemy_spawns.push_back({col_idx, row_idx, c});
            } else if (c == 'Q') {
                // objective tile — walkable floor that triggers win
                t.is_objective = true;
            } else {
                t = make_tile(c);
            }
            row.push_back(t);
        }
        tiles.push_back(row);
        row_idx++;
    }

    rows = (int)tiles.size();
    cols = rows > 0 ? (int)tiles[0].size() : 0;
    return true;
}

int  GameMap::get_tile_size()   const { return tile_size; }
int  GameMap::get_x_dimension() const { return cols * tile_size; }
int  GameMap::get_y_dimension() const { return rows * tile_size; }
int  GameMap::getCols()         const { return cols; }
int  GameMap::getRows()         const { return rows; }

Tile GameMap::get_tile(int col, int row) {
    if (row < 0 || row >= rows || col < 0 || col >= cols)
        return make_tile('.');
    return tiles[row][col];
}

bool GameMap::is_walkable(int col, int row) {
    return get_tile(col, row).walkable;
}

bool GameMap::is_objective(int col, int row) {
    if (row < 0 || row >= rows || col < 0 || col >= cols) return false;
    return tiles[row][col].is_objective;
}

const std::vector<std::vector<Tile>>& GameMap::get_tiles() const {
    return tiles;
}

std::vector<SpawnPoint> GameMap::get_player_spawns() { return player_spawns; }
std::vector<SpawnPoint> GameMap::get_enemy_spawns()  { return enemy_spawns; }