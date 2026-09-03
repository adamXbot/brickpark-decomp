/* LEGOLAND -- ride callback cluster at 0x0043xxxx (the sibling block to the
 * 0x0042xxxx cluster).  These are the per-class handlers that
 * SetCustomCallbacks (screen.c 0x00452c20) installs into the 0xd0-byte ObjDef
 * callback slots; they are NOT exported, so the extents below were taken from
 * the disassembly by control flow (tools/audit.py).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere).
 *
 * Which class each function serves, read back out of SetCustomCallbacks --
 * and, since the slot numbers were only ever guesses, what the slot really
 * turns out to be for these classes:
 *
 *   addr        class                     slot     what it really is
 *   0x00434330  JUNGLE CRUISE MONKEY FISH cb_90    MonkeyFish_CalcCursor  [OK]
 *   0x00435470  JUNGLE CRUISE             cb_9c    JungleCruise_Remove    [OK]
 *   0x00435750  JUNGLE CRUISE             cb_a8    JungleCruise_Tick      [WIP]
 *   0x00435ec0  JUNGLE CRUISE             cb_b8    LoadJungleCruise       [OK]
 *   0x00436a40  JUNGLE CRUISE WATER       cb_9c    JungleCruiseWater_Remove [OK]
 *   0x004316f0  OCTOPUS CAFE              cb_a8    OctopusCafe_Tick       (analysed only)
 *   0x00431d00  OCTOPUS CAFE              cb_b0    OctopusCafe_Draw       (analysed only)
 *   0x00430b10  RESTAURANT 2              cb_b0    Restaurant2_Draw       (analysed only)
 *
 * So cb_a8 is the per-frame SERVICE tick (the ride's state machine over its
 * live instances), cb_b0 is the class's own PAINTER (used only where the
 * visitors have to be interleaved with the building's sprite layers), cb_90
 * is the placement-cursor calculator, cb_9c the remove handler and cb_b8 the
 * savegame loader -- consistent with what ridesave.c infers from the other
 * side.
 *
 * ------------------------------------------------------------------------
 * THE JUNGLE CRUISE DATA MODEL (recovered from the load/save pair, the remove
 * handlers and the river subsystem in junglecruise.c)
 *
 * CORRECTION (this pass): the two big lists were named the wrong way round.
 * 0x0062fd2c is the WATER list -- the river squares, which really are the
 * "JUNGLE CRUISE WATER" map objects -- and 0x00616164 is the BOAT list, whose
 * 0x3f8-byte records carry three riders at +0x3e8 and an 80-frame animation
 * buffer.  Everything below is the corrected reading.
 *
 * The ride keeps FIVE singly-linked global lists.  Unlike the rides in
 * ridesave.c -- which write `{ int32 1; record }`* + `int32 0` -- the jungle
 * cruise writes a COUNT first and then that many raw records:
 *
 *     int32 count; count x record        (x5, one group per list)
 *
 * (SaveJungleCruise @ 0x00435c70 counts the list by walking it, writes the
 * count, then writes each record; the loader here reads the count and loops
 * `while (n-- != 0)`.)
 *
 *   list           head       size    next    patched on load
 *   station        0x629c3c   0x44    +0x3c   5 bloke ids at +0x18..+0x28
 *   water (river)  0x62fd2c   0x1c    +0x10   --
 *   monkey fish    0x629c30   0x0c    +0x08   --
 *   monkey tree    0x629c2c   0x08    +0x04   --
 *   boat           0x616164   0x3f8   +0x3f4  3 bloke ids at +0x3e8..+0x3f0
 *
 * The bloke fields are saved as INDICES (SaveJungleCruise runs them through
 * GetBlokeNum, 0x00482fb0) and turned back into pointers here with
 * GetBlokePtr (0x00482fe0), the same index<->pointer pair savegame.c uses.
 *
 * Station record (0x44):  +0x00 u16 its own map square; +0x02/+0x03 the
 * route START square and +0x04/+0x05 the route END square (both as {x,y}
 * byte pairs compared 16 bits at a time); +0x08 the laid route; +0x14 the
 * queue length; +0x18..+0x28 the five queue slots; +0x2c the dispatch timer;
 * +0x30..+0x38 the party waiting to board; +0x3c next; +0x40 the ride's
 * VALUE -- every piece of it is worth something (a water square or a monkey
 * tree 1, a monkey fish 2; 0x00436130 adds by square, and 0x00436160, the
 * ride's cb_c0, reports the largest value in the park).
 *
 * Water record (0x1c): +0x00 its own square, +0x02 the owning station's
 * square, +0x04 the link bitmap (1 N, 2 E, 4 S, 8 W in steps of FIVE map
 * cells), +0x08/+0x0c/+0x14/+0x18 the route walk's scratch, +0x10 next.
 * Boat record (0x3f8): +0x00 the station that launched it, +0x04..+0x10 the
 * square it is on and the one it is heading for, +0x14/+0x18 its screen
 * position this frame, +0x1c 80 wobble pairs, +0x29c 80 sprite codes,
 * +0x3e0 the mover state, +0x3e8 the three riders, +0x3f4 next.
 *
 * See LEGOLAND/junglecruise.c for the river geometry and the boat mover.
  * ======================================================================== */

/* ---- save-game primitives (LEGOLAND/saveprof.c) ------------------------- */
extern int SaveGameRead(void* buf, unsigned int n);          /* 0x0047d730 */
extern void* HeapAlloc_w(unsigned int size);                 /* 0x0049e4ff */

/* ---- bloke index <-> pointer (LEGOLAND/sweep3.c) ------------------------ */
extern void* GetBlokePtr(int id);                            /* 0x00482fe0 */

/* ---- shared map/cursor types (same offsets as objmap2.c) ---------------- */
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

/* An object class descriptor (the 0xd0-byte ODF record); only its footprint
 * rect matters here. */
struct RideInst;
typedef struct ObjDef {
    unsigned char pad00[0x0c];
    int           dx;               /* +0x0c  class origin offset */
    int           dy;               /* +0x10 */
    unsigned char pad14[0x24 - 0x14];
    signed char   ox;               /* +0x24  exit-cell offset (signed) */
    signed char   oy;               /* +0x25 */
    unsigned char pad26[0x3c - 0x26];
    Rect          rect;             /* +0x3c */
    unsigned char pad50[0xc4 - 0x50];
    void*         c4;               /* +0xc4  the class's shared instance */
    unsigned char padc8[0xcc - 0xc8];
    struct RideInst* instances;     /* +0xcc  live instance list */
} ObjDef;

/* A park visitor, as this ride touches it. */
typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short action;          /* +0x0e  0 = idle */
    unsigned char  pad10[0x24 - 0x10];
    int            tx;              /* +0x24  walk target (24.8) */
    int            ty;              /* +0x28 */
    unsigned char  pad2c[0x60 - 0x2c];
    unsigned char  stage;           /* +0x60  ride state machine */
    unsigned char  pad61;
    unsigned short flags;           /* +0x62  8 = on this ride, 0x80 = seated */
    unsigned char  pad64[0x68 - 0x64];
    int            x;               /* +0x68  world position (24.8) */
    int            y;               /* +0x6c */
    unsigned char  pad70[2];
    unsigned char  speed;           /* +0x72 */
    unsigned char  dir8;            /* +0x73  heading, 0x20 per octant */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[4];         /* +0x98  the move line CalcMoveLine fills */
} Bloke;

/* The 3D person record that carries the on-screen position. */
typedef struct Person3D {
    unsigned char pad00[0x1c];
    int           sx;               /* +0x1c */
    int           sy;               /* +0x20 */
} Person3D;

