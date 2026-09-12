/* LEGOLAND -- the depth-sorted print list, the sprite print front-ends and
 * the translucent blit.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field offsets, global addresses and callee arg counts are load-bearing;
 * names are ours.
 *
 * ---------------------------------------------------------------------------
 * The print list
 * ---------------------------------------------------------------------------
 * Everything drawn in depth order during a frame is first bump-allocated as a
 * PrintNode out of the arena at 0x007cb600 (byte cursor g_printlist_x @
 * 0x0066b5a8) and threaded by InsertPrintItem (0x00485bd0) into the tree
 * rooted at 0x0066b5a4, keyed on KEY; DrawAndClearPrintList then walks the
 * threaded order (the +0x04 link), draws each node and resets the arena.
 *
 * Node layout (the first 0x20 bytes are common):
 *   +0x00 left        tree link (InsertPrintItem)
 *   +0x04 next        in-order link (the walk order)
 *   +0x08 key         depth
 *   +0x0c type        0 sprite, 1 clipped sprite, 4 sprite + callback,
 *                     0x2000 3D person (blokeai.c SortPerson)
 *   +0x10 ctx         the 12-byte BlitCtx token handed on to PrintSprite
 *                     (kind 0x100 = "none" when the caller passed NULL)
 *   +0x1c sprite      (person for type 0x2000)
 *   +0x20 x, +0x24 y
 * then per type:
 *   sprite / callback (0x3c B): +0x28 mode, +0x2c palette override,
 *     +0x30 frame override, +0x34 callback, +0x38 callback argument
 *   clipped (0x4c B): +0x28 clip rect (16 B), +0x38 mode, +0x3c palette,
 *     +0x40 frame
 *
 * The palette / frame overrides current at SORT time are captured into the
 * node and re-installed around the deferred PrintSprite, so a sprite queued
 * under an override is drawn with it.
 *
 * ---------------------------------------------------------------------------
 * PrintSprite modes and the hit token
 * ---------------------------------------------------------------------------
 * PrintSprite(s, x, y, mode, ctx): mode 0 goes through RenderSprite (the
 * DirectDraw / plain software blit), any other mode through RenderSpriteX
 * (the SoftPrint_XBltFast path -- 0xff000000 is the recolour used for
 * highlighted icons, see fpui.c / render2.c). An ILF sprite (flag 0x8000)
 * is a layer table {+4 count, +8 sprites[], +0xc dx[], +0x10 dy[]} whose
 * layers are drawn in order at (x + dx/2, y + dy/2), skipping layers whose
 * sprite carries flag 0x4000 (hidden). The blitter raises g_blit_hit
 * (0x007feb14) when the pointer is over the pixels it drew; PrintSprite then
 * copies the caller's BlitCtx into g_hit_ctx (0x004bdd00) unless it is the
 * "none" token 0x100 -- that is how the frame ends up knowing which gadget
 * is under the mouse.
 *
 * PrintSpriteEx draws a SpriteDesc {sprite, dx, dy, ?, mode, layer_mask}
 * centred: x += dx/2, y += dy/2, and for an ILF sprite draws only the
 * layers whose bit is set in the mask (bit i for layer i).
 * ------------------------------------------------------------------------- */
/* No legoland.h: every type this file needs is defined locally. */

/* ---- local types -------------------------------------------------------- */

/* The 12-byte render-context token (money.c / render2.c BlitCtx). */
typedef struct BlitCtx {
    int   kind;                 /* +0x00 */
    void* p;                    /* +0x04 */
    int   n;                    /* +0x08 */
} BlitCtx;

/* Bare 16-byte clip rectangle (sweep4.c ClipRect: 0x004bdea0 g_clip). */
typedef struct ClipRect {
    int left;                   /* +0x00 */
    int top;                    /* +0x04 */
    int right;                  /* +0x08 */
    int bottom;                 /* +0x0c */
} ClipRect;

typedef struct ImageRec {
    void*          lls;         /* +0x00 */
    void*          pal;         /* +0x04 */
    short          w;           /* +0x08 */
    short          h;           /* +0x0a */
    unsigned short refcount;    /* +0x0c */
    char           kind;        /* +0x0e */
    char           pad0f;       /* +0x0f */
    char*          name;        /* +0x10 */
    int            type;        /* +0x14  2/3 = animated */
} ImageRec;

