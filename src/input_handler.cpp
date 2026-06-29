#include "input_handler.h"
#include "combat.h"
#include "raylib.h"
#include <algorithm>
#include <vector>

InputHandler::InputHandler(const AppConfig& cfg) : config(cfg) {}

int InputHandler::mouse_tile_x(Vector2 mouse, const Camera2D& cam) const {
    return (int)(GetScreenToWorld2D(mouse, cam).x / config.tile_size);
}
int InputHandler::mouse_tile_y(Vector2 mouse, const Camera2D& cam) const {
    return (int)(GetScreenToWorld2D(mouse, cam).y / config.tile_size);
}
bool InputHandler::mouse_on_grid(Vector2 mouse) const {
    return mouse.y < config.grid_h;
}

bool InputHandler::clicked_end_turn(Vector2 mouse) const {
    int bar_y = config.screen_h - 80;
    int ex    = config.screen_w - END_TURN_X_OFFSET;
    int ey    = bar_y + END_TURN_Y_OFFSET;
    return mouse.x >= ex && mouse.x < ex + END_TURN_W &&
           mouse.y >= ey && mouse.y < ey + END_TURN_H;
}

int InputHandler::clicked_ability(Vector2 mouse, const unit& active) const {
    int bar_y = config.screen_h - 80;
    int by    = bar_y + BTN_Y_OFFSET;
    const auto& abilities = active.get_abilities();
    for (int i = 0; i < (int)abilities.size(); i++) {
        int bx = BTN_START_X + i * (BTN_W + BTN_GAP);
        if (mouse.x >= bx && mouse.x < bx + BTN_W &&
            mouse.y >= by && mouse.y < by + BTN_H)
            return i;
    }
    return -1;
}

std::optional<IntentType> InputHandler::mode_to_intent(ActionMode mode) const {
    switch (mode) {
        case ActionMode::SHOOT:       return IntentType::Shoot;
        case ActionMode::MELEE:       return IntentType::Melee;
        case ActionMode::HEAL:        return IntentType::Heal;
        case ActionMode::DIRTY_TRICK: return IntentType::DirtyTrick;
        case ActionMode::AIMED_SHOT:  return IntentType::AimedShot;
        case ActionMode::RUSH:        return IntentType::Rush;
        default:                      return std::nullopt;
    }
}

std::vector<int> InputHandler::get_valid_targets(const GameState& state) const {
    if (state.players.empty()) return {};
    const unit& active = state.players[state.selected_player];
    std::vector<int> targets;

    // heal mode — targets are friendly players within range 1 (including self)
    if (state.mode == ActionMode::HEAL) {
        for (int i = 0; i < (int)state.players.size(); i++) {
            const auto& p = state.players[i];
            if (!p.is_alive()) continue;
            int dist = std::max(abs(p.get_x_pos() - active.get_x_pos()),
                                abs(p.get_y_pos() - active.get_y_pos()));
            if (dist <= 1)
                targets.push_back(i);
        }
        std::sort(targets.begin(), targets.end(), [&](int a, int b) {
            return state.players[a].get_x_pos() < state.players[b].get_x_pos();
        });
        return targets;
    }

    // attack modes — targets are spotted enemies
    for (int i = 0; i < (int)state.enemies.size(); i++) {
        const auto& e = state.enemies[i];
        if (!e.is_alive()) continue;
        if (!state.spotted[i]) continue;

        int dist = std::max(abs(e.get_x_pos() - active.get_x_pos()),
                            abs(e.get_y_pos() - active.get_y_pos()));

        bool valid = false;
        if (state.mode == ActionMode::SHOOT   ||
            state.mode == ActionMode::AIMED_SHOT ||
            state.mode == ActionMode::DIRTY_TRICK) {
            valid = dist <= active.get_sight_range() &&
                    has_los(active.get_x_pos(), active.get_y_pos(),
                            e.get_x_pos(), e.get_y_pos(), state.map);
        } else if (state.mode == ActionMode::MELEE) {
            valid = dist <= 1;
        }

        if (valid) targets.push_back(i);
    }

    std::sort(targets.begin(), targets.end(), [&](int a, int b) {
        return state.enemies[a].get_x_pos() < state.enemies[b].get_x_pos();
    });

    return targets;
}

