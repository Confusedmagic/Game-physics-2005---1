
#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "game.h"
#include <string>
#include <vector>
#include <cmath>

const unsigned int TARGET_FPS = 50; // frames/second
float dt = 1.0f / TARGET_FPS;       // seconds/frame
float time = 0;

// --------------------- Shapes ---------------------
enum FizziksShape
{
    CIRCLE,
    HALF_SPACE,
    AABB
};

class FizziksCircle;
class FizziksHalfspace;
class FizziksAABB;

bool CircleCircleCollisionResponse(FizziksCircle* circleA, FizziksCircle* circleB);
bool CircleHalfspaceCollisionResponse(FizziksCircle* circle, FizziksHalfspace* halfspace);
bool AABB_AABB_Collision(FizziksAABB* objA, FizziksAABB* objB);
bool CircleAABB_Collision(FizziksCircle* circle, FizziksAABB* box);
bool AABBHalfspaceCollisionResponse(FizziksAABB* box, FizziksHalfspace* halfspace);

// --------------------- Base objekt ---------------------
class FizziksObjekt
{
public:
    bool isStatic = false;
    Vector2 position = { 0,0 };
    Vector2 velocity = { 0,0 };
    float mass = 1.0f;   // kg

    // NEW: restitution (0..1)
    float bounce = 0.8f;

    std::string name = "objekt";
    Color color = RED;

    virtual void draw()
    {
        DrawCircle((int)position.x, (int)position.y, 2, color);
    }

    virtual FizziksShape Shape() = 0; // pure virtual
};

// --------------------- World ---------------------
class FizziksWorld
{
private:
    unsigned int objektCount = 0;

public:
    std::vector<FizziksObjekt*> objekts;

    // This is acceleration (px/s^2) controlled by slider
    Vector2 accelerationGravity = { 0, 500.0f };

    void add(FizziksObjekt* newObject)
    {
        // Don't rename objects that already have nice names (scenario labels)
        if (newObject->name == "objekt")
            newObject->name = std::to_string(objektCount);

        objekts.push_back(newObject);
        objektCount++;
    }

    void clearDynamic()
    {
        // delete heap objects only (we keep static halfspaces if they're stack objects)
        for (int i = 0; i < (int)objekts.size(); ++i)
        {
            FizziksObjekt* o = objekts[i];
            // We only ever allocate circles and AABBs with new in this file.
            // Halfspaces are added by pointer to globals below (stack).
            if (o->Shape() == CIRCLE || o->Shape() == AABB)
            {
                delete o;
            }
        }
        objekts.clear();
        objektCount = 0;
    }

    void update()
    {
        for (int i = 0; i < (int)objekts.size(); i++)
        {
            FizziksObjekt* objekt = objekts[i];
            if (objekt->isStatic) continue;

            // Integrate position and velocity (simple Euler like your existing code)
            objekt->position = objekt->position + objekt->velocity * dt;
            objekt->velocity = objekt->velocity + accelerationGravity * dt;
        }

        checkCollisions();
    }