/* ILF layer table hanging off an 0x8000 sprite's +0x08. */
typedef struct ILFTable {
    int    f00;                 /* +0x00 */
    int    count;               /* +0x04 */
    struct SpriteRec** sprites; /* +0x08 */
    int*   dx;                  /* +0x0c */
    int*   dy;                  /* +0x10 */
} ILFTable;

typedef struct DDSurface DDSurface;

/* IDirectDrawSurface vtable: +0x08 Release, +0x64 Lock, +0x6c Restore. */
typedef struct DDSurfaceVtbl {
    char pad00[0x08];                                              /* +0x00 */
    long(__stdcall* Release)(DDSurface*);                          /* +0x08 */
    char pad0c[0x64 - 0x0c];                                       /* +0x0c */
    long(__stdcall* Lock)(DDSurface*, void* rect, void* desc, unsigned long flags, void* ev); /* +0x64 */
    char pad68[4];                                                 /* +0x68 */
    long(__stdcall* Restore)(DDSurface*);                          /* +0x6c */
} DDSurfaceVtbl;

struct DDSurface {
    DDSurfaceVtbl* vtbl;                                           /* +0x00 */
};

typedef struct SpriteRec {
    struct SpriteRec* next;     /* +0x00 */
    DDSurface*     surface;     /* +0x04 */
    ImageRec*      image;       /* +0x08  ILFTable* when flags & 0x8000 */
    int            detail;      /* +0x0c */
    unsigned int   flags;       /* +0x10 */
    short          w;           /* +0x14 */
    short          h;           /* +0x16 */
    short          src_x;       /* +0x18 */
    short          src_y;       /* +0x1a */
    unsigned short refs;        /* +0x1c */
    short          pad1e;       /* +0x1e */
} SpriteRec;

/* DDSURFACEDESC -- 0x6c bytes; lPitch @+0x10, lpSurface @+0x24. */
typedef struct DDSurfaceDesc {
    unsigned long dwSize;               /* +0x00 */
    unsigned long dwFlags;              /* +0x04 */
    unsigned long dwHeight;             /* +0x08 */
    unsigned long dwWidth;              /* +0x0c */
    long          lPitch;               /* +0x10 */
    char          pad14[0x24 - 0x14];   /* +0x14 */
    void*         lpSurface;            /* +0x24 */
    char          pad28[0x6c - 0x28];   /* +0x28 */
} DDSurfaceDesc;

/* A locked-surface handle (spritemisc.c SpriteHandle): what GetSprite fills
 * and ReleaseSprite hands back. */
typedef struct SpriteHandle {
    int        pitch;           /* +0x00 */
    int        w;               /* +0x04 */
    int        h;               /* +0x08 */
    void*      pixels;          /* +0x0c */
    DDSurface* surface;         /* +0x10 */
    int        bpp;             /* +0x14  1 = 8-bit, 2 = 16-bit */
} SpriteHandle;

/* The map header with the screen size at +0x00/+0x02 (legoland.h's Map only
 * names width/height at +0x14/+0x16). */
typedef struct MapHdr {
    unsigned short screen_w;    /* +0x00 */
    unsigned short screen_h;    /* +0x02 */
    char           pad04[0x10]; /* +0x04 */
    unsigned short width;       /* +0x14 */
    unsigned short height;      /* +0x16 */
} MapHdr;

/* A print-list node (see the header comment). */
typedef struct PrintNode {
    struct PrintNode* left;     /* +0x00 */
    struct PrintNode* next;     /* +0x04 */
    int      key;               /* +0x08 */
    int      type;              /* +0x0c */
    BlitCtx  ctx;               /* +0x10 */
    void*    sprite;            /* +0x1c */
    int      x;                 /* +0x20 */
    int      y;                 /* +0x24 */
    union {
        struct {
            int   mode;         /* +0x28 */
            void* pal;          /* +0x2c */
            int   frame;        /* +0x30 */
            void (*cb)(int);    /* +0x34 */
            int   cbarg;        /* +0x38 */
        } s;
        struct {
            ClipRect clip;      /* +0x28 */
            int   mode;         /* +0x38 */
            void* pal;          /* +0x3c */
            int   frame;        /* +0x40 */
        } c;
    } u;
} PrintNode;

