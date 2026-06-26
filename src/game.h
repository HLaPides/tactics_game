#pragma once
#include "game_state.h"
#include "input_handler.h"
#include "turn_manager.h"
#include "ai_controller.h"
#include "renderer.h"
#include "types.h"
#include <string>

class game {
public:
    game(const std::string& level_dir, const AppConfig& config);
    void run();
private:
    AppConfig      config;
    GameState      state;
    InputHandler   input;
    TurnManager    turns;
    AIController   ai;
    Renderer       renderer;

    bool load_level(const std::string& level_dir);
    void update_visibility();
    void update(float dt);
    void draw();
};