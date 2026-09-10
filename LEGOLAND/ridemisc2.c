/* ---------------------------------------------------------------------------
 * ridemisc2.c -- five small ride helpers: the EARTH SLIDE queue spot, the
 * JUNGLE CRUISE wobble table, the SPACE TOWER queue step, the BOATING SCHOOL
 * boat walk and the RESTAURANT 1 seat waypoints.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are declared LOCALLY on purpose (legoland.h is owned
 * elsewhere) and follow ridecb1.c, ridecb5.c, mechrides.c and ridemisc.c.
 *
 * ==========================================================================
 * EARTH SLIDE -- where the next queuer stands  (0x0042cec0)
 * ==========================================================================
 * ridecb1.c's EarthSlide_Tick calls this from state 0, once per new rider,
 * and drops the answer straight into the bloke's walk target.  The spot is
 *
 *      (base + square + spots[n]) << 8
 *
 * where `n` is HOW MANY NODES ARE ALREADY ON THE SLIDE'S QUEUE (the 8-byte
 * {next, rider} chain at SlideRec +0x1c) and `spots` is the four-entry table
 * of tile deltas at 0x004b65c0:
 *
 *      spots[0] = {0, 4}   spots[1] = {0, 3}
 *      spots[2] = {0, 2}   spots[3] = {0, 1}
 *
 * -- a single-file queue running north towards the ride, one tile per person,
 * with the newest arrival furthest away.
 *
 * TWO THINGS THE ORIGINAL DOES THAT ARE WORTH RECORDING.
 *
 *  1. THE TABLE HAS FOUR ENTRIES AND THE INDEX IS NOT BOUNDED.  `n` is the
 *     raw queue length, so a fifth queuer indexes spots[4] -- the .rdata
 *     bytes immediately after the table, which are the start of a string
 *     constant.  Nothing here clamps it.  Reproduced; the queue is kept short
 *     by the ride's own "do not let anyone on" flag rather than by this
 *     function.
 *  2. THE EMPTY-QUEUE ARM IS WRITTEN OUT SEPARATELY.  The original has two
 *     complete copies of the store block, one with the table access folded to
 *     the absolute `[0x004b65c0]` / `[0x004b65c4]` (i.e. index 0 constant-
 *     folded) and one with `[esi*8 + ...]`.  That is an early `return` on the
 *     empty queue, not a merged tail with a zero-initialised counter.
 *
 * The two coordinate sums are spelled in OPPOSITE operand orders -- `base +
 * square` for x and `square + base` for y -- which is what puts the running x
 * in the base's register and the running y in the square byte's.  Same
 * interleave lever ridecb1.c records for EarthSlide_Tick's caller.
 * --------------------------------------------------------------------------- */

/* ---- shared shapes (same offsets as ridecb1.c) --------------------------- */

typedef struct Pos { int x; int y; } Pos;

typedef struct MapSquare {
    unsigned char bx;            /* +0x00 */
    unsigned char by;            /* +0x01 */
} MapSquare;

/* The class object; only the two base-square fields are needed here. */
typedef struct RideObject {
    unsigned char pad00[0x0c];
    int           base_x;        /* +0x0c */
    int           base_y;        /* +0x10 */
} RideObject;

/* One node of a slide's queue chain. */
typedef struct QueueNode {
    struct QueueNode* next;      /* +0x00 */
    void*             rider;     /* +0x04 */
} QueueNode;

/* The per-square EARTH SLIDE record (ridecb1.c owns the full shape). */
typedef struct SlideRec {
    unsigned char pad00[0x1c];
    QueueNode*    queue;         /* +0x1c */
} SlideRec;

extern SlideRec* EarthSlide_FindRec(MapSquare* sq);                  /* 0x0042ce20 */

/* Four queue positions, in map tiles relative to the ride's base square. */
extern const Pos g_slide_queue_spots[4];                             /* 0x004b65c0 */

