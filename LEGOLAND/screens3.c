/* LEGOLAND -- front-end screen internals: the icon input callbacks, per-icon
 * redraw hooks and small state helpers behind the in-game interface bar, the
 * progress screen, the saved-game / profile screens and the option screen.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field offsets, callee argument counts and global addresses are load-bearing;
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere; the Icon record is the same 0x40-byte one iconui.c / screens2.c /
 * bigscreens.c use).
 *
 * These are the bodies bigscreens.c / screens2.c / profiles.c named but did
 * not reproduce. What the screens are and where their state lives is
 * documented in those two files' headers; this file fills in the behaviour.
 *
 * ------------------------------------------------------------------------
 *  THE ICON INPUT CONTRACT
 * ------------------------------------------------------------------------
 * The icon's input hook at +0x2c takes FOUR arguments, not two:
 * `char (*)(Icon*, int buttons, int, int)`. Most handlers only ever look at
 * the first two, which is why two-argument reconstructions of them match --
 * but ProgressLevelInput (0x0048bb60), ProgressGoBackInput (0x0048c020),
 * SaveSlotInput (0x0048e4a0) and SaveScreenAcceptInput (0x0048da50) FORWARD
 * all four to another handler, and those only match with the full signature.
 *
 * `buttons` is a mask: bit 0 (0x01) is "pressed this frame", bit 1 (0x02) is
 * "clicked", bit 2 (0x04) is "still held". Almost every handler looks only at
 * bit 1 and returns 1 unconditionally -- the answer is "the icon consumed the
 * event" and hardly any of them decline -- so they compile to one shared
 * `mov al,1 / ret` tail with the work hanging off an `if (buttons & 2)` guard.
 * The exceptions are worth knowing: TitleFreeInput (0x0048ff20) and the three
 * volume handlers answer 2 when they acted, and the four forwarding handlers
 * answer whatever their callee said.
 *
 * ------------------------------------------------------------------------
 *  THE IN-GAME INTERFACE BAR  (icon group set up by iconui.c / fpui.c)
 * ------------------------------------------------------------------------
 * Every bar button first checks `g_game_mode != 1` -- mode 1 is the full-screen
 * map view, in which the bar is inert. They then play the UI click sample and
 * set the edit mode:
 *
 *   g_edit_changed (EditMode @ 0x8119b0):  0 = none, 2 = eraser
 *   g_ui_flags     (GamePad  @ 0x813a40):  bit 0x0400 = query cursor armed,
 *                                          bit 0x1000 = path-laying armed
 *
 *   Brief   (0x474f40) -> pop the mission-brief panel (0x4911c0) over the two
 *                         0x80-byte script text buffers 0x66861c / 0x66869c.
 *   ScriptEnd (0x474fa0) -> 0x46b700, end the running script.
 *   Path    (0x474fc0) -> SetEditObject(icon->data): the path object.
 *   Query   (0x475000) -> clears 0x1000 (drops path mode), EditMode 0.
 *   Eraser  (0x475040) -> clears 0x0400 (drops query mode), EditMode 2.
 *   Map     (0x475080) -> toggles the full-screen map: entering it stashes the
 *                         old g_game_mode in 0x667c60 and sets mode 1; leaving
 *                         restores it and calls 0x4562e0 (the icon-handler
 *                         restore in mapscreen.c).
 *   Options (0x475120) -> only when nothing is being dragged (g_drag_lock 0):
 *                         switch to the front end (mode 2, screen -1/5) with
 *                         the alternate icon set (g_icons2_mode 1).
 *
 * ------------------------------------------------------------------------
 *  THE FOUR THEME BUTTONS  (legoland / castle / western / adventurers)
 * ------------------------------------------------------------------------
 * The side panel shows one object menu at a time. `g_menu_index` (0x4baff8)
 * is the open menu, 5 = none, and `g_menus` (0x4bafa8, 20-byte records) is the
 * menu table. Theme n owns menu n: legoland 0, western 1, castle 2,
 * adventurers 3. Each theme button:
 *   * clicking the OPEN theme closes the panel: 0x474750 (drop the object
 *     icons), set that theme's "closed" flag (0x4bb094 + 4*theme), menu index
 *     5, and tell the panel to rebuild off-screen (g_panel_state.f00 = 1,
 *     .f04 = 1).
 *   * clicking another theme opens it: menu index = theme, object-list mode 0,
 *     TestMenu(&g_menus[theme]); on success light the button (SetIconSprite to
 *     the theme's ON sprite), remember it in g_active_theme_icon (0x668eb0) and
 *     clear that theme's closed flag. On failure the previous menu index and
 *     object-list mode are put back and TestMenu re-runs on the old menu.
 * 0x474590 resets all four closed flags to 1 and forgets the lit icon;
 * 0x474990 shows/hides the four theme icons from the four ints at 0x668e20,
 * 0x476180 from the four bytes at 0x80ffd0 (the current profile's per-theme
 * unlock flags, CurProfile+0x30). Icon flag 0x400 = hidden.
 *
 * NOTE the two index orders do not agree: the MENU index is legoland 0,
 * western 1, castle 2, adventurers 3, but the four "menu closed" flags at
 * 0x004bb094 run legoland, CASTLE, WESTERN, adventurers. Both are reproduced.
 *
 * ------------------------------------------------------------------------
 *  THE PROGRESS SCREENS
 * ------------------------------------------------------------------------
 * There are TWO, sharing the same button and marker code:
 *   * levels 6..15 -- the world map, built by bigscreens.c's
 *     InitProgressScreen from g_level_markers (0x004beb80, ten 0x1c-byte
 *     records: San Francisco, Belgium, Washington, New York, India, Holland,
 *     London, Italy, Sydney, Egypt, each with a Pro_<place>_Lit/_Unlit pair).
 *   * levels 1..5 -- the TUTORIAL report (backdrop TutorialBK.lls), built by
 *     InitTutorialScreen (0x0048bde0, which bigscreens.c calls
 *     SkipProgressScreen: it does not skip anything). Its five markers live in
 *     g_low_markers (0x004beca0) and are Appraisal_Yes / Appraisal_No ticks in
 *     a column at x=30, y=130..226, help strings 0x14..0x18.
 * The marker record is {lit_name, dim_name, str_id, x, y, lit, dim} of 0x1c
 * bytes. bigscreens.c frames the first table eight bytes in, at 0x004beb88 --
 * both framings agree on str_id/x/y/lit/dim, which is all it uses.
 *
 * State: g_progress_resume (0x00798660) survives a round trip so the screen is
 * not rebuilt; g_progress_is_low (0x00798664) says which of the two screens is
 * up and picks the Accept handler and the sprite table to release;
 * g_progress_798668 marks "the tutorial was chosen"; g_progress_click_ms /
 * g_progress_click_level (0x0079866c / 0x004bec98) implement the marker's
 * DOUBLE CLICK -- a second click on the same marker within 500 ms is forwarded
 * to the Accept handler, which starts the level.
 *
 * ------------------------------------------------------------------------
 *  VC6 LEVERS THIS FILE COST
 * ------------------------------------------------------------------------
 *  - A leading guard whose exit block must be laid out LAST wants
 *    `if (cond) { ... } else { return 1; } return 1;` -- an early
 *    `if (!cond) return 1;` sinks the callee-saved push correctly but puts the
 *    return block inline right after the guard (MapIconInput).
 *  - A post-guard block ending in ONE `rc = 1; return rc;` is what makes VC6
 *    sink a push past the guards AND duplicate the tail into the arms; it is
 *    the whole difference between 75 and 80 instructions in the four theme
 *    handlers, and the same lever fixes both progress-screen Accepts.
 *  - `for (i = 0; i < n; i++)` over a global array beats a pointer walk: the
 *    hand-scaled `*(T*)((char*)arr + i)` form loses the folded addressing.
 *    A pointer walk plus a separate down-counter is what you write when the
 *    two arrays being walked have DIFFERENT element sizes (0x00476180).
 *  - Zeroing a fixed-size record is `memset(p, 0, 20)` with
 *    `#pragma intrinsic(memset)`; five explicit field stores give a different
 *    register assignment (ClearMapCells).
 *  - A `short` local (not an int) is what produces `mov cx,[..] / movsx ecx,cx`
 *    (SetWaitSpriteRect); an INT local holding a u16 field is what produces
 *    `xor ecx,ecx / mov cx,[..]` and turns a switch's sub/je chain into a cmp
 *    chain (VolMarkerInput vs VolUpInput -- same construct, different width).
 *  - AVIStreamGetFrame is `__stdcall`: the missing `add esp,8` after its call
 *    was the single diverging instruction in RenderAdvisorIcon.
 *  - Reading a field into a LOCAL rather than re-spelling `p->u18.alt` is what
 *    puts it in edi and moves the push into the entry prologue
 *    (RenderDarkTitleIcon).
 */
#include "legoland.h"

void* memset(void*, int, unsigned int);
char* strcpy(char*, const char*);
unsigned int strlen(const char*);
#pragma intrinsic(memset, strcpy, strlen)

/* ---- the icon record (iconui.c) ---------------------------------------- */
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
    union {
        void*         owner;  /* +0x18  profile list icons: the Profile record */
        Sprite*       alt;    /* +0x18  brief icon: the un-focussed sprite */
        unsigned char level;  /* +0x18  progress screen: level number */
        unsigned short control; /* +0x18  option screen: control number 1..9 */
    } u18;
    unsigned char  slot;      /* +0x1c  profile / save slot number */
    char           pad1d[3];  /* +0x1d */
    unsigned char  flags20;   /* +0x20  bit 0: slot icon, 1/2: save type */
    char           pad21[0x28 - 0x21];
    int          (*render)(struct Icon*);          /* +0x28 */
    char         (*input)(struct Icon*, int, int, int); /* +0x2c */
    void*          widget;    /* +0x30 */
    unsigned int   flags;     /* +0x34  0x400 = hidden */
    char*          help;      /* +0x38 */
    int            help_id;   /* +0x3c */
} Icon;

/* The 5th PrintSprite argument (money.c's BlitCtx): kind plus an 8-byte
 * payload cleared as one block. */
typedef char (*IconInputFn)(Icon*, int, int, int);

typedef struct BlitCtx {
    int kind;                            /* +0x00 */
    struct { Icon* p; int n; } owner;    /* +0x04, +0x08 */
} BlitCtx;

/* The menu table @ 0x004bafa8 (panelui.c): 20-byte records indexed by
 * g_menu_index; 5 means "no menu open". The layout is opaque here. */
typedef struct Menu {
    int f[5];
} Menu;

/* The side-panel scroll state @ 0x007fdd80 (panelui.c's PanelState). */
typedef struct PanelState {
    char f00;    /* +0x00 */
    char pad[3];
    int  f04;    /* +0x04  1 = rebuild off-panel */
} PanelState;

