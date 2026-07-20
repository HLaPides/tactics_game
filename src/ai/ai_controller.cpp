#include "ai_controller.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_map>

// ─── constants ───────────────────────────────────────────────────────────────

static const int VANE_LAIR_COL = 53;

// ─── helpers ─────────────────────────────────────────────────────────────────

static std::vector<std::pair<int,int>> build_blocked(
    const enemy& self, const std::vector<enemy>& enemies, const unit& player) {
    std::vector<std::pair<int,int>> blocked;
    for (auto& other : enemies) {
        if (!other.is_alive() || &other == &self) continue;
        blocked.push_back({other.get_x_pos(), other.get_y_pos()});
    }
    blocked.push_back({player.get_x_pos(), player.get_y_pos()});
    return blocked;
}

bool AIController::tile_occupied(int x, int y, const unit& player,
                                  const std::vector<enemy>& enemies,
                                  const enemy& self) const {
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
    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1, 0, 0};

    for (int d = 0; d < 4; d++) {
        int nx = ex + dx[d];
        int ny = ey + dy[d];
        Tile t = game_map.get_tile(nx, ny);
        if (t.cover == COVER_NONE) continue;

        bool covers = false;
        if (d == 0 && py < ey && t.faces.south) covers = true;
        if (d == 1 && py > ey && t.faces.north) covers = true;
        if (d == 2 && px < ex && t.faces.east)  covers = true;
        if (d == 3 && px > ex && t.faces.west)  covers = true;

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

// ─── BFS full movement ───────────────────────────────────────────────────────

void AIController::move_toward_full(enemy& e, int tx, int ty,
                                     const std::vector<std::pair<int,int>>& blocked,
                                     const GameMap& game_map) {
    int mx       = e.get_x_pos();
    int my       = e.get_y_pos();
    int movement = e.get_movement();

    auto reachable = get_reachable_tiles(mx, my, movement, game_map, blocked);
    if (reachable.empty()) { e.use_action(); return; }

    std::pair<int,int> best     = {mx, my};
    int                best_dist = std::max(abs(mx - tx), abs(my - ty));

    for (auto& [col, row] : reachable) {
        int dist = std::max(abs(col - tx), abs(row - ty));
        if (dist < best_dist) {
            best_dist = dist;
            best      = {col, row};
        }
    }

    if (best.first != mx || best.second != my)
        e.set_position(best.first, best.second);

    e.use_action();
}

// ─── position finders ────────────────────────────────────────────────────────

std::optional<std::pair<int,int>> AIController::find_cover_tile(
    const enemy& e, const unit& player,
    const std::vector<std::pair<int,int>>& blocked,
    const GameMap& game_map) const {

    auto reachable = get_reachable_tiles(
        e.get_x_pos(), e.get_y_pos(), e.get_movement(), game_map, blocked);

    std::optional<std::pair<int,int>> best;
    int best_dist = std::numeric_limits<int>::max();

    int px = player.get_x_pos();
    int py = player.get_y_pos();

    for (auto& [col, row] : reachable) {
        if (!in_cover_from(col, row, px, py, game_map)) continue;
        int dist = std::max(abs(col - px), abs(row - py));
        if (dist > e.get_sight_range()) continue;
        if (!has_los(col, row, px, py, game_map)) continue;

        int move_dist = std::max(abs(col - e.get_x_pos()), abs(row - e.get_y_pos()));
        if (move_dist < best_dist) {
            best_dist = move_dist;
            best      = {col, row};
        }
    }
    return best;
}

std::optional<std::pair<int,int>> AIController::find_shooting_position(
    const enemy& e, const unit& player,
    const std::vector<std::pair<int,int>>& blocked,
    const GameMap& game_map) const {

    auto reachable = get_reachable_tiles(
        e.get_x_pos(), e.get_y_pos(), e.get_movement(), game_map, blocked);

    std::optional<std::pair<int,int>> best;
    int best_score = -1;

    int px = player.get_x_pos();
    int py = player.get_y_pos();

    for (auto& [col, row] : reachable) {
        int dist = std::max(abs(col - px), abs(row - py));
        if (dist > e.get_sight_range()) continue;
        if (!has_los(col, row, px, py, game_map)) continue;

        int odds = 85 + (e.get_aim() - 60)
                 - (dist * dist * 10) / std::max(e.get_aim(), 1)
                 - player.get_defense();
        odds = std::max(5, std::min(99, odds));
        if (odds < 40) continue;

        bool covered = in_cover_from(col, row, px, py, game_map);
        int  score   = odds + (covered ? 30 : 0);

        if (score > best_score) {
            best_score = score;
            best       = {col, row};
        }
    }
    return best;
}

std::optional<std::pair<int,int>> AIController::find_flank_tile(
    const enemy& e, const unit& player,
    const std::vector<enemy>& enemies,
    const std::vector<std::pair<int,int>>& blocked,
    const GameMap& game_map) const {

    auto reachable = get_reachable_tiles(
        e.get_x_pos(), e.get_y_pos(), e.get_movement(), game_map, blocked);

    auto [avg_x, avg_y] = average_attack_vector(player, enemies);
    float avg_len = std::sqrt(avg_x * avg_x + avg_y * avg_y);

    int px = player.get_x_pos();
    int py = player.get_y_pos();

    std::optional<std::pair<int,int>> best;
    float best_score = std::numeric_limits<float>::lowest();

    for (auto& [col, row] : reachable) {
        int dist = std::max(abs(col - px), abs(row - py));
        if (dist > e.get_sight_range()) continue;
        if (!has_los(col, row, px, py, game_map)) continue;

        float vx   = col - px;
        float vy   = row - py;
        float vlen = std::sqrt(vx * vx + vy * vy);
        if (vlen < 0.001f) continue;

        float dot = 0;
        if (avg_len > 0.001f)
            dot = (vx * avg_x + vy * avg_y) / (vlen * avg_len);

        float score = -dot;
        if (score > best_score) {
            best_score = score;
            best       = {col, row};
        }
    }
    return best;
}

// ─── shared attack helpers ───────────────────────────────────────────────────

bool AIController::try_attack(enemy& e, unit& player,
                               const GameMap& game_map, FloatingTextManager& texts) {
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

// ─── type-specific AI ────────────────────────────────────────────────────────

void AIController::act_soldier(enemy& e, unit& player, std::vector<enemy>& enemies,
                                const GameMap& game_map, FloatingTextManager& texts) {
    auto blocked = build_blocked(e, enemies, player);

    if (is_hurt(e) && !in_cover_from(e.get_x_pos(), e.get_y_pos(),
                                      player.get_x_pos(), player.get_y_pos(), game_map)) {
        auto cover = find_cover_tile(e, player, blocked, game_map);
        if (cover) {
            move_toward_full(e, cover->first, cover->second, blocked, game_map);
            return;
        }
    }

    if (try_attack(e, player, game_map, texts)) return;

    auto shoot_pos = find_shooting_position(e, player, blocked, game_map);
    if (shoot_pos) {
        move_toward_full(e, shoot_pos->first, shoot_pos->second, blocked, game_map);
        return;
    }

    auto flank = find_flank_tile(e, player, enemies, blocked, game_map);
    if (flank) {
        move_toward_full(e, flank->first, flank->second, blocked, game_map);
        return;
    }

    move_toward_full(e, player.get_x_pos(), player.get_y_pos(), blocked, game_map);
}

void AIController::act_guard(enemy& e, unit& player, std::vector<enemy>& enemies,
                              const GameMap& game_map, FloatingTextManager& texts) {
    auto blocked = build_blocked(e, enemies, player);

    if (is_hurt(e) && !in_cover_from(e.get_x_pos(), e.get_y_pos(),
                                      player.get_x_pos(), player.get_y_pos(), game_map)) {
        auto cover = find_cover_tile(e, player, blocked, game_map);
        if (cover) {
            move_toward_full(e, cover->first, cover->second, blocked, game_map);
            return;
        }
    }

    if (try_melee(e, player, game_map, texts)) return;

    move_toward_full(e, player.get_x_pos(), player.get_y_pos(), blocked, game_map);
}

void AIController::act_captain(enemy& e, unit& player, std::vector<enemy>& enemies,
                                const GameMap& game_map, FloatingTextManager& texts) {
    auto blocked = build_blocked(e, enemies, player);

    int px = player.get_x_pos();
    int py = player.get_y_pos();

    bool player_in_lair  = (px >= VANE_LAIR_COL);
    bool player_adjacent = std::max(abs(e.get_x_pos() - px),
                                     abs(e.get_y_pos() - py)) <= 1;

    if (try_attack(e, player, game_map, texts)) return;

    if (!player_in_lair) {
        auto shoot_pos = find_shooting_position(e, player, blocked, game_map);
        if (shoot_pos && shoot_pos->first >= VANE_LAIR_COL) {
            move_toward_full(e, shoot_pos->first, shoot_pos->second, blocked, game_map);
            return;
        }
        e.use_action();
        return;
    }

    if (player_adjacent) {
        if (try_melee(e, player, game_map, texts)) return;
    }

    auto shoot_pos = find_shooting_position(e, player, blocked, game_map);
    if (shoot_pos && shoot_pos->first >= VANE_LAIR_COL) {
        move_toward_full(e, shoot_pos->first, shoot_pos->second, blocked, game_map);
        return;
    }

    e.use_action();
}

// ─── main entry point ────────────────────────────────────────────────────────

void AIController::act(enemy& e, unit& player, std::vector<enemy>& enemies,
                        const GameMap& game_map, FloatingTextManager& texts) {
    if (!e.is_alive()) return;

    for (int a = 0; a < 2; a++) {
        if (e.get_actions() <= 0) break;
        const std::string& behavior = e.get_ai_behavior();
        if      (behavior == "soldier") act_soldier(e, player, enemies, game_map, texts);
        else if (behavior == "guard")   act_guard(e, player, enemies, game_map, texts);
        else if (behavior == "captain") act_captain(e, player, enemies, game_map, texts);
        else                            act_soldier(e, player, enemies, game_map, texts);
    }
    e.reset_actions();
}