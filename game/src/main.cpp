
#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "game.h"
#include <string>
#include <vector>

const unsigned int TARGET_FPS = 50; //frames/second
float dt = 1.0f / TARGET_FPS;      //seconds/frame
float time = 0;

enum FizziksShape
{
    CIRCLE,
    HALF_SPACE
};


class FizziksCircle;
class FizziksHalfspace;

bool CircleCircleCollisionResponse(FizziksCircle* circleA, FizziksCircle* circleB);
bool CircleHalfspaceCollisionResponse(FizziksCircle* circle, FizziksHalfspace* halfspace);

class FizziksObjekt
{
public:
    bool isStatic = false; 
    Vector2 position = { 0,0 };
    Vector2 velocity = { 0,0 };
    float mass = 1; // in kg

    std::string name = "objekt";
    Color color = RED;

    virtual void draw() 
    {
        DrawCircle(position.x, position.y, 2, color);

    }

    virtual FizziksShape Shape() = 0; // pure virtual
};

class FizziksWorld
{
private:
    unsigned int objektCount = 0;
public:
    std::vector<FizziksObjekt*> objekts; 

    Vector2 accelerationGravity = { 0, 0 };

    void add(FizziksObjekt* newObject)
    {
        newObject->name = std::to_string(objektCount);
        objekts.push_back(newObject);
        objektCount++;
    }


    void update()
    {
        for (int i = 0; i < objekts.size(); i++)
        {
            FizziksObjekt* objekt = objekts[i];

     
            if (objekt->isStatic) continue;

            objekt->position = objekt->position + objekt->velocity * dt;

            objekt->velocity = objekt->velocity + accelerationGravity * dt;
        }

        checkCollisions();
    }

