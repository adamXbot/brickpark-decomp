/* LEGOLAND -- ten more of the unnamed class callbacks from screen.c's
 * SetCustomCallbacks table (the fifth file; screencb.c .. screencb4.c hold
 * the first forty-one).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field offsets, record sizes, global addresses and callee arg counts are
 * load-bearing; the names are ours.  Types are defined LOCALLY on purpose
 * (legoland.h is owned elsewhere).
 *
 * ---------------------------------------------------------------------------
 * WHAT THESE ARE, AND HOW THEY WERE NAMED
 * ---------------------------------------------------------------------------
 * screen.c's SetCustomCallbacks (0x00452c20) runs once per class element as
 * the object database is loaded, matches the element's class NAME and stores
 * a run of function pointers into the class's 0xd0-byte ObjDef.  The slots
 * (docs/RIDE_CALLBACKS.md's numbering) are:
 *
 *     +0x8c tick/select   +0x90 update      +0x94 draw-selection
 *     +0x98 add           +0x9c remove      +0xa0 draw-descriptor
 *     +0xa4 create        +0xa8 activate    +0xac destroy
 *     +0xb0 interact      +0xb8 load        +0xbc save        +0xc0 extra
 *
 * Every name below is the class whose slot holds the address, read straight
 * out of SetCustomCallbacks -- none is a guess:
 *
 *   0x00413fa0  DRIVING SCHOOL ROADS  +0x94  Road_DrawSelection  (SHARED:
 *               ZEBRA CROSSING's +0x94 is the same address)
 *   0x0042ba80  BALLOONZ                   +0xbc  Balloonz_Save
 *   0x0042c4a0  CAROUSEL                   +0x9c  Carousel_Remove
 *   0x0042c590  CAROUSEL                   +0xbc  Carousel_Save
 *   0x0042de50  ENTRANCE 1                 +0xa4  Entrance1_Create
 *   0x0042def0  ENTRANCE 1                 +0xac  Entrance1_Destroy
 *   0x0042efb0  RESTAURANT 1               +0x9c  Restaurant1_Remove
 *   0x004322a0  RESTAURANT 1               +0xbc  Restaurant1_Save
 *   0x00432390  RESTAURANT 2               +0xbc  Restaurant2_Save
 *   0x00433d20  JUNGLE CRUISE MONKEY TREE  +0x98  MonkeyTree_Add
 *
 * ---------------------------------------------------------------------------
 * THE SHAPE GROUPS  (disassemble the family first, then write one body)
 * ---------------------------------------------------------------------------
 * Nine of the ten are 38 instructions, which is NOT evidence of twinning --
 * the diff is.  Diffing all ten found three groups and three singletons:
 *
 *  A. +0xbc SAVE x4 -- ONE source compiled four times.  Balloonz_Save,
 *     Carousel_Save, Restaurant1_Save and Restaurant2_Save are identical
 *     index for index once the list-head global and the record SIZE are
 *     substituted (0x20/0x2c/0x0c/0x40).  It is screencb2.c's
 *     EarthSlide_Save with the per-record queue pass deleted, so the whole
 *     group came off that body first try.
 *
 *  B. +0x9c REMOVE x2 -- ONE source compiled twice.  Carousel_Remove and
 *     Restaurant1_Remove differ only in the two per-class helpers they call
 *     (Carousel_FindRec/_FreeRec against Restaurant1_FindRec/_FreeRec);
 *     every register, every offset and every stack slot is the same.
 *
 *  C. ENTRANCE 1's +0xa4 / +0xac pair -- not twins but exact inverses, and
 *     worth reading together: whatever Create loads, Destroy kills, and the
 *     order of the five sprite globals in Destroy is NOT Create's (the fifth
 *     is killed first).  Reproduced.
 *
 *  Singletons: Road_DrawSelection (+0x94) and MonkeyTree_Add (+0x98).
 *
 * ---------------------------------------------------------------------------
 * WHAT THE SAVE CHUNKS LOOK LIKE  (the +0xbc format, recovered here)
 * ---------------------------------------------------------------------------
 * All four are the write side of the loaders in screencb2.c / screencb3.c:
 * a null-terminated chain of `int 1` + one RAW record image, closed by an
 * `int 0`.  No pointers are edited out on the way -- the record's own `next`
 * field goes into the file and the loader overwrites it -- so the chunk is
 * (record size + 4) bytes per placement plus a trailing 4.  A failed write
 * anywhere abandons the chunk through ONE shared `return 0`, and the value
 * of the terminating write is the function's result (`neg/sbb/neg`, i.e.
 * `!= 0`).
 *
 * These four are the ONLY classes in this file with a save chunk;
 * ENTRANCE 1, the roads and the monkey trees carry no state a savegame
 * needs (the entrance's is in its ObjDef, the roads' in the road list which
 * DrivingSchool_Save writes, the trees' in the map itself).
 *
 * ---------------------------------------------------------------------------
 * ORIGINAL BUGS AND QUIRKS REPRODUCED
 * ---------------------------------------------------------------------------
 *  - Road_DrawSelection sets the preview cursor's ERROR to 1 and its flags
 *    to 8 BEFORE it knows whether the block under the mouse is a plain road,
 *    then clears the footprint again when the kind is not 6.  So the error
 *    is armed on every road square and only the footprint distinguishes
 *    them; a caller reading the error alone sees "bad" everywhere.
 *  - Carousel_Remove / Restaurant1_Remove call their class's FindRec with
 *    the ADDRESS of the by-value square parameter and then hand the same
 *    parameter on by value twice more -- the packed {u8,u8} dword-plus-mask
 *    idiom, including the misaligned `mov eax,[esp+0x31]` read of byte 1.
 *  - Entrance1_Create writes its ObjDef global and then RE-READS it one
 *    instruction later to fetch the sprite (`mov ecx,[0x6160f4]`), exactly
 *    as screencb3.c's Restaurant1_Create does.  Reproduced by reading the
 *    global at the second use instead of reusing the local.
 *  - Entrance1_Destroy publishes the ObjDef global on the way OUT (it is
 *    written from `elem->data` while the sprites are being killed), so a
 *    destroyed ENTRANCE 1 leaves a live pointer behind.
 *  - MonkeyTree_Add ignores JungleCruise_ProbeRiver's return value: it keeps
 *    only the owner square the probe writes through its out-pointer, so a
 *    tree placed with no river around it is filed under owner {0,0}.
 * ---------------------------------------------------------------------------
 * WHAT THIS FILE MEASURED (levers, with the evidence)
 * ---------------------------------------------------------------------------
 *  - SIX CALLS, ONE `add esp,0x30`.  Entrance1_Create makes six __cdecl
 *    calls and cleans up ONCE at the end.  That is the recorded "a call
 *    result passed straight into another call splits the add esp" rule from
 *    the other side: NOTHING here consumes a call result as an argument (each
 *    LoadSprite result goes to a global), so VC6 defers every cleanup.  Its
 *    inverse, Entrance1_Destroy, cannot -- each guarded KillSprite pops its
 *    own argument, and the LAST one uses `pop ecx` because it sits directly
 *    before the shared `ret`.
 *  - A DEAD PARAMETER SLOT HOLDS THE PACKED SQUARE.  MonkeyTree_Add
 *    root-copies `pos` into esi at entry and then writes the two bytes of the
 *    packed square into `pos`'s OWN incoming slot, so the whole function's
 *    frame is one `push ecx` for the probe's out-parameter.  Reproduced by
 *    declaring the BPosW at function scope with `pos` used through the local
 *    -- an aggregate would have taken a fresh slot.
 *  - THE PACKED-SQUARE DWORD-PLUS-MASK IDIOM AT A BY-VALUE PARAMETER whose
 *    ADDRESS IS ALSO TAKEN: `Carousel_FindRec((MapSquare*)&sq)` keeps `sq`
 *    in its argument slot, so `sq.b.x` / `sq.b.y` come out as
 *    `mov edx,[esp+0x30] / and edx,0xff` and the MISALIGNED
 *    `mov eax,[esp+0x31] / and eax,0xff`.  Neither a `MapSquare*` parameter
 *    nor an `unsigned int` one produces both halves.
 *  - "READ THE CLASS GLOBAL DIRECTLY AT EVERY USE" AGAIN, at a create
 *    handler: `g_entrance_def = def; def->flags |= 0x20;` uses the local, and
 *    the very next statement re-reads the global for the sprite
 *    (`mov ecx,[0x6160f4]`).  Reusing `def` there loses the reload.
 *  - SIZE IS NOT EVIDENCE OF TWINNING, CONFIRMED AGAIN: nine of the ten
 *    functions here are 38 instructions and they fall into a 4-twin group, a
 *    2-twin group and four singletons.  The diff, not the size, found the
 *    groups -- and once found, all six twins came off one body first try.
 *
 * ---------------------------------------------------------------------------
 * EXTERN TYPE NOTES (caller-side levers -- nothing else is aligned to them)
 * ---------------------------------------------------------------------------
 *  - StandardRemoveObject (0x0045f220) and RemoveAllBlokesFromRide
 *    (0x0048a2e0) both take the packed square as a `BPosW` BY VALUE here
 *    (one pushed dword, no widening).  screencb.c declares 0x0045f220 three
 *    other ways and joust.c a fourth; do not align them.
 *  - JungleCruise_AddValue is declared `void` here and `void` in
 *    junglecruise.c, but its first parameter is a `BPosW` BY VALUE in both.
 *  - JungleCruise_ProbeRiver returns `int` in junglecruise.c; MonkeyTree_Add
 *    throws the result away, so it is declared `void` here -- an `int`
 *    return is byte-identical, but `void` says what the call site means.
 * ========================================================================= */