/* The gadget-owner object a 0x103 BlitCtx names: slot +0xb0 of its vtable is
 * the "draw over" hook called after the sprite for 0x2000-flagged sprites. */
typedef struct HitOwnerVtbl {
    char pad00[0xb0];
    void (*DrawOver)(struct HitOwner* self, int x, int y, int* n, ClipRect* clip, int mode);
} HitOwnerVtbl;

typedef struct HitOwner {
    char          pad00[0x0c];
    HitOwnerVtbl* vtbl;         /* +0x0c */
} HitOwner;

/* What PrintSpriteEx draws. */
typedef struct SpriteDesc {
    SpriteRec* sprite;          /* +0x00 */
    int        dx;              /* +0x04 */
    int        dy;              /* +0x08 */
    int        f0c;             /* +0x0c */
    int        mode;            /* +0x10 */
    int        layer_mask;      /* +0x14 */
} SpriteDesc;

/* A bloke as CheckForPeople sees it: flags @+0x62, 8.8 fixed position. */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00 */
    unsigned char  pad04[0x5e]; /* +0x04..0x61 */
    unsigned char  flags62;     /* +0x62  0x20 = in a vehicle / not drawn */
    unsigned char  pad63[5];    /* +0x63..0x67 */
    int            fx;          /* +0x68  x << 8 */
    int            fy;          /* +0x6c  y << 8 */
} Bloke;

/* A map cell (legoland.h Cell): the map flags word at +0x0c. */
typedef struct Cell {
    char           pad00[0x0c]; /* +0x00 */
    unsigned short flags;       /* +0x0c  0x1000 = a person stands here */
    char           pad0e[6];    /* +0x0e */
} Cell;

/* A footprint rectangle in cells (inclusive). */
typedef struct CellRect {
    int left;                   /* +0x00 */
    int top;                    /* +0x04 */
    int right;                  /* +0x08 */
    int bottom;                 /* +0x0c */
} CellRect;

/* ---- globals ------------------------------------------------------------ */

extern unsigned char g_printlist_arena[];   /* 0x007cb600 */
extern int           g_printlist_x;         /* 0x0066b5a8 */
extern PrintNode*    g_printlist;           /* 0x0066b5a4 */
extern int           g_printlist_drawn;     /* 0x0066b5ac */

extern SpriteRec*    g_sprites_head;        /* 0x0079a7c0 */
extern MapHdr*       g_maphdr;              /* 0x004bcbf4 (legoland.h g_map) */
extern Bloke*        g_people_head;         /* 0x0066b574 */
extern Cell**        g_map_rows;            /* 0x00801400 */

extern BlitCtx       g_hit_ctx;             /* 0x004bdd00 */
extern int           g_blit_hit;            /* 0x007feb14 */
/* blitmisc.c:118's own name for 0x00668078 -- the surface it Restores and
 * polls with GetFlipStatus(DDGFS_ISFLIPDONE), one slot past g_primary
 * (0x00668070) and one before g_draw_surface (0x0066807c), both of which this
 * file's siblings also declare.  It was spelled `g_screen` here until
 * PORT-M15; that name belongs to the config pointer at 0x004bcbf4 (the export
 * table's `lpConfig`), which five other files declare under it. */
extern DDSurface*    g_surface_78;           /* 0x00668078 */
extern int           g_screen_depth;        /* 0x00668088 */
extern ClipRect      g_clip;                /* 0x004bdea0 */

extern const char    g_msg_bad_clip[];      /* 0x004bdd0c */

/* ---- externals ---------------------------------------------------------- */

