/* LEGOLAND -- the coaster's PHYSICS SOLVER and its SOFTWARE RASTERISER
 * helpers: the state-vector combiner, the RK4 integrator step, the
 * bisection that lands the train exactly on a piece boundary, the z-buffer
 * span filler and the shade-ramp builder.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere) and follow schoolcar.c / schoolcar3.c / schoolcar4.c /
 * schoolcar5.c.
 *
 * =========================================================================
 * THE PHYSICS INTERFACE  (the central discovery of this lane)
 * =========================================================================
 * The route's physics object -- the 0x40 bytes at CoasterRoute+0x2c that
 * schoolcar5.c could only describe as "0x00420310 calls it through function
 * pointers at its own +0x04/+0x24/+0x2c" -- is a solver descriptor with two
 * interfaces bolted together:
 *
 *     +0x00  setstate(p, v)          the SYSTEM: load a state vector back
 *     +0x04  getstate(p, v)          read the live state out
 *     +0x08  deriv(p, t, out)        evaluate the derivative at time t
 *     +0x0c .. +0x30                 the ten VECTOR OPS, slot for slot the
 *                                    table schoolcar.c's CarPoolInit fills
 *                                    at 0x004dcbd0:
 *          +0x0c [0] 0x004212a0      +0x10 [1] 0x004212e0
 *          +0x14 [2] 0x00421320      +0x18 [3] 0x00421340  v *= k
 *          +0x1c [4] 0x00421360      +0x20 [5] 0x004213a0  out = ka*a + kb*b
 *          +0x24 [6] 0x00421400      +0x28 [7] 0x00421430  out += k*a
 *                    zero(v, n)      +0x2c [8] CarPool_Alloc(n)
 *          +0x30 [9] CarPool_Free(a, n)
 *     +0x34  dim                     the state vector's length
 *
 * A STATE VECTOR is `{ int n; float v[n]; }` and the pool hands out RUNS of
 * consecutive 0x54-byte slots, which is why Phys_Step asks for four at once
 * and indexes the returned pointer table. So the coaster's motion is a
 * generic ODE integrator over a car-shaped state vector, and the track only
 * enters through the system's three hooks.
 * ======================================================================== */

typedef struct PhysVec {
    int   n;                    /* +0x00 */
    float v[1];                 /* +0x04 */
} PhysVec;

typedef struct PhysObj PhysObj;

struct PhysObj {
    void      (*setstate)(PhysObj* p, PhysVec* v);              /* +0x00 */
    void      (*getstate)(PhysObj* p, PhysVec* v);              /* +0x04 */
    void      (*deriv)(PhysObj* p, float t, PhysVec* out);      /* +0x08 */
    void*     op0;                                              /* +0x0c */
    void*     op1;                                              /* +0x10 */
    void*     op2;                                              /* +0x14 */
    void      (*scale)(PhysVec* v, float k);                    /* +0x18 */
    void*     op4;                                              /* +0x1c */
    void      (*scaleadd2)(const PhysVec* a, float ka,
                           const PhysVec* b, float kb,
                           PhysVec* out);                       /* +0x20 */
    void      (*zero)(PhysVec* v, int n);                       /* +0x24 */
    void      (*addscaled)(const PhysVec* a, float k,
                           PhysVec* out);                       /* +0x28 */
    PhysVec** (*alloc)(int n);                                  /* +0x2c */
    void      (*release)(PhysVec** a, int n);                   /* +0x30 */
    int       dim;                                              /* +0x34 */
    int       f38;                                              /* +0x38 */
};

/* 0x00420310's scratch, as schoolcar5.c's Route_StepFree already described
 * it: +0x00 is the integrator's running TIME and +0x04 a context word the
 * caller refreshes from the route. */
typedef struct PhysStepCtx {
    float         t;            /* +0x00 */
    int           f04;          /* +0x04 */
    unsigned char pad08[8];
} PhysStepCtx;                  /* 0x10 */

