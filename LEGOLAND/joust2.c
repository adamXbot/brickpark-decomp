/* LEGOLAND -- overflow lane: three unowned bodies from three different
 * subsystems that had no home file of their own.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere), and deliberately mirror the ones in ridecb5.c / ridecb6.c
 * (boating school), mechrides.c (copters) and joust.c (joust).
 *
 *   addr        what it is                                      subsystem
 *   0x0041c690  BsWater_Probe        which lake arms touch a cell  BOATING SCHOOL
 *   0x004040f0  Copters_DrawVehicle  one copter, one frame         COPTERS
 *   0x00407c30  Joust_Update         the joust's per-tick update   JOUST
 *
 * NAMING: 0x00407c30 was handed to this lane as `Joust_A8` (the callback slot
 * it fills).  It is the JOUST class's +0xa8 ACTIVATE handler, i.e. the ride's
 * per-tick simulation step, and joust.c already declares it under the name
 * used here.  ridesave.c still declares the same address as
 * `extern void Joust_A8(void)`; that prototype has the WRONG arity (the
 * handler takes the RideElem) but it is only ever taken as a function
 * pointer, so it is left alone -- extern prototype types are caller-side
 * codegen levers and this lane does not own that file.
 *
 * TWO OTHER PLACES THIS FILE DELIBERATELY DISAGREES WITH ITS NEIGHBOURS:
 *  - joust.c's FXEntry names +0x04 `sample` and +0x08 `flags`.  Load_FXList
 *    (0x00496dd0) stores the created sample at +0x08, which is the field the
 *    joust update hands to PlayInstanceOfSample, so the FXEntry here has
 *    `sample` at +0x08.  joust.c never reads either field, so nothing there
 *    depends on it.
 *  - joust.c's JoustRec calls +0x1a `next_horse`.  Loop 2 below ticks it
 *    0..0x3f and loop 1 copies it into the animation frame at +0x1b, so it is
 *    named `cycle` here; the "which horse next" reading is what
 *    Joust_ByteIsSet makes of its low bit in state 2.
 *
 * ========================================================================= */

/* ---- shared map/cursor types (same offsets as ridecb5.c / ridecb6.c) ---- */
typedef struct Pos { int x; int y; } Pos;

/* A packed 2-byte map square passed BY VALUE, and its 16-bit view. */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union BPosW { unsigned short w; BPos b; } BPosW;

/* =========================================================================
 * BOATING SCHOOL -- 0x0041c690  BsWater_Probe
 * =========================================================================
 * THE LAKE IS A GRAPH ON A FIVE-CELL LATTICE.  Every BOATING SCHOOL WATER
 * square owns one 0x1c-byte record (head 0x004d823c) keyed by its own map
 * square, and two lake squares are NEIGHBOURS when their map coordinates
 * differ by exactly five in one axis.  This routine answers, for one map
 * square, "which of my four cardinal neighbours are water that belongs to the
 * SAME school as I do, and which school is that?":
 *
 *   bit 1  north  (x,     y - 5)
 *   bit 2  east   (x + 5, y)
 *   bit 4  south  (x,     y + 5)
 *   bit 8  west   (x - 5, y)
 *
 * and hands the owning school's packed map square back through `owner`.
 *
 * THE OWNER IS DECIDED BY THE FIRST CELL THAT HAS ONE, NOT BY THE CENTRE.
 * `have` starts clear; the centre cell sets it if a water record exists
 * there, and otherwise the first NEIGHBOUR that is water adopts ownership --
 * so probing a square that is not itself water still reports a school.  Once
 * `have` is set every further neighbour must match that school exactly or it
 * is left out of the mask, which is what stops two adjacent lakes owned by
 * different schools from being stitched into one route.
 *
 * THE SCHOOL'S OWN TWO DOCK CELLS COUNT AS ARMS.  The building is not a water
 * record, so after the four probes the station list (0x004cc074) is walked
 * and the probed square is compared against each station's two dock squares
 * (+0x02/+0x03 and +0x04/+0x05, matched as one 16-bit value).  Dock A adds
 * bit 1 and dock B adds bit 4 -- the same two masks ridecb5.c's placement
 * code stamps onto those cells -- so a lake arm that runs into the jetty is
 * reported as continuing north (dock A) or south (dock B).  Note the walk
 * stops at the FIRST station that matches either dock, and that it does NOT
 * check the owner: a dock always contributes its bit even when the probe
 * belongs to a different school.
 *
 * The four neighbour probes are each bounds-checked against the map header
 * (0x004bcbf4 +0x14/+0x16) BEFORE the lookup, in the order
 * x >= 0, y >= 0, x < width, y < height -- note this is a different order
 * from the map-cell fetch every other accessor open-codes, so it is its own
 * helper, not MapCellAt.
 * ========================================================================= */

typedef struct MapHdr {
    unsigned char  pad00[0x14];
    unsigned short width;           /* +0x14 */
    unsigned short height;          /* +0x16 */
} MapHdr;

/* One square of boating-school water: its own square and the school's. */
typedef struct BsWater {
    BPosW          pos;             /* +0x00 */
    BPosW          owner;           /* +0x02 the school that owns this cell */
    unsigned char  pad04[0x10 - 4];
} BsWater;

/* One placed boating school; only the head is read here.  The two dock
 * squares are the byte pairs at +0x02/+0x03 and +0x04/+0x05 (ridecb5.c). */
typedef struct BsStation {
    BPosW              key;         /* +0x00 the building's map square */
    BPosW              dock_a;      /* +0x02 near jetty  (arm bit 1, north) */
    BPosW              dock_b;      /* +0x04 far jetty   (arm bit 4, south) */
    unsigned char      pad06[0x2c - 6];
    struct BsStation*  next;        /* +0x2c */
    int                take;        /* +0x30 */
} BsStation;                        /* 0x34 */

