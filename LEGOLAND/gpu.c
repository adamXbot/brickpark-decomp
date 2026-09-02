/* LEGOLAND -- the DirectDraw host: the DirectDraw object and its caps, the
 * clip window, the render-target stack and the raw rasterisers (block fill,
 * sprite blits, the Z-buffer image) that every Print* front-end ends up in.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field offsets, global addresses and callee arg counts are load-bearing;
 * names are ours.
 *
 * ---------------------------------------------------------------------------
 * The host GPU block (0x00667d70 .. 0x00668148, 0xf6 dwords)
 * ---------------------------------------------------------------------------
 * CheckHostSystemGPU (util.c) zeroes exactly this range before calling
 * InitHostSystemGPU, and every DirectDraw handle the game owns lives in it:
 *
 *   0x00667d70  IDirectDraw*         g_ddraw1        DirectDrawCreate result
 *   0x00667d74  IDirectDraw2*        g_ddraw         QueryInterface(IID_IDirectDraw2)
 *   0x00667d78  DDCAPS               g_drvcaps       driver caps (dwSize 0x17c)
 *   0x00667ef4  DDCAPS               g_helcaps       HEL caps    (dwSize 0x17c)
 *   0x00668070  IDirectDrawSurface*  primary surface (spritemisc.c)
 *   0x0066807c  IDirectDrawSurface*  g_draw_surface  what we rasterise into
 *   0x00668080  IDirectDrawClipper*  g_clipper       attached for hardware fills
 *   0x0066808c..0x00668098  four GDI objects, DeleteObject'ed at shutdown
 *   0x0066809c  DDSURFACEDESC        g_ddsd          lock descriptor (surface.c)
 *   0x00668108  RECT                 g_render_clip   clip actually used to draw
 *   0x00668118  int                  g_target_sp     render-target stack index
 *   0x0066811c  IDirectDrawSurface*  g_target_stack[10]
 *   0x00668144  int                  g_video_locked
 *
 * The render-target stack: PushSetTarget parks the current draw surface and
 * redirects drawing into a sprite's own surface (this is how function-drawn
 * sprites are painted, see sprite2.c MakeSprite); PopTarget restores it. Both
 * bracket the switch with the surface.c status stack so the old target is
 * unlocked before the swap and the new one locked after it.
 *
 * The Z-buffer image: GenerateNewImageFromZBuffer redirects the lock
 * descriptor to a static 16-bpp buffer (0x0066be54, pitch w*2), draws a
 * sprite into it with the software blitter, then builds a second 16-bpp
 * image (0x00701e68) that keeps only the pixels whose Z (0x00488820) is at or
 * below a threshold and paints the rest in the transparent colour. The result
 * is handed back through a static sprite (0x00701e64) over a static image
 * record (0x0066be50). The lock descriptor and render clip are saved in
 * 0x0066b5b0 / 0x0066b620 and put back afterwards.
 * ------------------------------------------------------------------------- */

void* memset(void*, int, unsigned int);
__declspec(noreturn) void exit(int);

/* ---- local types -------------------------------------------------------- */

typedef struct WinRect {
    long left;     /* +0x00 */
    long top;      /* +0x04 */
    long right;    /* +0x08 */
    long bottom;   /* +0x0c */
} WinRect;

typedef struct WinPoint {
    long x;        /* +0x00 */
    long y;        /* +0x04 */
} WinPoint;

/* The global record at 0x004bcbf4 seen through its screen-extent view. */
typedef struct Screen {
    unsigned short w;   /* +0x00 */
    unsigned short h;   /* +0x02 */
} Screen;

/* DDSURFACEDESC, 0x6c bytes. */
typedef struct DDSurfaceDesc {
    unsigned long dwSize;              /* +0x00 */
    unsigned long dwFlags;             /* +0x04 */
    unsigned long dwHeight;            /* +0x08 */
    unsigned long dwWidth;             /* +0x0c */
    long          lPitch;              /* +0x10 */
    unsigned long dwBackBufferCount;   /* +0x14 */
    unsigned long dwMipMapCount;       /* +0x18 */
    unsigned long dwAlphaBitDepth;     /* +0x1c */
    unsigned long dwReserved;          /* +0x20 */
    void*         lpSurface;           /* +0x24 */
    char          pad28[0x6c - 0x28];  /* +0x28 */
} DDSurfaceDesc;

