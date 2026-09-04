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
extern void  AddBasicObject(void* obj, Pos* pos);            /* 0x0045efe0 */

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

extern void  KillSprite(void* sprite);                       /* 0x00497bd0 */
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
extern void*  g_copters_path0;                               /* 0x004c1124 */
extern void*  g_copters_path1;                               /* 0x004c1128 */
extern void*  g_copters_path2;                               /* 0x004c1130 */
extern void*  g_copters_path3;                               /* 0x004c1134 */
extern void*  g_copters_path4;                               /* 0x004c112c */
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
    if (g_copters_path2)
        FreeWalkPath(g_copters_path2);
    if (g_copters_path3)
        FreeWalkPath(g_copters_path3);
    if (g_copters_path4)
        FreeWalkPath(g_copters_path4);
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

extern RideDrawDesc g_copters_draw;                          /* 0x004c1170 */
extern void*        g_copters_layers;                        /* 0x004c1138 */

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
extern void*  g_copters_poly0;                               /* 0x004b41c8 (6 pts) */
extern void*  g_copters_poly1;                               /* 0x004b4208 (7 pts) */
extern void*  g_copters_poly2;                               /* 0x004b4240 (6 pts) */
extern void*  g_copters_poly3;                               /* 0x004b4270 (5 pts) */
extern void*  g_copters_poly4;                               /* 0x004b4298 (4 pts) */
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
    g_copters_path4 = BuildWalkPath(&g_copters_poly1);
    g_copters_path1 = BuildWalkPath(&g_copters_poly2);
    g_copters_path2 = BuildWalkPath(&g_copters_poly3);
    g_copters_path3 = BuildWalkPath(&g_copters_poly4);
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
    g_plane_bnv0 = g_plane_bnv_ride;
    g_plane_bnv1 = g_plane_bnv_on;
    g_plane_bnv2 = g_plane_bnv_off;
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
// WIP-FUNCTION: LEGOLAND 0x0043bac0  (38%, 138/222; frame, block layout, the ebx/ebp cursor split and everything to index 22 exact -- see above)
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
        tilex = tile->b.x;
        tx = def->base_x + tilex;
        tiley = tile->b.y;
        base[1] = (*(RideDef* volatile*)&def)->base_y + tiley;
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
            case 3:
                seat = b->seat;
                b->target.x = (g_tower_seat[seat].dx + tilex) << 8;
                b->target.y = (g_tower_seat[seat].dy + tiley) << 8;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)g_tower_car[seat >> 1].dir);
                b->action++;
                break;
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
                ((unsigned char*)rec)[0x20 + b->seat] = 0;
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
                b->target.x = ((def->qx + *(volatile int*)&tilex) << 8) + 0x80;
                b->target.y = ((def->qy + tiley) << 8) + 0x80;
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
extern void  Copters_StepRider(RiderNode* r);                /* 0x00403d30 */
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
                case 0: path = g_copters_path2; break;
                case 1: path = g_copters_path0; break;
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
                case 0: path2 = g_copters_path2; break;
                case 1: path2 = g_copters_path0; break;
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
extern void   ScreenToMapRef(Offset* screen, Pos* out, int z);/* 0x0045be90 */
extern void   HeapFree_w(void* p);                           /* 0x0049e4d0 */
extern int    sprintf_w(char* dst, const char* fmt, int v);  /* 0x0049e573 */
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

/* STATE: lpConfig is read through its POINTER (a semantic fix, see
 * SpinningBarrels_Activate), the MOUNT block reads world.x/y into wx/wy
 * before GetTileDimensions, and unlike the other three BNV rides
 * SafariRide_SeatOf really does take the rider's tile (0x00415760 reads
 * [arg2] as the key), so that call stays two-argument.
 * THIS ROUND (audit 333 -> 143, frame 0x34 -> 0x38 == the original, byte
 * length 1291 vs 1292): the whole head (indices 0-84) is now exact.  Two
 * transfers did it, and they had to go in together (measured as a 2x2x2 grid,
 * every other cell 249-340):
 *   1. `SpillPair spill; *(volatile int*)&spill.x = sx;` for the original's
 *      dead sx store at index 76 and the phantom dword that follows it (see
 *      the SpinningBarrels_Activate note, items 4-5), AND
 *   2. a FRESH `sy2` for the y chain (with `sy` reused the strict count is
 *      249), keeping the pivot-before-screen.oy order.
 *   3. The BNV seed is a 12-byte `BnvPos` here too.
 * NEXT ROUND (audit 143 -> 137, byte length now 1292 == 1292).  The y chain
 * is built FROM sy with the empty-`if` flatten breaker and `p = b->person;`
 * is cached for the z-sprite store (SpinningBarrels_Activate items 9-11).
 * The two MUST go in together here: the y form ALONE measures 227, because
 * it re-phases case 2 so that the second `b35=1; action=5; HeapFree; bnvpath
 * = 0` arm gets cross-jumped into the first (a 3-instruction stub plus an
 * out-of-line block) where the original keeps a full inline copy ending in
 * its own `BlokeSetFrame` + `jmp`; the `p` cache restores the phase.  Worth
 * recording, because the y form alone gets indices 155-183 exact
 * register-for-register and cuts the head residual from ~45 to 12 -- if the
 * case-2 layout can be pinned some other way the function should fall a long
 * way.  Measured against that layout and rejected: a local for the freed
 * path, statement order inside the arm, `r->bloke->` at both stores, a
 * volatile read of bnvpath, `&&` instead of the nested `if`, `else { }`, and
 * duplicating `BlokeSetFrame(b, b->b74); break;` into the arm (that gets the
 * arm right but merges the two `add esp` cleanups into `add esp,0xc` and
 * emits a third copy of the tail -> ESCAPES).
 * Two more statement-order fixes after that (137 -> 132): the case-13 flag
 * clear moved before the z-sprite store, and case 7's `b->flags |= 0x80;`
 * moved ABOVE the pos2 block.
 * WHAT IS LEFT (132): indices 86-124 are the last y-block items the Barrels
 * also has (the scroll subtraction's position, plus here the pos.x store and
 * the screen.oy reload); from 132 on it is one register rotation through
 * case 2, case 5 and case 7 -- the whole tail is register-blind clean
 * (rb=34 against 132 strict), so it is a single allocator phase step, not a
 * structural difference.  NOTE the Safari is the ONLY one of the four that
 * still needs `p = b->person;` (without it the y form regresses to 227 by
 * re-phasing case 2) and the only one whose head does NOT respond to the
 * volatile-rec removal -- its SeatOf takes the tile, so there is no rec
 * argument to spill in the first place.  The remaining rotation is most
 * likely one scratch temp too many or too few somewhere in case 1; the
 * `stub each case and watch a head register` diagnostic that cracked the
 * Plane is the thing to run here next.
 * ROUND 6 (2026-09-04, unchanged at 132).  Two things were pinned down:
 *   - THE TAIL ROTATION IS `p = b->person;` LANDING TWELVE SLOTS EARLY.
 *     Ours emits it at index 91 (into ecx, the register freed by the
 *     `mov ax,[cfg+0x22]`); the original emits it at 103 (into edx, the
 *     register the second `cdq` had just freed).  From there the whole tail
 *     is the map orig ecx -> ours eax, orig edx -> ours ecx, orig eax ->
 *     ours edx, i.e. we are ONE step behind in the eax/ecx/edx rotation, and
 *     that alone accounts for indices 132-379.  The load's position is a
 *     SCHEDULER decision and is completely insensitive to source placement:
 *     `p` before the pos stores, between them, after them, hoisted above the
 *     four subtractions, and dropped entirely all give 132 (dropping it or
 *     using `p` for f30/depth costs 17 instructions' worth of extra
 *     `[esi+4]` reloads and measures 227-303).  Per the scheduler lever in
 *     docs/DECOMP.md this wants a no-code IR tuple EARLIER in the function,
 *     not a different statement order here.
 *   - The y block's own residual (86-124) is the family-wide scroll
 *     subtraction; the one-web y chain described in the round-6 paragraph of
 *     SpinningBarrels_Activate moves this function's two GetUnitDepth pushes
 *     onto indices 86/89 where the original has them and HALVES the
 *     register-blind residual (rb 34 -> 18, shp 34 -> 18), but brings the
 *     same three-cycle callee-saved rotation, so audit goes 132 -> 149.
 * SHIMS RE-TESTED IN THIS STATE and all still carrying their weight:
 * removing `*(volatile int*)&spill.x = sx;` costs the frame (0x38 -> 0x34)
 * and measures 331; `spill.y` 133; spilling `sy` instead 147; reusing `sy`
 * for the whole y chain 148. */
/* ROUND 7 (2026-09-04, unchanged at 132).  Nothing new committed.  The y
 * block (86-124) is the family residual: see the ROUND 7 paragraph in
 * SpinningBarrels_Activate, which now proves the rotation is decided inside
 * this one block and that it is triggered by the y accumulator being ONE web
 * with the later subtractions kept separate -- not by the compound itself.
 * The tail rotation is still the `p = b->person;` load landing at index 91
 * instead of 103.  Note for the next lane: the original stores pos.x EARLY
 * (index 106) and then RECYCLES edi for the screen.oy reload (107), where
 * ours keeps screen.oy in ebp the whole time -- i.e. the original is one live
 * value SHORT of registers here where we are not, which is the same
 * "one temp too few" symptom recorded in round 5 and is probably the same
 * cause as the early `p` load. */
/* ROUND 8 (2026-09-04, unchanged at 132).  This function is now the best
 * INSTRUMENT for the family residual, because the one-web y chain here is
 * worth rb 386 -> 395 and bad 16 -> 7 (i.e. only SEVEN original indices are
 * left in a differing region register-blind) while the strict count goes
 * 132 -> 149.  What the one-web chain fixes here, on top of the two `sub`s:
 * indices 86-90 become exact, so BOTH GetUnitDepth pushes land where the
 * original has them, and screen.oy stops being loaded 20 slots early.  What
 * it costs is the three-cycle callee-saved rotation (orig ebx -> ours edi,
 * orig ebp -> ours ebx, orig edi -> ours ebp).
 * A named `ys` local (`ys = Get_YScroll(); sy += g_map_cfg->oy - ys;`)
 * UNDOES the rotation completely -- indices 60-84 then match register for
 * register -- but with the block split gone VC6 flattens the four later
 * subtractions into the delta.  The 32-cell sweep of {four breaker forms} x
 * {eight positions} shows the breaker is a pure block split (`if (sx2){}`
 * and `if (sy){}` are byte-identical) and that its position selects one of
 * three register attractors; none of the 32 keeps both the separate
 * subtractions and the original naming.  See the ROUND 8 paragraph in
 * SpinningBarrels_Activate for the full list of what else was measured. */
/* ROUND 9 (2026-09-04, unchanged at 132).  The tail rotation is now pinned to
 * a single load, and the one-web chain is the instrument that shows it:
 *   - Committed body: real=132 strict=132 rb=15 (IDENTITY permutation).
 *     One-web: real=131 strict=149 rb=6 (perm=bpdibxsi).  Under one-web the
 *     REGISTER-BLIND structural residual over all 402 instructions is only
 *     THREE sites: `mov edx,[esi+4]` (the `p` load) emitted at index 91 where
 *     the original has it at 103, the `g_safari_zspr` load one slot out, and
 *     the `mov byte [eax+0x35],0` / `lea eax,[edi+0xc]` pair at 121.
 *     Everything else agrees register-blind.
 *   - The `p` load's position is NOT reachable from source order, and this is
 *     now exhaustive: all 140 interleavings of {the four subtractions,
 *     `p = b->person`, `pos.x`, `pos.y`} that keep each axis in order are
 *     BYTE-IDENTICAL (real 131 / strict 149 / rb 6, identical bad lists).
 *     That also refutes the reason this note used to give for the cache: the
 *     escaped `pos` stores are NOT a schedule barrier, VC6 hoists the load
 *     straight over them.  What the cache turns on is COPY-PROPAGATION
 *     DISTANCE -- `p = b->person;` immediately before its only use is
 *     propagated away and measures 234 (one-web) / 227 (two-web), i.e.
 *     exactly the same as no cache; with any statement in between it survives
 *     and measures 131 / 132.  Also inert: a `static __inline void
 *     SeedBnvPos(BnvPos*, int, int)` for the two seed stores, in both y-chain
 *     shapes and with `p` on either side of it -- so the "arguments become
 *     temporaries" lever adds no IR tuples at this site.
 *   - Where the load lands is the stall after `mov ax,[cfg+0x22]` (a
 *     partial-register write followed by a full-register read); the original
 *     leaves that stall unfilled and emits the load at the next one, after
 *     the second `sub eax,edx`.  Per the Pentium-scheduler lever in
 *     docs/DECOMP.md that wants a no-code IR tuple EARLIER in the function,
 *     not a different statement order in this block -- that is the next thing
 *     to try, and this function is the cheapest place in the file to try it
 *     because its structural residual is down to three sites. */
