/* LEGOLAND -- font selection, GDI text printing, the string table and the
 * speech-bubble graphics.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Field offsets and calling conventions are load-bearing; names are ours.
 *
 * ---------------------------------------------------------------------------
 * How text is drawn
 * ---------------------------------------------------------------------------
 * The game does not rasterise text itself: every Print* routine borrows a GDI
 * DC from the DirectDraw draw surface (IDirectDrawSurface::GetDC, vtable
 * +0x44) and lets GDI draw into it, then hands it back (ReleaseDC, +0x68).
 * Because GDI needs the surface UNLOCKED, the routines bracket their work with
 * PushRenderingStatusAndUnlockVideoSurface / PopRenderingStatus (surface.c);
 * InfoPrintCent is the one exception and assumes the caller already did.
 *
 * Each routine clips to the game's own clip window (g_clip_rect @ 0x4bdea0)
 * by building a CreateRectRgn from it and selecting it into the DC, selects
 * one of the four pre-created fonts through SelectFont, draws, and restores
 * everything it selected in reverse order.
 *
 * DrawText bounding boxes are built inline: the *Cent variants centre on x
 * with a +-w/2 (or +-640) box, and every box is 400 (0x190) pixels tall.
 * ------------------------------------------------------------------------- */

#pragma intrinsic(strlen)

/* ---- local types -------------------------------------------------------- */

/* Win32 RECT; the game's clip window and the DrawText boxes use it. */
typedef struct WinRect {
    long left;     /* +0x00 */
    long top;      /* +0x04 */
    long right;    /* +0x08 */
    long bottom;   /* +0x0c */
} WinRect;

typedef struct DDSurface DDSurface;

/* IDirectDrawSurface's vtable; only the two DC slots are named here.
 * 0x44 = GetDC, 0x68 = ReleaseDC (surface.c names Lock/Restore/Unlock). */
typedef struct DDSurfaceVtbl {
    char pad00[0x44];                                       /* +0x00 */
    long(__stdcall* GetDC)(DDSurface*, void** hdc);         /* +0x44 */
    char pad48[0x68 - 0x48];                                /* +0x48 */
    long(__stdcall* ReleaseDC)(DDSurface*, void* hdc);      /* +0x68 */
} DDSurfaceVtbl;

struct DDSurface {
    DDSurfaceVtbl* vtbl;   /* +0x00 */
};

/* One entry of the loaded string table: a 12-byte node hashed on id % 10
 * into ten bucket chains (g_string_buckets @ 0x79a850..0x79a877). The
 * sibling at 0x00498f80 (not exported) allocates one and pushes it on the
 * chain head; GetString walks the chain; DeleteStrings tears them down. */
typedef struct StrNode {
    int             id;     /* +0x00 */
    char*           text;   /* +0x04  heap copy of the string */
    struct StrNode* next;   /* +0x08 */
} StrNode;

/* ---- globals ------------------------------------------------------------ */

extern StrNode*   g_string_buckets[10];   /* 0x0079a850 */

extern void*      g_font_1;               /* 0x0066808c  SelectFont(.., 1) */
extern void*      g_font_default;         /* 0x00668090  SelectFont(.., other) */
extern void*      g_font_2;               /* 0x00668094  SelectFont(.., 2) */
extern void*      g_font_3;               /* 0x00668098  SelectFont(.., 3) */

extern DDSurface* g_draw_surface;         /* 0x0066807c */
extern WinRect    g_clip_rect;            /* 0x004bdea0 */

/* DDSURFACEDESC g_ddsd @ 0x0066809c seen field by field (surface.c models
 * the whole 0x6c-byte descriptor; only these four are read here). */
extern unsigned long g_ddsd_height;       /* 0x006680a4  +0x08 dwHeight */
extern unsigned long g_ddsd_width;        /* 0x006680a8  +0x0c dwWidth */
extern long          g_ddsd_pitch;        /* 0x006680ac  +0x10 lPitch */
extern void*         g_ddsd_bits;         /* 0x006680c0  +0x24 lpSurface */

/* SoftPrint's cached surface extent and its row-length scratch. */
extern unsigned long g_sp_height;         /* 0x007fea14 */
extern unsigned long g_sp_width;          /* 0x007fea1c */
extern unsigned long g_sp_rowlen;         /* 0x007fe9a4 */

/* Bubble-help graphics. */
extern int   g_bubble_loaded;             /* 0x006675b4 */
extern int   g_bubble_state;              /* 0x008139e0  cleared on load */
extern void* g_bubble_sprites[10];        /* 0x008139e4..0x00813a08 */
extern void* g_bubble_data;               /* 0x00813a0c  "SPEECH BUBBLE" data */

/* ---- imports ------------------------------------------------------------ */

