/* LEGOLAND — path construction and path-edge tests.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; names are ours.
 *
 * ---------------------------------------------------------------------------
 * HOW A PATH SEGMENT IS LAID
 *
 * A path object class carries an "add" handler in its descriptor at +0x98
 * (installed at 0x00452c47 next to the remove handler at +0x9c, see
 * maprestore.c).  AddBasicPath is that handler for the ordinary path
 * classes; AddRollerCoasterPath is the coaster-track variant.  Both end in
 * AddPathTile (0x0045d3b0, pathsq.c) with the tile code read as a 16-bit word
 * from the loaded "path" tile record at 0x00832bf0 — the same word the two
 * removers and LoadBaseMap read.
 *
 *   AddBasicPath(obj, pos)
 *     - bounds-checks pos against the map, refuses a cell whose map flags
 *       carry any of 0x08e8 (0x0008 | 0x0020 build-footprint reserved |
 *       0x0040 | 0x0080 | 0x0800), then
 *     - AddBasicObject(obj, pos)  (0x0045efe0, the standard object placer:
 *       it is slot f3 of the standard callback set, sweep3.c) and
 *     - AddPathTile(pos, path_tile)  -> AddPathTileGFX (Cell.flags |= 0x10,
 *       Cell.tile = code, neighbour re-tiling) + AddPathSquare.
 *   AddRollerCoasterPath(pos)
 *     - no object, no checks: writes Cell.tile directly and calls
 *       AddPathTile.  (The direct write is redundant with AddPathTileGFX's
 *       own store; the original does both.)
 *
 * THE BLOKE "ON PATH" FLAG AND THE PATH-EDGE TEST
 *
 * Bloke flags word +0x62 bit 0x02 = "currently on a path".  SetPathFlag
 * recomputes it from the walker's 24.8 world position (+0x68/+0x6c, 256
 * units per tile):
 *     on_path = (rf & 1) || ((mapflags & 0x10) && !(rf & 2))
 * where rf is GetCurrentRFFlags (which already resolves the per-tile RF
 * handler) and mapflags is Cell.flags.  So RF bit 0 is "walkable path", RF
 * bit 1 is "blocked", and map flag 0x10 is the path-tile bit AddPathTileGFX
 * sets.  Anything off-map clears the flag.
 *
 * HitPathEdge(bloke, x, y) asks "would stepping to world (x,y) leave the
 * path?"  It is only meaningful for a walker that IS on a path (bit 0x02);
 * otherwise 0.  Off-map is an edge (1); otherwise it is the exact negation of
 * the on_path predicate above evaluated at the destination.
 *
 * TILE BOUNDS
 *
 * GetTileBounds is GetTileCentre's (tilehelp.c) sibling: same 2:1 iso
 * projection with tile pixel size (w=2h) taken from the default ground
 * sprite, but it returns the tile's inclusive pixel rectangle.  Using
 * (tx-ty-1) and (tx+ty) instead of the centre's (tx-ty) and (tx+ty+1) shifts
 * the point by (-w/2, -h/2), i.e. from the centre to the top-left corner;
 * right/bottom are then left+w-1 / top+h-1.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* The map header again, extended with the viewport origin (Map +0x20/+0x22). */
typedef struct MapHdr {
    char           pad00[0x20]; /* +0x00 (+0x14 width, +0x16 height) */
    unsigned short origin_x;    /* +0x20 viewport origin, pixels */
    unsigned short origin_y;    /* +0x22 */
} MapHdr;

/* The inclusive pixel rectangle GetTileBounds fills. */
typedef struct TileBounds {
    int left;   /* +0x00 */
    int top;    /* +0x04 */
    int right;  /* +0x08 */
    int bottom; /* +0x0c */
} TileBounds;

/* A walking entity (bloke / visitor); only the fields touched here. */
typedef struct Walker {
    char           pad00[0x62]; /* +0x00 */
    unsigned short flags62;     /* +0x62  bit 0x02 = on a path */
    char           pad64[4];    /* +0x64 */
    int            wx;          /* +0x68  world x, 24.8 (256 units per tile) */
    int            wy;          /* +0x6c  world y, 24.8 */
} Walker;

/* BNV path state (bnvpath.c's BNVPath); only +0x00 and +0x08 are read here. */
typedef struct BNVBin BNVBin;
typedef struct BNVNameList BNVNameList;
typedef struct BNVNameNode BNVNameNode;
typedef struct BNVVertex {
    short         x;      /* +0x00 */
    short         y;      /* +0x02 */
    float         z;      /* +0x04 */
    unsigned char pad08[12];
} BNVVertex;
typedef struct BNVPathHdr {
    BNVBin* bin;             /* +0x00 */
    int     tag;             /* +0x04 */
    char    object_name[20]; /* +0x08 */
} BNVPathHdr;

/* ---------------------------------------------------------------- globals -- */

extern int   g_scroll_x;        /* 0x00667cb4  8.8 fixed point */
extern int   g_scroll_y;        /* 0x00667cb8 */
extern void* g_path_tile_ptr;   /* 0x00832bf0  loaded path tile record; first word = tile code */

/* ------------------------------------------------------------ prototypes -- */

