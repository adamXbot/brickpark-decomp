/* LEGOLAND -- the DRIVING SCHOOL cars, and the coaster internals that
 * coaster.c reaches through externs.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere).
 *
 * =========================================================================
 * THE DRIVING SCHOOL CAR  (0x00401000 .. 0x00402c10)
 * =========================================================================
 * coaster.c has the spawn (NewSchoolCar), the per-frame sweep
 * (TickSchoolCars) and the two occupancy counters; goldrush.c has the
 * per-car frame (StepSchoolCar). This file has the rest of the state
 * machine, and the central discovery is what the bytes from +0x30 to +0xb8
 * really are:
 *
 *     A CAR DRIVES A QUEUE OF WAYPOINTS, NOT A SINGLE TARGET.
 *
 *     +0x30  Waypoint wp[16];   16 * {int x; int y;} in 16.16 world units
 *     +0xbb  unsigned char n;   how many of them are live
 *
 * StepSchoolCar's "manoeuvre target" (+0x30/+0x34) is simply wp[0], the
 * FRONT of that queue; SchoolCarIdleStep (0x00401cd0) is not an idle step at
 * all but the queue POP -- it shifts wp[1..n-1] down over wp[0] and
 * decrements n. The four manoeuvre routines (0x00401080, 0x00401320,
 * 0x004015e0, 0x00401660) are queue PUSHES: each appends the run of
 * waypoints that traces one road manoeuvre and leaves n pointing past them.
 * ========================================================================= */

#include <math.h>
#include <string.h>

#pragma intrinsic(sqrt, memset)

typedef struct SchoolCar SchoolCar;

typedef struct CarPos { int x; int y; } CarPos;

/* One queued waypoint: a world position in 16.16. */
typedef struct Waypoint { int x; int y; } Waypoint;

struct SchoolCar {
    SchoolCar*     next;        /* +0x00 */
    unsigned short school;      /* +0x04  the school's packed map square */
    unsigned char  pad06[2];
    int            sx;          /* +0x08  screen position, this frame */
    int            sy;          /* +0x0c */
    int            wx;          /* +0x10  world x, 16.16 */
    int            wy;          /* +0x14  world y, 16.16 */
    CarPos         cur;         /* +0x18  map square */
    CarPos         start;       /* +0x20  the square the manoeuvre starts on */
    int            vx;          /* +0x28  velocity, 16.16 */
    int            vy;          /* +0x2c */
    Waypoint       wp[16];      /* +0x30  the waypoint queue */
    float          ux;          /* +0xb0  unit heading vector */
    float          uy;          /* +0xb4 */
    unsigned char  frame;       /* +0xb8  body frame / 16-way heading */
    unsigned char  b9;          /* +0xb9  last drawn frame */
    unsigned char  turn;        /* +0xba  8-way road heading, 0..7 */
    unsigned char  nwp;         /* +0xbb  live waypoints in wp[] */
    unsigned short stall;       /* +0xbc  frames spent stalled */
    unsigned short t_life;      /* +0xbe */
    unsigned short t_horn;      /* +0xc0 */
    unsigned char  on_road;     /* +0xc2 */
    unsigned char  livery;      /* +0xc3  1..3 */
    unsigned char  manoeuvre;   /* +0xc4  current manoeuvre code, 1..5 */
    unsigned char  c5;          /* +0xc5 */
    unsigned short top_speed;   /* +0xc6 */
    unsigned short speed;       /* +0xc8 */
    unsigned char  padca[2];
    void*          bloke;       /* +0xcc  the driver */
};                              /* 0xd0 */

extern SchoolCar* g_school_cars;                        /* 0x004c10d4 */

/* ==========================================================================
 * 0x00401970 -- is any OTHER car standing on or next to (x, y)?
 *
 * The Chebyshev-1 test is spelled as two half-open compares per axis against
 * the SAME field read (`x + 1 >= p->cur.x && x <= p->cur.x + 1`), which is
 * what gives the original's `lea/cmp/jl/inc/cmp/jg` pair. NewSchoolCar uses
 * it to refuse a spawn square that is already crowded.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x00401970
SchoolCar* FindSchoolCarNear(SchoolCar* self, int x, int y)
{
    SchoolCar* p = g_school_cars;

    while (p) {
        if (x + 1 >= p->cur.x && x <= p->cur.x + 1 &&
            y + 1 >= p->cur.y && y <= p->cur.y + 1 && p != self)
            return p;
        p = p->next;
    }
    return 0;
}

/* ==========================================================================
 * 0x00401c60 -- retire one car: give the driver back, stop its samples,
 * free the record and unlink it from g_school_cars.
 *
 * ORIGINAL BUG, reproduced: the record is freed BEFORE the unlink walk, so
 * every comparison in the unlink is against a dangling pointer (and, on the
 * head path, `c->next` was luckily cached first). Harmless with this heap,
 * but it is the original's behaviour.
 * ======================================================================== */
typedef struct Bloke {
    unsigned char pad00[0x60];
    unsigned char free_flag;    /* +0x60  bumped when the bloke is released */
} Bloke;

typedef struct BlokeSoundSource {
    int    kind;                /* +0x00  1 = a bloke */
    Bloke* bloke;               /* +0x04 */
    int    pad08;
    int    pad0c;
} BlokeSoundSource;

extern void KillAllSamplesFromSource(BlokeSoundSource* src);    /* 0x00496b80 */
extern void HeapFree_w(void* p);                                /* 0x0049e4d0 */

// FUNCTION: LEGOLAND 0x00401c60
void RetireSchoolCar(SchoolCar* c)
{
    BlokeSoundSource src;
    SchoolCar*       next;
    SchoolCar*       p;

    if (c) {
        next = c->next;
        src.kind = 1;
        ((Bloke*)c->bloke)->free_flag++;
        src.bloke = (Bloke*)c->bloke;
        KillAllSamplesFromSource(&src);
        HeapFree_w(c);
        if (c == g_school_cars) {
            g_school_cars = next;
            return;
        }
        p = g_school_cars;
        while (p->next != c)
            p = p->next;
        p->next = next;
    }
}

/* ==========================================================================
 * 0x004019c0 -- steer the car at the front waypoint and set its velocity.
 *
 * With no waypoints the car simply stops. Otherwise the vector to wp[0] is
 * normalised (the unit vector is cached at +0xb0/+0xb4 for the driver's 3D
 * heading), the 16-way body frame is taken from ArcTan256 rounded to the
 * nearest sixteenth of a turn, and -- this is the interesting rule -- the
 * car may only TURN ONE STEP PER FRAME: if the new heading is more than two
 * sixteenths away from the old one the body frame is nudged a single step
 * towards it instead of snapping. The velocity is then speed * delta / len,
 * i.e. `speed` units per frame along the line to the waypoint.
 *
 * Degenerate case: a waypoint the car is already exactly on gives len == 0,
 * which zeroes the unit vector and the velocity rather than dividing.
 * ======================================================================== */
extern int ArcTan256(int x, int y);                     /* 0x004806e0 */

/* 103 instructions against the original's 106, and every block, branch sense,
 * x87 sequence and store is reproduced; the residual is ONE prologue
 * placement decision and the two instructions it drags with it.
 *
 * THE RESIDUAL, precisely: the original SINKS `push ebp` / `push edi` into
 * the non-empty-queue path (they sit at 0x004019eb / 0x004019ef, each
 * immediately before the first def of its register), leaving only
 * `push ecx / push ebx / push esi` at entry, so the empty-queue exit pops
 * three registers and the body pops five. We emit all five pushes at entry
 * and both exits pop five. That also costs the original's `xor ebx,ebx`
 * zero-extension of `frame` (VC6 proves the upper bits dead in our shape)
 * and turns the original's `and al,0xf` into `and eax,0xf`.
 * Tried and measured, none of them move the pushes: the body in a post-guard
 * inner scope; the body in a `do { ... } while (0)` exited by `break`; the
 * body in a `static __inline` helper; a `goto` form with the empty-queue
 * stores as the trailing block; `if (nwp) {body} else {zero}`; `c->wx` /
 * `c->wy` hoisted into locals before and after the guard; a `Waypoint*`
 * cursor instead of `c->wp[0]`; `int frame` with a separate `unsigned char`
 * for the nudge; and both orders of the dx/dy statements. /Ox and /O1 do not
 * split it either, so it is a source shape, not a flag. */
