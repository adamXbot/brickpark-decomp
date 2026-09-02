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
// WIP-FUNCTION: LEGOLAND 0x0043c5b0  (100% by audit.py; match.py cannot bound a tail-jmp function)
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
extern int        g_safari_ofs2_x;                           /* 0x0082c670 */
extern int        g_safari_ofs2_y;                           /* 0x0082c674 */
extern RenderList g_safari_list;                             /* 0x004cbecc */

/* RESIDUAL 4/166, and it is the SAME immovable canonicalisation joust.c's
 * TempleSlide_Draw carries (116/120 there): in the two four-term screen sums
 * the original adds the offset pair in the HIGHER frame slot (zofs, entry-0x10)
 * before the one in the lower (ofs, entry-0x18); VC6 SP3 here always sorts the
 * operands of a commutative sum of independent stack loads by ascending frame
 * displacement, so it emits the lower first. Measured inert this round: all
 * operand permutations of both sums, explicit parenthesisation, accumulation
 * through a temp (costs 13), both declaration orders of the two Offsets, and
 * function-level vs block-scope declaration (block scope is what took this
 * from 161 to 162 -- it fixes the person-pointer load position -- so the rest
 * of the frame and every other instruction is right). Treat this and
 * TempleSlide_Draw / simcore.c's IsAdjacentPos as one open question.
 *
 * ROUND 2 measured a further twenty spellings, all inert at 162/166: reading
 * either Offset through a pointer local (VC6 folds the pointer back to the
 * frame reference), explicit parenthesisation into two pairs, every operand
 * permutation of both sums, `- -x` in place of `+ x`, declaring the pair as
 * an int[2], as one Offset[2], with the declaration order reversed, and with
 * zofs moved into its own inner block. What round 2 DID establish is the rule
 * itself, and it is not "source order": mark the LOWER-slot operand
 * `*(volatile int*)&ofs.ox` and the emission order flips to the original's
 * (zofs first) -- so VC6 sorts the REORDERABLE stack reads of a commutative
 * sum by ASCENDING frame displacement and leaves a non-reorderable read last.
 * The volatile costs 12 instructions elsewhere (it also flips which operand
 * becomes the accumulator, `add edx,ecx` for `add ecx,edx`), so it is not the
 * answer, but it names the target: a spelling that makes exactly one of the
 * two reads non-reorderable without making it volatile. */
