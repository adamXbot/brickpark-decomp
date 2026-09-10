/* LEGOLAND -- the render / print pipeline: frame bracket, HUD gadget draws
 * and the "print" front-ends to the sprite rasterisers.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field offsets, global addresses and callee arg counts are load-bearing;
 * names are ours.
 *
 * ---------------------------------------------------------------------------
 * "Print" versus "Render"
 * ---------------------------------------------------------------------------
 * The blitter family has two tiers:
 *
 *   Render*Sprite (0x00488c50 RenderTiledSprite, 0x00489080 RenderScaledSprite,
 *   0x00488c80 RenderSpriteScaledOffset ...) rasterise into the LOCKED surface
 *   and trust that the sprite's pixels are resident.
 *
 *   Print*Sprite (0x004853a0 PrintSprite, 0x004858e0 PrintTiledSprite,
 *   0x00485940 PrintScaledSprite) are the front-ends everyone else calls: they
 *   first make sure the sprite is DRAWABLE -- the pixel pointer that
 *   GetVRAMAddress hands back (sprite +0x04) must be non-null, and when it is
 *   null the sprite is (re)created on the spot by 0x00499500 (RecreateSprite
 *   for a "resident-copy" sprite, MakeSprite otherwise) -- and only then call
 *   the Render tier. PrintSprite additionally takes the 12-byte BlitCtx token
 *   (see money.c) that says on whose behalf the blit is being made.
 *
 * ---------------------------------------------------------------------------
 * The frame
 * ---------------------------------------------------------------------------
 * A frame is RenderScreen (0x0048faf0):
 *
 *     UpdateSoundVols; ReadGameButtons;
 *     PushRenderingStatusAndLockVideoSurface();
 *         PrintSprite(backdrop) ; icons ; front-end help
 *     PopRenderingStatus();
 *     RenderingComplete();
 *
 * and RenderingComplete (0x00466500) is the thing that finalises it: pump the
 * OS (ProcessSystemEvents), age the rendered-text cache, then hand the frame
 * to the presentation routine through the g_present function pointer
 * (0x004b9ca4, initialised to 0x00466080 -- the LLSAuto / cursor / frame-rate
 * throttle / Flip routine). The rdtsc blocks around it are the game's own
 * profiling: the cycle counter is read into g_rdtsc_last / g_rdtsc_accum with
 * pushad/popad so the compiler's registers survive. Note that RenderingComplete
 * does NOT pop the status stack itself -- the caller has already done that --
 * it works from the unlocked state the pop restored.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

void* memset(void*, int, unsigned int);

/* ---- local types -------------------------------------------------------- */

/* A HUD gadget as laid out by the panel code (same record as money.c's
 * Gadget). Everything up to +0x12 is common; from +0x18 the record is
 * per-kind. A BOX gadget keeps its outline colour (u16) and thickness (u8)
 * there and its behaviour flags at +0x34 (0x20 = clip the client area to the
 * icon afterwards). Every x/y/w/h read is a movsx, so those are signed 16. */
typedef struct Gadget {
    int            f00;      /* +0x00 */
    Sprite*        sprite;   /* +0x04 backing sprite, may be NULL */
    int            f08;      /* +0x08 */
    short          x;        /* +0x0c screen x */
    short          y;        /* +0x0e screen y */
    short          w;        /* +0x10 width  */
    short          h;        /* +0x12 height */
    int            f14;      /* +0x14 */
    unsigned short colour;   /* +0x18 box: outline colour */
    unsigned char  thick;    /* +0x1a box: outline thickness in pixels */
    unsigned char  f1b;      /* +0x1b */
    char           pad1c[0x34 - 0x1c];
    unsigned char  flags34;  /* +0x34 */
} Gadget;

/* The 5th PrintSprite argument (see money.c for why the payload is one nested
 * struct): kind 1 = plain / no owner, kind 2 = "owned by this gadget". */
typedef struct BlitCtx {
    int kind;                          /* +0x00 */
    struct { void* p; int n; } sub;    /* +0x04, +0x08 */
} BlitCtx;

/* A sprite resource (spritemisc.c's SpriteRes): the VRAM/pixel pointer that
 * GetVRAMAddress returns the ADDRESS of is +0x04; flags at +0x10, bit 15 =
 * ILF (indexed frame table), which the tiled blitter cannot draw. */
typedef struct SpriteRec {
    int          pad0;      /* +0x00 */
    void*        vram;      /* +0x04 */
    int          pad8;      /* +0x08 */
    int          pad0c;     /* +0x0c */
    unsigned int flags;     /* +0x10 */
} SpriteRec;

