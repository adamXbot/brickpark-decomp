/* LEGOLAND -- the CATAPULT ride: its whole callback set plus the four
 * record-list helpers the callbacks are built on.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere). The table that installs everything below is
 * Catapult_GetInterfaces (0x00403bb0, interfaces.c); the save/load pair is in
 * ridesave.c; the shared boarding machinery is rides.c; and the six
 * mechanical rides in mechrides.c are the closest analogues -- the catapult is
 * built from the same parts, one size smaller.
 *
 * ==========================================================================
 * NAMES: WHAT THE SLOTS ARE REALLY CALLED HERE
 * ==========================================================================
 * docs/RIDE_CALLBACKS.md / interfaces.c name the slots by their position;
 * mechrides.c names them by what they do. This file follows mechrides.c, so
 * three of the eight are RENAMED against interfaces.c's extern list:
 *     +0x8c  Catapult_Tick      -> Catapult_Select      (arm the build cursor)
 *     +0x98  Catapult_Add       -> Catapult_Place       (place one on the map)
 *     +0xa0  Catapult_Draw      -> Catapult_GetDrawDesc (draw-DESCRIPTOR)
 * and +0xb0 Catapult_Interact keeps its name and is the real DRAW, exactly as
 * in every other ride module. The addresses are what matter; the extern
 * declarations in interfaces.c still name the same five unchanged.
 *
 * ==========================================================================
 * THE CATAPULT RECORD (0x3c bytes, list head 0x004c1118)
 * ==========================================================================
 * One record per copy of the ride placed on the map, keyed by the packed
 * {x,y} square, in a singly linked list. The layout recovered here COMPLETES
 * the one ridesave.c derived from Catapult_Save/Load:
 *
 *   +0x00  u16   the packed map square (the list key)
 *   +0x04  next
 *   +0x08  int   the machine's state; 0 = idle, non-zero = armed/firing
 *                (0x00403430 sets it to 1 and clears +0x0d on arming)
 *   +0x0c  s8    the idle animation frame   (drawn through sprite layer 0)
 *   +0x0d  s8    the firing animation frame (drawn through sprite layer 1)
 *   +0x10  RiderNode*[4]  the four seats
 *   +0x20  u8[4]          one per seat: bit 0 = "draw this seat's arm"
 *   +0x24  int   (unused by the functions in this file)
 *   +0x28  int[4]         one per seat: set to 1 when that seat FIRES
 *   +0x38  int   (unused by the functions in this file)
 *
 * *** CORRECTION TO ridesave.c *** its `RideInstNode* inst[4]` at +0x10 is
 * typed as pointers into `def->instances`. They are not: Catapult_Save's walk
 * and Catapult_Fire's seat scan both use ObjDef+0xcc, which rides.c calls
 * `riders` -- the class-wide RIDER list. So the four saved indices are 1-based
 * positions in the RIDER list, and a catapult record points at four RiderNodes
 * (each of which carries its own bloke at +0x08). The save format is
 * unaffected; the meaning of the four slots is not what the type said.
 *
 * ==========================================================================
 * WHAT THE RIDE DOES
 * ==========================================================================
 * ACTIVATE (+0xa8) is the simulation step: tick every record's machine, then
 * walk the CLASS-WIDE rider list once and step each rider's four-state
 * machine (b->action 0,1,3,4 -- 2 is a HOLE in the switch, so its jump-table
 * entry points at the default). The four steps are: claim a seat (0),
 * fire (1), walk away from the landing spot (3, the CalcMoveLine/NewDirForAction
 * pair every ride shares), and leave the ride (4).
 * INTERACT (+0xb0) is the render: draw the machine itself through sprite
 * layer 1 when it is firing and layer 0 when it is not, then the four seat
 * arms through the layer numbers in the table at 0x004b40a4, then every
 * rider standing on THIS square in 3D.
 *
 * The queue/exit offsets a mechanical ride reads from ObjDef+0x24/+0x25 are
 * NOT used here: the catapult adds the record's square to the class's base
 * square at ObjDef+0x0c/+0x10 instead, so the landing spot is the object's
 * own square.
 * ==========================================================================
 */

void* memset(void*, int, unsigned int);

