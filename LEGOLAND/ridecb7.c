/* LEGOLAND -- the remaining JUNGLE CRUISE class callbacks (0x0043xxxx) plus
 * the PARK ENTRANCE's per-tick state machine (0x0042dfa0).
 *
 * These are per-class handlers that SetCustomCallbacks (screen.c 0x00452c20)
 * installs into the 0xd0-byte ObjDef callback slots.  None of them is
 * exported, so every extent below was taken from the disassembly by control
 * flow (tools/audit.py).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere) and mirror the ones in ridecb2.c / junglecruise.c, which own the
 * rest of this ride.
 *
 * ------------------------------------------------------------------------
 * WHICH CLASS?  -- read back out of SetCustomCallbacks (screen.c):
 *
 *   addr        class                      slot      what it really is
 *   0x00436200  JUNGLE CRUISE WATER        cb_90     JcWater_CalcCursor   [OK]
 *   0x004365f0  JUNGLE CRUISE WATER        cb_add    JcWater_Add          [OK]
 *   0x00434100  JUNGLE CRUISE MONKEY FISH  cb_add    MonkeyFish_Add       [OK]
 *   0x00435c70  JUNGLE CRUISE              cb_save   SaveJungleCruise     [OK]
 *   0x0042dfa0  ENTRANCE 1                 cb_a8     Entrance1_Tick       [OK]
 *
 * ... plus the one unowned helper they call:
 *
 *   0x0042d970  ENTRANCE 1  Entrance1_PlayPaySound (the turnstile)   [OK]
 *
 * So all five names in screen.c ("CB_<addr>") were placeholders; the names
 * used here are the recovered ones.  Every function in this file is exact
 * under tools/audit.py and ends in a real `ret`.
 *
 * ------------------------------------------------------------------------
 * WHAT THE SLOT NUMBERS TURN OUT TO MEAN (confirming ridecb2.c / ridecb5.c)
 *
 *   cb_90   the PLACEMENT CURSOR calculator -- fills the edit cursor's
 *           footprint, validates it, and chains extra preview cursors onto
 *           it through the edit cursor's `next` link (0x008003f0) so a piece
 *           can draw its neighbours as well as itself.
 *   cb_add  the PLACE handler -- allocate the record, link it on the class's
 *           global list, put the object on the map, paint its tiles.
 *   cb_save the savegame writer (cb_b8 is the matching reader).
 *   cb_a8   the class's per-tick state machine over its whole rider list.
 *
 * ------------------------------------------------------------------------
 * TWO VC6 LEVERS THIS FILE PINNED DOWN (both new; see docs/DECOMP.md)
 *
 * 1. "EVALUATE ALL, THEN PUSH".  Writing `f(p->a, p->b, p->c, p->d)` makes
 *    VC6 push each argument as soon as it is evaluated (right to left), which
 *    needs only two scratch registers.  Loading the four values into NAMED
 *    LOCALS first and calling `f(x0, y0, x1, y1)` makes it evaluate all four
 *    before any push, so it runs out of scratch registers and spills one into
 *    a callee-saved register -- which is exactly what JcWater_Add's tail
 *    does.  This is the whole difference between 0 and 13 mismatches there.
 *
 * 2. TWO READS OF ONE GLOBAL BEAT ONE READ PLUS A COPY.  A list that is
 *    walked once to count it and then again to write it reads its head
 *    TWICE in the original (`n = 0; for (q = g_head; q; q = q->next) n++;
 *    p = g_head;`), not once into a variable that both loops share.  With no
 *    call between them VC6 CSEs the two reads into one load, but keeps the
 *    cursor's initial value as a real copy -- `mov esi,[head] / mov eax,esi /
 *    cmp eax,ebp`.  Spelling it as one read plus `q = p` lets copy
 *    propagation fold the copy into the compare (`cmp esi,ebp / mov eax,esi`)
 *    and costs a mismatch in every one of the five loops.
 * ========================================================================= */

/* ---- shared map / cursor types (same offsets as objmap2.c, ridecb2.c) ---- */
typedef struct Pos { int x; int y; } Pos;

typedef struct Rect {
    int left;                       /* +0x00 */
    int top;                        /* +0x04 */
    int right;                      /* +0x08 */
    int bottom;                     /* +0x0c */
    struct Rect* next;              /* +0x10 */
} Rect;                             /* 0x14 */

/* The bare four-int rectangle the people test takes (no list link). */
typedef struct WinRect { int left; int top; int right; int bottom; } WinRect;

/* A packed 2-byte map square passed BY VALUE, and its 16-bit view. */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union BPosW { unsigned short w; BPos b; } BPosW;

