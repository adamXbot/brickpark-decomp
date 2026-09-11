/* LEGOLAND -- ride callback cluster at 0x0043xxxx, third block: the JUNGLE
 * CRUISE station's own handlers, the MONKEY TREE updater, the OCTOPUS CAFE
 * teardown and the MECHANICS HUT.
 *
 * Every function here is a slot of the ObjDef callback table that
 * SetCustomCallbacks (screen.c 0x00452c20) installs by class name, so each
 * one's class and slot is read straight out of that dispatcher:
 *
 *   addr        class                     slot     what it really is
 *   0x00434f90  JUNGLE CRUISE             cb_98    JungleCruise_Add
 *   0x00433d90  JUNGLE CRUISE MONKEY TREE cb_90    MonkeyTree_CalcCursor
 *   0x00431520  OCTOPUS CAFE              cb_ac    OctopusCafe_Destroy
 *   0x00435230  JUNGLE CRUISE             cb_94    JungleCruise_CalcCursor2
 *   0x0043d580  MECHANICS HUT             cb_b0    MechanicsHut_Draw
 *   0x0043d2f0  MECHANICS HUT             cb_a8    MechanicsHut_Tick
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  None of
 * these is exported; every extent was taken from the disassembly by control
 * flow (tools/audit.py).  Struct field OFFSETS, record sizes and global
 * addresses are load-bearing; the names are ours.  Types are declared LOCALLY
 * on purpose (legoland.h is owned elsewhere) and mirror the ones in
 * ridecb2.c / ridecb7.c / junglecruise.c, which own the rest of this ride.
 *
 * =========================================================================
 * NEW: THE JUNGLE CRUISE STATION'S TWO RIVER BLOCKS (0x004b7260 / 0x004b7278)
 *
 * Two const Rects in .rdata, chained through Rect +0x10:
 *
 *     g_jc_dock_a  @0x004b7278 = { 0,  0, 4, 4, next = &g_jc_dock_b }
 *     g_jc_dock_b  @0x004b7260 = { 0, -5, 4,-1, next = 0 }
 *
 * i.e. the 5x5 river block the station stands on and the 5x5 block five cells
 * NORTH of it.  JungleCruise_CalcCursor2 is what chains them (and parks the
 * head in g_jc_dock_list @0x00629c50) before validating the footprint, and
 * JungleCruise_Add turns their CENTRES -- (left+2, top+2) -- into the
 * station's route start (+0x02/+0x03) and route end (+0x04/+0x05) squares.
 * That is the concrete form of the rule junglecruise.c states abstractly:
 * the start square reads as a NORTH link (mask 1) and the end square as a
 * SOUTH link (mask 4), so the river joins the building at both ends.
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

/* One JUNGLE CRUISE station (0x44 bytes; full record in ridecb2.c). */
typedef struct JcStation {
    BPosW          pos;             /* +0x00  the station's own map square */
    unsigned char  ax;              /* +0x02  route START x */
    unsigned char  ay;              /* +0x03  route START y */
    unsigned char  bx;              /* +0x04  route END x */
    unsigned char  by;              /* +0x05  route END y */
    unsigned char  pad06[2];
    void*          route;           /* +0x08  the laid boat route */
    int            best;            /* +0x0c  starts at 9999 */
    int            f10;             /* +0x10 */
    int            count;           /* +0x14  visitors in the queue */
    void*          blokes[5];       /* +0x18  the five queue slots */
    int            timer;           /* +0x2c  ticks until the next dispatch */
    void*          riders[3];       /* +0x30  the party waiting to board */
    struct JcStation* next;         /* +0x3c */
    int            take;            /* +0x40  the ride's accumulated value */
} JcStation;                        /* 0x44 */

extern JcStation* g_jc_stations;    /* 0x00629c3c */

/* The map rectangle the ride's own tiles cover (the x loop stops one short of
 * `right` while the y loop includes `bottom` -- an original asymmetry). */
extern Rect g_jc_area;              /* 0x00629c40 */

/* The station's two 5x5 river blocks (see the header note). */
extern Rect g_jc_dock_a;            /* 0x004b7278 */
extern Rect g_jc_dock_b;            /* 0x004b7260 */

/* The ride's tileset handle; its first tile id is two indirections in.  It is
 * re-read at every call site (never cached) because SetMapTile may move the
 * tile tables -- the original reloads 0x0081cb58 each time. */
extern void* g_jc_tsm;              /* 0x0081cb58 */
#define JC_TILE0  (**(unsigned short**)((char*)g_jc_tsm + 4))

/* An object class descriptor (the 0xd0-byte ODF record); only its footprint
 * rect matters here. */
typedef struct ObjDef {
    unsigned char pad00[0x3c];
    Rect          rect;             /* +0x3c */
    unsigned char pad50[0xd0 - 0x50];
} ObjDef;

/* A placed map object; only +0x0c (the class) matters here. */
typedef struct MapObj {
    unsigned char pad00[0x0c];
    ObjDef*       cls;              /* +0x0c */
    unsigned char pad10[4];
} MapObj;

/* The edit / preview cursor (objmap2.c's Cursor). */
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

extern Cursor  g_edit_cursor;          /* 0x007febc0 */
extern Pos     g_edit_cursor_origin;   /* 0x007fffc4 == g_edit_cursor.origin */
extern Rect    g_edit_cursor_rect;     /* 0x007fffd4 == g_edit_cursor.rect */
extern Cursor* g_edit_cursor_next;     /* 0x008003f0 == g_edit_cursor.next */

/* The JUNGLE CRUISE MONKEY TREE class's own four preview cursors, one per
 * river arm the piece can hang off. */
extern Cursor g_jctree_cursors[4];     /* 0x00622320, stride 0x1834 */

#ifndef LEGOLAND_PORTABLE
extern void  ScreenToMapRef(int sx, Pos* out, int sy);       /* 0x0045be90 */
#else
extern int ScreenToMapRef(int sx, Pos* out, int sy);       /* 0x0045be90 */
#endif
extern void  DefaultCursor(Cursor* c);                       /* 0x0045a390 */
extern void  ValidateCursor(Cursor* c, ObjDef* cls);         /* 0x0045f810 */
extern int   CursorIsValid(Cursor* c);                       /* 0x0045f4b0 */
extern void  ResetCursorFootprint(Cursor* c);                /* 0x0045f460 */
extern void  SetCursorError(Cursor* c, int code);            /* 0x0045f480 */

/* Probes the jungle-cruise river around (x, y): returns the bitmask of the
 * four arms that exist (1 N, 2 E, 4 S, 8 W, five map cells out) and stores
 * the owning station's map square in *owner. */
extern int   JungleCruise_ProbeRiver(int x, int y, void* owner); /* 0x00436fb0 */

