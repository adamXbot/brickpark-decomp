/* LEGOLAND -- the last unnamed handlers of the 0x0040xxxx..0x0042xxxx
 * callback cluster: the BOATING SCHOOL's save/remove pair, the DRIVING
 * SCHOOL's save/load pair, the BOATING SCHOOL MERMAID's update handler and
 * the CHUCK WAGON's activate handler.
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
 * WHICH RIDE?  -- read straight back out of SetCustomCallbacks (screen.c),
 * so none of these is a guess; the six CB_<address> names carried no
 * information and all six are renamed here:
 *
 *   addr        class                    slot     what it really is
 *   0x0041acf0  BOATING SCHOOL           cb_bc    SaveBoatingSchool
 *   0x00406070  DRIVING SCHOOL           cb_b8    LoadDrivingSchool
 *   0x0041a530  BOATING SCHOOL           cb_9c    BoatingSchool_Remove
 *   0x00405e70  DRIVING SCHOOL           cb_bc    SaveDrivingSchool
 *   0x0041b4c0  BOATING SCHOOL MERMAID   cb_90    Mermaid_CalcCursor
 *   0x0042e2a0  CHUCK WAGON              cb_a8    ChuckWagon_Activate
 * ========================================================================= */

/* ---- shared map/cursor types (same offsets as ridecb5.c / ridecb6.c) ---- */
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

/* =========================================================================
 * THE BOATING SCHOOL SAVEGAME  (the write side of ridecb6.c's loader)
 *
 * Four lists, each written as `int32 count` followed by that many RAW record
 * images.  The two lists holding visitor POINTERS are copied into a stack
 * buffer first so the pointers can be replaced by bloke INDICES (GetBlokeNum)
 * without disturbing the live record; the other two go straight out of the
 * heap.  It is SaveJungleCruise (ridecb7.c 0x00435c70) with one fewer child
 * list and one rider per boat instead of three.
 * ========================================================================= */

/* One placed boating school (the full field list is in ridecb5.c). */
typedef struct BsStation {
    BPosW              key;         /* +0x00 packed map square */
    unsigned char      pad02[0x18 - 2];
    void*              q[5];        /* +0x18 the five queue slots */
    struct BsStation*  next;        /* +0x2c */
    int                take;        /* +0x30 accumulated takings */
} BsStation;                        /* 0x34 */

/* One square of lake: its own map square and the school that owns it. */
typedef struct BsWater {
    BPosW           pos;            /* +0x00 */
    BPosW           owner;          /* +0x02 the school this cell belongs to */
    unsigned char   pad04[0x10 - 4];
    struct BsWater* next;           /* +0x10 */
    unsigned char   pad14[0x1c - 0x14];
} BsWater;                          /* 0x1c */

/* One mermaid: the same {own square, owner} head and nothing else. */
typedef struct BsMermaid {
    BPosW             pos;          /* +0x00 */
    BPosW             owner;        /* +0x02 */
    struct BsMermaid* next;         /* +0x04 */
} BsMermaid;                        /* 0x08 */

typedef struct BsBoat {
    BPosW          pos;             /* +0x00 the school it belongs to */
    unsigned char  pad02[0x3ec - 2];
    void*          bloke;           /* +0x3ec the rider */
    struct BsBoat* next;            /* +0x3f0 */
} BsBoat;                           /* 0x3f4 */

extern BsStation* g_bs_stations;    /* 0x004cc074 */
extern BsWater*   g_bs_water;       /* 0x004d823c */
extern BsMermaid* g_bs_mermaids;    /* 0x004d2164 */
extern BsBoat*    g_bs_boats;       /* 0x004cc03c */

extern int SaveGameWrite(const void* buf, unsigned int n); /* 0x0047d760 */
extern int GetBlokeNum(void* bloke);                       /* 0x00482fb0 */

// FUNCTION: LEGOLAND 0x0041acf0
int SaveBoatingSchool(void)
{
    int          n;
    BsStation    st;
    BsBoat       bt;
    BsStation*   sp;
    BsStation*   sq;
    BsWater*     w;
    BsWater*     wq;
    BsMermaid*   m;
    BsMermaid*   mq;
    BsBoat*      b;
    BsBoat*      bq;
    void**       q;
    int          i;

    n = 0;
    for (sq = g_bs_stations; sq; sq = sq->next)
        n++;
    sp = g_bs_stations;
    SaveGameWrite(&n, 4);
    while (n-- != 0) {
        st = *sp;
        q = st.q;
        i = 5;
        do {
            *q = (void*)GetBlokeNum(*q);
            q++;
        } while (--i);
        SaveGameWrite(&st, sizeof(st));
        sp = sp->next;
    }

    n = 0;
    for (wq = g_bs_water; wq; wq = wq->next)
        n++;
    w = g_bs_water;
    SaveGameWrite(&n, 4);
    while (n-- != 0) {
        SaveGameWrite(w, sizeof(*w));
        w = w->next;
    }

    n = 0;
    for (mq = g_bs_mermaids; mq; mq = mq->next)
        n++;
    m = g_bs_mermaids;
    SaveGameWrite(&n, 4);
    while (n-- != 0) {
        SaveGameWrite(m, sizeof(*m));
        m = m->next;
    }

    n = 0;
    for (bq = g_bs_boats; bq; bq = bq->next)
        n++;
    b = g_bs_boats;
    SaveGameWrite(&n, 4);
    while (n-- != 0) {
        bt = *b;
        bt.bloke = (void*)GetBlokeNum(bt.bloke);
        SaveGameWrite(&bt, sizeof(bt));
        b = b->next;
    }

    return 1;
}

/* =========================================================================
 * THE DRIVING SCHOOL SAVEGAME -- the read side (0x00406070, cb_b8).
 *
 * FOUR count-prefixed groups, and reading them back is what fixes the four
 * record sizes of the whole driving-school subsystem:
 *
 *   list      head        size    next    contents
 *   schools   0x004c11bc  0x0c    +0x08   one per placed DRIVING SCHOOL
 *                                         {u16 square, int take, next}
 *   roads     0x004cbeac  0x20    +0x00   one per 4x4 road block
 *   pumps     0x004cbea4  0x10    +0x0c   one per DRIVING SCHOOL PUMPS
 *   cars      0x004c10d4  0xd0    +0x00   the cars, driver pointer @ +0xcc
 *
 * UNLIKE the boating school's and jungle cruise's loaders (which leak
 * whatever was already on the list), this one NULLS each head before its
 * group, so loading over a live game drops the old chains cleanly -- the
 * records themselves still leak, but the lists do not tangle.  The driver
 * pointer of every car is re-resolved from its saved bloke index, and the
 * schools are walked once at the end so each one re-lays its road circuit.
 *
 * As everywhere in this subsystem no SaveGameRead result is checked and the
 * function always returns 1.
 * ========================================================================= */

/* One placed driving school (ridecb6.c's takings walk names the fields). */
typedef struct DsSchool {
    BPosW            key;           /* +0x00 its map square */
    int              take;          /* +0x04 accumulated takings */
    struct DsSchool* next;          /* +0x08 */
} DsSchool;                         /* 0x0c */

/* One 4x4 road block (coaster.c/ridecb5.c own the field list). */
typedef struct RoadTile {
    struct RoadTile* next;          /* +0x00 */
    unsigned char    pad04[4];
    unsigned short   school;        /* +0x08 the owning school's map square */
    unsigned char    pad0a[0x20 - 0x0a];
} RoadTile;                         /* 0x20 */

/* One fuel pump.  ridecb6.c named +0x04/+0x08/+0x0c from Pump_FindAt; the
 * placement handler below fills in the first four bytes as well, so the
 * record is the family's usual {own square, owner square} head followed by
 * the same square again as two INTS. */
typedef struct Pump {
    BPosW         key;              /* +0x00 its own map square */
    BPosW         owner;            /* +0x02 the road group / school it feeds */
    int           x;                /* +0x04 */
    int           y;                /* +0x08 */
    struct Pump*  next;             /* +0x0c */
} Pump;                             /* 0x10 */

/* One driving-school car (coaster.c owns the field list). */
typedef struct SchoolCar {
    struct SchoolCar* next;         /* +0x00 */
    unsigned char     pad04[0xcc - 4];
    void*             bloke;        /* +0xcc the driver */
} SchoolCar;                        /* 0xd0 */

extern DsSchool*  g_ds_schools;     /* 0x004c11bc */
extern RoadTile*  g_road_tiles;     /* 0x004cbeac */
extern Pump*      g_pump_list;      /* 0x004cbea4 */
extern SchoolCar* g_school_cars;    /* 0x004c10d4 */

extern int   SaveGameRead(void* buf, unsigned int n);        /* 0x0047d730 */
extern void* HeapAlloc_w(unsigned int size);                 /* 0x0049e4ff */
extern void* GetBlokePtr(int id);                            /* 0x00482fe0 */

/* Re-walks one school's road circuit (ridecb6.c 0x00405310). */
extern void  DrivingSchool_ResetPaths(BPosW school);         /* 0x00405310 */

