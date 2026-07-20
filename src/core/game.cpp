#include "game.h"
#include "../combat/combat.h"
#include "../units/unit_factory.h"
#include "raylib.h"
#include <algorithm>
#include <vector>
#include <string>
#include <functional>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cctype>

// ─── constructor ──────────────────────────────────────────────────────────────

game::game(const std::string& level_dir, const AppConfig& cfg)
    : config(cfg)
    , campaign()
    , state()
    , input(cfg)
    , turns()
    , ai()
    , renderer(cfg)
    , level_dir(level_dir)
{
    init_campaign();
    start_mission();
}

// ─── enemy defs ───────────────────────────────────────────────────────────────

void game::load_enemy_defs(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return;
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string src = ss.str();

    size_t arr_pos = src.find("\"enemies\"");
    if (arr_pos == std::string::npos) return;
    size_t arr_start = src.find('[', arr_pos);
    if (arr_start == std::string::npos) return;

    size_t search = arr_start + 1;
    while (true) {
        size_t obj_start = src.find('{', search);
        if (obj_start == std::string::npos) break;

        int    depth = 1;
        size_t p     = obj_start + 1;
        while (p < src.size() && depth > 0) {
            if (src[p] == '{') depth++;
            else if (src[p] == '}') depth--;
            p++;
        }
        std::string obj = src.substr(obj_start, p - obj_start);
        search = p;

        // skip the outer array wrapper
        if (obj.find("\"enemies\"") != std::string::npos) continue;

        auto jstr = [&](const std::string& key, const std::string& def) -> std::string {
            std::string k = "\"" + key + "\"";
            size_t pos = obj.find(k);
            if (pos == std::string::npos) return def;
            pos = obj.find(':', pos);
            if (pos == std::string::npos) return def;
            pos = obj.find('"', pos);
            if (pos == std::string::npos) return def;
            pos++;
            size_t end = obj.find('"', pos);
            if (end == std::string::npos) return def;
            return obj.substr(pos, end - pos);
        };

        auto jint = [&](const std::string& key, int def) -> int {
            std::string k = "\"" + key + "\"";
            size_t pos = obj.find(k);
            if (pos == std::string::npos) return def;
            pos = obj.find(':', pos);
            if (pos == std::string::npos) return def;
            pos++;
            while (pos < obj.size() && std::isspace((unsigned char)obj[pos])) pos++;
            try { return std::stoi(obj.substr(pos)); }
            catch (...) { return def; }
        };

        EnemyDef def;
        def.id          = jstr("id",     "soldier");
        def.sprite      = jstr("sprite", "soldier");
        def.ai_behavior = jstr("ai",     "soldier");
        std::string sc  = jstr("spawn_char", "E");
        def.spawn_char  = sc.empty() ? 'E' : sc[0];

        def.stats.movement     = jint("movement",     4);
        def.stats.hp           = jint("hp",           4);
        def.stats.aim          = jint("aim",          60);
        def.stats.defense      = jint("defense",      5);
        def.stats.shoot_range  = jint("shoot_range",  4);
        def.stats.shoot_damage = jint("shoot_damage", 1);
        def.stats.melee_damage = jint("melee_damage", 1);
        def.stats.sight_range  = jint("sight_range",  6);

        enemy_defs.push_back(def);
    }
}

// ─── save / load ──────────────────────────────────────────────────────────────

void game::save_campaign(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << "{\n";
    f << "  \"mission_index\": " << campaign.mission_index << ",\n";
    f << "  \"roster\": [\n";

    for (int i = 0; i < (int)campaign.names.size(); i++) {
        bool dead = campaign.permanently_dead[i];
        f << "    { \"name\": \"" << campaign.names[i] << "\","
          << " \"dead\": " << (dead ? "true" : "false") << " }";
        if (i < (int)campaign.names.size() - 1) f << ",";
        f << "\n";
    }

    f << "  ]\n";
    f << "}\n";
}

