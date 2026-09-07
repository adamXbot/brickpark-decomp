/* LEGOLAND -- track join / curve / station callees (scope LL7).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, callee argument counts and global addresses are
 * load-bearing; names are ours. Types are declared LOCALLY (legoland.h is
 * owned elsewhere) and follow coaster5.c / coaster6.c / coaster7.c /
 * coaster9.c.
 *
 * Neighbours: TrackJoinPieces (coaster5.c), TrackFitCheckSpan (coaster5.c),
 * Coaster_StationDerivative (coaster7.c), RouteNode_GetTransform /
 * TrackCurve_EvaluateDerivative (coaster9.c), RouteCar_SetPosition
 * (schoolcar4.c).
 */

#include <math.h>
#include <string.h>

#pragma intrinsic(memcpy, sqrt)

typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct Mat3 { Vec3f r[3]; } Mat3;
typedef struct PhysVec { int n; float v[20]; } PhysVec;

typedef struct RouteGeom RouteGeom;
typedef struct RoutePos RoutePos;
typedef struct TrackNode TrackNode;
typedef struct TrackDesc TrackDesc;
typedef struct CoasterRec CoasterRec;

typedef struct GeomEval {
    void (*pos)(RouteGeom* geom, float t, Vec3f* out);
    void (*dir)(RouteGeom* geom, float t, Vec3f* out);
} GeomEval;

struct RouteGeom {
    unsigned char pad00[0x4c];
    GeomEval*     eval;         /* +0x4c */
};

struct RoutePos {
    TrackNode*  node;           /* +0x00 */
    RouteGeom*  geom;           /* +0x04 */
    Vec3f       pos;            /* +0x08 */
};                              /* 0x14 */

typedef struct TrackJoint {
    int        dir;             /* +0x00 */
    float      h;               /* +0x04 */
    TrackNode* node;            /* +0x08 */
} TrackJoint;                   /* 0x0c */

struct TrackDesc {
    int raised;                 /* +0x00 */
    int h0;                     /* +0x04 */
    int h1;                     /* +0x08 */
    unsigned char pad0c[0x38 - 0x0c];
};                              /* 0x38 */

struct TrackNode {
    int        state;           /* +0x00 */
    short      sx;              /* +0x04 */
    short      sy;              /* +0x06 */
    void*      cls;             /* +0x08 */
    TrackDesc* desc;            /* +0x0c */
    void*      owner;           /* +0x10 */
    TrackJoint jin;             /* +0x14 */
    TrackJoint jout;            /* +0x20 */
    unsigned char pad2c[0x50 - 0x2c];
};                              /* 0x50 */

struct CoasterRec {
    unsigned char pad00[0xa8];
    TrackNode*    head_node;    /* +0xa8 */
    unsigned char padac[0xc0 - 0xac];
    TrackNode*    tail_node;    /* +0xc0 */
};

/* Two aligned track cursors plus the roll pair between them. 0x0042a5e0
 * writes both cursors from one (at, t); 0x0042a680 draws with the rolls at
 * +0x18/+0x1c and copies the first cursor onto the second. */
typedef struct TrackCursorPair {
    float    t0;                /* +0x00 */
    RoutePos at0;               /* +0x04 */
    float    roll0;             /* +0x18 */
    float    roll1;             /* +0x1c */
    float    t1;                /* +0x20 */
    RoutePos at1;               /* +0x24 */
} TrackCursorPair;              /* 0x38 */

extern void  Vec3Cross(const Vec3f* a, const Vec3f* b, Vec3f* out); /* 0x00425cf0 */
extern void  Vec3Normalise(Vec3f* v);                               /* 0x00425d50 */
extern int   Vec3Equal(const Vec3f* a, const Vec3f* b);             /* 0x00425da0 */
extern int   TrackJointSloped(TrackDesc* d, int din, int dout);     /* 0x00429910 */
extern int   TrackRunStepsBack(TrackNode* n, TrackNode** endOut);   /* 0x00429940 */
extern int   TrackRunSteps(TrackNode* n, TrackNode** endOut);       /* 0x00429990 */
extern int   JointOppositeDir(int dir);                             /* 0x0041cc50 */
extern void  TrackCurve_EvaluateOffset(RoutePos* at, int mode, float t,
                                       float offset, Vec3f* out);   /* 0x00429bb0 */
extern void  TrackCurve_EvaluateDerivative(RoutePos* at, int mode, float t,
                                           float offset, Vec3f* out); /* 0x00429c60 */

extern RoutePos* g_curve_at;     /* 0x00615f84 */
extern int       g_curve_mode;   /* 0x00615f90 */
extern float     g_curve_offset; /* 0x00615fd4 */
extern RoutePos* g_dist_at;      /* 0x00615f80 */
extern int       g_dist_mode;    /* 0x00615ff0 */
extern float     g_dist_offset;  /* 0x00615ff4 */

/* Stub: the draw loop adds this to each roll and the retail body is a
 * pooled 0.0f. Both arguments are live at the call site and unused here. */
