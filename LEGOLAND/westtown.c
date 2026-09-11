/* LEGOLAND -- WESTERN TOWN: the shops and civic buildings.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere).
 *
 * ==========================================================================
 * THE NINE CLASSES
 * ==========================================================================
 * WesternTown_GetInterfaces (0x0043a400, LEGOLAND/interfaces.c) installs the
 * callback runs for nine classes:
 *
 *   GENERAL STORE, SHERIFF, JAIL CELL, BANK, SALOON, EXPLORERS INSTITUTE,
 *   LEGO SHOP 1, LEGO SHOP 2, LEGO MEDIA SHOP
 *
 * All nine share one +0xa0 draw-descriptor override (0x0043a390) and six of
 * them share one +0x9c remove handler (0x0043a3d0).
 *
 * THE THREE THAT DO NOT SHARE THE REMOVE HANDLER -- and what state they own:
 *
 *   JAIL CELL       +0x98 0x00437f60 / +0x9c 0x00438020
 *                   Owns a per-placement record list (SaveJailCells /
 *                   LoadJailCells, ridesave.c: 0x1c bytes, next @ +0x00,
 *                   head 0x0062fd3c). Placing a jail cell allocates a record
 *                   keyed by the map square; removing one has to unlink and
 *                   free it before the footprint comes down, which is why it
 *                   cannot use the shared handler.
 *   LEGO SHOP 1     +0x98 0x00439320 / +0x9c 0x00439350
 *   LEGO MEDIA SHOP +0x98 0x00439c60 / +0x9c 0x00439c90
 *                   These two own a SINGLE-INSTANCE placement global rather
 *                   than a list: the add handler records the map square (and
 *                   the count of copies built) and the remove handler clears
 *                   it. They are the "there is only one of these in the park"
 *                   buildings, so their state is a scalar, not a record list,
 *                   and there is no save chunk for it.
 *
 * The other six (GENERAL STORE, SHERIFF, BANK, SALOON, EXPLORERS INSTITUTE,
 * LEGO SHOP 2) keep no placement state at all: everything they need is either
 * on the map object or on the class-wide rider list at ObjDef+0xcc, so taking
 * one off the map is just "unbuild the footprint, evict the customers".
 *
 * ==========================================================================
 * WHAT A WESTERN TOWN SHOP ACTUALLY DOES (recovered here)
 * ==========================================================================
 * A shop is not a ride: it owns no vehicle state and (with one exception) no
 * per-placement record. Everything it needs is the class-wide CUSTOMER LIST
 * at ObjDef+0xcc -- the same RiderNode list rides.c's PutBlokeInList feeds,
 * keyed by the packed {x,y} of the placement the customer walked into.
 *
 *   RiderNode: next @+0x00, prev @+0x04, bloke @+0x08, u16 ride_id @+0x0c
 *
 * Each class then implements exactly two behaviours over that list:
 *
 *  +0xa8  THE PER-TICK CUSTOMER STATE MACHINE (screen.c called it "activate")
 *         Walks the WHOLE class list every frame. For each customer:
 *           - skip it while Bloke+0x0e (the move countdown) is non-zero;
 *           - switch on Bloke+0x60, the ACTION byte, 0..N;
 *           - each case computes the next waypoint inside the building in
 *             24.8 world units from the placement square plus the class's
 *             base offset (ObjDef +0x0c/+0x10) -- e.g. GENERAL STORE case 0
 *             is (x<<8)-0x80, (y<<8)-0x100 -- writes it to Bloke +0x24/+0x28,
 *             and falls into one shared tail:
 *                 d = CalcMoveLine(Bloke->world (+0x68), Bloke->target (+0x24),
 *                                  Bloke->path (+0x98));
 *                 Bloke->+0x0e = 7;                  // busy for 7 ticks
 *                 Bloke->+0x73 = (unsigned char)(d + 0x10);
 *                 NewDirForAction(Bloke, ((d + 0x10) >> 5) + 3);
 *                 Bloke->action++;                   // advance one step
 *           - case 0 also sets Bloke->flags62 |= 8 ("inside a building");
 *           - the LAST case is always the exit: RemoveBlokeFromRide(def,
 *             bloke) and flags62 &= ~8.
 *         So the action byte is a straight-line SCRIPT: a fixed list of
 *         waypoints the customer walks through, one per tick-with-a-free-move,
 *         and the building is done with them at the end of the list.
 *
 *  +0xb0  THE DEPTH-SORTED OVERLAY DRAW (screen.c called it "interact")
 *         Collects the customers standing on THIS square into a 10-entry
 *         stack array and re-renders them grouped by the SAME action byte,
 *         blitting the building's matte sprites between the groups. So the
 *         action code doubles as the occlusion band: the script order is the
 *         back-to-front order inside the shop.
 *
 * Both together mean a shop's "gameplay" is entirely a waypoint script plus a
 * matte order; a browser runtime needs no per-shop state at all except for
 * JAIL CELL (below) and the two self-paving shops.
 */

typedef struct RiderNode RiderNode;

/* The packed map square the draw/tick slots are handed. */
typedef union ShopTile {
    unsigned short key;
    struct { unsigned char x, y; } b;
} ShopTile;


/* ---- the LLIDB element and the 0xd0-byte ObjDef -------------------------- */
typedef struct Spr {
    unsigned char pad00[0x10];
    unsigned int  flags;         /* +0x10  bit 0x2000 = draw via the +0xb0 slot */
} Spr;

typedef struct ShopDef {
    unsigned char pad00[0x0c];
    int           base_x;        /* +0x0c the class's base map square */
    int           base_y;        /* +0x10 */
    int           f14;           /* +0x14 draw-descriptor field 1 */
    int           f18;           /* +0x18 draw-descriptor field 2 */
    unsigned int  flags;         /* +0x1c  0x20 = tick via +0xa8, 0x400 = ask +0xa0 */
    unsigned char pad20[0x3c - 0x20];
    int           fp_x1;         /* +0x3c the class footprint rect, as four ints */
    int           fp_y1;         /* +0x40 */
    int           fp_x2;         /* +0x44 */
    int           fp_y2;         /* +0x48 */
    unsigned char pad4c[0x64 - 0x4c];
    Spr*          sprite;        /* +0x64 build sprite */
    unsigned char pad68[0xcc - 0x68];
    struct RiderNode* riders;    /* +0xcc the class-wide customer list */
} ShopDef;

typedef struct ShopElem {
    char*        name;           /* +0x00 */
    char*        image;          /* +0x04 */
    unsigned int type_flags;     /* +0x08 */
    ShopDef*     data;           /* +0x0c */
} ShopElem;