// WIP-FUNCTION: LEGOLAND 0x004019c0  (103 insns vs 106; the original sinks push ebp/edi into the non-empty path)
void SchoolCarAccelerate(SchoolCar* c)
{
    unsigned char frame = c->frame;
    int   dy, dx, len, a, d, speed;
    float fy, fx;

    if (c->nwp == 0) {
        c->vx = 0;
        c->vy = 0;
        return;
    }
    dy = c->wp[0].y - c->wy;
    dx = c->wp[0].x - c->wx;
    fy = (float)dy;
    fx = (float)dx;
    len = (int)sqrt(fx * fx + fy * fy);
    if (len == 0) {
        c->vx = 0;
        c->ux = 0.0f;
        c->uy = 0.0f;
        c->vy = 0;
        return;
    }
    c->ux = fx / len;
    c->uy = fy / len;
    a = (((ArcTan256(dx, dy) + 8) >> 4)) & 0xf;
    c->frame = (unsigned char)a;
    d = (a - frame) & 0xf;
    if (d & 8)
        d |= ~0xf;
    if (d < -2 || d > 2) {
        if (d & 8)
            c->frame = --frame;
        else
            c->frame = ++frame;
        c->frame = (unsigned char)(frame & 0xf);
    }
    speed = c->speed;
    c->vx = speed * dx / len;
    c->vy = speed * dy / len;
}

/* =========================================================================
 * THE COASTER INTERNALS  (0x0041c... .. 0x0042a...)
 *
 * coaster.c has the track node primitives, the editor pass and the save
 * image and reaches everything below through externs. The records:
 *
 *   CoasterRoute  0x15c bytes, one per coaster, hung off CoasterRec +0xd8.
 *       +0x00 state/flags   +0x04 started   +0x08 deadline
 *       +0x0c sub[0x0c]     +0x28 speed (float)
 *       +0x6c owner (the CoasterRec)
 *       +0x70 the head RouteNode, embedded -- so the head's `next` at
 *             node +0xe8 IS route +0x158, which is why the route walk
 *             reads [route+0x158] for its first step.
 *
 *   RouteNode     0xec bytes, circular through +0xe8, payload at +0xe4.
 *
 * Route state bit 1 is the "closed" flag: Coaster_StartIfComplete tests it
 * and, when it is set, clears it and resets the route (state <- 8) before
 * running the record's follow-up.
 * ======================================================================= */

typedef struct CoasterRec  CoasterRec;
typedef struct CoasterRoute CoasterRoute;
typedef struct CoasterCar CoasterCar;
typedef struct TrackNode TrackNode;

typedef struct Vec3f { float x; float y; float z; } Vec3f;

/* The route's POSITION record: the live track node, its render object and
 * its world position. It appears three times -- as route +0x0c (the route's
 * own position), as RouteNode +0x44 (each car's), and as the twelve bytes
 * coaster.c calls `sub` in the save record. */
typedef struct RoutePos {
    struct TrackNode* node;     /* +0x00 */
    void*             obj;      /* +0x04 */
    Vec3f             pos;      /* +0x08 */
} RoutePos;                     /* 0x14 */

/* One car of the train riding the route. */
typedef struct RouteNode {
    unsigned char     pad00[0x40];
    int               f40;      /* +0x40  the car's spacing parameter */
    RoutePos          at;       /* +0x44  where this car is */
    unsigned char     pad58[0xe4 - 0x58];
    void*             data;     /* +0xe4 */
    struct RouteNode* next;     /* +0xe8 */
} RouteNode;                    /* 0xec */

struct CoasterRoute {
    int           state;        /* +0x00  bit 1 = closed, bit 6 = has deadline */
    int           started;      /* +0x04  game-clock stamp */
    int           deadline;     /* +0x08  game-clock stamp */
    RoutePos      pos;          /* +0x0c */
    unsigned char pad20[4];
    int           f24;          /* +0x24  saved with the car */
    float         speed;        /* +0x28 */
    unsigned char pad2c[0x6c - 0x2c];
    CoasterRec*   owner;        /* +0x6c */
    RouteNode     head;         /* +0x70  embedded list sentinel */
};                              /* 0x15c */

extern void* AllocZeroed(unsigned int size, int a, int b, int c);   /* 0x004775b0 */
extern void  Free_w(void* p);                                       /* 0x004775d0 */

/* ---- the module's counters and pools ----------------------------------- */
extern int g_track_square_count;                        /* 0x004d8268 */

/* coaster.c's TrackPlaceIfFits adds a piece's squares; RemoveTrackNode
 * subtracts them. It RETURNS the new total -- coaster.c declares it void
 * because its callers discard the result, but the `mov eax,[g]` first and
 * the store from eax only come out of `g += d; return g;`. */
// FUNCTION: LEGOLAND 0x0041d6d0
int AddTrackSquareCount(int d)
{
    g_track_square_count += d;
    return g_track_square_count;
}

/* ---- the route object -------------------------------------------------- */
extern void  Sub_41e6a0(RouteNode* head, int mode);     /* 0x0041e6a0 */
extern void  Sub_41e420(CoasterRoute* rt, int which);   /* 0x0041e420 */
extern void  Sub_41dec0(CoasterRoute* rt);              /* 0x0041dec0 */
extern void  Route_Reset(CoasterRoute* rt);             /* 0x0041e500 */
extern void  Sub_41e360(CoasterRoute* rt);              /* 0x0041e360 */
extern void  Sub_41e380(CoasterRoute* rt);              /* 0x0041e380 */
extern void  Sub_41e460(RouteNode* n, CoasterRoute* rt);/* 0x0041e460 */
extern void  Sub_41e330(CoasterRoute* rt, void (*fn)(RouteNode*)); /* 0x0041e330 */
extern void  RouteAddNode_Cb(RouteNode* n);             /* 0x0041e3c0 */
extern void* g_route_pending;                           /* 0x0082adec */

/* Allocate and wire a coaster's route object. The two Sub_41e420 passes run
 * in the order 2 then 1. */
// FUNCTION: LEGOLAND 0x0041e570
CoasterRoute* CreateCoasterRoute(CoasterRec* r)
{
    CoasterRoute* rt = (CoasterRoute*)AllocZeroed(0x15c, 0, 0, 0);

    if (!rt)
        return 0;
    rt->owner = r;
    Sub_41e6a0(&rt->head, 0);
    Sub_41e420(rt, 2);
    Sub_41e420(rt, 1);
    rt->state = 0;
    Sub_41dec0(rt);
    Route_Reset(rt);
    Sub_41e360(rt);
    return rt;
}

/* Tear a route down: the node ring is walked from the EMBEDDED head, which
 * is itself released as the first "node" of the walk, and the whole 0x15c
 * block is freed through the slot it was stored in. */
// FUNCTION: LEGOLAND 0x0041e5d0
void DestroyCoasterRoute(CoasterRoute** slot)
{
    CoasterRoute* rt = *slot;
    RouteNode*    head = &rt->head;
    RouteNode*    n = head;
    RouteNode*    next;

    Sub_41e380(rt);
    do {
        next = n->next;
        Sub_41e460(n, rt);
        n = next;
    } while (next != head);
    Free_w(*slot);
    *slot = 0;
}

/* Append one track piece to a route. The piece is handed over through a
 * module global because the walk callback takes only the node. */
// FUNCTION: LEGOLAND 0x0041e3e0
void Route_AddNode(CoasterRoute* rt, void* piece)
{
    g_route_pending = piece;
    Sub_41e330(rt, RouteAddNode_Cb);
}

/* Route state bit 1: the circuit is closed. The writers (0x0041e4c0,
 * 0x0041e4f0) treat +0x00 as a dword; this reader narrows to its low byte.
 * 4 of 5 instructions: the original loads the byte with `movsx eax,byte[eax]`
 * and we get `mov al,[eax]`. Measured and rejected: an `int` field (VC6
 * reassociates to `sar 1 / and 1`), `(x & 2) / 2`, a `signed char` local, an
 * `unsigned` result, char bitfield containers (signed and unsigned), and a
 * `volatile char` read -- volatile DOES buy the `movsx`, but then the shift
 * reassociates and the `and eax,2` is lost, so the two halves of the
 * instruction pair cannot be bought at the same time. The `and` survives
 * here only because the byte load leaves eax's upper bits dirty. */
