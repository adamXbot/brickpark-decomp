/* LEGOLAND -- coaster/school-car micro-helpers, scope codex-a.
 * Reconstructed for VC6 SP3 /O2 /Gy /Gd. See docs/lanes/codex-a.md.
 * All types are local. Float copies retain their raw, 32-bit representation.
 */
#include <math.h>
#include <string.h>
#pragma intrinsic(fabs, memset)

typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct PhysVec { int n; float v[20]; } PhysVec; /* one pool slot: 0x54 */
typedef struct PhysOps { void* op[10]; int dim; } PhysOps;
typedef struct TrackNode TrackNode;
typedef struct RouteNode RouteNode;
typedef struct CoasterRoute CoasterRoute;
typedef struct CoasterRec CoasterRec;
typedef struct CoasterCar CoasterCar;
typedef struct RouteSeat RouteSeat;
typedef struct Curve {
    Vec3f pos;                 /* +00 */
    Vec3f dir;                 /* +0c */
    Vec3f offset;              /* +18 */
    float cubic[4];            /* +24, highest degree first */
    unsigned char pad34[0x10];
    float limits[2];           /* +44 */
} Curve;
struct TrackNode {
    int flags;
    unsigned int square;
    void* cls;
    void* desc;
    void* owner;
    int jin[2];
    TrackNode* prev;           /* +1c */
    int jout[2];
    TrackNode* next;           /* +28 */
};
struct CoasterCar {
    int state;
    void* model;
    void* rider;
    void* bloke;
    CoasterCar* prev;
    CoasterCar* next;
    RouteSeat* seat;     /* +18 */
    void* rest[3];
};
struct RouteNode {
    unsigned char pad00[0xe4];
    RouteNode* prev;           /* +e4 */
    RouteNode* next;           /* +e8 */
};
struct CoasterRoute {
    unsigned char pad00[0x34];
    void* derivative;          /* +34: solver's derivative callback */
    unsigned char pad38[0x6c-0x38];
    CoasterRec* owner;         /* +6c */
    RouteNode head;            /* +70 */
};
struct CoasterRec {
    unsigned char pad00[0xd8];
    CoasterRoute* route;       /* +d8 */
    unsigned char paddc[8];
    CoasterCar cars;           /* +e4, sentinel; next at +f8 */
    unsigned char pad10c[4];
    float (*step_primary)(CoasterRoute*, float);   /* +110 */
    float (*step_secondary)(CoasterRoute*, float); /* +114 */
};
typedef struct CarRef { int state, rider_id; } CarRef;
typedef struct ModelImage { void* data; int length; } ModelImage;
typedef struct NodeCursor { unsigned char b[0x38]; } NodeCursor;

extern PhysOps g_car_pool_hooks;                               /* 0x004dcbd0 */
extern ModelImage g_cc_obj;                                    /* 0x004dd860 */
extern char g_cc_name[0x100];                                   /* 0x004dd760 */
extern int __declspec(dllimport) __cdecl wsprintfA(char* out, const char* fmt, ...); /* 0x004ab298 */
extern void ModelRecord_GetName(ModelImage* image, char* out, int index); /* 0x00422390 */
extern TrackNode* FindTrackNodeAt(CoasterRec* rec, const unsigned int* square); /* 0x0041d060 */
extern void TrackCursor_Evaluate(NodeCursor* cursor, int mode, Vec3f* out); /* 0x0042a640 */
extern void RouteSeat_DetachCar(RouteSeat* seat);               /* 0x004273e0 */
extern void RouteSeat_AttachCar(RouteSeat* seat, CoasterCar* car); /* 0x004273d0 */
extern int CoasterRider_GetSaveIndex(void* bloke);              /* 0x00426ff0 */
extern void* CoasterModel_LoadFile(const char* name, int mode); /* 0x00420550 */
extern void Route_Reset(CoasterRoute* route);                   /* 0x0041e500 */
extern void Route_SetDeadline(CoasterRoute* route, int ticks);  /* 0x0041e4c0 */
extern RouteNode* RouteNode_Alloc(int kind);                    /* 0x0041eb30 */
extern void RouteNode_Free(RouteNode* node);                    /* 0x0041eb60 */