extern void  InsertPrintItem(PrintNode* it);                 /* 0x00485bd0 */
extern void* GetOverridePalette(void);                       /* 0x00464410 */
extern int   GetOverrideFrame(void);                         /* 0x00464430 */
extern void  SetOverridePalette(void* pal);                  /* 0x00464400 */
extern void  SetOverrideFrame(int frame);                    /* 0x00464420 */
extern void  ClearSpriteOverrides(void);                     /* 0x00464460 */
extern void  GetClipping(ClipRect* out);                     /* 0x0048a630 */
extern void  SetClipping(ClipRect* r);                       /* 0x0048a5c0 */
extern void  Render3DPerson(void* person);                   /* 0x0043fe50 */
extern int   printf(const char* fmt, ...);                   /* 0x0049e5c5 */

extern void* GetVRAMAddress(void* p);                        /* 0x00496f20 */
extern int   MakeSpriteDrawable(SpriteRec* s);               /* 0x00499500 */
extern int   RenderSprite(SpriteRec* s, int x, int y);       /* 0x00488a10 */
extern int   RenderSpriteX(SpriteRec* s, int x, int y, int mode); /* 0x00488b90 */
extern void  MarkSpriteResized(SpriteRec* s);                /* 0x00497150 */
extern int   ReleaseSprite(SpriteHandle* h);                 /* 0x00497dc0 */

extern int   LLSStop(void* lls);                             /* 0x0047d4c0 */
extern void  LLSPlay(void* lls, void* owner);                /* 0x0047d520 */
extern void  HeapFree_w(void* p);                            /* 0x0049e4d0 */
extern void  FreeBitmapResources(ImageRec* image);           /* 0x00497610 */
extern int   __BMPLoader(ImageRec* image);                   /* 0x0044e010 */

extern void  MarkWorkersOnMap(CellRect* r);                  /* 0x0049cf00 (ignores r) */
extern int   PrintSprite(SpriteRec* s, int x, int y, int mode, BlitCtx* ctx);

/* -------------------------------------------------------------------------
 * 0x00485d70 -- queue a sprite on the print list.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00485d70
void SortSprite(void* sprite, int x, int y, int key, int mode, BlitCtx* ctx)
{
    int        off = g_printlist_x;
    PrintNode* it;

    g_printlist_x += 0x3c;
    it = (PrintNode*)(g_printlist_arena + off);
    it->key = key;
    it->sprite = sprite;
    it->type = 0;
    it->x = x;
    it->y = y;
    it->u.s.mode = mode;
    it->u.s.pal = GetOverridePalette();
    it->u.s.frame = GetOverrideFrame();
    if (ctx)
        it->ctx = *ctx;
    else
        it->ctx.kind = 0x100;
    InsertPrintItem(it);
}

/* -------------------------------------------------------------------------
 * 0x00485cd0 -- queue a sprite plus a callback run after it is drawn.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00485cd0
void SortSpriteWithCallback(void* sprite, int x, int y, int key, int mode,
                            void (*cb)(int), int cbarg, BlitCtx* ctx)
{
    int        off = g_printlist_x;
    PrintNode* it;

    g_printlist_x += 0x3c;
    it = (PrintNode*)(g_printlist_arena + off);
    it->key = key;
    it->sprite = sprite;
    it->x = x;
    it->y = y;
    it->type = 4;
    it->u.s.mode = mode;
    it->u.s.cb = cb;
    it->u.s.cbarg = cbarg;
    it->u.s.pal = GetOverridePalette();
    it->u.s.frame = GetOverrideFrame();
    if (ctx)
        it->ctx = *ctx;
    else
        it->ctx.kind = 0x100;
    InsertPrintItem(it);
}

/* -------------------------------------------------------------------------
 * 0x00485e40 -- queue a sprite drawn under its own clip rectangle.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00485e40
void SortClippedSprite(void* sprite, int x, int y, int key, ClipRect* clip,
                       int mode, BlitCtx* ctx)
{
    int        off = g_printlist_x;
    PrintNode* it;

    g_printlist_x += 0x4c;
    it = (PrintNode*)(g_printlist_arena + off);
    it->key = key;
    it->sprite = sprite;
    it->type = 1;
    it->x = x;
    it->y = y;
    it->u.c.clip = *clip;
    it->u.c.mode = mode;
    it->u.c.pal = GetOverridePalette();
    it->u.c.frame = GetOverrideFrame();
    if (ctx)
        it->ctx = *ctx;
    else
        it->ctx.kind = 0x100;
    InsertPrintItem(it);
}

/* -------------------------------------------------------------------------
 * Halve a sprite's source rectangle (the detail level dropped) and clamp it
 * to its image. Shared by RemakeAllDetailDependentSprites and
 * ReloadImageBitmapAndBuildSprites.
 * ------------------------------------------------------------------------- */

