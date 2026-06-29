#pragma once
#include "game_state.h"
#include "ui/input_handler.h"
#include "turn/turn_manager.h"
#include "ai/ai_controller.h"
#include "ui/renderer.h"
#include "types.h"
#include <string>

class game {
public:
    game(const std::string& level_dir, const AppConfig& config);
    void run();
private:
    AppConfig    config;
    GameState    state;
    InputHandler input;
    TurnManager  turns;
    AIController ai;
    Renderer     renderer;

    bool load_level(const std::string& level_dir);
    void update_visibility();
    void update_camera();
    void check_win_conditions();
    void update(float dt);
    void draw();
};