/* Dispatch the primary route step through the coaster's +110 hook. */
// FUNCTION: LEGOLAND 0x0041e0e0
float Route_StepPrimary(CoasterRoute* rt, float distance)
{
    return rt->owner->step_primary(rt, distance);
}

/* Report the two endpoints of a curve's parameter interval. */
// FUNCTION: LEGOLAND 0x00421a70
int TrackCurve_GetLimits(const Curve* curve, float* out)
{
    out[0] = curve->limits[0];
    out[1] = curve->limits[1];
    return 2;
}

/* Read one object-model record's name. */
// FUNCTION: LEGOLAND 0x004225b0
void CoasterModel_GetRecordName(int index, char* out)
{
    ModelRecord_GetName(&g_cc_obj, out, index);
}

/* The save reference's second dword is the packed map square. */
// FUNCTION: LEGOLAND 0x00426ea0
TrackNode* TrackRef_FindPiece(const unsigned int* ref, CoasterRec* rec)
{
    return FindTrackNodeAt(rec, ref + 1);
}

/* The second cursor of a route node starts at +40; mode 2 is tangent. */
// FUNCTION: LEGOLAND 0x0041e930
void RouteNode_GetTailTangent(RouteNode* node, Vec3f* out)
{
    TrackCursor_Evaluate((NodeCursor*)((char*)node + 0x40), 2, out);
}

/* Vector-op slot 2 copies the entire 0x54-byte pool slot, not only n floats. */
// FUNCTION: LEGOLAND 0x00421320
void PhysVec_Copy(const PhysVec* src, PhysVec* dst)
{
    *dst = *src;
}

/* Drop the seat's occupant and then clear the car's seat link. */
// FUNCTION: LEGOLAND 0x004215b0
void CoasterCar_DetachSeat(CoasterCar* car)
{
    RouteSeat_DetachCar(car->seat);
    car->seat = 0;
}

/* Six samples span a quarter turn, using the original rounded constants. */
// FUNCTION: LEGOLAND 0x00421c80
int TrackCurve_GetQuarterTurnSamples(const Curve* unused, float* out)
{
    out[0] = 0.0f;
    out[1] = 0.314159006f;
    out[2] = 0.628318012f;
    out[3] = 0.942476988f;
    out[4] = 1.25663602f;
    out[5] = 1.57079506f;
    return 6;
}

/* Build the base-name plus four-digit model index. */
// FUNCTION: LEGOLAND 0x00422620
void CoasterModel_FormatIndexedName(int index, char* out)
{
    wsprintfA(out, "%s%04d", g_cc_name, index);
}

/* A straight segment's tangent is constant; the parameter is ignored. */
// FUNCTION: LEGOLAND 0x004219f0
void TrackCurve_LineTangent(const Curve* curve, float unused, Vec3f* out)
{
    *out = curve->dir;
}

/* Count predecessor pieces until NULL or either boundary flag (mask 6). */
// FUNCTION: LEGOLAND 0x0041cee0
int Track_CountHeadPieces(TrackNode* node)
{
    int count = 0;
    while (node) {
        if (node->flags & 6) break;
        node = node->prev;
        count++;
    }
    return count;
}

/* Same walk toward the tail. */
// FUNCTION: LEGOLAND 0x0041cf00
int Track_CountTailPieces(TrackNode* node)
{
    int count = 0;
    while (node) {
        if (node->flags & 6) break;
        node = node->next;
        count++;
    }
    return count;
}

/* Copy ten vector hooks plus the requested dimension into a descriptor. */
// FUNCTION: LEGOLAND 0x00421540
void PhysVec_InitOps(PhysOps* out, int dimension)
{
    g_car_pool_hooks.dim = dimension;
    *out = g_car_pool_hooks;
}

/* Establish both directions of the car-to-seat association. */
// FUNCTION: LEGOLAND 0x00421590
void CoasterCar_AttachSeat(CoasterCar* car, RouteSeat* seat)
{
    RouteSeat_AttachCar(seat, car);
    car->seat = seat;
}

/* Persist a car's state and its rider's save-list index. */
// FUNCTION: LEGOLAND 0x00427050
void CoasterCar_WriteRef(CoasterCar* car, CarRef* out)
{
    out->state = car->state;
    out->rider_id = CoasterRider_GetSaveIndex(car->bloke);
}