// FUNCTION: LEGOLAND 0x00406070
int LoadDrivingSchool(void)
{
    unsigned int n;
    DsSchool*    sp = 0;
    RoadTile*    rp = 0;
    Pump*        pp = 0;
    SchoolCar*   cp = 0;

    g_ds_schools = 0;
    SaveGameRead(&n, 4);
    while (n-- != 0) {
        if (!sp)
            sp = g_ds_schools = (DsSchool*)HeapAlloc_w(sizeof(DsSchool));
        else
            sp = sp->next = (DsSchool*)HeapAlloc_w(sizeof(DsSchool));
        SaveGameRead(sp, sizeof(DsSchool));
    }

    g_road_tiles = 0;
    SaveGameRead(&n, 4);
    while (n-- != 0) {
        if (!rp)
            rp = g_road_tiles = (RoadTile*)HeapAlloc_w(sizeof(RoadTile));
        else
            rp = rp->next = (RoadTile*)HeapAlloc_w(sizeof(RoadTile));
        SaveGameRead(rp, sizeof(RoadTile));
    }

    g_pump_list = 0;
    SaveGameRead(&n, 4);
    while (n-- != 0) {
        if (!pp)
            pp = g_pump_list = (Pump*)HeapAlloc_w(sizeof(Pump));
        else
            pp = pp->next = (Pump*)HeapAlloc_w(sizeof(Pump));
        SaveGameRead(pp, sizeof(Pump));
    }

    g_school_cars = 0;
    SaveGameRead(&n, 4);
    while (n-- != 0) {
        if (!cp)
            cp = g_school_cars = (SchoolCar*)HeapAlloc_w(sizeof(SchoolCar));
        else
            cp = cp->next = (SchoolCar*)HeapAlloc_w(sizeof(SchoolCar));
        SaveGameRead(cp, sizeof(SchoolCar));
        cp->bloke = GetBlokePtr((int)cp->bloke);
    }

    for (sp = g_ds_schools; sp; sp = sp->next)
        DrivingSchool_ResetPaths(sp->key);

    return 1;
}

/* =========================================================================
 * 0x0041a530 -- BoatingSchool_Remove (BOATING SCHOOL cb_9c).
 *
 * Tearing a boating school down is a FOUR-STAGE cascade, and the order is
 * what the function teaches:
 *
 *   1. the standard object remover takes the building itself off the map;
 *   2. every map square inside the class footprint rect (the chained Rect at
 *      0x004cc078) is handed back to the base map, so the water tiles the
 *      add handler stamped are undone;
 *   3. every CHILD object that names this school as its owner is removed
 *      through its OWN class handler -- the lake squares through
 *      BsWater_RemoveOne, the mermaids through Mermaid_Remove.  Both take a
 *      MapObj*, so the function builds a FAKE MapObj on the stack whose only
 *      filled field is the class pointer (+0x0c) and re-points it at each
 *      child class in turn;
 *   4. the station comes off its list, every boat carrying its square is
 *      freed, the visitors are evicted and the record is released.
 *
 * WHAT THE CHILD LOOPS SAY ABOUT THE LISTS.  Both restart from the list HEAD
 * after every successful removal instead of stepping to `next`, because the
 * child's own remove handler unlinks the node under them -- the classic
 * "delete while walking" fix, and it is why removing a school is O(n^2) in
 * the number of lake squares.
 *
 * THE TWO CURSORS.  A lake square is removed against the water class's own
 * scratch cursor (0x0082ae20), whose origin is simply overwritten; a mermaid
 * is removed against the CALLER's cursor, whose origin is saved, overwritten
 * and restored around the call.  The scratch cursor is shared with the jungle
 * cruise's water class (ridecb2.c calls the same address g_jc_water_cursor),
 * which is safe only because no two water classes are ever being torn down
 * at the same time.
 *
 * ORIGINAL BUG reproduced: IncrementObjectCount is called TWICE on the WATER
 * class and never on the mermaid class, so tearing down a school leaves the
 * water class's placed-object count two too high and the mermaid class's one
 * too low.  The second call plainly meant to name g_bs_mermaid_cls.
 * ========================================================================= */

/* The edit cursor (same offsets as ridecb6.c). */
typedef struct Cursor {
    unsigned char  pad0000[0x1404];
    Pos            origin;          /* +0x1404 map square under the mouse */
    unsigned char  pad140c[0x1414 - 0x140c];
    Rect           rect;            /* +0x1414 the footprint being previewed */
    unsigned char  pad1428[0x1828 - 0x1428];
    unsigned int   flags;           /* +0x1828 */
    unsigned char  pad182c[4];
    struct Cursor* next;            /* +0x1830 chained preview cursors */
} Cursor;                           /* 0x1834 */

/* An object-class definition; only the footprint rect is read here. */
typedef struct ObjDef {
    unsigned char pad00[0x18];
    void*         f18;              /* +0x18 the three map-query hooks the */
    void*         f1c;              /* +0x1c driving school installs on the */
    void*         f20;              /* +0x20 "TILES FOR DSCHOOL" class */
    unsigned char pad24[0x3c - 0x24];
    Rect          rect;             /* +0x3c class footprint */
    unsigned char pad50[0xc4 - 0x50];
    void*         c4;               /* +0xc4 the class's shared instance */
    unsigned char padc8[0xd0 - 0xc8];
} ObjDef;                           /* 0xd0 */

/* A placed map object.  The child removers only ever read its class. */
typedef struct MapObj {
    unsigned char  pad00[0x0c];
    ObjDef*        cls;             /* +0x0c */
    unsigned char  pad10[4];
} MapObj;                           /* 0x14 */

/* The class footprint rect (ridecb5.c: 0x004cc078, next -> dock A). */
extern Rect g_bs_footprint;             /* 0x004cc078 */

extern ObjDef* g_bs_water_cls;          /* 0x0082adf0 BOATING SCHOOL WATER */
extern ObjDef* g_bs_mermaid_cls;        /* 0x0082adf8 BOATING SCHOOL MERMAID */
/* The water class's scratch placement cursor (ridecb2.c 0x0082ae20). */
extern Cursor g_bs_water_cursor;        /* 0x0082ae20 */

extern void  StandardRemoveObject(MapObj* o, BPosW bp, Cursor* ctx);  /* 0x0045f220 */
extern void  RestoreBaseMap(int x, int y);                            /* 0x0045da60 */
extern void  IncrementObjectCount(ObjDef* cls);                       /* 0x00480d40 */
extern void  BsWater_RemoveOne(MapObj* o, BPosW bp, Cursor* ctx);     /* 0x0041c620 */
extern void  Mermaid_Remove(MapObj* o, BPosW bp, Cursor* ctx);        /* 0x0041b6f0 */
/* Unlinks one boat from 0x004cc03c and frees it. */
extern void  BsBoat_Destroy(BsBoat* b);                               /* 0x00418f90 */
extern void  RemoveAllBlokesFromRide(ObjDef* cls, BPosW bp);          /* 0x0048a2e0 */
extern void  HeapFree_w(void* p);                                     /* 0x0049e4d0 */

// FUNCTION: LEGOLAND 0x0041a530
void BoatingSchool_Remove(MapObj* o, BPosW bp, Cursor* ctx)
{
    BsBoat*    b;
    BsStation* prev = 0;
    BsStation* st = g_bs_stations;
    MapObj     obj;
    BsWater*   w;
    BsMermaid* mm;
    int        x;
    int        y;
    int        sx;
    int        sy;

    b = g_bs_boats;
    StandardRemoveObject(o, bp, ctx);

    for (y = g_bs_footprint.top; y <= g_bs_footprint.bottom; y++)
        for (x = g_bs_footprint.left; x <= g_bs_footprint.right; x++)
            RestoreBaseMap(ctx->origin.x + x, ctx->origin.y + y);

    while (st->key.w != bp.w) {
        prev = st;
        st = st->next;
        if (!st)
            break;
    }
    if (!st)
        return;

    obj.cls = g_bs_water_cls;
    IncrementObjectCount(g_bs_water_cls);
    /* ORIGINAL BUG: the mermaid class is meant here. */
    IncrementObjectCount(g_bs_water_cls);

    w = g_bs_water;
    while (w) {
        if (w->owner.w == bp.w) {
            g_bs_water_cursor.origin.x = w->pos.b.x;
            g_bs_water_cursor.origin.y = w->pos.b.y;
            BsWater_RemoveOne(&obj, w->pos, &g_bs_water_cursor);
            w = g_bs_water;
        } else {
            w = w->next;
        }
    }

    mm = g_bs_mermaids;
    obj.cls = g_bs_mermaid_cls;
    while (mm) {
        if (mm->owner.w == bp.w) {
            sx = ctx->origin.x;
            sy = ctx->origin.y;
            ctx->origin.x = mm->pos.b.x;
            ctx->origin.y = mm->pos.b.y;
            Mermaid_Remove(&obj, mm->pos, ctx);
            ctx->origin.x = sx;
            ctx->origin.y = sy;
            mm = g_bs_mermaids;
        } else {
            mm = mm->next;
        }
    }

    if (prev)
        prev->next = st->next;
    else
        g_bs_stations = st->next;

    while (b) {
        if (b->pos.w == bp.w) {
            BsBoat_Destroy(b);
            b = g_bs_boats;
        } else {
            b = b->next;
        }
    }

    RemoveAllBlokesFromRide(o->cls, bp);
    HeapFree_w(st);
}

