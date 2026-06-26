#include "turn_manager.h"
#include "combat.h"
#include "raylib.h"
#include <algorithm>

TurnManager::TurnManager() {
    enemy_index = 0;
    enemy_timer = 0.0f;
}

void TurnManager::apply_player_intent(const Intent& intent, GameState& state) {
    if (state.phase != GamePhase::PLAYER_TURN) return;

    switch (intent.type) {
        case IntentType::Cancel:
            state.mode    = ActionMode::NONE;
            state.preview = {};
            break;

        case IntentType::EndTurn:
            state.mode    = ActionMode::NONE;
            state.preview = {};
            start_enemy_turn(state);
            break;

        case IntentType::Move:
            apply_move(intent, state);
            break;

        case IntentType::Shoot:
            apply_shoot(intent, state);
            break;

        case IntentType::Melee:
            apply_melee(intent, state);
            break;
    }

    if (state.player.get_actions() <= 0) {
        state.mode    = ActionMode::NONE;
        state.preview = {};
        start_enemy_turn(state);
    }

    if (!state.player.is_alive()) {
        state.mode    = ActionMode::NONE;
        state.preview = {};
        state.phase   = GamePhase::ENEMY_TURN;
        enemy_index   = (int)state.enemies.size();
    }
}

void TurnManager::update_enemy_turn(float dt, GameState& state, AIController& ai) {
    if (state.phase != GamePhase::ENEMY_TURN) return;

    enemy_timer += dt;
    if (enemy_timer < ENEMY_DELAY) return;
    enemy_timer = 0.0f;

    while (enemy_index < (int)state.enemies.size() && !state.enemies[enemy_index].is_alive())
        enemy_index++;

    if (enemy_index < (int)state.enemies.size()) {
        ai.act(state.enemies[enemy_index], state.player, state.enemies,
               state.map, state.floating_texts);
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
    if (state.player.is_alive()) {
        state.player.reset_actions();
        state.phase = GamePhase::PLAYER_TURN;
    }
}

void TurnManager::apply_move(const Intent& intent, GameState& state) {
    if (state.player.get_actions() <= 0) return;

    int dist = std::max(abs(intent.target_x - state.player.get_x_pos()),
                        abs(intent.target_y - state.player.get_y_pos()));

    bool tile_blocked = false;
    for (auto& e : state.enemies) {
        if (e.is_alive() && e.get_x_pos() == intent.target_x && e.get_y_pos() == intent.target_y) {
            tile_blocked = true;
            break;
        }
    }

    if (dist <= state.player.get_movement()
        && !tile_blocked
        && state.map.is_walkable(intent.target_x, intent.target_y)) {
        state.player.set_position(intent.target_x, intent.target_y);
        state.player.use_action();
    }
}

void TurnManager::apply_shoot(const Intent& intent, GameState& state) {
    for (int i = 0; i < (int)state.enemies.size(); i++) {
        auto& e = state.enemies[i];
        if (!e.is_alive()) continue;
        if (!state.spotted[i]) continue;
        if (e.get_x_pos() != intent.target_x || e.get_y_pos() != intent.target_y) continue;

        int dist = std::max(abs(intent.target_x - state.player.get_x_pos()),
                            abs(intent.target_y - state.player.get_y_pos()));
        if (dist > state.player.get_shoot_range()) break;
        if (!has_los(state.player.get_x_pos(), state.player.get_y_pos(),
                     e.get_x_pos(), e.get_y_pos(), state.map)) break;

        AttackResult result = resolve_attack(state.player, e, state.map,
                                             state.player.get_shoot_damage());
        e.take_damage(result.damage);
        state.player.use_action();
        state.mode    = ActionMode::NONE;
        state.preview = {};

        if (!result.hit)
            state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "MISS!", GRAY);
        else if (result.crit)
            state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "CRIT!", YELLOW);
        break;
    }
}

void TurnManager::apply_melee(const Intent& intent, GameState& state) {
    for (int i = 0; i < (int)state.enemies.size(); i++) {
        auto& e = state.enemies[i];
        if (!e.is_alive()) continue;
        if (!state.spotted[i]) continue;
        if (e.get_x_pos() != intent.target_x || e.get_y_pos() != intent.target_y) continue;

        int dist = std::max(abs(intent.target_x - state.player.get_x_pos()),
                            abs(intent.target_y - state.player.get_y_pos()));
        if (dist > 1) break;

        AttackResult result = resolve_attack(state.player, e, state.map,
                                             state.player.get_melee_damage());
        e.take_damage(result.damage);
        state.player.use_action();
        state.mode    = ActionMode::NONE;
        state.preview = {};

        if (!result.hit)
            state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "MISS!", GRAY);
        else if (result.crit)
            state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "CRIT!", YELLOW);
        break;
    }
}