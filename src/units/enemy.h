#pragma once
#include "unit.h"
#include "map.h"
#include <vector>

class enemy : public unit {
public:
    enemy(int x_pos, int y_pos, int mvmt, int hp, int aim, int defense, int shoot_range, int shoot_dmg, int melee_dmg);
    void act(unit& player, std::vector<enemy>& enemies, map& game_map);
    void draw(int tile_size);
    void draw_hp(int tile_size);
private:
    bool tile_occupied(int x, int y, unit& player, std::vector<enemy>& enemies);
    void take_best_action(unit& player, std::vector<enemy>& enemies, map& game_map);
};