/* =========================================================================
 * 0x00405e70 -- SaveDrivingSchool (DRIVING SCHOOL cb_bc), the write side of
 * the loader above and the same routine as SaveBoatingSchool with a
 * different list table: four groups of `int32 count` + count raw records,
 * each list counted by WALKING it and then walked a SECOND time to write it,
 * so a list that changed between the two walks would desynchronise the
 * stream.  Only the car list carries a pointer, so only the car is copied
 * into a stack buffer to have its driver turned into a bloke index; the
 * other three go straight out of the heap.  No SaveGameWrite result is
 * checked and the function always returns 1.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00405e70
int SaveDrivingSchool(void)
{
    int        n;
    SchoolCar  car;
    DsSchool*  sp;
    DsSchool*  sq;
    RoadTile*  rp;
    RoadTile*  rq;
    Pump*      pp;
    Pump*      pq;
    SchoolCar* cp;
    SchoolCar* cq;

    n = 0;
    for (sq = g_ds_schools; sq; sq = sq->next)
        n++;
    sp = g_ds_schools;
    SaveGameWrite(&n, 4);
    while (n-- != 0) {
        SaveGameWrite(sp, sizeof(*sp));
        sp = sp->next;
    }

    n = 0;
    for (rq = g_road_tiles; rq; rq = rq->next)
        n++;
    rp = g_road_tiles;
    SaveGameWrite(&n, 4);
    while (n-- != 0) {
        SaveGameWrite(rp, sizeof(*rp));
        rp = rp->next;
    }

    n = 0;
    for (pq = g_pump_list; pq; pq = pq->next)
        n++;
    pp = g_pump_list;
    SaveGameWrite(&n, 4);
    while (n-- != 0) {
        SaveGameWrite(pp, sizeof(*pp));
        pp = pp->next;
    }

    n = 0;
    for (cq = g_school_cars; cq; cq = cq->next)
        n++;
    cp = g_school_cars;
    SaveGameWrite(&n, 4);
    while (n-- != 0) {
        car = *cp;
        car.bloke = (void*)GetBlokeNum(car.bloke);
        SaveGameWrite(&car, sizeof(car));
        cp = cp->next;
    }

    return 1;
}

/* =========================================================================
 * 0x0041b4c0 -- Mermaid_CalcCursor (BOATING SCHOOL MERMAID cb_90).
 *
 * The placement-cursor calculator for a mermaid, and it is BsWater_CalcCursor
 * (ridecb6.c 0x0041bd40) with TWO differences that between them say what a
 * mermaid is:
 *
 *  1. it takes its footprint from the CLASS descriptor (o->cls->rect) instead
 *     of the fixed 5x5 block a lake square always covers, so a mermaid is
 *     whatever size its .ODF says;
 *  2. it does NOT run the people test, so a mermaid may be dropped on a
 *     square a visitor is standing on -- lake squares refuse that with error
 *     4 / 3, mermaids do not care.
 *
 * Everything else is the lake's routine verbatim, INCLUDING the arm probe:
 * a mermaid asks BsWater_Probe (0x0041c690) which of the four cardinal lake
 * arms exists at the cursor square and refuses placement with error 14 when
 * none does.  So a mermaid can only ever be placed on boating-school water,
 * and the preview chains one of the class's four cursors onto each existing
 * arm five cells out, exactly as a lake square does.
 *
 * The mermaid class's four preview cursors live at 0x004cc090, stride 0x1834.
 * ========================================================================= */

/* The MERMAID class's own four preview cursors, one per arm (stride 0x1834). */
extern Cursor g_mermaid_cursors[4];     /* 0x004cc090 */

extern Cursor  g_edit_cursor;           /* 0x007febc0 (EditCursor) */
extern Pos     g_edit_cursor_origin;    /* 0x007fffc4 == g_edit_cursor.origin */
extern Rect    g_edit_cursor_rect;      /* 0x007fffd4 == g_edit_cursor.rect */
extern Cursor* g_edit_cursor_next;      /* 0x008003f0 == g_edit_cursor.next */

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
/* Which of the four lake arms exist at (x, y), and whose school they are. */
extern int   BsWater_Probe(int x, int y, int* owner);        /* 0x0041c690 */

// FUNCTION: LEGOLAND 0x0041b4c0
void Mermaid_CalcCursor(MapObj* o, int sx, int sy)
{
    ObjDef* cls;
    int     count;
    int     found;

    count = 0;
    cls = o->cls;
    g_edit_cursor_rect = cls->rect;
    ScreenToMapRef(sx, &g_edit_cursor_origin, sy);
    /* ORIGINAL QUIRK: the probe reports the owning school's map square
     * through its third argument, and this function hands it the address of
     * its own DEAD `o` parameter instead of a local -- the school square is
     * written over the parameter and never read again.  It is harmless, but
     * it is the reason the frame differs from BsWater_CalcCursor's: taking
     * the address of a parameter stops VC6 reusing any parameter slot as a
     * spill home, so `cls` gets a real 4-byte frame slot here. */
    found = BsWater_Probe(g_edit_cursor_origin.x,
                          g_edit_cursor_origin.y, (int*)&o);
    g_edit_cursor_next = 0;
    if (!found) {
        SetCursorError(&g_edit_cursor, 14);
        return;
    }

    ValidateCursor(&g_edit_cursor, cls);
    if (!CursorIsValid(&g_edit_cursor))
        return;

    DefaultCursor(&g_mermaid_cursors[0]);
    DefaultCursor(&g_mermaid_cursors[1]);
    DefaultCursor(&g_mermaid_cursors[2]);
    DefaultCursor(&g_mermaid_cursors[3]);
    g_mermaid_cursors[0].rect = g_edit_cursor_rect;
    g_mermaid_cursors[1].rect = g_edit_cursor_rect;
    g_mermaid_cursors[2].rect = g_edit_cursor_rect;
    g_mermaid_cursors[3].rect = g_edit_cursor_rect;
    ResetCursorFootprint(&g_mermaid_cursors[0]);
    ResetCursorFootprint(&g_mermaid_cursors[1]);
    ResetCursorFootprint(&g_mermaid_cursors[2]);
    ResetCursorFootprint(&g_mermaid_cursors[3]);
    g_mermaid_cursors[0].flags = 0x2034;
    g_mermaid_cursors[1].flags = 0x2034;
    g_mermaid_cursors[2].flags = 0x2034;
    g_mermaid_cursors[3].flags = 0x2034;

    if (found & 1) {
        g_mermaid_cursors[count].origin.x = g_edit_cursor_origin.x;
        g_mermaid_cursors[count].origin.y = g_edit_cursor_origin.y - 5;
        count++;
    }
    if (found & 2) {
        g_mermaid_cursors[count].origin.x = g_edit_cursor_origin.x + 5;
        g_mermaid_cursors[count].origin.y = g_edit_cursor_origin.y;
        count++;
    }
    if (found & 4) {
        g_mermaid_cursors[count].origin.x = g_edit_cursor_origin.x;
        g_mermaid_cursors[count].origin.y = g_edit_cursor_origin.y + 5;
        count++;
    }
    if (found & 8) {
        g_mermaid_cursors[count].origin.x = g_edit_cursor_origin.x - 5;
        g_mermaid_cursors[count].origin.y = g_edit_cursor_origin.y;
        count++;
    }
    if (count != 0) {
        g_edit_cursor_next = &g_mermaid_cursors[0];
        if (count > 1) {
            int j;
            for (j = 1; j < count; j++)
                g_mermaid_cursors[j - 1].next = &g_mermaid_cursors[j];
        }
    }
}

/* =========================================================================
 * 0x0042e2a0 -- ChuckWagon_TickCustomers (CHUCK WAGON cb_a8).
 *
 * The +0xa8 slot is not "activate" for a food stall any more than it is for
 * a western-town shop (westtown2.c makes the same correction): it is the
 * PER-TICK CUSTOMER STATE MACHINE, run once a frame over the class-wide
 * rider list, and its 6 states are the chuck wagon's whole script.  The list
 * node, the visitor record and the shared move tail are the ones westtown2.c
 * documents -- the two subsystems were written from one another.
 *
 * Waypoints are in tiles relative to (class base square + the placement's own
 * square), converted to 24.8 world units:
 *
 *   0  step to (x, y+1) and mark the customer "inside" (flags62 |= 8)
 *   1  step to (x - 0.5, y+1) -- up to the serving hatch
 *   2  face direction 7 (south-west, towards the wagon), advance, and sit
 *      for (rand() & 0x1f) + 4 ticks
 *   3  count that down; when it reaches zero, advance and charge the
 *      visitor for item 0
 *   4  step to (x + 0.5, y + 0.5) -- back out to the middle of the square
 *   5  leave: RemoveBlokeFromRide and clear the "inside" flag
 *
 * ORIGINAL FALL-THROUGH reproduced: case 1 has no `break`, so the tick that
 * walks the visitor to the hatch ALSO runs case 2 -- the script advances TWO
 * steps and the wait timer is seeded on the same tick as the walk.  State 2
 * is therefore only ever entered by falling into it, never by dispatch, and
 * the visitor's action byte (which doubles as the draw's occlusion band)
 * never rests on 2.
 *
 * ORIGINAL BUG reproduced in state 3: `timer--` runs on the tick the state
 * advances as well, so the counter is left at -1 for the next state to
 * inherit -- the same off-by-one westtown.c records for the EXPLORERS
 * INSTITUTE.
 * ========================================================================= */

