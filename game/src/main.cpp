/*
This project uses the Raylib framework to provide us functionality for math, graphics, GUI, input etc.
See documentation here: https://www.raylib.com/, and examples here: https://www.raylib.com/examples.html
*/

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "game.h"
#include <vector>
#include <string>
const int OFFSCREEN_DESPAWN_ZONE_PADDING = 300;
const unsigned int TARGET_FPS = 50; //frames/second
float dt = 1.0f / TARGET_FPS; //seconds/frame
float time = 0;

class FizzikzObjekt
{
public:

	Vector2 position = { 0,0 };
	Vector2 velocity = { 0,0 };
	float mass = 1; // kg
	float drag = 0; // ???


	std::string name = "objekt";
	float radius = 15; // radius of circle in pixels
	Color color = RED;

	void draw()
	{
		DrawCircle(position.x, position.y, radius, color);
		DrawLineEx(position, position + velocity, 2, color);
		DrawText(name.c_str(), position.x - radius / 2, position.y - radius / 2, radius, LIGHTGRAY);
	}
};

class FizziksWorld
{
private:
	unsigned int globalCounter = 0;
public:
	std::vector<FizzikzObjekt> objekts; // all physics objects in the game world
	Vector2 accelerationGravity = { 0, 9 };

	void add(FizzikzObjekt objekt)
	{
		objekt.name = std::to_string(globalCounter);
		objekts.push_back(objekt);
		globalCounter++;
	}

	void update()
	{
		//Update all positions and velocities of physics objects
		for (int i = 0; i < objekts.size(); i++)
		{
			FizzikzObjekt& objektRef = objekts[i];

			// a * b --> multiplication
			// *a --> dereference
			// TYPE* a --> declaring a pointer

			//vel = change in position / time, therefore     change in position = vel * time 
			objektRef.position = objektRef.position + objektRef.velocity * dt;

			//accel = deltaV / time (change in velocity over time) therefore     deltaV = accel * time
			objektRef.velocity = objektRef.velocity + accelerationGravity * dt;
		}
	}

	void draw()
	{
		//Draw all physics objects
		for (int i = 0; i < objekts.size(); i++)
		{
			FizzikzObjekt& objektRef = objekts[i];
			objektRef.draw();
		}
	}
};

float speed = 100;
float angle = 0;

FizziksWorld world;

//Destroy objects no longer in the screen
void cleanup()
{
	//Using Iterators to traverse a container like std::vector 
	// allows us to not worry about indices, just about what is next
	for (std::vector< FizzikzObjekt>::iterator iter = world.objekts.begin(); 
		iter != world.objekts.end();)
	{
		FizzikzObjekt &objekt = *iter; // *iter will give us the object the iterator points at
		//position out of screen...
		if (	objekt.position.y > GetScreenHeight() + OFFSCREEN_DESPAWN_ZONE_PADDING
			||	objekt.position.y < -OFFSCREEN_DESPAWN_ZONE_PADDING
			||	objekt.position.x < -OFFSCREEN_DESPAWN_ZONE_PADDING
			||	objekt.position.x > GetScreenWidth() + OFFSCREEN_DESPAWN_ZONE_PADDING
			)
		{
			iter = world.objekts.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}

//Changes world state
void update()
{
	dt = 1.0f / TARGET_FPS;
	time += dt;

	cleanup();

	world.update();

	if (IsKeyPressed(KEY_SPACE))
	{
		FizzikzObjekt bird;
		bird.position = { 100, (float)GetScreenHeight() - 100 };
		bird.velocity = { speed * (float)cos(angle * DEG2RAD), -speed * (float)sin(angle * DEG2RAD) };
		bird.radius = float((rand() % 15) + 5);
		bird.color = Color{ unsigned char(rand() % 255), unsigned char(rand() % 255), unsigned char(rand() % 255), 255};

		world.add(bird);
	}
}

//Display world state
void draw()
{
	BeginDrawing();
	ClearBackground(BLACK);
	DrawText("Joss Moo-Young 123456789", 10, float(GetScreenHeight() - 30), 20, LIGHTGRAY);


	GuiSliderBar(Rectangle{ 10, 15, 1000, 20 }, "", TextFormat("%.2f", time), &time, 0, 240);

	GuiSliderBar(Rectangle{ 10, 40, 500, 30 }, "Speed", TextFormat("Speed: %.0f", speed), &speed, -1000, 1000);

	GuiSliderBar(Rectangle{ 10, 80, 500, 30 }, "Angle", TextFormat("Angle: %.0f Degrees", angle), &angle, -180, 180);

	GuiSliderBar(Rectangle{ 10, 120, 500, 30 }, "Gravity Y", TextFormat("Gravity Y: %.0f Px/sec^2", world.accelerationGravity.y), &world.accelerationGravity.y, -1000, 1000);

	DrawText(TextFormat("Objects: %i", world.objekts.size()), 10, 160, 30, LIGHTGRAY);


	DrawText(TextFormat("T: %6.2f", time), GetScreenWidth() - 140, 10, 30, LIGHTGRAY);

	Vector2 startPos = {100, GetScreenHeight() - 100};
	Vector2 velocity = {speed * cos(angle * DEG2RAD), -speed * sin(angle * DEG2RAD)};

	DrawLineEx(startPos, startPos + velocity, 3, RED);


	world.draw();

	

	EndDrawing();

}

int main()
{
	InitWindow(InitialWidth, InitialHeight, "GAME2005 Joss Moo-Young 123456789");
	SetTargetFPS(TARGET_FPS);

	while (!WindowShouldClose()) // Loops TARGET_FPS times per second
	{
		update();
		draw();
	}

	CloseWindow();
	return 0;
}