    void checkCollisions()
    {

        for (int i = 0; i < objekts.size(); i++)
        {
            objekts[i]->color = GREEN;
        }

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


class FizziksCircle : public FizziksObjekt
{
public:
    float radius; 

    // NEW: kinetic friction coefficient and force vectors
    float kFriction = 0.3f;
    Vector2 normalForce = { 0, 0 };
    Vector2 frictionForce = { 0, 0 };

    void draw() override
    {
        DrawCircle(position.x, position.y, radius, color);
        DrawText(name.c_str(), position.x, position.y, radius * 2, LIGHTGRAY);


        Vector2 Fgravity = world.accelerationGravity * mass;
        DrawLineEx(position, position + Fgravity * 0.05f, 2, PURPLE);


        DrawLineEx(position, position + normalForce * 0.05f, 2, GREEN);

        DrawLineEx(position, position + frictionForce * 0.05f, 2, ORANGE);


        DrawLineEx(position, position + velocity, 1, RED);
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
    Vector2 normal = { 0, -1 }; 


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

        DrawCircle(position.x, position.y, 8, color);


        DrawLineEx(position, position + normal * 30, 1, color);


        Vector2 parallelToSurface = Vector2Rotate(normal, PI * 0.5f);
        DrawLineEx(position - parallelToSurface * 4000, position + parallelToSurface * 4000, 1, color);
    }

    FizziksShape Shape() override
    {
        return HALF_SPACE;
    }
};


FizziksHalfspace halfspace;
FizziksHalfspace halfspace2;

bool CircleCircleOverlap(FizziksCircle* circleA, FizziksCircle* circleB) // returns true if circles are overlapping
{
    Vector2 displacementFromAToB = circleB->position - circleA->position;
    float distance = Vector2Length(displacementFromAToB);
    float sumOfRadii = circleA->radius + circleB->radius;

    if (sumOfRadii > distance)
    {
        return true; 
    }
    else
        return false; 
}


bool CircleCircleCollisionResponse(FizziksCircle* circleA, FizziksCircle* circleB)
{
    Vector2 displacementFromAToB = circleB->position - circleA->position;
    float distance = Vector2Length(displacementFromAToB);
    float sumOfRadii = circleA->radius + circleB->radius;
    float overlap = sumOfRadii - distance;

    if (overlap > 0)
    {
        Vector2 normalAtoB;
        if (fabsf(distance) < 0.0001f)
            normalAtoB = { 0,1 };
        else
            normalAtoB = displacementFromAToB / distance;

        Vector2 mtv = normalAtoB * overlap; //minimum translation vector

        circleA->position -= mtv * 0.5f;
        circleB->position += mtv * 0.5f;
        return true;
    }
    else
        return false;
}

bool CircleHalfspaceOverlap(FizziksCircle* circle, FizziksHalfspace* halfspace)
{
    Vector2 displacementToCircle = circle->position - halfspace->position;
    float dot = Vector2DotProduct(displacementToCircle, halfspace->getNormal());

    Vector2 projectionDisplacementOntoNormal = halfspace->getNormal() * dot;

    DrawLineEx(circle->position, circle->position - projectionDisplacementOntoNormal, 1, GRAY);
    Vector2 midpoint = circle->position - projectionDisplacementOntoNormal * 0.5f;
    DrawText(TextFormat("D: %6.0f", dot), midpoint.x, midpoint.y, 30, GRAY);

    return dot < circle->radius;
}

// NEW: friction-aware collision response with a Halfspace
bool CircleHalfspaceCollisionResponse(FizziksCircle* circle, FizziksHalfspace* halfspace)
{
    Vector2 displacementToCircle = circle->position - halfspace->position;
    float dot = Vector2DotProduct(displacementToCircle, halfspace->getNormal());
    Vector2 projectionDisplacementOntoNormal = halfspace->getNormal() * dot;
    float overlap = circle->radius - dot;

    if (overlap < 0.0f)
    {

        return false;
    }

    // --- 1. Positional correction ---
    Vector2 mtv = halfspace->getNormal() * overlap;
    circle->position += mtv;

    // --- 2. Normal force ---
    Vector2 g = world.accelerationGravity;                 // acceleration due to gravity
    float gAlongNormal = Vector2DotProduct(g, halfspace->getNormal()); // can be negative
    float normalMag = -gAlongNormal * circle->mass;        // choose magnitude so it cancels normal component of gravity
    if (normalMag < 0.0f) normalMag = 0.0f;

    circle->normalForce = halfspace->getNormal() * normalMag;

    // Remove velocity into the surface so it doesn't tunnel
    float vAlongNormal = Vector2DotProduct(circle->velocity, halfspace->getNormal());
    if (vAlongNormal < 0.0f)
    {
        circle->velocity -= halfspace->getNormal() * vAlongNormal;
    }

    // --- 3. Friction along the surface ---
    // Tangent direction along plane
    Vector2 tangent = Vector2Rotate(halfspace->getNormal(), PI * 0.5f);
    float vAlongTangent = Vector2DotProduct(circle->velocity, tangent);

    circle->frictionForce = { 0, 0 };

    if (fabsf(vAlongTangent) > 0.0001f)
    {
        // acceleration magnitude from kinetic friction: a_f = μ |g_n|
        float aFrictionMag = circle->kFriction * fabsf(gAlongNormal);
        float dv = aFrictionMag * dt;

        float speedT = fabsf(vAlongTangent);
        float newSpeedT = speedT - dv;
        if (newSpeedT < 0.0f) newSpeedT = 0.0f;

        float sign = (vAlongTangent > 0.0f) ? 1.0f : -1.0f;
        float new_vAlongTangent = sign * newSpeedT;

        // change in tangential component
        circle->velocity += tangent * (new_vAlongTangent - vAlongTangent);

        // friction force vector for drawing: magnitude μN, opposite direction of motion
        float frictionMag = circle->kFriction * normalMag;
        circle->frictionForce = tangent * (-sign * frictionMag);
    }

    return true;
}


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


struct SphereSetup
{
    Color color;
    float mass;
    float kFriction;
};

const SphereSetup sphereSetups[4] =
{
    // Sphere, Mass, kFriction
    { RED,    2.0f, 0.1f },   // Red
    { GREEN,  2.0f, 0.8f },   // Green
    { BLUE,   8.0f, 0.1f },   // Blue
    { YELLOW, 8.0f, 0.8f }    // Yellow
};

void update()
{
    dt = 1.0f / TARGET_FPS;
    time += dt;

    // Clear per-frame contact forces on all circles
    for (int i = 0; i < world.objekts.size(); ++i)
    {
        if (world.objekts[i]->Shape() == CIRCLE)
        {
            FizziksCircle* c = (FizziksCircle*)world.objekts[i];
            c->normalForce = { 0,0 };
            c->frictionForce = { 0,0 };
        }
    }

    cleanup();
    world.update();

    static int spawnIndex = 0;

    if (IsKeyPressed(KEY_SPACE))
    {
        FizziksCircle* newBall = new FizziksCircle();
        newBall->position = startPos;
        newBall->velocity = { speed * (float)cos(angle * DEG2RAD),
                              -speed * (float)sin(angle * DEG2RAD) };

        // Use assignment table for first 4 spheres, then fall back to random
        if (spawnIndex < 4)
        {
            const SphereSetup& setup = sphereSetups[spawnIndex];
            newBall->radius = 25.0f;         // fixed radius
            newBall->mass = setup.mass;
            newBall->kFriction = setup.kFriction;
            newBall->color = setup.color;
        }
        else
        {
            // After the 4 required spheres, allow random extra ones
            newBall->radius = (rand() % 26) + 5; // radius from 5-30
            Color randomColor = { rand() % 256 , rand() % 256, rand() % 256, 255 };
            newBall->color = randomColor;
            newBall->mass = 2.0f;
            newBall->kFriction = 0.3f;
        }

        world.add(newBall);
        spawnIndex++;
    }
}

// -------------------------------------------------------------
//  Display world state
// -------------------------------------------------------------

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
    }

    EndDrawing();
}

// -------------------------------------------------------------
//  main
// -------------------------------------------------------------

int main()
{
    InitWindow(InitialWidth, InitialHeight, "GAME2005 Earl Fabian 101554213");
    SetTargetFPS(TARGET_FPS);

    halfspace.isStatic = true;
    halfspace.position = { 200, 700 };
    halfspace.setRotationDegrees(-30);
    world.add(&halfspace);

    halfspace2.isStatic = true;
    halfspace2.position = { 400, 800 };
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
