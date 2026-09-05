/* LEGOLAND -- small per-ride record helpers: the mechanical rides' record
 * unlinks, the driving school's petrol-pump teardown, and four one-job
 * helpers the ride state machines call every tick.
 *
 * None of these is exported; every extent below was taken from the
 * disassembly by control flow (tools/audit.py).  Reconstructed from
 * original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct field OFFSETS,
 * record sizes and global addresses are load-bearing; the names are ours.
 * Types are defined LOCALLY on purpose (legoland.h is owned elsewhere).
 *
 * =========================================================================
 * ONE RECORD-UNLINK SOURCE, COMPILED FIVE TIMES HERE (eleven in the game)
 * =========================================================================
 * bswater2.c matched 0x0043d8c0 (PlaneRide_RemoveRecord) and noted that the
 * six mechanical rides share one unlink.  The family is bigger than that:
 * the SAME source is also compiled for GOLD RUSH and EARTH SLIDE, which are
 * not mechanical rides at all.  Five copies live here, differing ONLY in the
 * list-head global and the byte offset of the `next` link:
 *
 *   addr        function                  head        next   record
 *   0x00403c80  Copters_RemoveRecord      0x004c11b4  +0x04  0xd8
 *   0x00414a00  SafariRide_RemoveRecord   0x004cbf0c  +0x10  0x28
 *   0x00415930  SpiderRide_RemoveRecord   0x004cbf58  +0x2c  0x30
 *   0x00406960  GoldRush_FreeRecord       0x004c1204  +0x0c  0x2c
 *   0x0042cdc0  EarthSlide_FreeRec        0x006160e8  +0x0c  0x24
 *
 * All five are 32 instructions and identical index for index once the two
 * constants are substituted -- the diff is literally `s/0x4c11b4/.../` and
 * `s/+4/+0x10/`.  Against PlaneRide_RemoveRecord the only difference is that
 * the tail here is ONE call (`HeapFree_w(rec)`), so VC6 tail-DUPLICATES it
 * into the head arm instead of jumping to it; the plane's tail is a fade
 * plus a free plus an argument frame, which is too big to copy.  That is a
 * clean measurement of where VC6 SP3's tail-duplication threshold sits: one
 * call + `add esp,4` is copied, two calls + a 0x10-byte argument block is
 * not.
 *
 * The original bug is inherited with the source: the walk seeds `link` from
 * `g_head->next` WITHOUT testing `g_head`, so unlinking a record from an
 * empty list dereferences NULL.  It is unreachable in practice (the head
 * arm has already proved the list non-empty for `rec == head`, and every
 * caller holds a record it found on the list) but the code is there and the
 * trailing `if (node != 0)` it forces is dead on every path that reaches it.
 *
 * =========================================================================
 * THE PUMP PAIR IS THE ODD ONE OUT -- IT FREES BEFORE IT UNLINKS
 * =========================================================================
 * Pump_Destroy (0x00411ad0) is NOT the shape above.  It tests the head for
 * NULL first (so it cannot fault on an empty list), then hands the record to
 * the heap and only afterwards walks the list comparing against the freed
 * pointer.  `next` is cached in edi before the free, so nothing is read
 * through the dangling pointer -- but the head global is RE-READ after the
 * call (the free is an aliasing barrier) and the comparison `head == p` is
 * made on freed memory's address.  ridecb6.c records the same use-after-free
 * ordering in Road_Delete; the driving school's two list teardowns were
 * evidently written by the same hand.
 *
 * =========================================================================
 * NEW LEVERS THIS FILE MEASURED
 * =========================================================================
 *  - VC6 SP3's TAIL-DUPLICATION THRESHOLD, measured on one source compiled
 *    six times: a shared tail of ONE call plus its `add esp,4` is COPIED
 *    into the early-return arm (the five unlinks here); a tail of two calls
 *    plus a 0x10-byte argument block is JUMPED to instead
 *    (PlaneRide_RemoveRecord, bswater2.c).  Same C, same layout decision
 *    taken both ways -- so when a twin's early arm ends in `jmp` rather than
 *    a copy of the tail, look at the tail's SIZE, not at the source.
 *  - `memset` OF A SUB-OBJECT is how a small member is cleared without
 *    touching the rest.  `memset(&rec->cars, 0, 6)` emits
 *    `lea eax,[rec+0xd] / xor ecx,ecx / mov [eax],ecx / mov [eax+4],cx` --
 *    a base register for the member AND a SECOND zero register beside the
 *    `xor ebx,ebx` the surrounding zero web already uses.  Six per-byte
 *    stores give six byte moves (+2 instructions), a struct assignment from
 *    a `static const` zero loads it out of `.rdata` (+1), and one from a
 *    zeroed local spills the local (-1 and wrong from index 13).  So two
 *    zero registers in one small function is a sub-object `memset`.
 *  - `x += -t * 8`, `x -= t * 8` and `x += (-t) << 3` are BYTE-IDENTICAL
 *    (VC6 canonicalises all three to `neg / shl 3 / add`).  Do not spend a
 *    wave permuting them.  What DOES matter at the same site: a store
 *    through `r->bloke` kills the CSE, so writing the x and y halves as two
 *    separate `r->bloke->world.` expressions reproduces the original's
 *    second `mov eax,[edx+8]`; a named `Bloke*` local keeps one load.
 *  - NEGATIVE, 46 spellings: VC6 always jump-threads a provably-NULL pointer
 *    into a following `if (p)`.  The original's un-threaded form (a `xor
 *    <scratch>,<scratch>` hoisted into the join, then a re-test of a
 *    known-zero) is reachable ONLY through a `volatile` local, which pays a
 *    stack home the original does not have.  See Pump_SnapToRoad.
 *
 * =========================================================================
 * EXTERN TYPE NOTES (caller-side levers -- nothing else is aligned to them)
 * =========================================================================
 *  - StandardRemoveObject (0x0045f220) is declared here with the map square
 *    as a `BPosW` BY VALUE (bswater2.c's fourth spelling): the call pushes
 *    the packed square's whole home dword unmasked, which is what
 *    Pump_Remove's `mov edx,[esp+8] / push edx` is.  An `unsigned int`
 *    parameter costs an `and edx,0xffff`.  screencb.c declares the same
 *    address three OTHER ways; each is its own call site's lever.
 *  - EarthSlide_FreeRec is declared `void*` by ridecb8.c and is DEFINED here
 *    over a typed record; ridecb8.c is left alone.
 *  - Pump_SnapToRoad returns `RoadTile*` (screencb2.c's declaration) even
 *    though the +0x14 test it makes is on the `type` byte, not the record.
 * ========================================================================= */

