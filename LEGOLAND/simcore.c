/* LEGOLAND — routing, bloke perception and map-interaction internals.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; names are ours.
 *
 * NONE of these functions is exported.  The names are the ones already-matched
 * callers invented for the addresses (input.c: TriggerSwitch, bighelp.c:
 * UpdateMapDrag, pathtile2.c: GetPathNeighbours, bnvmove.c:
 * ScanBlokeSurroundings, popup.c: RequestRoute); every one of them turned out
 * to describe what the code actually does, so nothing is renamed here.
 * IsAdjacentPos (0x00450500) had no name at all -- it is a direct callee of
 * ScanBlokeSurroundings and is named here for the first time.
 *
 * ---------------------------------------------------------------------------
 * WHAT IS IN HERE
 *
 *   0x00450500  IsAdjacentPos          Manhattan distance == 1 between two tiles
 *   0x00450530  ScanBlokeSurroundings  the 9x9 mood/retarget sweep, every 16 ticks
 *   0x00452030  UpdateMapDrag          rubber-band drag -> edit-cursor footprint
 *   0x0045c440  GetPathNeighbours      the 8-neighbour walkable-path mask
 *   0x00460560  TriggerSwitch          throw a park switch: lay a bridge deck
 *   0x00477bd0  RequestRoute           auto-path: best-first route + AddBasicPath
 *
 * ---------------------------------------------------------------------------
 * THINGS LEARNED ABOUT THE DATA (details are above each function)
 *
 * - The perimeter/terrain render objects (pathgfx.c's TerrainObj chain, head
 *   0x00667ca8) are not only cliffs: a record whose image word carries a
 *   non-zero SECOND byte is a DRAWBRIDGE, and that byte is the index of the
 *   park switch that lowers it (switch n matches selector n+1).  The four
 *   switch flags live in the int array at 0x00832be0, right in front of the
 *   exported `PathSprite` pointer.
 * - A bloke's mood is not one number: AdjustMood kinds 2..7 are separate
 *   channels, and ScanBlokeSurroundings feeds five of them from one 9x9 sweep
 *   (hemmed-in, scenery + emptiness, and the three attraction-class buckets)
 *   plus an age bonus.
 * - Bloke +0x14 is the LLIDB element of the class the bloke is currently
 *   heading for; +0x2c/+0x30 is its saved target, normally 24.8 world units --
 *   except in the type-3 arm below, which writes tile units.
 * - An object class's +0x0c/+0x10 pair is the offset from its BASE cell to the
 *   tile a visitor stands on to use it, +0x2a its notice radius, +0x36 its
 *   mood value and +0x20 its 1..5 behaviour bucket.
 * - The GamePad block's +0x2c..+0x50 run is the whole drag state: step size,
 *   drag anchor, current cursor cell and the published selection box; flags
 *   bit 0x800 is the axis lock.
 * - The route search keeps TWO intrusive lists through one link word: the open
 *   list at 0x00668fc0 sorted by f, and the closed list at 0x00668fc4.
 * - GetPathNeighbours is a byte-identical SECOND COPY of bigsim.c's
 *   Get_Path_Directions (0x0045c050): 322 instructions, 1005 bytes each.
 * ------------------------------------------------------------------------- */
#include "legoland.h"
#include <math.h>

/* ------------------------------------------------------------------ types -- */

/* The map/config record at 0x004bcbf4 (exported as `lpConfig`), extended with
 * the viewport origin at +0x20/+0x22 (same view pathbuild.c takes of it). */
typedef struct MapHdr {
    char           pad00[0x20]; /* +0x00 (+0x14 width, +0x16 height) */
    unsigned short origin_x;    /* +0x20 viewport origin, pixels */
    unsigned short origin_y;    /* +0x22 */
} MapHdr;

/* A perimeter/terrain render object (pathgfx.c's TerrainObj, 0x24 bytes).
 * `image` is the .MAP record's image word: the low byte is the terrain .ILF
 * frame, the SECOND byte selects the bridge .ILF.  TriggerSwitch reads it
 * with an ARITHMETIC shift, so the field is a signed int here. */
typedef struct TerrainObj {
    char               pad00[0x10]; /* +0x00 iso x/y/x2/y2 of the .MAP record */
    int                image;       /* +0x10 */
    int                sx;          /* +0x14 draw x */
    int                sy;          /* +0x18 draw y */
    struct TerrainObj* next;        /* +0x1c */
    void*              sprite;      /* +0x20 */
} TerrainObj;



/* An object class descriptor's footprint rect (ObjDef +0x3c, legoland.h's
 * ObjClass.rect); only the four edges are read here. */
typedef struct DragClass {
    char pad00[0x3c];   /* +0x00 */
    Rect rect;          /* +0x3c */
} DragClass;


/* A "bloke" (visitor / worker), stride 172; only the fields this file reads.
 * Same record blokeai.c / bnvmove.c / workers2.c document. */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00 */
    void*          person;      /* +0x04 */
    unsigned char  pad08[4];    /* +0x08..0x0b */
    unsigned short plan;        /* +0x0c  long-term action */
    unsigned short state;       /* +0x0e  low-level AI state */
    unsigned char  pad10[4];    /* +0x10..0x13 */
    void*          focus;       /* +0x14  LLIDB element of the class it is heading for */
    unsigned char  pad18[0x14]; /* +0x18..0x2b */
    Pos            saved;       /* +0x2c  saved target */
    unsigned char  pad34[0x2c]; /* +0x34..0x5f */
    unsigned char  action;      /* +0x60  progress code within the plan */
    unsigned char  pad61;       /* +0x61 */
    unsigned short flags62;     /* +0x62  0x20 = inside a ride */
    unsigned char  pad64[4];    /* +0x64..0x67 */
    Pos            world;       /* +0x68  world position, 24.8 */
    unsigned char  pad70[0x3c]; /* +0x70..0xab */
} Bloke;

/* An object class descriptor (ObjDef, 0xd0 bytes) as this file reads it. */
typedef struct ObjDef {
    char           pad00[0x0c];  /* +0x00 */
    int            access_x;     /* +0x0c  base-cell -> access tile offset */
    int            access_y;     /* +0x10 */
    char           pad14[0x20 - 0x14];
    short          type;         /* +0x20  1..5 stat/behaviour bucket */
    char           pad22[0x2a - 0x22];
    short          range;        /* +0x2a  notice radius, >= 1 */
    char           pad2c[0x36 - 0x2c];
    short          value;        /* +0x36  mood contribution */
    char           pad38[0xc4 - 0x38];
    void*          elem;         /* +0xc4  this class's LLIDB element */
    char           padc8[0xd0 - 0xc8];
} ObjDef;

/* A placed map object: its class sits at +0x0c (objmap2.c's MapObj). */
typedef struct MapObj {
    char    pad0[0x0c];          /* +0x00 */
    ObjDef* cls;                 /* +0x0c */
} MapObj;

/* The game-button / cursor state block at 0x00813a40 (exported as `GamePad`;
 * bighelp.c calls it GameInput).  Only the fields UpdateMapDrag touches are
 * named; +0x2c..+0x50 are the drag block. */
typedef struct GameInput {
    unsigned int flags;      /* +0x00 0x800 = lock the drag to one axis */
    char         pad04[0x2c - 0x04];
    int          step_w;     /* +0x2c 0x813a6c  drag step, cells */
    int          step_h;     /* +0x30 0x813a70 */
    int          anchor_x;   /* +0x34 0x813a74  map ref where the drag started */
    int          anchor_y;   /* +0x38 0x813a78 */
    int          click_x;    /* +0x3c 0x813a7c  map ref under the cursor now */
    int          click_y;    /* +0x40 0x813a80 */
    int          sel_x0;     /* +0x44 0x813a84  the published drag box */
    int          sel_y0;     /* +0x48 0x813a88 */
    int          sel_x1;     /* +0x4c 0x813a8c */
    int          sel_y1;     /* +0x50 0x813a90 */
} GameInput;

/* ---------------------------------------------------------------- globals -- */

