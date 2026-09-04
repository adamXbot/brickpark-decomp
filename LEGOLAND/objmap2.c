/* LEGOLAND — object placement, removal, cursor validation and the screen /
 * iso-plane projection.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; names are ours.
 *
 * ---------------------------------------------------------------------------
 * PLACEMENT RULES (what the map flags mean to the builder)
 *
 * A cell's map flags (Cell +0x0c) carry the object occupancy bits:
 *     0x08   object cell (AddBasicObject single-cell objects, AddBasicPath)
 *     0x10   path tile                        (AddPathTileGFX)
 *     0x20   render-chain "base" object cell  (SetObjRectFlags / AddObjectToMap
 *            callers pass it in `flags`; RemoveObjectFromMap clears 0xa0)
 *     0x40   cell blocked for building        (ValidateCursor error 1;
 *            BasicObjectDCalcCursor treats an off-map cell as 0x40)
 *     0x80   footprint cell of a placed object (AddObjectToMap sets it; the
 *            cell's +0x00 is then the placed object and the render order and
 *            GetObjectUID use it)
 *     0x800  cell needs a mechanic/no-build   (ValidateCursor error 5)
 *     0x4000 outstanding repair order         (RemObjFromMap erases it)
 *     0x8000 class flag 0x800000 -> "tall" cell (AddObjectToMap/AddBasicObject)
 *
 * The RF byte (Cell +0x10) is written from the class flags: class bit 2 ->
 * rf = 2, class bit 1 -> rf = 1 (bit 1 wins), and a footprint cell written by
 * SetObjRectFlags is rf = 2.  The edit-cursor validation (ValidateCursor)
 * walks every rect of every chained cursor and calls SetCursorError(cursor, n)
 * with, in order of discovery: 4/3 (people in the footprint, CheckForPeople
 * -1/+1), 7 (off the map), 1 (blocked cell), 10 (an existing object under the
 * footprint whose class is NOT the environment class and does NOT carry flag
 * 0x200000 — 0x200000 is the "may be built over" bit, and error 10 skips
 * straight to the path check), 6 (any other object under a class that does not
 * need one), 5 (0x800 cell), 9 (a class that needs a path but the cell is a
 * path tile without rf bit 0 — i.e. not a walkable path) and 8 (a class that
 * must NOT sit on a path, over a path tile or rf bit 0).
 * SetCursorError keeps the WORST (lowest -n) code in +0x140c/+0x1410.
 *
 * RENDER ORDER (CalculateMapRenderOrder)
 *
 * Every "base" cell (flags 0xa0: a footprint origin or an object cell) becomes
 * a render node in the 0x1000-entry table at 0x00807f60 (8 bytes: {live,
 * bpos, x, y}).  The map is scanned column by column, top to bottom; at each
 * base cell the object is linked into the render chain (Cell +0x06 packed
 * next-coordinate, head at 0x007febb8) when the scan has reached the RIGHT
 * edge of its footprint (x == bx + rect.right, or the last column), and the
 * scan then jumps back to the node's stored (x, bottom+1) — otherwise the node
 * only records where the scan must resume once the column advances.  The
 * result is the diagonal painter's order the isometric renderer walks with
 * GetFirstRenderObject / GetNextRenderObject.
 *
 * OBJECT UID LOOKUP (GetObjectUID)
 *
 * The reverse mapping — 24.8 world position + class -> the base cell of the
 * placed object under it — probes the FOUR NEIGHBOURS of the cell, never the
 * cell itself: above (y-1), below (y+1), left (x-1) and right (x+1), in that
 * order, accepting the first whose cell carries flag 0x80, holds an object of
 * that exact class, and whose base coordinates plus the class footprint offset
 * (ObjDef +0x0c/+0x10) land back on the queried cell.  Two quirks are part of
 * the original's behaviour, not of the reconstruction: the below-probe is
 * nested inside the above-probe's null check, so when the cell above is off
 * the map (y == 0, or x off the map) the cell below is never examined; and
 * that shared null check is the only one the below-probe gets, so an off-map
 * cell below dereferences a null pointer.  See the note on GetObjectUID.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* A packed 2-byte map coordinate passed BY VALUE (same shape as power.c's
 * BPos and buildtick.c's BuildTile). */
typedef struct BPos {
    unsigned char x;            /* +0x00 */
    unsigned char y;            /* +0x01 */
} BPos;

/* The same pair read as one 16-bit word straight from a global. */
typedef union BPosW {
    unsigned short w;
    BPos           b;
} BPosW;

/* An object class / definition (the 0xd0-byte ODF record). */
typedef struct ObjDef {
    char           pad0[0x0c];   /* +0x00 */
    int            dx;           /* +0x0c  GetObjectUID: footprint offset */
    int            dy;           /* +0x10 */
    char           pad14[0x1c - 0x14];
    unsigned int   flags;        /* +0x1c */
    short          type;         /* +0x20  2 = no instance record */
    char           pad22[0x2c - 0x22];
    unsigned char  life;         /* +0x2c  initial cell life */
    char           pad2d[0x3c - 0x2d];
    Rect           rect;         /* +0x3c  footprint rect list */
    char           pad50[0x74 - 0x50];
    unsigned short* tile;        /* +0x74  single-cell tile code */
    char           pad78[0x90 - 0x78];
    void         (*effect)(void* c4, Pos* pt, int effect); /* +0x90 */
    char           pad94[0x9c - 0x94];
    void         (*remove)(void* obj, BPos bp, void* ctx); /* +0x9c */
    char           pada0[0xc4 - 0xa0];
    void*          c4;           /* +0xc4 */
    char           padc8[0xd0 - 0xc8];
} ObjDef;

/* A placed map object: its class sits at +0x0c. */
typedef struct MapObj {
    char    pad0[0x0c];          /* +0x00 */
    ObjDef* cls;                 /* +0x0c */
} MapObj;

/* An edit / destroy cursor block (0x1834 bytes; 0x007febc0 is the edit cursor
 * and 0x00810160 the destroy cursor, each the head of a chain via +0x1830).
 * BuildCursorPtr fills the outline point list at the top. */
typedef struct Cursor {
    unsigned short count;        /* +0x0000 outline points used */
    short          px[0x400];    /* +0x0002 */
    short          py[0x400];    /* +0x0802 */
    unsigned char  kind[0x400];  /* +0x1002 style | 1 (left/right) or | 2 (top/bottom) */
    unsigned char  pad1402[2];
    Pos            origin;       /* +0x1404 map cell the footprint hangs off */
    int            status;       /* +0x140c 1 = valid, -n = error n */
    int            error;        /* +0x1410 */
    Rect           rect;         /* +0x1414 footprint rect list */
    unsigned char  style;        /* +0x1428 4 when the cursor is valid */
    char           pad1429[0x1828 - 0x1429];
    unsigned int   flags;        /* +0x1828 */
    int            f182c;        /* +0x182c */
    struct Cursor* next;         /* +0x1830 */
} Cursor;

/* One render-order node (0x00807f60, 0x1000 entries of 8 bytes). */
typedef struct RenderNode {
    int            live;         /* +0x00 */
    unsigned short bpos;         /* +0x04 packed base cell */
    unsigned char  x;            /* +0x06 column to resume at */
    unsigned char  y;            /* +0x07 row to resume at */
} RenderNode;

/* A sound "source" descriptor: kind 2 = a map tile at (x,y) (buildtick.c). */
typedef struct SoundSource {
    int kind;                    /* +0x00 */
    int pad4;                    /* +0x04 */
    int x;                       /* +0x08 */
    int y;                       /* +0x0c */
} SoundSource;

typedef struct WinRect {
    long left;
    long top;
    long right;
    long bottom;
} WinRect;

/* The map header fields the projection reads: the viewport origin. */
typedef struct MapHdr {
    char           pad0[0x20];
    unsigned short origin_x;     /* +0x20 */
    unsigned short origin_y;     /* +0x22 */
} MapHdr;

/* A tile element with the enter/leave callbacks (SP* module callbacks). */
typedef struct SPTileElem {
    char pad0[0x18];
    unsigned char (*rfflags)(int x, int y);       /* +0x18 */
    void          (*enter)(unsigned x, unsigned y); /* +0x1c */
    void          (*leave)(unsigned x, unsigned y); /* +0x20 */
} SPTileElem;
typedef struct SPTileInfo {
    SPTileElem*  elem;           /* +0x00 */
    unsigned int code;           /* +0x04 */
} SPTileInfo;
extern SPTileInfo g_sp_tile_info[];      /* 0x00801f40 */

/* ---------------------------------------------------------------- globals -- */

extern int        g_bg_full_update;      /* 0x004b9220 (BGFullUpdate) */
extern int        g_map_loading;         /* 0x00667cd8 suppress render-order rebuilds */
extern int        g_render_order_dirty;  /* 0x00667cdc */
extern int        g_build_in_progress;   /* 0x00667ca0 */
extern ObjDef*    g_sel_def;             /* 0x00667c58 class under the destroy cursor */
extern BPosW      g_sel_bpos;            /* 0x00667c54 its base cell */
extern Cursor     g_edit_cursor;         /* 0x007febc0 */
extern Cursor     g_destroy_cursor;      /* 0x00810160 */
extern RenderNode g_render_nodes[0x1000];/* 0x00807f60 */
extern int        g_render_node_next;    /* 0x00801408 */
extern unsigned short g_render_head;     /* 0x007febb8 */
extern int        g_scroll_x;            /* 0x00667cb4 (24.8) */
extern int        g_scroll_y;            /* 0x00667cb8 */
extern void*      g_env_class;           /* 0x007fd624 */
extern void*      g_placing_obj;         /* 0x0080ff64 */
extern int        g_placed_flag;         /* 0x0079a8d0 */
extern int        g_count_env;           /* 0x00667cf4 */
extern int        g_area_total;          /* 0x00667ce0 */
extern int        g_have_special;        /* 0x00667d0c */
extern int        g_area_type1;          /* 0x00667ce4 */
extern int        g_area_type2;          /* 0x00667cf8 */
extern int        g_area_type3;          /* 0x00667cf0 */
extern int        g_area_type4;          /* 0x00667ce8 */
extern int        g_area_type5;          /* 0x00667cec */
extern int        g_map_dirty;           /* 0x00668610 */
extern int        g_remove_sample;       /* 0x004b9248 */

/* ---------------------------------------------------------------- callees -- */

