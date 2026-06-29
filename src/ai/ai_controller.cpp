#include "ai_controller.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <limits>

// ─── helpers ────────────────────────────────────────────────────────────────

bool AIController::tile_occupied(int x, int y, const unit& player,
                                  const std::vector<enemy>& enemies, const enemy& self) const {
    if (player.get_x_pos() == x && player.get_y_pos() == y) return true;
    for (auto& e : enemies) {
        if (!e.is_alive()) continue;
        if (&e == &self) continue;
        if (e.get_x_pos() == x && e.get_y_pos() == y) return true;
    }
    return false;
}

bool AIController::is_hurt(const enemy& e) const {
    return e.get_hp() < e.get_max_hp() / 2;
}

bool AIController::has_los_to_player(const enemy& e, const unit& player,
                                      const GameMap& game_map) const {
    int dist = std::max(abs(e.get_x_pos() - player.get_x_pos()),
                        abs(e.get_y_pos() - player.get_y_pos()));
    return dist <= e.get_sight_range() &&
           has_los(e.get_x_pos(), e.get_y_pos(),
                   player.get_x_pos(), player.get_y_pos(), game_map);
}

bool AIController::in_cover_from(int ex, int ey, int px, int py,
                                  const GameMap& game_map) const {
    // check all four neighbours for a cover tile whose face points toward the player
    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1, 0, 0};

    for (int d = 0; d < 4; d++) {
        int nx = ex + dx[d];
        int ny = ey + dy[d];
        Tile t = game_map.get_tile(nx, ny);
        if (t.cover == COVER_NONE) continue;

        // check if cover faces toward the player
        bool covers = false;
        if (d == 0 && py < ey && t.faces.south) covers = true;  // north neighbour
        if (d == 1 && py > ey && t.faces.north) covers = true;  // south neighbour
        if (d == 2 && px < ex && t.faces.east)  covers = true;  // west neighbour
        if (d == 3 && px > ex && t.faces.west)  covers = true;  // east neighbour

        if (covers) return true;
    }
    return false;
}

std::pair<float,float> AIController::average_attack_vector(
    const unit& player, const std::vector<enemy>& enemies) const {
    float ax = 0, ay = 0;
    int   count = 0;
    for (auto& e : enemies) {
        if (!e.is_alive()) continue;
        ax += e.get_x_pos() - player.get_x_pos();
        ay += e.get_y_pos() - player.get_y_pos();
        count++;
    }
    if (count == 0) return {0, 0};
    return {ax / count, ay / count};
}

std::optional<std::pair<int,int>> AIController::find_cover_tile(
    const enemy& e, const unit& player,
    std::vector<enemy>& enemies, const GameMap& game_map) const {

    std::vector<std::pair<int,int>> blocked;
    for (auto& other : enemies) {
        if (!other.is_alive() || &other == &e) continue;
        blocked.push_back({other.get_x_pos(), other.get_y_pos()});
    }
    blocked.push_back({player.get_x_pos(), player.get_y_pos()});

    auto reachable = get_reachable_tiles(
        e.get_x_pos(), e.get_y_pos(), e.get_movement(), game_map, blocked);

    std::optional<std::pair<int,int>> best;
    int best_dist = std::numeric_limits<int>::max();

    int px = player.get_x_pos();
    int py = player.get_y_pos();

    for (auto& [col, row] : reachable) {
        // must have cover from player direction
        if (!in_cover_from(col, row, px, py, game_map)) continue;

        // must also have LOS to player so they can shoot from cover
        int dist = std::max(abs(col - px), abs(row - py));
        if (dist > e.get_sight_range()) continue;
        if (!has_los(col, row, px, py, game_map)) continue;

        // prefer closest covered tile with LOS
        int move_dist = std::max(abs(col - e.get_x_pos()), abs(row - e.get_y_pos()));
        if (move_dist < best_dist) {
            best_dist = move_dist;
            best = {col, row};
        }
    }
    return best;
}

