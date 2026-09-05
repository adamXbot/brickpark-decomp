/* LEGOLAND -- more of the DRIVING SCHOOL, and the coaster internals that sit
 * in the same address range.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere) and follow schoolcar.c / schoolcar2.c / schoolcar3.c.
 *
 * =========================================================================
 * PART ONE -- the two callees goldrush.c's StepSchoolCar reaches for
 * =========================================================================
 * schoolcar.c established that a school car drives a QUEUE of waypoints:
 *
 *     +0x30  Waypoint wp[16];   16 * {int x; int y;} in 16.16 world units
 *     +0xbb  unsigned char nwp; how many of them are live
 *
 * The four manoeuvre routines PUSH runs of waypoints onto that queue and
 * SchoolCarAccelerate steers at wp[0]. The two functions here complete the
 * picture:
 *
 *   0x00401cd0  SchoolCarIdleStep   -- the queue POP.
 *   0x00402490  SchoolCarBlockedAhead -- the "somebody is in my way" test.
 *
 * `SchoolCarBlockedAhead` is a two-stage proximity test in the car's own
 * 16.16 world space, both stages worked in 24.8 (`>> 8`) so the squares fit
 * an int:
 *
 *   stage 1  |p - c|^2 <= 0x40000       (0x200 in 24.8 = 2.0 world units)
 *   stage 2  |p - (c + u)|^2 <= 0x10000 (0x100 in 24.8 = 1.0 world unit)
 *
 * where `u` is the cached unit heading (+0xb0/+0xb4, set by
 * SchoolCarAccelerate as the normalised vector to wp[0]) scaled by one whole
 * world unit. So: a car is "blocked" by another car that is within two units
 * of it AND within one unit of the square metre it is about to drive into.
 * The scale factor is spelled -65536.0f and SUBTRACTED, which is how the
 * original moves the probe point FORWARD along the heading.
 *
 * Note the walk does not stop at the first hit -- it keeps going and returns
 * the LAST car in g_school_cars that passes both stages.
 * ========================================================================= */

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
 * 0x00401cd0 -- pop the front waypoint.
 *
 * StepSchoolCar calls this for a car that did nothing else this frame, which
 * is why goldrush.c's header calls it the "idle step"; what it actually does
 * is shift wp[1..] down over wp[0] and drop the count.
 *
 * ORIGINAL BUG, reproduced: the shift runs sixteen times, so the last copy
 * reads wp[16] -- one element PAST the array, i.e. the raw bits of the
 * cached unit heading at +0xb0/+0xb4 -- and drops them into wp[15]. It is
 * harmless because `nwp` is decremented in the same breath and never
 * exceeds 16, so the poisoned slot is always outside the live range; but it
 * is the original's code and the sixteenth copy is a third of the function.
 *
 * The sixteen copies are written out as sixteen SEPARATE 8-byte struct
 * assignments. That is not cosmetic: a `for (i = 0; i < 16; i++)` loop
 * compiles to 19 instructions (VC6 SP3 does not unroll) and an intrinsic
 * `memcpy` of the same 128 bytes to 15 (`rep movsd`); only the sixteen
 * statements give the original's 71, each one lowering to the load/store
 * pair `mov edx,[eax+src] / mov [eax+dst],edx`.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x00401cd0
void SchoolCarIdleStep(SchoolCar* c)
{
    if (c->nwp) {
        c->wp[0]  = c->wp[1];
        c->wp[1]  = c->wp[2];
        c->wp[2]  = c->wp[3];
        c->wp[3]  = c->wp[4];
        c->wp[4]  = c->wp[5];
        c->wp[5]  = c->wp[6];
        c->wp[6]  = c->wp[7];
        c->wp[7]  = c->wp[8];
        c->wp[8]  = c->wp[9];
        c->wp[9]  = c->wp[10];
        c->wp[10] = c->wp[11];
        c->wp[11] = c->wp[12];
        c->wp[12] = c->wp[13];
        c->wp[13] = c->wp[14];
        c->wp[14] = c->wp[15];
        c->wp[15] = c->wp[16];      /* one past the end -- the original's bug */
        c->nwp--;
    }
}

