/* LEGOLAND -- WATER WORKS (five classes) and the GARDEN (two classes).
 *
 * Every function here is an unexported per-class callback installed by
 * WaterWorks_GetInterfaces (0x00418c80) or Garden_GetInterfaces (0x004329c0)
 * -- both already matched in LEGOLAND/interfaces.c -- plus the unexported
 * helpers those callbacks share.  Extents were taken from the disassembly by
 * control flow (tools/audit.py); the names come from the provider tables in
 * docs/RIDE_CALLBACKS.md.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere).
 *
 * ==========================================================================
 * WHAT THE WATER WORKS IS
 * ==========================================================================
 * A water park built out of five ODF classes that all share ONE sound-effect
 * table and ONE reference count:
 *
 *   g_ww_fx (0x004b4fa8), three FXEntry{char* name; int; void* sample}:
 *      [0] "WaterworksShower01.wav"   the splash a person makes in the pool
 *                                     and under a shower
 *      [1] "spray or fountain.wav"    looped at the ENTRANCE
 *      [2] "Fountain01.wav"           looped at the CROCODILE FOUNTAIN
 *   g_ww_fx_refs (0x004cc028): every class's cb_a4 (create) bumps it and
 *   every cb_ac (destroy) drops it; the table is loaded on the 0->1 edge and
 *   killed on the 1->0 edge.  That is why the CROCODILE FOUNTAIN -- which has
 *   no create handler at all -- can still play g_ww_fx[2]: some OTHER water
 *   works class has always loaded the table for it.  Place a crocodile
 *   fountain in a park with no other water works piece and it is silent.
 *
 * THREE CLASSES OWN A RECORD LIST, all of the same 0x0c-byte shape:
 *
 *   WaterRec: next @+0x00, packed map square @+0x04 {u8 x, u8 y},
 *             state @+0x08, timer @+0x09, frame @+0x0a
 *
 *     g_waterblock_head (0x004cc02c)  WATER BLOCK      saved (ridesave.c)
 *     g_shower_head     (0x004cc030)  SHOWER           NOT saved
 *     g_elephantf_head  (0x004cc034)  ELEPHANT FOUNTAIN saved (ridesave.c)
 *
 *   ridesave.c's "0x0c bytes, next @+0x00" for WATER BLOCK and ELEPHANT F is
 *   exactly this record, written raw -- so a saved game restores the running
 *   splash animation state and frame, and the SHOWER's does not survive a
 *   save because that class has no cb_b8/cb_bc pair.
 *
 * THE ANIMATION IS DRIVEN BY PEOPLE STANDING ON THE SQUARE.  Each class's
 * cb_a8 ("activate", the per-tick entry) first runs a scan pass that arms any
 * record whose square has a person on it (state 0 -> 1), then walks the list
 * running the state machine.  The people test is the same for the pool and
 * the shower -- WW_AnyBlokeOnSquare, a 1x1 rect over the record's own square
 * against all THREE person lists (visitors, gardeners, handymen) -- while the
 * elephant fountain tests a 4x4 box anchored on the class footprint instead
 * (WW_AnyBlokeInFountainBox), because its splash lands in front of it.
 *
 * THE SECONDARY UPDATE SLOT (+0x90).  Water Works is one of only two
 * subsystems in the game that fills +0x90 (docs/RIDE_CALLBACKS.md note 6).
 * All four +0x90 handlers here turn out to be the same thing: the PLACEMENT
 * CURSOR update -- copy the class footprint into the edit cursor, project the
 * mouse to a map square, validate, and then REFUSE the placement with cursor
 * error 12 unless a WATER WORKS ENTRANCE already exists (WW_HasEntrance).
 * So the +0x90 slot here is "can this piece be dropped here", and the whole
 * subsystem is gated on its entrance being built first.
 *
 * ==========================================================================
 * WHAT THE GARDEN IS
 * ==========================================================================
 * Two pure-scenery classes, HEDGE and FLOWERS, and the only provider in the
 * game that compares class names with a case-SENSITIVE strcmp
 * (interfaces.c, 0x004329c0).  Neither owns a record list: their entire
 * per-placement state is the cell's USER FLAGS byte (Get_/Set_UserFlags),
 * used as an index into the class's .ILF image list.
 *
 *   HEDGE    picks its image from its four orthogonal NEIGHBOURS, so a run of
 *            hedges auto-joins; placing or removing one re-stitches itself
 *            and each neighbour that is also a hedge (Hedge_Restitch).
 *   FLOWERS  picks a RANDOM image once, at placement, and never changes --
 *            which is why FLOWERS needs no remove handler: there is nothing
 *            to tell the neighbours.
 *
 * Because the provider's compare is case sensitive, an .ODF that spelled
 * either class "Hedge" or "Flowers" would still load and still be placeable,
 * but would get NONE of these callbacks: no create (so no image list), no
 * draw override (so the shared draw descriptor is never filled), and for the
 * hedge no add/remove (so no user-flag index at all).  It would place as an
 * untextured object and draw with whatever the previous class left in the
 * shared descriptor.  The other fourteen providers use the _stricmp at
 * 0x004aab90 and are immune.
 *
 * ==========================================================================
 * THREE VC6 LEVERS THIS FILE PINNED DOWN (all new; see docs/DECOMP.md)
 * ==========================================================================
 * 1. A TWO-BYTE COMPARE IS AN INTRINSIC memcmp, NOT `==`.  WW_ListFind's
 *    square test is `memcmp(&p->sq, sq, 2) != 0` under
 *    `#pragma intrinsic(memcmp)`.  VC6's length-2 expansion is
 *    `lea r,<a> / mov r16,<a> / cmp r16,<b>`: the `lea` is the intrinsic's
 *    first-operand address and is left DEAD once the load folds the
 *    addressing mode back onto the base register, and the SECOND operand
 *    stays a memory operand instead of being hoisted out of the loop.  Every
 *    `a->w == b->w` spelling -- through a pointer, a union, an inlined
 *    helper, a struct copy, with or without `volatile` -- instead hoists
 *    `*b` into a register before the loop and drops both leas, four
 *    instructions short of the original.
 *
 * 2. A SMALL FIXED-SIZE CLEAR IS AN INTRINSIC memset.  The three record
 *    allocators clear 12 bytes with `memset(rec, 0, sizeof rec)` under
 *    `#pragma intrinsic(memset)`.  VC6's expansion copies the destination
 *    pointer into a SCRATCH register (`mov edx,eax`) and zeroes through that
 *    while the original pointer stays live for the following field store and
 *    the list push -- 17 instructions.  Three separate `= 0` field stores
 *    coalesce onto the one register, hoist the zero above the null test, and
 *    come out one instruction short every time.
 *
 * 3. A CONDITIONAL ARGUMENT IS TWO CALLS, TAIL-MERGED.  Hedge_Restitch's
 *    `Set_UserFlags(x, y, mask ? mask - 1 : 0x0e)` written as a ternary
 *    computes the value into a register and pushes once.  The original has
 *    `push 0xe / jmp` in one arm and `dec ebx / push ebx` in the other
 *    falling into ONE shared push/call, which is what VC6 produces from two
 *    complete `Set_UserFlags(...)` calls in an if/else -- it tail-merges the
 *    common suffix.  The zero case must be the INLINE arm (`if (mask == 0)`)
 *    or the two blocks come out the other way round.
 * ========================================================================== */

/* ---- packed map squares -------------------------------------------------- */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

typedef struct Pos { int x; int y; } Pos;