/* ---- globals ------------------------------------------------------------ */
extern int   g_game_mode;                 /* 0x008119b4  1 = full-screen map */
extern int   g_game_mode_saved;           /* 0x00667c60  mode stashed by the map button */
extern int   g_edit_changed;              /* 0x008119b0  EditMode */
extern unsigned int g_ui_flags;           /* 0x00813a40  GamePad */
extern int   g_8119bc;                    /* 0x008119bc */
extern int   g_80ff70;                    /* 0x0080ff70 */
extern int   g_6687b0;                    /* 0x006687b0 */
extern int   g_drag_lock;                 /* 0x00668954 */
extern int   g_icons2_mode;               /* 0x00668e38 */
extern int   g_cur_screen;                /* 0x0080ff84 */
extern int   g_screen_mode;               /* 0x0080ff88 */
extern int   g_script_running;            /* 0x00832ba4 */
extern char  g_script_text1[];            /* 0x0066861c  0x80 bytes */
extern char  g_script_text2[];            /* 0x0066869c  0x80 bytes */
extern void* g_snd_click;                 /* 0x004b92c0  the UI click sample */
extern void* g_snd_theme;                 /* 0x004b9314  the theme-button sample */

extern Menu  g_menus[];                   /* 0x004bafa8 */
extern int   g_menu_index;                /* 0x004baff8  5 = no menu open */
extern int   g_object_list_mode;          /* 0x00668e34 */
extern Icon* g_active_theme_icon;         /* 0x00668eb0 */
extern int   g_theme_closed[4];           /* 0x004bb094 legoland, castle, western, adventurers */
extern int   g_theme_enabled[4];          /* 0x00668e20 */
extern Icon* g_theme_icon[4];             /* 0x007fdd70 */
extern unsigned char g_profile_themes[4]; /* 0x0080ffd0  CurProfile+0x30 */
extern PanelState g_panel_state;          /* 0x007fdd80 */
extern Sprite* g_theme_legoland_on;       /* 0x007fdcc0 */
extern Sprite* g_theme_western_on;        /* 0x007fdcc4 */
extern Sprite* g_theme_castle_on;         /* 0x007fdcc8 */
extern Sprite* g_theme_adv_on;            /* 0x007fdccc */

extern Icon* g_focussed_icon;             /* 0x006687d0  FocussedIconPtr */

/* Profile / saved-game screen state (screens2.c / bigscreens.c). */
extern int   g_frontend_checkbox_closed;  /* 0x004bef9c */
extern int   g_delete_popup_up;           /* 0x007986e4 */
extern int   g_newprof_popup_up;          /* 0x007986e8 */
extern Icon* g_accept_icon;               /* 0x007986e0 */
extern unsigned char g_profile_slot;      /* 0x0080ffe3  CurProfile+0x43 */

extern void* g_snd_close;                 /* 0x004b929c  the popup-close sample */
extern int   g_exit_popup_up;             /* 0x007cb320 */
extern int   g_save_7cb324;               /* 0x007cb324 */
extern int   g_save_is_load;              /* 0x007cb328  non-zero: loading, not saving */
extern int   g_pending_state;             /* 0x00832ba0 */
extern int   g_newsave_popup_up;          /* 0x00798700 */
extern unsigned char g_save_type;         /* 0x0080ffe5  CurProfile+0x45: 1 normal, 2 free */
extern int   g_movie_7cb2f0;              /* 0x007cb2f0 */
extern int   g_movie_7cb300;              /* 0x007cb300 */
extern int   g_movie_7cb30c;              /* 0x007cb30c */
extern unsigned char g_cur_save_slot;     /* 0x0080ffe4  CurProfile+0x44 */
extern Icon* g_np_icon_ok;                /* 0x007986d8  popup OK icon */
extern Icon* g_np_close_icon;             /* 0x007986dc  popup close icon */
extern Sprite* g_fe_sprite_674;           /* 0x00798674  PU_OKON.lls */
extern Sprite* g_fe_sprite_680;           /* 0x00798680  RegCloseON / PU_ClosePopUpON.lls */

/* The global game record @ 0x004bcbf4 (legoland.h's g_map / the exported
 * `lpConfig`); the progress screen only wants the current level at +0x28. */
typedef struct LevelMap {
    char pad[0x28];
    int  level;      /* +0x28 */
} LevelMap;

/* The progress screen's level-marker table. TWO tables, same 0x1c-byte
 * record: g_level_markers @0x004beb80 is levels 6..15 (ten entries) and
 * g_low_markers @0x004beca0 is levels 1..5 (five). bigscreens.c frames the
 * first one from 0x004beb88, i.e. eight bytes in, because it only ever uses
 * str_id/x/y/lit/dim -- both framings agree on those five fields; the two
 * name pointers are what the offset hides. */
typedef struct LevelMarker {
    const char* lit_name;   /* +0x00 */
    const char* dim_name;   /* +0x04 */
    int         str_id;     /* +0x08  help string */
    int         x;          /* +0x0c */
    int         y;          /* +0x10 */
    Sprite*     lit;        /* +0x14 */
    Sprite*     dim;        /* +0x18 */
} LevelMarker;              /* 0x1c */

extern LevelMap*   g_level_map;           /* 0x004bcbf4 */
extern LevelMarker g_level_markers[10];   /* 0x004beb80  levels 6..15 */
extern LevelMarker g_low_markers[5];      /* 0x004beca0  levels 1..5 (tutorial) */

/* Progress-screen state. */
extern int   g_progress_resume;           /* 0x00798660 */
extern int   g_progress_is_low;           /* 0x00798664  1 = the levels 1..5 screen */
extern int   g_progress_798668;           /* 0x00798668 */
extern unsigned int g_progress_click_ms;  /* 0x0079866c  last marker click, ms */
extern int   g_progress_click_level;      /* 0x004bec98  level of that click */
extern int   g_last_level;                /* 0x007cb394 */
extern Icon* g_low_icons[5];              /* 0x007cb380 */
extern unsigned char g_level_done[15];    /* 0x0080ffd4  CurProfile+0x34 */
extern unsigned char g_have_profile;      /* 0x0080ffd9  CurProfile+0x39 */
extern Sprite* g_backdrop;                /* 0x00810148 */
extern IconInputFn g_icon_handler1;       /* 0x006687bc */
extern IconInputFn g_icon_handler2;       /* 0x006687c0 */

/* Sprite names (.rdata; only the addresses are load-bearing). */
extern const char g_lls_tutorial_bk[];          /* 0x004bef88 "TutorialBK.lls" */
extern const char g_lls_accept_on_report[];     /* 0x004bef70 "Accept_on_Report.lls" */
extern const char g_lls_goback_on_tut[];        /* 0x004bef5c "GoBack_on_Tut.lls" */

extern Icon* g_script_end_icon;           /* 0x00668eb8 */
extern Icon* g_help_text_icon;            /* 0x00668e9c */
extern int   g_832ba8;                    /* 0x00832ba8 */

/* 0x0080ffe4 read wide: the save-slot byte's home is an int field, and the
 * paths that pass it on read the whole dword and mask (bigscreens.c models it
 * as a union for the same reason). */
extern unsigned int  g_cur_save_slot_wide; /* 0x0080ffe4 */
extern int   g_cur_80ffc0;                /* 0x0080ffc0  CurProfile+0x20 */
extern int   g_vol_speech;                /* 0x0080ffc4 */
extern int   g_vol_music;                 /* 0x0080ffc8 */
extern int   g_vol_sfx;                   /* 0x0080ffcc */
extern int   g_temp_7cad80;               /* 0x007cad80  temp profile +0x20 */
extern unsigned char g_temp_save_type;    /* 0x007cad84  temp profile +0x24 */
extern int   g_temp_vol_speech;           /* 0x007cad88 */
extern int   g_temp_vol_music;            /* 0x007cad8c */
extern int   g_temp_vol_sfx;              /* 0x007cad90 */
extern int   g_667c64;                    /* 0x00667c64 */
extern int   g_667c80;                    /* 0x00667c80 */
extern const char g_str_empty_slot[];     /* 0x004befa0 "EMPTY" */

extern unsigned int g_save_time;           /* 0x00669204  last-save stamp */
extern void* g_66b44c;                    /* 0x0066b44c  head of a free list */
extern int   g_wait_active;               /* 0x00668204 */
extern Sprite* g_wait_sprite;             /* 0x00668208 */
extern int   g_wait_rect[4];              /* 0x007fea30  {x, y, x+w, y+h} */
extern const char g_lls_wait[];           /* 0x004b9d30 */

extern Pos   g_gfx_point;                 /* 0x00813a44  the mouse point */
extern unsigned char g_mouse_buttons;     /* 0x00813ac4  bit 2 = held */
extern int   g_6687b4;                    /* 0x006687b4 */
extern Icon* g_vol_marker_speech;         /* 0x00798748 */
extern Icon* g_vol_marker_fx;             /* 0x0079874c */
extern Icon* g_vol_marker_music;          /* 0x00798750 */

extern char  g_temp_name[0x1e];           /* 0x007cad60  temp profile name */
extern unsigned char g_temp_name_len;     /* 0x007cad7e */
extern const char g_fmt_save_default[];   /* 0x004bf2e8 "%s%d" */

/* The advisor video record (0x00665f5c). Only three fields are touched here. */
typedef struct VidAnim {
    int    frames;          /* +0x00 */
    char   pad04[0x10];     /* +0x04 */
    void*  pgf;             /* +0x14  AVI GetFrame object */
    char   pad18[0x0c];     /* +0x18 */
    void (*on_end)(void);   /* +0x24 */
} VidAnim;

/* A Win32 BITMAPINFOHEADER, as AVIStreamGetFrame returns it. */
typedef struct DibHeader {
    int size;      /* +0x00 */
    int width;     /* +0x04 */
    int height;    /* +0x08 */
} DibHeader;

extern VidAnim* g_vidanim;                /* 0x00665f5c  the playing advisor */
extern VidAnim* g_vid_next;               /* 0x00665f60  the one queued next */
extern int      g_vid_frame;              /* 0x00665eec  frame counter */
extern const char* g_dbg_where;           /* 0x00667c40  debug "where am I" */
extern BlitCtx  g_hit_info;               /* 0x004bdd00  {2, icon, 0} = a hit */
extern const char g_dbg_setvidanim[];     /* 0x004b7dc4 "SetVidAnim" */
extern const char g_dbg_getframe[];       /* 0x004b7db4 "AVI GetFrame" */
extern const char g_dbg_blt[];            /* 0x004b7da8 "BltAdvisor" */
extern const char g_dbg_exit[];           /* 0x004b7d98 "Exit Advisor" */