typedef struct ShopDef ShopDef;

typedef struct ShopElem {
    char*        name;          /* +0x00 */
    char*        image;         /* +0x04 */
    unsigned int type_flags;    /* +0x08 */
    ShopDef*     data;          /* +0x0c */
} ShopElem;

/* The placed object's map square: two bytes, also read as one u16 key. */
typedef union MapSquare {
    unsigned short key;         /* +0x00 */
    struct { unsigned char bx, by; } b;
} MapSquare;

typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;       /* +0x0e  low-level AI state (0 = idle) */
    unsigned char  pad10[0x14];
    Pos            target;      /* +0x24  walk target, 24.8 */
    unsigned char  pad2c[0x58 - 0x2c];
    int            timer;       /* +0x58  per-state countdown */
    int            wait;        /* +0x5c */
    unsigned char  action;      /* +0x60  script step / occlusion band */
    unsigned char  pad61;
    unsigned short flags62;     /* +0x62  8 = using this ride */
    unsigned char  pad64[4];
    Pos            world;       /* +0x68  world position, 24.8 */
    unsigned short f70;         /* +0x70 */
    unsigned char  dir;         /* +0x72 */
    unsigned char  new_dir;     /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14];  /* +0x98  CalcMoveLine scratch */
} Bloke;

typedef struct RiderNode {
    struct RiderNode* next;     /* +0x00 */
    struct RiderNode* prev;     /* +0x04 */
    Bloke*            bloke;    /* +0x08 */
    MapSquare         square;   /* +0x0c  the placement's own map square */
    unsigned short    pad0e;
    void*             person;   /* +0x10 */
} RiderNode;

/* The class's build sprite / layer holder. */
typedef struct Spr {
    unsigned char pad00[0x10];
    unsigned int  flags;        /* +0x10  bit 0x2000 = draw via the +0xb0 slot */
} Spr;

struct ShopDef {
    unsigned char  pad00[0x0c];
    int            base_x;      /* +0x0c  the class's base map square */
    int            base_y;      /* +0x10 */
    int            f14;         /* +0x14  draw-descriptor field 1 */
    int            f18;         /* +0x18  draw-descriptor field 2 */
    unsigned int   flags;       /* +0x1c  0x20 = tick via +0xa8, 0x400 = ask +0xa0 */
    unsigned char  pad20[0x3c - 0x20];
    int            fp_x1;       /* +0x3c  the class footprint rect, four ints */
    int            fp_y1;       /* +0x40 */
    int            fp_x2;       /* +0x44 */
    int            fp_y2;       /* +0x48 */
    unsigned char  pad4c[0x64 - 0x4c];
    Spr*           sprite;      /* +0x64  build sprite / layer holder */
    unsigned char  pad68[0xcc - 0x68];
    RiderNode*     riders;      /* +0xcc  class-wide customer list */
};

extern int    CalcMoveLine(Pos from, Pos to, void* path);            /* 0x00480740 */
extern int    NewDirForAction(Bloke* b, unsigned char dir);          /* 0x004833d0 */
extern void   RemoveBlokeFromRide(ShopDef* def, RiderNode* r);       /* 0x0048a100 */
extern void   BuyItem(ShopElem* elem, MapSquare* at, int which);     /* 0x004539e0 */
extern int    rand(void);                                            /* 0x0049e4b2 (CRT) */

/* The move tail every walking state shares (westtown2.c's ShopGo). */
static __inline void ShopGo(Bloke* b)
{
    unsigned char a;

    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
    b->state = 7;
    b->new_dir = a;
    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
    b->action++;
}

// FUNCTION: LEGOLAND 0x0042e2a0
void ChuckWagon_TickCustomers(ShopElem* elem)
{
    ShopDef*   def = elem->data;
    RiderNode* r;
    RiderNode* next;
    Bloke*     b;
    MapSquare* key;
    int        tx;
    int        ty;

    r = def->riders;
    while (r) {
        next = r->next;
        key = &r->square;
        b = r->bloke;
        tx = key->b.bx + def->base_x;
        ty = key->b.by + def->base_y;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x = tx << 8;
                b->target.y = (ty + 1) << 8;
                ShopGo(b);
                break;
            case 1:
                b->target.x = (tx << 8) - 0x80;
                b->target.y = (ty + 1) << 8;
                ShopGo(b);
                /* NO break: the original falls through into case 2. */
            case 2:
                b->dir = 7;
                b->action++;
                b->timer = (rand() & 0x1f) + 4;
                break;
            case 3:
                if (b->timer == 0) {
                    b->action++;
                    BuyItem(elem, key, 0);
                }
                b->timer--;
                break;
            case 4:
                b->target.x = (tx << 8) + 0x80;
                b->target.y = (ty << 8) + 0x80;
                ShopGo(b);
                break;
            case 5:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8u;
                break;
            }
        }
        r = next;
    }
}

/* =========================================================================
 * THE FOOD SUBSYSTEM'S RESOURCE / SELECT / OVERLAY SLOTS (0x0042e2xx..8xx)
 *
 * The nine classes SetCustomCallbacks installs from this address range --
 * CHUCK WAGON, SHARK CAFE, SHARK CAFE BROLLY, CASTLE BBQ, the three
 * FOODCARTs, OCTOPUS CAFE and RESTAURANT 1/2 -- are one subsystem, and the
 * slots below are the parts of it that are shared or trivial.  As in
 * westtown.c the slot names in the ObjDef are misleading and the same
 * correction applies:
 *
 *   +0xa4  LOAD RESOURCES  -- cache the ObjDef in a module global, OR the
 *                             class flags, arm the build sprite's custom-draw
 *                             bit and load the till sound.
 *   +0xac  FREE RESOURCES  -- the mirror.
 *   +0x8c  SELECT FOR PLACEMENT
 *   +0xb0  DEPTH-SORTED OVERLAY DRAW (not "interact")
 *   +0xa8  PER-TICK CUSTOMER STATE MACHINE (not "activate")
 *
 * TWO THINGS THIS CLUSTER SAYS THAT WESTERN TOWN DOES NOT.
 *
 * 1. THE FOOD CLASSES DO NOT USE THE DRAW-DESCRIPTOR SLOT.  Every western
 *    town shop sets `flags |= 0x420` (0x20 = tick me, 0x400 = ask my +0xa0
 *    for a draw descriptor) and overrides +0xa0.  Every food class here sets
 *    only 0x20 and arms the custom-draw bit (0x2000) straight on the build
 *    sprite, so the render walk builds the descriptor itself.  The one
 *    exception is SHARK CAFE BROLLY, which sets ONLY 0x400 and no 0x20 at
 *    all -- a brolly is scenery: it is drawn through the descriptor slot and
 *    never ticked.
 *
 * 2. ONE SELECT HANDLER SERVES ALL NINE.  0x0042e8d0 takes the element as a
 *    parameter and arms the edit cursor from `elem->data` rather than from a
 *    per-class global, which is why the nine classes can share it -- and it
 *    is the only handler in the whole cluster that does.  The brolly needs
 *    its own copy (0x0042e4c0) only because its own +0xa4 has to be the one
 *    that publishes the ObjDef, so its select reads the global instead.
 * ========================================================================= */

extern void   LoadMoneySFX(void);                               /* 0x00453900 */
extern void   KillMoneySFX(void);                               /* 0x00453930 */
extern void   Load_FXList(void* list, int count);               /* 0x00496dd0 */
extern void   Kill_FXList(void* list, int count);               /* 0x00496e30 */
extern void   SetEditCursorFootPrint(void* footprint);          /* 0x0045f440 */
extern void   IP_RenderBlokeIn3DNow(Bloke* b);                  /* 0x00440010 */
extern Pos    GetScreenCoordsForObject(MapSquare* sq, ShopDef* def); /* 0x00442cc0 */
extern int    LLIDB_FindElement(const char* name, void** out,
                                unsigned int* idx);             /* 0x0047b330 */
extern void*  LLIDB_LoadData(void* elem);                       /* 0x0047d3a0 */
#ifndef LEGOLAND_PORTABLE
extern void   LLIDB_UnLoadData(void* elem);                     /* 0x0047d450 */
#else
extern int LLIDB_UnLoadData(void* elem);                     /* 0x0047d450 */
#endif

extern int      g_edit_changed;                                 /* 0x008119b0 */
extern ObjDef*  g_edit_object;                                  /* 0x008119b8 */

