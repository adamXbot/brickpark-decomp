/* LEGOLAND -- the LOG FLUME, part 6: the neighbour probe's inner loop, the
 * cursor hit-test, the save side of the station queue, and THE BOAT PHYSICS
 * (spacing, spline advance, the drop/fall state machine and the boat's draw
 * pass).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  None of
 * these functions is exported; every extent comes from the disassembly by
 * control flow (tools/audit.py).  Struct field OFFSETS, record sizes and
 * global addresses are load-bearing; the names are ours.  Types are declared
 * LOCALLY on purpose (legoland.h is owned elsewhere) and follow logflume.c,
 * logflume2.c, logflume4.c and logflume5.c.
 *
 * NAMES: logflume5.c declares three of these by address only, as
 * `LFRun_BoatHasRoom`, `LFBoat_Advance` and `LFBoat_Fall`.  They are named here
 * `LFRun_BoatHasRoom`, `LFBoat_Advance` and `LFBoat_Fall` respectively; the
 * parameter lists are unchanged, so only the three extern NAMES in
 * logflume5.c need updating when that file is next touched.
 * LFAnim_SaveRef's second parameter is typed `LFQueue*` here where
 * logflume.c declares `int*` -- the same divergence logflume5.c already
 * carries for its mirror image LFAnim_LoadRefs.
 *
 * =========================================================================
 * WHAT THIS FILE ADDS TO THE PICTURE  (spec notes for a browser runtime)
 * =========================================================================
 * THE BOAT IS A POINT ON A ROUTE OF PIECES, DRAWN THROUGH A SPLINE.
 * Everything about a log-flume boat is four numbers on the 0x24-byte LFBoat
 * record: which piece of the route it is over (+0x14), how far along that
 * piece it is as a float 0..1 (+0x18), how much that advances per step
 * (+0x20) and a flag word (+0x04).  One step of the ride is:
 *
 *   LFBoat_Step  (0x0040bbb0, logflume5.c) -- every third frame, per boat
 *     |- falling?          -> LFBoat_Fall     (0x00411810, here)
 *     |- may it advance?   -> LFRun_BoatHasRoom (0x0040bab0, here)
 *     `- move it           -> LFBoat_Advance  (0x00411680, here)
 *
 * and the draw side is LFPiece_HasRider -> LFBoat_Draw (0x0040ae90, here).
 *
 * SPACING.  A boat may not step if another boat is more than 0.0 and less
 * than 0.8 of a piece AHEAD of it, measured in piece units along the route
 * (`other->z - z` on the same piece, `other->z + 1.0 - z` on the piece
 * ahead).  Only those two pieces count, and a boat BEHIND (a negative gap)
 * never blocks, so the rule is local and one-sided.
 *
 * THE CURVE THROUGH A TILE.  A piece's kind and orientation select one of the
 * six control polygons LFTrack_BuildGeometry builds (logflume4.c); the
 * position along the piece is the spline parameter, and the sampled point is
 * the boat's offset inside the tile.  The polygon is REVERSED when the route
 * runs through the tile the other way, which is decided by an eight-point
 * compass in ODD numbers (1 N, 3 E, 5 S, 7 W) computed from the map-square
 * delta to the piece ahead:
 *
 *     kind 1 (straight)  dir 1 -> br_tl (reverse heading E)
 *                        dir 0 -> tr_bl (reverse heading N)
 *     kind 2 (corner)    dir 3 -> tl_tr (reverse heading W)
 *                        dir 0 -> tr_br (reverse heading N)
 *                        dir 1 -> br_bl (reverse heading E)
 *                        dir 2 -> bl_tl (reverse heading S)
 *
 * THE DROP IS A TIMELINE IN ONE SCALAR.  A drop is a set piece with its own
 * sub-route (parent +0x30 = first, +0x34 = last).  While a boat is falling,
 * `t = (index of its piece in that sub-route) + z` drives everything:
 * frame 0 below 2, ramping to 120 over 2..4 at speed 0.05, held at 120 over
 * 4..5 at speed 0.1, ramping back to 0 over 5..10 while the speed grows by
 * 0.05 EVERY STEP, and 0 from 10 on.  The spray flag (boat bit 2) is up for
 * 5 <= t < 9.  On the single step that first crosses 10 the RUN starts its
 * splash: LFRun +0x04 bit 1 set, +0x24 (the splash frame cursor) zeroed and
 * +0x28 loaded with lf_splash.lls's frame count -- which reconciles
 * logflume.c's `splash_frame` with logflume5.c's `hold`/`hold_time`.
 *
 * DRAWING.  Two sprites per boat, swapped by the spray flag
 * (lf_barrel/lf_barrel_m normally, lf_barrel1/barrelmatte while splashing);
 * the matte is slaved to the barrel's animation frame.  A boat overhangs into
 * the neighbouring tile, so when the piece being drawn is not the piece the
 * boat is on, the clip rectangle is narrowed to the half of that tile facing
 * the boat.  The rider, if its bloke is flagged visible and has a 3D model,
 * is placed at the boat's point plus half the sprite width and three quarters
 * of its height and rendered immediately.
 *
 * THE PROBE STEPS BY A FLUME CELL, NOT A MAP SQUARE.  LFTrack_FillNeighbours
 * moves by the class footprint's width and height, which is why two adjacent
 * flume pieces are that far apart on the map.
 *
 * THE STATION QUEUE'S SAVE FORMAT is the exact mirror of LFAnim_LoadRefs:
 * a path point count and its points, then a rider count and the riders'
 * ordinals.  The count is not stored anywhere -- the list is walked twice.
 * ========================================================================= */

/* ---- shared types (same offsets as logflume2.c / logflume5.c) ----------- */
typedef struct Pos  { int x; int y; } Pos;
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

typedef struct FootPart FootPart;
typedef struct Footprint {
    int       v[4];             /* +0x00 */
    FootPart* parts;            /* +0x10 */
} Footprint;                    /* 0x14 */

typedef struct LFPiece LFPiece;
typedef struct LFRun   LFRun;

