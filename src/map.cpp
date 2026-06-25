#include "map.h"
#include "raylib.h"
#include <fstream>
#include <string>

map::map(int tile_size) {
    this->tile_size = tile_size;
    this->cols      = 0;
    this->rows      = 0;
}

Tile map::make_tile(char c) {
    Tile t;
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

bool map::load(const std::string& filepath) {
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
            if (c == 'P') {
                player_spawns.push_back({col_idx, row_idx});
                row.push_back(make_tile('.'));
            } else if (c == 'E') {
                enemy_spawns.push_back({col_idx, row_idx});
                row.push_back(make_tile('.'));
            } else {
                row.push_back(make_tile(c));
            }
        }
        tiles.push_back(row);
        row_idx++;
    }

    rows = (int)tiles.size();
    cols = rows > 0 ? (int)tiles[0].size() : 0;
    return true;
}

void map::draw_map() {
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < (int)tiles[row].size(); col++) {  // use actual row size
            int    x = col * tile_size;
            int    y = row * tile_size;
            Tile&  t = tiles[row][col];
            switch (t.type) {
                case TILE_WALL:    DrawRectangle(x, y, tile_size, tile_size, GRAY);      break;
                case TILE_BARREL:  DrawRectangle(x, y, tile_size, tile_size, BROWN);     break;
                case TILE_RAILING: DrawRectangle(x, y, tile_size, tile_size, DARKBROWN); break;
                default: break;
            }
            DrawRectangleLines(x, y, tile_size, tile_size, BLACK);
        }
    }
}

int  map::get_tile_size()   { return tile_size; }
int  map::get_x_dimension() { return cols * tile_size; }
int  map::get_y_dimension() { return rows * tile_size; }
int  map::getCols()         { return cols; }
int  map::getRows()         { return rows; }

Tile map::get_tile(int col, int row) {
    if (row < 0 || row >= rows || col < 0 || col >= cols)
        return make_tile('.');
    return tiles[row][col];
}

bool map::is_walkable(int col, int row) {
    return get_tile(col, row).walkable;
}

std::vector<SpawnPoint> map::get_player_spawns() { return player_spawns; }
std::vector<SpawnPoint> map::get_enemy_spawns()  { return enemy_spawns; }