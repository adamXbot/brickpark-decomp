/* LEGOLAND -- the LOG FLUME ride subsystem, part 3:
 * THE SET-PIECE PLACEMENT PASSES.
 *
 * Companion to logflume.c (the ten classes' callback shims), logflume2.c
 * (the shared piece machinery and the geometry/shape/probe quadruple) and
 * lfentrance.c (the station).  Read logflume2.c's header first: it defines
 * the four-slot neighbour array, the LFPiece/LFRun records and the
 * per-class geometry quadruple that this file's PLACE half completes.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  None of
 * these functions is exported, so every extent came from control flow
 * (tools/audit.py).  Struct field OFFSETS and global addresses are
 * load-bearing; names are ours.  Types are declared LOCALLY on purpose.
 *
 * =========================================================================
 * WHAT A "PLACE" PASS IS
 *
 * A log-flume SET PIECE (TUNNEL, C-SAW, HOLD UP, DROP, and the four SPECIAL
 * CORNERs) is a single object on the map as far as the build UI and the
 * money are concerned, but the boat has to be able to ride THROUGH it.  So
 * when the parent piece has been placed, its PLACE callback explodes the
 * art's footprint into a chain of ordinary one-cell flume sub-pieces:
 *
 *   - each is allocated by LFPiece_Alloc (zeroed, 0x38 bytes);
 *   - each is stamped with the LOG FLUME TRACK class definition
 *     (g_lftr_def) -- NOT the set piece's own def;
 *   - each inherits the parent's run (+0x24) and keeps a back-pointer to
 *     the parent at +0x28;
 *   - each gets flag bit 2 ("belongs to a set piece"), which is what stops
 *     it being billed, drawn or demolished on its own;
 *   - each is pushed onto the parent's sub-list with LFPiece_AddSub and
 *     threaded onto a private route with LFPiece_LinkAfter;
 *   - the FIRST and LAST sub-pieces are recorded in the parent at +0x30 and
 *     +0x34, which is exactly the pair LF*_Shape then publishes into the
 *     direction slots the class occupies.
 *
 * The chain's shape IS the ride's floor plan.  Sub-piece +0x18 ("kind") is
 * 3 at a free end, 1 for plain channel and 2 where the route turns; +0x1c
 * ("dir") is the compass orientation 0=N 1=E 2=S 3=W.  Stepping is in whole
 * map cells: the cell size comes from the shared flume footprint template
 * g_lf_footprint (0x004b4728), as
 *
 *      cellw = g_lf_footprint.v[2] - g_lf_footprint.v[0]
 *      cellh = g_lf_footprint.v[3] - g_lf_footprint.v[1]
 *
 * and the chain's origin is the parent's map square biased by the CLASS
 * def's own footprint corner (+0x3c/+0x40) plus a per-class constant.
 *
 * =========================================================================
 * THE RUNNING SQUARE IS ONE TWO-BYTE AGGREGATE, NOT TWO CHARS
 *
 * *** This is the lever that decides every function in this file. ***
 *
 * The pass carries a running map square across the LFPiece_Alloc /
 * LFPiece_AddSub / LFPiece_LinkAfter calls.  Both halves are byte-wide, and
 * on x86 the only BYTE-ADDRESSABLE callee-saved register is EBX -- so
 * exactly one half can survive in a register and the other must spill.  The
 * original spills the loser into the DEAD PARAMETER HOME: `parent` arrives
 * at [esp+argslot], is copied to a callee-saved register at entry, and the
 * slot is then reused.
 *
 * Written as TWO `unsigned char` locals, VC6 gives EBX to whichever has
 * FEWER weighted references -- and that is only sometimes the original's
 * choice (it matches on LFCsaw_Place and misses on LFTunnel_Place,
 * LFHoldUp_Place and LFDrop_Place).  Written as ONE two-byte aggregate
 *
 *      typedef struct BPos { unsigned char x; unsigned char y; } BPos;
 *      BPos c;
 *
 * with `c.x` / `c.y` in place of the two scalars, VC6 picks the original's
 * winner in ALL FOUR functions -- including the two where the loop-carried
 * half is the one that spills.  Measured on LFTunnel_Place: two scalars
 * 164/217 index mismatches and 213 instructions; `BPos c` 5/217 and 217
 * instructions, 680/680 bytes.  `BPosW` (the union) and `unsigned char
 * c[2]` are byte-identical to `BPos`; TWO one-member structs are not (they
 * behave exactly like two scalars), so it is the SHARED aggregate that
 * matters, not the struct wrapper.
 *
 * The aggregate also fixes a second, separate-looking symptom: with plain
 * scalars VC6 SINKS the last coordinate update into the trailing
 * `if (sub != 0)` block (the value is dead on the null path), losing two
 * instructions and moving the add past LFPiece_Alloc.  As an aggregate
 * member the store stays where the original has it.  That is the same
 * "sinks the last y += cellh past LFPiece_Alloc" symptom recorded in
 * LFDrop_Place's note; it is not a separate problem.
 *
 * The aggregate is still FLATTENED -- its address is never taken, one half
 * lives in EBX -- but LFCorner_Place shows the home is the struct's, not an
 * arbitrary spill slot: there `c.x` wins EBX and `c.y` spills to
 * [esp+0x1d], i.e. BYTE 1 of the 2-byte object homed at the parameter slot
 * [esp+0x1c].  In the other three `c.x` spills to byte 0 of the same slot.
 *
 * WHERE IT DOES NOT TRANSFER, measured so the next lane need not re-derive
 * it: LFDrop_Place (0x00410180, logflume2.c) is the same family and the
 * same tie, and the aggregate does NOT flip it there -- `c.x` still wins
 * EBX and `c.y` still spills, to the struct's byte 1.  Scored the same way
 * as its committed body (44 index mismatches, 110 insns): plain `BPos c`
 * is 75 / 108, `unsigned char c[2]`, `BPosW` and a padded three-member
 * struct are byte-identical to it, and `BPos c` plus that note's volatile
 * read is 47 / 111.  What the aggregate DOES fix there is the second
 * symptom: the last `c.y += cellh` is emitted BEFORE LFPiece_Alloc with its
 * store intact, which is item 4 of that note's "still unexplained" list, and
 * it no longer needs the volatile shim to put the reload at each block top.
 * The distinguishing feature is worth stating: in all four functions here
 * BOTH halves of the square are updated at least once, and in LFDrop_Place
 * `c.x` is written once and never again.
 *
 * SECOND LEVER, worth as much on the head: the class ObjDef must be read
 * DIRECTLY from its global at both uses, not through a `RideDef* def`
 * local.  With the local, VC6 schedules the pointer load after the parent
 * load; read directly it lands where the original has it, immediately after
 * the cellw subtraction and before the register pushes.  On LFCsaw_Place
 * that single change is 18 index mismatches -> 0.
 *
 * =========================================================================
 * WHAT IS IN THIS FILE
 *
 *   0x0040f5b0   LFCsaw_Place        172 insns   exact
 *   0x0040f050   LFTunnel_Place      217 insns   [5 of 217 differ]
 *   0x0040fad0   LFHoldUp_Place      287 insns   exact
 *   0x0040dc00   LFCorner_Place      574 insns   [10 of 574 differ]
 *
 * Both residuals reproduce the original's instruction COUNT exactly and are
 * pure instruction SCHEDULING inside one block each; the note above each
 * marker says precisely what differs and what was measured and ruled out.
 * ========================================================================= */