static __inline void HalveSpriteRect(SpriteRec* s)
{
    s->src_x >>= 1;
    s->w >>= 1;
    s->h >>= 1;
    s->src_y >>= 1;
    if (s->src_x < 0) {
        s->w += s->src_x;
        s->src_x = 0;
    }
    if (s->src_x + s->w > s->image->w) {
        if (s->src_x > s->image->w - 1)
            s->src_x = s->image->w - 1;
        s->w = s->image->w - s->src_x;
    }
    if (s->src_y < 0) {
        s->h += s->src_y;
        s->src_y = 0;
    }
    if (s->src_y + s->h > s->image->h) {
        if (s->src_y > s->image->h - 1)
            s->src_y = s->image->h - 1;
        s->h = s->image->h - s->src_y;
    }
}

/* -------------------------------------------------------------------------
 * 0x00497160 -- the detail level dropped: release every not-yet-resized
 * sprite's surface, then halve and rebuild each one.
 * ------------------------------------------------------------------------- */

/* The first pass lives in an inline helper: as one flat function VC6 sinks
 * the 0x400 mask load into the first loop's preheader (after the entry je);
 * the original materialises it in the entry block. */
static __inline SpriteRec* ReleaseUnresizedSurfaces(SpriteRec* s)
{
    SpriteRec* p;
    for (p = s; p; p = p->next) {
        if (!(p->flags & 0x400) && p->surface) {
            p->surface->vtbl->Release(p->surface);
            s = g_sprites_head;
        }
    }
    return s;
}

// FUNCTION: LEGOLAND 0x00497160
void RemakeAllDetailDependentSprites(void)
{
    SpriteRec* s = ReleaseUnresizedSurfaces(g_sprites_head);

    for (; s; s = s->next) {
        if (!(s->flags & 0x400)) {
            HalveSpriteRect(s);
            if (s->surface) {
                s->surface = 0;
                MakeSpriteDrawable(s);
            }
        }
        MarkSpriteResized(s);
    }
}

