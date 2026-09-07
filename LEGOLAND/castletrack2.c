/* LEGOLAND -- castle entrance track helpers (scope LL5).
 * VC6 SP3 /O2 /Gy /Gd. Types are local; offsets describe the original ABI.
 * Neighbours: coaster7.c (Castle_InitEntranceTrack), coastertiny.c
 * (TrackCurve_LineUpVector / CubicUpVector), schoolcar.c (CarClassTablesInit).
 */

#include <math.h>
#pragma intrinsic(sqrt)

typedef struct Vec3f { float x, y, z; } Vec3f;

/* schoolcar8.c's Curve, coaster7.c's RouteGeom and coaster3d.c's PieceDesc
 * are one 0x58-byte record. +0x24 is a height cubic (high order first) on a
 * line/cubic segment and the two rail radii on an arc. */
typedef struct RouteGeom {
    Vec3f pos;                 /* +00 */
    Vec3f dir;                 /* +0c */
    Vec3f offset;              /* +18 */
    float r0;                  /* +24  cubic c3 / arc rail-0 radius */
    float r1;                  /* +28  cubic c2 / arc rail-2 radius */
    float c1;                  /* +2c */
    float c0;                  /* +30 */
    unsigned char pad34[0x40 - 0x34];
    float length;              /* +40 */
    float t0;                  /* +44 */
    float t1;                  /* +48 */
    void* hooks;               /* +4c */
    struct RouteGeom* next;    /* +50 */
    struct RouteGeom* prev;    /* +54 */
} RouteGeom;                   /* 0x58 */

/* schoolcar.c's g_car_class_vt slices. Array-index forms do not relocate. */
extern void* g_car_class_vt[];                                 /* 0x004dd5e0 */
extern void* g_curve_line_hooks[];                             /* 0x004dd600 */
extern void* g_curve_arc_hooks[];                              /* 0x004dd620 */
/* Cubic Hermite basis, rows {c3,c2,c1,c0} times {z1, z0, s1, s0}. */
extern float g_cubic_hermite[4][4];                            /* 0x004b5abc */

void TrackCurve_InitLine(RouteGeom* curve, const Vec3f* from,
                         const Vec3f* to, const Vec3f* offset);

/* Quarter-turn arc: copy the two in-plane axes and the centre, store the
 * two rail-radius scales, parameter range [0, pi/2], arc hook table. */
// WIP-FUNCTION: LEGOLAND 0x00421ce0  (36/38, r1 in edx not ecx; r0 load after the +0x28 store)
void TrackCurve_InitArc(const Vec3f* a, const Vec3f* b, const Vec3f* c,
                        RouteGeom* curve, int r0, int r1)
{
    curve->pos = *a;
    curve->dir = *b;
    curve->offset = *c;
    curve->hooks = g_curve_arc_hooks;
    *(int*)&curve->r1 = r1;
    curve->t0 = 0.0f;
    *(int*)&curve->r0 = r0;
    curve->t1 = 1.57079506f;
    curve->next = 0;
    curve->prev = 0;
}

/* Straight segment: dir = to - from, length = |dir|, pos = from, offset as
 * given, parameter range [0, 1], line hook table, links cleared. */
// FUNCTION: LEGOLAND 0x00421ab0
void TrackCurve_InitLine(RouteGeom* curve, const Vec3f* from,
                         const Vec3f* to, const Vec3f* offset)
{
    float acc = 0.0f;
    int i;

    for (i = 0; i < 3; i++) {
        float s = ((const float*)to)[i] - ((const float*)from)[i];
        ((float*)&curve->dir)[i] = s;
        acc += s * s;
    }
    curve->length = (float)sqrt(acc);
    curve->pos = *from;
    curve->offset = *offset;
    curve->hooks = g_curve_line_hooks;
    curve->t0 = 0.0f;
    curve->t1 = 1.0f;
    curve->next = 0;
    curve->prev = 0;
}

/* Horizontal line plus a cubic height profile. Zeros the endpoints' z
 * (original mutates the caller's vectors), builds the XY line, then
 * multiplies {to.z, from.z, 0, 0} by g_cubic_hermite into cubic[4] and
 * installs the cubic hook table. */
// FUNCTION: LEGOLAND 0x00422180
void TrackCurve_InitCubic(Vec3f* from, Vec3f* to, const Vec3f* offset,
                          RouteGeom* curve)
{
    float v[4];
    float* row;
    float* dst;

    v[2] = 0.0f;
    v[0] = to->z;
    v[3] = 0.0f;
    v[1] = from->z;
    to->z = 0.0f;
    from->z = 0.0f;
    TrackCurve_InitLine(curve, from, to, offset);

    row = &g_cubic_hermite[0][0];
    dst = &curve->r0;
    do {
        float acc = 0.0f;
        float* col = v;
        int n = 4;
        do {
            float t = *row;
            acc += t * *col;
            row++;
            col++;
        } while (--n);
        *dst = acc;
        dst++;
    } while ((int)row <= (int)&g_cubic_hermite[3][0]);

    curve->hooks = g_car_class_vt;
}
