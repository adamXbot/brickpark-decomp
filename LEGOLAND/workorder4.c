/* LEGOLAND -- point-to-point routing, the 3x3 path rect and the worker map
 * stamp.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Struct field OFFSETS and global addresses are load-bearing; names are ours.
 * ------------------------------------------------------------------------- */
#include "legoland.h"
#ifdef LEGOLAND_PORTABLE
#define MarkWorkersOnMap MarkWorkersOnMap_vc6_body
#endif

/* A plain 16-byte rectangle with no chain link (objrect.c's / workorder2.c's
 * Rect4; pathsq.c calls the same shape PathRect). */
typedef struct Rect4 {
    int left;                    /* +0x00 */
    int top;                     /* +0x04 */
    int right;                   /* +0x08 */
    int bottom;                  /* +0x0c */
} Rect4;

/* -------------------------------------------------------------- functions -- */

/* Intersect `a` and `b` into `out`; non-zero when the result is non-empty.
 * The four edges are done left, right, top, bottom -- the pairing is by
 * min/max, not by axis -- and the emptiness test re-reads them out of `out`. */
// FUNCTION: LEGOLAND 0x0045d560
int IntersectRect4(Rect4* out, Rect4* a, Rect4* b)
{
    if (a->left > b->left)     out->left = a->left;     else out->left = b->left;
    if (a->right < b->right)   out->right = a->right;   else out->right = b->right;
    if (a->top > b->top)       out->top = a->top;       else out->top = b->top;
    if (a->bottom < b->bottom) out->bottom = a->bottom; else out->bottom = b->bottom;

    if (out->left <= out->right && out->top <= out->bottom)
        return 1;
    return 0;
}

/* Scan the 5x5 block of cells whose top-left corner is `origin` and return a
 * 25-bit map of which of them are WALKABLE PATH -- map flags bit 4 set and RF
 * bit 1 clear.  Bit 24 is the top-left cell and bit 0 the bottom-right (the
 * mask starts at 0x01000000 and is shifted right once per cell, x inner).
 * Off-map cells are scanned as flags 0x40 / rf 0, so they never contribute. */
extern unsigned int ScanPathArea5x5(Pos* origin);           /* 0x0045c9c0 */

/* The nine 3x3 sub-blocks of that 5x5 map, and the offset of each one's
 * top-left corner from the centre cell.  Three parallel tables laid out
 * back to back, which is why the search's end test is the address of the
 * next one. */
extern const unsigned int g_path_3x3_masks[9];              /* 0x004b9558 */
extern const int          g_path_3x3_dx[9];                 /* 0x004b957c */
extern const int          g_path_3x3_dy[9];                 /* 0x004b95a0 */

/* Find a 3x3 block of walkable path tiles containing `pos` and report it in
 * `rect` (inclusive, so right = left + 2).  The nine candidate placements are
 * tried top-left first; the first that is entirely path wins. */
// FUNCTION: LEGOLAND 0x0045ca90
int FindPathRect(Pos* pos, Rect4* rect)
{
    Pos          origin;
    unsigned int bits;
    int          i;

    origin.x = pos->x - 2;
    origin.y = pos->y - 2;
    bits = ScanPathArea5x5(&origin);

    for (i = 0; i < 9; i++) {
        if ((bits & g_path_3x3_masks[i]) == g_path_3x3_masks[i]) {
            rect->left   = g_path_3x3_dx[i] + pos->x;
            rect->top    = g_path_3x3_dy[i] + pos->y;
            rect->right  = rect->left + 2;
            rect->bottom = rect->top + 2;
            return 1;
        }
    }
    return 0;
}

/* Non-zero when the tile at `p` is on the map, its cell is walkable path
 * (0x0045ce10: RF bit 0, or map flags bit 4 with RF bit 1 clear) and the tile
 * it currently DISPLAYS is not the plain path tile *g_path_tile_base -- i.e.
 * the cell is showing one of the patterned 3x3 plaza tiles. */
extern int  PathTileIsPatterned(Pos* p);                    /* 0x0045ce30 */
/* Put the plain path tile back on the cell at `p` (no bounds check). */
extern void ResetPathTile(Pos* p);                          /* 0x0045cb90 */

/* Repaint the 5x5 neighbourhood of `pos` after the path under it changed.
 * Every neighbour except the centre that is showing a patterned plaza tile
 * but is no longer covered by ANY complete 3x3 block of path drops back to
 * the plain path tile.  The centre cell is left to the caller. */
// FUNCTION: LEGOLAND 0x0045cd70
void RefreshPathArea(Pos* pos)
{
    Pos   p;
    Rect4 rect;

    for (p.x = pos->x - 2; p.x <= pos->x + 2; p.x++) {
        for (p.y = pos->y - 2; p.y <= pos->y + 2; p.y++) {
            if (p.x == pos->x && p.y == pos->y)
                continue;
            if (!PathTileIsPatterned(&p))
                continue;
            if (FindPathRect(&p, &rect))
                continue;
            ResetPathTile(&p);
        }
    }
}