/* -------------------------------------------------------------------------
 * 0x00485260 -- is anybody standing inside a footprint? Clears the "person
 * here" cell flag (0x1000) over the rect, re-marks it from every walking
 * bloke (noting whether one fell inside the rect), then marks the workers
 * and re-scans the rect. Returns 1 for a visitor inside, -1 for a worker
 * inside, 0 for nobody.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00485260
int CheckForPeople(CellRect* r)
{
    Bloke* b = g_people_head;
    int    found = 0;
    int    x;
    int    y;

    for (y = r->top; y <= r->bottom; y++)
        for (x = r->left; x <= r->right; x++)
            g_map_rows[y][x].flags &= ~0x1000;

    while (b) {
        if (!(b->flags62 & 0x20)) {
            x = b->fx >> 8;
            y = b->fy >> 8;
            if (x >= 0 && x < g_maphdr->width && y >= 0 && y < g_maphdr->height) {
                g_map_rows[y][x].flags |= 0x1000;
                if (x >= r->left && x <= r->right && y >= r->top && y <= r->bottom)
                    found = 1;
            }
        }
        b = b->next;
    }
    if (found)
        return 1;

    MarkWorkersOnMap(r);
    for (y = r->top; y <= r->bottom; y++)
        for (x = r->left; x <= r->right; x++)
            if (g_map_rows[y][x].flags & 0x1000)
                return -1;
    return 0;
}

/* -------------------------------------------------------------------------
 * 0x00497c30 -- lock a sprite's surface (or the screen when s is NULL) and
 * describe the pixels in a SpriteHandle. Retries once through Restore on
 * DDERR_SURFACELOST.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497c30
int GetSprite(SpriteHandle* out, SpriteRec* s)
{
    ClipRect      rect;
    DDSurfaceDesc desc;
    DDSurface*    surf;
    long          rc;
    int           w;
    int           h;

    rect.left = 0;
    rect.top = 0;
    rect.right = 0;
    rect.bottom = 0;
    desc.dwSize = sizeof(desc);
    if (s) {
        rect.right = s->w;
        out->w = s->w;
        rect.bottom = s->h;
        out->h = s->h;
        out->surface = s->surface;
        surf = s->surface;
        rc = surf->vtbl->Lock(surf, &rect, &desc, 1, 0);
        if (rc == 0x887601c2) {
            surf = s->surface;
            surf->vtbl->Restore(surf);
            surf = s->surface;
            rc = surf->vtbl->Lock(surf, &rect, &desc, 1, 0);
        }
        if (rc != 0)
            return 0;
    } else {
        w = g_maphdr->screen_w;
        out->w = w;
        rect.right = w;
        h = g_maphdr->screen_h;
        out->h = h;
        rect.bottom = h;
        out->surface = g_surface_78;
        surf = g_surface_78;
        rc = surf->vtbl->Lock(surf, &rect, &desc, 1, 0);
        if (rc == 0x887601c2) {
            surf = g_surface_78;
            surf->vtbl->Restore(surf);
            surf = g_surface_78;
            rc = surf->vtbl->Lock(surf, &rect, &desc, 1, 0);
        }
        if (rc != 0)
            return 0;
    }
    out->pitch = desc.lPitch;
    out->pixels = desc.lpSurface;
    if (g_screen_depth == 0)
        out->bpp = 1;
    else
        out->bpp = 2;
    return 1;
}

/* -------------------------------------------------------------------------
 * 0x00497380 -- re-decode an image's bitmap (after a detail change) and
 * rebuild the sprite cut from it. An animating image keeps its LLS state
 * across the reload: the 24-byte header is saved, and the frame counter,
 * the +0x12 word and the "reversed" bit (4) of +0x14 are put back before
 * LLSPlay restarts it.
 * ------------------------------------------------------------------------- */

typedef struct LLSHdr {
    unsigned short frame;       /* +0x00 */
    char           pad02[0x10]; /* +0x02 */
    unsigned short f12;         /* +0x12 */
    unsigned int   flags;       /* +0x14 */
} LLSHdr;

// FUNCTION: LEGOLAND 0x00497380
int ReloadImageBitmapAndBuildSprites(ImageRec* img)
{
    LLSHdr     saved;
    int        animating = 0;
    SpriteRec* s;
    LLSHdr*    lls;

    /* `animating = 1` must precede the header copy: overlapping the rep
     * movsd puts the flag in ebp (not esi) so all four callee-saved
     * registers are pushed anyway, and VC6 then hoists a zero into the
     * free edi (cmp eax,edi / cmp ax,di / mov [esi+18h],di) instead of
     * spelling test/imm0. Set after the copy, the flag lands in esi and
     * no zero register is allocated. */
    if (img->type == 2 || img->type == 3) {
        if (LLSStop(img->lls)) {
            animating = 1;
            saved = *(LLSHdr*)img->lls;
        }
    }
    if (img->pal)
        HeapFree_w(img->pal);
    FreeBitmapResources(img);
    if (!__BMPLoader(img))
        return 0;
    if (animating) {
        lls = (LLSHdr*)img->lls;
        lls->frame = saved.frame;
        lls->f12 = saved.f12;
        lls->flags |= saved.flags & 4;
        LLSPlay(lls, img);
    }
    s = g_sprites_head;
    while (s->image != img)
        s = s->next;
    if (!(s->flags & 0x400) && s->image == img) {
        HalveSpriteRect(s);
        MakeSpriteDrawable(s);
        MarkSpriteResized(s);
    }
    return 1;
}

