/* LEGOLAND -- the BOATING SCHOOL boat ANIMATION side (the leg start and the
 * 80-sub-step wobble/heading builder) and the SPACE TOWER's four cars (the
 * seat->car mapping, the record unlink and the three sprite passes), plus the
 * 3D animation offset accumulator the tower's queue step runs on.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are declared LOCALLY on purpose (legoland.h is owned
 * elsewhere) and mirror bswater.c / bswater2.c (the BsBoat and BsStation
 * records), ridemisc2.c (the boat walk and the tower's queue step),
 * mechrides.c (the six mechanical rides) and roads.c (the JUNGLE CRUISE twin
 * of the wobble builder).
 *
 *   addr        what it is                                       insns state
 *   0x004193c0  BsBoat_StartLeg          begin a leg, reset the dock  31 [OK]
 *   0x0043abc0  SpaceTower_RemoveRecord  unlink one placed copy       32 [OK]
 *   0x0043a9b0  SpaceTower_CountSeated   seats -> cars                34 [OK]
 *   0x0043ad00  SpaceTower_PlaceCar      a car's screen depth         41 [OK]
 *   0x0043ad90  SpaceTower_DrawCarOver   the seat matte over a car    45 [OK]
 *   0x0043a7a0  Anim3D_OffsetAt          accumulated animation offset 51 [OK]
 *   0x0043ae20  SpaceTower_DrawCarUnder  the car body                 58 [OK]
 *   0x004198a0  BsBoat_Animate           80 sub-steps and headings   334 [WIP]
 *
 * =========================================================================
 * MECHANICS RECOVERED
 * =========================================================================
 * THE SPACE TOWER'S FOUR CARS, in full.  mechrides.c owns the record (list
 * head 0x0062fda8) and bswater2.c named the four 36-byte car slots at
 * TowerRec +0x14..+0xa4; this file completes them.  A car is
 *      +0x00 flags, bit 0 = in service
 *      +0x08 how far the car has been wound up (BOTH draw passes subtract it
 *            from the car's cached anchor point, so this is the lift height)
 *      +0x0c the car's own little state: 0 idle, 2 loaded
 *      +0x14 cleared with the height whenever a car is (re)armed
 *      +0x18 / +0x1c the two rider nodes (bswater2.c)
 * and the eight SEAT slots at +0xa4 map to cars two at a time:
 * SpaceTower_CountSeated first takes every car out of service and clears its
 * state, then puts back into service any car with at least one occupied seat
 * (`car[seat >> 1]`, the same index mechrides.c's rider step uses) with state
 * 2 and its height and +0x14 zeroed.  So "the car is in service" is derived
 * from the seats every time the machine ticks, not tracked.
 *
 * THE FOUR CAR SPRITE TABLES, and a correction to mechrides.c.  Drawing one
 * car is three passes (bswater2.c's SpaceTower_DrawCar drives them):
 *   0x0043ae20  the car BODY -- class sprite layer 6, 4, 0 or 2 for cars 0..3,
 *               the same order SpaceTower_Create cached the four anchor
 *               points in at 0x0062fd88;
 *   0x0043ad00  the car's screen DEPTH -- the isometric Y of the car's own
 *               map square, cars 0 and 3 two squares out, cars 1 and 2 two in;
 *   0x0043ad90  the seat MATTE over the riders -- from a FOUR-POINTER table
 *               at 0x0062fd64 indexed by car.  mechrides.c names 0x0062fd64
 *               `g_spacetower_state` and 0x0062fd70 `g_spacetower_phase`;
 *               they are elements 0 and 3 of this one table, which
 *               SpaceTower_Create leaves null while it loads "SpaceTower Seat2
 *               Matte.lls" into [1] and "SpaceTower Seat3 Matte.lls" into [2].
 *               A null entry is exactly what makes cars 0 and 3 skip the over
 *               pass -- so only the two cars facing the camera get a matte.
 *               The mechrides.c names are wrong but are left alone.
 *
 * THE 3D ANIMATION OFFSET (0x0043a7a0).  A bloke animation is
 * {+0x04: AnimPart*[]}, each part {n frames, AnimStep[]}, each step 16 bytes
 * of {whole dx, whole dy, 1/256 dx, 1/256 dy}.  Anim3D_OffsetAt sums
 * `(whole << 8) + frac` over every frame of every part up to and including
 * (part, frame) and returns the 24.8 displacement as an 8-byte struct in
 * eax:edx.  That is how the space tower's queue step (bswater2.c's
 * SpaceTower_StepAnim) turns an animation cursor into a walk target: the
 * accumulated offset plus the ride's base square IS where the rider should
 * be standing on this frame.
 *
 * THE BOAT'S LEG START (0x004193c0).  Mover state 1: replay the crossing
 * animation leaving by SOUTH, aim one square south, go to the stepping state
 * (4), and reset the owning school's building animation frame and direction
 * so the jetty doors play from the top.  The station walk dies quietly if the
 * school has been removed.
 *
 * =========================================================================
 * ORIGINAL BUGS REPRODUCED
 * =========================================================================
 *  - SpaceTower_RemoveRecord walks the record list from the HEAD with no null
 *    test, so removing a record when the list is empty faults, and its
 *    trailing `if (p)` is dead on every path that reaches it.  This is the
 *    seventh copy of one source (the six in mechrides.c plus bswater2.c's
 *    PlaneRide_RemoveRecord) and they all have it.
 *  - SpaceTower_DrawCarUnder's sprite-layer `switch` has NO default arm, so a
 *    car index outside 0..3 blits layer `o.oy` -- VC6 colours the
 *    uninitialised local onto the offset's own y slot.  Unreachable.
 *  - SpaceTower_PlaceCar computes a whole screen-X chain that nothing reads;
 *    VC6 deletes the arithmetic (which is why the tile WIDTH out-parameter is
 *    never loaded) but may not delete the Get_XScroll() call, so the call is
 *    still there with its result thrown away.
 * ========================================================================= */