/* The class footprint rect, 0x14 bytes (copied whole into the edit cursor). */
typedef struct Rect {
    int          left;              /* +0x00 */
    int          top;               /* +0x04 */
    int          right;             /* +0x08 */
    int          bottom;            /* +0x0c */
    struct Rect* next;              /* +0x10 */
} Rect;

/* A loaded sprite/layer holder; only the flag word matters here. */
typedef struct Spr {
    unsigned char pad00[0x10];
    unsigned int  flags;            /* +0x10  0x2000 = drawn via the +0xa0 slot */
} Spr;

/* The .ILF/.CSP parsed table (llidb_load.c's IlfData). */
typedef struct IlfData {
    int    f00;                     /* +0x00 */
    int    count;                   /* +0x04  number of images */
    void** sprites;                 /* +0x08 */
    int*   dx;                      /* +0x0c  (doubled at load) */
    int*   dy;                      /* +0x10 */
} IlfData;

/* The 0xd0-byte ObjDef, through the fields these classes touch. */
struct RiderNode;
typedef struct WWDef {
    unsigned char     pad00[0x0c];
    int               base_x;       /* +0x0c  class origin, in tiles */
    int               base_y;       /* +0x10 */
    int               f14;          /* +0x14  draw-descriptor field 1 */
    int               f18;          /* +0x18  draw-descriptor field 2 */
    unsigned int      flags;        /* +0x1c  0x20 tick, 0x400 custom draw,
                                     *        0x004 ask the class for its image */
    unsigned char     pad20[0x3c - 0x20];
    Rect              rect;         /* +0x3c  class footprint */
    unsigned char     pad50[0x64 - 0x50];
    Spr*              layers;       /* +0x64  build sprite / layer holder */
    unsigned char     pad68[0xcc - 0x68];
    struct RiderNode* riders;       /* +0xcc  class-wide rider list */
} WWDef;

typedef struct RideElem {
    char*        name;              /* +0x00 */
    char*        image;             /* +0x04 */
    unsigned int type_flags;        /* +0x08 */
    WWDef*       data;              /* +0x0c */
} RideElem;

/* A placed map object; its class record is at +0x0c. */
typedef struct WWMapObj {
    unsigned char pad00[0x0c];
    WWDef*        cls;              /* +0x0c */
} WWMapObj;

/* A person, through the fields the "is anybody standing here" test and the
 * pool's own walk-into-the-water script read (same offsets as westtown.c). */
typedef struct Bloke {
    struct Bloke*  next;            /* +0x00 */
    unsigned char  pad04[0x0e - 4];
    unsigned short busy;            /* +0x0e  move countdown */
    unsigned char  pad10[0x24 - 0x10];
    int            tx;              /* +0x24  target, 24.8 world units */
    int            ty;              /* +0x28 */
    unsigned char  pad2c[0x60 - 0x2c];
    unsigned char  action;          /* +0x60  the class script's step index */
    unsigned char  pad61;
    unsigned short flags62;         /* +0x62  bit 8 = inside a building */
    unsigned char  pad64[0x68 - 0x64];
    int            wx;              /* +0x68  world position, 24.8 */
    int            wy;              /* +0x6c */
    unsigned char  pad70[0x73 - 0x70];
    unsigned char  heading;         /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    int            path;            /* +0x98  path scratch */
} Bloke;

/* The class-wide rider list node (rides.c's PutBlokeInList feeds it). */
typedef struct RiderNode {
    struct RiderNode* next;         /* +0x00 */
    unsigned char     pad04[4];
    Bloke*            bloke;        /* +0x08 */
    BPosW             key;          /* +0x0c  the placement's map square */
} RiderNode;

/* The per-placement record all three record-owning classes use. */
typedef struct WaterRec {
    struct WaterRec* next;          /* +0x00 */
    BPosW            sq;            /* +0x04  the map square it sits on */
    unsigned short   pad06;         /* +0x06 */
    unsigned char    state;         /* +0x08  0 idle, 1..3 per class */
    unsigned char    timer;         /* +0x09  frame counter within the state */
    unsigned char    frame;         /* +0x0a  image index (WATER BLOCK) */
    unsigned char    pad0b;         /* +0x0b */
} WaterRec;

/* A sound source; kind 2 = "at this map square" (input2.c / objmap2.c). */
typedef struct SoundSource {
    int kind;                       /* +0x00 */
    int pad4;                       /* +0x04 */
    int x;                          /* +0x08 */
    int y;                          /* +0x0c */
} SoundSource;

/* One entry of a sound-effect table (audiomisc.c / loaders.c). */
typedef struct FXEntry {
    char* name;                     /* +0x00 */
    int   pad4;                     /* +0x04 */
    void* sample;                   /* +0x08  filled in by Load_FXList */
} FXEntry;

/* The block a +0xa0 (draw) slot returns.  Western Town's g_shop_draw
 * (0x0082c6a0, westtown.c) is the same five fields; the water works has its
 * own copy at 0x004cbff0 and the garden shares Western Town's. */
typedef struct DrawDesc {
    Spr*           sprite;          /* +0x00 */
    int            f04;             /* +0x04 */
    int            f08;             /* +0x08 */
    unsigned short f0c;             /* +0x0c  the placement's map square */
    unsigned short pad0e;           /* +0x0e */
    int            f10;             /* +0x10 */
} DrawDesc;

/* The LLS a sprite resolves to; +0x10 is its frame count. */
typedef struct LLS {
    unsigned char pad00[0x10];
    short         frames;           /* +0x10 */
} LLS;

/* The edit cursor and the globals the placement update writes. */
typedef struct Footprint { int v[4]; void* parts; } Footprint;

/* The bare four-int rectangle the people test builds on its own stack. */
typedef struct WinRect { int left; int top; int right; int bottom; } WinRect;

int   memcmp(const void* a, const void* b, unsigned int n);
void* memset(void* p, int v, unsigned int n);
#pragma intrinsic(memcmp)
#pragma intrinsic(memset)

