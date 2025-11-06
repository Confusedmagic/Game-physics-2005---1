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
#include <cmath>

// ---- Safe fallback: dot for Vector2 (in case raymath lib isn't linked) ----
inline float Vector2Dot(Vector2 a, Vector2 b) { return a.x * b.x + a.y * b.y; }

// ---- Small color utilities ----
static inline unsigned char u8lerp(unsigned char a, unsigned char b, float t)
{
    return (unsigned char)(a + (b - a) * t);
}

static inline Color ColorLerp(Color a, Color b, float t)
{
    if (t < 0) t = 0; if (t > 1) t = 1;
    return Color{ u8lerp(a.r,b.r,t), u8lerp(a.g,b.g,t), u8lerp(a.b,b.b,t), u8lerp(a.a,b.a,t) };
}

// --------------------------------------------------------------------------
const unsigned int TARGET_FPS = 50;
float dt = 1.0f / TARGET_FPS;
float timeNow = 0.0f;
Vector2 birdLaunchPosition = { 100, 1000 };

class FizziksObjekt
{
public:
    Vector2 position = { 0,0 };
    Vector2 velocity = { 0,0 };
    float mass = 1;
    std::string name = "objekt";

    virtual ~FizziksObjekt() = default;
    virtual void draw()
    {
        DrawCircle((int)position.x, (int)position.y, 2, GREEN);
    }
};

class FizziksCircle : public FizziksObjekt
{
public:
    float radius = 10.0f;

    // New color system:
    Color baseColor = GREEN;     // assigned on add()
    bool  isColliding = false;   // set each frame by collision pass

    void draw() override
    {
        // Blend toward red if colliding; otherwise use stable base color
        Color drawCol = isColliding ? ColorLerp(baseColor, RED, 0.85f) : baseColor;

        DrawCircleV(position, radius, drawCol);
        DrawText(name.c_str(), (int)position.x, (int)position.y, (int)(radius * 0.8f), LIGHTGRAY);
        DrawLineEx(position, position + velocity, 1, drawCol);
    }
};

/* =========================
   Halfspace (2D “infinite plane”)
   Equation: dot(n, x) = d  (n must be unit length)
   The “inside” is dot(n, x) >= d
   ========================= */
class FizziksHalfspace : public FizziksObjekt
{
public:
    Vector2 n = { 0.70710678f, -0.70710678f }; // unit normal
    float d = 400.0f;                          // offset
    Color lineColor = { 190, 80, 80, 255 };

    FizziksHalfspace() { n = Vector2Normalize(n); }

    void draw() override
    {
        Vector2 p0 = Vector2Scale(n, d);           // a point on the line
        Vector2 t = { -n.y, n.x };                // tangent
        float   L = 5000.0f;

        Vector2 a = p0 - Vector2Scale(t, L);
        Vector2 b = p0 + Vector2Scale(t, L);

        DrawLineEx(a, b, 6.0f, lineColor);

        Vector2 tip = p0 + Vector2Scale(n, 60.0f);
        DrawLineEx(p0, tip, 3.0f, lineColor);
        Vector2 left = tip + Vector2Scale(Vector2Rotate(n, 140.0f * DEG2RAD), 15.0f);
        Vector2 right = tip + Vector2Scale(Vector2Rotate(n, -140.0f * DEG2RAD), 15.0f);
        DrawTriangle(tip, left, right, lineColor);

        // light fill on the inside side
        Vector2 q0 = p0 + Vector2Scale(t, L);
        Vector2 q1 = p0 - Vector2Scale(t, L);
        Vector2 q2 = q1 + Vector2Scale(n, 8000.0f);
        Vector2 q3 = q0 + Vector2Scale(n, 8000.0f);
        Color fill = { 190, 80, 80, 60 };
        DrawTriangle(q0, q1, q2, fill);
        DrawTriangle(q0, q2, q3, fill);
    }
};

// -------------------- collision helpers --------------------
static inline bool CircleCircleOverlap(const FizziksCircle* A, const FizziksCircle* B)
{
    Vector2 d = B->position - A->position;
    float   dist = Vector2Length(d);
    return dist < (A->radius + B->radius);
}

static inline bool CircleHalfspaceOverlap(const FizziksCircle* C, const FizziksHalfspace* H)
{
    float s = Vector2Dot(H->n, C->position) - H->d;   // signed distance to plane
    return s < C->radius;
}

// --------------------------- world -------------------------
class FizziksWorld
{
private:
    unsigned int objektCount = 0;

public:
    std::vector<FizziksObjekt*> objekts;
    Vector2 accelerationGravity = { 0, 9 };

    void add(FizziksObjekt* newObject)
    {
        newObject->name = std::to_string(objektCount);

        // Assign a stable base color for circles using HSV (new color method)
        if (auto* c = dynamic_cast<FizziksCircle*>(newObject))
        {
            float hue = fmodf((objektCount * 97.0f), 360.0f);       // pseudo-random but stable
            c->baseColor = ColorFromHSV(hue, 0.65f, 0.95f);
        }

        objekts.push_back(newObject);
        objektCount++;
    }