struct RideInst;
typedef struct ObjDef {
    unsigned char pad00[0x0c];
    int           dx;               /* +0x0c */
    int           dy;               /* +0x10 */
    unsigned char pad14[0x3c - 0x14];
    Rect          rect;             /* +0x3c  class footprint */
    unsigned char pad50[0xc4 - 0x50];
    void*         c4;               /* +0xc4  the class's shared instance */
    unsigned char padc8[0xcc - 0xc8];
    struct RideInst* instances;     /* +0xcc */
} ObjDef;

/* A placed map object; only its class pointer matters here. */
typedef struct MapObj {
    unsigned char pad00[0x0c];
    ObjDef*       cls;              /* +0x0c */
    unsigned char pad10[4];
} MapObj;                           /* 0x14 */

/* The edit / preview cursor block (0x1834 bytes). */
typedef struct Cursor {
    unsigned char  pad0000[0x1404];
    Pos            origin;          /* +0x1404 */
    int            status;          /* +0x140c */
    int            error;           /* +0x1410 */
    Rect           rect;            /* +0x1414 */
    unsigned char  pad1428[0x1828 - 0x1428];
    unsigned int   flags;           /* +0x1828 */
    int            f182c;           /* +0x182c */
    struct Cursor* next;            /* +0x1830 */
} Cursor;                           /* 0x1834 */

/* ---- the edit cursor and its aliased sub-objects ------------------------ */
extern Cursor  g_edit_cursor;          /* 0x007febc0 */
extern Pos     g_edit_cursor_origin;   /* 0x007fffc4 == g_edit_cursor.origin */
extern Rect    g_edit_cursor_rect;     /* 0x007fffd4 == g_edit_cursor.rect */
extern Cursor* g_edit_cursor_next;     /* 0x008003f0 == g_edit_cursor.next */

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
extern int   CheckForPeople(WinRect* r);                     /* 0x00485260 */

/* Probes the jungle-cruise river around (x, y): returns the bitmask of the
 * four arms that exist (1 N, 2 E, 4 S, 8 W, five map cells out) and stores
 * the owning station's map square in *owner. */
extern int   JungleCruise_ProbeRiver(int x, int y, void* owner); /* 0x00436fb0 */

/* The JUNGLE CRUISE WATER class's own four preview cursors, one per arm the
 * new square can be joined to.  They sit between g_jc_anim_tick (0x00629c54)
 * and g_jc_water (0x0062fd2c), which is exactly 4 * 0x1834 bytes. */
extern Cursor g_jc_water_cursors[4];   /* 0x00629c58, stride 0x1834 */

/* The fixed 5x5 footprint a river square always takes, whatever the class
 * descriptor says (the original's constant at 0x004b7478). */
static const Rect kJcWaterRect = { -2, -2, 2, 2, 0 };

