/* LEGOLAND -- BOATING SCHOOL water tiles and boat launch, plus the carousel's
 * per-instance tick that had no home file of its own.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere) and deliberately mirror the ones in joust2.c (BsWater_Probe),
 * ridecb5.c / ridecb6.c (the boating-school placement side), anim2.c (the
 * boat record) and ridecb3.c (the carousel).
 *
 *   addr        what it is                                     state
 *   0x00418e60  BoatingSchool_TryLaunch  put a visitor in a boat   [OK]
 *   0x0042c6d0  Carousel_TickInstance    one carousel, one tick    [OK]
 *   0x0041cb20  BsWater_WalkStep         one ply of the route walk [OK]
 *   0x0041c4c0  BsWater_SetTile          repaint one lake square   WIP 109i, 3 X
 *
 * The first three are boating school, the fourth (0x0042c6d0) is the CAROUSEL
 * and is here only because it had no home file; ridecb3.c declares it.
 *
 * NEW FIELD: BsWater +0x04 is an INT holding the cell's arm mask -- the same
 * four-bit N/E/S/W answer BsWater_Probe (joust2.c) returns.  ridecb5.c and
 * ridecb6.c both have that offset as padding; nothing there reads it, so
 * neither file needs changing.
 *
 * TWO EXTERN TYPES DELIBERATELY DIFFER FROM THEIR NEIGHBOURS (extern
 * prototype types are caller-side codegen levers, so neither file is
 * "aligned"):
 *  - BsWater_SetTile's fourth parameter is a `BPosW*` here, because the
 *    original reads 16 bits through it (`mov dx, word ptr [edx]`).  ridecb5.c
 *    declares the same address with `void*` and ridecb6.c with `int*`.
 *  - 0x0082adf4 is a `TsmRec*` here (roads.c's {LLElem*, unsigned short*}
 *    pair), because the tile lookup indexes it by 8 and reads +0x04.
 *    ridecb5.c/ridecb6.c call the same object `BsTileSet* g_bs_water_tiles`.
 *
 * ========================================================================= */

void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)

/* ---- shared map types (same offsets as joust2.c / ridecb5.c) ------------ */
typedef struct Pos { int x; int y; } Pos;

/* A packed 2-byte map square passed BY VALUE, and its 16-bit view. */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union BPosW { unsigned short w; BPos b; } BPosW;

/* One placed boating school (0x34 bytes, list head 0x004cc074, next @ +0x2c).
 * Only the key and the near jetty's two coordinate bytes are read here. */
typedef struct BsStation {
    BPosW              key;         /* +0x00 the building's map square */
    unsigned char      ax;          /* +0x02 dock A x (route start) */
    unsigned char      ay;          /* +0x03 dock A y */
    unsigned char      bx;          /* +0x04 dock B x (route end) */
    unsigned char      by;          /* +0x05 dock B y */
    unsigned char      pad06[0x2c - 6];
    struct BsStation*  next;        /* +0x2c */
    int                take;        /* +0x30 */
} BsStation;                        /* 0x34 */

/* A visitor; nothing but the pointer identity is used in this file's launch. */
typedef struct Bloke {
    unsigned char  pad00[0x60];
    unsigned char  stage;           /* +0x60 */
} Bloke;

/* One boating-school boat (anim2.c's BsBoat, same offsets and names). */
typedef struct BsWobble { int x; int y; } BsWobble;

typedef struct BsBoat {
    BPosW          key;             /* +0x00  the station that launched it */
    unsigned char  pad02[2];
    int            cx;              /* +0x04  the map square it is on */
    int            cy;              /* +0x08 */
    int            nx;              /* +0x0c  the map square it is heading for */
    int            ny;              /* +0x10 */
    int            sx;              /* +0x14  screen position, this frame */
    int            sy;              /* +0x18 */
    BsWobble       wob[0x50];       /* +0x1c   80 precomputed sub-steps */
    int            frame[0x50];     /* +0x29c  80 precomputed sprite codes */
    int            f3dc;            /* +0x3dc */
    int            f3e0;            /* +0x3e0 */
    int            state;           /* +0x3e4  1..0x10, the mover's state */
    int            leg;             /* +0x3e8  which way this leg turns */
    Bloke*         rider;           /* +0x3ec  the single passenger */
    struct BsBoat* next;            /* +0x3f0 */
} BsBoat;                           /* 0x3f4 */

