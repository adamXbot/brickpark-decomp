/* ---------------------------------------------------------------------------
 * goldrush3.c -- the GOLD RUSH pan-animation family, the walk-path walker and
 * the driving-school "may I move off?" test.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are declared LOCALLY on purpose (legoland.h is owned
 * elsewhere) and follow goldrush.c / goldrush2.c / mappath.c / schoolcar2.c.
 *
 * ==========================================================================
 * THE GOLD-RUSH PANNING POSE FAMILY  (0x00406f60, 0x00407000, 0x004070b0,
 * 0x00407170) -- four functions, ONE template
 * ==========================================================================
 * GOLD RUSH's rider state machine (GoldRush_TickRiders, 0x004072b0) walks a
 * visitor down to the creek, parks it at one of SIX pan slots, and plays a
 * kneel / pan / stand cycle.  Steps 3, 4, 5, 7 and 8 of that machine are
 * these four helpers; every one of them just re-aims the bloke's walk target
 * (Bloke +0x24) at a point derived from its pan slot and then re-issues the
 * standard CalcMoveLine / NewDirForAction walk order, so the bloke SHUFFLES
 * into the pose rather than teleporting.
 *
 * TWO .rdata TABLES DRIVE ALL FOUR, and both are new here.
 *
 *   g_pan_slots[6] at 0x004b45b0, {int pan; float frac;} -- the bloke's slot
 *   byte (Bloke +0x36, handed out by GoldRush_ClaimPan 0x00406ec0) indexes
 *   this:
 *          slot 0 { 0, 0.8f }   slot 1 { 0, 0.2f }
 *          slot 2 { 1, 0.8f }   slot 3 { 1, 0.2f }
 *          slot 4 { 2, 0.8f }   slot 5 { 2, 0.2f }
 *      so there are THREE physical pans and TWO people per pan, one at 0.8
 *      and one at 0.2 of the way across it.
 *
 *   g_pan_pos[3] at 0x004b4610, {int x; int y;} in 24.8 units relative to the
 *   placement's own map square:
 *          pan 0 { 0x400, 0x060 }   =  (4.0,  0.375) tiles
 *          pan 1 { 0x400, 0x3d0 }   =  (4.0,  3.8125)
 *          pan 2 { 0x400, 0x700 }   =  (4.0,  7.0)
 *      i.e. the three pans are in a north-south line four tiles east of the
 *      wash's square.
 *
 * The fractional slot is turned into a POSITION with the single-precision
 * 512.0f at 0x004ab384: `(int)(frac * 512.0f)` is 409 for 0.8 and 102 for
 * 0.2, i.e. 1.6 and 0.4 tiles, and it is always applied as `0x80 - that`,
 * WESTWARD from the pan's own centre-of-square.  So the two people at one pan
 * stand 1.6 and 0.4 tiles west of it -- on opposite sides.
 *
 * What the four differ in, and nothing else:
 *
 *   GoldRush_PoseAtPan     0x00406f60  target = pan + square + (0.5, 0.5)
 *                                      -- stand ON the pan's square, no
 *                                      fraction, no float at all.
 *   GoldRush_MoveToPanEdge 0x00407000  target = pan + square
 *                                             + (-2.5 + 0.5 - frac*2, +0.5)
 *                                      -- shuffle 2.5 tiles west, to the
 *                                      creek-side edge, at the slot's offset.
 *   GoldRush_KneelAtPan    0x004070b0  the same x, but y is (-0x50) instead
 *                                      of (+0x80): the bloke drops 0.3125 of
 *                                      a tile NORTH -- the kneel.
 *   GoldRush_StandUpFromPan 0x00407170 the MoveToPanEdge target exactly, plus
 *                                      one extra statement FIRST:
 *                                          b->world.y += 0x80;
 *                                      which teleports the bloke half a tile
 *                                      south before it starts walking.  That
 *                                      is the kneel being undone: the kneel
 *                                      step moved the TARGET north by 0x50
 *                                      and the walk carried the bloke there,
 *                                      and standing up puts 0x80 back into
 *                                      the WORLD position directly.
 *
 * NOTE THE ASYMMETRY, which is the original's and is reproduced: kneeling
 * takes the bloke 0x50 (0.3125 tile) north but standing up gives 0x80 (0.5
 * tile) south, so every full pan cycle leaves the bloke 0x30 (0.1875 of a
 * tile) further south than it started.  The bloke then walks to a fixed
 * waypoint (state 9) so the drift never accumulates across visits.
 *
 * ==========================================================================
 * THE WALK-PATH WALKER  (0x00412300)
 * ==========================================================================
 * mappath.c's BuildWalkPath (0x00412100) expands a polyline of tile deltas
 * into a {int count; WalkNode* nodes;} header followed by `count` 12-byte
 * {int x; int y; int state;} nodes.  This is its consumer: one call per tick
 * aims the bloke at ONE node and steps the cursor.
 *
 *   Bloke +0x38 (short)       the current node index
 *   Bloke +0x34 (signed char) the step, +1 out and -1 home
 *
 * The (x,y) parameters are the placement's anchor tile; the node holds a
 * RELATIVE tile delta, so the target is `(anchor + node) << 8` -- note that
 * this lands on the tile's CORNER, not its centre, unlike every other target
 * in the ride.  When the cursor walks off either end of the array the
 * rider's action byte advances, which is how the state machine learns that
 * the walk is over; the two ends are tested with different code because the
 * end test needs the node count and the start test does not.
 *
 * ==========================================================================
 * THE DRIVING-SCHOOL PROCEED TEST  (0x00402390)
 * ==========================================================================
 * StepSchoolCar (goldrush.c) calls this before executing manoeuvre 1, 2 or 3
 * -- the three that move the car onto a new road block.  It answers "is the
 * car allowed to move off its current square?":
 *
 *   1. find the 4x4 road block COVERING the square the manoeuvre starts from
 *      (GetRoadRecord).  No block -> 0, the car stays put.  This is what
 *      strands a car whose road was bulldozed under it.
 *   2. step that block's ORIGIN four squares along the car's heading
 *      (Pos_Step4) and look for a block whose origin is EXACTLY there
 *      (Road_FindAt).  If there is one and its kind nibble is 5 -- a
 *      TRAFFIC-LIGHT block -- return whether the lights let this heading
 *      through (0x00402340, reading coaster.c's g_light_ns / g_light_ew).
 *   3. otherwise 1.
 *
 * The heading is the car's SIXTEEN-point body frame (+0xb8) mapped down to
 * the eight-point road ring through the 16-entry byte... int table at
 * 0x004b4034: {3,3,4,5,5,5,6,7,7,7,0,1,1,1,2,3}.  Frame 0 maps to 3 (east)
 * and the map is rotated so that each of the four road headings 1/3/5/7 owns
 * three frames and each diagonal owns one -- the same "a car's heading is
 * always odd" fact schoolcar2.c records, seen from the animation side.
 *
 * The `Pos_Step4(&sq, &sq, dir)` call passes the SAME address as both the
 * source and the destination: the step is done in place.  That is visible in
 * the original as two `lea`s of the same effective address into different
 * registers, and it is what a single local passed twice produces.
 * --------------------------------------------------------------------------- */