/* A placed map object: its class record sits at +0x0c. */
typedef struct ShopMapObj {
    unsigned char pad00[0x0c];
    ShopDef*      cls;           /* +0x0c */
} ShopMapObj;

/* The block the +0xa0 slot returns -- the same four fields the render walk
 * builds inline at 0x0045b96b when a class does not override the slot. */
typedef struct ShopDrawDesc {
    Spr*           sprite;       /* +0x00 */
    int            f04;          /* +0x04 */
    int            f08;          /* +0x08 */
    unsigned short f0c;          /* +0x0c */
} ShopDrawDesc;

/* ==========================================================================
 * +0xa0 -- DRAW DESCRIPTOR OVERRIDE (all nine classes)
 * The render walk (0x0045b938) tests ObjDef->flags & 0x400; if it is set it
 * calls this slot and uses the returned block, otherwise it builds the same
 * block on its own stack from ObjDef +0x64/+0x14/+0x18. Identical, register
 * for register, to Joust_GetDrawDesc (0x00408c50) -- one shared static block
 * per subsystem, here g_shop_draw at 0x0082c6a0, and the custom-draw bit
 * (0x2000) armed on the build sprite on the way out. There is exactly ONE
 * such block for the whole of Western Town, so it is only valid until the
 * next class asks for its descriptor.
 * ========================================================================== */

extern ShopDrawDesc g_shop_draw;                             /* 0x0082c6a0 */

// FUNCTION: LEGOLAND 0x0043a390
ShopDrawDesc* Shop_GetDrawDesc(ShopElem* elem, unsigned short arg)
{
    ShopDef* def = elem->data;

    g_shop_draw.sprite = def->sprite;
    g_shop_draw.f04 = def->f14;
    g_shop_draw.f08 = def->f18;
    g_shop_draw.f0c = arg;
    def->sprite->flags |= 0x2000;
    return &g_shop_draw;
}

/* ==========================================================================
 * +0x9c -- TAKE ONE OFF THE MAP (six of the nine)
 * The minimal remove: unbuild the footprint, then evict every customer the
 * class still has parked on that map square. No per-placement state, so no
 * record to free -- contrast Joust_Remove (0x00407ad0), which drops its own
 * record and fades its sample first.
 * ========================================================================== */

#ifndef LEGOLAND_PORTABLE
extern void StandardRemoveObject(void* obj, unsigned int tile, void* ctx);  /* 0x0045f220 */
extern void RemoveAllBlokesFromRide(ShopDef* cls, unsigned int tile);       /* 0x0048a2e0 */
#else
/* PORT-M11: both definitions take the packed map square as a 2-byte aggregate
 * BY VALUE -- objmap2.c:983 `BPos bp` (the shape the ObjClass +0x9c slot is
 * typed with, objmap2.c:99) and rides.c:400 `RideTile tile` -- and this file's
 * own `ShopTile` (line 92) is that same 2-byte union.  On x86 cdecl the
 * aggregate and this `unsigned int` are the same pushed dword, which is why
 * the byte gates never saw it; on wasm32 the aggregate goes INDIRECT, as a
 * pointer to a shadow-stack temp, against the scalar's direct i32 and at the
 * same arity -- so nothing warns and the callee reads an address as a square
 * (PORT-M10 s1b).  LegoShop1_Remove and LegoMedia_Remove already hold a real
 * `ShopTile` and only pun it into a dword for these two calls, so the packing
 * helper round-trips through the same 16 bits. */
extern void StandardRemoveObject(void* obj, ShopTile tile, void* ctx);      /* 0x0045f220 */
extern void RemoveAllBlokesFromRide(ShopDef* cls, ShopTile tile);           /* 0x0048a2e0 */
static __inline ShopTile ll_shoptile(unsigned int v)
{
    ShopTile t;
    t.key = (unsigned short)v;
    return t;
}
#define StandardRemoveObject(_o, _t, _c) \
    StandardRemoveObject((_o), ll_shoptile((unsigned int)(_t)), (_c))
#define RemoveAllBlokesFromRide(_c, _t) \
    RemoveAllBlokesFromRide((_c), ll_shoptile((unsigned int)(_t)))
#endif

#ifdef LEGOLAND_PORTABLE
/* PORT-M11: a +0x9c remove handler.  The slot passes the map square as a
 * 2-byte aggregate BY VALUE (objmap2.c:99, called objmap2.c:1895) -- a POINTER
 * on wasm32 -- and this body reads the dword as an `unsigned int` (and takes
 * its address for the record lookup).  Renamed for the portable build with a
 * twin of the slot's own shape exported over it. */
#define Shop_Remove Shop_Remove_vc6_body
#endif
// FUNCTION: LEGOLAND 0x0043a3d0
void Shop_Remove(void* obj, unsigned int tile, void* ctx)
{
    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(((ShopMapObj*)obj)->cls, tile);
}
#ifdef LEGOLAND_PORTABLE
#undef Shop_Remove
void Shop_Remove(void* obj, ShopTile tile, void* ctx)
{
    Shop_Remove_vc6_body(obj, tile.key, ctx);
}
#endif

/* ==========================================================================
 * WHAT THE SLOT NAMES REALLY MEAN HERE
 * (the same correction ridecb1.c made for the 0x0042xxxx cluster)
 *
 *   +0xa4  LOAD RESOURCES   -- run once when the class is installed. Records
 *                              the ObjDef in a module global, ORs the class
 *                              flags with 0x420 (0x20 = call +0xa8 every
 *                              frame, 0x400 = call +0xa0 for the draw
 *                              descriptor), loads the building's matte
 *                              sprites and the shared money SFX.
 *   +0xac  FREE RESOURCES   -- the mirror: kill the sprites, drop the SFX.
 *   +0x8c  SELECT FOR PLACEMENT -- arm the build cursor with this class.
 *   +0xb0  DEPTH-SORTED OVERLAY DRAW (not "interact")
 *   +0xa8  PER-TICK CUSTOMER STATE MACHINE (not "activate")
 *
 * Every one of the nine classes calls LoadMoneySFX/KillMoneySFX from its
 * +0xa4/+0xac -- these are the buildings that take money off the visitor, so
 * they all share the till sound. LoadMoneySFX is reference-free (money.c
 * counts nothing), so nine classes loading it nine times is the original's
 * behaviour, reproduced.
 * ========================================================================== */

/* A visitor. +0x60 is the ACTION byte the overlay draws sort on: it doubles as
 * the depth band inside a building, so a shop's draw renders the queue in
 * action order and blits its mattes between the groups. */
/* An {x,y} pair in 24.8 world units. CalcMoveLine takes two of them BY VALUE
 * (four dword pushes): that prototype is a codegen lever -- see the note above
 * Explorers_TickCustomers. */
