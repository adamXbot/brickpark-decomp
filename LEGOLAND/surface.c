/* LEGOLAND -- render surface lock/unlock and the rendering-status stack.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Field offsets and calling conventions are load-bearing; names are ours.
 *
 * ---------------------------------------------------------------------------
 * What this cluster is
 * ---------------------------------------------------------------------------
 * The game draws straight into the DirectDraw back buffer's linear memory.
 * Everything that rasterises (sprite blitters, the tile sweep, the ODF/LLIDB
 * loader's preview draws) needs the surface LOCKED; anything that talks to
 * GDI or to DirectDraw itself (Blt, palette work, mode changes) needs it
 * UNLOCKED. Callers therefore bracket their work with a push/pop pair:
 *
 *     PushRenderingStatusAndLockVideoSurface();     ... draw ...
 *     PopRenderingStatus();
 *
 *     PushRenderingStatusAndUnlockVideoSurface();   ... Blt/GDI ...
 *     PopRenderingStatus();
 *
 * The "status" that is pushed is simply the CURRENT lock flag, so Pop restores
 * whatever state the caller was in -- the pairs nest safely to 32 deep
 * (g_status_stack is 0x668164..0x6681e3, exactly 32 ints, immediately followed
 * by its index at 0x6681e4).
 *
 * ---------------------------------------------------------------------------
 * Recovered layout
 * ---------------------------------------------------------------------------
 *   0x00668164  int   g_status_stack[32]   saved lock flags
 *   0x006681e4  int   g_status_sp          next free slot (post-increment)
 *   0x00668144  int   g_video_locked       0 = unlocked, 1 = locked
 *   0x0066807c  IDirectDrawSurface* g_draw_surface   the buffer we rasterise into
 *   0x0066809c  DDSURFACEDESC g_ddsd       filled by Lock; dwSize preset to 0x6c
 *   0x006680ac  == &g_ddsd + 0x10          lPitch
 *   0x006680c0  == &g_ddsd + 0x24          lpSurface  (the pixel pointer)
 *   0x00668108  WinRect g_render_clip      clip window actually used for drawing
 *   0x004bdea0  WinRect g_clip_rect        the caller-set clipping window
 *   0x004bcbf4  the global game record; its first two u16 are the SCREEN pixel
 *               extent (legoland.h calls the same record g_map and uses its
 *               MAP extent at +0x14/+0x16 -- both views are correct)
 *   0x007fea44  int   g_transparent_colour
 *
 * The 0x6680ac / 0x6680c0 globals are not separate variables at all: they are
 * DDSURFACEDESC::lPitch (+0x10) and DDSURFACEDESC::lpSurface (+0x24) inside the
 * single static descriptor at 0x0066809c, which is why `dwSize = 0x6c`
 * (sizeof(DDSURFACEDESC)) is stored right before every Lock. Modelling them as
 * one struct is what makes the &g_ddsd argument and the two field reads share a
 * base; three unrelated ints would not reproduce the addressing.
 *
 * ---------------------------------------------------------------------------
 * Lock semantics
 * ---------------------------------------------------------------------------
 * Locking does three things, in this order:
 *   1. recompute the drawing clip: g_render_clip = full screen INTERSECT
 *      g_clip_rect, where "full screen" is the INCLUSIVE rect {0, 0, w-1, h-1}
 *      taken from the screen extent -- note the -1s, so the game's WinRects are
 *      inclusive even though IntersectRect treats them as exclusive;
 *   2. Lock(NULL, &g_ddsd, DDLOCK_WAIT|DDLOCK_WRITEONLY, NULL), retrying once
 *      through Restore() on DDERR_SURFACELOST;
 *   3. re-read the transparent colour key, which depends on the surface's
 *      pixel format and so can change across a lost/restored surface.
 * Unlocking is the same lost-surface retry around Unlock(g_ddsd.lpSurface).
 *
 * Both retries reload g_draw_surface and g_ddsd.lpSurface for the second call
 * rather than caching them -- Restore() may repoint them.
 * ------------------------------------------------------------------------- */

/* ---- local types -------------------------------------------------------- */

/* The Win32 RECT the blitter and IntersectRect work on (see util.c). */
typedef struct WinRect {
    long left;     /* +0x00 */
    long top;      /* +0x04 */
    long right;    /* +0x08 */
    long bottom;   /* +0x0c */
} WinRect;

/* The global record at 0x004bcbf4 seen through its screen-extent view: the
 * first two u16 are the display width and height in pixels. */
typedef struct Screen {
    unsigned short w;   /* +0x00 */
    unsigned short h;   /* +0x02 */
} Screen;

