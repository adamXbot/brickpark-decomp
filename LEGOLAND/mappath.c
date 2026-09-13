/* LEGOLAND -- the map/path cluster: path-square neighbour collection, walk
 * paths built from polylines, per-cell teardown when a path is laid, and the
 * cursor-driven stamping of a placed object into the map.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Struct field OFFSETS and global addresses are load-bearing; names are ours.
 * Types are declared LOCALLY (legoland.h is owned elsewhere); the PathSquare,
 * Cell, ObjDef, MapObj and Cursor shapes are taken verbatim from pathsq.c and
 * objmap2.c so the two files agree.
 *
 * ==========================================================================
 * 1. PATH SQUARES AND THEIR NEIGHBOURS  (CollectPathSquareNeighbours)
 * ==========================================================================
 * A "path square" is a maximal axis-aligned rectangle of walkable path cells.
 * They live on one singly linked list (head 0x0066b44c) and each carries its
 * inclusive Rect at +0x08; FindPathSquare (0x00481790) is a linear scan of
 * that list for the square containing a cell.
 *
 * CollectPathSquareNeighbours takes a rectangle and fills the global array at
 * 0x0066a45c with the DISTINCT path squares that touch it, in FOUR groups,
 * each terminated by a NULL slot:
 *
 *     group 0   the row ABOVE   (y = top-1,    x = left  .. right)
 *     group 1   the row BELOW   (y = bottom+1, x = left  .. right)
 *     group 2   the column LEFT (x = left-1,   y = top   .. bottom)
 *     group 3   the column RIGHT(x = right+1,  y = top   .. bottom)
 *
 * The corners are deliberately NOT probed: the row scans run left..right and
 * the column scans top..bottom, i.e. exactly the rectangle's own extent.
 * Duplicates are avoided without a set: when a probe lands inside a square,
 * the scan jumps the cursor to that square's far edge (rect.right for a row
 * scan, rect.bottom for a column scan) before the loop's own ++, so a square
 * spanning many cells of the edge is recorded once.  A miss still stores the
 * NULL into the array slot -- it is simply overwritten by the next probe --
 * so the array never holds stale pointers from a previous call.
 * 0x00669254 counts the squares found across all four groups (the array index
 * additionally counts the four NULL terminators, so the two differ).
 *
 * PathSquareAdded (0x00481b10, pathsq.c) is the consumer: it walks the four
 * groups in order and merges the new square into the first neighbour that
 * shares its span, which is why the group ORDER is part of the interface.
 *
 * ==========================================================================
 * CALLEES THIS FILE NAMES FOR THE FIRST TIME
 * ==========================================================================
 *   0x00450c00  FreeBuildSlotAt(BPos)          linear scan of the 256-entry
 *               "under construction" table at 0x006664f8 (stride 12) for the
 *               slot whose u16 key at +0x04 is this cell; on a hit it zeroes
 *               the slot's object pointer and decrements the live-build count
 *               at 0x006670f8.  Same table buildtick.c and popup.c describe.
 *   0x0045d3d0  RemoveObjectPathTiles(ObjDef*, Pos*)  for a class whose type
 *               is neither 0 nor 2, walks the class rect (+0x3c) GROWN BY ONE
 *               CELL on every side, biased by the position, and for each cell
 *               clears map flags 0x18, zeroes rf, restores tile from base and
 *               runs UpdatePathNeighbours + RemovePathSquare.  It is the path
 *               teardown a work order's target needs when the order is
 *               cancelled.
 *
 * ==========================================================================
 * VC6 LEVERS THIS FILE ADDED
 * ==========================================================================
 *  - A FREE `volatile` READ BELONGS AT THE DEFINITION SITE, NOT THE USE SITE.
 *    In ClearCellForPath the original does not share the probe's `pos->x` load
 *    with the two later reads of the same field; forcing that with a volatile
 *    read at the later USE reproduces the reload but leaves it scheduled after
 *    the `push` of the other argument (166/167), because VC6 will not hoist a
 *    volatile access across a store.  The same barrier written on the probe's
 *    own read -- where the original loads the field anyway, so it costs
 *    nothing -- is exact (167/167).  The knock-on is large: with the CSE alive
 *    VC6 pins pos->x in eax across the whole flag dispatch, which then pushes
 *    the u16 cell flags into cx and the row-table load into esi.
 *  - AN AGGREGATE LOCAL TAKES THE TOP OF THE FRAME, AND THAT IS A LEVER FOR
 *    PLAIN SCALARS TOO.  BuildWalkPath's running segment origin is two ints as
 *    far as the machine code shows, but as two `int` locals VC6 gives them the
 *    two LOWEST frame slots where the original has them at the two HIGHEST;
 *    declaring them as one `Pos` (whose address is never taken, so it still
 *    scalarises) moves the pair and closes 28 of 138.  Declaration order is
 *    inert here, as the playbook says -- the aggregate-ness is what moves it.
 *  - A PER-ITERATION UPDATE OF TWO STRUCT FIELDS CAN BE A WHOLE-STRUCT
 *    ASSIGNMENT.  `cur.x += dx; cur.y += dy;` loads cur.x into a fresh
 *    callee-saved register and accumulates into it; `t.x = dx + cur.x;
 *    t.y = dy + cur.y; cur = t;` consumes both loads into dx's and dy's own
 *    registers and writes the pair back as one copy, which is what the
 *    original does (worth 9).  Every operand order and `+=`/`=` spelling of
 *    the first form is byte-identical, so this is not reachable by permuting
 *    the sum -- the shape has to change.
 *  - AN ALLOCATION SIZE SPELLED TWICE IS NOT THE SAME AS A `size` LOCAL.  The
 *    CSE web VC6 builds for the duplicated expression lands in eax; routed
 *    through a named local the `lea` comes out on edx instead.  Two bytes, but
 *    it was the last two in BuildWalkPath.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* One walkable rectangle of path (pathsq.c; 0x24 bytes, HeapAlloc_w'd). */
