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

#pragma intrinsic(memcpy, memset, sqrt, sin, cos)

typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct Mat3 { Vec3f r[3]; } Mat3;
typedef struct Mat4 { float m[16]; } Mat4;
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
    Vec3f         pos;          /* +0x00 */
    Vec3f         dir;          /* +0x0c */
    Vec3f         offset;       /* +0x18 */
    unsigned char pad24[0x40 - 0x24];
    float         length;       /* +0x40 */
    float         t0;           /* +0x44 */
    float         t1;           /* +0x48 */
    GeomEval*     eval;         /* +0x4c */
    struct RouteGeom* next;     /* +0x50 */
    struct RouteGeom* prev;     /* +0x54 */
};                              /* 0x58 */

struct RoutePos {
    TrackNode*  node;           /* +0x00 */
    RouteGeom*  geom;           /* +0x04 */
    Vec3f       pos;            /* +0x08 */
};                              /* 0x14 */

typedef struct TrackCursor {
    float    t;
    RoutePos at;
} TrackCursor;                  /* 0x18 */

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
    unsigned char pad2c[0x40 - 0x2c];
    int        clear40[3];      /* +0x40 */
    RouteGeom  geom;            /* +0x4c  t0 at +0x90, t1 at +0x94 */
};                              /* 0xa4 */

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
extern int   JointDir_ToIndex(int direction);                         /* 0x0041cca0 */
extern void  MapSquareToWorld(const short* sq, float h, Vec3f* out);  /* 0x00425cb0 */
extern void  TrackGeom_BuildRamp(Vec3f* p0, Vec3f* p1, const Vec3f* half,
                                 RouteGeom* out);                     /* 0x00422180 */

extern Vec3f g_joint_world[4];   /* 0x004b6398 */
extern Vec3f g_joint_half[4];    /* 0x004b63c8 */
extern int   g_stat_c_615fc4;    /* 0x00615fc4 */
extern int   g_stat_c_615fc8;    /* 0x00615fc8 */
extern int   g_stat_c_615fcc;    /* 0x00615fcc */
extern int   g_bisect_max;       /* 0x00615fec */
extern Vec3f* g_step_origin;     /* 0x00615f8c */
extern float g_step_len;         /* 0x00615fd0 */
extern float g_step_len2;        /* 0x00615fd8 */
extern float g_step_hi2;         /* 0x00615fdc */
extern float g_step_lo2;         /* 0x00615fe0 */
extern int   g_step_far;         /* 0x00615fe4 */
extern int   g_step_up;          /* 0x00615fe8 */
extern int (*g_track_solver)(float (*fn)(float), float lo, float hi, float* out); /* 0x004b63fc */

extern void  TrackCurve_EvaluatePosition(RoutePos* at, int mode, float t,
                                         Vec3f* out);                     /* 0x00429a80 */
extern void  TrackCurve_EvaluateUp(RoutePos* at, int mode, float t,
                                   Vec3f* out);                           /* 0x00429b90 */
extern void  TrackCursor_RetreatGeometry(RoutePos* p);                    /* 0x0041f880 */
extern void  TrackCursor_AdvanceGeometry(RoutePos* p);                    /* 0x0041f850 */
extern void  TrackCursor_Evaluate(TrackCursor* c, int mode, Vec3f* out);  /* 0x0042a640 */
extern float Track_Integrate(float (*fn)(float), float a, float b, float tol); /* 0x00420200 */
extern void  Coaster3D_SetCarClipDepth(void);                             /* 0x00425c40 */
extern void  Coaster3D_DrawModel(void* mesh, void* tex, const Vec3f* pos,
                                 const Mat3* rot, int mode);              /* 0x00420e90 */
