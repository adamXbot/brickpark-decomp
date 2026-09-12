/* LEGOLAND -- the BOATING SCHOOL / DRIVING SCHOOL ROADS callback cluster at
 * 0x00413xxx..0x0041cxxx (the lowest-addressed block of per-class handlers).
 *
 * These are the handlers SetCustomCallbacks (screen.c 0x00452c20) installs
 * into the 0xd0-byte ObjDef callback slots.  They are NOT exported, so every
 * extent below was taken from the disassembly by control flow
 * (tools/audit.py).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere).
 *
 * ------------------------------------------------------------------------
 * WHICH RIDE?  -- read back out of SetCustomCallbacks, this lane is TWO
 * classes, not one:
 *
 *   addr        class                  slot     what it really is      state
 *   0x0041a040  BOATING SCHOOL         cb_98    BoatingSchool_Add    WIP 218i, 8 X
 *   0x0041a720  BOATING SCHOOL         cb_a8    BoatingSchool_Tick   WIP 358i, 47 X
 *   0x0041c130  BOATING SCHOOL WATER   cb_9c    BoatingSchoolWater_R [OK]
 *   0x00413b50  DRIVING SCHOOL ROADS   cb_90    Roads_CalcCursor     [OK]
 *
 * ... plus the seven unowned helpers those two modules call, six of them
 * exact:
 *
 *   0x004125a0  Road_FindAt           4x4 block whose ORIGIN is (x,y)   [OK]
 *   0x004125f0  GetRoadRecord         4x4 block COVERING (x,y)          [OK]
 *   0x004134f0  Road_KeepNeighbour    may this neighbour be joined?     [OK]
 *   0x00413520  Road_CardinalGroup    the four joinable cardinals       [OK]
 *   0x004135d0  Road_FindCardinals    ring slots 0/2/4/6                [OK]
 *   0x00413450  Road_FindDiagonals    ring slots 1/3/5/7                [OK]
 *   0x00413e30  Road_SnapCursor       snap the cursor to the road grid  [OK]
 *
 * NOTE ON THE WIPs (2026-09-04).  Two of the former four closed in the
 * previous round, and both fixes are levers that transfer to the twins in
 * ridecb1.c/ridecb2.c:
 *   - Roads_CalcCursor: an uninitialised u16 local read as a WORD needs the
 *     u16 wrapped in a one-member struct (a plain u16 whose uses are all
 *     promoted compares is stored int-wide);
 *   - BoatingSchoolWater_Remove: a volatile flag computed THROUGH a plain
 *     scalar (`n0 = m & 1; north = n0; if (n0)`) stays out of the scratch
 *     rotation, and byte fields passed to a call must be widened into int
 *     locals (in declaration order) BEFORE the call.
 * The remaining three keep the original's instruction count, block order
 * and stack homes; each note above its marker records the measured
 * residual, the first diverging index and everything ruled out.
 *
 * 2026-09-04 round: BoatingSchool_Tick went 289 -> 53 on two `volatile`
 * SHIMS that each force one extra step of VC6's eax->ecx->edx scratch
 * rotation -- one on case 0's `st->count++` (which stops VC6's tail merger
 * from swallowing the queue-full arm into case 6) and one on CalcMoveLine's
 * `b->ty` argument in cases 0 and 3 ONLY.  Both are marked in the body and
 * must come out when the real rotation break is found; the note above the
 * marker says exactly what each buys and what each costs.  Road_FindDiagonals
 * gained no ground but its shape is now PROVEN (see its note: an
 * uninitialised second counter reproduces the original 64/64 with 5 X, which
 * pins the original's two reloads as one local's per-edge phi).
 *
 * The same four addresses appear again in loaders.c's GetInterface
 * (0x0041b150), the BOATING SCHOOL family's built-in object-library
 * interface table, which is how the slot numbers are confirmed
 * independently: cb_98 = place/add, cb_9c = remove, cb_a8 = the per-frame
 * service tick, cb_90 = the placement-cursor calculator.
 *
 * ------------------------------------------------------------------------
 * THE BOATING SCHOOL DATA MODEL
 *
 * A boating school is one 0x34-byte STATION record per placed building, kept
 * on a singly linked list whose head is 0x004cc074 (next @ +0x2c).  It is the
 * same shape as the jungle cruise's station (ridecb2.c) with the fields
 * packed tighter:
 *
 *   +0x00 u16 key       the building's map square, packed {u8 x, u8 y}
 *   +0x02 u8  ax        route start x   ] the two dock cells, computed at
 *   +0x03 u8  ay        route start y   ] placement from the class's two
 *   +0x04 u8  bx        route end x     ] footprint sub-rects (see below)
 *   +0x05 u8  by        route end y     ]
 *   +0x08     route     the laid boat route (0x0041c8c0 builds it)
 *   +0x0c int frame     the building's own animation frame, INITIALISED TO
 *                       9999 by the add handler (see the bug note there)
 *   +0x10 int backwards 0 = counting up, 1 = counting down
 *   +0x14 int count     how many visitors are in the queue
 *   +0x18     q[5]      the five queue slots, q[0] at the head
 *   +0x2c     next
 *   +0x30 int take      accumulated takings, seeded to 5 at placement
 *
 * The class's geometry is three static Rects CHAINED through their `next`
 * fields, 0x18 apart in .data:
 *
 *   0x004cc078  the whole footprint      next -> 0x004cc060
 *   0x004cc060  dock A (the near jetty)  next -> 0x004cc048
 *   0x004cc048  dock B (the far jetty)   next -> 0
 *
 * (Roads_CalcCursor's sibling at 0x0041a2f0 builds that chain and hands it to
 * the edit cursor, which is why the placement preview shows the building plus
 * its two water cells.)  The add handler turns the two dock rects into the
 * station's route endpoints and stamps the water tiles: dock A gets river
 * mask 1, dock B mask 4, and every cell of the footprint rect gets a tile
 * from the water tileset -- base+9 on the left edge, base+0xc on the right
 * edge and the base tile in between.
 *
 * ------------------------------------------------------------------------
 * THE DRIVING SCHOOL ROAD MODEL (recovered here)
 *
 * Roads are laid on a FOUR-cell grid: every road record covers a 4x4 block of
 * map squares, and the two lookups either side of the placement code say so --
 * 0x004125a0 matches a record's origin EXACTLY, 0x004125f0 matches anything
 * inside its 4x4 block (`(unsigned)(x - rec->x) < 4`).  Both walk one global
 * list at 0x004cbeac (next @ +0x00, origin @ +0x0c/+0x10) and both are
 * bounds-checked against the map first.
 *
 * A road record's low nibble at +0x14 is its KIND; kind 6 is the piece the
 * placement code refuses to build onto except from the west, and kind 5 is
 * the lit junction the minimap draws (renderview.c).  Its u16 at +0x08 is the
 * road GROUP id -- every piece of one connected road carries the same value,
 * and that is what Roads_CalcCursor matches its neighbours against.
 *
 * The placement rule the cursor enforces is a 2x2 rule: collect the eight
 * neighbours in ring order N, NE, E, SE, S, SW, W, NW and reject the square
 * when any three CONSECUTIVE ring entries belong to the same road group,
 * because that plus the square being placed would be a solid 2x2 block of
 * road.  The four masks 0x07 / 0x1c / 0x70 / 0xc1 in the body are exactly
 * those four corners.
 * ========================================================================= */

/* ---- shared map/cursor types (same offsets as objmap2.c / ridecb2.c) ---- */
typedef struct Pos { int x; int y; } Pos;

typedef struct Rect {
    int left;                       /* +0x00 */
    int top;                        /* +0x04 */
    int right;                      /* +0x08 */
    int bottom;                     /* +0x0c */
    struct Rect* next;              /* +0x10 */
} Rect;

/* A packed 2-byte map square passed BY VALUE, and its 16-bit view. */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union BPosW { unsigned short w; BPos b; } BPosW;

struct Bloke;

/* ---- the boating school's station record ------------------------------- */
typedef struct BsStation {
    unsigned short     key;         /* +0x00 packed map square */
    unsigned char      ax;          /* +0x02 route start x */
    unsigned char      ay;          /* +0x03 route start y */
    unsigned char      bx;          /* +0x04 route end x */
    unsigned char      by;          /* +0x05 route end y */
    unsigned char      pad06[2];
    void*              route;       /* +0x08 */
    int                frame;       /* +0x0c animation frame */
    int                backwards;   /* +0x10 animation direction */
    int                count;       /* +0x14 visitors queueing */
    struct Bloke*      q[5];        /* +0x18 the five queue slots */
    struct BsStation*  next;        /* +0x2c */
    int                take;        /* +0x30 accumulated takings */
} BsStation;                        /* 0x34 */

