/* LEGOLAND -- the SIX MECHANICAL RIDES: SAFARI RIDE, SPIDER RIDE, PLANE RIDE,
 * SPINNING BARRELS RIDE, SPACE TOWER RIDE and COPTERS.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere). The callback-slot tables that install everything below are the
 * six *_GetInterfaces providers in interfaces.c; the per-ride save records are
 * in ridesave.c and the shared boarding machinery in rides.c.
 *
 * ==========================================================================
 * WHAT A MECHANICAL RIDE IS
 * ==========================================================================
 * All six are the same machine: ONE ObjDef (the class), one file-static block
 * of globals, and one singly linked list of per-placement records keyed by the
 * packed {x,y} map square the copy was built on. Every copy of the ride runs a
 * small fixed-length CYCLE (load -> run -> unload), and each of its riders
 * runs a per-bloke state machine kept in the bloke's +0x60 byte. The riders
 * are NOT owned by the ride: they live on the class-wide rider list at
 * ObjDef+0xcc (rides.c), each carrying the same packed map square at +0x0c, so
 * "the riders of this copy" is a filter over the class list.
 *
 * The slot map (see docs/RIDE_CALLBACKS.md) and what each one does here:
 *
 *   +0x8c SELECT     arm the build cursor with this class (identical in all 6)
 *   +0x98 ADD        place a copy: build the footprint, open a record
 *   +0x9c REMOVE     drop the record, unbuild, evict the riders
 *   +0xa0 DRAW DESC  return the class's draw descriptor (4 of the 6)
 *   +0xa4 CREATE     load the ride's sprites / .bnv / FX, arm the flag bits
 *   +0xa8 ACTIVATE   THE PER-TICK UPDATE -- advance every rider's state
 *                    machine one step, and (barrels/tower/plane/copters) the
 *                    machine's own cycle
 *   +0xac DESTROY    free everything +0xa4 loaded
 *   +0xb0 INTERACT   THE CUSTOM DRAW -- draw the ride and its riders
 *   +0xb8/+0xbc      load/save the record list (ridesave.c)
 *
 * ACTIVATE vs INTERACT (the distinction a browser runtime needs):
 *   ACTIVATE (+0xa8) is SIMULATION. The frame loop walks the ObjDef chain
 *   (0x0045b5dc) and calls it once per frame for every class whose ObjDef
 *   flags carry 0x20. It never touches the screen: it advances the vehicle's
 *   cycle counter, moves riders between "queueing", "boarding", "riding" and
 *   "leaving", starts and stops samples, and calls the walk planner
 *   (CalcMoveLine) to give a bloke its next destination. It is frame-rate
 *   driven, not time driven, and one call = one step for every rider of every
 *   copy of the class.
 *   INTERACT (+0xb0) is RENDERING. The sprite-draw path (0x00485ac3) calls it
 *   instead of blitting the class sprite whenever the build sprite carries
 *   flag 0x2000, handing it (elem, screen x, screen y, &map square, clip,
 *   mode). It reads the same records the update wrote and paints: the blokes
 *   who are merely queueing are drawn straight away underneath, the ride's
 *   own sprite (or a "matte") goes over them, and the blokes who are ON the
 *   ride are positioned by the ride itself and pushed onto a private
 *   depth-sorted list that is flushed last, so the vehicle occludes them
 *   correctly. INTERACT never changes simulation state.
 *
 * ==========================================================================
 * THE SIX RIDE-LOCAL GLOBAL BLOCKS
 * ==========================================================================
 *                    SAFARI      SPIDER      SBARREL     TOWER       PLANE       COPTERS
 *   def (ObjDef)     0x4cbec4    0x4cbf20    0x62fde4    0x62fd74    0x62fe58    0x4c1198
 *   draw desc        0x4cbed0    0x4cbf40    0x62fdb0    0x62fd48    0x62fe60    0x4c1170
 *   sprite (layers)  0x4cbec8    0x4cbf28    0x62fde0    0x62fd60    0x62fe7c    0x4c1138
 *   record list head 0x4cbf0c    0x4cbf58    0x62fe08    0x62fda8    0x62fe9c    0x4c11b4
 *   record size      0x28        0x30        0x34        0xb4        0x24        0xd8
 *   record next      +0x10       +0x2c       +0x00       +0x08       +0x20       +0x04
 *   record key       +0x00       +0x00       +0x04       +0x00       +0x00       +0x00
 *   FX list          0x4b4cb8/1  0x4b4d88/1  -           0x4b7618/1  0x4b79d0/2  0x4b4140/4
 *
 * ==========================================================================
 * WHAT A MECHANICAL RIDE'S RECORD HOLDS (recovered from the four updates)
 * ==========================================================================
 * Every one of the six per-placement records is the same handful of counters
 * under different offsets, and the pattern is worth stating once:
 *   - the packed {x,y} map SQUARE the copy was built on (the list key);
 *   - SEATED, how many riders have sat down, compared against the class
 *     capacity at ObjDef+0x2e to decide when the vehicle may leave;
 *   - RIDERS, how many blokes are on the ride at all, decremented as they
 *     leave; when it hits zero the ride resets and lets people on again
 *     (Ride_ClearFlagToNotLetAnyoneOn);
 *   - JOINED, bumped by every rider that claims a place;
 *   - a TIMER the join step arms (180 frames for the Safari, Spider, Plane and
 *     Space Tower, 400 for the Barrels) -- the machine's dwell time;
 *   - one BYTE PER SEAT, 0 when free; and
 *   - the vehicle's own animation FRAME(s), which the draw pushes into the
 *     sprite layers and into the ride's z-sprite.
 * Two rides overlay the seat bytes on a neighbouring field: the Spider's and
 * the Plane's seat 0 is the top byte of the 32-bit timer, and the Barrels'
 * seat 0 is the layer-2 animation frame. Harmless in practice, faithfully
 * reproduced, and a trap for any runtime that lays these records out afresh.
 *
 * ==========================================================================
 * THE TWO "SAMPLE TABLES" IN THE SAVE FORMAT ARE BNV PATH SETS
 * ==========================================================================
 * ridesave.c's loaders re-resolve, for every live instance, a "sample" from a
 * per-ride table by 1-based index. The create handlers here show what those
 * tables actually hold: three .bnv DEPTH-PATH sets (the ride's "run", "on" and
 * "off" motion curves) followed by the rider z-sprite, e.g. for the Safari
 *     0x4cbef8 = Zbuffers\Safarirun.bnv
 *     0x4cbefc = Zbuffers\Safarion.bnv
 *     0x4cbf00 = Zbuffers\Safarioff.bnv
 *     0x4cbf08 = z_Safari.lls
 * and the same shape for the Spider (0x4cbf30..) and the Barrels (0x62fdf0..).
 * So a mechanical ride's saved "sample index" selects which motion curve a
 * rider is on, not which wav is playing.
 * ==========================================================================
 */

/* ---- the LLIDB element and the 0xd0-byte ObjDef -------------------------- */
typedef struct Spr {
    unsigned char pad00[0x10];
    unsigned int  flags;         /* +0x10  bit 0x2000 = draw via the +0xb0 slot */
} Spr;

typedef struct RideDef {
    unsigned char pad00[0x0c];
    int           base_x;        /* +0x0c the class's base map square */
    int           base_y;        /* +0x10 */
    int           f14;           /* +0x14 draw-descriptor field 1 */
    int           f18;           /* +0x18 draw-descriptor field 2 */
    unsigned int  flags;         /* +0x1c 0x20 arms +0xa8, 0x400 arms +0xa0 */
    unsigned char pad20[4];
    signed char   qx;            /* +0x24 queue/exit offset from the base square */
    signed char   qy;            /* +0x25 */
    unsigned char pad26[8];
    short         capacity;      /* +0x2e how many riders one copy takes */
    unsigned char pad30[0x3c - 0x30];
    int           footprint;     /* +0x3c the class footprint rect; its first
                                  *       two ints double as the boarding spot */
    int           f40;           /* +0x40 */
    unsigned char pad44[0x64 - 0x44];
    Spr*          sprite;        /* +0x64 build-anim sprite */
    unsigned char pad68[0xcc - 0x68];
    struct RiderNode* riders;    /* +0xcc the class-wide rider list */
} RideDef;

typedef struct RideElem {
    char*    name;               /* +0x00 */
    char*    image;              /* +0x04 */
    unsigned int type_flags;     /* +0x08 */
    RideDef* data;               /* +0x0c */
} RideElem;

/* The block the +0xa0 slot returns: what the render walk builds inline from
 * ObjDef +0x64/+0x14/+0x18 when the class does NOT override it (0x0045b96b). */
typedef struct RideDrawDesc {
    Spr*           sprite;       /* +0x00 */
    int            f04;          /* +0x04 */
    int            f08;          /* +0x08 */
    unsigned short f0c;          /* +0x0c */
} RideDrawDesc;

/* The packed map square a placement is keyed by: two bytes every list walk
 * compares as one 16-bit value. */
typedef union RideTile {
    unsigned short key;                        /* +0x00 */
    struct { unsigned char x, y; } b;
} RideTile;

/* The map position the place/remove handlers are handed (two ints). */
typedef struct Pos {
    int x;                       /* +0x00 */
    int y;                       /* +0x04 */
} Pos;

/* ---- shared engine entry points ----------------------------------------- */
extern void  DefaultCursor(void* cursor);                    /* 0x0045a390 */
extern void  SetEditCursorFootPrint(void* src);              /* 0x0045f440 */
#ifndef LEGOLAND_PORTABLE
extern void  AddBasicObject(void* obj, Pos* pos);            /* 0x0045efe0 */
#else
extern void AddBasicObject(void* ll_obj, void* ll_pos, void* ll_ctx);            /* 0x0045efe0 */
#define AddBasicObject(_a1, _a2) AddBasicObject((_a1), (_a2), 0)
#endif

extern int      g_edit_changed;                              /* 0x008119b0 EditMode */
extern RideDef* g_edit_object;                               /* 0x008119b8 */
extern char     g_edit_cursor;                               /* 0x007febc0 EditCursor */

/* ---- the six class ObjDef pointers -------------------------------------- */
extern RideDef* g_safari_def;                                /* 0x004cbec4 */
extern RideDef* g_spider_def;                                /* 0x004cbf20 */
extern RideDef* g_sbarrel_def;                               /* 0x0062fde4 */
extern RideDef* g_spacetower_def;                            /* 0x0062fd74 */
extern RideDef* g_plane_def;                                 /* 0x0062fe58 */
extern RideDef* g_copters_def;                               /* 0x004c1198 */

/* ==========================================================================
 * +0x8c -- SELECT FOR PLACEMENT
 * Byte-identical in all six rides (and in Joust and Temple Slide): flag the
 * edit state dirty, make this class the object being placed, reset the build
 * cursor and give it the class footprint (ObjDef+0x3c). The re-read of
 * 0x008119b8 after DefaultCursor is the original's -- it does not reuse the
 * register, so the global must be read again in the source.
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00414f00
void SafariRide_Select(void)
{
    g_edit_changed = 1;
    g_edit_object = g_safari_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

// FUNCTION: LEGOLAND 0x00416060
void SpiderRide_Select(void)
{
    g_edit_changed = 1;
    g_edit_object = g_spider_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

// FUNCTION: LEGOLAND 0x0043c490
void SpinningBarrels_Select(void)
{
    g_edit_changed = 1;
    g_edit_object = g_sbarrel_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

// FUNCTION: LEGOLAND 0x0043b420
void SpaceTower_Select(void)
{
    g_edit_changed = 1;
    g_edit_object = g_spacetower_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

// FUNCTION: LEGOLAND 0x0043df50
void PlaneRide_Select(void)
{
    g_edit_changed = 1;
    g_edit_object = g_plane_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

// FUNCTION: LEGOLAND 0x00404450
void Copters_Select(void)
{
    g_edit_changed = 1;
    g_edit_object = g_copters_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

/* ==========================================================================
 * +0x98 -- PLACE ONE ON THE MAP
 * The generic placer builds the footprint; the ride then opens its own record
 * for the square the copy was dropped on. The RideTile is built from the two
 * ints of the position BEFORE AddBasicObject is called (the byte loads sit
 * between the two argument pushes in every one of the six).
 * ========================================================================== */

extern void SafariRide_AddRecord(RideTile* tile);            /* 0x004149c0 */
extern void SpiderRide_AddRecord(RideTile* tile);            /* 0x004158f0 */
extern void SpinningBarrels_AddRecord(RideTile* tile);       /* 0x0043bdb0 */
extern void SpaceTower_AddRecord(RideTile* tile);            /* 0x0043ab70 */
extern void PlaneRide_AddRecord(RideTile* tile);             /* 0x0043d880 */
extern void Copters_AddRecord(RideTile* tile);               /* 0x00403c40 */

// FUNCTION: LEGOLAND 0x00414fc0
void SafariRide_Place(void* obj, Pos* pos)
{
    RideTile tile;

    tile.b.x = (unsigned char)pos->x;
    tile.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    SafariRide_AddRecord(&tile);
}

// FUNCTION: LEGOLAND 0x004160f0
void SpiderRide_Place(void* obj, Pos* pos)
{
    RideTile tile;

    tile.b.x = (unsigned char)pos->x;
    tile.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    SpiderRide_AddRecord(&tile);
}

// FUNCTION: LEGOLAND 0x0043c540
void SpinningBarrels_Place(void* obj, Pos* pos)
{
    RideTile tile;

    tile.b.x = (unsigned char)pos->x;
    tile.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    SpinningBarrels_AddRecord(&tile);
}

// FUNCTION: LEGOLAND 0x0043b4b0
void SpaceTower_Place(void* obj, Pos* pos)
{
    RideTile tile;

    tile.b.x = (unsigned char)pos->x;
    tile.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    SpaceTower_AddRecord(&tile);
}

// FUNCTION: LEGOLAND 0x0043dfe0
void PlaneRide_Place(void* obj, Pos* pos)
{
    RideTile tile;

    tile.b.x = (unsigned char)pos->x;
    tile.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    PlaneRide_AddRecord(&tile);
}

// FUNCTION: LEGOLAND 0x00404600
void Copters_Place(void* obj, Pos* pos)
{
    RideTile tile;

    tile.b.x = (unsigned char)pos->x;
    tile.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    Copters_AddRecord(&tile);
}

/* ==========================================================================
 * +0xa0 -- DRAW DESCRIPTOR OVERRIDE
 * Four of the six override it (Space Tower and Copters do not: their +0xa0 is
 * a real draw). The render walk (0x0045b938) tests ObjDef->flags & 0x400 and,
 * when set, uses the block this returns instead of building the same one on
 * its own stack. Each also arms the custom-draw bit on the class sprite on the
 * way out, which is what routes the class through the +0xb0 handler.
 * ========================================================================== */

extern RideDrawDesc g_safari_draw;                           /* 0x004cbed0 */
extern RideDrawDesc g_spider_draw;                           /* 0x004cbf40 */
extern RideDrawDesc g_sbarrel_draw;                          /* 0x0062fdb0 */
extern RideDrawDesc g_plane_draw;                            /* 0x0062fe60 */

#ifdef LEGOLAND_PORTABLE
/* PORT-M12: the ObjDef +0xa0 DRAW slot is typed
 * `SpriteDesc* (*draw)(void* ctx, BPos base)` (renderview.c:304) and
 * renderview.c:1238 calls it that way, so on wasm32 the 2-byte square goes
 * INDIRECTLY -- a pointer to a shadow-stack temp -- while this body's
 * `unsigned short` is a direct i32.  Same arity, no wasm-ld warning: PORT-M10
 * s1b's silent window, in the direction PORT-M11 s1b closed for eight other
 * classes.  These ten were invisible to BOTH sweeps because the name stored in
 * the slot (`SafariRide_Draw`, interfaces.c:768) is not the name of the
 * body, and the slot section pairs slot to body BY NAME.
 * PORT-M3's `_vc6_body` rename: VC6 compiles the matched text unchanged and
 * the portable build exports a wrapper of the slot's own shape over it. */
#define SafariRide_GetDrawDesc SafariRide_GetDrawDesc_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00414ff0
RideDrawDesc* SafariRide_GetDrawDesc(RideElem* elem, unsigned short arg)
{
    RideDef* def = elem->data;

    g_safari_draw.sprite = def->sprite;
    g_safari_draw.f04 = def->f14;
    g_safari_draw.f08 = def->f18;
    g_safari_draw.f0c = arg;
    def->sprite->flags |= 0x2000;
    return &g_safari_draw;
}
#ifdef LEGOLAND_PORTABLE
#undef SafariRide_GetDrawDesc
RideDrawDesc* SafariRide_GetDrawDesc(RideElem* elem, RideTile base)
{
    return SafariRide_GetDrawDesc_vc6_body(elem, base.key);
}
#endif

#ifdef LEGOLAND_PORTABLE
/* PORT-M12: the ObjDef +0xa0 DRAW slot is typed
 * `SpriteDesc* (*draw)(void* ctx, BPos base)` (renderview.c:304) and
 * renderview.c:1238 calls it that way, so on wasm32 the 2-byte square goes
 * INDIRECTLY -- a pointer to a shadow-stack temp -- while this body's
 * `unsigned short` is a direct i32.  Same arity, no wasm-ld warning: PORT-M10
 * s1b's silent window, in the direction PORT-M11 s1b closed for eight other
 * classes.  These ten were invisible to BOTH sweeps because the name stored in
 * the slot (`SpiderRide_Draw`, interfaces.c:815) is not the name of the
 * body, and the slot section pairs slot to body BY NAME.
 * PORT-M3's `_vc6_body` rename: VC6 compiles the matched text unchanged and
 * the portable build exports a wrapper of the slot's own shape over it. */
#define SpiderRide_GetDrawDesc SpiderRide_GetDrawDesc_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00416120
RideDrawDesc* SpiderRide_GetDrawDesc(RideElem* elem, unsigned short arg)
{
    RideDef* def = elem->data;

    g_spider_draw.sprite = def->sprite;
    g_spider_draw.f04 = def->f14;
    g_spider_draw.f08 = def->f18;
    g_spider_draw.f0c = arg;
    def->sprite->flags |= 0x2000;
    return &g_spider_draw;
}
#ifdef LEGOLAND_PORTABLE
#undef SpiderRide_GetDrawDesc
RideDrawDesc* SpiderRide_GetDrawDesc(RideElem* elem, RideTile base)
{
    return SpiderRide_GetDrawDesc_vc6_body(elem, base.key);
}
#endif

#ifdef LEGOLAND_PORTABLE
/* PORT-M12: the ObjDef +0xa0 DRAW slot is typed
 * `SpriteDesc* (*draw)(void* ctx, BPos base)` (renderview.c:304) and
 * renderview.c:1238 calls it that way, so on wasm32 the 2-byte square goes
 * INDIRECTLY -- a pointer to a shadow-stack temp -- while this body's
 * `unsigned short` is a direct i32.  Same arity, no wasm-ld warning: PORT-M10
 * s1b's silent window, in the direction PORT-M11 s1b closed for eight other
 * classes.  These ten were invisible to BOTH sweeps because the name stored in
 * the slot (`SpinningBarrels_Draw`, interfaces.c:917) is not the name of the
 * body, and the slot section pairs slot to body BY NAME.
 * PORT-M3's `_vc6_body` rename: VC6 compiles the matched text unchanged and
 * the portable build exports a wrapper of the slot's own shape over it. */
#define SpinningBarrels_GetDrawDesc SpinningBarrels_GetDrawDesc_vc6_body
#endif
// FUNCTION: LEGOLAND 0x0043c570
RideDrawDesc* SpinningBarrels_GetDrawDesc(RideElem* elem, unsigned short arg)
{
    RideDef* def = elem->data;

    g_sbarrel_draw.sprite = def->sprite;
    g_sbarrel_draw.f04 = def->f14;
    g_sbarrel_draw.f08 = def->f18;
    g_sbarrel_draw.f0c = arg;
    def->sprite->flags |= 0x2000;
    return &g_sbarrel_draw;
}
#ifdef LEGOLAND_PORTABLE
#undef SpinningBarrels_GetDrawDesc
RideDrawDesc* SpinningBarrels_GetDrawDesc(RideElem* elem, RideTile base)
{
    return SpinningBarrels_GetDrawDesc_vc6_body(elem, base.key);
}
#endif

#ifdef LEGOLAND_PORTABLE
/* PORT-M12: the ObjDef +0xa0 DRAW slot is typed
 * `SpriteDesc* (*draw)(void* ctx, BPos base)` (renderview.c:304) and
 * renderview.c:1238 calls it that way, so on wasm32 the 2-byte square goes
 * INDIRECTLY -- a pointer to a shadow-stack temp -- while this body's
 * `unsigned short` is a direct i32.  Same arity, no wasm-ld warning: PORT-M10
 * s1b's silent window, in the direction PORT-M11 s1b closed for eight other
 * classes.  These ten were invisible to BOTH sweeps because the name stored in
 * the slot (`PlaneRide_Draw`, interfaces.c:964) is not the name of the
 * body, and the slot section pairs slot to body BY NAME.
 * PORT-M3's `_vc6_body` rename: VC6 compiles the matched text unchanged and
 * the portable build exports a wrapper of the slot's own shape over it. */
#define PlaneRide_GetDrawDesc PlaneRide_GetDrawDesc_vc6_body
#endif
// FUNCTION: LEGOLAND 0x0043e010
RideDrawDesc* PlaneRide_GetDrawDesc(RideElem* elem, unsigned short arg)
{
    RideDef* def = elem->data;

    g_plane_draw.sprite = def->sprite;
    g_plane_draw.f04 = def->f14;
    g_plane_draw.f08 = def->f18;
    g_plane_draw.f0c = arg;
    def->sprite->flags |= 0x2000;
    return &g_plane_draw;
}
#ifdef LEGOLAND_PORTABLE
#undef PlaneRide_GetDrawDesc
RideDrawDesc* PlaneRide_GetDrawDesc(RideElem* elem, RideTile base)
{
    return PlaneRide_GetDrawDesc_vc6_body(elem, base.key);
}
#endif

/* ==========================================================================
 * +0x9c -- TAKE ONE OFF THE MAP
 * The common shape is: find this square's record, drop it, unbuild the
 * footprint (StandardRemoveObject), then evict every rider the class still
 * has parked on that square (RemoveAllBlokesFromRide). Three of the six add
 * something:
 *   SAFARI  fades out the sample it had sourced at that square -- AND does
 *           the whole removal INSIDE the `if (rec)`, so a copy with no
 *           record is never unbuilt (reproduced).
 *   COPTERS fades too, but reads the map square back out of the record it has
 *           just FREED, without re-testing it for null: removing a Copters
 *           with no record dereferences NULL. Original bug, reproduced.
 *   SPIDER / SPACE TOWER call one more ride-local helper with the square (the
 *           spider before the unbuild, the tower after the eviction).
 * The by-value tile argument's home slot doubles as the RideTile the find
 * helpers are handed.
 * ========================================================================== */

/* The by-value map square the remove handler is handed: a two-byte struct in
 * ONE argument slot. Declaring it as a struct (not an unsigned int) is what
 * makes VC6 read the slot as a DWORD and mask it -- `mov edx,[esp+0x35] /
 * and edx,0xff` -- when the two coordinates are copied into the sound source
 * (input2.c's RemoveSoundObject has the identical shape). */
typedef struct CellPos {
    unsigned char x;             /* +0x00 */
    unsigned char y;             /* +0x01 */
} CellPos;

extern void  StandardRemoveObject(void* obj, CellPos tile, void* ctx);            /* 0x0045f220 */
extern void  RemoveAllBlokesFromRide(RideDef* cls, CellPos tile);                 /* 0x0048a2e0 */
extern void  UnSourceAndFadeAllSamplesFromSource(void* src, int fade);            /* 0x00496c80 */

/* money.c's SoundSource: kind 2 = "a map square"; +0x04 is never initialised
 * for that kind (reproduced -- the original leaves it holding stack junk). */
typedef struct RideSoundSource {
    int kind;                    /* +0x00 */
    int f04;                     /* +0x04 */
    int x;                       /* +0x08 */
    int y;                       /* +0x0c */
} RideSoundSource;

/* Fade out whatever this ride was playing at one map square. Has to be an
 * INLINED helper taking the two coordinates as scalars, exactly as in joust.c:
 * that is what makes VC6 read both coordinates at the call site before
 * touching the sound-source struct and lets the struct live in the
 * inline-expansion temporary pool. */
static __inline void FadeSamplesAtMapSquare(int x, int y)
{
    RideSoundSource src;

    src.kind = 2;
    src.x = x;
    src.y = y;
    UnSourceAndFadeAllSamplesFromSource(&src, -200);
}

/* A placed map object: its class record sits at +0x0c. */
typedef struct RideMapObj {
    unsigned char pad00[0x0c];
    RideDef*      cls;           /* +0x0c */
} RideMapObj;

typedef struct SafariRec  SafariRec;
typedef struct SpiderRec  SpiderRec;
typedef struct SBarrelRec SBarrelRec;
typedef struct TowerRec   TowerRec;
typedef struct PlaneRec   PlaneRec;
typedef struct CoptersRec CoptersRec;

extern SafariRec*  SafariRide_FindRecord(RideTile* tile);            /* 0x00414a80 */
extern void        SafariRide_RemoveRecord(SafariRec* rec);          /* 0x00414a00 */
extern SpiderRec*  SpiderRide_FindRecord(RideTile* tile);            /* 0x004159b0 */
extern void        SpiderRide_RemoveRecord(SpiderRec* rec);          /* 0x00415930 */
extern void        SpiderRide_ReleaseSquare(RideTile* tile);         /* 0x00415a20 */
extern SBarrelRec* SpinningBarrels_FindRecord(RideTile* tile);       /* 0x0043be40 */
extern void        SpinningBarrels_RemoveRecord(SBarrelRec* rec);    /* 0x0043be00 */
extern TowerRec*   SpaceTower_FindRecord(RideTile* tile);            /* 0x0043ac40 */
extern void        SpaceTower_RemoveRecord(TowerRec* rec);           /* 0x0043abc0 */
extern void        SpaceTower_ReleaseSquare(RideTile* tile);         /* 0x0043aa50 */
extern PlaneRec*   PlaneRide_FindRecord(RideTile* tile);             /* 0x0043d960 */
extern void        PlaneRide_RemoveRecord(PlaneRec* rec);            /* 0x0043d8c0 */
extern CoptersRec* Copters_FindRecord(RideTile* tile);               /* 0x00403d00 */
extern void        Copters_RemoveRecord(CoptersRec* rec);            /* 0x00403c80 */