std::optional<std::pair<int,int>> AIController::find_shooting_position(
    const enemy& e, const unit& player,
    std::vector<enemy>& enemies, const GameMap& game_map) const {

    std::vector<std::pair<int,int>> blocked;
    for (auto& other : enemies) {
        if (!other.is_alive() || &other == &e) continue;
        blocked.push_back({other.get_x_pos(), other.get_y_pos()});
    }
    blocked.push_back({player.get_x_pos(), player.get_y_pos()});

    auto reachable = get_reachable_tiles(
        e.get_x_pos(), e.get_y_pos(), e.get_movement(), game_map, blocked);

    std::optional<std::pair<int,int>> best;
    int best_score = -1;

    int px = player.get_x_pos();
    int py = player.get_y_pos();

    for (auto& [col, row] : reachable) {
        // must have LOS to player
        int dist = std::max(abs(col - px), abs(row - py));
        if (dist > e.get_sight_range()) continue;
        if (!has_los(col, row, px, py, game_map)) continue;

        // calculate odds from this position
        // temporarily fake the enemy position for calculation
        int odds = 85 + (e.get_aim() - 60)
                 - (dist * dist * 10) / std::max(e.get_aim(), 1)
                 - player.get_defense();
        odds = std::max(5, std::min(99, odds));

        if (odds < 40) continue;  // not worth shooting from here

        // prefer covered positions
        bool covered = in_cover_from(col, row, px, py, game_map);
        int  score   = odds + (covered ? 30 : 0);

        if (score > best_score) {
            best_score = score;
            best = {col, row};
        }
    }
    return best;
}

std::optional<std::pair<int,int>> AIController::find_flank_tile(
    const enemy& e, const unit& player,
    const std::vector<enemy>& enemies, const GameMap& game_map) const {

    std::vector<std::pair<int,int>> blocked;
    for (auto& other : enemies) {
        if (!other.is_alive() || &other == &e) continue;
        blocked.push_back({other.get_x_pos(), other.get_y_pos()});
    }
    blocked.push_back({player.get_x_pos(), player.get_y_pos()});

    auto reachable = get_reachable_tiles(
        e.get_x_pos(), e.get_y_pos(), e.get_movement(), game_map, blocked);

    auto [avg_x, avg_y] = average_attack_vector(player, enemies);
    float avg_len = std::sqrt(avg_x * avg_x + avg_y * avg_y);

    int   px = player.get_x_pos();
    int   py = player.get_y_pos();

    std::optional<std::pair<int,int>> best;
    float best_score = std::numeric_limits<float>::lowest();

    for (auto& [col, row] : reachable) {
        int dist = std::max(abs(col - px), abs(row - py));
        if (dist > e.get_sight_range()) continue;
        if (!has_los(col, row, px, py, game_map)) continue;

        // vector from player to this tile
        float vx = col - px;
        float vy = row - py;
        float vlen = std::sqrt(vx * vx + vy * vy);
        if (vlen < 0.001f) continue;

        // dot product with average attack vector — lower = more flanking
        float dot = 0;
        if (avg_len > 0.001f)
            dot = (vx * avg_x + vy * avg_y) / (vlen * avg_len);

        // score: prefer tiles that diverge from average attack direction
        float score = -dot;
        if (score > best_score) {
            best_score = score;
            best = {col, row};
        }
    }
    return best;
}

// ─── shared attack helpers ───────────────────────────────────────────────────

bool AIController::try_attack(enemy& e, unit& player,
                               const GameMap& game_map, FloatingTextManager& texts) {
    int dist = std::max(abs(e.get_x_pos() - player.get_x_pos()),
                        abs(e.get_y_pos() - player.get_y_pos()));

    if (!has_los_to_player(e, player, game_map)) return false;

    AttackResult result = resolve_attack(e, player, game_map, e.get_shoot_damage());
    player.take_damage(result.damage);
    e.use_action();

    if (result.hit && result.crit)
        texts.spawn(player.get_x_pos(), player.get_y_pos(), "CRIT!", YELLOW);
    else if (!result.hit)
        texts.spawn(player.get_x_pos(), player.get_y_pos(), "MISS!", GRAY);
    return true;
}

bool AIController::try_melee(enemy& e, unit& player,
                              const GameMap& game_map, FloatingTextManager& texts) {
    int dist = std::max(abs(e.get_x_pos() - player.get_x_pos()),
                        abs(e.get_y_pos() - player.get_y_pos()));
    if (dist > 1) return false;

    AttackResult result = resolve_attack(e, player, game_map, e.get_melee_damage());
    player.take_damage(result.damage);
    e.use_action();

    if (result.hit && result.crit)
        texts.spawn(player.get_x_pos(), player.get_y_pos(), "CRIT!", YELLOW);
    else if (!result.hit)
        texts.spawn(player.get_x_pos(), player.get_y_pos(), "MISS!", GRAY);
    return true;
}

