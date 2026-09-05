/* LEGOLAND — path edge/corner masks, the cursor tile filter, the rendered-text
 * cache's single-entry drop and the entrance path-square resolve.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, global addresses and callee argument counts are load-bearing;
 * names are ours.
 *
 * ---------------------------------------------------------------------------
 * THE FOUR-NEIGHBOUR PATH MASK
 *
 * render5.c's DrawPathTileOverlay (0x00460e90) and its four-argument twin
 * 0x00460f50 draw a path cell as ONE base tile chosen by PathEdgeMask plus at
 * most one corner filler chosen by PathCornerMask.  PathEdgeMask probes the
 * four ORTHOGONAL neighbours in the order N, E, S, W and ORs one bit per
 * walkable-path neighbour:
 *
 *              0x1  N (x,   y-1)
 *      0x8  W (x-1, y  )        0x2  E (x+1, y  )
 *              0x4  S (x,   y+1)
 *
 * so 0x3 is the N+E pair and 0xc the S+W pair — the two DIAGONAL pairs of the
 * isometric grid, which is why render5.c's header calls them "one diagonal
 * pair" and "the other".  PathCornerMask then answers a 2-bit code: bit 0 when
 * the 0xc pair is complete but the cell diagonally beyond it (x-1, y+1) is NOT
 * path, bit 1 when the 0x3 pair is complete but (x+1, y-1) is not — i.e. an
 * inside corner needs its filler only when the diagonal is a hole.
 *
 * Both use the same probe as simcore.c's GetPathNeighbours: the cell is COPIED
 * into a local (rep movsd, 5 dwords) and an off-map cell stands in as
 * tile=0 / flags=0x40 / rf=0, so anything off the map is never path.  The
 * difference is that these two hand the COPY to the real IsPathCell
 * (0x0045ce10) rather than inlining its predicate, which is why the dead
 * `tile = 0` store survives here (the copy is address-taken) and is dropped in
 * IsPathRectClear below, whose probe reads the two flag bytes directly.
 *
 * IsPathRectClear is the other side of the same predicate, used by
 * pathmisc2.c's GrowPathRectSide: every cell of an inclusive rectangle must be
 * a path TILE (map flag 0x10) that is not BLOCKED (RF bit 1).  Note it does
 * NOT accept RF bit 0 the way IsPathCell does — a walkable non-path cell fails
 * — and an empty rectangle (left > right) answers 1.
 *
 * DrawCursorTileAt is the per-cell filter behind render5.c's PaintCursorTiles:
 * bounds-check the cell, keep it only when IsPathCell says path AND it has a
 * non-zero displayed tile, then hand the ORIGINAL Pos (not the copy) to the
 * four-argument overlay painter at 0x00460f50 with the caller's blit mode.
 *
 * FreeCachedTextEntry drops one entry of fpui3.c's rendered-text cache and
 * compacts the array; render5.c's ExpireCachedText is its only caller and is
 * written around the compaction (it does not advance its index on the free
 * arm and re-reads the count every iteration).
 *
 * ResolveEntrancePathSquare rebuilds path-square CONNECTIVITY from the park
 * entrance: it clears flag 2 ("reachable") on every square in pathsq.c's list,
 * then flood-fills that flag out from the square containing `at` through
 * 0x004829c0, which recurses over the neighbour set 0x004819a0 collects.
 * tinystubs.c's RefreshEntranceTile is the caller.
 */
#include "legoland.h"

/* ---- types -------------------------------------------------------------- */

/* An inclusive rectangle with no list link (pathmisc2.c's Rect4). */
typedef struct Rect4 {
    int left;    /* +0x00 */
    int top;     /* +0x04 */
    int right;   /* +0x08 */
    int bottom;  /* +0x0c */
} Rect4;

/* One path square (pathsq.c's PathSquare, 0x24 bytes); only the link and the
 * flags word are touched here. */
typedef struct PathSquare {
    struct PathSquare* next;   /* +0x00 */
    int   pad04;               /* +0x04 */
    Rect4 rect;                /* +0x08 */
    void* rect_next;           /* +0x18 */
    int   distance2;           /* +0x1c */
    int   flags;               /* +0x20 bit 1 = reachable from the entrance */
} PathSquare;

/* One rendered-text cache entry (fpui3.c / render5.c's TextEntry, 0x20). */
typedef struct TextEntry {
    int   w;          /* +0x00 */
    int   h;          /* +0x04 */
    int   format;     /* +0x08 */
    char* text;       /* +0x0c the malloc'd copy this file frees */
    int   ink;        /* +0x10 */
    int   paper;      /* +0x14 */
    int   font;       /* +0x18 */
    void* sprite;     /* +0x1c the rendered bitmap */
} TextEntry;

