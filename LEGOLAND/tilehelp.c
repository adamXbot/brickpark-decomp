/* LEGOLAND — tile geometry helpers.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; field and type names are ours.
 *
 * ---------------------------------------------------------------------------
 * TILE <-> WORLD GEOMETRY (recovered here; cross-checked against GetTileBounds
 * 0x0045acc0 and DoRndWalkPathTileAction 0x00483920)
 *
 *  * A walking entity's world position is two 24.8 fixed-point ints, at +0x68
 *    (x) and +0x6c (y).  ONE MAP TILE = 256 UNITS on each axis, so the tile a
 *    walker occupies is simply (pos >> 8) — exactly what
 *    DoRndWalkPathTileAction does at 0x0048392e / 0x0048393b before it looks
 *    the cell's RF flags up.  The low byte is the position WITHIN the tile,
 *    and 0x80 is the TILE CENTRE.  Those two facts are the whole content of
 *    OverNewTile and CrossTileCentre:
 *        - "left the tile"      == some bit above bit 7 changed
 *        - "crossed the centre" == bit 7 changed while the tile did not
 *
 *  * Screen placement is a 2:1 isometric diamond.  The tile pixel size comes
 *    from the DEFAULT ground sprite: h = g_tile_sprites[g_default_tile]->h
 *    (Sprite +0x16), w = 2*h — the same relation GetTileDimensions
 *    (0x00460540) states.  For the shipped 32x16 ground tiles h=16, w=32.
 *
 *        centre_x = ((w+1)>>1) * (tx - ty)     - (g_scroll_x>>8) + map->origin_x
 *        centre_y = ((h+1)>>1) * (tx + ty + 1) - (g_scroll_y>>8) + map->origin_y
 *
 *    i.e. for a 32x16 tile: 16*(tx-ty) and 8*(tx+ty)+8.  The "+1" on the y
 *    axis is what moves the point from the diamond's TOP corner to its CENTRE
 *    (half of h).  GetTileBounds uses (tx-ty-1) and (tx+ty) with the same
 *    multipliers and then adds w-1 / h-1, which yields exactly
 *    left = centre_x - w/2, top = centre_y - h/2 — the two functions agree,
 *    and that agreement is what pins the formula.
 *    map->origin_x / origin_y (Map +0x20/+0x22, unsigned short) is the
 *    viewport origin in pixels; the scroll globals are 8.8 fixed point, hence
 *    the >>8.  The evaluation order really is "iso - scroll + origin".
 *
 *  * g_tile_sprites[] (0x00805f60, 0x800 slots) doubles as the tile-slot
 *    allocator's free map: -1 = free, anything else = in use.  AllocTileSpace
 *    (0x0045a9b0) first-fits a run of -1 slots, zeroes it, and fills the
 *    parallel g_tile_info[] (0x00801f40, stride 8) with {desc, tile code};
 *    FreeTileSpace below is its exact counterpart and paints the run back
 *    to -1.  The 0x800-slot cap is checked by AllocTileSpace, not here.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

void* memset(void*, int, unsigned int);

/* ------------------------------------------------------------------ types -- */

/* The map header again, extended past legoland.h's width/height with the
 * viewport origin the geometry helpers add in. */
typedef struct MapHdr {
    char           pad00[0x20]; /* +0x00 (+0x14 width, +0x16 height) */
    unsigned short origin_x;    /* +0x20 viewport origin, pixels */
    unsigned short origin_y;    /* +0x22 */
} MapHdr;

/* A walking entity (bloke / visitor).  Only the fields these helpers touch;
 * the rest is padding that holds the offsets. */
typedef struct Walker {
    char           pad00[0x62]; /* +0x00 */
    unsigned short flags62;     /* +0x62  bit 0x04 = tile action already done */
    char           pad64[4];    /* +0x64 */
    int            wx;          /* +0x68  world x, 24.8 (256 units per tile) */
    int            wy;          /* +0x6c  world y, 24.8 */
    char           pad70[2];    /* +0x70 */
    unsigned char  dir;         /* +0x72  facing, 0..7 */
} Walker;

