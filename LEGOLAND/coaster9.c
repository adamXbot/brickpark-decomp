/* LEGOLAND -- Codex-E: coaster model, route, seat and physics helpers.
 * VC6 SP3 /O2 /Gy /Gd. Local types describe the original 32-bit layouts.
 */
typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct Mat3 { float m[9]; } Mat3;
typedef struct Mat4 { float m[16]; } Mat4;
typedef struct Transform { Vec3f pos; Mat3 rot; } Transform;
typedef struct ClipRect { int left, top, right, bottom, mask; struct ClipRect *prev, *next; } ClipRect;
typedef struct RouteNode {
    unsigned flags; char pad04[0xc4-4]; float velocity;
    ClipRect clip; struct RouteNode *prev, *next;
} RouteNode;
typedef struct RoutePos { void* node; void* geom; Vec3f pos; } RoutePos;
typedef struct CoasterRoute {
    int flags, tick, deadline; RoutePos at; int dimension; float parameter, speed;
    char pad2c[0x70-0x2c]; RouteNode head;
} CoasterRoute;
typedef struct PhysObj {
    void* cb[5]; void (*copy)(void*, const void*); void* rest[8]; void* state;
} PhysObj;
typedef struct CoasterCar {
    char pad00[0x20]; void (*update)(struct CoasterCar*, Transform*);
    void (*release)(struct CoasterCar*);
} CoasterCar;
typedef struct RouteSeat { Vec3f pos; CoasterCar* car; } RouteSeat;
typedef struct ModelImage { char* data; int length; } ModelImage;
typedef struct ViewRec { float a; Vec3f direction; } ViewRec;
typedef struct SpanRect { int top, left, bottom, right; } SpanRect;

extern ModelImage g_cc_obj;                         /* 0x004dd860 */
extern ModelImage g_cc_txt;                         /* 0x004dd758 */
extern void (*g_fast_sqrt)(void);                  /* 0x00829a58; ST(0) ABI */
extern ViewRec g_view_cur;                         /* 0x004b5c9c */
extern ViewRec g_view_car;                         /* 0x004b5cbc */
extern float g_view_car_angle;                    /* 0x00611650 */
extern float g_view_angle_cur;                    /* 0x00611648 */
extern float g_view_half_hi;                      /* 0x00829a60 */
extern float g_view_half_lo;                      /* 0x0082999c */
extern void* g_route_node_model;                  /* 0x0082add0 */

extern void ClipRect_Link(ClipRect*);                     /* 0x004266b0 */
extern void ClipRect_Unlink(ClipRect*);                   /* 0x004266e0 */
extern void ClipRect_SetBounds(ClipRect*, const SpanRect*); /* 0x00426700 */
extern int ModelImage_FindName(ModelImage*, const char*); /* 0x00422400 */
extern int RouteNode_IsPending(RouteNode*);              /* 0x0041e660 */
extern int RouteNode_CanAdd(RouteNode*, void*);           /* 0x0041e670 */
extern void RouteNode_LinkPending(RouteNode*);           /* 0x0041ea70 */
extern void RouteNode_SetPending(RouteNode*, int);       /* 0x0041e640 */
extern float RouteNode_GetAcceleration(RouteNode*);      /* 0x0041e7e0 */
extern float Route_GetSpeed(CoasterRoute*);              /* 0x0041dca0 */
extern void TrackCurve_EvaluateDerivative(RoutePos*, int, int, int, Vec3f*); /* 0x00429c60 */
extern float Vec3Dot(const Vec3f*, const Vec3f*);         /* 0x00425d30 */
extern void TrackCurve_EvaluatePosition(RoutePos*, int, int, Vec3f*); /* 0x00429a80 */
extern void TrackCurve_EvaluateUp(RoutePos*, int, int, Vec3f*); /* 0x00429b90 */
extern void RouteNode_GetTransform(RouteNode*, Transform*); /* 0x0041e9e0 */
extern void CoasterModel_GetClipRect(void*, Vec3f*, Mat3*, SpanRect*); /* 0x00420fb0 */

extern int ModelImage_IsEOL(const char*);                  /* 0x004222f0 */
extern float VecMath_Sqrt(float);                       /* 0x00426a90 */
extern float Route_TravelPerTick(CoasterRoute*, float); /* 0x0041dd00 */