extern int         g_scroll_x;      /* 0x00667cb4  24.8 fixed point (ScrollX) */
extern int         g_scroll_y;      /* 0x00667cb8  (ScrollY) */
extern TerrainObj* g_terrain_objs;  /* 0x00667ca8  (OverlayList) */
extern ObjDef*     g_env_class;     /* 0x007fd624  the environment object class */
extern void*       g_path_tile_ptr; /* 0x00832bf0  (PathSprite): first word = tile code */
extern GameInput   g_input;         /* 0x00813a40  (GamePad) */
extern Pos         g_route_from;     /* 0x004bb598  the request the node factory reads */
extern Pos         g_route_to;       /* 0x004bb5a0 */
extern struct RouteNode* g_open_head;   /* 0x00668fc0  open list, sorted by f */
extern struct RouteNode* g_closed_head; /* 0x00668fc4  closed list */
extern int         g_switches[4];   /* 0x00832be0  one flag per park switch */
extern unsigned int g_edit_cursor_next; /* 0x008003f0  edit cursor +0x1830 */
extern Pos         g_edit_cursor_origin;/* 0x007fffc4  edit cursor +0x1404 */
extern Rect        g_edit_cursor_rect;  /* 0x007fffd4  edit cursor +0x1414 */
extern char        g_edit_cursor;       /* 0x007febc0  the edit cursor block */
extern int         g_edit_state;        /* 0x008119b0  (EditMode) */
extern DragClass*  g_edit_object;       /* 0x008119b8 */
extern DragClass*  g_drag_class;        /* 0x0080ff6c  class being placed by an EditMode 2 drag */

/* ------------------------------------------------------------ prototypes -- */

extern int  ScreenToMapRef(Pos* screen, Pos* map, int mode);  /* 0x0045be90 */
extern void AddPathTile(Pos* pos, unsigned short tile);       /* 0x0045d3b0 */
extern void ValidateCursor(void* cursor, DragClass* cls);     /* 0x0045f810 */
extern void ResetCursorFootprint(void* cursor);              /* 0x0045f460 */
extern int  abs(int v);
#pragma intrinsic(abs)
extern int  memcmp(const void* a, const void* b, unsigned int n);
extern int  GetBlokeNum(Bloke* b);                             /* 0x00482fb0 */
extern int  GetBlokeCounter(ObjDef* d, int idx);              /* 0x00480ee0 */
extern int  AdjustMood(Bloke* b, int kind, int amount);       /* 0x00482df0 */
extern signed char GetBlokeAgeGroup(Bloke* b);                /* 0x0044eb10 */
extern void NewLongTermAction(Bloke* b, int action);          /* 0x0044e760 */
extern int  Calc_Item_Attractiveness(ObjDef* d, Bloke* b, int viewing); /* 0x004814c0 */
extern int  IsAdjacentPos(Pos* a, Pos* b);                    /* 0x00450500 */
extern int  rand(void);                                       /* 0x0049e4b2 (CRT) */
extern struct RouteNode* GetRouteNode(Pos* pos, int* state);   /* 0x004777f0 */
extern void AddOpenNode(struct RouteNode* n);                 /* 0x004776e0 */
extern void AddClosedNode(struct RouteNode* n);               /* 0x004776c0 */
extern void RemoveOpenNode(struct RouteNode* n);              /* 0x00477790 */
extern void RemoveClosedNode(struct RouteNode* n);            /* 0x00477760 */
extern int  RouteInBounds(int x, int y);                      /* 0x00477680 */
extern int  RouteStepAxis(int x0, int y0, int x1, int y1);    /* 0x004779a0 */
extern int  RouteTurnCost(int a, int b);                      /* 0x00477980 */
extern void ClearCellForPath(Pos* pos);                       /* 0x004779d0 */
extern void AddBasicPath(void* obj, Pos* pos);                /* 0x0045dbe0 */
extern Elem* ElemID(const char* name);                        /* 0x0047b3f0 */

/* -------------------------------------------------------------- functions -- */

/* Throw park switch `which` (0..3; the ":DIGGER" cheat in input.c throws all
 * four).  A switch opens the drawbridge whose perimeter record carries the
 * bridge selector `which + 1` in the second byte of its image word: every
 * matching terrain object is converted from its recorded DRAW position back to
 * a map cell (undo the viewport origin and the 24.8 scroll, then
 * ScreenToMapRef) and a fixed bridge-deck footprint is stamped into the grid
 * around it.
 *
 * The footprint is 10 tiles long and 2 wide, and the low byte of the image
 * word picks the orientation:
 *   frame 0  -> origin (cell.x + 15, cell.y),      2 wide x 10 tall
 *   frame !=0 -> origin (cell.x + 10, cell.y + 6), 10 wide x 2 tall
 *
 * Per deck cell: the environment class's LLIDB element becomes the cell's
 * object, RF bit 0 (walkable path) is set and RF bit 1 (blocked) cleared, map
 * flags gain 0x48 and lose 0x8000, the path tile code is painted, the cell's
 * own coordinates are written into the +0x04/+0x05 back-reference pair, and
 * AddPathTile re-shapes the path graphics and registers the path square.
 *
 * NOTE the loops are NOT bounds-checked: a bridge object near the map edge
 * writes past its row.  Reproduced as found.
 *
 * The switch flag is set even when the chain head is null (the early-out path
 * has its own copy of the epilogue). */
// FUNCTION: LEGOLAND 0x00460560
void TriggerSwitch(int which)
{
    TerrainObj* node;
    Pos screen;
    Pos tile;
    int rows;
    int cols;

    node = g_terrain_objs;
    while (node) {
        if ((node->image >> 8) == which + 1) {
            screen.x = ((MapHdr*)g_map)->origin_x - (g_scroll_x >> 8) + node->sx;
            screen.y = ((MapHdr*)g_map)->origin_y - (g_scroll_y >> 8) + node->sy;
            ScreenToMapRef(&screen, &tile, 0);
            if ((node->image & 0xff) == 0) {
                tile.x += 15;
                rows = 10;
                do {
                    cols = 2;
                    do {
                        g_map_rows[tile.y][tile.x].obj = g_env_class->elem;
                        g_map_rows[tile.y][tile.x].rf |= 1;
                        g_map_rows[tile.y][tile.x].rf &= ~2;
                        g_map_rows[tile.y][tile.x].flags |= 0x48;
                        g_map_rows[tile.y][tile.x].flags &= 0x7fff;
                        g_map_rows[tile.y][tile.x].tile = *(unsigned short*)g_path_tile_ptr;
                        g_map_rows[tile.y][tile.x].bx = (unsigned char)tile.x;
                        g_map_rows[tile.y][tile.x].by = (unsigned char)tile.y;
                        AddPathTile(&tile, *(unsigned short*)g_path_tile_ptr);
                        tile.x++;
                    } while (--cols);
                    tile.x -= 2;
                    tile.y++;
                } while (--rows);
            } else {
                tile.x += 10;
                tile.y += 6;
                rows = 2;
                do {
                    cols = 10;
                    do {
                        g_map_rows[tile.y][tile.x].obj = g_env_class->elem;
                        g_map_rows[tile.y][tile.x].rf |= 1;
                        g_map_rows[tile.y][tile.x].rf &= ~2;
                        g_map_rows[tile.y][tile.x].flags |= 0x48;
                        g_map_rows[tile.y][tile.x].flags &= 0x7fff;
                        g_map_rows[tile.y][tile.x].tile = *(unsigned short*)g_path_tile_ptr;
                        g_map_rows[tile.y][tile.x].bx = (unsigned char)tile.x;
                        g_map_rows[tile.y][tile.x].by = (unsigned char)tile.y;
                        AddPathTile(&tile, *(unsigned short*)g_path_tile_ptr);
                        tile.x++;
                    } while (--cols);
                    tile.x -= 10;
                    tile.y++;
                } while (--rows);
            }
        }
        node = node->next;
    }
    g_switches[which] = 1;
}

/* ---- UpdateMapDrag ------------------------------------------------------ */

/* Recompute the edit cursor's footprint from the current rubber-band drag.
 *
 * ReadGameButtons (bighelp.c) calls this every tick while an in-game click is
 * live.  BeginMapClick (0x00452390) has stamped the drag ANCHOR into
 * g_input.anchor_x/anchor_y (+0x34/+0x38) and the current map ref lands in
 * g_input.click_x/click_y (+0x3c/+0x40) each tick.
 *
 * Steps:
 *  1. flags bit 0x800 = "lock the drag to one axis": whichever of |dx|/|dy| is
 *     the larger wins and the other coordinate snaps back to the anchor.
 *  2. EditMode 2 (the destroy/query drag) just takes the bounding box of the
 *     anchor and the cursor and publishes it in g_input.sel_x0..sel_y1
 *     (+0x44..+0x50); the drag step (+0x2c/+0x30) is the placing class's
 *     footprint size, or 1x1 when there is no class.  It leaves `ok` 0, so the
 *     tail always resets the cursor footprint.
 *  3. Otherwise the step is the EDIT class's footprint w x h and the bounding
 *     box is rounded OUT to a whole number of footprints.  A drag that runs
 *     back past the anchor (min == cursor, max == anchor) grows away from the
 *     anchor instead: the anchor keeps its own full footprint
 *     (max = anchor + w - 1) and the min end moves.  `ok` is set when the
 *     anchor and the cursor are on the same cell, or when the rounded box is
 *     still within one footprint of the raw box.
 *  4. EditMode 1 with a class offsets the published box by the class's own
 *     rect origin.
 *  5. The tail rewrites the edit cursor: origin = the box's top-left, the
 *     single footprint rect = the published box made origin-relative, and then
 *     ValidateCursor (`ok`) or ResetCursorFootprint (not `ok`).
 *
 * Note step 3's division is signed and by w/h, which BeginMapClick guarantees
 * to be >= 1; a class with a degenerate rect would divide by zero. */
