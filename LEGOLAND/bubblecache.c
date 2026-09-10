/* LEGOLAND — bubble-help teardown, the rendered-text cache's rasterise /
 * lookup / blit path, the function-sprite that paints a cache entry, and
 * the edit-cursor footprint clearance test.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, global addresses and callee argument counts are load-bearing;
 * names are ours.
 *
 * Neighbours: text.c (LoadBubbleHelpGFX), fpui2.c (HTBubbleHelp),
 * fpui3.c (FindCachedText), pathmask.c (FreeCachedTextEntry),
 * render5.c (ExpireCachedText), popup2.c (PrintCachedText caller).
 */

#pragma intrinsic(strlen, strcpy, strcmp, memset)

/* ---- local types -------------------------------------------------------- */

typedef struct WinRect {
    long left;     /* +0x00 */
    long top;      /* +0x04 */
    long right;    /* +0x08 */
    long bottom;   /* +0x0c */
} WinRect;

typedef struct DDColorKey {
    unsigned long low;    /* +0x00 */
    unsigned long high;   /* +0x04 */
} DDColorKey;

typedef struct DDSurface DDSurface;

typedef struct DDSurfaceVtbl {
    char pad00[0x44];
    long(__stdcall* GetDC)(DDSurface*, void** hdc);           /* +0x44 */
    char pad48[0x68 - 0x48];
    long(__stdcall* ReleaseDC)(DDSurface*, void* hdc);        /* +0x68 */
    char pad6c[0x74 - 0x6c];
    long(__stdcall* SetColorKey)(DDSurface*, unsigned long, DDColorKey*); /* +0x74 */
} DDSurfaceVtbl;

struct DDSurface {
    DDSurfaceVtbl* vtbl;   /* +0x00 */
};

typedef struct SpriteRec {
    struct SpriteRec* next;    /* +0x00 */
    DDSurface*        surface; /* +0x04 */
    void*             image;   /* +0x08 */
    int               detail;  /* +0x0c */
    unsigned int      flags;   /* +0x10 */
    short             w;       /* +0x14 */
    short             h;       /* +0x16 */
} SpriteRec;

/* fpui3.c's 0x20-byte rendered-text cache entry. */
typedef struct TextEntry {
    int        w;       /* +0x00 */
    int        h;       /* +0x04 */
    int        format;  /* +0x08 */
    char*      text;    /* +0x0c */
    int        ink;     /* +0x10 */
    int        paper;   /* +0x14 */
    int        font;    /* +0x18 */
    SpriteRec* sprite;  /* +0x1c */
} TextEntry;

typedef struct MapHdr {
    unsigned short w;         /* +0x00  screen width in pixels */
    unsigned short h;         /* +0x02 */
    char           pad04[0x10];
    unsigned short cells_w;   /* +0x14 */
    unsigned short cells_h;   /* +0x16 */
} MapHdr;

typedef struct HelpRect {
    int x0;   /* +0x00 */
    int y0;   /* +0x04 */
    int x1;   /* +0x08 */
    int y1;   /* +0x0c */
} HelpRect;

/* Same two-int cell as gameframe.c.  Taken by value so the incoming
 * slots are one aggregate and cannot be reused for the FindElement
 * out-local (FR02 / fable-b two-ints→Pos). */
typedef struct Pos {
    int x;    /* +0x00 */
    int y;    /* +0x04 */
} Pos;

typedef struct ObjDef {
    char    pad00[0x3c];
    int     ox;        /* +0x3c */
    int     oy;        /* +0x40 */
    char    pad44[0x80];
    void*   inst;      /* +0xc4  compared to the PATH CONTROL element */
} ObjDef;

typedef struct Cell {
    void*          obj;      /* +0x00 */
    char           pad04[8];
    unsigned short flags;    /* +0x0c */
    unsigned short uflags;   /* +0x0e */
    unsigned char  rf;       /* +0x10 */
    unsigned char  life;     /* +0x11 */
    unsigned short extra;    /* +0x12  non-zero: only PATH CONTROL may occupy */
} Cell;