#include "legoland.h"

/* ---------------------------------------------------------------- shapes -- */

/* An {x,y} pair in 24.8 fixed point (goldrush.c's Pos8). */
typedef struct Pos8 { int x, y; } Pos8;

/* The packed {x,y} map square a rider node carries at +0x0c. */
typedef struct MapSquare {
    unsigned char bx;            /* +0x00 */
    unsigned char by;            /* +0x01 */
} MapSquare;

typedef struct Bloke Bloke;

typedef struct RiderNode {
    struct RiderNode* next;      /* +0x00 */
    struct RiderNode* prev;      /* +0x04 */
    Bloke*            bloke;     /* +0x08 */
    unsigned short    ride_id;   /* +0x0c  packed {x,y} of the placement */
    unsigned short    pad0e;
    void*             person;    /* +0x10 */
} RiderNode;

/* The visitor bloke; same offsets as goldrush.c / goldrush2.c. */
struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;        /* +0x0e  low-level AI state (7 = walking) */
    unsigned char  pad10[0x24 - 0x10];
    Pos8           target;       /* +0x24  walk target, 24.8 */
    unsigned char  pad2c[0x34 - 0x2c];
    signed char    path_dir;     /* +0x34  walk-path step, +1 / -1 */
    unsigned char  pad35;
    unsigned char  pan;          /* +0x36  which pan slot the visitor has */
    unsigned char  pad37;
    short          path_node;    /* +0x38  current walk-path node */
    unsigned char  pad3a[0x60 - 0x3a];
    unsigned char  action;       /* +0x60  the ride's state-machine step */
    unsigned char  pad61[0x68 - 0x61];
    Pos8           world;        /* +0x68  world position, 24.8 */
    unsigned char  pad70[2];
    unsigned char  dir;          /* +0x72  facing */
    unsigned char  new_dir;      /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14];   /* +0x98  CalcMoveLine scratch */
};

