/* LEGOLAND -- the LOG FLUME, part 7: the three remaining boat callees.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  None of
 * these functions is exported; every extent comes from the disassembly by
 * control flow (tools/audit.py).  Struct field OFFSETS, record sizes and
 * global addresses are load-bearing; the names are ours.  Types are declared
 * LOCALLY on purpose (legoland.h is owned elsewhere) and follow logflume4.c
 * and logflume6.c, which declare all three of these from the caller's side.
 *
 *   0x0040bb50  LFRun_FindWaitingBoat    40/40 insns,  87/87 B, exact
 *   0x004092b0  LFBoat_Heading           49/49 insns, 148/148 B, exact
 *   0x004112f0  LFPath_Point             69/69 insns, 210/210 B, exact
 *
 * =========================================================================
 * WHAT THIS FILE ADDS TO THE PICTURE  (spec notes for a browser runtime)
 * =========================================================================
 * THE BOAT WAITING AT THE STATION is found by scanning the run's four boat
 * slots for the one that is (a) sitting on the piece one step along the route
 * from the station piece (`run->f08->fwd`), (b) carrying nobody, and (c) NOT
 * moving.  logflume4.c's extern comment says the "moving" bit must be SET; it
 * is the other way round -- the original's `test byte ptr [ecx-4],al / je`
 * takes the success edge when the bit is CLEAR, which is also the only
 * reading that makes sense (a boat that is under way is not waiting).  That
 * comment is a documentation error in a caller's extern block, not a
 * behavioural one; the extern's TYPE is unchanged.
 *
 * THE BOAT'S FACING is the same eight-point compass in ODD numbers that
 * LFBoat_Advance computes (1 = north, 3 = east, 5 = south, 7 = west), but it
 * is derived TWO different ways depending on the piece's kind:
 *
 *   kind 1, 2 (straight, corner)  from the map-square delta to the piece
 *                                 ahead, exactly as LFBoat_Advance does it;
 *                                 a zero delta on both axes gives 1.
 *   kind 3    (the DROP)          straight off the piece's orientation,
 *                                 through a four-entry jump table:
 *                                 dir 0 -> 1, 1 -> 3, 2 -> 5, 3 -> 7.
 *   anything else                 1.
 *
 * So a drop piece does not consult its neighbour at all -- which it cannot,
 * because a drop's sub-route pieces do not carry a map-square delta that
 * describes the fall.  The `dir` -> heading map is the identity on the odd
 * compass (2*dir + 1), but the original spells it as a real switch with four
 * separate `return`s, and the jump table proves it.
 *
 * A PIECE'S BOAT PATH IS A POLYLINE SAMPLED AT CONSTANT ARC INDEX.  Each of
 * the six control polygons LFTrack_BuildGeometry builds (logflume4.c) is a
 * {count, points} pair; LFPath_Point maps a position `t` in 0..1 onto it by
 * splitting 0..1 into `n - 1` equal SEGMENTS:
 *
 *     step = 1.0f / (n - 1)
 *     i    = (int)floor(t / step)          the segment
 *     u    = (t - i * step) / step         where in that segment, 0..1
 *     out  = a + (b - a) * u               a = pts[i], b = pts[i+1]
 *
 * so it is a piecewise-LINEAR interpolation, not a spline: the four-point
 * quarter turns are drawn as three chords.  `reverse` walks the same polygon
 * from the far end -- a = pts[n-1-i], b = pts[n-2-i] -- which is how one
 * table serves a piece traversed in either direction.
 *
 * TWO ORIGINAL BUGS ARE VISIBLE HERE, both unreachable in practice:
 *  - `t == 1.0` exactly gives i == n-1 and reads pts[n] (one past the array),
 *    or pts[-1] reversed.  LFBoat_Advance keeps `z` strictly below 1.0 by
 *    subtracting a whole piece as soon as it passes, so it never happens.
 *  - a one-point polygon would divide by zero in `1.0f / (n - 1)`.  All six
 *    tables have 2 or 4 points.
 * ========================================================================= */

#include <math.h>

/* ---- shared types (same offsets as logflume4.c / logflume6.c) ----------- */
typedef struct Pos  { int x; int y; } Pos;
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

typedef struct RiderNode RiderNode;
typedef struct LFRun     LFRun;