extern BsStation* g_bs_stations;    /* 0x004cc074 */
extern BsBoat*    g_bs_boats;       /* 0x004cc03c */

extern void* HeapAlloc_w(unsigned int size);                     /* 0x0049e4ff */
extern int   rand(void);                                         /* 0x0049e4b2 (CRT) */

/* =========================================================================
 * 0x00418e60 -- BoatingSchool_TryLaunch.
 *
 * Called from BoatingSchool_Tick (ridecb5.c) stage 1: the visitor at the head
 * of the queue asks its school for a boat.  Returns 1 if one was created.
 *
 * The refusal rules, in order, are all about the SCHOOL'S NEAR JETTY
 * (st->ax / st->ay, the route's start square):
 *   - a boat of this school already SITTING on the jetty square, or
 *   - a boat of this school already HEADING for it, or
 *   - a boat of this school still in state 1 (just launched, not yet away)
 * blocks the launch.  Otherwise a 0x3f4-byte boat record is allocated and
 * pushed on the front of the ride-wide list at 0x004cc03c.
 *
 * TWO ORIGINAL QUIRKS REPRODUCED.
 *  - The station search result is NEVER null-checked.  If a boat carries this
 *    key but no station record does, `st` is NULL when st->ax is read.  In
 *    practice the boat's key always comes from a live station, so it cannot
 *    fire; it is still the original's shape (the search's `st` is dereferenced
 *    inside the boat loop, not guarded).
 *  - The new boat is placed at (key.x - 1, key.y + 5), a FIXED offset from the
 *    building's own square, not at the station's recorded jetty (st->ax/ay)
 *    that the three refusal tests just compared against.  The two agree for
 *    the class's standard footprint, so the bug is latent.
 *  - `f3e0 = rand() & 3` yields 0..3 and 3 is then rewritten to 2, so the
 *    field's four-way choice is really three-way with 2 twice as likely.
 *    That trailing fixup is a separate statement in the original, after the
 *    two buffer fills, not part of the assignment.
 *
 * The two buffer fills are memsets: the 80 rocking offsets (0x1c..0x29c) are
 * filled with the byte 0xf1 -- a poison pattern, not a value: the mover
 * overwrites every entry it uses before the drawer reads it -- and the 80
 * sprite codes (0x29c..0x3dc) are zeroed.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00418e60
int BoatingSchool_TryLaunch(BPosW key, Bloke* b)
{
    BsStation* st = g_bs_stations;
    BsBoat*    bt = g_bs_boats;

    while (st && st->key.w != key.w)
        st = st->next;

    while (bt) {
        if (bt->key.w == key.w) {
            /* ORIGINAL BUG: `st` is not checked for NULL here. */
            if (bt->cx == st->ax && bt->cy == st->ay)
                return 0;
            if (bt->nx == st->ax && bt->ny == st->ay)
                return 0;
            if (bt->state == 1)
                return 0;
        }
        bt = bt->next;
    }

    bt = (BsBoat*)HeapAlloc_w(0x3f4);
    if (!bt)
        return 0;

    bt->next = g_bs_boats;
    bt->key.w = key.w;
    bt->cx = key.b.x - 1;
    bt->cy = key.b.y + 5;
    bt->nx = key.b.x - 1;
    bt->ny = key.b.y + 5;
    bt->f3dc = 1;
    bt->f3e0 = rand() & 3;
    bt->state = 1;
    bt->leg = (rand() & 0xf) + 4;
    bt->rider = b;
    g_bs_boats = bt;
    memset(bt->wob, 0xf1, sizeof(bt->wob));
    memset(bt->frame, 0, sizeof(bt->frame));
    if (bt->f3e0 == 3)
        bt->f3e0 = 2;
    return 1;
}