extern void  Mat3_ToMat4(const Mat3* src, Mat4* dst);                     /* 0x00426490 */
extern void  Mat4_ToMat3(Mat3* dst, const Mat4* src);                     /* 0x00426460 */
extern void  MatIdentity(Mat4* m);                                        /* 0x004260f0 */
extern void  MatMul(const Mat4* a, const Mat4* b, Mat4* out);             /* 0x00426120 */
extern void* g_wheel_a;                                                   /* 0x00616000 */
extern void* g_wheel_b;                                                   /* 0x00616004 */

extern void* CoasterTex_Get(int tag);                                     /* 0x00420780 */
extern int   BitLowestSet(int mask);                                      /* 0x00428840 */
extern void* g_raster_bits;                                               /* 0x004b5b20 */
extern short* g_zb_base;                                                  /* 0x004b5b24 */
extern int    g_zb_pitch;                                                 /* 0x004b5b28 */
extern int    g_zb_polys;                                                 /* 0x0060f900 */
extern int    g_shade_count;                                              /* 0x0060f904 */
extern void*  g_shade_tab[];                                              /* 0x00829c60 */
extern int    g_span_mask;                                                /* 0x0061195c */
extern short  g_span_lut[];                                               /* 0x00611960 */
extern int    g_span_dshade;                                              /* 0x00612160 */
extern int    g_span_dv;                                                  /* 0x00612164 */
extern int    g_span_dz;                                                  /* 0x0061216c */
extern int    g_span_tmask;                                               /* 0x00612174 */

typedef struct SortKey {
    int y;
    int idx;
} SortKey;

typedef struct SpanEdge {
    short y0;               /* +0x00 */
    short y1;               /* +0x02 */
    int   dir;              /* +0x04 */
    int   a[5];             /* +0x08 */
    int   d[5];             /* +0x1c */
} SpanEdge;                 /* 0x30 */

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

/* Build one ramp geom from the span's world endpoints and stamp it onto
 * every piece with parameter ranges [i/n, (i+1)/n]. z/dz only place the
 * endpoints; the per-piece +0x90/+0x94 slots are the parameter, not height. */
// WIP-FUNCTION: LEGOLAND 0x00429560  (90/90i, 302/302B, 7 eax/edx)
void TrackRunSetSlope(TrackNode* n, TrackNode* e, int steps, float z, float dz)
{
    RouteGeom geom;
    Vec3f p0;
    Vec3f p1;
    float t;
    float inv;
    int i0;
    int i1;

    i0 = JointDir_ToIndex(n->jin.dir);
    i1 = JointDir_ToIndex(e->jout.dir);
    inv = 1.0f / (float)steps;
    t = 0.0f;
    MapSquareToWorld(&n->sx, z, &p0);
    p0.x += g_joint_world[i0].x;
    p0.y += g_joint_world[i0].y;
    MapSquareToWorld(&e->sx, z + dz, &p1);
    p1.x += g_joint_world[i1].x;
    p1.y += g_joint_world[i1].y;
    TrackGeom_BuildRamp(&p0, &p1, &g_joint_half[i0], &geom);
    if (steps > 0) {
        do {
            n->state |= 1;
            n->geom = geom;
            n->geom.t0 = t;
            t += inv;
            n->geom.t1 = t;
            n->clear40[0] = 0;
            n->clear40[1] = 0;
            n->clear40[2] = 0;
            n = n->jout.node;
        } while (--steps);
    }
}

typedef float (*TrackSampleFn)(float);

/* Default [0x004b63fc] hook: bisection on a sign-changing sample. Same-sign
 * endpoints return 0; otherwise the midpoint of the final bracket. */