/* ---- shared types (same offsets as logflume.c / logflume2.c) ------------ */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

typedef struct FootPart FootPart;

typedef struct Footprint {
    int       v[4];             /* +0x00 */
    FootPart* parts;            /* +0x10 */
} Footprint;                    /* 0x14 */

typedef struct SpriteRec {
    unsigned char pad00[8];
    int           f08;
    unsigned char pad0c[4];
    unsigned int  flags;        /* +0x10 */
} SpriteRec;

typedef struct RideDef {
    unsigned char pad00[8];
    int           f08;
    unsigned char pad0c[8];
    int           f14;
    int           f18;
    unsigned int  flags1c;
    unsigned char pad20[0x1c];
    Footprint     footprint;    /* +0x3c */
    unsigned char pad50[0x14];
    SpriteRec*    sprite;       /* +0x64 */
} RideDef;

typedef struct LFPiece LFPiece;
typedef struct LFRun   LFRun;

/* A PLACED PIECE of log flume: one map square with a piece definition on it.
 * 0x38 bytes, zeroed by LFPiece_Alloc. */
struct LFPiece {
    LFPiece*      next;         /* +0x00  next piece of the same run */
    LFPiece*      prev;         /* +0x04 */
    LFPiece*      fwd;          /* +0x08  next piece ALONG THE ROUTE */
    LFPiece*      back;         /* +0x0c  previous piece along the route */
    unsigned int  flags;        /* +0x10  bit 2 = part of a set piece */
    BPosW         sq;           /* +0x14  the piece's map square */
    unsigned char pad16[2];
    int           kind;         /* +0x18  3 = free end, 1 = channel, 2 = turn */
    int           dir;          /* +0x1c  orientation 0=N 1=E 2=S 3=W */
    RideDef*      def;          /* +0x20 */
    LFRun*        run;          /* +0x24 */
    LFPiece*      owner;        /* +0x28  the set piece this cell belongs to */
    LFPiece*      sub;          /* +0x2c  head of this piece's sub-list */
    LFPiece*      end_a;        /* +0x30  the sub-piece at end A */
    LFPiece*      end_b;        /* +0x34  the sub-piece at end B */
};