/* One live instance of a class: the visitor and the map square it is on. */
typedef struct RideInst {
    struct RideInst* next;          /* +0x00 */
    unsigned char    pad04[4];
    Bloke*           bloke;         /* +0x08 */
    BPosW            key;           /* +0x0c */
} RideInst;

/* Queue-seat pixel offsets (g_jc_seat_ofs, indexed backwards). */
typedef struct SeatOfs { int dx; int dy; } SeatOfs;

/* A placed map object.  Only +0x0c (the class) matters here; the jungle
 * cruise builds a 0x14-byte STUB of one on its own stack to hand to the
 * child classes' remove handlers, so the declared size is load-bearing. */
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
    Rect          rect;             /* +0x1414 footprint rect list */
    unsigned char pad1428[0x1828 - 0x1428];
    unsigned int  flags;            /* +0x1828 */
    int           f182c;            /* +0x182c */
    struct Cursor* next;            /* +0x1830 */
} Cursor;                           /* 0x1834 */

/* ---- the jungle-cruise record lists -------------------------------------
 * Every record starts with its OWN map square and, at +0x02, the map square
 * of the station that owns it -- that pair is how the remove handler finds
 * everything belonging to one ride. */
typedef struct JcStation {
    BPosW          pos;             /* +0x00  the station's own map square */
    unsigned char  ax;              /* +0x02  route start x */
    unsigned char  ay;              /* +0x03  route start y */
    unsigned char  bx;              /* +0x04  route end x */
    unsigned char  by;              /* +0x05  route end y */
    unsigned char  pad06[2];
    void*          route;           /* +0x08  the laid boat route */
    unsigned char  pad0c[0x14 - 0x0c];
    int            count;           /* +0x14  visitors in the queue */
    void*          blokes[5];       /* +0x18  the five queue slots (indices in the save) */
    int            timer;           /* +0x2c  ticks until the next dispatch */
    Bloke*         riders[3];       /* +0x30  the party on the boat (raw in the save) */
    struct JcStation* next;         /* +0x3c */
    int            take;            /* +0x40  accumulated income */
} JcStation;                        /* 0x44 */

typedef struct JcWater {
    BPosW          pos;             /* +0x00 */
    BPosW          owner;           /* +0x02 */
    unsigned char  pad04[0x10 - 4];
    struct JcWater* next;            /* +0x10 */
    unsigned char  pad14[0x1c - 0x14];
} JcWater;                           /* 0x1c */

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

/* A fourth decoration list the remove handler tears down but the savegame
 * never writes -- its class pointer is 0x0081cb64 and its remove handler
 * 0x00434b40.  Same {pos, owner, next} head as the monkey tree. */
typedef struct JcDeco {
    BPosW          pos;             /* +0x00 */
    BPosW          owner;           /* +0x02 */
    struct JcDeco* next;            /* +0x04 */
} JcDeco;                           /* 0x08 */

typedef struct JcBoat {
    BPosW          pos;             /* +0x00 */
    unsigned char  pad02[0x3e8 - 2];
    void*          blokes[3];       /* +0x3e8 */
    struct JcBoat* next;           /* +0x3f4 */
} JcBoat;                          /* 0x3f8 */

/* Rebuilds one station's boat route: clears the +0x18 back-pointer of every
 * boat quoting `id`, finds the station with that id, and re-runs the river
 * walk from its map square.  Takes the id as a 16-bit value (the caller only
 * loads dx, the callee only reads cx). */
extern void JungleCruise_RebuildRoute(BPosW id);             /* 0x004373c0 */

extern JcStation*    g_jc_stations;    /* 0x00629c3c */
extern JcWater*       g_jc_water;       /* 0x0062fd2c */
extern JcMonkeyFish* g_jc_fish;        /* 0x00629c30 */
extern JcMonkeyTree* g_jc_trees;       /* 0x00629c2c */
extern JcDeco*       g_jc_deco;        /* 0x00629c34 */
extern JcBoat*      g_jc_boats;       /* 0x00616164 */

/* The map rectangle the ride's own tiles cover, restored cell by cell when
 * the ride is torn down (four ints; the x loop stops one short of `right`
 * while the y loop includes `bottom` -- an original asymmetry). */
extern Rect  g_jc_area;                /* 0x00629c40 */

/* The class descriptors of the four child object types. */
extern ObjDef* g_jc_water_cls;          /* 0x0081cb54 */
extern ObjDef* g_jc_deco_cls;          /* 0x0081cb64 */
extern ObjDef* g_jc_tree_cls;          /* 0x0081cb70 */
extern ObjDef* g_jc_fish_cls;          /* 0x0081cb74 */

/* A scratch cursor the boat teardown drives (its footprint rect is stamped
 * with {-2,-2,2,2} first). */
extern Cursor g_jc_water_cursor;        /* 0x0082ae20 */

/* ---- other subsystems --------------------------------------------------- */
extern void StandardRemoveObject(MapObj* obj, BPosW bp, Cursor* ctx); /* 0x0045f220 */
extern void RestoreBaseMap(int x, int y);                    /* 0x0045da60 */
extern void IncrementObjectCount(ObjDef* cls);               /* 0x00480d40 */
extern void RemoveAllBlokesFromRide(ObjDef* cls, BPosW tile);/* 0x0048a2e0 */
extern void HeapFree_w(void* p);                             /* 0x0049e4d0 */

/* The child classes' own remove handlers (0x00433fc0 and 0x00434670 are the
 * MONKEY TREE / MONKEY FISH cb_remove slots in SetCustomCallbacks). */
extern void JcWater_RemoveOne(MapObj* obj, BPosW bp, Cursor* ctx);      /* 0x00436f30 */
extern void MonkeyTree_Remove(MapObj* obj, BPosW bp, Cursor* ctx);  /* 0x00433fc0 */
extern void MonkeyFish_Remove(MapObj* obj, BPosW bp, Cursor* ctx);  /* 0x00434670 */
extern void JcDeco_Remove(MapObj* obj, BPosW bp, Cursor* ctx);      /* 0x00434b40 */

/* Unlinks one water record from g_jc_boats and frees it. */
extern void JcBoat_Unlink(JcBoat* w);                             /* 0x00432cb0 */

