#pragma once
#include "unit.h"

class enemy : public unit {
public:
    enemy(int x_pos, int y_pos, int mvmt, int hp);
    void draw(int tile_size);
    void draw_hp(int tile_size);
};