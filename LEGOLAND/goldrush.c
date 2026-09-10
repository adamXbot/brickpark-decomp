/* LEGOLAND -- the CASTLE LEVEL 1, FORT, TEMPLE and GOLD RUSH ride callback
 * sets, plus the RESTAURANT 2 per-instance animation tick and the DRIVING
 * SCHOOL car step.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere). The four callback sets here are installed by the three
 * 17-instruction providers and the 19-instruction one in interfaces.c
 * (CastleLevel1_GetInterfaces 0x00403080, Fort_GetInterfaces 0x004068b0,
 * Temple_GetInterfaces 0x00416e50, GoldRush_GetInterfaces 0x004078f0).
 *
 * ==========================================================================
 * SLOT NAMES -- two of the table's names are wrong, and this file renames
 * ==========================================================================
 * docs/RIDE_CALLBACKS.md labels +0xa8 "activate" and +0xb0 "interact"; the
 * bodies say otherwise, exactly as ridecb1.c found for its own cluster:
 *
 *   +0xa8 is the class's PER-TICK RIDER STATE MACHINE. It is called once a
 *         frame from the class walk at 0x0045b5e2 (gated on ObjDef flags
 *         & 0x20, a bit the +0xa4 loader sets itself) and it walks the WHOLE
 *         class's rider list, stepping each bloke's action byte.
 *   +0xb0 is the class's DEPTH-SORTED OVERLAY DRAW. Its one call site is the
 *         render-list walker at 0x00485ac3, which passes
 *         (elem, screen_x, screen_y, MapSquare*, clip, mode) -- and the clip
 *         argument is dead in every handler because the walker has already
 *         installed it.
 *
 * So the names below are `_TickRiders` and `_Draw`; the table's
 * `GoldRush_Activate` (0x004072b0) and `GoldRush_Interact` (0x00406b10) are
 * renamed `GoldRush_TickRiders` / `GoldRush_Draw` on that evidence.
 *
 * ==========================================================================
 * THE MATTE SPRITES -- what the +0xa4 loaders are actually for
 * ==========================================================================
 * All four of these classes are BUILDINGS PEOPLE WALK INTO, so all four need
 * to occlude their own visitors. Each one's +0xa4 create handler loads a set
 * of standalone "matte" .lls sprites -- the foreground slices of the
 * building -- and each one's +0xb0 draw handler renders the people first and
 * then paints the mattes over them:
 *
 *   CASTLE LEVEL 1  Castle Matte.lls                       (1 matte)
 *   FORT            fortmask.lls                           (1 matte)
 *   TEMPLE          temple_matte1.lls, temple_matte2.lls   (2 mattes)
 *   GOLD RUSH       goldwashmatte1.lls, goldwash.lls,
 *                   goldwashmatte2.lls, goldmask.lls       (4 mattes)
 *
 * The matte is printed at the object's screen position plus the render
 * offset of a numbered LAYER of the class's own build sprite (layer 2 for
 * the castle, 0 and 3 for the temple), so the mattes stay registered with
 * the building even though they are separate sprites.
 *
 * ==========================================================================
 * GOLD RUSH -- the one ride here with per-placement state
 * ==========================================================================
 * GOLD RUSH keeps a singly linked list of 0x2c-byte records, head at
 * 0x004c1204, keyed by the packed {x,y} map square at +0x00 with `next` at
 * +0x0c -- exactly the record ridesave.c's SaveGoldWash (0x00407800) /
 * LoadGoldWash (0x00407870) serialise. Its helpers, all just below the
 * callbacks in the original text, are:
 *
 *   0x00406920  GoldRush_NewRecord(square)   push a record
 *   0x00406960  GoldRush_FreeRecord(rec)     unlink and free one
 *   0x004069c0  GoldRush_FreeAllRecords()    drain the list
 *   0x004069e0  GoldRush_FindRecord(square)  find by the u16 at +0x00
 *
 * GOLD RUSH also owns a PATH: its +0xa4 builds one from the 5-segment
 * polyline at 0x004b45e0 ({-2,0},{0,-6},{-6,0},{0,1},{3,0} in tiles) via
 * 0x00412100, and its +0xac frees it with 0x00412290. And it is the only one
 * of the four that touches the map: its +0x98 place handler stamps FOUR path
 * tiles (the wash's approach) with AddPathTileGFX and its +0x9c remove
 * handler tears the same four down with RemoveRollerCoasterPath.
 *
 * ==========================================================================
 * WHAT THIS PASS LEARNED (data structures and rules)
 * ==========================================================================
 * THE FOUR CLASSES ARE ONE TEMPLATE. CASTLE LEVEL 1, FORT, TEMPLE and GOLD
 * RUSH share a single skeleton, filled in from a per-class block of globals:
 *
 *              CASTLE LEVEL 1   FORT        TEMPLE      GOLD RUSH
 *   ObjDef     0x004c10dc       0x004c11dc  0x004cbf5c  0x004c11f0
 *   layers     (none)           0x004c11d8  0x004cbf64  0x004c11e8
 *   render list 0x004c10e8      0x004c11e0  0x004cbf70  0x004c1208
 *   mattes     0x004c10e4       0x004c11cc  0x004cbf68   0x004c11f8
 *                                           0x004cbf6c   0x004c11fc
 *                                                        0x004c1200
 *                                                        0x004c11f4
 *   records    -                -           -           0x004c1204
 *   walk path  -                -           -           0x004c11e4
 *
 * A BLOKE, as these four read it (offsets confirmed against ridecb1.c):
 *   +0x0e u16 low-level AI state, 0 = idle, 7 = walking a CalcMoveLine
 *   +0x24 Pos target (24.8)          +0x34 i8 walk-path step direction
 *   +0x36 u8 pan/seat slot           +0x38 i16 walk-path node
 *   +0x3a i16 "stand around" timer   +0x40 i16 in-fort sub-state
 *   +0x58 int wander timer           +0x60 u8 action (the state machine)
 *   +0x62 u16 flags, bit 3 = "using this ride"
 *   +0x68 Pos world (24.8)           +0x72/+0x73 facing / new facing
 *   +0x98 CalcMoveLine scratch
 *
 * THE WALK-PATH SUBSYSTEM at 0x00412100..0x00412300 is new here and is NOT
 * the .bnv path machinery joust.c documents. It takes a POLYLINE descriptor
 * {int n; Pos* pts;} of tile deltas, walks the segments summing their integer
 * lengths, and allocates 12 bytes per unit step plus 8 (0x00412100); the
 * result is a node array the bloke walks one node a tick (0x00412300), with
 * the direction in Bloke+0x34 and the cursor in Bloke+0x38, started at either
 * end (0x004122d0 / 0x004122a0) and freed with 0x00412290. GOLD RUSH is the
 * only user in this lane: its five-segment polyline lives at 0x004b45e0.
 *
 * AN LLS RECORD, as seen from a ride: current frame u16 @ +0x00, frame count
 * u16 @ +0x10. FORT advances layer 2's frame itself and copies it onto its
 * mask; GOLD RUSH copies layer 1's frame MODULO 8 onto goldmask.lls.
 *
 * VC6 LEVERS THIS FILE ADDED TO THE PLAYBOOK
 *  - `screen.X + off.X` versus `off.X + screen.X` decides `add`-into-offset
 *    versus `lea`-into-scratch AND the schedule of the sprite-global load; it
 *    differs per call site inside ONE function (see Fort_Draw, whose two
 *    PrintSprites want opposite spellings).
 *  - An address-taken Offset declared at FUNCTION level takes the LOWER frame
 *    home than a plain struct local; declaring it inside the `if` block that
 *    uses it moves it above, which is what the multi-band draws need.
 *  - Reading a by-value packed square back as bytes: a pointer to a
 *    `struct { unsigned char x, y; }` gives the original's `mov reg,[esp+n]`
 *    / `and reg,0xff` pair with the second field read at +1; `unsigned int`
 *    arithmetic and 8-bit BITFIELDS both CSE into one dword load plus
 *    `mov dl,ah` instead.
 *  - `act = b->action; switch (b->action)` (assign the local, switch on the
 *    field) is what spills the action byte to a local AND uses the register
 *    copy for the jump-table index; `switch (act)` reloads.
 *  - Case blocks come out in SOURCE order, so a switch whose arms the
 *    original lays out 0,1,2,4,5,3 has to be written in that order.
 */

/* ---- shared shapes (same offsets as ridecb1.c / joust.c) ----------------- */

/* An 8-byte {x,y} pair returned in eax:edx (legoland.h's Offset). */
typedef struct Offset {
    int ox;
    int oy;
} Offset;

typedef struct Layers {
    int    pad0;
    int    pad4;
    void** sprites;      /* +0x08 */
    int*   render_ox;    /* +0x0c */
    int*   render_oy;    /* +0x10 */
} Layers;

typedef struct RenderObj {
    int     pad0;
    int     pad4;
    Layers* layers;      /* +0x08 */
} RenderObj;

/* A loaded sprite; bit 0x2000 of +0x10 routes its draw through slot +0xb0. */
typedef struct Spr {
    unsigned char pad00[0x10];
    unsigned int  flags;         /* +0x10 */
} Spr;

typedef struct RideObject RideObject;

typedef struct RideElem {
    char*        name;           /* +0x00 */
    char*        image;          /* +0x04 */
    unsigned int flags;          /* +0x08 */
    RideObject*  data;           /* +0x0c */
} RideElem;

/* An {x,y} pair in map tiles (the place/remove handlers' second argument). */
typedef struct Pos {
    int x;
    int y;
} Pos;

typedef struct Person3D Person3D;

typedef struct Bloke Bloke;

typedef struct RiderPerson {
    unsigned char pad00[0x20];
    int           depth;         /* +0x20  render sort key */
} RiderPerson;

typedef struct RiderNode {
    struct RiderNode*   next;    /* +0x00 */
    struct RiderNode*   prev;    /* +0x04 */
    Bloke*              bloke;   /* +0x08 */
    unsigned short      ride_id; /* +0x0c  packed {x,y} of the placement */
    unsigned short      pad0e;
    struct RiderPerson* person;  /* +0x10 */
} RiderNode;

struct RideObject {
    unsigned char  pad00[0x0c];
    int            base_x;       /* +0x0c */
    int            base_y;       /* +0x10 */
    int            f14;
    int            f18;
    unsigned int   flags;        /* +0x1c  0x20 = tick me, 0x400 = ask +0xa0 */
    unsigned char  pad20[0x24 - 0x20];
    signed char    qx;           /* +0x24 */
    signed char    qy;           /* +0x25 */
    unsigned char  pad26[0x2e - 0x26];
    short          capacity;     /* +0x2e */
    unsigned char  pad30[0x3c - 0x30];
    int            footprint;    /* +0x3c */
    unsigned char  pad40[0x64 - 0x40];
    RenderObj*     layers;       /* +0x64  build sprite / layer holder */
    unsigned char  pad68[0xc4 - 0x68];
    RideElem*      elem;         /* +0xc4 */
    unsigned char* visit_counts; /* +0xc8 */
    RiderNode*     riders;       /* +0xcc  the class-wide rider list */
};

/* A placed object's map square, packed as two bytes. */
typedef struct MapSquare {
    unsigned char bx;            /* +0x00 */
    unsigned char by;            /* +0x01 */
} MapSquare;

/* The scratch render list the overlay draws sort their people through. */
typedef struct RenderList {
    void* head;                  /* +0x00 */
} RenderList;

/* ---- engine entry points ------------------------------------------------ */
extern void*  LoadSprite(const char* name, int flag);                /* 0x00497ab0 */
extern void   KillSprite(void* spr);                                 /* 0x00497bd0 */
extern void   DefaultCursor(void* cursor);                           /* 0x0045a390 */
extern void   SetEditCursorFootPrint(void* src);                     /* 0x0045f440 */
extern void   AddBasicObject(void* obj, Pos* pos);                   /* 0x0045efe0 */
extern void   StandardRemoveObject(void* obj, unsigned int tile, void* ctx); /* 0x0045f220 */
extern void   RemoveAllBlokesFromRide(RideObject* cls, unsigned int tile);   /* 0x0048a2e0 */
extern void   RenderItems_New(void);                                 /* 0x00442e90 */
extern void   AddBlokeToRenderList(RenderList*, RiderNode*, int key);/* 0x00442f20 */
extern void   RenderBlokeList(RenderList* list);                     /* 0x00442f70 */
extern Offset GetScreenCoordsForObject(MapSquare* inst, RideObject* item); /* 0x00442cc0 */
extern void   AdjustOffsetForViewMode(Offset* o);                    /* 0x00442d30 */
extern Offset GetRenderOffsetForLayer(RenderObj* obj, int layer);    /* 0x00441ee0 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx);/* 0x004853a0 */

extern int       g_edit_changed;   /* 0x008119b0 EditMode */
extern RideObject* g_edit_object;  /* 0x008119b8 */
extern char      g_edit_cursor;    /* 0x007febc0 EditCursor */

/* ---- CASTLE LEVEL 1 state ------------------------------------------------ */
extern RideObject* g_castle1_def;    /* 0x004c10dc */
extern void*       g_castle1_matte;  /* 0x004c10e4  Castle Matte.lls */
extern RenderList  g_castle1_list;   /* 0x004c10e8 */

/* ---- FORT state ---------------------------------------------------------- */
extern RideObject* g_fort_def;       /* 0x004c11dc */
extern RenderObj*  g_fort_layers;    /* 0x004c11d8  = def->layers */
extern void*       g_fort_mask;      /* 0x004c11cc  fortmask.lls */

/* ---- TEMPLE state -------------------------------------------------------- */
extern RideObject* g_temple_def;     /* 0x004cbf5c */
extern RenderObj*  g_temple_layers;  /* 0x004cbf64  = def->layers */
extern void*       g_temple_matte1;  /* 0x004cbf68  temple_matte1.lls */
extern void*       g_temple_matte2;  /* 0x004cbf6c  temple_matte2.lls */
extern RenderList  g_temple_list;    /* 0x004cbf70 */