/* -------------------------------------------------------------------------
 * 0x004859d0 -- draw the frame's print list in order, then reset it.
 *
 * The type-4 callback in the CLIPPED arm is invoked through `cur`, the node
 * pointer the PLAIN arm last stored, not through the clipped node itself
 * (the original reads it back from the stack slot the plain arm spilled it
 * to). Reproduced, not fixed: a clipped node is never sorted with type 4 so
 * the path is dead in practice.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004859d0
void DrawAndClearPrintList(void)
{
    PrintNode* p = g_printlist;
    PrintNode* cur;
    void*      pal;
    int        frame;
    ClipRect   clip;
    HitOwner*  owner;

    pal = GetOverridePalette();
    frame = GetOverrideFrame();
    g_printlist_drawn = 0;
    while (p) {
        if (p->type & 1) {
            if (p->u.c.clip.left < 0 || p->u.c.clip.right > g_maphdr->screen_w ||
                p->u.c.clip.top < 0 || p->u.c.clip.bottom > g_maphdr->screen_h)
                printf(g_msg_bad_clip);
            GetClipping(&clip);
            SetClipping(&p->u.c.clip);
            if (p->sprite) {
                SetOverridePalette(p->u.c.pal);
                SetOverrideFrame(p->u.c.frame);
                PrintSprite(p->sprite, p->x, p->y, p->u.c.mode, &p->ctx);
                ClearSpriteOverrides();
                if (p->ctx.kind == 0x103) {
                    owner = (HitOwner*)p->ctx.p;
                    if (owner && (((SpriteRec*)p->sprite)->flags & 0x2000))
                        owner->vtbl->DrawOver(owner, p->x, p->y, &p->ctx.n,
                                              &p->u.c.clip, p->u.c.mode);
                }
            }
            if (p->type & 4)
                cur->u.s.cb(cur->u.s.cbarg);
            SetClipping(&clip);
        } else if (p->type & 0x2000) {
            Render3DPerson(p->sprite);
        } else {
            cur = p;
            if (cur->sprite) {
                SetOverridePalette(cur->u.s.pal);
                SetOverrideFrame(cur->u.s.frame);
                PrintSprite(cur->sprite, cur->x, cur->y, cur->u.s.mode, &cur->ctx);
                ClearSpriteOverrides();
                if (cur->ctx.kind == 0x103) {
                    owner = (HitOwner*)cur->ctx.p;
                    if (owner && (((SpriteRec*)cur->sprite)->flags & 0x2000))
                        owner->vtbl->DrawOver(owner, cur->x, cur->y, &cur->ctx.n,
                                              0, cur->u.s.mode);
                }
                if (cur->type & 4)
                    cur->u.s.cb(cur->u.s.cbarg);
            }
        }
        p = p->next;
    }
    g_printlist_x = 0;
    g_printlist = 0;
    SetOverridePalette(pal);
    SetOverrideFrame(frame);
}

/* Signed halve as the original spells it (test / neg / sar / neg). */
static __inline int HalfOf(int v)
{
    if (v < 0)
        return -((-v) >> 1);
    return v >> 1;
}