/* =========================================================================
 * 0x00436200 -- JcWater_CalcCursor (JUNGLE CRUISE WATER cb_90).
 *
 * The placement-cursor calculator for one river square.  Unlike every other
 * class it ignores the class footprint and stamps the fixed 5x5 block into
 * the edit cursor, because a river square always covers 5x5 map cells.
 *
 * It then asks the river which of the four cardinal arms already exists at
 * (x, y) -- the square may only be laid where it joins one.  A square with no
 * arm at all is refused with error 14 ("cannot build here"); one that would
 * be laid on top of a visitor is refused with error 4 (or 3 when the people
 * test returns 1).  Otherwise every arm that exists gets one of the class's
 * four preview cursors, positioned five cells out in that arm's direction,
 * and the used ones are chained onto the edit cursor so the whole join is
 * drawn under the mouse.
 *
 * The owning station's square that ProbeRiver reports is DISCARDED here (the
 * local lives in the dead `sy` argument slot); only the arm mask matters.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00436200
void JcWater_CalcCursor(MapObj* o, int sx, int sy)
{
    WinRect area;
    int     owner;
    int     count;
    int     found;
    int     people;

    count = 0;
    g_edit_cursor_rect = kJcWaterRect;
    ScreenToMapRef(sx, &g_edit_cursor_origin, sy);
    found = JungleCruise_ProbeRiver(g_edit_cursor_origin.x,
                                    g_edit_cursor_origin.y, &owner);
    g_edit_cursor_next = 0;
    if (!found) {
        SetCursorError(&g_edit_cursor, 14);
        return;
    }

    ValidateCursor(&g_edit_cursor, o->cls);
    if (!CursorIsValid(&g_edit_cursor))
        return;

    area.left = g_edit_cursor_rect.left + g_edit_cursor_origin.x;
    area.top = g_edit_cursor_rect.top + g_edit_cursor_origin.y;
    area.right = g_edit_cursor_rect.right + g_edit_cursor_origin.x;
    area.bottom = g_edit_cursor_rect.bottom + g_edit_cursor_origin.y;
    people = CheckForPeople(&area);
    if (people != -1) {
        if (people != 1) {
            DefaultCursor(&g_jc_water_cursors[0]);
            DefaultCursor(&g_jc_water_cursors[1]);
            DefaultCursor(&g_jc_water_cursors[2]);
            DefaultCursor(&g_jc_water_cursors[3]);
            g_jc_water_cursors[0].rect = g_edit_cursor_rect;
            g_jc_water_cursors[1].rect = g_edit_cursor_rect;
            g_jc_water_cursors[2].rect = g_edit_cursor_rect;
            g_jc_water_cursors[3].rect = g_edit_cursor_rect;
            ResetCursorFootprint(&g_jc_water_cursors[0]);
            ResetCursorFootprint(&g_jc_water_cursors[1]);
            ResetCursorFootprint(&g_jc_water_cursors[2]);
            ResetCursorFootprint(&g_jc_water_cursors[3]);
            g_jc_water_cursors[0].flags = 0x2034;
            g_jc_water_cursors[1].flags = 0x2034;
            g_jc_water_cursors[2].flags = 0x2034;
            g_jc_water_cursors[3].flags = 0x2034;

            if (found & 1) {
                g_jc_water_cursors[count].origin.x = g_edit_cursor_origin.x;
                g_jc_water_cursors[count].origin.y = g_edit_cursor_origin.y - 5;
                count++;
            }
            if (found & 2) {
                g_jc_water_cursors[count].origin.x = g_edit_cursor_origin.x + 5;
                g_jc_water_cursors[count].origin.y = g_edit_cursor_origin.y;
                count++;
            }
            if (found & 4) {
                g_jc_water_cursors[count].origin.x = g_edit_cursor_origin.x;
                g_jc_water_cursors[count].origin.y = g_edit_cursor_origin.y + 5;
                count++;
            }
            if (found & 8) {
                g_jc_water_cursors[count].origin.x = g_edit_cursor_origin.x - 5;
                g_jc_water_cursors[count].origin.y = g_edit_cursor_origin.y;
                count++;
            }
            if (count != 0) {
                g_edit_cursor_next = &g_jc_water_cursors[0];
                if (count > 1) {
                    int j;
                    for (j = 1; j < count; j++)
                        g_jc_water_cursors[j - 1].next = &g_jc_water_cursors[j];
                }
            }
        } else {
            SetCursorError(&g_edit_cursor, 3);
        }
    } else {
        SetCursorError(&g_edit_cursor, 4);
    }
}


/* ---- the river subsystem (LEGOLAND/junglecruise.c) ----------------------
 * The three passes that lay or re-lay one 5x5 river square.  `owner` is the
 * owning station's packed map square; UpdateRiverTile writes it and the other
 * two read it, which is why the caller keeps ONE int for it and hands its
 * address round.  A null `owner` (the neighbour passes below) means "do not
 * report it back". */
extern void  JungleCruise_UpdateRiverTile(int x, int y, int mask, void* owner); /* 0x00436dc0 */
extern void  JungleCruise_RelinkRiverCell(int x, int y, void* owner);          /* 0x004367b0 */
extern void* JungleCruise_BuildRoute(int x0, int y0, int x1, int y1);          /* 0x004371e0 */
extern void  JungleCruise_RebuildRoute(int id);                                /* 0x004373c0 */

extern void  IncrementObjectCount(ObjDef* cls);              /* 0x00480d40 */

/* One jungle cruise station (0x44 bytes; see ridecb2.c for the full record).
 * Only the identity, the two route endpoints, the laid route and the list
 * link matter here. */
typedef struct JcStation {
    BPosW          pos;             /* +0x00  the station's own map square */
    unsigned char  ax;              /* +0x02  route start x */
    unsigned char  ay;              /* +0x03  route start y */
    unsigned char  bx;              /* +0x04  route end x */
    unsigned char  by;              /* +0x05  route end y */
    unsigned char  pad06[2];
    void*          route;           /* +0x08  the laid boat route */
    unsigned char  pad0c[0x18 - 0x0c];
    void*          blokes[5];       /* +0x18  the five queue slots */
    unsigned char  pad2c[0x3c - 0x2c];
    struct JcStation* next;         /* +0x3c */
    int            take;            /* +0x40  the ride's accumulated value */
} JcStation;                        /* 0x44 */

extern JcStation* g_jc_stations;    /* 0x00629c3c */

