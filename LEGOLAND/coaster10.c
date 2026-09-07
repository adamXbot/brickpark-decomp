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
typedef struct PhysVec { int n; float v[20]; } PhysVec;      /* pool slot, 0x54 (coaster8.c) */
extern void FiniteDifference(void (*fn)(int t, PhysVec* out), void* ops, int t, float h, PhysVec* out); /* 0x0041f4e0 */
extern void TrackCurve_DerivSample(int t, PhysVec* out);     /* 0x00429c10 */
extern double sqrt(double);

/* Finite-difference sample context read by TrackCurve_DerivSample. */
extern RoutePos* g_deriv_at;                       /* 0x00615f84 */
extern int g_deriv_mode;                           /* 0x00615f90 */
extern float g_deriv_offset;                       /* 0x00615fd4; EvaluateOffset's `offset` */
extern int g_deriv_physvec_ops;                    /* 0x00615f98; PhysVec_InitOps(&, 3) pool (schoolcar.c: g_615f98) */


typedef struct ScreenVtx { int x, y, z, clip; } ScreenVtx; /* 0x10 */
typedef struct ModelFace {
    short mat;          /* +0x00 */
    short normal;       /* +0x02  face normal (pass1/3) */
    short v0, v1, v2;   /* +0x04 / +0x06 / +0x08 */
    short n0, n1, n2;   /* +0x0a / +0x0c / +0x0e  per-vertex normals (pass2) */
} ModelFace; /* 0x10 */
typedef struct CoasterMesh {
    int count;
    char pad04[0xc];
    Vec3f* normals;                 /* +0x10 */
    Vec3f* normals2;                /* +0x14 */
    ModelFace* faces1;              /* +0x18 */
    int nfaces1;                    /* +0x1c */
    ModelFace* faces2;              /* +0x20 */
    int nfaces2;                    /* +0x24 */
    ModelFace* faces3;              /* +0x28 */
    int nfaces3;                    /* +0x2c */
} CoasterMesh;
typedef struct PolyVtx {
    int f00;
    int sy;
    int sx;
    int shade;
    int sz;
    int f14;
    int f18;
} PolyVtx; /* 0x1c */
typedef struct PolyJob {
    int kind;
    int tag;
    int and_flags;
    int or_flags;
    int shade;
    float dx1, dx2, dy1, dy2, area;
    PolyVtx* v[3];
    int f34;
    void* shader;
} PolyJob; /* 0x3c */

extern ScreenVtx g_model_vertices[];           /* 0x004d8bb8 */
extern Vec3f g_model_light;                    /* 0x004dcbb8 */
extern float g_view_half_hi;                   /* 0x00829a60 */
extern int g_stat_c_4dcbc8;                    /* 0x004dcbc8 */
extern void Raster_SubmitPoly(int nverts, PolyJob* job); /* 0x0042a2f0 */

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








/* Transform `count` vectors by Mat4 (row-major, translation in m[3/7/11]).
 * MatMul house style (coastermath.c): BOTH operands are direct subscripts
 * with the row index folded in (`m->m[k*4+j] * ((float*)s)[j]`) and the
 * output is `((float*)d)[k]` with `d++` after the k loop. That is what
 * yields ebp=s / ebx=k-count, the sp-then-rp preheader order and
 * `fld [rp]` first; every walked-pointer spelling (`*rp++`, `*sp++`, a
 * `float* d` cursor) permutes one of the three. */