// FUNCTION: LEGOLAND 0x00429e20
int Track_Bisect(TrackSampleFn fn, float lo, float hi, float* out)
{
    float pa;
    float pb;
    float mid;
    float pm;
    int n;

    g_stat_c_615fc4++;
    g_stat_c_615fc8++;
    n = 1;
    pa = fn(lo);
    pb = fn(hi);
    if (((*(unsigned*)&pa ^ *(unsigned*)&pb) & 0x80000000) == 0)
        return 0;
    if (hi - lo > 0.00499999988f) {
        do {
            mid = (lo + hi) * 0.5f;
            pm = fn(mid);
            if ((*(unsigned*)&pm ^ *(unsigned*)&pa) & 0x80000000)
                hi = mid;
            else {
                lo = mid;
                pa = pm;
            }
            g_stat_c_615fc8++;
            n++;
        } while (hi - lo > 0.00499999988f);
    }
    *out = (lo + hi) * 0.5f;
    if (n > g_bisect_max)
        g_bisect_max = n;
    return 1;
}

/* Bisection objective for Track_StepAlong: |pos(t) - origin - offset*up|^2
 * minus step^2, with early +1/-1 when the raw |pos-origin| is outside
 * step±tol. */
// FUNCTION: LEGOLAND 0x00429cf0
float Track_StepObjective(float t)
{
    Vec3f pos;
    Vec3f up;
    float dist2;

    g_stat_c_615fcc++;
    TrackCurve_EvaluatePosition(g_curve_at, 1, t, &pos);
    pos.x -= g_step_origin->x;
    pos.y -= g_step_origin->y;
    pos.z -= g_step_origin->z;
    if ((dist2 = pos.x * pos.x + pos.y * pos.y + pos.z * pos.z) > g_step_hi2) {
        g_step_far++;
        return 1.0f;
    }
    if (g_step_len > g_curve_offset && dist2 < g_step_lo2)
        return -1.0f;
    TrackCurve_EvaluateUp(g_curve_at, 1, t, &up);
    g_step_up++;
    pos.x -= g_curve_offset * up.x;
    pos.y -= g_curve_offset * up.y;
    pos.z -= g_curve_offset * up.z;
    return pos.x * pos.x + pos.y * pos.y + pos.z * pos.z - g_step_len2;
}

/* Walk backward along the track until |pos - origin| == step. First try
 * the current geom's [t0, t]; then retreat and try each prior [t0, t1]. */
// WIP-FUNCTION: LEGOLAND 0x00429f30  (70/70i, 227/232B, solver push schedule)
void Track_StepAlong(Vec3f* origin, float step, RoutePos* from, float t,
                     float tol, RoutePos* out, float* out_t)
{
    RoutePos cur;
    float step2 = step * step;
    float t0;

    cur = *from;
    t0 = cur.geom->t0;
    g_step_len2 = step2;
    g_step_len = step;
    g_step_origin = origin;
    g_curve_offset = tol;
    g_curve_at = &cur;
    g_step_hi2 = (step + tol) * (step + tol);
    g_step_lo2 = (step - tol) * (step - tol);
    if (!g_track_solver(Track_StepObjective, t0, t, out_t)) {
        do {
            TrackCursor_RetreatGeometry(&cur);
            t0 = cur.geom->t0;
        } while (!g_track_solver(Track_StepObjective, t0, cur.geom->t1, out_t));
    }
    *out = cur;
}

/* Track distance from (to, t2) walking forward to (from, t). Same-piece
 * path integrates once; otherwise sum the tail of `to`, each full geom,
 * and the head of `from`. mode/offset stash the AbsDerivative cursor. */
// FUNCTION: LEGOLAND 0x0042a1b0
float Track_MeasureDistance(RoutePos* from, float t, RoutePos* to, float t2,
                            int mode, float offset)
{
    RoutePos cur;
    float sum;

    g_dist_mode = mode;
    g_dist_offset = offset;
    if (RoutePos_Equal(from, to)) {
        g_dist_at = from;
        return Track_Integrate(Track_AbsDerivative, t2, t, 0.01f);
    }
    cur = *to;
    g_dist_at = &cur;
    sum = Track_Integrate(Track_AbsDerivative, t2, cur.geom->t1, 0.01f);
    TrackCursor_AdvanceGeometry(&cur);
    while (!RoutePos_Equal(&cur, from)) {
        g_dist_at = &cur;
        sum += Track_Integrate(Track_AbsDerivative, cur.geom->t0, cur.geom->t1, 0.01f);
        TrackCursor_AdvanceGeometry(&cur);
    }
    g_dist_at = &cur;
    sum += Track_Integrate(Track_AbsDerivative, cur.geom->t0, t, 0.01f);
    return sum;
}