/* ---- the river subsystem (LEGOLAND/junglecruise.c) ---------------------- */
extern void  JungleCruise_UpdateRiverTile(int x, int y, int mask, void* owner); /* 0x00436dc0 */

/* ---- other subsystems --------------------------------------------------- */
#ifndef LEGOLAND_PORTABLE
extern void  AddBasicObject(void* o, Pos* p);                /* 0x0045efe0 */
#else
extern void AddBasicObject(void* ll_obj, void* ll_pos, void* ll_ctx);                /* 0x0045efe0 */
#define AddBasicObject(_a1, _a2) AddBasicObject((_a1), (_a2), 0)
#endif
extern void  SetMapTile(int x, int y, unsigned short tile);  /* 0x00461780 */
extern void* HeapAlloc_w(unsigned int size);                 /* 0x0049e4ff */

/* =========================================================================
 * 0x00434f90 -- JungleCruise_Add (JUNGLE CRUISE cb_98).
 *
 * Allocates the station record, seeds it (dispatch timer 150 ticks, ride
 * value 3, +0x0c = 9999) and pushes it on g_jc_stations, then joins the
 * river at both ends and paints the building's own tiles.
 *
 * THE OWNER ARGUMENT IS THE RECORD ITSELF.  UpdateRiverTile takes a POINTER
 * to the owning station's packed square; here the station record is handed
 * over directly, because its +0x00 IS that square.  So the two river blocks
 * are stamped with this station the moment it is created.
 *
 * The tile paint walks g_jc_area with the ride's own asymmetry -- the x loop
 * stops one column short of `right`, the y loop includes `bottom` -- and only
 * the first and last columns get an edge tile (+9 west, +0xc east); there is
 * no north/south row case at all, unlike the monkey fish.
 * ========================================================================= */

void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)

/* CLOSED (scope F continuation, 2026-09-05): 141/141 instructions, 438/438
 * bytes, strict/rb/ob 0/0/0, audit [OK].  The residual was a reconstruction
 * error, not an allocation floor: the five queue slots and the three rider
 * slots are cleared by two constant-count `for` loops --
 *
 *     for (x = 0; x < 5; x++) st->blokes[x] = 0;
 *     for (x = 0; x < 3; x++) st->riders[x] = 0;
 *
 * -- not by five plus three assignments and not by memset.  VC6 SP3 expands
 * a constant-count array fill as a FILL IDIOM (the same lowering that gives
 * `rep stosd` for CoasterShades_Init's 0x400-entry fill): six or more
 * elements become `rep stosd`, five or fewer become inline stores, and the
 * idiom materialises ITS OWN zero register (`xor r,r`) with base+displacement
 * addressing (`mov [esi+0x30],ecx`), no `lea`.  Each fill loop is its own
 * zero node created after CSE, so two fill loops are two zero webs: here the
 * blokes' zero coalesces with the function-wide web in eax and the riders'
 * zero is the original's `xor ecx,ecx` at index 42.  With the riders' zero
 * out of eax, web1 dies at blokes[4], the `o` argument lands in eax at 47 and
 * the head load in edx -- the whole 32..53 window and the post-call rotation.
 * Measured in isolation (scratchpad micro-test fill.c/fill2.c): a 5-fill is
 * `xor ecx,ecx` + five `[eax+disp]` stores; a 5-fill followed by a 3-fill is
 * `xor ecx,ecx / xor edx,edx` + eight base+disp stores; 6/7/8-fills are
 * `rep stosd`; `memset(s->r,0,12)` beside them is `lea edx,[eax+0x1c]` +
 * stores through edx.  That `lea` is why the whole memset family sat on 15.
 *
 * LOAD-BEARING: both groups must be fill loops (blokes plain + riders loop
 * 96; blokes loop + riders plain 19; memset on either group 93/101); the
 * blokes loop must precede the riders loop (9 reversed); the six scalar
 * seeds (route, best, f10, count, timer, take) come before both loops
 * (seeds after the loops 103; struct-order interleave 12; count last 3); the
 * link and publish follow the riders loop (link between the loops 8).
 * INERT: the loop variable (x, y, or a separate i, shared or not), its scope
 * (block or function), the bound spelling (<, !=, <=, sizeof-derived) and
 * braces -- all 0.  Pointer-cursor fills (`for (q = ...; q < ...; q++)`,
 * 123, escapes) and count-down do/while (96) are not recognised as the idiom.
 *
 * Why no earlier pass found it: the corpus has no other instance.  A scan of
 * every function in the binary for a second `xor r,r` inside a zero-store run
 * hits only this function, and a scan for an argument load threaded through a
 * record-initialisation run before a call hits only this function and its
 * twin BoatingSchool_Add (ridecb5.c) -- whose q[0..4] run and `o` threading
 * are the same shape and are the obvious place to try the fill-loop spelling.
 * The earlier notes' mechanism (`o` in eax versus ecx as a single allocator
 * decision) was right; the missing construct was the loop.
 *
 * DATA: the `owner` handed to JungleCruise_UpdateRiverTile really is the
 * station record itself (its +0x00 IS the packed square). */
// FUNCTION: LEGOLAND 0x00434f90
void JungleCruise_Add(void* o, Pos* p)
{
    BPosW      key;
    JcStation* st;

    key.b.x = (unsigned char)p->x;
    key.b.y = (unsigned char)p->y;
    st = (JcStation*)HeapAlloc_w(sizeof(JcStation));

    if (st) {
        int x;
        int y;

        st->pos = key;
        st->ax = (unsigned char)(p->x + g_jc_dock_a.left + 2);
        st->ay = (unsigned char)(p->y + g_jc_dock_a.top + 2);
        st->bx = (unsigned char)(p->x + g_jc_dock_b.left + 2);
        st->by = (unsigned char)(p->y + g_jc_dock_b.top + 2);
        st->route = 0;
        st->best = 0x270f;
        st->f10 = 0;
        st->count = 0;
        st->timer = 0x96;
        st->take = 3;
        for (x = 0; x < 5; x++) st->blokes[x] = 0;
        for (x = 0; x < 3; x++) st->riders[x] = 0;
        st->next = g_jc_stations;
        g_jc_stations = st;
        AddBasicObject(o, p);

        JungleCruise_UpdateRiverTile(p->x + g_jc_dock_a.left + 2,
                                     p->y + g_jc_dock_a.top + 2, 1, st);
        JungleCruise_UpdateRiverTile(p->x + g_jc_dock_b.left + 2,
                                     p->y + g_jc_dock_b.top + 2, 4, st);

        for (y = g_jc_area.top; y <= g_jc_area.bottom; y++) {
            for (x = g_jc_area.left; x <= g_jc_area.right - 1; x++) {
                if (x == g_jc_area.left)
                    SetMapTile(p->x + x, p->y + y, (unsigned short)(JC_TILE0 + 9));
                else if (x == g_jc_area.right - 1)
                    SetMapTile(p->x + x, p->y + y, (unsigned short)(JC_TILE0 + 0xc));
                else
                    SetMapTile(p->x + x, p->y + y, JC_TILE0);
            }
        }
    }
}