// FUNCTION: LEGOLAND 0x00452030
void UpdateMapDrag(void)
{
    int minx;
    int miny;
    int maxx;
    int maxy;
    int ok;

    ok = 0;
    if (g_input.flags & 0x800) {
        if (abs(g_input.anchor_x - g_input.click_x) > abs(g_input.anchor_y - g_input.click_y))
            g_input.click_y = g_input.anchor_y;
        else
            g_input.click_x = g_input.anchor_x;
    }
    if (g_edit_state == 2) {
        if (g_input.anchor_x < g_input.click_x)
            minx = g_input.anchor_x;
        else
            minx = g_input.click_x;
        if (g_input.anchor_y < g_input.click_y)
            miny = g_input.anchor_y;
        else
            miny = g_input.click_y;
        if (g_input.anchor_x > g_input.click_x)
            maxx = g_input.anchor_x;
        else
            maxx = g_input.click_x;
        if (g_input.anchor_y > g_input.click_y)
            maxy = g_input.anchor_y;
        else
            maxy = g_input.click_y;
        if (g_drag_class) {
            g_input.step_w = g_drag_class->rect.right - g_drag_class->rect.left + 1;
            g_input.step_h = g_drag_class->rect.bottom - g_drag_class->rect.top + 1;
            g_input.sel_x0 = minx;
            g_input.sel_y0 = miny;
            g_input.sel_x1 = maxx;
            g_input.sel_y1 = maxy;
        } else {
            g_input.step_w = 1;
            g_input.step_h = 1;
            g_input.sel_x0 = minx;
            g_input.sel_y0 = miny;
            g_input.sel_x1 = maxx;
            g_input.sel_y1 = maxy;
        }
    } else {
        if (g_input.anchor_x < g_input.click_x)
            minx = g_input.anchor_x;
        else
            minx = g_input.click_x;
        if (g_input.anchor_y < g_input.click_y)
            miny = g_input.anchor_y;
        else
            miny = g_input.click_y;
        if (g_input.anchor_x > g_input.click_x)
            maxx = g_input.anchor_x;
        else
            maxx = g_input.click_x;
        if (g_input.anchor_y > g_input.click_y)
            maxy = g_input.anchor_y;
        else
            maxy = g_input.click_y;
        g_input.step_w = g_edit_object->rect.right - g_edit_object->rect.left + 1;
        g_input.step_h = g_edit_object->rect.bottom - g_edit_object->rect.top + 1;
        if (memcmp(&g_input.anchor_x, &g_input.click_x, 8) != 0) {
            if (g_input.step_w > 1 && minx == g_input.click_x && maxx == g_input.anchor_x) {
                minx = maxx - ((g_input.step_w - minx + maxx) / g_input.step_w) * g_input.step_w;
                maxx = (g_input.step_w - 1) + g_input.anchor_x;
            } else {
                maxx = ((g_input.step_w - minx + maxx) / g_input.step_w) * g_input.step_w + minx - 1;
            }
            if (g_input.step_h > 1 && miny == g_input.click_y && maxy == g_input.anchor_y) {
                miny = maxy - ((g_input.step_h - miny + maxy) / g_input.step_h) * g_input.step_h;
                maxy = g_input.anchor_y + g_input.step_h - 1;
            } else {
                maxy = ((g_input.step_h - miny + maxy) / g_input.step_h) * g_input.step_h + miny - 1;
            }
            if (abs(minx - maxx) <= g_input.step_w && abs(miny - maxy) <= g_input.step_h)
                ok = 1;
        } else {
            ok = 1;
            maxx = ((g_input.step_w - minx + maxx) / g_input.step_w) * g_input.step_w + minx - 1;
            maxy = ((g_input.step_h - miny + maxy) / g_input.step_h) * g_input.step_h + miny - 1;
        }
        if (g_edit_state == 1 && g_edit_object) {
            g_input.sel_x0 = g_edit_object->rect.left + minx;
            g_input.sel_y0 = g_edit_object->rect.top + miny;
            g_input.sel_x1 = g_edit_object->rect.left + maxx;
            g_input.sel_y1 = g_edit_object->rect.top + maxy;
        }
    }
    g_edit_cursor_rect.right  = g_input.sel_x1 - minx;
    g_edit_cursor_rect.bottom = g_input.sel_y1 - miny;
    g_edit_cursor_rect.left   = g_input.sel_x0 - minx;
    g_edit_cursor_rect.top    = g_input.sel_y0 - miny;
    g_edit_cursor_rect.next   = 0;
    g_edit_cursor_origin.x    = minx;
    g_edit_cursor_origin.y    = miny;
    g_edit_cursor_next        = 0;
    if (ok)
        ValidateCursor(&g_edit_cursor, g_edit_object);
    else
        ResetCursorFootprint(&g_edit_cursor);
}

/* ---- GetPathNeighbours -------------------------------------------------- */

/* One neighbour probe: copy the cell (an off-map cell stands in as
 * flags=0x40/rf=0) and apply SetPathFlag's walkable-path predicate.  A MACRO
 * over one function-level `cell`, not a helper with its own local: VC6 keeps
 * each off-map `cell.rf = 0` store alive because the NEXT probe reads cell.rf
 * back after its copy (it does not treat the rep movsd as a kill), and drops
 * only the last probe's -- exactly the original's pattern.  (Same macro as
 * bigsim.c's Get_Path_Directions; the two functions are byte-identical.) */
#define PROBE(px, py) \
    if ((px) >= 0 && (px) < g_map->width && (py) >= 0 && (py) < g_map->height) { \
        cell = g_map_rows[(py)][(px)]; \
    } else { \
        cell.flags = 0x40; \
        cell.rf = 0; \
    } \
    rf = cell.rf; \
    if ((rf & 1) || ((cell.flags & 0x10) && !(rf & 2)))

/* The 8-neighbour walkable-path mask around `pos` (pathtile2.c's header
 * documents the bit layout and the RF shape bits it feeds):
 *
 *      0x80 0x01 0x02        NW N NE
 *      0x40      0x04        W     E
 *      0x20 0x10 0x08        SW S SE
 *
 * probed in the order N, S, E, W, NE, SE, SW, NW.  A neighbour counts when
 * RF bit 0 (walkable path) is set, or when map flag 0x10 (path tile) is set
 * and RF bit 1 (blocked) is not -- the same predicate SetPathFlag
 * (pathbuild.c) uses on a walker.  Anything off the map is treated as a cell
 * with flags 0x40 and rf 0, i.e. never a path.
 *
 * *ortho and *diag come back with the number of orthogonal / diagonal hits
 * (both optional).  bnvmove.c declares the two pointer parameters as int
 * because it passes literal 0s.
 *
 * This function is a SECOND, byte-identical compiled copy of
 * Get_Path_Directions @ 0x0045c050 (bigsim.c): 322 instructions and 1005
 * bytes each, differing only in the branch displacements.  The two were the
 * same routine in two translation units (or a header `__inline` emitted
 * twice); the callers use them interchangeably. */
// FUNCTION: LEGOLAND 0x0045c440
unsigned char GetPathNeighbours(Pos* pos, unsigned char* n_ortho, unsigned char* n_diag)
{
    Cell cell;
    unsigned char rf;
    unsigned char diag = 0;
    unsigned char ortho = 0;
    unsigned char mask = 0;
    int x = pos->x;
    int y = pos->y;
    int ym1 = y - 1;
    int yp1;
    int xp1;
    int xm1;

    PROBE(x, ym1)   { ortho++; mask |= 0x01; }
    yp1 = y + 1;
    PROBE(x, yp1)   { ortho++; mask |= 0x10; }
    xp1 = x + 1;
    PROBE(xp1, y)   { ortho++; mask |= 0x04; }
    xm1 = x - 1;
    PROBE(xm1, y)   { ortho++; mask |= 0x40; }
    PROBE(xp1, ym1) { diag++;  mask |= 0x02; }
    PROBE(xp1, yp1) { diag++;  mask |= 0x08; }
    PROBE(xm1, yp1) { diag++;  mask |= 0x20; }
    PROBE(xm1, ym1) { diag++;  mask |= 0x80; }
    if (n_ortho)
        *n_ortho = ortho;
    if (n_diag)
        *n_diag = diag;
    return mask;
}

