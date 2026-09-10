/* LEGOLAND -- unnamed class callbacks from screen.c's SetCustomCallbacks table.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field offsets, global addresses and callee arg counts are load-bearing;
 * names are ours.
 *
 * ---------------------------------------------------------------------------
 * WHAT THESE ARE
 * ---------------------------------------------------------------------------
 * LEGOLAND has no "screens" in the windowing sense: the park itself is the
 * screen and every buildable thing is an ODF *class*.  screen.c's
 * SetCustomCallbacks (0x00452c20) runs once per class element as the object
 * database is loaded, matches the element's class NAME, and stores a run of
 * function pointers into the class's 0xd0-byte ObjDef.  The slots (the same
 * numbering docs/RIDE_CALLBACKS.md uses) are:
 *
 *     +0x8c tick/select   +0x90 update      +0x94 draw-selection
 *     +0x98 add           +0x9c remove      +0xa0 draw-descriptor
 *     +0xa4 create        +0xa8 activate    +0xac destroy
 *     +0xb0 interact      +0xb8 load        +0xbc save        +0xc0 extra
 *
 * The functions in this file are the ones screen.c still declares as
 * `CB_<address>` -- callbacks whose owning module was never split out.  Each
 * one is identified here by the class it belongs to and the slot it fills:
 *
 *   0x00431300  OCTOPUS CAFE          +0xa4  OctopusCafe_Create
 *   0x0042f770  RESTAURANT 2          +0xa4  Restaurant2_Create
 *   0x00419d10  BOATING SCHOOL        +0xa4  BoatingSchool_Create
 *   0x00434cb0  JUNGLE CRUISE         +0xa4  JungleCruise_Create
 *   0x0042ea60  CASTLE BBQ            +0xa8  CastleBbq_Tick
 *   0x0042ec10  FOODCART DRINK        +0xa8  FoodcartDrink_Tick
 *   0x0042ed70  FOODCART FOOD         +0xa8  FoodcartFood_Tick
 *   0x00431170  FOODCART ICECREAM     +0xa8  FoodcartIcecream_Tick
 *   0x0042e610  SHARK CAFE            +0xa8  SharkCafe_Tick
 *   0x00405940  DRIVING SCHOOL        +0x9c  DrivingSchool_Remove
 *   0x0041b6f0  BOATING SCHOOL MERMAID+0x9c  BsMermaid_Remove
 *   0x0041abd0  BOATING SCHOOL        +0xb0  BoatingSchool_Draw
 *   0x0042d400  EARTH SLIDE RIDE      +0xb8  EarthSlide_Load
 *   0x0041bfb0  BOATING SCHOOL WATER  +0x94  BsWater_DrawSelection
 *   0x00436470  JUNGLE CRUISE WATER   +0x94  JcWater_DrawSelection
 *
 * The two +0x94 bodies are the only ones still WIP; everything else is exact.
 * Cross-file note: OCTOPUS CAFE's sprite globals are the same objects
 * ridecb4.c declares as the arrays g_cafe_table[9] (0x0081cd60) and
 * g_cafe_chair_mask[16] (0x0081cda0); they are spelled as separate scalars
 * here because the create handler loads them one literal at a time and an
 * array subscript is a different object to VC6.
 *
 * ---------------------------------------------------------------------------
 * THE +0xa4 CREATE SHAPE (three of the seven, and ~40 more in the frontier)
 * ---------------------------------------------------------------------------
 * A create handler is called by the ODF loader immediately after the callback
 * table is installed, and every one in the game opens with the same four
 * steps (identical to mechrides.c's six and westtown.c's nine):
 *
 *     def = elem->data;                 cache the ObjDef in a module global
 *     def->flags |= 0x20 | 0x400;       0x20 arms +0xa8, 0x400 arms +0xa0
 *     spr = def->sprite;                the class's build sprite (ObjDef+0x64)
 *     spr->flags |= 0x2000;             arm the custom draw in +0xb0
 *
 * A class that does NOT override the draw descriptor gets only 0x20 (OCTOPUS
 * CAFE); one that does gets 0x420 (RESTAURANT 2, BOATING SCHOOL).  After that
 * the handler resolves its own .wav names (Load_FXList), loads its sprites,
 * and parks any animated layers of the build sprite -- HideLayer +
 * StopLayerPlaying + LLSSetFrame(GetLLSForLayer(...), 0) is the three-call
 * "park this layer" idiom the whole game shares.
 *
 * The four cafe/restaurant classes above share ONE original translation unit:
 * OCTOPUS CAFE's sprite globals run 0x0081cd60..0x0081cd80 and
 * 0x0081cda0..0x0081cddc, and RESTAURANT 2's four are 0x0081cd20, 0x0081cd34,
 * 0x0081cd48 and 0x0081cd84 -- interleaved into the gaps, which is how the
 * original's .data ordering identifies them as one module.
 *
 * ---------------------------------------------------------------------------
 * THE +0x94 DRAW-SELECTION SHAPE (the two "WATER" classes)
 * ---------------------------------------------------------------------------
 * BOATING SCHOOL WATER and JUNGLE CRUISE WATER are the cells of a lake/river
 * the player paints; the ride building itself is a different class.  Their
 * +0x94 slot is what runs while the mouse hovers a water square with the
 * ride selected, and both are the SAME function with different globals:
 *
 *   1. resolve the hovered map reference to its cell and replace it with the
 *      cell's OWNER square (Cell+0x04, the packed {x,y} the object hangs off);
 *   2. walk the station list looking for that square among each station's two
 *      dock squares.  A hit means the mouse is on the building's jetty: look
 *      the water record up, retarget the reference at the record's owning
 *      school/river, stamp the ride's saved footprint onto the selection
 *      ObjDef, and hand the whole thing to the RIDE's own +0x94 handler
 *      through a synthetic RideElem built on the stack whose only filled
 *      field is `data`;
 *   3. otherwise stamp the fixed 5x5 water footprint {-2,-2,2,2} instead, ask
 *      BasicObjectDCalcCursor for the cursor, and then reject the square with
 *      cursor error 1 if a boat is sitting on it (each boat is tested at BOTH
 *      of its two positions -- the boat's own square and its target).
 *
 * ORIGINAL BUG, reproduced: step 1's cell fetch is bounds-checked and returns
 * 0 off-map, and the result is dereferenced without a null test, so hovering
 * the border of the map reads {x,y} out of address 4.
 * ------------------------------------------------------------------------- */

/* ---- the ODF class record (same 0xd0-byte ObjDef as screen.c) ------------ */
typedef struct BPos  { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; unsigned char c[2]; } BPosW;
typedef struct Pos   { int x; int y; } Pos;
/* The class geometry rect: the footprint and its sub-rects are chained
 * through `next` (ridecb5.c documents the boating school's chain). */
typedef struct Rect {
    int          left;              /* +0x00 */
    int          top;               /* +0x04 */
    int          right;             /* +0x08 */
    int          bottom;            /* +0x0c */
    struct Rect* next;              /* +0x10 */
} Rect;                             /* 0x14 */

typedef struct Spr {
    unsigned char pad00[0x10];
    unsigned int  flags;            /* +0x10  0x2000 = draw via the +0xb0 slot */
} Spr;

typedef struct RideDef {
    unsigned char pad00[0x0c];
    int           base_x;           /* +0x0c the class's base map square */
    int           base_y;           /* +0x10 */
    unsigned char pad14[0x1c - 0x14];
    unsigned int  flags;            /* +0x1c  0x20 = tick via +0xa8,
                                     *        0x400 = ask +0xa0 for the desc */
    unsigned char pad20[0x26 - 0x20];
    short         cost;             /* +0x26 bricks charged / refunded */
    unsigned char pad28[0x3c - 0x28];
    Rect          footprint;        /* +0x3c the class footprint rect */
    unsigned char pad50[0x64 - 0x50];
    Spr*          sprite;           /* +0x64 build sprite */
    unsigned char pad68[0xc4 - 0x68];
    void*         elem;             /* +0xc4 the LLIDB element this came from */
    unsigned char padc8[0xcc - 0xc8];
    struct RiderNode* riders;       /* +0xcc the class-wide customer list */
} RideDef;