/* ==========================================================================
 * 0x004213a0 -- slot 5 of the vector-op table: out = ka*a + kb*b.
 *
 * The family it belongs to reads off the four neighbours: [3] 0x00421340 is
 * `v *= k`, [4] 0x00421360 is `max |v[i]|`, [6] 0x00421400 is
 * `zero(v, n)` and [7] 0x00421430 is `out += k*a`. This one is the two-term
 * combination the integrator below uses to build each stage's probe state.
 *
 * The length is `a->n` and it is RE-READ every iteration, because the stores
 * into `out->v[]` may alias it -- and `out->n = a->n` is written TWICE, once
 * before the loop and once after, which is why the empty-vector path has its
 * own tail-duplicated copy of the store.
 *
 * LEVER: the three cursors must be spelled the SAME way (plain subscripts).
 * VC6 then eliminates two of the three induction variables, keeping ONE
 * walking pointer over `b` and two constant differences -- `esi = a - b`,
 * `edi = out - b`, then `[esi+eax]`, `[eax]`, `[edi+eax-4]`. That is the
 * recorded "two lockstep cursors spelled the same way let VC6 eliminate an
 * induction variable" rule with three of them.
 * ======================================================================== */

// FUNCTION: LEGOLAND 0x004213a0
void PhysVec_ScaleAdd2(const PhysVec* a, float ka, const PhysVec* b, float kb,
                       PhysVec* out)
{
    int i;

    out->n = a->n;
    for (i = 0; i < a->n; i++)
        out->v[i] = ka * a->v[i] + kb * b->v[i];
    out->n = a->n;
}

/* ==========================================================================
 * 0x00420310 -- one CLASSICAL FOURTH-ORDER RUNGE-KUTTA step.
 *
 * schoolcar5.c's Route_StepFree drives this once per sub-step at seventy
 * sub-steps per second, and Route_StepToPieceEnd below drives it again for
 * every bisection probe. The Butcher tableau is the four {node, weight}
 * pairs at 0x004b5660 and it is textbook RK4:
 *
 *     node    0     1/2   1/2   1
 *     weight  1/6   1/3   1/3   1/6
 *
 * The four state vectors come out of ONE pool allocation:
 *
 *     v[0] = y0    the state at the top of the step
 *     v[1] = acc   the weighted sum of the stage derivatives
 *     v[2] = k     the current stage's derivative, scaled by dt
 *     v[3] = probe the state the stage is evaluated at
 *
 * and each stage is
 *
 *     probe = 1*y0 + node*k          (stage 0 has node 0, so probe = y0,
 *                                     which is why `k` is ZEROED first)
 *     setstate(probe)
 *     k = deriv(t + node*dt)
 *     k *= dt
 *     acc += weight*k
 *
 * with `setstate(acc)` and `ctx->t += dt` at the end. So `acc` starts as a
 * COPY of the state (getstate is called twice, into v[0] and v[1]) and the
 * weighted increments accumulate straight into it -- the integrator never
 * forms y0 + sum separately.
 *
 * TWO CODEGEN NOTES:
 *  * A stage's `node` is read TWICE from the table -- once as a raw dword
 *    for the `scaleadd2` argument push and once as a float for
 *    `dt * node` -- because VC6 does not CSE a float it merely bit-copies
 *    through a GPR (schoolcar.c's PositionRouteCars observation).
 *  * The loop is `i <= 3`, so the strength-reduced table cursor's bound is
 *    `&g_rk4[3]` and the latch is `jle`; `i < 4` gives `jl` against
 *    `&g_rk4[4]`.
 * ======================================================================== */
typedef struct RkStage {
    float node;                 /* +0x00 */
    float weight;               /* +0x04 */
} RkStage;                      /* 8 */

extern const RkStage g_rk4[4];                                  /* 0x004b5660 */

// FUNCTION: LEGOLAND 0x00420310
void Phys_Step(PhysObj* p, PhysStepCtx* ctx, float dt)
{
    PhysVec** a = p->alloc(4);
    PhysVec*  v0 = a[0];
    PhysVec*  v1 = a[1];
    PhysVec*  k = a[2];
    PhysVec*  s = a[3];
    int       i;

    p->zero(k, p->dim);
    p->getstate(p, v0);
    p->getstate(p, v1);
    for (i = 0; i <= 3; i++) {
        p->scaleadd2(v0, 1.0f, k, g_rk4[i].node, s);
        p->setstate(p, s);
        p->deriv(p, dt * g_rk4[i].node + ctx->t, k);
        p->scale(k, dt);
        p->addscaled(k, g_rk4[i].weight, v1);
    }
    p->setstate(p, v1);
    ctx->t += dt;
    p->release(a, 4);
}

