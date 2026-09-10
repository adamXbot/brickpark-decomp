/* LEGOLAND -- ride callback cluster at 0x0041xxxx: the DRIVING SCHOOL ROADS
 * place/remove pair, four of the BOATING SCHOOL family's handlers, and the
 * nine unowned helpers the two rides share.
 *
 * These are the handlers SetCustomCallbacks (screen.c 0x00452c20) installs
 * into the 0xd0-byte ObjDef callback slots.  They are NOT exported, so every
 * extent below was taken from the disassembly by control flow
 * (tools/audit.py).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere).
 *
 * ------------------------------------------------------------------------
 * WHICH RIDE?  -- read back out of SetCustomCallbacks (screen.c), the six
 * addresses this lane was given are FOUR different classes:
 *
 *   addr        class                    slot       what it really is
 *   0x00414020  DRIVING SCHOOL ROADS     cb_98      Roads_Add
 *   0x00414220  DRIVING SCHOOL ROADS     cb_9c      Roads_Remove
 *               + ZEBRA CROSSING         cb_9c      (the SAME function; one
 *                                                   handler serves both, and
 *                                                   that is the tell for what
 *                                                   a zebra crossing is)
 *   0x0041aee0  BOATING SCHOOL           cb_b8      LoadBoatingSchool
 *   0x0041b2a0  BOATING SCHOOL MERMAID   cb_98      Mermaid_Add
 *   0x0041b8e0  BOATING SCHOOL WATER     cb_98      BsWater_Add
 *   0x0041bd40  BOATING SCHOOL WATER     cb_90      BsWater_CalcCursor
 *
 * so the old CB_<address> names carried no information at all and every one
 * of them is renamed here.  Nine more unowned helpers those six call (or, for
 * the last one, sit beside in the same class block) are matched with them:
 *
 *   0x0041b100  BOATING SCHOOL cb_c0  BoatingSchool_BestTake
 *   0x0041b0d0  BoatingSchool_AddTake      takings +/- for one school
 *   0x0041caa0  BoatingSchool_RebuildRoute re-walk one school's boat route
 *   0x0041c890  BsWater_FindAt             the lake cell at (x, y)
 *   0x00406020  DrivingSchool_AddTake      the driving school's own AddTake
 *   0x00405310  DrivingSchool_ResetPaths   re-walk one school's road circuit
 *   0x00412650  Road_FindStartPiece        the school's kind-6 entrance block
 *   0x004133e0  Road_Delete                free one road block + its pump
 *   0x00411aa0  Pump_FindAt                the pump at (x, y)
 *
 * ------------------------------------------------------------------------
 * WHAT THE CLUSTER SAYS ABOUT THE TWO RIDES
 *
 * 1. A ZEBRA CROSSING IS NOT AN OBJECT.  It is bit 0x10 of a road block's
 *    kind byte (+0x14).  SetCustomCallbacks gives the crossing class the
 *    ROAD's remove handler, which branches on that bit: set means "take the
 *    crossing off this block and refund it, leave the road", clear means
 *    "take the whole block".
 *
 * 2. A ROAD'S "GROUP ID" IS ITS DRIVING SCHOOL.  ridecb5.c found the u16 at
 *    +0x08 of a road record and called it a group id.  Roads_Add hands that
 *    id to 0x00406020, which is the driving school's AddTake walking the
 *    school list at 0x004c11bc -- so the id is the owning school's packed map
 *    square, and the placement rule "you may not join two roads with
 *    different ids" is really "you may not join two different schools".
 *
 * 3. BOTH RIDES RE-WALK A CIRCUIT AFTER EVERY EDIT, THROUGH THE SAME CODE.
 *    DrivingSchool_ResetPaths (roads) and BoatingSchool_RebuildRoute (lake)
 *    are instruction for instruction the same routine: clear the walk mark
 *    (+0x18 in both records) on every cell of this owner, seed a two-global
 *    cursor pair with the circuit's first cell, then step a walker until it
 *    queues nothing more.  Seeing them side by side is what identifies +0x18
 *    as a walk mark and 0x004c11c4/0x004c11c8 and 0x004d8240/0x004d8244 as
 *    the two cursor pairs.
 *
 * 4. THE LAKE AND THE JUNGLE CRUISE'S RIVER ARE ONE PIECE OF CODE WRITTEN
 *    TWICE.  BsWater_Add and BsWater_CalcCursor are line for line the jungle
 *    cruise's JcWater_Add / JcWater_CalcCursor (ridecb7.c), and
 *    LoadBoatingSchool is LoadJungleCruise (ridecb2.c) with one fewer child
 *    list.  Every quirk of one is a quirk of the other.
 *
 * 5. RUNNING COST IS COUNTED IN CELLS.  Placing a lake square or a mermaid
 *    adds one to the school's takings figure and removing one takes it away,
 *    and the same is true of a road block for a driving school.
 * ========================================================================= */

/* ---- shared map/cursor types (same offsets as ridecb5.c / objmap2.c) ---- */
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

/* ---- save-game primitives (LEGOLAND/saveprof.c) ------------------------- */
extern int   SaveGameRead(void* buf, unsigned int n);        /* 0x0047d730 */
extern void* HeapAlloc_w(unsigned int size);                 /* 0x0049e4ff */
extern void* GetBlokePtr(int id);                            /* 0x00482fe0 */

/* =========================================================================
 * THE BOATING SCHOOL'S FOUR SAVED LISTS  (recovered from the loader below)
 *
 * The boating school family keeps four singly linked lists, and the loader
 * restores them in this order.  Each is written as `int32 count` followed by
 * that many RAW record images (pointers included), so the loader re-links
 * `next` itself and re-resolves the bloke pointers from their saved indices
 * through GetBlokePtr.  Record shapes and heads:
 *
 *   list        head        size    next    contents
 *   stations    0x004cc074  0x34    +0x2c   one per placed BOATING SCHOOL
 *                                           (the record ridecb5.c documents),
 *                                           5 queue blokes at +0x18
 *   water       0x004d823c  0x1c    +0x10   one per BOATING SCHOOL WATER cell
 *                                           {own square @0, owner @2}
 *   mermaids    0x004d2164  0x08    +0x04   one per MERMAID
 *                                           {own square @0, owner @2}
 *   boats       0x004cc03c  0x3f4   +0x3f0  the boats themselves; one bloke
 *                                           pointer at +0x3ec
 *
 * It is the jungle cruise's list set with one fewer child class (ridecb2.c:
 * stations 0x44, water 0x1c, fish 0x0c, trees 0x08, boats 0x3f8 with THREE
 * riders).  The two rides were plainly written from one another -- their
 * loaders are the same routine with a different list table.
 * ========================================================================= */

