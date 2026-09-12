/* LEGOLAND — the pop-up panel's nine-slice background, its "control bar"
 * extension strip, and the loader for the two control-bar tool icons.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; names are ours.
 *
 *   0x00470950  InitPopUpTools     112/112 insns, 419/419 bytes, exact
 *   0x00471d90  DrawPopUpExtra     120/120 insns, 377/377 bytes, exact
 *   0x00471f10  DrawPopUpFrame     137/137 insns, 377/377 bytes, exact
 *
 * CODEGEN NOTE that closed two of the three.  The 12-byte `BlitCtx` handed to
 * PrintSprite is written as ONE PARTIAL INITIALISER, `BlitCtx ctx = { 1 };` —
 * the implicit zero-fill of the other two fields is emitted at the top of the
 * IR, where an initialiser has to go, while the explicit `1` schedules with
 * the call's argument block.  Three assignment statements instead sink all
 * three stores below the pushes and cost 11 of 137 in `DrawPopUpFrame`, and
 * none of the 720 orders of the function's six leading statements, nor a
 * pointer local, a `static __inline` filler, an int array or a flattened
 * struct, gets below that.  Both drawing functions need it.
 *
 * ---------------------------------------------------------------------------
 * HOW THE POP-UP PANEL IS PAINTED
 *
 * The info pop-up (popup.c `DrawPopUpInfo`, 0x004724a0) is a stretchable
 * nine-slice box whose only size parameter is `g_pu_size` — the BYTE at
 * 0x007fdfac, the panel height in "lines".  Everything is derived from it and
 * from the panel origin `g_popup_x` / `g_popup_y` (0x007fdecc / 0x007fded0,
 * placed by `ClampPopUpToScreen`, misc3.c):
 *
 *     panel width  = n * 0x20 + 0xc8      (0xbc left slice + n*0x20 + 0x0c)
 *     panel height = n * 20   + 0x96
 *
 * `DrawPopUpFrame` paints that box out of the nine sprites `g_pu_bg[0..8]`
 * (0x006688e0..0x00668900, loaded by bighelp.c `InitPopUpInfo` in the order
 * PU_BGMain, PU_BGCentreTop, PU_BGRightTop, PU_BGLeftMid, PU_BGCentreMid,
 * PU_BGRightMid, PU_BGLeftBtm, Pu_BGCentreBtm, PU_BGRightBtm .lls).  The
 * geometry, read off the walk of the three rows:
 *
 *     row 0 (the header)   y = py            height 0x63
 *         [0] at px          — 0xbc wide, and it carries the whole title bar
 *         [1] × n at px + 0xbc + j*0x20
 *         [2] at px + 0xbc + n*0x20
 *     rows 1..n (the body) y = py + 0x63 + i*0x14        (0x14 = 20 per line)
 *         [3] at px, [4] × n stepping 0x20, [5] at the end
 *     row n+1 (the footer) y = py + 0x63 + n*20
 *         [8] at px, [7] × n stepping 0x20, [6] at the end
 *
 * ORIGINAL BUG, reproduced: the footer's two corner slices are SWAPPED.  The
 * header row is painted [0],[1],[2] and the body rows [3],[4],[5] — left,
 * middle, right — but the footer is painted [8],[7],[6], and [8] is
 * PU_BGRightBtm.lls while [6] is PU_BGLeftBtm.lls.  So the bottom-RIGHT
 * corner artwork goes at the left end of the last row and the bottom-LEFT
 * corner at the right end.  (Verified from the .lls names the loader passes:
 * the middle slice [7] is symmetric so only the two corners are affected.)
 *
 * Every slice goes through `PrintSprite(spr, x, y, 0, &ctx)` with the same
 * 12-byte blit context {kind = 1, 0, 0} (money.c / fpui.c's BlitCtx); kind 1
 * is the plain unclipped path.
 *
 * ---------------------------------------------------------------------------
 * THE CONTROL BAR (the "expanded" pop-up)
 *
 * When `g_popup.expanded` (0x007fdfa4) is set, `DrawPopUpInfo` calls
 * `DrawPopUpExtra` after the icon row.  That paints a SECOND, one-line strip
 * below the panel body out of three more sprites — CB_BGleft / CB_BGCentre /
 * CB_BGRight (0x00668904/8/c) — and puts two gadgets on it:
 *
 *     strip y      = py + n*20 + 0x48
 *     CB_BGleft    at px                 (0x7a wide)
 *     CB_BGCentre  × n from px + 0x7a stepping 0x20
 *     CB_BGRight   at px + 0x7a + n*0x20 (0x4e wide)
 *     right edge   = px + 0x7a + n*0x20 + 0x4e  ( = px + n*0x20 + 0xc8,
 *                    i.e. the strip is exactly as wide as the panel)
 *     OK icon      (0x007fdea8) at (right - 0x4b, y + 3)
 *     Close icon   (0x007fe000) at (right - 0x27, y + 3)
 *
 * and one line of text — GetString(0xa2) through Format("%s") into a 20-byte
 * stack buffer — at (px + 0x0c, y + 6), 0x1b tall, in the same ink/paper the
 * panel body uses (0xff0000 on 0xffffff, styles 1 and 5).
 *
 * ORIGINAL BUG.  The text box's WIDTH is `n * 20 + 0x7a` — the vertical line
 * pitch (20 decimal) multiplied in where the horizontal cell pitch (0x20)
 * belongs.  Every other horizontal measurement in the pop-up, including this
 * function's own sprite walk, uses n*0x20; the strip is n*0x20 + 0xc8 wide
 * but the caption is laid out as if it were n*20 + 0x86 wide.  For n = 0 the
 * caption box even overhangs the OK icon by 9 pixels.  The one `lea` that
 * produces it (`lea edx,[eax+ebp+0x7a]` with ebp = n*20 from the y
 * computation) shows how it happened: the already-computed vertical term was
 * reused.  Reproduced.
 *
 * Leaving the strip with the mouse un-highlights the two gadgets: the hit
 * test is [OK.x .. Close.x + 0x24] × [y .. y + 0x1b] and a miss on EITHER
 * axis calls `ResetToolIcons` (0x00471d60), which is the control bar's own
 * two-icon version of misc3.c's `ClosePopUpIcons`.  `DrawPopUpExtra` then
 * ends by calling `DisablePopUpInputs` (0x00471470), which nulls the `input`
 * handler of the delete / gardener / mechanic / delete-confirm icons so the
 * panel's normal gadgets cannot be clicked while the control bar is up.
 *
 * ---------------------------------------------------------------------------
 * WHERE THE CONTROL BAR COMES FROM
 *
 * `InitPopUpTools` is the tail of bighelp.c's `InitPopUpInfo`.  It is passed
 * the two input handlers (PU_ToolA 0x00473310 and PU_ToolB 0x004731e0),
 * lazily loads the seven control-bar sprites — each behind its own
 * "already loaded?" test on the destination global, so a second call is free
 * — and creates the two gadgets in the pop-up's icon group 0x2c3:
 *
 *     0x00668938  PU_OK.lls        OK button, unlit      -> icon 0x007fdea8
 *     0x00668934  PU_OKON.lls      OK button, lit
 *     0x0066893c  CB_Close.lls     close button, unlit   -> icon 0x007fe000
 *     0x00668940  CB_CloseON.lls   close button, lit
 *     0x00668904  CB_BGleft.lls    strip left slice
 *     0x00668908  CB_BGCentre.lls  strip middle slice
 *     0x0066890c  CB_BGRight.lls   strip right slice
 *
 * Both gadgets get help strings 0x74 / 0x75, flags 0x2000 | 0x4002 | 0x400
 * (0x400 = hidden until `DrawPopUpExtra` un-hides them) and the caller's
 * handler.  Note the four sprite globals are loaded in the order
 * OK, OKON, Close, CloseON but the icons are built from the UNLIT pair only;
 * the lit pair is what `ResetToolIcons`' counterpart swaps in on hover.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* A loaded sprite (printlist.c SpriteRec) — opaque here. */
