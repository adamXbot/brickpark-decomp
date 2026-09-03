/* LEGOLAND -- ride callback cluster at 0x0042xxxx.
 *
 * These are per-class handlers that SetCustomCallbacks (screen.c 0x00452c20)
 * installs into the 0xd0-byte ObjDef callback slots.  They are NOT exported,
 * so every extent below was taken from the disassembly by control flow
 * (tools/audit.py).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are mostly defined LOCALLY on purpose (legoland.h is
 * owned elsewhere).
 *
 * ------------------------------------------------------------------------
 * WHICH RIDE?  -- this block is NOT one ride.
 *
 * The lane was described as "a contiguous block of ride callbacks", but read
 * back out of SetCustomCallbacks the nine addresses are the cb_a8 / cb_b0
 * PAIR of six different classes.  The 0x0042xxxx text is laid out
 * class-by-class and these two slots sit next to each other in every class's
 * block, so the cluster is by SLOT, not by ride:
 *
 *   addr        class              slot   what it really does        state
 *   0x0042aa90  BALLOONZ           cb_a8  per-rider state machine    documented
 *   0x0042b2e0  BALLOONZ           cb_b0  depth-sorted overlay draw  WIP (333)
 *   0x0042bcf0  CAROUSEL           cb_b0  depth-sorted overlay draw  WIP (24)
 *   0x0042c820  CAROUSEL           cb_a8  per-rider state machine    documented
 *   0x0042d610  EARTH SLIDE RIDE   cb_a8  per-rider state machine    100%
 *   0x0042d9c0  ENTRANCE 1         cb_b0  depth-sorted overlay draw  100%
 *   0x0042f1a0  RESTAURANT 1       cb_a8  per-rider state machine    100%
 *   0x0042f4c0  RESTAURANT 1       cb_b0  depth-sorted overlay draw  100%
 *   0x0042fbb0  RESTAURANT 2       cb_a8  per-rider state machine    documented
 *
 * So cb_a8 is NOT "activate": it is the class's PER-TICK STATE MACHINE, run
 * over the whole class's rider list, and cb_b0 is NOT "interact": it is the
 * class's DEPTH-SORTED OVERLAY DRAW.  Both names in screen.c were guesses.
 *
 * ------------------------------------------------------------------------
 * WHAT cb_b0 IS (recovered here; screen.c guessed "interact")
 *
 * cb_b0 is the class's DEPTH-SORTED OVERLAY DRAW.  Its one call site is the
 * render-list walker at 0x00485ac3:
 *
 *     mov  eax,[esi+0x14]              ; the RideElem of this render item
 *     mov  edx,[eax+0x0c]              ; -> its ObjDef
 *     push [esi+0x38]                  ; 6: blit mode
 *     push ebp                         ; 5: clip rect (already installed)
 *     lea  ecx,[esi+0x18] / push ecx   ; 4: the item's map square {u8 x,u8 y}
 *     push [esi+0x24]                  ; 3: screen y
 *     push [esi+0x20]                  ; 2: screen x
 *     push eax                         ; 1: the RideElem
 *     call [edx+0xb0] / add esp,0x18
 *
 * and it is only reached when the render item's context block is tagged
 * 0x103 (`cmp dword [esi+0x10],0x103`) -- the same tag cb_b0 stamps into the
 * context block it builds for its own PrintSprite call.  Argument 5 (the
 * clip rect) is dead in every handler in this file: the walker has already
 * installed it with SetClipping.
 *
 * The body is always the same three-part shape:
 *
 *   1. Collect, into a small stack array, every RiderNode of the class whose
 *      ride_id equals this map square's packed {x,y} -- i.e. the people
 *      currently AT this instance of the building.
 *   2. For depth band 1..N: render every collected person whose bloke byte
 *      +0x37 equals the band with IP_RenderBlokeIn3DNow, then PrintSprite the
 *      band's MASK sprite over them.  The masks are loaded by the class's
 *      cb_a4 (e.g. RestMaskLevel1.lls / RestMaskLevel2.lls ...), so the
 *      building's geometry occludes the people standing behind it.  Byte
 *      +0x37 of a bloke is therefore its OCCLUSION BAND inside the building,
 *      1 = furthest back.
 *   3. Advance the building's own animated layer one frame (wrapping at 15)
 *      and PrintSprite it at the object's screen position, passing the
 *      0x103-tagged context block so the blitter can re-enter the render
 *      list.
 *
 * The per-instance animation state lives in a class-private singly linked
 * list keyed by the map square: {next @ +0x00, u16 square @ +0x04, ...,
 * u8 frame @ +0x09}.  RESTAURANT 1's list head is 0x00616144 and the lookup
 * is 0x0042ef40.
 *
 * ------------------------------------------------------------------------
 * THE cb_a8 SHAPE (all five of them)
 *
 *     item = elem->data;
 *     for (r = item->riders; r; r = next) {
 *         next = r->next;  b = r->bloke;  key = &r->ride_id;
 *         rec = <Class>_FindRec(key);      // per-square record, by u16 @ +4
 *         if (!rec) return;                // aborts the WHOLE walk, not just
 *                                          // this rider (an original quirk)
 *         <copy the record's mutable state into locals>
 *         tx = item->base_x + key->bx;  ty = item->base_y + key->by;
 *         if (b->state == 0)               // low-level AI idle
 *             switch (b->action) { ... }   // one step, action++ to advance
 *         <write the locals back into the record>
 *     }
 *
 * The record is copied into locals field by field on entry and written back
 * at the end of every iteration -- that is why these functions have such big
 * frames.  The state machines all end with RemoveBlokeFromRide + clearing
 * flags62 bit 3 ("using this ride").
 *
 * ------------------------------------------------------------------------
 * THE THREE cb_a8 HANDLERS THIS PASS DOCUMENTED BUT DID NOT RECONSTRUCT
 *
 * 0x0042c820  CAROUSEL, cb_a8 (378 instructions).  Record list 0x006160c4;
 *   record: {next @0, u16 square @4, u8 boarded @6, u8 aboard @7, i8 frame
 *   @8, u8 ride_id @0x18, int timer @0x1c, u8 seat_taken[] @0x1f}.  Its
 *   switch is a TWO-LEVEL table (byte index at 0x0042cd10 -> block table at
 *   0x0042ccec) over actions 0..14, with 3, 4, 6 and 9..12 falling to the
 *   default:
 *     0   claim the ride (flags |= 8), timer = 180, walk to (tx-3, ty+0.5)
 *     1   the heavy one: GetScreenCoordsForObject + GetTileDimensions, then
 *         an isometric world->screen solve using Get_XScroll/Get_YScroll and
 *         the map origin at 0x004bcbf4 (+0x20/+0x22), GetUnitDepth for the
 *         3D person's z, a seat picked by 0x0042cd20 (rand() % seats, then
 *         the first free slot scanning forward from it in rec->seat_taken),
 *         a BNV path name built with sprintf("%02d") and NewBNVPath from
 *         Zbuffers\CarouselOn.bnv, and flags62 |= 0x80 (riding)
 *     2   follow that path; when it ends, free it, f35 = 1, action = 5
 *     5   sit: flags |= 0x80, frame 0, GetUnitDepth again, boarded++, and
 *         when boarded == item->capacity call 0x0042bc90 (start the ride:
 *         aboard = boarded, boarded = 0, +0x0c |= 1 and &= ~0x4000)
 *     7   stand up: walk animation, a second NewBNVPath from
 *         Zbuffers\CarouselOff.bnv using the rider's seat index
 *     8   follow the leaving path; when it ends f35 = 2, dir = 3, action = 13
 *     13  release the seat (rec->seat_taken[b->seat] = 0), clear flags 0x80,
 *         UnAdjustBlokePosition + ScreenToMapRef to land back on the map,
 *         then CalcMoveLine to walk off
 *     14  RemoveBlokeFromRide, clear flags 8, aboard--; on the last rider
 *         boarded = 0 and Ride_ClearFlagToNotLetAnyoneOn
 *   After the whole walk it calls 0x0042c800, which runs 0x0042c6d0 on every
 *   record (the per-instance carousel tick).  The `!rec` early return skips
 *   that call.
 *
 * 0x0042aa90  BALLOONZ, cb_a8 (637 instructions).  Record list 0x00616060;
 *   the state copied in and out is {u32 @8, u8 @0x0c, u32 @0x0d (UNALIGNED),
 *   u16 @0x11, i8 car @0x13, u8 @0x14, i8 wheel @0x15, u8 @0x17, u32 @0x18,
 *   u32 @0x1c}.  16 actions, 0..15.  On entry it also pushes the wheel frame
 *   into the z_Balloon2 sprite's LLS (`*(short*)*(g_bz_zspr->lls) = wheel`),
 *   the same z-buffer trick the carousel uses, and it reads two float
 *   constants from 0x004b64bc/0x004b64c0 into locals up front.
 *   After the rider walk there is a SECOND phase over the record list that
 *   advances each wheel: it recomputes `wheel % 8 == 0 || wheel == 0` (the
 *   same platform-aligned test cb_b0 uses) and calls 0x0042aa60 to decide
 *   which car is at the platform.
 *
 * 0x0042fbb0  RESTAURANT 2, cb_a8 (658 instructions, the largest in the
 *   lane).  Record list 0x00616148, found by 0x0042f9d0.  It copies FIFTEEN
 *   fields out of the record (+0x07, +0x08, +0x0c, +0x10, +0x11, +0x14,
 *   +0x18, +0x1c, +0x20, +0x24, +0x28, +0x2c, +0x30, +0x38, +0x3c) into the
 *   frame and writes them all back, which is why its frame is 0x44 bytes.
 *   16 actions, 0..15; the interesting ones:
 *     0   walk to (x*256 + 0x3c8, (y-2)*256), timer = 300, remember the
 *         bloke's speed (+0x7f) in +0x44 and force it to 0x15
 *     1   walk to a queue slot: (x*256 + 0x3c8, (y-4)*256 + slot*100)
 *     2   only while the queue counter (+0x11) < 3 and +0x24 is non-zero
 *     4   walk in through the door and, once three customers have gone in,
 *         reset the counters (+0x24 = 0, +0x2c = 3)
 *     5   0x004400b0 SetPersonDirection(rider->person, 5)
 *     6   dispatch on +0x30 / +0x34 into 0x0042fa90 (seat the customer:
 *         a per-seat delta table at 0x004b6860 applied to the bloke's world
 *         position)
 *     10  count the meal timer down; at exactly 250 charge with BuyItem
 *     11  leave the table: move to (x*256 - 0xa9c, y*256 + slot*100 - 0xd2c)
 *     12  the "everybody has finished" join: when the last customer leaves,
 *         +0x30 = 2, +0x44 = 0x143 and the counters reset
 *     13,14 walk out through the door
 *     15  RemoveBlokeFromRide + clear flags 8
 *   After the rider walk it runs a SECOND phase over the record list
 *   (0x00430??? onwards) that copies the same fifteen fields out of each
 *   record, ticks the restaurant itself and writes them back.
 *
 * THE RENDER-ITEM CONTEXT BLOCK (tag 0x103), built on the stack and passed
 * as PrintSprite's 5th argument:
 *     +0x00 int            0x103
 *     +0x04 RideElem*      the class element
 *     +0x08 unsigned short the map square, packed {x,y}
 * The walker's copy of the same block lives at render item +0x10..+0x1f.
 * ======================================================================== */

