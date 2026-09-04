/* LEGOLAND -- the three LARGEST ride callbacks in the 0x0043xxxx cluster.
 *
 * These are per-class handlers that SetCustomCallbacks (screen.c 0x00452c20)
 * installs into the 0xd0-byte ObjDef callback slots.  They are NOT exported,
 * so their extents come from the disassembly by control flow (tools/audit.py).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere); they agree with ridecb1.c / ridecb2.c.
 *
 *   addr        class            slot     what it really is         state
 *   0x004316f0  OCTOPUS CAFE     cb_a8    OctopusCafe_Tick          [WIP 87.4%]
 *   0x00431c50  OCTOPUS CAFE     (helper) CafeDrawTable             [OK]
 *   0x00431d00  OCTOPUS CAFE     cb_b0    OctopusCafe_Draw          [OK]
 *   0x00430b10  RESTAURANT 2     cb_b0    Restaurant2_Draw          [OK]
 *
 * (ridecb2.c's header carried a first-pass analysis of the three callbacks;
 * the names were confirmed against SetCustomCallbacks and are kept.
 * 0x00431c50 is a private helper of OctopusCafe_Draw that nothing else
 * calls; it had only been described as "a five-argument PrintSprite wrapper"
 * and is in fact a TEN-argument one -- see its own comment.)
 *
 * ========================================================================
 * THE OCTOPUS CAFE SEATING MODEL  (recovered here for the first time)
 *
 * The cafe is one map object with SIXTEEN chairs.  Its per-cell "user flags"
 * word -- the same 5-bit field Get_UserFlags/Set_UserFlags (sweep2.c) read
 * and write on any cell -- is used as a ROUND-ROBIN SEAT ALLOCATOR: a
 * customer arriving at stage 0 takes the current value as its seat number
 * and writes (seat + 1) & 0x1f back, so the next customer takes the seat
 * after it.  The counter is never checked for occupancy and wraps at 32
 * although only 16 seat pairs exist, so two customers 32 apart share a
 * chair -- reproduced, not fixed.
 *
 * Four contiguous const tables in .rdata drive the whole walk.  They are
 * laid out back to back (0x004b6990 .. 0x004b6c68) and the code indexes
 * across the boundaries, which is why the addresses look overlapping:
 *
 *   0x004b6990  g_cafe_pos[21]        {int x, y} in 24.8 world units.
 *                 [0]..[4]  the four approach waypoints outside the cafe
 *                           plus the door, [5]..[20] the sixteen chairs.
 *   0x004b6a34  g_cafe_walk[65]       the per-seat step list, four entries
 *                 per seat PAIR: g_cafe_walk[(seat>>1)*4 + stage] for
 *                 stage 1..4 gives the next g_cafe_pos index, or -1 for
 *                 "nothing to do at this stage, just advance".  Entry
 *                 (seat>>1)*4 + 4 is the chair itself, which is why the
 *                 chair lookups read the table 4 elements on (0x004b6a44).
 *                 The LEAVE walk (stages 13..16) reads the same rows
 *                 BACKWARDS, at (seat>>1)*4 + 17 - stage (0x004b6a78).
 *                 g_cafe_walk[0] is never read -- it aliases g_cafe_pos[20].y.
 *   0x004b6b38  g_cafe_chair_ofs[11]  {stand_x, stand_y, sit_x, sit_y}, the
 *                 offset from the chair's world position for a customer
 *                 STANDING beside it and SITTING on it, one row per chair
 *                 orientation.
 *   0x004b6be8  g_cafe_chair_dir[32]  seat -> orientation row (0..10).
 *
 * Every target is finally (chair.x + (cell.x << 8) + ofs - 0x80,
 * chair.y + (cell.y << 8) + ofs + 0x80): the cafe's cell in 24.8 plus the
 * table offset, biased by half a tile.
 *
 * The customer state machine (Bloke +0x60, nineteen stages) is:
 *
 *   0        claim a seat from the cell counter, flags |= 8, wait = 50
 *   1,2      wait out the 50 ticks (stage 2 only), then fall into the buy
 *   1..4     BuyItem(elem, cell, 1) then walk to g_cafe_walk[...]; a -1
 *            entry just advances the stage
 *   5        wait
 *   6        step to the STAND offset of the chair
 *   7        turn to face it: NewDirForAction(b, (dir - 4) & 7)
 *   8        step onto the SIT offset, mark +0x40 = 1 (the painter's
 *            "seated" flag), wait = 200
 *   9        sit: flags |= 0x100, f70 = 10, BlokeSitAnim + frame 0
 *   10       eat (the 200 ticks)
 *   11       stand up: flags &= ~0x100, BlokeWalkAnim, step back to STAND
 *   12       clear +0x40 and walk to the chair square
 *   13..16   retrace the approach walk backwards
 *   17       walk to the cafe's own origin square (class +0x0c/+0x10)
 *   18       RemoveBlokeFromRide and clear flags bit 8
 *
 * Stage 1/2 share a block that falls THROUGH into 3/4, and the 1..4 walk
 * and stage 12's walk share a tail (the compiler cross-jumped them); the C
 * reproduces that with plain fall-through and a `goto`.  Stage 6's copy of
 * the "face the target and advance" tail is written OUT IN FULL even though
 * the original ends up jumping into stage 12's copy -- see the note above
 * the marker: only the duplicated form keeps the deferred `add esp`.
 * ======================================================================== */