typedef struct RideElem {
    char*         name;             /* +0x00 */
    char*         image;            /* +0x04 */
    unsigned int  type_flags;       /* +0x08 */
    RideDef*      data;             /* +0x0c */
    int           refcount;         /* +0x10  (the LLIDB element is 20 bytes) */
} RideElem;

/* ---- shared resource loaders -------------------------------------------- */
extern Spr*  LoadSprite(const char* name, int mode);            /* 0x00497ab0 */
extern void  Load_FXList(void* list, int count);                /* 0x00496dd0 */
extern void  LoadMoneySFX(void);                                /* 0x00453900 */
extern void  HideLayer(Spr* sprite, int layer);                 /* 0x00497de0 */
extern void  StopLayerPlaying(Spr* sprite, int layer);          /* 0x00441f00 */
extern void* GetLLSForLayer(Spr* sprite, int layer);            /* 0x00441ea0 */
extern void  LLSSetFrame(void* lls, int frame);                 /* 0x0047d5a0 */


/* =========================================================================
 * 0x00431300 -- OCTOPUS CAFE +0xa4
 *
 * The largest pure resource loader in the table: eight tent sprites, the
 * kiosk, and sixteen table sprites (eight tables x two states, "A"/"B" --
 * empty and occupied).  The class arms only 0x20, so it uses the render
 * walk's built-in draw descriptor rather than overriding +0xa0.
 *
 * The tail call is real: LoadMoneySFX is reached by `jmp`, so this body has
 * no epilogue of its own.
 * ========================================================================= */
extern RideDef* g_oct_def;                                    /* 0x0081cd1c */
extern Spr*     g_oct_tent_a;                                 /* 0x0081cd60 */
extern Spr*     g_oct_tent_b;                                 /* 0x0081cd64 */
extern Spr*     g_oct_tent_c;                                 /* 0x0081cd68 */
extern Spr*     g_oct_tent_d;                                 /* 0x0081cd6c */
extern Spr*     g_oct_tent_e;                                 /* 0x0081cd70 */
extern Spr*     g_oct_tent_f;                                 /* 0x0081cd74 */
extern Spr*     g_oct_tent_g;                                 /* 0x0081cd78 */
extern Spr*     g_oct_tent_h;                                 /* 0x0081cd7c */
extern Spr*     g_oct_kiosk;                                  /* 0x0081cd80 */
extern Spr*     g_oct_tab_aa;                                 /* 0x0081cda0 */
extern Spr*     g_oct_tab_ab;                                 /* 0x0081cda4 */
extern Spr*     g_oct_tab_ba;                                 /* 0x0081cda8 */
extern Spr*     g_oct_tab_bb;                                 /* 0x0081cdac */
extern Spr*     g_oct_tab_ca;                                 /* 0x0081cdb0 */
extern Spr*     g_oct_tab_cb;                                 /* 0x0081cdb4 */
extern Spr*     g_oct_tab_da;                                 /* 0x0081cdb8 */
extern Spr*     g_oct_tab_db;                                 /* 0x0081cdbc */
extern Spr*     g_oct_tab_ea;                                 /* 0x0081cdc0 */
extern Spr*     g_oct_tab_eb;                                 /* 0x0081cdc4 */
extern Spr*     g_oct_tab_fa;                                 /* 0x0081cdc8 */
extern Spr*     g_oct_tab_fb;                                 /* 0x0081cdcc */
extern Spr*     g_oct_tab_ga;                                 /* 0x0081cdd0 */
extern Spr*     g_oct_tab_gb;                                 /* 0x0081cdd4 */
extern Spr*     g_oct_tab_ha;                                 /* 0x0081cdd8 */
extern Spr*     g_oct_tab_hb;                                 /* 0x0081cddc */

// FUNCTION: LEGOLAND 0x00431300
void OctopusCafe_Create(RideElem* elem)
{
    RideDef* def = elem->data;

    g_oct_def = def;
    def->flags |= 0x20;
    g_oct_def->sprite->flags |= 0x2000;
    g_oct_tent_a = LoadSprite("OctTentA.lls", 1);
    g_oct_tent_b = LoadSprite("OctTentB.lls", 1);
    g_oct_tent_c = LoadSprite("OctTentC.lls", 1);
    g_oct_tent_d = LoadSprite("OctTentD.lls", 1);
    g_oct_tent_e = LoadSprite("OctTentE.lls", 1);
    g_oct_tent_f = LoadSprite("OctTentF.lls", 1);
    g_oct_tent_g = LoadSprite("OctTentG.lls", 1);
    g_oct_tent_h = LoadSprite("OctTentH.lls", 1);
    g_oct_kiosk  = LoadSprite("OctoKiosk.lls", 1);
    g_oct_tab_aa = LoadSprite("OctTabAA.lls", 1);
    g_oct_tab_ab = LoadSprite("OctTabAB.lls", 1);
    g_oct_tab_ba = LoadSprite("OctTabBA.lls", 1);
    g_oct_tab_bb = LoadSprite("OctTabBB.lls", 1);
    g_oct_tab_ca = LoadSprite("OctTabCA.lls", 1);
    g_oct_tab_cb = LoadSprite("OctTabCB.lls", 1);
    g_oct_tab_da = LoadSprite("OctTabDA.lls", 1);
    g_oct_tab_db = LoadSprite("OctTabDB.lls", 1);
    g_oct_tab_ea = LoadSprite("OctTabEA.lls", 1);
    g_oct_tab_eb = LoadSprite("OctTabEB.lls", 1);
    g_oct_tab_fa = LoadSprite("OctTabFA.lls", 1);
    g_oct_tab_fb = LoadSprite("OctTabFB.lls", 1);
    g_oct_tab_ga = LoadSprite("OctTabGA.lls", 1);
    g_oct_tab_gb = LoadSprite("OctTabGB.lls", 1);
    g_oct_tab_ha = LoadSprite("OctTabHA.lls", 1);
    g_oct_tab_hb = LoadSprite("OctTabHB.lls", 1);
    LoadMoneySFX();
}


/* =========================================================================
 * 0x0042f770 -- RESTAURANT 2 +0xa4
 *
 * The tower restaurant: its own three-entry FX list (the lift's start / move
 * / stop samples), the money samples, four matte sprites, and then SIX of
 * the build sprite's layers parked.  The layers are the restaurant's moving
 * parts and the order they are parked in is the original's:
 *
 *     0  front door   6  ?         2  back door
 *     5  tower        1  lift      3  lift interior
 *
 * Layer 5 is hidden but NOT stopped or rewound -- the only layer of the six
 * that keeps its animation state, reproduced as written.
 * ========================================================================= */
typedef struct FXEntry {
    const char* name;               /* +0x00 .wav name */
    void*       sample;             /* +0x04 filled in by Load_FXList */
    int         flags;              /* +0x08 */
} FXEntry;

extern FXEntry  g_rest2_fx[3];                                /* 0x004b6968 */
extern RideDef* g_rest2_def;                                  /* 0x0081cd30 */
extern Spr*     g_rest2_layers;                               /* 0x00616118 */
extern Spr*     g_rest2_fdoor_m;                              /* 0x0081cd34 */
extern Spr*     g_rest2_fdoor_m1;                             /* 0x0081cd48 */
extern Spr*     g_rest2_bdoor_m;                              /* 0x0081cd84 */
extern Spr*     g_rest2_tower_m;                              /* 0x0081cd20 */

