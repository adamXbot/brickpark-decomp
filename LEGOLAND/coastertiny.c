#ifdef LEGOLAND_PORTABLE
#define PhysObj_Init PhysObj_Init_vc6_body
#endif
#ifdef LEGOLAND_PORTABLE
#define Raster_RestoreState Raster_RestoreState_vc6_body
#endif
/* LEGOLAND -- scope E: coaster, route, curve and raster micro-helpers.
 * VC6 SP3 /O2 /Gy /Gd; names and verification in docs/lanes/scope-e.md.
 */
typedef struct Pos { int x, y; } Pos;
typedef struct Square { short x, y; } Square;
typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct Vec3Bits { unsigned int x, y, z; } Vec3Bits;
typedef struct Mat3 { float m[9]; } Mat3;
typedef struct RoutePos { void* node; Vec3f pos; void* geom; } RoutePos;
typedef struct TrackCursor { float t; RoutePos at; } TrackCursor;
typedef struct RouteNode { char pad00[0xe4]; struct RouteNode* prev; struct RouteNode* next; } RouteNode;
typedef struct CoasterRoute {
    unsigned int flags; int tick, deadline; char pad0c[0x64]; RouteNode head;
} CoasterRoute;
typedef struct CoasterCar {
    int state; void* model; void* rider; void* bloke;
    struct CoasterCar* prev; struct CoasterCar* next;
} CoasterCar;
typedef struct CoasterRec { char pad00[0xe4]; CoasterCar cars; } CoasterRec;
typedef struct RouteSeat { char pad00[0xc]; CoasterCar* car; } RouteSeat;
typedef struct PhysOps { void* op[10]; int dim; } PhysOps;
typedef struct PhysObj { void* read; void* write; void* pad08; PhysOps ops; } PhysObj;
typedef struct ClassRect { int x0, y0, x1, y1; } ClassRect;
typedef struct RideDef {
    char pad00[0x3c]; ClassRect rect; char pad4c[0x78]; void* elem;
} RideDef;
typedef struct TrackNode {
    int flags; Square square; RideDef* cls; void* desc; void* owner;
    int jin[2]; struct TrackNode* prev; int jout[2]; struct TrackNode* next;
} TrackNode;
typedef struct RideElem { char pad00[0xc]; RideDef* data; } RideElem;
typedef struct CtIface { RideElem* elem; void* cb[5]; } CtIface;
typedef struct ClipRect {
    int left, top, right, bottom, mask; struct ClipRect* prev; struct ClipRect* next;
} ClipRect;
typedef struct StationCursor { char pad00[0x10]; void* geom; char pad14[0x10]; float t; } StationCursor;
typedef struct NodeRef { char pad00[4]; unsigned int square; void* cls; } NodeRef;
typedef struct SavedNodeRef { int cls; unsigned int square; } SavedNodeRef;

extern int g_cc_obj_count;                        /* 0x004dd868 */
extern int g_cc_txt_count;                        /* 0x004dd86c */
extern void* g_4d8bac;                            /* 0x004d8bac */
extern ClipRect g_clip_ring;                      /* 0x00829a3c */
extern int g_car_pool_used;                       /* 0x004dd5d8 */
extern void* g_car_slot_table[30];               /* 0x0082ac60 */
extern CtIface g_ct_iface[6];                    /* 0x0082ad20 */
extern void* g_route_pending;                    /* 0x0082adec */
extern void (*g_fast_rsqrt)(void);               /* 0x00829a5c; custom x87 ABI */
extern unsigned char g_support_model[];         /* 0x004b6300 */
extern unsigned char g_support_texture[];       /* 0x004b62f0 */
extern void* g_coaster_tab_a[];                  /* 0x004d8a40 */
extern void* g_coaster_tab_b[];                  /* 0x004d8abc */
extern void* g_coaster_tab_b2[];                 /* 0x004d8b34 */
extern unsigned char g_station_geometry[];       /* 0x006103a8 */
extern unsigned char* g_shade_block;             /* 0x00829c54 */
extern unsigned short* g_shade_tab[0x400];       /* 0x00829c60 */
extern Vec3f g_support_template[8];             /* 0x004b61e0 */
extern Vec3f g_support_vertices[8];             /* 0x00614858 */
extern int g_track_cursor_modes[];              /* 0x004b6408 */
extern RideDef* g_castle_def;                    /* 0x00829bf8 */

