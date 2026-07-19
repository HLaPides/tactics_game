#include "turn_manager.h"
#include "../combat/action_executor.h"
#include "../combat/combat.h"
#include "../units/enemy.h"
#include "raylib.h"
#include <algorithm>
#include <limits>

TurnManager::TurnManager() {
    enemy_index = 0;
    enemy_timer = 0.0f;
}

void TurnManager::apply_player_intent(const Intent& intent, GameState& state) {
    if (state.phase != GamePhase::PLAYER_TURN) return;
    if (state.players.empty()) return;

    switch (intent.type) {
        case IntentType::Cancel:
            state.mode         = ActionMode::NONE;
            state.preview      = {};
            state.target_index = -1;
            break;
        case IntentType::EndTurn:
            state.mode         = ActionMode::NONE;
            state.preview      = {};
            state.target_index = -1;
            start_enemy_turn(state);
            return;
        case IntentType::Move:       ActionExecutor::execute_move(intent, state);        break;
        case IntentType::Shoot:      ActionExecutor::execute_shoot(intent, state);       break;
        case IntentType::Melee:      ActionExecutor::execute_melee(intent, state);       break;
        case IntentType::AimedShot:  ActionExecutor::execute_aimed_shot(intent, state);  break;
        case IntentType::Rush:       ActionExecutor::execute_rush(intent, state);        break;
        case IntentType::Heal:       ActionExecutor::execute_heal(intent, state);        break;
        case IntentType::DirtyTrick: ActionExecutor::execute_dirty_trick(intent, state); break;
        default: break;
    }

    bool any_actions_left = false;
    for (auto& p : state.players) {
        if (p.is_alive() && p.get_actions() > 0) {
            any_actions_left = true;
            break;
        }
    }
    if (!any_actions_left) {
        state.mode         = ActionMode::NONE;
        state.preview      = {};
        state.target_index = -1;
        start_enemy_turn(state);
        return;
    }

    unit& active = state.players[state.selected_player];
    if (active.get_actions() <= 0) {
        for (int i = 1; i <= (int)state.players.size(); i++) {
            int idx = (state.selected_player + i) % (int)state.players.size();
            if (state.players[idx].is_alive() && state.players[idx].get_actions() > 0) {
                state.selected_player = idx;
                state.mode            = ActionMode::NONE;
                state.preview         = {};
                state.target_index    = -1;
                break;
            }
        }
    }

    if (!state.players[state.selected_player].is_alive()) {
        state.mode         = ActionMode::NONE;
        state.preview      = {};
        state.target_index = -1;
        state.phase        = GamePhase::ENEMY_TURN;
        enemy_index        = (int)state.enemies.size();
    }
}

void TurnManager::update_enemy_turn(float dt, GameState& state, AIController& ai) {
    if (state.phase != GamePhase::ENEMY_TURN) return;
    if (state.players.empty()) return;

    enemy_timer += dt;
    if (enemy_timer < ENEMY_DELAY) return;
    enemy_timer = 0.0f;

    while (enemy_index < (int)state.enemies.size() && !state.enemies[enemy_index].is_alive())
        enemy_index++;

    if (enemy_index < (int)state.enemies.size()) {
        enemy& acting    = state.enemies[enemy_index];
        unit*  target    = nullptr;
        int    best_dist = std::numeric_limits<int>::max();

        for (auto& p : state.players) {
            if (!p.is_alive()) continue;
            int dist = std::max(abs(acting.get_x_pos() - p.get_x_pos()),
                                abs(acting.get_y_pos() - p.get_y_pos()));
            if (dist < best_dist) {
                best_dist = dist;
                target    = &p;
            }
        }

        if (target != nullptr && state.spotted[enemy_index]) {
            ai.act(acting, *target, state.enemies, state.map, state.floating_texts);
        }

        acting.tick_aim_penalty();
        enemy_index++;
    } else {
        end_enemy_turn(state);
    }
}

void TurnManager::start_enemy_turn(GameState& state) {
    state.phase = GamePhase::ENEMY_TURN;
    enemy_index = 0;
    enemy_timer = 0.0f;
}

void TurnManager::end_enemy_turn(GameState& state) {
    bool any_alive = false;
    for (auto& p : state.players) {
        if (p.is_alive()) {
            p.reset_actions();
            p.tick_cooldowns();
            any_alive = true;
        }
    }
    if (any_alive) {
        state.turn_count++;
        state.phase = GamePhase::PLAYER_TURN;
    }
}