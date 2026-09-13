/* LEGOLAND -- the BOATING SCHOOL boat MOVER (the per-square leg step and the
 * docking sequence), the lake's route-reachability probe and its record
 * teardown, plus three small ride records that had no home file: RESTAURANT
 * 2's allocator, the PLANE RIDE's record unlink and the SPACE TOWER's car
 * draw and standing-rider animation step.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are declared LOCALLY on purpose (legoland.h is owned
 * elsewhere) and mirror bswater.c (the BsBoat / BsWater records), ridecb5.c /
 * ridecb6.c (the boating-school placement side), ridemisc.c (the JUNGLE
 * CRUISE twin of the docking sequence), ridemisc2.c (the boat walk and the
 * space tower's queue step), ridecb3.c (the RESTAURANT 2 record) and
 * mechrides.c (the six mechanical rides).
 *
 *   addr        what it is                                       insns state
 *   0x0042f920  Restaurant2_NewRecord    open a RESTAURANT 2 record  40  [OK]
 *   0x0043d8c0  PlaneRide_RemoveRecord   unlink + fade + free        41  [OK]
 *   0x0041c8c0  BoatingSchool_BuildRoute is the lake navigable?      42  [OK]
 *   0x0041c620  BsWater_RemoveOne        drop one lake square        43  [OK]
 *   0x0043aee0  SpaceTower_DrawCar       one car and its two riders  45  [OK]
 *   0x0043a820  SpaceTower_StepAnim      one frame of a queuer       56  [OK]
 *   0x00419420  BsBoat_EndLeg            the four-tick docking run   79  [OK]
 *   0x00419520  BsBoat_StepLeg           choose and take one hop    294  [OK]
 *
 * =========================================================================
 * THE BOATING SCHOOL'S BOAT MOVER, IN FULL
 * =========================================================================
 * A boating school is a building plus a LAKE: a set of 0x1c-byte BsWater
 * records (list head 0x004d823c), one per placed BOATING SCHOOL WATER square,
 * each carrying its own map square (+0x00), the school that owns it (+0x02)
 * and a four-bit ARM MASK (+0x04) saying which of its four cardinal
 * neighbours FIVE map squares away is also water: 1 = north (y-5), 2 = east
 * (x+5), 4 = south (y+5), 8 = west (x-5).  Water squares are laid on a 5x5
 * grid, so the lake is a graph whose nodes are five squares apart and the
 * mask is the adjacency row.
 *
 * A boat (0x3f4 bytes, head 0x004cc03c) hops that graph one node per 0x50
 * frames.  ridemisc2.c's BoatingSchool_AdvanceBoats commits the pending move
 * (cx,cy <- nx,ny) and then dispatches on the boat's mover state (+0x3e4):
 *
 *      1   start a leg              0x004193c0
 *      4   step, `turn` = 0         0x00419520   (this file)
 *      8   step, `turn` = 1         0x00419520   (this file)
 *      16  end the leg / dock       0x00419420   (this file)
 *
 * ONE STEP (0x00419520) is: find the water record under the boat, find the
 * school that owns it, take that record's arm mask, and then STRIKE OUT every
 * arm the boat may not take this tick --
 *
 *   - while the boat is STANDING ON the school's near jetty (st +0x02) the
 *     NORTH arm goes (bit 1) -- that is where the building itself sits, so a
 *     boat can never sail back up into the dock it launched from;
 *   - every OTHER boat on the lake strikes out the arm whose target square it
 *     is standing on (cx,cy) OR heading for (nx,ny).  Four written-out
 *     neighbour blocks, each an `||` of the two tests -- this is the whole
 *     collision avoidance, and it is a RESERVATION scheme: a boat's `next`
 *     square is as blocking as the one it is on;
 *   - with `turn` set (mover state 8, the step the boat enters once its leg
 *     counter runs out) ONE more arm goes, decided by where the route walk
 *     reached this cell FROM (BsWater +0x18, the walk's back-pointer).  With
 *     d = parent square - this square, in packed map bytes:
 *         d.y == 0  -> strike SOUTH (4)
 *         d.y != 0 and d.x >= 0 -> strike EAST (2)
 *         d.y != 0 and d.x <  0 -> strike WEST (8)
 *     i.e. the turning step biases the boat off the route's own axis, which
 *     is what makes a boat start circling instead of running the line.
 *
 * If nothing is left the boat STALLS: it replays its animation with the
 * "to" direction -1 and parks its entry side at -1, and the square is
 * retried next tick.  Otherwise, ONE TIME IN EIGHT, the surviving mask is
 * randomly PRUNED down to a single bit (excluding the side the boat came in
 * by) -- a deliberate wander, so boats do not all follow the same line.
 *
 * The direction is then chosen deterministically, in this order, and the
 * FIRST one still in the mask wins:
 *      1. straight on   (the opposite of the entry side, (entry + 2) % 4)
 *      2. one turn to a RANDOMLY chosen hand ((straight + s) & 3, s = +-1)
 *      3. the other hand ((straight - s) & 3)
 *      4. the ENTRY SIDE itself ((straight + 2) % 4, which is where the boat
 *         came in) -- the boat turns round and goes back the way it came.
 * The fallback is NOT re-tested against the mask, so a boat with every arm
 * struck out still moves, into a square another boat may have reserved: the
 * collision tests are advisory, not hard.  Note also that when the entry side
 * is -1 (the stall marker) or 0 the bit search runs off the end at index 4
 * and "straight on" comes out as SOUTH.
 *
 * The chosen direction sets the destination square (nx,ny five away), plays
 * the crossing animation, and records the OPPOSITE bit as the new entry side
 * (+0x3dc).  Finally the leg counter (+0x3e8) is decremented and, at zero,
 * the mover switches to state 8 -- the turning step.
 *
 * ARRIVING (0x00419520 again): when the water square under the boat is the
 * school's FAR jetty (st +0x04) the step is abandoned, the mover goes to
 * state 16 with leg = 3 and the boat aims one square south -- the run in to
 * the dock.
 *
 * DOCKING (0x00419420) is then four ticks, and it is the same routine as the
 * jungle cruise's JcBoat_Advance (ridemisc.c 0x004333e0) with this ride's
 * offsets:
 *      leg 3   animate, then freeze wob[64..79] on wob[64] -- the boat stops
 *              dead half way across the last square;
 *      leg 2   animate, freeze wob[0..63] on wob[64] instead (already parked
 *              when the buffer starts), aim one square further south;
 *      leg 1   animate, aim south again, then freeze the last SEVEN steps on
 *              wob[72] -- the settle as it ties up;
 *      leg 0   fade the boat's engine sample out, unlink and free the boat,
 *              and hand the walk the NEXT boat on the list.
 *
 * =========================================================================
 * BoatingSchool_BuildRoute IS A REACHABILITY PROBE, NOT A ROUTE
 * =========================================================================
 * 0x0041c8c0 clears every lake record's DFS mark (+0x0c), then runs a
 * recursive four-way flood (0x0041c940) from the near jetty, following only
 * cells that belong to the SAME school, and sets an out flag when it reaches
 * the far jetty.  It returns that flag.  ridecb5.c / ridecb6.c store the
 * result in BsStation +0x08 and only ever test it for truth, so the field is
 * a boolean "this school's two docks are joined by water", not a route
 * object; both files declare the function `void*` and are left alone (extern
 * prototype TYPES are caller-side levers).
 *
 * =========================================================================
 * EXTERN TYPE NOTES (caller-side levers -- nothing else is aligned to them)
 * =========================================================================
 *  - BoatingSchool_BuildRoute is DEFINED here returning `int`, because it
 *    returns a flag; ridecb5.c and ridecb6.c declare 0x0041c8c0 `void*` and
 *    stay as they are.
 *  - StandardRemoveObject takes the map square as a `BPosW` BY VALUE here
 *    (one pushed dword, no widening).  joust.c declares 0x0045f220 with an
 *    `unsigned int`, screencb.c with three different second parameters; each
 *    is its own call site's lever.
 *  - SpaceTower_StepAnim's first parameter is typed `AnimPart*` to match
 *    ridemisc2.c's declaration even though the body never reads it.
 *  - 0x0043a7a0 is NEW: `Pos Anim3D_OffsetAt(void* anim, int part, int
 *    frame)`, an 8-byte struct returned in eax:edx.  It sums an Anim3D's
 *    per-frame translation up to (part, frame); mechrides.c had no name for
 *    it.  0x0043ae20 / 0x0043ad00 / 0x0043ad90 are likewise newly named
 *    (SpaceTower_DrawCarUnder / _PlaceCar / _DrawCarOver) and 0x0041c940 is
 *    BsRoute_Trace, the route probe's recursive flood.
 *
 * =========================================================================
 * ORIGINAL BUGS REPRODUCED
 * =========================================================================
 *  - BsWater_RemoveOne dereferences the lake list HEAD without a NULL test
 *    (`cmp word ptr [esi],bx` on an empty list), so removing the last water
 *    square of the last school faults.  Only the peeled first compare is
 *    unguarded -- the loop body does test -- which is why VC6 still emits the
 *    otherwise-dead `if (w)` after the walk.
 *  - PlaneRide_RemoveRecord has the same shape and the same hole: the walk
 *    starts at the head without testing it, and its trailing `if (p)` is dead
 *    on every path that reaches it.  So do the other five mechanical rides'
 *    unlinks (mechrides.c), which are the same source compiled six times.
 *  - BsBoat_StepLeg uses BOTH search results unguarded: `mov cx,[eax+2]` on
 *    BsWater_FindAt's answer (a boat on a square with no water record) and
 *    `cmp cx, word ptr [ebx+2]` on the station walk's (a lake whose owning
 *    school has been removed).
 *  - The direction fallback (step 4 above) is taken whether or not the mask
 *    allows it.
 *  - BsBoat_StepLeg's `turn` block spills its y delta to the frame and never
 *    reads it back -- a dead store the aggregate makes visible.
 * ========================================================================= */