/* ---- globals ------------------------------------------------------------ */
extern PathSquare* g_path_squares;                       /* 0x0066b44c */
extern volatile int g_text_cache_count;                  /* 0x006675b8 */
extern TextEntry   g_text_cache[];                       /* 0x006675c0 */
extern const char  kFreeTextFmt[];                       /* 0x004b9098 */

/* ---- callees ------------------------------------------------------------ */
extern PathSquare* FindPathSquare(Pos* pos);             /* 0x00481790 */
/* 0x004829c0 (not exported): set flag 2 on `sq` and recurse into every
 * neighbour square that does not carry it yet — the entrance flood fill. */
extern void MarkPathSquareReachable(PathSquare* sq);     /* 0x004829c0 */
/* pathmisc.c's predicate, applied to a COPY of the cell. */
extern int  IsPathCell(Cell* cell);                      /* 0x0045ce10 */
/* 0x00460f50 (not exported): render5.c's DrawPathTileOverlay with the corner
 * draws routed through the five-argument PrintSprite so a blit mode gets
 * through.  Declared here the way the disassembly pushes it: four arguments. */
extern void DrawCursorPathTile(Pos* at, int x, int y, int mode); /* 0x00460f50 */
extern int  DBPrintf(const char* format, ...);           /* 0x00453a20 */
extern void HeapFree_w(void* p);                         /* 0x0049e4d0 */
extern void KillSprite(void* sprite);                    /* 0x00497bd0 */


/* =========================================================================
 *  ResolveEntrancePathSquare — re-flood the entrance's reachable set
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00482a40
void ResolveEntrancePathSquare(Pos* at)
{
    PathSquare* sq = g_path_squares;

    while (sq) {
        sq->flags &= ~2;
        sq = sq->next;
    }

    sq = FindPathSquare(at);
    if (sq)
        MarkPathSquareReachable(sq);
}


/* =========================================================================
 *  FreeCachedTextEntry — drop one rendered-text cache entry and compact
 * ========================================================================= */

/* render5.c's ExpireCachedText is the only caller; the compaction here is why
 * it does NOT advance its index on the free arm and re-reads the count on
 * every iteration.
 *
 * The count is decremented BEFORE the text is freed and before the sprite is
 * killed, so the compaction loop's bound is already the NEW count and the
 * final entry is left as a stale duplicate of its predecessor.  All four
 * arguments of the trace and of the free share ONE `add esp,0x10`: nothing
 * between the two calls consumes a stack result.
 *
 * kFreeTextFmt (0x004b9098) is "Deleting Cell (%d) %s\n".
 *
 * g_text_cache_count is declared VOLATILE here.  Without it VC6 schedules the
 * `.text` reload for the free ABOVE the count's store (13 before 12) and
 * hoists the loop's reload out of the latch; the original re-reads the global
 * in the latch and keeps the store in place, and one volatile declaration
 * fixes both at zero instruction cost (the read-modify-write and the two
 * reloads are exactly what the original emits).  render5.c reaches the same
 * behaviour from its side with a `*(volatile int*)&` cast at the one read it
 * needs -- the divergence is deliberate. */
// FUNCTION: LEGOLAND 0x00455ee0
void FreeCachedTextEntry(int i)
{
    int j;

    DBPrintf(kFreeTextFmt, i, g_text_cache[i].text);
    g_text_cache_count--;
    HeapFree_w(g_text_cache[i].text);
    if (g_text_cache[i].sprite) {
        KillSprite(g_text_cache[i].sprite);
        g_text_cache[i].sprite = 0;
    }
    for (j = i; j < g_text_cache_count; j++)
        g_text_cache[j] = g_text_cache[j + 1];
}


/* The bounds-checked cell fetch every map accessor open-codes; the LAZY form
 * (pos->y read only after pos->x has passed its bounds test) is the one this
 * file's callers emit — pathmisc2.c spells the same helper CellForPos. */
static __inline Cell* CellForPos(Pos* pos)
{
    int x = pos->x;
    int y;
    if (x >= 0 && x < g_map->width) {
        y = pos->y;
        if (y >= 0 && y < g_map->height) return &g_map_rows[y][x];
    }
    return 0;
}


/* =========================================================================
 *  DrawCursorTileAt — the build cursor's per-cell path filter
 * ========================================================================= */