/* ---- callees ------------------------------------------------------------ */
extern void  PushRenderingStatusAndLockVideoSurface(void);    /* 0x00463fc0 */
extern void  PopRenderingStatus(void);                        /* 0x004641f0 */
/* 0x00443dc0 (not exported): starts playing an advisor clip. */
extern void  SetVidAnim(VidAnim* a);
/* 0x0049e418: the AVIStreamGetFrame import thunk -- __stdcall, so it cleans
 * its own two arguments (no `add esp,8` after the call). */
extern DibHeader* __stdcall AVIStreamGetFrame(void* pgf, int frame);
/* 0x004659a0 (not exported): blits one decoded advisor frame. */
extern void  BltAdvisor(DibHeader* dib, int x, int y);

extern int   sprintf(char* buf, const char* fmt, ...);        /* 0x0049e573 */

/* 0x00441e80 / 0x0047d5a0 (matched elsewhere): the sprite's LLS animation
 * record (its current frame is the short at +0) and the frame setter. */
extern short* GetLLSForSprite(Sprite* sprite);
extern void   LLSSetFrame(short* lls, int frame);
extern int    GetBlink(void);                                 /* 0x00499480 */

/* 0x0046f360 (not exported): the icon under `pt`; the second argument is an
 * out-slot the caller does not read. */
extern Icon* GetIconAtPos(Pos* pt, int* out);
/* 0x0048eaf0 / 0x0048eac0 (not exported): volume 0..100 <-> marker x. */
extern int   VolumeToMarkerX(int vol);
extern int   MarkerXToVolume(int x);
/* 0x0046d230 / 0x0046d110 (not exported): the help-bar text (-2 clears it)
 * and the per-frame help update. */
extern void  ShowHelpString(int id);
extern void  UpdateHelpBar(void);
/* 0x0048faf0 / 0x00498b40 / 0x00498cf0 / 0x00492b50 (not exported). */
extern void  RenderScreen(void);
extern void  sub_498b40(void);
extern int   sub_498cf0(void);
extern void  KillPlayableSample(void* s);

extern unsigned int GetGameTimer(void);                       /* 0x00499430 */
extern void  ClearOverlays(void);                             /* 0x00462ce0 */
/* 0x00482a80 (not exported): the rest of the teardown 0x004828f0 tails into. */
extern void  sub_482a80(void);
/* 0x004913f0 (not exported): the new-profile popup's per-frame hook. Its
 * ADDRESS is what NewProfileCloseInput tests (always true). */
extern void  sub_4913f0(void);

/* 0x0048d8f0 (not exported): reads one save slot's header into the temp
 * profile; non-zero when the slot really holds a game. */
extern int   LoadDateIntoTempProfile(int profile_slot, int save_slot);
/* 0x0048dbc0 / 0x0048faa0 (not exported): drop the saved-game screen sprites. */
extern void  KillSaveScreenSprites(void);
/* 0x0048e160 (not exported): frees the saved-game list. */
extern void  DeleteSavedGameList(void);
/* 0x00458b20 (not exported): starts the actual load. */
extern void  sub_458b20(void);
/* 0x004912e0 (not exported): clears the temp profile record. */
extern void  ResetTempProfile(void);
/* 0x0048e280 (not exported): builds the new-saved-game name popup over `p`. */
extern void  InitNewSaveGamePOPUP(Icon* p);
/* 0x0048e3d0 (not exported): seeds the name editor with an existing name. */
extern void  sub_48e3d0(void* name);
extern char  SaveEmptySlotInput(Icon*, int, int, int);        /* 0x0048e4f0 */

extern Icon* FindIcon(int group);                             /* 0x0046d630 */
extern void  DeleteIcon(Icon* p);                             /* 0x0046d4e0 */
extern void  MemFree(void* p);                                /* 0x0049e4d0 */
/* 0x00458be0 / 0x00459820 (not exported): the script stop path. */
extern void  sub_458be0(void);
extern void  sub_459820(int a);
/* 0x0048c720 / 0x0048c860 (not exported): the delete-confirmation popups of
 * the profile list and of the saved-game screen. */
extern void  InitProfileCheckBoxIcons(Icon* p);
extern void  InitSaveDeletePopUp(Icon* p);
/* 0x004907a0 (not exported): loads the level's help text for `key`, non-zero
 * when there is any. 0x00490850 frees the buffer it leaves behind. */
extern int   LoadHelpTextFor(void* key);
extern void  FreeHelpTextBuffer(void);

extern Sprite* LoadSprite(const char* name, int mode);         /* 0x00497ab0 */
extern void  ReferenceSprite(Sprite* s);                      /* 0x00497bb0 */
extern int   KillSprite(Sprite* s);                           /* 0x00497bd0 */
extern Icon* InsertIcon(short x, short y, unsigned short group, Sprite* s); /* 0x0046d6c0 */
extern Icon* LoadSpriteIcon(const char* name, int mode, int x, int y, int group); /* 0x0046d7b0 */
extern char* GetString(int id);                               /* 0x00498f50 */
extern unsigned int GetTicks(void);                           /* 0x00499450 */
extern void  InitGameInterface(int a);                        /* 0x004749d0 */
extern int   RenderFlashingSpriteIcon(Icon*);                 /* 0x0046e8a0 */
/* 0x00466360 / 0x004663c0 (not exported): the render-view entry the park is
 * brought up with (profiles.c declares the same two). */
extern void  sub_466360(int a, int b);
extern void  sub_4663c0(void);
/* 0x00458a50 (not exported): park start-up. */
extern void  sub_458a50(void);
/* Progress-screen icon handlers that forward to each other. */
extern char  ProgressAcceptInput(Icon*, int, int, int);       /* 0x0048bc20 */
extern char  LowProgressAcceptInput(Icon*, int, int, int);    /* 0x0048bf90 */
extern char  FreePlayGoBackInput(Icon*, int, int, int);       /* 0x0048fb80 */
extern char  ProgressLevelInput(Icon*, int, int, int);        /* 0x0048bb60 */
extern char  ProgressGoBackInput(Icon*, int, int, int);       /* 0x0048c020 */
extern char  ProgressTutorialInput(Icon*, int, int, int);     /* 0x0048c090 */
char ScriptEndActiveInput(Icon*, int, int, int);              /* 0x00474f80 */
void ScriptSetRunning(int enable);                            /* 0x004748a0 */
int  ScriptRunning(void);                                     /* 0x0046b280 */
void KillLevelMarkerSprites(void);                            /* 0x0048b770 */
void KillLowMarkerSprites(void);                              /* 0x0048bd70 */
void LoadLowMarkerSprites(void);                              /* 0x0048bd00 */
void ReferenceLowMarkerSprites(void);                         /* 0x0048bd40 */

/* 0x0048f9f0 (not exported): starts the intro movie from the three title
 * blocks at 0x007cb30c / 0x007cb300 / 0x007cb2f0. */
extern void  PlayTitleMovie(int* a, int* b, int* c);
extern void  RemoveObjectListIcons(int group);                /* 0x0046fb40 */
/* 0x0048e420 (not exported): un-lights the save popup's OK / close icons. */
extern void  ResetSavePopupIcons(void);
/* 0x0048d4b0 (not exported): rebuilds the saved-game screen. */
extern void  InitSavedGameScreen(void);
/* 0x0048d490 (not exported): restores the icon handlers 0x0048d470 stashed. */
extern void  RestoreSavedGameIconHandlers(void);
/* 0x0048e870 (not exported): writes the new saved game to disk. */
extern void  StoreNewSaveGameToDisk(void);
/* 0x0048faa0 (not exported): drops the title-screen sprites. */
extern void  KillTitleScreenSprites(void);
/* 0x00491550 (not exported): refreshes the current save-slot record. */
extern void  UpDateCurrentSaveSlotInfo(void);
/* 0x00491680 (not exported): writes CurProfile back to the profile list. */
extern void  UpDateCurrentProfile(void);
/* 0x0048eb40 (not exported): restores the icon handlers 0x0048eb20 stashed. */
extern void  RestoreOptionIconHandlers(void);
/* 0x0048f0f0 (not exported): builds the exit confirmation popup at (x,y). */
extern void  InitExitCheckBox(int x, int y);
/* 0x00474880 (not exported): installs the in-game icon handlers
 * (0x00474820 / 0x00474830) into g_icon_handler1 / 2. */
extern void  SetInGameIconHandlers(void);
/* 0x004993c0 (not exported): un-pauses the game timer (PauseGameTimer's twin). */
extern void  ResumeGameTimer(void);

extern int   PlayInstanceOfSample(void* sample, int a, int b, void* src); /* 0x00496d20 */
extern int   PrintSprite(Sprite* s, int x, int y, int mode, BlitCtx* ctx); /* 0x004853a0 */
extern void  SetEditObject(void* obj);                       /* 0x004816e0 */
extern void  ResetFrontEnd(void);                            /* 0x00498920 */
extern void  RestoreIconHandlers(void);                      /* 0x004562e0 */
extern int   TestMenu(Menu* m);                              /* 0x00475710 */
extern void  SetIconSprite(Icon* p, Sprite* s);              /* 0x0046d680 */
extern void  CloseFontEndCheckBox(void);                     /* 0x0048cc10 */
extern void  RemoveIconGroup(int group);                     /* 0x0046d520 */
extern void  SaveProfileToDisk(void);                        /* 0x00491910 */
extern void  DeleteProfileList(void);                        /* 0x00491b50 */
extern char  LoadProfilesFormDisk(void);                     /* 0x00491470 */
extern void  RemoveProfile(unsigned char slot);              /* 0x00491ab0 */
extern void  InitNewProfilePoPUp(Icon* p);                   /* 0x00491290 */
/* 0x00490600 (not exported): opens a front-end/in-game info panel. */
extern void  ShowInfoPanel(int kind);
/* 0x004911c0 (not exported): fills the info panel from the two script texts. */
extern void  SetInfoPanelText(const char* a, const char* b);
/* 0x0046b700 (not exported): ends the running script. */
extern void  EndScript(void);
/* 0x00474750 (not exported): drops the side panel's object icons. */
extern void  ClearObjectMenuIcons(void);
/* 0x0048a800 (not exported): re-reads the selected profile into CurProfile. */
extern void  SelectProfileSlot(void);

/* =========================================================================
 *  Small state accessors
 * ========================================================================= */

/* Non-zero while a script is running (bigscreens.c calls it
 * ScriptRunning_46b280; the flag is the one ScriptSetRunning writes). */
// FUNCTION: LEGOLAND 0x0046b280
int ScriptRunning(void)
{
    return g_script_running;
}