extern void AddPathTile(Pos* pos, unsigned short tile);      /* 0x0045d3b0 */
extern void AddBasicObject(void* obj, Pos* pos);             /* 0x0045efe0 */
extern unsigned short Get_MapFlags(int x, int y);            /* 0x00461760 */
extern unsigned char  GetCurrentRFFlags(int x, int y);       /* 0x00461630 */
extern BNVNameList* GetBinVFrame(BNVBin* bin, int frame);                  /* 0x0044dd70 */
extern BNVNameNode* GetObjectFromName(BNVNameList* list, const char* name);/* 0x0044dda0 */
extern BNVVertex*   GetVertex(BNVNameNode* object, int index);             /* 0x0044ddf0 */

/* -------------------------------------------------------------- functions -- */

/* Class add handler for roller-coaster track: paint the path tile code
 * straight into the cell, then lay the path tile (graphics + path square). */
// FUNCTION: LEGOLAND 0x0045dc50
void AddRollerCoasterPath(Pos* pos)
{
    g_map_rows[pos->y][pos->x].tile = *(unsigned short*)g_path_tile_ptr;
    AddPathTile(pos, *(unsigned short*)g_path_tile_ptr);
}

/* Class add handler for the plain path classes: place the object and lay the
 * path tile, provided the cell is on the map and none of the 0x08e8 map
 * flags is set on it. */
// FUNCTION: LEGOLAND 0x0045dbe0
void AddBasicPath(void* obj, Pos* pos)
{
    Cell* cell;

    if (pos->x < 0 || pos->x >= g_map->width ||
        pos->y < 0 || pos->y >= g_map->height)
        cell = 0;
    else
        cell = &g_map_rows[pos->y][pos->x];

    if (cell != 0 && (cell->flags & 0x8e8) == 0) {
        AddBasicObject(obj, pos);
        AddPathTile(pos, *(unsigned short*)g_path_tile_ptr);
    }
}

/* Recompute the walker's "on a path" flag (flags62 bit 0x02) from its world
 * position. */
// FUNCTION: LEGOLAND 0x004831d0
void SetPathFlag(Walker* w)
{
    unsigned short mapflags;
    unsigned short rf;

    if (w->wx >= 0 && w->wx < (g_map->width << 8) &&
        w->wy >= 0 && w->wy < (g_map->height << 8)) {
        mapflags = Get_MapFlags(w->wx, w->wy);
        rf = GetCurrentRFFlags(w->wx, w->wy);
        if ((rf & 1) || ((mapflags & 0x10) && !(rf & 2))) {
            w->flags62 |= 2;
            return;
        }
    }
    w->flags62 &= ~2;
}

/* Would a walker that is on a path leave it by stepping to world (x,y)? */
// FUNCTION: LEGOLAND 0x004834a0
int HitPathEdge(Walker* w, int x, int y)
{
    unsigned short mapflags;
    unsigned short rf;

    if (w->flags62 & 2) {
        if (x >= 0 && x < (g_map->width << 8) &&
            y >= 0 && y < (g_map->height << 8)) {
            mapflags = Get_MapFlags(x, y);
            rf = GetCurrentRFFlags(x, y);
            if (rf & 1)
                return 0;
            if ((mapflags & 0x10) && !(rf & 2))
                return 0;
        }
        return 1;
    }
    return 0;
}

/* Inclusive pixel rectangle of map tile `tile`, scroll applied. */
// FUNCTION: LEGOLAND 0x0045acc0
void GetTileBounds(Pos* tile, TileBounds* out)
{
    short h = g_tile_sprites[g_default_tile]->h;
    short w = (short)(h + h);

    out->left = (short)((w + 1) >> 1) * (tile->x - tile->y - 1)
                - (g_scroll_x >> 8) + ((MapHdr*)g_map)->origin_x;
    out->top = (short)((h + 1) >> 1) * (tile->x + tile->y)
               - (g_scroll_y >> 8) + ((MapHdr*)g_map)->origin_y;
    out->right = out->left + w - 1;
    out->bottom = out->top + h - 1;
}

/* Screen position of a BNV path frame: the centroid of the named object's
 * eight vertices, scaled by 1/8, as an {x,y} pair in eax:edx. The `dframe`
 * parameter slot doubles as the int temporary for the x/y accumulation.
 * The two float->int conversions call the game's own fistp helper
 * (0x00458930, bnvpath.c) in the original; our (int) casts compile to
 * `call __ftol`, which the comparison treats as the same call — the same
 * situation as UpdateBlokeFromBNVPath. */
// FUNCTION: LEGOLAND 0x00485000
Offset BNVPath_GetBINVScreenCoords(BNVPathHdr* path, int dframe)
{
    BNVNameList* frame;
    BNVNameNode* object;
    BNVVertex* vertex;
    float sum_x;
    float sum_y;
    float sum_z;
    int i;
    Offset out;

    sum_x = 0.0f;
    sum_y = 0.0f;
    sum_z = 0.0f;

    frame = GetBinVFrame(path->bin, dframe);
    object = GetObjectFromName(frame, path->object_name);
    for (i = 0; i < 8; ++i) {
        vertex = GetVertex(object, i);
        dframe = vertex->x;
        sum_x += dframe;
        dframe = vertex->y;
        sum_y += dframe;
        sum_z += vertex->z;
    }

    out.ox = (int)(sum_x * 0.125);
    out.oy = (int)(sum_y * 0.125);
    return out;
}