/* The record head is the packed map square in every one of the six. The
 * Copters record in full (0xd8 bytes; ridesave.c serialises it raw): */
typedef struct RiderNode RiderNode;

typedef struct CopterSeat {
    unsigned char pad00[0x18];
    RiderNode*    rider;         /* +0x18 which rider is in this copter */
    unsigned char pad1c[4];
} CopterSeat;

struct CoptersRec {
    RideTile      tile;          /* +0x00 the map square this copy sits on */
    unsigned char seated;        /* +0x02 how many riders are seated */
    unsigned char riders;        /* +0x03 how many are on the ride at all */
    struct CoptersRec* next;     /* +0x04 */
    unsigned char pad08[8];
    unsigned char joined;        /* +0x10 how many have claimed a copter */
    unsigned char pad11[3];
    int           timer;         /* +0x14 the machine's countdown */
    CopterSeat    seat[6];       /* +0x18 .. +0xd8 one per copter */
};

// FUNCTION: LEGOLAND 0x00414f40
void SafariRide_Remove(void* obj, CellPos tile, void* ctx)
{
    SafariRec* rec = SafariRide_FindRecord((RideTile*)&tile);

    if (rec) {
        SafariRide_RemoveRecord(rec);
        StandardRemoveObject(obj, tile, ctx);
        RemoveAllBlokesFromRide(((RideMapObj*)obj)->cls, tile);
        FadeSamplesAtMapSquare(tile.x, tile.y);
    }
}

// FUNCTION: LEGOLAND 0x004160a0
void SpiderRide_Remove(void* obj, CellPos tile, void* ctx)
{
    SpiderRec* rec = SpiderRide_FindRecord((RideTile*)&tile);

    if (rec)
        SpiderRide_RemoveRecord(rec);
    SpiderRide_ReleaseSquare((RideTile*)&tile);
    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(((RideMapObj*)obj)->cls, tile);
}

// FUNCTION: LEGOLAND 0x0043c4f0
void SpinningBarrels_Remove(void* obj, CellPos tile, void* ctx)
{
    SBarrelRec* rec = SpinningBarrels_FindRecord((RideTile*)&tile);

    if (rec)
        SpinningBarrels_RemoveRecord(rec);
    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(((RideMapObj*)obj)->cls, tile);
}

// FUNCTION: LEGOLAND 0x0043b460
void SpaceTower_Remove(void* obj, CellPos tile, void* ctx)
{
    TowerRec* rec = SpaceTower_FindRecord((RideTile*)&tile);

    if (rec)
        SpaceTower_RemoveRecord(rec);
    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(((RideMapObj*)obj)->cls, tile);
    SpaceTower_ReleaseSquare((RideTile*)&tile);
}

// FUNCTION: LEGOLAND 0x0043df90
void PlaneRide_Remove(void* obj, CellPos tile, void* ctx)
{
    PlaneRec* rec = PlaneRide_FindRecord((RideTile*)&tile);

    if (rec)
        PlaneRide_RemoveRecord(rec);
    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(((RideMapObj*)obj)->cls, tile);
}

// FUNCTION: LEGOLAND 0x00404580
void Copters_Remove(void* obj, CellPos tile, void* ctx)
{
    CoptersRec* rec = Copters_FindRecord((RideTile*)&tile);

    if (rec)
        Copters_RemoveRecord(rec);
    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(((RideMapObj*)obj)->cls, tile);
    /* Original bug: `rec` is neither re-tested nor still allocated here. */
    FadeSamplesAtMapSquare(rec->tile.b.x, rec->tile.b.y);
}

/* ==========================================================================
 * +0xac -- DESTROY (free the ride's resources)
 * The mirror of +0xa4: kill the sprites, free the .bnv depth buffers, free
 * the FX (sample) list and drain the per-placement record list. Four of the
 * six guard every free with a null test (the resource may never have loaded);
 * Safari and Space Tower do not. Space Tower additionally clears the CLASS's
 * whole rider list pointer (ObjDef+0xcc = 0) -- it is the only one of the six
 * that does, and it leaks whatever rider nodes were still on it.
 * ========================================================================== */

#ifndef LEGOLAND_PORTABLE
extern void  KillSprite(void* sprite);                       /* 0x00497bd0 */
#else
extern int KillSprite(void* sprite);                       /* 0x00497bd0 */
#endif
extern void  FreeBinV(void* bnv);                            /* 0x0044dd60 */
extern void  Kill_FXList(void* list, int count);             /* 0x00496e30 */
extern void  FreeWalkPath(void* path);                       /* 0x00412290 */
extern void  UnloadPos(void* table);                         /* 0x0043f7d0 */

extern void  SafariRide_FreeAllRecords(void);                /* 0x00414a60 */
extern void  SpiderRide_FreeAllRecords(void);                /* 0x00415990 */
extern void  SpinningBarrels_FreeAllRecords(void);           /* 0x0043c4d0 */
extern void  SpaceTower_FreeAllRecords(void);                /* 0x0043ac20 */
extern void  PlaneRide_FreeAllRecords(void);                 /* 0x0043d940 */
extern void  Copters_FreeAllRecords(void);                   /* 0x00403ce0 */

/* Safari: three .bnv path sets and one sprite. NOTE the addresses -- 0x4cbef8
 * is what ridesave.c calls g_safari_obj_samples[], indexed 1.. by the save
 * loader, so the "object sample table" of a mechanical ride is really its
 * table of BNV path sets (Temple Slide's loader makes the same identity
 * explicit: g_ts_objsamples = g_ts_binv). */
extern void*  g_safari_fx;                                   /* 0x004b4cb8 (1 entry) */
extern void*  g_safari_bnv0;                                 /* 0x004cbef8 */
extern void*  g_safari_bnv1;                                 /* 0x004cbefc */
extern void*  g_safari_bnv2;                                 /* 0x004cbf00 */
extern void*  g_safari_zspr;                                 /* 0x004cbf08 */

// FUNCTION: LEGOLAND 0x00414ea0
void SafariRide_Destroy(RideElem* elem)
{
    g_safari_def = elem->data;
    SafariRide_FreeAllRecords();
    Kill_FXList(&g_safari_fx, 1);
    FreeBinV(g_safari_bnv0);
    FreeBinV(g_safari_bnv1);
    FreeBinV(g_safari_bnv2);
    KillSprite(g_safari_zspr);
}

extern void*  g_spider_fx;                                   /* 0x004b4d88 (1 entry) */
extern void*  g_spider_matte;                                /* 0x0082c668 */
extern void*  g_spider_bnv0;                                 /* 0x004cbf10 */
extern void*  g_spider_bnv1;                                 /* 0x004cbf24 */
extern void*  g_spider_bnv2;                                 /* 0x004cbf18 */
extern void*  g_spider_zspr;                                 /* 0x004cbf14 */
extern void*  g_spider_zspr2;                                /* 0x004cbf1c */

// FUNCTION: LEGOLAND 0x00415fd0
void SpiderRide_Destroy(RideElem* elem)
{
    g_spider_def = elem->data;
    if (g_spider_matte)
        KillSprite(g_spider_matte);
    if (g_spider_bnv0)
        FreeBinV(g_spider_bnv0);
    if (g_spider_bnv1)
        FreeBinV(g_spider_bnv1);
    if (g_spider_bnv2)
        FreeBinV(g_spider_bnv2);
    KillSprite(g_spider_zspr);
    KillSprite(g_spider_zspr2);
    SpiderRide_FreeAllRecords();
    Kill_FXList(&g_spider_fx, 1);
}

extern void*  g_plane_fx;                                    /* 0x004b79d0 (2 entries) */
extern void*  g_plane_matte;                                 /* 0x0081cae0 */
extern void*  g_plane_bnv0;                                  /* 0x0062fe90 */
extern void*  g_plane_bnv1;                                  /* 0x0062fe94 */
extern void*  g_plane_bnv2;                                  /* 0x0062fe78 */

// FUNCTION: LEGOLAND 0x0043dee0
void PlaneRide_Destroy(RideElem* elem)
{
    g_plane_def = elem->data;
    if (g_plane_matte)
        KillSprite(g_plane_matte);
    if (g_plane_bnv0)
        FreeBinV(g_plane_bnv0);
    if (g_plane_bnv1)
        FreeBinV(g_plane_bnv1);
    if (g_plane_bnv2)
        FreeBinV(g_plane_bnv2);
    PlaneRide_FreeAllRecords();
    Kill_FXList(&g_plane_fx, 2);
}

extern void*  g_spacetower_fx;                               /* 0x004b7618 (1 entry) */
extern void*  g_spacetower_spr0;                             /* 0x0062fd80 */
extern void*  g_spacetower_spr1;                             /* 0x0062fd7c */
extern void*  g_spacetower_spr2;                             /* 0x0062fd68 */
extern void*  g_spacetower_spr3;                             /* 0x0062fd6c */

// FUNCTION: LEGOLAND 0x0043b570
void SpaceTower_Destroy(void)
{
    KillSprite(g_spacetower_spr0);
    KillSprite(g_spacetower_spr1);
    KillSprite(g_spacetower_spr2);
    KillSprite(g_spacetower_spr3);
    SpaceTower_FreeAllRecords();
    Kill_FXList(&g_spacetower_fx, 1);
    g_spacetower_def->riders = 0;
}

extern void*  g_sbarrel_spr0;                                /* 0x0062fdd0 */
extern void*  g_sbarrel_spr1;                                /* 0x0062fdcc */
extern void*  g_sbarrel_spr2;                                /* 0x0062fe04 */
extern void*  g_sbarrel_bnv0;                                /* 0x0062fde8 */
extern void*  g_sbarrel_bnv1;                                /* 0x0062fdfc */
extern void*  g_sbarrel_bnv2;                                /* 0x0062fdc8 */

/* Ends in a tail `jmp SpinningBarrels_FreeAllRecords` with no `ret` of its
 * own: exact under tools/audit.py (29/29, 97 bytes), but the shared
 * tools/match.py cannot bound a tail-jump function, so the marker stays WIP. */
// FUNCTION: LEGOLAND 0x0043c5b0
void SpinningBarrels_Destroy(void)
{
    KillSprite(g_sbarrel_spr0);
    KillSprite(g_sbarrel_spr1);
    KillSprite(g_sbarrel_spr2);
    if (g_sbarrel_bnv0)
        FreeBinV(g_sbarrel_bnv0);
    if (g_sbarrel_bnv1)
        FreeBinV(g_sbarrel_bnv1);
    if (g_sbarrel_bnv2)
        FreeBinV(g_sbarrel_bnv2);
    SpinningBarrels_FreeAllRecords();
}

/* Copters owns TEN copter sprites in one array plus five walk paths (the five
 * flight polylines its riders are carried along) -- freed out of order, so
 * they are five separate globals, not an array. */
extern void*  g_copters_fx;                                  /* 0x004b4140 (4 entries) */
extern void*  g_copters_matte;                               /* 0x004c1120 */
extern void*  g_copters_spr[10];                             /* 0x004c113c..0x004c1160 */
/* The five are consecutive dwords (see g_copters_paths[5] below); the names
 * follow the addresses.  They were previously numbered out of order, which
 * made every use site name the wrong object -- relocs.py caught it. */
extern void*  g_copters_path0;                               /* 0x004c1124 */
extern void*  g_copters_path1;                               /* 0x004c1128 */
extern void*  g_copters_path2;                               /* 0x004c112c */
extern void*  g_copters_path3;                               /* 0x004c1130 */
extern void*  g_copters_path4;                               /* 0x004c1134 */
extern void*  g_copters_postable;                            /* 0x00830f98 */
extern void*  g_copters_paths[5];                            /* 0x004c1124 (the five as one array) */

// FUNCTION: LEGOLAND 0x00404040
void Copters_Destroy(void)
{
    int i;

    if (g_copters_matte)
        KillSprite(g_copters_matte);
    for (i = 0; i < 10; i++) {
        if (g_copters_spr[i])
            KillSprite(g_copters_spr[i]);
    }
    if (g_copters_path0)
        FreeWalkPath(g_copters_path0);
    if (g_copters_path1)
        FreeWalkPath(g_copters_path1);
    if (g_copters_path3)
        FreeWalkPath(g_copters_path3);
    if (g_copters_path4)
        FreeWalkPath(g_copters_path4);
    if (g_copters_path2)
        FreeWalkPath(g_copters_path2);
    Copters_FreeAllRecords();
    Kill_FXList(&g_copters_fx, 4);
    UnloadPos(g_copters_postable);
}

/* Space Tower and Copters override +0xa0 as well, and use it for a second
 * job: HIDING SPRITE LAYERS. The class's multi-layer build sprite carries one
 * layer per vehicle (four cars for the tower, ten copters for the roundabout);
 * the draw-descriptor call hides the ones that must not be blitted by the
 * generic path, because the ride draws them itself in +0xb0 at the position
 * its own record says. Copters only bothers when the square actually has a
 * record. Both re-read the sprite global for every call (never hoisted). */

extern void  HideLayer(void* sprite, int layer);             /* 0x00497de0 */

extern RideDrawDesc g_spacetower_draw;                       /* 0x0062fd48 */
extern void*        g_spacetower_layers;                     /* 0x0062fd60 */

#ifdef LEGOLAND_PORTABLE
/* PORT-M12: the ObjDef +0xa0 DRAW slot is typed
 * `SpriteDesc* (*draw)(void* ctx, BPos base)` (renderview.c:304) and
 * renderview.c:1238 calls it that way, so on wasm32 the 2-byte square goes
 * INDIRECTLY -- a pointer to a shadow-stack temp -- while this body's
 * `unsigned short` is a direct i32.  Same arity, no wasm-ld warning: PORT-M10
 * s1b's silent window, in the direction PORT-M11 s1b closed for eight other
 * classes.  These ten were invisible to BOTH sweeps because the name stored in
 * the slot (`SpaceTower_Draw`, interfaces.c:854) is not the name of the
 * body, and the slot section pairs slot to body BY NAME.
 * PORT-M3's `_vc6_body` rename: VC6 compiles the matched text unchanged and
 * the portable build exports a wrapper of the slot's own shape over it. */
#define SpaceTower_GetDrawDesc SpaceTower_GetDrawDesc_vc6_body
#endif
// FUNCTION: LEGOLAND 0x0043b4e0
RideDrawDesc* SpaceTower_GetDrawDesc(RideElem* elem, unsigned short tile)
{
    RideDef* def = elem->data;

    g_spacetower_draw.sprite = def->sprite;
    g_spacetower_draw.f04 = def->f14;
    g_spacetower_draw.f08 = def->f18;
    g_spacetower_draw.f0c = tile;
    def->sprite->flags |= 0x2000;
    HideLayer(g_spacetower_layers, 6);
    HideLayer(g_spacetower_layers, 4);
    HideLayer(g_spacetower_layers, 0);
    HideLayer(g_spacetower_layers, 2);
    HideLayer(g_spacetower_layers, 5);
    return &g_spacetower_draw;
}
#ifdef LEGOLAND_PORTABLE
#undef SpaceTower_GetDrawDesc
RideDrawDesc* SpaceTower_GetDrawDesc(RideElem* elem, RideTile base)
{
    return SpaceTower_GetDrawDesc_vc6_body(elem, base.key);
}
#endif

extern RideDrawDesc g_copters_draw;                          /* 0x004c1170 */
extern void*        g_copters_layers;                        /* 0x004c1138 */

#ifdef LEGOLAND_PORTABLE
/* PORT-M12: the ObjDef +0xa0 DRAW slot is typed
 * `SpriteDesc* (*draw)(void* ctx, BPos base)` (renderview.c:304) and
 * renderview.c:1238 calls it that way, so on wasm32 the 2-byte square goes
 * INDIRECTLY -- a pointer to a shadow-stack temp -- while this body's
 * `unsigned short` is a direct i32.  Same arity, no wasm-ld warning: PORT-M10
 * s1b's silent window, in the direction PORT-M11 s1b closed for eight other
 * classes.  These ten were invisible to BOTH sweeps because the name stored in
 * the slot (`Copters_Draw`, interfaces.c:715) is not the name of the
 * body, and the slot section pairs slot to body BY NAME.
 * PORT-M3's `_vc6_body` rename: VC6 compiles the matched text unchanged and
 * the portable build exports a wrapper of the slot's own shape over it. */
#define Copters_GetDrawDesc Copters_GetDrawDesc_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00404490
RideDrawDesc* Copters_GetDrawDesc(RideElem* elem, unsigned short tile)
{
    RideDef* def = elem->data;

    g_copters_draw.sprite = def->sprite;
    g_copters_draw.f04 = def->f14;
    g_copters_draw.f08 = def->f18;
    g_copters_draw.f0c = tile;
    def->sprite->flags |= 0x2000;
    if (Copters_FindRecord((RideTile*)&tile)) {
        HideLayer(g_copters_layers, 1);
        HideLayer(g_copters_layers, 2);
        HideLayer(g_copters_layers, 3);
        HideLayer(g_copters_layers, 10);
        HideLayer(g_copters_layers, 6);
        HideLayer(g_copters_layers, 5);
        HideLayer(g_copters_layers, 7);
        HideLayer(g_copters_layers, 8);
        HideLayer(g_copters_layers, 11);
        HideLayer(g_copters_layers, 4);
    }
    return &g_copters_draw;
}
#ifdef LEGOLAND_PORTABLE
#undef Copters_GetDrawDesc
RideDrawDesc* Copters_GetDrawDesc(RideElem* elem, RideTile base)
{
    return Copters_GetDrawDesc_vc6_body(elem, base.key);
}
#endif

/* ==========================================================================
 * +0xa4 -- CREATE (load the ride's resources)
 * Called by the ODF loader immediately after SetCustomCallbacks installs the
 * table. Every one of the six does the same four things and then loads its own
 * assets:
 *     def = elem->data                          cache the ObjDef
 *     def->flags |= 0x420                       arm +0xa8 (0x20) and +0xa0 (0x400)
 *     layers = def->sprite; layers->flags |= 0x2000   arm the custom draw +0xb0
 *     Load_FXList(fx, n)                        resolve the ride's .wav names
 * A ride whose flags get only 0x20 does not override the draw descriptor.
 * ========================================================================== */

extern void* LoadSprite(const char* name, int flag);         /* 0x00497ab0 */
extern void* LoadBinV(const char* name);                     /* 0x0044dc90 */
extern void  Load_FXList(void* list, int count);             /* 0x00496dd0 */
extern void* LoadPos(const char* name);                      /* 0x0043f660 */
extern void* BuildWalkPath(void* polyline);                  /* 0x00412100 */

/* Copters: TEN copter sprites (five small "s" + five medium "m" liveries),
 * one .pos table of 3D positions, five flight polylines, one base matte and a
 * four-entry FX list. The five pointer globals seeded here point at five
 * consecutive slots of the block at 0x004c119c -- the per-copter cursors the
 * update walks. */
extern const char* g_copters_spr_names[10];                  /* 0x004b4170 */

/* PORT-M8: these five are goldrush.c's `PolyLine` ({count, pts}), NOT `void*`.
 * The image lays the five out as [points][header] pairs and each header's
 * second word is the address of the point array immediately below it:
 *
 *   0x004b41c8 = { 6, 0x004b4198 }   0x004b4198 .. 0x004b41c8 = 6 x {int,int}
 *   0x004b4208 = { 7, 0x004b41d0 }   0x004b41d0 .. 0x004b4208 = 7 x {int,int}
 *   0x004b4240 = { 6, 0x004b4210 }   0x004b4210 .. 0x004b4240 = 6 x {int,int}
 *   0x004b4270 = { 5, 0x004b4248 }   0x004b4248 .. 0x004b4270 = 5 x {int,int}
 *   0x004b4298 = { 4, 0x004b4278 }   0x004b4278 .. 0x004b4298 = 4 x {int,int}
 *
 * -- the same shape `g_goldrush_polyline` (0x004b4608 = { 5, ... }) already
 * carries, and BuildWalkPath (0x00412100) is handed `&g_copters_polyN` here
 * exactly as it is handed `&g_goldrush_polyline` there.  The `void*` spelling
 * named the COUNT word and said nothing about the pointer word, so the
 * browser closure left all five `pts` as RAW x86 addresses: every copter
 * walk path was built from a number that means nothing in the rebuilt
 * layout.  `&g_copters_poly0` is the only use, so no byte moves. */
typedef struct PolyLine {
    int   count;                 /* +0x00 */
    Pos*  pts;                   /* +0x04 */
} PolyLine;

extern PolyLine g_copters_poly0;                             /* 0x004b41c8 (6 pts) */
extern PolyLine g_copters_poly1;                             /* 0x004b4208 (7 pts) */
extern PolyLine g_copters_poly2;                             /* 0x004b4240 (6 pts) */
extern PolyLine g_copters_poly3;                             /* 0x004b4270 (5 pts) */
extern PolyLine g_copters_poly4;                             /* 0x004b4298 (4 pts) */
extern void*  g_copters_cur0;                                /* 0x004c1194 */
extern void*  g_copters_cur1;                                /* 0x004c1190 */
extern void*  g_copters_cur2;                                /* 0x004c1164 */
extern void*  g_copters_cur3;                                /* 0x004c1168 */
extern void*  g_copters_cur4;                                /* 0x004c1188 */
extern int    g_copters_slot0;                               /* 0x004c119c */
extern int    g_copters_slot1;                               /* 0x004c11a0 */
extern int    g_copters_slot2;                               /* 0x004c11a4 */
extern int    g_copters_slot3;                               /* 0x004c11a8 */
extern int    g_copters_slot4;                               /* 0x004c11ac */

// FUNCTION: LEGOLAND 0x00403d90
void Copters_Create(RideElem* elem)
{
    int i;

    g_copters_def = elem->data;
    g_copters_def->flags |= 0x420;
    g_copters_layers = g_copters_def->sprite;
    ((Spr*)g_copters_layers)->flags |= 0x2000;
    for (i = 0; i < 10; i++)
        g_copters_spr[i] = LoadSprite(g_copters_spr_names[i], 1);
    g_copters_postable = LoadPos("3ddata\\copters.pos");
    g_copters_path0 = BuildWalkPath(&g_copters_poly0);
    g_copters_path2 = BuildWalkPath(&g_copters_poly1);
    g_copters_path1 = BuildWalkPath(&g_copters_poly2);
    g_copters_path3 = BuildWalkPath(&g_copters_poly3);
    g_copters_path4 = BuildWalkPath(&g_copters_poly4);
    g_copters_cur0 = &g_copters_slot0;
    g_copters_cur1 = &g_copters_slot1;
    g_copters_cur2 = &g_copters_slot2;
    g_copters_cur3 = &g_copters_slot3;
    g_copters_cur4 = &g_copters_slot4;
    g_copters_matte = LoadSprite("cop_base Matte.lls", 1);
    Load_FXList(&g_copters_fx, 4);
}

/* Safari: three .bnv depth buffers -- one per machine state ("run", "on",
 * "off") -- plus the rider z-sprite, and the pair of pixel offsets the draw
 * uses to place a rider on the vehicle (-41,-95). The last four stores are
 * what identify the ride's two "sample" tables from the save loader
 * (ridesave.c): the 3-entry table at 0x004cbef8 is the BNV set and the entry
 * at 0x004cbf08 is the z-sprite, so a mechanical ride's saved "sample index"
 * selects a depth-path set, not a wav. */
extern void  StopLayerPlaying(void* sprite, int layer);      /* 0x00441f00 */
extern void* GetLLSForLayer(void* sprite, int layer);        /* 0x00441ea0 */
extern void  LLSSetFrame(void* lls, int frame);              /* 0x0047d5a0 */

extern void*  g_safari_layers;                               /* 0x004cbec8 */
extern void*  g_safari_bnv_run;                              /* 0x004cbef4 */
extern void*  g_safari_bnv_on;                               /* 0x004cbf04 */
extern void*  g_safari_bnv_off;                              /* 0x004cbec0 */
extern void*  g_safari_zsprite;                              /* 0x0082c66c */
extern int    g_safari_zstate;                               /* 0x0082c670 */
extern int    g_safari_zframe;                               /* 0x0082c674 */
extern int    g_safari_rider_dx;                             /* 0x004cbee8 */
extern int    g_safari_rider_dy;                             /* 0x004cbeec */

// FUNCTION: LEGOLAND 0x00414d90
void SafariRide_Create(RideElem* elem)
{
    g_safari_def = elem->data;
    g_safari_def->flags |= 0x420;
    g_safari_layers = g_safari_def->sprite;
    ((Spr*)g_safari_layers)->flags |= 0x2000;
    g_safari_bnv_run = LoadBinV("Zbuffers\\Safarirun.bnv");
    g_safari_bnv_on = LoadBinV("Zbuffers\\Safarion.bnv");
    g_safari_bnv_off = LoadBinV("Zbuffers\\Safarioff.bnv");
    g_safari_zsprite = LoadSprite("z_Safari.lls", 1);
    g_safari_zstate = 0;
    g_safari_zframe = -1;
    g_safari_rider_dx = -41;
    g_safari_rider_dy = -95;
    HideLayer(g_safari_layers, 0);
    StopLayerPlaying(g_safari_layers, 0);
    LLSSetFrame(GetLLSForLayer(g_safari_layers, 0), 0);
    g_safari_bnv0 = g_safari_bnv_run;
    g_safari_bnv1 = g_safari_bnv_on;
    g_safari_bnv2 = g_safari_bnv_off;
    g_safari_zspr = g_safari_zsprite;
    Load_FXList(&g_safari_fx, 1);
}

/* PLANE RIDE loads "Zoomer" assets: internally this class is the Zoomer, the
 * name the save-chunk table in ridesave.c also uses. Two vehicle layers (1 and
 * 2) are hidden and parked on frame 0, and the last call fetches layer 1 into
 * a LayerOut block whose result is thrown away (a leftover; the 24-byte block
 * is why the frame is 0x18 bytes). */
typedef struct LayerOut {
    void* sprite;                /* +0x00 */
    int   dx;                    /* +0x04 */
    int   dy;                    /* +0x08 */
    int   pad0c;                 /* +0x0c */
    int   f10;                   /* +0x10 */
    int   pad14;                 /* +0x14 */
} LayerOut;

extern void  GetLayer(void* spr, LayerOut* out, unsigned int layer);  /* 0x00497e80 */

