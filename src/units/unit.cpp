#include "unit.h"
#include "raylib.h"
#include <algorithm>

unit::unit(int x_pos, int y_pos, int mvmt) {
    x_position = x_pos;
    y_position = y_pos;
    movement = mvmt;
    actions_remaining = MAX_ACTIONS;
}

int unit::get_movement() { 
    return movement; 
}

int unit::get_x_pos() { 
    return x_position; 
}

int unit::get_y_pos() { 
    return y_position; 
}

int unit::get_actions() { 
    return actions_remaining; 
}

void unit::use_action() { 
    if (actions_remaining > 0) actions_remaining--; 
}

void unit::reset_actions() { 
    actions_remaining = MAX_ACTIONS; 
}

void unit::draw(int tile_size) {
    DrawRectangle(x_position * tile_size, y_position * tile_size, tile_size, tile_size, RED);
}

void unit::draw_range(int tile_size, int cols, int rows) {
    if (actions_remaining <= 0) return;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int dist = std::max(abs(col - x_position), abs(row - y_position));
            if (dist <= movement && dist > 0) {
                DrawRectangleLines(col * tile_size, row * tile_size, tile_size, tile_size, Fade(YELLOW, 0.8f));
            }
        }
    }
}

void unit::set_position(int x, int y) {
    x_position = x;
    y_position = y;
}