// FUNCTION: LEGOLAND 0x0042f770
void Restaurant2_Create(RideElem* elem)
{
    RideDef* def;

    Load_FXList(g_rest2_fx, 3);
    LoadMoneySFX();
    def = elem->data;
    g_rest2_def = def;
    def->flags |= 0x420;
    g_rest2_layers = g_rest2_def->sprite;
    g_rest2_layers->flags |= 0x2000;
    g_rest2_fdoor_m  = LoadSprite("R2Fdoor_m.lls", 1);
    g_rest2_fdoor_m1 = LoadSprite("R2Fdoor_m1.lls", 1);
    g_rest2_bdoor_m  = LoadSprite("R2Bdoor_m.lls", 1);
    g_rest2_tower_m  = LoadSprite("R2Tower_m.lls", 1);
    HideLayer(g_rest2_layers, 0);
    StopLayerPlaying(g_rest2_layers, 0);
    LLSSetFrame(GetLLSForLayer(g_rest2_layers, 0), 0);
    HideLayer(g_rest2_layers, 6);
    StopLayerPlaying(g_rest2_layers, 6);
    LLSSetFrame(GetLLSForLayer(g_rest2_layers, 6), 0);
    HideLayer(g_rest2_layers, 2);
    StopLayerPlaying(g_rest2_layers, 2);
    LLSSetFrame(GetLLSForLayer(g_rest2_layers, 2), 0);
    HideLayer(g_rest2_layers, 5);
    HideLayer(g_rest2_layers, 1);
    StopLayerPlaying(g_rest2_layers, 1);
    LLSSetFrame(GetLLSForLayer(g_rest2_layers, 1), 0);
    HideLayer(g_rest2_layers, 3);
    StopLayerPlaying(g_rest2_layers, 3);
    LLSSetFrame(GetLLSForLayer(g_rest2_layers, 3), 0);
}


/* =========================================================================
 * 0x0041bfb0 -- BOATING SCHOOL WATER +0x94   (see the file header)
 * 0x00436470 -- JUNGLE CRUISE WATER  +0x94
 *
 * The two bodies are the same function written twice, register for register
 * -- the same "one piece of code written twice" anim2.c records for
 * BsWater_Relink / JungleCruise_RelinkRiverCell and ridecb6.c for
 * BsWater_Add / JcWater_Add.  Only the list heads, the record's `next`
 * offset (BsBoat +0x3f0 vs JcBoat +0x3f4, BsStation +0x2c vs JcStation
 * +0x3c), the fixed footprint constant and the ride's own +0x94 differ.
 * ========================================================================= */

/* ---- the map grid (same layout as junglecruise.c / objmap2.c) ----------- */
typedef struct Cell {
    void*          obj;             /* +0x00  the object descriptor on the cell */
    BPosW          key;             /* +0x04  the map square that object hangs off */
    unsigned char  pad06[0x14 - 0x06];
} Cell;                             /* 0x14 */

typedef struct MapHdr {
    unsigned char  pad00[0x14];
    unsigned short width;           /* +0x14 */
    unsigned short height;          /* +0x16 */
} MapHdr;

extern MapHdr* g_map;               /* 0x004bcbf4 (lpConfig) */
extern Cell**  g_map_rows;          /* 0x00801400 (GameMap) */

/* The bounds-checked cell fetch every map accessor open-codes; the result is
 * dereferenced WITHOUT a null check, exactly as the original does. */
static __inline Cell* MapCellAtRef(const Pos* p)
{
    if (p->x >= 0 && p->x < g_map->width && p->y >= 0 && p->y < g_map->height)
        return &g_map_rows[p->y][p->x];
    return 0;
}

/* The edit-selection state objmap2.c owns: the class under the cursor and
 * the map square it sits on. */
typedef struct Cursor { unsigned char pad[0x1834]; } Cursor;
extern RideDef* g_sel_def;                                    /* 0x00667c58 */
extern BPosW    g_sel_bpos;                                   /* 0x00667c54 */
extern Cursor   g_ghost_cursor;                               /* 0x00810160 */

extern void BasicObjectDCalcCursor(RideElem* elem, Pos* p);   /* 0x00480bb0 */
extern void SetCursorError(Cursor* c, int code);              /* 0x0045f480 */

/* ---- BOATING SCHOOL WATER ----------------------------------------------- */
typedef struct BsStation {
    BPosW             pos;          /* +0x00 the building's map square */
    BPosW             a;            /* +0x02 route START square (near jetty) */
    BPosW             b;            /* +0x04 route END square   (far jetty) */
    unsigned char     pad06[0x2c - 6];
    struct BsStation* next;         /* +0x2c */
} BsStation;                        /* 0x34 */

typedef struct BsWater {
    BPosW           pos;            /* +0x00 */
    BPosW           owner;          /* +0x02 the school that owns this cell */
    unsigned char   pad04[0x1c - 4];
} BsWater;                          /* 0x1c */

typedef struct BsBoat {
    unsigned char  pad00[4];
    int            cx;              /* +0x04 the map square it is on */
    int            cy;              /* +0x08 */
    int            nx;              /* +0x0c the map square it is heading for */
    int            ny;              /* +0x10 */
    unsigned char  pad14[0x3f0 - 0x14];
    struct BsBoat* next;            /* +0x3f0 */
} BsBoat;                           /* 0x3f4 */

extern BsStation* g_bs_stations;                              /* 0x004cc074 */
/* The BOATING SCHOOL class footprint, saved by BoatingSchool_Create. */
extern Rect       g_bs_footprint;                             /* 0x004cc078 */
extern BsBoat*    g_bs_boats;                                 /* 0x004cc03c */
extern RideDef*   g_bs_def;                                   /* 0x0082c658 */

extern BsWater* BsWater_FindAt(int x, int y);                 /* 0x0041c890 */
extern void BoatingSchool_DrawSelection(RideElem* elem, Pos* p); /* 0x0041a3d0 */

/* The fixed 5x5 footprint one lake square always takes (0x004b53c0). */
static const Rect kBsWaterRect = { -2, -2, 2, 2, 0 };

/* Scope F close (2026-09-05): 129/129 instructions, 374/374 bytes,
 * strict/rb/ob 0/0/0. First update BOTH cursor coordinates, then narrow
 * p->x and p->y into the packed square through its array view. Narrowing
 * breaks the cell-x value reuse across BasicObjectDCalcCursor; separating
 * both cursor writes from both byte stores gives the original early-load
 * schedule without the extra byte-register copies of the chained form.
 * The station cursor, coordinate registers and later boat-loop reloads now
 * all agree. This supersedes the prior claim that x could not rematerialise.
 * The identical change closes JcWater_DrawSelection too. Whole-file audit
 * preserves the 13 existing exact bodies and adds these two exact matches.
 * See docs/lanes/scope-f.md for the measured intermediate forms. */
// FUNCTION: LEGOLAND 0x0041bfb0
void BsWater_DrawSelection(RideElem* elem, Pos* p)
{
    BsStation* st;
    BsBoat*    boat;
    BsWater*   w;
    Cell*      c;
    BPosW      sq;
    RideElem   ride;

    st = g_bs_stations;
    c = MapCellAtRef(p);
    p->x = c->key.b.x;
    p->y = c->key.b.y;
    sq.c[0] = (unsigned char)p->x;
    sq.c[1] = (unsigned char)p->y;
    while (st) {
        if (sq.w == st->a.w || sq.w == st->b.w) {
            w = BsWater_FindAt(p->x, p->y);
            p->x = g_sel_bpos.b.x = w->owner.b.x;
            p->y = g_sel_bpos.b.y = w->owner.b.y;
            g_sel_def->footprint = g_bs_footprint;
            ride.data = g_bs_def;
            BoatingSchool_DrawSelection(&ride, p);
            return;
        }
        st = st->next;
    }
    boat = g_bs_boats;
    g_sel_def->footprint = kBsWaterRect;
    BasicObjectDCalcCursor(elem, p);
    while (boat) {
        if ((sq.b.x == boat->cx && sq.b.y == boat->cy)
         || (sq.b.x == boat->nx && sq.b.y == boat->ny)) {
            SetCursorError(&g_ghost_cursor, 1);
            return;
        }
        boat = boat->next;
    }
}


/* ---- JUNGLE CRUISE WATER ------------------------------------------------ */
typedef struct JcStation {
    BPosW             pos;          /* +0x00 the station's own map square */
    BPosW             a;            /* +0x02 route START square */
    BPosW             b;            /* +0x04 route END square */
    unsigned char     pad06[0x3c - 6];
    struct JcStation* next;         /* +0x3c */
} JcStation;                        /* 0x44 */

typedef struct JcWater {
    BPosW           pos;            /* +0x00 */
    BPosW           owner;          /* +0x02 the river this cell belongs to */
    unsigned char   pad04[0x1c - 4];
} JcWater;                          /* 0x1c */

