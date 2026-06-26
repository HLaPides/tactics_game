#pragma once
#include "map.h"
#include "units/unit.h"
#include "units/enemy.h"
#include "combat.h"
#include "floating_text.h"
#include "types.h"
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
    GamePhase                phase      = GamePhase::PLAYER_TURN;
    ActionMode               mode       = ActionMode::NONE;
    AttackPreview            preview    = {};
    FloatingTextManager      floating_texts;
    WinState                 win_state  = WinState::ONGOING;
    Camera2D                 camera     = { {0,0}, {0,0}, 0.0f, 1.0f };
};