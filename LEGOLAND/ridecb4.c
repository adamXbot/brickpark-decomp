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
 *   0x004316f0  OCTOPUS CAFE     cb_a8    OctopusCafe_Tick          [WIP 87.6%]
 *   0x00431c50  OCTOPUS CAFE     (helper) CafeDrawTable             [OK]
 *   0x00431d00  OCTOPUS CAFE     cb_b0    OctopusCafe_Draw          [OK]
 *   0x00430b10  RESTAURANT 2     cb_b0    Restaurant2_Draw          [WIP 98.1%]
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
 * reproduces that with plain fall-through and a `goto`.
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


/* 87.6% full-body (359 of 410 by tools/matchfull; 404 instructions in ours
 * and 404 in the original, index-for-index mismatch 299 because one early
 * displacement shifts the rest).  Cases 0-5, 7, 9, 10, 12-18 are exact,
 * INCLUDING the 13..16 retrace loop, the cross-jumped 1..4 / 12 walk tail and
 * the `goto` from case 6 into it.  The residual is in the three cases that
 * add a CHAIR OFFSET (6, 8, 11): the original materialises the waypoint's x
 * AND both offset words into registers before the sum, e.g. at 0x00431833
 * `mov ebx,[ecx*8+g_cafe_pos]` / `add ebx,eax` / `lea edx,[ebx+edx-0x80]`,
 * where VC6 here folds one of the three loads into the `add`.
 *
 * What was measured: the expression's additive SPELLING is not a lever at all
 * -- six orderings and every parenthesisation compile to the identical object,
 * because VC6 reassociates the sum before instruction selection.  What DOES
 * force a table read into a register is a possibly-aliasing store between the
 * read and its use, which is why case 6's waypoint read is hoisted above the
 * `b->world = b->target` copy here (semantically free: the tables are const
 * data the copy cannot touch, and it buys the correct instruction count).
 * The same trick applied to cases 8 and 11 makes them WORSE (81.7% / 77.2%),
 * so it is not the mechanism the original used.  A pointer local for the
 * waypoint moves the fold onto the offset instead; `volatile` on either table
 * forces the load but then reassociates the sum the wrong way round.  The
 * missing lever is whatever keeps all three terms in registers at once. */
// WIP-FUNCTION: LEGOLAND 0x004316f0  (87.6%, cases 6/8/11 fold one table load)
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
    int           px;
    int           ox;
    int           oy;
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
                px = g_cafe_pos[step].x;
                b->world = b->target;
                row = g_cafe_chair_dir[b->seat];
                ox = g_cafe_chair_ofs[row].stand_x;
                oy = g_cafe_chair_ofs[row].stand_y;
                b->target.x = px + (key->bx << 8) + ox - 0x80;
                b->target.y = g_cafe_pos[step].y + (key->by << 8) + oy + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->new_dir = a;
                goto face_and_advance;
            case 7:
                NewDirForAction(b, (unsigned char)((b->dir - 4) & 7));
                b->action++;
                break;
            case 8:
                b->world = b->target;
                b->seated = 1;
                row = g_cafe_chair_dir[b->seat];
                ox = g_cafe_chair_ofs[row].sit_x;
                oy = g_cafe_chair_ofs[row].sit_y;
                step = g_cafe_walk[(b->seat >> 1) * 4 + 4];
                b->target.x = g_cafe_pos[step].x + (key->bx << 8) + ox - 0x80;
                b->target.y = g_cafe_pos[step].y + (key->by << 8) + oy + 0x80;
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
                ox = g_cafe_chair_ofs[row].stand_x;
                oy = g_cafe_chair_ofs[row].stand_y;
                step = g_cafe_walk[(b->seat >> 1) * 4 + 4];
                b->target.x = g_cafe_pos[step].x + (key->bx << 8) + ox - 0x80;
                b->target.y = g_cafe_pos[step].y + (key->by << 8) + oy + 0x80;
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
            face_and_advance:
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

/* 530/530 instructions and 1541/1541 bytes -- the same size as the original
 * to the byte, and every block matches instruction for instruction.  The one
 * residual is WHERE the compiler puts the eleven-instruction tail that the
 * facing-2 and facing-4/5 arms share (the layer-3 GetSpriteForLayer +
 * PrintSprite + `mov ebx,[n]` + `jmp` at 0x00430d43).  The original keeps the
 * copy that belongs to the facing-2 arm and makes the facing-4/5 arm jump
 * BACK into it; VC6 here cross-jumps the other way -- it keeps the facing-4/5
 * copy and makes the facing-2 arm jump forward -- which displaces 171 of the
 * 530 index positions even though the instructions themselves are right.
 * Measured: the direction follows the SOURCE order of the two arms (VC6 keeps
 * the LATER arm's copy), so writing them as `... else if (facing == 5 ||
 * facing == 4) {C} else if (facing == 2) {B}` does keep the facing-2 copy --
 * but then the dispatch tests 5/4 before 2, which the original does not.
 * Ten structural variants were measured (nested if/else, a leading `goto`
 * guard, a switch, an inlined shared helper, bare-block wraps, both `||`
 * orders); this spelling is the one whose DISPATCH is exact, so it is the one
 * kept.  Everything else in the function -- the frame, the collect loop, all
 * three facing arms, the stage loops and the tail -- is instruction-exact. */
// WIP-FUNCTION: LEGOLAND 0x00430b10  (98.1%, shared tail cross-jumped the other way)
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
    }

    Rest2DrawStage(here, n, 0);
    Rest2DrawStage(here, n, 1);
    Rest2DrawStage(here, n, 2);
    Rest2DrawStage(here, n, 3);
    }
}
