/* LEGOLAND — work-order creation, assignment, hit-testing and the path-leg
 * request the Build handlers make each tick.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; the field and type names are ours.
 * Each matched function carries its original VA marker on the line above it.
 *
 * These are all UNEXPORTED functions: the names below are ours except where a
 * previously matched file had already invented one, in which case the existing
 * name is kept so the extern declarations across the tree stay consistent.
 *
 * ===========================================================================
 * THE WORK-ORDER LIFECYCLE
 *
 * A work order is a 60-byte (0x3c) record. There are two identical lists, one
 * per worker trade, each a head / tail / count triple:
 *
 *      gardener   head 0x0079a8b0   tail 0x0079a8b4   count 0x0079a8b8
 *      mechanic   head 0x0079a8c0   tail 0x0079a8c4   count 0x0079a8c8
 *
 * and a third, worker-free list of park-funded repairs at 0x0079a8d4.
 *
 * 1. CREATION.  AddGardenerWorkOrder / AddMechanicWorkOrder (0x00499780 /
 *    0x00499830) are the two constructors. Both take the object descriptor,
 *    the map cell it sits on, and a `kind`:
 *
 *        kind 1  a placed object. The footprint is ONE rect, copied by value
 *                out of the object class (ObjClass +0x3c), and nrects = 1.
 *        kind 2  a path / scenery span. The rect list is the caller's own
 *                chain; it is walked, counted, and copied into one malloc'd
 *                array, and the order's (ox,oy) bias is taken from the
 *                descriptor.
 *
 *    The gardener constructor is a thin one: it allocates through the shared
 *    tail-append allocator, fills the record and returns it. The mechanic
 *    constructor additionally REFUSES to build an order once the mechanic
 *    count has reached 0xe1 (225) — the count is a hard cap, and because
 *    ClearAMechanicsWorkList (workorder.c) increments it without ever
 *    balancing, a park that keeps firing mechanics eventually stops
 *    accepting repairs at all.
 *
 * 2. ASSIGNMENT.  A worker with nothing to do calls RunGardenerJob /
 *    RunMechanicJob (0x00499d00 / 0x00499d30). Each picks the UNASSIGNED
 *    order nearest the worker (FindNearestFreeOrder 0x00499be0: squared
 *    distance between the worker's world position >> 8 and the order's map
 *    cell) and hands it to AssignGardenerOrder / AssignMechanicOrder
 *    (0x00499ac0 / 0x00499b60), which:
 *      - park the order in Bloke +0x50,
 *      - compute the walk-to target into Bloke +0x2c/+0x30 (see below),
 *      - back-link the order (worker at +0x1c, assigned = 1 at +0x18),
 *      - and issue the long-term action:
 *              gardener  0x12 (object) / 0x15 (span)
 *              mechanic  0x13 (object) / 0x16 (span)
 *    The walk target for an object order is the CENTRE of the footprint in
 *    24.8 world units: ((left + right) + 2*cell) * 128 on each axis. For a
 *    span order it is the rect's near corner biased by the order's (ox,oy):
 *    x = (ox + cell.x + rect.left) << 8, y = ((cell.y - oy) + rect.bottom) << 8.
 *    The two trades differ only in that the mechanic gets the object centre
 *    from the shared helper GetOrderCentre (0x00499a70), which returns the
 *    8-byte Pos in eax:edx and picks the LONGER footprint axis to halve.
 *    FindFreeGardener / FindFreeMechanic (0x00499c40 / 0x00499ca0) are the
 *    mirror image: nearest IDLE worker (plan 0x10 / 0x11) to a given cell.
 *
 * 3. HIT-TESTING.  The mouse path is WorkerHitOnRide (0x00470270) ->
 *    WorkOrderUnderHit (0x00470410) -> WorkOrderNearHit (0x004704b0).
 *    WorkerHitOnRide converts the cursor to a map cell and decides what
 *    dropping the worker on it means; the two hit tests scan the list of the
 *    trade currently on the mouse (0x007fdffc == 0x307 -> gardener) and
 *    return the order whose cell is exactly under, or nearest to, the hit.
 *
 * 4. RELEASE.  GiveBackGardenerOrder / GiveBackMechanicOrder (0x00499e60 /
 *    0x00499f40) clear `assigned` and MOVE the node to the back of its list,
 *    so a job a worker could not reach goes to the end of the queue rather
 *    than being picked again immediately. Both are no-ops for the node that
 *    is already last (next == 0).
 *
 * 5. DESTRUCTION.  FreeGardenerOrder / FreeMechanicOrder (0x00499e30 /
 *    0x00499eb0) unlink, free the rect array and the node, and decrement the
 *    count. The gardener path goes through the noisy shared unlink helper
 *    (0x00499d60, which DBPrintf's the list state); the mechanic path has the
 *    unlink inlined, including a redundant null test on the predecessor that
 *    the search has already proved non-null.
 *
 * The park-funded list is simpler: AddNoneWorkersRepairOrder (0x0049b690)
 * pushes a node on the FRONT carrying a copy of the rect, the cell, the first
 * charge (amount * 1.5, the pooled float at 0x004ab480) and the per-tick
 * amount itself; FreeRepairOrder (0x0049b6e0) unlinks and frees it.
 *
 * ---------------------------------------------------------------------------
 * WHAT THE ORDER RECORD ACTUALLY HOLDS (corrections to earlier guesses)
 *
 *   +0x04  is NOT the placed object but the CLASS's own LLIDB element
 *          (WClass +0xc4); the class record is that element's parsed data at
 *          +0x0c. Both order constructors and ClearObjFootprint take it.
 *   +0x2c  is a BYTE, set to 1 at creation (workers2.c types it as an int).
 *   +0x28  (oy) is initialised to -1, not 0.
 *   +0x34  and +0x38 are FLOATS, not padding: SetOrderRepairAmount (0x00499760)
 *          writes amount*1.5 to +0x34 and the raw per-tick amount to +0x38.
 *   The park-funded node's +0x24 is likewise the per-tick FLOAT (workorder.c
 *   guessed "owning object"): AddNoneWorkersRepairOrder copies the argument's
 *   bits there with an integer move and the scaled value to +0x20.
 *
 * Signatures that differ from the externs other files declare:
 *   RepairCellTick        returns the new life (declared void in workers2.c).
 *   AddNoneWorkersRepairOrder returns a RepairOrder*, not a WorkOrder*.
 *   FindBrokenCellNear    ignores its first argument (the mechanic) entirely.
 *
 * ---------------------------------------------------------------------------
 * THE PATH / FOOTPRINT HELPERS IN THIS FILE
 *
 * ClearObjFootprint and RefreshObjList both work through the EDIT CURSOR
 * chain (0x007febc0, 0x1834-byte blocks linked by +0x1830, each with a rect
 * run at +0x1414 hanging off the map cell at +0x1404): the class is asked to
 * rebuild the cursor for a tile (WClass +0x90), the chain is walked, and the
 * cells it covers are edited. ClearObjFootprint saves and restores the global
 * cursor around the walk, so it is safe to call mid-edit. RefreshObjList
 * accumulates a DISJOINT rectangle set at 0x00801a80 (add the selected
 * object's footprint, subtract every other cursor's) and re-lays path on
 * whatever is left, then re-tiles the interior of the selected footprint.
 *
 * FindBrokenCellNear searches the 12-offset neighbourhood table at 0x004bff28
 * (cross, then two-cell axials, then diagonals) and reads an object's
 * condition from its ROOT cell (Cell +0x04/+0x05 hold the anchor tile), so any
 * tile of a multi-cell object finds the whole object.
 *
 * ShowMessage indexes the 17-record table at 0x004ba8e0 ({topic, cooldown ms,
 * last shown}); a message only appears if it outranks the one on screen
 * (0x00668960) and its own cooldown has expired.
 * =========================================================================== */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* A "bloke" (worker/visitor); allocation stride 172 (0xac). Only the fields
 * this file touches are named; see workers2.c for the fuller picture. */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00  intrusive list link */
    unsigned char  pad04[8];    /* +0x04..0x0b */
    unsigned short plan;        /* +0x0c  long-term action (plan) */
    unsigned short state;       /* +0x0e  low-level AI state */
    unsigned char  pad10[0x14]; /* +0x10..0x23 */
    Pos            target;      /* +0x24  walk target (24.8 world units) */
    Pos            saved;       /* +0x2c  saved target (the job's tile) */
    unsigned char  pad34[0x1c]; /* +0x34..0x4f */
    struct WorkOrder* order;    /* +0x50  current work order */
    unsigned char  pad54[0x0c]; /* +0x54..0x5f */
    unsigned char  action;      /* +0x60  progress code within the plan */
    unsigned char  pad61[7];    /* +0x61..0x67 */
    Pos            world;       /* +0x68  world position, 24.8 */
} Bloke;