typedef struct PathSquare {
    struct PathSquare* next;      /* +0x00 */
    int                pad4;      /* +0x04 */
    Rect               rect;      /* +0x08 inclusive, .next unused here */
    int                distance2; /* +0x1c */
    int                flags;     /* +0x20 */
} PathSquare;

/* ---------------------------------------------------------------- globals -- */

/* The neighbour scratch list: four NULL-terminated groups, and the count of
 * the squares (not the terminators) the last collection found. */
extern PathSquare* g_path_square_neighbours[]; /* 0x0066a45c */
extern int         g_path_square_neighbour_count; /* 0x00669254 */

/* ---------------------------------------------------------------- callees -- */

extern PathSquare* FindPathSquare(Pos* pos);   /* 0x00481790 */

/* -------------------------------------------------------------- functions -- */

/* Fill g_path_square_neighbours with the path squares bordering `bounds`:
 * above, below, left, right, each group NULL-terminated. */
// FUNCTION: LEGOLAND 0x00481810
void CollectPathSquareNeighbours(Rect* bounds)
{
    PathSquare* square;
    Pos         pos;
    int         n = 0;

    g_path_square_neighbour_count = 0;

    pos.y = bounds->top - 1;
    for (pos.x = bounds->left; pos.x <= bounds->right; pos.x++) {
        square = FindPathSquare(&pos);
        g_path_square_neighbours[n] = square;
        if (square != 0) {
            pos.x = square->rect.right;
            n++;
            g_path_square_neighbour_count++;
        }
    }
    g_path_square_neighbours[n++] = 0;

    pos.y = bounds->bottom + 1;
    for (pos.x = bounds->left; pos.x <= bounds->right; pos.x++) {
        square = FindPathSquare(&pos);
        g_path_square_neighbours[n] = square;
        if (square != 0) {
            pos.x = square->rect.right;
            n++;
            g_path_square_neighbour_count++;
        }
    }
    g_path_square_neighbours[n++] = 0;

    pos.x = bounds->left - 1;
    for (pos.y = bounds->top; pos.y <= bounds->bottom; pos.y++) {
        square = FindPathSquare(&pos);
        g_path_square_neighbours[n] = square;
        if (square != 0) {
            pos.y = square->rect.bottom;
            n++;
            g_path_square_neighbour_count++;
        }
    }
    g_path_square_neighbours[n++] = 0;

    pos.x = bounds->right + 1;
    for (pos.y = bounds->top; pos.y <= bounds->bottom; pos.y++) {
        square = FindPathSquare(&pos);
        g_path_square_neighbours[n] = square;
        if (square != 0) {
            pos.y = square->rect.bottom;
            n++;
            g_path_square_neighbour_count++;
        }
    }
    g_path_square_neighbours[n++] = 0;
}