/* ---- module globals ---------------------------------------------------- */
extern LFPiece*  g_lf_nb[4];            /* 0x004cbe20  the neighbour array */
extern Footprint g_lf_footprint;        /* 0x004b4728  the flume cell rect */

extern LFPiece* LFTrack_FindPiece(const BPos* sq);               /* 0x00408f30 */

/* =========================================================================
 * 0x00409360 -- fill the four-slot neighbour array around one square.
 *
 * LFTrack_ProbeNeighbours (logflume2.c) clears g_lf_nb and calls this; the
 * slots are filled NORTH, EAST, SOUTH, WEST from the four map squares one
 * flume CELL away in each direction.  A flume cell is not one map square:
 * its size is the class footprint's width and height
 * (g_lf_footprint.v[2] - v[0], v[3] - v[1]), so the probe steps by a whole
 * cell and two adjacent flume pieces are that far apart on the map.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x00409360
void LFTrack_FillNeighbours(BPos sq)
{
    int  dx = g_lf_footprint.v[2] - g_lf_footprint.v[0];
    int  dy = g_lf_footprint.v[3] - g_lf_footprint.v[1];
    BPos c;

    c.x = sq.x;
    c.y = (unsigned char)(sq.y - dy);
    g_lf_nb[0] = LFTrack_FindPiece(&c);
    c.x = (unsigned char)(sq.x + dx);
    c.y = sq.y;
    g_lf_nb[1] = LFTrack_FindPiece(&c);
    c.x = sq.x;
    c.y = (unsigned char)(sq.y + dy);
    g_lf_nb[2] = LFTrack_FindPiece(&c);
    c.x = (unsigned char)(sq.x - dx);
    c.y = sq.y;
    g_lf_nb[3] = LFTrack_FindPiece(&c);
}

/* ---- the boat record and the run it belongs to (logflume5.c) ------------ */
typedef struct RiderNode RiderNode;

/* One boat: 0x24 bytes, four of them from LFRun+0x40 on. */
typedef struct LFBoat {
    int           state;        /* +0x00  coast counter, counts DOWN */
    unsigned int  flags;        /* +0x04  1 = moving, 2 = falling, 4 = splash */
    RiderNode*    rider;        /* +0x08  who is aboard */
    int           ox;           /* +0x0c  draw offset within the piece, x */
    int           oy;           /* +0x10                              , y */
    LFPiece*      piece;        /* +0x14  the route piece it is over */
    float         z;            /* +0x18  how far along that piece, 0..1 */
    int           frame;        /* +0x1c  splash/tilt animation frame */
    float         speed;        /* +0x20  z per step */
} LFBoat;

typedef struct LFQueuePath {
    int   count;                /* +0x00  how many people fit on the path */
    char* pts;                  /* +0x04  count * 12 bytes, right behind it */
} LFQueuePath;

typedef struct LFQueueNode {
    struct LFQueueNode* next;   /* +0x00 */
    int                 rider;  /* +0x04 */
} LFQueueNode;

typedef struct LFQueue {
    LFQueuePath* path;          /* +0x00 */
    LFQueueNode* head;          /* +0x04 */
    LFQueueNode* tail;          /* +0x08 */
} LFQueue;

struct LFRun {
    LFRun*        next;         /* +0x00 */
    unsigned int  flags;        /* +0x04  2 = a splash is playing */
    LFPiece*      f08;          /* +0x08 */
    LFPiece*      f0c;          /* +0x0c */
    LFPiece*      pieces;       /* +0x10  head of this run's piece TREE */
    BPosW         sq;           /* +0x14  the station's map square */
    unsigned char pad16[2];
    LFPiece*      f18;          /* +0x18 */
    int           frame;        /* +0x1c */
    int           step;         /* +0x20 */
    int           splash_frame; /* +0x24  frame the drop splash sits on */
    int           splash_len;   /* +0x28  how many frames it has */
    LFQueue       queue;        /* +0x2c  path, head, TAIL */
    LFBoat*       boat;         /* +0x38  the boat at the station */
    int           boat_count;   /* +0x3c  how many boats this run runs */
    LFBoat        boats[4];     /* +0x40 .. +0xcf */
    int           piece_count;  /* +0xd0 */
};                              /* 0xd4 */

struct LFPiece {
    LFPiece*      next;         /* +0x00  next piece of the same list */
    LFPiece*      prev;         /* +0x04 */
    LFPiece*      fwd;          /* +0x08  next piece ALONG THE ROUTE */
    LFPiece*      back;         /* +0x0c  previous piece along the route */
    unsigned int  flags;        /* +0x10 */
    BPosW         sq;           /* +0x14  the piece's map square */
    unsigned char pad16[2];
    int           kind;         /* +0x18 */
    int           dir;          /* +0x1c  orientation / variant, 0..3 */
    void*         def;          /* +0x20  RideDef* */
    LFRun*        run;          /* +0x24 */
    LFPiece*      parent;       /* +0x28  the set piece this belongs to */
    LFPiece*      sub;          /* +0x2c  head of this piece's sub-list */
    LFPiece*      end_a;        /* +0x30  the sub-piece at end A */
    LFPiece*      end_b;        /* +0x34  the sub-piece at end B */
};

/* 0x0040b210 (not exported): does boat `b` draw on piece `p` this frame? */
extern int LFBoat_IsOnPiece(LFBoat* b, LFPiece* p);              /* 0x0040b210 */