extern MapHdr*    g_map;            /* 0x004bcbf4 (lpConfig) */
extern BsStation* g_bs_stations;    /* 0x004cc074 */

/* Returns the lake record whose OWN square is (x, y), or 0. */
extern BsWater* BsWater_FindAt(int x, int y);                    /* 0x0041c890 */

/* The bounds-checked lake lookup the four neighbour probes share.  Note the
 * test ORDER -- x >= 0, y >= 0, x < width, y < height -- differs from the
 * map-cell fetch every other accessor open-codes (x >= 0, x < width, ...),
 * and the coordinate arithmetic sits in the ARGUMENTS, which is why the
 * original computes `y - 5` before it tests `x`. */
static __inline BsWater* BsWater_FindInBounds(int x, int y)
{
    if (x >= 0 && y >= 0 && x < g_map->width && y < g_map->height)
        return BsWater_FindAt(x, y);
    return 0;
}

// FUNCTION: LEGOLAND 0x0041c690
int BsWater_Probe(int x, int y, BPosW* owner)
{
    int        mask = 0;
    int        have = 0;
    BsStation* st = g_bs_stations;
    BsWater*   w;
    BPosW      key;

    w = BsWater_FindAt(x, y);
    if (w) {
        owner->w = w->owner.w;
        have = 1;
    }

    w = BsWater_FindInBounds(x, y - 5);
    if (w) {
        if (have) {
            if (w->owner.w == owner->w)
                mask |= 1;
        } else {
            mask |= 1;
            owner->w = w->owner.w;
            have = 1;
        }
    }
    w = BsWater_FindInBounds(x + 5, y);
    if (w) {
        if (have) {
            if (w->owner.w == owner->w)
                mask |= 2;
        } else {
            mask |= 2;
            owner->w = w->owner.w;
            have = 1;
        }
    }
    w = BsWater_FindInBounds(x, y + 5);
    if (w) {
        if (have) {
            if (w->owner.w == owner->w)
                mask |= 4;
        } else {
            mask |= 4;
            owner->w = w->owner.w;
            have = 1;
        }
    }
    w = BsWater_FindInBounds(x - 5, y);
    if (w) {
        if (have) {
            if (w->owner.w == owner->w)
                mask |= 8;
        } else {
            mask |= 8;
            owner->w = w->owner.w;
            have = 1;
        }
    }

    key.b.x = (unsigned char)x;
    key.b.y = (unsigned char)y;
    while (st) {
        if (key.w == st->dock_a.w) {
            mask |= 1;
            break;
        }
        if (key.w == st->dock_b.w) {
            mask |= 4;
            break;
        }
        st = st->next;
    }
    return mask;
}

/* =========================================================================
 * COPTERS -- 0x004040f0  Copters_DrawVehicle
 * =========================================================================
 * Draws ONE of the ride's six copters for the current frame.  It is called
 * five times by Copters_Interact (mechrides.c 0x00404290) in the fixed order
 * 0, 2, 3, 4, 1, with a pass of the depth-sorted rider list between the
 * groups, so the copters interleave correctly with the blokes riding them.
 *
 * A copters record (mechrides.c, 0xd8 bytes) is the map square plus SIX
 * 0x20-byte seat records at +0x18; `which` indexes those directly
 * (`rec + 0x18 + which*0x20`).  The fields this routine reads are:
 *
 *   +0x00 u8   flags   bit 0 selects the ALTERNATE layer/sprite pair
 *   +0x04 s8   frame   the animation frame for BOTH sprites (signed)
 *   +0x08 int  layer_a build-sprite layer, pair A
 *   +0x0c int  layer_b build-sprite layer, pair B
 *   +0x10 int  spr_a   index into the loaded copter sprites, pair A
 *   +0x14 int  spr_b   index into the loaded copter sprites, pair B
 *   +0x18      rider   the RiderNode sitting in this copter, or 0
 *
 * WHAT IT DOES, in order:
 *  1. picks (layer, sprite index) = pair B if flags bit 0 is set, else pair A;
 *  2. STOPS AND REWINDS three of the class's layer animations -- layer_a,
 *     spr_a and layer_b, in that order.  Note the ASYMMETRY: `spr_a`
 *     (+0x10) is handed to GetLLSForLayer as if it were a layer number even
 *     though its other use is a sprite-table INDEX, and `spr_b` (+0x14) is
 *     never reset at all.  That is the original's, not a transcription slip;
 *     the three resets are what stop the previous frame's animation from
 *     continuing to play under the one about to be drawn.
 *  3. takes the square's screen origin and the chosen layer's render offset,
 *     adjusts the offset for the current view mode, and blits the build
 *     sprite's layer at origin+offset;
 *  4. if a rider is seated, pushes it through the 3D bloke renderer so the
 *     passenger is drawn between the two sprite passes;
 *  5. blits the cached copter sprite g_copters_spr[idx] (0x004c113c) over the
 *     top at the SAME screen position, after setting its frame.
 *
 * Both blits set the LLS frame from the seat's +0x04 byte, so the vehicle
 * body and the copter overlay always run on one shared frame counter.
 *
 * LEVER (worth the whole residual, 8 of 150): the stop/rewind helper takes
 * the LLS, not the layer number -- `Copters_StopLayerAnim(GetLLSForLayer(
 * g_copters_layers, v->layer_a))`.  Only the NESTED-CALL form keeps VC6's
 * eax->ecx->edx scratch rotation running across the three expansions
 * (eax,ecx | edx,eax | ecx,edx); a helper that takes the layer and reads the
 * global itself gets block one right but restarts the rotation at eax for
 * blocks two and three, costing 8 mismatches and 2 bytes.
 * ========================================================================= */