/* A "bloke" (worker/visitor) record, stride 0xac, as in renderlist.c. */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00 */
    void*          person;      /* +0x04 */
    unsigned char  pad08[0x3e]; /* +0x08..0x45 */
    unsigned short f46;         /* +0x46 non-zero: has a job/target (see RenderWorkers) */
    unsigned char  pad48[0x1a]; /* +0x48..0x61 */
    unsigned char  flags62;     /* +0x62 0x20 = in a vehicle / not drawn */
    unsigned char  pad63[0x49]; /* +0x63..0xab */
} Bloke;

/* ---- globals ------------------------------------------------------------ */

extern Bloke*  g_workers_head;     /* 0x0079a8a8  the workers list */
extern Bloke*  g_workers2_head;    /* 0x0079a8ac  second worker list */

extern SpriteRec* g_backdrop;      /* 0x00810148  full-screen background */
extern int     g_icons2_mode;      /* 0x00668e38  non-zero: alternate icon set */

extern int     g_last_tick;        /* 0x00667d68  GetTickCount at frame end */
extern int     g_rdtsc_last;       /* 0x00813a18  cycle counter at last read */
extern int     g_rdtsc_accum;      /* 0x00813a2c  cycles accumulated this frame */
typedef int  (*PresentFn)(void);
extern PresentFn g_present;       /* 0x004b9ca4  -> 0x00466080 (flip) */

/* ---- externals ---------------------------------------------------------- */

__declspec(dllimport) unsigned long __stdcall GetTickCount(void);   /* [0x4ab1f8] */

extern void  SortBlokeIn3D(Bloke* b);                               /* 0x0043ffd0 */
extern void  RenderThickBox(int x, int y, int w, int h, int t, int colour);
                                                                    /* 0x00489390 */
extern int   ClipToIcon(Gadget* g);                                 /* 0x0046df30 */
extern int   ProcessSystemEvents(void);                             /* 0x00480050 */
extern void  ExpireCachedText(int all);                             /* 0x00455f70 */
extern void* GetVRAMAddress(void* p);                               /* 0x00496f20 */
extern int   MakeSpriteDrawable(SpriteRec* s);                      /* 0x00499500 */
extern int   RenderTiledSprite(SpriteRec* s, int a, int b, int c, int d,
                               int e, int f);                       /* 0x00488c50 */
extern int   RenderScaledSprite(SpriteRec* s, int x, int y, int w, int h);
                                                                    /* 0x00489080 */
extern int   PrintSprite(void* s, int x, int y, int mode, BlitCtx* ctx);
                                                                    /* 0x004853a0 */
extern int   UpdateSoundVols(void);                                 /* 0x00495a90 (returns 0; audio3.c) */
extern void  ReadGameButtons(void);                                 /* 0x00452460 */
extern void  PushRenderingStatusAndLockVideoSurface(void);          /* 0x00463fc0 */
extern void  PopRenderingStatus(void);                              /* 0x004641f0 */
extern void  RenderIcons(void);                                     /* 0x0046eee0 */
extern void  RenderIcons2(int a, int b, int c);                     /* 0x0046f010 */
extern void  UpdateFocussedIconPtr(void);                           /* 0x004700a0 */
extern void  ProcessFrontEndHelp(void);                             /* 0x0046d080 */
extern int   GetBlink(void);                                        /* 0x00499480 */