/* =========================================================================
 * CAROUSEL -- 0x0042c6d0  Carousel_TickInstance
 * =========================================================================
 * One placed carousel, one tick.  Called for every record on the list at
 * 0x006160c4 by Carousel_TickInstances (ridecb3.c 0x0042c800).
 *
 * THE MACHINE IS A THREE-STATE CYCLE held in the record's flag word (+0x0c):
 *
 *   idle       (neither bit)   waiting for the first rider to board
 *   boarding   (bit 0x4000)    the doors are shut to newcomers; it turns as
 *                              soon as `boarded` reaches `visitors`
 *   running    (bit 0x0001)    it turns for `revs` revolutions of 0x40 frames
 *
 * IDLE -> BOARDING is on the record's own timer (+0x1c): once anybody has
 * boarded, the timer counts down one per tick and, at zero, the ride takes
 * the "not letting anyone on" flag on its own map square and latches 0x4000.
 * BOARDING -> RUNNING is Carousel_StartRide.  RUNNING -> IDLE is
 * Carousel_StopRide (0x0042c210), which re-rolls the revolution count to
 * rand() % 2 + 3 and fades the ride's sample out; it only runs once
 * GetAllBlokesOffRide reports everyone clear, and until then this handler
 * RETURNS EARLY -- the rider-positioning loop and the z-sprite frame store
 * below are skipped for a machine that is waiting for its riders to get off.
 *
 * THE ANIMATION is one frame every SECOND tick (+0x14 is the divide-by-two
 * counter), wrapping at 0x40 and spending one revolution each wrap.
 *
 * THE RIDER LOOP walks the CLASS-wide rider list (ObjDef +0xcc), not a
 * per-record one, so every tick every carousel walks every carousel's riders
 * and picks out its own by map square.  A rider whose pose set (+0x35) is 1
 * is being positioned by the machine: its seat number is printed into the
 * shared "BlokeBox??" path name and the .bnv solve places it at the
 * platform's current frame.  Note the path name buffer is a single shared
 * global -- two carousels turning at once overwrite each other's, which is
 * harmless only because the name is consumed inside the call.
 *
 * LEVER (extern prototype TYPE): GetAllBlokesOffRide's second parameter is
 * `unsigned short`, which is what makes the original load the record's square
 * with a 16-bit `mov cx, word ptr [esi+4]` into the register that still holds
 * the just-incremented tick counter and push the dirty upper half.  An `int`
 * parameter would zero-extend and cost the match.  rides.c declares it the
 * same way.
 * ========================================================================= */

typedef struct CBloke {
    unsigned char  pad00[0x35];
    unsigned char  pose;            /* +0x35 which 3D pose set the ride wants */
    unsigned char  seat;            /* +0x36 seat index, ONE-BASED */
} CBloke;

/* A rider slot on the class-wide list at ObjDef +0xcc. */
typedef struct RiderNode {
    struct RiderNode* next;         /* +0x00 */
    struct RiderNode* prev;         /* +0x04 */
    CBloke*           bloke;        /* +0x08 */
    unsigned short    ride_id;      /* +0x0c the packed map square it is using */
    unsigned short    pad0e;
    void*             owner;        /* +0x10 */
} RiderNode;

typedef struct RideObject {
    unsigned char  pad00[0xcc];
    RiderNode*     riders;          /* +0xcc the class-wide rider list */
} RideObject;