typedef struct Pos { int x; int y; } Pos;

typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short busy;         /* +0x0e  move countdown; non-zero = still walking */
    unsigned char  pad10[0x24 - 0x10];
    Pos            target;       /* +0x24  target world {x,y}, 24.8 */
    unsigned char  pad2c[0x58 - 0x2c];
    int            timer;        /* +0x58  per-state wait counter */
    unsigned char  pad5c[0x60 - 0x5c];
    unsigned char  action;       /* +0x60  the state/occlusion band */
    unsigned char  pad61;
    unsigned short flags62;      /* +0x62  bit 8 = inside a building */
    unsigned char  pad64[0x68 - 0x64];
    Pos            world;        /* +0x68  world {x,y}, 24.8 */
    unsigned char  pad70[0x73 - 0x70];
    unsigned char  f73;          /* +0x73  the move's raw direction byte */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14];   /* +0x98  the move-line scratch block */
} Bloke;

struct RiderNode {
    struct RiderNode* next;      /* +0x00 */
    struct RiderNode* prev;      /* +0x04 */
    Bloke*            bloke;     /* +0x08 */
    union ShopTile    ride_id;   /* +0x0c  the packed {x,y} of the placement */
    unsigned short    pad0e;
    void*             person;    /* +0x10 */
};

/* An {x,y} pair returned in eax:edx. */
typedef struct Offset { int ox; int oy; } Offset;

extern void*  LoadSprite(const char* name, int flag);           /* 0x00497ab0 */
#ifndef LEGOLAND_PORTABLE
extern void   KillSprite(void* sprite);                         /* 0x00497bd0 */
#else
extern int KillSprite(void* sprite);                         /* 0x00497bd0 */
#endif
extern void   LoadMoneySFX(void);                               /* 0x00453900 */
extern void   KillMoneySFX(void);                               /* 0x00453930 */
extern void   DefaultCursor(void* cursor);                      /* 0x0045a390 */
extern void   SetEditCursorFootPrint(void* footprint);          /* 0x0045f440 */
extern void   RenderBlokeIn3D(Bloke* b);                        /* 0x0043ffb0 */
extern void   IP_RenderBlokeIn3DNow(Bloke* b);                  /* 0x00440010 */
extern Offset GetScreenCoordsForObject(ShopTile* sq, ShopDef* def); /* 0x00442cc0 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */

extern int      g_edit_changed;                                 /* 0x008119b0 */
extern ShopDef* g_edit_object;                                  /* 0x008119b8 */
extern char     g_edit_cursor;                                  /* 0x007febc0 */

/* ==========================================================================
 * GENERAL STORE
 * ========================================================================== */

extern ShopDef* g_genstore_def;      /* 0x0081cb30 */
extern void*    g_genstore_matte;    /* 0x0081cb08  "G_Store Matte.LLS"  */
extern void*    g_genstore_matte2;   /* 0x0081cb24  "G_Store Matte2.LLS" */

// FUNCTION: LEGOLAND 0x004375d0
void GeneralStore_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_genstore_def = def;
    def->flags |= 0x420;
    g_genstore_matte = LoadSprite("G_Store Matte.LLS", 1);
    g_genstore_matte2 = LoadSprite("G_Store Matte2.LLS", 1);
    LoadMoneySFX();
}

// FUNCTION: LEGOLAND 0x00437610
void GeneralStore_FreeResources(void)
{
    KillSprite(g_genstore_matte);
    KillSprite(g_genstore_matte2);
    KillMoneySFX();
}

// FUNCTION: LEGOLAND 0x00437630
void GeneralStore_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_genstore_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->fp_x1);
}

/* ==========================================================================
 * SHERIFF
 * ========================================================================== */

extern ShopDef* g_sheriff_def;       /* 0x0081cb14 */
extern void*    g_sheriff_matte;     /* 0x0081cb38  "Sherifshut Matte.LLS" */

// FUNCTION: LEGOLAND 0x00437ba0
void Sheriff_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_sheriff_def = def;
    def->flags |= 0x420;
    g_sheriff_matte = LoadSprite("Sherifshut Matte.LLS", 1);
    LoadMoneySFX();
}

// FUNCTION: LEGOLAND 0x00437bd0
void Sheriff_FreeResources(void)
{
    KillSprite(g_sheriff_matte);
    KillMoneySFX();
}

// FUNCTION: LEGOLAND 0x00437bf0
void Sheriff_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_sheriff_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->fp_x1);
}

/* The simplest overlay draw in the lane: render every customer standing on
 * this square in 3D, and if there was at least one, blit the building's
 * "shutter" matte over them so the sheriff's office occludes them. The NULL
 * blit context is the loop variable, which VC6 knows is zero on exit. */
// FUNCTION: LEGOLAND 0x00437c30
void Sheriff_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq,
                         void* clip, int mode)
{
    ShopDef*   def = elem->data;
    RiderNode* r;
    int        n = 0;
    Offset     o;

    for (r = (RiderNode*)def->riders; r; r = r->next) {
        if (sq->key == r->ride_id.key) {
            RenderBlokeIn3D(r->bloke);
            n++;
        }
    }
    if (n) {
        o = GetScreenCoordsForObject(sq, def);
        PrintSprite(g_sheriff_matte, o.ox, o.oy, mode, 0);
    }
}

/* ==========================================================================
 * BANK
 * ========================================================================== */

extern ShopDef* g_bank_def;          /* 0x0081cb2c */
extern void*    g_bank_matte;        /* 0x0081cb34  "Bank Matte.lls" */

// FUNCTION: LEGOLAND 0x00438870
void Bank_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_bank_def = def;
    def->flags |= 0x420;
    g_bank_matte = LoadSprite("Bank Matte.lls", 1);
    LoadMoneySFX();
}

// FUNCTION: LEGOLAND 0x004388a0
void Bank_FreeResources(void)
{
    KillSprite(g_bank_matte);
    KillMoneySFX();
}

// FUNCTION: LEGOLAND 0x004388c0
void Bank_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_bank_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->fp_x1);
}

// FUNCTION: LEGOLAND 0x00438900
void Bank_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq,
                      void* clip, int mode)
{
    ShopDef*   def = elem->data;
    RiderNode* r;
    int        n = 0;
    Offset     o;

    for (r = def->riders; r; r = r->next) {
        if (sq->key == r->ride_id.key) {
            RenderBlokeIn3D(r->bloke);
            n++;
        }
    }
    if (n) {
        o = GetScreenCoordsForObject(sq, def);
        PrintSprite(g_bank_matte, o.ox, o.oy, mode, 0);
    }
}

/* ==========================================================================
 * SALOON
 * ========================================================================== */

