/* LEGOLAND -- the two map renderers: the isometric world view and the
 * overview ("full map") mini-map draw callback.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field offsets, global addresses and callee arg counts are load-bearing;
 * names are ours. No legoland.h: every type is defined locally.
 *
 * ===========================================================================
 * RenderView (0x0045b180) -- THE DRAW ORDER
 * ===========================================================================
 * The world view is a 2:1 isometric ("diamond") projection.  A tile's pixel
 * rectangle is GetTileBounds (pathbuild.c):
 *
 *     h  = g_tile_sprites[g_default_tile]->h        (tile pixel height)
 *     w  = 2*h                                      (tile pixel width)
 *     left = ((w+1)>>1) * (tx - ty - 1) - (g_scroll_x>>8) + map->origin_x
 *     top  = ((h+1)>>1) * (tx + ty)     - (g_scroll_y>>8) + map->origin_y
 *     right = left + w - 1 ; bottom = top + h - 1
 *
 * so screen x depends on (tx-ty) and screen y on (tx+ty).  The view rectangle
 * is {map->origin_x, map->origin_y, +view_w, +view_h} (Map +0x20/+0x22 origin,
 * +0x10/+0x12 size); scroll is 24.8 fixed point in g_scroll_x/g_scroll_y.
 *
 * RenderView draws a frame in FIVE stages, in this order:
 *
 *   1. GROUND.  g_bg_full_update (0x4b9220) is forced to 1 and the ground
 *      pass (0x00460e00) repaints the whole terrain/tile layer for the view
 *      rectangle, then clears the dirty flags (0x667cd0/0x667cd4) and
 *      g_bg_full_update.  PrintBackground (0x464360, an empty stub in the
 *      shipped build) is then called with the two dirty-rect words.
 *      Everything after this stage goes through the DEPTH-SORTED print list
 *      (printlist.c), so its order in the source is NOT the order on screen:
 *      the sort key decides that.
 *
 *   2. VISIBLE-OBJECT GATHER (pass 1).  Walk every map cell whose diamond can
 *      touch the view rectangle and collect the DISTINCT objects under them
 *      into a local 3000-entry array.  The walk is the interesting part:
 *
 *        qx = (g_scroll_x>>8)/w   rx = (g_scroll_x>>8)%w
 *        sy = (g_scroll_y>>8) - (h+1)/2
 *        qy = sy/h                ry = sy%h
 *        tile.x = qx + qy - 3     tile.y = qy - qx
 *
 *      The sub-tile remainder (rx, ry) says which quarter of the top-left
 *      diamond the view corner falls in; a 4-way switch (quadrant =
 *      1 + (rx >= w/2) + 2*(ry > h/2)) nudges (tile.x, tile.y) and (rx, ry)
 *      by half a tile so the walk starts on the diamond that actually covers
 *      the corner.  Then:
 *
 *        for (py = top - 2h - ry; py < view_h + bottom; py += h)   // rows
 *            for (px = left - 2w - rx; px < 2w + right; px += w) { // columns
 *                visit(tile.x, tile.y); tile.x++;                  // cell A
 *                visit(tile.x, tile.y);                            // cell B
 *                tile.y--;
 *            }
 *            tile.x = rowstart.x + 1; tile.y = rowstart.y + 1;     // next row
 *
 *      i.e. each column step is +1 in map x and -1 in map y (screen x += w),
 *      each row step is +1 in BOTH (screen y += h); the two cells visited per
 *      column step are the two half-offset diamonds that share the column.
 *      Stepping right by one cell is `cell + 1` in the row (cells are 20 B),
 *      and the column loop breaks as soon as tile.x reaches map->width.
 *      An empty per-cell hook (0x0045b170, a bare `ret` in the shipped build
 *      -- a stripped debug probe) is called with &tile for every visited cell.
 *
 *      visit(): if the cell's map flags carry 0xa0 (0x80 footprint cell or
 *      0x20 render-chain base cell) the cell's OWNER is Cell +0x04/+0x05 (the
 *      packed base coordinate); that owner cell is appended to the array
 *      unless its flag bit 0x400 is already set, and 0x400 is then set.  So
 *      0x400 is the per-frame "already queued" mark and each object is
 *      collected exactly once however many of its footprint cells are on
 *      screen.  (If the packed base coordinate is off the map the inlined
 *      lookup yields NULL and the flag word is read through it -- an original
 *      bug, harmless because AddObjectToMap always writes an in-map base.)
 *
 *   3. PER-CLASS PRE-RENDER.  Walk the ODF class list (head 0x669240, link at
 *      +0x00): every class with flag 0x20 whose +0xa8 hook is non-NULL gets
 *      hook(class->+0xc4) called once per frame, before any object is queued.
 *
 *   4. OBJECT QUEUE (pass 2).  For each collected owner cell, in gather order:
 *        - clear its 0x400 mark;
 *        - skip it if Cell +0x00 (the object instance) is NULL;
 *        - class = instance->+0x0c;
 *        - DEPTH KEY / SLICING.  The object's footprint rect is class
 *          +0x3c..+0x48 {left, top, right, bottom} in cells relative to the
 *          base coordinate.  Two corner tiles give the depth range:
 *              A = (base.x+left, base.y+bottom)   key1 = ((A.top+A.bottom)>>1) - origin_y
 *              B = (base.x+right, base.y+top)     key2 = ((B.top+B.bottom)>>1) - origin_y
 *          For a SQUARE footprint key1 == key2 and the object is queued as ONE
 *          sprite with key1.  Otherwise the object straddles several depth
 *          bands and is queued SEVERAL times, each under its own clip
 *          rectangle -- vertical strips of the same sprite with increasing
 *          keys, so a long building sorts correctly against blokes and other
 *          objects standing beside it.  The strip geometry uses the other two
 *          corners:
 *              C = (base.x+left,  base.y+top)     d1 = ((C.left+C.right)>>1) - A.left
 *              D = (base.x+right, base.y+bottom)  d2 = ((D.left+D.right)>>1) - A.left
 *              span = 2*min(d1,d2)   step = (d2 < d1) ? -(span/4) : span/4
 *          strip 0 : {view.left, view.top, A.left + 3*span/4, view.bottom} key1
 *          strip 1 : {A.left+3*span/4, .., A.left + 5*span/4, ..}  key1+step
 *          strip n : previous strip shifted right by span/2,       key1+n*step
 *                    while (strip.right + 3*span/4 < B.right)
 *          last    : {previous.right, view.top, view.right, view.bottom} key2
 *          The keys go to g_sort_keys (0x801b40), the clips to g_sort_clips
 *          (0x800400), the count to g_sort_count (0x801b24) -- reset per
 *          object.
 *        - WHAT to draw.  If the class has flag 0x400 it supplies its own
 *          draw callback at +0xa0: desc = cb(class->+0xc4, base) and the
 *          object is skipped when either the callback or its result is NULL.
 *          Otherwise a local SpriteDesc is filled from the class:
 *          {sprite = +0x64, dx = +0x14, dy = +0x18, mode = 0}.
 *        - WHERE.  The base cell's own tile rectangle T = GetTileBounds(base);
 *          the sprite lands at (T.left + desc->dx/2, T.top + desc->dy/2).
 *        - PER-CELL COLOUR / STATE (Cell +0x0c map flags):
 *            0x0020 set = UNDER CONSTRUCTION.  Context token kind 0x104.
 *                 If the class has a build animation (+0x6c) that sprite is
 *                 substituted, the offset is nudged by the signed bytes
 *                 class+0x32/+0x33 and SetOverrideFrame(GetBuildAnimFrame())
 *                 picks the frame; otherwise the normal sprite is drawn with
 *                 recolour mode 0xff00.
 *            0x0020 clear = NORMAL.  Context token kind 0x103, mode from the
 *                 SpriteDesc, then:
 *                   flags & 0x0004            -> leave the mode alone
 *                   flags & 0x0200 && blink   -> mode 0xff0000  (disconnected)
 *                   g_power_available &&
 *                   flags & 0x0100 && !blink  -> mode 0xffff    (no power)
 *          The context token is {kind, instance, base} and is what
 *          PrintSprite hands to the hit test, so the mouse lands on objects.
 *        - The sprite is queued with SortClippedSprite once per strip, or
 *          with SortSprite (key1) when there are no strips, then
 *          ClearOverrideFrame().
 *
 *   5. PEOPLE, WORKERS, FLUSH, OVERLAYS.  RenderPeople (0x483130) and
 *      RenderWorkers (0x49a080) push their own depth-keyed entries, then
 *      DrawAndClearPrintList (0x4859d0) paints the whole sorted list.  After
 *      the flush: the build-progress effects for every collected cell that is
 *      still under construction (DoBuildEffects), then -- if the edit cursor
 *      is live -- the cursor and its deferred sprite list are drawn straight
 *      onto the LOCKED surface (PushRenderingStatusAndLockVideoSurface /
 *      0x461220 / 0x461020 / PopRenderingStatus), and finally the worker
 *      interface GFX.  The clip rectangle is restored on the way out.
 *
 *   The edit-cursor gate: g_show_cursor (0x667d40) is 1 when g_edit_state
 *   (0x8119b0) is 2, or is 1 with g_edit_object (0x8119b8) equal to the
 *   environment class (0x7fd624); the cursor's flash phase g_cursor_blink
 *   (0x667d48) is 255 for half of every 256 ms of GetTickCount and 0 for the
 *   other half.
 *
 * ---------------------------------------------------------------------------
 * HOST / BROWSER-RENDERER CONTRACT for RenderView
 * ---------------------------------------------------------------------------
 * Everything the function reads or writes, so a re-implementation can be
 * driven from the same state without disassembling it again.
 *
 * READS (must be set up before the call):
 *   g_map            0x4bcbf4  view origin +0x20/+0x22, view size +0x10/+0x12,
 *                              map extent +0x14/+0x16 (all u16)
 *   g_map_rows       0x801400  Cell*[height]; Cell stride 0x14
 *   g_scroll_x/y     0x667cb4 / 0x667cb8   24.8 fixed point
 *   g_default_tile   0x667ca4  index into g_tile_sprites
 *   g_tile_sprites   0x805f60  Sprite*[]; sprite height at +0x16 (u16) is
 *                              the ONLY field this function reads, and it is
 *                              the unit of the whole isometric projection
 *   g_odf_head       0x669240  ObjDef chain, link at +0x00
 *   g_env_class      0x7fd624, g_edit_state 0x8119b0, g_edit_object 0x8119b8
 *   g_power_available 0x83298c (gates the "no power" recolour)
 *   g_bg_dirty_x/y   0x667cd0 / 0x667cd4  handed to PrintBackground verbatim
 *
 * WRITES / SIDE EFFECTS:
 *   g_bg_full_update 0x4b9220 forced to 1 before the ground pass
 *   g_view_dirty     0x667cc4 cleared
 *   g_show_cursor    0x667d40, g_cursor_blink 0x667d48 (see the gate above)
 *   Cell +0x0c bit 0x0400 is a PER-FRAME transient: pass 1 sets it on every
 *      owner cell it queues and pass 2 clears it again on every cell in the
 *      array, in the same order.  It is never persisted; a host that stops
 *      the walk early MUST clear it or the next frame drops those objects.
 *   g_sort_count 0x801b24, g_sort_keys 0x801b40 (int[], stride 4) and
 *      g_sort_clips 0x800400 (ClipRect[], stride 0x10) are scratch for ONE
 *      object: they are reset to 0 at the top of every queue iteration and
 *      consumed by the SortClippedSprite calls of that same iteration.
 *   The print list itself (printlist.c) is filled and then drained by
 *      DrawAndClearPrintList; nothing survives the call.
 *
 * INVARIANTS worth knowing:
 *   - The visible-object array is a 3000-entry LOCAL.  There is NO bound
 *     check on it: a view that can touch more than 3000 distinct objects
 *     smashes the frame.  With the shipped 640x480 view and the shipped tile
 *     size that cannot happen, but a host that widens the view must cap it.
 *   - visible[count] is written with 0 after the gather (a terminator that
 *     nothing reads -- the two later walks are both counted).  Reproduced.
 *   - The gather walk visits TWO cells per column step and steps the column
 *     by `cell + 1` when it can, so the row must be contiguous in memory;
 *     it falls back to the bounds-checked lookup at a row edge.
 *   - The owner lookup through the packed base coordinate is NOT bounds
 *     checked in the original (see stage 2 above); a host should keep the
 *     same behaviour for identical output but must not fault on it.
 * ------------------------------------------------------------------------- */

/* ---- local types -------------------------------------------------------- */

typedef struct Pos {
    int x;                      /* +0x00 */
    int y;                      /* +0x04 */
} Pos;

/* A packed 2-byte map coordinate passed BY VALUE (objmap2.c's BPos). */
typedef struct BPos {
    unsigned char x;            /* +0x00 */
    unsigned char y;            /* +0x01 */
} BPos;

/* The bare 16-byte clip rectangle SetClipping / SortClippedSprite take. */
typedef struct ClipRect {
    int left;                   /* +0x00 */
    int top;                    /* +0x04 */
    int right;                  /* +0x08 */
    int bottom;                 /* +0x0c */
} ClipRect;

/* pathbuild.c's TileBounds (GetTileBounds), inclusive pixels. */
typedef struct TileBounds {
    int left;                   /* +0x00 */
    int top;                    /* +0x04 */
    int right;                  /* +0x08 */
    int bottom;                 /* +0x0c */
} TileBounds;

/* printlist.c's 12-byte render-context token; the third field is the packed
 * map coordinate of the object the token names. */
typedef struct BlitCtx {
    int   kind;                 /* +0x00 0x103 normal, 0x104 under construction */
    void* obj;                  /* +0x04 */
    BPos  base;                 /* +0x08 */
    short pad0a;                /* +0x0a */
} BlitCtx;

/* printlist.c's SpriteDesc -- what a class draw callback returns. */
typedef struct SpriteDesc {
    void* sprite;               /* +0x00 */
    int   dx;                   /* +0x04 */
    int   dy;                   /* +0x08 */
    int   f0c;                  /* +0x0c */
    int   mode;                 /* +0x10 */
    int   layer_mask;           /* +0x14 */
} SpriteDesc;

/* A loaded sprite record; only the height is read here (tile geometry). */
typedef struct Sprite {
    char  pad00[0x14];          /* +0x00 */
    short w;                    /* +0x14 */
    short h;                    /* +0x16 */
} Sprite;

/* The map header (0x004bcbf4). */
typedef struct MapHdr {
    char           pad00[0x10]; /* +0x00 */
    unsigned short view_w;      /* +0x10 view rectangle size, pixels */
    unsigned short view_h;      /* +0x12 */
    unsigned short width;       /* +0x14 map size, cells */
    unsigned short height;      /* +0x16 */
    char           pad18[0x20 - 0x18];
    unsigned short origin_x;    /* +0x20 view rectangle origin, pixels */
    unsigned short origin_y;    /* +0x22 */
} MapHdr;

/* legoland.h's Cell (20 bytes). */
typedef struct Cell {
    void*          obj;         /* +0x00 object instance */
    BPos           base;        /* +0x04 owner cell (footprint origin) */
    unsigned char  nx;          /* +0x06 render-chain next, packed */
    unsigned char  ny;          /* +0x07 */
    unsigned short tile;        /* +0x08 */
    unsigned short ground;      /* +0x0a */
    unsigned short flags;       /* +0x0c map flags (0x400 = queued this frame) */
    unsigned short uflags;      /* +0x0e */
    unsigned char  rf;          /* +0x10 */
    unsigned char  life;        /* +0x11 */
    unsigned char  pad12[2];    /* +0x12 */
} Cell;

typedef struct ObjDef ObjDef;

/* The 0xd0-byte ODF class record, as the renderer reads it. */
struct ObjDef {
    ObjDef*      next;          /* +0x00 g_odf_head chain */
    char         pad04[0x14 - 0x04];
    int          dx;            /* +0x14 default sprite offset */
    int          dy;            /* +0x18 */
    unsigned int flags;         /* +0x1c 0x20 = has pre-render hook,
                                 *       0x400 = supplies its own draw callback */
    char         pad20[0x32 - 0x20];
    signed char  anim_dx;       /* +0x32 build-animation nudge */
    signed char  anim_dy;       /* +0x33 */
    char         pad34[0x3c - 0x34];
    int          rleft;         /* +0x3c footprint rect, cells */
    int          rtop;          /* +0x40 */
    int          rright;        /* +0x44 */
    int          rbottom;       /* +0x48 */
    char         pad4c[0x64 - 0x4c];
    void*        sprite;        /* +0x64 */
    char         pad68[0x6c - 0x68];
    void*        build_sprite;  /* +0x6c */
    char         pad70[0xa0 - 0x70];
    SpriteDesc*  (*draw)(void* ctx, BPos base);   /* +0xa0 */
    char         pada4[0xa8 - 0xa4];
    void         (*prerender)(void* ctx);         /* +0xa8 */
    char         padac[0xc4 - 0xac];
    void*        ctx;           /* +0xc4 */
};

/* An object instance: the class hangs off +0x0c. */
typedef struct Obj {
    char    pad00[0x0c];        /* +0x00 */
    ObjDef* def;                /* +0x0c */
} Obj;

/* ---- globals ------------------------------------------------------------ */

extern MapHdr*  g_map;              /* 0x004bcbf4 */
extern Cell**   g_map_rows;         /* 0x00801400 */
extern Sprite*  g_tile_sprites[];   /* 0x00805f60 */
extern int      g_default_tile;     /* 0x00667ca4 */
extern int      g_scroll_x;         /* 0x00667cb4  24.8 */
extern int      g_scroll_y;         /* 0x00667cb8 */
extern ObjDef*  g_odf_head;         /* 0x00669240 */

extern int      g_bg_full_update;   /* 0x004b9220 */
extern int      g_view_dirty;       /* 0x00667cc4 */
extern int      g_bg_dirty_x;       /* 0x00667cd0 */
extern int      g_bg_dirty_y;       /* 0x00667cd4 */
extern int      g_show_cursor;      /* 0x00667d40 */
extern int      g_cursor_blink;     /* 0x00667d48 */

extern int      g_edit_state;       /* 0x008119b0 */
extern void*    g_edit_object;      /* 0x008119b8 */
extern void*    g_env_class;        /* 0x007fd624 */
extern int      g_power_available;  /* 0x0083298c */

extern int      g_sort_count;       /* 0x00801b24 */
extern int      g_sort_keys[];      /* 0x00801b40 */
extern ClipRect g_sort_clips[];     /* 0x00800400 */

/* ---- externals ---------------------------------------------------------- */

__declspec(dllimport) unsigned long __stdcall GetTickCount(void); /* [0x4ab1f8] */

extern void GetClipping(ClipRect* out);                    /* 0x0048a630 */
extern void SetClipping(ClipRect* r);                      /* 0x0048a5c0 */
extern void GetTileBounds(Pos* tile, TileBounds* out);     /* 0x0045acc0 */
extern void RenderGroundLayer(void);                       /* 0x00460e00 */
extern void PrintBackground(int x, int y);                 /* 0x00464360 (empty stub) */
extern void RenderViewCellProbe(Pos* tile);                /* 0x0045b170 (bare ret) */
extern int  GetBuildAnimFrame(ObjDef* def, BPos base);     /* 0x00450cf0 */
extern void SetOverrideFrame(int frame);                   /* 0x00464420 */
extern void ClearOverrideFrame(void);                      /* 0x00464440 */
extern int  GetBlink(void);                                /* 0x00499480 */
extern void SortSprite(void* sprite, int x, int y, int key, int mode,
                       BlitCtx* ctx);                      /* 0x00485d70 */
extern void SortClippedSprite(void* sprite, int x, int y, int key,
                              ClipRect* clip, int mode, BlitCtx* ctx); /* 0x00485e40 */
extern void RenderPeople(void);                            /* 0x00483130 */
extern void RenderWorkers(void);                           /* 0x0049a080 */
extern void DrawAndClearPrintList(void);                   /* 0x004859d0 */
extern void DoBuildEffects(ObjDef* def, BPos base);        /* 0x00450d90 */
extern void PushRenderingStatusAndLockVideoSurface(void);  /* 0x00463fc0 */
extern void PopRenderingStatus(void);                      /* 0x004641f0 */
extern void DrawEditCursor(void);                          /* 0x00461220 */
extern void FlushCursorSpriteList(void);                   /* 0x00461020 */
extern void RenderWorkerInterfaceGFX(void);                /* 0x0049b0b0 */

/* Halve a sprite offset toward zero. The original spells this as an explicit
 * sign test (test/jge/neg/sar/neg), not the cdq form VC6 emits for `/ 2`. */
static __inline int HalfOffset(int v)
{
    if (v < 0)
        return -((-v) >> 1);
    return v >> 1;
}

/* Bounds-checked cell fetch, inlined at every use in the original. */
static __inline Cell* CellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}


/* Queue one object's sprite: once per depth strip under its own clip, or as a
 * single entry when the object did not need splitting. */
static __inline void EmitObjectSprite(SpriteDesc* desc, Pos at, int key,
                                      int mode, BlitCtx* ctx)
{
    if (g_sort_count != 0) {
        unsigned int i = 0;
        while (i < (unsigned int)g_sort_count) {
            SortClippedSprite(desc->sprite, at.x, at.y, g_sort_keys[i],
                              &g_sort_clips[i], mode, ctx);
            i++;
        }
    } else {
        SortSprite(desc->sprite, at.x, at.y, key, mode, ctx);
    }
}