// FUNCTION: LEGOLAND 0x0042cec0
void EarthSlide_GetQueueSpot(RideObject* item, MapSquare* key, Pos* out)
{
    SlideRec*  rec = EarthSlide_FindRec(key);
    QueueNode* q = rec->queue;
    int        kx = item->base_x + key->bx;
    int        ky = key->by + item->base_y;
    int        n;

    if (q == 0) {
        out->x = (kx + g_slide_queue_spots[0].x) << 8;
        out->y = (g_slide_queue_spots[0].y + ky) << 8;
        return;
    }
    n = 0;
    do {
        q = q->next;
        n++;
    } while (q);
    /* No bound on `n`: a fifth queuer reads past the four-entry table.
     * Original behaviour, reproduced. */
    out->x = (g_slide_queue_spots[n].x + kx) << 8;
    out->y = (g_slide_queue_spots[n].y + ky) << 8;
}

/* ==========================================================================
 * RESTAURANT 1 -- the customer's five waypoints  (0x0042f0f0)
 * ==========================================================================
 * ridecb1.c's Restaurant1_Tick calls this from states 0, 1, 2, 6 and 7 with
 * `phase` 0..4.  Everything about the move -- where to stand, whether to turn
 * to face it, and which occlusion band to draw in while walking there -- comes
 * out of ONE table of 24-byte rows at 0x004b66f4, indexed `seat * 5 + phase`,
 * so the restaurant's three seats each own five waypoints.  A row is
 *
 *      +0x00 int dx     tile delta from the restaurant's square
 *      +0x04 int dy
 *      +0x08 int ox     sub-tile offset, 1/256ths, added AFTER the shift
 *      +0x0c int oy
 *      +0x10 int turn   1 = also turn to face the new target
 *      +0x14 int band   the occlusion band to draw in (Bloke +0x37)
 *
 * so the target is `((dx + tile) << 8) + offset` per axis -- the offset is
 * NOT shifted, which is what lets a row place a customer a fraction of a tile
 * off the grid (row 2 of seat 0 uses ox = -80, i.e. 0.3 of a tile WEST).
 *
 * Seat 0's five rows, read out of .rdata:
 *      0 {-2, 0, 128, 128, turn, band 3}   the door, tile centre
 *      1 {-2, 1, 128, 150, turn, band 5}
 *      2 {-2, 1, -80, 150, no  , band 4}
 *      3 {-2, 1, 128, 150, no  , band 4}
 *      4 {-2, 0, 128, 128, turn, band 5}   back to the door
 *
 * The band is stored WHETHER OR NOT the customer turns, and the turn is the
 * only thing the flag gates -- the walk order itself (state 7 plus the new
 * facing byte) is issued either way.
 * ========================================================================== */

typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;        /* +0x0e  low-level AI state (7 = walking) */
    unsigned char  pad10[0x24 - 0x10];
    Pos            target;       /* +0x24  walk target, 24.8 */
    unsigned char  pad2c[0x36 - 0x2c];
    unsigned char  seat;         /* +0x36  which of the three seats */
    unsigned char  band;         /* +0x37  occlusion band 1..5 */
    unsigned char  pad38[0x68 - 0x38];
    Pos            world;        /* +0x68  world position, 24.8 */
    unsigned char  pad70[3];
    unsigned char  new_dir;      /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14];   /* +0x98  CalcMoveLine scratch */
} Bloke;

extern int  CalcMoveLine(Pos from, Pos to, void* path);              /* 0x00480740 */
extern int  NewDirForAction(Bloke* b, unsigned char dir);            /* 0x004833d0 */

/* Fifteen 6-int rows: three seats x five waypoints.  Spelled as a FLAT int
 * table because the original scales the index by 6 and then addresses with
 * `[eax*4 + disp32]`; a 24-byte struct array makes VC6 scale by 3 and address
 * with `[eax*8 + disp32]`, one instruction short. */
extern const int g_rest1_tbl[90];                                    /* 0x004b66f4 */

