#pragma once
#include "units/unit.h"
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
    int  penalty;
    bool flanked;
};

struct AttackPreview {
    bool         active = false;
    unit*        target = nullptr;
    AttackResult result = {};
};

bool         has_los(int x0, int y0, int x1, int y1, GameMap& game_map);
CoverResult  get_cover(unit& attacker, unit& target, GameMap& game_map);
AttackResult calculate_odds(unit& attacker, unit& target, GameMap& game_map, int base_damage);
AttackResult resolve_attack(unit& attacker, unit& target, GameMap& game_map, int base_damage);