/* LEGOLAND — the build cursor's tile stamp and its deferred sprite queue, the
 * path-tile overlay, the render-node column take, the on-screen text cache's
 * expiry sweep, and the certificate bitmap dump.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field offsets, global addresses and callee argument counts are load-bearing;
 * names are ours.
 *
 * render4.c's DrawEditCursor (0x00461220) calls PaintCursorTiles, which walks
 * the diamond grid under the cursor rectangle and hands every cell to
 * 0x00461080; that keeps the path cells and paints them THROUGH 0x00460f50,
 * the four-argument twin of this file's DrawPathTileOverlay.  So the cursor
 * paints immediately — see the note on FlushCursorSpriteList, whose deferred
 * sprite list turns out to be vestigial.
 *
 * DrawPathTileOverlay is the mode-0 half of the path layer: one base tile
 * indexed by the four-neighbour connection mask, plus at most one corner
 * filler chosen by a 2-bit code.  Its twin 0x00460f50 is the same body with
 * the corner draws routed through the full five-argument PrintSprite so the
 * caller can pass a blit mode.
 *
 * SaveCertificateBitmap is the certificate screen's "print" — a screen grab
 * to EGC.bmp stamped with the CRT's asctime(localtime()).
 */
#include "legoland.h"

#pragma intrinsic(strlen)
extern unsigned int strlen(const char* s);

/* ---- types -------------------------------------------------------------- */

/* A render node in the 0x1000-entry table (render3.c / objmap2.c). */
typedef struct RenderNode {
    int            live;         /* +0x00 */
    unsigned short bpos;         /* +0x04 packed base cell */
    unsigned char  x;            /* +0x06 column to resume at */
    unsigned char  y;            /* +0x07 row to resume at */
} RenderNode;

/* One entry of the deferred cursor-sprite list at 0x00801420 (stride 0x10):
 * the five arguments PrintSprite is replayed with, ctx excepted. */
typedef struct CursorSprite {
    Sprite* s;      /* +0x00 */
    int     x;      /* +0x04 */
    int     y;      /* +0x08 */
    int     mode;   /* +0x0c */
} CursorSprite;

/* Only SpriteRec::detail is read here (sprite2.c's header names the rest). */
typedef struct SpriteRec {
    char pad0[0x0c];  /* +0x00 */
    int  detail;      /* +0x0c  g_detail - 1 at creation */
} SpriteRec;

/* One rendered-text cache entry (fpui3.c's TextEntry, 0x20 bytes). */
typedef struct TextEntry {
    int     w;        /* +0x00 */
    int     h;        /* +0x04 */
    int     format;   /* +0x08 */
    char*   text;     /* +0x0c */
    int     ink;      /* +0x10 */
    int     paper;    /* +0x14 */
    int     font;     /* +0x18 */
    SpriteRec* sprite;/* +0x1c the rendered bitmap */
} TextEntry;

/* The loaded "path" tile record at 0x00832bf0; only the leading tile code is
 * read here, and it is read as a DWORD (maprestore.c and pathbuild.c read the
 * same word 16 bits wide from their side — the divergence is deliberate). */
typedef struct PathTileRec {
    int code;      /* +0x00 first tile index of the path run */
} PathTileRec;

/* An inclusive pixel rectangle of a map tile (pathbuild.c's GetTileBounds). */
typedef struct TileBounds {
    int left;   /* +0x00 */
    int top;    /* +0x04 */
    int right;  /* +0x08 */
    int bottom; /* +0x0c */
} TileBounds;


/* ---- globals ------------------------------------------------------------ */
extern RenderNode    g_render_nodes[0x1000];    /* 0x00807f60 */
extern CursorSprite  g_cursor_sprites[];        /* 0x00801420 */
extern CursorSprite* g_cursor_sprite_put;       /* 0x004b95f0 append cursor */
extern int           g_cursor_sprite_count;     /* 0x00667d44 */
extern int           g_text_cache_count;        /* 0x006675b8 */
extern TextEntry     g_text_cache[];            /* 0x006675c0 */
extern int           g_detail;                  /* 0x008119a4 */
extern char          g_cert_message[];          /* 0x0080ffa0 */
extern const char    kEgcBmpPath[];             /* 0x004b86fc "EGC.bmp" */
extern PathTileRec*  g_path_tile;               /* 0x00832bf0 */