/* ==========================================================================
 * 2. WALK PATHS  (BuildWalkPath)
 * ==========================================================================
 * A walk path is the pre-expanded, one-node-per-tile-step route a bloke walks
 * on a scripted ride (GOLD RUSH, the LOG FLUME animation set and the five
 * MECHANICAL COPTERS arms all build one at create time and free it with
 * 0x00412290).  The input is a POLYLINE descriptor {int count; Pos* pts;} of
 * relative tile deltas held in .rdata -- GOLD RUSH's is the five-segment
 * {-2,0},{0,-6},{-6,0},{0,1},{3,0} at 0x004b45e0.
 *
 * The build is two passes over the segment list:
 *   pass 1  total = sum over segments of (int)sqrt(dy*dy + dx*dx), the
 *           truncated Euclidean length of each delta in whole tiles;
 *   pass 2  allocate 8 + 12*total bytes, zero them, and lay the header
 *           {count = total; nodes = path + 8} over the front so the node
 *           array is contiguous with its own header.  Each segment then
 *           writes `len` nodes, linearly interpolating from the running
 *           segment origin (cx, cy) with an integer accumulator: the k-th
 *           node of a segment is (cx + k*dx/len, cy + k*dy/len).
 *
 * TWO ORIGINAL BEHAVIOURS ARE REPRODUCED HERE, NOT INVENTED:
 *
 *  (a) The interpolation is written as "store the current point, then compute
 *      the next one, then advance the accumulator" -- so the accumulator that
 *      feeds the division is always one step BEHIND.  The first two nodes of
 *      every segment are therefore identical (both the segment origin) and the
 *      last node of a segment stops at (len-2)/len of the way along it rather
 *      than at (len-1)/len; the segment end itself is never emitted, it only
 *      becomes the next segment's origin.  This is visible in the machine code
 *      as `idiv` reading the accumulator slot strictly before `add edx, edi`
 *      writes it back, and there is no spelling of the "correct" order that
 *      produces it.
 *
 *  (b) There is NO null check around the node stores.  When HeapAlloc_w fails,
 *      only the header initialisation is skipped -- the interpolation loop
 *      still runs and dereferences the null path.  The function returns the
 *      null pointer in that case (the second epilogue, reached when the
 *      polyline is empty, returns the same pointer out of edx).
 *
 * Node stride is 12 bytes: {int x; int y;} plus a third dword the builder
 * leaves zero and the walker (0x00412300) fills in.
 * ------------------------------------------------------------------------- */

/* The polyline descriptor (goldrush.c calls this shape PolyLine). */
typedef struct PolyLine {
    int  count;                  /* +0x00 number of segment deltas */
    Pos* pts;                    /* +0x04 the deltas, in map tiles */
} PolyLine;

/* One expanded step of a walk path. */
typedef struct WalkNode {
    int x;                       /* +0x00 */
    int y;                       /* +0x04 */
    int state;                   /* +0x08 left zero by the builder */
} WalkNode;

/* The walk path header; its nodes follow it in the same allocation. */
typedef struct WalkPath {
    int       count;             /* +0x00 total node count */
    WalkNode* nodes;             /* +0x04 == (WalkNode*)(path + 1) */
} WalkPath;

extern void* HeapAlloc_w(unsigned int size);   /* 0x0049e4ff */

double sqrt(double);
void*  memset(void*, int, unsigned int);
#pragma intrinsic(sqrt, memset)

/* Expand `poly` into a per-tile-step node array.
 *
 * Codegen notes: the running segment origin must be an aggregate (`Pos cur`)
 * -- as two plain ints it takes the BOTTOM two frame slots and the original
 * puts it at the TOP, which alone is worth 28 of 138.  The per-segment origin
 * advance is a whole-struct assignment through `t`: with `cur.x += dx` VC6
 * loads cur.x into a fresh callee-saved register and accumulates into that,
 * where the original consumes the loads into dx's and dy's own registers and
 * writes both fields back as one copy (worth 9).  The first pass is written
 * as an explicit guard + do/while over a pointer, which is what puts `total`
 * in ebx and `poly` in ebp; the equivalent counted `for` swaps that pair.
 * The allocation size is spelled TWICE (no `size` local): the CSE web that
 * creates lands in eax, where a named local leaves the `lea` on edx. */