/* THE PARAMETER TYPE IS THE WHOLE FUNCTION.  ridecb1.c declares this callee
 * `(Bloke*, int, int, int)` and calls it with two separate tile coordinates;
 * the DEFINITION takes them as ONE `Pos` BY VALUE.  Same __cdecl ABI (two
 * pushed dwords either way), and the caller is left alone -- but as two plain
 * `int` parameters VC6 accumulates the y sum into the TABLE delta
 * (`add ecx,edx`) where the original accumulates it into the tile coordinate
 * (`add edx,ecx`), and it also drops the `mov ecx,edx` copy that feeds the
 * CalcMoveLine argument: 56 instructions against 57 and 33 mismatches, with
 * indices 0..22 already exact.  As an aggregate member `tile.y` becomes the
 * destination symbol of its own sum and the body is byte-exact.  This is the
 * recorded "extern prototype TYPES are caller-side levers" rule seen from the
 * DEFINITION side; note the divergence and do not align ridecb1.c.
 *
 * Also load-bearing, and measured: the four values that must survive the
 * CalcMoveLine call -- dx, dy, turn and band -- have to be NAMED LOCALS read
 * BEFORE it.  Read at their use sites they load after the call and VC6 pushes
 * only three callee-saved registers; named, it pushes the original's four
 * (ebx, ebp, esi, edi) and root-copies the tile into ebp.  And the table is
 * spelled as a FLAT int array, because the original scales the index by 6 and
 * addresses with `[eax*4 + disp32]`; a 24-byte struct array makes VC6 scale by
 * 3 and address with `[eax*8 + disp32]`, one instruction short. */
// FUNCTION: LEGOLAND 0x0042f0f0
void Restaurant1_WalkToSeatSpot(Bloke* b, Pos tile, int phase)
{
    int           i = (b->seat * 5 + phase) * 6;
    int           dx = g_rest1_tbl[i];
    int           dy = g_rest1_tbl[i + 1];
    int           turn = g_rest1_tbl[i + 4];
    int           band = g_rest1_tbl[i + 5];
    unsigned char a;

    b->target.x = ((tile.x + dx) << 8) + g_rest1_tbl[i + 2];
    b->target.y = ((tile.y + dy) << 8) + g_rest1_tbl[i + 3];
    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
    b->new_dir = a;
    b->state = 7;
    b->band = (unsigned char)band;
    if (turn == 1)
        NewDirForAction(b, (unsigned char)((a >> 5) + 3));
}

/* ==========================================================================
 * BOATING SCHOOL -- step every boat one square  (0x00419300)
 * ==========================================================================
 * ridecb5.c's BoatingSchool_Tick calls this once every 0x50 ticks (the boats
 * carry 80 precomputed sub-steps between squares, bswater.c's `wob[0x50]`),
 * so this is the boats' "arrive at the next square" tick.
 *
 * Every boat first COMMITS its move -- the square it was heading for becomes
 * the square it is on -- and then its mover state (BsBoat +0x3e4) picks what
 * happens next:
 *
 *      1   start a leg           0x004193c0
 *      4   step, turning one way 0x00419520 (0)
 *      8   step, turning the other 0x00419520 (1)
 *      16  end the leg           0x00419420
 *      anything else: nothing
 *
 * 0x00419420 either turns the boat round for the return leg (and gives back
 * the SAME boat) or, when the leg counter has run out, unsources the boat's
 * engine sample, unlinks and frees it (0x00418f90) and gives back the boat
 * that FOLLOWED it.  That is why the walk compares the returned pointer with
 * the one it went in with: a different pointer means this boat is gone and
 * the returned one has not been stepped yet, so the walk continues from it
 * WITHOUT taking `->next`.
 *
 * AN ORIGINAL DEAD ARM, REPRODUCED.  State 16 is tested by the `if` above the
 * switch, so the switch's OWN `case 16` -- which calls the same helper but
 * without the pointer comparison -- can never be reached.  It is in the
 * binary: the 16-byte index table at 0x004193b0 is {0,4,4,1,4,4,4,2,4,4,4,4,
 * 4,4,4,3} and entry 15 (state 16) points at a real block at 0x0041937a.  The
 * case is kept here because deleting it changes the jump table.
 *
 * The `if (b) b = b->next;` before the latch is also the original's: the
 * switch can null `b` out, so the walk re-tests before dereferencing.
 * ========================================================================== */