/* render5.c's PaintCursorTiles hands every cell of the diamond under the
 * cursor rectangle to this; only an on-map cell that IsPathCell accepts AND
 * that carries a non-zero displayed tile is painted, through the four-argument
 * overlay twin at 0x00460f50 with the caller's blit mode.
 *
 * The CALLER'S Pos is what goes on, not the fetched cell — the painter does
 * its own lookup.  `cell != 0` survives as a real test even though the inline
 * has already branched to the same exit on every out-of-bounds arm: VC6 leaves
 * the merged null test in (`test esi,esi / je`), so the three tests are three
 * source `&&` terms, not two. */
// FUNCTION: LEGOLAND 0x00461080
void DrawCursorTileAt(Pos* at, int x, int y, int mode)
{
    Cell* cell = CellForPos(at);

    if (cell && IsPathCell(cell) && cell->tile != 0)
        DrawCursorPathTile(at, x, y, mode);
}


/* =========================================================================
 *  IsPathRectClear — is every cell of an inclusive rectangle free path?
 * ========================================================================= */

/* pathmisc2.c's GrowPathRectSide builds a one-cell-thick edge strip and asks
 * this whether the rectangle may grow that way.  Every cell must carry map
 * flag 0x10 (a path tile) and must NOT carry RF bit 1 (blocked) — note this
 * is NOT IsPathCell's predicate: RF bit 0 does not rescue a cell here.  An
 * EMPTY rectangle (left > right, or top > bottom) answers 1.
 *
 * The off-map stand-in cell is the same one simcore.c's GetPathNeighbours
 * uses, but its `tile = 0` store is DEAD here — the copy's address is never
 * taken, because the two flag bytes are read straight out of the local — and
 * VC6 drops it, leaving only the flags and rf stores.  That is the visible
 * difference from PathEdgeMask/PathCornerMask below, which pass &cell to a
 * real call and keep all three.
 *
 * `x` is the OUTER loop: the cell stride is x's, so VC6 strength-reduces
 * x * 0x14 and does the `x >= 0` test on the offset (`test edx,edx / jl`),
 * exactly as fpui4.c records.
 *
 * RESIDUAL (68 of 70; ours 72 instructions, first diverging index 5, class
 * STRUCTURAL — one derived induction variable).  Everything is the original's
 * except the ROW TABLE's base: the original hoists `g_map_rows` into EBX in
 * the OUTER preheader and re-indexes it every inner iteration
 * (`mov esi,[ebx+eax*4]`), while VC6 gives us the load in the INNER preheader
 * plus a derived IV over the row pointers (`mov edx,[g_map_rows] /
 * lea edx,[edx+eax*4]` … `mov esi,[edx]` … `add edx,4`) — two instructions
 * more, and it takes the register the original spends on the base, which is
 * why x lands in EBX here and in EBP there.  RULED OUT, every one of them
 * still exactly 72 instructions with the same first divergence: a `Cell** rows
 * = g_map_rows;` local (copy-propagated straight back to the global, 73),
 * `*(rows[y] + x)`, `(char*)rows[y] + x*20`, `(char*)g_map_rows[y] +
 * x*sizeof(Cell)`, a named `Cell* row` / `Cell* c` (73), a manually spelled
 * `xoff` outer IV with the `x >= 0` test written on it, while-loops, a
 * do/while outer with the guard split out (74), `cell` declared in the inner
 * scope, dropping the dead `tile = 0` store, both orders of the two failure
 * tests, both orders of the else stores, and the bounds test with y first.
 * A `*(Cell** volatile*)&g_map_rows` read is NOT free here: it anchors above
 * the guard and takes a frame slot (75-76).  A volatile read of `g_map`
 * inside the probe — free, the original reloads it every iteration — does not
 * act as a barrier on the g_map_rows load either; VC6 schedules the
 * g_map_rows load ABOVE it.  Also ruled out: a `Cell** rows` scoped inside
 * the outer loop body and inside the inner loop body (both 73), and the whole
 * probe as a `static __inline` helper taking a `Cell*` out-parameter (72, and
 * making `cell` address-taken does not stop the hoist).  Twenty spellings, all
 * 72 or 73, all diverging at index 5.
 *
 * §6B triage, diff-aligned (not index-aligned, since our two extra
 * instructions shift everything): strict 28, register-blind 17,
 * register+immediate-blind 5 — i.e. the SHAPE is the original's to within the
 * two IV instructions, and the residual is one code-motion decision, not a
 * spelling.  The decision looks like IV-formation profitability: with the row
 * table's base already in a register the IV saves nothing, and nothing
 * reachable from C puts it there.  AT ITS FLOOR unless someone finds a
 * construct that hoists a global load to the OUTER preheader while leaving the
 * inner loop's index unreduced. */