#include <math.h>

void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)

/* ---- shared types (same offsets as bswater.c / bswater2.c) -------------- */
typedef struct Pos { int x; int y; } Pos;

typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

extern int  HeapFree_w(void* p);                                /* 0x0049e4d0 */

/* =========================================================================
 * THE SPACE TOWER'S RECORD AND ITS FOUR CARS
 * =========================================================================
 * mechrides.c owns the placed-copy record (list head 0x0062fda8, keyed by the
 * packed map square at +0x00, eight seat slots at +0xa4).  bswater2.c named
 * the four 36-byte CAR slots that live between them at +0x14..+0xa4.  This
 * file fills in the rest of a car: +0x00 is a flag word whose bit 0 says the
 * car is in service, +0x08 is the height the car has been wound up to (both
 * the draw passes subtract it from the car's cached anchor point) and +0x14
 * is cleared with it whenever a car is (re)armed, and +0x0c is the car's own
 * little state (0 idle, 2 loaded).
 * ========================================================================= */

typedef struct RiderNode RiderNode;

typedef union RideTile {
    unsigned short key;             /* +0x00 */
    struct { unsigned char x, y; } b;
} RideTile;

typedef struct TowerCar {
    int               flags;        /* +0x00 bit 0 = this car is in service */
    unsigned char     pad04[4];
    int               height;       /* +0x08 how far the car is wound up */
    int               state;        /* +0x0c 0 idle, 2 loaded */
    unsigned char     pad10[4];
    int               f14;          /* +0x14 */
    RiderNode*        rider_a;      /* +0x18 (bswater2.c) */
    RiderNode*        rider_b;      /* +0x1c (bswater2.c) */
    unsigned char     pad20[4];
} TowerCar;                         /* 0x24 */

typedef struct TowerRec {
    RideTile          tile;         /* +0x00 */
    unsigned char     seated;       /* +0x02 */
    unsigned char     riders;       /* +0x03 */
    unsigned char     joined;       /* +0x04 */
    unsigned char     pad05[3];
    struct TowerRec*  next;         /* +0x08 */
    unsigned char     pad0c[0x14 - 0x0c];
    TowerCar          car[4];       /* +0x14 .. +0xa4 */
    unsigned char     seat[8];      /* +0xa4 one slot per seat, 0 = free */
    signed char       frame3;       /* +0xac */
    signed char       frame5;       /* +0xad */
    unsigned char     padae[2];
    int               timer;        /* +0xb0 */
} TowerRec;                         /* 0xb4 */

extern TowerRec* g_tower_recs;                                  /* 0x0062fda8 */