/* =========================================================================
 * 0x0042e8d0 -- Food_SelectForPlacement (+0x8c for ALL NINE food classes).
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x0042e8d0
void Food_SelectForPlacement(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_edit_changed = 1;
    g_edit_object = (ObjDef*)def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->rect);
}

/* ---- CHUCK WAGON -------------------------------------------------------- */
extern ShopDef* g_chuckwagon_def;   /* 0x0081cd44 */

// FUNCTION: LEGOLAND 0x0042e220
void ChuckWagon_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_chuckwagon_def = def;
    def->flags |= 0x20;
    g_chuckwagon_def->sprite->flags |= 0x2000;
    LoadMoneySFX();
}

// FUNCTION: LEGOLAND 0x0042e250
void ChuckWagon_FreeResources(void)
{
    KillMoneySFX();
}

/* 0x0042e260 -- CHUCK WAGON +0xb0.  Draws only the FIRST customer parked on
 * this map square and then calls GetScreenCoordsForObject and THROWS THE
 * RESULT AWAY -- a vestigial call the compiler could not remove because it
 * is not pure.  Contrast Foodcart_DrawOverlay below, which draws every
 * customer on the square and does not make that call at all. */
// FUNCTION: LEGOLAND 0x0042e260
void ChuckWagon_DrawOverlay(ShopElem* elem, int x, int y, MapSquare* sq,
                            void* clip, int mode)
{
    ShopDef*   def = elem->data;
    RiderNode* r = def->riders;

    while (r) {
        if (sq->key == r->square.key) {
            IP_RenderBlokeIn3DNow(r->bloke);
            GetScreenCoordsForObject(sq, def);
            return;
        }
        r = r->next;
    }
}

/* ---- SHARK CAFE --------------------------------------------------------- */
extern ShopDef* g_sharkcafe_def;    /* 0x0081cd18 */

// FUNCTION: LEGOLAND 0x0042e5d0
void SharkCafe_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_sharkcafe_def = def;
    def->flags |= 0x20;
    g_sharkcafe_def->sprite->flags |= 0x2000;
    LoadMoneySFX();
}

// FUNCTION: LEGOLAND 0x0042e600
void SharkCafe_FreeResources(void)
{
    KillMoneySFX();
}

/* ---- THE THREE FOODCARTS ------------------------------------------------ */
extern ShopDef* g_foodcart_drink_def;    /* 0x0081cd14 */
extern ShopDef* g_foodcart_icecream_def; /* 0x0081cde0 */
extern ShopDef* g_foodcart_food_def;     /* 0x0081cd3c */

// FUNCTION: LEGOLAND 0x0042e770
void FoodcartDrink_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_foodcart_drink_def = def;
    def->flags |= 0x20;
    g_foodcart_drink_def->sprite->flags |= 0x2000;
    LoadMoneySFX();
}

// FUNCTION: LEGOLAND 0x0042e7a0
void FoodcartDrink_FreeResources(void)
{
    KillMoneySFX();
}

// FUNCTION: LEGOLAND 0x0042e7b0
void FoodcartIcecream_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_foodcart_icecream_def = def;
    def->flags |= 0x20;
    g_foodcart_icecream_def->sprite->flags |= 0x2000;
    LoadMoneySFX();
}

// FUNCTION: LEGOLAND 0x0042e7e0
void FoodcartIcecream_FreeResources(void)
{
    KillMoneySFX();
}

// FUNCTION: LEGOLAND 0x0042e7f0
void FoodcartFood_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_foodcart_food_def = def;
    def->flags |= 0x20;
    g_foodcart_food_def->sprite->flags |= 0x2000;
    LoadMoneySFX();
}

// FUNCTION: LEGOLAND 0x0042e820
void FoodcartFood_FreeResources(void)
{
    KillMoneySFX();
}

/* 0x0042e830 -- +0xb0 for ALL THREE foodcarts: draw EVERY customer standing
 * on this square, not just the first. */
// FUNCTION: LEGOLAND 0x0042e830
void Foodcart_DrawOverlay(ShopElem* elem, int x, int y, MapSquare* sq,
                          void* clip, int mode)
{
    ShopDef*   def = elem->data;
    RiderNode* r = def->riders;

    while (r) {
        if (sq->key == r->square.key)
            IP_RenderBlokeIn3DNow(r->bloke);
        r = r->next;
    }
}

/* ---- CASTLE BBQ --------------------------------------------------------- */
extern ShopDef* g_castlebbq_def;      /* 0x0081cd0c */
extern Spr*     g_castlebbq_layers;   /* 0x0081cd10 */
/* One-entry FX list: "Dragon BBQ01.wav". */
extern void*    g_castlebbq_fx;       /* 0x004b66e8 */

// FUNCTION: LEGOLAND 0x0042e870
void CastleBbq_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_castlebbq_def = def;
    def->flags |= 0x20;
    g_castlebbq_layers = g_castlebbq_def->sprite;
    g_castlebbq_layers->flags |= 0x2000;
    Load_FXList(&g_castlebbq_fx, 1);
    LoadMoneySFX();
}

// FUNCTION: LEGOLAND 0x0042e8b0
void CastleBbq_FreeResources(void)
{
    Kill_FXList(&g_castlebbq_fx, 1);
    g_castlebbq_def = 0;
    KillMoneySFX();
}

/* ---- SHARK CAFE BROLLY -------------------------------------------------- */
extern ShopDef* g_brolly_def;     /* 0x0081cd38 */
extern void*    g_brolly_elem;    /* 0x0061613c the "BROLLY IMAGES" LLIDB element */
extern void*    g_brolly_data;    /* 0x00616140 its loaded data */

static const char kBrollyImages[] = "BROLLY IMAGES";

// FUNCTION: LEGOLAND 0x0042e460
void Brolly_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_brolly_def = def;
    /* 0x400 only: a brolly is asked for a draw descriptor but never ticked. */
    def->flags |= 0x400;
    if (!LLIDB_FindElement(kBrollyImages, &g_brolly_elem, 0))
        g_brolly_data = LLIDB_LoadData(g_brolly_elem);
}

// FUNCTION: LEGOLAND 0x0042e4b0
void Brolly_FreeResources(void)
{
    LLIDB_UnLoadData(g_brolly_elem);
}

// FUNCTION: LEGOLAND 0x0042e4c0
void Brolly_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = (ObjDef*)g_brolly_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->rect);
}

/* =========================================================================
 * SHARK CAFE BROLLY -- +0x98 add and +0xa0 draw descriptor.
 *
 * A brolly is the only class in this cluster that is pure scenery, and these
 * two slots say how: on placement it rolls a RANDOM variant index and stores
 * it in the MAP CELL's user-flags word, and its draw descriptor slot reads
 * that index back and picks the sprite and the two half-offsets out of the
 * "BROLLY IMAGES" data block.  So the variant survives without any per-object
 * record at all -- the map cell itself is the storage -- and the same map
 * square always draws the same brolly.
 *
 * Both offsets come out of the block SHIFTED RIGHT BY ONE (arithmetic), so
 * the table holds them at double resolution.
 * ========================================================================= */

/* The "BROLLY IMAGES" LLIDB data block: a count and three parallel arrays. */
typedef struct BrollyData {
    int    pad00;
    int    count;               /* +0x04  number of brolly variants */
    void*  sprites;             /* +0x08  void*[] indexed by byte offset */
    void*  ox;                  /* +0x0c  int[],  x offset, doubled */
    void*  oy;                  /* +0x10  int[],  y offset, doubled */
} BrollyData;

/* The subsystem's ONE shared draw descriptor (westtown.c 0x0082c6a0). */
typedef struct DrawDesc {
    void*          sprite;      /* +0x00 */
    int            f04;         /* +0x04 */
    int            f08;         /* +0x08 */
    unsigned short f0c;         /* +0x0c */
    unsigned short pad0e;
    int            f10;         /* +0x10 */
} DrawDesc;

extern DrawDesc g_shop_draw;    /* 0x0082c6a0 */

extern void  AddObjectToMap(void* obj, BPosW sq, int flag);      /* 0x0045dd80 */
extern void  Set_UserFlags(int wx, int wy, int value);           /* 0x00461730 */
extern unsigned short Get_UserFlags(int wx, int wy);             /* 0x00461710 */

// FUNCTION: LEGOLAND 0x0042e500
void Brolly_Add(void* obj, Pos* pos)
{
    BPosW sq;

    sq.b.x = (unsigned char)pos->x;
    sq.b.y = (unsigned char)pos->y;
    AddObjectToMap(obj, sq, 0);
    if (g_brolly_data)
        Set_UserFlags(pos->x << 8, pos->y << 8,
                      rand() % ((BrollyData*)g_brolly_data)->count);
}

