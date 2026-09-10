/* LEGOLAND -- sprite / image creation and reload.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field offsets, global addresses and callee arg counts are load-bearing;
 * names are ours.
 *
 * ---------------------------------------------------------------------------
 * The three records
 * ---------------------------------------------------------------------------
 * SpriteRec (0x20 bytes, allocated by NewSprite 0x00497580 which links it at
 * the head of the global sprite list @ 0x0079a7c0):
 *
 *   +0x00 next      list link
 *   +0x04 surface   IDirectDrawSurface* holding the pixels (the pointer whose
 *                   ADDRESS GetVRAMAddress hands back); 0 = not resident
 *   +0x08 image     ImageRec* (source bitmap) / draw callback (flag 0x20) /
 *                   ILF layer table (flag 0x8000)
 *   +0x0c detail    g_detail - 1 at creation: the detail level the sprite was
 *                   built for
 *   +0x10 flags     0x10 = system-memory surface, 0x20 = function-drawn,
 *                   0x80 = magenta colour key, 0x400 = needs resizing,
 *                   0x8000 = ILF (layered)
 *   +0x14 w, +0x16 h            (short)
 *   +0x18 src_x, +0x1a src_y    (short) sub-rectangle origin in the image
 *   +0x1c refs      (u16) reference count
 *
 * ImageRec (0x18-byte header, the name copied inline after it):
 *
 *   +0x00 lls       decoded bitmap (an LLS record: LLSStop'able for type 2/3)
 *   +0x04 pal       secondary buffer (palette), freed with the record
 *   +0x08 w, +0x0a h            (short) from the BMP header
 *   +0x0c refcount  (u16)
 *   +0x0e kind      (u8) 1 = detail-dependent: registered in the table
 *                   @ 0x0079a7c4 (cap @ 0x0079a7c8, count @ 0x0079a7cc)
 *   +0x10 name      -> +0x18
 *   +0x14 type      2/3 = animated
 *
 * ILF layer table (sprite +0x08 when flag 0x8000; the 36-byte descriptor
 * LLIDB_LoadILFData builds): +0x04 count, +0x08 sprites[], +0x0c dx[],
 * +0x10 dy[].
 * ------------------------------------------------------------------------- */

char*        strcpy(char*, const char*);
unsigned int strlen(const char*);
#pragma intrinsic(strcpy, strlen)

/* ---- local types -------------------------------------------------------- */

typedef struct DDSurface DDSurface;

/* IDirectDrawSurface vtable: +0x08 Release, +0x74 SetColorKey. */
typedef struct DDSurfaceVtbl {
    char pad00[0x08];                                             /* +0x00 */
    long(__stdcall* Release)(DDSurface*);                         /* +0x08 */
    char pad0c[0x74 - 0x0c];                                      /* +0x0c */
    long(__stdcall* SetColorKey)(DDSurface*, unsigned long, void*); /* +0x74 */
} DDSurfaceVtbl;

struct DDSurface {
    DDSurfaceVtbl* vtbl;                                          /* +0x00 */
};

/* DDSURFACEDESC -- 0x6c bytes; dwSize/dwFlags/dwHeight/dwWidth and
 * ddsCaps.dwCaps (+0x68) are the only fields RecreateSprite fills. */
typedef struct DDSurfaceDesc {
    unsigned long dwSize;              /* +0x00 */
    unsigned long dwFlags;             /* +0x04 */
    unsigned long dwHeight;            /* +0x08 */
    unsigned long dwWidth;             /* +0x0c */
    char          pad10[0x68 - 0x10];  /* +0x10 */
    unsigned long dwCaps;              /* +0x68 ddsCaps.dwCaps */
} DDSurfaceDesc;

typedef struct DDColorKey {
    unsigned long low;                 /* +0x00 */
    unsigned long high;                /* +0x04 */
} DDColorKey;

typedef struct DDraw DDraw;