/* The placed-piece record (0x38 bytes; the full struct is in logflume.c). */
typedef struct LFPiece {
    struct LFPiece* next;       /* +0x00 */
    struct LFPiece* prev;       /* +0x04 */
    struct LFPiece* fwd;        /* +0x08 next piece ALONG THE ROUTE */
    struct LFPiece* back;       /* +0x0c */
    unsigned int    flags;      /* +0x10 */
    BPosW           sq;         /* +0x14 the piece's map square */
    unsigned char   pad16[2];
    int             kind;       /* +0x18 1 straight, 2 corner, 3 drop */
    int             dir;        /* +0x1c orientation / variant, 0..3 */
    void*           def;        /* +0x20 */
    LFRun*          run;        /* +0x24 */
    unsigned char   pad28[0x38 - 0x28];
} LFPiece;

/* One boat: 0x24 bytes, four of them from LFRun+0x40 on. */
typedef struct LFBoat {
    int           state;        /* +0x00 coast counter */
    unsigned int  flags;        /* +0x04 1 = moving, 2 = falling, 4 = splash */
    RiderNode*    rider;        /* +0x08 who is aboard */
    unsigned char pad0c[8];
    LFPiece*      piece;        /* +0x14 the route piece it is over */
    float         z;            /* +0x18 how far along that piece, 0..1 */
    unsigned char pad1c[0x24 - 0x1c];
} LFBoat;

struct LFRun {
    LFRun*        next;         /* +0x00 */
    unsigned int  flags;        /* +0x04 */
    LFPiece*      f08;          /* +0x08 the station piece */
    unsigned char pad0c[0x3c - 0x0c];
    int           boat_count;   /* +0x3c how many boats this run runs */
    LFBoat        boats[4];     /* +0x40 .. +0xcf */
    int           piece_count;  /* +0xd0 */
};                              /* 0xd4 */

/* =========================================================================
 * 0x0040bb50 -- FIND THE BOAT WAITING AT THE STATION.
 *
 * LFRun_Tick (logflume4.c) calls this on the tick the flume steps, unless a
 * boat is already loading; the winner becomes `run->boat` and takes the
 * rider at the head of the queue.
 *
 * The three tests are made in this order and none of them is short-circuited
 * away by the compiler, so the boat must be on `run->f08->fwd` -- the piece
 * one step along the route from the station -- carry nobody, and have its
 * "moving" bit clear.  `*out` is written on BOTH exits (0 when there is no
 * such boat), which is why LFRun_Tick's own `boat = 0;` is redundant.
 *
 * LEVERS:
 *  - `test byte ptr [ecx-4], al` with `al == 1` is not a mask spelling: the
 *    `mov eax,1` in the loop preheader is the `return 1` value materialised
 *    early, and VC6 reuses it as the test's operand.  So the success arm's
 *    constant and the flag mask are ONE object, and any body that returns a
 *    different constant loses the fused form.
 *  - `b` must be a NAMED pointer.  The strength-reduced cursor runs at
 *    +0x08 (the `rider` field) and `lea edi,[ecx-8]` re-derives the record
 *    address at the TOP of every iteration, before the compares -- which is
 *    where the source names it, not where the address is used.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x0040bb50
int LFRun_FindWaitingBoat(LFRun* run, LFBoat** out)
{
    LFBoat* b;
    int     i;

    for (i = 0; i < run->boat_count; i++) {
        b = &run->boats[i];
        if (b->piece == run->f08->fwd && b->rider == 0 && !(b->flags & 1)) {
            *out = b;
            return 1;
        }
    }
    *out = 0;
    return 0;
}

/* =========================================================================
 * 0x004092b0 -- WHICH WAY IS THE BOAT FACING?
 *
 * Called by LFBoat_Draw (logflume6.c) to point the rider's 3D model.  See
 * the file header for the two derivations; the constants are the odd compass
 * LFBoat_Advance uses, so a boat's model and its motion agree.
 *
 * LEVERS (exact first try; the shapes were read off the dispatch):
 *  - **A switch whose cases 1 and 2 SHARE a body and whose case 3 has its
 *    own lowers to a RANGE chain, not a jump table.**  `test eax,eax / jle
 *    default`, `cmp eax,2 / jle <1,2>`, `cmp eax,3 / jne default` is VC6
 *    clustering the sorted case values into [1..2] and [3..3]; case 3, the
 *    last cluster, is the fall-through and the shared 1/2 body is laid out
 *    after it.  An `if (kind == 1 || kind == 2)` chain gives `cmp eax,1 / je`
 *    instead, so the range test is the tell that it was a switch.
 *  - The INNER switch on `p->dir` is a real four-entry jump table bounded by
 *    `cmp ecx,eax / ja` against the register still holding the compared 3 --
 *    VC6 reuses the case constant as the bound, which is why the check is
 *    unsigned.  Its cases 1 and 2 get their own blocks and cases 3 and 0 are
 *    cross-jumped into the delta block's `return 7` and the function's shared
 *    `return 1`; writing all four in natural order is what produces that.
 *  - The four `if (dx/dy < / > 0) return K;` are the SAME shape
 *    LFBoat_Advance uses for its `heading` assignments, with `mov eax,5`
 *    hoisted above its own `jg` so the dy == 0 path falls into `return 1`.
 *  - `t.b.x` / `t.b.y` on a `BPosW` LOCAL again emits the unaligned
 *    `mov eax,[esp] / and eax,0FFh` + `mov ecx,[esp+1] / and ecx,0FFh` pair
 *    while the other square is read as two byte loads straight out of the
 *    piece -- the packed-by-value idiom for a local, transferred verbatim
 *    from LFBoat_Advance.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x004092b0
int LFBoat_Heading(LFBoat* b)
{
    LFPiece* p = b->piece;
    BPosW    t;
    int      dx, dy;

    switch (p->kind) {
    case 1:
    case 2:
        t.w = p->fwd->sq.w;
        dx = t.b.x - p->sq.b.x;
        dy = t.b.y - p->sq.b.y;
        if (dx < 0) return 7;
        if (dx > 0) return 3;
        if (dy < 0) return 1;
        if (dy > 0) return 5;
        break;
    case 3:
        switch (p->dir) {
        case 0: return 1;
        case 1: return 3;
        case 2: return 5;
        case 3: return 7;
        }
        break;
    }
    return 1;
}

/* ---- the six boat paths through one tile (logflume4.c) ----------------- */
typedef struct LFPath {
    int  n;                     /* +0x00 how many control points */
    Pos* pts;                   /* +0x04 */
} LFPath;