/* An 8-byte {x,y} pair returned in eax:edx (legoland.h's Offset). */
typedef struct Offset {
    int ox;
    int oy;
} Offset;

/* The per-cell layer holder and the render object that owns it
 * (legoland.h's Layers / RenderObj -- defined locally so this file compiles
 * standalone). */
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

/* A per-layer sprite object; its LLS is *(+0x08) (legoland.h's SpriteObj). */
typedef struct SpriteObj {
    int    pad0;
    int    pad4;
    void** lls_holder;   /* +0x08 */
} SpriteObj;

/* ---- the class element and its ObjDef, seen from the ride side ---------- */
typedef struct RideObject RideObject;

typedef struct RideElem {
    char*        name;         /* +0x00 */
    char*        image;        /* +0x04 */
    unsigned int flags;        /* +0x08 */
    RideObject*  data;         /* +0x0c */
} RideElem;

/* An {x,y} pair in 24.8 world units, passed and returned by value. */
typedef struct Pos {
    int x;
    int y;
} Pos;

/* A person as the ride code sees it (bigsim.c's Bloke; only what this file
 * touches is named). */
typedef struct Person3D Person3D;

typedef struct Bloke {
    unsigned char  pad00[4];
    Person3D*      person;     /* +0x04  its 3D model record */
    unsigned char  pad08[0x0e - 0x08];
    unsigned short state;      /* +0x0e  low-level AI state (0 = idle) */
    unsigned char  pad10[0x14];
    Pos            target;     /* +0x24  walk target, 24.8 */
    unsigned char  pad2c[0x36 - 0x2c];
    unsigned char  seat;       /* +0x36  which of the building's 3 seats */
    unsigned char  band;       /* +0x37  occlusion band 1..5 inside a building */
    unsigned char  pad38[4];
    short          f3c;        /* +0x3c  per-rider offset inside a ride */
    short          f3e;        /* +0x3e */
    unsigned char  pad40[0x46 - 0x40];
    unsigned short f46;        /* +0x46  3 = got a seat, 4 = turned away */
    unsigned char  pad48[0x58 - 0x48];
    int            f58;        /* +0x58  ride-specific countdown */
    int            wait;       /* +0x5c  countdown ticks */
    unsigned char  action;     /* +0x60  state-machine step */
    unsigned char  pad61;
    unsigned short flags62;    /* +0x62  8 = using this ride, 0x100 = sitting */
    unsigned char  pad64[4];
    Pos            world;      /* +0x68  world position, 24.8 */
    unsigned short f70;        /* +0x70 */
    unsigned char  dir;        /* +0x72 */
    unsigned char  new_dir;    /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14]; /* +0x98  CalcMoveLine scratch */
} Bloke;

/* A rider slot: doubly linked, its bloke at +0x08 and the packed map square
 * of the ride it is using at +0x0c (rides.c's RiderNode). */
typedef struct RiderPerson RiderPerson;

typedef struct RiderNode {
    struct RiderNode* next;    /* +0x00 */
    struct RiderNode* prev;    /* +0x04 */
    Bloke*            bloke;   /* +0x08 */
    unsigned short    ride_id; /* +0x0c */
    unsigned short    pad0e;
    struct RiderPerson* person;/* +0x10 */
} RiderNode;

struct RideObject {
    unsigned char  pad00[0x0c];
    int            base_x;     /* +0x0c  the class's render/seat base square */
    int            base_y;     /* +0x10 */
    unsigned char  pad14[0x24 - 0x14];
    signed char    qx;         /* +0x24  queue/exit offset from the base square */
    signed char    qy;         /* +0x25 */
    unsigned char  pad26[0x2e - 0x26];
    short          capacity;   /* +0x2e  riders per car */
    unsigned char  pad30[0x40 - 0x30];
    int            world_y0;   /* +0x40  entrance origin, tiles */
    int            world_x0;   /* +0x44 */
    unsigned char  pad48[0x64 - 0x48];
    RenderObj*     layers;     /* +0x64  the class's layer holder */
    unsigned char  pad68[0xc4 - 0x68];
    RideElem*      elem;       /* +0xc4  back-pointer to the class element */
    unsigned char* visit_counts; /* +0xc8 */
    RiderNode*     riders;     /* +0xcc  live rider list of the whole class */
};

/* A placed object's map square, packed as two bytes (math3d.c's MapObject). */
typedef struct MapSquare {
    unsigned char bx;          /* +0x00 */
    unsigned char by;          /* +0x01 */
} MapSquare;

/* The render-item context block PrintSprite takes as its 5th argument. */
typedef struct DrawCtx {
    int            tag;        /* +0x00  0x103 */
    RideElem*      elem;       /* +0x04 */
    unsigned short square;     /* +0x08 */
} DrawCtx;

/* The three seat-occupied flags of one restaurant, copied in and out of the
 * record as a UNIT (a 3-byte struct assignment: word + byte). */
typedef struct Seats {
    unsigned char s[3];        /* 0 = free, 1 = taken */
} Seats;

/* RESTAURANT 1's per-instance record, found by map square. */
typedef struct RestRec {
    struct RestRec* next;      /* +0x00 */
    unsigned short  square;    /* +0x04  packed {x,y} */
    Seats           seats;     /* +0x06..+0x08 */
    char            frame;     /* +0x09  door animation frame, wraps at 15 */
} RestRec;

/* ---- callees ------------------------------------------------------------ */
extern Offset GetScreenCoordsForObject(MapSquare* inst, RideObject* item); /* 0x00442cc0 */
extern void   AdjustOffsetForViewMode(Offset* o);                    /* 0x00442d30 */
extern void   IP_RenderBlokeIn3DNow(Bloke* b);                       /* 0x00440010 */

/* THE BAND WALK, shared by every collect-and-draw callback in this file.
 *
 * THE SPELLING IS THE LEVER (the same one westtown.c's ShopDrawBand and
 * joust.c's Joust_DrawBand carry).  Written out at the call site as
 * `for (i = 0; i < n; i++) if (here[i]->action == K) IP_RenderBlokeIn3DNow(...)`,
 * or as an __inline that copies its list parameter into a local cursor inside
 * the guard, the per-band guard block is EMPTY, so the sign-extension of the
 * count is the block's only loop-invariant value: VC6 hoists ONE `movsx` of it
 * into a callee-saved register, bl falls free, loop-invariant motion then parks
 * the band CONSTANT in bl (`mov bl,6` + `cmp [eax+0x60],bl`), the count in bl is
 * destroyed and every later `test bl,bl` guard goes with it.  Walking the
 * PARAMETER (`++here`) puts the queue-address `lea` in the guard block, which
 * blocks the hoist: the count is re-derived per band with `movsx edi,bl`, the
 * band code stays an immediate, and all the guards come back.  Worth 166
 * mismatches on Balloonz_Draw and 61 on Carousel_Draw; Restaurant1_Draw,
 * Restaurant1_Tick and Entrance1_Draw are unaffected (still exact). */
static __inline void DrawBand(Bloke** here, char n, int code)
{
    int i;

    if (n > 0) {
        i = n;
        do {
            if ((*here)->action == code)
                IP_RenderBlokeIn3DNow(*here);
            ++here;
        } while (--i);
    }
}

extern void   LLSSetFrame(void* lls, int frame);                     /* 0x0047d5a0 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx);/* 0x004853a0 */
extern void*  GetLLSForLayer(RenderObj* obj, int layer);             /* 0x00441ea0 */
extern void*  GetSpriteForLayer(RenderObj* obj, int layer);          /* 0x00441ec0 */
extern Offset GetRenderOffsetForLayer(RenderObj* obj, int layer);    /* 0x00441ee0 */