/* ---- the LLIDB element and the 0xd0-byte ObjDef -------------------------- */
typedef struct Spr {
    unsigned char pad00[0x10];
    unsigned int  flags;         /* +0x10  bit 0x2000 = draw via the +0xb0 slot */
} Spr;

typedef struct RiderNode RiderNode;

typedef struct RideDef {
    unsigned char pad00[0x0c];
    int           base_x;        /* +0x0c the class's base map square */
    int           base_y;        /* +0x10 */
    int           f14;           /* +0x14 draw-descriptor field 1 */
    int           f18;           /* +0x18 draw-descriptor field 2 */
    unsigned int  flags;         /* +0x1c 0x20 arms +0xa8, 0x400 arms +0xa0 */
    unsigned char pad20[0x3c - 0x20];
    int           footprint;     /* +0x3c the class footprint rect */
    unsigned char pad40[0x64 - 0x40];
    Spr*          sprite;        /* +0x64 build-anim sprite */
    unsigned char pad68[0xcc - 0x68];
    RiderNode*    riders;        /* +0xcc the class-wide rider list */
} RideDef;

typedef struct RideElem {
    char*        name;           /* +0x00 */
    char*        image;          /* +0x04 */
    unsigned int type_flags;     /* +0x08 */
    RideDef*     data;           /* +0x0c */
} RideElem;

typedef struct RideDrawDesc {
    Spr*           sprite;       /* +0x00 */
    int            f04;          /* +0x04 */
    int            f08;          /* +0x08 */
    unsigned short f0c;          /* +0x0c */
} RideDrawDesc;

typedef struct Pos {
    int x;
    int y;
} Pos;

typedef struct Offset {
    int ox;
    int oy;
} Offset;

/* The map square a placement is keyed by: two bytes every list walk compares
 * as one 16-bit value. */
typedef union RideTile {
    unsigned short key;
    struct { unsigned char x, y; } b;
} RideTile;

/* The same square as the remove handler is handed it: BY VALUE, one dword. */
typedef struct CellPos {
    unsigned char x;
    unsigned char y;
} CellPos;

typedef struct RideMapObj {
    unsigned char pad00[0x0c];
    RideDef*      cls;           /* +0x0c */
} RideMapObj;

typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;        /* +0x0e low-level AI state (0 = idle) */
    unsigned char  pad10[0x24 - 0x10];
    Pos            target;       /* +0x24 walk target, 24.8 */
    unsigned char  pad2c[0x3a - 0x2c];
    short          f3a;          /* +0x3a throws left before the rider leaves */
    unsigned char  pad3c[0x58 - 0x3c];
    int            f58;          /* +0x58 frames until the next throw */
    unsigned char  pad5c[4];
    unsigned char  action;       /* +0x60 this ride's state-machine step */
    unsigned char  pad61;
    unsigned short flags;        /* +0x62 8 = on this ride */
    unsigned char  pad64[4];
    Pos            world;        /* +0x68 world position, 24.8 */
    unsigned char  pad70[2];
    unsigned char  b72;          /* +0x72 the frame the ride holds it on */
    unsigned char  new_dir;      /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14];   /* +0x98 CalcMoveLine scratch */
} Bloke;

struct RiderNode {
    RiderNode*     next;         /* +0x00 */
    RiderNode*     prev;         /* +0x04 */
    Bloke*         bloke;        /* +0x08 */
    unsigned short ride_id;      /* +0x0c the packed map square it is using */
    unsigned short pad0e;
    void*          owner;        /* +0x10 */
};

#define RIDE_TILE(r) ((RideTile*)&(r)->ride_id)

/* ---- the catapult's own record ------------------------------------------ */
typedef struct CatapultRec {
    RideTile           tile;     /* +0x00 */
    unsigned short     pad02;
    struct CatapultRec* next;    /* +0x04 */
    int                state;    /* +0x08 non-zero while the arm is swinging */
    signed char        frame0;   /* +0x0c idle frame   (sprite layer 0) */
    signed char        frame1;   /* +0x0d firing frame (sprite layer 1) */
    unsigned short     pad0e;
    RiderNode*         seat[4];  /* +0x10 */
    unsigned char      shown[4]; /* +0x20 bit 0 = draw this seat's arm */
    signed char        frame[4]; /* +0x24 that seat's throw-animation frame */
    int                fire[4];  /* +0x28 seat i has pulled the lever */
    int                f38;      /* +0x38 */
} CatapultRec;