std::optional<Intent> InputHandler::poll(GameState& state) {
    if (state.players.empty()) return std::nullopt;
    const unit& active = state.players[state.selected_player];
    Vector2     mouse  = GetMousePosition();

    // Tab — cycle targets in attack/heal mode, cycle units otherwise
    if (IsKeyPressed(KEY_TAB) && state.phase == GamePhase::PLAYER_TURN) {
        if (state.mode != ActionMode::NONE &&
            state.mode != ActionMode::RUSH &&
            state.mode != ActionMode::OVERWATCH) {
            auto targets = get_valid_targets(state);
            if (!targets.empty()) {
                int cur = -1;
                for (int i = 0; i < (int)targets.size(); i++) {
                    if (targets[i] == state.target_index) { cur = i; break; }
                }
                int next = (cur + 1) % (int)targets.size();
                state.target_index = targets[next];
            }
            return std::nullopt;
        } else {
            int next = state.selected_player;
            for (int i = 1; i <= (int)state.players.size(); i++) {
                int idx = (state.selected_player + i) % (int)state.players.size();
                if (state.players[idx].is_alive()) { next = idx; break; }
            }
            if (next != state.selected_player)
                return Intent{ IntentType::SelectUnit, 0, 0, next };
        }
    }

    // overwatch fires immediately
    if (state.mode == ActionMode::OVERWATCH && state.phase == GamePhase::PLAYER_TURN) {
        state.mode = ActionMode::NONE;
        return Intent{ IntentType::Overwatch };
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_ESCAPE)) {
        if (state.mode != ActionMode::NONE) {
            state.target_index = -1;
            return Intent{ IntentType::Cancel };
        }
    }

    if (IsKeyPressed(KEY_SPACE) && state.phase == GamePhase::PLAYER_TURN)
        return Intent{ IntentType::EndTurn };

    // Enter confirms on current target
    if (IsKeyPressed(KEY_ENTER) && state.phase == GamePhase::PLAYER_TURN &&
        state.target_index >= 0 && state.mode != ActionMode::NONE) {

        if (state.mode == ActionMode::HEAL &&
            state.target_index < (int)state.players.size()) {
            const auto& p = state.players[state.target_index];
            int tx = p.get_x_pos();
            int ty = p.get_y_pos();
            state.target_index = -1;
            return Intent{ IntentType::Heal, tx, ty };
        } else if (state.target_index < (int)state.enemies.size()) {
            const auto& e = state.enemies[state.target_index];
            auto intent_type = mode_to_intent(state.mode);
            if (intent_type.has_value()) {
                int tx = e.get_x_pos();
                int ty = e.get_y_pos();
                state.target_index = -1;
                return Intent{ intent_type.value(), tx, ty };
            }
        }
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return std::nullopt;

    // HUD clicks
    if (!mouse_on_grid(mouse)) {
        if (state.phase != GamePhase::PLAYER_TURN) return std::nullopt;
        if (clicked_end_turn(mouse)) return Intent{ IntentType::EndTurn };
        return std::nullopt;
    }

    int mx = mouse_tile_x(mouse, state.camera);
    int my = mouse_tile_y(mouse, state.camera);

    if (state.phase != GamePhase::PLAYER_TURN) return std::nullopt;

    // heal mode — clicking a friendly confirms heal
    if (state.mode == ActionMode::HEAL) {
        for (int i = 0; i < (int)state.players.size(); i++) {
            const auto& p = state.players[i];
            if (!p.is_alive()) continue;
            if (p.get_x_pos() == mx && p.get_y_pos() == my) {
                state.target_index = -1;
                return Intent{ IntentType::Heal, mx, my };
            }
        }
        return std::nullopt;
    }

    // click another player to select — only when not in ability mode
    if (state.mode == ActionMode::NONE) {
        for (int i = 0; i < (int)state.players.size(); i++) {
            if (i == state.selected_player) continue;
            if (!state.players[i].is_alive()) continue;
            if (state.players[i].get_x_pos() == mx && state.players[i].get_y_pos() == my)
                return Intent{ IntentType::SelectUnit, 0, 0, i };
        }
    }

    // click on an enemy in attack mode — confirm attack
    if (state.mode != ActionMode::NONE && state.mode != ActionMode::RUSH) {
        for (int i = 0; i < (int)state.enemies.size(); i++) {
            auto& e = state.enemies[i];
            if (!e.is_alive()) continue;
            if (!state.spotted[i]) continue;
            if (e.get_x_pos() != mx || e.get_y_pos() != my) continue;
            auto intent_type = mode_to_intent(state.mode);
            if (intent_type.has_value()) {
                state.target_index = -1;
                return Intent{ intent_type.value(), mx, my };
            }
        }
    }

    // rush / move
    if (state.mode != ActionMode::NONE) {
        auto intent_type = mode_to_intent(state.mode);
        if (intent_type.has_value())
            return Intent{ intent_type.value(), mx, my };
    }

    return Intent{ IntentType::Move, mx, my };
}