/* =========================================================================
 * 0x00435ec0 -- LoadJungleCruise (JUNGLE CRUISE cb_load).
 *
 * Reads the five count-prefixed groups back in order, allocating each record
 * and appending it to its list, then re-resolves the bloke indices and
 * finally rebuilds every station's boat route.
 *
 * TWO ORIGINAL QUIRKS reproduced verbatim:
 *  - the four leading lists start their append cursor at NULL, so loading
 *    over a non-empty list REPLACES the head and leaks the old chain; the
 *    water list instead starts its cursor at the CURRENT head, so it appends
 *    onto the first node (overwriting that node's next, not the tail's).
 *  - no SaveGameRead result is ever checked: a truncated save walks off the
 *    end of the stream instead of failing.  The function always returns 1.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00435ec0
int LoadJungleCruise(void)
{
    unsigned int n;
    JcStation* st = 0;
    JcWater* boat = 0;
    JcMonkeyTree* d = 0;
    JcMonkeyFish* c = 0;
    JcBoat* w;
    void** q;
    int k;

    SaveGameRead(&n, 4);
    while (n-- != 0) {
        if (!st)
            st = g_jc_stations = (JcStation*)HeapAlloc_w(sizeof(JcStation));
        else
            st = st->next = (JcStation*)HeapAlloc_w(sizeof(JcStation));
        SaveGameRead(st, sizeof(JcStation));
        q = st->blokes;
        k = 5;
        do {
            *q = GetBlokePtr((int)*q);
            q++;
        } while (--k);
    }

    SaveGameRead(&n, 4);
    while (n-- != 0) {
        if (!boat)
            boat = g_jc_water = (JcWater*)HeapAlloc_w(sizeof(JcWater));
        else
            boat = boat->next = (JcWater*)HeapAlloc_w(sizeof(JcWater));
        SaveGameRead(boat, sizeof(JcWater));
    }

    SaveGameRead(&n, 4);
    while (n-- != 0) {
        if (!c)
            c = g_jc_fish = (JcMonkeyFish*)HeapAlloc_w(sizeof(JcMonkeyFish));
        else
            c = c->next = (JcMonkeyFish*)HeapAlloc_w(sizeof(JcMonkeyFish));
        SaveGameRead(c, sizeof(JcMonkeyFish));
    }

    SaveGameRead(&n, 4);
    while (n-- != 0) {
        if (!d)
            d = g_jc_trees = (JcMonkeyTree*)HeapAlloc_w(sizeof(JcMonkeyTree));
        else
            d = d->next = (JcMonkeyTree*)HeapAlloc_w(sizeof(JcMonkeyTree));
        SaveGameRead(d, sizeof(JcMonkeyTree));
    }

    w = g_jc_boats;
    SaveGameRead(&n, 4);
    while (n-- != 0) {
        if (!w)
            w = g_jc_boats = (JcBoat*)HeapAlloc_w(sizeof(JcBoat));
        else
            w = w->next = (JcBoat*)HeapAlloc_w(sizeof(JcBoat));
        SaveGameRead(w, sizeof(JcBoat));
        q = w->blokes;
        k = 3;
        do {
            *q = GetBlokePtr((int)*q);
            q++;
        } while (--k);
    }

    for (st = g_jc_stations; st; st = st->next)
        JungleCruise_RebuildRoute(st->pos);

    return 1;
}

/* =========================================================================
 * 0x00435470 -- JungleCruise_Remove (JUNGLE CRUISE cb_remove).
 *
 * Tearing down one jungle cruise station:
 *  1. the standard class remove (refund, cell teardown) on the station cell;
 *  2. restore the base map under the ride's own tile rectangle g_jc_area
 *     -- note the x loop stops at `right - 1` while the y loop runs to
 *     `bottom` inclusive, an asymmetry the original really has;
 *  3. find the station record whose map square equals the removed cell
 *     (remembering its predecessor for the unlink);
 *  4. remove every child object that quotes that square as its owner --
 *     boats, monkey trees, monkey fish and the fourth decoration list --
 *     by calling each child class's OWN remove handler with a stack-built
 *     MapObj stub carrying that class, and a cursor whose origin has been
 *     repointed at the child's map square (the caller's cursor is borrowed
 *     and restored for three of the four lists; the boats get the scratch
 *     cursor g_jc_water_cursor with a {-2,-2,2,2} footprint stamped in).
 *     Each removal restarts the walk from the list head, because the child
 *     handler unlinks the record being pointed at.
 *  5. unlink the station, drop every water record of that ride, evict the
 *     riders and free the station.
 *
 * ORIGINAL QUIRKS reproduced: IncrementObjectCount is called TWICE on the
 * boat class before the boats are removed (each boat removal decrements, so
 * the class instance count ends up two high); and the station list head is
 * dereferenced without a null check, so removing a jungle cruise when the
 * station list is empty faults.
 * ========================================================================= */

/* The footprint stamped into the scratch cursor before the boats go: a 5x5
 * block centred on the boat (the original's constant at 0x004b7478). */
static const Rect kJcWaterRect = { -2, -2, 2, 2, 0 };

// FUNCTION: LEGOLAND 0x00435470
void JungleCruise_Remove(MapObj* obj, BPosW bp, Cursor* ctx)
{
    JcBoat*      w;
    JcStation*    prev;
    MapObj        child;
    JcStation*    st;
    JcWater*       b;
    JcMonkeyTree* t;
    JcMonkeyFish* f;
    JcDeco*       dc;
    int x, y;

    prev = 0;
    w = g_jc_boats;
    st = g_jc_stations;
    StandardRemoveObject(obj, bp, ctx);

    for (y = g_jc_area.top; y <= g_jc_area.bottom; y++) {
        for (x = g_jc_area.left; x <= g_jc_area.right - 1; x++)
            RestoreBaseMap(ctx->origin.x + x, ctx->origin.y + y);
    }

    while (st->pos.w != bp.w) {
        prev = st;
        st = st->next;
        if (!st)
            return;
    }
    if (!st)
        return;

    child.cls = g_jc_water_cls;
    IncrementObjectCount(g_jc_water_cls);
    IncrementObjectCount(g_jc_water_cls);
    g_jc_water_cursor.rect = kJcWaterRect;
    b = g_jc_water;
    while (b) {
        if (b->owner.w == bp.w) {
            g_jc_water_cursor.origin.x = b->pos.b.x;
            g_jc_water_cursor.origin.y = b->pos.b.y;
            JcWater_RemoveOne(&child, b->pos, &g_jc_water_cursor);
            b = g_jc_water;
        } else {
            b = b->next;
        }
    }

    child.cls = g_jc_tree_cls;
    t = g_jc_trees;
    while (t) {
        if (t->owner.w == bp.w) {
            int sx = ctx->origin.x;
            int sy = ctx->origin.y;
            ctx->origin.x = t->pos.b.x;
            ctx->origin.y = t->pos.b.y;
            MonkeyTree_Remove(&child, t->pos, ctx);
            ctx->origin.x = sx;
            ctx->origin.y = sy;
            t = g_jc_trees;
        } else {
            t = t->next;
        }
    }

    child.cls = g_jc_fish_cls;
    f = g_jc_fish;
    while (f) {
        if (f->owner.w == bp.w) {
            int sx = ctx->origin.x;
            int sy = ctx->origin.y;
            ctx->origin.x = f->pos.b.x;
            ctx->origin.y = f->pos.b.y;
            MonkeyFish_Remove(&child, f->pos, ctx);
            ctx->origin.x = sx;
            ctx->origin.y = sy;
            f = g_jc_fish;
        } else {
            f = f->next;
        }
    }

    child.cls = g_jc_deco_cls;
    dc = g_jc_deco;
    while (dc) {
        if (dc->owner.w == bp.w) {
            int sx = ctx->origin.x;
            int sy = ctx->origin.y;
            ctx->origin.x = dc->pos.b.x;
            ctx->origin.y = dc->pos.b.y;
            JcDeco_Remove(&child, dc->pos, ctx);
            ctx->origin.x = sx;
            ctx->origin.y = sy;
            dc = g_jc_deco;
        } else {
            dc = dc->next;
        }
    }

    if (prev)
        prev->next = st->next;
    else
        g_jc_stations = st->next;

    while (w) {
        if (w->pos.w == bp.w) {
            JcBoat_Unlink(w);
            w = g_jc_boats;
        } else {
            w = w->next;
        }
    }

    RemoveAllBlokesFromRide(obj->cls, bp);
    HeapFree_w(st);
}

