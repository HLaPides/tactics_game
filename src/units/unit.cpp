#include "unit.h"
#include <algorithm>

unit::unit(int x_pos, int y_pos, UnitStats s) {
    x_position        = x_pos;
    y_position        = y_pos;
    movement          = s.movement;
    actions_remaining = MAX_ACTIONS;
    hp                = s.hp;
    max_hp            = s.hp;
    aim               = s.aim;
    defense           = s.defense;
    shoot_range       = s.shoot_range;
    shoot_damage      = s.shoot_damage;
    melee_damage      = s.melee_damage;
    sight_range       = s.sight_range;
}

int  unit::get_x_pos()        const { return x_position; }
int  unit::get_y_pos()        const { return y_position; }
int  unit::get_movement()     const { return movement; }
int  unit::get_actions()      const { return actions_remaining; }
int  unit::get_hp()           const { return hp; }
int  unit::get_max_hp()       const { return max_hp; }
int  unit::get_aim()          const { return aim; }
int  unit::get_defense()      const { return defense; }
int  unit::get_shoot_range()  const { return shoot_range; }
int  unit::get_shoot_damage() const { return shoot_damage; }
int  unit::get_melee_damage() const { return melee_damage; }
int  unit::get_sight_range()  const { return sight_range; }
bool unit::is_alive()         const { return hp > 0; }

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