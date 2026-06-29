#pragma once
#include <string>

// forward declare to avoid circular include with types.h
enum class ActionMode;

class Ability {
public:
    Ability(std::string id, std::string label, ActionMode mode,
            int action_cost, int max_cooldown, bool turn_ender, bool target_needed);

    const std::string& get_id()        const;
    const std::string& get_label()     const;
    ActionMode         get_mode()      const;
    int                get_cost()      const;
    bool               get_ends_turn() const;
    bool               get_needs_target() const;
    bool               is_ready()      const;
    int                get_cooldown()  const;
    int                get_max_cooldown() const;
    void               use();   // sets cooldown to max
    void               tick();  // decrements cooldown by 1

private:
    std::string id;
    std::string label;
    ActionMode  mode;
    int         action_cost;
    int         max_cooldown;
    int         current_cooldown;
    bool        turn_ender;
    bool        target_needed;
};