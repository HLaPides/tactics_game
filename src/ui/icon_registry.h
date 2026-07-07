#pragma once
#include "raylib.h"
#include <string>
#include <unordered_map>

class IconRegistry {
public:
    ~IconRegistry();
    void      load(const std::string& ability_id, const std::string& filepath);
    bool      has(const std::string& ability_id) const;
    Texture2D get(const std::string& ability_id) const;
private:
    std::unordered_map<std::string, Texture2D> icons;
};