/* =========================================================================
 * 0x00433d90 -- MonkeyTree_CalcCursor (JUNGLE CRUISE MONKEY TREE cb_90).
 *
 * The monkey tree's placement-cursor calculator: the SINGLE-pass sibling of
 * JcWater_CalcCursor (ridecb7.c 0x00436200) and MonkeyFish_CalcCursor
 * (ridecb2.c 0x00434330).  It stamps the class footprint into the edit
 * cursor, asks the river which of the four cardinal arms exists under the
 * mouse, refuses the placement with error 14 when there is none, and
 * otherwise hands each existing arm one of the class's four preview cursors
 * five cells out in that direction, chaining the used ones onto the edit
 * cursor.
 *
 * Unlike the monkey fish it does NOT push the preview rect five cells south
 * and it does not compare two probes, and unlike the water square it uses the
 * class rect rather than a fixed 5x5 block and never calls CheckForPeople --
 * a tree may be dropped on a visitor.
 *
 * The station square ProbeRiver reports is DISCARDED (the local lives in the
 * dead `o` argument slot), and `cls` must be assigned immediately before the
 * footprint copy: as a declaration initialiser it takes a different frame
 * home and the whole register plan shifts (the same trap ridecb2.c hit).
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00433d90
void MonkeyTree_CalcCursor(MapObj* o, int sx, int sy)
{
    BPosW   owner;
    ObjDef* cls;
    int     count;
    int     found;

    count = 0;
    cls = o->cls;
    g_edit_cursor_rect = cls->rect;
    ScreenToMapRef(sx, &g_edit_cursor_origin, sy);
    found = JungleCruise_ProbeRiver(g_edit_cursor_origin.x,
                                    g_edit_cursor_origin.y, &owner);
    g_edit_cursor_next = 0;
    if (!found) {
        SetCursorError(&g_edit_cursor, 14);
        return;
    }

    ValidateCursor(&g_edit_cursor, cls);
    if (CursorIsValid(&g_edit_cursor)) {
        DefaultCursor(&g_jctree_cursors[0]);
        DefaultCursor(&g_jctree_cursors[1]);
        DefaultCursor(&g_jctree_cursors[2]);
        DefaultCursor(&g_jctree_cursors[3]);
        g_jctree_cursors[0].rect = g_edit_cursor_rect;
        g_jctree_cursors[1].rect = g_edit_cursor_rect;
        g_jctree_cursors[2].rect = g_edit_cursor_rect;
        g_jctree_cursors[3].rect = g_edit_cursor_rect;
        ResetCursorFootprint(&g_jctree_cursors[0]);
        ResetCursorFootprint(&g_jctree_cursors[1]);
        ResetCursorFootprint(&g_jctree_cursors[2]);
        ResetCursorFootprint(&g_jctree_cursors[3]);
        g_jctree_cursors[0].flags = 0x2034;
        g_jctree_cursors[1].flags = 0x2034;
        g_jctree_cursors[2].flags = 0x2034;
        g_jctree_cursors[3].flags = 0x2034;

        if (found & 1) {
            g_jctree_cursors[count].origin.x = g_edit_cursor_origin.x;
            g_jctree_cursors[count].origin.y = g_edit_cursor_origin.y - 5;
            count++;
        }
        if (found & 2) {
            g_jctree_cursors[count].origin.x = g_edit_cursor_origin.x + 5;
            g_jctree_cursors[count].origin.y = g_edit_cursor_origin.y;
            count++;
        }
        if (found & 4) {
            g_jctree_cursors[count].origin.x = g_edit_cursor_origin.x;
            g_jctree_cursors[count].origin.y = g_edit_cursor_origin.y + 5;
            count++;
        }
        if (found & 8) {
            g_jctree_cursors[count].origin.x = g_edit_cursor_origin.x - 5;
            g_jctree_cursors[count].origin.y = g_edit_cursor_origin.y;
            count++;
        }
        if (count != 0) {
            g_edit_cursor_next = &g_jctree_cursors[0];
            if (count > 1) {
                int j;
                for (j = 1; j < count; j++)
                    g_jctree_cursors[j - 1].next = &g_jctree_cursors[j];
            }
        }
    }
}


/* =========================================================================
 * 0x00431520 -- OctopusCafe_Destroy (OCTOPUS CAFE cb_ac).
 *
 * The cafe's teardown: kill every sprite the class owns and stop its money
 * sound.  It names TWENTY-FIVE sprite globals one at a time -- nine at
 * 0x0081cd60..0x0081cd80 (the building's own layers) and, after a 0x1c-byte
 * gap, sixteen at 0x0081cda0..0x0081cddc, one per chair -- which is what
 * confirms the sixteen-seat model ridecb4.c recovered from the walk tables:
 * every chair carries its own sprite handle.  Nothing is nulled afterwards,
 * so the handles dangle until the cafe is created again.
 *
 * The final KillMoneySFX is a TAIL CALL (the function ends in `jmp`, not
 * `ret`), which is why this one keeps a WIP marker: audit.py certifies it,
 * but the shared tools/match.py cannot bound a tail-jump function.
 * ========================================================================= */

#ifndef LEGOLAND_PORTABLE
extern void KillSprite(void* sprite);                        /* 0x00497bd0 */
#else
extern int KillSprite(void* sprite);                        /* 0x00497bd0 */
#endif
extern void KillMoneySFX(void);                              /* 0x00453930 */

/* The cafe's own sprite layers and the sixteen chair sprites. */
extern void* g_cafe_sprites[9];        /* 0x0081cd60 */
extern void* g_cafe_chair_sprites[16]; /* 0x0081cda0 */