typedef struct Spr { unsigned char pad00[0x10]; unsigned int flags; } Spr;

/* The class record (ObjDef).  Only the four fields the two bodies below read
 * are named; the Joust update takes its map origin from +0x0c/+0x10 and walks
 * the class-wide rider list at +0xcc. */
typedef struct RideDef {
    unsigned char pad00[0x0c];
    int           base_x;        /* +0x0c the class's base map square */
    int           base_y;        /* +0x10 */
    unsigned char pad14[0x64 - 0x14];
    Spr*          sprite;        /* +0x64 the class build-anim sprite */
    unsigned char pad68[0xcc - 0x68];
    struct RiderNode* riders;    /* +0xcc the class-wide rider list */
} RideDef;

/* The packed map square a placement is keyed by (mechrides.c's RideTile). */
typedef union RideTile {
    unsigned short key;
    struct { unsigned char x, y; } b;
} RideTile;

typedef struct Offset { int ox; int oy; } Offset;

typedef struct Bloke Bloke;
struct RiderNode;

/* A rider slot on the class-wide list at ObjDef+0xcc (mechrides.c). */
typedef struct RiderNode {
    struct RiderNode* next;      /* +0x00 */
    struct RiderNode* prev;      /* +0x04 */
    Bloke*            bloke;     /* +0x08 */
    unsigned short    ride_id;   /* +0x0c the packed map square it is using */
    unsigned short    pad0e;
    void*             owner;     /* +0x10 */
} RiderNode;

typedef struct CopterSeat {
    unsigned char flags;         /* +0x00 bit 0 selects pair B */
    unsigned char pad01[3];
    signed char   frame;         /* +0x04 shared animation frame */
    unsigned char pad05[3];
    int           layer_a;       /* +0x08 */
    int           layer_b;       /* +0x0c */
    int           spr_a;         /* +0x10 */
    int           spr_b;         /* +0x14 */
    RiderNode*    rider;         /* +0x18 */
    unsigned char pad1c[4];
} CopterSeat;                    /* 0x20 */

typedef struct CoptersRec {
    RideTile      tile;          /* +0x00 the map square this copy sits on */
    unsigned char pad02[0x16];
    CopterSeat    seat[6];       /* +0x18 .. +0xd8 */
} CoptersRec;

extern RideDef* g_copters_def;                               /* 0x004c1198 */
extern void*    g_copters_layers;                            /* 0x004c1138 */
extern void*    g_copters_spr[10];                           /* 0x004c113c */

extern void*  GetLLSForLayer(void* sprite, int layer);              /* 0x00441ea0 */
extern void*  GetSpriteForLayer(void* sprite, int layer);           /* 0x00441ec0 */
extern Offset GetRenderOffsetForLayer(void* sprite, int layer);     /* 0x00441ee0 */
extern void*  GetLLSForSprite(void* spr);                           /* 0x00441e80 */
extern void   LLSStop(void* lls);                                   /* 0x0047d4c0 */
extern void   LLSSetFrame(void* lls, int frame);                    /* 0x0047d5a0 */
extern Offset GetScreenCoordsForObject(RideTile* sq, RideDef* item);/* 0x00442cc0 */
extern void   AdjustOffsetForViewMode(Offset* o);                   /* 0x00442d30 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* c);/* 0x004853a0 */
extern void   IP_RenderBlokeIn3DNow(Bloke* b);                      /* 0x00440010 */

/* Take one layer animation back to a stopped frame 0.  It must take the LLS
 * and be called with the lookup NESTED in the argument -- see the lever note
 * above. */
static __inline void Copters_StopLayerAnim(void* lls)
{
    if (lls) {
        LLSStop(lls);
        LLSSetFrame(lls, 0);
    }
}

// FUNCTION: LEGOLAND 0x004040f0
void Copters_DrawVehicle(CoptersRec* rec, int which, int mode)
{
    CopterSeat* v = &rec->seat[which];
    Offset      screen;
    Offset      off;
    void*       spr;
    void*       lls;
    int         layer;
    int         idx;

    if (v->flags & 1) {
        idx = v->spr_b;
        layer = v->layer_b;
    } else {
        idx = v->spr_a;
        layer = v->layer_a;
    }
    Copters_StopLayerAnim(GetLLSForLayer(g_copters_layers, v->layer_a));
    Copters_StopLayerAnim(GetLLSForLayer(g_copters_layers, v->spr_a));
    Copters_StopLayerAnim(GetLLSForLayer(g_copters_layers, v->layer_b));

    screen = GetScreenCoordsForObject(&rec->tile, g_copters_def);
    off = GetRenderOffsetForLayer(g_copters_def->sprite, layer);
    AdjustOffsetForViewMode(&off);
    spr = GetSpriteForLayer(g_copters_def->sprite, layer);
    if (spr) {
        lls = GetLLSForSprite(spr);
        if (lls)
            LLSSetFrame(lls, v->frame);
    }
    PrintSprite(spr, screen.ox + off.ox, screen.oy + off.oy, mode, 0);
    if (v->rider)
        IP_RenderBlokeIn3DNow(v->rider->bloke);
    spr = g_copters_spr[idx];
    if (spr) {
        lls = GetLLSForSprite(spr);
        if (lls)
            LLSSetFrame(lls, v->frame);
        PrintSprite(spr, screen.ox + off.ox, screen.oy + off.oy, mode, 0);
    }
}