/* A gardener or mechanic work order — calloc(0x3c,1) @ 0x004995d0. */
typedef struct WorkOrder {
    struct WorkOrder* next;     /* +0x00 */
    void*             obj;      /* +0x04  target object descriptor */
    Pos               pos;      /* +0x08  map cell the rects are biased by */
    Rect*             rects;    /* +0x10  malloc'd rect array */
    int               nrects;   /* +0x14 */
    int               assigned; /* +0x18  1 once a worker has taken it */
    Bloke*            worker;   /* +0x1c */
    char              kind;     /* +0x20  1 = object, 2 = path/scenery span */
    unsigned char     pad21[3]; /* +0x21..0x23 */
    int               ox;       /* +0x24  x bias used when kind != 1 */
    int               oy;       /* +0x28  y bias used when kind != 1 */
    unsigned char     f2c;      /* +0x2c  1 at creation (byte) */
    unsigned char     pad2d[3];  /* +0x2d..0x2f */
    int               f30;      /* +0x30 */
    float             charge;   /* +0x34  amount * 1.5 */
    float             amount;   /* +0x38  bricks per repair tick */
} WorkOrder;

/* A park-funded ("none workers") repair order — malloc(0x28) @ 0x0049b690. */
typedef struct RepairOrder {
    struct RepairOrder* next;   /* +0x00 */
    Rect                rect;   /* +0x04  20 bytes, copied by value */
    Pos                 pos;    /* +0x18  the cell the rect is biased by */
    float               amount; /* +0x20  bricks owed right now (amount*1.5) */
    float               rate;   /* +0x24  added back onto `amount` each tick */
} RepairOrder;

/* An edit / destroy cursor block (0x1834 bytes; 0x007febc0 is the edit cursor
 * and 0x00810160 the destroy cursor, each the head of a chain via +0x1830).
 * Layout as objmap2.c established it. */
typedef struct Cursor {
    unsigned short count;        /* +0x0000 outline points used */
    short          px[0x400];    /* +0x0002 */
    short          py[0x400];    /* +0x0802 */
    unsigned char  kind[0x400];  /* +0x1002 */
    unsigned char  pad1402[2];
    Pos            origin;       /* +0x1404 map cell the footprint hangs off */
    int            status;       /* +0x140c */
    int            error;        /* +0x1410 */
    Rect           rect;         /* +0x1414 footprint rect list */
    unsigned char  style;        /* +0x1428 */
    unsigned char  pad1429[0x1828 - 0x1429];
    unsigned int   flags;        /* +0x1828 */
    int            f182c;        /* +0x182c */
    struct Cursor* next;         /* +0x1830 */
} Cursor;

/* The object class record as this file reads it. */
typedef struct WClass {
    unsigned char pad00[0x0c];  /* +0x00..0x0b */
    int           hut_ox;       /* +0x0c  door offset of a staff hut, tiles */
    int           hut_oy;       /* +0x10 */
    unsigned char pad14[8];     /* +0x14..0x1b */
    unsigned int  flags;        /* +0x1c  0x200000 gardener, 0x400000 mechanic */
    unsigned char pad20[0x0c];  /* +0x20..0x2b */
    unsigned char max_cond;     /* +0x2c  full condition (life is max/4) */
    unsigned char pad2d[0x0f];  /* +0x2d..0x3b */
    Rect          rect;         /* +0x3c  footprint rect (chained) */
    unsigned char pad50[0x40];  /* +0x50..0x8f */
    /* Rebuild the global edit cursor with this class's footprint at a tile
     * centre (mode 0x8f8 is what the footprint walk asks for). */
    void          (*build_cursor)(void* elem, Pos* at, int mode); /* +0x90 */
    unsigned char pad94[0x30];  /* +0x94..0xc3 */
    void*         elem;         /* +0xc4  the class's LLIDB element */
} WClass;

/* A placed object as the cell points at it: its class at +0x0c. */
typedef struct MapInst {
    unsigned char pad00[0x0c];  /* +0x00..0x0b */
    WClass*       cls;          /* +0x0c */
} MapInst;

/* An LLIDB element (legoland.h LLElem) as the order constructors see it: the
 * order's `obj` is the CLASS's own element (WClass +0xc4), and +0x0c is that
 * element's parsed data, i.e. the class record itself. The two structs have
 * the same shape on purpose -- both are "something with a class at +0x0c". */
typedef struct ObjElem {
    unsigned char pad00[0x0c];  /* +0x00..0x0b  name / image / type_flags */
    WClass*       cls;          /* +0x0c        parsed class record */
} ObjElem;

/* The map/config record at 0x004bcbf4 — the same object legoland.h calls
 * g_map (export `lpConfig`), with the map extent at +0x14/+0x16 and the park's
 * two worker-service switches at +0x34/+0x38. The hit tests read the extent
 * and the switches from ONE pointer, so it has to be one struct here. */