/* DDSURFACEDESC -- 0x6c bytes. Only dwSize, lPitch and lpSurface are touched
 * here; the rest is padding held at the right size so dwSize is honest. */
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
    char          pad28[0x6c - 0x28];  /* +0x28 ckDest*/
} DDSurfaceDesc;

typedef struct DDSurface DDSurface;

/* IDirectDrawSurface's vtable; only the three slots used here are named.
 * 0x64 = Lock, 0x6c = Restore, 0x80 = Unlock (0x7c = SetPalette, which is what
 * spritemisc.c's ResendPalette calls on the *primary* surface at 0x00668070 --
 * a different surface object from the 0x0066807c one locked here). */
typedef struct DDSurfaceVtbl {
    char pad00[0x64];                                                   /* +0x00 */
    long(__stdcall* Lock)(DDSurface*, WinRect*, DDSurfaceDesc*,
                          unsigned long, void*);                        /* +0x64 */
    char pad68[0x6c - 0x68];                                            /* +0x68 */
    long(__stdcall* Restore)(DDSurface*);                               /* +0x6c */
    char pad70[0x80 - 0x70];                                            /* +0x70 */
    long(__stdcall* Unlock)(DDSurface*, void*);                         /* +0x80 */
} DDSurfaceVtbl;

struct DDSurface {
    DDSurfaceVtbl* vtbl;   /* +0x00 */
};

/* What GetVideoSurface hands back: a plain linear-bitmap descriptor the
 * software rasterisers draw through. +0x10 is deliberately left alone -- the
 * original never writes it, so callers must already have it set (it is the
 * only field of the six that Lock has nothing to say about). +0x14 is a
 * constant 2, the surface's bytes-per-pixel / format selector for the 16-bit
 * modes this build ships. */
typedef struct VideoSurfaceInfo {
    long  pitch;    /* +0x00  bytes per scanline */
    int   width;    /* +0x04  pixels */
    int   height;   /* +0x08  pixels */
    void* bits;     /* +0x0c  top-left pixel */
    int   unused;   /* +0x10  NOT written */
    int   format;   /* +0x14  always 2 */
} VideoSurfaceInfo;

/* ---- globals ------------------------------------------------------------ */

extern int           g_status_stack[];    /* 0x00668164 */
extern int           g_status_sp;         /* 0x006681e4 */
extern int           g_video_locked;      /* 0x00668144 */
extern DDSurface*    g_draw_surface;      /* 0x0066807c */
extern DDSurfaceDesc g_ddsd;              /* 0x0066809c */
extern WinRect       g_render_clip;       /* 0x00668108 */
extern WinRect       g_clip_rect;         /* 0x004bdea0 */
extern Screen*       g_screen;            /* 0x004bcbf4 */
extern int           g_transparent_colour;/* 0x007fea44 */

/* ---- externals ---------------------------------------------------------- */

__declspec(dllimport) int __stdcall IntersectRect(WinRect* dst, const WinRect* a,
                                                  const WinRect* b);  /* [0x4ab2a0] */

/* LEGOLAND/sweep1.c 0x0044e690 -- re-read the colour-key for the current mode. */
extern int GetTransparentColour(void);

#define DDERR_SURFACELOST   0x887601c2
#define DDLOCK_WRITEONLY_WAIT 0x21   /* DDLOCK_WAIT | DDLOCK_WRITEONLY */

