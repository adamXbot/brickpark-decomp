/* LEGOLAND — path-tile shaping, screen->map projection and mouse scrolling.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; names are ours.
 *
 * ---------------------------------------------------------------------------
 * THE 8-NEIGHBOUR PATH MASK
 *
 * A path tile's shape is decided from which of its eight neighbours are
 * themselves walkable path (the same predicate SetPathFlag uses:
 * (rf & 1) || ((flags & 0x10) && !(rf & 2)), evaluated on a COPY of the cell;
 * an off-map cell is treated as flags=0x40 / rf=0, i.e. never path).  The
 * helper at 0x0045c440 walks them in the order N, S, E, W, then the diagonals,
 * ORing one bit per walkable neighbour into a byte and counting them:
 *
 *        bit 0 = N  (x,   y-1)      bit 1 = NE (x+1, y-1)
 *        bit 2 = E  (x+1, y  )      bit 3 = SE (x+1, y+1)
 *        bit 4 = S  (x,   y+1)      bit 5 = SW (x-1, y+1)
 *        bit 6 = W  (x-1, y  )      bit 7 = NW (x-1, y-1)
 *
 * so the orthogonals are the EVEN bits and the diagonals the ODD bits, each
 * diagonal sitting between the two orthogonals it touches.  That is what
 * ExcludeIsolatedDiags relies on: a diagonal neighbour only counts when BOTH
 * flanking orthogonals are path (NE needs N and E: (m & 0x05) == 0x05, and so
 * on round the compass with 0x14, 0x50, 0x41), otherwise its bit is dropped.
 *
 * The helper also hands back two counts through its pointer arguments (each
 * skipped when null): how many ORTHOGONAL neighbours are path and how many
 * DIAGONAL ones.  AdjustTileRFFlags classifies the cell from the orthogonal
 * count alone and records the shape in the RF byte:
 *
 *        0 or 1 neighbours  -> RF 0x10  dead end / isolated stub
 *        2, exactly one of N/S ((mask & 0x11) == 0x01 or 0x10)
 *                           -> RF 0x08  corner  (both or neither = a straight
 *                                       run, which gets no shape bit)
 *        3 neighbours       -> RF 0x04  T-junction
 *        4 neighbours       -> RF 0x20  crossroads
 *
 * RF bits 0x3c are cleared first (rf &= 0xc3) so the shape is rebuilt from
 * scratch; RF bits 0 (walkable), 1 (blocked), 6 and 7 are preserved.  The
 * function nominally returns the new RF byte (the `return cell->rf |= x`
 * form is what produces the load/or/store through al in those arms) but the
 * crossroads and straight-run paths fall off the end with no value — a bug in
 * the original, reproduced; no caller reads the result.
 *
 * AdjustPathTile is the per-cell driver that UpdatePathNeighbours (pathsq.c)
 * and RemovePathTile call for a cell and its eight neighbours: it marks the
 * path layer dirty, re-shapes the cell if it is walkable path, and clears map
 * flag bits 0..1 on any cell that is path.  Its second parameter (the path
 * tile code every caller passes) is never read.
 *
 * SCREEN -> MAP
 *
 * ScreenToMapRef2 is the inverse of GetTileCentre (tilehelp.c).  With the
 * default ground tile h = sprite->h and w = 2h, the forward projection is
 *      sx = (w/2)*(tx - ty) - scroll_x + origin_x
 *      sy = (h/2)*(tx + ty) - scroll_y + origin_y   (+h/2 for the centre)
 * so  u = (sx + scroll_x - origin_x + w/2) * (256/w)  = 128*(tx - ty) + 128
 *     v = (sy + scroll_y - origin_y)       * (256/h)  = 128*(tx + ty)
 * and out.x = v + u = 256*tx (+128), out.y = v - u = 256*ty (-128): the map
 * position in the walkers' 24.8 world units (256 per tile), the half-tile
 * bias landing the point on the diamond's centre line.  It returns -1 when no
 * default tile sprite is loaded and 0 otherwise.
 *
 * GetTileInDir steps a 24.8 world position one whole tile (0x100) along a
 * bloke heading 0..7:  0=NW 1=N 2=NE 3=E 4=SE 5=S 6=SW 7=W (y grows south,
 * x grows east), returning the new {x,y} pair in eax:edx.
 *
 * MOUSE SCROLLING
 *
 * MouseScrollMap runs the edge-of-screen autoscroll.  The map header's first
 * eight words are the scroll geometry: screen w/h (+0/+2), the edge margin
 * (+4/+6), the per-tick acceleration (+8/+a) and the speed cap (+c/+e).  A
 * cursor inside the left/top margin decelerates the velocity (0x667cbc /
 * 0x667cc0) down to -cap, inside the right/bottom margin accelerates it up to
 * +cap, and anywhere else zeroes it; the velocity scaled by the frame's tick
 * count (0x6681fc) / 256 is then fed to ProcessScrolling, and the tick count
 * is remembered in 0x667d4c.
 *
 * MAP GRID
 *
 * LoadMapTiles is the grid allocator DECOMP.md had not yet identified: one
 * calloc of 0x14041f bytes holds a 32-byte-aligned table of 256 row pointers
 * (0x400) followed by 256 rows of 256 20-byte cells (0x1400 each); the block
 * is kept in 0x667c9c and only allocated once.  It then resets the tile-slot
 * allocator, reserves slot 0, loads the "MAPPING 1", "BASIC TILES 1" and
 * "NORMAL PATH TILES" elements (the default ground tile is the basic set's
 * base slot; the path tile record pointer 0x832bf0 is the path set's data)
 * and the four scroll-arrow cursors.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* The map header with the fields this file reads: scroll geometry at +0..+e
 * and the viewport origin at +0x20/+0x22. */