/* IDirectDraw vtable: +0x0c Compact, +0x18 CreateSurface. */
typedef struct DDrawVtbl {
    char pad00[0x0c];                                             /* +0x00 */
    long(__stdcall* Compact)(DDraw*);                             /* +0x0c */
    char pad10[0x18 - 0x10];                                      /* +0x10 */
    long(__stdcall* CreateSurface)(DDraw*, DDSurfaceDesc*, DDSurface**, void*); /* +0x18 */
} DDrawVtbl;

struct DDraw {
    DDrawVtbl* vtbl;                                              /* +0x00 */
};

typedef struct ImageRec {
    void*          lls;        /* +0x00 */
    void*          pal;        /* +0x04 */
    short          w;          /* +0x08 */
    short          h;          /* +0x0a */
    unsigned short refcount;   /* +0x0c */
    char           kind;       /* +0x0e */
    char           pad0f;      /* +0x0f */
    char*          name;       /* +0x10 */
    int            type;       /* +0x14 */
    /* +0x18 name text follows */
} ImageRec;

typedef struct ILFTable {
    int    f00;                /* +0x00 */
    int    count;              /* +0x04 */
    void** sprites;            /* +0x08 */
    int*   dx;                 /* +0x0c */
    int*   dy;                 /* +0x10 */
} ILFTable;

typedef struct SpriteRec SpriteRec;
typedef void (*SpriteDrawFn)(SpriteRec* s);

struct SpriteRec {
    SpriteRec*     next;       /* +0x00 */
    DDSurface*     surface;    /* +0x04 */
    ImageRec*      image;      /* +0x08 */
    int            detail;     /* +0x0c */
    unsigned int   flags;      /* +0x10 */
    short          w;          /* +0x14 */
    short          h;          /* +0x16 */
    short          src_x;      /* +0x18 */
    short          src_y;      /* +0x1a */
    unsigned short refs;       /* +0x1c */
    short          pad1e;      /* +0x1e */
};

/* What GetLayer fills in for one layer. */
typedef struct LayerOut {
    void* sprite;              /* +0x00 */
    int   dx;                  /* +0x04 */
    int   dy;                  /* +0x08 */
} LayerOut;

/* ---- globals ------------------------------------------------------------ */

extern SpriteRec* g_sprites_head;     /* 0x0079a7c0 */
extern ImageRec** g_detail_images;    /* 0x0079a7c4 */
extern int        g_detail_images_cap;   /* 0x0079a7c8 */
extern int        g_detail_images_count; /* 0x0079a7cc */
extern int        g_detail;           /* 0x008119a4 */
extern DDraw*     g_ddraw;            /* 0x00667d74 */
extern const char g_ext_csp[];        /* 0x004bfed8 ".csp" */

/* ---- externals ---------------------------------------------------------- */

extern void* HeapAlloc_w(unsigned int size);              /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                         /* 0x0049e4d0 */
extern void  _splitpath(const char* path, char* drive, char* dir,
                        char* fname, char* ext);          /* 0x0049ec85 */
extern int   _stricmp(const char* a, const char* b);      /* 0x004aab90 */

extern int   LLSStop(void* lls);                          /* 0x0047d4c0 */
extern int   __BMPLoader(ImageRec* image);                /* 0x0044e010 */
extern SpriteRec* NewSprite(void);                        /* 0x00497580 */
extern int   KillSprite(SpriteRec* s);                    /* 0x00497bd0 */
extern int   MakeSpriteDrawable(SpriteRec* s);            /* 0x00499500 */
extern unsigned short ReferenceImage(ImageRec* p);        /* 0x00497500 */
#ifndef LEGOLAND_PORTABLE
extern void  RegisterDetailImage(ImageRec* p);            /* 0x00496fc0 */
#else
extern int RegisterDetailImage(ImageRec* p);            /* 0x00496fc0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void  UnregisterDetailImage(ImageRec* p);          /* 0x00497020 */
#else
extern int UnregisterDetailImage(ImageRec* p);          /* 0x00497020 */
#endif
extern int   ReloadImageBitmapAndBuildSprites(ImageRec* p); /* 0x00497380 */
extern int   LoadCSPSprite(SpriteRec* s, const char* name, int kind); /* 0x004978b0 */
extern void  SetLayerAnimatingState(SpriteRec* s, int layer, int state); /* 0x00497ed0 */
extern void  PushSetTarget(SpriteRec* s);                 /* 0x00466560 */
extern void  PopTarget(void);                             /* 0x00466600 */
extern int   GetNearestColour(int r, int g, int b);       /* 0x0044e6c0 */
extern int   GetTransparentColour(void);                  /* 0x0044e690 */