/* =========================================================================
 * 0x00434330 -- MonkeyFish_CalcCursor (JUNGLE CRUISE MONKEY FISH cb_90).
 *
 * cb_90 is the class's "work out the placement cursor(s)" hook, the same slot
 * CalcBasicObjectCursor (0x0045fa80) fills for ordinary objects.  The monkey
 * fish is a river decoration: it may only be dropped where the jungle-cruise
 * river runs, and it produces up to FOUR preview cursors (one per river
 * direction) chained onto the edit cursor.
 *
 * The edit cursor's own footprint is set from the class rect and its origin
 * from the mouse (ScreenToMapRef), then the body runs TWICE, i = 0 and 1,
 * probing the river at (origin.x, origin.y - 5*i):
 *
 *   JungleCruise_ProbeRiver(x, y, &probe[i]) returns a bitmask of the river
 *   directions found (1 = north, 2 = east, 4 = south, 8 = west, each five
 *   cells out) and stores the owning station's map square in probe[i].
 *
 *   - on the second pass, if the two probes name DIFFERENT stations and the
 *     first probe actually found one, the placement is abandoned silently;
 *   - if the second pass finds no river at all, the edit cursor is failed
 *     with error 14;
 *   - otherwise, when ValidateCursor leaves the edit cursor valid, the four
 *     scratch cursors g_jc_cursors[i][0..3] are reset, given the class rect
 *     with `top` pushed down five cells, flagged 0x2034, and the ones whose
 *     direction bit is set are handed the neighbouring origin.
 *
 * The scratch cursors are filled CONSECUTIVELY across both passes (the write
 * cursor is not reset between them), then chained through their +0x1830 next
 * pointers and hung off the edit cursor, so the renderer draws every
 * candidate square in one pass.
 *
 * CODEGEN NOTES (both cost a long search).  The scratch cursors are indexed
 * with the running `count`, and VC6 strength-reduces that into TWO induction
 * variables -- one per origin field -- homed in the dead argument slots, then
 * link-function-test-replaces the two `count` guards against them, which is
 * why the original compares a pointer with a SIGNED jle.  And `cls` must be
 * assigned immediately before the footprint copy, not at its declaration: as
 * a declaration initialiser it takes a different frame home and the whole
 * register plan shifts (that one line was the difference between 88% and
 * 100%).  The four probe bytes are zeroed from `count` so that VC6 parks the
 * function's zero in ebx early, which is what makes them `bl` stores.
 * ========================================================================= */

extern void  DefaultCursor(Cursor* c);                       /* 0x0045a390 */
extern void  ScreenToMapRef(int sx, Pos* out, int sy);       /* 0x0045be90 */
extern void  ValidateCursor(Cursor* c, ObjDef* cls);         /* 0x0045f810 */
extern int   CursorIsValid(Cursor* c);                       /* 0x0045f4b0 */
extern void  ResetCursorFootprint(Cursor* c);                /* 0x0045f460 */
extern void  SetCursorError(Cursor* c, int code);            /* 0x0045f480 */

/* Probes the jungle-cruise river around (x, y): returns the direction
 * bitmask and stores the owning station's map square in *owner. */
extern int   JungleCruise_ProbeRiver(int x, int y, BPosW* owner); /* 0x00436fb0 */

extern Cursor  g_edit_cursor;          /* 0x007febc0 */
extern Pos     g_edit_cursor_origin;   /* 0x007fffc4 == g_edit_cursor.origin */
extern Rect    g_edit_cursor_rect;     /* 0x007fffd4 == g_edit_cursor.rect */
extern Cursor* g_edit_cursor_next;     /* 0x008003f0 == g_edit_cursor.next */

/* The scratch preview cursors, two passes of four. */
extern Cursor  g_jc_cursors[2][4];     /* 0x00616180, stride 0x1834 */

// FUNCTION: LEGOLAND 0x00434330
void MonkeyFish_CalcCursor(MapObj* o, int sx, int sy)
{
    int     count;
    BPosW   probe[2];
    ObjDef* cls;
    int     i;

    count = 0;
    probe[0].b.x = (unsigned char)count;
    probe[0].b.y = (unsigned char)count;
    probe[1].b.x = (unsigned char)count;
    probe[1].b.y = (unsigned char)count;
    cls = o->cls;
    g_edit_cursor_rect = cls->rect;
    ScreenToMapRef(sx, &g_edit_cursor_origin, sy);
    g_edit_cursor_next = 0;

    for (i = 0; i < 2; i++) {
        int found = JungleCruise_ProbeRiver(g_edit_cursor_origin.x,
                                            g_edit_cursor_origin.y - 5 * i,
                                            &probe[i]);
        if (i == 1 && probe[0].w != probe[1].w && probe[0].b.x != 0)
            return;
        if (!found && i == 1) {
            SetCursorError(&g_edit_cursor, 14);
            return;
        }
        ValidateCursor(&g_edit_cursor, cls);
        if (CursorIsValid(&g_edit_cursor)) {
            Rect r = g_edit_cursor_rect;

            r.top += 5;
            DefaultCursor(&g_jc_cursors[i][0]);
            DefaultCursor(&g_jc_cursors[i][1]);
            DefaultCursor(&g_jc_cursors[i][2]);
            DefaultCursor(&g_jc_cursors[i][3]);
            g_jc_cursors[i][0].rect = r;
            g_jc_cursors[i][1].rect = r;
            g_jc_cursors[i][2].rect = r;
            g_jc_cursors[i][3].rect = r;
            ResetCursorFootprint(&g_jc_cursors[i][0]);
            ResetCursorFootprint(&g_jc_cursors[i][1]);
            ResetCursorFootprint(&g_jc_cursors[i][2]);
            ResetCursorFootprint(&g_jc_cursors[i][3]);
            g_jc_cursors[i][0].flags = 0x2034;
            g_jc_cursors[i][1].flags = 0x2034;
            g_jc_cursors[i][2].flags = 0x2034;
            g_jc_cursors[i][3].flags = 0x2034;
            if (found & 1) {
                g_jc_cursors[0][count].origin.x = g_edit_cursor_origin.x;
                g_jc_cursors[0][count].origin.y = g_edit_cursor_origin.y - (5 * i + 5);
                count++;
            }
            if (found & 2) {
                g_jc_cursors[0][count].origin.x = g_edit_cursor_origin.x + 5;
                g_jc_cursors[0][count].origin.y = g_edit_cursor_origin.y - 5 * i;
                count++;
            }
            if (found & 4) {
                g_jc_cursors[0][count].origin.x = g_edit_cursor_origin.x;
                g_jc_cursors[0][count].origin.y = g_edit_cursor_origin.y - 5 * i + 5;
                count++;
            }
            if (found & 8) {
                g_jc_cursors[0][count].origin.x = g_edit_cursor_origin.x - 5;
                g_jc_cursors[0][count].origin.y = g_edit_cursor_origin.y - 5 * i;
                count++;
            }
            if (count != 0) {
                g_edit_cursor_next = &g_jc_cursors[0][0];
                if (count > 1) {
                    int j;
                    for (j = 1; j < count; j++)
                        g_jc_cursors[0][j - 1].next = &g_jc_cursors[0][j];
                }
            }
        }
    }
}

