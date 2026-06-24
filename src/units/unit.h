#pragma once

class unit {
    public:
        unit(int x_pos, int y_pos, int mvmt);
        int get_movement();
        int get_x_pos();
        int get_y_pos();
        void draw(int tile_size);
        void draw_range(int tile_size, int cols, int rows);
        void set_position(int x, int y);
    private:
        int x_position;
        int y_position;
        int movement;
};