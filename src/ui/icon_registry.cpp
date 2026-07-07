#include "icon_registry.h"

IconRegistry::~IconRegistry() {
    for (auto& [id, tex] : icons)
        UnloadTexture(tex);
}

void IconRegistry::load(const std::string& ability_id, const std::string& filepath) {
    Texture2D tex = LoadTexture(filepath.c_str());
    if (tex.id != 0)
        icons[ability_id] = tex;
}

bool IconRegistry::has(const std::string& ability_id) const {
    return icons.find(ability_id) != icons.end();
}

Texture2D IconRegistry::get(const std::string& ability_id) const {
    auto it = icons.find(ability_id);
    return it != icons.end() ? it->second : Texture2D{};
}