/* RenderView -- 903/903 instructions, 2893B vs 2880B, no ESCAPES,
 * audit.py mismatch = 381.
 *
 * ===========================================================================
 * ROUND w10: NOTHING COMMITTED (still 381).  One RECONSTRUCTION ERROR found
 * and left uncommitted with its cost stated, and one new, sharper statement
 * of what the frame problem actually is -- it is NOT an ordering problem, it
 * is a REFERENCE-COUNT problem.
 * ===========================================================================
 *
 * *** RECONSTRUCTION ERROR: `g_sort_count = 0;` BELONGS INSIDE
 * `if (cell->obj != 0)`, NOT BEFORE IT. ***  The original stores the zero
 * exactly ONCE, at index 391, and 391 is PAST the `je` at 388:
 *      383 mov eax, dword ptr [esi]            cell->obj
 *      384 and word ptr [esi+0xc], 0xfbff      cell->flags &= ~0x400
 *      385 cmp eax, ecx                        (ecx = 0)
 *      386 mov dword ptr [esp+0x78], ecx       the last two sd zero stores
 *      387 mov dword ptr [esp+0x84], edx       (edx = 0)
 *      388 je  <skip>
 *      389 mov edi, dword ptr [eax+0xc]        def
 *      390 mov cx,  word ptr [esi+4]           cell->base
 *      391 mov dword ptr [0x801b24], edx       *** g_sort_count = 0 ***
 *      392 mov dl,  byte ptr [esi+4]           cell->base.x
 * edx is set by `xor edx,edx` at 376 and is not redefined in between, so 391
 * really is the constant store; the whole rest of the function only ever
 * READS 0x801b24 or writes back an incremented value (509/517, 538/546,
 * 575/584, 602/610, 726, 748, 812, 834).  VC6 does not sink a partially dead
 * store (proved twice on `RenderFullMap`'s sd fill), and our body duly emits
 * it at 377 with the sd zero fill, OUTSIDE the branch -- so the committed
 * source is wrong.  A corroborating signature: the original materialises TWO
 * zero registers (`xor edx,edx` 376 and `xor ecx,ecx` 378) for six identical
 * zero stores, precisely because one of them has to stay live across the
 * branch to feed 391; ours materialises one.
 * COST, STATED HONESTLY, WHICH IS WHY IT IS NOT COMMITTED: the best of eight
 * placements (`base = cell->base; g_sort_count = 0; tile.x = ...`) scores
 * strict 386 (from 381), register-blind 207 (from 206), offset-blind 223
 * (from 228), both-blind 57 (unchanged), mnemonic 47 (from 45), region total
 * 101 (from 99), 2894B (from 2893, original 2880).  The residual is local and
 * visible: with the store moved, our `je` lands at 387 instead of 388 and the
 * `base` word store lands at 390 where the original delays it to 396 (after
 * the byte load and the `add`).  Eight positions for the store and four for
 * `base` were swept, plus both orders of the sd fill against
 * `cell->flags &= ~0x400` (inert): none is a net win.  THIS FIX AND WHATEVER
 * DELAYS THE `base` STORE GO IN TOGETHER.  The variant is saved as
 * scratchpad/w10rv/p2_sortcount.c (split prologue intact: esi@5 edi@7
 * ebx@58 ebp@60).  The runner that produced every number in this round is
 * scratchpad/w10rv/t.py -- `t.run(name, [(old_text, new_text), ...])` prints
 * strict, all four blind measures, the region total, the big-displacement
 * total and the sd/join/track layout signature on one line, in ~2 s.
 *
 * *** THE LOW FRAME CARRIES FOUR MORE SURVIVING REFERENCES THAN THE
 * ORIGINAL'S, AND FRAME WEIGHT IS THE SURVIVING-IR REFERENCE COUNT. ***
 * Measured with scratchpad/laneG/fm.py (which works on this function; it
 * does NOT work on RenderFullMap -- use w9renderview/slots.py there).  From
 * +0x034 up, ours and the original agree slot for slot.  The low thirteen,
 * as (reference count) multisets:
 *      ORIGINAL  19 18 12 9 9 7 7 7 7 6 6 6 4   = 117 references
 *      OURS      19 15 12 11 9 9 7 7 7 7 7 6 5  = 121
 *      s2_geoJ   19 15 12 11 9 9 7 7 7 7 6 6 5  = 120
 * Since routing uses through an alias pointer is byte-identical and dead
 * reads are eliminated before the count (the `Draw3DPersonModel` proof), the
 * slot ORDER cannot be permuted into place while those four extra references
 * exist.  Find and remove them first; the permutation should then fall out.
 *
 * *** SLOT IDENTITIES, so the next round does not have to re-derive them. ***
 * The original's +0x000 is COLOUR-SHARED between `th` and `mode`, and it is
 * the 18-reference slot:
 *      th    77 store from edi, 88 reload into ebx, 173, 340
 *      mode  491, 502, 524, 553, 568, 663, 725 (=0xff00), 734, 756, 767,
 *            801 (=0xff0000), 811 (=0xffff), 820, 842
 * +0x004 (7 refs) is `rx`: 90 store, 97 reload, 184, 508, 510, 539, 577.
 * OURS SPLITS THEM: our +0x000 (15 refs) is `mode` ALONE plus seven refs the
 * original does not have in that slot (110, 185, 429, 529, 550, 559, 706),
 * and `th` is in our +0x00c (11 refs, first@72).  So the thing to look for is
 * whatever keeps our `th` web alive past the point where the original's dies
 * and lets `mode` take the same home.
 *
 * Also re-confirmed this round: `s1_ylate.c` and `s2_geoJ.c` (the two honest
 * uncommitted bodies in scratchpad/w9renderview/) BOTH KEEP the split
 * prologue -- push esi@5 edi@7 ebx@58 ebp@60, exactly the original's -- and
 * are better than the committed body on every structural measure (region
 * total 86/89 vs 99, register+offset-blind 49/51 vs 57, register-blind
 * 198/199 vs 206, offset-blind 187/225 vs 228, mnemonic 38/39 vs 45).  Their
 * strict count is 814/817 only because the low frame renumbers.  THEY ARE
 * THE RIGHT BODIES; they become committable the moment the four extra
 * low-frame references above are found.
 *
 * ===========================================================================
 * ROUND w8: NOTHING CHANGED HERE.  The body is unchanged from round w7 and
 * still mismatches 381 with the split prologue intact.  What this round adds
 * is (a) the exact mechanism of the split-prologue bit, which round w7 also
 * got wrong, and (b) a further eleven ruled-out families.  Read this before
 * touching the geometry block again.
 * ===========================================================================
 *
 * *** ROUND w7'S CORRECTION IS ITSELF WRONG: THE REGISTER ASSIGNMENT DOES
 * CHANGE. ***  w7 says "in every split-breaking variant the register
 * ASSIGNMENT is identical to the original and only the push PLACEMENT
 * changes".  Disassemble one and it is not so.  Committed / original:
 *      64 mov ebp,[g_scroll_x]   sx in ebp
 *      70 movsx edi,ax           th in edi
 *      73 movsx ebx,cx           tw in ebx      75 idiv ebx
 * A split-breaking variant (qx moved ahead of qy):
 *      64 mov ebx,[g_scroll_x]   sx in EBX
 *      70 movsx edi,ax           th in edi
 *      73 movsx ebp,dx           tw in EBP      75 idiv ebp
 * -- sx and tw SWAP between ebx and ebp.  esi/edi are unchanged in both (the
 * gate's 1 and 0, and then reused for g_scroll_y and th).
 *
 * *** AND IT IS NOT "SINKING" EITHER.  VC6 PUTS EXACTLY TWO PUSHES IN THE
 * ENTRY HOLES; THE BIT IS WHICH PAIR GETS THEM. ***  The original pushes
 * esi at 5 and edi at 7 -- BEFORE their own definitions, which are `xor
 * edi,edi` at 15 and `mov esi,1` at 21 -- interleaved into the four `view`
 * fills, and pushes ebx at 58 and ebp at 60, interleaved into SetClipping's
 * argument push.  A broken variant pushes ebx at 5 and ebp at 7 and esi at
 * 15, edi at 21 -- i.e. esi/edi now land exactly where their definitions are
 * and ebx/ebp take the two entry holes, even though ebx/ebp are not defined
 * until 64/73.  Both bodies place two pushes early and two late; the order of
 * the four pushes is (esi,edi,ebx,ebp) in the original and (ebx,ebp,esi,edi)
 * in every broken variant.  So the diagnostic remains "print the four push
 * indices", but the thing being decided is the ORDER of the callee-saved
 * pushes -- i.e. which pair of values is allocated first -- not whether a
 * push sinks.
 *
 * *** THE GEOMETRY ORDER THE DISASSEMBLY ACTUALLY IMPLIES ***, for whoever
 * gets the bit: sx (64-65), th (70), tw (71-73), qx (72-75), hw (76,78),
 * hh (79,81 -- `inc edi / sar edi,1`, destroying th's register after th is
 * spilled at 77), sy (82-83, `sar esi,8` then `sub esi,edi`), rx (85-87),
 * th reloaded into ebx (88), qy (89-93), ry (94-98), tile.x (99-100),
 * tile.y (101); then ylimit (162-167) and xlimit (168-172) after the switch,
 * ylimit FIRST.  Note that qx and rx are NOT adjacent in the original -- hw,
 * hh and sy are computed between them -- and that `hh` comes out as an
 * in-place `inc/sar` on th's register only because th is the SPILLED divisor;
 * with th in a register (our case) it is `lea r,[th+1] / sar r,1`.  That is a
 * consequence of the divisor order, not an independent lever.
 *
 * RULED OUT THIS ROUND
 *   - 23 geometry orders with `qx` before `qy`, with both limits late, with
 *     xlimit late and ylimit in the block, with xlimit in the block at six
 *     different positions, and with `sx` hoisted to first: EVERY one of them
 *     reports pushes=ebx@5 ebp@7 esi@15 edi@21.  Only orders with `qy`
 *     before `qx` AND xlimit still in the block keep esi@5 edi@7 ebx@58
 *     ebp@60.  (Generator: scratchpad/w8renderview/rvgeo.py, which takes a
 *     permutation string with a `|` separating the in-block statements from
 *     the ones emitted immediately before the row loop.)
 *   - the documented "post-guard inner scope" route to a deferred prologue:
 *     wrapping the whole geometry block plus the row loop in a `{ }` after
 *     SetClipping, with sh/sw/sx/sy/th/tw/hw/hh/qx/qy/rx/ry/quad/px/py/
 *     xlimit/ylimit/rowy declared inside it, is BYTE-IDENTICAL both on the
 *     committed order and on a broken one (scratchpad/w8renderview/rvsc.py).
 *   - eleven respellings applied on top of a qx-early order, all of which
 *     leave pushes=ebx@5 ebp@7: `hh` from `sh`; `th = (int)sh`; `tw =
 *     (int)sw`; `hw`/`hh` split into two statements; `hh = th; hh = (hh+1)
 *     >> 1;` (aimed at the original's in-place `inc/sar`); `sy` split into
 *     two statements; dividing by `sh`; no named `sx` (the shift spelled at
 *     both uses); no named `sy` likewise.  (scratchpad/w8renderview/rvt.py.)
 *   - `at.x = hx + tb.left; at.y = hy + tb.top;` (the original's add
 *     destination is hx's register, 721 `add esi,edx`) at each of the three
 *     sites and at all of them: BYTE-IDENTICAL.  VC6 canonicalises `a + b`,
 *     so the add-destination lever recorded in DECOMP.md does not reach this
 *     site from the operand order alone.
 *   - all 15 interleavings of `mode = desc->mode;` into the ctx_build /
 *     ctx_normal field fills (the original emits it BETWEEN the `.obj` and
 *     `.kind` stores, 663 between 660-662 and 665).  Moving it out of first
 *     position in the ctx_normal arm takes register-blind 206 -> 149, a
 *     56-slot jump, but the LCS region total stays at 99, mnemonic-only stays
 *     at 45, offset-blind goes 228 -> 229/230 and strict costs 2-6.  With the
 *     ranking measure flat and strict worse it is read as an alignment
 *     artefact of the register-blind LCS, and NOT applied.  If a later round
 *     moves the geometry block, re-run it -- it is the only thing in the
 *     function that moves a blind measure by that much.
 *
 * STILL TRUE, AND STILL THE INSTRUCTION FOR THE NEXT ROUND: the best
 * structural state ever measured is scratchpad/w7renderview/
 * geo_abcjdefghlikmno.c (region total 88, offset-blind 187, split prologue
 * kept, strict 817).  ADOPT IT THE MOMENT THE +10 REGION AT ours[98:108] IS
 * FIXED, and not before.
 *
 * ===========================================================================
 * ROUND w7: THE PROLOGUE-SPLIT BLOCKER, MEASURED PROPERLY.  Nothing below was
 * changed -- this round found no variant it believed in that did not cost
 * more strict than it bought structurally -- but the blocker is now pinned
 * down to one statement, and the previous round's stated CAUSE IS WRONG.
 * ===========================================================================
 *
 * *** THE OLD HYPOTHESIS IS FALSIFIED. ***  The note below says "the geometry
 * block's demand for esi/edi forces the g_edit_state gate's constants 0 and 1
 * into ebx/ebp".  Measured, that does not happen.  In EVERY split-breaking
 * variant the register ASSIGNMENT is unchanged from the committed file and
 * from the original: edi still holds the constant 0, esi the constant 1, ebp
 * still gets `sx` at index 64 and ebx `tw`/`th` at 70-73.  What changes is
 * only WHERE THE PUSHES GO.  The committed file and the original emit
 *     push esi@5  push edi@7   ...gate...   push ebx@58  push ebp@60
 * and a broken variant emits
 *     push ebx@5  push ebp@7   push esi@15  push edi@21
 * -- the same four registers holding the same four values, with the ebx/ebp
 * pair no longer sunk into the post-gate join block and the whole pair moved
 * AHEAD of esi/edi.  Frame size (0x2f90), frame slot count (40) and the code
 * from the SetClipping call onward are identical between a split and a broken
 * variant; the two bodies differ only by the two sunk pushes.  So this is a
 * push-SINKING decision, not a register-preference one, and it is decided
 * inside the geometry block.
 *
 * *** THE TRIGGER IS ONE STATEMENT: `qx = sx / tw;` MOVING AHEAD OF
 * `qy = sy / th;`. ***  Measured over a ladder of geometry orders (tooling:
 * scratchpad/w7renderview/geo.py + split.py, which prints the four push
 * indices next to the structural measures):
 *     ylimit alone after the switch                    SPLIT KEPT
 *     ylimit after the switch + `sx` hoisted to 4th    SPLIT KEPT
 *     ...  + qx placed between qy and ry               SPLIT KEPT (best)
 *     ...  + qx placed between ry and hw               BROKEN
 *     ...  + qx before qy (any position)               BROKEN
 *     xlimit after the switch (with or without ylimit) BROKEN
 *     qx/rx first, limits left in place                BROKEN
 * and on the committed geometry (both limits in the block) `sx` hoisted is
 * still fine but every qx move before qy still breaks it.  13 respellings of
 * the PROLOGUE -- all 8 statement orders of the four `view` fills / count /
 * cls / g_bg_full_update / g_view_dirty that were tried, plus three gate
 * shapes (leading `g_show_cursor = 0`, the fully inverted test, blink before
 * show) -- leave the push placement bit-for-bit unchanged.  The prologue text
 * is NOT the lever; do not spend another round on it.
 *
 * *** WHY THE TWO HORNS CANNOT BOTH BE SATISFIED YET. ***  The +10-instruction
 * region at ours[98:108] exists because the original's FIRST divisor is `tw`
 * (index 73 `movsx ebx,cx`, 75 `idiv ebx`, 87 `idiv ebx`, then 88 `mov ebx,
 * [esp+0x10]` reloading `th` for 92/96), so `tw` wins ebx; we divide by `th`
 * first, `th` wins ebx and `tw` is spilled to +0x20 and used as a MEMORY
 * divisor.  The only way to make `tw` the first divisor is to move qx/rx
 * ahead of qy/ry -- which is exactly the statement move that breaks the
 * split.  Two errors, one lever each, and the lever positions are mutually
 * exclusive.
 *
 * *** WHY 381 IS PROPPED UP BY A COMPENSATING ERROR (read before chasing the
 * strict count here). ***  The committed file has +10 instructions at 98..108
 * and -11 at 164..176 (the limits it computes early and the original computes
 * late).  They cancel: only indices 108..173 are shifted, ~65 slots, and the
 * rest of the body realigns.  Fix the limits ALONE and the +10 is no longer
 * compensated, so 800 slots shift and the strict count goes 381 -> 817 while
 * every structural measure improves.  Best structural state ever measured on
 * this function, geometry order
 *     sh, th, sw, sx, hh, sy, tw, xlimit, qy, qx, ry, hw, rx, tile.x, tile.y
 * with `ylimit = g_map->view_h + view.bottom;` moved to immediately before
 * the row loop (the original computes it at 162-167 and xlimit at 168-172,
 * ylimit FIRST -- xlimit first measures 5 worse):
 *     LCS region total  99 -> 88     both-blind deficit  57 -> 49
 *     register-blind   206 -> 198    offset-blind       228 -> 187
 *     mnemonic-only     45 -> 38     bytes    2893 -> 2891 (orig 2880)
 *     split prologue KEPT, first diverging index 64
 *     strict            381 -> 817
 * It is NOT applied, because a 2.1x strict regression for an 11% structural
 * gain is not a trade this lane is willing to make on someone else's
 * converged file.  ADOPT IT THE MOMENT THE +10 REGION IS FIXED: with both
 * errors gone the streams realign and the strict count should fall well
 * below 381.  The variant is on disk as
 * scratchpad/w7renderview/geo_abcjdefghlikmno.c (and geo.py regenerates any
 * order from a 15-letter permutation).
 *
 * CONFIRMED FROM THE DISASSEMBLY THIS ROUND (both were assertions before):
 *   - the original really does compute ylimit then xlimit AFTER the switch:
 *     162 `mov edx,[g_map]` / 163 view.bottom / 165 `xor edi,edi` / 166
 *     `mov di,word [edx+0x12]` / 167 `add edi,ecx` / 170 store = ylimit;
 *     168 `mov ecx,[esp+0x20]` (tw) / 169 `add ecx,ecx` / 171 `add ecx,ebx`
 *     (view.right) / 172 store = xlimit; 173-180 py, compared to edi at 179;
 *   - `sx` is computed EARLY, before `th` is even sign-extended: 64
 *     `mov ebp,[g_scroll_x]` / 65 `sar ebp,8`, with `movsx edi,ax` (th) only
 *     at 70.  Hoisting `sx` to fourth in the geometry block is worth 38 on
 *     the offset-blind measure by itself (228 -> 190) and keeps the split.
 * ------------------------------------------------------------------------- */