/* Load the .ltx file associated with a model base name. */
// FUNCTION: LEGOLAND 0x00420750
void* CoasterModel_LoadLTX(const char* name)
{
    char path[256];
    wsprintfA(path, "%s.ltx", name);
    return CoasterModel_LoadFile(path, 0);
}

/* Vector-op slot 3 scales the live components in place. */
// FUNCTION: LEGOLAND 0x00421340
void PhysVec_Scale(PhysVec* vec, float scale)
{
    int i;
    for (i = 0; i < vec->n; i++)
        vec->v[i] *= scale;
}

/* Straight centerline, at constant height. */
// FUNCTION: LEGOLAND 0x004219c0
void TrackCurve_LinePosition(const Curve* curve, float t, Vec3f* out)
{
    out->x = curve->pos.x + curve->dir.x * t;
    out->y = curve->pos.y + curve->dir.y * t;
    out->z = curve->pos.z;
}

/* Reset the coaster's route and set its next deadline. */
// FUNCTION: LEGOLAND 0x00424ae0
void Coaster_ResetRouteDeadline(CoasterRec* rec, int ticks)
{
    Route_Reset(rec->route);
    Route_SetDeadline(rec->route, ticks);
}

/* Secondary route step temporarily replaces the solver derivative callback. */
// FUNCTION: LEGOLAND 0x0041e100
float Route_StepSecondary(CoasterRoute* rt, float distance)
{
    void* derivative = rt->derivative;
    float result = rt->owner->step_secondary(rt, distance);
    rt->derivative = derivative;
    return result;
}

/* Load the .lfm file with the caller's mode. */
// FUNCTION: LEGOLAND 0x004206d0
void* CoasterModel_LoadLFM(const char* name, int mode)
{
    char path[256];
    wsprintfA(path, "%s.lfm", name);
    return CoasterModel_LoadFile(path, mode);
}

/* Insert a new route node immediately after the embedded head sentinel. */
// FUNCTION: LEGOLAND 0x0041e420
void Route_PrependNode(CoasterRoute* rt, int kind)
{
    RouteNode* node = RouteNode_Alloc(kind);
    if (node) {
        rt->head.next->prev = node;
        node->next = rt->head.next;
        rt->head.next = node;
        node->prev = &rt->head;
    }
}

/* Unlink and free any route node except the route's own sentinel. */
// FUNCTION: LEGOLAND 0x0041e460
void Route_RemoveNode(RouteNode* node, CoasterRoute* rt)
{
    if (node != &rt->head) {
        node->next->prev = node->prev;
        node->prev->next = node->next;
        RouteNode_Free(node);
    }
}

/* Parallel positive-offset line, at constant height. */
// FUNCTION: LEGOLAND 0x00421a10
void TrackCurve_LineOffsetPlus(const Curve* curve, float t, Vec3f* out)
{
    out->x = curve->pos.x + (curve->offset.x + curve->dir.x * t);
    out->y = curve->pos.y + (curve->offset.y + curve->dir.y * t);
    out->z = curve->pos.z;
}

/* Parallel negative-offset line, at constant height. */
// FUNCTION: LEGOLAND 0x00421a40
void TrackCurve_LineOffsetMinus(const Curve* curve, float t, Vec3f* out)
{
    out->x = (curve->pos.x + curve->dir.x * t) - curve->offset.x;
    out->y = (curve->pos.y + curve->dir.y * t) - curve->offset.y;
    out->z = curve->pos.z;
}

/* Find the first car in state 1; NULL when the sentinel is reached. */
// FUNCTION: LEGOLAND 0x00424b30
CoasterCar* Coaster_FindState1Car(CoasterRec* rec)
{
    CoasterCar* car = rec->cars.next;
    while (car != &rec->cars) {
        if (car->state == 1) break;
        car = car->next;
    }
    return car != &rec->cars ? car : 0;
}

/* Find the first car in state 2. */
// FUNCTION: LEGOLAND 0x00424b60
CoasterCar* Coaster_FindState2Car(CoasterRec* rec)
{
    CoasterCar* car = rec->cars.next;
    while (car != &rec->cars) {
        if (car->state == 2) break;
        car = car->next;
    }
    return car != &rec->cars ? car : 0;
}

