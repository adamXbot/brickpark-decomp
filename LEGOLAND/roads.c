/* LEGOLAND -- the driving school's road tiler and the jungle cruise's boat
 * path generator.
 *
 * Reconstructed from original/legoland.exe with the VC6 SP3 toolchain
 * (/O2 /Gy /Gd).  Struct field OFFSETS, record sizes and global addresses are
 * load-bearing; the names are ours.  Types are defined LOCALLY on purpose
 * (legoland.h is owned elsewhere).
 *
 *   0x00412680  Road_SetTile      803/803 insns   [OK]   -- the largest
 *                                                 function still unmatched
 *   0x00433840  JcBoat_Animate    330/330 insns   [WIP, 3 mismatches]
 *
 * =========================================================================
 * THE ROAD TILE ENCODING: WHICH NEIGHBOUR MASK PICKS WHICH TILE
 *
 * A driving-school road BLOCK is 4x4 map cells, not one.  ridecb6.c keeps one
 * RoadBlock record per block; anim2.c's Road_Restitch counts the four
 * ORTHOGONAL neighbouring blocks that belong to the same school (mask bits
 * 1 N, 2 E, 4 S, 8 W) and turns that count into a (shape, rotation) pair --
 * 0 leave alone, 1 dead end or straight run, 2 corner, 3 T junction,
 * 4 crossroads.  Road_SetTile is the half that turns that pair into sixteen
 * actual map tiles.
 *
 * `shape` is a packed byte, stored verbatim WITH the rotation into the
 * block's kind byte at +0x14 as `shape | ((rot & 3) << 5)`:
 *
 *      bits 0..3   1 dead end, 2 straight (also the DEFAULT arm),
 *                  3 T junction, 4 crossroads outer, 5 crossroads centre,
 *                  6 straight with a stem cap, 7 straight capped both ends
 *      bit 4       the block also carries a ZEBRA CROSSING
 *
 * `rot` is a quarter turn 0..3.  Everything else comes out of one table,
 * g_road_tiles at 0x004b4c08: FIFTEEN rows of FOUR unsigned shorts, indexed
 * [piece][quarter turn].  A table word is packed (tileset << 8) | slot and is
 * resolved against the road class's TSM record array (0x0082c67c -- the
 * 8-byte {LLElem* entry, void* loaded} records LLIDB_LoadTSMData builds) as
 *
 *      map tile = *(unsigned short*)tsm[word >> 8].loaded + (word & 0xff)
 *
 * i.e. the tileset's base slot plus the slot index.  Every word in the
 * shipped table has tileset 0, so in practice all 0x37 road tiles live in one
 * set; the encoding allows more.
 *
 * The fifteen pieces, with their four rotations (slot numbers):
 *      0  { 0, 1, 2, 3}  outer kerb corner       8  {20,21,1e,1f}  junction kerb
 *      1  { 6, 7, 4, 5}  kerb edge               9  {24,25,22,23}  junction corner
 *      2  { a, c, f, 9}  end cap, left corner   10  {27,28,29,26}  junction inner
 *      3  { b, d, e, 8}  end cap, right corner  11  {2a,2a,2a,2a}  plain tarmac
 *      4  {12,14,16,10}  end cap, left half     12  {2e,2b,2c,2d}  junction detail
 *      5  {13,15,17,11}  end cap, right half    13  {31,32,2f,30}  junction detail
 *      6  {18,1d,1c,19}  zebra end              14  {34,35,36,33}  T stem cap
 *      7  {1a,1b,1a,1b}  zebra centre
 *
 * Two of those slots are load-bearing constants further down: 0x1a and 0x1b
 * are the zebra centre tile seen ACROSS and ALONG the block, and the tail of
 * this function looks for exactly those two numbers to decide which way a
 * crossing runs.
 *
 * The sixteen tiles are built into a 4x4 scratch array `tiles[dy][dx]` (zero
 * filled first, so any cell a shape does not name is stamped with tile 0 of
 * set 0) and only then stamped, so the four rotations are ONE table fill plus
 * four different stamping orders:
 *      rot 0   map(x+r, y+c) = tiles[c][r]        (the transpose)
 *      rot 1   map(x+r, y+c) = tiles[3-r][c]
 *      rot 2   map(x+r, y+c) = tiles[3-c][3-r]
 *      rot 3   map(x+r, y+c) = tiles[r][3-c]
 * A rotation outside 0..3 stamps nothing at all (the switch has no default),
 * so such a block keeps whatever tiles it already had.
 *
 * Every arm builds its rows out of the same four rotation indices: `rot`
 * itself for the near side, `rot-2` for the far side (a half turn, which is
 * the same piece seen from the other end) and `rot-1` / `rot+1` for the two
 * sides of a junction.  The straight/default arm is four identical rows
 * {0[rot], 1[rot], 1[rot-2], 0[rot-2]} -- kerb, edge, edge, kerb -- and the
 * dead-end arms just replace the first and/or last row with the end cap
 * {2[..], 4[..], 5[..], 3[..]}.
 *
 * Before any of that, all sixteen cells are claimed for the road: each gets
 * rf = 2 (blocked, not walkable), map flag 0x8, the DRIVING SCHOOL class's
 * LLIDB element (its ObjDef is cached at 0x0082c684, the element at +0xc4)
 * and the block's packed map square as its owner key -- and each is handed
 * to RemovePathSquare, so laying a road over a footpath destroys the path.
 *
 * THE ZEBRA CROSSING, AND HOW IT IS MADE WALKABLE
 *
 * With bit 4 of `shape` set, one interior ROW of the scratch array is
 * overwritten with the crossing strip {6[rot], 7[rot], 7[rot], 6[rot-2]}:
 * row 1 for rot 0/1 and row 2 for rot 2/3.  After the stamping pass the
 * function reads the map back and asks which of the two centre tiles it
 * actually laid:
 *      cell(x+1, y+1) is slot 0x1a  ->  the crossing runs EAST-WEST, so the
 *                                       four cells (x..x+3, y+1) become path
 *      cell(x+2, y+1) is slot 0x1b  ->  it runs NORTH-SOUTH, so the four
 *                                       cells (x+2, y..y+3) become path
 * where "become path" is AdjustTileRFFlags + AddPathSquare + rf = 3 on each
 * cell.  Reading the answer back off the map instead of deriving it from
 * `rot` is what makes the two arms mutually exclusive, and is why those magic
 * numbers appear here at all.
 *
 * Finally the four cells just outside the middle of each edge --
 * (x-1, y+1), (x+4, y+1), (x+2, y-1), (x+2, y+4) -- are re-shaped with
 * AdjustTileRFFlags if they are walkable path, so a footpath meeting the
 * block's crossing re-stitches into it.
 *
 * ORIGINAL BUGS reproduced: every cell write in the claiming loop, and every
 * `rf = 3` in the two crossing arms, dereferences MapCellAt WITHOUT a null
 * check, so a block laid over the edge of the map writes through a null
 * pointer -- the same defect junglecruise.c and objmap2.c document.  Only the
 * four edge-neighbour probes at the end test for null.
 *
 * CODEGEN NOTES.  Two levers decided this function.  (1) The tile word must
 * be INDEXED TWICE inside the stamping loops, once for `>> 8` and once for
 * `& 0xff`; hoisting it into an `unsigned short` local matches the rot 0 loop
 * (where the address is a plain induction pointer and VC6 CSEs the two loads
 * back into one) but not the other three, where the original keeps a word
 * load and a separate byte load of the same address.  (2) The cell fetch that
 * looks for the crossing must read the coordinates back out of `pos`
 * (`MapCellAt(pos.x, pos.y)`), not from `x` and `y + 1`: reading the
 * parameters again there gives `y` a live range long enough for VC6 to park
 * it in ebp for the whole tail, where the original reloads it, and that one
 * register renamed the last 160 instructions.
 * ========================================================================= */

