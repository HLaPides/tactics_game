#pragma once
#include "map/map.h"
#include "units/unit.h"
#include "units/enemy.h"
#include "combat/combat.h"
#include "ui/floating_text.h"
#include "core/types.h"
#include "raylib.h"
#include <vector>
#include <string>

enum class WinState { ONGOING, VICTORY, DEFEAT };

struct GameState {
    GameMap                  map;
    std::vector<unit>        players;
    std::vector<std::string> player_names;
    int                      selected_player = 0;
    std::vector<enemy>       enemies;
    std::vector<bool>        spotted;
    GamePhase                phase        = GamePhase::PLAYER_TURN;
    ActionMode               mode         = ActionMode::NONE;
    AttackPreview            preview      = {};
    FloatingTextManager      floating_texts;
    WinState                 win_state    = WinState::ONGOING;
    Camera2D                 camera       = { {0,0}, {0,0}, 0.0f, 1.0f };
    int                      target_index = -1;  // current enemy target, -1 = none
    int                      turn_count   = 0;
};