extern void*  g_plane_layers;                                /* 0x0062fe7c */
extern void*  g_plane_bnv_ride;                              /* 0x0062fe90 */
/* The plane's BNV table, three consecutive dwords that PlaneRide_Create fills
 * and PlaneRide_Update reads back.  0x0062fe84 is also reached as
 * g_zoomer_obj_samples[] in ridesave.c, which indexes the same dwords. */
extern void*  g_plane_tab0;                                  /* 0x0062fe84 */
extern void*  g_plane_tab1;                                  /* 0x0062fe88 */
extern void*  g_plane_tab2;                                  /* 0x0062fe8c */
extern void*  g_plane_bnv_on;                                /* 0x0062fe94 */
extern void*  g_plane_bnv_off;                               /* 0x0062fe78 */
extern void*  g_plane_zsprite;                               /* 0x0062fe98 */
extern int    g_plane_rider_dx;                              /* 0x0081cae8 */
extern int    g_plane_rider_dy;                              /* 0x0081caec */

// FUNCTION: LEGOLAND 0x0043dda0
void PlaneRide_Create(RideElem* elem)
{
    LayerOut lay;

    g_plane_def = elem->data;
    g_plane_def->flags |= 0x420;
    g_plane_layers = g_plane_def->sprite;
    ((Spr*)g_plane_layers)->flags |= 0x2000;
    g_plane_bnv_ride = LoadBinV("Zbuffers\\Zoomeride.bnv");
    g_plane_bnv_on = LoadBinV("Zbuffers\\Zoomer0n.bnv");
    g_plane_bnv_off = LoadBinV("Zbuffers\\Zoomer0ff.bnv");
    g_plane_matte = LoadSprite("z_Zoomer.lls", 1);
    g_plane_zsprite = g_plane_matte;
    g_plane_rider_dx = -10;
    g_plane_rider_dy = -107;
    /* These land in the plane's BNV table at 0x0062fe84..0x0062fe8c, which
     * PlaneRide_Update reads back through g_plane_tab1 / g_plane_tab2 -- NOT
     * in the 0x0062fe90/94/78 slots PlaneRide_Destroy frees.  The two trios
     * were conflated here until relocs.py caught the three wrong stores. */
    g_plane_tab0 = g_plane_bnv_ride;
    g_plane_tab1 = g_plane_bnv_on;
    g_plane_tab2 = g_plane_bnv_off;
    Load_FXList(&g_plane_fx, 2);
    HideLayer(g_plane_layers, 1);
    StopLayerPlaying(g_plane_layers, 1);
    LLSSetFrame(GetLLSForLayer(g_plane_layers, 1), 0);
    HideLayer(g_plane_layers, 2);
    StopLayerPlaying(g_plane_layers, 2);
    LLSSetFrame(GetLLSForLayer(g_plane_layers, 2), 0);
    GetLayer(g_plane_def->sprite, &lay, 1);
}

/* Spider: the same three-BNV set ("run"/"on"/"off"), two hut masks that
 * occlude the riders, and the rider z-sprite. The four-slot table at
 * 0x004cbf30 is the save loader's g_spider_obj_samples and 0x004cbf3c is
 * g_spider_samples[1] -- the same {bnv,bnv,bnv,zsprite} shape Safari and
 * Spinning Barrels build, which is what a saved "sample index" of 1..3 in a
 * mechanical ride's record actually selects. */
extern void*  g_spider_layers;                               /* 0x004cbf28 */
extern void*  g_spider_bnv_run;                              /* 0x004cbf10 */
extern void*  g_spider_bnv_on;                               /* 0x004cbf24 */
extern void*  g_spider_bnv_off;                              /* 0x004cbf18 */
extern void*  g_spider_tab0;                                 /* 0x004cbf30 */
extern void*  g_spider_tab1;                                 /* 0x004cbf34 */
extern void*  g_spider_tab2;                                 /* 0x004cbf38 */
extern void*  g_spider_tab3;                                 /* 0x004cbf3c */
extern int    g_spider_zframe;                               /* 0x0082c660 */
extern int    g_spider_zstate;                               /* 0x0082c664 */

// FUNCTION: LEGOLAND 0x00415e80
void SpiderRide_Create(RideElem* elem)
{
    g_spider_def = elem->data;
    g_spider_def->flags |= 0x420;
    g_spider_layers = g_spider_def->sprite;
    ((Spr*)g_spider_layers)->flags |= 0x2000;
    g_spider_bnv_run = LoadBinV("Zbuffers\\spiderrun.bnv");
    g_spider_bnv_on = LoadBinV("Zbuffers\\spideron.bnv");
    g_spider_bnv_off = LoadBinV("Zbuffers\\spideroff.bnv");
    g_spider_zspr = LoadSprite("SpiderHutMask1.lls", 1);
    g_spider_zspr2 = LoadSprite("SpiderHutMask2.lls", 1);
    g_spider_matte = LoadSprite("z_spider.lls", 1);
    g_spider_tab0 = g_spider_bnv_run;
    g_spider_tab3 = g_spider_matte;
    g_spider_zframe = -1;
    g_spider_zstate = 2;
    g_spider_tab1 = g_spider_bnv_on;
    g_spider_tab2 = g_spider_bnv_off;
    HideLayer(g_spider_layers, 2);
    StopLayerPlaying(g_spider_layers, 2);
    LLSSetFrame(GetLLSForLayer(g_spider_layers, 2), 0);
    HideLayer(g_spider_layers, 1);
    StopLayerPlaying(g_spider_layers, 1);
    LLSSetFrame(GetLLSForLayer(g_spider_layers, 1), 0);
    Load_FXList(&g_spider_fx, 1);
}

/* Spinning Barrels: one z-sprite, three BNV sets, two entrance mattes, and
 * the ride's own pixel pivot -- derived from LAYER 3's own offsets biased by
 * (-0x67,-0x37), the only one of the six that reads the pivot out of the
 * sprite instead of hard-coding it. */
extern void*  g_sbarrel_layers;                              /* 0x0062fde0 */
extern void*  g_sbarrel_zsprite;                             /* 0x0062fe00 */
extern void*  g_sbarrel_bnv_on;                              /* 0x0062fdfc */
extern void*  g_sbarrel_bnv_ride;                            /* 0x0062fde8 */
extern void*  g_sbarrel_tab0;                                /* 0x0062fdf0 */
extern void*  g_sbarrel_tab1;                                /* 0x0062fdf4 */
extern void*  g_sbarrel_tab2;                                /* 0x0062fdf8 */
extern int    g_sbarrel_pivot_x;                             /* 0x0062fdd8 */
extern int    g_sbarrel_pivot_y;                             /* 0x0062fddc */

// FUNCTION: LEGOLAND 0x0043c340
void SpinningBarrels_Create(RideElem* elem)
{
    LayerOut lay;

    g_sbarrel_def = elem->data;
    g_sbarrel_def->flags |= 0x420;
    g_sbarrel_layers = g_sbarrel_def->sprite;
    ((Spr*)g_sbarrel_layers)->flags |= 0x2000;
    g_sbarrel_zsprite = LoadSprite("z_SpinningBarrels.lls", 1);
    g_sbarrel_spr2 = g_sbarrel_zsprite;
    g_sbarrel_bnv_on = LoadBinV("Zbuffers\\BoxBlokesOn1.bnv");
    g_sbarrel_tab0 = g_sbarrel_bnv_on;
    g_sbarrel_bnv_ride = LoadBinV("Zbuffers\\SpinningBarrels.bnv");
    g_sbarrel_tab1 = g_sbarrel_bnv_ride;
    g_sbarrel_bnv2 = LoadBinV("Zbuffers\\BoxBlokesOff.bnv");
    g_sbarrel_tab2 = g_sbarrel_bnv2;
    HideLayer(g_sbarrel_layers, 3);
    StopLayerPlaying(g_sbarrel_layers, 3);
    LLSSetFrame(GetLLSForLayer(g_sbarrel_layers, 3), 0);
    HideLayer(g_sbarrel_layers, 2);
    StopLayerPlaying(g_sbarrel_layers, 2);
    LLSSetFrame(GetLLSForLayer(g_sbarrel_layers, 2), 0);
    GetLayer(g_sbarrel_def->sprite, &lay, 3);
    g_sbarrel_pivot_x = lay.dx - 0x67;
    g_sbarrel_pivot_y = lay.dy - 0x37;
    g_sbarrel_spr0 = LoadSprite("SpinningBarrelsEntranceMatte.lls", 1);
    g_sbarrel_spr1 = LoadSprite("SpinningBarrelsEntranceMatte2.lls", 1);
}

/* Space Tower: four seat/tower mattes and, unusually, FOUR cached layer
 * offsets read straight out of the sprite -- one per car (layers 6, 4, 0 and
 * 2 in that order). Those four pairs at 0x0062fd88..0x0062fda4 are the car
 * anchor points the update and the draw both work from, which is why the ride
 * never has to ask the sprite for them again. */
typedef struct Offset {
    int ox;                      /* +0x00 */
    int oy;                      /* +0x04 */
} Offset;

extern Offset GetRenderOffsetForLayer(void* sprite, int layer);   /* 0x00441ee0 */

extern Offset g_spacetower_car_ofs[4];                       /* 0x0062fd88 */
extern int    g_spacetower_state;                            /* 0x0062fd64 */
extern int    g_spacetower_phase;                            /* 0x0062fd70 */

// FUNCTION: LEGOLAND 0x0043b2b0
void SpaceTower_Create(RideElem* elem)
{
    g_spacetower_def = elem->data;
    g_spacetower_def->flags |= 0x420;
    g_spacetower_layers = g_spacetower_def->sprite;
    ((Spr*)g_spacetower_layers)->flags |= 0x2000;
    HideLayer(g_spacetower_layers, 5);
    StopLayerPlaying(g_spacetower_layers, 5);
    LLSSetFrame(GetLLSForLayer(g_spacetower_layers, 5), 0);
    HideLayer(g_spacetower_layers, 3);
    StopLayerPlaying(g_spacetower_layers, 3);
    LLSSetFrame(GetLLSForLayer(g_spacetower_layers, 3), 0);
    g_spacetower_car_ofs[0] = GetRenderOffsetForLayer(g_spacetower_layers, 6);
    g_spacetower_car_ofs[1] = GetRenderOffsetForLayer(g_spacetower_layers, 4);
    g_spacetower_car_ofs[2] = GetRenderOffsetForLayer(g_spacetower_layers, 0);
    g_spacetower_car_ofs[3] = GetRenderOffsetForLayer(g_spacetower_layers, 2);
    g_spacetower_state = 0;
    g_spacetower_spr2 = LoadSprite("SpaceTower Seat2 Matte.lls", 1);
    g_spacetower_spr3 = LoadSprite("SpaceTower Seat3 Matte.lls", 1);
    g_spacetower_phase = 0;
    g_spacetower_spr0 = LoadSprite("Spacet Matte1.lls", 1);
    g_spacetower_spr1 = LoadSprite("Spacet Matte2.lls", 1);
    Load_FXList(&g_spacetower_fx, 1);
}

/* ==========================================================================
 * +0xb0 -- THE CUSTOM DRAW (INTERACT)
 * ==========================================================================
 * Called by the render-list walker (0x00485ac3) instead of blitting the class
 * sprite, with (elem, screen x, screen y, &map square, clip rect, blit mode);
 * the clip rect is already installed, so argument 5 is dead in all six.
 *
 * COPTERS is the depth-sorted shape at its purest. The roundabout has five
 * DEPTH BANDS, each a private bloke render list (five RenderList* globals
 * pointing into the 6-slot block at 0x004c119c). Every non-riding bloke of the
 * class is bucketed by how many tiles it is IN FRONT OF the ride
 * (ride_row - bloke_row, from the bloke's 24.8 world y):
 *      >= 5 tiles behind  -> band 1
 *      3..4 tiles         -> band 3
 *      0..2 tiles         -> band 0
 * (a bloke closer to the camera than the ride lands in no band and is drawn by
 * the normal walk). The ride then interleaves vehicle layers and bands back to
 * front: layer 0, layer 2, bands 2 and 1, layers 3 and 4, bands 3 and 4,
 * layer 1, band 0. Note the bucketing does NOT filter by map square, so every
 * bloke of the class is considered for every copy of the ride -- an original
 * quirk, harmless only because two copies rarely overlap on screen.
 * Whether or not the square has a record, the base matte is always blitted
 * last, at the class sprite's own layer-0 offset plus the square's screen
 * origin.
 * ========================================================================== */

typedef struct Person3D {
    unsigned char pad00[0x1c];
    Offset        screen;        /* +0x1c where the person is drawn */
    Offset        local;         /* +0x24 the person's own offset pair */
    void*         zsprite;       /* +0x2c the ride's depth sprite while riding */
    int           f30;           /* +0x30 1 while the ride positions the person */
    int           f34;           /* +0x34 */
    unsigned char pad38[4];
    float         depth;         /* +0x3c */
} Person3D;

typedef struct Bloke {
    unsigned char  pad00[4];
    Person3D*      person;       /* +0x04 */
    unsigned char  pad08[6];
    unsigned short state;        /* +0x0e low-level AI state (0 = idle) */
    unsigned char  pad10[0x24 - 0x10];
    Pos            target;       /* +0x24 walk target, 24.8 */
    unsigned char  pad2c[9];
    unsigned char  b35;          /* +0x35 */
    unsigned char  seat;         /* +0x36 which seat/car this rider took */
    unsigned char  pad37;
    unsigned short f38;          /* +0x38 */
    unsigned char  pad3a[2];
    short          ride_dx;      /* +0x3c per-rider offset inside the ride */
    short          ride_dy;      /* +0x3e */
    unsigned char  pad40[0x4a - 0x40];
    unsigned short f4a;          /* +0x4a */
    unsigned short f4c;          /* +0x4c */
    unsigned char  pad4e[2];
    int            anim;         /* +0x50 animation id (indexes 0x004b775c) */
    void*          bnvpath;      /* +0x54 the BNV path the rider is riding */
    int            f58;          /* +0x58 */
    unsigned char  pad5c[4];
    unsigned char  action;       /* +0x60 this ride's state-machine step */
    unsigned char  pad61;
    unsigned short flags;        /* +0x62 8 = on this ride, 0x80 = riding */
    unsigned char  pad64[4];
    Pos            world;        /* +0x68 world position, 24.8 */
    unsigned char  pad70[3];
    unsigned char  new_dir;      /* +0x73 */
    unsigned char  b74;          /* +0x74 the frame the ride holds it on */
    unsigned char  pad75[0x98 - 0x75];
    unsigned char  path[0x14];   /* +0x98 CalcMoveLine scratch */
} Bloke;

/* A rider slot on the class-wide list at ObjDef+0xcc. */
struct RiderNode {
    struct RiderNode* next;      /* +0x00 */
    struct RiderNode* prev;      /* +0x04 */
    Bloke*            bloke;     /* +0x08 */
    unsigned short    ride_id;   /* +0x0c the packed map square it is using */
    unsigned short    pad0e;
    struct RideOwner* owner;     /* +0x10 */
};

typedef struct RideOwner {
    unsigned char pad00[0x20];
    int           key;           /* +0x20 the depth key for the render list */
} RideOwner;

typedef struct RenderList {
    void* head;                  /* +0x00 */
} RenderList;

extern Offset GetScreenCoordsForObject(RideTile* sq, RideDef* item); /* 0x00442cc0 */
extern void   AdjustOffsetForViewMode(Offset* o);                    /* 0x00442d30 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx);/* 0x004853a0 */
extern void   RenderItems_New(void);                                 /* 0x00442e90 */
extern void   AddBlokeToRenderList(RenderList* l, RiderNode* r, int key); /* 0x00442f20 */
extern void   RenderBlokeList(RenderList* l);                        /* 0x00442f70 */

extern void   Copters_DrawVehicle(CoptersRec* rec, int which, int mode); /* 0x004040f0 */

extern RenderList* g_copters_list0;                          /* 0x004c1194 -> 0x004c119c */
extern RenderList* g_copters_list1;                          /* 0x004c1190 -> 0x004c11a0 */
extern RenderList* g_copters_list2;                          /* 0x004c1164 -> 0x004c11a4 */
extern RenderList* g_copters_list3;                          /* 0x004c1168 -> 0x004c11a8 */
extern RenderList* g_copters_list4;                          /* 0x004c1188 -> 0x004c11ac */
extern RenderList  g_copters_bands[6];                       /* 0x004c119c */

/* The matte blit is an INLINED helper so its Offset pair lives in the
 * inline-expansion temporary pool: that is what lets VC6 colour the loop's
 * spilled row number onto the same 8-byte frame slot, which is why the whole
 * function needs only `sub esp,8`. A named function-level Offset gets its own
 * home and pushes the frame to 0xc. */
static __inline void Copters_DrawMatte(RideTile* sq, RideDef* item, int mode)
{
    Offset screen;
    Offset off;

    screen = GetScreenCoordsForObject(sq, item);
    off = GetRenderOffsetForLayer(item->sprite, 0);
    AdjustOffsetForViewMode(&off);
    PrintSprite(g_copters_matte, screen.ox + off.ox, screen.oy + off.oy,
                mode, 0);
}

/* LEVER (worth 2 of 150, and a general rule): the loop-invariant ride row is
 * spilled to the UPPER half of the same 8-byte frame slot the inlined matte
 * helper's Offset uses. A bare `int ty` colours onto the LOWER half; the upper
 * half only comes out if the row is the SECOND field of a two-int local, i.e.
 * the original declared the ride's map position as a Pos and used only its y.
 * The pair {inlined helper for the tail, Pos for the row} is what gets this
 * function to 150/150 with `sub esp,8`. */
// FUNCTION: LEGOLAND 0x00404290
void Copters_Interact(RideElem* elem, int x, int y, RideTile* sq,
                      void* clip, int mode)
{
    RideDef*    item = elem->data;
    CoptersRec* rec = Copters_FindRecord(sq);
    RiderNode*  r;

    if (rec) {
        Pos ride;                /* only .y is ever used (original) */

        RenderItems_New();
        g_copters_bands[0].head = 0;
        g_copters_bands[1].head = 0;
        g_copters_bands[2].head = 0;
        g_copters_bands[3].head = 0;
        g_copters_bands[4].head = 0;
        g_copters_bands[5].head = 0;
        ride.y = rec->tile.b.y + g_copters_def->base_y;
        r = g_copters_def->riders;
        while (r) {
            if (!(r->bloke->flags & 0x80)) {
                int key = r->owner->key;
                int d = ride.y - (r->bloke->world.y >> 8);
                if (d >= 5)
                    AddBlokeToRenderList(g_copters_list1, r, key);
                if (d >= 3 && d <= 4)
                    AddBlokeToRenderList(g_copters_list3, r, key);
                if (d >= 0 && d <= 2)
                    AddBlokeToRenderList(g_copters_list0, r, key);
            }
            r = r->next;
        }
        Copters_DrawVehicle(rec, 0, mode);
        Copters_DrawVehicle(rec, 2, mode);
        RenderBlokeList(g_copters_list2);
        RenderBlokeList(g_copters_list1);
        Copters_DrawVehicle(rec, 3, mode);
        Copters_DrawVehicle(rec, 4, mode);
        RenderBlokeList(g_copters_list3);
        RenderBlokeList(g_copters_list4);
        Copters_DrawVehicle(rec, 1, mode);
        RenderBlokeList(g_copters_list0);
    }
    Copters_DrawMatte(sq, item, mode);
}

/* SAFARI RIDE's draw is the two-pass shape with an extra twist: before it
 * draws anything it publishes the record's animation frame into the ride's
 * Z-SPRITE (the depth sprite the 3D rider pass renders against), through a
 * pointer-to-pointer at zsprite+8 -- so the depth image tracks the vehicle
 * animation frame by frame. Then:
 *   - it scans the class rider list ONCE for anyone using this square; the
 *     result is a single flag byte, homed in the dead `elem` argument slot;
 *   - vehicle layer 0 is set to the record's frame and blitted either way,
 *     but only when the flag is set does it walk the riders, place the ones
 *     that are ON the ride (+0x62 & 0x80) at their own offset plus the two
 *     view-adjusted global offset pairs plus the square's screen origin, and
 *     push every rider of this square onto the ride's depth list.
 * The vehicle blit is therefore emitted TWICE, once in each arm. */

typedef struct SafariRec {
    RideTile           tile;     /* +0x00 the map square this copy sits on */
    unsigned short     pad02;
    int                seated;   /* +0x04 how many riders are seated */
    int                riders;   /* +0x08 how many are on the ride at all */
    int                frame;    /* +0x0c vehicle animation frame */
    struct SafariRec*  next;     /* +0x10 */
    unsigned char      pad14[0x0c];
    int                joined;   /* +0x20 how many have ever joined */
    int                timer;    /* +0x24 the machine's countdown */
} SafariRec;

/* The ride's depth sprite: +0x08 points at a slot holding the pointer to the
 * u16 frame the depth image is drawn from. */
typedef struct ZSprite {
    unsigned char    pad00[8];
    unsigned short** pframe;     /* +0x08 */
} ZSprite;

extern void   AdjustBlokePosition(Offset* p);                /* 0x00442d60 */
extern void*  GetSpriteForLayer(void* sprite, int layer);    /* 0x00441ec0 */

extern ZSprite*   g_safari_zspr_obj;                         /* 0x0082c66c */
extern Offset     g_safari_ofs2;                             /* 0x0082c670 (+4 = oy) */
extern RenderList g_safari_list;                             /* 0x004cbecc */

/* CLOSED (166/166). The last four mismatches were the operand order of the two
 * four-term screen sums: the original adds zofs (the HIGHER frame slot, 0x18)
 * before ofs (0x10); every scalar spelling adds ofs first. The lever is a
 * WHOLE-STRUCT COPY for zofs: `zofs = g_safari_ofs2;` (the 0x82c670/0x82c674
 * pair declared as one Offset). Why it works: VC6 ranks the operands of a
 * commutative sum by the creation order of their back-end symbols, LATEST
 * FIRST, and the frame slot by the same order (later-created = lower slot).
 * With scalar field stores `zofs.ox = ..; zofs.oy = ..;` the member symbols
 * zofs.ox/zofs.oy are created at the store (before ofs's), so zofs ranks
 * earlier: higher slot AND last in the sum. A struct copy creates only the
 * AGGREGATE symbol at that point (which still fixes the higher slot) and the
 * member symbols are first created inside the sum, after ofs's, so they sort
 * first. Measured: scalar stores in either order, every permutation of both
 * sums, function-level/block/inner-block declarations (20 variants) all inert;
 * `zofs = g_safari_ofs2;` alone closes it (copying ofs too is also exact).
 * The same lever should close joust.c's TempleSlide_Draw (116/120), which
 * carries the same residual. */
// FUNCTION: LEGOLAND 0x00414b80
void SafariRide_Interact(RideElem* elem, int x, int y, RideTile* sq,
                         void* clip, int mode)
{
    RideDef*   item = elem->data;
    RiderNode* r;
    SafariRec* rec;
    Offset     screen;
    char       any;

    r = item->riders;
    any = 0;
    rec = SafariRide_FindRecord(sq);
    if (rec) {
        RenderItems_New();
        g_safari_list.head = 0;
        screen = GetScreenCoordsForObject(sq, item);
        while (r) {
            if (sq->key == r->ride_id) {
                any = 1;
                break;
            }
            r = r->next;
        }
        **g_safari_zspr_obj->pframe = (unsigned short)rec->frame;
        if (any) {
            LLSSetFrame(GetLLSForLayer(g_safari_layers, 0), rec->frame);
            PrintSprite(GetSpriteForLayer(g_safari_layers, 0),
                        screen.ox, screen.oy, mode, 0);
            r = item->riders;
            while (r) {
                if (sq->key == r->ride_id) {
                    Bloke* b = r->bloke;

                    if (b->flags & 0x80) {
                        Person3D* p;
                        Offset    ofs;
                        Offset    zofs;

                        zofs = g_safari_ofs2;      /* whole-struct copy: see note */
                        p = b->person;
                        ofs.ox = g_safari_rider_dx;
                        ofs.oy = g_safari_rider_dy;
                        AdjustOffsetForViewMode(&ofs);
                        p->local.ox = b->ride_dx + ofs.ox;
                        p->local.oy = b->ride_dy + ofs.oy;
                        AdjustBlokePosition(&p->local);
                        AdjustOffsetForViewMode(&zofs);
                        p->screen.ox = b->ride_dx + zofs.ox + ofs.ox + screen.ox;
                        p->screen.oy = b->ride_dy + zofs.oy + ofs.oy + screen.oy;
                        AdjustBlokePosition(&p->screen);
                    }
                    AddBlokeToRenderList(&g_safari_list, r, r->owner->key);
                }
                r = r->next;
            }
        } else {
            LLSSetFrame(GetLLSForLayer(g_safari_layers, 0), rec->frame);
            PrintSprite(GetSpriteForLayer(g_safari_layers, 0),
                        screen.ox, screen.oy, mode, 0);
        }
        RenderBlokeList(&g_safari_list);
    }
}

/* ==========================================================================
 * +0xa8 -- THE PER-TICK UPDATE (ACTIVATE)
 * ==========================================================================
 * SPACE TOWER. One call per frame per class: first advance the MACHINE (the
 * cars' own lift cycle, 0x0043baa0, which owns the record list), then walk the
 * class rider list and give every rider whose low-level AI is idle
 * (Bloke+0x0e == 0) one step of its state machine (Bloke+0x60).
 *
 * THE SPACE TOWER STATE MACHINE (Bloke+0x60)
 *   0  JOIN: mark "on this ride" (+0x62 bit 8), bump the record's rider
 *      counter, arm its 200-frame timer, claim a seat (0x0043ac70), then walk
 *      to the tile directly below the ride's base square.
 *   1  clear the two animation words the sit pose uses, then step on.
 *   2, 7  hand the rider to 0x0043a8c0 (the queue/step helper) and stay put --
 *      the two steps that wait for the car.
 *   3  BOARD: walk to this seat's own offset from the square (the 8-byte
 *      {dx,dy} table at 0x004b77e8, one entry per seat) and face the direction
 *      the seat's car row names (0x004b77a8, 20-byte rows, indexed seat/2).
 *   4  SIT: mark "riding" (+0x62 bit 0x80), switch to the sit animation on
 *      frame 0, drop the BNV path, count the rider into the record and, when
 *      the count reaches the class capacity (ObjDef+0x2e), tell the machine it
 *      is full (0x0043aa90) so the car can leave.
 *   5  idle (the step the machine parks a rider on while the car is moving).
 *   6  ALIGHT: stop riding, free the seat slot (rec+0xa4+seat), back to the
 *      walk animation on frame 0.
 *   8  LEAVE: walk to the class's queue/exit square (ObjDef+0x24/+0x25),
 *      centred in the tile (+0x80).
 *   9  DONE: take the rider off the class list, clear "on this ride", and when
 *      the record's last rider has gone, reset its counter and let people on
 *      again.
 * A rider whose square has NO record breaks the whole walk (not just its own
 * step) -- reproduced.
 * ========================================================================== */