/* An {x,y} pair in 24.8 world units, passed by value. */
typedef struct Pos {
    int x;
    int y;
} Pos;

/* A placed object's map square, packed as two bytes. */
typedef struct MapSquare {
    unsigned char bx;          /* +0x00 */
    unsigned char by;          /* +0x01 */
} MapSquare;

typedef struct RideObject RideObject;

typedef struct RideElem {
    char*        name;         /* +0x00 */
    char*        image;        /* +0x04 */
    unsigned int flags;        /* +0x08 */
    RideObject*  data;         /* +0x0c */
} RideElem;

/* A person as the ride code sees it (bigsim.c's Bloke). */
typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;      /* +0x0e  low-level AI state (0 = idle) */
    unsigned char  pad10[0x14];
    Pos            target;     /* +0x24  walk target, 24.8 */
    unsigned char  pad2c[0x36 - 0x2c];
    unsigned char  seat;       /* +0x36  which of the cafe's 16 chairs */
    unsigned char  band;       /* +0x37  occlusion band inside a building */
    unsigned char  pad38[0x40 - 0x38];
    unsigned short seated;     /* +0x40  1 while on the chair (the painter
                                *        files these by seat instead of by
                                *        depth band) */
    unsigned char  pad42[0x5c - 0x42];
    int            wait;       /* +0x5c  countdown ticks */
    unsigned char  action;     /* +0x60  state-machine stage */
    unsigned char  pad61;
    unsigned short flags62;    /* +0x62  8 = using this ride, 0x100 = sitting */
    unsigned char  pad64[4];
    Pos            world;      /* +0x68  world position, 24.8 */
    unsigned short f70;        /* +0x70 */
    unsigned char  dir;        /* +0x72 */
    unsigned char  new_dir;    /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14]; /* +0x98  CalcMoveLine scratch */
} Bloke;

/* A rider slot: its bloke at +0x08 and the packed map square of the ride
 * instance it is using at +0x0c (rides.c's RiderNode). */
typedef struct RiderPerson RiderPerson;

typedef struct RiderNode {
    struct RiderNode* next;    /* +0x00 */
    struct RiderNode* prev;    /* +0x04 */
    Bloke*            bloke;   /* +0x08 */
    unsigned short    ride_id; /* +0x0c  packed {x,y} of the instance */
    unsigned short    pad0e;
    RiderPerson*      person;  /* +0x10 */
} RiderNode;

struct RideObject {
    unsigned char  pad00[0x0c];
    int            base_x;     /* +0x0c  the class's origin square */
    int            base_y;     /* +0x10 */
    unsigned char  pad14[0xcc - 0x14];
    RiderNode*     riders;     /* +0xcc  live rider list of the whole class */
};

/* ---- the cafe's const walk tables (.rdata, 0x004b6990..0x004b6c68) ------ */

extern Pos     g_cafe_pos[];        /* 0x004b6990 */
extern int     g_cafe_walk[];       /* 0x004b6a34 */
typedef struct CafeOfs { int stand_x; int stand_y; int sit_x; int sit_y; } CafeOfs;
extern CafeOfs g_cafe_chair_ofs[];  /* 0x004b6b38 */
extern int     g_cafe_chair_dir[];  /* 0x004b6be8 */

/* ---- callees ------------------------------------------------------------ */
extern unsigned short Get_UserFlags(int x, int y);                   /* 0x00461710 */
extern void   Set_UserFlags(int x, int y, unsigned short value);     /* 0x00461730 */
extern int    CalcMoveLine(Pos from, Pos to, void* path);            /* 0x00480740 */
extern int    NewDirForAction(Bloke* b, unsigned char dir);          /* 0x004833d0 */
extern void   BlokeSitAnim(Bloke* b);                                /* 0x00440780 */
extern void   BlokeSetFrame(Bloke* b, int frame);                    /* 0x00440870 */
extern void   BlokeWalkAnim(Bloke* b);                               /* 0x00440910 */
extern void   RemoveBlokeFromRide(RideObject* item, RiderNode* r);   /* 0x0048a100 */
extern void   BuyItem(RideElem* elem, MapSquare* at, int which);     /* 0x004539e0 */


