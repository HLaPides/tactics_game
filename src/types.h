#pragma once
#include <unordered_map>
#include <string>
#include <utility>

enum class GamePhase  { PLAYER_TURN, ENEMY_TURN };
enum class ActionMode { NONE, SHOOT, MELEE, HEAL };

enum class IntentType { Move, Shoot, Melee, Heal, EndTurn, Cancel, SelectUnit };

struct Intent {
    IntentType type;
    int        target_x = 0;
    int        target_y = 0;
    int        value    = 0;  // used for SelectUnit — holds player index
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
    inline UnitStats Bosun()        { return { 4, 10, 55, 20,  3, 1, 3,  8 }; }
    inline UnitStats Sharpshooter() { return { 3,  5, 85, 10, 10, 2, 1, 12 }; }
    inline UnitStats Medic()        { return { 3,  6, 60, 15,  4, 1, 1,  8 }; }
    inline UnitStats Swashbuckler() { return { 3,  6, 65, 15,  5, 1, 2,  8 }; }
    inline UnitStats Soldier()      { return { 2,  4, 60, 10,  4, 1, 1,  6 }; }
    inline UnitStats Guard()        { return { 3,  5, 50, 15,  2, 1, 2,  6 }; }
    inline UnitStats Captain()      { return { 3, 10, 70, 20,  5, 2, 3, 10 }; }
}