/* ---- shared map types (same offsets as mechrides.c / ridecb6.c) --------- */
typedef struct Pos { int x; int y; } Pos;

/* A packed 2-byte map square passed BY VALUE, and its 16-bit view. */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union BPosW { unsigned short w; BPos b; } BPosW;

/* mechrides.c's RideTile: the packed map square every record is keyed by. */
typedef union RideTile {
    unsigned short key;                         /* +0x00 */
    struct { unsigned char x, y; } b;
} RideTile;

/* money.c's SoundSource.  kind 2 = "sourced at a map square". */
typedef struct SoundSource {
    int   kind;                     /* +0x00 */
    void* bloke;                    /* +0x04 */
    int   x;                        /* +0x08 */
    int   y;                        /* +0x0c */
} SoundSource;

extern void* HeapAlloc_w(unsigned int size);                    /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                               /* 0x0049e4d0 */
extern int   rand(void);                                        /* 0x0049e4b2 (CRT) */
extern void  UnSourceAndFadeAllSamplesFromSource(SoundSource* s,
                                                 int fade);     /* 0x00496c80 */

void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)


/* =========================================================================
 * THE FIVE COPIES OF THE RECORD UNLINK
 *
 * The shape, from bswater2.c's PlaneRide_RemoveRecord: `link` must be
 * declared BEFORE `node` and seeded from the GLOBAL so it takes the first
 * register, and the ONE volatile read on the link deref is what stops VC6
 * forwarding the condition's load into the body's (joust.c's lever).
 * ========================================================================= */