// FUNCTION: LEGOLAND 0x00431520
void OctopusCafe_Destroy(void)
{
    if (g_cafe_sprites[0]) KillSprite(g_cafe_sprites[0]);
    if (g_cafe_sprites[1]) KillSprite(g_cafe_sprites[1]);
    if (g_cafe_sprites[2]) KillSprite(g_cafe_sprites[2]);
    if (g_cafe_sprites[3]) KillSprite(g_cafe_sprites[3]);
    if (g_cafe_sprites[4]) KillSprite(g_cafe_sprites[4]);
    if (g_cafe_sprites[5]) KillSprite(g_cafe_sprites[5]);
    if (g_cafe_sprites[6]) KillSprite(g_cafe_sprites[6]);
    if (g_cafe_sprites[7]) KillSprite(g_cafe_sprites[7]);
    if (g_cafe_sprites[8]) KillSprite(g_cafe_sprites[8]);
    if (g_cafe_chair_sprites[0]) KillSprite(g_cafe_chair_sprites[0]);
    if (g_cafe_chair_sprites[1]) KillSprite(g_cafe_chair_sprites[1]);
    if (g_cafe_chair_sprites[2]) KillSprite(g_cafe_chair_sprites[2]);
    if (g_cafe_chair_sprites[3]) KillSprite(g_cafe_chair_sprites[3]);
    if (g_cafe_chair_sprites[4]) KillSprite(g_cafe_chair_sprites[4]);
    if (g_cafe_chair_sprites[5]) KillSprite(g_cafe_chair_sprites[5]);
    if (g_cafe_chair_sprites[6]) KillSprite(g_cafe_chair_sprites[6]);
    if (g_cafe_chair_sprites[7]) KillSprite(g_cafe_chair_sprites[7]);
    if (g_cafe_chair_sprites[8]) KillSprite(g_cafe_chair_sprites[8]);
    if (g_cafe_chair_sprites[9]) KillSprite(g_cafe_chair_sprites[9]);
    if (g_cafe_chair_sprites[10]) KillSprite(g_cafe_chair_sprites[10]);
    if (g_cafe_chair_sprites[11]) KillSprite(g_cafe_chair_sprites[11]);
    if (g_cafe_chair_sprites[12]) KillSprite(g_cafe_chair_sprites[12]);
    if (g_cafe_chair_sprites[13]) KillSprite(g_cafe_chair_sprites[13]);
    if (g_cafe_chair_sprites[14]) KillSprite(g_cafe_chair_sprites[14]);
    if (g_cafe_chair_sprites[15]) KillSprite(g_cafe_chair_sprites[15]);
    KillMoneySFX();
}


/* =========================================================================
 * 0x00435230 -- JungleCruise_DrawSelection (JUNGLE CRUISE cb_94).
 *
 * RENAMED from the slot's generic "update2": this is the ride's SELECTION
 * PAINTER.  After letting the standard cursor calculator place the building's
 * own preview it walks all FOUR of the ride's child lists -- river squares,
 * monkey trees, monkey fish and the fourth (unsaved) decoration list -- and,
 * for every record whose owner is the map square currently under the cursor
 * (g_sel_bpos), stamps that record's square into the water class's scratch
 * cursor and renders it.  So selecting a jungle cruise lights up its whole
 * river and every decoration hanging off it in one pass.
 *
 * THE CURSOR IS RESHAPED BETWEEN LISTS, and that is the interesting part:
 *   - squares and trees use the fixed 5x5 block (kJcWaterRect, the constant
 *     at 0x004b7478 the whole river uses);
 *   - before the FISH list the block's `top` is moved five cells north, so a
 *     fish highlights the square it swims into rather than the one it is
 *     filed under;
 *   - each DECORATION gets a 1x3 vertical strip {0,-1,0,1} and is rendered
 *     TWICE, the second time with the origin six cells west -- the two posts
 *     of a rope/bridge decoration.
 * The reshaping is cumulative and never restored, so the -5 from the fish
 * pass is still in the cursor when the ride is next selected... except that
 * the fish pass runs after a fresh `rect = kJcWaterRect` every call.
 *
 * All four list heads are read BEFORE BasicObjectDCalcCursor, so a head the
 * cursor calculator changed would be missed (the same eager-read shape
 * BoatingSchool_Remove has).  VC6 keeps two of them in ebx/ebp and spills the
 * other two to the `sub esp,8` pair, reloading them every iteration.
 * ========================================================================= */

typedef struct JcWater2 {
    BPosW           pos;            /* +0x00 */
    BPosW           owner;          /* +0x02 */
    unsigned char   pad04[0x10 - 4];
    struct JcWater2* next;          /* +0x10 */
    unsigned char   pad14[0x1c - 0x14];
} JcWater2;                         /* 0x1c */

typedef struct JcMonkeyFish {
    BPosW           pos;            /* +0x00 */
    BPosW           owner;          /* +0x02 */
    unsigned char   pad04[4];
    struct JcMonkeyFish* next;      /* +0x08 */
} JcMonkeyFish;                     /* 0x0c */

typedef struct JcMonkeyTree {
    BPosW           pos;            /* +0x00 */
    BPosW           owner;          /* +0x02 */
    struct JcMonkeyTree* next;      /* +0x04 */
} JcMonkeyTree;                     /* 0x08 */

typedef struct JcDeco {
    BPosW           pos;            /* +0x00 */
    BPosW           owner;          /* +0x02 */
    struct JcDeco*  next;           /* +0x04 */
} JcDeco;                           /* 0x08 */

extern JcWater2*     g_jc_water;    /* 0x0062fd2c */
extern JcMonkeyTree* g_jc_trees;    /* 0x00629c2c */
extern JcMonkeyFish* g_jc_fish;     /* 0x00629c30 */
extern JcDeco*       g_jc_deco;     /* 0x00629c34 */

/* The map square currently under the edit cursor (objmap2.c's g_sel_bpos). */
extern BPosW  g_sel_bpos;              /* 0x00667c54 */
/* The water class's scratch placement cursor (shared with the boating
 * school -- ridecb8.c calls the same address g_bs_water_cursor). */
extern Cursor g_jc_water_cursor;       /* 0x0082ae20 */

/* The fixed 5x5 footprint a river square always takes (0x004b7478). */
static const Rect kJcWaterRect = { -2, -2, 2, 2, 0 };

extern void BasicObjectDCalcCursor(void* elem, void* p);     /* 0x00480bb0 */
extern void BuildCursorPtr(Cursor* c, int a, int b);         /* 0x0045f5f0 */
extern void RenderCursor(Cursor* c);                         /* 0x0045ff00 */