extern void  IncrementObjectCount(ObjDef* d);                  /* 0x00480d40 */
extern void  DecrementObjectCount(ObjDef* d);                  /* 0x00480d60 */
extern void  ApplyConsTileMap(MapObj* obj, BPos bp);           /* 0x0045efc0 */
extern void  ApplyDestrTileMap(MapObj* obj, BPos bp);          /* 0x0045efd0 */
extern void* CreateObjectInstance(ObjDef* d, BPos* key);       /* 0x004816a0 */
extern void  AddInstanceToList(void* inst);                    /* 0x0048a010 */
extern void* GetInstanceOfClass(ObjDef* d, BPos* key);         /* 0x0048a0c0 */
extern void  RemoveInstanceFromList(void* inst);               /* 0x0048a080 */
extern void  HeapFree_w(void* p);                              /* 0x0049e4d0 */
extern int   GetObjSalvageValue(ObjDef* d, int life);          /* 0x00480db0 */
extern void  AddBricks(int n);                                 /* 0x004578a0 */
extern void  ClearObjectUserFlags(MapObj* obj, Pos* pos);      /* 0x0045e850 */
extern void  RestoreBaseMap(int x, int y);                     /* 0x0045da60 */
extern void  SetMapFlags(int x, int y, unsigned short f);      /* 0x00461810 */
extern void  Set_RFFlags(int x, int y, unsigned char v);       /* 0x004616e0 */
extern void  GetTileCentre(Pos* tile, Pos* out);               /* 0x0045ad60 */
extern void  ResetCursorFootprint(Cursor* c);                  /* 0x0045f460 */
extern void  SetCursorError(Cursor* c, int code);              /* 0x0045f480 */
extern int   CursorIsValid(Cursor* c);                         /* 0x0045f4b0 */
extern void  PropagateCursorStatus(Cursor* c);                 /* 0x0045f4d0 */
extern int   CheckCursorFootprint(Cursor* c);                  /* 0x0045f540 */
extern int   IsBuildableClass(ObjDef* d);                      /* 0x0045ead0 */
extern int   ClassAllowsObjects(ObjDef* d);                    /* 0x0045eab0 */
extern int   ClassNeedsPath(ObjDef* d);                        /* 0x0045eaf0 */
extern int   CheckForPeople(WinRect* r);                       /* 0x00485260 */
extern void  TakeRenderNodeByPos(unsigned short bpos, Pos* out); /* 0x0045a430 */
extern void  TakeRenderNodeInColumn(Pos* p);                   /* 0x0045a3e0 */
extern void  EraseWorkOrdersAt(ObjDef* d, BPos bp);            /* 0x0049b270 */
extern void  PlayInstanceOfSample(int sample, int a, int b, SoundSource* src); /* 0x00496d20 */
extern void  RemoveObjectsPowerStats(MapObj* obj, BPos bp);    /* 0x0045a230 */
extern int   GetRectArea(Rect* rect);                          /* 0x00480960 */
extern int   UnmarkObjectTiles(Pos* pos);                      /* 0x00489f50 */
extern void  RemoveRepairOrderAT(ObjDef* d, int x, int y);     /* 0x0049b580 */
extern void  UpdatePathNeighbours(Pos* p);                     /* 0x0045d260 */
extern void  RemovePathSquare(Pos* p);                         /* 0x00481c90 */
__declspec(dllimport) int __stdcall IntersectRect(WinRect* dst, const WinRect* a,
                                                  const WinRect* b); /* [0x4ab2a0] */
void* memset(void*, int, unsigned int);

void CalculateMapRenderOrder(void);
void AddObjectToMap(MapObj* obj, BPos bp, unsigned int flags);
void RemoveObjectFromMap(BPos bp);

/* ---------------------------------------------------------- inline helpers -- */

/* The bounds-checked cell fetch every map accessor open-codes (objmap.c). */
static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

/* Same lookup reading the coordinates lazily through the Pos* (y only after
 * x passed) — objmap.c's CopyMapCell shape. */
static __inline Cell* MapCellAtPos(Pos* pos)
{
    int x = pos->x;
    int y;

    if (x >= 0 && x < g_map->width) {
        y = pos->y;
        if (y >= 0 && y < g_map->height)
            return &g_map_rows[y][x];
    }
    return 0;
}

/* Copy the cell out by value, or mark the copy "blocked" when off the map. */
static __inline void FetchCellAtPos(Pos* pos, Cell* out)
{
    int x = pos->x;
    int y;

    if (x >= 0 && x < g_map->width) {
        y = pos->y;
        if (y >= 0 && y < g_map->height) {
            *out = g_map_rows[y][x];
            return;
        }
    }
    out->flags = 0x40;
}

/* Copy the cell out by value, or mark the copy "blocked" when off the map. */
static __inline void FetchCellOr40(int x, int y, Cell* out)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        *out = g_map_rows[y][x];
    else
        out->flags = 0x40;
}

/* -------------------------------------------------------------- functions -- */

/* The three per-tile-element callbacks a "special path" tile module exposes:
 * look the cell up (24.8 world coordinates >> 8) and forward to the tile
 * element's handler at +0x18 / +0x1c / +0x20. */
// FUNCTION: LEGOLAND 0x00401820
unsigned char SPGetRFFlags(int x, int y)
{
    int   cx = x >> 8;
    int   cy = y >> 8;
    Cell* cell;
    unsigned char (*fn)(int, int);

    if (cx < 0 || cx >= g_map->width || cy < 0 || cy >= g_map->height)
        cell = 0;
    else
        cell = &g_map_rows[cy][cx];
    fn = g_sp_tile_info[cell->tile].elem->rfflags;

    if (fn)
        return fn(x, y);
    return 2;
}

// FUNCTION: LEGOLAND 0x00401890
void SPEnter(unsigned int x, unsigned int y)
{
    int   cx = x >> 8;
    int   cy = y >> 8;
    Cell* cell;
    void (*fn)(unsigned, unsigned);

    if (cx < 0 || cx >= g_map->width || cy < 0 || cy >= g_map->height)
        cell = 0;
    else
        cell = &g_map_rows[cy][cx];
    fn = g_sp_tile_info[cell->tile].elem->enter;

    if (fn)
        fn(x, y);
}

/* [sic] tests the ENTER callback but calls the LEAVE one — original bug. */
// FUNCTION: LEGOLAND 0x00401900
void SPLeave(unsigned int x, unsigned int y)
{
    int   cx = x >> 8;
    int   cy = y >> 8;
    Cell* cell;
    SPTileElem* e;

    if (cx < 0 || cx >= g_map->width || cy < 0 || cy >= g_map->height)
        cell = 0;
    else
        cell = &g_map_rows[cy][cx];
    e = g_sp_tile_info[cell->tile].elem;

    if (e->enter)
        e->leave(x, y);
}

/* Clear the render/object bits (0xa0) and the RF path bits (3) over every
 * footprint cell of the object whose base cell is bp, then rebuild the render
 * order unless a map load is in progress. */
/* Codegen notes: the loop offsets go through a Pos `t` (struct-member
 * temporaries keep VC6 from strength-reducing the row index and from turning
 * the y loop into a counted one), the row is computed before the column, and
 * only bp.x is routed through a named local so that x (not y) becomes the
 * first CSE temp (eax, the local slot) and y falls into the dead parameter
 * slot. */
// FUNCTION: LEGOLAND 0x0045f100
void RemoveObjectFromMap(BPos bp)
{
    int     x0 = bp.x;
    Cell*   cell = MapCellAt(x0, bp.y);
    ObjDef* def = ((MapObj*)cell->obj)->cls;
    Rect*   p;
    Rect    r;
    int     x, y;
    Pos     t;

    if (def->flags & 0x20000)
        g_bg_full_update = 1;
    DecrementObjectCount(def);

    p = &def->rect;
    do {
        r = *p;
        for (y = r.top; y <= r.bottom; y++) {
            for (x = r.left; x <= r.right; x++) {
                t.y = bp.y + y;
                t.x = bp.x + x;
                g_map_rows[t.y][t.x].flags &= ~0xa0;
                g_map_rows[t.y][t.x].rf &= ~3;
            }
        }
        p = r.next;
    } while (p);

    if (!g_map_loading) {
        CalculateMapRenderOrder();
        g_render_order_dirty = 1;
    }
}

/* Mark every footprint cell of `obj` (class rect list, relative to bp) as a
 * placed-object cell: owner, base cell, flags (keeps only the path bit 0x10
 * of what was there, adds `flags` and 0x80), fresh life, RF from the class. */
// FUNCTION: LEGOLAND 0x0045dd80
void AddObjectToMap(MapObj* obj, BPos bp, unsigned int flags)
{
    ObjDef* def = obj->cls;
    Rect    r = def->rect;
    int     x, y;
    Cell*   cell;

    if (def->flags & 0x20000)
        g_bg_full_update = 1;
    IncrementObjectCount(def);

    for (;;) {
        for (y = r.top; y <= r.bottom; y++) {
            for (x = r.left; x <= r.right; x++) {
                cell = MapCellAt(bp.x + x, bp.y + y);
                if (cell) {
                    cell->obj = obj;
                    *(BPos*)&cell->bx = bp;
                    cell->rf = 0;
                    cell->flags = (unsigned short)((cell->flags & 0x10) | flags | 0x80);
                    cell->life = def->life;
                    if (def->flags & 2)
                        cell->rf = 2;
                    if (def->flags & 1)
                        cell->rf = 1;
                    if (def->flags & 0x800000)
                        cell->flags |= 0x8000;
                }
            }
        }
        if (!r.next)
            break;
        r = *r.next;
    }

    if (!g_map_loading && !g_build_in_progress)
        CalculateMapRenderOrder();
}

/* The standard class "place" handler (ObjClass +0x98).  A class with flag
 * 0x40000 is a single-cell object written straight into its cell (tile from
 * the class, flag 0x08); anything else takes the footprint route through
 * AddObjectToMap and, unless type 2, gets an instance record keyed by cell.
 * The handler takes a THIRD argument like the remove handler at +0x9c: it is
 * never read, but `bp` is homed in its slot ([esp+0x1c]) and `key` in the
 * dead `pos` slot, which is what the original frame shows (no sub esp).
 * pathbuild.c declares it with two parameters; the caller-side codegen there
 * was matched that way, so the third argument is simply not pushed. */
// FUNCTION: LEGOLAND 0x0045efe0
void AddBasicObject(MapObj* obj, Pos* pos, void* ctx)   /* ctx: unused third handler arg (its slot homes bp) */
{
    ObjDef* def = obj->cls;
    Cell*   cell;
    BPos    bp;
    BPos    key;
    void*   inst;
    unsigned short tile;

    if (def->flags & 0x20000)
        g_bg_full_update = 1;

    if (def->flags & 0x40000) {
        cell = MapCellAtPos(pos);
        tile = *def->tile;
        cell->obj = obj;
        cell->tile = tile;
        cell->bx = (unsigned char)pos->x;
        cell->by = (unsigned char)pos->y;
        if (def->flags & 2)
            cell->rf = 2;
        if (def->flags & 1)
            cell->rf = 1;
        cell->flags |= 8;
        if (def->flags & 0x800000)
            cell->flags |= 0x8000;
        IncrementObjectCount(def);
    } else {
        bp.x = (unsigned char)pos->x;
        bp.y = (unsigned char)pos->y;
        AddObjectToMap(obj, bp, 0);
        ApplyConsTileMap(obj, bp);
        if (def->type != 2) {
            key.x = (unsigned char)pos->x;
            key.y = (unsigned char)pos->y;
            inst = CreateObjectInstance(def, &key);
            if (inst)
                AddInstanceToList(inst);
        }
    }
}
/* Position the destroy cursor: over an object cell (flags 0x8a0) it snaps to
 * the selected object's base cell (g_sel_bpos) and takes the "on object"
 * state 9, otherwise it sits on the pointed cell in state 8; the footprint is
 * the selected class's.  Off the map (either lookup) the cursor is blocked. */
