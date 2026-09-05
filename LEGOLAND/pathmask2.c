/* LEGOLAND -- the build cursor's path-tile painter, the entrance flood fill
 * and the rider-list cursor seek: the three callees scope K's pathmask.c and
 * texture.c declared and did not define.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, global addresses and callee argument counts are load-bearing;
 * names are ours.
 *
 * ---------------------------------------------------------------------------
 * DrawCursorPathTile (0x00460f50) is render5.c's DrawPathTileOverlay with a
 * blit mode: the base tile is still stamped through the three-argument
 * PrintSpriteAt, but the corner filler goes through the five-argument
 * PrintSprite so the caller's mode (render5.c's PaintCursorTiles hands the
 * cursor's mode down through pathmask.c's DrawCursorTileAt) reaches the blit.
 * The tile arithmetic is identical: PathEdgeMask's four-neighbour code indexes
 * g_tile_sprites[code + 3 + edges], PathCornerMask's 2-bit answer picks
 * g_tile_sprites[code + 19 .. 21], `code` being the first dword of the loaded
 * "path" tile record.
 *
 * MarkPathSquareReachable (0x004829c0) is the write side of the path-square
 * connectivity that pathmask.c's ResolveEntrancePathSquare starts: set flag 2
 * ("reachable from the entrance") on the square, ask 0x004819a0 for every
 * square touching its rectangle, and recurse into each one that does not
 * carry the flag yet.  The neighbour table is a GLOBAL that the recursion
 * would overwrite, so the function snapshots it into a heap copy first
 * (malloc'd count*4 bytes, memcpy'd, freed after the loop); with no
 * neighbours, or if the allocation fails, nothing further happens.
 *
 * RiderCursorSeek (0x00441830) is the shared helper texture.c's
 * ObjFirstRider/ObjNextRider tail into: from the global cursor onward, find
 * the first rider node whose 16-bit key equals the caller's, park the cursor
 * on it and return it, or clear the cursor and return 0.  Its `item`
 * argument is dead.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

#pragma intrinsic(memcpy, memcmp)
void* memcpy(void*, const void*, unsigned int);
int   memcmp(const void*, const void*, unsigned int);

/* ---- types -------------------------------------------------------------- */

/* An inclusive rectangle with no list link (pathmisc2.c's Rect4). */
typedef struct Rect4 {
    int left;    /* +0x00 */
    int top;     /* +0x04 */
    int right;   /* +0x08 */
    int bottom;  /* +0x0c */
} Rect4;

/* One path square (pathsq.c's PathSquare, 0x24 bytes). */
typedef struct PathSquare {
    struct PathSquare* next;   /* +0x00 */
    int   pad04;               /* +0x04 */
    Rect4 rect;                /* +0x08 */
    void* rect_next;           /* +0x18 */
    int   distance2;           /* +0x1c */
    int   flags;               /* +0x20 bit 1 = reachable from the entrance */
} PathSquare;

/* One entry of an item's rider list (texture.c's RiderNode). */
typedef struct RiderNode RiderNode;
struct RiderNode {
    RiderNode*     next;      /* +0x00 */
    int            pad04[2];  /* +0x04 */
    unsigned short key;       /* +0x0c  matched against *(u16*)inst */
};

/* The item a rider list hangs off (texture.c's RiderItem); unused here. */
typedef struct RiderItem RiderItem;

/* The loaded "path" tile record (render5.c's PathTileRec). */
typedef struct PathTileRec {
    int code;      /* +0x00 first tile index of the path run */
} PathTileRec;

/* ---- globals ------------------------------------------------------------ */
extern PathTileRec* g_path_tile;                          /* 0x00832bf0 */
extern RiderNode*   g_rider_cursor;                       /* 0x0081c8cc */
/* pathsq.c's neighbour table and, first named here, the count 0x004819a0
 * leaves beside it. */
extern PathSquare*  g_path_square_neighbours[];           /* 0x0066a45c */
extern int          g_path_square_neighbour_count;        /* 0x00669254 */