__declspec(dllimport) void*         __stdcall CreateRectRgn(int l, int t, int r, int b);   /* [0x4ab0b8] */
__declspec(dllimport) void*         __stdcall SelectObject(void* dc, void* obj);             /* [0x4ab080] */
__declspec(dllimport) int           __stdcall SetBkMode(void* dc, int mode);                 /* [0x4ab074] */
__declspec(dllimport) unsigned long __stdcall SetTextColor(void* dc, unsigned long c);       /* [0x4ab0b0] */
__declspec(dllimport) int           __stdcall TextOutA(void* dc, int x, int y,
                                                       const char* s, int n);                /* [0x4ab06c] */
__declspec(dllimport) int           __stdcall DrawTextA(void* dc, const char* s, int n,
                                                        WinRect* rc, unsigned int fmt);      /* [0x4ab2ac] */
__declspec(dllimport) int           __stdcall DeleteObject(void* obj);                       /* [0x4ab09c] */

/* ---- externs ------------------------------------------------------------ */

extern void  PushRenderingStatusAndUnlockVideoSurface(void);   /* 0x00464080 */
extern void  PopRenderingStatus(void);                         /* 0x004641f0 */
extern int   GetTransparentColour(void);                       /* 0x0044e690 */
extern void  DBPrintf(const char* fmt, ...);                   /* 0x00453a20 */
extern void  HeapFree_w(void* p);                              /* 0x0049e4d0 */
extern int   LLIDB_FindElement(const char* name, void** out, unsigned int* outidx); /* 0x0047b330 */
extern void* LLIDB_LoadData(void* elem);                       /* 0x0047d3a0 */
extern void* LoadSprite(const char* name, int flags);          /* 0x00497ab0 */