/* ---- ScanBlokeSurroundings ---------------------------------------------- */

/* Look around a bloke and fold what it sees into its mood, retargeting it if
 * something nearby is attractive enough.
 *
 * ControlPeople (bnvmove.c) calls this every 16th tick for every bloke that
 * is not inside a ride, right after AdjustMood(b, 8, 1).  It sweeps the 9x9
 * block of cells centred on the bloke's own tile (world >> 8) and produces
 * five mood deltas, applied in one go at the end as AdjustMood kinds 2..6:
 *
 *   kind 2  += 1 per cell that is off the map          (feeling hemmed in)
 *   kind 3  += the class's `value` for a type-2 object (scenery), and
 *             += -20/manhattan_distance for every EMPTY cell (nothing to look
 *             at nearby is depressing; the bloke's own cell is skipped so the
 *             division cannot trap)
 *   kind 4  += value >> visit_count for a type-3 object
 *   kind 5  += value >> visit_count for a type-4 or type-5 object
 *   kind 6  += value >> visit_count for a type-1 object
 *
 * A cell only counts as an object when it holds one AND map flag 0x80 is set
 * (the object's BASE cell), and only when the bloke is within the class's own
 * range (+0x2a, clamped to >= 1 by the ODF loader) in BOTH axes -- so `range`
 * is a Chebyshev radius on top of the fixed 9x9 window.
 *
 * Retargeting, per object type:
 *  - type 3 (the "wander in" classes): with probability 15/(visits+1) percent,
 *    and only if the bloke is not already focussed on this class, is not in a
 *    ride, is not already in low-level state 15 and is within 1 tile, the
 *    bloke is given long-term action 15 with the object's BASE CELL as its
 *    saved target.  NOTE that target is written in TILE units while every
 *    other writer of +0x2c/+0x30 uses 24.8 world units -- reproduced as found.
 *  - types 1, 4 and 5 (the queued attractions): the object's ACCESS tile
 *    (base cell + the class's +0x0c/+0x10 offset) must be orthogonally
 *    adjacent both to the bloke and to the cell being scanned, the class must
 *    score over 50 on Calc_Item_Attractiveness(viewing=1), and the bloke must
 *    be on plan 6; then its saved target becomes that access tile in 24.8,
 *    action 1, state 0, focus = the class's LLIDB element.
 *
 * Finally the bloke's age group (3 = adult and up) adds AdjustMood(b, 7,
 * age - 2).
 *
 * The two "queued attraction" arms are textually identical apart from the
 * accumulator and the Pos temporary; VC6 cross-jumps the type-1 copy into the
 * type-4/5 copy's tail from the first coordinate compare onwards. */
// FUNCTION: LEGOLAND 0x00450530
void ScanBlokeSurroundings(Bloke* b)
{
    int     m3;
    int     m2;
    int     m4;
    int     m5;
    int     m6;
    float   chance;
    Pos     p;
    Pos     me;
    Cell*   cell;
    ObjDef* d;
    int     n;
    int     age;

    m6 = 0;
    m3 = 0;
    m5 = 0;
    m4 = 0;
    m2 = 0;
    me.x = b->world.x >> 8;
    me.y = b->world.y >> 8;
    for (p.y = me.y - 4; p.y <= me.y + 4; p.y++) {
        for (p.x = me.x - 4; p.x <= me.x + 4; p.x++) {
            if (p.x < 0 || p.x >= g_map->width ||
                p.y < 0 || p.y >= g_map->height ||
                (cell = &g_map_rows[p.y][p.x]) == 0) {
                m2++;
            } else if (cell->obj != 0 && (cell->flags & 0x80)) {
                d = ((MapObj*)cell->obj)->cls;
                if (abs(me.x - p.x) > d->range)
                    continue;
                if (abs(me.y - p.y) > d->range)
                    continue;
                switch (d->type) {
                case 2:
                    m3 += d->value;
                    break;
                case 3:
                    m4 += d->value >> GetBlokeCounter(d, GetBlokeNum(b));
                    if (b->flags62 & 0x20)
                        break;
                    if (b->focus == d->elem)
                        break;
                    chance = (float)(15 / (GetBlokeCounter(d, GetBlokeNum(b)) + 1));
                    if (rand() % 100 < chance) {
                        if (b->state == 0xf)
                            break;
                        age = b->world.x >> 8;
                        n = b->world.y >> 8;
                        if ((int)sqrt((double)(abs(age - p.x) * abs(age - p.x) + abs(n - p.y) * abs(n - p.y))) <= 1) {
                            b->focus = d->elem;
                            b->saved.x = cell->bx;
                            b->saved.y = cell->by;
                            NewLongTermAction(b, 0xf);
                        }
                    }
                    break;
                case 4:
                case 5:
                    {
                    Pos spot;
                    m5 += d->value >> GetBlokeCounter(d, GetBlokeNum(b));
                    spot.x = cell->bx + d->access_x;
                    spot.y = cell->by + d->access_y;
                    if (!IsAdjacentPos(&me, &spot))
                        break;
                    if (!IsAdjacentPos(&p, &spot))
                        break;
                    if (Calc_Item_Attractiveness(d, b, 1) <= 0x32)
                        break;
                    if (b->plan != 6)
                        break;
                    if ((b->saved.x >> 8) == spot.x && (b->saved.y >> 8) == spot.y)
                        break;
                    b->saved.x = spot.x << 8;
                    b->saved.y = spot.y << 8;
                    b->action = 1;
                    b->state = 0;
                    b->focus = d->elem;
                    }
                    break;
                case 1:
                    {
                    Pos spot2;
                    m6 += d->value >> GetBlokeCounter(d, GetBlokeNum(b));
                    spot2.x = cell->bx + d->access_x;
                    spot2.y = cell->by + d->access_y;
                    if (!IsAdjacentPos(&spot2, &me))
                        break;
                    if (!IsAdjacentPos(&p, &spot2))
                        break;
                    if (Calc_Item_Attractiveness(d, b, 1) <= 0x32)
                        break;
                    if (b->plan != 6)
                        break;
                    if ((b->saved.x >> 8) == spot2.x && (b->saved.y >> 8) == spot2.y)
                        break;
                    b->saved.x = spot2.x << 8;
                    b->saved.y = spot2.y << 8;
                    b->action = 1;
                    b->state = 0;
                    b->focus = d->elem;
                    }
                    break;
                }
            } else if (cell->obj == 0) {
                n = abs(me.x - p.x) + abs(me.y - p.y);
                if (n != 0)
                    m3 += -20 / n;
            }
        }
    }
    AdjustMood(b, 2, m2);
    AdjustMood(b, 3, m3);
    AdjustMood(b, 4, m4);
    AdjustMood(b, 5, m5);
    AdjustMood(b, 6, m6);
    age = GetBlokeAgeGroup(b);
    if (age >= 3)
        AdjustMood(b, 7, age - 2);
}

/* ---- RequestRoute ------------------------------------------------------- */

/* One node of the auto-path route search: 0x28 bytes, malloc'd by
 * GetRouteNode, threaded through the open list (0x00668fc0, kept sorted by
 * `f`) and then the closed list (0x00668fc4) by the same +0x00 link. */
typedef struct RouteNode {
    struct RouteNode* next;    /* +0x00 open/closed list link */
    struct RouteNode* parent;  /* +0x04 how we got here */
    Pos               pos;     /* +0x08 map tile */
    int               cost;    /* +0x10 terrain cost of entering, -1 = impassable */
    int               g;       /* +0x14 cost so far */
    int               h;       /* +0x18 heuristic remaining */
    int               f;       /* +0x1c g + h, the open-list sort key */
    int               axis;    /* +0x20 pre-existing axis (0/1 = no turn charge) */
    int               dir;     /* +0x24 axis of the step that reached this node */
} RouteNode;

/* The bounds-checked cell fetch, taking the Pos BY POINTER so the y read
 * stays lazy (objmap.c's CopyMapCell reads its coordinates the same way). */
static __inline Cell* RouteCellAt(Pos* p)
{
    if (p->x >= 0 && p->x < g_map->width && p->y >= 0 && p->y < g_map->height)
        return &g_map_rows[p->y][p->x];
    return 0;
}

