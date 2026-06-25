#include "combat.h"
#include <algorithm>
#include <cstdlib>

CoverResult get_cover(unit& attacker, unit& target, map& game_map) {
    int tx = target.get_x_pos();
    int ty = target.get_y_pos();
    int ax = attacker.get_x_pos();
    int ay = attacker.get_y_pos();

    // check all four neighbours of the target for cover tiles
    int  best_penalty = 0;
    bool flanked      = false;

    // north neighbour (row - 1) — covers shots from the north (attacker_y < target_y)
    Tile north = game_map.get_tile(tx, ty - 1);
    if (north.cover != COVER_NONE && north.faces.north) {
        if (ay < ty) {
            // attacker is to the north — cover applies
            int pen = (north.cover == COVER_FULL) ? 40 : 20;
            if (pen > best_penalty) best_penalty = pen;
        } else if (ay > ty) {
            // attacker is to the south — flanking
            flanked = true;
        }
    }

    // south neighbour (row + 1) — covers shots from the south (attacker_y > target_y)
    Tile south = game_map.get_tile(tx, ty + 1);
    if (south.cover != COVER_NONE && south.faces.south) {
        if (ay > ty) {
            int pen = (south.cover == COVER_FULL) ? 40 : 20;
            if (pen > best_penalty) best_penalty = pen;
        } else if (ay < ty) {
            flanked = true;
        }
    }

    // west neighbour (col - 1) — covers shots from the west (attacker_x < target_x)
    Tile west = game_map.get_tile(tx - 1, ty);
    if (west.cover != COVER_NONE && west.faces.west) {
        if (ax < tx) {
            int pen = (west.cover == COVER_FULL) ? 40 : 20;
            if (pen > best_penalty) best_penalty = pen;
        } else if (ax > tx) {
            flanked = true;
        }
    }

    // east neighbour (col + 1) — covers shots from the east (attacker_x > target_x)
    Tile east = game_map.get_tile(tx + 1, ty);
    if (east.cover != COVER_NONE && east.faces.east) {
        if (ax > tx) {
            int pen = (east.cover == COVER_FULL) ? 40 : 20;
            if (pen > best_penalty) best_penalty = pen;
        } else if (ax < tx) {
            flanked = true;
        }
    }

    // flanking negates cover
    if (flanked) best_penalty = 0;

    return { best_penalty, flanked };
}

AttackResult resolve_attack(unit& attacker, unit& target, map& game_map, int base_damage) {
    AttackResult result = {};

    // range penalty
    int dist = std::max(abs(attacker.get_x_pos() - target.get_x_pos()),
                        abs(attacker.get_y_pos() - target.get_y_pos()));
    int half_range     = attacker.get_shoot_range() / 2;
    result.range_penalty = (dist > half_range) ? 10 : 0;

    // cover and flank
    CoverResult cover      = get_cover(attacker, target, game_map);
    result.cover_penalty   = cover.penalty;
    result.is_flanking     = cover.flanked;
    result.flank_bonus     = cover.flanked ? 30 : 0;

    // components
    result.aim_component     = attacker.get_aim();
    result.defense_penalty   = target.get_defense();

    // final hit chance clamped 1-99
    result.hit_chance = result.aim_component
                      + result.flank_bonus
                      - result.range_penalty
                      - result.defense_penalty
                      - result.cover_penalty;
    result.hit_chance = std::max(1, std::min(99, result.hit_chance));

    // crit chance
    result.crit_chance = cover.flanked ? 15 : 5;

    // roll
    int hit_roll  = GetRandomValue(1, 100);
    int crit_roll = GetRandomValue(1, 100);
    result.hit    = hit_roll <= result.hit_chance;
    result.crit   = result.hit && (crit_roll <= result.crit_chance);
    result.damage = result.hit ? (result.crit ? base_damage * 2 : base_damage) : 0;

    return result;
}