/* ---- globals ------------------------------------------------------------ */

extern volatile int  g_text_cache_count;     /* 0x006675b8 */
extern TextEntry     g_text_cache[];         /* 0x006675c0 */
extern int           g_bubble_loaded;        /* 0x006675b4 */
extern SpriteRec*    g_bubble_sprites[10];   /* 0x008139e4 */
extern SpriteRec*    g_bubble_icons[];       /* 0x008139e0  1-based; [1] = sprites[0] */
extern DDSurface*    g_draw_surface;         /* 0x0066807c */
extern ObjDef*       g_edit_object;          /* 0x008119b8 */
extern MapHdr*       g_map;                  /* 0x004bcbf4 */
extern Cell**        g_map_rows;             /* 0x00801400 */
extern int           g_fp_w;                 /* 0x00813a6c  g_input.fp_w */
extern int           g_fp_h;                 /* 0x00813a70  g_input.fp_h */

extern const char    kCreatingCell[];        /* 0x004b9080 "Creating Cell (%d) %s\n" */
extern const char    kPathControl[];         /* 0x004b8a70 "PATH CONTROL" */

/* ---- imports ------------------------------------------------------------ */

__declspec(dllimport) void*         __stdcall CreateCompatibleDC(void* dc);                   /* [0x4ab094] */
__declspec(dllimport) int           __stdcall DeleteDC(void* dc);                             /* [0x4ab0a4] */
__declspec(dllimport) unsigned long __stdcall GetNearestColor(void* dc, unsigned long color); /* [0x4ab0bc] */
__declspec(dllimport) int           __stdcall SetBkMode(void* dc, int mode);                  /* [0x4ab074] */
__declspec(dllimport) unsigned long __stdcall SetBkColor(void* dc, unsigned long color);      /* [0x4ab0cc] */
__declspec(dllimport) void*         __stdcall CreateSolidBrush(unsigned long color);          /* [0x4ab064] */
__declspec(dllimport) int           __stdcall FillRect(void* dc, WinRect* rc, void* brush);   /* [0x4ab2b0] */
__declspec(dllimport) int           __stdcall DeleteObject(void* obj);                        /* [0x4ab09c] */
__declspec(dllimport) unsigned long __stdcall SetTextColor(void* dc, unsigned long color);    /* [0x4ab0b0] */
__declspec(dllimport) void*         __stdcall SelectObject(void* dc, void* obj);              /* [0x4ab080] */
__declspec(dllimport) int           __stdcall DrawTextA(void* dc, const char* s, int n,
                                                        WinRect* rc, unsigned int fmt);      /* [0x4ab2ac] */

/* ---- callees ------------------------------------------------------------ */

extern unsigned int strlen(const char* s);
extern char*        strcpy(char* d, const char* s);
extern int          strcmp(const char* a, const char* b);
extern void*        memset(void* d, int c, unsigned n);