/* ---- GOLD RUSH state ----------------------------------------------------- */
extern RideObject* g_gold_def;       /* 0x004c11f0 */
extern RenderObj*  g_gold_layers;    /* 0x004c11e8  = def->layers */
extern void*       g_gold_path;      /* 0x004c11e4  built from 0x004b45e0 */
extern void*       g_gold_matte1;    /* 0x004c11f8  goldwashmatte1.lls */
extern void*       g_gold_wash;      /* 0x004c11fc  goldwash.lls */
extern void*       g_gold_matte2;    /* 0x004c1200  goldwashmatte2.lls */
extern void*       g_gold_mask;      /* 0x004c11f4  goldmask.lls */

/* ==========================================================================
 * +0xa4 -- LOAD THE CLASS'S RESOURCES
 * Arms the per-tick slot (ObjDef flags |= 0x20) and the custom-draw slot
 * (build sprite flags |= 0x2000), caches the build sprite, then loads the
 * class's matte sprites.
 *
 * CASTLE LEVEL 1 is the odd one out: it reads `def->layers` OUTSIDE the null
 * guard (the `je` lands on the load), so a class whose ODF has no data would
 * fault here. Reproduced -- it is what the original does.
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00402ca0
void CastleLevel1_Create(RideElem* elem)
{
    RideObject* def = elem->data;
    RenderObj*  spr;

    g_castle1_def = def;
    if (def)
        def->flags |= 0x20;
    spr = def->layers;
    if (spr)
        ((Spr*)spr)->flags |= 0x2000;
    g_castle1_matte = LoadSprite("Castle Matte.lls", 1);
}

// FUNCTION: LEGOLAND 0x00406240
void Fort_Create(RideElem* elem)
{
    g_fort_def = elem->data;
    if (g_fort_def) {
        g_fort_def->flags |= 0x20;
        if (g_fort_def->layers) {
            ((Spr*)g_fort_def->layers)->flags |= 0x2000;
            g_fort_layers = g_fort_def->layers;
        }
    }
    g_fort_mask = LoadSprite("fortmask.lls", 1);
}

// FUNCTION: LEGOLAND 0x004169c0
void Temple_Create(RideElem* elem)
{
    g_temple_def = elem->data;
    if (g_temple_def) {
        g_temple_def->flags |= 0x20;
        if (g_temple_def->layers) {
            ((Spr*)g_temple_def->layers)->flags |= 0x2000;
            g_temple_layers = g_temple_def->layers;
        }
    }
    g_temple_matte1 = LoadSprite("temple_matte1.lls", 1);
    g_temple_matte2 = LoadSprite("temple_matte2.lls", 1);
}

/* ==========================================================================
 * +0xac -- FREE THE CLASS'S RESOURCES
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00402ce0
void CastleLevel1_Destroy(void)
{
    if (g_castle1_matte)
        KillSprite(g_castle1_matte);
}

// FUNCTION: LEGOLAND 0x004062a0
void Fort_Destroy(void)
{
    if (g_fort_mask)
        KillSprite(g_fort_mask);
}

// FUNCTION: LEGOLAND 0x00416a30
void Temple_Destroy(void)
{
    if (g_temple_matte1)
        KillSprite(g_temple_matte1);
    if (g_temple_matte2)
        KillSprite(g_temple_matte2);
}

/* ==========================================================================
 * +0x8c -- SELECT FOR PLACEMENT
 * Flag the edit state dirty, make this class the object being placed, reset
 * the cursor, hand it the class footprint (ObjDef+0x3c). The global is
 * RE-READ after DefaultCursor in every one of these.
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00402ff0
void CastleLevel1_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_castle1_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

// FUNCTION: LEGOLAND 0x00406820
void Fort_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_fort_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

// FUNCTION: LEGOLAND 0x00416dc0
void Temple_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_temple_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

// FUNCTION: LEGOLAND 0x004075b0
void GoldRush_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_gold_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

/* ==========================================================================
 * +0x98 -- PLACE ONE ON THE MAP (three of the four are the generic handler)
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00403060
void CastleLevel1_Place(void* obj, Pos* pos)
{
    AddBasicObject(obj, pos);
}

// FUNCTION: LEGOLAND 0x00406860
void Fort_Place(void* obj, Pos* pos)
{
    AddBasicObject(obj, pos);
}

// FUNCTION: LEGOLAND 0x00416e00
void Temple_Place(void* obj, Pos* pos)
{
    AddBasicObject(obj, pos);
}

/* ==========================================================================
 * +0x9c -- TAKE ONE OFF THE MAP
 * Unbuild the footprint, then evict every rider the class still has parked
 * on that square.
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00403030
void CastleLevel1_Remove(void* obj, unsigned int tile, void* ctx)
{
    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(((RideElem*)obj)->data, tile);
}

// FUNCTION: LEGOLAND 0x00406880
void Fort_Remove(void* obj, unsigned int tile, void* ctx)
{
    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(((RideElem*)obj)->data, tile);
}

// FUNCTION: LEGOLAND 0x00416e20
void Temple_Remove(void* obj, unsigned int tile, void* ctx)
{
    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(((RideElem*)obj)->data, tile);
}

/* ==========================================================================
 * GOLD RUSH -- resources
 * The class also owns a walk PATH built from a 5-segment polyline in .rdata
 * ({-2,0},{0,-6},{-6,0},{0,1},{3,0}, in map tiles): 0x00412100 walks the
 * segments, sums their integer lengths and allocates 12 bytes per unit step
 * plus 8. 0x00412290 is its free. The +0xac handler ends by draining the
 * per-placement record list, which it reaches as a tail jump.
 * ========================================================================== */

/* The polyline descriptor at 0x004b4608: {count, points}. */
typedef struct PolyLine {
    int   count;                 /* +0x00 */
    Pos*  pts;                   /* +0x04 */
} PolyLine;

extern void* BuildWalkPath(PolyLine* poly);   /* 0x00412100 */
extern void  FreeWalkPath(void* path);        /* 0x00412290 */
extern void  GoldRush_FreeAllRecords(void);   /* 0x004069c0 */

extern PolyLine g_goldrush_polyline;          /* 0x004b4608 */

// FUNCTION: LEGOLAND 0x00406a10
void GoldRush_Create(RideElem* elem)
{
    g_gold_def = elem->data;
    if (g_gold_def) {
        g_gold_def->flags |= 0x20;
        if (g_gold_def->layers) {
            ((Spr*)g_gold_def->layers)->flags |= 0x2000;
            g_gold_layers = g_gold_def->layers;
        }
    }
    g_gold_path = BuildWalkPath(&g_goldrush_polyline);
    g_gold_matte1 = LoadSprite("goldwashmatte1.lls", 1);
    g_gold_wash = LoadSprite("goldwash.lls", 1);
    g_gold_matte2 = LoadSprite("goldwashmatte2.lls", 1);
    g_gold_mask = LoadSprite("goldmask.lls", 1);
}

// FUNCTION: LEGOLAND 0x00406ab0
void GoldRush_Destroy(void)
{
    if (g_gold_matte1)
        KillSprite(g_gold_matte1);
    if (g_gold_wash)
        KillSprite(g_gold_wash);
    if (g_gold_matte2)
        KillSprite(g_gold_matte2);
    if (g_gold_mask)
        KillSprite(g_gold_mask);
    if (g_gold_path)
        FreeWalkPath(g_gold_path);
    GoldRush_FreeAllRecords();
}

/* ==========================================================================
 * +0xb0 -- DEPTH-SORTED OVERLAY DRAW
 * Render every rider standing on THIS placement (matched by the packed map
 * square), sorted by the 3D person's depth key, then paint the class's matte
 * sprites over them at the object's screen position offset by a numbered
 * layer of the build sprite.
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00402d00
void CastleLevel1_Draw(RideElem* elem, int x, int y, MapSquare* sq,
                       void* clip, int mode)
{
    RideObject* item = elem->data;
    Offset      screen;
    Offset      off;
    RiderNode*  rd;

    RenderItems_New();
    g_castle1_list.head = 0;
    for (rd = item->riders; rd; rd = rd->next) {
        if (*(unsigned short*)sq == rd->ride_id)
            AddBlokeToRenderList(&g_castle1_list, rd, rd->person->depth);
    }
    RenderBlokeList(&g_castle1_list);
    if (g_castle1_matte) {
        screen = GetScreenCoordsForObject(sq, item);
        off = GetRenderOffsetForLayer(item->layers, 2);
        AdjustOffsetForViewMode(&off);
        PrintSprite(g_castle1_matte, screen.ox + off.ox, screen.oy + off.oy,
                    mode, 0);
    }
}

// FUNCTION: LEGOLAND 0x00416a60
void Temple_Draw(RideElem* elem, int x, int y, MapSquare* sq,
                 void* clip, int mode)
{
    RideObject* item = elem->data;
    Offset      screen;
    Offset      off;
    RiderNode*  rd;

    RenderItems_New();
    g_temple_list.head = 0;
    for (rd = item->riders; rd; rd = rd->next) {
        if (*(unsigned short*)sq == rd->ride_id)
            AddBlokeToRenderList(&g_temple_list, rd, rd->person->depth);
    }
    RenderBlokeList(&g_temple_list);
    screen = GetScreenCoordsForObject(sq, item);
    off = GetRenderOffsetForLayer(g_temple_layers, 0);
    AdjustOffsetForViewMode(&off);
    PrintSprite(g_temple_matte1, off.ox + screen.ox, off.oy + screen.oy,
                mode, 0);
    off = GetRenderOffsetForLayer(g_temple_layers, 3);
    AdjustOffsetForViewMode(&off);
    PrintSprite(g_temple_matte2, off.ox + screen.ox, off.oy + screen.oy,
                mode, 0);
}

/* ==========================================================================
 * 0x004062c0 -- FORT, +0xb0.
 *
 * The fort is drawn in two depth bands split by the visitor's WORLD X: the
 * class's base square plus the placement's own x, minus 4 tiles, is the
 * dividing line. Everyone at or west of it is drawn first, then the fort's
 * own layer 2 (its animation STOPPED first, so the wall does not flicker),
 * then everyone east of it, then fortmask.lls -- whose animation frame is
 * copied out of layer 2's LLS so the mask stays in step with the wall.
 *
 * The mask is printed at a FIXED screen offset (0x173, -0x7b) run through
 * AdjustOffsetForViewMode, not at a layer's render offset: it is a whole
 * separate piece of art registered by hand.
 * ========================================================================== */

extern void*  GetSpriteForLayer(RenderObj* obj, int layer);          /* 0x00441ec0 */
extern void*  GetLLSForSprite(void* spr);                            /* 0x00441e80 */
#ifndef LEGOLAND_PORTABLE
extern void   LLSStop(void* lls);                                    /* 0x0047d4c0 */
#else
extern int LLSStop(void* lls);                                    /* 0x0047d4c0 */
#endif
extern void   LLSSetFrame(void* lls, int frame);                     /* 0x0047d5a0 */

typedef struct FortBloke {
    unsigned char pad00[0x68];
    int           world_x;       /* +0x68  24.8 world position */
} FortBloke;

extern RenderList g_fort_list;       /* 0x004c11e0 */

// FUNCTION: LEGOLAND 0x004062c0
void Fort_Draw(RideElem* elem, int x, int y, MapSquare* sq,
               void* clip, int mode)
{
    RideObject* item = elem->data;
    Offset      screen;
    RiderNode*  rd;
    FortBloke*  b;
    void*       spr;
    void*       lls;
    int         px;
    int         frame;

    screen = GetScreenCoordsForObject(sq, item);
    px = item->base_x + sq->bx;

    RenderItems_New();
    g_fort_list.head = 0;
    for (rd = item->riders; rd; rd = rd->next) {
        if (*(unsigned short*)sq == rd->ride_id) {
            b = (FortBloke*)rd->bloke;
            if ((b->world_x >> 8) <= px - 4)
                AddBlokeToRenderList(&g_fort_list, rd, rd->person->depth);
        }
    }
    RenderBlokeList(&g_fort_list);

    if (g_fort_layers) {
        Offset off;
        spr = GetSpriteForLayer(g_fort_layers, 2);
        if (spr) {
            lls = GetLLSForSprite(spr);
            if (lls)
                LLSStop(lls);
        }
        off = GetRenderOffsetForLayer(g_fort_layers, 2);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(g_fort_layers, 2),
                    screen.ox + off.ox, screen.oy + off.oy, mode, 0);
    }

    RenderItems_New();
    g_fort_list.head = 0;
    for (rd = item->riders; rd; rd = rd->next) {
        if (*(unsigned short*)sq == rd->ride_id) {
            b = (FortBloke*)rd->bloke;
            if ((b->world_x >> 8) > px - 4)
                AddBlokeToRenderList(&g_fort_list, rd, rd->person->depth);
        }
    }
    RenderBlokeList(&g_fort_list);

    if (g_fort_mask) {
        Offset off;
        off.ox = 0x173;
        off.oy = -0x7b;
        AdjustOffsetForViewMode(&off);
        spr = GetSpriteForLayer(g_fort_layers, 2);
        if (spr && (lls = GetLLSForSprite(spr)) != 0)
            frame = *(short*)lls;
        else
            frame = mode;
        lls = GetLLSForSprite(g_fort_mask);
        if (lls)
            LLSSetFrame(lls, frame);
        PrintSprite(g_fort_mask, off.ox + screen.ox, off.oy + screen.oy,
                    mode, 0);
    }
}

/* ==========================================================================
 * GOLD RUSH -- the per-placement record and the four path tiles
 *
 * A GoldRec is 0x2c bytes, zeroed at birth, keyed by the packed {x,y} map
 * square at +0x00 and pushed on the head of the list at 0x004c1204 with
 * `next` at +0x0c (0x00406920). ridesave.c's SaveGoldWash/LoadGoldWash walk
 * exactly that list.
 *
 * Placing a GOLD RUSH also PAVES four squares -- the approach to the wash --
 * relative to the class's base square (ObjDef +0x0c/+0x10) plus the
 * placement's own square:
 *
 *      (bx-1, by  )   (bx-2, by  )   (bx-2, by-1)   (bx-2, by-2)
 *
 * with the loaded path tile's code (the first u16 of the record the pointer
 * at 0x00832bf0 points at). Removing it tears down the same four with
 * RemoveRollerCoasterPath, which -- see maprestore.c -- does the full path
 * removal and pays no refund. Note the ASYMMETRY, reproduced: paving is
 * graphics-only (AddPathTileGFX), unpaving is the full path removal.
 * ========================================================================== */