/* =========================================================================
 * 0x0040c2e0 -- is any of the run's boats over this piece?
 *
 * logflume2.c names it LFPiece_HasCursor because its one caller
 * (LFPiece_RefreshAt) uses it to decide whether to withdraw the placement
 * preview; what it really tests is whether a BOAT is on the square.  It is
 * LFPiece_HasRider (0x0040b390, logflume5.c) with the draw calls removed and
 * an early `return 1`: the same `for (i = 0; i < run->boat_count; i++)` over
 * the 0x24-byte boat records from +0x40, the same `if (p->sub) { walk the
 * sub-list } else { test the piece itself }`, and the same re-read of
 * `run->boat_count` in the latch.
 *
 * The difference from its twin is where `p` lives: HasRider copies it into
 * ebp and homes the loop counter in the freed argument slot, whereas here
 * ebp carries `run` (it is used by the latch as well as the boat address)
 * and `p` is RELOADED from its argument slot at the top of every iteration.
 * That falls out of the same source shape -- `run` gains a latch reference
 * that HasRider's version does not have, because here nothing else keeps it
 * live -- so no construct is needed for it.
 *
 * LEVER: the arms are INVERTED against the twin's.  HasRider is written
 * `if (s) { walk } else { one }` and lays the walk out inline; here the
 * single-piece test is the fall-through and the walk is the jump target, so
 * the source must test `if (s == 0) { one } else { walk }` -- 41 of 49 the
 * other way round, exact this way, with nothing else changed.  A twin's
 * block layout is a hypothesis, not an inheritance.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x0040c2e0
int LFPiece_HasCursor(LFPiece* p)
{
    LFRun*   run = p->run;
    LFPiece* s;
    int      i;

    for (i = 0; i < run->boat_count; i++) {
        s = p->sub;
        if (s == 0) {
            if (LFBoat_IsOnPiece(&run->boats[i], p))
                return 1;
        } else {
            while (s) {
                if (LFBoat_IsOnPiece(&run->boats[i], s))
                    return 1;
                s = s->next;
            }
        }
    }
    return 0;
}

/* ---- the save stream --------------------------------------------------- */
extern int SaveGameWrite(const void* buf, unsigned int n);       /* 0x0047d760 */
/* 0x004123a0: the rider's ORDINAL within the ride's rider list -- the id the
 * stream carries.  Mirror of 0x00412470 (LFAnim_FromId, logflume5.c). */
extern int LFAnim_SaveId(void* set, int ref);                    /* 0x004123a0 */

/* =========================================================================
 * 0x004123c0 -- WRITE the station queue, the save-side twin of
 * LFAnim_LoadRefs (0x00412490, logflume5.c).
 *
 * The stream it produces is exactly what the loader consumes:
 *
 *   dword n                   how many points the queue's walk path has
 *   n * 12 bytes              the path points themselves
 *   dword m                   how many riders are queueing
 *   m * dword                 each rider's ORDINAL in the ride's rider list
 *
 * The rider count is not stored anywhere -- the list is walked once to count
 * it and a second time to write the ids -- which is why the queue's O(1)
 * tail pointer (+0x08) is never written and the loader has to rebuild it.
 *
 * LEVER, inherited from the loader and confirmed here: the path count, the
 * path loop counter, the rider count AND the id being written are ONE `int n`
 * whose address is handed to SaveGameWrite.  Being address-taken it cannot be
 * enregistered, so the path loop reloads it from its home every iteration and
 * recomputes `n * 3 / lea [pts + n*4]` instead of getting a strength-reduced
 * induction variable -- the original.  `n` lives in the dead `q` argument
 * slot (q is copied into esi at entry), and the second walk reuses ESI itself
 * for the node cursor, so the whole body needs only edi -- whose push sinks
 * into the guarded id loop.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x004123c0
void LFAnim_SaveRef(void* set, LFQueue* q)
{
    LFQueueNode* p;
    int          n;

    SaveGameWrite(&q->path->count, 4);
    for (n = 0; n < q->path->count; n++)
        SaveGameWrite(q->path->pts + n * 12, 12);
    n = 0;
    p = q->head;
    while (p) {
        n++;
        p = p->next;
    }
    SaveGameWrite(&n, 4);
    p = q->head;
    while (p) {
        n = LFAnim_SaveId(set, p->rider);
        SaveGameWrite(&n, 4);
        p = p->next;
    }
}

/* =========================================================================
 * 0x0040bab0 -- MAY BOAT `idx` ADVANCE?  (was `LFRun_BoatHasRoom`)
 *
 * LFBoat_Step (0x0040bbb0, logflume5.c) asks this before every step, both
 * when the boat is already moving and when it is about to start; a 0 answer
 * simply leaves the boat where it is for another frame.  Named
 * `LFRun_BoatHasRoom` because that is exactly what it computes: the boat
 * ahead must be more than 0.8 of a piece away.
 *
 * The gap is measured in PIECE UNITS along the route, using the boat's
 * position within its piece (LFBoat +0x18, 0..1):
 *
 *   another boat on the SAME piece      gap = other->z - b->z
 *   another boat on the piece AHEAD     gap = other->z + 1.0 - b->z
 *   any other boat                      does not count
 *
 * and the move is refused iff some boat gives 0.0 < gap < 0.8.  A NEGATIVE
 * gap is a boat behind on the same piece and is ignored, which is what makes
 * the rule one-sided.  Only the piece the boat is on and the ONE piece ahead
 * are considered, so the spacing rule is local: a boat two pieces ahead never
 * blocks anything.
 *
 * `if (b == 0)` can never be true -- `b` is `run + idx*36 + 0x40` -- but VC6
 * cannot know that, so the test and its `return 0` are real (LFBoat_Step has
 * the same one).
 *
 * MECHANICS OF THE BODY:
 *  - `gap` is a `float` with NO memory home: VC6 keeps it in st(0) across the
 *    whole loop, so the initialiser `gap = z` is one `fld` in the loop
 *    preheader, every conditional assignment is `fstp st(0)` + a fresh `fld`,
 *    and BOTH exits from inside the loop have to pop the register stack.
 *    That is the whole reason the function has two `return 0` blocks
 *    (0x0040bb48 does the `fstp` and falls into the shared one).
 *  - `> 0.0f` compares against a dword pool constant and `< 0.8` against a
 *    qword one: the float literal keeps the comparison in float, the double
 *    literal promotes it.  Reading the two operand widths off the `fcom`s is
 *    how the literals' types were recovered.
 *  - `b->z` is copied into a float local because it is needed as a MEMORY
 *    operand of three `fsub`s; VC6 does that copy with an integer register
 *    pair (`mov ecx,[eax+18h] / mov [esp+14h],ecx`), not `fld/fstp`.
 *
 * TWO LEVERS, both needed (63/63 with them, 66 instructions and a fourth
 * callee-saved push without either):
 *
 *  1. `gap` IS NEVER INITIALISED.  The preheader `fld [esp+14h]` is not
 *     `gap = z;` -- it is VC6 materialising an UNINITIALISED float local
 *     whose home the allocator happened to give the same dead-argument slot
 *     as `z`.  Written `gap = z;` before the loop the `fld` schedules ONE
 *     instruction early (above `test edi,edi` instead of below the `jle`) and
 *     the loop-exit `fstp` slides two later; leaving `gap` uninitialised puts
 *     both exactly where the original has them.  The read is unreachable in
 *     practice -- the `||` guard admits only the two cases the two inner
 *     `if`s cover -- so this is the same benign uninitialised local as
 *     LFPiece_DrawJoin's `shape` and LFBoat_Advance's `dir`, and it is how
 *     you read "assigned in every arm but VC6 cannot prove it" off a
 *     disassembly: a `fld` of a live local's slot in a loop preheader.
 *  2. A FREE VOLATILE READ on the `piece->fwd` guard.  Spelled plainly, VC6
 *     keeps `fwd` live in EBX across the loop and needs a FOURTH callee-saved
 *     register (`push ebp`); the original has exactly six live values
 *     (idx, b->piece, the boat cursor, the count, `i`, and eax as scratch)
 *     and re-loads `[edx+8]` as a `cmp` memory operand at both uses.  The
 *     volatile read cannot be CSE'd with the two later reads, so the value
 *     dies at the guard and the whole allocation falls into place.  This is
 *     the free-volatile lever used to REMOVE a CSE rather than to advance the
 *     scratch rotation, and the placement rule holds: it belongs on the
 *     value's own original load.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x0040bab0
int LFRun_BoatHasRoom(LFRun* run, int idx)
{
    LFBoat* b = &run->boats[idx];
    float   z;
    float   gap;
    int     i;

    if (b == 0)
        goto fail;
    /* The free volatile read is what keeps `fwd` OUT of a register -- see the
     * note above.  It costs no instruction: the original loads here anyway. */
    if (*(LFPiece* volatile*)&b->piece->fwd == 0)
        goto fail;
    z = b->z;
    for (i = 0; i < run->boat_count; i++) {
        if (i != idx) {
            if (run->boats[i].piece == b->piece ||
                run->boats[i].piece == b->piece->fwd) {
                if (run->boats[i].piece == b->piece)
                    gap = run->boats[i].z - z;
                if (run->boats[i].piece == b->piece->fwd)
                    gap = (float)(run->boats[i].z + 1.0 - z);
                if (gap > 0.0f && gap < 0.8)
                    goto fail;
            }
        }
    }
    return 1;
