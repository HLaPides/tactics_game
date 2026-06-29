#pragma once
#include "raylib.h"
#include <string>
#include <vector>

struct FloatingText {
    int         col, row;    // grid position — renderer converts to screen
    float       timer;
    float       duration;
    std::string text;
    Color       color;
};

class FloatingTextManager {
public:
    void spawn(int col, int row, const std::string& text, Color color, float duration = 1.0f);
    void update(float dt);
    const std::vector<FloatingText>& get_all() const;
private:
    std::vector<FloatingText> texts;
};