#include "action_executor.h"
#include "combat.h"
#include "../abilities/ability.h"
#include "raylib.h"
#include <algorithm>

namespace ActionExecutor {

// helper — spawn death text if enemy just died from an attack
static void spawn_attack_texts(enemy& e, const AttackResult& result, GameState& state) {
    if (!result.hit) {
        state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "MISS!", GRAY);
    } else if (!e.is_alive()) {
        state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "DEAD!", RED);
    } else if (result.crit) {
        state.floating_texts.spawn(e.get_x_pos(), e.get_y_pos(), "CRIT!", YELLOW);
    }
}

void execute_move(const Intent& intent, GameState& state) {
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

    for (auto& [col, row] : reachable) {
        if (col == tx && row == ty) {
            active.set_position(tx, ty);
            active.use_action();
            break;
        }
    }
}

void execute_rush(const Intent& intent, GameState& state) {
    unit&    active = state.players[state.selected_player];
    Ability* rush   = active.get_ability("rush");
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

    for (auto& [col, row] : reachable) {
        if (col == tx && row == ty) {
            active.set_position(tx, ty);
            rush->use();
            state.mode    = ActionMode::NONE;
            state.preview = {};
            break;
        }
    }
}

void execute_shoot(const Intent& intent, GameState& state) {
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
        spawn_attack_texts(e, result, state);
        break;
    }
}

void execute_aimed_shot(const Intent& intent, GameState& state) {
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
        spawn_attack_texts(e, result, state);
        break;
    }
}

void execute_overwatch(GameState& state) {
    unit& active = state.players[state.selected_player];
    if (active.get_actions() <= 0) return;

    active.set_overwatch(true);
    while (active.get_actions() > 0) active.use_action();
    state.mode    = ActionMode::NONE;
    state.preview = {};
    state.floating_texts.spawn(active.get_x_pos(), active.get_y_pos(), "OVERWATCH", SKYBLUE);
}

void execute_melee(const Intent& intent, GameState& state) {
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
        spawn_attack_texts(e, result, state);
        break;
    }
}

void execute_heal(const Intent& intent, GameState& state) {
    unit&    active = state.players[state.selected_player];
    Ability* heal   = active.get_ability("heal");
    if (!heal || !heal->is_ready()) return;
    if (active.get_actions() <= 0) return;

    for (auto& p : state.players) {
        if (!p.is_alive()) continue;
        if (p.get_x_pos() != intent.target_x || p.get_y_pos() != intent.target_y) continue;

        int dist = std::max(abs(intent.target_x - active.get_x_pos()),
                            abs(intent.target_y - active.get_y_pos()));
        if (dist > 1) break;

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

void execute_dirty_trick(const Intent& intent, GameState& state) {
    unit&    active = state.players[state.selected_player];
    Ability* trick  = active.get_ability("dirty_trick");
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

}