void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)

/* ---- shared map types (same offsets as bswater.c / ridecb5.c) ----------- */
typedef struct Pos { int x; int y; } Pos;

/* A packed 2-byte map square passed BY VALUE, and its 16-bit view. */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union BPosW { unsigned short w; BPos b; } BPosW;

/* money.c's SoundSource.  kind 1 = "sourced at a person" (the person at +4),
 * kind 2 = "sourced at a map square" (the two coordinates at +8/+0xc). */
typedef struct SoundSource {
    int   kind;                     /* +0x00 */
    void* bloke;                    /* +0x04 */
    int   x;                        /* +0x08 */
    int   y;                        /* +0x0c */
} SoundSource;

extern void* HeapAlloc_w(unsigned int size);                    /* 0x0049e4ff */
#ifndef LEGOLAND_PORTABLE
extern int   HeapFree_w(void* p);                               /* 0x0049e4d0 */
#else
extern void HeapFree_w(void* p);                               /* 0x0049e4d0 */
#endif
extern int   rand(void);                                        /* 0x0049e4b2 (CRT) */
extern void  UnSourceAndFadeAllSamplesFromSource(SoundSource* s,
                                                 int fade);     /* 0x00496c80 */

/* =========================================================================
 * RESTAURANT 2 -- 0x0042f920  Restaurant2_NewRecord (the +0x98 place path).
 *
 * ridecb8.c's Restaurant2_Add hands this the packed square of a freshly
 * placed RESTAURANT 2.  It is the same "allocate, memset, fill, push" shape
 * as joust.c's Joust_AddRecord: the whole 0x40-byte record is zeroed by the
 * intrinsic `rep stosd` AND then every field is written again, because VC6's
 * rep stosd is not a kill and the redundant stores survive.  Only the packed
 * square and the waiter's starting x (0x143, the first entry of the waiter's
 * path table, ridecb3.c) are non-zero.
 *
 * With eighteen literal zeros the zero web is far past the three-to-five
 * threshold, so VC6 hoists `xor ebx,ebx` into the register it pushes anyway
 * and the null test on the allocation comes out as `cmp edx,ebx`.
 * ========================================================================= */

