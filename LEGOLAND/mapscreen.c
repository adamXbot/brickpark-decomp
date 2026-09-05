/* LEGOLAND — the overview map screen and the front-end screen lifecycle.
 *
 * The map screen is a 640x340 function-based sprite (RenderFullMap draws the
 * whole park into it) blitted at the map-view origin, with a "spanner" marker
 * sprite stamped on every non-zero entry of a 32x32 marker grid and the
 * current scroll viewport outlined on top.  The screen-to-map conversion
 * used by the mouse helpers is
 *
 *     map_x = (px - view.x)              * g_ms_scale_x / view.w + g_ms_x0
 *     map_y = (py - g_ms_y_off - view.y + 1) * g_ms_scale_y * 2 / view.w + g_ms_y0
 *
 * where the view rectangle lives at 0x008139c0 {h, w, x, y} (RenderFullMap
 * sets it to {340, 640, 0, 32}) and the five scale/offset globals at
 * 0x00667c00.. are recomputed by RenderFullMap from the map size.
 *
 * InitScreens / RenderFrontEndScreen are the front-end screen switch: the
 * current screen number (0x0080ff84) selects an Init* routine, and every
 * frame the backdrop sprite, the icon set and the per-screen detail printer
 * are drawn inside the PushRenderingStatus / RenderingComplete bracket
 * (render2.c's RenderScreen is the in-game equivalent). */
#include "legoland.h"

/* ---- types -------------------------------------------------------------- */

/* A loaded sprite (sprite2.c's SpriteRec); only the source origin at
 * +0x18/+0x1a is touched here. */
typedef struct SpriteRec {
    char  pad[0x18];        /* +0x00 */
    short src_x;            /* +0x18 */
    short src_y;            /* +0x1a */
} SpriteRec;

typedef struct BlitCtx BlitCtx;

/* The map-screen view rectangle @ 0x008139c0 — note the field order. */
typedef struct MapView {
    int h;                  /* +0x00  0x008139c0 */
    int w;                  /* +0x04  0x008139c4 */
    int x;                  /* +0x08  0x008139c8 */
    int y;                  /* +0x0c  0x008139cc */
} MapView;

/* The global game record seen through the fields the scroll clamp needs:
 * the view/window extent in tiles at +0x10/+0x12 (scrolltick.c's ScrollMap). */
typedef struct ScrollMap {
    char           pad0[0x10];
    unsigned short view_w;  /* +0x10 */
    unsigned short view_h;  /* +0x12 */
} ScrollMap;

typedef void (*IconHandler)(void);

/* ---- globals ------------------------------------------------------------ */

extern int        g_scroll_x;          /* 0x00667cb4  8.8 fixed point */
extern int        g_scroll_y;          /* 0x00667cb8 */
extern int        g_ms_x0;             /* 0x00667c00  map x of the view's left edge */
extern int        g_ms_y0;             /* 0x00667c04  map y of the view's top edge */
extern IconHandler g_ms_saved_handler1;/* 0x00667c10  saved 0x006687bc */
extern int        g_ms_scale_y;        /* 0x00667c18  map rows per view height (x2) */
extern int        g_ms_scale_x;        /* 0x00667c1c  map columns per view width */
extern int        g_ms_y_off;          /* 0x00667c20  extra y offset (title strip) */
extern IconHandler g_ms_saved_handler2;/* 0x00667c28  saved 0x006687c0 */
extern SpriteRec* g_ms_sprite;         /* 0x00667c2c  the 640x340 map sprite */
extern int        g_ms_ready;          /* 0x00667c30  non-zero once the map is built */
extern SpriteRec* g_ms_marker;         /* 0x00667c34  mapSpanner.lls */
extern int        g_ms_options_dirty;  /* 0x00667c78  set when entering the option screens */

extern IconHandler g_icon_handler1;    /* 0x006687bc */
extern IconHandler g_icon_handler2;    /* 0x006687c0 */
extern int        g_focussed_icon;     /* 0x006687d0  icon under the mouse */
extern int        g_6687b0;            /* 0x006687b0 */