typedef struct TowerRec {
    RideTile          tile;      /* +0x00 the map square this copy sits on */
    unsigned char     seated;    /* +0x02 how many riders are sitting */
    unsigned char     riders;    /* +0x03 how many are on the ride at all */
    unsigned char     joined;    /* +0x04 how many have ever joined */
    unsigned char     pad05[0xa4 - 5];
    unsigned char     seat[8];   /* +0xa4 one slot per seat, 0 = free */
    signed char       frame3;    /* +0xac animation frame of sprite layer 3 */
    signed char       frame5;    /* +0xad animation frame of sprite layer 5 */
    unsigned char     padae[2];
    int               timer;     /* +0xb0 */
} TowerRec;

typedef struct SeatOfs {
    int dx;                      /* +0x00 */
    int dy;                      /* +0x04 */
} SeatOfs;

typedef struct CarRow {
    int dir;                     /* +0x00 the facing the seat's car needs */
    int f04;
    int f08;
    int f0c;
    int f10;
} CarRow;

extern int   CalcMoveLine(Pos from, Pos to, void* path);     /* 0x00480740 */
extern int   NewDirForAction(Bloke* b, unsigned char dir);   /* 0x004833d0 */
extern void  BlokeSitAnim(Bloke* b);                         /* 0x00440780 */
extern void  BlokeSetFrame(Bloke* b, int frame);             /* 0x00440870 */
extern void  BlokeWalkAnim(Bloke* b);                        /* 0x00440910 */
extern void  RemoveBlokeFromRide(RideDef* item, RiderNode* r);/* 0x0048a100 */
extern void  Ride_ClearFlagToNotLetAnyoneOn(RideTile* tile); /* 0x00443000 */

extern void  SpaceTower_TickMachine(void);                   /* 0x0043baa0 */
extern void  SpaceTower_TakeSeat(RiderNode* r, RideTile* t); /* 0x0043ac70 */
extern void  SpaceTower_Queue(RiderNode* r);                 /* 0x0043a8c0 */
extern void  SpaceTower_CountSeated(TowerRec* rec);          /* 0x0043a9b0 */
extern void  SpaceTower_SetFull(TowerRec* rec);              /* 0x0043aa90 */

/* The rider's map square IS the two bytes at RiderNode+0x0c; the original
 * rematerialises that address at every use rather than keeping a pointer, so
 * it must be spelled inline (a `tile` local coalesces into the cursor's
 * register and costs the whole allocation). */
#define RIDE_TILE(r) ((RideTile*)&(r)->ride_id)

extern SeatOfs g_tower_seat[8];                              /* 0x004b77e8 */
extern CarRow  g_tower_car[4];                               /* 0x004b77a8 */

/* STATE THIS ROUND: all 222 instructions and the whole block layout (the
 * 10-entry jump table, the shared CalcMoveLine/NewDirForAction tail cases 3
 * and 8 jump into, the two cases sharing one body, the break on a missing
 * record) are reproduced, and the semantics above are certain. Two frame facts
 * are not:
 *   - the original's local area is 0x10 bytes laid out [entry-0x10 tile x]
 *     [entry-0x0c next][entry-0x08 UNUSED][entry-0x04 ride row]; ours is 0x0c
 *     with no hole, because VC6 here colours the tile-x spill onto the unused
 *     first half of the row pair. Declaring the pair as a Pos, as a two-int
 *     array, block-scope, function-level, with or without a spare local -- all
 *     six measured -- give the identical 0x0c frame.
 *   - the rider cursor lands in ebp where the original has ebx (and the
 *     rematerialised map-square lea the other way round).
 * Both shift every displacement and register name, so the strict gate rejects
 * it. The instruction stream itself is right.
 *
 * ROUND 2 result on the hole, and it is a general rule worth keeping: VC6
 * reserves frame slots for an array local starting at the LOWEST INDEX THE
 * CODE ACTUALLY REFERENCES and drops the unused leading elements, so `int
 * base[2]` used only as base[1] can never leave a hole BELOW the used field --
 * base[2] gives `sub esp,0xc`, base[3] gives 0x10, base[4] gives 0x14, and the
 * eighteen combinations of {base[2..4]} x {all six declaration orders of
 * tilex/next/base} confirm declaration order is irrelevant. (`int base[3]`
 * does reproduce the original's `sub esp,0x10` and moves the first divergence
 * from index 0 to index 9, but it puts the spare slot ABOVE the row where the
 * original has it BELOW, and there is no semantic reading of a three-int local
 * here, so it is not committed.) The hole at entry-0x08 is therefore NOT a
 * dead array element: it is the home of some other local of the original that
 * this reconstruction does not have -- most likely one whose only stores were
 * eliminated, since nothing in the body ever references that displacement.
 * The ebx/ebp swap is the other half: the original gives ebx to the rider
 * cursor and ebp to the rematerialised map-square pointer, ours the reverse,
 * and the original reads the square's y through a REMATERIALISED address
 * (`lea eax,[ebx+0xc]` / `mov dl,[eax+1]`) where ours folds it to
 * `mov cl,[ebp+0xd]` -- the signature VC6 produces when the same pointer is
 * also handed to a call, which it is (SpaceTower_FindRecord). */
/* ROUND 3 (audit 169, unchanged; ~40 measured variants, ranked by a
 * register-blind difflib alignment as well as the strict count):
 *   - The block layout is NOT fully reproduced: the original lays the shared
 *     `case 2: case 7:` (SpaceTower_Queue) block AFTER case 6, before case 8;
 *     ours puts it after case 1. Moving the two labels to just before `case
 *     8:` reproduces the layout exactly (Queue block at index 161) but by
 *     itself leaves the strict count at 177 because everything else is a
 *     register rename.
 *   - The original's register picture at the switch: eax = the CSE'd
 *     `&r->tile` (rematerialised `lea eax,[ebx+0xc]` once and kept through
 *     the jump table -- case 0 pushes it, case 3 re-reads y through it),
 *     edx = tile.y kept to case 8, ebp = tx, tile.x SPILLED to 0x10 (there is
 *     no scratch register left), row spilled to 0x1c, def in the dead `elem`
 *     slot, and the hole at 0x18. Ours keeps tile.x in eax, folds y through
 *     the cursor, and never keeps the pointer.
 *   - `int tiley` as a named local gives the frame 0x10 and the cursor ebx
 *     (first divergence 0 -> 7, 130 strict) but spills tiley to the arg slot
 *     and puts def at 0x10. A named `RideTile* tile` coalesces into the
 *     cursor (`add ebx,0xc`) and spills r (238 instructions). Writing case 8
 *     with target.y before target.x keeps tiley in edx (and turns the
 *     `cmp word [esi+0xe],dx` back into the original's immediate compare) but
 *     stores y first; two temps or a Pos temp for case 8 restore the store
 *     order at +5 instructions. `unsigned seat` gives the original's `shr`.
 *     All eight permutations of the tx/row/tilex/tiley statements are inert.
 *   Copters_Activate's lever (a nested call written as a statement) does not
 *   apply: this function has no nested-call argument. */
/* ROUND 4 (audit 169 -> 149). Two changes, both structural:
 *   - The `case 2: case 7:` (SpaceTower_Queue) labels moved to just before
 *     `case 8:`, which is where the original lays that block (round 3 had
 *     measured the layout but not committed it because it cost 8 alone).
 *   - The frame hole at entry-0x08 is NOT an unknown local after all: the
 *     original's `base[2]` array sits at 0x08-0x0f (only [1] used), and the
 *     missing home is `tilex`, which the original SPILLS at its def and
 *     reloads at both uses.  Forcing that with a volatile READ at the case-8
 *     use (`(def->qx + *(volatile int*)&tilex)`) makes the frame 0x10 == 0x10
 *     and the whole prologue exact.  (The same volatile-read lever closed the
 *     rec spill in SpiderRide_Activate; a volatile STORE is folded away.)
 * WHAT IS LEFT (149): the ebx/ebp swap between the rider cursor and the
 * rematerialised map-square pointer, which cascades through the whole body,
 * and with it the original's CSE of `&r->tile` into eax across the jump table
 * (case 0 pushes it; ours re-does the `lea`).  Measured and inert now: all
 * six orders of the next/b/rec statements, a named `RideTile* tile` local for
 * the FindRecord and TakeSeat calls (identical code) or for every use (231),
 * `int tiley`, `if (b->state == 0)` vs `if (!b->state)`, and splitting the
 * base[1] sum.
 * ROUND 5 (unchanged at 149).  A second signature was isolated and then
 * failed to move: ours emits `xor edx,edx / cmp word ptr [esi+0xe], dx`
 * where the original has the IMMEDIATE `cmp word ptr [esi+0xe], 0`, i.e. we
 * grow a function-wide zero WEB whose def lands in the loop head and absorbs
 * the compare, while the original materialises its zero locally in case 1
 * (`xor eax,eax` right before the two 16-bit stores).  That register-vs-
 * immediate compare is also part of the 5-byte deficit.  Measured and inert:
 * `b->f4a`/`b->f38` instead of `r->bloke->`, either store order, a
 * `unsigned short z = 0;` carrier, an explicit `(unsigned short)0` cast, and
 * `b->state == 0` vs `!b->state`.  Also re-run in this state and still
 * inert: all six loop-head statement orders.  The stub-each-case diagnostic
 * (which cracked PlaneRide_Activate this round -- stub every case body out
 * in turn and watch which register a head value lands in) has NOT yet been
 * run on this function and is the obvious next step. */
/* ROUND 6 (2026-09-04, still 149 committed).  THE HEAD'S SOURCE SHAPE IS NOW
 * KNOWN and it is a named local:
 *     RideTile* tile = RIDE_TILE(r);
 *     rec = SpaceTower_FindRecord(tile);
 *     ... tilex = tile->b.x; ... base[1] = def->base_y + tile->b.y;
 * with `tile` used for case 0's SpaceTower_TakeSeat as well.  That ONE change
 * reproduces THREE independent signatures the expression form cannot:
 *   - the rematerialised `lea r,[r_cursor+0xc]` at index 25/30 (ours had
 *     folded the second tile access into `mov al,[r+0xd]`);
 *   - the CSE of that lea LIVE ACROSS THE JUMP TABLE into case 0, which
 *     pushes it (ours re-did the `lea` inside case 0);
 *   - the IMMEDIATE `cmp word ptr [esi+0xe], 0` -- the function-wide zero web
 *     described in round 5 simply disappears.
 * Register-blind it is far closer than the committed body (rb 70 -> 42,
 * shp 64 -> 46) but the STRICT count rises 149 -> 200, because the whole
 * body is then renamed by the SAME ebx/ebp swap that was already the
 * residual: the original puts the loop cursor `r` in ebx and tile/tx in ebp,
 * ours does the opposite.  Not committed for that reason.
 * The swap is not caused by anything local: the stub-each-case diagnostic
 * was run (every case body replaced by `break;` in turn) and `r` lands in
 * ebp in EVERY variant -- ebx never appears -- and it is equally unmoved by
 * deleting `tx`, deleting `base[1]`, deleting `tilex`, `int base[3]`,
 * `while (r != 0)`, `for (r = ..; r; r = next)`, all six orders of the
 * next/b/tile statements, `if (b->state == 0)`, and moving TickMachine.
 * The original also reloads the spilled `def` TWICE back to back (indices
 * 22/23) so that the second field load can be in place (`mov ecx,[ecx+0x10]`)
 * -- a register-pressure symptom we do not have.  `(*(RideDef* volatile*)&def)
 * ->base_y` buys exactly that second reload and takes the byte length to
 * 697/698 (from 693) but costs the immediate compare and measures 152.
 * MEASURED (2026-09-04): tile x {no vol, vol on base_x, on base_y, on both}
 * x {tx first, base[1] first} = 149/141/152/153/186/192/200/203; separate
 * `bx`/`by` locals 200; a `tiley` local 200; `unsigned char tilex` 150.
 * NOTE `base[1] = ...` written BEFORE the tilex/tx pair measures 141, i.e.
 * 8 BETTER than the committed body -- it is NOT committed because it is a
 * numeric accident: it loses the case-0 tile CSE and keeps the zero web, so
 * it is further from the original than what is here. */
/* ROUND 7 (2026-09-04, still 149).  The ebx/ebp swap was attacked from the
 * reference-count side and is now known to be a PRIORITY ORDERING, one place
 * off.  The four callee-saved webs are ranked b > rec > {tile,tx} > r in ours
 * and b > rec > r > {tile,tx} in the original (registers are handed out
 * esi, edi, ebx, ebp in that priority order).  Evidence that the ranking is
 * movable at all: promoting case 0's `b->flags |= 8;` to
 * `r->bloke->flags |= 8;` -- ONE extra reference to `r` -- puts `r` in EDI,
 * i.e. it jumps r ABOVE rec, overshooting the target by one place (209).
 * Nothing found lands on the intermediate rank.  Measured and inert (r stays
 * in ebp): 22 single-point b/r reference mutations (every `b->` line in the
 * switch promoted to `r->bloke->` in turn, and every `r->bloke->` demoted),
 * eight declaration-order permutations, the six loop-head orders RE-RUN with
 * the named `tile` local, `tilex` without the volatile read, `for(;;)` with
 * the test inside, case 5 deleted, an extra `rec` reference, `def` re-read as
 * `((RideDef*)elem->data)`, `unsigned seat`, `int base[3]`, and `tx` inlined
 * into case 0.  The named-`tile` head (round 6) re-measured in this state:
 * 207 strict but rb 116 -> 85 and shp 112 -> 89, still the truest shape and
 * still not committed. 
/* ROUND 8 (2026-09-04, audit 149 -> 138, byte length 693 -> 697 of 698, and
 * THE ebx/ebp SWAP IS FIXED -- `r` is now in ebx and the whole prologue plus
 * indices 0-22 are exact).  Round 6 had the head's shape right (a named
 * `tile` local) and round 7 blamed a reference-count ranking; both were half
 * the answer.  What actually moves the ranking is WHICH POINTER THE LATER
 * CASES READ THROUGH, and the winning combination is:
 *   1. `RideTile* tile = RIDE_TILE(r);` for the FindRecord call, `tilex`,
 *      `base[1]` and case 0's SpaceTower_TakeSeat (round 6's shape), AND
 *   2. `int tiley = tile->b.y;` as a NAMED local that cases 3 and 8 both
 *      consume.  The original keeps that byte-load's value in edx live
 *      across the jump table (case 8's `add ecx,edx` reads it), so it really
 *      is one CSE'd value and not two reads.  With `tile` alone (case 3 and
 *      case 8 re-reading through `r`) the strict count is 200 and `r` is
 *      still in ebp; adding `tiley` gives 163 and `r` in EBX.  The grid was
 *      measured: {tiley, tile->b.y, RIDE_TILE(r)->b.y} x {same} for cases 3
 *      and 8 = 138/211/169/212/210/148/153/153/160, and every cell that has
 *      case 3 reading `tile->b.y` ESCAPES (VC6 tail-duplicates the
 *      NewDirForAction tail).
 *   3. The original reloads the spilled `def` TWICE back to back (indices
 *      22/23) and reads base_x through the first and base_y through the
 *      second.  `(*(RideDef* volatile*)&def)->base_y` buys the second load
 *      and is worth 163 -> 138.  This is a SHIM: it produces the right
 *      instruction from the right slot but pins it LATE (ours emits it at
 *      index 31, the original at 23), which is the whole of what is left in
 *      the head.  Proof that the schedule and not the shim is the residual:
 *      spelling that second read `g_spacetower_def->base_y` (a genuinely
 *      different memory reference, so no CSE) puts the load at index 23 and
 *      makes indices 22-29 and 34-41 exact at 132 strict -- but the original
 *      plainly reads [esp+0x24] there, not the global, so it is NOT
 *      committed.  Everything that would reload `def` without a barrier was
 *      measured and CSE'd away: a second `def2 = elem->data` local (163),
 *      `((RideDef*)elem->data)->base_y` (195), `(int)(unsigned)`, a volatile
 *      on the FIELD rather than the pointer, `RideDef* volatile def` (177),
 *      and a `static __inline` helper taking `&def` (136 strict but rb 208
 *      -> 195 and bad 14 -> 27: a compensating error, rejected).
 *   4. `unsigned seat` (round 3 had measured this and not committed it):
 *      strict- and byte-neutral, and it turns our `sar edi,1` into the
 *      original's `shr edi,1`.  Committed because it is right, not because
 *      it scores.
 * WHAT IS LEFT (138): (a) the head's second `def` load is scheduled at 31
 * instead of 23, which swaps eax/edx for the rematerialised `tile` pointer
 * and `tiley` at indices 30-33; (b) case 3 (indices 89-121) is a two-register
 * rotation plus the original's re-read of `tile->b.y` through the pointer
 * still live in eax -- because the original's dir byte lands in ECX there and
 * ours in AL, the original's `push ecx` cannot merge into the shared
 * NewDirForAction tail and ours does, which is the 3-instruction offset that
 * runs to the end of the body; (c) from index 122 on the body is a pure
 * 3-slot shift with no register differences at all.  Re-measured and inert in
 * this state: base[3], all the r/b reference mutations from round 7, the
 * volatile read on tilex moved to case 3, and a volatile read of tiley. */
/* ROUND 8b: the whole 138 now reduces to ONE tie-break, traced end to end.
 * At the head's index 30 the original puts the rematerialised `tile` pointer
 * in EAX and `tiley` in EDX; ours does the reverse.  Everything else follows:
 * with `tile` in eax the original's case 3 must reload `tiley` through it
 * (`xor edx,edx / mov dl,[eax+1]`, +2 instructions we do not emit) and must
 * index the car table through eax (`lea eax,[edi+edi*4] / mov cl,[eax*4+..]`),
 * which puts case 3's dir byte in CL where ours is in AL -- and because ours
 * matches case 8's AL, our `push` merges into the shared NewDirForAction tail
 * where the original's `push ecx` cannot (+1).  Those three instructions are
 * the whole 3-slot offset that makes indices 122-221 differ.  It is NOT a
 * free-register question: `g_spacetower_def->base_y` reproduces the original's
 * register PRESSURE at index 30 exactly (ecx busy from 23, only eax and edx
 * free) and STILL picks edx for the lea, so the choice is the order of the two
 * values in the allocator's list, not availability.  Measured and inert in
 * this state: all six loop-head orders, six random declaration-order
 * permutations, `base[1] = tiley + def->base_y` (operand order), `tiley` as
 * unsigned int, a `dir` local or a `car` index local in case 3,
 * `SpaceTower_TakeSeat(r, RIDE_TILE(r))` (now byte-identical to `tile`, so
 * case 0's argument spelling has stopped mattering), FindRecord taking
 * `RIDE_TILE(r)`, and `tilex` read through `RIDE_TILE(r)`.  Worse: `tiley` as
 * char/uchar/short (166-197), read as `tile->key >> 8` (194), a whole-union
 * `RideTile t = *tile;` copy (168), `tiley` read through `RIDE_TILE(r)` (157),
 * and `tiley` computed before `tx` (157). */
/* ROUND 8c: re-checked with scratchpad/laneK/permrank.py (see the ROUND 8c
 * paragraph in SpinningBarrels_Activate).  The committed body is real=136
 * against real=138 for the pre-round-8 body, so the change is a real
 * improvement but the strict 149 -> 138 OVERSTATED it: eleven of those were a
 * register renaming.  Full ranking of this round's candidates:
 *   c3R_c8Y no-voldef 103 | c3Y_c8Y voldef (COMMITTED) 136 | old body 138
 *   | c3T_c8R 146 | c3R_c8Y voldef 151 | c3R_c8R 152 | c3Y_c8Y no-voldef 159
 *   | c3Y_c8R 169 | c3T_c8R no-voldef 190 | c3T_c8Y 210
 * `c3R_c8Y` with no volatile def read scores real=103 with the IDENTITY
 * permutation -- by far the best number in the file -- and its case split
 * (case 3 RE-READS `tile->b.y`, case 8 consumes the CSE'd `tiley`) is exactly
 * what the disassembly shows.  It is NOT committed because it spills `tiley`
 * and takes the frame to `sub esp,0x14` where the original has 0x10; its bad
 * list opens [0, 1, 7, 15], the prologue.  That makes the remaining task
 * precise: find the spelling that keeps `tiley` in edx across the jump table
 * while case 3 re-reads it through the pointer in eax. */
/* ROUND 9 (2026-09-04, unchanged at 138 / real 136).  The index-30 tie-break
 * survived ~90 further spellings and they all reduce to one cause, now stated
 * exactly: the SECOND `def` reload.  The original's back-to-back
 * `mov edx,[esp+0x24]` / `mov ecx,[esp+0x24]` lets edx's copy DIE at
 * `mov ebp,[edx+0xc]` (index 26), which is what leaves edx free for `tiley`
 * and hands eax to the `lea` that rematerialises `tile`; ours keeps ONE copy
 * live across both field reads, so the `lea` takes edx and `tiley` reuses the
 * eax that `tx` has just freed.  Measured and all CSE'd back to a single
 * load: a one-member `struct { RideDef* p; } dd;` (a one-pointer aggregate is
 * NOT exempt from enregistration), `RideDef** dp = &def;` with `(*dp)->`,
 * `(*(RideDef**)&def)`, a `def2 = def` copy, and the volatile read moved to
 * base_x or applied to both.  Splitting the volatile read into its own
 * statement (`d2 = *(RideDef* volatile*)&def;` and
 * `by = (*(RideDef* volatile*)&def)->base_y;`) at each of the four head seams
 * does not pin it at index 23 either (145-192); `elem->data` spelled at every
 * use instead of a `def` local is 213 (195 with the shim).  The head's
 * statement-order grid (six legal orders x four volatile placements) is
 * 138-157, and empty-`if` block splits at all four head seams are byte-
 * identical.  NEGATIVE worth recording because it frees a degree of freedom:
 * spelling `tx` as `base[0]`, so the pair really is one 2-int array, is
 * BYTE-IDENTICAL to the separate `tx` local in every combination of voldef
 * and case-3 spelling -- the `int base[2]` frame object is right either way.
 * `c3R_c8Y` with no volatile def read still measures real=103 strict=103
 * under the IDENTITY permutation and is still NOT committed: its frame is
 * `sub esp,0x14` against the original's 0x10 (`tiley` is spilled at its
 * definition, index 31, and reloaded at index 33), and its case 8 -- indices
 * 164-199, which read `tiley` from that slot instead of from edx -- is then
 * wholly wrong, where the committed body's tail from 122 on is a pure 3-slot
 * shift with no register differences. */
/* PASS w8rides (2026-09-04).  NO CHANGE (138 strict / 136 real /
 * register-blind 14).  Confirmations and one newly closed door:
 *  - The first divergence is still index 23, still the SECOND back-to-back
 *    `mov ecx,[esp+0x24]`.  Read against ours the whole head difference is
 *    just this: the original reloads `def` TWICE up front (edx at 22, ecx at
 *    23) so edx dies at `mov ebp,[edx+0xc]` and is free for `tiley`, leaving
 *    eax for the `lea` that rematerialises `tile`; ours reloads `def` once at
 *    22 and once at 31, so the `lea` takes edx and `tiley` takes eax.  BOTH
 *    builds emit two reloads -- the difference is only where the second one
 *    lands, and the ROUND-9 volatile experiments already showed the
 *    allocation is fixed before the schedule.
 *  - NEWLY MEASURED: the case-3 / case-8 `tiley` grid re-run with the
 *    re-read spelled `tile->b.y` rather than `RIDE_TILE(r)->b.y`.  c3R with
 *    voldef 212, c3R without 213, c3R+c8R 214, c3Y+c8R 211, and all four
 *    ESCAPE the extent at 704-711 bytes against 698.  Dropping the volatile
 *    `def` read alone is 163 (register-blind 15).  So the committed
 *    combination remains the only one that holds both the frame and the byte
 *    length, and the promising `c3R_c8Y no-voldef` 103 recorded in ROUND 8c
 *    is reachable ONLY through the `RIDE_TILE(r)` spelling -- it is not a
 *    property of re-reading the field. */
/* PASS w9rides (2026-09-04).  NO CHANGE (138 strict / 136 real /
 * register-blind 14).  This body is NOT part of the five-function slot
 * family; its residual is the index-23 `def` reload, and the open task from
 * ROUND 8c -- hold `sub esp,0x10` while keeping the `c3R_c8Y no-voldef`
 * route -- was attacked directly and is now much better bounded.
 *
 * WHAT THE HEAD ACTUALLY NEEDS, read off the disassembly:
 *      orig 22 mov edx,[esp+0x24]   23 mov ecx,[esp+0x24]   (def, TWICE)
 *           26 mov ebp,[edx+0xc]    <- edx DIES here
 *           27 mov ecx,[ecx+0x10]   <- base_y in place
 *           30 lea eax,[ebx+0xc]    31 xor edx,edx  32 mov dl,[eax+1]
 *      ours 22 mov edx,[esp+0x28]   (def, ONCE, live to 32)
 *           25 lea ecx,[ebx+0xc]    30 mov al,[ecx+1]  31 mov [esp+0x14],eax
 * The second reload is the whole thing: it is what frees edx for `tiley`
 * and hands eax to the `lea` that rematerialises `tile`.  With only one
 * reload, edx stays busy, `tiley` takes eax and VC6 spills it -- which is
 * the `sub esp,0x14` and the 695/698 bytes.
 *
 * THE `c3R_c8Y no-voldef` ROUTE (real 103 / strict 103, IDENTITY) IS NOW
 * SHOWN INVARIANT under twenty further spellings, all of them exactly 103
 * with the identical bad list -- so the spill is not a spelling either:
 *   - the `def`-reload family CROSSED WITH c3R for the first time (it had
 *     only ever been swept in the committed c3Y state): a volatile pointer
 *     read on base_x, on base_y, on both; `((RideDef*)elem->data)` on either
 *     read; a second `def2 = elem->data` local used for either read, with
 *     and without a volatile on it.  Every non-volatile one is CSE'd back to
 *     a single load (103), every volatile one pins the reload late (151-211).
 *   - empty-`if` block splits at all three head seams with six different
 *     guard values (tx, tilex, tiley, def, b, rec), and two block scopes
 *     around the x pair and the y pair: byte-identical.  So the empty `if`
 *     does NOT give the two field reads separate basic blocks here.
 *   - six accumulate spellings (`base[1] = def->base_y; base[1] += tiley;`,
 *     the same for `tx`, both, y-pair first, tiley computed last): 103 for
 *     the four that keep the head order, 145-195 otherwise.
 * NOT COMMITTED, and the reason is now the standing test rather than taste:
 * strict falls 138 -> 103 but REGISTER-BLIND RISES 14 -> 21 and the frame
 * goes to 0x14 against the original's 0x10 -- the compensating-error
 * signature.  The committed body's tail from index 122 is a pure 3-slot
 * shift with no register differences; the 103 body's case 8 reads `tiley`
 * from a stack slot the original never has.  Flagged for the coordinator:
 * if a future pass finds a way to keep `tiley` in edx (i.e. a SECOND `def`
 * reload at index 23 without a volatile), the two changes go in together
 * and this function should fall a long way. */