/* ---- engine entry points ------------------------------------------------ */
extern void   Load_FXList(FXEntry* list, int count);                 /* 0x00496dd0 */
extern void   Kill_FXList(FXEntry* list, int count);                 /* 0x00496e30 */
extern void   PlayInstanceOfSample(void* s, int a, int b, SoundSource* q); /* 0x00496d20 */
extern void   UnSourceAndFadeAllSamplesFromSource(SoundSource* s, int f); /* 0x00496c80 */
extern void   AddBasicObject(WWMapObj* obj, Pos* pos);               /* 0x0045efe0 */
extern void   AddObjectToMap(WWMapObj* obj, BPos bp, unsigned int flags); /* 0x0045dd80 */
extern void   RemoveSoundObject(WWMapObj* obj, BPosW sq, void* c);   /* 0x00452a30 */
extern void   StandardRemoveObject(WWMapObj* obj, BPosW sq, void* c);/* 0x0045f220 */
extern void*  GetFirstObjectMatching(RideElem* elem);                /* 0x0045a910 */
extern void*  MemAlloc(unsigned int size);                           /* 0x0049e4ff */
extern void   MemFree(void* p);                                      /* 0x0049e4d0 */
extern int    rand(void);                                            /* 0x0049e4b2 (CRT) */
extern int    LLIDB_FindElement(const char* n, void** o, unsigned int* i); /* 0x0047b330 */
extern void*  LLIDB_LoadData(void* elem);                            /* 0x0047d3a0 */
extern void   LLIDB_UnLoadData(void* elem);                          /* 0x0047d450 */
extern Spr*   LoadSprite(const char* name, int mode);                /* 0x00497ab0 */
extern int    KillSprite(Spr* s);                                    /* 0x00497bd0 */
extern void   ScreenToMapRef(int screen, Pos* out, int mode);        /* 0x0045be90 */
extern void   ValidateCursor(void* cursor, WWDef* def);              /* 0x0045f810 */
extern void   SetCursorError(void* cursor, int code);                /* 0x0045f480 */
extern void   PropagateCursorStatus(void* cursor);                   /* 0x0045f4d0 */
extern void   DefaultCursor(void* cursor);                           /* 0x0045a390 */
extern void   SetEditCursorFootPrint(Footprint* fp);                 /* 0x0045f440 */
extern LLS*   GetLLSForSprite(void* sprite);                         /* 0x00441e80 */
extern void   DBPrintf(const char* fmt, ...);                        /* 0x00453a20 */
extern void   RemoveBlokeFromRide(WWDef* def, RiderNode* r);         /* 0x0048a100 */
extern int    CalcMoveLine(int fx, int fy, int tx, int ty, void* path); /* 0x00480740 */
extern int    NewDirForAction(Bloke* b, unsigned char dir);          /* 0x004833d0 */
extern void   SetOverrideFrame(int frame);                           /* 0x00464420 */
extern void   ClearOverrideFrame(void);                              /* 0x00464440 */
extern void   AdjustOffsetForViewMode(Pos* o);                       /* 0x00442d30 */
extern Pos    GetScreenCoordsForObject(BPosW* sq, WWDef* def);       /* 0x00442cc0 */
extern void   LLSSetFrame(LLS* lls, int frame);                      /* 0x0047d5a0 */
extern int    PrintSprite(Spr* s, int x, int y, int mode, void* ctx);/* 0x004853a0 */
extern void   SortSprite(Spr* s, int x, int y, int a, int b, void* ctx); /* 0x00485d70 */
extern void   IP_RenderBlokeIn3DNow(Bloke* b);                       /* 0x00440010 */
extern WWDef* GetObjectClassAndInstance(Pos* pos, unsigned short* out_bpos); /* 0x00461850 */
extern unsigned short Get_UserFlags(int x, int y);                   /* 0x00461710 */
extern void   Set_UserFlags(int x, int y, unsigned short value);     /* 0x00461730 */

/* ---- the three person lists the water tests scan ------------------------- */
extern Bloke* g_people_head;        /* 0x0066b574  visitors (blokeai.c) */
extern Bloke* g_gardener_list;      /* 0x0079a8a8  gardeners (savechunks.c) */
extern Bloke* g_handyman_list;      /* 0x0079a8ac  second worker list */

/* ---- shared placement/edit globals --------------------------------------- */
extern char      g_edit_cursor;     /* 0x007febc0  EditCursor */
extern Pos       g_mapref;          /* 0x007fffc4  ScreenToMapRef output */
extern Rect      g_edit_footprint;  /* 0x007fffd4 */
extern void*     g_8003f0;          /* 0x008003f0  preview-cursor chain */
extern int       g_edit_changed;    /* 0x008119b0  EditMode */
extern WWDef*    g_edit_object;     /* 0x008119b8  the class being placed */

/* ---- WATER WORKS module state -------------------------------------------- */
extern FXEntry   g_ww_fx[3];        /* 0x004b4fa8 */
extern int       g_ww_fx_refs;      /* 0x004cc028 */

extern RideElem* g_wwentrance_elem; /* 0x004cbfe4 */
extern WWDef*    g_wwentrance_def;  /* 0x004cc01c */

extern WWDef*    g_wb_def;          /* 0x004cc008  WATER BLOCK */
extern void*     g_wb_ilf_elem;     /* 0x004cbfe8  "WATER WORKS IMAGE LIST" */
extern IlfData*  g_wb_ilf;          /* 0x004cc018 */
extern Spr*      g_wb_sprite;       /* 0x004cbfd8  topwater.lls */
extern WaterRec* g_waterblock_head; /* 0x004cc02c */

extern WWDef*    g_shower_def;      /* 0x004cc024  SHOWER */
extern Spr*      g_shower_layers;   /* 0x004cbfe0 */
extern Spr*      g_shower_sprite;   /* 0x004cc014  shower.lls */
extern WaterRec* g_shower_head;     /* 0x004cc030 */

extern WWDef*    g_elephantf_def;   /* 0x004cbfdc  ELEPHANT FOUNTAIN */
extern Spr*      g_elephantf_layers;/* 0x004cbfec */
extern Spr*      g_elephantf_sprite;/* 0x004cc020  ww_elsquirt.lls */
extern WaterRec* g_elephantf_head;  /* 0x004cc034 */

extern DrawDesc  g_water_draw;      /* 0x004cbff0 */

/* .rdata strings */
extern char g_s_ww_image_list[];    /* 0x004b501c "WATER WORKS IMAGE LIST" */
extern char g_s_topwater[];         /* 0x004b500c "topwater.lls" */
extern char g_s_shower_lls[];       /* 0x004b506c "shower.lls" */
extern char g_s_elsquirt_lls[];     /* 0x004b5078 "ww_elsquirt.lls" */
extern char g_s_bad_block[];        /* 0x004b5034 "Bad Block Index (%d)\n" */
extern char g_s_bad_sprite[];       /* 0x004b504c "Bad Sprite in Waterworks block\n" */

/* ==========================================================================
 * THE SHARED SOUND-EFFECT REFERENCE COUNT
 *
 * Note the post-increment shape: the OLD value decides whether to load, so
 * the table loads on the 0 -> 1 edge; the decrement tests the NEW value, so
 * it unloads on the 1 -> 0 edge.  Written asymmetrically in the original
 * (inc tests the old, dec tests the new) and reproduced as written.
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00417a90
void WW_LoadFX(void)
{
    if (g_ww_fx_refs++ == 0)
        Load_FXList(g_ww_fx, 3);
}

// FUNCTION: LEGOLAND 0x00417ac0
void WW_KillFX(void)
{
    if (--g_ww_fx_refs == 0)
        Kill_FXList(g_ww_fx, 3);
}

/* Play g_ww_fx[which] sourced at a map square. */
// FUNCTION: LEGOLAND 0x00417af0
void WW_PlaySfx(BPos* sq, int which, int a)
{
    SoundSource src;

    src.kind = 2;
    src.x = sq->x;
    src.y = sq->y;
    PlayInstanceOfSample(g_ww_fx[which].sample, a, 1, &src);
}

/* Fade out (-400) everything sourced at a map square. */
// FUNCTION: LEGOLAND 0x00417b40
void WW_FadeSfx(BPos* sq)
{
    SoundSource src;

    src.kind = 2;
    src.x = sq->x;
    src.y = sq->y;
    UnSourceAndFadeAllSamplesFromSource(&src, -400);
}

/* ==========================================================================
 * THE SHARED RECORD-LIST PRIMITIVES
 * One push / find / unlink used by all three record-owning classes; the head
 * is always passed in, which is why there is one copy and not three.
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00417b80
void WW_ListPush(WaterRec** head, WaterRec* node)
{
    if (node) {
        node->next = *head;
        *head = node;
    }
}

/* NEW VC6 LEVER.  The square compare is an INTRINSIC memcmp of length 2, not
 * a 16-bit `==`.  With `#pragma intrinsic(memcmp)` VC6 expands a 2-byte
 * compare to `lea edx,<a> / mov dx,<a> / cmp dx,<b>` -- the `lea` is the
 * intrinsic's first-operand address and is left DEAD once the load folds the
 * addressing mode back onto the base register, and the second operand stays a
 * memory operand instead of being hoisted out of the loop.  Every plain
 * `a->w == b->w` spelling instead hoists `*b` into a register before the loop
 * (and drops the leas), which is 4 instructions short of the original.  The
 * peeled first compare + rotated loop is the `if (p) { while (...) { p =
 * p->next; if (!p) return 0; } return p; } return 0;` shape. */
