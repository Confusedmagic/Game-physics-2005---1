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

enum FizziksShape
{
	CIRCLE,
	HALF_SPACE
};

class FizziksObjekt
{
public:
	bool isStatic = false; // if this is set to true, don't move object according to velocity or gravity
	Vector2 position = { 0,0 };
	Vector2 velocity = { 0,0 };
	float mass = 1; // in kg

	std::string name = "objekt";
	Color color = RED;

	virtual void draw() // virtual keyword is required to allow this function to be overriden
	{
		DrawCircle(position.x, position.y, 2, color);
		//DrawText(name.c_str(), position.x, position.y, 12, LIGHTGRAY);
	}

	virtual FizziksShape Shape() = 0; // This is a pure virtual, or "abstract" function. 
	//It is a declaration that has no definition itself, but must be defined in child classes.
};

class FizziksCircle : public FizziksObjekt
{
public:
	float radius; // circle radius in pixels

	void draw() override // if we want to override a parent class function, 
		// the signature (name, return type, parameter list) must match exactly
		// the override keyword makes sure you are actually overriding something. 
		// If you are not (i.e. you did it wrong) it will tell you by making a compile-time error
	{
		DrawCircle(position.x, position.y, radius, color);

		DrawText(name.c_str(), position.x, position.y, radius * 2, LIGHTGRAY);

		//Draw velocity (for fun)
		DrawLineEx(position, position + velocity, 1, color);
	}

	FizziksShape Shape() override
	{
		return CIRCLE;
	}
};

class FizziksHalfspace : public FizziksObjekt
{
private:
	//FizziksObjekt::position in this context represents an arbitrary point that lies on the line.
	float rotation = 0;
	Vector2 normal = { 0, -1 }; // normal vector represents the direction perpendicular to the surface, pointing away from it.
	//We always keep normal vectors at a magnitude of 1, so they denote orrientation, but no magnitude

public:
	void setRotationDegrees(float rotationInDegrees)
	{
		rotation = rotationInDegrees;
		normal = Vector2Rotate({ 0, -1 }, rotation * DEG2RAD);
	}

	float getRotation()
	{
		return rotation;
	}

	Vector2 getNormal()
	{
		return normal;
	}

	void draw() override
	{
		//Draw arbitrary point on the line
		DrawCircle(position.x, position.y, 8, color);

		//Draw normal vector, perpendicular to the surface
		DrawLineEx(position, position + normal * 30, 1, color);

		//Draw the line/surface
		//Rotate function takes radians. 360 degrees = 2PI radians 
		Vector2 parallelToSurface = Vector2Rotate(normal, PI * 0.5f);
		DrawLineEx(position - parallelToSurface * 4000, position + parallelToSurface * 4000, 1, color);
	}

	FizziksShape Shape() override
	{
		return HALF_SPACE;
	}
};

bool CircleCircleOverlap(FizziksCircle* circleA, FizziksCircle* circleB) // returns true if circles are overlapping
{
	Vector2 displacementFromAToB = circleB->position - circleA->position;
	float distance = Vector2Length(displacementFromAToB);//Use pythagorean theorem to get magnitude of displacement vector between circles to get a distance
	float sumOfRadii = circleA->radius + circleB->radius;
	

	if (sumOfRadii > distance)
	{
		return true; //overlapping
	}
	else
		return false; // not overlapping
}


// moves objects apart if overlapping
bool CircleCircleCollisionResponse(FizziksCircle* circleA, FizziksCircle* circleB) // returns true if circles are overlapping
{
	Vector2 displacementFromAToB = circleB->position - circleA->position;
	float distance = Vector2Length(displacementFromAToB);//Use pythagorean theorem to get magnitude of displacement vector between circles to get a distance
	float sumOfRadii = circleA->radius + circleB->radius;
	float overlap = sumOfRadii - distance;

	if (overlap > 0)
	{
		Vector2 normalAtoB;
		if (abs(distance) < 0.0001f)
			normalAtoB = { 0,1 };
		else
			normalAtoB = displacementFromAToB / distance;
		Vector2 mtv = normalAtoB * overlap; //minimum translation vector. Shortest distance/direction I need to move bu so the objects no longer overlap

		circleA->position -= mtv * 0.5f;
		circleB->position += mtv * 0.5f;
		return true; //overlapping
	}
	else
		return false; // not overlapping
}

