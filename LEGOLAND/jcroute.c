/* LEGOLAND -- JUNGLE CRUISE routing: the two river walks.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Neither
 * function is exported; both extents were taken from the disassembly by
 * control flow (tools/audit.py).  Struct field OFFSETS, record sizes and
 * global addresses are load-bearing; the names are ours.  Types are declared
 * LOCALLY on purpose (legoland.h is owned elsewhere) and the JcWater shape is
 * the one LEGOLAND/junglecruise.c and ridecb2.c already use -- see the note
 * on +0x08/+0x14 below, where this file's evidence names two fields that
 * junglecruise.c could only guess at from the outside.
 *
 * =========================================================================
 * THE RIVER, IN ONE PARAGRAPH
 *
 * A jungle cruise river is a set of WATER SQUARES (g_jc_water, 0x0062fd2c,
 * 0x1c bytes each).  Each square covers a 5x5 block of map cells centred on
 * its own map coordinate, so every coordinate here steps by FIVE: the four
 * neighbours of (x, y) are (x, y-5) north, (x+5, y) east, (x, y+5) south and
 * (x-5, y) west, and the bitmap of which ones exist is the square's `links`
 * at +0x04 (1 N, 2 E, 4 S, 8 W).  Every square carries the map square of the
 * STATION that owns it at +0x02, so two rivers laid side by side never join.
 *
 * =========================================================================
 * TWO DIFFERENT WALKS OVER THE SAME GRAPH
 *
 * The ride runs two completely separate traversals of the river, and they
 * use DIFFERENT fields of JcWater, which is why both can be in flight
 * without interfering:
 *
 *   1. JungleCruise_TraceRoute (0x00437260) -- a RECURSIVE depth-first
 *      search for a path from (x,y) to (tx,ty).  It marks squares in +0x0c
 *      and reports success by storing 1 through the `route` out-parameter.
 *      Its caller JungleCruise_BuildRoute clears every +0x0c first.
 *
 *   2. JungleCruise_StepRoute (0x00437440) -- one PLY of a breadth-first
 *      flood from the station's route-end square, over the two-slot work
 *      list g_jc_route_cur/g_jc_route_next (0x0062fd30/0x0062fd34).  It
 *      marks squares by writing the predecessor into +0x18, records the ply
 *      depth in +0x08 and chains the next frontier through +0x14.  Its
 *      caller JungleCruise_RebuildRoute clears every +0x18 for the station
 *      and then calls this once per ply until the frontier is empty.
 *
 * So for the flood pass:  +0x08 = ply depth (an int, w->dist + 1),
 * +0x14 = the frontier chain link, +0x18 = the predecessor square.  In
 * junglecruise.c those three are called `rlink`, `f14` and `f18` and typed
 * pointer/int/pointer from the outside; the writes here show +0x08 is the
 * integer and +0x14 the pointer, i.e. the two are typed the other way round.
 * (No conflict: neither file's codegen depends on it -- RebuildRoute only
 * stores 0 into both -- so junglecruise.c is left exactly as it is.)
 *
 * NOTE ON THE FLOOD, and it is a genuine oddity of the original: the START
 * square's +0x18 is never stamped by RebuildRoute before the first ply, and
 * this function only stamps a square it REACHES.  So on the second ply the
 * start square looks unvisited to its own neighbours and is re-enqueued with
 * depth 2 and a predecessor pointing back at one of them.  That also
 * overwrites its +0x14 while it is not on any list, which is harmless only
 * because the first ply has already read it.  Reproduced, not fixed.
 *
 * =========================================================================
 * WHAT IS IN THIS FILE
 *
 *   0x00437440  JungleCruise_StepRoute    one ply of the breadth-first flood
 *   0x00437260  JungleCruise_TraceRoute   the recursive route search
 *
 * =========================================================================
 * TWO EXTERN-PROTOTYPE TYPES DIFFER FROM junglecruise.c ON PURPOSE.  Extern
 * prototype types are caller-side codegen levers, so junglecruise.c is left
 * exactly as it is; these are the types THIS file's disassembly needs:
 *
 *   - StepRoute's parameter is `unsigned short`, not `int`.  The original
 *     loads it with `mov cx, word ptr [esp+0x34]` and keeps it in cx across
 *     all four neighbour tests; an `int` parameter compared as
 *     `(unsigned short)id` loads the full dword and is the function's ONLY
 *     mismatch (107/108).  junglecruise.c declares it `int` and its caller
 *     (JungleCruise_RebuildRoute) is exact that way -- both are right.
 *   - TraceRoute's `route` is `int*`, not `void**`.  The original compares
 *     `*route` against 1 and stores 1 into it, so it is a success FLAG that
 *     JungleCruise_BuildRoute happens to hold in a `void*` and return.
 * ========================================================================= */

/* ---- shared types (same offsets as junglecruise.c / ridecb2.c) ---------- */