/* Lay a path along the best route the search found from (fx,fy) to (tx,ty).
 *
 * This is the auto-path builder the map UI calls (popup.c declares it as
 * RequestRoute(fx, fy, tx, ty)); the request itself is published in the four
 * ints at 0x004bb598 so the node factory can see it.
 *
 * The search is a Dijkstra/best-first walk over the four orthogonal tile
 * neighbours in the order N, E, S, W.  Nodes live on two intrusive lists: the
 * OPEN list (0x00668fc0), kept sorted by `f` ascending, and the CLOSED list
 * (0x00668fc4).  GetRouteNode(pos, &state) hands back the node for a tile and
 * reports through `state` whether it came off the closed list (2), the open
 * list (1) or was freshly allocated and costed (0).
 *
 * Per popped node: if it is the target, or its own `axis` field is 1, it
 * becomes the best node and is NOT expanded.  Otherwise each neighbour that is
 * on the map and passable (cost != -1) is costed as
 *      g = cur->g + cost  [+ RouteTurnCost(cur->dir, step axis) when the
 *                          neighbour's `axis` is neither 0 nor 1]
 * and linked in unless a best node already exists with a cheaper g.
 *
 * TWO ORIGINAL QUIRKS, reproduced:
 *  - the relaxation is guarded by `state == 0`, so the `state == 2` /
 *    `state == 1` tests inside it can never fire.  They are still emitted
 *    because `state`'s address escaped into GetRouteNode, so VC6 cannot fold
 *    them.
 *  - the first (north) neighbour tests the terrain cost BEFORE stamping
 *    nn->dir; the other three stamp it first.  That asymmetry is in the
 *    original source, not an artefact.
 *
 * Once the open list runs dry the parent chain from the best node is walked
 * and, for each tile that is not already a path (map flag 0x10) and whose RF
 * bits 0|1 are not both set, the cell is cleared (0x004779d0) and
 * AddBasicPath lays the "PATH CONTROL" class on it.
 *
 * THE CELL POINTER IS NOT NULL-CHECKED before `cell->flags` is read: a route
 * node whose tile is off the map dereferences 0x0c.  The nodes all come from
 * in-bounds neighbours so it cannot happen in practice; reproduced as found.
 *
 * The teardown only UNLINKS every closed node (RemoveClosedNode in a loop) --
 * the 0x28-byte allocations are never freed.  That leak is in the original.
 *
 * SIGNATURE: popup.c declares this `RequestRoute(int fx, int fy, int tx, int
 * ty)`.  The ABI is the same four dwords, but taking them as two Pos values is
 * a FRAME LEVER, not a style choice: the original homes the neighbour tile in
 * the tx/ty argument slots (R+0x0c/R+0x10) and `state` in fx's (R+4), and VC6
 * only hands a dead PAIR of parameter slots to an 8-byte local when that pair
 * IS one parameter.  Declared as four ints the function grows `sub esp,8`. */
/* 479/482 instructions, 1336/1336 bytes.  The three that differ are one
 * register-allocation decision in the first (north) neighbour block: the
 * original keeps `cur->pos.x` live in eax across the goal test, so the
 * y-comparison's `g_route_to.y` load lands in edx (idx 31/32) and the
 * neighbour's x is a COPY (`mov ecx,eax`, idx 38).  VC6 here splits that live
 * range instead -- it puts `g_route_to.y` in eax and rematerialises
 * `mov ecx,[esi+8]` in the else block.  Same instruction COUNT and byte
 * length, same semantics.  Every spelling tried (to.x-first vs to.y-first,
 * struct copy, memcpy, named temporaries for either operand, assignment
 * inside the condition, nested ifs, the store of g_open_head duplicated into
 * both arms, the route request as two Pos globals / one struct / an int
 * array) either reproduces this or flips the goal test's eax/ecx pair
 * instead, which costs the same three.
 * Re-measured this round: the residual is exactly ONE fact -- the original
 * CSEs `cur->pos.x` across the goal test into the else block, so the
 * y-comparison's global load has to go somewhere other than eax.  Writing the
 * else block as two field assignments (`to.x = cur->pos.x; to.y =
 * cur->pos.y - 1;`) DOES produce that CSE, but then VC6 gives the CSE'd value
 * ecx (the original gives it eax) and the copy `mov ecx,eax` the original
 * needs disappears -- 481 instructions instead of 482.  Passing
 * `cur->pos.x` to RouteInBounds instead of `to.x` gives the CSE in eax but
 * loses the store/push interleave.  All four operand orders of the goal test
 * were tried against both else-block spellings; the struct-copy else is the
 * only one that keeps 482 instructions.  Whoever gets the CSE into eax while
 * keeping `to = cur->pos;` finishes this function. */
/* NEW THIS ROUND -- the goal test's half of the residual is solvable, and the
 * lever is a THIRD use of `cur->pos.x` in the else block.  With two uses (the
 * goal test plus `to.x = cur->pos.x`) VC6 coalesces them into one long live
 * range and gives it ecx, which flips the x-compare to `cmp ecx,eax` and puts
 * `g_route_to.y` in eax (indices 25-32 wrong, 481 instructions).  With three
 * uses -- e.g. `to.x = cur->pos.x; to.y = cur->pos.y - 1;` and then
 * `RouteInBounds(cur->pos.x, to.y)`, or the same two stores plus a redundant
 * `to.x = cur->pos.x;` after them -- indices 25 THROUGH 37 match the original
 * exactly, the CSE lands in eax as the original has it, and the body is 482
 * instructions.  What is left in that shape is only the tail: the original
 * copies the CSE into ecx (`mov ecx,eax`) and uses ecx for BOTH the `to.x`
 * store and the `push`, while the three-use form stores from eax and then
 * reloads a third register for the push.  So the target is now precise: keep
 * the goal test's allocation of the three-use form and make the else block's
 * `to.x` a COPY of eax rather than a fresh load.  (Measured: 4 orderings of
 * the goal test x 2 else-block spellings, `to = cur->pos`, field assignments
 * in both orders, a named `cx` temporary defined before or inside the else,
 * and hoisting `to = cur->pos` above the test.) */
/* THIS ROUND, three more shapes measured and all rejected -- the residual is
 * unchanged at indices 31/32/38.  (i) A `Pos* cp = &cur->pos;` alias for the
 * else block, for the goal test, or for both: VC6 folds the alias back to
 * `[esi+8]` but then treats it as a SEPARATE value, so the goal test's read is
 * not CSE'd at all and index 38 stays a reload -- the opposite of what is
 * wanted.  (This is the lever that finished workorder2.c's RefreshObjList; it
 * moves a commutative operand ORDER, it does not create a register copy.)
 * (ii) `to.x = cur->pos.x; to.y = cur->pos.y - 1;` with the RouteInBounds
 * argument spelled `cur->pos.x` (three uses) still reproduces indices 25-37
 * exactly and only loses the store/push interleave.  (iii) All four operand
 * orders of the goal test are inert -- the compare direction follows the
 * source but the register assignment does not. */
/* THIS ROUND the residual was localised to ONE allocator tie-break, and the two
 * escape routes were both measured to a dead end.  All twelve combinations of
 * {`to = cur->pos; to.y--;` | `to.x = cur->pos.x; to.y = cur->pos.y - 1;` |
 * the same two in the other order} x {RouteInBounds(to.x,to.y) |
 * (cur->pos.x,to.y) | (to.x,cur->pos.y-1) | (cur->pos.x,cur->pos.y-1)} were
 * compiled; only two shapes are interesting and they are complementary:
 *   * STRUCT-COPY else + (to.x,to.y)  -- the committed body.  VC6 does NOT CSE
 *     the struct copy's x half with the goal test's load, so `cur->pos.x` dies
 *     at index 28, index 31 reuses eax for `g_route_to.y`, and index 38 is a
 *     reload.  482 instructions, 3 mismatches.
 *   * FIELD else + (cur->pos.x, to.y) -- indices 0..37 match EXACTLY: the CSE
 *     lands in eax, `g_route_to.y` goes to edx, 482 instructions.  What breaks
 *     is only 38..44: VC6 REMATERIALISES `[esi+8]` into edx for the `push`
 *     (after storing to.x straight out of eax) where the original COPIES the
 *     web into ecx once (`mov ecx,eax`) and uses ecx for both the store and
 *     the push.  So the question is now precisely "copy vs rematerialise", not
 *     "which register".
 *   * FIELD else + (to.x, to.y) makes the web three uses long, which flips the
 *     whole allocation to ecx and DELETES the copy -- 481 instructions, i.e.
 *     the original minus `mov ecx,eax`, byte-identical apart from an eax<->ecx
 *     rename in indices 25..43.
 * Measured inert on top of those: all four operand orders of the x/y goal
 * tests, a `Pos* tp = &to;` alias for the stores (VC6 still rematerialises),
 * `int cx = cur->pos.x;` temporaries in four positions (they always produce
 * the 481-instruction ecx form), `--to.y` and `to.y = to.y - 1`, and a
 * de Morgan'd `!(x != .. || y != ..)` goal test. */
