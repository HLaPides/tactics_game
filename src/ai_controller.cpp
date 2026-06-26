#include "ai_controller.h"
#include "raylib.h"
#include <algorithm>

bool AIController::tile_occupied(int x, int y, unit& player, std::vector<enemy>& enemies, enemy& self) {
    if (player.get_x_pos() == x && player.get_y_pos() == y) return true;
    for (auto& e : enemies) {
        if (!e.is_alive()) continue;
        if (&e == &self) continue;
        if (e.get_x_pos() == x && e.get_y_pos() == y) return true;
    }
    return false;
}

void AIController::take_best_action(enemy& e, unit& player, std::vector<enemy>& enemies,
                                    GameMap& game_map, FloatingTextManager& texts) {
    int px   = player.get_x_pos();
    int py   = player.get_y_pos();
    int dist = std::max(abs(e.get_x_pos() - px), abs(e.get_y_pos() - py));

    bool can_see_player = dist <= e.get_sight_range() &&
                          has_los(e.get_x_pos(), e.get_y_pos(), px, py, game_map);

    if (!can_see_player) {
        // no LOS — hold position, do nothing but use the action
        e.use_action();
        return;
    }

    if (dist <= 1) {
        AttackResult result = resolve_attack(e, player, game_map, e.get_melee_damage());
        player.take_damage(result.damage);
        e.use_action();
        if (result.hit && result.crit)
            texts.spawn(player.get_x_pos(), player.get_y_pos(), "CRIT!", YELLOW);
        else if (!result.hit)
            texts.spawn(player.get_x_pos(), player.get_y_pos(), "MISS!", GRAY);
    } else if (dist <= e.get_shoot_range() &&
               has_los(e.get_x_pos(), e.get_y_pos(), px, py, game_map)) {
        AttackResult result = resolve_attack(e, player, game_map, e.get_shoot_damage());
        player.take_damage(result.damage);
        e.use_action();
        if (result.hit && result.crit)
            texts.spawn(player.get_x_pos(), player.get_y_pos(), "CRIT!", YELLOW);
        else if (!result.hit)
            texts.spawn(player.get_x_pos(), player.get_y_pos(), "MISS!", GRAY);
    } else {
        int step_x = e.get_x_pos();
        int step_y = e.get_y_pos();
        if (px > e.get_x_pos()) step_x++;
        else if (px < e.get_x_pos()) step_x--;
        if (py > e.get_y_pos()) step_y++;
        else if (py < e.get_y_pos()) step_y--;

        if (!tile_occupied(step_x, step_y, player, enemies, e) &&
            game_map.is_walkable(step_x, step_y))
            e.set_position(step_x, step_y);
        e.use_action();
    }
}

void AIController::act(enemy& e, unit& player, std::vector<enemy>& enemies,
                       GameMap& game_map, FloatingTextManager& texts) {
    if (!e.is_alive()) return;
    for (int a = 0; a < 2; a++) {
        if (e.get_actions() > 0)
            take_best_action(e, player, enemies, game_map, texts);
    }
    e.reset_actions();
}