/* ---- geometry and the ODF class record (same offsets as screencb2.c) ---- */
typedef struct BPos  { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; unsigned char c[2]; } BPosW;
typedef struct Pos   { int x; int y; } Pos;

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
    unsigned char pad00[0x1c];
    unsigned int  flags;            /* +0x1c  0x20 = tick via +0xa8 */
    unsigned char pad20[0x64 - 0x20];
    Spr*          sprite;           /* +0x64 build sprite */
    unsigned char pad68[0xd0 - 0x68];
} RideDef;                          /* 0xd0 */

typedef struct RideElem {
    char*         name;             /* +0x00 */
    char*         image;            /* +0x04 */
    unsigned int  type_flags;       /* +0x08 */
    RideDef*      data;             /* +0x0c */
    int           refcount;         /* +0x10 */
} RideElem;

/* A packed map square as two bytes, the form the class helpers take. */
typedef struct MapSquare { unsigned char bx; unsigned char by; } MapSquare;

/* money.c's SoundSource.  kind 2 = "sourced at a map square". */
typedef struct SoundSource {
    int   kind;                     /* +0x00 */
    void* bloke;                    /* +0x04 */
    int   x;                        /* +0x08 */
    int   y;                        /* +0x0c */
} SoundSource;

/* The preview / destroy cursor (screencb2.c's Cursor, objmap2.c's). */
typedef struct Cursor {
    unsigned char  pad0000[0x1404];
    Pos            origin;          /* +0x1404 map square under the mouse */
    int            status;          /* +0x140c */
    int            error;           /* +0x1410 */
    Rect           rect;            /* +0x1414 the footprint being previewed */
    unsigned char  pad1428[0x1828 - 0x1428];
    unsigned int   flags;           /* +0x1828 */
    unsigned char  pad182c[0x1834 - 0x182c];
} Cursor;                           /* 0x1834 */