/* =========================================================================
 * 0x004365f0 -- JcWater_Add (JUNGLE CRUISE WATER cb_add).
 *
 * Lays one river square at the cursor's map cell and re-stitches everything
 * it touches.  The three-pass idiom the whole river uses is
 *
 *     mask = ProbeRiver(x, y, &owner);       which arms exist, and whose
 *     UpdateRiverTile(x, y, mask, &owner);   repaint the 5x5 block
 *     RelinkRiverCell(x, y, &owner);         fill the 2x2 inside corners
 *
 * run once for the new square and then once for EACH arm that exists, five
 * cells out (1 = north, 2 = east, 4 = south, 8 = west) -- a neighbour's tile
 * has to be repainted too because it has just gained a link.  The neighbour
 * passes hand UpdateRiverTile a NULL owner (they do not need it reported
 * back) but still hand RelinkRiverCell the owner the CENTRE square left in
 * the local -- so the corners of a neighbour are stitched as if they belonged
 * to the new square's station.  That is the original's behaviour, and it is
 * why laying river between two different cruises can stitch a corner across
 * the boundary.
 *
 * Finally, if the owning station is on the list, its boat route is laid again
 * from its two endpoint cells and, when that succeeds, the whole ride's river
 * link pass is re-run.
 *
 * NOTE: `owner` is only ever written by the CENTRE ProbeRiver/UpdateRiverTile
 * pair; the neighbour passes overwrite it through their own ProbeRiver call,
 * so the station looked up at the end is the one owning the LAST arm probed,
 * not necessarily the new square's.  Reproduced as-is.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004365f0
void JcWater_Add(MapObj* o, Pos* p)
{
    JcStation* st = g_jc_stations;
    int        owner;
    int        mask;

    mask = JungleCruise_ProbeRiver(p->x, p->y, &owner);
    JungleCruise_UpdateRiverTile(p->x, p->y, mask, &owner);
    IncrementObjectCount(o->cls);
    JungleCruise_RelinkRiverCell(p->x, p->y, &owner);

    if (mask & 1) {
        JungleCruise_UpdateRiverTile(p->x, p->y - 5,
            JungleCruise_ProbeRiver(p->x, p->y - 5, &owner), 0);
        JungleCruise_RelinkRiverCell(p->x, p->y - 5, &owner);
    }
    if (mask & 2) {
        JungleCruise_UpdateRiverTile(p->x + 5, p->y,
            JungleCruise_ProbeRiver(p->x + 5, p->y, &owner), 0);
        JungleCruise_RelinkRiverCell(p->x + 5, p->y, &owner);
    }
    if (mask & 4) {
        JungleCruise_UpdateRiverTile(p->x, p->y + 5,
            JungleCruise_ProbeRiver(p->x, p->y + 5, &owner), 0);
        JungleCruise_RelinkRiverCell(p->x, p->y + 5, &owner);
    }
    if (mask & 8) {
        JungleCruise_UpdateRiverTile(p->x - 5, p->y,
            JungleCruise_ProbeRiver(p->x - 5, p->y, &owner), 0);
        JungleCruise_RelinkRiverCell(p->x - 5, p->y, &owner);
    }

    while (st) {
        if (st->pos.w == (unsigned short)owner) {
            int x0 = st->ax;
            int y0 = st->ay;
            int x1 = st->bx;
            int y1 = st->by;
            st->route = JungleCruise_BuildRoute(x0, y0, x1, y1);
            if (st->route)
                JungleCruise_RebuildRoute(owner);
            break;
        }
        st = st->next;
    }
}

/* =========================================================================
 * ENTRANCE 1 -- the park entrance turnstiles.
 *
 * ridecb1.c owns the entrance's DRAW handler (0x0042d9c0) and recovered the
 * geometry it sorts by; this is the other half, the per-visitor state machine
 * the entrance runs every tick, and the little sound helper it calls.
 *
 * The entrance's footprint rect (ObjDef +0x3c) gives the two x lines the
 * visitor is walked between:
 *
 *     tx = square.x + rect.left            the OUTSIDE line (left of the gate)
 *     ex = square.x + rect.right + 6       the INSIDE line (six tiles past the
 *                                          right edge, well inside the park)
 *     ty = square.y + rect.top             the row the turnstiles sit on
 *
 * and the four turnstile lanes are the four y offsets at 0x004b6654: a
 * visitor is assigned one of the two lanes of a PAIR by a coin flip, and
 * which pair by its direction of travel.  The lane offsets are exactly the
 * four bands ridecb1.c's Entrance1_Draw sorts the crowd into.
 * ========================================================================= */

typedef struct MapPoint { unsigned char x; unsigned char y; } MapPoint;

typedef struct SoundSource {
    int kind;                      /* +0x00  1 = follow a person, 2 = a square */
    int f04;                       /* +0x04  the person, for kind 1 */
    int x;                         /* +0x08 */
    int y;                         /* +0x0c */
} SoundSource;

typedef struct BPos2 { int x; int y; } BPos2;

