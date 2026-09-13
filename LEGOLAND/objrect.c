/* LEGOLAND -- object rectangles, path tiles, work orders and the route-search
 * node factory.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Struct field OFFSETS and global addresses are load-bearing; names are ours.
 * The Cell / ObjDef / MapObj / Cursor shapes are objmap2.c's and mappath.c's,
 * extended where this file is the first to touch a field.
 *
 * ===========================================================================
 * 1. DOORS ARE A MAP-CELL FIELD  (SetObjectDoorFlags / ClearObjectUserFlags)
 * ===========================================================================
 * legoland.h has Cell +0x12 down as padding.  It is not: it is a 16-bit set
 * of DOOR DIRECTION BITS, one nibble per door, ORed on when an object is
 * placed and masked off when it goes.  A class carries an ENTRANCE offset at
 * ObjDef +0x0c/+0x10 (two ints -- the same pair objmap2.c's GetObjectUID uses
 * as the "footprint offset") and an EXIT offset at +0x24/+0x25 (two SIGNED
 * bytes; both readers use `movsx`).  Whether a door exists is decided by two
 * predicates bigrender.c already named:
 *
 *   ObjHasEntrance (0x0045e620)  class non-null, its type at +0x20 is none of
 *                                0, 2 or 3, and the entrance offset falls
 *                                OUTSIDE the class footprint rect at +0x3c
 *   ObjHasExit     (0x0045e690)  the exit offset differs from the entrance
 *
 * and the direction comes from which side of the footprint the door sits on:
 * GetObjEntranceDir (0x0045e6b0) returns 4 / 8 / 1 / 2 and GetObjExitDir
 * (0x0045e710) the same four sides in the high nibble, 0x80 / 0x40 / 0x20 /
 * 0x10.  A tile can therefore carry the doors of several neighbouring
 * objects at once, which is what the bloke AI needs to find a way in.
 *
 * NAMING: mappath.c and objmap2.c both declare 0x0045e850 as
 * `ClearObjectUserFlags`; the name is kept (one name per address) but it is a
 * misnomer -- the field it clears is +0x12, not the user flags at +0x0e.
 *
 * ===========================================================================
 * 2. RE-STAMPING A FOOTPRINT  (RefreshObjectAtPos)
 * ===========================================================================
 * The edit-cursor walk mappath.c's AddObjectToMapByCursor (0x0045e080) uses,
 * run ONCE: the class's placement effect (+0x90, effect code 0x8f8) repaints
 * the edit cursor's outline for this object at this tile, and every cell of
 * every rect of every cursor in the chain that does not carry flags 0x3000 is
 * RESET to "this object owns me, nothing else" -- flags 0, obj, base square,
 * rf 0.  Unlike the placement stamp it keeps no old flag bits, sets neither
 * 0x80 nor the 0x8000 "tall" bit, and never touches the cell's life byte.
 * The whole 0x1834-byte edit cursor is saved to the stack and restored, so a
 * caller mid-edit survives.
 *
 * ===========================================================================
 * 3. THE DISJOINT RECTANGLE SET  (SubtractObjRect)
 * ===========================================================================
 * The scratch set at 0x00801a80 (count 0x00667d3c) that workorder2.c's
 * RefreshObjList accumulates.  Subtraction removes every entry the argument
 * overlaps by unordered swap-erase and appends up to four remainder pieces:
 * full-width strips above and below, and left/right strips clipped to the
 * intersection's rows -- which is what keeps the set disjoint.  The scan
 * steps BACK over the entry it swapped in and the loop condition re-reads the
 * count, so pieces produced by one split are themselves re-tested by the same
 * call.  0x0045d560 is the plain four-int rect intersection it uses.
 *
 * ===========================================================================
 * 4. TEARING PATH OFF AN OBJECT  (RemoveObjectPathTiles)
 * ===========================================================================
 * Classes of type 0 and 2 are skipped.  For the rest the class footprint,
 * biased by the object's square, is swept TWICE -- once GROWN by one cell on
 * every side, once exactly -- clearing map flags 0x18, zeroing rf, restoring
 * the displayed tile from the ground tile, and running UpdatePathNeighbours
 * and RemovePathSquare per cell.  The second sweep is a genuine repeat of the
 * interior (pass 1's rect strictly contains pass 2's): pass 1's border-ring
 * neighbour updates re-tile the cells just inside the ring, so the interior
 * is swept again afterwards.  Reproduced, not fixed.  There is NO bounds
 * check on the cell fetch here -- objmap2.c's RemObjFromMap tail has the same
 * three statements with the same exposure.
 *
 * ===========================================================================
 * 5. THE ROUTE SEARCH'S TERRAIN MODEL  (GetRouteNode)
 * ===========================================================================
 * simcore.c's RequestRoute asks for the node of a tile; `*state` reports 2
 * (already closed), 1 (already open) or 0 (fresh).  A fresh node is costed
 * from the map cell in one dispatch, and that dispatch IS the auto-path
 * router's terrain model -- RouteNode +0x20 is the terrain class and +0x10
 * the price of stepping onto the tile:
 *
 *   path tile (flags 0x10) or rf bit 0  cost 1; class 1 when the tile joins a
 *                                       path square carrying flag 2 (which
 *                                       ends the search), else class 0
 *   blocked (flags 0x40)                class 5, cost -1 (impassable)
 *   object cell (flags 0x8a0)           the object's class decides:
 *                                         no 0x200000 -> class 5, cost -1
 *                                         class +0x2a > 1 -> class 4, cost 20
 *                                         otherwise       -> class 3, cost 9
 *   rf bit 1                            class 5, cost -1
 *   open ground                         class 2, cost 3
 *
 * so routing THROUGH a "may be built over" object is allowed but priced at 9
 * or 20 tiles of open ground.  The seed is g = INT_MAX, parent and dir zero,
 * h = Manhattan distance to g_route_to and f = COST + h (the entry cost, not
 * g).  The cell fetch is the same unguarded one mappath.c's ClearCellForPath
 * and simcore.c's RequestRoute use: an off-map tile dereferences 0.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* A map cell.  legoland.h's Cell calls +0x12 padding; SetObjectDoorFlags
 * proves it is a live 16-bit field holding the entrance/exit direction bits
 * of whatever object's door opens onto this tile (see the header note above
 * SetObjectDoorFlags).  Otherwise identical to legoland.h's Cell, 20 bytes. */