#include <math.h>

void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)

/* ------------------------------------------------------- shared map types -- */
typedef struct Pos { int x; int y; } Pos;
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

/* ------------------------------------------------------------ road types -- */
typedef struct Cell {
    void*          obj;             /* +0x00 */
    unsigned short key;             /* +0x04 */
    unsigned char  pad06[2];
    unsigned short tile;            /* +0x08 */
    unsigned char  pad0a[2];
    unsigned short flags;           /* +0x0c */
    unsigned char  pad0e[2];
    unsigned char  rf;              /* +0x10 */
    unsigned char  pad11[3];
} Cell;                             /* 0x14 */

typedef struct MapHdr {
    unsigned char  pad00[0x14];
    unsigned short width;           /* +0x14 */
    unsigned short height;          /* +0x16 */
} MapHdr;

typedef struct TileSet { int base_slot; } TileSet;
typedef struct TileRec { TileSet* set; unsigned int code; } TileRec;

typedef struct TsmRec { void* elem; unsigned short* loaded; } TsmRec;

typedef struct RoadBlock {
    unsigned char  pad00[0x14];
    unsigned char  kind;            /* +0x14 */
} RoadBlock;

/* An object-class descriptor, as this file touches it: the callback slots
 * live at +0x8c..+0xc0 (docs/RIDE_CALLBACKS.md) and the class's own LLIDB
 * element sits just past them at +0xc4 (loaders.c, power.c).  A cell claimed
 * by a class carries that ELEMENT, not the ObjDef -- junglecruise.c's
 * JcWater_Add does exactly the same thing for river cells. */
typedef struct ObjDef {
    unsigned char pad00[0xc4];
    void*         elem;             /* +0xc4  the class's LLIDB element */
} ObjDef;

extern MapHdr*  g_map;                      /* 0x004bcbf4 */
extern Cell**   g_map_rows;                 /* 0x00801400 */
extern TileRec  g_sp_tile_info[];           /* 0x00801f40 */
extern int      g_bg_full_update;           /* 0x004b9220 */
/* The "TILES FOR DSCHOOL" TSM record array (LLIDB_LoadTSMData's 8-byte
 * {LLElem* entry, void* loaded} records); the loaded TSF descriptor's first
 * word is that tileset's base slot. */
extern TsmRec*  g_road_tsm;                 /* 0x0082c67c */
/* The DRIVING SCHOOL class's ObjDef, cached by its create handler
 * (0x00413a10, which stores its element's `data` here before looking up
 * "DSCHOOL LIGHTS"). */
extern ObjDef*  g_dschool_cls;              /* 0x0082c684 */
/* [piece][quarter turn]; the "TILES FOR DSCHOOL" string sits immediately
 * after it at 0x004b4c80, which is what pins the table's extent. */
extern unsigned short g_road_tiles[15][4];  /* 0x004b4c08 */

extern RoadBlock* Road_FindAt(int x, int y);                   /* 0x004125a0 */
extern void  RemovePathSquare(Pos* p);                         /* 0x00481c90 */
extern void  AddPathSquare(Pos* p);                            /* 0x00481c50 */
extern unsigned char AdjustTileRFFlags(Pos* pos);              /* 0x0045c870 */
extern void  SetMapTile(int x, int y, unsigned short t);       /* 0x00461780 */

/* The bounds-checked cell fetch every map accessor open-codes (objmap.c);
 * the callers below dereference the result WITHOUT a null check, exactly as
 * the original does. */
static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

