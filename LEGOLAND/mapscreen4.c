/* LEGOLAND — the certificate screen's icon set, the report screen's per-frame
 * text and hint button, and the two map-input helpers off ReadGameButtons.
 *
 * CERTIFICATE SCREEN (front-end screen 8, "CertificateScreen.lls").
 *   mapscreen2.c's PrintScreenMode8 (0x00490410) draws this screen's text
 *   every frame; InitScreen8 (0x00490350) is the other half — it loads the
 *   backdrop, raises g_cert_active and installs two icons, both group 7 and
 *   both flags 0x6002 (0x2 = clickable, 0x2000 = drawn unfocussed, 0x4000 =
 *   takes the help bar):
 *     GoBack_On_Certificate.lls (0xc,   0xe)  help 0x26   CertGoBackInput
 *     Print_On_Certificate.lls  (0x207, 0xe)  help 0x23d  CertPrintInput
 *   It then loads printinfo.lls into g_cert_info_sprite and zeroes both phase
 *   counters, so the screen starts idle.  Note what it does NOT do: unlike
 *   every other front-end screen (InitScreen7, InitScreen9) it publishes
 *   neither icon into g_icon_handler1/2, so Return and Escape keep whatever
 *   the previous screen bound them to.
 *   CertPrintInput (0x00490300) is what starts the save: with g_cert_result
 *   and g_cert_saving both at rest it sets g_cert_saving = 2, and
 *   PrintScreenMode8 counts that down into SaveCertificateBitmap.
 *
 * REPORT SCREEN (front-end screen 7, mapscreen2.c's InitScreen7 builds it).
 *   PrintScreenMode7 (0x004910f0) draws the page: line 0 of g_rep_lines is
 *   the heading, in the big font, in the box (0xa, 0x45)-(0x1d6, 0x6c); then
 *   up to FOURTEEN lines starting at g_rep_lines[g_rep_page], each 0x18 apart
 *   from y = 0x6b, stopping early at g_rep_line_count.  g_rep_page is not a
 *   page NUMBER but a 1-based line cursor — InitScreen7 sets it to 1 and the
 *   Next/Previous handlers step it by 14 (0x00490b72) — so line 0 is only
 *   ever the heading and never body text.  After the page it
 *   blinks the Next/Previous icons and, while g_rep_hint_shown has not
 *   expired, floats the current hint in a bubble anchored on
 *   (0x1db, 5)-(0x280, 0x78).  The expiry is compared SIGNED (`jle`), so the
 *   sound clock's wrap past 2^31 would leave a hint up forever.
 *   ReportHintInput (0x00490be0) is the hint button: every call (hover
 *   included) swaps the icon to its lit sprite Rep_Hint2.lls, and a click
 *   (event bit 1) plays the UI click, advances g_rep_hint_index with wrap,
 *   SKIPPING empty strings, arms the bubble for 8000 ms and plays the hint's
 *   speech clip.  The skip loop has no exhaustion guard: a hint table whose
 *   strings are all empty spins forever — an original bug, reproduced.
 *
 * MAP INPUT (bighelp.c's ReadGameButtons is the caller of both).
 *   ScrollFromKeys (0x00451f70) is the arrow-key scroll, enabled by
 *   g_input.flags bit 0x20 and driven off each direction button's "held" bit
 *   (state bit 2).  Horizontal steps are TWICE the vertical one — the map is
 *   isometric, so g_game->scroll_step is a half-tile — and left/right and
 *   up/down are each an if/else pair, so an exact diagonal scrolls both axes
 *   but two opposed keys resolve to the first of the pair.  It returns 1 if
 *   any of the four was held, which is what suppresses the mouse edge scroll
 *   for that tick.
 *   BeginMapClick (0x00452390) latches the drag origin on mouse-down.  In
 *   edit mode 2 with a class selected it copies the class footprint rect
 *   (ObjDef+0x3c) into the drag block at 0x00813af0, derives the footprint
 *   step in map refs (right-left+1, bottom-top+1 — what the drag-place walker
 *   at 0x00457f64 strides by), and biases the drag origin by the class corner
 *   so a multi-cell object drags from its own base square; otherwise the
 *   origin is just the map ref under the cursor.  Either way the click point
 *   is stored, and a pending press on button 0 is converted into the
 *   "drag in progress" flag 0x1000 and consumed.
 */
