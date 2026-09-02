/* LEGOLAND -- the JUNGLE CRUISE ride subsystem: the river, the boats and the
 * decorations that hang off them.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  None of
 * these functions is exported; every extent was taken from the disassembly by
 * control flow (tools/audit.py).  Struct field OFFSETS, record sizes and
 * global addresses are load-bearing; the names are ours.  Types are declared
 * LOCALLY on purpose (legoland.h is owned elsewhere) and mirror the ones in
 * ridecb2.c, which owns the ride's five class callbacks.
 *
 * =========================================================================
 * HOW THE RIVER IS LAID OUT
 *
 * The jungle cruise is the only ride in the game with a player-drawn track.
 * The track is made of WATER SQUARES: each one is a record on g_jc_water
 * (0x0062fd2c, 0x1c bytes) and each one covers a 5x5 BLOCK of map cells whose
 * centre cell is the square's map coordinate.  That is why every coordinate
 * in this file steps by FIVE -- the four neighbours of (x, y) are
 *
 *              (x, y-5)  north = link bit 1
 *              (x+5, y)  east  = link bit 2
 *              (x, y+5)  south = link bit 4
 *              (x-5, y)  west  = link bit 8
 *
 * and the bitmap of the ones that exist lives in the square's +0x04.  Laying
 * or removing a square runs the same three steps for it and for each of its
 * neighbours:
 *
 *   JungleCruise_ProbeRiver(x, y, &owner)   -> which neighbours are there,
 *                                              and who owns the river
 *   JungleCruise_UpdateRiverTile(x,y,mask,&owner)
 *                                           -> repaint the 5x5 block from
 *                                              the 16-row tile table
 *   JungleCruise_RelinkRiverCell(x,y,&owner)-> fill the 2x2 inside corners
 *                                              between diagonal neighbours
 *
 * A square only links to a neighbour that belongs to the SAME station, which
 * is what keeps two cruises' rivers from merging when they are laid side by
 * side.  A station is not itself a water square, but its route START square
 * (JcStation +0x02, always its own x and y+5) reads as a NORTH link and its
 * route END square (+0x04) as a SOUTH link, so the river joins the building
 * at both ends.
 *
 * =========================================================================
 * HOW A BOAT TRAVERSES IT
 *
 * A route is laid once, by JungleCruise_BuildRoute: it clears the visit mark
 * on every square, looks up the start square to learn the owner, and hands
 * the walk to the recursive tracer at 0x00437260, which follows the link bits
 * from square to square.  JungleCruise_RebuildRoute re-runs the breadth-first
 * link pass through the two-slot work list at 0x0062fd30/0x0062fd34 whenever
 * the track changes.
 *
 * A boat (g_jc_boats, 0x00616164, 0x3f8 bytes) is NOT a map object: the
 * station allocates one when a party of up to three visitors is ready
 * (JungleCruise_TryLaunchBoat) and drops it on the route start square.  From
 * there the boat does not move a little each frame.  Instead the mover
 * precomputes the WHOLE crossing of one river square -- 80 sub-steps of
 * rocking offset (+0x1c) and sprite code (+0x29c) -- and the renderer plays
 * that buffer back.  g_jc_anim_tick (0x00629c54) is the shared playback
 * cursor; when it reaches 0x50 JungleCruise_Tick zeroes it and calls
 * JungleCruise_AdvanceBoats, which commits each boat's target square as its
 * current one and dispatches on the mover state at +0x3e0:
 *
 *      1  just launched   -> JcBoat_Depart  (aim five cells south, state 4)
 *      4  running         -> JcBoat_Step(b, 0)
 *      8  running         -> JcBoat_Step(b, 1)
 *   0x10  end of a leg    -> JcBoat_Advance (frees the boat when it is done,
 *                            returning the NEXT one)
 *
 * JungleCruise_UpdateRiverAnim draws them, interleaving the three riders with
 * the boat's three sprite layers by depth, and puts the party ashore on the
 * very last sub-step of the very last leg.
 *
 * =========================================================================
 * WHAT IS IN THIS FILE
 *
 *   0x004371b0  JcWater_FindAt                  [OK]   the square at (x, y)
 *   0x004332c0  JungleCruise_CountStationBoats  [OK]   (with its '=' bug)
 *   0x00432cb0  JcBoat_Unlink                   [OK]
 *   0x004371e0  JungleCruise_BuildRoute         [OK]
 *   0x004373c0  JungleCruise_RebuildRoute       [OK]
 *   0x00433fc0  MonkeyTree_Remove               [OK]   MONKEY TREE cb_remove
 *   0x00436f30  JcWater_RemoveOne               [OK]
 *   0x00434670  MonkeyFish_Remove               [OK]   MONKEY FISH cb_remove
 *   0x00434b40  JcDeco_Remove                   [OK]
 *   0x00432b90  JungleCruise_TryLaunchBoat      [OK]
 *   0x004332f0  JungleCruise_AdvanceBoats       [OK]
 *   0x00436dc0  JungleCruise_UpdateRiverTile    [WIP]  116/116 insns
 *   0x00436fb0  JungleCruise_ProbeRiver         [OK]
 *   0x004367b0  JungleCruise_RelinkRiverCell    [OK]
 *   0x00432d00  JungleCruise_UpdateRiverAnim    [WIP]  422/422 insns
 * ========================================================================= */

/* ---- shared map/cursor types (same offsets as objmap2.c / ridecb2.c) ---- */
typedef struct Pos { int x; int y; } Pos;

typedef struct Rect {
    int left;                       /* +0x00 */
    int top;                        /* +0x04 */
    int right;                      /* +0x08 */
    int bottom;                     /* +0x0c */
    struct Rect* next;              /* +0x10 */
} Rect;

/* A packed 2-byte map square passed BY VALUE, and its 16-bit view. */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union BPosW { unsigned short w; BPos b; } BPosW;

/* ---- the ride's record lists -------------------------------------------
 * Every record starts with its own map square, and the child records carry
 * the owning station's map square at +0x02.
 * ------------------------------------------------------------------------ */

/* One JUNGLE CRUISE station (the ride building itself). */
typedef struct JcStation {
    BPosW          pos;             /* +0x00  the station's own map square */
    BPosW          a;               /* +0x02  route START square (pos.x, pos.y+5) */
    BPosW          b;               /* +0x04  route END square */
    unsigned char  pad06[2];
    void*          route;           /* +0x08  the laid boat route */
    unsigned char  pad0c[0x14 - 0x0c];
    int            count;           /* +0x14  visitors in the queue */
    void*          blokes[5];       /* +0x18  the five queue slots */
    int            timer;           /* +0x2c  ticks until the next dispatch */
    void*          riders[3];       /* +0x30  the party waiting to board */
    struct JcStation* next;         /* +0x3c */
    int            take;            /* +0x40  accumulated income */
} JcStation;                        /* 0x44 */

