/* LEGOLAND -- the coaster module's last small helpers, lane w18b.
 * Reconstructed for VC6 SP3 /O2 /Gy /Gd.
 *
 * WHAT THIS FILE RECOVERS
 * =======================
 * Twelve leaf helpers that the already-matched coaster/school-car files call
 * but nobody had written yet.  Together they close four small mechanisms:
 *
 *  * THE JOINT DIRECTION ALGEBRA.  coaster5.c had already deduced that
 *    TrackJoint's +0x00 is a four-way DIRECTION BIT and that 0x0041cc50 is
 *    "the opposite way"; the body confirms it exactly -- 1<->4, 2<->8, and
 *    the free end (-1) is its OWN opposite mapped to 1.  That last arm is not
 *    a fifth direction: `return dir == -1;` yields 1 (== NORTH) for a free
 *    end, which is a quiet original quirk, not a sentinel.  TrackJointSloped
 *    (0x00429910) is the piece-level predicate built on it: a piece is
 *    "sloped" when its class is NOT raised (desc->raised == 0) AND its two
 *    ends face opposite ways -- i.e. it is a plain straight run-through.
 *
 *  * THE ROUTE'S SEAT TABLE.  A RouteNode is 0xec bytes with its two 0x20-byte
 *    seats inline at +0x78 and +0x98 and its ring links at +0xe4/+0xe8.  A
 *    seat is { Vec3f pos; CoasterCar* car; } followed by FOUR method pointers
 *    at +0x10..+0x1c, and RouteSeat_InitPosition installs the same four every
 *    time -- so the "vtable" is not a class pointer but four inline slots:
 *      +0x10  0x004273c0  occupied?      (car != 0)
 *      +0x14  0x004273d0  attach car
 *      +0x18  0x004273f0  detach car
 *      +0x1c  0x00427410  the seat's per-tick update
 *    RouteNode_FindFreeSeat walks the two seats calling slot +0x10 and hands
 *    back the first whose car is null.
 *
 *  * THE RIDER SAVE INDEX.  CoasterRider_GetSaveIndex is the exact inverse of
 *    schoolcar8.c's CoasterRider_FromSaveIndex: both walk class 0's rider
 *    chain (ClassDef +0xcc, links {next, unused, bloke}) counting nodes.  The
 *    index that lands in a save file is therefore a POSITION in a live linked
 *    list, so it is only meaningful within one session's list order.
 *
 *  * THE ROUTE PHYSICS PLUMBING.  RoutePhys_SetState is the solver's
 *    "write the state vector back" hook installed by Route_InitPhysics: the
 *    two live components are the train's PARAMETER (v[0], fed to
 *    PositionRouteCars) and its SPEED (v[1], fed to Route_SetSpeed), and it
 *    re-stamps the dimension (+0x20) to 2 on every write.  Both floats are
 *    forwarded as RAW DWORDS.  Route_SumCarVelocity is the Sigma(car+0xc4)
 *    half of schoolcar7.c's Route_Reset, a do/while around the whole car ring
 *    including the embedded sentinel at +0x70.
 *
 * DIVERGENCES FROM THE CALLERS' EXTERNS (deliberate; nothing shared changed):
 *   - coaster6.c declares FindCoasterColour returning `void*`; it returns an
 *     INDEX into the module colour table, or -1 when the colour is absent.
 *   - schoolcar3.c declares Raster_SaveState `void`; it returns 0/1.
 *   - schoolcar8.c declares ModelRecord_GetName `void`; it returns 0/1, and
 *     declares RouteNode_FindFreeSeat returning `void*`/RouteNode_Alloc
 *     returning `RouteNode*`, which are the same objects under other names.
 *   - schoolcar.c calls 0x0041cc50 `Sub_41cc50`; coaster5.c's
 *     JointOppositeDir is kept.
 *
 * CODEGEN NOTES (measured on these bodies):
 *   - FindCoasterColour: the ENTRY COUNT must be a named local read BEFORE the
 *     cursor is derived. Left in the loop condition (`i < table[0]`) VC6
 *     hoists it AFTER the `lea` and puts `i = 0` above the callee-saved
 *     pushes, and the table pointer lands in edx instead of eax -- which also
 *     costs the one-byte `mov eax,moffs32` form of the global load. Six
 *     mismatches and the whole byte difference from one declaration.
 *   - RouteNode_FindFreeSeat: `for (i = 0; i <= 1; i++, seat++)`. The seat
 *     step as a BODY statement emits `add edi,0x20` before `inc esi`; in the
 *     increment clause after `i++` the pair comes out in the original's order
 *     (the recorded latch-order rule, third instance). `cmp esi,1 / jle` is
 *     `i <= 1`, not `i < 2`.
 *   - Vec3Cross: the textbook spelling is byte-exact; no `fld`-order lever is
 *     needed because each product's operands are both memory.
 * ========================================================================= */

typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct PhysVec { int n; float v[20]; } PhysVec;   /* one pool slot: 0x54 */
typedef struct PhysOps { void* op[10]; int dim; } PhysOps;
typedef struct RouteNode RouteNode;
typedef struct CoasterRoute CoasterRoute;
typedef struct RouteSeat RouteSeat;
typedef struct CoasterCar CoasterCar;