/* Draw the two wheel models at the pair's first cursor, modes 0 and 1,
 * each rolled by roll[i] + GetRollDelta. Then copy the first cursor onto
 * the second slot. */
// FUNCTION: LEGOLAND 0x0042a680
void TrackCursorPair_Draw(TrackCursorPair* p)
{
    Mat3 basis;
    Mat4 world;
    Mat4 ident;
    Mat4 product;
    Mat3 drawn;
    Vec3f pos;
    int i;

    Coaster3D_SetCarClipDepth();
    TrackCurve_EvaluateBasis(&p->at0, p->t0, &basis);
    Mat3_ToMat4(&basis, &world);
    for (i = 0; i <= 1; i++) {
        float* roll = &p->roll0 + i;
        float s;
        float c;

        *roll += TrackCursorPair_GetRollDelta(p, i);
        TrackCursor_Evaluate((TrackCursor*)p, i, &pos);
        MatIdentity(&ident);
        s = (float)sin(*roll);
        ident.m[0] = s;
        c = (float)cos(*roll);
        ident.m[2] = c;
        ident.m[8] = -c;
        ident.m[10] = s;
        MatMul(&world, &ident, &product);
        Mat4_ToMat3(&drawn, &product);
        Coaster3D_DrawModel(g_wheel_a, g_wheel_b, &pos, &drawn, 0);
    }
    p->t1 = p->t0;
    p->at1 = p->at0;
}

/* Shaded z-buffer span filler (table 0x004b5f50). Sibling of
 * ZBuffer_FillPoly (schoolcar6.c): EBP frame, `xchg ebx,eax`, `add ebx,1`.
 * Raster_SubmitPoly calls this as shader[mode](tag, grad, ne, keys, edges).
 * 0x00420780 / 0x00428840 are owned by LL4 / LL6.
 *
 * Frame is 0x70: four 0x14 interpolant records plus the eight setup dwords
 * (y, last-key, two shifts, ylast, pitch, zrow, crow). */
typedef struct ShadeInterp {
    int x;
    int rest[4];
} ShadeInterp;                  /* 0x14 */