typedef struct WMap {
    unsigned char  pad00[0x14]; /* +0x00..0x13 */
    unsigned short width;       /* +0x14 */
    unsigned short height;      /* +0x16 */
    unsigned char  pad18[0x1c]; /* +0x18..0x33 */
    int            mechanics;   /* +0x34 */
    int            gardeners;   /* +0x38 */
} WMap;

/* A plain 16-byte rectangle (no chain link) — the element type of the
 * scratch rect list at 0x00801a80 that RefreshObjList accumulates into. */
typedef struct Rect4 {
    int left;    /* +0x00 */
    int top;     /* +0x04 */
    int right;   /* +0x08 */
    int bottom;  /* +0x0c */
} Rect4;

/* A node of the point-to-point flood fill's open list (bnvmove.c's PTPNode);
 * 16 bytes, allocated by PTPVisitTile. */
typedef struct PTPNode {
    struct PTPNode* next;   /* +0x00 */
    struct PTPNode* parent; /* +0x04 */
    int             x;      /* +0x08  tile x */
    int             y;      /* +0x0c  tile y */
} PTPNode;

/* One entry of the 12-record neighbourhood table at 0x004bff28: the 4-cell
 * cross, then the two-cell axials, then the diagonals. */
typedef struct NearOffset {
    int dx;   /* +0x00 */
    int dy;   /* +0x04 */
} NearOffset;

/* One entry of the 17-record message table at 0x004ba8e0 (stride 12). */
typedef struct MessageDef {
    int topic;   /* +0x00  help-popup topic id */
    int delay;   /* +0x04  minimum ms between two showings */
    int last;    /* +0x08  tick of the last showing (cleared @ 0x004735c0) */
} MessageDef;

/* ---------------------------------------------------------------- globals -- */

extern Bloke*       g_gardener_list;         /* 0x0079a8a8 (export GardenerList) */
extern Bloke*       g_mechanic_list;         /* 0x0079a8ac (export MechanicList) */

extern WorkOrder*   g_gardener_orders;       /* 0x0079a8b0 */
extern WorkOrder*   g_gardener_order_tail;   /* 0x0079a8b4 */
extern int          g_gardener_order_count;  /* 0x0079a8b8 */
extern WorkOrder*   g_mechanic_orders;       /* 0x0079a8c0 */
extern WorkOrder*   g_mechanic_order_tail;   /* 0x0079a8c4 */
extern int          g_mechanic_order_count;  /* 0x0079a8c8 */
extern RepairOrder* g_repair_orders;         /* 0x0079a8d4 */

extern int          g_power_supply;          /* 0x00832bd0 (power.c) */
extern int          g_power_unserved;        /* 0x00832bd8 (power.c) */

extern MessageDef   g_messages[];            /* 0x004ba8e0 */
extern NearOffset   g_near_offsets[];        /* 0x004bff28 */

/* The point-to-point flood fill's state (shared with bnvmove.c). */
extern int          g_ptp_wave_count;        /* 0x00669250  nodes added this wave */
extern PTPNode*     g_ptp_open_head;         /* 0x0066b450 */
extern PTPNode*     g_ptp_found;             /* 0x0066b454 */
extern PTPNode*     g_ptp_route_head;        /* 0x0066b458 */

/* The mouse-hit record: the cell the cursor is over, packed {u8 x, u8 y} in
 * the low 16 bits (the hit RECORD TYPE is the dword at 0x004bdd00). */
extern unsigned int g_hit_cell;              /* 0x004bdd08 */
extern Bloke*       g_worker_on_mouse;       /* 0x007fdff0 */
extern int          g_worker_on_mouse_type;  /* 0x007fdffc  0x307 gardener, 0x308 mechanic */
extern void*        g_elem_shed;             /* 0x007fdfb0  gardener's shed class element */
extern void*        g_elem_hut;              /* 0x007fdfb4  mechanic's hut class element */
extern void*        g_elem_path;             /* 0x007fdfb8  the path class element */
extern void*        g_sample_gardener;       /* 0x004b9320 */
extern void*        g_sample_mechanic;       /* 0x004b932c */
extern WMap*        g_wmap;                  /* 0x004bcbf4 (= legoland.h g_map) */
extern Cursor       g_edit_cursor;           /* 0x007febc0 (export EditCursor) */
extern void*        g_env_class;             /* 0x007fd624  environment class */

/* The scratch list of disjoint rectangles the cursor refresh accumulates. */
extern Rect4        g_obj_rects[];           /* 0x00801a80 */
extern int          g_obj_rect_count;        /* 0x00667d3c */
extern void*        g_path_tile_ptr;         /* 0x00832bf0 (PathSprite): first word = tile code */
extern int          g_map_dirty;             /* 0x00668610 */
/* Set to 1 on both sides of the interior re-tile pass; not otherwise
 * identified (nothing else matched touches it yet). */
extern int          g_path_gfx_batch;        /* 0x0066b46c */
extern int          g_message_level;         /* 0x00668960  highest message shown */

/* ------------------------------------------------------------ prototypes -- */

extern void  MemFree(void* p);                            /* 0x0049e4d0 */
extern void* MemAlloc(unsigned int size);                 /* 0x0049e4ff (malloc) */
extern void  NewLongTermAction(Bloke* b, int action);     /* 0x0044e760 */
extern int   GetTicks(void);                              /* 0x00499450 */
extern int   FindObjectsPower(WClass* cls);               /* 0x00459fa0 */
extern void  RecheckPoweredObjects(void);                 /* 0x0045a060 */
/* Open the help popup for a topic; 0 if it was suppressed. */
extern int   ShowHelpPopup(int topic);                    /* 0x0046d280 */

