#include "enemy.h"
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

void enemy::act(unit& player, std::vector<enemy>& enemies) {
    if (!is_alive()) return;

    int px   = player.get_x_pos();
    int py   = player.get_y_pos();
    int dist = std::max(abs(get_x_pos() - px), abs(get_y_pos() - py));

    // action 1 — melee if adjacent, shoot if in range, else move
    if (dist <= 1) {
        player.take_damage(get_melee_damage());
        use_action();
    } else if (dist <= get_shoot_range()) {
        player.take_damage(get_shoot_damage());
        use_action();
    } else {
        int step_x = get_x_pos();
        int step_y = get_y_pos();
        if (px > get_x_pos()) step_x++;
        else if (px < get_x_pos()) step_x--;
        if (py > get_y_pos()) step_y++;
        else if (py < get_y_pos()) step_y--;

        if (!tile_occupied(step_x, step_y, player, enemies)) {
            set_position(step_x, step_y);
        }
        use_action();
    }

    // action 2
    dist = std::max(abs(get_x_pos() - px), abs(get_y_pos() - py));
    if (get_actions() > 0) {
        if (dist <= 1) {
            player.take_damage(get_melee_damage());
            use_action();
        } else if (dist <= get_shoot_range()) {
            player.take_damage(get_shoot_damage());
            use_action();
        } else {
            int step_x = get_x_pos();
            int step_y = get_y_pos();
            if (px > get_x_pos()) step_x++;
            else if (px < get_x_pos()) step_x--;
            if (py > get_y_pos()) step_y++;
            else if (py < get_y_pos()) step_y--;

            if (!tile_occupied(step_x, step_y, player, enemies)) {
                set_position(step_x, step_y);
            }
            use_action();
        }
    }

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