fail:
    return 0;
}

/* ---- the six boat paths through one tile (logflume4.c) ----------------- */
typedef struct LFPath {
    int  n;                     /* +0x00  how many control points */
    Pos* pts;                   /* +0x04 */
} LFPath;

extern LFPath g_lf_path_tr_bl;          /* 0x004c2b58  straight, 2 points */
extern LFPath g_lf_path_br_tl;          /* 0x004c2b00  straight, 2 points */
extern LFPath g_lf_path_tr_br;          /* 0x004c2be8  quarter turn, 4 */
extern LFPath g_lf_path_br_bl;          /* 0x004c2bc0  quarter turn, 4 */
extern LFPath g_lf_path_bl_tl;          /* 0x004c2c10  quarter turn, 4 */
extern LFPath g_lf_path_tl_tr;          /* 0x004c2c08  quarter turn, 4 */

/* 0x004112f0 (not exported): sample a control polygon at `t` in 0..1 and
 * hand back the point as a Pos in eax:edx; `reverse` walks it backwards. */
extern Pos LFPath_Point(LFPath* path, float t, int reverse);     /* 0x004112f0 */

/* =========================================================================
 * 0x00411680 -- MOVE THE BOAT ALONG ITS PIECE  (was `LFBoat_Advance`)
 *
 * Named `LFBoat_Advance`.  Every caller -- LFBoat_Step (logflume5.c) and
 * LFBoat_Fall below -- treats a non-zero result as "the boat left this piece"
 * and does `b->piece = b->piece->fwd` itself; this function deliberately does
 * NOT update b->piece, only the position within it.  It:
 *
 *   1. adds the boat's speed (LFBoat +0x20) to its position along the piece
 *      (+0x18) and, if that passes 1.0, subtracts one whole piece and takes
 *      the NEXT piece as the one to draw on -- the value it returns;
 *   2. refuses (returns 0, leaving b->z alone) when there is no piece after
 *      that one, so a boat never runs off the end of the route;
 *   3. works out which way the route leaves this piece, from the map-square
 *      delta to the piece ahead, as an eight-point compass in ODD numbers:
 *      1 = north, 3 = east, 5 = south, 7 = west;
 *   4. picks one of the six control polygons LFTrack_BuildGeometry built
 *      (logflume4.c) from the piece's kind and orientation, and reverses it
 *      when the route runs through the tile the other way;
 *   5. samples it at the new position and stores the result as the boat's
 *      draw offset within the tile (LFBoat +0x0c/+0x10).
 *
 * So the table below is the whole map from "piece shape" to "curve":
 *
 *   kind 1 (STRAIGHT)  dir 1 -> br_tl, reversed when heading east
 *                      dir 0 -> tr_bl, reversed when heading north
 *   kind 2 (CORNER)    dir 3 -> tl_tr, reversed when heading west
 *                      dir 0 -> tr_br, reversed when heading north
 *                      dir 1 -> br_bl, reversed when heading east
 *                      dir 2 -> bl_tl, reversed when heading south
 *
 * A piece of any other kind gets no curve at all, and then the boat's draw
 * offset is written from an UNINITIALISED `Pos` -- see the note below.
 *
 * LEVERS (121/121; the last two took it from 122/121 with a first divergence
 * at index 11):
 *  - `if ((nz = b->z + b->speed) > 1.0)` -- the assignment FUSED into the
 *    condition.  That is what emits `fst [nz]` (store, keep) followed by
 *    `fcomp`; written as two statements VC6 emits `fstp` and then reloads
 *    with `fld`, one instruction too many.  A float compared right after
 *    being assigned is one expression, not two statements.
 *  - `switch (p->kind)`, not `if / else if`.  VC6 lowers the two-case switch
 *    to `dec eax / je / dec eax / jne` and lays case 2 out as the
 *    fall-through with case 1 after it; the `if` chain gives `cmp eax,1 /
 *    jne`, the opposite block order, and (because `eax` then holds 1 on the
 *    taken edge) a different register for three of the four `rev = 1`s.
 *  - `1.0` in the guard is a DOUBLE literal (`fcomp qword`) and the `- 1.0f`
 *    that follows is a FLOAT one (`fsub dword`).  The original mixes them;
 *    read each literal's type off its operand width.
 *  - `t.b.x` / `t.b.y` on a `BPosW` LOCAL is what emits the unaligned
 *    `mov eax,[esp+10h] / and eax,0FFh` and `mov edx,[esp+15h] / and
 *    edx,0FFh` pair -- the recorded packed-by-value rule, here for a local
 *    rather than a parameter.  The OTHER square in the same subtraction is
 *    read straight out of the piece as two byte loads, so the asymmetry in
 *    the disassembly is a spelling asymmetry in the source.
 *  - The four `rev = 1;` are all the same statement: VC6 propagates the
 *    constant it just compared against into the arm and stores whichever
 *    register already holds 1 (`mov ebx,esi` where `heading == 1`,
 *    `mov ebx,ecx` where `p->dir == 1`).  Do not reconstruct them as
 *    `rev = heading` or `rev = p->dir`.
 *
 * TWO UNINITIALISED LOCALS, both reproduced, both benign in practice:
 *  - `heading` is only assigned when dx or dy is non-zero, so the `mov esi,7
 *    / jl / mov esi,[esp+20h]` head is VC6 reloading it from a home that the
 *    allocator overlapped with `nz`'s.  Two adjacent route pieces always
 *    differ in one axis, so the garbage never survives.
 *  - `q` is only assigned when a curve was found, and the `mov edx,[esp+18h]
 *    / mov eax,[esp+14h]` at 0x004117ba is the same thing for the Pos: the
 *    no-curve path writes the boat's draw offset from an uninitialised frame
 *    slot.  That one IS reachable -- a piece whose kind is neither 1 nor 2
 *    gets a garbage offset -- and it is the same class of bug as
 *    LFPiece_DrawJoin's unwritten `shape`.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x00411680
int LFBoat_Advance(LFBoat* b)
{
    LFPiece* p;
    LFPiece* np;
    LFPath*  tbl = 0;
    BPosW    t;
    Pos      q;
    float    nz;
    int      dx, dy, heading;
    int      rev = 0;
    int      crossed = 0;

    p = b->piece;
    if ((nz = b->z + b->speed) > 1.0) {
        nz = nz - 1.0f;
        p = p->fwd;
        crossed = 1;
    }
    np = p->fwd;
    if (np == 0)
        return 0;
    t.w = np->sq.w;
    dx = t.b.x - p->sq.b.x;
    dy = t.b.y - p->sq.b.y;
    if (dx < 0) heading = 7;
    if (dx > 0) heading = 3;
    if (dy < 0) heading = 1;
    if (dy > 0) heading = 5;
    switch (p->kind) {
    case 1:
        if (p->dir == 1) { tbl = &g_lf_path_br_tl; if (heading == 3) rev = 1; }
        if (p->dir == 0) { tbl = &g_lf_path_tr_bl; if (heading == 1) rev = 1; }
        break;
    case 2:
        if (p->dir == 3) { tbl = &g_lf_path_tl_tr; if (heading == 7) rev = 1; }
        if (p->dir == 0) { tbl = &g_lf_path_tr_br; if (heading == 1) rev = 1; }
        if (p->dir == 1) { tbl = &g_lf_path_br_bl; if (heading == 3) rev = 1; }
        if (p->dir == 2) { tbl = &g_lf_path_bl_tl; if (heading == 5) rev = 1; }
        break;
    }
    if (tbl)
        q = LFPath_Point(tbl, nz, rev);
    b->ox = q.x;
    b->oy = q.y;
    b->z = nz;
    return crossed;
}

/* ---- the splash the drop makes ---------------------------------------- */
typedef struct LLS {
    short         frame;        /* +0x00  the frame it is showing */
    unsigned char pad02[0xe];
    short         nframes;      /* +0x10  how many frames the animation has */
} LLS;