/* The water tileset descriptor the add handler quotes its base tile from. */
typedef struct BsTileSet {
    unsigned char   pad00[4];
    unsigned short* codes;          /* +0x04 -> the tileset's code table */
} BsTileSet;

/* ---- globals ----------------------------------------------------------- */
extern Rect        g_bs_dock_b;         /* 0x004cc048 dock B, next -> 0 */
extern Rect        g_bs_dock_a;         /* 0x004cc060 dock A, next -> dock B */
extern BsStation*  g_bs_stations;       /* 0x004cc074 the station list head */
extern Rect        g_bs_footprint;      /* 0x004cc078 whole rect, next -> A */
extern BsTileSet*  g_bs_water_tiles;    /* 0x0082adf4 */

/* ---- externs ----------------------------------------------------------- */
extern void* HeapAlloc_w(unsigned int size);                 /* 0x0049e4ff */
#ifndef LEGOLAND_PORTABLE
extern void  AddBasicObject(void* obj, Pos* pos);            /* 0x0045efe0 */
#else
extern void AddBasicObject(void* ll_obj, void* ll_pos, void* ll_ctx);            /* 0x0045efe0 */
#define AddBasicObject(_a1, _a2) AddBasicObject((_a1), (_a2), 0)
#endif
extern void  SetMapTile(int x, int y, unsigned short tile);  /* 0x00461780 */
/* Re-stamps the river tile at (x, y) for a direction mask and hands back the
 * owning station's packed map square through `owner`. */
extern void  BsWater_SetTile(int x, int y, int mask, void* owner); /* 0x0041c4c0 */

/* =========================================================================
 * 0x0041a040 -- BoatingSchool_Add (BOATING SCHOOL cb_98 / iface slot 3).
 *
 * Allocates the station record for a newly placed boating school, links it on
 * the class list, places the building through the standard object placer and
 * then lays the water: the two dock cells are given river masks 1 and 4 and
 * the whole footprint rect is filled with the water tileset's tiles.
 *
 * ORIGINAL QUIRK reproduced: `frame` (+0x0c) is seeded to 9999 while the tick
 * handler wraps it at 100, so a freshly built school runs its animation from
 * 10000 upwards until the counter is reset by a save/load.
 *
 * RESIDUAL (measured 2026-09-03, audit 218/218 insns, 683/683 bytes,
 * mismatch=8, first diverging index 39).  Indices 39..50 only: the original
 * emits the zero-stores in source order and threads the AddBasicObject
 * argument through them -- `mov edx,[esp+14h]` (o) at 42 between the q[0]
 * and q[1] stores, `push edx` at 45 between q[2] and q[3], and the list-head
 * load at 48 directly before its store -- whereas we hoist the `o` load to
 * 39 (the first slot after dl dies), sink the push to 49 and load the head
 * one slot early.  Registers, stack homes and everything outside 39..50 are
 * exact.  What the measurements established:
 *   - the `o` load is hoisted to the EARLIEST free scratch register: with
 *     the plain order `q[4]=0; next=head; head=st` it climbs to index 33 in
 *     ecx and renames the by-temp cl->bl (147 X); only `next=head` spelled
 *     before `q[4]=0` keeps head in ecx and o in edx (the current 8 X); every
 *     other position of the link statement (8 tried) gives 8 or worse;
 *   - `void* volatile o` pins the load at its IR position: it then lands at
 *     49 adjacent to its push, so the original's load/push gap is not a
 *     latency rule but a split of one IR node placed BETWEEN the stores; a
 *     volatile read copied into a named local (after q[0]/q[1]/take) puts
 *     the copy in a register and destroys the bl plan (199 X);
 *   - ruled out with no change (still 8 X): `obj = o` at entry, `int o`
 *     with a cast, a one-member struct parameter by value, `*&o`, a pointer
 *     alias `po=&o; *po`, `(void)&o`, chained `q[0]=q[1]=q[2]=0` (9 X),
 *     `backwards=count=0` (9 X), `q[3]=q[4]=0` after the link;
 *   - `memset(st->q,0,20)` and a memset over +0x08..+0x2b expand through
 *     `lea ecx,[edi+18h]` pointer stores, not [edi+disp] (wrong shape).
 * 2026-09-04 -- re-measured and pushed further, still 8.  The store ORDER is
 * already exact: both bodies emit count, take, q[0..4], next, head in that
 * sequence, and the ONLY difference is where the three non-store operations
 * are interleaved.  The original spreads them evenly --
 *     S S S | Lo | S S | Push | S S | Lhead | S S
 * -- while we clump them at the edges --
 *     Lo | S S S S S S | Lhead | S S | Push | S.
 * The full cross-product of {take at each of the 12 positions} x {next/q[4]/
 * head in all three orders} was measured (36 bodies): the current spelling is
 * the unique minimum at 8 X and diff=26; the two 7-X bodies (take before
 * `count`, take after q[0]) are false alignments with diff=28.  The natural
 * order `q[4] = 0; next = head; head = st;` is confirmed at 147 X in six more
 * spellings (plain, cast, `**hp`, an `int` type-pun, and two temp-variable
 * forms): the head load always lands in eax once the last zero store is
 * behind it, which is why the link statements must sit before `q[4] = 0`.
 * Also inert this round: the extern prototype as `(int, Pos*)`, `(void*,
 * void*)` and unprototyped; `*(void**)&o`; `(Pos*)pos`; a block around the
 * link group; a comma expression; `(struct Bloke*)0` casts; the q slots
 * written as `*(int*)((char*)st + N)`; and `if (o) f(o,pos); else f(o,pos);`.
 * Best hypothesis (unchanged): the original's argument load/push pair was
 * placed by VC6's scheduler at a position no C statement order reaches (the
 * same class as ridecb9.c's JungleCruise_Add zero split and popup.c's
 * RequestRoute argument order).
 *
 * 2026-09-10, scope LL21.  UNCHANGED at 8, but the scope-Codex-F cancelled-
 * pair ANCHOR lever produced ONE result worth recording, because it overturns
 * a premise of the note above.
 *  - *** THE NATURAL STORE ORDER IS REACHABLE AFTER ALL. ***  With
 *    `q[4] = 0; next = head; head = st;` -- the order the disassembly
 *    actually has, and which every earlier pass measured at 146/147 X because
 *    the head load takes eax -- carrying the head through a cancelled pair
 *    ANCHORED ON `o` (`t.anchor = (int)o; t.h = (int)g_bs_stations;
 *    t.h += t.anchor; t.h -= t.anchor; st->next = (BsStation*)t.h;`) collapses
 *    it from 97 aligned mismatches to SIX, at 218 instructions.  Everything
 *    outside the store block, including the whole water-laying tail, stays
 *    exact.  So the committed body's artificial `next = head` BEFORE `q[4]`
 *    is a workaround for a register rank, not evidence about the source.
 *  - It still is not better: in that shape `o` lands in ecx and the head in
 *    eax, where the original has `o` in edx and the head in ecx -- both
 *    exactly ONE step along VC6's eax->ecx->edx scratch rotation.  Every
 *    documented rotation-advance was tried on top of it and none moves it:
 *    free volatile reads at pos->x, pos->y, g_bs_dock_a.left and on the head
 *    load itself (7/9/8/7), a volatile read of `o` (41), a second cancelled
 *    pair (64), an alias `void* o2 = o` (6), a named `BsStation* hd` (6),
 *    unsigned and pointer-typed members (6), and the cancel written on the
 *    anchor member instead (97).  The head cannot get ecx in that order for a
 *    mechanical reason: eax holds the function-wide zero and its last use is
 *    the q[4] store, so eax is free by the time the head loads.  Only a zero
 *    store AFTER the head load keeps eax busy -- which is precisely what the
 *    committed spelling does.
 *  - On the committed order the lever is inert or harmful: cancelled pairs on
 *    `o` (nine anchors, at the call and between q[0] and q[1]) and on the head
 *    (eight anchors) give 5 when the anchor is a literal (it folds in the
 *    front end, so the body is unchanged) and 7..117 otherwise.
 *  Residual and marker unchanged; the 8 remains three instructions of
 *  scheduler slack in the argument load/push placement.
 * ========================================================================= */