/* Forget the lit theme button and mark all four theme menus closed. */
// FUNCTION: LEGOLAND 0x00474590
void ResetThemeMenuFlags(void)
{
    g_active_theme_icon = 0;
    g_theme_closed[3] = 1;
    g_theme_closed[2] = 1;
    g_theme_closed[1] = 1;
    g_theme_closed[0] = 1;
}

/* =========================================================================
 *  In-game interface bar buttons
 * ========================================================================= */

/* Brief button: drop edit mode and raise the mission-brief panel over the two
 * script text buffers. */
// FUNCTION: LEGOLAND 0x00474f40
char BriefIconInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_game_mode != 1 && (buttons & 2)) {
        g_edit_changed = 0;
        ShowInfoPanel(1);
        SetInfoPanelText(g_script_text1, g_script_text2);
    }
    return 1;
}

/* Script-end button: stop the running script. */
// FUNCTION: LEGOLAND 0x00474fa0
char ScriptEndIconInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_game_mode != 1 && (buttons & 2))
        EndScript();
    return 1;
}

/* Path button: arm the path object the icon carries. */
// FUNCTION: LEGOLAND 0x00474fc0
char PathIconInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_game_mode != 1 && (buttons & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        SetEditObject(p->data);
    }
    return 1;
}

/* Query button: drop path mode and edit mode. */
// FUNCTION: LEGOLAND 0x00475000
char QueryIconInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_game_mode != 1 && (buttons & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_ui_flags &= ~0x1000;
        g_edit_changed = 0;
    }
    return 1;
}

/* Eraser button: drop query mode, edit mode 2. */
// FUNCTION: LEGOLAND 0x00475040
char EraserIconInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_game_mode != 1 && (buttons & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_ui_flags &= ~0x400;
        g_edit_changed = 2;
    }
    return 1;
}

/* Map button: toggles the full-screen map. Entering stashes the current game
 * mode in 0x667c60 and drops query/path mode; leaving restores it through the
 * icon-handler restore. Unlike its neighbours this one works in mode 1 (that
 * is how the map is left again). */
// FUNCTION: LEGOLAND 0x00475080
char MapIconInput(Icon* p, int buttons, int a3, int a4)
{
    if ((buttons & 2) != 0) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        if (g_game_mode != 1) {
            ResetFrontEnd();
            g_ui_flags &= ~0x1400;
            g_8119bc = 1;
            g_6687b0 = 4;
            g_edit_changed = 0;
            g_game_mode_saved = g_game_mode;
            g_game_mode = 1;
        } else {
            g_80ff70 = 1;
            g_game_mode = g_game_mode_saved;
            g_game_mode_saved = 1;
            RestoreIconHandlers();
        }
    } else {
        return 1;
    }
    return 1;
}

/* Options button: leaves the park for the front-end option screen (screen -1,
 * mode 5) with the alternate icon set. Ignored while something is being
 * dragged. */
// FUNCTION: LEGOLAND 0x00475120
char OptionsIconInput(Icon* p, int buttons, int a3, int a4)
{
    if ((buttons & 2) && g_game_mode != 1 && !g_drag_lock) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_ui_flags &= ~0x1400;
        g_edit_changed = 0;
        g_icons2_mode = 1;
        g_game_mode = 2;
        g_cur_screen = -1;
        g_screen_mode = 5;
        ResetFrontEnd();
        g_6687b0 = 4;
    }
    return 1;
}

/* Show or hide the four theme buttons from the four ints at 0x668e20. */
// FUNCTION: LEGOLAND 0x00474990
void UpdateThemeIconsFromFlags(void)
{
    int i;

    for (i = 0; i < 4; i++) {
        if (g_theme_enabled[i])
            g_theme_icon[i]->flags &= ~0x400;
        else
            g_theme_icon[i]->flags |= 0x400;
    }
}

/* Show or hide the four theme buttons from the current profile's four
 * per-theme unlock bytes (CurProfile+0x30). */
// FUNCTION: LEGOLAND 0x00476180
void UpdateThemeIconsFromProfile(void)
{
    unsigned char* t = g_profile_themes;
    Icon**         q = g_theme_icon;
    int            n = 4;

    do {
        if (*t)
            (*q)->flags &= ~0x400;
        else
            (*q)->flags |= 0x400;
        t++;
        q++;
    } while (--n);
}

/* The brief button's redraw hook: the focussed icon draws its own sprite, any
 * other draws the alternate one at +0x18. */
// FUNCTION: LEGOLAND 0x0046e040
int RenderBriefIcon(Icon* p)
{
    BlitCtx ctx;

    ctx.kind = 2;
    ctx.owner.p = p;
    ctx.owner.n = 0;
    if (p == g_focussed_icon)
        PrintSprite(p->sprite, p->x, p->y, 0, &ctx);
    else
        PrintSprite(p->u18.alt, p->x, p->y, 0, &ctx);
    return 0;
}

/* The four theme buttons. Same body four times over: clicking the open
 * theme closes the panel, clicking another opens it. `rc = 1; return rc;` in
 * the post-guard block is load-bearing -- it is what makes VC6 sink `push edi`
 * (the register VC6 parks the constant 0 in here, because theme 0 makes six zeros) past the two guards and duplicate the tail into the arms.
 * The failure path puts the previous menu index and object-list mode back.
 * Menu 0, closed flag 0x4bb094. */
// FUNCTION: LEGOLAND 0x004751a0
char LegolandThemeInput(Icon* p, int buttons, int a3, int a4)
{
    int prev_mode = g_object_list_mode;
    int prev_menu = g_menu_index;

    if (g_game_mode == 1 || (buttons & 2) == 0)
        return 1;
    {
        char rc;

        g_edit_changed = 0;
        g_ui_flags &= ~0x1400;
        PlayInstanceOfSample(g_snd_theme, 0, 1, 0);
        if (g_menu_index != 0) {
            g_menu_index = 0;
            g_object_list_mode = 0;
            if (TestMenu(&g_menus[0]) == 1) {
                ClearObjectMenuIcons();
                g_active_theme_icon = p;
                SetIconSprite(p, g_theme_legoland_on);
                g_theme_closed[0] = 0;
            } else {
                g_menu_index = prev_menu;
                if (prev_menu != 5) {
                    g_object_list_mode = prev_mode;
                    TestMenu(&g_menus[prev_menu]);
                }
            }
        } else {
            ClearObjectMenuIcons();
            g_theme_closed[0] = 1;
            g_menu_index = 5;
            g_panel_state.f00 = 1;
            g_panel_state.f04 = 1;
        }
        rc = 1;
        return rc;
    }
}

/* Western: menu 1, closed flag 0x4bb09c. No zero register (theme 1
 * reuses the constant-1 register), so only ebx/ebp/esi are saved. */
// FUNCTION: LEGOLAND 0x004754b0
char WesternThemeInput(Icon* p, int buttons, int a3, int a4)
{
    int prev_mode = g_object_list_mode;
    int prev_menu = g_menu_index;

    if (g_game_mode == 1 || (buttons & 2) == 0)
        return 1;
    {
        char rc;

        g_edit_changed = 0;
        g_ui_flags &= ~0x1400;
        PlayInstanceOfSample(g_snd_theme, 0, 1, 0);
        if (g_menu_index != 1) {
            g_menu_index = 1;
            g_object_list_mode = 0;
            if (TestMenu(&g_menus[1]) == 1) {
                ClearObjectMenuIcons();
                g_active_theme_icon = p;
                SetIconSprite(p, g_theme_western_on);
                g_theme_closed[2] = 0;
            } else {
                g_menu_index = prev_menu;
                if (prev_menu != 5) {
                    g_object_list_mode = prev_mode;
                    TestMenu(&g_menus[prev_menu]);
                }
            }
        } else {
            ClearObjectMenuIcons();
            g_theme_closed[2] = 1;
            g_menu_index = 5;
            g_panel_state.f00 = 1;
            g_panel_state.f04 = 1;
        }
        rc = 1;
        return rc;
    }
}

/* Castle: menu 2, closed flag 0x4bb098. Neither 1 nor 2 earns a register
 * here, so `prev_mode` is spilled to the frame and only esi is saved. */
// FUNCTION: LEGOLAND 0x004753a0
char CastleThemeInput(Icon* p, int buttons, int a3, int a4)
{
    int prev_mode = g_object_list_mode;
    int prev_menu = g_menu_index;

    if (g_game_mode == 1 || (buttons & 2) == 0)
        return 1;
    {
        char rc;

        g_edit_changed = 0;
        g_ui_flags &= ~0x1400;
        PlayInstanceOfSample(g_snd_theme, 0, 1, 0);
        if (g_menu_index != 2) {
            g_menu_index = 2;
            g_object_list_mode = 0;
            if (TestMenu(&g_menus[2]) == 1) {
                ClearObjectMenuIcons();
                g_active_theme_icon = p;
                SetIconSprite(p, g_theme_castle_on);
                g_theme_closed[1] = 0;
            } else {
                g_menu_index = prev_menu;
                if (prev_menu != 5) {
                    g_object_list_mode = prev_mode;
                    TestMenu(&g_menus[prev_menu]);
                }
            }
        } else {
            ClearObjectMenuIcons();
            g_theme_closed[1] = 1;
            g_menu_index = 5;
            g_panel_state.f00 = 1;
            g_panel_state.f04 = 1;
        }
        rc = 1;
        return rc;
    }
}

/* Adventurers: menu 3, closed flag 0x4bb0a0. ORIGINAL BUG, reproduced: unlike
 * the other three this one does NOT restore the previous menu index when
 * TestMenu fails and does not skip the retry when there was no menu open --
 * it re-reads g_menu_index (which it has just set to 3) and re-runs TestMenu
 * on menu 3, so a failed open of the adventurers menu leaves g_menu_index at
 * 3 with the panel empty. */
// FUNCTION: LEGOLAND 0x004752a0
char AdventureThemeInput(Icon* p, int buttons, int a3, int a4)
{
    int prev_mode = g_object_list_mode;
    int prev_menu = g_menu_index;

    if (g_game_mode == 1 || (buttons & 2) == 0)
        return 1;
    {
        char rc;

        g_edit_changed = 0;
        g_ui_flags &= ~0x1400;
        PlayInstanceOfSample(g_snd_theme, 0, 1, 0);
        if (g_menu_index != 3) {
            g_menu_index = 3;
            g_object_list_mode = 0;
            if (TestMenu(&g_menus[3]) == 1) {
                ClearObjectMenuIcons();
                g_active_theme_icon = p;
                SetIconSprite(p, g_theme_adv_on);
                g_theme_closed[3] = 0;
            } else {
                g_object_list_mode = prev_mode;
                TestMenu(&g_menus[g_menu_index]);
            }
        } else {
            ClearObjectMenuIcons();
            g_theme_closed[3] = 1;
            g_menu_index = 5;
            g_panel_state.f00 = 1;
            g_panel_state.f04 = 1;
        }
        rc = 1;
        return rc;
    }
}