// FUNCTION: LEGOLAND 0x00412680
void Road_SetTile(int x, int y, int shape, int rot)
{
    unsigned short tiles[4][4] = {0};
    Pos            pos;
    BPosW          key;
    RoadBlock*     blk;
    Cell*          cell;
    int            i;
    int            r;
    int            c;
    int            rp1 = (rot + 1) & 3;
    int            rm2 = (rot - 2) & 3;
    int            rm1 = (rot - 1) & 3;

    blk = Road_FindAt(x, y);
    if (blk == 0)
        return;
    g_bg_full_update = 1;
    blk->kind = (unsigned char)(((rot & 3) << 5) | shape);
    key.b.x = (unsigned char)x;
    key.b.y = (unsigned char)y;
    pos.x = x;
    for (i = 0; i < 4; i++) {
        pos.y = y + i;
        cell = MapCellAt(x, pos.y);
        cell[0].rf = 2;
        cell[0].flags |= 8;
        cell[0].obj = g_dschool_cls->elem;
        cell[0].key = key.w;
        cell[1].rf = 2;
        cell[1].flags |= 8;
        cell[1].obj = g_dschool_cls->elem;
        cell[1].key = key.w;
        cell[2].rf = 2;
        cell[2].flags |= 8;
        cell[2].obj = g_dschool_cls->elem;
        cell[2].key = key.w;
        cell[3].rf = 2;
        cell[3].flags |= 8;
        cell[3].obj = g_dschool_cls->elem;
        cell[3].key = key.w;
        RemovePathSquare(&pos);
        pos.x++;
        RemovePathSquare(&pos);
        pos.x++;
        RemovePathSquare(&pos);
        pos.x++;
        RemovePathSquare(&pos);
        pos.x = x;
    }
    switch (shape & 0xf) {
    case 1:
        tiles[0][0] = g_road_tiles[2][rot];
        tiles[0][1] = g_road_tiles[4][rot];
        tiles[0][2] = g_road_tiles[5][rot];
        tiles[0][3] = g_road_tiles[3][rot];
        tiles[1][0] = g_road_tiles[0][rot];
        tiles[1][1] = g_road_tiles[1][rot];
        tiles[1][2] = g_road_tiles[1][rm2];
        tiles[1][3] = g_road_tiles[0][rm2];
        tiles[2][0] = g_road_tiles[0][rot];
        tiles[2][1] = g_road_tiles[1][rot];
        tiles[2][2] = g_road_tiles[1][rm2];
        tiles[2][3] = g_road_tiles[0][rm2];
        tiles[3][0] = g_road_tiles[0][rot];
        tiles[3][1] = g_road_tiles[1][rot];
        tiles[3][2] = g_road_tiles[1][rm2];
        tiles[3][3] = g_road_tiles[0][rm2];
        break;
    case 7:
        tiles[0][0] = g_road_tiles[2][rot];
        tiles[0][1] = g_road_tiles[4][rot];
        tiles[0][2] = g_road_tiles[5][rot];
        tiles[0][3] = g_road_tiles[3][rot];
        tiles[1][0] = g_road_tiles[0][rot];
        tiles[1][1] = g_road_tiles[1][rot];
        tiles[1][2] = g_road_tiles[1][rm2];
        tiles[1][3] = g_road_tiles[0][rm2];
        tiles[2][0] = g_road_tiles[0][rot];
        tiles[2][1] = g_road_tiles[1][rot];
        tiles[2][2] = g_road_tiles[1][rm2];
        tiles[2][3] = g_road_tiles[0][rm2];
        tiles[3][0] = g_road_tiles[3][rm2];
        tiles[3][1] = g_road_tiles[5][rm2];
        tiles[3][2] = g_road_tiles[4][rm2];
        tiles[3][3] = g_road_tiles[2][rm2];
        break;
    case 3:
        tiles[0][0] = g_road_tiles[14][rot];
        tiles[0][1] = g_road_tiles[0][rp1];
        tiles[0][2] = g_road_tiles[0][rp1];
        tiles[0][3] = g_road_tiles[0][rp1];
        tiles[1][0] = g_road_tiles[0][rot];
        tiles[1][1] = g_road_tiles[11][rot];
        tiles[1][2] = g_road_tiles[12][rot];
        tiles[1][3] = g_road_tiles[8][rp1];
        tiles[2][0] = g_road_tiles[0][rot];
        tiles[2][1] = g_road_tiles[13][rot];
        tiles[2][2] = g_road_tiles[10][rot];
        tiles[2][3] = g_road_tiles[8][rm1];
        tiles[3][0] = g_road_tiles[0][rot];
        tiles[3][1] = g_road_tiles[8][rot];
        tiles[3][2] = g_road_tiles[8][rm2];
        tiles[3][3] = g_road_tiles[9][rot];
        break;
    case 4:
        tiles[0][0] = g_road_tiles[9][rm2];
        tiles[0][1] = g_road_tiles[11][rot];
        tiles[0][2] = g_road_tiles[11][rm2];
        tiles[0][3] = g_road_tiles[9][rm1];
        tiles[1][0] = g_road_tiles[1][rp1];
        tiles[1][1] = g_road_tiles[1][rp1];
        tiles[1][2] = g_road_tiles[1][rp1];
        tiles[1][3] = g_road_tiles[1][rp1];
        tiles[2][0] = g_road_tiles[1][rm1];
        tiles[2][1] = g_road_tiles[1][rm1];
        tiles[2][2] = g_road_tiles[1][rm1];
        tiles[2][3] = g_road_tiles[1][rm1];
        tiles[3][0] = g_road_tiles[0][rm1];
        tiles[3][1] = g_road_tiles[0][rm1];
        tiles[3][2] = g_road_tiles[0][rm1];
        tiles[3][3] = g_road_tiles[0][rm1];
        break;
    case 5:
        tiles[0][0] = g_road_tiles[9][rm2];
        tiles[0][1] = g_road_tiles[11][rot];
        tiles[0][2] = g_road_tiles[11][rot];
        tiles[0][3] = g_road_tiles[9][rm1];
        tiles[1][0] = g_road_tiles[11][rot];
        tiles[1][1] = g_road_tiles[11][rot];
        tiles[1][2] = g_road_tiles[11][rot];
        tiles[1][3] = g_road_tiles[11][rot];
        tiles[2][0] = g_road_tiles[11][rot];
        tiles[2][1] = g_road_tiles[11][rot];
        tiles[2][2] = g_road_tiles[11][rot];
        tiles[2][3] = g_road_tiles[11][rot];
        tiles[3][0] = g_road_tiles[9][rp1];
        tiles[3][1] = g_road_tiles[11][rot];
        tiles[3][2] = g_road_tiles[11][rot];
        tiles[3][3] = g_road_tiles[9][rot];
        break;
    case 6:
        tiles[0][0] = g_road_tiles[2][rot];
        tiles[0][1] = g_road_tiles[4][rot];
        tiles[0][2] = g_road_tiles[5][rot];
        tiles[0][3] = g_road_tiles[3][rot];
        tiles[1][0] = g_road_tiles[0][rot];
        tiles[1][1] = g_road_tiles[1][rot];
        tiles[1][2] = g_road_tiles[1][rm2];
        tiles[1][3] = g_road_tiles[0][rm2];
        tiles[2][0] = g_road_tiles[0][rot];
        tiles[2][1] = g_road_tiles[1][rot];
        tiles[2][2] = g_road_tiles[1][rm2];
        tiles[2][3] = g_road_tiles[0][rm2];
        tiles[3][0] = g_road_tiles[14][rm1];
        tiles[3][1] = g_road_tiles[0][rm1];
        tiles[3][2] = g_road_tiles[0][rm1];
        tiles[3][3] = g_road_tiles[0][rm1];
        break;
    default:
        tiles[0][0] = g_road_tiles[0][rot];
        tiles[0][1] = g_road_tiles[1][rot];
        tiles[0][2] = g_road_tiles[1][rm2];
        tiles[0][3] = g_road_tiles[0][rm2];
        tiles[1][0] = g_road_tiles[0][rot];
        tiles[1][1] = g_road_tiles[1][rot];
        tiles[1][2] = g_road_tiles[1][rm2];
        tiles[1][3] = g_road_tiles[0][rm2];
        tiles[2][0] = g_road_tiles[0][rot];
        tiles[2][1] = g_road_tiles[1][rot];
        tiles[2][2] = g_road_tiles[1][rm2];
        tiles[2][3] = g_road_tiles[0][rm2];
        tiles[3][0] = g_road_tiles[0][rot];
        tiles[3][1] = g_road_tiles[1][rot];
        tiles[3][2] = g_road_tiles[1][rm2];
        tiles[3][3] = g_road_tiles[0][rm2];
        break;
    }
    if (shape & 0x10) {
        if (rot < 2) {
            tiles[1][0] = g_road_tiles[6][rot];
            tiles[1][1] = g_road_tiles[7][rot];
            tiles[1][2] = g_road_tiles[7][rot];
            tiles[1][3] = g_road_tiles[6][rm2];
        } else {
            tiles[2][0] = g_road_tiles[6][rot];
            tiles[2][1] = g_road_tiles[7][rot];
            tiles[2][2] = g_road_tiles[7][rot];
            tiles[2][3] = g_road_tiles[6][rm2];
        }
    }
    switch (rot) {
    case 0:
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++) {
                SetMapTile(x + r, y + c,
                           (unsigned short)(*g_road_tsm[tiles[c][r] >> 8].loaded
                                            + (tiles[c][r] & 0xff)));
            }
        }
        break;
    case 1:
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++) {
                SetMapTile(x + r, y + c,
                           (unsigned short)(*g_road_tsm[tiles[3 - r][c] >> 8].loaded
                                            + (tiles[3 - r][c] & 0xff)));
            }
        }
        break;
    case 2:
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++) {
                SetMapTile(x + r, y + c,
                           (unsigned short)(*g_road_tsm[tiles[3 - c][3 - r] >> 8].loaded
                                            + (tiles[3 - c][3 - r] & 0xff)));
            }
        }
        break;
    case 3:
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++) {
                SetMapTile(x + r, y + c,
                           (unsigned short)(*g_road_tsm[tiles[r][3 - c] >> 8].loaded
                                            + (tiles[r][3 - c] & 0xff)));
            }
        }
        break;
    }
    pos.x = x;
    pos.y = y + 1;
    cell = MapCellAt(pos.x, pos.y);
    if (cell[1].tile - g_sp_tile_info[cell[1].tile].set->base_slot == 0x1a) {
        AdjustTileRFFlags(&pos);
        AddPathSquare(&pos);
        cell[0].rf = 3;
        pos.x++;
        AdjustTileRFFlags(&pos);
        AddPathSquare(&pos);
        cell[1].rf = 3;
        pos.x++;
        AdjustTileRFFlags(&pos);
        AddPathSquare(&pos);
        cell[2].rf = 3;
        pos.x++;
        AdjustTileRFFlags(&pos);
        AddPathSquare(&pos);
        cell[3].rf = 3;
    } else if (cell[2].tile - g_sp_tile_info[cell[2].tile].set->base_slot == 0x1b) {
        pos.x = x + 2;
        /* back to `y`; written as a decrement of pos.y (which is y + 1) because
         * reading the parameter again here gives `y` a live range long enough
         * for VC6 to park it in a callee-saved register for the rest of the
         * function, where the original rematerialises it. */
        pos.y--;
        AdjustTileRFFlags(&pos);
        AddPathSquare(&pos);
        MapCellAt(pos.x, pos.y)->rf = 3;
        pos.y++;
        AdjustTileRFFlags(&pos);
        AddPathSquare(&pos);
        MapCellAt(pos.x, pos.y)->rf = 3;
        pos.y++;
        AdjustTileRFFlags(&pos);
        AddPathSquare(&pos);
        MapCellAt(pos.x, pos.y)->rf = 3;
        pos.y++;
        AdjustTileRFFlags(&pos);
        AddPathSquare(&pos);
        MapCellAt(pos.x, pos.y)->rf = 3;
    }
    pos.x = x - 1;
    pos.y = y + 1;
    cell = MapCellAt(pos.x, pos.y);
    if (cell != 0 && (cell->rf & 1))
        AdjustTileRFFlags(&pos);
    pos.x = x + 4;
    pos.y = y + 1;
    cell = MapCellAt(pos.x, pos.y);
    if (cell != 0 && (cell->rf & 1))
        AdjustTileRFFlags(&pos);
    pos.x = x + 2;
    pos.y = y - 1;
    cell = MapCellAt(pos.x, pos.y);
    if (cell != 0 && (cell->rf & 1))
        AdjustTileRFFlags(&pos);
    pos.x = x + 2;
    pos.y = y + 4;
    cell = MapCellAt(pos.x, pos.y);
    if (cell != 0 && (cell->rf & 1))
        AdjustTileRFFlags(&pos);
}