/* =========================================================================
 * 0x0043abc0 -- SpaceTower_RemoveRecord: unlink one placed copy and free it.
 *
 * The mechanical rides' record unlink, compiled a seventh time (the six named
 * in mechrides.c plus bswater2.c's PlaneRide_RemoveRecord).  It has the same
 * hole as all of them: the walk starts at the list HEAD with no null test, so
 * removing a record when the list is empty faults, and the trailing `if (p)`
 * is dead on every path that reaches it.  The single-call tail is short
 * enough that VC6 tail-DUPLICATES it into both arms here, where the Plane
 * ride's two-call tail is jumped to instead.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0043abc0
void SpaceTower_RemoveRecord(TowerRec* rec)
{
    if (g_tower_recs == rec) {
        g_tower_recs = rec->next;
    } else {
        /* joust.c's solved link-pointer walk: the ONE volatile read on the
         * link deref is what stops VC6 forwarding the condition's load into
         * the body's, and `link` must be declared BEFORE `node` and seeded
         * from the GLOBAL so it takes the first register. */
        TowerRec** link = &g_tower_recs->next;
        TowerRec*  node = g_tower_recs;

        while (*link != rec) {
            node = *(TowerRec* volatile*)link;
            if (node == 0)
                break;
            link = &node->next;
        }
        /* Dead on every path that reaches it -- the original emits it. */
        if (node != 0)
            node->next = rec->next;
    }
    HeapFree_w(rec);
}

/* =========================================================================
 * 0x0043a9b0 -- SpaceTower_CountSeated: rebuild the cars from the seat slots.
 *
 * Two seats per car (seat >> 1 is the car, as mechrides.c's rider step also
 * reads it).  Every car is first taken OUT of service and reset, then any car
 * with at least one occupied seat is put back in with state 2.  The four
 * per-car resets run off a strength-reduced cursor with a down-counting trip
 * register; the seat scan keeps the plain induction variable because the car
 * subscript needs it.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0043a9b0
void SpaceTower_CountSeated(TowerRec* rec)
{
    int i;

    for (i = 0; i < 4; i++) {
        rec->car[i].flags &= ~1;
        rec->car[i].state = 0;
    }
    for (i = 0; i < 8; i++) {
        if (rec->seat[i] != 0) {
            rec->car[i >> 1].flags |= 1;
            rec->car[i >> 1].state = 2;
            rec->car[i >> 1].f14 = 0;
            rec->car[i >> 1].height = 0;
        }
    }
}

/* =========================================================================
 * THE BOATING SCHOOL'S BOATS
 * ========================================================================= */

typedef struct BsWobble { int x; int y; } BsWobble;

typedef struct Bloke Bloke;

typedef struct BsBoat {
    BPosW          key;             /* +0x00  the station that launched it */
    unsigned char  pad02[2];
    int            cx;              /* +0x04  the map square it is on */
    int            cy;              /* +0x08 */
    int            nx;              /* +0x0c  the square it is heading for */
    int            ny;              /* +0x10 */
    int            sx;              /* +0x14  screen position, this frame */
    int            sy;              /* +0x18 */
    BsWobble       wob[0x50];       /* +0x1c   80 precomputed sub-steps */
    int            frame[0x50];     /* +0x29c  80 precomputed sprite codes */
    int            entry;           /* +0x3dc the side it came in by */
    int            hull;            /* +0x3e0 which hull sprite set */
    int            state;           /* +0x3e4  the mover's state */
    int            leg;             /* +0x3e8  squares left in this leg */
    Bloke*         rider;           /* +0x3ec  the single passenger */
    struct BsBoat* next;            /* +0x3f0 */
} BsBoat;                           /* 0x3f4 */

/* ridecb5.c's placed boating school. */
typedef struct BsStation {
    unsigned short     key;         /* +0x00 packed map square */
    unsigned char      ax;          /* +0x02 route start x */
    unsigned char      ay;          /* +0x03 route start y */
    unsigned char      bx;          /* +0x04 route end x */
    unsigned char      by;          /* +0x05 route end y */
    unsigned char      pad06[2];
    void*              route;       /* +0x08 */
    int                frame;       /* +0x0c animation frame */
    int                backwards;   /* +0x10 animation direction */
    int                count;       /* +0x14 visitors queueing */
    Bloke*             q[5];        /* +0x18 the five queue slots */
    struct BsStation*  next;        /* +0x2c */
    int                take;        /* +0x30 accumulated takings */
} BsStation;                        /* 0x34 */

extern BsStation* g_bs_stations;                                /* 0x004cc074 */