// WIP-FUNCTION: LEGOLAND 0x0041e4a0  (4/5; `mov al,[eax]` vs the original's `movsx eax,byte ptr [eax]`)
int Route_IsClosed(CoasterRoute* rt)
{
    return ((unsigned)(*(char*)rt & 2)) >> 1;
}

/* Clear that bit and put the route back into state 8 (running). */
// FUNCTION: LEGOLAND 0x0041e4f0
void Route_Open(CoasterRoute* rt)
{
    rt->state &= ~2;
    Route_Reset(rt);
}

/* The route's speed, stored as a float at +0x28. coaster.c's caller passes
 * the raw dword out of the save record, so it declares the parameter `int`;
 * the callee really takes a float. */
// FUNCTION: LEGOLAND 0x0041dad0
void Route_SetSpeed(CoasterRoute* rt, float v)
{
    *(float*)((char*)rt + 0x28) = v;
}

/* ---- the coaster's car list -------------------------------------------- */
typedef struct CoasterCar {
    int               state;    /* +0x00  bit 0 = the +0x08 block is borrowed */
    unsigned char     pad04[4];
    void*             body;     /* +0x08  owned unless state bit 0 is set */
    void*             rider;   /* +0x0c  the bloke aboard */
    struct CoasterCar* prev;    /* +0x10 */
    struct CoasterCar* next;    /* +0x14 */
    unsigned char     pad18[4];
    int               born;     /* +0x1c  GetGameTimer() stamp */
} CoasterCar;

/* Unlink one car and free it. The two link re-reads are the original's: the
 * first store may alias, so `c->next` and `c->prev` are loaded twice. */
// FUNCTION: LEGOLAND 0x00421980
void KillCoasterCar(CoasterCar* c)
{
    c->prev->next = c->next;
    c->next->prev = c->prev;
    if ((c->state & 1) == 0)
        Free_w(c->body);
    Free_w(c);
}

extern CoasterCar* Sub_424b60(CoasterRec* r);           /* 0x00424b60 */
extern void        Sub_4215b0(CoasterCar* c);           /* 0x004215b0 */

/* Once the circuit closes, every car the record still has parked is started
 * and moved into state 4. */
// FUNCTION: LEGOLAND 0x00424d80
void Coaster_StartQueuedCars(CoasterRec* r)
{
    CoasterCar* c = Sub_424b60(r);

    while (c) {
        Sub_4215b0(c);
        c->state = 4;
        c = Sub_424b60(r);
    }
}

extern void Coaster_StartIfComplete(CoasterRec* r);     /* 0x00424ab0 */
extern CoasterRec g_castle;                             /* 0x00829ae0 */

// FUNCTION: LEGOLAND 0x00424e70
void Castle_StartCoasterIfComplete(void)
{
    Coaster_StartIfComplete(&g_castle);
}

/* ---- the coaster's rect list (0x008299a0 sentinel, 0x008299b8 head) -----
 * Each node carries a rectangle at +0x00..+0x0f, the clip code the rect
 * helper at 0x004265d0 produced for it at +0x10, and its links at +0x14 /
 * +0x18; the walk steps through +0x18. A code of 15 means all four edges of
 * the node's rect were inside the reference rect at 0x008299ac, i.e. the
 * region is fully covered. Coaster_AddNodeToRoute only extends the route
 * while one of them is. */
typedef struct CoasterRegion {
    int                   rect[4];  /* +0x00 */
    int                   code;     /* +0x10  clip code, 15 = fully inside */
    struct CoasterRegion* f14;      /* +0x14 */
    struct CoasterRegion* next;     /* +0x18 */
} CoasterRegion;

extern CoasterRegion  g_coaster_regions;        /* 0x008299a0 sentinel (0x829a3c is
                                                 * its own base, next @0x829a54) */

// FUNCTION: LEGOLAND 0x00426650
int AnyCoasterRegionFullyInside(void)
{
    CoasterRegion* p = g_coaster_regions.next;

    while (p != &g_coaster_regions) {
        if (p->code == 15)
            return 1;
        p = p->next;
    }
    return 0;
}

/* ---- Castle_Create's initialisers -------------------------------------- */
extern void Sub_421540(void* pool, int n);              /* 0x00421540 */
extern int  g_4d8270;                                   /* 0x004d8270 */
extern int  g_615f98;                                   /* 0x00615f98 */

// FUNCTION: LEGOLAND 0x0041e620
void RouteNodePoolInit(void)
{
    Sub_421540(&g_4d8270, 10);
}

// FUNCTION: LEGOLAND 0x0042a2e0
void CoasterFxPoolInit(void)
{
    Sub_421540(&g_615f98, 3);
}

/* A stub that reports success; Castle_Create runs it as the seventh of its
 * eleven initialisers. */
// FUNCTION: LEGOLAND 0x0041ef00
int RouteSystemInit(void)
{
    return 1;
}

/* The three coaster train models, each fetched twice -- once through
 * 0x004206b0 and once through 0x00420710 -- into two parallel triples of
 * module globals. */
extern void* Sub_4206b0(const char* name);              /* 0x004206b0 */
extern void* Sub_420710(const char* name);              /* 0x00420710 */
extern void* g_train_head_a;                            /* 0x0082add0 */
extern void* g_train_mid_a;                             /* 0x0082add4 */
extern void* g_train_tail_a;                            /* 0x0082add8 */
extern void* g_train_head_b;                            /* 0x0082ade0 */
extern void* g_train_mid_b;                             /* 0x0082ade4 */
extern void* g_train_tail_b;                            /* 0x0082ade8 */

// FUNCTION: LEGOLAND 0x0041eb70
void LoadCoasterTrainModels(void)
{
    g_train_head_a = Sub_4206b0("coastertrain.headcar");
    g_train_mid_a  = Sub_4206b0("coastertrain.midcar");
    g_train_tail_a = Sub_4206b0("coastertrain.tailcar");
    g_train_head_b = Sub_420710("coastertrain.headcar");
    g_train_mid_b  = Sub_420710("coastertrain.midcar");
    g_train_tail_b = Sub_420710("coastertrain.tailcar");
}

/* ---- the track node and the castle record, as this file reaches them ----
 * Same offsets coaster.c documents. Two additions this file discovers:
 *   * a JOINT's second word (+0x04) is a FLOAT -- the joint's world height.
 *     GetTrackNodeWorldPos hands it to MapSquareToWorld as the `h` argument
 *     and LevelTrackRun writes it, so InitTrackJoint's `f04 = 0` is 0.0f.
 *   * a RAISED piece (state bit 0) caches its world position at +0x40 and
 *     its render object at +0x4c, so it never re-derives them from the
 *     square. Those bytes overlap what coaster.c calls the footprint element
 *     at +0x2c, which therefore cannot be a whole 0x24-byte FootPart. */
typedef struct TrackDesc TrackDesc;

typedef struct TrackJoint {
    int        height;          /* +0x00  -1 while the end is free */
    float      h;               /* +0x04  world height of this end */
    TrackNode* node;            /* +0x08  the piece joined here */
} TrackJoint;

struct TrackDesc {
    int   raised;               /* +0x00  bit 0 = a raised piece */
    int   h0;                   /* +0x04 */
    int   h1;                   /* +0x08 */
    unsigned char pad0c[0x24 - 0x0c];
    void* (*query)(TrackNode*); /* +0x24 */
};

/* The piece's CLASS record: only its object element (+0xc4) is used here. */
typedef struct TrackClass {
    unsigned char pad00[0xc4];
    void*         elem;         /* +0xc4  the class's object element */
} TrackClass;

