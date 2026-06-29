#pragma once
#include "ability.h"
#include "core/types.h"

namespace AbilityDefs {
    inline Ability make_shoot() {
        return Ability("shoot", "Shoot", ActionMode::SHOOT, 1, 0, false, false);
    }
    inline Ability make_melee() {
        return Ability("melee", "Melee", ActionMode::MELEE, 1, 0, false, false);
    }
    inline Ability make_heal() {
        return Ability("heal", "Heal", ActionMode::HEAL, 1, 3, false, true);
    }
    inline Ability make_rush() {
        return Ability("rush", "Rush", ActionMode::RUSH, 0, 2, false, false);
    }
    inline Ability make_overwatch() {
        return Ability("overwatch", "Overwatch", ActionMode::OVERWATCH, 1, 0, true, false);
    }
    inline Ability make_dirty_trick() {
        return Ability("dirty_trick", "Trick", ActionMode::DIRTY_TRICK, 1, 2, false, true);
    }
    inline Ability make_aimed_shot() {
        return Ability("aimed_shot", "Aimed Shot", ActionMode::AIMED_SHOT, 2, 0, false, false);
    }
}