/* A person as this handler sees it (ridecb1.c's Bloke, same offsets). */
typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;          /* +0x0e  low-level AI state (0 = idle) */
    unsigned char  pad10[4];
    int            f14;            /* +0x14  long-term action scratch */
    int            f18;            /* +0x18 */
    unsigned char  pad1c[0x24 - 0x1c];
    BPos2          target;         /* +0x24  walk target, 24.8 */
    unsigned char  pad2c[0x36 - 0x2c];
    unsigned char  f36;            /* +0x36  which way through the gate:
                                    *        1 = inwards, 2 = outwards */
    unsigned char  pad37[0x3a - 0x37];
    short          f3a;            /* +0x3a  which lane of the pair, 0 or 1 */
    unsigned char  pad3c[0x60 - 0x3c];
    unsigned char  action;         /* +0x60  state-machine step */
    unsigned char  pad61;
    unsigned short flags62;        /* +0x62  8 = using this ride */
    unsigned char  pad64[4];
    BPos2          world;          /* +0x68  world position, 24.8 */
    unsigned char  pad70[0x73 - 0x70];
    unsigned char  new_dir;        /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14];     /* +0x98  CalcMoveLine scratch */
} Bloke;

/* A rider slot on the class's list (rides.c's RiderNode). */
typedef struct RiderNode {
    struct RiderNode* next;        /* +0x00 */
    struct RiderNode* prev;        /* +0x04 */
    Bloke*            bloke;       /* +0x08 */
    unsigned short    ride_id;     /* +0x0c  packed {x,y} of the instance */
    unsigned short    pad0e;
} RiderNode;

/* The object record behind a class element -- the same 0xd0-byte ObjDef the
 * cursor code above uses, seen through the fields the ride code wants. */
typedef struct RideObject {
    unsigned char  pad00[0x3c];
    Rect           rect;           /* +0x3c  class footprint, in tiles */
    unsigned char  pad50[0xcc - 0x50];
    RiderNode*     riders;         /* +0xcc  live rider list of the class */
} RideObject;

typedef struct RideElem {
    char*          name;           /* +0x00 */
    char*          image;          /* +0x04 */
    unsigned int   flags;          /* +0x08 */
    RideObject*    data;           /* +0x0c */
} RideElem;

extern int   rand(void);                                     /* 0x0049e4b2 (CRT) */
extern void  AddBricks(int n);                               /* 0x004578a0 */
extern void  PlayMoneySFX(MapPoint* at, int which, int flags);/* 0x00453950 */
extern void* PlayInstanceOfSample(void* sample, int a, int b,
                                  SoundSource* src);         /* 0x00496d20 */
extern int   CalcMoveLine(BPos2 from, BPos2 to, void* path); /* 0x00480740 */
extern int   NewDirForAction(Bloke* b, unsigned char dir);   /* 0x004833d0 */
extern void  RemoveBlokeFromRide(RideObject* item, RiderNode* r); /* 0x0048a100 */
extern void  PopLongTermAction(Bloke* b);                    /* 0x0044ebd0 */

extern void* g_entrance_pay_sample;  /* 0x004b6670  loaded by the class cb_a4 */
extern int   g_entrance_fee;         /* 0x00832970  bricks per admission */
extern int   g_entrance_coin;        /* 0x00616110  alternating lane bias, 0/1 */

/* The four turnstile lane offsets (0x004b6654), as two pairs.  A visitor
 * heading INTO the park uses the far pair, one heading out the near pair. */
static const int kEntranceLaneIn[2]  = { 0x978, 0xd78 };
static const int kEntranceLaneOut[2] = { 0x878, 0xc78 };