    void checkCollisions()
    {
        for (int i = 0; i < (int)objekts.size(); i++)
            objekts[i]->color = GREEN;

        for (int i = 0; i < (int)objekts.size(); i++)
        {
            for (int j = i + 1; j < (int)objekts.size(); j++)
            {
                FizziksObjekt* a = objekts[i];
                FizziksObjekt* b = objekts[j];

                FizziksShape sa = a->Shape();
                FizziksShape sb = b->Shape();

                bool didOverlap = false;

                if (sa == CIRCLE && sb == CIRCLE)
                {
                    didOverlap = CircleCircleCollisionResponse((FizziksCircle*)a, (FizziksCircle*)b);
                }
                else if (sa == CIRCLE && sb == HALF_SPACE)
                {
                    didOverlap = CircleHalfspaceCollisionResponse((FizziksCircle*)a, (FizziksHalfspace*)b);
                }
                else if (sa == HALF_SPACE && sb == CIRCLE)
                {
                    didOverlap = CircleHalfspaceCollisionResponse((FizziksCircle*)b, (FizziksHalfspace*)a);
                }
                else if (sa == AABB && sb == HALF_SPACE)
                {
                    didOverlap = AABBHalfspaceCollisionResponse((FizziksAABB*)a, (FizziksHalfspace*)b);
                }
                else if (sa == HALF_SPACE && sb == AABB)
                {
                    didOverlap = AABBHalfspaceCollisionResponse((FizziksAABB*)b, (FizziksHalfspace*)a);
                }
                else if (sa == AABB && sb == AABB)
                {
                    didOverlap = AABB_AABB_Collision((FizziksAABB*)a, (FizziksAABB*)b);
                }
                else if (sa == CIRCLE && sb == AABB)
                {
                    didOverlap = CircleAABB_Collision((FizziksCircle*)a, (FizziksAABB*)b);
                }
                else if (sa == AABB && sb == CIRCLE)
                {
                    didOverlap = CircleAABB_Collision((FizziksCircle*)b, (FizziksAABB*)a);
                }

                if (didOverlap)
                {
                    a->color = RED;
                    b->color = RED;
                }
            }
        }
    }
};

FizziksWorld world;

// --------------------- Circle ---------------------
class FizziksCircle : public FizziksObjekt
{
public:
    float radius = 20.0f;

    // kinetic friction coefficient and force vectors (kept similar to your file)
    float kFriction = 0.3f;
    Vector2 normalForce = { 0, 0 };
    Vector2 frictionForce = { 0, 0 };

    void draw() override
    {
        DrawCircle((int)position.x, (int)position.y, radius, color);
        DrawText(name.c_str(), (int)position.x, (int)position.y, (int)(radius * 0.8f), LIGHTGRAY);

        Vector2 Fgravity = world.accelerationGravity * mass;
        DrawLineEx(position, position + Fgravity * 0.05f, 2, PURPLE);
        DrawLineEx(position, position + normalForce * 0.05f, 2, GREEN);
        DrawLineEx(position, position + frictionForce * 0.05f, 2, ORANGE);
        DrawLineEx(position, position + velocity, 1, RED);
    }

    FizziksShape Shape() override { return CIRCLE; }
};

// --------------------- Halfspace ---------------------
class FizziksHalfspace : public FizziksObjekt
{
private:
    float rotation = 0;
    Vector2 normal = { 0, -1 };

public:
    void setRotationDegrees(float rotationInDegrees)
    {
        rotation = rotationInDegrees;
        normal = Vector2Rotate({ 0, -1 }, rotation * DEG2RAD);
    }

    float getRotation() { return rotation; }
    Vector2 getNormal() { return normal; }

    void draw() override
    {
        DrawCircle((int)position.x, (int)position.y, 8, color);
        DrawLineEx(position, position + normal * 30, 1, color);

        Vector2 parallelToSurface = Vector2Rotate(normal, PI * 0.5f);
        DrawLineEx(position - parallelToSurface * 4000, position + parallelToSurface * 4000, 1, color);
    }

    FizziksShape Shape() override { return HALF_SPACE; }
};

// --------------------- AABB ---------------------
class FizziksAABB : public FizziksObjekt
{
public:
    Vector2 halfSize = { 30, 30 };

    float minX() const { return position.x - halfSize.x; }
    float maxX() const { return position.x + halfSize.x; }
    float minY() const { return position.y - halfSize.y; }
    float maxY() const { return position.y + halfSize.y; }

    void draw() override
    {
        Rectangle r;
        r.x = minX();
        r.y = minY();
        r.width = halfSize.x * 2.0f;
        r.height = halfSize.y * 2.0f;

        DrawRectangleLinesEx(r, 2, color);
        DrawText(name.c_str(), (int)r.x + 4, (int)r.y + 4, 16, LIGHTGRAY);
        DrawLineEx(position, position + velocity, 1, RED);
    }

    FizziksShape Shape() override { return AABB; }
};