/* DDBLTFX, 0x64 bytes; only dwSize and dwFillColor are used. */
typedef struct DDBltFx {
    unsigned long dwSize;              /* +0x00 */
    char          pad04[0x50 - 0x04];  /* +0x04 */
    unsigned long dwFillColor;         /* +0x50 */
    char          pad54[0x64 - 0x54];  /* +0x54 */
} DDBltFx;

/* DDCAPS (DirectX 5 layout), 0x17c bytes; only dwSize is written here. */
typedef struct DDCaps {
    unsigned long dwSize;              /* +0x00 */
    char          pad04[0x17c - 0x04]; /* +0x04 */
} DDCaps;

/* RGNDATA with one rectangle: a 0x20-byte header, its bounding rect at
 * +0x10, then the rect list at +0x20. */
typedef struct RgnData {
    unsigned long dwSize;    /* +0x00 sizeof(RGNDATAHEADER) = 0x20 */
    unsigned long iType;     /* +0x04 RDH_RECTANGLES = 1 */
    unsigned long nCount;    /* +0x08 */
    unsigned long nRgnSize;  /* +0x0c bytes of rect data */
    WinRect       rcBound;   /* +0x10 */
    WinRect       rect;      /* +0x20 */
} RgnData;

typedef struct DDSurface DDSurface;
typedef struct DDClipper DDClipper;
typedef struct DDraw     DDraw;
typedef struct DDraw2    DDraw2;
typedef struct Guid { unsigned char b[16]; } Guid;

/* IDirectDrawSurface vtable (slots used: Release 0x08, Blt 0x14, Restore
 * 0x6c, SetClipper 0x70, Unlock 0x80). */
typedef struct DDSurfaceVtbl {
    char pad00[0x08];
    long(__stdcall* Release)(DDSurface*);                                     /* +0x08 */
    char pad0c[0x14 - 0x0c];
    long(__stdcall* Blt)(DDSurface*, WinRect*, DDSurface*, WinRect*,
                         unsigned long, DDBltFx*);                            /* +0x14 */
    char pad18[0x6c - 0x18];
    long(__stdcall* Restore)(DDSurface*);                                     /* +0x6c */
    long(__stdcall* SetClipper)(DDSurface*, DDClipper*);                      /* +0x70 */
    char pad74[0x80 - 0x74];
    long(__stdcall* Unlock)(DDSurface*, void*);                               /* +0x80 */
} DDSurfaceVtbl;
struct DDSurface { DDSurfaceVtbl* vtbl; };

/* IDirectDrawClipper vtable: SetClipList is slot 0x1c. */
typedef struct DDClipperVtbl {
    char pad00[0x1c];
    long(__stdcall* SetClipList)(DDClipper*, RgnData*, unsigned long);        /* +0x1c */
} DDClipperVtbl;
struct DDClipper { DDClipperVtbl* vtbl; };

/* IDirectDraw (v1) vtable: QueryInterface 0x00, Release 0x08. */
typedef struct DDrawVtbl {
    long(__stdcall* QueryInterface)(DDraw*, const Guid*, void**);             /* +0x00 */
    char pad04[0x04];
    long(__stdcall* Release)(DDraw*);                                         /* +0x08 */
} DDrawVtbl;
struct DDraw { DDrawVtbl* vtbl; };

/* IDirectDraw2 vtable: Release 0x08, GetCaps 0x2c. */
typedef struct DDraw2Vtbl {
    char pad00[0x08];
    long(__stdcall* Release)(DDraw2*);                                        /* +0x08 */
    char pad0c[0x2c - 0x0c];
    long(__stdcall* GetCaps)(DDraw2*, DDCaps*, DDCaps*);                      /* +0x2c */
} DDraw2Vtbl;
struct DDraw2 { DDraw2Vtbl* vtbl; };