// FUNCTION: LEGOLAND 0x00417ba0
WaterRec* WW_ListFind(WaterRec* p, BPosW* sq)
{
    if (p) {
        while (memcmp(&p->sq, sq, 2) != 0) {
            p = p->next;
            if (!p)
                return 0;
        }
        return p;
    }
    return 0;
}

/* Unlink `node`.  The trailing `if (p)` is redundant on the normal exit --
 * VC6 threads the in-loop `break` straight to the return and cannot fold the
 * other arm, which is exactly the shape the original has. */
// FUNCTION: LEGOLAND 0x00417bd0
void WW_ListUnlink(WaterRec** head, WaterRec* node)
{
    WaterRec* p = *head;

    if (p == node) {
        *head = node->next;
        return;
    }
    while (p->next != node) {
        p = p->next;
        if (!p)
            break;
    }
    if (p)
        p->next = node->next;
}

/* ==========================================================================
 * WATER WORKS ENTRANCE
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00417c00
void WWEntrance_Create(RideElem* elem)
{
    g_wwentrance_elem = elem;
    g_wwentrance_def = elem->data;
    WW_LoadFX();
}

// FUNCTION: LEGOLAND 0x00417ae0
void WWEntrance_Destroy(void)
{
    WW_KillFX();
}

// FUNCTION: LEGOLAND 0x00417c20
void WWEntrance_Add(WWMapObj* obj, Pos* pos)
{
    SoundSource src;

    AddBasicObject(obj, pos);
    src.x = pos->x;
    src.kind = 2;
    src.y = pos->y;
    PlayInstanceOfSample(g_ww_fx[1].sample, 1, 1, &src);
}

// FUNCTION: LEGOLAND 0x00417c70
void WWEntrance_Remove(WWMapObj* obj, BPosW sq, void* c)
{
    RemoveSoundObject(obj, sq, c);
}

/* Is there a WATER WORKS ENTRANCE on the map at all?  Every water works
 * placement update refuses unless there is. */
// FUNCTION: LEGOLAND 0x00417c90
int WW_HasEntrance(void)
{
    return GetFirstObjectMatching(g_wwentrance_elem) != 0;
}

/* ==========================================================================
 * "IS ANYBODY STANDING THERE" -- the trigger for every water animation
 *
 * The rect test walks ONE person list; the square test runs it over all
 * three (visitors, gardeners, handymen) against a 1x1 rect on the record's
 * own square.  Positions are 24.8 world units, so `>> 8` is the tile.
 * ========================================================================== */