/* PASS w8rides (2026-09-04).  NO CHANGE (132 strict, 132 real under the
 * IDENTITY permutation, register-blind 15).  Two things are now settled.
 *
 *  (1) THE RESIDUAL IS ONE CLUSTER PLUS A SCRATCH ROTATION.  Register-blind
 *      distance is 15 of 402 and a full seven-register permutation search
 *      (scratch AND callee-saved) puts the residual at 60, with the winning
 *      scratch map a THREE-CYCLE eax->edx->ecx->eax.  So roughly half the 132
 *      is one rotation of the scratch registers through the whole tail, not
 *      132 different instructions.  The structural part is indices 86-124:
 *      the original interleaves GetUnitDepth's two float-constant pushes
 *      (0xc9c582b0 at index 86, 0xc9c57bd8 at 89 -- thirty instructions
 *      before the call at 116) into the x/y chains, and hoists
 *      `mov edx,[esi+4]` into the second division's latency gap at 103; we
 *      emit the pushes at 101/107 and the `b->person` load at 91.  This is
 *      the same phenomenon as SpiderRide's 87-97 and SpinningBarrels'
 *      125-133 windows: VC6 and the original disagree about WHICH free slot a
 *      constant push or an independent load falls into, with identical
 *      instructions and identical registers either side.
 *
 *  (2) THE COMPOUND Y CHAIN IS DECISIVELY WRONG HERE.  The original's index
 *      91 is `sub eax,edx` = `g_map_cfg->oy - Get_YScroll()` as ONE
 *      difference, then `add ebx,eax`; ours splits it (`sub ebx,edx` then
 *      `add ebx,eax`).  That invites the SpiderRide ROUND-10 compound
 *      rewrite, and it must not be taken: TEN spellings of it were measured
 *      and they collapse to just TWO objects, 249 and 236, both far worse
 *      than 132 AND both 17-19 bytes over the original's 1292.
 *      `sy2 = g_map_cfg->oy - Get_YScroll() + sy;`, `sy2 = sy + (oy - ysc)`,
 *      `sy2 = sy; sy2 += oy - ysc;`, `sy2 = sy + oy - ysc;` and the first two
 *      without the empty-`if` flatten breaker are ALL ONE OBJECT at 249;
 *      hoisting the call into its own `int ysc = Get_YScroll();` statement
 *      first, in four arrangements, is ONE OBJECT at 236.  The byte blow-out
 *      is the tell: making the difference compound moves the Get_YScroll call
 *      inside the sum's evaluation and the whole case-0 frame reshuffles.  So
 *      the single-`sub` shape the original has is NOT reachable by spelling
 *      the expression compound at this site, unlike SpiderRide where the same
 *      rewrite at least holds the byte length. */
/* PASS w9rides (2026-09-04).  NO CHANGE (132 strict, 132 real under the
 * IDENTITY permutation, register-blind 15).  This function is the clearest
 * instance of the family slot, and the reading given in the previous pass is
 * confirmed and sharpened: the original leaves the X slot EMPTY and fills
 * the Y slot with `mov edx,[esi+4]` -- the `p` load -- at index 103, while
 * we fill the X slot with the screen.ox reload at 96 and put `p` at 91.  Its
 * `p` load position and the two float-constant pushes are DOWNSTREAM of that
 * one slot, not independent items; see the PASS w9rides paragraph in
 * SpinningBarrels_Activate for the aligned five-function table and the
 * negatives.
 * New here and all INVARIANT at 132 (byte length exact in every cell): the
 * folded-store form for both axes, for x only and for y only, with `p`
 * before / between / after the folded stores; per-axis interleaved blocks
 * (`sx2 -= ..; sx2 -= ..; pos.x = sx2 * 2;` then the y pair), y-axis first;
 * and the Carousel_Tick spelling of the y chain -- the product assigned
 * straight into `sy2` with no `sy` at all and a compound
 * `sy2 += g_map_cfg->oy - Get_YScroll();` -- which reproduces the one-web
 * numbers exactly (real 131 / strict 149 / rb 6, perm bpdibxsi) whether or
 * not an empty `if` follows it, i.e. the empty `if` is INERT in the one-name
 * spelling.  Dropping the cache (`b->person->zsprite`) or moving it below
 * both pos stores is 227 in the folded form as it is in the statement form.
 * Doing the same to the X chain as well (`sx2` used for the product and the
 * spill) is 382 and moves the prologue. */
/* PASS w10rides (2026-09-05).  NO CHANGE (132 strict, 132 real under the
 * IDENTITY permutation, register-blind 15) -- but THE 132 IS NOW KNOWN TO BE
 * ONE REGISTER CHOICE, not 132 problems, and the whole tail was read against
 * the original instruction by instruction to prove it.
 *
 * THE SEED: `p = b->person;` is allocated ECX here and EDX in the original.
 * The original loads it at index 103, in the family Y slot
 * (`mov edx,[esi+4]` between `sub eax,edx` and `sar eax,1`); we load it at
 * 91, as early as the `g_map_cfg` pointer frees ECX at 90.  From index 113
 * (`mov [ecx+0x2c],edx` against the original's `mov [edx+0x2c],eax`) to the
 * end of the case the two bodies are the SAME INSTRUCTIONS WITH THE SAME
 * OPERANDS in a three-way eax/ecx/edx rotation -- 145/146, 149/151, 159/161,
 * 167/169, 179/181/182, 184/186, 191/192/193 and so on are all
 * `mov <scratch>,[esi+0x54]` / `push <scratch>` pairs that differ only in
 * which scratch register they name.  That is what register-blind 15 against
 * strict 132 is measuring, and it is why this body scores so much worse than
 * its four siblings while being no further from the original structurally.
 *
 * ALSO READ OFF THE DISASSEMBLY THIS PASS (and not reproducible): the
 * original stores `pos.x` at index 106, BEFORE the y chain finishes, and
 * then reloads `screen.oy` into EDI -- the register `sx2` has just vacated
 * at `shl edi,1` / `mov [esp+0x48],edi`.  We hoist `screen.oy` into EBP at
 * index 86 instead and keep both pos stores together at 111/112.  The
 * obvious source for the original's shape, the per-axis tail
 * (`sx2 -= ..; sx2 -= ..; pos.x = sx2 * 2;` then the y triple), is
 * BYTE-IDENTICAL to the committed body -- measured this pass in six tail
 * spellings x three y-chain shapes (18 cells): every two-web cell with the
 * cache is exactly 132 (the per-axis one with `p` between the two axes is
 * 132 with rb 13 instead of 15), the no-cache cells are 227, and the
 * one-web cells are 138 (no cache) or 249 (cache).  So the interleave is a
 * scheduler consequence of the EDI allocation, not a statement order.
 *
 * The family cause is the same as everywhere else: see the PASS w10rides
 * paragraph in SpinningBarrels_Activate.  The `p` cache must stay -- without
 * it the load sinks below the two escaped `pos` stores and the body is
 * 227. */