typedef struct MapHdr {
    unsigned short screen_w;    /* +0x00 */
    unsigned short screen_h;    /* +0x02 */
    unsigned short edge_x;      /* +0x04 autoscroll margin, pixels */
    unsigned short edge_y;      /* +0x06 */
    unsigned short accel_x;     /* +0x08 velocity step per tick */
    unsigned short accel_y;     /* +0x0a */
    unsigned short vmax_x;      /* +0x0c velocity cap */
    unsigned short vmax_y;      /* +0x0e */
    char           pad10[0x10]; /* +0x10 (+0x14 width, +0x16 height) */
    unsigned short origin_x;    /* +0x20 viewport origin, pixels */
    unsigned short origin_y;    /* +0x22 */
} MapHdr;

/* A tile-set descriptor as the tile-info table points at it: the first dword
 * is the set's base slot in g_tile_sprites (llidb_load.c's TsfDesc). */
typedef struct TileSet {
    int base_slot;              /* +0x00 */
} TileSet;

/* The parallel tile-info table (0x00801f40, stride 8). */
typedef struct TileRec {
    TileSet*       set;         /* +0x00 */
    unsigned short code;        /* +0x04 */
    unsigned short pad6;        /* +0x06 */
} TileRec;
extern TileRec g_tile_recs[];                 /* 0x00801f40 */

/* An LLIDB element as this file needs it: its data at +0x0c. */
typedef struct DbElem {
    char  pad0[0x0c];
    void* data;                 /* +0x0c */
} DbElem;

/* ---------------------------------------------------------------- globals -- */

extern int   g_scroll_x;                      /* 0x00667cb4  8.8 fixed point */
extern int   g_scroll_y;                      /* 0x00667cb8 */
extern int   g_scroll_vx;                     /* 0x00667cbc  autoscroll velocity */
extern int   g_scroll_vy;                     /* 0x00667cc0 */
extern int   g_frame_ticks;                   /* 0x006681fc  ticks this frame */
extern int   g_scroll_ticks;                  /* 0x00667d4c  ticks at last scroll */
extern Pos   g_gfx_point;                     /* 0x00813a44  mouse point */
extern int   g_path_dirty;                    /* 0x004b9220 */
extern int   g_path_overlay_active;           /* 0x00832984 */
extern void* g_path_tile_ptr;                 /* 0x00832bf0 */

extern void* g_map_block;                     /* 0x00667c9c  calloc'd grid */
extern void* g_basic_tiles_data;              /* 0x00801a6c  "BASIC TILES 1" data */
extern int   g_tileset_id0;                   /* 0x008003f8 */
extern int   g_tileset_id1;                   /* 0x00801b20 */
extern int   g_tileset_id2;                   /* 0x0080ff60 */
extern int   g_tileset_id3;                   /* 0x00805f48 */
extern int   g_tileset_id4;                   /* 0x0080ff68 */
extern Sprite* g_arrow1;                      /* 0x00667c8c */
extern Sprite* g_arrow2;                      /* 0x00667c88 */
extern Sprite* g_arrow3;                      /* 0x00667c94 */
extern Sprite* g_arrow4;                      /* 0x00667c90 */

