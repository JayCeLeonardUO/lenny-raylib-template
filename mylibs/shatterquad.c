// shatter_quad.c
// Drop-in shatter effect for a raylib project. Single file, shaders embedded.
// Port of the card_layers.html shatter demo.
//
// Extern these two (plus UnloadShatterQuad) where you use them:
//
//   Model GenTessellateQuad(float width, float height, int pieces, Texture2D texture);
//   void  DrawShatterEffect(Model model, Vector3 position, float shatterTime, float time);
//   void  UnloadShatterQuad(Model* model);
//
// Usage:
//   Model card = GenTessellateQuad(2.5f, 3.5f, 40, texture);
//   ...
//   // caller owns timing: shatterTime = seconds since the shatter was
//   // triggered, time = running clock for drift/wobble phase.
//   DrawShatterEffect(card, position, shatterTime, GetTime());
//
// The quad is tessellated ONCE on the CPU (recursive longest-edge split,
// same algorithm as the demo). Each shard's centroid is baked into the
// vertexNormal slot; everything random is hashed from it inside the VS,
// so the animation is fully GPU-side, driven only by the time uniforms.
// The VS is stateless: shatterTime = 0 is the intact card, so replay is
// just resetting the caller's clock.
//
// All models share ONE shader, loaded lazily on the first Gen call.
// Tunables (split, spin, duration, overshoot, ...) are consts at the top
// of the embedded vertex shader below.
//
// Standalone demo build:
//   cc -DSHATTER_QUAD_DEMO shatter_quad.c -o shatter_quad -lraylib -lm

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

// ---- embedded shaders (generated from shatter_card.vs / .fs) ---------
static const char *SHATTER_VS =
    "#version 330\n"
    "\n"
    "// Shatter card vertex shader. Port of the card_layers.html shard VS.\n"
    "// Geometry contract (built by BuildShatterQuad):\n"
    "//   vertexPosition  = resting shard vertex, quad plane, z = 0\n"
    "//   vertexTexCoord  = UV baked from resting position\n"
    "//   vertexNormal    = shard centroid (xy, z = 0) -- NOT a surface normal.\n"
    "//                     All per-shard randomness is hashed from this in here.\n"
    "\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec3 vertexNormal;\n"
    "\n"
    "uniform mat4 mvp;\n"
    "uniform float uShatterTime;   // seconds since shatter start (host accumulates dt)\n"
    "uniform float uTime;          // running clock, drives drift/wobble phase\n"
    "\n"
    "out vec2 fragTexCoord;\n"
    "\n"
    "// ---- tunables (values carried over from card_layers.html P block) ----\n"
    "const float DURATION    = 0.3;    // seconds to reach full split\n"
    "const float OVERSHOOT   = 1.7;    // 0 collapses easing to plain ease-out cubic\n"
    "const float SPLIT       = 0.18;   // radial travel at rest of the split\n"
    "const float Z_SPREAD    = 0.35;   // depth scatter\n"
    "const float SPIN        = 0.5;    // radians of tumble at full progress\n"
    "const float DIR_JITTER  = 0.35;   // blend from pure radial toward random dir\n"
    "const float FLOAT_AMP   = 0.04;   // continuous drift amplitude\n"
    "const float FLOAT_SPEED = 1.2;    // drift/wobble speed\n"
    "const float WOBBLE      = 0.0;    // secondary rotation amplitude\n"
    "\n"
    "const float TWO_PI = 6.28318530718;\n"
    "\n"
    "// ---- per-shard randomness, hashed from the centroid ------------------\n"
    "// Every vertex of a shard shares the same centroid, so these are flat\n"
    "// per piece, matching the CPU-baked attributes of the original demo.\n"
    "float hash11(float p) {\n"
    "  p = fract(p * 0.1031);\n"
    "  p *= p + 33.33;\n"
    "  p *= p + p;\n"
    "  return fract(p);\n"
    "}\n"
    "\n"
    "float shardRand(vec2 c, float k) {\n"
    "  return hash11(dot(c, vec2(127.1, 311.7)) + k * 17.317);\n"
    "}\n"
    "\n"
    "vec3 shardVec3(vec2 c, float k) {\n"
    "  vec3 v = vec3(shardRand(c, k) * 2.0 - 1.0,\n"
    "                shardRand(c, k + 1.0) * 2.0 - 1.0,\n"
    "                shardRand(c, k + 2.0) * 2.0 - 1.0);\n"
    "  return v / max(length(v), 1e-5);\n"
    "}\n"
    "\n"
    "// Ease-out with overshoot: launches hard, passes the resting split,\n"
    "// settles back onto it. Same curve as the demo.\n"
    "float easeOutBack(float x) {\n"
    "  float c1 = OVERSHOOT;\n"
    "  float c3 = c1 + 1.0;\n"
    "  float i = x - 1.0;\n"
    "  return 1.0 + c3 * i * i * i + c1 * i * i;\n"
    "}\n"
    "\n"
    "vec3 rotateAxis(vec3 v, vec3 axis, float a) {\n"
    "  float c = cos(a);\n"
    "  float s = sin(a);\n"
    "  return v * c + cross(axis, v) * s + axis * dot(axis, v) * (1.0 - c);\n"
    "}\n"
    "\n"
    "void main() {\n"
    "  vec2 centroid2 = vertexNormal.xy;\n"
    "\n"
    "  float t = easeOutBack(clamp(uShatterTime / DURATION, 0.0, 1.0));\n"
    "\n"
    "  float mag   = 0.6 + shardRand(centroid2, 0.0) * 0.8;\n"
    "  float zUnit = shardRand(centroid2, 3.0) * 2.0 - 1.0;\n"
    "  float spinU = shardRand(centroid2, 4.0) * 2.0 - 1.0;\n"
    "  float phase = shardRand(centroid2, 5.0) * TWO_PI;\n"
    "  vec2 jitter = vec2(shardRand(centroid2, 6.0) * 2.0 - 1.0,\n"
    "                     shardRand(centroid2, 7.0) * 2.0 - 1.0);\n"
    "  vec3 axis      = shardVec3(centroid2, 8.0);\n"
    "  vec3 floatAxis = shardVec3(centroid2, 11.0);\n"
    "\n"
    "  // outward direction, blended toward the per-piece jitter vector\n"
    "  vec2 radial = centroid2 / max(length(centroid2), 1e-5);\n"
    "  vec2 d = radial + jitter * DIR_JITTER;\n"
    "  d /= max(length(d), 1e-5);\n"
    "  vec3 offset = vec3(d * (SPLIT * mag), zUnit * Z_SPREAD);\n"
    "\n"
    "  // continuous drift, each piece on its own phase\n"
    "  vec3 drift = floatAxis * (sin(uTime * FLOAT_SPEED + phase) * FLOAT_AMP);\n"
    "\n"
    "  float spinA = spinU * SPIN * t;\n"
    "  float wobA  = sin(uTime * FLOAT_SPEED * 0.7 + phase * 1.3) * WOBBLE * t;\n"
    "\n"
    "  vec3 centroid3 = vec3(centroid2, 0.0);\n"
    "  vec3 local = rotateAxis(vertexPosition - centroid3, axis, spinA);\n"
    "  local = rotateAxis(local, floatAxis, wobA);\n"
    "\n"
    "  // drift is gated by t as well, so the card is solid at rest\n"
    "  vec3 p = centroid3 + local + (offset + drift) * t;\n"
    "\n"
    "  fragTexCoord = vertexTexCoord;\n"
    "  gl_Position = mvp * vec4(p, 1.0);\n"
    "}\n";
