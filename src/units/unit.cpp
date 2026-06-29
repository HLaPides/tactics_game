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
    on_overwatch      = false;
    aim_penalty       = 0;
    aim_penalty_turns = 0;
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
int  unit::get_aim_penalty()  const { return aim_penalty; }
bool unit::is_alive()         const { return hp > 0; }
bool unit::is_on_overwatch()  const { return on_overwatch; }

void unit::use_action() {
    if (actions_remaining > 0) actions_remaining--;
}

void unit::use_actions(int count) {
    actions_remaining = std::max(0, actions_remaining - count);
}

void unit::reset_actions() {
    actions_remaining = MAX_ACTIONS;
    on_overwatch      = false;
}

void unit::take_damage(int amount) {
    hp = std::max(0, hp - amount);
}

void unit::heal(int amount) {
    hp = std::min(max_hp, hp + amount);
}

void unit::set_position(int x, int y) {
    x_position = x;
    y_position = y;
}

void unit::set_overwatch(bool val) {
    on_overwatch = val;
}

void unit::clear_overwatch() {
    on_overwatch = false;
}

void unit::apply_aim_penalty(int amount, int turns) {
    aim_penalty       = amount;
    aim_penalty_turns = turns;
}

void unit::tick_aim_penalty() {
    if (aim_penalty_turns > 0) {
        aim_penalty_turns--;
        if (aim_penalty_turns <= 0)
            aim_penalty = 0;
    }
}

void unit::add_ability(const Ability& a) {
    for (auto& existing : abilities)
        if (existing.get_id() == a.get_id()) return;
    abilities.push_back(a);
}

void unit::remove_ability(const std::string& id) {
    abilities.erase(
        std::remove_if(abilities.begin(), abilities.end(),
            [&id](const Ability& a) { return a.get_id() == id; }),
        abilities.end());
}

bool unit::has_ability(const std::string& id) const {
    for (auto& a : abilities)
        if (a.get_id() == id) return true;
    return false;
}

Ability* unit::get_ability(const std::string& id) {
    for (auto& a : abilities)
        if (a.get_id() == id) return &a;
    return nullptr;
}

const std::vector<Ability>& unit::get_abilities() const {
    return abilities;
}

void unit::tick_cooldowns() {
    for (auto& a : abilities)
        a.tick();
}

void unit::reset_hp() {
    hp = max_hp;
}