void AIController::move_toward(enemy& e, unit& player,
                                std::vector<enemy>& enemies, const GameMap& game_map) {
    int px = player.get_x_pos();
    int py = player.get_y_pos();
    int step_x = e.get_x_pos() + (px > e.get_x_pos() ? 1 : px < e.get_x_pos() ? -1 : 0);
    int step_y = e.get_y_pos() + (py > e.get_y_pos() ? 1 : py < e.get_y_pos() ? -1 : 0);

    if (!tile_occupied(step_x, step_y, player, enemies, e) &&
        game_map.is_walkable(step_x, step_y))
        e.set_position(step_x, step_y);
    e.use_action();
}

void AIController::move_to(enemy& e, int tx, int ty,
                            std::vector<enemy>& enemies, unit& player,
                            const GameMap& game_map) {
    // step one tile toward target
    int step_x = e.get_x_pos() + (tx > e.get_x_pos() ? 1 : tx < e.get_x_pos() ? -1 : 0);
    int step_y = e.get_y_pos() + (ty > e.get_y_pos() ? 1 : ty < e.get_y_pos() ? -1 : 0);

    if (!tile_occupied(step_x, step_y, player, enemies, e) &&
        game_map.is_walkable(step_x, step_y))
        e.set_position(step_x, step_y);
    e.use_action();
}

// ─── type-specific AI ────────────────────────────────────────────────────────

void AIController::act_soldier(enemy& e, unit& player, std::vector<enemy>& enemies,
                                const GameMap& game_map, FloatingTextManager& texts) {
    // priority 1: if hurt and not in cover, find cover
    if (is_hurt(e) && !in_cover_from(e.get_x_pos(), e.get_y_pos(),
                                      player.get_x_pos(), player.get_y_pos(), game_map)) {
        auto cover = find_cover_tile(e, player, enemies, game_map);
        if (cover) { move_to(e, cover->first, cover->second, enemies, player, game_map); return; }
    }

    // priority 2: shoot if LOS
    if (try_attack(e, player, game_map, texts)) return;

    // priority 3: move to covered shooting position
    auto shoot_pos = find_shooting_position(e, player, enemies, game_map);
    if (shoot_pos) { move_to(e, shoot_pos->first, shoot_pos->second, enemies, player, game_map); return; }

    // priority 4: flank
    auto flank = find_flank_tile(e, player, enemies, game_map);
    if (flank) { move_to(e, flank->first, flank->second, enemies, player, game_map); return; }

    // fallback: move toward player
    move_toward(e, player, enemies, game_map);
}

void AIController::act_guard(enemy& e, unit& player, std::vector<enemy>& enemies,
                              const GameMap& game_map, FloatingTextManager& texts) {
    // priority 1: if hurt and not in cover, find cover
    if (is_hurt(e) && !in_cover_from(e.get_x_pos(), e.get_y_pos(),
                                      player.get_x_pos(), player.get_y_pos(), game_map)) {
        auto cover = find_cover_tile(e, player, enemies, game_map);
        if (cover) { move_to(e, cover->first, cover->second, enemies, player, game_map); return; }
    }

    // priority 2: melee if adjacent
    if (try_melee(e, player, game_map, texts)) return;

    // priority 3: flank then charge
    auto flank = find_flank_tile(e, player, enemies, game_map);
    if (flank) { move_to(e, flank->first, flank->second, enemies, player, game_map); return; }

    // fallback: charge
    move_toward(e, player, enemies, game_map);
}

void AIController::act_captain(enemy& e, unit& player, std::vector<enemy>& enemies,
                                const GameMap& game_map, FloatingTextManager& texts) {
    // captain: shoot if has LOS, otherwise find flanking shooting position
    // more aggressive than soldier, doesn't retreat to cover when hurt

    // priority 1: shoot if LOS
    if (try_attack(e, player, game_map, texts)) return;

    // priority 2: find best flanking shooting position
    auto flank = find_flank_tile(e, player, enemies, game_map);
    if (flank) { move_to(e, flank->first, flank->second, enemies, player, game_map); return; }

    // priority 3: move toward player aggressively
    move_toward(e, player, enemies, game_map);
}

// ─── main entry point ────────────────────────────────────────────────────────

void AIController::act(enemy& e, unit& player, std::vector<enemy>& enemies,
                        const GameMap& game_map, FloatingTextManager& texts) {
    if (!e.is_alive()) return;

    for (int a = 0; a < 2; a++) {
        if (e.get_actions() <= 0) break;
        switch (e.get_type()) {
            case EnemyType::SOLDIER: act_soldier(e, player, enemies, game_map, texts); break;
            case EnemyType::GUARD:   act_guard(e, player, enemies, game_map, texts);   break;
            case EnemyType::CAPTAIN: act_captain(e, player, enemies, game_map, texts); break;
        }
    }
    e.reset_actions();
}