/* Find the first car in state 4 (the eviction candidate). */
// FUNCTION: LEGOLAND 0x00424b90
CoasterCar* Coaster_FindState4Car(CoasterRec* rec)
{
    CoasterCar* car = rec->cars.next;
    while (car != &rec->cars) {
        if (car->state == 4) break;
        car = car->next;
    }
    return car != &rec->cars ? car : 0;
}

/* Remaining locally typed records for route setup, saved riders and bounds. */
typedef struct PhysObj {
    void (*setstate)(struct PhysObj*, PhysVec*);
    void* getstate;
    void (*deriv)(struct PhysObj*, float, PhysVec*);
    PhysOps ops;
    PhysVec* state;
    CoasterRoute* route;
} PhysObj;
typedef struct RoutePhysics {
    unsigned char pad00[0x20];
    int dimension;
    float state[2];
    PhysObj physics; /* +2c */
} RoutePhysics;
typedef struct RouteGeom {
    unsigned char pad00[0x50];
    struct RouteGeom* next;
} RouteGeom;
typedef struct RoutePos { TrackNode* node; RouteGeom* geom; Vec3f pos; } RoutePos;
typedef struct RiderLink { struct RiderLink* next; int unused; void* bloke; } RiderLink;
typedef struct RiderClass { unsigned char pad00[0xcc]; RiderLink* riders; } RiderClass;
typedef struct Rect { int left, top, right, bottom; } Rect;
typedef struct TrackBoundsClass { unsigned char pad00[0x3c]; Rect bounds; } TrackBoundsClass;
typedef struct Pos16 { short x, y; } Pos16;
struct RouteSeat { unsigned char b[0x20]; };
typedef struct RouteNodeInit {
    int flags, kind;
    unsigned char pad08[0x70];
    RouteSeat seat[2];          /* +78, +98 */
    unsigned char padb8[0x2c];
    struct RouteNodeInit* prev;
    struct RouteNodeInit* next;
} RouteNodeInit;
typedef struct ShadeClampTable { unsigned char value[192]; } ShadeClampTable;

extern void PhysObj_Init(PhysObj* obj, int dimension);          /* 0x00420410 */
extern void RoutePhys_SetState(PhysObj* obj, PhysVec* value);   /* 0x0041ddd0 */
extern void RoutePhys_EvaluateDerivative(PhysObj* obj, float t, PhysVec* out); /* 0x0041de10 */
extern void* GetTrackNodeWorldPos(TrackNode* node, Vec3f* out); /* 0x0041cff0 */
extern int Coaster_GetLongestWait(CoasterRec* rec);            /* 0x00424bc0 */
extern int Coaster_CountWaitingCars(CoasterRec* rec);          /* 0x00424c10 */
extern RiderClass* CastleClassDef(int index);                 /* 0x0041ec00 */
extern void MapSquareToWorld(const short* sq, float height, Vec3f* out); /* 0x00425cb0 */
extern void* RouteNode_FindFreeSeat(RouteNode* node);          /* 0x0041e760 */
extern void RouteSeat_InitPosition(RouteSeat* seat, const Vec3f* pos); /* 0x004274b0 */
extern void Castle_GetFirstCorner(const Pos16* square, Pos16* out); /* 0x004239b0 */
extern void Castle_GetSecondCorner(const Pos16* square, Pos16* out); /* 0x004239e0 */
extern void Castle_InitEntranceTrack(void);                    /* 0x00423a10 */
extern int Route_AtStationEnd(CoasterRoute* route);            /* 0x00424960 */
extern void Coaster_StartIfComplete(CoasterRec* rec);          /* 0x00424ab0 */
extern int GetGameTimer(void);                                /* 0x00499430 */
extern void Coaster_StationDerivative(PhysObj* obj, float t, PhysVec* out); /* 0x004248b0 */
extern float Route_StepFree(CoasterRoute* route, float distance); /* 0x0041e000 */
extern unsigned char g_station_geometry[];                    /* 0x006103a8 */
extern int g_station_height;                                  /* 0x004b5b50 */
extern ShadeClampTable g_shade_clamp;                         /* 0x004d88f4 */
extern unsigned char* g_shade_clamp_mid;                      /* 0x004d89c4 */
extern Pos16 g_castle_first_corner;                           /* 0x004b5b58 */
extern Pos16 g_castle_second_corner;                          /* 0x004b5b60 */
extern float g_route_seat_x[3];                               /* 0x004b559c */
extern float g_route_seat_spacing;                            /* 0x004b55a8 */
extern Vec3f g_support_shadow_templates[12];                  /* 0x004b5f80 */
extern Vec3f g_shadow_src[12];                                /* 0x00612210 */