// --------------------- Global halfspaces (like your file) ---------------------
FizziksHalfspace halfspace;
FizziksHalfspace halfspace2;

// ============================================================
// Ex7: Circle-Circle collision response with momentum + bounce
// ============================================================
bool CircleCircleCollisionResponse(FizziksCircle* circleA, FizziksCircle* circleB)
{
    Vector2 displacementFromAToB = circleB->position - circleA->position;
    float distance = Vector2Length(displacementFromAToB);
    float sumOfRadii = circleA->radius + circleB->radius;
    float overlap = sumOfRadii - distance;

    if (overlap <= 0.0f) return false;

    Vector2 n;
    if (distance < 0.0001f) n = { 0, 1 };
    else n = displacementFromAToB / distance;

    // --- MTV (positional correction) ---
    float invA = circleA->isStatic ? 0.0f : 1.0f / circleA->mass;
    float invB = circleB->isStatic ? 0.0f : 1.0f / circleB->mass;
    float invSum = invA + invB;
    if (invSum <= 0.0f) return true;

    Vector2 mtv = n * overlap;
    if (!circleA->isStatic) circleA->position -= mtv * (invA / invSum);
    if (!circleB->isStatic) circleB->position += mtv * (invB / invSum);

    // --- IMPULSE (momentum + restitution) ---
    Vector2 relVel = circleA->velocity - circleB->velocity;
    float vn = Vector2DotProduct(relVel, n);

    // If moving towards each other along normal
    if (vn < 0.0f)
    {
        float e = circleA->bounce * circleB->bounce; // combine elasticities
        float j = -(1.0f + e) * vn / invSum;

        if (!circleA->isStatic) circleA->velocity += n * (j * invA);
        if (!circleB->isStatic) circleB->velocity -= n * (j * invB);
    }

    return true;
}

// ============================================================
// Circle-Halfspace collision response (kept similar to yours)
// - add bounce so ball can bounce on planes for Ex7 scenario #1
// ============================================================
bool CircleHalfspaceCollisionResponse(FizziksCircle* circle, FizziksHalfspace* hs)
{
    Vector2 displacementToCircle = circle->position - hs->position;
    float dot = Vector2DotProduct(displacementToCircle, hs->getNormal());
    float overlap = circle->radius - dot;

    if (overlap < 0.0f) return false;

    // Stop penetration
    Vector2 mtv = hs->getNormal() * overlap;
    circle->position += mtv;

    // Normal force (for drawing only, like your file)
    Vector2 g = world.accelerationGravity;
    float gAlongNormal = Vector2DotProduct(g, hs->getNormal());
    float normalMag = -gAlongNormal * circle->mass;
    if (normalMag < 0.0f) normalMag = 0.0f;
    circle->normalForce = hs->getNormal() * normalMag;

    // Bounce/impulse on plane: reflect normal component using restitution
    float vAlongNormal = Vector2DotProduct(circle->velocity, hs->getNormal());
    if (vAlongNormal < 0.0f)
    {
        float e = circle->bounce * hs->bounce;
        // v' = v - (1+e) * (v·n) * n
        circle->velocity -= hs->getNormal() * ((1.0f + e) * vAlongNormal);
    }

    // Kinetic friction along surface (same vibe as your old code)
    Vector2 tangent = Vector2Rotate(hs->getNormal(), PI * 0.5f);
    float vAlongTangent = Vector2DotProduct(circle->velocity, tangent);

    circle->frictionForce = { 0,0 };
    if (fabsf(vAlongTangent) > 0.0001f)
    {
        float aFrictionMag = circle->kFriction * fabsf(gAlongNormal);
        float dv = aFrictionMag * dt;

        float speedT = fabsf(vAlongTangent);
        float newSpeedT = speedT - dv;
        if (newSpeedT < 0.0f) newSpeedT = 0.0f;

        float sign = (vAlongTangent > 0.0f) ? 1.0f : -1.0f;
        float new_vAlongTangent = sign * newSpeedT;

        circle->velocity += tangent * (new_vAlongTangent - vAlongTangent);

        float frictionMag = circle->kFriction * normalMag;
        circle->frictionForce = tangent * (-sign * frictionMag);
    }

    return true;
}
bool AABBHalfspaceCollisionResponse(FizziksAABB* box, FizziksHalfspace* hs)
{
    Vector2 n = hs->getNormal();

    // Support radius of AABB along the normal
    float r = box->halfSize.x * fabsf(n.x) + box->halfSize.y * fabsf(n.y);

    // Signed distance from center to plane
    float d = Vector2DotProduct(box->position - hs->position, n);

    float penetration = r - d;
    if (penetration <= 0.0f) return false;

    // Push out of plane
    if (!box->isStatic)
        box->position += n * penetration;

    // Bounce / resting velocity correction
    if (!box->isStatic)
    {
        float vn = Vector2DotProduct(box->velocity, n);

        if (vn < -1e-4f)
        {
            float e = box->bounce * hs->bounce;
            box->velocity -= n * ((1.0f + e) * vn);
        }
        else
        {
            box->velocity -= n * vn;
        }
    }

    return true;
}