typedef struct Sprite Sprite;

/* An icon/gadget record (iconui.c Icon); only the fields used here matter. */
typedef struct Icon {
    struct Icon*   next;      /* +0x00 */
    Sprite*        sprite;    /* +0x04 */
    void*          data;      /* +0x08 */
    short          x;         /* +0x0c */
    short          y;         /* +0x0e */
    short          w;         /* +0x10 */
    short          h;         /* +0x12 */
    unsigned short group;     /* +0x14 */
    short          pad16;     /* +0x16 */
    int            f18;
    int            f1c;
    int            f20;
    int            f24;
    int          (*render)(struct Icon*);          /* +0x28 */
    char         (*input)(struct Icon*, int);      /* +0x2c */
    void*          widget;    /* +0x30 */
    unsigned int   flags;     /* +0x34 */
    char*          help;      /* +0x38 */
    int            help_id;   /* +0x3c */
} Icon;

typedef char (*IconInputFn)(Icon*, int);

/* The 5th PrintSprite argument (bighelp.c / fpui.c / money.c): a kind and an
 * 8-byte payload the list renderers clear as one block. */
typedef struct BlitCtx {
    int kind;                            /* +0x00 */
    struct { void* p; int n; } sub;      /* +0x04, +0x08 */
} BlitCtx;