extern ShopDef* g_saloon_def;        /* 0x0081cb1c */
extern void*    g_saloon_matte1;     /* 0x0081cb00  "SaloonMatte1.LLS" */
extern void*    g_saloon_matte2;     /* 0x0081cb04  "SaloonMatte2.LLS" */

// FUNCTION: LEGOLAND 0x00438c60
void Saloon_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_saloon_def = def;
    def->flags |= 0x420;
    g_saloon_matte1 = LoadSprite("SaloonMatte1.LLS", 1);
    g_saloon_matte2 = LoadSprite("SaloonMatte2.LLS", 1);
    LoadMoneySFX();
}

// FUNCTION: LEGOLAND 0x00438ca0
void Saloon_FreeResources(void)
{
    KillSprite(g_saloon_matte1);
    KillSprite(g_saloon_matte2);
    KillMoneySFX();
}

// FUNCTION: LEGOLAND 0x00438cc0
void Saloon_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_saloon_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->fp_x1);
}

/* ==========================================================================
 * EXPLORERS INSTITUTE
 * ========================================================================== */

extern ShopDef* g_explorers_def;     /* 0x0081cb44 */
extern void*    g_explorers_matte;   /* 0x0081cb28  "Explorers Institute Matte.LLS" */

// FUNCTION: LEGOLAND 0x0043a0f0
void Explorers_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_explorers_def = def;
    def->flags |= 0x420;
    g_explorers_matte = LoadSprite("Explorers Institute Matte.LLS", 1);
    LoadMoneySFX();
}

// FUNCTION: LEGOLAND 0x0043a120
void Explorers_FreeResources(void)
{
    KillSprite(g_explorers_matte);
    KillMoneySFX();
}

// FUNCTION: LEGOLAND 0x0043a140
void Explorers_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_explorers_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->fp_x1);
}

// FUNCTION: LEGOLAND 0x0043a180
void Explorers_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq,
                           void* clip, int mode)
{
    ShopDef*   def = elem->data;
    RiderNode* r;
    int        n = 0;
    Offset     o;

    for (r = def->riders; r; r = r->next) {
        if (sq->key == r->ride_id.key) {
            RenderBlokeIn3D(r->bloke);
            n++;
        }
    }
    if (n) {
        o = GetScreenCoordsForObject(sq, def);
        PrintSprite(g_explorers_matte, o.ox, o.oy, mode, 0);
    }
}

/* ==========================================================================
 * THE TWO SELF-PAVING SHOPS: LEGO SHOP 1 and LEGO MEDIA SHOP
 *
 * These are the two classes whose +0x98/+0x9c are their own rather than the
 * shared pair, and the reason is NOT a record list: they PAVE THEIR OWN
 * FOOTPRINT. Placing one lays a path tile on every cell of the class
 * footprint rect (ObjDef +0x3c..+0x48, {x1,y1,x2,y2} relative to the placed
 * position) so the shop comes with its own forecourt; removing one strips
 * those tiles again. So their "placement state" lives in the MAP, not in the
 * module -- which is exactly why neither has a save chunk: LoadBaseMap
 * restores the path tiles with the rest of the map.
 *
 * Two file-private helpers do the work, one per direction; both are shared
 * by the two classes. Neither is in the callback table -- they were found by
 * following the calls out of the add/remove handlers.
 *
 * NOTE the asymmetry, reproduced: paving calls AddPathTileGFX (graphics only,
 * re-reading the loaded path-tile record 0x00832bf0 on EVERY cell) while
 * unpaving calls RemoveRollerCoasterPath, which is the full path removal.
 * ========================================================================== */


extern void  AddPathTileGFX(Pos* pos, unsigned short tile);     /* 0x0045d350 */
extern void  RemoveRollerCoasterPath(Pos* pos);                 /* 0x0045dcd0 */
#ifndef LEGOLAND_PORTABLE
extern void  AddBasicObject(void* elem, Pos* pos);              /* 0x0045efe0 */
#else
extern void AddBasicObject(void* ll_obj, void* ll_pos, void* ll_ctx);              /* 0x0045efe0 */
#define AddBasicObject(_a1, _a2) AddBasicObject((_a1), (_a2), 0)
#endif
extern void* g_path_tile_ptr;   /* 0x00832bf0  loaded path tile record; first word = tile code */

// FUNCTION: LEGOLAND 0x00439230
void Shop_PavePathFootprint(ShopDef* def, Pos* at)
{
    Pos p;
    int x  = at->x + def->fp_x1;
    int x2 = at->x + def->fp_x2;
    int y  = at->y + def->fp_y1;
    int y2 = at->y + def->fp_y2;

    while (x <= x2) {
        while (y <= y2) {
            p.x = x;
            p.y = y;
            AddPathTileGFX(&p, *(unsigned short*)g_path_tile_ptr);
            y++;
        }
        y = at->y + def->fp_y1;
        x++;
    }
}

// FUNCTION: LEGOLAND 0x004392b0
void Shop_UnpavePathFootprint(ShopDef* def, Pos* at)
{
    Pos p;
    int x  = at->x + def->fp_x1;
    int x2 = at->x + def->fp_x2;
    int y  = at->y + def->fp_y1;
    int y2 = at->y + def->fp_y2;

    while (x <= x2) {
        while (y <= y2) {
            p.x = x;
            p.y = y;
            RemoveRollerCoasterPath(&p);
            y++;
        }
        y = at->y + def->fp_y1;
        x++;
    }
}

/* ==========================================================================
 * LEGO SHOP 1
 * ========================================================================== */

extern ShopDef* g_legoshop1_def;     /* 0x0081cb3c */
extern void*    g_legoshop1_matte;   /* 0x0081cb18  "Lego Shop 1 Matte.LLS" */

// FUNCTION: LEGOLAND 0x00439200
void LegoShop1_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_legoshop1_def = def;
    def->flags |= 0x420;
    g_legoshop1_matte = LoadSprite("Lego Shop 1 Matte.LLS", 1);
    LoadMoneySFX();
}

// FUNCTION: LEGOLAND 0x00439320
void LegoShop1_Add(ShopElem* elem, Pos* at)
{
    ShopDef* def = elem->data;

    AddBasicObject(elem, at);
    Shop_PavePathFootprint(def, at);
}

/* The tile arrives BY VALUE as a packed {u8 x, u8 y}; the original reads its
 * y through an UNALIGNED dword load and masks it, which is what a byte field
 * of a by-value packed argument compiles to here. */
// FUNCTION: LEGOLAND 0x00439350
void LegoShop1_Remove(void* obj, ShopTile tile, void* ctx)
{
    ShopDef* def = ((ShopMapObj*)obj)->cls;
    Pos      at;

    StandardRemoveObject(obj, *(unsigned int*)&tile, ctx);
    RemoveAllBlokesFromRide(def, *(unsigned int*)&tile);
    at.x = tile.b.x;
    at.y = tile.b.y;
    Shop_UnpavePathFootprint(def, &at);
}