// ============================================================
// Ex8: AABB-AABB collision (your provided code adapted)
// NOTE: raymath uses Vector2 * float, NOT float * Vector2.
// ============================================================
bool AABB_AABB_Collision(FizziksAABB* objA, FizziksAABB* objB)
{
    // Displacement vector from A → B
    Vector2 displacementFromAtoB = objB->position - objA->position;

    // Absolute overlap along X and Y
    float px = (objA->halfSize.x + objB->halfSize.x) - fabsf(displacementFromAtoB.x);
    if (px <= 0) return false;

    float py = (objA->halfSize.y + objB->halfSize.y) - fabsf(displacementFromAtoB.y);
    if (py <= 0) return false;

    // Minimum translation vector (MTV)
    Vector2 mtv = { 0,0 };
    if (px < py)
        mtv.x = (displacementFromAtoB.x < 0 ? -px : px);
    else
        mtv.y = (displacementFromAtoB.y < 0 ? -py : py);

    Vector2 n = Vector2Normalize(mtv);

    // Inverse masses
    float invA = objA->isStatic ? 0.0f : 1.0f / objA->mass;
    float invB = objB->isStatic ? 0.0f : 1.0f / objB->mass;
    float invSum = invA + invB;
    if (invSum == 0.0f) return true;

    // Apply positional correction
    if (!objA->isStatic) objA->position -= mtv * (invA / invSum);
    if (!objB->isStatic) objB->position += mtv * (invB / invSum);

    // Relative velocity
    Vector2 relVel = objA->velocity - objB->velocity;
    float vn = Vector2DotProduct(relVel, n);

    // Impulse if closing
    if (vn < 0.0f)
    {
        float e = objA->bounce * objB->bounce;
        float j = -(1.0f + e) * vn / invSum;

        if (!objA->isStatic) objA->velocity += n * (j * invA);
        if (!objB->isStatic) objB->velocity -= n * (j * invB);
    }
    else
    {
        // Kill normal velocity for resting contacts
        if (!objA->isStatic) objA->velocity -= n * vn;
        if (!objB->isStatic) objB->velocity += n * vn;
    }

    return true;
}