/* ==========================================================================
 * 0x0041df00 -- step the train up to the END of the live track piece, and
 * return how much time that took.
 *
 * schoolcar5.c's Route_StepFree asks for this the moment a sub-step has
 * pushed the track parameter past the live piece's `t1`: it restores the
 * snapshot and hands the sub-step's own dt here. This does a BISECTION on
 * the step length -- sixteen or so probes, each of which
 *
 *   * re-seats the whole train from the route's own position record with the
 *     OLD parameter (0x0041da10) and restores the OLD speed (0x0041dad0),
 *     so every probe starts from exactly the same state,
 *   * runs one RK4 step of length `mid`,
 *   * and moves whichever bound the result overshot.
 *
 * The tolerance is a POOLED DOUBLE, 0.001 s, compared with `fcomp qword`.
 * When the interval is tight enough the same restore-and-step is done ONE
 * more time -- written out again, not shared -- and the final midpoint is
 * returned as the time consumed. The guard on the way in is the loop's own
 * first test, which VC6 peels and folds with `lo == 0` into `dt > 0.001`.
 *
 * Note `ctx.t` starts at 0 for every probe but `ctx.f04` is taken once: the
 * probes are independent, and the integrator's own clock is relative.
 *
 * THE LEVER THAT MOVED THE FRAME: `lo = 0.0f;` must be a STATEMENT placed
 * AFTER the `ctx.f04 = rt->f20;` load, not an initialiser. As
 * `float lo = 0.0f;` the store lands at the top of the IR, `lo` takes a real
 * frame slot and the frame grows to 0x18; as a statement in that position it
 * lands at the original's index and `lo` moves into the dead `rt` ARGUMENT
 * slot, giving the original's 0x14 frame with `hi` as the only frame scalar
 * besides `ctx` (and the probe midpoint living in the dead `dt` slot). All
 * six placements were measured; positions 3, 4 and 5 give the 0x14 frame and
 * 0, 1, 2 do not. This is the recorded `Route_AdvanceTrain` rule --
 * "whether a zero float is an INITIALISER or a STATEMENT decides which of
 * two floats takes the dead argument slot" -- confirmed at a second site.
 *
 * Scope G reconstruction: a complete 12-byte snapshot copy preserves the
 * shared source address without an extra move in this routine. The three
 * fields then supply ctx.f04 and the saved parameter/speed in registers.
 * Unlike the earlier scalar form, this has the original 84 instructions and
 * 250 bytes. A free volatile read of hi in each midpoint expression selects
 * the original fld-hi/fadd-lo order without adding loads or instructions.
 *
 * The remaining 32 strict mismatches begin at index1: the initial dt copy
 * and comparison still precede the saved-register pushes, the shared source
 * uses ECX rather than EAX, and later scratch registers rotate. Assigning
 * the final midpoint back into lo restores its original dead-argument home;
 * equal extent still does not make the residual a register-only problem.
 * ======================================================================== */
typedef struct Vec3f { float x; float y; float z; } Vec3f;
typedef struct TrackNode TrackNode;

/* The live track piece; only its parameter range's END is read here. */
typedef struct RouteObj { unsigned char pad00[0x48]; float t1; } RouteObj;

typedef struct RoutePos {
    TrackNode* node;            /* +0x00 */
    RouteObj*  obj;             /* +0x04 */
    Vec3f      pos;             /* +0x08 */
} RoutePos;                     /* 0x14 */

typedef struct CoasterRoute {
    int      state;             /* +0x00 */
    int      started;           /* +0x04 */
    int      deadline;          /* +0x08 */
    RoutePos pos;               /* +0x0c */
    int      f20;               /* +0x20  fed to the step context */
    float    t;                 /* +0x24  the live track parameter */
    float    speed;             /* +0x28 */
    PhysObj  phys;              /* +0x2c */
} CoasterRoute;