extern int    CalcMoveLine(Pos from, Pos to, void* path);             /* 0x00480740 */
extern int    NewDirForAction(Bloke* b, unsigned char dir);          /* 0x004833d0 */
extern void   BlokeSitAnim(Bloke* b);                                /* 0x00440780 */
extern void   BlokeSetFrame(Bloke* b, int frame);                    /* 0x00440870 */
extern void   BlokeWalkAnim(Bloke* b);                               /* 0x00440910 */
extern void   RemoveBlokeFromRide(RideObject* item, RiderNode* r);   /* 0x0048a100 */
extern void   BuyItem(RideElem* elem, MapSquare* at, int which);     /* 0x004539e0 */
extern int    rand(void);                                            /* 0x0049e4b2 (CRT) */

/* RESTAURANT 1's per-square animation record lookup (list head 0x00616144). */
extern RestRec* Restaurant1_FindRec(MapSquare* sq);                  /* 0x0042ef40 */

/* Aim a customer at waypoint `phase` of its seat: a table of 6-dword entries
 * at 0x004b66f4 indexed (seat * 5 + phase), giving a tile delta, a sub-tile
 * offset, a "turn to face" flag and the occlusion band to take. */
extern void Restaurant1_WalkToSeatSpot(Bloke*, int, int, int);        /* 0x0042f0f0 */

/* ---- RESTAURANT 1 globals (all filled by its cb_a4, 0x0042f030) --------- */
extern RenderObj* g_rest1_layers;      /* 0x0081cd2c  obj->[0x64] layer holder */
extern void*      g_rest1_mask_main;   /* 0x0081cd28  RestMask_Main.lls */
extern void*      g_rest1_mask_1aa;    /* 0x0081cd8c  RestMaskLevel1aa.lls */
extern void*      g_rest1_mask_1;      /* 0x0081cd88  RestMaskLevel1.lls */
extern void*      g_rest1_mask_2;      /* 0x0081cd94  RestMaskLevel2.lls */
extern void*      g_rest1_mask_3;      /* 0x0081cd90  RestMaskLevel3.lls */

/* =========================================================================
 * 0x0042f4c0 -- RESTAURANT 1, cb_b0.
 *
 * Five occlusion bands: the customers standing in band k are drawn, then
 * mask k is blitted over them.  Band order and mask order are
 *   1 -> RestMaskLevel1aa, 2 -> RestMaskLevel1, 3 -> RestMaskLevel2,
 *   4 -> RestMaskLevel3, 5 -> RestMask_Main
 * i.e. the "Main" mask is LAST, not first -- it is the front wall.
 *
 * At most eight customers are collected; the array is not bounds-checked, so
 * a ninth rider at the same square would write past it (reproduced).
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0042f4c0
void Restaurant1_Draw(RideElem* elem, int x, int y, MapSquare* sq,
                      void* clip, int mode)
{
    RideObject* item = elem->data;
    Offset      off;
    Offset      screen;
    DrawCtx     ctx;
    RiderNode*  r;
    RestRec*    rec;
    int         n;
    char        frame;

    r = item->riders;
    n = 0;
    ctx.tag = 0x103;
    ctx.elem = item->elem;
    ctx.square = *(unsigned short*)sq;

    /* The collection array's zero-init must sit AFTER the three ctx stores:
     * as a function-level `= {0}` VC6 emits it first, ties up eax/ecx and
     * sinks the ctx stores past the pushes.  An inner scope puts it where
     * the original has it. */
    {
    Bloke* here[8] = { 0 };
    Bloke** p;
    int i;

    n = 0;
    screen = GetScreenCoordsForObject(sq, item);

    if (r) {
        p = here;
        do {
            if (*(unsigned short*)sq == r->ride_id) {
                *p = r->bloke;
                n++;
                p++;
            }
            r = r->next;
        } while (r);

        if (n != 0) {
            for (i = 0; i < n; i++)
                if (here[i]->band == 1)
                    IP_RenderBlokeIn3DNow(here[i]);
            PrintSprite(g_rest1_mask_1aa, x, y, mode, 0);

            for (i = 0; i < n; i++)
                if (here[i]->band == 2)
                    IP_RenderBlokeIn3DNow(here[i]);
            PrintSprite(g_rest1_mask_1, x, y, mode, 0);

            for (i = 0; i < n; i++)
                if (here[i]->band == 3)
                    IP_RenderBlokeIn3DNow(here[i]);
            PrintSprite(g_rest1_mask_2, x, y, mode, 0);

            for (i = 0; i < n; i++)
                if (here[i]->band == 4)
                    IP_RenderBlokeIn3DNow(here[i]);
            PrintSprite(g_rest1_mask_3, x, y, mode, 0);

            for (i = 0; i < n; i++)
                if (here[i]->band == 5)
                    IP_RenderBlokeIn3DNow(here[i]);
            PrintSprite(g_rest1_mask_main, x, y, mode, 0);
        }
    }
    }

    rec = Restaurant1_FindRec(sq);
    if (rec) {
        frame = (char)(rec->frame + 1);
        if (frame > 15)
            frame = 0;
        LLSSetFrame(GetLLSForLayer(g_rest1_layers, 1), frame);
        off = GetRenderOffsetForLayer(g_rest1_layers, 1);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(g_rest1_layers, 1),
                    screen.ox + off.ox, screen.oy + off.oy, mode, &ctx);
        rec->frame = frame;
    }
}

/* =========================================================================
 * 0x0042f1a0 -- RESTAURANT 1, cb_a8: the per-customer state machine.
 *
 * Walks the class's whole rider list once per tick.  For each rider it finds
 * the restaurant record of the square that rider is using, copies that
 * record's three seat flags into a local, runs one step of the customer's
 * 11-state machine (only while the bloke's low-level AI is idle, state 0)
 * and writes the seat flags back.
 *
 * The states:
 *   0  walk to the counter (6 tiles left of the base square), then look for
 *      a free seat: with all three free pick one at random, otherwise take
 *      the last free one found.  f46 = 3 and wait = 300 on success, f46 = 4
 *      and wait = 500 when the restaurant is full.
 *   1  pay (BuyItem); if state 0 found no seat (f46 == 4) jump straight to
 *      state 8 (leave), else start walking to the seat.
 *   2,3 further seat waypoints.
 *   4  sit down: flags |= 0x100, sit animation, frame 0.
 *   5  eat: count `wait` down, then advance.
 *   6  stand up: flags &= ~0x100, walk animation, waypoint 3.
 *   7  waypoint 4 and free the seat; action += 2 (skips state 8).
 *   8  wait out the "no seat" delay, then FALL THROUGH into 9.
 *   9  walk one tile below the base square (the exit) and advance.
 *  10  leave the ride: RemoveBlokeFromRide + clear the "using this ride" bit.
 *
 * ORIGINAL BUG, reproduced: `nfree` and `freeidx` are function-level and
 * initialised ONCE, before the rider loop -- not per rider.  The second
 * customer to reach state 0 in the same tick therefore sees the first one's
 * free-seat count added to its own, so it can believe every seat is free
 * (nfree >= 3 takes the random branch) or find seats when none are free.
 *
 * Note also that a rider whose square has no restaurant record aborts the
 * WHOLE walk (a `return`, not a `continue`), so later riders are not ticked
 * at all that frame.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0042f1a0
void Restaurant1_Tick(RideElem* elem)
{
    RideObject*   item = elem->data;
    char          freeidx = 0;
    char          nfree = 0;
    Seats         seats;
    RiderNode*    r;
    RiderNode*    next;
    Seats*        seatp;
    Bloke*        b;
    RestRec*      rec;
    MapSquare*    key;
    int           tx;
    int           ty;
    unsigned char a;
    char          i;
    char          pick;

    r = item->riders;
    while (r) {
        next = r->next;
        key = (MapSquare*)&r->ride_id;
        b = r->bloke;
        rec = Restaurant1_FindRec(key);
        if (!rec)
            return;
        seatp = &rec->seats;
        tx = item->base_x;
        seats = *seatp;
        tx += key->bx;
        ty = key->by + item->base_y;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                tx -= 6;
                b->flags62 |= 8;
                ty <<= 8;
                tx <<= 8;
                b->band = 3;
                b->target.x = tx;
                b->target.y = ty;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                for (i = 0; i < 3; i++) {
                    if (seats.s[i] == 0) {
                        freeidx = i;
                        nfree++;
                    }
                }
                if (nfree != 0) {
                    if (nfree == 3)
                        pick = (char)(rand() % 3);
                    else
                        pick = freeidx;
                    seats.s[pick] = 1;
                    b->seat = pick;
                    b->f46 = 3;
                    b->wait = 300;
                    b->action++;
                } else {
                    b->f46 = 4;
                    b->wait = 500;
                    b->action++;
                }
                break;
            case 1:
                BuyItem(elem, key, 1);
                if (b->f46 == 4) {
                    b->action = 8;
                } else {
                    Restaurant1_WalkToSeatSpot(b, tx, ty, 0);
                    b->action++;
                }
                break;
            case 2:
                Restaurant1_WalkToSeatSpot(b, tx, ty, 1);
                b->action++;
                break;
            case 3:
                Restaurant1_WalkToSeatSpot(b, tx, ty, 2);
                b->action++;
                break;
            case 4:
                b->flags62 |= 0x100;
                b->f70 = 10;
                BlokeSitAnim(b);
                BlokeSetFrame(b, 0);
                b->action++;
                break;
            case 5:
                if (b->wait-- < 0)
                    b->action++;
                break;
            case 6:
                b->flags62 &= ~0x100;
                b->f70 = 0;
                BlokeWalkAnim(b);
                Restaurant1_WalkToSeatSpot(b, tx, ty, 3);
                b->action++;
                break;
            case 7:
                Restaurant1_WalkToSeatSpot(b, tx, ty, 4);
                b->action = (unsigned char)(b->action + 2);
                seats.s[b->seat] = 0;
                break;
            case 8:
                if (b->wait-- < 0)
                    b->action++;
                /* falls through into 9 -- the original has no break here */
            case 9:
                tx <<= 8;
                ty++;
                ty <<= 8;
                b->band = 3;
                b->target.x = tx;
                b->target.y = ty;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 10:
                RemoveBlokeFromRide(item, r);
                b->flags62 &= ~8;
                break;
            }
        }

        *seatp = seats;
        r = next;
    }
}