// ============================================================
// Ex8: Circle-AABB collision (your provided code adapted)
// ============================================================
bool CircleAABB_Collision(FizziksCircle* circle, FizziksAABB* box)
{
    float cx = Clamp(circle->position.x, box->minX(), box->maxX());
    float cy = Clamp(circle->position.y, box->minY(), box->maxY());

    Vector2 closest = { cx, cy };
    Vector2 delta = circle->position - closest;
    float dist = Vector2Length(delta);

    float overlap = circle->radius - dist;
    if (overlap <= 0.0f) return false;

    // Fix zero-distance case (circle center inside box)
    if (dist < 0.0001f)
    {
        float left = fabsf(circle->position.x - box->minX());
        float right = fabsf(circle->position.x - box->maxX());
        float top = fabsf(circle->position.y - box->minY());
        float bottom = fabsf(circle->position.y - box->maxY());

        if (left < right && left < top && left < bottom) delta = { -1,0 };
        else if (right < top && right < bottom) delta = { 1,0 };
        else if (top < bottom) delta = { 0,-1 };
        else delta = { 0,1 };

        dist = 1.0f;
    }

    Vector2 n = delta / dist;
    Vector2 mtv = n * overlap;

    // MTV correction (weighted by inverse masses, so it feels solid)
    float invA = 1.0f / circle->mass;
    float invB = box->isStatic ? 0.0f : 1.0f / box->mass;
    float invSum = invA + invB;
    if (invSum <= 0.0f) return true;

    circle->position += mtv * (invA / invSum);
    if (!box->isStatic)
        box->position -= mtv * (invB / invSum);

    // --- IMPULSE ---
    Vector2 relVel = circle->velocity - box->velocity;
    float vn = Vector2DotProduct(relVel, n);

    if (vn < 0.0f)
    {
        float e = circle->bounce * box->bounce;
        float j = -(1.0f + e) * vn / invSum;

        circle->velocity += n * (j * invA);
        if (!box->isStatic)
            box->velocity -= n * (j * invB);
    }

    return true;
}

// --------------------- cleanup (kept similar) ---------------------
void cleanup()
{
    for (int i = 0; i < (int)world.objekts.size(); i++)
    {
        FizziksObjekt* objekt = world.objekts[i];

        // Skip stack halfspaces
        if (objekt->Shape() == HALF_SPACE) continue;

        if (objekt->position.y > GetScreenHeight() + 200
            || objekt->position.y < -200
            || objekt->position.x > GetScreenWidth() + 200
            || objekt->position.x < -200)
        {
            auto it = world.objekts.begin() + i;
            delete* it;
            world.objekts.erase(it);
            i--;
        }
    }
}

// ============================================================
// Scenario system (so you can demonstrate Ex7 + Ex8 cleanly)
// ============================================================
enum Scenario
{
    SCN_BOUNCY_BALL = 0,
    SCN_POOL = 1,
    SCN_GALILEAN = 2,
    SCN_AABB_TOWER = 3
};

Scenario currentScenario = SCN_BOUNCY_BALL;

// sliders / params requested by screenshot
float massA = 8.0f;
float massB = 2.0f;
float velAx = 300.0f;
float velBx = 0.0f;
float restitutionA = 0.9f;
float restitutionB = 0.9f;

Vector2 startPos = { 120, 120 };