/* ---------------------------------------------------------------- globals -- */

/* The pop-up panel's nine background slices (bighelp.c g_pu_bg). */
extern Sprite* g_pu_bg[9];              /* 0x006688e0 .. 0x00668900 */

/* The control bar's three strip slices and its four button sprites. */
extern Sprite* g_cb_bg[3];              /* 0x00668904 .. 0x0066890c */
extern Sprite* g_pu_ok;                 /* 0x00668938  PU_OK.lls */
extern Sprite* g_pu_ok_on;              /* 0x00668934  PU_OKON.lls */
extern Sprite* g_cb_close;              /* 0x0066893c  CB_Close.lls */
extern Sprite* g_cb_close_on;           /* 0x00668940  CB_CloseON.lls */

extern const char g_lls_pu_ok[];        /* 0x004baa70 "PU_OK.lls" */
extern const char g_lls_pu_okon[];      /* 0x004baa64 "PU_OKON.lls" */
extern const char g_lls_cb_close[];     /* 0x004baa54 "CB_Close.lls" */
extern const char g_lls_cb_closeon[];   /* 0x004baa44 "CB_CloseON.lls" */
extern const char g_lls_cb_bgleft[];    /* 0x004baa34 "CB_BGleft.lls" */
extern const char g_lls_cb_bgcentre[];  /* 0x004baa24 "CB_BGCentre.lls" */
extern const char g_lls_cb_bgright[];   /* 0x004baa14 "CB_BGRight.lls" */
extern const char g_fmt_s[];            /* 0x004b8bbc "%s" */

/* The two control-bar gadgets (iconui.c g_info_icon_d / g_info_icon_e). */
extern Icon* g_cb_icon_ok;              /* 0x007fdea8 */
extern Icon* g_cb_icon_close;           /* 0x007fe000 */

/* The pop-up's placement and height (misc3.c ClampPopUpToScreen moves the
 * first two; popup.c DrawPopUpInfo sets the third). */
extern int           g_popup_x;         /* 0x007fdecc  PopUpInfo.pos.x */
extern int           g_popup_y;         /* 0x007fded0  PopUpInfo.pos.y */
/* The panel height in lines.  It is a BYTE (popup.c stores it with
 * `mov byte ptr [7fdfacH]`), but every reader loads the whole dword and masks
 * — the shape a `unsigned char` STRUCT MEMBER gives (popup.c's PopUpUI +0x108)
 * and a bare `unsigned char` global does NOT (that gives xor/mov bl).  Spelt
 * here as the masked dword read, which is the same object. */
extern int g_pu_size;                   /* 0x007fdfac  panel height in lines */

extern Pos g_input_point;               /* 0x00813a44  g_input.point */