/* (the previous round's note follows, unchanged except where round w7 above
 * corrects it)
 * RenderView -- that round took the body 478 -> 381; it had been 633 the
 * round before.  Every step of 478 -> 381 came from four source-shape
 * changes that are argued from the disassembly rather than found by search.
 * Register/offset-blind LCS is now 846 of 903 (was 791) and mnemonic LCS 858
 * (was 812), so the instruction SEQUENCE is 94% right; what is left is
 * 57 structurally-displaced slots plus the frame-slot permutation.
 *
 * ===========================================================================
 * WHAT CHANGED THIS ROUND (all four are structural, not score-chasing)
 * ===========================================================================
 * 1. THE HALFOFFSET SITES ARE FOUR-TEMP, NOT INLINE EXPRESSIONS.  All three
 *    sprite-offset sites read as
 *        gx = <dx expr>;  gy = <dy expr>;
 *        hx = HalfOffset(gx);  hy = HalfOffset(gy);
 *        at.x = tb.left + hx;  at.y = tb.top + hy;
 *    and NOT `at.x = tb.left + HalfOffset(<dx expr>);`.  The proof is in the
 *    original at 668..673: it loads BOTH desc->dx and desc->dy into registers
 *    and computes BOTH sums before touching either sign test, so the flags of
 *    the first `add` are dead by the time the first branch is reached and the
 *    original must spell `test eax,eax` / `jge` where the fused form gives a
 *    bare `jns`.  Writing both sums first buys those two instructions back;
 *    halving into temps before the tb.left/tb.top adds buys the original's
 *    "half, half, add, add, store, store" order.  Indices 668..702 (the
 *    build-animation arm) are now instruction-for-instruction identical to
 *    the original, and the emit window went from 34 structurally-wrong slots
 *    to 6.  The old note's "two-temp spellings cost 31 elsewhere" was
 *    measured on the two-temp (raw dx/dy) form, which is NOT the right shape:
 *    both the sums AND the halves need temps.
 *
 * 2. THE SWITCH CASE ORDER IS 1, 2, 3, 4 -- the natural one.  The previous
 *    round used 1, 3, 2, 4 because it scored 3 better on the strict count.
 *    It is wrong: the original's arms at 115..161 are, in address order,
 *    case 1 (`hw - 2*ry`), case 2 (`hw + 2*ry`), case 3 (`hw + 2*(ry-th)`),
 *    case 4 (`hw + 2*(th-ry)`), and switch arms are emitted in SOURCE order.
 *    Restoring 1,2,3,4 removes an 11-instruction displacement (the old file
 *    had orig[128:139] missing and ours[147:158] extra) and drops the
 *    geometry window from 41 structurally-wrong slots to 30.  Case 3 ends in
 *    `jmp` into case 4's last two instructions (`mov [rx],edx` / `sub esi,edi`)
 *    -- the identical-suffix merge, which only lines up when 3 and 4 are
 *    adjacent in source.
 *
 * 3. `mode = 0xff00;` IS THE FIRST STATEMENT OF THE `else` ARM, not the last.
 *    THIS IS THE BIG ONE: it alone took the file from 502 to 386.  Moving a
 *    dead-simple constant assignment to the TOP of the block changes which
 *    registers at.x / at.y are computed into: with it last, VC6 lands them in
 *    edx/eax (caller-saved), the values do not survive the
 *    GetBuildAnimFrame / SetOverrideFrame calls in the sibling arm, and the
 *    single-sprite emit has to RELOAD them from the frame -- three extra
 *    instructions that shifted the whole tail (indices 800..903) by three and
 *    cost 73 strict mismatches on their own.  With it first they land in
 *    esi/ebx exactly as the original, both arms end with the same
 *    `mov ecx,[ebp]` / `push ecx` / `call` / `add esp,0x18` suffix, VC6's
 *    cross-jump merges all four, and the tail realigns to zero structural
 *    error.  GENERAL RULE, worth trying anywhere a block ends with a
 *    constant store: the PLACEMENT of a constant assignment inside a block
 *    decides the callee-saved-vs-caller-saved split for the values computed
 *    beside it, because VC6 allocates in creation order and the constant
 *    takes a register the moment it is created.
 *
 * 4. `mode = desc->mode;` is likewise the FIRST statement of both BlitCtx
 *    arms (mode, obj, kind, base), matching the original's 765..774 and
 *    658..666.  Worth 3.
 *
 * ===========================================================================
 * THE FRAME.  Size is right (0x2f90); BOTH SIDES USE EXACTLY THE SAME 40
 * SLOTS; only the lowest 13 are permuted, and the permutation is ONE MOVE.
 * ===========================================================================
 * Measured with scratchpad/laneG/fm.py (control-flow-aware esp simulation
 * from the entry, so pushes inside call-argument runs are accounted for;
 * slot = the [esp+N] operand plus the push depth at that instruction).
 * Every slot from +0x034 up agrees in address, reference count and
 * first-reference index, to within a couple of instructions:
 *     +0x034 view(16)  +0x044 cls  +0x048 corner(16)  +0x058 bright
 *     +0x05c key2  +0x060 sd(16)  +0x078/+0x88/+0x94 ctx tokens
 *     +0x0a0 saved(16)  +0x0b0 visible[3000]  (0xb0 + 0x2ee0 = 0x2f90 exactly)
 * The low block:
 *     ORIG  th | rx | tile.x tile.y | tw | count | qx/rowx | xlimit |
 *           ylimit&at.x | at.y | py | rowy | ...
 *     OURS  rx | tile.x tile.y | th | tw | count | ylimit | xlimit |
 *           qx/rowy | py | 2*tw | at.x | at.y
 * i.e. ours is the original's list with `th` moved from the bottom to fourth,
 * plus ONE EXTRA spilled temp: our `tw + tw` is CSE'd between `xlimit` and the
 * column-loop `px` initialiser and gets a home of its own at +0x028, while
 * the original recomputes `add ecx,ecx` at both sites (169 and 185) and never
 * spills it.  Breaking that CSE by hand does not work -- `view.right+tw+tw`,
 * `tw+(tw+view.right)` and `view.left-tw-tw-rx` all compile to the identical
 * object; VC6 canonicalises the sum before CSE.
 * The two frame differences cost 56 of the 381 on their own: index-for-index
 * mismatch is 381 strict, 326 with every [esp+N] collapsed, 310 with every
 * register collapsed, 234 with both, and 190 on mnemonics alone.
 *
 * FIRST DIVERGING INDEX: 67.  Indices 0..66 are byte-identical (the __chkstk
 * prologue, the SPLIT prologue with `push esi`/`push edi` at 5/7 and
 * `push ebx`/`push ebp` sunk to 58/60, the four `view` fills, count/cls, the
 * two global flags, GetClipping / RenderGroundLayer / PrintBackground, the
 * whole g_edit_state cursor gate and SetClipping).
 *      orig  67 mov esi,[g_scroll_y] / 68 add esp,4 / 69 mov ax,[ecx+0x16]
 *      ours  67 add esp,4 / 68 mov ax,[ecx+0x16] / 69 mov ecx,[g_scroll_y]
 * -- the same instructions, one slot apart in the schedule.
 *
 * ===========================================================================
 * WHAT IS LEFT: 57 structurally-displaced slots, 42 of them in ONE knot
 * ===========================================================================
 * Aligned register+offset-blind, the differing regions are
 *   67..91    the tile-geometry preamble: same instructions, VC6 interleaves
 *             our ylimit/xlimit loads into it.
 *   98..108   TEN instructions ours has here and the original does not: our
 *             `sx / tw` and `sx % tw` idivs, done SECOND and with a MEMORY
 *             divisor (`idiv dword [esp+0x20]`).  The original does the
 *             sx/tw pair FIRST with `idiv ebx` (tw in ebx from `movsx ebx,cx`
 *             at 73), spills th at 77 and RELOADS it into ebx at 88 for the
 *             sy/th pair.  So the original's source computes qx, rx before
 *             qy, ry.
 *   164..176  TWELVE instructions the original has here and we do not: it
 *             computes ylimit and xlimit AFTER the quadrant switch, right
 *             before the row loop.  We compute them inside the geometry
 *             block, so they are emitted at 74..88 instead.
 *   the rest  1-2 instruction schedule noise, ~15 slots in total, spread
 *             over the depth-key and strip-clip blocks.
 * The two ten/twelve-instruction regions nearly cancel, which is why indices
 * 108..173 are simply SHIFTED by ten and why the geometry window costs 119 of
 * the 381 strict mismatches while being only 27 structurally wrong.
 *
 * *** THE GEOMETRY BLOCK IS A HARD TWO-ATTRACTOR LEVER.  Fixing either half
 * kills the SPLIT PROLOGUE. ***  Measured on the current file:
 *     ylimit after the switch          geom 27 -> 21, total structural 57 ->
 *                                      51, but strict 381 -> 815 (prologue
 *                                      survives, first=67, but the whole low
 *                                      frame renumbers);
 *     xlimit after the switch          geom -> 24, strict 798, first=0;
 *     both after the switch            geom -> 22, strict 551, first=5;
 *     qx/rx pair first                 geom -> 32, strict 845, first=5;
 *     both + qx/rx first               geom -> 15 (the BEST geometry seen,
 *                                      and structurally the best whole
 *                                      function: 51 slots), strict 532,
 *                                      first=5.
 * first=5 means all four callee-saved pushes moved to the entry: the
 * geometry block's demand for esi/edi forces the g_edit_state gate's
 * constants 0 and 1 into ebx/ebp, whose live ranges then start at index 5.
 * The original needs esi/edi in BOTH places and still splits, so there is a
 * register-preference lever here that is not yet identified.  Three
 * respellings of the gate (separate if/else for g_show_cursor; a leading
 * `g_show_cursor = 0;` with a single positive arm; the current form) do not
 * change it: 532 / 854 / 532.  WHOEVER PICKS THIS UP: this is the single
 * biggest remaining win -- it is worth ~90 strict mismatches -- and it needs
 * the prologue-split rule, not another statement permutation.
 *
 * ===========================================================================
 * RULED OUT THIS ROUND (scores are this round's own baselines, 381/384/386)
 * ===========================================================================
 *   - 90 random topological orders of the 16-statement geometry block plus a
 *     full move-one-statement local search to convergence (two passes): the
 *     committed order (sh, th, sw, hh, sy, tw, xlimit, ylimit, qy, ry, sx,
 *     hw, qx, rx, tile.x, tile.y) is a sharp local optimum; the best random
 *     order found was 458 and the second 520.  The previous round's order
 *     differed only in ylimit's position and scored 386;
 *   - all 24 permutations of the ctx_build fill {mode, obj, kind, base}:
 *     the committed mode/obj/kind/base is best (next best +1);
 *   - all 24 of the ctx_normal fill: mode/obj/kind/base best at 381,
 *     mode/obj/base/kind ties;
 *   - 9 orders of the object-queue loop head {cell, sd zero-fill,
 *     g_sort_count = 0, flags &= ~0x400} and 3 orders of the six sd fields:
 *     converged (moving the sd fill after `flags &=` costs 6, the field
 *     order costs 3);
 *   - aleft before key1 (no change), bright before key2 (+6), `base =
 *     cell->base` after the tile fills (+260!), tile.y before tile.x at
 *     either footprint corner (no change on strict, +4 structurally);
 *   - the four-temp form at sites B and C WITHOUT `mode = 0xff00` first
 *     (517 / 499) -- the two levers only pay off together;
 *   - separate gx2/gy2/hx2/hy2 per arm instead of shared temps: no change;
 *   - EmitObjectSprite taking two ints instead of `Pos at` by value: no
 *     change on strict, and `at` as two plain int locals costs 152 and
 *     shrinks the frame to 0x2f8c;
 *   - pinning key2 against 17 different partners (bright, aleft, key1, d1,
 *     span, step, qx, qy, hw, hh, sy, quad, three, d2, px, py, rowy): rowx,
 *     qx, py and rowy all tie at 381 and everything else is worse or loses
 *     the 4-byte slot (frame 0x2f8c, first=0).  KEEP THE `pin` STRUCT;
 *   - `hh` from `sh`, `th = (int)sh`, `sw = (short)(sh * 2)`, dividing by
 *     `sh` instead of `th`: all byte-identical.  Dividing by `sw` or taking
 *     `hw` from `sw` costs 430 (it breaks the prologue);
 *   - `desc->sprite = def->build_sprite;` moved before or between the two
 *     `at` assignments: +2 / +3.
 *
 * ===========================================================================
 * ROUND w9 -- THE COMPENSATING ERROR IS IDENTIFIED, AND IT IS THE SAME BIT
 * ===========================================================================
 * The +10 region at ours[98:108] that cancels the -11 at orig[164:176] is
 * NOT a scheduling accident: **it is `xlimit` (and `ylimit`) being computed
 * inside the geometry block instead of after the quadrant switch.**  The
 * original computes them at 162-175, immediately before the row loop and in
 * the order ylimit, xlimit:
 *      162 mov edx,[g_map]        163 mov ecx,[esp+F50]   (view.bottom)
 *      164 mov ebx,[esp+F4c]      (view.right)
 *      165 xor edi,edi            166 mov di,[edx+0x12]   (view_h, u16)
 *      167 add edi,ecx            170 mov [esp+F30],edi    <- ylimit
 *      168 mov ecx,[esp+F20]      169 add ecx,ecx          (tw+tw)
 *      171 add ecx,ebx            172 mov [esp+F2c],ecx    <- xlimit
 *      173 mov ecx,[esp+F10] / 174 add ecx,ecx / 175 mov ebx,ecx  (2*th)
 * Moving BOTH out of the block removes BOTH regions -- the region report
 * loses the 98..108 insert and the 164..176 replace entirely -- but it
 * breaks the split prologue (pushes become ebx@5 ebp@7 esi@15 edi@21) and
 * the strict count goes 381 -> 548.  All 24 orders of the four leading
 * constant statements {count = 0, cls, g_bg_full_update = 1,
 * g_view_dirty = 0} on top of that give the identical broken push order, so
 * the leading block is inert for the bit.  **The +10 region therefore CANNOT
 * be fixed before the push-order bit falls: they are one problem.**  That
 * retires the standing instruction to adopt geo_abcjdefghlikmno "once the
 * +10 is fixed"; the two measured honest bodies are recorded here instead:
 *      s1  ylimit alone after the switch, committed geometry order:
 *          region total 89, register+offset 51, offset-blind 225, mnem 39,
 *          bytes 2885, SPLIT KEPT, strict 814
 *      s2  geo_abcjdefghlikmno's order plus ylimit late:
 *          region total 86, register+offset 49, offset-blind 187, mnem 38,
 *          bytes 2891, SPLIT KEPT, strict 817
 * Both are structurally better than the committed body (99 / 57 / 228 / 45 /
 * 2893) on every measure, and both are what the disassembly says; neither is
 * committed, because the strict count is what the file's marker reports and
 * a 381 -> 814 headline regression buys nothing until the prologue falls.
 * The variants are scratchpad/w9renderview/s1_ylate.c and s2_geoJ.c.
 *
 * NEW ON THE PROLOGUE BIT: what esi and edi actually HOLD in the original is
 * the two constants -- `xor edi,edi` at 15 is the 0 that feeds count,
 * g_view_dirty and the `g_show_cursor = 0` arm, and `mov esi,1` at 21 is the
 * 1 that feeds g_bg_full_update, the `g_edit_state == 1` compare and the
 * `g_show_cursor = 1` arm -- and edi is then REUSED for `th` and esi for
 * `sy`.  In a broken variant the constants still get esi/edi and the
 * geometry still gets ebx/ebp; what changes is only that `th` goes to ebp
 * instead of coalescing with the const-0 web in edi, and the ebx/ebp pushes
 * consequently take the two entry holes instead of sinking to 58/60.  So the
 * bit to attack is narrower than "which pair is allocated first": **does
 * `th`'s web coalesce with the constant-zero register.**
 *
 * Tooling for all of this is in scratchpad/laneG/:  sc.py (audit-exact score
 * plus strict / register-blind / offset-blind / both-blind LCS), regions.py
 * (LCS-aligned structural region report -- USE THIS, not the strict count,
 * when a change displaces a block), wscore.py (per-window structural and
 * strict breakdown), fm.py + fmd.py (the frame slot map for either side),
 * geo.py (topological search over the geometry block), try_.py (textual
 * variant runner).
 * ------------------------------------------------------------------------- */
/* Scope I (2026-09-05): at the measured geometry/prologue and
 * frame-reference floor, 381/903 strict, first 67, 2893/2880 bytes. BOTH
 * documented statement-placement corrections were tested together: limits
 * after the quadrant switch (y first), and sort-count reset after base in
 * the non-null object arm. Result: strict 549, first 5, 2887 bytes. Proper
 * CFG/stack/import-aware edit distances all regress: strict/rb/ob/both
 * 321/189/249/90 -> 353/220/287/118. The paired experiment does not solve
 * the split prologue. Retain this baseline with the two known, behaviorally
 * unobservable placement discrepancies explicitly documented above.
 * Full measurements: docs/lanes/scope-i.md.
 */
/* Scope LL20 (2026-09-08): unchanged at 381/903, first 67, 2893/2880 B.
 * Re-measured the qx-first attractor with a frame map that resolves homes by
 * push depth (scratch fm.py; the LL9/LL10/LL14/LL17/LL18/LL19 levers were
 * the brief).  What the qx-first order (sx, th, tw, qx, hw, hh, sy, rx, qy,
 * ry; limits after the switch) actually does, read off its object:
 *   - 64..87 become shape-identical to the original with ONE swap, sx in ebx
 *     and tw in ebp (original ebp/ebx); th is spilled at frame+0xc instead
 *     of +0x0 and stays a MEMORY divisor at 91/96 (the original reloads it
 *     into ebx at 88 and gives ebx to rx only at 97).
 *   - the gather loop flips count/px: the original keeps `count` in memory
 *     (248-253 `mov edx,[count] / inc / mov`) and px in ebx; the variant
 *     keeps count in ebx (`inc ebx`, spilled at the row-loop exit) and px in
 *     ecx+memory.
 *   - the entry const-0 web (edi) is then EXTENDED with rematerialisations at
 *     352 (class walk: `cmp eax,edi` / `cmp esi,edi`), 378 (queue head: all
 *     six sd zero stores, g_sort_count and the obj test, where the original
 *     materialises the two scratch zeros edx/ecx) and 863 (tail: count > 0,
 *     cell->obj, g_show_cursor).  In the tail the zero holds edi, so n goes
 *     to ebx, ebx lives to the epilogue, the {ebx,ebp} save pair can no
 *     longer bracket [58,866] (the original pops them at 866/868) and both
 *     pushes move to the entry: pushes ebx@5 ebp@7 esi@15 edi@21, strict
 *     519 / rb 479 / ob 476, first 5, 2875 B.  This is the RA09 tail-zero
 *     mechanism, now traced to its root in the gather loop.
 * Fourteen spellings on top of that order, all aimed at the extended zero web
 * or the count/px rank, and ALL byte-identical to it (519) unless noted:
 * `while (--count)` as the tail counter (533); `if (count > 0) { n = count;`
 * (533); a volatile count reload (614, frame 0x2f8c); a volatile
 * g_show_cursor read; a volatile cell->obj read (527); the LL10 pin
 * `if (count) ;`; block-scope n/pp; `g_sort_count = 0` inside the non-null
 * arm (520 at exactly 2880 B); the same plus the count loop (534);
 * `visible[count] = owner; count++;`; px declared ahead of count; px
 * block-scoped in the row loop.  On the committed order the same
 * `g_sort_count` move is 386 / 2894 B (matches round w10).  The zero-web
 * extension is not reachable from the tail, the counter or the sort-count
 * placement; it is decided with the geometry allocation.  Retired at 381:
 * the committed order is the only one measured that keeps the split
 * prologue, and every order that fixes 64..101 pays the prologue and the
 * whole low frame.  Frame map (push-depth resolved) agrees slot for slot
 * from +0x34 up; the low thirteen carry 134 references against the
 * original's 130.
 */
// WIP-FUNCTION: LEGOLAND 0x0045b180  (57.8%, 381/903 strict; paired placement corrections tested and rejected; first 67)
void RenderView(void)
{
    Cell*       visible[3000];
    ClipRect    saved;
    ClipRect    view;
    BlitCtx     ctx_build;
    BlitCtx     ctx_normal;
    TileBounds  corner;
    TileBounds  tb;
    SpriteDesc  sd;
    Pos         tile;
    BPos        base;
    ObjDef*     cls;
    Cell*       cell;
    Cell*       owner;
    Cell**      pp;
    ClipRect*   c;
    ClipRect*   prev;
    SpriteDesc* desc;
    short       sh, sw;
    int         count;
    int         sx, sy;
    int         th, tw, hw, hh;
    int         qx, qy, rx, ry;
    int         quad;
    int         px, py, xlimit, ylimit;
    int         rowy;
    /* FRAME-LAYOUT PIN -- a VC6 codegen lever, not semantics.  The two
     * members are unrelated (`rowx` is the gather walk's row-start tile x,
     * `key2` the second depth key of an object's footprint); they are in one
     * aggregate only because the original gives `key2` a frame home of its
     * own and VC6 otherwise colour-shares it with a gather-loop slot, leaving
     * the frame 4 bytes short (0x2f8c instead of 0x2f90).  Pinning `key2`
     * beside ANY local whose live range does not overlap it restores the
     * slot; `rowx` measured best.  See the note above. */
    struct { int rowx; int key2; } pin;
    int         n;
    int         key1, key;
    int         aleft, bright;
    int         d1, d2, span, step, three;
    int         mode;
    int         hx, hy, gx, gy;
    Pos         at;
    unsigned short f;

    view.left = g_map->origin_x;
    view.top = g_map->origin_y;
    view.right = g_map->view_w + g_map->origin_x;
    view.bottom = g_map->view_h + g_map->origin_y;
    count = 0;
    cls = g_odf_head;
    g_bg_full_update = 1;
    g_view_dirty = 0;
    GetClipping(&saved);
    RenderGroundLayer();
    PrintBackground(g_bg_dirty_x, g_bg_dirty_y);
    if (g_edit_state == 2 || (g_edit_state == 1 && g_edit_object == g_env_class)) {
        g_show_cursor = 1;
        g_cursor_blink = (GetTickCount() & 0x100) ? 0xff : 0;
    } else {
        g_show_cursor = 0;
    }
    SetClipping(&view);

    sh = g_tile_sprites[g_default_tile]->h;
    th = sh;
    sw = (short)(sh + sh);
    hh = (th + 1) >> 1;
    sy = (g_scroll_y >> 8) - hh;
    tw = sw;
    xlimit = tw + tw + view.right;
    ylimit = g_map->view_h + view.bottom;
    qy = sy / th;
    ry = sy % th;
    sx = g_scroll_x >> 8;
    hw = (tw + 1) >> 1;
    qx = sx / tw;
    rx = sx % tw;
    tile.x = qx + qy - 3;
    tile.y = qy - qx;
    if (rx >= hw)
        quad = 2;
    else
        quad = 1;
    if (ry > hh)
        quad += 2;
    switch (quad) {
    case 1:
        if (rx < hw - 2 * ry) {
            tile.x--;
            rx += hw;
            ry += hh;
        }
        break;
    case 2:
        if (rx >= hw + 2 * ry) {
            tile.y--;
            rx -= hw;
            ry += hh;
        }
        break;
    case 3:
        if (rx < hw + 2 * (ry - th)) {
            tile.y++;
            rx += hw;
            ry -= hh;
        }
        break;
    case 4:
        if (rx >= hw + 2 * (th - ry)) {
            tile.x++;
            rx -= hw;
            ry -= hh;
        }
        break;
    }

    for (py = view.top - (th + th) - ry; py < ylimit; py += th) {
        pin.rowx = tile.x;
        rowy = tile.y;
        for (px = view.left - (tw + tw) - rx; px < xlimit; px += tw) {
            cell = CellAt(tile.x, tile.y);
            if (cell) {
                RenderViewCellProbe(&tile);
                if (cell->flags & 0xa0) {
                    owner = CellAt(cell->base.x, cell->base.y);
                    f = owner->flags;
                    if (!(f & 0x400)) {
                        visible[count++] = owner;
                        owner->flags = (unsigned short)(f | 0x400);
                    }
                }
            }
            tile.x++;
            if (cell) {
                if (tile.x == g_map->width)
                    break;
                cell = cell + 1;
            }
            if (cell == 0)
                cell = CellAt(tile.x, tile.y);
            if (cell) {
                RenderViewCellProbe(&tile);
                if (cell->flags & 0xa0) {
                    owner = CellAt(cell->base.x, cell->base.y);
                    f = owner->flags;
                    if (!(f & 0x400)) {
                        visible[count++] = owner;
                        owner->flags = (unsigned short)(f | 0x400);
                    }
                }
            }
            tile.y--;
        }
        tile.x = pin.rowx + 1;
        tile.y = rowy + 1;
    }

    while (cls) {
        if (cls->flags & 0x20) {
            if (cls->prerender)
                cls->prerender(cls->ctx);
        }
        cls = cls->next;
    }

    visible[count] = 0;
    if (count > 0) {
        pp = visible;
        n = count;
        do {
            cell = *pp;
            sd.f0c = 0;
            sd.mode = 0;
            sd.sprite = 0;
            sd.dx = 0;
            sd.dy = 0;
            sd.layer_mask = 0;
            g_sort_count = 0;
            cell->flags &= (unsigned short)~0x400;
            if (cell->obj != 0) {
                ObjDef* def = ((Obj*)cell->obj)->def;

                base = cell->base;
                tile.x = cell->base.x + def->rleft;
                tile.y = cell->base.y + def->rbottom;
                GetTileBounds(&tile, &corner);
                key1 = ((corner.top + corner.bottom) >> 1) - g_map->origin_y;
                aleft = corner.left;
                tile.x = cell->base.x + def->rright;
                tile.y = cell->base.y + def->rtop;
                GetTileBounds(&tile, &corner);
                pin.key2 = ((corner.top + corner.bottom) >> 1) - g_map->origin_y;
                bright = corner.right;
                if (key1 != pin.key2) {
                    tile.x = cell->base.x + def->rleft;
                    tile.y = cell->base.y + def->rtop;
                    GetTileBounds(&tile, &corner);
                    d1 = ((corner.left + corner.right) >> 1) - aleft;
                    tile.x = cell->base.x + def->rright;
                    tile.y = cell->base.y + def->rbottom;
                    GetTileBounds(&tile, &corner);
                    d2 = ((corner.left + corner.right) >> 1) - aleft;
                    if (d2 < d1) {
                        span = d2 + d2;
                        step = -(span / 4);
                    } else {
                        span = d1 + d1;
                        step = span / 4;
                    }
                    g_sort_keys[g_sort_count] = key1;
                    key = key1 + step;
                    c = &g_sort_clips[g_sort_count];
                    g_sort_count++;
                    c->top = view.top;
                    c->bottom = view.bottom;
                    c->left = view.left;
                    three = span * 3 / 4;
                    c->right = aleft + three;
                    if (c->right + three < bright) {
                        g_sort_keys[g_sort_count] = key;
                        key += step;
                        c = &g_sort_clips[g_sort_count];
                        g_sort_count++;
                        c->top = view.top;
                        c->bottom = view.bottom;
                        c->left = aleft + three;
                        c->right = aleft + span * 5 / 4;
                    }
                    while (c->right + three < bright) {
                        prev = c;
                        c = &g_sort_clips[g_sort_count];
                        g_sort_keys[g_sort_count] = key;
                        g_sort_count++;
                        key += step;
                        c->top = view.top;
                        c->bottom = view.bottom;
                        c->left = prev->left + span / 2;
                        c->right = prev->right + span / 2;
                    }
                    prev = c;
                    g_sort_keys[g_sort_count] = pin.key2;
                    c = &g_sort_clips[g_sort_count];
                    g_sort_count++;
                    c->top = view.top;
                    c->bottom = view.bottom;
                    c->left = prev->right;
                    c->right = view.right;
                }
                if (def->flags & 0x400) {
                    if (def->draw == 0)
                        goto next;
                    desc = def->draw(def->ctx, base);
                    if (desc == 0)
                        goto next;
                } else {
                    sd.sprite = def->sprite;
                    sd.dx = def->dx;
                    sd.dy = def->dy;
                    sd.mode = 0;
                    desc = &sd;
                }
                tile.x = cell->base.x;
                tile.y = cell->base.y;
                GetTileBounds(&tile, &tb);
                f = cell->flags;
                if (f & 0x20) {
                    mode = desc->mode;
                    ctx_build.obj = cell->obj;
                    ctx_build.kind = 0x104;
                    ctx_build.base = cell->base;
                    if (def->build_sprite) {
                        gx = desc->dx + def->anim_dx;
                        gy = desc->dy + def->anim_dy;
                        hx = HalfOffset(gx);
                        hy = HalfOffset(gy);
                        at.x = tb.left + hx;
                        at.y = tb.top + hy;
                        desc->sprite = def->build_sprite;
                        SetOverrideFrame(GetBuildAnimFrame(def, base));
                    } else {
                        mode = 0xff00;
                        gx = desc->dx;
                        gy = desc->dy;
                        hx = HalfOffset(gx);
                        hy = HalfOffset(gy);
                        at.x = tb.left + hx;
                        at.y = tb.top + hy;
                    }
                    EmitObjectSprite(desc, at, key1, mode, &ctx_build);
                } else {
                    mode = desc->mode;
                    ctx_normal.obj = cell->obj;
                    ctx_normal.kind = 0x103;
                    ctx_normal.base = cell->base;
                    gx = desc->dx;
                    gy = desc->dy;
                    hx = HalfOffset(gx);
                    hy = HalfOffset(gy);
                    at.x = tb.left + hx;
                    at.y = tb.top + hy;
                    if (!(f & 4)) {
                        if ((f & 0x200) && GetBlink()) {
                            mode = 0xff0000;
                        } else if (g_power_available && (cell->flags & 0x100)
                                   && !GetBlink()) {
                            mode = 0xffff;
                        }
                    }
                    EmitObjectSprite(desc, at, key1, mode, &ctx_normal);
                }
                ClearOverrideFrame();
            }
        next:
            pp++;
        } while (--n);
    }

    RenderPeople();
    RenderWorkers();
    DrawAndClearPrintList();
    n = count;
    if (n > 0) {
        pp = visible;
        do {
            cell = *pp;
            if (cell->obj && (cell->flags & 0x20)) {
                base = cell->base;
                DoBuildEffects(((Obj*)cell->obj)->def, base);
            }
            pp++;
        } while (--n);
    }
    if (g_show_cursor) {
        PushRenderingStatusAndLockVideoSurface();
        DrawEditCursor();
        FlushCursorSpriteList();
        PopRenderingStatus();
    }
    RenderWorkerInterfaceGFX();
    SetClipping(&saved);
}