typedef struct JcBoat {
    unsigned char  pad00[4];
    int            cx;              /* +0x04 the map square it is on */
    int            cy;              /* +0x08 */
    int            nx;              /* +0x0c the map square it is heading for */
    int            ny;              /* +0x10 */
    unsigned char  pad14[0x3f4 - 0x14];
    struct JcBoat* next;            /* +0x3f4 */
} JcBoat;                           /* 0x3f8 */

extern JcStation* g_jc_stations;                              /* 0x00629c3c */
/* The JUNGLE CRUISE class footprint, saved by the ride's create handler. */
extern Rect       g_jc_footprint;                             /* 0x00629c40 */
extern JcBoat*    g_jc_boats;                                 /* 0x00616164 */
extern RideDef*   g_jc_def;                                   /* 0x0081cb60 */

extern JcWater* JcWater_FindAt(int x, int y);                 /* 0x004371b0 */
extern void JungleCruise_DrawSelection(RideElem* elem, Pos* p); /* 0x00435230 */

/* The fixed 5x5 footprint one river square always takes (0x004b7478; the same
 * rect ridecb9.c calls kJcWaterRect). */
static const Rect kJcRiverRect = { -2, -2, 2, 2, 0 };

/* Scope F close (2026-09-05): 129/129 instructions, 374/374 bytes,
 * strict/rb/ob 0/0/0. Updating both cursor coordinates before copying their
 * narrowed values into sq.c[] closes the same register/spill residual as
 * BsWater_DrawSelection above. Full-file audit adds both exact twins while
 * preserving all 13 previously exact functions. */
// FUNCTION: LEGOLAND 0x00436470
void JcWater_DrawSelection(RideElem* elem, Pos* p)
{
    JcStation* st;
    JcBoat*    boat;
    JcWater*   w;
    Cell*      c;
    BPosW      sq;
    RideElem   ride;

    st = g_jc_stations;
    c = MapCellAtRef(p);
    p->x = c->key.b.x;
    p->y = c->key.b.y;
    sq.c[0] = (unsigned char)p->x;
    sq.c[1] = (unsigned char)p->y;
    while (st) {
        if (sq.w == st->a.w || sq.w == st->b.w) {
            w = JcWater_FindAt(p->x, p->y);
            p->x = g_sel_bpos.b.x = w->owner.b.x;
            p->y = g_sel_bpos.b.y = w->owner.b.y;
            g_sel_def->footprint = g_jc_footprint;
            ride.data = g_jc_def;
            JungleCruise_DrawSelection(&ride, p);
            return;
        }
        st = st->next;
    }
    boat = g_jc_boats;
    g_sel_def->footprint = kJcRiverRect;
    BasicObjectDCalcCursor(elem, p);
    while (boat) {
        if ((sq.b.x == boat->cx && sq.b.y == boat->cy)
         || (sq.b.x == boat->nx && sq.b.y == boat->ny)) {
            SetCursorError(&g_ghost_cursor, 1);
            return;
        }
        boat = boat->next;
    }
}


/* =========================================================================
 * 0x00419d10 -- BOATING SCHOOL +0xa4  (loaders.c reaches the same address as
 * the class's `init` slot, so this is the school's whole resource setup)
 *
 * Beyond the usual four create steps it does three things worth naming:
 *
 *  1. It resolves TWO LLIDB elements by name rather than loading files --
 *     "BOATING SCHOOL TILE MAPPING" (the lake's tileset descriptor, which
 *     ridecb5.c/ridecb8.c quote their base water tile from) and "BOATING
 *     SCHOOL BOATS" (the boat image list).  LLIDB_FindElement returns 0 when
 *     the element is NOT already loaded, so the load call is on the FALSE
 *     arm; a non-zero return means it was already resident and the global is
 *     simply re-read.
 *
 *  2. Every boat sprite in the image list is started playing.  Note the
 *     subscript: the loop counter is a full int (`inc esi` / signed `cmp`
 *     against the list count) but the array index is narrowed to a BYTE, so
 *     an image list longer than 256 entries would wrap -- reproduced.
 *
 *  3. It derives the class's three geometry rects (ridecb5.c names them:
 *     0x004cc078 the whole footprint, 0x004cc060 dock A, 0x004cc048 dock B)
 *     from the ObjDef's own footprint.  Dock B is the FAR jetty, the 5x5
 *     patch of water five cells NORTH of the building's top edge; dock A is
 *     the NEAR jetty, the 5x5 patch immediately SOUTH of its bottom edge.
 *     Those two rects are what the add handler turns into the station's
 *     route start/end squares.
 * ========================================================================= */

/* The 0x08-byte-header image list: `count` entries of `sprites`. */
typedef struct ImageList {
    unsigned char pad00[4];
    int           count;            /* +0x04 */
    void**        sprites;          /* +0x08 */
} ImageList;

/* A sprite record, as far as this file reads it. */
typedef struct SprRec {
    unsigned char pad00[8];
    void*         def;              /* +0x08  the SpriteDef LLSPlay wants */
} SprRec;

/* The decoded-bitmap record, as far as this file reads it. */
typedef struct LLS {
    short         frame;            /* +0x00  the frame being shown */
    unsigned char pad02[0x10 - 2];
    short         count;            /* +0x10 */
} LLS;

extern int   LLIDB_FindElement(const char* name, void** out,
                               unsigned int* idx);            /* 0x0047b330 */
extern void* LLIDB_LoadData(void* elem);                      /* 0x0047d3a0 */
extern void* GetSpriteForLayer(Spr* sprite, int layer);       /* 0x00441ec0 */
extern LLS*  GetLLSForSprite(void* sprite);                   /* 0x00441e80 */
#ifndef LEGOLAND_PORTABLE
extern void  LLSStop(LLS* lls);                               /* 0x0047d4c0 */
#else
extern int LLSStop(LLS* lls);                               /* 0x0047d4c0 */
#endif
extern void  LLSPlay(LLS* lls, void* owner);                  /* 0x0047d520 */

extern FXEntry    g_bs_fx[2];                                 /* 0x004b52c0 */
extern void*      g_bs_water_tiles;                           /* 0x0082adf4 */
extern ImageList* g_bs_boat_ilf;                              /* 0x0082c65c */
extern Spr*       g_bs_hullmask;                              /* 0x0082adfc */
extern Spr*       g_bs_railm;                                 /* 0x0082c654 */
extern void*      g_bs_sprite;                                /* 0x0082ae00 */
extern Rect       g_bs_dock_b;                                /* 0x004cc048 */
extern Rect       g_bs_dock_a;                                /* 0x004cc060 */

/* Dock B sits five cells above the footprint's top edge, dock A one cell
 * below its bottom edge; both are 5x5 (0x004b5260 / 0x004b5278). */
static const Rect kBsDockB = { 0, -5, 4, -1, 0 };
static const Rect kBsDockA = { 0,  0, 4,  4, 0 };

/* CODEGEN NOTE: the four `+=` on each rect must be written top, left, right,
 * bottom.  VC6 forward-propagates the FIRST field read after a struct copy
 * back to the copy's .rdata source -- the original's `mov eax,[4b5264h]` is
 * `kBsDockB.top` read from the constant, not from the destination -- and the
 * remaining three then read the destination.  Any other order forwards a
 * different field and costs 16 to 35 instructions. */