/* Unlink a gardener order from its list (noisy: DBPrintf's the list state). */
extern void  UnlinkGardenerOrder(WorkOrder* o);           /* 0x00499d60 */
/* Centre of an object order's footprint, 24.8, returned in eax:edx. */
extern Pos   GetOrderCentre(WorkOrder* o);                /* 0x00499a70 */
/* The unassigned order in `list` nearest to worker `b` (0 if none). */
extern WorkOrder* FindNearestFreeOrder(WorkOrder* list, Bloke* b); /* 0x00499be0 */
/* calloc(0x3c,1) a gardener order and append it to the gardener list. */
extern WorkOrder* NewGardenerOrder(void);                 /* 0x00499570 */
/* calloc(0x3c,1) a mechanic order and append it to the mechanic list. */
extern WorkOrder* NewMechanicOrder(void);                 /* 0x004995d0 */
/* Grow a span order's rect run until it stops changing. */
extern void  SettleOrderSpan(WorkOrder* o);               /* 0x00499720 */
/* o->amount = a; o->charge = a * 1.5f. */
extern void  SetOrderRepairAmount(WorkOrder* o, float a); /* 0x00499760 */
extern int   GetObjCost(WClass* cls);                     /* 0x00480da0 */
extern WorkOrder* GetGardenerWorkOrderAt(int x, int y);   /* 0x0049b130 */
extern WorkOrder* GetMechanicWorkOrderAt(int x, int y);   /* 0x0049b180 */
extern WorkOrder* AddRepairOrderForObject(WClass* cls, int x, int y); /* 0x0049b930 */
extern void  PutWorkerOnRide(Bloke* b, Cell* cell);       /* 0x0049a0d0 */
extern void* PlayInstanceOfSample(void* def, int a, int b, void* src); /* 0x00496d20 */
extern void  FreePTPOpenList(void);                       /* 0x004821e0 */
extern void  FreePTPRouteList(void);                      /* 0x00482210 */
extern void  ClearPTPVisited(void);                       /* 0x004821c0 */
/* Push (x,y) onto the open list if it is on the map, walkable and unvisited. */
#ifndef LEGOLAND_PORTABLE
extern int   PTPVisitTile(int x, int y, PTPNode* parent);  /* 0x00482620 */
#else
extern void PTPVisitTile(int x, int y, PTPNode* parent);  /* 0x00482620 */
#endif
/* Walk the parent chain of g_ptp_found back into the route list; non-zero
 * when the first step IS the tile we are standing on. */
extern int   BuildPTPRoute(void);                         /* 0x00482430 */
extern void  GetTileCentre(Pos* tile, Pos* out);          /* 0x0045ad60 */
extern void  RemovePathSquare(Pos* p);                    /* 0x00481c90 */
extern int   GetObjRepairCost(WClass* cls, int life);     /* 0x00480de0 */
/* Subtract `r` from the accumulated disjoint-rectangle list, splitting the
 * entries it overlaps. */
extern void  SubtractObjRect(Rect4* r);                   /* 0x0045d5d0 */
/* Subtract `r`, then append it as a new entry. */
extern void  AddObjRectSpan(Rect4* r);                    /* 0x0045d730 */
extern void  ClearCellForPath(Pos* p);                    /* 0x004779d0 */
extern void  AddPathTileGFX(Pos* p, unsigned short tile); /* 0x0045d350 */
extern void  AddPathSquare(Pos* p);                       /* 0x00481c50 */

/* ---------------------------------------------------------- inline helpers -- */

/* The bounds-checked cell fetch (objmap.c's MapCellAt). */
static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

/* The same fetch through a Pos*, reading y lazily (only after x passed). */
static __inline Cell* CellForPos(Pos* pos)
{
    int x = pos->x;
    int y;

    if (x >= 0 && x < g_map->width) {
        y = pos->y;
        if (y >= 0 && y < g_map->height)
            return &g_map_rows[y][x];
    }
    return 0;
}

/* The same fetch read through g_wmap, so the extent reads CSE with the
 * caller's own (and so VC6 hoists ONE load into a loop preheader). */
static __inline Cell* WMapCellAtYX(int y, int x)
{
    if (x >= 0 && x < g_wmap->width && y >= 0 && y < g_wmap->height)
        return &g_map_rows[y][x];
    return 0;
}

/* -------------------------------------------------------------- functions -- */

/* Always 0 in the shipped binary: the build-progress overlay is compiled out
 * (buildtick.c's DoBuildEffects is gated on this and so never draws). */
// FUNCTION: LEGOLAND 0x00450c70
int WantBuildEffects(void* obj)
{
    return 0;
}

/* Unlink, free the rect array and the node, and drop the list count. */
// FUNCTION: LEGOLAND 0x00499e30
void FreeGardenerOrder(WorkOrder* o)
{
    UnlinkGardenerOrder(o);
    MemFree(o->rects);
    MemFree(o);
    g_gardener_order_count--;
}

/* Release the order and put it back on the TAIL of the list so the same
 * worker does not pick it again straight away. A node that is already last
 * (next == 0) is only marked unassigned. */
// FUNCTION: LEGOLAND 0x00499e60
void GiveBackGardenerOrder(WorkOrder* o)
{
    o->assigned = 0;
    if (o->next && g_gardener_orders) {
        UnlinkGardenerOrder(o);
        g_gardener_order_tail->next = o;
        g_gardener_order_tail = o;
        if (!g_gardener_orders)
            g_gardener_orders = o;
        o->next = 0;
    }
}

/* One tick of repair on a cell: the object wears one point of life back. When
 * the life reaches the class maximum (max_cond / 4) the cell's "switched off"
 * flag 0x200 is cleared and, if the object generates power, the park supply
 * grows and the blacked-out objects are re-checked. Returns the new life. */
// FUNCTION: LEGOLAND 0x0049b0d0
int RepairCellTick(Cell* cell, WClass* cls)
{
    cell->life++;
    if (cell->life == (unsigned char)(cls->max_cond >> 2)) {
        int power = FindObjectsPower(cls);
        cell->flags &= ~0x0200;
        if (power > 0) {
            g_power_supply += power;
            if (g_power_unserved)
                RecheckPoweredObjects();
        }
    }
    return cell->life;
}

/* Show park message `which` if it outranks the message already on screen and
 * its own cooldown has expired. Returns 1 when the popup actually opened. */
// FUNCTION: LEGOLAND 0x004735e0
int ShowMessage(int which)
{
    MessageDef* m = &g_messages[which];
    int now;

    if (which > g_message_level) {
        now = GetTicks();
        if (now > m->delay + m->last) {
            if (ShowHelpPopup(m->topic)) {
                g_message_level = which;
                m->last = now;
                return 1;
            }
        }
    }
    return 0;
}

/* The idle gardener (plan 0x10) nearest to the map cell `pos`. */
// FUNCTION: LEGOLAND 0x00499c40
Bloke* FindFreeGardener(Pos* pos)
{
    Bloke* best = 0;
    Bloke* g = g_gardener_list;
    int bestd = 0x7fffffff;

    while (g) {
        if (g->plan == 0x10) {
            int dx = (g->world.x >> 8) - pos->x;
            int dy = (g->world.y >> 8) - pos->y;
            int d = dy * dy + dx * dx;
            if (d < bestd) {
                bestd = d;
                best = g;
            }
        }
        g = g->next;
    }
    return best;
}

/* Hand `o` to gardener `g`: park it in the bloke, work out where to walk to,
 * back-link the order and start the matching long-term action.
 *   kind 1 (object)  target = footprint CENTRE, 24.8: (l+r+2*cell)*128
 *   kind 2 (span)    target = the rect corner biased by the order's (ox,oy) */
