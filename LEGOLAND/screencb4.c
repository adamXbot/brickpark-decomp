/* LEGOLAND -- the last three unnamed class callbacks from screen.c's
 * SetCustomCallbacks table (the fourth file; screencb.c holds fifteen,
 * screencb2.c seventeen and screencb3.c eight).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field offsets, global addresses and callee arg counts are load-bearing;
 * names are ours.
 *
 * ---------------------------------------------------------------------------
 * WHAT THESE ARE
 * ---------------------------------------------------------------------------
 * screen.c's SetCustomCallbacks (0x00452c20) runs once per class element as
 * the object database is loaded, matches the element's class NAME with the
 * CRT _stricmp at 0x004aab90 and stores a run of function pointers into the
 * class's 0xd0-byte ObjDef.  The slots (the numbering docs/RIDE_CALLBACKS.md
 * uses) are:
 *
 *     +0x8c tick/select   +0x90 update      +0x94 draw-selection
 *     +0x98 add           +0x9c remove      +0xa0 draw-descriptor
 *     +0xa4 create        +0xa8 activate    +0xac destroy
 *     +0xb0 interact      +0xb8 load        +0xbc save        +0xc0 extra
 *
 * Each function here is named from the class arm and the slot it is stored
 * into, read straight back out of SetCustomCallbacks:
 *
 *   0x00405570  DRIVING SCHOOL   +0x8c  DrivingSchool_SelectForPlacement
 *                                       (arm at 0x00452dcd, class string
 *                                       0x004b80b0 "DRIVING SCHOOL")
 *   0x004058a0  DRIVING SCHOOL   +0x94  DrivingSchool_DrawSelection
 *                                       (same arm, 0x00452deb)
 *   0x00414950  ZEBRA CROSSING   +0x98  ZebraCrossing_Add
 *                                       (arm at 0x00452ec7, class string
 *                                       0x004b89bc "ZEBRA CROSSING")
 *
 * The DRIVING SCHOOL arm is the widest in the whole table -- thirteen slots
 * (0x00452dcd..0x00452e4f) -- and the two taken here were the only ones still
 * unnamed; the other eleven live in screencb.c (+0x9c), screencb2.c (+0xa4,
 * +0x90, +0x98, +0xb0, +0xac), ridecb8.c (+0xa0, +0xbc, +0xb8, +0xc0) and
 * castleobj.c (+0xa8).  ZEBRA CROSSING has six, and shares +0x94
 * (0x00413fa0) and +0x9c (0x00414220, ridecb6.c's Roads_Remove) with DRIVING
 * SCHOOL ROADS -- the +0x9c sharing is ridecb6.c's proof that a crossing is a
 * BIT on a road block, and this file's +0x98 is the write that sets it.
 *
 * ---------------------------------------------------------------------------
 * THE DRIVING SCHOOL'S FOUR-CURSOR PREVIEW  (recovered here)
 * ---------------------------------------------------------------------------
 * The driving school is the only class in the game that previews FOUR
 * footprints at once.  Placement cursors are 0x1834-byte blocks chained
 * through `next` (+0x1830), and the school owns three ghosts beside the edit
 * cursor:
 *
 *     g_edit_cursor  0x007febc0   the class footprint under the mouse
 *     g_ds_prev_a    0x0082f760   { 0, -4, 8,  7}  the car-park apron
 *     g_ds_prev_b    0x0082c6e0   {-3, -4, 0, -1}  the pump bay to its west
 *     g_ds_prev_c    0x0082df20   {-1,  0, 4,  8}  the track below it
 *
 * +0x8c (select) is what ARMS that preview: it points the editor at the
 * class, resets all four cursors, gives each ghost its fixed rect and
 * chains edit -> A -> B.  +0x90 (screencb2.c's DrivingSchool_Update) then
 * re-runs every frame, re-anchors the three ghosts on the mouse square and
 * completes the chain with B -> C.
 *
 * ORIGINAL BUG, reproduced.  The select handler chains only TWO links
 * (edit -> A, A -> B) and never B -> C, so between the click on the palette
 * icon and the first update frame the track ghost is orphaned -- and, worse,
 * `g_ds_prev_b.next` is whatever the previous placement left there.  It is
 * masked because +0x90 runs before the cursor is ever rendered.  The two
 * ghost FLAG words the select arms (A |= 0x100, B |= 0x200; C gets none) are
 * likewise immediately overwritten by +0x90's 0x4108 / 0x4208 / 0x5008, so
 * the select's flag bits only ever survive on a frame that is never drawn.
 *
 * ---------------------------------------------------------------------------
 * WHAT A ZEBRA CROSSING REALLY IS  (confirmed from the +0x98 side)
 * ---------------------------------------------------------------------------
 * ridecb6.c recovered from the REMOVE side that a crossing is not an object
 * but BIT 0x10 of a road block's `kind` byte.  ZebraCrossing_Add is the
 * matching write: it looks the 4x4 block up by EXACT origin (Road_FindAt,
 * not GetRoadRecord -- ZebraCrossing_Update has already snapped the cursor
 * onto the block's own square), sets the bit, re-stitches the tile under the
 * block's OWN school id, and clears the two scratch bytes at +0x1c/+0x1d
 * that NewRoadRecord also zeroes.  No record of any kind is allocated, and
 * the object count that is incremented is the ZEBRA CROSSING class's own --
 * which is what Roads_Remove hands back when it clears the bit again.
 *
 * ORIGINAL QUIRK, reproduced: the handler calls Road_FindCardinals into a
 * local eight-pointer ring array and then reads neither the ring nor the
 * count.  The call is a leftover -- laying a crossing changes no
 * neighbour's tile -- but it is a real call with a real 0x20-byte frame, so
 * it stays.
 *
 * ---------------------------------------------------------------------------
 * CODEGEN LEVERS RECOVERED HERE
 * ---------------------------------------------------------------------------
 * `or ah,1` / `or dh,2` on a DWORD flags global is `|= 0x100` / `|= 0x200`.
 * VC6 loads the whole dword, ORs the SECOND BYTE of the register and stores
 * the whole dword back -- the same byte-narrowing it does for `|= 0x100` on
 * an `unsigned short`, extended to a 32-bit destination that has to be
 * written in full.  The load/store pair is not optional: a plain
 * `or dword ptr [mem],8` never appears in this family (compare
 * ridecb8.c's Roads_SelectForPlacement, which spells `|= 8` and gets
 * `mov ecx,[mem] / or ecx,8 / mov [mem],ecx`).
 * ------------------------------------------------------------------------- */

