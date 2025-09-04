// This project uses the Raylib ramework to handle graphics 
// Link: https://www.raylib.com/examples.html

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "game.h" // <-- Bsically uses a copy paste feature but files 

const unsigned int TARGET_FPS = 50; // 1/5
float time = 0;
float dt;
float x = 500;
float y = 500;
float frequency = 100;
float amplitude = 70;
//

void update()
{
    dt = 1.0 / TARGET_FPS;
    time += dt; // 1.0 / TARGET_FPS <-- const


    y = y + (cos(time * frequency)) * frequency * amplitude * dt;
    x = x + (-sin(time * frequency)) * frequency * amplitude * dt;

}

void draw()
{
    BeginDrawing();
    ClearBackground(BLUE);
    DrawText("Earl Fabian 101554213", 10, 100, 20, BLACK);


    GuiSliderBar(Rectangle{ 60, 5, 1000, 10 }, "Time", TextFormat("%.2f", time), &time, 0, 240);
    DrawText(TextFormat("FPS: %i, TIME : %.2f", TARGET_FPS,  time), GetScreenWidth() - 400, 15, 20, BLACK);


    DrawCircle(x, y, 60, RED);
    DrawCircle(GetScreenWidth() / 2 + cos (time * frequency) * amplitude, GetScreenWidth() / 2 + sin(time * frequency) * amplitude, 60, GREEN);

    EndDrawing();
}

int main()
{
    InitWindow(InitialWidth, InitialHeight, "Game Physics: Earl Fabian 101554213");
    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose())
    {
        update();
        draw();
    }

    CloseWindow();
    return 0;
}


