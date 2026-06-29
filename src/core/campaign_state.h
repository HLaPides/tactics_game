#pragma once
#include "../units/unit.h"
#include <vector>
#include <string>

struct CampaignState {
    std::vector<unit>        roster;
    std::vector<std::string> names;
    std::vector<bool>        permanently_dead;
    int                      mission_index = 0;

    bool is_wiped() const {
        for (int i = 0; i < (int)roster.size(); i++) {
            if (!permanently_dead[i]) return false;
        }
        return true;
    }

    int living_count() const {
        int count = 0;
        for (auto dead : permanently_dead)
            if (!dead) count++;
        return count;
    }
};