/* ===========================================================================
 * RenderFullMap (0x004567a0) -- THE OVERVIEW MAP
 * ===========================================================================
 * mapscreen.c registers this as the draw callback of a 640x340 FUNCTION-BASED
 * sprite (CreateFunctionBasedSprite, sprite2.c), so it runs once whenever the
 * map screen's sprite is printed and paints the whole overview from scratch.
 * It is re-entrancy guarded by g_fullmap_busy (0x00667c30): if that is already
 * non-zero the call returns immediately, having done nothing but resolve its
 * elements and create its GDI pen (the early-out jumps straight to the
 * epilogue, so the pen and the three sprites LEAK on that path -- an original
 * bug, reproduced below).
 *
 * ---------------------------------------------------------------------------
 * Set-up
 * ---------------------------------------------------------------------------
 *   Elements resolved by name (ElemID): SQUARE_TRACK, SQUARE_TRACK_HEIGHT,
 *   SQUARE_TRACK_HEIGHT_0, SQUARE_TRACK_HEIGHT_PATH, CASTLE_DUMMY and
 *   DRIVING SCHOOL ROADS -- the five that get the roller-coaster treatment
 *   plus the driving-school roads.  Sprites loaded: TRACKBLOB.LLS,
 *   TRACKSTICK.LLS, MAPLIGHTS.LLS (all freed with KillSprite on the way out).
 *   A 2px GDI pen of colour 0x00ff4000 is created for the coaster polyline.
 *
 *   The map render chain is rebuilt by the full-map variant at 0x0045a660 (it
 *   is CalculateMapRenderOrder's sibling: same column scan, but it ALSO chains
 *   cells whose flags carry 0x08 when their object is the DRIVING SCHOOL ROADS
 *   element).  The normal chain is rebuilt by CalculateMapRenderOrder at the
 *   very end, so the world view is unaffected.
 *
 *   The 32x32 mark grid at 0x008119c0 (mapscreen.c draws marks from it, gated
 *   by the detail byte 0x008119a4 & 0x10) is cleared -- 0x800 dwords = 32*32
 *   {x,y} pairs.  The map-screen view rectangle at 0x008139c0 is reset to
 *   {h = 340, w = 640, x = 0, y = 0x20}, the clip is stored and set to
 *   {0, 0, 640, 340}, and the map's view origin (Map +0x20/+0x22) is saved and
 *   zeroed so GetTileBounds returns UNSCROLLED world pixels.
 *
 * ---------------------------------------------------------------------------
 * The projection (world pixels -> minimap pixels)
 * ---------------------------------------------------------------------------
 *   tw = 2*h, th = h            (GetTileDimensions: h = the ground sprite's)
 *   g_fm_ox (0x667c00) = -(map->height * tw / 2)     world x of minimap x = 0
 *   g_fm_oy (0x667c04) = 0                           world y of minimap y = 0
 *   0x667c08            =  map->width * tw / 2
 *   0x667c0c            = (map->width + map->height) * th / 2
 *   g_fm_spanx (0x667c1c) = 0x667c08 - g_fm_ox + 1   world width  of the map
 *   g_fm_spany (0x667c18) = 0x667c0c + 1             world height of the map
 *   scale_x  = (640 << 16) / g_fm_spanx              16.16 fixed point
 *   scale_y  = (340 << 16) / g_fm_spany
 *   g_fm_cw (0x667c16) = (short)(640 * tw / spanx + 1.0)   minimap cell size
 *   g_fm_ch (0x667c14) = (short)(340 * th / spany + 1.0)
 *   Both are computed with the x87 in INTEGER-OPERAND form (fild view.w,
 *   fimul tw, fidiv spanx, fadd the pooled 1.0f at 0x4ab38c, __ftol) and
 *   stored as 16-bit words -- they are scratch, and passes 3 and 4 overwrite
 *   them with the sprite dimensions of whatever they are about to blit.
 *
 *   g_fm_cy (0x667c20) = (340 - ((map->w + map->h - 2) * ((th+1)>>1)
 *                                - g_fm_oy) * 640 / spany / 2) >> 1
 *   (the subtraction of g_fm_oy is real -- it is 0 in the shipped code but the
 *   instruction is there; the outer halving is an arithmetic SHIFT, the inner
 *   one a signed divide, and the two are not interchangeable in C).
 *
 *   minimap_x(world) = ((world_x + (g_scroll_x >> 8)) - g_fm_ox) * scale_x >> 16
 *   minimap_y(world) = ((world_y + (g_scroll_y >> 8)) - g_fm_oy) * scale_y >> 16
 *                      + g_fm_cy
 *   world_x/world_y come from GetTileBounds (left/top of the tile diamond).
 *   Every one of the seven projection sites writes the scrolled value BACK
 *   into the TileBounds it came from, so the helper takes the rect by pointer
 *   and mutates it (FullMapX / FullMapY below).  Sprite offsets are folded in
 *   before the projection: tb.left += HalfOffset(desc->dx) etc.
 *
 * ---------------------------------------------------------------------------
 * THE PER-CELL COLOUR RULE (pass 1: the terrain wash)
 * ---------------------------------------------------------------------------
 *   The whole 640x340 area is first filled black (GetNearestColour(0,0,0)),
 *   then every map cell (y outer, x inner) is classified:
 *
 *     - flags & 0x0008 (an object cell)          -> nothing here; the object
 *                                                   is drawn in pass 2.
 *     - flags & 0x0010 AND flags & 0x0080        -> GREY  0x80,0x80,0x80
 *       (a path tile that is also an object footprint: roads/paths)
 *     - otherwise the DISPLAYED TILE decides.  desc = g_tile_info[tile].set
 *       is the TSF descriptor (base slot at +0x00, code[] at +0x0c), so
 *           code = desc->code[tile - desc->base_slot] & 0x3f
 *       and code selects the colour through a real C `switch`: `cmp esi,0x30 /
 *       ja skip` is the range check, then the 0x31-BYTE index table at
 *       0x00457834 feeds the 4-entry jump table at 0x00457824.  Both were read
 *       out of the image, so the mapping is exact, not inferred:
 *           index bytes  [0]=0 [1]=1 [2..0x1f]=3 [0x20]=0 [0x21]=1
 *                        [0x22..0x2f]=3 [0x30]=2
 *           target 0 (0x456b91) code 0x00, 0x20 -> GREEN r=0x00 g=0x8f b=0x4f
 *           target 1 (0x456bb8) code 0x01, 0x21 -> SAND  r=0xff g=0xe0 b=0x8f
 *           target 2 (0x456be5) code 0x30       -> LIME  r=0x5b g=0xbe b=0x02
 *           target 3 (0x456c1a) everything else -> not drawn (black = water)
 *       so only five of the 0x31 terrain codes paint anything; the whole rest
 *       of the map reads as water.
 *
 *     The block painted is RenderBlock(mx - 7, my - 2, g_fm_cw + 5,
 *     g_fm_ch + 4, colour) -- i.e. each cell is a slightly oversized filled
 *     rectangle so the diamonds tile without gaps.
 *
 *     CODEGEN NOTE: RenderBlock is called INSIDE each of the four arms, not
 *     once after a `colour` join.  VC6's cross-jumper then merges only the
 *     common suffix (`add ebp,-2 / add edi,-7 / push / push / call`), leaving
 *     three separate GetNearestColour calls, and merges the GREY arm entirely
 *     into the SAND arm from its `call` onward (the grey arm is just
 *     `push 0x80 x3 / jmp 0x456bc7`).  Writing it as one call after a `colour`
 *     variable collapses all four arms into a single call and loses ~14
 *     instructions.
 *
 * ---------------------------------------------------------------------------
 * Pass 2: object cells
 * ---------------------------------------------------------------------------
 *   The surface is unlocked (PushRenderingStatusAndUnlockVideoSurface) so the
 *   scaled blitter can work, then every cell with flags & 0x0008 draws its own
 *   displayed tile sprite g_tile_sprites[cell.tile] with PrintScaledSprite at
 *   the projected position, offset by half the object class's render offset
 *   (class +0x14/+0x18, halved toward zero by the helper at 0x00456770), and
 *   scaled to (g_fm_cw + 1) x (g_fm_ch + 1).
 *
 * Pass 3: the perimeter terrain objects
 *   The list at 0x00667ca8 (built by BuildPerimeterObject, see loadmap.c):
 *   each node {+0x14 x, +0x18 y, +0x1c next, +0x20 sprite} is drawn scaled at
 *   (base + (node->x + 2*tw/3) * scale_x >> 16, base + node->y * scale_y >> 16)
 *   with size (sprite->w * scale_x >> 16) x (sprite->h * scale_y >> 16).
 *
 * Pass 4: the render chain (GetFirstRenderObject / GetNextRenderObject)
 *   For each chained object cell, a 20-byte copy of the cell is taken and:
 *     - if flags & 0x0200 and not flags & 0x0004, the object's MARK is stored
 *       in the 32x32 grid at 0x008119c0: index ((by>>3)<<5)|(bx>>3), value the
 *       projected position of the tile at ((bx & ~7)+4, (by & ~7)+4).  That is
 *       the grid mapscreen.c draws the little markers from.
 *     - class = cell.obj->+0x0c.  Then:
 *         * class +0xc4 equal to one of SQUARE_TRACK, SQUARE_TRACK_HEIGHT,
 *           SQUARE_TRACK_HEIGHT_0, SQUARE_TRACK_HEIGHT_PATH or CASTLE_DUMMY
 *           -> the ROLLER-COASTER path.  The ride query at 0x00424050 takes
 *           FIVE arguments -- GetTrackSegment(Pos* tile, float* h0, Pos* p1,
 *           float* h1, int* link) -- and `tile` is IN-OUT: it goes in holding
 *           the chain cell's base coordinate and comes back holding this
 *           piece's own node, while p1 is the far node.  Returns 0 when the
 *           cell carries no piece.  Then:
 *             h0 = max(h0, 0), h1 = max(h1, 0)   (compared against the pooled
 *                 0.0f at 0x4ab390, stored back as an integer 0)
 *             (x0,y0) = project(tile) with tb.top nudged by
 *                       HalfOffset(-(int)h0); (x1,y1) likewise from p1/h1.
 *             unless the class is SQUARE_TRACK_HEIGHT_PATH, the support stick:
 *                 g_fm_cw = TRACKSTICK->w
 *                 g_fm_ch = (short)(int)(scale_y * h0 * 2^-17)   (0x4ab514)
 *                 if (g_fm_ch > 0) PrintScaledSprite(TRACKSTICK,
 *                     x0 - ((g_fm_cw*scale_x)>>17), y0,
 *                     (g_fm_cw*scale_x)>>16, g_fm_ch)
 *                 -- the half-width shift is applied to X (the stick is
 *                 centred on the node), NOT to Y.
 *             then the surface is unlocked, GetDC/SelectObject(pen),
 *             MoveToEx(x0,y0), LineTo(x1,y1), and IF `link` came back non-zero
 *             a second LineTo to the projection of the tile
 *             {tile.x - 0xa, tile.y} (again nudged by HalfOffset(-(int)h0)),
 *             then SelectObject(old)/ReleaseDC/PopRenderingStatus.
 *           TRACKBLOB.LLS is NEVER drawn: the arm ends by filling the local
 *           SpriteDesc with {TRACKBLOB, dx = 0, dy = (int)(-h0)} and falling
 *           straight through to the next chain entry.  Three dead stores --
 *           an original bug (the descriptor was presumably meant to feed the
 *           common draw tail), and the same three stores also run when
 *           GetTrackSegment returned 0, reading an uninitialised h0.
 *         * class flags (+0x1c) & 0x0004 or & 0x0400 -> the normal object:
 *           either the class draw callback (+0xa0, called with +0xc4 and the
 *           packed base coordinate forwarded as a dword) or the static
 *           {sprite +0x64, dx +0x14, dy +0x18}; the descriptor and its sprite
 *           are both null-checked.  A sprite whose +0x10 word carries 0x8000
 *           is an ILF: its layer table hangs off sprite+0x08 and each layer is
 *           drawn at HalfOffset(desc->dx)+HalfOffset(layer dx) -- and the loop
 *           bound is re-read through desc->sprite->table->count on EVERY
 *           iteration, not cached.  Otherwise the single sprite is drawn
 *           scaled.  In both cases g_fm_cw/g_fm_ch are overwritten with the
 *           sprite's own w/h before the blit.
 *         * neither flag set -> only the DRIVING SCHOOL ROADS class draws, and
 *           only where the road record at 0x004125f0(x, y) has kind
 *           (+0x14 & 0x0f) == 5: MAPLIGHTS.LLS at (-0x33, -0x2c).  The arm
 *           first fills the SpriteDesc with {MAPLIGHTS, dx = -0x66, dy =
 *           -0x58} -- the -0x33/-0x2c the blit uses are those two halved, and
 *           VC6 folds them, but the three stores are still emitted because the
 *           descriptor is address-taken elsewhere.
 *
 * Epilogue: PopRenderingStatus, restore Map +0x20/+0x22, RestoreClipping,
 * CommitCliprectToHardware, KillSprite x3, DeleteObject(pen) and finally
 * CalculateMapRenderOrder to put the world-view chain back.  ebx/esi/edi are
 * popped BEFORE the three KillSprite tests; only ebp survives to the very end,
 * because the `if (g_fullmap_busy) return;` guard splits the prologue: ebp is
 * pushed at entry (it is the function-wide zero register -- CreatePen's style
 * argument and the guard's own `cmp eax,ebp` are pre-guard uses) and
 * ebx/esi/edi are pushed only after the guard falls through.
 *
 * ---------------------------------------------------------------------------
 * HOST / BROWSER-RENDERER CONTRACT for RenderFullMap
 * ---------------------------------------------------------------------------
 * Call shape: a void draw callback with no arguments, invoked by the
 * function-based sprite the map screen creates.  It paints the whole 640x340
 * overview from scratch every time; there is no incremental path and no
 * dirty-rectangle input.
 *
 * PRECONDITIONS: the LLIDB is loaded (all six ElemID lookups must resolve or
 * the corresponding class comparisons simply never match), Graphics*.res is
 * mounted (the three .LLS loads), the map grid and the render chain exist,
 * and a GDI-capable surface is available (the coaster polyline goes through
 * GetDC/MoveToEx/LineTo on the DirectDraw surface at 0x66807c).
 *
 * STATE IT OWNS AND LEAVES BEHIND:
 *   g_fullmap_busy   0x667c30  set to 1 and never cleared BY THIS FUNCTION
 *                    (verified: the only two references in the body are the
 *                    guard's read at 0x45685a and the `mov dword ptr
 *                    [0x667c30], 1` at 0x456a09).  Whatever clears it again
 *                    lives outside this translation unit; until it does,
 *                    every later call takes the early-out -- and that path
 *                    jumps straight to the epilogue, so it LEAKS the pen and
 *                    the three sprites it has just created.  That leak is an
 *                    original bug and it is reproduced below.
 *   g_fm_ox/oy       0x667c00 / 0x667c04   world origin of minimap (0,0)
 *   0x667c08/0x667c0c                      world extent halves
 *   g_fm_spanx/spany 0x667c1c / 0x667c18   world size + 1
 *   g_fm_cw/ch       0x667c16 / 0x667c14   SCRATCH u16 -- the terrain-wash
 *                    cell size at first, then overwritten by every sprite
 *                    blit with that sprite's own w/h.  Do not treat them as
 *                    stable outputs.
 *   g_fm_cy          0x667c20  the vertical centring offset every projection
 *                    adds; the only one of these a caller needs afterwards.
 *   g_map_marks      0x8119c0  32x32 {int x, int y} grid, cleared at entry
 *                    and filled with the projected position of every chained
 *                    object whose cell carries 0x200 and not 0x004.  This is
 *                    the ONLY durable output besides the pixels; mapscreen.c
 *                    draws the little markers from it, gated by the detail
 *                    byte 0x8119a4 & 0x10.
 *   g_fm_view        0x8139c0  reset to {h=340, w=640, x=0, y=0x20}
 *   Map +0x20/+0x22  saved, zeroed for the duration, restored at the end --
 *                    which is what makes GetTileBounds return UNSCROLLED
 *                    world pixels inside this function.
 *   The render chain is rebuilt twice: CalculateFullMapRenderOrder at the
 *                    start (which also chains DRIVING SCHOOL ROADS cells) and
 *                    CalculateMapRenderOrder at the very end, so the world
 *                    view is unaffected afterwards.
 *
 * PROJECTION, restated for a re-implementation (scale_x/scale_y are 16.16):
 *   scale_x = (640 << 16) / spanx ;  scale_y = (340 << 16) / spany
 *   mx = ((tile_left + (g_scroll_x >> 8)) - g_fm_ox) * scale_x >> 16
 *   my = ((tile_top  + (g_scroll_y >> 8)) - g_fm_oy) * scale_y >> 16 + g_fm_cy
 * Passes 1, 3 and 4 use exactly that; pass 2 uses the FLOAT form
 *   fsx = 640.0f / spanx ; fsy = 340.0f / spany
 *   mx = (int)((tile_left + halfoff_x + (g_scroll_x >> 8) - g_fm_ox) * fsx)
 * which rounds differently.  Both are in the original and they are not
 * interchangeable.
 * ------------------------------------------------------------------------- */

/* ---- RenderFullMap-only types ------------------------------------------- */