/* =========================================================================
 *  Profile list screen  (screens2.c InitListProfiles)
 * ========================================================================= */

/* Accept: commits the selected profile and moves to sub-screen 1. Inert while
 * the delete popup is up, while the accept icon is hidden (flag 0x400) or
 * with no slot selected. A pending NEW profile is written to disk first and
 * the list reloaded. */
// FUNCTION: LEGOLAND 0x0048d300
char ProfileAcceptInput(Icon* p, int buttons, int a3, int a4)
{
    if (!g_delete_popup_up && (buttons & 2) &&
        !(g_accept_icon->flags & 0x400) && g_profile_slot) {
        if (g_newprof_popup_up) {
            SaveProfileToDisk();
            DeleteProfileList();
            LoadProfilesFormDisk();
            RemoveIconGroup(0x15);
            CloseFontEndCheckBox();
            g_newprof_popup_up = 0;
        }
        ResetFrontEnd();
        g_6687b0 = 4;
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_screen_mode = 1;
    }
    return 1;
}

/* An occupied profile slot: select it. */
// FUNCTION: LEGOLAND 0x0048d390
char ProfileSlotInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        g_profile_slot = p->slot;
        SelectProfileSlot();
    }
    return 1;
}

/* An empty profile slot: raise the new-profile popup over it. */
// FUNCTION: LEGOLAND 0x0048d3c0
char ProfileEmptySlotInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        g_profile_slot = p->slot;
        g_newprof_popup_up = 1;
        InitNewProfilePoPUp(p);
        g_frontend_checkbox_closed = 0;
    }
    return 1;
}

/* The delete-profile popup's OK button: really delete the slot and go back to
 * the top of the front end (screen -1, mode 0). */
// FUNCTION: LEGOLAND 0x0048d400
char ProfileOkInput(Icon* p, int buttons, int a3, int a4)
{
    if ((buttons & 2) && g_profile_slot) {
        CloseFontEndCheckBox();
        g_delete_popup_up = 0;
        RemoveProfile(g_profile_slot);
        g_profile_slot = 0;
        g_cur_screen = -1;
        g_screen_mode = 0;
    }
    return 1;
}

/* The delete-profile popup's close button. */
// FUNCTION: LEGOLAND 0x0048d450
char ProfileCloseInput(Icon* p, int buttons, int a3, int a4)
{
    if (buttons & 2) {
        g_delete_popup_up = 0;
        CloseFontEndCheckBox();
    }
    return 1;
}

/* =========================================================================
 *  Saved-game, option, title and exit screens (screens2.c)
 * ========================================================================= */

/* The saved-game popup's close button: drops the popup, rebuilds the
 * saved-game screen and re-arms the front-end icon handlers. Like every popup
 * button it first re-lights itself: a null icon means "the popup's own close
 * icon", which is how the screen code calls it directly. */
// FUNCTION: LEGOLAND 0x0048e810
char SaveGameCloseInput(Icon* p, int buttons, int a3, int a4)
{
    ResetSavePopupIcons();
    if (!p)
        p = g_np_close_icon;
    SetIconSprite(p, g_fe_sprite_680);
    if (buttons & 2) {
        g_cur_save_slot = 0;
        RemoveIconGroup(7);
        InitSavedGameScreen();
        g_newsave_popup_up = 0;
        g_frontend_checkbox_closed = 1;
        RestoreSavedGameIconHandlers();
    }
    return 1;
}

/* The in-game exit popup's OK button: leave the park (game mode 0). */
// FUNCTION: LEGOLAND 0x0048ef90
char ExitOkInput(Icon* p, int buttons, int a3, int a4)
{
    if (buttons & 2) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        CloseFontEndCheckBox();
        g_game_mode = 0;
        RemoveIconGroup(7);
    }
    return 1;
}

/* Option screen Accept: back into the park. Commits the save-slot record when
 * a slot is live, drops the front-end icon set and restarts the game timer. */
// FUNCTION: LEGOLAND 0x0048efd0
char OptionAcceptInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        ResetFrontEnd();
        g_6687b0 = 4;
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        if (g_cur_save_slot)
            UpDateCurrentSaveSlotInfo();
        g_icons2_mode = 0;
        RemoveIconGroup(7);
        KillTitleScreenSprites();
        g_game_mode = 3;
        SetInGameIconHandlers();
        UpDateCurrentProfile();
        ResumeGameTimer();
    }
    return 1;
}

/* Option screen Save: go to the saved-game screen in SAVE mode (sub-mode 4). */
// FUNCTION: LEGOLAND 0x0048f050
char OptionSaveInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_save_is_load = 0;
        g_save_7cb324 = 0;
        g_screen_mode = 4;
    }
    return 1;
}

/* Option screen Load: the same screen in LOAD mode. */
// FUNCTION: LEGOLAND 0x0048f550
char OptionLoadInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_save_is_load = 1;
        g_save_7cb324 = 0;
        g_screen_mode = 4;
    }
    return 1;
}

/* Option screen Exit: raise the exit confirmation popup at (0xfa,0x28). */
// FUNCTION: LEGOLAND 0x0048f0a0
char TitleExitInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        InitExitCheckBox(0xfa, 0x28);
        g_exit_popup_up = 1;
        g_frontend_checkbox_closed = 0;
    }
    return 1;
}

/* Exit popup OK (from the option screen): tear the front end down and go to
 * pending state 3 with the park running again. */
// FUNCTION: LEGOLAND 0x0048f440
char ExitBigOkInput(Icon* p, int buttons, int a3, int a4)
{
    if (buttons & 2) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_exit_popup_up = 0;
        CloseFontEndCheckBox();
        g_pending_state = 3;
        g_icons2_mode = 0;
        RemoveIconGroup(7);
        RemoveObjectListIcons(0xd2);
        KillTitleScreenSprites();
        g_game_mode = 3;
        RestoreOptionIconHandlers();
        SetInGameIconHandlers();
    }
    return 1;
}

/* Exit popup close: just drop the popup. Uses the close sample, not the
 * click one. */
// FUNCTION: LEGOLAND 0x0048f4b0
char ExitCloseInput(Icon* p, int buttons, int a3, int a4)
{
    if (buttons & 2) {
        PlayInstanceOfSample(g_snd_close, 0, 1, 0);
        g_exit_popup_up = 0;
        CloseFontEndCheckBox();
        RestoreOptionIconHandlers();
    }
    return 1;
}

/* Title screen Load: the saved-game screen in LOAD mode, entered from the
 * title (0x7cb324 = 1 marks "came from the title", not the option screen). */
// FUNCTION: LEGOLAND 0x0048fe20
char TitleLoadInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_save_is_load = 1;
        g_save_7cb324 = 1;
        g_screen_mode = 4;
    }
    return 1;
}

/* Title screen New: start a normal game -- save type 1, front-end mode 2,
 * sub-screen 6 (the theme/level chooser), pending state cleared. */
// FUNCTION: LEGOLAND 0x0048feb0
char TitleNewInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        ResetFrontEnd();
        g_6687b0 = 4;
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_save_type = 1;
        RemoveIconGroup(7);
        KillTitleScreenSprites();
        g_game_mode = 2;
        g_screen_mode = 6;
        g_pending_state = 0;
    }
    return 1;
}

/* Title screen Free Play: sub-screen 3. NOTE the return value -- this is the
 * one handler in the game that answers 2 rather than 1 when it acted (it
 * still answers 1 when it did not). */
// FUNCTION: LEGOLAND 0x0048ff20
char TitleFreeInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        ResetFrontEnd();
        g_6687b0 = 4;
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_screen_mode = 3;
        return 2;
    }
    return 1;
}

/* Title screen Register: the profile list (sub-screen 0). */
// FUNCTION: LEGOLAND 0x0048ff70
char TitleRegInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_screen_mode = 0;
    }
    return 1;
}

/* Title screen Movie: plays the intro, then comes back to sub-screen 9 with
 * the alternate icon set. */
// FUNCTION: LEGOLAND 0x0048ffe0
char TitleMovieInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        PlayTitleMovie(&g_movie_7cb30c, &g_movie_7cb300, &g_movie_7cb2f0);
        g_game_mode = 2;
        g_cur_screen = -1;
        g_screen_mode = 9;
        g_icons2_mode = 1;
    }
    return 1;
}

/* =========================================================================
 *  Progress screen  (bigscreens.c InitProgressScreen builds it)
 * ========================================================================= */

/* Normalises the profile's per-level "done" bytes to exactly 1 and leaves
 * g_last_level holding the index of the highest one set. Level 1 is always
 * marked done. (bigscreens.c calls it ProgressInit_48b6d0.) */
// FUNCTION: LEGOLAND 0x0048b6d0
void NormaliseLevelsDone(void)
{
    int i;

    g_level_done[0] = 1;
    for (i = 0; i < 15; i++) {
        if (g_level_done[i]) {
            g_level_done[i] = 1;
            g_last_level = i;
        }
    }
}

/* Loads the lit and dim sprite of every levels-6..15 marker. */
// FUNCTION: LEGOLAND 0x0048b700
void LoadLevelMarkerSprites(void)
{
    int i;

    for (i = 0; i < 10; i++) {
        g_level_markers[i].lit = LoadSprite(g_level_markers[i].lit_name, 4);
        g_level_markers[i].dim = LoadSprite(g_level_markers[i].dim_name, 4);
    }
}

/* Takes a reference on both sprites of every levels-6..15 marker, so the
 * icons built from them survive the next RemoveIconGroup.
 * (bigscreens.c calls it Progress_48b740.) */
// FUNCTION: LEGOLAND 0x0048b740
void ReferenceLevelMarkerSprites(void)
{
    int i;

    for (i = 0; i < 10; i++) {
        ReferenceSprite(g_level_markers[i].lit);
        ReferenceSprite(g_level_markers[i].dim);
    }
}

/* Drops the progress screen's icons and releases every reference held on the
 * levels-6..15 marker sprites -- KillSprite answers 0 while references
 * remain, so each is killed in a loop until it answers non-zero. */
// FUNCTION: LEGOLAND 0x0048b770
void KillLevelMarkerSprites(void)
{
    int i;

    RemoveIconGroup(0x1c);
    RemoveIconGroup(0x23);
    for (i = 0; i < 10; i++) {
        while (KillSprite(g_level_markers[i].lit) == 0)
            ;
        while (KillSprite(g_level_markers[i].dim) == 0)
            ;
        g_level_markers[i].dim = 0;
        g_level_markers[i].lit = 0;
    }
}

