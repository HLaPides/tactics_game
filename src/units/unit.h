#pragma once
#include "../core/types.h"
#include "../abilities/ability.h"
#include <vector>
#include <string>

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
    int  get_sight_range()  const;
    bool is_alive()         const;

    void use_action();
    void use_actions(int count);
    void reset_actions();
    void take_damage(int amount);
    void heal(int amount);
    void set_position(int x, int y);
    void reset_hp();

    // aim penalty (dirty trick / debuffs)
    void apply_aim_penalty(int amount, int turns);
    void tick_aim_penalty();
    int  get_aim_penalty()  const;

    // abilities
    void                        add_ability(const Ability& a);
    void                        remove_ability(const std::string& id);
    bool                        has_ability(const std::string& id) const;
    Ability*                    get_ability(const std::string& id);
    const std::vector<Ability>& get_abilities() const;
    void                        tick_cooldowns();

private:
    int  x_position;
    int  y_position;
    int  movement;
    int  actions_remaining;
    int  hp;
    int  max_hp;
    int  aim;
    int  defense;
    int  shoot_range;
    int  shoot_damage;
    int  melee_damage;
    int  sight_range;
    int  aim_penalty       = 0;
    int  aim_penalty_turns = 0;
    std::vector<Ability> abilities;
    static const int MAX_ACTIONS = 2;
};