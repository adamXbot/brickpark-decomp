/* LEGOLAND -- POSITION STEPS AND LOG-FLUME LOOKUPS.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere) and are copies of the ones schoolcar2.c / goldrush3.c /
 * goldrush4.c and logflume2.c / logflume4.c already established.
 *
 * Five small helpers that three unrelated subsystems share:
 *
 *   0x00480840  Pos_Step2          step a map position +/- 2 squares
 *   0x004808d0  Pos_Step4          step a map position +/- 4 squares
 *   0x0040b210  LFBoat_IsOnPiece   does a boat draw on this flume piece?
 *   0x0040a580  LFRun_Start        park a run's boats on the route
 *   0x00408f30  LFTrack_FindPiece  find the flume piece on a map square
 *
 * =========================================================================
 * 1. THE FOUR-WAY STEP  (Pos_Step2 / Pos_Step4)
 * =========================================================================
 * The driving school's road graph is built out of 4x4-map-square BLOCKS, so
 * a car moves either a whole block (4 squares) or half a block (2 squares)
 * at a time.  These two functions are the whole of that arithmetic:
 *
 *      Pos_Step4(from, to, dir)    *to = *from stepped 4 squares along dir
 *      Pos_Step2(from, to, dir)    ... 2 squares
 *
 * `dir` is a heading on the eight-way compass ring
 *
 *      0 = NW   1 = N   2 = NE   3 = E   4 = SE   5 = S   6 = SW   7 = W
 *
 * and ONLY the four odd (cardinal) headings do anything: 1 subtracts from y,
 * 3 adds to x, 5 adds to y, 7 subtracts from x.  The four even headings --
 * and every value outside 1..7 -- fall into the switch's default and leave
 * *to COMPLETELY UNTOUCHED, i.e. holding whatever the caller left in it.
 * That is why a school car's heading byte (SchoolCar +0xba) is always odd
 * and why `(dir - 2) & 7` / `(dir + 2) & 7` are the left and right turns.
 *
 * Both are called from goldrush3.c, goldrush4.c, schoolcar2.c and
 * schoolcar3.c, always with the same `(CarPos*, CarPos*, int)` extern.
 *
 * Codegen: the switch is a JUMP TABLE over dir-1 (`dec eax / cmp eax,6 /
 * ja <default> / jmp [eax*4+tbl]`), so the three even entries point at the
 * function's bare `ret` and the four live blocks are laid out in ASCENDING
 * case order.  Each block reads BOTH parameters, copies the untouched axis
 * and writes the stepped one -- there is no shared tail.
 *
 * =========================================================================
 * 2. WHERE A BOAT IS  (LFBoat_IsOnPiece)
 * =========================================================================
 * A log-flume boat carries the route piece it is currently over (+0x14) and
 * a position along that piece, `z` in 0..1 (+0x18).  Because a boat is drawn
 * as a sprite that overhangs its own tile, it is visible on up to TWO pieces
 * at once, and the draw pass asks this predicate per piece:
 *
 *      the piece it is ON                       -> always
 *      the piece AHEAD  (cur->fwd)              -> once z >= 0.5
 *      the piece BEHIND (cur->back)             -> while z <  0.5
 *
 * The two halves are complementary, so exactly two pieces answer yes (one
 * when the neighbour is null).  The 0.5 comparison really is done TWICE in
 * the original -- two `fld [b+0x18] / fcomp qword [0x4ab398]` pairs -- so it
 * is two separate source tests, not one cached flag.
 *
 * =========================================================================
 * 3. PARKING THE BOATS  (LFRun_Start)
 * =========================================================================
 * A run keeps its boats INLINE, four of them from LFRun +0x40, 0x24 bytes
 * each, and `boat_count` (+0x3c) says how many of them this run uses.
 * Starting a run walks BACKWARDS along the route from run->f08, one piece
 * per boat (`p = p->back`), and writes each boat:
 *
 *      state  (+0x00) = 0        flags (+0x04) = 0
 *      pos    (+0x0c) = LFQuadBottomLeft()   the tile-quadrant the boat
 *                                            enters a piece at
 *      piece  (+0x14) = p
 *      z      (+0x18) = 1.0f     -- at the END of its piece, so the very
 *                                   first step carries it onto p->fwd
 *      f1c    (+0x1c) = 0
 *      speed  (+0x20) = 0.1f     -- 1/10 of a piece per step
 *
 * so the boats end up nose to tail, evenly spaced one piece apart, all
 * pointing forwards.  `boat_count` is RE-READ from the record every
 * iteration (the loop bound is a struct field, not a local), and so is the
 * boat-array base -- both reproduced.
 *
 * LFEntrance_Add sets boat_count to 4 and piece_count to 6 before calling
 * this (see the wrapper at 0x0040a5d0), so a stock flume runs four boats.
 *
 * =========================================================================
 * 4. THE THREE-LEVEL PIECE LOOKUP  (LFTrack_FindPiece)
 * =========================================================================
 * Every flume run (0x004cbe84 heads the list) owns a list of placed pieces
 * (+0x10), and a piece that was expanded into several map squares owns a
 * SUB-LIST of its own (+0x2c).  This is the map-square -> piece lookup:
 *
 *      for each run
 *          for each piece p of the run
 *              if p has a sub-list      search THAT for the square
 *              else                     compare p's own square
 *
 * Note the asymmetry, which is the original's and is reproduced: once a
 * piece has sub-pieces its OWN square at +0x14 is never tested, so a
 * multi-square piece is only ever found through its parts.
 *
 * The square compare is an INTRINSIC memcmp of length 2, exactly as in
 * logflume2.c's LFStation_FindAt / LFPiece_FindAt: VC6 expands it to
 * `lea r,[base+0x14] / mov r16,[base+0x14] / cmp r16,[sq]`, leaving the
 * address computation DEAD once the load folds the addressing mode back
 * onto the base.  A 16-bit `==` hoists *sq out of the loop and drops the
 * two leas.
 * ========================================================================= */