/* One pan slot: which of the three pans, and how far across it to stand. */
typedef struct PanSlot {
    int   pan;                   /* +0x00  index into g_pan_pos */
    float frac;                  /* +0x04  0.8 or 0.2 */
} PanSlot;

extern PanSlot g_pan_slots[6];   /* 0x004b45b0 */
extern Pos8    g_pan_pos[3];     /* 0x004b4610 */

extern int  CalcMoveLine(Pos8 from, Pos8 to, void* path);            /* 0x00480740 */
extern int  NewDirForAction(Bloke* b, unsigned char dir);            /* 0x004833d0 */

/* --------------------------------------------------------------------------
 * 0x00406f60 -- GOLD RUSH state 3 / 8: take up the panning pose.
 *
 * The plain member of the family: no float, no offset, just the pan's own
 * square plus half a tile in each axis.  The six stores to Bloke +0x24/+0x28
 * are SIX SEPARATE STATEMENTS -- VC6 SP3 does not forward a struct-field
 * store to the load that follows it, so each `+=` costs a reload and a store
 * and the count is exact evidence of how many statements the source had.
 * -------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00406f60
void GoldRush_PoseAtPan(RiderNode* rd, MapSquare* key)
{
    Bloke*        b = rd->bloke;
    int           i = g_pan_slots[b->pan].pan;
    unsigned char a;

    b->target.x = g_pan_pos[i].x;
    b->target.y = g_pan_pos[i].y;
    b->target.x += key->bx << 8;
    b->target.y += key->by << 8;
    b->target.x += 0x80;
    b->target.y += 0x80;
    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
    b->state = 7;
    b->new_dir = a;
    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
}

/* --------------------------------------------------------------------------
 * 0x00407000 -- GOLD RUSH state 4: shuffle to the pan's near edge.
 *
 * PoseAtPan's target moved 2.5 tiles west (0x280) and then west again by the
 * slot's own fraction of a tile: `0x80 - (int)(frac * 512.0f)`, which is
 * -0x99 for the 0.8 slot and +0x1a for the 0.2 slot.  The two people at one
 * pan therefore end up 1.6 and 0.4 tiles apart, facing it from opposite
 * sides.
 *
 * The y side is ONE statement (`+= (by << 8) + 0x80`) where x is three: the
 * store count proves it, since VC6 emits a store and a reload per `+=` on a
 * struct field.  In KneelAtPan below the same y arithmetic IS two statements,
 * because __ftol sits between them and a store cannot migrate across a call.
 * -------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00407000
void GoldRush_MoveToPanEdge(RiderNode* rd, MapSquare* key)
{
    Bloke*        b = rd->bloke;
    PanSlot*      s = &g_pan_slots[b->pan];
    int           i = s->pan;
    float         f = s->frac;
    unsigned char a;

    b->target.x = g_pan_pos[i].x - 0x280;
    b->target.y = g_pan_pos[i].y;
    b->target.x += key->bx << 8;
    b->target.y += (key->by << 8) + 0x80;
    b->target.x += 0x80 - (int)(f * 512.0f);
    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
    b->state = 7;
    b->new_dir = a;
    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
}

/* --------------------------------------------------------------------------
 * 0x004070b0 -- GOLD RUSH state 5: kneel down at the pan.
 *
 * MoveToPanEdge's target with y taken 0x50 (0.3125 of a tile) NORTH instead
 * of 0x80 south, so the visitor shuffles up to the pan and drops.  The third
 * y statement lands AFTER the __ftol call that the x statement contains, and
 * a store cannot migrate across a call, so this is the one member of the
 * family with three separate y stores -- and the one that needs a second
 * callee-saved register to carry the value over the call.
 *
 * RESIDUAL, precisely (2026-09-05).  57 instructions against 58, 37 strict
 * mismatches, first divergence at index 3.  The whole difference is ONE
 * register decision: the original keeps the running y value in EDI (a third
 * push) from `mov edi,[esi+28h]` at index 16 through `add edi,-50h` at 29 and
 * the store at 35 to the CalcMoveLine argument at 37, i.e. ONE web spanning
 * the __ftol call; this build lets the web die at the y2 store and RELOADS
 * `[esi+28h]` after the call, which is one instruction shorter and needs no
 * push.  Everything else -- the pan-slot head, the three x stores, the three
 * y stores, their order and x's two reloads -- is the original's.
 *
 * MEASURED, and none of it closes the function:
 *  - the six store orders and both `-= 0x50` / `+= -0x50` spellings: inert.
 *  - `b->target.y -= 0x50;` BEFORE the x statement merges y2 and y3 into one
 *    store (55 instructions), so the third y store PROVES the call sits
 *    between them, and hoisting `(int)(f * 512.0f)` into its own statement
 *    lets VC6 SINK the call below the y statements and merge them again.
 *  - a plain `int ty` local carrying the y value gives the original's 58
 *    instructions but VC6 then enregisters x TOO (ebx+esi+edi, and the two
 *    target stores sink below the argument pushes): 55 of 58.
 *  - `int ty` plus a volatile READ of `b->target.x` in the x statement (so x
 *    cannot be forwarded across the call) is the closest point reached --
 *    58/58 instructions, the original's `push esi / xor ecx,ecx / push edi`
 *    prologue and both store positions, 26 mismatches -- but the ty load is
 *    still scheduled at index 23 instead of 16 and the sum accumulates the
 *    other way (`mov edi,edx / shl edi,8 / add edi,eax` against the
 *    original's `shl edx,8 / add edi,edx`).  Not adopted: it needs two
 *    constructs the original's source cannot plausibly have had, and it is
 *    still not exact.
 *  - free volatile reads on `b->target.y` at the definition site (with and
 *    without the carrier local), a volatile READ or a volatile STORE on
 *    `b->target.x` alone, `Pos8` aggregates for the running pair AND for the
 *    carrier (the aggregate lever that closed WalkPath_Advance below and
 *    Restaurant1_WalkToSeatSpot in ridemisc2.c), naming the __ftol result,
 *    naming the shifted byte, four statement orders for the two post-call
 *    statements, and every head spelling that MoveToPanEdge and
 *    StandUpFromPan match with: all inert or worse.  About seventy variants.
 *  - ALL TEN legal interleavings of the two three-statement chains (each
 *    chain in order, the third y store after the call) -- best 35, still 57
 *    instructions.
 *  - signature levers: returning NewDirForAction's result (`int` rather than
 *    `void`) is byte-identical; declaring CalcMoveLine with FIVE int
 *    parameters instead of two by-value `Pos8` (ridecb5.c's spelling) costs
 *    two more instructions.  The `Pos` BY-VALUE parameter lever that closed
 *    Restaurant1_WalkToSeatSpot in ridemisc2.c does not apply here -- both
 *    parameters are dereferenced pointers.
 * The body below is the reconstruction the mechanics say is right; the
 * residual is a register-allocation decision, not a missing statement.
 * -------------------------------------------------------------------------- */