/* One WATER square of the river.  The class is "JUNGLE CRUISE WATER" and
 * each record covers a 5x5 block of map cells, so the whole subsystem steps
 * in units of FIVE: the neighbours of (x,y) are (x,y-5) (x+5,y) (x,y+5)
 * (x-5,y), which is what `links` records. */
typedef struct JcWater {
    BPosW          pos;             /* +0x00  this square */
    BPosW          owner;           /* +0x02  the station that owns the river */
    int            links;           /* +0x04  1 N, 2 E, 4 S, 8 W */
    struct JcWater* rlink;          /* +0x08  route: the previous square */
    int            mark;            /* +0x0c  route walk: already visited */
    struct JcWater* next;           /* +0x10  list link */
    int            f14;             /* +0x14  route: step index */
    void*          f18;             /* +0x18  route: back-pointer */
} JcWater;                          /* 0x1c */

/* THE BOAT ANIMATION BUFFER.  A boat does not move a little every frame: the
 * mover computes the WHOLE 80-frame crossing of one river square in one go
 * and the renderer just plays it back.  g_jc_anim_tick (0..0x4f) is the
 * playback cursor, shared by every boat in the park; when it wraps,
 * JungleCruise_AdvanceBoats steps each boat to the next square and refills
 * the buffers.  The wobble pair is a rocking offset in 1/512ths of a tile,
 * applied in ISOMETRIC axes (x-y across, x+y down); the frame code is an
 * index into the boat image list, its low nibble being the heading. */
typedef struct JcWobble { int x; int y; } JcWobble;

/* One boat.  Boats are not map objects -- the ride allocates them and they
 * ride the river squares. */
typedef struct JcBoat {
    BPosW          key;             /* +0x00  the station that launched it */
    unsigned char  pad02[2];
    int            cx;              /* +0x04  the map square it is on */
    int            cy;              /* +0x08 */
    int            nx;              /* +0x0c  the map square it is heading for */
    int            ny;              /* +0x10 */
    int            sx;              /* +0x14  screen position, this frame */
    int            sy;              /* +0x18 */
    JcWobble       wob[0x50];       /* +0x1c   80 precomputed sub-steps */
    int            frame[0x50];     /* +0x29c  80 precomputed sprite codes */
    int            f3dc;            /* +0x3dc */
    int            state;           /* +0x3e0  1..0x10, the mover's state */
    int            leg;             /* +0x3e4  which way this leg turns (0..3) */
    void*          riders[3];       /* +0x3e8  the party on board */
    struct JcBoat* next;            /* +0x3f4 */
} JcBoat;                           /* 0x3f8 */

extern JcStation* g_jc_stations;    /* 0x00629c3c */
extern JcWater*   g_jc_water;       /* 0x0062fd2c */
extern JcBoat*    g_jc_boats;       /* 0x00616164 */

/* ---- other subsystems --------------------------------------------------- */
extern void* HeapAlloc_w(unsigned int size);                 /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                            /* 0x0049e4d0 */
extern int   rand(void);                                     /* 0x0049e4b2 (CRT) */

/* =========================================================================
 * 0x004371b0 -- JcWater_FindAt: the river square covering a map square.
 *
 * The two int arguments are narrowed to bytes and packed into the FIRST
 * argument slot (VC6 homes the packed key in the dead parameter), then the
 * water list is walked comparing the pair as one 16-bit value.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004371b0
JcWater* JcWater_FindAt(int x, int y)
{
    BPosW key;
    JcWater* w;

    key.b.x = (unsigned char)x;
    w = g_jc_water;
    key.b.y = (unsigned char)y;
    while (w && w->pos.w != key.w)
        w = w->next;
    return w;
}

/* =========================================================================
 * 0x004332c0 -- JungleCruise_CountStationBoats.
 *
 * ORIGINAL BUG, reproduced verbatim: the test was written with a single '='.
 * What was meant is "how many boats belong to this station"; what the code
 * does is STAMP this station's map square onto every boat in the game and
 * then count them all (or none, when the station's square is 0).  The result
 * is the divisor JungleCruise_Tick compares the station's takings against, so
 * a park with two cruises miscounts -- and every boat silently changes owner
 * once per frame.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004332c0
int JungleCruise_CountStationBoats(JcStation* st)
{
    JcBoat* b = g_jc_boats;
    int n = 0;

    while (b) {
        if ((b->key.w = st->pos.w) != 0)
            n++;
        b = b->next;
    }
    return n;
}

/* =========================================================================
 * 0x00432cb0 -- JcBoat_Unlink: take one boat out of the list and free it.
 *
 * The walk compares against the target FIRST, so an empty boat list faults on
 * the very first `b->next` -- reproduced (the callers only reach it with a
 * boat they have just found in the list).
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00432cb0
void JcBoat_Unlink(JcBoat* boat)
{
    JcBoat* prev = 0;
    JcBoat* b = g_jc_boats;

    while (b != boat) {
        prev = b;
        b = b->next;
        if (b == 0)
            return;
    }
    if (b == 0)
        return;
    if (prev != 0)
        prev->next = b->next;
    else
        g_jc_boats = b->next;
    HeapFree_w(boat);
}

/* Recursive river-route tracer (0x00437260): walks out of (x,y) through the
 * `links` bits, following only squares that belong to *owner and have not
 * been marked, and hands the finished route back through *route. */
extern void JungleCruise_TraceRoute(int x, int y, int tx, int ty, BPosW* owner, void** route); /* 0x00437260 */
/* One step of the breadth-first re-link pass (0x00437440); it consumes
 * g_jc_route_cur and leaves the next square in g_jc_route_next. */
extern void JungleCruise_StepRoute(int id);                  /* 0x00437440 */

extern JcWater* g_jc_route_cur;     /* 0x0062fd30 */
extern JcWater* g_jc_route_next;    /* 0x0062fd34 */