// WIP-FUNCTION: LEGOLAND 0x0041a040  (218/218 insns, 683/683 bytes, 8 mismatches at idx 39..50: the `o` argument load/push threaded between the zero-stores)
void BoatingSchool_Add(void* o, Pos* pos)
{
    BsStation* st;
    BPosW      key;
    int        x;
    int        y;

    key.b.x = (unsigned char)pos->x;
    key.b.y = (unsigned char)pos->y;
    st = HeapAlloc_w(0x34);
    if (st == 0)
        return;

    st->key = key.w;
    st->ax = (unsigned char)((unsigned char)pos->x + (unsigned char)g_bs_dock_a.left + 2);
    st->ay = (unsigned char)((unsigned char)pos->y + (unsigned char)g_bs_dock_a.top + 2);
    st->bx = (unsigned char)((unsigned char)pos->x + (unsigned char)g_bs_dock_b.left + 2);
    st->by = (unsigned char)((unsigned char)pos->y + (unsigned char)g_bs_dock_b.top + 2);
    st->route = 0;
#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
    /* QUIRKS.md Q7: seeded to 9999 the tick handler never reaches its turn-
     * round at exactly 100 and a new school never animates until a save/load
     * rewrites the record with 0. Start where a loaded record starts. */
    st->frame = 0;
#else
    st->frame = 0x270f;
#endif
    st->backwards = 0;
    st->count = 0;
    st->take = 5;
    st->q[0] = 0;
    st->q[1] = 0;
    st->q[2] = 0;
    st->q[3] = 0;
    /* The list link is spelled BEFORE the last queue slot on purpose.  With
     * the (surely original) order `q[4] = 0; next = head; head = st;` VC6
     * puts the list head in eax -- killing the function-wide zero one
     * instruction early -- and the whole eax/ecx/edx assignment rotates from
     * the `by` computation onwards, for 146 mismatches.  Moving the two link
     * statements up one slot restores the original's register plan (head in
     * ecx, `o` in edx) and everything from AddBasicObject on is exact; the
     * only residual is three instructions of scheduling slack (VC6 hoists
     * the `o` argument load three slots and sinks its push four).  Zeroing a
     * freshly malloc'd record in a different order is not a semantic
     * change. */
    st->next = g_bs_stations;
    st->q[4] = 0;
    g_bs_stations = st;
    AddBasicObject(o, pos);

    BsWater_SetTile(pos->x + g_bs_dock_a.left + 2, pos->y + g_bs_dock_a.top + 2, 1, st);
    BsWater_SetTile(pos->x + g_bs_dock_b.left + 2, pos->y + g_bs_dock_b.top + 2, 4, st);

    for (y = g_bs_footprint.top; y <= g_bs_footprint.bottom; y++) {
        for (x = g_bs_footprint.left; x <= g_bs_footprint.right; x++) {
            if (x == g_bs_footprint.left)
                SetMapTile(pos->x + x, pos->y + y,
                           (unsigned short)(g_bs_water_tiles->codes[0] + 9));
            else if (x == g_bs_footprint.right)
                SetMapTile(pos->x + x, pos->y + y,
                           (unsigned short)(g_bs_water_tiles->codes[0] + 12));
            else
                SetMapTile(pos->x + x, pos->y + y, g_bs_water_tiles->codes[0]);
        }
    }
    SetMapTile(pos->x + g_bs_footprint.right, pos->y + g_bs_footprint.top,
               (unsigned short)(g_bs_water_tiles->codes[0] + 8));
    SetMapTile(pos->x + g_bs_footprint.right, pos->y + g_bs_footprint.bottom,
               (unsigned short)(g_bs_water_tiles->codes[0] + 7));
    SetMapTile(pos->x + g_bs_footprint.left + 4, pos->y + g_bs_footprint.bottom,
               (unsigned short)(g_bs_water_tiles->codes[0] + 4));
    SetMapTile(pos->x + g_bs_footprint.left + 4, pos->y + g_bs_footprint.top,
               (unsigned short)(g_bs_water_tiles->codes[0] + 1));
    SetMapTile(pos->x + g_bs_footprint.left + 5, pos->y + g_bs_footprint.bottom,
               (unsigned short)(g_bs_water_tiles->codes[0] + 11));
    SetMapTile(pos->x + g_bs_footprint.left + 5, pos->y + g_bs_footprint.top,
               (unsigned short)(g_bs_water_tiles->codes[0] + 10));
}

/* =========================================================================
 * 0x0041c130 -- BoatingSchoolWater_Remove (BOATING SCHOOL WATER cb_9c).
 *
 * Removing one square of boating-school water.  The cell under the removed
 * square decides which teardown runs: if the cell's object is NOT the water
 * class's shared instance (ObjDef +0xc4) the square belongs to the SCHOOL
 * itself, so the whole ride goes through BoatingSchool_Remove with a
 * stack-built MapObj stub carrying the school class; otherwise only this
 * piece of water is removed and its neighbourhood is re-stitched:
 *
 *   - BsWater_Probe on the removed cell gives the direction mask of the water
 *     arms that touched it (1 N, 2 E, 4 S, 8 W, five cells out) and the map
 *     square of the school that owns them;
 *   - BsWater_RemoveOne tears down this square's own water object and
 *     BoatingSchool_AddTake(owner, -1) takes one off the school's takings --
 *     the water squares are what the takings are measured against, so
 *     shrinking the lake makes the ride cheaper to run;
 *   - each arm that existed is re-probed and its tile rebuilt (BsWater_SetTile
 *     hands back the owning school's square, which BsWater_Relink then uses
 *     to re-attach the cell);
 *   - each DIAGONAL between two surviving arms is rebuilt the same way, but
 *     only where a water record actually exists there;
 *   - finally the school's boat route is rebuilt and, if the removed square
 *     was a school's own square, that school's route is recomputed from its
 *     two dock cells (+0x02/+0x03 and +0x04/+0x05) into +0x08.
 *
 * ORIGINAL BUG reproduced: the bounds-checked cell fetch can return NULL and
 * the very next instruction dereferences it, so removing a water square whose
 * map coordinates are off the map faults.  (The jungle cruise's copy of this
 * function, ridecb2.c 0x00436a40, carries exactly the same bug -- the two
 * were plainly written from one another.)
 *
 * `north` and `south` carry `volatile` as a pure CODEGEN LEVER: the original
 * re-loads and re-tests `north` at the NE guard and `south` at the SE guard
 * even though both are provably still set on every path that reaches them,
 * and nothing else stops VC6 from folding those tests away.  `east` must NOT
 * be volatile -- all eight combinations were measured and only
 * (north, east, south) = (volatile, plain, volatile) reproduces the
 * original's `mov eax,ebp / and eax,2 / mov [esp+18h],eax` for east and its
 * block layout for the four diagonals.
 *
 * CLOSED (was 331/332 with 38 scratch-register mismatches) by two levers,
 * both of which transfer to ridecb2.c's JungleCruiseWater_Remove:
 *
 *   1. A VOLATILE FLAG COMPUTED THROUGH A PLAIN SCALAR.  `north = mask & 1;
 *      if (north)` evaluates the rvalue into a scratch TEMPORARY and stores
 *      it, and that temporary takes a slot in VC6's eax/ecx/edx rotation, so
 *      every `lea` temp of the next two blocks came out one register on
 *      (ecx/edx/eax for the original's eax/ecx/edx).  `n0 = mask & 1;
 *      north = n0; if (n0)` makes the flag a variable def in eax outside the
 *      rotation; the code is byte-identical up to the tail and 38 -> 21.
 *   2. WIDEN BYTE ARGUMENTS INTO INT LOCALS BEFORE THE CALL, in declaration
 *      order.  `BuildRoute(st->ax, st->ay, st->bx, st->by)` evaluates the
 *      four u8 fields right-to-left straight into the pushes and puts the
 *      station walker in esi; reading them into `int ax, ay, bx, by` first
 *      moves the walker to edi and gives the original's bx, ay, [bx->esi],
 *      by, ax load order with the extra `mov esi,edx` (332 insns).  All 24
 *      read orders were measured; only the natural ax, ay, bx, by is exact
 *      (the others 4..12 X).  `unsigned char` parameters on the extern and a
 *      swapped key compare do nothing.
 * ========================================================================= */

/* The map grid the cell fetch walks (same layout as objmap2.c). */
typedef struct Cell {
    void*         obj;              /* +0x00 */
    unsigned char pad04[0x14 - 4];
} Cell;                             /* 0x14 */

typedef struct MapHdr {
    unsigned char  pad00[0x14];
    unsigned short width;           /* +0x14 */
    unsigned short height;          /* +0x16 */
} MapHdr;