/* One placed boating school (the full field list is in ridecb5.c). */
typedef struct BsStation {
    BPosW              key;         /* +0x00 packed map square */
    unsigned char      ax;          /* +0x02 route start x */
    unsigned char      ay;          /* +0x03 route start y */
    unsigned char      bx;          /* +0x04 route end x */
    unsigned char      by;          /* +0x05 route end y */
    unsigned char      pad06[2];
    void*              route;       /* +0x08 */
    int                frame;       /* +0x0c animation frame */
    int                backwards;   /* +0x10 animation direction */
    int                count;       /* +0x14 visitors queueing */
    void*              q[5];        /* +0x18 the five queue slots */
    struct BsStation*  next;        /* +0x2c */
    int                take;        /* +0x30 accumulated takings */
} BsStation;                        /* 0x34 */

/* One square of boating-school water: its own square and the school's. */
typedef struct BsWater {
    BPosW           pos;            /* +0x00 */
    BPosW           owner;          /* +0x02 the school that owns this cell */
    unsigned char   pad04[4];
    int             f08;            /* +0x08 cleared on the route's first cell */
    unsigned char   pad0c[4];
    struct BsWater* next;           /* +0x10 */
    int             f14;            /* +0x14 cleared with f08 */
    int             f18;            /* +0x18 the route-walk mark */
} BsWater;                          /* 0x1c */

/* One mermaid: same {own, owner} head, nothing else. */
typedef struct BsMermaid {
    BPosW             pos;          /* +0x00 */
    BPosW             owner;        /* +0x02 */
    struct BsMermaid* next;         /* +0x04 */
} BsMermaid;                        /* 0x08 */

/* One boat.  Only the rider slot and the link are touched here. */
typedef struct BsBoat {
    BPosW           pos;            /* +0x00 */
    unsigned char   pad02[0x3ec - 2];
    void*           bloke;          /* +0x3ec the rider (an index in the save) */
    struct BsBoat*  next;           /* +0x3f0 */
} BsBoat;                           /* 0x3f4 */

extern BsStation* g_bs_stations;    /* 0x004cc074 */
extern BsWater*   g_bs_water;       /* 0x004d823c */
extern BsMermaid* g_bs_mermaids;    /* 0x004d2164 */
extern BsBoat*    g_bs_boats;       /* 0x004cc03c */

/* Re-lays the boat route of the school whose map square is `key`.
 * DEFINED at the end of this file. */
void BoatingSchool_RebuildRoute(BPosW key);                  /* 0x0041caa0 */

/* =========================================================================
 * 0x0041aee0 -- LoadBoatingSchool (BOATING SCHOOL cb_b8).
 *
 * Reads the four count-prefixed groups back in order, allocating each record
 * and appending it to its list, re-resolves the saved bloke indices, and
 * finally rebuilds every station's boat route.
 *
 * THE SAME TWO ORIGINAL QUIRKS AS THE JUNGLE CRUISE'S LOADER (ridecb2.c
 * 0x00435ec0) are reproduced verbatim:
 *  - the three leading lists start their append cursor at NULL, so loading
 *    over a non-empty list REPLACES the head and leaks the old chain; the
 *    boat list instead starts its cursor at the CURRENT head, so it appends
 *    onto the first node (overwriting that node's next, not the tail's).
 *  - no SaveGameRead result is ever checked: a truncated save walks off the
 *    end of the stream instead of failing.  The function always returns 1.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0041aee0
int LoadBoatingSchool(void)
{
    unsigned int n;
    BsStation*   st = 0;
    BsWater*     wt = 0;
    BsMermaid*   mm = 0;
    BsBoat*      bt;
    void**       q;
    int          k;

    SaveGameRead(&n, 4);
    while (n-- != 0) {
        if (!st)
            st = g_bs_stations = (BsStation*)HeapAlloc_w(sizeof(BsStation));
        else
            st = st->next = (BsStation*)HeapAlloc_w(sizeof(BsStation));
        SaveGameRead(st, sizeof(BsStation));
        q = st->q;
        k = 5;
        do {
            *q = GetBlokePtr((int)*q);
            q++;
        } while (--k);
    }

    SaveGameRead(&n, 4);
    while (n-- != 0) {
        if (!wt)
            wt = g_bs_water = (BsWater*)HeapAlloc_w(sizeof(BsWater));
        else
            wt = wt->next = (BsWater*)HeapAlloc_w(sizeof(BsWater));
        SaveGameRead(wt, sizeof(BsWater));
    }

    SaveGameRead(&n, 4);
    while (n-- != 0) {
        if (!mm)
            mm = g_bs_mermaids = (BsMermaid*)HeapAlloc_w(sizeof(BsMermaid));
        else
            mm = mm->next = (BsMermaid*)HeapAlloc_w(sizeof(BsMermaid));
        SaveGameRead(mm, sizeof(BsMermaid));
    }

    bt = g_bs_boats;
    SaveGameRead(&n, 4);
    while (n-- != 0) {
        if (!bt)
            bt = g_bs_boats = (BsBoat*)HeapAlloc_w(sizeof(BsBoat));
        else
            bt = bt->next = (BsBoat*)HeapAlloc_w(sizeof(BsBoat));
        SaveGameRead(bt, sizeof(BsBoat));
        bt->bloke = GetBlokePtr((int)bt->bloke);
    }

    for (st = g_bs_stations; st; st = st->next)
        BoatingSchool_RebuildRoute(st->key);

    return 1;
}

/* ---- the edit cursor and its aliased sub-objects ------------------------ */
typedef struct Cursor {
    unsigned char  pad0000[0x1404];
    Pos            origin;          /* +0x1404 */
    int            status;          /* +0x140c */
    int            error;           /* +0x1410 */
    Rect           rect;            /* +0x1414 */
    unsigned char  pad1428[0x1828 - 0x1428];
    unsigned int   flags;           /* +0x1828 */
    int            f182c;           /* +0x182c */
    struct Cursor* next;            /* +0x1830 */
} Cursor;                           /* 0x1834 */

/* The bare four-int rectangle the people test takes (no list link). */
typedef struct WinRect { int left; int top; int right; int bottom; } WinRect;

struct RideInst;
typedef struct ObjDef {
    unsigned char pad00[0x0c];
    int           dx;               /* +0x0c */
    int           dy;               /* +0x10 */
    unsigned char pad14[0x3c - 0x14];
    Rect          rect;             /* +0x3c  class footprint */
    unsigned char pad50[0xc4 - 0x50];
    void*         c4;               /* +0xc4  the class's shared instance */
    unsigned char padc8[0xcc - 0xc8];
    struct RideInst* instances;     /* +0xcc */
} ObjDef;

/* A placed map object; only its class pointer matters here. */
typedef struct MapObj {
    unsigned char pad00[0x0c];
    ObjDef*       cls;              /* +0x0c */
    unsigned char pad10[4];
} MapObj;                           /* 0x14 */