typedef struct MapCell {
    void*          obj;      /* +0x00 render/tile object */
    unsigned char  bx;       /* +0x04 base cell x of the object that owns this */
    unsigned char  by;       /* +0x05 */
    unsigned char  nx;       /* +0x06 next cell in the render chain */
    unsigned char  ny;       /* +0x07 */
    unsigned short tile;     /* +0x08 displayed tile */
    unsigned short base;     /* +0x0a ground/terrain tile */
    unsigned short flags;    /* +0x0c map flags */
    unsigned short uflags;   /* +0x0e user flags */
    unsigned char  rf;       /* +0x10 RF/path flags */
    unsigned char  life;     /* +0x11 remaining life */
    unsigned short doors;    /* +0x12 door direction bits (see below) */
} MapCell;

/* An object class / definition (the 0xd0-byte ODF record); objmap2.c's
 * ObjDef with the exit offset at +0x24/+0x25 added -- ObjHasExit and
 * GetObjExitDir both read those two bytes with `movsx`, so they are SIGNED
 * bytes where the entrance offset at +0x0c/+0x10 is a pair of ints. */
typedef struct ObjDef {
    char           pad0[0x0c];   /* +0x00 */
    int            dx;           /* +0x0c entrance offset from the base cell */
    int            dy;           /* +0x10 */
    char           pad14[0x1c - 0x14];
    unsigned int   flags;        /* +0x1c */
    short          type;         /* +0x20 */
    char           pad22[0x24 - 0x22];
    signed char    ex;           /* +0x24 exit offset from the base cell */
    signed char    ey;           /* +0x25 */
    char           pad26[0x2a - 0x26];
    short          w2a;          /* +0x2a  route cost selector, see GetRouteNode */
    char           pad2c[0x3c - 0x2c];
    Rect           rect;         /* +0x3c footprint rect list */
    char           pad50[0x90 - 0x50];
    void         (*effect)(void* ctx, Pos* pt, int effect); /* +0x90 */
    char           pad94[0xc4 - 0x94];
    void*          ctx;          /* +0xc4 the class's LLIDB element */
    char           padc8[0xd0 - 0xc8];
} ObjDef;

