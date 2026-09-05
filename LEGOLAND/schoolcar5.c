/* LEGOLAND -- the last of the DRIVING SCHOOL / coaster subs in the
 * 0x0041e000..0x00423400 range: the shade-ramp table, the 3D command
 * dispatcher, the whole-file loader, the free-running route stepper and the
 * curve's parameter collector.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere) and follow schoolcar.c / schoolcar3.c / schoolcar4.c.
 */

/* ==========================================================================
 * 0x00422fe0 -- build the module's SHADE-RAMP table.
 *
 * CoasterGeomInit (schoolcar.c, 0x00423740) runs this first, right after
 * LoadCoasterData has published the module object at 0x004d8bac. That object
 * begins with the module's COLOUR TABLE -- `{ int n; unsigned rgb[n]; }` --
 * which 0x004207c0 hands back whole and 0x004207d0 searches by masked
 * (`& 0xffffff`) colour, so the entries are 24-bit RGB.
 *
 * Each colour gets a 128-byte SHADE RAMP: one contiguous block of n*128
 * bytes is allocated at 0x00829c54 and 0x00422e10 both slices it
 * (`base + i*128`) into the 1024-entry pointer table at 0x00829c60 and fills
 * the slice through 0x00422e40, which is the ramp builder proper (it reads
 * the 5/6/5-vs-5/5/5 selector at 0x00668088 and interpolates the colour's
 * three components in x87).
 *
 * If the allocation fails the module does not give up: it builds ONE ramp for
 * white into the static fallback at 0x00579878 and points all 1024 table
 * slots at it, so every later lookup still finds a legal ramp.
 * ======================================================================== */
extern int*  GetCoasterColours(void);                           /* 0x004207c0 */
extern void* AllocZeroed(unsigned int size, int a, void* tag, int c); /* 0x004775b0 */
extern void  Shade_BuildRamp(int rgb, void* out);               /* 0x00422e40 */
extern void  Shade_BuildEntry(int rgb, int index);              /* 0x00422e10 */

extern int   g_shade_count;                                     /* 0x0060f904 */
extern void* g_shade_block;                                     /* 0x00829c54 */
extern void* g_shade_tab[0x400];                                /* 0x00829c60 */
extern unsigned char g_shade_white[0x80];                       /* 0x00579878 */

// FUNCTION: LEGOLAND 0x00422fe0
void CoasterShades_Init(void)
{
    int* pal = GetCoasterColours();
    int  n;
    int  i;

    n = *pal++;
    g_shade_count = n;
    g_shade_block = AllocZeroed(n * 128, 0, 0, 0);
    if (!g_shade_block) {
        Shade_BuildRamp(0xffffff, g_shade_white);
        for (i = 0; i < 0x400; i++)
            g_shade_tab[i] = g_shade_white;
        return;
    }
    for (i = 0; i < n; i++)
        Shade_BuildEntry(*pal++, i);
}