extern Cursor  g_edit_cursor;          /* 0x007febc0 (EditCursor) */
extern Pos     g_edit_cursor_origin;   /* 0x007fffc4 == g_edit_cursor.origin */
extern Rect    g_edit_cursor_rect;     /* 0x007fffd4 == g_edit_cursor.rect */
extern Cursor* g_edit_cursor_next;     /* 0x008003f0 == g_edit_cursor.next */

#ifndef LEGOLAND_PORTABLE
extern void  ScreenToMapRef(int sx, Pos* out, int sy);       /* 0x0045be90 */
#else
extern int ScreenToMapRef(int sx, Pos* out, int sy);       /* 0x0045be90 */
#endif
extern void  DefaultCursor(Cursor* c);                       /* 0x0045a390 */
extern void  ValidateCursor(Cursor* c, ObjDef* cls);         /* 0x0045f810 */
extern int   CursorIsValid(Cursor* c);                       /* 0x0045f4b0 */
extern void  ResetCursorFootprint(Cursor* c);                /* 0x0045f460 */
extern void  SetCursorError(Cursor* c, int code);            /* 0x0045f480 */
extern int   CheckForPeople(WinRect* r);                     /* 0x00485260 */

/* Probes the boating-school lake around (x, y): returns the bitmask of the
 * four arms that exist (1 N, 2 E, 4 S, 8 W, five map cells out) and stores
 * the owning school's map square in *owner. */
extern int   BsWater_Probe(int x, int y, int* owner);        /* 0x0041c690 */

/* The BOATING SCHOOL WATER class's own four preview cursors, one per arm the
 * new square can be joined to (stride 0x1834, the Cursor size). */
extern Cursor g_bs_water_cursors[4];   /* 0x004d2168 */

/* The fixed 5x5 footprint a lake square always takes, whatever the class
 * descriptor says (the original's constant at 0x004b53c0 -- byte for byte
 * the jungle cruise's kJcWaterRect at 0x004b7478). */
static const Rect kBsWaterRect = { -2, -2, 2, 2, 0 };

/* =========================================================================
 * 0x0041bd40 -- BsWater_CalcCursor (BOATING SCHOOL WATER cb_90).
 *
 * The placement-cursor calculator for one lake square, and line for line the
 * jungle cruise's JcWater_CalcCursor (ridecb7.c 0x00436200) with the boating
 * school's probe and cursor array substituted -- the two water classes are
 * one routine written twice.
 *
 * Like its twin it ignores the class footprint and stamps the fixed 5x5
 * block into the edit cursor, because a lake square always covers 5x5 map
 * cells.  It then asks the lake which of the four cardinal arms already
 * exists at (x, y): a square with no arm at all is refused with error 14
 * ("cannot build here"), one laid on top of a visitor with error 4 (or 3
 * when the people test returns 1).  Otherwise every arm that exists gets one
 * of the class's four preview cursors, positioned five cells out in that
 * arm's direction, and the used ones are chained onto the edit cursor so the
 * whole join is drawn under the mouse.
 *
 * The owning school's square that BsWater_Probe reports is DISCARDED here;
 * the local holding it lives in the dead `sy` argument slot.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0041bd40
void BsWater_CalcCursor(MapObj* o, int sx, int sy)
{
    WinRect area;
    int     owner;
    int     count;
    int     found;
    int     people;

    count = 0;
    g_edit_cursor_rect = kBsWaterRect;
    ScreenToMapRef(sx, &g_edit_cursor_origin, sy);
    found = BsWater_Probe(g_edit_cursor_origin.x,
                          g_edit_cursor_origin.y, &owner);
    g_edit_cursor_next = 0;
    if (!found) {
        SetCursorError(&g_edit_cursor, 14);
        return;
    }

    ValidateCursor(&g_edit_cursor, o->cls);
    if (!CursorIsValid(&g_edit_cursor))
        return;

    area.left = g_edit_cursor_rect.left + g_edit_cursor_origin.x;
    area.top = g_edit_cursor_rect.top + g_edit_cursor_origin.y;
    area.right = g_edit_cursor_rect.right + g_edit_cursor_origin.x;
    area.bottom = g_edit_cursor_rect.bottom + g_edit_cursor_origin.y;
    people = CheckForPeople(&area);
    if (people != -1) {
        if (people != 1) {
            DefaultCursor(&g_bs_water_cursors[0]);
            DefaultCursor(&g_bs_water_cursors[1]);
            DefaultCursor(&g_bs_water_cursors[2]);
            DefaultCursor(&g_bs_water_cursors[3]);
            g_bs_water_cursors[0].rect = g_edit_cursor_rect;
            g_bs_water_cursors[1].rect = g_edit_cursor_rect;
            g_bs_water_cursors[2].rect = g_edit_cursor_rect;
            g_bs_water_cursors[3].rect = g_edit_cursor_rect;
            ResetCursorFootprint(&g_bs_water_cursors[0]);
            ResetCursorFootprint(&g_bs_water_cursors[1]);
            ResetCursorFootprint(&g_bs_water_cursors[2]);
            ResetCursorFootprint(&g_bs_water_cursors[3]);
            g_bs_water_cursors[0].flags = 0x2034;
            g_bs_water_cursors[1].flags = 0x2034;
            g_bs_water_cursors[2].flags = 0x2034;
            g_bs_water_cursors[3].flags = 0x2034;

            if (found & 1) {
                g_bs_water_cursors[count].origin.x = g_edit_cursor_origin.x;
                g_bs_water_cursors[count].origin.y = g_edit_cursor_origin.y - 5;
                count++;
            }
            if (found & 2) {
                g_bs_water_cursors[count].origin.x = g_edit_cursor_origin.x + 5;
                g_bs_water_cursors[count].origin.y = g_edit_cursor_origin.y;
                count++;
            }
            if (found & 4) {
                g_bs_water_cursors[count].origin.x = g_edit_cursor_origin.x;
                g_bs_water_cursors[count].origin.y = g_edit_cursor_origin.y + 5;
                count++;
            }
            if (found & 8) {
                g_bs_water_cursors[count].origin.x = g_edit_cursor_origin.x - 5;
                g_bs_water_cursors[count].origin.y = g_edit_cursor_origin.y;
                count++;
            }
            if (count != 0) {
                g_edit_cursor_next = &g_bs_water_cursors[0];
                if (count > 1) {
                    int j;
                    for (j = 1; j < count; j++)
                        g_bs_water_cursors[j - 1].next = &g_bs_water_cursors[j];
                }
            }
        } else {
            SetCursorError(&g_edit_cursor, 3);
        }
    } else {
        SetCursorError(&g_edit_cursor, 4);
    }
}

/* Repaints the 5x5 lake block at (x, y) for the arm mask, and reports the
 * owning school's map square back through `owner` when one is given. */