/* =========================================================================
 * EARTH SLIDE RIDE
 *
 * A second per-square record list (head 0x006160e8, u16 square at +0x00,
 * next at +0x0c) holds one slide instance per placed ride.  Its queue lives
 * at +0x1c as a chain of 8-byte nodes {next, rider}; a car is "away" while
 * bit 0x8000 of +0x10 is set.
 * ========================================================================= */

typedef struct SlideRec {
    unsigned short   square;   /* +0x00  packed {x,y} */
    unsigned char    pad02[2];
    int              f04;      /* +0x04  cleared when the car fills up */
    unsigned char    b08;      /* +0x08  car loaded flag */
    unsigned char    b09;      /* +0x09  riders that have boarded */
    unsigned char    b0a;      /* +0x0a  riders still aboard */
    signed char      b0b;      /* +0x0b  seat/frame index for the 3D riders */
    struct SlideRec* next;     /* +0x0c */
    int              flags;    /* +0x10  0x0001 full, 0x8000 car away */
    int              f14;      /* +0x14 */
    signed char      b18;      /* +0x18  riders currently queued/aboard */
    unsigned char    pad19[3];
    void*            queue;    /* +0x1c  head of the 8-byte queue nodes */
} SlideRec;

/* Tick every placed slide (0x0042d560 per record). */
extern void  EarthSlide_TickInstances(void);                         /* 0x0042d5f0 */
extern SlideRec* EarthSlide_FindRec(MapSquare* sq);                  /* 0x0042ce20 */
/* Where the next queuer should stand, in 24.8 world units. */
extern void  EarthSlide_GetQueueSpot(RideObject*, MapSquare*, Pos*); /* 0x0042cec0 */
/* Append `r` to the slide's queue (marks the bloke with flag 0x40). */
extern void  EarthSlide_JoinQueue(SlideRec* rec, RiderNode* r);      /* 0x0042ce50 */
extern int   EarthSlide_IsFrontOfQueue(SlideRec* rec, Bloke* b);     /* 0x0042cf40 */
/* Pop the front queuer, advance its action and re-shuffle the queue. */
extern void  EarthSlide_LaunchCar(SlideRec* rec);                    /* 0x0042cf70 */
extern void  Put3DBlokesOnRide(RideObject*, void*, int, void*);      /* 0x00441a60 */
extern void  Ride_ClearFlagToNotLetAnyoneOn(SlideRec* rec);          /* 0x00443000 */

extern RideObject* g_slide_item;   /* 0x006160d0  the EARTH SLIDE class object */
extern void*       g_slide_anim;   /* 0x006160e4  its 3D rider animation */

/* =========================================================================
 * 0x0042d610 -- EARTH SLIDE RIDE, cb_a8: the per-rider state machine.
 *
 * Ticks the slide instances first, then walks the class's rider list.  The
 * states, with (tx,ty) the ride's base square and (ex,ey) its exit square:
 *   0  claim a queue spot and walk to it (flags |= 8 = using this ride)
 *   1  wait at the front of the queue; when the car is not away and this
 *      bloke is at the head, launch it, count the boarder (the first one
 *      sets the "car away" bit) and walk to (tx-1.5, ty+3.5)
 *   2  walk to (tx-3, ty+3)
 *   3  walk to (tx-4, ty+3)
 *   4  teleport to (tx-4, ty+3) and wait rand()%32 + 4 ticks
 *   5  count that wait down; the tick it reaches 0 the rider's own action is
 *      bumped through r->bloke (the original re-loads the rider rather than
 *      reusing `b`, so the redundant indirection is reproduced)
 *   6  sit: flags |= 0x80, sit animation, frame 0; count the car's boarders,
 *      and when the last one is in mark the car full and place the 3D riders
 *   (7 has no case: it falls to the default and does nothing)
 *   8  stand up, teleport to the exit square centre, leave the ride and
 *      release the "do not let anyone on" flag once the car has emptied
 * ========================================================================= */

/* Case 0's queue-spot block.  `spot` MUST live here, not in EarthSlide_Tick:
 * see the note below the helper. */
static __inline void EarthSlide_AimAtQueue(RideObject* item, RiderNode* r, SlideRec* rec, Bloke* b)
{
    Pos spot;
    EarthSlide_GetQueueSpot(item, (MapSquare*)&r->ride_id, &spot);
    EarthSlide_JoinQueue(rec, r);
    b->target.x = spot.x;
    b->target.y = spot.y;
}

/* 276/276 instructions, 824/824 bytes -- audit.py [OK].
 *
 * Two levers closed it:
 *
 * 1. INTERLEAVE the two coordinate sums instead of grouping the two key
 *    reads (`kx = key->bx; tx = item->base_x + kx; ky = key->by; ty = ...`):
 *    this makes VC6 put `item` in ebp and `key` in ebx, so `tx` inherits
 *    item's register and `ty` key's, as the original has them (was 15 -> 2).
 *
 * 2. THE LAST PAIR (indices 62/64, `mov eax,[spot.y]` vs `lea ecx,[b+0x98]`)
 *    was an ALIAS-ANALYSIS effect, not a schedule or register choice.  The
 *    original loads BOTH halves of the queue spot before it stores
 *    b->target.x, i.e. VC6 was free to hoist the spot.y load above a store
 *    through `b`.  With `spot` a named, address-taken local of THIS function
 *    (its address goes to EarthSlide_GetQueueSpot), VC6 treats every
 *    pointer store as a possible alias of it, pins the y load after the x
 *    store, and fills the slot with the lea.  Moving the queue-spot block
 *    into a `static __inline` helper (EarthSlide_AimAtQueue) that owns the
 *    `Pos spot` fixes it: an address-taken local born inside an inlined
 *    helper is NOT in the caller's escaped-local class, so the store through
 *    `b` no longer orders against it.  The helper keeps the scalar stores
 *    (x then y); the original's `mov edx,eax` / reload of x from [b+0x24]
 *    for the CalcMoveLine argument copy is what those scalar stores produce.
 *    Frame home of spot (top slot, 0x20) is unchanged by the helper.
 *
 * Ruled out on the way (all measured): `b->target = spot;` (11: VC6 then
 * CSEs the CalcMoveLine argument reads to spot and swaps the exit/spot
 * frame homes); the same in a case-0 block scope (210); a volatile struct
 * copy (2, keeps the frame but pins the y load); memcpy (11, = struct copy);
 * any by-value Pos argument to an inlined setter (217, adds a frame temp);
 * reusing the dead kx/ky or tx/ty locals or block-scoped int temps as the
 * carriers (208 -- VC6 reverses the two stores once both values sit in
 * temps); spot as int[2] or as a distinct anonymous struct type (2, inert);
 * GetQueueSpot's out-parameter typed int* or void* (2, inert); #pragma
 * optimize("w") (87 -- it does hoist the load but also changes case 5) and
 * ("a") (241).  Earlier rounds' findings (two int temporaries, storing y
 * first, __inline setters with (Bloke*, int, int) or (Bloke*, const Pos*)
 * bodies in either order) are consistent with the above. */
// FUNCTION: LEGOLAND 0x0042d610
void EarthSlide_Tick(RideElem* elem)
{
    RideObject*   item = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    SlideRec*     rec;
    MapSquare*    key;
    Pos           exit;
    int           kx;
    int           ky;
    int           tx;
    int           ty;
    unsigned char a;

    r = item->riders;
    EarthSlide_TickInstances();

    while (r) {
        next = r->next;
        key = (MapSquare*)&r->ride_id;
        b = r->bloke;
        rec = EarthSlide_FindRec(key);
        if (!rec)
            return;
        kx = key->bx;
        tx = item->base_x + kx;
        ky = key->by;
        ty = item->base_y + ky;
        exit.x = item->qx + kx;
        exit.y = item->qy + ky;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                EarthSlide_AimAtQueue(item, r, rec, b);
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 1:
                if (rec->flags & 0x8000)
                    break;
                if (!EarthSlide_IsFrontOfQueue(rec, b))
                    break;
                EarthSlide_LaunchCar(rec);
                rec->b09++;
                if (rec->b09 == 1)
                    rec->flags |= 0x8000;
                tx <<= 8;
                ty <<= 8;
                tx -= 0x180;
                ty += 0x380;
                b->target.x = tx;
                b->target.y = ty;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 2:
                tx -= 3;
                ty += 3;
                tx <<= 8;
                ty <<= 8;
                b->target.x = tx;
                b->target.y = ty;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 3:
                tx -= 4;
                ty += 3;
                tx <<= 8;
                ty <<= 8;
                b->target.x = tx;
                b->target.y = ty;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 4:
                tx -= 4;
                ty += 3;
                tx <<= 8;
                ty <<= 8;
                b->world.x = tx;
                b->world.y = ty;
                b->f58 = (rand() & 0x1f) + 4;
                b->action++;
                break;
            case 5:
                if (b->f58 == 0)
                    r->bloke->action++;
                b->f58--;
                break;
            case 6:
                b->flags62 |= 0x80;
                BlokeSitAnim(b);
                BlokeSetFrame(b, 0);
                rec->b08++;
                rec->b09--;
                if (rec->b08 == 1) {
                    rec->b0a = rec->b08;
                    rec->b0b = 0;
                    rec->flags |= 1;
                    rec->f14 = 0;
                    rec->f04 = 0;
                }
                b->f58 = 8;
                b->action++;
                Put3DBlokesOnRide(g_slide_item, rec, rec->b0b, g_slide_anim);
                break;
            case 8:
                BlokeWalkAnim(b);
                b->flags62 &= ~0x80;
                b->world.x = (exit.x << 8) + 0x80;
                b->world.y = (exit.y << 8) + 0x80;
                RemoveBlokeFromRide(item, r);
                b->flags62 &= ~8;
                rec->b0a--;
                if (rec->b0a == 0) {
                    rec->b08 = 0;
                    rec->flags &= ~0x8000;
                    rec->f04 = 1;
                }
                if ((short)rec->b18 != item->capacity)
                    Ride_ClearFlagToNotLetAnyoneOn(rec);
                break;
            }
        }

        r = next;
    }
}