/* --------------------------------------------------- jungle cruise types -- */

/* One precomputed sub-step of a boat's crossing of its 5x5 river square,
 * in 1/16 map units relative to the square's centre. */
typedef struct JcWob { int x; int y; } JcWob;

typedef struct JcBoat {
    BPosW           key;            /* +0x00  the station that launched it */
    unsigned char   pad02[2];
    int             cx;             /* +0x04  the map square it is on */
    int             cy;             /* +0x08 */
    int             nx;             /* +0x0c  the map square it is heading for */
    int             ny;             /* +0x10 */
    unsigned char   pad14[8];
    JcWob           wob[0x50];      /* +0x1c  80 sub-step offsets */
    int             frame[0x50];    /* +0x29c 80 sprite/heading codes */
    int             f3dc;           /* +0x3dc the side it entered by */
    int             state;          /* +0x3e0 */
    int             leg;            /* +0x3e4 */
    unsigned char   pad3e8[0x3f4 - 0x3e8];
    struct JcBoat*  next;           /* +0x3f4 */
} JcBoat;                           /* 0x3f8 */

/* Per-side geometry, indexed by the BIT INDEX of a side mask
 * (0 N, 1 E, 2 S, 3 W).  {dx,dy} is the direction of travel for a boat that
 * came IN by that side, {sx,sy} is the unit position of that side's edge. */