// WIP-FUNCTION: LEGOLAND 0x004070b0  (78.9%, 57 insns vs 58; the y value wants EDI across __ftol, this build reloads it)
void GoldRush_KneelAtPan(RiderNode* rd, MapSquare* key)
{
    Bloke*        b = rd->bloke;
    PanSlot*      s = &g_pan_slots[b->pan];
    int           i = s->pan;
    float         f = s->frac;
    unsigned char a;

    b->target.x = g_pan_pos[i].x - 0x280;
    b->target.y = g_pan_pos[i].y;
    b->target.x += key->bx << 8;
    b->target.y += key->by << 8;
    b->target.x += 0x80 - (int)(f * 512.0f);
    b->target.y -= 0x50;
    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
    b->state = 7;
    b->new_dir = a;
    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
}

/* --------------------------------------------------------------------------
 * 0x00407170 -- GOLD RUSH state 7: stand up again.
 *
 * MoveToPanEdge exactly, with ONE extra leading statement: the bloke's WORLD
 * y is moved half a tile south before the walk order is issued, undoing the
 * kneel by teleporting rather than by walking.  (The kneel was 0x50 north and
 * this is 0x80 south, so a full cycle leaves the visitor 0x30 further south
 * than it began -- the original's asymmetry, reproduced.)
 * -------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00407170
void GoldRush_StandUpFromPan(RiderNode* rd, MapSquare* key)
{
    Bloke*        b = rd->bloke;
    PanSlot*      s = &g_pan_slots[b->pan];
    int           i = s->pan;
    float         f = s->frac;
    unsigned char a;

    b->world.y += 0x80;
    b->target.x = g_pan_pos[i].x - 0x280;
    b->target.y = g_pan_pos[i].y;
    b->target.x += key->bx << 8;
    b->target.y += (key->by << 8) + 0x80;
    b->target.x += 0x80 - (int)(f * 512.0f);
    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
    b->state = 7;
    b->new_dir = a;
    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
}

/* ---- the walk path (mappath.c's BuildWalkPath, 0x00412100, builds it) ---- */

