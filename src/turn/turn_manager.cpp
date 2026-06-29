#include "turn_manager.h"
#include "combat/combat.h"
#include "units/enemy.h"
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
            state.mode    = ActionMode::NONE;
            state.preview = {};
            break;

        case IntentType::EndTurn:
            state.mode    = ActionMode::NONE;
            state.preview = {};
            start_enemy_turn(state);
            return;

        case IntentType::Move:
            apply_move(intent, state);
            break;

        case IntentType::Shoot:
            apply_shoot(intent, state);
            break;

        case IntentType::Melee:
            apply_melee(intent, state);
            break;

        case IntentType::AimedShot:
            apply_aimed_shot(intent, state);
            break;

        case IntentType::Overwatch:
            apply_overwatch(state);
            break;

        case IntentType::Rush:
            apply_rush(intent, state);
            break;

        case IntentType::Heal:
            apply_heal(intent, state);
            break;

        case IntentType::DirtyTrick:
            apply_dirty_trick(intent, state);
            break;

        default:
            break;
    }

    bool any_actions_left = false;
    for (auto& p : state.players) {
        if (p.is_alive() && p.get_actions() > 0) {
            any_actions_left = true;
            break;
        }
    }
    if (!any_actions_left) {
        state.mode    = ActionMode::NONE;
        state.preview = {};
        start_enemy_turn(state);
        return;
    }

    unit& active = state.players[state.selected_player];
    if (active.get_actions() <= 0) {
        for (int i = 1; i <= (int)state.players.size(); i++) {
            int idx = (state.selected_player + i) % (int)state.players.size();
            if (state.players[idx].is_alive() && state.players[idx].get_actions() > 0) {
                state.selected_player = idx;
                state.mode    = ActionMode::NONE;
                state.preview = {};
                break;
            }
        }
    }

    if (!state.players[state.selected_player].is_alive()) {
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

        if (target != nullptr)
            ai.act(acting, *target, state.enemies, state.map, state.floating_texts);

        // tick aim penalty after enemy acts
        acting.tick_aim_penalty();

        check_overwatch(state, acting);

        enemy_index++;
    } else {
        end_enemy_turn(state);
    }
}

void TurnManager::check_overwatch(GameState& state, const enemy& acting_enemy) {
    for (auto& p : state.players) {
        if (!p.is_alive()) continue;
        if (!p.is_on_overwatch()) continue;

        int dist = std::max(abs(p.get_x_pos() - acting_enemy.get_x_pos()),
                            abs(p.get_y_pos() - acting_enemy.get_y_pos()));
        if (dist > p.get_sight_range()) continue;
        if (!has_los(p.get_x_pos(), p.get_y_pos(),
                     acting_enemy.get_x_pos(), acting_enemy.get_y_pos(), state.map)) continue;

        for (auto& e : state.enemies) {
            if (!e.is_alive()) continue;
            if (e.get_x_pos() != acting_enemy.get_x_pos() ||
                e.get_y_pos() != acting_enemy.get_y_pos()) continue;

            AttackResult result = resolve_attack(
                const_cast<unit&>(p), e, state.map, p.get_shoot_damage());
            e.take_damage(result.damage);

            if (!result.hit)
                state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "MISS!", GRAY);
            else if (result.crit)
                state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "CRIT!", YELLOW);
            else
                state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "OW!", SKYBLUE);

            const_cast<unit&>(p).clear_overwatch();
            break;
        }
        break;
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
    if (any_alive)
        state.phase = GamePhase::PLAYER_TURN;
}

void TurnManager::apply_move(const Intent& intent, GameState& state) {
    unit& active = state.players[state.selected_player];
    if (active.get_actions() <= 0) return;

    int tx = intent.target_x;
    int ty = intent.target_y;

    if (tx < 0 || ty < 0 || tx >= state.map.getCols() || ty >= state.map.getRows()) return;

    std::vector<std::pair<int,int>> blocked;
    for (auto& e : state.enemies)
        if (e.is_alive()) blocked.push_back({e.get_x_pos(), e.get_y_pos()});
    for (int i = 0; i < (int)state.players.size(); i++) {
        if (i == state.selected_player) continue;
        if (state.players[i].is_alive())
            blocked.push_back({state.players[i].get_x_pos(), state.players[i].get_y_pos()});
    }

    auto reachable = get_reachable_tiles(
        active.get_x_pos(), active.get_y_pos(),
        active.get_movement(), state.map, blocked);

    bool can_reach = false;
    for (auto& [col, row] : reachable) {
        if (col == tx && row == ty) { can_reach = true; break; }
    }

    if (can_reach) {
        active.set_position(tx, ty);
        active.use_action();
    }
}