/* An LLIDB element (llidb_*.c): the parsed payload hangs off +0x0c. */
typedef struct Elem {
    char* name;                 /* +0x00 */
    char* image;                /* +0x04 */
    int   type_flags;           /* +0x08 */
    void* data;                 /* +0x0c */
    int   refcount;             /* +0x10 */
} Elem;

/* The TSF tile-sprite descriptor (llidb_load.c). */
typedef struct TileSet {
    int    base_slot;           /* +0x00 first global tile slot */
    int    n_tiles;             /* +0x04 */
    void** sprites;             /* +0x08 */
    int*   code;                /* +0x0c per-tile terrain code */
    int*   second;              /* +0x10 */
} TileSet;

/* The parallel tile-info table at 0x00801f40 (stride 8). */
typedef struct TileInfo {
    TileSet*     set;           /* +0x00 */
    unsigned int code;          /* +0x04 */
} TileInfo;

/* A perimeter cliff/bridge render node (loadmap.c BuildPerimeterObject). */
typedef struct TerrainObj {
    char               pad00[0x14];
    int                x;       /* +0x14 */
    int                y;       /* +0x18 */
    struct TerrainObj* next;    /* +0x1c */
    Sprite*            sprite;  /* +0x20 */
} TerrainObj;

/* The map-screen view rectangle at 0x008139c0 (mapscreen.c). */
typedef struct FullMapView {
    int h;                      /* +0x00 */
    int w;                      /* +0x04 */
    int x;                      /* +0x08 */
    int y;                      /* +0x0c */
} FullMapView;

/* One entry of the 32x32 mark grid at 0x008119c0. */
typedef struct MapMark {
    int x;                      /* +0x00 */
    int y;                      /* +0x04 */
} MapMark;

/* A driving-school road record (0x004125f0 looks one up by cell). */
typedef struct RoadRec {
    char pad00[0x14];
    unsigned char kind;         /* +0x14 low nibble: 5 = lit junction */
} RoadRec;

typedef struct DDSurface DDSurface;
typedef struct DDSurfaceVtbl {
    char pad00[0x44];
    long(__stdcall* GetDC)(DDSurface*, void** hdc);      /* +0x44 */
    char pad48[0x68 - 0x48];
    long(__stdcall* ReleaseDC)(DDSurface*, void* hdc);   /* +0x68 */
} DDSurfaceVtbl;
struct DDSurface { DDSurfaceVtbl* vtbl; };

extern TileInfo     g_tile_info[];          /* 0x00801f40 */
extern TerrainObj*  g_terrain_objects;      /* 0x00667ca8 */
extern FullMapView  g_fm_view;              /* 0x008139c0 */
extern MapMark      g_map_marks[32][32];    /* 0x008119c0 */
extern int          g_fullmap_busy;         /* 0x00667c30 */
extern int          g_fm_ox;                /* 0x00667c00 */
extern int          g_fm_oy;                /* 0x00667c04 */
extern int          g_fm_w2;                /* 0x00667c08 */
extern int          g_fm_h2;                /* 0x00667c0c */
extern short        g_fm_ch;                /* 0x00667c14 minimap cell height */
extern short        g_fm_cw;                /* 0x00667c16 minimap cell width  */
extern int          g_fm_spany;             /* 0x00667c18 */
extern int          g_fm_spanx;             /* 0x00667c1c */
extern int          g_fm_cy;                /* 0x00667c20 */
extern DDSurface*   g_draw_surface;         /* 0x0066807c */

__declspec(dllimport) void* __stdcall CreatePen(int style, int width, unsigned long colour);
__declspec(dllimport) void* __stdcall SelectObject(void* hdc, void* obj);
__declspec(dllimport) int   __stdcall DeleteObject(void* obj);
__declspec(dllimport) int   __stdcall MoveToEx(void* hdc, int x, int y, void* pt);
__declspec(dllimport) int   __stdcall LineTo(void* hdc, int x, int y);

extern Elem*   ElemID(const char* name);                            /* 0x0047b3f0 */
extern Sprite* LoadSprite(const char* name, int mode);              /* 0x00497ab0 */
extern void    KillSprite(Sprite* s);                               /* 0x00497bd0 */
extern void    GetTileDimensions(int* out_w, int* out_h);           /* 0x00460540 */
extern int     GetNearestColour(int r, int g, int b);               /* 0x0044e6c0 */
extern void    RenderBlock(int x, int y, int w, int h, int colour);  /* 0x004890c0 */
extern int     PrintScaledSprite(Sprite* s, int x, int y, int w, int h); /* 0x00485940 */
extern void    StoreClipping(void);                                 /* 0x0048a660 */
extern void    RestoreClipping(void);                               /* 0x0048a690 */
extern void    PushRenderingStatusAndUnlockVideoSurface(void);      /* 0x00464080 */
extern void    CommitCliprectToHardware(void);                      /* 0x00464370 */
extern void    CalculateFullMapRenderOrder(void);                   /* 0x0045a660 */
extern void    CalculateMapRenderOrder(void);                       /* 0x0045a4a0 */
extern Cell*   GetFirstRenderObject(void);                          /* 0x0045a850 */
extern Cell*   GetNextRenderObject(Cell* c);                        /* 0x0045a8b0 */
extern void*   memset(void* d, int c, unsigned int n);
#pragma intrinsic(memset)

extern void    HalfPos(Pos* p);                                     /* 0x00456770 */
extern RoadRec* GetRoadRecord(int x, int y);                        /* 0x004125f0 */
/* 0x00424050: the track-piece query.  `tile` is IN-OUT (it comes back holding
 * this piece's own node), `h0`/`h1` are the two node heights and `p1` the far
 * node's tile; `link` is a flag saying the segment has a second leg.  Returns
 * 0 when the cell carries no track piece. */
extern int     GetTrackSegment(Pos* tile, float* h0, Pos* p1, float* h1,
                               int* link);

/* An ILF layer table hanging off an 0x8000 sprite's +0x08 (printlist.c). */
typedef struct ILFTable {
    int      f00;               /* +0x00 */
    int      count;             /* +0x04 */
    Sprite** sprites;           /* +0x08 */
    int*     dx;                /* +0x0c */
    int*     dy;                /* +0x10 */
} ILFTable;

/* Each GetTileBounds site in the original builds its OWN Pos temporary (six
 * distinct stack slots), which is what an inlined helper gives us. */
/* Does this chain cell want an entry in the 32x32 mark grid?  (Cell +0x0c
 * bit 0x0200 set and bit 0x0004 clear.)
 *
 * TAKING THE CELL BY VALUE IS A FRAME-LAYOUT LEVER, not a style choice: the
 * 20-byte by-value argument of an inlined helper becomes an inline-expansion
 * TEMPORARY, and VC6 gives those homes in the pool at the top of the frame,
 * where a named local -- of any scope -- never lands.  That is what gives the
 * pass-4 cell copy its own 20-byte slot at frame+0xe4 the way the original
 * has it, instead of being colour-shared with the pass-1/2 copy down at
 * +0x54.  Passing the already-copied local `c` here instead of `*chain`
 * gives back the shared slot and 12 bytes of frame.  See the note above
 * RenderFullMap. */
static __inline int CellMarkTest(Cell c)
{
    return (c.flags & 0x200) && !(c.flags & 4);
}

static __inline void TileBoundsAt(int tx, int ty, TileBounds* out)
{
    Pos p;
    p.x = tx;
    p.y = ty;
    GetTileBounds(&p, out);
}

/* Project a tile's world-pixel corner onto the minimap.
 *
 * The scroll offset is added INTO the TileBounds -- the stores back into
 * tb.left/tb.top are visible at every one of the nine call sites -- but it is
 * a SEPARATE statement, not part of the projection: `tb` is address-taken, so
 * the g_fm_cw/g_fm_ch stores that follow it at four of the sites may alias it
 * and VC6 cannot hoist a scroll add written inside FullMapX/FullMapY past
 * them.  Doing so emitted a second load/store pair of tb.left and tb.top at
 * every sprite site; splitting it out is worth 77 structural slots.  See item
 * 1 of the round-w8 note above RenderFullMap. */
static __inline void FullMapScroll(TileBounds* t)
{
    t->left += (g_scroll_x >> 8);
    t->top += (g_scroll_y >> 8);
}

static __inline int FullMapX(TileBounds* t, int scale_x)
{
    return ((t->left - g_fm_ox) * scale_x) >> 16;
}

static __inline int FullMapY(TileBounds* t, int scale_y)
{
    return (((t->top - g_fm_oy) * scale_y) >> 16) + g_fm_cy;
}