void InputHandler::update_preview(GameState& state) {
    if (state.players.empty()) return;
    unit&   active = state.players[state.selected_player];
    Vector2 mouse  = GetMousePosition();
    state.preview  = {};

    // ability button clicks
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !mouse_on_grid(mouse)
        && state.phase == GamePhase::PLAYER_TURN) {
        int idx = clicked_ability(mouse, active);
        if (idx >= 0) {
            const Ability& ab = active.get_abilities()[idx];
            bool on_cooldown = !ab.is_ready();
            bool cant_afford = (ab.get_cost() > 0 && active.get_actions() < ab.get_cost());
            if (on_cooldown || cant_afford) return;

            if (state.mode == ab.get_mode()) {
                state.mode         = ActionMode::NONE;
                state.target_index = -1;
            } else {
                state.mode = ab.get_mode();
                auto targets = get_valid_targets(state);
                state.target_index = targets.empty() ? -1 : targets[0];
            }
            state.preview = {};
            return;
        }
        return;
    }

    // heal preview from target_index
    if (state.mode == ActionMode::HEAL) {
        if (state.target_index >= 0 && state.target_index < (int)state.players.size()) {
            // no attack preview panel for heal — highlight handled by draw_target_highlights
        }
        return;
    }

    // attack preview from target_index
    bool attack_mode = (state.mode == ActionMode::SHOOT     ||
                        state.mode == ActionMode::AIMED_SHOT ||
                        state.mode == ActionMode::MELEE      ||
                        state.mode == ActionMode::DIRTY_TRICK);

    if (attack_mode && state.target_index >= 0 &&
        state.target_index < (int)state.enemies.size()) {
        auto& e = state.enemies[state.target_index];
        if (e.is_alive() && state.spotted[state.target_index]) {
            int base_dmg = (state.mode == ActionMode::MELEE)
                         ? active.get_melee_damage()
                         : active.get_shoot_damage();
            state.preview.active = true;
            state.preview.target = &e;
            state.preview.result = calculate_odds(active, e, state.map, base_dmg);
            if (state.mode == ActionMode::AIMED_SHOT)
                state.preview.result.crit_chance =
                    state.preview.result.is_flanking ? 35 : 20;
        }
        return;
    }

    // fallback hover preview
    if (!mouse_on_grid(mouse) || !attack_mode) return;

    int mx = mouse_tile_x(mouse, state.camera);
    int my = mouse_tile_y(mouse, state.camera);

    for (int i = 0; i < (int)state.enemies.size(); i++) {
        auto& e = state.enemies[i];
        if (!e.is_alive()) continue;
        if (!state.spotted[i]) continue;
        if (e.get_x_pos() != mx || e.get_y_pos() != my) continue;

        int dist = std::max(abs(mx - active.get_x_pos()),
                            abs(my - active.get_y_pos()));
        bool in_range = false;
        if (state.mode == ActionMode::SHOOT    ||
            state.mode == ActionMode::AIMED_SHOT ||
            state.mode == ActionMode::DIRTY_TRICK) {
            in_range = dist <= active.get_sight_range() &&
                       has_los(active.get_x_pos(), active.get_y_pos(),
                               e.get_x_pos(), e.get_y_pos(), state.map);
        } else if (state.mode == ActionMode::MELEE) {
            in_range = dist <= 1;
        }

        if (in_range) {
            int base_dmg = (state.mode == ActionMode::MELEE)
                         ? active.get_melee_damage()
                         : active.get_shoot_damage();
            state.preview.active = true;
            state.preview.target = &e;
            state.preview.result = calculate_odds(active, e, state.map, base_dmg);
            if (state.mode == ActionMode::AIMED_SHOT)
                state.preview.result.crit_chance =
                    state.preview.result.is_flanking ? 35 : 20;
        }
        break;
    }
}