/* A packed 2-byte map square, and its 16-bit view. */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union BPosW { unsigned short w; BPos b; } BPosW;

/* One WATER square of the river.  0x1c bytes; see the header for the two
 * disjoint sets of walk fields. */
typedef struct JcWater {
    BPosW           pos;            /* +0x00  this square */
    BPosW           owner;          /* +0x02  the station that owns the river */
    int             links;          /* +0x04  1 N, 2 E, 4 S, 8 W */
    int             dist;           /* +0x08  flood: ply depth */
    int             mark;           /* +0x0c  trace: already visited */
    struct JcWater* next;           /* +0x10  list link */
    struct JcWater* qnext;          /* +0x14  flood: next square of the frontier */
    struct JcWater* prev;           /* +0x18  flood: predecessor square */
} JcWater;                          /* 0x1c */

extern JcWater* g_jc_route_cur;     /* 0x0062fd30 */
extern JcWater* g_jc_route_next;    /* 0x0062fd34 */

/* The river square covering a map square, or NULL. */
extern JcWater* JcWater_FindAt(int x, int y);                /* 0x004371b0 */

/* =========================================================================
 * 0x00437440 -- JungleCruise_StepRoute: advance the breadth-first flood by
 * one ply.
 *
 * Consumes the frontier at g_jc_route_cur, and for each of its squares looks
 * up the four neighbours by coordinate -- NOT through the `links` bitmap,
 * which this walk ignores entirely; a square that merely EXISTS five cells
 * away and belongs to the same station is treated as connected.  (The trace
 * walk below does honour `links`.  The two disagree; reproduced.)  A
 * neighbour joins the next frontier when it belongs to station `id` and has
 * no predecessor yet.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00437440
void JungleCruise_StepRoute(unsigned short id)
{
    JcWater* w = g_jc_route_cur;

    while (w) {
        JcWater* n = JcWater_FindAt(w->pos.b.x, w->pos.b.y - 5);
        JcWater* e = JcWater_FindAt(w->pos.b.x + 5, w->pos.b.y);
        JcWater* s = JcWater_FindAt(w->pos.b.x, w->pos.b.y + 5);
        JcWater* t = JcWater_FindAt(w->pos.b.x - 5, w->pos.b.y);

        if (n && n->owner.w == id && n->prev == 0) {
            n->prev = w;
            n->dist = w->dist + 1;
            n->qnext = g_jc_route_next;
            g_jc_route_next = n;
        }
        if (e && e->owner.w == id && e->prev == 0) {
            e->prev = w;
            e->dist = w->dist + 1;
            e->qnext = g_jc_route_next;
            g_jc_route_next = e;
        }
        if (s && s->owner.w == id && s->prev == 0) {
            s->prev = w;
            s->dist = w->dist + 1;
            s->qnext = g_jc_route_next;
            g_jc_route_next = s;
        }
        if (t && t->owner.w == id && t->prev == 0) {
            t->prev = w;
            t->dist = w->dist + 1;
            t->qnext = g_jc_route_next;
            g_jc_route_next = t;
        }
        w = w->qnext;
    }
}

/* Two FREE volatile reads, both at sites where the original loads from the
 * variable's memory home anyway, so neither costs an instruction.  They are
 * diagnostics, not claims about the source: they pin `w` and `owner` to their
 * frame homes, which is where the original keeps them, and without them VC6
 * hands those two the callee-saved registers the original spends on tx/ty.
 * See the residual note below -- they buy the exact byte length, not the
 * match. */
#define W_MEM      (*(JcWater* volatile*)&w)
#define OWNER_MEM  (*(BPosW*   volatile*)&owner)

