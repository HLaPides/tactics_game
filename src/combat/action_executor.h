#pragma once
#include "../core/game_state.h"

namespace ActionExecutor {
    void execute_move       (const Intent& intent, GameState& state);
    void execute_shoot      (const Intent& intent, GameState& state);
    void execute_aimed_shot (const Intent& intent, GameState& state);
    void execute_melee      (const Intent& intent, GameState& state);
    void execute_overwatch  (GameState& state);
    void execute_rush       (const Intent& intent, GameState& state);
    void execute_heal       (const Intent& intent, GameState& state);
    void execute_dirty_trick(const Intent& intent, GameState& state);
}