/* A placed object's map square, packed as two bytes (objmap2.c). */
typedef struct BPos {
    unsigned char x;             /* +0x00 */
    unsigned char y;             /* +0x01 */
} BPos;

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
    Rect           rect;         /* +0x1414 footprint rect list */
    unsigned char  style;        /* +0x1428 */
    char           pad1429[0x1828 - 0x1429];
    unsigned int   flags;        /* +0x1828 */
    int            f182c;        /* +0x182c */
    struct Cursor* next;         /* +0x1830 */
} Cursor;

/* One node of the auto-path route search (simcore.c's RouteNode); 0x28
 * bytes, allocated here and threaded through the open and closed lists. */
typedef struct RouteNode {
    struct RouteNode* next;      /* +0x00 open/closed list link */
    struct RouteNode* parent;    /* +0x04 how we got here */
    Pos               pos;       /* +0x08 map tile */
    int               cost;      /* +0x10 cost of entering, -1 = impassable */
    int               g;         /* +0x14 cost so far */
    int               h;         /* +0x18 heuristic remaining */
    int               f;         /* +0x1c the open-list sort key */
    int               axis;      /* +0x20 terrain class (see GetRouteNode) */
    int               dir;       /* +0x24 axis of the step that reached here */
} RouteNode;

/* A placed map object: its class sits at +0x0c. */
typedef struct MapObj {
    char    pad0[0x0c];          /* +0x00 */
    ObjDef* cls;                 /* +0x0c */
} MapObj;

/* ---------------------------------------------------------------- callees -- */

extern int  ObjHasEntrance(ObjDef* d);                          /* 0x0045e620 */
extern int  GetObjEntranceDir(ObjDef* d);                       /* 0x0045e6b0 */
extern int  ObjHasExit(ObjDef* d);                              /* 0x0045e690 */
extern int  GetObjExitDir(ObjDef* d);                           /* 0x0045e710 */
extern void GetTileCentre(Pos* tile, Pos* out);                 /* 0x0045ad60 */
extern void UpdatePathNeighbours(Pos* p);                       /* 0x0045d260 */
extern void RemovePathSquare(Pos* p);                           /* 0x00481c90 */

/* --- the route search's node factory needs these ------------------------- */
struct RouteNode;
/* Linear scan of the closed / open list (0x00668fc4, 0x00668fc0) for the node
 * whose Pos at +0x08 is this tile. */
extern struct RouteNode* FindClosedRouteNode(Pos* pos);         /* 0x00477730 */
extern struct RouteNode* FindOpenRouteNode(Pos* pos);           /* 0x004777c0 */
/* Non-zero when `pos` lies in a path square carrying flag 2 (PathSquare
 * +0x20).  It also FLUSHES the pending path-GFX batch (0x0066b46c) through
 * 0x00482b20 and zeroes it, so it is not a pure predicate. */
extern int   TileJoinsPathNetwork(Pos* pos);                    /* 0x00482b60 */
extern void* HeapAlloc_w(unsigned int size);                    /* 0x0049e4ff */

int abs(int);
#pragma intrinsic(abs)

/* A plain 16-byte rectangle with no chain link -- the element type of the
 * scratch rect list at 0x00801a80 (workorder2.c's Rect4). */
typedef struct Rect4 {
    int left;                    /* +0x00 */
    int top;                     /* +0x04 */
    int right;                   /* +0x08 */
    int bottom;                  /* +0x0c */
} Rect4;

/* Intersect `a` and `b` into `out`; non-zero when the result is non-empty. */
extern int  IntersectRect4(Rect4* out, Rect4* a, Rect4* b);     /* 0x0045d560 */

/* ---------------------------------------------------------------- globals -- */

extern Cursor g_edit_cursor;                                    /* 0x007febc0 */

/* The route request the node factory reads (simcore.c's g_route_to). */
extern Pos    g_route_to;                                       /* 0x004bb5a0 */

/* The scratch list of disjoint rectangles the cursor refresh accumulates
 * (workorder2.c's g_obj_rects / g_obj_rect_count). */
extern Rect4  g_obj_rects[];                                    /* 0x00801a80 */
extern int    g_obj_rect_count;                                 /* 0x00667d3c */

/* ---------------------------------------------------------- inline helpers -- */

/* The bounds-checked cell fetch every map accessor open-codes (objmap2.c),
 * typed to this file's MapCell. */
static __inline MapCell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return (MapCell*)&g_map_rows[y][x];
    return 0;
}