/* ---- shared engine entry points ----------------------------------------- */
extern void* HeapAlloc_w(unsigned int size);                    /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                               /* 0x0049e4d0 */

extern void  DefaultCursor(void* cursor);                       /* 0x0045a390 */
extern void  SetEditCursorFootPrint(void* src);                 /* 0x0045f440 */
#ifndef LEGOLAND_PORTABLE
extern void  AddBasicObject(void* obj, Pos* pos);               /* 0x0045efe0 */
#else
extern void AddBasicObject(void* ll_obj, void* ll_pos, void* ll_ctx);               /* 0x0045efe0 */
#define AddBasicObject(_a1, _a2) AddBasicObject((_a1), (_a2), 0)
#endif
extern void  StandardRemoveObject(void* obj, CellPos tile, void* ctx);
                                                                /* 0x0045f220 */
extern void  RemoveAllBlokesFromRide(RideDef* cls, CellPos tile);
                                                                /* 0x0048a2e0 */
extern void  RemoveBlokeFromRide(RideDef* item, RiderNode* r);  /* 0x0048a100 */

extern void  HideLayer(void* sprite, int layer);                /* 0x00497de0 */
extern void  Load_FXList(void* list, int count);                /* 0x00496dd0 */
extern void  Kill_FXList(void* list, int count);                /* 0x00496e30 */

extern void*  GetLLSForLayer(void* sprite, int layer);          /* 0x00441ea0 */
extern void*  GetSpriteForLayer(void* sprite, int layer);       /* 0x00441ec0 */
extern void   LLSSetFrame(void* lls, int frame);                /* 0x0047d5a0 */
extern Offset GetRenderOffsetForLayer(void* sprite, int layer); /* 0x00441ee0 */
extern Offset GetScreenCoordsForObject(RideTile* sq, RideDef* item);
                                                                /* 0x00442cc0 */
extern void   AdjustOffsetForViewMode(Offset* o);               /* 0x00442d30 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx);
                                                                /* 0x004853a0 */
extern void   IP_RenderBlokeIn3DNow(Bloke* b);                  /* 0x00440010 */

extern int    CalcMoveLine(Pos from, Pos to, void* path);       /* 0x00480740 */
extern int    NewDirForAction(Bloke* b, unsigned char dir);     /* 0x004833d0 */

/* ---- the catapult's own globals and its two remaining internals ---------- */
extern int      g_edit_changed;                                 /* 0x008119b0 */
extern RideDef* g_edit_object;                                  /* 0x008119b8 */
extern char     g_edit_cursor;                                  /* 0x007febc0 */

extern void*        g_catapult_layers;                          /* 0x004c10f0 */
extern RideDef*     g_catapult_def;                             /* 0x004c10f4 */
extern RideDrawDesc g_catapult_draw;                            /* 0x004c1100 */
extern CatapultRec* g_catapult_head;                            /* 0x004c1118 */
extern void*        g_catapult_fx;                              /* 0x004b40c8 (4 entries) */
extern int          g_catapult_seat_layer[4];                   /* 0x004b40a4 */

/* All three are defined at the bottom of this file. */
void Catapult_TickAllRecords(void);
void Catapult_TakeSeat(RideTile* tile, RiderNode* r, int x, int y);
void Catapult_Fire(RideTile* tile, RiderNode* r);

/* ==========================================================================
 * THE RECORD LIST -- add to head, find by square, unlink-and-free, drain.
 * Identical in shape to joust.c's four (see the long notes there); both of
 * the awkward ones need the same no-op volatile levers, for the same two VC6
 * optimisations.
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x004030f0
void Catapult_AddRecord(RideTile* tile)
{
    CatapultRec* rec = (CatapultRec*)HeapAlloc_w(sizeof(CatapultRec));

    if (rec != 0) {
        memset(rec, 0, sizeof(CatapultRec));
        rec->tile.key = tile->key;
        rec->next = g_catapult_head;
        g_catapult_head = rec;
    }
}

/* Unlink one record and free it. The head case is tail-duplicated; the walk
 * dereferences the head with no null test (removing from an empty list would
 * fault -- reproduced, the callers only pass a record they just found), and
 * the `if (node)` after the loop is the original's redundant re-test.
 * The ONE volatile read on the link deref, and declaring `link` BEFORE `node`
 * seeded from the GLOBAL, are what reproduce the original's redundant second
 * load and its eax/ecx role assignment -- see joust.c 0x00407a50. */