/* audiomisc.c's FX table entry. */
typedef struct FXEntry {
    const char* name;               /* +0x00 */
    void*       sample;             /* +0x04 */
} FXEntry;                          /* 0x08 */

/* ---- the save stream ---------------------------------------------------- */
extern int   SaveGameWrite(const void* buf, unsigned int n);    /* 0x0047d760 */
extern void* HeapAlloc_w(unsigned int size);                    /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                               /* 0x0049e4d0 */
extern void  UnSourceAndFadeAllSamplesFromSource(SoundSource* s,
                                                 int fade);     /* 0x00496c80 */


/* =========================================================================
 * GROUP A -- THE +0xbc SAVE HANDLERS  (one source, four classes)
 *
 * screencb2.c's EarthSlide_Save without the queue pass.  The head is read
 * into the walk pointer BEFORE the guard so its load sits in the push block,
 * and the two literal ints live in their own frame slots because their
 * addresses are handed to the writer.
 * ========================================================================= */

/* ridecb3.c's BalloonzRec (0x20 bytes, list head 0x00616060). */
typedef struct BalloonzRec {
    struct BalloonzRec* next;       /* +0x00 */
    unsigned short      square;     /* +0x04 packed {x,y} */
    unsigned char       pad06[0x20 - 6];
} BalloonzRec;                      /* 0x20 */