/* ==========================================================================
 * DOOR BITS ON THE MAP  (SetObjectDoorFlags)
 * ==========================================================================
 * A class carries up to two doors: an ENTRANCE at ObjDef +0x0c/+0x10 (two
 * ints) and an EXIT at +0x24/+0x25 (two signed bytes), both offsets from the
 * object's base cell.  ObjHasEntrance (0x0045e620) is true when the class is
 * non-null, its type at +0x20 is none of 0, 2 or 3, and the entrance offset
 * falls OUTSIDE the class footprint rect at +0x3c; ObjHasExit (0x0045e690) is
 * true when the exit offset differs from the entrance offset.
 *
 * When an object is placed, this function ORs the door's direction bits into
 * the tile the door opens onto -- MapCell +0x12, a field legoland.h had down
 * as padding.  GetObjEntranceDir returns 4 / 8 / 1 / 2 (the side of the
 * footprint the entrance sits on: below / above / right / left in the order
 * the tests run) and GetObjExitDir the same four sides shifted four bits up,
 * 0x80 / 0x40 / 0x20 / 0x10.  So one nibble per door, and a tile can carry the
 * doors of several neighbouring objects at once.
 *
 * Codegen: the two coordinate sums are written inside the MapCellAt CALL
 * ARGUMENTS, which is what puts both adds before the helper's own `x >= 0`
 * guard.  The exit block is NESTED inside the entrance block, so both
 * failures share the single trailing epilogue.
 * ------------------------------------------------------------------------- */

/* Stamp `obj`'s entrance and exit direction bits onto the tiles they open
 * onto, with the object placed at map cell `pos`.
 *
 * Codegen: the two coordinate sums must be STATEMENTS assigned into a `Pos t`
 * in x-then-y order, not expressions written into MapCellAt's arguments.  In
 * the argument form VC6 evaluates the two arguments right to left, so the y
 * sum is computed last and its own flags carry the `x >= 0` guard as a bare
 * `js`; the original re-tests with `test eax,eax / jl`, which is what a y sum
 * scheduled BETWEEN the x sum and the guard forces.  Worth 52/76 -> 78/78. */
// FUNCTION: LEGOLAND 0x0045e770
void SetObjectDoorFlags(MapObj* obj, Pos* pos)
{
    ObjDef*  def;
    MapCell* cell;
    Pos      t;

    def = obj->cls;
    if (ObjHasEntrance(def)) {
        t.x = def->dx + pos->x;
        t.y = def->dy + pos->y;
        cell = MapCellAt(t.x, t.y);
        if (cell != 0)
            cell->doors |= GetObjEntranceDir(def);
        if (ObjHasExit(def)) {
            t.x = def->ex + pos->x;
            t.y = def->ey + pos->y;
            cell = MapCellAt(t.x, t.y);
            if (cell != 0)
                cell->doors |= GetObjExitDir(def);
        }
    }
}

/* The exact twin of SetObjectDoorFlags with `&= ~` for `|=` -- called when an
 * object is removed, or a work order over it cancelled, to take its door bits
 * back off the tiles they were stamped onto.
 *
 * NAME: mappath.c and objmap2.c both declare this as `ClearObjectUserFlags`
 * and that name is kept (one name per address, and a symbol name is not a
 * codegen lever), but it is a misnomer -- the field it clears is the door
 * nibble pair at MapCell +0x12, not the user flags at +0x0e. */
// FUNCTION: LEGOLAND 0x0045e850
void ClearObjectUserFlags(MapObj* obj, Pos* pos)
{
    ObjDef*  def;
    MapCell* cell;
    Pos      t;

    def = obj->cls;
    if (ObjHasEntrance(def)) {
        t.x = def->dx + pos->x;
        t.y = def->dy + pos->y;
        cell = MapCellAt(t.x, t.y);
        if (cell != 0)
            cell->doors &= ~GetObjEntranceDir(def);
        if (ObjHasExit(def)) {
            t.x = def->ex + pos->x;
            t.y = def->ey + pos->y;
            cell = MapCellAt(t.x, t.y);
            if (cell != 0)
                cell->doors &= ~GetObjExitDir(def);
        }
    }
}