// FUNCTION: LEGOLAND 0x00403130
void Catapult_RemoveRecord(CatapultRec* rec)
{
    if (g_catapult_head == rec) {
        g_catapult_head = rec->next;
    } else {
        CatapultRec** link = &g_catapult_head->next;
        CatapultRec*  node = g_catapult_head;
        while (*link != rec) {
            node = *(CatapultRec* volatile*)link;
            if (node == 0)
                break;
            link = &node->next;
        }
        if (node)
            node->next = rec->next;
    }
    HeapFree_w(rec);
}

// FUNCTION: LEGOLAND 0x00403190
void Catapult_FreeAllRecords(void)
{
    while (g_catapult_head)
        Catapult_RemoveRecord(g_catapult_head);
}

/* Same pair of no-op volatile levers as Joust_FindRecord (0x00407a20): the
 * self-assignment pins `mov ecx,[esp+4]` where the original has it, and the
 * two volatile key reads keep the RECORD key in dx with the TILE key as the
 * compare's memory operand instead of hoisting it out of the loop. */
// FUNCTION: LEGOLAND 0x004031b0
CatapultRec* Catapult_FindRecord(RideTile volatile* tile)
{
    CatapultRec* rec = g_catapult_head;

    if (rec != 0) {
        tile = *(RideTile volatile* volatile*)&tile;
        while (((CatapultRec volatile*)rec)->tile.key != tile->key) {
            rec = rec->next;
            if (rec == 0)
                return 0;
        }
        return rec;
    }
    return 0;
}