extern void* g_lfdr_spr_splash;                                  /* 0x004c2b64 */
extern LLS*  GetLLSForSprite(void* sprite);                      /* 0x00441e80 */
/* 0x004117e0 (not exported): how many pieces along the drop's own sub-route
 * the boat's piece is -- the drop's `end_a` chain walked by `fwd` -- or -1
 * if the boat is not on it at all. */
extern int   LFBoat_DropStep(LFBoat* b);                         /* 0x004117e0 */

/* =========================================================================
 * 0x00411810 -- RUN ONE FRAME OF A BOAT FALLING DOWN A DROP
 *               (was `LFBoat_Fall`; named `LFBoat_Fall`)
 *
 * NOT a twin of 0x00411680.  The two are the same SIZE (121 instructions
 * each) and this lane's brief expected one source compiled twice; diffing the
 * disassemblies shows they share nothing but the boat pointer.  Size is not
 * evidence of twinning -- diff before assuming it.
 *
 * LFBoat_Step (logflume5.c) hands the boat here on every tick while its
 * flag bit 1 is set, and nothing else happens to the boat that frame.  The
 * whole drop is driven by ONE scalar:
 *
 *     t = LFBoat_DropStep(b) + b->z
 *
 * i.e. how far down the drop's sub-route the boat is, in pieces, carried to
 * a fraction by its position within the current piece.  Everything else is a
 * piecewise function of `t`, and reading the constants off the `fcom`s gives
 * the drop's whole animation timeline (the frame counter is the SPLASH
 * sprite's 0..120, and the "speed" is z per step):
 *
 *     t <  2      frame 0                       -- still on the lip
 *     2 <= t < 4  frame ramps 0 -> 120,          speed 0.05  (tipping in)
 *     4 <= t <= 5 frame 120,                     speed 0.1   (free fall)
 *     5 <= t <=10 frame ramps 120 -> 0,          speed += 0.05 each step,
 *                 and flag bit 2 is SET while t < 9 and cleared from 9 on
 *    10 <= t      frame 0,                       speed 0.1
 *                 and, on the single step that first crosses 10 (t < 10.5),
 *                 the RUN is told to play its splash.
 *
 * So the boat accelerates all the way down (0.05 per step from t = 5) and the
 * 10.0..10.5 window is a one-shot: the step size at that point is at most
 * 0.5, so exactly one frame of one boat can open the splash.
 *
 * THE SPLASH IS THE RUN'S, NOT THE BOAT'S.  It sets LFRun +0x04 bit 1, zeroes
 * +0x24 and stores the splash sprite's FRAME COUNT in +0x28 -- which
 * reconciles logflume.c (which named +0x24 `splash_frame`, "the frame the
 * drop splash freezes on", and tests bit 1 of +0x04 in LFDrop_Interact) with
 * logflume5.c (which called the pair `hold` / `hold_time`).  They are the
 * splash's frame cursor and its length, and the guard `if (!(run->flags & 2))`
 * is what stops a second boat restarting an animation already running.
 *
 * Bit 2 of the BOAT's flags (set for 5 <= t < 9) is the "in the spray" flag
 * the draw pass reads.
 *
 * LEVERS:
 *  - `(t - 2.0f) * 0.5f * 120.0f` written as ONE expression is
 *    constant-folded by VC6 into `* 60.0f` -- it DOES reassociate a float
 *    chain of two literals.  A two-step float local
 *    (`f = (t - 2.0f) * 0.5f; ... (int)(f * 120.0f)`) blocks the fold and
 *    gives the original's two `fmul`s; that is the whole 2-instruction
 *    deficit, at both sites, and the same local serves both.  (Confirms the
 *    recorded "a two-step float local prevents reassociation".)
 *  - Every threshold is a FLOAT literal (`fcom dword`) except the 10.5 that
 *    closes the splash window, which is a DOUBLE (`fcomp qword`) -- and that
 *    `fcomp` POPS, which is why the two paths out of the last block need
 *    different epilogues (0x00411991 pops the x87 stack, 0x00411993 does
 *    not).
 *  - `b->flags &= ~2` / `|= 4` on a dword field narrow to `and al,0FDh` /
 *    `or al,4` around a dword load and store, because the mask leaves the
 *    top 24 bits alone.
 *  - `if (t >= 9.0f) b->flags &= ~4; else b->flags |= 4;` -- the CLEAR is the
 *    fall-through.  Written the other way round the arms swap.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x00411810
void LFBoat_Fall(LFBoat* b)
{
    LFPiece* drop;
    LFRun*   run;
    LLS*     lls;
    float    t;
    float    f;

    if (b->flags & 2) {
        drop = b->piece->parent;
        if (LFBoat_Advance(b)) {
            b->piece = b->piece->fwd;
            if (b->piece == drop->end_b->fwd) {
                b->flags &= ~2;
                return;
            }
        }
        t = LFBoat_DropStep(b) + b->z;
        if (t < 2.0f) {
            b->frame = 0;
        } else if (t < 4.0f) {
            f = (t - 2.0f) * 0.5f;
            b->frame = (int)(f * 120.0f);
            b->speed = 0.05f;
        }
        if (t >= 4.0f && t <= 5.0f) {
            b->frame = 120;
            b->speed = 0.1f;
        }
        if (t >= 5.0f && t <= 10.0f) {
            f = (t - 5.0f) * 0.2f;
            b->frame = 120 - (int)(f * 120.0f);
            b->speed = b->speed + 0.05f;
            if (t >= 9.0f)
                b->flags &= ~4;
            else
                b->flags |= 4;
        }
        if (t >= 10.0f) {
            b->frame = 0;
            b->speed = 0.1f;
            if (t < 10.5) {
                run = b->piece->run;
                if (run && !(run->flags & 2)) {
                    run->flags |= 2;
                    run->splash_frame = 0;
                    lls = GetLLSForSprite(g_lfdr_spr_splash);
                    if (lls)
                        run->splash_len = lls->nframes;
                }
            }
        }
    }
}

/* ---- the boat's own artwork and the rider riding it -------------------- */
typedef struct ClipRect   { int left, top, right, bottom; } ClipRect;
typedef struct TileBounds { int left, top, right, bottom; } TileBounds;