/* ridecb3.c's RestRec2, in full (list head 0x00616148). */
typedef struct RestRec2 {
    struct RestRec2* next;          /* +0x00 */
    unsigned short   square;        /* +0x04 */
    char             swing;         /* +0x06 door/serve dwell counter */
    char             step;          /* +0x07 the waiter's step, 0..0x20 */
    char             serve;         /* +0x08 serving dwell counter */
    char             frame;         /* +0x09 building animation frame */
    unsigned char    pad0a[2];
    int              walking;       /* +0x0c customers walking in */
    unsigned char    queued;        /* +0x10 customers queueing, max 3 */
    unsigned char    seated;        /* +0x11 customers at the table, max 3 */
    unsigned char    pad12[2];
    int              ready;         /* +0x14 */
    int              phase;         /* +0x18 the restaurant's machine, 0..5 */
    int              idle;          /* +0x1c idle frames before the waiter */
    int              seating;       /* +0x20 */
    int              called;        /* +0x24 */
    int              serving;       /* +0x28 */
    int              leaving;       /* +0x2c */
    int              clearing;      /* +0x30 */
    int              returning;     /* +0x34 */
    int              waiter_x;      /* +0x38 the waiter's world position */
    int              waiter_y;      /* +0x3c */
} RestRec2;                         /* 0x40 */

extern RestRec2* g_r2_recs;                                     /* 0x00616148 */

// FUNCTION: LEGOLAND 0x0042f920
void Restaurant2_NewRecord(BPosW* sq)
{
    RestRec2* rec = (RestRec2*)HeapAlloc_w(sizeof(RestRec2));

    if (rec != 0) {
        memset(rec, 0, sizeof(RestRec2));
        rec->square = sq->w;
        rec->next = g_r2_recs;
        rec->swing = 0;
        rec->step = 0;
        rec->serve = 0;
        rec->frame = 0;
        rec->walking = 0;
        rec->queued = 0;
        rec->seated = 0;
        rec->ready = 0;
        rec->phase = 0;
        rec->idle = 0;
        rec->seating = 0;
        rec->called = 0;
        rec->serving = 0;
        rec->leaving = 0;
        rec->clearing = 0;
        rec->returning = 0;
        rec->waiter_x = 0x143;
        rec->waiter_y = 0;
        g_r2_recs = rec;
    }
}