// FUNCTION: LEGOLAND 0x00480bb0
void BasicObjectDCalcCursor(void* unused, Pos* pos)
{
    Cell cell;
    int  x, y;

    FetchCellAtPos(pos, &cell);
    g_destroy_cursor.rect = g_sel_def->rect;
    g_destroy_cursor.flags = 8;
    if (cell.flags & 0x8a0) {
        g_destroy_cursor.flags = 9;
        x = g_sel_bpos.b.x;
        g_destroy_cursor.origin.x = x;      /* stored in each arm: not tail-merged */
        y = g_sel_bpos.b.y;
    } else {
        x = pos->x;
        g_destroy_cursor.origin.x = x;
        y = pos->y;
    }
    g_destroy_cursor.origin.y = y;
    FetchCellOr40(x, y, &cell);
    if (cell.flags & 0x40)
        SetCursorError(&g_destroy_cursor, 1);
    else
        ResetCursorFootprint(&g_destroy_cursor);
}

/* Stamp a placed object's footprint into the map through the EDIT-CURSOR
 * chain (every chained cursor that is not in state 0x3000): each cursor's
 * rect list, offset by its origin, becomes cells owned by `obj` with the base
 * cell bp, flags (keep path bit 0x10) | `flags`, RF 2 and the tall bit for a
 * class with flag 0x800000.  The class effect callback fires at the tile
 * centre with effect 0x8f8 first, and the whole cursor block is saved around
 * the operation and restored afterwards. */
// FUNCTION: LEGOLAND 0x0045dee0
void SetObjRectFlags(MapObj* obj, Pos* pos, unsigned int flags)
{
    ObjDef* def = obj->cls;
    Cursor  saved;
    BPos    bp;
    Pos     centre;
    Rect    r;
    Cursor* c;
    Cell*   cell;
    int     x, y;
    int     cx, cy;

    saved = g_edit_cursor;
    bp.x = (unsigned char)pos->x;
    bp.y = (unsigned char)pos->y;
    GetTileCentre(pos, &centre);
    def->effect(def->c4, &centre, 0x8f8);

    for (c = &g_edit_cursor; c; c = c->next) {
        r = c->rect;
        if (c->flags & 0x3000)
            continue;
        for (;;) {
            for (y = r.top; y <= r.bottom; y++) {
                for (x = r.left; x <= r.right; x++) {
                    cx = c->origin.x + x;
                    cy = c->origin.y + y;
                    cell = MapCellAt(cx, cy);
                    if (cell) {
                        cell->obj = obj;
                        *(BPos*)&cell->bx = bp;
                        cell->flags = (unsigned short)((cell->flags & 0x10) | flags);
                        if (def->flags & 0x800000)
                            cell->flags |= 0x8000;
                        cell->rf = 2;
                    }
                }
            }
            if (!r.next)
                break;
            r = *r.next;
        }
    }
    g_edit_cursor = saved;
}

/* Playfield point -> map cell (the inverse of the tile diamond projection).
 * With the default ground tile h high and w = 2h wide, a point is first
 * placed on the coarse tile grid by (x + w/2) / w and y / h, the diamond
 * corners are then resolved from the remainders: the four quadrants of the
 * bounding square belong to the neighbouring cell when the remainder lies
 * outside the diamond edge (each edge is |rx - w/2| = 2*|ry - h/2|). */
// FUNCTION: LEGOLAND 0x0045bcd0
void PointToIsoPlane(Pos* in, Pos* out)
{
    Sprite* s = g_tile_sprites[g_default_tile];
    short   h = s->h;
    short   w = (short)(h + h);
    int     w2 = (w + 1) >> 1;
    int     h2 = (h + 1) >> 1;
    int     sx = in->x + w2;
    int     sy = in->y;
    int     qx, rx, qy, ry;
    int     sum;
    int     k;

    qx = sx / w;
    rx = sx % w;
    qy = sy / h;
    ry = sy % h;
    sum = qy + qx;
    out->x = sum;
    out->y = qy - qx;
    if (rx < 0) {
        out->y = qy - qx + 1;
        rx += w - 2;
        out->x = sum - 1;
    }
    if (ry < 0) {
        out->y--;
        out->x--;
        ry += h - 1;
    }
    k = (rx >= w2) + 1;
    if (ry > h2)
        k += 2;
    switch (k) {
    case 1:
        if (rx < w2 - ry * 2)
            out->x--;
        break;
    case 2:
        if (rx >= w2 + ry * 2)
            out->y--;
        break;
    case 3:
        if (rx < w2 + (ry - h) * 2)
            out->y++;
        break;
    case 4:
        if (rx >= w2 + (h - ry) * 2)
            out->x++;
        break;
    }
}

/* Screen pixel -> map cell: PointToIsoPlane after undoing the scroll (24.8)
 * and the viewport origin.  Returns -1 when no default tile is loaded, else 1.
 * The third parameter is never read. */
// FUNCTION: LEGOLAND 0x0045be90
int ScreenToMapRef(Pos* screen, Pos* out, int mode)
{
    Sprite* s = g_tile_sprites[g_default_tile];
    short   h;
    short   w;
    int     w2, h2;
    int     sx, sy;
    int     qx, rx, qy, ry;
    int     sum;
    int     k;

    if (s == 0)
        return -1;
    h = s->h;
    w = (short)(h + h);
    h2 = (h + 1) >> 1;
    w2 = (w + 1) >> 1;
    sx = (g_scroll_x >> 8) - ((MapHdr*)g_map)->origin_x + screen->x + w2;
    sy = (g_scroll_y >> 8) - ((MapHdr*)g_map)->origin_y + screen->y;
    qx = sx / w;
    rx = sx % w;
    qy = sy / h;
    ry = sy % h;
    sum = qx + qy;
    out->x = sum;
    out->y = qy - qx;
    if (rx < 0) {
        out->y = qy - qx + 1;
        rx += w - 2;
        out->x = sum - 1;
    }
    if (ry < 0) {
        out->y--;
        out->x--;
        ry += h - 1;
    }
    k = (rx >= w2) + 1;
    if (ry > h2)
        k += 2;
    switch (k) {
    case 1:
        if (rx < w2 - ry * 2)
            out->x--;
        break;
    case 2:
        if (rx >= w2 + ry * 2)
            out->y--;
        break;
    case 3:
        if (rx < w2 + (ry - h) * 2)
            out->y++;
        break;
    case 4:
        if (rx >= w2 + (h - ry) * 2)
            out->x++;
        break;
    }
    return 1;
}

/* Rebuild the render chain (see the file header).  `p` is the scan position;
 * it is address-taken by the node helpers, which is why it lives in memory. */
/* 143/143 instructions, 444B, index-for-index — matched.  Two levers, both
 * counter-intuitive, got the last 65 instructions:
 *
 *  - The emit test is written as `if (A) {emit} else if (B) {emit} else
 *    {step}` with the emit body spelled TWICE.  VC6 tail-merges the two
 *    identical arms late and lays the survivor out between the two tests —
 *    test A falls through into it, test B jumps BACKWARD into it — which is
 *    exactly the original's block order.  Written as `if (A || B) {emit} else
 *    {step}` VC6 instead puts the step arm inline and the emit arm last, and
 *    the register allocation of the whole loop changes with it (54.5%).  Do
 *    not "simplify" the duplicate arm away.
 *  - The four render-node stores must be in the order bpos, x, y, live: VC6
 *    then sinks the `live = 1` store into the middle of the `y` computation
 *    (between `add bl,cl` and `inc bl`), as the original has it.  Any other
 *    order of the four costs 3-11 instructions.
 *
 * The setup order p.x / p.y+node_next / memset / link is also load-bearing: it
 * is the only one of the 24 permutations that keeps the ebx/ebp pushes in the
 * prologue instead of sinking them past the width guard (which tail-duplicates
 * the epilogue and ESCAPES). */
// FUNCTION: LEGOLAND 0x0045a4a0
void CalculateMapRenderOrder(void)
{
    Pos             p;
    unsigned short* link;
    Cell*           cell;
    Cell*           base;
    ObjDef*         def;
    RenderNode*     node;
    int             bx, by;

    p.x = 0;
    p.y = 0;
    g_render_node_next = 0;
    memset(g_render_nodes, 0, sizeof(g_render_nodes));
    link = &g_render_head;
    while (p.x < g_map->width) {
        cell = MapCellAt(p.x, p.y);
        if (!(cell->flags & 0xa0)) {
            p.y++;
        } else {
            bx = cell->bx;
            by = cell->by;
            base = MapCellAt(bx, by);
            node = &g_render_nodes[g_render_node_next];
            def = ((MapObj*)base->obj)->cls;
            g_render_node_next++;
            if (g_render_node_next == 0x1000)
                g_render_node_next = 0;
            node->bpos = *(unsigned short*)&cell->bx;
            node->x = (unsigned char)p.x;
            node->y = (unsigned char)(def->rect.bottom + by + 1);
            node->live = 1;
            /* The emit body is deliberately written out twice — see the note
             * above: VC6 merges the two arms and that is what reproduces the
             * original's block layout. */
            if (p.x == def->rect.right + bx) {
                *link = *(unsigned short*)&base->bx;
                link = (unsigned short*)&base->nx;
                TakeRenderNodeByPos(*(unsigned short*)&cell->bx, &p);
            } else if (p.x == g_map->width - 1) {
                *link = *(unsigned short*)&base->bx;
                link = (unsigned short*)&base->nx;
                TakeRenderNodeByPos(*(unsigned short*)&cell->bx, &p);
            } else {
                p.x++;
                TakeRenderNodeInColumn(&p);
            }
        }
        while (p.y >= g_map->height) {
            p.x++;
            TakeRenderNodeInColumn(&p);
        }
    }
    *link = 0;
}

/* Build a cursor chain's outline point list: for every cell of the first
 * rect of every cursor, the left/right/top/bottom edge midpoints of the cell
 * diamond in playfield pixels (relative to the cursor origin) — one entry per
 * open edge (cursor flags 0x100/0x200/0x40/0x80 suppress an edge).  The kind
 * byte is the cursor style (4 when valid) | 1 for left/right, | 2 for
 * top/bottom.  `refresh` (only honoured for the head cursor) re-checks the
 * footprint first. */
