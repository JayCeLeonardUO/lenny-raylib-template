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
#include "mylib.h" // Required for: MyClampInt()

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
// Board configuration - rendering choices, not hex math
//----------------------------------------------------------------------------------
#define BOARD_RADIUS 4    // Rings of cells around the center
#define TILE_RADIUS 1.0f  // Circumradius of one cell, in world units
#define TILE_FILL 0.94f   // Mesh radius as a fraction of the cell, leaves a visible gutter
#define TILE_HEIGHT 0.25f // Tile thickness. Base sits at y = 0, top face at y = TILE_HEIGHT
static HexGridLayout board;
static Camera3D camera = {0};
static Model hexModel; // Hexagonal prism: the tile body
static Model capModel; // Flat hexagon stamped on top, lighter tint to fake a lit face
typedef struct Entity
{
    int active;
    int gen_id;
    int is_hex;
    // Hex coordinates
    HexCoord cell;
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
    board = HexGridLayoutMake(BOARD_RADIUS, TILE_RADIUS, (Vector3){0.0f, 0.0f, 0.0f});
    camera.position = (Vector3){0.0f, 12.0f, 12.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    // TILE_FILL shrinks the mesh inside its cell so neighbours do not share coplanar
    // side faces. TILE_HEIGHT is the same constant the cap offset uses below.
    hexModel = LoadModelFromMesh(GenMeshCylinder(TILE_RADIUS * TILE_FILL, TILE_HEIGHT, 6));
    capModel = LoadModelFromMesh(GenMeshPoly(6, TILE_RADIUS * TILE_FILL));
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
    for (int id = 0; id < board.count; id++)
    {
        HexCoord cell;
        if (!HexIdToCoord(&board, id, &cell)) continue;
        Vector3 pos = HexToWorld(&board, cell);
        // Body dark, top face light. The stock shader is unlit, so one tint would
        // render the prism as a flat silhouette and the depth would not read.
        DrawModel(hexModel, pos, 1.0f, DARKGRAY);
        Vector3 capPos = pos;
        capPos.y += TILE_HEIGHT + 0.002f; // Clear the prism's own top cap
        DrawModel(capModel, capPos, 1.0f, GRAY);
    }
    EndMode3D();
    // mylibs demo: MyClampInt comes from the amalgamated mylib.h
    DrawText(TextFormat("mylib: MyClampInt(15, 0, 10) = %d", MyClampInt(15, 0, 10)), 10, 10, 20, MAROON);
    // hex distance demo: first tile to last, straight across the board
    DrawText(TextFormat("dist(0, %d) = %d", board.count - 1, HexIdDistance(&board, 0, board.count - 1)), 10, 36, 20, MAROON);
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