/* mechrides.c's CoptersRec (0xd8 bytes, list head 0x004c11b4). */
typedef struct CoptersRec {
    RideTile           tile;        /* +0x00 */
    unsigned short     pad02;
    struct CoptersRec* next;        /* +0x04 */
    unsigned char      pad08[0xd8 - 8];
} CoptersRec;                       /* 0xd8 */

extern CoptersRec* g_copter_recs;                               /* 0x004c11b4 */

// FUNCTION: LEGOLAND 0x00403c80
void Copters_RemoveRecord(CoptersRec* rec)
{
    if (g_copter_recs == rec) {
        g_copter_recs = rec->next;
    } else {
        CoptersRec** link = &g_copter_recs->next;
        CoptersRec*  node = g_copter_recs;

        while (*link != rec) {
            node = *(CoptersRec* volatile*)link;
            if (node == 0)
                break;
            link = &node->next;
        }
        /* Dead on every path that reaches it -- the original emits it. */
        if (node != 0)
            node->next = rec->next;
    }
    HeapFree_w(rec);
}

/* mechrides.c's SafariRec (0x28 bytes, list head 0x004cbf0c). */
typedef struct SafariRec {
    RideTile          tile;         /* +0x00 */
    unsigned short    pad02;
    unsigned char     pad04[0x10 - 4];
    struct SafariRec* next;         /* +0x10 */
    unsigned char     pad14[0x28 - 0x14];
} SafariRec;                        /* 0x28 */

extern SafariRec* g_safari_recs;                                /* 0x004cbf0c */

// FUNCTION: LEGOLAND 0x00414a00
void SafariRide_RemoveRecord(SafariRec* rec)
{
    if (g_safari_recs == rec) {
        g_safari_recs = rec->next;
    } else {
        SafariRec** link = &g_safari_recs->next;
        SafariRec*  node = g_safari_recs;

        while (*link != rec) {
            node = *(SafariRec* volatile*)link;
            if (node == 0)
                break;
            link = &node->next;
        }
        if (node != 0)
            node->next = rec->next;
    }
    HeapFree_w(rec);
}

/* mechrides.c's SpiderRec (0x30 bytes, list head 0x004cbf58). */
typedef struct SpiderRec {
    RideTile          tile;         /* +0x00 */
    unsigned char     pad02[0x2c - 2];
    struct SpiderRec* next;         /* +0x2c */
} SpiderRec;                        /* 0x30 */

extern SpiderRec* g_spider_recs;                                /* 0x004cbf58 */

// FUNCTION: LEGOLAND 0x00415930
void SpiderRide_RemoveRecord(SpiderRec* rec)
{
    if (g_spider_recs == rec) {
        g_spider_recs = rec->next;
    } else {
        SpiderRec** link = &g_spider_recs->next;
        SpiderRec*  node = g_spider_recs;

        while (*link != rec) {
            node = *(SpiderRec* volatile*)link;
            if (node == 0)
                break;
            link = &node->next;
        }
        if (node != 0)
            node->next = rec->next;
    }
    HeapFree_w(rec);
}

/* goldrush.c's GoldRec (0x2c bytes, list head 0x004c1204; the same record
 * ridesave.c's SaveGoldWash / LoadGoldWash serialise raw). */
typedef struct GoldRec {
    RideTile        tile;           /* +0x00 */
    unsigned char   pad02[0x0c - 2];
    struct GoldRec* next;           /* +0x0c */
    unsigned char   pad10[0x2c - 0x10];
} GoldRec;                          /* 0x2c */

extern GoldRec* g_gold_recs;                                    /* 0x004c1204 */

// FUNCTION: LEGOLAND 0x00406960
void GoldRush_FreeRecord(GoldRec* rec)
{
    if (g_gold_recs == rec) {
        g_gold_recs = rec->next;
    } else {
        GoldRec** link = &g_gold_recs->next;
        GoldRec*  node = g_gold_recs;

        while (*link != rec) {
            node = *(GoldRec* volatile*)link;
            if (node == 0)
                break;
            link = &node->next;
        }
        if (node != 0)
            node->next = rec->next;
    }
    HeapFree_w(rec);
}