// FUNCTION: LEGOLAND 0x00412100
WalkPath* BuildWalkPath(PolyLine* poly)
{
    int       ax, ay, i, node, total, j, dx, dy, len, px, py;
    WalkPath* path;
    Pos       cur;
    Pos       t;
    Pos*      p;

    /* Pass 1: how many nodes the whole polyline needs. */
    total = 0;
    i = poly->count;
    if (i > 0) {
        p = poly->pts;
        do {
            dx = p->x;
            dy = p->y;
#ifndef LEGOLAND_PORTABLE
            total += (int)sqrt((double)(dy * dy + dx * dx));
#else
            total += LL_FISTPD(sqrt((double)(dy * dy + dx * dx))); /* PORT-M5 */
#endif
            p++;
        } while (--i);
    }

    path = (WalkPath*)HeapAlloc_w(sizeof(WalkPath) + total * sizeof(WalkNode));
    if (path != 0) {
        memset(path, 0, sizeof(WalkPath) + total * sizeof(WalkNode));
        path->count = total;
        path->nodes = (WalkNode*)(path + 1);
    }
    /* No else: an allocation failure falls straight into the loop below and
     * dereferences the null `path`.  Original behaviour, reproduced. */

    /* Pass 2: interpolate each segment into `len` nodes. */
    cur.x = 0;
    cur.y = 0;
    node = 0;
    for (i = 0; i < poly->count; i++) {
        dx = poly->pts[i].x;
        dy = poly->pts[i].y;
#ifndef LEGOLAND_PORTABLE
        len = (int)sqrt((double)(dy * dy + dx * dx));
#else
        len = LL_FISTPD(sqrt((double)(dy * dy + dx * dx))); /* PORT-M5 */
#endif
        px = cur.x;
        py = cur.y;
        if (len > 0) {
            ay = 0;
            ax = 0;
            for (j = 0; j < len; j++) {
                path->nodes[node].x = px;
                path->nodes[node].y = py;
                /* The accumulator is advanced AFTER the next point is
                 * computed from it, so the point written on the next pass
                 * repeats this one: nodes 0 and 1 of every segment are both
                 * the segment origin.  Original off-by-one, reproduced. */
                px = cur.x + ax / len;
                py = cur.y + ay / len;
                ax += dx;
                ay += dy;
                node++;
            }
        }
        t.x = dx + cur.x;
        t.y = dy + cur.y;
        cur = t;
    }
    return path;
}

/* ==========================================================================
 * 3. CLEARING A CELL SO A PATH CAN BE LAID  (ClearCellForPath)
 * ==========================================================================
 * Called for every tile a new path is about to occupy -- by the router
 * (RequestRoute, simcore.c) for each tile of the chosen route, and by the
 * path-drag builder (workorder2.c) for every tile of every dragged rect.
 * It is the one place that decides what "build a path here" does to whatever
 * is already on the tile, and it dispatches on the cell's map flags:
 *
 *   0x40  blocked                  -> nothing at all
 *   0xa0  a placed object's cell   -> remove the object (see below)
 *   0x800 needs a mechanic         -> cancel the outstanding work order
 *   0x08  an object/path cell      -> drop the path square and clear rf bit 0
 *
 * THE OBJECT CASE saves BOTH the destroy cursor (all 0x1834 bytes of it) and
 * the "class under the destroy cursor" global around the removal, because the
 * removal path runs the class's own +0x94 handler and then either the normal
 * RemObjFromMap -- which reads the destroy cursor to decide which path tiles
 * to tear down -- or, for a class carrying flag 0x200000 ("may be built
 * over") on a cell already reserved for building (flag 0x20), the shorter
 * "cancel the construction" path: free the build slot, clear the object's
 * user flags, put the object back in the class's available count and unmap
 * it.  Both are run with g_sel_def pointing at the class being removed, and
 * both leave the caller's destroy cursor exactly as they found it.
 *
 * THE WORK-ORDER CASE tries the gardener queue first and the mechanic queue
 * only if no gardener order covers the tile; each is torn down the same way
 * (clear the target's user flags, strip the path tiles around the target's
 * footprint, erase the order).
 *
 * ORIGINAL BUG, reproduced: the cell pointer is NOT null-checked.  The
 * bounds-checked lookup yields 0 for an off-map tile and the very next
 * instruction reads flags at +0x0c through it.  Both callers only ever pass
 * in-map tiles, so it cannot fire in practice.
 * ------------------------------------------------------------------------- */