extern int        g_icons2_mode;       /* 0x00668e38  non-zero: alternate icon set */
extern int        g_screen_popup;      /* 0x0080ff80  pending front-end pop-up */
extern int        g_cur_screen;        /* 0x0080ff84  current front-end screen */
extern int        g_screen_mode;       /* 0x0080ff88  sub-mode of the current screen */

extern SpriteRec* g_backdrop;          /* 0x00810148  full-screen background */
extern SpriteRec* g_title_sprite;      /* 0x007986b8 */
extern int        g_title_7986d8;      /* 0x007986d8 */
extern int        g_title_7986dc;      /* 0x007986dc */
extern int        g_title_7986e0;      /* 0x007986e0 */
extern int        g_saved_game_icon;   /* 0x007cb360 */

extern SpriteRec* g_save_panel;        /* 0x00798704 */
extern SpriteRec* g_savebk[8];         /* 0x00798708  (panelui.c's g_savebk) */
extern SpriteRec* g_save_extra[3];     /* 0x00798728 */
extern SpriteRec* g_save_79868c;       /* 0x0079868c */

extern unsigned char g_ms_flags;       /* 0x008119a4  bit 4: draw the marker grid */
extern Pos        g_ms_marks[32][32];  /* 0x008119c0  marker positions (31x31 used) */
extern MapView    g_ms_view;           /* 0x008139c0 */
extern Pos        g_gfx_point;         /* 0x00813a44  mouse point */
extern ScrollMap* g_scroll_map;        /* 0x004bcbf4 — the same object as g_map */

extern const char g_map_spanner_lls[]; /* 0x004b90b4 "mapSpanner.lls" */

/* ---- externals ---------------------------------------------------------- */

extern SpriteRec* LoadSprite(const char* name, int kind);          /* 0x00497ab0 */
extern int        KillSprite(SpriteRec* s);                        /* 0x00497bd0 */
extern SpriteRec* CreateFunctionBasedSprite(void (*fn)(void), short w, short h);
                                                                   /* 0x004976c0 */
extern void       RenderFullMap(void);                             /* 0x004567a0 */
extern void       MapScreenIconHandler(void);                      /* 0x00475080 */
extern int        PrintSprite(SpriteRec* s, int x, int y, int mode, BlitCtx* ctx);
                                                                   /* 0x004853a0 */
extern int        GetNearestColour(int r, int g, int b);           /* 0x0044e6c0 */
extern void       RenderThickBox(int x, int y, int w, int h, int t, int colour);
                                                                   /* 0x00489390 */
extern void       RenderBox(int x, int y, int w, int h, int colour); /* 0x00489410 */
extern void       PointToIsoPlane(Pos* in, Pos* out);              /* 0x0045bcd0 */
extern void       ClampScrollToMap(int vw, int vh, int pad_x, int pad_y); /* 0x00461290 */

extern void       ResetHitInfo(void);                              /* 0x00485ef0 */
extern void       PushRenderingStatusAndLockVideoSurface(void);    /* 0x00463fc0 */
extern void       PopRenderingStatus(void);                        /* 0x004641f0 */
extern int        RenderingComplete(void);                         /* 0x00466500 */
extern void       RenderIcons(void);                               /* 0x0046eee0 */
extern void       RenderIcons2(int a, int b, int c);               /* 0x0046f010 */
extern void       ProcessFrontEndHelp(void);                       /* 0x0046d080 */
extern void       UpdateFocussedIconPtr(void);                     /* 0x004700a0 */
extern void       CheckFocussedIcon(void);                         /* 0x0046f4c0 */
extern int        SetPointer(int idx);                             /* 0x00463850 */
extern void       PrintProfileDetails(void);                       /* 0x0048cf10 */
extern void       PrintSavedGameDetails(void);                     /* 0x0048dd00 */
extern void       PrintExitCheckBox(void);                         /* 0x0048f2d0 */
extern void       PrintScreenMode7(void);                          /* 0x004910f0 */
extern void       PrintScreenMode8(void);                          /* 0x00490410 */
extern void       PrintScreenMode6(void);                          /* 0x0048c100 */
extern void       ProcessScreenPopup(void);                        /* 0x0048fc00 */