/* The packed map square a placement is keyed by. */
typedef union RideTile {
    unsigned short key;
    struct { unsigned char x, y; } b;
} RideTile;

typedef struct GoldRec GoldRec;



extern void     GoldRush_NewRecord(RideTile* square);  /* 0x00406920 */
extern void     GoldRush_FreeRecord(GoldRec* rec);     /* 0x00406960 */
extern GoldRec* GoldRush_FindRecord(RideTile* square); /* 0x004069e0 */

extern void  AddPathTileGFX(Pos* p, unsigned short tile);     /* 0x0045d350 */
extern void  RemoveRollerCoasterPath(Pos* p);                 /* 0x0045dcd0 */
extern void** g_path_tile_ptr;                                /* 0x00832bf0 */

// FUNCTION: LEGOLAND 0x004075f0
void GoldRush_Place(void* obj, Pos* pos)
{
    RideTile    tile;
    RideObject* item;
    Pos         p;

    tile.b.x = (unsigned char)pos->x;
    tile.b.y = (unsigned char)pos->y;
    item = ((RideElem*)obj)->data;
    AddBasicObject(obj, pos);
    GoldRush_NewRecord(&tile);

    p.x = pos->x + item->base_x - 1;
    p.y = pos->y + item->base_y;
    AddPathTileGFX(&p, *(unsigned short*)g_path_tile_ptr);
    p.x = pos->x + item->base_x - 2;
    p.y = pos->y + item->base_y;
    AddPathTileGFX(&p, *(unsigned short*)g_path_tile_ptr);
    p.x = pos->x + item->base_x - 2;
    p.y = pos->y + item->base_y - 1;
    AddPathTileGFX(&p, *(unsigned short*)g_path_tile_ptr);
    p.x = pos->x + item->base_x - 2;
    p.y = pos->y + item->base_y - 2;
    AddPathTileGFX(&p, *(unsigned short*)g_path_tile_ptr);
}

// FUNCTION: LEGOLAND 0x004076e0
void GoldRush_Remove(void* obj, unsigned int tile, void* ctx)
{
    RideObject* item = ((RideElem*)obj)->data;
    MapSquare*  t = (MapSquare*)&tile;
    GoldRec*    rec;
    Pos         p;

    rec = GoldRush_FindRecord((RideTile*)&tile);
    if (rec)
        GoldRush_FreeRecord(rec);
    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(item, tile);

    p.x = t->bx + item->base_x - 1;
    p.y = t->by + item->base_y;
    RemoveRollerCoasterPath(&p);
    p.x = t->bx + item->base_x - 2;
    p.y = t->by + item->base_y;
    RemoveRollerCoasterPath(&p);
    p.x = t->bx + item->base_x - 2;
    p.y = t->by + item->base_y - 1;
    RemoveRollerCoasterPath(&p);
    p.x = t->bx + item->base_x - 2;
    p.y = t->by + item->base_y - 2;
    RemoveRollerCoasterPath(&p);
}

/* ==========================================================================
 * +0xa8 -- THE PER-TICK RIDER STATE MACHINE
 *
 * Called once a frame with the class element; walks the WHOLE class's rider
 * list and steps each visitor's action byte, but only while that visitor's
 * low-level AI is idle (Bloke +0x0e == 0). The packed {x,y} of the placement
 * the rider is using lives at RiderNode +0x0c, and the class's base offsets
 * at ObjDef +0x0c/+0x10, so `px`/`py` below is the tile the ride's art is
 * anchored at.
 * ========================================================================== */

typedef struct Pos8 { int x, y; } Pos8;

struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;        /* +0x0e  low-level AI state (0 = idle) */
    unsigned char  pad10[0x24 - 0x10];
    Pos8           target;       /* +0x24  walk target, 24.8 */
    unsigned char  pad2c[0x34 - 0x2c];
    signed char    path_dir;     /* +0x34  walk-path step direction */
    unsigned char  pad35;
    unsigned char  pan;          /* +0x36  which pan slot the visitor has */
    unsigned char  pad37;
    short          path_node;    /* +0x38  current walk-path node */

    short          idle;         /* +0x3a  "stand around" countdown, ticks */
    unsigned char  pad3c[4];
    short          f40;          /* +0x40  in-fort sub-state */
    short          f42;          /* +0x42 */
    unsigned char  pad44[0x58 - 0x44];
    int            wander;       /* +0x58  ticks until the next random turn */
    unsigned char  pad5c[4];
    unsigned char  action;       /* +0x60  state-machine step */
    unsigned char  pad61;
    unsigned short flags62;      /* +0x62  8 = using this ride */
    unsigned char  pad64[4];
    Pos8           world;        /* +0x68  world position, 24.8 */
    unsigned char  pad70[2];
    unsigned char  dir;          /* +0x72  facing */
    unsigned char  new_dir;      /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14];   /* +0x98  CalcMoveLine scratch */
};

extern int  CalcMoveLine(Pos8 from, Pos8 to, void* path);            /* 0x00480740 */
extern int  NewDirForAction(Bloke* b, unsigned char dir);            /* 0x004833d0 */
extern void RemoveBlokeFromRide(RideObject* item, RiderNode* r);     /* 0x0048a100 */
extern int  rand(void);                                              /* 0x0049e4b2 (CRT) */

/* --------------------------------------------------------------------------
 * 0x00402dc0 -- CASTLE LEVEL 1, +0xa8.
 *
 * A castle visitor's seven steps. (px,py) is the castle's anchor tile.
 *   0  walk to the gate, tile (px-7, py-1)
 *   1  walk to the same tile jittered by a random half-tile in each axis
 *   2  pick a random idle time (1..256 ticks) and a random facing, and a
 *      random 16..47-tick wander timer
 *   3  stand in the courtyard: count the idle timer down (when it expires
 *      move on) and re-roll the facing every time the wander timer expires
 *   4  walk to (px-6.5, py-0.5) -- the doorway
 *   5  walk back to the centre of the castle's own square
 *   6  leave the ride
 *
 * TWO ORIGINAL QUIRKS, both reproduced:
 *  - the random half-tile jitter in step 1 maps rand()&3 as 1->0, 0->-128,
 *    2->+128 and leaves 3 AS 3, so one roll in four nudges the target by
 *    three 1/256ths of a tile instead of half a tile.
 *  - step 2 writes the wander timer through `rd->bloke` and the facing
 *    through the cached `b`; they are the same object.
 * -------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00402dc0
void CastleLevel1_TickRiders(RideElem* elem)
{
    RideObject*   item = elem->data;
    RiderNode*    rd;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           px;
    int           py;
    int           rx;
    int           ry;
    unsigned char a;

    rd = item->riders;
    while (rd) {
        next = rd->next;
        key = (MapSquare*)&rd->ride_id;
        b = rd->bloke;
        px = item->base_x + key->bx;
        py = item->base_y + key->by;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x = (px - 7) << 8;
                b->target.y = (py << 8) - 0x100;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 1:
                rx = rand() & 3;
                ry = rand() & 3;
                if (rx == 1)
                    rx = 0;
                else if (rx == 0)
                    rx = -0x80;
                else if (rx == 2)
                    rx = 0x80;
                if (ry == 1)
                    ry = 0;
                else if (ry == 0)
                    ry = -0x80;
                else if (ry == 2)
                    ry = 0x80;
                b->target.x = ((px - 7) << 8) + rx;
                b->target.y = (py << 8) + ry - 0x100;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 2:
                rd->bloke->wander = (rand() & 0x1f) + 0x10;
                b->dir = (unsigned char)(rand() & 7);
                b->idle = (short)((rand() & 0xff) + 1);
                b->action++;
                break;

            case 3:
                b->idle--;
                if (b->idle <= 0)
                    b->action++;
                b->wander--;
                if (b->wander <= 0) {
                    b->wander = (rand() & 0x1f) + 0x10;
                    b->dir = (unsigned char)(rand() & 7);
                }
                break;

            case 4:
                b->target.x = (px << 8) - 0x680;
                b->target.y = (py << 8) - 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 5:
                b->target.x = (px << 8) + 0x80;
                b->target.y = (py << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 6:
                RemoveBlokeFromRide(item, rd);
                b->flags62 &= ~8;
                break;
            }
        }

        rd = next;
    }
}

/* --------------------------------------------------------------------------
 * 0x00406660 -- FORT, +0xa8.
 *
 * Unlike the other three this one also ANIMATES the building: before it walks
 * the riders it advances layer 2's LLS by one frame, wrapping at the LLS's
 * own frame count (LLS record: current frame u16 @ +0x00, frame count u16
 * @ +0x10). Fort_Draw then copies that frame onto fortmask.lls, which is why
 * the wall and its mask stay in step.
 *
 * The visitor's five steps:
 *   0  walk to (px-4, py) -- the gate -- and set the in-fort sub-state
 *      (Bloke +0x40) to 2
 *   1  hand off to Fort_StepVisitor (0x004064d0), the in-fort behaviour that
 *      switches on that sub-state: 2 = pick what to do next (a 1-in-2 chance
 *      of a random facing and a 3..18-tick timer, 1-in-4 of stopping, 1-in-4
 *      of advancing the rider's action), 1 = walk between the fixed points in
 *      the .rdata table at 0x004b4580, 3 = ...
 *   2  walk to the CENTRE of the placement's own map square (not the anchor)
 *   3  walk to the centre of the anchor square
 *   4  leave the ride
 * -------------------------------------------------------------------------- */

typedef struct Lls {
    short         frame;         /* +0x00  current frame */
    unsigned char pad02[0x10 - 2];
    short         nframes;       /* +0x10  frames in the animation */
} Lls;

extern void Fort_StepVisitor(RiderNode* rd, Bloke* b);               /* 0x004064d0 */

// FUNCTION: LEGOLAND 0x00406660
void Fort_TickRiders(RideElem* elem)
{
    RideObject*   item = elem->data;
    RiderNode*    rd;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    Lls*          lls;
    void*         spr;
    int           kx;
    int           px;
    int           py;
    unsigned char a;

    spr = GetSpriteForLayer(g_fort_layers, 2);
    if (spr) {
        lls = (Lls*)GetLLSForSprite(spr);
        if (lls) {
            lls->frame++;
            if (lls->frame >= lls->nframes)
                lls->frame = 0;
        }
    }

    rd = item->riders;
    while (rd) {
        next = rd->next;
        b = rd->bloke;
        key = (MapSquare*)&rd->ride_id;
        kx = key->bx;
        px = item->base_x + kx;
        py = item->base_y + key->by;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x = (px - 4) << 8;
                b->target.y = py << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->f40 = 2;
                b->action++;
                break;

            case 1:
                Fort_StepVisitor(rd, b);
                break;

            case 2:
                b->target.x = (kx << 8) + 0x80;
                b->target.y = (key->by << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 3:
                b->target.x = (px << 8) + 0x80;
                b->target.y = (py << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 4:
                RemoveBlokeFromRide(item, rd);
                b->flags62 &= ~8;
                break;
            }
        }

        rd = next;
    }
}

/* --------------------------------------------------------------------------
 * 0x00416b50 -- TEMPLE, +0xa8.
 *
 * The temple has no per-instance record and no sub-behaviour helper: a
 * visitor simply walks a FIXED ELEVEN-STEP ROUTE of waypoints measured from
 * the class's anchor tile (px,py), then leaves. In tiles, with the 24.8
 * fractions written out:
 *
 *   0 (px-2, py-4)      claim the ride first (flags |= 8)
 *   1 (px-2, py-6.5)    2 (px-2.375, py-8)   3 (px-2, py-9.5)
 *   4 (px-2.375, py-12) 5 (px-2, py-9.5)     6 (px-2.375, py-8)
 *   7 (px-2, py-6.5)    8 (px-2, py-4)
 *   9 (px+0.5, py+0.5)  -- back to the centre of the anchor square
 *  10 leave the ride
 *
 * i.e. steps 1..4 climb the temple and 5..8 retrace the same points back
 * down: it is a there-and-back-again staircase walk.
 *
 * VC6 pins the constant 7 (the "walking" AI state every waypoint sets) in
 * ebp for the whole function -- the hoisted-constant lever, available here
 * because ebp is pushed anyway.
 * -------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00416b50
void Temple_TickRiders(RideElem* elem)
{
    RideObject*   item = elem->data;
    RiderNode*    rd;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           px;
    int           py;
    unsigned char a;

    rd = item->riders;
    while (rd) {
        next = rd->next;
        b = rd->bloke;
        key = (MapSquare*)&rd->ride_id;
        px = item->base_x + key->bx;
        py = item->base_y + key->by;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x = (px - 2) << 8;
                b->target.y = (py - 4) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 1:
                b->target.x = (px - 2) << 8;
                b->target.y = (py << 8) - 0x680;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 2:
                b->target.x = (px << 8) - 0x260;
                b->target.y = (py - 8) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 3:
                b->target.x = (px - 2) << 8;
                b->target.y = (py << 8) - 0x980;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 4:
                b->target.x = (px << 8) - 0x260;
                b->target.y = (py - 12) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 5:
                b->target.x = (px - 2) << 8;
                b->target.y = (py << 8) - 0x980;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 6:
                b->target.x = (px << 8) - 0x260;
                b->target.y = (py - 8) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 7:
                b->target.x = (px - 2) << 8;
                b->target.y = (py << 8) - 0x680;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 8:
                b->target.x = (px - 2) << 8;
                b->target.y = (py - 4) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 9:
                b->target.x = (px << 8) + 0x80;
                b->target.y = (py << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 10:
                RemoveBlokeFromRide(item, rd);
                b->flags62 &= ~8;
                break;
            }
        }

        rd = next;
    }
}

/* --------------------------------------------------------------------------
 * 0x004072b0 -- GOLD RUSH, +0xa8: the gold-panning state machine.
 *
 * The richest of the four. GOLD RUSH is the only class in this file with
 * per-placement state (the GoldRec list) AND a walk path: the 5-segment
 * polyline its +0xa4 built is what the visitor follows to the creek and back,
 * stepped one node a tick by WalkPath_Step. b->+0x34 is the step direction
 * (+1 forward, -1 back) and b->+0x38 the current node, so the SAME path is
 * walked out (started at node 0) and home (started at the last node).
 *
 * Fourteen steps:
 *   0  claim the ride, take a free pan slot out of the record (GoldRec +0x14,
 *      six slots), refresh the "no more customers" flag on the placement, and
 *      start the walk path at its first node
 *   1  follow the path; when the visitor stands exactly on (kx+5, ky-3) --
 *      the creek's edge -- switch to the walking-with-a-pan animation
 *   2  step the action on FIRST, then walk to (kx+4.5, ky-1.5)
 *   3, 8  take up the panning pose at the assigned pan
 *   4  shuffle to the pan's near edge
 *   5  kneel at the pan and roll a 15..45-tick panning timer
 *   6  pan: tick the animation down; this is the only step that does NOT
 *      advance the action itself
 *   7  stand up again
 *   9  walk back to (kx+4.5, ky-1.5)
 *  10  start the same path at its LAST node (walk it in reverse)
 *  11  follow it home; back at the creek edge, drop the pan animation
 *  12  walk to the centre of the anchor square
 *  13  leave the ride
 *
 * Note kx/ky (the placement's own square) and px/py (that plus the class's
 * base offsets) are BOTH used: the pan waypoints are measured from the raw
 * square, the path steps and the exit from the anchor.
 * -------------------------------------------------------------------------- */