// FUNCTION: LEGOLAND 0x004393a0
void LegoShop1_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_legoshop1_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->fp_x1);
}

// FUNCTION: LEGOLAND 0x00439400
void LegoShop1_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq,
                           void* clip, int mode)
{
    ShopDef*   def = elem->data;
    RiderNode* r;
    int        n = 0;
    Offset     o;

    for (r = def->riders; r; r = r->next) {
        if (sq->key == r->ride_id.key) {
            RenderBlokeIn3D(r->bloke);
            n++;
        }
    }
    if (n) {
        o = GetScreenCoordsForObject(sq, def);
        PrintSprite(g_legoshop1_matte, o.ox, o.oy, mode, 0);
    }
}

/* ==========================================================================
 * LEGO SHOP 2
 * ========================================================================== */

extern ShopDef* g_legoshop2_def;     /* 0x0081cb4c */
extern void*    g_legoshop2_matte;   /* 0x0081cb20  "Lego Shop 2 Matte.LLS" */

// FUNCTION: LEGOLAND 0x004396d0
void LegoShop2_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_legoshop2_def = def;
    def->flags |= 0x420;
    g_legoshop2_matte = LoadSprite("Lego Shop 2 Matte.LLS", 1);
    LoadMoneySFX();
}

/* The only one of the nine that null-checks its sprite before killing it. */
// FUNCTION: LEGOLAND 0x00439700
void LegoShop2_FreeResources(void)
{
    if (g_legoshop2_matte)
        KillSprite(g_legoshop2_matte);
    KillMoneySFX();
}

// FUNCTION: LEGOLAND 0x00439720
void LegoShop2_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_legoshop2_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->fp_x1);
}

/* ==========================================================================
 * LEGO MEDIA SHOP
 * ========================================================================== */

extern ShopDef* g_legomedia_def;     /* 0x0081cb40 */
extern void*    g_legomedia_mask1;   /* 0x0081cb48  "LegMediaShopMask1.LLS" */
extern void*    g_legomedia_mask2;   /* 0x0081cb50  "LegMediaShopMask2.LLS" */

// FUNCTION: LEGOLAND 0x00439c20
void LegoMedia_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_legomedia_def = def;
    def->flags |= 0x420;
    g_legomedia_mask1 = LoadSprite("LegMediaShopMask1.LLS", 1);
    g_legomedia_mask2 = LoadSprite("LegMediaShopMask2.LLS", 1);
    LoadMoneySFX();
}

// FUNCTION: LEGOLAND 0x00439c60
void LegoMedia_Add(ShopElem* elem, Pos* at)
{
    ShopDef* def = elem->data;

    AddBasicObject(elem, at);
    Shop_PavePathFootprint(def, at);
}

// FUNCTION: LEGOLAND 0x00439c90
void LegoMedia_Remove(void* obj, ShopTile tile, void* ctx)
{
    ShopDef* def = ((ShopMapObj*)obj)->cls;
    Pos      at;

    StandardRemoveObject(obj, *(unsigned int*)&tile, ctx);
    RemoveAllBlokesFromRide(def, *(unsigned int*)&tile);
    at.x = tile.b.x;
    at.y = tile.b.y;
    Shop_UnpavePathFootprint(def, &at);
}

// FUNCTION: LEGOLAND 0x00439ce0
void LegoMedia_FreeResources(void)
{
    KillSprite(g_legomedia_mask1);
    KillSprite(g_legomedia_mask2);
    KillMoneySFX();
}

// FUNCTION: LEGOLAND 0x00439d00
void LegoMedia_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_legomedia_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->fp_x1);
}

/* ==========================================================================
 * JAIL CELL -- the one Western Town class with a per-placement record list
 *
 * The 0x1c-byte record ridesave.c's SaveJailCells/LoadJailCells writes raw:
 *
 *     +0x00  next            (re-linked on load)
 *     +0x04  u16 key         the packed {x,y} map square this cell sits on
 *     +0x06  u8  frame       the cell's layer-1 LLS animation frame; set to 9
 *                        when the cell is built and fed straight to LLSSetFrame
 *                        by JailCell_DrawOverlay (0x00438150), so it is what
 *                        makes a jail cell look occupied. It IS saved.
 *     +0x08 .. +0x18  five ints, all zeroed at create
 *
 * The record is allocated by the +0x98 add handler and freed by the +0x9c
 * remove handler, which is why JAIL CELL cannot use the shared Shop_Remove.
 * The four list primitives below (add / find-by-square / unlink+free /
 * free-all) are not in the callback table -- they were recovered by following
 * the calls out of the four slots that are.
 * ========================================================================== */

typedef struct JailCellRec {
    struct JailCellRec* next;    /* +0x00 */
    ShopTile            tile;    /* +0x04 the map square this copy sits on */
    unsigned char       frame;   /* +0x06 the cell's layer-1 animation frame */
    unsigned char       pad07;
    int                 f08;     /* +0x08 */
    int                 f0c;     /* +0x0c */
    int                 f10;     /* +0x10 */
    int                 f14;     /* +0x14 */
    int                 f18;     /* +0x18 */
} JailCellRec;

extern JailCellRec* g_jailcells_head;   /* 0x0062fd3c (ridesave.c) */
extern Spr*         g_jail_sprite;      /* 0x0062fd40 */
extern ShopDef*     g_jail_def;         /* 0x0081cb10 */
extern void*        g_jail_matte;       /* 0x0081cb0c  "JailCellMask.LLS" */

extern void* HeapAlloc_w(unsigned int size);                    /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                               /* 0x0049e4d0 */
extern void  HideLayer(Spr* sprite, int layer);                 /* 0x00497de0 */
extern void  StopLayerPlaying(Spr* sprite, int layer);          /* 0x00441f00 */
extern void* GetLLSForLayer(Spr* sprite, int layer);            /* 0x00441ea0 */
extern void  LLSSetFrame(void* lls, int frame);                 /* 0x0047d5a0 */
void* memset(void*, int, unsigned int);
/* CRT; the 2-byte memcmp is expanded inline by /O2 (see JailCell_FindRecord). */
extern int memcmp(const void* a, const void* b, unsigned int n);   /* CRT intrinsic */

/* Same "rep stosd is not a kill" pattern as Joust_AddRecord: the record is
 * memset whole and then the five tail ints are stored again from the
 * function-wide zero register. */