// FUNCTION: LEGOLAND 0x0042e560
DrawDesc* Brolly_GetDrawDesc(void* elem, BPosW sq)
{
    BrollyData*  d;
    unsigned int i;

    /* The variant index is a BYTE OFFSET, not an element index: the original
     * scales it once (`shl eax,2`) and uses [base + offset] for all three
     * tables.  Splitting the u16 return, the byte mask and the scale into
     * separate statements is also what keeps VC6's `and eax,0xffff` return
     * widen alive -- folded into one `and eax,0xff` in every one-expression
     * spelling tried. */
    i = Get_UserFlags(sq.b.x << 8, sq.b.y << 8);
    d = (BrollyData*)g_brolly_data;
    i = (i & 0xff) << 2;
    g_shop_draw.sprite = *(void**)((char*)d->sprites + i);
    g_shop_draw.f04 = *(int*)((char*)d->ox + i) >> 1;
    g_shop_draw.f08 = *(int*)((char*)d->oy + i) >> 1;
    g_shop_draw.f10 = 0;
    return &g_shop_draw;
}

/* =========================================================================
 * CASTLE BBQ -- +0x98 add and +0x9c remove.
 *
 * The only class in the cluster with a sound of its own: placing one starts
 * "Dragon BBQ01.wav" looping AT THE MAP SQUARE (a kind-2 sound source is a
 * map position, and the source's object field is left UNINITIALISED -- the
 * original never fills it, and it is reproduced), and removing one fades
 * every sample from that same square out over 200 units.
 * ========================================================================= */

typedef struct FXEntry {
    char* name;                 /* +0x00 */
    int   pad4;                 /* +0x04 */
    void* sample;               /* +0x08  filled in by Load_FXList */
} FXEntry;

/* A map-square sound source (audio3.c's SoundSource). */
typedef struct SoundSource {
    int   kind;                 /* +0x00  2 = a map position */
    void* obj;                  /* +0x04  NEVER SET by this cluster */
    int   x;                    /* +0x08 */
    int   y;                    /* +0x0c */
} SoundSource;

extern FXEntry g_castlebbq_fx_list[1];  /* 0x004b66e8 "Dragon BBQ01.wav" */

#ifndef LEGOLAND_PORTABLE
extern void AddBasicObject(void* obj, Pos* pos);                  /* 0x0045efe0 */
#else
extern void AddBasicObject(void* ll_obj, void* ll_pos, void* ll_ctx);                  /* 0x0045efe0 */
#define AddBasicObject(_a1, _a2) AddBasicObject((_a1), (_a2), 0)
#endif
#ifndef LEGOLAND_PORTABLE
extern void PlayInstanceOfSample(void* sample, int a, int b,
                                 SoundSource* src);               /* 0x00496d20 */
#else
extern int PlayInstanceOfSample(void* sample, int a, int b,
                                 SoundSource* src);               /* 0x00496d20 */
#endif
extern void UnSourceAndFadeAllSamplesFromSource(SoundSource* src,
                                                int fade);        /* 0x00496c80 */

// FUNCTION: LEGOLAND 0x0042e9c0
void CastleBbq_Add(void* obj, Pos* pos)
{
    SoundSource src;

    AddBasicObject(obj, pos);
    src.x = pos->x;
    src.kind = 2;
    src.y = pos->y;
    PlayInstanceOfSample(g_castlebbq_fx_list[0].sample, 1, 1, &src);
}

// FUNCTION: LEGOLAND 0x0042ea10
void CastleBbq_Remove(MapObj* o, BPosW bp, Cursor* ctx)
{
    SoundSource src;

    StandardRemoveObject(o, bp, ctx);
    src.kind = 2;
    src.x = bp.b.x;
    src.y = bp.b.y;
    UnSourceAndFadeAllSamplesFromSource(&src, -200);
}

/* =========================================================================
 * THE BOATING SCHOOL / MERMAID / DRIVING SCHOOL ODDS AND ENDS
 * ========================================================================= */

extern ObjDef* g_bs_cls;            /* 0x0082c658 BOATING SCHOOL */
extern Rect    g_bs_dock_b;         /* 0x004cc048 dock B, next -> 0 */
extern Rect    g_bs_dock_a;         /* 0x004cc060 dock A, next -> dock B */

/* 0x0041a000 -- BOATING SCHOOL +0x8c.  Note it RE-CHAINS the three static
 * footprint rects every time the class is selected (whole -> dock A -> dock
 * B), which is why the placement preview shows the building plus its two
 * jetty cells: the chain is rebuilt rather than assumed. */
// FUNCTION: LEGOLAND 0x0041a000
void BoatingSchool_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_bs_cls;
    DefaultCursor(&g_edit_cursor);
    g_bs_footprint.next = &g_bs_dock_a;
    g_bs_dock_a.next = &g_bs_dock_b;
    SetEditCursorFootPrint(&g_bs_footprint);
}

// FUNCTION: LEGOLAND 0x0041b250
void Mermaid_LoadResources(ShopElem* elem)
{
    g_bs_mermaid_cls = (ObjDef*)elem->data;
}

/* 0x0041b260 -- BOATING SCHOOL MERMAID +0x8c.  The extra `flags |= 8` on the
 * edit cursor is what makes the preview follow the water rather than snap to
 * the build grid. */
// FUNCTION: LEGOLAND 0x0041b260
void Mermaid_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_bs_mermaid_cls;
    DefaultCursor(&g_edit_cursor);
    g_edit_cursor.flags |= 8;
    SetEditCursorFootPrint(&g_bs_mermaid_cls->rect);
}

extern void BasicObjectDCalcCursor(MapObj* o, int a);            /* 0x00480bb0 */

/* 0x0041b6d0 -- BOATING SCHOOL MERMAID +0x94: the secondary cursor slot is
 * the stock handler, forwarded unchanged. */
// FUNCTION: LEGOLAND 0x0041b6d0
void Mermaid_CalcCursor2(MapObj* o, int a)
{
    BasicObjectDCalcCursor(o, a);
}

/* 0x00406050 -- DRIVING SCHOOL +0xc0: the largest takings figure of any
 * placed driving school.  It is BoatingSchool_BestTake (ridecb6.c
 * 0x0041b100) over the school list WITHOUT the boating school's
 * "only count schools with a laid route" filter -- neither argument is read.
 * Note the same split prologue is absent because nothing lives in the loop. */
// FUNCTION: LEGOLAND 0x00406050
int DrivingSchool_BestTake(ObjDef* cls, int working_only)
{
    DsSchool* s = g_ds_schools;
    int       best = 0;

    while (s) {
        int take = s->take;
        if (take > best)
            best = take;
        s = s->next;
    }
    return best;
}

extern ObjDef* g_pump_def;          /* 0x004cbe9c DRIVING SCHOOL PUMPS */
extern ObjDef* g_zebra_def;         /* 0x0082c678 ZEBRA CROSSING */

// FUNCTION: LEGOLAND 0x00411a10
void Pump_LoadResources(ShopElem* elem)
{
    g_pump_def = (ObjDef*)elem->data;
}

// FUNCTION: LEGOLAND 0x00414940
void ZebraCrossing_LoadResources(ShopElem* elem)
{
    g_zebra_def = (ObjDef*)elem->data;
}

/* =========================================================================
 * THE DRIVING SCHOOL'S OWN RESOURCE / DRAW / SELECT SLOTS
 *
 * DRIVING SCHOOL ROADS is the only class in this lane that PATCHES ANOTHER
 * CLASS on the way in.  Its +0xa4 looks up two LLIDB elements by name:
 *
 *   "DSCHOOL LIGHTS"    -- loaded, and its data cached in a module global;
 *                          freed again by the matching +0xac.
 *   "TILES FOR DSCHOOL" -- NOT loaded.  Instead the three function pointers
 *                          at +0x18/+0x1c/+0x20 of ITS object definition are
 *                          overwritten with three road-tile hooks
 *                          (0x00413990 / 0x004139c0 / 0x004139e0).
 *
 * All three hooks take a pair of 24.8 world coordinates, shift them down to
 * map squares and look up the road block covering them (GetRoadRecord,
 * ridecb5.c 0x004125f0).  That is what those three ObjDef slots are: the
 * generic "what does this map square cost / claim it / release it" interface
 * a tile class exposes to the walker, and the driving school swaps in road
 * versions so cars and pedestrians route over roads.
 * ========================================================================= */

/* The road block's 4x4 placement rect and the pump's 1x4 one (.data). */
static const Rect kRoadRect = { 0, 0, 3, 3, 0 };

extern ObjDef* g_roads_def;         /* 0x0082c684 DRIVING SCHOOL ROADS */
extern void*   g_roads_data;        /* 0x0082c680 the loaded "DSCHOOL LIGHTS" */
/* The class's second preview cursor, drawn beside the edit cursor. */
extern Cursor  g_road_cursor2;      /* 0x0082f760 */

static const char kDSchoolLights[]   = "DSCHOOL LIGHTS";
static const char kTilesForDSchool[] = "TILES FOR DSCHOOL";

/* An LLIDB element: only its object definition is touched here. */
typedef struct LLElem {
    unsigned char pad00[0x0c];
    ObjDef*       data;             /* +0x0c */
} LLElem;

/* The three road-tile hooks installed on "TILES FOR DSCHOOL"; all three
 * take 24.8 world coordinates.  Cost returns 2 for "no road here". */