/* ---- callees ------------------------------------------------------------ */
extern int PrintSprite(Sprite* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
/* 0x00455ee0 (not exported): drop text-cache entry `i` and compact the array. */
extern void FreeCachedTextEntry(int i);                              /* 0x00455ee0 */
/* 0x00451740 (not exported): grab the locked video surface into a 24-bpp .BMP
 * at `path` and format the saved path into `msg`, stamping it with `stamp`.
 * Non-zero on success. */
extern int  SaveScreenshotBmp(const char* path, char* msg,
                              const char* stamp);                    /* 0x00451740 */
/* 0x0045ceb0 (not exported): the four-neighbour path connection mask of a
 * cell (bits 0x1/0x2 one diagonal pair, 0x4/0x8 the other). */
extern char PathEdgeMask(Pos* at);                                   /* 0x0045ceb0 */
/* 0x0045d080 (not exported): the 2-bit corner-filler code for a cell whose
 * edge mask completes one or both pairs. */
extern char PathCornerMask(char edges, Pos* at);                     /* 0x0045d080 */
/* 0x00485f00: PrintSprite(s, x, y, 0, 0). */
extern void PrintSpriteAt(Sprite* s, int x, int y);                  /* 0x00485f00 */
/* 0x00461080 (not exported): bounds-check `at`, and if the cell is a path
 * cell with a non-zero tile, hand it to the cursor painter at 0x00460f50. */
extern void DrawCursorTileAt(Pos* at, int x, int y, int mode);       /* 0x00461080 */
/* objmap.c defines this returning its own 8-byte Offset; the pair is a map
 * coordinate here, so it is declared Pos (the divergence is deliberate). */
extern Pos  PlayfieldToMap(int x, int y);                            /* 0x0045a970 */
extern void GetTileBounds(Pos* tile, TileBounds* out);               /* 0x0045acc0 */
extern long  time(long* t);                                          /* 0x0049fbc6 */
extern void* localtime(const long* t);                               /* 0x0049fa66 */
extern char* asctime(const void* tm);                                /* 0x0049f990 */

/* =========================================================================
 *  TakeRenderNodeInColumn — consume the render node parked in one column
 * ========================================================================= */

/* The scan-resume half of the render-order builder (render3.c's
 * CalculateFullMapRenderOrder and objmap2.c's CalculateMapRenderOrder).  A
 * node records "when the column scan next reaches column x, resume at row y";
 * this call looks the current column up, hands the row back in p->y and
 * retires the node.  With no node parked in the column the scan restarts at
 * row 0.  The table is a flat 0x1000-entry ring; `live` is the occupancy
 * flag, and it is cleared AFTER p->y is stored (the store order is visible in
 * the original).
 *
 * The cursor anchors at +6 (`x`) rather than +0: the two fields read in the
 * loop have one reference each and the tie breaks to the LAST, exactly as the
 * recorded strength-reduction rule says (offset 0 never wins).  p->x is
 * re-read on every live entry — the pointer parameter defeats the hoist —
 * so it is spelled inline in the compare. */
// FUNCTION: LEGOLAND 0x0045a3e0
void TakeRenderNodeInColumn(Pos* p)
{
    int i;

    for (i = 0; i < 0x1000; i++) {
        if (g_render_nodes[i].live && g_render_nodes[i].x == p->x) {
            p->y = g_render_nodes[i].y;
            g_render_nodes[i].live = 0;
            return;
        }
    }
    p->y = 0;
}


/* =========================================================================
 *  FlushCursorSpriteList — paint and empty the deferred cursor sprite list
 * ========================================================================= */

/* renderview.c's RenderView calls DrawEditCursor (0x00461220) and then this,
 * both inside PushRenderingStatusAndLockVideoSurface/PopRenderingStatus.  The
 * cursor pass does not paint directly: it APPENDS {sprite, x, y, mode} records
 * to a flat 16-byte-stride array at 0x00801420 through the append cursor at
 * 0x004b95f0, with the count at 0x00667d44 (PaintCursorTiles below is the
 * producer).  This call replays them in order with PrintSprite's ctx = 0 and
 * resets both the count and the cursor.
 *
 * The list is walked through the GLOBAL cursor, not a local: the original
 * reloads 0x004b95f0 after every call and stores it back with the `add
 * eax,0x10` — that is the recorded "walk a list through its head global"
 * shape.  The count is likewise reloaded in the latch (PrintSprite may
 * append), while the guard's load is hoisted above `push esi`.
 *
 * The loop must be a `while` with `i++` written as a statement BEFORE the
 * cursor advance: in `for (...; i++)` VC6 emits `add eax,0x10` before
 * `inc esi` and sinks the store above the compare (2 of 29 at identical
 * length).  The recorded "a decrement in the for-increment vs the body
 * reorders two ALU ops" rule, measured from the increment side. */
// FUNCTION: LEGOLAND 0x00461020
void FlushCursorSpriteList(void)
{
    int i;

    g_cursor_sprite_put = g_cursor_sprites;
    i = 0;
    while (i < g_cursor_sprite_count) {
        PrintSprite(g_cursor_sprite_put->s, g_cursor_sprite_put->x,
                    g_cursor_sprite_put->y, g_cursor_sprite_put->mode, 0);
        i++;
        g_cursor_sprite_put++;
    }
    g_cursor_sprite_count = 0;
    g_cursor_sprite_put = g_cursor_sprites;
}


/* =========================================================================
 *  ExpireCachedText — drop stale entries from the rendered-text cache
 * ========================================================================= */

/* render2.c's RenderingComplete calls this once a frame with all = 0; a
 * non-zero argument empties the cache outright.  The cache is fpui3.c's flat
 * 0x20-byte TextEntry array at 0x006675c0 with its count at 0x006675b8, and
 * 0x00455ee0 removes ONE entry by index and compacts the array — which is why
 * the index is NOT advanced on the free arm and why the count is re-read on
 * every iteration.
 *
 * THE AGE TEST IS A DETAIL-GENERATION TEST, not a clock: sprite2.c's
 * NewSprite stamps SpriteRec::detail (+0x0c) with `g_detail - 1` at creation,
 * and the test here is `(unsigned)(g_detail - sprite->detail) > 10`.  So an
 * entry survives while the global detail counter has not moved more than ten
 * steps past the value it was rasterised at; the compare is UNSIGNED, so a
 * counter that goes BACKWARDS expires the whole cache immediately.
 *
 * The cursor anchors at +0x1c (`sprite`), the only field the loop reads, and
 * both callee-saved pushes sink past the count guard into the loop.
 *
 * The `volatile` on the count is FREE and worth 3 of 29: without it VC6
 * hoists the guard's load ABOVE `push esi` (a volatile access cannot cross
 * the push's store) and folds the latch's reload into `cmp esi,[mem]` instead
 * of the original's `mov eax,[mem] / cmp esi,eax`.  Same lever, same shape,
 * as fpui3.c's FindCachedText — the two halves of this cache need it. */
// FUNCTION: LEGOLAND 0x00455f70
void ExpireCachedText(int all)
{
    int i;

    for (i = 0; i < *(volatile int*)&g_text_cache_count; ) {
        if (all
         || (unsigned int)(g_detail - g_text_cache[i].sprite->detail) > 10)
            FreeCachedTextEntry(i);
        else
            i++;
    }
}


/* =========================================================================
 *  SaveCertificateBitmap — dump the certificate screen to EGC.bmp
 * ========================================================================= */

/* mapscreen2.c's PrintScreenMode8 runs this when the "saving" counter reaches
 * zero and turns the answer into g_cert_result = +140 / -140 (the success and
 * failure strings shown for 140 frames).
 *
 * The screen grabber at 0x00451740 takes THREE arguments — the output path,
 * the message buffer it formats the resulting path into (g_cert_message, the
 * 0x0080ffa0 buffer PrintScreenMode8 centres under the certificate box) and a
 * TIMESTAMP string.  The timestamp is the CRT's `asctime(localtime(&t))`, and
 * because asctime's result always ends in "\n" the call strips it in place —
 * `s[strlen(s) - 1] = 0`, with the zero byte coming free out of the inlined
 * strlen's own `xor eax,eax` (the store is `mov [ecx+edx-1],al`).
 *
 * All four calls share ONE `add esp,0x18`: nothing between them consumes a
 * result on the stack, so VC6 merges the whole 24 bytes of __cdecl cleanup.
 * `return ... != 0` is the recorded neg/sbb/neg. */
// FUNCTION: LEGOLAND 0x00451e20
int SaveCertificateBitmap(void)
{
    long  t;
    char* stamp;

    time(&t);
    stamp = asctime(localtime(&t));
    stamp[strlen(stamp) - 1] = 0;
    return SaveScreenshotBmp(kEgcBmpPath, g_cert_message, stamp) != 0;
}


/* =========================================================================
 *  DrawPathTileOverlay — paint one path cell plus its corner filler
 * ========================================================================= */

/* The path layer is drawn from ONE base tile plus at most one corner filler.
 * 0x0045ceb0 answers the four-neighbour connection mask for the cell (bits
 * 0x1/0x2 are one diagonal pair, 0x4/0x8 the other) and that mask indexes the
 * sixteen path tiles at g_tile_sprites[code + 0 .. code + 15], where `code` is
 * the first dword of the loaded "path" tile record at 0x00832bf0 and the base
 * of the run is code + 3.  0x0045d080 then answers a 2-bit corner code: bit 0
 * when the 0xc pair is complete but the cell diagonally beyond it is NOT a
 * path, bit 1 for the 0x3 pair, and the three combinations pick
 * g_tile_sprites[code + 19], [code + 20] and [code + 21].
 *
 * Two things are read straight off the listing.  The edge mask is a CHAR
 * local homed in the DEAD arg0 SLOT (`mov byte ptr [esp+0x18],al` writes into
 * the incoming `at` slot once `at` has been root-copied into esi), which is
 * why passing it on loads the whole dword and why the index needs the
 * explicit `& 0xff`.  And the switch's blocks come out in REVERSE source
 * order — case 3 inline, then 2, then 1 — the recorded case-order layout
 * rule, with `dec/je` chains rather than a jump table. */
// FUNCTION: LEGOLAND 0x00460e90
void DrawPathTileOverlay(Pos* at, int x, int y)
{
    char edges;
    int  corner;

    edges  = PathEdgeMask(at);
    corner = PathCornerMask(edges, at) & 0xff;
    PrintSpriteAt(g_tile_sprites[(edges & 0xff) + g_path_tile->code + 3], x, y);
    switch (corner) {
    case 1:
        PrintSpriteAt(g_tile_sprites[g_path_tile->code + 19], x, y);
        break;
    case 2:
        PrintSpriteAt(g_tile_sprites[g_path_tile->code + 20], x, y);
        break;
    case 3:
        PrintSpriteAt(g_tile_sprites[g_path_tile->code + 21], x, y);
        break;
    }
}


/* =========================================================================
 *  PaintCursorTiles — stamp the build cursor over a rectangle of tiles
 * ========================================================================= */

/* render4.c's DrawEditCursor (0x00461220) calls this with the cursor's
 * PLAYFIELD position and the clip rectangle; renderview.c's RenderView then
 * replays the queue with FlushCursorSpriteList above.  Each visited map cell
 * goes to 0x00461080, which bounds-checks it, keeps only path cells with a
 * non-zero tile, and hands the survivor to the cursor painter at 0x00460f50.
 *
 * THE GEOMETRY.  A tile is `w = 2h` pixels wide and `h` tall (the same short
 * arithmetic pathbuild.c's GetTileBounds does on the DEFAULT tile sprite), and
 * the grid is a diamond: from the cell under (x, y), the cell under
 * (x - w/2, y + h/2) is one step further in +y, and the cell under (x + w,
 * y) is +1 in x and -1 in y.  So the inner loop paints TWO cells per full
 * tile width — the row cell and the half-step cell below-left — and steps
 * (map.x + 1, map.y - 1) each time; the outer loop restarts the row at
 * (row.x + 1, row.y + 1) and steps y by one tile height.
 *
 * THE CALLER'S Pos IS MODIFIED: `p->x -= w` is written back through the
 * pointer before the playfield-to-map conversion, so the caller's cursor
 * origin moves one full tile width left.  Reproduced as written.
 *
 * Two spellings are load-bearing.  `short h` / `short w = (short)(h + h)`
 * gives the `mov ax / lea / movsx / movsx` opening, exactly as in
 * GetTileBounds; and the two half-steps are INT locals with the `(short)`
 * cast at their USE sites — that is what emits `movsx edi,si` in the
 * preheader and `movsx edx, word ptr [esp+0x10]` inside the loop against
 * dword stores in the prologue. */
// FUNCTION: LEGOLAND 0x004610f0
void PaintCursorTiles(Pos* p, TileBounds* clip)
{
    short      h = g_tile_sprites[g_default_tile]->h;
    short      w = (short)(h + h);
    int        hw = (w + 1) >> 1;
    int        hh = (h + 1) >> 1;
    Pos        map;
    Pos        row;
    TileBounds b;
    int        x, y;

    p->x -= w;
    map = PlayfieldToMap(p->x, p->y);
    GetTileBounds(&map, &b);
    row = map;
    for (y = b.top; y < clip->bottom; y += h) {
        for (x = b.left; x < clip->right + (short)hw; x += w) {
            DrawCursorTileAt(&map, x, y, 0);
            map.y++;
            DrawCursorTileAt(&map, x - (short)hw, y + (short)hh, 0);
            map.y -= 2;
            map.x++;
        }
        row.x++;
        row.y++;
        map.x = row.x;
        map.y = row.y;
    }
}