/* =========================================================================
 * 0x004371e0 -- JungleCruise_BuildRoute: lay a boat route from (x0,y0) to
 * (x1,y1).
 *
 * Clears the visit mark on every river square, looks the start square up to
 * learn which station owns the river, and hands the walk to the recursive
 * tracer.  Returns 0 when (x0,y0) is not river at all.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004371e0
void* JungleCruise_BuildRoute(int x0, int y0, int x1, int y1)
{
    void*  route = 0;
    BPosW  owner;
    JcWater* w = g_jc_water;

    while (w) {
        w->mark = 0;
        w = w->next;
    }
    w = JcWater_FindAt(x0, y0);
    if (!w)
        return 0;
    owner = w->owner;
    JungleCruise_TraceRoute(x0, y0, x1, y1, &owner, &route);
    return route;
}

/* =========================================================================
 * 0x004373c0 -- JungleCruise_RebuildRoute: re-run the river's link pass for
 * one station.
 *
 * Drops the back-pointer of every square the station owns, finds the
 * station, and re-walks the river from its route END square (+0x04/+0x05)
 * with the little two-slot work list at 0x0062fd30/0x0062fd34.
 *
 * ORIGINAL BUG reproduced: the station search may fall off the end of the
 * list and the code dereferences the null cursor for `bx`/`by`; likewise the
 * square it finds is used without a null check.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004373c0
void JungleCruise_RebuildRoute(int id)
{
    JcWater*   w  = g_jc_water;
    JcStation* st = g_jc_stations;
    JcWater*   start;

    while (w) {
        if (w->owner.w == (unsigned short)id)
            w->f18 = 0;
        w = w->next;
    }
    while (st && st->pos.w != (unsigned short)id)
        st = st->next;
    start = JcWater_FindAt(st->b.b.x, st->b.b.y);
    start->rlink = 0;
    start->f14 = 0;
    g_jc_route_cur = start;
    g_jc_route_next = 0;
    do {
        JungleCruise_StepRoute(id);
        g_jc_route_cur = g_jc_route_next;
        g_jc_route_next = 0;
    } while (g_jc_route_cur);
}

/* ---- the ride's decoration lists ---------------------------------------
 * All three share the {own square, owning station's square, next} head; only
 * the offset of `next` and the value each one is worth differ. */
typedef struct JcMonkeyFish {
    BPosW          pos;             /* +0x00 */
    BPosW          owner;           /* +0x02 */
    unsigned char  pad04[4];
    struct JcMonkeyFish* next;      /* +0x08 */
} JcMonkeyFish;                     /* 0x0c */

typedef struct JcMonkeyTree {
    BPosW          pos;             /* +0x00 */
    BPosW          owner;           /* +0x02 */
    struct JcMonkeyTree* next;      /* +0x04 */
} JcMonkeyTree;                     /* 0x08 */

typedef struct JcDeco {
    BPosW          pos;             /* +0x00 */
    BPosW          owner;           /* +0x02 */
    struct JcDeco* next;            /* +0x04 */
} JcDeco;                           /* 0x08 */

extern JcMonkeyFish* g_jc_fish;     /* 0x00629c30 */
extern JcMonkeyTree* g_jc_trees;    /* 0x00629c2c */
extern JcDeco*       g_jc_deco;     /* 0x00629c34 */

/* An object class descriptor (the 0xd0-byte ODF record); only the footprint
 * rect and the class origin matter in this file. */
typedef struct ObjDef {
    unsigned char pad00[0x0c];
    int           dx;               /* +0x0c  class origin offset */
    int           dy;               /* +0x10 */
    unsigned char pad14[0x3c - 0x14];
    Rect          rect;             /* +0x3c  footprint, INCLUSIVE both ways */
    unsigned char pad50[0xc4 - 0x50];
    void*         c4;               /* +0xc4  the class's shared instance */
    unsigned char padc8[0xd0 - 0xc8];
} ObjDef;

extern ObjDef* g_jc_fish_cls;       /* 0x0081cb74 */

/* A placed map object; only +0x0c (the class) is touched here. */
typedef struct MapObj {
    unsigned char pad00[0x0c];
    ObjDef*       cls;              /* +0x0c */
    unsigned char pad10[4];
} MapObj;

/* The edit / destroy cursor (objmap2.c's Cursor). */
typedef struct Cursor {
    unsigned char pad0000[0x1404];
    Pos           origin;           /* +0x1404 map cell the footprint hangs off */
    int           status;           /* +0x140c */
    int           error;            /* +0x1410 */
    Rect          rect;             /* +0x1414 */
    unsigned char pad1428[0x1834 - 0x1428];
} Cursor;                           /* 0x1834 */

extern void StandardRemoveObject(MapObj* obj, BPosW bp, Cursor* ctx); /* 0x0045f220 */
extern void RestoreBaseMap(int x, int y);                    /* 0x0045da60 */

/* Adds `delta` to the value (+0x40) of the station whose map square is `id`.
 * Every piece of the ride is worth something: a water square or a monkey
 * tree 1, a monkey fish 2.  The JUNGLE CRUISE cb_c0 (0x00436160) reports the
 * largest station value in the park. */
extern void JungleCruise_AddValue(BPosW id, int delta);      /* 0x00436130 */

/* =========================================================================
 * 0x00433fc0 -- MonkeyTree_Remove (JUNGLE CRUISE MONKEY TREE cb_remove).
 *
 * Standard object removal, then take the tree out of its list, refund one
 * point of the owning station's value and free the record.  The list walk
 * compares before testing for the end, so an EMPTY tree list faults -- the
 * handler is only ever reached for a tree that is in the list.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00433fc0
void MonkeyTree_Remove(MapObj* obj, BPosW bp, Cursor* ctx)
{
    JcMonkeyTree* prev = 0;
    JcMonkeyTree* t = g_jc_trees;

    StandardRemoveObject(obj, bp, ctx);
    while (t->pos.w != bp.w) {
        prev = t;
        t = t->next;
        if (t == 0)
            return;
    }
    if (t == 0)
        return;
    JungleCruise_AddValue(t->owner, -1);
    if (prev != 0)
        prev->next = t->next;
    else
        g_jc_trees = t->next;
    HeapFree_w(t);
}

/* =========================================================================
 * 0x00436f30 -- JcWater_RemoveOne: the same teardown for one river square.
 *
 * Called from the JUNGLE CRUISE WATER cb_remove (0x00436a40 in ridecb2.c)
 * once it has decided the square really is river.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00436f30
void JcWater_RemoveOne(MapObj* obj, BPosW bp, Cursor* ctx)
{
    JcWater* prev = 0;
    JcWater* w = g_jc_water;

    StandardRemoveObject(obj, bp, ctx);
    while (w->pos.w != bp.w) {
        prev = w;
        w = w->next;
        if (w == 0)
            return;
    }
    if (w == 0)
        return;
    JungleCruise_AddValue(w->owner, -1);
    if (prev != 0)
        prev->next = w->next;
    else
        g_jc_water = w->next;
    HeapFree_w(w);
}

/* =========================================================================
 * 0x00434670 -- MonkeyFish_Remove (JUNGLE CRUISE MONKEY FISH cb_remove).
 *
 * The fish is more than one map cell, so before the standard removal every
 * cell of the class footprint (rect at ObjDef +0x3c, INCLUSIVE on all four
 * sides) is restored to base terrain, offset by the cursor origin.  The
 * class pointer is re-read from the global inside the loop -- RestoreBaseMap
 * may move the object tables.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00434670
void MonkeyFish_Remove(MapObj* obj, BPosW bp, Cursor* ctx)
{
    JcMonkeyFish* prev = 0;
    JcMonkeyFish* f = g_jc_fish;
    int x;
    int y;

    for (y = g_jc_fish_cls->rect.top; y <= g_jc_fish_cls->rect.bottom; y++) {
        for (x = g_jc_fish_cls->rect.left; x <= g_jc_fish_cls->rect.right; x++)
            RestoreBaseMap(ctx->origin.x + x, ctx->origin.y + y);
    }
    StandardRemoveObject(obj, bp, ctx);
    while (f->pos.w != bp.w) {
        prev = f;
        f = f->next;
        if (f == 0)
            return;
    }
    if (f == 0)
        return;
    JungleCruise_AddValue(f->owner, -2);
    if (prev != 0)
        prev->next = f->next;
    else
        g_jc_fish = f->next;
    HeapFree_w(f);
}

/* ---- the map grid (same layout as objmap2.c / ridecb2.c) ---------------- */
typedef struct Cell {
    void*          obj;             /* +0x00  the object descriptor on the cell */
    unsigned short key;             /* +0x04  the map square that object hangs off */
    unsigned char  pad06[0x0c - 0x06];
    unsigned short flags;           /* +0x0c  map flags */
    unsigned char  pad0e[0x10 - 0x0e];
    unsigned char  rf;              /* +0x10  RF / path flags */
    unsigned char  pad11[0x14 - 0x11];
} Cell;                             /* 0x14 */

