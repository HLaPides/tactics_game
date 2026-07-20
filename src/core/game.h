#pragma once
#include "game_state.h"
#include "campaign_state.h"
#include "../ui/input_handler.h"
#include "../turn/turn_manager.h"
#include "../ai/ai_controller.h"
#include "../ui/renderer.h"
#include "../units/enemy.h"
#include "../core/types.h"
#include <string>
#include <vector>

class game {
public:
    game(const std::string& level_dir, const AppConfig& config);
    void run();
private:
    AppConfig      config;
    CampaignState  campaign;
    GameState      state;
    InputHandler   input;
    TurnManager    turns;
    AIController   ai;
    Renderer       renderer;
    std::string    level_dir;

    struct EnemyDef {
        std::string id;
        std::string sprite;
        std::string ai_behavior;
        char        spawn_char;
        UnitStats   stats;
    };
    std::vector<EnemyDef> enemy_defs;
    void load_enemy_defs(const std::string& path);

    void init_campaign();
    bool load_level(const std::string& path);
    void start_mission();
    void end_mission();
    void write_back_to_roster();
    void reset_mission_state();
    void save_campaign(const std::string& path) const;
    bool load_campaign(const std::string& path);

    void update_visibility();
    void check_win_conditions();
    void update(float dt);
    void draw();
};