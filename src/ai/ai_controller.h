#pragma once
#include "units/unit.h"
#include "units/enemy.h"
#include "map/map.h"
#include "combat/combat.h"
#include "ui/floating_text.h"
#include <vector>
#include <optional>

class AIController {
public:
    void act(enemy& e, unit& player, std::vector<enemy>& enemies,
             const GameMap& game_map, FloatingTextManager& texts);
private:
    void act_soldier(enemy& e, unit& player, std::vector<enemy>& enemies,
                     const GameMap& game_map, FloatingTextManager& texts);
    void act_guard(enemy& e, unit& player, std::vector<enemy>& enemies,
                   const GameMap& game_map, FloatingTextManager& texts);
    void act_captain(enemy& e, unit& player, std::vector<enemy>& enemies,
                     const GameMap& game_map, FloatingTextManager& texts);

    bool try_attack(enemy& e, unit& player, const GameMap& game_map,
                    FloatingTextManager& texts);
    bool try_melee(enemy& e, unit& player, const GameMap& game_map,
                   FloatingTextManager& texts);
    void move_toward_full(enemy& e, int tx, int ty,
                          const std::vector<std::pair<int,int>>& blocked,
                          const GameMap& game_map);

    bool is_hurt(const enemy& e) const;
    bool in_cover_from(int ex, int ey, int px, int py,
                       const GameMap& game_map) const;
    bool has_los_to_player(const enemy& e, const unit& player,
                           const GameMap& game_map) const;

    std::optional<std::pair<int,int>> find_cover_tile(
        const enemy& e, const unit& player,
        const std::vector<std::pair<int,int>>& blocked,
        const GameMap& game_map) const;

    std::optional<std::pair<int,int>> find_shooting_position(
        const enemy& e, const unit& player,
        const std::vector<std::pair<int,int>>& blocked,
        const GameMap& game_map) const;

    std::optional<std::pair<int,int>> find_flank_tile(
        const enemy& e, const unit& player,
        const std::vector<enemy>& enemies,
        const std::vector<std::pair<int,int>>& blocked,
        const GameMap& game_map) const;

    bool tile_occupied(int x, int y, const unit& player,
                       const std::vector<enemy>& enemies,
                       const enemy& self) const;

    std::pair<float,float> average_attack_vector(
        const unit& player, const std::vector<enemy>& enemies) const;
};