/* Return the station geometry, initial parameter and map-derived position. */
// FUNCTION: LEGOLAND 0x00424850
void Coaster_GetStationStart(const short* square_record, void** geometry,
                             float* parameter, Vec3f* position)
{
    *geometry = g_station_geometry;
    *parameter = 0.1f;
    MapSquareToWorld(square_record + 4, (float)g_station_height, position);
}

/* Install the route's two-component physics system and point it at state. */
// FUNCTION: LEGOLAND 0x0041dec0
void Route_InitPhysics(RoutePhysics* rt)
{
    PhysObj* phys = &rt->physics;
    PhysObj_Init(phys, 2);
    phys->setstate = RoutePhys_SetState;
    rt->physics.state = (PhysVec*)&rt->dimension;
    rt->physics.route = (CoasterRoute*)rt;
    rt->physics.deriv = RoutePhys_EvaluateDerivative;
    rt->dimension = 2;
}

/* Advance within a geometry chain, then onto the tail-linked track piece. */
// FUNCTION: LEGOLAND 0x0041f850
void TrackCursor_AdvanceGeometry(RoutePos* cursor)
{
    RouteGeom* geom = cursor->geom->next;
    if (!geom) {
        cursor->node = cursor->node->next;
        geom = (RouteGeom*)GetTrackNodeWorldPos(cursor->node, &cursor->pos);
    }
    cursor->geom = geom;
}

/* Vector-op slot 6 clears positive-length data and always stores n. */
// FUNCTION: LEGOLAND 0x00421400
void PhysVec_Zero(PhysVec* vec, int n)
{
    int i;
    for (i = 0; i < n; i++)
        vec->v[i] = 0.0f;
    vec->n = n;
}

/* Biased lookup clamps input [-64,127] to [0,63]. */
// FUNCTION: LEGOLAND 0x0041fd30
void CoasterShades_InitClamp(void)
{
    int i;
    memset(&g_shade_clamp.value[0], 0, 64);
    g_shade_clamp_mid = &g_shade_clamp.value[64];
    for (i = 64; i <= 127; i++)
        g_shade_clamp.value[i] = (unsigned char)(i - 64);
    memset(&g_shade_clamp.value[128], 63, 64);
}

/* Vector-op slot 4 is the maximum absolute component (infinity norm). */
// FUNCTION: LEGOLAND 0x00421360
float PhysVec_MaxAbs(const PhysVec* vec)
{
    float max = 0.0f;
    float value;
    int i;
    for (i = 0; i < vec->n; i++)
        if ((value = (float)fabs(vec->v[i])) > max)
            max = value;
    return max;
}

/* Linear horizontal coordinates and a cubic vertical profile. */
// FUNCTION: LEGOLAND 0x00421d60
void TrackCurve_CubicPosition(const Curve* curve, float t, Vec3f* out)
{
    out->x = curve->pos.x + curve->dir.x * t;
    out->y = curve->pos.y + curve->dir.y * t;
    out->z = curve->cubic[3] + t * (curve->cubic[2] + t *
             (curve->cubic[1] + curve->cubic[0] * t));
}

/* Start dispatch once any car waited >5000 ticks or >3 cars are waiting. */
// FUNCTION: LEGOLAND 0x00424c40
int Coaster_ShouldDispatch(CoasterRec* rec)
{
    int wait = Coaster_GetLongestWait(rec);
    int count = Coaster_CountWaitingCars(rec);
    if (wait > 5000 || count > 3)
        return 1;
    return 0;
}

/* Decode a one-hot direction, retaining zero for invalid masks. */
// FUNCTION: LEGOLAND 0x0041cca0
int JointDir_ToIndex(int direction)
{
    if (direction == 1) return 0;
    if (direction == 4) return 2;
    if (direction == 2) return 1;
    return direction == 8 ? 3 : 0;
}