/* -------------------------------------------------------------------------
 * 0x00498f50 -- look a string up by id in the hashed string table.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00498f50
char* GetString(int id)
{
    StrNode* p = g_string_buckets[id % 10];

    while (p) {
        if (p->id == id)
            return p->text;
        p = p->next;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * 0x00498ff0 -- free every string-table node and clear the buckets.
 *
 * ORIGINAL BUG, reproduced: the inner loop is guarded on `next`, which is
 * only set from the FIRST node's link, so a chain of exactly one node is
 * never freed (the bucket is still cleared, so it leaks).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00498ff0
void DeleteStrings(void)
{
    StrNode* p;
    StrNode* next = 0;
    int      i;

    for (i = 0; i < 10; i++) {
        p = g_string_buckets[i];
        if (p)
            next = p->next;
        while (next) {
            next = p->next;
            HeapFree_w(p->text);
            HeapFree_w(p);
            p = next;
        }
        g_string_buckets[i] = 0;
    }
}

/* -------------------------------------------------------------------------
 * 0x00454b40 -- select one of the four pre-created fonts into a DC; returns
 * the previously selected object so the caller can restore it.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00454b40
void* SelectFont(void* hdc, int which)
{
    switch (which) {
    case 1:
        return SelectObject(hdc, g_font_1);
    case 2:
        return SelectObject(hdc, g_font_2);
    case 3:
        return SelectObject(hdc, g_font_3);
    default:
        return SelectObject(hdc, g_font_default);
    }
}

/* -------------------------------------------------------------------------
 * 0x004651d0 -- fill the (locked) 16-bit draw surface with the transparent
 * colour, row by row: width words per row, then skip the pitch remainder.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004651d0
void SoftPrint_Clear(void)
{
    int colour = GetTransparentColour();

    g_sp_height = g_ddsd_height;
    g_sp_width  = g_ddsd_width;
    __asm {
        pushad
        mov  edi, g_ddsd_bits
        mov  edx, g_sp_width
        mov  g_sp_rowlen, edx
        mov  edx, g_sp_height
        mov  ebx, g_ddsd_pitch
        sub  ebx, g_sp_rowlen
        sub  ebx, g_sp_rowlen
        mov  eax, colour
    row:
        mov  ecx, g_sp_rowlen
        rep  stosw
        add  edi, ebx
        dec  edx
        jne  row
        popad
    }
}

/* -------------------------------------------------------------------------
 * 0x00454910 -- load the speech-bubble element and the ten bubble icons.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00454910
void LoadBubbleHelpGFX(void)
{
    void* elem;

    if (!g_bubble_loaded) {
        if (LLIDB_FindElement("SPEECH BUBBLE", &elem, 0) == 0)
            g_bubble_data = LLIDB_LoadData(elem);
        g_bubble_state = 0;
        g_bubble_sprites[0] = LoadSprite("mi_hungry.lls", 0);
        g_bubble_sprites[1] = LoadSprite("mi_happy.lls", 0);
        g_bubble_sprites[2] = LoadSprite("mi_sad.lls", 0);
        g_bubble_sprites[3] = LoadSprite("mi_home.lls", 0);
        g_bubble_sprites[4] = LoadSprite("mi_eat.lls", 0);
        g_bubble_sprites[5] = LoadSprite("great.lls", 0);
        g_bubble_sprites[6] = LoadSprite("poor.lls", 0);
        g_bubble_sprites[7] = LoadSprite("favourite.lls", 0);
        g_bubble_sprites[8] = LoadSprite("opinion.lls", 0);
        g_bubble_sprites[9] = LoadSprite("mi_bored.lls", 0);
        g_bubble_loaded = 1;
    }
}

/* -------------------------------------------------------------------------
 * 0x00454ba0 -- TextOut at (x, y) in the given font, opaque background.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00454ba0
void Print(int x, int y, const char* text, int font)
{
    void* hdc;
    void* rgn;
    void* oldrgn;
    void* oldfont;

    rgn = CreateRectRgn(g_clip_rect.left, g_clip_rect.top,
                        g_clip_rect.right, g_clip_rect.bottom);
    PushRenderingStatusAndUnlockVideoSurface();
    g_draw_surface->vtbl->GetDC(g_draw_surface, &hdc);
    SetBkMode(hdc, 2);
    oldrgn = SelectObject(hdc, rgn);
    oldfont = SelectFont(hdc, font);
    TextOutA(hdc, x, y, text, strlen(text));
    SelectObject(hdc, oldfont);
    SelectObject(hdc, oldrgn);
    g_draw_surface->vtbl->ReleaseDC(g_draw_surface, hdc);
    PopRenderingStatus();
    DeleteObject(rgn);
}

/* -------------------------------------------------------------------------
 * 0x00454c70 -- DrawText into a w-wide box at (x, y) with caller-supplied
 * colour and format flags; transparent background.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00454c70
void PrintLimitedText(int x, int y, int w, const char* text, int font,
                      unsigned long colour, unsigned int format)
{
    WinRect rc;
    void*   hdc;
    void*   rgn;
    void*   oldrgn;
    void*   oldfont;

    rc.left   = x;
    rc.right  = x + w;
    rc.top    = y;
    rc.bottom = y + 400;
    rgn = CreateRectRgn(g_clip_rect.left, g_clip_rect.top,
                        g_clip_rect.right, g_clip_rect.bottom);
    PushRenderingStatusAndUnlockVideoSurface();
    g_draw_surface->vtbl->GetDC(g_draw_surface, &hdc);
    SetBkMode(hdc, 1);
    SetTextColor(hdc, colour);
    oldrgn = SelectObject(hdc, rgn);
    oldfont = SelectFont(hdc, font);
    DrawTextA(hdc, text, strlen(text), &rc, format);
    SelectObject(hdc, oldfont);
    SelectObject(hdc, oldrgn);
    g_draw_surface->vtbl->ReleaseDC(g_draw_surface, hdc);
    PopRenderingStatus();
    DeleteObject(rgn);
}

/* -------------------------------------------------------------------------
 * 0x00454e60 -- word-wrapped, centred text in a w-wide box around x.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00454e60
void PrintCent(int x, int y, int w, const char* text, int font)
{
    WinRect rc;
    void*   hdc;
    void*   rgn;
    void*   oldrgn;
    void*   oldfont;

    rc.left   = x - w / 2;
    rc.right  = x + w / 2;
    rc.top    = y;
    rc.bottom = y + 400;
    rgn = CreateRectRgn(g_clip_rect.left, g_clip_rect.top,
                        g_clip_rect.right, g_clip_rect.bottom);
    PushRenderingStatusAndUnlockVideoSurface();
    g_draw_surface->vtbl->GetDC(g_draw_surface, &hdc);
    SetBkMode(hdc, 1);
    oldrgn = SelectObject(hdc, rgn);
    oldfont = SelectFont(hdc, font);
    DrawTextA(hdc, text, strlen(text), &rc, 0x11);   /* DT_CENTER|DT_WORDBREAK */
    SelectObject(hdc, oldfont);
    SelectObject(hdc, oldrgn);
    g_draw_surface->vtbl->ReleaseDC(g_draw_surface, hdc);
    PopRenderingStatus();
    DeleteObject(rgn);
}