    void integrate()
    {
        for (auto* o : objekts)
        {
            if (auto* c = dynamic_cast<FizziksCircle*>(o))
            {
                c->position = c->position + c->velocity * dt;
                c->velocity = c->velocity + accelerationGravity * dt;
            }
        }
    }

    void collide()
    {
        // reset collision flags each frame (don’t touch colors directly)
        for (auto* o : objekts)
            if (auto* c = dynamic_cast<FizziksCircle*>(o)) c->isColliding = false;

        // circle–circle
        for (size_t i = 0; i < objekts.size(); ++i)
        {
            auto* A = dynamic_cast<FizziksCircle*>(objekts[i]);
            if (!A) continue;

            for (size_t j = i + 1; j < objekts.size(); ++j)
            {
                auto* B = dynamic_cast<FizziksCircle*>(objekts[j]);
                if (B && CircleCircleOverlap(A, B))
                {
                    A->isColliding = true;
                    B->isColliding = true;
                }
            }

            // circle–halfspace
            for (auto* o : objekts)
            {
                if (auto* H = dynamic_cast<FizziksHalfspace*>(o))
                {
                    if (CircleHalfspaceOverlap(A, H)) A->isColliding = true;
                }
            }
        }
    }

    void update()
    {
        integrate();
        collide();
    }
};

// --------------------------- globals ------------------------
float speed = 100;
float angleDeg = 0;

FizziksWorld world;

// Halfspace UI
float hsAngleDeg = 135.0f;
float hsOffset = 500.0f;
FizziksHalfspace* gHalfspace = nullptr;

// remove circles that leave the screen
void cleanup()
{
    for (int i = 0; i < (int)world.objekts.size(); ++i)
    {
        if (auto* c = dynamic_cast<FizziksCircle*>(world.objekts[i]))
        {
            if (c->position.y > GetScreenHeight() || c->position.y < 0 ||
                c->position.x > GetScreenWidth() || c->position.x < 0)
            {
                delete c;
                world.objekts.erase(world.objekts.begin() + i);
                --i;
            }
        }
    }
}

void update()
{
    dt = 1.0f / TARGET_FPS;
    timeNow += dt;

    // live-edit halfspace
    if (gHalfspace)
    {
        float ang = hsAngleDeg * DEG2RAD;
        gHalfspace->n = Vector2Normalize(Vector2{ cosf(ang), -sinf(ang) }); // y-down screen
        gHalfspace->d = hsOffset;
    }

    cleanup();
    world.update();

    if (IsKeyPressed(KEY_SPACE))
    {
        auto* c = new FizziksCircle();
        c->position = birdLaunchPosition;
        c->velocity = { speed * cosf(angleDeg * DEG2RAD), -speed * sinf(angleDeg * DEG2RAD) };
        c->radius = (float)((rand() % 26) + 5); // 5–30
        world.add(c);
    }
}

void draw()
{
    BeginDrawing();
    ClearBackground(BLACK);

    DrawText("Halfspace overlap: circles tint toward RED when colliding", 10, 10, 18, LIGHTGRAY);
    GuiSliderBar(Rectangle{ 10, 40, 500, 30 }, "Speed", TextFormat("Speed: %.0f", speed), &speed, -1000, 1000);
    GuiSliderBar(Rectangle{ 10, 80, 500, 30 }, "Angle", TextFormat("Angle: %.0f deg", angleDeg), &angleDeg, -180, 180);
    GuiSliderBar(Rectangle{ 10,120, 500, 30 }, "Gravity Y", TextFormat("gY: %.0f", world.accelerationGravity.y), &world.accelerationGravity.y, -1000, 1000);
    GuiSliderBar(Rectangle{ 10,160, 500, 30 }, "Launch Height", TextFormat("H: %.0f", birdLaunchPosition.y), &(birdLaunchPosition.y), 0, (float)GetScreenHeight());
    GuiSliderBar(Rectangle{ 10,210, 500, 30 }, "Halfspace Angle", TextFormat("%.0f deg", hsAngleDeg), &hsAngleDeg, -180, 180);
    GuiSliderBar(Rectangle{ 10,250, 500, 30 }, "Halfspace Offset", TextFormat("d=%.0f", hsOffset), &hsOffset, -2000, 2000);

    DrawText(TextFormat("Objects: %i", (int)world.objekts.size()), 10, 290, 30, LIGHTGRAY);
    DrawText(TextFormat("T: %6.2f", timeNow), GetScreenWidth() - 140, 10, 30, LIGHTGRAY);

    Vector2 v = { speed * cosf(angleDeg * DEG2RAD), -speed * sinf(angleDeg * DEG2RAD) };
    DrawLineEx(birdLaunchPosition, birdLaunchPosition + v, 3, RED);

    for (auto* o : world.objekts) o->draw();

    EndDrawing();
}

int main()
{
    InitWindow(InitialWidth, InitialHeight, "GAME2005 Halfspace Overlap (HSV color method)");
    SetTargetFPS(TARGET_FPS);

    gHalfspace = new FizziksHalfspace();
    world.add(gHalfspace);

    while (!WindowShouldClose())
    {
        update();
        draw();
    }

    for (auto* o : world.objekts) delete o;
    world.objekts.clear();
    CloseWindow();
    return 0;
}