/* Cubic-height positive-offset rail. */
// FUNCTION: LEGOLAND 0x00421df0
void TrackCurve_CubicOffsetPlus(const Curve* curve, float t, Vec3f* out)
{
    out->x = curve->pos.x + (curve->offset.x + curve->dir.x * t);
    out->y = curve->pos.y + (curve->offset.y + curve->dir.y * t);
    out->z = curve->cubic[3] + t * (curve->cubic[2] + t *
             (curve->cubic[1] + curve->cubic[0] * t));
}

/* Cubic-height negative-offset rail. */
// FUNCTION: LEGOLAND 0x00421e40
void TrackCurve_CubicOffsetMinus(const Curve* curve, float t, Vec3f* out)
{
    out->x = (curve->pos.x + curve->dir.x * t) - curve->offset.x;
    out->y = (curve->pos.y + curve->dir.y * t) - curve->offset.y;
    out->z = curve->cubic[3] + t * (curve->cubic[2] + t *
             (curve->cubic[1] + curve->cubic[0] * t));
}

/* Resolve a zero-based save index in castle class 0's rider list. */
// FUNCTION: LEGOLAND 0x00427020
void* CoasterRider_FromSaveIndex(int index)
{
    int i = 0;
    RiderLink* rider = CastleClassDef(0)->riders;
    while (rider) {
        if (index == i)
            return rider->bloke;
        rider = rider->next;
        i++;
    }
    return 0;
}

/* Translate a class's footprint rectangle by the packed map square.
 * Naming the embedded Rect changes the first pointer temporary: the entire
 * eax/ecx rotation closes (12 strict mismatches -> 0, unchanged 24i/62B). */
// FUNCTION: LEGOLAND 0x0041cce0
void TrackClass_GetWorldBounds(const short* square, TrackBoundsClass* cls, Rect* out)
{
    Rect* bounds = &cls->bounds;
    out->left = square[0] + bounds->left;
    out->right = bounds->right + square[0];
    out->top = bounds->top + square[1];
    out->bottom = bounds->bottom + square[1];
}

/* Vector-op slot 0 adds two vectors; the first vector determines length.
 * Naming b[i] eliminates one induction register (27i/57B -> 24i/53B). */
// FUNCTION: LEGOLAND 0x004212a0
void PhysVec_Add(const PhysVec* a, const PhysVec* b, PhysVec* out)
{
    int i;
    out->n = a->n;
    for (i = 0; i < a->n; i++) {
        float value = b->v[i];
        out->v[i] = a->v[i] + value;
    }
}

/* Vector-op slot 7 adds a scaled vector to the destination. */
// FUNCTION: LEGOLAND 0x00421430
void PhysVec_AddScaled(const PhysVec* a, float scale, PhysVec* out)
{
    int i;
    out->n = a->n;
    for (i = 0; i < a->n; i++)
        out->v[i] += a->v[i] * scale;
}

/* Set x87 single precision, nearest rounding, all exceptions masked.
 * Return the old control word. The original saves it into a DWORD slot
 * with a WORD store; only the low 16 bits are subsequently observed.
 */
// FUNCTION: LEGOLAND 0x004236f0
unsigned short Raster_SetFloatMode(void)
{
    unsigned int saved;
    unsigned short control;
    __asm { fstcw word ptr saved }
    if ((saved & 0x300) || ((unsigned char)saved & 0x3f) != 0x3f || (saved & 0xc00)) {
        __asm {
            mov ax, word ptr saved
            and ax, 0fcffh
            or ax, 3fh
            and ax, 0f3ffh
            mov control, ax
            fldcw control
        }
    }
    return (unsigned short)saved;
}

/* Search both seats of every route node, including the embedded head. */
// FUNCTION: LEGOLAND 0x0041e2b0
void* Route_FindFreeSeat(CoasterRoute* rt)
{
    RouteNode* node = &rt->head;
    void* seat;
    do {
        seat = RouteNode_FindFreeSeat(node);
        if (seat) return seat;
        node = node->next;
    } while (node != &rt->head);
    return 0;
}

/* Tangent to a planar circular arc, with zero vertical component.
 * Reuse t for cosine: the original stores it in the dead argument slot.
 * Direct field subscripts avoid a separately enregistered dir pointer. */
// FUNCTION: LEGOLAND 0x00421b90
void TrackCurve_ArcTangent(const Curve* curve, float t, Vec3f* out)
{
    float sine = (float)sin(t);
    int i;
    t = (float)cos(t);
    sine = -sine;
    for (i = 0; i < 2; i++)
        ((float*)out)[i] = sine * ((const float*)&curve->pos)[i]
                        + t * ((const float*)&curve->dir)[i];
    out->z = 0.0f;
}