extern void GoldRush_ClaimPan(RiderNode* rd, MapSquare* key);        /* 0x00406ec0 */
extern void GoldRush_UpdateFullFlag(MapSquare* key);                 /* 0x00406f30 */
extern void GoldRush_PoseAtPan(RiderNode* rd, MapSquare* key);       /* 0x00406f60 */
extern void GoldRush_MoveToPanEdge(RiderNode* rd, MapSquare* key);   /* 0x00407000 */
extern void GoldRush_KneelAtPan(RiderNode* rd, MapSquare* key);      /* 0x004070b0 */
extern void GoldRush_StandUpFromPan(RiderNode* rd, MapSquare* key);  /* 0x00407170 */
extern void GoldRush_RollPanTimer(RiderNode* rd);                    /* 0x00407230 */
extern void GoldRush_PanTick(RiderNode* rd);                         /* 0x00407250 */

extern void WalkPath_StartAtEnd(void* path, Bloke* b);               /* 0x004122a0 */
extern void WalkPath_StartAtBegin(void* path, Bloke* b);             /* 0x004122d0 */
extern void WalkPath_Step(void* path, int px, int py, Bloke* b);     /* 0x00412300 */

extern void BlokeWalkAnim(Bloke* b);                                 /* 0x00440910 */
extern void BlokeWalkWithPan(Bloke* b);                              /* 0x00440960 */

// FUNCTION: LEGOLAND 0x004072b0
void GoldRush_TickRiders(RideElem* elem)
{
    RideObject*   item = elem->data;
    RiderNode*    rd;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           px;
    int           py;
    unsigned char act;
    unsigned char a;

    rd = item->riders;
    while (rd) {
        next = rd->next;
        key = (MapSquare*)&rd->ride_id;
        b = rd->bloke;
        px = key->bx + item->base_x;
        py = key->by + item->base_y;

        if (b->state == 0) {
            act = b->action;
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                GoldRush_ClaimPan(rd, key);
                GoldRush_UpdateFullFlag(key);
                WalkPath_StartAtBegin(g_gold_path, b);
                break;

            case 1:
                WalkPath_Step(g_gold_path, px, py, b);
                if ((b->world.x >> 8) == key->bx + 5 &&
                    (b->world.y >> 8) == key->by - 3)
                    BlokeWalkWithPan(b);
                break;

            case 2:
                b->action = (unsigned char)(act + 1);
                b->target.x = (key->bx << 8) + 0x480;
                b->target.y = (key->by << 8) - 0x180;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                break;

            case 3:
            case 8:
                GoldRush_PoseAtPan(rd, key);
                b->action++;
                break;

            case 4:
                GoldRush_MoveToPanEdge(rd, key);
                b->action++;
                break;

            case 5:
                GoldRush_KneelAtPan(rd, key);
                GoldRush_RollPanTimer(rd);
                b->action++;
                break;

            case 6:
                GoldRush_PanTick(rd);
                break;

            case 7:
                GoldRush_StandUpFromPan(rd, key);
                b->action++;
                break;

            case 9:
                b->target.x = (key->bx << 8) + 0x480;
                b->target.y = (key->by << 8) - 0x180;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 10:
                WalkPath_StartAtEnd(g_gold_path, b);
                break;

            case 11:
                WalkPath_Step(g_gold_path, px, py, b);
                if ((b->world.x >> 8) == key->bx + 5 &&
                    (b->world.y >> 8) == key->by - 3)
                    BlokeWalkAnim(b);
                break;

            case 12:
                b->target.x = (px << 8) + 0x80;
                b->target.y = (py << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 13:
                RemoveBlokeFromRide(item, rd);
                b->flags62 &= ~8;
                break;
            }
        }

        rd = next;
    }
}

/* --------------------------------------------------------------------------
 * 0x00406b10 -- GOLD RUSH, +0xb0: the four-band overlay draw.
 *
 * Four passes over the class's rider list, each followed by one piece of the
 * wash's foreground art, so the visitors are sliced correctly into the
 * creek. The bands are cut by WORLD POSITION against the placement's own
 * square (kx,ky), in 24.8 units:
 *
 *   1  x <= kx+7.5 and y <= ky-2.5    then goldmask.lls
 *   2  x <= kx+7.5 and y >= ky-2.5    then goldwashmatte2.lls
 *   3  x >  kx+7.5                     then goldwashmatte1.lls
 *   4  x/256 > kx+8                    then layer 3 of the build sprite
 *
 * goldmask.lls is ANIMATED in step with the build sprite: its frame is layer
 * 1's current LLS frame taken MODULO 8 -- signed modulo, so the whole
 * and/jns/dec/or/inc sequence is `frame % 8` on a value VC6 cannot prove
 * non-negative. goldwash.lls, the fourth sprite the +0xa4 loads, is not used
 * by the draw at all.
 * -------------------------------------------------------------------------- */

extern RenderList g_gold_list;       /* 0x004c1208 */

// FUNCTION: LEGOLAND 0x00406b10
void GoldRush_Draw(RideElem* elem, int x, int y, MapSquare* sq,
                   void* clip, int mode)
{
    RideObject* item = elem->data;
    Offset      screen;
    RiderNode*  rd;
    Bloke*      b;
    void*       spr;
    Lls*        lls;
    int         frame;

    screen = GetScreenCoordsForObject(sq, item);

    RenderItems_New();
    g_gold_list.head = 0;
    for (rd = item->riders; rd; rd = rd->next) {
        if (*(unsigned short*)sq == rd->ride_id) {
            b = rd->bloke;
            if (b->world.x <= (sq->bx << 8) + 0x780 &&
                b->world.y <= (sq->by << 8) - 0x280)
                AddBlokeToRenderList(&g_gold_list, rd, rd->person->depth);
        }
    }
    RenderBlokeList(&g_gold_list);

    if (g_gold_mask) {
        Offset off;

        frame = 0;
        spr = GetSpriteForLayer(g_gold_layers, 1);
        if (spr) {
            lls = (Lls*)GetLLSForSprite(spr);
            if (lls)
                frame = lls->frame % 8;
        }
        lls = (Lls*)GetLLSForSprite(g_gold_mask);
        if (lls)
            LLSSetFrame(lls, frame);
        off = GetRenderOffsetForLayer(g_gold_layers, 1);
        AdjustOffsetForViewMode(&off);
        PrintSprite(g_gold_mask, screen.ox + off.ox, screen.oy + off.oy,
                    mode, 0);
    }

    RenderItems_New();
    g_gold_list.head = 0;
    for (rd = item->riders; rd; rd = rd->next) {
        if (*(unsigned short*)sq == rd->ride_id) {
            b = rd->bloke;
            if (b->world.x <= (sq->bx << 8) + 0x780 &&
                b->world.y >= (sq->by << 8) - 0x280)
                AddBlokeToRenderList(&g_gold_list, rd, rd->person->depth);
        }
    }
    RenderBlokeList(&g_gold_list);

    if (g_gold_matte2) {
        Offset off;

        off = GetRenderOffsetForLayer(g_gold_layers, 1);
        AdjustOffsetForViewMode(&off);
        PrintSprite(g_gold_matte2, off.ox + screen.ox, off.oy + screen.oy,
                    mode, 0);
    }

    RenderItems_New();
    g_gold_list.head = 0;
    for (rd = item->riders; rd; rd = rd->next) {
        if (*(unsigned short*)sq == rd->ride_id) {
            b = rd->bloke;
            if (b->world.x > (sq->bx << 8) + 0x780)
                AddBlokeToRenderList(&g_gold_list, rd, rd->person->depth);
        }
    }
    RenderBlokeList(&g_gold_list);

    if (g_gold_matte1) {
        Offset off;

        off = GetRenderOffsetForLayer(g_gold_layers, 1);
        AdjustOffsetForViewMode(&off);
        PrintSprite(g_gold_matte1, off.ox + screen.ox, off.oy + screen.oy,
                    mode, 0);
    }

    RenderItems_New();
    g_gold_list.head = 0;
    for (rd = item->riders; rd; rd = rd->next) {
        if (*(unsigned short*)sq == rd->ride_id) {
            b = rd->bloke;
            if ((b->world.x >> 8) > sq->bx + 8)
                AddBlokeToRenderList(&g_gold_list, rd, rd->person->depth);
        }
    }
    RenderBlokeList(&g_gold_list);

    if (g_gold_layers) {
        Offset off;

        off = GetRenderOffsetForLayer(g_gold_layers, 3);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(g_gold_layers, 3),
                    off.ox + screen.ox, off.oy + screen.oy, mode, 0);
    }
}

/* ==========================================================================
 * 0x00402780 -- StepSchoolCar: one frame of one DRIVING SCHOOL car.
 *
 * coaster.c's TickSchoolCars calls this for every live car that is not being
 * retired. It does two jobs in one pass -- DRAW the car and MOVE it -- which
 * is why the projection comes first:
 *
 *  1. Project the car's 16.16 world position to the screen with the standard
 *     isometric solve (the same one anim2.c's BoatingSchool_DrawBoats uses):
 *         sy = ((wx>>8) + (wy>>8)) * th >> 9   - ScrollY>>8
 *         sx = ((wx>>8) - (wy>>8)) * tw >> 9   - (tw2+1)/2 - ScrollX>>8
 *     plus the car sprite's own per-heading offset (the image list at
 *     0x00830f9c, its dx/dy halved) and the map origin. GetTileDimensions is
 *     called TWICE with the same answer, exactly as the boating school does.
 *  2. Queue the car sprite with SortSpriteWithCallback, under the livery
 *     palette its heading byte (+0xc3) selects (1/2/3 -> 0x0082c6bc /
 *     0x0082c6b8 / 0x0082c690) and with the body frame +0xb8 forced as the
 *     override frame. The callback 0x00402550 draws the DRIVER inside the
 *     car, under the same palette, once the body has been blitted.
 *  3. Place the driver: the bloke's world position IS the car's screen
 *     position, its facing is (frame + 6) & 15, and its 3D person is put at
 *     (+0x10, +8) from there and given the matching 16-way heading.
 *  4. Move: if another car is too close ahead (0x00402490) the car stalls --
 *     halve its speed, count the stall up, and give up after 0x200 frames.
 *     Otherwise accelerate back towards its top speed in steps of 0x40, add
 *     the velocity, recompute the map square and, if the square refuses the
 *     car (0x00402430: a different road tile that is full), ROLL THE WHOLE
 *     MOVE BACK and return.
 *  5. Having moved, test whether the car crossed its manoeuvre target
 *     (+0x30/+0x34) in either axis this frame -- the sign of (target - now)
 *     differing from the sign of (target - before) -- and, if it did, ask the
 *     route planner (0x00401f30 when the horn timer is still running,
 *     0x00402150 otherwise) for the next manoeuvre code, then run one step of
 *     that manoeuvre. Code 5 is the only one that runs two manoeuvre steps.
 *
 * `moved` (initialised to 0 next to the road lookup) is the "this car did
 * something this frame" flag; a car that reaches the end without it gets
 * 0x00401cd0 -- the idle/park step.
 *
 * NOTE the dead store at 0x004027f3: the screen x is spilled into the
 * manoeuvre-target slot and overwritten eleven bytes later without ever
 * being read. It is VC6 scheduling, not behaviour.
 * ========================================================================== */

typedef struct CarPos { int x; int y; } CarPos;

/* The car state StepSchoolCar snapshots before it moves: the map square it
 * is on plus the manoeuvre target, kept as ONE aggregate (that is what pins
 * the four slots together in the original's frame). */
typedef struct CarSave { int x; int y; int tx; int ty; } CarSave;

typedef struct SchoolCar {
    struct SchoolCar* next;      /* +0x00 */
    unsigned short    school;    /* +0x04  the school's packed map square */
    unsigned char     pad06[2];
    int               sx;        /* +0x08  screen position, this frame */
    int               sy;        /* +0x0c */
    int               wx;        /* +0x10  world x, 16.16 */
    int               wy;        /* +0x14  world y, 16.16 */
    CarPos            cur;       /* +0x18  map square */
    CarPos            start;     /* +0x20  the square it started from */
    int               vx;        /* +0x28  velocity, 16.16 */
    int               vy;        /* +0x2c */
    int               tx;        /* +0x30  manoeuvre target, 16.16 */
    int               ty;        /* +0x34 */
    unsigned char     pad38[0xb8 - 0x38];
    unsigned char     b8;        /* +0xb8  body frame / heading, 0..15 */
    unsigned char     b9;        /* +0xb9  last drawn frame */
    unsigned char     ba;        /* +0xba */
    unsigned char     bb;        /* +0xbb  1 = must land exactly on the target */
    unsigned short    stall;     /* +0xbc  frames spent stalled */
    unsigned short    t_life;    /* +0xbe */
    unsigned short    t_horn;    /* +0xc0 */
    unsigned char     on_road;   /* +0xc2 */
    unsigned char     dir;       /* +0xc3  livery, 1..3 */
    unsigned char     manoeuvre; /* +0xc4  current manoeuvre code, 1..5 */
    unsigned char     c5;        /* +0xc5 */
    unsigned short    top_speed; /* +0xc6 */
    unsigned short    speed;     /* +0xc8 */
    unsigned char     padca[2];
    Bloke*            bloke;     /* +0xcc  the driver */
} SchoolCar;                     /* 0xd0 */