extern void  BsWater_SetTile(int x, int y, int mask, int* owner); /* 0x0041c4c0 */
/* Fills in the 2x2 inside corners around (x, y) for the school in *owner. */
extern void  BsWater_Relink(int x, int y, int* owner);            /* 0x0041bab0 */
/* Adds `amount` to the takings of the school whose map square is `key`.
 * The callee reads only `cx`, but the caller pushes a full dword, so the
 * parameter is spelled `int` HERE (ridecb5.c declares the same function with
 * a BPosW by value for its own call sites).  DEFINED at the end of this file. */
void  BoatingSchool_AddTake_I(int key, int amount);               /* 0x0041b0d0 */
/* Lays a boat route between two map squares; returns the route record. */
extern void* BoatingSchool_BuildRoute(int x0, int y0, int x1, int y1); /* 0x0041c8c0 */
extern void  IncrementObjectCount(ObjDef* cls);                   /* 0x00480d40 */
/* The SAME routine as BoatingSchool_RebuildRoute above, declared a second
 * time under a second name because the two call sites in this file need
 * two different parameter TYPES to reproduce their pushes: the loader has
 * a BPosW field to hand over (`mov dx,word ptr [esi] / push edx`) while
 * BsWater_Add has the int `owner` local (`mov eax,dword ptr [..] / push
 * eax`).  Both resolve to 0x0041caa0. */
extern void  BoatingSchool_RebuildRouteI(int key);                /* 0x0041caa0 */

/* =========================================================================
 * 0x0041b8e0 -- BsWater_Add (BOATING SCHOOL WATER cb_98).
 *
 * Lays one lake square at the cursor's map cell and re-stitches everything it
 * touches.  It is the jungle cruise's JcWater_Add (ridecb7.c 0x004365f0) with
 * one extra statement -- the school's takings go UP by one per water square,
 * the mirror of BoatingSchoolWater_Remove's `AddTake(owner, -1)` -- so the
 * running cost of a boating school is measured in lake cells.
 *
 * The three-pass idiom the whole lake uses is
 *
 *     mask = BsWater_Probe(x, y, &owner);     which arms exist, and whose
 *     BsWater_SetTile(x, y, mask, &owner);    repaint the 5x5 block
 *     BsWater_Relink(x, y, &owner);           fill the 2x2 inside corners
 *
 * run once for the new square and then once for EACH arm that exists, five
 * cells out (1 = north, 2 = east, 4 = south, 8 = west), because a neighbour's
 * tile has to be repainted too now that it has gained a link.  The neighbour
 * passes hand BsWater_SetTile a NULL owner (they do not need it reported
 * back) but still hand BsWater_Relink the owner left in the local -- so a
 * neighbour's corners are stitched as if they belonged to the new square's
 * school.  That is the original's behaviour.
 *
 * SAME ORIGINAL QUIRK as the jungle cruise: `owner` is overwritten by every
 * arm's own probe, so the station looked up at the end is the one owning the
 * LAST arm probed, not necessarily the new square's.  Reproduced as-is.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0041b8e0
void BsWater_Add(MapObj* o, Pos* p)
{
    BsStation* st = g_bs_stations;
    int        owner;
    int        mask;

    mask = BsWater_Probe(p->x, p->y, &owner);
    BsWater_SetTile(p->x, p->y, mask, &owner);
    IncrementObjectCount(o->cls);
    BoatingSchool_AddTake_I(owner, 1);
    BsWater_Relink(p->x, p->y, &owner);

    if (mask & 1) {
        BsWater_SetTile(p->x, p->y - 5,
            BsWater_Probe(p->x, p->y - 5, &owner), 0);
        BsWater_Relink(p->x, p->y - 5, &owner);
    }
    if (mask & 2) {
        BsWater_SetTile(p->x + 5, p->y,
            BsWater_Probe(p->x + 5, p->y, &owner), 0);
        BsWater_Relink(p->x + 5, p->y, &owner);
    }
    if (mask & 4) {
        BsWater_SetTile(p->x, p->y + 5,
            BsWater_Probe(p->x, p->y + 5, &owner), 0);
        BsWater_Relink(p->x, p->y + 5, &owner);
    }
    if (mask & 8) {
        BsWater_SetTile(p->x - 5, p->y,
            BsWater_Probe(p->x - 5, p->y, &owner), 0);
        BsWater_Relink(p->x - 5, p->y, &owner);
    }

    while (st) {
        if (st->key.w == (unsigned short)owner) {
            int x0 = st->ax;
            int y0 = st->ay;
            int x1 = st->bx;
            int y1 = st->by;
            st->route = BoatingSchool_BuildRoute(x0, y0, x1, y1);
            if (st->route)
                BoatingSchool_RebuildRouteI(owner);
            break;
        }
        st = st->next;
    }
}

/* The water tileset descriptor the tile-laying code quotes its base tile
 * from (the same object ridecb5.c's BoatingSchool_Add uses). */
typedef struct BsTileSet {
    unsigned char   pad00[4];
    unsigned short* codes;          /* +0x04 -> the tileset's code table */
} BsTileSet;

/* The 16-byte sound-source block PlayInstanceOfSample takes by address
 * (ridecb3.c's RideSoundSource). */
typedef struct RideSoundSource {
    int kind;                       /* +0x00  2 = "at this map square" */
    int f04;                        /* +0x04  never written here */
    int x;                          /* +0x08 */
    int y;                          /* +0x0c */
} RideSoundSource;

extern BsTileSet* g_bs_water_tiles;    /* 0x0082adf4 */
extern void*      g_bs_mermaid_sample; /* 0x004b52d4 the splash sample */

extern void  AddBasicObject(void* obj, Pos* pos);                 /* 0x0045efe0 */
extern void  SetMapTile(int x, int y, unsigned short tile);       /* 0x00461780 */
#ifndef LEGOLAND_PORTABLE
extern void  PlayInstanceOfSample(void* def, int a, int b, RideSoundSource* src); /* 0x00496d20 */
#else
extern int PlayInstanceOfSample(void* def, int a, int b, RideSoundSource* src); /* 0x00496d20 */
#endif