/* ==========================================================================
 * 0x004232b0 -- decode one entry of the module's 3D COMMAND BUFFER and hand
 * it to the z-buffer span filler at 0x00423350.
 *
 * Coaster3D_EndFrame (schoolcar.c, 0x00423140) walks 0x004dd870 and calls
 * this once per command; schoolcar.c already established the stride, "its
 * first dword is a count and the next command starts count*8 + 8 bytes on",
 * so the command is a HEADER plus `n` eight-byte vertex records:
 *
 *     +0x00  int   n          how many edge records follow
 *     +0x04  short ylast      the polygon's last scanline, exclusive-ish
 *     +0x08  { short x; short y; int step; } v[n]
 *
 * The decode fills two parallel working arrays, both eight entries deep --
 * that cap is the frame (0x40 + 0x180 = 0x1c0) and it is the module's limit
 * on the number of edges one command may carry:
 *
 *     ZKey  key[8]   {int y; int idx;}   the scanline the edge starts on and
 *                                        which edge it is
 *     ZEdge edge[8]  0x30 bytes each     +0x02 last scanline, +0x04 side,
 *                                        +0x08 x in 16.16, +0x1c dx/dy
 *
 * The vertex's `y` is SIGNED and the sign is not part of the coordinate: it
 * selects which of the filler's two edge chains (left or right) the record
 * joins, so the key gets |y| and +0x04 gets the sign as a 0/1 flag. The
 * filler reads `key[]` in order and never sorts, so the producer emits the
 * edges already ordered by scanline.
 *
 * Only the LAST edge's +0x02 is initialised, to `ylast - 1`: 0x00423350
 * INCREMENTS that field in place and writes the result into `key[n]` -- one
 * past the live keys -- as the sentinel that stops its scanline walk. So the
 * pre-decrement here and the `inc word ptr` there are one mechanism, and the
 * other seven +0x02 fields are never read.
 *
 * The store is indexed by the LOOP variable, not by `n`, which is why VC6
 * folds `(i-1)*0x30 + 2` into the `[esp + i*0x30 + 0x1e]` displacement; on a
 * command with n <= 0 it would write 0x2e bytes below `edge[0]`, i.e. into
 * `key[]`. Reproduced -- the producer never emits an empty command.
 *
 * THREE LEVERS, all measured here, all about the three strength-reduced
 * cursors VC6 manufactures for `key[i]`, `edge[i]` and `c->v[i]`:
 *  * WHICH offset a cursor is anchored at is decided by the REFERENCE COUNT
 *    of the fields in the group, not by their order. `edge[i].side` is
 *    written in BOTH arms of the `if`, so +0x04 has two references and the
 *    cursor sits there, addressing +0x08 and +0x1c forwards; `c->v[i].y` is
 *    named THREE times (the test and one arm each) so its cursor sits at the
 *    record's +0x02 and reads `.x` back as `[edx-2]`. Spelling either of
 *    those once -- `short y = c->v[i].y;` and a `k` temporary -- moves both
 *    anchors to the record base and costs 16 of 53.
 *  * On a TIE the LAST reference wins: `key[i].y` then `key[i].idx` anchors
 *    at +0x04 and stores through `[ecx-4]/[ecx]`; the two written the other
 *    way round anchors at +0x00. Writing `key[i].y` inside both arms breaks
 *    the tie the other way (two references) and keeps the emitted pair in
 *    y-then-idx order, because VC6 cross-jumps the two arms' identical
 *    stores into the join.
 *  * The ORDER OF THE INCREMENTS in the loop latch follows source statement
 *    order even when the store itself is scheduled elsewhere. With
 *    `key[i].idx = i;` written before the two `edge[i]` stores the latch is
 *    `inc esi / add ecx,8 / add edx,8 / add eax,0x30`; written LAST it is the
 *    original's `inc esi / add eax,0x30 / add ecx,8 / add edx,8`, while the
 *    store stays hoisted back next to `key[i].y`. (DECOMP records the
 *    opposite as a negative from another body -- so latch order is reachable
 *    from source on this shape, and 24 orders were measured to find it.)
 * ======================================================================== */
typedef struct ZCmdVert {
    short x;                    /* +0x00  start x, whole pixels */
    short y;                    /* +0x02  start scanline, sign = which chain */
    int   step;                 /* +0x04  dx/dy in 16.16 */
} ZCmdVert;                     /* 8 */

typedef struct ZCmd {
    int      n;                 /* +0x00 */
    short    ylast;             /* +0x04 */
    short    f06;               /* +0x06 */
    ZCmdVert v[1];              /* +0x08 */
} ZCmd;

typedef struct ZKey {
    int y;                      /* +0x00 */
    int idx;                    /* +0x04 */
} ZKey;                         /* 8 */

typedef struct ZEdge {
    short         f00;          /* +0x00 */
    short         ylast;        /* +0x02  only the last edge's is set */
    int           side;         /* +0x04  1 = the negative chain */
    int           x;            /* +0x08  16.16 */
    unsigned char pad0c[0x1c - 0x0c];
    int           step;         /* +0x1c  16.16 */
    unsigned char pad20[0x30 - 0x20];
} ZEdge;                        /* 0x30 */

extern void ZBuffer_FillPoly(int n, ZKey* key, ZEdge* edge);    /* 0x00423350 */

// FUNCTION: LEGOLAND 0x004232b0
void ZBuffer_RunCommand(ZCmd* c)
{
    ZKey  key[8];
    ZEdge edge[8];
    int   i;
    int   n;

    n = c->n;
    for (i = 0; i < n; i++) {
        if (c->v[i].y < 0) {
            edge[i].side = 1;
            key[i].y = -c->v[i].y;
        } else {
            edge[i].side = 0;
            key[i].y = c->v[i].y;
        }
        edge[i].x = (int)c->v[i].x << 16;
        edge[i].step = c->v[i].step;
        key[i].idx = i;
    }
    edge[i - 1].ylast = (short)(c->ylast - 1);
    ZBuffer_FillPoly(n, key, edge);
}