// FUNCTION: LEGOLAND 0x00435230
void JungleCruise_DrawSelection(void* elem, void* p)
{
    JcMonkeyFish* f = g_jc_fish;
    JcDeco*       d = g_jc_deco;
    JcWater2*     w = g_jc_water;
    JcMonkeyTree* t = g_jc_trees;

    BasicObjectDCalcCursor(elem, p);
    DefaultCursor(&g_jc_water_cursor);
    g_jc_water_cursor.rect = kJcWaterRect;

    while (w) {
        if (w->owner.w == g_sel_bpos.w) {
            g_jc_water_cursor.origin.x = w->pos.b.x;
            g_jc_water_cursor.origin.y = w->pos.b.y;
            ResetCursorFootprint(&g_jc_water_cursor);
            g_jc_water_cursor.flags = 8;
            BuildCursorPtr(&g_jc_water_cursor, 0, 0);
            RenderCursor(&g_jc_water_cursor);
        }
        w = w->next;
    }

    while (t) {
        if (t->owner.w == g_sel_bpos.w) {
            g_jc_water_cursor.origin.x = t->pos.b.x;
            g_jc_water_cursor.origin.y = t->pos.b.y;
            ResetCursorFootprint(&g_jc_water_cursor);
            g_jc_water_cursor.flags = 8;
            BuildCursorPtr(&g_jc_water_cursor, 0, 0);
            RenderCursor(&g_jc_water_cursor);
        }
        t = t->next;
    }

    g_jc_water_cursor.rect.top -= 5;

    while (f) {
        if (f->owner.w == g_sel_bpos.w) {
            g_jc_water_cursor.origin.x = f->pos.b.x;
            g_jc_water_cursor.origin.y = f->pos.b.y;
            ResetCursorFootprint(&g_jc_water_cursor);
            g_jc_water_cursor.flags = 8;
            BuildCursorPtr(&g_jc_water_cursor, 0, 0);
            RenderCursor(&g_jc_water_cursor);
        }
        f = f->next;
    }

    while (d) {
        if (d->owner.w == g_sel_bpos.w) {
            g_jc_water_cursor.rect.top = -1;
            g_jc_water_cursor.rect.bottom = 1;
            g_jc_water_cursor.rect.left = 0;
            g_jc_water_cursor.rect.right = 0;
            g_jc_water_cursor.origin.x = d->pos.b.x;
            g_jc_water_cursor.origin.y = d->pos.b.y;
            ResetCursorFootprint(&g_jc_water_cursor);
            g_jc_water_cursor.flags = 8;
            BuildCursorPtr(&g_jc_water_cursor, 0, 0);
            RenderCursor(&g_jc_water_cursor);
            g_jc_water_cursor.origin.x -= 6;
            BuildCursorPtr(&g_jc_water_cursor, 0, 0);
            RenderCursor(&g_jc_water_cursor);
        }
        d = d->next;
    }
}


/* =========================================================================
 * THE MECHANICS HUT (class "MECHANICS HUT"), cb_b0 and cb_a8.
 *
 * The hut is the park's repair depot: it is where the MECHANICS live between
 * jobs, so its customers are staff, not visitors, and its rider list holds
 * whoever is currently inside or on their way.  Like the octopus cafe it has
 * to interleave its people with its own sprite, so it owns a painter (cb_b0)
 * as well as the per-frame tick (cb_a8).
 *
 * THE STAGE NUMBERS.  A mechanic's Bloke +0x60 runs 0..3 for the approach
 * (walk in, queue, enter, wait) and then a SECOND, disjoint block at
 * 0x64..0x67 for being inside the hut.  The painter's pass order is what
 * says where each stage stands relative to the building:
 *
 *     0, 1, 2, 3, 0x64, 0x65, 0x67   drawn BEFORE the hut sprite (behind it)
 *     [the hut sprite]
 *     0x66                            drawn AFTER it (in front of the door)
 *
 * so 0x66 is the one stage that stands on the near side of the hut.
 * ========================================================================= */

/* A person as this class sees it (ridecb4.c's Bloke, same offsets). */
typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;           /* +0x0e  low-level AI state (0 = idle) */
    unsigned char  pad10[0x24 - 0x10];
    Pos            target;          /* +0x24  walk target, 24.8 */
    unsigned char  pad2c[0x36 - 0x2c];
    unsigned char  seat;            /* +0x36  job slot / carriage index */
    unsigned char  pad37[0x60 - 0x37];
    unsigned char  action;          /* +0x60  state-machine stage */
    unsigned char  pad61;
    unsigned short flags62;         /* +0x62  8 = using this ride, 0x20 = hired */
    unsigned char  pad64[4];
    Pos            world;           /* +0x68  world position, 24.8 */
    unsigned char  pad70[0x73 - 0x70];
    unsigned char  new_dir;         /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14];      /* +0x98  CalcMoveLine scratch */
} Bloke;

/* A rider slot: its bloke at +0x08 and the packed map square of the ride
 * instance it is using at +0x0c (rides.c's RiderNode). */
typedef struct RiderNode {
    struct RiderNode* next;         /* +0x00 */
    struct RiderNode* prev;         /* +0x04 */
    Bloke*            bloke;        /* +0x08 */
    unsigned short    ride_id;      /* +0x0c  packed {x,y} of the instance */
    unsigned short    pad0e;
    void*             person;       /* +0x10 */
} RiderNode;

/* A sprite handle, as this class touches it. */
typedef struct Spr {
    unsigned char pad00[0x10];
    unsigned int  flags;            /* +0x10  0x2000 = draw via the +0xa0 slot */
} Spr;

/* The class record behind an element -- the same 0xd0-byte ObjDef the rest of
 * the game calls a RideDef, with the fields this class touches named. */
typedef struct RideObject {
    unsigned char pad00[0x14];
    int           f14;              /* +0x14  draw-descriptor field 1 */
    int           f18;              /* +0x18  draw-descriptor field 2 */
    unsigned int  flags;            /* +0x1c  0x20 tick, 0x400 ask +0xa0 */
    unsigned char pad20[0x3c - 0x20];
    Rect          rect;             /* +0x3c  class footprint */
    unsigned char pad50[0x64 - 0x50];
    Spr*          sprite;           /* +0x64  build sprite */
    unsigned char pad68[0xcc - 0x68];
    RiderNode*    riders;           /* +0xcc  live rider list of the class */
} RideObject;

/* A placed map object as the hut's remove handler sees it. */
typedef struct RideMapObj {
    unsigned char pad00[0x0c];
    RideObject*   cls;              /* +0x0c */
} RideMapObj;

/* The block the +0xa0 slot returns (westtown.c's ShopDrawDesc, one shared
 * static per subsystem). */
typedef struct HutDrawDesc {
    Spr*           sprite;          /* +0x00 */
    int            f04;             /* +0x04 */
    int            f08;             /* +0x08 */
    unsigned short f0c;             /* +0x0c */
} HutDrawDesc;

typedef struct RideElem {
    char*        name;              /* +0x00 */
    char*        image;             /* +0x04 */
    unsigned int flags;             /* +0x08 */
    RideObject*  data;              /* +0x0c */
} RideElem;

/* A placed object's map square, packed as two bytes. */
typedef struct MapSquare { unsigned char bx; unsigned char by; } MapSquare;

typedef struct Offset { int ox; int oy; } Offset;

extern Offset GetScreenCoordsForObject(MapSquare* sq, RideObject* item); /* 0x00442cc0 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx);   /* 0x004853a0 */
extern void   IP_RenderBlokeIn3DNow(Bloke* b);                           /* 0x00440010 */

/* The hut's own sprite handle (its cb_a4 fills it). */
extern void* g_hut_sprite;             /* 0x0062fe54 */

/* Draw every collected mechanic whose state-machine stage is `stage`. */
/* Draw every collected mechanic whose state-machine stage is `stage`.
 * The count is a CHAR and so is the loop variable: that is what makes VC6
 * rematerialise `movsx edi,bl` in each of the eight expansions instead of
 * hoisting one int copy of the count into ebp -- which in turn leaves ebp for
 * `item` and ebx free of a hoisted constant, so the stage compares stay
 * immediates.  With `int i` the whole register plan shifts and the function
 * grows ten instructions. */