// FUNCTION: LEGOLAND 0x00499ac0
void AssignGardenerOrder(Bloke* g, WorkOrder* o)
{
    g->order = o;
    if (o->kind == 1) {
        g->saved.x = (o->rects->right + o->pos.x * 2 + o->rects->left) << 7;
        g->saved.y = (o->rects->bottom + o->pos.y * 2 + o->rects->top) << 7;
    } else {
        g->saved.x = (o->ox + o->pos.x + o->rects->left) << 8;
        g->saved.y = (o->rects->bottom - o->oy + o->pos.y) << 8;
    }
    o->worker = g;
    o->assigned = 1;
    if (o->kind == 1)
        NewLongTermAction(g, 0x12);
    else
        NewLongTermAction(g, 0x15);
}

/* The mechanic's half of the same job. The object-order centre comes from the
 * shared helper (returned as an 8-byte Pos in eax:edx), so only the y store is
 * common to both arms and VC6 tail-merges it. */
// FUNCTION: LEGOLAND 0x00499b60
void AssignMechanicOrder(Bloke* m, WorkOrder* o)
{
    m->order = o;
    if (o->kind == 1) {
        m->saved = GetOrderCentre(o);
    } else {
        m->saved.x = (o->ox + o->pos.x + o->rects->left) << 8;
        m->saved.y = (o->pos.y - o->oy + o->rects->bottom) << 8;
    }
    o->worker = m;
    o->assigned = 1;
    if (o->kind == 1)
        NewLongTermAction(m, 0x13);
    else
        NewLongTermAction(m, 0x16);
}

/* Give an idle gardener the nearest unassigned gardener order; returns the
 * order's kind (0 when there was nothing to do). */
// FUNCTION: LEGOLAND 0x00499d00
int RunGardenerJob(Bloke* g)
{
    WorkOrder* o = FindNearestFreeOrder(g_gardener_orders, g);

    if (!o)
        return 0;
    AssignGardenerOrder(g, o);
    return o->kind;
}

// FUNCTION: LEGOLAND 0x00499d30
int RunMechanicJob(Bloke* m)
{
    WorkOrder* o = FindNearestFreeOrder(g_mechanic_orders, m);

    if (!o)
        return 0;
    AssignMechanicOrder(m, o);
    return o->kind;
}

/* The mechanic list's unlink is written out in line (the gardener path calls
 * the shared helper instead). The `if (p)` after the search is dead — the loop
 * only leaves through the break with p non-null — but VC6 cannot see that and
 * emits the test; it is in the original and kept. */
// FUNCTION: LEGOLAND 0x00499eb0
void FreeMechanicOrder(WorkOrder* o)
{
    WorkOrder* p = g_mechanic_orders;
    WorkOrder* nx;

    if (!p)
        return;
    if (p == o) {
        g_mechanic_order_count--;
        g_mechanic_orders = o->next;
        if (!g_mechanic_orders)
            g_mechanic_order_tail = g_mechanic_orders;
        MemFree(o->rects);
        MemFree(o);
        return;
    }
    while (p) {
        if (p->next == o)
            break;
        p = p->next;
    }
    if (!p)
        return;
    g_mechanic_order_count--;
    nx = o->next;
    p->next = nx;
    if (!nx)
        g_mechanic_order_tail = p;
    MemFree(o->rects);
    MemFree(o);
}

/* Release a mechanic order and move it to the back of the queue. */
// FUNCTION: LEGOLAND 0x00499f40
void GiveBackMechanicOrder(WorkOrder* o)
{
    WorkOrder* p = g_mechanic_orders;

    o->assigned = 0;
    if (o->next && g_mechanic_orders) {
        if (g_mechanic_orders == o) {
            g_mechanic_orders = o->next;
        } else if (p) {
            while (p) {
                if (p->next == o)
                    break;
                p = p->next;
            }
            if (p)
                p->next = o->next;
        }
        g_mechanic_order_tail->next = o;
        g_mechanic_order_tail = o;
        if (!g_mechanic_orders)
            g_mechanic_orders = o;
        o->next = 0;
    }
}

/* Unlink and free a park-funded repair order. The head is dereferenced with no
 * null check, so calling this with an empty list faults (reproduced). */
// FUNCTION: LEGOLAND 0x0049b6e0
void FreeRepairOrder(RepairOrder* r)
{
    RepairOrder* p = g_repair_orders;

    if (p == r) {
        g_repair_orders = r->next;
    } else {
        while (p->next != r) {
            /* The original re-loads the link for the advance (`mov eax,[eax]`)
             * and leaves the loop test as a memory compare; a plain
             * `p = p->next` here is CSE'd with the test and costs two
             * instructions. The volatile read is a codegen lever only — the
             * semantics are exactly `p = p->next`. */
            p = *(RepairOrder* volatile*)p;
            if (!p)
                break;
        }
        if (p)
            p->next = r->next;
    }
    MemFree(r);
}

/* Push a park-funded repair on the front of the list: a by-value copy of the
 * footprint rect, the cell it is biased by, the first charge (amount * 1.5)
 * and the per-tick amount that is added back each frame. */
// FUNCTION: LEGOLAND 0x0049b690
RepairOrder* AddNoneWorkersRepairOrder(Rect* r, Pos* pos, float amount)
{
    RepairOrder* o = (RepairOrder*)MemAlloc(0x28);

    o->next = g_repair_orders;
    g_repair_orders = o;
    o->rect = *r;
    o->pos.x = pos->x;
    o->pos.y = pos->y;
    o->amount = amount * 1.5f;
    o->rate = amount;
    return o;
}

/* Create a gardener order on the class element `elem` at `pos`. The footprint is the class's own
 * rect copied by value; a span order (kind 2) also gets the per-tick repair
 * charge, cost / full-condition. Refuses once the list holds 225 orders. */
// FUNCTION: LEGOLAND 0x00499780
WorkOrder* AddGardenerWorkOrder(ObjElem* elem, Pos* pos, int kind)
{
    WorkOrder* o;
    WClass*    cls;
    float      amount;

    if (g_gardener_order_count >= 0xe1)
        return 0;
    o = NewGardenerOrder();
    o->obj = elem;
    o->pos.x = pos->x;
    o->pos.y = pos->y;
    o->assigned = 0;
    cls = elem->cls;
    o->rects = (Rect*)MemAlloc(0x14);
    *o->rects = cls->rect;
    o->nrects = 1;
    o->kind = (char)kind;
    o->f2c = 1;
    o->oy = -1;
    SettleOrderSpan(o);
    if (kind == 2) {
        amount = (float)GetObjCost(cls) / cls->max_cond;
        SetOrderRepairAmount(o, amount);
    }
    return o;
}

/* The work order EXACTLY under the mouse hit, for the trade currently on the
 * cursor. `cell` (optional) receives the order's near corner. Picking one up
 * plays the trade's pick-up sample. */