/* sprite2.c's SpriteRec: surface @4, detail @0xc, flags @0x10, w/h @0x14/16. */
typedef struct SpriteRec {
    struct SpriteRec* next;    /* +0x00 */
    DDSurface*        surface; /* +0x04 */
    void*             image;   /* +0x08 */
    int               detail;  /* +0x0c */
    unsigned int      flags;   /* +0x10 0x20 function-drawn, 0x40 colour-keyed */
    short             w;       /* +0x14 */
    short             h;       /* +0x16 */
    short             src_x;   /* +0x18 */
    short             src_y;   /* +0x1a */
    unsigned short    refs;    /* +0x1c */
    short             pad1e;   /* +0x1e */
} SpriteRec;

/* sprite2.c's ImageRec: w/h @ +0x08/+0x0a. */
typedef struct ImageRec {
    void*  lls;                /* +0x00 */
    void*  pal;                /* +0x04 */
    short  w;                  /* +0x08 */
    short  h;                  /* +0x0a */
} ImageRec;

/* ---- globals ------------------------------------------------------------ */

extern DDraw*        g_ddraw1;            /* 0x00667d70 */
extern DDraw2*       g_ddraw;             /* 0x00667d74 */
extern DDCaps        g_drvcaps;           /* 0x00667d78 */
extern DDCaps        g_helcaps;           /* 0x00667ef4 */
extern DDSurface*    g_draw_surface;      /* 0x0066807c */
extern DDClipper*    g_clipper;           /* 0x00668080 */
extern void*         g_gdi_obj0;          /* 0x0066808c */
extern void*         g_gdi_obj1;          /* 0x00668090 */
extern void*         g_gdi_obj2;          /* 0x00668094 */
extern void*         g_gdi_obj3;          /* 0x00668098 */
/* The lock descriptor and the render clip are ONE object (0x0066809c..
 * 0x00668118): GenerateNewImageFromZBuffer restores the descriptor with a
 * rep movsd and the clip field by field, and the clip stores stay AFTER the
 * copy in the original -- as separate globals VC6 hoists two of them above
 * it. surface.c declares the same two ranges as separate globals; both views
 * address the same bytes. */
typedef struct LockState {
    DDSurfaceDesc ddsd;         /* +0x00 0x0066809c */
    WinRect       render_clip;  /* +0x6c 0x00668108 */
} LockState;
extern LockState     g_lock;              /* 0x0066809c */
#define g_ddsd        g_lock.ddsd
#define g_render_clip g_lock.render_clip
/* The target stack and its index are one object (0x00668118..0x00668144). */
typedef struct TargetStack {
    int        sp;          /* +0x00 0x00668118 */
    DDSurface* stack[10];   /* +0x04 0x0066811c */
} TargetStack;
extern TargetStack   g_target;            /* 0x00668118 */
extern int           g_video_locked;      /* 0x00668144 */
extern int           g_status_stack[];    /* 0x00668164 */
extern int           g_status_sp;         /* 0x006681e4 */
extern WinRect       g_clip_rect;         /* 0x004bdea0 */
extern Screen*       g_screen;            /* 0x004bcbf4 */
extern int           g_transparent_colour;/* 0x007fea44 */
extern int           g_detail;            /* 0x008119a4 */

extern int            g_zbuf_ready;       /* 0x00798590 */
extern ImageRec*      g_zimage;           /* 0x0066be50 */
extern unsigned short g_zbuf_pixels[];    /* 0x0066be54 */
extern SpriteRec*     g_zsprite;          /* 0x00701e64 */
extern unsigned short g_zimg_pixels[];    /* 0x00701e68 */
extern DDSurfaceDesc  g_saved_ddsd;       /* 0x0066b5b0 */
extern WinRect        g_saved_clip;       /* 0x0066b620 */

extern const Guid  IID_IDirectDraw2;      /* 0x004acf80 */
extern const char  g_font_remove[];       /* 0x004b9ce4 "lego.ttf" */
extern const char  g_font_add[];          /* 0x004b9cac "Lego.ttf" */
extern const char  g_msg_no_ddcom[];      /* 0x004b9cd0 "Can't Create DDCOM" */
extern const char  g_msg_no_caps[];       /* 0x004b9cb8 "Can't get DDCOM caps" */