typedef struct ObjDef {
    unsigned char pad00[0x0c];
    int           dx;               /* +0x0c  class origin offset */
    int           dy;               /* +0x10 */
    unsigned char pad14[0x24 - 0x14];
    signed char   ox;               /* +0x24  exit-cell offset (signed) */
    signed char   oy;               /* +0x25 */
    unsigned char pad26[0x3c - 0x26];
    Rect          rect;             /* +0x3c  the class footprint rect list */
    unsigned char pad50[0x64 - 0x50];
    void*         layer;            /* +0x64  the class's render layer */
    unsigned char pad68[0xc4 - 0x68];
    void*         c4;               /* +0xc4  the class's shared instance */
    unsigned char padc8[0xcc - 0xc8];
    struct RideInst* instances;     /* +0xcc  live instance list */
} ObjDef;

typedef struct MapObj {
    unsigned char pad00[0x0c];
    ObjDef*       cls;              /* +0x0c */
    unsigned char pad10[4];
} MapObj;

/* The edit / destroy cursor (objmap2.c's Cursor). */
typedef struct Cursor {
    unsigned char pad0000[0x1404];
    Pos           origin;           /* +0x1404 map cell the footprint hangs off */
    int           status;           /* +0x140c */
    int           error;            /* +0x1410 */
    Rect          rect;             /* +0x1414 footprint rect list */
    unsigned char pad1428[0x1828 - 0x1428];
    unsigned int  flags;            /* +0x1828 */
    int           f182c;            /* +0x182c */
    struct Cursor* next;            /* +0x1830 */
} Cursor;                           /* 0x1834 */

/* A boating-school water record: its own square at +0x00 and the school that
 * owns it at +0x02, the same {own, owner} pair the jungle cruise uses. */
typedef struct BsWater {
    BPosW          pos;             /* +0x00 */
    BPosW          owner;           /* +0x02 */
    unsigned char  pad04[0x10 - 4];
} BsWater;

extern MapHdr*    g_map;            /* 0x004bcbf4 (lpConfig) */
extern Cell**     g_map_rows;       /* 0x00801400 (GameMap) */
extern ObjDef*    g_bs_water_cls;   /* 0x0082adf0 BOATING SCHOOL WATER */
extern ObjDef*    g_bs_cls;         /* 0x0082c658 BOATING SCHOOL */

/* Tears the whole school down (the school's own cb_9c). */
extern void  BoatingSchool_Remove(MapObj* o, BPosW bp, Cursor* ctx);  /* 0x0041a530 */
/* Removes one water square's object. */
extern void  BsWater_RemoveOne(MapObj* o, BPosW bp, Cursor* ctx);     /* 0x0041c620 */
/* Probes the water around (x, y): returns the direction bitmask and stores
 * the owning school's map square in *owner. */
extern int   BsWater_Probe(int x, int y, BPosW* owner);               /* 0x0041c690 */
/* Re-attaches the cell at (x, y) to the school named by *owner. */
extern void  BsWater_Relink(int x, int y, BPosW* owner);              /* 0x0041bab0 */
/* Returns the water record covering the map square, or 0. */
extern BsWater* BsWater_FindAt(int x, int y);                         /* 0x0041c890 */
/* Adds `amount` to the takings of the school whose square is `key`. */
extern void  BoatingSchool_AddTake(BPosW key, int amount);            /* 0x0041b0d0 */
/* Re-lays the boat route of the school whose square is `key`. */
extern void  BoatingSchool_RebuildRoute(BPosW key);                   /* 0x0041caa0 */
/* Lays a boat route between two map squares; returns the route record. */
extern void* BoatingSchool_BuildRoute(int x0, int y0, int x1, int y1); /* 0x0041c8c0 */

/* The bounds-checked cell fetch every map accessor open-codes. */
static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

// FUNCTION: LEGOLAND 0x0041c130
void BoatingSchoolWater_Remove(MapObj* o, BPosW bp, Cursor* ctx)
{
    BsStation* st = g_bs_stations;
    Cell*      cell;
    int        mask;
    volatile int north;
    int        n0;          /* plain scalar the volatile flag is computed through: see note */
    int        s0;
    int        east;
    volatile int south;
    int        west;
    BPosW      owner;
    int        ax, ay, bx, by;  /* the dock bytes widened BEFORE the call: see note */

    {
        int cx = bp.b.x;
        int cy = bp.b.y;
        cell = MapCellAt(cx, cy);
    }
    /* Original: no null check -- an off-map square faults here. */
    if (cell->obj != g_bs_water_cls->c4) {
        MapObj stub;

        stub.cls = g_bs_cls;
        BoatingSchool_Remove(&stub, bp, ctx);
        return;
    }

    mask = BsWater_Probe(ctx->origin.x, ctx->origin.y, &owner);
    BsWater_RemoveOne(o, bp, ctx);
    BoatingSchool_AddTake(owner, -1);

    n0 = mask & 1;
    north = n0;
    if (n0) {
        int ny = ctx->origin.y - 5;
        int nx = ctx->origin.x;
        BPosW probe;
        int m = BsWater_Probe(nx, ny, &probe);

        BsWater_SetTile(nx, ny, m, &owner);
        BsWater_Relink(nx, ny, &owner);
    }
    east = mask & 2;
    if (east) {
        int nx = ctx->origin.x + 5;
        int ny = ctx->origin.y;
        BPosW probe;
        int m = BsWater_Probe(nx, ny, &probe);

        BsWater_SetTile(nx, ny, m, &owner);
        BsWater_Relink(nx, ny, &owner);
    }
    s0 = mask & 4;
    south = s0;
    if (s0) {
        int ny = ctx->origin.y + 5;
        int nx = ctx->origin.x;
        BPosW probe;
        int m = BsWater_Probe(nx, ny, &probe);

        BsWater_SetTile(nx, ny, m, &owner);
        BsWater_Relink(nx, ny, &owner);
    }
    west = mask & 8;
    if (west) {
        int nx = ctx->origin.x - 5;
        int ny = ctx->origin.y;
        BPosW probe;
        int m = BsWater_Probe(nx, ny, &probe);

        BsWater_SetTile(nx, ny, m, &owner);
        BsWater_Relink(nx, ny, &owner);
    }

    if (north && west) {
        int nx = ctx->origin.x - 5;
        int ny = ctx->origin.y - 5;

        if (BsWater_FindAt(nx, ny)) {
            BPosW probe;
            int m = BsWater_Probe(nx, ny, &probe);

            BsWater_SetTile(nx, ny, m, &owner);
            BsWater_Relink(nx, ny, &owner);
        }
    }
    if (north && east) {
        int nx = ctx->origin.x + 5;
        int ny = ctx->origin.y - 5;

        if (BsWater_FindAt(nx, ny)) {
            BPosW probe;
            int m = BsWater_Probe(nx, ny, &probe);

            BsWater_SetTile(nx, ny, m, &owner);
            BsWater_Relink(nx, ny, &owner);
        }
    }
    if (south && west) {
        int nx = ctx->origin.x - 5;
        int ny = ctx->origin.y + 5;

        if (BsWater_FindAt(nx, ny)) {
            BPosW probe;
            int m = BsWater_Probe(nx, ny, &probe);

            BsWater_SetTile(nx, ny, m, &owner);
            BsWater_Relink(nx, ny, &owner);
        }
    }
    if (south && east) {
        int nx = ctx->origin.x + 5;
        int ny = ctx->origin.y + 5;

        if (BsWater_FindAt(nx, ny)) {
            BPosW probe;
            int m = BsWater_Probe(nx, ny, &probe);

            BsWater_SetTile(nx, ny, m, &owner);
            BsWater_Relink(nx, ny, &owner);
        }
    }

    BoatingSchool_RebuildRoute(owner);
    while (st) {
        if (st->key == owner.w) {
            ax = st->ax;
            ay = st->ay;
            bx = st->bx;
            by = st->by;
            st->route = BoatingSchool_BuildRoute(ax, ay, bx, by);
            return;
        }
        st = st->next;
    }
}

/* =========================================================================
 * THE DRIVING SCHOOL ROADS MODULE (0x00412xxx..0x00414xxx)
 *
 * Everything below hangs off one global list of 4x4 road blocks at
 * 0x004cbeac.  The record is at least 0x18 bytes:
 *
 *   +0x00 next         +0x08 u16 group   the connected road this belongs to
 *   +0x0c int x        +0x10 int y       the block's origin, a multiple of 4
 *   +0x14 u8  kind     low nibble: 5 = lit junction (renderview.c draws
 *                      MAPLIGHTS there), 6 = the piece the placement rule
 *                      only accepts from the west
 * ========================================================================= */