/* ------------------------------------------------------------ prototypes -- */

extern void* calloc(unsigned int n, unsigned int size);
extern void  FreeTileSpace(unsigned short base, unsigned short n);        /* 0x0045aa90 */
extern void* AllocTileSpace(void* desc, unsigned short n, unsigned short* out_base); /* 0x0045a9b0 */
extern int   LLIDB_FindElement(const char* name, DbElem** out, unsigned int* outidx); /* 0x0047b330 */
extern void* LLIDB_LoadData(DbElem* elem);                                /* 0x0047d3a0 */
extern Sprite* LoadSprite(const char* name, int mode);                    /* 0x00497ab0 */
extern int   PrintSprite(Sprite* s, int x, int y, int mode, void* ctx);   /* 0x004853a0 */
extern void  ProcessScrolling(int dx, int dy);                            /* 0x004614a0 */
extern void  RestoreBaseMap(int x, int y);                                /* 0x0045da60 */
extern void  RemovePathSquare(Pos* pos);                                  /* 0x00481c90 */
/* Repaint the 5x5 path area around a cell while the overlay is live. */
extern void  RefreshPathArea(Pos* pos);                                   /* 0x0045cd70 */
/* Is this (copied) cell walkable path? (rf&1) || ((flags&0x10) && !(rf&2)). */
extern int   IsPathCell(Cell* c);                                         /* 0x0045ce10 */
/* Build the 8-neighbour path mask (returned) and, when the pointers are
 * non-null, the number of orthogonal and of diagonal path neighbours. */
extern unsigned char GetPathNeighbours(Pos* pos, unsigned char* ortho, unsigned char* diag); /* 0x0045c440 */

unsigned char AdjustTileRFFlags(Pos* pos);

/* -------------------------------------------------------------- functions -- */

/* Drop every diagonal neighbour bit whose two flanking orthogonals are not
 * both present. */
// FUNCTION: LEGOLAND 0x0045c830
unsigned char ExcludeIsolatedDiags(unsigned char mask)
{
    if ((mask & 0x05) != 0x05)
        mask &= ~0x02;
    if ((mask & 0x41) != 0x41)
        mask &= ~0x80;
    if ((mask & 0x14) != 0x14)
        mask &= ~0x08;
    if ((mask & 0x50) != 0x50)
        mask &= ~0x20;
    return mask;
}

/* Screen pixel -> 24.8 map position (inverse of GetTileCentre). */
// FUNCTION: LEGOLAND 0x0045be00
int ScreenToMapRef2(Pos* screen, Pos* out)
{
    Sprite* s = g_tile_sprites[g_default_tile];
    short h;
    short w;
    int u;
    int v;

    if (s == 0)
        return -1;

    h = s->h;
    w = (short)(h + h);
    u = (((w + 1) >> 1) - ((MapHdr*)g_map)->origin_x + (g_scroll_x >> 8) + screen->x)
        * (256 / w);
    v = ((g_scroll_y >> 8) - ((MapHdr*)g_map)->origin_y + screen->y) * (256 / h);
    out->x = v + u;
    out->y = v - u;
    return 0;
}

/* Step a 24.8 world position one tile along heading `dir` (0..7). */
// FUNCTION: LEGOLAND 0x004846a0
Offset GetTileInDir(Offset p, int dir)
{
    switch (dir & 7) {
    case 1:
        p.oy -= 0x100;
        break;
    case 5:
        p.oy += 0x100;
        break;
    case 3:
        p.ox += 0x100;
        break;
    case 7:
        p.ox -= 0x100;
        break;
    case 2:
        p.oy -= 0x100;
        p.ox += 0x100;
        break;
    case 0:
        p.oy -= 0x100;
        p.ox -= 0x100;
        break;
    case 4:
        p.oy += 0x100;
        p.ox += 0x100;
        break;
    case 6:
        p.oy += 0x100;
        p.ox -= 0x100;
        break;
    }
    return p;
}

/* Draw a path tile at a screen position, then its highlight overlay: the
 * tile set's sprites base+16..base+18 for mode 1..3 (mode 0 = none). */