typedef struct JcStep { int dx; int dy; int sx; int sy; } JcStep;

/* Per-side arc, again indexed by the entry side's bit index.  The boat's
 * offset traces {sin, cos} of an angle swept from a0 to a1 degrees, with
 * the unit circle recentred on {ox, oy}. */
typedef struct JcCurve { float a0; float a1; float ox; float oy; } JcCurve;

extern JcStep  g_jc_step[4];        /* 0x004b7148 */
extern JcCurve g_jc_curve_cw[4];    /* 0x004b7188  N->E->S->W->N */
extern JcCurve g_jc_curve_ccw[4];   /* 0x004b71c8  N->W->S->E->N */

extern int ArcTan256(int x, int y);                            /* 0x004806e0 */

/* =========================================================================
 * 0x00433840 -- JcBoat_Animate: lay down the 80 sub-step offsets a boat
 * follows while it crosses one 5x5 river square, and the 80 heading codes
 * that go with them.  This is the drawing half of junglecruise.c's
 * JcBoat_Step (anim2.c 0x004334c0), which calls it once per square.
 *
 * `from` is the side the boat came IN by and `to` the side it leaves by,
 * both single-bit masks (1 N, 2 E, 4 S, 8 W) or -1 for "none".  Everything
 * is table-driven off g_jc_step, indexed by the BIT INDEX of a side (so N=0,
 * E=1, S=2, W=3): {dx,dy} is the direction a boat that entered by that side
 * travels in, {sx,sy} the unit position of that side's edge.  The wobble
 * buffer is in 1/16 map units, and one whole square is 640 of them -- which
 * is why the entry offset is `edge * 40 * 16.0f` and a full crossing is 80
 * sub-steps of `step * 16`, 40 to the centre and 40 out again.
 *
 * FIVE CASES, in the order the function tests them:
 *   from == -1 && to == -1   the boat is stuck and stays stuck: the whole
 *                            wobble buffer is zeroed (it sits dead centre).
 *   to == -1                 it drifts IN from `from`'s edge to the centre
 *                            over the first 40 sub-steps and holds there.
 *   from == -1               it sits at the centre for 40 sub-steps and then
 *                            drifts OUT towards `to` (using the step of the
 *                            OPPOSITE side, (i+2)%4, because g_jc_step is
 *                            indexed by entry side).
 *   from == to               a U-turn: it runs out to 40 units past the
 *                            entry edge and comes back, `min(j, 80-j)`.
 *   otherwise                a crossing.  Here `from` and `to` are first
 *                            remapped 1 -> 0x11 so that N sorts ABOVE W, and
 *                            then `to & (from >> 2)` (or the mirror when
 *                            to > from) says "straight through" -- N/S and
 *                            E/W -- which lays a straight line of 80 steps,
 *                            while anything else is a quarter turn.
 *
 * A TURN IS A REAL ARC, not two half-lines.  g_jc_curve_cw (0x004b7188) and
 * g_jc_curve_ccw (0x004b71c8) hold, per entry side, {a0, a1, ox, oy}: a
 * start and end angle in DEGREES and a recentring offset.  `to & (from * 2)`
 * picks the clockwise table (the N->E->S->W cycle) and `from & (to * 2)` the
 * anticlockwise one; the two straight cases have already been taken, so the
 * remaining `curve == 0` default is unreachable -- but the pointer really is
 * initialised to null and would be dereferenced, which is where the `xor
 * eax,eax` at the top of the function comes from.  The 80 points are then
 *      angle = a0, stepping by (a1 - a0) / 80
 *      wob.x = (sin(angle * pi/180) + ox) * 640
 *      wob.y = (cos((angle + 180) * pi/180) + oy) * 640
 * The +180 on the y term negates the cosine, so the pair traces a quarter
 * circle of radius 640 (one square) recentred on the corner the boat turns
 * around.  The tables' angles run 0/90/180/270/360 and the offsets are all
 * +-1, i.e. exactly the four corners of the square.
 *
 * THE HEADING CODES.  Whatever produced the wobble, the tail then walks the
 * buffer and writes a 0..15 heading code per sub-step, from the direction
 * between the point 3 BEHIND and the point 4 AHEAD (both clamped to the ends
 * of the buffer):
 *      frame[j] = ((ArcTan256(dx, dy) >> 4) + 6) & 0xf
 * ArcTan256 (math3d.c) returns 256ths of a turn, so >> 4 is a sixteenth and
 * the + 6 is the sprite set's rotation offset.  The look-behind and
 * look-ahead are DELIBERATELY asymmetric (-3 / +4), which biases the boat's
 * heading a little into the turn ahead of it.  JcBoat_Step reads these codes
 * back through JungleCruise_UpdateRiverAnim to pick the hull sprite and to
 * turn the three riders.
 * ========================================================================= */