/* EXACT (2026-09 PASS 3): the sums are `short sx, sy`, not int.  VC6 keeps
 * both in full registers (every consumer is a 16-bit store or an add that is
 * truncated at the store, so no movsx ever appears), but the narrower type
 * changes the allocation of the temporaries feeding them: the `y` load then
 * goes straight into edx (sy's home) and is consumed in place by `add edx,edi`,
 * which is the whole residual described below.  Every other spelling in the
 * history below is unchanged; `int sx, sy` is the 6-mismatch attractor.
 * LEVER: when an int-valued temp feeds only 16-bit stores and VC6's in-place
 * accumulate/lea tie-break comes out wrong, try declaring the local `short`.
 *
 * --- history (all measured with int sx/sy; kept so nothing is re-derived) ---
 * 96.3% (161/161 instructions, 544B vs 543B; index-for-index everywhere except
 * six instructions at the head of the x-loop body).
 *   original: mov edx,[y] / mov eax,[r.left] / mov ecx,edi / sub ecx,edx /
 *             add edx,edi / imul ecx,[w2] / imul edx,esi
 *   ours:     mov eax,[y] / mov ecx,edi / sub ecx,eax / imul ecx,[w2] /
 *             lea edx,[edi+eax] / mov eax,[r.left] / imul edx,esi
 * Same seven operations, same registers for the two results (ecx = sx,
 * edx = sy).  The original loads `y` straight into edx — the register sy will
 * live in — so the second sum is an in-place `add edx,edi`, which leaves both
 * ALU ops adjacent and lets the scheduler hoist the r.left load into the gap
 * before the two imuls.  VC6 here loads `y` into eax, so the sum needs a
 * three-operand `lea` and the r.left load lands after it.  The choice is the
 * allocator's, not the expression's — there are only TWO attractors and every
 * spelling lands in one of them:
 *   sx first  -> this code (one `y` load, `lea` for the sum);
 *   sy first  -> `mov edx,[y] / add edx,edi` exactly as the original, but VC6
 *                then RE-LOADS `y` for the subtraction (162 instructions, so
 *                everything downstream shifts by one).
 * Tried and refolded to the sx-first form: the two-step (`sx = x - y;
 * sx *= w2;`), accumulate-in-place (`sy = y; sy += x;`), `sy = y` hoisted
 * before the subtraction so the sub reads the accumulator, all operand orders
 * of both sums, splitting the multiplies out, moving the w2/h2 computation
 * before/after/between the sums, `x == lft` against a hoisted r.left (inside
 * and outside the x loop), a boolean `at_left` temp, `static __inline` helpers
 * taking (x, y) by value both as two functions and as one filling both results
 * through pointers, a helper returning an {sx,sy} struct by value (the byte
 * count then matches at 543 but twelve MORE instructions differ), and a `yy`
 * copy of the loop variable.  The same "consume the freshly loaded operand
 * versus copy the register operand" tie-break shows up in ValidateCursor's
 * mx/my sums, where passing the origin by value into an inline helper fixed
 * it; there is no equivalent here because `y` is already a stack local.
 *
 * MECHANISM PROVEN (2026-09, do not re-derive).  The residual is NOT in the
 * sum expressions at all: it is that eax must already be BUSY when `y` is
 * loaded.  Pinning the r.left load with one volatile read placed before the
 * sums —
 *     lft = *(volatile int*)&r.left;
 *     sx = (x - y) * w2;  sy = (y + x) * h2;  ... if (x == lft && ...)
 * — reproduces the original's allocation EXACTLY inside the block (edx = y
 * consumed in place by `add edx,edi`, ecx = sx, eax = r.left, both imuls after
 * both ALU ops).  Only TWO instructions of that block are then wrong: VC6
 * emits the pinned load first, so 51 and 52 come out swapped (`mov
 * eax,[r.left] / mov edx,[y]` instead of `mov edx,[y] / mov eax,[r.left]`).
 * It is NOT an improvement overall — the volatile read cannot be CSE'd out of
 * the loop rotation, so it costs one extra `mov edi,[r.left]` in the y-loop
 * tail and the whole function lands at 19 mismatches / 546B versus the 6 / 544B
 * here (the variant is scratchpad/objmap2/n_WC0.c).  So the wanted lever is
 * anything that
 * makes VC6 load r.left into eax at the top of the x-loop body WITHOUT being a
 * scheduling barrier ahead of the y load.  Everything tried to order the two
 * loads has failed: the volatile read moved after a y-consuming statement
 * (register scramble, 66), a volatile read of `y` as well (`volatile int y`,
 * `*(volatile int*)&refresh`) — that makes the variable address-taken, which
 * costs the ARG-SLOT homes ([esp+0x30] = w2, [esp+0x38] = y) and blows up the
 * frame (first divergence moves to index 24), so the volatile route is closed
 * on the y side.  A plain (non-volatile) `lft` is always sunk to its use.
 * Also measured and refolded to one of the two attractors: all 16 combinations
 * of `(x-y)*w2` / `w2*(x-y)` with `(y+x)*h2` / `(x+y)*h2` / `h2*(y+x)` /
 * `h2*(x+y)` in both statement orders; separate named temps for the two sums
 * (d/s) with the multiplies in either order; both sums written BEFORE the
 * w2/h2 computation; `lft` hoisted in five different positions; declaring
 * w2/h2/sx/sy inside the inner block (four groupings); `x == AtEdge(x, r.left)`
 * through a static __inline helper in three positions; four boolean edge flags
 * (ESCAPES); `r.left == x`, `x - r.left == 0` and `!(c->flags & 0x100) && ...`
 * orderings; computing h2 before w2 (that one is a real regression, index 19);
 * and using the dead `refresh` PARAMETER itself as the y loop variable, which
 * is byte-identical to a local (so the [esp+0x38] home is not evidence either
 * way).
 *
 * 2026-09 PASS 2.  No improvement, but the plateau is now bounded and one part
 * of the diagnosis above is WRONG.
 *  - EXHAUSTIVE statement-order search: the block was rewritten as the six
 *    statements w2 / h2 / sx=x-y / sy=y+x / sx*=w2 / sy*=h2 and every one of
 *    the 80 dependency-valid permutations compiled.  They collapse into
 *    exactly FOUR outcomes: 6 mismatches / 544B (this code), 108 / 546B,
 *    120 / 549B and 120 / 546B.  No statement order reaches the original.
 *  - Measured with a difflib-ALIGNED, register-normalised diff (see the
 *    TOOLING note on GetObjectUID), the entire residual is an edit distance of
 *    TWO: the original's `add R,R` against our `lea R,[R+R]`.  Everything else
 *    in the function, register naming included, is identical.
 *  - The two attractors are decided ONLY by the order of the two multiplies.
 *    `sx *= w2` before `sy *= h2` gives one y load plus the `lea` (this code);
 *    `sy *= h2` first gives `mov edx,[y] / add edx,edi` exactly as the
 *    original but a SECOND `mov eax,[y]` for the subtraction (108).  Routing
 *    the subtraction through a named copy of y (`t = y; sx = x - t;
 *    sy = t + x;`) restores the single load but drops straight back to
 *    attractor 1: VC6 will not give one load AND the in-place add together.
 *  - CORRECTION: "eax must already be BUSY when y is loaded" is not what the
 *    original does.  The x-loop body is entered at 0x45f695 from the preheader
 *    and from the back edge at 0x45f7db, and NEITHER leaves anything live in
 *    eax - eax is dead at the body head in the original too.  What differs is
 *    the order in which VC6 CREATES the two temps: it hands out eax, ecx, edx
 *    in creation order, so in the original the r.left temp is created BEFORE
 *    the y temp, which therefore gets edx (sy's home) and is consumed in
 *    place.  The volatile pin reproduces that creation order, which is why it
 *    works; being "busy" is the symptom, not the mechanism.  The lever wanted
 *    is a spelling in which the r.left VALUE exists in the IR before the y
 *    value without being a scheduling barrier ahead of the y load.
 *  - Also measured and refolded to this same 6/544B: `int yy = y` used in the
 *    sums only, and used in the sums plus the y==r.top / y==r.bottom tests;
 *    `int xx = x` in the sums; the two sums as members of a block-scope
 *    struct; `unsigned` and `long` sx/sy; a static __inline helper writing
 *    both sums through int* out-parameters.  Hoisting the w2/h2 statements out
 *    of the x loop into the y-loop body is a REAL regression (559B, first
 *    divergence at index 1, ESCAPES for four of the six orders): it changes
 *    the frame, so they must stay inside the inner loop and be hoisted by VC6.
 * NOTE: the marker must stay on the line directly above the signature; the
 * verifier only looks 1-3 lines ahead, and this note used to sit between them,
 * which made the function silently uncounted. */
// FUNCTION: LEGOLAND 0x0045f5f0
void BuildCursorPtr(Cursor* c, void* unused, int refresh)
{
    Rect  r;
    short h;
    int   w2, h2;
    short sx, sy;               /* 16-bit: what puts y in edx (sy's home) at the x-loop head */
    int   x, y;
    unsigned short style;       /* 16-bit: the original's lea uses +0xfffc, not -4 */

    for (;;) {
        r = c->rect;
        if (refresh)
            CheckCursorFootprint(c);
        h = g_tile_sprites[g_default_tile]->h;
        c->count = 0;
        style = (CursorIsValid(c) ? 2 : 1) * 4 - 4;   /* ternary: keeps the *4-4 unfolded */
        c->style = (unsigned char)style;
        for (y = r.top; y <= r.bottom; y++) {
            for (x = r.left; x <= r.right; x++) {
                w2 = (short)(h + h) / 2;
                h2 = h / 2;
                sx = (x - y) * w2;
                sy = (y + x) * h2;
                if (x == r.left && !(c->flags & 0x100)) {
                    c->px[c->count] = (short)(sx + w2);
                    c->py[c->count] = (short)sy;
                    c->kind[c->count] = (unsigned char)(style | 1);
                    c->count++;
                }
                if (x == r.right && !(c->flags & 0x200)) {
                    c->px[c->count] = (short)(sx + h + h);
                    c->py[c->count] = (short)(sy + h2);
                    c->kind[c->count] = (unsigned char)(style | 1);
                    c->count++;
                }
                if (y == r.top && !(c->flags & 0x40)) {
                    c->px[c->count] = (short)(sx + w2);
                    c->py[c->count] = (short)sy;
                    c->kind[c->count] = (unsigned char)(style | 2);
                    c->count++;
                }
                if (y == r.bottom && !(c->flags & 0x80)) {
                    c->px[c->count] = (short)sx;
                    c->py[c->count] = (short)(sy + h2);
                    c->kind[c->count] = (unsigned char)(style | 2);
                    c->count++;
                }
            }
        }
        c = c->next;
        if (!c)
            break;
        refresh = 0;
    }
}

/* The standard class "remove" handler (ObjClass +0x9c): refund the salvage
 * value, then either (footprint object, cell flag 0x80) tear the cells down
 * through RemoveObjectFromMap, or (a cursor-shaped `ctx` footprint) restore
 * the ground tile, clear the flags and RF and null the owner per cell; unless
 * the class is type 2 the instance record keyed by the cell is freed.
 * The by-value BPos is unpacked ONCE into a function-scope Pos whose address
 * is what ClearObjectUserFlags is handed — that single 8-byte aggregate (and
 * not two ints plus a scratch Pos) is what makes the frame 0x1c bytes and
 * keeps all four register pushes in the prologue. */
