#pragma once

class unit {
public:
    unit(int x_pos, int y_pos, int mvmt, int hp, int aim, int defense, int shoot_range, int shoot_dmg, int melee_dmg);
    int  get_movement();
    int  get_x_pos();
    int  get_y_pos();
    int  get_actions();
    int  get_hp();
    int  get_max_hp();
    int  get_aim();
    int  get_defense();
    int  get_shoot_range();
    int  get_shoot_damage();
    int  get_melee_damage();
    bool is_alive();
    void use_action();
    void reset_actions();
    void take_damage(int amount);
    void draw(int tile_size);
    void draw_range(int tile_size, int cols, int rows);
    void set_position(int x, int y);
private:
    int x_position;
    int y_position;
    int movement;
    int actions_remaining;
    int hp;
    int max_hp;
    int aim;
    int defense;
    int shoot_range;
    int shoot_damage;
    int melee_damage;
    static const int MAX_ACTIONS = 2;
};