/* ==========================================================================
 * 0x00422470 -- read a whole file into a fresh block.
 *
 * schoolcar4.c's LoadCoasterModelSet calls this twice, for "<name>.obj" and
 * "<name>.txt", and keeps {image, byte length} for each; this is the body it
 * described from the outside.  It is a plain CreateFileA / GetFileSize /
 * allocate / ReadFile / CloseHandle, and it is STRICT: a short read frees the
 * block and fails rather than returning a partial image.
 *
 * FOUR THINGS THE CODEGEN SAYS:
 *  * `if (!name) return 0;` needs no `xor` -- the test is on the value just
 *    loaded into eax, and on that edge eax IS zero, so the early exit is a
 *    bare pop/pop/pop/ret while the three LATER failures each carry their own
 *    `xor eax,eax` between the pops.  Four textual `return 0`s, none merged.
 *  * `got` is homed in the DEAD `name` ARGUMENT SLOT -- `lea eax,[esp+0x10]`
 *    is `[esp0+4]`, read by push depth -- which is why the frame is three
 *    pushes and nothing else.  The recorded "uninitialised out-pointer locals
 *    are homed in dead argument slots" rule, at its cleanest.
 *  * The allocation is the module's four-argument facade with the .bss tag at
 *    0x004d8bb0 in the third slot; 0x004775b0 forwards only the size, so the
 *    tag is a label the shipped build ignores.
 *  * `CloseHandle(h)` appears in three arms and is duplicated in all three --
 *    the success arm's copy is followed by `*len = n` and the pointer return,
 *    so there is no shared tail to cross-jump into.
 * ======================================================================== */
__declspec(dllimport) int __stdcall CreateFileA(const char* name,
                                                unsigned int access,
                                                unsigned int share, void* sa,
                                                unsigned int disp,
                                                unsigned int flags,
                                                void* tmpl);            /* [0x4ab258] */
__declspec(dllimport) int __stdcall GetFileSize(int h, unsigned int* hi); /* [0x4ab25c] */
__declspec(dllimport) int __stdcall CloseHandle(int h);                  /* [0x4ab260] */
__declspec(dllimport) int __stdcall ReadFile(int h, void* buf, unsigned int n,
                                             unsigned int* got, void* ov); /* [0x4ab264] */

extern void  Free_w(void* p);                                   /* 0x004775d0 */
extern char  g_alloc_tag[];                                     /* 0x004d8bb0 */

// FUNCTION: LEGOLAND 0x00422470
void* LoadWholeFile(const char* name, unsigned int* len)
{
    int          h;
    void*        p;
    unsigned int n;
    unsigned int got;

    if (!name)
        return 0;
    h = CreateFileA(name, 0x80000000, 1, 0, 3, 0x8000000, 0);
    if (h == -1)
        return 0;
    n = GetFileSize(h, 0);
    p = AllocZeroed(n, 0, g_alloc_tag, 0);
    if (!p) {
        CloseHandle(h);
        return 0;
    }
    ReadFile(h, p, n, &got, 0);
    if (got != n) {
        Free_w(p);
        CloseHandle(h);
        return 0;
    }
    CloseHandle(h);
    *len = n;
    return p;
}

