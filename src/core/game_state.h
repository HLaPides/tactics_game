#pragma once
#include "../map/map.h"
#include "../units/unit.h"
#include "../units/enemy.h"
#include "../combat/combat.h"
#include "../ui/floating_text.h"
#include "types.h"
#include <vector>
#include <string>

enum class WinState { ONGOING, VICTORY, DEFEAT };

struct GameState {
    GameMap                  map;
    std::vector<unit>        players;
    std::vector<std::string> player_names;
    std::vector<bool>        from_roster;
    int                      selected_player = 0;
    std::vector<enemy>       enemies;
    std::vector<bool>        spotted;
    GamePhase                phase        = GamePhase::PLAYER_TURN;
    ActionMode               mode         = ActionMode::NONE;
    AttackPreview            preview      = {};
    FloatingTextManager      floating_texts;
    WinState                 win_state    = WinState::ONGOING;
    int                      target_index = -1;
    int                      turn_count   = 0;
};