extern void       ResetFrontEnd(void);                             /* 0x00498920 */
extern void       KillCurrentScreen(void);                         /* 0x004585c0 */
extern void       InitListProfiles(void);                          /* 0x0048c260 */
extern void       DeletePlayableSamples(int all);                  /* 0x00492b90 */
extern void       InitTitleScreen(void);                           /* 0x0048fc40 */
extern void       InitOptionSamples(void);                         /* 0x00492830 */
extern void       InitOptionScreen(void);                          /* 0x0048eb60 */
extern void       InitSavedGameScreen(void);                       /* 0x0048d4b0 */
extern void       InitFreePlayScreen(void);                        /* 0x0048a8a0 */
extern void       InitProgressScreen(void);                        /* 0x0048b7e0 */
extern void       InitScreen7(void);                               /* 0x00490c70 */
extern void       InitScreen8(void);                               /* 0x00490350 */
extern void       InitScreen9(void);                               /* 0x00490150 */

void RenderFrontEndScreen(int screen);

/* -------------------------------------------------------------------------
 * 0x004562c0 / 0x004562e0 -- stash / restore the two icon handler pointers
 * the map screen overrides with its own handler.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004562c0
void SaveIconHandlers(void)
{
    g_ms_saved_handler1 = g_icon_handler1;
    g_ms_saved_handler2 = g_icon_handler2;
}

// FUNCTION: LEGOLAND 0x004562e0
void RestoreIconHandlers(void)
{
    g_icon_handler1 = g_ms_saved_handler1;
    g_icon_handler2 = g_ms_saved_handler2;
}

/* -------------------------------------------------------------------------
 * 0x00456300 -- build the map sprite (once) and take over the icon handlers.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00456300
void InitMapScreen(void)
{
    if (g_ms_ready == 0) {
        g_ms_sprite = CreateFunctionBasedSprite(RenderFullMap, 640, 340);
        if (g_ms_sprite) {
            g_ms_marker = LoadSprite(g_map_spanner_lls, 0);
            g_ms_sprite->src_x = 0;
            g_ms_sprite->src_y = 0;
        }
        SaveIconHandlers();
        g_icon_handler2 = MapScreenIconHandler;
        g_icon_handler1 = MapScreenIconHandler;
    }
}

/* -------------------------------------------------------------------------
 * 0x00456370 -- free the map sprite and (only then) the marker sprite.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00456370
void KillMapScreen(void)
{
    if (g_ms_sprite) {
        KillSprite(g_ms_sprite);
        g_ms_sprite = 0;
        if (g_ms_marker) {
            KillSprite(g_ms_marker);
            g_ms_marker = 0;
        }
    }
    g_ms_ready = 0;
}

/* -------------------------------------------------------------------------
 * 0x004563b0 -- outline the current scroll viewport on the map screen: a
 * 2-pixel white box whose position is the scroll origin mapped into view
 * pixels and whose size is the in-game window mapped the same way.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004563b0
void DrawMapViewportRect(void)
{
    int x, y, w, h;

    x = ((g_scroll_x >> 8) - g_ms_x0) * g_ms_view.w / g_ms_scale_x;
    y = ((g_scroll_y >> 8) - g_ms_y0) * g_ms_view.w / g_ms_scale_y / 2 + g_ms_y_off;
    w = g_ms_view.w * g_ms_view.w / g_ms_scale_x;
    h = g_ms_view.h * g_ms_view.w / g_ms_scale_y / 2;
    RenderThickBox(g_ms_view.x + x, g_ms_view.y + y, w, h, 2,
                   GetNearestColour(255, 255, 255));
}

/* -------------------------------------------------------------------------
 * 0x00456460 -- while the mouse is over the map, draw the green box that
 * shows where the in-game window would land if the map were clicked there.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00456460
void RenderMouseBounds(void)
{
    Pos pt;
    Pos iso;
    int x, y, w, h;

    pt.x = (g_gfx_point.x - g_ms_view.x) * g_ms_scale_x / g_ms_view.w + g_ms_x0;
    pt.y = (g_gfx_point.y - g_ms_view.y - g_ms_y_off + 1) * g_ms_scale_y * 2 / g_ms_view.w + g_ms_y0;
    PointToIsoPlane(&pt, &iso);
    if (iso.x >= 0 && iso.y >= 0 && iso.x < g_ms_view.w && iso.y < g_ms_view.h) {
        x = g_gfx_point.x - (g_ms_view.w / 2) * g_ms_view.w / g_ms_scale_x - g_ms_view.x;
        y = g_gfx_point.y - (g_ms_view.h / 2) * g_ms_view.w / g_ms_scale_y / 2 - g_ms_view.y;
        w = g_ms_view.w * g_ms_view.w / g_ms_scale_x;
        h = g_ms_view.h * g_ms_view.w / (g_ms_scale_y * 2);
        RenderBox(g_ms_view.x + x, g_ms_view.y + y, w, h, GetNearestColour(0, 255, 0));
    }
}

/* -------------------------------------------------------------------------
 * 0x004565b0 -- a click on the map screen: if the mouse is over the map,
 * centre the in-game scroll origin on the clicked map point `p` (8.8 fixed)
 * and clamp it to the map.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004565b0
void MapScreenSetScrollPos(Pos* p)
{
    Pos pt;
    Pos iso;

    pt.x = (g_gfx_point.x - g_ms_view.x) * g_ms_scale_x / g_ms_view.w + g_ms_x0;
    pt.y = (g_gfx_point.y - g_ms_view.y - g_ms_y_off + 1) * g_ms_scale_y * 2 / g_ms_view.w + g_ms_y0;
    PointToIsoPlane(&pt, &iso);
    if (iso.x >= 0 && iso.y >= 0 && iso.x < g_ms_view.w && iso.y < g_ms_view.h) {
        g_scroll_x = ((p->x - g_ms_view.x + 1) * g_ms_scale_x / g_ms_view.w
                      - g_ms_view.w / 2 + g_ms_x0) << 8;
        g_scroll_y = ((p->y - g_ms_view.y - g_ms_y_off + 1) * g_ms_scale_y * 2 / g_ms_view.w
                      - g_ms_view.h / 2 + g_ms_y0) << 8;
        ClampScrollToMap(g_scroll_map->view_w << 8, g_scroll_map->view_h << 8, 0, 0);
    }
}

/* -------------------------------------------------------------------------
 * 0x004566f0 -- draw the map screen: the map sprite, the marker grid (when
 * enabled by g_ms_flags bit 4) and the viewport outline.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004566f0
void DrawMapScreen(void)
{
    int row, col;
    Pos* p;

    PrintSprite(g_ms_sprite, g_ms_view.x, g_ms_view.y, 0, 0);
    if (g_ms_flags & 0x10) {
        for (row = 0; row < 31; row++) {
            for (col = 0; col < 31; col++) {
                p = &g_ms_marks[row][col];
                if (p->x || p->y)
                    PrintSprite(g_ms_marker, p->x, p->y, 0, 0);
            }
        }
    }
    DrawMapViewportRect();
}

/* -------------------------------------------------------------------------
 * 0x0048faa0 -- title screen teardown: the backdrop, the title sprite, and
 * the four title-screen state words.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0048faa0
void KillTitleScreenSprites(void)
{
    if (g_backdrop) {
        KillSprite(g_backdrop);
        g_backdrop = 0;
    }
    if (g_title_sprite) {
        KillSprite(g_title_sprite);
        g_title_sprite = 0;
    }
    g_title_7986d8 = 0;
    g_title_7986dc = 0;
    g_saved_game_icon = 0;
    g_title_7986e0 = 0;
}

/* -------------------------------------------------------------------------
 * 0x0048dbc0 -- saved-game screen teardown: the panel, the eleven backdrop
 * slots and the selection sprite, each freed and cleared if loaded.
 * ------------------------------------------------------------------------- */