/* ==========================================================================
 * 0x00402490 -- is another car occupying the square this one is driving into?
 *
 * StepSchoolCar stalls the car (halving its speed, counting the stall, and
 * giving up after 0x200 frames) while this returns non-zero. The test is the
 * two-stage proximity check described at the top of the file, and it does NOT
 * stop at the first hit: every car in g_school_cars is examined and the LAST
 * one that passes both stages is returned.
 *
 * goldrush.c declares this `int SchoolCarBlockedAhead(SchoolCar*)` because
 * that is all StepSchoolCar's `test eax,eax` needs; the body returns a
 * SchoolCar*, which is what the store of `ebp` into the result slot says.
 * The divergence is deliberate -- an extern's type is a caller-side lever.
 *
 * WIP RESIDUAL (measured this round, 62/63 instructions, 178/180 bytes,
 * strict 27, register-blind 22, offset-blind 22, first divergence at index
 * 36).  Everything up to and including index 35 is exact: the split
 * prologue (`push ecx` for the result home, `push ebp` for the walk pointer,
 * the other three sunk into the non-empty-list path), the whole of stage
 * one, and the first `fld/fmul/__ftol` pair.
 *
 * What is left is ONE scheduling permutation of stage two plus the single
 * `mov eax,edi` register copy it implies (which is the whole 63rd
 * instruction).  The original interleaves:
 *      sub esi,eax | call __ftol | mov edx,[p+0x10] | sub edi,eax |
 *      mov eax,edi | mov edi,[p+0x14] | sub eax,edi | sub esi,edx
 * i.e. the ux chain runs FIRST but the y difference is subtracted first, and
 * p->wx's load sits after the second __ftol.  VC6 offers exactly TWO
 * schedules here and neither is that one:
 *   * source x-first  -> the p->wx load and the whole x difference are
 *     scheduled into the fmul/call latency gap, and the second __ftol sinks
 *     past them (strict 27, first divergence 36) -- this build;
 *   * source y-first  -> the whole uy chain moves ahead of the ux chain
 *     (strict 30, first divergence 30).
 * ALL 40 legal interleavings of the six statements {ax, dx, dx>>=8} x
 * {ay, dy, dy>>=8} produce one of those two, as do: naming the __ftol
 * results, naming p->wx/p->wy, naming one or both of c->wx/c->wy, an inlined
 * `Near(p,x,y,lim)` helper with either argument order, `&&` instead of
 * nesting, the reassociated `(c->wx - p->wx - fx)` spelling, duplicated
 * square expressions, a `sq()` helper, and a CarPos aggregate for the ahead
 * point; so are the in-place `dx -= p->wx` spelling and an explicit
 * accumulator (`d2 = dy*dy; d2 += dx*dx;`) in either order.
 * The one construct that DOES buy the original's instruction count
 * and drops register-blind to 11 is a free `volatile` read on BOTH stage-two
 * loads (`ax - *(volatile int*)&p->wx`), which pins them after the call --
 * but each volatile read is another reference to `p`, and two of them lift
 * `p` above `c` in the callee-saved ranking, so `p` takes edi and `c` ebp
 * instead of the original's ebp/ebx, and the prologue and every base register
 * change (strict 29).  One volatile read alone flips the same pair.  So the
 * residual is "the original's schedule needs a barrier VC6 only offers with a
 * reference that re-ranks the register the barrier is on".
 * ======================================================================== */
// WIP-FUNCTION: LEGOLAND 0x00402490  (62/63 insns, 178/180 B, strict 27; exact through index 35, stage-two schedule permuted)
SchoolCar* SchoolCarBlockedAhead(SchoolCar* c)
{
    SchoolCar* p = g_school_cars;
    SchoolCar* hit = 0;
    int        dx, dy, ax, ay;

    while (p) {
        if (p != c) {
            dx = (c->wx - p->wx) >> 8;
            dy = (c->wy - p->wy) >> 8;
            if (dy * dy + dx * dx <= 0x40000) {
                /* one whole world unit along the cached unit heading */
                ax = c->wx - (int)(c->ux * -65536.0f);
                ay = c->wy - (int)(c->uy * -65536.0f);
                dx = (ax - p->wx) >> 8;
                dy = (ay - p->wy) >> 8;
                if (dy * dy + dx * dx <= 0x10000)
                    hit = p;
            }
        }
        p = p->next;
    }
    return hit;
}