/* ---- module globals ---------------------------------------------------- */
extern Footprint g_lf_footprint;        /* 0x004b4728  the flume cell rect */
extern RideDef*  g_lftr_def;            /* 0x004cbe30  LOG FLUME TRACK */
extern RideDef*  g_lftu_def;            /* 0x004cbe18  LOG FLUME TUNNEL */
extern RideDef*  g_lfcs_def;            /* 0x004c2bf0  LOG FLUME CSAW */
extern RideDef*  g_lfhu_def;            /* 0x004c2b60  LOG FLUME HOLD UP */
extern RideDef*  g_lfc1_def;            /* 0x004c445c  SPECIAL CORNER 1 */
extern RideDef*  g_lfc2_def;            /* 0x004c2aa0  SPECIAL CORNER 2 */
extern RideDef*  g_lfc3_def;            /* 0x004c2b0c  SPECIAL CORNER 3 */
extern RideDef*  g_lfc4_def;            /* 0x004c74d4  SPECIAL CORNER 4 */
extern int       g_lf_corner_index;     /* 0x004c2af4  which corner class is live */

/* ---- shared piece machinery (logflume2.c) ------------------------------ */
extern LFPiece* LFPiece_Alloc(void);                        /* 0x00409010 */
extern void     LFPiece_AddSub(LFPiece* parent, LFPiece* p);/* 0x00409170 */
extern void     LFPiece_LinkAfter(LFPiece* a, LFPiece* b);  /* 0x00409080 */

/* The sub-piece constructor, as a macro: VC6 expands it once per cell with
 * no call, which is how the original reads.  The store order below IS the
 * original's source order -- every one of the four functions here has at
 * least one block that emits it verbatim, and the rest are VC6 reordering
 * adjacent stores around the flags read-modify-write. */
#define LF_MAKE_SUB(kind_, dir_)                                          \
    sub = LFPiece_Alloc();                                                \
    if (sub) {                                                            \
        sub->kind   = (kind_);                                            \
        sub->dir    = (dir_);                                             \
        sub->run    = parent->run;                                        \
        sub->owner  = parent;                                             \
        sub->def    = g_lftr_def;                                         \
        sub->flags |= 4;                                                  \
        sub->sq.b.x = c.x;                                                \
        sub->sq.b.y = c.y;                                                \
    }

