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
    int grid_h    = 620;   // reduced to account for taller HUD
    int tile_size = 32;
};

namespace UnitPresets {
    //Movement, Health, Aim, Defense, Shooting range, shooting damage, meele damage, sight
    inline UnitStats Bosun()        { return {  6, 10,  55,  10,  3,   1,   3,   8 }; }
    inline UnitStats Sharpshooter() { return {  4,  5,  85,   5, 10,   2,   1,  12 }; }
    inline UnitStats Medic()        { return {  4,  6,  60,   5,  4,   1,   1,   8 }; }
    inline UnitStats Swashbuckler() { return {  5,  6,  65,   8,  5,   1,   2,   8 }; }
    inline UnitStats Soldier()      { return {  4,  4,  60,   5,  4,   1,   1,   6 }; }
    inline UnitStats Guard()        { return {  5,  5,  50,   8,  2,   1,   2,   6 }; }
    inline UnitStats Captain() { return { 5, 6, 75, 5, 7, 3, 4, 8 }; }
}