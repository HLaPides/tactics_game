#pragma once
#include "game_state.h"
#include "ai_controller.h"
#include "types.h"

class TurnManager {
public:
    TurnManager();
    void apply_player_intent(const Intent& intent, GameState& state);
    void update_enemy_turn(float dt, GameState& state, AIController& ai);
private:
    int   enemy_index;
    float enemy_timer;

    static constexpr float ENEMY_DELAY = 0.4f;

    void start_enemy_turn(GameState& state);
    void end_enemy_turn(GameState& state);
    void check_overwatch(GameState& state, const enemy& acting_enemy);

    void apply_move(const Intent& intent, GameState& state);
    void apply_shoot(const Intent& intent, GameState& state);
    void apply_aimed_shot(const Intent& intent, GameState& state);
    void apply_overwatch(GameState& state);
    void apply_melee(const Intent& intent, GameState& state);
    void apply_rush(const Intent& intent, GameState& state);
    void apply_heal(const Intent& intent, GameState& state);
    void apply_dirty_trick(const Intent& intent, GameState& state);
};