/* PASS w10rides (2026-09-05).  NO CHANGE (138 strict / 136 real /
 * register-blind 14).  The open task from w9 -- a NON-VOLATILE second `def`
 * reload at index 23 -- was attacked from three new directions and all three
 * fail; one of them also DISPROVES the mechanism the w9 note attributed the
 * eax/edx tie-break to.
 *
 *  (1) THE SECOND RELOAD IS NOT SUFFICIENT.  `a27_volboth`
 *      (`(*(RideDef* volatile*)&def)->base_x` AND `...->base_y`) emits the
 *      two reloads and lets EDX die at `mov ebp,[edx+0xc]` exactly as the
 *      original does -- and STILL puts the rematerialised `tile` in EDX and
 *      `tiley` in EAX (136/138).  So "edx dies at 26 and is therefore free
 *      for tiley" is not the tie-break; the free set at the `lea` is
 *      {eax, edx} in the original and {eax, ecx, edx} in ours, and what
 *      differs is that in the original ECX IS STILL BUSY holding `base_y`,
 *      because the original evaluates `def->base_y` BEFORE the `tile->b.y`
 *      byte load.
 *
 *  (2) BUT THAT ORDER IS NOT SPELLABLE.  Twelve head spellings that put the
 *      `base_y` read ahead of the `tiley` read -- `base[1] = def->base_y;`
 *      then `base[1] += tiley;`, a named `by` local, `by` before `tx`, `by`
 *      first of all, both operand orders, each crossed with the volatile and
 *      with case 3's spelling -- are BYTE-IDENTICAL to the plain form in
 *      every non-volatile cell (159/163).  VC6 normalises the head back to
 *      one shape before it allocates.
 *
 *  (3) THE `tiley` LOCAL MAY NOT EXIST IN THE ORIGINAL, BUT SPELLING IT AWAY
 *      IS WORSE.  Read off the disassembly: the original keeps `tile` (the
 *      `lea eax,[ebx+0xc]` at index 30) live in EAX across the jump table --
 *      case 0 pushes it at 43, case 3 RE-READS the byte through it at 98
 *      (`xor edx,edx / mov dl,[eax+1]`) because case 3 has just clobbered
 *      edx for its x computation -- while the head's byte load stays in EDX
 *      and case 8 consumes it there (`add ecx,edx`, index 170).  That is
 *      exactly the signature of ONE CSE of `tile->b.y` with no local at all.
 *      Measured: the full {head, case 3, case 8} x {`tilex`/`tiley` local,
 *      `tile->b.x`/`tile->b.y`, `RIDE_TILE(r)->..`, the volatile `tilex`} =
 *      70 cells.  Every cell in which case 3 reads `tile->b.y` ESCAPES the
 *      extent (207-214 at 704-711 bytes against 698) -- VC6 tail-duplicates
 *      the NewDirForAction tail -- and dropping the `tiley` local entirely is
 *      207+.  The committed pair (`tiley` in the head and case 8, the
 *      volatile `def` read) is still the only combination that holds both
 *      `sub esp,0x10` and 698 bytes.
 *
 *  (4) THE INDEX-30 TIE-BREAK IS REACHABLE AFTER ALL, AND THE OPEN TASK IS
 *      NOW A DIFFERENT ONE.  Evaluating `def->base_y` into a NAMED local
 *      BEFORE the `tile->b.y` read, with the volatile second reload on that
 *      read, puts the `lea eax,[ebx+0xc]` at index 30 and `tiley` in EDX --
 *      the original's assignment, and the first time any spelling has done
 *      it:
 *          by = (*(RideDef* volatile*)&def)->base_y;   (at any of the three
 *          head seams)                                  ... 145/146
 *          base[1] = (*(RideDef* volatile*)&def)->base_y;
 *          base[1] += tiley;                            ... 151
 *      What it costs is a FIFTH frame slot: `sub esp,0x14` against 0x10, 705
 *      bytes against 698.  VC6 loads `base_y` into EDX (not the free ECX the
 *      original uses) and then has to spill it at index 31 to make room for
 *      the `xor edx,edx` of the byte load -- so the intermediate always
 *      acquires a home, whether it is a named `by`, `base[1]` accumulated in
 *      two statements, or a separately hoisted `d2` pointer (nine cells
 *      measured, 145-170; every non-volatile one collapses back to 163).
 *      SO THE OPEN TASK IS NO LONGER "a non-volatile second `def` reload":
 *      it is "get `def->base_y` into ECX rather than EDX at index 27".  With
 *      that, the head's twelve instructions and the whole eax/edx cascade
 *      through cases 0, 3 and 8 come with it.
 *
 *  Also re-confirmed inert: `(void)&def;` and `if (&def) { }` (VC6 folds them
 *  away, so neither makes `def` address-taken), `def = def;`, and a
 *  `((RideDef*)((char*)def))` cast -- all 163, i.e. one reload. */
/* Scope F (2026-09-05): RECONSTRUCTION ERROR FIXED in case 6. The
 * original at 0x43bc9d clears [edi+edx+0xa4], matching TowerRec.seat; the
 * old byte-pointer spelling wrote rec+0x20 instead. Use rec->seat[b->seat].
 * This repairs behavior even though index-aligned mismatch stays 138,
 * first 23. The real compiled body is 220i/696B versus 222i/698B, not the
 * equal-length body earlier notes claimed: audit's 222i/703B includes two
 * decodes of trailing jump-table data after the ret. Before the fix, the
 * real body was 220i/693B; the wrong short displacement hid three bytes.
 * Named base-y and def pointers give 162 and 169; grouping by/tx gives 151
 * in either member order. Rechecked after the seat fix; none improves the
 * head or removes the extra frame slot of the pair forms. Keep the existing
 * allocation at its current tested floor, with the seat correction retained.
 * Whole-file audit keeps all 43 exact functions. See docs/lanes/scope-f.md. */
/* Scope F continuation (2026-09-06): still 138 strict, first 23; real body
 * 220i/696B (audit decodes two trailing jump-table entries and reports the
 * original as 222i/698B).  Register-blind the residual is only THIRTEEN
 * instructions: the position of the second `def` load, case 3's missing
 * `xor edx,edx / mov dl,[eax+1]` plus one extra push, and a case-8 store
 * permutation.  Case 0 (47-88) is already exact.  ~1200 spellings measured.
 *
 * TWO EARLIER CLAIMS IN THIS NOTE WERE WRONG AND ARE WITHDRAWN:
 *   - It is NOT true that a plain head collapses to one `def` load.  The
 *     plain head emits TWO (indices 22 and 30) and the volatile head emits
 *     two (22 and 31); the ORIGINAL's two are adjacent at 22 and 23.  The
 *     number of loads was never the problem, only where they land -- always
 *     print their indices rather than counting them.
 *   - "A direct case-3 field read scores 188" holds only for the `tile->b.y`
 *     spelling (222i/701B, no escape).  `RIDE_TILE(r)->b.y` is a different
 *     lever: it folds to `[ebx+0xd]`, keeps the extent and reaches strict
 *     100-102 -- but only by paying a FIFTH frame slot (`sub esp,0x14`),
 *     which renames the prologue, every displacement and the whole tail.
 *     That is a compensating error of the same class earlier sessions
 *     rejected at 103, and it raises the register-blind residual 13 -> 26.
 *     It must not be landed.  `tile->b.y` in case 3 makes VC6 tail-duplicate
 *     the shared CalcMoveLine/NewDirForAction tail and ESCAPES the extent in
 *     every other crossing (707-716B against 698).
 *
 * THE BLOCKER, isolated.  At indices 30-33 the original holds exactly seven
 * live values (r, b, rec, tx, tile, tiley, base_y) in seven registers,
 * because both `def` reloads are consumed by index 27 and `def` is then
 * dead.  Every spelling of ours keeps `def` live to index 32/33, making
 * eight, and VC6 spills whichever of {def, tiley} loses a fixed tie-break.
 * This is NOT reference counting and NOT pressure elsewhere: stubbing out
 * `def`'s case-8 and case-9 uses still spills `tiley` (152), and deleting
 * `tx` from the head entirely still spills it (141).  Only shortening
 * `def`'s live range inside the head itself moves it.
 *
 * THE ONE CONSTRUCT THAT REACHES THE ORIGINAL'S 22/23 PAIR is two separate
 * volatile pointer reads into two distinct named locals:
 *     d1 = *(RideDef* volatile*)&def;
 *     d2 = *(RideDef* volatile*)&def;
 *     ... tx = d1->base_x + tilex;  base[1] = d2->base_y + tiley;
 * One volatile read is pinned after the nearest store and cannot cross it;
 * two, each into its own local, both hoist and land adjacent.  That gives
 * `sub esp,0x10`, the `lea` at 30, the byte read through it at 32, the add
 * at 33 and the store at 35 -- the original's whole head shape, index 22
 * byte-exact, register-blind 13 -> 12 (-> 7 with the case-8 change below).
 * What remains is a fixed THREE-WEB ROTATION: ours puts {def2, base_y,
 * base[1]} in EAX where the original uses ECX, `tilex` in ECX where the
 * original uses EAX, and tile/tiley in EDX/ECX where the original uses
 * EAX/EDX.  Thirteen rotation levers (declaration order and position, read
 * order, use swap, a third volatile read, an extra IR temporary, operand
 * order, const, unsigned tilex/tiley, an empty if between the reads, tx as
 * base[0], int base[3], a spare local, split tx accumulation) are all
 * BYTE-IDENTICAL at 152.  Reading d2 first flips tiley into EDX correctly
 * but exchanges both loads' roles (153).
 *
 * PAIRED CHANGE, worth ~6 instructions but only once the head is right:
 * spelling case 8 as two named sums computed BEFORE either store aligns its
 * whole tail (register-blind 13 -> 11 on this head, 12 -> 7 on the two-load
 * head).  It costs strict on the current allocation, so it is recorded, not
 * landed.  Head statement order is fully normalised before allocation --
 * nine legal orders are byte-identical; do not re-sweep it.  Spellings that
 * do NOT split the local's web, all byte-identical here: a one- or
 * two-element RideDef* array, a block-scope copy, a plain second copy, an
 * `int*` field indirection, ((int*)def)[3]/[4], reusing the parameter, and
 * re-assigning def inside the loop.
 *
 * INDEPENDENTLY RE-SWEPT (same day, 18 further variants on the two-load
 * head): swapping which local feeds which field, moving either read past
 * the tile-x read or past the x sum, computing the y sum first or last,
 * a third volatile read, an extra IR temporary before or between the two
 * reads, naming the base-y load, both sum operand orders, explicit unsigned
 * byte casts, and a plain scalar in place of base[1] -- ALL byte-identical
 * at 152 (or 153-171 where they change the loads' roles).  Note the
 * two-load head is NOT merely a register swap away: its register-blind
 * alignment is 7, so seven instructions differ even ignoring register
 * names, and it is 3 bytes short of the original's real 696.
 *
 * The committed body below is the best STRICT result found (138) that also
 * holds the original's frame and real length; it is retained deliberately
 * over the 100/102 candidates, which win on strict only by spilling. */
/* Scope F fourteenth continuation (2026-09-06): 138 -> 136 strict;
 * first 25, real 220i/693B against the ORIGINAL's 222i/698B. The 696B
 * figure above describes the PREVIOUS compiled body, not the original.
 * No fifth frame slot: sub esp,0x10 is retained. Still WIP.
 *
 * The two separate volatile def reads from the preceding continuation,
 * combined with a NAMED DESTINATION POINTER for the base-y sum, break the
 * remaining head register rotation without a volatile sum store:
 *     int* dst = &base[1]; *dst = d2->base_y + tiley;
 * Together with the two named, unscaled case-8 sums, this makes indices
 * 22-24 and 28-88 exact. EDX/ECX are the two def reloads; tile is EAX,
 * tiley is EDX, base-y is ECX. Case 0 is now exact in full. The only head
 * difference is a three-instruction permutation at 25-27: base-y loads
 * before tile-x/base-x. A volatile sum store gives the same register roles
 * but exchanges the state compare and sum store and changes case 0: 141.
 *
 * This is a real allocation improvement, not a close or an extent trick.
 * On RET-bounded bodies, aligned strict edit count falls 68 -> 18, and
 * aligned register-blind count 22 -> 11; index-aligned strict/rb/ob is now
 * 136/123/136 (previously 138/120/138). Do not equate these metrics. Audit
 * still takes 222 instructions, including two trailing decodes (697B).
 *
 * REMAINING: case 3 needs its tile-y re-read after the target-x store, plus
 * original seat/x register choices and earlier seat shift; case 8 retains
 * two load-order differences. Direct tile->b.y introduces an extra cursor
 * home and rewrites the loop around the derived tile address; RIDE_TILE(r)
 * instead retains r but spills other head values. Pointer/cursor copies,
 * aggregate/type/lifetime variants, explicit reloads, loop/exit shapes,
 * free field barriers, paired coordinates, shared-direction labels and
 * two-axis fill loops do not recover both original frame and reload.
 * Full-file audit preserves all 47 exact neighbors. Detailed evidence and
 * rejected lower-index-score candidates: docs/lanes/scope-f.md. */
/* Scope F fifteenth continuation (2026-09-06): 136 -> 34 strict;
 * first 22, real 222i/698B, original 16-byte frame. Still WIP.
 *
 * Case 3 now re-reads the tile-y byte AFTER the target-x store, as the
 * original does. Initialize a case-local xx with a volatile read of the
 * existing tilex home BEFORE reading seat, and keep the case-8 read shim:
 *     int xx = *(volatile int*)&tilex;
 * Use xx for target-x and RIDE_TILE(r)->b.y for target-y. Both volatile
 * reads are needed: moving the only shim, copying tilex plainly, or using
 * tile->b.y instead loses the original frame or reintroduces spills.
 * This is the first checkpoint whose actual RET-bounded body is 222/698;
 * the audit no longer includes trailing data. All 47 exact neighbors pass.
 *
 * REMAINING: seven head differences at 22-29; case-3 register allocation,
 * scheduling and earlier seat shift; four case-8 load-order differences.
 * The original keeps tile in EAX and reloads xx into EBX; ours folds the
 * y read into [r+0x0d] and uses EAX for xx. Case-local pointer copies fold
 * back to r; empty pointer consumers instead keep the tile but spill r.
 * Indices 30-88, 118-164 and 169-221 are exact. Index strict/rb/ob is
 * 34/25/34; RET-bounded aligned strict/rb is 35/15. These are distinct
 * metrics: the newly restored instructions remove a two-instruction
 * index shift but do not resolve the remaining allocation differences.
 * Further rejected families and measurements: docs/lanes/scope-f.md. */
/* Scope F sixteenth continuation (2026-09-06): 34 -> 13 strict;
 * first 25, real 222i/698B, original 16-byte frame. Still WIP.
 *
 * The register problem is resolved. Read tilex inline in the case-3
 * target-x expression, re-read y through tile, and give the car index its
 * own local: car = seat >> 1; if (car) { }. Put that definition/consumer
 * AFTER target-x and BEFORE target-y. The empty consumer emits nothing,
 * but keeps the correct lifetime: tile EAX, reloaded x EBX, seat ECX,
 * x sum EDX, car EDI, and the shift before CalcMoveLine. Without it the
 * mismatch is 29; an early named xx instead spills r (211). Moving car
 * before target-x moves the shift too early (32). These are paired levers.
 *
 * In the loop head, name the byte, base-x and base-y reads before writing
 * tilex/tx. Keep both volatile def reads and the named base-y destination.
 * This restores the original head registers as well. Strict/rb/ob is now
 * 13/13/13; aligned strict/rb is 11/11, with no trailing data or escapes.
 * All 47 exact neighbors pass. The corrected case-6 seat store is intact.
 *
 * REMAINING: only instruction order, at head 25-27 (base-y first instead
 * of last), case 3 at 89-94 (tilex reload and seat copy too late), and
 * case 8 at 165-168 (two reordered pairs). Full residual and rejected
 * families are in docs/lanes/scope-f.md. No exact-completion claim yet. */
/* Scope F seventeenth continuation (2026-09-06): 13 -> 11 strict;
 * first 25, real 222i/698B, original 16-byte frame. Still WIP.
 *
 * Case 8's instruction order is now exact. Read def through a separate
 * volatile pointer into d, then reload tilex into xx before the sums.
 * Split after the sums with if (sx) { }. That empty block emits nothing;
 * together with the ordered def read, it moves both offset loads before
 * their adds and the tilex reload ahead of the first offset load.
 * Only xx's register differs there: EDI instead of original EBX at 165/168.
 * The head 25-27 and case-3 89-94 permutations remain unchanged.
 * Strict/rb/ob is 11/9/11; aligned strict/rb is 10/8. All 47 exact
 * neighbors pass, no data padding or branch escapes. The goal is open.
 *
 * Renaming/retyping the carrier, reusing existing locals, moving it to
 * loop/function scope, and later empty guards do not switch EDI to EBX.
 * Removing the earlier spill shims reintroduces spills/extra instructions.
 * A fully volatile tilex merely moves the head permutation; accompanying
 * field-read barriers add storage. Details: docs/lanes/scope-f.md. */
/* Scope F eighteenth continuation (2026-09-06): 11 -> 10 strict;
 * first 25, real 222i/698B, original 16-byte frame. Still WIP.
 *
 * Name the case-3 SeatOfs pointer and use it for the two table fields.
 * No pointer instruction or home is emitted. This moves the tilex reload
 * and car copy earlier, leaving five mismatches at 89-93 instead of six
 * at 89-94. The x reload now uses EDX and the table value EBX, opposite
 * the original; the sum, target stores and remainder of case 3 are exact.
 * Head 25-27 and case-8 165/168 are unchanged. Strict/rb/ob is 10/8/10;
 * RET-bounded aligned strict/rb is 8/6. All 47 exact neighbors pass.
 *
 * Named early x values still spill the rider. Local operand/sum groups,
 * head source/destination views, equivalent loop forms and local movement
 * call views do not close the remaining differences. Case-8 inline x
 * reads choose EBX but reorder its loads; named early reads order those
 * loads but choose EDI. These are measured alternatives, not an
 * impossibility result. Evidence and exclusions: docs/lanes/scope-f.md. */
// WIP-FUNCTION: LEGOLAND 0x0043bac0  (95.5%, 10/222 mismatches, first 25; real body 222i/698B, head and case-3 scheduling plus register choices remain)
void SpaceTower_Activate(RideElem* elem)
{
    RideDef*   def = elem->data;
    int        tilex;
    int        tiley;
    RiderNode* next;
    RideTile*  tile;
    int        base[2];          /* only [1] is ever used (original) */
    RiderNode* r;
    TowerRec*  rec;
    Bloke*     b;
    int        tx;
    unsigned   seat;
    unsigned char dir;

    SpaceTower_TickMachine();
    r = def->riders;
    while (r) {
        next = r->next;
        b = r->bloke;
        tile = RIDE_TILE(r);
        rec = SpaceTower_FindRecord(tile);
        if (rec == 0)
            break;
        {
            RideDef* d1 = *(RideDef* volatile*)&def;
            RideDef* d2 = *(RideDef* volatile*)&def;
            int xx = tile->b.x;
            int bx = d1->base_x;
            int by = d2->base_y;
            tilex = xx;
            tx = bx + xx;
            tiley = tile->b.y;
            {
                int* dst = &base[1];
                *dst = by + tiley;
            }
        }
        if (!b->state) {
            switch (b->action) {
            case 0:
                rec->joined++;
                rec->timer = 200;
                SpaceTower_TakeSeat(r, tile);
                b->flags |= 8;
                b->target.x = tx << 8;
                b->target.y = (base[1] + 1) << 8;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 1:
                r->bloke->f4a = 0;
                r->bloke->f38 = 0;
                b->action++;
                break;
            case 3: {
                unsigned car;
                SeatOfs* offset;
                seat = b->seat;
                offset = &g_tower_seat[seat];
                b->target.x = (offset->dx + *(volatile int*)&tilex) << 8;
                car = seat >> 1;
                if (car) { } /* Retain the original car-index lifetime. */
                b->target.y = (offset->dy + tile->b.y) << 8;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)g_tower_car[car].dir);
                b->action++;
                break;
            }
            case 4:
                b->flags |= 0x80;
                BlokeSitAnim(b);
                BlokeSetFrame(b, 0);
                b->f58 = 0;
                b->action++;
                rec->seated++;
                SpaceTower_CountSeated(rec);
                if ((short)(signed char)rec->seated
                        == g_spacetower_def->capacity)
                    SpaceTower_SetFull(rec);
                break;
            case 5:
                break;
            case 6:
                r->bloke->f4c = 0xffff;
                rec->seat[b->seat] = 0;
                b->flags &= (unsigned short)~0x80u;
                BlokeWalkAnim(b);
                BlokeSetFrame(b, 0);
                b->action++;
                break;
            case 2:
            case 7:
                SpaceTower_Queue(r);
                break;
            case 8:
                {
                    RideDef* d = *(RideDef* volatile*)&def;
                    int xx = *(volatile int*)&tilex;
                    int sx;
                    int sy;
                    sx = d->qx + xx;
                    sy = d->qy + tiley;
                    if (sx) { } /* Keep both offset loads before their adds. */
                    b->target.x = (sx << 8) + 0x80;
                    b->target.y = (sy << 8) + 0x80;
                }
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 9:
                RemoveBlokeFromRide(def, r);
                b->flags &= (unsigned short)~8u;
                if (--rec->riders == 0) {
                    rec->seated = 0;
                    Ride_ClearFlagToNotLetAnyoneOn((RideTile*)rec);
                }
                break;
            }
        }
        r = next;
    }
}

/* SPACE TOWER's draw is the only one of the six that sorts its riders by what
 * ANIMATION they are playing rather than by position: the 8-byte-per-entry
 * animation table at 0x004b775c maps a bloke's animation id (Bloke+0x50) to
 * the Anim3D it is running, and the two passes pick out the two animations the
 * tower uses. Riders on the first animation are drawn BEFORE the two lower car
 * layers and the tower body, riders on the second AFTER them (so one set is
 * occluded by the car and the other is not). Then the two moving layers (5 and
 * 3) are set to the frames the record carries and blitted at their own layer
 * offsets. When the class has no riders at all the whole vehicle sequence is
 * emitted a second time without either pass -- the original's two arms, which
 * share only the very last blit. */

typedef struct AnimRef {
    void* anim;                  /* +0x00 the Anim3D this id plays */
    int   n;                     /* +0x04 */
} AnimRef;

extern void  IP_RenderBlokeIn3DNow(Bloke* b);                /* 0x00440010 */

static __inline void MechDrawBand(Bloke** found, char n, int code)
{
    int i;

    if (n > 0) {
        i = n;
        do {
            if ((*found)->action == code)
                IP_RenderBlokeIn3DNow(*found);
            ++found;
        } while (--i);
    }
}

int   memcmp(const void* a, const void* b, unsigned int n);      /* CRT, intrinsic */
#pragma intrinsic(memcmp)

extern void  SpaceTower_DrawCar(TowerRec* rec, int car, int mode); /* 0x0043aee0 */

extern AnimRef g_bloke_anim_ref[];                           /* 0x004b775c */
extern int     g_anim_tower_a;                               /* 0x004b7750 */
extern int     g_anim_tower_b;                               /* 0x004b76b8 */

/* CLOSED (288/288, 849 B). Two levers, both already in the playbook but not
 * tried together here; 250 -> 0 in two steps:
 * (1) `PrintSprite(spr, screen.ox + off.ox, ..)` -- SCREEN FIRST (Joust).  All
 *     six call sites: with `off.ox + screen.ox` VC6 emits a three-register
 *     `lea` (both operands live past the sum) where the original has the
 *     two-operand `add eax, ebp` that lets the offset half die, and that
 *     rotated the registers for the rest of the body (250 -> 255 strict but
 *     register-blind 242 -> 30, which is what showed the change was right).
 * (2) The rider test is an INTRINSIC 2-byte `memcmp(t, sq, 2) == 0`, not
 *     `t->key == sq->key`.  That is what emits the "dead" `lea eax,[edi+0xc]`
 *     the previous rounds could not explain: VC6's inline memcmp
 *     materialises the first operand's ADDRESS and then folds the comparison
 *     into a word load, leaving the lea with no consumer (the same signature
 *     the _FindRec/_FindRecord searches have, and logflume2.c's
 *     LFStation_FindAt).  Ten `==` spellings had been measured and none can
 *     produce it.  With both levers the body is byte-exact. */