// FUNCTION: LEGOLAND 0x00419d10
void BoatingSchool_Create(RideElem* elem)
{
    RideDef* def;
    void*    e;
    LLS*     lls;
    int      i;

    Load_FXList(g_bs_fx, 2);
    def = elem->data;
    g_bs_def = def;
    def->flags |= 0x20;
    g_bs_def->sprite->flags |= 0x2000;
    if (LLIDB_FindElement("BOATING SCHOOL TILE MAPPING", &e, 0) == 0)
        g_bs_water_tiles = LLIDB_LoadData(e);
    if (LLIDB_FindElement("BOATING SCHOOL BOATS", &e, 0) == 0)
        g_bs_boat_ilf = (ImageList*)LLIDB_LoadData(e);
    for (i = 0; i < g_bs_boat_ilf->count; i++) {
        SprRec* spr = (SprRec*)g_bs_boat_ilf->sprites[(unsigned char)i];
        LLSPlay(GetLLSForSprite(spr), spr->def);
    }
    g_bs_hullmask = LoadSprite("bs_hullmask.lls", 1);
    g_bs_railm = LoadSprite("bs_railm.lls", 1);
    g_bs_sprite = GetSpriteForLayer(g_bs_def->sprite, 5);
    lls = GetLLSForSprite(g_bs_sprite);
    LLSStop(lls);
    LLSSetFrame(lls, lls->count);
    g_bs_footprint = g_bs_def->footprint;
    g_bs_dock_b = kBsDockB;
    g_bs_dock_b.top    += g_bs_footprint.top;
    g_bs_dock_b.left   += g_bs_footprint.left;
    g_bs_dock_b.right  += g_bs_footprint.left;
    g_bs_dock_b.bottom += g_bs_footprint.top;
    g_bs_dock_a = kBsDockA;
    g_bs_dock_a.top    += g_bs_footprint.bottom + 1;
    g_bs_dock_a.left   += g_bs_footprint.left;
    g_bs_dock_a.right  += g_bs_footprint.left;
    g_bs_dock_a.bottom += g_bs_footprint.bottom + 1;
}


/* =========================================================================
 * 0x00405940 -- DRIVING SCHOOL +0x9c  (remove one placed driving school)
 *
 * Tearing a school down is four independent sweeps, and all four list heads
 * are read at entry -- BEFORE StandardRemoveObject -- so a head that call
 * changed would be missed (the same eager-read shape ridecb9.c records for
 * JungleCruise_DrawSelection):
 *
 *   1. the school's own record (g_ds_schools, key at +0x00, next at +0x08)
 *      is unlinked and freed;
 *   2. every pump belonging to it is dropped (0x00411ba0);
 *   3. every ROAD BLOCK whose owner is this school is torn up: a block
 *      carrying a zebra crossing (type bit 0x10) loses the bit, is restitched
 *      and refunds the ZEBRA CROSSING class's cost, then the block itself is
 *      removed through the shared road preview cursor and freed;
 *   4. every car of the school is retired.
 *
 * Finally the class refunds FIVE times the road class's own cost -- once, not
 * per block -- and the remaining customers are thrown off the ride.
 *
 * The square all four sweeps match against is g_sel_bpos (0x00667c54), the
 * map square under the edit cursor, NOT the `tile` argument; only the school
 * record lookup and the final RemoveAllBlokesFromRide use `tile`.
 *
 * ORIGINAL BUG, reproduced: in the school-record unlink the non-head arm
 * relinks `prev->next = prev->next->next` and then frees `prev->next` -- the
 * record AFTER the one it meant to remove.  The removed record leaks and a
 * still-linked record is freed.  The head arm is correct.
 * ========================================================================= */

typedef struct DsSchool {
    BPosW            key;           /* +0x00 its map square */
    int              take;          /* +0x04 accumulated takings */
    struct DsSchool* next;          /* +0x08 */
} DsSchool;                         /* 0x0c */

typedef struct RoadTile {
    struct RoadTile* next;          /* +0x00 */
    unsigned char    pad04[4];
    unsigned short   school;        /* +0x08 the owning school's map square */
    unsigned char    pad0a[2];
    int              x;             /* +0x0c map square */
    int              y;             /* +0x10 */
    unsigned char    type;          /* +0x14 low nibble kind, 0x10 = crossing */
} RoadTile;

typedef struct SchoolCar {
    struct SchoolCar* next;         /* +0x00 */
    unsigned short    school;       /* +0x04 */
} SchoolCar;

/* The road preview cursor (ridecb5.c's g_road_preview / ridecb8.c's
 * g_road_cursor2), reused here to remove each block. */
typedef struct EditCursorRec {
    unsigned char pad0000[0x1404];
    int           x;                /* +0x1404 */
    int           y;                /* +0x1408 */
    unsigned char pad140c[8];
    Rect          footprint;        /* +0x1414 */
} EditCursorRec;

extern EditCursorRec g_road_preview;                          /* 0x0082f760 */
extern DsSchool*  g_ds_schools;                               /* 0x004c11bc */
extern RoadTile*  g_road_tiles;                               /* 0x004cbeac */
extern SchoolCar* g_school_cars;                              /* 0x004c10d4 */
extern RideDef*   g_zebra_def;                                /* 0x0082c678 */
extern RideDef*   g_roads_def;                                /* 0x0082c684 */

extern void  StandardRemoveObject(void* obj, unsigned int tile,
                                  EditCursorRec* ctx);        /* 0x0045f220 */
/* The SAME routine declared a second time with a 16-bit `tile`: the road
 * block's owner is a u16 field and the original pushes it with `mov ax,[..] /
 * push eax`, i.e. WITHOUT zero-extending, which only a `unsigned short`
 * parameter produces.  (ridecb6.c does the same for 0x0041b0d0.) */
extern void  StandardRemoveObject_W(void* obj, unsigned short tile,
                                    EditCursorRec* ctx);      /* 0x0045f220 */
extern void  RemoveAllBlokesFromRide(void* cls, unsigned int tile); /* 0x0048a2e0 */
extern void  DefaultCursor(EditCursorRec* c);                 /* 0x0045a390 */
extern void  AddBricks(int n);                                /* 0x004578a0 */
extern void  HeapFree_w(void* p);                             /* 0x0049e4d0 */
extern void  Pump_RemoveAllForSchool(BPosW school);           /* 0x00411ba0 */
extern void  Road_Restitch(BPosW school, int x, int y);       /* 0x00413650 */
extern void  Road_Delete(int x, int y);                       /* 0x004133e0 */
extern void  RetireSchoolCar(SchoolCar* car);                 /* 0x00401c60 */

/* The 4x4 block one road record covers (0x004b4bf0). */
static const Rect kRoadBlockRect = { 0, 0, 3, 3, 0 };

// FUNCTION: LEGOLAND 0x00405940
void DrivingSchool_Remove(void* obj, unsigned int tile, EditCursorRec* ctx)
{
    SchoolCar* car = g_school_cars;
    DsSchool*  s = g_ds_schools;
    RoadTile*  rec = g_road_tiles;
    RoadTile*  rnext;
    SchoolCar* cnext;

    StandardRemoveObject(obj, tile, ctx);
    DefaultCursor(&g_road_preview);
    g_road_preview.footprint = kRoadBlockRect;
    if (s->key.w == (unsigned short)tile) {
        g_ds_schools = s->next;
        HeapFree_w(s);
    } else {
        while (s->next) {
            if (s->next->key.w == (unsigned short)tile) {
                /* ORIGINAL BUG: frees the record AFTER the one unlinked. */
                s->next = s->next->next;
                HeapFree_w(s->next);
                break;
            }
            s = s->next;
        }
    }
    Pump_RemoveAllForSchool(g_sel_bpos);
    while (rec) {
        rnext = rec->next;
        if (rec->school == g_sel_bpos.w) {
            if (rec->type & 0x10) {
                rec->type &= ~0x10;
                Road_Restitch(*(BPosW*)&rec->school, rec->x, rec->y);
                AddBricks(g_zebra_def->cost);
            }
            g_road_preview.x = rec->x;
            g_road_preview.y = rec->y;
            StandardRemoveObject_W(g_roads_def->elem, rec->school, &g_road_preview);
            Road_Delete(rec->x, rec->y);
        }
        rec = rnext;
    }
    AddBricks(g_roads_def->cost * 5);
    while (car) {
        cnext = car->next;
        if (car->school == g_sel_bpos.w)
            RetireSchoolCar(car);
        car = cnext;
    }
    RemoveAllBlokesFromRide(((RideDef**)obj)[3], tile);
}