/* screencb2.c's SlideRec (0x24 bytes, list head 0x006160e8). */
typedef struct SlideRec {
    unsigned char    pad00[0x0c];
    struct SlideRec* next;          /* +0x0c */
    unsigned char    pad10[0x24 - 0x10];
} SlideRec;                         /* 0x24 */

extern SlideRec* g_slide_head;                                  /* 0x006160e8 */

// FUNCTION: LEGOLAND 0x0042cdc0
void EarthSlide_FreeRec(SlideRec* rec)
{
    if (g_slide_head == rec) {
        g_slide_head = rec->next;
    } else {
        SlideRec** link = &g_slide_head->next;
        SlideRec*  node = g_slide_head;

        while (*link != rec) {
            node = *(SlideRec* volatile*)link;
            if (node == 0)
                break;
            link = &node->next;
        }
        if (node != 0)
            node->next = rec->next;
    }
    HeapFree_w(rec);
}


/* =========================================================================
 * THE DRIVING SCHOOL PUMPS
 * ========================================================================= */

/* ridecb6.c's Pump: the map square is TWO INTS, not the packed pair. */
typedef struct Pump {
    unsigned char pad00[4];
    int           x;                /* +0x04 */
    int           y;                /* +0x08 */
    struct Pump*  next;             /* +0x0c */
} Pump;

extern Pump* g_pump_list;                                       /* 0x004cbea4 */

/* -------------------------------------------------------------------------
 * 0x00411ad0 -- Pump_Destroy: free one pump, then unlink it.
 *
 * The head is tested for NULL up front, so this one cannot fault on an empty
 * list the way the five record unlinks above can.  Everything else about it
 * is unusual: the record is freed BEFORE the walk, the head global is
 * re-read afterwards (the free is the aliasing barrier that forces the
 * reload), and the walk therefore compares live links against a pointer the
 * heap already owns.  Reproduced exactly -- `next` is cached first, so no
 * freed memory is ever dereferenced.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00411ad0
void Pump_Destroy(Pump* p)
{
    Pump* prev;
    Pump* next;

    if (g_pump_list != 0) {
        next = p->next;
        HeapFree_w(p);
        if (g_pump_list == p) {
            g_pump_list = next;
        } else {
            prev = g_pump_list;
            while (prev->next != p) {
                prev = prev->next;
                if (prev == 0)
                    return;
            }
            if (prev != 0)
                prev->next = next;
        }
    }
}


/* ridecb6.c's Rect (five dwords -- the `rep movsd` below copies exactly 5). */
typedef struct Rect {
    int          left;              /* +0x00 */
    int          top;               /* +0x04 */
    int          right;             /* +0x08 */
    int          bottom;            /* +0x0c */
    struct Rect* next;              /* +0x10 */
} Rect;                             /* 0x14 */

/* ridecb8.c's ObjDef. */
typedef struct ObjDef {
    unsigned char pad00[0x3c];
    Rect          rect;             /* +0x3c class footprint */
    unsigned char pad50[0xc4 - 0x50];
    void*         instance;         /* +0xc4 the class's shared instance */
    unsigned char padc8[0xd0 - 0xc8];
} ObjDef;                           /* 0xd0 */

/* screencb2.c's Cursor. */
typedef struct Cursor {
    unsigned char  pad0000[0x1404];
    Pos            origin;          /* +0x1404 map square under the mouse */
    unsigned char  pad140c[0x1414 - 0x140c];
    Rect           rect;            /* +0x1414 the footprint being previewed */
    unsigned char  pad1428[0x1834 - 0x1428];
} Cursor;                           /* 0x1834 */

/* screencb2.c's RoadTile (the driving school's road blocks). */
typedef struct RoadTile {
    struct RoadTile* next;          /* +0x00 */
    unsigned char    pad04[4];
    unsigned short   school;        /* +0x08 the owning school's map square */
    unsigned char    pad0a[2];
    int              x;             /* +0x0c map square */
    int              y;             /* +0x10 */
    unsigned char    type;          /* +0x14 low nibble kind, 0x10 = crossing */
} RoadTile;