// FUNCTION: LEGOLAND 0x004261c0
void TransformVec3(const Vec3f* s, Vec3f* d, const Mat4* m, int count)
{
    int j, k;
    while (count-- > 0) {
        for (k = 0; k < 3; k++) {
            float acc = 0.0f;
            for (j = 0; j < 3; j++)
                acc += m->m[k * 4 + j] * ((const float*)s)[j];
            acc += m->m[k * 4 + 3];
            ((float*)d)[k] = acc;
        }
        s++;
        d++;
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
 * h != 0: numerical derivative of EvaluateOffset(at, mode, t, h) through
 * FiniteDifference(0x0041f4e0) on the 3-dim PhysVec pool, step 0.01.
 * 0x615fd4 is the float `offset` handed to DerivSample (a `mov` of h's raw
 * bits), not an out pointer; the out vector is re-read from its home after
 * the call, and the three-float copy uses fld/fstp for v[0] because eax
 * holds `out` and only ecx/edx remain for v[1]/v[2]. */
// FUNCTION: LEGOLAND 0x00429c60
void TrackCurve_EvaluateDerivative(RoutePos* at, int mode, int t, float h, Vec3f* out)
{
    PhysVec scratch;
    if (h != 0.0f) {
        g_deriv_at = at;
        g_deriv_mode = mode;
        g_deriv_offset = h;
        FiniteDifference(TrackCurve_DerivSample, &g_deriv_physvec_ops, t, 0.01f, &scratch);
        out->x = scratch.v[0];
        out->y = scratch.v[1];
        out->z = scratch.v[2];
    } else {
        TrackCurve_EvaluateTangent(at, mode, t, out);
    }
}

// FUNCTION: LEGOLAND 0x00420810
void CoasterModel_DrawPass1(CoasterMesh* model, void* texture, int mode)
{
    unsigned int cycles;
    int i;
    int k;
    int tri[4];
    PolyVtx v[4];
    PolyJob job;

    __asm {
        push eax
        push edx
        rdtsc
        mov cycles, eax
        pop edx
        pop eax
    }
    job.kind = (mode != 0);
    job.v[0] = &v[0];
    job.v[1] = &v[1];
    job.v[2] = &v[2];
    job.shader = (void*)0x004b5648;
    for (i = 0; i < model->nfaces1; i++) {
        const ModelFace* face = &model->faces1[i];
        const Vec3f* n;
        float lit;

        tri[0] = face->v0;
        tri[1] = face->v1;
        tri[2] = face->v2;
        tri[3] = tri[0];
        job.dx1 = (float)(g_model_vertices[tri[1]].x - g_model_vertices[tri[0]].x);
        job.dy1 = (float)(g_model_vertices[tri[1]].y - g_model_vertices[tri[0]].y);
        job.dx2 = (float)(g_model_vertices[tri[2]].x - g_model_vertices[tri[0]].x);
        job.dy2 = (float)(g_model_vertices[tri[2]].y - g_model_vertices[tri[0]].y);
        job.area = job.dy2 * job.dx1 - job.dx2 * job.dy1;
        if (job.area > 0.0f) {
            job.and_flags = 0xff;
            job.or_flags = 0;
            job.or_flags |= g_model_vertices[tri[0]].clip;
            job.and_flags &= g_model_vertices[tri[0]].clip;
            job.or_flags |= g_model_vertices[tri[1]].clip;
            job.and_flags &= g_model_vertices[tri[1]].clip;
            job.or_flags |= g_model_vertices[tri[2]].clip;
            job.and_flags &= g_model_vertices[tri[2]].clip;
            if ((job.or_flags & 0xf) == 0xf) {
                n = &model->normals[face->normal];
                lit = g_model_light.z * n->z;
                lit = lit + g_model_light.y * n->y;
                lit = lit + g_model_light.x * n->x;
                lit = lit + g_view_half_hi;
                {
                    int shade;
                    __asm {
                        fld lit
                        fistp shade
                    }
                    job.shade = shade;
                }
                job.tag = ((int*)texture)[face->mat * 3];
                for (k = 0; k < 3; k++) {
                    v[k].sy = g_model_vertices[tri[k]].y;
                    v[k].sx = g_model_vertices[tri[k]].x;
                    v[k].shade = g_model_vertices[tri[k]].z;
                }
                Raster_SubmitPoly(2, &job);
            }
        }
    }
    __asm {
        push eax
        push edx
        rdtsc
        sub eax, cycles
        mov cycles, eax
        pop edx
        pop eax
    }
    g_stat_c_4dcbc8 += cycles;
}

/* Pass 2: faces +0x20/+0x24, normals2 at +0x14, per-vertex normals via *(&face->n0 + k). */
// FUNCTION: LEGOLAND 0x00420a20
void CoasterModel_DrawPass2(CoasterMesh* model, void* texture, int mode)
{
    unsigned int cycles;
    int i;
    int k;
    int tri[4];
    PolyVtx v[4];
    PolyJob job;

    __asm {
        push eax
        push edx
        rdtsc
        mov cycles, eax
        pop edx
        pop eax
    }
    job.kind = (mode != 0);
    job.v[0] = &v[0];
    job.v[1] = &v[1];
    job.v[2] = &v[2];
    job.shader = (void*)0x004b5658;
    for (i = 0; i < model->nfaces2; i++) {
        const ModelFace* face = &model->faces2[i];

        tri[0] = face->v0;
        tri[1] = face->v1;
        tri[2] = face->v2;
        tri[3] = tri[0];
        job.dx1 = (float)(g_model_vertices[tri[1]].x - g_model_vertices[tri[0]].x);
        job.dy1 = (float)(g_model_vertices[tri[1]].y - g_model_vertices[tri[0]].y);
        job.dx2 = (float)(g_model_vertices[tri[2]].x - g_model_vertices[tri[0]].x);
        job.dy2 = (float)(g_model_vertices[tri[2]].y - g_model_vertices[tri[0]].y);
        job.area = job.dy2 * job.dx1 - job.dx2 * job.dy1;
        if (job.area > 0.0f) {
            job.and_flags = 0xff;
            job.or_flags = 0;
            job.or_flags |= g_model_vertices[tri[0]].clip;
            job.and_flags &= g_model_vertices[tri[0]].clip;
            job.or_flags |= g_model_vertices[tri[1]].clip;
            job.and_flags &= g_model_vertices[tri[1]].clip;
            job.or_flags |= g_model_vertices[tri[2]].clip;
            job.and_flags &= g_model_vertices[tri[2]].clip;
            if ((job.or_flags & 0xf) == 0xf) {

                for (k = 0; k < 3; k++) {
                    const Vec3f* n = &model->normals2[*(&face->n0 + k)];
                    float lit;
                    int shade;
                    lit = g_model_light.z * n->z;
                    lit = lit + g_model_light.y * n->y;
                    lit = lit + g_model_light.x * n->x;
                    lit = lit + g_view_half_hi;
                    __asm {
                        fld lit
                        fistp shade
                    }
                    v[k].sy = g_model_vertices[tri[k]].y;
                    v[k].sx = g_model_vertices[tri[k]].x;
                    v[k].shade = shade;
                    v[k].sz = g_model_vertices[tri[k]].z;
                }

                job.tag = ((int*)texture)[face->mat * 3];
                Raster_SubmitPoly(3, &job);
            }
        }
    }
    __asm {
        push eax
        push edx
        rdtsc
        sub eax, cycles
        mov cycles, eax
        pop edx
        pop eax
    }
    g_stat_c_4dcbc8 += cycles;
}

/* Pass 3: faces +0x28/+0x2c, filler 0x4b5f50, attr count 4 with UV. */
// FUNCTION: LEGOLAND 0x00420c40
void CoasterModel_DrawPass3(CoasterMesh* model, void* texture, int mode)
{
    unsigned int cycles;
    int i;
    int tri[4];
    PolyVtx v[4];
    PolyJob job;

    __asm {
        push eax
        push edx
        rdtsc
        mov cycles, eax
        pop edx
        pop eax
    }
    job.kind = (mode != 0);
    job.v[0] = &v[0];
    job.v[1] = &v[1];
    job.v[2] = &v[2];
    job.shader = (void*)0x004b5f50;
    for (i = 0; i < model->nfaces3; i++) {
        const ModelFace* face = &model->faces3[i];
        const Vec3f* n;
        float lit;

        tri[0] = face->v0;
        tri[1] = face->v1;
        tri[2] = face->v2;
        tri[3] = tri[0];
        job.dx1 = (float)(g_model_vertices[tri[1]].x - g_model_vertices[tri[0]].x);
        job.dy1 = (float)(g_model_vertices[tri[1]].y - g_model_vertices[tri[0]].y);
        job.dx2 = (float)(g_model_vertices[tri[2]].x - g_model_vertices[tri[0]].x);
        job.dy2 = (float)(g_model_vertices[tri[2]].y - g_model_vertices[tri[0]].y);
        job.area = job.dy2 * job.dx1 - job.dx2 * job.dy1;
        if (job.area > 0.0f) {
            job.and_flags = 0xff;
            job.or_flags = 0;
            job.or_flags |= g_model_vertices[tri[0]].clip;
            job.and_flags &= g_model_vertices[tri[0]].clip;
            job.or_flags |= g_model_vertices[tri[1]].clip;
            job.and_flags &= g_model_vertices[tri[1]].clip;
            job.or_flags |= g_model_vertices[tri[2]].clip;
            job.and_flags &= g_model_vertices[tri[2]].clip;
            if ((job.or_flags & 0xf) == 0xf) {
                n = &model->normals[face->normal];
                lit = g_model_light.z * n->z;
                lit = lit + g_model_light.y * n->y;
                lit = lit + g_model_light.x * n->x;
                lit = lit + g_view_half_hi;
                {
                    int shade;
                    __asm {
                        fld lit
                        fistp shade
                    }
                    job.shade = shade;
                }

                mode = 0;
                job.tag = ((int*)texture)[face->mat * 3];
                do {
                    v[mode].sy = g_model_vertices[tri[mode]].y;
                    v[mode].sx = g_model_vertices[tri[mode]].x;
                    v[mode].shade = ((unsigned char*)texture)[(mode + face->mat * 6) * 2 + 4];
                    v[mode].sz = ((unsigned char*)texture)[(mode + face->mat * 6) * 2 + 5];
                    v[mode].f14 = g_model_vertices[tri[mode]].z;
                    mode++;
                } while (mode <= 2);

                Raster_SubmitPoly(4, &job);
            }
        }
    }
    __asm {
        push eax
        push edx
        rdtsc
        sub eax, cycles
        mov cycles, eax
        pop edx
        pop eax
    }
    g_stat_c_4dcbc8 += cycles;
}