/* THIS ROUND (2026-09-03, ~115 more measured variants, residual UNCHANGED at
 * 479/482, indices 31/32/38).  The target was restated from the machine code:
 * the original's else block is `T2 = W1` (mov ecx,eax) with BOTH the to.x
 * store and the push reading T2, and `g_route_to.y` in edx only because W1 is
 * live into that copy.  So the original's IR holds TWO webs joined by a copy
 * that VC6's coalescer/copy-propagator left alone; every C spelling we can
 * write gives either one web (481 instructions, W in ecx) or an independent
 * reload (482, the committed body).  Ruled out, all measured (all give the
 * 481 one-web form unless noted):
 *  - a user variable for the else block's x, single-def or MULTI-def (x
 *    assigned in two or all four neighbour blocks, or on both failure paths
 *    of the goal test phi-style): VC6 builds webs per def-use chain and
 *    propagates every one of them;
 *  - the x/y pair carried in an aggregate (`Pos p`, p.x/p.y assigned then
 *    stored/pushed; `to = p`; `p = cur->pos` -- the last two keep 482 but
 *    reorder the loads), in a static __inline SetPos(&to,x,y) / Step(&to,x,y)
 *    helper (store + RouteInBounds inside), or a MakePos(x,y) helper
 *    returning a Pos by value, in the north block alone or in all four;
 *  - every post-CSE identity on the stored value: `+0`, `-0`, `*1`, `|0`,
 *    `^0`, `&-1`, `>>0`, `<<0`, `/1`, `-(-x)`, `~~x`, `+1-1`, a (dx,dy) step
 *    macro/helper with dx=0, a multi-def `dx` scalar or a `Pos step`
 *    aggregate, casts to int/unsigned/long and a round trip through __int64
 *    -- VC6 folds them all BEFORE CSE;
 *  - a type CONVERSION between the node's pos and the tile: an unsigned or
 *    long `pos` in RouteNode, an unsigned g_route_to, an unsigned `to`: the
 *    field form then stops CSE-ing altogether (482, a reload at 38 exactly
 *    like the struct copy) -- so a same-width conversion is a CSE barrier,
 *    not a copy;
 *  - the goal test restructured: axis first, nested ifs with a goto, de
 *    Morgan, else-as-then, a `found` flag, a `x = cur->pos.x` local used by
 *    the test (with struct-copy, field and push spellings for the else), and
 *    memcmp(&cur->pos,&g_route_to,8) (NOT intrinsic-expanded: a call, 490);
 *  - the neighbour as a separate `Pos nb` local instead of the `to` parameter
 *    (function-level, first- or last-declared, or declared at the top of the
 *    while body): byte-identical to reusing `to` -- VC6 homes it in the dead
 *    parameter pair -- so parameter-vs-local is not the lever; four disjoint
 *    inner-scope nb's move the frame (484 instructions);
 *  - a register-homed neighbour (never address-taken) copied into an
 *    address-taken `nb` right before RouteInBounds / inside the if: 462/457
 *    at best, the copy still does not appear;
 *  - RouteInBounds declared to take the tile BY VALUE (`RouteInBounds(Pos)`,
 *    `RouteInBounds(to)`): byte-identical to the (int,int) prototype in both
 *    the struct-copy (479) and field (481) forms, so the callee's prototype
 *    is not recoverable from this site and is not a lever either.
 * Corpus check: the byte pattern `mov ecx,eax / mov [esp+d8],ecx` occurs
 * ONCE in the whole executable (here).  The four `mov rA,rB / mov [esp+d],rA`
 * sites inside audit-exact functions (UpdateMapDrag 0x452156, bighelp.c
 * 0x4554e4, fpui2.c 0x455937, loadmap.c 0x461ef3) are all a multi-def user
 * variable with a register home assigned from a value that STAYS LIVE
 * afterwards (`maxx = a` where `a` is reused), i.e. the copy survives only
 * under interference.  W1 here has no later use, so the best remaining
 * hypothesis is that the original source kept `cur->pos.x` live past the
 * copy through a use that VC6 later deleted (or a use hidden in code that
 * was #ifdef'd out); no surviving C reaches it.  Semantics are identical. */
