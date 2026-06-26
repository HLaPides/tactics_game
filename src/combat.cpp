#include "combat.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>

bool has_los(int x0, int y0, int x1, int y1, GameMap& game_map) {
    // Bresenham's line algorithm
    int dx  =  abs(x1 - x0);
    int dy  = -abs(y1 - y0);
    int sx  = x0 < x1 ? 1 : -1;
    int sy  = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    int x = x0;
    int y = y0;

    while (true) {
        // reached target
        if (x == x1 && y == y1) return true;

        // check if this tile blocks LOS — skip start tile
        if (!(x == x0 && y == y0)) {
            Tile t = game_map.get_tile(x, y);
            if (t.type == TILE_WALL) return false;
        }

        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

CoverResult get_cover(unit& attacker, unit& target, GameMap& game_map) {
    int tx = target.get_x_pos();
    int ty = target.get_y_pos();
    int ax = attacker.get_x_pos();
    int ay = attacker.get_y_pos();

    int  best_penalty = 0;
    bool flanked      = false;

    Tile north = game_map.get_tile(tx, ty - 1);
    if (north.cover != COVER_NONE && north.faces.south) {
        if (ay < ty) {
            int pen = (north.cover == COVER_FULL) ? 40 : 20;
            if (pen > best_penalty) best_penalty = pen;
        } else if (ay > ty) {
            flanked = true;
        }
    }

    Tile south = game_map.get_tile(tx, ty + 1);
    if (south.cover != COVER_NONE && south.faces.north) {
        if (ay > ty) {
            int pen = (south.cover == COVER_FULL) ? 40 : 20;
            if (pen > best_penalty) best_penalty = pen;
        } else if (ay < ty) {
            flanked = true;
        }
    }

    Tile west = game_map.get_tile(tx - 1, ty);
    if (west.cover != COVER_NONE && west.faces.east) {
        if (ax < tx) {
            int pen = (west.cover == COVER_FULL) ? 40 : 20;
            if (pen > best_penalty) best_penalty = pen;
        } else if (ax > tx) {
            flanked = true;
        }
    }

    Tile east = game_map.get_tile(tx + 1, ty);
    if (east.cover != COVER_NONE && east.faces.west) {
        if (ax > tx) {
            int pen = (east.cover == COVER_FULL) ? 40 : 20;
            if (pen > best_penalty) best_penalty = pen;
        } else if (ax < tx) {
            flanked = true;
        }
    }

    if (flanked) best_penalty = 0;
    return { best_penalty, flanked };
}

AttackResult calculate_odds(unit& attacker, unit& target, GameMap& game_map, int base_damage) {
    AttackResult result = {};

    int dist       = std::max(abs(attacker.get_x_pos() - target.get_x_pos()),
                              abs(attacker.get_y_pos() - target.get_y_pos()));
    int half_range = attacker.get_shoot_range() / 2;
    result.range_penalty = (dist > half_range) ? 10 : 0;

    CoverResult cover    = get_cover(attacker, target, game_map);
    result.cover_penalty = cover.penalty;
    result.is_flanking   = cover.flanked;
    result.flank_bonus   = cover.flanked ? 30 : 0;

    result.aim_component   = attacker.get_aim();
    result.defense_penalty = target.get_defense();

    result.hit_chance = result.aim_component
                      + result.flank_bonus
                      - result.range_penalty
                      - result.defense_penalty
                      - result.cover_penalty;
    result.hit_chance = std::max(1, std::min(99, result.hit_chance));
    result.crit_chance = cover.flanked ? 15 : 5;

    return result;
}

AttackResult resolve_attack(unit& attacker, unit& target, GameMap& game_map, int base_damage) {
    AttackResult result = calculate_odds(attacker, target, game_map, base_damage);

    int hit_roll  = GetRandomValue(1, 100);
    int crit_roll = GetRandomValue(1, 100);
    result.hit    = hit_roll  <= result.hit_chance;
    result.crit   = result.hit && (crit_roll <= result.crit_chance);
    result.damage = result.hit ? (result.crit ? base_damage * 2 : base_damage) : 0;

    return result;
}