/* =========================================================================
 * 0x0041b2a0 -- Mermaid_Add (BOATING SCHOOL MERMAID cb_98).
 *
 * Places a mermaid on the lake.  The record is the smallest in the family --
 * eight bytes of {own square, owning school's square, next} -- and, like a
 * water square, a mermaid costs the school one unit of takings
 * (BoatingSchool_AddTake(owner, +1)).
 *
 * The owning school is found by probing the lake at the mermaid's cell;
 * BsWater_Probe's RETURN VALUE (the arm mask) is thrown away, only the owner
 * it reports through the out-pointer is kept.
 *
 * After the standard object placement the mermaid stamps its own footprint
 * with water tiles, straight off the class rect (+0x3c..+0x48) rather than a
 * fixed rect: the left column gets base+9, the right column base+12, the top
 * row base+10, the bottom row base+11 and the interior the base tile -- then
 * the four corners are overwritten with base+5 (NW), base+8 (NE), base+6 (SW)
 * and base+7 (SE).  That is the same tileset and the same base+N vocabulary
 * BoatingSchool_Add uses (ridecb5.c), so the whole lake is drawn from one
 * 13-entry water tile set.
 *
 * Both loop bounds are re-read from the class rect on every iteration: the
 * SetMapTile call may alias the descriptor as far as VC6 knows, and the
 * original really does reload them.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0041b2a0
void Mermaid_Add(MapObj* o, Pos* p)
{
    RideSoundSource src;
    ObjDef*         cls = o->cls;
    BsMermaid*      m;
    BPosW           key;
    int             owner;
    int             x;
    int             y;

    key.b.x = (unsigned char)p->x;
    key.b.y = (unsigned char)p->y;
    BsWater_Probe(p->x, p->y, &owner);
    m = (BsMermaid*)HeapAlloc_w(8);
    if (m == 0)
        return;

    m->pos.w = key.w;
    m->owner.w = (unsigned short)owner;
    m->next = g_bs_mermaids;
    g_bs_mermaids = m;
    BoatingSchool_AddTake_I(owner, 1);
    AddBasicObject(o, p);

    for (y = cls->rect.top; y <= cls->rect.bottom; y++) {
        for (x = cls->rect.left; x <= cls->rect.right; x++) {
            if (x == cls->rect.left)
                SetMapTile(p->x + x, p->y + y,
                           (unsigned short)(g_bs_water_tiles->codes[0] + 9));
            else if (x == cls->rect.right)
                SetMapTile(p->x + x, p->y + y,
                           (unsigned short)(g_bs_water_tiles->codes[0] + 12));
            else if (y == cls->rect.top)
                SetMapTile(p->x + x, p->y + y,
                           (unsigned short)(g_bs_water_tiles->codes[0] + 10));
            else if (y == cls->rect.bottom)
                SetMapTile(p->x + x, p->y + y,
                           (unsigned short)(g_bs_water_tiles->codes[0] + 11));
            else
                SetMapTile(p->x + x, p->y + y, g_bs_water_tiles->codes[0]);
        }
    }
    SetMapTile(p->x + cls->rect.left, p->y + cls->rect.top,
               (unsigned short)(g_bs_water_tiles->codes[0] + 5));
    SetMapTile(p->x + cls->rect.right, p->y + cls->rect.top,
               (unsigned short)(g_bs_water_tiles->codes[0] + 8));
    SetMapTile(p->x + cls->rect.left, p->y + cls->rect.bottom,
               (unsigned short)(g_bs_water_tiles->codes[0] + 6));
    SetMapTile(p->x + cls->rect.right, p->y + cls->rect.bottom,
               (unsigned short)(g_bs_water_tiles->codes[0] + 7));

    src.kind = 2;
    src.x = p->x;
    src.y = p->y;
    PlayInstanceOfSample(g_bs_mermaid_sample, 1, 1, &src);
}

/* =========================================================================
 * THE DRIVING SCHOOL ROAD MODEL, part 2 (recovered here)
 *
 * ridecb5.c established that roads sit on a FOUR-cell grid -- every record
 * covers a 4x4 block of map squares -- and that the u16 at +0x08 of a record
 * is a "road group id" every piece of one connected road carries.  The two
 * handlers below say what that id actually IS: the owning DRIVING SCHOOL's
 * packed map square.  0x00406020, which the placement handler calls with it,
 * is the driving school's own AddTake -- the exact twin of the boating
 * school's 0x0041b0d0 -- walking the driving-school record list at 0x004c11bc
 * ({key @ +0x00, take @ +0x04, next @ +0x08}) for the record whose key equals
 * the id, and 0x00405310 walks the ROAD list for every record carrying the id
 * and clears +0x18 before re-laying the school's driving paths.  So a road's
 * "group" is the school it belongs to, and that is why the placement cursor
 * (ridecb5.c Roads_CalcCursor) refuses to join two roads whose ids differ:
 * they are two different driving schools.
 *
 * The id is therefore a PACKED {u8 x, u8 y} map square, passed BY VALUE as a
 * two-byte struct -- which is exactly what the code says, since it is stored
 * with a 16-bit `mov word` and then handed to five different routines with a
 * 32-bit `mov ebx, dword ptr [..] / push ebx` that carries two bytes of
 * garbage above it.
 *
 * A road record (ridecb5.c's RoadRec, repeated here for the offsets this
 * file needs): next @ +0x00, school @ +0x08, x @ +0x0c, y @ +0x10, kind
 * @ +0x14 (low nibble; kind 6 is the piece that may not be joined).
 * ========================================================================= */

typedef struct RoadRec {
    struct RoadRec* next;           /* +0x00 */
    int             f04;            /* +0x04 cleared when the paths are re-laid */
    BPosW           school;         /* +0x08 the owning driving school */
    unsigned char   pad0a[2];
    int             x;              /* +0x0c */
    int             y;              /* +0x10 */
    unsigned char   kind;           /* +0x14 low nibble = piece kind */
    unsigned char   f15;            /* +0x15 cleared with f04 on the start piece */
    unsigned char   pad16[2];
    int             f18;            /* +0x18 the path-walk mark */
    unsigned char   pad1c[4];
} RoadRec;                          /* 0x20 (the allocation size at 0x004132a0) */

extern RoadRec* g_road_list;        /* 0x004cbeac (ridecb5.c owns the walkers) */
/* The road-path walker's cursor pair: the piece being walked and the piece
 * the walk queued next. */
extern RoadRec* g_road_walk_cur;    /* 0x004c11c4 */
extern RoadRec* g_road_walk_next;   /* 0x004c11c8 */
/* One step of the driving-school path walk (it consumes g_road_walk_cur and
 * fills g_road_walk_next). */
extern void  DrivingSchool_WalkStep(BPosW school);               /* 0x004051a0 */

/* Collects the four cardinal neighbours that may be JOINED into ring slots
 * 0/2/4/6 and returns how many there were (ridecb5.c owns it). */
extern int   Road_CardinalGroup(int x, int y, RoadRec** ring);   /* 0x00413520 */
/* Fills ring slots 1/3/5/7 with the four diagonal neighbours. */
extern int   Road_FindDiagonals(int x, int y, RoadRec** ring);   /* 0x00413450 */
/* Allocates a 0x20-byte road record for the school, links it on the road
 * list and lays its tiles. */