// FUNCTION: LEGOLAND 0x0043af50
void SpaceTower_Interact(RideElem* elem, int x, int y, RideTile* sq,
                         void* clip, int mode)
{
    RideDef*   item = elem->data;
    RiderNode* r;
    TowerRec*  rec;
    Offset     off2;
    Offset     off1;
    Offset     screen;

    r = item->riders;
    rec = SpaceTower_FindRecord(sq);
    if (rec) {
        screen = GetScreenCoordsForObject(sq, item);
        off1 = GetRenderOffsetForLayer(g_spacetower_layers, 1);
        AdjustOffsetForViewMode(&off1);
        if (r) {
            while (r) {
                RideTile* t = RIDE_TILE(r);

                if (memcmp(t, sq, 2) == 0
                    && !(r->bloke->flags & 0x80)
                    && g_bloke_anim_ref[r->bloke->anim].anim == &g_anim_tower_a)
                    IP_RenderBlokeIn3DNow(r->bloke);
                r = r->next;
            }
            SpaceTower_DrawCar(rec, 1, mode);
            SpaceTower_DrawCar(rec, 2, mode);
            PrintSprite(g_spacetower_spr1, screen.ox + off1.ox,
                        screen.oy + off1.oy, mode, 0);
            SpaceTower_DrawCar(rec, 0, mode);
            SpaceTower_DrawCar(rec, 3, mode);
            r = item->riders;
            while (r) {
                RideTile* t = RIDE_TILE(r);

                if (memcmp(t, sq, 2) == 0
                    && !(r->bloke->flags & 0x80)
                    && g_bloke_anim_ref[r->bloke->anim].anim == &g_anim_tower_b)
                    IP_RenderBlokeIn3DNow(r->bloke);
                r = r->next;
            }
            PrintSprite(g_spacetower_spr0, screen.ox + off1.ox,
                        screen.oy + off1.oy, mode, 0);
            LLSSetFrame(GetLLSForLayer(g_spacetower_layers, 5), rec->frame5);
            off2 = GetRenderOffsetForLayer(g_spacetower_layers, 5);
            AdjustOffsetForViewMode(&off2);
            PrintSprite(GetSpriteForLayer(g_spacetower_layers, 5),
                        screen.ox + off2.ox, screen.oy + off2.oy, mode, 0);
            LLSSetFrame(GetLLSForLayer(g_spacetower_layers, 3), rec->frame3);
            off2 = GetRenderOffsetForLayer(g_spacetower_layers, 3);
            AdjustOffsetForViewMode(&off2);
            PrintSprite(GetSpriteForLayer(g_spacetower_layers, 3),
                        screen.ox + off2.ox, screen.oy + off2.oy, mode, 0);
        } else {
            SpaceTower_DrawCar(rec, 1, mode);
            SpaceTower_DrawCar(rec, 2, mode);
            PrintSprite(g_spacetower_spr1, screen.ox + off1.ox,
                        screen.oy + off1.oy, mode, 0);
            SpaceTower_DrawCar(rec, 0, mode);
            SpaceTower_DrawCar(rec, 3, mode);
            LLSSetFrame(GetLLSForLayer(g_spacetower_layers, 5), rec->frame5);
            off2 = GetRenderOffsetForLayer(g_spacetower_layers, 5);
            AdjustOffsetForViewMode(&off2);
            PrintSprite(GetSpriteForLayer(g_spacetower_layers, 5),
                        screen.ox + off2.ox, screen.oy + off2.oy, mode, 0);
            LLSSetFrame(GetLLSForLayer(g_spacetower_layers, 3), rec->frame3);
            off2 = GetRenderOffsetForLayer(g_spacetower_layers, 3);
            AdjustOffsetForViewMode(&off2);
            PrintSprite(GetSpriteForLayer(g_spacetower_layers, 3),
                        screen.ox + off2.ox, screen.oy + off2.oy, mode, 0);
        }
    }
}

/* ==========================================================================
 * COPTERS -- the per-tick update.
 * Same two-part shape as the Space Tower: tick the roundabout itself
 * (0x00404bc0, which owns the record list and the copter animation), then one
 * step of every rider's state machine. A rider's COPTER is chosen once, at
 * join time, by 0x00404f20(rider, square) -- the same call is repeated at
 * every step that needs it rather than being cached in the bloke -- and the
 * index it returns doubles as (a) the slot in the record (rec+0x30, stride
 * 0x20) that remembers which rider is in which copter and (b) the flight path
 * the rider is carried along.
 *
 * THE COPTERS STATE MACHINE (Bloke+0x60), twelve steps:
 *   0  JOIN: mark "on this ride", claim a copter, write the rider into that
 *      copter's slot, count it in, arm the record's 180-frame timer, clear the
 *      walk path.
 *   1  put the rider ON its copter's flight path (0x004122d0) and hand it to
 *      the ride's own step helper (0x00403d30).
 *   2  advance the rider along the path it is on (0x00412300), which is what
 *      actually flies the copter round.
 *   3, 7  step on (the two pure "wait a frame" steps).
 *   4  count the rider as seated and, at the class capacity (ObjDef+0x2e),
 *      tell the machine it is full (0x004048b0).
 *   5  SIT: mark "riding" and switch to the sit animation on frame 0 -- and do
 *      NOT advance the step; the machine moves the rider on itself.
 *   6  ALIGHT: back to the walk animation on frame 0, stop riding.
 *   8  the descent mirror of step 1 (path helper 0x004122a0).
 *   9  the descent mirror of step 2.
 *  10  LEAVE: walk to the centre of the ride's own square.
 *  11  DONE: off the class list, clear "on this ride", and when the last rider
 *      has gone let people on again.
 * The path lookup in steps 1 and 8 leaves its local UNINITIALISED when the
 * copter index is above 4 -- an original bug, reproduced.
 * ========================================================================== */

extern void  Copters_TickMachine(void);                      /* 0x00404bc0 */
extern int   Copters_CopterOf(RiderNode* r, RideTile* t);    /* 0x00404f20 */
#ifndef LEGOLAND_PORTABLE
extern void  Copters_StepRider(RiderNode* r);                /* 0x00403d30 */
#else
extern int Copters_StepRider(RiderNode* r);                /* 0x00403d30 */
#endif
extern void  Copters_SetFull(CoptersRec* rec);               /* 0x004048b0 */
extern void  WalkPath_Board(void* path, Bloke* b);           /* 0x004122d0 */
extern void  WalkPath_Alight(void* path, Bloke* b);          /* 0x004122a0 */
extern int   WalkPath_IndexOf(Bloke* b);                     /* 0x004122f0 */
extern void  WalkPath_Advance(void* path, int x, int y, Bloke* b); /* 0x00412300 */

/* CLOSED (229/229; was 213 mismatches + ESCAPES). The whole thing was ONE
 * argument-evaluation decision, not a register-priority mystery: in steps 2
 * and 9 the original looks the flight path up in its own statement
 *     i = WalkPath_IndexOf(b);
 *     WalkPath_Advance(g_copters_paths[i], tx, ty, b);
 * With the nested call written INSIDE the argument list, VC6 pushes b, ty and
 * tx BEFORE calling WalkPath_IndexOf (a nested call in the last-pushed argument
 * lets it push the simple arguments first), so tx/ty never have to survive a
 * call and stay in eax/ecx; the two callee-saved registers then go to the
 * cursor and the tile pointer, `act` is pushed out to a byte home (+4 frame),
 * and the shared Board/StepRider tail of step 1 is duplicated into every
 * inner case (the ESCAPES). Written as a statement, WalkPath_IndexOf is called
 * FIRST and tx/ty are live across it: they take ebx/ebp, the cursor is homed
 * in the dead `elem` slot and reloaded per use (the rotated loop with the
 * `jmp` into its middle), `def` is reloaded from 0x18 per use, the tile
 * pointer is rematerialised from those reloads, and the tails share. The index
 * MUST go through the same `i` that steps 1 and 8 use (a `void* fly` local or
 * a block-scope pointer leaves a dead store of the path into its home before
 * each Advance call; reusing `path`/`path2` does too). Two companions fell
 * out with the pressure gone: the head reads x and y through ONE named
 * `tile` pointer (both through ebp, the cursor dead after the lea), and the
 * switch is on `b->action` directly with `b->action++` in steps 3/4/7 -- the
 * original's `inc al` on the dispatch byte is VC6 forwarding the load, not a
 * saved `act` local (a named `act` is what put it in memory). */
// FUNCTION: LEGOLAND 0x00404be0
void Copters_Activate(RideElem* elem)
{
    RideDef*    def = elem->data;
    void*       path;
    void*       path2;
    RiderNode*  next;
    RiderNode*  r;
    CoptersRec* rec;
    Bloke*      b;
    int         tx;
    int         ty;
    int         i;
    unsigned char dir;
    RideTile*   tile;

    Copters_TickMachine();
    r = def->riders;
    while (r) {
        next = r->next;
        b = r->bloke;
        tile = RIDE_TILE(r);
        rec = Copters_FindRecord(tile);
        if (rec == 0)
            break;
        tx = def->base_x + tile->b.x;
        ty = def->base_y + tile->b.y;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags |= 8;
                rec->seat[Copters_CopterOf(r, RIDE_TILE(r))].rider = r;
                rec->joined++;
                rec->timer = 180;
                b->f58 = 0;
                b->action++;
                break;
            case 1:
                i = Copters_CopterOf(r, RIDE_TILE(r));
                switch (i) {
                case 0: path = g_copters_path0; break;
                case 1: path = g_copters_path2; break;
                case 2: path = g_copters_path1; break;
                case 3: path = g_copters_path3; break;
                case 4: path = g_copters_path4; break;
                }
                WalkPath_Board(path, b);
                Copters_StepRider(r);
                break;
            case 2:
                i = WalkPath_IndexOf(b);
                WalkPath_Advance(g_copters_paths[i], tx, ty, b);
                break;
            case 3:
            case 7:
                b->action++;
                break;
            case 4:
                b->action++;
                rec->seated++;
                if ((short)(signed char)rec->seated == g_copters_def->capacity)
                    Copters_SetFull(rec);
                break;
            case 5:
                b->flags |= 0x80;
                BlokeSitAnim(b);
                BlokeSetFrame(b, 0);
                break;
            case 6:
                BlokeWalkAnim(b);
                BlokeSetFrame(b, 0);
                b->flags &= (unsigned short)~0x80u;
                b->action++;
                break;
            case 8:
                i = Copters_CopterOf(r, RIDE_TILE(r));
                switch (i) {
                case 0: path2 = g_copters_path0; break;
                case 1: path2 = g_copters_path2; break;
                case 2: path2 = g_copters_path1; break;
                case 3: path2 = g_copters_path3; break;
                case 4: path2 = g_copters_path4; break;
                }
                WalkPath_Alight(path2, b);
                Copters_StepRider(r);
                break;
            case 9:
                i = WalkPath_IndexOf(b);
                WalkPath_Advance(g_copters_paths[i], tx, ty, b);
                break;
            case 10:
                b->target.x = (tx << 8) + 0x80;
                b->target.y = (ty << 8) + 0x80;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 11:
                RemoveBlokeFromRide(def, r);
                b->flags &= (unsigned short)~8u;
                if (--rec->riders == 0)
                    Ride_ClearFlagToNotLetAnyoneOn((RideTile*)rec);
                break;
            }
        }
        r = next;
    }
}

/* PLANE RIDE ("Zoomer" in the assets and in the save chunk) draws in three
 * passes and is the only one of the six that COLLECTS its riders into a small
 * fixed array first: every bloke whose ride_id is this square goes into a
 * four-entry stack array (with no bounds check -- a fifth rider on one plane
 * would write past it; reproduced), and the array is then walked twice, once
 * for the blokes on state-machine step 13 and once for step 14, so the two
 * groups are drawn on either side of the plane body. After the body (sprite
 * layer 1, on the frame the record carries) the record's frame is published
 * into the ride's z-sprite exactly as Safari does, the riders that are
 * actually ON the plane are positioned and drawn, and the second moving layer
 * (2) goes on top. With no riders at all, the two layers are emitted again on
 * their own -- the original's second arm. */

typedef struct PlaneRec {
    RideTile          tile;      /* +0x00 the map square this copy sits on */
    unsigned char     seated;    /* +0x02 */
    unsigned char     riders;    /* +0x03 */
    signed char       frame1;    /* +0x04 animation frame of sprite layer 1 */
    signed char       frame2;    /* +0x05 animation frame of sprite layer 2 */
    unsigned char     pad06[0x14 - 6];
    unsigned char     joined;    /* +0x14 */
    unsigned char     pad15[3];
    int               timer;     /* +0x18 (its top byte doubles as seat 0 --
                                  * the seat slots are at +0x1b, exactly as in
                                  * the Spider record) */
    unsigned char     pad1c[4];
    struct PlaneRec*  next;      /* +0x20 */
} PlaneRec;

extern ZSprite* g_plane_zspr_obj;                            /* 0x0081cae0 */

/* STATE: all 275 instructions, the 0x24 frame (rec spill / off / screen /
 * the four-entry rider array, with the rider offset pair pooled onto the
 * array) and the whole block layout are reproduced; the semantics are certain.
 * The residual is register naming: the original's zero register is ecx and
 * ours is eax (which also flips two of the five zero stores between an
 * immediate and a register form), and our two array walks spill their counter
 * where the original keeps it in ebx. Hoisting the map-square key into a local
 * before the collect loop is REQUIRED (without it VC6 spills the key to a u16
 * stack temp and the frame grows by 8).
 *
 * ROUND 2 narrowed both halves to ONE register-allocation decision, and they
 * are the same decision. The original gives ebx to the ARRAY-WALK COUNTER and
 * leaves the rider count `n` memory-resident in its byte home, reloading it
 * into cl right after each IP_RenderBlokeIn3DNow call (`mov cl,[esp+0x3c]`),
 * so the two walk guards share one `test cl,cl` and each walk re-widens with
 * its own `movsx ebx,cl`. Ours does the mirror image: it widens n ONCE into
 * ebx, keeps it there across both walks, and spills the counter into n's own
 * home (`mov [esp+0x38],ebx` then a load/dec/store every iteration). That
 * costs the extra `test ebx,ebx`, and -- because the zero register is picked
 * before scheduling -- it is very probably also why our function-wide zero
 * lands in eax (materialised after `elem` dies) where the original's lands in
 * ecx (materialised while eax still holds `elem`, which is what lets VC6
 * schedule the `xor` up into the push run and emit one of the five zero
 * stores before the argument push, with found[0] taking an immediate 0).
 * Measured inert this round: all 120 permutations of the five zero stores
 * (VC6 fully normalises them), every chained-assignment grouping,
 * memset(found,0,sizeof found), moving `r = item->riders` before/among the
 * zero stores, spelling it `elem->data->riders`, `(void)&n`, routing the
 * count through `char* np = &n` (VC6 folds np straight back to n), and four
 * walk spellings (pointer+count for, `!= 0` guard, per-block counters,
 * down-counting index). One spelling DOES flip the counter into ebx -- two
 * `if (n > 0) { q = found; c = n; do {...} while (--c != 0); }` blocks with a
 * shared c -- but it also enregisters n in bl across the COLLECT loop, which
 * breaks the (currently exact) collect loop and the prologue. And moving
 * `item = elem->data` below the zero stores proves the other constraint:
 * `elem` must be DEAD before the first zero store, or n loses its home in the
 * dead elem argument slot and the frame grows from 0x24 to 0x28. */
/* THIS ROUND the two band loops were moved to the shared MechDrawBand helper
 * (declared next to IP_RenderBlokeIn3DNow above): walking the PARAMETER rather
 * than a copied local cursor keeps the queue-address `lea` in the guard block,
 * which stops VC6 hoisting the count's `movsx` out of the bands and parking the
 * band CONSTANT in the count's register.  277 -> 275 instructions (the
 * original's exact count) and mismatch 237 -> 94.
 * WHAT IS LEFT, in three groups:
 *  (a) THE ZERO REGISTER, indices 5-16.  The original hoists a zero into ecx
 *      and stores it into four of the `found[]`/screen slots while leaving two
 *      of them as immediates (`mov byte ptr [esp+N],0`, `mov dword ptr
 *      [esp+N],0`); we hoist into eax and use it for ALL six.  See person3d.c's
 *      LoadAnim3D for the shape that fixed the same class there (a memset of
 *      the whole aggregate rather than field-by-field stores).
 *  (b) the lea/jle transposition, one per band, shared with every other banded
 *      draw in the project.
 *  (c) one schedule/rotation difference around index 71 in the
 *      GetRenderOffsetForLayer / PrintSprite argument block. */
/* AND THE ZERO REGISTER FELL OUT TOO (mismatch 94 -> 60, first divergence
 * index 5 -> 45).  The five explicit stores
 *     found[0] = 0; found[1] = 0; n = 0; found[2] = 0; found[3] = 0;
 * were an attempt to reproduce the original's interleave by hand.  The real
 * shape is `Bloke* found[4] = { 0 };` -- VC6's array-initialiser lowering emits
 * the FIRST element as an immediate store and the other three out of a hoisted
 * zero register, which is exactly what the original has
 * (`xor ecx,ecx` ... `mov [E-0x0c],ecx / mov [E-0x08],ecx /
 * mov byte [E+4],0 / mov [E-0x10],0 / mov [E-0x04],ecx`).  `char n = 0;` must
 * then be declared BEFORE the array for its immediate byte store to land
 * between the register stores and the array's own immediate; declared after it
 * (or written as a separate statement) the store comes out `mov byte
 * [esp+0x3c],cl` at the end of the run instead (63).
 * WHAT IS LEFT, 60 of 275: the two per-band lea/jle transpositions and five
 * one-instruction schedule swaps in the PrintSprite argument blocks (the
 * original loads the screen offset one slot earlier and pushes in the other
 * register order). */
/* Position one riding bloke: its person's own offset is the rider's seat
 * offset, its screen position that plus the ride-wide rider offset (view
 * adjusted) plus the square's screen origin. MUST be an inlined helper -- see
 * the closing note below. */
static __inline void PlaneRide_PlaceRider(Bloke* b, Offset* screen)
{
    Person3D* p = b->person;
    Offset    ofs;

    ofs.ox = g_plane_rider_dx;
    ofs.oy = g_plane_rider_dy;
    p->local.ox = b->ride_dx;
    p->local.oy = b->ride_dy;
    AdjustBlokePosition(&p->local);
    AdjustOffsetForViewMode(&ofs);
    p->screen.ox = b->ride_dx + ofs.ox + screen->ox;
    p->screen.oy = b->ride_dy + ofs.oy + screen->oy;
    AdjustBlokePosition(&p->screen);
}

/* CLOSED (275/275). The last five were one scheduling interleave in the
 * rider block: the original loads `p = b->person` between the two rider-offset
 * global loads and sinks the `ofs.oy` store below the `b->ride_dx` load and
 * the `lea` of &p->local. With `ofs` a named local of THIS function it is an
 * escaped local, so VC6 orders every pointer load against its stores and the
 * two stores stay adjacent; no statement order, struct copy, aggregate
 * initialiser, scalar temps or setter helper moves them (22 measured). An
 * explicit `t = b->ride_dx` between the two stores reproduces the interleave
 * but flips the item/rec edi<->ebp tie-break. THE LEVER: the whole
 * position-the-rider block is a `static __inline` helper (PlaneRide_PlaceRider
 * above); its `ofs` is an inline-expansion temporary, which escapes the
 * caller's alias class, so the scheduler is free to float the store and picks
 * the original's pairing. By value, by pointer or as two ints for the screen
 * origin are all exact. */
// FUNCTION: LEGOLAND 0x0043da60
void PlaneRide_Interact(RideElem* elem, int x, int y, RideTile* sq,
                        void* clip, int mode)
{
    RideDef*   item = elem->data;
    Offset     off;
    Offset     screen;
    char       n = 0;
    Bloke*     found[4] = { 0 };
    RiderNode* r;
    PlaneRec*  rec;
    unsigned short key;
    char i;

    r = item->riders;
    rec = PlaneRide_FindRecord(sq);
    if (rec) {
        screen = GetScreenCoordsForObject(sq, item);
        if (r) {
            key = sq->key;
            while (r) {
                if (key == r->ride_id)
                    found[n++] = r->bloke;
                r = r->next;
            }
            if (n) {
                for (i = 0; i < n; i++)
                    if (found[i]->action == 13)
                        IP_RenderBlokeIn3DNow(found[i]);
                for (i = 0; i < n; i++)
                    if (found[i]->action == 14)
                        IP_RenderBlokeIn3DNow(found[i]);
                LLSSetFrame(GetLLSForLayer(g_plane_layers, 1), rec->frame1);
                off = GetRenderOffsetForLayer(item->sprite, 1);
                AdjustOffsetForViewMode(&off);
                PrintSprite(GetSpriteForLayer(item->sprite, 1),
                            screen.ox + off.ox, screen.oy + off.oy, mode, 0);
                r = item->riders;
                **g_plane_zspr_obj->pframe = (unsigned short)rec->frame1;
                for (; r; r = r->next) {
                    if (sq->key == r->ride_id
                        && (r->bloke->flags & 0x80)) {
                        PlaneRide_PlaceRider(r->bloke, &screen);
                        IP_RenderBlokeIn3DNow(r->bloke);
                    }
                }
                LLSSetFrame(GetLLSForLayer(g_plane_layers, 2), rec->frame2);
                off = GetRenderOffsetForLayer(item->sprite, 2);
                AdjustOffsetForViewMode(&off);
                PrintSprite(GetSpriteForLayer(item->sprite, 2),
                            screen.ox + off.ox, screen.oy + off.oy, mode, 0);
                return;
            }
        }
        LLSSetFrame(GetLLSForLayer(g_plane_layers, 1), rec->frame1);
        off = GetRenderOffsetForLayer(item->sprite, 1);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(item->sprite, 1),
                    screen.ox + off.ox, screen.oy + off.oy, mode, 0);
        LLSSetFrame(GetLLSForLayer(g_plane_layers, 2), rec->frame2);
        off = GetRenderOffsetForLayer(item->sprite, 2);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(item->sprite, 2),
                    screen.ox + off.ox, screen.oy + off.oy, mode, 0);
    }
}

/* SPIDER RIDE draws like the Plane -- collect this square's riders into a
 * stack array, walk it once for the blokes on step 14, then the vehicle -- but
 * its array holds SIXTEEN riders (it is the big ride) and its riders get TWO
 * offset pairs, not one: the ride-wide pair (0x0082c660/64, set to (-1,2) by
 * the create handler) is always applied, and a second pair
 * (0x004b4e20/0x004b4e24) is added on top only for a rider whose Bloke+0x35
 * flag is 1 -- the riders on the outer arm of the spider, which swing wider
 * than the rest. The second pair is view-adjusted separately and added to the
 * on-screen position AFTER it has already been formed, in its own pair of
 * statements. Layer 1 is the arm, layer 2 the car; the car's own sprite is
 * blitted from the ride's two extra sprites (0x004cbf1c before the riders,
 * 0x004cbf14 after) when the square has riders, and straight from the class
 * sprite when it does not. */

typedef struct SpiderRec {
    RideTile          tile;      /* +0x00 the map square this copy sits on */
    unsigned char     seated;    /* +0x02 */
    unsigned char     riders;    /* +0x03 */
    signed char       frame;     /* +0x04 the arm's animation frame */
    unsigned char     pad05[0x14 - 5];
    unsigned char     joined;    /* +0x14 */
    unsigned char     pad15[3];
    int               timer;     /* +0x18 */
    unsigned char     pad1c[0x2c - 0x1c];  /* the seat slots live at +0x1b,
                                            * one byte per seat and hence
                                            * overlapping this timer's top
                                            * byte for seat 0 -- see
                                            * SPIDER_SEAT below */
    struct SpiderRec* next;      /* +0x2c */
} SpiderRec;

void* memset(void*, int, unsigned int);

/* The Spider's per-seat "occupied" bytes start at record+0x1b, which is the
 * TOP BYTE of the 0x18 timer: seat 0 writes into it. Harmless only because
 * the timer never exceeds 0xffffff. Reproduced as an original quirk. */
#define SPIDER_SEAT(rec, i) (((unsigned char*)(rec))[0x1b + (i)])

extern ZSprite* g_spider_zspr_obj;                           /* 0x0082c668 */
extern int      g_spider_swing_x;                            /* 0x004b4e20 */
extern int      g_spider_swing_y;                            /* 0x004b4e24 */

/* CLOSED (289/289). History: 249 -> 3 by (1) `Bloke* found[16] = { 0 };`
 * (the original's `mov dword [esp+0x30],0` + 15-dword `rep stosd` is VC6's
 * aggregate-initialiser lowering, not a 15-array plus an int), (2) the array
 * and `n` in an INNER SCOPE opened after `r = item->riders;` (a function-level
 * initialiser is filled before every other statement; the original loads `r`
 * into ebp before the fill), (3) the walk as `if (n > 0) { Bloke** q = found;
 * int i = n; do..while(--i) }` (guard on the char: test al / je / jle, lea
 * after the jle), (4) `a` and `swing` as BLOCK locals of the rider loop
 * (pooled onto the dead key/item homes at 0x10/0x18).
 * THE LAST 3 (a scheduling tie in the rider prelude: the original issues the
 * `mov esi,[edi+4]` p load between the `xor eax,eax` and the two swing zero
 * stores, and keeps the a.oy store above the `cmp byte [edi+0x35],1`) closed
 * with ONE lever, found after ~250 measured variants: WHICH POINTER SPELLING
 * A DEREF GOES THROUGH DECIDES ITS ALIAS ORDERING AGAINST STORES TO ESCAPED
 * LOCALS. A load through a NAMED pointer local (`bl`, assigned inside the
 * condition: `((bl = r->bloke)->flags & 0x80)`) is NOT ordered against the
 * a/swing stores, so `p = bl->person` floats above the swing zero stores; a
 * load through the textual CSE temp (`r->bloke->b35`, the second textual
 * `r->bloke`) IS ordered, so the a.oy store stays above the byte compare
 * instead of sinking past it (which also splits the compare into
 * mov al / cmp al). With `Bloke* bl = r->bloke;` declared in the if-body
 * (bl is then the CSE temp of the condition's `r->bloke->flags`) every deref
 * pins both ways and the p load lands after the zframe load (the old 3);
 * with bl named and used for the b35 test too nothing pins and the a.oy
 * store sinks (3 the other way); the same split via a `static __inline`
 * PlaceRider helper (the Plane's lever) also leaves the a.oy sink. The zero
 * is `swing.oy = swing.ox = 0;` (ox stored first); p before or after it is
 * inert once the pins are right. Casts, tuple-count probes, aggregate
 * initialisers, struct copies of the 0x82c660 pair, b35 through a local,
 * and the bl-at-loop-top form (`Bloke* bl = r->bloke; if (.. && bl->flags..)`,
 * 4 off: p load pinned) were all measured and are recorded in
 * scratchpad/mechrides/siv1..17.py. */