struct TrackNode {
    int         state;          /* +0x00  bit 0 raised, bit 1 in a ring */
    short       sq[2];          /* +0x04  map square */
    TrackClass* cls;            /* +0x08 */
    TrackDesc* desc;            /* +0x0c */
    void*      owner;           /* +0x10 */
    TrackJoint jin;             /* +0x14 */
    TrackJoint jout;            /* +0x20 */
    unsigned char pad2c[0x40 - 0x2c];
    Vec3f      pos;             /* +0x40  cached world position (raised only) */
    int        obj;             /* +0x4c  cached render object (raised only) */
};

struct CoasterRec {
    int           state;        /* +0x00  0 none, 1 open route, 2 closed */
    TrackNode     ring;         /* +0x04  the list sentinel */
    unsigned char pad54[0xa8 - 0x54];
    TrackNode*    head_node;    /* +0xa8 */
    unsigned char padac[0xc0 - 0xac];
    TrackNode*    tail_node;    /* +0xc0 */
    unsigned char padc4[0xd8 - 0xc4];
    CoasterRoute* route;        /* +0xd8 */
    unsigned char paddc[0xe4 - 0xdc];
    CoasterCar    cars;         /* +0xe4  car list sentinel */
};

/* ---- counting -------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00427130
int CountCoasterCars(CoasterRec* r)
{
    CoasterCar* head;
    CoasterCar* c = r->cars.next;
    int         n = 0;

    head = &r->cars;
    while (c != head) {
        c = c->next;
        n++;
    }
    return n;
}

/* The piece count. A CLOSED circuit is one ring walked through jout; an OPEN
 * route is the two NULL-terminated chains growing out of the station, one
 * through jout and one through jin. Either way the station itself counts,
 * which is why the total starts at 1. */
// FUNCTION: LEGOLAND 0x00427150
int CountCoasterNodes(CoasterRec* r)
{
    TrackNode* n;
    int        count = 1;

    if (r->state == 2) {
        n = r->ring.jout.node;
        while (n != &r->ring) {
            n = n->jout.node;
            count++;
        }
        return count;
    }
    n = r->ring.jout.node;
    while (n) {
        n = n->jout.node;
        count++;
    }
    n = r->ring.jin.node;
    while (n) {
        n = n->jin.node;
        count++;
    }
    return count;
}

/* ---- track node geometry ---------------------------------------------- */
extern int   Sub_41cc50(int h);                         /* 0x0041cc50 */
extern int   Sub_41cca0(int h);                         /* 0x0041cca0 */
extern int   Sub_41cf00(TrackNode* n);                  /* 0x0041cf00 */
extern int   Sub_41cee0(TrackNode* n);                  /* 0x0041cee0 */
extern void  MapSquareToWorld(const short* sq, float h, Vec3f* out);  /* 0x00425cb0 */

/* The piece's two-bit-per-end slope code: each free end (height -1) borrows
 * the other end's height first. The tail code is the high pair. */
// FUNCTION: LEGOLAND 0x0041ce60
int TrackNodeSlopeCode(TrackNode* n)
{
    int a = n->jin.height;
    int b;

    if (a == -1)
        a = Sub_41cc50(n->jout.height);
    b = Sub_41cca0(a);
    a = n->jout.height;
    if (a == -1)
        a = Sub_41cc50(n->jin.height);
    return (Sub_41cca0(a) << 2) | b;
}

/* Where a piece is in the world, and what to draw for it. A raised piece has
 * both cached; a flat one derives the position from its map square and asks
 * the class descriptor's query hook for the object. */
// FUNCTION: LEGOLAND 0x0041cff0
void* GetTrackNodeWorldPos(TrackNode* n, Vec3f* out)
{
    if (n->state & 1) {
        *out = n->pos;
        return (char*)n + 0x4c;
    }
    MapSquareToWorld(n->sq, n->jin.h, out);
    return n->desc->query(n);
}

/* The two end heights of an OPEN route, in steps; a closed circuit has none.
 * coaster.c passes the fit record's +0x04 for `r` and declares it `int`: it
 * really is the coaster record the fit belongs to. */
// FUNCTION: LEGOLAND 0x0041cf70
void GetOpenEndSteps(CoasterRec* r, int* head_steps, int* tail_steps)
{
    *head_steps = 0;
    *tail_steps = 0;
    if (r->state != 2) {
        *tail_steps = Sub_41cf00(r->tail_node);
        *head_steps = Sub_41cee0(r->head_node);
    }
}

/* Level the run of un-raised pieces that ends at `n` onto the height of the
 * next RAISED piece downstream: walk forward through jout to the first piece
 * whose class is raised, take its second end height, then walk back through
 * jin writing that height into both joints and clearing each piece's raised
 * bit. */
// FUNCTION: LEGOLAND 0x00429a30
void LevelTrackRun(TrackNode* n)
{
    TrackNode* p = n;
    TrackNode* q;
    float      h;

    while (!(p->desc->raised & 1))
        p = p->jout.node;
    h = (float)p->desc->h1;
    for (q = p->jin.node; q != n->jin.node; q = q->jin.node) {
        q->jin.h = h;
        q->jout.h = h;
        q->state &= ~1;
    }
}

/* ---- more of Castle_Create's initialisers ------------------------------ */
extern void* g_wheel_a;                                 /* 0x00616000 */
extern void* g_wheel_b;                                 /* 0x00616004 */

// FUNCTION: LEGOLAND 0x0042a780
void LoadCoasterWheelModel(void)
{
    g_wheel_a = Sub_4206b0("coastertrain.wheel01");
    g_wheel_b = Sub_420710("coastertrain.wheel01");
}

/* ---- route ticking ----------------------------------------------------- */
extern void Sub_41e400(CoasterRoute* rt);               /* 0x0041e400 */
extern int  Route_HasDeadline(CoasterRoute* rt);        /* 0x0041e4b0 */
extern void Sub_41e240(CoasterRoute* rt);               /* 0x0041e240 */
extern void Sub_41e130(CoasterRoute* rt);               /* 0x0041e130 */
extern void Sub_41e3a0(CoasterRoute* rt);               /* 0x0041e3a0 */

/* One frame of the coaster's route. Every test re-reads rec->route because
 * the calls in between may replace it. */
// FUNCTION: LEGOLAND 0x00424a50
void Coaster_TickRoute(CoasterRec* r)
{
    CoasterRoute* rt = r->route;

    Sub_41e400(rt);
    if (Route_IsClosed(r->route)) {
        if (Route_HasDeadline(r->route))
            Sub_41e240(rt);
        Sub_41e130(rt);
    }
    Sub_41e3a0(rt);
}

/* ---- riders ------------------------------------------------------------ */
extern CoasterCar* Sub_424b90(CoasterRec* r);           /* 0x00424b90 */

/* Scrap every car that still has a rider: the rider's action byte is forced
 * to 0x21 (the same byte RetireSchoolCar bumps) and the car is unlinked and
 * freed. */
// FUNCTION: LEGOLAND 0x00424dc0
void Coaster_EvictRidingCars(CoasterRec* r)
{
    CoasterCar* c = Sub_424b90(r);

    while (c) {
        ((Bloke*)c->rider)->free_flag = 0x21;
        KillCoasterCar(c);
        c = Sub_424b90(r);
    }
}

/* ---- the coaster save blob's CAR records --------------------------------
 * Each saved car is 8 bytes: {int state; int rider}. coaster.c calls them
 * CoasterNodeRef {node, link} because that is all its own walk needs. */
typedef struct CoasterCarRef {
    int   state;                /* +0x00  the car's state word, 2 = riding */
    int   rider;                /* +0x04  the rider's saved id */
} CoasterCarRef;

extern void*       Sub_427020(int rider);                       /* 0x00427020 */
extern CoasterCar* Coaster_AddCar(void* bloke, CoasterRec* r);  /* 0x00421930 */
extern void*       Sub_41e2b0(CoasterRoute* rt);                /* 0x0041e2b0 */
extern void        Sub_421590(CoasterCar* c, void* a);          /* 0x00421590 */
extern void        Sub_427050(CoasterCar* c, CoasterCarRef* out); /* 0x00427050 */

/* LOAD: re-create one car from its saved record. A car that was riding
 * (state 2) is put back onto the route. */