// Setup helpers
void AddFloorPlane()
{
    halfspace.isStatic = true;
    halfspace.bounce = 1.0f;
    halfspace.position = { (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() - 80.0f };
    halfspace.setRotationDegrees(0);
    world.add(&halfspace);
}

void SetupScenario(Scenario scn)
{
    // Reset time for presentation
    time = 0.0f;

    // Clear all heap objects and remove previous halfspaces
    world.clearDynamic();

    // Also clear any per-frame forces left around (safe)
    // (will be recomputed anyway)

    currentScenario = scn;

    // default gravity
    world.accelerationGravity = { 0, 700.0f };

    // Common plane for Ex7/Ex8 (a single floor)
    AddFloorPlane();

    if (scn == SCN_BOUNCY_BALL)
    {
        // One ball, no initial velocity, bounces on plane
        FizziksCircle* ball = new FizziksCircle();
        ball->name = "Ball";
        ball->position = { (float)GetScreenWidth() * 0.5f, 100.0f };
        ball->velocity = { 0, 0 };
        ball->radius = 30.0f;
        ball->mass = massA;
        ball->bounce = restitutionA;
        ball->kFriction = 0.1f;
        ball->color = RED;
        world.add(ball);
    }
    else if (scn == SCN_POOL)
    {
        // One moving circle hits a stationary circle on the plane
        FizziksCircle* A = new FizziksCircle();
        A->name = "A";
        A->position = { 200.0f, (float)GetScreenHeight() - 120.0f };
        A->velocity = { velAx, 0.0f };
        A->radius = 30.0f;
        A->mass = massA;
        A->bounce = restitutionA;
        A->kFriction = 0.2f;
        A->color = RED;
        world.add(A);

        FizziksCircle* B = new FizziksCircle();
        B->name = "B";
        B->position = { 520.0f, (float)GetScreenHeight() - 120.0f };
        B->velocity = { velBx, 0.0f };
        B->radius = 30.0f;
        B->mass = massB;
        B->bounce = restitutionB;
        B->kFriction = 0.2f;
        B->color = SKYBLUE;
        world.add(B);
    }
    else if (scn == SCN_GALILEAN)
    {
        // Two balls stacked, drop together and bounce (Galilean cannon arrangement)
        FizziksCircle* bottom = new FizziksCircle();
        bottom->name = "Bottom";
        bottom->position = { (float)GetScreenWidth() * 0.5f, 200.0f };
        bottom->velocity = { 0, 0 };
        bottom->radius = 35.0f;
        bottom->mass = massA;
        bottom->bounce = restitutionA;
        bottom->kFriction = 0.05f;
        bottom->color = ORANGE;
        world.add(bottom);

        FizziksCircle* top = new FizziksCircle();
        top->name = "Top";
        top->position = { (float)GetScreenWidth() * 0.5f, 200.0f - (bottom->radius + 25.0f) };
        top->velocity = { 0, 0 };
        top->radius = 25.0f;
        top->mass = massB;
        top->bounce = restitutionB;
        top->kFriction = 0.05f;
        top->color = YELLOW;
        world.add(top);
    }
    else if (scn == SCN_AABB_TOWER)
    {
        // Tower: bottom is static, rest dynamic
        // Big base
        FizziksAABB* base = new FizziksAABB();
        base->name = "Base";
        base->isStatic = true;
        base->position = { (float)GetScreenWidth() * 0.6f, (float)GetScreenHeight() - 120.0f };
        base->halfSize = { 250, 30 };
        base->mass = 999999.0f;
        base->bounce = 0.2f;
        base->color = WHITE;
        world.add(base);

        // Stack 3 boxes on top (movable)
        for (int i = 0; i < 3; ++i)
        {
            FizziksAABB* b = new FizziksAABB();
            b->name = "Box";
            b->isStatic = false;
            b->position = { base->position.x, base->position.y - (70.0f + i * 70.0f) };
            b->halfSize = { 60, 30 };
            b->mass = 5.0f;
            b->bounce = 0.2f;
            b->color = RED;
            world.add(b);
        }
    }
}

// ============================================================
// update/draw (kept similar structure)
// ============================================================

// ------------------------------------------------------------
// Sandbox controls (no "modes")
// 1 = spawn Circle at mouse
// 2 = spawn AABB  at mouse
// R = reset (keeps ground)
// Mouse: click + drag any shape, release to "throw"
// ------------------------------------------------------------
static FizziksObjekt* grabbed = nullptr;
static Vector2 grabOffset = { 0,0 };
static Vector2 prevGrabPos = { 0,0 };

static bool PointInAABB(const Vector2& p, const FizziksAABB* b)
{
    return (p.x >= b->minX() && p.x <= b->maxX() && p.y >= b->minY() && p.y <= b->maxY());
}

static bool PointInCircle(const Vector2& p, const FizziksCircle* c)
{
    return Vector2Distance(p, c->position) <= c->radius;
}

static void SpawnCircleAtMouse()
{
    Vector2 m = GetMousePosition();
    FizziksCircle* c = new FizziksCircle();
    c->name = "C";
    c->position = m;
    c->velocity = { 0,0 };
    c->radius = 22.0f;
    c->mass = 2.0f;
    c->bounce = 0.7f;
    c->kFriction = 0.15f;
    c->color = SKYBLUE;
    world.add(c);
}

static void SpawnAABBAtMouse()
{
    Vector2 m = GetMousePosition();
    FizziksAABB* b = new FizziksAABB();
    b->name = "B";
    b->position = m;
    b->velocity = { 0,0 };
    b->halfSize = { 28, 28 };
    b->mass = 5.0f;
    b->bounce = 0.3f;
    b->color = ORANGE;
    world.add(b);
}

static void ResetSandbox()
{
    world.clearDynamic();

    // Ground plane (halfspace) at bottom
    halfspace.position = { (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() - 80.0f };
    halfspace.setRotationDegrees(0);
    halfspace.isStatic = true;
    halfspace.name = "ground";
    halfspace.color = DARKGRAY;
    world.add(&halfspace);

    // Optional: a wall on the left to keep things in view
    halfspace2.position = { 80.0f, (float)GetScreenHeight() * 0.5f };
    halfspace2.setRotationDegrees(90);
    halfspace2.isStatic = true;
    halfspace2.name = "wall";
    halfspace2.color = DARKGRAY;
    world.add(&halfspace2);

    grabbed = nullptr;
}

void update()
{
    dt = 1.0f / TARGET_FPS;
    time += dt;

    // Clear per-frame forces on circles (same as your old update)
    for (int i = 0; i < (int)world.objekts.size(); ++i)
    {
        if (world.objekts[i]->Shape() == CIRCLE)
        {
            FizziksCircle* c = (FizziksCircle*)world.objekts[i];
            c->normalForce = { 0,0 };
            c->frictionForce = { 0,0 };
        }
    }


    // Keybind spawns
    if (IsKeyPressed(KEY_ONE)) SpawnCircleAtMouse();
    if (IsKeyPressed(KEY_TWO)) SpawnAABBAtMouse();

    // Reset
    if (IsKeyPressed(KEY_R))
    {
        ResetSandbox();
        return;
    }

    // Mouse picking/dragging
    Vector2 mouse = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        grabbed = nullptr;

        // Pick topmost object under cursor (iterate backwards)
        for (int i = (int)world.objekts.size() - 1; i >= 0; --i)
        {
            FizziksObjekt* o = world.objekts[i];
            if (o->isStatic) continue;

            bool hit = false;
            if (o->Shape() == CIRCLE) hit = PointInCircle(mouse, (FizziksCircle*)o);
            else if (o->Shape() == AABB) hit = PointInAABB(mouse, (FizziksAABB*)o);

            if (hit)
            {
                grabbed = o;
                grabOffset = grabbed->position - mouse;
                prevGrabPos = grabbed->position;
                grabbed->velocity = { 0,0 }; // stop while holding
                break;
            }
        }
    }

    if (grabbed && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
        Vector2 newPos = mouse + grabOffset;
        // "Throw" velocity based on mouse movement
        grabbed->velocity = (newPos - prevGrabPos) / dt;
        grabbed->position = newPos;
        prevGrabPos = newPos;
    }

    if (grabbed && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
    {
        // keep last computed velocity
        grabbed = nullptr;
    }

    cleanup();
    world.update();
}

void draw()
{
    BeginDrawing();
    ClearBackground(BLACK);

    DrawText("GAME2005 Earl Fabian 101554213", 10, 10, 20, LIGHTGRAY);
    DrawText("1: Spawn Circle   2: Spawn AABB   R: Reset", 10, 40, 18, LIGHTGRAY);
    DrawText("Mouse Left: Click + drag shapes to test collisions", 10, 64, 18, LIGHTGRAY);


    // Draw all physics objects
    for (int i = 0; i < (int)world.objekts.size(); i++)
        world.objekts[i]->draw();

    EndDrawing();
}

int main()
{
    InitWindow(InitialWidth, InitialHeight, "GAME2005 Earl Fabian 101554213");
    SetTargetFPS(TARGET_FPS);

    // Start in sandbox
    ResetSandbox();

    while (!WindowShouldClose())
    {
        update();
        draw();
    }

    // cleanup heap objects
    world.clearDynamic();

    CloseWindow();
    return 0;
}