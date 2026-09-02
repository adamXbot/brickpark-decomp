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
 * audit.py mismatch = 478 (425 of 903 index-for-index identical, 47.1%).
 * Was 633 at the start of this round; the whole gain came from ONE class of
 * lever, described below, and the function is now structurally very close.
 *
 * WHAT IS NOW EXACT (do not disturb it):
 *  - indices 0..63 are IDENTICAL, byte for byte: the __chkstk prologue with
 *    frame 0x2f90 (was 0x2f8c), the SPLIT prologue (`push esi` / `push edi`
 *    at 5/7, `push ebx` / `push ebp` sunk to 58/60 in the block that calls
 *    SetClipping), the four `view` fills, count/cls, the two global flags,
 *    GetClipping / RenderGroundLayer / PrintBackground and the whole
 *    g_edit_state cursor gate;
 *  - the ODF pre-render walk (352..366): `cls` now lives in esi across the
 *    walk instead of being reloaded from its frame home each iteration;
 *  - the object-queue head (375..391): TWO short-lived zeros rematerialised
 *    inside the loop (`xor edx,edx` / `xor ecx,ecx`) exactly as the original,
 *    instead of one function-wide zero register hoisted above the walk;
 *  - the gather loop's two collect sites (248..254 / 322..328): `count` is
 *    read-modify-written in its frame slot and the array store goes through
 *    the strength-reduced `edi` put pointer, `lea edi,[esp+ecx*4+0xc0]` at
 *    194 with the SAME displacement 0xc0 as the original;
 *  - the whole tail from 862 to the `ret` at 902: `mov edi,[count]` AFTER
 *    the three RenderPeople/RenderWorkers/DrawAndClearPrintList calls,
 *    `pop ebp` / `pop ebx` interleaved into the count test, `test`-against-
 *    self instead of `cmp reg,zeroreg`, `dec edi`, and `pop edi` / `pop esi`
 *    interleaved into the g_show_cursor test.  The function-wide zero
 *    register that used to pin ebx past DrawAndClearPrintList is gone.
 *
 * THE LEVER THAT DID IT: the ORDER OF THE INDEPENDENT STATEMENTS in the
 * tile-geometry block.  Not their spelling -- their order.  VC6 assigns the
 * callee-saved registers in the order the values are created, and the
 * assignment it makes for `sx` vs `tw` at index 64/73 propagates through
 * every later block: it decides which of the two idiv pairs gets a register
 * divisor, which of `count`/`px` keeps ebx in the gather loop, whether the
 * ODF walk gets a register for `cls`, and whether the tail needs a zero
 * register at all.  With `sx` in ebp and `tw` in ebx (the original's choice)
 * everything downstream falls out; with them swapped nothing does.
 *
 * The order that produces it -- found bya search over topological orders of the
 * block's dependence graph (16 statements, ~4000 legal orders; a random
 * sample of 1500 plus hill-climbing and simulated annealing, ~4000 compiles
 * in total, tooling in scratchpad/renderview/w/) -- is the one in the body
 * below:  sh, th, ylimit, sw, hh, sy, tw, xlimit, qy, ry, sx, hw, qx, rx,
 * tile.x, tile.y.  Note that `ylimit` and `xlimit` are pulled UP into the
 * middle of the block; leaving them after the quadrant switch (where a
 * reading of the disassembly would put them, since the original emits them
 * at 162..172) costs 162 mismatches -- VC6 schedules them back down on its
 * own, and having them in the block changes the register order for the
 * better.  Measured: best-with-them-inside 478, best-with-them-outside 640.
 *
 * Three smaller levers of the same family, all measured:
 *   - the SWITCH CASE ORDER of the quadrant nudge: `1, 3, 2, 4` beats the
 *     natural `1, 2, 3, 4` by 3 (VC6 lays the arms out in case order);
 *   - `key2 = ...;` before `bright = corner.right;` (6);
 *   - the field-fill order of the two BlitCtx tokens (5), and the
 *     `c->top / c->left / c->bottom / c->right` order of the second depth
 *     strip (1), and `count = 0;` before `cls = g_odf_head;` (2), and
 *     `g_sort_keys[..] = key2;` before `c = &g_sort_clips[..];` (6).
 *
 * THE FRAME.  The original's frame is 0x2f90 and ours was 0x2f8c: one
 * 4-byte slot short.  The missing slot is `key2`'s: the original gives it a
 * home of its own between `corner` and `sd`, and VC6 colour-shares ours with
 * a gather-loop slot because its live range is short.  Aggregate PINNING
 * fixes it -- putting `key2` in a struct beside ANY local whose live range
 * does not overlap it forces both to distinct homes.  Pinning it with a
 * local that DOES overlap (aleft, bright, key1) does nothing, which is the
 * test that identifies the mechanism.  `rowx` measured best (503 vs 511 for
 * `key`); see the comment on the `pin` struct in the body.
 *
 * FIRST DIVERGING INDEX: 64.
 *      orig  mov ebp,[g_scroll_x] / sar ebp,8 / mov ecx,[g_tile_sprites+eax*4]
 *            / mov esi,[g_scroll_y] / add esp,4
 *      ours  mov edx,[g_map] / mov ebp,[g_scroll_x] / add esp,4
 *            / mov ecx,[g_tile_sprites+eax*4] / sar ebp,8
 * i.e. the same instructions in a different SCHEDULE, because our `ylimit`
 * needs `g_map->view_h` here and the original's does not yet.  Registers are
 * already right (ebp = sx).  This is scheduling noise, not allocation.
 *
 * WHAT IS LEFT, measured on a register/offset-NORMALISED diff (296 of 903
 * slots differ; the streams are the same instructions in the same order
 * except in these runs):
 *   98..131   the quadrant switch.  The original computes the switch arms
 *             with `th` still in a register and folds `(th+1)>>1` into the
 *             gap between the two idiv pairs; ours reloads.  Permuting the
 *             four case bodies and the two guard forms did not close it.
 *   159..172  the ylimit/xlimit stores land in different frame slots
 *             (ours 0x48/0x24, the original 0x20/0x1c) -- see below.
 *   408..451  the two footprint-corner GetTileBounds calls: same
 *             instructions, different scheduling of the stores of key1 /
 *             aleft / key2 / bright.
 *   669..786  the sprite-emit block.  The ORIGINAL loads BOTH desc->dx and
 *             desc->dy into registers before halving either, then does the
 *             two `test/jge/neg/sar/neg/jmp/sar` sequences back to back and
 *             only then adds tb.left/tb.top; we finish at.x (including its
 *             store) before starting at.y, so our first halving fuses the
 *             flags of the preceding `add` into a `jns` where the original
 *             emits a separate `test`/`jge`.  Two-temp spellings
 *             (`hx = desc->dx; hy = desc->dy;` before the two HalfOffsets)
 *             reproduce the eager loads but cost 31 elsewhere; an inlined
 *             helper taking l/t/dx/dy did nothing.  THIS IS THE NEXT THING
 *             TO ATTACK: it is 120 slots, it is a real structural
 *             difference (not allocation), and the eager-load form is
 *             provably what the original does.
 *
 * FRAME SLOT MAP, ours vs the original (base = esp after `sub esp,0x2f90`;
 * a disassembly `[esp+N]` outside a call-argument run is base+N-16):
 *   ORIG th@0x00 rx@0x04 tile@0x08 tw@0x10 count@0x14 qx@0x18 xlimit@0x1c
 *        ylimit@0x20 .. view@0x34 cls@0x44 corner@0x48 bright@0x58 key2@0x5c
 *        sd@0x60 ctx@0x78/0x88 saved@0xa0 visible@0xb0
 *   OURS th@0x0c rx@0x00 tile@0x04 tw@0x18 count@0x1c ... xlimit@0x24
 *        ylimit@0x48 view@0x34 cls@0x44 corner@0x4c ... sd@0x5c
 *        saved@0x9c visible@0xac  (frame size now correct; 57 of 135
 *        single-operand [esp+N] references agree exactly, was 44)
 *
 * Ruled out this round, all measured (score = audit.py mismatch, baseline
 * 478 unless stated), so that they are not repeated:
 *   - five spellings of the ODF pre-render walk (do/while, for, one
 *     combined test, hoisted callback, hoisted ctx): all identical;
 *   - an explicit live-range split of `cls` into a second pointer: 642;
 *   - reading g_odf_head at the walk instead of at the top: 868;
 *   - `while (--count)` reusing the gather counter in the DoBuildEffects
 *     tail, `n = count` moved inside the `if`, `pp = visible` hoisted, and
 *     `*(volatile int*)&count` to stop the load being hoisted above the
 *     three flush calls: 503 / 503 / 652 / 717 (all worse -- the tail is
 *     already exact, leave it alone);
 *   - an explicit per-row `put = &visible[count]` induction pointer: 804
 *     (VC6's own strength reduction already produces the original's
 *     `lea edi,[esp+ecx*4+0xc0]` in the column-loop preheader);
 *   - making `count` address-taken by pinning it in a struct with `tile`
 *     (which IS address-taken, via RenderViewCellProbe): this DOES give the
 *     original's memory read-modify-write of `count`, but it kills the
 *     strength reduction that produces the `edi` put pointer, so the array
 *     store becomes `mov [esp+edx*4+0xc8],ecx` and the score goes to 655;
 *   - `Pos rowstart;` instead of the two ints rowx/rowy: 517;
 *   - pinning key2 with aleft / bright / key1 (overlapping live ranges): no
 *     extra slot, 530;
 *   - a full 1-opt sweep of every adjacent independent statement pair in the
 *     function (two passes, ~500 compiles): converged.
 * ------------------------------------------------------------------------- */