extern void* CoasterModel_LoadFile(const char*, int);      /* 0x00420550 */
extern int CoasterModel_FindPartIndex(const char*);       /* 0x00422600 */
extern int CoasterModel_FindMeshIndex(const char*);       /* 0x00422590 */
extern void Free_w(void*);                               /* 0x004775d0 */
extern void RouteNode_LinkClipRect(RouteNode*);           /* 0x0041e950 */
extern void RouteNode_UnlinkClipRect(RouteNode*);         /* 0x0041e970 */
extern void RouteNode_UpdateClipRect(RouteNode*);         /* 0x0041e990 */
extern void RouteNode_ClearActive(RouteNode*);            /* 0x0041e630 */
extern float Route_StepFree(CoasterRoute*, float);        /* 0x0041e000 */
extern __declspec(dllimport) int __stdcall SetCurrentDirectoryA(const char*); /* 0x004ab268 */
extern void RouteNode_AddPending(RouteNode*, void*);      /* 0x0041eaf0 */
extern int GetGameTimer(void);                           /* 0x00499430 */
extern float Route_TotalAcceleration(CoasterRoute*);     /* 0x0041dd70 */
extern void Coaster3D_DrawModel(void*, void*, const Vec3f*, const Mat3*, int); /* 0x00420e90 */
extern void PhysObj_ReadState(PhysObj*, void*);           /* 0x004203d0 */
extern void PhysObj_WriteState(PhysObj*, void*);          /* 0x004203f0 */
extern void PhysVec_InitOps(PhysOps*, int);               /* 0x00421540 */
extern int PackTrackClass(void*);                        /* 0x0041ebd0 */
extern void Shade_BuildRamp(unsigned int, unsigned short*); /* 0x00422e40 */
#ifndef LEGOLAND_PORTABLE
extern void TrackCurve_EvaluateOffset(RoutePos*, int, float, float, Vec3f*); /* 0x00429bb0 */
#else
/* coaster9.c's definition takes the curve parameter as a RAW DWORD (`int`)
 * and forwards it to the geometry vtable that way; on wasm32 that is a
 * different function type from f32, so TrackCursor_Evaluate below has to hand
 * over the bits of `cursor->t`, which is what its `push` did. */
extern void TrackCurve_EvaluateOffset(RoutePos*, int, int, float, Vec3f*); /* 0x00429bb0 */
#define TrackCurve_EvaluateOffset(_at, _m, _t, _o, _out) \
    TrackCurve_EvaluateOffset((_at), (_m), LL_ASINT(_t), (_o), (_out))
#endif
extern void Coaster3D_SetCarClipDepth(void);              /* 0x00425c40 */
extern void TrackAddBasicObject(void*, Pos*);            /* 0x0041ed90 */

// FUNCTION: LEGOLAND 0x00424e60
void Coaster_OnCircuitClosed(CoasterRec* rec) { (void)rec; }
// FUNCTION: LEGOLAND 0x00423790
void Raster_RestoreState(void) {}
// FUNCTION: LEGOLAND 0x00422640
int CoasterModel_GetPartCount(void) { return g_cc_txt_count; }
// FUNCTION: LEGOLAND 0x004225d0
int CoasterModel_GetMeshCount(void) { return g_cc_obj_count; }
// FUNCTION: LEGOLAND 0x004207c0
void* GetCoasterColours(void) { return g_4d8bac; }
/* Constant per-node contribution used by the route's acceleration sum. */
// FUNCTION: LEGOLAND 0x0041e7e0
float RouteNode_GetAcceleration(RouteNode* node) { (void)node; return 0.1f; }
// FUNCTION: LEGOLAND 0x004273e0
CoasterCar* RouteSeat_DetachCar(RouteSeat* seat)
{
    CoasterCar* car = seat->car; seat->car = 0; return car;
}
// FUNCTION: LEGOLAND 0x004273d0
CoasterCar* RouteSeat_AttachCar(RouteSeat* seat, CoasterCar* car)
{
    seat->car = car; return car;
}
// FUNCTION: LEGOLAND 0x00426740
void Raster_ResetClipRing(void) { g_clip_ring.next = &g_clip_ring; g_clip_ring.prev = &g_clip_ring; }
// FUNCTION: LEGOLAND 0x0041cc90
unsigned int JointBitFromIndex(int index) { return 1u << index; }

/* This original helper uses inline x87 assembly and an EBP frame. The
 * argument is the saved control WORD itself, not a pointer to that word. */
