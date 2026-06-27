#include "input_handler.h"
#include "combat.h"
#include "raylib.h"
#include <algorithm>

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

std::optional<Intent> InputHandler::poll(GameState& state) {
    if (state.players.empty()) return std::nullopt;
    const unit& active = state.players[state.selected_player];
    Vector2     mouse  = GetMousePosition();

    // Tab cycles living players
    if (IsKeyPressed(KEY_TAB) && state.phase == GamePhase::PLAYER_TURN) {
        int next = state.selected_player;
        for (int i = 1; i <= (int)state.players.size(); i++) {
            int idx = (state.selected_player + i) % (int)state.players.size();
            if (state.players[idx].is_alive()) { next = idx; break; }
        }
        if (next != state.selected_player)
            return Intent{ IntentType::SelectUnit, 0, 0, next };
    }

    // overwatch needs no target — fire intent as soon as mode is set
    if (state.mode == ActionMode::OVERWATCH && state.phase == GamePhase::PLAYER_TURN) {
        state.mode = ActionMode::NONE;
        return Intent{ IntentType::Overwatch };
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_ESCAPE)) {
        if (state.mode != ActionMode::NONE)
            return Intent{ IntentType::Cancel };
    }

    if (IsKeyPressed(KEY_SPACE) && state.phase == GamePhase::PLAYER_TURN)
        return Intent{ IntentType::EndTurn };

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

    // heal mode — clicking any friendly unit fires heal intent before select check
    if (state.mode == ActionMode::HEAL) {
        for (auto& p : state.players) {
            if (!p.is_alive()) continue;
            if (p.get_x_pos() == mx && p.get_y_pos() == my)
                return Intent{ IntentType::Heal, mx, my };
        }
        return std::nullopt;  // clicked empty tile in heal mode — do nothing
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

    // active mode — fire intent at clicked tile
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

            // toggle mode — overwatch sets mode and poll fires the intent next frame
            state.mode = (state.mode == ab.get_mode()) ? ActionMode::NONE : ab.get_mode();
            state.preview = {};
            return;
        }
        return;
    }

    if (!mouse_on_grid(mouse) || state.mode == ActionMode::NONE) return;

    int mx = mouse_tile_x(mouse, state.camera);
    int my = mouse_tile_y(mouse, state.camera);

    // attack preview for shoot / aimed shot
    if (state.mode == ActionMode::SHOOT || state.mode == ActionMode::AIMED_SHOT) {
        for (int i = 0; i < (int)state.enemies.size(); i++) {
            auto& e = state.enemies[i];
            if (!e.is_alive()) continue;
            if (!state.spotted[i]) continue;
            if (e.get_x_pos() != mx || e.get_y_pos() != my) continue;

            int dist = std::max(abs(mx - active.get_x_pos()),
                                abs(my - active.get_y_pos()));
            if (dist <= active.get_sight_range() &&
                has_los(active.get_x_pos(), active.get_y_pos(),
                        e.get_x_pos(), e.get_y_pos(), state.map)) {
                state.preview.active = true;
                state.preview.target = &e;
                state.preview.result = calculate_odds(
                    active, e, state.map, active.get_shoot_damage());
                if (state.mode == ActionMode::AIMED_SHOT)
                    state.preview.result.crit_chance =
                        state.preview.result.is_flanking ? 35 : 20;
            }
            break;
        }
        return;
    }

    if (state.mode == ActionMode::MELEE) {
    for (int i = 0; i < (int)state.enemies.size(); i++) {
        auto& e = state.enemies[i];
        if (!e.is_alive()) continue;
        if (!state.spotted[i]) continue;
        if (e.get_x_pos() != mx || e.get_y_pos() != my) continue;

        int dist = std::max(abs(mx - active.get_x_pos()),
                            abs(my - active.get_y_pos()));
        if (dist <= 1) {
            state.preview.active = true;
            state.preview.target = &e;
            state.preview.result = calculate_odds(
                active, e, state.map, active.get_melee_damage());
        }
        break;
    }
    return;
}

if (state.mode == ActionMode::DIRTY_TRICK) {
    for (int i = 0; i < (int)state.enemies.size(); i++) {
        auto& e = state.enemies[i];
        if (!e.is_alive()) continue;
        if (!state.spotted[i]) continue;
        if (e.get_x_pos() != mx || e.get_y_pos() != my) continue;

        int dist = std::max(abs(mx - active.get_x_pos()),
                            abs(my - active.get_y_pos()));
        if (dist <= active.get_sight_range() &&
            has_los(active.get_x_pos(), active.get_y_pos(),
                    e.get_x_pos(), e.get_y_pos(), state.map)) {
            state.preview.active = true;
            state.preview.target = &e;
            // preview shows what their aim will be after the trick
            state.preview.result = calculate_odds(
                active, e, state.map, active.get_melee_damage());
        }
        break;
    }
    return;
}
}