/* ---------------------------------------------------------------- callees -- */
extern Sprite* LoadSprite(const char* name, int mode);                     /* 0x00497ab0 */
extern Icon*   InsertIcon(short x, short y, unsigned short group, Sprite* s); /* 0x0046d6c0 */
extern char*   GetString(int id);                                          /* 0x00498f50 */
extern int     PrintSprite(Sprite* s, int x, int y, int mode, BlitCtx* ctx);/* 0x004853a0 */
extern int     Format(char* dst, const char* fmt, ...);                    /* 0x0049e573 */
extern void    PrintCachedText(const char* text, int x, int y, int w, int h,
                               int f1, int f2, int ink, int paper);        /* 0x00455e50 */
/* 0x00471d60 (not exported): put the control bar's two gadgets back to their
 * unlit sprites — SetIconSprite(g_cb_icon_ok, g_pu_ok) and
 * SetIconSprite(g_cb_icon_close, g_cb_close). */
extern void    ResetToolIcons(void);                                       /* 0x00471d60 */
/* 0x00471470 (not exported): null the `input` handler (+0x2c) of the pop-up's
 * delete (0x007fdfdc), gardener (0x007fdfe0), mechanic (0x007fdea4) and
 * delete-confirm (0x007fdfcc) icons. */
extern void    DisablePopUpInputs(void);                                   /* 0x00471470 */

#define PU_GROUP 0x2c3

/* -------------------------------------------------------------------------
 * 0x00470950 -- load the control-bar sprites (once) and build its two
 * gadgets, wiring `ok_fn` to the OK button and `close_fn` to the close
 * button.  Called at the end of bighelp.c's InitPopUpInfo.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00470950
void InitPopUpTools(IconInputFn ok_fn, IconInputFn close_fn)
{
    Icon* p;

    if (!g_pu_ok)        g_pu_ok        = LoadSprite(g_lls_pu_ok, 4);
    if (!g_pu_ok_on)     g_pu_ok_on     = LoadSprite(g_lls_pu_okon, 4);
    if (!g_cb_close)     g_cb_close     = LoadSprite(g_lls_cb_close, 4);
    if (!g_cb_close_on)  g_cb_close_on  = LoadSprite(g_lls_cb_closeon, 4);
    if (!g_cb_bg[0])     g_cb_bg[0]     = LoadSprite(g_lls_cb_bgleft, 4);
    if (!g_cb_bg[1])     g_cb_bg[1]     = LoadSprite(g_lls_cb_bgcentre, 4);
    if (!g_cb_bg[2])     g_cb_bg[2]     = LoadSprite(g_lls_cb_bgright, 4);

    p = InsertIcon(0, 0, PU_GROUP, g_pu_ok);
    g_cb_icon_ok = p;
    p->help_id = 0x74;
    g_cb_icon_ok->help = GetString(0x74);
    g_cb_icon_ok->flags |= 0x2000;
    g_cb_icon_ok->flags |= 0x4002;
    g_cb_icon_ok->flags |= 0x400;
    g_cb_icon_ok->input = ok_fn;

    p = InsertIcon(0, 0, PU_GROUP, g_cb_close);
    g_cb_icon_close = p;
    p->help_id = 0x75;
    g_cb_icon_close->help = GetString(0x75);
    g_cb_icon_close->flags |= 0x2000;
    g_cb_icon_close->flags |= 0x4002;
    g_cb_icon_close->flags |= 0x400;
    g_cb_icon_close->input = close_fn;
}

/* -------------------------------------------------------------------------
 * 0x00471f10 -- paint the pop-up's nine-slice background.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00471f10
void DrawPopUpFrame(void)
{
    /* The three context stores are ONE partial initialiser: `{ 1 }` zero-fills
     * sub.p / sub.n, and VC6 puts those two zero stores at the top of the IR
     * (where an initialiser must go) while the explicit `1` schedules with the
     * argument block.  Three assignment statements instead — in any of the 720
     * orders of these six statements, and through a pointer, a helper, an
     * array or a flat struct — sink all three below the pushes and cost 11. */
    BlitCtx ctx = { 1 };
    int     x, y, cx, cy, n, i, j;

    n = g_pu_size & 0xff;
    x = g_popup_x;
    y = g_popup_y;

    PrintSprite(g_pu_bg[0], x, y, 0, &ctx);
    cx = x + 0xbc;
    for (j = n; j > 0; j--) {
        PrintSprite(g_pu_bg[1], cx, y, 0, &ctx);
        cx += 0x20;
    }
    PrintSprite(g_pu_bg[2], cx, y, 0, &ctx);

    if (n != 0) {
        cy = y + 0x63;
        for (i = n; i > 0; i--) {
            PrintSprite(g_pu_bg[3], x, cy, 0, &ctx);
            cx = x + 0xbc;
            for (j = n; j > 0; j--) {
                PrintSprite(g_pu_bg[4], cx, cy, 0, &ctx);
                cx += 0x20;
            }
            PrintSprite(g_pu_bg[5], cx, cy, 0, &ctx);
            cy += 0x14;
        }
    }

    cy = y + n * 20 + 0x63;
    PrintSprite(g_pu_bg[8], x, cy, 0, &ctx);
    cx = x + 0xbc;
    for (j = n; j > 0; j--) {
        PrintSprite(g_pu_bg[7], cx, cy, 0, &ctx);
        cx += 0x20;
    }
    PrintSprite(g_pu_bg[6], cx, cy, 0, &ctx);
}