/* ---- externals ---------------------------------------------------------- */

__declspec(dllimport) int __stdcall IntersectRect(WinRect* dst, const WinRect* a,
                                                  const WinRect* b);  /* [0x4ab2a0] */
__declspec(dllimport) int __stdcall OffsetRect(WinRect* rc, int dx, int dy); /* [0x4ab29c] */
__declspec(dllimport) int __stdcall DeleteObject(void* obj);            /* [0x4ab09c] */
__declspec(dllimport) int __stdcall AddFontResourceA(const char* name); /* [0x4ab0c8] */
__declspec(dllimport) int __stdcall RemoveFontResourceA(const char* name); /* [0x4ab070] */
/* Called through the linker's jump thunk (0x0049d314 -> [0x4ab04c]). */
long __stdcall DirectDrawCreate(Guid* guid, DDraw** out, void* unk);

extern void* HeapAlloc_w(unsigned int size);              /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                         /* 0x0049e4d0 */
extern void  DBPrintf(const char* fmt, ...);              /* 0x00453a20 */
extern int   GetTransparentColour(void);                  /* 0x0044e690 */
extern void  PushRenderingStatusAndUnlockVideoSurface(void); /* 0x00464080 */
extern void  PopRenderingStatus(void);                    /* 0x004641f0 */
/* 0x004640f0: push the lock status, unlock if locked, then lock (surface.c's
 * lock body). Used to lock a freshly swapped-in render target. */
extern void  PushRenderingStatusAndRelockVideoSurface(void);
extern void  SoftPrint_Clear(void);                       /* 0x004651d0 */
extern void  SoftPrint_XBltFast(SpriteRec* s, WinRect* src, WinRect* dst, int a); /* 0x00465a40 */
/* 0x00464ee0: software blit of `src` (sprite space) to dst->left/top. */
extern void  __fastcall SoftBlitSprite(SpriteRec* s, WinRect* src, WinRect* dst);
extern void  InitZBuffer(void);                           /* 0x004887a0 */
extern unsigned char GetZBufferPixel(int x, int y);       /* 0x00488820 */
extern void  RenderZBufferObject(int a, int b, int c);    /* 0x00485fe0 */
extern int   RenderBlock(int x, int y, int w, int h, int colour); /* 0x004890c0 */

#define DDERR_SURFACELOST   0x887601c2

/* surface.c's PushRenderingStatusAndUnlockVideoSurface, which the original
 * inlines into PushSetTarget (it is exported out-of-line at 0x00464080 too). */
static __inline void PushStatusAndUnlock_inl(void)
{
    int status;

    status = g_video_locked;
    g_status_stack[g_status_sp++] = status;
    if (status != 0) {
        if (g_draw_surface->vtbl->Unlock(g_draw_surface, g_ddsd.lpSurface)
                == DDERR_SURFACELOST) {
            g_draw_surface->vtbl->Restore(g_draw_surface);
            g_draw_surface->vtbl->Unlock(g_draw_surface, g_ddsd.lpSurface);
        }
    }
    g_video_locked = 0;
}