// WIP-FUNCTION: LEGOLAND 0x00417e70  (31/31 instructions and block layout; 12 mismatches = a pure eax<->ecx swap: the original keeps the list cursor in eax and the >>8 temp in ecx, VC6 does the reverse here)
int WW_AnyBlokeInRect(Bloke* b, WinRect* r)
{
    while (b) {
        int x = b->wx >> 8;
        if (x >= r->left && x <= r->right) {
            int y = b->wy >> 8;
            if (y >= r->top && y <= r->bottom)
                return 1;
        }
        b = b->next;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x00417ec0
int WW_AnyBlokeOnSquare(BPos* sq)
{
    WinRect r;

    r.left = sq->x;
    r.right = sq->x;
    r.top = sq->y;
    r.bottom = sq->y;
    if (WW_AnyBlokeInRect(g_people_head, &r))
        return 1;
    if (WW_AnyBlokeInRect(g_gardener_list, &r))
        return 1;
    return WW_AnyBlokeInRect(g_handyman_list, &r) != 0;
}

/* ==========================================================================
 * WATER WORKS WATER BLOCK -- the pool itself
 * ========================================================================== */

/* The record is cleared with an INTRINSIC memset, not three field stores:
 * VC6's 12-byte expansion copies the destination pointer into a scratch
 * register (`mov edx,eax`) and zeroes through THAT while the original
 * pointer stays live for the `rec->sq` store and the list push, which is
 * exactly the 17-instruction shape.  Three separate `= 0` stores coalesce
 * onto one register and come out one instruction short. */
// FUNCTION: LEGOLAND 0x00417cb0
void WaterBlock_NewRecord(BPosW sq)
{
    WaterRec* rec = (WaterRec*)MemAlloc(sizeof(WaterRec));

    if (rec) {
        memset(rec, 0, sizeof(WaterRec));
        rec->sq = sq;
        WW_ListPush(&g_waterblock_head, rec);
    }
}

// FUNCTION: LEGOLAND 0x00417cf0
void WaterBlock_FreeRecord(WaterRec* rec)
{
    WW_ListUnlink(&g_waterblock_head, rec);
    MemFree(rec);
}

// FUNCTION: LEGOLAND 0x00417d10
WaterRec* WaterBlock_FindRecord(BPosW* sq)
{
    return WW_ListFind(g_waterblock_head, sq);
}

/* The pool's cb_a4.  Note the six zeroed ObjDef fields: the class footprint
 * rect AND the draw offsets are wiped, because the pool draws itself out of
 * the "WATER WORKS IMAGE LIST" per placement (WaterBlock_Draw) rather than
 * from the class's own sprite.  0x420 = ask this class for its draw
 * descriptor (0x400) + take part in the render sort (0x20). */
// FUNCTION: LEGOLAND 0x00417d30
void WaterBlock_Create(RideElem* elem)
{
    WW_LoadFX();
    g_wb_def = elem->data;
    g_wb_def->flags |= 0x420;
    g_wb_def->rect.left = 0;
    g_wb_def->rect.right = 0;
    g_wb_def->rect.top = 0;
    g_wb_def->rect.bottom = 0;
    g_wb_def->f14 = 0;
    g_wb_def->f18 = 0;
    if (LLIDB_FindElement(g_s_ww_image_list, &g_wb_ilf_elem, 0) == 0)
        g_wb_ilf = (IlfData*)LLIDB_LoadData(g_wb_ilf_elem);
    g_wb_sprite = LoadSprite(g_s_topwater, 1);
}

// FUNCTION: LEGOLAND 0x00417e40
void WaterBlock_Destroy(void)
{
    if (g_wb_sprite)
        KillSprite(g_wb_sprite);
    LLIDB_UnLoadData(g_wb_ilf_elem);
    WW_KillFX();
}

/* +0x90: the placement cursor.  Refused with error 12 unless a WATER WORKS
 * ENTRANCE is already on the map. */
// FUNCTION: LEGOLAND 0x00417dd0
void WaterBlock_Update(RideElem* elem, int screen, int mode)
{
    WWDef* def = elem->data;

    g_edit_footprint = def->rect;
    g_8003f0 = 0;
    ScreenToMapRef(screen, &g_mapref, mode);
    ValidateCursor(&g_edit_cursor, def);
    if (!WW_HasEntrance()) {
        SetCursorError(&g_edit_cursor, 12);
        PropagateCursorStatus(&g_edit_cursor);
    }
}

// FUNCTION: LEGOLAND 0x004181e0
void WaterBlock_Add(WWMapObj* obj, Pos* pos)
{
    BPosW     sq;
    WaterRec* rec;

    sq.b.x = (unsigned char)pos->x;
    sq.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    WaterBlock_NewRecord(sq);
    rec = WaterBlock_FindRecord(&sq);
    if (rec)
        rec->frame = (unsigned char)(rand() % g_wb_ilf->count);
}

// FUNCTION: LEGOLAND 0x00418230
void WaterBlock_Remove(WWMapObj* obj, BPosW sq, void* c)
{
    WaterRec* rec;

    StandardRemoveObject(obj, sq, c);
    rec = WaterBlock_FindRecord(&sq);
    if (rec)
        WaterBlock_FreeRecord(rec);
}

/* +0xa0: the pool's draw descriptor.  Each placement picks its own image out
 * of the class image list (the index was rolled at placement time) and the
 * running splash frame is pushed through the render override. */
// FUNCTION: LEGOLAND 0x00418110
DrawDesc* WaterBlock_Draw(WWMapObj* obj, BPosW sq)
{
    WaterRec* rec = WaterBlock_FindRecord(&sq);

    if (rec) {
        unsigned int i = rec->frame;

        g_water_draw.f0c = sq.w;
        g_water_draw.sprite = (Spr*)g_wb_ilf->sprites[i & 0xff];
        g_water_draw.f04 = g_wb_ilf->dx[i & 0xff] >> 1;
        g_water_draw.f08 = g_wb_ilf->dy[i & 0xff] >> 1;
        g_water_draw.f10 = 0;
        g_water_draw.sprite->flags |= 0x2000;
        SetOverrideFrame(rec->timer);
    }
    return &g_water_draw;
}

/* +0xb0: draw every person the class has parked on this square in 3D. */
// FUNCTION: LEGOLAND 0x004181a0
void WaterBlock_Interact(RideElem* elem, int x, int y, BPosW* sq,
                         void* clip, int mode)
{
    RiderNode* r = elem->data->riders;

    while (r) {
        if (sq->w == r->key.w)
            IP_RenderBlokeIn3DNow(r->bloke);
        r = r->next;
    }
}

/* The arming pass the pool's tick runs first: any pool square with a person
 * standing on it that is not already splashing starts a splash and plays
 * g_ww_fx[0]. */
// FUNCTION: LEGOLAND 0x00417f40
void WaterBlock_ArmSplashes(void)
{
    WaterRec* p = g_waterblock_head;

    while (p) {
        BPos* sq = &p->sq.b;

        if (WW_AnyBlokeOnSquare(sq) && p->state == 0) {
            p->state = 1;
            p->timer = 0;
            WW_PlaySfx(sq, 0, 0);
        }
        p = p->next;
    }
}

/* The tail every walking state shares (identical to westtown.c's
 * ShopStepToTarget): run the move line to the target the state just wrote,
 * mark the person busy for 7 ticks, stash the raw direction byte, turn to
 * face it, and advance to the next state. */
static __inline void WaterStepToTarget(Bloke* b)
{
    unsigned char d;

    d = (unsigned char)(CalcMoveLine(b->wx, b->wy, b->tx, b->ty, &b->path) + 0x10);
    b->busy = 7;
    b->heading = d;
    NewDirForAction(b, (unsigned char)((d >> 5) + 3));
    b->action++;
}

/* ==========================================================================
 * WATER BLOCK cb_a8 -- the pool's per-tick entry, TWO independent walks.
 *
 * (1) The SPLASH ANIMATION over the placement records.  A record in state 1
 *     advances one frame a tick until it runs off the end of its image's LLS
 *     (frame count at LLS+0x10) and then goes idle; a record in any other
 *     state has its frame counter parked at 0.  Both arms re-resolve the
 *     image out of the class image list every tick and complain to the debug
 *     log if the index or the sprite is bad -- "Bad Block Index (%d)" when
 *     the placement's rolled index has no sprite, "Bad Sprite in Waterworks
 *     block" when the sprite has no LLS.
 *
 * (2) The SWIMMER SCRIPT over the class rider list, which is only two states:
 *       0  walk to the middle of the pool square (x<<8 - 0x80, y<<8 + 0x80),
 *          flagged "inside a building" (flags62 bit 3)
 *       1  leave: RemoveBlokeFromRide and clear the flag
 *     The waypoint is the RIDER's key square plus the class base offset, so
 *     every pool square a visitor steps into pulls them to its centre.
 * ========================================================================== */

/* 119 of 131 instructions exact.  The residual is ONE scheduling block: in
 * case 0 the original completes BOTH target stores before evaluating the move
 * call's arguments, so `b->ty` forwards out of the register that just stored
 * it and `b->tx` has to be RELOADED (VC6 forwards only the most recent
 * store); we sink the `b->ty` store past the argument pushes and forward both.
 * Measured and rejected: all four orders of the two stores against the flags
 * OR, computing the two coordinates into locals first, doing the shifts and
 * the +-0x80 as separate statements, and routing the two stores through an
 * inlined two-int helper. */
// WIP-FUNCTION: LEGOLAND 0x00417f90  (131/131 instructions and 92% of the body index-for-index; audit mismatch 35: case 0's target-store/argument-push schedule -- see above)
void WaterBlock_Activate(RideElem* elem)
{
    WaterRec*  p;
    WWDef*     def;
    RiderNode* r;
    RiderNode* next;
    Bloke*     b;

    WaterBlock_ArmSplashes();
    p = g_waterblock_head;
    while (p) {
        if (p->state == 1) {
            int          n = 0;
            unsigned int f;
            void*        s;
            LLS*         lls;

            p->timer++;
            f = p->frame;
            s = g_wb_ilf->sprites[f & 0xff];
            if (s) {
                lls = GetLLSForSprite(s);
                if (lls)
                    n = lls->frames;
                else
                    DBPrintf(g_s_bad_sprite);
            } else {
                DBPrintf(g_s_bad_block, f);
            }
            if (p->timer >= n) {
                p->state = 0;
                p->timer = 0;
            }
        } else {
            void* s = g_wb_ilf->sprites[p->frame];

            if (GetLLSForSprite(s))
                p->timer = 0;
            else
                DBPrintf(g_s_bad_sprite);
        }
        p = p->next;
    }

    def = elem->data;
    r = def->riders;
    while (r) {
        int x;
        int y;

        b = r->bloke;
        next = r->next;
        x = def->base_x + r->key.b.x;
        y = def->base_y + r->key.b.y;
        if (b->busy == 0) {
            switch (b->action) {
            case 0:
                y <<= 8;
                x <<= 8;
                x -= 0x80;
                b->flags62 |= 8;
                y += 0x80;
                b->tx = x;
                b->ty = y;
                WaterStepToTarget(b);
                break;
            case 1:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8u;
                break;
            }
        }
        r = next;
    }
}

/* ==========================================================================
 * WATER WORKS SHOWER
 *
 * A three-state cycle rather than the pool's one-shot splash:
 *   1  0x0b ticks of "warming up" (no sprite yet)
 *   2  play the shower animation once; when it runs out, restart the last
 *      0x0b ticks of it if the person is STILL under the shower, otherwise
 *      drop to state 3
 *   3  0x2c - 0x24 = 8 ticks of run-off, then idle
 * State 2 is the one the draw slot and the overlay draw both special-case.
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00418260
void Shower_NewRecord(BPosW sq)
{
    WaterRec* rec = (WaterRec*)MemAlloc(sizeof(WaterRec));

    if (rec) {
        memset(rec, 0, sizeof(WaterRec));
        rec->sq = sq;
        WW_ListPush(&g_shower_head, rec);
    }
}

// FUNCTION: LEGOLAND 0x004182a0
void Shower_FreeRecord(WaterRec* rec)
{
    WW_ListUnlink(&g_shower_head, rec);
    MemFree(rec);
}

// FUNCTION: LEGOLAND 0x004182c0
WaterRec* Shower_FindRecord(BPosW* sq)
{
    return WW_ListFind(g_shower_head, sq);
}

// FUNCTION: LEGOLAND 0x004182e0
void Shower_Create(RideElem* elem)
{
    WW_LoadFX();
    g_shower_def = elem->data;
    g_shower_def->flags |= 0x420;
    g_shower_layers = g_shower_def->layers;
    g_shower_layers->flags |= 0x2000;
    g_shower_sprite = LoadSprite(g_s_shower_lls, 1);
}

// FUNCTION: LEGOLAND 0x00418330
void Shower_Destroy(void)
{
    if (g_shower_sprite)
        KillSprite(g_shower_sprite);
    WW_KillFX();
}

// FUNCTION: LEGOLAND 0x00418350
void Shower_ArmShowers(void)
{
    WaterRec* p = g_shower_head;

    while (p) {
        BPos* sq = &p->sq.b;

        if (WW_AnyBlokeOnSquare(sq) && p->state == 0) {
            p->state = 1;
            p->timer = 0;
            WW_PlaySfx(sq, 0, 0);
        }
        p = p->next;
    }
}

// FUNCTION: LEGOLAND 0x004183a0
void Shower_Activate(void)
{
    WaterRec* p;

    Shower_ArmShowers();
    p = g_shower_head;
    while (p) {
        if (p->state == 1) {
            if (++p->timer >= 0x0b) {
                p->state = 2;
                p->timer = 0;
            }
        }
        if (p->state == 2) {
            int  n = 0;
            LLS* lls;

            p->timer++;
            lls = GetLLSForSprite(g_shower_sprite);
            if (lls)
                n = lls->frames;
            if (p->timer >= n) {
                if (WW_AnyBlokeOnSquare(&p->sq.b)) {
                    p->state = 2;
                    p->timer = 0x0b;
                } else {
                    p->state = 3;
                    p->timer = 0x24;
                }
            }
        }
        if (p->state == 3) {
            if (++p->timer >= 0x2c) {
                p->state = 0;
                p->timer = 0;
            }
        }
        p = p->next;
    }
}

/* +0xb0: the shower's spray, drawn over the object only while it is running
 * (state 2), offset (-8, +0x14) in the current view mode. */
// FUNCTION: LEGOLAND 0x00418450
void Shower_Interact(RideElem* elem, int x, int y, BPosW* sq, void* clip, int mode)
{
    WWDef*    def = elem->data;
    WaterRec* rec = Shower_FindRecord(sq);

    if (rec && rec->state == 2) {
        Pos  o;
        Pos  at;
        LLS* lls;

        o.x = -8;
        o.y = 0x14;
        AdjustOffsetForViewMode(&o);
        at = GetScreenCoordsForObject(sq, def);
        lls = GetLLSForSprite(g_shower_sprite);
        LLSSetFrame(lls, rec->timer);
        PrintSprite(g_shower_sprite, at.x + o.x, at.y + o.y, mode, 0);
    }
}

// FUNCTION: LEGOLAND 0x004184e0
void Shower_Add(WWMapObj* obj, Pos* pos)
{
    BPosW sq;

    sq.b.x = (unsigned char)pos->x;
    sq.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    Shower_NewRecord(sq);
}

// FUNCTION: LEGOLAND 0x00418510
void Shower_Remove(WWMapObj* obj, BPosW sq, void* c)
{
    WaterRec* rec;

    StandardRemoveObject(obj, sq, c);
    rec = Shower_FindRecord(&sq);
    if (rec)
        Shower_FreeRecord(rec);
}

/* +0xa0: the shower draws from its own class sprite, but while it is running
 * the render override is pinned to frame 11 -- the last frame of the warm-up
 * -- so the body stays still and only the +0xb0 spray animates. */
// FUNCTION: LEGOLAND 0x00418540
DrawDesc* Shower_Draw(RideElem* elem, BPosW sq)
{
    WWDef*    def = elem->data;
    WaterRec* rec;

    g_water_draw.f0c = sq.w;
    g_water_draw.sprite = def->layers;
    g_water_draw.f04 = def->f14;
    g_water_draw.f08 = def->f18;
    g_water_draw.f10 = 0;
    rec = Shower_FindRecord(&sq);
    if (rec) {
        if (rec->state == 2) {
            SetOverrideFrame(11);
            return &g_water_draw;
        }
        SetOverrideFrame(rec->timer);
    }
    return &g_water_draw;
}

// FUNCTION: LEGOLAND 0x004185c0
void Shower_Update(RideElem* elem, int screen, int mode)
{
    WWDef* def = elem->data;

    ScreenToMapRef(screen, &g_mapref, mode);
    g_edit_footprint = def->rect;
    g_8003f0 = 0;
    ValidateCursor(&g_edit_cursor, def);
    if (!WW_HasEntrance()) {
        SetCursorError(&g_edit_cursor, 12);
        PropagateCursorStatus(&g_edit_cursor);
    }
}

/* ==========================================================================
 * WATER WORKS ELEPHANT FOUNTAIN
 *
 * The only class whose trigger box is NOT its own square: the squirt lands in
 * front of the elephant, so the people test is a 4x4 tile box anchored at
 * (square.x + rect.left, square.y + rect.bottom) of the CLASS footprint.
 * The squirt itself is not part of the object's sprite -- the tick draws it
 * directly with SortSprite, offset (-0x75, +0x8a) in the current view mode
 * and sorted 0x20 below its own foot, so it passes in front of the elephant.
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00418630
void ElephantF_NewRecord(BPosW sq)
{
    WaterRec* rec = (WaterRec*)MemAlloc(sizeof(WaterRec));

    if (rec) {
        memset(rec, 0, sizeof(WaterRec));
        rec->sq = sq;
        WW_ListPush(&g_elephantf_head, rec);
    }
}

// FUNCTION: LEGOLAND 0x00418670
void ElephantF_FreeRecord(WaterRec* rec)
{
    WW_ListUnlink(&g_elephantf_head, rec);
    MemFree(rec);
}

// FUNCTION: LEGOLAND 0x00418690
WaterRec* ElephantF_FindRecord(BPosW* sq)
{
    return WW_ListFind(g_elephantf_head, sq);
}

/* 0x20 only: unlike the pool and the shower this class does NOT set 0x400,
 * so it has no draw-descriptor override and draws from its own sprite. */
// FUNCTION: LEGOLAND 0x004186b0
void ElephantFountain_Create(RideElem* elem)
{
    WW_LoadFX();
    g_elephantf_def = elem->data;
    g_elephantf_def->flags |= 0x20;
    g_elephantf_layers = g_elephantf_def->layers;
    g_elephantf_sprite = LoadSprite(g_s_elsquirt_lls, 1);
}

// FUNCTION: LEGOLAND 0x004186f0
void ElephantFountain_Destroy(void)
{
    if (g_elephantf_sprite)
        KillSprite(g_elephantf_sprite);
    WW_KillFX();
}

/* 52/52 instructions, 12 mismatches: a one-step register ROTATION across the
 * three probes.  The original materialises `&r` into the (now dead) base
 * pointer's register as soon as it frees up and loads the list head after it,
 * giving (head=eax,&r=edx) -> (edx,ecx) -> (ecx,eax); we fill that same slot
 * with the head load instead and get the same rotation started one step
 * along, (ecx,eax) -> (eax,edx) -> (edx,ecx).  The identical body without the
 * `esi` push (WW_AnyBlokeOnSquare) matches exactly, so this is purely the
 * extra live value.  Measured and rejected: both operand orders of each
 * addition, two temporaries for the two coordinates, computing top before
 * right, and hoisting `&r` into a pointer local. */
// WIP-FUNCTION: LEGOLAND 0x00418710  (52/52 instructions, 12 mismatches: probe register rotation -- see above)
int WW_AnyBlokeInFountainBox(BPos* sq, Rect* base)
{
    WinRect r;

    r.left = sq->x + base->left;
    r.right = r.left + 3;
    r.top = sq->y + base->bottom;
    r.bottom = r.top + 3;
    if (WW_AnyBlokeInRect(g_people_head, &r))
        return 1;
    if (WW_AnyBlokeInRect(g_gardener_list, &r))
        return 1;
    return WW_AnyBlokeInRect(g_handyman_list, &r) != 0;
}

/* No sound here: the elephant's own splash is the looped g_ww_fx[2] the
 * placement started, so arming only starts the animation. */
// FUNCTION: LEGOLAND 0x004187b0
void ElephantF_ArmSquirts(void)
{
    WaterRec* p = g_elephantf_head;

    while (p) {
        if (WW_AnyBlokeInFountainBox(&p->sq.b, &g_elephantf_def->rect) &&
            p->state == 0) {
            p->state = 1;
            p->timer = 0;
        }
        p = p->next;
    }
}

// FUNCTION: LEGOLAND 0x004187f0
void ElephantFountain_Activate(void)
{
    Pos       o;
    WaterRec* p;

    ElephantF_ArmSquirts();
    p = g_elephantf_head;
    while (p) {
        if (p->state == 1) {
            int  n = 0;
            LLS* lls = GetLLSForSprite(g_elephantf_sprite);

            if (lls)
                n = lls->frames;
            if (++p->timer >= n) {
                p->state = 0;
                p->timer = 0;
            }
            if (p->state == 1) {
                Pos at;

                o.x = -0x75;
                o.y = 0x8a;
                AdjustOffsetForViewMode(&o);
                at = GetScreenCoordsForObject(&p->sq, g_elephantf_def);
                SetOverrideFrame(p->timer);
                SortSprite(g_elephantf_sprite, at.x + o.x, o.y + at.y,
                           o.y + at.y + 0x20, 0, 0);
                ClearOverrideFrame();
            }
        }
        p = p->next;
    }
}

/* The elephant fountain has no overlay of its own: its +0xb0 slot is empty. */
// FUNCTION: LEGOLAND 0x004188c0
void ElephantFountain_Interact(RideElem* elem, int x, int y, BPosW* sq,
                               void* clip, int mode)
{
}

// FUNCTION: LEGOLAND 0x004188d0
void ElephantFountain_Add(WWMapObj* obj, Pos* pos)
{
    BPosW sq;

    sq.b.x = (unsigned char)pos->x;
    sq.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    ElephantF_NewRecord(sq);
    WW_PlaySfx(&sq.b, 2, 1);
}

// FUNCTION: LEGOLAND 0x00418910
void ElephantFountain_Remove(WWMapObj* obj, BPosW sq, void* c)
{
    WaterRec* rec;

    StandardRemoveObject(obj, sq, c);
    rec = ElephantF_FindRecord(&sq);
    if (rec)
        ElephantF_FreeRecord(rec);
    WW_FadeSfx(&sq.b);
}

// FUNCTION: LEGOLAND 0x00418950
void ElephantFountain_Update(RideElem* elem, int screen, int mode)
{
    WWDef* def = elem->data;

    g_edit_footprint = def->rect;
    ScreenToMapRef(screen, &g_mapref, mode);
    g_8003f0 = 0;
    ValidateCursor(&g_edit_cursor, def);
    if (!WW_HasEntrance()) {
        SetCursorError(&g_edit_cursor, 12);
        PropagateCursorStatus(&g_edit_cursor);
    }
}

/* ==========================================================================
 * WATER WORKS CROCODILE FOUNTAIN -- the thinnest class in the game
 *
 * Three slots and no create handler, so it loads NOTHING of its own: the
 * looped "Fountain01.wav" it starts on placement is g_ww_fx[2], which only
 * exists because some other water works class bumped the shared reference
 * count.  Drop a crocodile fountain into a park that has no other water works
 * piece and it is silent -- an original consequence of the shared table, not
 * a transcription slip.
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x004189c0
void CrocodileFountain_Add(WWMapObj* obj, Pos* pos)
{
    SoundSource src;

    AddBasicObject(obj, pos);
    src.x = pos->x;
    src.kind = 2;
    src.y = pos->y;
    PlayInstanceOfSample(g_ww_fx[2].sample, 1, 1, &src);
}

// FUNCTION: LEGOLAND 0x00418a10
void CrocodileFountain_Remove(WWMapObj* obj, BPosW sq, void* c)
{
    RemoveSoundObject(obj, sq, c);
}

// FUNCTION: LEGOLAND 0x00418a30
void CrocodileFountain_Update(RideElem* elem, int screen, int mode)
{
    WWDef* def = elem->data;

    g_edit_footprint = def->rect;
    g_8003f0 = 0;
    ScreenToMapRef(screen, &g_mapref, mode);
    ValidateCursor(&g_edit_cursor, def);
    if (!WW_HasEntrance()) {
        SetCursorError(&g_edit_cursor, 12);
        PropagateCursorStatus(&g_edit_cursor);
    }
}

/* ==========================================================================
 * THE GARDEN -- HEDGE and FLOWERS
 *
 * Neither class owns a record list.  A placement's entire state is the map
 * cell's USER FLAGS byte, used as an index into the class's own .ILF image
 * list, and the +0xa0 draw slot turns that index back into a sprite plus the
 * halved draw offsets.  Both classes set ObjDef flags 0x404 -- 0x400 "ask the
 * class for its draw descriptor" plus 0x004 -- and neither takes part in the
 * render sort (0x20), because scenery never has people inside it.
 *
 * THE HEDGE INDEX IS A NEIGHBOUR MASK.  Hedge_Restitch probes the four
 * orthogonal cells in the order N(1), E(2), S(4), W(8) and writes mask - 1 as
 * the user flag, or 0x0e when there are no hedge neighbours at all.  So the
 * image table is indexed 0..14 by "which sides join", and the two extremes
 * COLLIDE: an isolated hedge (mask 0 -> 0x0e) draws with the same image as one
 * joined on all four sides (mask 15 -> 0x0e).  Reproduced as written.
 *
 * THE FLOWERS INDEX IS A DIE ROLL.  Flowers_Add picks rand() % count once and
 * never touches it again -- which is exactly why FLOWERS has no remove
 * handler: nothing outside the cell depends on it, so taking one off the map
 * is the engine's default teardown.  The hedge cannot do that, because every
 * neighbour's image depends on it; Hedge_Remove has to re-stitch all four.
 *
 * BOTH ARE INSTALLED BY A CASE-SENSITIVE COMPARE.  Garden_GetInterfaces
 * (interfaces.c 0x004329c0) is the one provider in the game that uses plain
 * strcmp instead of the _stricmp at 0x004aab90.  An .ODF that spelled either
 * class in any other case would still load and still place, but with none of
 * these callbacks: no create, so g_hedge_ilf / g_flowers_ilf stay null and
 * nothing ever indexes an image; no draw override, so the render walk would
 * fall back to the class's own sprite; and for the hedge no add/remove, so no
 * user-flag index would ever be written and neighbouring hedges would never
 * re-stitch.  The other fourteen providers are case insensitive and immune.
 * ========================================================================== */

extern WWDef*   g_hedge_def;        /* 0x0081cd08 */
extern void*    g_hedge_ilf_elem;   /* 0x0061615c  "HEDGE IMAGES" */
extern IlfData* g_hedge_ilf;        /* 0x0061614c */

extern WWDef*   g_flowers_def;      /* 0x0081cd04 */
extern void*    g_flowers_ilf_elem; /* 0x00616150  "FLOWERS 1" */
extern IlfData* g_flowers_ilf;      /* 0x00616158 */

/* The +0xa0 descriptor block the garden hands back.  It is the SAME storage
 * as westtown.c's g_shop_draw -- one shared block for the whole render walk --
 * and the garden fills only the first three fields, leaving whatever square
 * and flag the previous class wrote in +0x0c/+0x10.  Reproduced. */
extern DrawDesc g_garden_draw;      /* 0x0082c6a0 (= westtown.c g_shop_draw) */

extern char g_s_hedge_images[];     /* 0x004b7114 "HEDGE IMAGES" */
extern char g_s_flowers_1[];        /* 0x004b7124 "FLOWERS 1" */

// FUNCTION: LEGOLAND 0x00432480
void Hedge_Create(RideElem* elem)
{
    g_hedge_def = elem->data;
    g_hedge_def->flags |= 0x404;
    if (LLIDB_FindElement(g_s_hedge_images, &g_hedge_ilf_elem, 0) == 0)
        g_hedge_ilf = (IlfData*)LLIDB_LoadData(g_hedge_ilf_elem);
}

// FUNCTION: LEGOLAND 0x004324c0
void Hedge_Destroy(void)
{
    LLIDB_UnLoadData(g_hedge_ilf_elem);
}

// FUNCTION: LEGOLAND 0x004324d0
void Hedge_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_hedge_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint((Footprint*)&g_edit_object->rect);
}

/* Re-derive one hedge cell's image index from its four orthogonal
 * neighbours.  N=1, E=2, S=4, W=8; the index written is mask-1, and a hedge
 * with NO hedge neighbours gets 0x0e -- the same index as one joined on all
 * four sides.  An original collision, not a transcription slip. */
// FUNCTION: LEGOLAND 0x00432510
void Hedge_Restitch(int x, int y)
{
    Pos t;
    int mask = 0;

    t.x = x;
    t.y = y - 1;
    if (GetObjectClassAndInstance(&t, 0) == g_hedge_def)
        mask = 1;
    t.x = x + 1;
    t.y = y;
    if (GetObjectClassAndInstance(&t, 0) == g_hedge_def)
        mask |= 2;
    t.x = x;
    t.y = y + 1;
    if (GetObjectClassAndInstance(&t, 0) == g_hedge_def)
        mask |= 4;
    t.x = x - 1;
    t.y = y;
    if (GetObjectClassAndInstance(&t, 0) == g_hedge_def)
        mask |= 8;
    if (mask == 0)
        Set_UserFlags(x << 8, y << 8, 0x0e);
    else
        Set_UserFlags(x << 8, y << 8, (unsigned short)(mask - 1));
}

// FUNCTION: LEGOLAND 0x004325e0
void Hedge_Add(WWMapObj* obj, Pos* pos)
{
    BPos bp;
    Pos  t;

    bp.x = (unsigned char)pos->x;
    bp.y = (unsigned char)pos->y;
    AddObjectToMap(obj, bp, 0);
    Hedge_Restitch(pos->x, pos->y);
    t.x = pos->x;
    t.y = pos->y - 1;
    if (GetObjectClassAndInstance(&t, 0) == g_hedge_def)
        Hedge_Restitch(t.x, t.y);
    t.x = pos->x + 1;
    t.y = pos->y;
    if (GetObjectClassAndInstance(&t, 0) == g_hedge_def)
        Hedge_Restitch(t.x, t.y);
    t.x = pos->x;
    t.y = pos->y + 1;
    if (GetObjectClassAndInstance(&t, 0) == g_hedge_def)
        Hedge_Restitch(t.x, t.y);
    t.x = pos->x - 1;
    t.y = pos->y;
    if (GetObjectClassAndInstance(&t, 0) == g_hedge_def)
        Hedge_Restitch(t.x, t.y);
}

// FUNCTION: LEGOLAND 0x00432700
void Hedge_Remove(WWMapObj* obj, BPosW bp, void* c)
{
    Pos t;
    int x = bp.b.x;
    int y = bp.b.y;

    StandardRemoveObject(obj, bp, c);
    t.x = x;
    t.y = y - 1;
    if (GetObjectClassAndInstance(&t, 0) == g_hedge_def)
        Hedge_Restitch(t.x, t.y);
    t.x = x + 1;
    t.y = y;
    if (GetObjectClassAndInstance(&t, 0) == g_hedge_def)
        Hedge_Restitch(t.x, t.y);
    t.x = x;
    t.y = y + 1;
    if (GetObjectClassAndInstance(&t, 0) == g_hedge_def)
        Hedge_Restitch(t.x, t.y);
    t.x = x - 1;
    t.y = y;
    if (GetObjectClassAndInstance(&t, 0) == g_hedge_def)
        Hedge_Restitch(t.x, t.y);
}

// FUNCTION: LEGOLAND 0x00432810
DrawDesc* Hedge_Draw(WWMapObj* obj, BPosW bp)
{
    unsigned int i = Get_UserFlags(bp.b.x << 8, bp.b.y << 8);

    g_garden_draw.sprite = (Spr*)g_hedge_ilf->sprites[i & 0xff];
    g_garden_draw.f04 = g_hedge_ilf->dx[i & 0xff] >> 1;
    g_garden_draw.f08 = g_hedge_ilf->dy[i & 0xff] >> 1;
    return &g_garden_draw;
}

// FUNCTION: LEGOLAND 0x00432870
void Flowers_Create(RideElem* elem)
{
    g_flowers_def = elem->data;
    g_flowers_def->flags |= 0x404;
    if (LLIDB_FindElement(g_s_flowers_1, &g_flowers_ilf_elem, 0) == 0)
        g_flowers_ilf = (IlfData*)LLIDB_LoadData(g_flowers_ilf_elem);
}

// FUNCTION: LEGOLAND 0x004328b0
void Flowers_Destroy(void)
{
    LLIDB_UnLoadData(g_flowers_ilf_elem);
}

// FUNCTION: LEGOLAND 0x004328c0
void Flowers_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_flowers_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint((Footprint*)&g_edit_object->rect);
}

// FUNCTION: LEGOLAND 0x00432900
void Flowers_Add(WWMapObj* obj, Pos* pos)
{
    BPos bp;

    bp.x = (unsigned char)pos->x;
    bp.y = (unsigned char)pos->y;
    AddObjectToMap(obj, bp, 0);
    if (g_flowers_ilf)
        Set_UserFlags(pos->x << 8, pos->y << 8,
                      (unsigned short)(rand() % g_flowers_ilf->count));
}

// FUNCTION: LEGOLAND 0x00432960
DrawDesc* Flowers_Draw(WWMapObj* obj, BPosW bp)
{
    unsigned int i = Get_UserFlags(bp.b.x << 8, bp.b.y << 8);

    g_garden_draw.sprite = (Spr*)g_flowers_ilf->sprites[i & 0xff];
    g_garden_draw.f04 = g_flowers_ilf->dx[i & 0xff] >> 1;
    g_garden_draw.f08 = g_flowers_ilf->dy[i & 0xff] >> 1;
    return &g_garden_draw;
}
