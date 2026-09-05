/* LEGOLAND -- the TERRAIN TILE PAINTER and two render-order helpers.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are declared LOCALLY on purpose (legoland.h is owned
 * elsewhere) and mirror renderview.c (MapHdr / Cell / the projection),
 * objmap2.c and render3.c (the render-node table) and sysmisc3.c (the
 * caller, RenderGroundLayer).
 *
 *   addr        what it is                                       insns
 *   addr        what it is                                    insns state
 *   0x00461220  DrawEditCursor        paint the cursor's tiles   32   [OK]
 *   0x0045a430  TakeRenderNodeByPos   take a node by base cell   34   [OK]
 *   0x004608c0  PaintTileLayer        the terrain grid painter  431   [WIP]
 *
 * EXTERN TYPE NOTES (caller-side levers -- nothing else is aligned to them)
 *  - 0x00485f00 is NEW: `PrintSpriteXY(void* s, int x, int y)`, the
 *    three-argument wrapper that forwards to PrintSprite(s, x, y, 0, 0).
 *    0x00460e90 is likewise newly named DrawPathTileOverlay and 0x004610f0
 *    PaintCursorTiles (the cursor twin of PaintTileLayer's grid walk).
 *  - g_bridge_sets (0x00832bdc) is declared here as an array of pointers
 *    indexed by a bridge object's frame HIGH byte; power.c and powerhelp.c
 *    declare the same address as the scalar `g_power_unserved_n`, the last
 *    of the five power accumulators at 0x00832bcc..0x00832bdc.  Only index
 *    >= 1 is ever reached from here (the high byte is non-zero by the time
 *    the table is read), so the two views do not overlap in practice; both
 *    are left as they are.
 * ========================================================================= */

/* ------------------------------------------------------- shared map types -- */
typedef struct Pos { int x; int y; } Pos;
typedef struct WinRect { int left; int top; int right; int bottom; } WinRect;

/* renderview.c's MapHdr (the record at 0x004bcbf4, lpConfig). */
typedef struct MapHdr {
    unsigned char  pad00[0x10];
    unsigned short view_w;          /* +0x10 view rectangle size, pixels */
    unsigned short view_h;          /* +0x12 */
    unsigned short width;           /* +0x14 map size, cells */
    unsigned short height;          /* +0x16 */
    unsigned char  pad18[0x20 - 0x18];
    unsigned short origin_x;        /* +0x20 view rectangle origin, pixels */
    unsigned short origin_y;        /* +0x22 */
} MapHdr;

extern MapHdr* g_map;               /* 0x004bcbf4 */
extern int     g_scroll_x;          /* 0x00667cb4  ScrollX, 24.8 */
extern int     g_scroll_y;          /* 0x00667cb8  ScrollY, 24.8 */

/* =========================================================================
 * 0x0045a430 -- TakeRenderNodeByPos.
 *
 * objmap2.c's render-order pass leaves a 0x1000-entry table of {live, bpos,
 * x, y} nodes at 0x00807f60: one per base cell, recording the column and row
 * the column scan must RESUME at once that object has been threaded into the
 * render chain.  This is the consumer: given a packed base cell, find its
 * live node, hand the resume point back and retire the node.
 *
 * When there is no node for that cell the resume point is the map's own
 * (width, height) -- i.e. past the last column and row, which stops the scan
 * dead rather than looping.  The two u16 extent fields are read through TWO
 * separate loads of the map-header pointer, because the store through the
 * caller's `out` between them may alias it.
 *
 * The walk runs a strength-reduced cursor anchored on `.bpos` (+4) -- the
 * recorded "anchor at the field with the MOST references, ties to the LAST,
 * offset 0 never wins" rule, with `live` and `bpos` referenced once each --
 * while the plain induction variable survives for the hit block's three
 * subscripted accesses.
 * ========================================================================= */

/* One render-order node (0x00807f60, 0x1000 entries of 8 bytes). */
typedef struct RenderNode {
    int            live;            /* +0x00 */
    unsigned short bpos;            /* +0x04 packed base cell */
    unsigned char  x;               /* +0x06 column to resume at */
    unsigned char  y;               /* +0x07 row to resume at */
} RenderNode;

extern RenderNode g_render_nodes[0x1000];   /* 0x00807f60 */