/* Rebuild the boat's 80 sub-steps and sprite codes for a crossing that
 * enters by `from` and leaves by `to` (-1 = stall in place). */
extern void BsBoat_Animate(BsBoat* b, int from, int to);        /* 0x004198a0 */

/* =========================================================================
 * 0x004193c0 -- BsBoat_StartLeg: the mover's state 1, one tick after a boat
 * is launched.  Lay down a crossing that leaves by SOUTH (4) from whichever
 * side the boat came in by, aim one square south, go to the stepping state,
 * and reset the owning school's building animation (frame and direction) so
 * the jetty doors play from the top.  The station walk is the usual rotated
 * list search and dies quietly if the school has already been removed.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004193c0
void BsBoat_StartLeg(BsBoat* b)
{
    BsStation* st = g_bs_stations;

    BsBoat_Animate(b, b->entry, 4);
    b->state = 4;
    b->ny = b->cy + 5;
    while (st != 0) {
        if (st->key == b->key.w) {
            st->frame = 0;
            st->backwards = 0;
            return;
        }
        st = st->next;
    }
}

/* =========================================================================
 * THE 3D CHARACTER ANIMATION'S ACCUMULATED TRANSLATION
 * =========================================================================
 * blokeanim.c's animation object: +0x04 is an array of PART pointers, one per
 * part of the animation, and each part is {n frames, AnimStep[]}.  A step is
 * a 16-byte per-frame translation in TWO halves -- whole map units at +0x00 /
 * +0x04 and 1/256ths at +0x08 / +0x0c -- so the accumulated offset is
 * `(whole << 8) + frac` summed over every frame of every part up to and
 * including (part, frame).  The result is the 24.8 displacement a bloke
 * playing that animation has travelled, which is how the space tower's queue
 * step turns an animation cursor into a walk target.
 * ========================================================================= */

typedef struct AnimStep {
    int dx;                         /* +0x00 whole map units */
    int dy;                         /* +0x04 */
    int fx;                         /* +0x08 1/256ths */
    int fy;                         /* +0x0c */
} AnimStep;                         /* 0x10 */

typedef struct AnimPart {
    int       n;                    /* +0x00 frames in this part */
    AnimStep* step;                 /* +0x04 */
} AnimPart;

typedef struct Anim3D {
    unsigned char pad00[4];
    AnimPart**    part;             /* +0x04 */
} Anim3D;

/* TWO LEVERS, one of them the REVERSE of bswater2.c's recorded rule:
 *  - THE TWO HALVES ARE ACCUMULATED IN PLAIN `int` LOCALS and assigned into
 *    the returned `Pos` at the end.  bswater2.c's SpaceTower_StepAnim needed
 *    the 8-byte struct return to BE the accumulator (`p.x += ...`), but there
 *    the sum is straight-line; here it is a LOOP accumulation, and `r.x += e`
 *    inside the loop costs six instructions: VC6 keeps the outer part cursor
 *    in EBX across the inner loop, that leaves the inner loop one scratch
 *    register, and it then folds both fractional loads into `add reg,[mem]`
 *    memory operands -- 45 instructions for the original's 51.  With plain
 *    `int` accumulators the cursor spills to the dead `anim` argument slot,
 *    the inner loop gets EBX and EBP, and all four fields are loaded into
 *    registers.  Exact.  So a memory-operand FOLD inside a loop is a
 *    register-PRESSURE symptom: look at what is held across the loop, not at
 *    the expression.  (Measured against ~50 other spellings: four free
 *    volatile reads on the fractional fields do force the loads and reach
 *    51/51 with 10 mismatches, but VC6 then reassociates the sum into
 *    `add reg,eax / lea eax,[..]`; the cursor written out as a walking
 *    pointer, as `anim->part[i]` spelled twice, volatile-homed, or assigned
 *    into the parameter itself are all 45-49.)
 *  - THE `if` IS WRITTEN FALSE-ARM-FIRST.  `if (i != part) n = q->n; else
 *    n = frame + 1;` puts the `q->n` load inline and exiles the frame arm,
 *    which is the original's `je` direction; the positive form inverts both.
 * The `for (i = 0; i < part + 1; i++)` guard is the recorded "a loop guard
 * comparing <expr> + 1, with no decrement anywhere, proves the source counted
 * from + 1" shape -- `i <= part` would emit `cmp edi,ebx / jle`. */