// WIP-FUNCTION: LEGOLAND 0x00415220  (67%, 132/402; frame, byte length and the head exact, a tail register rotation remains -- see above)
void SafariRide_Activate(RideElem* elem)
{
    RideDef*    def = elem->data;
    RideTile*   tile;
    int         tw;
    int         th;
    RiderNode*  next;
    Offset      screen;
    BnvPos      pos;
    BnvPos      pos2;
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
    Person3D*   p;
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
            case 1:
                screen = GetScreenCoordsForObject(tile, def);
                wy = b->world.y;
                wx = b->world.x;
                GetTileDimensions(&tw, &th);
                sx = (wx - wy) * tw >> 9;
                sy = (wx + wy) * th >> 9;
                *(volatile int*)&spill.x = sx;
                sx2 = g_map_cfg->ox - Get_XScroll() + sx;
                /* See SpinningBarrels_Activate: scroll subtracted from sy
                 * first, origin added back, empty `if` as the flatten breaker;
                 * `p` cached only for the z-sprite store so its load can be
                 * hoisted above the two escaped `pos` stores. */
                sy2 = sy - Get_YScroll();
                sy2 += g_map_cfg->oy;
                if (sy2) { }
                sx2 -= g_safari_ofs2.ox / 2;
                sx2 -= screen.ox;
                sy2 -= g_safari_ofs2.oy / 2;
                sy2 -= screen.oy;
                p = b->person;
                pos.x = sx2 * 2;
                pos.y = sy2 * 2;
                p->zsprite = g_safari_zspr;
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
            case 7:
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

/* STATE (this round: audit 344 -> 84). Three things closed the bulk, and all
 * three transfer to the Safari/Spider/Plane twins:
 *   1. SpinningBarrels_SeatOf takes the RECORD, not the rider's tile: the
 *      callee indexes the seat bytes at rec+0x21 (0x0043ce10 disassembled),
 *      and the original reloads rec from its 0x24 home for the push. The old
 *      `SeatOf(r, tile, cap)` was a semantic bug (it handed the callee r+0xc).
 *      That reload is also WHY the original homes rec right after FindRecord.
 *   2. lpConfig (0x004bcbf4) is a POINTER to the map config (`mov eax,[..];
 *      mov di,[eax+0x20]`), not the struct; g_map_cfg->ox.
 *   3. The MOUNT block reads world.x/world.y into `wx`/`wy` BEFORE
 *      GetTileDimensions, and the scroll stage assigns FRESH variables
 *      (`sx2 = cfg->ox - Get_XScroll() + sx; sy2 = ...`): with `sx` reused
 *      the sum lands in sx's register, where the original puts it in the
 *      register that computed (ox - xscroll) and lets sx die -- the two-def
 *      shape. This is what flips def/rec back to ebx/ebp.
 * THIS ROUND (audit 69 -> 25). The frame is now byte-for-byte the original's
 * (0x3c, 1148 B == 1148 B) and every [esp+N] displacement matches:
 *   4. The BNV seed is TWELVE bytes, not eight (`BnvPos`): the never-written
 *      third dword above each of `pos` (frame+0x24) and `pos2` (frame+0x30)
 *      is its z.  That closed two of the three "dead homes".
 *   5. The third one is not a variable at all: it is the SECOND HALF of an
 *      8-byte object the original spills sx into.  `SpillPair spill;` plus
 *      `*(volatile int*)&spill.x = sx;` reproduces both the dead
 *      `mov [esp+0x24],ebp` and the phantom dword: VC6 lifetime-colours the
 *      pair onto the home `tile` has just stopped using, so .x lands on
 *      tile's slot (frame+0x04) and .y is the phantom (frame+0x08).  Measured
 *      and rejected first: unused locals of every type (dropped), an
 *      address-taken scalar through a no-op `static __inline` (dropped), a
 *      12-byte struct for tw/th with only &member taken (VC6 drops the unused
 *      member), `int dim[3]` and a whole-struct touch (both reserve, but an
 *      aggregate goes to the TOP of the frame, not into the scalar pool).
 *   6. SpinningBarrels_SeatOf's third parameter is a `char`: the original
 *      pushes `mov al,[def+0x2e] / push eax` with no zero-extension, which
 *      only a char-typed parameter fed the truncated `short` field produces.
 *   7. Case 8 stores pos2.y BEFORE pos2.x -- with the natural order VC6 sorts
 *      the two movsx loads by descending displacement (+0x3e first).
 *   8. Of the four subtraction statements only the RELATIVE order of the two
 *      sy2 ones matters (all 24 permutations measured): screen.oy before the
 *      pivot half is what hoists its load above the x pivot.
 * ROUND 5 (audit 25 -> 19).  THE FAMILY-WIDE Y-CHAIN RESIDUAL IS SOLVED here
 * and transfers to all four BNV _Activate callbacks; the two levers are:
 *   9. THE Y CHAIN IS ACCUMULATED FROM sy, NOT FROM THE DELTA.  Spelled
 *      `sy2 = sy + (cfg->oy - Get_YScroll())` the parenthesised delta is a
 *      compiler TEMPORARY, so it wins the rank-1 destination copy, sy2
 *      coalesces with IT and the sum lands in a scratch register (`add
 *      ecx,ebx`).  Spelled as two statements that start FROM sy --
 *          sy2 = sy - Get_YScroll();
 *          sy2 += g_map_cfg->oy;
 *      -- the first operation is in-place on sy, sy2 coalesces with sy, and
 *      the whole chain runs in sy's own callee-saved register exactly as the
 *      original does (`add ebx,edx`, `sub ebx,eax`, `sub ebx,ebp`).  The
 *      value is identical: (sy - yscroll) + oy == sy + (oy - yscroll).
 *      A lab of ~40 isolated spellings (named delta local, `int da[2]`, a
 *      Pos field, `short` delta, volatile on either side, `sy - (yscroll -
 *      oy)`, all six textual orders) confirms NOTHING ELSE moves that
 *      destination: VC6 forward-substitutes every named delta back into a
 *      temporary first.  In-place on `sy` itself also gets the add right but
 *      merges sy's web with wx's and rotates the whole wx/wy/sx/sy register
 *      triple (37 here, and it is what the Spider had).
 *  10. THE EMPTY `if (sy2) { }` IS LOAD-BEARING.  Without it VC6 flattens the
 *      two later `sy2 -=` statements back into that sum and re-sorts the
 *      terms (screen.oy gets folded into the oy load, `sub ecx,eax`).  The
 *      `if` is a second consumer of sy2 evaluated before the subtractions, so
 *      the reassociation is refused; the branch itself is deleted afterwards
 *      and costs zero instructions (`if (sy2) ;`, `{ }` and `!= 0` are
 *      byte-identical).  Same mechanism as DrawPopUpMock's `(X+c1)-c2`.
 *  11. `p = b->person;` CACHED FOR THE Z-SPRITE STORE ONLY (f30 and depth
 *      re-read `b->person`) is worth 6 here.  The original hoists that one
 *      `mov ecx,[esi+4]` twenty instructions, above the two `pos` stores; a
 *      store to the escaped `pos` is a schedule barrier for a pointer load,
 *      so the load cannot climb on its own -- naming it does it instead.
 *      Caching it for f30 as well is much worse (234): the original really
 *      does reload after the store through the pointer.
 * WHAT IS LEFT (19): indices 113-132 and the two case-8 pos2 stores.  The
 * y block is now register-for-register the original's; the one remaining
 * instruction is the SCROLL SUBTRACTION -- ours is `sub ebx,ecx` (sy minus
 * yscroll, scheduled at 113 because it is ready as soon as the movsx lands),
 * the original's is `sub edx,ecx` (oy minus yscroll, at 117, so it waits for
 * the `mov dx,[cfg+0x22]`).  That one displacement pushes the two
 * GetUnitDepth `push`es and the screen.ox load one slot each.
 * ROUND 6 (2026-09-04) -- THE SOURCE SHAPE IS NOW KNOWN, only its register
 * assignment is not.  Writing the y chain as ONE web
 *     sy2 = (wx + wy) * th >> 9;          (no separate `sy` at all)
 *     sy2 += g_map_cfg->oy - Get_YScroll();
 * reproduces indices 113-119 EXACTLY -- `mov ebp,[esp+0x40]` at 113, the
 * first GetUnitDepth push at 114, `sub edx,ecx` at 117 and `add r,edx` at
 * 119 -- and drops the register-blind distance (rb 18 -> 16, shp 18 -> 14).
 * It is not committed because it costs a THREE-CYCLE rotation of the
 * callee-saved registers over the whole block (ours ebx->orig ebp for sx,
 * ours edi->orig ebx for sy2, ours ebp->orig edi for wy/sx2), audit 19 -> 34.
 * Everything else about that variant is right, so the residual is now
 * exactly "give the merged sy2 web ebx".  Why the shape is certain:
 *   - `add edi,ebp` for X (destination = the ox-xscroll TEMP, sx folded) and
 *     `add ebx,edx` for Y (destination = the sy VARIABLE, delta folded) can
 *     only both be true if X is `sx2 = <delta> + sx` (fresh variable, so the
 *     rank-1 temp wins the destination copy) and Y is `sy2 += <delta>`
 *     (compound assignment on the shift's own web).  A corpus scan over all
 *     1542 exact bodies for `sub <scratch>,<scratch>` followed within three
 *     slots by `add <callee-saved>,<that scratch>` returns exactly TWO hits,
 *     both in PrintSpriteEx (0x4856a0), and both are `param += expr`.
 *     *** 2026-09-04, WITHDRAWN by the integrator: that scan was over-narrow
 *     (it required the `sub` within three slots and a specific operand
 *     class).  The loose form -- `add <callee-saved>,<scratch>` with no
 *     preceding-instruction or window condition -- finds 171 hits across the
 *     1544 exact bodies, so it does NOT show "the shape only ever comes from
 *     a compound assignment".  The compound-assignment conclusion still
 *     stands on the direct 2x2 isolation measured below; it simply has no
 *     corpus evidence behind it.  General rule now in docs/DECOMP.md: a
 *     conclusion resting on a scan finding few or no instances must be
 *     re-run with looser conditions before it is relied on. ***
 *   - the same substitution moves the Safari's two GetUnitDepth pushes onto
 *     indices 86/89 where the original has them, and halves its
 *     register-blind residual (rb 34 -> 18), with the identical 3-cycle.
 * MEASURED AND INERT against the rotation (~90 variants, all exactly 34):
 * every statement order in the block (2 shift orders x 2 world-read orders x
 * 3 spill positions x 2 chain orders x the empty `if`), all 24 orders of the
 * four subtraction statements, all 7 positions of the GetUnitDepth call,
 * `unsigned`/`short` types for sx/sy2/wy, splitting the shift into 2 or 3
 * statements, `if (v) {}` consumers on sy2/sx/sx2/wy, `sy2 *= 2` /
 * `sy2 += sy2` as an extra reference before the pos store, direct
 * `b->world.x/y` reads, `sy2 = sy2 + (...)` vs `+=` vs
 * `-= (Get_YScroll() - oy)`, and the x chain written as two statements.
 * Also inert (second pass): every position of `screen =
 * GetScreenCoordsForObject(..)` and of `GetTileDimensions(..)`, `unsigned`
 * and `long` for each of wx/wy/sx/sx2, `*(volatile int*)&spill` instead of
 * `&spill.x`, a named delta local for the X chain, and commuting either
 * product's operands (`th * (wx + wy)`, `(wy + wx)`).  ~130 variants in all.
 * Every attempt to keep TWO names (`sy2 = sy; sy2 += delta;`) is
 * copy-propagated back into the delta-wins form (106-113 and one byte short),
 * including with `if (sy) {}`, `sy2 -= 0`, `sy2++/--` and a block-scoped temp
 * between them.  Splitting the web at any OTHER point (a name for `wx + wy`,
 * for `(wx+wy)*th` before the `sar`, for the value after the delta add, or
 * for `sy2 * 2`) is byte-identical to the merged form, so it is not the
 * number of names that decides the register -- it is whether the yscroll
 * subtraction is itself an in-place op on the long web.
 * USEFUL MEASUREMENT for whoever takes this next: with the CURRENT y chain
 * and `p = b->person;` REMOVED the register-blind residual collapses to
 * rb=8 / shp=8 (strict 25), i.e. exactly TWO misplaced instructions in the
 * whole function -- the scroll `sub` (ours 113, original 117) and the
 * `mov ecx,[esi+4]` (ours 137, original 118).  The `p` cache fixes the
 * second and the merged y chain fixes the first, but no body has yet been
 * found that fixes both: merged + no `p` measures 37.  Ref counts are NOT the driver: the two-name form and the
 * one-name two-statement form have the same reference count on the web and
 * still get different registers, so what the allocator ranks here is the
 * number of NAMED webs, not their weight.  Ruled out on top of round 5: every
 * position of the z-sprite/f30 statements, all 24 orders of the four
 * subtractions, `unsigned`/`short` types for sx/sy/sx2/sy2 (`unsigned sy`
 * alone was worth 1 before round 5 and is subsumed), and re-testing every
 * round-4 hypothesis in the new configuration.
 * Case 8 (indices 213/214) is a separate 2: the original emits the two
 * `movsx` loads ASCENDING (+0x3c, +0x3e) AND stores pos2.x before pos2.y;
 * every spelling measured gives one or the other, never both -- the emitted
 * store order always follows source order and the two grouped loads always
 * come out in the OPPOSITE order.  Measured and rejected: temps in both
 * orders, direct stores, `int pv[2]`, `short` temps, `x+x` instead of `x*2`,
 * a `static __inline` seed helper with either argument order (only the
 * helper's STORE order matters), an empty `if` between the stores. */
/* ROUND 7 (2026-09-04, unchanged at 19).  ~330 further variants; the wall is
 * now characterised exactly, so do not re-derive it.
 * (a) DESTINATION RULE, refined and re-measured.  For a commutative add the
 *     destination is the expression TEMPORARY when the sum contains one; when
 *     BOTH operands are named locals it is the one defined LAST (nearest the
 *     add), NOT the earliest.  `sy2 = sy + dy` and `sy2 = dy + sy` both emit
 *     `add <dy>,<sy>`.  So the original's `add ebx,edx` (destination = the
 *     long y web) can only come from a compound `sy2 += <expr>` on that web,
 *     or from `<expression temporary> + <named local>`; a named delta cannot
 *     produce it.
 * (b) THE ROTATION IS DECIDED ENTIRELY INSIDE case 1.  Stub-each-case (every
 *     other case body replaced by `break;`, 8 variants) leaves the rotated
 *     triple in every one, so nothing outside this block can be blamed and
 *     nothing outside it can cure it.
 * (c) THE DISCRIMINATOR IS NOT THE COMPOUND.  Isolated on a 2x2 (strict
 *     counts; 19 = committed):
 *       2 webs, `sy2 = sy - ys; sy2 += oy;`  + empty-if   19   ebx/ebp
 *       2 webs, same, no empty-if                        237   ebx/ebp
 *       1 web,  `sy2 -= ys; sy2 += oy;`      + empty-if    38   edi/ebx  ROT
 *       1 web,  same, no empty-if                        237   ebx/ebp
 *       1 web,  `sy2 += oy - ys;` (the original's shape)   34   edi/ebx  ROT
 *     The one-web SUB/ADD form rotates too, so the trigger is "the y
 *     accumulator is ONE web AND the later subtractions are kept separate",
 *     not the compound.  Both no-empty-if rows fold the two later
 *     subtractions into a scratch web (that is what the empty `if` exists to
 *     stop), which is why they do not rotate.
 * (d) Inert this round -- ALL identical to the plain merged form: the six
 *     loop-head orders (next/b/tile) re-run under the merged y chain; tx/ty
 *     swapped and moved above FindRecord; world-read order; sx/sy compute
 *     order; y-chain before x-chain; all orders and interleavings of the four
 *     subtractions; `unsigned`/`unsigned int`/`long`/`short` on each of
 *     wx,wy,sx,sx2,sy2; three spill placements; a dummy extra use of wx or
 *     wy; GetTileDimensions first; four positions for
 *     GetScreenCoordsForObject; three declaration-order permutations; named
 *     `sox`/`soy` locals read early or late; the `p` cache removed; an extra
 *     int web; a late use of sx; compound-chain shifts (`sy2 = wx + wy;
 *     sy2 *= th; sy2 >>= 9;`) in three spellings; inlined world reads with no
 *     wx/wy locals; a zero-cost extra coalescing web at either chain's tail
 *     (`{int py = sy2 * 2; pos.y = py;}`, `sy3 = sy2 - screen.oy`,
 *     `sx3 = sx2 - pivot/2`, `sx2 *= 2`); and SEVEN alternative spill shims
 *     (`volatile SpillPair`, `volatile int[2]` at either index, a volatile
 *     STRUCT MEMBER, two volatile ints, the cast on `.y`) -- the shim's
 *     spelling is irrelevant to the rotation, only its existence is (dropping
 *     it costs the frame, 0x3c -> 0x38).
 * (e) Strictly worse and rejected: a copy `sy2 = sy;` before the compound
 *     (VC6 does NOT coalesce it -- sy2 lands in a SCRATCH register and the
 *     whole tail re-schedules); a destructive `sar`/`imul`/`add` web split
 *     (`sy = (wx+wy)*th; sy2 = sy >> 9;` keeps the triple but sinks the `sar`
 *     below both scroll calls and still puts sy2 in ecx); a named delta `dy`
 *     (keeps the triple, but VC6 then FLATTENS the two later subtractions
 *     into dy, and the empty-`if` that stops the flattening brings the
 *     rotation straight back); the one-statement form
 *     `sy2 = ((wx+wy)*th>>9) + (oy - Get_YScroll())` (frame 0x3c -> 0x40: the
 *     shift then lives across both scroll calls); a real dead store instead
 *     of the volatile one (`pos2.x`, `pos2.y`, `pos2.z`, `pos.x`, `pos.y`,
 *     plain `spill.x`) -- all are dead-store-eliminated, frame 0x38.
 * (f) The DECOMP rider-placement lever ("an AdjustBlokePosition block must be
 *     a `static __inline` helper that OWNS the escaped Offsets") was built
 *     and measured here: a `SBarrel_SeedPos(b, tile, def, &pos)` helper
 *     owning `screen` and `spill` keeps the 0x3c frame only when the `p`
 *     cache stays in the caller, and ROTATES with BOTH y shapes (the base
 *     sub/add form rotates inside the helper too).  It is not the cure here.
 * (f2) Delta-spelling closure: `sy2 = sy - (Get_YScroll() - oy)`,
 *     `sy2 = sy + (oy - Get_YScroll())`, `sy2 = sy; sy2 -= ys - oy;` and the
 *     `(int)`-cast form all canonicalise to the SAME code as the plain copy
 *     form -- VC6 folds the difference into one temporary and the accumulator
 *     lands in a scratch register (ecx), so the long web is never the
 *     destination.  Combined with (a) this closes the search: the only
 *     spelling that puts the sum in the long web is the in-place compound,
 *     and the in-place compound always rotates.
 * (f3) Function-level structure is inert too (all identical to merged):
 *     `return` vs `break` on the missing record, a lazily assigned `def`,
 *     `for (r = ..; r; r = next)`, `while (r != 0)`, `!b->state`, and moving
 *     SpinningBarrels_TickMachine() to the head.
 * (g) Case 8's 2 (indices 213/214) re-searched with nine spellings; the
 *     committed `int px = ..*2; int py = ..*2; pos2.y = py; pos2.x = px;` is
 *     still the floor.  Natural x-then-y stores cost the movsx order back. */
/* ROUND 8 (2026-09-04, unchanged at 19).  The wall was re-derived from
 * scratch on the Safari and the Spider as well as here, and TWO NEW FACTS
 * pin it down; do not re-run these searches.
 * (i) THE EMPTY `if` IS A PURE BLOCK SPLIT, NOT A SECOND CONSUMER.  Under the
 *     one-web chain, `if (sx2) { }` and `if (sy) { }` in the same position
 *     compile to BYTE-IDENTICAL objects (measured on the Safari: 153 either
 *     way, and the same for `if (sy != 0)`).  What stops the reassociation is
 *     the basic-block boundary, and its POSITION is what picks which of three
 *     attractors you land in -- after the compound gives the ebx/ebp naming,
 *     after `sx2 -= screen.ox` gives a THIRD naming (edi/ebx), later still is
 *     inert.  Under the one-web chain the empty `if` is unnecessary
 *     (`oneweb_noif` is byte-identical to `oneweb_if` on all three
 *     functions), which is itself an argument that the one-web chain is the
 *     original's source: it needs no shim.
 * (ii) A NAMED `ys` LOCAL RESTORES THE ORIGINAL'S CALLEE-SAVED NAMING.
 *     `ys = Get_YScroll(); sy += g_map_cfg->oy - ys;` (one web) puts wy back
 *     in edi, wx/sy in ebx and sx in ebp -- the Safari's indices 60-84 then
 *     match the original REGISTER FOR REGISTER -- but with no block split
 *     VC6 flattens the two later subtractions into the delta and the block
 *     costs 17 bytes (Safari 307, Spider 299).  Adding the block split back
 *     re-rotates.  So the two halves of the residual are now known to be the
 *     SAME switch: `separate later subtractions` XOR `original registers`.
 *     Nothing found sits on both sides.
 * Also measured this round and inert or worse (all on the one-web chain
 * unless noted): wx/wy read order; the spill on sy, on spill.y, moved after
 * the x chain, or doubled; `if` on sx2/both/before; y-chain before x-chain;
 * all four subtraction interleavings; `p` removed or moved; a coalescing
 * tail web for x or for y (`sy3 = sy - ofs/2`, `{int py = sy*2;}`); named
 * `oy`, named `xs`, a `tw` copy; screen.ox/oy in named locals; sum-before-
 * difference (329 -- confirms the DECOMP difference-before-sum lever, which
 * this file already had right); GetScreenCoordsForObject or the world reads
 * moved below GetTileDimensions.  `pos.y` stored before `pos.x` scores 28
 * here (bad 6 -> 2) but emits the two seed stores in the wrong order and is
 * a compensating error -- NOT committed.  All 60 legal orderings of the
 * seven tail statements (p / pos.x / pos.y / zsprite / f30 / depth / b35)
 * were re-run and the committed order is the floor (next best 23).
 * The `goto endsw` lever (DECOMP: an explicit goto at a switch's join flips
 * the identical-suffix merge) was applied to all five functions in this file:
 * inert on the Safari and the Plane, 19 -> 58 here, 15 -> 134 on the Spider
 * and 138 -> 145 on the Tower.  This file's shared tails are already hosted
 * where the original hosts them. */
/* ROUND 8c (2026-09-04): THE ONE-WEB Y CHAIN WAS RE-RANKED WITH
 * scratchpad/laneK/permrank.py (mismatches surviving the best permutation of
 * ebx/ebp/edi/esi) AND IS STILL NOT ADOPTED HERE.  Adapter:
 * scratchpad/mechJ/vJ.py (permrank's V.build over whole files) --
 *   PYTHONPATH=scratchpad/mechJ python scratchpad/laneK/permrank.py vJ \
 *       <Name> <0xVA> scratchpad/mechJ/cand/*.c
 * Result over all four BNV activations, `real` = permutation-aware count:
 *       function      cur(2-web+if)        one-web
 *       Barrels       19  perm=IDENTITY    17  perm=bpdibxsi
 *       Spider        15  perm=IDENTITY    25  perm=bpdibxsi
 *       Plane         19  perm=IDENTITY    29  perm=bpdibxsi
 *       Safari       132  perm=IDENTITY   131  perm=bpdibxsi
 * The committed two-web bodies score real == strict with the IDENTITY
 * permutation in all four, i.e. they have NO register difference from the
 * original anywhere in the function; the one-web bodies need the three-cycle
 * rename and STILL carry more real mismatches on three of the four.  The
 * one-web chain's extra, non-rotation cost is a load VC6 hoists into the slot
 * the in-place `sub` used to occupy: `mov ecx,[esp+0x3c]` (the screen.ox
 * reload) seven slots early on the Spider and the Plane, and the `p =
 * b->person` load on the Safari.  So on this file the one-web chain buys two
 * instructions on the Barrels, ties on the Safari and loses ten on the Spider
 * and the Plane -- it is NOT the uniform win it is on Carousel_Tick, and only
 * the two-web body reproduces the original's register allocation.
 * The coordinator's paired lever ("name only the FIRST b->person read") was
 * completed as a 2x2 on every function: adding the cache to the Spider's and
 * the Plane's case 0 is INERT (VC6 CSEs it, byte-identical either way, with
 * or without the one-web chain), and removing it from the Barrels or the
 * Safari is worse (real 17 -> 20 and 131 -> 234).
 * The empty `if` is NOT droppable in the two-web shape: `noif` measures real
 * 231 / 283 / 294 / 300 on Barrels / Spider / Plane / Safari.  It is a block
 * split, so what it buys is a reassociation barrier, and no non-`if`
 * construct has been found that provides one. */
/* ROUND 9 (2026-09-04, unchanged at 19).  The round-8c table was reproduced
 * from scratch with an independent strict+permutation ranker
 * (scratchpad/w7mech/w7.py) and every number matches exactly: Barrels
 * 19 -> 17, Spider 15 -> 25, Plane 19 -> 29, Safari 132 -> 131, one-web
 * perm=bpdibxsi in all four.  Three NEW results:
 *   - The one-web chain is now confirmed to be the ORIGINAL's shape by
 *     direct reading rather than inference: on all four rides the original
 *     emits `sub eax,edx` (the `oy - yscroll` DELTA in a scratch register)
 *     followed by `add <sy>,eax`, where the committed two-web body emits
 *     `sub <sy>,edx` early and `add <sy>,eax` later.  What the one-web body
 *     costs is ONE hoisted load filling the Pentium partial-register stall
 *     after `mov ax,[cfg+0x22]` (write ax, read eax) -- `mov ecx,[esp+0x3c]`,
 *     the screen.ox reload, on the Spider and the Plane.  The original leaves
 *     that stall unfilled.  Measured and unable to stop the hoist (all on the
 *     Spider under one-web, every cell exactly 25): an empty `if` BEFORE the
 *     compound as well as after it and in all four seams of the subtraction
 *     chain, both `if (sx2)` and `if (sy2)` spellings, and all six legal
 *     orderings of the four subtractions.  A volatile read of `screen.ox` at
 *     its use does stop it but costs the frame (294).
 *   - So the strict count and the truth genuinely disagree here.  The
 *     two-web body stays committed because one-web RAISES audit's number on
 *     three of the four rides (34/40/44 against 19/15/19).
 *   - The Safari is the exception worth recording: under one-web its
 *     REGISTER-BLIND residual more than halves (normalised edit distance
 *     15 -> 6) and its whole structural difference reduces to three sites.
 *     See the ROUND 9 paragraph there.
 * CORRECTION that applies to all four rides: the `p = b->person;` cache does
 * NOT work because "a store to an escaped local is a schedule barrier for a
 * pointer load".  It is not a barrier at all -- VC6 hoists `mov ecx,[esi+4]`
 * straight above the `pos.x`/`pos.y` stores, and all 140 interleavings of
 * {the four subtractions, `p`, `pos.x`, `pos.y`} that keep each axis in order
 * are BYTE-IDENTICAL.  What the cache actually turns on is COPY-PROPAGATION
 * DISTANCE: `p = b->person;` written immediately before its single use is
 * propagated away and measures exactly the same as no cache at all (Safari
 * 234 one-web / 227 two-web), while the same assignment with ANY statement
 * between it and the use survives and measures 131 / 132.  A
 * `static __inline void SeedBnvPos(BnvPos*, int, int)` for the two seed
 * stores is byte-identical to writing them out, in both y-chain shapes. */
/* PASS w8rides (2026-09-04).  NO CHANGE (19 strict, 19 real under the
 * IDENTITY permutation, register-blind 6).  Characterised rather than moved:
 * the whole residual is ONE slot-placement window, indices 113-132, and it is
 * the family signature.  The original leaves the register the `cdq` frees
 * idle and re-uses eax serially (`sar / sub edi,eax / mov eax,[esp+0x44] /
 * sub edi,eax / mov eax,[g_barrels_ofs2.oy]`); we fill that slot with the
 * screen.oy load and then have to place the two GetUnitDepth float-constant
 * pushes (0xc9c5848e, 0xc9c58012) two and one slots earlier than the original
 * does.  Same instructions, same registers, same byte length, three slots of
 * rotation.  SpiderRide_Activate's 87-97, SafariRide_Activate's 86-124 and
 * Carousel_Tick's 123-127 are the same window in the same shape, which is why
 * a fix on any one of them should be tried on all four at once.  Nothing new
 * was measured here this pass; the ROUND 6/9 grids already cover the source
 * spellings and the family evidence says the lever, if it exists, is not in
 * this arm's source. */
/* PASS w9rides (2026-09-04).  NO CHANGE (19 strict, 19 real under the
 * IDENTITY permutation, register-blind 6).  THE FIVE-FUNCTION WINDOW IS NOW
 * IDENTIFIED AS ONE INSTRUCTION SLOT, and this is the family note for it.
 *
 * Every one of the five bodies contains the same idiom TWICE, once per axis:
 *
 *      mov eax,[<ride rider offset, x or y>]
 *      cdq
 *      sub eax,edx
 *      <<<SLOT>>>                 <-- the entire family residual lives here
 *      sar eax,1                  ; the C `/ 2`
 *      sub <acc>,eax
 *      mov eax,[esp+<screen.ox|oy>]
 *      sub <acc>,eax
 *
 * and the whole of what the five "windows" disagree about is WHICH
 * instruction, if any, VC6 puts in <<<SLOT>>>.  Aligned over all five
 * (X = the sx2 halving, Y = the sy2 halving; `ok` = we already agree):
 *
 *  function          X ORIG  X OURS              Y ORIG                  Y OURS
 *  Carousel_Tick     empty   mov edx,[scr.ox]    mov edx,[zspr]          same  ok
 *  SpinningBarrels   empty   mov edx,[scr.ox]    mov edx,[zspr]          same  ok
 *  SafariRide        empty   mov edx,[scr.ox]    mov edx,[esi+4] (p)     mov edx,[zspr]
 *  SpiderRide        empty ok  empty ok          or [esi+62],80 ;
 *                                                mov edx,[esi+4]         empty
 *  PlaneRide         empty ok  empty ok          the same pair           empty
 *
 * TWO RULES DESCRIBE THE ORIGINAL AND WE BREAK BOTH, IN OPPOSITE
 * DIRECTIONS.  The original NEVER fills the X slot, and it ALWAYS fills the
 * Y slot with the first ready operation belonging to the statements that
 * FOLLOW the two `pos` stores.  We fill the X slot on the three rides that
 * cache `p = b->person` (Carousel, Barrels, Safari) and we never fill the Y
 * slot from below the pos stores (Spider, Plane).  Everything else in the
 * five windows is DOWNSTREAM of that one slot: the two GetUnitDepth
 * float-constant pushes and the `p` load merely take the next free slot
 * after it, which is why this function's pushes land at 126/132 where the
 * original has 114/120 and the Safari's `p` load lands at 91 where the
 * original has 103 -- same instructions, same registers either side, and in
 * this function the same byte length.  So the "five windows" are one
 * phenomenon with one degree of freedom, not five problems.
 *
 * THE BLOCK'S OWN SOURCE IS INVARIANT, which by the standing test means the
 * driver is not in it.  Measured on Carousel_Tick (the cheapest instrument,
 * 8 real) and BYTE-IDENTICAL in every cell: the x tail as one flat
 * three-term expression, the y tail likewise, both, named halves
 * (`int hx = g_carousel_dx / 2;`), named `screen.ox`/`.oy` locals, an
 * `Offset*` through which both are read, the four subtractions interleaved
 * x,y,x,y and y-first, a `zs = g_carousel_zspr;` cache placed before the
 * block / mid-block / before `p` (VC6 sinks the load every time, so local
 * register pressure cannot be raised from source), a `p2 = b->person`
 * second cache, and -- new -- respelling the two adjacent halving globals
 * as ONE object (`Offset g_carousel_ofs;` at 0x616078, and `int
 * g_carousel_d[2]`).  That last one also kills the tempting correlation
 * that the SafariRide's struct spelling (`g_safari_ofs2.ox`) was the cause:
 * struct and two-scalar spellings are byte-identical, the Spider gets the
 * slot right with scalars and the Safari gets it wrong with a struct.
 *
 * NEW AND USEFUL: THE REASSOCIATION CONFOUND.  Moving `b->flags |= 0x80;`
 * (or the z-sprite store, or `p = b->person`) ABOVE `pos.x = sx2 * 2;` makes
 * VC6 REASSOCIATE the four tail subtractions into `sx2 + (-screen.ox - h)`
 * -- `mov ecx,eax / sar ecx,1 / neg eax / sub eax,ecx / add ecx,eax` -- which
 * also takes the frame 0x3c -> 0x38 and costs 330+.  It is the `pos.x`
 * store IMMEDIATELY AFTER the subtractions that is the barrier, and an
 * empty `if` does NOT substitute for it: {`if (sx2){}`, `if (sy2){}`, both,
 * none} x {flag, flag+p, p+flag, p} before the pos stores are all exactly
 * the same object.  THIS CONFOUNDED EVERY EARLIER "lift the statement above
 * the pos stores" MEASUREMENT in these notes.  The clean way to run it is
 * the FOLDED STORE form, `pos.x = (sx2 - <h> / 2 - screen.ox) * 2;`, which
 * is BYTE-IDENTICAL to the committed statement form and is immune to the
 * reassociation.  Re-run on SpiderRide in that form: 27 cells of {flag,
 * flag+p, flag+zsprite, flag+zsprite+f30, zsprite, zsprite+flag} x {before,
 * between, after the two folded stores}, plus 20 cells of the flag/`p` pair
 * at all five seams of the four subtractions, plus per-axis interleaved
 * blocks.  15 is still the floor; the flag store either stays where the
 * source puts it (below) or floats to the TOP of the region (index 75,
 * where the original has 87), never to the slot.  So the conclusion the old
 * notes reached survives, but it now rests on a measurement that is not
 * confounded.
 *
 * CORRECTION TO THE ROUND-8 PARAGRAPH ABOVE: it is NOT the one-web y chain
 * that flattens the four later subtractions, it is the NAMED `ys` LOCAL.
 * Measured here: `sy += g_map_cfg->oy - Get_YScroll();` with ONE name (no
 * `sy2`, no `ys`, no empty `if`) holds the byte length EXACTLY (1148/1148)
 * and measures real 17 / strict 34 -- identical to every folded spelling of
 * it, and identical to keeping the four subtractions as statements.  The
 * `ys = Get_YScroll(); sy += g_map_cfg->oy - ys;` form measures 57/72 at
 * 1146 bytes, i.e. two bytes SHORT: that is the flattened object.  So the
 * empty `if` is needed only in the TWO-web spelling; in the one-name
 * spelling it is inert (byte-identical with and without, on this function
 * and on the Safari).  The one-name chain is still not committed for the
 * same reason as before -- it costs the three-cycle callee-saved rotation
 * and takes audit 19 -> 34.
 *
 * ALSO NEW: THE SLOT FOLLOWS THE VALUE, NOT THE CHAIN.  On Carousel_Tick,
 * storing `pos.y` before `pos.x` emits the Y chain first and the X chain
 * second -- and in BOTH orders it is `screen.ox` that gets hoisted into a
 * slot and `screen.oy` that stays serial in eax.  So the choice is a
 * property of that one load, not of which halving comes first.  (That
 * variant scores strict 26 against 29 but real 11 against 8 and reverses
 * the two seed stores: a compensating error, not committed.) */
/* PASS w10rides (2026-09-05).  NO CHANGE (19 strict, 19 real under the
 * IDENTITY permutation, register-blind 6) -- but THE FAMILY SLOT NOW HAS A
 * SINGLE NAMED CAUSE, established causally rather than by correlation, and
 * the "one instruction slot" framing of the w9 paragraph above is wrong.
 *
 * DELETE THE `p = b->person;` CACHE AND THE ENTIRE WINDOW COMES RIGHT.
 * With `b->person->zsprite = g_sbarrel_spr2;` spelled out (no cache) this
 * body measures strict 25 / real 24 but REGISTER-BLIND 3, and read against
 * the original its indices 120-137 are the original's 121-138 with NOTHING
 * else changed -- a pure one-slot shift whose only content is the missing
 * `mov ecx,[esi+4]`:
 *      orig 123 sar eax,1        <- X SLOT EMPTY
 *           125 mov eax,[esp+0x44]   screen.ox SERIAL IN EAX
 *           130 mov edx,[0x62fe04]   <- Y SLOT = the zsprite load
 *           114/120 the two GetUnitDepth pushes
 *      nop  122 sar eax,1        <- X SLOT EMPTY   (was `mov edx,[scr.ox]`)
 *           124 mov eax,[esp+0x44]   screen.ox SERIAL IN EAX
 *           129 mov edx,[zspr]       <- Y SLOT, same
 *           118/121 the two pushes   (committed body: 126/132)
 * So the X-slot fill, the twelve-slot sink of the two float-constant pushes
 * and the Y slot are ALL DOWNSTREAM OF ONE THING: which register the
 * `screen.ox` temp is allocated.  The original gives it EAX -- reusing the
 * register the halving chain has just freed -- so the load cannot move above
 * `sub edi,eax`, nothing is ready for the X slot, and the pushes float up
 * past it.  We give it EDX (free between the two `cdq`s), so it hoists into
 * the X slot and the pushes have to wait.  The `p` cache is what changes
 * that allocation: with `p` occupying ECX the screen temp takes EDX; with no
 * cache at all it takes EAX here (and ECX on Carousel_Tick, which is why
 * dropping the cache is a LOSS there -- 8 -> 17).
 *
 * THE CACHE IS STILL RIGHT AND STILL HAS TO STAY.  The original's
 * `mov ecx,[esi+4]` at index 118 is ABOVE the two `pos` stores, and a load
 * through `b` cannot climb past a store to the escaped `pos` on its own:
 * without the cache VC6 emits it at 137, immediately before its use.  So the
 * two halves of the original -- p hoisted to 118 AND screen.ox serial in eax
 * -- are not simultaneously reachable, and the committed body buys the first
 * (19) rather than the second (24/25).
 *
 * WHAT THE ORIGINAL IS DOING THAT WE CANNOT: IT LEAVES ECX IDLE.  On every
 * one of the five bodies the `g_map_cfg` pointer dies in ECX at the
 * `mov ?x,[ecx+0x22]` that reads `oy`, and from there to the `mov ecx,[esi+4]`
 * that reloads `b->person` for the f30 store the original NEVER USES ECX
 * AGAIN -- one whole scratch register left idle across the arithmetic.  Ours
 * always fills it (with `p`, or with a screen field).  Nothing in the source
 * raises or lowers local pressure enough to change that; see the invariance
 * results below.
 *
 * MEASURED THIS PASS (all on this function unless said otherwise, and all
 * BYTE-IDENTICAL to the committed body at 19/19/6):
 *   - all 10 orderings of the four subtractions that keep `sx2 -= half`
 *     before `sx2 -= screen.ox`, and the 5 that reverse it, crossed with
 *     {cache, no cache, cache after the pos stores} = 30 cells.  Every
 *     no-cache cell is exactly 24/25/3 and every cache cell exactly 19/19/6:
 *     the subtraction order is irrelevant, the cache is everything.
 *   - y-chain shape x cache: two-web (committed) / `sy2 = sy + (oy - scroll)`
 *     / `sy2 = sy; sy2 += oy - scroll;` / with and without the empty `if`,
 *     crossed with the three cache placements = 15 cells.  One-web is
 *     17/25/27 with no cache and 105-113 with it (the callee-saved rotation).
 *   - 15 further cache-placement and tail-folding spellings: folded stores,
 *     cache first / between the axes / between the two pos stores, named
 *     `ox`/`oy` locals, a `zs` cache, a three-term x tail, y-axis first, a
 *     `(Person3D*)` cast plus `if (p) { }`, `p->f30 = 1` promoted.  All 19.
 *   - empty-`if` block splits at all five seams of the four subtractions,
 *     with three different guard values, plus `do { } while (0)` = 16 cells.
 *     Seam 0 is byte-identical; seams 1-4 REASSOCIATE (a `neg eax` appears)
 *     and cost 227-242.  So the empty `if` is a reassociation barrier only
 *     where it already sits; it does not give the arithmetic its own basic
 *     block. */
// WIP-FUNCTION: LEGOLAND 0x0043c950  (95%, 19/362; frame, byte length and the whole y block exact -- see above)
void SpinningBarrels_Activate(RideElem* elem)
{
    RideDef*    def = elem->data;
    RideTile*   tile;
    int         tw;
    int         th;
    RiderNode*  next;
    Offset      screen;
    BnvPos      pos;
    BnvPos      pos2;
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
    Person3D*   p;

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
            case 1:
                screen = GetScreenCoordsForObject(tile, def);
                wy = b->world.y;
                wx = b->world.x;
                GetTileDimensions(&tw, &th);
                sx = (wx - wy) * tw >> 9;
                sy = (wx + wy) * th >> 9;
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
                /* The y chain is accumulated IN PLACE from sy (the scroll is
                 * subtracted from sy first, then the origin added back), which
                 * is what puts it in sy's own callee-saved register the way the
                 * original does -- with `sy2 = sy + (oy - yscroll)` the sum
                 * lands in the delta's scratch register instead and the whole
                 * wx/wy/sx/sy register triple rotates.  The empty `if` is the
                 * zero-instruction second consumer that stops VC6 flattening
                 * the two later subtractions back into this sum (it deletes the
                 * branch only after the reassociation decision). */
                sy2 = sy - Get_YScroll();
                sy2 += g_map_cfg->oy;
                if (sy2) { }
                sx2 -= g_sbarrel_pivot_x / 2;
                sx2 -= screen.ox;
                sy2 -= screen.oy;
                sy2 -= g_sbarrel_pivot_y / 2;
                /* `p` is cached ONLY for the z-sprite store: the original
                 * hoists that one `mov ecx,[esi+4]` above the two pos stores
                 * (a store to the escaped `pos` is a schedule barrier for a
                 * pointer load, so the load cannot move on its own), and
                 * re-reads b->person for f30 and depth. */
                p = b->person;
                pos.x = sx2 * 2;
                pos.y = sy2 * 2;
                p->zsprite = g_sbarrel_spr2;
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
            case 8:
                {
                    /* .y stored first: with the natural order VC6 sorts the
                     * two movsx loads by descending displacement (+0x3e
                     * first); storing y first restores the +0x3c/+0x3e load
                     * order the original has. */
                    int px = b->ride_dx * 2;
                    int py = b->ride_dy * 2;
                    pos2.y = py;
                    pos2.x = px;
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

/* STATE: all 376 instructions, both dispatch tables and the whole block
 * layout are reproduced; the semantics and the record layout are certain.
 * Same residual class as the other updates: the frame is short and the
 * cursor/ObjDef/record register split differs. */
/* THIS ROUND (audit 195 -> 63, byte length now exact at 1228). Five levers,
 * four of them transfers from the Barrels:
 *   1. A NAMED `tile` local (RIDE_TILE(r) once, used by FindRecord, by
 *      GetScreenCoordsForObject and by case 13) instead of the macro at each
 *      site: 212 -> 165 X on its own; case 13 reads it out of ebp as the
 *      original does.
 *   2. The rec spill the original does at its def is reached by making the
 *      SeatOf argument a VOLATILE READ of the variable --
 *      `SeatOf(r, *(SpiderRec* volatile*)&rec, ..)`.  That forces rec into
 *      memory, so VC6 stores it right after the null test (the original's
 *      `mov [esp+0x1c],edi` between `test` and `je`) and reloads it for the
 *      push; the reload also frees rec's register for wy, which is what lets
 *      the wx/wy pair live across GetTileDimensions.  A volatile store
 *      (`*(volatile T**)&rec = rec` / `= FindRecord(..)`) is INERT here --
 *      VC6 folds the self-assignment away; only the volatile READ works.
 *   3. `SpillPair spill; *(volatile int*)&spill.x = sx;` for the dead sx
 *      spill + phantom dword (see SpinningBarrels_Activate note items 4-5).
 *   4. `wy = b->world.y; wx = b->world.x;` BEFORE GetTileDimensions, and a
 *      fresh `sx2` for the x chain.
 *   5. `b->flags |= 0x80;` moved ABOVE the two sx2 subtractions (it is
 *      order-independent).  With the flag store between the x and y
 *      subtractions VC6 reassociates `sx2 -= a; sx2 -= b;` into
 *      `neg/sub/add` (three instructions, wrong registers, +3 length);
 *      above them it keeps the original's two separate `sub edi,eax`.
 *      Worth 321 -> 79 X on its own.  The frame is now 0x3c == 0x3c.
 * THIS ROUND (audit 63 -> 42).  Two transfers, both derived on the Barrels:
 *   6. The y chain is built FROM sy (`sy2 = sy - Get_YScroll(); sy2 +=
 *      g_map_cfg->oy;`) with the empty `if (sy2) { }` flatten breaker -- see
 *      SpinningBarrels_Activate items 9 and 10.  That fixed BOTH the add's
 *      destination register AND the wx/wy pair the old note blamed
 *      separately: `sy` in place merged sy's web with wx's and was what
 *      rotated ebp/edi/ebx; a fresh `sy2` that coalesces with sy does not.
 *      First divergence 44 -> 73 on this alone.
 *   7. `b->flags |= 0x80;` moved BELOW the two `pos` stores (63 -> 42).  It
 *      is order-independent, and that is where the original schedules the
 *      `or byte ptr [esi+0x62],0x80` -- between the two pivot divisions.
 *      NOTE this reverses round 5's finding that the flag store had to sit
 *      ABOVE the sx2 subtractions: that was only true while `sy` was in
 *      place, because it was the store that broke the `neg/sub/add`
 *      reassociation.  With the round-6 y chain the reassociation is gone.
 *   8. The VOLATILE READ of `rec` at the SeatOf call (round 5 item 2) is now
 *      HARMFUL and has been removed: 42 -> 36.  It was only ever a way of
 *      buying the rec spill while the y chain was wrong; with the round-6 y
 *      chain VC6 spills rec by itself and the volatile read costs a reload.
 *      (The same removal is worth 87 -> 35 on the Plane.)
 *   9. Case 13 (ALIGHT): the flag clear before the z-sprite store, and the
 *      four shift statements ordered world.y, target.y, world.x, target.x --
 *      the only one of the 48 measured head/shift orders that reproduces the
 *      original's interleave of the shifts with the CalcMoveLine argument
 *      pushes and the early `mov [esi+0x28],ebp`.  36 -> 17.
 * ROUND 6 (2026-09-04, audit 17 -> 15).  Case 7's pos2 pair: BOTH short
 * fields are read into int temps FIRST and the doubling happens at the
 * STORE --
 *     { int px = b->ride_dx; int py = b->ride_dy;
 *       pos2.x = px * 2; pos2.y = py * 2; }
 * -- which groups the two `movsx` ahead of the two `shl` exactly as the
 * original does.  `pos2.x = b->ride_dx * 2;` interleaves load/shift/store per
 * component (the old body); pre-doubled temps (`int px = b->ride_dx * 2;
 * pos2.x = px;`) tail-duplicate and ESCAPE the extent (224).  All 18
 * combinations of {declaration order} x {`* 2`, `+`, `<< 1`} for each
 * component measure the same 15, as do assignment-order and `unsigned`
 * variants; `int pv[2]` is 17 and storing y first is 17.
 * WHAT IS LEFT (15): (a) indices 73/75 and 87-97 -- the same two items the
 * Barrels has, the position of the scroll subtraction (ours `sub ebx,edx`
 * scheduled as soon as the movsx lands, the original's `sub eax,edx` on the
 * oy temp four slots later) and the `b->person` load the original hoists
 * above the pos stores; the Barrels' `p = b->person;` cache is INERT here
 * (17 either way) and harmful before the subtractions.  See the round-6
 * paragraph in SpinningBarrels_Activate: the y chain's SOURCE shape is now
 * known (one web, `sy2 = (wx + wy) * th >> 9; sy2 += oy - Get_YScroll();`)
 * and reproduces those indices, but costs the same three-cycle rotation of
 * the callee-saved registers here too (15 -> 42 when applied).
 * (b) indices 195/196/198: the two `movsx` are now grouped but come out
 * DESCENDING (+0x3e first) where the original is ascending, and the second
 * doubling is `add eax,eax` where the original has `shl eax,1`.  Floor for
 * this shape. */
/* ROUND 7 (2026-09-04, unchanged at 15).  Two searches re-run from scratch:
 *   - The y chain: see the ROUND 7 paragraph in SpinningBarrels_Activate.
 *     The one-web/compound form is confirmed as the original's shape and
 *     confirmed to cost the three-cycle callee-saved rotation here too
 *     (15 -> 42); ~330 variants ruled out.
 *   - Indices 87-97 (the `or byte [esi+0x62],0x80` and the `mov edx,[esi+4]`
 *     the original interleaves INTO the arithmetic): every arrangement that
 *     lifts the flag store above the two `pos` stores -- flag alone, flag +
 *     z-sprite, flag + z-sprite + f30, flag between the two pos stores, with
 *     and without a `p = b->person;` cache, and crossed with EIGHT spill-shim
 *     variants (`spill.y`, both members, a 12-byte `BnvPos`, `int[2]`,
 *     `int[3]`, a volatile READ, no spill) -- COSTS THE FRAME, 0x3c -> 0x38,
 *     and measures 359-380.  The p cache alone remains inert (15 either way).
 *     So the original's hoist is not reachable by moving the statement; like
 *     the Safari's early `p` load it is a scheduler phase, not source order. */
/* ROUND 8 (2026-09-04, unchanged at 15).  Case 7's remaining 3 (indices
 * 195/196/198) is now proved to be a FLOOR and not a spelling: the full grid
 * {`* 2`, `<< 1`, `x + x`} x {both temp declaration orders} x {both store
 * orders} = 36 variants was re-run, and every cell gives 15 with the movsx
 * pair DESCENDING (x-store first) or 17 with the pair ASCENDING (y-store
 * first).  The second doubling is `add eax, eax` in all 36 -- the original's
 * `shl eax, 1` is not reachable from any spelling, because ours loads
 * +0x3e into EAX first (eax is free at the jump-table target) where the
 * original loads +0x3c into ECX (its scratch rotation is one step further
 * on at that point).  The y chain was re-attacked with eight further two-web
 * spellings (`sy2 = sy;` then the compound, `sy2 = oy - ys; sy2 += sy;`,
 * `sy + oy - ys`, `sy - (ys - oy)`, a named delta, and the oy-first orders):
 * every one is 116 or 350.  See the ROUND 8 paragraph in
 * SpinningBarrels_Activate for the one-web/named-`ys` result, which restores
 * this function's callee-saved naming too and costs the flattening. */
/* ROUND 9 (2026-09-04, unchanged at 15).  Re-derived independently with a
 * combined strict + permutation ranker (scratchpad/w7mech/w7.py): the
 * committed body is real=15 strict=15 rb=6 under the IDENTITY permutation.
 *   - Cluster B (indices 87-97: the `or byte [esi+0x62],0x80` and the
 *     `mov edx,[esi+4]` that the original schedules INTO the second
 *     division's `sub`/`sar` latency gap) was re-swept as ALL 24 orderings of
 *     {flag store, zsprite store, `pos.x`, `pos.y`} crossed with {f30 after
 *     the zsprite store, f30 last} and with/without a `p = b->person` cache
 *     -- 48 cells.  15 is the floor, and every cell that lifts ANY statement
 *     above `pos.x = sx2 * 2;` costs the frame (0x3c -> 0x38, or 1232/1238
 *     bytes) and measures 132-356.  NOTE the `p` cache placed BEFORE the pos
 *     stores is 138 here, not inert; the earlier "inert" finding was for the
 *     cache placed after them, which VC6 copy-propagates away entirely (see
 *     the ROUND 9 paragraph in SpinningBarrels_Activate).
 *   - Cluster A (73/75) is the family one-web chain, and the original's
 *     shape really is `sub eax,edx` / `add ebx,eax`.  With the shift left in
 *     its committed position and only the accumulation made compound
 *     (`sy2 = (wx + wy) * th >> 9;` ... `sy2 += g_map_cfg->oy -
 *     Get_YScroll();`) the byte length stays exact at 1228 and rb stays 6,
 *     73/75 become exact -- but real goes 15 -> 25 because VC6 then hoists
 *     `mov ecx,[esp+0x3c]` (screen.ox) into the partial-register stall the
 *     in-place `sub` used to fill, shifting indices 75-89 by one.  About 40
 *     further cells (block splits before AND after the compound, both `if`
 *     values, all six subtraction orderings, a named `ys`, a volatile
 *     screen.ox read) are all 25 or worse. */
/* PASS w8rides (2026-09-04).  NO CHANGE (15 strict, 15 real under the
 * IDENTITY permutation, register-blind 6).  The residual is now split three
 * ways and one third of it is proved to be a cross-file twin:
 *   73/75    cluster A, the y chain (`sub eax,edx` / `add ebx,eax` against
 *            our `sub ebx,edx` / `add ebx,eax`) -- see the ROUND 10
 *            paragraph; unchanged.
 *   87-97    the original schedules `or byte [esi+0x62],0x80`,
 *            `mov edx,[esi+4]` and `mov eax,[g_spider_tab3]` INTO the two
 *            division latency gaps of the x/y chains; we emit them as a
 *            block after `mov [esp+0x50],ebx`.  Same instructions, same
 *            registers, three slots apart.
 *   195/196/198  case 7's `pos2` pair.
 * CASE 7 IS THE SAME BUG AS Carousel_Tick 0x0042c820 (ridecb3.c) AT ITS
 * INDICES 228/229/231 -- identical instructions, identical field offsets
 * (+0x3c then +0x3e), identical `shl edx,1` / `add eax,eax` split, identical
 * three-index residual, reached from two independently written sources.  Nine
 * more spellings measured HERE this pass, on top of the 30 measured there:
 *   `int px = f*2; int py = g*2;` with `pos2.y=py; pos2.x=px;`   224 ESCAPES
 *   the same with `pos2.x=px; pos2.y=py;`                        224 ESCAPES
 *   the same with a second pair of doubled temps                 224 ESCAPES
 *   `int px=f; int py=g; pos2.y=py*2; pos2.x=px*2;`               17
 *   plain `pos2.x = b->ride_dx*2; pos2.y = b->ride_dy*2;`         17
 *   the same with the stores swapped                              19
 *   `py` declared before `px` (committed body otherwise)           15 (tie)
 *   `px + px` / `py + py`                                         15 (tie)
 *   `px << 1` / `py << 1`                                         15 (tie)
 * (THE DIRECTION IN THIS SENTENCE IS WRONG and is corrected in the PASS
 * w9rides paragraph below: the ORIGINAL's pair is ASCENDING, +0x3c before
 * +0x3e, and it is OUR output that is descending because VC6 evaluates the
 * LAST store's operand first.  Nothing in the source reverses either
 * without also reversing the stores, so 195/196/198 is still
 * unreachable.)  With cluster A also settled, the honest floor for this body
 * is the 87-97 scheduling window plus these three. */
/* PASS w9rides (2026-09-04).  NO CHANGE (15 strict, 15 real under the
 * IDENTITY permutation, register-blind 6).  Two results.
 *
 *  (1) The 87-97 window is the family Y-SLOT: see the PASS w9rides paragraph
 *      in SpinningBarrels_Activate, which states the whole five-function
 *      phenomenon as one instruction slot and lists what is now ruled out.
 *      What is new HERE is that the old "lift the flag store above the pos
 *      stores" experiments were CONFOUNDED: doing that reassociates the four
 *      tail subtractions (`neg`/`sub`/`add`) and changes the frame, which is
 *      what those 330-380 scores actually measured.  Re-run cleanly in the
 *      folded-store form (`pos.x = (sx2 - g_spider_zframe / 2 - screen.ox)
 *      * 2;`, byte-identical to the committed statement form), 47 further
 *      cells: {flag, flag+p, p+flag, flag+zsprite, flag+zsprite+f30,
 *      zsprite, zsprite+flag} x {before / between / after the folded
 *      stores}, and the flag/`p` pair at all five seams of the four
 *      subtractions.  15 is the floor in every one.  The flag store either
 *      stays below (committed) or floats to index 75, twelve slots ABOVE
 *      where the original schedules it; nothing lands it on 87.
 *
 *  (2) CORRECTION -- CASE 7's 195/196/198 WAS READ BACKWARDS.  The original
 *      is ASCENDING, not descending:
 *          orig: movsx edx,[esi+0x3c] / movsx eax,[esi+0x3e] / shl edx,1 /
 *                shl eax,1
 *          ours: movsx eax,[esi+0x3e] / movsx edx,[esi+0x3c] / shl edx,1 /
 *                add eax,eax
 *      The FINAL registers agree (+0x3c in edx, +0x3e in eax) and the two
 *      stores agree; only the two loads are swapped, and our `add eax,eax`
 *      is a consequence of loading eax FIRST.  The mechanism the old note
 *      described is right -- VC6 evaluates the LAST store's operand first,
 *      so an x-then-y store order forces the +0x3e load first -- but the
 *      conclusion drawn from it ("descending is VC6's canonical sort") was a
 *      description of OUR output, not the original's, and the target was
 *      therefore stated wrongly.  The corrected target is {ascending loads,
 *      x-store first}, and it is not reachable: measured on Carousel_Tick,
 *      whose indices 228/229/231 are this function's twin, y-store-first
 *      DOES give ascending loads but swaps the registers and the stores (5
 *      mismatches), pre-doubled temps (`int px = f * 2;`) give BOTH `shl`
 *      and the right registers but reverse the two loads and cost 8
 *      elsewhere, and inert are: `2 * x` on either or both sites, a
 *      `volatile short` read of either field, a `short*` walk over the pair,
 *      `r->bloke->` on either field, a `static __inline` two-int seeder,
 *      comma operators, nested scopes, reversed temp declaration order and
 *      `short` temps (152).  So 195/196/198 stays unreachable, but for a
 *      different and now correctly stated reason. */
/* PASS w10rides (2026-09-05).  NO CHANGE (15 strict, 15 real under the
 * IDENTITY permutation, register-blind 6).  Two results, both negative but
 * both narrowing.
 *
 *  (1) THE Y SLOT IS THE `p` CACHE AND IT OVERSHOOTS HERE.  See the PASS
 *      w10rides paragraph in SpinningBarrels_Activate for the family
 *      statement.  Adding `p = b->person;` above the two `pos` stores (with
 *      `p->zsprite = g_spider_tab3;` below them) DOES lift the load above the
 *      stores -- but VC6 schedules it at index 75, thirteen slots above the
 *      original's 88, and the register it frees then drags `screen.oy` into
 *      EDX and into the Y slot at 88.  Measured: cache alone 138 (1231 B
 *      against 1228), cache + the flag store above the stores 347, cache with
 *      the flag store between the two pos stores 288, the folded-store form
 *      of the pair 292 (ESCAPES), the flag store alone 345.  So on this body
 *      the cache is worse than no cache -- the opposite of the Barrels and
 *      the Safari; the load simply does not stop where the original stops it.
 *      What the original wants at 87/88 is the flag store AND the p load in
 *      that order, and every spelling that puts either above the pos stores
 *      floats it to the top of the region instead.
 *
 *  (2) Nothing new on case 7.  The sixteen-cell grid recorded in
 *      Carousel_Tick (ridecb3.c) -- {declaration order} x {which temp carries
 *      the `* 2`} x {store order} -- settles the movsx pair for all three
 *      twins: in EVERY cell the two loads come out in the REVERSE of the two
 *      stores' order, and the original has x stored first AND +0x3c loaded
 *      first, which no cell reaches. */
// WIP-FUNCTION: LEGOLAND 0x00416330  (96%, 15/376; frame, byte length, the y chain, case 7's movsx pair and case 13 exact -- see above)
void SpiderRide_Activate(RideElem* elem)
{
    RideDef*    def = elem->data;
    int         tw;
    int         th;
    RiderNode*  next;
    RideTile*   tile;
    Offset      screen;
    BnvPos      pos;
    BnvPos      pos2;
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
            case 0:
                rec->joined++;
                rec->timer = 180;
                b->flags |= 8;
                screen = GetScreenCoordsForObject(tile, def);
                wy = b->world.y;
                wx = b->world.x;
                GetTileDimensions(&tw, &th);
                sx = (wx - wy) * tw >> 9;
                sy = (wx + wy) * th >> 9;
                *(volatile int*)&spill.x = sx;
                sx2 = g_map_cfg->ox - Get_XScroll() + sx;
                /* See SpinningBarrels_Activate: scroll subtracted from sy
                 * first, origin added back, empty `if` as the flatten breaker. */
                sy2 = sy - Get_YScroll();
                sy2 += g_map_cfg->oy;
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
            case 7:
                /* Both short fields are read into int temps FIRST: that is
                 * what groups the two `movsx` ahead of the two `shl` the way
                 * the original does (17 -> 15).  Writing the doubling into the
                 * loads (`pos2.x = b->ride_dx * 2;`) interleaves
                 * load/shift/store per component; storing the pre-doubled
                 * temps instead (`int px = b->ride_dx * 2; pos2.x = px;`)
                 * escapes the extent entirely. */
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

/* STATE: all 387 instructions, both dispatch tables and the whole block
 * layout are reproduced; the semantics and the record layout are certain.
 * Same residual class as the other updates (frame size and the
 * cursor/ObjDef/record register split). */
/* THIS ROUND (audit 205 -> 108, frame now 0x44 == 0x44). The Spider recipe
 * transfers wholesale -- named `tile` local, the volatile READ of `rec` at the
 * SeatOf call to force its spill, `SpillPair spill` for the dead sx store and
 * its phantom dword, wx/wy read before GetTileDimensions, a fresh `sx2`,
 * `b->flags |= 0x80` moved above the sx2 subtractions, and a `char` capacity
 * parameter -- plus one thing only the Plane has:
 *   case 7 fills a SEPARATE 8-byte Offset (frame+0x14), hands it to
 *   UnAdjustBlokePosition and copies it into `pos2` (frame+0x38); `pos` at
 *   frame+0x2c is case 0's seed only.  Our old body reused pos2 as the
 *   scratch and passed `pos` to NewBNVPath.  The frame proves the three
 *   objects: 0x14 (8) scratch, 0x1c (8) screen, 0x24 (8) spill pair,
 *   0x2c (12) pos, 0x38 (12) pos2 = 0x44.
 * NEXT ROUND (audit 108 -> 87), both transfers from the Barrels/Spider:
 *   - the y chain built FROM sy plus the empty-`if` flatten breaker (see
 *     SpinningBarrels_Activate items 9 and 10): 108 -> 95;
 *   - `b->flags |= 0x80;` moved BELOW the two `pos` stores: 95 -> 87.  As on
 *     the Spider this reverses the earlier "flag store above the sx2
 *     subtractions" rule, which only held while the y chain reassociated.
 * AND THEN (audit 87 -> 21, byte length now 1253 == 1253):
 *   - REMOVING the volatile read of `rec` at the SeatOf call was worth
 *     87 -> 35 on its own AND fixed the head's three-way register rotation
 *     (r=edi, tile=ebx, rec=ebp) and the 2-byte deficit.  The volatile read
 *     was only ever a crutch for the wrong y chain.  DIAGNOSTIC THAT FOUND
 *     IT: stub each `case` body out in turn and watch where the cursor load
 *     lands -- stubbing case 0 alone restored `mov edi,[eax+0xcc]`, which
 *     said the pressure was inside case 0, not in the loop head.
 *   - case 7: `p = b->person;` cached for the z-sprite store only, 35 -> 28.
 *   - case 13: the flag clear before the z-sprite store and world.x before
 *     world.y (best of the 48 measured head/shift orders), 28 -> 21.
 * ROUND 6 (2026-09-04, audit 21 -> 19).  Case 7's `ofs` pair takes the same
 * lever as SpiderRide_Activate case 7: both short fields into int temps
 * first, doubling at the store (`int px = b->ride_dx; int py = b->ride_dy;
 * ofs.ox = px * 2; ofs.oy = py * 2;`).  The two `movsx` then group ahead of
 * the two doublings.  All 18 spelling combinations measure 19.  Cost: the
 * second doubling comes out `lea edx,[eax+eax]` where the original has
 * `shl edx,1`, so the byte length goes 1253 == 1253 to 1254 vs 1253 -- the
 * one place in this file where a strict-count win costs the exact length,
 * recorded here so it is not mistaken for a regression.
 * WHAT IS LEFT (19): indices 72/74 and 86-96 are the two y-block items the
 * Barrels also has (the position of the scroll subtraction and the
 * `b->person` hoist -- see the round-6 paragraph in
 * SpinningBarrels_Activate, which identifies the source shape and the
 * three-cycle register rotation that blocks it; applying it here measures
 * 46); 198/199/201 is what remains of case 7's `ofs` pair (grouped now, but
 * descending, plus the `lea`); and 212-215 is the `b->flags |= 0x80` store,
 * which the original schedules AFTER both `ofs` frame loads.  The flag/`p`
 * sweep was RE-RUN in the new case-7 shape (5 flag positions x 3 `p`
 * positions plus the pre-UnAdjust position) and 19 is still the floor. */
/* ROUND 7 (2026-09-04, unchanged at 19).  The y chain is the family residual
 * (see the ROUND 7 paragraph in SpinningBarrels_Activate) and indices 86-96
 * are the Spider's flag/person-load window, ruled out there this round.
 * NEW here: indices 212-215 (case 7, after UnAdjustBlokePosition) were
 * searched exhaustively -- all twelve orderings of
 * {`b->flags |= 0x80`, `p = b->person`, `pos2.x = ofs.ox`, `pos2.y = ofs.oy`}
 * that keep x before y were measured and the COMMITTED order is the best of
 * them (19; the others 19-26).  The original emits the two `ofs` loads BEFORE
 * the flag store and the two `pos2` stores after it, which no source order
 * reproduces: VC6 will not hoist a load of an escaped local above an earlier
 * may-aliasing store, so this is the same scheduler-phase item as the
 * Spider's 87-97 window. */
/* ROUND 8 (2026-09-04, unchanged at 19).  THE ROUND-7 CLAIM ABOUT 212-215 IS
 * HALF WRONG and is corrected here: reading `ofs` into two int temps before
 * the flag store --
 *     { int ox = ofs.ox; int oy = ofs.oy;
 *       b->flags |= 0x80; p = b->person; pos2.x = ox; pos2.y = oy; }
 * -- DOES hoist both `[esp+0x34]` / `[esp+0x38]` loads above the
 * `or byte [esi+0x62],0x80`, exactly as the original has them (rb 378 -> 380,
 * bad 9 -> 7).  It is not committed because the eax/ecx/edx assignment then
 * sits one step behind the original's rotation and the two `pos2` stores come
 * out y-then-x, which costs 1-3 strict (all 8 combinations of {temp
 * declaration order} x {store order} x {flag/`p` order} measure 20-22).  So
 * this window is a rotation phase, not a missing hoist.
 * Case 7's 198/199/201 was re-run as the same 36-cell grid as
 * SpiderRide_Activate case 7 (identical outcome: 19 with the movsx pair
 * descending, 21 with it ascending, `lea edx,[eax+eax]` in every cell; the
 * plain `ofs.ox = b->ride_dx * 2;` form is 21 at the exact 1253 bytes).  The
 * one-web y chain measures 44 here and does NOT change case 7, which kills
 * the theory that the missing `oy - ys` scratch temp is what puts our
 * rotation a step behind inside a later case block. */
/* ROUND 9 (2026-09-04, unchanged at 19).  Measured with the combined ranker:
 * the committed body is real=19 strict=19 rb=8 under the IDENTITY
 * permutation, and the one-web y chain measures real=29 strict=44 rb=8 with
 * the three-cycle rename -- the Spider's result exactly, including the
 * screen.ox load VC6 hoists into the partial-register stall the in-place
 * `sub` had filled.  See the ROUND 9 paragraphs in SpiderRide_Activate and
 * SpinningBarrels_Activate; this body is the Spider's twin in both remaining
 * windows and everything ruled out there rules out here. */
/* PASS w8rides (2026-09-04).  NO CHANGE (19 strict, 19 real under the
 * IDENTITY permutation, register-blind 8, and still one byte long at
 * 1254/1253).  The residual splits into the same two family windows:
 * indices 86-96 (the x/y chain slot placement, as in SpiderRide 87-97) and
 * indices 212-219, where the original delays `or byte [esi+0x62],0x80` until
 * AFTER both `mov ecx,[esp+0x34]` / `mov edx,[esp+0x38]` reloads and we emit
 * it immediately after the call.  Both are slot placement with identical
 * instructions and identical registers either side; the ROUND 6/7 grids
 * already swept the source order of the flag store and the `pos2` pair, so
 * this arm's source is not where the lever is.  The extra byte is inside the
 * 212-219 window, not a systematic encoding difference. */
/* PASS w9rides (2026-09-04).  NO CHANGE (19 strict, 19 real under the
 * IDENTITY permutation, register-blind 8, still 1254/1253 bytes).  Indices
 * 86-96 are the family Y-SLOT and 212-219 is the same slot's shadow after
 * the UnAdjustBlokePosition call: see the PASS w9rides paragraph in
 * SpinningBarrels_Activate for the full statement and the negatives, and
 * the one in SpiderRide_Activate for the folded-store re-run that removes
 * the reassociation confound from every earlier "hoist the flag store"
 * measurement.  This body is the Spider's twin in both windows and
 * everything ruled out there rules out here.
 * CASE 7 (198/199/201) TAKES THE SAME CORRECTION as SpiderRide's 195/196/198
 * and it is worth spelling out here because the extra byte lives in it:
 *      orig: movsx ecx,[esi+0x3c] / movsx edx,[esi+0x3e] / shl ecx,1 /
 *            shl edx,1
 *      ours: movsx eax,[esi+0x3e] / movsx ecx,[esi+0x3c] / shl ecx,1 /
 *            lea edx,[eax+eax]
 * The original's pair is ASCENDING; ours is descending because VC6
 * evaluates the LAST store's operand first and `ofs.ox` is stored first.
 * The 3-byte `lea` (against the original's 2-byte `shl`) is the whole of the
 * 1254 vs 1253, so fixing the load order would fix the byte length too --
 * but the corrected target {ascending loads, x-store first} is unreachable;
 * see the twelve further spellings listed in SpiderRide_Activate. */
/* PASS w10rides (2026-09-05).  NO CHANGE (19 strict, 19 real under the
 * IDENTITY permutation, register-blind 8, still 1254/1253).  This body is
 * still the Spider's twin in both windows and inherits both of that
 * function's PASS w10rides results:
 *   - indices 86-96 are the family Y slot, and the family cause is now named
 *     (the `p = b->person;` cache; see the PASS w10rides paragraph in
 *     SpinningBarrels_Activate).  The Spider's measurement applies: adding
 *     the cache lifts the load above the two `pos` stores but overshoots to
 *     the top of the region and drags a screen field into EDX behind it.
 *   - indices 198/199/201 are the movsx twin, settled by the sixteen-cell
 *     grid in Carousel_Tick (ridecb3.c): the two loads are ALWAYS emitted in
 *     the reverse of the two stores' order, so {ascending loads, x-store
 *     first} is not reachable from any of the sixteen source shapes.  The
 *     one cell that produces BOTH `shl` (so the 3-byte `lea edx,[eax+eax]`
 *     and with it the 1254/1253 disappears) is `int rdy = <y> * 2;` with the
 *     x store first -- but it emits the two component pairs in the opposite
 *     order, i.e. exactly the original with its halves interchanged, and
 *     costs 45+ elsewhere on the Carousel.
 *   - indices 212-215 (the `or byte [esi+0x62],0x80` after
 *     UnAdjustBlokePosition) is a 2-slot displacement of the same flag store
 *     the Spider has at 87, unchanged. */
// WIP-FUNCTION: LEGOLAND 0x0043e410  (95%, 19/387; frame, head and y chain exact; byte length 1254 vs 1253 -- see above)
void PlaneRide_Activate(RideElem* elem)
{
    RideDef*    def = elem->data;
    int         tw;
    int         th;
    RiderNode*  next;
    RideTile*   tile;
    Offset      ofs;
    Offset      screen;
    BnvPos      pos;
    BnvPos      pos2;
    RiderNode*  r;
    PlaneRec*   rec;
    Bloke*      b;
    int         qx;
    int         qy;
    int         sx;
    int         sy;
    unsigned char dir;
    SpillPair   spill;
    Person3D*   p;
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
            case 0:
                rec->joined++;
                rec->timer = 180;
                b->flags |= 8;
                screen = GetScreenCoordsForObject(tile, def);
                wy = b->world.y;
                wx = b->world.x;
                GetTileDimensions(&tw, &th);
                sx = (wx - wy) * tw >> 9;
                sy = (wx + wy) * th >> 9;
                *(volatile int*)&spill.x = sx;
                sx2 = g_map_cfg->ox - Get_XScroll() + sx;
                /* See SpinningBarrels_Activate: scroll subtracted from sy
                 * first, origin added back, empty `if` as the flatten breaker. */
                sy2 = sy - Get_YScroll();
                sy2 += g_map_cfg->oy;
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
            case 7:
                /* Both short fields into int temps FIRST: that groups the two
                 * `movsx` ahead of the two doublings the way the original does
                 * (21 -> 19).  Same lever as SpiderRide_Activate case 7. */
                {
                    int px = b->ride_dx;
                    int py = b->ride_dy;
                    ofs.ox = px * 2;
                    ofs.oy = py * 2;
                }
                BlokeWalkAnim(b);
                BlokeSetFrame(b, 0);
                UnAdjustBlokePosition(&ofs);
                b->flags |= 0x80;
                /* cached only for the z-sprite store, as in case 0 */
                p = b->person;
                pos2.x = ofs.ox;
                pos2.y = ofs.oy;
                p->zsprite = g_plane_zsprite;
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