// FUNCTION: LEGOLAND 0x0045a430
void TakeRenderNodeByPos(unsigned short bpos, Pos* out)
{
    int i;

    for (i = 0; i < 0x1000; i++) {
        if (g_render_nodes[i].live && g_render_nodes[i].bpos == bpos) {
            out->x = g_render_nodes[i].x;
            out->y = g_render_nodes[i].y;
            g_render_nodes[i].live = 0;
            return;
        }
    }
    out->x = g_map->width;
    out->y = g_map->height;
}

/* =========================================================================
 * 0x00461220 -- DrawEditCursor.
 *
 * renderview.c's RenderView calls this between
 * PushRenderingStatusAndLockVideoSurface and FlushCursorSpriteList, so it is
 * the pass that stamps the placement cursor's tiles over the finished frame.
 * The body is sysmisc3.c's RenderGroundLayer (0x00460e00) minus the trailing
 * dirty-flag stores: the same scroll `Pos` and the same view `WinRect` built
 * from the map header's origin and view size, handed to the layer painter.
 * The two right/bottom values are computed by READING BACK view.left and
 * view.top -- the recorded corollary that two reads of the same u16 global
 * field do not CSE across a store to an address-taken aggregate.
 * ========================================================================= */

/* Paint the cursor's tile layer for one view rectangle (unexported). */
extern void PaintCursorTiles(Pos* scroll, WinRect* view);  /* 0x004610f0 */

// FUNCTION: LEGOLAND 0x00461220
void DrawEditCursor(void)
{
    Pos     scroll;
    WinRect view;

    scroll.x = g_scroll_x >> 8;
    scroll.y = g_scroll_y >> 8;
    view.left = g_map->origin_x;
    view.top = g_map->origin_y;
    view.right = g_map->view_w + view.left;
    view.bottom = g_map->view_h + view.top;
    PaintCursorTiles(&scroll, &view);
}

/* =========================================================================
 * 0x004608c0 -- PaintTileLayer: the terrain grid painter.
 * =========================================================================
 * sysmisc3.c's RenderGroundLayer hands this a scroll position and a view
 * rectangle; it paints EVERY terrain tile of the view and then the perimeter
 * cliff/bridge objects, between one PushRenderingStatusAndLockVideoSurface
 * and its Pop.  It is the bottom layer of renderview.c's RenderView -- the
 * objects, people and cursor are all drawn over it.
 *
 * THE WALK.  The tile grid is the standard 2:1 isometric diamond
 * (renderview.c's header): th = g_tile_sprites[g_default_tile]->h,
 * tw = 2*th, and the two halves of a diamond row are half a tile apart in
 * both axes.  From the scroll position the painter derives the map cell the
 * top-left of the view lands on --
 *      q    = scroll->x / tw            xrem = scroll->x % tw
 *      r    = (scroll->y - (th+1)/2) / th    yrem = the same % th
 *      tile = { r + q - 3, r - q }
 * -- and then CORRECTS it by one cell, because the cell the division names is
 * only right for the middle of the diamond.  Which correction depends on
 * which QUARTER of the diamond (xrem, yrem) fell in, and that is the
 * four-arm switch: the selector is 1 + (xrem >= halfw) + 2*(yrem > halfh),
 * and each arm re-tests the point against that quarter's diagonal
 * (halfw -+ 2*yrem, or halfw + 2*(yrem - th)) before moving the cell one
 * step and folding half a tile back into the remainders.  The -3 is the two
 * tiles of bleed the loops start with.
 *
 * The loops then run rows of DIAMONDS, two cells per step:
 *      for (py = view->top - 2*th - yrem; py < view->bottom + 2*th; py += th)
 *          for (px = view->left - 2*tw - xrem; px < view->right + 2*tw;
 *               px += tw)
 * with tile.x++ between the two cells of a step, tile.y-- at the end of the
 * step, and (tile.x, tile.y) reset to (rowx + 1, rowy + 1) per row.  The
 * second cell of a step is the NEXT cell in the same map row, so the painter
 * takes it by pointer arithmetic (`cell + 1`) rather than a second bounds
 * check -- but only when the first fetch succeeded and the column has not
 * run off the right edge, which is also the only way out of the column loop
 * other than px reaching the limit.
 *
 * PER CELL, when the cell exists and its displayed tile is non-zero:
 *   - a cell that carries BOTH map flags 0x0001|0x0002 and 0x0008 and holds
 *     an object whose CLASS has a custom draw callback (ObjDef +0xa0, the
 *     slot renderview.c documents) is skipped entirely -- the object pass
 *     will paint it;
 *   - the same cell with NO callback gets its tile painted and nothing else;
 *   - every other cell gets its tile painted and, if it is a path cell
 *     (0x0045ce10), the path overlay (0x00460e90) stamped over it.  That
 *     overlay takes the tile POSITION BY ADDRESS, which is why the two cell
 *     coordinates are one address-taken `Pos` at the top of the frame and
 *     are reloaded after every call.
 *
 * THE PERIMETER PASS.  The list at 0x00667ca8 (loadmap.c's
 * BuildPerimeterObject, bound to sprites by renderinit.c's
 * BindTerrainObjectSprites) is walked last: each node's cached bank entry is
 * printed at its world position less the scroll-to-view delta computed at
 * the very top of the function.  A node whose packed frame id has a NON-ZERO
 * HIGH BYTE is a BRIDGE piece, and its low byte says which half (0 or 1);
 * for those, and only when the bridge group's slot in the table at
 * 0x00832bdc is loaded, two more sprites are drawn from the bridge bank
 * (0x00667cb0): entry (half + 2) at renderinit.c's per-theme half offsets,
 * and entry (half + 4) -- if the bank is that big -- SORTED into the object
 * list at the half's shadow offsets with the sprite's own bottom edge as the
 * sort key, so a bridge's companion piece can be occluded by whatever walks
 * over it.  Anything but half 0 or 1 draws only the base sprite.
 *
 * ORIGINAL BUG REPRODUCED.  The two cells of a diamond step are two textual
 * copies of the same block, and the SECOND copy's "object cell with no draw
 * callback" arm was never updated: it paints at (px, py) where every other
 * draw in that copy paints at (px + halfw, py + halfh).  So such a tile is
 * stamped on top of its diamond neighbour.  It is rare (the cell must carry
 * flags 0x3 and 0x8 AND hold an object AND that object's class must have no
 * +0xa0 callback) and both arms of the FIRST copy are correct, which is why
 * the map looks right.  Reproduced as found.
 * ========================================================================= */