typedef struct SpriteRec {
    unsigned char pad00[0x14];
    short         w;            /* +0x14  art width  in screen pixels */
    short         h;            /* +0x16  art height in screen pixels */
} SpriteRec;

/* Only the byte the draw pass tests is named. */
typedef struct Bloke { unsigned char pad00[0x62]; unsigned char vis; } Bloke;
typedef struct Person3D Person3D;

struct RiderNode {
    RiderNode*     next;        /* +0x00 */
    RiderNode*     prev;        /* +0x04 */
    Bloke*         bloke;       /* +0x08 */
    unsigned short ride_id;     /* +0x0c  the packed map square it is using */
    unsigned short pad0e;
    Person3D*      person;      /* +0x10  the 3D model that gets drawn */
};

extern SpriteRec* g_spr_barrel;         /* 0x004cbe74  "lf_barrel.lls" */
extern SpriteRec* g_spr_barrel_m;       /* 0x004cbe78  "lf_barrel_m.lls" */
extern SpriteRec* g_spr_barrel1;        /* 0x004cbe7c  "lf_barrel1.lls" */
extern SpriteRec* g_spr_barrelmatte;    /* 0x004cbe80  "barrelmatte.lls" */

extern void GetClipping(ClipRect* out);                          /* 0x0048a630 */
extern void SetClipping(ClipRect* r);                            /* 0x0048a5c0 */
extern void GetTileDimensions(int* w, int* h);                   /* 0x00460540 */
extern void GetTileBounds(Pos* tile, TileBounds* out);           /* 0x0045acc0 */
extern Pos  LFPiece_ScreenPos(LFPiece* p);                       /* 0x0040cfd0 */
extern void AdjustOffsetForViewMode(Pos* o);                     /* 0x00442d30 */
extern void AdjustBlokePosition(Pos* p);                         /* 0x00442d60 */
/* DIVERGENCE, deliberate: every other file declares 0x00440190 as
 * `SetPersonPosition(p, int x, int y)`.  This caller needs the POINT BY
 * VALUE -- see the note below; the ABI is identical (two pushes either way)
 * and the two spellings schedule differently.  Do not "align" it. */