extern BalloonzRec* g_bz_recs;                                  /* 0x00616060 */

// FUNCTION: LEGOLAND 0x0042ba80
int Balloonz_Save(void)
{
    BalloonzRec* rec;
    int          one = 1;
    int          zero = 0;

    rec = g_bz_recs;
    while (rec != 0) {
        if (SaveGameWrite(&one, 4) == 0)
            return 0;
        if (SaveGameWrite(rec, 0x20) == 0)
            return 0;
        rec = rec->next;
    }
    return SaveGameWrite(&zero, 4) != 0;
}

/* screencb2.c's CarouselRec (0x2c bytes, list head 0x006160c4). */
typedef struct CarouselRec {
    struct CarouselRec* next;       /* +0x00 */
    unsigned short      square;     /* +0x04 packed {x,y} */
    unsigned char       pad06[0x2c - 6];
} CarouselRec;                      /* 0x2c */

extern CarouselRec* g_carousel_recs;                            /* 0x006160c4 */

// FUNCTION: LEGOLAND 0x0042c590
int Carousel_Save(void)
{
    CarouselRec* rec;
    int          one = 1;
    int          zero = 0;

    rec = g_carousel_recs;
    while (rec != 0) {
        if (SaveGameWrite(&one, 4) == 0)
            return 0;
        if (SaveGameWrite(rec, 0x2c) == 0)
            return 0;
        rec = rec->next;
    }
    return SaveGameWrite(&zero, 4) != 0;
}

/* screencb3.c's RestRec (0x0c bytes, list head 0x00616144). */
typedef struct RestRec {
    struct RestRec* next;           /* +0x00 */
    unsigned short  square;         /* +0x04 packed {x,y} */
    unsigned char   pad06[3];
    unsigned char   frame;          /* +0x09 the building's animation frame */
    unsigned char   pad0a[2];
} RestRec;                          /* 0x0c */

extern RestRec* g_rest1_recs;                                   /* 0x00616144 */

// FUNCTION: LEGOLAND 0x004322a0
int Restaurant1_Save(void)
{
    RestRec* rec;
    int      one = 1;
    int      zero = 0;

    rec = g_rest1_recs;
    while (rec != 0) {
        if (SaveGameWrite(&one, 4) == 0)
            return 0;
        if (SaveGameWrite(rec, 0x0c) == 0)
            return 0;
        rec = rec->next;
    }
    return SaveGameWrite(&zero, 4) != 0;
}

/* ridecb3.c's RestRec2 (0x40 bytes, list head 0x00616148). */
typedef struct RestRec2 {
    struct RestRec2* next;          /* +0x00 */
    unsigned short   square;        /* +0x04 packed {x,y} */
    unsigned char    pad06[0x40 - 6];
} RestRec2;                         /* 0x40 */