/* ==========================================================================
 * 0x0041e000 -- the FREE-RUNNING route stepper.
 *
 * Route_AdvanceTrain (schoolcar4.c, 0x0041e130) picks one of three steppers
 * per sub-step and this is the default one -- the train under gravity, with
 * neither the lift bit (8) nor the past-the-lap bit (0x10) set. It is handed
 * the time still owed for this frame and returns how much of it it consumed,
 * which is what makes the caller's `acc += stepper(rt, dt - acc)` loop
 * terminate.
 *
 * It sub-divides that time at SEVENTY steps per second -- `ceil(dt * 70)`
 * sub-steps of `dt / n` each -- and runs the physics object embedded at
 * rt+0x2c once per sub-step through 0x00420310, which drives it from a small
 * 16-byte context whose +0x04 is refreshed from rt+0x20 every time. The
 * whole time is consumed and `dt` returned if the train stays on the piece.
 *
 * The interesting half is the ROLLBACK. Before each sub-step the route's
 * three live scalars (+0x20, the track parameter +0x24 and the speed +0x28)
 * are snapshotted into registers; if the sub-step pushed the parameter past
 * the live piece's END (+0x48 of rt->pos.obj) the snapshot is put back --
 * PositionRouteCars re-seats the whole train from the route's own position
 * record with the OLD parameter, Route_SetSpeed restores the old speed --
 * and 0x0041df00 is asked to do the partial step up to the boundary. Its
 * result plus the sub-steps already banked is the time consumed. So a piece
 * boundary is never crossed inside a sub-step: it is always backed out of
 * and re-done exactly.
 *
 * `PositionRouteCars(rt, t, &rt->pos)` copies rt->pos onto itself, which is
 * a no-op the original pays for; it is there because the same routine also
 * serves the save-restore path, which passes a foreign position record.
 *
 * FOUR LEVERS THAT LANDED (59 -> 64 of 75 with the instruction count exact):
 *  * The two snapshots must be RAW DWORDS passed to `int` prototypes (which
 *    is how coaster.c already declares both callees). As `float` locals they
 *    get stack homes, `i` takes edi instead, and 17 instructions move; as
 *    ints they take edi/ebx across the call, exactly as the original, and
 *    `i`/`n` are then spilled and reloaded in the latch.
 *  * The step context's +0x00 is a FLOAT. `ctx.f00 = 0;` on an int field
 *    merges with `i = 0` into one hoisted zero register (`xor ecx,ecx` plus
 *    `cmp eax,ecx` for the guard); `ctx.f00 = 0.0f;` on a float field keeps
 *    both stores immediate and the guard `test eax,eax`. Worth 3.
 *  * The parameter +0x24 is read TWICE (once as the argument dword, once for
 *    the `fcomp`) because VC6 does not CSE a float it merely bit-copies
 *    through a GPR -- schoolcar.c's observation on PositionRouteCars, from
 *    the calling side.
 *  * A FREE volatile read of `acc` at the accumulate is the only thing that
 *    puts the `fld` on `acc` rather than on `step`: `acc += step`,
 *    `acc = acc + step` and `acc = step + acc` are byte-identical, so the
 *    recorded "VC6 flds the operand written SECOND" rule does NOT reach this
 *    site -- both orders canonicalise to `fld step / fadd acc`.

 * WIP RESIDUAL: 76/76 instructions, 217/218 bytes, 64 of 75 aligned, and the
 * body is exact except for ONE five-instruction window at the top of the
 * loop (audit's strict index compare reports 53 only because that window is
 * scheduled early and shifts everything after it). The original computes a
 * shared ADDRESS REGISTER for the snapshot group -- `lea eax,[esi+0x20]`,
 * then `[eax]/[eax+4]/[eax+8]` -- and does the three loads AFTER the first
 * two argument pushes, with the `ctx.f04` store between the first and second
 * load. Ours folds the displacements into esi and batches the loads ahead of
 * the store. Measured and inert: a named `int*` / `RouteLive*` cursor for the
 * group (inside the loop, outside it, and `volatile`), all three orders of
 * the three reads, a volatile STORE to ctx.f04, volatile reads on either or
 * both snapshots, a named `StepCtx*` and a named `RoutePhys*` for the two
 * pointer arguments, six init/guard orders and `while` vs `for`, and a real
 * `RouteState` SUB-STRUCT at +0x20 read through a `RouteState*` (inside the
 * loop, outside it, and with `&s->phys` as the call's first argument -- 63,
 * 63, 42). VC6 folds every one of those pointers back into `esi + disp`; the
 * original pays three extra bytes for the base register, so something in the
 * original source forces it that no pointer spelling reaches. The one byte of
 * difference is exactly that `lea`/`mov ecx,[eax]` pair.
 * ======================================================================== */
#include <math.h>

typedef struct Vec3f { float x; float y; float z; } Vec3f;

typedef struct TrackNode TrackNode;

/* The live track piece; only its parameter range's END is read here. */
typedef struct RouteObj { unsigned char pad00[0x48]; float t1; } RouteObj;

typedef struct RoutePos {
    TrackNode* node;            /* +0x00 */
    RouteObj*  obj;             /* +0x04 */
    Vec3f      pos;             /* +0x08 */
} RoutePos;                     /* 0x14 */