// FUNCTION: LEGOLAND 0x0042a670
float TrackCursorPair_GetRollDelta(TrackCursorPair* pair, int index)
{
    (void)pair;
    (void)index;
    return 0.0f;
}

/* geom->eval[mode].dir -- the +4 slot of each 8-byte vtable pair. Position
 * (0x00429a80) uses the +0 slot and then adds RoutePos.pos. */
// FUNCTION: LEGOLAND 0x00429ac0
void TrackCurve_EvalVtable(RoutePos* at, int mode, float t, Vec3f* out)
{
    RouteGeom* geom = at->geom;
    geom->eval[mode].dir(geom, t, out);
}

/* Gram-Schmidt basis from a track tangent; twin of MakeRotation
 * (coaster6.c 0x00426560). */
// FUNCTION: LEGOLAND 0x00429af0
void TrackCurve_MakeBasis(const Vec3f* dir, Mat3* out)
{
    int i;

    out->r[2].x = 0.0f;
    out->r[2].y = 0.0f;
    out->r[2].z = 1.0f;
    Vec3Cross(&out->r[2], dir, &out->r[1]);
    Vec3Cross(dir, &out->r[1], &out->r[2]);
    out->r[0] = *dir;
    for (i = 0; i < 3; i++)
        Vec3Normalise(&out->r[i]);
}

/* Tangent via vtable slot 1, then the basis above. */
// FUNCTION: LEGOLAND 0x00429b60
void TrackCurve_EvaluateBasis(RoutePos* at, float t, Mat3* out)
{
    Vec3f dir;
    TrackCurve_EvalVtable(at, 1, t, &dir);
    TrackCurve_MakeBasis(&dir, out);
}

/* Copy one (at, t) into both cursor slots. The rolls at +0x18/+0x1c are
 * left alone. */
// FUNCTION: LEGOLAND 0x0042a5e0
void TrackCursorPair_Init(TrackCursorPair* d, const RoutePos* at, float t)
{
    d->at0 = *at;
    d->t0 = t;
    d->at1 = *at;
    d->t1 = t;
}

/* Node and geom identity, then the three-float compare at 0x00425da0. */
// FUNCTION: LEGOLAND 0x0042a110
int RoutePos_Equal(const RoutePos* a, const RoutePos* b)
{
    if (a->node != b->node)
        return 0;
    if (a->geom != b->geom)
        return 0;
    return Vec3Equal(&a->pos, &b->pos) != 0;
}

/* Walk forward levelling both joint heights until a sloped joint or `end`. */
// FUNCTION: LEGOLAND 0x00429690
TrackNode* TrackRunSetLevel(TrackNode* n, float z, TrackNode* end)
{
    while (!TrackJointSloped(n->desc, n->jin.dir, n->jout.dir) && n != end) {
        n->jin.h = z;
        n->jout.h = z;
        n = n->jout.node;
    }
    return n;
}

/* Span-piece height check: sloped joints on either partner run, plus the
 * new descriptor against the two facing direction bits. Named opposite
 * results keep the first in ebx across the second call so the five
 * cdecls share one `add esp,0x24`. */
// FUNCTION: LEGOLAND 0x004298a0
int TrackFitSpanGeom(TrackDesc* d, CoasterRec* rec, TrackNode** an, TrackNode** bn)
{
    int n;
    int head_opp;
    int tail_opp;

    n = TrackRunStepsBack(rec->head_node, an);
    n += TrackRunSteps(rec->tail_node, bn);
    head_opp = JointOppositeDir(rec->head_node->jout.dir);
    tail_opp = JointOppositeDir(rec->tail_node->jin.dir);
    if (TrackJointSloped(d, head_opp, tail_opp))
        n++;
    return n > 0;
}

/* Solver sample used as the 0x00429c10 pointer inside
 * TrackCurve_EvaluateDerivative: offset-evaluate the stashed cursor into a
 * 3-wide PhysVec. */
// FUNCTION: LEGOLAND 0x00429c10
void TrackCurve_SolverSample(float t, PhysVec* out)
{
    Vec3f tmp;
    TrackCurve_EvaluateOffset(g_curve_at, g_curve_mode, t, g_curve_offset, &tmp);
    out->v[0] = tmp.x;
    out->v[1] = tmp.y;
    out->v[2] = tmp.z;
    out->n = 3;
}

/* |d/dt| of the stashed distance-measure cursor. fsqrt of the three-term
 * sum of squares; the 0.0f accumulator is stored before the call. */
// FUNCTION: LEGOLAND 0x0042a150
float Track_AbsDerivative(float t)
{
    Vec3f d;
    float sum = 0.0f;
    float* v;
    int i;

    TrackCurve_EvaluateDerivative(g_dist_at, g_dist_mode, t, g_dist_offset, &d);
    v = (float*)&d;
    for (i = 0; i < 3; i++)
        sum += v[i] * v[i];
    return (float)sqrt(sum);
}