/* -------------------------------------------------------------------------
 * 0x00466600 -- restore the previous render target. An underflow is fatal:
 * exit(2) (0x004a02b8 is the CRT exit; it must be declared noreturn or VC6
 * emits a `pop ecx` cleanup after the call that the original lacks).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00466600
void PopTarget(void)
{
    PopRenderingStatus();
    g_draw_surface = g_target.stack[--g_target.sp];
    PopRenderingStatus();
    if (g_target.sp < 0)
        exit(2);
}

/* -------------------------------------------------------------------------
 * 0x004637e0 -- release DirectDraw and the game font.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004637e0
void KillHostSystemGPU(void)
{
    RemoveFontResourceA(g_font_remove);
    if (g_ddraw) {
        g_ddraw->vtbl->Release(g_ddraw);
        g_ddraw = 0;
    }
    DeleteObject(g_gdi_obj1);
    DeleteObject(g_gdi_obj3);
    DeleteObject(g_gdi_obj0);
    DeleteObject(g_gdi_obj2);
    if (g_ddraw1) {
        g_ddraw1->vtbl->Release(g_ddraw1);
        g_ddraw1 = 0;
    }
}

/* -------------------------------------------------------------------------
 * 0x0048a5c0 -- set the clip window, clamped to the screen: top/bottom first,
 * then left/right. `x < 0 ? 0 : x` is the branchless setl/dec/and, and the
 * `> lim ? lim : x` ternary is what stores the limit first and conditionally
 * overwrites it (an if/else on a temp reads the field after the store and
 * loses the match). g_screen is read once into a root copy.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0048a5c0
void SetClipping(WinRect* r)
{
    Screen* scr = g_screen;

    g_clip_rect.top = r->top < 0 ? 0 : r->top;
    g_clip_rect.bottom = r->bottom > scr->h ? scr->h : r->bottom;
    g_clip_rect.left = r->left < 0 ? 0 : r->left;
    g_clip_rect.right = r->right > scr->w ? scr->w : r->right;
}

/* -------------------------------------------------------------------------
 * 0x00466560 -- park the current draw surface and draw into a sprite's.
 *
 * The body of PushRenderingStatusAndUnlockVideoSurface is inlined at the top
 * (the original built it from an __inline definition that is also exported
 * out-of-line at 0x00464080), and the last statement is a void call, so the
 * function ends in `jmp 0x4640f0` with no `ret`. tools/audit.py bounds that
 * and certifies it exact (36/36 instructions, 146 bytes); the shared
 * match.py/verify.py cannot, so it is held as WIP for tooling only.
 *
 * `g_target.stack[g_target.sp++] = ...` reloads g_target.sp for the
 * increment ONLY when the stack and its index are fields of one struct --
 * as two separate globals VC6 keeps the index in the register it already
 * has (compare surface.c's status stack, which does exactly that).
 * ------------------------------------------------------------------------- */

// WIP-FUNCTION: LEGOLAND 0x00466560  (100% by audit.py; match.py cannot bound a tail-jmp function)
void PushSetTarget(SpriteRec* s)
{
    PushStatusAndUnlock_inl();
    g_target.stack[g_target.sp++] = g_draw_surface;
    g_draw_surface = s->surface;
    PushRenderingStatusAndRelockVideoSurface();
}

