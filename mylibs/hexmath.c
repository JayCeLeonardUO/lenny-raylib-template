#include "raylib.h"  // Vector3
#include <stdbool.h>

typedef struct HexCoord
{
    int q, r;
} HexCoord;

typedef struct HexGridLayout
{
    int radius;      // rings of cells around the center
    int count;       // 3N^2 + 3N + 1, the draw loop's upper bound
    float hexRadius; // circumradius, center to vertex
    float inradius;  // derived: hexRadius * sqrt(3)/2
    Vector3 origin;  // world position of cell (0,0)
} HexGridLayout;

HexGridLayout HexGridLayoutMake(int radius, float hexRadius, Vector3 origin)
{
    HexGridLayout g;
    g.radius = (radius < 0) ? 0 : radius;
    g.count = 3 * g.radius * g.radius + 3 * g.radius + 1;
    g.hexRadius = hexRadius;
    g.inradius = hexRadius * 0.86602540f;
    g.origin = origin;
    return g;
}

// Column q holds 2N+1-|q| cells, so walk columns subtracting heights until the
// remainder lands inside one. That remainder is the offset from the column's low r.
bool HexIdToCoord(const HexGridLayout *g, int id, HexCoord *out)
{
    if ((id < 0) || (id >= g->count)) return false;

    int n = g->radius;
    for (int q = -n; q <= n; q++)
    {
        int aq = (q < 0) ? -q : q;
        int height = 2 * n + 1 - aq;

        if (id < height)
        {
            out->q = q;
            out->r = -n + ((q < 0) ? -q : 0) + id;
            return true;
        }
        id -= height;
    }
    return false;
}

Vector3 HexToWorld(const HexGridLayout *g, HexCoord c)
{
    Vector3 world;
    world.x = g->origin.x + g->inradius * (2.0f * (float)c.q + (float)c.r);
    world.y = g->origin.y;
    world.z = g->origin.z + g->hexRadius * 1.5f * (float)c.r;
    return world;
}