extern ObjDef*   g_pump_def;                                    /* 0x004cbe9c */
extern RoadTile* GetRoadRecord(int x, int y);                   /* 0x004125f0 */
/* The BPosW-BY-VALUE spelling of 0x0045f220 (bswater2.c's): this call site
 * pushes the whole home dword of the packed square with NO widening and no
 * mask, which an `unsigned int` parameter cannot do -- it inserts
 * `and edx,0xffff`.  screencb.c declares the same address three other ways;
 * each is its own call site's lever and none of them is aligned. */
extern void      StandardRemoveObject(void* inst, BPosW sq,
                                      Cursor* ctx);             /* 0x0045f220 */

/* -------------------------------------------------------------------------
 * 0x00411b20 -- Pump_Remove: unbuild one pump's footprint, then destroy it.
 *
 * A whole 0x1834-byte Cursor on the stack (hence the __chkstk probe) just to
 * hand StandardRemoveObject a footprint and a square: the class rect is
 * copied in with `rep movsd` and the pump's own map square written into the
 * origin.  The packed {x,y} form of the same square lives in its own 4-byte
 * local BELOW the cursor -- the aggregate takes the top of the frame, which
 * is what makes the two locals add up to exactly 0x1838.
 *
 * Note the square is built by truncating the pump's two INTS to bytes, so a
 * pump beyond map column 255 would pack wrong; the map is 256x256, so the
 * truncation is exact.  ridecb6.c's Pump_FindAt reads the same two ints.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00411b20
void Pump_Remove(Pump* p)
{
    BPosW  sq;
    Cursor cur;

    cur.rect = g_pump_def->rect;
    sq.b.x = (unsigned char)p->x;
    sq.b.y = (unsigned char)p->y;
    cur.origin.x = p->x;
    cur.origin.y = p->y;
    StandardRemoveObject(g_pump_def->instance, sq, &cur);
    Pump_Destroy(p);
}

/* -------------------------------------------------------------------------
 * 0x00411dc0 -- Pump_SnapToRoad: snap the edit cursor onto the road block
 * one square EAST of the mouse, and hand that block back.
 *
 * A pump attaches to a plain road block only: `type` must be zero, so a
 * junction, a crossing or any other kind is refused by zeroing the record
 * rather than by branching (the `xor eax,eax` inside the test).  When a
 * block is found the cursor origin jumps to the block's own square, shifted
 * one west and up by the CLASS footprint's top edge, so the preview covers
 * the pump's four-cell strip rather than the block.
 *
 * The `push edi` sits INSIDE the taken arm -- the split prologue, because
 * the only value needing a fourth register is the class rect's top.
 * ------------------------------------------------------------------------- */

/* RESIDUAL, precisely: the original does NOT jump-thread the `rec = 0` arm
 * into the trailing `return 0`.  It emits `xor eax,eax` (the clear), falls
 * into a join that starts `xor ecx,ecx`, then RE-TESTS the value it has just
 * proved zero (`test eax,eax / je`) and returns through `mov eax,ecx`.  Our
 * VC6 folds all of that: the two failure edges and the clear all jump
 * straight to `xor eax,eax / pop esi / ret`, four instructions short.
 *
 * Measured (46 spellings): every natural form -- if/else, `goto`, a ternary,
 * an `else if` chain, an inline predicate helper, a named result local, a
 * zero carried in its own local, a cast-to-int handle, an `int` type read, a
 * volatile read of the TYPE, a self-subtracting clear, two and three
 * `return 0` sites, an if/else phi in both arms, a `while` that runs at most
 * once, a `do/while(0)`, an inline `ClearRec(&rec)` helper, a degenerate
 * `else rec = rec;`, and four extra literal zeros to widen the zero web --
 * lands on one of exactly two shapes, 29i/77B: mismatch 20 (first divergence
 * at index 13, the branch sense) or mismatch 18 (first divergence at index
 * 15, the missing `xor ecx,ecx`).  Hoisting the class rect's top into a
 * block-scoped local is 27i/75B, mismatch 16.  The ONLY spelling
 * that reproduces the original's block structure is a `volatile` local for
 * the clear (`RoadTile* volatile vr; ... vr = 0; rec = vr;`): 33i/93B,
 * mismatch 3, exact from index 16 to the end -- but it pays for the barrier
 * with a stack home (`mov [esp+8],0 / mov eax,[esp+8]` where the original
 * has `xor eax,eax / xor ecx,ecx`), so it is not adopted.  What is missing
 * is a register-resident opaque zero: the join's `xor ecx,ecx` is what stops
 * VC6 threading, and no C spelling found puts a second zero register there. */

