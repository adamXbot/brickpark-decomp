/* LEGOLAND — the money (brick) HUD bar and the money sound effects.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field offsets, callee arg counts and global addresses are load-bearing;
 * names are ours.
 *
 * "Money" in LEGOLAND is the brick count: a plain SIGNED int at 0x004b90f8
 * (10000 in the image's initialised data) guarded by a lock flag at 0x004b90fc
 * (1 in the image). AddBricks/UseBricks (sweep1.c) only touch the counter when
 * the lock is clear, and GetBrickCount returns 0x7fffffff while it is set —
 * that is the free-play / unlimited-bricks mode, and it is why RenderMoneyBar
 * bails out before it can multiply 0x7fffffff by the bar width. Free play gets
 * RenderFreePlayBar (0x0046e7b0) instead, which draws a time bar rather than a
 * quantity bar.
 */
#include "legoland.h"

void* memset(void*, int, unsigned int);
int   sprintf(char*, const char*, ...);

/* ---- HUD gadget --------------------------------------------------------- */
/* One HUD item as the panel code lays it out. The whole 0x0046e670..0x0046e9xx
 * family — RenderMoneyBar, RenderFreePlayBar (0x0046e7b0), RenderGBarSpriteIcon
 * (0x0046e850), RenderFlashingSpriteIcon (0x0046e8a0) — takes one of these and
 * reads exactly these fields, so the offsets are firm even though +0x00/+0x08
 * are never touched from here. Every x/y/w/h read is a `movsx`, so all four are
 * signed 16-bit. */
typedef struct Gadget {
    int     f00;      /* +0x00 */
    Sprite* sprite;   /* +0x04 backing sprite, may be NULL */
    int     f08;      /* +0x08 */
    short   x;        /* +0x0c screen x */
    short   y;        /* +0x0e screen y */
    short   w;        /* +0x10 width in pixels — also the bar's full length */
    short   h;        /* +0x12 height in pixels */
} Gadget;

/* The 5th PrintSprite argument: a 12-byte render-context token, `kind` plus an
 * 8-byte payload. The icon renderers next door build {2, gadget, 0} — kind 2
 * carries the gadget that owns the blit. The money bar wants no owner at all
 * and clears the whole payload in one go: the original is
 *
 *     xor eax,eax / mov [esp],1 / mov [esp+4],eax / mov [esp+8],eax
 *
 * and VC6 will NOT materialise that shared zero for two independent scalar
 * stores here (`ctx.owner = 0; ctx.f08 = 0;` compiles to two immediate stores,
 * which is one instruction short and shifts the register allocation of the
 * whole rest of the function). An 8-byte block clear is what produces it —
 * hence the payload being modelled as one nested struct. */
typedef struct BlitCtx {
    int kind;                          /* +0x00 */
    struct { void* p; int n; } sub;    /* +0x04, +0x08 */
} BlitCtx;

/* The clipping rectangle is the bare 16-byte form, NOT legoland.h's Rect —
 * that one carries a `next` at +0x10 and is 20 bytes, which pushes every later
 * local up by four and grows the frame from 0x80 to 0x84. */
typedef struct ClipRect {
    int left;    /* +0x00 */
    int top;     /* +0x04 */
    int right;   /* +0x08 */
    int bottom;  /* +0x0c */
} ClipRect;

/* ---- globals ------------------------------------------------------------ */
/* Denominator of the bar: the brick count at which the bar is completely full.
 * Uninitialised in the image, so it is set up at map-load time. */
extern int g_bricks_full;      /* 0x00832974 */
/* The *displayed* bar length in pixels. Chases the true length by 6 pixels per
 * frame so the bar slides instead of jumping. It is a file-scope global rather
 * than gadget state, so only one money bar can ever be animating. */
extern int g_money_bar_len;    /* 0x006688c4 */

/* ---- callees ------------------------------------------------------------ */
extern int  BricksAreLimited(void);   /* 0x00457890 — !g_brick_lock */
extern int  GetBrickCount(void);      /* 0x004578e0 */
extern void StoreClipping(void);      /* 0x0048a660 */
extern void SetClipping(ClipRect* r); /* 0x0048a5c0 */
extern void RestoreClipping(void);    /* 0x0048a690 */
extern int  PrintSprite(Sprite* s, int x, int y, int mode, BlitCtx* ctx); /* 0x004853a0 */
/* Cached text blitter (0x00455e50, not an export): looks the string up in the
 * rendered-text cache at 0x006675b8, keyed on the whole tuple
 * (text, w, h, f1, f2, ink, paper) — note x and y are NOT part of the key —
 * rasterises it on a miss (0x00455bb0), then PrintSprites the cached entry's
 * bitmap (entry +0x1c) at (x,y). */
extern void PrintCachedText(const char* text, int x, int y, int w, int h,
                            int f1, int f2, int ink, int paper); /* 0x00455e50 */

/* Draw the brick counter: a bar sprite revealed left-to-right in proportion to
 * the player's bricks, plus the count printed to its right.
 *
 * The art is a full-length image that gets *clipped*, not stretched — the
 * sprite is always blitted whole at the gadget origin with the clip rectangle
 * cut down to the filled length.
 *
 * `return limited;` on the early-out rather than `return 0;` is load-bearing:
 * the value is already in eax and known to be zero there, so VC6 emits a bare
 * `add esp,0x80 / ret` with no `xor eax,eax`. */
