#include "floating_text.h"
#include <algorithm>

void FloatingTextManager::spawn(int col, int row, const std::string& text, Color color, float duration) {
    texts.push_back({ col, row, 0.0f, duration, text, color });
}

void FloatingTextManager::update(float dt) {
    for (auto& ft : texts)
        ft.timer += dt;
    texts.erase(
        std::remove_if(texts.begin(), texts.end(),
            [](const FloatingText& ft) { return ft.timer >= ft.duration; }),
        texts.end());
}

const std::vector<FloatingText>& FloatingTextManager::get_all() const {
    return texts;
}