/* -------------------------------------------------------------------------
 * 0x004567a0 -- the overview-map draw callback (see the write-up above).
 *
 * ===========================================================================
 * ROUND w10.  NO SCORE CHANGE (844, region total 465, big 303).  The round
 * was spent on the layout bit, and it produced two hard NEGATIVES that
 * retire standing hypotheses, one sharpened RULE with a sixth in-tree proof,
 * and a set of register facts read off the original.  READ THE NEGATIVES
 * BEFORE SPENDING ANOTHER ROUND ON THIS.
 * ===========================================================================
 *
 * 1. *** WITHDRAWN: "the layout bit and `desc`'s register are ONE problem". ***
 *    Direct diagnostic: make `scale_x` and/or `scale_y` `volatile` so they
 *    cannot hold ebx/ebp (not committable -- it also kills the CSE the
 *    original keeps -- but it is a clean instrument for "what if those two
 *    registers were free").  All three variants free the registers and
 *    reallocate `desc`, and the layout does not move a millimetre:
 *        both volatile   strict 1044  sd=682 join=695
 *        scale_x only    strict  892  sd=677 join=690
 *        scale_y only    strict 1056  sd=678 join=691
 *    (`sd` = index of the sd fill, `join` = index of `test ch,0x80`; the
 *    original is sd=707 AFTER join=686, ours is sd=674 BEFORE join=688.)
 *    So the bit is not a register-allocation consequence.  Round w9's
 *    "get `desc` into ebx and the one-statement duplication should merge" is
 *    hereby retracted.
 *
 * 2. *** THE EXILE RULE, STATED EXACTLY, WITH A SIXTH PROOF AND A LOCAL
 *    EXPERIMENT THAT CONFIRMS IT ON THIS VERY FUNCTION. ***
 *        An `else` arm is exiled past the fall-through trace IF AND ONLY IF
 *        THE ARM ITSELF ENDS IN AN UNCONDITIONAL JUMP.  An arm that falls
 *        into the join is laid out in place, always.
 *    Sixth proof, and the closest analogue in the tree to our own shape:
 *    `ScanBlokeSurroundings` 0x450530 -- FIVE guard conditionals (29, 34, 36,
 *    40, 46, exactly like our five track compares at 660..668) all target ONE
 *    two-instruction block `inc dword [esp+0x14]` / `jmp <latch>` which sits
 *    at index 355, past the epilogue, 300+ instructions away.  It is exiled
 *    because it ends in the `continue`.  `KillAllSamplesFromSource` 0x496b80
 *    shows the other half: VC6 TAIL-DUPLICATES a four-instruction tail
 *    (`FreePlayableSample(s); continue;`) into BOTH arms of `if (prev) ...
 *    else ...`, so both arms end in `jmp <latch>`, and then exiles the second
 *    one past the epilogue (53..57) while the first stays inline (28..32).
 *    Confirmed on THIS function by experiment: duplicating the tail down to
 *    and including the single-sprite block (whose copy ends in `continue`)
 *    DOES exile the sd fill and lands the join on the original's index
 *    exactly -- sd=759, join=686 -- while duplicating only the two null tests
 *    (so the arm still ends in a `je`) does not: sd=676, join=691.
 *
 * 3. *** THE CONSEQUENCE, AND WHY THE ROUTE IS CLOSED FROM BOTH ENDS. ***
 *    The original's join at 680 is a genuinely SHARED block: 682 is
 *    `mov eax,[ebx]`, a RELOAD of `desc->sprite` through desc's register, and
 *    the sd arm at 707..712 does not keep the stored sprite in any register
 *    (eax is reused at 710 for `def->dy`).  A plain if/else with a shared
 *    join therefore cannot put the else arm anywhere but immediately before
 *    the join -- and cross-jumping cannot rescue it, because a merged block's
 *    FALL-THROUGH SUCCESSOR must be shared too, so the merge cannot stop
 *    part-way: the merged suffix would have to be the whole remainder of the
 *    loop body.  Measured again at this round's baseline: with the whole tail
 *    textually in both arms the sd copy diverges immediately --
 *        cb copy   680 test edi,edi / 681 je / 682 mov eax,[edi] /
 *                  683 test eax,eax / 685 mov ecx,[eax+0x10] / 686 test ch,0x80
 *        sd copy   873 lea eax,[esp+0xcc] / 876 test eax,eax   (rematerialised
 *                  address, NOT desc's register) / 880 test esi,esi  (the
 *                  just-stored sd.sprite FORWARDED, no reload) /
 *                  882 mov eax,[esi+0x10] / 883 test ah,0x80
 *    -- three separate reasons the two copies are not the same machine code.
 *    Four spellings of the sd arm's assignment+test (`desc = &sd; if (desc ==
 *    0)`, `if (!desc)`, `if ((desc = &sd) == 0)`, and via a second pointer
 *    local) are byte-identical to one another: VC6 always rematerialises the
 *    `lea` for the test.
 *
 * 4. *** THE FRONT END NORMALISES EVERY `goto` FORM, INCLUDING THE ONE SHAPE
 *    THAT WOULD HAVE WORKED. *** Four more spellings, ALL byte-identical to
 *    the committed file (same 4205 bytes, same 844, same sd=674/join=688):
 *      - `if (flags & 0x400) { cb...; goto have_desc; }` with the sd fill as
 *        the source fall-through and `have_desc:` before the tail (braced and
 *        unbraced) -- this is the LoadObjectLibrary shape with the goto on
 *        the OTHER arm from the one round w9 tried;
 *      - if/else with `goto have_desc;` as the last statement of the else;
 *      - *** the whole tail written INSIDE the then arm with `have_desc:` in
 *        its middle and the else arm's only exit a `goto` into it. ***  This
 *        is the exact CFG the original has -- one shared tail, an else arm
 *        that can only reach it by a jump -- and VC6's front end flattens it
 *        back into the plain if/else.  Same again with the sd fill after the
 *        closing brace instead of in an else.
 *    And inverting the CONDITION with the bodies swapped
 *    (`if (!(def->flags & 0x400)) { sd } else { cb }`, plus `== 0` and an
 *    unsigned spelling) gives the MIRROR, sd=664 before join=687: VC6 does
 *    not invert a test to reorder arms.
 *    NET: no C source shape reachable from here expresses the original's
 *    block order with a shared tail.  Either VC6 SP3's block-ordering pass
 *    keys on something not yet identified (it is NOT registers -- see 1), or
 *    the shipped source contained something structurally different in the
 *    loop body that we have not reconstructed.  DO NOT SPEND ANOTHER ROUND
 *    ON RESPELLING THE SELECTION; the eleven w8 spellings, w9's five and
 *    these nine are all the same answer.
 *
 * 5. REGISTER FACTS READ OFF THE ORIGINAL (useful whatever the layout does):
 *    - `scale_x` is NOT one enregistered web.  It is a spilled local at
 *      frame +0x38 (stored at 112; `scale_y` at +0x14, stored at 118) that
 *      VC6 caches PER REGION: pass 2 reads memory (231/240); pass 3 puts
 *      scale_x in EDI and scale_y in EBX (455/468) -- different registers,
 *      which is the proof that these are separate webs; the pass-4 loop
 *      loads scale_x into EBX at the LOOP HEADER on BOTH entry paths
 *      (517 in the preheader, 519 at the latch) because ebx is clobbered
 *      inside the body on every path; and every later use reads memory again
 *      (570 mark block, 641 roads, 826/923 track, 988 single sprite,
 *      1092/1099 ILF).  Ours enregisters scale_x in ebx and scale_y in ebp
 *      as ONE web each, live from index 457 (pass 3) to the end of the
 *      pass-4 loop.
 *    - The original SPILLS `def` (580 `mov [esp+0x50],ecx`, reloaded 831) and
 *      keeps it in ecx, a scratch register; ours keeps it in esi for the
 *      whole loop body.  That is where our fourth callee-saved register goes.
 *    - The original hoists `bpos` (the packed dword at frame +0x3c) into EDI
 *      at 669, BEFORE the flags test, and both arms use it; ours has no free
 *      register there and emits the load TWICE, once inside each arm
 *      (667 and 677).  A visible symptom of the same pressure difference.
 *    - `chain` lives in EAX with a spill home at +0x44: the preheader loads
 *      only ebx and jumps INTO the loop header past the redundant
 *      `mov eax,[esp+0x44]` (517/518 vs 519/520), and the body needs
 *      `mov esi,eax` (522) to feed the `rep movsd`.  Ours loads chain
 *      straight into esi at the header (517) and needs no copy -- one
 *      instruction fewer and no peeled preheader.  A `while` loop instead of
 *      the `for` was measured: strict 843 but region total 475 and
 *      offset-blind 404 (from 369), i.e. worse; NOT applied.
 *    - The original does NOT cache the `LineTo` import: `call dword ptr
 *      [0x4ab0c0]` at BOTH 884 and 933.  Ours has a spare callee-saved
 *      register in the track arm and emits `mov esi,[__imp_LineTo]` +
 *      `call esi` twice.  Another symptom, not a source difference.
 *    - The ILF loop carries `desc->sprite + 8` as an ADDRESS (1007
 *      `add eax,8`; 1017 `mov eax,[eax]` at the body top; recomputed
 *      1110..1112 at the latch) and so loads `*(sprite+8)` TWICE per
 *      iteration -- once for the `while` condition (1114) and once at the
 *      body top (1017).  Ours CSEs the two into one and carries `ilf` itself
 *      (760 and latch 865, both `mov eax,[eax+8]`), one instruction fewer per
 *      iteration.  No spelling found that keeps the address without the
 *      value; recorded as the next thing to try.
 *
 * ===========================================================================
 * ROUND w9 (strict 847 -> 844, and every structural measure with it: region
 * total 498 -> 465, register-blind 451 -> 407, offset-blind 403 -> 369,
 * register+offset-blind 266 -> 245, mnemonic-only 236 -> 222; frame 0xfc ->
 * 0xf4 against the original's 0xf8, i.e. 4 over becomes 4 under; bytes 4223
 * -> 4205 of 4225).  Six reconstruction errors, all found by reading the
 * original instruction by instruction with a CORRECT esp-tracking listing.
 * ===========================================================================
 *
 * *** READ THIS FIRST: every [esp+N] in this function must be converted to a
 * FRAME offset before it means anything. ***  Round w8's slot map was wrong
 * because its esp simulation drifted (it linearly accumulated pushes across
 * branch joins).  scratchpad/w9renderview/esp.py and slots.py print an
 * annotated listing and a frame map with esp tracked properly: push/pop,
 * `add/sub esp,N`, __stdcall callees (`call dword ptr [..]`) popping their
 * own arguments, and -- the fix that mattered -- esp RESET to the frame base
 * at every branch target.  Four of the six errors below were invisible
 * without it, and several offsets quoted in the round-w8 section above are
 * wrong (the ILF loop's Pos is at +0xa0, not +0xc4; the roads Pos is at
 * +0xd4).
 *
 * 1. *** PASS 1 BUILDS ITS COORDINATE IN THE FUNCTION-LEVEL `tile`, NOT IN
 *    AN INLINED TileBoundsAt TEMPORARY. ***  The original's pass-1
 *    GetTileBounds is `lea eax,[esp+F18]` (&tb) / `lea ecx,[esp+F2c]` at
 *    215/217, and F0x2c is the SAME slot pass 2 (374/375/382), pass 3
 *    (446/449) and the TRACK arm (719/725/746) use.  A pooled TileBoundsAt
 *    temp would have had its own slot.  Worth 6 structural slots and 4
 *    strict.
 * 2. *** THE MARK GRID IS A 32x32 TWO-DIMENSIONAL ARRAY. ***  Declaring
 *    `g_map_marks[32][32]` and writing `g_map_marks[c.base.y >> 3]
 *    [c.base.x >> 3].x` reproduces the original's index block EXACTLY --
 *    `shr esi,3 / shl esi,5 / shr edi,3 / add esi,edi` (554-560), the byte
 *    `and al,0xf8` at 540, the late `shl esi,3` at 571 and the two based
 *    stores `[esi+0x8119c0]` / `[esi+0x8119c4]`.  The flat form
 *    `i = ((y>>3)<<5) + (x>>3); g_map_marks[i].x` reassociates the index into
 *    `(y & ~7) * 4` (it reuses the AND the TileBoundsAt argument computed)
 *    and folds the scale into `[esi*8+base]`.  NOTE THE TWO SEPARATE LEVERS
 *    THIS DECOMPOSES INTO, both new: **`(y>>3) * 32` and `((y>>3) << 5)` are
 *    NOT the same to VC6** -- the multiply blocks the `y & ~7` reuse, the
 *    shift allows it (worth 6 structural on its own) -- and a byte offset
 *    (an index pre-scaled by the element size) buys the separate `shl`
 *    (another 4).  The 2D array gets both and is the natural spelling.
 * 3. *** THERE IS NO `cls` LOCAL. ***  At 831 the TRACK arm does
 *    `mov ecx,[esp+F50] / cmp [ecx+0xc4],eax` -- it RELOADS `def` from its
 *    spill home and re-reads `def->ctx` for the `cls != e_track_hp` test,
 *    across three intervening calls.  A named `cls` would have been spilled
 *    and reloaded from its own slot.  Spelling `def->ctx` at all seven sites
 *    is byte-identical to spelling it only at the e_track_hp test (VC6 CSEs
 *    the five call-free compares into one load either way), and it also
 *    restores the `mov [esp+F50],ecx` spill of `def` at 580 that we were
 *    missing.  Worth 9 structural, 14 offset-blind.
 * 4. *** PASS 3 WRITES ITS BOUNDS INTO A DIFFERENT TileBounds. ***  Passes
 *    1, 2 and 4 pass &F0x18; pass 3 passes &F0xa0 (445, read back at
 *    453/454, written back at 460/465).  A block-scope `TileBounds tb3;`
 *    around pass 3 reproduces it (worth 23 on register-blind).  In the
 *    original that slot is shared with the ILF loop's Pos (1029/1032) -- a
 *    disjoint-lifetime pool pairing we do NOT reproduce (ours pairs tb3 with
 *    the ILF `lo.y` spill instead), which is where the last of the frame
 *    discrepancy lives.
 * 5. *** PASS 2's GetTileBounds Pos IS THE TRACK ARM's, AND `off` IS THE
 *    PASS-1/3 ONE. ***  There are exactly two function-level Pos objects:
 *    A at F0x2c = {pass-1 GetTileBounds arg, pass-2 HalfPos out-param,
 *    pass-3 GetTileBounds arg, TRACK arm's tile coordinate} and B at F0x48 =
 *    {pass-2 GetTileBounds arg (360/365/366), TRACK arm's GetTileBounds arg
 *    (748/750/753, 788/792, 892/893)}.  We had pass 2 using A for the call
 *    and a second local for HalfPos, and a BLOCK-SCOPE `tp` for the TRACK
 *    calls -- so B was pooled instead of named.  Making B a function-level
 *    `gtb` used at both sites, and A (`tile`) the HalfPos out-param, is
 *    worth 2 strict, 2 register-blind and EIGHT FRAME BYTES, and it brings
 *    the top-of-frame pool to exactly four 8-byte Pos slots as the original
 *    has (+0xd4 roads, +0xdc single sprite, +0xe4 mark grid, +0xec p1).
 * 6. *** `saved_ox` IS READ BEFORE `saved_oy` *** (141-145 read +0x20 and
 *    store it, then 146-150 read +0x22).  Round w7 had chosen the other
 *    order because it scored better on strict; it is 1 worse on strict and
 *    better on all four blind measures, which is the honest direction.
 *    Same class: the `clip` fill is stored left, RIGHT, top, bottom
 *    (58/60/61/62 and 439/440/441/442), not in field order.
 * ALSO APPLIED, as a lever rather than a reconstruction fact: the ILF
 *    loop's `lo` is two plain `int`s, not a `Pos`.  With a `Pos` the pool
 *    pairs tb3 with `lo.y` and the frame grows 8; with ints it does not.
 *    Neither spelling is observable in the original (the original keeps
 *    lo.x in ebp and spills only lo.y, at F0x98).
 *
 * MEASURED AND REJECTED THIS ROUND, though each LOWERS the strict count --
 * they contradict the frame evidence, which says the original pools a Pos at
 * all four of the loop's TileBoundsAt sites:
 *    - the ILF loop or the single-sprite site building its coordinate in
 *      `gtb` instead of a pooled temp (both: strict 844 -> 840,
 *      register-blind 407 -> 399, but the frame collapses 0xf4 -> 0xec);
 *    - the roads site filling its Pos before GetRoadRecord, which is what
 *      the original emits (599/600 before the call at 601) but costs 6
 *      structural slots and 3 on both blind measures -- so the original's
 *      early stores are VC6 hoisting a not-yet-escaped local, not source
 *      order;
 *    - moving the ILF loop's TileBoundsAt above the `lo` reads (region total
 *      480 -> 471 and big-displacement 303 -> 299, the only thing ever
 *      measured to move `big`, but offset-blind 386 -> 416 and it is against
 *      the emitted order at 1022/1024/1027).
 *
 * ===========================================================================
 * THE LAYOUT BIT: THE MECHANISM IS NOW NAMED, WITH THREE IN-TREE PROOFS
 * ===========================================================================
 * 303 of the 465 remaining structural slots are still the one displacement
 * (the `if (def->flags & 0x400)` else arm).  Round w9's contribution is to
 * find what source shape produces the original's layout, by SCANNING ALL
 * 1544 AUDIT-EXACT FUNCTIONS for the same signature -- a conditional at i
 * whose target block K ends in an unconditional jmp BACKWARD to j with
 * i < j < K, where j is also reached by fall-through
 * (scratchpad/w9renderview/scan.py).  Eleven hits; four are real:
 *
 *   UpdateMapDrag   0x452030  cond@57  arm[75..78] jmp->67   <- THE MODEL
 *   InitSavedGameScreen 0x48d4b0 cond@119 arm[130..135] jmp->125
 *   KillAllSamplesFromSource 0x496b80 cond@27 arm[53..57] jmp->45
 *   LoadObjectLibrary 0x480f00 cond@39 arm[131..137] jmp->63
 *
 * *** THE RULE: A BLOCK THAT ENDS IN AN UNCONDITIONAL JMP IS EXILED PAST THE
 * FALL-THROUGH TRACE; A BLOCK THAT FALLS INTO ITS SUCCESSOR IS LAID OUT IN
 * PLACE. ***  Every one of the eleven hits obeys it, and so does our own
 * TRACK arm.  So the question "why is the original's sd fill exiled" reduces
 * to "why does it end in a jmp", and the answer in three of the four proofs
 * is CROSS-JUMPING: the source writes BOTH arms out in full with a common
 * tail, VC6 merges the common suffix, glues the merged copy to the THEN arm
 * (which falls into it) and rewrites the ELSE arm's copy into a backward
 * `jmp` into the MIDDLE of the then arm's straight-line code.  UpdateMapDrag
 * is the cleanest: `if (g_drag_class) { step_w=..; step_h=..; sel_x0=minx;
 * sel_y0=miny; sel_x1=maxx; sel_y1=maxy; } else { step_w=1; step_h=1;
 * <the same four sel_ stores>; }` -- the four stores are merged at 67-73 and
 * the else arm becomes three instructions plus `jmp 67`.  That is EXACTLY
 * the shape of our 707-714 + `jmp 680`.
 * WHY IT DOES NOT WORK HERE, PRECISELY.  The merged suffix has to be
 * IDENTICAL MACHINE CODE in both arms.  Duplicating `if (desc == 0)
 * continue;` into both arms does emit the test in the sd arm as well (VC6
 * does NOT fold it on `&sd` -- the round-w8 note's claim that it does is
 * wrong; what folds is only the case where the tail's later uses let VC6
 * delete it), but it allocates a SECOND `lea` and puts the test in a
 * different register: the cb arm gets `mov edi,eax / test edi,edi / je`, the
 * sd arm `lea eax,[esp+0xc4] / test eax,eax / lea edi,[esp+0xc4] / je`.
 * Two registers, no merge.  Adding the sprite test as well diverges further
 * (VC6 forwards the just-stored `sd.sprite` into it).  In the original BOTH
 * arms use ebx.  **So the layout bit and `desc`'s register are the same
 * problem: get `desc` into ebx (the original keeps ebx = scale_x early in
 * the loop body and ebx = desc over the whole sprite path, reading scale_x
 * back from the stack at 641 and 988; we keep ebx = scale_x throughout and
 * put desc in edi), and the one-statement duplication should merge.**
 * ALSO RULED OUT THIS ROUND (on top of round w8's eleven spellings):
 *   - `goto plain;` with `plain:` at the very END of the loop body, and the
 *     same with `plain:` after the whole function's epilogue and a `goto`
 *     back INTO the loop: the FRONT END normalises both into the plain
 *     if/else and INVERTS it, laying the sd fill out first at 660 (the
 *     mirror).  A source `goto` survives only when it crosses a LOOP
 *     boundary, which is why LoadObjectLibrary's does;
 *   - the LoadObjectLibrary shape written directly -- `if (!(flags & 0x400))
 *     { fill; desc = &sd; goto have_desc; }` with the cb path falling
 *     through to `have_desc:` -- three variants, all the mirror;
 *   - two separate `if`s on the same flag (VC6 does not thread them: +216
 *     strict), and the same through a named `int has_cb` flag (byte count
 *     lands exactly on 4225 but the layout does not move);
 *   - `do { ... break; ... } while (0)` round the selection, and hoisting
 *     the whole sd fill above the `if` with no else arm (VC6 does not sink
 *     partially-dead stores: the fill is emitted before the test).
 *
 * ===========================================================================
 * ROUND w8 (894 -> 847 strict, and EVERY structural measure improved with it:
 * region total 594 -> 498, register+offset-blind 311 -> 266, offset-blind
 * 468 -> 403, mnemonic-only 280 -> 236, bytes 4199 -> 4223 (original 4225)).
 * Five reconstruction errors were found by reading the original instruction
 * by instruction; no codegen-lever grinding was involved in any of them.
 * ===========================================================================
 *
 * 1. *** THE SCROLL OFFSET IS ADDED TO `tb` BEFORE g_fm_cw/g_fm_ch ARE SET,
 *    NOT INSIDE FullMapX/FullMapY. ***  Worth 77 structural slots and 53
 *    strict on its own -- by far the biggest single win of the round.
 *    `tb` is ADDRESS-TAKEN (it is GetTileBounds' out parameter), so a store
 *    to a GLOBAL may alias it and VC6 may not move a `tb.left` update across
 *    one.  At the roads site the original emits
 *        617 sub ecx,0x33          tb.left += HalfOffset(-0x66)
 *        619 add ecx,edx           tb.left += g_scroll_x >> 8
 *        621 sub eax,0x2c          tb.top  += HalfOffset(-0x58)
 *        624 add eax,edx           tb.top  += g_scroll_y >> 8
 *        622/626 the two stores back into tb
 *        628-633 g_fm_cw = s_lights->w; g_fm_ch = s_lights->h;
 *        629/636 the projection multiplies
 *    -- ONE store per field, both adjustments folded in, and the g_fm_*
 *    stores strictly after them.  With the scroll add living inside
 *    FullMapX/FullMapY (i.e. at the PrintScaledSprite argument, after the
 *    g_fm_* stores) VC6 cannot hoist it past those stores, so it emitted a
 *    SECOND load/store pair of tb.left and tb.top at every one of the four
 *    sprite sites.  The same [adjust, store, g_fm_*, project] order holds at
 *    the single-sprite tail (962-974 vs 975-981 vs 982-1000) and in the ILF
 *    loop (1068-1082 vs 1083-1086 vs 1096-1106).
 *    FIXED BY: FullMapX/FullMapY are now PURE (no mutation), and a
 *    `static __inline void FullMapScroll(TileBounds*)` is called explicitly
 *    at each of the nine projection sites, immediately after the HalfOffset
 *    adjustments and before the g_fm_cw/g_fm_ch stores.
 *    Measured alternatives: folding the scroll into the same statement
 *    (`tb.left += HalfOffset(sd.dx) + (g_scroll_x >> 8);`) is 12 worse;
 *    putting FullMapScroll BEFORE the HalfOffset adjustments is 10 better on
 *    the region total and 26 better on the big-block measure but 13 worse on
 *    mnemonic-only and 13 worse on strict -- and the disassembly says
 *    HalfOffset first (964/966 before 969/972), so it is NOT applied.
 *
 * 2. *** THE TRACK ARM'S THREE GetTileBounds CALLS SHARE ONE NAMED `Pos`,
 *    AND THE `if (link)` SITE DOES NOT MUTATE `tile`. ***  All three write
 *    the SAME slot, frame +0x48 (which is low in the frame -- the named-local
 *    region -- not in the top-of-frame inline-temporary pool the other four
 *    Pos sites use), and all three COPY their coordinates in:
 *        746 mov eax,[esp+0x2c] / 747 mov ecx,[esp+0x30]   (tile.x, tile.y)
 *        748 mov [esp+0x48],eax / 753 mov [esp+0x54],ecx   (the copy)
 *        785 mov eax,[esp+0xec] / 791 mov ecx,[esp+0xf0]   (p1.x, p1.y)
 *        788/795 into the same +0x48 slot
 *        888 mov edx,[esp+0x2c] / 890 add edx,-0xa / 892 mov [esp+0x48],edx
 *        889 mov eax,[esp+0x30] / 896 mov [esp+0x54],eax
 *    The third one reads `tile.x` and writes `tile.x - 0xa` into the COPY, so
 *    the older note's "the original really does mutate `tile.x` in place" is
 *    WRONG and is hereby retracted.  Reverting this edit costs 70 structural
 *    slots (498 -> 568), so it is the second-biggest item of the round.
 *
 * 3. *** MoveToEx TAKES THE p1 PROJECTION AND LineTo THE `tile` PROJECTION --
 *    OURS HAD THEM SWAPPED. ***  esi/edi carry the FIRST (tile) projection
 *    (784 `mov esi,eax`, 790 `mov edi,ecx`) and are what the stick sprite
 *    uses (851 `mov edx,esi`, 855 `push edi`) AND what LineTo is given
 *    (881 `push edi` / 882 `push esi` / 884 call).  ebp and +0x74 carry the
 *    SECOND (p1) projection (821 `mov ebp,eax`, 830 `mov [esp+0x74],ecx`)
 *    and are what MoveToEx is given (872 `mov edx,[esp+0x74]` / 877 `push
 *    ebp` / 879 call).  So the original draws the segment from the far end
 *    back to the base and then on to the neighbour.  Worth 4.
 *
 * 4. *** PASS 4 MAKES ONE 20-BYTE CELL COPY, NOT TWO. ***  The original has
 *    exactly THREE `rep movsd` in the whole function (indices 202, 350, 526),
 *    one per pass; we had four, because `c = *chain` and the by-value
 *    `CellMarkTest(*chain)` each made one.  `CellMarkTest(c)` collapses them.
 *    NOTE THE COST, because it is a real regression that has to be paid back:
 *    the original's two Cell slots are +0x54 (passes 1 and 2, coalesced) and
 *    +0xf4 (pass 4, 20 bytes at the very top of the frame), i.e. the pass-4
 *    cell does NOT share with the pass-1/2 one; with a single copy ours
 *    shares at +0x60 and the frame loses 20 bytes.  Every attempt to keep
 *    both properties failed -- see RULED OUT below.
 *
 * 5. *** THE ROADS AND SINGLE-SPRITE SITES GET THEIR POOLED `Pos` BACK. ***
 *    The two "FRAME BALLAST" demotions round w7 added (building the Pos in
 *    the function-level `tile`) are gone; the original pools a Pos at both
 *    (`lea eax,[esp+0xdc]` at 690 for the single sprite, the +0xd4 slot at
 *    599/610 for roads).  They only existed to buy back the 20 bytes item 4
 *    now frees.  Reverting either costs 4-7 on register-blind.
 *
 * FRAME: 0xfc, and the original is 0xf8 -- FOUR bytes over, and that is the
 * one property round w7 had right and this round does not.  The arithmetic:
 * w7's 0xf8 = correct-shape 0x110 minus 20 (the duplicate Cell copy that
 * should not exist) minus 2x8 (the two Pos demotions that should not exist);
 * this round removes all three wrongs and lands 4 over.  The missing 4-byte
 * saving was NOT found: dropping the `cls` local entirely (spelling
 * `def->ctx` at all five comparisons, the cb call and the e_track_hp test)
 * leaves the frame at 0xf8 unchanged and trades register-blind 475 -> 462
 * for offset-blind 468 -> 541, so it is not it either.
 *
 * ===========================================================================
 * THE BLOCK LAYOUT: NOW REDUCED TO ONE BINARY DECISION.  READ THIS FIRST.
 * ===========================================================================
 * 303 of the remaining 498 structural slots are still the one displacement,
 * in three regions: orig[953:1117] (164), orig[714] vs ours[774:866] (92) and
 * orig[710:711] vs ours[724:771] (47).  Round w8's contribution is to explain
 * the layout completely.
 *
 * *** VC6 LAYS BLOCKS OUT BY TRACE, WITH A LIFO PENDING STACK. ***  Walk the
 * fall-through chain from the current block; every conditional's TARGET is
 * pushed on a stack; when the trace ends (an unconditional jmp or a `continue`
 * jump) pop the stack and start the next trace there.  Label the chain-loop
 * blocks
 *      B0  the `test dh,4` block          CB  the callback arm
 *      J   the desc/sprite/ILF tests      SS1 the single-sprite head
 *      NEG the HalfOffset negative arm    SD  the sd fill
 *      TRK the track arm                  SS2/SSJ the single-sprite tail
 *      ILF the ILF loop                   L   the loop latch (`continue`)
 * ORIGINAL: trace B0 -> CB -> J -> SS1 -> NEG (jmp), pushing TRK(660),
 * SD(671), ILF(687), SS2(702); pop SD, pop TRK, pop SS2 (-> SSJ), pop ILF,
 * then L.  That is exactly 707, 715, 954, 1006, 1117.
 * OURS: trace B0 -> CB, which ENDS with `jmp J`; pop SD; SD falls through to
 * J -> SS1 -> NEG; pop SS2 -> SSJ; pop ILF; pop TRK; then L.  That is exactly
 * what we emit.
 * *** SO THE ENTIRE 303-SLOT RESIDUAL IS ONE BIT: WHICH ARM OF
 * `if (def->flags & 0x400)` FALLS THROUGH INTO THE JOIN. ***  In the original
 * the THEN arm (CB) falls through and the ELSE arm (SD) is sunk past J, SS1
 * and NEG and jumps BACKWARD to J at index 680 -- which is the middle of the
 * block [679 mov ebx,eax][680 test ebx,ebx].  In ours the else arm falls
 * through and the then arm jumps forward.  Nothing else about the layout is
 * wrong: get that bit and TRK, SS2, ILF and L all follow from the model.
 *
 * ===========================================================================
 * RULED OUT THIS ROUND (all measured at this round's baselines)
 * ===========================================================================
 * ON THE LAYOUT BIT.  Eleven spellings of the descriptor selection, none of
 * which moves the sd fill after the join (the layout signature to watch is
 * scratchpad/w8renderview/lay.py, which prints the index of the sd fill, the
 * join, and the track arm's first/last instruction):
 *   - `desc = &sd;` hoisted above the `if` with the else arm doing only the
 *     three field stores;  - the arms inverted (sd as the then arm);
 *   - an `else if (def != 0) {...} else { }` empty trailing arm;
 *   - `if (cb != 0) desc = cb(...); else continue;`;
 *   - `if (desc == 0) continue;` duplicated into BOTH arms;
 *   - a two-case `switch (def->flags & 0x400)`;
 *   - `if (!(flags & 0x400)) goto plain; ... plain: ...; goto have_desc;`
 *     with the label OUTSIDE the if body (VC6 inverts the branch polarity and
 *     lays the sd fill out FIRST, at index 667 -- the mirror image);
 *   - the same goto form with `have_desc:` INSIDE the if body and the sd fill
 *     textually after the whole `if` (byte-identical to the committed file);
 *   - the same again spelled as if/else with `goto have_desc` in the else
 *     (byte-identical);
 *   - the sd fill through a `static __inline SpriteDesc* FillPlainDesc(...)`
 *     (byte-identical);
 *   - a `?:` with a comma expression for the fill (+2 structural, no move).
 *   VC6 emits [test][arm1][jmp join][arm2][join] for EVERY one of them: the
 *   arm that falls through is always the one written first, and the join
 *   always follows arm2.  Source shape cannot express the original's layout.
 * THE TAIL-DUPLICATION ROUTE IS REAL BUT DOES NOT MERGE.  Writing the WHOLE
 *   remainder of the loop body textually in both arms DOES sink the sd fill
 *   past the join (signature sd=876 > join=690, and the big-displacement
 *   measure falls 320 -> 173), but VC6's cross-jump does not merge the two
 *   copies: the body grows from 1154 to 1339 instructions.  The reason is
 *   visible in the output -- in the sd arm VC6 forwards the just-stored
 *   `sd.sprite` into the `desc->sprite == 0` test (`test esi,esi`) and folds
 *   `desc == 0` on `&sd`, so the two copies are not identical machine code.
 *   A shorter duplication (down to a shared `goto ilf_loop;`) sinks the block
 *   too (sd=757 > join=682) and still does not merge (1233 instructions).
 *   The original's join at 680 is a genuinely SHARED block, not a merge
 *   artefact.  Whoever picks this up: the bit is decided before layout, so it
 *   has to come from register allocation or from a CFG shape not yet tried.
 * ON THE CELL COPY.  Two by-value expansions of `*chain` in ADJACENT
 *   statements DO share one pool temporary (adding `CellObj(*chain)` next to
 *   `CellMarkTest(*chain)` keeps the copy count at four), but six expansions
 *   spread over the loop body do NOT: routing every pass-4 cell read through
 *   `CellObj`/`CellBaseX`/`CellBaseY` by-value helpers and deleting `c`
 *   entirely gives NINE `rep movsd` and a 0x15c frame.  Reading `chain->obj`
 *   / `chain->base.x` directly while keeping `CellMarkTest(*chain)` gives the
 *   right copy count (three) but the pool temp then coalesces with the
 *   pass-1/2 cell (frame 0xe4) and costs 4 structural slots and 10 strict.
 * ALSO MEASURED AND NOT APPLIED: `g_fm_cw` before `g_fm_ch` in the ILF loop
 *   (the original's emitted order, 1084 before 1086) is identical on every
 *   structural measure and 4 bytes worse; three spellings of the ILF loop
 *   (`while` with the count expression in the condition, a `for`, and a
 *   second textual copy of the `desc->sprite + 8` read for the `sprites[i]`
 *   load) are all BYTE-IDENTICAL, so the original's loop-top `mov eax,[eax]`
 *   -- it keeps `desc->sprite + 8` as a loop-carried ADDRESS and re-loads the
 *   pointer through it every iteration, where we forward the value from the
 *   bottom test -- is not reachable by respelling the loop; FullMapScroll
 *   writing `top` before `left` costs 29; moving FullMapScroll before the
 *   HalfOffset pair at the roads site costs 13.
 *
 * Tooling added this round, in scratchpad/w8renderview/:
 *   m.py     one line per variant: structural region total, the >=30-slot
 *            "big" part of it, strict, all four blind measures, byte count,
 *            FRAME SIZE, first divergence, ESCAPES and the callee-saved push
 *            indices -- the single command to rank anything;
 *   lay.py   the block-layout signature (sd fill / join / track arm indices)
 *            for the original and any variant, which is what reduced the
 *            residual to the one bit above;
 *   cfg.py / cfgc.py  predecessor+successor maps with instruction indices for
 *            the original and for a compiled variant -- this is how the trace
 *            model was derived;
 *   dumpo.py the original with index, address and resolved branch targets;
 *   mk.py    named textual edits and their combinations;
 *   rvgeo.py / rvsc.py / rvt.py  RenderView geometry-order, scope and
 *            spelling variants;
 *   fm2.py   frame slot map (note: laneG/fm.py's esp simulation does not
 *            model __stdcall callees popping their own arguments, so every
 *            slot it reports after the first COM/import call is wrong; the
 *            offsets in this note were read off the disassembly by hand).
 *
 * ===========================================================================
 * ROUND w7 (strict 881 -> 894, but EVERY structural measure improved and the
 * frame is still exactly 0xf8).  Read this section first; the older sections
 * below it are still accurate about the frame and the statement orders.
 * ===========================================================================
 * Measures, all from scratchpad/w7renderview/w7.py (score + permutation-aware
 * ranking + LCS region report; run.py ranks a list of variants):
 *                     before   after
 *   strict mismatch     881      894      <- the ONE number that got worse
 *   register-blind      524      475
 *   offset-blind        485      468
 *   register+offset     316      311
 *   mnemonic-only       284      280
 *   bytes              4201     4199      (original 4225)
 *   frame              0xf8     0xf8      (original 0xf8)
 * The strict count is meaningless here: 164 instructions of the original are
 * displaced relative to ours (see THE BLOCK LAYOUT below), so index-for-index
 * comparison is measuring an offset, not a set of wrong choices.  Rank with
 * the region report.
 *
 * 1. THERE IS NO NAMED `spr` LOCAL IN THE CHAIN LOOP.  The original re-reads
 *    `desc->sprite` through ebx at index 975 (`mov edx,[ebx]`, for the
 *    g_fm_cw/g_fm_ch pair) AND AGAIN at 999 (`mov eax,[ebx]`, for the
 *    PrintScaledSprite argument), 24 instructions apart with no call between
 *    them.  A named `Sprite* spr` is not aliased by the intervening stores to
 *    the g_fm_* globals, so VC6 would have loaded it once at 682 and kept it;
 *    two reloads prove the source spells `desc->sprite` at every use.  Worth
 *    15 structurally (small-region residual 290 -> 275) and, more to the
 *    point, it stops `spr` taking a callee-saved register: the original keeps
 *    ebx = desc live across the whole sprite path and reads scale_x/scale_y
 *    from the stack inside the ILF loop (index 1092 `mov edx,[esp+0x3c]`),
 *    while we were keeping scale_x/scale_y in ebx/ebp and spilling desc --
 *    which is why our ILF loop exit needs the `mov ebp,ebx` / `mov ebx,[..]`
 *    shuffle the original does not have.
 *
 * 2. THE SINGLE-SPRITE SITE LOADS BOTH desc->dx AND desc->dy BEFORE THE FIRST
 *    SIGN TEST (original 698 `mov eax,[ebx+4]` / 699 `mov edx,[ebx+8]`, then
 *    701 `test eax,eax`).  This is RenderView's four-temp HalfOffset lever
 *    (see its note, item 1) and it had never been applied here.  Two temps
 *    are enough at this site -- `gdx`/`gdy` before the two HalfOffset calls;
 *    adding `hdx`/`hdy` as well measures 1 worse.
 *
 * 3. THE ROADS Pos IS NOW FRAME BALLAST.  Item 1 costs an 8-byte spill home
 *    (frame 0xf8 -> 0x100), so a second TileBoundsAt site has to be demoted
 *    to the function-level `tile` to get back to 0xf8.  This is a construct
 *    the original does NOT have and it is commented as such at the site.  The
 *    original pools a Pos at ALL FOUR sites -- `lea` of 0xd4 (roads, index
 *    610), 0xdc (single-sprite, 690), 0xe4 (mark grid, 544) and 0xa0 (ILF,
 *    1029) -- so the fact that we cannot hold 0xf8 without two demotions is
 *    itself evidence that our register allocation is still wrong, not that
 *    the demotion is right.  Which site is demoted does not matter (roads /
 *    ILF / mark all measure within 1).
 *
 * 4. NEW FRAME FACT, not yet reproduced: PASS 3 USES ITS OWN TileBounds.  The
 *    original's perimeter-terrain-object setup writes its GetTileBounds out
 *    parameter to +0xa0 (index 445 `lea eax,[esp+0xa4]` at push depth 1, read
 *    back at 453/454 as [esp+0xac]/[esp+0xb0] at depth 3, `add esp,0xc` at
 *    474 confirming the depth) while every other site in the function uses
 *    the shared `tb` at +0x18.  +0xa0 is also the ILF loop's pooled Pos, so
 *    the pass-3 TileBounds is a POOL TEMPORARY sharing a slot with it, not a
 *    named local: a block-scope `TileBounds tb3;` reproduces the split but
 *    lands in its own 8 bytes (frame 0x100, and 0x108 once the single-sprite
 *    Pos is also pooled).  Whatever helper owns it must own the TileBounds
 *    and take the Pos by pointer, because the Pos it is given is the
 *    function-level `tile` at +0x2c (index 446 `lea ecx,[esp+0x30]`).
 *
 * ===========================================================================
 * THE BLOCK LAYOUT IS THE WHOLE RESIDUAL, AND IT IS NOT A SOURCE-ORDER LEVER
 * ===========================================================================
 * Aligned register+offset-blind, 311 of the 601 differing slots are ONE
 * displacement.  The original lays the chain loop's second half out as
 *   [track test N1][flags&0x400 test N2][cb arm N3][join J][single-sprite
 *    head 688-706][sd fill 707-714][TRACK ARM 715-953][single-sprite tail
 *    954-1005][ILF loop 1006-1116][continue 1117]
 * and we lay it out as
 *   [N1][N2][N3][sd fill][J][single-sprite head+tail][ILF loop][TRACK ARM]
 *   [continue].
 * Two blocks move: the sd fill goes from beside the join to after the
 * single-sprite head, and the track arm goes from last-before-continue to the
 * middle.  Equivalently, the original's `jge 0x457582` at index 702 jumps
 * over the ENTIRE track arm to the `v >= 0` half of the first HalfOffset --
 * which is also where the original's extra 24 bytes live (six long jcc/jmp
 * encodings that our short forms do not need).
 * MEASURED THIS ROUND, all BYTE-IDENTICAL to the committed file, i.e. VC6
 * normalises source order completely for these blocks:
 *   - the descriptor selection as `goto build_sd;` with the sd fill written
 *     at the very END of the loop body (this repeats the older note's
 *     experiment and confirms it at the current baseline);
 *   - the track arm behind `goto track_arm;` with its body at the end of the
 *     loop body;
 *   - the track test INVERTED (`cls != e_track && ...`) so the whole sprite
 *     path becomes the then-arm and the track arm is textually last.
 *   - inverting the descriptor selection (sd fill as the then-arm) is NOT
 *     byte-identical but is 2 worse and does not move the layout either.
 * So the layout is a function of the CFG plus register allocation, not of
 * statement order.  The one lead: our ILF loop exit needs a two-instruction
 * register shuffle (`mov ebp,ebx` / `mov ebx,[esp+0x4c]`) before its `jmp` to
 * the loop continue, and the original's ILF loop falls straight through into
 * the continue.  Whoever picks this up: make the ILF loop stop clobbering the
 * chain loop's invariants (item 1 is the first half of that -- get scale_x /
 * scale_y OUT of ebx/ebp and desc INTO one of them) and the block that wins
 * the fall-through into the continue should change with it.
 *
 * ===========================================================================
 * RULED OUT THIS ROUND
 * ===========================================================================
 *   - the four ILF-site HalfOffset shapes.  The original computes them in the
 *     order hlx, hly, hdx, hdy (indices 1035, 1044, 1054, 1061) and adds them
 *     LEFT-ASSOCIATED -- `tb.left = tb.left + hdx + hlx` (1069 `add ecx,eax`
 *     then 1073 `add ecx,edx`), NOT `tb.left += hdx + hlx`, which pre-sums
 *     the pair.  THE ASSOCIATION ALONE IS A NO-OP: written either way VC6
 *     emits a byte-identical object, so do not spend time on it.  What does
 *     move is naming hlx/hly in temps computed BEFORE the desc->dx/dy pair
 *     (the original's order); on this file that measures small-region
 *     residual 274 -> 270 and register-blind 475 -> 468 with the frame still
 *     0xf8, but strict 894 -> 1056, so it is not applied.  Take it once the
 *     block layout is fixed and the strict count means something again;
 *   - pass 3 with a block-scope TileBounds (frame 0x100, struct +2);
 *   - all four TileBoundsAt sites pooled at once, with and without the pass-3
 *     TileBounds: frames 0x100 / 0x108 / 0x110.  The 0x110 variant is the
 *     only body that reaches the original's 4225 BYTES exactly, which is a
 *     coincidence of longer displacements, not a match;
 *   - moving the single-sprite site's demotion to roads / ILF / mark: all
 *     within 1 of each other, as the older note said.
 *
 * Tooling added this round, in scratchpad/w7renderview/:
 *   w7.py    score + best-callee-saved-permutation ranking (laneK's permrank
 *            metric, ported to this file's harness) + LCS region report;
 *   run.py   rank a list of variant .c files by structural region total,
 *            splitting the residual into "big" (>=30-slot displacements) and
 *            "small" (everything else) -- the small column is the one that
 *            responds to source changes while the layout is wrong;
 *   dumpc.py our compiled body with indices and addresses, for reading the
 *            block layout directly.
 *
 * audit.py: 1161/1161 instructions, 4199B vs 4225B, no ESCAPES, mismatch=894.
 * (Historic: 1064 at the start of the round that wrote the sections below.)
 * (The previous revision of this body was 1079 instructions -- 82 short -- and
 * mismatched 1124; it was a semantic sketch, not a structural one.)
 *
 * WHAT WAS MISSING, and is now here: (a) the per-arm RenderBlock in pass 1 (a
 * single joined call collapses four arms into one, ~14 instructions); (b) the
 * `- g_fm_oy` term and the shift-vs-divide split in g_fm_cy; (c) the x87
 * integer-operand form of g_fm_cw/g_fm_ch; (d) the SpriteDesc fills in the
 * roads arm and at the end of the coaster arm (six dead stores the original
 * really emits); (e) GetTrackSegment's fifth out-parameter `link` and the
 * second LineTo it gates; (f) the stick's half-width shift on X; (g) the
 * ILF loop re-reading its bound through desc->sprite each iteration; (h) six
 * distinct Pos temporaries -- one per GetTileBounds site -- which is what the
 * inlined TileBoundsAt helper buys and what most of the frame difference was.
 *
 * FIXED THIS ROUND (1064 -> 893 mismatches, instruction count still exactly
 * 1161/1161, no ESCAPES):
 *   1. `saved_ox`/`saved_oy` are **int**, not `unsigned short`.  The original
 *      emits `xor ecx,ecx / mov cx,[eax+0x20] / mov DWORD [esp+0x94],ecx`
 *      (indices 142-150): a zero-extending load into a 4-byte spill home, not
 *      a 2-byte store.  Worth 151 mismatches on its own -- the single biggest
 *      lever found on this function.  Whenever a `u16` field is stashed and
 *      restored, check the width of the STORE before believing the local is
 *      also 16 bits.
 *   2. The TRACK arm reads the CELL COPY's base (`c.base.x/.y`), not the
 *      separate 2-byte `bpos`.  Original index 715 is
 *      `mov eax,[esp+0xf8] / and eax,0xff` -- an unaligned dword read of the
 *      Cell copy at frame+0xe8/0xe9 -- while the ROADS arm (index 590/592) and
 *      the sprite/ILF arm (index 688) read the packed `bpos` at frame+0x2c.
 *      Both spellings exist in the original and they are not interchangeable.
 *   3. `p1`, `h0`, `h1` and `link` are BLOCK-SCOPE locals of the track arm,
 *      not function-level ones (worth 4).
 *   4. The mark grid is written with plain array indexing,
 *      `i = ((c.base.y>>3)<<5) + (c.base.x>>3); g_map_marks[i].x = ...`.
 *      The original emits `shr/shl 5/shr/add` then ONE `shl esi,3` and two
 *      `[esi + 0x8119c0]` / `[esi + 0x8119c4]` stores.  Spelling it as
 *      `*(int*)((char*)g_map_marks + i)` with `i` pre-scaled by 8 lets VC6
 *      fold the scale into an `lea` and costs 16 (worth 16).
 *
 * FIRST DIVERGING INDEX: 0 -- `sub esp,0xf8` vs `sub esp,0xec`.  The frame is
 * still 12 bytes short and that is the whole remaining story: the block
 * STRUCTURE, the branch senses, the split prologue (push ebp at index 1,
 * push ebx/esi/edi at 42-44 behind the `g_fullmap_busy` guard) and the call
 * sequence are all reproduced -- all 70 calls, in order -- and a
 * register/offset-NORMALISED diff now scores 847 of 1161 identical.  What is
 * left is frame slot assignment plus the register renaming it forces.
 *
 * THE ORIGINAL'S FRAME, MEASURED (offsets relative to esp immediately after
 * `sub esp,0xf8`; the four callee-saves put canonical esp at base-16, so a
 * disassembly `[esp+N]` outside a call-argument run is base+N-16):
 *
 *     +0x00  h0 / fsy (shared)        +0xa8  link
 *     +0x08  tb (16B, passes 1-3)     +0xac  sd (SpriteDesc, 24B)
 *     +0x18  fsx                      +0xc4  Pos: roads-arm TileBoundsAt
 *     +0x1c  tile / off (8B, shared)  +0xcc  Pos: sprite-draw TileBoundsAt
 *     +0x24  h1                       +0xd4  Pos: mark-block TileBoundsAt
 *     +0x2c  bpos (2B) +0x2d          +0xdc  Pos: p1 AND the ILF-loop temp
 *     +0x44  clip (16B) SHARED with            (shared -- disjoint arms)
 *            the pass-1/2 Cell copy    +0xe4  the pass-4 Cell copy (20B)
 *     +0x5c  tw   +0x68  th          -> top of frame 0xf8
 *     +0x70..+0xa4  the ElemID/LoadSprite/pen/saved_ox/saved_oy spill homes
 *
 * OURS is identical from +0xa8 up to +0xdc except that (a) the four Pos slots
 * are handed out in a different ORDER (sprite-draw, mark, ILF, roads) and
 * p1 does NOT share with the ILF temp, so p1 takes +0xe4; and (b) the pass-4
 * Cell copy is colour-shared with the pass-1/2 one down at +0x54 instead of
 * getting its own 20-byte slot at the top.  20 - 8 = 12: that IS the missing
 * frame.  So the ONE thing left to reproduce is: force the pass-4 Cell copy
 * to its own top-of-frame slot and let p1 pool with the ILF loop's Pos.
 *
 * Ruled out this round, all measured, do not repeat:
 *   - splitting the pass-4 Cell into a second named local `cc` (no change:
 *     VC6 colours the two into one slot because their live ranges are
 *     disjoint), with `cc` declared before or after `c`;
 *   - declaring the pass-1/2 Cell and/or the pass-4 Cell block-scope inside
 *     their loops, in any of the four combinations (no change);
 *   - moving `off`/`fsx`/`fsy` into the pass-2 inner block (X 910 -> 1010 and
 *     it flips the zero register from ebp to ebx -- clearly wrong);
 *   - declaring `tb` or `sd` inside the pass-4 loop (X 1021 / compile error
 *     shapes; `tb` gets frame 0xf4 but the body is 3 instructions short);
 *   - a named `Pos` in the ILF loop instead of the inlined TileBoundsAt (no
 *     change -- it still will not pool with p1);
 *   - hoisting `(th+1)>>1` into a local before the RenderBlock call, in three
 *     spellings (X 910 -> 1009); a named `int thv = th;` copy used in the
 *     g_fm_h2 / g_fm_cy expressions, three spellings (X 910 -> 1092).  The
 *     original really does keep `th` in esi across the two __ftol calls
 *     (index 95 `mov esi,[esp+0x84]`, then `inc esi`/`sar esi,1` at 127/131)
 *     and we reload it from memory at 152, but every source form that buys
 *     the register copy costs more elsewhere;
 *   - writing the clip fills in the original's store order (left, right, top,
 *     bottom): no change, VC6 reorders adjacent stores anyway;
 *   - `(unsigned char)(c.base.y & 0xf8)` to buy the original's `and al,0xf8`
 *     (X 910 -> 929);
 *   - giving the `if (link)` LineTo its own Pos (frame 0xf4 but +1
 *     instruction: the original really does mutate `tile.x` in place);
 *   - a goto form (`goto static_desc` / `have_desc:`) for the descriptor
 *     selection, aimed at the original's out-of-line placement of the
 *     `sd`-fill block: no change (X 910), normalised equality 847 -> 846.
 *
 * The other visible layout difference, for whoever picks this up: the
 * original lays the second half of the chain loop out as
 * [single-sprite head 688-706][sd fill 707-714][TRACK ARM 715-953]
 * [HalfOffset join 954-961][single-sprite tail 962-1005][ILF loop 1006+],
 * i.e. the track arm sits BETWEEN a HalfOffset expansion's branch (index 702)
 * and its `v >= 0` join block (index 954).  We emit the track arm after the
 * whole sprite path instead.  That accounts for ~160 instructions of the
 * residual on its own and is worth attacking next, together with the Cell
 * slot above.
 *
 * ===========================================================================
 * NEXT ROUND (mismatch 893 -> 881; THE FRAME IS NOW EXACT, 0xf8 == 0xf8).
 * Two statements in the note above are corrected and the frame question is
 * settled; what is left is one register decision at index 95 and the block
 * layout of the second half of the chain loop.
 * ===========================================================================
 *
 * *** THE LEVER: a by-value struct argument of a `static __inline` helper
 * gets its OWN home in VC6's top-of-frame temporary pool, where a named
 * local -- of any scope -- never can. ***
 * That is what splits the two Cell copies.  VC6 COALESCES the pass-1/2 cell
 * copy with the pass-4 one whenever their live ranges are disjoint, and no
 * naming or scoping changes that; measured, all producing a BYTE-IDENTICAL
 * object: two function-level `Cell c; Cell cc;` in either declaration order;
 * `c` function-level for passes 1/2 with a block-scope `Cell cc;` inside the
 * pass-4 for-body (with or without the copy as its initialiser); and three
 * block-scope cells, one per loop body.  But routing ONE read of the pass-4
 * cell through
 *     static __inline int CellMarkTest(Cell c)   <- BY VALUE, 20 bytes
 * creates an inline-expansion temporary that lands at the top of the frame
 * exactly like the original's, and the frame grows by the 20 bytes the
 * original has: 0xec -> 0x100, with the instruction count still 1161 and the
 * score improving to 888.  (Passing the already-copied local `c` to the same
 * helper instead of `*chain` changes nothing -- 892, frame 0xec -- because
 * then VC6 has no reason to make a second copy.  That pair of measurements
 * is the proof that it is the by-value ARGUMENT, not the helper, that buys
 * the slot.)
 *
 * The remaining 8 bytes come off by NOT giving one of the four TileBoundsAt
 * sites its own pooled `Pos`: the single-sprite draw builds its coordinate
 * in the function-level `tile` instead.  0x100 - 8 = 0xf8, score 884, and it
 * does not matter which of the roads / single-sprite / ILF sites is the one
 * demoted (all three measure 884; the mark-grid site measures 886), which
 * matches the original, where the ROADS Pos and the ILF-loop Pos share one
 * slot.  The original's pool, measured with scratchpad/renderview/w/fmap.py
 * (a correct per-instruction esp simulation -- the older fmap2.py is 12
 * bytes out because it does not know that the __stdcall callees pop their
 * own arguments), is
 *     +0xc4  Pos: the ROADS arm's TileBoundsAt  AND the ILF loop's
 *     +0xcc  Pos: the single-sprite draw
 *     +0xd4  Pos: the mark-grid block
 *     +0xdc  p1 (shares with a set-up spill home, NOT with the ILF temp --
 *            the note above has this wrong)
 *     +0xe4  the pass-4 Cell copy, 20 bytes, to the top of frame at 0xf8
 * An alternative that also reaches an exact-sized frame but scores worse:
 * give pass 3 its own `ClipRect clip2` so `clip`'s live range ends before
 * pass 1 and the pass-1/2 cell can share its slot as the original's does --
 * +16 to 0xfc, score 895; with a shared roads/ILF `Pos` it lands on 0xf4,
 * score 894.  Neither is applied.
 *
 * THE STATEMENT-ORDER LEVER THAT CRACKED RenderView BARELY WORKS HERE.
 * RenderView went 633 -> 478 purely by permuting the independent statements
 * of its tile-geometry block (see the note above).  The same treatment here
 * is worth 4 in total: an exhaustive permutation of every group of
 * independent assignments (the four `clip` fills, scale_x/scale_y,
 * saved_ox/saved_oy, the two `g_map->origin_*` zeroes, mx/my at both
 * projection sites, tcode/set, fsx/fsy, off.x/off.y, tile.x/tile.y, the
 * three SpriteDesc fills, the g_fm_cw/g_fm_ch pairs, lo.x/lo.y) plus two
 * full 1-opt sweeps of every adjacent independent statement pair in the
 * function finds only `saved_oy` before `saved_ox`, `g_fm_ch` before
 * `g_fm_cw` in the ILF loop, and `tile.y = 0;` before `tile.x = 0;`.
 * Also measured and worthless: `th * (w+h)` vs `(w+h) * th` and the same
 * operand flip in g_fm_ox and g_fm_w2 (no change at all -- VC6
 * canonicalises); hoisting `(th+1)>>1` into a local at four different points
 * (881 -> 995 / 1005 / 1047); an `int thv = th;` copy used in the g_fm_h2
 * and/or g_fm_ch expressions (1069).
 *
 * THE BLOCK ORDER IS NOT DRIVEN BY SOURCE ORDER.  Moving the whole TRACK arm
 * to the very end of the chain-loop body behind a forward `goto track_arm;`
 * produces a byte-identical object.  So the original's
 * [single-sprite head][sd fill][TRACK ARM][HalfOffset join][single-sprite
 * tail][ILF loop] layout has to be bought with register pressure or branch
 * shape, not by rearranging the source.
 *
 * WHERE TO START NEXT: index 95, the FIRST place the two streams stop being
 * the same instruction sequence (everything before it is normalised-equal).
 *      orig  95 mov esi,[esp+0x84]      (reload th into a callee-saved reg)
 *            98 imul eax,esi            127 inc esi      131 sar esi,1
 *      ours  97 imul eax,dword [esp+0x6c]        (memory multiplier)
 * The original keeps `th` in esi from 95 to 131 -- across BOTH __ftol calls
 * -- and computes `(th+1)>>1` in the gap between them; we use memory
 * operands at all three sites, which is ONE INSTRUCTION FEWER and shifts the
 * two streams by one from index 95 onward, so a large part of the residual
 * is that single misalignment rather than 881 independent wrong choices.
 * Win that one register decision first.  After it, the biggest
 * register/offset-NORMALISED runs (741 of 1161 slots differ) are 667..762,
 * 792..847, 903..938, 960..1062 and 1110..1136 -- all inside the second half
 * of the chain loop, i.e. the block-layout problem above.
 *
 * Tooling added this round, in scratchpad/renderview/w/:
 *   fmap.py    correct per-instruction esp simulation -> frame slot map for
 *              both sides (handles __stdcall callees popping their own args);
 *   search.py  compile-and-score one file (mismatch, frame size, push
 *              placement) in ~0.2 s, importable;
 *   try.py     run a list of named textual variants and rank them;
 *   groups.py  exhaustively permute named groups of statements;
 *   swap.py / swap2.py  1-opt adjacent-statement-swap sweep with a
 *              conservative dependence test;
 *   perm3.py / climb.py / anneal*.py  random topological orders, hill
 *              climbing and annealing over a statement block's dependence
 *              graph -- this is what found RenderView's ordering.
 * ------------------------------------------------------------------------- */