/* Both of these really take a FLOAT; declared with the raw dword here (as
 * coaster.c and schoolcar5.c already declare them) because that is what
 * keeps the two snapshots in edi/ebx across the probe. */
#ifndef LEGOLAND_PORTABLE
extern void PositionRouteCars(CoasterRoute* rt, int a, const RoutePos* at); /* 0x0041da10 */
extern void Route_SetSpeed(CoasterRoute* rt, int v);            /* 0x0041dad0 */
#else
/* schoolcar.c defines both with `float`; the raw-dword spelling above is the
 * frame lever. On wasm32 the two are different function types, so the
 * snapshot dwords go across as the floats they are. */
extern void PositionRouteCars(CoasterRoute* rt, float a, const RoutePos* at); /* 0x0041da10 */
extern void Route_SetSpeed(CoasterRoute* rt, float v);          /* 0x0041dad0 */
#define PositionRouteCars(_rt, _a, _at) PositionRouteCars((_rt), LL_ASFLT(_a), (_at))
#define Route_SetSpeed(_rt, _v)         Route_SetSpeed((_rt), LL_ASFLT(_v))
#endif

// FUNCTION: LEGOLAND 0x0041df00
float Route_StepToPieceEnd(CoasterRoute* rt, float dt)
{
    typedef struct Snapshot {int f,t,v;} Snapshot;
    float    hi;
    float    elapsed;
    Snapshot saved;
    float    lo;
    float    mid;

    saved = *(Snapshot*)&rt->f20;
    lo = 0.0f;                  /* a STATEMENT here, not an initialiser */
    hi = dt;
    elapsed = 0.0f;
    while (hi - lo > 0.001) {
        mid = (*(volatile float*)&hi + lo) * 0.5f;
        PositionRouteCars(rt, saved.t, &rt->pos);
        Route_SetSpeed(rt, saved.v);
        Phys_Step(&rt->phys, (PhysStepCtx*)&elapsed, mid);
        if (rt->t > rt->pos.obj->t1)
            hi = mid;
        else
            lo = mid;
    }
    PositionRouteCars(rt, saved.t, &rt->pos);
    Route_SetSpeed(rt, saved.v);
    lo = (*(volatile float*)&hi + lo) * 0.5f;
    Phys_Step(&rt->phys, (PhysStepCtx*)&elapsed, lo);
    return lo;
}