/* One placed carousel (ridecb1.c / ridecb3.c, same offsets). */
typedef struct CarouselRec {
    struct CarouselRec* next;       /* +0x00 */
    unsigned short      square;     /* +0x04 packed {x,y} */
    unsigned char       boarded;    /* +0x06 riders that have got on */
    unsigned char       aboard;     /* +0x07 riders still on board */
    signed char         frame;      /* +0x08 animation frame of the platform */
    unsigned char       pad09[3];
    int                 flags;      /* +0x0c 1 = running, 0x4000 = boarding */
    unsigned char       revs;       /* +0x10 revolutions left to turn */
    unsigned char       pad11[3];
    int                 half;       /* +0x14 the divide-by-two frame counter */
    unsigned char       visitors;   /* +0x18 riders this go-round is waiting for */
    unsigned char       pad19[3];
    int                 timer;      /* +0x1c idle -> boarding countdown */
    unsigned char       seat[4];    /* +0x20 one byte per seat, 0 = free */
} CarouselRec;

/* z_Carousel.lls's holder; the frame word is two indirections down. */
typedef struct SpriteObj {
    int     pad0;
    int     pad4;
    short** lls_holder;             /* +0x08 */
} SpriteObj;

extern int  GetAllBlokesOffRide(RideObject* item, unsigned short ride_id); /* 0x0048a390 */
extern void Ride_SetFlagToNotLetAnyoneOn(unsigned short* square);          /* 0x00442fa0 */
extern void Carousel_StopRide(CarouselRec* rec);                           /* 0x0042c210 */
extern void Carousel_StartRide(CarouselRec* rec);                          /* 0x0042bc90 */
extern void SetBlokePositionFromBNV(void* bin, CBloke* b, const char* name,
                                    int frame, float near_z, float far_z,
                                    int flag);                             /* 0x00484a70 */
extern int  sprintf(char* dst, const char* fmt, ...);                      /* 0x0049e573 (CRT) */

extern RideObject* g_carousel_item;    /* 0x006160bc the class payload */
/* screencb2.c's Carousel_Create stores each of the carousel's four "table"
 * resources under a SCALAR name FIRST and then copies it into the class table
 * (screencb2.c:768-770, 767 and the prose at :788): 0x0061608c is the scalar
 * `g_carousel_run_s` and 0x00616090 is `g_carousel_bnv[0]`; 0x006160b8 is the
 * scalar `g_carousel_zspr_s` and 0x006160c0 is `g_carousel_zspr[0]`.  Both
 * were spelled with the TABLE name here until PORT-M15, which made one object
 * out of each scalar/table pair. */
extern void*       g_carousel_run_s;   /* 0x0061608c the carousel .bnv bundle */
extern SpriteObj*  g_carousel_zspr_s;  /* 0x006160b8 z_Carousel.lls */
extern char        g_carousel_path[];  /* 0x004b64cc "BlokeBox??" */
extern char        g_fmt_02d[];        /* 0x004b4704 "%02d" */

// FUNCTION: LEGOLAND 0x0042c6d0
void Carousel_TickInstance(CarouselRec* rec)
{
    RiderNode* r = g_carousel_item->riders;

    if (rec->flags & 1) {
        rec->half++;
        if (rec->revs == 0) {
            if (!GetAllBlokesOffRide(g_carousel_item, rec->square))
                return;
            Carousel_StopRide(rec);
            return;
        }
        if (rec->half >= 2) {
            rec->half = 0;
            rec->frame++;
            if (rec->frame >= 0x40) {
                rec->frame = 0;
                rec->revs--;
            }
        }
    } else if (rec->flags & 0x4000) {
        if (rec->boarded == rec->visitors) {
            rec->flags &= ~0x4000;
            Carousel_StartRide(rec);
            return;
        }
    } else if (rec->boarded != 0) {
        if (rec->timer == 0) {
            rec->flags |= 0x4000;
            Ride_SetFlagToNotLetAnyoneOn(&rec->square);
        } else {
            rec->timer--;
        }
    }

    while (r) {
        if (rec->square == r->ride_id && r->bloke->pose == 1) {
            sprintf(&g_carousel_path[8], g_fmt_02d, r->bloke->seat);
            SetBlokePositionFromBNV(g_carousel_run_s, r->bloke, g_carousel_path,
                                    rec->frame, -1617853.25f, -1618109.0f, 0);
        }
        r = r->next;
    }
    *(short*)*(g_carousel_zspr_s->lls_holder) = rec->frame;
}