/* The map header: the screen origin of cell (0,0) (lpConfig @0x004bcbf4). */
typedef struct MapHdr {
    unsigned char  pad00[0x20];
    unsigned short origin_x;     /* +0x20 */
    unsigned short origin_y;     /* +0x22 */
} MapHdr;

/* A loaded image list (the LLIDB .ILF descriptor), dx/dy doubled at load. */
typedef struct ImageList {
    unsigned char pad00[8];
    void**        sprites;       /* +0x08 */
    int*          dx;            /* +0x0c */
    int*          dy;            /* +0x10 */
} ImageList;

/* The 3D person record, as this function touches it. */
typedef struct Person3D {
    unsigned char pad00[0x1c];
    int           sx;            /* +0x1c  screen position */
    int           sy;            /* +0x20 */
    unsigned char pad24[0x54 - 0x24];
    int           depth;         /* +0x54  render sort key */
} Person3D;

/* The blit context SortSpriteWithCallback copies into the print item. */
typedef struct BlitCtx {
    int   kind;                  /* +0x00  0x306 = a driving-school car */
    void* owner;                 /* +0x04  the driver */
    int   f08;                   /* +0x08 */
} BlitCtx;

typedef struct RoadTile RoadTile;

extern void      GetTileDimensions(int* out_w, int* out_h);          /* 0x00460540 */
extern RoadTile* GetRoadRecord(int x, int y);                        /* 0x004125f0 */
extern void      SetOverridePalette(void* pal);                      /* 0x00464400 */
extern void      SetOverrideFrame(int frame);                        /* 0x00464420 */
extern void      ClearOverrideFrame(void);                           /* 0x00464440 */
extern void      ClearOverridePalette(void);                         /* 0x00464450 */
extern void      SortSpriteWithCallback(void* s, int x, int y, int key, int mode, void (*cb)(int), int cbarg, BlitCtx* ctx); /* 0x00485cd0 */
extern Person3D* Find3DPersonFromBloke(Bloke* b);                    /* 0x0043f890 */
extern void      AdjustBlokePosition(Pos8* p);                       /* 0x00442d60 */

extern void SchoolCar_DrawDriver(int c);                             /* 0x00402550 */
extern void SetPerson3DHeading(Person3D* p, int dir);                /* 0x004025d0 */
extern int  SchoolCarBlockedAhead(SchoolCar* c);                     /* 0x00402490 */
extern void SchoolCarAccelerate(SchoolCar* c);                       /* 0x004019c0 */
extern int  SchoolCarMayEnterSquare(CarPos* to, CarPos* from);       /* 0x00402430 */
extern int  SchoolCarNextManoeuvreHorn(unsigned short school, CarPos* start, int ba); /* 0x00401f30 */
extern int  SchoolCarNextManoeuvre(unsigned short school, CarPos* start, int ba); /* 0x00402150 */
extern int  SchoolCarAtTarget(SchoolCar* c);                         /* 0x00402390 */
extern void SchoolCarManoeuvreA(SchoolCar* c);                       /* 0x00401320 */
extern void SchoolCarManoeuvreB(SchoolCar* c);                       /* 0x004015e0 */
extern void SchoolCarManoeuvreC(SchoolCar* c);                       /* 0x00401080 */
extern void SchoolCarManoeuvreD(SchoolCar* c);                       /* 0x00401660 */
extern void SchoolCarIdleStep(SchoolCar* c);                         /* 0x00401cd0 */

extern MapHdr*    g_map;             /* 0x004bcbf4 lpConfig */
extern int        g_scroll_x;        /* 0x00667cb4 ScrollX (24.8) */
extern int        g_scroll_y;        /* 0x00667cb8 ScrollY (24.8) */
extern void*      g_car_sprite;      /* 0x00830f94 */
extern ImageList* g_car_images;      /* 0x00830f9c */
extern void*      g_car_pal_a;       /* 0x0082c690  livery 3 */
extern void*      g_car_pal_b;       /* 0x0082c6b8  livery 2 */
extern void*      g_car_pal_c;       /* 0x0082c6bc  livery 1 */

/* 351/351 instructions (was 345 -- the six-instruction shortfall is CLOSED),
 * 1151 bytes against 1147, 322 strict mismatches (was 330), and the frame
 * layout is the original's exactly: 0x38, with the first GetTileDimensions'
 * WIDTH out-param homed in the dead `c` argument slot (entry+0x04), `th` at
 * entry-0x38 doubling as the "this car did something" flag, tw2/th2 at -0x34
 * and -0x30, swx/swy at -0x2c/-0x28, off at -0x24/-0x20, saved at -0x1c/-0x18,
 * tx/ty at -0x14/-0x10 and the 0x306 blit context in the top twelve bytes.
 *
 * WHAT CLOSED THE SIX (2026-09-04).  The original does NOT fall through from
 * the blocked-ahead arm into the join: it branches FORWARD to the join when
 * `c->manoeuvre == 0` and keeps a full INLINE epilogue for the `manoeuvre != 0`
 * return (0x004028f6, `pop edi/esi/ebp/ebx; add esp,0x38; ret` -- exactly the
 * six instructions we were short), so the else arm is laid out before the join
 * and its own MayEnterSquare failure gets a second inline epilogue.  Spelling
 * the guard as `if (c->manoeuvre == 0) goto joint; return;` with `joint:` on
 * the tx/ty test reproduces both.  `goto joint` from the else arm as well, and
 * dropping the `else` altogether, give byte-identical code.
 *
 * THE RESIDUAL, precisely: one register tie-break in the isometric projection
 * at index 19, and the register renaming it cascades through the whole body
 * (315 of the 322 survive a register-blind compare, so almost nothing else is
 * structurally different).  The original computes the SUM first into a fresh
 * scratch register, which is only possible if `wy` is still live at that point
 * -- i.e. the difference is allocated after it:
 *      lea eax,[ebx+ebp] / mov edi,ebx / imul eax,th / sub edi,ebp /
 *      imul edi,tw / sar eax,9 / ... / mov ebp,eax / ... / sar edi,9
 * This build computes the difference first and folds the sum in place
 * (`add ebp,ebx`), which costs the `mov ebp,eax` copy.  Exactly TWO outcomes
 * exist across every spelling measured (~25 this round on top of the earlier
 * set): with `sx >>= 9` BEFORE `sy >>= 9` we get wy in ebp / wx in ebx (the
 * original's pair) and the in-place sum -- 322 strict, 351 instructions; with
 * `sy >>= 9` first we get the original's three-register `lea` but the pair
 * rotates to wy in ebx / wx in edi and one instruction is lost -- 330 strict,
 * 350 instructions.  Nothing reaches both.  Inert: `sy = wx + wy; sy *= th;`
 * and every other split of the add/multiply/shift; `sx = wx; sx -= wy;`;
 * a block with `int s`/`int d` temporaries; `(wy + wx)`; both statement
 * orders; both declaration orders of wx/wy and of sx/sy; loading `wx` before
 * `wy`.
 *
 * TWO SMALLER RESIDUALS, both measured this round and both left as they are:
 *  (a) VC6 reassociates the two `sx -=` steps into one `add`; the original
 *      keeps them apart with the car-image loads scheduled between.  Moving
 *      the two `off` stores does NOT stop it (the store-between-the-subs cure
 *      from `SpinningBarrels` does not reach this shape): every one of the
 *      eight orderings of {sy-=, sx-=, sx-=, off.x/off.y} still emits the
 *      merged `add`.  Two of them are structurally closer on the blind
 *      metrics without touching the strict count -- both `sx -=` first then
 *      `sy -=` then the `off` stores gives register+offset-blind 64 (this
 *      build: 104) at 1155 bytes, and putting `sy -=` last gives offset-blind
 *      95 (this build: 156) but loses an instruction -- so neither is adopted:
 *      the strict count is identical and the byte length gets worse.
 *  (b) the "did something" flag: the original caches `th` in EDI over the
 *      manoeuvre switch (`mov edi,1` with no memory store, plus a one
 *      instruction block `mov edi,[esp+0x10]` at 0x00402b56 reached only from
 *      the `c->bb != 0` branch, and `test edi,edi` at the end), where this
 *      build keeps it in memory throughout.  That is a consequence of the
 *      index-19 allocation, not a source shape: giving the flag its own local,
 *      or reusing `wx`/`wy` for it, adds a fifteenth frame slot (0x3c) and is
 *      worse (325); reusing `tw2`/`th2` keeps 0x38 but diverges at index 5
 *      (323).  The original's home for the flag IS `th`'s -0x38 slot, which
 *      is why `th` is reused for it below.
 * The original also SPILLS sx to `tx`'s home at -0x14 immediately after
 * `sar edi,9` and then overwrites it with `c->tx` -- a dead store this build
 * does not emit, and the one instruction we have spare elsewhere.
 * Behaviour, the frame, the SchoolCar record layout and the whole manoeuvre
 * dispatch are reconstructed; only the schedule differs.
 *
 * 2026-09-04, lane H.  Residual UNCHANGED at 322.  The "exactly two outcomes"
 * claim above was re-derived from scratch as a full CROSS PRODUCT rather than
 * a list of one-off spellings: {wy= before wx= | wx= before wy=} x {sy= before
 * sx= | sx= before sy=} x {sx>>=9 first | sy>>=9 first | both shifts folded
 * into the product expressions} = 12 builds, plus 11 more spellings
 * (fully-sequential per coordinate, an explicit `sx = wx; sx -= wy;` copy,
 * `wx + wy` split into a sum temp then multiplied, `th * (wx + wy)` with the
 * multiplier first, `(wy + wx)`, `wx + -wy`, `sx = sx >> 9`).  Every one of
 * the 23 lands on exactly one of the two known points:
 *     322 X / 351 insns / 1151B  -- wy in ebp, wx in ebx (the ORIGINAL'S
 *         PAIR, indices 0..18 exact) but the difference computed first, so
 *         the sum folds in place as `add ebp,ebx`;
 *     330 X / 350 insns / 1151B  -- the original's three-register
 *         `lea ebp,[edi+ebx]` for the sum, but wy rotates to ebx and wx to
 *         edi, first divergence 8, and the difference then folds in place
 *         instead (`sub edi,ebx`), losing an instruction.
 * The two are exactly complementary: whichever product is emitted SECOND is
 * the one VC6 coalesces with its operand's register, and the original has
 * NEITHER coalesced (`lea eax,[ebx+ebp]` for the sum into a scratch register,
 * `mov edi,ebx / sub edi,ebp` for the difference, then `mov ebp,eax` to move
 * the sum into wy's freed register).  Reaching that needs BOTH webs kept
 * separate from wx/wy, and the shift order is the only control we have over
 * which one is emitted first.  Committed body keeps the 322 point because it
 * has the original's register pair and its instruction count.
 *
 * 2026-09-04, lane M.  322 -> 320, and the "351/351 instructions" headline is
 * now known to have been TWO COMPENSATING ERRORS cancelling.
 *  - ADOPTED: the original's dead store at 0x004027f3 is now emitted, as
 *    `*(volatile int*)&tx = sx;` immediately before `tx = c->tx;`.  audit.py:
 *    mismatch 322 -> 320, LCS diff 198 -> 195, register-blind diff 150 -> 131.
 *    It costs one instruction (352 emitted, audit trims to 351) and four bytes
 *    (1151 -> 1155).  That is not a regression being hidden; the arithmetic
 *    says so.  The original is `ours + dead store + lea + mov ebp,eax
 *    - add ebp,ebx - E`, so with 351 == 351 before this change E was exactly
 *    TWO: we carry two instructions the original does not (residual (b), the
 *    flag kept in memory).  The old count match was those two extras cancelling
 *    the two missing projection instructions.  Emitting a real instruction of
 *    the original's exposes that, and every structural metric improved, so it
 *    is kept.  When index 19 is fixed the count returns to 351 (+1 for
 *    lea/mov-minus-add, -2 for the flag).
 *  - The phantom-home lever was measured in all its documented forms.  Only
 *    the write THROUGH AN EXISTING LOCAL works here: `*(volatile int*)&tx`
 *    before `tx = c->tx` gives 320/352.  A block-scope `struct { int x, y; }
 *    spill` (or the sy variant) RESERVES A REAL SLOT and re-lays the prologue
 *    -- first divergence 0, 328/330 -- because this frame is already the
 *    original's 0x38 with nothing spare; a one-member struct and a bare `int`
 *    spill are inert-but-costly (322/352, worse LCS than &tx); `&ty`, `&swx`
 *    and `&saved.x` all score worse than `&tx`.  Position matters: before the
 *    swx/swy/saved.y run it costs TWO instructions (353), after it costs one.
 *  - The "exactly two outcomes" result survives a THIRD, larger sweep (49
 *    builds this pass) and is now certain.  New spellings, all landing on one
 *    of the two points: a statement of the save block interleaved between the
 *    sum and the difference (all three of swx/swy/saved.y); `-(wy - wx)`; an
 *    explicit temp for the SUM whose live range straddles the difference
 *    (`int t = (wx+wy)*th; sx = (wx-wy)*tw; sx >>= 9; sy = t >> 9;` plus two
 *    variants) -- the strongest idea, because the original's `mov ebp,eax` IS
 *    a two-web copy, but VC6 flattens the temp anyway; an explicit temp for
 *    the DIFFERENCE (330 point); both temps; a volatile barrier between the
 *    two products (flips to the 330 point); and the full
 *    {sum-first|diff-first} x {sx>>=9|sy>>=9} x {separate|folded|inline}
 *    x {dead store|none} cross product.
 *  - WHAT THE TWO POINTS ACTUALLY ARE, stated so nobody re-derives it: the
 *    REGISTER ASSIGNMENT of our 322/320 point is already the original's
 *    exactly -- wx=ebx, wy=ebp, sx=edi, sy=ebp.  Only the EMISSION ORDER of
 *    the two products differs.  The original emits the sum first, so ebp still
 *    holds wy and the sum must go to a scratch (`lea eax,[ebx+ebp]`, then
 *    `mov ebp,eax` once wy dies).  We emit the difference first, so the sum
 *    folds in place (`add ebp,ebx`).  Every construct that makes THIS build
 *    emit the sum first also makes it re-assign wy to ebx and wx to edi, so
 *    the sum can be built straight into a free ebp -- that is the 330 point,
 *    first divergence 8.  The lever needed is one that forces the sum first
 *    WITHOUT freeing ebp, i.e. something that keeps wy live past the sum; no
 *    zero-cost spelling of that exists (a third use of wy costs an instruction
 *    the original does not have). */