/* The same three for the levels-1..5 table. */
// FUNCTION: LEGOLAND 0x0048bd00
void LoadLowMarkerSprites(void)
{
    int i;

    for (i = 0; i < 5; i++) {
        g_low_markers[i].lit = LoadSprite(g_low_markers[i].lit_name, 4);
        g_low_markers[i].dim = LoadSprite(g_low_markers[i].dim_name, 4);
    }
}

// FUNCTION: LEGOLAND 0x0048bd40
void ReferenceLowMarkerSprites(void)
{
    int i;

    for (i = 0; i < 5; i++) {
        ReferenceSprite(g_low_markers[i].lit);
        ReferenceSprite(g_low_markers[i].dim);
    }
}

// FUNCTION: LEGOLAND 0x0048bd70
void KillLowMarkerSprites(void)
{
    int i;

    RemoveIconGroup(0x1c);
    RemoveIconGroup(0x23);
    for (i = 0; i < 5; i++) {
        while (KillSprite(g_low_markers[i].lit) == 0)
            ;
        while (KillSprite(g_low_markers[i].dim) == 0)
            ;
        g_low_markers[i].dim = 0;
        g_low_markers[i].lit = 0;
    }
}

/* A level marker. Every click sets g_progress_resume, so the screen is not
 * rebuilt from scratch on the way back. The marker itself is a DOUBLE-CLICK
 * control: a second click on the same level within 500 ms is forwarded to the
 * Accept handler (the levels-1..5 twin when that screen is up), which starts
 * the game. A single click just selects the level (map level = marker + 1)
 * and re-renders the screen. */
// FUNCTION: LEGOLAND 0x0048bb60
char ProgressLevelInput(Icon* p, int buttons, int a3, int a4)
{
    g_progress_resume = 1;
    if (buttons & 2) {
        if ((int)(GetTicks() - g_progress_click_ms) < 500 &&
            p->u18.level == g_progress_click_level) {
            if (g_progress_is_low)
                return LowProgressAcceptInput(p, buttons, a3, a4);
            return ProgressAcceptInput(p, buttons, a3, a4);
        }
        g_progress_click_ms = GetTicks();
        g_progress_click_level = p->u18.level;
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_level_map->level = p->u18.level + 1;
        g_cur_screen = -1;
    }
    return 1;
}

/* Progress screen Accept (also reached by double-clicking a marker): tears the
 * front end down and starts the level. Level > 15 means the game is finished,
 * so it goes to the end-of-game screen (mode 8) with the alternate icon set
 * instead. Both arms fall into one `rc = 1; return rc;`, which is what makes
 * VC6 sink `push esi` (the constant 0) past the button guard. */
// FUNCTION: LEGOLAND 0x0048bc20
char ProgressAcceptInput(Icon* p, int buttons, int a3, int a4)
{
    if ((buttons & 2) != 0) {
        char rc;

        g_icon_handler2 = 0;
        g_icon_handler1 = 0;
        ResetFrontEnd();
        g_6687b0 = 4;
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        KillLevelMarkerSprites();
        if (g_level_map->level <= 15) {
            sub_466360(0xfa, 0x181);
            g_icons2_mode = 0;
            InitGameInterface(1);
            g_game_mode = 3;
            SetInGameIconHandlers();
            sub_458a50();
            sub_4663c0();
            g_progress_resume = 0;
            g_progress_798668 = 0;
        } else {
            g_ui_flags &= ~0x20;
            g_progress_resume = 0;
            g_progress_798668 = 0;
            g_game_mode = 2;
            g_cur_screen = -1;
            g_screen_mode = 8;
            g_icons2_mode = 1;
        }
        rc = 1;
        return rc;
    }
    return 1;
}

/* The levels-1..5 screen's Accept. Same shape without the end-of-game arm,
 * and the park is brought up at (0x186,0x18b) rather than (0xfa,0x181). */
// FUNCTION: LEGOLAND 0x0048bf90
char LowProgressAcceptInput(Icon* p, int buttons, int a3, int a4)
{
    if ((buttons & 2) != 0) {
        char rc;

        g_icon_handler2 = 0;
        g_icon_handler1 = 0;
        ResetFrontEnd();
        g_6687b0 = 4;
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        KillLowMarkerSprites();
        sub_466360(0x186, 0x18b);
        g_icons2_mode = 0;
        InitGameInterface(1);
        g_game_mode = 3;
        SetInGameIconHandlers();
        sub_458a50();
        sub_4663c0();
        g_progress_resume = 0;
        g_progress_798668 = 0;
        rc = 1;
        return rc;
    }
    return 1;
}

/* Progress screen Go Back: drops the marker sprites of whichever of the two
 * progress screens is up and hands the click on to the free-play Go Back
 * handler, whose answer it returns. */
// FUNCTION: LEGOLAND 0x0048c020
char ProgressGoBackInput(Icon* p, int buttons, int a3, int a4)
{
    if (buttons & 2) {
        g_icon_handler2 = 0;
        g_icon_handler1 = 0;
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_progress_resume = 0;
        g_progress_798668 = 0;
        if (g_progress_is_low)
            KillLowMarkerSprites();
        else
            KillLevelMarkerSprites();
        return FreePlayGoBackInput(p, buttons, a3, a4);
    }
    return 1;
}

/* Progress screen Tutorial: restarts at the first level of whichever screen is
 * up (6 for levels 6..15, 1 for levels 1..5) with 0x798668 set, which is what
 * stops InitProgressScreen advancing the level again. */
// FUNCTION: LEGOLAND 0x0048c090
char ProgressTutorialInput(Icon* p, int buttons, int a3, int a4)
{
    if (buttons & 2) {
        g_progress_resume = 0;
        g_progress_798668 = 1;
        g_cur_screen = -1;
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        if (g_progress_is_low) {
            KillLowMarkerSprites();
            g_level_map->level = 6;
            return 1;
        }
        KillLevelMarkerSprites();
        g_level_map->level = 1;
    }
    return 1;
}

/* The levels 1..5 screen -- the TUTORIAL report, backdrop TutorialBK.lls.
 * bigscreens.c's InitProgressScreen jumps straight here when the level is 5 or
 * less and calls it SkipProgressScreen; it does not skip anything, it builds
 * the other progress screen, so it is renamed here.
 *
 * Layout: Accept_on_Report at (0x20a,0x16c) and GoBack_on_Tut at (0x20a,0xf5),
 * both in group 0x23, plus one marker per tutorial level in group 0x1c from
 * g_low_markers -- five Appraisal_Yes / Appraisal_No ticks in a column at
 * x=30, y=130,154,178,202,226 (help strings 0x14..0x18). The current level
 * gets the "yes" tick, an already-done level the "no" one, and a level that is
 * neither gets no icon at all (its g_low_icons slot is left null).
 *
 * The Go Back button doubles as the Tutorial button: with a profile selected
 * (g_have_profile == 1) it becomes "Tutorial" (help 0x26c, handler
 * ProgressTutorialInput), otherwise it stays "Go Back" (help 0x26). The
 * original assigns the Go Back help string BEFORE the test and then again in
 * the else arm -- two GetString(0x26) calls on that path; reproduced. */
// FUNCTION: LEGOLAND 0x0048bde0
void InitTutorialScreen(void)
{
    Icon* p;
    int   i;

    g_backdrop = LoadSprite(g_lls_tutorial_bk, 0);
    g_progress_is_low = 1;
    if (g_progress_resume == 0) {
        LoadLowMarkerSprites();
        p = LoadSpriteIcon(g_lls_accept_on_report, 4, 0x20a, 0x16c, 0x23);
        p->help_id = 0x262;
        p->help = GetString(0x262);
        p->flags |= 0x6002;
        p->input = LowProgressAcceptInput;
        g_icon_handler1 = LowProgressAcceptInput;
        p = LoadSpriteIcon(g_lls_goback_on_tut, 4, 0x20a, 0xf5, 0x23);
        p->help_id = 0x26;
        p->help = GetString(0x26);
        if (g_have_profile == 1) {
            p->help_id = 0x26c;
            p->help = GetString(0x26c);
            p->input = ProgressTutorialInput;
        } else {
            p->help_id = 0x26;
            p->help = GetString(0x26);
            p->input = ProgressGoBackInput;
        }
        p->flags |= 0x6002;
        g_icon_handler2 = p->input;
    }
    ReferenceLowMarkerSprites();
    RemoveIconGroup(0x1c);
    for (i = 0; i < 5; i++) {
        if (i == g_level_map->level - 1)
            p = InsertIcon(g_low_markers[i].x, g_low_markers[i].y, 0x1c,
                           g_low_markers[i].lit);
        else if (g_level_done[i] == 1)
            p = InsertIcon(g_low_markers[i].x, g_low_markers[i].y, 0x1c,
                           g_low_markers[i].dim);
        else
            p = 0;
        g_low_icons[i] = p;
        if (p) {
            p->help = GetString(0x276);
            p->help_id = g_low_markers[i].str_id;
            p->u18.level = (unsigned char)i;
            p->input = ProgressLevelInput;
            p->flags |= 0x6002;
        }
    }
}

/* =========================================================================
 *  Shared icon-list helpers (iconui.c neighbours)
 * ========================================================================= */

/* Swaps an icon's sprite, keeping the reference counts straight: the old one
 * is killed and the new one referenced, and nothing happens at all when the
 * icon already holds it. Every "light this button up" path goes through here. */
// FUNCTION: LEGOLAND 0x0046d680
void SetIconSprite(Icon* p, Sprite* s)
{
    if (p && s != p->sprite) {
        if (p->sprite)
            KillSprite(p->sprite);
        if (s)
            ReferenceSprite(s);
        p->sprite = s;
    }
}

/* Drops a whole object-list panel: the five icons of the group (base+3
 * scroll-up, base+4 scroll-down, base+5 the list itself, base+6 the extra),
 * frees the list icon's widget block and then the group. Nothing happens
 * when the list icon (base+5) is not there. */
// FUNCTION: LEGOLAND 0x0046fb40
void RemoveObjectListIcons(int group)
{
    Icon* list = FindIcon(group + 5);

    if (list) {
        Icon* p;
        p = FindIcon(group + 3);
        if (p)
            DeleteIcon(p);
        p = FindIcon(group + 4);
        if (p)
            DeleteIcon(p);
        p = FindIcon(group + 6);
        if (p)
            DeleteIcon(p);
        MemFree(list->widget);
        DeleteIcon(list);
        RemoveIconGroup(group);
    }
}

/* =========================================================================
 *  Script state (the in-game mission script)
 * ========================================================================= */

/* Sets the script-running flag and re-dresses the ScriptEnd button; when a
 * script really is starting it also kicks the script machine. */