/* =========================================================================
 * BOATING SCHOOL -- 0x0041cb20  BsWater_WalkStep
 * =========================================================================
 * ONE PLY of the breadth-first walk that lays a school's boat route, and the
 * lake's copy of DrivingSchool_WalkStep (0x004051a0) over the road grid.
 *
 * The walk state is two globals: g_bs_walk_cur (0x004d8240) is the FRONTIER
 * built by the previous ply, chained through each cell's +0x14, and
 * g_bs_walk_next (0x004d8244) is the frontier this call builds.
 * BoatingSchool_RebuildRoute (ridecb6.c 0x0041caa0) seeds the first with the
 * school's far dock and calls this until it queues nothing.
 *
 * For every cell on the frontier the four cardinal neighbours FIVE squares
 * away are looked up -- north (y-5), east (x+5), south (y+5), west (x-5), in
 * that order, which is the order the four results are then tested -- and a
 * neighbour is taken iff it exists, belongs to the SAME school as the walk
 * (+0x02 == key) and has not been reached yet (+0x18 == 0).  Taking it
 * records where it was reached from (+0x18 = the current cell), its distance
 * from the seed (+0x08 = current + 1), and pushes it on the next frontier.
 *
 * So +0x18 is BOTH the visited mark and the back-pointer, which is how
 * BoatingSchool_RebuildRoute can clear the whole lake's marks with one pass
 * over +0x18 and then read the route back by following it, and why the
 * pushed-first cell ends up LAST on the next frontier (the queue is a stack).
 *
 * The four neighbour lookups share ONE `add esp, 0x20`: VC6 merges the
 * cleanups of consecutive __cdecl calls whose results all land in locals.
 * The four blocks are written out in full rather than looped -- each gets its
 * own callee-saved register (edi/ebx/ebp/eax), which no array or loop over
 * the four results reproduces.
 *
 * The split prologue is the standard one: ebx/ebp/edi are pushed only on the
 * non-empty path, because all three live entirely inside the walk loop.
 * ========================================================================= */

typedef struct BsWater {
    BPosW           pos;            /* +0x00 this cell's own map square */
    BPosW           owner;          /* +0x02 the school that owns it */
    int             mask;           /* +0x04 ... see BsWater_SetTile */
    int             dist;           /* +0x08 steps from the seed dock */
    unsigned char   pad0c[4];
    struct BsWater* next;           /* +0x10 the lake-wide list link */
    struct BsWater* queue;          /* +0x14 the walk frontier link */
    struct BsWater* prev;           /* +0x18 reached from / visited mark */
} BsWater;                          /* 0x1c */

extern BsWater* BsWater_FindAt(int x, int y);                    /* 0x0041c890 */

extern BsWater* g_bs_walk_cur;      /* 0x004d8240 */
extern BsWater* g_bs_walk_next;     /* 0x004d8244 */

// FUNCTION: LEGOLAND 0x0041cb20
void BsWater_WalkStep(BPosW key)
{
    BsWater* cur = g_bs_walk_cur;
    BsWater* n;
    BsWater* e;
    BsWater* s;
    BsWater* w;

    while (cur) {
        n = BsWater_FindAt(cur->pos.b.x, cur->pos.b.y - 5);
        e = BsWater_FindAt(cur->pos.b.x + 5, cur->pos.b.y);
        s = BsWater_FindAt(cur->pos.b.x, cur->pos.b.y + 5);
        w = BsWater_FindAt(cur->pos.b.x - 5, cur->pos.b.y);

        if (n && n->owner.w == key.w && n->prev == 0) {
            n->prev = cur;
            n->dist = cur->dist + 1;
            n->queue = g_bs_walk_next;
            g_bs_walk_next = n;
        }
        if (e && e->owner.w == key.w && e->prev == 0) {
            e->prev = cur;
            e->dist = cur->dist + 1;
            e->queue = g_bs_walk_next;
            g_bs_walk_next = e;
        }
        if (s && s->owner.w == key.w && s->prev == 0) {
            s->prev = cur;
            s->dist = cur->dist + 1;
            s->queue = g_bs_walk_next;
            g_bs_walk_next = s;
        }
        if (w && w->owner.w == key.w && w->prev == 0) {
            w->prev = cur;
            w->dist = cur->dist + 1;
            w->queue = g_bs_walk_next;
            g_bs_walk_next = w;
        }
        cur = cur->queue;
    }
}

