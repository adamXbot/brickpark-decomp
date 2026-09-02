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

/* 903/903 instructions, 2870B vs 2880B, no ESCAPES, but only 270 of them
 * compare equal (audit.py mismatch=633, 29.9%).  The BLOCK STRUCTURE, the branch senses, the loop shapes and the call
 * sequence are index-for-index right (a register/offset-normalised diff shows
 * only ~160 differing slots out of 903); what is wrong is one global register
 * decision that then renames registers and shifts frame offsets everywhere:
 *
 *  - The original SPLITS the prologue: `push esi / push edi` at entry and
 *    `push ebx / push ebp` sunk into the block that calls SetClipping (they are
 *    popped again right after DrawAndClearPrintList).  VC6 gives us all four at
 *    entry, in the order ebx, ebp, esi, edi -- the two pairs swap, so the later
 *    pair can no longer sink.  Measured: the split appears as soon as the
 *    sprite-emit block (`if (g_sort_count) loop else SortSprite`) needs one
 *    register fewer -- hoisting `desc->sprite` into a local, or dropping either
 *    arm, flips it -- but every such spelling costs the original's three
 *    `mov reg,[ebp]` sprite reads, so it is not the source form.
 *  - Downstream of that: `sx`/`tw` land in ebx/ebp the other way round (so the
 *    two `idiv` use a memory divisor where the original reloads `th` into ebx),
 *    the gather loop keeps `count` in ebx and spills the column position where
 *    the original does the reverse, and the frame is 4 bytes short (0x2f8c vs
 *    0x2f90 -- one spill slot the original's tighter allocation forces).
 * Everything the browser runtime needs (the draw order, the walk, the strip
 * rule, the per-cell colour rule) is recovered and documented above.
 * Tried: 30+ orderings of the setup and tile-geometry statements, post-guard
 * inner scopes, an inline gather helper, an inline emit helper (with and
 * without a Pos-by-value argument), an explicit `put` induction pointer, an
 * address-taken count, count as a global, both arm orders of the 0x20 branch,
 * `f` re-read vs cached, and a separate tail counter. */