/* =========================================================================
 * JOUST -- 0x00407c30  Joust_Update  (the class's +0xa8 per-tick update)
 * =========================================================================
 * THE JOUST HAS TWO POPULATIONS AND TWO LOOPS.
 *
 * LOOP 1 walks the class-wide rider list (ObjDef+0xcc) and runs a
 * THIRTY-ONE-state machine on each rider's action byte (Bloke+0x60): states
 * 0..7 are the two JOUSTERS (queue up, claim a horse, mount, ride, dismount,
 * leave) and 0x0a..0x1e are the SPECTATORS, one chain per side of the arena.
 * States 8, 9 and 0x11..0x13 are gaps -- they fall into the default arm and
 * do nothing.  Each iteration:
 *   - finds the per-placement record for the rider's map square and, IF THERE
 *     IS NONE, RETURNS FROM THE WHOLE HANDLER -- it does not skip the rider
 *     and it does not run loop 2 either, so a rider whose ride has just been
 *     removed under it freezes every joust in the park for that frame
 *     (original behaviour, reproduced);
 *   - copies the record's mutable fields into frame locals, publishes the
 *     arena animation frame into the depth sprite's layer header, and works
 *     on the COPY -- one shared writeback at the bottom of the loop puts them
 *     back, so nothing the switch does is visible to another rider until the
 *     next iteration;
 *   - skips the switch entirely when the rider's low-level AI is busy
 *     (Bloke+0x0e != 0) but STILL runs the writeback.
 *
 * The record's copied fields, and what the two loops make of them:
 *   +0x0c f0c       the arena's "a jouster is queueing" counter: state 1
 *                   sets it to 1 when a spectator arrives with none set, and
 *                   state 2 decrements it when a jouster commits
 *   +0x10 jousters  how many riders are on horses (0, 1 or 2)
 *   +0x11 seated    how many spectators are in the stands (max 6)
 *   +0x12 seats[6]  the six stand seats: 0 free, 2 taken
 *   +0x18 horse[2]  per-horse state: 0 free, 1 claimed, 2 mounted, 3 riding
 *   +0x1a cycle     the arena cycle counter, 0..0x3f, ticked by loop 2 --
 *                   it is BOTH the BNV frame the riders are positioned at and
 *                   (through Joust_ByteIsSet) the index of the horse the next
 *                   jouster gets, so the two horses are half a cycle apart
 *   +0x1b frame     the animation frame the draw handler quotes; loop 1
 *                   writes the CYCLE into it every iteration
 *   +0x1c f1c       "horse free, waiting to be claimed" -- recomputed from
 *                   scratch by loop 2 every tick and read by state 2
 *   +0x20 f20       "the ride is over, dismount" -- likewise, read by state 6
 *
 * A SPECTATOR'S PATH: state 0 walks to (tile+1, tile-1); state 1 either sets
 * the queue flag and walks one square further (no joust pending) or picks a
 * seat -- `rand() % 6` then a forward scan for a free one -- claims it, and
 * jumps to 0x0a for seats 0..2 or 0x14 for seats 3..5, the two sides of the
 * arena.  0x0a..0x10 and 0x14..0x1e are those two walk-to-your-seat chains;
 * 0x0d/0x19 is the WATCHING state (a 150-tick countdown on Bloke+0x58, then
 * free the seat) and 0x10/0x1d/0x1e are the exits.
 *
 * A JOUSTER'S PATH: state 2 waits for f1c, claims the horse the cycle points
 * at, remembers its index in Bloke+0x44 and walks to it; state 3 turns on the
 * riding flag, hands the 3D person the joust depth sprite and places the
 * rider on the BNV path; states 4..6 re-place the rider at the current cycle
 * frame each tick, hold for 150 ticks and then walk off when f20 says the
 * pass is over AND the cycle has come back round to this rider's horse.
 * The BNV object name is built ON THE STACK: "manBox??" is copied into a
 * 9-byte local and sprintf(&name[6], "%02d", horse + 1) finishes it.
 *
 * LOOP 2 walks the record list (0x004c1250) and drives the arena cycle and
 * its looping sound.  At cycle 0 it evaluates HORSE 0, at cycle 0x20 HORSE 1,
 * and at any other value it just runs.  The evaluation is the same both ways:
 *   if ((f0c == 0 || jousters >= 2) && jousters != 0)
 *        horse == 1 -> STOP; horse == 3 -> f20 = 1, STOP; else RUN
 *   else horse == 0 -> f1c = 1, STOP; else RUN
 *   RUN  = start the looping "Joust Horses.wav" instance if the record has
 *          none, advance the cycle (wrapping past 0x3f) and clear BOTH gates;
 *   STOP = fade the record's sample out and clear it, leave the cycle alone
 *          and leave whichever gate was just set standing.
 * So the horses only turn over when the arena is idle or full, the sound
 * plays exactly while the cycle is advancing, and f1c/f20 are recomputed from
 * scratch every tick -- which is why loop 1 only ever reads them.
 * ========================================================================= */

/* The Joust's depth sprite as this body reads it: the update publishes the
 * arena's animation frame into the FIRST entry of the layer-header block the
 * sprite points at (+0x08 -> a pointer -> the u16 frame). */
typedef struct ZSpr {
    unsigned char pad00[8];
    short**       layers;        /* +0x08 -> the layer-header block */
} ZSpr;

typedef struct RideElem {
    char*        name;           /* +0x00 */
    char*        image;          /* +0x04 */
    unsigned int type_flags;     /* +0x08 */
    RideDef*     data;           /* +0x0c */
} RideElem;

typedef struct Person3D {
    unsigned char pad00[0x2c];
    void*         zsprite;       /* +0x2c */
    int           f30;           /* +0x30 */
} Person3D;