/* ROUND OF 2026-09-04.  320 -> 319 strict, but the number that moved is the
 * STRUCTURAL one: register+offset-blind LCS 311 -> 324 of 351 and ORIGINAL
 * indices inside a differing region 69 -> 46 (permutation-aware "real"
 * 318 -> 317).  With a body this displaced the index-for-index count is nearly
 * meaningless -- 315+ of the mismatches are the same instructions at shifted
 * positions -- so rank on the register-blind alignment, not on it.
 * TWO CHANGES, both transferred from other files in this batch:
 *  1. The `c->sx` sum's two leading addends written into the fields of a
 *     non-address-taken `Pos` (the anim2.c BoatingSchool_DrawBoats lever).
 *     x only: 320 -> 319, robl 311 -> 318, bad 69 -> 58.  Protecting the y sum
 *     as well costs four strict (323) even though it reads better structurally
 *     (robl 319-320); one aggregate for x alone is the Pareto point.
 *  2. The order of the five statements between the second GetTileDimensions
 *     and AdjustOffsetForViewMode: the two `sx` adjustments FIRST and adjacent,
 *     then `sy`, then the two `off` fields.  All 60 legal orders were measured
 *     (off.x must precede off.y); this one is robl 324 / bad 46, the best, and
 *     it is also ordinary source.  The original's own EMISSION order is
 *     sy, sx-(tw2+1)>>1, sx-scroll_x, off.x, off.y -- writing exactly that
 *     scores robl 320 / bad 56, so VC6 reorders these freely and the emission
 *     does not pin the source.
 * MEASURED, BETTER ON STRICT, NOT SHIPPED: putting `sx -= g_scroll_x >> 8;`
 * BETWEEN the two `off` assignments is 313 strict / real 310 / robl 320-321 /
 * bad 54-56 (1155 bytes, closer than this build's 1159).  Splitting the two
 * `off` field stores is not a line any human wrote, and it wins on the two
 * metrics that a displacement distorts, so it stays out; record it here rather
 * than re-finding it.
 *
 * THE PROJECTION TIE-BREAK AT 19 IS NOW UNDERSTOOD, AND IT IS THE SAME LEVER
 * AS joust.c's TempleSlide_Update, running the OTHER WAY.  The original is
 *      lea eax,[ebx+ebp] / mov edi,ebx / imul eax,th / sub edi,ebp /
 *      imul edi,tw / sar eax,9 / mov ebp,eax
 * -- the SUM emitted first, into a third register (so `lea`), and the diff
 * built from a copy of wx; we emit `mov edi,ebx / sub edi,ebp / add ebp,ebx`,
 * the diff first with the sum overwriting wy's register.  WHAT DECIDES IT IS
 * THE ORDER OF THE TWO `>>= 9` STATEMENTS, not the order of the two products:
 * with `sy >>= 9;` before `sx >>= 9;` (or the products written with the shift
 * folded in) VC6 emits the sum first and produces the original's `lea` shape --
 * robl 316-326, bad 42-63, the best structural numbers seen on this function --
 * but the whole callee-saved assignment then rotates at index 8: `wy` takes
 * ebx and `wx` edi where the original (and this build) have ebp and ebx, and
 * the strict count goes to 326 with the first divergence moving from 19 to 8.
 * All four positions of `sx >>= 9;` among the following statements collapse to
 * the same result, so it is the ORDER of the two shifts and nothing else.
 * The mechanism: `sy` is live from the sum, `wy` until the diff, so the two
 * overlap; the original assigns `sy` the register `wy` is in (ebp) anyway and
 * pays for it with `mov ebp,eax` at 27, which is why the sum has to be
 * computed in a scratch.  Our build assigns `sy` a free register instead.
 * A future round wanting this function should attack that: sum-first emission
 * with `wy` still in ebp is the whole remaining difference at 19-34, and it is
 * worth ~13 register-blind indices plus the two copies the original has and we
 * do not (which is where most of the 12-byte overshoot lives).
 * PERMRANK PERMUTATION INVARIANCE (the sibling-lane diagnostic, applied here).
 * Across every variant that touches the projection block, the `c->s?` sums or
 * the five-statement order -- roughly 80 builds -- the best callee-saved
 * permutation stays `ebx->esi ebp->edi edi->ebx esi->ebp`.  It moves in exactly
 * two places: the `>>= 9` shift-order family (to `ebx->ebp ebp->edi edi->ebx`)
 * and the one statement order that splits the two `off` stores.  By the
 * invariance rule that means the projection block itself is the WRONG place to
 * look: the webs, references and live ranges are the same in all of those
 * builds and only VC6's preference order differs.  The shift order is the one
 * construct in this function that changes the allocator's input, which is why
 * it is the lever recorded above.
 * ADD-DESTINATION TIE-BREAK: not used here.  A controlled probe on joust.c's
 * TempleSlide_Update (same isometric pair, read order held constant) shows the
 * destination is decided by EMISSION ORDER, not by definition or read order --
 * VC6 copies the difference's left operand into a fresh register and whichever
 * of {sum, difference} is emitted second takes the remaining register.  That is
 * consistent with this function: the original emits the sum first (`lea`), so
 * its destination is a third register entirely.
 * Also inert this round: `wx` read before `wy`; the products written as single
 * statements with the shift folded in (identical to `sy >>= 9` first); the
 * diff defined before the sum with either shift order (identical to the
 * matching shift order); `off.x + g_map->origin_x + sx` operand order; and
 * swapping the two `c->s?` stores. */
/* ROUND OF 2026-09-04 (second pass).  Strict is unchanged at 319 -- with the
 * body displaced by three instructions from index 19 that number is nearly
 * meaningless here -- but every structural metric moved and TWO RECONSTRUCTION
 * ERRORS were found and fixed:
 *     metric                        was    now
 *     real (permutation-aware)      317    317
 *     register+offset-blind LCS     324    326   (of 351)
 *     orig indices in a bad region   46     42
 *     bytes                        1159   1153   (orig 1147)
 *
 * *** RECONSTRUCTION ERROR 1: `c->sx` was not a flat sum.  The original is
 *      80 xor ecx,ecx / 81 mov cx,[edx+0x20] / 82 add ecx,eax / 84 add ecx,edi
 * -- a FLAT three-term left-assoc chain in source order (origin_x, off.x, sx),
 * identical in shape to `c->sy` two instructions later, which our flat source
 * already reproduced.  The `Pos t` partial-sum barrier the previous round
 * imported from anim2.c emitted the other tree (`lea ecx,[edx+edi] /
 * add ecx,eax`) and was exactly where the global eax/ecx/edx phase first
 * parted company: with the barrier the original's `mov edx,[0x4bcbf4]` at 77
 * came out as `mov ecx,...`, and every scratch from there to the end of the
 * function was one step round the three-cycle.  Plain
 *      c->sx = g_map->origin_x + off.x + sx;
 *      c->sy = g_map->origin_y + off.y + sy;
 * puts indices 74-81 back in the original's registers and takes six bytes off.
 * (Aggregating the y sum instead, or both, is 323; y-before-x is robl 321.)
 * NOTE for the sibling anim2.c function this lever came from: DrawBoats' sums
 * are FOUR-term and VC6 does reassociate those -- all 24 source orders of a
 * four-term flat sum are byte-identical there -- so the barrier is still
 * earning its keep in DrawBoats and only this three-term site was wrong.
 *
 * *** RECONSTRUCTION ERROR 2: the failed-move restore is a WHOLE-STRUCT
 * assignment.  The original ends the `SchoolCarMayEnterSquare` failure path
 *      223 mov eax,[saved.x]  224 mov edx,[swx]   225 mov ecx,[saved.y]
 *      226 mov [ebx],eax      227 mov eax,[swy]   228 mov [esi+0x10],edx
 *      229 mov [esi+0x14],eax 230 pop edi  231 pop esi
 *      232 mov [ebx+4],ecx    233 pop ebp  234 pop ebx
 * -- BOTH halves of `c->cur` through the one `&c->cur` (ebx) the call argument
 * already materialised, the second of them after two of the pops.  That is an
 * 8-byte struct assignment, not two field stores: written field-by-field VC6
 * sends `.y` through `esi` and emits it before the pops.  `c->cur = saved;`
 * placed FIRST in the block reproduces the sequence instruction for
 * instruction (robl 324 -> 326, bad 45 -> 42); placed last it is robl 324, in
 * the middle 322.  An explicit `CarPos* cur = &c->cur;` scores the same but is
 * a less likely source line.  The SAVE side is genuinely field-by-field -- the
 * original's two loads there use DIFFERENT bases ([esi+0x18] at 26 and
 * [ebx+4] at 32) and sit six instructions apart, which a copy would not do;
 * `saved = c->cur;` scores robl 329 / bad 39 but emits the two loads adjacent
 * and through one base, so it is measured-better and NOT SHIPPED.
 *
 * *** THE NEXT LEVER, LOCATED PRECISELY: THE "DID SOMETHING" FLAG WANTS TO BE
 * ITS OWN LOCAL.  Declaring a separate `int flag` instead of reusing `th`
 * reproduces the original's tail EXACTLY -- `mov edi,1` at 265 and the
 * `mov edi,[esp+0x10]` reload at 294, and the whole body comes out at
 * 1147/1147 BYTES, the original's length to the byte, with bad regions 42 ->
 * 41.  It is rejected only because it takes a FIFTEENTH frame slot
 * (`sub esp,0x3c` where the original has 0x38), which moves every home and
 * makes the first divergence 0.  The original's flag SHARES frame E-0x38 with
 * the first `GetTileDimensions`' height out-param (it stores 0 there at index
 * 45 and reloads it at 294), so the two are lifetime-coloured together -- and
 * that cannot happen while `&th` is taken, because an address-taken local is
 * live function-wide.  Reusing `th` as we do gets the HOME right and the
 * register wrong; a separate local gets the register right and the home wrong.
 * Tried and did NOT merge the two slots: `th` alone in a block scope around
 * the projection, `flag` alone in a block scope running to the end, and both.
 * A separate `int depth` for the render key (the other `tw` reuse) is 326 and
 * also grows the frame.  Fourteen frame slots are otherwise all accounted for
 * (th, tw2, th2, swx, swy, off[2], saved[2], tx, ty, ctx[3]) with `tw` in the
 * dead argument slot, so the fifteenth has to come from a sharing, not from a
 * spare.  THIS IS THE HIGHEST-VALUE THING LEFT IN THIS FUNCTION.
 *
 * RE-RUN ON THE NEW BASELINE: all 60 legal orders of the five statements
 * between the second GetTileDimensions and AdjustOffsetForViewMode (the
 * shipped order is still the best at robl 326 / bad 42; the three orders that
 * split the two `off` stores are 313 strict but robl 322 / bad 52 -- the same
 * compensating error the last round recorded); the whole `>>= 9` shift-order
 * family (`sy >>= 9` first is now robl 328 / bad 39, better than this build
 * structurally, but 326 strict with the callee-saved rotation at index 8 and
 * six bytes longer, so still not shipped); `wx` read first; the products with
 * the shifts folded in (identical to the matching shift order); the tile
 * dimensions read before the world reads (328).
 *
 * WHAT IS LEFT (42 original indices, first divergence 19):
 *   - 19-40, the projection emission order.  Unchanged and fully understood:
 *     our register ASSIGNMENT is already the original's (wx=ebx, wy=ebp,
 *     sx=edi, sy=ebp); only the emission order differs, and the one construct
 *     that flips it (the order of the two `>>= 9`) also rotates the whole
 *     callee-saved assignment at index 8.
 *   - 53-63, the order of the three `-=` adjustments inside the scroll block.
 *   - 83/93/94, `tw = th2 + sy` -- the original emits it AFTER the `c->sy`
 *     store into a register that store has just freed (`lea eax,[edx+ebp]`);
 *     ours emits it before, in place on th2's register.  Moving the source
 *     line above `c->sy` is robl 319; moving it after the switch is 322.
 *   - 265/293/294/340, the flag, above. */