// FUNCTION: LEGOLAND 0x00470410
WorkOrder* WorkOrderUnderHit(Pos* cell)
{
    unsigned int h = g_hit_cell & 0xffff;
    int x = h & 0xff;
    int y = h >> 8;
    WorkOrder* o;

    if (g_worker_on_mouse_type == 0x307)
        o = GetGardenerWorkOrderAt(x, y);
    else
        o = GetMechanicWorkOrderAt(x, y);
    if (o) {
        if (cell) {
            cell->x = o->pos.x + o->rects->left;
            cell->y = o->rects->bottom + o->pos.y;
        }
        if (g_worker_on_mouse_type == 0x307)
            PlayInstanceOfSample(g_sample_gardener, 0, 1, 0);
        else
            PlayInstanceOfSample(g_sample_mechanic, 0, 1, 0);
        return o;
    }
    return 0;
}

/* Drop the worker on the mouse onto the cell under the hit. A gardener may
 * only be dropped on its shed and a mechanic only on its hut; either way the
 * worker is put inside (plan 5 = leave the building, action 0x64) at the
 * class's door offset, and the mechanic stands half a tile to the left. The
 * cell's object is dereferenced with no null check, exactly as shipped.
 * Returns 1 when the drop was taken; dropping on a PATH also returns 1 (the
 * caller treats that as handled), anything else 0. */
// FUNCTION: LEGOLAND 0x00470270
int WorkerHitOnRide(void)
{
    unsigned int h = g_hit_cell & 0xffff;
    int          x = h & 0xff;
    int          y = h >> 8;
    Cell*        cell = MapCellAt(x, y);
    WClass*      cls = ((MapInst*)cell->obj)->cls;

    if (cls->elem == g_elem_shed && g_worker_on_mouse_type == 0x307) {
        PutWorkerOnRide(g_worker_on_mouse, cell);
        g_worker_on_mouse->world.x = (cls->hut_ox + x) << 8;
        g_worker_on_mouse->target.x = g_worker_on_mouse->world.x;
        g_worker_on_mouse->world.y = (cls->hut_oy + y) << 8;
        g_worker_on_mouse->target.y = g_worker_on_mouse->world.y;
        g_worker_on_mouse->plan = 5;
        g_worker_on_mouse->action = 0x64;
        PlayInstanceOfSample(g_sample_gardener, 0, 1, 0);
        return 1;
    }
    if (cls->elem == g_elem_hut && g_worker_on_mouse_type == 0x308) {
        PutWorkerOnRide(g_worker_on_mouse, cell);
        g_worker_on_mouse->world.x = ((cls->hut_ox + x) << 8) + 0x80;
        g_worker_on_mouse->target.x = g_worker_on_mouse->world.x;
        g_worker_on_mouse->world.y = (cls->hut_oy + y) << 8;
        g_worker_on_mouse->target.y = g_worker_on_mouse->world.y;
        g_worker_on_mouse->plan = 5;
        g_worker_on_mouse->action = 0x64;
        PlayInstanceOfSample(g_sample_mechanic, 0, 1, 0);
        return 1;
    }
    return cls->elem == g_elem_path;
}

/* Raise (or find) the repair order for the object under the mouse hit and hand
 * it to the worker on the cursor. The cell must carry flags 0x08|0x80 (a
 * repairable object is present) and the class must have a condition scale; the
 * gardener (0x307) takes classes flagged 0x200000 when the park's gardener
 * service is on, the mechanic (0x308) classes flagged 0x400000. A cell that
 * already carries CF_REPAIRORDER (0x4000) is refused; a new order sets it.
 * `cell` (optional) receives the order's near corner. As in WorkerHitOnRide
 * the cell and its object are dereferenced with no null check. */
/* SOLVED (129/129, 363/363 bytes) by three separate source facts, all of
 * which are worth reusing:
 *
 * 1. THE MAP POINTER LIVES IN EBX because the `map` LOCAL is used ONLY by the
 *    bounds guard; the two service tests read the GLOBAL directly
 *    (`g_wmap->gardeners` / `g_wmap->mechanics`).  Reading them through the
 *    local instead makes VC6 keep `map` in a caller-saved EDX, reuse EAX for
 *    both u16 extent temps and SINK the `push ebx` into the post-guard path.
 *    Counter-intuitively, giving a value FEWER source uses is what buys it a
 *    callee-saved home: with one live range that dies at the guard VC6 parks
 *    it in EBX and rematerialises the global at each later use, which is
 *    exactly what the original does (it even reloads EBX at 0x47053a after
 *    borrowing it to compare the worker type).
 *
 * 2. THE FAILURE RETURN MUST BE THE FUNCTION'S LAST BLOCK.  Written as
 *    `if (!(flags & 0x88)) goto none;` with `none: return 0;` at the bottom,
 *    VC6 inverts the test and emits the 4-pop `return 0` epilogue INLINE
 *    there, so every later block shifts.  Wrapping the whole body in
 *    `if (flags & 0x88) { ... }` and ending with a bare `return 0;` keeps the
 *    success path falling through and puts the second epilogue at the end,
 *    where the original has it.
 *
 * 3. `o = 0` MUST BE INITIALISED BEFORE `y`.  The zeroing of the result and
 *    the `h >> 8` are independent, and VC6 emits them in declaration order:
 *    with `y` declared first the `shr eax,8` beats `xor esi,esi` and the
 *    interleaved `push edi` lands on the wrong side of both. */
// FUNCTION: LEGOLAND 0x004704b0
WorkOrder* WorkOrderNearHit(Pos* cell)
{
    WMap*          map = g_wmap;
    unsigned int   h = g_hit_cell & 0xffff;
    int            x = h & 0xff;
    WorkOrder*     o = 0;
    int            y = h >> 8;
    Cell*          c;
    unsigned short flags;
    WClass*        cls;
    int            mask;

    if (x >= 0 && x < map->width && y >= 0 && y < map->height)
        c = &g_map_rows[y][x];
    else
        c = 0;
    flags = c->flags;
    if (flags & 0x88) {
        cls = ((MapInst*)c->obj)->cls;
        if (!cls->max_cond)
            goto done;
        if ((cls->flags & 0x200000) && g_worker_on_mouse_type == 0x307
            && g_wmap->gardeners) {
            mask = 0x4000;
            if (flags & mask)
                goto done;
            o = AddRepairOrderForObject(cls, x, y);
            if (!o)
                goto done;
        } else if ((cls->flags & 0x400000) && g_worker_on_mouse_type == 0x308
                   && g_wmap->mechanics) {
            mask = 0x4000;
            if (flags & mask)
                goto done;
            o = AddRepairOrderForObject(cls, x, y);
            if (!o)
                goto done;
        } else {
            goto done;
        }
        c->flags |= (unsigned short)mask;
        if (cell) {
            cell->x = o->pos.x + o->rects->left;
            cell->y = o->rects->bottom + o->pos.y;
        }
        if (g_worker_on_mouse_type == 0x307)
            PlayInstanceOfSample(g_sample_gardener, 0, 1, 0);
        else
            PlayInstanceOfSample(g_sample_mechanic, 0, 1, 0);
done:
        return o;
    }
    return 0;
}

