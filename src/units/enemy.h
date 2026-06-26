#pragma once
#include "unit.h"

class enemy : public unit {
public:
    enemy(int x_pos, int y_pos, UnitStats stats);
};