struct Bloke {
    unsigned char  pad00[4];
    Person3D*      person;       /* +0x04 */
    unsigned char  pad08[6];
    unsigned short state;        /* +0x0e */
    unsigned char  pad10[0x24 - 0x10];
    Pos            target;       /* +0x24 */
    unsigned char  pad2c[0x36 - 0x2c];
    unsigned char  seat;         /* +0x36 */
    unsigned char  pad37[0x44 - 0x37];
    short          horse_id;     /* +0x44 */
    unsigned char  pad46[0x58 - 0x46];
    int            timer;        /* +0x58 */
    unsigned char  pad5c[4];
    unsigned char  action;       /* +0x60 */
    unsigned char  pad61;
    unsigned short flags;        /* +0x62 */
    unsigned char  pad64[4];
    Pos            world;        /* +0x68 */
    unsigned char  pad70[2];
    unsigned char  sit;          /* +0x72 */
    unsigned char  new_dir;      /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14];   /* +0x98 */
};


typedef struct RideSoundSource {
    int kind;                    /* +0x00 */
    int f04;                     /* +0x04 never initialised for kind 2 */
    int x;                       /* +0x08 */
    int y;                       /* +0x0c */
} RideSoundSource;

typedef struct FXEntry {
    const char* name;            /* +0x00 */
    int         f04;             /* +0x04 */
    void*       sample;          /* +0x08 Load_FXList fills this in */
} FXEntry;

typedef struct JoustSeats { unsigned char s[6]; } JoustSeats;
typedef union  JoustHorses { unsigned short w; unsigned char h[2]; } JoustHorses;

#pragma pack(push, 1)
typedef struct JoustRec {
    RideTile         tile;       /* +0x00 */
    unsigned short   pad02;
    struct JoustRec* next;       /* +0x04 */
    void*            sample;     /* +0x08 */
    int              f0c;        /* +0x0c */
    char             jousters;   /* +0x10 */
    char             seated;     /* +0x11 */
    JoustSeats       seats;      /* +0x12 */
    JoustHorses      horses;     /* +0x18 */
    char             cycle;      /* +0x1a */
    char             frame;      /* +0x1b */
    int              f1c;        /* +0x1c */
    int              f20;        /* +0x20 */
} JoustRec;
#pragma pack(pop)

extern JoustRec* g_joust_head;                               /* 0x004c1250 */
extern ZSpr*     g_joust_zspr;                               /* 0x004c1210 */
extern void*     g_joust_zspr2;                              /* 0x004c1240 */
extern void*     g_joust_binv;                               /* 0x004c1218 */
extern FXEntry   g_joust_fx[];                               /* 0x004b4688 */

extern JoustRec* Joust_FindRecord(RideTile volatile* tile);  /* 0x00407a20 */
extern char      Joust_ByteIsSet(char v);                    /* 0x00407c20 */
extern int       CalcMoveLine(Pos from, Pos to, void* path); /* 0x00480740 */
extern int       NewDirForAction(Bloke* b, unsigned char dir);/* 0x004833d0 */
extern void      RemoveBlokeFromRide(RideDef* item, RiderNode* r); /* 0x0048a100 */
extern void SetBlokePositionFromBNV(void* bin, Bloke* b, const char* name, int frame, float near_z, float far_z, int flag); /* 0x00484a70 */
extern void      UnSourceAndFadeAllSamplesFromSource(RideSoundSource* src, int fade); /* 0x00496c80 */
extern void*     PlayInstanceOfSample(void* def, int a, int b, RideSoundSource* src); /* 0x00496d20 */
extern void      SetSampleLooping(void* s);                  /* 0x00496d10 */
extern int       rand(void);                                 /* 0x0049e4b2 (CRT) */
extern int       sprintf(char* buf, const char* fmt, ...);   /* 0x0049e573 (CRT) */

typedef struct JoustName { char c[9]; } JoustName;
static const JoustName k_manbox = { "manBox??" };

static __inline void Joust_Walk(Bloke* b)
{
    b->new_dir = (unsigned char)CalcMoveLine(b->world, b->target, b->path) + 0x10;
    b->state = 7;
    NewDirForAction(b, (unsigned char)((b->new_dir >> 5) + 3));
}

static __inline void Joust_FadeSample(int x, int y)
{
    RideSoundSource src;

    src.kind = 2;
    src.x = x;
    src.y = y;
    UnSourceAndFadeAllSamplesFromSource(&src, -1000);
}

static __inline void* Joust_StartSample(int x, int y)
{
    RideSoundSource src;

    src.kind = 2;
    src.x = x;
    src.y = y;
    return PlayInstanceOfSample(g_joust_fx[0].sample, 1, 0, &src);
}