/* ---------------------------------------------------------------- globals -- */

extern int g_scroll_x;  /* 0x00667cb4  8.8 fixed point */
extern int g_scroll_y;  /* 0x00667cb8 */

/* ------------------------------------------------------------ prototypes -- */

extern int  OverNewTile(Walker* w, int x, int y);      /* 0x00483650 */
extern int  CrossTileCentre(Walker* w, int x, int y);  /* 0x004837d0 */
/* Unexported: OverNewTile, and on a "yes" also clear flags62 bit 0x04 so the
 * next tile's action is allowed to fire.  Returns 1 when the tile changed. */
extern int  RndWalk_LeftTile(Walker* w, int x, int y); /* 0x004837a0 */
extern int  DoRndWalkPathTileAction(Walker* w);        /* 0x00483920 */

/* -------------------------------------------------------------- functions -- */

/* Free a run of tile slots: paint g_tile_sprites[base .. base+n) back to -1.
 * Counterpart of AllocTileSpace (0x0045a9b0).  Both parameters arrive as
 * 16-bit values (the .TSF descriptor keeps its base slot as `id & 0xffff`),
 * which is what produces the two `and reg,0ffffh`. */
// FUNCTION: LEGOLAND 0x0045aa90
void FreeTileSpace(unsigned short base, unsigned short n)
{
    memset(&g_tile_sprites[base], 0xff, n * 4);
}

/* True when stepping to (x,y) would leave the walker's current map tile.
 * Tile == coord >> 8, so "same tile" is "nothing above bit 7 differs". */
// FUNCTION: LEGOLAND 0x00483650
int OverNewTile(Walker* w, int x, int y)
{
    if (((w->wx ^ x) & 0xffffff00) == 0) {
        if (((w->wy ^ y) & 0xffffff00) == 0) {
            return 0;
        }
    }
    return 1;
}

/* True when the step to (x,y) crosses the CENTRE of the tile the walker is
 * already standing on.  The centre is sub-tile offset 0x80, so a crossing is
 * "bit 7 flipped" on the axis this facing walks along — facings 0,1,4,5 test
 * the y axis, 2,3 (and anything out of range) test x — AND the step must not
 * have left the tile altogether.  This is the hook the path AI uses to fire a
 * tile's action exactly once per visit, at the tile's midpoint. */
// FUNCTION: LEGOLAND 0x004837d0
int CrossTileCentre(Walker* w, int x, int y)
{
    int d;

    switch (w->dir) {
    case 0:
    case 1:
    case 4:
    case 5:
        d = w->wy ^ y;
        break;
    default:
        d = w->wx ^ x;
        break;
    }
    if (d & 0x80) {
        if (OverNewTile(w, x, y) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Per-step tile bookkeeping for a randomly-walking visitor: if the step stays
 * inside the current tile, and it crosses that tile's centre, and the tile's
 * action has not already fired this visit (flags62 bit 0x04), run it and
 * return its result.  Anything else returns 0. */
// FUNCTION: LEGOLAND 0x00483b10
int Handle_RndWalk_TileSpecifics(Walker* w, int x, int y)
{
    if (RndWalk_LeftTile(w, x, y) == 0) {
        if (CrossTileCentre(w, x, y) != 0) {
            if ((w->flags62 & 4) == 0) {
                return DoRndWalkPathTileAction(w);
            }
        }
    }
    return 0;
}

/* Pixel centre of map tile `tile`, with the scroll offset applied. */
// FUNCTION: LEGOLAND 0x0045ad60
void GetTileCentre(Pos* tile, Pos* out)
{
    short h = g_tile_sprites[g_default_tile]->h;
    short w = (short)(h + h);

    out->x = (short)((w + 1) >> 1) * (tile->x - tile->y)
             - (g_scroll_x >> 8) + ((MapHdr*)g_map)->origin_x;
    out->y = (short)((h + 1) >> 1) * (tile->x + tile->y + 1)
             - (g_scroll_y >> 8) + ((MapHdr*)g_map)->origin_y;
}