/* =========================================================================
 * PART TWO -- the coaster internals in the same address range.
 *
 * schoolcar.c reaches all of these through externs and calls them Sub_*;
 * the names below are recovered from what the bodies do.
 * ========================================================================= */

#include <math.h>
#include <string.h>

#pragma intrinsic(strcpy, fabs)

/* ==========================================================================
 * 0x004226c0 -- load the coaster's MODEL SET: "<name>.obj" and "<name>.txt".
 *
 * LoadCoasterData (schoolcar.c, 0x00420440) calls this once with
 * "ROLLERCOASTER" after pointing the archive layer at the module's
 * CreatedData directory. It reads the two files whole into memory through
 * 0x00422470 (CreateFileA / GetFileSize / allocate / ReadFile, returning the
 * image and writing the byte length into its out-parameter) and then counts
 * the records in each with 0x004223c0, which walks the image two bytes at a
 * time over a per-byte predicate. Both counts and both images are kept in
 * globals, together with the base name, which is why the seven globals
 * 0x004dd758..0x004dd86c are one object in all but the declaration:
 *
 *   0x004dd758 / 0x004dd75c   the ".txt" image and its byte length
 *   0x004dd760 [0x100]        the base name ("ROLLERCOASTER")
 *   0x004dd860 / 0x004dd864   the ".obj" image and its byte length
 *   0x004dd868 / 0x004dd86c   the record counts of each
 *
 * The failure paths are asymmetric and that asymmetry is the codegen: the
 * FIRST `return 0` is on the value just tested in eax so it needs no `xor`,
 * while the second follows the Free_w call and does. The name is copied with
 * the intrinsic `strcpy` (repne scasb + rep movsd/movsb) and the `mov eax,1`
 * of the success return is scheduled into the middle of that expansion.
 *
 * The format strings are built with USER32's wsprintfA, and because it is
 * called twice VC6 hoists the import thunk into esi rather than emitting two
 * `call dword ptr [__imp__wsprintfA]`.
 *
 * schoolcar.c declares this `void LoadCoasterModelSet(const char*)` -- the caller
 * discards the result. The body returns int; the divergence is deliberate.
 * ======================================================================== */
extern int __declspec(dllimport) __cdecl wsprintfA(char* buf, const char* fmt, ...);

/* 0x00422470: read a whole file into a fresh block; *len gets its size. */
extern void* LoadWholeFile(const char* name, void* len);       /* 0x00422470 */
/* 0x004223c0: count the records in a {image, length} pair. */
extern int   CountModelRecords(void* img);                     /* 0x004223c0 */
extern void  Free_w(void* p);                                  /* 0x004775d0 */

extern void* g_cc_txt;             /* 0x004dd758 */
extern int   g_cc_txt_len;         /* 0x004dd75c */
extern char  g_cc_name[0x100];     /* 0x004dd760 */
extern void* g_cc_obj;             /* 0x004dd860 */
extern int   g_cc_obj_len;         /* 0x004dd864 */
extern int   g_cc_obj_count;       /* 0x004dd868 */
extern int   g_cc_txt_count;       /* 0x004dd86c */

// FUNCTION: LEGOLAND 0x004226c0
int LoadCoasterModelSet(const char* name)
{
    char buf[0x100];

    wsprintfA(buf, "%s.obj", name);
    g_cc_obj = LoadWholeFile(buf, &g_cc_obj_len);
    if (!g_cc_obj)
        return 0;
    wsprintfA(buf, "%s.txt", name);
    g_cc_txt = LoadWholeFile(buf, &g_cc_txt_len);
    if (!g_cc_txt) {
        Free_w(g_cc_obj);
        return 0;
    }
    g_cc_obj_count = CountModelRecords(&g_cc_obj);
    g_cc_txt_count = CountModelRecords(&g_cc_txt);
    strcpy(g_cc_name, name);
    return 1;
}

