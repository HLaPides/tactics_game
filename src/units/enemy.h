#pragma once
#include "unit.h"

enum class EnemyType { SOLDIER, GUARD, CAPTAIN };

class enemy : public unit {
public:
    enemy(int x_pos, int y_pos, UnitStats stats, EnemyType type);
    EnemyType get_type() const;
private:
    EnemyType type;
};