/* ==========================================================================
 * 0x00423350 -- the Z-BUFFER SPAN FILLER.
 *
 * schoolcar5.c's ZBuffer_RunCommand builds the two working arrays and hands
 * them here; this is the scanline loop that consumes them. It is a classic
 * two-chain polygon filler:
 *
 *   * `key[i]` is read IN ORDER, never sorted: each entry says "at scanline
 *     key[i].y, chain `edge[key[i].idx].side` switches to that edge". The
 *     producer therefore emits the edges already ordered by start scanline.
 *   * Each chain keeps a 16.16 x and a per-scanline step. The x is stored
 *     PRE-BIASED by one step (`x - step`), because every scanline advances
 *     both chains BEFORE it draws -- so the first scanline drawn uses the
 *     edge's own starting x.
 *   * A span is painted only when the two chains are at least half a pixel
 *     apart (`right - left >= 0x8000`), from `left >> 16` to `right >> 16`
 *     inclusive.
 *   * The outer loop stops at the sentinel ZBuffer_RunCommand planted in
 *     `key[n]`, which is why that routine INCREMENTS the last edge's +0x02
 *     and copies it there: the two are one mechanism.
 *
 * The buffer is three globals -- 0x004b5b24 the 16-bit target's base,
 * 0x004b5b28 its pitch in pixels, 0x004b5b20 a third that this routine
 * copies into a local and never reads (an original DEAD STORE, reproduced
 * as a plain store into a member of the frame aggregate -- see the DEAD
 * STORE lever below). 0x0060f900 counts the polygons filled. The value written is
 * a local that is only ever ZERO, so the pass clears the polygon's coverage
 * rather than painting a colour.
 *
 * =========================================================================
 * PART OF THIS FUNCTION IS HAND-WRITTEN ASSEMBLY -- three separate proofs:
 * =========================================================================
 *  1. It has an EBP FRAME (`push ebp / mov ebp,esp / sub esp,0x60`) in an
 *     /O2 file, which VC6 emits only for `__asm`, `alloca` or SEH.
 *  2. `xchg ebx,eax` (the one-byte 0x93 form) for a register swap, and
 *     `add ebx,1` (3 bytes) where VC6 always emits `inc ebx` (1 byte).
 *  3. `cmp ecx,8000h / jns wide / jmp done` -- a two-instruction branch
 *     where `js done` alone would do. That is a hand-written `jns/jmp`
 *     pair, not a compiler's inverted test.
 * Everything around it is ordinary VC6 output, and the boundary is visible:
 * after the block VC6 reloads `pitch`, `row`, `y` and even the `edge`
 * parameter from memory, because it assumes an `__asm` region clobbers
 * eax/ecx/edx/edi. That is also why every per-scanline value lives in the
 * frame: the block names them.
 *
 * THE FOUR EDGE SLOTS ARE 0x14 BYTES APART. The frame is
 * [4 x 0x14 interpolant records][dead][row][pitch][fillv] = 0x60 exactly,
 * with the four records at -0x60/-0x4c/-0x38/-0x24 and only their FIRST
 * dword ever touched. A 0x14-byte record is five ints -- x plus four more
 * interpolants -- so this is the module's general span-edge record used by a
 * flat fill that needs only x. Written as four plain `int` locals the frame
 * collapses to 0x1c; only an ARRAY of the 0x14-byte record reserves the
 * homes, and only ONE array (`ed[4]`) puts them in the original's order --
 * four separate arrays come out ldp/rdp/lp/rp, and declaration order is
 * inert across all eleven permutations measured.
 *
 * THREE MORE LEVERS, all three carried over from scope LL9's closing of
 * the shaded twin (unref1.c ZBuffer_FillShadedPoly, 0x0041fa10):
 *  * FRAME HOMES. The dead copy, the row pointer and `pitch` are ONE
 *    aggregate, `struct { int dead; short* row; int pitch; } r`, and the
 *    `__asm` block names `r.row`. VC6 gives a multi-member aggregate a
 *    true frame home, deepest first in declaration order, AHEAD of every
 *    spilled scalar; so `r` lands at -0x10/-0xc/-8 and the two remaining
 *    spills, `ylast` and `y`, fall back to the dead `n` and `key` argument
 *    slots exactly as the original has them. That is the tie-break the old
 *    residual called unreachable: as five plain scalars VC6 hands `ylast`
 *    the -0xc home and `row` the argument slot. Measured on the way:
 *    `{row, pitch}` alone puts `r` BELOW the dead scalar (-0x10/-0xc, dead
 *    at -8); a 4-byte `dead` in any aggregate shape (`short[2]`,
 *    `char[4]`, a struct of two shorts, a one-member struct or array) is
 *    scalarised and sorted after `r` all the same; `{dead, row}` with a
 *    scalar `pitch` fixes those two homes but swaps `pitch` and `ylast`
 *    and rotates n/y into EDI (78/101).
 *  * THE DEAD STORE is a PLAIN member store, `r.dead = g_zb_4b5b20;`. VC6
 *    does not delete a store into a memory-resident aggregate, so no
 *    volatile is needed -- and none must be used: a volatile cast on the
 *    member (`*(int volatile*)&r.dead`), on `&r`, or a `volatile int`
 *    member makes the whole aggregate address-exposed, the frame grows to
 *    0x64, `pitch*2` is hoisted into the `n` slot and the body drops to
 *    60/104. A volatile READ of the global with a plain member store is
 *    99/101 (the load moves). As a separate scalar, the store needs the
 *    volatile cast (a plain scalar store is deleted outright even beside
 *    `__asm`), which is why the old body carried one.
 *  * THE PER-KEY EDGE SWITCH. The original hoists `e->x` above the
 *    `e->side` branch ahead of `e->step`. Written `ed[k].x = e->x -
 *    e->step;` VC6 forms the CSE temporary for `e->step` first and hoists
 *    that instead. Each arm is stores then a read-modify-write through the
 *    address-taken `ed`: `ed[k].x = e->x; ed[k+1].x = e->step; ed[k].x -=
 *    ed[k+1].x;` -- store-to-load forwarding folds the RMW back to the one
 *    register subtract, so the count does not move, only the order.
 * With row and pitch as members the old free volatile read of `y` in
 * `row = base + pitch * y` is no longer wanted: written `r.row = g_zb_base;
 * r.pitch = g_zb_pitch;` up front and `r.row += r.pitch * y;` after the
 * `key[n].y` store, VC6 loads the base into EDI right after the `inc`,
 * keeps `pitch` in EBX and reloads `y` from its home for the multiply
 * itself; with the volatile read kept it is one instruction LONG (90/102:
 * the pitch load a slot early and the `y` reload duplicated into the
 * outer latch). The set-up order above is the one that matches: `pitch`
 * before `row` (99), the dead store before `ylast` (98) or after the
 * counter (99) each move a load in the head.
 *
 * CLOSED at 101/101 instructions and 292/292 bytes, strict 0, from the
 * recorded 46: the old residual (row/ylast homes exchanged plus the
 * register rotation and the head's scheduling window) was the frame-home
 * rule above, not a global rank.
 * ======================================================================== */
