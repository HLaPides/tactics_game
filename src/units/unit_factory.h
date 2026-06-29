#pragma once
#include "unit.h"
#include "enemy.h"
#include "../abilities/ability_defs.h"
#include "../core/types.h"

namespace UnitFactory {

    inline unit make_bosun(int col, int row) {
        unit u(col, row, UnitPresets::Bosun());
        u.add_ability(AbilityDefs::make_shoot());
        u.add_ability(AbilityDefs::make_melee());
        u.add_ability(AbilityDefs::make_rush());
        return u;
    }

    inline unit make_sharpshooter(int col, int row) {
        unit u(col, row, UnitPresets::Sharpshooter());
        u.add_ability(AbilityDefs::make_shoot());
        u.add_ability(AbilityDefs::make_aimed_shot());
        u.add_ability(AbilityDefs::make_overwatch());
        return u;
    }

    inline unit make_medic(int col, int row) {
        unit u(col, row, UnitPresets::Medic());
        u.add_ability(AbilityDefs::make_shoot());
        u.add_ability(AbilityDefs::make_melee());
        u.add_ability(AbilityDefs::make_heal());
        return u;
    }

    inline unit make_swashbuckler(int col, int row) {
        unit u(col, row, UnitPresets::Swashbuckler());
        u.add_ability(AbilityDefs::make_shoot());
        u.add_ability(AbilityDefs::make_melee());
        u.add_ability(AbilityDefs::make_dirty_trick());
        return u;
    }

    inline enemy make_soldier(int col, int row) {
        return enemy(col, row, UnitPresets::Soldier(), EnemyType::SOLDIER);
    }

    inline enemy make_guard(int col, int row) {
        return enemy(col, row, UnitPresets::Guard(), EnemyType::GUARD);
    }

    inline enemy make_captain(int col, int row) {
        return enemy(col, row, UnitPresets::Captain(), EnemyType::CAPTAIN);
    }
}