/* A placed object's map square, packed as two bytes and passed BY VALUE. */
typedef struct BPos {
    unsigned char x;             /* +0x00 */
    unsigned char y;             /* +0x01 */
} BPos;

/* An object class / definition (objmap2.c's ObjDef, the fields used here). */
typedef struct ObjDef {
    char          pad0[0x1c];    /* +0x00 */
    unsigned int  flags;         /* +0x1c  0x200000 = may be built over,
                                  *        0x800000 = "tall" cell */
    char          pad20[0x90 - 0x20];
    void        (*effect)(void* ctx, Pos* pt, int effect); /* +0x90 */
    void        (*deselect)(void* obj, Pos* pos);          /* +0x94 */
    char          pad98[0xc4 - 0x98];
    void*         ctx;           /* +0xc4  the class's LLIDB element */
    char          padc8[0xd0 - 0xc8];
} ObjDef;

/* A placed map object: its class sits at +0x0c. */
typedef struct MapObj {
    char    pad0[0x0c];          /* +0x00 */
    ObjDef* cls;                 /* +0x0c */
} MapObj;

/* An edit / destroy cursor block (objmap2.c; 0x1834 bytes). */
typedef struct Cursor {
    unsigned short count;        /* +0x0000 */
    short          px[0x400];    /* +0x0002 */
    short          py[0x400];    /* +0x0802 */
    unsigned char  kind[0x400];  /* +0x1002 */
    unsigned char  pad1402[2];
    Pos            origin;       /* +0x1404 */
    int            status;       /* +0x140c */
    int            error;        /* +0x1410 */
    Rect           rect;         /* +0x1414 */
    unsigned char  style;        /* +0x1428 */
    char           pad1429[0x1828 - 0x1429];
    unsigned int   flags;        /* +0x1828 */
    int            f182c;        /* +0x182c */
    struct Cursor* next;         /* +0x1830 */
} Cursor;

/* A gardener or mechanic work order (workorder2.c; the fields used here). */
typedef struct WorkOrder {
    struct WorkOrder* next;      /* +0x00 */
    MapObj*           obj;       /* +0x04 */
    Pos               pos;       /* +0x08 */
} WorkOrder;

extern ObjDef* g_sel_def;        /* 0x00667c58 class under the destroy cursor */
extern Cursor  g_destroy_cursor; /* 0x00810160 */

#ifndef LEGOLAND_PORTABLE
extern void       FreeBuildSlotAt(BPos sq);                     /* 0x00450c00 */
#else
/* PORT-M10: pathmisc.c:99 DEFINES 0x00450c00 as `(unsigned short key)`.  On
 * x86 cdecl that is the same dword as this two-byte by-value BPos; on wasm32
 * clang passes a multi-member struct INDIRECTLY and a scalar DIRECTLY, and
 * both still lower to ONE i32 parameter, so wasm-ld reports no signature
 * mismatch and nothing traps -- the scan compares a shadow-stack ADDRESS
 * against every slot key and silently frees nothing (the slot leaks and
 * g_build_count is never decremented).  Same class as popup.c's
 * AddObjectToBuildList; see docs/lanes/scope-port-m10.md §1. */
extern void       FreeBuildSlotAt(unsigned short sq);           /* 0x00450c00 */
#define FreeBuildSlotAt(_sq) \
    FreeBuildSlotAt((unsigned short)((_sq).x | ((_sq).y << 8)))
#endif
extern void       ClearObjectUserFlags(MapObj* obj, Pos* pos);  /* 0x0045e850 */
extern void       IncrementObjectCount(ObjDef* def);            /* 0x00480d40 */
extern void       RemoveObjectFromMap(BPos sq);                 /* 0x0045f100 */
extern void       RemObjFromMap(ObjDef* d, MapObj* o, BPos sq, void* c); /* 0x00459c90 */
extern WorkOrder* GetGardenerWorkOrderAt(int x, int y);         /* 0x0049b130 */
extern void       EraseGardenerOrder(WorkOrder* o);             /* 0x0049b230 */
extern WorkOrder* GetMechanicWorkOrderAt(int x, int y);         /* 0x0049b180 */
extern void       EraseMechanicOrder(WorkOrder* o);             /* 0x0049b1d0 */
extern void       RemoveObjectPathTiles(ObjDef* def, Pos* pos); /* 0x0045d3d0 */
extern void       RemovePathSquare(Pos* pos);                   /* 0x00481c90 */