/* =========================================================================
 * LOG FLUME C-SAW  (0x0040f5b0)
 *
 * Eight cells laid west to east with a single one-cell jog north near the
 * far end -- the "C" of the name is the kink, not a curve:
 *
 *      (x0+5w, y0-h) 2/E --- (x0+6w, y0-h) 3/E      <- end B
 *          |
 *      (x0, y0) 3/W - 1/E - 1/E - 1/E - 1/E - (x0+5w, y0) 2/W
 *          ^ end A
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0040f5b0
void LFCsaw_Place(LFPiece* parent)
{
    int           cellw;
    int           cellh;
    BPos          c;
    unsigned char px;
    unsigned char py;
    LFPiece*      sub;
    LFPiece*      prev;
    int           i;

    cellw = g_lf_footprint.v[2] - g_lf_footprint.v[0];
    cellh = g_lf_footprint.v[3] - g_lf_footprint.v[1];
    px = parent->sq.b.x;
    py = parent->sq.b.y;
    c.x = (unsigned char)((unsigned char)g_lfcs_def->footprint.v[0] + px);
    c.y = (unsigned char)((unsigned char)g_lfcs_def->footprint.v[1] + py + 3);

    LF_MAKE_SUB(3, 3)
    LFPiece_AddSub(parent, sub);
    parent->end_a = sub;
    prev = sub;

    for (i = 0; i < 4; i++) {
        c.x += (unsigned char)cellw;
        LF_MAKE_SUB(1, 1)
        LFPiece_AddSub(parent, sub);
        LFPiece_LinkAfter(prev, sub);
        prev = sub;
    }

    c.x += (unsigned char)cellw;
    LF_MAKE_SUB(2, 3)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    prev = sub;

    c.y -= (unsigned char)cellh;
    LF_MAKE_SUB(2, 1)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    prev = sub;

    c.x += (unsigned char)cellw;
    LF_MAKE_SUB(3, 1)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    parent->end_b = sub;
}

/* =========================================================================
 * LOG FLUME TUNNEL  (0x0040f050)
 *
 * Seven cells in a dogleg: two south, two west, then two south again, so
 * the boat disappears into the hillside on one side of the mound and comes
 * out one column over.
 *
 *                  (x0, y0) 3/N       <- end A
 *                      |
 *                  (x0, y0+h) 1/N
 *                      |
 *   (x0-2w, y0+2h) 2/E - 1/E - (x0, y0+2h) 2/W
 *        |
 *   (x0-2w, y0+3h) 1/N
 *        |
 *   (x0-2w, y0+4h) 3/S <- end B
 * ========================================================================= */

/* RESIDUAL: 217/217 instructions, 680/680 bytes, 5 index mismatches, first
 * diverging index 16.  Everything from index 23 (the first LFPiece_Alloc)
 * to the end is EXACT; the whole residual is a five-slot permutation of the
 * head's last nine instructions, which are the same multiset in both:
 *
 *   orig  14 mov al,[def+3ch]  15 mov bl,[esi+14h]  16 mov dl,[esi+15h]
 *         17 add al,bl  18 mov bl,[def+40h]  19 add al,6  20 push edi
 *         21 add bl,dl  22 mov [esp+1ch],al
 *   ours  14 ..  15 ..  16 push edi  17 add al,bl  18 mov bl,[def+40h]
 *         19 mov cl,[esi+15h]  20 add al,6  21 mov [esp+1ch],al
 *         22 add bl,cl
 *
 * i.e. VC6 schedules the `py` load LATE (and therefore the deferred
 * `push edi` early).  The one structural difference from the two exact
 * siblings is WHICH coordinate carries the per-class constant: LFCsaw_Place
 * (+3) and LFHoldUp_Place (+9) put it on the SECOND-computed coordinate and
 * both keep the two parent-square loads adjacent; LFTunnel_Place puts it on
 * the FIRST (+6), which lengthens x's dependency chain and drops py's
 * priority in the list scheduler.  That is a hypothesis about VC6's
 * priority function, not a lever -- nothing found moves it.
 *
 * MEASURED AND RULED OUT (all give exactly 5, byte-identical, unless
 * stated): every legal order of the six head statements {cellw, cellh, px,
 * py, c.x, c.y} (240 orders; the only other plateaus are 7, 17, 18, 19, 20,
 * 21, 22); six spellings of the c.x expression x four of c.y (the +6 split
 * off as its own statement, folded into the footprint read, px as the
 * accumulator, the inner (unsigned char) cast dropped, three-def forms);
 * px/py inlined at their uses; px/py as `int`; px/py as their own BPos;
 * `p = parent` copied to a local; reading the square once through a BPosW.
 * Worse and rejected: initialising c from the parent square and adding the
 * footprint afterwards -- the LFCorner_Place idiom -- in all four orders
 * (19, 21, 115, 200); `BPos p = parent->sq.b` (193).
 *
 * The `BPos c` aggregate and the direct `g_lftu_def` reads (see the file
 * header) are both load-bearing here: two `unsigned char` locals give 164
 * mismatches and 213 instructions, and a `RideDef* def` local gives 8.
 */