/* =========================================================================
 * PLANE RIDE -- 0x0043d8c0  PlaneRide_RemoveRecord.
 *
 * The sixth copy of the mechanical rides' record unlink (mechrides.c names
 * the other five), and the only one that fades a sample: the other five are
 * `unlink; free`, this one is `unlink; fade the samples sourced at the map
 * square; free`.  That is why the tail is NOT tail-duplicated into the
 * head arm the way the Safari's and the Copters' are -- the shared block is
 * two calls and an argument frame, so VC6 jumps to it instead of copying it.
 * ========================================================================= */

/* mechrides.c's RideTile: the packed map square every record is keyed by. */
typedef union RideTile {
    unsigned short key;                         /* +0x00 */
    struct { unsigned char x, y; } b;
} RideTile;

/* mechrides.c's PlaneRec (0x24 bytes, list head 0x0062fe9c, next @ +0x20). */
typedef struct PlaneRec {
    RideTile          tile;         /* +0x00 */
    unsigned char     pad02[0x20 - 2];
    struct PlaneRec*  next;         /* +0x20 */
} PlaneRec;                         /* 0x24 */

extern PlaneRec* g_plane_recs;                                  /* 0x0062fe9c */

/* Fade out whatever this ride was playing at one map square.  The inlined
 * helper taking the two coordinates as scalars is joust.c's and mechrides.c's
 * shape: it is what makes VC6 read both bytes before touching the source
 * struct and keeps the struct in the inline-expansion temporary pool. */
static __inline void FadeSamplesAtMapSquare(int x, int y)
{
    SoundSource src;

    src.kind = 2;
    src.x = x;
    src.y = y;
    UnSourceAndFadeAllSamplesFromSource(&src, -200);
}

// FUNCTION: LEGOLAND 0x0043d8c0
void PlaneRide_RemoveRecord(PlaneRec* rec)
{
    if (g_plane_recs == rec) {
        g_plane_recs = rec->next;
    } else {
        /* joust.c's solved link-pointer walk: the ONE volatile read on the
         * link deref is what stops VC6 forwarding the condition's load into
         * the body's, and `link` must be declared BEFORE `node` and seeded
         * from the GLOBAL so it takes the first register. */
        PlaneRec** link = &g_plane_recs->next;
        PlaneRec*  node = g_plane_recs;
#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
        if (!node)                /* QUIRKS.md B: the record list is empty (the shipped game reads address 4) */
            return;
#endif

        while (*link != rec) {
            node = *(PlaneRec* volatile*)link;
            if (node == 0)
                break;
            link = &node->next;
        }
        /* Dead on every path that reaches it -- the original emits it. */
        if (node != 0)
            node->next = rec->next;
    }
    FadeSamplesAtMapSquare(rec->tile.b.x, rec->tile.b.y);
    HeapFree_w(rec);
}

/* =========================================================================
 * THE LAKE -- BsWater records, the route probe and the record teardown.
 * ========================================================================= */

/* bswater.c's BsWater, plus the DFS mark this file names. */
typedef struct BsWater {
    BPosW           pos;            /* +0x00 this cell's own map square */
    BPosW           owner;          /* +0x02 the school that owns it */
    int             mask;           /* +0x04 arm mask: 1 N, 2 E, 4 S, 8 W */
    int             dist;           /* +0x08 steps from the seed dock */
    int             seen;           /* +0x0c the reachability probe's mark */
    struct BsWater* next;           /* +0x10 the lake-wide list link */
    struct BsWater* queue;          /* +0x14 the walk frontier link */
    struct BsWater* from;           /* +0x18 reached from / visited mark */
} BsWater;                          /* 0x1c */

/* ridecb5.c's placement types; only the class pointer and the by-value
 * square are read through them here. */
typedef struct ObjDef  ObjDef;
typedef struct Cursor  Cursor;

typedef struct MapObj {
    unsigned char pad00[0x0c];
    ObjDef*       cls;              /* +0x0c */
} MapObj;

extern BsWater* g_bs_water;                                     /* 0x004d823c */

extern BsWater* BsWater_FindAt(int x, int y);                   /* 0x0041c890 */
extern void     StandardRemoveObject(MapObj* o, BPosW bp,
                                     Cursor* ctx);              /* 0x0045f220 */
/* The recursive four-way flood the route probe runs; sets *ok when it
 * reaches (x1, y1) without leaving the school named by *owner. */
extern void     BsRoute_Trace(int x, int y, int x1, int y1,
                              BPosW* owner, int* ok);           /* 0x0041c940 */

/* =========================================================================
 * 0x0041c620 -- BsWater_RemoveOne (called from BoatingSchoolWater_Remove).
 *
 * Unbuild the map object first, then drop this square's lake record.  The
 * search is the recorded rotated form: the `while`'s condition is the KEY
 * compare (that is the test VC6 leaves in the latch) and the NULL test is a
 * `break` inside the body, so the peeled copy doubles as the enclosing `if`
 * and the trailing `if (w)` survives -- VC6 cannot prove the head non-null
 * on the peel edge, which is also the ORIGINAL BUG: an empty lake list is
 * dereferenced by the peel.
 *
 * The two unlink arms are written out IN FULL, each with its own `free`,
 * because they are not instruction-identical (one stores through `prev`, the
 * other through the head global) and therefore cannot cross-jump.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0041c620
void BsWater_RemoveOne(MapObj* o, BPosW bp, Cursor* ctx)
{
    BsWater* w = g_bs_water;
    BsWater* prev = 0;

    StandardRemoveObject(o, bp, ctx);
#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
    while (w && w->pos.w != bp.w) {   /* QUIRKS.md B: an empty lake list */
