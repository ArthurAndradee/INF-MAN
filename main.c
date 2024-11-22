#include "raylib.h"

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Janela básica da raylib");

    SetTargetFPS(60);             

    while (!WindowShouldClose())  
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawText("Hello world!", 630, 384, 20, BLACK);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}