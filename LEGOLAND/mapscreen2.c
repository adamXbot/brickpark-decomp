/* LEGOLAND — the end-of-period REPORT screen (screen 7) and the CERTIFICATE
 * screen's per-frame text (screen 8).  Siblings of mapscreen.c's front-end
 * screen switch: InitScreens() calls one Init*() per screen number and
 * RenderFrontEndScreen() calls one PrintScreenMode*() per frame, inside the
 * PushRenderingStatus / RenderingComplete bracket.
 *
 * REPORT SCREEN (screen 7, "Interval_Screen.lls").
 *   Layout, all icons in group 7:
 *     Accept_On_Report.lls   (0x20a, 0x16c)  help 0x244 / 0x245
 *     Rep_Hint1.lls          (0x1b9, 0x50)   help 0x2d0   (InsertIcon)
 *     NextPage.lls           (0x1b9, 0x1ae)  help 0x88e   (InsertIcon)
 *     PreviousPage.lls       (6,     0x1ae)  help 0x88f   (InsertIcon)
 *   NextPageLit.lls / PreviousPageLit.lls are the pressed sprites the two
 *   page handlers swap in; Rep_Hint2.lls is the hint icon's lit sprite.
 *   State:
 *     g_rep_page      (0x004bf670)  current page, 1-based, reset to 1 here.
 *     g_rep_line_count(0x0079887c)  number of report lines (PrintScreenMode7
 *                                   draws 14 of them from 0x007cafa0 + page).
 *     g_rep_hint_count(0x00798880)  number of hint strings at 0x007cb140; 0
 *                                   disables the hint icon (flag 0x400).
 *     g_rep_hint_index(0x00798884)  hint cursor, pre-incremented and wrapped
 *                                   by the hint icon's handler, so -1 here
 *                                   makes the first click show hint 0.
 *     g_rep_hint_shown(0x00798888)  non-zero while the hint box is up.
 *     g_rep_accept_mode(0x00798878) set by the 0x00490600 setter; picks the
 *                                   Accept icon's help string.
 *   The two page icons take flags 0x2000 (drawn even when not focussed) and
 *   0x4002 as two separate ORs; UpdateReportPageIcons() then greys the
 *   Previous icon on page 1 (flag 0x400).
 *
 * CERTIFICATE SCREEN (screen 8, "CertificateScreen.lls") — the per-frame text
 * only; InitScreen8 (0x00490350) builds the icons.  It is a three-phase state
 * machine driven by two counters:
 *     g_cert_active  (0x00798770) 1 while the screen is up.
 *     g_cert_saving  (0x0079876c) counts DOWN while "saving" is displayed;
 *                                 on reaching 0 it runs SaveCertificateBitmap
 *                                 (0x00451e20 — writes EGC.bmp and formats the
 *                                 path into the 0x0080ffa0 message buffer),
 *                                 resets the front end, sets g_6687b0 = 4 and
 *                                 loads g_cert_result with +140 on success or
 *                                 -140 on failure.
 *     g_cert_result  (0x00798768) counts back TOWARDS zero, positive showing
 *                                 the success string 0x23b and negative the
 *                                 failure string 0x23c, i.e. each result is
 *                                 shown for 140 frames.
 *   Every phase draws printinfo.lls (g_cert_info_sprite) at (0xc6, 0x28) with
 *   a kind-1 blit context and centres its string in the box
 *   (0xc6, 0x2d)-(0xc6 + sprite->w, 0x43); the 0x0080ffa0 buffer is always
 *   centred in (0, 0x102)-(0x280, 0x130) underneath.
 */
#include "legoland.h"

extern void* memset(void* d, int c, unsigned int n);
#pragma intrinsic(memset)

/* ---- types (shared with screens2.c / bigscreens.c; offsets are the
 * load-bearing part) ------------------------------------------------------ */

