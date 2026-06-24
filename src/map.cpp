#include "map.h"
#include "raylib.h"

map::map(int x, int y, int tile_size) {
    this->x_dimension = x;
    this->y_dimension = y;
    this->tile_size = tile_size;
}

void map::draw_map() {
    int rows = y_dimension/tile_size;
    int cols = x_dimension/tile_size;
    for (int row = 0; row<rows; row++) {
        for (int col = 0; col<cols; col++) {
            DrawRectangleLines(col*tile_size, row*tile_size, tile_size, tile_size, BLACK);
    }
}

int map::get_tile_size() {
    return tile_size;
}
}