// WIP-FUNCTION: LEGOLAND 0x0040f050  (217/217 insns, 680/680B, 5 index mismatches; the head's py-load schedule -- see note)
void LFTunnel_Place(LFPiece* parent)
{
    int           cellw;
    int           cellh;
    BPos          c;
    unsigned char px;
    unsigned char py;
    LFPiece*      sub;
    LFPiece*      prev;

    cellw = g_lf_footprint.v[2] - g_lf_footprint.v[0];
    cellh = g_lf_footprint.v[3] - g_lf_footprint.v[1];
    px = parent->sq.b.x;
    py = parent->sq.b.y;
    c.x = (unsigned char)((unsigned char)g_lftu_def->footprint.v[0] + px + 6);
    c.y = (unsigned char)((unsigned char)g_lftu_def->footprint.v[1] + py);

    LF_MAKE_SUB(3, 0)
    LFPiece_AddSub(parent, sub);
    parent->end_a = sub;
    prev = sub;

    c.y += (unsigned char)cellh;
    LF_MAKE_SUB(1, 0)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    prev = sub;

    c.y += (unsigned char)cellh;
    LF_MAKE_SUB(2, 3)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    prev = sub;

    c.x -= (unsigned char)cellw;
    LF_MAKE_SUB(1, 1)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    prev = sub;

    c.x -= (unsigned char)cellw;
    LF_MAKE_SUB(2, 1)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    prev = sub;

    c.y += (unsigned char)cellh;
    LF_MAKE_SUB(1, 0)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    prev = sub;

    c.y += (unsigned char)cellh;
    LF_MAKE_SUB(3, 2)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    parent->end_b = sub;
}

/* =========================================================================
 * LOG FLUME HOLD UP  (0x0040fad0)
 *
 * Ten cells: the boat runs east two, climbs north two, runs east three more,
 * drops back south one and leaves east -- the staircase that holds a boat
 * back while the one ahead clears the drop.
 *
 *   (x0+2w, y0-2h) 2/E - 1/E - 1/E - (x0+5w, y0-2h) 2/S
 *        |                                   |
 *   (x0+2w, y0-h) 1/N                 (x0+5w, y0-h) 2/N - (x0+6w, y0-h) 3/E
 *        |                                                     ^ end B
 *   (x0, y0) 3/W - 1/E - (x0+2w, y0) 2/W
 *      ^ end A
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0040fad0
void LFHoldUp_Place(LFPiece* parent)
{
    int           cellw;
    int           cellh;
    BPos          c;
    unsigned char px;
    unsigned char py;
    LFPiece*      sub;
    LFPiece*      prev;
    int           i;

    cellw = g_lf_footprint.v[2] - g_lf_footprint.v[0];
    cellh = g_lf_footprint.v[3] - g_lf_footprint.v[1];
    px = parent->sq.b.x;
    py = parent->sq.b.y;
    c.x = (unsigned char)((unsigned char)g_lfhu_def->footprint.v[0] + px);
    c.y = (unsigned char)((unsigned char)g_lfhu_def->footprint.v[1] + py + 9);

    LF_MAKE_SUB(3, 3)
    LFPiece_AddSub(parent, sub);
    parent->end_a = sub;
    prev = sub;

    c.x += (unsigned char)cellw;
    LF_MAKE_SUB(1, 1)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    prev = sub;

    c.x += (unsigned char)cellw;
    LF_MAKE_SUB(2, 3)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    prev = sub;

    c.y -= (unsigned char)cellh;
    LF_MAKE_SUB(1, 0)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    prev = sub;

    c.y -= (unsigned char)cellh;
    LF_MAKE_SUB(2, 1)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    prev = sub;

    for (i = 0; i < 2; i++) {
        c.x += (unsigned char)cellw;
        LF_MAKE_SUB(1, 1)
        LFPiece_AddSub(parent, sub);
        LFPiece_LinkAfter(prev, sub);
        prev = sub;
    }

    c.x += (unsigned char)cellw;
    LF_MAKE_SUB(2, 2)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    prev = sub;

    c.y += (unsigned char)cellh;
    LF_MAKE_SUB(2, 0)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    prev = sub;

    c.x += (unsigned char)cellw;
    LF_MAKE_SUB(3, 1)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    parent->end_b = sub;
}

/* =========================================================================
 * LOG FLUME SPECIAL CORNER 1..4  (0x0040dc00)
 *
 * ONE function for all four corner classes: the shims in logflume.c stamp
 * the class index 0..3 into g_lf_corner_index before delegating, and this
 * pass switches on it.  Each corner is a five-cell L: two cells along one
 * axis, a turn cell, two along the other.  The four differ only in which
 * quadrant they turn through and in the per-class origin bias:
 *
 *   case 0 (CORNER 1)  south 2, turn, east 2     ends 3/E
 *   case 1 (CORNER 2)  west 2,  turn, south 2    ends 3/S
 *   case 2 (CORNER 3)  north 2, turn, west 2     ends 3/W
 *   case 3 (CORNER 4)  east 2,  turn, north 2    ends 3/N
 *
 * Cases 1 and 2 bias the far edge of the art rather than the near one, so
 * their origin carries the (v[2] - v[0] - cellw + 1) / (v[3] - v[1] -
 * cellh + 1) correction: "the last whole cell inside the footprint".
 *
 * ORIGINAL BUG, reproduced: the parent's map square is read BEFORE the null
 * check, so a null parent faults here rather than returning quietly.  The
 * compiled code hoists both byte loads above the `test esi,esi`, which is
 * only legal because the source really does read them first.
 *
 * The pass also OVERWRITES the parent's orientation (+0x1c) with the corner
 * index -- which is why the CSAW/HOLD UP/DROP add handlers' stray
 * `g_lf_corner_index = 0` (see logflume.c) is not merely cosmetic: a corner
 * placed after one of those is stamped as SPECIAL CORNER 1 whatever it is.
 * ========================================================================= */