/* =========================================================================
 * ENTRANCE 1
 *
 * The park entrance draws its visitors interleaved with five sprites: four
 * "matte" strips (entrance_matte1..4.lls) that are the turnstile fronts and
 * booth1.lls.  Unlike RESTAURANT 1, which sorts by a per-bloke band byte,
 * the entrance sorts by the bloke's WORLD POSITION: everything left of the
 * entrance's x line is drawn first, then each turnstile's 0xb0-unit y band
 * gets its own pass followed by that turnstile's matte, and finally every
 * remaining bloke goes behind the booth.
 * ========================================================================= */

typedef struct RenderList {
    void* head;                /* +0x00 */
} RenderList;

struct RiderPerson {
    unsigned char pad00[0x20];
    int           depth;       /* +0x20  render sort key */
};

extern void  RenderItems_New(void);                                  /* 0x00442e90 */
extern void  AddBlokeToRenderList(RenderList*, RiderNode*, int key); /* 0x00442f20 */
extern void  RenderBlokeList(RenderList* list);                      /* 0x00442f70 */

extern RenderList g_entrance_list;   /* 0x006160ec  scratch render list */
extern void* g_entrance_matte1;      /* 0x006160fc  entrance_matte1.lls */
extern void* g_entrance_matte2;      /* 0x00616100  entrance_matte2.lls */
extern void* g_entrance_matte3;      /* 0x00616104  entrance_matte3.lls */
extern void* g_entrance_matte4;      /* 0x00616108  entrance_matte4.lls */
extern void* g_entrance_booth;       /* 0x0061610c  booth1.lls */

/* =========================================================================
 * 0x0042d9c0 -- ENTRANCE 1, cb_b0.
 *
 * item->+0x40 / +0x44 are the entrance's world-space y/x origin in TILES;
 * shifted left 8 they give the 24.8 thresholds the bloke positions are
 * tested against.  The four turnstile bands are y0+0x820..0x8d0,
 * y0+0x920..0x9d0, y0+0xc20..0xcd0 and y0+0xd20..0xdd0 -- i.e. two pairs of
 * lanes 0x100 apart, with a 0x300 gap between the pairs.
 *
 * Only the FIRST sprite is drawn in the caller's blit mode; the four
 * remaining sprites are drawn with mode 0 and no context block.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0042d9c0
void Entrance1_Draw(RideElem* elem, int x, int y, MapSquare* sq,
                    void* clip, int mode)
{
    RideObject* item = elem->data;
    Offset      screen;
    Offset      off;
    Offset      o2;
    RiderNode*  rd;
    Bloke*      p;
    int         px;
    int         py;
    int         wy;
    void*       spr;

    screen = GetScreenCoordsForObject(sq, item);
    off = GetRenderOffsetForLayer(item->layers, 3);
    AdjustOffsetForViewMode(&off);
    py = item->world_y0 + sq->by;
    px = item->world_x0 + sq->bx;
    py <<= 8;
    px <<= 8;

    RenderItems_New();
    g_entrance_list.head = 0;
    for (rd = item->riders; rd; rd = rd->next) {
        if (*(unsigned short*)sq == rd->ride_id) {
            p = rd->bloke;
            if (p->world.x < px)
                AddBlokeToRenderList(&g_entrance_list, rd, rd->person->depth);
        }
    }
    RenderBlokeList(&g_entrance_list);

    o2 = GetRenderOffsetForLayer(item->layers, 1);
    AdjustOffsetForViewMode(&o2);
    spr = GetSpriteForLayer(item->layers, 1);
    PrintSprite(spr, screen.ox + o2.ox, screen.oy + o2.oy, mode, 0);

    o2 = GetRenderOffsetForLayer(item->layers, 2);
    AdjustOffsetForViewMode(&o2);
    spr = GetSpriteForLayer(item->layers, 2);
    PrintSprite(spr, screen.ox + o2.ox, screen.oy + o2.oy, 0, 0);

    RenderItems_New();
    g_entrance_list.head = 0;
    for (rd = item->riders; rd; rd = rd->next) {
        if (*(unsigned short*)sq == rd->ride_id) {
            p = rd->bloke;
            if (p->world.x >= px) {
                wy = p->world.y;
                if (wy >= py + 0x820 && wy <= py + 0x8d0)
                    AddBlokeToRenderList(&g_entrance_list, rd, rd->person->depth);
            }
        }
    }
    RenderBlokeList(&g_entrance_list);
    if (g_entrance_matte4)
        PrintSprite(g_entrance_matte4, screen.ox + off.ox, screen.oy + off.oy, 0, 0);

    RenderItems_New();
    g_entrance_list.head = 0;
    for (rd = item->riders; rd; rd = rd->next) {
        if (*(unsigned short*)sq == rd->ride_id) {
            p = rd->bloke;
            if (p->world.x >= px) {
                wy = p->world.y;
                if (wy >= py + 0x920 && wy <= py + 0x9d0)
                    AddBlokeToRenderList(&g_entrance_list, rd, rd->person->depth);
            }
        }
    }
    RenderBlokeList(&g_entrance_list);
    if (g_entrance_matte3)
        PrintSprite(g_entrance_matte3, screen.ox + off.ox, screen.oy + off.oy, 0, 0);

    RenderItems_New();
    g_entrance_list.head = 0;
    for (rd = item->riders; rd; rd = rd->next) {
        if (*(unsigned short*)sq == rd->ride_id) {
            p = rd->bloke;
            if (p->world.x >= px) {
                wy = p->world.y;
                if (wy >= py + 0xc20 && wy <= py + 0xcd0)
                    AddBlokeToRenderList(&g_entrance_list, rd, rd->person->depth);
            }
        }
    }
    RenderBlokeList(&g_entrance_list);
    if (g_entrance_matte2)
        PrintSprite(g_entrance_matte2, screen.ox + off.ox, screen.oy + off.oy, 0, 0);

    RenderItems_New();
    g_entrance_list.head = 0;
    for (rd = item->riders; rd; rd = rd->next) {
        if (*(unsigned short*)sq == rd->ride_id) {
            p = rd->bloke;
            if (p->world.x >= px) {
                wy = p->world.y;
                if (wy >= py + 0xd20 && wy <= py + 0xdd0)
                    AddBlokeToRenderList(&g_entrance_list, rd, rd->person->depth);
            }
        }
    }
    RenderBlokeList(&g_entrance_list);
    if (g_entrance_matte1)
        PrintSprite(g_entrance_matte1, screen.ox + off.ox, screen.oy + off.oy, 0, 0);

    RenderItems_New();
    g_entrance_list.head = 0;
    for (rd = item->riders; rd; rd = rd->next) {
        if (*(unsigned short*)sq == rd->ride_id) {
            p = rd->bloke;
            if (p->world.x >= px) {
                wy = p->world.y;
                if ((wy < py + 0x820 || wy > py + 0x8d0)
                 && (wy < py + 0x920 || wy > py + 0x9d0)
                 && (wy < py + 0xc20 || wy > py + 0xcd0)
                 && (wy < py + 0xd20 || wy > py + 0xdd0))
                    AddBlokeToRenderList(&g_entrance_list, rd, rd->person->depth);
            }
        }
    }
    RenderBlokeList(&g_entrance_list);
    if (g_entrance_booth)
        PrintSprite(g_entrance_booth, screen.ox + off.ox, screen.oy + off.oy, 0, 0);
}

/* =========================================================================
 * CAROUSEL
 *
 * Class globals, all filled by the carousel's cb_a4 (0x0042c280):
 *   0x006160bc  the ObjDef            0x00616068  its layer holder
 *   0x006160c4  the per-square record list head (next @ +0, u16 square @ +4)
 *   0x006160b8/0x006160c0  z_Carousel.lls          (the z-buffer sprite)
 *   0x0061606c  Carousel Entrance Matte.lls
 *   0x00616070  Carousel Entrance Matte2.lls
 *   0x00616078/0x0061607c  the ride's rider pivot, taken from layer 0's
 *                          ILF offsets minus (0x58, 0xcd)
 * ========================================================================= */

typedef struct CarouselRec {
    struct CarouselRec* next;    /* +0x00 */
    unsigned short      square;  /* +0x04  packed {x,y} */
    unsigned char       boarded; /* +0x06  riders that have got on */
    unsigned char       aboard;  /* +0x07  riders still on board */
    signed char         frame;   /* +0x08  animation frame of the platform */
} CarouselRec;

/* GetLayer's output block (sprite2.c).  cb_b0 fetches layer 0 and then only
 * writes a zero into +0x10 -- the fetched layer is never read, so this is a
 * leftover in the original; the store survives because the block is
 * address-taken. */
typedef struct LayerOut {
    void* sprite;              /* +0x00 */
    int   dx;                  /* +0x04 */
    int   dy;                  /* +0x08 */
    int   pad0c;               /* +0x0c */
    int   f10;                 /* +0x10 */
    int   pad14;               /* +0x14 */
} LayerOut;