#include "legoland.h"

extern unsigned int strlen(const char* s);
#pragma intrinsic(strlen)

/* ---- types (shared with mapscreen2.c / mapscreen3.c; the offsets are the
 * load-bearing part) ------------------------------------------------------ */

/* iconui.c's 0x40-byte icon record. */
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
    void*          owner;     /* +0x18 */
    unsigned char  slot;      /* +0x1c */
    char           pad1d[0x28 - 0x1d];
    int          (*render)(struct Icon*);          /* +0x28 */
    char         (*input)(struct Icon*, int);      /* +0x2c */
    void*          widget;    /* +0x30 */
    unsigned int   flags;     /* +0x34 */
    char*          help;      /* +0x38 */
    int            help_id;   /* +0x3c */
} Icon;

/* The packed map square (anim2.c's BPos / BPosW). */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

/* A footprint rectangle: legoland.h's Rect without its list link, which the
 * drag block at 0x00813af0 does not have. */
typedef struct Rect4 { int left; int top; int right; int bottom; } Rect4;

/* An object class record seen through its footprint rect (objmap2.c's
 * ObjDef.rect). */
typedef struct ObjDef {
    char  pad0[0x3c];
    Rect4 rect;               /* +0x3c */
} ObjDef;

/* Win32 RECT (text.c's WinRect); BubbleHelp takes it BY ADDRESS. */
typedef struct WinRect { long left; long top; long right; long bottom; } WinRect;

/* One virtual game button (bighelp.c's GameButton): the mask it is bound to
 * and the state word ReadGameButtons rebuilds every tick. */
typedef struct GameButton { int mask; int state; } GameButton;

/* The game-button/cursor block @ 0x00813a40 (bighelp.c's GameInput); only
 * the fields this file touches are named. */
typedef struct GameInput {
    int        flags;         /* +0x00  0x1000 = drag in progress */
    char       pad04[0x24 - 0x04];
    int        map_x;         /* +0x24  map ref under the cursor */
    int        map_y;         /* +0x28 */
    char       pad2c[0x34 - 0x2c];
    int        f34;           /* +0x34  drag origin, class-corner adjusted */
    int        f38;           /* +0x38 */
    int        click_x;       /* +0x3c  map ref at the last click */
    int        click_y;       /* +0x40 */
    char       pad44[0x58 - 0x44];
    GameButton up;            /* +0x58  0x813a98 / 0x813a9c */
    GameButton right;         /* +0x60  0x813aa0 / 0x813aa4 */
    GameButton down;          /* +0x68  0x813aa8 / 0x813aac */
    GameButton left;          /* +0x70  0x813ab0 / 0x813ab4 */
    GameButton tab;           /* +0x78 */
    GameButton btn0;          /* +0x80  0x813ac0 / 0x813ac4 */
} GameInput;

/* The game record @ 0x004bcbf4 (legoland.h's g_map) seen through the keyboard
 * scroll step at +0x24, in half-tiles. */
typedef struct GameRec {
    char           pad0[0x24];
    unsigned short scroll_step;   /* +0x24 */
} GameRec;

/* ---- globals ------------------------------------------------------------ */
extern Sprite* g_backdrop;             /* 0x00810148 full-screen background */

extern void*   g_snd_click;            /* 0x004b92c0 the UI click sample */

extern char*   g_rep_lines[];          /* 0x007cafa0 the report line table */
extern int     g_rep_line_count;       /* 0x0079887c */
extern int     g_rep_page;             /* 0x004bf670 current page (1-based) */
extern Sprite* g_rep_hint2;            /* 0x007cb1c4 Rep_Hint2.lls (the lit hint) */
extern char*   g_rep_hints[];          /* 0x007cb140 the hint string table */
extern int     g_rep_hint_count;       /* 0x00798880 */
extern int     g_rep_hint_index;       /* 0x00798884 */
extern int     g_rep_hint_shown;       /* 0x00798888 hint expiry, in ms */