/* =========================================================================
 * 0x00436a40 -- JungleCruiseWater_Remove (JUNGLE CRUISE WATER cb_remove).
 *
 * Removing one river square.  The cell under the removed square decides
 * which teardown runs: if the cell's object is NOT the boat class's shared
 * instance (ObjDef +0xc4) the square belongs to the STATION, so the whole
 * ride goes through JungleCruise_Remove with a stack-built MapObj stub
 * carrying the station class; otherwise only this piece of river is removed
 * and its neighbourhood is re-stitched:
 *
 *   - JungleCruise_ProbeRiver on the removed cell gives the direction mask
 *     of the river arms that touched it (1 N, 2 E, 4 S, 8 W, five cells out);
 *   - JcWater_RemoveOne tears down this square's own boat/water object;
 *   - each arm that existed is re-probed and its tile rebuilt
 *     (UpdateRiverTile gives back the owning station's square, which
 *     RelinkRiverCell then uses to re-attach the cell to its ride);
 *   - each DIAGONAL between two surviving arms is rebuilt the same way, but
 *     only where a water record actually exists there;
 *   - finally the ride's route is rebuilt and, if the removed square was a
 *     station's own square, that station's route is recomputed from its two
 *     endpoint cells (+0x02/+0x03 and +0x04/+0x05) into +0x08.
 *
 * ORIGINAL BUG reproduced: the bounds-checked cell fetch can return NULL and
 * the very next instruction dereferences it, so removing a river square whose
 * map coordinates are off the map faults.
 *
 * STATE: 327 of the original's 328 instructions, block-for-block identical,
 * every stack home identical (st/south/east/stub in the 0x20 frame; the
 * north flag, `owner` and `probe` in the three dead argument slots).  What
 * differs is 37 SCRATCH-REGISTER names: from the `north = mask & 1` onward
 * the original uses eax where this uses edx and the `lea` of &probe rotates
 * one register further, and in the tail the station walker lands in esi
 * rather than edi, which costs the original's one extra `mov esi,edx` in the
 * BuildRoute argument set-up (hence 327 vs 328).  Every re-spelling tried
 * (operand order, temporaries, declaration order, loop shape, break-vs-
 * return, aggregate pinning) leaves the permutation untouched, so it is
 * being decided somewhere upstream.  Held as WIP until the register plan is
 * reproduced.
 *
 * The three direction flags carry `volatile` as a pure CODEGEN LEVER: the
 * original re-loads and re-tests `north` at the NE guard and `south` at the
 * SE guard even though both are provably still set on every path that
 * reaches them, and nothing else stops VC6 from folding those tests away.
 * ========================================================================= */

/* The map grid the cell fetch walks (same layout as objmap2.c). */
typedef struct Cell {
    void*         obj;              /* +0x00 */
    unsigned char pad04[0x14 - 4];
} Cell;                             /* 0x14 */

typedef struct MapHdr {
    unsigned char  pad00[0x14];
    unsigned short width;           /* +0x14 */
    unsigned short height;          /* +0x16 */
} MapHdr;

extern MapHdr* g_map;               /* 0x004bcbf4 (lpConfig) */
extern Cell**  g_map_rows;          /* 0x00801400 (GameMap) */

/* The JUNGLE CRUISE station class (the one whose remove tears the ride down). */
extern ObjDef* g_jc_station_cls;    /* 0x0081cb60 */

/* Returns the water record covering the map square, or 0. */
extern JcBoat* JcWater_FindAt(int x, int y);            /* 0x004371b0 */
/* Rebuilds the river tile at (x, y) for the given direction mask and hands
 * back the owning station's map square. */
extern void JungleCruise_UpdateRiverTile(int x, int y, int mask, BPosW* owner); /* 0x00436dc0 */
/* Re-attaches the cell at (x, y) to the station named by *owner. */
extern void JungleCruise_RelinkRiverCell(int x, int y, BPosW* owner); /* 0x004367b0 */
/* Lays a boat route between two map squares; returns the route record. */
extern void* JungleCruise_BuildRoute(int x0, int y0, int x1, int y1); /* 0x004371e0 */

/* The bounds-checked cell fetch every map accessor open-codes. */
static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

/* CLOSED (2026-09-03): the 21 tail mismatches were the station walker landing
 * in ESI instead of EDI.  Transferred from the twin BoatingSchoolWater_Remove
 * (ridecb5.c, 0x0041c130): the four u8 dock coordinates are widened into
 * `int ax, ay, bx, by;` locals read in exactly that order BEFORE the
 * BuildRoute call, instead of being passed straight from the fields.  That
 * alone was enough here -- the twin also needed its volatile north/south
 * flags routed through a plain scalar def, but this body already spells the
 * four `(mask & bit)` guards inline (which VC6 CSEs into a spilled temporary
 * and does not thread across the next guard), so the flag lever was not
 * required.  Earlier passes recorded: inlining those guards was worth 16 of
 * the original 37 mismatches. */
// FUNCTION: LEGOLAND 0x00436a40
void JungleCruiseWater_Remove(MapObj* o, BPosW bp, Cursor* ctx)
{
    JcStation* st = g_jc_stations;
    Cell*      cell;
    int        mask;
    BPosW      owner;
    int        ax, ay, bx, by;

    {
        int cx = bp.b.x;
        int cy = bp.b.y;
        cell = MapCellAt(cx, cy);
    }
    /* Original: no null check -- an off-map square faults here. */
    if (cell->obj != g_jc_water_cls->c4) {
        MapObj stub;

        stub.cls = g_jc_station_cls;
        JungleCruise_Remove(&stub, bp, ctx);
        return;
    }

    mask = JungleCruise_ProbeRiver(ctx->origin.x, ctx->origin.y, &owner);
    JcWater_RemoveOne(o, bp, ctx);

    if (mask & 1) {
        int ny = ctx->origin.y - 5;
        int nx = ctx->origin.x;
        BPosW probe;
        int m = JungleCruise_ProbeRiver(nx, ny, &probe);

        JungleCruise_UpdateRiverTile(nx, ny, m, &owner);
        JungleCruise_RelinkRiverCell(nx, ny, &owner);
    }
    if (mask & 2) {
        int nx = ctx->origin.x + 5;
        int ny = ctx->origin.y;
        BPosW probe;
        int m = JungleCruise_ProbeRiver(nx, ny, &probe);

        JungleCruise_UpdateRiverTile(nx, ny, m, &owner);
        JungleCruise_RelinkRiverCell(nx, ny, &owner);
    }
    if (mask & 4) {
        int ny = ctx->origin.y + 5;
        int nx = ctx->origin.x;
        BPosW probe;
        int m = JungleCruise_ProbeRiver(nx, ny, &probe);

        JungleCruise_UpdateRiverTile(nx, ny, m, &owner);
        JungleCruise_RelinkRiverCell(nx, ny, &owner);
    }
    if (mask & 8) {
        int nx = ctx->origin.x - 5;
        int ny = ctx->origin.y;
        BPosW probe;
        int m = JungleCruise_ProbeRiver(nx, ny, &probe);

        JungleCruise_UpdateRiverTile(nx, ny, m, &owner);
        JungleCruise_RelinkRiverCell(nx, ny, &owner);
    }

    if ((mask & 1) && (mask & 8)) {
        int nx = ctx->origin.x - 5;
        int ny = ctx->origin.y - 5;

        if (JcWater_FindAt(nx, ny)) {
            BPosW probe;
            int m = JungleCruise_ProbeRiver(nx, ny, &probe);

            JungleCruise_UpdateRiverTile(nx, ny, m, &owner);
            JungleCruise_RelinkRiverCell(nx, ny, &owner);
        }
    }
    if ((mask & 1) && (mask & 2)) {
        int nx = ctx->origin.x + 5;
        int ny = ctx->origin.y - 5;

        if (JcWater_FindAt(nx, ny)) {
            BPosW probe;
            int m = JungleCruise_ProbeRiver(nx, ny, &probe);

            JungleCruise_UpdateRiverTile(nx, ny, m, &owner);
            JungleCruise_RelinkRiverCell(nx, ny, &owner);
        }
    }
    if ((mask & 4) && (mask & 8)) {
        int nx = ctx->origin.x - 5;
        int ny = ctx->origin.y + 5;

        if (JcWater_FindAt(nx, ny)) {
            BPosW probe;
            int m = JungleCruise_ProbeRiver(nx, ny, &probe);

            JungleCruise_UpdateRiverTile(nx, ny, m, &owner);
            JungleCruise_RelinkRiverCell(nx, ny, &owner);
        }
    }
    if ((mask & 4) && (mask & 2)) {
        int nx = ctx->origin.x + 5;
        int ny = ctx->origin.y + 5;

        if (JcWater_FindAt(nx, ny)) {
            BPosW probe;
            int m = JungleCruise_ProbeRiver(nx, ny, &probe);

            JungleCruise_UpdateRiverTile(nx, ny, m, &owner);
            JungleCruise_RelinkRiverCell(nx, ny, &owner);
        }
    }

    JungleCruise_RebuildRoute(owner);
    while (st) {
        if (st->pos.w == owner.w) {
            ax = st->ax;
            ay = st->ay;
            bx = st->bx;
            by = st->by;
            st->route = JungleCruise_BuildRoute(ax, ay, bx, by);
            return;
        }
        st = st->next;
    }
}