extern RestRec2* g_r2_recs;                                     /* 0x00616148 */

// FUNCTION: LEGOLAND 0x00432390
int Restaurant2_Save(void)
{
    RestRec2* rec;
    int       one = 1;
    int       zero = 0;

    rec = g_r2_recs;
    while (rec != 0) {
        if (SaveGameWrite(&one, 4) == 0)
            return 0;
        if (SaveGameWrite(rec, 0x40) == 0)
            return 0;
        rec = rec->next;
    }
    return SaveGameWrite(&zero, 4) != 0;
}


/* =========================================================================
 * GROUP B -- THE +0x9c REMOVE HANDLERS  (one source, two classes)
 *
 * Drop the per-placement record if there is one, run the standard object
 * removal, evict every rider the class had on this square, and fade the
 * ride's sample out.  The packed square is a BY-VALUE parameter whose
 * ADDRESS is handed to FindRec, so it keeps its incoming argument slot and
 * the two byte reads for the fade come out of that slot as the dword /
 * dword-plus-one pair.
 * ========================================================================= */

extern void StandardRemoveObject(RideElem* o, BPosW sq, Cursor* ctx);
                                                                /* 0x0045f220 */
extern void RemoveAllBlokesFromRide(RideDef* cls, BPosW sq);    /* 0x0048a2e0 */

extern CarouselRec* Carousel_FindRec(MapSquare* sq);            /* 0x0042bc60 */
extern void         Carousel_FreeRec(CarouselRec* rec);         /* 0x0042bc00 */

// FUNCTION: LEGOLAND 0x0042c4a0
void Carousel_Remove(RideElem* o, BPosW sq, Cursor* ctx)
{
    SoundSource  src;
    CarouselRec* rec = Carousel_FindRec((MapSquare*)&sq);

    if (rec != 0)
        Carousel_FreeRec(rec);
    StandardRemoveObject(o, sq, ctx);
    RemoveAllBlokesFromRide(o->data, sq);

    src.kind = 2;
    src.x = sq.b.x;
    src.y = sq.b.y;
    UnSourceAndFadeAllSamplesFromSource(&src, -200);
}

extern RestRec* Restaurant1_FindRec(MapSquare* sq);             /* 0x0042ef40 */
extern void     Restaurant1_FreeRec(RestRec* rec);              /* 0x0042ef70 */

// FUNCTION: LEGOLAND 0x0042efb0
void Restaurant1_Remove(RideElem* o, BPosW sq, Cursor* ctx)
{
    SoundSource src;
    RestRec*    rec = Restaurant1_FindRec((MapSquare*)&sq);

    if (rec != 0)
        Restaurant1_FreeRec(rec);
    StandardRemoveObject(o, sq, ctx);
    RemoveAllBlokesFromRide(o->data, sq);

    src.kind = 2;
    src.x = sq.b.x;
    src.y = sq.b.y;
    UnSourceAndFadeAllSamplesFromSource(&src, -200);
}


/* =========================================================================
 * GROUP C -- ENTRANCE 1's CREATE / DESTROY PAIR
 *
 * The park entrance: one sound effect (turnstyles.wav), the money effects,
 * four occlusion mattes and the booth sprite.  Create publishes the ObjDef
 * and the build sprite in module globals and arms the +0xa8 tick (0x20) and
 * the +0xb0 custom draw (0x2000); Destroy kills the five sprites, each
 * guarded by its own null test.
 *
 * Note that ALL SIX calls in Create share ONE `add esp,0x30`: nothing
 * between them consumes a call result as an argument, so VC6 defers every
 * cleanup to the end.  Destroy cannot do that -- its Kill_FXList result is
 * unused but the guarded KillSprite calls each pop their own argument, and
 * the LAST one uses `pop ecx` because it is the final instruction before the
 * shared `ret`.
 * ========================================================================= */