/* The nearest cell around (x,y) holding a worn object a mechanic could repair.
 * The 12 offsets are searched in table order (cross, two-cell axials,
 * diagonals); the candidate cell must carry an object (flags 0x08|0x80), and
 * the object's condition is read from its ROOT cell (+0x04/+0x05 hold the tile
 * the object is anchored at), so any tile of a multi-cell object finds it. The
 * object's class must be mechanic-serviced (0x400000), the park's mechanic
 * service on, and the root cell must not already carry a repair order. The
 * first argument (the mechanic) is unused in the shipped build. */
// FUNCTION: LEGOLAND 0x0049b350
Cell* FindBrokenCellNear(Bloke* m, int x, int y)
{
    unsigned int i;

    for (i = 0; i < 12; i++) {
        int nx = g_near_offsets[i].dx + x;
        int ny = g_near_offsets[i].dy + y;

        if (nx >= 0 && nx < g_wmap->width && ny >= 0 && ny < g_wmap->height) {
            Cell* c = &g_map_rows[ny][nx];
            if (c && (c->flags & 0x88)) {
                WClass* cls = ((MapInst*)c->obj)->cls;
                Cell*   base = WMapCellAtYX(c->by, c->bx);
                if (cls->max_cond && base->life < cls->max_cond
                    && (cls->flags & 0x400000)) {
                    if (g_wmap->mechanics && !(base->flags & 0x4000))
                        return base;
                }
            }
        }
    }
    return 0;
}

/* The path-finder call the Build handlers make each tick. A breadth-first
 * flood fill over the walkable tiles from `from` to `to` (both 24.8 world
 * coordinates), one wave at a time: each wave re-reads the node count the
 * previous wave added, walks that many nodes, and expands N, E, S, W.
 *   0  the fill died out — no route.
 *   1  `out` = the centre of the next route tile ((tile << 8) + 0x80).
 *   2  the route's first step is the tile we are already on, so `out` is the
 *      destination itself and the caller can walk straight at it. */
// FUNCTION: LEGOLAND 0x00482710
int FindPathLeg(Pos* from, Pos* to, Pos* out)
{
    int          sx = from->x >> 8;
    int          sy = from->y >> 8;
    int          tx = to->x >> 8;
    int          ty = to->y >> 8;
    PTPNode*     node;
    unsigned int n;

    FreePTPOpenList();
    FreePTPRouteList();
    ClearPTPVisited();
    g_ptp_wave_count = 0;
    g_ptp_found = 0;
    PTPVisitTile(sx, sy, 0);
    n = g_ptp_wave_count;
    while (n) {
        node = g_ptp_open_head;
        g_ptp_wave_count = 0;
        while (n-- != 0) {
            if (node->x == tx && node->y == ty)
                goto found;
            PTPVisitTile(node->x, node->y - 1, node);
            PTPVisitTile(node->x + 1, node->y, node);
            PTPVisitTile(node->x, node->y + 1, node);
            PTPVisitTile(node->x - 1, node->y, node);
            node = node->next;
        }
        n = g_ptp_wave_count;
    }
    FreePTPOpenList();
    FreePTPRouteList();
    return 0;
found:
    g_ptp_found = node;
    if (BuildPTPRoute()) {
        out->x = to->x;
        out->y = to->y;
        FreePTPOpenList();
        FreePTPRouteList();
        return 2;
    }
    out->x = (g_ptp_route_head->x << 8) + 0x80;
    out->y = (g_ptp_route_head->y << 8) + 0x80;
    FreePTPOpenList();
    FreePTPRouteList();
    return 1;
}

/* Erase the path/scenery footprint an object of `elem`'s class would cover at
 * `pos`. The edit cursor is saved into a local, the class's own cursor builder
 * is run over it, and every cell of every rect of every cursor in the chain
 * that holds an ENVIRONMENT object (flags bit 3, class == g_env_class) is
 * cleared: its path square is dropped, flags 0x18 and RF bit 0 are cleared and
 * the displayed tile falls back to the ground tile. The cursor is restored
 * from the local afterwards, so the caller's edit cursor survives. The tile
 * centre GetTileCentre computes is never used (the original computes it
 * anyway); the two `while (p)` guards on addresses of objects are likewise
 * always true and are in the original. */
// FUNCTION: LEGOLAND 0x0045e300
void ClearObjFootprint(ObjElem* elem, Pos* pos)
{
    Pos     cell;
    Pos     centre;
    Cursor  saved;
    WClass* cls = elem->cls;
    Cursor* e;
    Rect*   r;
    int     bx;
    int     by;

    saved = g_edit_cursor;
    GetTileCentre(pos, &centre);
    cls->build_cursor(cls->elem, &centre, 0x8f8);
    e = &g_edit_cursor;
    while (e) {
        if (!(e->flags & 0x3000)) {
            r = &e->rect;
            while (r) {
                for (by = r->top; by <= r->bottom; by++) {
                    for (bx = r->left; bx <= r->right; bx++) {
                        cell.x = e->origin.x + bx;
                        cell.y = e->origin.y + by;
                        if (cell.x >= 0 && cell.x < g_wmap->width
                            && cell.y >= 0 && cell.y < g_wmap->height) {
                            Cell* c = &g_map_rows[cell.y][cell.x];
                            if (c && (c->flags & 8)
                                && ((MapInst*)c->obj)->cls == (WClass*)g_env_class) {
                                RemovePathSquare(&cell);
                                c->flags &= 0xffe7;
                                c->rf &= 0xfe;
                                g_map_rows[cell.y][cell.x].tile =
                                    g_map_rows[cell.y][cell.x].base;
                            }
                        }
                    }
                }
                r = r->next;
            }
        }
        e = e->next;
    }
    g_edit_cursor = saved;
}

/* Create a mechanic order on the class element `elem` at `pos`, refusing once the mechanic list
 * holds 225 orders (the cap workorder.c documents). A kind-2 (object) order
 * carries one rect copied out of the class; anything else copies the whole
 * EDIT CURSOR chain: the rect run of every cursor whose flags lack 0x3000 is
 * counted, one array is allocated for the lot, and each rect is copied in and
 * re-based from the cursor's origin onto the order's own cell. An object order
 * also gets the per-tick repair charge: the repair cost of the cell's current
 * life spread over the condition still to be restored. */