typedef struct RoadRec {
    struct RoadRec* next;           /* +0x00 */
    unsigned char   pad04[4];
    unsigned short  group;          /* +0x08 */
    unsigned char   pad0a[2];
    int             x;              /* +0x0c */
    int             y;              /* +0x10 */
    unsigned char   kind;           /* +0x14 */
    unsigned char   pad15[3];
} RoadRec;

typedef struct WinRect { int left; int top; int right; int bottom; } WinRect;

extern RoadRec* g_road_list;            /* 0x004cbeac */

/* Returns the 4x4 road block whose ORIGIN is exactly (x, y), or 0. */
// FUNCTION: LEGOLAND 0x004125a0
RoadRec* Road_FindAt(int x, int y)
{
    RoadRec* r = g_road_list;

    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height) {
        while (r) {
            if (r->x == x && r->y == y)
                return r;
            r = r->next;
        }
    }
    return 0;
}

/* Returns the 4x4 road block COVERING (x, y), or 0.  This is the lookup the
 * rest of the game uses (renderview.c calls it GetRoadRecord). */
// FUNCTION: LEGOLAND 0x004125f0
RoadRec* GetRoadRecord(int x, int y)
{
    RoadRec* r = g_road_list;

    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height) {
        while (r) {
            if ((unsigned int)(x - r->x) < 4 && (unsigned int)(y - r->y) < 4)
                return r;
            r = r->next;
        }
    }
    return 0;
}

/* The eight neighbours of a road square, in RING order:
 *     0 N   1 NE   2 E   3 SE   4 S   5 SW   6 W   7 NW
 * Cardinals and diagonals are filled by two different helpers into the same
 * array, which is why each writes only every other slot. */

/* Fills the four CARDINAL ring slots with the road blocks five... four cells
 * out in each direction and returns how many of them exist.  `out` may be 0
 * (the count alone is wanted). */
// FUNCTION: LEGOLAND 0x004135d0
int Road_FindCardinals(int x, int y, RoadRec** out)
{
    int      n = 0;
    RoadRec* r;

    r = Road_FindAt(x, y - 4);
    if (r)
        n = 1;
    if (out)
        out[0] = r;
    r = Road_FindAt(x + 4, y);
    if (r)
        n++;
    if (out)
        out[2] = r;
    r = Road_FindAt(x, y + 4);
    if (r)
        n++;
    if (out)
        out[4] = r;
    r = Road_FindAt(x - 4, y);
    if (r)
        n++;
    if (out)
        out[6] = r;
    return n;
}

/* The same for the four DIAGONAL ring slots.
 *
 * Closed 2026-09-05 (Scope F, eleventh continuation) at 64/64 instructions,
 * 157/157 bytes, after ~130 further candidates on top of the earlier ~300.
 * The whole body is the cardinal twin's shape; the ONLY difference is the
 * third lookup, which must be spelled with an explicit `r == 0` arm FIRST
 * that carries a runtime no-op assignment to `r`:
 *
 *     if (!r) r = 0; else n++;
 *
 * Mechanism, measured step by step (docs/lanes/scope-f.md):
 *   - `n` is spilled to [esp+10h] across the first three calls (ebx/ebp/esi/
 *     edi hold out, y-4, x+4, y+4).  After the third call edi is free and n
 *     moves into it.  Written `if (r) n++;`, VC6 places that reload EAGERLY
 *     (`mov edi,[esp+18h]` before the `add esp,8`) and emits `je` over a bare
 *     `inc edi`: 27 X.  The original reloads LAZILY, in each arm.
 *   - Any non-empty else arm that survives to layout turns the taken arm into
 *     the original's `mov edi,[esp+10h] / inc edi` and puts the allocator's
 *     compensation copy `mov edi,[esp+10h]` on the other edge (a volatile
 *     self-read there does it at the cost of its own load: 24 X).
 *   - `r = 0` in that arm is kept by the front end (it does not use the
 *     branch's known-zero), so the arm is non-empty at layout; with the arm
 *     FIRST it emits nothing (0 X); with the arm SECOND it emits `xor eax,eax`
 *     (24 X, 158 B).  The cast spelling `(RoadRec*)0` and `!r` vs `r == 0` are
 *     inert.  The volatile `out` read the earlier two-variable form needed is
 *     not needed here and is gone.
 *   - Every identity statement tried in that arm (`n ^= 0`, `n = n * 1`,
 *     `memcpy(&n,&n,4)`, aggregate/union self-copies, `x = x`, `out = out`,
 *     `__assume`) folds before layout: 27 X.  `__noop` is a real call in
 *     this VC6.
 * The previous two-variable `result` form (2 X) is superseded. */
// FUNCTION: LEGOLAND 0x00413450
int Road_FindDiagonals(int x, int y, RoadRec** out)
{
    int      n = 0;
    RoadRec* r;

    r = Road_FindAt(x + 4, y - 4);
    if (r)
        n = 1;
    if (out)
        out[1] = r;
    r = Road_FindAt(x + 4, y + 4);
    if (r)
        n++;
    if (out)
        out[3] = r;
    r = Road_FindAt(x - 4, y + 4);
    /* A runtime no-op that the front end keeps: it gives the diamond its
     * non-empty fall-through arm, which is what makes VC6 reload `n` lazily
     * on each edge instead of eagerly after the call (see the note). */
    if (!r)
        r = 0;
    else
        n++;
    if (out)
        out[5] = r;
    r = Road_FindAt(x - 4, y - 4);
    if (r)
        n++;
    if (out)
        out[7] = r;
    return n;
}

/* Keeps a neighbouring road block only if it may be built onto from (x, y):
 * a kind-6 piece is accepted only from its EAST side (the caller's square is
 * the block immediately to its east); every other kind is always accepted. */
// FUNCTION: LEGOLAND 0x004134f0
RoadRec* Road_KeepNeighbour(int x, int y, RoadRec* r)
{
    if (r != 0 && ((r->kind & 0xf) != 6 || (r->x + 4 == x && r->y == y)))
        return r;
    return 0;
}

/* The four cardinal neighbours a road at (x, y) may actually JOIN, written
 * into the cardinal ring slots of `out`; returns how many there are. */
// FUNCTION: LEGOLAND 0x00413520
int Road_CardinalGroup(int x, int y, RoadRec** out)
{
    RoadRec* tmp[8];
    int      n = 0;
    RoadRec* w;

    Road_FindCardinals(x, y, tmp);
    tmp[0] = Road_KeepNeighbour(x, y, tmp[0]);
    if (tmp[0])
        n = 1;
    tmp[2] = Road_KeepNeighbour(x, y, tmp[2]);
    if (tmp[2])
        n++;
    tmp[4] = Road_KeepNeighbour(x, y, tmp[4]);
    if (tmp[4])
        n++;
    w = Road_KeepNeighbour(x, y, tmp[6]);
    if (w)
        n++;
    if (out) {
        out[0] = tmp[0];
        out[2] = tmp[2];
        out[4] = tmp[4];
        out[6] = w;
    }
    return n;
}

/* ---- the edit cursor and the road placement preview --------------------- */
extern Cursor  g_edit_cursor;          /* 0x007febc0 */
extern Pos     g_edit_cursor_origin;   /* 0x007fffc4 == g_edit_cursor.origin */
extern Rect    g_edit_cursor_rect;     /* 0x007fffd4 == g_edit_cursor.rect */
extern Cursor* g_edit_cursor_next;     /* 0x008003f0 == g_edit_cursor.next */
extern Cursor  g_road_preview;         /* 0x0082f760 the snapped-road preview */
extern Rect    g_road_preview_rect;    /* 0x004b4bf0 its static 4x4 footprint */

#ifndef LEGOLAND_PORTABLE
extern void  ScreenToMapRef(int sx, Pos* out, int sy);       /* 0x0045be90 */
#else
extern int ScreenToMapRef(int sx, Pos* out, int sy);       /* 0x0045be90 */
#endif
extern void  ValidateCursor(Cursor* c, ObjDef* cls);         /* 0x0045f810 */
extern int   CursorIsValid(Cursor* c);                       /* 0x0045f4b0 */
extern void  SetCursorError(Cursor* c, int code);            /* 0x0045f480 */
extern int   CheckForPeople(WinRect* r);                     /* 0x00485260 */
extern int   GetObjCost(ObjDef* cls);                        /* 0x00480da0 */
extern int   GetBrickCount(void);                            /* 0x004578e0 */