// FUNCTION: LEGOLAND 0x0046b240
void StopScript(int running)
{
    g_script_running = running;
    ScriptSetRunning(1);
    if (running) {
        sub_458be0();
        if (!g_832ba8)
            sub_459820(1);
    }
}

/* Enables / disables the ScriptEnd button. While a script is running it also
 * gets its "abandon" help string (0x8fc) and the handler that ends it; when
 * none is, `enable` alone decides whether the icon is shown and clickable
 * (flags 0x2002 = visible + hit-testable). */
// FUNCTION: LEGOLAND 0x004748a0
void ScriptSetRunning(int enable)
{
    if (g_script_end_icon) {
        if (!ScriptRunning()) {
            if (enable)
                g_script_end_icon->flags |= 0x2002;
            else
                g_script_end_icon->flags &= ~0x2002;
        } else {
            g_script_end_icon->flags |= 0x2002;
            g_script_end_icon->input = ScriptEndActiveInput;
            g_script_end_icon->help_id = 0x8fc;
            g_script_end_icon->help = GetString(0x8fc);
        }
    }
}

/* The ScriptEnd button while a script IS running (ScriptSetRunning installs
 * it): abandons the script. */
// FUNCTION: LEGOLAND 0x00474f80
char ScriptEndActiveInput(Icon* p, int buttons, int a3, int a4)
{
    if (buttons & 2)
        sub_459820(1);
    return 1;
}

/* =========================================================================
 *  More front-end handlers
 * ========================================================================= */

/* Free-play / progress screen Go Back: to front-end sub-screen 1. The
 * `if (g_icons2_mode) g_icons2_mode = 0;` is the original's shape, not a
 * simplification (VC6 kept the test). */
// FUNCTION: LEGOLAND 0x0048fb80
char FreePlayGoBackInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        if (g_icons2_mode)
            g_icons2_mode = 0;
        g_screen_mode = 1;
    }
    return 1;
}

/* The delete (bin) icon that LightUpthisDeleteIcon parks beside a slot:
 * raises the right confirmation popup for the screen it is on -- the profile
 * one on sub-screen 0, the saved-game one on sub-screen 4. */
// FUNCTION: LEGOLAND 0x0048cc30
char DeleteIconInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        if (g_screen_mode == 0)
            InitProfileCheckBoxIcons(p);
        if (g_screen_mode == 4)
            InitSaveDeletePopUp(p);
        g_frontend_checkbox_closed = 0;
        g_delete_popup_up = 1;
    }
    return 1;
}

/* Shows or hides the help-text icon depending on whether the level has any
 * text for `key`, and answers which. The text buffer the loader leaves behind
 * is freed either way. (bigscreens.c calls it InitSidePanel_491240.) */
// FUNCTION: LEGOLAND 0x00491240
int UpdateHelpIconForText(void* key)
{
    if (!g_help_text_icon)
        return 0;
    {
        int rc;

        if (LoadHelpTextFor(key)) {
            g_help_text_icon->flags &= ~0x400;
            rc = 1;
        } else {
            g_help_text_icon->flags |= 0x400;
            rc = 0;
        }
        FreeHelpTextBuffer();
        return rc;
    }
}

/* =========================================================================
 *  Saved-game screen (screens2.c / bigscreens.c InitSavedGameScreen)
 * ========================================================================= */

/* Accept on the saved-game screen in LOAD mode: pulls the slot's header into
 * the temp profile and, if the slot really holds a game, copies its settings
 * (save type and the three volumes) into CurProfile before tearing the screen
 * down. 0x7cb324 marks "came from the title screen", where the load is
 * started elsewhere; from the option screen 0x00458b20 starts it here. */
// FUNCTION: LEGOLAND 0x0048d970
char LoadAcceptInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2) && g_cur_save_slot) {
        ResetFrontEnd();
        g_6687b0 = 4;
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        if (LoadDateIntoTempProfile(g_profile_slot, g_cur_save_slot_wide & 0xff)) {
            g_save_type = g_temp_save_type;
            g_cur_80ffc0 = g_temp_7cad80;
            g_vol_speech = g_temp_vol_speech;
            g_vol_music = g_temp_vol_music;
            g_vol_sfx = g_temp_vol_sfx;
            g_667c64 = 1;
            g_667c80 = 1;
        }
        RemoveIconGroup(7);
        KillSaveScreenSprites();
        KillTitleScreenSprites();
        DeleteSavedGameList();
        if (!g_save_7cb324)
            sub_458b20();
    }
    return 1;
}

/* Go Back from the saved-game screen: to sub-screen 5 (the options). */
// FUNCTION: LEGOLAND 0x0048db10
char SaveGoBackInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_screen_mode = 5;
    }
    return 1;
}

/* The plain "write the game out" Accept (no screen teardown). */
// FUNCTION: LEGOLAND 0x0048f5a0
char SaveAcceptInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        StoreNewSaveGameToDisk();
    }
    return 1;
}

/* An occupied save slot. In LOAD mode it just selects the slot; in SAVE mode
 * it is the empty-slot handler that runs (overwriting an existing game asks
 * for a name the same way a new one does), and its answer is returned. */
// FUNCTION: LEGOLAND 0x0048e4a0
char SaveSlotInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        if (g_save_is_load)
            g_cur_save_slot = p->slot;
        else
            return SaveEmptySlotInput(p, buttons, a3, a4);
    }
    return 1;
}

/* An empty save slot (SAVE mode only): clears the temp profile, selects the
 * slot and raises the name popup, seeded with the slot's existing name unless
 * that is the literal "EMPTY". */
/* WIP: 22/27 instructions, byte-exact to index 21. The residual is the tail:
 * the original stores g_newsave_popup_up = 1 as an immediate and lets the body
 * fall through into the shared `mov al,1 / ret`, where VC6 here materialises
 * the return value into eax first (`mov eax,1`), reuses it for that store and
 * so has to duplicate the return block. Neither statement order, an extra
 * scope, split guards, an `rc` local nor a volatile store moves it; a volatile
 * store DOES fix the store order (popup before checkbox) but not the eax. */
// WIP-FUNCTION: LEGOLAND 0x0048e4f0  (81%, tail return block duplicated)
char SaveEmptySlotInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2) && !g_save_is_load) {
        ResetTempProfile();
        g_cur_save_slot = p->slot;
        InitNewSaveGamePOPUP(p);
        if (p->u18.owner != g_str_empty_slot)
            sub_48e3d0(p->u18.owner);
        g_newsave_popup_up = 1;
        g_frontend_checkbox_closed = 0;
    }
    return 1;
}

/* The new-profile popup's close button. The `if (sub_4913f0)` is the
 * original's own dead test -- it loads a FUNCTION ADDRESS into eax and tests
 * it against zero, which can never be false; reproduced as written. */
// FUNCTION: LEGOLAND 0x004920a0
char NewProfileCloseInput(Icon* p, int buttons, int a3, int a4)
{
    if ((buttons & 2) && sub_4913f0) {
        RemoveIconGroup(0x15);
        CloseFontEndCheckBox();
        g_cur_screen = -1;
        g_newprof_popup_up = 0;
        g_profile_slot = 0;
    }
    return 1;
}

/* =========================================================================
 *  profiles.c / savegame.c helpers
 * ========================================================================= */

/* Stamps the "time of last save" with the game timer. */
// FUNCTION: LEGOLAND 0x0047f810
void ResetSaveTimer(void)
{
    g_save_time = GetGameTimer();
}

/* Frees the singly linked list at 0x0066b44c and tails into the rest of the
 * teardown. Ends in a tail JMP, not a ret, AND is one instruction out:
 * 13/14, the head store is `mov [g],esi` where the original writes the copy
 * in eax (`mov eax,esi / ... / mov [g],eax`). Every loop spelling tried
 * (while/do-while/for, chained assignment, an explicit break on `next`)
 * produces the same register choice. */
// WIP-FUNCTION: LEGOLAND 0x004828f0  (93%, head store from esi not eax; also a tail-jmp function)
void sub_4828f0(void)
{
    void* p;
    void* next;

    p = g_66b44c;
    if (p) {
        do {
            next = *(void**)p;
            MemFree(p);
            p = next;
            g_66b44c = p;
        } while (next);
    }
    sub_482a80();
}

/* Wipes every cell of the 256x256 map grid (all five dwords of the 20-byte
 * record) and drops the overlay chain. The row pointer is re-read from
 * g_map_rows on every cell because the stores may alias it. */
// FUNCTION: LEGOLAND 0x00463680
void ClearMapCells(void)
{
    int y;
    int x;

    for (y = 0; y < 256; y++)
        for (x = 0; x < 256; x++)
            memset(&g_map_rows[y][x], 0, 20);
    ClearOverlays();
}

/* Arms the "please wait" panel: loads its sprite once and records the
 * rectangle it covers at (x,y). */
// FUNCTION: LEGOLAND 0x00466360
void SetWaitSpriteRect(int x, int y)
{
    short w;
    short h;

    if (!g_wait_sprite) {
        g_wait_sprite = LoadSprite(g_lls_wait, 4);
        if (!g_wait_sprite)
            return;
    }
    g_wait_active = 1;
    w = g_wait_sprite->w;
    h = g_wait_sprite->h;
    g_wait_rect[0] = x;
    g_wait_rect[1] = y;
    g_wait_rect[2] = x + w;
    g_wait_rect[3] = y + h;
}

/* Disarms it again. */
// FUNCTION: LEGOLAND 0x004663c0
void ClearWaitSprite(void)
{
    if (g_wait_sprite) {
        KillSprite(g_wait_sprite);
        g_wait_sprite = 0;
    }
    g_wait_active = 0;
}

/* =========================================================================
 *  Option screen volume rows
 * ========================================================================= */
/* Three rows (speech, music, fx), three icons each, numbered in the +0x18
 * word in creation order: 1/4/7 = the Down<n> arrows (x=0x19c, the RIGHT of
 * the row), 2/5/8 = the Up<n> arrows (x=0x26, the LEFT) and 3/6/9 = the
 * VolMarker<n> sliders. The marker x runs 0x7c..0x16d for volume 0..100, so
 * the LEFT ("Up") arrow is the one that DECREASES the volume and the RIGHT
 * ("Down") arrow the one that increases it -- the sprite names read the other
 * way round, the handlers are named after the sprites, and the behaviour
 * below is what the original does.
 *
 * All three are press-and-hold: bit 0 of the button mask is the click (which
 * plays the tick), bit 2 is "still held", and while it is held the handler
 * spins its own loop redrawing the screen. The fx row additionally starts a
 * looping sample so the player can hear the level, and kills it on release.
 * They answer 2, not 1 -- the only handlers besides Free-play that do. */

