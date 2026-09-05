/* LEGOLAND — two more front-end screens off mapscreen.c's screen switch.
 *
 * ADVERT SCREEN (front-end screen 9, "AdvertScreen.lls").
 *   InitScreens(9) -> InitScreen9 (0x00490150).  Four icons, all in group 7,
 *   all with flags 0x6002 (0x2 = clickable, 0x2000 = drawn unfocussed,
 *   0x4000 = takes the help bar):
 *     GoBack_On_Advert.lls  (0x13,  0x14b)  help 0x26   AdvertGoBackInput
 *     California.lls        (0x1c,  0x24)   help 0x2bc  AdvertCaliforniaInput
 *     Windsor.lls           (0xea,  0x24)   help 0x2bd  AdvertWindsorInput
 *     billund.lls           (0x1b8, 0x24)   help 0x2be  AdvertBillundInput
 *   The three park buttons play a full-screen advert clip — the pointer is
 *   hidden (SetPointer(0)), PlayMovieFile("<park>.avi", 1, 1) runs it, and the
 *   pointer comes back as shape 6.  Go Back drops the three movie records and
 *   returns to the previous screen.
 *   Handler registration is the unusual part: the screen puts Go Back in
 *   g_icon_handler2 (0x006687c0, the "escape/cancel" slot) and explicitly
 *   CLEARS g_icon_handler1 (0x006687bc, the "accept" slot) — this screen has
 *   no default action, so Return does nothing on it.
 *
 * TUTORIAL PROGRESS TEXT (front-end screen 6's per-frame printer, the
 * "levels 1..5" variant).  RenderFrontEndScreen calls PrintScreenMode6
 * (0x0048c100) every frame; it does nothing at all unless g_progress_is_low
 * (0x00798664) says the tutorial report — not the ten-level one — is up.
 *   It draws the screen title (string 0x28a, font 3) centred in
 *   (0xa, 0x45)-(0x1d6, 0x6c), then one line per tutorial level next to the
 *   Appraisal tick built by InitTutorialScreen: the level name (its help
 *   string id, taken from g_low_markers[i].str_id) in a 0x190 x 0x16 box at
 *   (marker.x + 0x32, marker.y + 6), font 2, in one of three colours —
 *     black    0x000000  the level the player is ON (i == level - 1)
 *     dark     0x323232  a level already completed (g_level_done[i] == 1)
 *     grey     0xa0a0a0  a level not reached yet
 *   Only the COMPLETED lines are hot: with the mouse inside such a line's box
 *   the routine forges a hit record (g_hit_info = {2, g_low_icons[i]}), so
 *   clicking the level's TEXT is the same as clicking its tick.  The text of
 *   the current level is drawn in the "on" colour but is deliberately not
 *   clickable, and a line whose g_low_icons slot was left null (the level is
 *   neither current nor done) can never satisfy the g_level_done test, so the
 *   null is never published.
 *
 * VC6 notes recovered here:
 *  - The leading by-value WinRect's four constants land in edx/eax/esi/ecx;
 *    the eax->ecx->edx->esi ring says the source assigns top, bottom, left,
 *    right (mapscreen2.c's PrintScreenMode8 lever, transferred verbatim).
 *  - The three-colour choice is THREE separate calls, not one call with a
 *    colour variable: the 0x323232 arm carries its own copy of the by-value
 *    rect block (built through edx) while the 0 and 0xa0a0a0 arms share one
 *    (built through ecx).  A single call with a jump-threaded constant would
 *    give one shared argument block; two blocks means two textual calls whose
 *    register choices differed and so could not cross-jump.
 */
#include "legoland.h"

/* ---- types (shared with screens3.c / mapscreen2.c; the offsets are the
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
    char         (*input)(struct Icon*, int, int, int); /* +0x2c */
    void*          widget;    /* +0x30 */
    unsigned int   flags;     /* +0x34 */
    char*          help;      /* +0x38 */
    int            help_id;   /* +0x3c */
} Icon;

typedef char (*IconInputFn)(Icon*, int, int, int);

/* Win32 RECT, passed BY VALUE to the text printers (text.c's WinRect). */
typedef struct WinRect {
    long left;                /* +0x00 */
    long top;                 /* +0x04 */
    long right;               /* +0x08 */
    long bottom;              /* +0x0c */
} WinRect;

/* The 5th PrintSprite argument (money.c's BlitCtx); 0x004bdd00 is the mouse
 * hit record seen through the same shape ({2, icon, 0} = "icon hit"). */
typedef struct BlitCtx {
    int kind;                            /* +0x00 */
    struct { Icon* p; int n; } owner;    /* +0x04, +0x08 */
} BlitCtx;

/* The global game record @ 0x004bcbf4 (legoland.h's g_map): the progress
 * screens only want the current level number at +0x28. */
typedef struct LevelMap {
    char pad[0x28];
    int  level;      /* +0x28 */
} LevelMap;

/* The progress screen's level-marker table (screens3.c): 0x1c-byte records,
 * g_low_markers @0x004beca0 being the five tutorial levels. */
typedef struct LevelMarker {
    const char* lit_name;   /* +0x00 */
    const char* dim_name;   /* +0x04 */
    int         str_id;     /* +0x08 */
    int         x;          /* +0x0c */
    int         y;          /* +0x10 */
    Sprite*     lit;        /* +0x14 */
    Sprite*     dim;        /* +0x18 */
} LevelMarker;              /* 0x1c */