static const char *SHATTER_FS =
    "#version 330\n"
    "\n"
    "in vec2 fragTexCoord;\n"
    "\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "\n"
    "out vec4 finalColor;\n"
    "\n"
    "void main() {\n"
    "  finalColor = texture(texture0, fragTexCoord) * colDiffuse;\n"
    "}\n";

// ---- shared shader, loaded on first use ------------------------------
static Shader g_shatter_shader = {0};
static int g_loc_shatter_time = -1;
static int g_loc_time = -1;

static Shader ShatterShader(void)
{
    if (g_shatter_shader.id == 0)
    {
        g_shatter_shader = LoadShaderFromMemory(SHATTER_VS, SHATTER_FS);
        g_loc_shatter_time = GetShaderLocation(g_shatter_shader, "uShatterTime");
        g_loc_time = GetShaderLocation(g_shatter_shader, "uTime");
    }
    return g_shatter_shader;
}

// ---- tessellation: recursive longest-edge split ----------------------
typedef struct Tri2
{
    Vector2 p[3];
} Tri2;

static float TriArea(const Tri2 *t)
{
    float a = (t->p[1].x - t->p[0].x) * (t->p[2].y - t->p[0].y) - (t->p[2].x - t->p[0].x) * (t->p[1].y - t->p[0].y);
    return fabsf(a) * 0.5f;
}

static float Rand01(void)
{
    return (float)GetRandomValue(0, 0x7FFF) / (float)0x7FFF;
}