/* A 3D person record as the ride draw code sees it (joust.c's Person3D). */
struct Person3D {
    unsigned char pad00[0x1c];
    Offset        screen;      /* +0x1c  where the person is drawn */
    Offset        local;       /* +0x24  its own offset pair */
};

extern void  GetLayer(void* spr, LayerOut* out, unsigned int layer);  /* 0x00497e80 */
extern void  AdjustBlokePosition(Offset* p);                          /* 0x00442d60 */
extern CarouselRec* Carousel_FindRec(MapSquare* sq);                  /* 0x0042bc60 */

extern RenderObj* g_carousel_layers;  /* 0x00616068 */
extern void*      g_carousel_matte1;  /* 0x0061606c  Carousel Entrance Matte.lls */
extern void*      g_carousel_matte2;  /* 0x00616070  Carousel Entrance Matte2.lls */
extern int        g_carousel_pivot_x; /* 0x00616078 */
extern int        g_carousel_pivot_y; /* 0x0061607c */
extern SpriteObj* g_carousel_zspr;    /* 0x006160b8  z_Carousel.lls */

/* =========================================================================
 * 0x0042bcf0 -- CAROUSEL, cb_b0.
 *
 * Two completely separate draws depending on whether anybody is on the ride:
 *
 *  - EMPTY: just the three layers of the carousel sprite (0, 1, 2), with
 *    layers 0 and 2 wound to the record's animation frame.
 *  - OCCUPIED: layer 0, then Matte2, then layer 2, then the queueing/leaving
 *    riders in FOUR action-ordered passes (action 0, 1, 0x0d, 0x0e), then the
 *    z-buffer sprite's LLS frame is set from the record so the riders sort
 *    against the platform, then every RIDING rider (flags 0x80) is positioned
 *    from its per-rider offset plus the carousel pivot and drawn, and finally
 *    Matte1 goes over the lot.
 *
 * At most ten riders are collected and the array is not bounds-checked
 * (reproduced).  `n` lives in the dead `elem` argument slot.
 * ========================================================================= */

/* 412/412 instructions and every block in the right place (1300 B vs 1308 B),
 * but only 135 of the 412 compare equal: VC6 assigns the two callee-saved
 * registers the other way round -- the original keeps the rider walker in esi
 * and the record in edi (and later reuses esi for the blit mode and then the
 * array cursor), while this build uses edi for the walker and esi for the
 * record, which renames a register in roughly two thirds of the body and
 * pushes the four render loops' cursor into a stack slot.  Also two `mov
 * byte,0` stores come out as `mov byte,al` because the array's zero register
 * is still live.  ~30 variants searched (statement order, declaration order,
 * separate walk variables, aggregate-init placement, `n` hoisting, operand
 * order); the shape below is the closest found and is semantically exact. */
/* The residual is one callee-saved TIE-BREAK, the mirror of the one that was
 * just fixed in EarthSlide_Tick: the original puts the rider cursor `r` in
 * esi and the per-square record `rec` in edi, and VC6 here picks the other
 * way round, which renames two thirds of the body.  The original also loads
 * `item->riders` very early (before the array zeroing, at index 9) whereas we
 * emit it at index 15, and it stores `n = 0` as an immediate rather than
 * reusing the `xor eax,eax` the zeroing set up.  Measured this round and all
 * inert or worse: four positions for `n = 0;` in the opening run, moving
 * `r = item->riders;` down to the `if (r)`, `rec == 0` / `r != 0` spellings
 * of the two guards, and both declaration orders of r/rec.  The interleaving
 * trick that fixed EarthSlide_Tick (splitting a grouped pair of reads so the
 * two sums alternate) has no counterpart here -- worth looking for one. */
/* RESIDUAL, measured this round: 412/412 instructions, 1300 bytes against
 * 1308.  Two facts, and the first causes most of the 277:
 *  (1) SCHEDULING of `r = item->riders`.  The original issues
 *      `mov esi,[ebx+0xcc]` at index 9 -- BEFORE the `here[10] = {0}` fill --
 *      so the rider cursor lives in esi across the rep stosd; VC6 here sinks
 *      that load past the fill and puts it in edi (index 15), because edi is
 *      the register the rep stosd just finished with.  esi and edi then swap
 *      roles for the whole rest of the body, which is what most of the
 *      mismatch is.
 *  (2) `n = 0`.  The original stores it as an immediate,
 *      `mov byte ptr [esp+0x80],0`, AFTER the fill and after `push ebp`; VC6
 *      here folds it into the fill's zero (`mov byte ptr [esp+0x7c],al`)
 *      before the stosd, and homes the char 4 bytes lower.  This is the
 *      documented "where a flag local is assigned relative to a rep-movsd
 *      decides the zero register" lever, seen from the other side.
 *  Six orderings of the five leading statements (`r =`, `n = 0`, and the three
 *  ctx fields) were measured; none moves either load.  What is needed is
 *  something that makes the rider cursor live BEFORE the array fill. */
/* THIS ROUND: the six leading-statement orderings quoted above were extended to
 * ALL 120 permutations of the five (`r =`, `n = 0`, and the three ctx fields).
 * Every one of them emits 414 instructions with the rider load still sunk past
 * the `here[10] = {0}` fill; the best score is 279 and eleven permutations tie
 * on it, so statement order is provably NOT the dial.  Replacing the aggregate
 * initialiser with an explicit `memset(here, 0, sizeof here)` (before or after
 * the rider load) is worse -- 413 instructions, 391 mismatches -- so `= { 0 }`
 * is the right spelling and the split fill (`mov [esp+0x50],0` + `rep stosd`
 * of nine) is its signature.  What is still needed is whatever makes VC6 issue
 * `mov esi,item->riders` in the prologue region, BEFORE `xor eax,eax`; every
 * source-level knob tried so far leaves it after the `rep stosd`, where edi is
 * the warm register and the esi/edi roles invert for the whole body. */
/* THIS ROUND: the band loops were moved to the shared DrawBand helper above
 * (walk the PARAMETER, do not copy it into a local cursor).  That kills the
 * one hoisted `movsx` of the count and brings back every per-band
 * `test bl,bl / movsx` guard: 412 of 412 instructions, mismatch 216. */
/* THIS ROUND (216 -> 24), three structural levers, all measured:
 *  (1) THE ARRAY LIVES IN AN INNER SCOPE ENTERED AFTER `r = item->riders`.
 *      A function-level `here[10] = {0}` is emitted first no matter what
 *      (120 statement permutations proved it); as a block-scope local its
 *      fill follows the rider load, so `mov esi,[ebx+0xcc]` lands at index 9
 *      before the `rep stosd` and esi/edi take the original's roles for the
 *      whole body.  Same trick Restaurant1_Draw (exact) already carried.
 *  (2) `n = 0` is assigned BEFORE the block (before the fill in source):
 *      VC6 sinks it past the fill and past `push ebp` and emits the
 *      immediate `mov byte ptr [esp+0x80],0` the original has.  Inside the
 *      block, in any position before the ctx stores, it is folded into the
 *      fill's zero register (`mov byte,al`) four instructions too early;
 *      after ctx.square it is an immediate but in the wrong place.
 *  (3) THE RIDING LOOP CACHES `Bloke* b = r->bloke` (flags, person, f3c,
 *      f3e read through it, esi across the two AdjustBlokePosition calls)
 *      but the IP_RenderBlokeIn3DNow argument is `r->bloke` re-read through
 *      r's spill slot; VC6 parks r in the dead `n` slot (0x7c) and `o` in
 *      the dead `item` spill home (0x14).  With `r->bloke->x` everywhere,
 *      r stays in esi and `o` takes its own slot (+4 on the whole frame).
 * THE RESIDUAL (24 = 4 bands x 6) is one shape: the original puts the
 * queue-address `lea esi,[esp+0x50]` AFTER the band's `jle` (in the loop
 * preheader, between `movsx edx,cl` and the trip-count store) and threads
 * every band's failed guard straight to the block after the fourth band
 * (`jle 0x2d0` x4); we emit the lea in the guard block before the `jle`,
 * so the guard blocks are not empty and each `jle` only reaches the next
 * band.  Every spelling that moves the lea into the preheader (a cursor
 * local born inside the guard, `if (n <= 0) return;`, the count widened
 * before the guard, guard at the call site with a char or int count, an
 * inline macro, an escaped or live-past-the-bands `n`) makes VC6 CSE the
 * four `movsx` into one widened int (`mov [esp+0x7c],eax`) and DELETE the
 * guards of bands 1..3 (248-263); decrementing the char copy instead of an
 * int trip count gives a byte counter in a new slot (289).  The original
 * has the preheader lea AND per-band guards AND no CSE; the construct that
 * yields that combination was not found (~45 variants this round). */