/* 2026-09-04, endgame lane.  ~35 more measured variants (all five else-block
 * spellings x both call-argument spellings, named temps in four positions, a
 * volatile push argument, a half-struct copy) -- NOTHING beats 3.  Best of the
 * new shapes: `to.y = cur->pos.y; to.y--; to.x = cur->pos.x;` + (to.x,to.y)
 * gives 7 with the right 1336 bytes but reorders the y block; the field forms
 * still give either 419 (one web in ecx, the whole function's plan flips at
 * index 25) or 138 (three uses: 0..37 EXACT, then a store from eax and a
 * remat for the push).
 *  - THE DECISIVE NEW FACT, from a corpus scan written for exactly this shape
 *    (scratchpad/endgame/scan_copy3.py: `mov rB,rA` where rA was defined by a
 *    memory load, rA is DEAD at the copy, and rB is immediately stored to a
 *    stack home or pushed): the shape occurs in the whole repo exactly TWICE,
 *    and BOTH are unmatched WIPs -- this function (index 38) and
 *    `InsertChildIntoList` (fpui.c 0x475630, index 46, its one missing
 *    instruction).  ZERO of the 1541 audit-exact bodies contain it.  So this
 *    is a FAMILY-WIDE unknown, not a per-function one: whatever source form
 *    makes VC6 leave an un-coalesced copy of a rematerialisable loaded value
 *    will close both functions at once, and neither lane should keep paying
 *    for it alone.  fpui.c's note reaches the same conclusion independently
 *    and names the only near-analogue in the corpus, `RenderAdvisorIcon`
 *    0x443e8a, where the copy is a genuine PHI (`if (!g) g = v;` then
 *    `f(g)`, the push reading the merge register).  Block 0x477c54 here has
 *    exactly ONE predecessor (verified against every branch in the body), so
 *    a phi cannot be the mechanism at this site.
 *  - Restated mechanism, from the register map: at index 27 the goal test's
 *    `cur->pos.x` is in eax and the global in ecx in BOTH bodies; the only
 *    question is whether that web is still live at 38.  If it is not (the
 *    committed struct-copy body) eax is reused for `g_route_to.y` at 31 and
 *    38 is a fresh load -- 3 mismatches.  If it is (any field spelling) VC6
 *    either coalesces the whole thing into ecx (419) or keeps eax and remats
 *    (138).  The original's third possibility -- keep eax, split at the block
 *    EDGE with a copy -- is the one no C spelling reaches.
 *
 * 2026-09-04, lane H.  ~30 more variants, residual UNCHANGED at 3 (indices
 * 31/32/38), and the family-wide unknown now has a PRECISE NECESSARY
 * CONDITION that this site cannot satisfy.
 *  - The 16-combination sweep was re-run from scratch (four else-block
 *    spellings {x-then-y-1, x-then-y-then-dec, y-1-then-x, struct-copy plus a
 *    redundant `to.x = cur->pos.x`} x four argument spellings {(to.x,to.y),
 *    (cur->pos.x,to.y), (to.x,cur->pos.y-1), (cur->pos.x,cur->pos.y-1)}).
 *    Exactly three outcomes, all previously recorded: 3 (the committed struct
 *    copy), 7 (`to.y = cur->pos.y; to.y--; to.x = cur->pos.x;` + (to.x,to.y),
 *    right byte count, y block reordered), 138 (any x-field spelling with the
 *    x argument written `cur->pos.x`: indices 0..37 EXACT, including
 *    `mov edx,[4bb5a4h]` at 31, then a store from eax and a remat for the
 *    push) and 419 (the one-web ecx form).  Nothing else.
 *  - Also new and all worse: making the struct copy DOMINATE the goal test
 *    (`to = cur->pos;` before the test, testing `to.x`/`to.y` or still
 *    `cur->pos.x`/`cur->pos.y`), a `Pos cp` function-level local copied once
 *    and used by both the test and the else, and the same inside the else
 *    only -- 425..443, several with ESCAPES.
 *  - *** WHY RenderAdvisorIcon's COPY EXISTS, read instruction by instruction
 *    off 0x443e59..0x443eaa, because the answer narrows this residual.  The
 *    sequence is
 *        0x443e80  mov ecx,[665f60h]   ; ecx = g_vid_next
 *        0x443e86  cmp ecx,edi         ; edi is the function's zero
 *        0x443e88  jne 0x443e92
 *        0x443e8a  mov ecx,eax         ; ecx = g_vidanim   <-- THE COPY
 *        0x443e8c  mov [665f60h],ecx
 *        0x443e92  push ecx            ; SetVidAnim(g_vid_next)
 *    Its source eax is NOT live afterwards (it is redefined by a reload at
 *    0x443ea2), so interference is NOT the reason.  The reason is that ecx is
 *    a PHI REGISTER: the pushed value has TWO reaching defs -- the load at
 *    0x443e80 on the fall-through edge and g_vidanim on the taken edge -- and
 *    a phi has to live in ONE register, so the taken edge must materialise
 *    into it.  The store then reads the phi register too.  So the corpus
 *    contains exactly two mechanisms that keep such a copy: a PHI
 *    (RenderAdvisorIcon) and INTERFERENCE (the four `mov rA,rB /
 *    mov [esp+d],rA` sites the earlier scan found, where the source stays
 *    live).  THIS SITE HAS NEITHER.  Block 0x477c54 has one predecessor
 *    (re-verified), so the to.x value has a single reaching def; and
 *    `cur->pos.x` has no use after index 38 -- the later reads at 0x477caa,
 *    0x477cf8 and 0x477d42 are all fresh loads behind calls, so no web spans
 *    them.  With neither a phi nor interference there is nothing for VC6 to
 *    refuse to coalesce, which is exactly why every field spelling either
 *    coalesces (419) or remats (138).  Unless the original's translation unit
 *    had a second reaching def or a later use of that value which was
 *    afterwards deleted, this copy is not reachable from C. ***
 *    Apply the same two-way test to `InsertChildIntoList` (fpui.c 0x475630,
 *    index 46) before assuming the two are one problem: if ITS copy sits at a
 *    join, or its source survives, that one is reachable and this one is
 *    not.
 *
 * 2026-09-04 (fifth lane).  Unchanged at 3.  The cross-check this note asks
 * for HAS NOW BEEN RUN on `InsertChildIntoList` (fpui.c 0x475630) by a lane
 * that owns both files, and it comes out the SAME WAY: its 0x4756aa is not a
 * branch target (every target in that body was enumerated), so its block has
 * one predecessor and there is no phi; and its eax is defined at 0x47569d,
 * read twice, then redefined by the call, with nothing else competing, so
 * there is no interference either.  Additionally, the strongest untried
 * member of the "make the pushed value a temporary" family -- a
 * `static __inline` wrapper round the call, which turns the argument into an
 * inline-expansion temporary -- was measured there and is inert.
 * CONSEQUENCE FOR THIS FUNCTION: the two sites are NOT one shared unknown
 * waiting on a single lever; they are two independent instances of the same
 * NEGATIVE result (neither mechanism present).  Nobody should keep paying for
 * either on the theory that the other will pay for it.  RETIREMENT ENDORSED
 * at 479 of 482 instructions, 1336/1336 bytes, three register-allocation
 * instructions at indices 31/32/38.
 *
 * 2026-09-05, lane w11a.  Unchanged at 3, but the impossibility argument is
 * now MECHANISTIC rather than enumerative -- two rules were measured that
 * between them account for every outcome any spelling can reach, so the
 * search space is closed rather than merely explored.
 *  1. ALIAS KILL.  `to` is address-taken (`&to` reaches GetRouteNode), so a
 *     store to ANY field of `to` kills the CSE availability of `cur->pos.x`.
 *     Measured twice: (a) `to.y = cur->pos.y; to.y--; to.x = cur->pos.x;`
 *     with (to.x,to.y) -- the x read after the y store is a FRESH
 *     `mov ecx,[esi+8]` (index 43) and the goal test's web is not extended
 *     (7 mismatches, 1336 bytes); (b) a redundant THIRD occurrence
 *     `to.x = cur->pos.x; to.y = cur->pos.y - 1; to.x = cur->pos.x;` -- 483
 *     instructions, the first two occurrences merge into the eax web (index
 *     38 becomes `mov [esp+1ch],eax`) and the third is a fresh load into ecx.
 *     So the else block's x can only ever be (i) the SAME web as the goal
 *     test, when no store to `to` precedes it, or (ii) a fresh load, when one
 *     does.  A register-to-register copy is neither; there is no third regime
 *     for VC6 to land in.
 *  2. THE REGISTER PAIR FOLLOWS THE WEB LENGTH, not the source.  Whenever the
 *     else block's x is NOT in the goal test's web, indices 25/27/28 come out
 *     as the original has them (`mov ecx,[4bb5a0h]` / `mov eax,[esi+8]` /
 *     `cmp eax,ecx`) and the y compare's global then takes the freed eax --
 *     the committed body's 3.  Whenever it IS in the web (every field
 *     spelling), the whole plan flips to g_route_to.x in eax and cur->pos.x
 *     in ecx, and only THEN does index 31 correctly become
 *     `mov edx,[4bb5a4h]`.  The original needs the short-web pair at 25..28
 *     AND a value live at 30..38 simultaneously; measured across five
 *     spellings, VC6 gives eax to the short web and ecx to the long one every
 *     time, so that combination is unreachable by construction.
 * Newly measured this round and all reproducing one of the two regimes above:
 * `ny = cur->pos.y - 1;` hoisted above the x store with (to.x,ny) [404] and
 * with (cur->pos.x,ny) [10, the eax<->ecx flip plus a reordered to.y store];
 * `nx = cur->pos.x;` before the struct copy [405]; the struct copy followed
 * by `nx = cur->pos.x; ny = to.y;` [405]; `to.x = cur->pos.x;` with the call
 * folded into the y store [403]; and a `static __inline PosEq(&cur->pos,
 * &g_route_to)` goal test in three else spellings [9/9/410 -- the inline
 * costs an instruction].  Nothing beats 3.  FLOOR RE-ENDORSED.
 *
 * 2026-09-10, scope LL21.  Still 3, committed body unchanged -- but PASS
 * w11a's rule 1 is now FALSIFIED in its strong form, and the replacement rule
 * says exactly what is missing.
 *  - *** THERE IS A THIRD REGIME. ***  w11a concluded that the else block's x
 *    can only be the goal test's own web or a fresh load, "a register-to-
 *    register copy is neither; there is no third regime for VC6 to land in".
 *    A scope-V CANCELLED PAIR is that third regime.  Written on the plain
 *    field spelling that w11a measured at 138 (207 as re-measured here):
 *        struct { int ox, oy; } t;   ...
 *        t.oy = (int)cur;            -- or best, &g_route_to, &g_open_head
 *        t.ox = cur->pos.x;
 *        t.ox += t.oy;  t.ox -= t.oy;
 *        to.x = t.ox;   to.y = cur->pos.y - 1;
 *    the body comes out with indices 0..37 AND 39..480 exact -- the whole
 *    138/207 cascade disappears, INCLUDING the `mov edx,[4bb5a4h]` at 31 and
 *    the `cmp` at 32 that are two of this body's three mismatches.  The pair
 *    costs nothing: the anchor is dead and is DCE'd, and the add/sub cancel at
 *    instruction selection.  Sixty-one anchor x placement combinations were
 *    measured; any pointer or link-time address constant reaches it, and it is
 *    robust (chained pairs, unsigned or char* members, a redundant `*1` or
 *    `|0`, an extra copy member, and feeding the carrier to RouteInBounds all
 *    reproduce it exactly).
 *  - WHAT IT STILL DOES NOT DO is emit the copy.  VC6 COALESCES the carrier's
 *    web with the goal test's, so `to.x` is stored straight out of eax and the
 *    body is 481 instructions: the original's `mov ecx,eax` at index 38 is the
 *    single missing instruction, and eax/ecx are then swapped for the two
 *    RouteInBounds arguments.  So the rule should read: a cancelled pair DOES
 *    give the else block's x a web of its own, but a web whose source dies at
 *    its definition is coalesced away, and the copy still needs the
 *    interference or the phi this site does not have.  Making y the anchor (to
 *    bump it into eax) does not work either -- the anchor is then a real load,
 *    costs its own instruction, and lands exactly where the copy should be.
 *  - Two mechanical facts measured on the way, worth reusing elsewhere:
 *    (a) the fold is ORDER-SENSITIVE -- `+=` then `-=` folds, `-=` then `+=`
 *    does NOT when the anchor is a link-time address constant (it emits
 *    `sub eax,K` / `lea ecx,[eax+K]`, i.e. the copy plus a displacement);
 *    (b) ANY statement between the cancel and the carrier's use un-folds the
 *    pair or collapses the body back to the 207 regime -- writing `to.y`
 *    first, re-introducing the struct copy, or declaring a y temp ahead of it
 *    are all 207..212.
 *  FLOOR RE-ENDORSED at 479 of 482 instructions, 1336/1336 bytes, three
 *  register-allocation instructions at indices 31/32/38.
 */