/* =========================================================================
 * 0x0042d970 -- Entrance1_PlayPaySound.
 *
 * The turnstile click, sourced on the VISITOR (SoundSource kind 1 carries the
 * person at +0x04, kind 2 a map square), plus the money jingle at the gate's
 * square when the park actually charges for admission.
 *
 * The first argument is only used for the money effect: when admission is
 * free the square is never read.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0042d970
void Entrance1_PlayPaySound(MapPoint* at, Bloke* b)
{
    SoundSource src;

    src.kind = 1;
    src.f04 = (int)b;
    PlayInstanceOfSample(g_entrance_pay_sample, 0, 1, &src);
    if (g_entrance_fee)
        PlayMoneySFX(at, 1, 0);
}

/* =========================================================================
 * 0x0042dfa0 -- Entrance1_Tick (ENTRANCE 1 cb_a8).
 *
 * The visitor's four-step walk through a turnstile:
 *
 *   0   claim the gate (flags62 |= 8) and pick a DIRECTION by comparing the
 *       visitor's world x against the two gate lines: standing at or left of
 *       the outside line it is coming IN (f36 = 2), at or right of the inside
 *       line it is going OUT (f36 = 1).  Either way it flips the shared coin
 *       at 0x00616110 once per visitor and picks its lane from
 *       (rand() >> 8) + coin, so the two lanes of a pair are used
 *       alternately.  Then it walks to the far line of its pair -- an
 *       outbound visitor goes straight to action 50, an inbound one to
 *       action 1.
 *   1   pay: AddBricks(the admission fee) and the turnstile sound (only for
 *       the OUT direction, f36 == 1 -- an inbound visitor pays nothing), then
 *       walk to the other side of the gate and advance to action 2.
 *   2   done: leave the ride's rider list and pop the visitor's long-term
 *       action so its own AI takes over again.
 *  50   the outbound visitor's extra step: walk to three tiles short of the
 *       inside line, then become action 1.
 *
 * ORIGINAL QUIRKS reproduced:
 *  - a visitor standing BETWEEN the two lines passes neither test, so f36
 *    keeps whatever value it had, action is never incremented and the visitor
 *    sits in action 0 for ever, re-flipping the coin every tick.
 *  - a visitor that passes BOTH tests (the rect is degenerate) has action
 *    incremented twice and then overwritten with 50 anyway.
 *  - action 50 sets no target y, so the move line is computed from the y this
 *    visitor was last given -- which is what makes the outbound walk a
 *    straight line down its lane.
 *  - the state machine never clears flags62 bit 3 the way every other ride's
 *    does, so a visitor that has been through the entrance keeps the "using a
 *    ride" bit set.
 *
 * VC6 NOTE: action 0 and action 50 end in the identical five statements, and
 * VC6 TAIL-MERGES them -- the shared block re-reads b->target from memory,
 * which is why action 0's stores look redundant.  Action 1 ends in the same
 * five statements plus `action++`, which is enough to stop the merge, so its
 * copy stays inline.  All three copies are written out here on purpose.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0042dfa0
void Entrance1_Tick(RideElem* elem)
{
    RideObject*   item = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    MapPoint*     key;
    const int*    tbl;
    int           tx;
    int           ty;
    int           ex;
    int           txs;
    int           exs;
    unsigned char a;

    r = item->riders;
    while (r) {
        next = r->next;
        key = (MapPoint*)&r->ride_id;
        b = r->bloke;
        tx = key->x + item->rect.left;
        ex = key->x + item->rect.right + 6;
        ty = key->y + item->rect.top;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                g_entrance_coin ^= 1;
                b->flags62 |= 8;
                txs = tx << 8;
                if (b->world.x <= txs) {
                    b->f36 = 2;
                    b->action++;
                    b->f3a = (short)(((rand() >> 8) + g_entrance_coin) & 1);
                }
                exs = ex << 8;
                if (b->world.x >= exs) {
                    b->f36 = 1;
                    b->action++;
                    b->f3a = (short)(((rand() >> 8) + g_entrance_coin) & 1);
                }
                if (b->f36 == 1) {
                    b->target.x = exs;
                    b->action = 50;
                    tbl = kEntranceLaneIn;
                } else {
                    b->target.x = txs;
                    tbl = kEntranceLaneOut;
                }
                b->target.y = tbl[b->f3a] + (ty << 8);
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                break;
            case 50:
                b->action = 1;
                b->target.x = (ex - 3) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                break;
            case 1:
                if (b->f36 == 1) {
                    AddBricks(g_entrance_fee);
                    Entrance1_PlayPaySound(key, b);
                    tbl = kEntranceLaneIn;
                    b->target.x = (tx << 8) - 0x80;
                } else {
                    tbl = kEntranceLaneOut;
                    b->target.x = ex << 8;
                }
                b->target.y = tbl[b->f3a] + (ty << 8);
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;
            case 2:
                RemoveBlokeFromRide(item, r);
                b->f14 = 0;
                b->f18 = 0;
                PopLongTermAction(b);
                break;
            }
        }

        r = next;
    }
}

/* =========================================================================
 * JUNGLE CRUISE MONKEY FISH -- one of the ride's three decorations.
 *
 * A monkey fish is a 0x0c-byte record on g_jc_fish, allocated when the piece
 * is placed and worth 2 to the ride's value (a water square or a monkey tree
 * is worth 1).  Like every jungle-cruise piece it remembers the map square of
 * the STATION that owns it, which is how the ride's teardown finds it again.
 * ========================================================================= */

typedef struct JcMonkeyFish {
    BPosW                pos;       /* +0x00  its own map square */
    BPosW                owner;     /* +0x02  the owning station's square */
    int                  f04;       /* +0x04  cleared on creation */
    struct JcMonkeyFish* next;      /* +0x08 */
} JcMonkeyFish;                     /* 0x0c */

extern JcMonkeyFish* g_jc_fish;     /* 0x00629c30 */
extern ObjDef*       g_jc_fish_cls; /* 0x0081cb74  JUNGLE CRUISE MONKEY FISH */