#else
    while (w->pos.w != bp.w) {
#endif
        prev = w;
        w = w->next;
        if (w == 0)
            break;
    }
    if (w != 0) {
        if (prev != 0) {
            prev->next = w->next;
            HeapFree_w(w);
        } else {
            g_bs_water = w->next;
            HeapFree_w(w);
        }
    }
}

/* =========================================================================
 * 0x0041c8c0 -- BoatingSchool_BuildRoute: are the school's two docks joined?
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0041c8c0
int BoatingSchool_BuildRoute(int x0, int y0, int x1, int y1)
{
    BsWater* w = g_bs_water;
    int      ok = 0;
    BsWater* start;
    BPosW    owner;

    while (w != 0) {
        w->seen = 0;
        w = w->next;
    }
    start = BsWater_FindAt(x0, y0);
    if (start == 0)
        return 0;
    owner.w = start->owner.w;
    BsRoute_Trace(x0, y0, x1, y1, &owner, &ok);
    return ok;
}

/* =========================================================================
 * SPACE TOWER -- 0x0043aee0  SpaceTower_DrawCar, one car of the tower.
 *
 * mechrides.c's SpaceTower_Interact calls this four times per copy, in the
 * order 1, 2, (tower body), 0, 3, so the four cars are drawn in two pairs
 * either side of the body sprite.  The record's car array is four 36-byte
 * slots at TowerRec +0x14 (it runs up to the seat bytes at +0xa4): +0x00 is
 * a flag word whose bit 0 says the car is in service, and +0x18 / +0x1c are
 * the two rider nodes it is carrying.
 *
 * The lower half of the car is drawn unconditionally; only a car in service
 * gets its depth set, its two riders rendered in 3D between the halves (so
 * the upper half occludes them) and its upper half drawn.
 * ========================================================================= */

typedef struct RiderNode RiderNode;

/* mechrides.c's Bloke, with the fields the animation step writes. */
typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;           /* +0x0e  low-level AI state (7 = walk) */
    unsigned char  pad10[0x24 - 0x10];
    Pos            target;          /* +0x24  walk target, 24.8 */
    unsigned char  pad2c[0x38 - 0x2c];
    short          frame;           /* +0x38  frame within the current part */
    unsigned char  pad3a[0x4a - 0x3a];
    short          part;            /* +0x4a  which part of the animation */
    short          stop;            /* +0x4c  the part to finish at */
    unsigned char  pad4e[2];
    int            anim;            /* +0x50  animation id */
    unsigned char  pad54[0x68 - 0x54];
    Pos            world;           /* +0x68  world position, 24.8 */
    unsigned char  pad70[3];
    unsigned char  new_dir;         /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14];      /* +0x98  CalcMoveLine scratch */
} Bloke;

struct RiderNode {
    struct RiderNode* next;         /* +0x00 */
    struct RiderNode* prev;         /* +0x04 */
    Bloke*            bloke;        /* +0x08 */
    unsigned short    ride_id;      /* +0x0c the packed map square in use */
    unsigned short    pad0e;
    void*             owner;        /* +0x10 */
};

/* mechrides.c's RideDef; only the class base square is read here. */
typedef struct RideDef {
    unsigned char pad00[0x0c];
    int           base_x;           /* +0x0c */
    int           base_y;           /* +0x10 */
} RideDef;

typedef struct TowerCar {
    int               flags;        /* +0x00 bit 0 = this car is in service */
    unsigned char     pad04[0x18 - 4];
    RiderNode*        rider_a;      /* +0x18 */
    RiderNode*        rider_b;      /* +0x1c */
    unsigned char     pad20[4];
} TowerCar;                         /* 0x24 */

typedef struct TowerRec {
    RideTile          tile;         /* +0x00 */
    unsigned char     pad02[0x14 - 2];
    TowerCar          car[4];       /* +0x14 .. +0xa4 */
    unsigned char     seat[8];      /* +0xa4 */
    signed char       frame3;       /* +0xac */
    signed char       frame5;       /* +0xad */
    unsigned char     padae[2];
    int               timer;        /* +0xb0 */
} TowerRec;                         /* 0xb4 */

extern void SpaceTower_DrawCarUnder(TowerRec* rec, int car, int mode); /* 0x0043ae20 */
#ifndef LEGOLAND_PORTABLE
extern void SpaceTower_PlaceCar(TowerRec* rec, int car);              /* 0x0043ad00 */
#else
extern int SpaceTower_PlaceCar(TowerRec* rec, int car);              /* 0x0043ad00 */
#endif
extern void SpaceTower_DrawCarOver(TowerRec* rec, int car, int mode);  /* 0x0043ad90 */
extern void IP_RenderBlokeIn3DNow(Bloke* b);                          /* 0x00440010 */