/* =========================================================================
 * 0x00435750 -- JungleCruise_Tick (JUNGLE CRUISE cb_a8).
 *
 * The ride's per-frame service handler; it runs in two halves.
 *
 * (1) THE STATIONS.  Every 0x50 calls the boats are advanced one step
 * (JungleCruise_AdvanceBoats) and the river animation is refreshed on every
 * call.  Then each station is polled: when it holds a party (riders[0] set),
 * its dispatch timer has run out, it has a laid route, and its takings are
 * more than six per river square, JungleCruise_TryLaunchBoat puts the party
 * on the water.  On success each rider is sat down (flag 0x80, stage++,
 * BlokeSitAnim + frame 0), the timer is reset to 0x96 and the three rider
 * slots are cleared.
 *
 * (2) THE QUEUE.  Each live instance of the station class carries the bloke
 * at +0x08 and the station's map square at +0x0c.  The bloke's stage byte
 * (+0x60) drives a six-way switch, but only while its action word (+0x0e) is
 * zero -- i.e. while it is not already busy:
 *
 *   0  join / shuffle up the five-deep queue.  If the bloke is not in the
 *      queue yet it is appended at slot 4 (rejected, and thrown off the ride,
 *      when the queue is full or slot 4 is taken); if it is already in the
 *      queue and the slot in front is free it steps forward, and reaching
 *      slot 0 advances it to stage 1.  Either way it is given flag 8 and
 *      walked to the seat's pixel offset (g_jc_seat_ofs, a table indexed
 *      BACKWARDS from 0x004b72b0: {-832,592} {-832,296} {-832,0} {-544,0}
 *      {-256,0} for slots 0..4) with action 7.
 *   1  at the head of the queue: take the first free boat seat (+0x30/+0x34/
 *      +0x38), park the bloke off-screen at -0x270f, drop it out of the queue
 *      and decrement the queue count.
 *   2  riding -- nothing to do.
 *   3  getting off: walk it back to the map, converting the 3D person's
 *      screen position (Find3DPersonFromBloke + AdjustBlokePosition, minus
 *      the sprite's 0x10 half-width) into a map reference, clearing flag
 *      0x80 and walking it to the station's exit cell (class +0x24/+0x25,
 *      SIGNED byte offsets), then stage++.
 *   4  walking to the station centre, then stage++.
 *   5  done: clear flag 8 and RemoveBlokeFromRide.
 *
 * TWO ORIGINAL BUGS reproduced: the station lookup by map square may end
 * with NO station and the switch dereferences it anyway; and the three boat
 * seat pointers at +0x30..+0x38 are written to the savegame RAW while the
 * five queue pointers at +0x18..+0x28 go through GetBlokeNum, so a reloaded
 * game restores stale rider pointers for a boat that is on the water.
 * ========================================================================= */

extern void  BlokeSitAnim(Bloke* b);                         /* 0x00440780 */
extern void  BlokeSetFrame(Bloke* b, int frame);             /* 0x00440870 */
extern void  BlokeWalkAnim(Bloke* b);                        /* 0x00440910 */
extern Person3D* Find3DPersonFromBloke(Bloke* b);            /* 0x0043f890 */
extern void  AdjustBlokePosition(Pos* p);                    /* 0x00442d60 */
/* The call site here passes a THIRD argument the callee (pathtile2.c, two
 * parameters) never reads -- the original pushes a zero for it, so the
 * prototype this translation unit was compiled against had three. */
extern int   ScreenToMapRef2(Pos* screen, Pos* out, int unused); /* 0x0045be00 */
/* Declared with FIVE int parameters rather than the two by-value Pos of
 * workers2.c/blokeai.c: identical ABI, but it keeps the caller from building
 * the two aggregates and reproduces the original's push order here. */
extern int   CalcMoveLine(int fx, int fy, int tx, int ty, void* path); /* 0x00480740 */
extern int   NewDirForAction(Bloke* b, unsigned char dir);   /* 0x004833d0 */
extern void  RemoveBlokeFromRide(ObjDef* cls, RideInst* r);  /* 0x0048a100 */

/* Steps every boat one place along its route (every 0x50 ticks). */
extern void  JungleCruise_AdvanceBoats(void);                /* 0x004332f0 */
/* Re-lays the river tiles / boat sprites for the frame. */
extern void  JungleCruise_UpdateRiverAnim(int mode);         /* 0x00432d00 */
/* Stamps the station's map square onto every water record and returns how
 * many of them there are (0 when the station's square is 0). */
extern int   JungleCruise_CountStationBoats(JcStation* st); /* 0x004332c0 */
/* Puts a party of up to three riders onto the water at the station. */
extern int   JungleCruise_TryLaunchBoat(BPosW key, Bloke* a, Bloke* b, Bloke* c); /* 0x00432b90 */

extern int    g_jc_anim_tick;        /* 0x00629c54 */
/* Seat pixel offsets, indexed BACKWARDS: g_jc_seat_ofs[-slot]. */
extern SeatOfs g_jc_seat_ofs[];      /* 0x004b72b0 */