/* ==========================================================================
 * 0x0041e130 -- run the train along the track for one frame's worth of real
 * time.  Coaster_TickRoute (schoolcar.c) calls it only for a CLOSED circuit.
 *
 * The route is integrated in SUB-STEPS, not in one jump.  The frame's
 * elapsed time is `(now - rt->started) * 0.001` seconds, clamped to 0.8s so a
 * stalled frame cannot fling the train down the track, and the loop keeps
 * asking one of three per-state steppers for "how much of the remaining time
 * did you actually consume" until the accumulated time reaches dt - 1e-6:
 *
 *     state & 0x08  ->  0x0041e0e0     (the LIFT/launch stepper)
 *     state & 0x10  ->  0x0041e100
 *     otherwise     ->  0x0041e000     (free running)
 *
 * Each returns a float, and `acc += stepper(rt, dt - acc)` is the whole
 * integration.  Between sub-steps 0x0041f850 walks the position record to the
 * next track piece and rt->f24 is refreshed from the new piece's +0x44, and
 * when the piece actually CHANGED the route's state machine advances:
 *
 *     8 -> 4                                    on any piece boundary
 *     4 -> 0x10   only when the new piece is the coaster's ring SENTINEL
 *                 (`&rt->owner->ring`, i.e. the train has come all the way
 *                 round), which is the lap counter.
 *
 * The whole body is bracketed by RDTSC and the elapsed cycles are added into
 * the module's counter 0x004d83bc (the one Coaster3D_SampleStats copies into
 * the 0x0060fdf8 ring).  The two `__asm` blocks are what force the ebp frame.
 *
 * TWO LEVERS, both measured here:
 *  * `float acc = 0.0f;` as an INITIALISER puts `mov [slot],0` at the very
 *    top of the IR -- before the GetGameTimer call -- and gives `acc` a real
 *    frame slot.  Written as a STATEMENT *after* `owner = rt->owner;` it
 *    lands at the original's index 11 AND moves `acc` into the dead `rt`
 *    parameter slot, which is what frees [ebp-4] for `dt`.  Placed before
 *    `owner` (16 mismatches) or before the timer read (16) it does neither.
 *    So: which of two contending floats takes the dead argument slot is
 *    decided by whether one of them is initialised or assigned, and where.
 *  * `dt = ((float)now - rt->started) * 0.001f;` gives `fild/fisub` -- the
 *    subtraction is done in FLOAT with an integer memory operand; an int
 *    subtraction `(float)(now - rt->started)` would emit `sub` + one `fild`.
 * ======================================================================== */
typedef struct TrackNode    TrackNode;
typedef struct CoasterRec   CoasterRec;
typedef struct CoasterRoute CoasterRoute;

typedef struct Vec3f { float x; float y; float z; } Vec3f;

/* What rt->pos.obj points at; only its +0x44 float is touched here. */
typedef struct RouteObj { unsigned char pad00[0x44]; float f44; } RouteObj;

/* The route's POSITION record (schoolcar.c's RoutePos, with `obj` typed). */
typedef struct RoutePos {
    TrackNode* node;            /* +0x00  the live track piece */
    RouteObj*  obj;             /* +0x04 */
    Vec3f      pos;             /* +0x08 */
} RoutePos;                     /* 0x14 */

struct TrackNode { unsigned char pad00[0x50]; };

struct CoasterRec {
    int       state;            /* +0x00 */
    TrackNode ring;             /* +0x04  the list SENTINEL */
};

struct CoasterRoute {
    int           state;        /* +0x00  bit 2 = running, bit 3 = lift,
                                 *        bit 4 = past the lap mark */
    int           started;      /* +0x04  game-clock stamp of the last step */
    int           deadline;     /* +0x08 */
    RoutePos      pos;          /* +0x0c */
    unsigned char pad20[4];
    float         f24;          /* +0x24  refreshed from the live piece */
    float         speed;        /* +0x28 */
    unsigned char pad2c[0x6c - 0x2c];
    CoasterRec*   owner;        /* +0x6c */
};                              /* 0x15c */

extern int   GetGameTimer(void);                                /* 0x00499430 */
extern float Route_StepPrimary(CoasterRoute* rt, float d);             /* 0x0041e0e0 */
extern float Route_StepSecondary(CoasterRoute* rt, float d);             /* 0x0041e100 */
extern float Route_StepFree(CoasterRoute* rt, float d);             /* 0x0041e000 */
extern void  TrackCursor_AdvanceGeometry(RoutePos* p);                           /* 0x0041f850 */
extern int   g_stat_c_4d83bc;                                   /* 0x004d83bc */