/* ==========================================================================
 * +0xa4 / +0xac -- CREATE and DESTROY
 * Create arms both flag bits (0x20 = per-frame update, 0x400 = draw
 * descriptor) and the sprite's custom-draw bit, caches the class sprite, then
 * HIDES layers 0 and 1 -- the machine's two animation layers, which the ride
 * blits itself in +0xb0 -- and loads its four-entry FX list. Note the two
 * nested null guards: unlike the six mechanical rides, the catapult tolerates
 * both a missing ObjDef and a missing sprite (and then hides layers on
 * whatever the stale global still holds -- reproduced).
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x004031e0
void Catapult_Create(RideElem* elem)
{
    g_catapult_def = elem->data;
    if (g_catapult_def) {
        g_catapult_def->flags |= 0x420;
        if (g_catapult_def->sprite) {
            g_catapult_def->sprite->flags |= 0x2000;
            g_catapult_layers = g_catapult_def->sprite;
        }
    }
    HideLayer(g_catapult_layers, 0);
    HideLayer(g_catapult_layers, 1);
    Load_FXList(&g_catapult_fx, 4);
}

// FUNCTION: LEGOLAND 0x00403250
void Catapult_Destroy(void)
{
    Catapult_FreeAllRecords();
    Kill_FXList(&g_catapult_fx, 4);
}

/* ==========================================================================
 * +0xb0 -- THE DRAW
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00403270
void Catapult_Interact(RideElem* elem, int x, int y, RideTile* sq,
                       void* clip, int mode)
{
    RideDef*     item = elem->data;
    RiderNode*   r = item->riders;
    CatapultRec* rec = Catapult_FindRecord(sq);
    Offset       screen;
    Offset       off;
    unsigned char* shown;
    int*         layer;
    int          i;

    if (rec) {
        screen = GetScreenCoordsForObject(sq, item);
        if (rec->state & 1) {
            LLSSetFrame(GetLLSForLayer(g_catapult_layers, 1), rec->frame1);
            off = GetRenderOffsetForLayer(g_catapult_layers, 1);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_catapult_layers, 1),
                        screen.ox + off.ox, screen.oy + off.oy, mode, 0);
        } else {
            LLSSetFrame(GetLLSForLayer(g_catapult_layers, 0), rec->frame0);
            off = GetRenderOffsetForLayer(g_catapult_layers, 0);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_catapult_layers, 0),
                        screen.ox + off.ox, screen.oy + off.oy, mode, 0);
        }
        layer = g_catapult_seat_layer;
        shown = rec->shown;
        i = 4;
        do {
            if (*shown & 1) {
                off = GetRenderOffsetForLayer(g_catapult_layers, *layer);
                AdjustOffsetForViewMode(&off);
                PrintSprite(GetSpriteForLayer(g_catapult_layers, *layer),
                            screen.ox + off.ox, screen.oy + off.oy, mode, 0);
            }
            shown++;
            layer++;
        } while (--i);
        while (r) {
            if (sq->key == r->ride_id)
                IP_RenderBlokeIn3DNow(r->bloke);
            r = r->next;
        }
    }
}

/* ==========================================================================
 * +0xa8 -- THE SIMULATION STEP
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00403820
void Catapult_Activate(RideElem* elem)
{
    RideDef*   item = elem->data;
    RiderNode* r;
    RiderNode* next;
    RideTile*  tile;
    Bloke*     b;
    int        wx;
    int        wy;
    unsigned char dir;

    Catapult_TickAllRecords();
    r = item->riders;
    while (r) {
        next = r->next;
        tile = RIDE_TILE(r);
        b = r->bloke;
        wx = tile->b.x + item->base_x;
        wy = tile->b.y + item->base_y;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags |= 8;
                Catapult_TakeSeat(tile, r, wx, wy);
                break;
            case 1:
                Catapult_Fire(tile, r);
                b->action++;
                break;
            case 3:
                b->target.x = (wx << 8) + 0x80;
                b->target.y = (wy << 8) + 0x80;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 4:
                RemoveBlokeFromRide(item, r);
                b->flags &= (unsigned short)~8u;
                break;
            }
        }
        r = next;
    }
}

/* ==========================================================================
 * +0x8c / +0x98 / +0x9c / +0xa0 -- the four small slots
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00403930
void Catapult_Select(void)
{
    g_edit_changed = 1;
    g_edit_object = g_catapult_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

// FUNCTION: LEGOLAND 0x00403970
void Catapult_Place(void* obj, Pos* pos)
{
    RideTile tile;

    tile.b.x = (unsigned char)pos->x;
    tile.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    Catapult_AddRecord(&tile);
}

/* The catapult is the only ride of its family that looks its record up AFTER
 * the generic remove and the eviction, not before. */
// FUNCTION: LEGOLAND 0x004039a0
void Catapult_Remove(void* obj, CellPos tile, void* ctx)
{
    CatapultRec* rec;

    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(((RideMapObj*)obj)->cls, tile);
    rec = Catapult_FindRecord((RideTile*)&tile);
    if (rec)
        Catapult_RemoveRecord(rec);
}

// FUNCTION: LEGOLAND 0x004039e0
RideDrawDesc* Catapult_GetDrawDesc(RideElem* elem, unsigned short tile)
{
    RideDef* def = elem->data;

    g_catapult_draw.sprite = def->sprite;
    g_catapult_draw.f04 = def->f14;
    g_catapult_draw.f08 = def->f18;
    g_catapult_draw.f0c = tile;
    def->sprite->flags |= 0x2000;
    return &g_catapult_draw;
}

/* ==========================================================================
 * THE MACHINE ITSELF -- what one copy of the catapult does per frame.
 *
 * A record's machine has two independent animations and four seats:
 *   +0x0c frame0  the IDLE arm, stepped every tick and wrapped at 16;
 *   +0x0d frame1  the FIRING arm, stepped only while state&1 and wrapped at
 *                 32, at which point state&~1 puts the machine back to idle;
 *   +0x20 shown[i]/+0x24 frame[i]  the same pair per seat -- the seat's own
 *                 32-frame throw animation, blitted through the layer number
 *                 in g_catapult_seat_layer[i];
 *   +0x28 fire[i] set by Catapult_Fire when the rider in seat i pulls the
 *                 lever, which is what lets TickRecord run that seat.
 *
 * The FX list at 0x004b40c8 is FOUR 12-byte entries whose third dword is the
 * resolved sample, so 0x004b40d0 is entry 0's sample and the three "thrown"
 * yells are entries 0..2 picked at random; entry 3 (0x004b40f4) is the
 * machine's own launch thud.
 * ========================================================================== */