// FUNCTION: LEGOLAND 0x00427070
void RestoreCoasterCar(CoasterCarRef* p, CoasterRec* r)
{
    void*       bloke = Sub_427020(p->rider);
    CoasterCar* c = Coaster_AddCar(bloke, r);
    int         st = p->state;

    c->state = st;
    if (st == 2)
        Sub_421590(c, Sub_41e2b0(r->route));
}

/* SAVE: one 8-byte record per live car, in list order. */
// FUNCTION: LEGOLAND 0x004270c0
void SaveCoasterCars(CoasterRec* r, CoasterCarRef* out)
{
    CoasterCar* head = &r->cars;
    CoasterCar* c = r->cars.next;

    while (c != head) {
        Sub_427050(c, out++);
        c = c->next;
    }
}

extern TrackNode* Sub_426ea0(const void* ref, CoasterRec* r);   /* 0x00426ea0 */

/* LOAD: turn a saved node reference back into a live piece and cache its
 * world position with it. */
// FUNCTION: LEGOLAND 0x00426f10
void RestoreRoutePos(const void* ref, RoutePos* out, CoasterRec* r)
{
    out->node = Sub_426ea0(ref, r);
    out->obj = GetTrackNodeWorldPos(out->node, &out->pos);
}

/* The save side of RestoreCoasterCar (coaster.c, 0x00426f90): both timers go
 * out RELATIVE to the moment of saving. */
typedef struct CoasterCarSave {
    int  f00;                   /* +0x00 */
    int  f04;                   /* +0x04  route +0x24 */
    int  f08;                   /* +0x08  route +0x28, the speed float */
    int  elapsed;               /* +0x0c  now - started */
    int  remaining;             /* +0x10  deadline - now */
    unsigned char sub[0x0c];    /* +0x14  the packed RoutePos */
} CoasterCarSave;

extern void Sub_426ec0(RoutePos* pos, unsigned char* out);      /* 0x00426ec0 */
extern int  GetGameTimer(void);                                 /* 0x00499430 */

// FUNCTION: LEGOLAND 0x00426f40
void SaveCoasterRouteState(CoasterRoute* rt, CoasterCarSave* out)
{
    Sub_426ec0(&rt->pos, out->sub);
    out->f00 = rt->state;
    out->f04 = rt->f24;
    out->f08 = *(int*)&rt->speed;
    out->elapsed = GetGameTimer() - rt->started;
    out->remaining = rt->deadline - GetGameTimer();
}

/* ---- the other end of the level-a-run pair ----------------------------- */

/* The mirror of LevelTrackRun: walk BACK through jin to the first raised
 * piece, take its FIRST end height (desc +0x04) and level the run forward
 * through jout. */
// FUNCTION: LEGOLAND 0x004299e0
void LevelTrackRunBack(TrackNode* n)
{
    TrackNode* p = n;
    TrackNode* q;
    float      h;

    while (!(p->desc->raised & 1))
        p = p->jin.node;
    h = (float)p->desc->h0;
    for (q = p->jout.node; q != n->jout.node; q = q->jout.node) {
        q->jin.h = h;
        q->jout.h = h;
        q->state &= ~1;
    }
}

/* ---- does a piece cover a given map square? ---------------------------- */
/* Sub_41cce0 fills a 4-int bounding box (in a 20-byte record) for the piece's
 * square and class; the test is inclusive on all four edges. */
typedef struct TrackBox { int left; int top; int right; int bottom; int f10; } TrackBox;

extern void Sub_41cce0(const short* sq, void* cls, TrackBox* out);  /* 0x0041cce0 */

// FUNCTION: LEGOLAND 0x0041d0b0
int TrackNodeCoversSquare(TrackNode* n, const short* sq)
{
    TrackBox box;

    Sub_41cce0(n->sq, n->cls, &box);
    if (sq[0] >= box.left && sq[0] <= box.right &&
        sq[1] >= box.top && sq[1] <= box.bottom)
        return 1;
    return 0;
}

/* ---- taking a piece's object off the map -------------------------------
 * The packed {u8,u8} map square is built in the DEAD ARGUMENT SLOT (frame
 * +0x04) and the two-int MapRefI in the function's own 8-byte frame. */
typedef struct MapRefI { int x; int y; } MapRefI;
typedef struct MapPos  { unsigned char x; unsigned char y; } MapPos;

extern void  BasicObjectDCalcCursor(void* elem, MapRefI* pos);  /* 0x00480bb0 */
extern void  TrackRemoveObject(void* elem, MapPos sq, void* cursor); /* 0x0041edb0 */
extern void* g_810160;                                          /* 0x00810160 */

// FUNCTION: LEGOLAND 0x0041d760
void RemoveTrackNodeObject(TrackNode* n)
{
    MapRefI pos;
    MapPos  sq;

    /* The two records are filled a coordinate at a time, x then y -- writing
     * each record whole reorders the four stores. */
    sq.x = (unsigned char)n->sq[0];
    pos.x = n->sq[0];
    sq.y = (unsigned char)n->sq[1];
    pos.y = n->sq[1];
    BasicObjectDCalcCursor(n->cls->elem, &pos);
    TrackRemoveObject(n->cls->elem, sq, &g_810160);
}

/* ---- the two 3D end passes of DrawTrackPiece3D --------------------------
 * coaster.c's DrawTrackPiece3D runs three passes over a piece; these are the
 * two END ones. The first asks the object's own transform hook (its +0x4c
 * vtable, slot +0x18) for the model and CACHES it in g_615f6c; the second
 * reuses the cache, which is why the pair must run in that order and why
 * mode decides which end goes first, not which pass. Both bracket the work
 * with the renderer's push/pop state pair (0x004236f0 / 0x00423730). */
typedef struct DrawObj DrawObj;

typedef struct DrawObjVt {
    unsigned char pad00[0x18];
    void*       (*transform)(DrawObj*, void*);  /* +0x18 */
} DrawObjVt;

struct DrawObj {
    unsigned char pad00[0x4c];
    DrawObjVt*    vt;           /* +0x4c */
};

extern void* Sub_4236f0(void);                                  /* 0x004236f0 */
extern void  Sub_423730(void* saved);                           /* 0x00423730 */
extern void  Sub_428cb0(DrawObj* o, void* b, int c, void* model, void* ctx); /* 0x00428cb0 */
extern void  Sub_4234e0(void* pal);                             /* 0x004234e0 */
extern void* g_615f6c;                                          /* 0x00615f6c */
extern int   g_612178;                                          /* 0x00612178 */
extern int   g_4b5f60;                                          /* 0x004b5f60 */

// FUNCTION: LEGOLAND 0x00428e70
void DrawTrackEnd_Fetch(DrawObj* o, void* b, int c)
{
    void* saved = Sub_4236f0();
    void* model = o->vt->transform(o, &g_612178);

    g_615f6c = model;
    Sub_428cb0(o, b, c, model, &g_612178);
    Sub_4234e0(&g_4b5f60);
    Sub_423730(saved);
}

// FUNCTION: LEGOLAND 0x00428ec0
void DrawTrackEnd_Cached(DrawObj* o, void* b, int c)
{
    void* saved = Sub_4236f0();

    Sub_428cb0(o, b, c, g_615f6c, &g_612178);
    Sub_4234e0(&g_4b5f60);
    Sub_423730(saved);
}

/* ---- the coaster CAR pool and its dispatch tables -----------------------
 * Castle_Create runs both of these. The pool is 30 fixed 0x54-byte car
 * slots at 0x004dcc00 with a parallel pointer table at 0x0082ac60; the
 * allocator (0x004214f0) hands out a RUN of consecutive slots by bumping the
 * high-water mark at 0x004dd5d8 and returning &table[old], and the free
 * (0x00421510) just winds the mark back, so it is a stack, not a free list.
 * The ten hooks at 0x004dcbd0 are that allocator's own interface. */
typedef struct CarSlot { unsigned char b[0x54]; } CarSlot;

extern CarSlot  g_car_pool[30];                 /* 0x004dcc00 */
extern CarSlot* g_car_slot_table[30];           /* 0x0082ac60 */
extern void*    g_car_pool_hooks[10];           /* 0x004dcbd0 */

