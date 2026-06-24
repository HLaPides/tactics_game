#include "unit.h"
#include "raylib.h"

unit::unit(int x_pos, int y_pos) {
    this->x_position = x_pos;
    this->y_position = y_pos;
}

void unit::draw(int tile_size) {
    DrawRectangle(x_position*tile_size, y_position*tile_size, tile_size, tile_size, RED);
}

void unit::set_position(int x, int y) {
    this->x_position = x;
    this->y_position = y;
}