// Splits the w x h quad (centered on origin) into exactly `target`
// triangles. Caller frees the returned array.
static Tri2 *Tessellate(float w, float h, int target, int *out_count)
{
    if (target < 2) target = 2;
    Tri2 *tris = (Tri2 *)MemAlloc(sizeof(Tri2) * target);

    float hw = w * 0.5f, hh = h * 0.5f;
    Vector2 A = {-hw, -hh}, B = {hw, -hh}, C = {hw, hh}, D = {-hw, hh};
    tris[0] = (Tri2){{A, B, C}};
    tris[1] = (Tri2){{A, C, D}};
    int count = 2;

    while (count < target)
    {
        // pick a triangle, biased toward large ones but not strictly largest
        int idx = 0;
        float best = -1.0f;
        for (int i = 0; i < count; i++)
        {
            float weight = TriArea(&tris[i]) * (0.5f + Rand01());
            if (weight > best)
            {
                best = weight;
                idx = i;
            }
        }
        Tri2 t = tris[idx];

        // longest edge
        int e = 0;
        float len = -1.0f;
        for (int i = 0; i < 3; i++)
        {
            Vector2 a = t.p[i], b = t.p[(i + 1) % 3];
            float l = (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
            if (l > len)
            {
                len = l;
                e = i;
            }
        }

        Vector2 p = t.p[e], q = t.p[(e + 1) % 3], r = t.p[(e + 2) % 3];
        float s = 0.3f + Rand01() * 0.4f;
        Vector2 mid = {p.x + (q.x - p.x) * s, p.y + (q.y - p.y) * s};

        // replace the picked triangle with one half, append the other
        tris[idx] = (Tri2){{p, mid, r}};
        tris[count] = (Tri2){{mid, q, r}};
        count++;
    }

    *out_count = count;
    return tris;
}

// ---- gen -------------------------------------------------------------
Model GenTessellateQuad(float width, float height, int pieces, Texture2D texture)
{
    int n = 0;
    Tri2 *tris = Tessellate(width, height, pieces, &n);

    Mesh mesh = {0};
    mesh.vertexCount = n * 3;
    mesh.triangleCount = n;
    mesh.vertices = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float *)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
    mesh.normals = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));

    float hw = width * 0.5f, hh = height * 0.5f;

    for (int i = 0; i < n; i++)
    {
        Tri2 *t = &tris[i];
        float cx = (t->p[0].x + t->p[1].x + t->p[2].x) / 3.0f;
        float cy = (t->p[0].y + t->p[1].y + t->p[2].y) / 3.0f;

        for (int v = 0; v < 3; v++)
        {
            int o3 = (i * 3 + v) * 3;
            int o2 = (i * 3 + v) * 2;

            mesh.vertices[o3 + 0] = t->p[v].x;
            mesh.vertices[o3 + 1] = t->p[v].y;
            mesh.vertices[o3 + 2] = 0.0f;

            // UV baked from resting position, so the texture travels with the
            // pieces. V flipped so the texture reads upright with +Y up.
            mesh.texcoords[o2 + 0] = (t->p[v].x + hw) / width;
            mesh.texcoords[o2 + 1] = 1.0f - (t->p[v].y + hh) / height;

            // centroid smuggled through the normal slot (see embedded VS)
            mesh.normals[o3 + 0] = cx;
            mesh.normals[o3 + 1] = cy;
            mesh.normals[o3 + 2] = 0.0f;
        }
    }

    MemFree(tris);
    UploadMesh(&mesh, false);

    Model model = LoadModelFromMesh(mesh);
    model.materials[0].shader = ShatterShader();
    SetMaterialTexture(&model.materials[0], MATERIAL_MAP_DIFFUSE, texture);
    return model;
}

// ---- draw ------------------------------------------------------------
// shatterTime: seconds since the shatter was triggered (0 = intact card,
// clamped at full split past the duration). time: running clock driving
// the drift/wobble phase. The VS is stateless; pose is analytic in both.
void DrawShatterEffect(Model model, Vector3 position, float shatterTime, float time)
{
    Shader shader = ShatterShader();
    SetShaderValue(shader, g_loc_shatter_time, &shatterTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, g_loc_time, &time, SHADER_UNIFORM_FLOAT);

    // shards tumble past edge-on; without this a spun shard vanishes
    rlDisableBackfaceCulling();
    DrawModel(model, position, 1.0f, WHITE);
    rlEnableBackfaceCulling();
}

// Detaches the shared shader before unload so UnloadModel doesn't kill
// the shader other quads are still using. Does not unload the texture.
void UnloadShatterQuad(Model *model)
{
    model->materials[0].shader = (Shader){rlGetShaderIdDefault(), rlGetShaderLocsDefault()};
    model->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = (Texture2D){0};
    UnloadModel(*model);
    *model = (Model){0};
}

// ---- demo ------------------------------------------------------------
#ifdef SHATTER_QUAD_DEMO
int main(void)
{
    InitWindow(900, 700, "shatter quad");
    SetTargetFPS(60);

    Camera3D camera = {
        .position = (Vector3){0.0f, 0.0f, 6.0f},
        .target = (Vector3){0.0f, 0.0f, 0.0f},
        .up = (Vector3){0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE};

    Image checker = GenImageChecked(256, 358, 32, 32, GOLD, DARKPURPLE);
    Texture2D tex = LoadTextureFromImage(checker);
    UnloadImage(checker);

    Model card = GenTessellateQuad(2.5f, 3.5f, 40, tex);
    float shatter_time = 0.0f;

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_SPACE)) shatter_time = 0.0f;
        shatter_time += GetFrameTime();

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
        DrawShatterEffect(card, (Vector3){0, 0, 0}, shatter_time, (float)GetTime());
        EndMode3D();

        DrawText("SPACE: replay shatter", 10, 10, 20, RAYWHITE);
        EndDrawing();
    }

    UnloadShatterQuad(&card);
    UnloadTexture(tex);
    CloseWindow();
    return 0;
}
#endif // SHATTER_QUAD_DEMO