/* Snaps the cursor onto the 4x4 road grid and returns the road block the new
 * piece will be joined to, having moved the cursor to the free block next to
 * it (north neighbour -> the block below it, east neighbour -> the block to
 * its west, and so on).  A kind-6 piece is never joined to. */
// FUNCTION: LEGOLAND 0x00413e30
RoadRec* Road_SnapCursor(Cursor* c)
{
    RoadRec* result;
    RoadRec* here;
    RoadRec* n;
    RoadRec* e;
    RoadRec* s;
    RoadRec* w;

    here = GetRoadRecord(c->origin.x, c->origin.y);
    if (here) {
        c->origin.x = here->x;
        c->origin.y = here->y;
    }
    n = GetRoadRecord(c->origin.x, c->origin.y - 4);
    if (n && (n->kind & 0xf) == 6)
        n = 0;
    e = GetRoadRecord(c->origin.x + 4, c->origin.y);
    if (e && (e->kind & 0xf) == 6)
        e = 0;
    s = GetRoadRecord(c->origin.x, c->origin.y + 4);
    if (s && (s->kind & 0xf) == 6)
        s = 0;
    w = GetRoadRecord(c->origin.x - 4, c->origin.y);
    if (w && (w->kind & 0xf) == 6)
        w = 0;
    result = 0;
    if (n) {
        c->origin.x = n->x;
        c->origin.y = n->y + 4;
        result = n;
    } else if (e) {
        c->origin.x = e->x - 4;
        c->origin.y = e->y;
        result = e;
    } else if (s) {
        c->origin.x = s->x;
        c->origin.y = s->y - 4;
        result = s;
    } else if (w) {
        c->origin.x = w->x + 4;
        c->origin.y = w->y;
        result = w;
    }
    return result;
}

/* =========================================================================
 * 0x00413b50 -- Roads_CalcCursor (DRIVING SCHOOL ROADS cb_90).
 *
 * The placement-cursor calculator for a road block.  It snaps the cursor to
 * the 4x4 road grid, validates it, refuses the placement when somebody is
 * standing in the footprint (error 4 = off the map, 3 = a person is there)
 * or the park cannot afford it (error 2), and then enforces the 2x2 rule:
 * the eight ring neighbours are collected and the block is refused (error 14)
 * when any three CONSECUTIVE ring entries belong to the same road group.
 *
 * Finally, when a road WAS found to join onto, the second preview cursor at
 * 0x0082f760 is given that block's origin and chained onto the edit cursor,
 * so the join is drawn as well.
 *
 * ORIGINAL BUG reproduced: `group` is only assigned from a cardinal
 * neighbour, and its fall-through path reads the frame home VC6 shares with
 * the spilled `snap` pointer -- i.e. the low half of a pointer used as a road
 * group id.  It is unreachable in practice (Road_CardinalGroup returning
 * non-zero guarantees one of the four cardinals is set), which is why it
 * survived.
 *
 * CLOSED (was 225/226, the one miss at idx 97): that uninitialised read is
 * `mov si, WORD ptr [esp+0x44]`, and a plain `unsigned short group` local
 * emits `mov esi, DWORD ptr` there -- VC6 promotes a u16 local whose every
 * use is a promoted compare to int-width storage, so its undefined-value
 * load is 4 bytes wide.  Wrapping the u16 in a one-member struct
 * (`group.id`) keeps the 16-bit storage class (aggregates are never
 * promoted) while VC6 still enregisters it in si and still reads the
 * undefined path from the same slot, now as a word.  Ruled out first:
 * `short`, scoping the local inside the validity block, an explicit
 * `else group = group;`, and routing the compares through a static __inline
 * helper taking `unsigned short` by value (all left idx 97 unchanged).
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x00413b50
void Roads_CalcCursor(MapObj* o, int sx, int sy)
{
    WinRect        area;
    RoadRec*       ring[8];
    ObjDef*        cls = o->cls;
    RoadRec*       snap;
    int            mask;
    struct { unsigned short id; } group;   /* one-member struct: see note */
    int            cost;
    int            people;

    mask = 0;
    g_edit_cursor_rect = cls->rect;
    g_edit_cursor_next = 0;
    ScreenToMapRef(sx, &g_edit_cursor_origin, sy);
    snap = Road_SnapCursor(&g_edit_cursor);
    ValidateCursor(&g_edit_cursor, cls);
    if (!CursorIsValid(&g_edit_cursor))
        return;

    area.left = g_edit_cursor_rect.left + g_edit_cursor_origin.x;
    area.top = g_edit_cursor_origin.y + g_edit_cursor_rect.top;
    area.right = g_edit_cursor_origin.x + g_edit_cursor_rect.right;
    area.bottom = g_edit_cursor_origin.y + g_edit_cursor_rect.bottom;
    people = CheckForPeople(&area);
    if (people != -1) {
        if (people != 1) {
            cost = GetObjCost(cls);
            if (GetBrickCount() < cost) {
                SetCursorError(&g_edit_cursor, 2);
            } else {
            if (CursorIsValid(&g_edit_cursor)) {
                if (Road_CardinalGroup(g_edit_cursor_origin.x,
                                       g_edit_cursor_origin.y, ring) == 0)
                    goto reject;
                if (ring[0])
                    group.id = ring[0]->group;
                else if (ring[2])
                    group.id = ring[2]->group;
                else if (ring[4])
                    group.id = ring[4]->group;
                else if (ring[6])
                    group.id = ring[6]->group;
                Road_FindCardinals(g_edit_cursor_origin.x, g_edit_cursor_origin.y, ring);
                Road_FindDiagonals(g_edit_cursor_origin.x, g_edit_cursor_origin.y, ring);
                if (ring[0] && ring[0]->group == group.id)
                    mask = 1;
                if (ring[1] && ring[1]->group == group.id)
                    mask |= 2;
                if (ring[2] && ring[2]->group == group.id)
                    mask |= 4;
                if (ring[3] && ring[3]->group == group.id)
                    mask |= 8;
                if (ring[4] && ring[4]->group == group.id)
                    mask |= 0x10;
                if (ring[5] && ring[5]->group == group.id)
                    mask |= 0x20;
                if (ring[6] && ring[6]->group == group.id)
                    mask |= 0x40;
                if (ring[7] && ring[7]->group == group.id)
                    mask |= 0x80;
                if ((mask & 7) == 7 || (mask & 0x1c) == 0x1c ||
                    (mask & 0x70) == 0x70 || (mask & 0xc1) == 0xc1) {
        reject:
                    SetCursorError(&g_edit_cursor, 14);
                }
            }

            if (snap != 0 && CursorIsValid(&g_edit_cursor)) {
                g_road_preview.rect = g_road_preview_rect;
                g_edit_cursor_next = &g_road_preview;
                g_road_preview.origin.x = snap->x;
                g_road_preview.origin.y = snap->y;
                g_road_preview.next = 0;
                g_road_preview.flags = 0x2034;
            } else {
                g_edit_cursor_next = 0;
            }
                return;
            }
        } else {
            SetCursorError(&g_edit_cursor, 3);
        }
    } else {
        SetCursorError(&g_edit_cursor, 4);
    }
}