extern void  NewRoadRecord(BPosW school, int x, int y, int kind, int extra); /* 0x004132a0 */
/* Re-derives the road piece at (x, y) from its neighbours and re-stamps it. */
extern void  Road_Restitch(BPosW school, int x, int y);          /* 0x00413650 */
/* Clears every road record of the school and re-lays its driving paths. */
void  DrivingSchool_ResetPaths(BPosW school);                    /* 0x00405310 DEFINED below */
/* Adds `amount` to the takings of the driving school whose square is
 * `school` -- the driving school's copy of BoatingSchool_AddTake.
 * DEFINED at the end of this file. */
void  DrivingSchool_AddTake(BPosW school, int amount);           /* 0x00406020 */

/* =========================================================================
 * 0x00414020 -- Roads_Add (DRIVING SCHOOL ROADS cb_98).
 *
 * Lays one 4x4 road block.  The block joins whichever driving school its
 * cardinal neighbours already belong to, so the first thing it does is ask
 * Road_CardinalGroup for the joinable cardinals and take the school id off
 * the first one it finds (N, then E, then S, then W).
 *
 * It then creates the record (kind 0), re-stitches its own tile, re-lays the
 * school's driving paths, and re-stitches every one of the EIGHT ring
 * neighbours that belongs to the same school and is not a kind-6 piece --
 * diagonals included, which is why laying a block redraws the whole 3x3 of
 * road blocks around it.  Finally the school's takings go up by one and the
 * class instance count is incremented.
 *
 * ORIGINAL BUG reproduced (the same one ridecb5.c documents in
 * Roads_CalcCursor): when NONE of the four cardinals is set, `school` is
 * never assigned and the code reads its frame home -- here the dead `p`
 * ARGUMENT slot, so a road laid with no neighbour at all is filed under the
 * low 16 bits of the caller's Pos pointer.  Roads_CalcCursor refuses that
 * placement, so it is unreachable through the UI; it is reproduced because
 * the original really does read the uninitialised slot.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00414020
void Roads_Add(MapObj* o, Pos* p)
{
    RoadRec* ring[8];
    BPosW    school;
    int      x = p->x;
    int      y = p->y;

    Road_CardinalGroup(x, y, ring);
    if (ring[0])
        school = ring[0]->school;
    else if (ring[2])
        school = ring[2]->school;
    else if (ring[4])
        school = ring[4]->school;
    else if (ring[6])
        school = ring[6]->school;

    NewRoadRecord(school, x, y, 0, 0);
    Road_Restitch(school, x, y);
    DrivingSchool_ResetPaths(school);
    Road_FindDiagonals(x, y, ring);

    if (ring[0] && ring[0]->school.w == school.w && (ring[0]->kind & 0xf) != 6)
        Road_Restitch(school, x, y - 4);
    if (ring[1] && ring[1]->school.w == school.w && (ring[1]->kind & 0xf) != 6)
        Road_Restitch(school, x + 4, y - 4);
    if (ring[2] && ring[2]->school.w == school.w && (ring[2]->kind & 0xf) != 6)
        Road_Restitch(school, x + 4, y);
    if (ring[3] && ring[3]->school.w == school.w && (ring[3]->kind & 0xf) != 6)
        Road_Restitch(school, x + 4, y + 4);
    if (ring[4] && ring[4]->school.w == school.w && (ring[4]->kind & 0xf) != 6)
        Road_Restitch(school, x, y + 4);
    if (ring[5] && ring[5]->school.w == school.w && (ring[5]->kind & 0xf) != 6)
        Road_Restitch(school, x - 4, y + 4);
    if (ring[6] && ring[6]->school.w == school.w && (ring[6]->kind & 0xf) != 6)
        Road_Restitch(school, x - 4, y);
    if (ring[7] && ring[7]->school.w == school.w && (ring[7]->kind & 0xf) != 6)
        Road_Restitch(school, x - 4, y - 4);

    DrivingSchool_AddTake(school, 1);
    IncrementObjectCount(o->cls);
}

extern int   g_bg_full_update;                                   /* 0x004b9220 (BGFullUpdate) */
/* The ZEBRA CROSSING class -- the crossing is not an object of its own but a
 * FLAG (kind bit 0x10) on the road block it is painted across, which is why
 * this one handler serves both classes' cb_9c slot. */
extern ObjDef* g_zebra_cls;                                      /* 0x0082c678 */

extern void  StandardRemoveObject(MapObj* o, BPosW bp, Cursor* ctx); /* 0x0045f220 */
extern void  DecrementObjectCount(ObjDef* cls);                  /* 0x00480d60 */
extern int   GetObjCost(ObjDef* cls);                            /* 0x00480da0 */
extern void  AddBricks(int amount);                              /* 0x004578a0 */
/* Returns the 4x4 road block whose ORIGIN is exactly (x, y) (ridecb5.c). */
extern RoadRec* Road_FindAt(int x, int y);                       /* 0x004125a0 */
/* Fills ring slots 0/2/4/6 with the four cardinal neighbours. */
extern int   Road_FindCardinals(int x, int y, RoadRec** ring);   /* 0x004135d0 */
/* Frees the road record at (x, y), taking its pump attachment with it.
 * DEFINED at the end of this file. */
void  Road_Delete(int x, int y);                                 /* 0x004133e0 */