// FUNCTION: LEGOLAND 0x0043a7a0
Pos Anim3D_OffsetAt(Anim3D* anim, int part, int frame)
{
    Pos r;
    int i;
    int j;
    int n;
    int sx = 0;
    int sy = 0;

    for (i = 0; i < part + 1; i++) {
        AnimPart* q = anim->part[i];

        if (i != part)
            n = q->n;
        else
            n = frame + 1;
        for (j = 0; j < n; j++) {
            sx += (q->step[j].dx << 8) + q->step[j].fx;
            sy += (q->step[j].dy << 8) + q->step[j].fy;
        }
    }
    r.x = sx;
    r.y = sy;
    return r;
}

/* =========================================================================
 * THE SPACE TOWER'S THREE SPRITE PASSES
 * ========================================================================= */

typedef struct Offset { int ox; int oy; } Offset;

typedef struct RideDef RideDef;

extern RideDef* g_spacetower_def;                               /* 0x0062fd74 */
extern void*    g_spacetower_layers;                            /* 0x0062fd60 */
/* The four cached car anchor points (layers 6, 4, 0, 2 in that order). */
extern Offset   g_spacetower_car_ofs[4];                        /* 0x0062fd88 */
/* The per-car OVER matte.  mechrides.c names 0x0062fd64 g_spacetower_state
 * and 0x0062fd70 g_spacetower_phase; they are elements 0 and 3 of THIS
 * four-pointer table, which SpaceTower_Create leaves null while it loads
 * "SpaceTower Seat2 Matte.lls" into [1] and "SpaceTower Seat3 Matte.lls"
 * into [2].  A null entry is what makes cars 0 and 3 skip the over pass. */
extern void*    g_spacetower_car_matte[4];                      /* 0x0062fd64 */

extern Offset GetScreenCoordsForObject(void* inst, RideDef* item); /* 0x00442cc0 */
extern void   AdjustOffsetForViewMode(Offset* o);               /* 0x00442d30 */
extern void*  GetSpriteForLayer(void* sprite, int layer);       /* 0x00441ec0 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern void   GetTileDimensions(int* out_w, int* out_h);        /* 0x00460540 */
extern short  Get_XScroll(void);                                /* 0x004615f0 */
extern short  Get_YScroll(void);                                /* 0x00461600 */

typedef struct MapConfig {
    unsigned char  pad00[0x20];
    unsigned short ox;              /* +0x20 render origin */
    unsigned short oy;              /* +0x22 */
} MapConfig;

extern MapConfig* g_map_cfg;                                    /* 0x004bcbf4 */

/* =========================================================================
 * 0x0043ae20 -- SpaceTower_DrawCarUnder: the lower half of one car.
 *
 * The car's anchor point, raised by however far the car has been wound up
 * (car +0x1c), turned into screen space and blitted with the class sprite's
 * layer for this car -- 6, 4, 0 and 2, the same order SpaceTower_Create
 * cached the four anchors in.  There is no default arm: `layer` is left
 * UNINITIALISED, and VC6 colours it onto the offset's own y slot, so a car
 * index outside 0..3 would blit layer `o.oy`.  Unreachable (the ride only
 * ever draws cars 0..3) and original.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0043ae20
void SpaceTower_DrawCarUnder(TowerRec* rec, int car, int mode)
{
    Offset scr = GetScreenCoordsForObject(rec, g_spacetower_def);
    Offset o;
    int    layer;

    o = g_spacetower_car_ofs[car];
    o.oy -= rec->car[car].height;
    AdjustOffsetForViewMode(&o);
    switch (car) {
    case 0:
        layer = 6;
        break;
    case 1:
        layer = 4;
        break;
    case 2:
        layer = 0;
        break;
    case 3:
        layer = 2;
        break;
    }
    PrintSprite(GetSpriteForLayer(g_spacetower_layers, layer),
                scr.ox + o.ox, scr.oy + o.oy, mode, 0);
}

/* =========================================================================
 * 0x0043ad90 -- SpaceTower_DrawCarOver: the seat matte over the riders.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0043ad90
void SpaceTower_DrawCarOver(TowerRec* rec, int car, int mode)
{
    Offset scr = GetScreenCoordsForObject(rec, g_spacetower_def);
    Offset o;

    if (g_spacetower_car_matte[car] != 0) {
        Offset p;

        o = g_spacetower_car_ofs[car];
        o.oy -= rec->car[car].height;
        AdjustOffsetForViewMode(&o);
        p.ox = scr.ox + o.ox;
        p.oy = scr.oy + o.oy;
        PrintSprite(g_spacetower_car_matte[car], p.ox, p.oy, mode, 0);
    }
}

/* =========================================================================
 * 0x0043ad00 -- SpaceTower_PlaceCar: the car's screen depth.
 *
 * The isometric Y of the car's own map square: cars 0 and 3 sit two squares
 * further out, cars 1 and 2 two squares in.  Only the y half survives -- the
 * x chain is dead-code-eliminated down to its Get_XScroll() call, which is
 * why the tile WIDTH out-parameter is never read.  Original.
 * ========================================================================= */

