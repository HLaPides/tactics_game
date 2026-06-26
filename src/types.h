#pragma once

enum class GamePhase  { PLAYER_TURN, ENEMY_TURN };
enum class ActionMode { NONE, SHOOT, MELEE };

enum class IntentType { Move, Shoot, Melee, EndTurn, Cancel };

struct Intent {
    IntentType type;
    int        target_x = 0;
    int        target_y = 0;
};

struct UnitStats {
    int movement     = 3;
    int hp           = 5;
    int aim          = 60;
    int defense      = 10;
    int shoot_range  = 4;
    int shoot_damage = 1;
    int melee_damage = 1;
};

struct AppConfig {
    int screen_w  = 1280;
    int screen_h  = 720;
    int grid_h    = 640;
    int tile_size = 32;
};