/* ===========================================================================
 * THE POINT-TO-POINT FLOOD FILL
 * ===========================================================================
 * workorder2.c's FindPathLeg drives a breadth-first fill over walkable tiles
 * with 16-byte nodes {next, parent, x, y} pushed on the open list at
 * 0x0066b450; g_ptp_wave_count (0x00669250) counts the nodes THIS wave added
 * and g_ptp_visited (0x00669258) is the closed set.
 *
 * The visited set is a bitmap of SIX dwords per row, so 192 bits wide, and
 * 0x0066a45c (pathsq.c's g_path_square_neighbours) starts 0x1204 bytes later
 * -- 192 rows of 24 bytes is 4608, so it is a 192 x 192-bit map, not the
 * 256 x 256 the earlier note in bnvmove.c assumed.  A map wider or taller
 * than 192 cells would alias one row into the next / overrun the array.
 * ------------------------------------------------------------------------ */

/* A point-to-point flood-fill node (bnvmove.c / workorder2.c; 16 bytes). */
typedef struct PTPNode {
    struct PTPNode* next;   /* +0x00 open-list link */
    struct PTPNode* parent; /* +0x04 how we got here */
    int             x;      /* +0x08 tile x */
    int             y;      /* +0x0c tile y */
} PTPNode;

extern int          g_ptp_wave_count;       /* 0x00669250 */
extern unsigned int g_ptp_visited[];        /* 0x00669258 */
extern PTPNode*     g_ptp_open_head;        /* 0x0066b450 */

extern void* MemAlloc(int size);                            /* 0x0049e4ff */

/* Open the tile (x, y) reached from `parent`.  Rejected when off the map,
 * when the cell's RF bit 1 is set (impassable) or when it is already in the
 * visited set.  bnvmove.c and workorder2.c both declare this `int`; the
 * definition sets no return value, so the declared type is a caller-side
 * artefact -- it is written `void` here (see the notes). */
// FUNCTION: LEGOLAND 0x00482240
void AddPTPOpenNode(int x, int y, PTPNode* parent)
{
    int      w;
    PTPNode* n;

    if (x < 0 || x >= g_map->width)
        return;
    if (y < 0 || y >= g_map->height)
        return;
    if (g_map_rows[y][x].rf & 2)
        return;

    w = (x >> 5) + y * 6;
    if (g_ptp_visited[w] & (1 << (x & 0x1f)))
        return;

    n = (PTPNode*)MemAlloc(0x10);
    if (!n)
        return;

    n->next = g_ptp_open_head;
    g_ptp_open_head = n;
    n->parent = parent;
    n->x = x;
    n->y = y;
    g_ptp_wave_count++;
    g_ptp_visited[w] |= 1 << (x & 0x1f);
}

/* Open the tile (x, y) reached from `parent` during workorder2.c's
 * FindPathLeg.  Looser than AddPTPOpenNode: a cell whose RF bit 1 says
 * impassable is still accepted when its map flags carry 0x0800.
 * There is NO null check on the allocation -- the node is filled in
 * unconditionally (an original bug, reproduced; AddPTPOpenNode at 0x00482240
 * does test it).  workorder2.c declares this `int`; nothing sets a return
 * value, so it is written `void` here. */
// FUNCTION: LEGOLAND 0x00482620
void PTPVisitTile(int x, int y, PTPNode* parent)
{
    int      w;
    PTPNode* n;
    Cell*    c;

    if (x < 0 || x >= g_map->width)
        return;
    if (y < 0 || y >= g_map->height)
        return;
    c = &g_map_rows[y][x];
    if ((c->rf & 2) && !(c->flags & 0x800))
        return;

    w = (x >> 5) + y * 6;
    if (g_ptp_visited[w] & (1 << (x & 0x1f)))
        return;

    n = (PTPNode*)MemAlloc(0x10);
    n->next = g_ptp_open_head;
    g_ptp_open_head = n;
    n->parent = parent;
    n->x = x;
    n->y = y;
    g_ptp_wave_count++;
    g_ptp_visited[w] |= 1 << (x & 0x1f);
}

/* ===========================================================================
 * THE WORKER MAP STAMP  (MarkWorkersOnMap)
 * ===========================================================================
 * printlist.c's build-site check needs to know which cells a hired worker is
 * standing on; map flag 0x1000 is that mark.  Both worker lists are walked --
 * mechanics (0x0079a8ac) FIRST, then gardeners (0x0079a8a8) -- and the cell
 * under each worker's 24.8 world position gets the bit.  Nothing ever clears
 * it here: the caller reads it immediately afterwards.
 * ------------------------------------------------------------------------ */