#define KILL_IF(p) if (p) { KillSprite(p); (p) = 0; }

// FUNCTION: LEGOLAND 0x0048dbc0
void KillSaveScreenSprites(void)
{
    KILL_IF(g_save_panel);
    KILL_IF(g_savebk[0]);
    KILL_IF(g_savebk[1]);
    KILL_IF(g_savebk[2]);
    KILL_IF(g_savebk[3]);
    KILL_IF(g_savebk[4]);
    KILL_IF(g_savebk[5]);
    KILL_IF(g_savebk[6]);
    KILL_IF(g_savebk[7]);
    KILL_IF(g_save_extra[0]);
    KILL_IF(g_save_extra[1]);
    KILL_IF(g_save_extra[2]);
    KILL_IF(g_save_79868c);
}

/* -------------------------------------------------------------------------
 * 0x00458640 -- switch to front-end screen `screen` (a char, passed as an
 * int), running its Init* routine if it is not already current, then render
 * it if the backdrop exists.  Screen 2 has no initialiser.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00458640
void InitScreens(int screen)
{
    char s = (char)screen;

    if (g_cur_screen != s) {
        ResetFrontEnd();
        g_6687b0 = 4;
        KillCurrentScreen();
        g_screen_popup = 0;
        switch (s) {
        case 0:
            InitListProfiles();
            break;
        case 1:
            DeletePlayableSamples(0);
            InitTitleScreen();
            break;
        case 5:
            g_ms_options_dirty = 1;
            InitOptionSamples();
            InitOptionScreen();
            break;
        case 4:
            InitSavedGameScreen();
            break;
        case 3:
            InitFreePlayScreen();
            break;
        case 6:
            DeletePlayableSamples(0);
            InitProgressScreen();
            break;
        case 7:
            g_ms_options_dirty = 1;
            InitOptionSamples();
            InitScreen7();
            break;
        case 8:
            InitScreen8();
            break;
        case 9:
            InitScreen9();
            break;
        }
        g_cur_screen = s;
    }
    if (g_backdrop)
        RenderFrontEndScreen(screen);
}

/* -------------------------------------------------------------------------
 * 0x00458740 -- draw one front-end frame: backdrop, icon set (the alternate
 * set adds a second group in mode 6), the per-mode detail printer, help,
 * the pointer, and the frame end.
 * ------------------------------------------------------------------------- */