/* ---- globals ------------------------------------------------------------ */
extern Sprite*     g_backdrop;            /* 0x00810148 full-screen background */
extern IconInputFn g_icon_handler1;       /* 0x006687bc */
extern IconInputFn g_icon_handler2;       /* 0x006687c0 */

extern LevelMap*   g_level_map;           /* 0x004bcbf4 */
extern LevelMarker g_low_markers[5];      /* 0x004beca0  levels 1..5 */
extern Icon*       g_low_icons[5];        /* 0x007cb380 */
extern unsigned char g_level_done[15];    /* 0x0080ffd4  CurProfile+0x34 */
extern int         g_progress_is_low;     /* 0x00798664  1 = the 1..5 screen */
extern Pos         g_gfx_point;           /* 0x00813a44  the mouse point */
extern BlitCtx     g_hit_info;            /* 0x004bdd00 */

/* ---- string constants --------------------------------------------------- */
extern const char g_lls_advert_screen[];    /* 0x004bf5f4 "AdvertScreen.lls" */
extern const char g_lls_goback_on_advert[]; /* 0x004bf5dc "GoBack_On_Advert.lls" */
extern const char g_lls_california[];       /* 0x004bf5cc "California.lls" */
extern const char g_lls_windsor[];          /* 0x004bf5c0 "Windsor.lls" */
extern const char g_lls_billund[];          /* 0x004bf5b4 "billund.lls" */

/* ---- callees ------------------------------------------------------------ */
extern Sprite* LoadSprite(const char* name, int mode);                 /* 0x00497ab0 */
extern Icon*   LoadSpriteIcon(const char* name, int mode, int x, int y, int group); /* 0x0046d7b0 */
extern char*   GetString(int id);                                      /* 0x00498f50 */
extern void    NewPrintCent(const char* text, int font, WinRect rc, char white); /* 0x00491d60 */
/* 0x00454d80 (text.c's unmatched neighbour): single-line DrawText in a
 * caller-supplied box passed BY VALUE, in an explicit colour. */
extern void    NewPrintColoured(const char* text, int font, WinRect rc, unsigned long colour); /* 0x00454d80 */

/* Advert-screen icon handlers (0x00490050..0x00490110, this screen's own). */
extern char AdvertGoBackInput(Icon*, int, int, int);      /* 0x00490050 */
extern char AdvertBillundInput(Icon*, int, int, int);     /* 0x00490090 */
extern char AdvertWindsorInput(Icon*, int, int, int);     /* 0x004900d0 */
extern char AdvertCaliforniaInput(Icon*, int, int, int);  /* 0x00490110 */

/* =========================================================================
 *  Advert screen (front-end screen 9)
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00490150
void InitScreen9(void)
{
    Icon* p;

    g_backdrop = LoadSprite(g_lls_advert_screen, 0);
    p = LoadSpriteIcon(g_lls_goback_on_advert, 4, 0x13, 0x14b, 7);
    p->help_id = 0x26;
    p->help = GetString(0x26);
    p->flags |= 0x6002;
    p->input = AdvertGoBackInput;
    g_icon_handler2 = AdvertGoBackInput;
    g_icon_handler1 = 0;
    p = LoadSpriteIcon(g_lls_california, 4, 0x1c, 0x24, 7);
    p->help_id = 0x2bc;
    p->help = GetString(0x2bc);
    p->flags |= 0x6002;
    p->input = AdvertCaliforniaInput;
    p = LoadSpriteIcon(g_lls_windsor, 4, 0xea, 0x24, 7);
    p->help_id = 0x2bd;
    p->help = GetString(0x2bd);
    p->flags |= 0x6002;
    p->input = AdvertWindsorInput;
    p = LoadSpriteIcon(g_lls_billund, 4, 0x1b8, 0x24, 7);
    p->help_id = 0x2be;
    p->help = GetString(0x2be);
    p->flags |= 0x6002;
    p->input = AdvertBillundInput;
}

/* =========================================================================
 *  Tutorial progress screen (front-end screen 6, levels 1..5) — per-frame text
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0048c100
void PrintScreenMode6(void)
{
    WinRect rc;
    char*   s;
    int     i;

    if (g_progress_is_low) {
        rc.top = 0x45;
        rc.bottom = 0x6c;
        rc.left = 0xa;
        rc.right = 0x1d6;
        NewPrintCent(GetString(0x28a), 3, rc, 0);
        for (i = 0; i < 5; i++) {
            s = GetString(g_low_markers[i].str_id);
            rc.left = g_low_markers[i].x + 0x32;
            rc.top = g_low_markers[i].y + 6;
            rc.right = rc.left + 0x190;
            rc.bottom = rc.top + 0x16;
            if (i == g_level_map->level - 1)
                NewPrintColoured(s, 2, rc, 0);
            else if (g_level_done[i] == 1)
                NewPrintColoured(s, 2, rc, 0x323232);
            else
                NewPrintColoured(s, 2, rc, 0xa0a0a0);
            if (g_level_done[i] == 1
                && g_gfx_point.x >= rc.left && g_gfx_point.x < rc.right
                && g_gfx_point.y >= rc.top && g_gfx_point.y < rc.bottom) {
                g_hit_info.kind = 2;
                g_hit_info.owner.p = g_low_icons[i];
            }
        }
    }
}