typedef struct ZKey {
    int y;                      /* +0x00 */
    int idx;                    /* +0x04 */
} ZKey;                         /* 8 */

typedef struct ZEdge {
    short         f00;          /* +0x00 */
    short         ylast;        /* +0x02 */
    int           side;         /* +0x04  1 = the right-hand chain */
    int           x;            /* +0x08  16.16 */
    unsigned char pad0c[0x1c - 0x0c];
    int           step;         /* +0x1c  16.16 */
    unsigned char pad20[0x30 - 0x20];
} ZEdge;                        /* 0x30 */

/* One span-edge interpolant record: five ints, of which only x is used by
 * this flat fill. The `__asm` block below addresses them by BYTE offset
 * (`ed[20]` is ed[1].x), because MSVC's inline assembler does not scale a
 * bracketed index by the element size. */
typedef struct ZInterp {
    int x;                      /* +0x00 */
    int rest[4];                /* +0x04 */
} ZInterp;                      /* 0x14 */

extern short* g_zb_base;                                        /* 0x004b5b24 */
extern int    g_zb_pitch;                                       /* 0x004b5b28 */
extern int    g_zb_4b5b20;                                      /* 0x004b5b20 */
extern int    g_zb_polys;                                       /* 0x0060f900 */

// FUNCTION: LEGOLAND 0x00423350
void ZBuffer_FillPoly(int n, ZKey* key, ZEdge* edge)
{
    ZInterp      ed[4];
    /* ONE aggregate: the dead copy, the row pointer and the pitch. See the
     * FRAME HOMES lever above -- this is what puts the three at
     * -0x10/-0xc/-8 and leaves `ylast` and `y` to the argument slots. */
    struct { int dead; short* row; int pitch; } r;
    int          fillv;
    int          y;
    int          ylast;

    fillv = 0;
    y = key[0].y;
    edge[key[n - 1].idx].ylast++;
    r.row = g_zb_base;
    r.pitch = g_zb_pitch;
    ylast = edge[key[n - 1].idx].ylast;
    r.dead = g_zb_4b5b20;                       /* the original's dead store */
    g_zb_polys++;
    key[n].y = edge[key[n - 1].idx].ylast;
    r.row += r.pitch * y;
    do {
        ZEdge* e = &edge[key->idx];

        key++;
        /* Stores, then a read-modify-write through the address-taken `ed`:
         * this is what hoists `e->x` above the branch ahead of `e->step`
         * (store-to-load forwarding folds the RMW back to one subtract). */
        if (e->side) {
            ed[2].x = e->x;
            ed[3].x = e->step;
            ed[2].x -= ed[3].x;
        } else {
            ed[0].x = e->x;
            ed[1].x = e->step;
            ed[0].x -= ed[1].x;
        }
        while (y < key->y) {
            y++;
#ifndef LEGOLAND_PORTABLE
            __asm {
                mov  eax, ed[0]                 /* left.x            */
                mov  ebx, ed[40]                /* right.x           */
                add  eax, ed[20]                /* += left.step      */
                add  ebx, ed[60]                /* += right.step     */
                mov  ed[0], eax
                mov  ed[40], ebx
                mov  ecx, ebx
                sub  ecx, eax
                cmp  ecx, 8000h                 /* half a pixel?     */
                jns  wide
                jmp  done
            wide:
                sar  eax, 16
                sar  ebx, 16
                mov  edi, r.row
                xchg ebx, eax
                mov  dx, word ptr fillv
                sub  ebx, eax                   /* left - right, <= 0 */
                lea  edi, [edi + eax*2]         /* &row[right]        */
            fill:
                mov  word ptr [edi + ebx*2], dx
                add  ebx, 1
                jle  fill
            done:
            }
#else
            {
                int ll_l, ll_r, ll_x;
                ed[0].x += ed[1].x;
                ed[2].x += ed[3].x;
                if (ed[2].x - ed[0].x >= 0x8000) {
                    ll_l = ed[0].x >> 16;
                    ll_r = ed[2].x >> 16;
                    for (ll_x = ll_l; ll_x <= ll_r; ll_x++)
                        r.row[ll_x] = (short)fillv;
                }
            }
#endif
            r.row += r.pitch;
        }
    } while (y < ylast);
}