// FUNCTION: LEGOLAND 0x0043aee0
void SpaceTower_DrawCar(TowerRec* rec, int car, int mode)
{
    SpaceTower_DrawCarUnder(rec, car, mode);
    if (rec->car[car].flags & 1) {
        SpaceTower_PlaceCar(rec, car);
        if (rec->car[car].rider_a)
            IP_RenderBlokeIn3DNow(rec->car[car].rider_a->bloke);
        if (rec->car[car].rider_b)
            IP_RenderBlokeIn3DNow(rec->car[car].rider_b->bloke);
        SpaceTower_DrawCarOver(rec, car, mode);
    }
}

/* =========================================================================
 * SPACE TOWER -- 0x0043a820  SpaceTower_StepAnim: play one animation frame.
 *
 * ridemisc2.c's SpaceTower_Queue calls this once the part / frame cursors
 * have been checked.  It is what actually MOVES a queueing rider: the
 * animation's accumulated offset at (part, frame) (0x0043a7a0, an 8-byte
 * {x, y} returned in eax:edx) is added to the ride's own base square, the
 * result becomes the bloke's walk target, and CalcMoveLine turns that into a
 * heading.  Then the frame cursor is bumped -- through `r->bloke` again,
 * not through the cached pointer, exactly as SpaceTower_Queue does.
 *
 * `part`, the first parameter, is DEAD: the caller works the part out and
 * passes it, and this body re-derives it from the bloke.  Original.
 * ========================================================================= */

/* mechrides.c's AnimRef table: animation id -> the Anim3D it plays. */
typedef struct AnimRef {
    void* anim;                     /* +0x00 */
    int   n;                        /* +0x04 */
} AnimRef;

typedef struct AnimPart AnimPart;

extern AnimRef  g_bloke_anim_ref[];                             /* 0x004b775c */
extern RideDef* g_spacetower_def;                               /* 0x0062fd74 */

/* The animation's accumulated {x, y} offset at (part, frame). */
extern Pos  Anim3D_OffsetAt(void* anim, int part, int frame);   /* 0x0043a7a0 */
extern int  CalcMoveLine(Pos from, Pos to, void* path);         /* 0x00480740 */
extern int  NewDirForAction(Bloke* b, unsigned char dir);       /* 0x004833d0 */

/* TWO LEVERS, both new (measured over 19 spellings):
 *  1. THE RETURNED `Pos` MUST BE THE ACCUMULATOR.  `p.x += (...) << 8;` emits
 *     `add eax, ecx` -- the struct return's own eax/edx as the DESTINATION of
 *     each sum, which is what leaves the y half untouched in edx across the
 *     whole x block.  Written `b->target.x = p.x + (...)` (or with the
 *     operands the other way round -- the two are byte-identical) the shifted
 *     sum is the destination instead, p.y has to be saved into ebp, and the
 *     body is 57 instructions for the original's 56.
 *  2. THE CLASS GLOBAL IS READ DIRECTLY AT BOTH USES, and the two sums are
 *     computed BEFORE either store so the two reads still CSE.  As a named
 *     `RideDef* def` local it takes a SCRATCH register (ecx) and pushes the
 *     tile byte on to edx; as VC6's own CSE temporary it takes EBP, the
 *     original's choice -- the recorded "read a class/def global directly at
 *     every use" rule, with the store-ordering caveat that an intervening
 *     store to the object being written forces a reload (16 mismatches at 52
 *     instructions).
 * Also ruled out at 13-23 mismatches: `int bx/by` locals for the two base
 * fields (folds the load into the add), one reused `base` local, `int tx/ty`
 * locals for the tile bytes, a `BPosW sq` copy of the rider's square, a
 * volatile read of base_x, `b->target = p;` followed by two `+=`, and a `Pos
 * t` temporary stored afterwards. */
// FUNCTION: LEGOLAND 0x0043a820
void SpaceTower_StepAnim(AnimPart* part, RiderNode* r)
{
    Bloke*        b = r->bloke;
    Pos           p = Anim3D_OffsetAt(g_bloke_anim_ref[b->anim].anim,
                                      b->part, b->frame);
    unsigned char a;

    p.x += (((BPosW*)&r->ride_id)->b.x + g_spacetower_def->base_x) << 8;
    p.y += (((BPosW*)&r->ride_id)->b.y + g_spacetower_def->base_y) << 8;
    b->target.x = p.x;
    b->target.y = p.y;
    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
    b->state = 7;
    b->new_dir = a;
    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
    r->bloke->frame++;
}

/* =========================================================================
 * THE BOAT RECORD, and the two mover entry points.
 * ========================================================================= */