/* RESIDUAL: 574/574 instructions, 1824 of 1825 bytes, 10 index mismatches,
 * first diverging index 38.  The head, the switch, all four origin
 * expressions and NINETEEN of the twenty sub-piece blocks are exact; the
 * whole residual is the store SCHEDULE of case 0's FIRST block, indices
 * 38..48, which is the same multiset in both:
 *
 *   orig  kind, dir, [def load], [c.y load], def, [run load], run,
 *         [flags load], or, owner, flags, x, y
 *   ours  kind, dir, [run load], [c.y load], run, owner, [def load], x,
 *         def, [flags load], or, y, flags
 *
 * The single missing byte is a consequence, not a separate fault: the
 * original loads g_lftr_def into EDX (6 bytes) and we load it into EAX,
 * for which x86 has the 5-byte A1 accumulator form.
 *
 * MEASURED AND RULED OUT: the committed store order is the best of 14
 * orders of the eight member stores measured on THIS function alone with a
 * private macro (next best 84, then 88, 90, 117...), so the order is not
 * the issue and it is the same order that makes the other three functions
 * exact; writing case 0's first block OUT of the macro with any of ten
 * orders is strictly worse (11 -> 149+) because it breaks the cross-jumped
 * tail the compiler builds out of cases 1..3; all six orders of case 0's
 * three origin statements (only the committed one survives -- the other
 * five cost 538, they lose the tail merge); moving `parent->end_a = sub`
 * before LFPiece_AddSub, and `prev = sub` before it, in every case.
 *
 * WHY THE FOUR CASES CARRY DIFFERENT STATEMENT ORDERS.  `parent->end_a`,
 * `prev` and the following coordinate update are three independent writes,
 * and unlike the analogous block in LFEntrance_Add their order here IS
 * visible in the output (17 / 16 / 11 for the uniform and the committed
 * mixtures).  So the committed per-case order is read off the original's
 * own emission -- case 0 end_a, update, prev; cases 1 and 2 prev, end_a;
 * case 3 end_a, prev -- which is what four hand-written case blocks look
 * like.  A uniform `end_a; prev` (what the other three functions in this
 * file use) costs 6 more.
 *
 * NOT EXPLAINED: why case 0's first block alone hoists the g_lftr_def load
 * above the run load.  It is the only block of the twenty that does, and
 * nothing above moves it.
 */