typedef struct MapHdr {
    unsigned char  pad00[0x14];
    unsigned short width;           /* +0x14 */
    unsigned short height;          /* +0x16 */
    unsigned char  pad18[0x20 - 0x18];
    unsigned short origin_x;        /* +0x20  screen origin of cell (0,0) */
    unsigned short origin_y;        /* +0x22 */
} MapHdr;

extern MapHdr* g_map;               /* 0x004bcbf4 (lpConfig) */
extern Cell**  g_map_rows;          /* 0x00801400 (GameMap) */

/* The bounds-checked cell fetch every map accessor open-codes; the callers
 * below all dereference the result WITHOUT a null check, exactly as the
 * original does. */
static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

/* =========================================================================
 * 0x00434b40 -- JcDeco_Remove: the remove handler of the fourth jungle
 * cruise decoration class (ObjDef at 0x0081cb64), the one the savegame never
 * writes.
 *
 * It stamps the cursor footprint to the one-cell-wide, three-cell-tall strip
 * {0,-1,0,1} before the standard removal, then clears map flag 0x40 on the
 * three cells SIX squares to the left of the cursor origin (the decoration
 * hangs off the left of its own cell), and finally unlinks and frees the
 * record.  No value is refunded -- this class is not part of the ride's
 * score.  Off-map cells reach `and word [0+0xc]` and fault: original.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00434b40
void JcDeco_Remove(MapObj* obj, BPosW bp, Cursor* ctx)
{
    JcDeco* prev = 0;
    JcDeco* d = g_jc_deco;
    int x;
    int y;

    ctx->rect.top = -1;
    ctx->rect.bottom = 1;
    ctx->rect.left = 0;
    ctx->rect.right = 0;
    StandardRemoveObject(obj, bp, ctx);
    x = ctx->origin.x - 6;
    y = ctx->origin.y;
    MapCellAt(x, y)->flags &= (unsigned short)~0x40;
    y--;
    MapCellAt(x, y)->flags &= (unsigned short)~0x40;
    y += 2;
    MapCellAt(x, y)->flags &= (unsigned short)~0x40;
    while (d->pos.w != bp.w) {
        prev = d;
        d = d->next;
        if (d == 0)
            return;
    }
    if (d == 0)
        return;
    if (prev != 0)
        prev->next = d->next;
    else
        g_jc_deco = d->next;
    HeapFree_w(d);
}

#pragma intrinsic(memset)
extern void* memset(void* d, int c, unsigned int n);

/* =========================================================================
 * 0x00432b90 -- JungleCruise_TryLaunchBoat: put a party of up to three
 * riders on the water at the station whose map square is `key`.
 *
 * A boat may only leave when nothing is already sitting on, or heading for,
 * the station's route START square (JcStation +0x02/+0x03) and no other boat
 * of the ride is still in state 1 (just launched).  The new boat is dropped
 * on (key.x, key.y + 5) -- five map cells south of the station, the first
 * river square -- pointed at the same square, given state 1 and a per-boat
 * leg code of (rand() & 0xf) + 4, and pushed on the head of the boat list.
 *
 * The two animation buffers are initialised with rep stosd: the 80 wobble
 * pairs (+0x1c..+0x29b, 0xa0 dwords) to 0xf1f1f1f1 and the 80 frame codes
 * (+0x29c..+0x3db, 0x50 dwords) to zero.
 *
 * ORIGINAL BUG reproduced: the station lookup may end with a NULL cursor and
 * the boat scan then reads st->a.b.x/st->a.b.y through it.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00432b90
int JungleCruise_TryLaunchBoat(BPosW key, void* r0, void* r1, void* r2)
{
    JcStation* st = g_jc_stations;
    JcBoat* b = g_jc_boats;
    JcBoat* nb;

    while (st && st->pos.w != key.w)
        st = st->next;
    while (b) {
        if (b->key.w == key.w) {
            if (b->cx == st->a.b.x && b->cy == st->a.b.y)
                return 0;
            if (b->nx == st->a.b.x && b->ny == st->a.b.y)
                return 0;
            if (b->state == 1)
                return 0;
        }
        b = b->next;
    }
    nb = (JcBoat*)HeapAlloc_w(sizeof(JcBoat));
    if (!nb)
        return 0;
    nb->next = g_jc_boats;
    nb->key.w = key.w;
    nb->cx = key.b.x;
    nb->cy = key.b.y + 5;
    nb->nx = key.b.x;
    nb->ny = key.b.y + 5;
    nb->f3dc = 1;
    nb->state = 1;
    nb->leg = (rand() & 0xf) + 4;
    nb->riders[0] = r0;
    nb->riders[1] = r1;
    nb->riders[2] = r2;
    g_jc_boats = nb;
    memset(nb->wob, 0xf1, sizeof(nb->wob));
    memset(nb->frame, 0, sizeof(nb->frame));
    return 1;
}

/* The three per-state boat movers (all in the 0x00433xxx block).
 *  - JcBoat_Depart  (0x004333b0) pushes the boat off the station: it animates
 *    it with the shared mover 0x00433840, aims it five cells further south
 *    and moves it on to state 4.
 *  - JcBoat_Step    (0x004334c0) is the ordinary along-the-river step; the
 *    second argument picks which of the two turn tables to use.
 *  - JcBoat_Advance (0x004333e0) finishes a leg.  When the boat's mode word
 *    (+0x3e4) has reached 0 it unlinks and FREES the boat and returns the
 *    NEXT one, which is why the caller has to compare the pointer it gets
 *    back before stepping the list on. */