extern int     g_edit_mode;            /* 0x008119b0 castleobj.c EditMode */
extern ObjDef* g_sel_def;              /* 0x00667c58 class under the edit cursor */
extern BPosW   g_sel_bpos;             /* 0x00667c54 its base cell */
extern BPosW   g_drag_sq;              /* 0x00813a34 base cell latched at click */
extern int     g_drag_step_x;          /* 0x00813a38 footprint width, in refs */
extern int     g_drag_step_y;          /* 0x00813a3c footprint height, in refs */
extern Rect4   g_drag_rect;            /* 0x00813af0 the class footprint rect */
extern GameInput g_input;              /* 0x00813a40 */
extern GameRec*  g_game;               /* 0x004bcbf4  legoland.h's g_map */

extern int     g_cert_active;          /* 0x00798770 */
extern int     g_cert_result;          /* 0x00798768 */
extern int     g_cert_saving;          /* 0x0079876c */
extern Sprite* g_cert_info_sprite;     /* 0x00798764 printinfo.lls */

/* ---- string constants --------------------------------------------------- */
extern const char g_lls_certificate_screen[];    /* 0x004bf654 "CertificateScreen.lls" */
extern const char g_lls_goback_on_certificate[]; /* 0x004bf638 "GoBack_On_Certificate.lls" */
extern const char g_lls_print_on_certificate[];  /* 0x004bf61c "Print_On_Certificate.lls" */
extern const char g_lls_printinfo[];             /* 0x004bf60c "printinfo.lls" */

/* ---- callees ------------------------------------------------------------ */
extern Sprite* LoadSprite(const char* name, int mode);                 /* 0x00497ab0 */
extern Icon*   LoadSpriteIcon(const char* name, int mode, int x, int y, int group); /* 0x0046d7b0 */
extern char*   GetString(int id);                                      /* 0x00498f50 */
extern void    SetIconSprite(Icon* p, Sprite* s);                      /* 0x0046d680 */
#ifndef LEGOLAND_PORTABLE
extern void    PlayInstanceOfSample(void* def, int a, int b, void* src); /* 0x00496d20 */
#else
extern int PlayInstanceOfSample(void* def, int a, int b, void* src); /* 0x00496d20 */
#endif
/* 0x00499450: the sound clock in ms.  Declared SIGNED here because
 * PrintScreenMode7's expiry test is `jle`, not `ja`; audiomisc.c declares the
 * same address `unsigned int` (its use has no compare) — a caller-side type
 * divergence, deliberately not aligned. */
extern int     SoundTimeMS(void);                                      /* 0x00499450 */
extern void    BubbleHelp(WinRect* r, char* text, int mode);           /* 0x00455370 */
extern void    ProcessScrolling(int dx, int dy);                       /* 0x004614a0 */
/* 0x00491080 (not exported): print one report line, centred, in the box
 * (x, y)-(x + 0x1cc, y + h); the last argument picks the big font. */
extern void    PrintReportLine(char* text, int x, int y, int h, int big); /* 0x00491080 */
/* 0x00490ea0 (not exported): blink the Next/Previous page icons unless the
 * mouse is over the Next icon. */
extern void    BlinkReportPageIcons(void);                             /* 0x00490ea0 */
/* 0x00490a60 (not exported): format "<prefix>%02d.wav" for hint index+1 into
 * a 0x100-byte local, reset the front end and start the speech clip. */
extern void    PlayReportHint(int index);                              /* 0x00490a60 */

/* Certificate-screen icon handlers (this screen's own, 0x004902c0/0x00490300). */
extern char CertGoBackInput(Icon*, int);   /* 0x004902c0 */
extern char CertPrintInput(Icon*, int);    /* 0x00490300 */