// FUNCTION: LEGOLAND 0x00423730
void Raster_RestoreFloatMode(unsigned short control)
{
#ifndef LEGOLAND_PORTABLE
    __asm { fldcw control }
#else
    (void)control; /* x87 control word: no-op in the portable build */
#endif
}
// FUNCTION: LEGOLAND 0x00421cc0
void TrackCurve_CubicUpVector(void* curve, float t, Vec3f* out)
{
    (void)curve; (void)t; out->x = 0.0f; out->y = 0.0f; out->z = 1.0f;
}
// FUNCTION: LEGOLAND 0x00421a90
void TrackCurve_LineUpVector(void* curve, float t, Vec3f* out)
{
    (void)curve; (void)t; out->x = 0.0f; out->y = 0.0f; out->z = 1.0f;
}
/* The pool is a stack: the pointer is ignored; only the count is subtracted. */
// FUNCTION: LEGOLAND 0x00421510
void CarPool_Free(void* ptr, int count) { (void)ptr; g_car_pool_used -= count; }
// FUNCTION: LEGOLAND 0x004207a0
void* CoasterModel_LoadPalette(void) { return CoasterModel_LoadFile("Rollercoaster.lpt", 0); }
// FUNCTION: LEGOLAND 0x00420790
int FindCoasterPart(const char* name) { return CoasterModel_FindPartIndex(name); }
// FUNCTION: LEGOLAND 0x0041ec00
RideDef* CastleClassDef(int index) { return g_ct_iface[index].elem->data; }
// FUNCTION: LEGOLAND 0x0041eb60
void RouteNode_Free(RouteNode* node) { Free_w(node); }
// FUNCTION: LEGOLAND 0x0041d430
void TrackLinkNodes(TrackNode* a, TrackNode* b) { a->next = b; b->prev = a; }

/* Includes the embedded head node: the callback runs at least once. */
// FUNCTION: LEGOLAND 0x0041e330
void Route_ForEachNode(CoasterRoute* route, void (*visit)(RouteNode*))
{
    RouteNode* head = &route->head;
    RouteNode* node = head;
    do { visit(node); node = node->next; } while (node != head);
}
// FUNCTION: LEGOLAND 0x0041e400
void Route_ClearNodeActiveFlags(CoasterRoute* route) { Route_ForEachNode(route, RouteNode_ClearActive); }
// FUNCTION: LEGOLAND 0x0041e3a0
void Route_UpdateClipRects(CoasterRoute* route) { Route_ForEachNode(route, RouteNode_UpdateClipRect); }
// FUNCTION: LEGOLAND 0x0041e380
void Route_UnlinkClipRects(CoasterRoute* route) { Route_ForEachNode(route, RouteNode_UnlinkClipRect); }
// FUNCTION: LEGOLAND 0x0041e360
void Route_LinkClipRects(CoasterRoute* route) { Route_ForEachNode(route, RouteNode_LinkClipRect); }
// FUNCTION: LEGOLAND 0x00424890
float Coaster_StepFreeRoute(CoasterRoute* route, float dt) { return Route_StepFree(route, dt); }
// FUNCTION: LEGOLAND 0x004214f0
void** CarPool_Alloc(int count)
{
    int first = g_car_pool_used; g_car_pool_used += count; return &g_car_slot_table[first];
}
// FUNCTION: LEGOLAND 0x00420530
int CoasterModel_SetDirectory(const char* path)
{
    if (!path) return 0;
    return SetCurrentDirectoryA(path);
}
// FUNCTION: LEGOLAND 0x0041e3c0
void RouteAddNode_Cb(RouteNode* node) { RouteNode_AddPending(node, g_route_pending); }
// FUNCTION: LEGOLAND 0x0041e240
void Route_UpdateTimer(CoasterRoute* route)
{
    int now = GetGameTimer(); if (now > route->deadline) route->flags &= ~0x40u; route->tick = now;
}
/* The dispatch target takes and returns ST(0), not a C stack argument.
 * Preserve the original rounding through the argument's float-sized home. */