// WIP-FUNCTION: LEGOLAND 0x0042bcf0  (412 of 412 instructions, audit mismatch 24; four per-band lea/jle transpositions -- see note)
void Carousel_Draw(RideElem* elem, int x, int y, MapSquare* sq,
                   void* clip, int mode)
{
    RideObject*  item = elem->data;
    unsigned short key;
    DrawCtx      ctx;
    Offset       offA;
    Offset       offB;
    LayerOut     lay;
    Offset       screen;
    RiderNode*   r;
    CarouselRec* rec;
    char         n;

    r = item->riders;
    n = 0;
    {
    Bloke* here[10] = { 0 };
    
    ctx.tag = 0x103;
    ctx.elem = elem;
    ctx.square = *(unsigned short*)sq;
    rec = Carousel_FindRec(sq);
    if (!rec)
        return;

    screen = GetScreenCoordsForObject(sq, item);
    GetLayer(item->layers, &lay, 0);
    lay.f10 = 0;

    if (r) {
        key = *(unsigned short*)sq;
        do {
            if (key == r->ride_id) {
                here[n] = r->bloke;
                n++;
            }
            r = r->next;
        } while (r);

        if (n != 0) {
            LLSSetFrame(GetLLSForLayer(g_carousel_layers, 0), rec->frame);
            offA = GetRenderOffsetForLayer(g_carousel_layers, 0);
            AdjustOffsetForViewMode(&offA);
            PrintSprite(GetSpriteForLayer(g_carousel_layers, 0),
                        screen.ox + offA.ox, screen.oy + offA.oy, mode, &ctx);

            offB = GetRenderOffsetForLayer(g_carousel_layers, 1);
            AdjustOffsetForViewMode(&offB);
            PrintSprite(g_carousel_matte2, screen.ox + offB.ox,
                        screen.oy + offB.oy, mode, &ctx);

            LLSSetFrame(GetLLSForLayer(g_carousel_layers, 2), rec->frame);
            offA = GetRenderOffsetForLayer(g_carousel_layers, 2);
            AdjustOffsetForViewMode(&offA);
            PrintSprite(GetSpriteForLayer(g_carousel_layers, 2),
                        screen.ox + offA.ox, screen.oy + offA.oy, mode, &ctx);

            DrawBand(here, n, 0);
            DrawBand(here, n, 1);
            DrawBand(here, n, 0x0d);
            DrawBand(here, n, 0x0e);

            *(short*)*(g_carousel_zspr->lls_holder) = rec->frame;

            for (r = item->riders; r; r = r->next) {
                if (*(unsigned short*)sq == r->ride_id) {
                    Bloke* b = r->bloke;
                    if (b->flags62 & 0x80) {
                        Offset     o;
                        Person3D*  p = b->person;

                        o.ox = g_carousel_pivot_x;
                        o.oy = g_carousel_pivot_y;
                        p->local.ox = b->f3c;
                        p->local.oy = b->f3e;
                        AdjustBlokePosition(&p->local);
                        AdjustOffsetForViewMode(&o);
                        p->screen.ox = b->f3c + o.ox + screen.ox;
                        p->screen.oy = b->f3e + o.oy + screen.oy;
                        AdjustBlokePosition(&p->screen);
                        IP_RenderBlokeIn3DNow(r->bloke);
                    }
                }
            }

            offA = GetRenderOffsetForLayer(g_carousel_layers, 1);
            AdjustOffsetForViewMode(&offA);
            PrintSprite(g_carousel_matte1, screen.ox + offA.ox,
                        screen.oy + offA.oy, mode, 0);
            return;
        }
    }

    LLSSetFrame(GetLLSForLayer(g_carousel_layers, 0), rec->frame);
    offA = GetRenderOffsetForLayer(g_carousel_layers, 0);
    AdjustOffsetForViewMode(&offA);
    PrintSprite(GetSpriteForLayer(g_carousel_layers, 0),
                screen.ox + offA.ox, screen.oy + offA.oy, mode, &ctx);

    offB = GetRenderOffsetForLayer(g_carousel_layers, 1);
    AdjustOffsetForViewMode(&offB);
    PrintSprite(GetSpriteForLayer(g_carousel_layers, 1),
                screen.ox + offB.ox, screen.oy + offB.oy, mode, &ctx);

    LLSSetFrame(GetLLSForLayer(g_carousel_layers, 2), rec->frame);
    offA = GetRenderOffsetForLayer(g_carousel_layers, 2);
    AdjustOffsetForViewMode(&offA);
    PrintSprite(GetSpriteForLayer(g_carousel_layers, 2),
                screen.ox + offA.ox, screen.oy + offA.oy, mode, &ctx);
    }
}

/* =========================================================================
 * BALLOONZ
 *
 * Class globals (filled by cb_a4, 0x0042a7b0):
 *   0x0081cde4  the ObjDef        0x00616044  its layer holder
 *   0x00616060  the per-square record list head (next @ +0, u16 square @ +4)
 *   0x00616048  Ballbasem1.lls    0x0061604c  Ballbasem2.lls
 *   0x00616050  Ballbasem3.lls    0x00616054  BZRedCarM1.lls
 *   0x00616058  BZGreenCarM1.lls  0x0061605c  BZBlueCarM1.lls
 *   0x00616040  z_Balloon2.lls    0x00616010  Zbuffers\balloonz.bnv
 * ========================================================================= */

typedef struct BalloonzRec {
    struct BalloonzRec* next;    /* +0x00 */
    unsigned short      square;  /* +0x04  packed {x,y} */
    unsigned char       pad06[0x13 - 0x06];
    signed char         car;     /* +0x13  which car is at the platform */
    unsigned char       pad14;
    signed char         wheel;   /* +0x15  wheel rotation frame */
    signed char         flag;    /* +0x16  flag/banner animation frame, 0..0x30 */
} BalloonzRec;

extern BalloonzRec* Balloonz_FindRec(MapSquare* sq);                 /* 0x0042a980 */
/* Which car sprite belongs at wheel frame `w` for car `c`: 0 while the ride
 * is loading (c == 1 and w > 0x17), otherwise w/8 + c*3 -- so the three car
 * colours repeat every 8 wheel frames. */
extern int Balloonz_CarSprite(char w, char c);                       /* 0x0042aa60 */

extern RenderObj* g_bz_layers;   /* 0x00616044 */
extern void*      g_bz_base1;    /* 0x00616048  Ballbasem1.lls */
extern void*      g_bz_base2;    /* 0x0061604c  Ballbasem2.lls */
extern void*      g_bz_base3;    /* 0x00616050  Ballbasem3.lls */
extern void*      g_bz_car_red;  /* 0x00616054  BZRedCarM1.lls */
extern void*      g_bz_car_green;/* 0x00616058  BZGreenCarM1.lls */
extern void*      g_bz_car_blue; /* 0x0061605c  BZBlueCarM1.lls */

/* =========================================================================
 * 0x0042b2e0 -- BALLOONZ, cb_b0.
 *
 * The balloon wheel is drawn as three base mattes with the queue interleaved
 * between them by the customers' state-machine step (+0x60), then the wheel
 * itself, then -- only on a wheel frame that is a multiple of 8, i.e. when a
 * car is level with the platform -- the boarding/alighting customers and the
 * car sprite, and finally every RIDING customer is positioned from the
 * layer-1 ILF offsets and drawn.  The banner (layer 2) always goes last, its
 * frame advanced and wrapped at 0x30 and written back to the record.
 *
 * Draw order by customer action:
 *   6, 5      -> Ballbasem1     4         -> Ballbasem2
 *   0,1,2,3   -> Ballbasem3     7,14,15   -> (wheel layer 1)
 *   8,9,13,14 -> (only on a platform frame, before the car sprite)
 *
 * ORIGINAL BUG, reproduced: the very first AdjustOffsetForViewMode call runs
 * on an UNINITIALISED Offset before anything has filled it.  Its result is
 * overwritten before use, so it is harmless, but the call is really there.
 *
 * At most six customers are collected and the array is not bounds-checked.
 * ========================================================================= */

/* 576/576 instructions with every block, loop and switch arm in the right
 * place, but the frame comes out 0x58 instead of 0x5c and that shifts almost
 * every [esp+N] reference (1719 B vs 1749 B, 507 index mismatches).  The
 * missing slot is `saved`: the original SPILLS the second walk's list head to
 * S-0x54 and colours the `car` byte onto the S-0x4c pair, while VC6 here
 * keeps `saved` in a register and gives `car` its own slot, so the flag byte
 * lands in the dead `elem` argument slot instead of the rider count.
 * Searched: char declaration order and types, `saved` placement/removal,
 * block-scope vs function-level pair locals.  Semantics are exact. */
/* FRAME ANALYSIS (this round, for whoever picks it up).  The whole body is
 * shifted because the frame is ONE 4-byte slot short: the original is
 * `sub esp,0x5c`, ours `sub esp,0x58`, and the six-entry `here` array (six
 * `mov dword ptr [..],0` stores, contiguous once the interleaved pushes are
 * unwound) sits at frame+0x2c in the original and frame+0x28 here.  The
 * missing slot is BELOW the array: the original homes ALL THREE of the record
 * bytes it stashes -- wheel at frame+0x04, flag at frame+0x03, car at
 * frame+0x10 -- and puts the collected count `n` in the DEAD arg-1 slot
 * (`mov byte ptr [esp+0x74],bl`, spilled immediately after `xor bl,bl` and
 * still kept in bl).  We home only two of the three bytes, give the dead arg
 * slot to `flag` instead, and never spill `n` at all.  So the question is not
 * "why is `saved` not spilled" (the earlier guess) but "what makes VC6 spill
 * the count to the dead parameter slot while keeping it in bl".  Moving
 * `n = 0;` around the AdjustOffsetForViewMode/FindRec calls does not do it
 * (three positions measured, all 0x58 and slightly worse). */
/* FRAME MAP AND RESIDUAL, measured this round (E = esp on entry, so the
 * return address is at E and the parameters at E+4..E+0x18).  Ours is
 * 576/576 instructions but `sub esp,0x58` against the original's `sub esp,0x5c`
 * -- FOUR bytes of locals short -- and that shift is what the 507 mismatches
 * really are.  Everything above the char pool is already identical:
 *     here[6]  E-0x30 .. E-0x19   (six dword zero stores, same order)
 *     &off     E-0x40             (the uninitialised pair handed to
 *                                  AdjustOffsetForViewMode)
 *     screen   E-0x38 / E-0x34
 *     &lay     E-0x18, lay.f10 at E+8
 * The difference is the CHAR pool and which char is homed in a dead parameter
 * slot:
 *     original   n -> E+4 (the dead `elem` slot, stored 0 with
 *                `mov byte ptr [esp+0x74],bl` before the first call)
 *                flag -> E-0x59, wheel -> E-0x58 (two chars PACKED into one
 *                dword), car -> E-0x4c
 *     ours       flag -> E+4, wheel -> E-0x58, car -> E-0x54 (each char in its
 *                own dword, so the pool is 4 bytes shorter)
 * So the question is not a missing variable: it is why VC6 packs `flag` next
 * to `wheel` and spills `n` to the parameter slot.  All 24 permutations of the
 * four char declarations emit byte-identical code, so declaration order is not
 * the lever here either. */