typedef struct SoundSource {
    int   kind;                  /* +0x00  2 = a map square */
    void* obj;                   /* +0x04 */
    int   x;                     /* +0x08 */
    int   y;                     /* +0x0c */
} SoundSource;

extern int   rand(void);                                        /* 0x0049e4b2 (CRT) */
extern int   PlayInstanceOfSample(void* sample, int a, int b,
                                  SoundSource* src);            /* 0x00496d20 */

/* The resolved-sample field of each 12-byte FX entry: [k*3] is entry k's.
 * Same spelling as castleobj.c's g_ds_samples. */
extern void* g_catapult_sample[];                               /* 0x004b40d0 */
/* One int per seat: the y offset (24.8) of that seat's landing spot. */
extern int   g_catapult_seat_yofs[4];                           /* 0x004b40b4 */

// FUNCTION: LEGOLAND 0x00403430
void Catapult_Launch(CatapultRec* rec)
{
    SoundSource src;

    if (rec->state == 0) {
        src.kind = 2;
        src.x = rec->tile.b.x;
        src.y = rec->tile.b.y;
        rec->state = 1;
        rec->frame1 = 0;
        PlayInstanceOfSample(g_catapult_sample[9], 0, 1, &src);
    }
}

/* Step the firing arm. The GetSpriteForLayer call's result is DISCARDED --
 * the original asks for layer 1's sprite and throws it away (reproduced). */
// FUNCTION: LEGOLAND 0x00403480
void Catapult_StepArmAnim(CatapultRec* rec)
{
    if (rec->state & 1) {
        GetSpriteForLayer(g_catapult_layers, 1);
        rec->frame1++;
        if (rec->frame1 >= 0x20)
            rec->state &= ~1;
    }
}

// FUNCTION: LEGOLAND 0x004034c0
void Catapult_StartSeatFlight(CatapultRec* rec, int i)
{
    SoundSource src;
    int         k;

    if (rec->shown[i] == 0) {
        rec->shown[i] = 1;
        rec->frame[i] = 0;
        src.kind = 2;
        src.x = rec->tile.b.x;
        src.y = rec->tile.b.y;
        k = rand() % 3;
        PlayInstanceOfSample(g_catapult_sample[k * 3], 0, 1, &src);
    }
}

// FUNCTION: LEGOLAND 0x00403530
void Catapult_StepSeatAnim(CatapultRec* rec, int i)
{
    if (rec->shown[i] & 1) {
        rec->frame[i]++;
        if (rec->frame[i] >= 0x20)
            rec->shown[i] = 0;
        LLSSetFrame(GetLLSForLayer(g_catapult_layers, g_catapult_seat_layer[i]),
                    rec->frame[i]);
    }
}

// FUNCTION: LEGOLAND 0x00403580
void Catapult_ClearSeat(CatapultRec* rec, int i)
{
    rec->fire[i] = 0;
    rec->seat[i] = 0;
}

// FUNCTION: LEGOLAND 0x004035a0
void Catapult_TickRecord(CatapultRec* rec)
{
    RiderNode*  r;
    int         i;

    rec->frame0++;
    if (rec->frame0 >= 0x10)
        rec->frame0 = 0;
    for (i = 0; i < 4; i++) {
        /* The seat array is named TWICE on purpose -- once in the guard and
         * once for the local. With one reference VC6 bases the loop's induction
         * pointer on the HIGHER array (rec->fire, +0x28) and reads the seat at
         * [ebx-0x18]; with two it bases it on the seat (+0x10) and reads the
         * fire flag at [ebx+0x18], which is what the original does. The second
         * load is CSE'd away, so this costs nothing. */
        if (rec->seat[i] && (rec->fire[i] & 1)) {
            r = rec->seat[i];
            r->bloke->b72 = 7;
            r->bloke->f58--;
            if (r->bloke->f58 <= 0) {
                r->bloke->f58 = rand() % 20 + 50;
                r->bloke->f3a--;
                if (rand() % 100 <= 45) {
                    Catapult_StartSeatFlight(rec, i);
                    if (rec->state == 0)
                        Catapult_Launch(rec);
                }
                r->bloke->f3a--;
                if (r->bloke->f3a <= 0) {
                    r->bloke->action++;
                    Catapult_ClearSeat(rec, i);
                }
            }
        }
    }
    Catapult_StepArmAnim(rec);
    for (i = 0; i < 4; i++)
        Catapult_StepSeatAnim(rec, i);
}