/* 330/330 instructions, 1108/1108 bytes; 327 match index for index and the
 * residual is three instructions at index 275-277, the straight-run y product
 * (0x00433bfd).  The original emits
 *      mov ecx, edi / mov [ebx-4], eax / imul ecx, [esi+0x4b714c]
 * (the counter copied into the destination, the table folded as the imul
 * memory operand) where every C spelling measured so far emits
 *      mov [ebx-4], eax / mov ecx, [esi+0x4b714c] / imul ecx, edi.
 * The unported boating-school twin of this function (0x004198xx, straight
 * loop at 0x00419c4f) has the identical form, so it is a property of the
 * shared source, not a fluke.
 *
 * WHAT DECIDES THE ORDER (measured on ~70 kernel functions, see
 * scratchpad/roads/k/): VC6 lowers `a * b` to `mov r, X / imul r, Y` by a
 * fixed RANK of the operands, never by source order.  A compiler temporary
 * (the result of an operation in the block -- sub, lea, and, movsx, a call
 * result, or a load that is CSE'd because it is textually repeated) always
 * becomes the destination copy, so the other operand is folded as the imul
 * memory operand: `(j - k) * g[1]`, `h(j) * g[1]` and `out[3] * g[1]; ..
 * out[3]` all give `imul r, [g]`.  A memory reference (global, static,
 * array element, pointer deref, whatever its base symbol) outranks a
 * register-candidate symbol (local, parameter, induction variable) and is
 * loaded into the destination: `j * g[1]` and `g[1] * j` both give
 * `mov r, [g] / imul r, j`.  Two symbols order by DECLARATION order, the
 * earlier one copied, the later one folded from its home slot if it still
 * lives there (`int f(int* o, int j, int k) { o[0] = k * j; return j; }`
 * gives `mov ecx, j / imul ecx, [esp+k]`; swap the parameter order and it
 * flips).  The original's y line is therefore MUL(temp, memref) with a temp
 * whose only definition is a register copy of the counter -- and no
 * spelling found makes VC6 keep such a copy: every identity is folded
 * BEFORE the ordering.
 *
 * Ruled out, each measured against the full function (unless noted, all
 * reproduce the same 3 mismatches): both source orders; `(long)`, `(int)`,
 * `(unsigned)` (changes the float conversion), `(int)(unsigned)`,
 * `(int)(__int64)`, pointer casts; `(j + 0)`, `-(-j)`, `~~j`, `j | 0`,
 * `j & j`, `(j + i) - i`, `(j ^ i) ^ i`, `j / 1`, `j * (i + 1 - i)`,
 * `j >> (i - i)`, comma and conditional forms (VC6 even folds
 * `from != to ? j : 0` on this path); `(j & 0xff)`, `(short)j`, `(j % 256)`,
 * `abs(j)` DO flip the order but keep their and/movsx/cdq (`(short)j` is
 * 1 mismatch: `movsx ecx, di` for `mov ecx, edi`), `__assume` does not
 * remove them; identity and multiply `static __inline` helpers (inlining
 * runs before the ordering), helpers with a path-dead early return; `k = j`
 * copies before or between the lines, `k = j; k *= dy`, `(k = j) * dy`,
 * `j++`/`k++` inside the product, a second identical IV (`j, k` both
 * stepping, VC6 merges them first), offset IVs `(k - 1)`, `(0x50 - k)`,
 * pointer differences (all keep real arithmetic: 13-300 mismatches); the
 * counter as `to`, `from`, `register`, `long`, `short` (widened plus a
 * trip counter, 61), `unsigned` (jb), a struct member, a fresh block
 * local; the table as `const`, a flat/2-D array, a byte-offset cast, a
 * union, a `JcStep*` or `int*` local (forward-substituted, no change),
 * `volatile`, a single-use `int dy` local (hoisted into ebp, 63); both
 * products through named int temps (12); holding the x result across the
 * y product (12); do/while, pointer loops, `j++` in the body (13).  The
 * hoisted `fild` pair, the shared [esp+0x1c] spill home and everything
 * else in the loop already match.
 *
 * 2026-09-04, endgame lane -- the CORPUS SCAN the method asks for was run and
 * it DID produce the one worked example, which is what finally explains why
 * this is unreachable.  scratchpad/endgame/scan_imul.py classifies every
 * two-operand `imul` in the 1541 exact bodies by how its destination was
 * seeded: 12 are `mov r,[mem] / imul r,reg` (rank-2 memory into the
 * destination, what we emit), 22 are `mov r,reg / imul r,reg`, and exactly
 * ONE is the shape needed here, `mov r,reg / imul r,[mem]` --
 * `SoftPrint_XBltFast` (bigrender.c 0x465a40, at 0x465bd6).  Read its note:
 * there the register operand is `g_ddsd.lPitch`, a value LOADED FROM MEMORY
 * for an earlier product and still live in edx, i.e. a rank-1 compiler
 * TEMPORARY, and the lever that closed it was spelling the two otherwise
 * identical products the OTHER way round so they are not linked as one
 * textual CSE candidate (linked, the live register is demoted to a
 * register-candidate SYMBOL and the memory operand becomes the destination).
 * That mechanism cannot transfer here, and the reason is now measured: our
 * register operand is `j`, a plain enregistered induction variable, which is
 * a register-candidate symbol at BOTH sites and can never be demoted or
 * promoted by spelling.  All 16 combinations of the operand order of the four
 * `g_jc_step[i].d? * <ctr>` product pairs in this function (the two in the
 * `to == -1` loop, the two pairs in the `from == to` loop and the straight
 * run's own) are byte-identical, first divergence 275 in every one -- so the
 * textual-CSE link has no effect when one operand is a symbol.  Isolated
 * kernels (scratchpad/endgame/jb1.c) confirm it: with the counter as an int
 * local, as a GLOBAL, through an `int*`, with both products on the same table
 * field spelled the same way and commuted, VC6 emits memory-into-destination
 * every time.  CONCLUSION: the y product's multiplier in the original is a
 * rank-1 temporary whose materialisation costs no instruction, and no C
 * expression turns an enregistered IV into one -- every identity is folded
 * before the ranking runs and every non-identity (`(short)j` reaches 1
 * mismatch with `movsx ecx,di`, `abs`, `&0xff`, a call, a subtraction) leaves
 * its own instruction behind.  Treat as exhausted.  (Related but NOT the same
 * unknown: `RequestRoute` index 38 and `InsertChildIntoList` index 46 both
 * need an EXTRA register-to-register copy that VC6 coalesced away for us,
 * whereas here the copy exists in both bodies and only its SOURCE differs --
 * register in the original, memory for us.  Do not conflate them.)
 *
 * ===================== RETIRED 2026-09-04 (lane H) =====================
 * FORMALLY EXHAUSTED -- do not spend another round on index 275.  The facts
 * that close it were all RE-MEASURED from scratch on today's toolchain before
 * this paragraph was written, so none of it is a stale claim:
 *   1. The residual is exactly 3, at 330/330 instructions and 1108/1108
 *      BYTES.  The body is byte-exact everywhere else, so nothing but the
 *      operand rank of this one `imul` is left to find.
 *   2. Both source orders of the y product (`j * g_jc_step[i].dy` and
 *      `g_jc_step[i].dy * j`) are BYTE-IDENTICAL, as are `(j + 0) * dy`,
 *      `(long)j * dy` and an `(int)` cast on the table operand.  Source order
 *      is not a lever and every zero-cost identity is folded before the
 *      ranking pass runs.
 *   3. `(short)j` reaches ONE mismatch -- `movsx ecx, di` where the original
 *      has `mov ecx, edi` -- at 1109 bytes.  That is simultaneously the proof
 *      of the mechanism and of its price: a cast is what promotes `j` from a
 *      register-candidate symbol (rank 3) to a compiler TEMPORARY (rank 1)
 *      and so folds the table into the imul, and every construct that manages
 *      it leaves its own instruction or its own byte behind.  NOT committed:
 *      it trades the exact byte length for a cosmetic 3 -> 1 and `movsx
 *      ecx,di` is certainly not what the original emitted.  (It is however
 *      semantically identical for 0 <= j < 0x50, so it is available if a
 *      future rule ever makes 1-mismatch/1-byte-over preferable.)
 *   4. `*(volatile int*)&j` as the multiplier does force a fresh temporary
 *      but spills the whole loop: 247 mismatches, 1135 bytes.
 *   5. The corpus scan (scratchpad/endgame/scan_imul.py) found exactly ONE
 *      `mov r,reg / imul r,[mem]` in the 1541 audit-exact bodies, and its
 *      register operand is a value LOADED FROM MEMORY for an earlier product
 *      -- a rank-1 temporary by construction.  Ours is an enregistered
 *      induction variable, rank 3 at every site, and all 16 commutations of
 *      this function's four product pairs are byte-identical, so the
 *      textual-CSE link that closed SoftPrint_XBltFast cannot transfer.
 * CONCLUSION: reproducing `mov ecx,edi / imul ecx,[g_jc_step+i*8+4]` needs a
 * C expression that turns an enregistered IV into a rank-1 temporary at zero
 * instruction and zero byte cost.  No such expression exists in VC6 SP3's IR:
 * anything that creates a temporary creates a tuple that emits code, and
 * anything that emits no code is folded before the ranking runs.  Behaviour,
 * the frame, the wobble tables and 327 of 330 instructions are exact; treat
 * the remaining 3 as unreachable, as with `UpdateControllerFromMouseData`.
 *
 * 2026-09-05, lane w11a.  Re-measured from scratch on today's toolchain:
 * baseline 330/330 instructions, 1108/1108 bytes, mismatch 3, first
 * divergence index 275 -- unchanged.  Three more spellings, none of them in
 * the lists above, all inert or worse:
 *   - the y product with the table operand FIRST (`g_jc_step[i].dy * j`):
 *     confirmed byte-identical to the committed `j * g_jc_step[i].dy`, so
 *     the "source order is not a lever" claim above is re-verified rather
 *     than inherited;
 *   - the y line reassociated so the float constant leads
 *     (`16.0f * (j * g_jc_step[i].dy) + y0`): byte-identical, 3 at 275 --
 *     float reassociation does not reach the integer product's ranking;
 *   - the straight loop written as an EXPLICIT pointer walk with a separate
 *     counter (`for (j = 0, w = b->wob; j < 0x50; j++, w++)`, storing through
 *     `w->x` / `w->y`): still 330/330 and 1108/1108 BYTES, but 5 mismatches
 *     with the first at 267 -- VC6's own strength reduction of `b->wob[j]`
 *     is what the original has, and writing the pointer out by hand perturbs
 *     the loop head without touching the imul.
 * FLOOR RE-ENDORSED at index 275. */
