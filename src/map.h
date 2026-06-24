#pragma once

class map {
public:
    map(int x, int y, int tile_size);
    void draw_map();
    int get_tile_size();
    int get_x_dimension();
    int get_y_dimension();
    int getCols();
    int getRows();
private:
    int x_dimension = 1280;
    int y_dimension = 1280;
    int tile_size = 64;
};