/* Every piece of a jungle cruise is worth something to the ride; the class's
 * cb_c0 reports the largest total in the park. */
extern void  JungleCruise_AddValue(BPosW id, int delta);     /* 0x00436130 */
extern void  AddBasicObject(void* o, Pos* p);                /* 0x0045efe0 */
extern void  SetMapTile(int x, int y, unsigned short tile);  /* 0x00461780 */
extern void* HeapAlloc_w(unsigned int size);                 /* 0x0049e4ff */

/* The ride's tileset handle; its first tile id is two indirections in.  It is
 * re-read at every call site (never cached) because SetMapTile may move the
 * tile tables -- the original reloads 0x0081cb58 each time. */
extern void* g_jc_tsm;              /* 0x0081cb58 */
#define JC_TILE0  (**(unsigned short**)((char*)g_jc_tsm + 4))

/* =========================================================================
 * 0x00434100 -- MonkeyFish_Add (JUNGLE CRUISE MONKEY FISH cb_add).
 *
 * Places a monkey fish: work out which station's river it belongs to, hang a
 * record off g_jc_fish, add its value to that station, put the object on the
 * map and then paint its footprint from the ride's tileset.
 *
 * FINDING THE OWNER.  The piece must sit ON the river, so the owner is simply
 * whatever ProbeRiver reports for the cell -- and if that cell has no river
 * arms at all, the cell FIVE NORTH is probed instead, which is where a piece
 * dropped on the square just below a river square finds it.  If neither probe
 * finds anything `owner` is left UNINITIALISED and the record is filed under
 * whatever was on the stack: an original bug, kept.
 *
 * THE FOOTPRINT PAINT.  The class rect (ObjDef +0x3c) is filled cell by cell
 * from the tileset's first tile, with the border cells getting edge tiles:
 *
 *     +0  interior          +9  west edge     +0xa  north edge
 *     +5  north-west        +6  south-west    +0xb  south edge
 *     +7  south-east        +8  north-east    +0xc  east edge
 *
 * The edge tests are ordered x-first, so on a one-cell-wide rect every cell
 * gets the WEST tile and the north/south rows never appear; the four corners
 * are then stamped over the top unconditionally, which is what actually makes
 * a 1x1 or 2x2 fish look right.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00434100
void MonkeyFish_Add(void* o, Pos* p)
{
    BPosW         key;
    BPosW         owner;
    JcMonkeyFish* f;

    key.b.x = (unsigned char)p->x;
    key.b.y = (unsigned char)p->y;
    if (!JungleCruise_ProbeRiver(p->x, p->y, &owner))
        JungleCruise_ProbeRiver(p->x, p->y - 5, &owner);

    f = (JcMonkeyFish*)HeapAlloc_w(sizeof(JcMonkeyFish));
    if (f) {
        int x;
        int y;

        f->pos = key;
        f->owner = owner;
        f->next = g_jc_fish;
        f->f04 = 0;
        g_jc_fish = f;
        JungleCruise_AddValue(f->owner, 2);
        AddBasicObject(o, p);

        for (y = g_jc_fish_cls->rect.top; y <= g_jc_fish_cls->rect.bottom; y++) {
            for (x = g_jc_fish_cls->rect.left; x <= g_jc_fish_cls->rect.right; x++) {
                if (x == g_jc_fish_cls->rect.left)
                    SetMapTile(p->x + x, p->y + y, (unsigned short)(JC_TILE0 + 9));
                else if (x == g_jc_fish_cls->rect.right)
                    SetMapTile(p->x + x, p->y + y, (unsigned short)(JC_TILE0 + 0xc));
                else if (y == g_jc_fish_cls->rect.top)
                    SetMapTile(p->x + x, p->y + y, (unsigned short)(JC_TILE0 + 0xa));
                else if (y == g_jc_fish_cls->rect.bottom)
                    SetMapTile(p->x + x, p->y + y, (unsigned short)(JC_TILE0 + 0xb));
                else
                    SetMapTile(p->x + x, p->y + y, JC_TILE0);
            }
        }

        SetMapTile(g_jc_fish_cls->rect.left + p->x, g_jc_fish_cls->rect.top + p->y,
                   (unsigned short)(JC_TILE0 + 5));
        SetMapTile(g_jc_fish_cls->rect.right + p->x, g_jc_fish_cls->rect.top + p->y,
                   (unsigned short)(JC_TILE0 + 8));
        SetMapTile(g_jc_fish_cls->rect.left + p->x, g_jc_fish_cls->rect.bottom + p->y,
                   (unsigned short)(JC_TILE0 + 6));
        SetMapTile(g_jc_fish_cls->rect.right + p->x, g_jc_fish_cls->rect.bottom + p->y,
                   (unsigned short)(JC_TILE0 + 7));
    }
}

/* =========================================================================
 * THE JUNGLE CRUISE SAVEGAME
 *
 * The ride's five lists are written one group after another, each as
 *
 *     int32 count;   count x <raw record>
 *
 * -- unlike every ride in ridesave.c, which writes `{int32 1; record}`* and
 * an int32 0 terminator.  The two lists that carry POINTERS to visitors are
 * copied into a stack buffer first so the pointers can be turned into bloke
 * INDICES (GetBlokeNum) without touching the live record; the other three are
 * written straight out of the heap.
 *
 * The group order, and the sizes, are exactly what LoadJungleCruise
 * (0x00435ec0, ridecb2.c) reads back:
 *
 *     stations   0x44 bytes, next @ +0x3c, 5 bloke slots @ +0x18
 *     water      0x1c bytes, next @ +0x10
 *     fish       0x0c bytes, next @ +0x08
 *     trees      0x08 bytes, next @ +0x04
 *     boats      0x3f8 bytes, next @ +0x3f4, 3 bloke slots @ +0x3e8
 *
 * Every group is counted by WALKING the list first and then walked a second
 * time to write it, and the write loop is driven by the count -- so a list
 * that changes between the two walks would desynchronise.  No SaveGameWrite
 * result is ever checked; the function always returns 1.
 * ========================================================================= */