/* =========================================================================
 * 0x0042ea60 -- CASTLE BBQ +0xa8  (the class tick: one pass over the ride's
 * rider list)
 *
 * Structurally the same body as ridecb4.c's OctopusCafe_Tick (0x004316f0) --
 * walk the ObjDef's class-wide rider list, skip anybody whose low-level AI
 * state is busy, and step the customer's own seven-stage state machine -- but
 * the BBQ has no seating, so the whole thing is one queue-and-leave cycle:
 *
 *   0  arrive: claim the ride (flags 8) and walk to the serving point
 *   1  walk to the counter (the y target is NOT centred in its cell here,
 *      the only stage with no +0x80 -- reproduced)
 *   2  being served: wait (rand() & 0x1f) + 4 ticks
 *   3  each tick of that wait, on the LAST one, pay (the money jingle plays
 *      at the customer's own map square) and advance
 *   4  walk back out to the serving point
 *   5  walk off the class's origin square
 *   6  leave the ride and drop the "using this ride" flag
 *
 * The two map coordinates the walk targets are built from are computed ONCE
 * per rider at the top of the loop -- the class's origin square plus the
 * rider node's own packed square -- and each stage only scales them into
 * 24.8 world units.
 * ========================================================================= */

typedef struct MapSquare {
    unsigned char bx;               /* +0x00 */
    unsigned char by;               /* +0x01 */
} MapSquare;

typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;           /* +0x0e  low-level AI state (0 = idle) */
    unsigned char  pad10[0x14];
    Pos            target;          /* +0x24  walk target, 24.8 */
    unsigned char  pad2c[0x58 - 0x2c];
    int            timer;           /* +0x58  countdown ticks */
    unsigned char  pad5c[0x60 - 0x5c];
    unsigned char  action;          /* +0x60  state-machine stage */
    unsigned char  pad61;
    unsigned short flags62;         /* +0x62  8 = using this ride */
    unsigned char  pad64[4];
    Pos            world;           /* +0x68  world position, 24.8 */
    unsigned short f70;             /* +0x70 */
    unsigned char  dir;             /* +0x72  current heading */
    unsigned char  new_dir;         /* +0x73  requested heading */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14];      /* +0x98  CalcMoveLine scratch */
} Bloke;

typedef struct RiderNode {
    struct RiderNode* next;         /* +0x00 */
    struct RiderNode* prev;         /* +0x04 */
    Bloke*            bloke;        /* +0x08 */
    unsigned short    ride_id;      /* +0x0c  packed {x,y} of the instance */
    unsigned short    pad0e;
} RiderNode;

extern int  rand(void);                                       /* 0x0049e4b2 (CRT) */
extern int  CalcMoveLine(Pos from, Pos to, void* path);       /* 0x00480740 */
extern int  NewDirForAction(Bloke* b, unsigned char dir);     /* 0x004833d0 */
extern void RemoveBlokeFromRide(RideDef* def, RiderNode* r);  /* 0x0048a100 */
extern void PlayMoneySFX(MapSquare* at, int which, int flags);/* 0x00453950 */

// FUNCTION: LEGOLAND 0x0042ea60
void CastleBbq_Tick(RideElem* elem)
{
    RideDef*      def = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           x;
    int           y;
    unsigned char a;

    r = def->riders;
    while (r) {
        next = r->next;
        key = (MapSquare*)&r->ride_id;
        b = r->bloke;
        x = key->bx + def->base_x;
        y = key->by + def->base_y;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x = (x << 8) + 0x280;
                b->target.y = (y << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 1:
                /* The one stage whose y target is NOT centred in its cell.
                 * Written x-then-y like every other case: VC6 reorders the
                 * two adjacent stores (y lands first) and re-reads whichever
                 * it stored first as CalcMoveLine's argument.  Writing the
                 * source in the emitted order costs one instruction. */
                b->target.x = (x << 8) + 0x280;
                b->target.y = y << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 2:
                b->timer = (rand() & 0x1f) + 4;
                b->action++;
                break;
            case 3:
                if (b->timer == 0) {
                    b->action++;
                    PlayMoneySFX(key, 0, 0);
                }
                b->timer--;
                break;
            case 4:
                b->target.x = (x << 8) + 0x280;
                b->target.y = (y << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 5:
                b->target.x = (x << 8) + 0x80;
                b->target.y = (y << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 6:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8;
                break;
            }
        }
        r = next;
    }
}


/* =========================================================================
 * 0x0042ec10 -- FOODCART DRINK +0xa8  (the class tick)
 *
 * The same rider-list walk as CastleBbq_Tick above with a five-stage machine
 * instead of seven; a food cart has no service point of its own, so the
 * customer only walks to the cart, faces it, buys, turns away and leaves:
 *
 *   0  claim the ride (flags 8) and walk to the cart's serving side
 *      (half a cell WEST of the class origin, one and a half cells SOUTH)
 *   1  face south-west (heading 7) and wait (rand() & 0x1f) + 4 ticks
 *   2  on the last tick of that wait, pay for one item
 *   3  turn to heading 3 and walk back to the centre of the cell
 *   4  leave the ride and drop the "using this ride" flag
 *
 * Unlike the BBQ this one still needs `elem` after the prologue (BuyItem
 * takes it), so the cached ObjDef gets a frame slot of its own instead of
 * reusing the parameter's home -- that is the whole difference between this
 * body's `sub esp,8` and the BBQ's `push ecx`.
 * ========================================================================= */

extern void BuyItem(RideElem* elem, MapSquare* at, int which);/* 0x004539e0 */
extern void NewLongTermAction(Bloke* b, int action);          /* 0x0044e760 */

// FUNCTION: LEGOLAND 0x0042ec10
void FoodcartDrink_Tick(RideElem* elem)
{
    RideDef*      def = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           x;
    int           y;
    unsigned char a;

    r = def->riders;
    while (r) {
        next = r->next;
        key = (MapSquare*)&r->ride_id;
        b = r->bloke;
        x = key->bx + def->base_x;
        y = key->by + def->base_y;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x = (x << 8) - 0x80;
                b->target.y = (y << 8) + 0x180;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 1:
                b->dir = 7;
                b->action++;
                b->timer = (rand() & 0x1f) + 4;
                break;
            case 2:
                if (b->timer == 0) {
                    b->action++;
                    BuyItem(elem, key, 1);
                }
                b->timer--;
                break;
            case 3:
                b->target.x = (x << 8) + 0x80;
                b->dir = 3;
                b->target.y = (y << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 4:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8;
                break;
            }
        }
        r = next;
    }
}