extern void    JcBoat_Depart(JcBoat* b);                     /* 0x004333b0 */
extern void    JcBoat_Step(JcBoat* b, int table);            /* 0x004334c0 */
extern JcBoat* JcBoat_Advance(JcBoat* b);                    /* 0x004333e0 */

/* =========================================================================
 * 0x004332f0 -- JungleCruise_AdvanceBoats: step every boat in the park one
 * place along its river.  JungleCruise_Tick calls this once every 0x50
 * frames.
 *
 * Each boat first commits the square it was heading for (+0x0c/+0x10) as the
 * square it is on (+0x04/+0x08), then its state word (+0x3e0) is dispatched:
 *
 *      1  just launched          -> JcBoat_Depart
 *      4  running                -> JcBoat_Step(b, 0)
 *      8  running the other way  -> JcBoat_Step(b, 1)
 *   0x10  end of a leg           -> JcBoat_Advance
 *   else  nothing
 *
 * State 0x10 is tested BEFORE the switch, so the switch's own `case 16` (the
 * fourth entry of the jump table at 0x0043338c) is dead code -- kept because
 * it is what makes VC6 emit that table.  When JcBoat_Advance hands back a
 * DIFFERENT boat the current one has been freed and the successor is already
 * in hand, so the list step is skipped.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004332f0
void JungleCruise_AdvanceBoats(void)
{
    JcBoat* b = g_jc_boats;

    while (b) {
        b->cx = b->nx;
        b->cy = b->ny;
        if (b->state == 0x10) {
            JcBoat* was = b;

            b = JcBoat_Advance(b);
            if (b != was)
                goto again;
        } else {
            switch (b->state) {
            case 1:
                JcBoat_Depart(b);
                break;
            case 4:
                JcBoat_Step(b, 0);
                break;
            case 8:
                JcBoat_Step(b, 1);
                break;
            case 16:
                b = JcBoat_Advance(b);
                break;
            }
        }
        if (b == 0)
            break;
        b = b->next;
again:
        ;
    }
}

extern void SetMapTile(int x, int y, unsigned short tile);   /* 0x00461780 */

extern ObjDef* g_jc_water_cls;      /* 0x0081cb54  JUNGLE CRUISE WATER */
/* The water class's TSM record array (LLIDB_LoadTSMData's 8-byte
 * `{LLElem* entry, void* loaded}` records); the loaded TSF descriptor's
 * first word is the tileset's base slot. */
extern void* g_jc_tsm;              /* 0x0081cb58 */
extern int   g_bg_full_update;      /* 0x004b9220 (BGFullUpdate) */

/* 16 river shapes (one per link mask) x the 5x5 block of map cells a river
 * square covers, as tile codes relative to the water tileset. */
extern unsigned char g_jc_river_tiles[16][25];               /* 0x004b72e4 */

/* =========================================================================
 * 0x00436dc0 -- JungleCruise_UpdateRiverTile: (re)build one river square.
 *
 * THE RIVER LAYOUT.  A river square is a 5x5 block of map cells whose CENTRE
 * cell is the square's map coordinate, so neighbouring squares are five
 * cells apart -- every +-5 in this subsystem is one river step.  The record
 * (JcWater, 0x1c bytes) is created on demand and pushed on g_jc_water; a new
 * square is worth one point of the owning station's value.
 *
 * `mask` is the link bitmap from JungleCruise_ProbeRiver (1 N, 2 E, 4 S,
 * 8 W) and picks one of the 16 rows of the 25-byte tile table at 0x004b72e4;
 * the row is then walked linearly across the 5x5 block, each cell getting
 * flags = 8, RF = 2, the class's shared object and the square's key, and the
 * tile code resolved through the class TSM the way the map loader does it
 * (`tsm[code >> 8].tsf->base + (code & 0xff)`, here always tileset 0 because
 * the table holds bytes).
 *
 * `owner` is optional: a null pointer leaves the square's owner alone.  Off-
 * map cells still get written through a null Cell*: original.
 * ========================================================================= */

/* 116/116 instructions and 367/367 bytes -- the same shape as the original
 * throughout.  The residual is one register tie-break: the original keeps the
 * row coordinate in EBX (the register it loaded `y` into at entry) and the
 * tile-table cursor in EBP (the register the function-wide zero vacates once
 * `i = 0` is stored), and this reconstruction has them the other way round,
 * so 16 instructions differ only in which of the two they name.  Three
 * spellings were needed to get this far and are load-bearing: `y` must live
 * across the JcWater_FindAt call (hence the key's y byte is stored after it),
 * the inner loop must run over a COUNTER with `cx` derived (`cx = x + j - 2`)
 * so VC6 link-function-test-replaces the guard into `(2 - x) + cx < 5`, and
 * `cy` must be written INLINE at both use sites -- as a named variable VC6
 * strength-reduces `cy * 4` into a fourth induction variable, which costs a
 * frame slot and five instructions. */
// WIP-FUNCTION: LEGOLAND 0x00436dc0  (116/116 insns, ebx<->ebp tie-break, 16 mismatches)
void JungleCruise_UpdateRiverTile(int x, int y, int mask, BPosW* owner)
{
    BPosW key;
    JcWater* w;
    const unsigned char* t;
    int i;
    int j;
    int cx;

    key.b.x = (unsigned char)x;
    w = JcWater_FindAt(x, y);
    key.b.y = (unsigned char)y;
    if (w == 0) {
        w = (JcWater*)HeapAlloc_w(sizeof(JcWater));
        if (w == 0)
            return;
        w->next = g_jc_water;
        w->f18 = 0;
        g_jc_water = w;
        JungleCruise_AddValue(*owner, 1);
    }
    t = g_jc_river_tiles[mask];
    w->pos = key;
    w->links = mask;
    if (owner != 0)
        w->owner = *owner;
    i = 0;
    g_bg_full_update = 1;
    for (; i < 5; i++) {
        for (j = 0; j < 5; j++) {
            Cell* c;
            unsigned short* rec;

            cx = x + j - 2;
            c = MapCellAt(cx, y + i - 2);
            c->flags = 8;
            c->rf = 2;
            c->obj = g_jc_water_cls->c4;
            c->key = key.w;
            rec = *(unsigned short**)((char*)g_jc_tsm +
                      ((unsigned)(unsigned char)*t >> 8) * 8 + 4);
            SetMapTile(cx, y + i - 2, (unsigned short)(*rec + (unsigned char)*t));
            t++;
        }
    }
}