/* -------------------------------------------------------------------------
 * 0x0046e9d0 -- draw a gadget's sprite with an ownerless (kind 1) context.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0046e9d0
int RenderGBarSprite(Gadget* g)
{
    BlitCtx ctx;

    ctx.kind = 1;
    memset(&ctx.sub, 0, sizeof(ctx.sub));
    if (g->sprite)
        PrintSprite(g->sprite, g->x, g->y, 0, &ctx);
    return 0;
}

/* -------------------------------------------------------------------------
 * 0x0049a080 -- submit the workers to the 3D depth sorter. Two lists: every
 * worker on the first, and on the second only those with a non-zero +0x46.
 * flags62 & 0x20 (riding / hidden) skips the sort on both.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0049a080
void RenderWorkers(void)
{
    Bloke* p;

    for (p = g_workers_head; p; p = p->next) {
        if (!(p->flags62 & 0x20))
            SortBlokeIn3D(p);
    }
    for (p = g_workers2_head; p; p = p->next) {
        if (p->f46 != 0 && !(p->flags62 & 0x20))
            SortBlokeIn3D(p);
    }
}

/* -------------------------------------------------------------------------
 * 0x0046df70 -- a box gadget: an outline `thick` pixels wide drawn OUTSIDE
 * the gadget rectangle, then (flag 0x20) the clip window set to its inside.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0046df70
int RenderBoxIcon(Gadget* g)
{
    unsigned short t = g->thick;

    RenderThickBox(g->x - t, g->y - t, g->w + 2 * t, g->h + 2 * t, t, g->colour);
    if (g->flags34 & 0x20)
        ClipToIcon(g);
    return 0;
}

/* -------------------------------------------------------------------------
 * 0x00466500 -- finish the frame (see the header comment).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00466500
int RenderingComplete(void)
{
    int result;

    ProcessSystemEvents();
    ExpireCachedText(0);
#ifndef LEGOLAND_PORTABLE
    __asm {
        pushad
        rdtsc
        mov ebx, g_rdtsc_last
        sub eax, ebx
        mov g_rdtsc_last, eax
        add g_rdtsc_accum, eax
        popad
    }
#else
    {
        int ll_now = (int)ll_rdtsc();
        g_rdtsc_last = ll_now - g_rdtsc_last;
        g_rdtsc_accum += g_rdtsc_last;
    }
#endif
    g_last_tick = GetTickCount();
    result = g_present();
    g_rdtsc_accum = 0;
#ifndef LEGOLAND_PORTABLE
    __asm {
        pushad
        rdtsc
        mov g_rdtsc_last, eax
        popad
    }
#else
    g_rdtsc_last = (int)ll_rdtsc();
#endif
    return result;
}

/* -------------------------------------------------------------------------
 * 0x004858e0 -- tiled blit front-end. ILF sprites (bit 15) cannot be tiled.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004858e0
int PrintTiledSprite(SpriteRec* s, int a, int b, int c, int d, int e, int f)
{
    if (!(s->flags & 0x8000)) {
        if (*(void**)GetVRAMAddress(s) != 0 || MakeSpriteDrawable(s))
            return RenderTiledSprite(s, a, b, c, d, e, f);
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * 0x00485940 -- scaled blit front-end.
 *
 * Unlike PrintTiledSprite, VC6 did NOT tail-merge the two RenderScaledSprite
 * calls here: the resident path is a second copy at the very end. Statement
 * ORDER inside the null-VRAM block is load-bearing: `if (Make) return Render;
 * return 0;` gives the original's `je <fail>` with a real `xor eax,eax` block
 * sitting between the two copies. Writing it as `if (!Make) return 0;` first
 * lets VC6 fall through from the `test` with eax known-zero, drops the xor
 * and flips the branch (90%); a flat three-return form moves the fail block
 * to the end instead (81%).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00485940
int PrintScaledSprite(SpriteRec* s, int x, int y, int w, int h)
{
    if (*(void**)GetVRAMAddress(s) == 0) {
        if (MakeSpriteDrawable(s))
            return RenderScaledSprite(s, x, y, w, h);
        return 0;
    }
    return RenderScaledSprite(s, x, y, w, h);
}

/* -------------------------------------------------------------------------
 * 0x0048faf0 -- one whole frame of the front end (see the header comment).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0048faf0
void RenderScreen(void)
{
    BlitCtx ctx;

    ctx.kind = 1;
    memset(&ctx.sub, 0, sizeof(ctx.sub));
    UpdateSoundVols();
    ReadGameButtons();
    PushRenderingStatusAndLockVideoSurface();
    PrintSprite(g_backdrop, 0, 0, 0, &ctx);
    if (g_icons2_mode) {
        RenderIcons2(7, 14, 0);
        UpdateFocussedIconPtr();
    } else {
        RenderIcons();
    }
    ProcessFrontEndHelp();
    PopRenderingStatus();
    RenderingComplete();
}

/* -------------------------------------------------------------------------
 * 0x0046e8a0 -- a gadget sprite that flashes: on the blink's off phase it is
 * printed with mode 0xff000000 (a darkening / recolour mode) instead of 0.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0046e8a0
int RenderFlashingSpriteIcon(Gadget* g)
{
    BlitCtx ctx;

    ctx.kind = 2;
    ctx.sub.p = g;
    ctx.sub.n = 0;
    if (g->sprite) {
        if (GetBlink())
            PrintSprite(g->sprite, g->x, g->y, 0, &ctx);
        else
            PrintSprite(g->sprite, g->x, g->y, 0xff000000, &ctx);
    }
    return 0;
}