// Returns true if the circle overlaps the halfspace, false otherwise
bool CircleHalfspaceOverlap(FizziksCircle* circle, FizziksHalfspace* halfspace) // returns true if circles are overlapping
{
	//Get a displacement vector FROM the arbitrary point on the halfspace TO the circle
	Vector2 displacementToCircle = circle->position - halfspace->position;
	//Let D be the DOT PRODUCT of this displacement and the Normal vector.
	//If D < 0, circle is behind it and overlapping, if D > 0, circle is in front. If 0 < D < circle.radius, overlapping
	//In other words... return (D < radius)
	//return (Dot(displacement, normal) < radius)

	float dot = Vector2DotProduct(displacementToCircle, halfspace->getNormal());
	/*
	* Vector projection Proj a onto b(both are vectors) =
		a dot b * (b/||b||)
		If b is already normalized, we don't need to bother dividing by its magnitude
	*/
	Vector2 projectionDisplacementOntoNormal = halfspace->getNormal() * dot;

	DrawLineEx(circle->position, circle->position - projectionDisplacementOntoNormal, 1, GRAY);
	Vector2 midpoint = circle->position - projectionDisplacementOntoNormal * 0.5f;
	DrawText(TextFormat("D: %6.0f", dot), midpoint.x, midpoint.y, 30, GRAY);

	return dot < circle->radius;
}

bool CircleHalfspaceCollisionResponse(FizziksCircle* circle, FizziksHalfspace* halfspace) // returns true if circles are overlapping
{

	Vector2 displacementToCircle = circle->position - halfspace->position;
	float dot = Vector2DotProduct(displacementToCircle, halfspace->getNormal());
	Vector2 projectionDisplacementOntoNormal = halfspace->getNormal() * dot;
	float overlap = circle->radius - dot;
	if (overlap > 0)
	{
		Vector2 mtv = halfspace->getNormal() * overlap;
		circle->position += mtv;
		return true;
	}
	else
	{
		return false;
	}

	//DrawLineEx(circle->position, circle->position - projectionDisplacementOntoNormal, 1, GRAY);
	//Vector2 midpoint = circle->position - projectionDisplacementOntoNormal * 0.5f;
	//DrawText(TextFormat("D: %6.0f", dot), midpoint.x, midpoint.y, 30, GRAY);

	return dot < circle->radius;
}

class FizziksWorld
{
private:
	unsigned int objektCount = 0;
public:
	std::vector<FizziksObjekt*> objekts; // All objects in physics simulation

	Vector2 accelerationGravity = { 0, 0 };

	void add(FizziksObjekt* newObject) // Add to physics simulation
	{
		newObject->name = std::to_string(objektCount);
		objekts.push_back(newObject);
		objektCount++;
	}

	// Update state of all physics objects
	void update()
	{
		for (int i = 0; i < objekts.size(); i++)
		{
			FizziksObjekt* objekt = objekts[i];

			// Avoid modifying position and applying gravity to objects we label "static"
			if (objekt->isStatic) continue;

			//vel = change in position / time, therefore     change in position = vel * time 
			objekt->position = objekt->position + objekt->velocity * dt;
			//accel = deltaV / time (change in velocity over time) therefore     deltaV = accel * time
			objekt->velocity = objekt->velocity + accelerationGravity * dt;
		}

		checkCollisions();
	}

	void checkCollisions()
	{
		//Start by painting everything green. When they touch they will be turned red and stay that way
		for (int i = 0; i < objekts.size(); i++)
		{
			objekts[i]->color = GREEN;
		}

		//assuming all objects in objekts are circles...
		//for each object...
		for (int i = 0; i < objekts.size(); i++)
		{
			//check against another object...
			for (int j = i + 1; j < objekts.size(); j++)
			{
				FizziksObjekt* objektPointerA = objekts[i];
				FizziksObjekt* objektPointerB = objekts[j];

				//Ask objects what shape they are
				FizziksShape shapeOfA = objektPointerA->Shape();
				FizziksShape shapeOfB = objektPointerB->Shape();

				bool didOverlap = false;

				//
				if (shapeOfA == CIRCLE && shapeOfB == CIRCLE)
				{
					didOverlap = CircleCircleCollisionResponse((FizziksCircle*)objektPointerA, (FizziksCircle*)objektPointerB);
				}
				//If one is a circle and one is a halfspace
				else if (shapeOfA == CIRCLE && shapeOfB == HALF_SPACE)
				{
					didOverlap = CircleHalfspaceCollisionResponse((FizziksCircle*)objektPointerA, (FizziksHalfspace*)objektPointerB);
				}
				else if (shapeOfA == HALF_SPACE && shapeOfB == CIRCLE)
				{
					didOverlap = CircleHalfspaceCollisionResponse((FizziksCircle*)objektPointerB, (FizziksHalfspace*)objektPointerA);
				}

				if (didOverlap)
				{
					objektPointerA->color = RED;
					objektPointerB->color = RED;
				}
			}
		}
	}
};

float speed = 0;
float angle = 0;
Vector2 startPos = { 100, 500 };