/* =========================================================================
 * 0x004112f0 -- SAMPLE A CONTROL POLYGON AT `t`.
 *
 * See the file header for the maths.  The point comes back as a Pos in
 * eax:edx (VC6's 8-byte struct return).
 *
 * LEVERS (69/69, 210/210 bytes; the first cut was 64 instructions with the
 * first divergence at index 2):
 *  - **`j = i + 1` must be a NAMED local computed BEFORE the branch.**  It is
 *    used one way in each arm (`pts[j]` forward, `n - j - 1` reversed), and
 *    spelling it inline in both arms lets VC6 fold `+1` into the forward
 *    arm's displacement (`[ecx+eax*8+8]`) and re-derive it in the reverse
 *    one.  The name is worth the original's `lea edi,[eax+1]` scheduled
 *    between the `test` and the `jne` -- AND a fourth callee-saved push:
 *    with `j` inline only four values are live across the join and VC6 needs
 *    ebx/esi/edi, with it five are and `ebp` appears.  A missing push is a
 *    live-value deficit; look for a named intermediate before hunting
 *    allocation.
 *  - **The reversed index must be `n - i - 1`, not `n - 1 - i`.**  Written
 *    with the constant first VC6 has to materialise `n - 1` (which is already
 *    spoken for as the step's divisor); written variable-first it subtracts
 *    into a copy of `n` and folds the `- 1` into the `-8` displacement, which
 *    is what makes ONE `path->n` read serve both arms.
 *  - **`if (reverse == 0) { forward } else { reversed }`.**  The original's
 *    fall-through arm is the FORWARD one, so the test has to be spelled
 *    against zero; `if (reverse) { reversed } else { forward }` emits `je`
 *    and swaps the two blocks.  (The recorded negated-condition rule, used
 *    to place a block without touching the condition.)
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x004112f0
Pos LFPath_Point(LFPath* path, float t, int reverse)
{
    Pos   a, b, r;
    float step, u;
    int   i, j;

    step = 1.0f / (path->n - 1);
    i = (int)floor(t / step);
    j = i + 1;
    if (reverse == 0) {
        a = path->pts[i];
        b = path->pts[j];
    } else {
        a = path->pts[path->n - i - 1];
        b = path->pts[path->n - j - 1];
    }
    u = (t - i * step) / step;
    r.x = (int)((b.x - a.x) * u + a.x);
    r.y = (int)((b.y - a.y) * u + a.y);
    return r;
}