// FUNCTION: LEGOLAND 0x00426960
float VecMath_ReciprocalSqrt(float value)
{
#ifndef LEGOLAND_PORTABLE
    __asm {
        fld value
        call dword ptr [g_fast_rsqrt]
        fstp value
    }
#else
    value = ((float (*)(float))g_fast_rsqrt)(value);
#endif
    return value;
}
// FUNCTION: LEGOLAND 0x0041ddb0
float Route_AccelDistance(CoasterRoute* route, float time)
{
    return Route_TotalAcceleration(route) * time * time * 0.5f;
}
// FUNCTION: LEGOLAND 0x00429490
void DrawSupportModel(const Vec3f* pos, const Mat3* matrix)
{
    Coaster3D_DrawModel(g_support_model, g_support_texture, pos, matrix, 1);
}
// FUNCTION: LEGOLAND 0x00420730
void* GetCoasterModelSize(const char* name)
{
    int i = CoasterModel_FindMeshIndex(name); if (i != -1) return g_coaster_tab_b2[i]; return 0;
}
// FUNCTION: LEGOLAND 0x00420710
void* LoadCoasterMeshTex(const char* name)
{
    int i = CoasterModel_FindMeshIndex(name); if (i != -1) return g_coaster_tab_b[i]; return 0;
}
// FUNCTION: LEGOLAND 0x004206b0
void* LoadCoasterMesh(const char* name)
{
    int i = CoasterModel_FindMeshIndex(name); if (i != -1) return g_coaster_tab_a[i]; return 0;
}
// FUNCTION: LEGOLAND 0x00420410
void PhysObj_Init(PhysObj* obj)
{
    PhysVec_InitOps(&obj->ops, 2); obj->read = PhysObj_ReadState; obj->write = PhysObj_WriteState;
}
// FUNCTION: LEGOLAND 0x0042a620
void TrackCursor_Init(TrackCursor* cursor, const RoutePos* at, float t)
{
    cursor->at = *at; cursor->t = t;
}
// FUNCTION: LEGOLAND 0x00426e80
void WriteCoasterNodeRef(NodeRef* node, SavedNodeRef* out)
{
    out->cls = PackTrackClass(node->cls); out->square = node->square;
}
// FUNCTION: LEGOLAND 0x00424960
int Route_AtStationEnd(StationCursor* cursor)
{
    if (cursor->geom == g_station_geometry && cursor->t > 0.099f) return 1; return 0;
}
// FUNCTION: LEGOLAND 0x00422e10
void Shade_BuildEntry(unsigned int rgb, int index)
{
    unsigned short* entry = (unsigned short*)(g_shade_block + index * 128);
    g_shade_tab[index] = entry; Shade_BuildRamp(rgb, entry);
}
/* Scale all eight support vertices, preserving the third component's bits. */
// FUNCTION: LEGOLAND 0x004294b0
void Coaster_BuildSupportVertices(void)
{
    int i;
    for (i = 0; i < 8; ++i) {
        g_support_vertices[i].x = g_support_template[i].x * 2.5f;
        g_support_vertices[i].y = g_support_template[i].y * 8.2f;
        ((Vec3Bits*)g_support_vertices)[i].z = ((Vec3Bits*)g_support_template)[i].z;
    }
}
// FUNCTION: LEGOLAND 0x00424c10
int Coaster_CountWaitingCars(CoasterRec* rec)
{
    int n = 0; CoasterCar* c;
    for (c = rec->cars.next; c != &rec->cars; c = c->next) if (c->state == 1) ++n;
    return n;
}
// FUNCTION: LEGOLAND 0x0042a640
void TrackCursor_Evaluate(TrackCursor* cursor, int mode, Vec3f* out)
{
    TrackCurve_EvaluateOffset(&cursor->at, g_track_cursor_modes[mode], cursor->t, 4.8f, out);
}
// FUNCTION: LEGOLAND 0x004239e0
void Castle_GetSecondCorner(const Square* square, Square* out)
{
    ClassRect* r = &g_castle_def->rect;
    out->x = (short)r->x0 + square->x - 2; out->y = (short)r->y1 + square->y;
}
// FUNCTION: LEGOLAND 0x004239b0
void Castle_GetFirstCorner(const Square* square, Square* out)
{
    ClassRect* r = &g_castle_def->rect;
    out->x = (short)r->x1 + square->x; out->y = (short)r->y0 + square->y - 2;
}
// FUNCTION: LEGOLAND 0x00421560
void CoasterCar_Draw(CoasterCar* car, Vec3f* transform)
{
    Coaster3D_SetCarClipDepth();
    Coaster3D_DrawModel(car->model, car->rider, transform, (Mat3*)(transform + 1), 0);
}
// FUNCTION: LEGOLAND 0x0041ceb0
void TrackNode_PlaceObject(TrackNode* node)
{
    Pos pos; pos.x = node->square.x; pos.y = node->square.y; TrackAddBasicObject(node->cls->elem, &pos);
}

#ifdef LEGOLAND_PORTABLE
/* Raster_RestoreState is called with 1 argument(s) the original ignores: the body
 * at this address never reads them, and in cdecl the caller cleans them up.
 * On wasm the argument count is part of the function type, so the exported
 * name is this forwarder and the matched body keeps its own.  */
#undef Raster_RestoreState
void Raster_RestoreState(int ll_a1) { (void)ll_a1; Raster_RestoreState_vc6_body(); }
#endif

#ifdef LEGOLAND_PORTABLE
/* PhysObj_Init is called with 0 argument(s) the original ignores: the body
 * at this address never reads them, and in cdecl the caller cleans them up.
 * On wasm the argument count is part of the function type, so the exported
 * name is this forwarder and the matched body keeps its own.  */
#undef PhysObj_Init
void PhysObj_Init(PhysObj* ll_obj, int ll_dim) {  PhysObj_Init_vc6_body(ll_obj); }
#endif