/* ---- callees ------------------------------------------------------------ */
extern char  PathEdgeMask(Pos* at);                       /* 0x0045ceb0 */
extern char  PathCornerMask(char edges, Pos* at);         /* 0x0045d080 */
extern void  PrintSpriteAt(Sprite* s, int x, int y);      /* 0x00485f00 */
extern int   PrintSprite(Sprite* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
/* 0x004819a0 (not exported): fill g_path_square_neighbours with every square
 * touching `rect` and leave the count in g_path_square_neighbour_count. */
extern void  CollectPathSquareNeighboursCounted(Rect4* rect); /* 0x004819a0 */
extern void* HeapAlloc_w(unsigned int n);                 /* 0x0049e4ff (CRT malloc) */
extern void  HeapFree_w(void* p);                         /* 0x0049e4d0 */


/* =========================================================================
 *  RiderCursorSeek -- advance the shared rider cursor to the next key match
 * ========================================================================= */

/* The key test is `memcmp(&p->key, inst, 2) == 0`: the original materialises
 * the key's ADDRESS (`lea edx,[eax+0xc]`) before the word compare, which is
 * the recorded signature of the intrinsic two-byte memcmp (LEVERS RC01), and
 * a scalar `p->key == *(u16*)inst` has no such lea. */
// FUNCTION: LEGOLAND 0x00441830
RiderNode* RiderCursorSeek(RiderItem* item, void* inst)
{
    RiderNode* p = g_rider_cursor;

    while (p) {
        if (memcmp(&p->key, inst, 2) == 0) {
            g_rider_cursor = p;
            return p;
        }
        p = p->next;
    }
    g_rider_cursor = 0;
    return 0;
}


/* =========================================================================
 *  MarkPathSquareReachable -- the entrance flood fill, one square at a time
 * ========================================================================= */

/* THE COUNT IS READ FROM THE GLOBAL AT EVERY USE, and `n` is a separate copy
 * taken BEFORE the guard.  The original's `mov eax,[count] / test eax,eax /
 * mov ebp,eax / je / lea esi,[eax*4]` is one CSE'd load (eax) feeding the test
 * and the malloc size, plus an unconditional copy into ebp for the loop bound
 * -- the copy has to exist because the recursion below overwrites the global.
 * Measured: sizing with `n * 4` fuses the two webs (`mov ebp,[count] / test
 * ebp,ebp / lea esi,[ebp*4]`), which also drops edi's prologue push -- the
 * flag read-modify-write then takes esi and the memcpy gets a local `push edi
 * / pop edi` bracket -- 40 of 51 at the original's byte length, and the
 * early-return, explicit-size and pointer-walk loop spellings are all that
 * same body; sizing from the global with `n = ...` INSIDE the guard is 51 of
 * 52 (the copy lands after the lea); `n = ...` before the guard with the
 * guard on the GLOBAL closes it, and guarding on `n` instead reverts to 40 of
 * 51.  The RMW's spelling (`|=`, or a named temporary before or after the OR)
 * is inert.  The variable-length memcpy is the intrinsic's `rep movsd` +
 * `rep movsb` pair, and the counted `for` over the copy becomes a countdown
 * on `n` with a walking cursor, guarded by the signed `test ebp,ebp / jle`. */
// FUNCTION: LEGOLAND 0x004829c0
void MarkPathSquareReachable(PathSquare* sq)
{
    int          n;
    PathSquare** list;
    int          i;
    int          size;

    sq->flags |= 2;
    CollectPathSquareNeighboursCounted(&sq->rect);
    n = g_path_square_neighbour_count;
    if (g_path_square_neighbour_count) {
        size = g_path_square_neighbour_count * 4;
        list = (PathSquare**)HeapAlloc_w(size);
        if (list) {
            memcpy(list, g_path_square_neighbours, size);
            for (i = 0; i < n; i++) {
                if (!(list[i]->flags & 2))
                    MarkPathSquareReachable(list[i]);
            }
            HeapFree_w(list);
        }
    }
}


/* =========================================================================
 *  DrawCursorPathTile -- paint one cursor path cell plus its corner filler
 * ========================================================================= */

/* render5.c's DrawPathTileOverlay body with the corner draws routed through
 * the five-argument PrintSprite.  As there, the edge mask is a CHAR local
 * homed in the DEAD arg0 slot (`mov byte ptr [esp+0x18],al` after `at` has
 * been root-copied into esi), so passing it on loads the whole stale dword
 * and the index needs the explicit `& 0xff`; and the switch lowers to a
 * `dec/je` chain laid out in REVERSE case order (3 inline, then 2, then 1).
 * All three calls before the switch share ONE `add esp,0x18`. */
// FUNCTION: LEGOLAND 0x00460f50
void DrawCursorPathTile(Pos* at, int x, int y, int mode)
{
    char edges;
    int  corner;

    edges  = PathEdgeMask(at);
    corner = PathCornerMask(edges, at) & 0xff;
    PrintSpriteAt(g_tile_sprites[(edges & 0xff) + g_path_tile->code + 3], x, y);
    switch (corner) {
    case 1:
        PrintSprite(g_tile_sprites[g_path_tile->code + 19], x, y, mode, 0);
        break;
    case 2:
        PrintSprite(g_tile_sprites[g_path_tile->code + 20], x, y, mode, 0);
        break;
    case 3:
        PrintSprite(g_tile_sprites[g_path_tile->code + 21], x, y, mode, 0);
        break;
    }
}
