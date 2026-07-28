/**********************************************************************************************
 *
 *   raylib - Advance Game template
 *
 *   Gameplay Screen Functions Definitions (Init, Update, Draw, Unload)
 *
 *   Copyright (c) 2014-2022 Ramon Santamaria (@raysan5)
 *
 *   This software is provided "as-is", without any express or implied warranty. In no event
 *   will the authors be held liable for any damages arising from the use of this software.
 *
 *   Permission is granted to anyone to use this software for any purpose, including commercial
 *   applications, and to alter it and redistribute it freely, subject to the following restrictions:
 *
 *     1. The origin of this software must not be misrepresented; you must not claim that you
 *     wrote the original software. If you use this software in a product, an acknowledgment
 *     in the product documentation would be appreciated but is not required.
 *
 *     2. Altered source versions must be plainly marked as such, and must not be misrepresented
 *     as being the original software.
 *
 *     3. This notice may not be removed or altered from any source distribution.
 *
 **********************************************************************************************/

#include "raylib.h"
#include "screens.h"

#include <math.h> // Required for: sqrtf(), roundf(), fabsf()

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------
#define MAX_ENTITIES 1024 // Entity pool capacity, allocated once at startup

//----------------------------------------------------------------------------------
// Board geometry
//----------------------------------------------------------------------------------
#define GRID_RADIUS 4            // Hex board radius in cells around the center
#define HEX_RADIUS 1.0f          // Circumradius: center -> vertex (half the point-to-point height)
#define HEX_INRADIUS 0.86602540f // Inradius: center -> edge midpoint. == HEX_RADIUS * sqrt(3)/2
#define HEX_FILL 0.94f           // Mesh radius as a fraction of the cell, leaves a visible gutter
#define HEX_HEIGHT 0.25f         // Tile thickness. Base sits at y = 0, top face at y = HEX_HEIGHT

// Centered hexagonal number: 1 + sum(6k, k=1..N)
#define HEX_COUNT(N) (3 * (N) * (N) + 3 * (N) + 1)
#define TILE_COUNT HEX_COUNT(GRID_RADIUS)

static Camera3D camera = {0};
static Model hexModel; // Hexagonal prism: the tile body
static Model capModel; // Flat hexagon stamped on top, lighter tint to fake a lit face

// Convert axial coordinates to a world position on the board plane (pointy-top layout).
// x counts half-widths (one inradius each), z counts row spacing of 1.5 circumradii.
static Vector3 HexAxialToWorld(int q, int r)
{
    Vector3 world = {0};
    world.x = HEX_INRADIUS * (2.0f * (float)q + (float)r);
    world.y = 0.0f;
    world.z = HEX_RADIUS * 1.5f * (float)r;
    return world;
}

// Convert a point on the board plane to axial coordinates, rounded to the nearest cell
static void HexWorldToAxial(float x, float z, int *q, int *r)
{
    float sqrt3 = sqrtf(3.0f);

    float px = x / HEX_RADIUS;
    float pz = z / HEX_RADIUS;

    float qf = sqrt3 / 3.0f * px - 1.0f / 3.0f * pz;
    float rf = 2.0f / 3.0f * pz;

    // Cube rounding: round each cube coord, then fix the one that drifted most
    float cx = qf;
    float cz = rf;
    float cy = -cx - cz;

    float rx = roundf(cx);
    float ry = roundf(cy);
    float rz = roundf(cz);

    float dx = fabsf(rx - cx);
    float dy = fabsf(ry - cy);
    float dz = fabsf(rz - cz);

    if ((dx > dy) && (dx > dz))
        rx = -ry - rz;
    else if (dy > dz)
        ry = -rx - rz;
    else
        rz = -rx - ry;

    *q = (int)rx;
    *r = (int)rz;
}

