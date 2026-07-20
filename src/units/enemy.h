#pragma once
#include "unit.h"
#include <string>

class enemy : public unit {
public:
    enemy(int x_pos, int y_pos, UnitStats stats,
          const std::string& type_id, const std::string& ai_behavior,
          const std::string& sprite_key);

    const std::string& get_type_id()     const;
    const std::string& get_ai_behavior() const;
    const std::string& get_sprite_key()  const;

private:
    std::string type_id;
    std::string ai_behavior;
    std::string sprite_key;
};