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
/* 54.5% (143/143 instructions, 444B exact, no ESCAPES; index-for-index over
 * 0-41 apart from three prologue slots, and over 110-142 apart from renames).
 * What the current shape already buys (do not undo it): the four setup
 * statements in the order p.x / p.y+node_next / memset / link is the ONLY one
 * of the 24 that gives 143 instructions — the others sink the ebx/ebp pushes
 * past the width guard and tail-duplicate the epilogue (ESCAPES).  Writing the
 * base-cell test as `if (!(cell->flags & 0xa0)) p.y++; else {...}` is what puts
 * the two-instruction p.y++ arm inline, as the original has it.
 *
 * Residual is one register tie-break and the block layout it drags with it:
 *   original: ebx = the hoisted g_map (reloaded after each call), esi = `link`
 *             (spilled to [esp+0x10] and reused for the render node and for
 *             &base->nx), eax = `base`;
 *   ours:     esi = g_map, ebx = link, eax = the node and esi = base.
 * Because `base` ends up in a callee-saved register, VC6 also lays the
 * `p.x == right+bx || p.x == width-1` join out with the ELSE (step) arm inline
 * and the THEN (emit) arm last, where the original falls through into the emit
 * arm and puts the second disjunct test after it.  Tried: both arm orders, the
 * fully explicit goto transcription of the original CFG, block-scoping the
 * object-arm locals, `for(;;)`+break, unsigned char bx/by and hoisting
 * width-1.  VC6 re-flattens every one of them to the same layout, so the lever
 * has to be whatever makes g_map outrank `link` for ebx. */