/* =========================================================================
 *  Certificate screen (front-end screen 8) — icon set
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00490350
void InitScreen8(void)
{
    Icon* p;

    g_backdrop = LoadSprite(g_lls_certificate_screen, 0);
    g_cert_active = 1;
    p = LoadSpriteIcon(g_lls_goback_on_certificate, 4, 0xc, 0xe, 7);
    p->help_id = 0x26;
    p->help = GetString(0x26);
    p->flags |= 0x6002;
    p->input = CertGoBackInput;
    p = LoadSpriteIcon(g_lls_print_on_certificate, 4, 0x207, 0xe, 7);
    p->help_id = 0x23d;
    p->help = GetString(0x23d);
    p->flags |= 0x6002;
    p->input = CertPrintInput;
    g_cert_info_sprite = LoadSprite(g_lls_printinfo, 4);
    g_cert_result = 0;
    g_cert_saving = 0;
}

/* =========================================================================
 *  Report screen (front-end screen 7) — the hint button
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00490be0
char ReportHintInput(Icon* icon, int ev)
{
    SetIconSprite(icon, g_rep_hint2);
    if (ev & 2) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        /* ORIGINAL BUG (reproduced): the empty-string skip has no
         * exhaustion guard, so a hint table whose entries are all empty
         * spins here forever. */
        do {
            g_rep_hint_index++;
            if (g_rep_hint_index >= g_rep_hint_count)
                g_rep_hint_index = 0;
        } while (strlen(g_rep_hints[g_rep_hint_index]) == 0);
        g_rep_hint_shown = SoundTimeMS() + 0x1f40;
        PlayReportHint(g_rep_hint_index);
    }
    return 1;
}

/* =========================================================================
 *  Map input — BeginMapClick / ScrollFromKeys (bighelp.c's ReadGameButtons)
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00452390
void BeginMapClick(void)
{
    ObjDef* d;

    if (g_edit_mode == 2 && (d = g_sel_def) != 0) {
        g_drag_sq.w = g_sel_bpos.w;
        g_drag_rect.left = d->rect.left;
        g_drag_rect.right = d->rect.right;
        g_drag_rect.top = d->rect.top;
        g_drag_rect.bottom = d->rect.bottom;
        g_drag_step_x = g_drag_rect.right - g_drag_rect.left + 1;
        g_drag_step_y = g_drag_rect.bottom - g_drag_rect.top + 1;
        g_input.f34 = g_drag_sq.b.x + g_drag_rect.left;
        g_input.f38 = g_drag_sq.b.y + g_drag_rect.top;
        g_input.click_x = g_input.map_x;
        g_input.click_y = g_input.map_y;
    } else {
        g_input.f34 = g_input.map_x;
        g_input.f38 = g_input.map_y;
        g_input.click_x = g_input.map_x;
        g_input.click_y = g_input.map_y;
    }
    if (g_input.btn0.state & 1) {
        g_input.flags |= 0x1000;
        g_input.btn0.state &= ~1;
    }
}

/* =========================================================================
 *  Report screen (front-end screen 7) — per-frame text
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004910f0
void PrintScreenMode7(void)
{
    WinRect rc;
    int     line;
    int     y;
    int     i;

    PrintReportLine(g_rep_lines[0], 0xa, 0x45, 0x27, 1);
    line = g_rep_page;
    y = 0x6b;
    for (i = 0; i < 14; i++, line++, y += 0x18) {
        if (line > g_rep_line_count)
            break;
        PrintReportLine(g_rep_lines[line], 0xa, y, 0x16, 0);
    }
    BlinkReportPageIcons();
    if (g_rep_hint_shown) {
        rc.left = 0x1db;
        rc.top = 5;
        rc.right = 0x280;
        rc.bottom = 0x78;
        /* ORIGINAL BUG (reproduced): the expiry compare is SIGNED (`jle`),
         * so once the sound clock passes 2^31 ms the bubble never expires.
         * That is why 0x00499450 is declared `int` in this file. */
        if (SoundTimeMS() > g_rep_hint_shown)
            g_rep_hint_shown = 0;
        BubbleHelp(&rc, g_rep_hints[g_rep_hint_index], 2);
    }
}

// FUNCTION: LEGOLAND 0x00451f70
int ScrollFromKeys(void)
{
    int scrolled = 0;

    if (!(g_input.flags & 0x20))
        return 0;
    if (g_input.left.state & 4)
        ProcessScrolling(-g_game->scroll_step * 2, 0);
    else if (g_input.right.state & 4)
        ProcessScrolling(g_game->scroll_step * 2, 0);
    if (g_input.up.state & 4)
        ProcessScrolling(0, -g_game->scroll_step);
    else if (g_input.down.state & 4)
        ProcessScrolling(0, g_game->scroll_step);
    if ((g_input.left.state & 4) || (g_input.right.state & 4)
        || (g_input.up.state & 4) || (g_input.down.state & 4))
        scrolled = 1;
    return scrolled;
}