static __inline void HutDrawStage(Bloke** list, char n, int stage)
{
    char i;
    for (i = 0; i < n; i++)
        if (list[i]->action == stage)
            IP_RenderBlokeIn3DNow(list[i]);
}

/* =========================================================================
 * 0x0043d580 -- MechanicsHut_Draw (MECHANICS HUT cb_b0).
 *
 * Collects every rider filed under this instance's map square into a
 * thirty-slot stack array (a signed CHAR counts them, so the array would
 * overflow at 127 riders long before the count wrapped), then paints them in
 * eight stage passes with the hut's sprite between the seventh and eighth.
 *
 * The array is zeroed on entry even though only the first `count` slots are
 * ever read, and the collect walk is a do/while entered only when the class
 * has any riders at all.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0043d580
void MechanicsHut_Draw(RideElem* elem, int x, int y, MapSquare* sq)
{
    RideObject* item = elem->data;
    RiderNode*  r = item->riders;
    Bloke*      people[30] = { 0 };
    char        count = 0;
    Offset      screen;

    if (r) {
        do {
            if (*(unsigned short*)sq == r->ride_id)
                people[count++] = r->bloke;
            r = r->next;
        } while (r);

        if (count != 0) {
            HutDrawStage(people, count, 0);
            HutDrawStage(people, count, 1);
            HutDrawStage(people, count, 2);
            HutDrawStage(people, count, 3);
            HutDrawStage(people, count, 0x64);
            HutDrawStage(people, count, 0x65);
            HutDrawStage(people, count, 0x67);
            screen = GetScreenCoordsForObject(sq, item);
            PrintSprite(g_hut_sprite, screen.ox, screen.oy, 0, 0);
            HutDrawStage(people, count, 0x66);
        }
    }
}


/* =========================================================================
 * 0x0043d2f0 -- MechanicsHut_Tick (MECHANICS HUT cb_a8).
 *
 * The hut's per-frame state machine, run over every rider filed against the
 * class.  A mechanic is only stepped while its low-level AI state (+0x0e) is
 * idle, and the stage byte (+0x60) selects one of eight arms through a jump
 * table (the byte index table at 0x0043d50c maps 0..0x67 onto nine entries,
 * so every stage from 4 to 0x63 is a no-op).
 *
 * SIX of the eight arms are the same thing: nudge the walk target and re-plan
 * the move.  They spell out the door approach in 24.8 world units --
 *
 *   stage 0     mark "using this ride", walk +0x300 south
 *   stage 1     walk +0x400 east and +0x100 south
 *   stage 2     walk +0x400 south                (into the hut)
 *   stage 0x64  mark "using this ride", walk -0x300 north
 *   stage 0x65  walk -0x100 north and -0x480 west
 *   stage 0x66  walk -0x300 north                (out of the hut)
 *
 * -- and each ends with CalcMoveLine, state = 7 (walking), the new heading
 * byte, NewDirForAction with the heading's octant + 3, and stage++.  So 0..2
 * is the walk IN and 0x64..0x66 the walk OUT, which is exactly the split
 * MechanicsHut_Draw paints around the building.
 *
 * The two remaining arms end a mechanic's stay:
 *   stage 3     clear the "using this ride" and "hired" bits, unlink the
 *               rider node, FREE it, and give the bloke long-term action 0x11
 *               (the repair round);
 *   stage 0x67  unlink the rider node WITHOUT freeing it -- an original leak,
 *               reproduced -- set the job slot to 0x64 and refund the
 *               mechanic's 30 bricks (the player has fired him).
 *
 * `next` is latched BEFORE the arm runs, which is what makes stage 3 safe to
 * free the node it is standing on.
 * ========================================================================= */

extern void RemoveBlokeFromList(RideObject* owner, RiderNode* slot); /* 0x0044f470 */
extern void NewLongTermAction(Bloke* b, int action);                 /* 0x0044e760 */
extern void RefundMechanic(void);                                    /* 0x0049a190 */
extern int  CalcMoveLine(Pos from, Pos to, void* path);              /* 0x00480740 */
extern int  NewDirForAction(Bloke* b, unsigned char dir);            /* 0x004833d0 */
extern void HeapFree_w(void* p);                                     /* 0x0049e4d0 */

// FUNCTION: LEGOLAND 0x0043d2f0
void MechanicsHut_Tick(RideElem* elem)
{
    RideObject*   item = elem->data;
    RiderNode*    r = item->riders;
    RiderNode*    next;
    Bloke*        b;
    unsigned char dir;

    if (!r)
        return;

    do {
        b = r->bloke;
        next = r->next;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.y += 0x300;
                dir = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 1:
                b->target.x += 0x400;
                b->target.y += 0x100;
                dir = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 2:
                b->target.y += 0x400;
                dir = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 3:
                b->flags62 &= ~0x28;
                RemoveBlokeFromList(item, r);
                HeapFree_w(r);
                NewLongTermAction(b, 0x11);
                break;
            case 0x64:
                b->flags62 |= 8;
                b->target.y -= 0x300;
                dir = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 0x65:
                b->target.x -= 0x480;
                b->target.y -= 0x100;
                dir = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 0x66:
                b->target.y -= 0x300;
                dir = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 0x67:
                RemoveBlokeFromList(item, r);
                /* ORIGINAL: the node is NOT freed here (stage 3 does free it). */
                b->seat = 0x64;
                RefundMechanic();
                break;
            }
        }
        r = next;
    } while (r);
}


/* =========================================================================
 * THE REST OF THE MECHANICS HUT -- the six small slots.
 *
 * Together with MechanicsHut_Tick and MechanicsHut_Draw above this is the
 * WHOLE class.  What it says about the hut: it is a single-sprite building
 * with no per-placement record at all (its add handler is a bare forward to
 * AddBasicObject and its remove handler just unbuilds the footprint and
 * evicts the mechanics), and the only state it owns is class-wide -- the
 * ObjDef pointer, the mask sprite and the one draw descriptor.
 * ========================================================================= */

#ifndef LEGOLAND_PORTABLE
extern void  StandardRemoveObject(void* o, unsigned int bp, void* ctx); /* 0x0045f220 */
#else
/* PORT-M11: the definition (objmap2.c:983) and the ObjClass +0x9c slot it is
 * stored in (objmap2.c:99) take the map square as a 2-byte aggregate BY VALUE.
 * Same pushed dword as this `unsigned int` on x86 cdecl, a POINTER to a
 * shadow-stack temp on wasm32, and the same i32 arity either way -- so the
 * whole class is invisible to wasm-ld, linkreport and test_callback_types
 * (PORT-M10 s1b).  `BPosW` is this file's own spelling of the square. */