// FUNCTION: LEGOLAND 0x0045f220
void StandardRemoveObject(MapObj* obj, BPos bp, Cursor* ctx)
{
    Pos     pos;
    Cell*   cell;
    ObjDef* def;
    void*   inst;

    pos.x = bp.x;
    pos.y = bp.y;
    cell = MapCellAt(pos.x, pos.y);
    def = obj->cls;

    if (def->flags & 0x20000)
        g_bg_full_update = 1;
    AddBricks(GetObjSalvageValue(def, cell->life));

    if (cell->flags & 0x80) {
        ApplyDestrTileMap(obj, bp);
        ClearObjectUserFlags(obj, &pos);
        RemoveObjectFromMap(bp);
    } else {
        Rect r = ctx->rect;
        int  x, y;

        DecrementObjectCount(def);
        for (;;) {
            for (y = r.top; y <= r.bottom; y++) {
                for (x = r.left; x <= r.right; x++) {
                    RestoreBaseMap(ctx->origin.x + x, ctx->origin.y + y);
                    SetMapFlags(ctx->origin.x + x, ctx->origin.y + y, 0);
                    Set_RFFlags((ctx->origin.x + x) << 8, (ctx->origin.y + y) << 8, 0);
                    g_map_rows[ctx->origin.y + y][ctx->origin.x + x].obj = 0;
                }
            }
            if (!r.next)
                break;
            r = *r.next;
        }
    }

    if (def->type != 2) {
        BPos key;

        key.x = (unsigned char)pos.x;
        key.y = (unsigned char)pos.y;
        cell = MapCellAt(pos.x, pos.y);
        inst = GetInstanceOfClass(((MapObj*)cell->obj)->cls, &key);
        if (inst) {
            RemoveInstanceFromList(inst);
            HeapFree_w(inst);
        }
    }
}

/* The cell lookup GetObjectUID probes with.  The parameter order (y, x) is a
 * codegen lever: VC6 evaluates the arguments right to left, so the CSE'd
 * `wpos->x >> 8` is created before `wpos->y >> 8` and the prologue comes out
 * `mov eax,[ecx] / ... / mov edi,[ecx+4] / sar eax,8 / sar edi,8 / test
 * eax,eax` (x-first, separate test) instead of the (x, y) order's y-first
 * head with the x test fused into its `sar` (`js`).  The body is the same
 * bounds-checked fetch as MapCellAt. */
static __inline Cell* CellYX(int y, int x)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

/* Does the object based at `c` put its footprint offset on (x, y)?  The two
 * sums go through a Pos LOCAL on purpose: an aggregate defeats forward
 * substitution, so VC6 computes BOTH sums before the first compare (the
 * original's `add esi,edx / ... / add edx,ebx / cmp esi,eax / jne / cmp
 * edx,edi / jne`); as scalar temps or inline in the `&&` chain the second
 * sum is short-circuited below the first `jne`. */
static __inline int UidHit(Cell* c, ObjDef* def, int x, int y)
{
    Pos p;

    p.x = def->dx + c->bx;
    p.y = c->by + def->dy;
    return p.x == x && p.y == y;
}

