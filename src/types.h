#pragma once
#include <string>
#include <utility>

enum class GamePhase  { PLAYER_TURN, ENEMY_TURN };

enum class ActionMode {
    NONE,
    SHOOT,
    MELEE,
    HEAL,
    OVERWATCH,
    DIRTY_TRICK,
    AIMED_SHOT,
    RUSH
};

enum class IntentType {
    Move,
    Shoot,
    Melee,
    Heal,
    Overwatch,
    DirtyTrick,
    AimedShot,
    Rush,
    EndTurn,
    Cancel,
    SelectUnit
};

struct Intent {
    IntentType type;
    int        target_x = 0;
    int        target_y = 0;
    int        value    = 0;
};

struct UnitStats {
    int movement     = 3;
    int hp           = 5;
    int aim          = 60;
    int defense      = 10;
    int shoot_range  = 4;
    int shoot_damage = 1;
    int melee_damage = 1;
    int sight_range  = 8;
};

struct AppConfig {
    int screen_w  = 1280;
    int screen_h  = 720;
    int grid_h    = 640;
    int tile_size = 32;
};

namespace UnitPresets {
    inline UnitStats Bosun()        { return { 4, 10, 55, 10,  3, 1, 3,  8 }; }
    inline UnitStats Sharpshooter() { return { 3,  5, 85,  5, 10, 2, 1, 12 }; }
    inline UnitStats Medic()        { return { 3,  6, 60,  5,  4, 1, 1,  8 }; }
    inline UnitStats Swashbuckler() { return { 3,  6, 65,  8,  5, 1, 2,  8 }; }
    inline UnitStats Soldier()      { return { 4,  4, 60,  5,  4, 1, 1,  6 }; }
    inline UnitStats Guard()        { return { 5,  5, 50,  8,  2, 1, 2,  6 }; }
    inline UnitStats Captain()      { return { 4, 10, 70, 10,  5, 2, 3, 10 }; }
}