/* 354/354 instructions, 1109 of 1114 bytes; mismatch 229 -> 208 (audit.py).
 * TWO SEMANTIC FIXES from an earlier pass, still standing:
 *  - case 1's "find a free boat seat" loop exits straight to the case end
 *    when no seat is free (the original threads the `i == 3` test away), and
 *  - case 3's walk target is built from the INSTANCE's map square (`key`,
 *    the two adjacent bytes at [esp+0x14]) and not from the screen point the
 *    3D person was converted from -- the original reads a byte PAIR there,
 *    which a Pos of two ints cannot produce.
 * Also: the call to ScreenToMapRef2 passes a THIRD argument the two-parameter
 * callee never reads (the original pushes a zero for it, and the merged
 * `add esp,0x3c` counts 15 pushes in that case, not 14).
 *
 * WHAT CLOSED 21 OF THE RESIDUAL THIS PASS -- two source-order levers, both
 * semantics-preserving and both measured against ~200 rejected variants:
 *  - `b = inst->bloke;` moved AHEAD of the station-search loop.  That alone
 *    puts the function-wide constant zero in EBX and the instance cursor in
 *    EBP, which is how the original has them; with `b` written after the
 *    search (the obvious spelling) the two are swapped and every `cmp r,0`,
 *    `push 0` and zero store in the function is off by a register.  All 24
 *    permutations of the four head assignments x 3 positions relative to the
 *    search were measured: the placement of `b` is the ONLY knob that flips
 *    it (assigning `inst` late flips it too, but then the `mov ebp,[eax+0xcc]`
 *    cannot stay in the prologue).  The first 110 instructions now match.
 *  - case 3's `b->flags &= ~0x80` written AFTER the two world stores.
 *
 * WHAT IS LEFT.  First divergence: index 110, the loop-2 head.  The original
 * allocates its three temps key->EAX(ax), st->ECX, next->EDX; we get
 * st->EAX and next/key sharing ECX.  Everything after it is knock-on:
 *   * the switch selector and `seat` land in ECX where the original uses EAX;
 *   * case 0's target arithmetic therefore runs one step out of phase -- the
 *     key.b.y read is scheduled BEFORE the seat-table dx load instead of
 *     after it, so the dx load takes EDI and the dy load takes EAX where the
 *     original re-uses its serial ECX for dx and spends the dead EBP on dy;
 *   * the final `add` then accumulates into the table value instead of into
 *     the shift result, so b->ty ends up in EAX, and the `mov ecx, edx` the
 *     original needs to free EDX for the b->tx reload never appears.  The
 *     body is exactly ONE instruction short because of that copy, which is
 *     why cases 1/3/4/5 are all off by one index.  Comparing our tail
 *     shifted by one leaves only 32 of 148 tail instructions wrong.
 *
 * THE ROOT CAUSE, located by probe: it is case 1's `for (i = 0; i < 3; i++)`
 * scan of the boat seats.  Stub that loop out (keeping its guard) and `st`
 * moves to ECX immediately; keep the loop and drop the guard and it stays in
 * EAX.  `st` and case 1's `i` interfere and VC6 colours them the other way
 * round from the original -- our `st` wins EAX, the original's `i` does.
 *
 * Ruled out (all measured, all identical or worse).  Head: every permutation
 * of the four head assignments and of moving `next`/`b` past the search;
 * for/while/do-while/rotated/`&&`-condition search shapes; a `static
 * __inline` finder taking BPosW or unsigned short; a separate cursor
 * variable; loop-2-only copies of st/key/next/b/i/seat (a separate `key`
 * moves its frame home and costs 2); block-scope declarations; `switch` on an
 * int or uchar temp; `case 2:`/`default:` added; `while (inst != 0)`,
 * `for(;;)`, `for (; inst; inst = next)` and a `goto tail`; comparing against
 * `inst->key.w`; writing the key or next store through a cast pointer to make
 * the local address-taken.  Case 1: while/`!=`/`continue`/`!p` loop forms, a
 * different index variable (fresh `j`, or `seat`), unsigned counters, and all
 * 60 orderings of the five stores in its body -- the current order is the
 * unique one that reproduces the original's store schedule.  Case 0: a
 * `SeatOfs*` pointer, `int oa[2]`, int temporaries for either table read,
 * `* 256`, operand swaps in both the inner and the outer sum, and
 * `(int)`/`(unsigned char)`/`|0`/`+0`/`&0xff`/byte-pointer spellings of
 * key.b.y.
 *
 * Three spellings DO produce the original's case-0 register plan and the
 * missing `mov ecx, edx` -- `(unsigned char)(key.w >> 8)` for key.b.y (124
 * mismatches), and moving either `b->action = 7` or `b->flags |= 8` down
 * between the b->ty store and the CalcMoveLine call (130) -- but the first
 * costs an extra `xor/mov al` pair, the second is a semantic change the
 * original does not make (its action store is emitted after the call, so it
 * cannot have been before it in the source), the third puts a store the
 * original emits at the top of the block into the middle of it, and all
 * three cost the `mov edi, 0x96` hoist in loop 1.  They are recorded as
 * evidence that the one missing instruction is the whole tail, not adopted.
 * The correct fix is whatever makes case 1's `i` outrank `st` for EAX. */
// WIP-FUNCTION: LEGOLAND 0x00435750  (354/354 insns, 208 mismatches; loop-2 `st` colours to EAX, not ECX)
void JungleCruise_Tick(void)
{
    ObjDef*    def = g_jc_station_cls;
    RideInst*  inst = def->instances;
    RideInst*  next;
    JcStation* st;
    BPosW      key;
    Bloke*     b;
    int        seat;
    int        i;
    Pos        screen;
    Pos        world;

    if (++g_jc_anim_tick == 0x50) {
        g_jc_anim_tick = 0;
        JungleCruise_AdvanceBoats();
    }
    JungleCruise_UpdateRiverAnim(0);

    for (st = g_jc_stations; st; st = st->next) {
        key = st->pos;
        if (st->riders[0] == 0)
            continue;
        if (--st->timer > 0)
            continue;
        if (st->route == 0)
            continue;
        if (st->take <= 6 * JungleCruise_CountStationBoats(st))
            continue;
        if (!JungleCruise_TryLaunchBoat(key, st->riders[0], st->riders[1], st->riders[2]))
            continue;
        st->riders[0]->flags |= 0x80;
        st->riders[0]->stage++;
        BlokeSitAnim(st->riders[0]);
        BlokeSetFrame(st->riders[0], 0);
        if (st->riders[1] != 0) {
            st->riders[1]->flags |= 0x80;
            st->riders[1]->stage++;
            BlokeSitAnim(st->riders[1]);
            BlokeSetFrame(st->riders[1], 0);
        }
        if (st->riders[2] != 0) {
            st->riders[2]->flags |= 0x80;
            st->riders[2]->stage++;
            BlokeSitAnim(st->riders[2]);
            BlokeSetFrame(st->riders[2], 0);
        }
        st->timer = 0x96;
        st->riders[0] = 0;
        st->riders[1] = 0;
        st->riders[2] = 0;
    }

    while (inst) {
        st = g_jc_stations;
        next = inst->next;
        key = inst->key;
        b = inst->bloke;
        while (st) {
            if (st->pos.w == key.w)
                break;
            st = st->next;
        }
        if (b->action == 0) {
            switch (b->stage) {
            case 0:
                seat = 4;
                for (i = 0; i < 5; i++) {
                    if (st->blokes[i] == b) {
                        seat = i;
                        break;
                    }
                }
                if (i == 5) {
                    if (st->count == 5 || st->blokes[4] != 0) {
                        RemoveBlokeFromRide(g_jc_station_cls, inst);
                        break;
                    }
                    st->blokes[seat] = b;
                    st->count++;
                } else {
                    void* front = st->blokes[seat - 1];
                    b = (Bloke*)st->blokes[seat];
                    if (front != 0)
                        break;
                    st->blokes[seat - 1] = b;
                    st->blokes[seat] = 0;
                    seat--;
                    if (seat == 0)
                        b->stage++;
                }
                b->flags |= 8;
                b->tx = ((g_jc_station_cls->dx + key.b.x) << 8) + g_jc_seat_ofs[-seat].dx;
                b->ty = ((g_jc_station_cls->dy + key.b.y) << 8) + g_jc_seat_ofs[-seat].dy;
                b->dir8 = (unsigned char)(CalcMoveLine(b->x, b->y, b->tx, b->ty, &b->path) + 0x10);
                b->action = 7;
                NewDirForAction(b, (unsigned char)((b->dir8 >> 5) + 3));
                break;
            case 1:
                if (b != st->blokes[0])
                    break;
                for (i = 0; i < 3; i++) {
                    if (st->riders[i] == 0) {
                        b->x = -0x270f;
                        b->y = -0x270f;
                        st->riders[i] = b;
                        st->count--;
                        st->blokes[0] = 0;
                        break;
                    }
                }
                break;
            case 3:
                screen.x = 0;
                screen.y = 0;
                BlokeWalkAnim(b);
                BlokeSetFrame(b, 0);
                {
                    Person3D* p = Find3DPersonFromBloke(b);
                    AdjustBlokePosition(&screen);
                    screen.x = p->sx - screen.x - 0x10;
                    screen.y = p->sy - screen.y;
                }
                ScreenToMapRef2(&screen, &world, 0);
                b->x = world.x;
                b->y = world.y;
                b->flags &= (unsigned short)~0x80;
                b->speed = 0xa;
                b->tx = (((int)g_jc_station_cls->ox + key.b.x) << 8) - 0x180;
                b->ty = (((int)g_jc_station_cls->oy + key.b.y) << 8) + 0x80;
                b->dir8 = (unsigned char)(CalcMoveLine(b->x, b->y, b->tx, b->ty, &b->path) + 0x10);
                b->action = 7;
                NewDirForAction(b, (unsigned char)((b->dir8 >> 5) + 3));
                b->stage++;
                break;
            case 4:
                b->tx = (((int)g_jc_station_cls->ox + key.b.x) << 8) + 0x80;
                b->ty = (((int)g_jc_station_cls->oy + key.b.y) << 8) + 0x80;
                b->dir8 = (unsigned char)(CalcMoveLine(b->x, b->y, b->tx, b->ty, &b->path) + 0x10);
                b->action = 7;
                NewDirForAction(b, (unsigned char)((b->dir8 >> 5) + 3));
                b->stage++;
                break;
            case 5:
                b->flags &= (unsigned short)~8;
                RemoveBlokeFromRide(g_jc_station_cls, inst);
                break;
            }
        }
        inst = next;
    }
}