/* =========================================================================
 * 0x0041a720 -- BoatingSchool_Tick (BOATING SCHOOL cb_a8 / the class's
 * per-frame service handler).  Structurally the twin of ridecb2.c's
 * JungleCruise_Tick and castleobj.c's DrivingSchool_TickRiders.
 *
 * It runs in two halves.
 *
 * (1) THE QUEUE.  Each live instance of the class (ObjDef +0xcc) carries the
 * visitor at +0x08 and the school's map square at +0x0c.  The visitor's stage
 * byte (+0x60) drives a seven-way switch, but only while its action word
 * (+0x0e) is zero -- i.e. while it is not already busy:
 *
 *   0  join / shuffle up the five-deep queue.  A visitor not in the queue is
 *      appended at slot 4 (and thrown off the ride when the queue is full or
 *      slot 4 is taken); one already in it steps forward when the slot ahead
 *      is free, and reaching slot 0 advances it to stage 1.  Either way it is
 *      flagged 8 and walked to that slot's pixel offset (g_bs_seat_ofs, a
 *      table indexed BACKWARDS from 0x004b52b0) with action 7.
 *   1  at the head of the queue: only when the school has a laid route and
 *      its takings are at least six per water square does it ask
 *      BoatingSchool_TryLaunch for a boat.  On success the visitor sits down
 *      (sit animation, frame 0, flag 0x80, stage++), leaves the queue
 *      (q[0] = 0, count--) and the launch sample is played from it, pitched
 *      up by 10.
 *   2  out on the water -- nothing to do here.
 *   3  getting off: walk animation, clear the sitting flag, snap the visitor
 *      onto the map four cells west / two cells south of the school's exit
 *      cell (ObjDef +0x24/+0x25, SIGNED bytes) and walk it to (-0xc0, +0x240)
 *      of that cell.
 *   4  walk on to (-0xc0, +0x80) of the exit cell, stage++.
 *   5  walk to the centre of the exit cell (+0x80, +0x80), stage++, and fade
 *      the visitor's samples out over 90 ticks.
 *   6  clear the "using this ride" flag and leave the ride.
 *
 * (2) THE BUILDING ANIMATION.  Every station record's frame counter (+0x0c)
 * is stepped and pushed into the class's shared .lls, counting BACKWARDS
 * (nframes - frame) once the record's direction flag (+0x10) is set.
 *
 * ORIGINAL BUG reproduced: BoatingSchool_Add seeds `frame` to 9999 while this
 * loop only turns the animation round when the counter is exactly 100, so a
 * freshly built school runs its counter up from 10000 for ever -- past the
 * sprite's frame count, so LLSSetFrame is never called and the building's
 * animation never plays.  Only a save/load (which rewrites the record) or a
 * counter that happens to be below 100 gets it going.
 *
 * The two SoundSource locals do NOT share a frame slot (case 1's is at
 * frame-0x20, case 5's at frame-0x10) even though the cases are disjoint --
 * the same shape castleobj.c records for the driving school.
 * HOW THE TWO ARMS OF CASE 0 JOIN.  The walk-to-the-slot tail (`flags |= 8`
 * through NewDirForAction) is written out TWICE in the source, once at the
 * end of the enqueue arm and once at the end of the shuffle arm; VC6 cross-
 * jumps the two copies back into the single block at 0x41a82d, so the
 * emitted code is the same either way.  Written once after the `if`, the
 * enqueue's `st->count++` is the LAST scratch temporary of its own basic
 * block and VC6 gives it ecx; with the tail inline it is no longer last and
 * it takes the original's edx, which is the whole of the old residual --
 * and the queue-full arm then keeps its own `mov edx,[g_bs_cls] / push ebp
 * / push edx` instead of being swallowed by case 6's identical call.  See
 * the lever note below the struct declarations.
 * ========================================================================= */

/* A person as this handler sees it (same record as ridecb1.c/ridecb2.c). */
typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short action;          /* +0x0e  low-level AI state (0 = idle) */
    unsigned char  pad10[0x24 - 0x10];
    int            tx;              /* +0x24  walk target (24.8) */
    int            ty;              /* +0x28 */
    unsigned char  pad2c[0x60 - 0x2c];
    unsigned char  stage;           /* +0x60  ride state machine */
    unsigned char  pad61;
    unsigned short flags;           /* +0x62  8 = on this ride, 0x80 = seated */
    unsigned char  pad64[0x68 - 0x64];
    int            x;               /* +0x68  world position (24.8) */
    int            y;               /* +0x6c */
    unsigned char  pad70[2];
    unsigned char  speed;           /* +0x72 */
    unsigned char  dir8;            /* +0x73  heading, 0x20 per octant */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[4];         /* +0x98  the move line CalcMoveLine fills */
} Bloke;

/* One live use of the class: the visitor and the school's map square. */
typedef struct RideInst {
    struct RideInst* next;          /* +0x00 */
    unsigned char    pad04[4];
    Bloke*           bloke;         /* +0x08 */
    BPosW            key;           /* +0x0c */
    unsigned char    pad0e[2];
} RideInst;

/* money.c's SoundSource; kind 1 = "sourced at a person", the person at +4. */
typedef struct BlokeSoundSource {
    int    kind;                    /* +0x00 */
    Bloke* bloke;                   /* +0x04 */
    int    x;                       /* +0x08 */
    int    y;                       /* +0x0c */
} BlokeSoundSource;

/* A queue slot's pixel offset, indexed BACKWARDS: g_bs_seat_ofs[-slot]. */
typedef struct SeatOfs { int dx; int dy; } SeatOfs;

/* The class's shared .lls; only its frame count is read here. */
typedef struct LLSprite {
    unsigned char pad00[0x10];
    short         nframes;          /* +0x10 */
} LLSprite;

extern LLSprite* GetLLSForSprite(void* sprite);              /* 0x00441e80 */
extern void  LLSSetFrame(LLSprite* s, int frame);            /* 0x0047d5a0 */
extern void  BlokeSitAnim(Bloke* b);                         /* 0x00440780 */
extern void  BlokeSetFrame(Bloke* b, int frame);             /* 0x00440870 */
extern void  BlokeWalkAnim(Bloke* b);                        /* 0x00440910 */
/* Declared with FIVE int parameters rather than two by-value Pos: identical
 * ABI, and it keeps the caller from building the two aggregates. */
#ifndef LEGOLAND_PORTABLE
extern int   CalcMoveLine(int fx, int fy, int tx, int ty, void* path); /* 0x00480740 */
#else
/* Identical ABI on x86 only: on wasm32 a by-value struct travels as a pointer
 * to a copy, so the flattened declaration is a different function type from
 * bnvmove.c's definition and the link traps. Every call here already goes
 * through the scope-local `MoveLineFn` cast to the by-value shape, so the
 * definition's prototype makes those casts identities. */
extern int   CalcMoveLine(Pos from, Pos to, void* path);              /* 0x00480740 */
#endif
extern int   NewDirForAction(Bloke* b, unsigned char dir);   /* 0x004833d0 */
extern void  RemoveBlokeFromRide(ObjDef* cls, RideInst* r);  /* 0x0048a100 */
extern void* PlayInstanceOfSample(void* s, int a, int b, BlokeSoundSource* q); /* 0x00496d20 */
extern void  AdjustPSampleFreq(void* handle, int delta);     /* 0x00492aa0 */
extern void  UnSourceAndFadeAllSamplesFromSource(BlokeSoundSource* s, int f); /* 0x00496c80 */
/* Steps every boat one place along its route (every 0x50 ticks). */
extern void  BoatingSchool_AdvanceBoats(void);               /* 0x00419300 */
/* Re-lays the water tiles / boat sprites for the frame. */
extern void  BoatingSchool_UpdateWater(int mode);            /* 0x00418fe0 */
/* How many water squares this school has. */
extern int   BoatingSchool_CountWater(BsStation* st);        /* 0x004192d0 */
/* Puts a visitor into a boat at the school's square. */
extern int   BoatingSchool_TryLaunch(BPosW key, Bloke* b);   /* 0x00418e60 */

extern void*    g_bs_sprite;         /* 0x0082ae00 the building's sprite */
extern int      g_bs_tick;           /* 0x004cc08c the 0x50-tick counter */
extern SeatOfs  g_bs_seat_ofs[];     /* 0x004b52b0, indexed backwards */
extern void*    g_bs_launch_sample;  /* 0x004b52c8 */


