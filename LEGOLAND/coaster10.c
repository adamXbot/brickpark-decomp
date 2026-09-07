/* LEGOLAND -- Codex-F: coaster draw-pass and route/curve callees.
 * VC6 SP3 /O2 /Gy /Gd. Local types describe the original 32-bit layouts.
 * Verification and recovered mechanics: docs/lanes/codex-f.md.
 */
typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct Mat3 { float m[9]; } Mat3;
typedef struct Mat4 { float m[16]; } Mat4;
typedef struct Transform { Vec3f pos; Mat3 rot; } Transform;
typedef struct ClipRect {
    int left, top, right, bottom, mask;
    struct ClipRect *prev, *next;
} ClipRect;
typedef struct SpanRect { int top, left, bottom, right; } SpanRect;
typedef struct RoutePos { void* node; void* geom; Vec3f pos; } RoutePos;
typedef struct NodeCursor { char pad00[4]; void* ref; char pad08[0x30]; } NodeCursor; /* 0x38 */
typedef struct RouteSeat {
    Vec3f pos;
    char pad0c[0x1c - 0x0c];
    void (__cdecl *apply)(void* seat, Transform* xf);
} RouteSeat; /* 0x20 */
typedef struct RouteNode {
    unsigned flags;
    int kind;
    NodeCursor cursor0;                 /* +0x08 */
    NodeCursor cursor1;                 /* +0x40 */
    RouteSeat seat0;                    /* +0x78 */
    RouteSeat seat1;                    /* +0x98 */
    char padb8[0xc8 - 0xb8];
    ClipRect clip;                      /* +0xc8 */
    struct RouteNode *prev, *next;
} RouteNode;
typedef struct CoasterRoute {
    int flags, tick, deadline;
    RoutePos at;
    int dimension;
    float parameter;
    float energy;                       /* +0x28 */
    char pad2c[0x70 - 0x2c];
    RouteNode head;
} CoasterRoute;
typedef struct ModelImage { char* data; int length; } ModelImage;
/* geom+0x4c: method table — position at mode*8, up at +0x1c. */
typedef struct RouteGeom {
    char pad00[0x4c];
    void* vt;
} RouteGeom;

extern int g_view_left;                            /* 0x008299ac */
extern void* g_route_node_model[];                 /* 0x0082add0 */
extern void* g_route_node_texture[];               /* 0x0082ade0 */

extern void MakeTransform(const Vec3f*, const Mat3*, Mat4*); /* 0x004264e0 */
extern void Mat4_Transpose(const Mat4*, Mat4*);              /* 0x00426190 */
extern int ClipRect_ComputeMask(ClipRect*, const int*);      /* 0x004265d0 */
extern void ModelClip_Project(void* mesh, const Vec3f*, const Mat3*, SpanRect*); /* 0x00426750 */
extern float Route_SumPotentialEnergy(CoasterRoute*);        /* 0x0041dae0 */
extern void Route_GetMassAndPower(CoasterRoute*, float*, float*); /* 0x0041db90 */
extern void TrackCursor_Resolve(NodeCursor*);                /* 0x0042a680 */
extern void RouteNode_GetTransform(RouteNode*, Transform*);  /* 0x0041e9e0 */
extern void Coaster3D_DrawModel(void*, void*, const Vec3f*, const Mat3*, int); /* 0x00420e90 */
extern void Coaster3D_SetCarClipDepth(void);                 /* 0x00425c40 */
extern void TrackCursor_Evaluate(NodeCursor*, int, Vec3f*);  /* 0x0042a640 */
extern void Mat3_BuildBasis(Mat3* forward, Mat3* out);           /* 0x00429af0 */
extern int ModelRecord_GetName(ModelImage*, char*, int);     /* 0x00422390 */
extern int _stricmp(const char*, const char*);               /* 0x004aab90 */
extern void TrackCurve_EvaluateTangent(RoutePos*, int mode, int t, Vec3f*); /* 0x00429ac0 */
extern void FiniteDifference(void (*fn)(void), void* ctx, int t, float h, void* out); /* 0x0041f4e0 */
extern void TrackCurve_DerivSample(void);                    /* 0x00429c10 */
extern double sqrt(double);

extern RoutePos* g_deriv_at;                       /* 0x00615f84 */
extern int g_deriv_mode;                           /* 0x00615f90 */
extern int g_deriv_t;                              /* 0x00615f98 */
extern Vec3f* g_deriv_out;                         /* 0x00615fd4 */

/* ------------------------------------------------------------------------- */
// FUNCTION: LEGOLAND 0x0041e640
void RouteNode_SetPending(RouteNode* node, int pending)
{
    if (pending == 1)
        node->flags |= 1u;
    else
        node->flags &= ~1u;
}

/* Method-table slot +0x1c; mode is unused (caller still passes it). */
// FUNCTION: LEGOLAND 0x00429b90
void TrackCurve_EvaluateUp(RoutePos* at, int mode, int t, Vec3f* out)
{
    RouteGeom* geom = (RouteGeom*)at->geom;
    (void)mode;
    (*(void (__cdecl **)(void*, int, Vec3f*))((char*)geom->vt + 0x1c))(geom, t, out);
}

// FUNCTION: LEGOLAND 0x00420fb0
void CoasterModel_GetClipRect(void* model, Vec3f* pos, Mat3* rot, SpanRect* out)
{
    ModelClip_Project((char*)model + 0x30, pos, rot, out);
}

// FUNCTION: LEGOLAND 0x0041e670
int RouteNode_CanAdd(RouteNode* node, void* target)
{
    if (node->cursor0.ref == target)
        return 1;
    return node->cursor1.ref == target;
}