/* One expanded step: a tile delta from the placement's anchor.  The third
 * dword is left zero by the builder and is what this walker would fill in. */
typedef struct WalkNode {
    int x;                       /* +0x00 */
    int y;                       /* +0x04 */
    int state;                   /* +0x08 */
} WalkNode;

typedef struct WalkPath {
    int       count;             /* +0x00  total nodes */
    WalkNode* nodes;             /* +0x04  == (WalkNode*)(path + 1) */
} WalkPath;

/* --------------------------------------------------------------------------
 * 0x00412300 -- step a bloke one node along a walk path.
 *
 * Called once a tick from GOLD RUSH's states 1 and 11 (goldrush.c) and from
 * the MECHANICAL COPTERS arm walker (mechrides.c).  It aims the bloke at
 * node[b->path_node] measured from the anchor tile (x, y), issues the usual
 * walk order, then steps the cursor by b->path_dir and advances the rider's
 * action byte when the cursor leaves the array -- at 0 going backwards, at
 * `count` going forwards.  The two end tests are written as the two arms of
 * ONE `if (dir < 0)`, which is why the backward arm carries its own inline
 * epilogue (`mov al,[esi+0x60] / pop edi / inc al / mov [esi+0x60],al /
 * pop esi / ret`) and the forward arm falls into the shared one with a plain
 * `inc byte ptr`.
 *
 * NOTE the target is the tile CORNER (`<< 8` with no half-tile bias), unlike
 * every hand-written waypoint in the ride.
 *
 * CODEGEN: the two sums MUST live in one `Pos8` aggregate.  Written straight
 * into `b->target.x` / `b->target.y` VC6 completes and stores x before it
 * even loads node.y (the whole middle of the body, 20 of 60); as two plain
 * `int` locals it hoists both parameter loads to the top and grabs EBX
 * (63 instructions); as ONE aggregate whose address is never taken it
 * evaluates node.x, node.y, both sums, both shifts, both stores -- the
 * original, exactly.  A `b->target = t;` whole-struct assignment instead of
 * the two field stores is three instructions SHORT (it keeps both values in
 * registers for the CalcMoveLine argument, where the original reloads x).
 * -------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00412300
void WalkPath_Advance(WalkPath* path, int x, int y, Bloke* b)
{
    int           n = b->path_node;
    Pos8          t;
    signed char   dir;
    unsigned char a;

    t.x = x + path->nodes[n].x;
    t.y = path->nodes[n].y + y;
    b->target.x = t.x << 8;
    b->target.y = t.y << 8;
    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
    b->state = 7;
    b->new_dir = a;
    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
    dir = b->path_dir;
    b->path_node += dir;
    if (dir < 0) {
        if (b->path_node < 0)
            b->action++;
    } else {
        if (b->path_node >= path->count)
            b->action++;
    }
}

/* ---- the driving school (schoolcar2.c / goldrush.c own these records) ---- */

