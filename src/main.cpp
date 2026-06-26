#include "raylib.h"
#include "game.h"
#include "types.h"

int main() {
    AppConfig config;
    InitWindow(config.screen_w, config.screen_h, "Xcom Knockoff");
    PollInputEvents();

    game game("levels", config);
    game.run();

    CloseWindow();
    return 0;
}