// WIP-FUNCTION: LEGOLAND 0x0045c900  (70i extent, 189B; ours 72i, first
// diverging index 5; STRUCTURAL — one derived induction variable over the row
// table; at its floor, see the note above)
int IsPathRectClear(Rect4* rect)
{
    Cell cell;
    int  x;
    int  y;

    for (x = rect->left; x <= rect->right; x++) {
        for (y = rect->top; y <= rect->bottom; y++) {
            if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height) {
                cell = g_map_rows[y][x];
            } else {
                cell.tile = 0;
                cell.flags = 0x40;
                cell.rf = 0;
            }
            if (!(cell.flags & 0x10) || (cell.rf & 2))
                return 0;
        }
    }
    return 1;
}


/* One neighbour probe: copy the cell into `cell`, with an off-map cell
 * standing in as tile=0 / flags=0x40 / rf=0.  The same macro over one
 * function-level `cell` that simcore.c's GetPathNeighbours uses — a helper
 * with its own local would not keep the off-map stores alive.  Here all THREE
 * stores survive in every probe, because &cell goes on to a real call. */
#define PROBE(px, py) \
    if ((px) >= 0 && (px) < g_map->width && (py) >= 0 && (py) < g_map->height) { \
        cell = g_map_rows[(py)][(px)]; \
    } else { \
        cell.tile = 0; \
        cell.flags = 0x40; \
        cell.rf = 0; \
    }


/* =========================================================================
 *  PathCornerMask — which inside corners of a path cell need a filler
 * ========================================================================= */

/* Called with the mask PathEdgeMask has just answered.  A corner filler is
 * wanted only where a DIAGONAL PAIR of the four-neighbour mask is complete and
 * the cell diagonally beyond that pair is NOT itself path — an inside corner
 * with a hole in it:
 *
 *      (edges & 0xc) == 0xc  (S and W)  and (x-1, y+1) not path  ->  0x1
 *      (edges & 0x3) == 0x3  (N and E)  and (x+1, y-1) not path  ->  0x2
 *
 * `edges` is a CHAR parameter (`mov bl,[esp+0x20]`, then `mov al,bl / and
 * al,0xc / cmp al,0xc` — byte-wide throughout), which is what lets render5.c's
 * caller push the raw dword out of its own byte local's home; declaring it int
 * would make the caller mask before the push.  ebx holds `edges` for the whole
 * body, so unlike PathEdgeMask there is no zero register and the off-map
 * stores use immediates.
 *
 * The second arm's two exits share one epilogue: `mov al,[corner]` is emitted
 * before the branch so the "already path" arm can jump straight into the pops,
 * and the `or al,2` sits between them in the fall-through copy. */
// FUNCTION: LEGOLAND 0x0045d080
char PathCornerMask(char edges, Pos* at)
{
    Cell cell;
    char corner = 0;
    int  px;
    int  py;

    if ((edges & 0xc) == 0xc) {
        px = at->x - 1;
        py = at->y + 1;
        PROBE(px, py)
        if (!IsPathCell(&cell))
            corner = 1;
    }
    if ((edges & 3) == 3) {
        px = at->x + 1;
        py = at->y - 1;
        PROBE(px, py)
        if (!IsPathCell(&cell))
            corner |= 2;
    }
    return corner;
}


/* =========================================================================
 *  PathEdgeMask — the four-neighbour path connection mask of a cell
 * ========================================================================= */

/* N, E, S, W in that order, one bit each (0x1, 0x2, 0x4, 0x8), through the
 * same probe and the same real IsPathCell call.  The answer is a CHAR local in
 * the frame (`mov byte ptr [esp+0x13]`), which is what lets render5.c's caller
 * home it in a dead parameter slot and push the whole stale dword on.  The
 * first hit is a `mov …,1` rather than an `or` — VC6 knows the mask is still
 * zero there — and the last is folded into the epilogue as `or al,8` after
 * the register copy, so only bits 2 and 4 are read-modify-writes in memory.
 *
 * ebx is the zero register (it is pushed anyway), so `>= 0` compares come out
 * as `cmp reg,ebx` and every off-map zero store as `bl`/`bx`. */
// FUNCTION: LEGOLAND 0x0045ceb0
char PathEdgeMask(Pos* at)
{
    Cell cell;
    char mask = 0;
    int  px;
    int  py;

    px = at->x;
    py = at->y - 1;
    PROBE(px, py)
    if (IsPathCell(&cell))
        mask |= 1;

    px = at->x + 1;
    py = at->y;
    PROBE(px, py)
    if (IsPathCell(&cell))
        mask |= 2;

    px = at->x;
    py = at->y + 1;
    PROBE(px, py)
    if (IsPathCell(&cell))
        mask |= 4;

    px = at->x - 1;
    py = at->y;
    PROBE(px, py)
    if (IsPathCell(&cell))
        mask |= 8;

    return mask;
}