/* 2026-09-04: audit.py mismatch 299 -> 51 of 404, and the body is now the
 * original's SIZE exactly (404 instructions / 1288 bytes, no ESCAPES).  Two
 * things closed the 248:
 *
 *  (a) Case 6 must carry its OWN copy of the `b->state = 7; NewDirForAction(b,
 *      (a >> 5) + 3); b->action++;` tail instead of a `goto` into case 12's.
 *      VC6 cross-jumps the two copies post-codegen anyway (case 6 ends in
 *      `jmp 0x431aba`, into case 12's copy), but only when they are separate
 *      statements does the CalcMoveLine cleanup stay DEFERRED: the original
 *      merges it with NewDirForAction's into one `add esp,0x1c` in the shared
 *      block.  With the `goto`, case 7 -- which reaches the same tail with an
 *      empty stack -- gets merged in too, the deferral is impossible, both
 *      cleanups are emitted separately (`add esp,0x14` + `add esp,8`) and
 *      case 7 loses its own six-instruction copy.  Same cause, both symptoms.
 *      Cases 13..16 and 17 already showed the merged `add esp,0x1c`/`0x20`.
 *  (b) The three chair-offset cases (6, 8, 11) need the waypoint AND the two
 *      offset words in registers before the sum.  A read into a plain `int`
 *      local is forward-substituted and folded back into the `add`; reading
 *      them into ONE-DIMENSIONAL ARRAY locals with constant indices
 *      (`int oa[2]`, `int wa[2]`) defeats forward substitution, keeps them in
 *      registers, costs no frame (VC6 scalarises a constant-indexed local
 *      array) and restores the missing four instructions.  `volatile` does
 *      the same but is dirtier and scores worse everywhere else.
 *      With the arrays in place the STATEMENT ORDER inside each case is a
 *      real lever and was searched exhaustively (120 + 840 + 20 valid orders,
 *      coordinate-descended to a fixed point): the winning orders all read
 *      the Y offset BEFORE the X one and put a store to `b` (`b->world =
 *      b->target`, `b->seated = 1`) between the offset reads and their use.
 *
 * WHAT IS LEFT (51 mismatches, all in cases 6/8/11, register naming only --
 * register-blind edit distance is 16).  The three-term sum
 * `waypoint + (cell << 8) + offset - 0x80` is associated the wrong way: the
 * original pairs (waypoint + cell) in the `add` and puts the offset in the
 * closing `lea` (`mov ebx,[ecx*8+g_cafe_pos] / add ebx,ecx /
 * lea edx,[ebx+edx-0x80]`); ours pairs (cell + offset) and puts the waypoint
 * in the lea.  Consequence: the cell byte's `xor r,r / mov r8,[edi] / shl r,8`
 * is hoisted several slots early into a callee-saved register instead of
 * sitting next to the `add`, and every register downstream is renamed.
 *
 * Ruled out for the association, all measured: all six textual orders of the
 * three terms and every parenthesisation (identical objects -- VC6 sorts the
 * flattened sum before instruction selection); the constant written at the
 * end, in the middle, or bracketed with one term; named `int` locals for any
 * subset of {waypoint, cell shift, offset} (48 combinations); a `const
 * CafeOfs*` / `const Pos*` pointer local (moves the fold, does not remove
 * it); `unsigned`/`long` locals with a narrowing cast; a two-statement
 * partial sum (`px = wp + cell; target = px + ofs - 0x80`); an inline helper
 * `CafeTarget(wp, cell, ofs, bias)` in four parameter orders; a whole-row
 * `CafeOfs` struct copy (grows the frame); one merged `int t[4]` for both
 * pairs in four index assignments (55, worse); `volatile` on the waypoint
 * (387 instructions, much worse).  The observed rank is: a MEMORY reference
 * pairs with the computed shift ahead of an array symbol, and an array symbol
 * pairs with it ahead of nothing -- so the pair can be made (cell, waypoint)
 * by leaving the waypoint inline, but then it is FOLDED (`add r,[mem]`) and
 * the body is four instructions short.  Getting the waypoint both FIRST and
 * in a register is the one thing no spelling reached.
 *
 * 2026-09-04 (sweep lane): 51 -> 49.  Declaring the offset pair `unsigned int
 * oa[2]` (or casting either offset to `unsigned` inside the three-term sum --
 * measured identical, and so is `unsigned wa[2]`) is worth two instructions:
 * the same-width conversion changes the operand ranking enough that case 6's
 * X and Y sums come out with the original's registers, so 112..123 there now
 * match index for index even though the ASSOCIATION is still (cell + offset)
 * rather than (waypoint + cell).  Arithmetic is unchanged modulo 2^32.
 * Also measured this pass, all worse: a partial-sum spelling with the array
 * local carrying `waypoint + cell` (`wa[0] = g_cafe_pos[step].x + (key->bx <<
 * 8); target.x = wa[0] + oa[0] - 0x80;`) -- VC6 folds the waypoint into the
 * add (`add ecx,[eax*8+g_cafe_pos]`, strict 265-273); `const Pos* wp` /
 * `const CafeOfs* co` pointer locals (306/314); a `Pos` struct copy of the
 * waypoint (82); plain `int` locals for the offsets (310); `key->bx * 256`
 * and `(int)key->bx << 8` (inert); and every textual order and
 * parenthesisation of the TWO-term sums in cases 12 and 13..16, whose single
 * residual each (0x38a and 0x422) is the same association shown as a `lea`
 * operand swap -- `lea ecx,[edx+ecx-0x80]` (waypoint as base) against ours
 * `lea ecx,[ecx+edx-0x80]`.  Note the original is NOT uniform: the X sums put
 * the waypoint first and the Y sums put the cell first, which tracks which of
 * the two values the emitted stream defines first.
 * The case-8 statement order was re-searched exhaustively (all 840
 * dependency-valid orders) with the unsigned offsets in place: the order
 * already in the file (world, row, oa[1], oa[0], seated, step, wa[0]) is the
 * unique best at 49; the next is 51.  Case 6's 180 valid orders were searched
 * too -- the order in the file is again the best.
 * What is left in case 6 is now pure SCHEDULING (indices 88..111): the
 * original emits the two `b->world` stores, then `row`, then the waypoint,
 * then stand_x, then stand_y, then the cell shift; ours hoists `row` to the
 * top and emits stand_y, waypoint, the world stores, stand_x, cell.
 *
 * 2026-09-04 (sweep6 lane).  No change to the code -- still 49 -- but the
 * association question is now REDUCED TO ONE CONCRETE OBSTACLE, and the
 * newest cross-lane lever was tested here and does not apply.
 *
 * 1. THE PARTIAL-SUM AGGREGATE CURE DOES NOT TRANSFER.  anim2.c's
 *    BoatingSchool_DrawBoats closed 79 mismatches by writing a commutative
 *    sum's leading pair into the fields of a non-address-taken aggregate
 *    (`Pos t; t.x = A + B; t.y = C; dst = t.x + t.y + D;`, two such
 *    aggregates so the field assignments stay contiguous).  Applied here to
 *    all three cases (`t.x = wa[0] + (key->bx << 8); t.y = oa[0];
 *    b->target.x = t.x + t.y - 0x80;` and the same for y) it grows the frame
 *    to 0x10, ESCAPES and scores 293.  Variants measured: the constant folded
 *    into `t.x`; only `t.x` protected; block-scope instead of function-scope
 *    aggregates; `oa` back to plain `int`.  All 112..312.  The old note's
 *    single-field attempt (`wa[0] = wp + cell; target = wa[0] + oa[0]`) was
 *    not a fair test of that lever -- a one-field aggregate is inert by
 *    construction -- but the fair two-field test now says no.
 *
 * 2. THE RULE THAT DECIDES THE ASSOCIATION IS THE ADD'S *DESTINATION*, and it
 *    is now characterised.  For the three-term sum both builds add the
 *    remaining operands in DESCENDING DEFINITION ORDER; they differ only in
 *    which operand becomes the destination:
 *      ours     destination = the compiler TEMPORARY (the `<< 8` shift, the
 *               last value defined), then + oa[0] (defined 2nd),
 *               then + wa[0] (defined 1st)   -> `add cell,ofs` + `lea +wp`
 *      original destination = the EARLIEST-DEFINED SYMBOL (the waypoint),
 *               then + the cell shift, then + the offset
 *                                            -> `add wp,cell` + `lea +ofs`
 *    So there is no temp in the original's sum: the cell shift must have been
 *    a NAMED value there too.
 *
 * 3. NAMING THE CELL SHIFT DOES FLIP THE DESTINATION -- and then loses on a
 *    PEEPHOLE.  With `int ca[2]; ca[0] = key->bx << 8;` written BEFORE
 *    `wa[0] = g_cafe_pos[step].x;` (definition order offsets, cell, waypoint)
 *    case 8 comes out as
 *        mov ebx, [eax*8 + g_cafe_pos] / add ebx, ecx / lea ecx,[ebx+ebp-0x80]
 *    which is the ORIGINAL, instruction for instruction.  But VC6 then
 *    compiles the standalone `ca[0] = key->bx << 8;` with its byte-into-the-
 *    high-half peephole -- `xor ecx,ecx / mov ch, byte ptr [edi]`, TWO
 *    instructions -- where the original has the three-instruction
 *    `xor ecx,ecx / mov cl,[edi] / shl ecx,8`.  One instruction short per
 *    sum, six short overall, everything downstream misaligns: 306.
 *    So the whole remaining problem is: NAME THE CELL SHIFT WITHOUT LETTING
 *    VC6 COLLAPSE `byte << 8` INTO `mov ch`.  Measured and rejected:
 *    `unsigned int ca[2]` (identical); the cell in `wa[]` with the waypoint
 *    left inline (309 -- the waypoint is still not folded, but the peephole
 *    still fires); a two-stage `ba[0] = key->bx; ca[0] = ba[0] << 8;`, which
 *    VC6 folds straight back to the inline form (byte-IDENTICAL to the
 *    current file, so array locals do NOT defeat forward substitution for
 *    this value the way they do for the offsets); `key->bx * 256` and
 *    `(int)key->bx << 8` (inert, as the old note already recorded).
 *    Per-case application (case 6 only / 8 only / 11 only / 8+11) is worse
 *    than all-three in every column: 211..313. */