extern void        DBPrintf(const char* fmt, ...);                          /* 0x00453a20 */
extern void*       HeapAlloc_w(unsigned int size);                          /* 0x0049e4ff */
extern void        ExpireCachedText(int all);                               /* 0x00455f70 */
extern SpriteRec*  CreateFunctionBasedSprite(void (*fn)(SpriteRec*), int w, int h); /* 0x004976c0 */
extern int         PrintSprite(SpriteRec* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern void        PushRenderingStatusAndUnlockVideoSurface(void);          /* 0x00464080 */
extern void        PopRenderingStatus(void);                                /* 0x004641f0 */
extern void*       SelectFont(void* dc, int font);                          /* 0x00454b40 */
extern int         GetNearestColour(int r, int g, int b);                   /* 0x0044e6c0 */
extern int         LLIDB_FindElement(const char* name, void** out, unsigned int* outidx); /* 0x0047b330 */
extern int         LLIDB_UnLoadData(void* elem);                            /* 0x0047d450 */
extern void        KillSprite(SpriteRec* s);                                /* 0x00497bd0 */
extern TextEntry*  FindCachedText(const char* text, int font, int format, int ink, int paper); /* 0x00455d40 */
#ifndef LEGOLAND_PORTABLE
extern void        RenderBlock(int x, int y, int w, int h, int colour);     /* 0x004890c0 */
#else
extern int RenderBlock(int x, int y, int w, int h, int colour);     /* 0x004890c0 */
#endif

void DrawCachedTextSprite(SpriteRec* s);

/* Ink pair inverted so the inlined schedule lands paper-then-ink (CC05). */
static __inline void SetEntryInk(TextEntry* e, int paper, int ink, int font)
{
    e->ink = ink;
    e->paper = paper;
    e->font = font;
}


/* =========================================================================
 *  FindCachedEntryBySprite — look a cache entry up by its function-sprite
 * ========================================================================= */

/* The draw callback at 0x00455a50 is handed only the sprite; this walks the
 * cache by +0x1c and optionally writes the index.  The cursor anchors at
 * sprite (the only field the loop reads).  Count is re-read in the latch.
 */

// FUNCTION: LEGOLAND 0x00455a10
TextEntry* FindCachedEntryBySprite(SpriteRec* sprite, int* out_index)
{
    int i;

    for (i = 0; i < *(volatile int*)&g_text_cache_count; i++) {
        if (g_text_cache[i].sprite == sprite) {
            if (out_index)
                *out_index = i;
            return &g_text_cache[i];
        }
    }
    return 0;
}

/* =========================================================================
 *  FindCachedTextBox — sized lookup (w, h, format, ink, paper, font, text)
 * ========================================================================= */

/* PrintCachedText's key, as money.c recorded it.  Distinct from fpui3.c's
 * FindCachedText (0x00455d40), which does not compare w/h.
 */

// FUNCTION: LEGOLAND 0x00455c80
TextEntry* FindCachedTextBox(const char* text, int w, int h, int font,
                             int format, int ink, int paper)
{
    int i;

    for (i = 0; i < *(volatile int*)&g_text_cache_count; i++) {
        if (g_text_cache[i].w == w
         && g_text_cache[i].h == h
         && g_text_cache[i].format == format
         && g_text_cache[i].ink == ink
         && g_text_cache[i].paper == paper
         && g_text_cache[i].font == font
         && strcmp(g_text_cache[i].text, text) == 0)
            return &g_text_cache[i];
    }
    return 0;
}

/* =========================================================================
 *  RasterizeText — append a cache entry and wrap it in a function-sprite
 * ========================================================================= */

/* Cap is 50.  A full cache is emptied (ExpireCachedText(1)) before the
 * append.  The text is HeapAlloc_w'd + strcpy'd; the sprite is
 * CreateFunctionBasedSprite(DrawCachedTextSprite, w, h) with flags |= 0x40
 * (on top of the 0x30 CreateFunctionBasedSprite already stores).
 * format is re-read volatile so its load wins eax and the store sits
 * before strlen's xor eax (w/h then sink into the scasb setup).
 */

// FUNCTION: LEGOLAND 0x00455bb0
TextEntry* RasterizeText(const char* text, int w, int h, int font,
                         int format, int ink, int paper)
{
    TextEntry* e;
    char*      copy;
    DBPrintf(kCreatingCell, g_text_cache_count, text);
    if (g_text_cache_count >= 0x32)
        ExpireCachedText(1);
    e = &g_text_cache[g_text_cache_count];
    g_text_cache_count++;
    {
        int f = *(volatile int*)&format;
        e->format = f;
        e->w = w;
        e->h = h;
    }
    copy = (char*)HeapAlloc_w(strlen(text) + 1);
    e->text = copy;
    strcpy(copy, text);
    SetEntryInk(e, paper, ink, font);
    e->sprite = CreateFunctionBasedSprite(DrawCachedTextSprite, w, h);
    e->sprite->flags |= 0x40;
    return e;
}

/* =========================================================================
 *  PrintCachedText — lookup-or-rasterise, then blit the cached sprite
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00455e50
void PrintCachedText(const char* text, int x, int y, int w, int h,
                     int font, int format, int ink, int paper)
{
    TextEntry* e;

    e = FindCachedTextBox(text, w, h, font, format, ink, paper);
    if (e == 0)
        e = RasterizeText(text, w, h, font, format, ink, paper);
    PrintSprite(e->sprite, x, y, 0, 0);
}

/* =========================================================================
 *  UnloadBubbleHelpGFX — tear down LoadBubbleHelpGFX's ten icons + element
 * ========================================================================= */

/* Inverse of text.c's LoadBubbleHelpGFX.  The loaded flag is cleared before
 * the find; a miss (FindElement != 0) skips UnLoadData.  Each sprite is
 * killed individually so VC6 keeps the unrolled cmp/je/KillSprite/store-0
 * blocks (a counted for-loop collapses them).
 */

// FUNCTION: LEGOLAND 0x00454a10
void UnloadBubbleHelpGFX(void)
{
    void* elem;

    if (g_bubble_loaded) {
        g_bubble_loaded = 0;
        if (LLIDB_FindElement("SPEECH BUBBLE", &elem, 0) == 0)
            LLIDB_UnLoadData(elem);
        if (g_bubble_sprites[0]) {
            KillSprite(g_bubble_sprites[0]);
            g_bubble_sprites[0] = 0;
        }
        if (g_bubble_sprites[1]) {
            KillSprite(g_bubble_sprites[1]);
            g_bubble_sprites[1] = 0;
        }
        if (g_bubble_sprites[2]) {
            KillSprite(g_bubble_sprites[2]);
            g_bubble_sprites[2] = 0;
        }
        if (g_bubble_sprites[3]) {
            KillSprite(g_bubble_sprites[3]);
            g_bubble_sprites[3] = 0;
        }
        if (g_bubble_sprites[4]) {
            KillSprite(g_bubble_sprites[4]);
            g_bubble_sprites[4] = 0;
        }
        if (g_bubble_sprites[5]) {
            KillSprite(g_bubble_sprites[5]);
            g_bubble_sprites[5] = 0;
        }
        if (g_bubble_sprites[6]) {
            KillSprite(g_bubble_sprites[6]);
            g_bubble_sprites[6] = 0;
        }
        if (g_bubble_sprites[7]) {
            KillSprite(g_bubble_sprites[7]);
            g_bubble_sprites[7] = 0;
        }
        if (g_bubble_sprites[8]) {
            KillSprite(g_bubble_sprites[8]);
            g_bubble_sprites[8] = 0;
        }
        if (g_bubble_sprites[9]) {
            KillSprite(g_bubble_sprites[9]);
            g_bubble_sprites[9] = 0;
        }
    }
}

/* =========================================================================
 *  FootprintClearanceTest — every cell of the edit class's footprint is free
 * ========================================================================= */

/* HandleMapClick (gameframe.c) calls this with the origin-adjusted cell
 * before WorkOrderBuildObject.  ox/oy are added back, then the rectangle
 * [x+ox, x+ox+fp_w) × [y+oy, y+oy+fp_h) is walked.  A cell fails the test
 * when it is off-map, null, carries flags 0x8f8, or has extra != 0 and the
 * edit class's +0xc4 is not the PATH CONTROL element.  An empty footprint
 * (fp_w or fp_h <= 0) succeeds.
 *
 * Taken as Pos by value: the two incoming slots are one aggregate, so
 * elem cannot steal the dead x-arg and the original `push ecx` falls out
 * (FR02 / fable-b).  Assign into p.x / p.y so ebx loads p.x and y0 lives
 * in the y-arg slot.
 */

// FUNCTION: LEGOLAND 0x00457970
int FootprintClearanceTest(Pos p)
{
    int oy;
    int xx;
    int yy;
    void* elem;
    Cell* cell;

    oy = g_edit_object->oy;
    p.x += g_edit_object->ox;
    p.y += oy;
    yy = p.y;
    if (yy >= p.y + g_fp_h)
        goto success;
loop:
    for (xx = p.x; xx < p.x + g_fp_w; xx++) {
        if (xx < 0 || xx >= (int)g_map->cells_w || yy < 0 || yy >= (int)g_map->cells_h)
            goto fail;
        cell = &g_map_rows[yy][xx];
        if (cell == 0)
            goto fail;
        if (cell->flags & 0x8f8)
            goto fail;
        if (cell->extra != 0) {
            LLIDB_FindElement(kPathControl, &elem, 0);
            if (g_edit_object->inst != elem)
                goto fail;
        }
    }
    yy++;
    if (yy >= p.y + g_fp_h)
        goto success;
    goto loop;
fail:
    return 0;
success:
    return 1;
}

/* =========================================================================
 *  DrawCachedTextSprite — function-sprite painter for one cache entry
 * ========================================================================= */

/* CreateFunctionBasedSprite installs this at sprite+0x08.  The sprite
 * argument is looked up in the cache (out-index unused); a miss is a no-op.
 * The paint path unlocks the video surface, GetDCs the draw surface
 * (hdc reuses the dead sprite-argument slot), fills the paper with a
 * solid brush of GetNearestColor(ink), DrawTextA's the entry, then
 * SetColorKey's the sprite surface to GetNearestColour of that ink.
 * rc.left is a plain 0; memset of the remaining 12 bytes (RC08) splits
 * the four-zero web so eax stays the zero and edi is not hoisted.
 */

// FUNCTION: LEGOLAND 0x00455a50
void DrawCachedTextSprite(SpriteRec* s)
{
    SpriteRec*   p;
    TextEntry*   e;
    WinRect      rc;
    DDColorKey   ck;
    unsigned int nearest;

    p = s;
    rc.left = 0;
    memset(&rc.top, 0, 12);
    e = FindCachedEntryBySprite(p, 0);
    if (e == 0)
        return;
    {
        void* brush;
        void* oldfont;
        int   key;

        rc.right = e->w;
        rc.bottom = e->h;
        PushRenderingStatusAndUnlockVideoSurface();
        g_draw_surface->vtbl->GetDC(g_draw_surface, (void**)&s);
        nearest = GetNearestColor(s, e->ink & 0xffffff);
        SetBkMode(s, 1);
        SetBkColor(s, nearest);
        brush = CreateSolidBrush(nearest);
        FillRect(s, &rc, brush);
        DeleteObject(brush);
        SetTextColor(s, e->paper & 0xffffff);
        oldfont = SelectFont(s, e->font);
        DrawTextA(s, e->text, strlen(e->text), &rc, e->format);
        SelectObject(s, oldfont);
        g_draw_surface->vtbl->ReleaseDC(g_draw_surface, s);
        PopRenderingStatus();
        key = GetNearestColour(nearest & 0xff, (nearest >> 8) & 0xff,
                               (nearest >> 16) & 0xff);
        ck.high = key;
        ck.low = key;
        e->sprite->surface->vtbl->SetColorKey(e->sprite->surface, 8, &ck);
    }
}

/* =========================================================================
 *  VisitorBubbleHelp — HTBubbleHelp plus an optional mood-icon index
 * ========================================================================= */

/* gameframe.c's hit 0x306 (a visitor) calls this with GetVisitorName and
 * sub_482cb0's mood code.  A non-zero icon leaves a 40-pixel pad; the box
 * grows by pad/2 and PrintSprites g_bubble_icons[icon] at box.x1 - pad/2.
 * Screen width is g_map->w.  Unlike HTBubbleHelp the text is DrawTextA'd
 * onto the video surface; the cache is size-only (same RasterizeText miss
 * path).
 */

// FUNCTION: LEGOLAND 0x00455fc0
void VisitorBubbleHelp(HelpRect* r, char* text, int font, int icon)
{
    TextEntry* ent;
    WinRect    rc;
    int        colour;
    int        h;
    int        tw;
    int        bx;
    int        pad;
    int        half;
    HelpRect   box;
    void*      hdc;
    void*      oldfont;

    rc.left = 0;
    rc.top = 0;
    rc.right = 0;
    rc.bottom = 0;
    colour = GetNearestColour(0xda, 0xc6, 0x96);
    pad = 0;
    if (icon != 0)
        pad = 0x28;
    if (text == 0)
        return;
    rc.right = 200;
    ent = FindCachedText(text, font, 0x10, 0x96c6da, 0);
    if (ent == 0) {
        void* dc;
        void* old;
        dc = CreateCompatibleDC(0);
        SetBkMode(dc, 1);
        old = SelectFont(dc, font);
        h = DrawTextA(dc, text, strlen(text), &rc, 0x410);
        rc.top = r->y0;
        rc.bottom = h + rc.top;
        SelectObject(dc, old);
        DeleteDC(dc);
        ent = RasterizeText(text, rc.right - rc.left, h, font, 0x10, 0x96c6da, 0);
    } else {
        rc.left = 0;
        rc.top = 0;
        rc.right = ent->w;
        rc.bottom = ent->h;
        h = ent->h;
    }
    bx = (r->x1 + r->x0) >> 1;
    if (bx < 0)
        bx = 0;
    else if (bx > (int)g_map->w)
        bx = g_map->w;
    tw = rc.right - rc.left;
    rc.left = bx - (tw >> 1);
    rc.right = bx + ((tw + 1) >> 1);
    if (rc.left < 0)
        rc.left = 0;
    else if (rc.right + pad >= (int)g_map->w)
        rc.left = g_map->w - tw - pad;
    half = pad / 2;
    rc.right = rc.left + tw + half;
    if (r->y0 < rc.bottom - rc.top + 8) {
        rc.top = r->y1 + 6;
        rc.bottom = rc.top + h;
    } else {
        rc.bottom = r->y0 - 6;
        rc.top = rc.bottom - h;
    }
    box.x1 = rc.right + 4;
    box.x0 = rc.left - 4;
    box.y0 = rc.top - 4;
    box.y1 = rc.bottom + 4;
    RenderBlock(box.x0 + 1, box.y0 + 1, box.x1 - box.x0, box.y1 - box.y0 - 1, colour);
    RenderBlock(box.x0, box.y0, box.x1 - box.x0, 1, 0);
    RenderBlock(box.x0, box.y1, box.x1 - box.x0, 1, 0);
    RenderBlock(box.x0, box.y0, 1, box.y1 - box.y0, 0);
    RenderBlock(box.x1, box.y0, 1, box.y1 - box.y0, 0);
    if (icon != 0)
        PrintSprite(g_bubble_icons[icon], box.x1 - half,
                    (box.y0 + box.y1) / 2 - 0x14, 0, 0);
    PushRenderingStatusAndUnlockVideoSurface();
    g_draw_surface->vtbl->GetDC(g_draw_surface, &hdc);
    SetBkMode(hdc, 1);
    oldfont = SelectFont(hdc, font);
    DrawTextA(hdc, text, strlen(text), &rc, 0x10);
    SelectObject(hdc, oldfont);
    g_draw_surface->vtbl->ReleaseDC(g_draw_surface, hdc);
    PopRenderingStatus();
}