extern void  StandardRemoveObject(void* o, BPosW bp, void* ctx);        /* 0x0045f220 */
static __inline BPosW ll_bposw(unsigned int v)
{
    BPosW t;
    t.w = (unsigned short)v;
    return t;
}
#define StandardRemoveObject(_o, _b, _c) \
    StandardRemoveObject((_o), ll_bposw((unsigned int)(_b)), (_c))
#endif
extern void  SetEditCursorFootPrint(void* rect);             /* 0x0045f440 */
extern void* LoadSprite(const char* name, int mode);         /* 0x00497ab0 */
/* Evicts every mechanic the class has parked on one map square (the hut's
 * own version of RemoveAllBlokesFromRide; the potting shed calls it too). */
#ifndef LEGOLAND_PORTABLE
extern void  MechanicsHut_EvictRiders(RideObject* cls, unsigned int bp, int flag); /* 0x0043d7c0 */
#else
/* PORT-M11, the same class: the definition (ridemisc.c:93) takes `BPosW key`. */
extern void  MechanicsHut_EvictRiders(RideObject* cls, BPosW bp, int flag); /* 0x0043d7c0 */
#define MechanicsHut_EvictRiders(_c, _b, _f) \
    MechanicsHut_EvictRiders((_c), ll_bposw((unsigned int)(_b)), (_f))
#endif

/* The class-wide state: the ObjDef, its build sprite's LLS, the mask sprite
 * and the one draw descriptor the +0xa0 slot hands back. */
extern RideObject* g_hut_def;          /* 0x0081caf4 */
extern void*       g_hut_lls;          /* 0x0062fe50 */
extern HutDrawDesc g_hut_draw;         /* 0x0062fe30 */

/* The edit cursor's "what is being placed" pair (catapult.c's names). */
extern int         g_edit_changed;     /* 0x008119b0 */
extern RideObject* g_edit_object;      /* 0x008119b8 */

/* =========================================================================
 * 0x0043d250 -- MechanicsHut_Create (MECHANICS HUT cb_a4).
 *
 * Caches the class record, arms three ObjDef flag bits and loads the mask
 * sprite.  The flags go on in TWO steps -- 0x420 first (0x20 = tick me every
 * frame through cb_a8, 0x400 = ask cb_a0 for a draw descriptor) and 0x2000
 * after the LLS is cached -- and the global is re-read between them, which is
 * how the original spells it.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0043d250
void MechanicsHut_Create(RideElem* elem)
{
    g_hut_def = elem->data;
    g_hut_def->flags |= 0x420;
    g_hut_lls = g_hut_def->sprite;
    g_hut_def->flags |= 0x2000;
    g_hut_sprite = LoadSprite("MechHutMask.lls", 1);
}

/* =========================================================================
 * 0x0043d2a0 -- MechanicsHut_Add (MECHANICS HUT cb_98): a bare forward.  The
 * hut keeps no per-placement record, so placing one is just the standard
 * single-cell object add.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0043d2a0
void MechanicsHut_Add(void* o, Pos* p)
{
    AddBasicObject(o, p);
}

/* =========================================================================
 * 0x0043d2c0 -- MechanicsHut_Remove (MECHANICS HUT cb_9c): unbuild the
 * footprint, then throw out every mechanic filed against that square.
 * ========================================================================= */

#ifdef LEGOLAND_PORTABLE
/* PORT-M11: a +0x9c remove handler.  That slot passes the map square as a
 * 2-byte aggregate BY VALUE (objmap2.c:99, called objmap2.c:1895) -- a POINTER
 * on wasm32 -- and the matched body reads the dword as an `unsigned int`.
 * Renamed for the portable build, with a twin of the slot's own shape over it. */
#define MechanicsHut_Remove MechanicsHut_Remove_vc6_body
#endif
// FUNCTION: LEGOLAND 0x0043d2c0
void MechanicsHut_Remove(RideMapObj* o, unsigned int bp, void* ctx)
{
    StandardRemoveObject(o, bp, ctx);
    MechanicsHut_EvictRiders(o->cls, bp, 0);
}
#ifdef LEGOLAND_PORTABLE
#undef MechanicsHut_Remove
void MechanicsHut_Remove(RideMapObj* o, BPosW bp, void* ctx)
{
    MechanicsHut_Remove_vc6_body(o, bp.w, ctx);
}
#endif

/* =========================================================================
 * 0x0043d730 -- MechanicsHut_Destroy (MECHANICS HUT cb_ac): drop the mask
 * sprite.  The handle is not cleared, so it dangles until cb_a4 runs again.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0043d730
void MechanicsHut_Destroy(void)
{
    KillSprite(g_hut_sprite);
}

/* =========================================================================
 * 0x0043d740 -- MechanicsHut_Select (MECHANICS HUT cb_8c): arm the editor to
 * place a hut.  g_edit_object is written and then RE-READ after
 * DefaultCursor, which is why the footprint pointer comes off the global
 * rather than the local.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0043d740
void MechanicsHut_Select(void)
{
    g_edit_changed = 1;
    g_edit_object = g_hut_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->rect);
}

/* =========================================================================
 * 0x0043d780 -- MechanicsHut_GetDrawDesc (MECHANICS HUT cb_a0).
 *
 * Identical in shape to Shop_GetDrawDesc (westtown.c 0x0043a390) and
 * Joust_GetDrawDesc: fill the one shared static block from the ObjDef, stamp
 * the caller's map square into it, arm the build sprite's custom-draw bit and
 * hand the block back.  There is exactly one block, so it is only valid until
 * the next class asks for its descriptor.
 * ========================================================================= */

#ifdef LEGOLAND_PORTABLE
/* PORT-M11: a +0xa0 draw handler, and that slot takes the object's base map
 * square as a 2-byte aggregate BY VALUE too (renderview.c:304) -- so on wasm32
 * this body was stamping a shadow-stack ADDRESS into the draw descriptor's
 * `f0c` square instead of the cell.  Same rename, same reason. */
#define MechanicsHut_GetDrawDesc MechanicsHut_GetDrawDesc_vc6_body
#endif
// FUNCTION: LEGOLAND 0x0043d780
HutDrawDesc* MechanicsHut_GetDrawDesc(RideElem* elem, unsigned short bp)
{
    RideObject* def = elem->data;

    g_hut_draw.sprite = def->sprite;
    g_hut_draw.f04 = def->f14;
    g_hut_draw.f08 = def->f18;
    g_hut_draw.f0c = bp;
    def->sprite->flags |= 0x2000;
    return &g_hut_draw;
}
#ifdef LEGOLAND_PORTABLE
#undef MechanicsHut_GetDrawDesc
HutDrawDesc* MechanicsHut_GetDrawDesc(RideElem* elem, BPosW bp)
{
    return MechanicsHut_GetDrawDesc_vc6_body(elem, bp.w);
}
#endif