// FUNCTION: LEGOLAND 0x00415ae0
void SpiderRide_Interact(RideElem* elem, int x, int y, RideTile* sq,
                         void* clip, int mode)
{
    RideDef*    item = elem->data;
    Offset      off;
    Offset      screen;
    RiderNode*  r;
    SpiderRec*  rec;

    r = item->riders;
    {
    char        n = 0;
    Bloke*      found[16] = { 0 };

    rec = SpiderRide_FindRecord(sq);
    if (rec) {
        screen = GetScreenCoordsForObject(sq, item);
        if (r) {
            while (r) {
                if (sq->key == r->ride_id)
                    found[n++] = r->bloke;
                r = r->next;
            }
            if (n) {
                if (n > 0) {
                    Bloke** q = found;
                    int     i = n;

                    do {
                        if ((*q)->action == 14)
                            IP_RenderBlokeIn3DNow(*q);
                        q++;
                    } while (--i);
                }
                LLSSetFrame(GetLLSForLayer(g_spider_layers, 1), rec->frame);
                off = GetRenderOffsetForLayer(g_spider_layers, 1);
                AdjustOffsetForViewMode(&off);
                PrintSprite(GetSpriteForLayer(g_spider_layers, 1),
                            screen.ox + off.ox, screen.oy + off.oy, mode, 0);
                off = GetRenderOffsetForLayer(g_spider_layers, 2);
                AdjustOffsetForViewMode(&off);
                PrintSprite(g_spider_zspr2, off.ox + screen.ox,
                            off.oy + screen.oy, mode, 0);
                **g_spider_zspr_obj->pframe = (unsigned short)rec->frame;
                for (r = item->riders; r; r = r->next) {
                    Bloke* bl;
                    if (sq->key == r->ride_id
                        && ((bl = r->bloke)->flags & 0x80)) {
                        Person3D* p;
                        Offset    a;
                        Offset    swing;

                        swing.oy = swing.ox = 0;
                        p = bl->person;
                        a.ox = g_spider_zframe;
                        a.oy = g_spider_zstate;
                        /* r->bloke, not bl: a deref through the textual
                         * CSE temp is alias-ordered against the a/swing
                         * stores (pins a.oy before the byte load); a deref
                         * through the NAMED bl is not (frees the p load to
                         * float above the swing stores). See the note. */
                        if (r->bloke->b35 == 1) {
                            swing.ox = g_spider_swing_x;
                            swing.oy = g_spider_swing_y;
                            AdjustOffsetForViewMode(&swing);
                        }
                        p->local.ox = bl->ride_dx;
                        p->local.oy = bl->ride_dy;
                        AdjustBlokePosition(&p->local);
                        AdjustOffsetForViewMode(&a);
                        p->screen.ox = bl->ride_dx + a.ox + screen.ox;
                        p->screen.oy = bl->ride_dy + a.oy + screen.oy;
                        p->screen.ox += swing.ox;
                        p->screen.oy += swing.oy;
                        AdjustBlokePosition(&p->screen);
                        IP_RenderBlokeIn3DNow(r->bloke);
                    }
                }
                off = GetRenderOffsetForLayer(g_spider_layers, 2);
                AdjustOffsetForViewMode(&off);
                PrintSprite(g_spider_zspr, off.ox + screen.ox,
                            off.oy + screen.oy, mode, 0);
                return;
            }
        }
        LLSSetFrame(GetLLSForLayer(g_spider_layers, 1), rec->frame);
        off = GetRenderOffsetForLayer(g_spider_layers, 1);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(g_spider_layers, 1),
                    screen.ox + off.ox, screen.oy + off.oy, mode, 0);
        off = GetRenderOffsetForLayer(g_spider_layers, 2);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(g_spider_layers, 2),
                    screen.ox + off.ox, screen.oy + off.oy, mode, 0);
    }
    }
}

/* ==========================================================================
 * SAFARI RIDE -- the per-tick update, and the most complete of the six.
 * Tick the vehicle (0x00415200), then step every rider. The Safari's riders
 * are carried by BNV PATHS (pre-baked 3D motion curves in Zbuffers\*.bnv, one
 * named "manbox01".."manboxNN" per seat pair): the ride builds a path when the
 * rider boards, advances it a frame per tick, and frees it when the path runs
 * out or passes the seat's end frame. The path name is built IN PLACE, by
 * sprintf-ing the two digits into the middle of the string literal
 * "manbox??" at 0x004b4cac -- so the literal is writable data and the ride
 * mutates it on every mount (reproduced).
 *
 * THE SAFARI STATE MACHINE (Bloke+0x60) -- a SPARSE switch (VC6 emits a
 * 15-byte case-index table at 0x00415750 plus a 9-entry jump table):
 *   0   JOIN: count the rider into the record, arm its 180-frame timer, mark
 *       "on this ride", clear the walk path.
 *   1   MOUNT (on): project the bloke's 24.8 world position into screen space
 *       (the isometric transform: (x-y)*tilew>>9, (x+y)*tileh>>9, minus the
 *       scroll and the ride's own half-offsets and the square's screen
 *       origin, doubled), hand the 3D person the ride's z-sprite and depth,
 *       then open BNV path "manbox<seat/2+1>" from the ON buffer and park it
 *       on frame 0.
 *   2   RIDING (on): advance the path; when it runs out, or its frame reaches
 *       the seat's limit (0x004b4cc4, {66,66,47,47,80,80,48,48}), free the
 *       path and jump to step 5. Either way the bloke is held on frame
 *       Bloke+0x74.
 *   5   SEATED: sit animation on frame 0, new depth, count the rider as
 *       seated and, at the class capacity (ObjDef+0x2e), tell the machine the
 *       ride is full (0x00414ab0).
 *   7   MOUNT (off): the mirror of 1 -- walk animation, z-sprite and depth
 *       again, and a second BNV path from the OFF buffer, seeded from the
 *       rider's own doubled offset pair rather than from its world position.
 *   8   RIDING (off): advance it, limit table 0x004b4ce4, then step 13.
 *   13  ALIGHT: drop the z-sprite, stop riding, un-adjust the person's screen
 *       position back into a map reference (this is how the ride hands the
 *       bloke back to the walking AI at wherever the path left it), then walk
 *       to the class's queue/exit square.
 *   14  DONE: off the class list, clear "on this ride", and when the record's
 *       last rider has gone reset its seated count and let people on again.
 * Steps 3, 4, 6, 9, 10, 11 and 12 are not this ride's (they fall through the
 * default arm), which is why the switch is sparse.
 * ========================================================================== */

extern void   UnAdjustBlokePosition(Offset* p);              /* 0x00442d80 */
#ifndef LEGOLAND_PORTABLE
extern void   ScreenToMapRef(Offset* screen, Pos* out, int z);/* 0x0045be90 */
#else
extern int ScreenToMapRef(Offset* screen, Pos* out, int z);/* 0x0045be90 */
#endif
extern void   HeapFree_w(void* p);                           /* 0x0049e4d0 */
#ifndef LEGOLAND_PORTABLE
extern int    sprintf_w(char* dst, const char* fmt, int v);  /* 0x0049e573 */
#else
/* 0x0049e573 IS the CRT's `sprintf` (six other files spell it `Format(char*,
 * const char*, ...)`), and on wasm32 a variadic callee's third parameter is the
 * ADDRESS of the buffer clang wrote the variable arguments into -- not the
 * value. Both spellings lower to (i32, i32, i32) -> i32, so wasm-ld, linkreport
 * and name_trap see nothing; the fixed one made `sprintf` read the seat number
 * as a va_list, every "%02d" below came out "00" and no rider on the Spider,
 * Safari, Barrels or Plane ever reached a numbered path. PORT-M20 / PORT-A11,
 * gated by portable/tools/variadic_sweep.py. */
extern int    sprintf_w(char* dst, const char* fmt, ...);    /* 0x0049e573 */
#endif
extern void   GetTileDimensions(int* out_w, int* out_h);     /* 0x00460540 */
extern short  Get_XScroll(void);                             /* 0x004615f0 */
extern short  Get_YScroll(void);                             /* 0x00461600 */
extern float  GetUnitDepth(float near_z, float far_z);       /* 0x0044de50 */
/* 0x00484c20 */
/* The seed position handed to a BNV path is THREE ints, not two: the four
 * _Activate frames each carry an unreferenced dword directly above their two
 * seed locals (pos at frame+0x24 and pos2 at frame+0x30 in the Barrels, 12
 * bytes apart), which is the z the ride never fills in. Declaring the sixth
 * parameter `Pos*` makes the locals 8 bytes and shortens every frame by 8. */
typedef struct BnvPos {
    int x;                   /* +0x00 */
    int y;                   /* +0x04 */
    int z;                   /* +0x08 never stored by any of the four rides */
} BnvPos;

/* The 8-byte object the four _Activate frames spill sx into; see the note in
 * SpinningBarrels_Activate. */
typedef struct SpillPair {
    int x;
    int y;
} SpillPair;

extern void*  NewBNVPath(void* bin, int tag, const char* name,
                         float near_z, float far_z, BnvPos* pos);
extern void   BNVPath_SetDFrame(Bloke* b, void* path, int dframe); /* 0x004850b0 */
extern int    UpdateBlokeFromBNVPath(Bloke* b, void* path);  /* 0x00484cd0 */
extern int    BNVPath_GetDFrame(void* path);                 /* 0x00484ff0 */

typedef struct MapConfig {
    unsigned char  pad00[0x20];
    unsigned short ox;           /* +0x20 render origin */
    unsigned short oy;           /* +0x22 */
} MapConfig;

extern MapConfig* g_map_cfg;                                 /* 0x004bcbf4 lpConfig -- a POINTER to the config */

extern void SafariRide_TickMachine(void);                    /* 0x00415200 */
extern int  SafariRide_SeatOf(RiderNode* r, RideTile* t);    /* 0x00415760 */
extern void SafariRide_SetFull(SafariRec* rec);              /* 0x00414ab0 */

extern char g_safari_pathname[];                             /* 0x004b4cac "manbox??" */
extern const int g_safari_end_on[8];                         /* 0x004b4cc4 */
extern const int g_safari_end_off[8];                        /* 0x004b4ce4 */

/* Scope F close (2026-09-05): 402/402 instructions,
 * 1292/1292 bytes, strict/rb/ob 0/0/0.
 *
 * Keep the BNV position objects inside their owning cases. Their earlier
 * function-wide lifetime falsely constrained stores before the first escape;
 * narrowing it restores the person/flag hoists and departure short-load order.
 * The separate person-cache workaround is no longer needed.
 *
 * Keep the unshifted y product in sy, with an empty-if consumer BOTH before
 * and after sy2 = sy >> 9. Then accumulate the origin-minus-scroll delta
 * into sy2, retaining the third empty-if before the pivot subtractions.
 * The two early consumers vanish only after the compiler has kept the
 * multiplication/shift webs separate. Both are required: neither or only
 * the post-shift consumer gives Spider 34; only the pre-shift one gives 343;
 * both give zero, and the same shape closes all five BNV ride callbacks.
 *
 * The 12-byte BNV seeds still leave z uninitialized, as the original does.
 * The eight-byte spill object and its single volatile sx store reproduce
 * the original dead store and unused frame dword; do not remove them.
 * All earlier scope, register-rotation and departure-floor claims in
 * this function's former notes are superseded. See docs/lanes/scope-f.md. */
// FUNCTION: LEGOLAND 0x00415220
void SafariRide_Activate(RideElem* elem)
{
    RideDef*    def = elem->data;
    RideTile*   tile;
    int         tw;
    int         th;
    RiderNode*  next;
    Offset      screen;
    RiderNode*  r;
    SafariRec*  rec;
    Bloke*      b;
    int         qx;
    int         qy;
    int         sx;
    int         sy;
    unsigned char dir;
    int         wx;
    int         wy;
    int         sx2;
    SpillPair   spill;
    int         sy2;

    SafariRide_TickMachine();
    r = def->riders;
    while (r) {
        next = r->next;
        b = r->bloke;
        tile = RIDE_TILE(r);
        rec = SafariRide_FindRecord(tile);
        if (rec == 0)
            break;
        qx = def->qx + tile->b.x;
        qy = def->qy + tile->b.y;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                rec->joined++;
                rec->timer = 180;
                b->flags |= 8;
                b->action++;
                b->f58 = 0;
                break;
            case 1: {
                BnvPos pos;
                screen = GetScreenCoordsForObject(tile, def);
                wy = b->world.y;
                wx = b->world.x;
                GetTileDimensions(&tw, &th);
                sx = (wx - wy) * tw >> 9;
                sy = (wx + wy) * th;
                if (sy) { }
                sy2 = sy >> 9;
                if (sy2) { }
                *(volatile int*)&spill.x = sx;
                sx2 = g_map_cfg->ox - Get_XScroll() + sx;
                sy2 += g_map_cfg->oy - Get_YScroll();
                if (sy2) { }
                sx2 -= g_safari_ofs2.ox / 2;
                sx2 -= screen.ox;
                sy2 -= g_safari_ofs2.oy / 2;
                sy2 -= screen.oy;

                pos.x = sx2 * 2;
                pos.y = sy2 * 2;
                b->person->zsprite = g_safari_zspr;
                b->person->f30 = 1;
                b->person->depth = GetUnitDepth(-1617787.0f, -1618006.0f);
                r->bloke->b35 = 0;
                sprintf_w(&g_safari_pathname[6], "%02d",
                          (SafariRide_SeatOf(r, RIDE_TILE(r)) >> 1) + 1);
                r->bloke->bnvpath = NewBNVPath(g_safari_bnv1, 1,
                                               g_safari_pathname,
                                               -1617787.0f, -1618006.0f, &pos);
                UpdateBlokeFromBNVPath(b, r->bloke->bnvpath);
                BNVPath_SetDFrame(b, b->bnvpath, 0);
                b->action++;
                break;
            }
            case 2:
                b->flags |= 0x80;
                if (UpdateBlokeFromBNVPath(b, b->bnvpath) == 0) {
                    b->b35 = 1;
                    b->action = 5;
                    HeapFree_w(b->bnvpath);
                    b->bnvpath = 0;
                }
                if (b->bnvpath) {
                    if (BNVPath_GetDFrame(b->bnvpath)
                            >= g_safari_end_on[b->seat]) {
                        b->b35 = 1;
                        b->action = 5;
                        HeapFree_w(b->bnvpath);
                        b->bnvpath = 0;
                    }
                }
                BlokeSetFrame(b, b->b74);
                break;
            case 5:
                b->flags |= 0x80;
                BlokeSitAnim(b);
                BlokeSetFrame(b, 0);
                b->b35 = 1;
                b->person->depth = GetUnitDepth(-1617787.0f, -1618006.0f);
                b->action++;
                rec->seated++;
                if (rec->seated == g_safari_def->capacity)
                    SafariRide_SetFull(rec);
                break;
            case 7: {
                BnvPos pos2;
                b->flags |= 0x80;
                {
                    int px = b->ride_dx * 2;
                    int py = b->ride_dy * 2;
                    pos2.x = px;
                    pos2.y = py;
                }
                BlokeWalkAnim(b);
                BlokeSetFrame(b, 0);
                b->person->zsprite = g_safari_zspr;
                b->person->f30 = 1;
                b->person->depth = GetUnitDepth(-1617787.0f, -1618006.0f);
                b->b35 = 2;
                sprintf_w(&g_safari_pathname[6], "%02d", (b->seat >> 1) + 1);
                r->bloke->bnvpath = NewBNVPath(g_safari_bnv2, 2,
                                               g_safari_pathname,
                                               -1617787.0f, -1618006.0f,
                                               &pos2);
                BNVPath_SetDFrame(b, b->bnvpath, 0);
                UpdateBlokeFromBNVPath(b, b->bnvpath);
                b->action++;
                break;
            }
            case 8:
                if (UpdateBlokeFromBNVPath(b, b->bnvpath) == 0) {
                    b->b35 = 2;
                    b->action = 13;
                    HeapFree_w(r->bloke->bnvpath);
                    b->bnvpath = 0;
                }
                if (b->bnvpath) {
                    if (BNVPath_GetDFrame(b->bnvpath)
                            >= g_safari_end_off[b->seat]) {
                        b->b35 = 2;
                        b->action = 13;
                        HeapFree_w(b->bnvpath);
                        b->bnvpath = 0;
                    }
                }
                BlokeSetFrame(b, b->b74);
                break;
            case 13:
                /* the flag clear before the z-sprite store (137 -> 135) */
                b->flags &= (unsigned short)~0x80u;
                b->person->zsprite = 0;
                b->person->f30 = 0;
                UnAdjustBlokePosition(&b->person->screen);
                ScreenToMapRef(&b->person->screen, &b->world, 0);
                b->person->f34 = 0;
                b->world.x = b->world.x << 8;
                b->world.y = b->world.y << 8;
                b->target.x = (qx << 8) + 0x80;
                b->target.y = (qy << 8) + 0x80;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 14:
                RemoveBlokeFromRide(def, r);
                b->flags &= (unsigned short)~8u;
                if (--rec->riders == 0) {
                    rec->seated = 0;
                    Ride_ClearFlagToNotLetAnyoneOn((RideTile*)rec);
                }
                break;
            }
        }
        r = next;
    }
}

/* ==========================================================================
 * SPINNING BARRELS -- the per-tick update. Same machine as the Safari (BNV
 * paths named "BoxBloke%02d", built in place in the literal at 0x004b78b4),
 * with two differences worth calling out:
 *   - the vehicle tick (0x0043c930) runs AFTER the rider walk, not before,
 *     and a rider whose square has no record RETURNS from the handler, so
 *     that frame the barrels do not turn at all (reproduced);
 *   - the rider's leaving direction is RANDOM: step 16 pulls one bit out of
 *     rand() and shifts the exit target half a tile left for odd draws, which
 *     is why the queue out of the barrels fans out.
 * THE STATE MACHINE (Bloke+0x60), sparse over 0..17:
 *   0   JOIN: count in, arm the 400-frame timer, mark "on this ride", walk to
 *       five tiles above the ride's base square.
 *   1   MOUNT (on): the isometric projection of the bloke's world position
 *       (biased by the ride's own pivot, the one the create handler read out
 *       of sprite layer 3) becomes the seed of BNV path "BoxBloke<seat>",
 *       where the seat number comes from 0x0043ce10(rider, square, capacity).
 *   2   RIDING (on): advance the path; when it ends, step 6. The path is NOT
 *       freed here (it is left to the alighting path to replace) -- unlike
 *       the Safari, which frees it.
 *   6   SEATED: sit animation, count the rider as seated, and at the class
 *       capacity tell the machine it is full (0x0043c320).
 *   8   MOUNT (off) / 9 RIDING (off): the mirror pair, path buffer 0x0062fdf8.
 *   15  ALIGHT: free the seat slot (rec+0x20+seat), stop riding, and step out
 *       of the barrel -- world position and walk target are both set by hand
 *       to fixed sub-tile offsets around the base square.
 *   16  LEAVE: walk two tiles east of the base square, half a tile left on a
 *       random draw.
 *   17  DONE: off the class list, and reset the record when the last rider
 *       goes. NOTE the record's map square is at +0x04, not +0x00 -- the
 *       `next` link is what sits at +0x00 in this ride (ridesave.c agrees).
 * ========================================================================== */

typedef struct SBarrelRec {
    struct SBarrelRec* next;     /* +0x00 */
    RideTile          tile;      /* +0x04 the map square this copy sits on */
    unsigned char     seated;    /* +0x06 */
    unsigned char     riders;    /* +0x07 */
    signed char       frame3;    /* +0x08 animation frame of sprite layer 3 */
    unsigned char     pad09[0x18 - 9];
    unsigned char     joined;    /* +0x18 */
    unsigned char     pad19[3];
    int               timer;     /* +0x1c */
    signed char       frame2;    /* +0x20 animation frame of sprite layer 2 --
                                  * and ALSO seat slot 0, which the update
                                  * clears through rec+0x20+seat */
    unsigned char     seat1[0x13];/* +0x21 the rest of the seat slots */
} SBarrelRec;

extern int  rand_w(void);                                    /* 0x0049e4b2 */
extern void SpinningBarrels_TickMachine(void);               /* 0x0043c930 */
extern int  SpinningBarrels_SeatOf(RiderNode* r, SBarrelRec* rec, char cap); /* 0x0043ce10 -- takes the RECORD: it indexes the seat bytes at rec+0x21 */
extern void SpinningBarrels_SetFull(SBarrelRec* rec);        /* 0x0043c320 */

extern char g_sbarrel_pathname[];                            /* 0x004b78b4 "BoxBloke??" */

/* Scope F close (2026-09-05): 362/362 instructions,
 * 1148/1148 bytes, strict/rb/ob 0/0/0.
 *
 * Keep the BNV position objects inside their owning cases. Their earlier
 * function-wide lifetime falsely constrained stores before the first escape;
 * narrowing it restores the person/flag hoists and departure short-load order.
 * The separate person-cache workaround is no longer needed.
 *
 * Keep the unshifted y product in sy, with an empty-if consumer BOTH before
 * and after sy2 = sy >> 9. Then accumulate the origin-minus-scroll delta
 * into sy2, retaining the third empty-if before the pivot subtractions.
 * The two early consumers vanish only after the compiler has kept the
 * multiplication/shift webs separate. Both are required: neither or only
 * the post-shift consumer gives Spider 34; only the pre-shift one gives 343;
 * both give zero, and the same shape closes all five BNV ride callbacks.
 *
 * The 12-byte BNV seeds still leave z uninitialized, as the original does.
 * The eight-byte spill object and its single volatile sx store reproduce
 * the original dead store and unused frame dword; do not remove them.
 * All earlier scope, register-rotation and departure-floor claims in
 * this function's former notes are superseded. See docs/lanes/scope-f.md. */
// FUNCTION: LEGOLAND 0x0043c950
void SpinningBarrels_Activate(RideElem* elem)
{
    RideDef*    def = elem->data;
    RideTile*   tile;
    int         tw;
    int         th;
    RiderNode*  next;
    Offset      screen;
    RiderNode*  r;
    SBarrelRec* rec;
    Bloke*      b;
    int         tx;
    int         ty;
    int         sx;
    int         sy;
    unsigned char dir;
    int         wx;
    int         wy;
    int         sx2;
    int         sy2;
    SpillPair   spill;

    r = def->riders;
    while (r) {
        next = r->next;
        b = r->bloke;
        tile = RIDE_TILE(r);
        rec = SpinningBarrels_FindRecord(tile);
        if (rec == 0)
            return;
        tx = def->base_x + tile->b.x;
        ty = def->base_y + tile->b.y;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                rec->joined++;
                rec->timer = 400;
                b->flags |= 8;
                b->target.x = tx << 8;
                b->target.y = (ty - 5) << 8;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                b->f58 = 0;
                break;
            case 1: {
                BnvPos pos;
                screen = GetScreenCoordsForObject(tile, def);
                wy = b->world.y;
                wx = b->world.x;
                GetTileDimensions(&tw, &th);
                sx = (wx - wy) * tw >> 9;
                sy = (wx + wy) * th;
                if (sy) { }
                sy2 = sy >> 9;
                if (sy2) { }
                /* THE FAMILY-WIDE RESIDUAL, SOLVED.  The original stores sx
                 * into a frame home here and never reads it back, and carries
                 * one more dword nothing references at all -- the two halves
                 * of ONE EIGHT-BYTE spilled object, of which only .x is ever
                 * written.  An 8-byte local with a volatile store to its first
                 * member reproduces both: VC6 lifetime-colours it into the
                 * home `tile` has just stopped using (frame+0x04), and its
                 * second dword is the "phantom" slot at frame+0x08 that no
                 * scalar, array, unused local or inline-helper temporary could
                 * ever produce.  What the original's source spelled here is
                 * unknown; this reproduces its object exactly. */
                *(volatile int*)&spill.x = sx;
                sx2 = g_map_cfg->ox - Get_XScroll() + sx;
                sy2 += g_map_cfg->oy - Get_YScroll();
                if (sy2) { }
                sx2 -= g_sbarrel_pivot_x / 2;
                sx2 -= screen.ox;
                sy2 -= screen.oy;
                sy2 -= g_sbarrel_pivot_y / 2;
                /* The case-local seed permits the original person-load hoist. */
                pos.x = sx2 * 2;
                pos.y = sy2 * 2;
                b->person->zsprite = g_sbarrel_spr2;
                b->person->f30 = 1;
                b->person->depth = GetUnitDepth(-1617922.25f, -1618065.75f);
                b->b35 = 0;
                sprintf_w(&g_sbarrel_pathname[8], "%02d",
                          SpinningBarrels_SeatOf(r, rec,
                                                 (char)g_sbarrel_def->capacity));
                b->bnvpath = NewBNVPath(g_sbarrel_tab0, 0, g_sbarrel_pathname,
                                        -1617922.25f, -1618065.75f, &pos);
                b->action++;
                break;
            }
            case 2:
                b->flags |= 0x80;
                if (UpdateBlokeFromBNVPath(b, b->bnvpath) == 0) {
                    b->b35 = 1;
                    b->action = 6;
                }
                BlokeSetFrame(b, b->b74);
                break;
            case 6:
                BlokeSitAnim(b);
                BlokeSetFrame(b, 0);
                b->action++;
                rec->seated++;
                if ((short)(signed char)rec->seated == g_sbarrel_def->capacity)
                    SpinningBarrels_SetFull(rec);
                break;
            case 8: {
                BnvPos pos2;
                {
                    /* With pos2 case-local, natural x/y order also gives
                     * the original ascending short loads. */
                    int px = b->ride_dx * 2;
                    int py = b->ride_dy * 2;
                    pos2.x = px;
                    pos2.y = py;
                }
                BlokeWalkAnim(b);
                BlokeSetFrame(b, 0);
                b->person->zsprite = g_sbarrel_spr2;
                b->person->f30 = 1;
                b->person->depth = GetUnitDepth(-1617922.25f, -1618065.75f);
                b->b35 = 2;
                sprintf_w(&g_sbarrel_pathname[8], "%02d", b->seat);
                b->bnvpath = NewBNVPath(g_sbarrel_tab2, 2, g_sbarrel_pathname,
                                        -1617922.25f, -1618065.75f, &pos2);
                b->action++;
                break;
            }
            case 9:
                if (UpdateBlokeFromBNVPath(b, b->bnvpath) == 0) {
                    b->action = 15;
                    HeapFree_w(b->bnvpath);
                    b->bnvpath = 0;
                }
                BlokeSetFrame(b, b->b74);
                break;
            case 15:
                b->flags &= (unsigned short)~0x80u;
                ((unsigned char*)rec)[0x20 + b->seat] = 0;
                b->world.x = (tx << 8) + 0x280;
                b->world.y = (ty << 8) - 0x180;
                b->target.x = (tx << 8) + 0x180;
                b->target.y = (ty << 8) - 0x80;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 16:
                b->target.x = ((tx + 2) << 8) - ((rand_w() % 2) ? 0x80 : 0);
                b->target.y = (ty << 8) + 0x80;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 17:
                RemoveBlokeFromRide(def, r);
                b->flags &= (unsigned short)~8u;
                if (--rec->riders == 0) {
                    rec->seated = 0;
                    Ride_ClearFlagToNotLetAnyoneOn(&rec->tile);
                }
                break;
            }
        }
        r = next;
    }
    SpinningBarrels_TickMachine();
}