/* =========================================================================
 * 0x0042ed70 -- FOODCART FOOD +0xa8  (the class tick)
 *
 * Byte for byte the same body as SharkCafe_Tick below -- the two classes
 * carry their own copy of the same source.  It differs from FOODCART DRINK
 * only in the serving point: the customer stops HALF A CELL WEST of the
 * class origin on the same row (the drink cart pulls them one and a half
 * cells south) and the walk-away stage does not re-aim the heading.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0042ed70
void FoodcartFood_Tick(RideElem* elem)
{
    RideDef*      def = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           x;
    int           y;
    unsigned char a;

    r = def->riders;
    while (r) {
        next = r->next;
        key = (MapSquare*)&r->ride_id;
        b = r->bloke;
        x = key->bx + def->base_x;
        y = key->by + def->base_y;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x = (x << 8) - 0x80;
                b->target.y = (y << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 1:
                b->dir = 7;
                b->action++;
                b->timer = (rand() & 0x1f) + 4;
                break;
            case 2:
                if (b->timer == 0) {
                    b->action++;
                    BuyItem(elem, key, 1);
                }
                b->timer--;
                break;
            case 3:
                b->target.x = (x << 8) + 0x80;
                b->target.y = (y << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 4:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8;
                break;
            }
        }
        r = next;
    }
}


/* =========================================================================
 * 0x0042e610 -- SHARK CAFE +0xa8  (the class tick)
 *
 * FoodcartFood_Tick's source again, with ONE extra statement: a customer who
 * has finished at the shark cafe is given long-term action 0xd on the way
 * out, so the cafe releases them into a specific plan instead of leaving the
 * simulator to pick one.  Nothing else differs -- the two bodies are
 * otherwise instruction for instruction the same.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0042e610
void SharkCafe_Tick(RideElem* elem)
{
    RideDef*      def = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           x;
    int           y;
    unsigned char a;

    r = def->riders;
    while (r) {
        next = r->next;
        key = (MapSquare*)&r->ride_id;
        b = r->bloke;
        x = key->bx + def->base_x;
        y = key->by + def->base_y;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x = (x << 8) - 0x80;
                b->target.y = (y << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 1:
                b->dir = 7;
                b->action++;
                b->timer = (rand() & 0x1f) + 4;
                break;
            case 2:
                if (b->timer == 0) {
                    b->action++;
                    BuyItem(elem, key, 1);
                }
                b->timer--;
                break;
            case 3:
                b->target.x = (x << 8) + 0x80;
                b->target.y = (y << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 4:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8;
                NewLongTermAction(b, 0xd);
                break;
            }
        }
        r = next;
    }
}


/* =========================================================================
 * 0x00431170 -- FOODCART ICECREAM +0xa8  (the class tick)
 *
 * The third food cart, and the plainest: the customer walks to the point
 * half a cell EAST and half a cell NORTH of the class origin, waits without
 * being turned to face the cart, buys, and walks back to the centre of the
 * cell.  Because stage 1 has one store fewer than the other two carts, VC6
 * saves a fourth register BEFORE the empty-list guard instead of after it,
 * which is why this body's prologue splits differently.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00431170
void FoodcartIcecream_Tick(RideElem* elem)
{
    RideDef*      def = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           x;
    int           y;
    unsigned char a;

    r = def->riders;
    while (r) {
        next = r->next;
        key = (MapSquare*)&r->ride_id;
        b = r->bloke;
        x = key->bx + def->base_x;
        y = key->by + def->base_y;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x = (x << 8) + 0x80;
                b->target.y = (y << 8) - 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 1:
                b->action++;
                b->timer = (rand() & 0x1f) + 4;
                break;
            case 2:
                if (b->timer == 0) {
                    b->action++;
                    BuyItem(elem, key, 1);
                }
                b->timer--;
                break;
            case 3:
                b->target.x = (x << 8) + 0x80;
                b->target.y = (y << 8) + 0x80;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 4:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8;
                break;
            }
        }
        r = next;
    }
}


/* =========================================================================
 * 0x00434cb0 -- JUNGLE CRUISE +0xa4  (the ride's whole resource setup)
 *
 * BoatingSchool_Create's source with the school-specific parts removed: no
 * FX list, no build-sprite layer to park, and a river wobble table to build
 * at the end instead.  The two LLIDB elements it resolves are the water
 * tileset -- and note the NAME, "BOATING SCHOOL TILE MAPPING": the jungle
 * cruise river is drawn with the boating school's tileset, which is why
 * junglecruise.c/ridecb9.c find the same descriptor cached in two globals --
 * and "JUNGLE CRUISE BOATS", the boat image list, every sprite of which is
 * started playing (with the same byte-narrowed subscript).
 *
 * The three geometry rects come out exactly as the school's: the class
 * footprint is copied to 0x00629c40 and the two 5x5 jetty rects are offset
 * from it, dock B five cells north of the top edge and dock A one cell south
 * of the bottom edge.  The four `+=` per rect must be written top, left,
 * right, bottom for the same reason (see BoatingSchool_Create).
 * ========================================================================= */

extern void*      g_jc_tsm;                                   /* 0x0081cb58 */
extern Spr*       g_jc_mask;                                  /* 0x0081cb5c */
extern ImageList* g_jc_boat_ilf;                              /* 0x0081cd00 */
extern Rect       g_jc_dock_b;                                /* 0x004b7260 */
extern Rect       g_jc_dock_a;                                /* 0x004b7278 */
extern void       JungleCruise_BuildWobbleTable(void);        /* 0x00432ac0 */

static const Rect kJcDockB = { 0, -5, 4, -1, 0 };
static const Rect kJcDockA = { 0,  0, 4,  4, 0 };

// FUNCTION: LEGOLAND 0x00434cb0
void JungleCruise_Create(RideElem* elem)
{
    RideDef* def;
    void*    e;
    int      i;

    def = elem->data;
    g_jc_def = def;
    def->flags |= 0x20;
    g_jc_def->sprite->flags |= 0x2000;
    if (LLIDB_FindElement("BOATING SCHOOL TILE MAPPING", &e, 0) == 0)
        g_jc_tsm = LLIDB_LoadData(e);
    if (LLIDB_FindElement("JUNGLE CRUISE BOATS", &e, 0) == 0)
        g_jc_boat_ilf = (ImageList*)LLIDB_LoadData(e);
    for (i = 0; i < g_jc_boat_ilf->count; i++) {
        SprRec* spr = (SprRec*)g_jc_boat_ilf->sprites[(unsigned char)i];
        LLSPlay(GetLLSForSprite(spr), spr->def);
    }
    g_jc_mask = LoadSprite("jungmask.lls", 1);
    g_jc_footprint = g_jc_def->footprint;
    g_jc_dock_b = kJcDockB;
    g_jc_dock_b.top    += g_jc_footprint.top;
    g_jc_dock_b.left   += g_jc_footprint.left;
    g_jc_dock_b.right  += g_jc_footprint.left;
    g_jc_dock_b.bottom += g_jc_footprint.top;
    g_jc_dock_a = kJcDockA;
    g_jc_dock_a.top    += g_jc_footprint.bottom + 1;
    g_jc_dock_a.left   += g_jc_footprint.left;
    g_jc_dock_a.right  += g_jc_footprint.left;
    g_jc_dock_a.bottom += g_jc_footprint.bottom + 1;
    JungleCruise_BuildWobbleTable();
}


/* =========================================================================
 * 0x0041abd0 -- BOATING SCHOOL +0xb0  (the class's custom draw)
 *
 * The +0xb0 slot is what the sprite flag 0x2000 the create handler set arms:
 * the render walk hands the class the whole square instead of blitting the
 * build sprite itself.  For the boating school that means, in order:
 *
 *   1. step and repaint the lake (BoatingSchool_UpdateWater, mode 1);
 *   2. blit the boat HULL MASK over layer 3, first copying layer 3's own
 *      animation frame onto it so the mask stays in step with the jetty;
 *   3. render, in 3D, every customer of the class standing on THIS square
 *      whose stage is not 2 (stage 2 is the one spent inside the building,
 *      where the walls would have to occlude them);
 *   4. blit the RAIL MASK, the same layer-3 offset shifted (0x71, 0xac).
 *
 * Both blits take the layer offset through AdjustOffsetForViewMode and add
 * the object's own screen position; the `x`/`y` parameters the render walk
 * passes are ignored.  Note the ObjDef is re-read from its global at every
 * use rather than kept in the local the prologue already loaded.
 * ========================================================================= */

/* An {x,y} pair returned in eax:edx. */
typedef struct Offset { int ox; int oy; } Offset;

extern void   BoatingSchool_UpdateWater(int mode);            /* 0x00418fe0 */
extern Offset GetScreenCoordsForObject(MapSquare* sq, RideDef* def); /* 0x00442cc0 */
extern Offset GetRenderOffsetForLayer(Spr* sprite, int layer);/* 0x00441ee0 */
extern void   AdjustOffsetForViewMode(Offset* o);             /* 0x00442d30 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern void   IP_RenderBlokeIn3DNow(Bloke* b);                /* 0x00440010 */

/* CODEGEN NOTES: (1) the two LLS handles must be separate statements -- as one
 * nested call expression VC6 reads `lls->frame` before making the other call
 * and emits the argument cleanups separately, where the original keeps the
 * layer handle in ebx across the second call and defers every cleanup into
 * one `add esp,44h`; (2) both blit sums must be written screen-FIRST: with
 * `off` first the offsets keep the add's destination and VC6 emits `lea`
 * where the original has an in-place `add`. */