/* =========================================================================
 * THE POTTING SHED (class "POTTING SHED") -- the gardeners' depot, and the
 * MECHANICS HUT's twin.
 *
 * Six of its eight slots are the hut's six with different globals, down to
 * the instruction: the same ObjDef cache + 0x420/0x2000 flag pair + mask
 * sprite in cb_a4, the same bare AddBasicObject forward in cb_98, the same
 * unbuild-then-evict in cb_9c, the same KillSprite in cb_ac, the same editor
 * arm in cb_8c and the same shared draw descriptor in cb_a0.  The ONE
 * difference is the third argument of the shared evictor: the shed passes 1
 * where the hut passes 0, which is what tells MechanicsHut_EvictRiders which
 * of the two staff kinds it is throwing out.
 * ========================================================================= */

/* The class-wide state, laid out exactly like the hut's four globals. */
extern RideObject* g_shed_def;         /* 0x0081caf0 */
extern void*       g_shed_lls;         /* 0x0062fe48 */
extern void*       g_shed_sprite;      /* 0x0062fe4c */
extern HutDrawDesc g_shed_draw;        /* 0x0062fe10 */

// FUNCTION: LEGOLAND 0x0043ce60
void PottingShed_Create(RideElem* elem)
{
    g_shed_def = elem->data;
    g_shed_def->flags |= 0x420;
    g_shed_lls = g_shed_def->sprite;
    g_shed_def->flags |= 0x2000;
    g_shed_sprite = LoadSprite("gshedmatte.lls", 1);
}

// FUNCTION: LEGOLAND 0x0043ceb0
void PottingShed_Add(void* o, Pos* p)
{
    AddBasicObject(o, p);
}

#ifdef LEGOLAND_PORTABLE
/* PORT-M11: the same, for the +0x9c slot's shape -- see above. */
#define PottingShed_Remove PottingShed_Remove_vc6_body
#endif
// FUNCTION: LEGOLAND 0x0043ced0
void PottingShed_Remove(RideMapObj* o, unsigned int bp, void* ctx)
{
    StandardRemoveObject(o, bp, ctx);
    MechanicsHut_EvictRiders(o->cls, bp, 1);
}
#ifdef LEGOLAND_PORTABLE
#undef PottingShed_Remove
void PottingShed_Remove(RideMapObj* o, BPosW bp, void* ctx)
{
    PottingShed_Remove_vc6_body(o, bp.w, ctx);
}
#endif

// FUNCTION: LEGOLAND 0x0043d1c0
void PottingShed_Destroy(void)
{
    KillSprite(g_shed_sprite);
}

// FUNCTION: LEGOLAND 0x0043d1d0
void PottingShed_Select(void)
{
    g_edit_changed = 1;
    g_edit_object = g_shed_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->rect);
}

#ifdef LEGOLAND_PORTABLE
/* PORT-M11: the same, for the +0xa0 slot's shape -- see above. */
#define PottingShed_GetDrawDesc PottingShed_GetDrawDesc_vc6_body
#endif
// FUNCTION: LEGOLAND 0x0043d210
HutDrawDesc* PottingShed_GetDrawDesc(RideElem* elem, unsigned short bp)
{
    RideObject* def = elem->data;

    g_shed_draw.sprite = def->sprite;
    g_shed_draw.f04 = def->f14;
    g_shed_draw.f08 = def->f18;
    g_shed_draw.f0c = bp;
    def->sprite->flags |= 0x2000;
    return &g_shed_draw;
}
#ifdef LEGOLAND_PORTABLE
#undef PottingShed_GetDrawDesc
HutDrawDesc* PottingShed_GetDrawDesc(RideElem* elem, BPosW bp)
{
    return PottingShed_GetDrawDesc_vc6_body(elem, bp.w);
}
#endif


/* =========================================================================
 * 0x0043cf00 -- PottingShed_Tick (POTTING SHED cb_a8).
 *
 * The gardeners' version of MechanicsHut_Tick, and much shorter because the
 * shed's door is on the SIDE: there is one walk step in and one out, both
 * along x, where the hut needs three of each.
 *
 *   stage 0     mark "using this ride", walk +0x900 east, re-plan
 *   stage 1     clear the ride/hired bits, unlink the rider node, hand the
 *               gardener long-term action 0x10 (the gardening round) and free
 *               the node -- note the order: the node is freed AFTER the new
 *               action is planned, the opposite of the hut, which frees first
 *   stage 0x64  mark "using this ride", walk -0x900 west, re-plan
 *   stage 0x65  unlink the node WITHOUT freeing it (the same leak the hut
 *               has), set the job slot to 0x64 and refund the gardener
 *
 * Every stage from 2 to 0x63 is a no-op, exactly as in the hut.
 * ========================================================================= */

extern void RefundGardener(void);                            /* 0x0049a150 */

// FUNCTION: LEGOLAND 0x0043cf00
void PottingShed_Tick(RideElem* elem)
{
    RideObject*   item = elem->data;
    RiderNode*    r = item->riders;
    RiderNode*    next;
    Bloke*        b;
    unsigned char dir;

    if (!r)
        return;

    do {
        b = r->bloke;
        next = r->next;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x += 0x900;
                dir = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 1:
                b->flags62 &= ~0x28;
                RemoveBlokeFromList(item, r);
                NewLongTermAction(b, 0x10);
                HeapFree_w(r);
                break;
            case 0x64:
                b->flags62 |= 8;
                b->target.x -= 0x900;
                dir = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 0x65:
                RemoveBlokeFromList(item, r);
                /* ORIGINAL: the node is NOT freed here. */
                b->seat = 0x64;
                RefundGardener();
                break;
            }
        }
        r = next;
    } while (r);
}


/* =========================================================================
 * 0x0043d0b0 -- PottingShed_Draw (POTTING SHED cb_b0).
 *
 * MechanicsHut_Draw with four stage passes instead of eight, and with NOTHING
 * drawn after the building: every gardener stage (0, 1, 0x64, 0x65) is behind
 * the shed's mask sprite.  Same thirty-slot array, same signed-char count,
 * same collect walk.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0043d0b0
void PottingShed_Draw(RideElem* elem, int x, int y, MapSquare* sq)
{
    RideObject* item = elem->data;
    RiderNode*  r = item->riders;
    Bloke*      people[30] = { 0 };
    char        count = 0;
    Offset      screen;

    if (r) {
        do {
            if (*(unsigned short*)sq == r->ride_id)
                people[count++] = r->bloke;
            r = r->next;
        } while (r);

        if (count != 0) {
            HutDrawStage(people, count, 0);
            HutDrawStage(people, count, 1);
            HutDrawStage(people, count, 0x64);
            HutDrawStage(people, count, 0x65);
            screen = GetScreenCoordsForObject(sq, item);
            PrintSprite(g_shed_sprite, screen.ox, screen.oy, 0, 0);
        }
    }
}