typedef struct ObjDef {
    unsigned char pad00[0xa0];
    void*         draw;             /* +0xa0 custom draw callback */
} ObjDef;

typedef struct Obj {
    unsigned char pad00[0x0c];
    ObjDef*       def;              /* +0x0c */
} Obj;

typedef struct Cell {
    Obj*           obj;             /* +0x00 */
    unsigned char  pad04[4];
    unsigned short tile;            /* +0x08 */
    unsigned char  pad0a[2];
    unsigned short flags;           /* +0x0c */
    unsigned char  pad0e[0x14 - 0x0e];
} Cell;                             /* 0x14 */

typedef struct Sprite { unsigned char pad00[0x16]; short h; } Sprite;

typedef struct BankEntry BankEntry;
typedef struct SpriteBank {
    int          pad00;
    int          count;             /* +0x04 */
    BankEntry**  entries;           /* +0x08 */
} SpriteBank;

typedef struct TerrainObj {
    unsigned char      pad00[0x10];
    int                frame;       /* +0x10 */
    int                sx;          /* +0x14 */
    int                sy;          /* +0x18 */
    struct TerrainObj* next;        /* +0x1c */
    BankEntry*         sprite;      /* +0x20 */
} TerrainObj;

extern MapHdr*      g_map;                  /* 0x004bcbf4 */
extern Cell**       g_map_rows;             /* 0x00801400 */
extern Sprite*      g_tile_sprites[];       /* 0x00805f60 */
extern int          g_default_tile;         /* 0x00667ca4 */
extern TerrainObj*  g_terrain_objects;      /* 0x00667ca8 */
extern SpriteBank*  g_bridge_bank;          /* 0x00667cb0 */
extern void*        g_bridge_sets[];        /* 0x00832bdc */
extern int g_bridge_half1_ox;               /* 0x004b9210 */
extern int g_bridge_half1_oy;               /* 0x004b9214 */
extern int g_bridge_half0_ox;               /* 0x004b9218 */
extern int g_bridge_half0_oy;               /* 0x004b921c */
extern int g_bridge_half1_shadow_ox;        /* 0x00801a60 */
extern int g_bridge_half1_shadow_oy;        /* 0x00801a64 */
extern int g_bridge_half0_shadow_ox;        /* 0x00805f40 */
extern int g_bridge_half0_shadow_oy;        /* 0x00805f44 */