// FUNCTION: LEGOLAND 0x0041e630
void RouteNode_ClearActive(RouteNode* node) { node->flags &= ~1u; }
// FUNCTION: LEGOLAND 0x0041e810
float RouteCar_GetVelocity(RouteNode* node) { return node->velocity; }
// FUNCTION: LEGOLAND 0x004222f0
int ModelImage_IsEOL(const char* p) { return *(const unsigned short*)p == 0x0a0d; }
// FUNCTION: LEGOLAND 0x0041e950
void RouteNode_LinkClipRect(RouteNode* node) { ClipRect_Link(&node->clip); }
// FUNCTION: LEGOLAND 0x0041e970
void RouteNode_UnlinkClipRect(RouteNode* node) { ClipRect_Unlink(&node->clip); }
// FUNCTION: LEGOLAND 0x00422590
int CoasterModel_FindMeshIndex(const char* name) { return ModelImage_FindName(&g_cc_obj, name); }
// FUNCTION: LEGOLAND 0x00422600
int CoasterModel_FindPartIndex(const char* name) { return ModelImage_FindName(&g_cc_txt, name); }
// FUNCTION: LEGOLAND 0x004273c0
int RouteSeat_IsOccupied(RouteSeat* seat) { return seat->car != 0; }
// FUNCTION: LEGOLAND 0x004203d0
void PhysObj_ReadState(PhysObj* obj, void* out) { obj->copy(out, obj->state); }
// FUNCTION: LEGOLAND 0x004203f0
void PhysObj_WriteState(PhysObj* obj, const void* in) { obj->copy(obj->state, in); }
/* Original inline assembly: hook takes and returns ST(0), rounded to float.
 * The EBP frame is also present in the executable. */
// FUNCTION: LEGOLAND 0x00426a90
float VecMath_Sqrt(float value)
{
    __asm { fld value
        call dword ptr [g_fast_sqrt]
        fstp value }
    return value;
}
/* 0x4273e0 already owns RouteSeat_DetachCar. This operation invokes the
 * car's release method and does not clear the seat pointer itself. */
// FUNCTION: LEGOLAND 0x004273f0
void RouteSeat_ReleaseCar(RouteSeat* seat)
{
    CoasterCar* car = seat->car;
    if (car) car->release(car);
}
/* Length of the track tangent times speed and the original 0.0015875
 * conversion factor. The parameter is forwarded as its raw float bits. */
// FUNCTION: LEGOLAND 0x0041dd00
float Route_TravelPerTick(CoasterRoute* route, float speed)
{
    Vec3f derivative;
    TrackCurve_EvaluateDerivative(&route->at, 1, *(int*)&route->parameter, 0, &derivative);
    return VecMath_Sqrt(Vec3Dot(&derivative, &derivative)) * speed * 0.0015875f;
}
// FUNCTION: LEGOLAND 0x0041dd50
float Route_TravelThisTick(CoasterRoute* route)
{
    float speed = Route_GetSpeed(route);
    return Route_TravelPerTick(route, speed);
}
/* Keep the direction copy separate from a: a whole ViewRec assignment
 * passes normalized audit but reads/stores different global fields. */
// FUNCTION: LEGOLAND 0x00425c40
void Coaster3D_SetCarClipDepth(void)
{
    g_view_half_hi = (g_view_car_angle + g_view_car.a) * 0.5f;
    g_view_cur.direction = g_view_car.direction;
    g_view_angle_cur = g_view_car_angle;
    g_view_cur.a = g_view_car.a;
    g_view_half_lo = (g_view_car.a - g_view_car_angle) * 0.5f;
}
/* Includes the embedded sentinel, just like the other route sums. The
 * historical mass name is retained; each node contributes constant 0.1. */
// FUNCTION: LEGOLAND 0x0041dd70
float Route_SumMass(CoasterRoute* route)
{
    float sum = 0.0f;
    RouteNode* node = &route->head;
    do { sum += RouteNode_GetAcceleration(node); node = node->next; } while (node != &route->head);
    return sum;
}
// FUNCTION: LEGOLAND 0x0041eaf0
void RouteNode_AddPending(RouteNode* node, void* target)
{
    if (!RouteNode_IsPending(node) && RouteNode_CanAdd(node, target)) {
        RouteNode_LinkPending(node); RouteNode_SetPending(node, 1);
    }
}
// FUNCTION: LEGOLAND 0x0041e990
void RouteNode_UpdateClipRect(RouteNode* node)
{
    Transform transform;
    SpanRect bounds;
    RouteNode_GetTransform(node, &transform);
    CoasterModel_GetClipRect(g_route_node_model, &transform.pos, &transform.rot, &bounds);
    ClipRect_SetBounds(&node->clip, &bounds);
}
/* Despite the inherited name this copies an entire CRLF-terminated line,
 * excludes CRLF, appends no NUL and returns the end of the output. */
// FUNCTION: LEGOLAND 0x00422300
char* ModelRecord_CopyToken(const char* record, char* out)
{
    while (!ModelImage_IsEOL(record)) { *out++ = *record++; }
    return out;
}
// FUNCTION: LEGOLAND 0x00422340
char* ModelImage_FindRecord(ModelImage* image, int index)
{
    char* p = image->data;
    int length = image->length;
    int i = 0;
    char* end = p + length;
    if (index == 0) return p;
    while (p < end - 1) {
        if (ModelImage_IsEOL(p)) { ++i; p += 2; if (i == index) return p; }
        else ++p;
    }
    return 0;
}
/* t is forwarded as raw float bits to preserve the original cdecl loads. */
// FUNCTION: LEGOLAND 0x00429bb0
void TrackCurve_EvaluateOffset(RoutePos* at, int mode, int t, float offset, Vec3f* out)
{
    Vec3f up;
    TrackCurve_EvaluatePosition(at, mode, t, out);
    TrackCurve_EvaluateUp(at, mode, t, &up);
    out->x -= up.x * offset;
    out->y -= up.y * offset;
    out->z -= up.z * offset;
}
/* Temporarily translate the supplied transform to this seat, invoke the
 * car update, then restore all three original position components. The
 * second argument is real despite the one-argument caller-side typedef. */