/* Only the two fields this file touches of workers2.c's 0xb0-byte Bloke. */
typedef struct Bloke {
    struct Bloke* next;      /* +0x00 */
    char          pad4[0x68 - 0x04];
    Pos           world;     /* +0x68 world position, 24.8 */
} Bloke;

extern Bloke* g_gardener_list;                              /* 0x0079a8a8 */
extern Bloke* g_mechanic_list;                              /* 0x0079a8ac */

// FUNCTION: LEGOLAND 0x0049cf00
void MarkWorkersOnMap(void)
{
    Bloke* b;

    for (b = g_mechanic_list; b != 0; b = b->next) {
        if (b->world.x >= 0 && (b->world.x >> 8) < g_map->width
            && b->world.y >= 0 && (b->world.y >> 8) < g_map->height)
            g_map_rows[b->world.y >> 8][b->world.x >> 8].flags |= 0x1000;
    }
    for (b = g_gardener_list; b != 0; b = b->next) {
        if (b->world.x >= 0 && (b->world.x >> 8) < g_map->width
            && b->world.y >= 0 && (b->world.y >> 8) < g_map->height)
            g_map_rows[b->world.y >> 8][b->world.x >> 8].flags |= 0x1000;
    }
}

/* ===========================================================================
 * CORNER CUTTING  (PTPShortcutSteps)
 * ===========================================================================
 * workorder3.c's BuildPTPRoute walks the parent chain of the node that
 * reached the target back to the tile the walker stands on, keeping the last
 * four it passed: `a` (the chain root, where we are), then `b`, `c`, `d`
 * outwards along the route.  This decides how many of them one move may
 * cover: 0 = step to `b`, 1 = step to `c`, 2 = step to `d`.
 *
 * Two steps that lie on the SAME axis need no test -- walking straight
 * through `b` cannot clip anything -- so the answer is pushed one node
 * further out.  Two steps that TURN form a diagonal, and the diagonal is only
 * allowed when the tile it cuts across, `a` offset by the SECOND step's
 * delta, is enterable: RF bit 1 clear, or map flags 0x0800 set (the same
 * predicate PTPVisitTile opens a tile on).
 *
 * The third-step test at the bottom compares against the FIRST step's deltas
 * again, not the second's, and the tile it looks up is `a` offset by the
 * THIRD step's delta -- reproduced as found.  Neither cell fetch is bounds
 * checked; an off-map route node dereferences a wild row pointer.  (Both
 * are safe in practice only because every node on the chain came from
 * PTPVisitTile, which does bound its tiles.)
 *
 * NOTE for workorder3.c's account: the blocked bit is map flags 0x0800 read
 * as byte +0x0d of the cell, not "Cell +0x1d bit 3", and it EXCUSES the
 * tile rather than rejecting it.
 *
 * The four deltas must be computed in the order dx1, dy1, dx2, dy2 -- step by
 * step, not "the pair the first test needs" first.  Computing dx2 first
 * spills dx1 to the frame instead of `c`'s coordinates and costs 67 of 113
 * plus an instruction; the step order is exact.  The operand order of the two
 * sums that index the cell is then inert.
 * ------------------------------------------------------------------------ */

// FUNCTION: LEGOLAND 0x00482330
int PTPShortcutSteps(PTPNode* a, PTPNode* b, PTPNode* c, PTPNode* d)
{
    int   dx1, dy1, dx2, dy2, dx3, dy3;
    Cell* cell;

    if (!b)
        return 0;
    if (!c)
        return 0;

    dx1 = b->x - a->x;
    dy1 = b->y - a->y;
    dx2 = c->x - b->x;
    dy2 = c->y - b->y;

    if ((dx2 && dy1) || (dy2 && dx1)) {
        cell = &g_map_rows[a->y + dy2][a->x + dx2];
        if ((cell->rf & 2) && !(cell->flags & 0x800))
            return 0;
        return 1;
    }

    if (!d)
        return 0;

    dx3 = d->x - c->x;
    dy3 = d->y - c->y;

    if ((dx3 && dy1) || (dy3 && dx1)) {
        cell = &g_map_rows[a->y + dy3][a->x + dx3];
        if ((cell->rf & 2) && !(cell->flags & 0x800))
            return 0;
        return 2;
    }
    return 1;
}

#ifdef LEGOLAND_PORTABLE
/* MarkWorkersOnMap is called with 1 argument(s) the original ignores: the body
 * at this address never reads them, and in cdecl the caller cleans them up.
 * On wasm the argument count is part of the function type, so the exported
 * name is this forwarder and the matched body keeps its own.  */
#undef MarkWorkersOnMap
void MarkWorkersOnMap(int ll_a1) { (void)ll_a1; MarkWorkersOnMap_vc6_body(); }
#endif