// FUNCTION: LEGOLAND 0x00499830
WorkOrder* AddMechanicWorkOrder(ObjElem* elem, Pos* pos, int kind)
{
    WorkOrder* o;
    WClass*    cls;
    Cursor*    e;
    Rect*      r;
    int        dx;
    int        dy;
    Cell*      c;
    int        life;
    float      amount;

    if (g_mechanic_order_count >= 0xe1)
        return 0;
    o = NewMechanicOrder();
    o->obj = elem;
    o->pos.x = pos->x;
    o->pos.y = pos->y;
    o->assigned = 0;
    cls = elem->cls;
    if (kind == 2) {
        o->rects = (Rect*)MemAlloc(0x14);
        o->nrects = 1;
        *o->rects = cls->rect;
    } else {
        o->nrects = 0;
        e = &g_edit_cursor;
        while (e) {
            if (!(e->flags & 0x3000)) {
                r = &e->rect;
                while (r) {
                    o->nrects++;
                    r = r->next;
                }
            }
            e = e->next;
        }
        o->rects = (Rect*)MemAlloc(o->nrects * 20);
        o->nrects = 0;
        e = &g_edit_cursor;
        while (e) {
            if (!(e->flags & 0x3000)) {
                dx = e->origin.x - o->pos.x;
                dy = e->origin.y - o->pos.y;
                r = &e->rect;
                while (r) {
                    o->rects[o->nrects] = *r;
                    o->rects[o->nrects].left += dx;
                    o->rects[o->nrects].top += dy;
                    o->rects[o->nrects].right += dx;
                    o->rects[o->nrects].bottom += dy;
                    o->nrects++;
                    r = r->next;
                }
            }
            e = e->next;
        }
    }
    o->kind = (char)kind;
    o->f2c = 1;
    o->oy = -1;
    SettleOrderSpan(o);
    if (kind == 2) {
        c = CellForPos(pos);
        life = c->life;
        amount = (float)GetObjRepairCost(cls, life) / (cls->max_cond - life);
        SetOrderRepairAmount(o, amount);
    }
    return o;
}

/* Re-lay the path under a chain of cursors after the SELECTED object (the one
 * whose cursor carries flag 0x1000) has moved. The disjoint-rectangle scratch
 * list at 0x00801a80 is reset, the selected cursor's footprint is added to it,
 * and every cursor BEFORE it in the chain subtracts its own footprint (its
 * bounding rect first, then every rect of its run — the first rect is
 * therefore subtracted twice, as shipped). What is left of the list is the
 * ground the object uncovered: each of its cells is cleared for path use, its
 * RF direction bits are dropped, the path tile and a path square are laid, and
 * the map is marked dirty (0x10). Finally the INTERIOR of the selected
 * footprint (inset by one on every side) is re-tiled. */
/* SOLVED, and the lever generalises -- write it down.  201/201.
 *
 * The interior pass computes four bounds of the form `origin_field +
 * rect_field +/- 1`, each folded into one `lea`.  Which of the two loads VC6
 * emits FIRST is NOT decided by source operand order: every permutation
 * (a+b+/-1, a+(b+/-1), (a+/-1)+b, -1+a+b, while- instead of for-loops, an
 * inlined field getter, and even a flat struct with the same offsets) emits
 * the identical schedule.  VC6 SP3 canonicalises a commutative sum of two
 * independent loads by an internal key.
 *
 * What DOES move that key is giving one side a DIFFERENT BASE SYMBOL: assign
 * `Rect* r = &sel->rect;` / `Pos* o = &sel->origin;` and spell the operand
 * `r->bottom` instead of `sel->rect.bottom`.  VC6 still folds the address
 * back to `[ebp+0x1420]`, so not one instruction changes -- but the operand
 * now sorts on `r` rather than on `sel`, and the pair flips.  The key follows
 * the ORDER THE ALIASES ARE CREATED (assigning `o` before `r` flips every
 * site the other way), and it is global to the region, so the four sites
 * cannot be tuned independently -- the fourth bound has to fall back to the
 * direct `sel->origin.x` spelling to land on the original's order.
 * Aliases only work when they introduce a genuinely new base: `&a->x` (a
 * zero offset) and a pointer to a whole stack local both fold away and change
 * nothing, which is why simcore.c's IsAdjacentPos and joust.c's
 * TempleSlide_Draw -- the same canonicalisation over two POINTER PARAMETERS
 * and over two STACK LOCALS -- are still open. */
// FUNCTION: LEGOLAND 0x0045d770
void RefreshObjList(Cursor* head)
{
    Pos     p;
    Rect4   rect;
    Cursor* c;
    Cursor* sel;
    Rect*   r;
    Pos*    o;
    int     i;

    c = head;
    if (!c)
        return;
    while (c) {
        if (c->flags & 0x1000)
            goto found;
        c = c->next;
    }
    return;
found:
    sel = c;
    if (!c)
        return;
    if (!c)
        return;
    g_obj_rect_count = 0;
    rect.left = c->rect.left + c->origin.x;
    rect.top = c->rect.top + c->origin.y;
    rect.right = c->rect.right + c->origin.x;
    rect.bottom = c->rect.bottom + c->origin.y;
    AddObjRectSpan(&rect);
    while (head) {
        if (head->flags & 0x1000) {
            sel = head;
            break;
        }
        rect.left = head->origin.x + head->rect.left;
        rect.top = head->rect.top + head->origin.y;
        rect.right = head->rect.right + head->origin.x;
        rect.bottom = head->rect.bottom + head->origin.y;
        r = &head->rect;
        SubtractObjRect(&rect);
        while (r) {
            rect.left = head->origin.x + r->left;
            rect.top = r->top + head->origin.y;
            rect.right = r->right + head->origin.x;
            rect.bottom = r->bottom + head->origin.y;
            SubtractObjRect(&rect);
            r = r->next;
        }
        head = head->next;
    }
    i = 0;
    if (g_obj_rect_count > 0) {
        do {
            for (p.y = g_obj_rects[i].top; p.y <= g_obj_rects[i].bottom; p.y++) {
                for (p.x = g_obj_rects[i].left; p.x <= g_obj_rects[i].right; p.x++) {
                    ClearCellForPath(&p);
                    g_map_rows[p.y][p.x].rf &= ~3;
                    AddPathTileGFX(&p, *(unsigned short*)g_path_tile_ptr);
                    g_map_dirty |= 0x10;
                    AddPathSquare(&p);
                }
            }
            i++;
        } while (i < g_obj_rect_count);
    }
    g_path_gfx_batch = 1;
    r = &sel->rect;
    o = &sel->origin;
    for (p.y = r->top + o->y + 1;
         p.y <= o->y + r->bottom - 1; p.y++) {
        for (p.x = r->left + o->x + 1;
             p.x <= r->right + sel->origin.x - 1; p.x++)
            AddPathTileGFX(&p, *(unsigned short*)g_path_tile_ptr);
    }
    g_path_gfx_batch = 1;
}
