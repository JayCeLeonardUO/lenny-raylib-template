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

// Shatter-on-hover test (mylibs/shatterquad.c): the card renders into its own
// texture with a fixed front camera and sits in a screen region; hovering the
// region replays the shatter
#define SHATTER_REGION_SIZE 200
static Texture2D shatterTex;
static Model shatterCard;
static RenderTexture2D shatterRT;
static Rectangle shatterRegion;
static bool shatterHover = false;
static float shatterTime = 0.0f; // seconds since the hover entered the region
static Camera3D cardCam = {0};

// Board camera state: right-drag orbit, wheel zoom, middle-drag/WASD pan, R resets
static float camYaw = 0.0f;
static float camPitch = PI / 4;
static float camDist = 17.0f;
static Vector3 camTarget = {0};

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

// Orbit/zoom/pan camera around camTarget. Position is recomputed from the
// spherical state every frame, so there is no drift to correct.
static void UpdateBoardCamera(void)
{
    // Right-drag orbits
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        Vector2 d = GetMouseDelta();
        camYaw -= d.x * 0.005f;
        camPitch += d.y * 0.005f;
        camPitch = Clamp(camPitch, 0.15f, 1.45f); // keep off the horizon and the pole
    }

    // Wheel zooms
    camDist -= GetMouseWheelMove() * 1.5f;
    camDist = Clamp(camDist, 5.0f, 35.0f);

    // Ground-projected view basis for panning
    float sy = sinf(camYaw), cy = cosf(camYaw);
    Vector3 fwd = (Vector3){-sy, 0.0f, -cy};
    Vector3 right = (Vector3){cy, 0.0f, -sy};

    // Middle-drag pans (grab the board), scaled by distance so screen speed is constant
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
    {
        Vector2 d = GetMouseDelta();
        float panScale = camDist * 0.0015f;
        camTarget = Vector3Subtract(camTarget, Vector3Scale(right, d.x * panScale));
        camTarget = Vector3Add(camTarget, Vector3Scale(fwd, d.y * panScale));
    }

    // WASD pans too
    float panSpeed = camDist * 0.6f * GetFrameTime();
    if (IsKeyDown(KEY_W)) camTarget = Vector3Add(camTarget, Vector3Scale(fwd, panSpeed));
    if (IsKeyDown(KEY_S)) camTarget = Vector3Subtract(camTarget, Vector3Scale(fwd, panSpeed));
    if (IsKeyDown(KEY_D)) camTarget = Vector3Add(camTarget, Vector3Scale(right, panSpeed));
    if (IsKeyDown(KEY_A)) camTarget = Vector3Subtract(camTarget, Vector3Scale(right, panSpeed));

    if (IsKeyPressed(KEY_R))
    {
        camYaw = 0.0f;
        camPitch = PI / 4;
        camDist = 17.0f;
        camTarget = (Vector3){0};
    }

    float cp = cosf(camPitch), sp = sinf(camPitch);
    camera.position = (Vector3){camTarget.x + camDist * cp * sy, camTarget.y + camDist * sp, camTarget.z + camDist * cp * cy};
    camera.target = camTarget;
}

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    framesCounter = 0;
    finishScreen = 0;
    board = HexGridLayoutMake(BOARD_RADIUS, TILE_RADIUS, (Vector3){0.0f, 0.0f, 0.0f});
    // Position/target come from UpdateBoardCamera each frame
    camYaw = 0.0f;
    camPitch = PI / 4;
    camDist = 17.0f;
    camTarget = (Vector3){0};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    // TILE_FILL shrinks the mesh inside its cell so neighbours do not share coplanar
    // side faces. TILE_HEIGHT is the same constant the cap offset uses below.
    hexModel = LoadModelFromMesh(GenMeshCylinder(TILE_RADIUS * TILE_FILL, TILE_HEIGHT, 6));
    capModel = LoadModelFromMesh(GenMeshPoly(6, TILE_RADIUS * TILE_FILL));
    // Shatter card: rendered into its own texture with a fixed front camera,
    // shown in a screen region in the bottom-right corner
    Image checker = GenImageChecked(256, 256, 32, 32, GOLD, DARKPURPLE);
    shatterTex = LoadTextureFromImage(checker);
    UnloadImage(checker);
    shatterCard = GenTessellateQuad(2.5f, 2.5f, 40, shatterTex);
    shatterRT = LoadRenderTexture(SHATTER_REGION_SIZE, SHATTER_REGION_SIZE);
    shatterRegion = (Rectangle){(float)(GetScreenWidth() - SHATTER_REGION_SIZE - 12),
                                (float)(GetScreenHeight() - SHATTER_REGION_SIZE - 12),
                                SHATTER_REGION_SIZE, SHATTER_REGION_SIZE};
    cardCam.position = (Vector3){0.0f, 0.0f, 5.0f};
    cardCam.target = (Vector3){0};
    cardCam.up = (Vector3){0.0f, 1.0f, 0.0f};
    cardCam.fovy = 45.0f;
    cardCam.projection = CAMERA_PERSPECTIVE;
    shatterHover = false;
    shatterTime = 0.0f;
}

// Gameplay Screen Update logic
void UpdateGameplayScreen(void)
{
    framesCounter++;
    // Must run here, not inside BeginMode3D. BeginMode3D snapshots the camera into
    // the matrix stack, so an update after it does not apply until the next frame.
    UpdateBoardCamera();
    // Region hover: intact while outside, bursts on entry and stays split inside
    bool hover = CheckCollisionPointRec(GetMousePosition(), shatterRegion);
    if (hover && !shatterHover) shatterTime = 0.0f; // replay on each entry
    shatterHover = hover;
    if (shatterHover) shatterTime += GetFrameTime();
    else shatterTime = 0.0f;
    // Press enter to change to ENDING screen (mouse buttons belong to the camera)
    if (IsKeyPressed(KEY_ENTER))
    {
        finishScreen = 1;
        PlaySound(fxCoin);
    }
}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    // Card panel first: render into its own texture with the fixed front camera
    BeginTextureMode(shatterRT);
    ClearBackground(BLANK);
    BeginMode3D(cardCam);
    DrawShatterEffect(shatterCard, (Vector3){0}, shatterTime, (float)GetTime());
    EndMode3D();
    EndTextureMode();

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
    DrawText("RMB drag: orbit | wheel: zoom | MMB/WASD: pan | R: reset cam", 10, GetScreenHeight() - 30, 20, DARKGRAY);
    // Shatter panel (render texture contents are Y-flipped)
    DrawTextureRec(shatterRT.texture,
                   (Rectangle){0, 0, (float)shatterRT.texture.width, (float)-shatterRT.texture.height},
                   (Vector2){shatterRegion.x, shatterRegion.y}, WHITE);
    DrawRectangleLinesEx(shatterRegion, 2, shatterHover ? GOLD : DARKGRAY);
    DrawText("hover me", (int)shatterRegion.x + 8, (int)shatterRegion.y + 8, 20, shatterHover ? GOLD : DARKGRAY);
}

// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    // UnloadModel frees the mesh it was built from, so do not also call UnloadMesh
    UnloadModel(hexModel);
    UnloadModel(capModel);
    UnloadShatterQuad(&shatterCard);
    UnloadTexture(shatterTex);
    UnloadRenderTexture(shatterRT);
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}