// FUNCTION: LEGOLAND 0x00437f10
void JailCell_AddRecord(ShopTile* tile)
{
    JailCellRec* rec = (JailCellRec*)HeapAlloc_w(sizeof(JailCellRec));

    if (rec != 0) {
        memset(rec, 0, sizeof(JailCellRec));
        rec->tile.key = tile->key;
        rec->frame = 9;
        rec->f08 = 0;
        rec->f0c = 0;
        rec->f10 = 0;
        rec->f14 = 0;
        rec->f18 = 0;
        rec->next = g_jailcells_head;
        g_jailcells_head = rec;
    }
}

/* CLOSED (2026-09-03) by the intrinsic-memcmp form the ridecb3 lane found for
 * Carousel_FindRec / Balloonz_FindRec: `memcmp(&rec->tile, tile, 2)`. VC6
 * expands the 2-byte memcmp late (after loop-invariant hoisting), which is
 * why the CALLER's key is re-read as the compare's memory operand on every
 * iteration, and the intrinsic materialises the first operand's ADDRESS
 * (`lea edx,[rec+4]`) before the folded word load -- the "dead lea" that
 * the earlier both-volatile SameTile helper (16 of 18) could not reproduce.
 * A key at offset 0 (Joust/Catapult) shows no lea because the address is the
 * record pointer itself. No volatile needed. */
// FUNCTION: LEGOLAND 0x00437f90
JailCellRec* JailCell_FindRecord(ShopTile* tile)
{
    JailCellRec* rec = g_jailcells_head;

    if (rec != 0) {
        while (memcmp(&rec->tile, tile, 2) != 0) {
            rec = rec->next;
            if (rec == 0)
                return 0;
        }
        return rec;
    }
    return 0;
}

/* Unlink one record and free it. The head case is tail-duplicated (it gets
 * its own free + ret); the `if (p)` after the walk is the original's
 * redundant re-test, reachable only on the break path.
 *
 * THE LEVER (this function sat at 27 instructions against 25 for a long
 * time): the loop compares the link and then FOLLOWS it, and VC6 CSEs the
 * two reads into one -- 'mov edx,[eax] / cmp edx,ecx' where the original has
 * 'cmp dword ptr [eax],ecx' and a separate 'mov eax,[eax]'. Marking the read
 * that FOLLOWS the link volatile (not the one that compares it) splits them:
 * the compare keeps its memory operand and the walk becomes its own load.
 * Volatile on the COMPARE instead goes the wrong way -- it forces the load
 * into a register and leaves the CSE'd walk. */
// FUNCTION: LEGOLAND 0x00437fc0
void JailCell_RemoveRecord(JailCellRec* rec)
{
    JailCellRec* p = g_jailcells_head;

    if (p == rec) {
        g_jailcells_head = rec->next;
    } else {
        while (p->next != rec) {
            p = ((volatile JailCellRec*)p)->next;
            if (p == 0)
                break;
        }
        if (p)
            p->next = rec->next;
    }
    HeapFree_w(rec);
}

// FUNCTION: LEGOLAND 0x00438000
void JailCell_FreeAllRecords(void)
{
    while (g_jailcells_head)
        JailCell_RemoveRecord(g_jailcells_head);
}

/* +0x98 -- place a jail cell. The two-byte key is built from the placement
 * position BEFORE the object goes down and lives in the (now dead) `at`
 * parameter's home slot. */
// FUNCTION: LEGOLAND 0x00437f60
void JailCell_Add(ShopElem* elem, Pos* at)
{
    ShopTile tile;

    tile.b.x = (unsigned char)at->x;
    tile.b.y = (unsigned char)at->y;
    AddBasicObject(elem, at);
    JailCell_AddRecord(&tile);
}

/* +0x9c -- drop this cell's record first, then the shared two steps. */
#ifdef LEGOLAND_PORTABLE
/* PORT-M11: a +0x9c remove handler.  The slot passes the map square as a
 * 2-byte aggregate BY VALUE (objmap2.c:99, called objmap2.c:1895) -- a POINTER
 * on wasm32 -- and this body reads the dword as an `unsigned int` (and takes
 * its address for the record lookup).  Renamed for the portable build with a
 * twin of the slot's own shape exported over it. */
#define JailCell_Remove JailCell_Remove_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00438020
void JailCell_Remove(void* obj, unsigned int tile, void* ctx)
{
    JailCellRec* rec = JailCell_FindRecord((ShopTile*)&tile);

    if (rec)
        JailCell_RemoveRecord(rec);
    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(((ShopMapObj*)obj)->cls, tile);
}
#ifdef LEGOLAND_PORTABLE
#undef JailCell_Remove
void JailCell_Remove(void* obj, ShopTile tile, void* ctx)
{
    JailCell_Remove_vc6_body(obj, tile.key, ctx);
}
#endif

/* +0xa4. NOTE the quirk, reproduced: every other class in the lane ORs
 * 0x2000 into the BUILD SPRITE's flags (Spr +0x10) to arm the custom draw;
 * JAIL CELL ORs it into the ObjDef's own flags word (+0x1c) instead, on top
 * of the 0x420 it has just set there. The sprite never gets the bit. */
// FUNCTION: LEGOLAND 0x00438070
void JailCell_LoadResources(ShopElem* elem)
{
    ShopDef* def = elem->data;

    g_jail_def = def;
    def->flags |= 0x420;
    g_jail_sprite = g_jail_def->sprite;
    g_jail_def->flags |= 0x2000;
    g_jail_matte = LoadSprite("JailCellMask.LLS", 1);
    HideLayer(g_jail_sprite, 1);
    StopLayerPlaying(g_jail_sprite, 1);
    LLSSetFrame(GetLLSForLayer(g_jail_sprite, 1), 9);
}

// FUNCTION: LEGOLAND 0x004380f0
void JailCell_FreeResources(void)
{
    KillSprite(g_jail_matte);
    JailCell_FreeAllRecords();
}

// FUNCTION: LEGOLAND 0x00438110
void JailCell_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_jail_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->fp_x1);
}

/* ==========================================================================
 * THE BANDED OVERLAY DRAWS (+0xb0) -- GENERAL STORE, SALOON, LEGO SHOP 2,
 * JAIL CELL, LEGO MEDIA SHOP
 *
 * The four small shops (SHERIFF, BANK, EXPLORERS INSTITUTE, LEGO SHOP 1)
 * just render everyone and blit one matte. The five below are the buildings
 * whose geometry occludes the customers, so they sort by the customer's
 * ACTION byte (Bloke +0x60) and blit their mattes BETWEEN the groups: the
 * action code doubles as the depth band.
 *
 * The shape is always the same:
 *   1. collect, into a 10-entry stack array, every rider on this map square
 *      (the count is a signed CHAR -- so a building with more than 127
 *      customers on one square would corrupt the frame; the array itself
 *      overflows at 10, which is the original's real limit);
 *   2. for each band in the class's own order, render every collected
 *      customer whose action equals that band;
 *   3. blit a matte where the group changes.
 * ========================================================================== */