/* -------------------------------------------------------------------------
 * 0x00463fc0 -- push the current lock status, then make sure we are LOCKED.
 *
 * The already-locked case falls straight through to the trailing
 * `g_video_locked = 1` (a redundant store the original keeps), which is why
 * the guard compiles to `jne <tail>` rather than a jump over an else.
 *
 * `status` has to be a local here: the original loads 0x00668144 once and
 * reuses that register for the guard AFTER storing it into the stack array.
 * The same register then supplies the two literal zeroes for rect.left/top --
 * on the fall-through of `test eax,eax / jne` VC6 knows eax is 0 and stores it
 * instead of an immediate. (PopRenderingStatus does NOT get that: it loads
 * g_screen into eax first, so eax is gone by the time the zeroes are stored.
 * The two functions differ only in how VC6 scheduled that block.)
 *
 * `g_ddsd.dwSize = 0x6c` must be written BEFORE the IntersectRect call in the
 * source. Putting it after lets VC6 sink the store into the argument pushes;
 * the original has it hoisted above the call, and VC6 will not move a global
 * store across a call on its own.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00463fc0
void PushRenderingStatusAndLockVideoSurface(void)
{
    WinRect rect;
    int     status;

    status = g_video_locked;
    g_status_stack[g_status_sp++] = status;
    if (status == 0) {
        rect.left = 0;
        rect.top = 0;
        rect.right = g_screen->w - 1;
        rect.bottom = g_screen->h - 1;
        g_ddsd.dwSize = 0x6c;
        IntersectRect(&g_render_clip, &rect, &g_clip_rect);

        if (g_draw_surface->vtbl->Lock(g_draw_surface, 0, &g_ddsd,
                                       DDLOCK_WRITEONLY_WAIT, 0) == DDERR_SURFACELOST) {
            g_draw_surface->vtbl->Restore(g_draw_surface);
            g_draw_surface->vtbl->Lock(g_draw_surface, 0, &g_ddsd,
                                       DDLOCK_WRITEONLY_WAIT, 0);
        }
        g_transparent_colour = GetTransparentColour();
    }
    g_video_locked = 1;
}

/* -------------------------------------------------------------------------
 * 0x00464080 -- push the current lock status, then make sure we are UNLOCKED.
 *
 * Mirror image of the above: no rect work, no colour-key re-read, and the
 * trailing store is 0. No stack frame at all -- every temporary is a register.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00464080
void PushRenderingStatusAndUnlockVideoSurface(void)
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
 * 0x004641f0 -- pop a rendering status and transition to it.
 *
 * Four cases, only two of which do work:
 *      saved  now   action
 *        1     0    lock   (then g_video_locked = 1)
 *        1     1    nothing
 *        0     1    unlock (then g_video_locked = 0)
 *        0     0    nothing
 *
 * Written as if/else on `saved` with an inner test on `now`. VC6 duplicates the
 * `add esp,0x10 / ret` epilogue at the end of the lock arm instead of jumping
 * to the shared one -- that is its own block placement, not something the
 * source can (or needs to) spell out.
 *
 * The inner tests read g_video_locked DIRECTLY -- caching it in a local first
 * is what a reader naturally writes, and it costs the match. With a local, VC6
 * schedules the load before the g_status_sp decrement and burns three separate
 * registers (ecx=sp, eax=locked, edx=saved). Reading the global in each arm
 * lets its CSE place the load after the array read, so eax is free to carry the
 * stack index first and then be reloaded with the flag -- the original's exact
 * `mov eax,[sp] / dec / store / mov ecx,[stack+eax*4] / mov eax,[locked]`.
 * That took this function from 88.3% to 100%.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004641f0
void PopRenderingStatus(void)
{
    WinRect rect;
    int     saved;

    saved = g_status_stack[--g_status_sp];
    if (saved != 0) {
        if (g_video_locked == 0) {
            rect.left = 0;
            rect.top = 0;
            rect.right = g_screen->w - 1;
            rect.bottom = g_screen->h - 1;
            g_ddsd.dwSize = 0x6c;
            IntersectRect(&g_render_clip, &rect, &g_clip_rect);

            if (g_draw_surface->vtbl->Lock(g_draw_surface, 0, &g_ddsd,
                                           DDLOCK_WRITEONLY_WAIT, 0) == DDERR_SURFACELOST) {
                g_draw_surface->vtbl->Restore(g_draw_surface);
                g_draw_surface->vtbl->Lock(g_draw_surface, 0, &g_ddsd,
                                           DDLOCK_WRITEONLY_WAIT, 0);
            }
            g_transparent_colour = GetTransparentColour();
            g_video_locked = 1;
        }
    } else {
        if (g_video_locked != 0) {
            if (g_draw_surface->vtbl->Unlock(g_draw_surface, g_ddsd.lpSurface)
                    == DDERR_SURFACELOST) {
                g_draw_surface->vtbl->Restore(g_draw_surface);
                g_draw_surface->vtbl->Unlock(g_draw_surface, g_ddsd.lpSurface);
            }
            g_video_locked = 0;
        }
    }
}

/* -------------------------------------------------------------------------
 * 0x00464310 -- hand the caller a linear-bitmap view of the locked surface.
 *
 * Returns 0 (and touches nothing) when the surface is not currently locked, so
 * every rasteriser can call this as its "can I draw?" test.
 *
 * NOTE ON EXTENT: the first `ret` here is only the early-out four instructions
 * in; the real function is 21 instructions ending at 0x0046435a. A previous
 * attempt was rejected at 67% for stopping at that first `ret`.
 *
 * g_screen is reloaded between the width and height reads -- the store to
 * out->width may alias it, so VC6 cannot keep the pointer live. Writing it as
 * two separate `g_screen->` reads reproduces that; caching the pointer in a
 * local does not.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00464310
int GetVideoSurface(VideoSurfaceInfo* out)
{
    if (g_video_locked == 0)
        return 0;

    out->pitch = g_ddsd.lPitch;
    out->width = g_screen->w;
    out->height = g_screen->h;
    out->bits = g_ddsd.lpSurface;
    out->format = 2;
    return 1;
}