typedef struct JcWater {
    unsigned char   pad00[0x10];
    struct JcWater* next;           /* +0x10 */
    unsigned char   pad14[0x1c - 0x14];
} JcWater;                          /* 0x1c */

typedef struct JcMonkeyTree {
    unsigned char        pad00[4];
    struct JcMonkeyTree* next;      /* +0x04 */
} JcMonkeyTree;                     /* 0x08 */

typedef struct JcBoat {
    unsigned char  pad00[0x3e8];
    void*          blokes[3];       /* +0x3e8  the party aboard */
    struct JcBoat* next;            /* +0x3f4 */
} JcBoat;                           /* 0x3f8 */

extern JcWater*       g_jc_water;   /* 0x0062fd2c */
extern JcMonkeyTree*  g_jc_trees;   /* 0x00629c2c */
extern JcBoat*        g_jc_boats;   /* 0x00616164 */

extern int SaveGameWrite(const void* buf, unsigned int n); /* 0x0047d760 */
extern int GetBlokeNum(void* bloke);                       /* 0x00482fb0 */

// FUNCTION: LEGOLAND 0x00435c70
int SaveJungleCruise(void)
{
    int            n;
    JcStation      st;
    JcBoat         bt;
    JcStation*     sp;
    JcStation*     sq;
    JcWater*       w;
    JcWater*       wq;
    JcMonkeyFish*  f;
    JcMonkeyFish*  fq;
    JcMonkeyTree*  t;
    JcMonkeyTree*  tq;
    JcBoat*        b;
    JcBoat*        bq;
    void**         q;
    int            i;

    n = 0;
    for (sq = g_jc_stations; sq; sq = sq->next)
        n++;
    sp = g_jc_stations;
    SaveGameWrite(&n, 4);
    while (n-- != 0) {
        st = *sp;
        q = st.blokes;
        i = 5;
        do {
            *q = (void*)GetBlokeNum(*q);
            q++;
        } while (--i);
        SaveGameWrite(&st, sizeof(st));
        sp = sp->next;
    }

    n = 0;
    for (wq = g_jc_water; wq; wq = wq->next)
        n++;
    w = g_jc_water;
    SaveGameWrite(&n, 4);
    while (n-- != 0) {
        SaveGameWrite(w, sizeof(*w));
        w = w->next;
    }

    n = 0;
    for (fq = g_jc_fish; fq; fq = fq->next)
        n++;
    f = g_jc_fish;
    SaveGameWrite(&n, 4);
    while (n-- != 0) {
        SaveGameWrite(f, sizeof(*f));
        f = f->next;
    }

    n = 0;
    for (tq = g_jc_trees; tq; tq = tq->next)
        n++;
    t = g_jc_trees;
    SaveGameWrite(&n, 4);
    while (n-- != 0) {
        SaveGameWrite(t, sizeof(*t));
        t = t->next;
    }

    n = 0;
    for (bq = g_jc_boats; bq; bq = bq->next)
        n++;
    b = g_jc_boats;
    SaveGameWrite(&n, 4);
    while (n-- != 0) {
        bt = *b;
        q = bt.blokes;
        i = 3;
        do {
            *q = (void*)GetBlokeNum(*q);
            q++;
        } while (--i);
        SaveGameWrite(&bt, sizeof(bt));
        b = b->next;
    }

    return 1;
}