/* Draw every collected customer whose action byte is `band`. Inlined at every
 * call site; the count is re-sign-extended per band, which is why the
 * original re-tests and re-`movsx`es the char for every band.
 *
 * THE SPELLING IS THE LEVER, and it took two rounds to find:
 *  - `for (i = 0; i < n; i++)` with an INT index promotes `n` in the IR, so
 *    the sign-extension is one CSE-able expression: VC6 hoists ONE `movsx`
 *    for the whole function, spills it, and re-registers everything
 *    downstream (that spelling cost GENERAL STORE 196 mismatches).
 *  - `if (n > 0) { k = n; do { .. } while (--k); }` walking the parameter
 *    keeps the per-band `movsx edi,bl`, but only because the queue-address
 *    `lea` lands in the guard block, BEFORE the `jle`; the original has it
 *    after (27/22/16 mismatches, one lea/jle transposition per band).
 *  - CLOSED (2026-09-03) by a CHAR index: `char i; for (i = 0; i < n; i++)
 *    list[i]`. The compare `i < n` is then a byte compare with no widening
 *    in the IR, so the dword trip count `movsx edi,bl` is created late by
 *    the loop optimiser, per loop, and is never CSE'd; the guard is the
 *    inverted loop test (`test bl,bl / jle`), the preheader is `lea esi,
 *    queue / movsx edi,bl` in that order, and the char index itself dies
 *    into the strength-reduced cursor. Exact on all three banded draws in
 *    this file; the same change should close westtown2.c's two, ridecb1.c's
 *    Carousel_Draw/Balloonz_Draw and mechrides.c's residual (b). `short`,
 *    `unsigned char`, `i != n`, `while (i < n)` and a `Bloke* b` temp were
 *    also measured: only `char` with `<` (or the equivalent while) is exact. */
static __inline void ShopDrawBand(Bloke** list, char n, int band)
{
    char i;

    for (i = 0; i < n; i++)
        if (list[i]->action == band)
            IP_RenderBlokeIn3DNow(list[i]);
}

/* GENERAL STORE: actions 4,5,6 stand behind the shelves (Matte2), everything
 * else in front of them and behind the shop front (Matte). */
/* All three banded draws below were CLOSED on 2026-09-03 by the char loop
 * index in ShopDrawBand (see the note on it). Everything else -- frame
 * layout, the 10-entry queue's {0} init (one explicit store plus 'rep stosd'
 * for the other nine), the signed-char count, the collect loop, the band
 * order, the sprites and the PrintSprite calls -- was already exact with the
 * declarations ordered def, n, r, queue, o. */
// FUNCTION: LEGOLAND 0x00437670
void GeneralStore_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq,
                              void* clip, int mode)
{
    ShopDef*   def = elem->data;
    char       n = 0;
    RiderNode* r = def->riders;
    Bloke*     queue[10] = { 0 };
    Offset     o;

    while (r) {
        if (sq->key == r->ride_id.key)
            queue[n++] = r->bloke;
        r = r->next;
    }
    if (n) {
        o = GetScreenCoordsForObject(sq, def);
        ShopDrawBand(queue, n, 4);
        ShopDrawBand(queue, n, 5);
        ShopDrawBand(queue, n, 6);
        PrintSprite(g_genstore_matte2, o.ox, o.oy, mode, 0);
        ShopDrawBand(queue, n, 0);
        ShopDrawBand(queue, n, 1);
        ShopDrawBand(queue, n, 2);
        ShopDrawBand(queue, n, 3);
        ShopDrawBand(queue, n, 7);
        ShopDrawBand(queue, n, 8);
        ShopDrawBand(queue, n, 9);
        ShopDrawBand(queue, n, 10);
        ShopDrawBand(queue, n, 11);
        PrintSprite(g_genstore_matte, o.ox, o.oy, mode, 0);
    }
}

/* SALOON: actions 4,5,6 are at the bar (behind SaloonMatte2), 2,3,7,8 in the
 * middle of the room (behind SaloonMatte1), 0,1,9 in front of everything. */
// FUNCTION: LEGOLAND 0x00438d00
void Saloon_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq,
                        void* clip, int mode)
{
    ShopDef*   def = elem->data;
    char       n = 0;
    RiderNode* r = def->riders;
    Bloke*     queue[10] = { 0 };
    Offset     o;

    while (r) {
        if (sq->key == r->ride_id.key)
            queue[n++] = r->bloke;
        r = r->next;
    }
    if (n) {
        o = GetScreenCoordsForObject(sq, def);
        ShopDrawBand(queue, n, 4);
        ShopDrawBand(queue, n, 5);
        ShopDrawBand(queue, n, 6);
        PrintSprite(g_saloon_matte2, o.ox, o.oy, mode, 0);
        ShopDrawBand(queue, n, 2);
        ShopDrawBand(queue, n, 3);
        ShopDrawBand(queue, n, 7);
        ShopDrawBand(queue, n, 8);
        PrintSprite(g_saloon_matte1, o.ox, o.oy, mode, 0);
        ShopDrawBand(queue, n, 0);
        ShopDrawBand(queue, n, 1);
        ShopDrawBand(queue, n, 9);
    }
}

/* LEGO MEDIA SHOP: actions 2..5 are inside (behind Mask2), 0,1,6 in front
 * of the shelving (behind Mask1). */
// FUNCTION: LEGOLAND 0x00439d40
void LegoMedia_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq,
                           void* clip, int mode)
{
    ShopDef*   def = elem->data;
    char       n = 0;
    RiderNode* r = def->riders;
    Bloke*     queue[10] = { 0 };
    Offset     o;

    while (r) {
        if (sq->key == r->ride_id.key)
            queue[n++] = r->bloke;
        r = r->next;
    }
    if (n) {
        o = GetScreenCoordsForObject(sq, def);
        ShopDrawBand(queue, n, 2);
        ShopDrawBand(queue, n, 3);
        ShopDrawBand(queue, n, 4);
        ShopDrawBand(queue, n, 5);
        PrintSprite(g_legomedia_mask2, o.ox, o.oy, mode, 0);
        ShopDrawBand(queue, n, 0);
        ShopDrawBand(queue, n, 1);
        ShopDrawBand(queue, n, 6);
        PrintSprite(g_legomedia_mask1, o.ox, o.oy, mode, 0);
    }
}

