#pragma once
#include "units/unit.h"
#include "units/enemy.h"
#include "map.h"
#include "combat.h"
#include "floating_text.h"
#include <vector>

class AIController {
public:
    // returns attack result if an attack was made, empty otherwise
    void act(enemy& e, unit& player, std::vector<enemy>& enemies,
             GameMap& game_map, FloatingTextManager& texts);
private:
    void take_best_action(enemy& e, unit& player, std::vector<enemy>& enemies,
                          GameMap& game_map, FloatingTextManager& texts);
    bool tile_occupied(int x, int y, unit& player, std::vector<enemy>& enemies, enemy& self);
};