/* Find the base-cell id (packed {x,y}) of the placed object `def` whose
 * footprint offset (dx, dy) lands on the cell at the 24.8 world position, by
 * probing the four neighbours.  0 when none.
 *
 * *** SEMANTICS CORRECTED (was misread) ***  The four probes are NOT four
 * independent tests.  The original's control flow (0x48a3fc, 0x48a414,
 * 0x48a41c, 0x48a42a and 0x48a441 all leave the ABOVE probe for the LEFT
 * probe at 0x48a4df, while only the five content tests fall through to the
 * BELOW probe at 0x48a47b) says the below-probe is nested inside the
 * above-probe's `if (cell)`:
 *
 *     c = MapCellAt(x, y - 1);
 *     if (c) {                 <- one null check for BOTH vertical probes
 *         if (match) return uid;
 *         c = MapCellAt(x, y + 1);
 *         if (match) return uid;      <- c is NOT re-checked: the bug
 *     }
 *
 * So (a) when the cell ABOVE is off the map (y == 0, x off the map, or the
 * row is null) the cell BELOW is never looked at at all, and (b) the
 * unchecked deref of the below-cell — the original's `xor esi,esi` landing pad
 * at 0x48a4a1 falling straight into `test byte [esi+0xc],0x80` — is what that
 * shared null check was meant to cover.  An earlier reconstruction had the
 * four probes flat, which probes below whenever above is out of bounds; that
 * is a behaviour difference, not just a codegen one, so this shape stands even
 * though it scores no better.  The left/right probes each keep their own null
 * check.
 *
 * SPELLING NOTE (this is a codegen lever, not a style choice).  The map
 * coordinates are written as the expressions `wpos->x >> 8` / `wpos->y >> 8`
 * at EVERY use instead of being read once into `int x, y` locals.  VC6 CSEs
 * them back into one value each (one load + one sar, exactly as the original),
 * but the two spellings do NOT compile the same:
 *   - with `int x` locals, VC6 proves the below-probe's x bounds tests are
 *     dominated by the above-probe's and DELETES them (the original keeps all
 *     four tests), and it then CSEs x*20 into a callee-saved register for both
 *     vertical probes (the original recomputes `lea edx,[eax+eax*4]` per probe
 *     and folds the *4 into the final `lea ecx,[ecx+edx*4]`);
 *   - with the expression spelling, both of those go away: the below probe
 *     re-emits `test x,x / cmp x,width / test y+1,y+1 / cmp y+1,height` and the
 *     cell address is built with the original's two-lea form.
 * Only the x spelling matters (the y one is free); mixing them was measured in
 * all four combinations: locals everywhere 179 mismatches, expression in the
 * probes only 174, expression in the probes AND in the x comparison 163.
 *
 * 2026-09 PASS 3: 89.5% (20 mismatches, 477B = the original's length,
 * index-for-index over 0-94 and 109-190).  Two levers closed 143 of the 163:
 *   1. the lookup helper's parameter order (y, x) — see CellYX — which puts
 *      `def` in ebp, x in eax, y in edi, width in the dead arg slot and
 *      height in ebx exactly as the original (the whole "def spilled / g_map
 *      hoisted" residual below was this one evaluation-order tie-break);
 *   2. the coordinate sums through a Pos local — see UidHit — which computes
 *      both sums before the first compare.
 * Neither lever works alone (174 / 188); together they give 20.  What is
 * left is ONLY the left and right probes' g_map access: the original
 * reloads g_map into esi at the probe ENTRY (a landing pad `mov esi,[g_map]`
 * at 0x48a4d9 / 0x48a547 that the edges on which esi still holds g_map jump
 * past, to 0x48a4df / 0x48a54d) and uses edx as the width/height zero-extend
 * scratch; ours loads g_map at its first use (after the x-1 / x+1 sign test)
 * into edx and zero-extends through esi.  Same instructions, two registers
 * swapped and one load moved up by three instructions, twice.
 * Measured for that residual (scratchpad/objmap2/p3/uid_v*.py):
 *   - a `Map* m` variable reassigned before every probe (`m = g_map;
 *     c = CellM(m, ..)`) DOES produce the original's landing pads with the
 *     retargeted edges, and so does a helper-local `Map* m = g_map` in every
 *     probe — but both also make the BELOW probe reload g_map and re-read
 *     width/height instead of using the above probe's CSE'd copies, which
 *     frees ebx and lets VC6 hoist g_map_rows into it at the entry (146);
 *   - every mix that keeps the vertical pair's CSE (below probe reusing the
 *     above's m, direct g_map reads, a block-scoped m, `m2 = m` copies,
 *     separate m2/m3 for left/right, `register`) sinks the left/right load
 *     to its use (this 20).  The pad appears only when the above probe's m
 *     is not reused by the below probe — the two wanted properties have not
 *     been reached together;
 *   - passing g_map as a helper argument, a comma-expression macro (ESCAPES),
 *     x and/or y as int locals (the note below still applies: an int x makes
 *     VC6 delete the below probe's x tests and CSE x*20).
 *
 * --- history (measured before the two levers above) ---
 * 14.7% (191/191 instructions, 480B vs 477B; 28 exact and most of the rest
 * identical up to register naming — the block structure, the branch targets,
 * the `xor esi,esi` landing pad and the four epilogues now line up
 * index-for-index).  What is left is one register-allocation decision:
 *   original: eax=x, edi=y, ebp=def (preloaded in the prologue), ebx=height,
 *             and g_map->width SPILLED into the dead arg-1 slot [esp+0x14],
 *             where the below probe reads it as a memory operand; esi is the
 *             per-region scratch that holds g_map, then g_map_rows, then the
 *             cell, and g_map is REMATERIALISED by the two one-instruction
 *             stubs at 0x48a4d9 / 0x48a547 (it is not hoisted, because the
 *             x<0 path reaches the left probe without ever loading it).
 *   ours:     eax=x, ecx=y, ebp=width, edi=height, ebx=g_map hoisted above the
 *             first branch and then reused as scratch — and `def` is the value
 *             that loses, so it is re-read from its argument home [esp+0x18]
 *             before each use (which also splits the original's adjacent
 *             `mov ecx,[ebp+0xc] / mov ebx,[ebp+0x10]` pair).
 * The lever still wanted is whatever stops the g_map hoist / makes VC6 rank
 * `def` above `width`.  Tried and refolded: the goto transcription of the same
 * CFG, a second cell variable, a second (differently spelled) MapCellAt for the
 * below probe, an explicit (non-helper) bounds test in either or both vertical
 * probes, the negated (`x < 0 || ...`) and unsigned forms, `w`/`h` cached by
 * assignment inside the first bounds test, a MapCellAt taking (y, x) so the
 * coordinates evaluate in the other order, a local copy of `def`, reversed
 * operand order in the class and coordinate compares, and an inlined `Sum`
 * helper for the coordinate sums — all give byte-identical output.  A volatile
 * read of g_map inside the helper DOES stop the hoist and puts `def` back in a
 * register, but it also kills the width/height CSE the below probe depends on
 * (the below probe then reloads g_map), so it is a net loss (171/175); a
 * volatile g_map_rows on top of that frees ebx and VC6 spends it on a
 * `mov bl,0x80` constant register.  Head note: with this spelling VC6 fuses
 * the x<0 test into the shift (`js`) because x's `sar` is the last flag-setter;
 * the original's `sar eax,8 / sar edi,8 / test eax,eax` needs the x shift
 * emitted FIRST, and no argument order or helper signature achieves that.
 *
 * 2026-09 PASS 2.  *** THE LEVER THE PARAGRAPH ABOVE ASKS FOR IS FOUND. ***
 * It trades against two other hoists, so the committed body is unchanged (it
 * still has the best index-for-index count) - but start here, not from
 * scratch.
 *   LEVER: spell the map x as a LOCAL `int x = wpos->x >> 8;` (used BOTH in
 *   the probe arguments and in the coordinate comparisons; y stays an
 *   expression) AND read g_map through a helper-local pointer, in a private
 *   copy of MapCellAt:
 *       static __inline Cell* CellL(int x, int y)
 *       {
 *           Map* m = g_map;
 *           if (x >= 0 && x < m->width && y >= 0 && y < m->height)
 *               return &g_map_rows[y][x];
 *           return 0;
 *       }
 *   With that, VC6 stops CSE'ing width and height into two callee-saved
 *   registers and puts `def` in EBP exactly as the original - every probe then
 *   emits `cmp dword ptr [edx+0xc], ebp` with no [esp+0x18] reload - and the
 *   head comes out `mov eax,[ecx] / mov ebp,[esp+0x10] / sar eax,8 /
 *   sar esi,8 / test eax,eax`, i.e. the x shift FIRST and the separate `test`
 *   that the paragraph above calls unreachable.  The body is then 477 bytes,
 *   EXACTLY the original's length (this code is 480).  Full variant:
 *   scratchpad/objmap2/o5/v_QL1100.c.
 *   COST, and why it is not committed: index-for-index the count goes
 *   163 -> 167, purely because of the one extra instruction in (a) below
 *   shifting the rest; the difflib-ALIGNED raw edit distance improves a lot,
 *   152 -> 132.  Two defects remain in that variant:
 *     (a) `mov ebx,[g_map]` is still hoisted above the first branch - one
 *         extra instruction at index 2, which IS the whole index shift - and
 *         g_map_rows is hoisted into ebp;
 *     (b) because x is a local, x*20 is CSE'd across the two vertical probes
 *         (`lea edi,[eax+eax*4] / shl edi,2 ... add ecx,edi`) instead of the
 *         original's per-probe `lea edx,[eax+eax*4] / lea ecx,[ecx+edx*4]`.
 *   Kill (a) and (b) and the function falls.  Volatile reads of g_map and/or
 *   g_map_rows inside the helper do kill the hoists but cost the width/height
 *   CSE the below probe depends on; ranked by aligned raw edits, volatile-map
 *   130, volatile-rows 133 and the plain local 132 are all within noise of one
 *   another and all beat this code's 152, but none is index-for-index better.
 *   Measured and BYTE-IDENTICAL to this code (do not repeat): a `UidHit`
 *   static __inline carrying the whole content test with `def` as an argument;
 *   a `CellM(Map* m, int x, int y)` helper with `g_map` passed at every call
 *   site; the two coordinate sums written as separate `int sx = ...;
 *   int sy = ...;` statements before the compare (VC6 still short-circuits
 *   after the x compare, where the original computes both sums first, so that
 *   ordering is allocation-driven too, not a source spelling); and all four
 *   addend orders in those sums.  Worse: `&` and `|` non-short-circuit
 *   spellings of the coordinate test (496B / 492B, both ESCAPE).
 *   The full 2x2x2x2 local/expression matrix (x and y, probe argument and
 *   comparison) crossed with both helpers is measured: the winner for the
 *   REGISTER ALLOCATION is (x local everywhere, y expression); the winner for
 *   the index-for-index count is (both expressions), which is this code.
 *   TOOLING for the next agent: scratchpad/objmap2/o5/fast.py compiles a
 *   variant in-process in ~0.15s and returns the mismatch count, the byte
 *   length AND a difflib-aligned edit distance over the raw and the
 *   register-normalised instruction streams (score2.py wraps it).  For these
 *   allocation puzzles the ALIGNED distance is the number that moves; the
 *   index-for-index count mostly measures how far one inserted instruction
 *   shifted the rest of the body, and it hid this lever from two earlier
 *   passes.
 *
 * 2026-09 PASS 4.  No improvement (still 20 / 477B / 191i); the residual is
 * now bounded by a PROOF that the two wanted properties are mutually
 * exclusive over the whole map-pointer family.  Tooling: scratchpad/objmap2/p4
 * (ut.py = harness, u1..u7.py = the sweeps).
 *   MECHANISM (recovered, was only guessed before).  The original's three
 *   `mov esi,[g_map]` (0x48a402, and the pads 0x48a4d9 / 0x48a547) are ONE
 *   value whose live range is split by the two clobbers (`mov esi,
 *   [g_map_rows]` and `mov esi,[ebp+0xc]`), with the reload placed at each
 *   region ENTRY, i.e. BEFORE the `lea ecx,[eax-1] / test / jl` sign test -
 *   because the LEFT probe's copy is still live along the sign-test FAILURE
 *   edge and is what the RIGHT probe reads (0x48a556/0x48a564 read esi on
 *   every edge that leaves the left probe before 0x48a522).  Ours emits
 *   exactly three loads too, but each at the FIRST USE inside its own
 *   `x >= 0` branch, so nothing is live across the failure edge and the right
 *   probe reloads for itself.  Instruction count, byte count and the free
 *   registers are identical; only that liveness differs, and the esi/edx
 *   role swap follows from it (the longer range takes the callee-saved
 *   register).
 *   THE RULE that blocks every fix (104 variants: all 52 ways of grouping the
 *   four probes over `Map*` locals - helper-local, caller-local or helper
 *   parameter - measured with the assignments both at first use and all at
 *   the function top).  A `Map* m = g_map;` local is copy-propagated away and
 *   compiles BYTE-IDENTICALLY to a direct global read whenever the ABOVE and
 *   BELOW probes use the SAME pointer expression - which is exactly the
 *   condition for the vertical width/height CSE ([esp+0x14] + ebx) that this
 *   body needs.  Give the above and below probes two DIFFERENT locals and the
 *   fold stops: the horizontal probes then get the original's landing-pad
 *   reload before the sign test, but the vertical CSE is gone and VC6 spends
 *   the freed ebx on a hoisted g_map_rows (468B, 146).  One local shared by
 *   ALL FOUR probes hoists the load to the entry block, the common dominator
 *   of its uses (477B, 28, first divergence 6, otherwise identical to this
 *   code).  So "pads" and "vertical CSE" cannot both be had from any
 *   pointer-variable spelling.
 *   Additionally measured and BYTE-IDENTICAL to this code (do not repeat):
 *   a horizontal-only helper with `int w = g_map->width;` read before the
 *   guard; one with `Map* m; int w, h;` all named; the negated
 *   `if (x < 0 || x >= m->width || ...) return 0;` guard; four separate early
 *   `return 0;` guards; a degenerate `if (g_map_rows) m = g_map; else m =
 *   g_map;`; `CellM(Map* m, int y, int x)` with g_map passed at each call
 *   site; and the x/y local-vs-expression matrix re-measured in the PASS-3
 *   shape (x as an int local is still a loss - 481B).
 *   Left to try: something that makes rematerialising the global at the right
 *   probe UNATTRACTIVE (the original preferred a live range over a 6-byte
 *   reload), or a shape in which the vertical width/height CSE survives
 *   without the two vertical probes sharing one pointer expression.
 *
 * 2026-09 PASS 5.  Still 20 / 477B / 191i, but the residual is now a SINGLE
 * register-preference decision, not a placement problem.  Tooling:
 * scratchpad/objmap2/p5 (ut.py = the p4 harness, v1..v5.py = the sweeps).
 *   THE PLACEMENT IS REACHABLE.  Pinning the horizontal probes' map read with
 *   a volatile at each probe entry -
 *       #define VMAP (*(Map* volatile*)&g_map)
 *       m = VMAP;  c = CellM(m, y, x - 1);  ...
 *       m = VMAP;  c = CellM(m, y, x + 1);  ...
 *   - reproduces the original's landing pads EXACTLY: 191/191 instructions,
 *   477/477 bytes and a REGISTER-BLIND edit distance of ZERO, i.e. the whole
 *   instruction stream is the original's modulo register names.  It does not
 *   lower the strict count (still 20) because it flips g_map into edx and the
 *   width/height zero-extend temp into esi in ALL THREE probe regions - the
 *   original has g_map in esi and the temp in edx.  One volatile pin (left or
 *   right alone) is 17 and leaves the other probe unpinned; one pin SHARED by
 *   both probes is 471B (VC6 keeps the volatile value live, so the second
 *   reload disappears).  Reusing one variable, two variables, either
 *   declaration order and pinning the vertical probes too are all 20/edit-0
 *   or worse.  So: the missing lever is now purely "which of {the map
 *   pointer, the zero-extend temp} gets esi", and a fix for it should be
 *   tried WITHOUT the volatile first - in the plain body the ABOVE probe
 *   already gets esi=g_map right, and only the two horizontal probes are
 *   wrong.
 *   CORPUS EVIDENCE for the mechanism (scratchpad/sweep4/scan_pad.py finds
 *   every one-instruction landing pad in the matched corpus - 49 of them):
 *     - roads.c Road_SetTile 0x41310d is this function's shape from PLAIN C:
 *       four consecutive `cell = MapCellAt(pos.x, pos.y); if (cell && (cell->rf
 *       & 1)) AdjustTileRFFlags(&pos);` statements keep ONE g_map value in edx
 *       across all four probes, loaded BEFORE the first sign test and
 *       rematerialised only after each call.  The load's block dominates every
 *       probe there, which is exactly what this function's `if (x >= 0)` guard
 *       denies;
 *     - simcore.c GetPathNeighbours 0x45c4a8 / 0x45c593 and bigsim.c
 *       Get_Path_Directions have the same pads from a probe MACRO with a
 *       direct `g_map->width` read, and there the value is shared by probe
 *       PAIRS (probe 1 loads inside its branch, probe 2 reads the pad);
 *     - workorder2.c FindBrokenCellNear 0x49b410 gets it from a loop-invariant
 *       hoist.
 *   In all three the map pointer is ONE value whose live range the allocator
 *   splits, and the reload lands on the EDGE, never at the use.  Ours emits
 *   three loads at first use in each region instead. */
// WIP-FUNCTION: LEGOLAND 0x0048a3e0  (89.5%, left/right probes reload g_map at its use into edx, the original at the probe entry into esi)
unsigned short GetObjectUID(Pos* wpos, ObjDef* def)
{
    Cell* c;

    c = CellYX((wpos->y >> 8) - 1, wpos->x >> 8);
    if (c) {
        if ((c->flags & 0x80) && c->obj && ((MapObj*)c->obj)->cls == def &&
            UidHit(c, def, wpos->x >> 8, wpos->y >> 8))
            return *(unsigned short*)&c->bx;
        /* [sic] no null check on this fetch — the original reuses the one
         * above, so an off-map cell here dereferences 0. */
        c = CellYX((wpos->y >> 8) + 1, wpos->x >> 8);
        if ((c->flags & 0x80) && c->obj && ((MapObj*)c->obj)->cls == def &&
            UidHit(c, def, wpos->x >> 8, wpos->y >> 8))
            return *(unsigned short*)&c->bx;
    }
    c = CellYX(wpos->y >> 8, (wpos->x >> 8) - 1);
    if (c && (c->flags & 0x80) && c->obj && ((MapObj*)c->obj)->cls == def &&
        UidHit(c, def, wpos->x >> 8, wpos->y >> 8))
        return *(unsigned short*)&c->bx;
    c = CellYX(wpos->y >> 8, (wpos->x >> 8) + 1);
    if (c && (c->flags & 0x80) && c->obj && ((MapObj*)c->obj)->cls == def &&
        UidHit(c, def, wpos->x >> 8, wpos->y >> 8))
        return *(unsigned short*)&c->bx;
    return 0;
}

