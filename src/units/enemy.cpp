#include "enemy.h"
#include "raylib.h"

enemy::enemy(int x_pos, int y_pos, int mvmt) : unit(x_pos, y_pos, mvmt) {}

void enemy::draw(int tile_size) {
    DrawRectangle(get_x_pos() * tile_size, get_y_pos() * tile_size, tile_size, tile_size, BLUE);
}