/* ROUND OF 2026-09-04 (third pass).  The flag's frame-slot question is ANSWERED
 * mechanically, and the answer is a construct, not a spelling -- but the
 * construct costs more elsewhere than it wins, so nothing is shipped and the
 * build is unchanged at 319 / real 317 / robl 326 / bad 42 / 1153B.
 *
 * *** THE CONSTRUCT THAT PUTS THE FLAG IN EDI WITH A 0x38 FRAME: making the
 * FIRST GetTileDimensions' two out-params locals of a `static __inline`
 * helper.  As inline-expansion temporaries they come out of the
 * lifetime-coloured spill pool (docs/DECOMP.md's "inline-expansion
 * temporaries share the spill-home pool"), which a NAMED address-taken local
 * never does, and that is exactly what the note above asked for:
 *      static __inline int GR_TileProject(int sum, int diff, int* pdiff)
 *      { int w, h; GetTileDimensions(&w, &h); *pdiff = diff * w;
 *        return sum * h; }
 *      sy = GR_TileProject(wx + wy, wx - wy, &sx);
 * plus separate `int flag` and `int depth` locals gives `sub esp,0x38`,
 * `mov edi,1` at the flag's set and a `mov edi,[home]` reload on the
 * bb != 0 path -- the original's tail shape -- WITHOUT the fifteenth slot.
 * So "an address-taken local cannot share" was right and the way round it is
 * to stop the local being named.
 * WHY IT IS NOT SHIPPED (322 / real 322 / robl 321 / bad 52 / 1150B):
 *   (a) the pool lands THREE SLOTS HIGH.  The helper's `h` gets E+0x10 and
 *       swx/swy fall to E+0x00/E+0x04, where the original has h at E+0x00 and
 *       swx/swy at E+0x0c/E+0x10; `w` does take the dead argument slot
 *       E+0x3c correctly.  The flag then gets a home of its own at E+0x18
 *       rather than sharing h's, so the sharing that motivated the change
 *       does not actually happen -- the frame only stays 0x38 because `depth`
 *       stops needing a slot.
 *   (b) inlining reorders the projection: the `lea eax,[ebx+ebp]` sum-first
 *       shape is lost and the diff is emitted first, which is the opposite of
 *       what index 19 wants.  Eight helper spellings (out-param, two
 *       out-params, an explicit result temp, shifts inside the helper, both
 *       shift orders, w-first and h-first declarations) are ONE object, so
 *       the reordering is not reachable from the helper's source.
 * Variants measured around it: only `h` as a temp with `tw` still named
 * (334, frame 0x3c -- so BOTH out-params have to leave); only `w` as a temp
 * (335); both tile calls via helpers (327); `th2` reused for the depth key
 * (330 but robl 327 / bad 41 / 1143B -- the best structural numbers on this
 * function and FOUR BYTES SHORT, recorded because it is the only build that
 * has ever come in under length); plain `int flag` with and without a
 * separate `int depth` (333, frame 0x3c, 1147B -- the note above's build).
 *
 * *** THE joust.c LEVER DOES NOT TRANSFER HERE.  TempleSlide_Update's
 * projection was fixed this round by moving the two world reads ABOVE the
 * preceding call, which lengthened a pointer's live range and stopped the
 * callee-saved rotation.  The analogue was swept here -- the `>>= 9` shift
 * order crossed with all four positions of the three `ctx` stores, the world
 * read order and the product order (10 builds) -- and nothing decouples the
 * rotation at index 8 from the sum-first emission at 19: `sy >>= 9` first is
 * still robl 328 / bad 39 (structurally the best) with strict 326 and twelve
 * bytes over.  The two functions' projections are the SAME shape read from
 * opposite ends, and this one has no statement in front of the reads that can
 * be moved. */
/* ROUND OF 2026-09-05 (fourth pass).  UNCHANGED AT 319 / real 317 / robl 326 /
 * bad 42 / 1153B, and ONE RECONSTRUCTION ERROR FOUND -- in the note above,
 * not in the code.
 *
 * *** RECONSTRUCTION ERROR: THE FRAME LAYOUT IS NOT THE ORIGINAL'S.  The
 * headline note at the top of this function ("the frame layout is the
 * original's exactly ... tw2/th2 at -0x34 and -0x30, swx/swy at -0x2c/-0x28,
 * off at -0x24/-0x20, saved at -0x1c/-0x18, tx/ty at -0x14/-0x10") is FALSE
 * for this build, and every later round's slot reasoning -- including
 * "fourteen frame slots are otherwise all accounted for" -- rests on it.
 * Measured by tracking push depth through both bodies (scratchpad/w10joust/
 * fm.py, fm2.py), with E = esp after `sub esp,0x38`:
 *      original   th/flag 0x00 | tw2 0x04 | th2 0x08 | swx 0x0c | swy 0x10 |
 *                 off 0x14/0x18 | saved 0x1c/0x20 | tx 0x24 | ty 0x28 |
 *                 ctx 0x2c/0x30/0x34 | tw/depth 0x3c (dead arg slot)
 *      ours       th/flag 0x00 | tx 0x04 | swx 0x08 | swy 0x0c | ty 0x10 |
 *                 tw2 0x14 | th2 0x18 | off 0x1c/0x20 | saved 0x24/0x28 |
 *                 ctx 0x2c/0x30/0x34 | tw/depth 0x3c
 * Evidence, both with six pushes live: the original's `mov [esp+0x24],eax`
 * at 0x004027e2 is swx at E+0x0c, ours is `mov [esp+0x20],eax` at E+0x08;
 * the original's `lea eax,[esp+0x24]` at 0x00402822 is &tw2 at E+0x04, ours
 * is `lea ecx,[esp+0x34]` at E+0x14.  Only th, the three ctx words and tw's
 * dead-argument home are right.  So the frame SIZE matches and nothing else
 * in the middle does, and half of the strict count is these displaced
 * offsets rather than displaced instructions.
 * *** IT IS NOT A DECLARATION LEVER.  Reversing the entire local block, moving
 * tx/ty to the front, and putting ctx first are ALL BYTE-IDENTICAL (LoadBaseMap's
 * "declaration order is irrelevant" holds here).  The shape of ours is
 * [th][the four scalars VC6 spills: tx, swx, swy, ty][the address-taken and
 * aggregate locals in declaration order: tw2, th2, off, saved, ctx]; the
 * original's is one ascending declaration-order run with the SAME four
 * scalars sitting between the aggregates.  So in the original those four are
 * plain memory locals and in ours they are spill homes, which is downstream
 * of the index-19 allocation -- the frame is a SYMPTOM of the projection
 * emission order, not an independent lever.  That also retires the
 * flag/depth line of attack in the round above: the fifteenth slot was never
 * the problem.
 * *** ALSO MEASURED AND NOT A HANDLE: swx/swy and/or tx/ty as `CarPos`
 * aggregates are byte-identical (VC6 flattens an aggregate whose address is
 * never taken); the unused `CarSave` 4-int carrier for saved+tx+ty is
 * 322/first divergence 13 in all three spellings; block scopes around BOTH
 * GetTileDimensions pairs with separate `flag`/`depth` locals give
 * 322/robl 321/1150B with `&th` moved to E+0x10 (first divergence 5), and
 * the second pair alone is 321.
 * *** AND A STANDING-TEST FLAG ON THE COMMITTED DEAD STORE.  Removing
 * `*(volatile int*)&tx = sx;` gives 322 strict but robl 329 / bad 37 / 1155B
 * -- i.e. the dead store buys three strict indices while making BOTH
 * register-blind metrics worse, which is the compensating-error signature.
 * The store itself is real (0x004027f3), so what the signature is saying is
 * that its POSITION relative to the swx/swy/saved run is wrong, or that it
 * only lands because the frame is displaced.  Left in place -- it is an
 * instruction of the original -- but it should be re-tested the moment index
 * 19 moves. */
/* SCOPE G RECHECK (2026-09-05).  No body change.  Three definition-site
 * volatile-read probes, on c->wx, c->wy and both, respectively give audit
 * 329/329/330 mismatches (baseline 319), all first diverging at index 5;
 * register+immediate-blind LCS is 320/321/316 (baseline 326).  They do not
 * decouple the projection's emission order from its allocation.  Independent
 * push-depth checks of the straight-line head confirm the fourth-pass frame
 * map above: swx/swy occupy E+0x08/+0x0c instead of +0x0c/+0x10, tx/ty
 * +0x04/+0x10 instead of +0x24/+0x28, and tw2/th2 +0x14/+0x18 instead of
 * +0x04/+0x08.  Thus equal frame size still does not imply equal homes.
 * At its measured floor for these probes; not a proof of global exhaustion. */
/* ROUND OF 2026-09-06 (second pass).  93 -> 83, still 351i/1147B to the byte,
 * and the structural metrics are the best this function has ever shown:
 * register-blind mismatch 30 -> 20, register+offset-blind LCS 340 -> 342 of
 * 351, first divergence still 19.  ONE source change: THE ORDER OF THE SIX
 * SNAPSHOT STATEMENTS BETWEEN THE PROJECTION BLOCK AND GetRoadRecord.
 *
 * *** THE STALE ASSUMPTION THAT BLOCKED FOUR ROUNDS: "THE FRAME IS NOT THE
 * ORIGINAL'S".  The fourth-pass note above (and every round that built on it,
 * including "the frame is a SYMPTOM, not a lever" and "the fifteenth slot was
 * never the problem") measured a build that no longer exists.  Re-derived by
 * push depth on THIS build -- the six pushes are ebx, ebp, esi, edi and the
 * two GetTileDimensions arguments, and VC6 never pops them, it just extends
 * the frame and settles up with `add esp,0x1c` at index 79 -- every home is
 * now the original's exactly:
 *      h/flag 0x00 | tw2 0x04 | th2 0x08 | swx 0x0c | swy 0x10 |
 *      off 0x14/0x18 | saved 0x1c/0x20 | tx 0x24 | ty 0x28 |
 *      ctx 0x2c/0x30/0x34 | w/tw 0x3c (the dead parameter slot)
 * Indices 31, 47, 48 and 52 -- swy's store and both `lea`s at the second
 * GetTileDimensions -- match the original to the displacement byte, which they
 * could not do if a single home were off.  `flag` DOES share `h`'s E+0x00 with
 * `&h` taken, because `w`/`h` are BLOCK locals: the block scope, not the name,
 * bounds the lifetime.  So the fifteenth-slot problem is closed and the whole
 * residual is emission ORDER.
 *
 * *** THE LEVER.  All 720 orders of
 *     frame.swx / frame.swy / frame.saved / frame.target.x / frame.target.y /
 *     flag = 0
 * were measured.  They span 83..126 and the best is
 *     frame.target.y = c->ty;  frame.swy = c->wy;  frame.saved = c->cur;
 *     frame.target.x = c->tx;  frame.swx = c->wx;  flag = 0;
 * -- the two halves of EACH pair split around the saved-square copy, y before
 * x in both.  That is worth ten indices and it puts 31..40 (swy, both cur
 * loads through the two bases, the dead tx store, saved.y, tx, saved.x) in the
 * original's order INSTRUCTION FOR INSTRUCTION.  The naive orders are all
 * worse: the shipped-before order XYSPQF is 93, the "obvious" XYSQPF (which
 * reads like the original's emission) 94, and pulling any of the six into the
 * projection block or in front of it costs 9-46.  The head order
 * (ctx.kind, ctx.owner, ctx.f08, wy, wx) was swept too -- all 120 orders --
 * and the shipped one is the unique optimum at 83; the next is 85.
 *
 * *** WHAT IS LEFT, RE-DERIVED, AND WHY IT IS ONE FACT.  12 indices in 19..44,
 * 9 at 83..94 and 53 in 103..199, and the last two are DOWNSTREAM of the
 * first.  The original's projection is
 *      lea eax,[ebx+ebp] / mov edi,ebx / imul eax,[h] / sub edi,ebp /
 *      imul edi,[w] / sar eax,9 / mov ebp,eax ... sar edi,9 / mov [tx],edi
 * -- the SUM emitted first, so both wx and wy are still live and it needs a
 * scratch (`lea`) plus a copy (`mov ebp,eax`); ours emits the diff first,
 * which frees wy, so the sum folds in place (`add ebp,ebx`) and we spend the
 * saved instruction on a SECOND dead store instead.  Instruction and byte
 * counts balance exactly: original = 8 projection ops + 1 dead store, ours =
 * 7 + 2.
 * THE COUNT OF DEAD STORES IS THE WHOLE PROBLEM, AND IT IS A DSE FACT.
 * The original stores the projected X into tx's home at 35 and never stores
 * the projected Y.  Measured on this build: an 8-byte `frame.target = project`
 * copy is NEVER dead-store-eliminated (both halves survive -> our two dead
 * stores), and field-by-field `frame.target.x = ...; frame.target.y = ...;`
 * with read-back is ALWAYS eliminated (BOTH halves vanish -> 114, first 8,
 * because sx then loses the reference that ranks it into edi).  There is no
 * source construct that eliminates exactly one, so the residual dead Y store
 * and the diff-first schedule are the same fact.
 * AND THE ESCAPE IS BLOCKED BY REGISTER PRESSURE, NOT BY AN ANCHOR.  Every
 * spelling that leaves sy without a memory home lets VC6 SINK the whole y
 * product past GetRoadRecord and the second GetTileDimensions (321-322,
 * first 5): it keeps wx/wy in ebx/ebp across the calls and never materialises
 * `&c->cur` at all, which is the only reason the four callee-saved registers
 * stretch.  Note that this CANNOT be an anchoring effect -- all fourteen frame
 * slots plus the parameter slot are accounted for above, so the ORIGINAL's sy
 * has no home either and still does not sink.  What pins it there is that
 * `&c->cur` (six references) outranks wx and wy (two each) and takes ebx.
 * Tried and did not move it: an explicit `CarPos* cur = &c->cur;` used in the
 * call and restore only, in the snapshot, in GetRoadRecord's arguments and in
 * all of them (321/333/175/100); the copy's y member carrying `c->ty` so only
 * X's store is dead (313-314, sinks anyway); x-anchored with the products,
 * shifts and temps in every order.
 * *** MEASURED BETTER ON STRICT AND NOT SHIPPED, both the compensating-error
 * signature the rounds above name: reusing `th2` as the depth key is 70 strict
 * but register-blind 20 -> 34 and 1144B (three bytes SHORT, first divergence
 * 5); `p3->sy` stored before `p3->sx` is 75 strict, register-blind 34,
 * 1146B.  Both trade a byte-exact body for index luck.  Re-test them the
 * moment the emission at 19 moves.
 * *** ALSO INERT ON THIS BASE: the declaration order of w/h (all four
 * spellings), of sx/sy and of wx/wy; the two `>>= 9` shifts in either order
 * (the old shift-order lever is GONE now that the copy pins both products);
 * every temp spelling of the sum and difference; reading sx/sy back from
 * `project` or from `frame.target` in either order (83 or 94); the copy placed
 * before, between or after the two read-backs; six positions and spellings of
 * `tw = th2 + sy` (83, or 274 once it crosses the switch); the bloke store
 * order and `dir` first (83). */
/* FGH-100b (2026-09-07).  83 -> 72, still 351i/1147B, first divergence 19,
 * behaviour verified identical to the original on 1,200 randomized cases
 * (scratchpad/fgh100b/diffexec.py).  Found by a MECHANICAL mutation search
 * over ~5,300 semantics-preserving textual variants of this body
 * (scratchpad/fgh100b/mutate.py, mutants/StepSchoolCar/m2 + m3), not by hand:
 *  - the three Person3D stores are written sy, depth, sx (the original emits
 *    depth, sx, sy; VC6 reorders the adjacent stores itself, and this order
 *    is what fixes the scratch rotation of the +0x1c/+0x20 sums);
 *  - a free `if (sx) { }` consumer after `sx -= (tw2 + 1) >> 1` -- measured
 *    load-bearing (74 without it), zero instructions.
 * Every cosmetic artefact of the search (`!(!(x))`, named index temporaries)
 * was measured inert and removed.  Indices 19-44 (the projection's dead
 * store) are unchanged; the search's ~1,900 distinct objects for this body
 * never moved index 19, so that class needs a construct, not a spelling. */