/* Exact: 55 instructions / 192 bytes, zero mismatches over audit.py's
 * true_extent of the original.  Held as WIP for tooling only: the body ends
 * in a tail `jmp RenderingComplete` and /Gy places the 9-entry switch jump
 * table right after it in the COMDAT; match.py's relocation sentinel hides
 * the zero rel32, so neither match.py nor audit.py's end_of_body can bound
 * it and both count the table as seven junk instructions. */
// FUNCTION: LEGOLAND 0x00458740
void RenderFrontEndScreen(int screen)
{
    ResetHitInfo();
    PushRenderingStatusAndLockVideoSurface();
    PrintSprite(g_backdrop, 0, 0, 0, 0);
    if (g_icons2_mode) {
        RenderIcons2(7, 14, 0);
        if (g_screen_mode == 6)
            RenderIcons2(28, 35, 0);
    } else {
        RenderIcons();
    }
    switch (g_screen_mode) {
    case 0:
        PrintProfileDetails();
        break;
    case 4:
        PrintSavedGameDetails();
        break;
    case 1:
    case 5:
        PrintExitCheckBox();
        break;
    case 7:
        PrintScreenMode7();
        break;
    case 8:
        PrintScreenMode8();
        break;
    case 6:
        PrintScreenMode6();
        break;
    }
    ProcessScreenPopup();
    ProcessFrontEndHelp();
    UpdateFocussedIconPtr();
    PopRenderingStatus();
    if (g_focussed_icon)
        SetPointer(6);
    CheckFocussedIcon();
    RenderingComplete();
}