/* ---- geometry ------------------------------------------------------------ */
typedef struct BPos  { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; unsigned char c[2]; } BPosW;
typedef struct Pos   { int x; int y; } Pos;

/* The class geometry rect; sub-rects chain through `next`. */
typedef struct Rect {
    int          left;              /* +0x00 */
    int          top;               /* +0x04 */
    int          right;             /* +0x08 */
    int          bottom;            /* +0x0c */
    struct Rect* next;              /* +0x10 */
} Rect;                             /* 0x14 */

/* An object-class definition; only the footprint is read here. */
typedef struct RideDef {
    unsigned char pad00[0x3c];
    Rect          footprint;        /* +0x3c the class footprint rect */
    unsigned char pad50[0xd0 - 0x50];
} RideDef;                          /* 0xd0 */

/* A placed map object; only its class is read here. */
typedef struct MapObj {
    unsigned char pad00[0x0c];
    RideDef*      cls;              /* +0x0c */
    unsigned char pad10[4];
} MapObj;                           /* 0x14 */

/* The placement / edit cursor (objmap2.c's Cursor, same offsets as
 * ridecb5.c and ridecb8.c). */
typedef struct Cursor {
    unsigned char  pad0000[0x1404];
    Pos            origin;          /* +0x1404 map square the footprint hangs off */
    unsigned char  pad140c[0x1414 - 0x140c];
    Rect           rect;            /* +0x1414 the footprint being previewed */
    unsigned char  pad1428[0x1828 - 0x1428];
    unsigned int   flags;           /* +0x1828 */
    unsigned char  pad182c[4];
    struct Cursor* next;            /* +0x1830 chained preview cursors */
} Cursor;                           /* 0x1834 */

/* One 4x4 road block (ridecb5.c / roads2.c own the full field list). */
typedef struct RoadRec {
    struct RoadRec* next;           /* +0x00 */
    unsigned char   pad04[4];
    BPosW           school;         /* +0x08 the owning driving school */
    unsigned char   pad0a[2];
    int             x;              /* +0x0c the block origin, a multiple of 4 */
    int             y;              /* +0x10 */
    unsigned char   kind;           /* +0x14 shape | rot<<5, bit 0x10 = crossing */
    unsigned char   pad15[0x1c - 0x15];
    unsigned char   f1c;            /* +0x1c scratch, zeroed with the record */
    unsigned char   f1d;            /* +0x1d */
    unsigned char   pad1e[2];
} RoadRec;                          /* 0x20 */

/* ---- the editor and the four placement cursors --------------------------- */
extern int      g_edit_changed;     /* 0x008119b0 */
extern RideDef* g_edit_object;      /* 0x008119b8 the class the cursor edits */
extern Cursor   g_edit_cursor;      /* 0x007febc0 */
extern Cursor   g_ds_prev_a;        /* 0x0082f760 */
extern Cursor   g_ds_prev_b;        /* 0x0082c6e0 */
extern Cursor   g_ds_prev_c;        /* 0x0082df20 */

/* The DRIVING SCHOOL class definition, cached by its +0xa4 create. */
extern RideDef* g_ds_def;           /* 0x0082c694 */
/* The one global list of 4x4 road blocks. */
extern RoadRec* g_road_tiles;       /* 0x004cbeac */
/* The map square currently under the edit cursor (objmap2.c's g_sel_bpos). */
extern BPosW    g_sel_bpos;         /* 0x00667c54 */