/* ==========================================================================
 * THE REST OF WESTERN TOWN IS IN LEGOLAND/westtown2.c
 *
 * The eight other per-tick customer state machines (+0xa8), the two overlay
 * draws that go through the layer path (+0xb0) and the shared "browse, then
 * maybe buy" step they all call now live in westtown2.c. Read that file's
 * header for the script of each class; the short version is that every shop
 * is a fixed list of waypoints plus, in five of the nine, one or two random
 * rolls:
 *
 *   0x004378e0  GENERAL STORE       12 states; a coin toss at state 3 lets
 *                                   half the customers skip the queue
 *   0x00437c90  SHERIFF              8 states; rand()%3 picks a spot along
 *                                   the counter, dir forced to 8
 *   0x00438430  JAIL CELL           11 states, and it drives the cell door
 *                                   animation for EVERY jail cell in a
 *                                   second loop -- this is what the six
 *                                   ints in the saved record are for
 *   0x00438960  BANK                 9 states; states 3/4 ping-pong
 *   0x00438f10  SALOON              10 states; a coin toss picks the end of
 *                                   the bar, dir forced to 8
 *   0x00439460  LEGO SHOP 1          7 states; the browsing spot is jittered
 *                                   on both axes and the shopper spins
 *   0x00439950  LEGO SHOP 2         13 states, two of them dead
 *   0x00439ef0  LEGO MEDIA SHOP      7 states; state 3 TELEPORTS
 *   0x0043a1e0  EXPLORERS INSTITUTE  6 states -- the shortest, below
 *
 *   0x00437570  Shop_BrowseAndBuy   the shared counter step (westtown2.c)
 *   0x00438150  JailCell_DrawOverlay / 0x00439760 LegoShop2_DrawOverlay
 * ========================================================================== */

/* ==========================================================================
 * EXPLORERS INSTITUTE -- the shortest customer script in the lane (6 states)
 *
 * Its waypoints come straight from the RiderNode key bytes rather than from
 * the class base offsets, except state 4 (the way out), which uses the class
 * base square plus half a tile.
 *
 *   0  step to (key.x, key.y + 1)  and mark the customer "inside" (flags 8)
 *   1  step to (key.x, key.y + 2)  then sit still for rand()%70 + 20 ticks
 *   2  tick that counter down; advance when it reaches zero (note: the
 *      decrement happens on the same tick as the advance, so the counter is
 *      left at -1 -- reproduced)
 *   3  step back to (key.x, key.y + 1)
 *   4  step to the class base square, centred (+0x80 on both axes)
 *   5  leave: RemoveBlokeFromRide and clear the "inside" flag
 * ========================================================================== */

extern int  CalcMoveLine(Pos from, Pos to, void* path);              /* 0x00480740 */
extern int  NewDirForAction(Bloke* b, unsigned char dir);              /* 0x004833d0 */
extern void RemoveBlokeFromRide(ShopDef* def, RiderNode* r);           /* 0x0048a100 */
extern int  rand(void);                                                /* 0x0049e4b2 (CRT) */

/* The tail every walking state shares: run the move line to the target the
 * state just wrote, mark the customer busy for 7 ticks, stash the raw
 * direction byte, turn to face it, and advance to the next state. */
static __inline void ShopStepToTarget(Bloke* b)
{
    unsigned char d;

    d = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
    b->busy = 7;
    b->f73 = d;
    NewDirForAction(b, (unsigned char)((d >> 5) + 3));
    b->action++;
}

/* CLOSED 2026-09-03 (was 144/144 with 118 mismatches and an escaping branch)
 * by two levers, both borrowed from the exact scripts in westtown2.c:
 *
 *  1. CalcMoveLine takes `Pos` BY VALUE (world, target, path) -- the same
 *     prototype westtown2.c declares. With four int arguments VC6 loaded
 *     b->world.y EARLY, before the `inc/shl` of the target y, which forced
 *     the y value into a fresh register (`mov eax,ecx / inc eax`) in every
 *     case and made case 0 and case 3 cross-jump into one push block; with
 *     the struct arguments each pair is loaded as a unit between the pushes
 *     and y is computed in place (`inc ecx / shl ecx,8`), so case 3 shares
 *     case 4's "lea path / store y / push five" block and case 0 (whose x/y
 *     land in ecx/edx rather than edx/ecx) keeps its own and jumps to the
 *     shared `call`. 118 -> 115, but the layout is then the original's.
 *  2. The placement square is read through `ShopTile* key = &r->ride_id`
 *     (westtown2.c's `key = &r->square`) rather than `r->ride_id.b.x`.
 *     The generated loads are the same `[eax+0xc]`/`[eax+0xd]`, but the
 *     sum `def->base_x + key->b.x` in case 4 then takes base_x's register
 *     (`add edx,ecx`) where the direct field read makes the zero-extended
 *     byte the destination (`add ecx,edx`); that one coalescing choice flips
 *     x/y into edx/ecx and lets case 3 merge into case 4's tail. 115 -> 0.
 *     Declaration order of def/r/next/b/key is irrelevant (28 permutations).
 *
 * Measured inert for the case-4 sum before the key pointer was tried: both
 * operand orders on both axes, `long`/`unsigned`/`short` base fields and
 * casts, `int`/`unsigned`/`unsigned char` locals for the key bytes (int
 * ones are folded away; `unsigned char` ones spill through a byte slot;
 * `char` ones sign-extend but come within 9), `volatile` on either operand,
 * a `ShopTileCentre(base, k)` inline in both parameter orders and both
 * parameter types, and a `ShopStep(b, tx, ty)` inline (that one hoists the
 * loop-invariant base reads into callee-saved registers and moves the
 * prologue). */
// FUNCTION: LEGOLAND 0x0043a1e0
void Explorers_TickCustomers(ShopElem* elem)
{
    ShopDef*   def = elem->data;
    RiderNode* r = def->riders;
    RiderNode* next;
    Bloke*     b;
    ShopTile*  key;

    while (r) {
        b = r->bloke;
        next = r->next;
        key = &r->ride_id;
        if (b->busy == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x = key->b.x << 8;
                b->target.y = (key->b.y + 1) << 8;
                ShopStepToTarget(b);
                break;
            case 1:
                b->target.x = key->b.x << 8;
                b->target.y = (key->b.y + 2) << 8;
                ShopStepToTarget(b);
                b->timer = rand() % 70 + 20;
                break;
            case 2:
                if (b->timer == 0)
                    b->action++;
                b->timer--;
                break;
            case 3:
                b->target.x = key->b.x << 8;
                b->target.y = (key->b.y + 1) << 8;
                ShopStepToTarget(b);
                break;
            case 4:
                b->target.x = ((def->base_x + key->b.x) << 8) + 0x80;
                b->target.y = ((def->base_y + key->b.y) << 8) + 0x80;
                ShopStepToTarget(b);
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