/* =========================================================================
 * 0x00414220 -- Roads_Remove (DRIVING SCHOOL ROADS cb_9c *and* ZEBRA
 * CROSSING cb_9c -- SetCustomCallbacks installs this ONE function in both
 * classes, which is the tell for what a zebra crossing really is).
 *
 * A zebra crossing is not an object: it is BIT 0x10 of a road block's kind
 * byte.  So removing "an object" at a road square means two different things
 * and the handler decides by that bit:
 *
 *   bit set   -- the square carries a crossing.  Only the crossing goes:
 *                the road's own class count is put BACK (StandardRemoveObject
 *                has just taken it off), the ZEBRA CROSSING count comes off,
 *                the bit is cleared, the tile is re-stitched and the
 *                crossing's cost is refunded.  The road itself survives.
 *   bit clear -- the road block goes: one unit comes off the owning driving
 *                school's takings, the record (and its pump attachment) is
 *                freed, the school's driving paths are re-laid, and all
 *                EIGHT ring neighbours belonging to the same school that are
 *                not kind-6 pieces are re-stitched, exactly as in Roads_Add.
 *
 * The owning school's square is taken off the record BEFORE the branch,
 * because the bit-clear path frees the record before it needs it -- and it is
 * written straight back INTO THE `bp` PARAMETER, whose last use was the
 * StandardRemoveObject call above.  That is not a cosmetic choice: a separate
 * local lands in the `ctx` home instead (0x0c rather than 0x08 off the frame)
 * and, with that slot repurposed, VC6 also stops hoisting the `ctx` load
 * above the `sub esp` and sinks the BGFullUpdate store two pushes further --
 * four instructions' difference from one assignment.  Reusing the parameter
 * is what the original does.
 *
 * NOTE the missing null check: Road_FindAt can return 0 (nothing is laid at
 * the cursor square) and the very next instruction dereferences it.  In
 * practice cb_9c only runs on a square the map says holds a road, so it does
 * not fire; it is reproduced as the original has it.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00414220
void Roads_Remove(MapObj* o, BPosW bp, Cursor* ctx)
{
    RoadRec* ring[8];
    RoadRec* r;
    int      x = ctx->origin.x;
    int      y = ctx->origin.y;

    g_bg_full_update = 1;
    StandardRemoveObject(o, bp, ctx);
    r = Road_FindAt(x, y);
    bp = r->school;   /* the dead parameter carries it */
    if (r->kind & 0x10) {
        IncrementObjectCount(o->cls);
        DecrementObjectCount(g_zebra_cls);
        r->kind &= ~0x10;
        Road_Restitch(bp, x, y);
        AddBricks(GetObjCost(g_zebra_cls));
    } else {
        DrivingSchool_AddTake(bp, -1);
        Road_Delete(x, y);
        DrivingSchool_ResetPaths(bp);
        Road_FindCardinals(x, y, ring);
        Road_FindDiagonals(x, y, ring);

        if (ring[0] && ring[0]->school.w == bp.w && (ring[0]->kind & 0xf) != 6)
            Road_Restitch(bp, x, y - 4);
        if (ring[1] && ring[1]->school.w == bp.w && (ring[1]->kind & 0xf) != 6)
            Road_Restitch(bp, x + 4, y - 4);
        if (ring[2] && ring[2]->school.w == bp.w && (ring[2]->kind & 0xf) != 6)
            Road_Restitch(bp, x + 4, y);
        if (ring[3] && ring[3]->school.w == bp.w && (ring[3]->kind & 0xf) != 6)
            Road_Restitch(bp, x + 4, y + 4);
        if (ring[4] && ring[4]->school.w == bp.w && (ring[4]->kind & 0xf) != 6)
            Road_Restitch(bp, x, y + 4);
        if (ring[5] && ring[5]->school.w == bp.w && (ring[5]->kind & 0xf) != 6)
            Road_Restitch(bp, x - 4, y + 4);
        if (ring[6] && ring[6]->school.w == bp.w && (ring[6]->kind & 0xf) != 6)
            Road_Restitch(bp, x - 4, y);
        if (ring[7] && ring[7]->school.w == bp.w && (ring[7]->kind & 0xf) != 6)
            Road_Restitch(bp, x - 4, y - 4);
    }
}

/* =========================================================================
 * THE TWO "ADD TAKE" HELPERS  (0x0041b0d0 and 0x00406020)
 *
 * Every ride in this cluster keeps its running income in its own record and
 * every child object (a lake square, a mermaid, a road block) adds or removes
 * one unit of it as it is placed or destroyed.  Both rides use literally the
 * same routine over their own list, so they are matched together here; the
 * shape is a list search whose "found" arm carries a REDUNDANT null test the
 * compiler could not thread away, and whose loop fall-out has its own bare
 * `ret` -- both reproduced.
 *
 *   ride            list head    key    take   next
 *   BOATING SCHOOL  0x004cc074   +0x00  +0x30  +0x2c   (the 0x34 station)
 *   DRIVING SCHOOL  0x004c11bc   +0x00  +0x04  +0x08
 * ========================================================================= */

static __inline BsStation* FindBsStation(BPosW key)
{
    BsStation* st = g_bs_stations;

    while (st) {
        if (st->key.w == key.w)
            return st;
        st = st->next;
    }
    return 0;
}

/* One placed driving school.  Only the three fields the takings walk needs. */
typedef struct DsSchool {
    BPosW            key;           /* +0x00 its map square */
    int              take;          /* +0x04 accumulated takings */
    struct DsSchool* next;          /* +0x08 */
} DsSchool;

extern DsSchool* g_ds_schools;      /* 0x004c11bc */

static __inline DsSchool* FindDsSchool(BPosW key)
{
    DsSchool* s = g_ds_schools;

    while (s) {
        if (s->key.w == key.w)
            return s;
        s = s->next;
    }
    return 0;
}

/* 0x0041b0d0 -- add `amount` to the takings of the boating school whose map
 * square is `key`.  Declared a SECOND time above as BoatingSchool_AddTake_I
 * with an `int` first parameter: BsWater_Add and Mermaid_Add hold the square
 * in an int local and push it whole, which is the same two bytes but a
 * different caller-side load, so the two call sites need the int spelling
 * while the definition needs the two-byte one.  Both are 0x0041b0d0. */
// FUNCTION: LEGOLAND 0x0041b0d0
void BoatingSchool_AddTake(BPosW key, int amount)
{
    BsStation* st = FindBsStation(key);

    /* The original re-tests here even though the search's success path
     * cannot return null; that is the inline's `return 0` arm not being
     * threaded away, and it is reproduced. */
    if (st)
        st->take += amount;
}

/* 0x00406020 -- the driving school's copy of exactly the same routine. */
// FUNCTION: LEGOLAND 0x00406020
void DrivingSchool_AddTake(BPosW school, int amount)
{
    DsSchool* s = FindDsSchool(school);

    if (s)
        s->take += amount;
}

/* =========================================================================
 * 0x00412650 -- Road_FindStartPiece.
 *
 * The first road record of `school` whose piece kind is 6, or 0.  Kind 6 is
 * the piece ridecb5.c's Roads_CalcCursor refuses to build onto except from
 * the west: it is the school's ENTRANCE block, where a lesson starts, which
 * is why the path re-lay below begins its walk from it.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00412650
RoadRec* Road_FindStartPiece(BPosW school)
{
    RoadRec* r = g_road_list;

    while (r) {
        if (r->school.w == school.w && (r->kind & 0xf) == 6)
            return r;
        r = r->next;
    }
    return 0;
}

/* =========================================================================
 * 0x00405310 -- DrivingSchool_ResetPaths.
 *
 * Re-lays one driving school's road paths after the road layout changed.
 * It clears the walk mark (+0x18) on every road block of the school, finds
 * the school's start piece, clears its +0x04/+0x15 and seeds the walk cursor
 * with it, then runs DrivingSchool_WalkStep until the walk queues nothing
 * more.  Roads_Add and Roads_Remove both call it, so every road edit re-walks
 * the whole circuit.
 *
 * ORIGINAL BUG reproduced: Road_FindStartPiece can return 0 -- a school with
 * no kind-6 entrance block yet -- and the next two stores dereference it.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00405310
void DrivingSchool_ResetPaths(BPosW school)
{
    RoadRec* r = g_road_list;
    RoadRec* start;

    while (r) {
        if (r->school.w == school.w)
            r->f18 = 0;
        r = r->next;
    }

    start = Road_FindStartPiece(school);
    start->f15 = 0;
    start->f04 = 0;
    g_road_walk_cur = start;
    g_road_walk_next = 0;
    do {
        DrivingSchool_WalkStep(school);
        g_road_walk_cur = g_road_walk_next;
        g_road_walk_next = 0;
    } while (g_road_walk_cur != 0);
}

/* =========================================================================
 * THE DRIVING SCHOOL PUMPS  (0x00411aa0 -- the list this cluster shares)
 *
 * A "pump" is the DRIVING SCHOOL PUMPS class (screen.c installs 0x00411a10 /
 * 0x00411bf0 / 0x00411c70 for it).  Its records hang off 0x004cbea4 with
 * their map square as two INTS -- x @ +0x04, y @ +0x08, next @ +0x0c -- not
 * the packed pair the rest of the family uses.  A pump belongs to the road
 * block one cell WEST and one cell SOUTH of its own square, which is why
 * Road_Delete looks for it at (block.x - 1, block.y + 1).
 * ========================================================================= */