/* =========================================================================
 * 0x0041a720 -- BoatingSchool_Tick (BOATING SCHOOL cb_a8 / iface slot 8).
 *
 * EXACT (2026-09-06): 358/358 instructions, 1164/1164 bytes, mismatch 0,
 * no escapes, whole-file audit PASS with ten exact bodies, /W3 clean.
 *
 * THE LEVER THAT CLOSED IT, and the rule behind it.  VC6 SP3 hands out the
 * scratch registers of a basic block so that the temporary which DIES LAST
 * in that block gets ecx and earlier-dying ones get edx (then eax, when eax
 * is not already carrying a live variable).  Measured in this very block:
 * `st->count++; st->take++;` gives count=edx / take=ecx and
 * `st->take++; st->count++;` gives take=edx / count=ecx, i.e. the choice is
 * positional, not a rotation and not a CSE effect.  A temporary in a
 * NEIGHBOURING block -- the shuffle arm, the queue-full arm, the join --
 * changes nothing; only a later temporary in the SAME block does.
 * The enqueue arm holds one temporary, so written the obvious way (the
 * walk-to-the-slot tail once, after the `if`) it dies last and takes ecx.
 * Writing that tail out in BOTH arms puts the tail's own temporaries after
 * the increment in the same block, the increment stops being last, and it
 * takes the original's edx.  VC6 then cross-jumps the two copies back into
 * one block (they hold two calls, so by the tail-duplication threshold the
 * copies are jumped to rather than kept) and the emitted code is identical
 * to the original index for index.
 *
 * THREE `volatile` SHIMS AND ONE NAMED POINTER CAME OUT WITH IT, as every
 * earlier note predicted they must: the count-read shim
 * (`st->count = *(volatile int*)&st->count + 1`), the `b->action = 7`
 * volatile store in case 0, and the named `Pos* world` in case 0 are all
 * inert now and are gone.  The body contains no `volatile` at all.  With
 * the tail written once, the clean body is 11 mismatches; the duplication
 * alone closes all eleven.  Re-adding the count shim ON TOP of the
 * duplication costs 288 -- the two are mutually exclusive.
 *
 * STILL LOAD-BEARING.  The scope-local `MoveLineFn` function-pointer type
 * (two by-value `Pos` at the five CalcMoveLine sites, which is the ABI the
 * original uses without changing the shared five-scalar extern) is worth
 * 17 at the case-5 site, 42 at case 4, 157 at case 3, 288 at either case-0
 * site and 265 for all five.  The animation loop's failure-first arms
 * (`if (!st->backwards) ... else ...`) are unchanged.
 *
 * EQUIVALENT SPELLINGS, both also exact, kept here so nobody re-derives
 * them: the guard as an early exit (`if (st->count == 5 || st->q[4] != 0)
 * { RemoveBlokeFromRide(...); break; }` before the enqueue, tail duplicated
 * in both arms), and the tail written inside the enqueue arm with the
 * queue-full arm unindented after it and the shuffle path falling out to a
 * second copy.  What does NOT work: duplicating only part of the tail
 * (`b->flags |= 8` alone 3, flags + the tx line 291); the split-guard form
 * with the call written twice (12); `if (i != 5) shuffle else enqueue`
 * (82-297).
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x0041a720
void BoatingSchool_Tick(void)
{
    typedef int (__cdecl* MoveLineFn)(Pos, Pos, void*);
    RideInst*        next;
    LLSprite*        lls;
    BPosW            key;
    BlokeSoundSource src;
    ObjDef*          def = g_bs_cls;
    RideInst*        inst = def->instances;
    BsStation*       st;
    Bloke*           b;
    int              slot;
    int              i;
    void*            h;

    lls = GetLLSForSprite(g_bs_sprite);
    if (++g_bs_tick == 0x50) {
        g_bs_tick = 0;
        BoatingSchool_AdvanceBoats();
    }
    BoatingSchool_UpdateWater(0);

    while (inst) {
        st = g_bs_stations;
        next = inst->next;
        key = inst->key;
        while (st) {
            if (st->key == key.w)
                break;
            st = st->next;
        }
        b = inst->bloke;
        if (b->action == 0) {
            switch (b->stage) {
            case 0:
                slot = 4;
                for (i = 0; i < 5; i++) {
                    if (st->q[i] == b) {
                        slot = i;
                        break;
                    }
                }
                /* The walk-to-the-slot tail is written out in BOTH arms; VC6
                 * cross-jumps the two copies back into one block (they hold
                 * two calls, so the copies are jumped to rather than kept).
                 * Written once after the `if`, the enqueue's `st->count++`
                 * is the last scratch temporary of its own block and takes
                 * ecx; with the tail inline it is no longer last and takes
                 * the original's edx, and the queue-full arm keeps its own
                 * `mov edx,[g_bs_cls] / push ebp / push edx` instead of
                 * being swallowed by case 6's identical call. */
                if (i == 5) {
                    if (st->count != 5 && st->q[4] == 0) {
                        st->q[slot] = b;
                        st->count++;
                    } else {
                        RemoveBlokeFromRide(g_bs_cls, inst);
                        break;
                    }
                    b->flags |= 8;
                    b->tx = ((g_bs_cls->dx + key.b.x) << 8) + g_bs_seat_ofs[-slot].dx;
                    b->ty = ((g_bs_cls->dy + key.b.y) << 8) + g_bs_seat_ofs[-slot].dy;
                    b->dir8 = (unsigned char)(((MoveLineFn)CalcMoveLine)(*(Pos*)&b->x, *(Pos*)&b->tx, &b->path) + 0x10);
                    b->action = 7;
                    NewDirForAction(b, (unsigned char)((b->dir8 >> 5) + 3));
                    break;
                } else {
                    void* front = st->q[slot - 1];
                    b = st->q[slot];
                    if (front != 0)
                        break;
                    st->q[slot - 1] = b;
                    st->q[slot] = 0;
                    slot--;
                    if (slot == 0)
                        b->stage++;
                    b->flags |= 8;
                    b->tx = ((g_bs_cls->dx + key.b.x) << 8) + g_bs_seat_ofs[-slot].dx;
                    b->ty = ((g_bs_cls->dy + key.b.y) << 8) + g_bs_seat_ofs[-slot].dy;
                    b->dir8 = (unsigned char)(((MoveLineFn)CalcMoveLine)(*(Pos*)&b->x, *(Pos*)&b->tx, &b->path) + 0x10);
                    b->action = 7;
                    NewDirForAction(b, (unsigned char)((b->dir8 >> 5) + 3));
                    break;
                }
            case 1:
                if (b != st->q[0])
                    break;
                if (st->route == 0)
                    break;
                if (st->take < 6 * BoatingSchool_CountWater(st))
                    break;
                if (!BoatingSchool_TryLaunch(key, b))
                    break;
                BlokeSitAnim(b);
                BlokeSetFrame(b, 0);
                st->q[0] = 0;
                st->count--;
                b->flags |= 0x80;
                b->stage++;
                src.kind = 1;
                src.bloke = b;
                h = PlayInstanceOfSample(g_bs_launch_sample, 1, 1, &src);
                AdjustPSampleFreq(h, 0xa);
                break;
            case 3:
                BlokeWalkAnim(b);
                BlokeSetFrame(b, 0);
                b->flags &= (unsigned short)~0x80;
                b->x = ((int)g_bs_cls->ox + key.b.x - 4) << 8;
                b->y = ((int)g_bs_cls->oy + key.b.y + 2) << 8;
                b->speed = 0xa;
                b->tx = (((int)g_bs_cls->ox + key.b.x) << 8) - 0xc0;
                b->ty = (((int)g_bs_cls->oy + key.b.y) << 8) + 0x240;
                b->dir8 = (unsigned char)(((MoveLineFn)CalcMoveLine)(*(Pos*)&b->x, *(Pos*)&b->tx, &b->path) + 0x10);
                b->action = 7;
                NewDirForAction(b, (unsigned char)((b->dir8 >> 5) + 3));
                b->stage++;
                break;
            case 4:
                b->tx = (((int)g_bs_cls->ox + key.b.x) << 8) - 0xc0;
                b->ty = (((int)g_bs_cls->oy + key.b.y) << 8) + 0x80;
                b->dir8 = (unsigned char)(((MoveLineFn)CalcMoveLine)(*(Pos*)&b->x, *(Pos*)&b->tx, &b->path) + 0x10);
                b->action = 7;
                NewDirForAction(b, (unsigned char)((b->dir8 >> 5) + 3));
                b->stage++;
                break;
            case 5:
                b->tx = (((int)g_bs_cls->ox + key.b.x) << 8) + 0x80;
                b->ty = (((int)g_bs_cls->oy + key.b.y) << 8) + 0x80;
                b->dir8 = (unsigned char)(((MoveLineFn)CalcMoveLine)(*(Pos*)&b->x, *(Pos*)&b->tx, &b->path) + 0x10);
                b->action = 7;
                NewDirForAction(b, (unsigned char)((b->dir8 >> 5) + 3));
                b->stage++;
                {
                    BlokeSoundSource src2;
                    src2.kind = 1;
                    src2.bloke = b;
                    UnSourceAndFadeAllSamplesFromSource(&src2, -0x5a);
                }
                break;
            case 6:
                b->flags &= (unsigned short)~8;
                RemoveBlokeFromRide(g_bs_cls, inst);
                break;
            }
        }
        inst = next;
    }

    for (st = g_bs_stations; st; st = st->next) {
        st->frame++;
        if (st->frame <= lls->nframes) {
            /* Spelled failure-first: the original lays the counting-down
             * arm out of line (`jne`) and the nframes-frame arm inline. */
            if (!st->backwards)
                LLSSetFrame(lls, lls->nframes - st->frame);
            else
                LLSSetFrame(lls, st->frame);
        }
        if (st->backwards == 0 && st->frame == 100) {
            st->frame = 0;
            st->backwards = 1;
        }
    }
}