// FUNCTION: LEGOLAND 0x0045dcf0
void DrawBasicPath(int tile, int x, int y, int mode)
{
    int base;

    PrintSprite(g_tile_sprites[tile], x, y, 0, 0);
    base = g_tile_recs[tile].set->base_slot;
    switch (mode & 3) {
    case 3:
        PrintSprite(g_tile_sprites[base + 18], x, y, 0, 0);
        break;
    case 2:
        PrintSprite(g_tile_sprites[base + 17], x, y, 0, 0);
        break;
    case 1:
        PrintSprite(g_tile_sprites[base + 16], x, y, 0, 0);
        break;
    }
}

/* Rebuild one cell's RF shape bits from its walkable neighbours (see the
 * table at the top).  `ortho` lives in the dead `pos` argument slot in the
 * original — VC6 reuses it once pos has been pushed for the call — and `diag`
 * in the one-dword frame; both fall out of plain byte locals here.  The two
 * dead-end arms are separate `return`s (VC6 does not merge them), and the
 * corner test is a two-case switch on (mask & 0x11), which is what gives the
 * dec/sub dispatch rather than two compares.  Falls off the end without a
 * value on the straight-run and crossroads paths, as the original does. */
// FUNCTION: LEGOLAND 0x0045c870
unsigned char AdjustTileRFFlags(Pos* pos)
{
    Cell* cell = &g_map_rows[pos->y][pos->x];
    unsigned char ortho;
    unsigned char diag;
    unsigned char mask;

    cell->rf &= 0xc3;
    mask = GetPathNeighbours(pos, &ortho, &diag);
    if (ortho == 0)
        return cell->rf |= 0x10;
    if (ortho == 1)
        return cell->rf |= 0x10;
    if (ortho == 2) {
        switch (mask & 0x11) {
        case 0x01:
        case 0x10:
            return cell->rf |= 0x08;
        }
    } else if (ortho == 3) {
        return cell->rf |= 0x04;
    } else if (ortho == 4) {
        cell->rf |= 0x20;
    }
}

/* Re-shape one cell after a path change.  `tile` is never read (every caller
 * passes the path tile code).  The off-map stand-in cell is built with
 * immediate stores and cell.rf is read back AFTER the join — spelling it as a
 * separate `rf = 0` lets VC6 hoist a zero register and reorders the prologue.
 * The final `flags &= ~3` on a u16 field leaves VC6's dead `lea eax` behind,
 * exactly as in the original. */
// FUNCTION: LEGOLAND 0x0045d1a0
void AdjustPathTile(Pos* pos, int tile)
{
    Cell cell;
    unsigned char rf;

    g_path_dirty = 1;
    if (pos->x >= 0 && pos->x < g_map->width &&
        pos->y >= 0 && pos->y < g_map->height) {
        cell = g_map_rows[pos->y][pos->x];
    } else {
        cell.tile = 0;
        cell.flags = 0x40;
        cell.rf = 0;
    }

    rf = cell.rf;
    if ((rf & 1) || ((cell.flags & 0x10) && !(rf & 2)))
        AdjustTileRFFlags(pos);

    if (IsPathCell(&cell))
        g_map_rows[pos->y][pos->x].flags &= ~3;
}

/* Allocate the 256x256 cell grid (once) and load the base tile sets. */
// FUNCTION: LEGOLAND 0x0045aad0
int LoadMapTiles(void)
{
    unsigned short base;
    DbElem* elem;
    void* data;
    char* block;
    char* row;
    Cell** rows;
    int i;

    if (g_map_block == 0) {
        block = (char*)calloc(0x14041f, 1);
        rows = (Cell**)(((unsigned int)block + 0x1f) & ~0x1f);
        g_map_block = block;
        row = (char*)(((unsigned int)block + 0x41f) & ~0x1f);
        g_map_rows = rows;
        for (i = 0; i < 256; i++) {
            g_map_rows[i] = (Cell*)row;
            row += 0x1400;
        }
    }

    FreeTileSpace(0, 0x800);
    AllocTileSpace(0, 1, &base);

    LLIDB_FindElement("MAPPING 1", &elem, 0);
    LLIDB_LoadData(elem);

    LLIDB_FindElement("BASIC TILES 1", &elem, 0);
    data = elem->data;
    g_basic_tiles_data = data;
    g_default_tile = *(int*)data;
    g_tileset_id0 = 0;
    g_tileset_id1 = 1;
    g_tileset_id2 = 2;
    g_tileset_id3 = 3;
    g_tileset_id4 = 4;

    LLIDB_FindElement("NORMAL PATH TILES", &elem, 0);
    g_path_tile_ptr = elem->data;

    g_arrow1 = LoadSprite("arrow01.lls", 1);
    g_arrow2 = LoadSprite("arrow02.lls", 1);
    g_arrow3 = LoadSprite("arrow03.lls", 1);
    g_arrow4 = LoadSprite("arrow04.lls", 1);
    return 1;
}