/* ==========================================================================
 * 0x00422e40 -- build ONE 128-byte SHADE RAMP for a 24-bit colour.
 *
 * schoolcar5.c's CoasterShades_Init slices a block into one of these per
 * palette colour (and builds a single white one into the static fallback
 * when the allocation fails); this is the builder. The ramp is 64 packed
 * 16-bit pixels:
 *
 *     entry  0  BLACK
 *     entry 32  the colour itself
 *     entry 63  WHITE
 *
 * -- two linear runs in the display's own pixel format. The selector at
 * 0x00668088 picks it: value 2 means 5/6/5 (green 6 bits, red shifted 11),
 * anything else 5/5/5 (green 5, red shifted 10), and BOTH arms store the
 * green width to the frame because the red shift lives in a register.
 *
 * The three components are taken straight out of the 24-bit value at the
 * display's precision:
 *
 *     red   = (rgb >> 19) & 0x1f                    5 bits
 *     green = (rgb & 0xff00) >> (16 - greenbits)    5 or 6 bits
 *     blue  = (rgb >> 3) & 0x1f                     5 bits
 *
 * and each run keeps all three accumulators on x87 and rounds per entry:
 *
 *     run 1, 33 entries from 0:      step = component / 32
 *     run 2, 32 entries from the colour: step = (max - component) / 31
 *
 * where `max` is 31.0 for red and blue and `(1 << greenbits) - 1` for green.
 * Entry 32 is therefore written TWICE, once as the end of the dark run and
 * once as the start of the light one, with the same value.
 *
 * THE X87 STACK IS THE WHOLE FUNCTION. Each run keeps SIX values live on
 * the stack for its entire body -- three accumulators and three steps --
 * pushed accumulators-first so the loop reads them as st(5)/st(4)/st(3) and
 * the steps as st(2)/st(1)/st(0), and the accumulate is `fld st(n) /
 * faddp st(n+1)` per component. Only the three component values themselves
 * get frame homes, and `red`'s home is the DEAD `rgb` argument slot, which
 * run 2 then reuses again for the `fild` of `(1 << greenbits) - 1`.
 *
 * FIVE THINGS THE CODEGEN SAYS:
 *  * `fild qword` with a zero high dword is VC6's UNSIGNED-to-float
 *    conversion, so the three components are `unsigned`. (schoolcar5.c
 *    declares this function's first parameter `int`; it must be `unsigned`
 *    here or the conversions come out as `fild dword`. ABI-identical, and
 *    that extern is left alone.)
 *  * `mov bp,ax` after `__ftol` is the 16-bit narrowing of a compiler
 *    TEMPORARY -- the packed pixel -- not a `short` local; the following
 *    `shl ebp,cl` is 32-bit because only the low half is ever stored.
 *  * Run 2's three subtractions are `fsub st(3)`: the saved components
 *    remain float, but the six register-resident accumulators and steps are
 *    double. Float-to-double copies preserve distinct x87 identities and
 *    put the integer green-maximum setup at the original schedule boundary.
 *    Float accumulators required volatile copies and delayed that setup.
 *  * The two loops are NOT merged and their trip counts differ by one (33
 *    and 32); the second starts at `out + 0x40`.
 *  * `0.032258064f` is 1/31 as a float and `0.03125f` is 1/32; both are
 *    exact single-precision literals in the original's pool.
 *
 * MATCHED (Scope G, 2026-09-05): 129/129 instructions, 401/401 bytes,
 * strict 0. The local static float constants keep zero and 31 as dword
 * loads even though their destinations are double. Casting each second-run
 * subtraction to float keeps the 1/31 multiplier dword-sized. All eleven
 * floating-point constant references were checked against the original
 * bytes; both exact neighbors and the unassigned span body are unchanged.
 * ======================================================================== */