/* =========================================================================
 * BOATING SCHOOL -- 0x0041c4c0  BsWater_SetTile
 * =========================================================================
 * Repaints ONE lake square's 5x5 block of map cells for a given arm mask, and
 * creates the lake record for that square if it does not exist yet.  This is
 * the write half of the family whose read half is BsWater_Probe (joust2.c):
 * the mask it is handed is exactly the four-bit "which cardinal neighbours are
 * my school's water" answer Probe returns, and the lake's five-cell lattice is
 * why the block is 5x5 -- one lake square owns the whole 5x5 patch of map
 * cells centred on it.
 *
 * NEW: BsWater +0x04 is an INT holding that arm mask (ridecb5.c/ridecb6.c had
 * it as padding).  It is the only field this routine writes besides the key
 * and the owner, and it is what the drawer needs to pick the water shape
 * again without re-probing.
 *
 * A NEWLY CREATED RECORD IS ONLY PARTLY INITIALISED: +0x18 (the route walk's
 * back-pointer/mark) is cleared and +0x10 is linked, but +0x08 (the route
 * distance) and +0x14 (the walk frontier link) are left as heap garbage --
 * BoatingSchool_RebuildRoute clears +0x18 lake-wide and seeds +0x08/+0x14 on
 * the start cell only, so a cell that is never reached by a walk carries junk
 * in both.  Original; not fixed.
 *
 * ORIGINAL BUG reproduced: the 5x5 fill does NOT null-check the map cell.
 * MapCellAt returns 0 for a square off the edge of the map and the very next
 * instruction stores into it, so a lake square placed within two cells of the
 * map border faults.  The bounds test is compiled (it is the standard
 * x >= 0, x < width, y >= 0, y < height fetch every accessor open-codes) and
 * its result is then thrown away.
 *
 * ORIGINAL DEAD CODE reproduced: the tile lookup is
 * `*g_bs_water_tsm[t >> 8].loaded + (t & 0xff)` copied from the road layer
 * (roads.c 0x00412a20), where `t` is an `unsigned short` tile word.  Here the
 * shape table is a table of BYTES, so `t >> 8` is always 0 and only the first
 * TSM record is ever consulted -- but VC6 still emits the shift
 * (`mov edx,eax / and edx,0xff / shr edx,8`) because it does not range-narrow
 * the promoted byte.  Those three instructions are the fingerprint of the
 * copied expression and are load-bearing for the match.
 *
 * TWO ARGUMENT SLOTS ARE REUSED AS LOCALS, which is why the function needs
 * only 8 bytes of frame for a 2-byte key and one compiler temp: `mask`'s slot
 * holds the outer row counter (0..4) and `owner`'s slot holds `x - 2`, both
 * written after the last read of the argument.
 *
 * EXTERN TYPE NOTE: `owner` is a `BPosW*` here (the original reads a 16-bit
 * value through it, `mov dx, word ptr [edx]`).  ridecb5.c declares the same
 * address with `void* owner` and ridecb6.c with `int* owner`; both are
 * caller-side choices and neither is changed.
 * ========================================================================= */

typedef struct Cell {
    void*          obj;             /* +0x00 */
    unsigned short key;             /* +0x04 the object's map square */
    unsigned char  pad06[2];
    unsigned short tile;            /* +0x08 */
    unsigned char  pad0a[2];
    unsigned short flags;           /* +0x0c */
    unsigned char  pad0e[2];
    unsigned char  rf;              /* +0x10 */
    unsigned char  pad11[3];
} Cell;                             /* 0x14 */