/* The bounds-checked cell fetch every map accessor open-codes, reading the
 * coordinates lazily through the Pos* (objmap2.c's MapCellAtPos).
 *
 * The `volatile` read is FREE -- the original loads pos->x here anyway -- and
 * is a CSE barrier, not a semantic change: the original does NOT share this
 * load with the two later `pos->x` reads in the work-order arm, it loads the
 * field again at each site.  Without the barrier VC6 keeps the probe's value
 * in eax across the whole dispatch, which costs the later reload AND pushes
 * the cell flags out of ax into cx (they are then tested as `cl`/`ch`), the
 * row-table load out of eax into esi, and shifts the rest of the body by one
 * index -- 36 of 167.  A volatile read at the USE site instead reproduces the
 * reload but not the schedule (VC6 will not hoist it across the `push` of the
 * y argument, where the original loads both operands before either push). */
static __inline Cell* MapCellAtPos(Pos* pos)
{
    int x = *(volatile int*)&pos->x;
    int y;

    if (x >= 0 && x < g_map->width) {
        y = pos->y;
        if (y >= 0 && y < g_map->height)
            return &g_map_rows[y][x];
    }
    return 0;
}

/* Make the tile at `pos` free for a path tile to be laid on. */
// FUNCTION: LEGOLAND 0x004779d0
void ClearCellForPath(Pos* pos)
{
    Cursor         saved;
    Cell*          cell;
    unsigned short flags;
    MapObj*        obj;
    ObjDef*        prev;
    BPos           sq;
    Pos            p;
    WorkOrder*     order;

    cell = MapCellAtPos(pos);   /* [sic] not null-checked below */
#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
    if (!cell)                /* QUIRKS.md B: mappath.c:362 -- an off-map tile */
        return;
#endif
    flags = cell->flags;
    if (flags & 0x40)
        return;

    if (flags & 0xa0) {
        prev = g_sel_def;
        obj = (MapObj*)cell->obj;
        saved = g_destroy_cursor;
        g_sel_def = obj->cls;
        sq.x = cell->bx;
        p.x = sq.x;
        sq.y = cell->by;
        p.y = sq.y;
        g_sel_def->deselect(obj, &p);
        if (cell->flags & 0x20) {
            if (g_sel_def->flags & 0x200000) {
                FreeBuildSlotAt(sq);
                ClearObjectUserFlags(obj, &p);
                IncrementObjectCount(g_sel_def);
                RemoveObjectFromMap(sq);
            }
        } else {
            RemObjFromMap(g_sel_def, obj, sq, &g_destroy_cursor);
        }
        g_destroy_cursor = saved;
        g_sel_def = prev;
        return;
    }

    if (flags & 0x800) {
        order = GetGardenerWorkOrderAt(pos->x, pos->y);
        if (order != 0) {
            ClearObjectUserFlags(order->obj, &order->pos);
            RemoveObjectPathTiles(order->obj->cls, &order->pos);
            EraseGardenerOrder(order);
            return;
        }
        order = GetMechanicWorkOrderAt(pos->x, pos->y);
        if (order != 0) {
            ClearObjectUserFlags(order->obj, &order->pos);
            RemoveObjectPathTiles(order->obj->cls, &order->pos);
            EraseMechanicOrder(order);
            return;
        }
    } else if (flags & 8) {
        RemovePathSquare(pos);
        cell->flags &= ~8;
        cell->rf &= ~1;
    }
}