typedef struct Pump {
    unsigned char pad00[4];
    int           x;                /* +0x04 */
    int           y;                /* +0x08 */
    struct Pump*  next;             /* +0x0c */
} Pump;

extern Pump* g_pump_list;           /* 0x004cbea4 */
/* Tears one pump down (DRIVING SCHOOL PUMPS' own removal path). */
extern void  Pump_Remove(Pump* p);                               /* 0x00411b20 */
extern void  HeapFree_w(void* p);                                /* 0x0049e4d0 */

// FUNCTION: LEGOLAND 0x00411aa0
Pump* Pump_FindAt(int x, int y)
{
    Pump* p = g_pump_list;

    while (p) {
        if (p->x == x && p->y == y)
            return p;
        p = p->next;
    }
    return 0;
}

/* =========================================================================
 * 0x004133e0 -- Road_Delete.
 *
 * Removes the road block whose ORIGIN is (x, y): its pump attachment goes
 * first, then the record is freed and unlinked.
 *
 * Note the order: the record is handed to the heap BEFORE it is unlinked, so
 * the list walk that follows compares against freed memory.  It only ever
 * compares the POINTER (`next` was cached before the free), so it works, but
 * it is a genuine use-after-free in the original and is reproduced.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004133e0
void Road_Delete(int x, int y)
{
    RoadRec* r = Road_FindAt(x, y);
    RoadRec* next;
    RoadRec* prev;
    Pump*    pump;

    if (r) {
        next = r->next;
        pump = Pump_FindAt(r->x - 1, r->y + 1);
        if (pump)
            Pump_Remove(pump);
        HeapFree_w(r);
        if (r == g_road_list) {
            g_road_list = next;
        } else {
            prev = g_road_list;
            while (prev->next != r)
                prev = prev->next;
            prev->next = next;
        }
    }
}

/* Returns the lake record covering the map square, or 0.
 * DEFINED at the end of this file. */
BsWater* BsWater_FindAt(int x, int y);                           /* 0x0041c890 */
/* One step of the boat-route walk (it consumes g_bs_walk_cur and fills
 * g_bs_walk_next) -- the lake's copy of DrivingSchool_WalkStep. */
extern void  BsWater_WalkStep(BPosW key);                        /* 0x0041cb20 */
extern BsWater* g_bs_walk_cur;      /* 0x004d8240 */
extern BsWater* g_bs_walk_next;     /* 0x004d8244 */

/* =========================================================================
 * 0x0041caa0 -- BoatingSchool_RebuildRoute.
 *
 * Re-lays one boating school's boat route, and it is the SAME ROUTINE as
 * DrivingSchool_ResetPaths above with the lake substituted for the road:
 * clear the walk mark on every cell of this owner, find the school, seed the
 * walk with the cell at the school's SECOND dock (+0x04/+0x05) after clearing
 * its +0x08/+0x14, then step the walk until it queues nothing more.  Seeing
 * the two side by side is what identifies +0x18 as a walk mark in both
 * records.  Called from the loader for every station and from BsWater_Add
 * whenever a route is successfully re-laid.
 *
 * ORIGINAL BUG reproduced: the station search's "not found" exit falls into
 * the same block as the "found" one, so a key that matches no station (or an
 * empty list) dereferences a null pointer two instructions later.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0041caa0
void BoatingSchool_RebuildRoute(BPosW key)
{
    BsWater*   w = g_bs_water;
    BsStation* st = g_bs_stations;
    BsWater*   start;

    while (w) {
        if (w->owner.w == key.w)
            w->f18 = 0;
        w = w->next;
    }
    while (st) {
        if (st->key.w == key.w)
            break;
        st = st->next;
    }

    start = BsWater_FindAt(st->bx, st->by);
    start->f08 = 0;
    start->f14 = 0;
    g_bs_walk_cur = start;
    g_bs_walk_next = 0;
    do {
        BsWater_WalkStep(key);
        g_bs_walk_cur = g_bs_walk_next;
        g_bs_walk_next = 0;
    } while (g_bs_walk_cur != 0);
}

/* =========================================================================
 * 0x0041c890 -- BsWater_FindAt.
 *
 * The lake record whose OWN square is (x, y), or 0.  The two int coordinates
 * are packed back down into a two-byte key and the list is matched 16 bits at
 * a time -- and the key is built in the `x` PARAMETER'S OWN STACK SLOT, which
 * is why the function needs no frame at all.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0041c890
BsWater* BsWater_FindAt(int x, int y)
{
    BPosW    key;
    BsWater* w;

    key.b.x = (unsigned char)x;
    key.b.y = (unsigned char)y;
    w = g_bs_water;
    while (w) {
        if (w->pos.w == key.w)
            break;
        w = w->next;
    }
    return w;
}

/* =========================================================================
 * 0x0041b100 -- BoatingSchool_BestTake (BOATING SCHOOL cb_c0).
 *
 * The class's "how well is this ride doing" query: the largest takings figure
 * of any placed boating school.  The second argument filters: when it is
 * non-zero only schools that actually have a laid boat route (+0x08) count,
 * so a half-built school does not flatter the figure.  The first argument is
 * the class and is not read.
 *
 * The DRIVING SCHOOL's cb_c0 at 0x00406050 is the same routine over its own
 * list without the route filter, which is what identifies the slot.
 *
 * Note the split prologue: esi/edi are pushed only on the non-empty-list
 * path, because both live entirely inside the loop.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0041b100
int BoatingSchool_BestTake(ObjDef* cls, int working_only)
{
    BsStation* st = g_bs_stations;
    int        best = 0;

    while (st) {
        int take = st->take;
        if (take > best) {
            if (!working_only || st->route != 0)
                best = take;
        }
        st = st->next;
    }
    return best;
}