/* coaster6.c's TrackDesc, only as far as this file reads it. */
typedef struct TrackDesc {
    int           raised;       /* +0x00  bit 0 = an anchored piece */
    int           h0;           /* +0x04 */
    int           h1;           /* +0x08 */
    unsigned char pad0c[0x38 - 0x0c];
} TrackDesc;                    /* 0x38 */

/* surface.c's linear-bitmap view; 0x18 bytes, schoolcar3.c's RasterState. */
typedef struct VideoSurfaceInfo {
    long  pitch;                /* +0x00  bytes per scanline */
    int   width;                /* +0x04 */
    int   height;               /* +0x08 */
    void* bits;                 /* +0x0c */
    int   unused;               /* +0x10 */
    int   format;               /* +0x14 */
} VideoSurfaceInfo;

/* schoolcar8.c's ModelImage: one loaded record block plus its length. */
typedef struct ModelImage { char* data; int length; } ModelImage;

/* One seat of a route node. */
struct RouteSeat {
    Vec3f       pos;            /* +0x00 */
    CoasterCar* car;            /* +0x0c */
    int (*occupied)(RouteSeat*);              /* +0x10 */
    void (*attach)(RouteSeat*, CoasterCar*);  /* +0x14 */
    void (*detach)(RouteSeat*);               /* +0x18 */
    void (*update)(RouteSeat*);               /* +0x1c */
};                              /* 0x20 */

/* schoolcar8.c's RouteNodeInit, spelled for the two accessors here. */
typedef struct RouteNodeSeats {
    int           flags, kind;
    unsigned char pad08[0x78 - 0x08];
    RouteSeat     seat[2];      /* +0x78, +0x98 */
    unsigned char padb8[0xe4 - 0xb8];
    struct RouteNodeSeats* prev;
    struct RouteNodeSeats* next;
} RouteNodeSeats;               /* 0xec */

/* The car ring node, as far as the velocity sum reads it. */
struct RouteNode {
    unsigned char pad00[0xe4];
    RouteNode*    prev;         /* +0xe4 */
    RouteNode*    next;         /* +0xe8 */
};

/* schoolcar7.c's CoasterRoute, only as far as this file reaches. */
struct CoasterRoute {
    int           state;        /* +0x00 */
    int           started;      /* +0x04 */
    int           deadline;     /* +0x08 */
    unsigned char pos[0x14];    /* +0x0c  RoutePos */
    int           dimension;    /* +0x20 */
    float         parameter;    /* +0x24 */
    float         speed;        /* +0x28 */
    unsigned char pad2c[0x70 - 0x2c];
    RouteNode     head;         /* +0x70  embedded car-ring sentinel */
};

/* schoolcar8.c's PhysObj; the solver descriptor, route pointer at +0x3c. */
typedef struct PhysObj {
    void (*setstate)(struct PhysObj*, PhysVec*);
    void*   getstate;
    void (*deriv)(struct PhysObj*, float, PhysVec*);
    PhysOps ops;
    PhysVec* state;             /* +0x38 */
    CoasterRoute* route;        /* +0x3c */
} PhysObj;

/* schoolcar8.c's rider chain, hanging off class 0's +0xcc. */
typedef struct RiderLink { struct RiderLink* next; int unused; void* bloke; } RiderLink;
typedef struct RiderClass { unsigned char pad00[0xcc]; RiderLink* riders; } RiderClass;

/* The coaster module's colour table: { int n; unsigned rgb[n]; }. */

extern int   JointOppositeDir(int dir);                         /* 0x0041cc50 */
extern int   GetVideoSurface(VideoSurfaceInfo* out);            /* 0x00464310 */
extern char* ModelImage_FindRecord(ModelImage* image, int index); /* 0x00422340 */
extern char* ModelRecord_CopyToken(const char* rec, char* out);  /* 0x00422300 */
extern void* AllocZeroed(unsigned int size, int a, int b, int c); /* 0x004775b0 */
extern void  RouteNode_InitSeats(RouteNodeSeats* node, int kind); /* 0x0041e6a0 */
/* Both take a float; spelled `int` so the caller forwards the raw dword. */
extern void  PositionRouteCars(CoasterRoute* rt, int t, void* at); /* 0x0041da10 */
extern void  Route_SetSpeed(CoasterRoute* rt, int v);           /* 0x0041dad0 */
extern float RouteCar_GetVelocity(RouteNode* car);              /* 0x0041e810 */
extern RiderClass* CastleClassDef(int index);                   /* 0x0041ec00 */

extern void* g_raster_bits;                                     /* 0x004b5b20 */
extern int   g_zb_pitch;                                        /* 0x004b5b28 */
extern int*  g_coaster_colours;                                 /* 0x004d8bac */
extern int   RouteSeat_IsOccupied(RouteSeat* seat);             /* 0x004273c0 */
extern void  RouteSeat_AttachCar(RouteSeat* seat, CoasterCar* car); /* 0x004273d0 */
extern void  RouteSeat_DetachCar(RouteSeat* seat);              /* 0x004273f0 */
extern void  RouteSeat_Update(RouteSeat* seat);                 /* 0x00427410 */