// FUNCTION: LEGOLAND 0x00403690
void Catapult_TickAllRecords(void)
{
    CatapultRec* p = g_catapult_head;

    while (p) {
        Catapult_TickRecord(p);
        p = p->next;
    }
}

/* Pick a free seat: start at a random one and walk forwards, wrapping. If
 * every seat is taken this spins forever -- the caller only calls it for a
 * rider the ride has already accepted. */
// FUNCTION: LEGOLAND 0x004036b0
char Catapult_PickFreeSeat(CatapultRec* rec)
{
    char i = (char)(rand() % 4);

    while (rec->seat[i]) {
        i++;
        if (i >= 4)
            i = 0;
    }
    return i;
}

// FUNCTION: LEGOLAND 0x004036f0
void Catapult_TakeSeat(RideTile* tile, RiderNode* r, int x, int y)
{
    Bloke*       b = r->bloke;
    CatapultRec* rec = Catapult_FindRecord(tile);

    if (rec) {
        int           seat = Catapult_PickFreeSeat(rec);
        unsigned char dir;

        rec->seat[seat] = r;
        b->f3a = 3;
        b->target.x = (x << 8) - (rand() % 2 ? 0x10 : -0x10) - 0xe0;
        b->target.y = (y << 8) + (rand() % 2 ? 0x20 : -0x20)
                    + g_catapult_seat_yofs[seat];
        dir = (unsigned char)CalcMoveLine(b->world, b->target, b->path) + 0x10;
        b->state = 7;
        b->new_dir = dir;
        NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
        b->action++;
    }
}

// FUNCTION: LEGOLAND 0x004037d0
void Catapult_Fire(RideTile* tile, RiderNode* r)
{
    CatapultRec* rec = Catapult_FindRecord(tile);
    int          i;

    if (rec) {
        for (i = 0; i < 4; i++) {
            if (rec->seat[i] == r)
                rec->fire[i] = 1;
        }
        r->bloke->f58 = (rand() & 0x1f) + 3;
        r->bloke->f3a = 3;
    }
}

/* ==========================================================================
 * ONE STRAY WESTERN-TOWN CALLBACK
 * 0x004393e0 is LEGO SHOP 1's +0xac (destroy). It BELONGS IN westtown.c --
 * that file already has the rest of the class (LegoShop1_LoadResources at
 * 0x00439200, _Add, _Remove, _SelectForPlacement, _DrawOverlay) and both of
 * the globals below -- but it was the one entry of interfaces.c's callee list
 * that no lane in this round owned, so it is matched here. MOVE IT to
 * westtown.c next to LegoShop1_LoadResources; nothing else in this file uses
 * it. It is the mirror of that loader: drop the matte sprite (guarded, the
 * class tolerates never having loaded it) and release the shared till sound.
 *
 * It ends in a tail 'jmp KillMoneySFX' with no ret of its own, so it keeps a
 * WIP marker: audit.py reports it exact, but the shared tools/match.py cannot
 * bound a tail-jmp function and would score it red.
 * ========================================================================== */

extern void  KillSprite(void* sprite);                          /* 0x00497bd0 */
extern void  KillMoneySFX(void);                                /* 0x00453930 */
extern void* g_legoshop1_matte;   /* 0x0081cb18  "Lego Shop 1 Matte.LLS" */

// FUNCTION: LEGOLAND 0x004393e0
void LegoShop1_Destroy(void)
{
    if (g_legoshop1_matte)
        KillSprite(g_legoshop1_matte);
    KillMoneySFX();
}
