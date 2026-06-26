#pragma once
#include "../types.h"

class unit {
public:
    unit() : unit(0, 0, UnitStats{}) {}
    unit(int x_pos, int y_pos, UnitStats stats);
    int  get_x_pos()        const;
    int  get_y_pos()        const;
    int  get_movement()     const;
    int  get_actions()      const;
    int  get_hp()           const;
    int  get_max_hp()       const;
    int  get_aim()          const;
    int  get_defense()      const;
    int  get_shoot_range()  const;
    int  get_shoot_damage() const;
    int  get_melee_damage() const;
    bool is_alive()         const;
    void use_action();
    void reset_actions();
    void take_damage(int amount);
    void set_position(int x, int y);
private:
    int x_position;
    int y_position;
    int movement;
    int actions_remaining;
    int hp;
    int max_hp;
    int aim;
    int defense;
    int shoot_range;
    int shoot_damage;
    int melee_damage;
    static const int MAX_ACTIONS = 2;
};