#ifndef LEGOLAND_PORTABLE
extern void SetPersonPosition(Person3D* p, Pos pos);             /* 0x00440190 */
#else
/* The by-value spelling is an x86 scheduling lever: two pushes either way.
 * On wasm32 a by-value struct is passed as a POINTER to a copy, so this is a
 * different function type from sweep1.c's `(Person*, int, int)` definition
 * and the link resolves the one call below to a trapping stub. Unpack the
 * point at the call; the bits the callee stores are the same two dwords. */
extern void SetPersonPosition(Person3D* p, int x, int y);         /* 0x00440190 */
#define SetPersonPosition(_p, _pos) SetPersonPosition((_p), (_pos).x, (_pos).y)
#endif
extern void SetPersonDirection(Person3D* p, int dir);            /* 0x004400b0 */
extern void IP_RenderBlokeIn3DNow(Bloke* b);                     /* 0x00440010 */
extern void LLSSetFrame(LLS* lls, int frame);                    /* 0x0047d5a0 */
extern int  PrintSprite(SpriteRec* s, int x, int y, int mode, void* ctx);
                                                                 /* 0x004853a0 */
/* 0x004092b0 (not exported): which way the boat faces, as the same odd
 * compass LFBoat_Advance computes (1 N, 3 E, 5 S, 7 W); a kind-3 piece takes
 * it from a jump table on the piece's orientation instead. */
extern int  LFBoat_Heading(LFBoat* b);                           /* 0x004092b0 */