// WIP-FUNCTION: LEGOLAND 0x00411dc0  (29 of 33 insns, 77/85 B; first divergence at index 13 -- our `jne` threads the clear away where the original falls into a re-test of a known-zero; see the note above)
RoadTile* Pump_SnapToRoad(Cursor* c)
{
    RoadTile* rec = GetRoadRecord(c->origin.x + 1, c->origin.y);

    if (rec != 0 && rec->type != 0)
        rec = 0;
    if (rec == 0)
        return 0;
    c->origin.x = rec->x - 1;
    c->origin.y = rec->y - g_pump_def->rect.top;
    return rec;
}


/* =========================================================================
 * GOLD RUSH -- the panning tick (goldrush.c's action 6).
 *
 * The only step of the fourteen that does not advance the action itself:
 * it holds the "panning" animation flag, runs the pan animation one frame,
 * and counts the wander timer down.  At zero it drops the flag, goes back to
 * the walking-with-a-pan animation, gives the pan slot back, refreshes the
 * placement's "no more customers" flag and only THEN advances the action.
 *
 * `flags62 |= 0x100` narrows to `or byte ptr [b+0x63],1` while the clear
 * stays a 16-bit `and` -- the recorded u16 flag asymmetry, at a new site.
 * ========================================================================= */

typedef struct Pos8 { int x; int y; } Pos8;

/* goldrush.c's Bloke, cut down to what this tick touches. */
typedef struct Bloke {
    unsigned char  pad00[0x36];
    unsigned char  pan;             /* +0x36 which pan slot the visitor has */
    unsigned char  pad37[0x58 - 0x37];
    int            wander;          /* +0x58 ticks until the next random turn */
    unsigned char  pad5c[4];
    unsigned char  action;          /* +0x60 state-machine step */
    unsigned char  pad61;
    unsigned short flags62;         /* +0x62 8 = using this ride,
                                     *       0x100 = panning */
    unsigned char  pad64[4];
    Pos8           world;           /* +0x68 world position, 24.8 */
    unsigned char  pad70[2];
    unsigned char  dir;             /* +0x72 facing */
} Bloke;

typedef struct RiderNode {
    struct RiderNode* next;         /* +0x00 */
    struct RiderNode* prev;         /* +0x04 */
    Bloke*            bloke;        /* +0x08 */
    RideTile          ride_id;      /* +0x0c the packed map square in use */
    unsigned short    pad0e;
    void*             owner;        /* +0x10 */
} RiderNode;

extern void BlokePanWithPan(Bloke* b);                          /* 0x00440970 */
extern void BlokeAnimNextFrame(Bloke* b);                       /* 0x004408e0 */
extern void BlokeWalkWithPan(Bloke* b);                         /* 0x00440960 */
/* Hand the visitor's pan slot back to its GoldRec (the inverse of
 * goldrush.c's GoldRush_ClaimPan). */
extern void GoldRush_ReleasePan(RiderNode* rd);                 /* 0x00406f00 */
extern void GoldRush_UpdateFullFlag(RideTile* key);             /* 0x00406f30 */

// FUNCTION: LEGOLAND 0x00407250
void GoldRush_PanTick(RiderNode* rd)
{
    Bloke* b = rd->bloke;

    b->flags62 |= 0x100;
    BlokePanWithPan(b);
    BlokeAnimNextFrame(b);
    b->dir = 1;
    b->wander = b->wander - 1;
    if (b->wander <= 0) {
        b->flags62 &= ~0x100;
        BlokeWalkWithPan(b);
        GoldRush_ReleasePan(rd);
        GoldRush_UpdateFullFlag(&rd->ride_id);
        b->action++;
    }
}