// WIP-FUNCTION: LEGOLAND 0x0040dc00  (574/574 insns, 1824/1825B, 10 index mismatches; case 0's first store block -- see note)
void LFCorner_Place(LFPiece* parent)
{
    int      cellw;
    int      cellh;
    BPos     c;
    LFPiece* sub;
    LFPiece* prev;

    cellw = g_lf_footprint.v[2] - g_lf_footprint.v[0];
    cellh = g_lf_footprint.v[3] - g_lf_footprint.v[1];
    c.x = parent->sq.b.x;       /* read before the null test -- see note */
    c.y = parent->sq.b.y;
    if (parent) {
        parent->dir = g_lf_corner_index;
        switch (g_lf_corner_index) {
        case 0:
            c.x += (unsigned char)g_lfc1_def->footprint.v[0];
            c.y += (unsigned char)g_lfc1_def->footprint.v[1];
            c.x += 3;

            LF_MAKE_SUB(3, 0)
            LFPiece_AddSub(parent, sub);
            parent->end_a = sub;

            c.y += (unsigned char)cellh;
            prev = sub;
            LF_MAKE_SUB(1, 0)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            prev = sub;

            c.y += (unsigned char)cellh;
            LF_MAKE_SUB(2, 0)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            prev = sub;

            c.x += (unsigned char)cellw;
            LF_MAKE_SUB(1, 1)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            prev = sub;

            c.x += (unsigned char)cellw;
            LF_MAKE_SUB(3, 1)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            parent->end_b = sub;
            break;

        case 1:
            c.x += (unsigned char)g_lfc2_def->footprint.v[0];
            c.y += (unsigned char)g_lfc2_def->footprint.v[1];
            c.x += (unsigned char)((unsigned char)g_lfc2_def->footprint.v[2]
                                   - (unsigned char)g_lfc2_def->footprint.v[0]
                                   - cellw + 1);
            c.y += 2;

            LF_MAKE_SUB(3, 1)
            LFPiece_AddSub(parent, sub);
            prev = sub;
            parent->end_a = sub;

            c.x -= (unsigned char)cellw;
            LF_MAKE_SUB(1, 1)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            prev = sub;

            c.x -= (unsigned char)cellw;
            LF_MAKE_SUB(2, 1)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            prev = sub;

            c.y += (unsigned char)cellh;
            LF_MAKE_SUB(1, 0)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            prev = sub;

            c.y += (unsigned char)cellh;
            LF_MAKE_SUB(3, 2)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            parent->end_b = sub;
            break;

        case 2:
            c.x += (unsigned char)g_lfc3_def->footprint.v[0];
            c.y += (unsigned char)g_lfc3_def->footprint.v[1];
            c.x += 4;
            c.y += (unsigned char)((unsigned char)g_lfc3_def->footprint.v[3]
                                   - (unsigned char)g_lfc3_def->footprint.v[1]
                                   - cellh + 1);

            LF_MAKE_SUB(3, 2)
            LFPiece_AddSub(parent, sub);
            prev = sub;
            parent->end_a = sub;

            c.y -= (unsigned char)cellh;
            LF_MAKE_SUB(1, 0)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            prev = sub;

            c.y -= (unsigned char)cellh;
            LF_MAKE_SUB(2, 2)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            prev = sub;

            c.x -= (unsigned char)cellw;
            LF_MAKE_SUB(1, 1)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            prev = sub;

            c.x -= (unsigned char)cellw;
            LF_MAKE_SUB(3, 3)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            parent->end_b = sub;
            break;

        case 3:
            c.x += (unsigned char)g_lfc4_def->footprint.v[0];
            c.y += (unsigned char)((unsigned char)g_lfc4_def->footprint.v[1] + 4);

            LF_MAKE_SUB(3, 3)
            LFPiece_AddSub(parent, sub);
            parent->end_a = sub;
            prev = sub;

            c.x += (unsigned char)cellw;
            LF_MAKE_SUB(1, 1)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            prev = sub;

            c.x += (unsigned char)cellw;
            LF_MAKE_SUB(2, 3)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            prev = sub;

            c.y -= (unsigned char)cellh;
            LF_MAKE_SUB(1, 0)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            prev = sub;

            c.y -= (unsigned char)cellh;
            LF_MAKE_SUB(3, 0)
            LFPiece_AddSub(parent, sub);
            LFPiece_LinkAfter(prev, sub);
            parent->end_b = sub;
            break;
        }
    }
}