/* The physics object embedded in the route at +0x2c: 0x00420310 calls it
 * through function pointers at its own +0x04/+0x24/+0x2c. */
typedef struct RoutePhys { unsigned char pad00[0x40]; } RoutePhys;

typedef struct CoasterRoute {
    int       state;            /* +0x00 */
    int       started;          /* +0x04 */
    int       deadline;         /* +0x08 */
    RoutePos  pos;              /* +0x0c */
    int       f20;              /* +0x20  fed to the step context each time */
    float     t;                /* +0x24  the live track parameter */
    float     speed;            /* +0x28 */
    RoutePhys phys;             /* +0x2c */
} CoasterRoute;

/* 0x00420310's scratch: only +0x00 (zeroed once) and +0x04 are written here. */
typedef struct StepCtx {
    float         f00;          /* +0x00  a FLOAT: see the note below */
    int           f04;          /* +0x04 */
    unsigned char pad08[8];
} StepCtx;                      /* 0x10 */

extern void  Phys_Step(RoutePhys* p, StepCtx* ctx, float dt);   /* 0x00420310 */
/* Both of these really take a FLOAT; declared with the raw dword here (as
 * coaster.c already declares them) because that is what keeps the two
 * snapshots in edi/ebx across Phys_Step instead of giving them stack homes. */
extern void  PositionRouteCars(CoasterRoute* rt, int a,
                               const RoutePos* at);             /* 0x0041da10 */
extern void  Route_SetSpeed(CoasterRoute* rt, int v);           /* 0x0041dad0 */
extern float Route_StepToPieceEnd(CoasterRoute* rt, float dt);  /* 0x0041df00 */

// WIP-FUNCTION: LEGOLAND 0x0041e000  (76/76 insns, 217/218 B, 64 of 75 aligned; one 5-instruction window at the loop head)
float Route_StepFree(CoasterRoute* rt, float dt)
{
    StepCtx ctx;
    float   acc;
    float   step;
    int     i;
    int     n;

    n = (int)ceil(dt * 70.0f);
    acc = 0.0f;
    ctx.f00 = 0.0f;
    step = dt / n;
    for (i = 0; i < n; i++) {
        int t;
        int v;

        ctx.f04 = rt->f20;
        t = *(int*)&rt->t;
        v = *(int*)&rt->speed;
        Phys_Step(&rt->phys, &ctx, step);
        if (rt->t > rt->pos.obj->t1) {
            PositionRouteCars(rt, t, &rt->pos);
            Route_SetSpeed(rt, v);
            return Route_StepToPieceEnd(rt, step) + acc;
        }
        /* The volatile read is FREE -- the original loads `acc` here anyway
         * -- and it is the only thing that puts the `fld` on `acc` instead of
         * on `step`: all three spellings of the sum (`+=`, `acc + step`,
         * `step + acc`) are byte-identical without it. */
        acc = *(float volatile*)&acc + step;
    }
    return dt;
}