/* -------------------------------------------------------------------------
 * The four-way direction algebra. NORTH(1) <-> SOUTH(4), EAST(2) <-> WEST(8);
 * a FREE end (-1) maps to 1, which is a quirk of the shipped code -- the
 * trailing arm is a plain `dir == -1` comparison, not a fifth case.
 * ------------------------------------------------------------------------- */
// FUNCTION: LEGOLAND 0x0041cc50
int JointOppositeDir(int dir)
{
    if (dir == 1) return 4;
    if (dir == 4) return 1;
    if (dir == 2) return 8;
    if (dir == 8) return 2;
    return dir == -1;
}

/* A piece is sloped when its class is not raised and its two ends face
 * opposite ways. */
// FUNCTION: LEGOLAND 0x00429910
int TrackJointSloped(const TrackDesc* d, int din, int dout)
{
    if (((d->raised == 0) & 1) != 0) {
        if (dout == JointOppositeDir(din))
            return 1;
    }
    return 0;
}

/* Point the 16-bit rasteriser at the locked surface. Its partner at
 * 0x00423790 is a bare `ret`, so nothing is ever restored. */
// FUNCTION: LEGOLAND 0x00423760
int Raster_SaveState(VideoSurfaceInfo* g)
{
    if (!GetVideoSurface(g))
        return 0;
    g_raster_bits = g->bits;
    g_zb_pitch = g->pitch >> 1;
    return 1;
}

/* Copy one model record's leading token out as a NUL-terminated name. */
// FUNCTION: LEGOLAND 0x00422390
int ModelRecord_GetName(ModelImage* image, char* out, int index)
{
    char* rec = ModelImage_FindRecord(image, index);
    if (!rec)
        return 0;
    *ModelRecord_CopyToken(rec, out) = 0;
    return 1;
}

/* Allocate a 0xec-byte route node and give it its two seats. */
// FUNCTION: LEGOLAND 0x0041eb30
RouteNodeSeats* RouteNode_Alloc(int kind)
{
    RouteNodeSeats* node = (RouteNodeSeats*)AllocZeroed(0xec, 0, 0, 0);
    if (!node)
        return 0;
    RouteNode_InitSeats(node, kind);
    return node;
}

/* Install the two live components of the solver's state vector. */
// FUNCTION: LEGOLAND 0x0041ddd0
void RoutePhys_SetState(PhysObj* obj, PhysVec* value)
{
    CoasterRoute* rt = obj->route;
    rt->dimension = 2;
    PositionRouteCars(rt, *(int*)&value->v[0], rt->pos);
    Route_SetSpeed(rt, *(int*)&value->v[1]);
}

/* Sum every car's +0xc4 velocity around the ring, sentinel included. */
// FUNCTION: LEGOLAND 0x0041dae0
float Route_SumCarVelocity(CoasterRoute* rt)
{
    float sum = 0.0f;
    RouteNode* car = &rt->head;
    do {
        sum += RouteCar_GetVelocity(car);
        car = car->next;
    } while (car != &rt->head);
    return sum;
}

/* The rider's save index is its position in class 0's rider chain. */
// FUNCTION: LEGOLAND 0x00426ff0
int CoasterRider_GetSaveIndex(void* bloke)
{
    int i = 0;
    RiderLink* rider = CastleClassDef(0)->riders;
    while (rider) {
        if (bloke == rider->bloke)
            return i;
        rider = rider->next;
        i++;
    }
    return -1;
}

/* out = a x b. */
// FUNCTION: LEGOLAND 0x00425cf0
void Vec3Cross(const Vec3f* a, const Vec3f* b, Vec3f* out)
{
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - a->x * b->z;
    out->z = a->x * b->y - a->y * b->x;
}

/* Find a 24-bit RGB in the module's colour table; -1 when absent. */
// FUNCTION: LEGOLAND 0x004207d0
int FindCoasterColour(int rgb)
{
    const int* table = g_coaster_colours;
    int n = table[0];
    const int* entry = table + 1;
    int i;
    for (i = 0; i < n; i++)
        if (((*entry++ ^ rgb) & 0xffffff) == 0)
            return i;
    return -1;
}

/* The first of the node's two seats whose car slot is empty. */
// FUNCTION: LEGOLAND 0x0041e760
RouteSeat* RouteNode_FindFreeSeat(RouteNodeSeats* node)
{
    int i;
    RouteSeat* seat = &node->seat[0];
    for (i = 0; i <= 1; i++, seat++) {
        if (!seat->occupied(seat))
            return seat;
    }
    return 0;
}

/* Place a seat and install its four inline method slots. */
// FUNCTION: LEGOLAND 0x004274b0
void RouteSeat_InitPosition(RouteSeat* seat, const Vec3f* pos)
{
    seat->pos = *pos;
    seat->car = 0;
    seat->occupied = RouteSeat_IsOccupied;
    seat->attach = RouteSeat_AttachCar;
    seat->detach = RouteSeat_DetachCar;
    seat->update = RouteSeat_Update;
}