/* The people check for one footprint rect: build the footprint in map
 * coordinates, clip it to the map bound and ask CheckForPeople.  `foot`,
 * `hit` and `people` are HELPER locals on purpose: an address-taken local
 * born inside an inlined helper is not in the caller's escaped class, so the
 * four stores into `foot` no longer order against the reads of the origin
 * through `o` and both origin halves stay in one register each (the earlier
 * MakeFoot(WinRect*, Rect*, Pos by value) shape needed the by-value copy for
 * the same reason and then mis-scheduled the block).  The origin is read
 * through a `Pos*` (not `cur->origin`, which is also read in the x loop: the
 * textual repetition would CSE it and flip the mx/my sums to copy-then-add).
 * The sum order top, bottom, left, right is the original's: origin.y is
 * consumed in place by the bottom sum and origin.x is loaded late. */
static __inline void PeopleCheck(Cursor* cur, Rect* r, WinRect* bound, Pos* o)
{
    WinRect foot;
    WinRect hit;
    int     people;

    foot.top = r->top + o->y;
    foot.bottom = r->bottom + o->y;
    foot.left = r->left + o->x;
    foot.right = r->right + o->x;
    if (IntersectRect(&hit, &foot, bound)) {
        people = CheckForPeople(&hit);
        if (people != -1) {
            if (people == 1)
                SetCursorError(cur, 3);
        } else {
            SetCursorError(cur, 4);
        }
    }
}

/* Validate an edit-cursor chain for class `def` (see the file header for the
 * error codes).  [sic] the map bound rect passed to IntersectRect reads the
 * map HEIGHT for both its right and bottom edges — original bug.
 *
 * Two things the disassembly settled:
 *  - the parameter `cur` IS the chain walker (VC6 coalesces the loop variable
 *    with it and keeps the separate `root` copy in a stack slot); that is what
 *    puts the cursor in ebp, the rect in edi, and folds the four pushes into
 *    one prologue;
 *  - error 10 fires for an object of a class WITHOUT flag 0x200000
 *    (`test dword [under+0x1c],0x200000 / jne skip`), not with it.  An earlier
 *    reconstruction had the test inverted; the file header is corrected.
 *
 * 2026-09 PASS 3: 97.6% (5 mismatches, 617B exact, index-for-index over
 * 0-41 and 47-204).  The whole foot block from index 47 on now matches: the
 * lever was moving `foot`/`hit`/`people` INTO the inlined helper (see
 * PeopleCheck) with the origin read through a `Pos*` and the sums ordered
 * top, bottom, left, right.  The remaining five instructions are 42-46:
 *   original: mov eax,[origin.y] / mov ecx,[r->top] / add ecx,eax /
 *             mov [bound.right],edx / mov edx,[r->left]
 *   ours:     mov [bound.right],edx / mov eax,[origin.y] / mov ecx,[r->top] /
 *             mov edx,[r->left] / add ecx,eax
 * i.e. the original stores bound.right (the second g_map->height read, loaded
 * into edx at index 41 in both) only AFTER the top sum, and its r->left load
 * then cannot pass that store; ours stores it first.  The origin.y load is
 * never hoisted above a store to the address-taken `bound` (VC6 orders every
 * pointer load after an escaped-local store), so in the original's IR the
 * bound.right STORE sits after the top sum while its LOAD sits before the
 * origin.y load.  Measured and ruled out for that shape (~600 variants,
 * scratchpad/objmap2/p3/vc_v*.py): every position of `bound.right = ...`
 * inside the helper (a single-use load, int or unsigned short, local or
 * argument, is always sunk to its store, which drags g_map's eax across the
 * top sum); all 24 bound store orders x {caller, helper-by-pointer,
 * helper-local} x {assignments, initialiser}; `bound` as a helper local (the
 * stores then stop ordering against the loads, but VC6 also copies origin.y
 * for the top sum and spends ebx); aggregate initialisers for bound and/or
 * foot (VC6 evaluates them in field order left, top, right, bottom and sinks
 * the last store, but the original's order is top, left, bottom, right);
 * volatile pins of the second height read (frame changes); origin by value,
 * as two ints, as int* and as `cur->origin` reads (the last flips the x-loop
 * mx/my sums, 121); the field types of the rect and origin (no type-based
 * disambiguation in VC6); all four bound stores AFTER the sums inside the
 * helper (23 by pointer, 170 as a helper local); chained `bound.left =
 * bound.top = 0` (identical) and `bound.right = bound.bottom = h` (one load,
 * 168); the two height reads through block-scoped unsigned short locals
 * (identical).  Hypothesis left: the second height read is not a single-use
 * temporary in the original.
 *
 * --- history (measured with the old MakeFoot shape) ---
 * 93.2% (205/205 instructions, 617B exact, index-for-index over 0-41 and
 * 56-204 — the ONLY residual is the 14-instruction foot/bound block at 42-55).
 *
 * The store into the address-taken `foot` rect is treated as a possible alias
 * of `cur->origin`, so spelling the four sums inline reloads both origin
 * fields (that shape was 74.1%).  Passing the origin BY VALUE into a static
 * __inline helper is what fixes it: `p` is an argument temp, so both fields
 * are loaded once and stay live across the stores, and the whole tail of the
 * function (the IntersectRect/CheckForPeople argument leas, y in edx, the
 * g_map_rows cell address) then falls into place index-for-index.  The field
 * order inside the helper matters and was searched exhaustively (all 24): any
 * order ENDING in `right` gives 617 bytes and this 14-instruction residual;
 * every other order costs 13 more.  Naming any of the four sums with a local
 * temp inside the helper (or hoisting the r-> loads) blows the frame up and
 * costs ~160.
 *
 * What is left is pure scheduling inside that block; ours emits the same 14
 * instructions with the same opcodes:
 *   original: loads r->top, r->left, r->bottom, r->right in that order but
 *             STORES top, bottom, left, right, loads origin.x LATE (one slot
 *             before the two adds that consume it) and delays the
 *             `bound.right` store to sit between the top add and the r->left
 *             load;  origin.y is consumed in place by the bottom sum.
 *   ours:     stores in the helper's textual order (top, left, bottom, right),
 *             hoists the origin.x load into the slot the original gives the
 *             bound.right store, and keeps origin.y live instead of consuming
 *             it.  A source order of top,bottom,left,right restores the
 *             original's store order but then loses the early r->left load and
 *             the late origin.x load (27), because nothing makes the left/right
 *             adds wait.
 *
 * Re-measured 2026-09; the following are dead ends, do not repeat them:
 *  - all 24 field orders inside MakeFoot: any order ENDING in `right` gives
 *    617B and 14 (tlbR / ltbR / lbtR / bltR), tRbl / bRtl / Rtbl / Rbtl give
 *    15, every other order 616B and 27;
 *  - all 24 orders of the four `bound` stores: the present tlbR is the only
 *    14, the rest cost 15-20 and move the first divergence up to index 36-40;
 *  - moving the MakeFoot call to every one of the five positions among the
 *    bound stores, for four different bound orders: 23-34, first divergence 34;
 *  - MakeFoot(o, r, int px, int py) / (o, r, int py, int px): 139 / 23;
 *  - MakeFoot(o, r, Pos* p, int py) with the x sums reading p->x: VC6 reloads
 *    p->x for the second sum (622B, 133);
 *  - splitting into MakeFootY(o,r,int py) + MakeFootX(o,r,int px): the X call
 *    DOES materialise origin.x late, exactly as the original, but the Y sums
 *    then need an extra `mov ecx,eax` copy (the top sum's destination stops
 *    being r->top's register) and the mx/my block downstream picks up the same
 *    copy shape: 158/160.  Passing Pos by value to both halves changes the
 *    frame (631B, first=0);
 *  - naming any r-> load in a temp inside the helper (`int l = r->left;`,
 *    with or without a block, one/two/three temps): frame changes, 607-612B;
 *  - the four sums spelled inline with `int py = cur->origin.y;` and
 *    cur->origin.x read directly: both origin fields reload, 620B, ~160.
 * So the whole 14-instruction residual is the scheduler's choice of what to
 * put in ONE slot: the original delays the `bound.right` store into it and
 * loads origin.x only after all four r-> loads; ours puts the bound.right
 * store before the block and spends the slot on an early origin.x load.
 *
 * 2026-09 PASS 2.  No improvement; three exhaustive searches are now closed
 * and the residual is characterised exactly.
 *  - The 14 instructions at 42-55 are a PERMUTATION of the same multiset in
 *    both builds: the difflib-ALIGNED, register-normalised edit distance is
 *    FOUR (two moves).  Nothing is missing or extra - only the order and the
 *    register roles differ.
 *  - WHAT actually differs (this is the thing to attack): the original loads
 *    origin.y into eax, uses it for the top sum (`add ecx,eax`, accumulating
 *    into r->top's register) and then LETS IT DIE IN PLACE at the bottom sum
 *    (`add eax,ecx` - its last use), which frees eax for origin.x, loaded late
 *    and used by the two x sums back to back (`add edx,eax / add ecx,eax`).
 *    Ours pins origin.y in ecx and origin.x in eax across the whole block and
 *    rotates edx as the only scratch, so both origin halves are loaded up
 *    front and neither dies.  The lever wanted is whatever makes VC6 give the
 *    two origin halves the SAME register sequentially.  (This is the same
 *    "consume the operand in place on its last use" tie-break as
 *    BuildCursorPtr's `add edx,edi` - the two are almost certainly one bug.)
 *  - Sweep 1, 2880 variants: all 24 MakeFoot field orders x all 24 `bound`
 *    store orders x all 5 positions of the MakeFoot call among the bound
 *    stores.  The present tlbR / top,left,bottom,right / call-last is the
 *    UNIQUE optimum (aligned edit 4, 14 mismatches, 617B); the next best is
 *    aligned edit 6.
 *  - Sweep 2, 384 variants: all 24 field orders x all 16 combinations of
 *    operand order inside the four sums (`r->top + p.y` vs `p.y + r->top`,
 *    and so on).  VC6 canonicalises addend order COMPLETELY - all 16 flips are
 *    byte-identical for every field order.  So the original's `add eax,ecx`
 *    for the bottom sum is the allocator consuming origin.y in place, NOT a
 *    reversed source spelling; do not go looking for one.
 *  - Sweep 3, 128 variants: the MakeFootY + MakeFootX split with both field
 *    orders, all operand flips and both call orders - aligned edit 7, strictly
 *    worse than the single helper.
 *  - Also refolded to this same 14: `const Rect*` on the helper, the helper's
 *    parameters in a different order (Rect*, Pos, WinRect*), and
 *    `MakeFoot(&foot, r, *(Pos*)&cur->origin)`.  Worse: naming the four sums
 *    in helper-local ints (608B, 178), naming the r->left / r->right loads in
 *    helper-local ints (608B, 175), and copying the origin either through a
 *    `Pos p = cur->origin;` temp at the call site or through a `const Pos*`
 *    parameter dereferenced into a helper-local Pos - both 618B / 163,
 *    because both add a `mov edx,ecx`.
 *  - PARTIAL WIN worth continuing from: MUTATING the by-value parameter is a
 *    real lever (unlike operand order, which canonicalises).  Writing the
 *    helper as
 *        o->top = r->top + p.y;  o->left = r->left + p.x;
 *        p.y += r->bottom;       o->bottom = p.y;
 *        p.x += r->right;        o->right = p.x;
 *    (semantically identical - p is a by-value copy with no later use) makes
 *    VC6 emit the original's in-place accumulate `add eax,ebx` with origin.y
 *    dying in eax, and puts origin.y in eax at 0x45f89b exactly as the
 *    original.  It costs 616B / 27 / aligned edit 7 because VC6 then grabs
 *    EBX as a fourth scratch and still loads origin.x early into ecx; the
 *    variants that batch the two mutations (aligned edit 5) do the same.  So
 *    the last question is only why the original spends ONE register on both
 *    origin halves.  It is NOT register pressure: ebx is genuinely dead across
 *    this block in the original too - its first definition is the x-loop
 *    variable at 0x45f91f, well after the block.
 *
 * 2026-09 PASS 4.  No improvement (still 5 / 617B / 205i, first divergence
 * 42, aligned register-normalised edit distance TWO - exactly ONE displaced
 * instruction).  ~1760 further variants, tooling in scratchpad/objmap2/p4
 * (vt.py = harness, vc1..vc13.py = the sweeps).  The residual is now pinned
 * to a single measured rule, and that rule blocks every spelling.
 *   THE RULE (proved in isolation, scratchpad/objmap2/p4/vc5.py).  A store to
 *   an ESCAPED CALLER LOCAL placed between two reads of a pointer field kills
 *   the CSE and forces a RELOAD of the field.  Probe: move the IMMEDIATE
 *   store `bound->left = 0;` (no loads, so register pressure cannot explain
 *   it) into the helper between the top and bottom sums - `cur->origin.y` is
 *   then loaded TWICE.  `bound` is escaped (its address reaches the real
 *   IntersectRect call); `foot`/`hit` are born inside the inlined helper and
 *   are NOT in that class, so their stores are not barriers.
 *   WHY THAT SETTLES THE SHAPE.  The original stores bound.right at index 45,
 *   i.e. BETWEEN its two `origin.y` uses (the top sum at 44 and the bottom
 *   sum at 49), and still loads origin.y only once.  So in the ORIGINAL'S
 *   SOURCE that store cannot sit between the two sums either - it must be
 *   before them, exactly as here - and the displacement to index 45 is done
 *   later, by the scheduler.  Every C spelling that puts the store after the
 *   top sum pays for it: the escaped-store barrier reloads origin.y (166),
 *   unless origin.y is made immune, and both ways of doing that cost more
 *   than they save (see below).
 *   Measured this pass, all worse, do not repeat:
 *    - origin.y through a named scalar `int py = o->y;` in the helper (immune
 *      to the barrier: the reload does go away) x px named/inline x 5 store
 *      positions x all 24 orders of {bound.top, bound.left, bound.bottom,
 *      h = g_map->height} in the caller, 480 variants: best 19, 618B - the
 *      height LOAD always sinks with its store, dragging g_map's register
 *      across the top sum.  Putting a bound store after `h = g_map->height`
 *      to pin that load does not pin it;
 *    - the origin passed BY VALUE (argument temps, also immune): best 18,
 *      618B, and it loads BOTH origin halves up front, where the original
 *      loads origin.x late at index 52;
 *    - `bound` born inside the inlined helper (out of the caller's escaped
 *      class, so its stores stop being barriers): worse still;
 *    - 576 variants: all 24 foot-sum orders x all 24 caller bound-store
 *      orders in the PASS-3 shape.  sums top,bottom,left,right x bound
 *      top,left,bottom,right is the UNIQUE 5; next best 6 (bound tlRb, which
 *      diverges at 40), then 7;
 *    - all four bound stores moved INSIDE the helper before the sums, 24
 *      orders: byte-identical 5.  So there is NO scheduling-region boundary
 *      at the inline site - the scheduler simply keeps this store next to its
 *      load whatever region it is in;
 *    - the height read spelled (int) / (unsigned) / (long) / (unsigned short)
 *      / +0 / |0 / *(&..): all byte-identical.  Unlike the float case there
 *      is no surviving no-code integer conversion tuple;
 *    - scheduler-window probe (p4/gap.py): padding the block with 1-3 extra
 *      instructions ahead of it shifts the whole block but the load->store
 *      gap stays 1, so window alignment counted from the function start is
 *      not the actor;
 *    - the four sums computed into scalar locals first, then stored: 135;
 *    - `WinRect bound = { 0, 0, g_map->height, g_map->height };` in the rect
 *      block (165, 618B) is the ONLY shape that reproduces the wanted
 *      behaviour in kind: the two height reads are NOT CSE'd (two loads, as
 *      in the original) and the initialiser's LAST store IS sunk past the
 *      `cur->origin.y` load WITHOUT breaking its CSE - confirming that
 *      initialiser stores are non-aliasing.  It still cannot be the original:
 *      the sunk store is always the HIGHEST-OFFSET field (bound.bottom at
 *      +0xc) where the original sinks bound.right at +8, it only sinks ONE
 *      slot where the original sinks three, and the two zero stores come out
 *      in field order left,top where the original has top,left.  Partial
 *      initialisers plus a trailing assignment (165/166) do not help.
 *   DIAGNOSTIC worth keeping: VC6 separates the FIRST height load from its
 *   store by two instructions (it schedules `xor edx,edx` in between) and the
 *   second by one; the original separates the second by four, filling the gap
 *   with the first three instructions of the sums.  Same multiset, same
 *   registers, only the order differs - so the pre-scheduling list differs by
 *   one item's position, and no source order reaches it without breaking the
 *   origin.y CSE. */