void TurnManager::apply_rush(const Intent& intent, GameState& state) {
    unit& active = state.players[state.selected_player];
    Ability* rush = active.get_ability("rush");
    if (!rush || !rush->is_ready()) return;

    int tx = intent.target_x;
    int ty = intent.target_y;

    if (tx < 0 || ty < 0 || tx >= state.map.getCols() || ty >= state.map.getRows()) return;

    std::vector<std::pair<int,int>> blocked;
    for (auto& e : state.enemies)
        if (e.is_alive()) blocked.push_back({e.get_x_pos(), e.get_y_pos()});
    for (int i = 0; i < (int)state.players.size(); i++) {
        if (i == state.selected_player) continue;
        if (state.players[i].is_alive())
            blocked.push_back({state.players[i].get_x_pos(), state.players[i].get_y_pos()});
    }

    auto reachable = get_reachable_tiles(
        active.get_x_pos(), active.get_y_pos(),
        active.get_movement(), state.map, blocked);

    bool can_reach = false;
    for (auto& [col, row] : reachable) {
        if (col == tx && row == ty) { can_reach = true; break; }
    }

    if (can_reach) {
        active.set_position(tx, ty);
        rush->use();
        state.mode    = ActionMode::NONE;
        state.preview = {};
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
        if (dist > active.get_sight_range()) break;
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

void TurnManager::apply_aimed_shot(const Intent& intent, GameState& state) {
    unit& active = state.players[state.selected_player];
    if (active.get_actions() < 2) return;

    for (int i = 0; i < (int)state.enemies.size(); i++) {
        auto& e = state.enemies[i];
        if (!e.is_alive()) continue;
        if (!state.spotted[i]) continue;
        if (e.get_x_pos() != intent.target_x || e.get_y_pos() != intent.target_y) continue;

        int dist = std::max(abs(intent.target_x - active.get_x_pos()),
                            abs(intent.target_y - active.get_y_pos()));
        if (dist > active.get_sight_range()) break;
        if (!has_los(active.get_x_pos(), active.get_y_pos(),
                     e.get_x_pos(), e.get_y_pos(), state.map)) break;

        AttackResult result = calculate_odds(active, e, state.map, active.get_shoot_damage());
        result.crit_chance  = result.is_flanking ? 35 : 20;

        int hit_roll  = GetRandomValue(1, 100);
        int crit_roll = GetRandomValue(1, 100);
        result.hit    = hit_roll  <= result.hit_chance;
        result.crit   = result.hit && (crit_roll <= result.crit_chance);
        result.damage = result.hit ? (result.crit ? active.get_shoot_damage() * 2
                                                  : active.get_shoot_damage()) : 0;

        e.take_damage(result.damage);
        active.use_actions(2);
        state.mode    = ActionMode::NONE;
        state.preview = {};

        if (!result.hit)
            state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "MISS!", GRAY);
        else if (result.crit)
            state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "CRIT!", YELLOW);
        break;
    }
}

void TurnManager::apply_overwatch(GameState& state) {
    unit& active = state.players[state.selected_player];
    if (active.get_actions() <= 0) return;

    active.set_overwatch(true);
    while (active.get_actions() > 0) active.use_action();
    state.mode    = ActionMode::NONE;
    state.preview = {};
    state.floating_texts.spawn(active.get_x_pos(), active.get_y_pos(), "OVERWATCH", SKYBLUE);
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

void TurnManager::apply_heal(const Intent& intent, GameState& state) {
    unit& active = state.players[state.selected_player];
    Ability* heal = active.get_ability("heal");
    if (!heal || !heal->is_ready()) return;
    if (active.get_actions() <= 0) return;

    for (auto& p : state.players) {
        if (!p.is_alive()) continue;
        if (p.get_x_pos() != intent.target_x || p.get_y_pos() != intent.target_y) continue;

        int dist = std::max(abs(intent.target_x - active.get_x_pos()),
                            abs(intent.target_y - active.get_y_pos()));
        if (dist > 1) break;  // dist 0 = self, dist 1 = adjacent

        p.heal(2);
        active.use_action();
        heal->use();
        state.mode         = ActionMode::NONE;
        state.preview      = {};
        state.target_index = -1;
        state.floating_texts.spawn(p.get_x_pos(), p.get_y_pos(), "+2 HP", GREEN);
        break;
    }
}

void TurnManager::apply_dirty_trick(const Intent& intent, GameState& state) {
    unit& active = state.players[state.selected_player];
    Ability* trick = active.get_ability("dirty_trick");
    if (!trick || !trick->is_ready()) return;
    if (active.get_actions() <= 0) return;

    for (int i = 0; i < (int)state.enemies.size(); i++) {
        auto& e = state.enemies[i];
        if (!e.is_alive()) continue;
        if (!state.spotted[i]) continue;
        if (e.get_x_pos() != intent.target_x || e.get_y_pos() != intent.target_y) continue;

        int dist = std::max(abs(intent.target_x - active.get_x_pos()),
                            abs(intent.target_y - active.get_y_pos()));
        if (dist > active.get_sight_range()) break;
        if (!has_los(active.get_x_pos(), active.get_y_pos(),
                     e.get_x_pos(), e.get_y_pos(), state.map)) break;

        e.apply_aim_penalty(20, 1);
        active.use_action();
        trick->use();
        state.mode    = ActionMode::NONE;
        state.preview = {};
        state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "TRICKED!", PURPLE);
        break;
    }
}