// FUNCTION: LEGOLAND 0x0041e130
void Route_AdvanceTrain(CoasterRoute* rt)
{
    unsigned int t;
    float        acc;
    float        dt, limit;
    int          now, st;
    CoasterRec*  owner;
    TrackNode*   node;

    now = GetGameTimer();
    owner = rt->owner;
    acc = 0.0f;
    dt = ((float)now - rt->started) * 0.001f;
    if (dt > 0.8f)
        dt = 0.8f;
    __asm {
        push eax
        push edx
        rdtsc
        mov  t, eax
        pop  edx
        pop  eax
    }
    limit = dt - 1e-6f;
    for (;;) {
        st = rt->state;
        node = rt->pos.node;
        if (st & 8)
            acc += Route_StepPrimary(rt, dt - acc);
        else if (st & 0x10)
            acc += Route_StepSecondary(rt, dt - acc);
        else
            acc += Route_StepFree(rt, dt - acc);
        if (!(acc < limit))
            break;
        TrackCursor_AdvanceGeometry(&rt->pos);
        rt->f24 = rt->pos.obj->f44;
        if (node != rt->pos.node) {
            st = rt->state;
            if (st & 8) {
                rt->state = (st & ~8) | 4;
            } else if (st & 4) {
                if (rt->pos.node == &owner->ring)
                    rt->state = (st & ~4) | 0x10;
            }
        }
    }
    rt->started = now;
    __asm {
        push eax
        push edx
        rdtsc
        sub  eax, t
        mov  t, eax
        pop  edx
        pop  eax
    }
    g_stat_c_4d83bc += t;
}

/* ==========================================================================
 * 0x0041e820 -- put ONE car of the train at a position on the track.
 *
 * PositionRouteCars (schoolcar.c, 0x0041da10) calls this for the head car
 * with the route's own position, and then once per car with the position
 * 0x00429f30 found 30 units behind the car in front.
 *
 * A RouteNode carries TWO track cursors, `{ float t; RoutePos at; }` at +0x08
 * and +0x40 -- the car's FRONT and REAR bogies. This routine seats the front
 * one at (`at`, `a`), asks 0x0042a640 for its tangent (mode 2), steps 30.0
 * units back along the track from it with a 4.8 tolerance to find where the
 * rear bogie sits, seats that, and takes its tangent too. The car's
 * ORIENTATION is then the mean of the two tangents:
 *
 *     n->dx/dy/dz = (front + rear) * 0.5
 *     n->c4       = 0x0041e7e0(n) * n->dz * -0.001349375
 *
 * -- i.e. the car points along the chord between its bogies, and +0xc4 is a
 * pitch/bank term scaled by the vertical component. This is why the RouteNode
 * field schoolcar.c calls `f40` is the "spacing parameter" it hands to
 * 0x00429f30: it is the rear cursor's own track parameter.
 *
 * FRAME: 0x2c of aggregates -- the front tangent lowest, the rear tangent
 * above it, the 20-byte RoutePos at the top -- and 0x00429f30's scalar
 * out-parameter homed in the dead `n` argument slot, exactly as in
 * PositionRouteCars. `add esp,0x40` merges the first four calls' clean-ups.
 *
 * LEVER: `(rear + front) * 0.5f` is NOT what the original loads. VC6 emits
 * `fld` on the operand written SECOND and `fadd` on the first, so the source
 * order is front-then-rear at all three components. Same family as "adjacent
 * address stores come out REVERSED"; worth all six mismatches here.
 * ======================================================================== */
typedef struct RouteNode RouteNode;

/* One bogie: where it is on the track, plus the track parameter there. */
typedef struct TrackCursor {
    float    t;                 /* +0x00  the parameter 0x00429f30 steps in */
    RoutePos at;                /* +0x04 */
} TrackCursor;                  /* 0x18 */

struct RouteNode {
    unsigned char pad00[8];
    TrackCursor   front;        /* +0x08 */
    unsigned char pad20[0x40 - 0x20];
    TrackCursor   rear;         /* +0x40  (schoolcar.c's `f40` is rear.t) */
    unsigned char pad58[0xb8 - 0x58];
    float         dx;           /* +0xb8  the car's heading */
    float         dy;           /* +0xbc */
    float         dz;           /* +0xc0 */
    float         c4;           /* +0xc4  pitch/bank term */
};

extern void  Sub_42a620(TrackCursor* c, const RoutePos* at, float a);   /* 0x0042a620 */
extern void  Sub_42a640(TrackCursor* c, int mode, Vec3f* out);          /* 0x0042a640 */
extern void  Sub_429f30(Vec3f* dir, float step, RoutePos* from, float f40,
                        float tol, RoutePos* out, float* out_a);        /* 0x00429f30 */
