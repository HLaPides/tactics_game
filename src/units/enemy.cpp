#include "enemy.h"
#include "combat.h"
#include "raylib.h"
#include <algorithm>

enemy::enemy(int x_pos, int y_pos, int mvmt, int hp, int aim, int defense, int shoot_range, int shoot_dmg, int melee_dmg)
    : unit(x_pos, y_pos, mvmt, hp, aim, defense, shoot_range, shoot_dmg, melee_dmg) {}

bool enemy::tile_occupied(int x, int y, unit& player, std::vector<enemy>& enemies) {
    if (player.get_x_pos() == x && player.get_y_pos() == y) return true;
    for (auto& e : enemies) {
        if (!e.is_alive()) continue;
        if (&e == this) continue;
        if (e.get_x_pos() == x && e.get_y_pos() == y) return true;
    }
    return false;
}

void enemy::take_best_action(unit& player, std::vector<enemy>& enemies, map& game_map) {
    int px   = player.get_x_pos();
    int py   = player.get_y_pos();
    int dist = std::max(abs(get_x_pos() - px), abs(get_y_pos() - py));

    if (dist <= 1) {
        // melee — run through resolve_attack
        AttackResult result = resolve_attack(*this, player, game_map, get_melee_damage());
        player.take_damage(result.damage);
        use_action();
    } else if (dist <= get_shoot_range()) {
        // shoot — run through resolve_attack
        AttackResult result = resolve_attack(*this, player, game_map, get_shoot_damage());
        player.take_damage(result.damage);
        use_action();
    } else {
        // move toward player, respecting walkability and occupancy
        int step_x = get_x_pos();
        int step_y = get_y_pos();
        if (px > get_x_pos()) step_x++;
        else if (px < get_x_pos()) step_x--;
        if (py > get_y_pos()) step_y++;
        else if (py < get_y_pos()) step_y--;

        if (!tile_occupied(step_x, step_y, player, enemies) && game_map.is_walkable(step_x, step_y)) {
            set_position(step_x, step_y);
        }
        use_action();
    }
}

void enemy::act(unit& player, std::vector<enemy>& enemies, map& game_map) {
    if (!is_alive()) return;
    take_best_action(player, enemies, game_map);
    if (get_actions() > 0)
        take_best_action(player, enemies, game_map);
    reset_actions();
}

void enemy::draw(int tile_size) {
    if (!is_alive()) return;
    DrawRectangle(get_x_pos() * tile_size, get_y_pos() * tile_size, tile_size, tile_size, BLUE);
}

void enemy::draw_hp(int tile_size) {
    if (!is_alive()) return;
    int x = get_x_pos() * tile_size;
    int y = get_y_pos() * tile_size;
    for (int i = 0; i < get_max_hp(); i++) {
        Color pip_color = i < get_hp() ? GREEN : DARKGRAY;
        DrawRectangle(x + 4 + i * 10, y - 10, 8, 6, pip_color);
    }
}