extern void Load_FXList(FXEntry* list, int count);              /* 0x00496dd0 */
extern void Kill_FXList(FXEntry* list, int count);              /* 0x00496e30 */
extern void LoadMoneySFX(void);                                 /* 0x00453900 */
extern void KillMoneySFX(void);                                 /* 0x00453930 */
extern Spr* LoadSprite(const char* name, int mode);             /* 0x00497ab0 */
extern void KillSprite(Spr* s);                                 /* 0x00497bd0 */

/* The entrance's one-entry FX table: {"turnstyles.wav", sample}. */
extern FXEntry g_entrance_fx[];                                 /* 0x004b6668 */

extern RideDef* g_entrance_def;                                 /* 0x006160f4 */
extern Spr*     g_entrance_layers;                              /* 0x006160f0 */
extern Spr*     g_entrance_matte1;                              /* 0x006160fc */
extern Spr*     g_entrance_matte2;                              /* 0x00616100 */
extern Spr*     g_entrance_matte3;                              /* 0x00616104 */
extern Spr*     g_entrance_matte4;                              /* 0x00616108 */
extern Spr*     g_entrance_booth;                               /* 0x0061610c */

static const char kMatte1[] = "entrance_matte1.lls";
static const char kMatte2[] = "entrance_matte2.lls";
static const char kMatte3[] = "entrance_matte3.lls";
static const char kMatte4[] = "entrance_matte4.lls";
static const char kBooth[]  = "booth1.lls";

// FUNCTION: LEGOLAND 0x0042de50
void Entrance1_Create(RideElem* elem)
{
    RideDef* def;
    Spr*     spr;

    Load_FXList(g_entrance_fx, 1);
    LoadMoneySFX();
    def = elem->data;
    g_entrance_def = def;
    def->flags |= 0x20;
    spr = g_entrance_def->sprite;
    g_entrance_layers = spr;
    spr->flags |= 0x2000;
    g_entrance_matte1 = LoadSprite(kMatte1, 1);
    g_entrance_matte2 = LoadSprite(kMatte2, 1);
    g_entrance_matte3 = LoadSprite(kMatte3, 1);
    g_entrance_matte4 = LoadSprite(kMatte4, 1);
    g_entrance_booth = LoadSprite(kBooth, 1);
}

// FUNCTION: LEGOLAND 0x0042def0
void Entrance1_Destroy(RideElem* elem)
{
    Kill_FXList(g_entrance_fx, 1);
    KillMoneySFX();
    g_entrance_def = elem->data;
    if (g_entrance_booth != 0)
        KillSprite(g_entrance_booth);
    if (g_entrance_matte1 != 0)
        KillSprite(g_entrance_matte1);
    if (g_entrance_matte2 != 0)
        KillSprite(g_entrance_matte2);
    if (g_entrance_matte3 != 0)
        KillSprite(g_entrance_matte3);
    if (g_entrance_matte4 != 0)
        KillSprite(g_entrance_matte4);
}


/* =========================================================================
 * 0x00413fa0 -- DRIVING SCHOOL ROADS +0x94, and ZEBRA CROSSING's +0x94 too:
 * SetCustomCallbacks installs this ONE address in both classes' slots, which
 * is why it takes the class element it is given and never reads it.
 *
 * The +0x94 slot paints the "what is under the mouse" preview for a class
 * that is already placed.  It snaps the ghost cursor onto the road block the
 * mouse is over, gives it the block's own 4x4 rect and the standard preview
 * flags, and marks it error 1.  Only a road piece of kind 6 keeps its
 * footprint; anything else has it cleared again, so the highlight shows
 * exactly the blocks the tool would act on.
 * ========================================================================= */