/* THE TWO 24.8 WORLD COORDINATES MUST BE ONE AGGREGATE.  As two plain `int`
 * locals VC6 distributes the shift out of the sum -- `(x<<8) + (y<<8)` becomes
 * `(x + y) << 8` and the pair of `shl`s moves BELOW the `imul` (`imul / shl 8 /
 * sar 9`), which is one instruction long and shifts everything from index 0.
 * Wrapped in one struct the distribution does not run and both `shl`s stay
 * ahead of the call, where the original has them.  (Confirmed from the other
 * side in an isolated kernel: with the dead x chain kept ALIVE the shifts
 * survive as two `int`s too, so the transform is the algebra pass running
 * after dead-code elimination has left each shift a single use.)  The pair is
 * then assigned Y FIRST -- the recorded "two derived values are emitted in
 * REVERSE source order" tie-break -- which is what puts x<<8 in EDI and y<<8
 * in ESI. */
// FUNCTION: LEGOLAND 0x0043ad00
int SpaceTower_PlaceCar(TowerRec* rec, int car)
{
    int x = rec->tile.b.x;
    int y = rec->tile.b.y;
    Pos w;
    int tw;
    int th;
    int sx;
    int sy;

    switch (car) {
    case 0:
    case 3:
        x += 2;
        y += 2;
        break;
    case 1:
    case 2:
        x -= 2;
        y -= 2;
        break;
    }
    w.y = y << 8;
    w.x = x << 8;
    GetTileDimensions(&tw, &th);
    /* The x half is dead -- nothing reads `sx` and the tile WIDTH out-param is
     * never loaded -- so VC6 deletes all of its arithmetic and leaves only the
     * Get_XScroll() call, which it may not delete.  Original. */
    sx = ((w.x - w.y) * tw) >> 9;
    sy = ((w.y + w.x) * th) >> 9;
    sx += g_map_cfg->ox - Get_XScroll();
    sy += g_map_cfg->oy - Get_YScroll();
    return sy;
}

/* =========================================================================
 * 0x004198a0 -- BsBoat_Animate: lay down the 80 sub-step offsets a boat
 * follows while it crosses one 5x5 lake square, and the 80 heading codes that
 * go with them.
 *
 * This is the SAME SOURCE as the jungle cruise's JcBoat_Animate (roads.c
 * 0x00433840) with the boating school's tables and one extra term in the
 * heading tail; read that function's note for the geometry.  In short:
 * `from` is the side the boat came in by and `to` the side it leaves by, both
 * single-bit masks (1 N, 2 E, 4 S, 8 W) or -1 for "none", and everything is
 * table-driven off g_bs_step (0x004b5118), indexed by the BIT INDEX of a side
 * (N=0, E=1, S=2, W=3): {dx,dy} is the direction a boat that entered by that
 * side travels in, {sx,sy} the unit position of that side's edge.  The wobble
 * buffer is in 1/16 map units and one square is 640 of them, so the entry
 * offset is `edge * 40 * 16.0f` and a crossing is 80 sub-steps of `step * 16`.
 * The five cases, in test order: stuck (zero the buffer), drift in, drift
 * out, U-turn, crossing -- and a crossing that is not straight through is a
 * real quarter ARC swept from g_bs_curve_cw (0x004b5158) or _ccw (0x004b5198),
 * {a0, a1, ox, oy} per entry side, 640 units of radius recentred on the
 * corner.  The shipped tables are identical to the jungle cruise's.
 *
 * THE ONE DIFFERENCE FROM THE TWIN is the heading tail: the boating school's
 * boats come in more than one hull colour, so the 0..15 heading code is
 * biased by the boat's own hull index (+0x3e0) times 16 --
 *      frame[j] = (((ArcTan256(dx, dy) >> 4) + 6) & 0xf) + (b->hull << 4)
 * -- which is why the sprite codes run 0..0x3f rather than 0..0xf.  The
 * three instructions that costs, plus the `xor ebp,ebp` the extra register
 * pressure hoists into the two literal zeros of the drift-out arm, are the
 * whole 334-against-330 size difference.
 * ========================================================================= */