extern int g_pixel_fmt;                                         /* 0x00668088 */

// FUNCTION: LEGOLAND 0x00422e40
void Shade_BuildRamp(unsigned int rgb, unsigned short* out)
{
    static const float zero = 0.0f, maximum = 31.0f;
    unsigned short* dst;
    int             gbits;
    int             rshift;
    int             i;
    float           rv, gv, bv;
    double          ra, ga, ba;
    double          rs, gs, bs;

    if (g_pixel_fmt == 2) {
        gbits = 6;
        rshift = 11;
    } else {
        gbits = 5;
        rshift = 10;
    }
    ra = zero;
    ga = zero;
    ba = zero;
    rv = (float)((rgb >> 19) & 0x1f);
    rs = rv * 0.03125f;
    gv = (float)((rgb & 0xff00) >> (16 - gbits));
    gs = gv * 0.03125f;
    bv = (float)((rgb >> 3) & 0x1f);
    bs = bv * 0.03125f;
    dst = out;
    for (i = 33; i != 0; i--) {
#ifndef LEGOLAND_PORTABLE
        *dst++ = (unsigned short)(((int)ra << rshift) | ((int)ga << 5) | (int)ba);
#else
        /* PORT-M5: 0x00458930 ROUNDS (a bare `fistp` under the game's
         * round-to-nearest control word), so a C cast is one level out on
         * every ramp entry whose channel has a fraction. */
        *dst++ = (unsigned short)((LL_FISTP(ra) << rshift) |
                                  (LL_FISTP(ga) << 5) | LL_FISTP(ba));
#endif
        ra += rs;
        ga += gs;
        ba += bs;
    }
    /* The float-to-double copies keep the saved components distinct from
     * the live x87 accumulators without volatile scheduling barriers. */
    ra = rv;
    ga = gv;
    ba = bv;
    rs = (float)(maximum - ra) * 0.032258064f;
    gs = (float)((float)((1 << gbits) - 1) - ga) * 0.032258064f;
    bs = (float)(maximum - ba) * 0.032258064f;
    dst = out + 32;
    for (i = 32; i != 0; i--) {
#ifndef LEGOLAND_PORTABLE
        *dst++ = (unsigned short)(((int)ra << rshift) | ((int)ga << 5) | (int)ba);
#else
        /* PORT-M5: 0x00458930 ROUNDS (a bare `fistp` under the game's
         * round-to-nearest control word), so a C cast is one level out on
         * every ramp entry whose channel has a fraction. */
        *dst++ = (unsigned short)((LL_FISTP(ra) << rshift) |
                                  (LL_FISTP(ga) << 5) | LL_FISTP(ba));
#endif
        ra += rs;
        ga += gs;
        ba += bs;
    }
}