/* screencb2.c's RoadTile. */
typedef struct RoadTile {
    struct RoadTile* next;          /* +0x00 */
    unsigned char    pad04[4];
    unsigned short   school;        /* +0x08 the owning school's map square */
    unsigned char    pad0a[2];
    int              x;             /* +0x0c map square */
    int              y;             /* +0x10 */
    unsigned char    type;          /* +0x14 low nibble kind, 0x10 = crossing */
} RoadTile;

extern RoadTile* GetRoadRecord(int x, int y);                   /* 0x004125f0 */
extern void      SetCursorError(Cursor* c, int code);           /* 0x0045f480 */
extern void      ResetCursorFootprint(Cursor* c);               /* 0x0045f460 */

/* logflume.c's g_ghost_cursor / castleobj.c's QueryCursor. */
extern Cursor g_ghost_cursor;                                   /* 0x00810160 */

/* The 4x4 block one road record covers (0x004b4bf0). */
static const Rect kRoadBlockRect = { 0, 0, 3, 3, 0 };

// FUNCTION: LEGOLAND 0x00413fa0
void Road_DrawSelection(RideElem* elem, Pos* pos)
{
    RoadTile* rec = GetRoadRecord(pos->x, pos->y);

    if (rec != 0) {
        g_ghost_cursor.origin.x = rec->x;
        g_ghost_cursor.origin.y = rec->y;
        g_ghost_cursor.rect = kRoadBlockRect;
        g_ghost_cursor.flags = 8;
        SetCursorError(&g_ghost_cursor, 1);
        if ((rec->type & 0x0f) != 6)
            ResetCursorFootprint(&g_ghost_cursor);
    }
}


/* =========================================================================
 * 0x00433d20 -- JUNGLE CRUISE MONKEY TREE +0x98  (place one monkey tree).
 *
 * Pack the placement square, ask JungleCruise_ProbeRiver which station's
 * river the square belongs to, allocate the 8-byte JcMonkeyTree, push it on
 * the class list at 0x00629c2c, put the object on the map and add one point
 * to the owning station's value.  junglecruise.c's MonkeyTree_Remove is the
 * exact inverse (unlink, refund one, free).
 *
 * The packed square is a LOCAL homed in the DEAD `pos` argument slot -- `pos`
 * is root-copied into esi at entry, which frees its incoming slot -- so the
 * function has only a `push ecx` frame for the probe's out-parameter.
 * ========================================================================= */

/* junglecruise.c's JcMonkeyTree (list head 0x00629c2c). */
typedef struct JcMonkeyTree {
    BPosW                pos;       /* +0x00 */
    BPosW                owner;     /* +0x02 the station whose river it is */
    struct JcMonkeyTree* next;      /* +0x04 */
} JcMonkeyTree;                     /* 0x08 */

extern JcMonkeyTree* g_jc_trees;                                /* 0x00629c2c */

#ifndef LEGOLAND_PORTABLE
extern void JungleCruise_ProbeRiver(int x, int y, BPosW* owner);/* 0x00436fb0 */
#else
extern int JungleCruise_ProbeRiver(int x, int y, BPosW* owner);/* 0x00436fb0 */
#endif
extern void JungleCruise_AddValue(BPosW id, int delta);         /* 0x00436130 */
extern void AddBasicObject(void* obj, Pos* pos);                /* 0x0045efe0 */

// FUNCTION: LEGOLAND 0x00433d20
void MonkeyTree_Add(void* o, Pos* pos)
{
    BPosW         owner;
    BPosW         sq;
    JcMonkeyTree* t;

    sq.b.x = (unsigned char)pos->x;
    sq.b.y = (unsigned char)pos->y;
    JungleCruise_ProbeRiver(pos->x, pos->y, &owner);
    t = (JcMonkeyTree*)HeapAlloc_w(sizeof(JcMonkeyTree));
    if (t != 0) {
        t->pos.w = sq.w;
        t->owner.w = owner.w;
        t->next = g_jc_trees;
        g_jc_trees = t;
        AddBasicObject(o, pos);
        JungleCruise_AddValue(owner, 1);
    }
}