typedef struct BsStep  { int dx; int dy; int sx; int sy; } BsStep;
typedef struct BsCurve { float a0; float a1; float ox; float oy; } BsCurve;

extern BsStep  g_bs_step[4];        /* 0x004b5118 */
extern BsCurve g_bs_curve_cw[4];    /* 0x004b5158  N->E->S->W->N */
extern BsCurve g_bs_curve_ccw[4];   /* 0x004b5198  N->W->S->E->N */

extern int ArcTan256(int x, int y);                             /* 0x004806e0 */

/* STATE: 334/334 instructions, 1133/1133 BYTES, THREE mismatches at indices
 * 277-279 -- the straight-run y product, and it is the SAME residual as the
 * jungle cruise twin, index for index:
 *      orig   mov edx,edi / mov [ebp-4],eax / imul edx,[g_bs_step+i*16+4]
 *      ours   mov [ebp-4],eax / mov edx,[g_bs_step+i*16+4] / imul edx,edi
 * roads.c's JcBoat_Animate (0x00433840) carries the full RETIRED analysis of
 * exactly this window -- ~100 spellings, a corpus scan of every two-operand
 * `imul` in the exact bodies, and the conclusion that `mov r,reg / imul r,[mem]`
 * needs its register operand to be a rank-1 compiler TEMPORARY, which no
 * zero-cost C expression makes out of an enregistered induction variable
 * (`(short)j` reaches ONE mismatch at the price of `movsx ecx,di` and one
 * byte).  roads.c predicted from the disassembly alone that this unported twin
 * had the identical form; that is now CONFIRMED from the C side -- both are
 * byte-exact everywhere else and differ at the same three indices, which is
 * itself the proof that the two are one piece of source compiled twice.  Do
 * not re-open it here; a fix on either closes both. */
/* CLOSED (Scope F continuation, 2026-09-05): 334/334 instructions,
 * 1133/1133 bytes, strict/rb/ob 0/0/0, audit [OK].  The three-instruction
 * residual at 277-279 was never in the STRAIGHT loop it appeared in: it is
 * a remote effect of how the U-TURN loop above spells its x product.
 *
 * Naming the u-turn loop's x product in a `float` local -- either the whole
 * scaled product or just `dx * j` -- makes the straight loop 170 instructions
 * below emit the original's operand rank, `mov edx,edi / imul edx,[table]`,
 * instead of our `mov edx,[table] / imul edx,edi`.  The local is a FRONT-END
 * effect only: no store, reload, conversion or stack slot is emitted for it,
 * and the body is byte-for-byte the original either way (1133 bytes).  What
 * it changes is the typed temporary VC6 carries into its later ranking
 * decision -- which is why `double` is inert while `float` closes the body.
 *
 * LOAD-BEARING and measured on this body:
 *   - the local must carry the X expression: naming Y instead is inert (3),
 *     naming BOTH re-schedules the whole loop (283 strict, 1135B);
 *   - it must be `float`: `double` is inert (3);
 *   - it must be the product, not the sum: naming `(dx*j)*16.0f` gives 0 and
 *     naming `dx*j` gives 0, but naming `(dx*j)*16.0f + x0` is inert (3).
 * The earlier note's operand-rank "floor" was a floor only for the families
 * it had tested, all of which spelled the straight loop; that retirement is
 * superseded.  The same one-line change closes the twin JcBoat_Animate
 * (roads.c, 0x00433840) index for index, as its note predicted. */