// WIP-FUNCTION: LEGOLAND 0x0045b180  (29%, 903/903 insns; split-prologue register-pair tie-break)
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
    int         rowx, rowy;
    int         n;
    int         key1, key2, key;
    int         aleft, bright;
    int         d1, d2, span, step, three;
    int         mode;
    Pos         at;
    unsigned short f;

    view.left = g_map->origin_x;
    view.top = g_map->origin_y;
    view.right = g_map->view_w + g_map->origin_x;
    view.bottom = g_map->view_h + g_map->origin_y;
    cls = g_odf_head;
    count = 0;
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
    sw = (short)(sh + sh);
    th = sh;
    tw = sw;
    sx = g_scroll_x >> 8;
    qx = sx / tw;
    hw = (tw + 1) >> 1;
    hh = (th + 1) >> 1;
    sy = (g_scroll_y >> 8) - hh;
    rx = sx % tw;
    qy = sy / th;
    ry = sy % th;
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

    ylimit = g_map->view_h + view.bottom;
    xlimit = tw + tw + view.right;
    for (py = view.top - (th + th) - ry; py < ylimit; py += th) {
        rowx = tile.x;
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
        tile.x = rowx + 1;
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
                bright = corner.right;
                key2 = ((corner.top + corner.bottom) >> 1) - g_map->origin_y;
                if (key1 != key2) {
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
                        c->left = aleft + three;
                        c->bottom = view.bottom;
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
                    c = &g_sort_clips[g_sort_count];
                    g_sort_keys[g_sort_count] = key2;
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
                    ctx_build.obj = cell->obj;
                    mode = desc->mode;
                    ctx_build.kind = 0x104;
                    ctx_build.base = cell->base;
                    if (def->build_sprite) {
                        at.x = tb.left + HalfOffset(desc->dx + def->anim_dx);
                        at.y = tb.top + HalfOffset(desc->dy + def->anim_dy);
                        desc->sprite = def->build_sprite;
                        SetOverrideFrame(GetBuildAnimFrame(def, base));
                    } else {
                        at.x = tb.left + HalfOffset(desc->dx);
                        at.y = tb.top + HalfOffset(desc->dy);
                        mode = 0xff00;
                    }
                    EmitObjectSprite(desc, at, key1, mode, &ctx_build);
                } else {
                    mode = desc->mode;
                    ctx_normal.obj = cell->obj;
                    ctx_normal.base = cell->base;
                    ctx_normal.kind = 0x103;
                    at.x = tb.left + HalfOffset(desc->dx);
                    at.y = tb.top + HalfOffset(desc->dy);
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
 *   g_fm_cy (0x667c20) = (340 - ((map->w + map->h - 2) * (th+1)/2 * 640
 *                                / spany)) / 2       vertical centring
 *
 *   minimap_x(world) = ((world_x + (g_scroll_x >> 8)) - g_fm_ox) * scale_x >> 16
 *   minimap_y(world) = ((world_y + (g_scroll_y >> 8)) - g_fm_oy) * scale_y >> 16
 *                      + g_fm_cy
 *   world_x/world_y come from GetTileBounds (left/top of the tile diamond).
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
 *     - otherwise the DISPLAYED TILE decides.  desc = g_tile_info[tile].elem
 *       is the TSF descriptor (base slot at +0x00, code[] at +0x0c), so
 *           code = desc->code[tile - desc->base] & 0x3f
 *       and code (0..0x30) selects the colour through the 0x31-byte table at
 *       0x00457834 -> the 4-way jump table at 0x00457824:
 *           code 0x00, 0x20            -> GREEN  r=0x00 g=0x8f b=0x4f  (grass)
 *           code 0x01, 0x21            -> SAND   r=0xff g=0xe0 b=0x8f  (beach)
 *           code 0x30                  -> LIME   r=0x5b g=0xbe b=0x02
 *           everything else, or > 0x30 -> not drawn (stays black = water)
 *
 *     The block painted is RenderBlock(mx - 7, my - 2, g_fm_cw + 5,
 *     g_fm_ch + 4, colour) -- i.e. each cell is a slightly oversized filled
 *     rectangle so the diamonds tile without gaps.
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
 *           -> the ROLLER-COASTER path: the ride query at 0x00424050 returns
 *           the piece's two endpoints and heights; the segment is drawn as a
 *           GDI MoveToEx/LineTo polyline in the 0x00ff4000 pen on the unlocked
 *           surface DC, TRACKSTICK.LLS is drawn for the support height and
 *           TRACKBLOB.LLS for the node.
 *         * class flags (+0x1c) & 0x0004 or & 0x0400 -> the normal object:
 *           either the class draw callback (+0xa0, called with +0xc4 and the
 *           packed base coordinate) or the static {sprite +0x64, dx +0x14,
 *           dy +0x18}; an ILF sprite (flags & 0x8000) draws each of its layers
 *           at its own offset, otherwise the single sprite is drawn scaled.
 *         * neither flag set -> only the DRIVING SCHOOL ROADS class draws, and
 *           only where the road record at 0x004125f0(x, y) has kind
 *           (+0x14 & 0x0f) == 5: MAPLIGHTS.LLS at (-0x33, -0x2c).
 *
 * Epilogue: PopRenderingStatus, restore Map +0x20/+0x22, RestoreClipping,
 * CommitCliprectToHardware, KillSprite x3, DeleteObject(pen) and finally
 * CalculateMapRenderOrder to put the world-view chain back.
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
extern MapMark      g_map_marks[];          /* 0x008119c0 */
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
extern void    HalfPos(Pos* p);                                     /* 0x00456770 */
extern RoadRec* GetRoadRecord(int x, int y);                        /* 0x004125f0 */
/* 0x00424050: the track-piece query -- fills the two end tiles and their
 * heights for the coaster segment on `tile`; 0 when there is none. */
extern int     GetTrackSegment(Pos* tile, float* h0, Pos* p0, float* h1, Pos* p1);

/* An ILF layer table hanging off an 0x8000 sprite's +0x08 (printlist.c). */
typedef struct ILFTable {
    int      f00;               /* +0x00 */
    int      count;             /* +0x04 */
    Sprite** sprites;           /* +0x08 */
    int*     dx;                /* +0x0c */
    int*     dy;                /* +0x10 */
} ILFTable;

/* Project a world pixel position onto the minimap. */
static __inline int FullMapX(int wx, int scale_x)
{
    return ((wx + (g_scroll_x >> 8)) - g_fm_ox) * scale_x >> 16;
}

static __inline int FullMapY(int wy, int scale_y)
{
    return (((wy + (g_scroll_y >> 8)) - g_fm_oy) * scale_y >> 16) + g_fm_cy;
}

/* -------------------------------------------------------------------------
 * 0x004567a0 -- the overview-map draw callback (see the write-up above).
 *
 * NOT MATCHED: this body is a semantic reconstruction of the 1161-instruction
 * original, written from the disassembly so the browser runtime and a later
 * matching pass have the algorithm, the projection and the colour rule in one
 * place.  It has not been driven to instruction parity: the roller-coaster
 * arm in particular (the GDI polyline through GetTrackSegment) is reconstructed
 * from the call/branch shape only.  Do not promote this marker without running
 * tools/audit.py.
 * ------------------------------------------------------------------------- */

// WIP-FUNCTION: LEGOLAND 0x004567a0  (semantic reconstruction only, not driven to a match)
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
    Pos         off;
    Cell        c;
    Cell*       chain;
    SpriteDesc  sd;
    SpriteDesc* desc;
    TerrainObj* tobj;
    TileSet*    set;
    Sprite*     spr;
    ILFTable*   ilf;
    ObjDef*     def;
    RoadRec*    road;
    unsigned short saved_ox;
    unsigned short saved_oy;
    int         tw, th;
    int         scale_x, scale_y;
    int         x, y, i, code, colour;
    int         mx, my;
    float       fsx, fsy;

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
    for (i = 0; i < 1024; i++) {
        g_map_marks[i].x = 0;
        g_map_marks[i].y = 0;
    }
    g_fm_view.x = 0;
    g_fm_view.y = 0x20;
    g_fm_view.w = 640;
    g_fm_view.h = 340;
    StoreClipping();
    clip.left = 0;
    clip.top = 0;
    clip.right = g_fm_view.w;
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
    g_fm_cw = (short)(int)((float)(g_fm_view.w * tw) / (float)g_fm_spanx + 1.0f);
    g_fm_ch = (short)(int)((float)(g_fm_view.h * th) / (float)g_fm_spany + 1.0f);
    RenderBlock(0, 0, g_fm_view.w, g_fm_view.h, GetNearestColour(0, 0, 0));

    saved_ox = g_map->origin_x;
    saved_oy = g_map->origin_y;
    g_map->origin_x = 0;
    g_map->origin_y = 0;
    g_fullmap_busy = 1;
    g_fm_cy = (g_fm_view.h
               - ((g_map->width + g_map->height - 2) * ((th + 1) >> 1)
                  * g_fm_view.w / g_fm_spany) / 2) / 2;

    /* ---- pass 1: the terrain wash ---- */
    for (y = 0; y < g_map->height; y++) {
        for (x = 0; x < g_map->width; x++) {
            if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
                c = g_map_rows[y][x];
            else
                c.flags = 0x40;
            if (c.flags & 8)
                continue;
            set = g_tile_info[g_map_rows[y][x].tile].set;
            tile.x = x;
            tile.y = y;
            GetTileBounds(&tile, &tb);
            mx = FullMapX(tb.left, scale_x);
            my = FullMapY(tb.top, scale_y);
            if ((c.flags & 0x10) && (c.flags & 0x80)) {
                colour = GetNearestColour(0x80, 0x80, 0x80);
            } else {
                code = set->code[g_map_rows[y][x].tile - set->base_slot] & 0x3f;
                if (code > 0x30)
                    continue;
                if (code == 0x00 || code == 0x20)
                    colour = GetNearestColour(0x00, 0x8f, 0x4f);
                else if (code == 0x01 || code == 0x21)
                    colour = GetNearestColour(0xff, 0xe0, 0x8f);
                else if (code == 0x30)
                    colour = GetNearestColour(0x5b, 0xbe, 0x02);
                else
                    continue;
            }
            RenderBlock(mx - 7, my - 2, g_fm_cw + 5, g_fm_ch + 4, colour);
        }
    }

    /* ---- pass 2: object cells draw their own tile sprite ---- */
    PushRenderingStatusAndUnlockVideoSurface();
    fsx = (float)g_fm_view.w / (float)g_fm_spanx;
    fsy = (float)g_fm_view.h / (float)g_fm_spany;
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
            def = ((Obj*)c.obj)->def;
            tile.x = x;
            tile.y = y;
            GetTileBounds(&tile, &tb);
            off.x = def->dx;
            off.y = def->dy;
            HalfPos(&off);
            mx = (int)((float)((tb.left + off.x + (g_scroll_x >> 8)) - g_fm_ox) * fsx);
            my = (int)((float)((tb.top + off.y + (g_scroll_y >> 8)) - g_fm_oy) * fsy)
                 + g_fm_cy;
            PrintScaledSprite(g_tile_sprites[c.tile], mx, my,
                              g_fm_cw + 1, g_fm_ch + 1);
        }
    }

    /* ---- pass 3: the perimeter terrain objects ---- */
    clip.left = 0;
    clip.top = 0;
    clip.right = g_fm_view.w;
    clip.bottom = g_fm_view.h;
    SetClipping(&clip);
    tile.x = 0;
    tile.y = 0;
    GetTileBounds(&tile, &tb);
    mx = FullMapX(tb.left, scale_x);
    my = FullMapY(tb.top, scale_y);
    for (tobj = g_terrain_objects; tobj; tobj = tobj->next) {
        PrintScaledSprite(tobj->sprite,
                          mx + ((tobj->x + (tw + tw) / 3) * scale_x >> 16),
                          my + (tobj->y * scale_y >> 16),
                          tobj->sprite->w * scale_x >> 16,
                          tobj->sprite->h * scale_y >> 16);
    }

    /* ---- pass 4: the render chain ---- */
    for (chain = GetFirstRenderObject(); chain; chain = GetNextRenderObject(chain)) {
        BPos bpos = chain->base;

        c = *chain;
        if ((c.flags & 0x200) && !(c.flags & 4)) {
            tile.x = (c.base.x & ~7) + 4;
            tile.y = (c.base.y & ~7) + 4;
            GetTileBounds(&tile, &tb);
            i = ((c.base.y >> 3) << 5) + (c.base.x >> 3);
            g_map_marks[i].x = FullMapX(tb.left, scale_x);
            g_map_marks[i].y = FullMapY(tb.top, scale_y);
        }
        def = ((Obj*)c.obj)->def;
        if (!(def->flags & 4) && !(def->flags & 0x400)) {
            /* only the driving-school roads draw here, and only at a lit
             * junction record */
            if (def != (ObjDef*)e_roads->data)
                continue;
            road = GetRoadRecord(c.base.x, c.base.y);
            if (road == 0 || (road->kind & 0xf) != 5)
                continue;
            tile.x = c.base.x;
            tile.y = c.base.y;
            GetTileBounds(&tile, &tb);
            g_fm_cw = s_lights->w;
            g_fm_ch = s_lights->h;
            PrintScaledSprite(s_lights,
                              FullMapX(tb.left - 0x33, scale_x),
                              FullMapY(tb.top - 0x2c, scale_y),
                              s_lights->w * scale_x >> 16,
                              s_lights->h * scale_y >> 16);
            continue;
        }
        if (def->ctx == e_track || def->ctx == e_track_h
            || def->ctx == e_track_h0 || def->ctx == e_track_hp
            || def->ctx == e_castle) {
            /* The roller-coaster arm: a GDI polyline between the segment's two
             * projected endpoints, plus a support stick and a node blob.
             * Reconstructed from the call/branch shape; NOT verified. */
            Pos   p0, p1;
            float h0, h1;
            void* hdc;
            void* old;
            int   x0, y0, x1, y1;

            tile.x = c.base.x;
            tile.y = c.base.y;
            if (!GetTrackSegment(&tile, &h0, &p0, &h1, &p1))
                continue;
            if (h0 < 0.0f)
                h0 = 0.0f;
            if (h1 < 0.0f)
                h1 = 0.0f;
            GetTileBounds(&p0, &tb);
            x0 = FullMapX(tb.left, scale_x);
            y0 = FullMapY(tb.top - (int)h0 / 2, scale_y);
            GetTileBounds(&p1, &tb);
            x1 = FullMapX(tb.left, scale_x);
            y1 = FullMapY(tb.top - (int)h1 / 2, scale_y);
            if (def->ctx != e_track_hp) {
                g_fm_cw = s_stick->w;
                g_fm_ch = (short)(int)((float)th * (float)h0 * 7.62939453125e-06f);
                if (g_fm_ch > 0)
                    PrintScaledSprite(s_stick, x0,
                                      y0 - ((g_fm_cw * scale_x) >> 17),
                                      (g_fm_cw * scale_x) >> 16, g_fm_ch);
            }
            PushRenderingStatusAndUnlockVideoSurface();
            g_draw_surface->vtbl->GetDC(g_draw_surface, &hdc);
            old = SelectObject(hdc, pen);
            MoveToEx(hdc, x0, y0, 0);
            LineTo(hdc, x1, y1);
            if (s_blob) {
                GetTileBounds(&p1, &tb);
                LineTo(hdc, FullMapX(tb.left, scale_x),
                       FullMapY(tb.top - 0xa, scale_y));
            }
            SelectObject(hdc, old);
            g_draw_surface->vtbl->ReleaseDC(g_draw_surface, hdc);
            PopRenderingStatus();
            continue;
        }
        if (def->flags & 0x400) {
            if (def->draw == 0)
                continue;
            desc = def->draw(def->ctx, bpos);
            if (desc == 0)
                continue;
        } else {
            sd.sprite = def->sprite;
            sd.dx = def->dx;
            sd.dy = def->dy;
            desc = &sd;
        }
        spr = (Sprite*)desc->sprite;
        if (spr == 0)
            continue;
        if (*(unsigned int*)((char*)spr + 0x10) & 0x8000) {
            ilf = *(ILFTable**)((char*)spr + 0x08);
            for (i = 0; i < ilf->count; i++) {
                Sprite* layer = ilf->sprites[i];
                Pos     lo;

                lo.x = ilf->dx[i];
                lo.y = ilf->dy[i];
                tile.x = c.base.x;
                tile.y = c.base.y;
                GetTileBounds(&tile, &tb);
                g_fm_cw = layer->w;
                g_fm_ch = layer->h;
                PrintScaledSprite(layer,
                                  FullMapX(tb.left + HalfOffset(desc->dx)
                                           + HalfOffset(lo.x), scale_x),
                                  FullMapY(tb.top + HalfOffset(desc->dy)
                                           + HalfOffset(lo.y), scale_y),
                                  layer->w * scale_x >> 16,
                                  layer->h * scale_y >> 16);
            }
            continue;
        }
        tile.x = c.base.x;
        tile.y = c.base.y;
        GetTileBounds(&tile, &tb);
        g_fm_cw = spr->w;
        g_fm_ch = spr->h;
        PrintScaledSprite(spr,
                          FullMapX(tb.left + HalfOffset(desc->dx), scale_x),
                          FullMapY(tb.top + HalfOffset(desc->dy), scale_y),
                          spr->w * scale_x >> 16,
                          spr->h * scale_y >> 16);
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