/* -------------------------------------------------------------------------
 * 0x004853a0 -- the sprite print front-end (see the header comment).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004853a0
int PrintSprite(SpriteRec* s, int x, int y, int mode, BlitCtx* ctx)
{
    int        rc = 1;
    ILFTable*  t;
    SpriteRec* layer;
    int        i;
    int        dx;
    int        dy;

    g_blit_hit = 0;
    if (!(s->flags & 0x8000)) {
        if (mode) {
            if (*(void**)GetVRAMAddress(s) == 0) {
                if (MakeSpriteDrawable(s))
                    rc = RenderSpriteX(s, x, y, mode);
                else
                    rc = 0;
            } else {
                rc = RenderSpriteX(s, x, y, mode);
            }
        } else {
            if (*(void**)GetVRAMAddress(s) == 0) {
                if (MakeSpriteDrawable(s))
                    rc = RenderSprite(s, x, y);
                else
                    rc = 0;
            } else {
                rc = RenderSprite(s, x, y);
            }
        }
    } else {
        for (i = 0; i < ((ILFTable*)s->image)->count; i++) {
            t = (ILFTable*)s->image;
            layer = t->sprites[i];
            if (layer->flags & 0x4000)
                continue;
            dx = t->dx[i];
            dy = t->dy[i];
            if (dx < 0)
                dx = -((-dx) >> 1);
            else
                dx >>= 1;
            if (dy < 0)
                dy = -((-dy) >> 1);
            else
                dy >>= 1;
            if (mode) {
                if (*(void**)GetVRAMAddress(layer) == 0) {
                    if (!MakeSpriteDrawable(((ILFTable*)s->image)->sprites[i]))
                        continue;
                }
                RenderSpriteX(((ILFTable*)s->image)->sprites[i], x + dx, y + dy, mode);
            } else {
                if (*(void**)GetVRAMAddress(layer) == 0) {
                    if (MakeSpriteDrawable(((ILFTable*)s->image)->sprites[i]))
                        RenderSprite(((ILFTable*)s->image)->sprites[i], x + dx, y + dy);
                } else {
                    RenderSprite(((ILFTable*)s->image)->sprites[i], x + dx, y + dy);
                }
            }
        }
    }
    if (ctx && g_blit_hit && ctx->kind != 0x100)
        g_hit_ctx = *ctx;
    return rc;
}

/* -------------------------------------------------------------------------
 * 0x004856a0 -- print a SpriteDesc centred on (x, y) with a layer mask.
 *
 * Same shape as PrintSprite: one `rc` and one trailing `return rc`. VC6 then
 * tail-duplicates the epilogue into every arm (six copies) rather than
 * spilling rc, which is what puts `mov ecx,1` up in the prologue: rc's
 * initial value is what the "no layers" arm returns. Spelling the arms as
 * direct `return Render...()` calls merges the two ILF exits instead.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004856a0
int PrintSpriteEx(SpriteDesc* d, int x, int y)
{
    int        rc = 1;
    SpriteRec* s;
    ILFTable*  t;
    int        mask;
    int        i;
    int        dx;
    int        dy;

    x += d->dx / 2;
    y += d->dy / 2;
    s = d->sprite;
    if (!(s->flags & 0x8000)) {
        if (d->mode) {
            if (*(void**)GetVRAMAddress(s) == 0) {
                if (MakeSpriteDrawable(s))
                    rc = RenderSpriteX(s, x, y, d->mode);
                else
                    rc = 0;
            } else {
                rc = RenderSpriteX(s, x, y, d->mode);
            }
        } else {
            if (*(void**)GetVRAMAddress(s) == 0) {
                if (MakeSpriteDrawable(s))
                    rc = RenderSprite(s, x, y);
                else
                    rc = 0;
            } else {
                rc = RenderSprite(s, x, y);
            }
        }
    } else {
        mask = d->layer_mask;
        i = 0;
        t = (ILFTable*)s->image;
        if (t->count > 0) {
            do {
                if (mask & 1) {
                    dx = t->dx[i];
                    dy = t->dy[i];
                    if (dx < 0)
                        dx = -((-dx) >> 1);
                    else
                        dx >>= 1;
                    if (dy < 0)
                        dy = -((-dy) >> 1);
                    else
                        dy >>= 1;
                    if (d->mode) {
                        if (*(void**)GetVRAMAddress(t->sprites[i]) == 0) {
                            if (MakeSpriteDrawable(((ILFTable*)s->image)->sprites[i]))
                                RenderSpriteX(((ILFTable*)s->image)->sprites[i],
                                              x + dx, y + dy, d->mode);
                        } else {
                            RenderSpriteX(((ILFTable*)s->image)->sprites[i],
                                          x + dx, y + dy, d->mode);
                        }
                    } else {
                        if (*(void**)GetVRAMAddress(t->sprites[i]) == 0) {
                            if (MakeSpriteDrawable(((ILFTable*)s->image)->sprites[i]))
                                RenderSprite(((ILFTable*)s->image)->sprites[i],
                                             x + dx, y + dy);
                        } else {
                            RenderSprite(((ILFTable*)s->image)->sprites[i],
                                         x + dx, y + dy);
                        }
                    }
                }
                i++;
                mask >>= 1;
                t = (ILFTable*)s->image;
            } while (i < t->count);
        }
    }
    return rc;
}