extern void Sub_4212a0(void);   /* 0x004212a0 */
extern void Sub_4212e0(void);   /* 0x004212e0 */
extern void Sub_421320(void);   /* 0x00421320 */
extern void Sub_421340(void);   /* 0x00421340 */
extern void Sub_421360(void);   /* 0x00421360 */
extern void Sub_4213a0(void);   /* 0x004213a0 */
extern void Sub_421400(void);   /* 0x00421400 */
extern void Sub_421430(void);   /* 0x00421430 */
extern void CarPool_Alloc(void);/* 0x004214f0 */
extern void CarPool_Free(void); /* 0x00421510 */

// FUNCTION: LEGOLAND 0x00421470
void CarPoolInit(void)
{
    int i;

    g_car_pool_hooks[0] = (void*)Sub_4212a0;
    g_car_pool_hooks[1] = (void*)Sub_4212e0;
    g_car_pool_hooks[2] = (void*)Sub_421320;
    g_car_pool_hooks[3] = (void*)Sub_421340;
    g_car_pool_hooks[4] = (void*)Sub_421360;
    g_car_pool_hooks[5] = (void*)Sub_4213a0;
    g_car_pool_hooks[6] = (void*)Sub_421400;
    g_car_pool_hooks[7] = (void*)Sub_421430;
    g_car_pool_hooks[8] = (void*)CarPool_Alloc;
    g_car_pool_hooks[9] = (void*)CarPool_Free;
    for (i = 0; i < 30; i++)
        g_car_slot_table[i] = &g_car_pool[i];
}

/* The three CAR CLASS vtables at 0x004dd5e0, eight slots each -- head car,
 * middle car and tail car, in that order. Slots 1, 3 and 5 of each are the
 * same routine, which is what lets VC6 hoist the three repeated addresses
 * into eax and emit those nine stores first. */
extern void* g_car_class_vt[24];                /* 0x004dd5e0 */

extern void Sub_421df0(void); /* 0x00421df0 */
extern void Sub_421da0(void); /* 0x00421da0 */
extern void Sub_421d60(void); /* 0x00421d60 */
extern void Sub_421e40(void); /* 0x00421e40 */
extern void Sub_422000(void); /* 0x00422000 */
extern void Sub_4220e0(void); /* 0x004220e0 */
extern void Sub_421a10(void); /* 0x00421a10 */
extern void Sub_4219f0(void); /* 0x004219f0 */
extern void Sub_4219c0(void); /* 0x004219c0 */
extern void Sub_421a40(void); /* 0x00421a40 */
extern void Sub_421a70(void); /* 0x00421a70 */
extern void Sub_421a90(void); /* 0x00421a90 */
extern void Sub_421be0(void); /* 0x00421be0 */
extern void Sub_421b90(void); /* 0x00421b90 */
extern void Sub_421b40(void); /* 0x00421b40 */
extern void Sub_421c30(void); /* 0x00421c30 */
extern void Sub_421c80(void); /* 0x00421c80 */
extern void Sub_421cc0(void); /* 0x00421cc0 */

// FUNCTION: LEGOLAND 0x00422210
void CarClassTablesInit(void)
{
    g_car_class_vt[0]  = (void*)Sub_421df0;
    g_car_class_vt[1]  = (void*)Sub_421da0;
    g_car_class_vt[2]  = (void*)Sub_421d60;
    g_car_class_vt[3]  = (void*)Sub_421da0;
    g_car_class_vt[4]  = (void*)Sub_421e40;
    g_car_class_vt[5]  = (void*)Sub_421da0;
    g_car_class_vt[6]  = (void*)Sub_422000;
    g_car_class_vt[7]  = (void*)Sub_4220e0;
    g_car_class_vt[8]  = (void*)Sub_421a10;
    g_car_class_vt[9]  = (void*)Sub_4219f0;
    g_car_class_vt[10] = (void*)Sub_4219c0;
    g_car_class_vt[11] = (void*)Sub_4219f0;
    g_car_class_vt[12] = (void*)Sub_421a40;
    g_car_class_vt[13] = (void*)Sub_4219f0;
    g_car_class_vt[14] = (void*)Sub_421a70;
    g_car_class_vt[15] = (void*)Sub_421a90;
    g_car_class_vt[16] = (void*)Sub_421be0;
    g_car_class_vt[17] = (void*)Sub_421b90;
    g_car_class_vt[18] = (void*)Sub_421b40;
    g_car_class_vt[19] = (void*)Sub_421b90;
    g_car_class_vt[20] = (void*)Sub_421c30;
    g_car_class_vt[21] = (void*)Sub_421b90;
    g_car_class_vt[22] = (void*)Sub_421c80;
    g_car_class_vt[23] = (void*)Sub_421cc0;
}

/* ---- the 3D view state DrawTrackPiece3D arms every piece ----------------
 * 0x004b5cac and 0x004b5cbc are two stored view records (16 bytes each,
 * {63.0, 0, 0, -1} and {40.0, 0.235, 0.345, -1}); 0x004b5c9c is the ACTIVE
 * one. Selecting a view copies the record whole (four register moves, VC6's
 * 16-byte copy), latches the current angle, and recomputes the two half
 * angles the projection uses: (view + angle)/2 and (view - angle)/2.
 * The sibling at 0x00425c40 does exactly this for the second record. */
typedef struct ViewRec { float a; float b; float c; float d; } ViewRec;

extern ViewRec g_view_cur;                      /* 0x004b5c9c */
extern ViewRec g_view_wide;                     /* 0x004b5cac */
extern float   g_view_angle;                    /* 0x0061164c */
extern float   g_view_angle_cur;                /* 0x00611648 */
extern float   g_view_half_hi;                  /* 0x00829a60 */
extern float   g_view_half_lo;                  /* 0x0082999c */

// FUNCTION: LEGOLAND 0x00425bd0
void SetupTrackDrawView(void)
{
    g_view_half_hi = (g_view_wide.a + g_view_angle) * 0.5f;
    g_view_cur = g_view_wide;
    g_view_angle_cur = g_view_angle;
    g_view_half_lo = (g_view_wide.a - g_view_angle) * 0.5f;
}

/* ---- Castle_Create's remaining initialisers ----------------------------
 * All three end in a tail call, so audit.py can bound them but the shared
 * match.py cannot: they keep a WIP marker. */
extern void Sub_422fe0(void);                   /* 0x00422fe0 */
extern void Sub_41fd30(void);                   /* 0x0041fd30 */
extern void Sub_423d40(void);                   /* 0x00423d40 */
extern void Sub_428f00(void);                   /* 0x00428f00 */
extern void Sub_429270(void);                   /* 0x00429270 */
extern void Sub_4294b0(void);                   /* 0x004294b0 */
extern void Sub_424850(void);                   /* 0x00424850 */
extern void Sub_424890(void);                   /* 0x00424890 */
extern void Sub_424990(void);                   /* 0x00424990 */
extern void* g_castle_hooks[3];                 /* 0x00829bec */

// WIP-FUNCTION: LEGOLAND 0x00423740  (100% by audit.py; match.py cannot bound a tail-jmp function)
void CoasterGeomInit(void)
{
    Sub_422fe0();
    Sub_41fd30();
}

/* Installs the castle's own three-hook table at 0x00829bec (immediately
 * below g_castle_def) and then runs the pass it belongs to. */
// WIP-FUNCTION: LEGOLAND 0x00423db0  (100% by audit.py; match.py cannot bound a tail-jmp function)
void InstallCastleHooks(void)
{
    g_castle_hooks[0] = (void*)Sub_424850;
    g_castle_hooks[1] = (void*)Sub_424890;
    g_castle_hooks[2] = (void*)Sub_424990;
    Sub_423d40();
}

// WIP-FUNCTION: LEGOLAND 0x00428b70  (100% by audit.py; match.py cannot bound a tail-jmp function)
void CoasterSceneInit(void)
{
    Sub_428f00();
    Sub_429270();
    Sub_4294b0();
}

/* This one carries the memory module's UNOPTIMISED codegen (an ebp frame for
 * a body with no locals), like the four wrappers coaster.c keeps at the top
 * of its own `#pragma optimize("", off)` region. */
#pragma optimize("", off)

// FUNCTION: LEGOLAND 0x00477400
int MemScratchInit(void)
{
    return 1;
}