extern void SetClipping(WinRect* r);                        /* 0x0048a5c0 */
extern void PushRenderingStatusAndLockVideoSurface(void);   /* 0x00463fc0 */
extern void PopRenderingStatus(void);                       /* 0x004641f0 */
extern int  PrintSpriteXY(void* s, int x, int y);           /* 0x00485f00 */
extern int  IsPathCell(Cell* c);                            /* 0x0045ce10 */
extern void DrawPathTileOverlay(Pos* tile, int x, int y, int mode); /* 0x00460e90 */
extern void SortSprite(void* sprite, int x, int y, int key, int mode, void* ctx); /* 0x00485d70 */

static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}


/* STATE: 431/431 instructions and the whole block layout -- the four switch
 * arms and their .rdata table, both written-out cell copies, the `cell + 1`
 * fast path with its re-test, the shared draw tails, the two bridge halves
 * cross-jumped into one SortSprite call and the frame's 0x34 bytes with the
 * address-taken `tile` Pos at the top and `xrem` in the dead `scroll`
 * argument slot -- are all reproduced.  1301 of 1315 bytes; the residual is
 * an ALLOCATION permutation, not a structural one: the original runs
 * {tile.x: ecx, tile.y: ebx, px: edi} where we get {edi, edx, ebx}, and `tw`
 * takes ebp there and ecx here (which is also the whole `lea ecx,[ebp+1]`
 * versus `inc ecx` and `mov ebx,[th] / idiv ebx` versus `idiv [th]`
 * difference in the head).  Everything after index 27 is shifted by that.
 *
 * ONE LEVER FOUND AND KEPT, and it is worth recording: the free volatile read
 * on `halfw`.  Written `px + halfw`, VC6 strength-reduces the second cell's x
 * into its OWN derived induction variable with its own frame slot, which
 * makes the frame 0x38 and adds `add <iv>,tw` to the column latch; the
 * original reloads halfw from its home at that site instead.  The volatile
 * read is free (the original's `mov ecx,[esp+0x20]` is exactly it), it kills
 * the derived IV and it takes the frame back to the original's 0x34.  Its
 * price is that halfw's home moves from 0x20 to 0x28 and the two view limits
 * slide down with it -- so if a future round finds a NON-volatile way to
 * refuse that induction variable, the three slots come back in order too.
 * Measured and rejected: `halfw + px` and a named `x2` local (IV returns, 408
 * at 1313 bytes, frame 0x38), a volatile step `px += *(volatile int*)&tw`
 * (415), naming `scroll->y - halfh` (392), halfh before halfw (390),
 * `tile.y` assigned before `tile.x` (391), the row saves and restores in the
 * other order (389/390) and `tile.y = tile.y - 1` (389, byte-identical).
 * The `lea ecx,[eax+eax]` for tw needs `tw` DEFINED BEFORE `th` in the
 * source (the reversed-derived-pair rule); with th first VC6 doubles in
 * place and the whole head is one form off. */