/* One boat (bswater.c / anim2.c own the full 0x3f4-byte record). */
typedef struct BsBoat {
    unsigned char  pad00[4];
    int            cx;           /* +0x04  the map square it is on */
    int            cy;           /* +0x08 */
    int            nx;           /* +0x0c  the square it is heading for */
    int            ny;           /* +0x10 */
    unsigned char  pad14[0x3e4 - 0x14];
    int            state;        /* +0x3e4  1..0x10, the mover's state */
    int            leg;          /* +0x3e8 */
    void*          rider;        /* +0x3ec */
    struct BsBoat* next;         /* +0x3f0 */
} BsBoat;                        /* 0x3f4 */

extern BsBoat* g_bs_boats;                                           /* 0x004cc03c */

extern void    BsBoat_StartLeg(BsBoat* b);                           /* 0x004193c0 */
extern void    BsBoat_StepLeg(BsBoat* b, int turn);                  /* 0x00419520 */
/* Turn the boat round, or retire it; returns the boat to carry on from. */
extern BsBoat* BsBoat_EndLeg(BsBoat* b);                             /* 0x00419420 */

// FUNCTION: LEGOLAND 0x00419300
void BoatingSchool_AdvanceBoats(void)
{
    BsBoat* b = g_bs_boats;
    BsBoat* old;

    while (b) {
        b->cx = b->nx;
        b->cy = b->ny;
        if (b->state == 16) {
            old = b;
            b = BsBoat_EndLeg(b);
            if (b != old)
                continue;        /* this boat was retired; step the next one */
        } else {
            switch (b->state) {
            case 1:
                BsBoat_StartLeg(b);
                break;
            case 4:
                BsBoat_StepLeg(b, 0);
                break;
            case 8:
                BsBoat_StepLeg(b, 1);
                break;
            case 16:
                /* Unreachable -- the `if` above already took state 16.  The
                 * original has this arm and its jump-table entry. */
                b = BsBoat_EndLeg(b);
                break;
            }
        }
        if (b)
            b = b->next;
    }
}

/* ==========================================================================
 * SPACE TOWER -- step a waiting rider's 3D animation  (0x0043a8c0)
 * ==========================================================================
 * mechrides.c's SpaceTower_TickRiders hands states 2 and 7 -- the two "wait
 * for the car" steps -- straight to this.  It is the 3D ANIMATION PLAYER for
 * a rider that is standing still: it advances the animation and, when the
 * animation runs out, bumps the rider's own action byte so the state machine
 * moves on.  So "the animation finishing" is the clock for those two steps.
 *
 * The animation is reached through the 8-byte table at 0x004b775c that
 * mechrides.c names g_bloke_anim_ref: Bloke +0x50 is an animation id, the
 * table's +0x00 the Anim3D it plays.  An Anim3D is {int nparts; Part** parts}
 * and a Part starts with its own frame count, so the two cursors are
 *
 *      Bloke +0x4a  the part (sequence) the rider is on
 *      Bloke +0x38  the frame within that part
 *      Bloke +0x4c  the part to stop at
 *
 * and the rules are: a part cursor past the end finishes at once; a frame
 * cursor past the part's length rolls to the next part, and finishing (the
 * stop part, or the last part) also bumps the action.  Otherwise 0x0043a820
 * plays one frame -- which is what actually moves the person, since that
 * helper writes the bloke's walk target from the frame's own offset plus the
 * ride's base square.
 *
 * THE THREE RE-READS OF `r->bloke` ARE FORCED, not a quirk: the stores
 * `frame = 0` and `part++` go through a `Bloke*`, which may alias the
 * `Bloke*` field at RiderNode +0x08, so VC6 must reload it after each one.
 * Writing those through `r->bloke->` rather than through the cached `b` is
 * what reproduces them -- a cached local would keep the pointer in a register
 * and lose all three loads.
 *
 * TYPE DIVERGENCE, deliberate: mechrides.c declares +0x38, +0x4a and +0x4c
 * `unsigned short`; this function sign-extends all three (`movsx`), so they
 * are `short` here.  Caller-side lever -- do not align them.
 * ========================================================================== */