// WIP-FUNCTION: LEGOLAND 0x00477bd0  (99.4%, 3 register-allocation instructions at idx 31/32/38 -- see above)
void RequestRoute(Pos from, Pos to)
{
    int        state;
    RouteNode* cur;
    RouteNode* nn;
    RouteNode* best;
    int        g;
    Elem*      elem;
    RouteNode* node;
    Cell*      cell;

    g_route_from = from;
    best = 0;
    g_route_to = to;
    nn = GetRouteNode(&g_route_from, &state);
    nn->g = 0;
    AddOpenNode(nn);
    cur = g_open_head;
    while (cur) {
        g_open_head = cur->next;
        if ((cur->pos.x == g_route_to.x && cur->pos.y == g_route_to.y) || cur->axis == 1) {
            best = cur;
        } else {
            to = cur->pos;
            to.y--;
            if (RouteInBounds(to.x, to.y)) {
                nn = GetRouteNode(&to, &state);
                if (nn->cost != -1) {
                    if (nn->axis != 0 && nn->axis != 1)
                        g = RouteTurnCost(cur->dir,
                                RouteStepAxis(cur->pos.x, cur->pos.y, nn->pos.x, nn->pos.y))
                            + cur->g + nn->cost;
                    else
                        g = cur->g + nn->cost;
                    if (state == 0) {
                        if (best != 0 && g > best->g) {
                            AddClosedNode(nn);
                        } else {
                            nn->dir = RouteStepAxis(cur->pos.x, cur->pos.y, nn->pos.x, nn->pos.y);
                            nn->parent = cur;
                            nn->g = g;
                            nn->f = nn->h + g;
                            if (state == 2)
                                RemoveClosedNode(nn);
                            if (state == 1)
                                RemoveOpenNode(nn);
                            AddOpenNode(nn);
                        }
                    }
                }
            }
            to.x = cur->pos.x + 1;
            to.y = cur->pos.y;
            if (RouteInBounds(to.x, to.y)) {
                nn = GetRouteNode(&to, &state);
                nn->dir = RouteStepAxis(cur->pos.x, cur->pos.y, nn->pos.x, nn->pos.y);
                if (nn->cost != -1) {
                    if (nn->axis != 0 && nn->axis != 1)
                        g = RouteTurnCost(cur->dir,
                                RouteStepAxis(cur->pos.x, cur->pos.y, nn->pos.x, nn->pos.y))
                            + cur->g + nn->cost;
                    else
                        g = cur->g + nn->cost;
                    if (state == 0) {
                        if (best != 0 && g > best->g) {
                            AddClosedNode(nn);
                        } else {
                            nn->dir = RouteStepAxis(cur->pos.x, cur->pos.y, nn->pos.x, nn->pos.y);
                            nn->parent = cur;
                            nn->g = g;
                            nn->f = nn->h + g;
                            if (state == 2)
                                RemoveClosedNode(nn);
                            if (state == 1)
                                RemoveOpenNode(nn);
                            AddOpenNode(nn);
                        }
                    }
                }
            }
            to.x = cur->pos.x;
            to.y = cur->pos.y + 1;
            if (RouteInBounds(to.x, to.y)) {
                nn = GetRouteNode(&to, &state);
                nn->dir = RouteStepAxis(cur->pos.x, cur->pos.y, nn->pos.x, nn->pos.y);
                if (nn->cost != -1) {
                    if (nn->axis != 0 && nn->axis != 1)
                        g = RouteTurnCost(cur->dir,
                                RouteStepAxis(cur->pos.x, cur->pos.y, nn->pos.x, nn->pos.y))
                            + cur->g + nn->cost;
                    else
                        g = cur->g + nn->cost;
                    if (state == 0) {
                        if (best != 0 && g > best->g) {
                            AddClosedNode(nn);
                        } else {
                            nn->dir = RouteStepAxis(cur->pos.x, cur->pos.y, nn->pos.x, nn->pos.y);
                            nn->parent = cur;
                            nn->g = g;
                            nn->f = nn->h + g;
                            if (state == 2)
                                RemoveClosedNode(nn);
                            if (state == 1)
                                RemoveOpenNode(nn);
                            AddOpenNode(nn);
                        }
                    }
                }
            }
            to.x = cur->pos.x - 1;
            to.y = cur->pos.y;
            if (RouteInBounds(to.x, to.y)) {
                nn = GetRouteNode(&to, &state);
                nn->dir = RouteStepAxis(cur->pos.x, cur->pos.y, nn->pos.x, nn->pos.y);
                if (nn->cost != -1) {
                    if (nn->axis != 0 && nn->axis != 1)
                        g = RouteTurnCost(cur->dir,
                                RouteStepAxis(cur->pos.x, cur->pos.y, nn->pos.x, nn->pos.y))
                            + cur->g + nn->cost;
                    else
                        g = cur->g + nn->cost;
                    if (state == 0) {
                        if (best != 0 && g > best->g) {
                            AddClosedNode(nn);
                        } else {
                            nn->dir = RouteStepAxis(cur->pos.x, cur->pos.y, nn->pos.x, nn->pos.y);
                            nn->parent = cur;
                            nn->g = g;
                            nn->f = nn->h + g;
                            if (state == 2)
                                RemoveClosedNode(nn);
                            if (state == 1)
                                RemoveOpenNode(nn);
                            AddOpenNode(nn);
                        }
                    }
                }
            }
        }
        AddClosedNode(cur);
        cur = g_open_head;
    }
    node = best;
    elem = ElemID("PATH CONTROL");
    while (node) {
        cell = RouteCellAt(&node->pos);
        if (!(cell->flags & 0x10) && (cell->rf & 3) != 3) {
            ClearCellForPath(&node->pos);
            AddBasicPath(elem, &node->pos);
        }
        node = node->parent;
    }
    while (g_closed_head)
        RemoveClosedNode(g_closed_head);
}

/* ---- IsAdjacentPos ------------------------------------------------------ */

/* Are two map tiles orthogonally adjacent?  Manhattan distance exactly 1, so
 * the same tile and the diagonals both answer 0.  ScanBlokeSurroundings uses
 * it to decide whether a bloke (and the cell it is scanning) touch an
 * attraction's access tile.  Not exported; sits directly in front of
 * ScanBlokeSurroundings. */
/* CLOSED (24/24, 45 bytes) -- the lever is an AGGREGATE local for the two
 * abs() results.  Every scalar spelling of `abs(a->x-b->x) + abs(a->y-b->y)`
 * (some sixty were measured across four rounds: both source orders, dx/dy
 * temporaries, accumulate forms, `1 ==`, `-1 == 0`, `!(..-1)`, unsigned,
 * char/short returns, const/volatile parameters -- even `volatile Pos* a` --
 * int* parameters, distinct struct types for a and b, mixed a-b / b-a signs,
 * static __inline helpers, and a struct copy) comes out Y-TERM FIRST with the
 * sum accumulated into the first term's register and a closing `mov eax,edi`.
 * VC6 forward-substitutes every scalar temp into the `+`, and then sorts the
 * two structurally identical operands by the displacement of their loads
 * (+4 beats +0; separate int PARAMETERS all tie and keep source order, a
 * `Pos` BY VALUE sorts +4 first too).  Storing the two abs() values into the
 * fields of a `Pos` local stops the forward substitution: VC6 evaluates the
 * two statements in SOURCE order (x first, into ecx), builds the sum from the
 * field temps with the SECOND one as the add's destination (`add eax,ecx`,
 * result already in eax), and the freed eax/ecx/edx let both pointers sit in
 * esi/edi with the pops interleaved exactly as the original has them.  An
 * `int d[2]` array or an address-taken `int t` (`*pt = abs(..)`) does the
 * same; a `Pos d` holding the raw DIFFERENCES (abs applied at the sum) does
 * not.  Also learned: `volatile` on the loads does NOT pin their evaluation
 * order against the non-volatile ones, and the "add dest = second operand"
 * shape is the signature of an operand that was NOT forward-substituted. */
// FUNCTION: LEGOLAND 0x00450500
int IsAdjacentPos(Pos* a, Pos* b)
{
    Pos d;

    d.x = abs(a->x - b->x);
    d.y = abs(a->y - b->y);
    return d.x + d.y == 1;
}