/* iconui.c's 0x40-byte icon record. */
typedef struct Icon {
    struct Icon*   next;      /* +0x00 */
    struct Sprite* sprite;    /* +0x04 */
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

/* Sprite (size at +0x14/+0x16) comes from legoland.h. */

/* Win32 RECT, passed BY VALUE to the text printers (text.c's WinRect). */
typedef struct WinRect {
    long left;                /* +0x00 */
    long top;                 /* +0x04 */
    long right;               /* +0x08 */
    long bottom;              /* +0x0c */
} WinRect;

/* The 5th PrintSprite argument (money.c's BlitCtx). */
typedef struct BlitCtx {
    int kind;                            /* +0x00 */
    struct { void* p; int n; } sub;      /* +0x04, +0x08 */
} BlitCtx;

/* ---- globals ------------------------------------------------------------ */
extern Sprite* g_backdrop;             /* 0x00810148 full-screen background */
extern char  (*g_icon_handler1)(Icon*, int); /* 0x006687bc */
extern char  (*g_icon_handler2)(Icon*, int); /* 0x006687c0 */
extern int     g_6687b0;               /* 0x006687b0 */

extern Sprite* g_rep_next;             /* 0x0081c02c NextPage.lls */
extern Sprite* g_rep_next_lit;         /* 0x0081c034 NextPageLit.lls */
extern Sprite* g_rep_prev;             /* 0x0081c080 PreviousPage.lls */
extern Sprite* g_rep_prev_lit;         /* 0x0081c084 PreviousPageLit.lls */
extern Sprite* g_rep_hint1;            /* 0x007caf80 Rep_Hint1.lls */
extern Sprite* g_rep_hint2;            /* 0x007cb1c4 Rep_Hint2.lls */
extern Icon*   g_rep_hint_icon;        /* 0x007cb1c0 */
extern Icon*   g_rep_next_icon;        /* 0x007cb2e4 */
extern Icon*   g_rep_prev_icon;        /* 0x007cb2e0 */
extern int     g_rep_page;             /* 0x004bf670 current page (1-based) */
extern int     g_rep_accept_mode;      /* 0x00798878 */
extern int     g_rep_hint_count;       /* 0x00798880 */
extern int     g_rep_hint_index;       /* 0x00798884 */
extern int     g_rep_hint_shown;       /* 0x00798888 */

extern int     g_cert_active;          /* 0x00798770 */
extern int     g_cert_result;          /* 0x00798768 */
extern int     g_cert_saving;          /* 0x0079876c */
extern Sprite* g_cert_info_sprite;     /* 0x00798764 printinfo.lls */
extern char    g_cert_message[];       /* 0x0080ffa0 */

/* ---- string constants --------------------------------------------------- */
extern const char g_lls_interval_screen[];  /* 0x004bf6cc "Interval_Screen.lls" */
extern const char g_lls_nextpage[];         /* 0x004b8198 "NextPage.lls" */
extern const char g_lls_nextpagelit[];      /* 0x004b8188 "NextPageLit.lls" */
extern const char g_lls_previouspage[];     /* 0x004b8174 "PreviousPage.lls" */
extern const char g_lls_previouspagelit[];  /* 0x004b8160 "PreviousPageLit.lls" */
extern const char g_lls_rep_hint1[];        /* 0x004bf6bc "Rep_Hint1.lls" */
extern const char g_lls_rep_hint2[];        /* 0x004bf6ac "Rep_Hint2.lls" */
extern const char g_lls_accept_on_report[]; /* 0x004bf694 "Accept_On_Report.lls" */

/* ---- callees ------------------------------------------------------------ */
extern Sprite* LoadSprite(const char* name, int mode);                 /* 0x00497ab0 */
extern Icon*   InsertIcon(short x, short y, unsigned short group, Sprite* s); /* 0x0046d6c0 */
extern Icon*   LoadSpriteIcon(const char* name, int mode, int x, int y, int group); /* 0x0046d7b0 */
extern char*   GetString(int id);                                      /* 0x00498f50 */
extern void    NewPrintCent(const char* text, int font, WinRect rc, char white); /* 0x00491d60 */
extern int     PrintSprite(Sprite* s, int x, int y, int mode, BlitCtx* ctx); /* 0x004853a0 */
/* The definition's real return type, for the portable build only: a stale
 * extern name whose signature disagrees with its body makes gen_link.py
 * bridge the two with a CAST, the cast call lowers to `call_indirect`, and
 * binaryen's directize pass turns a constant-index call_indirect into an
 * invalid DIRECT call -- the module then fails validation hundreds of
 * functions away (scope PORT-M2 section 4). 0x00498920 is `int PauseCurrentTrack(void)`. The VC6 arm is the
 * shipped spelling and its bytes cannot move: cdecl discards EAX here. */
#ifndef LEGOLAND_PORTABLE
extern void    ResetFrontEnd(void);                                    /* 0x00498920 */
#else
extern int     ResetFrontEnd(void);                                    /* 0x00498920 */
#endif
extern void    InitOptionSamples(void);                                /* 0x00492830 */
/* 0x00499380 (not exported): pause the game timer (returns 1 if already). */
extern int     PauseGameTimer(void);                                   /* 0x00499380 */
/* 0x00451e20 (not exported): write the certificate to EGC.bmp and format the
 * resulting path into g_cert_message; non-zero on success. */
extern int     SaveCertificateBitmap(void);                            /* 0x00451e20 */
/* 0x00490aa0 (not exported): grey/ungrey the Previous-page icon for the
 * current page. */
extern void    UpdateReportPageIcons(void);                            /* 0x00490aa0 */

/* Report-screen icon input handlers (this file's siblings). */
extern char ReportAcceptInput(Icon*, int);      /* 0x00490970 */
extern char ReportHintInput(Icon*, int);        /* 0x00490be0 */
extern char ReportNextPageInput(Icon*, int);    /* 0x00490b20 */
extern char ReportPrevPageInput(Icon*, int);    /* 0x00490b90 */

/* =========================================================================
 *  Report screen (front-end screen 7)
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00490c70
void InitScreen7(void)
{
    Icon* p;

    PauseGameTimer();
    InitOptionSamples();
    g_backdrop = LoadSprite(g_lls_interval_screen, 0);
    g_rep_next = LoadSprite(g_lls_nextpage, 4);
    g_rep_next_lit = LoadSprite(g_lls_nextpagelit, 4);
    g_rep_prev = LoadSprite(g_lls_previouspage, 4);
    g_rep_prev_lit = LoadSprite(g_lls_previouspagelit, 4);
    g_rep_hint1 = LoadSprite(g_lls_rep_hint1, 4);
    g_rep_hint2 = LoadSprite(g_lls_rep_hint2, 4);
    p = LoadSpriteIcon(g_lls_accept_on_report, 4, 0x20a, 0x16c, 7);
    if (g_rep_accept_mode) {
        p->help_id = 0x244;
        p->help = GetString(0x244);
    } else {
        p->help_id = 0x245;
        p->help = GetString(0x245);
    }
    p->flags |= 0x6002;
    p->input = ReportAcceptInput;
    g_icon_handler2 = ReportAcceptInput;
    p = InsertIcon(0x1b9, 0x50, 7, g_rep_hint1);
    p->help_id = 0x2d0;
    p->help = GetString(0x2d0);
    p->flags |= 0x6002;
    p->input = ReportHintInput;
    g_rep_hint_icon = p;
    if (!g_rep_hint_count)
        p->flags |= 0x400;
    g_rep_hint_shown = 0;
    g_rep_hint_index = -1;
    g_rep_next_icon = InsertIcon(0x1b9, 0x1ae, 7, g_rep_next);
    g_rep_next_icon->help_id = 0x88e;
    g_rep_next_icon->help = GetString(0x88e);
    g_rep_next_icon->flags |= 0x2000;
    g_rep_next_icon->flags |= 0x4002;
    g_rep_next_icon->input = ReportNextPageInput;
    g_icon_handler1 = ReportNextPageInput;
    g_rep_prev_icon = InsertIcon(6, 0x1ae, 7, g_rep_prev);
    g_rep_prev_icon->help_id = 0x88f;
    g_rep_prev_icon->help = GetString(0x88f);
    g_rep_prev_icon->flags |= 0x2000;
    g_rep_prev_icon->flags |= 0x4002;
    g_rep_prev_icon->input = ReportPrevPageInput;
    g_rep_page = 1;
    UpdateReportPageIcons();
}

/* =========================================================================
 *  Certificate screen (front-end screen 8) — per-frame text
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00490410
void PrintScreenMode8(void)
{
    BlitCtx ctx;
    WinRect rc;

    memset(&ctx.sub, 0, sizeof(ctx.sub));
    ctx.kind = 1;
    if (g_cert_active) {
        rc.top = 0x102;
        rc.bottom = 0x130;
        rc.left = 0;
        rc.right = 0x280;
        NewPrintCent(g_cert_message, 0, rc, 0);
        if (g_cert_saving) {
            PrintSprite(g_cert_info_sprite, 0xc6, 0x28, 0, &ctx);
            rc.top = 0x2d;
            rc.bottom = 0x43;
            rc.left = 0xc6;
            rc.right = g_cert_info_sprite->w + 0xc6;
            NewPrintCent(GetString(0x23a), 1, rc, 1);
            if (--g_cert_saving == 0) {
                ResetFrontEnd();
                g_6687b0 = 4;
                g_cert_result = SaveCertificateBitmap() ? 140 : -140;
            }
        } else if (g_cert_result > 0) {
            PrintSprite(g_cert_info_sprite, 0xc6, 0x28, 0, &ctx);
            rc.top = 0x2d;
            rc.bottom = 0x43;
            rc.left = 0xc6;
            rc.right = g_cert_info_sprite->w + 0xc6;
            NewPrintCent(GetString(0x23b), 1, rc, 1);
            g_cert_result--;
        } else if (g_cert_result < 0) {
            PrintSprite(g_cert_info_sprite, 0xc6, 0x28, 0, &ctx);
            rc.top = 0x2d;
            rc.bottom = 0x43;
            rc.left = 0xc6;
            rc.right = g_cert_info_sprite->w + 0xc6;
            NewPrintCent(GetString(0x23c), 1, rc, 1);
            g_cert_result++;
        }
    }
}