extern void Road_TileCost(int wx, int wy);      /* 0x00413990 */
extern void Road_TileClaim(int wx, int wy);     /* 0x004139c0 */
extern void Road_TileRelease(int wx, int wy);   /* 0x004139e0 */

extern void BuildCursorPtr(Cursor* c, int a, int b);             /* 0x0045f5f0 */

// FUNCTION: LEGOLAND 0x00413a10
void Roads_LoadResources(ShopElem* elem)
{
    LLElem* found;

    g_roads_def = (ObjDef*)elem->data;
    if (!LLIDB_FindElement(kDSchoolLights, (void**)&found, 0))
        g_roads_data = LLIDB_LoadData(found);
    if (!LLIDB_FindElement(kTilesForDSchool, (void**)&found, 0)) {
        ObjDef* tiles = found->data;

        tiles->f18 = (void*)Road_TileCost;
        tiles->f1c = (void*)Road_TileClaim;
        tiles->f20 = (void*)Road_TileRelease;
    }
}

// FUNCTION: LEGOLAND 0x00413a80
void Roads_FreeResources(void)
{
    RoadTile* p = g_road_tiles;
    RoadTile* next;
    LLElem*   found;

    if (!LLIDB_FindElement(kDSchoolLights, (void**)&found, 0))
        LLIDB_UnLoadData(found);
    while (p) {
        next = p->next;
        HeapFree_w(p);
        p = next;
    }
}

/* 0x00413ad0 -- DRIVING SCHOOL ROADS +0x8c.  Two cursors: the edit cursor
 * gets the 4x4 road rect plus the "follow the terrain" bit and build-cursor
 * sprite 0x8f8, and a SECOND cursor at 0x0082f760 gets the same rect and
 * flags 0x34 -- that is the ghost of the block the road will snap to, drawn
 * alongside the one under the mouse. */
// FUNCTION: LEGOLAND 0x00413ad0
void Roads_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_roads_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint((void*)&kRoadRect);
    g_edit_cursor.flags |= 8;
    BuildCursorPtr(&g_edit_cursor, 0x8f8, 0);
    DefaultCursor(&g_road_cursor2);
    g_road_cursor2.rect = kRoadRect;
    g_road_cursor2.flags = 0x34;
}

/* 0x00414830 -- ZEBRA CROSSING +0x8c: the same 4x4 rect, but build-cursor
 * sprite 0 -- a crossing is laid onto an existing road block, so it has no
 * build sprite of its own. */
// FUNCTION: LEGOLAND 0x00414830
void ZebraCrossing_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_zebra_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint((void*)&kRoadRect);
    g_edit_cursor.flags |= 8;
    BuildCursorPtr(&g_edit_cursor, 0, 0);
}

/* 0x00405ad0 -- DRIVING SCHOOL +0xa0: the draw-descriptor override, and
 * instruction for instruction westtown.c's Shop_GetDrawDesc (0x0043a390)
 * over the SAME single shared block at 0x0082c6a0 -- so the driving school
 * and every western-town shop take turns with one descriptor. */
// FUNCTION: LEGOLAND 0x00405ad0
DrawDesc* DrivingSchool_GetDrawDesc(ShopElem* elem, unsigned short arg)
{
    ShopDef* def = elem->data;

    g_shop_draw.sprite = def->sprite;
    g_shop_draw.f04 = def->f14;
    g_shop_draw.f08 = def->f18;
    g_shop_draw.f0c = arg;
    def->sprite->flags |= 0x2000;
    return &g_shop_draw;
}

/* =========================================================================
 * DRIVING SCHOOL PUMPS -- select, place and remove.
 *
 * A pump is not free-standing: its +0x98 refuses to place one unless there
 * is a road block covering the square ONE CELL EAST of it (GetRoadRecord(x +
 * 1, y)), and it copies that road's group id into the new record.  That is
 * what ridecb6.c's "a pump belongs to the road block one cell west and one
 * cell south of its own square" looks like from the placement side, and it
 * pins the last four bytes of the pump record: +0x00 its own square, +0x02
 * the road group, +0x04/+0x08 the same square again as two ints.
 *
 * The remove handler never reads its `o` argument: it finds the pump by map
 * square instead and unbuilds the footprint through the CLASS's shared
 * instance (ObjDef +0xc4).  It also sets the full-background-redraw flag
 * BEFORE it knows whether there is a pump there at all -- reproduced.
 * ========================================================================= */

/* The pump's 1x4 placement rect (.data, beside the road's 4x4). */
static const Rect kPumpRect = { 0, 0, 0, 3, 0 };

extern RoadTile* GetRoadRecord(int x, int y);                    /* 0x004125f0 */
extern Pump*     Pump_FindAt(int x, int y);                      /* 0x00411aa0 */
/* Unlinks one pump from 0x004cbea4 and frees it. */
extern void      Pump_Destroy(Pump* p);                          /* 0x00411ad0 */
extern int       g_bg_full_update;                               /* 0x004b9220 */

// FUNCTION: LEGOLAND 0x00411a20
void Pump_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_pump_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint((void*)&kPumpRect);
    g_edit_cursor.flags |= 8;
    BuildCursorPtr(&g_edit_cursor, 0x8f8, 0);
    DefaultCursor(&g_road_cursor2);
    g_road_cursor2.rect = kPumpRect;
    g_road_cursor2.flags = 0x34;
}

// FUNCTION: LEGOLAND 0x00411bf0
void Pump_Add(void* obj, Pos* pos)
{
    BPosW     sq;
    RoadTile* road;
    Pump*     p;

    sq.b.x = (unsigned char)pos->x;
    sq.b.y = (unsigned char)pos->y;
    road = GetRoadRecord(pos->x + 1, pos->y);
    if (road) {
        AddBasicObject(obj, pos);
        p = (Pump*)HeapAlloc_w(sizeof(Pump));
        p->key.w = sq.w;
        p->owner.w = road->school;
        p->x = sq.b.x;
        p->y = sq.b.y;
        p->next = g_pump_list;
        g_pump_list = p;
    }
}

// FUNCTION: LEGOLAND 0x00411c70
void Pumps_Remove(void* o, BPosW bp, Cursor* ctx)
{
    Pump* p;

    g_bg_full_update = 1;
    p = Pump_FindAt(bp.b.x, bp.b.y);
    if (p) {
        StandardRemoveObject((MapObj*)g_pump_def->c4, bp, ctx);
        Pump_Destroy(p);
    }
}

/* =========================================================================
 * 0x0041b880 -- BsWater_SelectForPlacement (BOATING SCHOOL WATER +0x8c).
 *
 * It copies the module rect at 0x004b53c0 INTO the class descriptor's
 * footprint (ObjDef +0x3c) before arming the cursor.  ridecb6.c calls that
 * rect kBsWaterRect and treats it as the constant 5x5 block; it is in fact
 * WRITABLE and the class's +0xa4 (0x0041b830) adds the class footprint's
 * top-left into it, so the "fixed 5x5" is really "5x5 shifted by whatever
 * the .ODF's own rect starts at", and this handler pushes it back out to the
 * class every time the player picks the tool up.
 * ========================================================================= */

extern Rect g_bs_water_rect;        /* 0x004b53c0 (ridecb6.c's kBsWaterRect) */

// FUNCTION: LEGOLAND 0x0041b880
void BsWater_SelectForPlacement(void)
{
    ObjDef* cls = g_bs_water_cls;

    g_edit_changed = 1;
    g_edit_object = cls;
    cls->rect = g_bs_water_rect;
    DefaultCursor(&g_edit_cursor);
    g_edit_cursor.flags |= 8;
    SetEditCursorFootPrint(&g_edit_object->rect);
}

/* =========================================================================
 * 0x0041b830 -- BsWater_LoadResources (BOATING SCHOOL WATER +0xa4).
 *
 * There is no resource here at all: the handler publishes the ObjDef and
 * then TRANSLATES the module's water rect (0x004b53c0) by the class
 * descriptor's own top-left.  Left and right both move by rect.left and top
 * and bottom both move by rect.top, so the 5x5 block keeps its size and
 * slides to wherever the .ODF puts it.  Because it is an accumulate, not an
 * assign, loading the class twice would translate it twice -- an original
 * quirk, reproduced.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0041b830
void BsWater_LoadResources(ShopElem* elem)
{
    ObjDef* cls = (ObjDef*)elem->data;

    g_bs_water_cls = cls;
    g_bs_water_rect.top += cls->rect.top;
    g_bs_water_rect.left += cls->rect.left;
    g_bs_water_rect.right += cls->rect.left;
    g_bs_water_rect.bottom += cls->rect.top;
}

/* =========================================================================
 * THREE MORE COPIES OF THE SELECT HANDLER, AND FIVE OF THE PLACE HANDLER.
 *
 * BALLOONZ, CAROUSEL and EARTH SLIDE each carry their own byte-for-byte copy
 * of Food_SelectForPlacement's body, differing only in which module global
 * holds the ObjDef -- they cannot share the food classes' one because that
 * one takes the element as a parameter and these read a global.
 *
 * The five "+0x98 place" handlers are likewise one routine written five
 * times: pack the placement's map square into two bytes, put the object on
 * the map, then hand the packed square to the ride's own record allocator.
 * In every one of the five the packed square is homed in the DEAD `pos`
 * argument slot, which is why none of them has a stack frame.
 * ========================================================================= */

