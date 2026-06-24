#include "unit.h"
#include "raylib.h"
#include <algorithm>

unit::unit(int x_pos, int y_pos, int mvmt, int hp, int shoot_range, int shoot_dmg, int melee_dmg) {
    x_position        = x_pos;
    y_position        = y_pos;
    movement          = mvmt;
    actions_remaining = MAX_ACTIONS;
    this->hp          = hp;
    max_hp            = hp;
    this->shoot_range  = shoot_range;
    shoot_damage      = shoot_dmg;
    melee_damage      = melee_dmg;
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

int unit::get_hp() { 
    return hp; 
}

int unit::get_max_hp() { 
    return max_hp; 
}

int unit::get_shoot_range() { 
    return shoot_range; 
}

int unit::get_shoot_damage() { 
    return shoot_damage; 
}

int unit::get_melee_damage() { 
    return melee_damage; 
}

bool unit::is_alive() { 
    return hp > 0; 
}

void unit::use_action() { 
    if (actions_remaining > 0) actions_remaining--; 
}

void unit::reset_actions() { 
    actions_remaining = MAX_ACTIONS; 
}

void unit::take_damage(int amount) { 
    hp = std::max(0, hp - amount); 
}

void unit::set_position(int x, int y) {
    x_position = x;
    y_position = y;
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