typedef struct SpaceTowerBloke {
    unsigned char  pad00[0x38];
    short          frame;        /* +0x38  frame within the current part */
    unsigned char  pad3a[0x4a - 0x3a];
    short          part;         /* +0x4a  which part of the animation */
    short          stop;         /* +0x4c  the part to finish at */
    unsigned char  pad4e[2];
    int            anim;         /* +0x50  animation id */
    unsigned char  pad54[0x60 - 0x54];
    unsigned char  action;       /* +0x60  the ride's state-machine step */
} SpaceTowerBloke;

typedef struct RiderNode {
    struct RiderNode* next;      /* +0x00 */
    struct RiderNode* prev;      /* +0x04 */
    SpaceTowerBloke*  bloke;     /* +0x08 */
    unsigned short    ride_id;   /* +0x0c */
    unsigned short    pad0e;
    void*             person;    /* +0x10 */
} RiderNode;

typedef struct AnimPart {
    int   nframes;               /* +0x00 */
    void* frames;                /* +0x04 */
} AnimPart;

typedef struct Anim3D {
    int        nparts;           /* +0x00 */
    AnimPart** parts;            /* +0x04 */
} Anim3D;

/* mechrides.c's AnimRef: animation id -> the Anim3D it plays. */
typedef struct AnimRef {
    Anim3D* anim;                /* +0x00 */
    int     n;                   /* +0x04 */
} AnimRef;

extern AnimRef g_bloke_anim_ref[];                                   /* 0x004b775c */

/* Play one frame: writes the rider's walk target from the frame's offset. */
extern void SpaceTower_StepAnim(AnimPart* part, RiderNode* r);       /* 0x0043a820 */

// FUNCTION: LEGOLAND 0x0043a8c0
void SpaceTower_Queue(RiderNode* r)
{
    SpaceTowerBloke* b = r->bloke;
    Anim3D*          anim = g_bloke_anim_ref[b->anim].anim;
    AnimPart*        part = anim->parts[b->part];

    if (b->part >= anim->nparts) {
        b->action++;
        return;
    }
    if (b->frame >= part->nframes) {
        r->bloke->frame = 0;
        r->bloke->part++;
        if (r->bloke->part == r->bloke->stop || r->bloke->part == anim->nparts) {
            r->bloke->action++;
            return;
        }
    }
    if (r->bloke->frame != part->nframes)
        SpaceTower_StepAnim(part, r);
}