/* Scope I (2026-09-05): improved, then stopped at the documented
 * shared-tail layout and register/frame floors: 827/1161 strict,
 * first 0, 4216/4225 bytes, no ESCAPES. An explicit ILFTable** carries the
 * sprite+8 address to the loop condition, re-derived at each latch. VC6 now
 * emits add eax,8 / mov eax,[eax] there; the head still CSEs the initial
 * count load, so this is a partial reconstruction of the address carrier.
 * Raw strict improves 844 -> 827, both-blind region cost 465 -> 455, and
 * CFG/stack/import-aware edit distances improve 666/568/536/406 ->
 * 664/562/526/401 (strict/rb/ob/both). No volatile access was added.
 * All six register facts were addressed: ordinary escaped scale homes
 * restore uncached LineTo calls but cost 873-880; escaping def or chain
 * costs 897/1066; a shared BPos copy is inert at 844; a free def reload
 * costs 845; forcing the ILF head load volatile costs 846. Combining the
 * scale/def home forms with the address loop costs 876-884. These changes
 * do not solve the independently-retired cold-arm layout bit; those regressions are rejected.
 * The stack frame remains 0xf4 against the original 0xf8, and the current
 * address spelling is the best measured reachable improvement.
 * Full measurements: docs/lanes/scope-i.md.
 */