/* CLOSED (Scope F continuation, 2026-09-05): 330/330 instructions,
 * 1108/1108 bytes, strict/rb/ob 0/0/0, audit [OK].  The three-instruction
 * residual at 275-277 was never in the STRAIGHT loop it appeared in: it is
 * a remote effect of how the U-TURN loop above spells its x product.
 *
 * Naming the u-turn loop's x product in a `float` local -- either the whole
 * scaled product or just `dx * j` -- makes the straight loop 170 instructions
 * below emit the original's operand rank, `mov edx,edi / imul edx,[table]`,
 * instead of our `mov edx,[table] / imul edx,edi`.  The local is a FRONT-END
 * effect only: no store, reload, conversion or stack slot is emitted for it,
 * and the body is byte-for-byte the original either way (1108 bytes).  What
 * it changes is the typed temporary VC6 carries into its later ranking
 * decision -- which is why `double` is inert while `float` closes the body.
 *
 * LOAD-BEARING and measured on this body:
 *   - the local must carry the X expression: naming Y instead is inert (3),
 *     naming BOTH re-schedules the whole loop (283 strict, 1112B);
 *   - it must be `float`: `double` is inert (3);
 *   - it must be the product, not the sum: naming `(dx*j)*16.0f` gives 0 and
 *     naming `dx*j` gives 0, but naming `(dx*j)*16.0f + x0` is inert (3).
 * The earlier note's operand-rank "floor" was a floor only for the families
 * it had tested, all of which spelled the straight loop; that retirement is
 * superseded.  The same one-line change closes the twin JcBoat_Animate
 * (roads.c, 0x00433840) index for index, as its note predicted. */