/* ==========================================================================
 * REPAINTING A PLACED OBJECT'S FOOTPRINT  (RefreshObjectAtPos)
 * ==========================================================================
 * The same edit-cursor walk mappath.c's AddObjectToMapByCursor (0x0045e080)
 * uses, run ONCE and with a different cell write.  The class's placement
 * effect (+0x90, effect code 0x8f8) is handed the tile centre so it repaints
 * the edit cursor's outline for this object at this position, and every cell
 * of every rect of every cursor in the chain that does not carry flags
 * 0x3000 is then RESET to "this object owns me, nothing else":
 *
 *     flags = 0            (all map bits dropped -- not merged as in the
 *                           placement stamp, which keeps 0x10)
 *     obj   = the object
 *     bx/by = the object's base square
 *     rf    = 0
 *
 * so it does NOT set 0x80/0x20 or the "tall" bit, and it does not touch the
 * cell's life byte.  workers.c and workorder.c call it when a work order
 * finishes and the object's tiles have to be taken back from whatever the
 * order painted over them.  The whole 0x1834-byte edit cursor is saved to the
 * stack on entry and restored on exit, so the caller's cursor survives.
 * ------------------------------------------------------------------------- */

/* Re-stamp `obj` over the footprint its class's effect handler paints into the
 * edit cursor at `pos`. */