#pragma optimize("", on)

/* ==========================================================================
 * 0x00423140 -- end the coaster's 3D frame.
 *
 * The module keeps a COMMAND BUFFER at 0x004dd870 (write pointer at
 * 0x004b5b3c) and a 640x480 16-bit Z-BUFFER at 0x004e3870 (0x25800 dwords =
 * 614400 bytes; the "Zbuffers\..." strings sit next to the model names in
 * .rdata). Ending a frame either
 *   * CLEARS the whole z-buffer and arms 0x0060f914, when 0x0060f90c says
 *     the frame was abandoned, or
 *   * runs each region's own flush (0x00423480, skipping regions whose clip
 *     code is 0) and then walks the command buffer, handing every command to
 *     0x004232b0. A command is variable length: its first dword is a count
 *     and the next command starts count*8 + 8 bytes on.
 * Either way the write pointer is rewound and the two frame flags cleared.
 *
 * The whole body is timed with RDTSC and the elapsed low dword is left in
 * 0x0060f910, which is why this function has an ebp frame: the two `__asm`
 * blocks force one. The push/pop of eax and edx inside them is the source's
 * own, since RDTSC clobbers both. coaster.c declares this with an `int`
 * argument; it reads none.
 * ======================================================================== */
extern int          g_frame_abandoned;          /* 0x0060f90c */
extern int          g_60f914;                   /* 0x0060f914 */
extern int          g_frame_cmds;               /* 0x0060f908 */
extern unsigned int g_frame_ticks;              /* 0x0060f910 */
extern int*         g_cmd_write;                /* 0x004b5b3c */
extern int          g_cmd_buf[];                /* 0x004dd870 */
extern unsigned int g_zbuffer[];                /* 0x004e3870 */
extern void         Sub_423480(CoasterRegion* r);   /* 0x00423480 */
extern void         Sub_4232b0(int* cmd);           /* 0x004232b0 */

// FUNCTION: LEGOLAND 0x00423140
void Coaster3D_EndFrame(void)
{
    unsigned int t;
    int*         p = g_cmd_buf;

    __asm {
        push eax
        push edx
        rdtsc
        mov  t, eax
        pop  edx
        pop  eax
    }
    if (g_frame_abandoned) {
        memset(g_zbuffer, 0, 0x25800 * 4);
        g_60f914 = 10;
    } else {
        CoasterRegion* r = g_coaster_regions.next;

        while (r != &g_coaster_regions) {
            if (r->code)
                Sub_423480(r);
            r = r->next;
        }
        while (p != g_cmd_write) {
            int* cmd = p;

            p = (int*)((char*)p + *p * 8 + 8);
            Sub_4232b0(cmd);
        }
    }
    g_cmd_write = g_cmd_buf;
    g_frame_abandoned = 0;
    g_frame_cmds = 0;
    __asm {
        push eax
        push edx
        rdtsc
        sub  eax, t
        mov  t, eax
        pop  edx
        pop  eax
    }
    g_frame_ticks = t;
}

/* ==========================================================================
 * 0x0041da10 -- put the whole train back on the track from one position.
 *
 * `at` is where the FRONT of the train goes (RestoreCoasterCar hands it the
 * route's own just-restored position record) and `a` is the state word that
 * goes with it. The route's node ring is then walked from the head, and each
 * car is placed relative to the one in front of it: 0x0041e930 turns the
 * previous car into a direction, 0x00429f30 steps 30.0 units along the track
 * with a 4.8 tolerance from the previous car's own position and spacing, and
 * 0x0041e820 writes the result into this car.
 *
 * THE FRAME (0x34): [dir 12][prev_at 20][next_at 20], and BOTH of the
 * remaining scalars live in dead argument slots -- the previous car's
 * spacing in the `a` slot and 0x00429f30's scalar out-parameter in the `at`
 * slot -- with the head sentinel cached in the `rt` slot.
 * ======================================================================== */
extern void Sub_41e820(RouteNode* n, const RoutePos* at, int a);        /* 0x0041e820 */
extern void Sub_41e930(RouteNode* n, Vec3f* dir);                       /* 0x0041e930 */
extern void Sub_429f30(Vec3f* dir, float step, RoutePos* from, int f40,
                       float tol, RoutePos* out, int* out_a);           /* 0x00429f30 */

/* 65 instructions against the original's 66, the frame (0x34), all three
 * spilled scalars in their dead argument slots, both `rep movsd` copies, the
 * seven-argument call and the whole loop identical. The single missing
 * instruction is a DUPLICATED read of the `a` argument: the original loads
 * [esp+0x3c] TWICE before the prologue pushes -- once into ecx for the
 * `rt->f24` store and once into edx for the call -- where we load it once and
 * use the same register for both. `prev = head` as the initialiser (rather
 * than an assignment after the call) is what buys the original's spill of
 * `head` into the `rt` argument slot; measured and rejected for the second
 * load: a named `int aa = a` copy in four positions, `Sub_41e820(..., rt->f24)`
 * (forwards the stored value, same one load), and the store moved before,
 * between and after the two leading statements (moving it AFTER the call does
 * give 66 instructions, but with a 0x38 frame and 46 mismatches). */
// WIP-FUNCTION: LEGOLAND 0x0041da10  (65 insns vs 66; the original loads the `a` argument twice)
void PositionRouteCars(CoasterRoute* rt, int a, const RoutePos* at)
{
    Vec3f      dir;
    RoutePos   prev_at;
    RoutePos   next_at;
    RouteNode* head = &rt->head;
    RouteNode* n = rt->head.next;
    RouteNode* prev = head;
    int        spacing;
    int        next_a;

    rt->f24 = a;
    rt->pos = *at;
    Sub_41e820(prev, at, a);
    while (n != head) {
        spacing = prev->f40;
        prev_at = prev->at;
        Sub_41e930(prev, &dir);
        Sub_429f30(&dir, 30.0f, &prev_at, spacing, 4.8f, &next_at, &next_a);
        Sub_41e820(n, &next_at, next_a);
        prev = n;
        n = n->next;
    }
}

/* ==========================================================================
 * 0x00420440 -- load the coaster's own data set.
 *
 * Castle_Create's third initialiser. It opens the module's archive
 * ("RollerCoaster\RollerCoaster\CreatedData" under the "ROLLERCOASTER" data
 * group, with "..\..\.." as the path prefix) and then loads three parallel
 * tables from it, each driven by a count-and-name-by-index pair:
 *   * 0x004225d0 / 0x004225b0 -> 0x004d8a40, through 0x00420640
 *   * the same pair again     -> 0x004d8abc, through 0x004206d0, which also
 *                                fills a second table at 0x004d8b34
 *   * 0x00422640 / 0x00422620 -> 0x004d89c8, through 0x00420750
 * The 0x100-byte name buffer is the function's only local.
 * ======================================================================== */
extern void* Sub_4207a0(void);                          /* 0x004207a0 */
extern void  Sub_420530(const char* s);                 /* 0x00420530 */
extern void  Sub_4226c0(const char* s);                 /* 0x004226c0 */
extern int   Sub_4225d0(void);                          /* 0x004225d0 */
extern void  Sub_4225b0(int i, char* name);             /* 0x004225b0 */
extern void* Sub_420640(const char* name);              /* 0x00420640 */
extern void* Sub_4206d0(const char* name, void** out);  /* 0x004206d0 */
extern int   Sub_422640(void);                          /* 0x00422640 */
extern void  Sub_422620(int i, char* name);             /* 0x00422620 */
extern void* Sub_420750(const char* name);              /* 0x00420750 */
extern void* g_4d8bac;                                  /* 0x004d8bac */
extern void* g_coaster_tab_a[];                         /* 0x004d8a40 */
extern void* g_coaster_tab_b[];                         /* 0x004d8abc */
extern void* g_coaster_tab_b2[];                        /* 0x004d8b34 */
extern void* g_coaster_tab_c[];                         /* 0x004d89c8 */

