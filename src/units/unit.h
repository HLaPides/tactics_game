#pragma once

class unit {
    public:
        unit(int x_pos, int y_pos);
        void draw(int tile_size);
        void set_position(int x, int y);
    private:
        int x_position;
        int y_position;
};