/* =========================================================================
 * NOT YET RECONSTRUCTED -- what the remaining three functions of this lane
 * do, read out of the disassembly.  Recorded here so the next pass starts
 * from the behaviour rather than from the opcodes.
 *
 * ------------------------------------------------------------------------
 * 0x004316f0  OctopusCafe_Tick  (OCTOPUS CAFE cb_a8, 404 instructions)
 *
 *   void OctopusCafe_Tick(RideElem* elem);      /  elem->data = the ObjDef
 *
 * The cafe's customer state machine, run once per frame over the class's
 * live instance list (ObjDef +0xcc).  For each instance it takes the visitor
 * (+0x08) and the instance's map square (+0x0c, addressed as `edi` = a
 * two-byte {x,y}), skips it while the visitor's action word (+0x0e) is
 * non-zero, and otherwise switches on the visitor's stage byte (+0x60) --
 * NINETEEN cases, 0..0x12, through a jump table at 0x00431bf8.
 *
 *   0     claim a seat: Get_UserFlags(cell) gives the seat index, which is
 *         stashed in the visitor's +0x36, flag 8 is set, the wait counter
 *         +0x5c is primed to 0x32, and Set_UserFlags writes (seat+1) & 0x1f
 *         back so the next customer takes the following seat.
 *   1,2   count +0x5c down; when it goes negative BuyItem(elem, cell, 1) is
 *         charged and the seat's step list is looked up in the table at
 *         0x004b6a34 (indexed [seat/2*4 + stage]); -1 means "no more steps"
 *         and the stage simply advances.
 *   3..8  walk the visitor along the seat's approach: each step reads a
 *         step record out of the tables at 0x004b6a44 (seat -> chair id),
 *         0x004b6be8 (stage -> table row) and 0x004b6990 (chair -> 24.8
 *         world offset), turns it into a target at (cell.x<<8) + dx - 0x80,
 *         (cell.y<<8) + dy + 0x80, and issues CalcMoveLine + action 7 +
 *         NewDirForAction((dir>>5)+3).
 *   9     sit down: flag +0x63 bit 0, +0x70 = 0xa, BlokeSitAnim + frame 0.
 *   0xa   eat: count +0x5c (0xc8 ticks) down.
 *   0xb   stand up: clear flag 0x100, +0x70 = 0, BlokeWalkAnim, walk back.
 *   0xc.. leave: the step list at 0x004b6a78 is walked BACKWARDS (index
 *         seat/2*4 - stage) until it yields -1, each entry giving another
 *         waypoint; the final case walks to the cafe's own origin
 *         (ObjDef +0x0c / +0x10) and the last case clears flag 8 and calls
 *         RemoveBlokeFromRide.
 *
 * The shared tail at 0x00431aba (CalcMoveLine result -> +0x73, action 7,
 * NewDirForAction) is reached by `goto` from several cases, so the C wants
 * one labelled block rather than a copy per case.
 *
 * ------------------------------------------------------------------------
 * 0x00431d00  OctopusCafe_Draw  (OCTOPUS CAFE cb_b0, 486 instructions)
 *
 *   void OctopusCafe_Draw(RideElem* elem, BPosW bp, int x, int y, ...);
 *
 * The cafe's painter, and the reason the class needs its own: the customers
 * sit INSIDE the building, so the visitors and the building's sprite layers
 * have to be interleaved by depth instead of drawn as two passes.
 *
 *   1. A 0x7c-byte local array (frame +0x14, 31 dwords, zeroed by a rep
 *      stosd) is filled with the visitors of THIS cafe -- every instance
 *      whose map square (+0x0c) equals the drawn square.  A visitor that is
 *      already seated (+0x40 != 0) is filed by its seat index (+0x36);
 *      everyone else is appended to a second list at frame +0x94 and given
 *      a DEPTH BAND in +0x37, computed from its world position as
 *          band = tbl_0x004b6d58[ 11*(x>>8 - cell.x) - (y>>8) + cell.y ]
 *      i.e. a lookup that turns the offset from the cafe's cell into a
 *      painter's-algorithm layer number.
 *   2. The bands are then painted in order 1,2 -> a sprite group -> 3,4 ->
 *      a sprite group -> 5,6 -> ... up to 0xc, each visitor drawn with
 *      IP_RenderBlokeIn3DNow (0x00440010) and each building layer with the
 *      helper at 0x00431c50 (a five-argument PrintSprite wrapper) fed from
 *      the sprite/offset tables at 0x0081cd6c.. and 0x0081cdb8...  One layer
 *      (0x0081cd80) is drawn straight through PrintSprite (0x004853a0).
 *
 * ------------------------------------------------------------------------
 * 0x00430b10  Restaurant2_Draw  (RESTAURANT 2 cb_b0, 530 instructions)
 *
 *   void Restaurant2_Draw(RideElem* elem, BPosW bp, int x, int y, ...);
 *
 * The same idea for the two-storey restaurant, but the interleaving depends
 * on which way the building faces:
 *
 *   1. 0x0042f9d0 resolves the restaurant record for the drawn square; it
 *      carries the facing at +0x18, a style byte at +0x09, a second at
 *      +0x11 and two sprite handles at +0x38/+0x3c.  0x004304e0 does the
 *      per-frame bookkeeping and GetScreenCoordsForObject gives the pixel
 *      origin.
 *   2. A 0x74-byte local array (frame +0x40, 29 dwords, rep-stosd cleared)
 *      collects every instance of the class sitting on this square.
 *   3. The facing (0, 1, 2 or 3) selects one of four painting orders; each
 *      is a straight-line sequence of PrintSprite (0x004853a0) calls for the
 *      building's layers, IP_RenderBlokeIn3DNow for the diners, and the
 *      helpers 0x00441ea0 / 0x00441ec0 / 0x00441ee0 / 0x00442d30 for the
 *      furniture and the seated poses.  Facings 0 and 1 share one sequence.
 *
 * Both painters are long but shallow -- no loops beyond the collect pass and
 * the per-band scans -- so they should reconstruct cleanly once the sprite
 * tables at 0x0081cd6c/0x0081cdb8 and the helper signatures are pinned down.
 * ========================================================================= */