/* ==========================================================================
 * SPIDER RIDE -- the per-tick update. The same BNV-path machine as the Safari
 * (paths "manbox%02d" again, from its own two buffers), with a shorter state
 * machine because the Spider's riders do NOT walk to the ride: step 0 does the
 * bookkeeping AND the mount in one go, so a bloke that reaches the ride is
 * placed on the arm immediately.
 *   0   JOIN + MOUNT: count in, arm the 180-frame timer, mark "on this ride"
 *       and "riding", project the world position into screen space (biased by
 *       the ride's own pair at 0x0082c660/64), take the z-sprite and depth,
 *       ask 0x00416830(rider, square, capacity) for a seat and open path
 *       "manbox<seat>" from the ON buffer.
 *   1   RIDING (on): advance it; end of path, or frame past the seat's limit
 *       (0x004b4d9c = {0,32,28,20,24,12,16,20}), frees the path and steps to 5.
 *   5   SEATED: sit animation, fresh depth, count seated, full at capacity
 *       (0x00415a60).
 *   7/8 the mirror pair for getting off, limits at 0x004b4ddc.
 *   13  ALIGHT: free the seat byte, drop the z-sprite, un-adjust the screen
 *       position back to a map reference and walk to the queue/exit square.
 *   14  DONE: off the class list; reset the record when the last rider goes.
 * ========================================================================== */

extern void SpiderRide_TickMachine(void);                    /* 0x00416310 */
extern int  SpiderRide_SeatOf(RiderNode* r, SpiderRec* rec, char cap); /* 0x00416830 -- takes the RECORD: it indexes the seat bytes at rec+0x1c */
extern void SpiderRide_SetFull(SpiderRec* rec);              /* 0x00415a60 */

extern char g_spider_pathname[];                             /* 0x004b4d94 "manbox??" */
extern const int g_spider_end_on[8];                         /* 0x004b4d9c */
extern const int g_spider_end_off[8];                        /* 0x004b4ddc */

/* Scope F close (2026-09-05): 376/376 instructions,
 * 1228/1228 bytes, strict/rb/ob 0/0/0.
 *
 * Keep the BNV position objects inside their owning cases. Their earlier
 * function-wide lifetime falsely constrained stores before the first escape;
 * narrowing it restores the person/flag hoists and departure short-load order.
 * The separate person-cache workaround is no longer needed.
 *
 * Keep the unshifted y product in sy, with an empty-if consumer BOTH before
 * and after sy2 = sy >> 9. Then accumulate the origin-minus-scroll delta
 * into sy2, retaining the third empty-if before the pivot subtractions.
 * The two early consumers vanish only after the compiler has kept the
 * multiplication/shift webs separate. Both are required: neither or only
 * the post-shift consumer gives Spider 34; only the pre-shift one gives 343;
 * both give zero, and the same shape closes all five BNV ride callbacks.
 *
 * The 12-byte BNV seeds still leave z uninitialized, as the original does.
 * The eight-byte spill object and its single volatile sx store reproduce
 * the original dead store and unused frame dword; do not remove them.
 * All earlier scope, register-rotation and departure-floor claims in
 * this function's former notes are superseded. See docs/lanes/scope-f.md. */
// FUNCTION: LEGOLAND 0x00416330
void SpiderRide_Activate(RideElem* elem)
{
    RideDef*    def = elem->data;
    int         tw;
    int         th;
    RiderNode*  next;
    RideTile*   tile;
    Offset      screen;
    RiderNode*  r;
    SpiderRec*  rec;
    Bloke*      b;
    int         qx;
    int         qy;
    int         sx;
    int         sy;
    unsigned char dir;
    SpillPair   spill;
    int         wx;
    int         wy;
    int         sx2;
    int         sy2;

    r = def->riders;
    SpiderRide_TickMachine();
    while (r) {
        next = r->next;
        b = r->bloke;
        tile = RIDE_TILE(r);
        rec = SpiderRide_FindRecord(tile);
        if (rec == 0)
            break;
        if (b->state == 0) {
            switch (b->action) {
            case 0: {
                BnvPos pos;
                rec->joined++;
                rec->timer = 180;
                b->flags |= 8;
                screen = GetScreenCoordsForObject(tile, def);
                wy = b->world.y;
                wx = b->world.x;
                GetTileDimensions(&tw, &th);
                sx = (wx - wy) * tw >> 9;
                sy = (wx + wy) * th;
                if (sy) { }
                sy2 = sy >> 9;
                if (sy2) { }
                *(volatile int*)&spill.x = sx;
                sx2 = g_map_cfg->ox - Get_XScroll() + sx;
                sy2 += g_map_cfg->oy - Get_YScroll();
                if (sy2) { }
                sx2 -= g_spider_zframe / 2;
                sx2 -= screen.ox;
                sy2 -= g_spider_zstate / 2;
                sy2 -= screen.oy;
                pos.x = sx2 * 2;
                pos.y = sy2 * 2;
                /* the flag store is order-independent; below the seed stores
                 * is where the original schedules it (63 -> 42 on this alone
                 * once the y chain was in sy's register) */
                b->flags |= 0x80;
                b->person->zsprite = g_spider_tab3;
                b->person->f30 = 1;
                b->person->depth = GetUnitDepth(-1617787.75f, -1618096.5f);
                b->b35 = 0;
                sprintf_w(&g_spider_pathname[6], "%02d",
                          SpiderRide_SeatOf(r, rec,
                                            (char)g_spider_def->capacity));
                b->bnvpath = NewBNVPath(g_spider_tab1, 1, g_spider_pathname,
                                        -1617787.75f, -1618096.5f, &pos);
                UpdateBlokeFromBNVPath(b, b->bnvpath);
                b->action++;
                b->f58 = 0;
                break;
            }
            case 1:
                if (UpdateBlokeFromBNVPath(b, b->bnvpath) == 0) {
                    b->b35 = 1;
                    b->action = 5;
                    HeapFree_w(b->bnvpath);
                    b->bnvpath = 0;
                }
                if (b->bnvpath) {
                    if (BNVPath_GetDFrame(b->bnvpath)
                            >= g_spider_end_on[b->seat]) {
                        b->b35 = 1;
                        b->action = 5;
                        HeapFree_w(b->bnvpath);
                        b->bnvpath = 0;
                    }
                }
                BlokeSetFrame(b, b->b74);
                break;
            case 5:
                b->flags |= 0x80;
                BlokeSitAnim(b);
                BlokeSetFrame(b, 0);
                b->b35 = 1;
                b->person->zsprite = g_spider_tab3;
                b->person->f30 = 1;
                b->person->depth = GetUnitDepth(-1617787.75f, -1618096.5f);
                b->action++;
                rec->seated++;
                if ((short)(signed char)rec->seated == g_spider_def->capacity)
                    SpiderRide_SetFull(rec);
                break;
            case 7: {
                BnvPos pos2;
                /* Read both shorts before doubling into the case-local seed. */
                {
                    int px = b->ride_dx;
                    int py = b->ride_dy;
                    pos2.x = px * 2;
                    pos2.y = py * 2;
                }
                BlokeWalkAnim(b);
                BlokeSetFrame(b, 0);
                b->flags |= 0x80;
                b->person->zsprite = g_spider_tab3;
                b->person->f30 = 1;
                b->person->depth = GetUnitDepth(-1617787.75f, -1618096.5f);
                b->b35 = 2;
                sprintf_w(&g_spider_pathname[6], "%02d", b->seat);
                b->bnvpath = NewBNVPath(g_spider_tab2, 2, g_spider_pathname,
                                        -1617787.75f, -1618096.5f, &pos2);
                BNVPath_SetDFrame(b, b->bnvpath, 0);
                UpdateBlokeFromBNVPath(b, b->bnvpath);
                b->action++;
                break;
            }
            case 8:
                if (UpdateBlokeFromBNVPath(b, b->bnvpath) == 0) {
                    b->b35 = 2;
                    b->action = 13;
                    HeapFree_w(b->bnvpath);
                    b->bnvpath = 0;
                }
                if (b->bnvpath) {
                    if (BNVPath_GetDFrame(b->bnvpath)
                            >= g_spider_end_off[b->seat]) {
                        b->b35 = 2;
                        b->action = 13;
                        HeapFree_w(b->bnvpath);
                        b->bnvpath = 0;
                    }
                }
                BlokeSetFrame(b, b->b74);
                break;
            case 13:
                qx = def->qx + tile->b.x;
                qy = def->qy + tile->b.y;
                SPIDER_SEAT(rec, b->seat) = 0;
                b->flags &= (unsigned short)~0x80u;
                b->person->zsprite = 0;
                b->person->f30 = 0;
                UnAdjustBlokePosition(&b->person->screen);
                ScreenToMapRef(&b->person->screen, &b->world, 0);
                b->person->f34 = 0;
                /* world.y before target.y, then world.x, then target.x: the
                 * only one of the 24 orders that reproduces the original's
                 * interleave of the four shifts with the CalcMoveLine pushes
                 * (36 -> 17 together with the flag store moving first) */
                b->world.y = b->world.y << 8;
                b->target.y = qy << 8;
                b->world.x = b->world.x << 8;
                b->target.x = qx << 8;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 14:
                RemoveBlokeFromRide(def, r);
                b->flags &= (unsigned short)~8u;
                if (--rec->riders == 0) {
                    rec->seated = 0;
                    Ride_ClearFlagToNotLetAnyoneOn(&rec->tile);
                }
                break;
            }
        }
        r = next;
    }
}

/* ==========================================================================
 * PLANE RIDE (Zoomer) -- the per-tick update. Structurally the Spider's, with
 * three differences: the vehicle tick (0x0043e3f0) runs AFTER the rider walk
 * (and is skipped when a rider's square has no record, as in the Barrels);
 * the two ride-length limits are CONSTANTS (63 frames on, 32 off) rather than
 * per-seat tables; and the dismount path is seeded from the rider's own
 * offset pair run backwards through the view transform
 * (UnAdjustBlokePosition), not from a screen projection.
 *   0   JOIN + MOUNT (count in, 180-frame timer, project, seat from
 *       0x0043e050(rider, square, capacity), path "manbox%02d" from 0x0062fe88)
 *   1   RIDING (on)    -- ends at frame 63
 *   5   SEATED         -- full at capacity (0x0043d990)
 *   7   MOUNT (off)    -- path buffer 0x0062fe8c
 *   8   RIDING (off)   -- ends at frame 32
 *   13  ALIGHT         -- free the seat byte at record+0x1b+seat, hand the
 *                         bloke back to the walker at the queue square
 *   14  DONE
 * ========================================================================== */

#define PLANE_SEAT(rec, i) (((unsigned char*)(rec))[0x1b + (i)])

extern void PlaneRide_TickMachine(void);                     /* 0x0043e3f0 */
extern int  PlaneRide_SeatOf(RiderNode* r, PlaneRec* rec, char cap); /* 0x0043e050 -- takes the RECORD: it indexes the seat bytes at rec+0x1c */
extern void PlaneRide_SetFull(PlaneRec* rec);                /* 0x0043d990 */

extern char g_plane_pathname[];                              /* 0x004b79bc "manbox??" */
extern void* g_plane_tab1;                                   /* 0x0062fe88 */
extern void* g_plane_tab2;                                   /* 0x0062fe8c */

/* Scope F close (2026-09-05): 387/387 instructions,
 * 1253/1253 bytes, strict/rb/ob 0/0/0.
 *
 * Keep the BNV position objects inside their owning cases. Their earlier
 * function-wide lifetime falsely constrained stores before the first escape;
 * narrowing it restores the person/flag hoists and departure short-load order.
 * The separate person-cache workaround is no longer needed.
 *
 * Keep the unshifted y product in sy, with an empty-if consumer BOTH before
 * and after sy2 = sy >> 9. Then accumulate the origin-minus-scroll delta
 * into sy2, retaining the third empty-if before the pivot subtractions.
 * The two early consumers vanish only after the compiler has kept the
 * multiplication/shift webs separate. Both are required: neither or only
 * the post-shift consumer gives Spider 34; only the pre-shift one gives 343;
 * both give zero, and the same shape closes all five BNV ride callbacks.
 *
 * The 12-byte BNV seeds still leave z uninitialized, as the original does.
 * The eight-byte spill object and its single volatile sx store reproduce
 * the original dead store and unused frame dword; do not remove them.
 * All earlier scope, register-rotation and departure-floor claims in
 * this function's former notes are superseded. See docs/lanes/scope-f.md. */
// FUNCTION: LEGOLAND 0x0043e410
void PlaneRide_Activate(RideElem* elem)
{
    RideDef*    def = elem->data;
    int         tw;
    int         th;
    RiderNode*  next;
    RideTile*   tile;
    Offset      screen;
    RiderNode*  r;
    PlaneRec*   rec;
    Bloke*      b;
    int         qx;
    int         qy;
    int         sx;
    int         sy;
    unsigned char dir;
    SpillPair   spill;
    int         wx;
    int         wy;
    int         sx2;
    int         sy2;

    r = def->riders;
    while (r) {
        next = r->next;
        b = r->bloke;
        tile = RIDE_TILE(r);
        rec = PlaneRide_FindRecord(tile);
        if (rec == 0)
            return;
        if (b->state == 0) {
            switch (b->action) {
            case 0: {
                BnvPos pos;
                rec->joined++;
                rec->timer = 180;
                b->flags |= 8;
                screen = GetScreenCoordsForObject(tile, def);
                wy = b->world.y;
                wx = b->world.x;
                GetTileDimensions(&tw, &th);
                sx = (wx - wy) * tw >> 9;
                sy = (wx + wy) * th;
                if (sy) { }
                sy2 = sy >> 9;
                if (sy2) { }
                *(volatile int*)&spill.x = sx;
                sx2 = g_map_cfg->ox - Get_XScroll() + sx;
                sy2 += g_map_cfg->oy - Get_YScroll();
                if (sy2) { }
                sx2 -= g_plane_rider_dx / 2;
                sx2 -= screen.ox;
                sy2 -= g_plane_rider_dy / 2;
                sy2 -= screen.oy;
                pos.x = sx2 * 2;
                pos.y = sy2 * 2;
                /* order-independent; below the seed stores is where the
                 * original schedules it (95 -> 87 on this alone) */
                b->flags |= 0x80;
                b->person->zsprite = g_plane_zsprite;
                b->person->f30 = 1;
                b->person->depth = GetUnitDepth(-1617706.75f, -1617948.625f);
                b->b35 = 0;
                sprintf_w(&g_plane_pathname[6], "%02d",
                          PlaneRide_SeatOf(r, rec,
                                           (char)g_plane_def->capacity));
                b->bnvpath = NewBNVPath(g_plane_tab1, 1, g_plane_pathname,
                                        -1617706.75f, -1617948.625f, &pos);
                UpdateBlokeFromBNVPath(b, b->bnvpath);
                b->action++;
                b->f58 = 0;
                break;
            }
            case 1:
                if (UpdateBlokeFromBNVPath(b, b->bnvpath) == 0) {
                    b->b35 = 1;
                    b->action = 5;
                    HeapFree_w(b->bnvpath);
                    b->bnvpath = 0;
                }
                if (b->bnvpath) {
                    if (BNVPath_GetDFrame(b->bnvpath) >= 63) {
                        b->b35 = 1;
                        b->action = 5;
                        HeapFree_w(b->bnvpath);
                        b->bnvpath = 0;
                    }
                }
                BlokeSetFrame(b, b->b74);
                break;
            case 5:
                b->flags |= 0x80;
                BlokeSitAnim(b);
                BlokeSetFrame(b, 0);
                b->b35 = 1;
                b->person->depth = GetUnitDepth(-1617706.75f, -1617948.625f);
                b->action++;
                rec->seated++;
                if ((short)(signed char)rec->seated == g_plane_def->capacity)
                    PlaneRide_SetFull(rec);
                break;
            case 7: {
                Offset ofs;
                {
                    BnvPos pos2;
                    /* Both shorts are widened before doubling into the local offset. */
                    {
                        int px = b->ride_dx;
                        int py = b->ride_dy;
                        ofs.ox = px * 2;
                        ofs.oy = py * 2;
                    }
                    BlokeWalkAnim(b);
                    BlokeSetFrame(b, 0);
                    UnAdjustBlokePosition(&ofs);

                    pos2.x = ofs.ox;
                    pos2.y = ofs.oy;
                    b->flags |= 0x80;
                    b->person->zsprite = g_plane_zsprite;
                    b->person->f30 = 1;
                    b->person->depth = GetUnitDepth(-1617706.75f, -1617948.625f);
                    b->b35 = 2;
                    sprintf_w(&g_plane_pathname[6], "%02d", b->seat);
                    b->bnvpath = NewBNVPath(g_plane_tab2, 2, g_plane_pathname,
                                            -1617706.75f, -1617948.625f, &pos2);
                    BNVPath_SetDFrame(b, b->bnvpath, 0);
                    UpdateBlokeFromBNVPath(b, b->bnvpath);
                    b->action++;
                    break;
                }
            }
            case 8:
                if (UpdateBlokeFromBNVPath(b, b->bnvpath) == 0) {
                    b->b35 = 2;
                    b->action = 13;
                    HeapFree_w(b->bnvpath);
                    b->bnvpath = 0;
                }
                if (b->bnvpath) {
                    if (BNVPath_GetDFrame(b->bnvpath) >= 32) {
                        b->b35 = 2;
                        b->action = 13;
                        HeapFree_w(b->bnvpath);
                        b->bnvpath = 0;
                    }
                }
                BlokeSetFrame(b, b->b74);
                break;
            case 13:
                qx = def->qx + tile->b.x;
                qy = def->qy + tile->b.y;
                PLANE_SEAT(rec, b->seat) = 0;
                /* the flag clear FIRST and world.x before world.y: the
                 * best of the 48 measured head/shift orders (28 -> 21) */
                b->flags &= (unsigned short)~0x80u;
                b->person->zsprite = 0;
                b->person->f30 = 0;
                UnAdjustBlokePosition(&b->person->screen);
                ScreenToMapRef(&b->person->screen, &b->world, 0);
                b->person->f34 = 0;
                b->world.x = b->world.x << 8;
                b->world.y = b->world.y << 8;
                b->target.x = (qx << 8) + 0x80;
                b->target.y = (qy << 8) + 0x80;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 14:
                b->flags &= (unsigned short)~8u;
                RemoveBlokeFromRide(def, r);
                if (--rec->riders == 0) {
                    rec->seated = 0;
                    Ride_ClearFlagToNotLetAnyoneOn(&rec->tile);
                }
                break;
            }
        }
        r = next;
    }
    PlaneRide_TickMachine();
}

/* SPINNING BARRELS' draw is the most layered of the six: it collects this
 * square's riders into a 15-entry array and then walks that array FIVE times,
 * once per state-machine step that has to be drawn at a particular depth
 * (0, 1, 15, 16, 17), interleaving the three sprite layers between the passes:
 *   layer 3 (the barrels) -> riders that are ON the ride, positioned by the
 *   ride's pivot -> steps 0/1/15 -> the front matte -> layer 2 -> steps 16/17
 *   -> the second matte.
 * As in the Safari and the Plane, the record's layer-3 frame is published into
 * the z-sprite before the riders are drawn. Note both PrintSprite calls pass
 * blit mode 0 rather than the caller's mode -- this ride ignores it. */

extern ZSprite* g_sbarrel_zspr_obj;                          /* 0x0062fe00 */
extern Offset   g_sbarrel_pivot;                             /* 0x0062fdd8 (+4 = oy); the 0x62fdd8/dc pair as one Offset */

/* Position one riding bloke (the Plane's lever: a static __inline helper so
 * `piv` is an inline-expansion temporary outside the caller's alias class,
 * which lets the scheduler interleave the p load and the pivot stores the
 * way the original does). */
static __inline void SpinningBarrels_PlaceRider(Bloke* b, Offset* screen)
{
    Offset    piv = g_sbarrel_pivot;
    Person3D* p = b->person;

    p->local.ox = b->ride_dx;
    p->local.oy = b->ride_dy;
    AdjustBlokePosition(&p->local);
    AdjustOffsetForViewMode(&piv);
    p->screen.ox = b->ride_dx + piv.ox + screen->ox;
    p->screen.oy = b->ride_dy + piv.oy + screen->oy;
    AdjustBlokePosition(&p->screen);
}

/* CLOSED (373/373). 39 -> 0 in three steps, all frame/alias levers:
 * (1) TWO Offset locals, split by BLIT, not by kind: the original keeps the
 *     layer-3 blit's offset (both arms) in one home (0x20) and the two matte
 *     blits' AND the layer-2 blit's offset in another (0x18). One `off` for
 *     everything gives a frame 8 bytes short; `off` for layer 3 + `moff` for
 *     the mattes only leaves the layer-2 blit on the wrong home (20 X).
 *     Declaration order and block scope of the two are inert (four forms all
 *     exact); wrapping the layer-3 blit in its own inner scope costs 37.
 * (2) The rider pivot is ONE 8-byte global (0x62fdd8/dc) copied whole:
 *     `Offset piv = g_sbarrel_pivot;` declared BEFORE `p`. That is what
 *     issues the loads in DESCENDING displacement order (0x62fddc first) with
 *     the `p = b->person` load between them (the Plane's ofs/p interleave
 *     with the pair reversed). Scalar stores in either order, or the struct
 *     copy as a statement after `p`, leave the pair ascending (+2/+4).
 * (3) The band loops are `char i` (transfer #1) -- already in place.
 * Also measured and rejected: the layer blits or the matte blits as a
 * static __inline helper (loses the shared LLSSetFrame/GetLLS scheduling,
 * 362/158 X); a third Offset for the second matte (61). */
// FUNCTION: LEGOLAND 0x0043be70
void SpinningBarrels_Interact(RideElem* elem, int x, int y, RideTile* sq,
                              void* clip, int mode)
{
    RideDef*    item = elem->data;
    LayerOut    lay;
    Offset      off;         /* the layer-3 blit's offset (frame 0x20) */
    Offset      moff;        /* the matte and layer-2 blits' offset (0x18) */
    Offset      screen;
    RiderNode*  r;
    SBarrelRec* rec;
    char        i;

    r = item->riders;
    {
    char        n = 0;
    Bloke*      found[16] = { 0 };

    rec = SpinningBarrels_FindRecord(sq);
    if (rec == 0)
        return;
    screen = GetScreenCoordsForObject(sq, item);
    GetLayer(item->sprite, &lay, 3);
    lay.f10 = 0;                 /* +0x10, not dy: lay sits at 0x38 and the
                                  * store goes to [esp+0x48] */
    if (r) {
        while (r) {
            if (sq->key == r->ride_id)
                found[n++] = r->bloke;
            r = r->next;
        }
        if (n) {
            LLSSetFrame(GetLLSForLayer(g_sbarrel_layers, 3), rec->frame3);
            off = GetRenderOffsetForLayer(g_sbarrel_layers, 3);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_sbarrel_layers, 3),
                        screen.ox + off.ox, screen.oy + off.oy, 0, 0);
            r = item->riders;
            **g_sbarrel_zspr_obj->pframe = (unsigned short)rec->frame3;
            for (; r; r = r->next) {
                if (sq->key == r->ride_id && (r->bloke->flags & 0x80)) {
                    SpinningBarrels_PlaceRider(r->bloke, &screen);
                    IP_RenderBlokeIn3DNow(r->bloke);
                }
            }
            for (i = 0; i < n; i++) {
                if (found[i]->action == 0)
                    IP_RenderBlokeIn3DNow(found[i]);
            }
            for (i = 0; i < n; i++) {
                if (found[i]->action == 1)
                    IP_RenderBlokeIn3DNow(found[i]);
            }
            for (i = 0; i < n; i++) {
                if (found[i]->action == 15)
                    IP_RenderBlokeIn3DNow(found[i]);
            }
            moff = GetRenderOffsetForLayer(g_sbarrel_layers, 1);
            AdjustOffsetForViewMode(&moff);
            PrintSprite(g_sbarrel_spr0, moff.ox + screen.ox,
                        moff.oy + screen.oy, 0, 0);
            LLSSetFrame(GetLLSForLayer(g_sbarrel_layers, 2), rec->frame2);
            moff = GetRenderOffsetForLayer(g_sbarrel_layers, 2);
            AdjustOffsetForViewMode(&moff);
            PrintSprite(GetSpriteForLayer(g_sbarrel_layers, 2),
                        screen.ox + moff.ox, screen.oy + moff.oy, 0, 0);
            for (i = 0; i < n; i++) {
                if (found[i]->action == 16)
                    IP_RenderBlokeIn3DNow(found[i]);
            }
            for (i = 0; i < n; i++) {
                if (found[i]->action == 17)
                    IP_RenderBlokeIn3DNow(found[i]);
            }
            moff = GetRenderOffsetForLayer(g_sbarrel_layers, 1);
            AdjustOffsetForViewMode(&moff);
            PrintSprite(g_sbarrel_spr1, moff.ox + screen.ox,
                        moff.oy + screen.oy, 0, 0);
            return;
        }
    }
    LLSSetFrame(GetLLSForLayer(g_sbarrel_layers, 3), rec->frame3);
    off = GetRenderOffsetForLayer(g_sbarrel_layers, 3);
    AdjustOffsetForViewMode(&off);
    PrintSprite(GetSpriteForLayer(g_sbarrel_layers, 3),
                screen.ox + off.ox, screen.oy + off.oy, 0, 0);
    LLSSetFrame(GetLLSForLayer(g_sbarrel_layers, 2), rec->frame2);
    moff = GetRenderOffsetForLayer(g_sbarrel_layers, 2);
    AdjustOffsetForViewMode(&moff);
    PrintSprite(GetSpriteForLayer(g_sbarrel_layers, 2),
                screen.ox + moff.ox, screen.oy + moff.oy, 0, 0);
    }
}
