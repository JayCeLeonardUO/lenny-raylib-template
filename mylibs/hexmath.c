/*  hexmath - minimal hex grid library, pointy-top axial coordinates on the XZ plane.
 *
 *  mylibs unity style: no guards or prototypes needed - amalgamate.cmake wraps
 *  everything in mylib.h's guards and auto-generates the prototypes.
 */

#include "raylib.h" /* Vector3 */
#include <stdbool.h>

/*  Axial coordinate. The third cube coordinate s is always -q-r and is never
 *  stored, because a stored s is an invariant nothing enforces. */
typedef struct HexCoord
{
    int q, r;
} HexCoord;

typedef struct HexGridLayout
{
    int radius;      /* rings of cells around the center */
    int count;       /* 3N^2 + 3N + 1, the draw loop's upper bound */
    float hexRadius; /* circumradius, center to vertex */
    float inradius;  /* derived: hexRadius * sqrt(3)/2 */
    Vector3 origin;  /* world position of cell (0,0) */
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

/*  Column q holds 2N+1-|q| cells, so walk columns subtracting heights until the
 *  remainder lands inside one. That remainder is the offset from the column's
 *  low r bound, which is -N for q >= 0 and -q-N for q < 0. */
bool HexIdToCoord(const HexGridLayout *grid, int id, HexCoord *out)
{
    if ((id < 0) || (id >= grid->count)) return false;

    int n = grid->radius;
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

/*  x counts half-widths (one inradius each), z counts row spacing of 1.5 circumradii. */
Vector3 HexToWorld(const HexGridLayout *grid, HexCoord c)
{
    Vector3 world;
    world.x = grid->origin.x + grid->inradius * (2.0f * (float)c.q + (float)c.r);
    world.y = grid->origin.y;
    world.z = grid->origin.z + grid->hexRadius * 1.5f * (float)c.r;
    return world;
}

/*  Cube distance. The three deltas sum to zero, so one always carries the sign
 *  opposite the other two and its magnitude equals half the Manhattan sum. That
 *  makes it a max, not sum/2 - and never |dq|+|dr|, which overcounts diagonal
 *  steps because the axial axes sit 120 degrees apart, not 90. */
int HexDistance(HexCoord a, HexCoord b)
{
    int dq = a.q - b.q;
    int dr = a.r - b.r;
    int ds = -dq - dr;

    if (dq < 0) dq = -dq;
    if (dr < 0) dr = -dr;
    if (ds < 0) ds = -ds;

    int m = (dq > dr) ? dq : dr;
    return (m > ds) ? m : ds;
}

/*  -1 when either id is out of range. Each HexIdToCoord walks up to 2N+1
 *  columns, so prefer HexDistance when the caller already holds coordinates. */
int HexIdDistance(const HexGridLayout *grid, int idA, int idB)
{
    HexCoord a, b;
    if (!HexIdToCoord(grid, idA, &a)) return -1;
    if (!HexIdToCoord(grid, idB, &b)) return -1;
    return HexDistance(a, b);
}