// WIP-FUNCTION: LEGOLAND 0x0045b180  (903/903 insns, mismatch=478; scheduling of the sprite-offset block)
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
    ylimit = g_map->view_h + view.bottom;
    sw = (short)(sh + sh);
    hh = (th + 1) >> 1;
    sy = (g_scroll_y >> 8) - hh;
    tw = sw;
    xlimit = tw + tw + view.right;
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
    case 3:
        if (rx < hw + 2 * (ry - th)) {
            tile.y++;
            rx += hw;
            ry -= hh;
        }
        break;
    case 2:
        if (rx >= hw + 2 * ry) {
            tile.y--;
            rx -= hw;
            ry += hh;
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
                    ctx_normal.obj = cell->obj;
                    ctx_normal.kind = 0x103;
                    ctx_normal.base = cell->base;
                    mode = desc->mode;
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

/* Project a tile's world-pixel corner onto the minimap.  The original adds the
 * scroll offset INTO the TileBounds (the stores back into tb.left/tb.top are
 * visible at every one of the seven call sites), so these take a pointer. */
static __inline int FullMapX(TileBounds* t, int scale_x)
{
    t->left += (g_scroll_x >> 8);
    return ((t->left - g_fm_ox) * scale_x) >> 16;
}

static __inline int FullMapY(TileBounds* t, int scale_y)
{
    t->top += (g_scroll_y >> 8);
    return (((t->top - g_fm_oy) * scale_y) >> 16) + g_fm_cy;
}

/* -------------------------------------------------------------------------
 * 0x004567a0 -- the overview-map draw callback (see the write-up above).
 *
 * audit.py: 1161/1161 instructions, 4201B vs 4225B, no ESCAPES, mismatch=1064.
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

// WIP-FUNCTION: LEGOLAND 0x004567a0  (1161/1161 insns, mismatch=881; frame now exact, one register decision at index 95)
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
    void*       cls;
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
    g_fm_cw = (short)(int)((float)g_fm_view.w * tw / g_fm_spanx + 1.0f);
    g_fm_ch = (short)(int)((float)g_fm_view.h * th / g_fm_spany + 1.0f);
    RenderBlock(0, 0, g_fm_view.w, g_fm_view.h, GetNearestColour(0, 0, 0));

    saved_oy = g_map->origin_y;
    saved_ox = g_map->origin_x;
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
            TileBoundsAt(x, y, &tb);
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
            tile.x = x;
            tile.y = y;
            GetTileBounds(&tile, &tb);
            off.x = def->dx;
            off.y = def->dy;
            HalfPos(&off);
            tb.top += off.y + (g_scroll_y >> 8);
            my = (int)((float)(tb.top - g_fm_oy) * fsy) + g_fm_cy;
            tb.left += off.x + (g_scroll_x >> 8);
            mx = (int)((float)(tb.left - g_fm_ox) * fsx);
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
    tobj = g_terrain_objects;
    tile.y = 0;
    tile.x = 0;
    GetTileBounds(&tile, &tb);
    mx = FullMapX(&tb, scale_x);
    my = FullMapY(&tb, scale_y);
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
        if (CellMarkTest(*chain)) {
            TileBoundsAt((c.base.x & ~7) + 4, (c.base.y & ~7) + 4, &tb);
            i = ((c.base.y >> 3) << 5) + (c.base.x >> 3);
            g_map_marks[i].x = FullMapX(&tb, scale_x);
            g_map_marks[i].y = FullMapY(&tb, scale_y);
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
            TileBoundsAt(bpos.x, bpos.y, &tb);
            tb.left += HalfOffset(sd.dx);
            tb.top += HalfOffset(sd.dy);
            g_fm_cw = s_lights->w;
            g_fm_ch = s_lights->h;
            PrintScaledSprite(s_lights,
                              FullMapX(&tb, scale_x),
                              FullMapY(&tb, scale_y),
                              g_fm_cw * scale_x >> 16,
                              g_fm_ch * scale_y >> 16);
            continue;
        }
        cls = def->ctx;
        if (cls == e_track || cls == e_track_h || cls == e_track_h0
            || cls == e_track_hp || cls == e_castle) {
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
                GetTileBounds(&tile, &tb);
                tb.top += HalfOffset(-(int)h0);
                x0 = FullMapX(&tb, scale_x);
                y0 = FullMapY(&tb, scale_y);
                GetTileBounds(&p1, &tb);
                tb.top += HalfOffset(-(int)h1);
                mx = FullMapX(&tb, scale_x);
                my = FullMapY(&tb, scale_y);
                if (cls != e_track_hp) {
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
                MoveToEx(hdc, x0, y0, 0);
                LineTo(hdc, mx, my);
                if (link) {
                    tile.x = tile.x - 0xa;
                    GetTileBounds(&tile, &tb);
                    tb.top += HalfOffset(-(int)h0);
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
            desc = cb(cls, bpos);
        } else {
            sd.sprite = def->sprite;
            sd.dx = def->dx;
            sd.dy = def->dy;
            desc = &sd;
        }
        if (desc == 0)
            continue;
        spr = (Sprite*)desc->sprite;
        if (spr == 0)
            continue;
        if (!(*(unsigned int*)((char*)spr + 0x10) & 0x8000)) {
            /* This site deliberately builds its Pos in the function-level
             * `tile` instead of going through TileBoundsAt: the inlined
             * helper would give it a pooled Pos of its own, and the original
             * has only THREE such pooled slots for the four sites in this
             * loop (its ROADS Pos and its ILF-loop Pos share one).  Demoting
             * any one of roads / single-sprite / ILF measures the same; this
             * is the 8 bytes that, with the by-value Cell above, makes the
             * frame 0xf8 exactly. */
            tile.x = bpos.x;
            tile.y = bpos.y;
            GetTileBounds(&tile, &tb);
            tb.left += HalfOffset(desc->dx);
            tb.top += HalfOffset(desc->dy);
            g_fm_cw = spr->w;
            g_fm_ch = spr->h;
            PrintScaledSprite(spr,
                              FullMapX(&tb, scale_x),
                              FullMapY(&tb, scale_y),
                              g_fm_cw * scale_x >> 16,
                              g_fm_ch * scale_y >> 16);
            continue;
        }
        ilf = *(ILFTable**)((char*)spr + 0x08);
        if (ilf->count <= 0)
            continue;
        i = 0;
        do {
            Sprite* layer;
            Pos     lo;

            ilf = *(ILFTable**)((char*)desc->sprite + 0x08);
            layer = ilf->sprites[i];
            lo.x = ilf->dx[i];
            lo.y = ilf->dy[i];
            TileBoundsAt(bpos.x, bpos.y, &tb);
            tb.left += HalfOffset(desc->dx) + HalfOffset(lo.x);
            tb.top += HalfOffset(desc->dy) + HalfOffset(lo.y);
            g_fm_ch = layer->h;
            g_fm_cw = layer->w;
            PrintScaledSprite(layer,
                              FullMapX(&tb, scale_x),
                              FullMapY(&tb, scale_y),
                              g_fm_cw * scale_x >> 16,
                              g_fm_ch * scale_y >> 16);
            i++;
        } while (i < (*(ILFTable**)((char*)desc->sprite + 0x08))->count);
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