/* The bounds test the river probe uses -- note the order differs from
 * MapCellAt's: both LOW bounds first, then both high ones. */
static __inline int InMapBounds(int x, int y)
{
    return x >= 0 && y >= 0 && x < g_map->width && y < g_map->height;
}

/* =========================================================================
 * 0x00436fb0 -- JungleCruise_ProbeRiver: what does the river square at
 * (x, y) connect to?
 *
 * Returns the link bitmap JungleCruise_UpdateRiverTile draws from:
 *
 *      1  north  (x,     y - 5)   or the route START of a station
 *      2  east   (x + 5, y    )
 *      4  south  (x,     y + 5)   or the route END of a station
 *      8  west   (x - 5, y    )
 *
 * and hands back, through *owner, the map square of the station the river
 * belongs to.  The FIRST square found that already has an owner sets it; a
 * neighbour whose owner disagrees is not linked, which is what keeps two
 * cruises' rivers from merging when they are laid next to each other.  The
 * square being probed is consulted first, so an existing square keeps its
 * own owner.
 *
 * The station pass is why a station reads as river to its neighbours: a
 * station's own square is never in the water list, but its route start
 * (+0x02) and route end (+0x04) squares link north and south respectively.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00436fb0
int JungleCruise_ProbeRiver(int x, int y, BPosW* owner)
{
    BPosW key;
    JcWater* w;
    int mask = 0;
    int have = 0;
    JcStation* st = g_jc_stations;

    w = JcWater_FindAt(x, y);
    if (w != 0) {
        *owner = w->owner;
        have = 1;
    }
    if (InMapBounds(x, y - 5)) {
        w = JcWater_FindAt(x, y - 5);
        if (w != 0) {
            if (have) {
                if (w->owner.w == owner->w)
                    mask |= 1;
            } else {
                mask |= 1;
                *owner = w->owner;
                have = 1;
            }
        }
    }
    if (InMapBounds(x + 5, y)) {
        w = JcWater_FindAt(x + 5, y);
        if (w != 0) {
            if (have) {
                if (w->owner.w == owner->w)
                    mask |= 2;
            } else {
                mask |= 2;
                *owner = w->owner;
                have = 1;
            }
        }
    }
    if (InMapBounds(x, y + 5)) {
        w = JcWater_FindAt(x, y + 5);
        if (w != 0) {
            if (have) {
                if (w->owner.w == owner->w)
                    mask |= 4;
            } else {
                mask |= 4;
                *owner = w->owner;
                have = 1;
            }
        }
    }
    if (InMapBounds(x - 5, y)) {
        w = JcWater_FindAt(x - 5, y);
        if (w != 0) {
            if (have) {
                if (w->owner.w == owner->w)
                    mask |= 8;
            } else {
                mask |= 8;
                *owner = w->owner;
                have = 1;
            }
        }
    }
    key.b.x = (unsigned char)x;
    key.b.y = (unsigned char)y;
    while (st) {
        if (key.w == st->a.w) {
            mask |= 1;
            break;
        }
        if (key.w == st->b.w) {
            mask |= 4;
            break;
        }
        st = st->next;
    }
    return mask;
}

/* Tile 0 of the water tileset -- the plain corner filler.  Written out at
 * every call site (never cached) because SetMapTile may move the tables, and
 * the original reloads 0x0081cb58 before each one. */
#define JC_CORNER_TILE  (**(unsigned short**)((char*)g_jc_tsm + 4))

/* =========================================================================
 * 0x004367b0 -- JungleCruise_RelinkRiverCell: fill in the inside corners
 * where two arms of the river meet.
 *
 * JungleCruise_UpdateRiverTile paints each 5x5 river square from its own
 * link mask, which leaves the four 2x2 blocks BETWEEN diagonally adjacent
 * squares as bare ground.  This pass looks at the square at (x, y), takes
 * its link mask, and for each of the four diagonal pairs -- (west, north),
 * (west, south), (east, north), (east, south) -- checks that the vertical
 * neighbour also carries the matching horizontal link, and if so stamps the
 * 2x2 corner block with tile 0 of the water tileset.
 *
 * Before that the square's own mask is adjusted for the station it belongs
 * to: the route START square loses its NORTH link and the route END square
 * its SOUTH link, so no corner is ever drawn into the station building.
 *
 * ORIGINAL BUGS reproduced: the station search may end on a null cursor
 * which is then dereferenced, and each neighbour lookup is dereferenced
 * without a null check (it cannot fail while the link bit is set, but
 * nothing enforces that).
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004367b0
void JungleCruise_RelinkRiverCell(int x, int y, BPosW* owner)
{
    JcStation* st = g_jc_stations;
    JcWater* w;
    JcWater* n;
    int links;

    w = JcWater_FindAt(x, y);
    while (st && st->pos.w != owner->w)
        st = st->next;
    if (w == 0)
        return;
    links = w->links;
    if (w->pos.w == st->a.w)
        links &= ~1;
    else if (w->pos.w == st->b.w)
        links &= ~4;

    if ((links & 8) && (links & 1)) {
        n = JcWater_FindAt(x, y - 5);
        if (n->links & 8) {
            SetMapTile(x - 3, y - 3, JC_CORNER_TILE);
            SetMapTile(x - 2, y - 3, JC_CORNER_TILE);
            SetMapTile(x - 3, y - 2, JC_CORNER_TILE);
            SetMapTile(x - 2, y - 2, JC_CORNER_TILE);
        }
    }
    if ((links & 8) && (links & 4)) {
        n = JcWater_FindAt(x, y + 5);
        if (n->links & 8) {
            SetMapTile(x - 3, y + 3, JC_CORNER_TILE);
            SetMapTile(x - 2, y + 3, JC_CORNER_TILE);
            SetMapTile(x - 3, y + 2, JC_CORNER_TILE);
            SetMapTile(x - 2, y + 2, JC_CORNER_TILE);
        }
    }
    if ((links & 2) && (links & 1)) {
        n = JcWater_FindAt(x, y - 5);
        if (n->links & 2) {
            SetMapTile(x + 3, y - 3, JC_CORNER_TILE);
            SetMapTile(x + 2, y - 3, JC_CORNER_TILE);
            SetMapTile(x + 3, y - 2, JC_CORNER_TILE);
            SetMapTile(x + 2, y - 2, JC_CORNER_TILE);
        }
    }
    if ((links & 2) && (links & 4)) {
        n = JcWater_FindAt(x, y + 5);
        if (n->links & 2) {
            SetMapTile(x + 3, y + 3, JC_CORNER_TILE);
            SetMapTile(x + 2, y + 3, JC_CORNER_TILE);
            SetMapTile(x + 3, y + 2, JC_CORNER_TILE);
            SetMapTile(x + 2, y + 2, JC_CORNER_TILE);
        }
    }
}

/* ---- the boat renderer's helpers ---------------------------------------- */
typedef struct Vec3 { float x; float y; float z; } Vec3;