/* RESIDUAL (measured 2026-09-05).  audit: 703/703 instructions, 2261/2258
 * bytes, mismatch 16/703 (97.7% exact), FIRST DIVERGING INDEX 500.  Every
 * block boundary, every jump target and every frame home is the original's;
 * the whole residual is scratch-register allocation in two places.
 *
 *  - 500-503 + 522-525 (6).  Case 0x18's seat multiply rotates eax->eax->ecx
 *    ->edx in the original and eax->eax->edx->eax here, and that decides
 *    which of the two "reload both targets" push blocks its tail merges
 *    into: the original's case 0x18 joins case 0x1c's chain (whose push
 *    block therefore has to RELOAD b->target.y from memory, because case
 *    0x18 arrives with the y value in edx, not ebp), ours joins case 0x0a's
 *    and case 0x1c's chain then forward-substitutes ebp (`mov ecx,ebp`
 *    instead of `mov ecx,[esi+0x28]`).  The two are one fact, and the cause
 *    is upstream: indices 0..499 are byte-identical in both builds, so the
 *    allocator state entering case 0x18 "should" be the same.  Eliminated
 *    here: every association and operand order of `(ty << 8) + seat*200 - K`
 *    (all canonicalise), `200 * seat`, `(seat*25)*8`, `(seat*5)*40`, a plain
 *    int temp for the seat, a free `volatile` byte read of b->seat (moves the
 *    first divergence earlier, 18), and swapping the two target stores in
 *    that case alone (19).  Case 0x0c has the SAME source expression and
 *    matches, which is what makes this an allocator-state difference rather
 *    than a spelling one.
 *  - 608/609, 623, 629, 649/650, 668, 673, 700/701 (10).  Phase 2's two gate
 *    locals get the callee-saved pair the other way round: the original puts
 *    f1c in edi and f20 in ebp, we put f1c in ebp and f20 in edi (index 673
 *    is the same difference seen through the cross-jump it changes -- the
 *    original merges horse 0's `jne run / mov ebp,1` pair, we merge only the
 *    `mov ebp,1` and reach it with a `je`).  ELIMINATED: both orders of the
 *    two initialisers and of the two RUN-arm clears and of the two writeback
 *    stores; all six orders of the five phase-2 record reads; separate
 *    block-scope gate locals instead of reusing phase 1's (identical);
 *    swapping the f1c/f20 DECLARATION order; a `for` loop instead of the
 *    `while`; reusing `rec` as the phase-2 cursor; declaring the cursor
 *    before `rec`.  Nothing moves it, which by the free-volatile rule in
 *    docs/DECOMP.md makes this a global web RANK, not a local rotation.
 *
 * LEVERS THAT GOT IT HERE, each worth a lot and each transferable:
 *  1. The "manBox??" template must be a STRUCT ASSIGNMENT placed after
 *     `r = def->riders;`, not a `char name[9] = "manBox??"` declaration
 *     initialiser.  A declaration initialiser is pinned to the top of the
 *     function and VC6 then emits the three template stores BEFORE the
 *     def/rider setup; the original interleaves them after it.  Worth 58 and
 *     it moved the first divergence from 3 to 40.
 *  2. Cases 0x0f and 0x19 have their OWN bodies in the source (VC6 merges
 *     each with 0x0a's / 0x0d's whole block, which is why the jump table
 *     points both entries at one address).  Writing them as shared labels
 *     `case 0x0a: case 0x0f:` costs 105 and two instructions.  Case 0x1a is
 *     NOT like that -- it really is a shared label on case 0x17's block, and
 *     giving it its own body costs 23.
 *  3. The busy-AI guard is `if (b->state == 0) { switch ... }`, NOT the
 *     `goto endsw` form Temple Slide's update needs.  Worth 153 on this body:
 *     it is what hosts the two merged CalcMoveLine tails in the LAST case of
 *     each group (0x1c's and 0x1d's) instead of the first, which is the whole
 *     block layout of the 0x0a..0x1e half of the switch.
 *  4. Phase 2's two horse evaluations are `if (!((f0c == 0 || jousters >= 2)
 *     && jousters != 0)) { <horse is free> } else { <horse is busy> }`.  The
 *     NEGATED form keeps the original's test order (the positive condition,
 *     evaluated the same way) while putting the "free" arm inline and the
 *     "busy" arm after it; the positive form swaps them and costs 8.
 *  5. Statement order that is load-bearing and was found by sweeping: the
 *     record copy is f0c, jousters, seated, seats, horses, cycle, then the
 *     z-sprite store; the writeback is f0c, jousters, seated, seats, horses,
 *     frame, f1c, f20.  `seats` and `horses` swapped in the copy is worth 4,
 *     `seated` before `seats` in the writeback 8, `horses` before `frame` 4.
 *  6. `b->action = (unsigned char)(s < 3 ? 0x0a : 0x14)` -- the ternary's
 *     TRUE value is the one VC6 uses as the `add` base, so the `>= 3 ? 0x14 :
 *     0x0a` spelling emits `setl / and 0xa / add 0xa` where the original has
 *     `setge / and 0xfffffff6 / add 0x14`.  Worth 3.
 * INERT (measured, so nobody repeats them): a `dir` local vs reading
 * b->new_dir back in the walk helper; the walk written out longhand in every
 * case instead of the inline helper; a `Joust_WalkTo(b, x, y)` helper taking
 * the target as arguments; explicit empty `case 8/9/0x11/0x12/0x13` and a
 * `default:`; every order of the three loop-head reads; `tx`/`ty` operand
 * orders and two-step accumulate forms; passing `(RideTile*)&r->ride_id`
 * inline; dropping the `volatile` from Joust_FindRecord's parameter type. */
/* RULED OUT for the two remaining encodings (measured this pass, so nobody
 * repeats them).  Tail copy membership: an exhaustive single-move plus
 * pairwise-swap hill climb over all twenty-five case bodies, started from
 * both the numeric order and this one, and a several-thousand-iteration
 * randomized multi-move search on top of the fixed gate, never place both
 * 0x16 and 0x18 on copy B -- seven of eight tail targets is the ceiling, and
 * every order that fixes 0x18 costs 0x16.  Explicit empty `case 8/9/0x11/
 * 0x12/0x13` and a `default:` are inert at every position.  Respellings that
 * VC6 folds to the identical instructions, and therefore cannot break the
 * tie: `(b->seat - 3) * 200 - 0xdd6`, `((ty - 15) << 8) - 0x46`,
 * `((tx - 3) << 8) - 0xa4`, and case 0x18's walk written out longhand.  For
 * index 673: an explicit `goto stop;` after either gate assignment, flipping
 * the cycle arm order, testing 0x20 first, `if (h == 3) goto set20; goto
 * run;`, swapping the stop/run block order, a combined `int f1c, f20;`
 * declaration in either order, and separate phase-two gate locals for one
 * gate only (either one) are all inert or much worse.  Everything in the
 * earlier ELIMINATED lists still holds. */