/* Edge-of-screen autoscroll from the mouse position.
 *
 * Shape notes.  Each axis keeps its new velocity in a local and stores it
 * ONCE after the if-chain; the two "already at the cap" paths jump past that
 * store (the original's `jle 461556` / `jle 4615ac`), so they are gotos here.
 * The two axes are not written identically in the original: the x arms load
 * the velocity into the local first and then compare, while the first y arm
 * compares the GLOBAL against -cap before copying it (VC6 then evaluates the
 * negated cap first, zero-extends it in ecx and has to move it to edx to make
 * room for the velocity — the `mov edx,ecx` at 0x461566).  Every symmetric
 * spelling puts the cap straight into edx. */
// FUNCTION: LEGOLAND 0x004614f0
void MouseScrollMap(void)
{
    MapHdr* m = (MapHdr*)g_map;
    int mx = g_gfx_point.x;
    int my = g_gfx_point.y;
    int vx;
    int vy;
    int ticks;

    if (mx < m->edge_x) {
        vx = g_scroll_vx;
        if (vx <= -m->vmax_x)
            goto x_done;
        vx -= m->accel_x;
    } else if (mx > m->screen_w - m->edge_x) {
        vx = g_scroll_vx;
        if (vx >= m->vmax_x)
            goto x_done;
        vx += m->accel_x;
    } else {
        vx = 0;
    }
    g_scroll_vx = vx;
x_done:

    if (my < m->edge_y) {
        if (g_scroll_vy > -m->vmax_y) {
            vy = g_scroll_vy - m->accel_y;
        } else {
            vy = g_scroll_vy;
            goto y_done;
        }
    } else if (my > m->screen_h - m->edge_y) {
        vy = g_scroll_vy;
        if (vy >= m->vmax_y)
            goto y_done;
        vy += m->accel_y;
    } else {
        vy = 0;
    }
    g_scroll_vy = vy;
y_done:

    ticks = g_frame_ticks;
    ProcessScrolling(ticks * vx / 256, ticks * vy / 256);
    g_scroll_ticks = g_frame_ticks;
}

/* Tear a path tile down: restore the ground, clear the cell's path state
 * (map flags 0x1b and the whole RF byte), re-shape the eight neighbours in
 * the same order UpdatePathNeighbours uses minus the cell itself, split the
 * path square and repaint the area while the overlay is live.  `tile` is
 * read as a dword (an `unsigned short` parameter costs three instructions),
 * so it is an int here although maprestore.c declares the extern as
 * unsigned short; it is only forwarded to AdjustPathTile, which ignores it. */
// FUNCTION: LEGOLAND 0x0045daa0
void RemovePathTile(Pos* pos, int tile)
{
    Pos n;

    RestoreBaseMap(pos->x, pos->y);
    g_map_rows[pos->y][pos->x].flags &= ~0x1b;
    g_map_rows[pos->y][pos->x].rf = 0;

    n.x = pos->x;
    n.y = pos->y - 1;
    AdjustPathTile(&n, tile);

    n.x = pos->x + 1;
    n.y = pos->y;
    AdjustPathTile(&n, tile);

    n.x = pos->x;
    n.y = pos->y + 1;
    AdjustPathTile(&n, tile);

    n.x = pos->x - 1;
    n.y = pos->y;
    AdjustPathTile(&n, tile);

    n.x = pos->x - 1;
    n.y = pos->y + 1;
    AdjustPathTile(&n, tile);

    n.x = pos->x + 1;
    n.y = pos->y - 1;
    AdjustPathTile(&n, tile);

    n.x = pos->x + 1;
    n.y = pos->y + 1;
    AdjustPathTile(&n, tile);

    n.x = pos->x - 1;
    n.y = pos->y - 1;
    AdjustPathTile(&n, tile);

    RemovePathSquare(pos);
    if (g_path_overlay_active)
        RefreshPathArea(pos);
}