// WIP-FUNCTION: LEGOLAND 0x00402780  (351i/1147B vs 351i/1147B -- the original's length to the byte, and every frame home is the original's; 72 mismatches, first 19; classes: the projection's sum/diff emission order and its second dead store at 19-44, the `tw = th2 + sy` placement at 83-94, and the eax/edx/ecx scratch rotation from 103 on, now 44 indices instead of 53)
void StepSchoolCar(SchoolCar* c)
{
    struct {
        int swx;
        int swy;
        Pos8 off;
        CarPos saved;
        CarPos target;
        BlitCtx ctx;
    } frame;

    /* The projection block's dimension locals end before the movement flag
     * starts. Their homes can share the later flag/depth storage. */
    int       tw;
    int       flag;
    int       tw2;
    int       th2;
    Person3D* p3;
    int       wx;
    int       wy;
    int       sx;
    int       sy;

    frame.ctx.kind = 0x306;
    frame.ctx.owner = c->bloke;
    frame.ctx.f08 = 0;
    wy = c->wy >> 8;
    wx = c->wx >> 8;
    {
        CarPos project;
        int w, h;

        GetTileDimensions(&w, &h);
        project.x = (wx - wy) * w >> 9;
        project.y = (wx + wy) * h >> 9;
        frame.target = project;
        sx = project.x;
        sy = project.y;
    }
    /* Both pairs are SPLIT around the saved-square copy, y before x in each:
     * that is the unique best of all 720 orders of these six statements and it
     * reproduces indices 31-40 -- swy, the two `c->cur` loads through their two
     * different bases, the dead tx store, saved.y, tx, saved.x -- instruction
     * for instruction.  Writing them as two tidy adjacent pairs costs ten. */
    frame.target.y = c->ty;
    frame.swy = c->wy;
    frame.saved = c->cur;
    frame.target.x = c->tx;
    frame.swx = c->wx;
    flag = 0;
    GetRoadRecord(c->cur.x, c->cur.y);
    GetTileDimensions(&tw2, &th2);
    sx -= (tw2 + 1) >> 1;
    if (sx) { }
    sx -= g_scroll_x >> 8;
    sy -= g_scroll_y >> 8;
    frame.off.x = g_car_images->dx[c->b8] >> 1;
    frame.off.y = g_car_images->dy[c->b8] >> 1;
    AdjustOffsetForViewMode((Offset*)&frame.off);
    /* BOTH sums are flat three-term left-assoc chains in the original
     * (`xor ecx,ecx / mov cx,[edx+0x20] / add ecx,eax / add ecx,edi`), and a
     * flat source reproduces both in source order.  The `Pos t` aggregate this
     * used to carry emitted the other tree (`lea ecx,[edx+edi] / add ecx,eax`)
     * and was where the global eax/ecx/edx phase first parted company. */
    c->sx = g_map->origin_x + frame.off.x + sx;
    c->sy = g_map->origin_y + frame.off.y + sy;
    tw = th2 + sy;

    switch (c->dir) {
    case 1:
        SetOverridePalette(g_car_pal_c);
        break;
    case 2:
        SetOverridePalette(g_car_pal_b);
        break;
    case 3:
        SetOverridePalette(g_car_pal_a);
        break;
    }

    c->b9 = c->b8;
    SetOverrideFrame(c->b8);
    SortSpriteWithCallback(g_car_sprite, c->sx, c->sy, tw, 0,
                           SchoolCar_DrawDriver, (int)c, &frame.ctx);
    ClearOverridePalette();
    ClearOverrideFrame();

    c->bloke->world.x = sx;
    c->bloke->world.y = sy;
    c->bloke->dir = (unsigned char)((c->b8 + 6) & 0xf);
    p3 = Find3DPersonFromBloke(c->bloke);
    p3->sy = g_map->origin_y + c->bloke->world.y + 8;
    p3->depth = tw;
    p3->sx = g_map->origin_x + c->bloke->world.x + 0x10;
    AdjustBlokePosition((Pos8*)&p3->sx);
    SetPerson3DHeading(p3, c->bloke->dir);

    if (SchoolCarBlockedAhead(c)) {
        c->speed = (unsigned short)(c->top_speed >> 1);
        c->stall++;
        if ((short)c->stall <= 0x200)
            return;
        if (c->manoeuvre == 0)
            goto joint;
        return;
    } else {
        c->stall = 0;
        if (c->speed < c->top_speed)
            c->speed = (unsigned short)(c->speed + 0x40);
        SchoolCarAccelerate(c);
        c->wx += c->vx;
        c->wy += c->vy;
        c->cur.x = (c->wx + 0x10000) >> 16;
        c->cur.y = (c->wy + 0x10000) >> 16;
        if (SchoolCarMayEnterSquare(&c->cur, &frame.saved) == 0) {
            /* A WHOLE-STRUCT restore, not two field stores: the original
             * emits both halves through the one `&c->cur` the call argument
             * already materialised (`mov [ebx],eax` ... `mov [ebx+4],ecx`,
             * the second after two of the pops), which is what an 8-byte
             * struct assignment lowers to.  Written field-by-field VC6 sends
             * `.y` through `esi` instead.  The SAVE side above is genuinely
             * field-by-field -- there the original's two loads use different
             * bases and sit eight instructions apart. */
            c->cur = frame.saved;
            c->wx = frame.swx;
            c->wy = frame.swy;
            return;
        }
    }

joint:
    if ((((frame.target.x - c->wx) ^ (frame.target.x - frame.swx)) | ((frame.target.y - c->wy) ^ (frame.target.y - frame.swy))) & 0x80000000)
        goto step;
    if (c->bb) {
        if (frame.target.x != c->wx)
            return;
        if (frame.target.y != c->wy)
            return;
    }

step:
    if (c->bb == 0) {
        flag = 1;
        if (c->manoeuvre == 0) {
            c->on_road = 0;
            if (c->t_horn != 0)
                c->manoeuvre = (unsigned char)
                    SchoolCarNextManoeuvreHorn(c->school, &c->start, c->ba);
            else
                c->manoeuvre = (unsigned char)
                    SchoolCarNextManoeuvre(c->school, &c->start, c->ba);
            if (c->manoeuvre == 0)
                c->speed = 0;
        }
    }

    switch (c->manoeuvre) {
    case 1:
        if (SchoolCarAtTarget(c)) {
            SchoolCarManoeuvreA(c);
            c->manoeuvre = 0;
        }
        break;
    case 2:
        if (SchoolCarAtTarget(c)) {
            c->manoeuvre = 0;
            SchoolCarManoeuvreB(c);
        }
        break;
    case 3:
        if (SchoolCarAtTarget(c)) {
            c->manoeuvre = 0;
            SchoolCarManoeuvreC(c);
        }
        break;
    case 4:
        SchoolCarManoeuvreD(c);
        c->manoeuvre = 0;
        break;
    case 5:
        SchoolCarManoeuvreA(c);
        SchoolCarManoeuvreB(c);
        c->manoeuvre = 0;
        break;
    }

    if (!flag)
        SchoolCarIdleStep(c);
}

/* ==========================================================================
 * 0x004304e0 -- RESTAURANT 2's animated-layer pass.
 *
 * ridecb2.c and ridecb4.c both call this "Restaurant2_AnimTick" and reach it
 * from Restaurant2_Draw (0x00430b10); the name is kept for continuity, but
 * what it actually does is APPLY THE PER-INSTANCE ANIMATION FRAMES AND PRINT
 * THE ANIMATED LAYERS. For the restaurant on one map square it:
 *
 *   1. builds the 0x103 render-item context for the class element and that
 *      square (the tag the render walk's re-entry test looks for),
 *   2. finds the square's record (0x0042f9d0) -- nothing is drawn without
 *      one -- and takes the FOUR signed frame bytes it carries:
 *          +0x06  the "small" animation (layers 0 and 2)
 *          +0x07  the main door/sign animation (layer 1)
 *          +0x08  the top-floor animation (layer 6)
 *          +0x09  the second-floor animation (layer 3)
 *   3. switches on the record's FACING (+0x18, 0..5) and, for each layer that
 *      facing shows, does the same four steps: push the frame into that
 *      layer's LLS, take the layer's render offset, run it through the view
 *      transform, and print the layer's sprite at the object's screen
 *      position with the 0x103 context.
 *
 * The per-facing layer sets:
 *      facing 0   layers 1, 3   + layer 6's frame ONLY IF rec->+0x0c is set
 *      facing 1   layers 1, 3   + layer 6's frame unconditionally
 *      facing 2   layer 1
 *      facing 3   layers 1, 3
 *      facing 4   layers 1, 2, 0
 *      facing 5   layers 1, 2, 0   (an identical second copy in the original)
 *
 * Layer 6 is never PRINTED here -- only its frame is set -- so the top floor
 * is drawn later by Restaurant2_Draw's own facing sequence.
 *
 * Two of the record's bytes are homed in the DEAD `sq` and `item` argument
 * slots (the classic VC6 reuse); the frame proper is just the 8-byte layer
 * offset plus the 12-byte context block.
 * ========================================================================== */

/* RESTAURANT 2's per-square record, as this pass reads it. */
typedef struct Rest2Rec {
    unsigned char pad00[6];
    char          f06;           /* +0x06  layers 0 and 2 */
    char          f07;           /* +0x07  layer 1 */
    char          f08;           /* +0x08  layer 6 */
    char          f09;           /* +0x09  layer 3 */
    unsigned char pad0a[2];
    int           f0c;           /* +0x0c  facing-0 gate for layer 6 */
    unsigned char pad10[0x18 - 0x10];
    int           facing;        /* +0x18  0..5 */
} Rest2Rec;

/* The render-item context block PrintSprite takes as its 5th argument. */
typedef struct DrawCtx {
    int            tag;          /* +0x00  0x103 */
    RideElem*      elem;         /* +0x04 */
    unsigned short square;       /* +0x08 */
} DrawCtx;

extern Rest2Rec* Restaurant2_FindRec(MapSquare* sq);                 /* 0x0042f9d0 */
extern void*     GetLLSForLayer(RenderObj* obj, int layer);          /* 0x00441ea0 */

extern RenderObj* g_rest2_layers;    /* 0x00616118 */

// FUNCTION: LEGOLAND 0x004304e0
void Restaurant2_AnimTick(MapSquare* sq, RideObject* item, int mode)
{
    Offset    off;
    DrawCtx   ctx;
    Rest2Rec* rec;
    Offset    screen;
    char      a;
    char      b;
    char      c;
    char      d;

    ctx.tag = 0x103;
    ctx.elem = item->elem;
    ctx.square = *(unsigned short*)sq;
    rec = Restaurant2_FindRec(sq);
    if (rec) {
        screen = GetScreenCoordsForObject(sq, item);
        a = rec->f06;
        b = rec->f07;
        c = rec->f08;
        d = rec->f09;
        switch (rec->facing) {
        case 0:
            LLSSetFrame(GetLLSForLayer(g_rest2_layers, 1), b);
            off = GetRenderOffsetForLayer(g_rest2_layers, 1);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_rest2_layers, 1),
                        screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
            LLSSetFrame(GetLLSForLayer(g_rest2_layers, 3), d);
            off = GetRenderOffsetForLayer(g_rest2_layers, 3);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_rest2_layers, 3),
                        screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
            if (rec->f0c)
                LLSSetFrame(GetLLSForLayer(g_rest2_layers, 6), c);
            break;

        case 1:
            LLSSetFrame(GetLLSForLayer(g_rest2_layers, 1), b);
            off = GetRenderOffsetForLayer(g_rest2_layers, 1);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_rest2_layers, 1),
                        screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
            LLSSetFrame(GetLLSForLayer(g_rest2_layers, 3), d);
            off = GetRenderOffsetForLayer(g_rest2_layers, 3);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_rest2_layers, 3),
                        screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
            LLSSetFrame(GetLLSForLayer(g_rest2_layers, 6), c);
            break;

        case 2:
            LLSSetFrame(GetLLSForLayer(g_rest2_layers, 1), b);
            off = GetRenderOffsetForLayer(g_rest2_layers, 1);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_rest2_layers, 1),
                        screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
            break;

        case 4:
            LLSSetFrame(GetLLSForLayer(g_rest2_layers, 1), b);
            off = GetRenderOffsetForLayer(g_rest2_layers, 1);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_rest2_layers, 1),
                        screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
            LLSSetFrame(GetLLSForLayer(g_rest2_layers, 2), a);
            off = GetRenderOffsetForLayer(g_rest2_layers, 2);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_rest2_layers, 2),
                        screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
            LLSSetFrame(GetLLSForLayer(g_rest2_layers, 0), a);
            off = GetRenderOffsetForLayer(g_rest2_layers, 0);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_rest2_layers, 0),
                        screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
            break;

        case 5:
            LLSSetFrame(GetLLSForLayer(g_rest2_layers, 1), b);
            off = GetRenderOffsetForLayer(g_rest2_layers, 1);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_rest2_layers, 1),
                        screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
            LLSSetFrame(GetLLSForLayer(g_rest2_layers, 2), a);
            off = GetRenderOffsetForLayer(g_rest2_layers, 2);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_rest2_layers, 2),
                        screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
            LLSSetFrame(GetLLSForLayer(g_rest2_layers, 0), a);
            off = GetRenderOffsetForLayer(g_rest2_layers, 0);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_rest2_layers, 0),
                        screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
            break;

        case 3:
            LLSSetFrame(GetLLSForLayer(g_rest2_layers, 1), b);
            off = GetRenderOffsetForLayer(g_rest2_layers, 1);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_rest2_layers, 1),
                        screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
            LLSSetFrame(GetLLSForLayer(g_rest2_layers, 3), d);
            off = GetRenderOffsetForLayer(g_rest2_layers, 3);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_rest2_layers, 3),
                        screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
            break;

        }
    }
}