// FUNCTION: LEGOLAND 0x004198a0
void BsBoat_Animate(BsBoat* b, int from, int to)
{
    BsCurve* c = 0;
    int      i;
    int      j;
    int      k;
    int      x0;
    int      y0;

    if (to == -1) {
        if (from == to) {
            memset(b->wob, 0, sizeof(b->wob));
            goto frames;
        }
        for (i = 0; i < 4; i++) {
            if (from & (1 << i))
                break;
        }
        x0 = (int)((g_bs_step[i].sx * 40) * 16.0f);
        y0 = (int)((g_bs_step[i].sy * 40) * 16.0f);
        for (j = 0; j < 0x50; j++) {
            if (j < 0x28) {
                /* CODEGEN LEVER: this float local is what gives the STRAIGHT
                 * loop's y product (170 instructions below) the original's
                 * `mov r,j / imul r,[table]` operand rank; see the note above. */
                float fx = (g_bs_step[i].dx * j) * 16.0f;
                b->wob[j].x = (int)(fx + x0);
                b->wob[j].y = (int)((g_bs_step[i].dy * j) * 16.0f + y0);
            } else {
                b->wob[j].x = 0;
                b->wob[j].y = 0;
            }
        }
        goto frames;
    }
    if (from == -1) {
        for (i = 0; i < 4; i++) {
            if (to & (1 << i))
                break;
        }
        k = (i + 2) % 4;
        for (j = 0; j < 0x50; j++) {
            if (j >= 0x28) {
                b->wob[j].x = b->wob[j - 1].x + g_bs_step[k].dx * 16;
                b->wob[j].y = b->wob[j - 1].y + g_bs_step[k].dy * 16;
            } else {
                b->wob[j].x = 0;
                b->wob[j].y = 0;
            }
        }
        goto frames;
    }
    if (from == 1)
        from = 0x11;
    if (to == 1)
        to = 0x11;
    if (to < from) {
        if (!(to & (from >> 2)))
            goto curve;
    } else if (to != from) {
        if (!(from & (to >> 2)))
            goto curve;
    }
    for (i = 0; i < 4; i++) {
        if (from & (1 << i))
            break;
    }
    x0 = (int)((g_bs_step[i].sx * 40) * 16.0f);
    y0 = (int)((g_bs_step[i].sy * 40) * 16.0f);
    if (from == to) {
        for (j = 0, k = 0x50; k > 0; j++, k--) {
            if (k > 0x28) {
                b->wob[j].x = (int)((g_bs_step[i].dx * j) * 16.0f + x0);
                b->wob[j].y = (int)((g_bs_step[i].dy * j) * 16.0f + y0);
            } else {
                b->wob[j].x = (int)((g_bs_step[i].dx * k) * 16.0f + x0);
                b->wob[j].y = (int)((g_bs_step[i].dy * k) * 16.0f + y0);
            }
        }
        goto frames;
    }
    for (j = 0; j < 0x50; j++) {
        b->wob[j].x = (int)((g_bs_step[i].dx * j) * 16.0f + x0);
        b->wob[j].y = (int)((j * g_bs_step[i].dy) * 16.0f + y0);
    }
    goto frames;

curve:
    if (to & (from * 2))
        c = g_bs_curve_cw;
    else if (from & (to * 2))
        c = g_bs_curve_ccw;
    for (i = 0; i < 4; i++) {
        if (from & (1 << i))
            break;
    }
    {
        float step = (c[i].a1 - c[i].a0) * 0.012500000186264515f;
        float a = c[i].a0;

        b->wob[0].x = (int)(((float)sin(a * 0.01745329238474369f) + c[i].ox) * 640.0f);
        b->wob[0].y = (int)(((float)cos((a + 180.0f) * 0.01745329238474369f) + c[i].oy) * 640.0f);
        j = 1;
        k = 0x4f;
        do {
            a += step;
            b->wob[j].x = (int)(((float)sin(a * 0.01745329238474369f) + c[i].ox) * 640.0f);
            b->wob[j].y = (int)(((float)cos((a + 180.0f) * 0.01745329238474369f) + c[i].oy) * 640.0f);
            j++;
        } while (--k);
    }

frames:
    {
        BsWobble* p;
        int*      q;

        for (j = 0, q = b->frame, p = b->wob - 3; j < 0x50; j++, q++, p++) {
            int dx;
            int dy;

            if (j < 0x4c) {
                dx = p[7].x;
                dy = p[7].y;
            } else {
                dx = b->wob[0x4f].x;
                dy = b->wob[0x4f].y;
            }
            if (j > 3) {
                dx -= p[0].x;
                dy -= p[0].y;
            } else {
                dx -= b->wob[0].x;
                dy -= b->wob[0].y;
            }
            *q = (((ArcTan256(dx, dy) >> 4) + 6) & 0xf) + (b->hull << 4);
        }
    }
}