/* ==========================================================================
 * JUNGLE CRUISE -- build the seat-position table  (0x00432ac0)
 * ==========================================================================
 * screencb.c's JungleCruise_Create calls this once, at ride-create time, and
 * junglecruise.c's boat draw reads the result as
 * `g_jc_seat_pos[seat][frame & 0xf]` -- so despite the name this is not a
 * wobble at all: it is WHERE EACH OF THE THREE PASSENGERS SITS INSIDE THE
 * BOAT SPRITE, in pixels, for each of the sixteen boat headings.  (The name
 * is kept because screencb.c already declares it.)
 *
 * The maths, recovered from the constant pool:
 *
 *      a = i * 22.5                     degrees; 16 headings, a full turn
 *      seat 0 at a           seat 1 at a + 164     seat 2 at a + 196
 *      x = (int)(sin(a * pi/180) * -56.0)
 *      y = (int)(cos(a * pi/180) *  28.0)
 *
 * i.e. every seat is placed on the SAME 56x28-pixel isometric ellipse -- the
 * projection of a circle of radius 56 onto a 2:1 isometric floor -- and the
 * three seats are one at the bow and two at the stern, 164 and 196 degrees
 * round, so the stern pair straddles the boat's axis by 16 degrees either
 * side.  x is NEGATED (-56) because screen x grows the other way from the
 * maths angle.
 *
 * WHAT THE CODEGEN PINS DOWN.  22.5, 180, 16, 196 and pi/180 are single
 * precision (`fmul dword ptr`), -56 and 28 are DOUBLES (`fmul qword ptr`);
 * the running angle `a` is a float local kept on the x87 stack across all
 * three seats, and the second angle is `a + 180.0f - 16.0f` written out in
 * that order (an `fadd` then an `fsub`), not folded to `a + 164.0f`.  `sin`
 * and `cos` are the INTRINSICS -- `fld st(0)` duplicates the radian value,
 * `fsin` consumes the copy and `fcos` the original, so each seat costs one
 * angle computation and two __ftol calls.  VC6 strength-reduces the three
 * subscripted rows to ONE cursor biased at `&g_jc_seat_pos[0][i].y`, with the
 * other five stores reached as [-4], [+0x7c], [+0x80], [+0xfc], [+0x100], and
 * runs the loop to the address 0x0081cc04; the counter still gets a stack
 * home because `(float)i` needs a memory operand for `fild`.  Writing the
 * plain counted `for` with plain subscripts is exact -- do not hand-write the
 * cursor.
 * ========================================================================== */

/* Where each of the three seats sits in the boat sprite, per heading
 * (junglecruise.c's g_jc_seat_pos, indexed [seat][frame & 0xf]). */
typedef struct SeatOfs { int x; int y; } SeatOfs;

extern SeatOfs g_jc_seat_pos[3][16];                                 /* 0x0081cb80 */

double sin(double);
double cos(double);
#pragma intrinsic(sin, cos)

// FUNCTION: LEGOLAND 0x00432ac0
void JungleCruise_BuildWobbleTable(void)
{
    int   i;
    float a;
    float r;

    for (i = 0; i < 16; i++) {
        a = i * 22.5f;
        r = a * 0.017453292f;
#ifndef LEGOLAND_PORTABLE
        g_jc_seat_pos[0][i].x = (int)(sin(r) * -56.0);
        g_jc_seat_pos[0][i].y = (int)(cos(r) * 28.0);
#else
        /* PORT-M5: all six conversions in this table go through 0x00458930,
         * which ROUNDS; the seats would sit one pixel out otherwise. */
        g_jc_seat_pos[0][i].x = LL_FISTPD(sin(r) * -56.0);
        g_jc_seat_pos[0][i].y = LL_FISTPD(cos(r) * 28.0);
#endif
        r = (a + 180.0f - 16.0f) * 0.017453292f;
#ifndef LEGOLAND_PORTABLE
        g_jc_seat_pos[1][i].x = (int)(sin(r) * -56.0);
        g_jc_seat_pos[1][i].y = (int)(cos(r) * 28.0);
#else
        g_jc_seat_pos[1][i].x = LL_FISTPD(sin(r) * -56.0);   /* PORT-M5 */
        g_jc_seat_pos[1][i].y = LL_FISTPD(cos(r) * 28.0);
#endif
        r = (a + 196.0f) * 0.017453292f;
#ifndef LEGOLAND_PORTABLE
        g_jc_seat_pos[2][i].x = (int)(sin(r) * -56.0);
        g_jc_seat_pos[2][i].y = (int)(cos(r) * 28.0);
#else
        g_jc_seat_pos[2][i].x = LL_FISTPD(sin(r) * -56.0);   /* PORT-M5 */
        g_jc_seat_pos[2][i].y = LL_FISTPD(cos(r) * 28.0);
#endif
    }
}