/* INDEX 673 CLOSED (fgh-100b): the busy arms are `if (h != 1) { if (h == 3)
 * f20 = 1; else goto run; }` (horse 0's with an explicit `else goto stop;`).
 * The nested form makes the front end emit the `cmp 3 / je set / jmp run /
 * set:` jump-around-jump on BOTH arms, so the tail merger sees an identical
 * `je / jmp run / label / mov ebp,1` suffix and folds horse 0's arm into a
 * single `jmp` landing on horse 1's `jne run` -- the original's encoding.  The
 * flat `if (h != 3) goto run;` spelling is already a bare `jne run` when the
 * merger runs and it only ever folds the `mov`.  Verified by differential
 * execution, 1200/1200. */
/* CASE 0x16's TAIL COPY CLOSED (fgh-100b, 2258/2258 bytes): the walk-tail
 * groups are decided BEFORE register allocation and depend on which case
 * bodies sit between which, including bodies VC6 later merges away whole.
 * Two layout-neutral moves put 0x16 on copy B (0x1c's, with 0x0b and 0x18)
 * where it had been on copy A (0x0e's): the 0x19 duplicate body sits right
 * after 0x0d instead of between 0x15 and 0x16, and 0x1a is its OWN duplicate
 * of 0x17's body placed between 0x16 and 0x18 (it still merges into 0x17's
 * block, so the jump table is unchanged).  Neither move alone does it; the
 * pair was found by a 2-D sweep of the two duplicates' slots
 * (scratchpad/fgh100b/joust_v17.py, four other slot pairs also give 2258).
 * The earlier note that a separate 0x1a body "costs 23" was true only for
 * the slot next to 0x17.  Verified by differential execution, 1200/1200. */