/* =========================================================================
 * 0x00437260 -- JungleCruise_TraceRoute: the recursive route search.
 *
 * Depth-first from (x,y) towards (tx,ty), through the `links` bits and only
 * over squares owned by *owner.  `route` is the shared success flag: every
 * level tests it on entry and the level that reaches the target sets it, so
 * the whole recursion unwinds without doing any more work.  Squares are
 * marked in +0x0c on the way DOWN and never unmarked, so this finds *a*
 * route, not the shortest one, and it can strand itself -- the caller
 * (JungleCruise_BuildRoute) clears every mark before the top-level call.
 *
 * The WEST arm is in tail position, so VC6 SP3 eliminates the tail call: the
 * function's own body becomes the loop (x -= 5, back to the head at
 * 0x00437283), the entry guard is re-tested as the loop latch, and the two
 * dead register copies at 0x0043738e/0x00437390 are the tx/ty argument copies
 * of the call that is no longer there.  That is why this body ends in a real
 * `ret` and not a `jmp`, and why the frame is entered once, outside the loop.
 *
 * WHY THIS IS STILL A WIP -- the residual is ONE allocation fact, and it is a
 * compiler PHASE-ORDERING difference, not a source shape.  The original puts
 * its four callee-saved registers on the four int parameters
 * (esi=x, edi=y, ebp=tx, ebx=ty) and leaves `w`, `owner` and `route` in
 * memory homes, reloading each at every use (`w` lives in the dead arg3 slot
 * [esp+0x1c]).  Every spelling of this function compiled here does the
 * opposite: the dereferenced pointers win the registers and tx/ty spill.
 *
 * That was isolated with a four-line probe rather than guessed (the probes are
 * in the lane notes).  A loop-free function with 4 int params + 2 dereferenced
 * pointer params + 1 pointer local from a call reproduces the ORIGINAL's
 * allocation exactly -- ints first, pointers to memory -- even though the
 * pointers have MORE references.  Add a self-tail-call to the same probe and
 * VC6 turns it into a loop at IR level, and the ranking inverts: the
 * dereferenced pointers take the registers, tx/ty spill, and a secondary
 * induction variable for `x + 5` appears in a stack slot (three instructions
 * the original does not have).  So the original's tail-call-to-loop
 * conversion happened AFTER register allocation, and ours happens before it.
 * No source spelling reaches the other side of that: an explicit `while`,
 * a `goto`, `x -= 5` vs a `nx` temp, `register`, `static`, splitting the
 * owner/route webs into extra locals, and an inlined per-direction helper are
 * all on our side of it (28% - 53%); blocking the conversion outright (any
 * dead trailing statement does it) restores the original's esi/edi/ebp
 * assignment but then emits a real call and epilogue instead of the loop.
 *
 * WHAT THE COMMITTED BODY BUYS, measured by audit.py (all variants are
 * 130/130 instructions; the numbers are audit strict mismatch / byte length
 * against the original's 339):
 *     plain tail-recursive C, no shims        116 / 352
 *     plain `while (*route != 1)` loop        113 / 345
 *     + free volatile on `w`                  111 / 333
 *     + free volatile on `w` and `owner`      105 / 339   <- committed
 *     + free volatile on `w` and `route`      112 / 346  (loop form: 101/340)
 * The committed pair is the only one that is BYTE-EXACT, i.e. every encoding
 * is the right size and nothing is a wrong type, immediate or addressing
 * form.  Both shims are free reads (the original loads from those two homes
 * at every one of those sites), and neither closes the allocation above.
 *
 * FIRST DIVERGING INDEX 0 (`mov eax,[esp+0x18]` vs `push ebx`: the original
 * schedules the `route` load ahead of the callee-saved pushes because route
 * has no register).  §6B class: ALLOCATION (strict 113 / register-blind 85 on
 * an index alignment; the register-blind residue is the induction variable and
 * the reload/rematerialise pattern that follow from the same fact).
 * ========================================================================= */

// WIP-FUNCTION: LEGOLAND 0x00437260  (130/130 insns, 339/339 B -- byte-exact -- 105 by audit; ALLOCATION: the original gives its four callee-saved registers to x/y/tx/ty and homes w/owner/route in memory, ours does the reverse because our tail-call-to-loop conversion runs BEFORE register allocation and the original's after; first diverging index 0)
void JungleCruise_TraceRoute(int x, int y, int tx, int ty, BPosW* owner, int* route)
{
    JcWater* w;
    JcWater* p;

    if (*route == 1)
        return;
    w = JcWater_FindAt(x, y);
    if (!w)
        return;
    if (w->owner.w != OWNER_MEM->w)
        return;
    if (x == tx && y == ty) {
        *route = 1;
        return;
    }
    w->mark = 1;
    if (w->links & 1) {
        p = JcWater_FindAt(x, y - 5);
        if (p && p->mark == 0)
            JungleCruise_TraceRoute(x, y - 5, tx, ty, OWNER_MEM, route);
    }
    if (W_MEM->links & 2) {
        p = JcWater_FindAt(x + 5, y);
        if (p && p->mark == 0)
            JungleCruise_TraceRoute(x + 5, y, tx, ty, OWNER_MEM, route);
    }
    if (W_MEM->links & 4) {
        p = JcWater_FindAt(x, y + 5);
        if (p && p->mark == 0)
            JungleCruise_TraceRoute(x, y + 5, tx, ty, OWNER_MEM, route);
    }
    if (W_MEM->links & 8) {
        /* `nx` is not cosmetic: with `x - 5` spelled twice (or `x -= 5`) x
         * becomes a basic induction variable of the loop VC6 makes out of the
         * tail call, and a secondary induction variable for `x + 5` appears in
         * a stack slot -- three instructions the original does not have.
         * Assigning the step through a temporary breaks the recognition. */
        int nx = x - 5;
        p = JcWater_FindAt(nx, y);
        if (p && p->mark == 0)
            JungleCruise_TraceRoute(nx, y, tx, ty, OWNER_MEM, route);
    }
}
