/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

by Jeffery Myers is marked with CC0 1.0. To view a copy of this license, visit https://creativecommons.org/publicdomain/zero/1.0/

*/

#include "raylib.h"
#include "map.h"
#include "units/unit.h"
#include <algorithm>

#include "resource_dir.h"	// utility header for SearchAndSetResourceDir

int main ()
{
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

	// Create the window and OpenGL context
	InitWindow(800, 800, "Xcom Knockoff");
	map game_map = map(640,640,64);
	unit test_unit = unit(1,2,1);
	int tile_size = game_map.get_tile_size();
	
	// game loop
	while (!WindowShouldClose())		// run the loop until the user presses ESCAPE or presses the Close button on the window
	{
		// drawing
		BeginDrawing();
		ClearBackground(DARKGRAY);
		game_map.draw_map();
		test_unit.draw(game_map.get_tile_size());
		test_unit.draw_range(tile_size, game_map.get_x_dimension(),game_map.get_y_dimension());
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			Vector2 mouse_pos = GetMousePosition();
			int clicked_col = mouse_pos.x/tile_size;
			int clicked_row = mouse_pos.y/tile_size;
			int dist = std::max(abs(clicked_col - test_unit.get_x_pos()), abs(clicked_row - test_unit.get_y_pos()));
			if (dist <= test_unit.get_movement()) {
				test_unit.set_position(clicked_col, clicked_row);
			}
		}
		EndDrawing();

	}

	CloseWindow();
	return 0;
}