/* Derivative of the cubic height, retaining the raw horizontal tangent. */
// FUNCTION: LEGOLAND 0x00421da0
void TrackCurve_CubicTangent(const Curve* curve, float t, Vec3f* out)
{
    *out = curve->dir;
    out->z = curve->cubic[2] + t * (curve->cubic[1] + curve->cubic[1] +
             (curve->cubic[0] * t) * 3.0f);
}

/* Build the two station corners relative to map square (0,0). */
// FUNCTION: LEGOLAND 0x00423d40
void Castle_InitStationCorners(void)
{
    Pos16 square, first, second;
    square.x = 0;
    square.y = 0;
    Castle_GetFirstCorner(&square, &first);
    Castle_GetSecondCorner(&square, &second);
    g_castle_first_corner.x = first.x;
    g_castle_first_corner.y = first.y;
    g_castle_second_corner.x = second.x;
    g_castle_second_corner.y = second.y;
    Castle_InitEntranceTrack();
}

/* End station departure when its endpoint is reached; otherwise free-step
 * using the station derivative. +e0 on the owner receives the exit time. */
// FUNCTION: LEGOLAND 0x00424990
float Coaster_StepStationDeparture(CoasterRoute* rt, float distance)
{
    if (Route_AtStationEnd(rt)) {
        Coaster_StartIfComplete(rt->owner);
        *(int*)((char*)rt->owner + 0xe0) = GetGameTimer();
        return distance;
    }
    rt->derivative = (void*)Coaster_StationDerivative;
    return Route_StepFree(rt, distance);
}

/* Vector-op slot 1 subtracts b from a. */
// FUNCTION: LEGOLAND 0x004212e0
void PhysVec_Subtract(const PhysVec* a, const PhysVec* b, PhysVec* out)
{
    int i;
    out->n = a->n;
    for (i = 0; i < a->n; i++)
        out->v[i] = a->v[i] - b->v[i];
}

/* Initialize the node's two seats and make its links a singleton ring. */
// FUNCTION: LEGOLAND 0x0041e6a0
void RouteNode_InitSeats(RouteNodeInit* node, int kind)
{
    Vec3f pos;
    pos.y = 0.0f;
    pos.z = -4.0f;
    node->kind = kind;
    pos.x = g_route_seat_x[kind];
    RouteSeat_InitPosition(&node->seat[0], &pos);
    pos.x -= g_route_seat_spacing;
    RouteSeat_InitPosition(&node->seat[1], &pos);
    node->next = node;
    node->prev = node;
    node->flags = 0;
}

/* Planar circular arc: pos*cos(t) + dir*sin(t) + offset. */
// FUNCTION: LEGOLAND 0x00421b40
void TrackCurve_ArcPosition(const Curve* curve, float t, Vec3f* out)
{
    float sine = (float)sin(t);
    float cosine = (float)cos(t);
    int i;
    for (i = 0; i < 2; i++)
        ((float*)out)[i] = ((const float*)&curve->offset)[i] +
                          (((const float*)&curve->dir)[i] * sine +
                           ((const float*)&curve->pos)[i] * cosine);
    out->z = curve->offset.z;
}

/* Scale three four-point shadow templates; z stays at its static value.
 * Original quirk: the first group copies source x into both x and y. */
// FUNCTION: LEGOLAND 0x00429270
void CoasterShadows_InitTemplates(void)
{
    int i;
    for (i = 0; i <= 3; i++) {
        g_shadow_src[i].x = g_support_shadow_templates[i].x * 5.0f;
        g_shadow_src[i].y = g_shadow_src[i].x;
    }
    for (i = 0; i <= 3; i++) {
        g_shadow_src[i + 4].x = g_support_shadow_templates[i + 4].x * 1.5f;
        g_shadow_src[i + 4].y = g_support_shadow_templates[i + 4].y * 1.5f;
    }
    for (i = 0; i <= 3; i++) {
        g_shadow_src[i + 8].x = g_support_shadow_templates[i + 8].x * 1.5f;
        g_shadow_src[i + 8].y = g_support_shadow_templates[i + 8].y * 1.5f;
    }
}