// WIP-FUNCTION: LEGOLAND 0x004608c0  (431/431 insns, 1301/1315 bytes, 389 mismatches from index 27; block layout exact, residual is the callee-saved permutation described above)
void PaintTileLayer(Pos* scroll, WinRect* view)
{
    TerrainObj* e = g_terrain_objects;
    Pos    tile;
    Cell*  cell;
    int    dx;
    int    dy;
    int    tw;
    int    th;
    int    halfw;
    int    halfh;
    int    q;
    int    r;
    int    xrem;
    int    yrem;
    int    px;
    int    py;
    int    xlimit;
    int    ylimit;
    int    rowx;
    int    rowy;
    int    sel;

    dx = scroll->x - view->left;
    dy = scroll->y - view->top;
    SetClipping(view);
    tw = (short)(g_tile_sprites[g_default_tile]->h * 2);
    th = g_tile_sprites[g_default_tile]->h;
    q = scroll->x / tw;
    halfw = (tw + 1) >> 1;
    halfh = (th + 1) >> 1;
    xrem = scroll->x % tw;
    r = (scroll->y - halfh) / th;
    yrem = (scroll->y - halfh) % th;
    tile.x = r + q - 3;
    tile.y = r - q;
    sel = (xrem >= halfw) + 1;
    if (yrem > halfh)
        sel += 2;
    switch (sel) {
    case 1:
        if (xrem < halfw - 2 * yrem) {
            tile.x--;
            xrem += halfw;
            yrem += halfh;
        }
        break;
    case 2:
        if (xrem >= halfw + 2 * yrem) {
            tile.y--;
            xrem -= halfw;
            yrem += halfh;
        }
        break;
    case 3:
        if (xrem < halfw + 2 * (yrem - th)) {
            tile.y++;
            xrem += halfw;
            yrem -= halfh;
        }
        break;
    case 4:
        if (xrem >= halfw + 2 * (th - yrem)) {
            tile.x++;
            xrem -= halfw;
            yrem -= halfh;
        }
        break;
    }
    ylimit = view->bottom + 2 * th;
    xlimit = view->right + 2 * tw;
    PushRenderingStatusAndLockVideoSurface();
    for (py = view->top - 2 * th - yrem; py < ylimit; py += th) {
        rowx = tile.x;
        rowy = tile.y;
        for (px = view->left - 2 * tw - xrem; px < xlimit; px += tw) {
            cell = MapCellAt(tile.x, tile.y);
            if (cell != 0) {
                unsigned short t = cell->tile;

                if (t != 0) {
                    if ((cell->flags & 3) && (cell->flags & 8) && cell->obj != 0) {
                        if (cell->obj->def->draw == 0)
                            PrintSpriteXY(g_tile_sprites[t], px, py);
                    } else {
                        PrintSpriteXY(g_tile_sprites[t], px, py);
                        if (IsPathCell(cell))
                            DrawPathTileOverlay(&tile, px, py, 0);
                    }
                }
            }
            tile.x++;
            if (cell != 0) {
                if (tile.x == g_map->width)
                    break;
                cell++;
            }
            if (cell == 0)
                cell = MapCellAt(tile.x, tile.y);
            if (cell != 0) {
                unsigned short t = cell->tile;

                if (t != 0) {
                    if ((cell->flags & 3) && (cell->flags & 8) && cell->obj != 0) {
                        if (cell->obj->def->draw == 0)
                            PrintSpriteXY(g_tile_sprites[t], px, py);
                    } else {
                        int x2 = px + *(volatile int*)&halfw;

                        PrintSpriteXY(g_tile_sprites[t], x2, py + halfh);
                        if (IsPathCell(cell))
                            DrawPathTileOverlay(&tile, x2, py + halfh, 0);
                    }
                }
            }
            tile.y--;
        }
        tile.x = rowx + 1;
        tile.y = rowy + 1;
    }
    while (e != 0) {
        if (e->sprite != 0) {
            int x;
            int y;
            int k;

            PrintSpriteXY(e->sprite, e->sx - dx, e->sy - dy);
            if ((e->frame & 0xff00) && g_bridge_sets[e->frame >> 8] != 0) {
                if ((e->frame & 0xff) == 0) {
                    x = e->sx - dx;
                    y = e->sy - dy;
                    PrintSpriteXY(g_bridge_bank->entries[(e->frame + 2) & 0xff],
                                  x + g_bridge_half0_ox, y + g_bridge_half0_oy);
                    k = (e->frame + 4) & 0xff;
                    if (g_bridge_bank->count > k) {
                        BankEntry* s2 = g_bridge_bank->entries[k];

                        SortSprite(s2, x + g_bridge_half0_shadow_ox,
                                   y + g_bridge_half0_shadow_oy,
                                   y + ((Sprite*)s2)->h + g_bridge_half0_shadow_oy,
                                   0, 0);
                    }
                } else if ((e->frame & 0xff) == 1) {
                    x = e->sx - dx;
                    y = e->sy - dy;
                    PrintSpriteXY(g_bridge_bank->entries[(e->frame + 2) & 0xff],
                                  x + g_bridge_half1_ox, y + g_bridge_half1_oy);
                    k = (e->frame + 4) & 0xff;
                    if (g_bridge_bank->count > k) {
                        BankEntry* s2 = g_bridge_bank->entries[k];

                        SortSprite(s2, x + g_bridge_half1_shadow_ox,
                                   y + g_bridge_half1_shadow_oy,
                                   y + ((Sprite*)s2)->h + g_bridge_half1_shadow_oy,
                                   0, 0);
                    }
                }
            }
        }
        e = e->next;
    }
    PopRenderingStatus();
}