/* THIS ROUND THE WHOLE FRAME QUESTION ABOVE DISSOLVED, and the answer was not
 * a frame lever at all: moving the ten band loops to the shared DrawBand
 * helper above (walk the PARAMETER) stops the hoisted `movsx` of the count,
 * which stops VC6 parking the band CONSTANT in bl, which is what had been
 * destroying `n` -- and with `n` alive in bl for the whole body VC6 spills it
 * to the dead `elem` argument slot (`mov byte ptr [esp+0x74],bl`) exactly as
 * the original does, packs `flag`/`wheel` as adjacent BYTES at E-0x59/E-0x58,
 * and emits `sub esp,0x5c`.  The frame is now byte-for-byte the original's
 * except `car`, which we home at E-0x54 where the original has E-0x4c.
 * 584 -> 576 instructions (the original's exact count), mismatch 507 -> 341.
 * (A `struct { char flag; char wheel; }` DOES also produce `sub esp,0x5c` on
 * its own -- that was how the packing was first reproduced -- but it is
 * unnecessary once the band shape is right, and it scores slightly worse
 * (346), so it is not shipped.)
 * WHAT IS LEFT: the same lea/jle transposition as the other banded draws (one
 * per band), an edi/ebp rename that follows from it, `car`'s home, and one
 * schedule difference in the `off = GetRenderOffsetForLayer(...)` /
 * AdjustOffsetForViewMode pair at indices 61-71 (the original stores off.oy,
 * then takes &off and pushes it, then stores off.ox; we store ox first). */
/* THIS ROUND (audit 341 -> 333; the side-by-side lister says 339 -> 331), and
 * what the residual really is.  Everything up
 * to index 33 is exact; the whole body then diverges on ONE allocator
 * decision, the contest for the two free callee-saved registers after
 * `saved = item->riders` kills `item` (edi) and the collect loop kills
 * `sq` (ebp):
 *     original   band trip counters -> edi (`movsx edi,bl / dec edi`),
 *                screen.oy -> ebp from the first PrintSprite (index 102,
 *                `mov ebp,[esp+0x34]`, kept through index 275),
 *                `mode` reloaded from [esp+0x84] at every PrintSprite,
 *                the hoisted collect key in ax (`mov ax,[ebp]`),
 *                riding loop: r in ebx, the 0x12 pivot constant in ebp.
 *     ours       trip counters -> ebp, `mode` hoisted into edi at 101,
 *                screen.oy reloaded each time, the key in bp (VC6 reuses
 *                the dead sq register for it), riding loop r in ebp and
 *                0x12 in ebx.
 * So screen.oy must out-rank both `mode` and the trip counters in the
 * original's priority order and ranks below both here.  Two smaller
 * residuals: `car` and the `saved` spill swap homes (E-0x4c / E-0x54), and
 * the three occupied-path `off = GetRenderOffsetForLayer` sites store edx
 * (oy) before the `lea/push` and eax after, where we store eax first -- the
 * original's other two sites (empty path, banner) store eax first, so that
 * is allocation context, not a source shape (an inline helper for the pair
 * is byte-identical to the open-coded form).
 * Measured and inert (339): `saved` declared first or last, `mode` routed
 * through an int local assigned at entry (VC6 coalesces it), a u16 `key`
 * local for the collect compare, the collect loop as a `for`, the pair
 * helper, `pivot` assigned before `seat`, screen's halves copied into two
 * ints or into the dead x/y parameters (331, same as the operand order).
 * `screen.ox + off.ox` (this round's 8: the PrintSprite sums now build in
 * the original's temp order) is shipped.  A volatile read of `mode` (probe
 * only) is much worse (489): it changes the frame, so the original's
 * per-call reload is a spill decision, not a source construct.
 * The Carousel levers were checked here too: the array fill is already
 * interleaved with the pushes at the top (the fill precedes `r =
 * item->riders` in the original -- opposite of Carousel), `n = 0` already
 * lands as `mov [esp+0x74],bl`, and the riding loop already caches
 * `b = r->bloke`. */
// WIP-FUNCTION: LEGOLAND 0x0042b2e0  (576 of 576 instructions, audit mismatch 333; callee-saved contest -- see note)
void Balloonz_Draw(RideElem* elem, int x, int y, MapSquare* sq,
                   void* clip, int mode)
{
    RideObject*  item = elem->data;
    Bloke*       here[6] = { 0 };
    char         n;
    char         wheel;
    char         flag;
    char         car;
    RiderNode*   r;
    RiderNode*   saved;
    BalloonzRec* rec;
    Offset       off;
    Offset       screen;
    LayerOut     lay;

    r = item->riders;
    n = 0;
    AdjustOffsetForViewMode(&off);   /* on an uninitialised pair -- see above */
    rec = Balloonz_FindRec(sq);
    if (!rec)
        return;

    wheel = rec->wheel;
    flag = rec->flag;
    car = rec->car;
    screen = GetScreenCoordsForObject(sq, item);
    GetLayer(item->layers, &lay, 1);
    lay.f10 = 0;

    if (r) {
        do {
            if (*(unsigned short*)sq == r->ride_id) {
                here[n] = r->bloke;
                n++;
            }
            r = r->next;
        } while (r);

        if (n != 0) {
            saved = item->riders;
            off = GetRenderOffsetForLayer(g_bz_layers, 0);
            AdjustOffsetForViewMode(&off);

            DrawBand(here, n, 6);
            DrawBand(here, n, 5);
            PrintSprite(g_bz_base1, screen.ox + off.ox, screen.oy + off.oy, mode, 0);

            DrawBand(here, n, 4);
            PrintSprite(g_bz_base2, screen.ox + off.ox, screen.oy + off.oy, mode, 0);

            DrawBand(here, n, 0);
            DrawBand(here, n, 1);
            DrawBand(here, n, 2);
            DrawBand(here, n, 3);
            PrintSprite(g_bz_base3, screen.ox + off.ox, screen.oy + off.oy, mode, 0);

            DrawBand(here, n, 7);
            DrawBand(here, n, 0x0e);
            DrawBand(here, n, 0x0f);

            LLSSetFrame(GetLLSForLayer(g_bz_layers, 1), wheel);
            off = GetRenderOffsetForLayer(g_bz_layers, 1);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_bz_layers, 1),
                        screen.ox + off.ox, screen.oy + off.oy, mode, 0);

            if (wheel % 8 == 0 || wheel == 0) {
                LLSSetFrame(GetLLSForLayer(g_bz_layers, 1), wheel);
                off = GetRenderOffsetForLayer(g_bz_layers, 1);
                AdjustOffsetForViewMode(&off);

                DrawBand(here, n, 8);
                DrawBand(here, n, 9);
                DrawBand(here, n, 0x0d);
                DrawBand(here, n, 0x0e);

                switch (Balloonz_CarSprite(wheel, car)) {
                case 0:
                case 3:
                    PrintSprite(g_bz_car_red, screen.ox + off.ox,
                                screen.oy + off.oy, mode, 0);
                    break;
                case 1:
                case 4:
                    PrintSprite(g_bz_car_green, screen.ox + off.ox,
                                screen.oy + off.oy, mode, 0);
                    break;
                case 2:
                case 5:
                    PrintSprite(g_bz_car_blue, screen.ox + off.ox,
                                screen.oy + off.oy, mode, 0);
                    break;
                }
            }

            for (r = saved; r; r = r->next) {
                if (*(unsigned short*)sq == r->ride_id
                    && (r->bloke->flags62 & 0x80)) {
                    Bloke*    b = r->bloke;
                    Person3D* p;
                    Offset    seat;
                    Offset    pivot;

                    seat.ox = lay.dx + 0xc;
                    seat.oy = lay.dy - 6;
                    pivot.ox = 0x12;
                    pivot.oy = 0;
                    AdjustOffsetForViewMode(&pivot);
                    pivot.oy -= 8;
                    p = b->person;
                    p->local.ox = b->f3c - pivot.ox;
                    p->local.oy = b->f3e - pivot.oy;
                    AdjustBlokePosition(&p->local);
                    AdjustOffsetForViewMode(&seat);
                    p->screen.ox = b->f3c - pivot.ox + seat.ox + screen.ox;
                    p->screen.oy = b->f3e - pivot.oy + seat.oy + screen.oy;
                    AdjustBlokePosition(&p->screen);
                    IP_RenderBlokeIn3DNow(r->bloke);
                }
            }
            goto banner;
        }
    }

    LLSSetFrame(GetLLSForLayer(g_bz_layers, 1), wheel);
    off = GetRenderOffsetForLayer(g_bz_layers, 1);
    AdjustOffsetForViewMode(&off);
    PrintSprite(GetSpriteForLayer(g_bz_layers, 1),
                screen.ox + off.ox, screen.oy + off.oy, mode, 0);

banner:
    flag++;
    if (flag > 0x30)
        flag = 0;
    rec->flag = flag;
    LLSSetFrame(GetLLSForLayer(g_bz_layers, 2), flag);
    off = GetRenderOffsetForLayer(g_bz_layers, 2);
    AdjustOffsetForViewMode(&off);
    PrintSprite(GetSpriteForLayer(g_bz_layers, 2),
                screen.ox + off.ox, screen.oy + off.oy, mode, 0);
}