typedef struct BsWobble { int x; int y; } BsWobble;

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
    int            f3e0;            /* +0x3e0 */
    int            state;           /* +0x3e4  the mover's state */
    int            leg;             /* +0x3e8  squares left in this leg */
    Bloke*         rider;           /* +0x3ec  the single passenger */
    struct BsBoat* next;            /* +0x3f0 */
} BsBoat;                           /* 0x3f4 */

/* One placed boating school (ridecb5.c owns the full field list). */
typedef struct BsStation {
    BPosW              key;         /* +0x00 the building's map square */
    BPosW              dock_a;      /* +0x02 near jetty (the route start) */
    BPosW              dock_b;      /* +0x04 far jetty  (the route end) */
    unsigned char      pad06[0x2c - 6];
    struct BsStation*  next;        /* +0x2c */
    int                take;        /* +0x30 */
} BsStation;                        /* 0x34 */

extern BsStation* g_bs_stations;                                /* 0x004cc074 */
extern BsBoat*    g_bs_boats;                                   /* 0x004cc03c */

/* Rebuild the boat's 80 sub-steps and sprite codes for a crossing that
 * enters by `from` and leaves by `to` (-1 = stall in place). */
extern void BsBoat_Animate(BsBoat* b, int from, int to);        /* 0x004198a0 */
/* Unlink the boat from 0x004cc03c and free it. */
extern void BsBoat_Unlink(BsBoat* b);                           /* 0x00418f90 */

/* Fade out the engine sample sourced at this boat's passenger. */
static __inline void FadeSamplesFromBloke(void* bloke)
{
    SoundSource src;

    src.kind = 1;
    src.bloke = bloke;
    UnSourceAndFadeAllSamplesFromSource(&src, -90);
}

/* =========================================================================
 * 0x00419420 -- BsBoat_EndLeg, the four-tick docking sequence.
 *
 * The JUNGLE CRUISE twin is ridemisc.c's JcBoat_Advance (0x004333e0) and the
 * three buffer fills follow its recorded lever exactly: the two ASCENDING
 * fills are per-FIELD assignments (the induction variable is anchored on
 * `.y`, addressing `[eax-4]` / `[eax]`) and the DESCENDING one is a
 * whole-struct assignment (anchored on `.x`, addressing `[eax]` / `[eax+4]`).
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00419420
BsBoat* BsBoat_EndLeg(BsBoat* b)
{
    BsBoat* next;
    int     i;

    if (b->leg == 0) {
        next = b->next;
        FadeSamplesFromBloke(b->rider);
        BsBoat_Unlink(b);
        return next;
    }
    BsBoat_Animate(b, 1, 4);
    b->entry = 1;
    if (b->leg == 3) {
        for (i = 64; i < 80; i++) {
            b->wob[i].x = b->wob[64].x;
            b->wob[i].y = b->wob[64].y;
        }
    } else if (b->leg == 2) {
        for (i = 0; i < 64; i++) {
            b->wob[i].x = b->wob[64].x;
            b->wob[i].y = b->wob[64].y;
        }
    }
    if (b->leg != 3)
        b->ny = b->cy + 5;
    if (b->leg == 1) {
        for (i = 79; i > 72; i--)
            b->wob[i] = b->wob[72];
    }
    b->leg--;
    return b;
}

/* =========================================================================
 * 0x00419520 -- BsBoat_StepLeg: choose the next lake square and move to it.
 * ========================================================================= */

/* FOUR LEVERS, all recorded ones applied to a new body:
 *  - THE `turn` BLOCK'S TWO DELTAS ARE ONE `Pos` AGGREGATE.  As two plain
 *    `int`s VC6 homes the spilled one in the freed `b` argument slot and the
 *    frame collapses to nothing; as one aggregate it takes a fresh slot and
 *    the frame is the original's `sub esp, 8` -- the whole body shifted by
 *    one instruction until this was changed.  (`d.y`'s home is then a DEAD
 *    store: nothing reads it back.  Original.)
 *  - THE STATION COMPARE IS `cell->owner.w != st->key.w`, in that operand
 *    order: `cmp cx, word ptr [ebx]`.  Written the other way round VC6 emits
 *    `cmp word ptr [ebx], cx`.
 *  - THE THREE-WAY `turn` CHOICE IS A NESTED `if`, NOT AN `else if` CHAIN.
 *    `if (dy == 0) A; else if (dx >= 0) B; else C;` puts A INLINE and exiles
 *    B and C; the original's inline arm is C, which only
 *    `if (dy != 0) { if (dx < 0) C; else B; } else { A; }` produces -- and it
 *    also puts the two cold blocks out in the original's order (B then A).
 *  - `for (i = 0, n = 0; ...)` -- the comma operator is what emits
 *    `xor ecx,ecx` (the counter) BEFORE `xor eax,eax` (the accumulator);
 *    `n = 0;` as its own preceding statement reverses the pair.
 * Two more shapes worth reading off the listing rather than guessing:
 *  - `char s0 = (char)rand(); s = (s0 & 1) ? 1 : -1;` is the recorded
 *    "a char `x ? 1 : -1` narrows only via a char LOCAL" idiom -- it is what
 *    emits `and al,1 / neg al / sbb eax,eax / and eax,2 / dec eax`.
 *  - `(i + 2) % 4` on a SIGNED int gives the four-instruction
 *    `and 0x80000003 / jns / dec / or 0xfffffffc / inc` correction, while the
 *    two hand turns are written `& 3` -- the original genuinely mixes the two
 *    spellings in one expression tree, so do not "tidy" either of them.
 *  - The literal 8 appears twice (`b->entry = 8` in the east arm and
 *    `b->state = 8` at the tail) and VC6 hoists it into EDI ahead of the
 *    switch; nothing in the source asks for that. */