// WIP-FUNCTION: LEGOLAND 0x0045f810  (97.6%, bound.right store scheduled before the top sum: 5 insns at idx 42-46)
void ValidateCursor(Cursor* cur, ObjDef* def)
{
    Cursor* root = cur;
    Rect*   r;
    WinRect bound;
    Cell*   cell;
    ObjDef* under;
    int     x, y;

    if (IsBuildableClass(def) && !(cur->flags & 0x4000))
        CheckCursorFootprint(cur);
    ResetCursorFootprint(cur);

    for (; cur; cur = cur->next) {
        if (cur->flags & 0x2000)
            continue;
        for (r = &cur->rect; r; r = r->next) {
            bound.top = 0;
            bound.left = 0;
            bound.bottom = g_map->height;
            bound.right = g_map->height;
            PeopleCheck(cur, r, &bound, &cur->origin);
            for (y = r->top; y <= r->bottom; y++) {
                for (x = r->left; x <= r->right; x++) {
                    int mx = cur->origin.x + x;
                    int my = cur->origin.y + y;

                    cell = MapCellAt(mx, my);
                    if (!cell) {
                        SetCursorError(cur, 7);
                        continue;
                    }
                    if (cell->flags & 0x40)
                        SetCursorError(cur, 1);
                    if (cell->flags & 0xa8) {
                        under = ((MapObj*)cell->obj)->cls;
                        if (under == g_env_class)
                            under = 0;
                    } else {
                        under = 0;
                    }
                    if (under && !(under->flags & 0x200000)) {
                        SetCursorError(cur, 10);
                        goto pathcheck;
                    }
                    if (!ClassAllowsObjects(def)) {
                        if (under)
                            SetCursorError(cur, 6);
                        if (cell->flags & 0x800)
                            SetCursorError(cur, 5);
                    }
pathcheck:
                    if (ClassNeedsPath(def)) {
                        if ((cell->flags & 0x10) && !(cell->rf & 1))
                            SetCursorError(cur, 9);
                    } else {
                        if ((cell->flags & 0x10) || (cell->rf & 1))
                            SetCursorError(cur, 8);
                    }
                }
            }
        }
    }
    PropagateCursorStatus(root);
}

/* Remove a placed object from the map: erase its work orders, play the
 * removal sample (or just flag the render order dirty during a load), drop
 * its power stats, run the class remove handler, roll the build statistics
 * back (mirror of PutObjOnMap), erase a pending repair order on its cell and,
 * when a destroy cursor with flag 0x1000 is active, tear down the path tiles
 * under that cursor's first rect.
 * [sic] the Pos handed to UnmarkObjectTiles is never initialised.
 * The repair-order cell coordinates live in their OWN block-scope pair of
 * ints: that is what makes VC6 keep both halves of the packed BPos argument
 * (the raw dword and the raw dword at +1) alive in callee-saved registers for
 * the whole body, which is worth the 4th push. */
// FUNCTION: LEGOLAND 0x00459c90
void RemObjFromMap(ObjDef* def, MapObj* obj, BPos bp, void* ctx)
{
    Cell*   cell;
    Cursor* c;
    int     area;

    EraseWorkOrdersAt(def, bp);
    if (obj == g_placing_obj)
        g_placed_flag = 0;

    if (!g_map_loading) {
        SoundSource src;

        src.x = bp.x;
        src.kind = 2;
        src.y = bp.y;
        PlayInstanceOfSample(g_remove_sample, 0, 1, &src);
    } else {
        g_render_order_dirty = 1;
    }

    RemoveObjectsPowerStats(obj, bp);
    def->remove(obj, bp, ctx);

    if ((void*)def == g_env_class) {
        g_count_env--;
        g_area_total--;
        g_have_special = 1;
    } else {
        Pos p;      /* [sic] never initialised — original bug */

        area = GetRectArea(&def->rect);
        switch (def->type - 1) {
        case 0:
            UnmarkObjectTiles(&p);
            g_area_type1 -= area;
            break;
        case 1:
            g_have_special = 1;
            g_area_type2 -= area;
            break;
        case 2:
            g_area_type3 -= area;
            break;
        case 3:
            UnmarkObjectTiles(&p);
            g_area_type4 -= area;
            break;
        case 4:
            UnmarkObjectTiles(&p);
            g_area_type5 -= area;
            break;
        }
        g_area_total -= area;
    }

    {
        int x0 = bp.x;
        int y0 = bp.y;

        cell = MapCellAt(x0, y0);
        if (cell->flags & 0x4000) {
            RemoveRepairOrderAT(def, x0, y0);
            cell->flags &= ~0x4000;
        }
    }

    c = &g_destroy_cursor;
    while (c) {
        if (c->flags & 0x1000)
            break;
        c = c->next;
    }
    if (c) {
        Pos p;

        for (p.y = c->origin.y + c->rect.top; p.y <= c->origin.y + c->rect.bottom; p.y++) {
            for (p.x = c->origin.x + c->rect.left; p.x <= c->origin.x + c->rect.right; p.x++) {
                g_map_rows[p.y][p.x].flags &= ~0x18;
                g_map_rows[p.y][p.x].rf = 0;
                g_map_rows[p.y][p.x].tile = g_map_rows[p.y][p.x].base;
                UpdatePathNeighbours(&p);
                RemovePathSquare(&p);
            }
        }
    }
    g_map_dirty |= 4;
}
