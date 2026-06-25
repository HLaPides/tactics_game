#pragma once
#include "units/unit.h"
#include "raylib.h"
#include "map.h"

struct AttackResult {
    int  hit_chance;
    int  crit_chance;
    int  aim_component;
    int  cover_penalty;
    int  defense_penalty;
    int  flank_bonus;
    int  range_penalty;
    bool is_flanking;
    bool hit;
    bool crit;
    int  damage;
};

struct CoverResult {
    int  penalty;   // 0, 20, or 40
    bool flanked;
};

CoverResult get_cover(unit& attacker, unit& target, map& game_map);
AttackResult resolve_attack(unit& attacker, unit& target, map& game_map, int base_damage);