/* Scope LL20 (2026-09-08): unchanged at 827/1161, first 0, 4216/4225 B,
 * frame 0xf4.  Index 0 is two things, both measured this round with a
 * push-depth frame map (scratch fm.py) and the LL9/LL10/LL14/LL17/LL18/LL19
 * levers as the brief:
 *  1. `push ebp` vs our `push ebx` at index 1.  The register is the whole
 *     zero web: CreatePen's 0, the busy guard, g_fm_view.x, clip.left/top,
 *     g_fm_oy, the five GetNearestColour/RenderBlock zeros, the two origin
 *     clears, the height/width guards, x = 0 in pass 1 -- and it COALESCES
 *     with pass 2's outer counter y (323 `xor ebp,ebp` is both the zero and
 *     y = 0; 347 `[ecx+ebp*4]`, 430 `inc ebp`).  Pass 2's x takes the other
 *     register (329 `xor ebx,ebx`).  Ours is the same web with ebx/ebp
 *     swapped everywhere (zero/y in ebx, x in ebp); the zero-use count is
 *     identical (25 through pass 2) and in BOTH bodies neither ebx nor ebp
 *     is otherwise written anywhere over the web's range (the original's
 *     first ebx write is `set` at 216, ours the same).  So it is a pure
 *     ebx/ebp tie-break between the zero/y node and x, decided by VC6's
 *     colouring order, not by interference.  Inert (byte-identical):
 *     `int y, x` declaration order; block-scope `int x, y` around pass 2;
 *     pass 2 on fresh names x2/y2.
 *  2. Frame 0xf4 vs 0xf8, and it is NOT one missing 4-byte slot: the pool
 *     differs by +20 and -16.  Original pool: sd at +0xac, Pos +0xc4 (roads)
 *     +0xcc (single sprite) +0xd4 (mark) +0xdc (p1), the pass-4 Cell copy at
 *     +0xe4..+0xf8 (525 `lea edi,[esp+0xf4]`).  Ours: three ElemID homes
 *     and `link` at +0xac..+0xb8, sd at +0xbc, the four Pos at +0xd4..+0xf0,
 *     and NO Cell in the pool -- our pass-4 copy lands on the pass-1/2 slot
 *     (521 `lea edi,[esp+0x6c]`), because VC6 forwards `c`'s fields into the
 *     by-value CellMarkTest(c) and never makes the inline temporary the
 *     helper's comment promises.
 *     *** THE LEVER THAT MOVES INDEX 0 (measured, not committed): put the
 *     WHOLE pass-4 loop body in one `static __inline void
 *     FullMap_ChainCell(Cell c, BPos bpos, int scale_x, int scale_y,
 *     Elem* ... x6, Sprite* ... x3, void* pen, TileBounds* tbp, Pos* tilep,
 *     Pos* gtbp, SpriteDesc* sdp)` taking the cell BY VALUE, called as
 *     `FullMap_ChainCell(*chain, bpos, ...)` with every `continue` a
 *     `return`. ***  Result: frame 0xf8, first diverging index 1, the pool
 *     EXACTLY the original's (sd +0xac, Pos +0xc4/+0xcc/+0xd4/+0xdc in the
 *     original's order, Cell +0xe4, 521-526 `lea edi,[esp+0xf4] / rep movsd
 *     / mov eax,[esp+0x100]`), still three `rep movsd`, 260 = 260 frame
 *     references.  Cost: strict 864, rb 814, ob 786 (from 827/763/758),
 *     three instructions long (ESCAPES: the trimmed body hides a jump
 *     target), because the block layout moves AGAIN -- the track arm is now
 *     laid out LAST (876..1121, after the ILF loop) and the sd fill stays
 *     before the join.  So an inlined by-value body is the first thing
 *     found that moves the layout bit at all; it just moves it the wrong
 *     way.  Two respellings of the helper are byte-identical to it: the
 *     track arm as `} else {` around the sprite path instead of `return`,
 *     and the mark test as a direct `c.flags` expression.  Retired at 827
 *     with the helper recorded as the next lane's starting point (its text
 *     is in docs/lanes/scope-ll20.md); it is the only construct measured in
 *     eleven rounds that reproduces the original's pool.
 */
// WIP-FUNCTION: LEGOLAND 0x004567a0  (28.8%, 827/1161 strict, 4216/4225 bytes; ILF address carrier improved; first 0)
void RenderFullMap(void)
{
    Elem*       e_track;
    Elem*       e_track_h;
    Elem*       e_track_h0;
    Elem*       e_track_hp;
    Elem*       e_castle;
    Elem*       e_roads;
    Sprite*     s_blob;
    Sprite*     s_stick;
    Sprite*     s_lights;
    void*       pen;
    ClipRect    clip;
    TileBounds  tb;
    Pos         tile;
    Pos         gtb;
    int         lox, loy;
    Cell        c;
    Cell*       chain;
    SpriteDesc  sd;
    SpriteDesc* desc;
    TerrainObj* tobj;
    TileSet*    set;
    Sprite*     spr;
    ILFTable*   ilf;
    ILFTable**  slot;
    ObjDef*     def;
    RoadRec*    road;
    int         saved_ox;
    int         saved_oy;
    BPos        bpos;
    int         tw, th;
    int         scale_x, scale_y;
    int         x, y, i;
    int         mx, my;
    int         x0, y0;
    float       fsx, fsy;
    unsigned int tcode;

    e_track    = ElemID("SQUARE_TRACK");
    e_track_h  = ElemID("SQUARE_TRACK_HEIGHT");
    e_track_h0 = ElemID("SQUARE_TRACK_HEIGHT_0");
    e_track_hp = ElemID("SQUARE_TRACK_HEIGHT_PATH");
    e_castle   = ElemID("CASTLE_DUMMY");
    e_roads    = ElemID("DRIVING SCHOOL ROADS");
    s_blob     = LoadSprite("TRACKBLOB.LLS", 1);
    s_stick    = LoadSprite("TRACKSTICK.LLS", 1);
    s_lights   = LoadSprite("MAPLIGHTS.LLS", 1);
    pen        = CreatePen(0, 2, 0xff4000);
    if (g_fullmap_busy != 0)
        return;                 /* original bug: pen and sprites leak here */

    CalculateFullMapRenderOrder();
    g_fm_view.x = 0;
    memset(g_map_marks, 0, 1024 * sizeof(MapMark));
    g_fm_view.y = 0x20;
    g_fm_view.w = 640;
    g_fm_view.h = 340;
    StoreClipping();
    clip.left = 0;
    clip.right = g_fm_view.w;
    clip.top = 0;
    clip.bottom = g_fm_view.h;
    SetClipping(&clip);

    GetTileDimensions(&tw, &th);
    g_fm_oy = 0;
    g_fm_ox = -(g_map->height * tw / 2);
    g_fm_w2 = g_map->width * tw / 2;
    g_fm_h2 = (g_map->width + g_map->height) * th / 2;
    g_fm_spanx = g_fm_w2 - g_fm_ox + 1;
    g_fm_spany = g_fm_h2 + 1;
    scale_x = (g_fm_view.w << 16) / g_fm_spanx;
    scale_y = (g_fm_view.h << 16) / g_fm_spany;
    g_fm_cw = (short)(int)((float)g_fm_view.w * tw / g_fm_spanx + 1.0f);
    g_fm_ch = (short)(int)((float)g_fm_view.h * th / g_fm_spany + 1.0f);
    RenderBlock(0, 0, g_fm_view.w, g_fm_view.h, GetNearestColour(0, 0, 0));

    saved_ox = g_map->origin_x;
    saved_oy = g_map->origin_y;
    g_map->origin_x = 0;
    g_map->origin_y = 0;
    g_fullmap_busy = 1;
    g_fm_cy = (g_fm_view.h
               - ((g_map->width + g_map->height - 2) * ((th + 1) >> 1) - g_fm_oy)
                 * g_fm_view.w / g_fm_spany / 2) >> 1;

    /* ---- pass 1: the terrain wash ---- */
    for (y = 0; y < g_map->height; y++) {
        for (x = 0; x < g_map->width; x++) {
            if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
                c = g_map_rows[y][x];
            else
                c.flags = 0x40;
            if (c.flags & 8)
                continue;
            tcode = g_map_rows[y][x].tile;
            set = g_tile_info[tcode].set;
            tile.y = y;
            tile.x = x;
            GetTileBounds(&tile, &tb);
            FullMapScroll(&tb);
            mx = FullMapX(&tb, scale_x);
            my = FullMapY(&tb, scale_y);
            if ((c.flags & 0x10) && (c.flags & 0x80)) {
                RenderBlock(mx - 7, my - 2, g_fm_cw + 5, g_fm_ch + 4,
                            GetNearestColour(0x80, 0x80, 0x80));
                continue;
            }
            tcode = set->code[tcode - set->base_slot] & 0x3f;
            if (tcode > 0x30)
                continue;
            switch (tcode) {
            case 0x00:
            case 0x20:
                RenderBlock(mx - 7, my - 2, g_fm_cw + 5, g_fm_ch + 4,
                            GetNearestColour(0x00, 0x8f, 0x4f));
                break;
            case 0x01:
            case 0x21:
                RenderBlock(mx - 7, my - 2, g_fm_cw + 5, g_fm_ch + 4,
                            GetNearestColour(0xff, 0xe0, 0x8f));
                break;
            case 0x30:
                RenderBlock(mx - 7, my - 2, g_fm_cw + 5, g_fm_ch + 4,
                            GetNearestColour(0x5b, 0xbe, 0x02));
                break;
            }
        }
    }

    /* ---- pass 2: object cells draw their own tile sprite ---- */
    PushRenderingStatusAndUnlockVideoSurface();
    for (y = 0; y < g_map->height; y++) {
        for (x = 0; x < g_map->width; x++) {
            if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height) {
                c = g_map_rows[y][x];
            } else {
                c.tile = 0;
                c.flags = 0x40;
            }
            if (!(c.flags & 8))
                continue;
            fsx = (float)g_fm_view.w / g_fm_spanx;
            fsy = (float)g_fm_view.h / g_fm_spany;
            def = ((Obj*)c.obj)->def;
            gtb.x = x;
            gtb.y = y;
            GetTileBounds(&gtb, &tb);
            tile.x = def->dx;
            tile.y = def->dy;
            HalfPos(&tile);
            tb.top += tile.y + (g_scroll_y >> 8);
            my = (int)((float)(tb.top - g_fm_oy) * fsy) + g_fm_cy;
            tb.left += tile.x + (g_scroll_x >> 8);
            mx = (int)((float)(tb.left - g_fm_ox) * fsx);
            PrintScaledSprite(g_tile_sprites[c.tile], mx, my,
                              g_fm_cw + 1, g_fm_ch + 1);
        }
    }

    /* ---- pass 3: the perimeter terrain objects ---- */
    clip.left = 0;
    clip.right = g_fm_view.w;
    clip.top = 0;
    clip.bottom = g_fm_view.h;
    SetClipping(&clip);
    {
        TileBounds tb3;

        tobj = g_terrain_objects;
        tile.y = 0;
        tile.x = 0;
        GetTileBounds(&tile, &tb3);
        FullMapScroll(&tb3);
        mx = FullMapX(&tb3, scale_x);
        my = FullMapY(&tb3, scale_y);
    }
    while (tobj) {
        spr = tobj->sprite;
        PrintScaledSprite(spr,
                          mx + (((tw + tw) / 3 + tobj->x) * scale_x >> 16),
                          my + (tobj->y * scale_y >> 16),
                          spr->w * scale_x >> 16,
                          spr->h * scale_y >> 16);
        tobj = tobj->next;
    }

    /* ---- pass 4: the render chain ---- */
    for (chain = GetFirstRenderObject(); chain; chain = GetNextRenderObject(chain)) {
        bpos = chain->base;
        c = *chain;
        if (CellMarkTest(c)) {
            TileBoundsAt((c.base.x & ~7) + 4, (c.base.y & ~7) + 4, &tb);
            FullMapScroll(&tb);
            g_map_marks[c.base.y >> 3][c.base.x >> 3].x = FullMapX(&tb, scale_x);
            g_map_marks[c.base.y >> 3][c.base.x >> 3].y = FullMapY(&tb, scale_y);
        }
        def = ((Obj*)c.obj)->def;
        if (!(def->flags & 4) && !(def->flags & 0x400)) {
            /* only the driving-school roads draw here, and only at a lit
             * junction record */
            if (def != (ObjDef*)e_roads->data)
                continue;
            sd.sprite = s_lights;
            sd.dx = -0x66;
            sd.dy = -0x58;
            road = GetRoadRecord(bpos.x, bpos.y);
            if (road == 0)
                continue;
            if ((road->kind & 0xf) != 5)
                continue;
            /* Pooled Pos, as the original has here (its `lea eax,[esp+0xd4]`
             * at index 610).  Round w7's "frame ballast" demotion of this
             * site is gone; see item 5 of the round-w8 note above. */
            TileBoundsAt(bpos.x, bpos.y, &tb);
            tb.left += HalfOffset(sd.dx);
            tb.top += HalfOffset(sd.dy);
            FullMapScroll(&tb);
            g_fm_cw = s_lights->w;
            g_fm_ch = s_lights->h;
            PrintScaledSprite(s_lights,
                              FullMapX(&tb, scale_x),
                              FullMapY(&tb, scale_y),
                              g_fm_cw * scale_x >> 16,
                              g_fm_ch * scale_y >> 16);
            continue;
        }
        if (def->ctx == e_track || def->ctx == e_track_h
            || def->ctx == e_track_h0 || def->ctx == e_track_hp
            || def->ctx == e_castle) {
            Pos   p1;
            float h0, h1;
            int   link;

            /* the track arm reads the CELL COPY's base, not `bpos` -- that is
             * what keeps the 20-byte Cell live across the arm dispatch in the
             * original (see the residual note above). */
            tile.x = c.base.x;
            tile.y = c.base.y;
            if (GetTrackSegment(&tile, &h0, &p1, &h1, &link)) {
                void* hdc;
                void* old;

                if (h0 < 0.0f)
                    h0 = 0.0f;
                if (h1 < 0.0f)
                    h1 = 0.0f;
                gtb.x = tile.x;
                gtb.y = tile.y;
                GetTileBounds(&gtb, &tb);
                tb.top += HalfOffset(-(int)h0);
                FullMapScroll(&tb);
                x0 = FullMapX(&tb, scale_x);
                y0 = FullMapY(&tb, scale_y);
                gtb.x = p1.x;
                gtb.y = p1.y;
                GetTileBounds(&gtb, &tb);
                tb.top += HalfOffset(-(int)h1);
                FullMapScroll(&tb);
                mx = FullMapX(&tb, scale_x);
                my = FullMapY(&tb, scale_y);
                if (def->ctx != e_track_hp) {
                    g_fm_cw = s_stick->w;
                    g_fm_ch = (short)(int)((float)scale_y * h0
                                           * 7.62939453125e-06f);
                    if (g_fm_ch > 0)
                        PrintScaledSprite(s_stick,
                                          x0 - ((g_fm_cw * scale_x) >> 17), y0,
                                          (g_fm_cw * scale_x) >> 16, g_fm_ch);
                }
                PushRenderingStatusAndUnlockVideoSurface();
                g_draw_surface->vtbl->GetDC(g_draw_surface, &hdc);
                old = SelectObject(hdc, pen);
                MoveToEx(hdc, mx, my, 0);
                LineTo(hdc, x0, y0);
                if (link) {
                    gtb.x = tile.x - 0xa;
                    gtb.y = tile.y;
                    GetTileBounds(&gtb, &tb);
                    tb.top += HalfOffset(-(int)h0);
                    FullMapScroll(&tb);
                    LineTo(hdc, FullMapX(&tb, scale_x),
                           FullMapY(&tb, scale_y));
                }
                SelectObject(hdc, old);
                g_draw_surface->vtbl->ReleaseDC(g_draw_surface, hdc);
                PopRenderingStatus();
            }
            /* The original fills the descriptor for TRACKBLOB here and then
             * moves straight on to the next chain entry -- the blob is never
             * drawn.  Reproduced: three dead stores. */
            sd.sprite = s_blob;
            sd.dx = 0;
            sd.dy = (int)(-h0);
            continue;
        }
        if (def->flags & 0x400) {
            SpriteDesc* (*cb)(void*, BPos) = def->draw;

            if (cb == 0)
                continue;
            desc = cb(def->ctx, bpos);
        } else {
            sd.sprite = def->sprite;
            sd.dx = def->dx;
            sd.dy = def->dy;
            desc = &sd;
        }
        if (desc == 0)
            continue;
        if (desc->sprite == 0)
            continue;
        if (!(*(unsigned int*)((char*)desc->sprite + 0x10) & 0x8000)) {
            /* Pooled Pos, as the original has here (its `lea eax,[esp+0xdc]`
             * at index 690).  The original pools one at ALL FOUR of this
             * loop's TileBoundsAt sites -- 0xd4 roads, 0xdc HERE, 0xe4 mark
             * grid, 0xa0 ILF -- and round w8 restored the two that round w7
             * had demoted to the function-level `tile` as frame ballast. */
            TileBoundsAt(bpos.x, bpos.y, &tb);
            {
                int gdx = desc->dx;
                int gdy = desc->dy;

                tb.left += HalfOffset(gdx);
                tb.top += HalfOffset(gdy);
            }
            FullMapScroll(&tb);
            g_fm_cw = ((Sprite*)desc->sprite)->w;
            g_fm_ch = ((Sprite*)desc->sprite)->h;
            PrintScaledSprite((Sprite*)desc->sprite,
                              FullMapX(&tb, scale_x),
                              FullMapY(&tb, scale_y),
                              g_fm_cw * scale_x >> 16,
                              g_fm_ch * scale_y >> 16);
            continue;
        }
        slot = (ILFTable**)((char*)desc->sprite + 0x08);
        ilf = *slot;
        if (ilf->count <= 0)
            continue;
        i = 0;
        do {
            Sprite* layer;

            ilf = *slot;
            layer = ilf->sprites[i];
            lox = ilf->dx[i];
            loy = ilf->dy[i];
            TileBoundsAt(bpos.x, bpos.y, &tb);
            tb.left += HalfOffset(desc->dx) + HalfOffset(lox);
            tb.top += HalfOffset(desc->dy) + HalfOffset(loy);
            FullMapScroll(&tb);
            g_fm_ch = layer->h;
            g_fm_cw = layer->w;
            PrintScaledSprite(layer,
                              FullMapX(&tb, scale_x),
                              FullMapY(&tb, scale_y),
                              g_fm_cw * scale_x >> 16,
                              g_fm_ch * scale_y >> 16);
            i++;
            slot = (ILFTable**)((char*)desc->sprite + 0x08);
        } while (i < (*slot)->count);
    }

    PopRenderingStatus();
    g_map->origin_x = saved_ox;
    g_map->origin_y = saved_oy;
    RestoreClipping();
    CommitCliprectToHardware();
    if (s_blob)
        KillSprite(s_blob);
    if (s_stick)
        KillSprite(s_stick);
    if (s_lights)
        KillSprite(s_lights);
    DeleteObject(pen);
    CalculateMapRenderOrder();
}