// FUNCTION: LEGOLAND 0x00407c30
void Joust_Update(RideElem* elem)
{
    RideDef*    def = elem->data;
    RiderNode*  r;
    RiderNode*  next;
    RideTile*   sq;
    Bloke*      b;
    JoustRec*   rec;
    JoustRec*   arena;
    int         tx;
    int         ty;
    char        jousters;
    char        seated;
    char        cycle;
    int         f0c;
    int         f1c;
    int         f20;
    JoustSeats  seats;
    JoustHorses horses;
    JoustName   name;

    r = def->riders;
    name = k_manbox;
    while (r) {
        next = r->next;
        b = r->bloke;
        sq = (RideTile*)&r->ride_id;
        rec = Joust_FindRecord(sq);
        if (!rec)
            return;
        f0c = rec->f0c;
        jousters = rec->jousters;
        seated = rec->seated;
        seats = rec->seats;
        horses = rec->horses;
        cycle = rec->cycle;
        f1c = rec->f1c;
        f20 = rec->f20;
        (*g_joust_zspr->layers)[0] = rec->frame;
        tx = def->base_x + sq->b.x;
        ty = def->base_y + sq->b.y;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags |= 8;
                b->target.x = (tx + 1) << 8;
                b->target.y = (ty << 8) - 0x100;
                Joust_Walk(b);
                b->timer = 0;
                b->action++;
                break;

            case 1:
                if (f0c == 0) {
                    b->target.x = (tx + 2) << 8;
                    b->target.y = (ty << 8) - 0x100;
                    Joust_Walk(b);
                    b->action++;
                    f0c = 1;
                } else if (seated < 6) {
                    char s = (char)(rand() % 6);

                    while (seats.s[s] != 0) {
                        s++;
                        if (s > 5)
                            s = 0;
                    }
                    b->seat = (unsigned char)s;
                    seats.s[s] = 2;
                    seated++;
                    b->action = (unsigned char)(s < 3 ? 0x0a : 0x14);
                }
                break;

            case 2:
                if (f1c != 0) {
                    short h;

                    b->target.x = (tx + 1) << 8;
                    b->target.y = (ty - 3) << 8;
                    Joust_Walk(b);
                    h = Joust_ByteIsSet(cycle);
                    b->horse_id = h;
                    horses.h[h] = 1;
                    b->action++;
                    jousters++;
                    f0c--;
                }
                break;

            case 3:
                b->flags |= 0x80;
                b->person->zsprite = g_joust_zspr2;
                b->person->f30 = 1;
                sprintf(&name.c[6], "%02d", b->horse_id + 1);
                SetBlokePositionFromBNV(g_joust_binv, b, name.c, cycle,
                                        -1617735.0f, -1617993.5f, 0);
                b->action++;
                break;

            case 4:
                horses.h[b->horse_id] = 2;
                sprintf(&name.c[6], "%02d", b->horse_id + 1);
                SetBlokePositionFromBNV(g_joust_binv, b, name.c, cycle,
                                        -1617735.0f, -1617993.5f, 0);
                if (b->timer++ > 150)
                    b->action++;
                break;

            case 5:
                sprintf(&name.c[6], "%02d", b->horse_id + 1);
                SetBlokePositionFromBNV(g_joust_binv, b, name.c, cycle,
                                        -1617735.0f, -1617993.5f, 0);
                horses.h[b->horse_id] = 3;
                b->action++;
                break;

            case 6:
                sprintf(&name.c[6], "%02d", b->horse_id + 1);
                SetBlokePositionFromBNV(g_joust_binv, b, name.c, cycle,
                                        -1617735.0f, -1617993.5f, 0);
                if (f20 != 0 && (short)Joust_ByteIsSet(cycle) == b->horse_id) {
                    b->flags &= ~0x80;
                    b->target.x = (tx + 2) << 8;
                    b->target.y = (ty << 8) - 0x100;
                    b->new_dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                             b->path) + 0x10;
                    b->state = 7;
                    b->person->zsprite = 0;
                    b->person->f30 = 0;
                    NewDirForAction(b, (unsigned char)((b->new_dir >> 5) + 3));
                    horses.h[b->horse_id] = 1;
                    b->action++;
                    jousters--;
                }
                break;

            case 7:
                b->target.x = tx << 8;
                b->target.y = ty << 8;
                Joust_Walk(b);
                b->action = 0x1e;
                horses.h[b->horse_id] = 0;
                break;

            case 0x0a:
                b->target.x = (tx << 8) - 0x100;
                b->target.y = (ty << 8) - 0x100;
                Joust_Walk(b);
                b->action++;
                break;

            case 0x0b:
                b->target.x = (tx << 8) - 0x364;
                b->target.y = (ty - 3) << 8;
                Joust_Walk(b);
                b->action++;
                break;

            case 0x0c:
                b->target.x = (tx << 8) - 0x364;
                b->target.y = (ty << 8) + b->seat * 200 - 0x638;
                Joust_Walk(b);
                b->action++;
                break;

            case 0x0d:
                b->sit = 3;
                if (b->timer++ > 150) {
                    seated--;
                    seats.s[b->seat] = 0;
                    b->action++;
                }
                break;

            case 0x19:
                b->sit = 3;
                if (b->timer++ > 150) {
                    seated--;
                    seats.s[b->seat] = 0;
                    b->action++;
                }
                break;

            case 0x0e:
                b->target.x = (tx << 8) - 0x364;
                b->target.y = (ty - 3) << 8;
                Joust_Walk(b);
                b->action++;
                break;

            case 0x0f:
                b->target.x = (tx << 8) - 0x100;
                b->target.y = (ty << 8) - 0x100;
                Joust_Walk(b);
                b->action++;
                break;

            case 0x10:
                b->target.x = tx << 8;
                b->target.y = ty << 8;
                Joust_Walk(b);
                b->action = 0x1e;
                break;

            case 0x14:
                b->target.x = (tx << 8) - 0x100;
                b->target.y = (ty << 8) - 0x100;
                Joust_Walk(b);
                b->action++;
                break;

            case 0x15:
                b->target.x = (tx << 8) - 0x16a;
                b->target.y = (ty - 2) << 8;
                Joust_Walk(b);
                b->action++;
                break;

            case 0x16:
                b->target.x = (tx << 8) - 0x16a;
                b->target.y = (ty << 8) - 0xf46;
                Joust_Walk(b);
                b->action++;
                break;

            case 0x1a:
                b->target.x = (tx << 8) - 0x364;
                b->target.y = (ty << 8) - 0xf46;
                Joust_Walk(b);
                b->action++;
                break;

            case 0x18:
                b->target.x = (tx << 8) - 0x364;
                b->target.y = (ty << 8) + b->seat * 200 - 0x102e;
                Joust_Walk(b);
                b->action++;
                break;

            case 0x17:
                b->target.x = (tx << 8) - 0x364;
                b->target.y = (ty << 8) - 0xf46;
                Joust_Walk(b);
                b->action++;
                break;

            case 0x1b:
                b->target.x = (tx << 8) - 0x16a;
                b->target.y = (ty << 8) - 0xf46;
                Joust_Walk(b);
                b->action++;
                break;

            case 0x1c:
                b->target.x = (tx << 8) - 0x16a;
                b->target.y = (ty - 2) << 8;
                Joust_Walk(b);
                b->action++;
                break;

            case 0x1d:
                b->target.x = tx << 8;
                b->target.y = ty << 8;
                Joust_Walk(b);
                b->action = 0x1e;
                break;

            case 0x1e:
                RemoveBlokeFromRide(def, r);
                b->flags &= ~8;
                break;
            }
        }
        rec->f0c = f0c;
        rec->jousters = jousters;
        rec->seated = seated;
        rec->seats = seats;
        rec->horses = horses;
        rec->frame = cycle;
        rec->f1c = f1c;
        rec->f20 = f20;
        r = next;
    }

    arena = g_joust_head;
    while (arena) {
        cycle = arena->cycle;
        horses = arena->horses;
        f0c = arena->f0c;
        jousters = arena->jousters;
        f20 = arena->f20;
        f1c = 0;
        if (cycle != 0) {
            if (cycle != 0x20)
                goto run;
            if (!((f0c == 0 || jousters >= 2) && jousters != 0)) {
                if (horses.h[1] != 0)
                    goto run;
                f1c = 1;
            } else {
                if (horses.h[1] != 1) {
                    if (horses.h[1] == 3)
                        f20 = 1;
                    else
                        goto run;
                }
            }
        } else {
            if (!((f0c == 0 || jousters >= 2) && jousters != 0)) {
                if (horses.h[0] != 0)
                    goto run;
                f1c = 1;
            } else {
                if (horses.h[0] != 1) {
                    if (horses.h[0] == 3)
                        f20 = 1;
                    else
                        goto run;
                } else
                    goto stop;
            }
        }
        /* STOP: this horse is not ready to turn over -- hold the cycle where
         * it is, silence the arena and leave the gate that was just set. */
stop:
        if (arena->sample) {
            Joust_FadeSample(arena->tile.b.x, arena->tile.b.y);
            arena->sample = 0;
        }
        goto write;
        /* RUN: the arena is turning -- keep the looping sample playing, step
         * the cycle (wrapping past 0x3f) and clear both gates. */
run:
        if (arena->sample == 0) {
            void* s = Joust_StartSample(arena->tile.b.x, arena->tile.b.y);

            arena->sample = s;
            SetSampleLooping(s);
        }
        cycle++;
        if (cycle > 0x3f)
            cycle = 0;
        f1c = 0;
        f20 = 0;
write:
        arena->horses = horses;
        arena->cycle = cycle;
        arena->f1c = f1c;
        arena->f20 = f20;
        arena = arena->next;
    }
}
