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
    if (state.players.empty()) return;

    unit& active = state.players[state.selected_player];

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

        default:
            break;
    }

    // re-fetch active since vector may have been modified
    unit& active2 = state.players[state.selected_player];

    if (active2.get_actions() <= 0) {
        state.mode    = ActionMode::NONE;
        state.preview = {};
        start_enemy_turn(state);
    }

    if (!active2.is_alive()) {
        state.mode    = ActionMode::NONE;
        state.preview = {};
        state.phase   = GamePhase::ENEMY_TURN;
        enemy_index   = (int)state.enemies.size();
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
        // AI targets the selected player — could be extended to target nearest
        ai.act(state.enemies[enemy_index], state.players[state.selected_player],
               state.enemies, state.map, state.floating_texts);
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
    // reset actions for all living players
    bool any_alive = false;
    for (auto& p : state.players) {
        if (p.is_alive()) {
            p.reset_actions();
            any_alive = true;
        }
    }
    if (any_alive)
        state.phase = GamePhase::PLAYER_TURN;
}

void TurnManager::apply_move(const Intent& intent, GameState& state) {
    unit& active = state.players[state.selected_player];
    if (active.get_actions() <= 0) return;

    int dist = std::max(abs(intent.target_x - active.get_x_pos()),
                        abs(intent.target_y - active.get_y_pos()));

    bool tile_blocked = false;
    for (auto& e : state.enemies) {
        if (e.is_alive() && e.get_x_pos() == intent.target_x && e.get_y_pos() == intent.target_y) {
            tile_blocked = true;
            break;
        }
    }
    // also block tiles occupied by other players
    for (int i = 0; i < (int)state.players.size(); i++) {
        if (i == state.selected_player) continue;
        if (state.players[i].is_alive() &&
            state.players[i].get_x_pos() == intent.target_x &&
            state.players[i].get_y_pos() == intent.target_y) {
            tile_blocked = true;
            break;
        }
    }

    if (dist <= active.get_movement()
        && !tile_blocked
        && state.map.is_walkable(intent.target_x, intent.target_y)) {
        active.set_position(intent.target_x, intent.target_y);
        active.use_action();
    }
}

void TurnManager::apply_shoot(const Intent& intent, GameState& state) {
    unit& active = state.players[state.selected_player];

    for (int i = 0; i < (int)state.enemies.size(); i++) {
        auto& e = state.enemies[i];
        if (!e.is_alive()) continue;
        if (!state.spotted[i]) continue;
        if (e.get_x_pos() != intent.target_x || e.get_y_pos() != intent.target_y) continue;

        int dist = std::max(abs(intent.target_x - active.get_x_pos()),
                            abs(intent.target_y - active.get_y_pos()));
        if (dist > active.get_shoot_range()) break;
        if (!has_los(active.get_x_pos(), active.get_y_pos(),
                     e.get_x_pos(), e.get_y_pos(), state.map)) break;

        AttackResult result = resolve_attack(active, e, state.map, active.get_shoot_damage());
        e.take_damage(result.damage);
        active.use_action();
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
    unit& active = state.players[state.selected_player];

    for (int i = 0; i < (int)state.enemies.size(); i++) {
        auto& e = state.enemies[i];
        if (!e.is_alive()) continue;
        if (!state.spotted[i]) continue;
        if (e.get_x_pos() != intent.target_x || e.get_y_pos() != intent.target_y) continue;

        int dist = std::max(abs(intent.target_x - active.get_x_pos()),
                            abs(intent.target_y - active.get_y_pos()));
        if (dist > 1) break;

        AttackResult result = resolve_attack(active, e, state.map, active.get_melee_damage());
        e.take_damage(result.damage);
        active.use_action();
        state.mode    = ActionMode::NONE;
        state.preview = {};

        if (!result.hit)
            state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "MISS!", GRAY);
        else if (result.crit)
            state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "CRIT!", YELLOW);
        break;
    }
}