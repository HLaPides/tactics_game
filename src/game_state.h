#pragma once
#include "map.h"
#include "units/unit.h"
#include "units/enemy.h"
#include "combat.h"
#include "floating_text.h"
#include "types.h"
#include <vector>

struct GameState {
    GameMap              map;
    unit                 player;
    std::vector<enemy>   enemies;
    std::vector<bool>    spotted;   // spotted[i] mirrors enemies[i]
    GamePhase            phase   = GamePhase::PLAYER_TURN;
    ActionMode           mode    = ActionMode::NONE;
    AttackPreview        preview = {};
    FloatingTextManager  floating_texts;
};