extern ObjDef* g_balloonz_def;      /* 0x0081cde4 */
extern ObjDef* g_carousel_def;      /* 0x006160bc (ridecb3.c: g_carousel_item) */
extern ObjDef* g_earthslide_def;    /* 0x006160d0 (ridecb1.c: g_slide_item) */

/* Each ride's "allocate a record for a newly placed one" helper. */
extern void Balloonz_NewRecord(BPosW* sq);      /* 0x0042a8f0 */
extern void Carousel_NewRecord(BPosW* sq);      /* 0x0042bbc0 */
extern void EarthSlide_NewRecord(BPosW* sq);    /* 0x0042cd70 */
extern void Restaurant1_NewRecord(BPosW* sq);   /* 0x0042eec0 */
extern void Restaurant2_NewRecord(BPosW* sq);   /* 0x0042f920 */

// FUNCTION: LEGOLAND 0x0042ba40
void Balloonz_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_balloonz_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->rect);
}

// FUNCTION: LEGOLAND 0x0042c460
void Carousel_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_carousel_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->rect);
}

// FUNCTION: LEGOLAND 0x0042d230
void EarthSlide_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_earthslide_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->rect);
}

// FUNCTION: LEGOLAND 0x0042a950
void Balloonz_Add(void* obj, Pos* pos)
{
    BPosW sq;

    sq.b.x = (unsigned char)pos->x;
    sq.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    Balloonz_NewRecord(&sq);
}

// FUNCTION: LEGOLAND 0x0042c520
void Carousel_Add(void* obj, Pos* pos)
{
    BPosW sq;

    sq.b.x = (unsigned char)pos->x;
    sq.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    Carousel_NewRecord(&sq);
}

// FUNCTION: LEGOLAND 0x0042d2c0
void EarthSlide_Add(void* obj, Pos* pos)
{
    BPosW sq;

    sq.b.x = (unsigned char)pos->x;
    sq.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    EarthSlide_NewRecord(&sq);
}

// FUNCTION: LEGOLAND 0x0042ef10
void Restaurant1_Add(void* obj, Pos* pos)
{
    BPosW sq;

    sq.b.x = (unsigned char)pos->x;
    sq.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    Restaurant1_NewRecord(&sq);
}

// FUNCTION: LEGOLAND 0x0042f9a0
void Restaurant2_Add(void* obj, Pos* pos)
{
    BPosW sq;

    sq.b.x = (unsigned char)pos->x;
    sq.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    Restaurant2_NewRecord(&sq);
}

/* =========================================================================
 * TWO MORE DRAW-DESCRIPTOR OVERRIDES, AND THE "FIND, FREE, UNBUILD, EVICT"
 * REMOVE HANDLER THREE RIDES SHARE.
 *
 * BALLOONZ and CAROUSEL each own a PRIVATE descriptor block (0x00616028 and
 * 0x006160a0) instead of the one shared block at 0x0082c6a0 that the driving
 * school and every western-town shop take turns with -- so those two rides
 * can be asked for a descriptor while another class holds the shared one.
 * The bodies are otherwise identical.
 *
 * The remove handler is the same four steps in BALLOONZ, EARTH SLIDE RIDE
 * and RESTAURANT 2: look the per-square record up by the map square (the
 * finder is handed the ADDRESS OF THE `bp` PARAMETER), free it if there is
 * one, unbuild the footprint and evict everyone still on the ride.  ENTRANCE
 * 1 has the same handler without the record steps, because an entrance keeps
 * no per-placement record at all.
 * ========================================================================= */

extern DrawDesc g_balloonz_draw;    /* 0x00616028 */
extern DrawDesc g_carousel_draw;    /* 0x006160a0 */

// FUNCTION: LEGOLAND 0x0042b2a0
DrawDesc* Balloonz_GetDrawDesc(ShopElem* elem, unsigned short arg)
{
    ShopDef* def = elem->data;

    g_balloonz_draw.sprite = def->sprite;
    g_balloonz_draw.f04 = def->f14;
    g_balloonz_draw.f08 = def->f18;
    g_balloonz_draw.f0c = arg;
    def->sprite->flags |= 0x2000;
    return &g_balloonz_draw;
}

// FUNCTION: LEGOLAND 0x0042c550
DrawDesc* Carousel_GetDrawDesc(ShopElem* elem, unsigned short arg)
{
    ShopDef* def = elem->data;

    g_carousel_draw.sprite = def->sprite;
    g_carousel_draw.f04 = def->f14;
    g_carousel_draw.f08 = def->f18;
    g_carousel_draw.f0c = arg;
    def->sprite->flags |= 0x2000;
    return &g_carousel_draw;
}

/* Per-square record lookup / free, one pair per ride (ridecb1.c names the
 * BALLOONZ finder Balloonz_FindRec and documents its record). */
extern void* Balloonz_FindRec(BPosW* sq);        /* 0x0042a980 */
extern void  Balloonz_FreeRec(void* rec);        /* 0x0042a9b0 */
extern void* EarthSlide_FindRec(BPosW* sq);      /* 0x0042ce20 */
extern void  EarthSlide_FreeRec(void* rec);      /* 0x0042cdc0 */
extern void* Restaurant2_FindRec(BPosW* sq);     /* 0x0042f9d0 */
extern void  Restaurant2_FreeRec(void* rec);     /* 0x0042fa00 */

// FUNCTION: LEGOLAND 0x0042df70
void Entrance1_Remove(MapObj* o, BPosW bp, Cursor* ctx)
{
    StandardRemoveObject(o, bp, ctx);
    RemoveAllBlokesFromRide(o->cls, bp);
}

// FUNCTION: LEGOLAND 0x0042aa10
void Balloonz_Remove(MapObj* o, BPosW bp, Cursor* ctx)
{
    void* rec = Balloonz_FindRec(&bp);

    if (rec)
        Balloonz_FreeRec(rec);
    StandardRemoveObject(o, bp, ctx);
    RemoveAllBlokesFromRide(o->cls, bp);
}

// FUNCTION: LEGOLAND 0x0042d270
void EarthSlide_Remove(MapObj* o, BPosW bp, Cursor* ctx)
{
    void* rec = EarthSlide_FindRec(&bp);

    if (rec)
        EarthSlide_FreeRec(rec);
    StandardRemoveObject(o, bp, ctx);
    RemoveAllBlokesFromRide(o->cls, bp);
}

// FUNCTION: LEGOLAND 0x0042fa40
void Restaurant2_Remove(MapObj* o, BPosW bp, Cursor* ctx)
{
    void* rec = Restaurant2_FindRec(&bp);

    if (rec)
        Restaurant2_FreeRec(rec);
    StandardRemoveObject(o, bp, ctx);
    RemoveAllBlokesFromRide(o->cls, bp);
}

/* ---- two more resource teardowns ---------------------------------------- */

extern void*  g_slide_rin;          /* 0x006160d4 the ride's 3D model */
extern void*  g_slide_spr1;         /* 0x006160d8 */
extern void*  g_slide_spr2;         /* 0x006160e0 */
extern void*  g_slide_anim;         /* 0x006160e4 its 3D rider animation */

extern void*  g_rest1_mask_main;    /* 0x0081cd28 RestMask_Main.lls */
extern void*  g_rest1_mask_1aa;     /* 0x0081cd8c RestMaskLevel1aa.lls */
extern void*  g_rest1_mask_1;       /* 0x0081cd88 RestMaskLevel1.lls */
extern void*  g_rest1_mask_2;       /* 0x0081cd94 RestMaskLevel2.lls */
extern void*  g_rest1_mask_3;       /* 0x0081cd90 RestMaskLevel3.lls */

extern void UnLoadRin(void* rin);                                /* 0x00441cf0 */
extern void UnloadPos(void* anim);                               /* 0x0043f7d0 */
extern void KillSprite(void* sprite);                            /* 0x00497bd0 */

/* 0x0042d1f0 -- EARTH SLIDE RIDE +0xac.  Note it RE-PUBLISHES the ObjDef on
 * the way OUT (the +0xa4 already did), which is harmless but is the
 * original's behaviour. */
// FUNCTION: LEGOLAND 0x0042d1f0
void EarthSlide_FreeResources(ShopElem* elem)
{
    g_earthslide_def = (ObjDef*)elem->data;
    UnLoadRin(g_slide_rin);
    UnloadPos(g_slide_anim);
    KillSprite(g_slide_spr1);
    KillSprite(g_slide_spr2);
}

// FUNCTION: LEGOLAND 0x0042f720
void Restaurant1_FreeResources(void)
{
    KillSprite(g_rest1_mask_main);
    KillSprite(g_rest1_mask_1aa);
    KillSprite(g_rest1_mask_1);
    KillSprite(g_rest1_mask_2);
    KillSprite(g_rest1_mask_3);
    KillMoneySFX();
}