// FUNCTION: LEGOLAND 0x00426510
void Mat3_TransposeToMat4(const Mat3* rot, Mat4* out)
{
    Vec3f zero;
    Mat4 tmp;
    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;
    MakeTransform(&zero, rot, &tmp);
    Mat4_Transpose(&tmp, out);
}

/* Aggregate assign of the four bounds emits the original ecx dest alias. */
// FUNCTION: LEGOLAND 0x00426700
void ClipRect_SetBounds(ClipRect* clip, const SpanRect* bounds)
{
    *(SpanRect*)&clip->left = *bounds;
    clip->mask = ClipRect_ComputeMask(clip, &g_view_left);
}

/* Method table[mode] (stride 8), then add the route-pos translation. */
// FUNCTION: LEGOLAND 0x00429a80
void TrackCurve_EvaluatePosition(RoutePos* at, int mode, int t, Vec3f* out)
{
    RouteGeom* geom = (RouteGeom*)at->geom;
    (*(void (__cdecl **)(void*, int, Vec3f*))((char*)geom->vt + mode * 8))(geom, t, out);
    out->x += at->pos.x;
    out->y += at->pos.y;
    out->z += at->pos.z;
}

/* v = sqrt(2*(E - PE)/m), clamped at 0. Bare fsqrt; power out-param unused. */
// FUNCTION: LEGOLAND 0x0041dca0
float Route_GetSpeed(CoasterRoute* route)
{
    float pe;
    float mass, power;
    float v;
    pe = Route_SumPotentialEnergy(route);
    Route_GetMassAndPower(route, &mass, &power);
    v = 2.0f * (route->energy - pe) / mass;
    if (v > 0.0f)
        return (float)sqrt(v);
    return 0.0f;
}








/* Transform `count` vectors by Mat4 (row-major, translation in m[3/7/11]). */


/* Transform `count` vectors by Mat4 (row-major, translation in m[3/7/11]). */


/* Transform `count` vectors by Mat4 (row-major, translation in m[3/7/11]).
 * Residual: out+=12 spills the out slot; original keeps edi live without a
 * writeback (86.7%, ESCAPES). Same shape as TransformVerts without clip. */
// WIP-FUNCTION: LEGOLAND 0x004261c0  (86.7%, out-pointer spill vs live edi)
void TransformVec3(const Vec3f* s, Vec3f* d, const Mat4* m, int count)
{
    int j, k;
    while (count-- > 0) {
        const float* row = m->m;
        float* o = (float*)d;
        for (k = 0; k < 3; k++) {
            float acc = 0.0f;
            const float* sp = (const float*)s;
            const float* rp = row;
            for (j = 0; j < 3; j++)
                acc += *rp++ * sp[j];
            acc += row[3];
            *o = acc;
            row += 4;
            o++;
        }
        s = (const Vec3f*)((const char*)s + 12);
        d = (Vec3f*)((char*)d + 12);
    }
}

// FUNCTION: LEGOLAND 0x00422400
int ModelImage_FindName(ModelImage* image, const char* name)
{
    char buf[256];
    int i = 0;
    while (ModelRecord_GetName(image, buf, i)) {
        if (_stricmp(name, buf) == 0)
            return i;
        i++;
    }
    return -1;
}
// FUNCTION: LEGOLAND 0x0041e9e0
void RouteNode_GetTransform(RouteNode* node, Transform* out)
{
    Vec3f a, b;
    TrackCursor_Evaluate(&node->cursor0, 2, &a);
    TrackCursor_Evaluate(&node->cursor1, 2, &b);
    out->pos.x = (a.x + b.x) * 0.5f;
    out->pos.y = (a.y + b.y) * 0.5f;
    out->pos.z = (a.z + b.z) * 0.5f;
    out->rot.m[0] = a.x - b.x;
    out->rot.m[1] = a.y - b.y;
    out->rot.m[2] = a.z - b.z;
    Mat3_BuildBasis(&out->rot, &out->rot);
}

/* Resolve both track cursors, build the node transform, draw the kind's
 * model, then run both seat apply hooks. SetCarClipDepth runs first. */
// FUNCTION: LEGOLAND 0x0041ea70
void RouteNode_LinkPending(RouteNode* node)
{
    Transform xf;
    Coaster3D_SetCarClipDepth();
    TrackCursor_Resolve(&node->cursor0);
    TrackCursor_Resolve(&node->cursor1);
    RouteNode_GetTransform(node, &xf);
    Coaster3D_DrawModel(
        g_route_node_model[node->kind],
        g_route_node_texture[node->kind],
        &xf.pos,
        &xf.rot,
        0);
    node->seat0.apply(&node->seat0, &xf);
    node->seat1.apply(&node->seat1, &xf);
}

/* h == 0: direct tangent via method table[mode*8+4].
 * h != 0: finite-difference of EvaluateOffset through 0x0041f4e0.
 * Residual: FD call arg schedule and PhysVec→Vec3f copy (stores to
 * g_deriv_* interleaved with pushes) not yet recovered. */
// WIP-FUNCTION: LEGOLAND 0x00429c60  (55%, FD schedule / result copy)
void TrackCurve_EvaluateDerivative(RoutePos* at, int mode, int t, float h, Vec3f* out)
{
    if (h != 0.0f) {
        char scratch[0x54];
        g_deriv_at = at;
        g_deriv_mode = mode;
        g_deriv_out = out;
        FiniteDifference(TrackCurve_DerivSample, (void*)0x00615f98, t, 0.01f, scratch);
        out->x = ((float*)scratch)[1];
        out->y = ((float*)scratch)[2];
        out->z = ((float*)scratch)[3];
    } else {
        TrackCurve_EvaluateTangent(at, mode, t, out);
    }
}