typedef struct CarPos { int x; int y; } CarPos;

/* A 4x4-map-square road block; only the fields this test reads. */
typedef struct RoadRec {
    unsigned char pad00[0x0c];
    int           x;             /* +0x0c  block origin, a multiple of 4 */
    int           y;             /* +0x10 */
    unsigned char kind;          /* +0x14  low nibble: 5 = lights, 6 = entrance */
} RoadRec;

/* The car; only +0x20 (the square the manoeuvre starts from) and +0xb8 (the
 * sixteen-point body frame) are touched here.  schoolcar2.c has the rest. */
typedef struct SchoolCar {
    unsigned char pad00[0x20];
    CarPos        start;         /* +0x20 */
    unsigned char pad28[0xb8 - 0x28];
    unsigned char frame;         /* +0xb8  0..15 */
} SchoolCar;

/* Sixteen-point body frame -> eight-point road heading:
 * {3,3,4,5,5,5,6,7,7,7,0,1,1,1,2,3}.  Each of the four road headings 1/3/5/7
 * owns three frames and each diagonal exactly one. */
extern const int g_dir16to8[16];                                     /* 0x004b4034 */

extern RoadRec* GetRoadRecord(int x, int y);                         /* 0x004125f0 */
extern RoadRec* Road_FindAt(int x, int y);                           /* 0x004125a0 */
extern void     Pos_Step4(CarPos* from, CarPos* to, int dir);        /* 0x004808d0 */

/* 0x00402340: 1 if the junction lights let `dir` through.  Reads coaster.c's
 * g_light_ns (0x004cbeb0) and g_light_ew (0x004cbeb4): the north-south green
 * stops east/west (3, 7), the east-west green stops north/south (1, 5), and
 * all-red (both zero) stops everything. */
extern int      TrafficLightAllows(int dir);                         /* 0x00402340 */

/* --------------------------------------------------------------------------
 * 0x00402390 -- may the car move off its current square?
 *
 * A StepSchoolCar callee, tested before manoeuvres 1, 2 and 3 (the three that
 * cross onto a new block).  Returns 0 when there is no road block under the
 * square the manoeuvre starts from -- which is what strands a car whose road
 * was bulldozed from under it -- 1 normally, and the junction's own answer
 * when the block FOUR SQUARES AHEAD is a traffic-light block (kind nibble 5).
 *
 * `Pos_Step4(&sq, &sq, dir)` passes one local as both source and destination:
 * the step is done in place, which is why the original materialises the same
 * effective address into two different registers.  The heading is re-read
 * from the table at both call sites (the original reloads `c->frame` and
 * indexes again rather than caching it).
 *
 * The `ahead == 0 || nibble != 5` short-circuit EXILES the `return 1` block
 * past the whole function -- the recorded exile rule, and here it is what the
 * original does, so the merged guard is the right spelling.
 * -------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00402390
int SchoolCarAtTarget(SchoolCar* c)
{
    RoadRec* here;
    RoadRec* ahead;
    CarPos   sq;

    here = GetRoadRecord(c->start.x, c->start.y);
    if (!here)
        return 0;
    sq.x = here->x;
    sq.y = here->y;
    Pos_Step4(&sq, &sq, g_dir16to8[c->frame]);
    ahead = Road_FindAt(sq.x, sq.y);
    if (ahead == 0 || (ahead->kind & 0xf) != 5)
        return 1;
    return TrafficLightAllows(g_dir16to8[c->frame]);
}