// FUNCTION: LEGOLAND 0x00420440
void LoadCoasterData(void)
{
    char name[0x100];
    int  count;
    int  i;

    g_4d8bac = Sub_4207a0();
    Sub_420530("RollerCoaster\\RollerCoaster\\CreatedData");
    Sub_4226c0("ROLLERCOASTER");
    Sub_420530("..\\..\\..");
    count = Sub_4225d0();
    for (i = 0; i < count; i++) {
        Sub_4225b0(i, name);
        g_coaster_tab_a[i] = Sub_420640(name);
    }
    count = Sub_4225d0();
    for (i = 0; i < count; i++) {
        Sub_4225b0(i, name);
        g_coaster_tab_b[i] = Sub_4206d0(name, &g_coaster_tab_b2[i]);
    }
    count = Sub_422640();
    for (i = 0; i < count; i++) {
        Sub_422620(i, name);
        g_coaster_tab_c[i] = Sub_420750(name);
    }
}

/* ==========================================================================
 * 0x00424e80 -- sample the coaster module's per-frame statistics.
 *
 * Eleven counters are copied into eleven 64-entry ring buffers indexed by
 * `g_610a10 & 0x3f`, six of them are summed into a twelfth ring, and all
 * eleven counters are then zeroed for the next frame. Castle_Reset runs it.
 *
 * The codegen is UNOPTIMISED -- an ebp frame with no locals and
 * `g_610a10 & 0x3f` recomputed at every one of its eighteen uses -- and the
 * padding after it is `int3`, not `nop`. It is the first function of the
 * original's `#pragma optimize("", off)` region, which runs on through
 * Castle_Interact (0x00425050), Castle_Reset (0x00425170, coaster.c) and
 * Castle_Activate (0x004251c0).
 * ======================================================================== */
extern int g_stat_index;                /* 0x00610a10 */

extern int g_stat_c_60f8fc;             /* 0x0060f8fc */
extern int g_stat_c_60f900;             /* 0x0060f900 */
extern int g_stat_c_611644;             /* 0x00611644 */
extern int g_stat_c_615f68;             /* 0x00615f68 */
extern int g_stat_c_4dcbc8;             /* 0x004dcbc8 */
extern int g_stat_c_4d83bc;             /* 0x004d83bc */
extern int g_stat_c_615fc4;             /* 0x00615fc4 */
extern int g_stat_c_615fc8;             /* 0x00615fc8 */
extern int g_stat_c_615fcc;             /* 0x00615fcc */

extern int g_stat_h_60fbf8[64];         /* 0x0060fbf8 */
extern int g_stat_h_60fcf8[64];         /* 0x0060fcf8 */
extern int g_stat_h_60fdf8[64];         /* 0x0060fdf8 */
extern int g_stat_h_60fef8[64];         /* 0x0060fef8 */
extern int g_stat_h_6100f8[64];         /* 0x006100f8 */
extern int g_stat_h_6101f8[64];         /* 0x006101f8 */
extern int g_stat_h_610400[64];         /* 0x00610400 */
extern int g_stat_h_610500[64];         /* 0x00610500 */
extern int g_stat_h_610604[64];         /* 0x00610604 */
extern int g_stat_h_610704[64];         /* 0x00610704 */
extern int g_stat_h_610804[64];         /* 0x00610804 */
extern int g_stat_h_total[64];          /* 0x00610904 */

#pragma optimize("", off)

// FUNCTION: LEGOLAND 0x00424e80
void Coaster3D_SampleStats(void)
{
    g_stat_h_610804[g_stat_index & 0x3f] = g_frame_cmds;
    g_stat_h_610500[g_stat_index & 0x3f] = g_stat_c_60f900;
    g_stat_h_610400[g_stat_index & 0x3f] = g_stat_c_60f8fc;
    g_stat_h_610704[g_stat_index & 0x3f] = g_stat_c_611644;
    g_stat_h_610604[g_stat_index & 0x3f] = g_stat_c_615f68;
    g_stat_h_6100f8[g_stat_index & 0x3f] = g_stat_c_4dcbc8;
    g_stat_h_60fdf8[g_stat_index & 0x3f] = g_stat_c_4d83bc;
    g_stat_h_60fef8[g_stat_index & 0x3f] = g_frame_ticks;
    g_stat_h_60fbf8[g_stat_index & 0x3f] = g_stat_c_615fc4;
    g_stat_h_6101f8[g_stat_index & 0x3f] = g_stat_c_615fc8;
    g_stat_h_60fcf8[g_stat_index & 0x3f] = g_stat_c_615fcc;
    g_stat_h_total[g_stat_index & 0x3f] =
        g_stat_h_610704[g_stat_index & 0x3f] +
        g_stat_h_610400[g_stat_index & 0x3f] +
        g_stat_h_60fdf8[g_stat_index & 0x3f] +
        g_stat_h_610604[g_stat_index & 0x3f] +
        g_stat_h_6100f8[g_stat_index & 0x3f] +
        g_stat_h_60fef8[g_stat_index & 0x3f];
    g_frame_cmds = 0;
    g_stat_c_60f900 = 0;
    g_stat_c_60f8fc = 0;
    g_stat_c_611644 = 0;
    g_stat_c_615f68 = 0;
    g_stat_c_4dcbc8 = 0;
    g_stat_c_4d83bc = 0;
    g_frame_ticks = 0;
    g_stat_c_615fc4 = 0;
    g_stat_c_615fc8 = 0;
    g_stat_c_615fcc = 0;
}

#pragma optimize("", on)

/* ==========================================================================
 * 0x00424c70 -- one tick of the coaster's LOADING BAY.
 *
 * With the circuit CLOSED (castle state 2) the ride boards: while the route
 * still has a free slot (0x0041e2b0) every car queued by 0x00424b30 is
 * attached to one and moved into state 2. Boarding anybody at all arms the
 * 3000-tick departure timer (0x00424ae0); a route that ran out of slots
 * without boarding anybody leaves the timer alone. A route that is closed
 * but has no deadline yet, or a bay that 0x00424c40 says is not ready, does
 * nothing at all.
 *
 * With the circuit OPEN the bay instead ages out its cars: anything sitting
 * in the list for more than 5000 ticks is moved to state 4.
 *
 * The three calls to 0x00424b30 are the original's: a leading readiness
 * guard, then an explicit `if (!c) return;` and a do/while. A plain
 * `while ((c = ...) != 0)` is NOT the same shape -- VC6's rotation sends the
 * entry guard to the code after the loop, where the original sends it to the
 * shared epilogue, so the departure timer would be armed on an empty queue.
 * ======================================================================== */
extern int  g_castle_state;                             /* 0x00829ae0 */
extern int  Sub_424c40(CoasterRec* r);                  /* 0x00424c40 */
extern CoasterCar* Sub_424b30(CoasterRec* r);           /* 0x00424b30 */
extern void Sub_424ae0(CoasterRec* r, int ticks);       /* 0x00424ae0 */

/* Unfinished: 83 of the original's 89 instructions, 256 vs 266 bytes,
 * 89 mismatches. The lane writing this was stopped mid-function at a
 * session checkpoint, so the body is a partial draft rather than a
 * diagnosed near-miss — re-derive it from the disassembly rather than
 * trusting the shape below. */
// WIP-FUNCTION: LEGOLAND 0x00424c70  (83/89 insns, 89 mismatches; interrupted draft)
void Coaster_TickLoadingBay(CoasterRec* r)
{
    int now = GetGameTimer();

    if (g_castle_state == 2) {
        CoasterCar* c;
        void*       slot;
        int         boarded;

        if (Route_IsClosed(r->route) && !Route_HasDeadline(r->route))
            return;
        if (!Sub_424c40(r))
            return;
        if (!Sub_424b30(r))
            return;
        boarded = 0;
        c = Sub_424b30(r);
        if (!c)
            return;
        do {
            slot = Sub_41e2b0(r->route);
            if (!slot) {
                if (boarded)
                    Sub_424ae0(r, 3000);
                return;
            }
            Sub_421590(c, slot);
            c->state = 2;
            boarded = 1;
            c = Sub_424b30(r);
        } while (c);
        Sub_424ae0(r, 3000);
    } else {
        CoasterCar* c = r->cars.next;
        CoasterCar* head = &r->cars;

        while (c != head) {
            if (now - c->born > 5000)
                c->state = 4;
            c = c->next;
        }
    }
}