extern float Sub_41e7e0(RouteNode* n);                                  /* 0x0041e7e0 */

// FUNCTION: LEGOLAND 0x0041e820
void RouteCar_SetPosition(RouteNode* n, const RoutePos* at, float a)
{
    Vec3f    d1, d2;
    RoutePos rear;
    float    rear_a;

    Sub_42a620(&n->front, at, a);
    Sub_42a640(&n->front, 2, &d1);
    Sub_429f30(&d1, 30.0f, &n->front.at, n->front.t, 4.8f, &rear, &rear_a);
    Sub_42a620(&n->rear, &rear, rear_a);
    Sub_42a640(&n->rear, 2, &d2);
    n->dx = (d1.x + d2.x) * 0.5f;
    n->dy = (d1.y + d2.y) * 0.5f;
    n->dz = (d1.z + d2.z) * 0.5f;
    n->c4 = Sub_41e7e0(n) * n->dz * -0.001349375f;
}

/* ==========================================================================
 * THE TRACK CURVE.  Slots 6 and 7 of the FIRST of the three eight-entry car
 * class tables schoolcar.c's CarClassTablesInit fills (0x004dd5e0), and the
 * four globals immediately after that table are their shared scratch:
 *
 *   0x004dd644 / 0x004dd648   the curve the collector is working on
 *   0x004dd64c                the collector's WRITE POINTER
 *   0x004dd650                how many values it has written
 *
 * A track curve's vertical profile is a CUBIC in the segment parameter:
 *
 *     z(u) = ((u*c3 + c2)*u + c1)*u + c0     with c3..c0 at +0x24..+0x30
 *
 * and its horizontal direction is the constant unit vector at +0x0c. The
 * segment spans parameters t0 (+0x44) to t1 (+0x48).
 * ======================================================================== */
typedef struct TrackGeom {
    unsigned char pad00[0x0c];
    Vec3f         dir;          /* +0x0c  constant horizontal direction */
    unsigned char pad18[0x24 - 0x18];
    float         c3;           /* +0x24  the height cubic, high order first */
    float         c2;           /* +0x28 */
    float         c1;           /* +0x2c */
    float         c0;           /* +0x30 */
    unsigned char pad34[0x44 - 0x34];
    float         t0;           /* +0x44  the segment's parameter range */
    float         t1;           /* +0x48 */
} TrackGeom;

extern TrackGeom* g_tc_curve_a;         /* 0x004dd644 */
extern TrackGeom* g_tc_curve_b;         /* 0x004dd648 */
extern float*     g_tc_wp;              /* 0x004dd64c */
extern int        g_tc_n;               /* 0x004dd650 */

extern void TrackCurve_Refine(float a, float pa, float b, float pb);   /* 0x00421e90 */
extern void Vec3_Normalize(Vec3f* v);                               /* 0x00425d50 */

/* ==========================================================================
 * 0x00422000 -- gather and SORT the curve's interesting parameter values.
 *
 * The two segment ends go into the caller's array first, then 0x00421e90 is
 * handed each end together with its height `((u*c3 + c2)*u + c1)*u + c0` and
 * appends whatever extra parameters it finds through the same write pointer
 * (which is why the count starts at 2 and is re-read afterwards). The array
 * is then bubble-sorted ascending.
 *
 * The four arguments to 0x00421e90 evaluate RIGHT to LEFT, which is why the
 * t1 polynomial is emitted first; `g->t1` is spilled into the dead `g`
 * argument slot because it is held in ecx as an integer bit pattern for the
 * push and x87 cannot read a GPR.  The two appends are `*g_tc_wp = v;
 * g_tc_wp++;` -- the write pointer is RE-READ after every store because the
 * store may alias the pointer itself.
 *
 * WIP RESIDUAL: 69/70 instructions, 206/211 bytes, strict 6, register-blind
 * 5, and the first 64 instructions are exact.  The single missing
 * instruction is a DEAD `mov eax,[0x004dd650]` at the inner loop's exit --
 * VC6 reloading the count into the register it had hoisted it into, for a
 * use that does not exist -- and the five that follow are only shifted by it.
 * The reload only appears if `g_tc_n` is live across the inner loop, and no
 * spelling of the sort makes it so: both loop directions, subscripts versus a
 * pointer walk, `k > 0` versus `k < i`, a `pass` counter with the inner bound
 * spelled `g_tc_n - 1 - pass` (74 instructions), a mirrored local re-read at
 * the top or the bottom of the outer body, `for (p = out; p < out + i; p++)`,
 * both swap orders and a free `volatile` read of the count were all measured;
 * the closest alternatives are strict 8 and 10.  The `jle` guard (signed, not
 * `je`) pins the inner loop to `for (k = 0; k < i; k++)`, and the swap's store
 * order -- `out[k] = out[k+1]` before `out[k+1] = t` -- only comes out right
 * with the SUBSCRIPT form; through a walking pointer VC6 reverses the pair.
 * ======================================================================== */