/* =========================================================================
 * BALLOONZ -- 0x0042a8f0  Balloonz_NewRecord (the +0x98 place path).
 *
 * ridecb8.c's Balloonz_Add hands this the packed square of a freshly placed
 * BALLOONZ.  Same "allocate, memset, fill, push" shape as bswater2.c's
 * Restaurant2_NewRecord and joust.c's Joust_AddRecord: the whole record is
 * zeroed AND then every field written again, because VC6's `rep stosd` is
 * not a kill and the redundant stores survive.  The packed square is the
 * only non-zero field -- a BALLOONZ starts with an empty wheel.
 *
 * NEW LEVER, and it is what this body turns on: the six gondola bytes are
 * cleared by an INTRINSIC `memset` OF THE SUB-OBJECT, not by six byte
 * stores and not by a struct assignment.  `memset(&rec->cars, 0, 6)` is the
 * only spelling that emits the original's `lea eax,[edx+0xd] / xor ecx,ecx
 * / mov [eax],ecx / mov [eax+4],cx` -- the base register for the sub-object
 * plus a SECOND zero register beside the `xor ebx,ebx` the rest of the web
 * uses.  Six separate `cars.c[i] = 0` stores give six byte moves (37i), a
 * struct assignment from a `static const` zero loads it from `.rdata` (36i)
 * and one from a zeroed local spills the local (34i).
 * ========================================================================= */

/* ridecb3.c's BalloonzRec (0x20 bytes, list head 0x00616060).  `cars` is one
 * 6-byte unit -- ridecb3.c copies it in and out of the tick frame as a
 * dword plus a word for the same reason. */
typedef struct BzCars { unsigned char c[6]; } BzCars;

typedef struct BalloonzRec {
    struct BalloonzRec* next;       /* +0x00 */
    unsigned short      square;     /* +0x04 */
    unsigned char       pad06[2];
    int                 waiting;    /* +0x08 riders queued but not boarded */
    char                riders;     /* +0x0c riders on the wheel, max 6 */
    BzCars              cars;       /* +0x0d per-gondola state */
    char                half;       /* +0x13 which half-turn, 0 or 1 */
    char                wheel;      /* +0x14 wheel angle, 0..0x17 */
    char                zframe;     /* +0x15 the frame in the z-sprite */
    char                f16;        /* +0x16 */
    char                alighting;  /* +0x17 riders getting off */
    int                 docked;     /* +0x18 */
    int                 unloading;  /* +0x1c */
} BalloonzRec;                      /* 0x20 */

extern BalloonzRec* g_bz_recs;                                  /* 0x00616060 */

// FUNCTION: LEGOLAND 0x0042a8f0
void Balloonz_NewRecord(BPosW* sq)
{
    BalloonzRec* rec = (BalloonzRec*)HeapAlloc_w(sizeof(BalloonzRec));

    if (rec != 0) {
        memset(rec, 0, sizeof(BalloonzRec));
        rec->square = sq->w;
        rec->next = g_bz_recs;
        rec->waiting = 0;
        rec->riders = 0;
        memset(&rec->cars, 0, sizeof(rec->cars));
        rec->half = 0;
        rec->wheel = 0;
        rec->zframe = 0;
        rec->f16 = 0;
        rec->alighting = 0;
        rec->docked = 0;
        rec->unloading = 0;
        g_bz_recs = rec;
    }
}


/* =========================================================================
 * CAROUSEL -- 0x0042c210  Carousel_StopRide (RUNNING -> IDLE).
 *
 * bswater.c's tick calls this once GetAllBlokesOffRide reports the platform
 * clear.  It resets the go-round: the half-tick counter, the platform frame,
 * the boarded and expected rider counts, a fresh revolution count of
 * rand() % 2 + 3, and it drops BOTH machine bits (0x4000 boarding and
 * 0x0001 running) in one `and ...,0xffffbffe`.  Then it fades whatever the
 * ride was playing at its own map square.
 *
 * `rand() % 2` is the signed remainder, so it is the five-instruction
 * `and eax,0x80000001 / jns / dec / or -2 / inc` sequence, and the `+ 3` is
 * done in AL because the field is a byte -- `(unsigned char)(rand() % 2 + 3)`
 * and `(unsigned char)(rand() % 2) + 3` are byte-identical.
 * ========================================================================= */