/* ==========================================================================
 * 4. STAMPING A PLACED OBJECT INTO THE MAP  (AddObjectToMapByCursor)
 * ==========================================================================
 * NAMING.  This function was originally reconstructed as `AddObjectToMap`,
 * taking that name from popup.c's extern -- but `AddObjectToMap` is a real
 * export of the game at 0x0045dd80 (16 in the export table), the footprint
 * stamp objmap2.c defines.  THIS is 0x0045e080, a different, non-exported
 * function, so the reconstruction briefly held two definitions of one name at
 * two addresses, which the original program cannot have had and which would
 * not link.  Renamed here to `AddObjectToMapByCursor` (2026-09-05); objmap2.c
 * owns the exported name.  popup.c's extern and both call sites were renamed
 * with it -- a symbol name is not a codegen lever (only the prototype TYPES
 * are), and popup.c was re-audited unchanged after the rename.
 *
 * The "place one on the map" route the pop-up build panel takes.  Unlike the
 * plain footprint stamp at 0x0045dd80 (objmap2.c), which walks the CLASS's own
 * rect list, this one walks the EDIT CURSOR: it hands the class's placement
 * effect the tile centre, which rebuilds the edit cursor's outline for this
 * object at this position, and then uses the resulting cursor chain as the
 * footprint.  The whole 0x1834-byte cursor block is copied to the stack on
 * entry and copied back on exit, so the caller's cursor survives.
 *
 * The cursor chain is walked TWICE, in the same shape both times: for every
 * cursor in the chain whose flags do not carry 0x3000, for every rect in that
 * cursor's rect list, for every cell of that rect biased by the cursor origin:
 *
 *   pass 1  ClearCellForPath(cell)  -- evict whatever is there (see 3 above)
 *   pass 2  write the cell: owner, base cell, flags = (old & 0x10) | flags
 *           (plus 0x8000 when the class is "tall"), rf = 2
 *
 * Note what pass 2 does NOT do compared with 0x0045dd80: it never sets the
 * 0x80 "footprint cell" bit and never writes the cell's life byte, and its rf
 * is an unconditional 2 rather than being derived from class bits 1 and 2.
 * The caller supplies 0x20 for the render-chain base cell and 0 for the rest.
 * ------------------------------------------------------------------------- */

extern Cursor g_edit_cursor;                          /* 0x007febc0 */

extern void GetTileCentre(Pos* tile, Pos* out);       /* 0x0045ad60 */

/* The same bounds-checked cell fetch with the coordinates already in hand. */
static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

/* Place `obj` on the map over the footprint its class's effect handler paints
 * into the edit cursor at `pos`. */
// FUNCTION: LEGOLAND 0x0045e080
void AddObjectToMapByCursor(MapObj* obj, Pos* pos, unsigned int flags)
{
    Cursor  saved;
    ObjDef* def;
    Cursor* c;
    Rect    r;
    Pos     t;
    Pos     centre;
    BPos    bp;
    Cell*   cell;
    int     x;
    int     y;

    def = obj->cls;
    saved = g_edit_cursor;
    bp.x = (unsigned char)pos->x;
    bp.y = (unsigned char)pos->y;
    GetTileCentre(pos, &centre);
    def->effect(def->ctx, &centre, 0x8f8);

    c = &g_edit_cursor;
    while (c != 0) {
        r = c->rect;
        if (!(c->flags & 0x3000)) {
            for (;;) {
                for (y = r.top; y <= r.bottom; y++) {
                    for (x = r.left; x <= r.right; x++) {
                        t.x = c->origin.x + x;
                        t.y = c->origin.y + y;
                        cell = MapCellAt(t.x, t.y);
                        if (cell != 0)
                            ClearCellForPath(&t);
                    }
                }
                if (r.next == 0)
                    break;
                r = *r.next;
            }
        }
        c = c->next;
    }

    c = &g_edit_cursor;
    while (c != 0) {
        r = c->rect;
        if (!(c->flags & 0x3000)) {
            for (;;) {
                for (y = r.top; y <= r.bottom; y++) {
                    for (x = r.left; x <= r.right; x++) {
                        t.x = c->origin.x + x;
                        t.y = c->origin.y + y;
                        cell = MapCellAt(t.x, t.y);
                        if (cell != 0) {
                            cell->obj = obj;
                            *(BPos*)&cell->bx = bp;
                            cell->flags = (unsigned short)((cell->flags & 0x10) | flags);
                            if (def->flags & 0x800000)
                                cell->flags |= 0x8000;
                            cell->rf = 2;
                        }
                    }
                }
                if (r.next == 0)
                    break;
                r = *r.next;
            }
        }
        c = c->next;
    }

    g_edit_cursor = saved;
}