// FUNCTION: LEGOLAND 0x0046e670
int RenderMoneyBar(Gadget* g)
{
    BlitCtx  ctx;
    ClipRect clip;
    char     text[100];
    int      limited;
    int      want;
    int      bricks;

    ctx.kind = 1;
    memset(&ctx.sub, 0, sizeof(ctx.sub));

    limited = BricksAreLimited();
    if (!limited)
        return limited;

    /* Length the bar should have, clamped into [0, gadget width]. Both clamps
     * are needed: the count is signed and can legitimately go negative, and
     * g_bricks_full is only the *nominal* full mark, not a hard cap. */
    want = GetBrickCount() * g->w / g_bricks_full;
    if (want < 0)
        want = 0;
    if (want > g->w)
        want = g->w;

    /* Ease the drawn length towards it, 6 pixels a frame, never overshooting. */
    if (want > g_money_bar_len) {
        g_money_bar_len += 6;
        if (g_money_bar_len > want)
            g_money_bar_len = want;
    } else {
        g_money_bar_len -= 6;
        if (g_money_bar_len < want)
            g_money_bar_len = want;
    }

    /* Clip to the filled part and blit the whole sprite through it. Building
     * right/bottom off clip.left/clip.top rather than off g->x/g->y again is
     * what gives VC6 the original's load schedule in this block. */
    StoreClipping();
    clip.left   = g->x;
    clip.right  = clip.left + g_money_bar_len;
    clip.top    = g->y;
    clip.bottom = clip.top + g->h;
    SetClipping(&clip);
    if (g->sprite != 0)
        PrintSprite(g->sprite, g->x, g->y, 0, &ctx);
    RestoreClipping();

    /* …and the count itself, right-aligned in five columns, ten pixels past the
     * end of the gadget and three above its top. The cached-text cell is a
     * fixed 200 x gadget-height box; 0x00ff0000 / 0x00ffffff are the ink and
     * paper colours. */
    bricks = GetBrickCount();
    if (bricks < 0)
        bricks = 0;
    sprintf(text, "%5d", bricks);
    PrintCachedText(text, g->x + g->w + 10, g->y - 3, 200, g->h,
                    0, 0, 0xff0000, 0xffffff);
    return 0;
}

/* ---- money sound effects ------------------------------------------------ */

/* An FX table row, as in audiomisc.c (stride 0xc). */
typedef struct FXEntry {
    char* name;    /* +0x00 */
    int   pad4;    /* +0x04 */
    void* sample;  /* +0x08  filled in by Load_FXList */
} FXEntry;

/* 0x004b87a8 — two rows, loaded and freed by the refcounted LoadMoneySFX /
 * KillMoneySFX pair in audiomisc.c:
 *   [0] "Coin drop for food stands or entrance.wav"
 *   [1] "Cash Register.wav"
 * so `which` is 0 for a small payment and 1 for a till ring. */
extern FXEntry g_money_fx[];

/* Where a sound is coming from. 16 bytes; PlayInstanceOfSample copies the whole
 * record into the playing instance at +0x0c as four dword moves
 * (0x00496d3e..0x00496d55). kind 2 is "a map square", with the square's
 * coordinates in x/y. The +0x04 slot belongs to the other source kinds and is
 * deliberately left alone here — the original never initialises it, on either
 * entry point. */
typedef struct SoundSource {
    int kind;   /* +0x00 */
    int f04;    /* +0x04 */
    int x;      /* +0x08 */
    int y;      /* +0x0c */
} SoundSource;

/* Map coordinates arrive as a pointer to a packed byte pair — the same (bx,by)
 * layout Cell carries at +0x04/+0x05, so a caller can hand over `&cell->bx`
 * directly. Both bytes are read with a zero-extending load, so they are
 * unsigned; the map is at most 256x256. */
typedef struct MapPoint {
    unsigned char x;  /* +0x00 */
    unsigned char y;  /* +0x01 */
} MapPoint;

extern void* PlayInstanceOfSample(void* sample, int a, int b,
                                  SoundSource* src);            /* 0x00496d20 */
extern void  UnSourceAndFadeAllSamplesFromSource(SoundSource* src,
                                                 int fade);     /* 0x00496c80 */

/* Start money effect `which` at map square `at`. `flags` and the literal 1 go
 * straight through to PlaySample (0x00492710) as its 2nd and 3rd arguments. */
// FUNCTION: LEGOLAND 0x00453950
void PlayMoneySFX(MapPoint* at, int which, int flags)
{
    SoundSource src;

    src.kind = 2;
    src.x = at->x;
    src.y = at->y;
    PlayInstanceOfSample(g_money_fx[which].sample, flags, 1, &src);
}

/* …and fade out every sample sourced at that square, whichever of the two it
 * was. -400 is the fade amount handed to the mixer (DirectSound attenuation,
 * hundredths of a dB — i.e. -4 dB). */
// FUNCTION: LEGOLAND 0x004539a0
void StopMoneySFX(MapPoint* at)
{
    SoundSource src;

    src.kind = 2;
    src.x = at->x;
    src.y = at->y;
    UnSourceAndFadeAllSamplesFromSource(&src, -400);
}