/* ==========================================================================
 * 0x00421e90 -- the track curve's ADAPTIVE SUBDIVISION.
 *
 * schoolcar4.c's TrackCurve_GatherParams seeds the collector with the
 * segment's two ends and then hands them to this routine as the pair
 * (t0, z(t0)) / (t1, z(t1)); everything else in the parameter array is put
 * there from here. The rule is flatness against the CHORD:
 *
 *     m = (pb - pa) / (b - a)          the chord's slope
 *     c = pa - m*a                     its intercept
 *
 * The height cubic's derivative is `3*c3*u^2 + 2*c2*u + c1`, so the two
 * parameters where the curve's tangent is PARALLEL to the chord -- i.e. where
 * it deviates most from it -- are the roots of
 *
 *     3*c3*u^2 + 2*c2*u + (c1 - m) = 0
 *
 * which the body solves with the quadratic formula spelled around `3*c3` and
 * `2*c2` as shared subexpressions (`4*a*c` is literally `(c1-m) * 3c3 * 4`).
 * Each root that falls inside [a, b] is scored by |z(u) - (m*u + c)| and the
 * worst one wins; `best` starts at FLT_MIN (0x00800000), which is how "no
 * root in range" turns into an immediate return.
 *
 * If the worst deviation is more than ONE world unit the parameter is
 * appended to the collector (`*g_tc_wp++ = u; g_tc_n++;`) and the segment is
 * split at it: a real recursive call on [a, u] and a TAIL CALL on [u, b],
 * which VC6 turns into the `jmp` back to the top with the two parameters
 * overwritten in their own argument slots. So the array TrackCurve_GatherParams
 * then bubble-sorts holds every parameter at which the profile bends by more
 * than a unit away from a straight line -- the curve's own tessellation.
 *
 * Only 0x004dd648 is read, not 0x004dd644: GatherParams sets both to the same
 * curve and the two halves of the pair are used by different routines.
 *
 * FRAME: eight floats and nothing else. `3*c3`, `2*(3*c3)` and `best` never
 * get stack homes at all -- they live on the x87 stack across the whole body,
 * `best` for the entire root loop -- and two pairs share a slot ([esp+4] is
 * `2*c2` then the deviation, [esp+0x10] is the square root then z(u)). The
 * two winners are copied with INTEGER moves, which is VC6's float-to-float
 * copy when no arithmetic is wanted.
 *
 * WIP RESIDUAL: 110/110 instructions, 355/356 bytes, 104 of 109 aligned, and
 * the ONLY difference is which of two values VC6 leaves on the x87 stack:
 *   original   `fst m` (kept)  ... `fstp b2` then `fld b2 / fmul b2 / fxch`
 *   ours       `fstp m / fld m`  ... `fst b2` (kept) then `fmul b2`
 * -- i.e. the original keeps the chord slope in st across its own store and
 * evaluates `c1 - m` BEFORE `b2*b2`, and we do the mirror image. Measured and
 * inert: six spellings of the discriminant (operand order, `4.0f` first,
 * parenthesisation, negation), naming `(c1-m)` or the whole `4ac` term in a
 * local, swapping the `a3`/`b2` statements, computing `den` early, `a3` as a
 * repeated expression (78) and `b2` as one (111 instructions), the embedded
 * assignment `c = pa - (m = ...) * a` (102), a free volatile read of `g->c1`
 * (103), three positions for the `best` initialiser and block- versus
 * function-scope for the two loop temporaries. It is a scheduler choice inside
 * one expression, with the frame, every constant and both recursions exact.
 * ======================================================================== */
#pragma intrinsic(sqrt, fabs)

typedef struct TrackGeom {
    unsigned char pad00[0x24];
    float         c3;           /* +0x24  the height cubic, high order first */
    float         c2;           /* +0x28 */
    float         c1;           /* +0x2c */
    float         c0;           /* +0x30 */
} TrackGeom;

extern TrackGeom* g_tc_curve_b;                                 /* 0x004dd648 */
extern float*     g_tc_wp;                                      /* 0x004dd64c */
extern int        g_tc_n;                                       /* 0x004dd650 */

// WIP-FUNCTION: LEGOLAND 0x00421e90  (110/110 insns, 355/356 B, 104 of 109 aligned; two x87 stack-residency choices)
void TrackCurve_Refine(float a, float pa, float b, float pb)
{
    TrackGeom* g;
    float*     r;
    float      root[2];
    float      m, c, a3, b2, q, den;
    float      best, bx, by;
    int        i;

    best = 1.1754944e-038f;             /* FLT_MIN, 0x00800000 */
    m = (pb - pa) / (b - a);
    g = g_tc_curve_b;
    r = root;
    c = pa - m * a;
    a3 = g->c3 * 3.0f;
    b2 = g->c2 + g->c2;
    q = (float)sqrt(b2 * b2 - (g->c1 - m) * a3 * 4.0f);
    den = a3 + a3;
    root[0] = (q - b2) / den;
    root[1] = (-b2 - q) / den;
    for (i = 2; i != 0; i--) {
        if (*r >= a && *r <= b) {
            float zr = ((g->c3 * *r + g->c2) * *r + g->c1) * *r + g->c0;
            float dev = (float)fabs(zr - m * *r - c);

            if (dev > best) {
                bx = *r;
                by = zr;
                best = dev;
            }
        }
        r++;
    }
    if (fabs(best) <= 1.0)
        return;
    *g_tc_wp = bx;
    g_tc_wp++;
    g_tc_n++;
    TrackCurve_Refine(a, pa, bx, by);
    TrackCurve_Refine(bx, by, b, pb);
}
