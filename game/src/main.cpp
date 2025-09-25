/*
This project uses the Raylib framework to provide us functionality for math, graphics, GUI, input etc.
See documentation here: https://www.raylib.com/, and examples here: https://www.raylib.com/examples.html
*/

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "game.h"
#include <string>
#include <vector>

const unsigned int TARGET_FPS = 50; //frames/second
float dt = 1.0f / TARGET_FPS; //seconds/frame
float time = 0;

class FizziksObjekt
{
public:
	Vector2 position = { 0,0 };
	Vector2 velocity = { 0,0 };
	float mass = 1; // in kg

	float radius = 15; // circle radius in pixels
	std::string name = "objekt";
	Color color = RED;

	void draw()
	{
		DrawCircle(position.x, position.y, radius, color);

		DrawText(name.c_str(), position.x, position.y, radius * 2, LIGHTGRAY);

		//Draw velocity (for fun)
		DrawLineEx(position, position + velocity, 1, color);
	}
};

class FizziksWorld
{
private:
	unsigned int objektCount = 0;
public: 
	std::vector<FizziksObjekt> objekts; // All objects in physics simulation
	Vector2 accelerationGravity = {0, 9};

	void add(FizziksObjekt newObject) // Add to physics simulation
	{
		newObject.name = std::to_string(objektCount);
		objekts.push_back(newObject);
		objektCount++;
	}

	// Update state of all physics objects
	void update()
	{
		for (int i = 0; i < objekts.size(); i++)
		{
			//vel = change in position / time, therefore     change in position = vel * time 
			objekts[i].position = objekts[i].position + objekts[i].velocity * dt;
			//accel = deltaV / time (change in velocity over time) therefore     deltaV = accel * time
			objekts[i].velocity = objekts[i].velocity + accelerationGravity * dt;
		}
	}
};

float speed = 100;
float angle = 0;

FizziksWorld world;

//Remove objects offscreen
void cleanup()
{
	//For each object, check if it is offscreen!
	for (int i = 0; i < world.objekts.size(); i++)
	{
		//Is it offscreen?
		if (	world.objekts[i].position.y > GetScreenHeight()
			||	world.objekts[i].position.y < 0
			||  world.objekts[i].position.x > GetScreenWidth()
			||  world.objekts[i].position.x < 0
			)
		{
			//Destroy!
			world.objekts.erase(world.objekts.begin() + i);
			i--;
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
		FizziksObjekt newBird;
		newBird.position = { 100, (float)GetScreenHeight() - 100 };
		newBird.velocity = { speed * (float)cos(angle * DEG2RAD), -speed * (float)sin(angle * DEG2RAD) };
		
		//rand() % N produces random number from 0 to N-1
		newBird.radius = (rand() % 26) + 5; // radius from 5-30
		Color randomColor = {rand() % 256 , rand() % 256, rand() % 256, 255};
		newBird.color = randomColor;

		world.add(newBird); // Add bird to simulation
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

	DrawText(TextFormat("Obects: %i", world.objekts.size()), 10, 160, 30, LIGHTGRAY);

	DrawText(TextFormat("T: %6.2f", time), GetScreenWidth() - 140, 10, 30, LIGHTGRAY);

	Vector2 startPos = {100, GetScreenHeight() - 100};
	Vector2 velocity = {speed * cos(angle * DEG2RAD), -speed * sin(angle * DEG2RAD)};

	DrawLineEx(startPos, startPos + velocity, 3, RED);

	//Draw all physics objects!
	for (int i = 0; i < world.objekts.size(); i++)
	{
		world.objekts[i].draw();
	}

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