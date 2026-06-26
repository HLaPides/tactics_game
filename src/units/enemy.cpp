#include "enemy.h"

enemy::enemy(int x_pos, int y_pos, UnitStats stats, EnemyType type)
    : unit(x_pos, y_pos, stats), type(type) {}

EnemyType enemy::get_type() const { return type; }