// WIP-FUNCTION: LEGOLAND 0x00414b80  (162/166; the two four-term sums add the higher frame slot first in the original -- VC6 canonicalises to lower-first)
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

                        zofs.ox = g_safari_ofs2_x;
                        zofs.oy = g_safari_ofs2_y;
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
// WIP-FUNCTION: LEGOLAND 0x0043bac0  (222/222 instructions and block layout; the frame is one 4-byte hole short and the cursor register is ebp not ebx)
void SpaceTower_Activate(RideElem* elem)
{
    RideDef*   def = elem->data;
    int        tilex;
    RiderNode* next;
    int        base[2];          /* only [1] is ever used (original) */
    RiderNode* r;
    TowerRec*  rec;
    Bloke*     b;
    int        tx;
    int        seat;
    unsigned char dir;

    SpaceTower_TickMachine();
    r = def->riders;
    while (r) {
        next = r->next;
        b = r->bloke;
        rec = SpaceTower_FindRecord(RIDE_TILE(r));
        if (rec == 0)
            break;
        tilex = RIDE_TILE(r)->b.x;
        tx = def->base_x + tilex;
        base[1] = def->base_y + RIDE_TILE(r)->b.y;
        if (!b->state) {
            switch (b->action) {
            case 0:
                rec->joined++;
                rec->timer = 200;
                SpaceTower_TakeSeat(r, RIDE_TILE(r));
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
            case 2:
            case 7:
                SpaceTower_Queue(r);
                break;
            case 3:
                seat = b->seat;
                b->target.x = (g_tower_seat[seat].dx + tilex) << 8;
                b->target.y = (g_tower_seat[seat].dy + RIDE_TILE(r)->b.y) << 8;
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
            case 8:
                b->target.x = ((def->qx + tilex) << 8) + 0x80;
                b->target.y = ((def->qy + RIDE_TILE(r)->b.y) << 8) + 0x80;
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
extern void  SpaceTower_DrawCar(TowerRec* rec, int car, int mode); /* 0x0043aee0 */

extern AnimRef g_bloke_anim_ref[];                           /* 0x004b775c */
extern int     g_anim_tower_a;                               /* 0x004b7750 */
extern int     g_anim_tower_b;                               /* 0x004b76b8 */

/* STATE: all 288 instructions and the whole block layout are reproduced and
 * the semantics are certain. Three codegen facts are not: (1) the original
 * keeps a DEAD `lea eax,[edi+0xc]` (the map-square address computed and then
 * discarded) in each rider test -- a merged-arm ghost no spelling of the
 * compare reproduces (pointer local, cast through the address, both compare
 * directions measured); (2) the two `off + screen` sums come out as `lea`
 * here and as two-def `add`s in the original (temporaries, compound
 * assignment and both operand orders all canonicalise to the same form); and
 * (3) the registers that carry the screen pair are swapped. Together they
 * shift the displacements, so the strict gate rejects it.
 *
 * ROUND 2 pinned down what that dead lea IS and what it is not. VC6 emits
 * exactly this pair -- `mov r16,[p+K]` immediately followed by `lea r32,[p+K]`
 * with the compare taken from the r16 -- when a NAMED pointer to that field is
 * live OUT of the loop; a two-function micro test reproduces it byte for byte,
 * and the lea then lands in a callee-saved register because the value must
 * survive. Here it lands in a SCRATCH register that the very next instruction
 * overwrites (eax at 0x0043afbd, killed by `mov eax,[edi+8]`; ecx at
 * 0x0043b013, killed by `mov ecx,[esp+0x30]`), so the pointer has NO use at
 * all: an earlier pass created it and a later one removed every consumer.
 * Ten spellings were measured, all inert (no lea at all): the key compared
 * through a pointer local, through the macro inline, in both compare
 * directions, hoisted into an unsigned short temp, the pointer declared at
 * function level, a static __inline predicate taking the pointer, another
 * taking two RideTiles BY VALUE, a whole-RideTile struct copy, byte-wise
 * b.x/b.y compares, and `(RideTile*)((char*)r + 0x0c)` pointer arithmetic.
 * A null test on the pointer (`&& k`) DOES produce the lea -- VC6 never folds
 * `&x != 0` -- but leaves a `test/je` pair behind that the original lacks, so
 * the construct being looked for is one whose consumer VC6 deletes late. */
// WIP-FUNCTION: LEGOLAND 0x0043af50  (288/288 instructions and block layout; a dead lea, the sum form and two register roles differ)
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

                if (t->key == sq->key
                    && !(r->bloke->flags & 0x80)
                    && g_bloke_anim_ref[r->bloke->anim].anim == &g_anim_tower_a)
                    IP_RenderBlokeIn3DNow(r->bloke);
                r = r->next;
            }
            SpaceTower_DrawCar(rec, 1, mode);
            SpaceTower_DrawCar(rec, 2, mode);
            PrintSprite(g_spacetower_spr1, off1.ox + screen.ox,
                        off1.oy + screen.oy, mode, 0);
            SpaceTower_DrawCar(rec, 0, mode);
            SpaceTower_DrawCar(rec, 3, mode);
            r = item->riders;
            while (r) {
                RideTile* t = RIDE_TILE(r);

                if (t->key == sq->key
                    && !(r->bloke->flags & 0x80)
                    && g_bloke_anim_ref[r->bloke->anim].anim == &g_anim_tower_b)
                    IP_RenderBlokeIn3DNow(r->bloke);
                r = r->next;
            }
            PrintSprite(g_spacetower_spr0, off1.ox + screen.ox,
                        off1.oy + screen.oy, mode, 0);
            LLSSetFrame(GetLLSForLayer(g_spacetower_layers, 5), rec->frame5);
            off2 = GetRenderOffsetForLayer(g_spacetower_layers, 5);
            AdjustOffsetForViewMode(&off2);
            PrintSprite(GetSpriteForLayer(g_spacetower_layers, 5),
                        off2.ox + screen.ox, off2.oy + screen.oy, mode, 0);
            LLSSetFrame(GetLLSForLayer(g_spacetower_layers, 3), rec->frame3);
            off2 = GetRenderOffsetForLayer(g_spacetower_layers, 3);
            AdjustOffsetForViewMode(&off2);
            PrintSprite(GetSpriteForLayer(g_spacetower_layers, 3),
                        off2.ox + screen.ox, off2.oy + screen.oy, mode, 0);
        } else {
            SpaceTower_DrawCar(rec, 1, mode);
            SpaceTower_DrawCar(rec, 2, mode);
            PrintSprite(g_spacetower_spr1, off1.ox + screen.ox,
                        off1.oy + screen.oy, mode, 0);
            SpaceTower_DrawCar(rec, 0, mode);
            SpaceTower_DrawCar(rec, 3, mode);
            LLSSetFrame(GetLLSForLayer(g_spacetower_layers, 5), rec->frame5);
            off2 = GetRenderOffsetForLayer(g_spacetower_layers, 5);
            AdjustOffsetForViewMode(&off2);
            PrintSprite(GetSpriteForLayer(g_spacetower_layers, 5),
                        off2.ox + screen.ox, off2.oy + screen.oy, mode, 0);
            LLSSetFrame(GetLLSForLayer(g_spacetower_layers, 3), rec->frame3);
            off2 = GetRenderOffsetForLayer(g_spacetower_layers, 3);
            AdjustOffsetForViewMode(&off2);
            PrintSprite(GetSpriteForLayer(g_spacetower_layers, 3),
                        off2.ox + screen.ox, off2.oy + screen.oy, mode, 0);
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

/* STATE: all 229 instructions, both jump tables and the whole block layout are
 * reproduced (including the two cases that share the bare `action++` block and
 * the uninitialised-path bug), and the semantics above are certain. What is not
 * reproduced is which values win the four callee-saved registers: the original
 * spills the rider cursor AND the ObjDef to their frame homes and reloads both
 * at the loop top (a rotated loop), keeping the two map coordinates in ebx/ebp;
 * ours keeps the cursor and the ObjDef in registers and spills the coordinates.
 * The same divergence appears in SpaceTower_Activate, so it is one question,
 * not two: what makes VC6 rank the computed coordinates above the pointers.
 * Measured inert: a `tile` pointer local vs the address spelled inline, the
 * do/while form, the for form, and hoisting the switch value into a local
 * (that one is kept -- it is worth 6). */
// WIP-FUNCTION: LEGOLAND 0x00404be0  (229/229 instructions and block layout; the loop cursor/ObjDef vs coordinate register split differs)
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

    Copters_TickMachine();
    r = def->riders;
    while (r) {
        next = r->next;
        b = r->bloke;
        rec = Copters_FindRecord(RIDE_TILE(r));
        if (rec == 0)
            break;
        tx = def->base_x + RIDE_TILE(r)->b.x;
        ty = def->base_y + RIDE_TILE(r)->b.y;
        if (b->state == 0) {
            unsigned char act = b->action;

            switch (act) {
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
                WalkPath_Advance(g_copters_paths[WalkPath_IndexOf(b)],
                                 tx, ty, b);
                break;
            case 3:
            case 7:
                b->action = (unsigned char)(act + 1);
                break;
            case 4:
                b->action = (unsigned char)(act + 1);
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
                WalkPath_Advance(g_copters_paths[WalkPath_IndexOf(b)],
                                 tx, ty, b);
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
// WIP-FUNCTION: LEGOLAND 0x0043da60  (275/275 instructions, frame and block layout; zero-register and loop-counter allocation differ)
void PlaneRide_Interact(RideElem* elem, int x, int y, RideTile* sq,
                        void* clip, int mode)
{
    RideDef*   item = elem->data;
    Offset     off;
    Offset     screen;
    Bloke*     found[4];
    RiderNode* r;
    PlaneRec*  rec;
    char       n;
    unsigned short key;

    found[0] = 0;
    found[1] = 0;
    n = 0;
    found[2] = 0;
    found[3] = 0;
    r = item->riders;
    rec = PlaneRide_FindRecord(sq);
    if (rec) {
        int i;

        screen = GetScreenCoordsForObject(sq, item);
        if (r) {
            key = sq->key;
            while (r) {
                if (key == r->ride_id)
                    found[n++] = r->bloke;
                r = r->next;
            }
            if (n) {
                for (i = 0; i < n; i++) {
                    if (found[i]->action == 13)
                        IP_RenderBlokeIn3DNow(found[i]);
                }
                for (i = 0; i < n; i++) {
                    if (found[i]->action == 14)
                        IP_RenderBlokeIn3DNow(found[i]);
                }
                LLSSetFrame(GetLLSForLayer(g_plane_layers, 1), rec->frame1);
                off = GetRenderOffsetForLayer(item->sprite, 1);
                AdjustOffsetForViewMode(&off);
                PrintSprite(GetSpriteForLayer(item->sprite, 1),
                            off.ox + screen.ox, off.oy + screen.oy, mode, 0);
                **g_plane_zspr_obj->pframe = (unsigned short)rec->frame1;
                for (r = item->riders; r; r = r->next) {
                    if (sq->key == r->ride_id
                        && (r->bloke->flags & 0x80)) {
                        Bloke*    b = r->bloke;
                        Person3D* p = b->person;
                        Offset    ofs;

                        ofs.ox = g_plane_rider_dx;
                        ofs.oy = g_plane_rider_dy;
                        p->local.ox = b->ride_dx;
                        p->local.oy = b->ride_dy;
                        AdjustBlokePosition(&p->local);
                        AdjustOffsetForViewMode(&ofs);
                        p->screen.ox = b->ride_dx + ofs.ox + screen.ox;
                        p->screen.oy = b->ride_dy + ofs.oy + screen.oy;
                        AdjustBlokePosition(&p->screen);
                        IP_RenderBlokeIn3DNow(r->bloke);
                    }
                }
                LLSSetFrame(GetLLSForLayer(g_plane_layers, 2), rec->frame2);
                off = GetRenderOffsetForLayer(item->sprite, 2);
                AdjustOffsetForViewMode(&off);
                PrintSprite(GetSpriteForLayer(item->sprite, 2),
                            off.ox + screen.ox, off.oy + screen.oy, mode, 0);
                return;
            }
        }
        LLSSetFrame(GetLLSForLayer(g_plane_layers, 1), rec->frame1);
        off = GetRenderOffsetForLayer(item->sprite, 1);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(item->sprite, 1),
                    off.ox + screen.ox, off.oy + screen.oy, mode, 0);
        LLSSetFrame(GetLLSForLayer(g_plane_layers, 2), rec->frame2);
        off = GetRenderOffsetForLayer(item->sprite, 2);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(item->sprite, 2),
                    off.ox + screen.ox, off.oy + screen.oy, mode, 0);
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

/* STATE: all 289 instructions and the whole block layout are reproduced and
 * the semantics are certain. The frame is 4 bytes too big: the original
 * colours the array-walk counter onto the same dead `elem` argument slot that
 * holds the rider count (a char and an int sharing one home), where ours gives
 * the counter its own slot. The 15-pointer array plus one separately zeroed
 * int (which is what the original's `mov dword ptr [..],0` + 15-dword
 * `rep stosd` says the source declares) is reproduced; a plain 16-entry array
 * emits a 16-dword stosd and costs 36 more. Do NOT hoist the map-square key
 * here: unlike the Plane, the Spider's original DOES spill it to a u16 stack
 * temp, which is what a direct `sq->key` comparison produces. */
// WIP-FUNCTION: LEGOLAND 0x00415ae0  (289/289 instructions and block layout; the walk counter needs one more frame slot than the original)
void SpiderRide_Interact(RideElem* elem, int x, int y, RideTile* sq,
                         void* clip, int mode)
{
    RideDef*    item = elem->data;
    Offset      a;
    Offset      swing;
    Offset      off;
    Offset      screen;
    Bloke*      found[15];
    int         zero_slot;       /* the original zeroes one more dword just
                                  * below the array (its `rep stosd` is 15
                                  * wide with a separate immediate store) */
    RiderNode*  r;
    SpiderRec*  rec;
    char        n;
    int         i;

    zero_slot = 0;
    memset(found, 0, sizeof(found));
    r = item->riders;
    n = 0;
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
                i = n;
                if (i > 0) {
                    Bloke** q = found;

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
                            off.ox + screen.ox, off.oy + screen.oy, mode, 0);
                off = GetRenderOffsetForLayer(g_spider_layers, 2);
                AdjustOffsetForViewMode(&off);
                PrintSprite(g_spider_zspr2, off.ox + screen.ox,
                            off.oy + screen.oy, mode, 0);
                **g_spider_zspr_obj->pframe = (unsigned short)rec->frame;
                for (r = item->riders; r; r = r->next) {
                    if (sq->key == r->ride_id && (r->bloke->flags & 0x80)) {
                        Bloke*    bl = r->bloke;
                        Person3D* p = bl->person;

                        swing.ox = 0;
                        swing.oy = 0;
                        a.ox = g_spider_zframe;
                        a.oy = g_spider_zstate;
                        if (bl->b35 == 1) {
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
                    off.ox + screen.ox, off.oy + screen.oy, mode, 0);
        off = GetRenderOffsetForLayer(g_spider_layers, 2);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(g_spider_layers, 2),
                    off.ox + screen.ox, off.oy + screen.oy, mode, 0);
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
extern void*  NewBNVPath(void* bin, int tag, const char* name,
                         float near_z, float far_z, Pos* pos);
extern void   BNVPath_SetDFrame(Bloke* b, void* path, int dframe); /* 0x004850b0 */
extern int    UpdateBlokeFromBNVPath(Bloke* b, void* path);  /* 0x00484cd0 */
extern int    BNVPath_GetDFrame(void* path);                 /* 0x00484ff0 */

typedef struct MapConfig {
    unsigned char  pad00[0x20];
    unsigned short ox;           /* +0x20 render origin */
    unsigned short oy;           /* +0x22 */
} MapConfig;

extern MapConfig g_map_cfg;                                  /* 0x004bcbf4 lpConfig */

extern void SafariRide_TickMachine(void);                    /* 0x00415200 */
extern int  SafariRide_SeatOf(RiderNode* r, RideTile* t);    /* 0x00415760 */
extern void SafariRide_SetFull(SafariRec* rec);              /* 0x00414ab0 */

extern char g_safari_pathname[];                             /* 0x004b4cac "manbox??" */
extern const int g_safari_end_on[8];                         /* 0x004b4cc4 */
extern const int g_safari_end_off[8];                        /* 0x004b4ce4 */

/* STATE: all 402 instructions, the sparse switch's byte case-index table and
 * 9-entry jump table, and the whole block layout are reproduced; the semantics
 * and the record layout above are certain. The frame is 0x28 where the
 * original's is 0x38 and, as in the other three updates, the original spills
 * the ObjDef and keeps the rider cursor in the dead argument slot while ours
 * does the reverse. The float pair is the same GetUnitDepth(near,far) pair
 * Temple Slide uses, one ulp apart: -1617787.0f / -1618006.0f. */
// WIP-FUNCTION: LEGOLAND 0x00415220  (402/402 instructions, both dispatch tables and block layout; frame 0x28 vs 0x38 and the same cursor/ObjDef register split)
void SafariRide_Activate(RideElem* elem)
{
    RideDef*    def = elem->data;
    RideTile*   tile;
    int         tw;
    int         th;
    RiderNode*  next;
    Offset      screen;
    Pos         pos;
    Pos         pos2;
    RiderNode*  r;
    SafariRec*  rec;
    Bloke*      b;
    int         qx;
    int         qy;
    int         sx;
    int         sy;
    unsigned char dir;

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
                GetTileDimensions(&tw, &th);
                sx = (b->world.x - b->world.y) * tw >> 9;
                sy = (b->world.x + b->world.y) * th >> 9;
                sx = g_map_cfg.ox - Get_XScroll() + sx;
                sy = sy + (g_map_cfg.oy - Get_YScroll());
                sx -= g_safari_ofs2_x / 2;
                sx -= screen.ox;
                sy -= g_safari_ofs2_y / 2;
                sy -= screen.oy;
                pos.x = sx * 2;
                pos.y = sy * 2;
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
                pos2.x = b->ride_dx * 2;
                pos2.y = b->ride_dy * 2;
                b->flags |= 0x80;
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
                b->person->zsprite = 0;
                b->flags &= (unsigned short)~0x80u;
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
extern int  SpinningBarrels_SeatOf(RiderNode* r, RideTile* t, int cap); /* 0x0043ce10 */
extern void SpinningBarrels_SetFull(SBarrelRec* rec);        /* 0x0043c320 */

extern char g_sbarrel_pathname[];                            /* 0x004b78b4 "BoxBloke??" */

/* STATE: all 362 instructions, both dispatch tables and the whole block
 * layout are reproduced (1149 bytes against 1148), including the tick-after-
 * the-walk order and the early return on a missing record; the semantics and
 * the record layout above are certain. The frame is 0x2c against the
 * original's 0x3c: the original ALSO spills the map-square pointer and the
 * record to frame homes and reloads them in the cases, where ours keeps both
 * in callee-saved registers. This is the closest of the four updates -- here
 * the rider cursor and ObjDef spills already agree. */
// WIP-FUNCTION: LEGOLAND 0x0043c950  (362/362 instructions and block layout; the original spills the square pointer and the record where ours keeps them in registers)
void SpinningBarrels_Activate(RideElem* elem)
{
    RideDef*    def = elem->data;
    RideTile*   tile;
    int         tw;
    int         th;
    RiderNode*  next;
    Offset      screen;
    Pos         pos;
    Pos         pos2;
    RiderNode*  r;
    SBarrelRec* rec;
    Bloke*      b;
    int         tx;
    int         ty;
    int         sx;
    int         sy;
    unsigned char dir;

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
                GetTileDimensions(&tw, &th);
                sx = (b->world.x - b->world.y) * tw >> 9;
                sy = (b->world.x + b->world.y) * th >> 9;
                sx = g_map_cfg.ox - Get_XScroll() + sx;
                sy = sy + (g_map_cfg.oy - Get_YScroll());
                sx -= g_sbarrel_pivot_x / 2;
                sx -= screen.ox;
                sy -= g_sbarrel_pivot_y / 2;
                sy -= screen.oy;
                pos.x = sx * 2;
                pos.y = sy * 2;
                b->person->zsprite = g_sbarrel_spr2;
                b->person->f30 = 1;
                b->person->depth = GetUnitDepth(-1617922.25f, -1618065.75f);
                b->b35 = 0;
                sprintf_w(&g_sbarrel_pathname[8], "%02d",
                          SpinningBarrels_SeatOf(r, tile,
                                                 *(unsigned char*)&g_sbarrel_def->capacity));
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
                pos2.x = b->ride_dx * 2;
                pos2.y = b->ride_dy * 2;
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
extern int  SpiderRide_SeatOf(RiderNode* r, RideTile* t, int cap); /* 0x00416830 */
extern void SpiderRide_SetFull(SpiderRec* rec);              /* 0x00415a60 */

extern char g_spider_pathname[];                             /* 0x004b4d94 "manbox??" */
extern const int g_spider_end_on[8];                         /* 0x004b4d9c */
extern const int g_spider_end_off[8];                        /* 0x004b4ddc */

/* STATE: all 376 instructions, both dispatch tables and the whole block
 * layout are reproduced; the semantics and the record layout are certain.
 * Same residual class as the other updates: the frame is short and the
 * cursor/ObjDef/record register split differs. */
// WIP-FUNCTION: LEGOLAND 0x00416330  (376/376 instructions and block layout; frame and register allocation differ)
void SpiderRide_Activate(RideElem* elem)
{
    RideDef*    def = elem->data;
    int         tw;
    int         th;
    RiderNode*  next;
    Offset      screen;
    Pos         pos;
    Pos         pos2;
    RiderNode*  r;
    SpiderRec*  rec;
    Bloke*      b;
    int         qx;
    int         qy;
    int         sx;
    int         sy;
    unsigned char dir;

    r = def->riders;
    SpiderRide_TickMachine();
    while (r) {
        next = r->next;
        b = r->bloke;
        rec = SpiderRide_FindRecord(RIDE_TILE(r));
        if (rec == 0)
            break;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                rec->joined++;
                rec->timer = 180;
                b->flags |= 8;
                screen = GetScreenCoordsForObject(RIDE_TILE(r), def);
                GetTileDimensions(&tw, &th);
                sx = (b->world.x - b->world.y) * tw >> 9;
                sy = (b->world.x + b->world.y) * th >> 9;
                sx = g_map_cfg.ox - Get_XScroll() + sx;
                sy = sy + (g_map_cfg.oy - Get_YScroll());
                sx -= g_spider_zframe / 2;
                sx -= screen.ox;
                b->flags |= 0x80;
                sy -= g_spider_zstate / 2;
                sy -= screen.oy;
                pos.x = sx * 2;
                pos.y = sy * 2;
                b->person->zsprite = g_spider_tab3;
                b->person->f30 = 1;
                b->person->depth = GetUnitDepth(-1617787.75f, -1618096.5f);
                b->b35 = 0;
                sprintf_w(&g_spider_pathname[6], "%02d",
                          SpiderRide_SeatOf(r, RIDE_TILE(r),
                                            *(unsigned char*)&g_spider_def->capacity));
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
                pos2.x = b->ride_dx * 2;
                pos2.y = b->ride_dy * 2;
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
                qx = def->qx + RIDE_TILE(r)->b.x;
                qy = def->qy + RIDE_TILE(r)->b.y;
                SPIDER_SEAT(rec, b->seat) = 0;
                b->person->zsprite = 0;
                b->flags &= (unsigned short)~0x80u;
                b->person->f30 = 0;
                UnAdjustBlokePosition(&b->person->screen);
                ScreenToMapRef(&b->person->screen, &b->world, 0);
                b->person->f34 = 0;
                b->target.y = qy << 8;
                b->world.y = b->world.y << 8;
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
extern int  PlaneRide_SeatOf(RiderNode* r, RideTile* t, int cap); /* 0x0043e050 */
extern void PlaneRide_SetFull(PlaneRec* rec);                /* 0x0043d990 */

extern char g_plane_pathname[];                              /* 0x004b79bc "manbox??" */
extern void* g_plane_tab1;                                   /* 0x0062fe88 */
extern void* g_plane_tab2;                                   /* 0x0062fe8c */

/* STATE: all 387 instructions, both dispatch tables and the whole block
 * layout are reproduced; the semantics and the record layout are certain.
 * Same residual class as the other updates (frame size and the
 * cursor/ObjDef/record register split). */
// WIP-FUNCTION: LEGOLAND 0x0043e410  (387/387 instructions and block layout; frame and register allocation differ)
void PlaneRide_Activate(RideElem* elem)
{
    RideDef*    def = elem->data;
    int         tw;
    int         th;
    RiderNode*  next;
    Offset      screen;
    Pos         pos;
    Pos         pos2;
    RiderNode*  r;
    PlaneRec*   rec;
    Bloke*      b;
    int         qx;
    int         qy;
    int         sx;
    int         sy;
    unsigned char dir;

    r = def->riders;
    while (r) {
        next = r->next;
        b = r->bloke;
        rec = PlaneRide_FindRecord(RIDE_TILE(r));
        if (rec == 0)
            return;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                rec->joined++;
                rec->timer = 180;
                b->flags |= 8;
                screen = GetScreenCoordsForObject(RIDE_TILE(r), def);
                GetTileDimensions(&tw, &th);
                sx = (b->world.x - b->world.y) * tw >> 9;
                sy = (b->world.x + b->world.y) * th >> 9;
                sx = g_map_cfg.ox - Get_XScroll() + sx;
                sy = sy + (g_map_cfg.oy - Get_YScroll());
                sx -= g_plane_rider_dx / 2;
                sx -= screen.ox;
                b->flags |= 0x80;
                sy -= g_plane_rider_dy / 2;
                sy -= screen.oy;
                pos.x = sx * 2;
                pos.y = sy * 2;
                b->person->zsprite = g_plane_zsprite;
                b->person->f30 = 1;
                b->person->depth = GetUnitDepth(-1617706.75f, -1617948.625f);
                b->b35 = 0;
                sprintf_w(&g_plane_pathname[6], "%02d",
                          PlaneRide_SeatOf(r, RIDE_TILE(r),
                                           *(unsigned char*)&g_plane_def->capacity));
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
                pos2.x = b->ride_dx * 2;
                pos2.y = b->ride_dy * 2;
                BlokeWalkAnim(b);
                BlokeSetFrame(b, 0);
                UnAdjustBlokePosition((Offset*)&pos2);
                b->flags |= 0x80;
                pos.x = pos2.x;
                pos.y = pos2.y;
                b->person->zsprite = g_plane_zsprite;
                b->person->f30 = 1;
                b->person->depth = GetUnitDepth(-1617706.75f, -1617948.625f);
                b->b35 = 2;
                sprintf_w(&g_plane_pathname[6], "%02d", b->seat);
                b->bnvpath = NewBNVPath(g_plane_tab2, 2, g_plane_pathname,
                                        -1617706.75f, -1617948.625f, &pos);
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
                qx = def->qx + RIDE_TILE(r)->b.x;
                qy = def->qy + RIDE_TILE(r)->b.y;
                PLANE_SEAT(rec, b->seat) = 0;
                b->person->zsprite = 0;
                b->flags &= (unsigned short)~0x80u;
                b->person->f30 = 0;
                UnAdjustBlokePosition(&b->person->screen);
                ScreenToMapRef(&b->person->screen, &b->world, 0);
                b->person->f34 = 0;
                b->world.y = b->world.y << 8;
                b->world.x = b->world.x << 8;
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

/* STATE: all 373 instructions and the whole block layout (both arms, the five
 * array passes, the shared final blit) are reproduced; the semantics are
 * certain. Residual is frame/register allocation of the same class as the
 * other two array-collecting draws. */
// WIP-FUNCTION: LEGOLAND 0x0043be70  (373/373 instructions and block layout; frame and register allocation differ)
void SpinningBarrels_Interact(RideElem* elem, int x, int y, RideTile* sq,
                              void* clip, int mode)
{
    RideDef*    item = elem->data;
    LayerOut    lay;
    Bloke*      found[15];
    Offset      off;
    Offset      screen;
    RiderNode*  r;
    SBarrelRec* rec;
    char        n;
    int         i;

    r = item->riders;
    n = 0;
    memset(found, 0, sizeof(found));
    rec = SpinningBarrels_FindRecord(sq);
    if (rec == 0)
        return;
    screen = GetScreenCoordsForObject(sq, item);
    GetLayer(item->sprite, &lay, 3);
    lay.dy = 0;
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
                        off.ox + screen.ox, off.oy + screen.oy, 0, 0);
            **g_sbarrel_zspr_obj->pframe = (unsigned short)rec->frame3;
            for (r = item->riders; r; r = r->next) {
                if (sq->key == r->ride_id && (r->bloke->flags & 0x80)) {
                    Bloke*    bl = r->bloke;
                    Person3D* p = bl->person;
                    Offset    piv;

                    piv.oy = g_sbarrel_pivot_y;
                    piv.ox = g_sbarrel_pivot_x;
                    p->local.ox = bl->ride_dx;
                    p->local.oy = bl->ride_dy;
                    AdjustBlokePosition(&p->local);
                    AdjustOffsetForViewMode(&piv);
                    p->screen.ox = bl->ride_dx + piv.ox + screen.ox;
                    p->screen.oy = bl->ride_dy + piv.oy + screen.oy;
                    AdjustBlokePosition(&p->screen);
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
            off = GetRenderOffsetForLayer(g_sbarrel_layers, 1);
            AdjustOffsetForViewMode(&off);
            PrintSprite(g_sbarrel_spr0, off.ox + screen.ox,
                        off.oy + screen.oy, 0, 0);
            LLSSetFrame(GetLLSForLayer(g_sbarrel_layers, 2), rec->frame2);
            off = GetRenderOffsetForLayer(g_sbarrel_layers, 2);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_sbarrel_layers, 2),
                        off.ox + screen.ox, off.oy + screen.oy, 0, 0);
            for (i = 0; i < n; i++) {
                if (found[i]->action == 16)
                    IP_RenderBlokeIn3DNow(found[i]);
            }
            for (i = 0; i < n; i++) {
                if (found[i]->action == 17)
                    IP_RenderBlokeIn3DNow(found[i]);
            }
            off = GetRenderOffsetForLayer(g_sbarrel_layers, 1);
            AdjustOffsetForViewMode(&off);
            PrintSprite(g_sbarrel_spr1, off.ox + screen.ox,
                        off.oy + screen.oy, 0, 0);
            return;
        }
    }
    LLSSetFrame(GetLLSForLayer(g_sbarrel_layers, 3), rec->frame3);
    off = GetRenderOffsetForLayer(g_sbarrel_layers, 3);
    AdjustOffsetForViewMode(&off);
    PrintSprite(GetSpriteForLayer(g_sbarrel_layers, 3),
                off.ox + screen.ox, off.oy + screen.oy, 0, 0);
    LLSSetFrame(GetLLSForLayer(g_sbarrel_layers, 2), rec->frame2);
    off = GetRenderOffsetForLayer(g_sbarrel_layers, 2);
    AdjustOffsetForViewMode(&off);
    PrintSprite(GetSpriteForLayer(g_sbarrel_layers, 2),
                off.ox + screen.ox, off.oy + screen.oy, 0, 0);
}