/* =========================================================================
 * 0x0040ae90 -- DRAW ONE BOAT OVER ONE PIECE
 *
 * Called by LFPiece_HasRider (logflume5.c) for every boat that lands on the
 * piece being drawn, and by the track's interleaved pass in logflume4.c.
 * `p` is the piece being DRAWN, which is not always the piece the boat is ON:
 * a boat overhangs into its neighbour, and that is the whole reason for the
 * first half of this function.
 *
 * 1. THE BOAT'S TWO SPRITES.  Flag bit 2 (set by LFBoat_Fall while the boat
 *    is 5..9 units down a drop) swaps the normal pair for the splashing one:
 *
 *        clear:  lf_barrel.lls   + lf_barrel_m.lls
 *        set:    lf_barrel1.lls  + barrelmatte.lls
 *
 *    The first is drawn only when the caller passes a non-zero `mode`, and it
 *    is the one whose .lls FRAME is read; the second is always drawn, and is
 *    set to the first one's frame first, so the matte tracks the barrel.
 *
 * 2. CLIPPING WHEN THE BOAT OVERHANGS.  When `p` is not the boat's own piece
 *    the drawing is clipped to the half of `p` that faces the boat, so the
 *    overhanging artwork cannot spill across the whole neighbouring tile.
 *    Which half depends on whether the two squares share a map COLUMN:
 *
 *      same column   right edge <- the tile below-right's right edge, and
 *                    left  edge <- the diagonal tile's left + half a tile
 *      different     left  edge <- the tile below's left + 1, and
 *                    right edge <- the diagonal tile's left + half a tile + 1
 *
 *    The saved rectangle is restored on every exit, including the two early
 *    ones.
 *
 * 3. WHERE THE BOAT LANDS.  The boat's own tile gives the origin (its
 *    bounds' left/top); the boat's within-tile offset (LFBoat +0x0c/+0x10,
 *    written by LFBoat_Advance from the spline) is shifted left by a whole
 *    tile width -- GetTileDimensions is DOUBLED first, exactly as
 *    LFTrack_BuildGeometry doubles it, because the flume art is drawn at
 *    double tile scale -- then run through the view-mode adjust, then pulled
 *    up by `spr->h * 0.75f + boat->frame / 2`.  `frame` is the drop
 *    animation counter, so a falling boat is lifted by up to 60 pixels.
 *
 * 4. THE TWO DEAD LFPiece_ScreenPos CALLS.  Both results are discarded; the
 *    calls are made for their SIDE EFFECT -- LFPiece_ScreenPos clears the
 *    track class's two draw parameters (RideDef +0x14/+0x18) for a plain
 *    track piece.  They are two separate `if`s with opposite conditions, not
 *    an if/else: written as an if/else the body is four instructions short.
 *
 * 5. THE RIDER.  Drawn only in `mode`, only if the bloke's +0x62 has bit 7,
 *    and only if it has a 3D model.  Its point is the boat's offset plus half
 *    the sprite width and three quarters of its height, put through
 *    AdjustBlokePosition and then offset by the tile origin.
 *
 * LEVERS (299/299; the search went 304 -> 302 -> 299 -> 48 -> 17 -> 0):
 *  - **SetPersonPosition takes the point BY VALUE here.**  The last 17
 *    mismatches were a scratch-register rotation that ran one position behind
 *    the original from index 237 to the end.  `(int x, int y)` evaluates the
 *    two sums RIGHT to LEFT and needs only three register definitions; a
 *    `Pos` by value evaluates them LEFT to RIGHT (x first) and takes four,
 *    which both matches the original's load order and advances the rotation
 *    for everything after it.  All four operand orders of the `(int, int)`
 *    form are byte-identical, so this is not reachable by permuting the sums.
 *  - **`tw`/`th` must be declared INSIDE each block, not once at function
 *    scope.**  Two block-scoped pairs let VC6 home both in the dead `b` and
 *    `p` argument slots (the original's `[esp+70h]`/`[esp+74h]`); one
 *    function-scope pair homes only `tw` there, pushes `th` into the frame,
 *    and the whole frame and every `[esp+N]` shifts.  Worth 98.
 *  - **The boat's own offset must be ONE `Pos`, not two `int`s.**  As two
 *    scalars the spilled half lands one slot lower and no longer overlaps the
 *    `Pos` the rider block passes to AdjustBlokePosition; worth 2, at
 *    identical instruction count.  The aggregate placement rule again.
 *  - **A two-step float local is what emits `fiadd`.**  `(int)(spr->h *
 *    0.75f + (b->frame >> 1))` as one expression converts BOTH operands
 *    (`fild / fild / fmul / faddp`, one instruction too many);
 *    `k = spr->h * 0.75f;` then `(int)(k + (b->frame >> 1))` gives the
 *    original's `fild / fmul / fiadd`.  Same lever as LFBoat_Fall's, from the
 *    other side: there it stops a constant fold, here it picks the addend
 *    form.  Reversing the operands is inert.
 *  - **`person` must be a named local.**  Spelling `b->rider->person` at each
 *    of its three uses makes VC6 re-load it after AdjustBlokePosition (the
 *    call kills the load) and frees a callee-saved register, which then holds
 *    `base.y` and lets the rider's point share `base`'s frame slots.  The
 *    local pins the whole allocation and the frame: 132 -> 48.
 *  - `off.x -= spr->w >> 1;` BEFORE `off.y -= ...`: the x update then
 *    schedules into the x87 latency window, where written after it lands
 *    entirely past the `__ftol` call.
 *  - `bp = b->piece;` before the flags test is what splits
 *    `test byte ptr [esi+4],4` into `mov al,[esi+4] / ... / test al,4` --
 *    VC6 fills the gap with the piece load.  Assigning it after the sprite
 *    choice loses the split.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x0040ae90
void LFBoat_Draw(LFBoat* b, LFPiece* p, int mode)
{
    ClipRect   saved;
    ClipRect   clip;
    LFPiece*   bp = b->piece;
    SpriteRec* spr;
    SpriteRec* spr2;

    if (b->flags & 4) {
        spr  = g_spr_barrel1;
        spr2 = g_spr_barrelmatte;
    } else {
        spr  = g_spr_barrel;
        spr2 = g_spr_barrel_m;
    }
    GetClipping(&saved);
    clip = saved;
    if (p != bp) {
        Pos        tile;
        TileBounds tb;
        int        tw, th;

        GetTileDimensions(&tw, &th);
        if (bp->sq.b.x == p->sq.b.x) {
            tile.x = p->sq.b.x + 1;
            tile.y = p->sq.b.y;
            GetTileBounds(&tile, &tb);
            if (tb.right < clip.right)
                clip.right = tb.right;
            tile.x = p->sq.b.x + 1;
            tile.y = p->sq.b.y + 1;
            GetTileBounds(&tile, &tb);
            if (tb.left + (tw >> 1) > clip.left)
                clip.left = tb.left + (tw >> 1);
        } else {
            tile.x = p->sq.b.x;
            tile.y = p->sq.b.y + 1;
            GetTileBounds(&tile, &tb);
            if (tb.left + 1 > clip.left)
                clip.left = tb.left + 1;
            tile.x = p->sq.b.x + 1;
            tile.y = p->sq.b.y + 1;
            GetTileBounds(&tile, &tb);
            if (tb.left + (tw >> 1) + 1 < clip.right)
                clip.right = tb.left + (tw >> 1) + 1;
        }
    }
    SetClipping(&clip);
    if (spr) {
        Pos        tile;
        TileBounds tb;
        Pos        off;
        Pos        base;
        LLS*       lls;
        float      k;
        int        tw, th;
        Pos        o;
        int        frame;

        o.y = b->oy;
        o.x = b->ox;
        GetTileDimensions(&tw, &th);
        tw <<= 1;
        th <<= 1;
        o.x -= tw >> 1;
        tile.x = bp->sq.b.x;
        tile.y = bp->sq.b.y;
        GetTileBounds(&tile, &tb);
        base.x = tb.left;
        base.y = tb.top;
        if (bp->parent)
            LFPiece_ScreenPos(bp->parent);
        if (bp->parent == 0)
            LFPiece_ScreenPos(bp);
        off.x = o.x;
        off.y = o.y;
        AdjustOffsetForViewMode(&off);
        k = spr->h * 0.75f;
        off.x -= spr->w >> 1;
        off.y -= (int)(k + (b->frame >> 1));
        if (mode) {
            PrintSprite(spr, off.x + base.x, off.y + base.y, 0, 0);
            if (b->rider && (b->rider->bloke->vis & 0x80)) {
                Person3D* person = b->rider->person;

                if (person) {
                    Pos q;
                    Pos t;

                    q.x = off.x + (spr->w >> 1);
                    q.y = off.y + (spr->h >> 2) + (spr->h >> 1);
                    AdjustBlokePosition(&q);
                    t.x = q.x + base.x;
                    t.y = q.y + base.y;
                    SetPersonPosition(person, t);
                    SetPersonDirection(person, LFBoat_Heading(b));
                    IP_RenderBlokeIn3DNow(b->rider->bloke);
                }
            }
        }
        if (spr2) {
            frame = 0;
            lls = GetLLSForSprite(spr);
            if (lls)
                frame = lls->frame;
            lls = GetLLSForSprite(spr2);
            if (lls)
                LLSSetFrame(lls, frame);
            PrintSprite(spr2, off.x + base.x, off.y + base.y, 0, 0);
        }
    }
    SetClipping(&saved);
}
