#include "ability.h"
#include <algorithm>

Ability::Ability(std::string id, std::string label, ActionMode mode,
                 int action_cost, int max_cooldown, bool turn_ender, bool target_needed)
    : id(std::move(id))
    , label(std::move(label))
    , mode(mode)
    , action_cost(action_cost)
    , max_cooldown(max_cooldown)
    , current_cooldown(0)
    , turn_ender(turn_ender)
    , target_needed(target_needed)
{}

const std::string& Ability::get_id()            const { return id; }
const std::string& Ability::get_label()          const { return label; }
ActionMode         Ability::get_mode()           const { return mode; }
int                Ability::get_cost()           const { return action_cost; }
bool               Ability::get_ends_turn()      const { return turn_ender; }
bool               Ability::get_needs_target()   const { return target_needed; }
bool               Ability::is_ready()           const { return current_cooldown <= 0; }
int                Ability::get_cooldown()       const { return current_cooldown; }
int                Ability::get_max_cooldown()   const { return max_cooldown; }

void Ability::use() {
    current_cooldown = max_cooldown;
}

void Ability::tick() {
    if (current_cooldown > 0) current_cooldown--;
}