/* -------------------------------------------------------------------------
 * 0x00497610 -- stop and free an image's decoded bitmap.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497610
void FreeBitmapResources(ImageRec* image)
{
    if (image->lls) {
        LLSStop(image->lls);
        HeapFree_w(image->lls);
        image->lls = 0;
    }
}

/* -------------------------------------------------------------------------
 * 0x004976c0 -- a sprite drawn by a callback (flags 0x30: sysmem + function).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004976c0
SpriteRec* CreateFunctionBasedSprite(SpriteDrawFn fn, short w, short h)
{
    SpriteRec* s = NewSprite();

    if (!s)
        return 0;

    s->surface = 0;
    s->image = (ImageRec*)fn;
    s->refs = 1;
    s->src_x = 0;
    s->src_y = 0;
    s->flags = 0x30;
    s->detail = g_detail - 1;
    s->w = w;
    s->h = h;
    return s;
}

/* -------------------------------------------------------------------------
 * 0x00497110 -- flag every sprite whose image is NOT detail-dependent for
 * resizing; returns how many detail-dependent ones were left alone.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497110
int MarkAllSpritesForResizing(void)
{
    SpriteRec* s;
    int n = 0;

    for (s = g_sprites_head; s; s = s->next) {
        if (s->image->kind == 1) {
            s->flags &= ~0x400;
            n++;
        } else {
            s->flags |= 0x400;
        }
    }
    return n;
}

/* -------------------------------------------------------------------------
 * 0x00497b70 -- make a sprite's pixels available: run the draw callback for
 * a function-based sprite, otherwise decode the source bitmap if it is not
 * loaded yet.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497b70
int MakeSprite(SpriteRec* s)
{
    ImageRec* image;

    if (s->flags & 0x20) {
        PushSetTarget(s);
        ((SpriteDrawFn)s->image)(s);
        PopTarget();
        return 1;
    }
    image = s->image;
    if (!image->lls && !__BMPLoader(image))
        return 0;
    return 1;
}

/* -------------------------------------------------------------------------
 * 0x00497340 -- drop and re-decode an image's bitmap.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497340
int ReloadImageBitmap(ImageRec* image)
{
    if (image->pal)
        HeapFree_w(image->pal);
    if (image->lls) {
        FreeBitmapResources(image);
        return __BMPLoader(image) != 0;
    }
    return 1;
}

/* -------------------------------------------------------------------------
 * 0x00497ee0 / 0x00497f20 -- start / stop every layer of an ILF sprite.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497ee0
void TellAllLayersToAnimate(SpriteRec* s)
{
    int i;

    if (s->flags & 0x8000) {
        for (i = 0; i < ((ILFTable*)s->image)->count; i++)
            SetLayerAnimatingState(s, i, 1);
    }
}

// FUNCTION: LEGOLAND 0x00497f20
void TellAllLayersToStopAnimating(SpriteRec* s)
{
    int i;

    if (s->flags & 0x8000) {
        for (i = 0; i < ((ILFTable*)s->image)->count; i++)
            SetLayerAnimatingState(s, i, 0);
    }
}

/* -------------------------------------------------------------------------
 * 0x00497e80 -- fetch one layer (sprite + offsets) of an ILF sprite.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497e80
void GetLayer(SpriteRec* s, LayerOut* out, unsigned int layer)
{
    void* sprite;

    if (s->flags & 0x8000) {
        if (layer < (unsigned int)((ILFTable*)s->image)->count) {
            sprite = ((ILFTable*)s->image)->sprites[layer];
            if (sprite) {
                out->sprite = sprite;
                out->dx = ((ILFTable*)s->image)->dx[layer];
                out->dy = ((ILFTable*)s->image)->dy[layer];
            }
        }
    }
}

/* -------------------------------------------------------------------------
 * 0x00497070 -- reload every registered detail-dependent image.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497070
void ReloadAllDetailDependentImages(void)
{
    int        count = g_detail_images_count;
    int        cap   = g_detail_images_cap;
    ImageRec** p     = g_detail_images;

    while (cap != 0 && count != 0) {
        if (*p) {
            ReloadImageBitmapAndBuildSprites(*p);
            count--;
        }
        cap--;
        p++;
    }
}

/* -------------------------------------------------------------------------
 * 0x004970b0 -- release every non-resizing sprite's surface, then reload the
 * detail-dependent images.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004970b0
void ReloadAllDetailDependentImagesAndBuildSprites(void)
{
    SpriteRec* s;
    int        count;
    int        cap;
    ImageRec** p;

    for (s = g_sprites_head; s; s = s->next) {
        if (!(s->flags & 0x400) && s->surface)
            s->surface->vtbl->Release(s->surface);
    }

    cap   = g_detail_images_cap;
    count = g_detail_images_count;
    p     = g_detail_images;

    while (cap != 0 && count != 0) {
        if (*p) {
            ReloadImageBitmapAndBuildSprites(*p);
            count--;
        }
        cap--;
        p++;
    }
}

/* -------------------------------------------------------------------------
 * 0x00497510 -- drop a reference on an image; on the last one free it.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497510
int KillImage(ImageRec* image)
{
    image->refcount--;
    if (image->refcount == 0) {
        if (image->type == 2 || image->type == 3)
            LLSStop(image->lls);
        if (image->lls)
            HeapFree_w(image->lls);
        if (image->pal)
            HeapFree_w(image->pal);
        if (image->kind == 1)
            UnregisterDetailImage(image);
        HeapFree_w(image);
        return 1;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * 0x00497790 -- a sprite showing a sub-rectangle of an image.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497790
SpriteRec* CreatePartialSprite(ImageRec* image, short sx, short sy, short w, short h)
{
    SpriteRec* s = NewSprite();

    if (!s)
        return 0;

    s->surface = 0;
    s->image = image;
    image->refcount++;
    s->src_x = sx;
    s->refs = 1;
    s->src_y = sy;
    s->flags = 0;
    s->detail = g_detail - 1;
    if (!image->lls && !__BMPLoader(image)) {
        KillSprite(s);
        return 0;
    }
    s->w = w;
    s->h = h;
    return s;
}

/* -------------------------------------------------------------------------
 * 0x00497710 -- a whole-image sprite kept in system memory (flag 0x10).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497710
SpriteRec* CreateSysmemSprite(ImageRec* image)
{
    SpriteRec* s = NewSprite();

    if (!s)
        return 0;

    s->surface = 0;
    s->image = image;
    image->refcount++;
    s->refs = 1;
    s->src_x = 0;
    s->src_y = 0;
    s->flags = 0x10;
    s->detail = g_detail - 1;
    if (!image->lls && !__BMPLoader(image)) {
        KillSprite(s);
        return 0;
    }
    s->w = image->w;
    s->h = image->h;
    return s;
}

/* -------------------------------------------------------------------------
 * 0x00497640 -- a whole-image sprite; with a null image it is an empty
 * record for the caller to fill (LoadSprite's .csp path).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497640
SpriteRec* CreateSprite(ImageRec* image)
{
    SpriteRec* s = NewSprite();

    if (!s)
        return 0;

    s->surface = 0;
    s->image = image;
    s->refs = 1;
    s->src_x = 0;
    s->src_y = 0;
    s->flags = 0;
    s->detail = g_detail - 1;
    if (image) {
        image->refcount++;
        if (!image->lls && !__BMPLoader(image)) {
            KillSprite(s);
            return 0;
        }
        s->w = image->w;
        s->h = image->h;
        MakeSpriteDrawable(s);
    }
    return s;
}

/* -------------------------------------------------------------------------
 * 0x00497280 -- allocate an image record (0x18-byte header + strlen(name)+1,
 * the name strcpy'd inline after the header: the inlined strlen/strcpy are
 * the repne scasb / rep movsd+movsb pairs); kind 1 registers it as
 * detail-dependent. The null-alloc exit hands back the allocator's own 0 (no
 * xor), so it is written as `return image`.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497280
ImageRec* CreateSourceImage(const char* name, int kind)
{
    ImageRec* image = (ImageRec*)HeapAlloc_w(strlen(name) + 0x19);

    if (!image)
        return image;

    image->kind = (char)kind;
    image->refcount = 1;
    image->lls = 0;
    image->pal = 0;
    image->name = (char*)(image + 1);
    strcpy(image->name, name);
    if ((char)kind == 1)
        RegisterDetailImage(image);
    return image;
}

/* -------------------------------------------------------------------------
 * 0x00497810 -- rebind an existing sprite to a new image sub-rectangle,
 * rebuilding its surface if it had one.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497810
int RecreatePartialSprite(SpriteRec* s, ImageRec* image, short sx, short sy, short w, short h)
{
    int had;

    ReferenceImage(image);
    KillImage(s->image);
    if (s->surface) {
        s->surface->vtbl->Release(s->surface);
        had = 1;
    } else {
        had = 0;
    }
    s->src_x = sx;
    s->surface = 0;
    s->image = image;
    s->src_y = sy;
    s->flags = 0;
    s->w = w;
    s->h = h;
    if (!image->lls && !__BMPLoader(image))
        return 0;
    if (had)
        MakeSpriteDrawable(s);
    return 1;
}

/* -------------------------------------------------------------------------
 * 0x00497ab0 -- load a sprite by file name: ".csp" builds a composite sprite
 * over an empty record, anything else goes through a source image.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497ab0
SpriteRec* LoadSprite(const char* name, int kind)
{
    char       ext[0x100];
    SpriteRec* result = 0;
    SpriteRec* s;
    ImageRec*  image;

    _splitpath(name, 0, 0, 0, ext);
    if (_stricmp(ext, g_ext_csp) == 0) {
        s = CreateSprite(0);
        if (s) {
            if (!LoadCSPSprite(s, name, kind & 0xff)) {
                KillSprite(s);
                return 0;
            }
        }
        return s;
    }
    image = CreateSourceImage(name, kind);
    if (image) {
        result = CreateSprite(image);
        KillImage(image);
    }
    return result;
}

/* -------------------------------------------------------------------------
 * 0x00466640 -- (re)create the DirectDraw surface behind a sprite: video
 * memory first (unless flag 0x10), then system memory, with one Compact and
 * retry; then set the colour key (magenta for flag 0x80, else the mode's
 * transparent colour).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00466640
int RecreateSprite(SpriteRec* s)
{
    DDSurfaceDesc ddsd;
    DDColorKey    ck;
    int           key;

    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwWidth = s->w;
    ddsd.dwFlags = 7;
    ddsd.dwCaps = 0x40;
    ddsd.dwHeight = s->h;
    if ((s->flags & 0x10) ||
        g_ddraw->vtbl->CreateSurface(g_ddraw, &ddsd, &s->surface, 0) != 0) {
        ddsd.dwWidth = s->w;
        ddsd.dwHeight = s->h;
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = 7;
        ddsd.dwCaps = 0x840;
        if (g_ddraw->vtbl->CreateSurface(g_ddraw, &ddsd, &s->surface, 0) != 0) {
            if (g_ddraw->vtbl->Compact(g_ddraw) == 0)
                g_ddraw->vtbl->CreateSurface(g_ddraw, &ddsd, &s->surface, 0);
            return 0;
        }
    }

    /* VC6 duplicates the colour-key tail into both arms. */
    if (s->flags & 0x80) {
        key = GetNearestColour(0xff, 0, 0xff);
        ck.high = key;
        ck.low = key;
        s->surface->vtbl->SetColorKey(s->surface, 8, &ck);
        return 1;
    }
    key = GetTransparentColour();
    ck.high = key;
    ck.low = key;
    s->surface->vtbl->SetColorKey(s->surface, 8, &ck);
    return 1;
}