/* -------------------------------------------------------------------------
 * 0x00464370 -- hand the clip window to the DirectDraw clipper as a
 * one-rectangle region. The original allocates 0x33 bytes for a 0x30-byte
 * RGNDATA (kept as is); the two 16-byte rect copies are whole-struct
 * assignments (four register moves each, in field order).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00464370
void CommitCliprectToHardware(void)
{
    RgnData* rgn = (RgnData*)HeapAlloc_w(0x33);

    rgn->dwSize = 0x20;
    rgn->iType = 1;
    rgn->nCount = 1;
    rgn->nRgnSize = 0x10;
    rgn->rcBound = g_clip_rect;
    rgn->rect = g_clip_rect;
    g_clipper->vtbl->SetClipList(g_clipper, rgn, 0);
    HeapFree_w(rgn);
}

/* -------------------------------------------------------------------------
 * 0x00489390 -- a rectangle outline `t` pixels thick, as four fills: top,
 * left, right, bottom. `h - 2 * t` is spelled inline in the two side fills;
 * VC6 CSEs it after the first call and homes it in the dead `colour`
 * argument slot ([esp+0x50]). Hoisting it into a named local computes it
 * before the first call and reassigns every register.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00489390
void RenderThickBox(int x, int y, int w, int h, int t, int colour)
{
    RenderBlock(x, y, w, t, colour);
    RenderBlock(x, y + t, t, h - 2 * t, colour);
    RenderBlock(x - t + w, y + t, t, h - 2 * t, colour);
    RenderBlock(x, y - t + h, w, t, colour);
}

/* -------------------------------------------------------------------------
 * 0x00463700 -- create DirectDraw, get the IDirectDraw2 interface and both
 * caps blocks (DirectX 5 DDCAPS, dwSize 0x17c), register the game font.
 *
 * The block structure is load-bearing. VC6 redirects a bare leading
 * `return K` to the function's FINAL block whenever that block returns the
 * same constant (and only then -- a middle block with the same tail is never
 * used). The original keeps its own inline `mov eax,1 / ret` for the
 * `if (g_ddraw)` guard and has the DirectDrawCreate failure jump to the
 * `xor eax,eax / ret` that the QueryInterface failure falls into, so the
 * function must END in `return 0` with the success path in an else arm:
 * that makes the shared return-0 the final block, leaves the guard alone,
 * and gives the AddFontResourceA arm its own return-1 copy. A flat
 * `if (fail) return 0;` chain scores 51/54 with the guard merged away.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00463700
int InitHostSystemGPU(void)
{
    if (g_ddraw)
        return 1;
    if (DirectDrawCreate(0, &g_ddraw1, 0) == 0) {
        if (g_ddraw1->vtbl->QueryInterface(g_ddraw1, &IID_IDirectDraw2, (void**)&g_ddraw) != 0) {
            g_ddraw1->vtbl->Release(g_ddraw1);
            DBPrintf(g_msg_no_ddcom);
        } else {
            g_drvcaps.dwSize = 0x17c;
            g_helcaps.dwSize = 0x17c;
            if (g_ddraw->vtbl->GetCaps(g_ddraw, &g_drvcaps, &g_helcaps) != 0) {
                g_ddraw->vtbl->Release(g_ddraw);
                g_ddraw1->vtbl->Release(g_ddraw1);
                DBPrintf(g_msg_no_caps);
                return 0;
            }
            AddFontResourceA(g_font_add);
            return 1;
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * 0x00488b90 -- clipped software sprite blit through SoftPrint_XBltFast.
 * dst is the sprite's screen rect clipped to g_clip_rect; src starts as the
 * whole sprite {0,0,w,h}, is overwritten with the clipped dst (a struct
 * copy) and translated back into sprite space with OffsetRect(-x,-y). The
 * sprite's detail stamp (+0x0c) is refreshed on every draw. Always returns 1.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00488b90
int RenderSpriteX(SpriteRec* s, int x, int y, int a)
{
    WinRect dst;
    WinRect src;

    dst.left = x;
    dst.top = y;
    dst.right = x + s->w;
    dst.bottom = y + s->h;
    src.left = 0;
    src.top = 0;
    src.right = s->w;
    src.bottom = s->h;
    if (IntersectRect(&dst, &dst, &g_clip_rect)) {
        s->detail = g_detail;
        src = dst;
        OffsetRect(&src, -x, -y);
        SoftPrint_XBltFast(s, &src, &dst, a);
    }
    return 1;
}

/* -------------------------------------------------------------------------
 * 0x004890c0 -- hardware colour fill of a clipped rectangle: unlock, attach
 * the clipper, Blt(DDBLT_COLORFILL|DDBLT_WAIT) with a DDBLTFX that only has
 * dwSize and dwFillColor set (no memset -- the original leaves the rest
 * uninitialised), detach, relock. Returns 1 on success or when fully
 * clipped, 0 when the Blt fails; both exits are spelled out (VC6 does not
 * merge them).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004890c0
int RenderBlock(int x, int y, int w, int h, int colour)
{
    WinRect rc;
    DDBltFx fx;

    rc.left = x;
    rc.top = y;
    rc.right = x + w;
    rc.bottom = y + h;
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = colour;
    if (!IntersectRect(&rc, &rc, &g_clip_rect))
        return 1;
    PushRenderingStatusAndUnlockVideoSurface();
    g_draw_surface->vtbl->SetClipper(g_draw_surface, g_clipper);
    if (g_draw_surface->vtbl->Blt(g_draw_surface, &rc, 0, 0, 0x1000400, &fx) == 0) {
        g_draw_surface->vtbl->SetClipper(g_draw_surface, 0);
        PopRenderingStatus();
        return 1;
    }
    g_draw_surface->vtbl->SetClipper(g_draw_surface, 0);
    PopRenderingStatus();
    return 0;
}

/* -------------------------------------------------------------------------
 * 0x00488a10 -- clipped sprite blit: DirectDraw Blt for sprites with flag
 * 0x20 or 0x40 (0x40 adds DDBLT_KEYSRC), the software blitter otherwise.
 * The IntersectRect/OffsetRect prologue is duplicated in both arms (it is in
 * the original: two textual copies). The DDBLT flag choice must be an
 * if/else with two Blt calls -- VC6 tail-merges them into one call with a
 * differing push; a ?: on the flag is turned into branchless and/or/shl.
 * The DDBLTFX here IS memset to zero (unlike RenderBlock's).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00488a10
int RenderSprite(SpriteRec* s, int x, int y)
{
    WinRect dst;
    WinRect src;
    DDBltFx fx;

    dst.left = x;
    dst.top = y;
    dst.right = x + s->w;
    dst.bottom = y + s->h;
    src.left = 0;
    src.top = 0;
    src.right = s->w;
    src.bottom = s->h;
    if (s->flags & 0x60) {
        if (IntersectRect(&dst, &dst, &g_clip_rect)) {
            s->detail = g_detail;
            src = dst;
            OffsetRect(&src, -x, -y);
            memset(&fx, 0, sizeof(fx));
            fx.dwSize = sizeof(fx);
            PushRenderingStatusAndUnlockVideoSurface();
            if (s->flags & 0x40)
                g_draw_surface->vtbl->Blt(g_draw_surface, &dst, s->surface, &src, 0x1008000, &fx);
            else
                g_draw_surface->vtbl->Blt(g_draw_surface, &dst, s->surface, &src, 0x1000000, &fx);
            PopRenderingStatus();
            return 1;
        }
    } else {
        if (IntersectRect(&dst, &dst, &g_clip_rect)) {
            s->detail = g_detail;
            src = dst;
            OffsetRect(&src, -x, -y);
            SoftBlitSprite(s, &src, &dst);
        }
    }
    return 1;
}

/* -------------------------------------------------------------------------
 * 0x00488840 -- see the header comment. Arguments after the sprite: `a`,
 * `b`, `c` are passed straight to 0x00485fe0 (the object drawn over the
 * sprite -- a, b, c look like an object id and a position); `zmax` is the
 * Z threshold a pixel must not exceed to survive.
 *
 * Levers: (1) the save is `ddsd first, then clip`; (2) the pixel loop uses a
 * single index and an unsigned short temporary chosen by if/else -- a ?:
 * promotes to int and adds xor/and zero-extensions; (3) g_ddsd and
 * g_render_clip must be ONE struct (LockState) so the clip restores stay
 * after the rep movsd; (4) VC6 homes y, key and the row offset in the dead
 * s/b/c argument slots on its own.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00488840
SpriteRec* GenerateNewImageFromZBuffer(SpriteRec* s, int a, int zmax, int b, int c)
{
    WinPoint at;
    WinRect  src;
    int      w = s->w;
    int      h = s->h;
    int      key;
    int      x;
    int      y;
    unsigned short v;

    if (g_zbuf_ready == 0)
        InitZBuffer();

    g_saved_ddsd = g_ddsd;
    g_saved_clip = g_render_clip;

    g_ddsd.lpSurface = g_zbuf_pixels;
    g_ddsd.dwWidth = w;
    g_ddsd.dwHeight = h;
    g_ddsd.lPitch = w * 2;
    g_render_clip.left = 0;
    g_render_clip.right = w;
    g_render_clip.top = 0;
    g_render_clip.bottom = h;
    at.x = 0;
    at.y = 0;
    src.left = 0;
    src.right = w;
    src.top = 0;
    src.bottom = h;
    g_transparent_colour = GetTransparentColour();
    SoftPrint_Clear();
    SoftBlitSprite(s, &src, (WinRect*)&at);
    RenderZBufferObject(a, b, c);
    key = GetTransparentColour();
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (GetZBufferPixel(x, y) <= zmax)
                v = g_zbuf_pixels[y * w + x];
            else
                v = (unsigned short)key;
            g_zimg_pixels[y * w + x] = v;
        }
    }
    g_zimage->w = (short)w;
    g_zimage->h = (short)h;
    g_zsprite->w = (short)w;
    g_zsprite->h = (short)h;
    g_ddsd = g_saved_ddsd;
    g_render_clip = g_saved_clip;
    return g_zsprite;
}