/* The 3D person record, as this file touches it. */
typedef struct Person3D {
    unsigned char pad00[0x1c];
    int           sx;               /* +0x1c  screen position */
    int           sy;               /* +0x20 */
    unsigned char pad24[0x40 - 0x24];
    Vec3          rot;              /* +0x40  rot.y (+0x44) is the heading */
} Person3D;

/* A loaded image list (the LLIDB .ILF/.CSP descriptor). */
typedef struct ImageList {
    unsigned char pad00[8];
    void**        sprites;          /* +0x08 */
    int*          dx;               /* +0x0c  doubled at load, halved here */
    int*          dy;               /* +0x10 */
} ImageList;

typedef struct SeatOfs { int x; int y; } SeatOfs;

extern void GetTileDimensions(int* out_w, int* out_h);       /* 0x00460540 */
extern void AdjustOffsetForViewMode(Pos* o);                 /* 0x00442d30 */
extern void AdjustBlokePosition(Pos* p);                     /* 0x00442d60 */
extern int  PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern Person3D* Find3DPersonFromBloke(void* bloke);         /* 0x0043f890 */
extern void SetPersonRotation(Person3D* p, Vec3* rot);       /* 0x00440020 */
extern void IP_RenderBlokeIn3DNow(void* bloke);              /* 0x00440010 */

/* The visitor's ride state machine byte, the only Bloke field used here. */
typedef struct Bloke {
    unsigned char pad00[0x60];
    unsigned char stage;            /* +0x60 */
} Bloke;

extern ImageList* g_jc_boat_ilf;    /* 0x0081cd00  the boat image list */
/* Where each of the three seats sits inside the boat sprite, per heading:
 * [seat][frame & 0xf]. */
extern SeatOfs    g_jc_seat_pos[3][16];                      /* 0x0081cb80 */
extern int        g_jc_anim_tick;   /* 0x00629c54  playback cursor, 0..0x4f */
extern int        g_scroll_x;       /* 0x00667cb4  ScrollX (24.8) */
extern int        g_scroll_y;       /* 0x00667cb8  ScrollY (24.8) */

/* =========================================================================
 * 0x00432d00 -- JungleCruise_UpdateRiverAnim: draw every boat in the park.
 *
 * JungleCruise_Tick calls this once a frame with mode 0; the JUNGLE CRUISE
 * station's own painter calls it with mode 1.  `mode` selects WHICH half of
 * the boats to draw: a boat counts as INSIDE the station building when its
 * state is 1 (just launched) or 0x10 (arriving), or when it is standing on
 * the station's own column at or south of the station's exit row.  Mode 0
 * paints the boats on open water, mode 1 the ones inside the station, so the
 * building's sprite layers can be interleaved between the two passes.
 *
 * The isometric projection is the standard one: a cell (cx, cy) lands at
 *   x = (cx - cy) * (tw/2) - (tw+1)/2 - ScrollX>>8
 *   y = (cx + cy) * (th/2)           - ScrollY>>8
 * plus the map's screen origin, plus the boat's rocking offset for this
 * sub-step, converted from isometric (x-y, x+y) and scaled by 1/512.
 *
 * PAINT ORDER.  The boat is three sprites -- hull (code), and two overlays
 * (code + 0x10, code + 0x20) -- interleaved with up to three seated riders.
 * When the heading code is 4..0xb the boat faces "away" and the seats are
 * painted front to back (0, 1, 2) with the +0x10 overlay after seat 0 and
 * +0x20 after seat 2; otherwise they are painted back to front (2, 1, 0)
 * with +0x20 after seat 0 and +0x10 after seat 1.  And when the code is
 * above 8 seats 1 and 2 swap, so the near-side rider is always drawn last.
 *
 * Each rider's heading is the boat's, turned by +-6 sixteenths for the two
 * side seats and converted to radians:
 *      angle = -(code * 22.5 + 45) / 360 * 2*pi
 * (the constants at 0x004ab3dc..e8), and seat 2 is lifted 0x10 pixels.
 *
 * Finally, on the LAST sub-step of an arriving boat's last leg (state 0x10,
 * leg 2, tick 0x4f) and only in the station pass, the party is put ashore:
 * each rider's stage byte is advanced and its seat cleared, which is what
 * hands them back to JungleCruise_Tick's case 3.
 * ========================================================================= */

/* 422 emitted vs 419 original instructions.  Everything from the boat test to
 * the disembark tail matches the original one instruction for one, offset by
 * a single slot, and two things account for the whole residual:
 *   1. the original parks the constant 0 in edx for the whole body (`xor
 *      edx,edx` before the loop and again at 0x00433227) and compares against
 *      it -- `cmp esi,edx`, `cmp dword ptr [esp+0x48],edx` -- where this
 *      reconstruction emits `test`/`mov`+`test`.  That is the missing
 *      instruction at index 8 and the two extra ones in the disembark test.
 *   2. the frame is the right SIZE (0x34) but two pool homes differ: the
 *      original shares ONE 8-byte home (+0x14) between the sprite offset and
 *      the boat position -- disjoint blocks, exactly the rule in DECOMP.md --
 *      and puts `sy` in the top slot (+0x30); here the two Pos objects get
 *      separate homes (+0x24 and +0x2c) and `sy` lands at +0x04, which
 *      renumbers every [esp+N].  Splitting them into disjoint blocks (below)
 *      got the frame size right but not the sharing.
 * The behaviour, the record layout and the paint order are all reconstructed;
 * only the register/slot assignment is still off. */
