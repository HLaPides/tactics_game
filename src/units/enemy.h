#pragma once
#include "unit.h"

class enemy : public unit {
public:
    enemy(int x_pos, int y_pos, int mvmt);
    void draw(int tile_size);  // override to draw blue so you can see it
};