/* bswater.c's CarouselRec (list head 0x006160c4). */
typedef struct CarouselRec {
    struct CarouselRec* next;       /* +0x00 */
    unsigned short      square;     /* +0x04 packed {x,y} */
    unsigned char       boarded;    /* +0x06 riders that have got on */
    unsigned char       aboard;     /* +0x07 riders still on board */
    signed char         frame;      /* +0x08 platform animation frame */
    unsigned char       pad09[3];
    int                 flags;      /* +0x0c 1 = running, 0x4000 = boarding */
    unsigned char       revs;       /* +0x10 revolutions left to turn */
    unsigned char       pad11[3];
    int                 half;       /* +0x14 the divide-by-two frame counter */
    unsigned char       visitors;   /* +0x18 riders this go-round waits for */
    unsigned char       pad19[3];
    int                 timer;      /* +0x1c idle -> boarding countdown */
    unsigned char       seat[4];    /* +0x20 one byte per seat, 0 = free */
} CarouselRec;                      /* 0x2c */

// FUNCTION: LEGOLAND 0x0042c210
void Carousel_StopRide(CarouselRec* rec)
{
    SoundSource src;

    rec->half = 0;
    rec->frame = 0;
    rec->revs = (unsigned char)(rand() % 2 + 3);
    rec->visitors = 0;
    rec->boarded = 0;
    rec->flags &= ~0x4001;

    src.kind = 2;
    src.x = ((BPos*)&rec->square)->x;
    src.y = ((BPos*)&rec->square)->y;
    UnSourceAndFadeAllSamplesFromSource(&src, -200);
}


/* =========================================================================
 * RESTAURANT 2 -- 0x0042fa90  Restaurant2_SeatCustomer.
 *
 * ridecb3.c's customer machine calls this once a frame while a diner walks
 * to (which = 1) or away from (which = 2) the table, with `step` the walk's
 * frame counter.  It applies ONE waypoint delta to the diner's 24.8 world
 * position -- the same delta to x and to y, i.e. a move straight along one
 * isometric diagonal.
 *
 * The two arms use the waiter's TWO path tables and they are indexed
 * DIFFERENTLY, which is the find here: `which == 1` reads the x table at
 * 0x004b6860, one entry PAST its base (ridecb3.c's `g_r2_path_dx` starts at
 * 0x004b685c and its entry 0 is a zero), and negates it; `which != 1` reads
 * the y table at its own base and adds it.  So the walk in and the walk out
 * are one step out of phase with each other, and the leading zero of the x
 * table is never seen by this path.
 *
 * `r->bloke` is re-derived for the y half: the x store kills the CSE, which
 * is why the original loads it twice.  Write the two halves as two separate
 * `r->bloke->world.` expressions and it falls out; a named `Bloke*` local
 * would keep one load.  `+= -t * 8`, `-= t * 8` and `+= (-t) << 3` are all
 * byte-identical (VC6 canonicalises to `neg` + `shl` + `add`).
 * ========================================================================= */

extern int g_r2_path_dx[];      /* 0x004b685c the waiter's x steps, 33 ints */
extern int g_r2_path_dy[];      /* 0x004b68e0 the waiter's y steps, 33 ints */

// FUNCTION: LEGOLAND 0x0042fa90
void Restaurant2_SeatCustomer(RiderNode* r, int which, int step)
{
    if (which == 1) {
        r->bloke->world.x -= g_r2_path_dx[step + 1] * 8;
        r->bloke->world.y -= g_r2_path_dx[step + 1] * 8;
    } else {
        r->bloke->world.x += g_r2_path_dy[step] * 8;
        r->bloke->world.y += g_r2_path_dy[step] * 8;
    }
}