FizziksWorld world;
FizziksHalfspace halfspace;
FizziksHalfspace halfspace2;

//Remove objects offscreen
void cleanup()
{
	//For each object, check if it is offscreen!
	for (int i = 0; i < world.objekts.size(); i++)
	{
		FizziksObjekt* objekt = world.objekts[i];
		//Is it offscreen?
		if (objekt->position.y > GetScreenHeight()
			|| objekt->position.y < 0
			|| objekt->position.x > GetScreenWidth()
			|| objekt->position.x < 0
			)
		{
			//Destroy!
			std::vector<FizziksObjekt*>::iterator iterator = (world.objekts.begin() + i);
			FizziksObjekt* pointerToFizziksObjekt = *iterator;
			delete pointerToFizziksObjekt;

			world.objekts.erase(iterator);
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
		FizziksCircle* newBird = new FizziksCircle();
		// New keyword allocates and reserves memory on the heap
		// (as opposed to the stack, where the data will be lost on exiting scope)
		newBird->position = startPos;
		newBird->velocity = { speed * (float)cos(angle * DEG2RAD), -speed * (float)sin(angle * DEG2RAD) };

		//rand() % N produces random number from 0 to N-1
		newBird->radius = (rand() % 26) + 5; // radius from 5-30
		Color randomColor = { rand() % 256 , rand() % 256, rand() % 256, 255 };
		newBird->color = randomColor;

		world.add(newBird); // Add bird to simulation
	}
}

//Display world state
void draw()
{
	BeginDrawing();
	ClearBackground(BLACK);
	DrawText("Earl Fabian 101554213", 10, float(GetScreenHeight() - 30), 20, LIGHTGRAY);


	GuiSliderBar(Rectangle{ 10, 15, 1000, 20 }, "", TextFormat("%.2f", time), &time, 0, 240);

	GuiSliderBar(Rectangle{ 10, 40, 500, 30 }, "Speed", TextFormat("Speed: %.0f", speed), &speed, -1000, 1000);

	GuiSliderBar(Rectangle{ 10, 80, 500, 30 }, "Angle", TextFormat("Angle: %.0f Degrees", angle), &angle, -180, 180);

	GuiSliderBar(Rectangle{ 10, 120, 500, 30 }, "Gravity Y", TextFormat("Gravity Y: %.0f Px/sec^2", world.accelerationGravity.y), &world.accelerationGravity.y, -1000, 1000);

	DrawText(TextFormat("Obects: %i", world.objekts.size()), 10, 160, 30, LIGHTGRAY);

	DrawText(TextFormat("T: %6.2f", time), GetScreenWidth() - 140, 10, 30, LIGHTGRAY);


	Vector2 velocity = { speed * cos(angle * DEG2RAD), -speed * sin(angle * DEG2RAD) };

	DrawLineEx(startPos, startPos + velocity, 3, RED);

	//Controls for halfspace
	GuiSliderBar(Rectangle{ 80, 200, 240, 30 }, "X", TextFormat("%.0f", halfspace.position.x), &halfspace.position.x, 0, GetScreenWidth());
	GuiSliderBar(Rectangle{ 380, 200, 240, 30 }, "Y", TextFormat("%.0f", halfspace.position.y), &halfspace.position.y, 0, GetScreenHeight());

	float halfspaceRotation = halfspace.getRotation();
	GuiSliderBar(Rectangle{ 700, 200, 200, 30 }, "Rotation", TextFormat("%.0f", halfspace.getRotation()), &halfspaceRotation, -360, 360);
	halfspace.setRotationDegrees(halfspaceRotation);

	//Draw all physics objects!
	for (int i = 0; i < world.objekts.size(); i++)
	{
		world.objekts[i]->draw();
		//Through the magic of polymorphism, we can place multiple 
		// types of objects in world.objekts. Circle, Box, Halfspace etc.
		// Then, when we call the parent function draw(), we should get the 
		// derived class behaviour specific to what that object actually is e.g.
		// Circle.draw() on a Circle, Box.draw() on a Box
	}

	EndDrawing();
}

int main()
{
	InitWindow(InitialWidth, InitialHeight, "GAME2005 Earl Fabian 101554213");
	SetTargetFPS(TARGET_FPS);
	halfspace.isStatic = true;
	halfspace.position = { 200, 700 };
	halfspace.setRotationDegrees(-30);
	world.add(&halfspace);
	halfspace2.isStatic = true;
	halfspace2.position = { 400, 700 };
	halfspace2.setRotationDegrees(30);
	world.add(&halfspace2);
	startPos = { 100, GetScreenHeight() - 500.0f };	

	while (!WindowShouldClose()) // Loops TARGET_FPS times per second
	{
		update();
		draw();
	}

	CloseWindow();
	return 0;
}