// FUNCTION: LEGOLAND 0x0045e4a0
void RefreshObjectAtPos(MapObj* obj, Pos* pos)
{
    Cursor   saved;
    ObjDef*  def;
    Cursor*  c;
    Rect     r;
    Pos      t;
    Pos      centre;
    BPos     bp;
    MapCell* cell;
    int      x;
    int      y;

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
                        if (cell != 0) {
                            cell->flags = 0;
                            cell->obj = obj;
                            *(BPos*)&cell->bx = bp;
                            cell->rf = 0;
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

/* ==========================================================================
 * THE DISJOINT RECTANGLE SET  (SubtractObjRect)
 * ==========================================================================
 * workorder2.c's RefreshObjList keeps a set of DISJOINT rectangles in the
 * array at 0x00801a80 with its count at 0x00667d3c: it adds the selected
 * object's footprint and subtracts every other cursor's, then re-lays path on
 * whatever is left.  This is the subtraction.
 *
 * For every entry the argument overlaps, the entry is REMOVED -- by copying
 * the list's last entry over it and dropping the count, the usual unordered
 * swap-erase -- and up to four remainder pieces are appended in this order:
 *
 *     top strip     {s.left,      s.top,        s.right, out.top - 1   }
 *     left strip    {s.left,      out.top,      out.left - 1, out.bottom}
 *     right strip   {out.right+1, out.top,      s.right, out.bottom    }
 *     bottom strip  {s.left,      out.bottom+1, s.right, s.bottom      }
 *
 * i.e. full-width top and bottom strips and left/right strips clipped to the
 * intersection's rows, which is why the result stays disjoint.  Because the
 * appended pieces land at the END of the array and the loop index is stepped
 * BACK over the swapped-in entry (`i--; p--;` against the for's own `i++;
 * p++`), the scan re-tests the entry it just moved into place and then walks
 * on through everything it appended -- so a rectangle produced by one split
 * is itself split by the same call if it still overlaps.  The loop condition
 * re-reads the count each iteration, so the list growing under it is part of
 * the design, not an accident.
 * ------------------------------------------------------------------------- */

/* Subtract `r` from the disjoint rect set, splitting every entry it meets.
 *
 * Codegen notes, all measured:
 *  - THE FOUR APPENDS MUST BE WHOLE-STRUCT ASSIGNMENTS THROUGH A `Rect4 t`.
 *    Written as four stores through a `Rect4* q = &g_obj_rects[n]`, the
 *    stores kill the CSEs of the address-taken `out` and VC6 reloads
 *    out.top / out.bottom in every block (47.9%); written as four direct
 *    `g_obj_rects[n].field = ...` subscripts the CSEs survive but the base
 *    folds into each store's disp32 instead of being added into a register
 *    (`mov [ecx+g_obj_rects], esi` for the original's `add ebp,0x801a80` +
 *    `mov [ebp], esi`).  Filling a scalarised `t` and assigning
 *    `g_obj_rects[n] = t;` gets both: the struct-assignment lowering
 *    materialises the destination address in a register, and the field values
 *    are all read BEFORE the first store.
 *  - THE COUNT IS A LOCAL `n` MIRRORED INTO THE GLOBAL, and the loop
 *    condition tests the LOCAL.  With `i < g_obj_rect_count` in the condition
 *    the reload lands in the shared latch; with `i < n` and the miss arm
 *    written as `else { n = g_obj_rect_count; }` the reload is the only thing
 *    in that arm, the arm ends in a jump, and the block-exile rule parks it
 *    past the epilogue exactly as the original does.
 *  - THAT ARM'S READ IS A FREE `volatile` READ.  Without the barrier VC6
 *    shares it with the `n = g_obj_rect_count - 1` load at the top of the hit
 *    arm and the whole body comes out with s.left and s.top swapped between
 *    esi and edx (14 of 118 strict, register-blind 0 -- a pure allocation
 *    residual).  The barrier costs nothing (the original loads there anyway)
 *    and closes all 14.  Placed instead on the `- 1` load it closes 11 and
 *    pushes the s.bottom spill store two indices early, because VC6 will not
 *    hoist a volatile access across a store.
 *  - THE POINTER LIVES IN A POST-GUARD INNER SCOPE, which is what sinks the
 *    `push ebx` and the `mov ebx, g_obj_rects` past the `count > 0` guard. */
// FUNCTION: LEGOLAND 0x0045d5d0
void SubtractObjRect(Rect4* r)
{
    Rect4 out;
    Rect4 s;
    Rect4 t;
    int   i;
    int   n;

    i = 0;
    n = g_obj_rect_count;
    if (n > 0) {
        Rect4* p = g_obj_rects;
        do {
            if (IntersectRect4(&out, r, p)) {
                s = *p;
                n = g_obj_rect_count - 1;
                g_obj_rect_count = n;
                if (i < n)
                    *p = g_obj_rects[n];   /* unordered swap-erase */
                i--;                       /* re-test the entry moved in */
                p--;
                if (out.top > s.top) {     /* full-width strip above */
                    t.left = s.left;
                    t.top = s.top;
                    t.right = s.right;
                    t.bottom = out.top - 1;
                    g_obj_rects[n] = t;
                    n++;
                    g_obj_rect_count = n;
                }
                if (out.left > s.left) {   /* strip left of the overlap */
                    t.left = s.left;
                    t.top = out.top;
                    t.right = out.left - 1;
                    t.bottom = out.bottom;
                    g_obj_rects[n] = t;
                    n++;
                    g_obj_rect_count = n;
                }
                if (out.right < s.right) { /* strip right of the overlap */
                    t.left = out.right + 1;
                    t.top = out.top;
                    t.right = s.right;
                    t.bottom = out.bottom;
                    g_obj_rects[n] = t;
                    n++;
                    g_obj_rect_count = n;
                }
                if (out.bottom < s.bottom) { /* full-width strip below */
                    t.left = s.left;
                    t.top = out.bottom + 1;
                    t.right = s.right;
                    t.bottom = s.bottom;
                    g_obj_rects[n] = t;
                    n++;
                    g_obj_rect_count = n;
                }
            } else {
                /* Free volatile read -- the original loads here anyway; the
                 * barrier keeps it out of the hit arm's load.  See above. */
                n = *(volatile int*)&g_obj_rect_count;
            }
        } while (p++, i++, i < n);
    }
}

/* ==========================================================================
 * TEARING THE PATH OFF AN OBJECT'S TILES  (RemoveObjectPathTiles)
 * ==========================================================================
 * mappath.c's ClearCellForPath calls this when a work order over an object is
 * cancelled, and RemObjFromMap when the object itself goes: it drops the path
 * that was laid on and around the object's footprint.
 *
 * Classes of type 0 and 2 are skipped entirely (type 2 is objmap2.c's "no
 * instance record" class).  For everything else the class footprint rect at
 * ObjDef +0x3c, biased by the object's map square, is walked TWICE:
 *
 *   pass 1  the rect GROWN by one cell on every side (left-1 .. right+1,
 *           top-1 .. bottom+1) -- the footprint plus its border ring;
 *   pass 2  the rect exactly as it is.
 *
 * and each cell of each pass gets the identical treatment: map flags 0x18
 * (object cell | path tile) cleared, rf zeroed, the displayed tile restored
 * from the ground tile underneath, then UpdatePathNeighbours and
 * RemovePathSquare for that cell.  The second pass is a genuine repeat of
 * work pass 1 already did over the interior -- pass 1's rect strictly
 * contains pass 2's -- and it is reproduced, not fixed: pass 1's border-ring
 * UpdatePathNeighbours calls re-tile the cells just inside the ring, so the
 * interior sweep is run again afterwards.
 *
 * NO BOUNDS CHECK.  Unlike every other map accessor in this cluster the cells
 * are reached as `g_map_rows[y][x]` with no width/height test, so an object
 * whose footprint touches the map edge indexes one row/column off the grid.
 * Original behaviour, reproduced; objmap2.c's RemObjFromMap tail does the
 * same thing with the same three statements.
 * ------------------------------------------------------------------------- */

/* Strip the path tiles on and around `def`'s footprint at map square `pos`.
 *
 * Codegen: the loop counters must be PLAIN INTS with `p.x = x; p.y = y;`
 * written at the call sites.  Running the loops on `p.x` / `p.y` directly
 * (objmap2.c's RemObjFromMap tail does exactly that and matches there) pins
 * the address-taken pair in memory, costs the `x * 20` strength reduction
 * (the hoisted `lea edi,[ebp+ebp*4] / shl edi,2` with `add edi,0x14` at the
 * latch) and lands at 136 instructions against 122.  `if (def->type && ...)`
 * rather than `def->type != 0` keeps a zero register out of the pre-push
 * guard.  The dead `lea eax,[eax+edi+0xc]` after the flag AND is the
 * original's own ghost and comes out for free from the plain
 * `g_map_rows[y][x].flags &= ~0x18;`. */
// FUNCTION: LEGOLAND 0x0045d3d0
void RemoveObjectPathTiles(ObjDef* def, Pos* pos)
{
    Pos p;
    int x;
    int y;

    if (def->type && def->type != 2) {
        for (y = def->rect.top + pos->y - 1; y <= def->rect.bottom + pos->y + 1; y++) {
            for (x = pos->x + def->rect.left - 1; x <= pos->x + def->rect.right + 1; x++) {
                g_map_rows[y][x].flags &= ~0x18;
                g_map_rows[y][x].rf = 0;
                g_map_rows[y][x].tile = g_map_rows[y][x].base;
                p.x = x;
                p.y = y;
                UpdatePathNeighbours(&p);
                RemovePathSquare(&p);
            }
        }
        for (y = def->rect.top + pos->y; y <= def->rect.bottom + pos->y; y++) {
            for (x = pos->x + def->rect.left; x <= pos->x + def->rect.right; x++) {
                g_map_rows[y][x].flags &= ~0x18;
                g_map_rows[y][x].rf = 0;
                g_map_rows[y][x].tile = g_map_rows[y][x].base;
                p.x = x;
                p.y = y;
                UpdatePathNeighbours(&p);
                RemovePathSquare(&p);
            }
        }
    }
}

/* ==========================================================================
 * THE ROUTE SEARCH'S NODE FACTORY  (GetRouteNode)
 * ==========================================================================
 * simcore.c's RequestRoute asks for the node of a tile and is told through
 * `*state` where it came from: 2 = already on the closed list, 1 = already on
 * the open list, 0 = freshly allocated and costed here.  Only the 0 case
 * allocates; the other two hand back the existing node untouched, which is
 * what makes the two lists the search's "visited" set.
 *
 * A fresh node is costed from the map cell in one dispatch, and this is the
 * whole terrain model of the auto-path router.  `axis` (RouteNode +0x20) is
 * the terrain CLASS and `cost` (+0x10) what it costs to step onto the tile:
 *
 *   flags 0x10 (path tile) or rf bit 0 (walkable path)
 *                                 -> cost 1, axis 1 if the tile joins an
 *                                    existing path square carrying flag 2
 *                                    (RequestRoute stops the search there),
 *                                    else axis 0
 *   flags 0x40 (blocked)          -> axis 5, cost -1 (impassable)
 *   flags 0x8a0 (an object cell)  -> the cell's object's class decides:
 *                                    no 0x200000 ("may be built over")
 *                                       -> axis 5, cost -1
 *                                    class +0x2a > 1 -> axis 4, cost 20
 *                                    otherwise       -> axis 3, cost 9
 *   rf bit 1                      -> axis 5, cost -1
 *   otherwise (open ground)       -> axis 2, cost 3
 *
 * so building a path THROUGH a demolishable object is allowed but priced at
 * 9 or 20 tiles of open ground, and the router will detour round it unless
 * the detour is longer.
 *
 * The rest of the node is the A* seed: g = INT_MAX, parent and dir zero, and
 * h = |dx| + |dy| to g_route_to (Manhattan), with f = cost + h -- note f uses
 * the ENTRY COST, not g, so a fresh node's f is its heuristic plus its own
 * terrain price.
 *
 * ORIGINAL BUG, reproduced: the bounds-checked cell fetch returns 0 for an
 * off-map tile and the very next instruction reads flags at +0x0c through it.
 * mappath.c's ClearCellForPath and simcore.c's RequestRoute have the same
 * unguarded dereference of the same helper.
 * ------------------------------------------------------------------------- */

/* The bounds-checked cell fetch, reading the coordinates lazily through the
 * Pos* (simcore.c's RouteCellAt). */
static __inline Cell* RouteCellAt(Pos* p)
{
    if (p->x >= 0 && p->x < g_map->width && p->y >= 0 && p->y < g_map->height)
        return &g_map_rows[p->y][p->x];
    return 0;
}

/* Fetch (or make) the route node for `pos`; *state says where it came from.
 *
 * Codegen notes:
 *  - THE 0x200000 TEST MUST BE WRITTEN POSITIVELY, with the impassable case
 *    as its `else`.  Two `axis = 5; cost = -1;` blocks merge at the FIRST
 *    site, so the later one has to be the arm that JUMPS; written as
 *    `if (!(def->flags & 0x200000)) { 5; -1; } else ...` VC6 emits a second
 *    copy of the block and the function comes out three instructions long.
 *    The third copy (the `rf & 2` arm) is NOT merged in the original and is
 *    not merged here either -- it sits between the two in the layout.
 *  - THE HEURISTIC'S SUM DESTINATION.  `h = abs(dx) + abs(dy)` in either
 *    written order evaluates the y term first into ecx and lands the sum in
 *    ecx; the original's `add eax,ecx` puts it in the x term's register, so
 *    `h` has to BE the x term (`dy = abs(...y...); h = abs(...x...); h += dy;`).
 *  - THE TWO ZERO STORES ARE SCHEDULING BOUNDARIES.  `n->parent = 0` and
 *    `n->dir = 0` are interleaved into the abs sequence in the original;
 *    written together (before or after the sum) they emit together and cost
 *    5.  Split either side of the `h += dy` they land on the original's
 *    indices.  This is a reconstruction of the SCHEDULE, not a claim about
 *    how the statements were spaced in the original source. */
// FUNCTION: LEGOLAND 0x004777f0
RouteNode* GetRouteNode(Pos* pos, int* state)
{
    RouteNode*     n;
    Cell*          cell;
    ObjDef*        def;
    unsigned short flags;
    unsigned char  rf;
    int            h;
    int            dy;

    n = FindClosedRouteNode(pos);
    if (n != 0) {
        *state = 2;
        return n;
    }
    n = FindOpenRouteNode(pos);
    if (n != 0) {
        *state = 1;
        return n;
    }

    *state = 0;
    n = (RouteNode*)HeapAlloc_w(sizeof(RouteNode));

    cell = RouteCellAt(pos);   /* [sic] not null-checked below */
#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
    if (!cell) {              /* QUIRKS.md B: objrect.c:653 -- an off-map square routes as impassable */
        static Cell ll_offmap_cell;
        ll_offmap_cell.flags = 0x40;
        cell = &ll_offmap_cell;
    }
#endif
    flags = cell->flags;
    if ((flags & 0x10) || (cell->rf & 1)) {
        if (TileJoinsPathNetwork(pos))
            n->axis = 1;
        else
            n->axis = 0;
        n->cost = 1;
    } else {
        rf = cell->rf;
        if (flags & 0x40) {
            n->axis = 5;
            n->cost = -1;
        } else if (flags & 0x8a0) {
            def = ((MapObj*)cell->obj)->cls;
            if (def->flags & 0x200000) {
                if (def->w2a > 1) {
                    n->axis = 4;
                    n->cost = 0x14;
                } else {
                    n->axis = 3;
                    n->cost = 9;
                }
            } else {
                /* Identical to the 0x40 arm above; VC6 merges the pair at the
                 * FIRST site, which is why this must be the else (jump) arm
                 * and the 0x200000 test must be written positively. */
                n->axis = 5;
                n->cost = -1;
            }
        } else if (rf & 2) {
            n->axis = 5;
            n->cost = -1;
        } else {
            n->axis = 2;
            n->cost = 3;
        }
    }

    n->pos.x = pos->x;
    n->pos.y = pos->y;
    n->g = 0x7fffffff;
    dy = abs(pos->y - g_route_to.y);
    h = abs(pos->x - g_route_to.x);
    n->parent = 0;
    h += dy;
    n->dir = 0;
    n->h = h;
    n->f = n->cost + h;
    return n;
}