// FUNCTION: LEGOLAND 0x00433840
void JcBoat_Animate(JcBoat* b, int from, int to)
{
    JcCurve* c = 0;
    int      i;
    int      j;
    int      k;
    int      x0;
    int      y0;

    if (to == -1) {
        if (from == to) {
            memset(b->wob, 0, sizeof(b->wob));
            goto frames;
        }
        for (i = 0; i < 4; i++) {
            if (from & (1 << i))
                break;
        }
        x0 = (int)((g_jc_step[i].sx * 40) * 16.0f);
        y0 = (int)((g_jc_step[i].sy * 40) * 16.0f);
        for (j = 0; j < 0x50; j++) {
            if (j < 0x28) {
                /* CODEGEN LEVER: this float local is what gives the STRAIGHT
                 * loop's y product (170 instructions below) the original's
                 * `mov r,j / imul r,[table]` operand rank; see the note above. */
                float fx = (g_jc_step[i].dx * j) * 16.0f;
                b->wob[j].x = (int)(fx + x0);
                b->wob[j].y = (int)((g_jc_step[i].dy * j) * 16.0f + y0);
            } else {
                b->wob[j].x = 0;
                b->wob[j].y = 0;
            }
        }
        goto frames;
    }
    if (from == -1) {
        for (i = 0; i < 4; i++) {
            if (to & (1 << i))
                break;
        }
        k = (i + 2) % 4;
        for (j = 0; j < 0x50; j++) {
            if (j >= 0x28) {
                b->wob[j].x = b->wob[j - 1].x + g_jc_step[k].dx * 16;
                b->wob[j].y = b->wob[j - 1].y + g_jc_step[k].dy * 16;
            } else {
                b->wob[j].x = 0;
                b->wob[j].y = 0;
            }
        }
        goto frames;
    }
    if (from == 1)
        from = 0x11;
    if (to == 1)
        to = 0x11;
    if (to < from) {
        if (!(to & (from >> 2)))
            goto curve;
    } else if (to != from) {
        if (!(from & (to >> 2)))
            goto curve;
    }
    for (i = 0; i < 4; i++) {
        if (from & (1 << i))
            break;
    }
    x0 = (int)((g_jc_step[i].sx * 40) * 16.0f);
    y0 = (int)((g_jc_step[i].sy * 40) * 16.0f);
    if (from == to) {
        for (j = 0, k = 0x50; k > 0; j++, k--) {
            if (k > 0x28) {
                b->wob[j].x = (int)((g_jc_step[i].dx * j) * 16.0f + x0);
                b->wob[j].y = (int)((g_jc_step[i].dy * j) * 16.0f + y0);
            } else {
                b->wob[j].x = (int)((g_jc_step[i].dx * k) * 16.0f + x0);
                b->wob[j].y = (int)((g_jc_step[i].dy * k) * 16.0f + y0);
            }
        }
        goto frames;
    }
    for (j = 0; j < 0x50; j++) {
        b->wob[j].x = (int)((g_jc_step[i].dx * j) * 16.0f + x0);
        b->wob[j].y = (int)((j * g_jc_step[i].dy) * 16.0f + y0);
    }
    goto frames;

curve:
    if (to & (from * 2))
        c = g_jc_curve_cw;
    else if (from & (to * 2))
        c = g_jc_curve_ccw;
    for (i = 0; i < 4; i++) {
        if (from & (1 << i))
            break;
    }
    {
        float step = (c[i].a1 - c[i].a0) * 0.012500000186264515f;
        float a = c[i].a0;

#ifndef LEGOLAND_PORTABLE
        b->wob[0].x = (int)(((float)sin(a * 0.01745329238474369f) + c[i].ox) * 640.0f);
        b->wob[0].y = (int)(((float)cos((a + 180.0f) * 0.01745329238474369f) + c[i].oy) * 640.0f);
#else
        /* PORT-M5: the ARC arms convert a genuinely fractional value through
         * 0x00458930, which ROUNDS.  The straight/drift arms of this function
         * are insensitive -- `(int)((integer) * 16.0f + integer)` has no
         * fraction to round -- so only these four sites change. */
        b->wob[0].x = LL_FISTP(((float)sin(a * 0.01745329238474369f) + c[i].ox) * 640.0f);
        b->wob[0].y = LL_FISTP(((float)cos((a + 180.0f) * 0.01745329238474369f) + c[i].oy) * 640.0f);
#endif
        j = 1;
        k = 0x4f;
        do {
            a += step;
#ifndef LEGOLAND_PORTABLE
            b->wob[j].x = (int)(((float)sin(a * 0.01745329238474369f) + c[i].ox) * 640.0f);
            b->wob[j].y = (int)(((float)cos((a + 180.0f) * 0.01745329238474369f) + c[i].oy) * 640.0f);
#else
            b->wob[j].x = LL_FISTP(((float)sin(a * 0.01745329238474369f) + c[i].ox) * 640.0f);   /* PORT-M5 */
            b->wob[j].y = LL_FISTP(((float)cos((a + 180.0f) * 0.01745329238474369f) + c[i].oy) * 640.0f);
#endif
            j++;
        } while (--k);
    }

frames:
    {
        JcWob* p;
        int*   q;

        for (j = 0, q = b->frame, p = b->wob - 3; j < 0x50; j++, q++, p++) {
            int dx;
            int dy;

            if (j < 0x4c) {
                dx = p[7].x;
                dy = p[7].y;
            } else {
                dx = b->wob[0x4f].x;
                dy = b->wob[0x4f].y;
            }
            if (j > 3) {
                dx -= p[0].x;
                dy -= p[0].y;
            } else {
                dx -= b->wob[0].x;
                dy -= b->wob[0].y;
            }
            *q = ((ArcTan256(dx, dy) >> 4) + 6) & 0xf;
        }
    }
}