extern void  DefaultCursor(Cursor* c);                        /* 0x0045a390 */
extern void  SetEditCursorFootPrint(void* footprint);         /* 0x0045f440 */
extern void  ResetCursorFootprint(Cursor* c);                 /* 0x0045f460 */
extern void  BuildCursorPtr(Cursor* c, int a, int b);         /* 0x0045f5f0 */
extern void  RenderCursor(Cursor* c);                         /* 0x0045ff00 */
extern void  BasicObjectDCalcCursor(void* elem, void* p);     /* 0x00480bb0 */

/* The three ghost rects (0x004b4440 / 0x004b4458 / 0x004b4470 -- the same
 * three objects screencb2.c's DrivingSchool_Update copies). */
static const Rect kDsPrevA = {  0, -4, 8,  7, 0 };
static const Rect kDsPrevB = { -3, -4, 0, -1, 0 };
static const Rect kDsPrevC = { -1,  0, 4,  8, 0 };

/* -------------------------------------------------------------------------
 * 0x00405570 -- DRIVING SCHOOL +0x8c: the palette icon was clicked, so arm
 * the four-cursor preview.  See the header for the two bugs this reproduces.
 * ------------------------------------------------------------------------- */
// FUNCTION: LEGOLAND 0x00405570
void DrivingSchool_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_ds_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);

    DefaultCursor(&g_ds_prev_a);
    g_ds_prev_a.rect = kDsPrevA;
    g_ds_prev_a.flags |= 0x100;

    DefaultCursor(&g_ds_prev_b);
    g_ds_prev_b.rect = kDsPrevB;
    g_ds_prev_b.flags |= 0x200;

    DefaultCursor(&g_ds_prev_c);
    g_ds_prev_c.rect = kDsPrevC;

    g_edit_cursor.next = &g_ds_prev_a;
    g_ds_prev_a.next = &g_ds_prev_b;
}

/* The 4x4 block one road record covers (0x004b4bf0, the object ridecb5.c
 * calls g_road_preview_rect). */
static const Rect kRoadBlockRect = { 0, 0, 3, 3, 0 };

/* -------------------------------------------------------------------------
 * 0x004058a0 -- DRIVING SCHOOL +0x94: light the whole circuit up when the
 * school is selected.  Instruction for instruction the shape of
 * screencb2.c's BoatingSchool_DrawSelection and ridecb9.c's
 * JungleCruise_DrawSelection -- read the child list BEFORE the cursor
 * calculation, reset the class's scratch cursor, stamp the fixed footprint
 * into it and render it once per child whose owner is the selected square --
 * with the road list as the children.  A road block carries its origin as
 * two INTS (+0x0c/+0x10), where a lake square and a river square carry a
 * packed BPos, so this is the only member of the family that does not widen
 * the coordinates.  The render mode is 0x18 rather than the water classes' 8.
 * ------------------------------------------------------------------------- */
// FUNCTION: LEGOLAND 0x004058a0
void DrivingSchool_DrawSelection(void* elem, void* p)
{
    RoadRec* t = g_road_tiles;

    BasicObjectDCalcCursor(elem, p);
    DefaultCursor(&g_ds_prev_a);
    g_ds_prev_a.rect = kRoadBlockRect;

    while (t) {
        if (t->school.w == g_sel_bpos.w) {
            g_ds_prev_a.origin.x = t->x;
            g_ds_prev_a.origin.y = t->y;
            ResetCursorFootprint(&g_ds_prev_a);
            g_ds_prev_a.flags = 0x18;
            BuildCursorPtr(&g_ds_prev_a, 0, 0);
            RenderCursor(&g_ds_prev_a);
        }
        t = t->next;
    }
}

/* ---- ZEBRA CROSSING +0x98 ------------------------------------------------ */

/* Returns the 4x4 road block whose ORIGIN is exactly (x, y) (ridecb5.c). */
extern RoadRec* Road_FindAt(int x, int y);                    /* 0x004125a0 */
/* Fills ring slots 0/2/4/6 with the four cardinal neighbours (ridecb5.c). */
extern int   Road_FindCardinals(int x, int y, RoadRec** ring);/* 0x004135d0 */
/* Re-pick the tile for the block at (x, y) of the school `school`. */
extern void  Road_Restitch(BPosW school, int x, int y);       /* 0x00413650 */
extern void  IncrementObjectCount(RideDef* cls);              /* 0x00480d40 */

/* -------------------------------------------------------------------------
 * 0x00414950 -- ZEBRA CROSSING +0x98: paint a crossing across the road block
 * under the cursor.  See the header for the dead ring probe.
 * ------------------------------------------------------------------------- */
// FUNCTION: LEGOLAND 0x00414950
void ZebraCrossing_Add(MapObj* obj, Pos* pos)
{
    RoadRec* ring[8];
    RoadRec* r;

    r = Road_FindAt(pos->x, pos->y);
    if (r) {
        Road_FindCardinals(pos->x, pos->y, ring);
        r->kind |= 0x10;
        Road_Restitch(r->school, pos->x, pos->y);
        r->f1d = 0;
        r->f1c = 0;
        IncrementObjectCount(obj->cls);
    }
}