// WIP-FUNCTION: LEGOLAND 0x0045a4a0  (54.5%, g_map/link register tie-break and the || arm layout)
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
            node->live = 1;
            node->y = (unsigned char)(def->rect.bottom + by + 1);
            if (p.x == def->rect.right + bx || p.x == g_map->width - 1) {
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
/* 96.3% (161/161 instructions, 544B vs 543B; index-for-index everywhere except
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
 * allocator's, not the expression's: the two-step (`sx = x - y; sx *= w2;`),
 * the accumulate-in-place, the swapped-operand and the fully-inlined forms all
 * refold to the identical instruction list, and none of the declaration
 * orders, block scopes, unsigned types, hoisting w2/h2 to the y-loop, or
 * rephrasing the `x == r.left` guard moves it.
 * NOTE: the marker must stay on the line directly above the signature; the
 * verifier only looks 1-3 lines ahead, and this note used to sit between them,
 * which made the function silently uncounted. */
// WIP-FUNCTION: LEGOLAND 0x0045f5f0  (96.3%, y lands in eax not edx at the x-loop head)
void BuildCursorPtr(Cursor* c, void* unused, int refresh)
{
    Rect  r;
    short h;
    int   w2, h2;
    int   sx, sy;
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

/* Find the base-cell id (packed {x,y}) of the placed object `def` whose
 * footprint offset (dx, dy) lands on the cell at the 24.8 world position, by
 * probing the four neighbours (above, below, left, right).  0 when none.
 * [sic] the "below" probe does not null-check its cell (the other three do) —
 * the original's `xor esi,esi` null landing pad falls straight into
 * `test byte [esi+0xc],0x80`.
 *
 * 8.9% (191/191 instructions; the four probe bodies are the right shape and
 * the tail-duplicated `pop edi/esi/ebp/ebx; ret` after each hit is reproduced).
 * The whole residual is ONE allocation decision that then renames every
 * register in the body:
 *   original: eax=x, edi=y, ebp=def, ebx=(the CSE'd g_map->height, later
 *             def->dy), esi=a rematerialised g_map, ecx=cell.
 *             g_map->width is CSE'd into the stack slot [esp+0x14] and
 *             g_map->height into ebx ACROSS PROBES 1 AND 2 ONLY; probes 3 and
 *             4 reload both.  g_map itself is kept in esi with two one-
 *             instruction reload stubs (`mov esi,g_map` at 0x48a4d9 and
 *             0x48a547) placed immediately before the probe they feed, on the
 *             paths where esi had been clobbered by the g_map_rows load.
 *   ours:     ebx=g_map and ebp=g_map_rows are both hoisted into callee-saved
 *             registers for the whole body, so nothing is ever rematerialised
 *             and the width/height values are re-read per probe instead.
 * Because both hoists are legal and cheaper by VC6's own cost model, no source
 * phrasing tried moved it: probe-local `Cell*`, x/y declaration order and
 * scope, `def->dx + c->bx` vs `c->bx + def->dx`, the two sums as named temps
 * inside the guarded block (which is what the original's `mov esi,[ebp+0xc] /
 * mov ebx,[ebp+0x10]` pair in probes 2-4 looks like), the `&&` chain flat vs
 * nested, and reversing the compare operands.  The lever wanted is one that
 * DEMOTES g_map_rows out of a callee-saved register — with only three long-
 * lived values (x, y, def) plus one scratch there is no pressure to force it. */
// WIP-FUNCTION: LEGOLAND 0x0048a3e0  (8.9%, g_map/g_map_rows hoisted where the original rematerialises)
unsigned short GetObjectUID(Pos* wpos, ObjDef* def)
{
    int   x = wpos->x >> 8;
    int   y = wpos->y >> 8;
    Cell* c;

    c = MapCellAt(x, y - 1);
    if (c && (c->flags & 0x80) && c->obj && ((MapObj*)c->obj)->cls == def &&
        def->dx + c->bx == x && def->dy + c->by == y)
        return *(unsigned short*)&c->bx;
    c = MapCellAt(x, y + 1);
    if ((c->flags & 0x80) && c->obj && ((MapObj*)c->obj)->cls == def &&
        def->dx + c->bx == x && def->dy + c->by == y)
        return *(unsigned short*)&c->bx;
    c = MapCellAt(x - 1, y);
    if (c && (c->flags & 0x80) && c->obj && ((MapObj*)c->obj)->cls == def &&
        def->dx + c->bx == x && def->dy + c->by == y)
        return *(unsigned short*)&c->bx;
    c = MapCellAt(x + 1, y);
    if (c && (c->flags & 0x80) && c->obj && ((MapObj*)c->obj)->cls == def &&
        def->dx + c->bx == x && def->dy + c->by == y)
        return *(unsigned short*)&c->bx;
    return 0;
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
 * 74.1% (205/205 instructions, index-for-index over 0-41, 92-105 and 110-204).
 * Residual is one scheduling difference and its knock-on shift:
 *  - idx 42-91: the original keeps ONE `cur->origin.y` / `cur->origin.x` load
 *    alive across the two `foot` stores that use it (and delays the
 *    `bound.right` store past them); VC6 here reloads the origin field, because
 *    the store into the address-taken `foot` is treated as a possible alias.
 *    Hoisting the origins into locals does produce the load-once form but then
 *    costs a `mov ecx,eax` copy — a net loss.  All 16 orderings of the
 *    bound/foot stores, the interleaved forms and three shapes of the
 *    `origin + x` sum were tried; the best is this one.
 *  - idx 106-109: the cell address is built in the same three ops with
 *    g_map_rows loaded one slot later. */
// WIP-FUNCTION: LEGOLAND 0x0045f810  (74.1%, origin-field CSE across the address-taken foot rect)
void ValidateCursor(Cursor* cur, ObjDef* def)
{
    Cursor* root = cur;
    Rect*   r;
    WinRect bound;
    WinRect foot;
    WinRect hit;
    Cell*   cell;
    ObjDef* under;
    int     x, y;
    int     people;

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
            foot.top = r->top + cur->origin.y;
            foot.bottom = r->bottom + cur->origin.y;
            foot.left = r->left + cur->origin.x;
            foot.right = r->right + cur->origin.x;
            if (IntersectRect(&hit, &foot, &bound)) {
                people = CheckForPeople(&hit);
                if (people != -1) {
                    if (people == 1)
                        SetCursorError(cur, 3);
                } else {
                    SetCursorError(cur, 4);
                }
            }
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
