#include "enemy.h"
#include "raylib.h"

enemy::enemy(int x_pos, int y_pos, int mvmt, int hp) : unit(x_pos, y_pos, mvmt, hp) {}

void enemy::draw(int tile_size) {
    if (!is_alive()) return;
    DrawRectangle(get_x_pos() * tile_size, get_y_pos() * tile_size, tile_size, tile_size, BLUE);
}

void enemy::draw_hp(int tile_size) {
    if (!is_alive()) return;
    int x = get_x_pos() * tile_size;
    int y = get_y_pos() * tile_size;
    for (int i = 0; i < get_max_hp(); i++) {
        Color pip_color = i < get_hp() ? GREEN : DARKGRAY;
        DrawRectangle(x + 4 + i * 10, y - 10, 8, 6, pip_color);
    }
}