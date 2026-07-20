#include "enemy.h"

enemy::enemy(int x_pos, int y_pos, UnitStats stats,
             const std::string& type_id, const std::string& ai_behavior,
             const std::string& sprite_key)
    : unit(x_pos, y_pos, stats)
    , type_id(type_id)
    , ai_behavior(ai_behavior)
    , sprite_key(sprite_key)
{}

const std::string& enemy::get_type_id()     const { return type_id; }
const std::string& enemy::get_ai_behavior() const { return ai_behavior; }
const std::string& enemy::get_sprite_key()  const { return sprite_key; }