// Column q holds 2N+1-|q| cells. Ids run q ascending from -N, and within each
// column r ascending from that column's low bound.
static bool HexIdToAxial(int id, int *q, int *r)
{
    if ((id < 0) || (id >= HEX_COUNT(GRID_RADIUS))) return false;

    for (int cq = -GRID_RADIUS; cq <= GRID_RADIUS; cq++)
    {
        int aq = (cq < 0) ? -cq : cq;
        int height = 2 * GRID_RADIUS + 1 - aq;

        if (id < height)
        {
            *q = cq;
            *r = -GRID_RADIUS + ((cq < 0) ? -cq : 0) + id;
            return true;
        }
        id -= height;
    }
    return false;
}

static bool HexIdToWorld(int id, Vector3 *world)
{
    int q, r;
    if (!HexIdToAxial(id, &q, &r)) return false;
    *world = HexAxialToWorld(q, r);
    return true;
}

static int HexAxialToId(int q, int r)
{
    int s = -q - r;
    int aq = (q < 0) ? -q : q;
    int ar = (r < 0) ? -r : r;
    int as = (s < 0) ? -s : s;
    if ((aq > GRID_RADIUS) || (ar > GRID_RADIUS) || (as > GRID_RADIUS)) return -1;

    int id = 0;
    for (int cq = -GRID_RADIUS; cq < q; cq++)
    {
        int a = (cq < 0) ? -cq : cq;
        id += 2 * GRID_RADIUS + 1 - a;
    }
    return id + (r - (-GRID_RADIUS + ((q < 0) ? -q : 0)));
}

typedef struct Entity
{
    int active;
    int gen_id;
    int is_hex;

    // Hex coordinates
    int q;
    int r;

    // World position
    Vector3 world;

    // asset data
    Model model;
} Entity;

struct EntityHandle
{
    unsigned int slot;
    unsigned int generation;
};

typedef struct ArrayOfEntities
{
    int highest_id;
    Entity entities[MAX_ENTITIES];
} ArrayOfEntities;

static ArrayOfEntities entities;

static void SpawnEnity(void)
{
    // ittorate until a free slot
}

static void KillEntity(void)
{
    // marks entity as inactive
}

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    framesCounter = 0;
    finishScreen = 0;

    camera.position = (Vector3){0.0f, 12.0f, 12.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // HEX_FILL shrinks the mesh inside its cell so neighbours do not share coplanar
    // side faces. HEX_HEIGHT is the same constant the cap offset uses below.
    hexModel = LoadModelFromMesh(GenMeshCylinder(HEX_RADIUS * HEX_FILL, HEX_HEIGHT, 6));
    capModel = LoadModelFromMesh(GenMeshPoly(6, HEX_RADIUS * HEX_FILL));
}

// Gameplay Screen Update logic
void UpdateGameplayScreen(void)
{
    framesCounter++;

    // Must run here, not inside BeginMode3D. BeginMode3D snapshots the camera into
    // the matrix stack, so an update after it does not apply until the next frame.
    UpdateCamera(&camera, CAMERA_ORBITAL);

    // Press enter or tap to change to ENDING screen
    if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
    {
        finishScreen = 1;
        PlaySound(fxCoin);
    }
}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    BeginMode3D(camera);

    // draw hex board
    for (int id = 0; id < HEX_COUNT(GRID_RADIUS); id++)
    {
        Vector3 pos;
        if (!HexIdToWorld(id, &pos)) continue;

        // Body dark, top face light. The stock shader is unlit, so one tint would
        // render the prism as a flat silhouette and the depth would not read.
        DrawModel(hexModel, pos, 1.0f, DARKGRAY);

        Vector3 capPos = pos;
        capPos.y += HEX_HEIGHT + 0.002f; // Clear the prism's own top cap
        DrawModel(capModel, capPos, 1.0f, GRAY);
    }

    EndMode3D();

    // mylibs demo: MyClampInt comes from the amalgamated mylib.h
    DrawText(TextFormat("mylib: MyClampInt(15, 0, 10) = %d", MyClampInt(15, 0, 10)), 10, 10, 20, MAROON);
}

// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    // UnloadModel frees the mesh it was built from, so do not also call UnloadMesh
    UnloadModel(hexModel);
    UnloadModel(capModel);
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}
