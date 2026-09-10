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

/* =========================================================================
 * 0x00437260 -- JungleCruise_TraceRoute: recursive depth-first route search.
 *
 * Follow the links bitmap, accepting only squares owned by *owner.  The
 * shared success flag stops further work after reaching the target.  Mark
 * each square on entry, never unmark it; BuildRoute clears the marks before
 * the search.  This finds a route, not necessarily the shortest one.
 *
 * Scope H (2026-09-05): exact, 130 instructions / 339 bytes.  The source
 * and target coordinates are BOTH 8-byte aggregates passed by value, and
 * recursive steps are assembled in one non-address-taken aggregate local.
 * Two int parameters have the same stack ABI but different compiler symbol
 * identities.  The aggregate form gives x/ESI, y/EDI, target.x/EBP and
 * target.y/EBX; the water pointer reuses target.x's dead argument home.
 * The west tail call still becomes a loop, including its two dead target
 * copies.  Thus the previous phase-ordering retirement diagnosis was false.
 *
 * Diagnostic controls: target aggregate alone, no volatile shims, gives 68
 * audit mismatches (first 16); adding the source/step aggregates closes all
 * of them.  A shared Pos type and either step-field assignment order are
 * exact.  Both former volatile macros are unnecessary and removed.
 *
 * The tagged parameter type is local to this definition; caller translation
 * units deliberately retain their existing six-scalar prototypes.  Their
 * pushes occupy the same 24 bytes.  route is an int* success flag here;
 * junglecruise.c holds it as void*, an existing ABI-compatible divergence.
 * Full-file audit: 2 exact / 0 WIP; /W3 clean.  Evidence and controls:
 * scratchpad/scope-h/jungle2/trace-promote-audit.txt and trace_*.c.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00437260
void JungleCruise_TraceRoute(struct JcRoutePos { int x; int y; } here,
                            struct JcRoutePos target, BPosW* owner, int* route)
{
    JcWater* w;
    JcWater* p;
    struct JcRoutePos step;

    if (*route == 1)
        return;
    w = JcWater_FindAt(here.x, here.y);
    if (!w)
        return;
    if (w->owner.w != owner->w)
        return;
    if (here.x == target.x && here.y == target.y) {
        *route = 1;
        return;
    }
    w->mark = 1;
    if (w->links & 1) {
        p = JcWater_FindAt(here.x, here.y - 5);
        if (p && p->mark == 0) {
            step.x = here.x;
            step.y = here.y - 5;
            JungleCruise_TraceRoute(step, target, owner, route);
        }
    }
    if (w->links & 2) {
        p = JcWater_FindAt(here.x + 5, here.y);
        if (p && p->mark == 0) {
            step.x = here.x + 5;
            step.y = here.y;
            JungleCruise_TraceRoute(step, target, owner, route);
        }
    }
    if (w->links & 4) {
        p = JcWater_FindAt(here.x, here.y + 5);
        if (p && p->mark == 0) {
            step.x = here.x;
            step.y = here.y + 5;
            JungleCruise_TraceRoute(step, target, owner, route);
        }
    }
    if (w->links & 8) {
        /* Pass the stepped coordinate as an aggregate, just like the other
         * directions.  VC6 turns this west call into the loop back-edge. */
        int nx = here.x - 5;
        p = JcWater_FindAt(nx, here.y);
        if (p && p->mark == 0) {
            step.x = nx;
            step.y = here.y;
            JungleCruise_TraceRoute(step, target, owner, route);
        }
    }
}