// WIP-FUNCTION: LEGOLAND 0x00422000  (69/70 insns, 206/211 B, strict 6; exact through index 63, one dead count reload missing)
void TrackCurve_GatherParams(TrackGeom* g, float* out)
{
    int i, k;

    g_tc_curve_a = g;
    g_tc_curve_b = g;
    g_tc_n = 2;
    g_tc_wp = out;
    *g_tc_wp = g->t0;
    g_tc_wp++;
    *g_tc_wp = g->t1;
    g_tc_wp++;
    TrackCurve_Refine(g->t0, ((g->t0 * g->c3 + g->c2) * g->t0 + g->c1) * g->t0 + g->c0,
               g->t1, ((g->t1 * g->c3 + g->c2) * g->t1 + g->c1) * g->t1 + g->c0);
    for (i = g_tc_n - 1; i >= 0; i--) {
        for (k = 0; k < i; k++) {
            if (out[k] > out[k + 1]) {
                float t = out[k];
                out[k] = out[k + 1];
                out[k + 1] = t;
            }
        }
    }
}

/* ==========================================================================
 * 0x004220e0 -- the curve's unit NORMAL at parameter t.
 *
 * The tangent is the segment's constant horizontal direction with the height
 * cubic's DERIVATIVE as its z:
 *
 *     out = g->dir;  out.z = (t*c3*3 + (c2 + c2))*t + c1
 *
 * 0x00425d50 normalises it, and the tail then rotates it a quarter turn to
 * get a perpendicular. Which plane it rotates in depends on the tangent: if
 * |y| is negligible (< 0.001) it turns in the x/z plane, otherwise in y/z,
 * and the direction of the turn is picked from the SIGN BIT of the component
 * being replaced, tested as an integer (`test dword ptr [mem],0x80000000`),
 * not with an x87 compare. In both planes the rule is (a, z) -> (z, -a) when
 * a is negative and (-z, a) when it is not.
 *
 * TWO LEVERS, both worth the whole tail:
 *  * `s = out->z;` written BEFORE the `if` schedules its `fld` ahead of the
 *    `fabs`, four instructions early (32 mismatches). Written as the first
 *    statement of BOTH arms, VC6 head-merges the two loads into the single
 *    `fld [esi+8]` it schedules into the fcomp/fnstsw gap -- the original.
 *  * the "not negative" arm must be written with NO temporary at all:
 *    `out->z = out->x; out->x = -s;`. A `float` temp gets a stack home and
 *    costs a spill plus a register copy; an `int`-bits temp CSEs with the
 *    sign test's memory operand and degrades `test dword ptr [esi],K` into
 *    `mov eax,[esi]` + `test eax,K`. Reversing the two stores gets the value
 *    right and the order wrong, so the two-statement form is the only one.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x004220e0
void TrackCurve_NormalAt(TrackGeom* g, float t, Vec3f* out)
{
    float s;

    *out = g->dir;
    out->z = (t * g->c3 * 3.0f + (g->c2 + g->c2)) * t + g->c1;
    Vec3_Normalize(out);
    if (fabs(out->y) < 0.001) {
        s = out->z;
        if (*(unsigned*)&out->x & 0x80000000) {
            out->z = -out->x;
            out->x = s;
        } else {
            out->z = out->x;
            out->x = -s;
        }
    } else {
        s = out->z;
        if (*(unsigned*)&out->y & 0x80000000) {
            out->z = -out->y;
            out->y = s;
        } else {
            out->z = out->y;
            out->y = -s;
        }
    }
}