// WIP-FUNCTION: LEGOLAND 0x00432d00  (422/419 insns; constant-zero register + two pool homes)
void JungleCruise_UpdateRiverAnim(int mode)
{
    int     sy;
    int     tw;
    int     th;
    JcBoat* b;

    b = g_jc_boats;
    GetTileDimensions(&tw, &th);
    if (b == 0)
        return;
    do {
        if (mode != 0) {
            if (b->state == 1)
                goto draw;
            if (b->state == 0x10)
                goto draw;
            if (b->cx != b->key.b.x)
                goto next;
            if (b->cy >= b->key.b.y + 5)
                goto draw;
            goto next;
        } else {
            if (b->state == 1)
                goto next;
            if (b->state == 0x10)
                goto next;
            if (b->cx != b->key.b.x)
                goto draw;
            if (b->cy >= b->key.b.y + 5)
                goto next;
        }
draw:
        {
            int wy = b->wob[g_jc_anim_tick].y;
            int wx = b->wob[g_jc_anim_tick].x;
            int tw2;
            int th2;
            int ox;
            int oy;
            int sx;
            int i;
            int code;

            GetTileDimensions(&tw2, &th2);
            ox = ((wx - wy) * tw2) >> 9;
            oy = ((wx + wy) * th2) >> 9;
            sx = (b->cx - b->cy) * (tw >> 1) - ((tw + 1) >> 1) - (g_scroll_x >> 8);
            sy = (b->cx + b->cy) * (th >> 1) - (g_scroll_y >> 8);
            {
                Pos ofs;

                ofs.x = g_jc_boat_ilf->dx[b->frame[g_jc_anim_tick] & 0xff] >> 1;
                ofs.y = g_jc_boat_ilf->dy[b->frame[g_jc_anim_tick] & 0xff] >> 1;
                AdjustOffsetForViewMode(&ofs);
                b->sx = g_map->origin_x + ox + ofs.x + sx;
                b->sy = g_map->origin_y + oy + ofs.y + sy;
                PrintSprite(g_jc_boat_ilf->sprites[b->frame[g_jc_anim_tick] & 0xff],
                            b->sx, b->sy, 0, 0);
            }
            {
            Pos bp;

            bp.x = g_map->origin_x + ox + sx;
            bp.y = g_map->origin_y + oy + sy;
            AdjustBlokePosition(&bp);

            if (b->frame[g_jc_anim_tick] >= 4 && b->frame[g_jc_anim_tick] < 0xc) {
                for (i = 0; i < 3; i++) {
                    Pos soA;
                    int adj = 0;
                    int seat;

                    if (b->frame[g_jc_anim_tick] > 8) {
                        if (i == 1)
                            adj = i;
                        else if (i == 2)
                            adj = -1;
                    }
                    seat = adj + i;
                    if (b->riders[seat] != 0) {
                        Person3D* p3 = Find3DPersonFromBloke(b->riders[seat]);

                        soA.x = g_jc_seat_pos[seat][b->frame[g_jc_anim_tick] & 0xf].x + 0x20;
                        soA.y = g_jc_seat_pos[seat][b->frame[g_jc_anim_tick] & 0xf].y + 0x18;
                        switch (seat) {
                        case 0:
                            {
                                float ang = ((float)b->frame[g_jc_anim_tick] * 22.5f + 45.0f)
                                            * -0.0027777769f;
                                p3->rot.y = ang * 6.2831855f;
                            }
                            break;
                        case 1:
                            {
                                float ang = ((float)((b->frame[g_jc_anim_tick] + 6) & 0xf) * 22.5f + 45.0f)
                                            * -0.0027777769f;
                                p3->rot.y = ang * 6.2831855f;
                            }
                            break;
                        case 2:
                            {
                                float ang = ((float)((b->frame[g_jc_anim_tick] - 6) & 0xf) * 22.5f + 45.0f)
                                            * -0.0027777769f;
                                p3->rot.y = ang * 6.2831855f;
                            }
                            soA.y -= 0x10;
                            break;
                        }
                        SetPersonRotation(p3, &p3->rot);
                        AdjustOffsetForViewMode(&soA);
                        p3->sx = soA.x + bp.x;
                        p3->sy = soA.y + bp.y;
                        IP_RenderBlokeIn3DNow(b->riders[seat]);
                    }
                    if (i != 0) {
                        if (i != 2)
                            goto no_overlay_a;
                        code = b->frame[g_jc_anim_tick] + 0x20;
                    } else {
                        code = b->frame[g_jc_anim_tick] + 0x10;
                    }
                    PrintSprite(g_jc_boat_ilf->sprites[code & 0xff], b->sx, b->sy, 0, 0);
no_overlay_a:
                    ;
                }
            } else {
                for (i = 2; i >= 0; i--) {
                    Pos soB;
                    int adj = 0;
                    int seat;

                    if (b->frame[g_jc_anim_tick] < 8) {
                        if (i == 1)
                            adj = i;
                        else if (i == 2)
                            adj = -1;
                    }
                    seat = adj + i;
                    if (b->riders[seat] != 0) {
                        Person3D* p3 = Find3DPersonFromBloke(b->riders[seat]);

                        soB.x = g_jc_seat_pos[seat][b->frame[g_jc_anim_tick] & 0xf].x + 0x20;
                        soB.y = g_jc_seat_pos[seat][b->frame[g_jc_anim_tick] & 0xf].y + 0x18;
                        switch (seat) {
                        case 0:
                            {
                                float ang = ((float)b->frame[g_jc_anim_tick] * 22.5f + 45.0f)
                                            * -0.0027777769f;
                                p3->rot.y = ang * 6.2831855f;
                            }
                            break;
                        case 1:
                            {
                                float ang = ((float)((b->frame[g_jc_anim_tick] + 6) & 0xf) * 22.5f + 45.0f)
                                            * -0.0027777769f;
                                p3->rot.y = ang * 6.2831855f;
                            }
                            break;
                        case 2:
                            {
                                float ang = ((float)((b->frame[g_jc_anim_tick] - 6) & 0xf) * 22.5f + 45.0f)
                                            * -0.0027777769f;
                                p3->rot.y = ang * 6.2831855f;
                            }
                            soB.y -= 0x10;
                            break;
                        }
                        SetPersonRotation(p3, &p3->rot);
                        AdjustOffsetForViewMode(&soB);
                        p3->sx = soB.x + bp.x;
                        p3->sy = soB.y + bp.y;
                        IP_RenderBlokeIn3DNow(b->riders[seat]);
                    }
                    if (i != 0) {
                        if (i != 1)
                            goto no_overlay_b;
                        code = b->frame[g_jc_anim_tick] + 0x10;
                    } else {
                        code = b->frame[g_jc_anim_tick] + 0x20;
                    }
                    PrintSprite(g_jc_boat_ilf->sprites[code & 0xff], b->sx, b->sy, 0, 0);
no_overlay_b:
                    ;
                }
            }
            }
            if (b->state == 0x10 && b->leg == 2 && g_jc_anim_tick == 0x4f && mode != 0
                && b->riders[0] != 0) {
                ((Bloke*)b->riders[0])->stage++;
                b->riders[0] = 0;
                if (b->riders[1] != 0) {
                    ((Bloke*)b->riders[1])->stage++;
                    b->riders[1] = 0;
                }
                if (b->riders[2] != 0) {
                    ((Bloke*)b->riders[2])->stage++;
                    b->riders[2] = 0;
                }
            }
        }
next:
        b = b->next;
    } while (b);
}