/* The Up<n> arrows: one step DOWN per frame while held. */
// FUNCTION: LEGOLAND 0x0048f5f0
char VolUpInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed) {
        if (buttons & 1)
            PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        if ((buttons & 4) != 0) {
        void* loop;
        int   hit;

        if (p->u18.control == 8)
            loop = (void*)PlayInstanceOfSample(g_snd_click, 1, 1, 0);
        else
            loop = 0;
        while (g_mouse_buttons & 4) {
            if (GetIconAtPos(&g_gfx_point, &hit) == p) {
                switch (p->u18.control) {
                case 2:
                    if (g_vol_speech) {
                        g_vol_speech--;
                        g_vol_marker_speech->x = (short)VolumeToMarkerX(g_vol_speech);
                        if (!g_6687b4 && !sub_498cf0())
                            ShowHelpString(-2);
                        ShowHelpString(p->help_id);
                    }
                    break;
                case 5:
                    if (g_vol_music) {
                        g_vol_music--;
                        g_vol_marker_music->x = (short)VolumeToMarkerX(g_vol_music);
                    }
                    break;
                case 8:
                    if (g_vol_sfx) {
                        g_vol_sfx--;
                        g_vol_marker_fx->x = (short)VolumeToMarkerX(g_vol_sfx);
                    }
                    break;
                }
            }
            UpdateHelpBar();
            RenderScreen();
            sub_498b40();
        }
        if (loop)
            KillPlayableSample(loop);
        return 2;
        }
    }
    return 1;
}

/* The Down<n> arrows: one step UP per frame while held, capped at 100. */
// FUNCTION: LEGOLAND 0x0048f760
char VolDownInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed) {
        if (buttons & 1)
            PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        if ((buttons & 4) != 0) {
        void* loop;
        int   hit;

        if (p->u18.control == 7)
            loop = (void*)PlayInstanceOfSample(g_snd_click, 1, 1, 0);
        else
            loop = 0;
        while (g_mouse_buttons & 4) {
            if (GetIconAtPos(&g_gfx_point, &hit) == p) {
                switch (p->u18.control) {
                case 1:
                    if (g_vol_speech < 100) {
                        g_vol_speech++;
                        g_vol_marker_speech->x = (short)VolumeToMarkerX(g_vol_speech);
                        if (!g_6687b4 && !sub_498cf0())
                            ShowHelpString(-2);
                        ShowHelpString(p->help_id);
                    }
                    break;
                case 4:
                    if (g_vol_music < 100) {
                        g_vol_music++;
                        g_vol_marker_music->x = (short)VolumeToMarkerX(g_vol_music);
                    }
                    break;
                case 7:
                    if (g_vol_sfx < 100) {
                        g_vol_sfx++;
                        g_vol_marker_fx->x = (short)VolumeToMarkerX(g_vol_sfx);
                    }
                    break;
                }
            }
            UpdateHelpBar();
            RenderScreen();
            sub_498b40();
        }
        if (loop)
            KillPlayableSample(loop);
        return 2;
        }
    }
    return 1;
}

/* The VolMarker<n> sliders: drag the marker between x=0x7c and x=0x16d and
 * feed the position back through MarkerXToVolume. The switch here reads the
 * control number into an INT local first (`xor ecx,ecx / mov cx,[esi+18h]`),
 * which is why it lowers to a cmp chain (cmp 3 / 6 / 9) where the two arrow
 * handlers -- switching on the u16 field directly -- lower to sub/je chains. */
// FUNCTION: LEGOLAND 0x0048f8d0
char VolMarkerInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 4)) {
        void* loop;
        int   grab;

        if (p->u18.control == 9)
            loop = (void*)PlayInstanceOfSample(g_snd_click, 1, 1, 0);
        else
            loop = 0;
        grab = g_gfx_point.x - p->x;
        while (g_mouse_buttons & 4) {
            int vol;
            int ctrl;

            p->x = (short)(g_gfx_point.x - grab);
            if (p->x > 0x16d)
                p->x = 0x16d;
            if (p->x < 0x7c)
                p->x = 0x7c;
            vol = MarkerXToVolume(p->x);
            ctrl = p->u18.control;
            switch (ctrl) {
            case 3:
                g_vol_speech = vol;
                if (!g_6687b4 && !sub_498cf0())
                    ShowHelpString(-2);
                ShowHelpString(p->help_id);
                break;
            case 6:
                g_vol_music = vol;
                break;
            case 9:
                g_vol_sfx = vol;
                break;
            }
            UpdateHelpBar();
            RenderScreen();
            sub_498b40();
        }
        if (loop)
            KillPlayableSample(loop);
        return 2;
    }
    return 1;
}

/* The title screen's "dark" buttons (New when there is no profile): on the
 * blink's ON phase the icon draws its own sprite, on the OFF phase it draws
 * the darkened twin at +0x18 -- first copying the live sprite's animation
 * frame across so the two stay in step. */
// FUNCTION: LEGOLAND 0x0046e920
int RenderDarkTitleIcon(Icon* p)
{
    BlitCtx ctx;

    ctx.kind = 2;
    ctx.owner.p = p;
    ctx.owner.n = 0;
    if (p->sprite) {
        if (GetBlink()) {
            PrintSprite(p->sprite, p->x, p->y, 0, &ctx);
        } else {
            /* `alt` must be a LOCAL: the original reads +0x18 once into edi
             * (which is why edi is pushed in the entry prologue), where
             * re-spelling p->u18.alt at each use reloads it three times. */
            Sprite* alt = p->u18.alt;
            if (alt) {
                short* lls;
                int    frame;

                lls = GetLLSForSprite(p->sprite);
                if (lls)
                    frame = *lls;
                else
                    frame = 0;
                lls = GetLLSForSprite(alt);
                if (lls)
                    LLSSetFrame(lls, frame);
                PrintSprite(alt, p->x, p->y, 0, &ctx);
            }
        }
    }
    return 0;
}

/* The new-saved-game popup's OK button. A null icon means "the popup's own OK
 * icon" (the screen code calls it directly), and it re-lights itself before
 * looking at the button. With no name typed yet it does not save: it fills the
 * editor with the default name GetString(0x87) + the slot number and comes
 * back. With a name it writes the game out and closes the screen. */
// FUNCTION: LEGOLAND 0x0048e720
char SaveGameOkInput(Icon* p, int buttons, int a3, int a4)
{
    char buf[16];

    if (!p)
        p = g_np_icon_ok;
    ResetSavePopupIcons();
    SetIconSprite(p, g_fe_sprite_674);
    if (buttons & 2) {
        if (!g_temp_name[0]) {
            sprintf(buf, g_fmt_save_default, GetString(0x87),
                    g_cur_save_slot_wide & 0xff);
            strcpy(g_temp_name, buf);
            g_temp_name_len = (unsigned char)strlen(buf);
            RestoreSavedGameIconHandlers();
            return 1;
        }
        ResetFrontEnd();
        g_6687b0 = 4;
        StoreNewSaveGameToDisk();
        g_cur_screen = -1;
        g_newsave_popup_up = 0;
        g_frontend_checkbox_closed = 1;
        RestoreSavedGameIconHandlers();
    }
    return 1;
}

/* Accept on the saved-game screen while the name popup is up (checkbox not
 * closed): the click really belongs to the popup's OK button, so it is handed
 * straight to SaveGameOkInput with a NULL icon and its answer returned unless
 * that closed the screen (g_cur_screen == -1), in which case this handler
 * finishes the teardown. With no popup up it saves directly. Both routes join
 * at the same teardown tail, which is why the `goto` is spelled into the
 * second block. */
// FUNCTION: LEGOLAND 0x0048da50
char SaveScreenAcceptInput(Icon* p, int buttons, int a3, int a4)
{
    if (!g_frontend_checkbox_closed) {
        char rc = SaveGameOkInput(0, buttons, a3, a4);
        if (g_cur_screen != -1)
            return rc;
        g_icons2_mode = 0;
        RemoveIconGroup(7);
        goto finish;
    }
    if ((buttons & 2) && g_cur_save_slot) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        ResetFrontEnd();
        g_6687b0 = 4;
        StoreNewSaveGameToDisk();
        g_icons2_mode = 0;
        RemoveIconGroup(7);
finish:
        KillTitleScreenSprites();
        g_game_mode = 3;
        SetInGameIconHandlers();
        ResumeGameTimer();
        g_frontend_checkbox_closed = 1;
    }
    return 1;
}

/* The ADVISOR panel's redraw hook (bigscreens.c calls it
 * RenderScriptEndIcon). While a script is running the icon just draws its own
 * sprite; otherwise it plays the advisor AVI: when the frame counter runs past
 * the clip's length it fires the clip's end callback, restarts (or starts the
 * queued clip), then pulls the next frame with AVIStreamGetFrame and blits it.
 * Finally, if the mouse is inside the blitted frame's rectangle -- the DIB
 * header's own width/height, not the icon's -- the icon registers itself as
 * the hit by copying its 12-byte blit context over g_hit_info. The four
 * strings written to 0x00667c40 on the way are the original's debug trail
 * ("SetVidAnim", "AVI GetFrame", "BltAdvisor", "Exit Advisor"). */
// FUNCTION: LEGOLAND 0x00443e30
int RenderAdvisorIcon(Icon* p)
{
    BlitCtx ctx;

    ctx.kind = 2;
    ctx.owner.p = p;
    ctx.owner.n = 0;
    if (!ScriptRunning()) {
        if (g_vidanim) {
            DibHeader* dib;

            if (g_vid_frame >= g_vidanim->frames) {
                if (g_vidanim->on_end)
                    g_vidanim->on_end();
                if (!g_vid_next)
                    g_vid_next = g_vidanim;
                g_dbg_where = g_dbg_setvidanim;
                SetVidAnim(g_vid_next);
                g_vid_next = 0;
            }
            g_dbg_where = g_dbg_getframe;
            dib = AVIStreamGetFrame(g_vidanim->pgf, g_vid_frame);
            g_dbg_where = g_dbg_blt;
            PushRenderingStatusAndLockVideoSurface();
            BltAdvisor(dib, p->x, p->y);
            PopRenderingStatus();
            g_vid_frame++;
            g_dbg_where = g_dbg_exit;
            if (g_gfx_point.x >= p->x && g_gfx_point.y >= p->y &&
                g_gfx_point.x < p->x + dib->width &&
                g_gfx_point.y < p->y + dib->height)
                g_hit_info = ctx;
        }
    } else {
        PushRenderingStatusAndLockVideoSurface();
        PrintSprite(p->sprite, p->x, p->y, 0, &ctx);
        PopRenderingStatus();
    }
    return 0;
}