/* -------------------------------------------------------------------------
 * 0x00471d90 -- paint the expanded pop-up's control-bar strip, place its two
 * gadgets, draw its caption, and drop the panel's other gadgets' input.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00471d90
void DrawPopUpExtra(void)
{
    BlitCtx ctx = { 1 };
    char    buf[20];
    int     x, y, n, j;
    /* The caption rectangle, and then the mouse strip — ONE aggregate, as in
     * popup.c's DrawPopUpInfo: with four separate ints VC6 propagates each
     * corner into the `right - left` / `bottom - top` argument and folds it to
     * a constant, where the original subtracts the two live values. */
    struct { int left, top, right, bottom; } box;

    n = g_pu_size & 0xff;
    y = g_popup_y + n * 20 + 0x48;
    x = g_popup_x;

    PrintSprite(g_cb_bg[0], x, y, 0, &ctx);
    x += 0x7a;
    for (j = n; j > 0; j--) {
        PrintSprite(g_cb_bg[1], x, y, 0, &ctx);
        x += 0x20;
    }
    PrintSprite(g_cb_bg[2], x, y, 0, &ctx);
    x += 0x4e;

    g_cb_icon_ok->flags &= 0xfffffbff;
    g_cb_icon_ok->x = (short)(x - 0x4b);
    g_cb_icon_ok->y = (short)(y + 3);
    g_cb_icon_close->flags &= 0xfffffbff;
    g_cb_icon_close->x = (short)(x - 0x27);
    g_cb_icon_close->y = (short)(y + 3);

    Format(buf, g_fmt_s, GetString(0xa2));
    box.left = g_popup_x + 0xc;
    box.top = y + 6;
#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
    /* QUIRKS.md Q1: the shipped width is n*20 + 0x7a -- the VERTICAL line
     * pitch reused where the horizontal cell pitch (0x20) belongs, so for n = 0
     * the caption box overhangs the OK icon by 9 px. The caption belongs in the
     * strip's free span: from px + 0xc to just short of the OK gadget, which
     * the same function has just placed at x - 0x4b. */
    box.right = g_cb_icon_ok->x - 3;
#else
    box.right = box.left + n * 20 + 0x7a;
#endif
    box.bottom = box.top + 0x1b;
    PrintCachedText(buf, box.left, box.top, box.right - box.left,
                    box.bottom - box.top, 1, 5, 0xff0000, 0xffffff);

    box.bottom = y + 0x1b;
    box.left = g_cb_icon_ok->x;
    box.right = g_cb_icon_close->x + 0x24;
    if (box.right < g_input_point.x || g_input_point.x < box.left)
        ResetToolIcons();
    if (box.bottom < g_input_point.y || g_input_point.y < y)
        ResetToolIcons();
    DisablePopUpInputs();
}