bool game::load_campaign(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::ostringstream ss;
    ss << f.rdbuf();
    std::string src = ss.str();

    int mission_index = 0;
    {
        size_t pos = src.find("\"mission_index\"");
        if (pos != std::string::npos) {
            pos = src.find(':', pos);
            if (pos != std::string::npos) {
                pos++;
                while (pos < src.size() && std::isspace((unsigned char)src[pos])) pos++;
                try { mission_index = std::stoi(src.substr(pos)); }
                catch (...) {}
            }
        }
    }

    std::vector<std::string> names;
    std::vector<bool>        dead_flags;

    size_t search = 0;
    while (true) {
        size_t obj = src.find('{', search);
        if (obj == std::string::npos) break;
        size_t end = src.find('}', obj);
        if (end == std::string::npos) break;
        std::string entry = src.substr(obj, end - obj + 1);
        search = end + 1;

        if (entry.find("mission_index") != std::string::npos) continue;
        if (entry.find("roster")        != std::string::npos) continue;

        size_t npos = entry.find("\"name\"");
        if (npos == std::string::npos) continue;
        npos = entry.find('"', npos + 6);
        if (npos == std::string::npos) continue;
        npos++;
        size_t nend = entry.find('"', npos);
        if (nend == std::string::npos) continue;
        std::string name = entry.substr(npos, nend - npos);

        bool dead = entry.find("\"dead\": true") != std::string::npos ||
                    entry.find("\"dead\":true")  != std::string::npos;

        names.push_back(name);
        dead_flags.push_back(dead);
    }

    if (names.empty()) return false;

    campaign = CampaignState{};
    campaign.mission_index = mission_index;

    struct NamedFactory {
        std::string name;
        std::function<unit(int,int)> factory;
    };
    std::vector<NamedFactory> factories = {
        { "Bosun",        UnitFactory::make_bosun        },
        { "Sharpshooter", UnitFactory::make_sharpshooter },
        { "Medic",        UnitFactory::make_medic        },
        { "Swashbuckler", UnitFactory::make_swashbuckler },
    };

    for (auto& n : names) {
        for (auto& fac : factories) {
            if (fac.name == n) {
                campaign.roster.push_back(fac.factory(0, 0));
                campaign.names.push_back(n);
                break;
            }
        }
    }
    campaign.permanently_dead = dead_flags;

    return true;
}

// ─── campaign ─────────────────────────────────────────────────────────────────

void game::init_campaign() {
    load_enemy_defs("levels/enemies.json");
    if (load_campaign("save.json")) return;

    struct RosterEntry {
        std::function<unit(int,int)> factory;
        std::string                  name;
    };
    std::vector<RosterEntry> entries = {
        { UnitFactory::make_bosun,        "Bosun"        },
        { UnitFactory::make_sharpshooter, "Sharpshooter" },
        { UnitFactory::make_medic,        "Medic"        },
        { UnitFactory::make_swashbuckler, "Swashbuckler" },
    };
    for (auto& e : entries) {
        campaign.roster.push_back(e.factory(0, 0));
        campaign.names.push_back(e.name);
        campaign.permanently_dead.push_back(false);
    }
}

void game::start_mission() {
    reset_mission_state();

    std::vector<std::string> levels = {
        level_dir + "/mutiny_map.tmj"
    };

    std::string path = levels[campaign.mission_index % (int)levels.size()];
    if (!load_level(path)) return;
}

bool game::load_level(const std::string& path) {
    if (!state.map.load(path)) return false;

    auto player_spawns = state.map.get_player_spawns();
    auto enemy_spawns  = state.map.get_enemy_spawns();

    if (player_spawns.empty()) return false;

    std::sort(player_spawns.begin(), player_spawns.end(),
              [](const SpawnPoint& a, const SpawnPoint& b) {
                  return a.type < b.type;
              });

    for (auto& sp : player_spawns) {
        int roster_idx = sp.type - '1';
        if (roster_idx < 0 || roster_idx >= (int)campaign.roster.size()) continue;
        if (campaign.permanently_dead[roster_idx]) continue;

        unit u = campaign.roster[roster_idx];
        u.set_position(sp.col, sp.row);
        u.reset_hp();

        state.players.push_back(u);
        state.player_names.push_back(campaign.names[roster_idx]);
        state.from_roster.push_back(true);
    }

    for (auto& sp : enemy_spawns) {
        for (auto& def : enemy_defs) {
            if (def.spawn_char == sp.type) {
                state.enemies.push_back(enemy(
                    sp.col, sp.row, def.stats,
                    def.id, def.ai_behavior, def.sprite));
                break;
            }
        }
    }

    if (state.players.empty()) return false;

    state.spotted.assign(state.enemies.size(), false);
    state.selected_player = 0;

    if (!state.players.empty()) {
        const unit& active = state.players[0];
        renderer.update_camera(
            active.get_x_pos(),
            active.get_y_pos(),
            state.map.getCols() * config.tile_size);
    }

    update_visibility();
    return true;
}

void game::end_mission() {
    write_back_to_roster();
    campaign.mission_index++;
    save_campaign("save.json");
}