/* -------------------------------------------------------------------------
 * 0x00454f60 -- single-line centred text with an opaque background, in a
 * 1280-wide box around x.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00454f60
void PrintCentOpaque(int x, int y, const char* text, int font)
{
    WinRect rc;
    void*   hdc;
    void*   rgn;
    void*   oldrgn;
    void*   oldfont;

    rc.left   = x - 640;
    rc.right  = x + 640;
    rc.top    = y;
    rc.bottom = y + 400;
    rgn = CreateRectRgn(g_clip_rect.left, g_clip_rect.top,
                        g_clip_rect.right, g_clip_rect.bottom);
    PushRenderingStatusAndUnlockVideoSurface();
    g_draw_surface->vtbl->GetDC(g_draw_surface, &hdc);
    SetBkMode(hdc, 2);
    oldrgn = SelectObject(hdc, rgn);
    oldfont = SelectFont(hdc, font);
    DrawTextA(hdc, text, strlen(text), &rc, 1);      /* DT_CENTER */
    SelectObject(hdc, oldfont);
    SelectObject(hdc, oldrgn);
    g_draw_surface->vtbl->ReleaseDC(g_draw_surface, hdc);
    PopRenderingStatus();
    DeleteObject(rgn);
}

/* -------------------------------------------------------------------------
 * 0x00455060 -- PrintCent in a caller-supplied colour (restored after).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00455060
void PrintCentColref(unsigned long colour, int x, int y, int w,
                     const char* text, int font)
{
    WinRect       rc;
    void*         hdc;
    void*         rgn;
    void*         oldrgn;
    void*         oldfont;
    unsigned long oldcolour;

    rc.left   = x - w / 2;
    rc.right  = x + w / 2;
    rc.top    = y;
    rc.bottom = y + 400;
    rgn = CreateRectRgn(g_clip_rect.left, g_clip_rect.top,
                        g_clip_rect.right, g_clip_rect.bottom);
    PushRenderingStatusAndUnlockVideoSurface();
    g_draw_surface->vtbl->GetDC(g_draw_surface, &hdc);
    SetBkMode(hdc, 1);
    oldcolour = SetTextColor(hdc, colour);
    DBPrintf("DC= %08x\n", hdc);
    oldrgn = SelectObject(hdc, rgn);
    oldfont = SelectFont(hdc, font);
    DrawTextA(hdc, text, strlen(text), &rc, 0x11);   /* DT_CENTER|DT_WORDBREAK */
    SelectObject(hdc, oldfont);
    SelectObject(hdc, oldrgn);
    SetTextColor(hdc, oldcolour);
    g_draw_surface->vtbl->ReleaseDC(g_draw_surface, hdc);
    PopRenderingStatus();
    DeleteObject(rgn);
}

/* -------------------------------------------------------------------------
 * 0x00491d60 -- single-line, vertically and horizontally centred text in a
 * caller-supplied box (passed BY VALUE); optionally forced white.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00491d60
void NewPrintCent(const char* text, int font, WinRect rc, char white)
{
    void* hdc;
    void* rgn;
    void* oldrgn;
    void* oldfont;

    rgn = CreateRectRgn(g_clip_rect.left, g_clip_rect.top,
                        g_clip_rect.right, g_clip_rect.bottom);
    PushRenderingStatusAndUnlockVideoSurface();
    g_draw_surface->vtbl->GetDC(g_draw_surface, &hdc);
    SetBkMode(hdc, 1);
    if (white == 1)
        SetTextColor(hdc, 0xffffff);
    oldrgn = SelectObject(hdc, rgn);
    oldfont = SelectFont(hdc, font);
    DrawTextA(hdc, text, strlen(text), &rc, 0x25);   /* DT_CENTER|DT_VCENTER|DT_SINGLELINE */
    SelectObject(hdc, oldfont);
    SelectObject(hdc, oldrgn);
    g_draw_surface->vtbl->ReleaseDC(g_draw_surface, hdc);
    PopRenderingStatus();
    DeleteObject(rgn);
}

/* -------------------------------------------------------------------------
 * 0x004716a0 -- white word-wrapped text in a caller-supplied box (BY VALUE),
 * centred or left-aligned. Does NOT push/pop the rendering status -- the
 * caller must already have the surface unlocked. The first parameter is
 * never read.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004716a0
void InfoPrintCent(int unused, const char* text, int font, WinRect rc, int centre)
{
    void* hdc;
    void* rgn;
    void* oldrgn;
    void* oldfont;

    rgn = CreateRectRgn(g_clip_rect.left, g_clip_rect.top,
                        g_clip_rect.right, g_clip_rect.bottom);
    g_draw_surface->vtbl->GetDC(g_draw_surface, &hdc);
    SetBkMode(hdc, 1);
    SetTextColor(hdc, 0xffffff);
    oldrgn = SelectObject(hdc, rgn);
    oldfont = SelectFont(hdc, font);
    if (centre)
        DrawTextA(hdc, text, strlen(text), &rc, 0x11);   /* DT_CENTER|DT_WORDBREAK */
    else
        DrawTextA(hdc, text, strlen(text), &rc, 0x10);   /* DT_WORDBREAK */
    SelectObject(hdc, oldfont);
    SelectObject(hdc, oldrgn);
    g_draw_surface->vtbl->ReleaseDC(g_draw_surface, hdc);
    DeleteObject(rgn);
}