// WIP-FUNCTION: LEGOLAND 0x004316f0  (404/404 insns, 1288/1288B, 49 by audit; cases 6/8/11 pair the sum the other way)
void OctopusCafe_Tick(RideElem* elem)
{
    RideObject*   item = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           seat;
    int           step;
    int           row;
    unsigned int  oa[2];
    int           wa[2];
    unsigned char a;

    r = item->riders;
    while (r) {
        b = r->bloke;
        next = r->next;
        key = (MapSquare*)&r->ride_id;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                seat = Get_UserFlags(key->bx << 8, key->by << 8);
                b->flags62 |= 8;
                b->seat = (unsigned char)seat;
                b->seated = 0;
                b->wait = 50;
                b->action++;
                Set_UserFlags(key->bx << 8, key->by << 8,
                              (unsigned short)((seat + 1) & 0x1f));
                break;
            case 1:
            case 2:
                if (b->action == 2 && b->wait-- >= 0)
                    break;
                /* falls through into 3/4 -- the original has no break */
            case 3:
            case 4:
                BuyItem(elem, key, 1);
                step = g_cafe_walk[(b->seat >> 1) * 4 + b->action];
                if (step == -1) {
                    b->action++;
                    break;
                }
                goto walk_to_step;
            case 5:
                if (b->wait-- < 0)
                    b->action++;
                break;
            case 6:
                step = g_cafe_walk[(b->seat >> 1) * 4 + 4];
                wa[0] = g_cafe_pos[step].x;
                row = g_cafe_chair_dir[b->seat];
                oa[1] = g_cafe_chair_ofs[row].stand_y;
                b->world = b->target;
                oa[0] = g_cafe_chair_ofs[row].stand_x;
                b->target.x = wa[0] + (key->bx << 8) + oa[0] - 0x80;
                wa[1] = g_cafe_pos[step].y;
                b->target.y = wa[1] + (key->by << 8) + oa[1] + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->new_dir = a;
                b->state = 7;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 7:
                NewDirForAction(b, (unsigned char)((b->dir - 4) & 7));
                b->action++;
                break;
            case 8:
                b->world = b->target;
                row = g_cafe_chair_dir[b->seat];
                oa[1] = g_cafe_chair_ofs[row].sit_y;
                oa[0] = g_cafe_chair_ofs[row].sit_x;
                b->seated = 1;
                step = g_cafe_walk[(b->seat >> 1) * 4 + 4];
                wa[0] = g_cafe_pos[step].x;
                b->target.x = wa[0] + (key->bx << 8) + oa[0] - 0x80;
                wa[1] = g_cafe_pos[step].y;
                b->target.y = wa[1] + (key->by << 8) + oa[1] + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->new_dir = a;
                b->state = 7;
                b->action++;
                b->wait = 200;
                break;
            case 9:
                b->world = b->target;
                b->flags62 |= 0x100;
                b->f70 = 10;
                BlokeSitAnim(b);
                BlokeSetFrame(b, 0);
                b->action++;
                break;
            case 10:
                if (b->wait-- < 0)
                    b->action++;
                break;
            case 11:
                b->flags62 &= ~0x100;
                b->f70 = 0;
                BlokeWalkAnim(b);
                row = g_cafe_chair_dir[b->seat];
                oa[1] = g_cafe_chair_ofs[row].stand_y;
                oa[0] = g_cafe_chair_ofs[row].stand_x;
                step = g_cafe_walk[(b->seat >> 1) * 4 + 4];
                wa[0] = g_cafe_pos[step].x;
                b->target.x = wa[0] + (key->bx << 8) + oa[0] - 0x80;
                wa[1] = g_cafe_pos[step].y;
                b->target.y = wa[1] + (key->by << 8) + oa[1] + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->new_dir = a;
                b->state = 7;
                b->action++;
                break;
            case 12:
                b->seated = 0;
                step = g_cafe_walk[(b->seat >> 1) * 4 + 4];
            walk_to_step:
                b->target.x = g_cafe_pos[step].x + (key->bx << 8) - 0x80;
                b->target.y = g_cafe_pos[step].y + (key->by << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->new_dir = a;
                b->state = 7;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 13:
            case 14:
            case 15:
            case 16:
                do {
                    step = g_cafe_walk[(b->seat >> 1) * 4 + 17 - b->action];
                    b->action++;
                } while (step == -1);
                b->target.x = g_cafe_pos[step].x + (key->bx << 8) - 0x80;
                b->target.y = g_cafe_pos[step].y + (key->by << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                break;
            case 17:
                BlokeWalkAnim(b);
                b->target.x = ((item->base_x + key->bx) << 8) + 0x80;
                b->target.y = ((item->base_y + key->by) << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 18:
                RemoveBlokeFromRide(item, r);
                b->flags62 &= ~8;
                break;
            }
        }
        r = next;
    }
}

/* =========================================================================
 * OCTOPUS CAFE, cb_b0 -- the painter (0x00431d00) and its one helper.
 *
 * The cafe is a single map object with EIGHT four-seat tables and a service
 * counter, and its customers sit INSIDE it, so the visitors and the
 * building's sprite layers have to be interleaved by depth instead of drawn
 * as two passes.  The painter therefore does its own two-list collect and
 * then walks the building front to back.
 *
 *   `seats[32]`  -- one slot per chair; a customer who has reached the
 *                   "seated" stage (Bloke +0x40) is filed by its seat number
 *                   (+0x36) so the table it belongs to can draw it between
 *                   its own two mask layers.
 *   `queue[n]`   -- everyone else (walking in, queueing, walking out), each
 *                   given a DEPTH BAND in +0x37 from
 *                       g_cafe_band[ 11*(wx - cell.x) - wy + cell.y ]
 *                   where wx/wy are the visitor's world position in whole
 *                   tiles.  The table (0x004b6d58) turns the offset from the
 *                   cafe's own cell into one of nineteen painter's-algorithm
 *                   layers, 1 (furthest) .. 0x13 (nearest).  It is indexed
 *                   WITHOUT bounds checking, so a visitor that has wandered
 *                   off the building's footprint reads past the table --
 *                   reproduced, not fixed.
 *
 * The paint order is then bands 1,2 -> table 4, bands 3,4 -> table 5,
 * 5,6 -> table 3, 7 -> table 2, 8,9,10 -> the counter sprite, 11,12 ->
 * table 6, 13,14 -> table 7, 15,16 -> table 1, 17,18 -> table 0, and
 * finally band 19 with nothing behind it.  Each table k is drawn with its
 * own sprite g_cafe_table[k] and its two chair masks g_cafe_chair_mask[2k]
 * and [2k+1]; all of them are filled by the class's cb_a4 (0x00431300).
 *
 * The seat numbers per table are NOT contiguous -- they are the order the
 * round-robin allocator hands them out -- so they are spelled out below.
 * ========================================================================= */

extern void  IP_RenderBlokeIn3DNow(Bloke* b);                        /* 0x00440010 */
extern int   PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */

/* The cafe's sprite handles (cb_a4 fills them). */
extern void* g_cafe_table[9];        /* 0x0081cd60  [0..7] tables, [8] counter */
extern void* g_cafe_chair_mask[16];  /* 0x0081cda0  two per table */
/* Offset-from-the-cafe-cell -> depth band, 1..0x13. */
extern int   g_cafe_band[];          /* 0x004b6d58 */

/* One table: its own sprite, then the two diners behind the first chair
 * mask, then the two in front of it behind the second.  A mask is only
 * blitted when there is somebody to hide behind it. */
// FUNCTION: LEGOLAND 0x00431c50
void CafeDrawTable(int x, int y, void* table, void* mask_back, void* mask_front,
                   Bloke* b1, Bloke* b2, Bloke* b3, Bloke* b4, int mode)
{
    PrintSprite(table, x, y, mode, 0);
    if (b1 || b2) {
        if (b1)
            IP_RenderBlokeIn3DNow(b1);
        if (b2)
            IP_RenderBlokeIn3DNow(b2);
        PrintSprite(mask_back, x, y, mode, 0);
    }
    if (b3 || b4) {
        if (b3)
            IP_RenderBlokeIn3DNow(b3);
        if (b4)
            IP_RenderBlokeIn3DNow(b4);
        PrintSprite(mask_front, x, y, mode, 0);
    }
}

/* Draw every collected walker whose depth band is `band`. */
static __inline void CafeDrawBand(Bloke** list, int n, int band)
{
    int i;
    for (i = 0; i < n; i++)
        if (list[i]->band == band)
            IP_RenderBlokeIn3DNow(list[i]);
}

// FUNCTION: LEGOLAND 0x00431d00
void OctopusCafe_Draw(RideElem* elem, int x, int y, MapSquare* sq,
                      void* clip, int mode)
{
    int         n;
    Bloke*      seats[32] = { 0 };
    Bloke*      queue[32];
    RideObject* item;
    RiderNode*  r;
    Bloke**     p;
    Bloke*      b;
    int         d;

    item = elem->data;
    n = 0;
    r = item->riders;
    if (r) {
        p = queue;
        do {
            if (*(unsigned short*)sq == r->ride_id) {
                b = r->bloke;
                if (b->seated) {
                    seats[b->seat] = b;
                } else {
                    *p = b;
                    d = ((b->world.x >> 8) - sq->bx) * 11
                        - (b->world.y >> 8) + sq->by;
                    b->band = (unsigned char)g_cafe_band[d];
                    n++;
                    p++;
                }
            }
            r = r->next;
        } while (r);
    }

    CafeDrawBand(queue, n, 1);
    CafeDrawBand(queue, n, 2);
    CafeDrawTable(x, y, g_cafe_table[4], g_cafe_chair_mask[8], g_cafe_chair_mask[9],
                  seats[14], seats[15], seats[12], seats[13], mode);
    CafeDrawBand(queue, n, 3);
    CafeDrawBand(queue, n, 4);
    CafeDrawTable(x, y, g_cafe_table[5], g_cafe_chair_mask[10], g_cafe_chair_mask[11],
                  seats[17], seats[18], seats[16], seats[19], mode);
    CafeDrawBand(queue, n, 5);
    CafeDrawBand(queue, n, 6);
    CafeDrawTable(x, y, g_cafe_table[3], g_cafe_chair_mask[6], g_cafe_chair_mask[7],
                  seats[10], seats[11], seats[8], seats[9], mode);
    CafeDrawBand(queue, n, 7);
    CafeDrawTable(x, y, g_cafe_table[2], g_cafe_chair_mask[4], g_cafe_chair_mask[5],
                  seats[4], seats[7], seats[5], seats[6], mode);
    CafeDrawBand(queue, n, 8);
    CafeDrawBand(queue, n, 9);
    CafeDrawBand(queue, n, 10);
    PrintSprite(g_cafe_table[8], x, y, mode, 0);
    CafeDrawBand(queue, n, 11);
    CafeDrawBand(queue, n, 12);
    CafeDrawTable(x, y, g_cafe_table[6], g_cafe_chair_mask[12], g_cafe_chair_mask[13],
                  seats[21], seats[22], seats[20], seats[23], mode);
    CafeDrawBand(queue, n, 13);
    CafeDrawBand(queue, n, 14);
    CafeDrawTable(x, y, g_cafe_table[7], g_cafe_chair_mask[14], g_cafe_chair_mask[15],
                  seats[24], seats[25], seats[26], seats[27], mode);
    CafeDrawBand(queue, n, 15);
    CafeDrawBand(queue, n, 16);
    CafeDrawTable(x, y, g_cafe_table[1], g_cafe_chair_mask[2], g_cafe_chair_mask[3],
                  seats[0], seats[3], seats[1], seats[2], mode);
    CafeDrawBand(queue, n, 17);
    CafeDrawBand(queue, n, 18);
    CafeDrawTable(x, y, g_cafe_table[0], g_cafe_chair_mask[0], g_cafe_chair_mask[1],
                  seats[28], seats[29], seats[30], seats[31], mode);
    CafeDrawBand(queue, n, 19);
}

/* =========================================================================
 * RESTAURANT 2, cb_b0 -- the painter (0x00430b10).
 *
 * The two-storey restaurant.  Like RESTAURANT 1 (ridecb1.c 0x0042f4c0) it
 * has to interleave its diners with its own sprite layers, but here the
 * interleaving depends on which way the building FACES: the per-square
 * record's +0x18 selects one of three completely different paint orders,
 * and the customers are picked out by their state-machine stage (+0x60)
 * rather than by an occlusion band.
 *
 *   facing 0,1  the two "side on" views: layer 5, stages 5 and 6, layer 6,
 *               stage 4, then the g_rest2_front overlay -- all four of them
 *               offset by the layer's own render offset.
 *   facing 2    stage 0xc (only when the record's style byte is set) behind
 *               g_rest2_floor, then g_rest2_walls, stage 6, g_rest2_upper
 *               and finally layer 3 (whose LLS frame is the record's door
 *               animation frame at +0x09).
 *   facing 4,5  stages 7,8,9 outside, then 0xd,0xe,0xf, then the same
 *               floor / walls / upper / layer-3 sequence.
 *   anything else draws nothing but the tail.
 *
 * The tail, shared by every facing, draws stages 0..3 -- the customers who
 * are still outside the building.
 *
 * TWO ASYMMETRIES reproduced verbatim: under facing 2 the g_rest2_upper
 * blit takes the layer's x offset but NOT its y (the y comes from the
 * record's +0x38 halved), and under facing 4/5 the g_rest2_floor blit takes
 * the layer-0 y offset but NOT its x.  Both look like slips in the original
 * -- the other blits add both halves -- but they are what it does.
 * The layer-6 offset is also REUSED for the final facing-0/1 overlay
 * instead of being fetched again.
 * ========================================================================= */

/* An {x,y} pair returned in eax:edx. */
typedef struct Offset {
    int ox;
    int oy;
} Offset;

typedef struct RenderObj RenderObj;

/* The render-item context block PrintSprite takes as its 5th argument. */
typedef struct DrawCtx {
    int            tag;        /* +0x00  0x103 */
    RideElem*      elem;       /* +0x04 */
    unsigned short square;     /* +0x08 */
} DrawCtx;

/* RESTAURANT 2's per-square record (list head 0x00616148). */
typedef struct Rest2Rec {
    unsigned char pad00[9];
    char          frame;       /* +0x09  door animation frame */
    unsigned char pad0a[0x11 - 0x0a];
    unsigned char style;       /* +0x11  second style byte */
    unsigned char pad12[0x18 - 0x12];
    int           facing;      /* +0x18  0..5 */
    unsigned char pad1c[0x38 - 0x1c];
    int           lift_a;      /* +0x38  vertical lift, halved when used */
    int           lift_b;      /* +0x3c */
} Rest2Rec;

extern Rest2Rec* Restaurant2_FindRec(MapSquare* sq);                 /* 0x0042f9d0 */
extern void   Restaurant2_AnimTick(MapSquare* sq, RideObject* item, int mode); /* 0x004304e0 */
extern Offset GetScreenCoordsForObject(MapSquare* inst, RideObject* item); /* 0x00442cc0 */
extern void   AdjustOffsetForViewMode(Offset* o);                    /* 0x00442d30 */
extern void   LLSSetFrame(void* lls, int frame);                     /* 0x0047d5a0 */
extern void*  GetLLSForLayer(RenderObj* obj, int layer);             /* 0x00441ea0 */
extern void*  GetSpriteForLayer(RenderObj* obj, int layer);          /* 0x00441ec0 */
extern Offset GetRenderOffsetForLayer(RenderObj* obj, int layer);    /* 0x00441ee0 */

extern RenderObj* g_rest2_layers;    /* 0x00616118  the class's layer holder */
extern void*      g_rest2_floor;     /* 0x0081cd84 */
extern void*      g_rest2_walls;     /* 0x0081cd20 */
extern void*      g_rest2_upper;     /* 0x0081cd34 */
extern void*      g_rest2_front;     /* 0x0081cd48 */

/* Draw every collected diner whose state-machine stage is `stage`. */
static __inline void Rest2DrawStage(Bloke** list, int n, int stage)
{
    int i;
    for (i = 0; i < n; i++)
        if (list[i]->action == stage)
            IP_RenderBlokeIn3DNow(list[i]);
}

/* Exact (530/530 instructions, 1541/1541 bytes).  Closed 2026-09-04 by the
 * EMPTY TRAILING `else { }` on the facing chain.
 *
 * What was wrong before: the eleven-instruction tail the facing-2 and
 * facing-4/5 arms share (the layer-3 GetSpriteForLayer + PrintSprite +
 * `mov ebx,[n]` + `jmp`, at 0x00430d43) was cross-jumped the WRONG WAY.  The
 * original keeps the facing-2 arm's copy and makes the facing-4/5 arm jump
 * BACKWARDS into it; without the empty else VC6 keeps the facing-4/5 copy and
 * makes facing-2 jump forward, displacing 171 of the 530 index positions
 * although every instruction was already right.
 *
 * The mechanism: the LAST arm of an if/else chain is the block that falls
 * through into the join, so its copy of a shared tail is free to keep while
 * the earlier arm's copy costs a `jmp` -- VC6 therefore deletes the EARLIER
 * arm's copy.  Give the chain one more (empty) arm and the facing-4/5 block
 * has to jump to the join like everyone else, the tie breaks the other way,
 * and the merge goes backwards into the facing-2 arm exactly as the original.
 * `else { }` is the only spelling that works: `else { stmt; }`, `else if
 * (facing == 3) { stmt; }` and a trailing separate `if` all add real code
 * (+14 .. +27 instructions, ESCAPES), and `else { n = n; }` is folded to a
 * `goto`-shaped chain that hoists the `n` reload out of the shared tail.
 *
 * Ruled out first, all inert (about 45 measured variants): every additive
 * spelling and both operand orders of the two sums in each arm's layer-3
 * PrintSprite (all 16 combinations byte-identical), bare-block / `do {} while
 * (0)` / `if (1)` wrappers on either tail, `(char)`/`(DrawCtx*)` casts, a
 * sprite temp (breaks the merge entirely, +8 and ESCAPES), nested if/else,
 * `if (facing != 0 && facing != 1)`, a switch (VC6 builds a jump table), and
 * six `goto`-label spellings -- all of which converge on one shape that moves
 * the `n` reload into the join block and loses an instruction.  Source order
 * IS a lever (`... else if (facing == 5 || facing == 4) {C} else if (facing
 * == 2) {B}` keeps the facing-2 copy) but it also moves the 5/4 dispatch
 * ahead of the 2 test, which the original does not do. */
// FUNCTION: LEGOLAND 0x00430b10
void Restaurant2_Draw(RideElem* elem, int x, int y, MapSquare* sq,
                      void* clip, int mode)
{
    Offset      off;
    char        frame;
    int         n;
    Offset      screen;
    DrawCtx     ctx;
    int         lift_a;
    int         lift_b;
    int         style;
    RideObject* item = elem->data;
    RiderNode*  r = item->riders;
    Rest2Rec*   rec;
    int         facing;

    ctx.elem = elem;
    ctx.square = *(unsigned short*)sq;
    {
    Bloke*      here[30] = { 0 };
    Bloke**     p;

    ctx.tag = 0x103;
    n = 0;
    rec = Restaurant2_FindRec(sq);
    if (!rec)
        return;
    frame = rec->frame;
    facing = rec->facing;
    style = rec->style;
    lift_a = rec->lift_a;
    lift_b = rec->lift_b;
    Restaurant2_AnimTick(sq, item, mode);
    screen = GetScreenCoordsForObject(sq, item);
    if (!r)
        return;

    p = here;
    do {
        if (*(unsigned short*)sq == r->ride_id) {
            *p = r->bloke;
            n++;
            p++;
        }
        r = r->next;
    } while (r);
    if (n == 0)
        return;

    if (facing == 0 || facing == 1) {
        off = GetRenderOffsetForLayer(g_rest2_layers, 5);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(g_rest2_layers, 5),
                    screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
        Rest2DrawStage(here, n, 5);
        Rest2DrawStage(here, n, 6);
        off = GetRenderOffsetForLayer(g_rest2_layers, 6);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(g_rest2_layers, 6),
                    screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
        Rest2DrawStage(here, n, 4);
        PrintSprite(g_rest2_front,
                    screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
    } else if (facing == 2) {
        if (style) {
            Rest2DrawStage(here, n, 12);
            PrintSprite(g_rest2_floor, screen.ox, lift_b / 2 + screen.oy,
                        mode, &ctx);
        }
        PrintSprite(g_rest2_walls, screen.ox, screen.oy, mode, &ctx);
        Rest2DrawStage(here, n, 6);
        off = GetRenderOffsetForLayer(g_rest2_layers, 6);
        AdjustOffsetForViewMode(&off);
        /* the y half of this offset is dropped -- see the header */
        PrintSprite(g_rest2_upper, screen.ox + off.ox,
                    lift_a / 2 + screen.oy, mode, &ctx);
        LLSSetFrame(GetLLSForLayer(g_rest2_layers, 3), frame);
        off = GetRenderOffsetForLayer(g_rest2_layers, 3);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(g_rest2_layers, 3),
                    screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
    } else if (facing == 5 || facing == 4) {
        Rest2DrawStage(here, n, 7);
        Rest2DrawStage(here, n, 8);
        Rest2DrawStage(here, n, 9);
        off = GetRenderOffsetForLayer(g_rest2_layers, 0);
        AdjustOffsetForViewMode(&off);
        Rest2DrawStage(here, n, 13);
        Rest2DrawStage(here, n, 14);
        Rest2DrawStage(here, n, 15);
        /* the x half of this offset is dropped -- see the header */
        PrintSprite(g_rest2_floor, screen.ox, screen.oy + off.oy, mode, &ctx);
        PrintSprite(g_rest2_walls, screen.ox, screen.oy, mode, &ctx);
        off = GetRenderOffsetForLayer(g_rest2_layers, 2);
        AdjustOffsetForViewMode(&off);
        PrintSprite(g_rest2_upper, screen.ox + off.ox, screen.oy + off.oy,
                    mode, &ctx);
        LLSSetFrame(GetLLSForLayer(g_rest2_layers, 3), frame);
        off = GetRenderOffsetForLayer(g_rest2_layers, 3);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(g_rest2_layers, 3),
                    screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
    } else {
        /* Every other facing draws nothing here.  This empty arm is NOT
         * cosmetic: it stops the facing-4/5 block being the last one before
         * the join, which is what makes VC6 cross-jump the shared layer-3
         * tail BACKWARDS into the facing-2 arm the way the original does.
         * See the note above the marker. */
    }

    Rest2DrawStage(here, n, 0);
    Rest2DrawStage(here, n, 1);
    Rest2DrawStage(here, n, 2);
    Rest2DrawStage(here, n, 3);
    }
}