typedef struct MapHdr {
    unsigned char  pad00[0x14];
    unsigned short width;           /* +0x14 */
    unsigned short height;          /* +0x16 */
} MapHdr;

typedef struct ObjDef {
    unsigned char pad00[0xc4];
    void*         c4;               /* +0xc4 the class's shared instance */
} ObjDef;

/* LLIDB_LoadTSMData's 8-byte {LLElem* entry, unsigned short* loaded} records;
 * the loaded TSF descriptor's first word is the tileset's base slot. */
typedef struct TsmRec { void* elem; unsigned short* loaded; } TsmRec;

extern MapHdr*       g_map;              /* 0x004bcbf4 (lpConfig) */
extern Cell**        g_map_rows;         /* 0x00801400 (GameMap) */
extern ObjDef*       g_bs_water_cls;     /* 0x0082adf0 BOATING SCHOOL WATER */
extern TsmRec*       g_bs_water_tsm;     /* 0x0082adf4 */
extern BsWater*      g_bs_water;         /* 0x004d823c the lake record list */
extern int           g_bg_full_update;   /* 0x004b9220 (BGFullUpdate) */
/* 16 masks x 25 cells of the 5x5 block; the entries are tile codes. */
extern unsigned char g_bs_water_shapes[][25];                    /* 0x004b53d4 */

extern void  SetMapTile(int x, int y, unsigned short tile);      /* 0x00461780 */

/* The bounds-checked cell fetch every map accessor open-codes. */
static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

/* Scope F close (2026-09-05): 109/109 instructions, 350/350 bytes,
 * strict/rb/ob 0/0/0. Compute both SetMapTile coordinates in a local pair
 * AFTER the map-cell stores and BEFORE evaluating the tile expression,
 * then pass the pair's fields through the existing scalar call. This puts
 * the shape-pointer increment after the x/counter updates in the latch.
 * A by-value pair call also fixes that latch but swaps the two row reloads;
 * the existing scalar call closes those final two instructions. The pair
 * adds no storage and preserves both reused argument homes, all bounds
 * checks, and the original byte-table shift. Whole-file audit keeps the
 * three existing exact bodies and adds this one. Earlier scheduling-floor
 * claims above are superseded; see docs/lanes/scope-f.md for evidence. */
// FUNCTION: LEGOLAND 0x0041c4c0
void BsWater_SetTile(int x, int y, int mask, BPosW* owner)
{
    typedef struct PaintPos { int x; int y; } PaintPos;
    PaintPos paint;
    BPosW    key;
    BsWater* w;
    int      r;
    int      c;

    key.b.x = (unsigned char)x;
    key.b.y = (unsigned char)y;
    w = BsWater_FindAt(x, y);
    if (w == 0) {
        w = (BsWater*)HeapAlloc_w(0x1c);
        if (w == 0)
            return;
        w->next = g_bs_water;
        w->prev = 0;
        g_bs_water = w;
    }
    w->pos.w = key.w;
    w->mask = mask;
    if (owner != 0)
        w->owner.w = owner->w;

    g_bg_full_update = 1;
    for (r = 0; r < 5; r++) {
        for (c = 0; c < 5; c++) {
            /* ORIGINAL BUG: no null check on the cell. */
            Cell* cell = MapCellAt(x - 2 + c, y - 2 + r);

            cell->flags = 8;
            cell->rf = 2;
            cell->obj = g_bs_water_cls->c4;
            cell->key = key.w;
            /* Materialize the two paint coordinates before the tile lookup. */
            paint.x = x - 2 + c;
            paint.y = y - 2 + r;
            SetMapTile(paint.x, paint.y,
                       (unsigned short)(*g_bs_water_tsm[g_bs_water_shapes[mask][r * 5 + c] >> 8].loaded
                                        + (g_bs_water_shapes[mask][r * 5 + c] & 0xff)));
        }
    }
}