// FUNCTION: LEGOLAND 0x00427410
void RouteSeat_Update(RouteSeat* seat, Transform* transform)
{
    if (seat->car) {
        Vec3f saved = transform->pos;
        transform->pos.x = transform->rot.m[0]*seat->pos.x + transform->rot.m[6]*seat->pos.z + transform->rot.m[3]*seat->pos.y + saved.x;
        transform->pos.y = transform->rot.m[1]*seat->pos.x + transform->rot.m[7]*seat->pos.z + transform->rot.m[4]*seat->pos.y + saved.y;
        transform->pos.z = transform->rot.m[2]*seat->pos.x + transform->rot.m[8]*seat->pos.z + transform->rot.m[5]*seat->pos.y + saved.z;
        seat->car->update(seat->car, transform);
        transform->pos = saved;
    }
}

/* Widen the signed byte before a boolean test: a direct mask narrows the
 * load to mov al; this int local preserves the original movsx. */
// FUNCTION: LEGOLAND 0x0041e660
int RouteNode_IsPending(RouteNode* node)
{
    int flags = *(signed char*)&node->flags;
    if (flags & 1) return 1;
    return 0;
}

typedef struct ModelMesh { int count; char pad04[8]; Vec3f* vertices; } ModelMesh;
typedef struct VideoSurfaceInfo { long pitch; int width, height; void* bits; int unused, format; } VideoSurfaceInfo;
extern Vec3f g_model_light;                      /* 0x004dcbb8 */
extern int g_stat_c_4dcbc8;             /* 0x004dcbc8 */
extern unsigned char g_model_vertices[];         /* 0x004d8bb8 */
extern Mat4 g_view_matrix;                       /* 0x008299fc */
extern void Mat3_TransposeToMat4(const Mat3*, Mat4*); /* 0x00426510 */
extern void TransformVec3(const Vec3f*, Vec3f*, const Mat4*, int); /* 0x004261c0 */
extern void MakeTransform(const Vec3f*, const Mat3*, Mat4*); /* 0x004264e0 */
extern void MatMul(const Mat4*, const Mat4*, Mat4*);       /* 0x00426120 */
extern void TransformVerts(const Vec3f*, void*, const Mat4*, int, int); /* 0x00426250 */
extern int Raster_SaveState(VideoSurfaceInfo*);          /* 0x00423760 */
extern void Raster_RestoreState(VideoSurfaceInfo*);      /* 0x00423790 */
extern void CoasterModel_DrawPass1(ModelMesh*, void*, int); /* 0x00420810 */
extern void CoasterModel_DrawPass2(ModelMesh*, void*, int); /* 0x00420a20 */
extern void CoasterModel_DrawPass3(ModelMesh*, void*, int); /* 0x00420c40 */

/* The executable brackets vertex transformation with inline RDTSC probes.
 * These save eax/edx explicitly, and account only the low 32-bit delta. */
// FUNCTION: LEGOLAND 0x00420e90
void Coaster3D_DrawModel(ModelMesh* model, void* texture, const Vec3f* pos, const Mat3* rot, int mode)
{
    unsigned int cycles;
    VideoSurfaceInfo surface;
    Mat4 transform, projected;
    __asm {
        push eax
        push edx
        rdtsc
        mov cycles, eax
        pop edx
        pop eax
    }
    if (model && texture) {
        Mat3_TransposeToMat4(rot, &transform);
        TransformVec3(&g_view_cur.direction, &g_model_light, &transform, 1);
        g_model_light.x *= g_view_half_lo;
        g_model_light.y *= g_view_half_lo;
        g_model_light.z *= g_view_half_lo;
        MakeTransform(pos, rot, &transform);
        MatMul(&g_view_matrix, &transform, &projected);
        TransformVerts(model->vertices, g_model_vertices, &projected, 0x10, model->count);
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
        if (Raster_SaveState(&surface)) {
            CoasterModel_DrawPass1(model, texture, mode);
            CoasterModel_DrawPass2(model, texture, mode);
            CoasterModel_DrawPass3(model, texture, mode);
            Raster_RestoreState(&surface);
        }
    }
}

extern ClipRect g_clip_ring;                            /* 0x00829a3c */
/* Insertion is at the sentinel's next side (+0x18); each pointer store is
 * observable before the following aliased link is read. */
// FUNCTION: LEGOLAND 0x004266b0
void ClipRect_Link(ClipRect* rect)
{
    g_clip_ring.next->prev = rect;
    rect->next = g_clip_ring.next;
    g_clip_ring.next = rect;
    rect->prev = &g_clip_ring;
}
// FUNCTION: LEGOLAND 0x004266e0
void ClipRect_Unlink(ClipRect* rect)
{
    rect->next->prev = rect->prev;
    rect->prev->next = rect->next;
}