/* ---------------------------------------------------------------- types -- */

/* A map position in whole squares.  schoolcar2.c / goldrush3.c / goldrush4.c
 * all spell this `CarPos` and all four callers of the two steppers declare
 * them `void (CarPos*, CarPos*, int)`; kept identical here. */
typedef struct CarPos { int x; int y; } CarPos;

typedef struct Pos { int x; int y; } Pos;

/* A packed 2-byte map square. */
typedef struct BPos  { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

typedef struct LFPiece LFPiece;
typedef struct LFRun   LFRun;

/* A PLACED PIECE of log flume (0x38 bytes; logflume2.c owns the record). */
struct LFPiece {
    LFPiece*      next;         /* +0x00  next piece of the same run */
    LFPiece*      prev;         /* +0x04 */
    LFPiece*      fwd;          /* +0x08  next piece ALONG THE ROUTE */
    LFPiece*      back;         /* +0x0c  previous piece along the route */
    unsigned int  flags;        /* +0x10 */
    BPosW         sq;           /* +0x14  the piece's map square */
    unsigned char pad16[2];
    int           kind;         /* +0x18 */
    int           dir;          /* +0x1c */
    void*         def;          /* +0x20 */
    LFRun*        run;          /* +0x24 */
    int           f28;          /* +0x28 */
    LFPiece*      sub;          /* +0x2c  head of this piece's sub-list */
    LFPiece*      end_a;        /* +0x30 */
    LFPiece*      end_b;        /* +0x34 */
};

/* One boat: 0x24 bytes, four of them from LFRun +0x40 on (logflume4.c). */
typedef struct LFBoat {
    int           state;        /* +0x00 */
    unsigned int  flags;        /* +0x04  bit 0 = waiting at the station */
    void*         rider;        /* +0x08  who is aboard */
    Pos           pos;          /* +0x0c  the quadrant point it entered at */
    LFPiece*      piece;        /* +0x14  the route piece it is over */
    float         z;            /* +0x18  how far along that piece, 0..1 */
    int           f1c;          /* +0x1c */
    float         speed;        /* +0x20  fraction of a piece per step */
} LFBoat;

struct LFRun {
    LFRun*        next;         /* +0x00 */
    unsigned int  flags;        /* +0x04 */
    LFPiece*      f08;          /* +0x08  where the boats are seeded from */
    LFPiece*      f0c;          /* +0x0c */
    LFPiece*      pieces;       /* +0x10  head of this run's piece list */
    BPosW         sq;           /* +0x14  the station's map square */
    unsigned char pad16[2];
    LFPiece*      f18;          /* +0x18 */
    int           frame;        /* +0x1c */
    unsigned char pad20[0x18];  /* +0x20..0x37 */
    LFBoat*       boat;         /* +0x38  the boat at the station */
    int           boat_count;   /* +0x3c  how many boats this run runs */
    LFBoat        boats[4];     /* +0x40 .. +0xcf */
    int           piece_count;  /* +0xd0 */
};                              /* 0xd4 */

/* --------------------------------------------------------------- globals -- */

extern LFRun* g_lf_queue;                       /* 0x004cbe84 */

/* --------------------------------------------------------------- callees -- */

/* The centre of the bottom-left quadrant of the (doubled) flume tile --
 * (tw, 3*th), returned as an 8-byte Pos in eax:edx (logflume4.c). */
extern Pos LFQuadBottomLeft(void);                              /* 0x004112c0 */

int memcmp(const void* a, const void* b, unsigned int n);       /* CRT, intrinsic */
#pragma intrinsic(memcmp)

/* =========================================================================
 * THE FOUR-WAY STEP
 * ========================================================================= */

/* Step `from` two map squares along `dir` into `to`; an even heading leaves
 * `to` untouched (the jump table's default is the function's bare ret). */
// FUNCTION: LEGOLAND 0x00480840
void Pos_Step2(CarPos* from, CarPos* to, int dir)
{
    switch (dir) {
    case 1:
        to->x = from->x;
        to->y = from->y - 2;
        break;
    case 3:
        to->x = from->x + 2;
        to->y = from->y;
        break;
    case 5:
        to->x = from->x;
        to->y = from->y + 2;
        break;
    case 7:
        to->x = from->x - 2;
        to->y = from->y;
        break;
    }
}

/* The whole-block twin. */
// FUNCTION: LEGOLAND 0x004808d0
void Pos_Step4(CarPos* from, CarPos* to, int dir)
{
    switch (dir) {
    case 1:
        to->x = from->x;
        to->y = from->y - 4;
        break;
    case 3:
        to->x = from->x + 4;
        to->y = from->y;
        break;
    case 5:
        to->x = from->x;
        to->y = from->y + 4;
        break;
    case 7:
        to->x = from->x - 4;
        to->y = from->y;
        break;
    }
}

/* =========================================================================
 * THE LOG FLUME
 * ========================================================================= */

/* Does boat `b` draw on piece `p` this frame?  True for the piece it is on,
 * for the piece AHEAD once it is past the half-way mark, and for the piece
 * BEHIND until it is.  The half-way test really is written twice. */
// FUNCTION: LEGOLAND 0x0040b210
int LFBoat_IsOnPiece(LFBoat* b, LFPiece* p)
{
    LFPiece* cur = b->piece;

    if (cur == p)
        return 1;
    if (b->z >= 0.5 && cur->fwd == p)
        return 1;
    if (b->z < 0.5 && cur->back == p)
        return 1;
    return 0;
}

/* Seed a run's boats: one per piece walking BACKWARDS from run->f08, each at
 * the very end (z = 1) of its piece and moving a tenth of a piece per step.
 * The count is re-read from the record each iteration (a struct-field loop
 * bound), and so is the boat array's base.
 *
 * The boats MUST be written through plain subscripts, not through a named
 * `LFBoat* b = &run->boats[i];`.  Two separate things ride on it:
 *   - VC6's strength-reduced cursor is anchored on the SECOND store statement
 *     (`lea esi,[run+0x4c]` = boat+0x0c, the pos), and through a named
 *     pointer that anchor slides to the third;
 *   - the `piece` store schedules AHEAD of the call-result store, which no
 *     ordering of the seven statements reaches through the pointer (floor 3).
 * All 48 statement orders x block/function scope of a `Pos q` temporary were
 * measured; only the subscript form is exact (a volatile store on `piece`, or
 * a volatile store on `pos.x`, also reach it through the pointer). */
// FUNCTION: LEGOLAND 0x0040a580
void LFRun_Start(LFRun* run)
{
    LFPiece* p;
    Pos      q;
    int      i;

    p = run->f08;
    for (i = 0; i < run->boat_count; i++) {
        q = LFQuadBottomLeft();
        run->boats[i].piece = p;
        run->boats[i].pos = q;
        run->boats[i].f1c = 0;
        run->boats[i].z = 1.0f;
        run->boats[i].speed = 0.1f;
        run->boats[i].state = 0;
        run->boats[i].flags = 0;
        p = p->back;
    }
}

/* The map-square -> flume-piece lookup: every run, every piece, and for a
 * piece that owns a sub-list the sub-list INSTEAD of the piece itself. */
// FUNCTION: LEGOLAND 0x00408f30
LFPiece* LFTrack_FindPiece(const BPos* sq)
{
    LFRun*   run;
    LFPiece* p;
    LFPiece* s;

    /* The head is COPIED into the cursor and then the GLOBAL is tested again
     * -- objmap2.c's state-copy lever, here on a list head.  It splits the web
     * so VC6 loads into eax before `push esi/edi` and emits the original's
     * `mov esi,eax`; testing `run` instead gives one instruction fewer, and a
     * `do/while` inverts the latch and exiles the `xor eax,eax` exit. */
    run = g_lf_queue;
    if (g_lf_queue == 0)
        return 0;
    while (run) {
        p = run->pieces;
        while (p) {
            s = p->sub;
            if (s == 0) {
                if (memcmp(&p->sq, sq, 2) == 0)
                    return p;
            } else {
                do {
                    if (memcmp(&s->sq, sq, 2) == 0)
                        return s;
                    s = s->next;
                } while (s);
            }
            p = p->next;
        }
        run = run->next;
    }
    return 0;
}