// FUNCTION: LEGOLAND 0x0041abd0
void BoatingSchool_Draw(RideElem* elem, int x, int y, MapSquare* sq,
                        void* clip, int mode)
{
    RideDef*   def = elem->data;
    RiderNode* r = def->riders;
    Offset     screen;
    Offset     off;
    LLS*       lls;
    LLS*       mask;

    BoatingSchool_UpdateWater(1);
    screen = GetScreenCoordsForObject(sq, def);
    off = GetRenderOffsetForLayer(g_bs_def->sprite, 3);
    AdjustOffsetForViewMode(&off);
    lls = GetLLSForSprite(GetSpriteForLayer(g_bs_def->sprite, 3));
    mask = GetLLSForSprite(g_bs_hullmask);
    LLSSetFrame(mask, lls->frame);
    PrintSprite(g_bs_hullmask, screen.ox + off.ox, screen.oy + off.oy, mode, 0);
    while (r) {
        if (*(unsigned short*)sq == r->ride_id && r->bloke->action != 2)
            IP_RenderBlokeIn3DNow(r->bloke);
        r = r->next;
    }
    off = GetRenderOffsetForLayer(g_bs_def->sprite, 3);
    off.ox += 0x71;
    off.oy += 0xac;
    AdjustOffsetForViewMode(&off);
    PrintSprite(g_bs_railm, screen.ox + off.ox, screen.oy + off.oy, mode, 0);
}


/* =========================================================================
 * 0x0041b6f0 -- BOATING SCHOOL MERMAID +0x9c  (remove one waving mermaid)
 *
 * The mermaid is the boating school's decoration: a 0x0c-byte record on the
 * list at 0x004d2164 keyed by its own map square (+0x00) and carrying the
 * school that owns it (+0x02).  Removing one does four things:
 *
 *   1. the standard object removal;
 *   2. fades out its looping "Waving Mermaid" sample -- the sound source is
 *      built on the stack as {kind 2, x, y} from the two bytes of the packed
 *      map square, and the sample count is checked first purely to print a
 *      diagnostic ("Can't find samples for mermaid") when it is not exactly
 *      one.  The fade is called either way;
 *   3. repaints the base map under the class footprint, cell by cell, from
 *      the cursor origin;
 *   4. unlinks and frees the record, refunding one unit of takings to the
 *      owning school.
 *
 * NOTE the argument narrowing: the packed square is taken BY VALUE and its
 * two halves are read out of the argument slot, the y half as a dword load
 * one byte in.  The row counter of the repaint loop then lives in the (now
 * dead) `obj` argument slot.
 * ========================================================================= */

typedef struct BsMermaid {
    BPosW             key;          /* +0x00 its own map square */
    BPosW             owner;        /* +0x02 the school it belongs to */
    struct BsMermaid* next;         /* +0x04 */
} BsMermaid;                        /* 0x0c */

/* The sound source PlayInstanceOfSample / CountSamplesFromSource take. */
typedef struct RideSoundSource {
    int kind;                       /* +0x00  2 = "at this map square" */
    int f04;                        /* +0x04  never initialised for kind 2 */
    int x;                          /* +0x08 */
    int y;                          /* +0x0c */
} RideSoundSource;

extern BsMermaid* g_bs_mermaids;                              /* 0x004d2164 */

/* StandardRemoveObject a third time, with the packed square BY VALUE: the
 * mermaid's remove handler takes it that way and forwards it unchanged. */
extern void StandardRemoveObject_B(void* obj, BPosW tile,
                                   EditCursorRec* ctx);       /* 0x0045f220 */
extern int  CountSamplesFromSource(RideSoundSource* src);     /* 0x00496b10 */
extern void UnSourceAndFadeAllSamplesFromSource(RideSoundSource* src,
                                                int fade);    /* 0x00496c80 */
extern void DBPrintf(const char* fmt, ...);                   /* 0x00453a20 */
extern void RestoreBaseMap(int x, int y);                     /* 0x0045da60 */
extern void BoatingSchool_AddTake(BPosW key, int amount);     /* 0x0041b0d0 */

// FUNCTION: LEGOLAND 0x0041b6f0
void BsMermaid_Remove(void* obj, BPosW tile, EditCursorRec* ctx)
{
    RideSoundSource src;
    BsMermaid*      prev = 0;
    BsMermaid*      rec = g_bs_mermaids;
    RideDef*        def = ((RideDef**)obj)[3];
    int             row;
    int             col;

    StandardRemoveObject_B(obj, tile, ctx);
    src.kind = 2;
    src.x = tile.b.x;
    src.y = tile.b.y;
    if (CountSamplesFromSource(&src) != 1)
        DBPrintf("Can't find samples for mermaid\n");
    UnSourceAndFadeAllSamplesFromSource(&src, -400);
    for (row = def->footprint.top; row <= def->footprint.bottom; row++)
        for (col = def->footprint.left; col <= def->footprint.right; col++)
            RestoreBaseMap(col + ctx->x, row + ctx->y);
    while (rec->key.w != tile.w) {
        prev = rec;
        rec = rec->next;
        if (rec == 0)
            return;
    }
    if (rec == 0)
        return;
    BoatingSchool_AddTake(rec->owner, -1);
    if (prev != 0)
        prev->next = rec->next;
    else
        g_bs_mermaids = rec->next;
    HeapFree_w(rec);
}


/* =========================================================================
 * 0x0042d400 -- EARTH SLIDE RIDE +0xb8  (load the ride's save chunk)
 *
 * The chunk is a null-terminated chain: an int "another record follows",
 * then the 0x24-byte SlideRec verbatim, then an int queue length, then that
 * many ints -- each one an INDEX into the class's rider list, which is
 * resolved back to a RiderNode by walking the list that many links (the
 * helper at 0x0042d540).  The queue is rebuilt as a fresh chain of 8-byte
 * {next, rider} nodes, and only the LAST node's `next` is cleared, so an
 * empty queue leaves both the head and the tail pointer null.
 *
 * Every read is checked and any short read abandons the load with 0 -- the
 * four failure sites share one `return 0` block, which is why it sits at the
 * very first check.  Note that a failure part-way through leaves the records
 * already allocated linked into the live list.
 * ========================================================================= */

typedef struct SlideNode {
    struct SlideNode* next;         /* +0x00 */
    void*             rider;        /* +0x04 the RiderNode this seat holds */
} SlideNode;                        /* 0x08 */

typedef struct SlideRec {
    unsigned char     pad00[0x0c];
    struct SlideRec*  next;         /* +0x0c */
    unsigned char     pad10[0x1c - 0x10];
    SlideNode*        queue;        /* +0x1c head of the queue chain */
    SlideNode*        tail;         /* +0x20 its last node, used while loading */
} SlideRec;                         /* 0x24 */

extern SlideRec* g_slide_head;                                /* 0x006160e8 */
extern RideDef*  g_slide_item;                                /* 0x006160d0 */

extern int   SaveGameRead(void* buf, unsigned int n);         /* 0x0047d730 */
extern void* HeapAlloc_w(unsigned int size);                  /* 0x0049e4ff */
/* Walk `n` links down a rider list -- the saved queue holds indices. */
extern void* NthRiderNode(void* list, int n);                 /* 0x0042d540 */

// FUNCTION: LEGOLAND 0x0042d400
int EarthSlide_Load(void)
{
    SlideRec* cur = 0;
    int       more;
    int       n;
    int       idx;

    if (SaveGameRead(&more, 4) == 0)
        return 0;
    while (more != 0) {
        if (cur == 0) {
            cur = (SlideRec*)HeapAlloc_w(0x24);
            g_slide_head = cur;
        } else {
            cur->next = (SlideRec*)HeapAlloc_w(0x24);
            cur = cur->next;
        }
        if (SaveGameRead(cur, 0x24) == 0)
            return 0;
        if (SaveGameRead(&n, 4) == 0)
            return 0;
        cur->queue = 0;
        cur->tail = 0;
        while (n-- != 0) {
            if (cur->tail == 0) {
                cur->tail = (SlideNode*)HeapAlloc_w(8);
                cur->queue = cur->tail;
            } else {
                cur->tail->next = (SlideNode*)HeapAlloc_w(8);
                cur->tail = cur->tail->next;
            }
            if (SaveGameRead(&idx, 4) == 0)
                return 0;
            cur->tail->rider = NthRiderNode(g_slide_item->riders, idx);
        }
        if (cur->tail != 0)
            cur->tail->next = 0;
        if (SaveGameRead(&more, 4) == 0)
            return 0;
    }
    return 1;
}