// WIP-FUNCTION: LEGOLAND 0x00428860  (254/254i, 748/771B, frame 0x6c vs 0x70; mixed C+__asm)
void TrackShade_FillPoly(int tag, int* grad, int nkeys, SortKey* keys, SpanEdge* edges)
{
    ShadeInterp ed[4];
    int         y;
    int         ylast;
    int         pitch;
    int         shift0;
    int         shift1;
    SortKey*    last;
    short*      zrow;
    short*      crow;
    char*       tex;
    int         bit0;
    int         bit1;
    int         nshade;
    short*      yp;
    char*       lut;

    y = keys[0].y;
    last = &keys[nkeys - 1];
    yp = &edges[last->idx].y1;
    (*yp)++;
    ylast = edges[last->idx].y1;
    crow = (short*)g_raster_bits;
    zrow = g_zb_base;
    pitch = g_zb_pitch;
    tex = (char*)CoasterTex_Get(tag);
    if (tex == 0)
        return;
    bit0 = BitLowestSet(*(int*)tex);
    bit1 = BitLowestSet(((int*)tex)[1]);
    tex += 8;
    tag = (int)tex;
    shift0 = 8 - bit0;
    shift1 = shift0 - bit1 + 8;
    g_span_mask = (int)0xff000000 >> shift0;
    g_span_dshade = grad[1] >> shift0;
    g_span_dv = (grad[2] << 8) >> shift1;
    g_span_dz = grad[3];
    g_zb_polys++;
    g_span_tmask = (1 << (bit0 + bit1)) - 1;
    keys[nkeys].y = ylast;
    crow += pitch * y;
    zrow += pitch * y;
    nshade = g_shade_count;
    if (nshade > 0) {
        void** src = g_shade_tab;
        short* dst = g_span_lut;
        int    idx = grad[0];

        do {
            *dst++ = ((short*)*src++)[idx];
        } while (--nshade);
    }
    lut = (char*)g_span_lut;
    do {
        SpanEdge* e = &edges[keys->idx];

        keys++;
        if (e->dir) {
            ed[3].x = e->d[0];
            ed[2].x = e->a[0] - e->d[0];
        } else {
            ed[1].x = e->d[0];
            ed[0].x = e->a[0] - e->d[0];
            ed[1].rest[2] = e->d[1];
            ed[0].rest[2] = e->a[1] - e->d[1];
            ed[1].rest[3] = e->d[2];
            ed[0].rest[3] = e->a[2] - e->d[2];
            ed[1].rest[1] = e->d[3];
            ed[0].rest[1] = e->a[3] - e->d[3];
        }
        while (y < keys->y) {
            int span;

            y++;
            ed[0].x += ed[1].x;
            ed[2].x += ed[3].x;
            span = ed[2].x - ed[0].x;
            if (span >= 0x8000) {
                __asm {
                    mov  eax, ed[0]
                    mov  ebx, ed[40]
                    sar  eax, 16
                    sar  ebx, 16
                    mov  edi, crow
                    mov  esi, zrow
                    xchg ebx, eax
                    sub  ebx, eax
                    lea  edi, [edi + eax*2]
                    lea  esi, [esi + eax*2]
                    push ebx
                    mov  eax, ed[12]
                    mov  edx, ed[16]
                    mov  ebx, ed[8]
                    add  eax, ed[32]
                    add  edx, ed[36]
                    add  ebx, ed[28]
                    mov  ed[12], eax
                    mov  ed[16], edx
                    mov  ed[8], ebx
                    mov  ecx, shift1
                    shl  edx, 8
                    shr  edx, cl
                    mov  ecx, shift0
                    sar  eax, cl
                    push ebp
                    sub  esp, 8
                    mov  ecx, esi
                    sub  ecx, dword ptr tag
                    mov  dword ptr [esp + 4], ecx
                    mov  ecx, esi
                    sub  ecx, dword ptr lut
                    sar  ecx, 1
                    mov  dword ptr [esp], ecx
                    mov  ebp, ebx
                    mov  ebx, dword ptr [esp + 0xc]
                pix:
                    mov  ecx, ebp
                    sar  ecx, 16
                    cmp  cx, word ptr [esi + ebx*2]
                    jb   skip
                    mov  word ptr [esi + ebx*2], cx
                    mov  ecx, g_span_mask
                    and  ecx, edx
                    or   ecx, eax
                    shr  ecx, 16
                    and  ecx, g_span_tmask
                    sub  ecx, dword ptr [esp + 4]
                    mov  cl, byte ptr [esi + ecx]
                    and  ecx, 0xff
                    sub  ecx, dword ptr [esp]
                    mov  cx, word ptr [esi + ecx*2]
                    mov  word ptr [edi + ebx*2], cx
                skip:
                    add  eax, g_span_dshade
                    add  edx, g_span_dv
                    add  ebp, g_span_dz
                    add  ebx, 1
                    jle  pix
                    add  esp, 8
                    pop  ebp
                    add  esp, 4
                }
            } else {
                ed[0].rest[2] += ed[1].rest[2];
                ed[0].rest[3] += ed[1].rest[3];
                ed[0].rest[1] += ed[1].rest[1];
            }
            crow += pitch;
            zrow += pitch;
        }
    } while (y < ylast);
}