void game::write_back_to_roster() {
    int roster_slot = 0;
    for (int i = 0; i < (int)state.players.size(); i++) {
        if (!state.from_roster[i]) continue;

        while (roster_slot < (int)campaign.permanently_dead.size() &&
               campaign.permanently_dead[roster_slot])
            roster_slot++;

        if (roster_slot >= (int)campaign.roster.size()) break;

        if (!state.players[i].is_alive()) {
            campaign.permanently_dead[roster_slot] = true;
        } else {
            campaign.roster[roster_slot] = state.players[i];
        }

        roster_slot++;
    }
}

void game::reset_mission_state() {
    state = GameState{};
}

// ─── visibility ───────────────────────────────────────────────────────────────

void game::update_visibility() {
    if (state.players.empty()) return;

    for (int i = 0; i < (int)state.enemies.size(); i++) {
        if (state.spotted[i]) continue;
        if (!state.enemies[i].is_alive()) continue;

        int ex = state.enemies[i].get_x_pos();
        int ey = state.enemies[i].get_y_pos();

        for (auto& player : state.players) {
            if (!player.is_alive()) continue;
            int px   = player.get_x_pos();
            int py   = player.get_y_pos();
            int sr   = player.get_sight_range();
            int dist = std::max(abs(ex - px), abs(ey - py));

            if (dist <= sr && has_los(px, py, ex, ey, state.map)) {
                state.spotted[i] = true;
                break;
            }
        }
    }
}

// ─── win conditions ───────────────────────────────────────────────────────────

void game::check_win_conditions() {
    if (state.win_state != WinState::ONGOING) return;

    bool any_alive = false;
    for (auto& p : state.players)
        if (p.is_alive()) { any_alive = true; break; }
    if (!any_alive) {
        state.win_state = WinState::DEFEAT;
        return;
    }

    const auto& objectives = state.map.get_objectives();
    if (objectives.empty()) return;

    for (auto& obj : objectives) {
        bool met = false;

        if (obj.type == Objective::Type::KILL_UNIT) {
            bool any_target_alive = false;
            for (auto& e : state.enemies) {
                if (!e.is_alive()) continue;
                if (e.get_type_id() == obj.target)
                    any_target_alive = true;
            }
            met = !any_target_alive;
        }
        else if (obj.type == Objective::Type::HOLD_TILE) {
            for (auto& p : state.players) {
                if (!p.is_alive()) continue;
                if (state.map.is_objective(p.get_x_pos(), p.get_y_pos()))
                    met = true;
            }
        }

        if (!met) return;
    }

    state.win_state = WinState::VICTORY;
    end_mission();
}

// ─── update / draw ────────────────────────────────────────────────────────────

void game::update(float dt) {
    if (state.win_state != WinState::ONGOING) {
        if (state.win_state == WinState::DEFEAT && IsKeyPressed(KEY_R)) {
            std::remove("save.json");
            campaign = CampaignState{};
            enemy_defs.clear();
            init_campaign();
            start_mission();
        }
        return;
    }

    state.floating_texts.update(dt);

    const Camera2D& cam = renderer.get_camera();
    input.update_preview(state, cam);

    if (state.phase == GamePhase::PLAYER_TURN) {
        auto intent = input.poll(state, cam);
        if (intent.has_value()) {
            if (intent->type == IntentType::SelectUnit) {
                state.selected_player = intent->value;
                state.mode            = ActionMode::NONE;
                state.preview         = {};
            } else {
                turns.apply_player_intent(intent.value(), state);
            }
        }

        if (!state.players.empty()) {
            const unit& active = state.players[state.selected_player];
            renderer.update_camera(
                active.get_x_pos(),
                active.get_y_pos(),
                state.map.getCols() * config.tile_size);
        }

        update_visibility();
        check_win_conditions();
    }

    if (state.phase == GamePhase::ENEMY_TURN) {
        turns.update_enemy_turn(dt, state, ai);

        if (!state.players.empty()) {
            const unit& active = state.players[state.selected_player];
            renderer.update_camera(
                active.get_x_pos(),
                active.get_y_pos(),
                state.map.getCols() * config.tile_size);
        }

        update_visibility();
        check_win_conditions();
    }

    if (state.mode == ActionMode::SHOOT     ||
        state.mode == ActionMode::MELEE     ||
        state.mode == ActionMode::HEAL      ||
        state.mode == ActionMode::DIRTY_TRICK)
        SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
    else
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

void game::draw() {
    renderer.draw_frame(state);
}

void game::run() {
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        update(dt);
        draw();
    }
}