// FUNCTION: LEGOLAND 0x00419520
void BsBoat_StepLeg(BsBoat* b, int turn)
{
    BsStation* st = g_bs_stations;
    BsBoat*    ot = g_bs_boats;
    BsWater*   cell = BsWater_FindAt(b->cx, b->cy);
    int        mask;
    int        entry;
    int        dir;
    int        i;
    int        n;

#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
    if (!cell)                /* QUIRKS.md B: BsBoat_StepLeg -- a boat on a square with no water record */
        return;
#endif
    while (st != 0 && cell->owner.w != st->key.w)
        st = st->next;
#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
    if (!st)                /* QUIRKS.md B: BsBoat_StepLeg -- the owning school has been removed */
        return;
#endif

    mask = cell->mask;
    if (cell->pos.w == st->dock_a.w) {
        mask &= ~1;
    } else if (cell->pos.w == st->dock_b.w) {
        b->state = 0x10;
        b->leg = 3;
        BsBoat_Animate(b, b->entry, 4);
        b->entry = 1;
        b->ny = b->cy + 5;
        return;
    }

    while (ot != 0) {
        if (ot != b) {
            if ((b->cx == ot->cx && b->cy - 5 == ot->cy)
                || (b->cx == ot->nx && b->cy - 5 == ot->ny))
                mask &= ~1;
            if ((b->cx + 5 == ot->cx && b->cy == ot->cy)
                || (b->cx + 5 == ot->nx && b->cy == ot->ny))
                mask &= ~2;
            if ((b->cx == ot->cx && b->cy + 5 == ot->cy)
                || (b->cx == ot->nx && b->cy + 5 == ot->ny))
                mask &= ~4;
            if ((b->cx - 5 == ot->cx && b->cy == ot->cy)
                || (b->cx - 5 == ot->nx && b->cy == ot->ny))
                mask &= ~8;
        }
        ot = ot->next;
    }

    if (turn != 0 && cell->from != 0) {
        Pos d;

        d.x = cell->from->pos.b.x - cell->pos.b.x;
        d.y = cell->from->pos.b.y - cell->pos.b.y;
        if (d.y != 0) {
            if (d.x < 0)
                mask &= ~8;
            else
                mask &= ~2;
        } else {
            mask &= ~4;
        }
    }

    if (mask == 0) {
        BsBoat_Animate(b, b->entry, -1);
        b->entry = -1;
        return;
    }

    if ((rand() & 7) == 0) {
        int avail = mask & ~b->entry;

        if (avail != 0) {
            for (;;) {
                for (i = 0, n = 0; i < 4; i++)
                    if (avail & (1 << i))
                        n++;
                if (n <= 1)
                    break;
                avail &= ~(1 << (rand() & 3));
            }
            mask = avail;
        }
    }

    entry = b->entry;
    for (i = 0; i < 4; i++)
        if (entry & (1 << i))
            break;
    i = (i + 2) % 4;
    dir = 1 << i;
    if ((mask & dir) == 0) {
        char s0 = (char)rand();
        int  s = (s0 & 1) ? 1 : -1;

        dir = 1 << ((i + s) & 3);
        if ((mask & dir) == 0) {
            dir = 1 << ((i - s) & 3);
            if ((mask & dir) == 0)
                dir = 1 << ((i + 2) % 4);
        }
    }

    switch (dir) {
    case 1:
        b->nx = b->cx;
        b->ny = b->cy - 5;
        BsBoat_Animate(b, b->entry, dir);
        b->entry = 4;
        break;
    case 2:
        b->nx = b->cx + 5;
        b->ny = b->cy;
        BsBoat_Animate(b, b->entry, dir);
        b->entry = 8;
        break;
    case 4:
        b->nx = b->cx;
        b->ny = b->cy + 5;
        BsBoat_Animate(b, b->entry, dir);
        b->entry = 1;
        break;
    case 8:
        b->nx = b->cx - 5;
        b->ny = b->cy;
        BsBoat_Animate(b, b->entry, dir);
        b->entry = 2;
        break;
    }
    if (--b->leg == 0)
        b->state = 8;
}
