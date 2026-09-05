/* LEGOLAND -- scope P: the game frame.
 *
 * The per-frame dispatcher the main loop (0x00459520, scope Q) calls until it
 * returns 0, the in-game frame it dispatches to, the click-on-map action
 * handler that frame calls, and their helpers: front-end entry, level
 * completion, the title sprite, controller reset, park start-up, park
 * teardown before a load, and the exe version string WinMain reads.
 *
 * VC6 SP3 /O2 /Gy /Gd. Types describe only the fields used here; names are
 * ours (the integrator's provisional readings in docs/SCOPE_P_game_frame.md),
 * offsets and addresses are load-bearing. Verification and recovered
 * mechanics: docs/lanes/scope-p.md.
 */

/* ---- types --------------------------------------------------------------- */
typedef struct Sprite Sprite;

/* The map header (anim2.c's MapHdr / bigscreens.c's LevelMap): the screen
 * size in pixels at +0x00/+0x02, the visitor allowance at +0x1a, the level
 * number at +0x28. */
typedef struct MapHdr {
    unsigned short w;           /* +0x00 */
    unsigned short h;           /* +0x02 */
    char           pad04[0x16]; /* +0x04 */
    unsigned short bloke_count; /* +0x1a */
    char           pad1c[0x0c]; /* +0x1c */
    int            level;       /* +0x28 */
} MapHdr;

/* The controller record (bighelp.c / input.c), plus the three SPI_GETMOUSE
 * values ResetController stores after the button word. */
typedef struct Controller {
    int x0;        /* +0x00 */
    int y0;        /* +0x04 */
    int x;         /* +0x08 */
    int y;         /* +0x0c */
    int dx;        /* +0x10 */
    int dy;        /* +0x14 */
    int buttons;   /* +0x18 */
    int mouse0;    /* +0x1c  SPI_GETMOUSE threshold 1 */
    int mouse1;    /* +0x20  SPI_GETMOUSE threshold 2 */
    int mouse2;    /* +0x24  SPI_GETMOUSE acceleration */
} Controller;

typedef struct ClipRect { int left, top, right, bottom; } ClipRect;

/* The live profile as bigscreens.c lays it out; only level_done is used. */
#pragma pack(push, 1)
typedef struct CurProfile {
    char           name[0x20];      /* +0x00  0x0080ffa0 */
    int            f20, f24, f28, f2c, tail;
    unsigned char  level_done[15];  /* +0x34  0x0080ffd4 */
    unsigned char  profile_slot;    /* +0x43  0x0080ffe3 */
    unsigned char  save_slot;       /* +0x44  0x0080ffe4 */
    unsigned char  f45;             /* +0x45 */
    char           block[200];      /* +0x46 */
} CurProfile;
#pragma pack(pop)

/* ---- globals ------------------------------------------------------------- */
extern MapHdr*      g_map;               /* 0x004bcbf4 (lpConfig) */
extern Controller*  g_controller;        /* 0x00813b00  CONTROLLERBUFFER */
extern CurProfile   g_cur_profile;       /* 0x0080ffa0 */
extern ClipRect     g_fullscreen_clip;   /* 0x007fe020 */
extern int          g_game_mode;         /* 0x008119b4  0 quit, 1 map screen, 2 front end, 3 in game */
extern int          g_cur_screen;        /* 0x0080ff84 */
extern int          g_screen_mode;       /* 0x0080ff88 */
extern int          g_map_ready;         /* 0x00667c7c  a park is running */
extern int          g_sel_def;           /* 0x00667c58  class under the cursor */
extern int          g_castle_placed;     /* 0x0079a8d0 */
extern void*        g_freeplay_db;       /* 0x00667c4c  the loaded level database */
extern int          g_map_loading;       /* 0x00667cd8  suppress render-order rebuilds */
extern int          g_pending_state;     /* 0x00832ba0 */
extern unsigned int g_ui_flags;          /* 0x00813a40  0x400 in-game clicks enabled, 0x1000 drag */
extern char         g_script_text1[];    /* 0x0066861c */

/* ---- string literals (.data; only the addresses are load-bearing) -------- */
extern const char g_str_title_lls[];     /* 0x004b913c "TitleScreen1.lls" */
extern const char g_str_objlist_fmt[];   /* 0x004b9150 "objlist%d.txt" */
extern const char g_str_exe_name[];      /* 0x004b912c "Legoland.exe" */
extern const char g_str_version_key[];   /* 0x004b9104 "\StringFileInfo\080904B0\ProductVersion" */

/* ---- callees ------------------------------------------------------------- */
extern Sprite* LoadSprite(const char* name, int mode);                 /* 0x00497ab0 */
extern int   PrintSprite(void* s, int x, int y, int mode, void* ctx);  /* 0x004853a0 */
extern void  PushRenderingStatusAndLockVideoSurface(void);             /* 0x00463fc0 */
extern void  PopRenderingStatus(void);                                 /* 0x004641f0 */
extern int   RenderingComplete(void);                                  /* 0x00466500 */
extern int   KillSprite(void* sprite);                                 /* 0x00497bd0 */
extern void  SetPointer(int shape);                                    /* 0x00463850 */
extern void  UpDateCurrentProfile(void);                               /* 0x00491680 */
extern int   FreezeGameClock(void);                                    /* 0x00499380 */
extern void  ThawGameClock(void);                                      /* 0x004993c0 */
extern void  ResetGameClock(void);                                     /* 0x00499410 */
extern void  ResetSaveTimer(void);                                     /* 0x0047f810 */
extern void  ResetMapAI(void);                                         /* 0x00462dd0 */
extern void* LoadLevelDatabase(const char* name);                      /* 0x0047afb0 */
extern void  AllocBlokeCounters(int count);                            /* 0x00480e10 */
extern void  FreeBlokeCounters(void);                                  /* 0x00480e60 */
extern void  EnterParkPlayMode(void);                                  /* 0x00458940 */
extern void  UpdateMenu(void);                                         /* 0x004758c0 */
extern void  ShowInfoPanel(int kind);                                  /* 0x00490600 */
extern void  SetInfoPanelText(const char* a, const char* b);           /* 0x004911c0 */
extern void  ClearObjInfoList(void);                                   /* 0x00481170 */
extern void  RemoveObjectListIcons(int group);                         /* 0x0046fb40 */
extern void  DelObjectList(void);                                      /* 0x004756e0 */
extern void  ClearNewObjectMarkers(void);                              /* 0x004714e0 */
extern void  ResetInfoStruct(void);                                    /* 0x00471510 */
extern void  ClearBuildObjList(void);                                  /* 0x00450f10 */
extern int   sprintf(char* buf, const char* fmt, ...);                 /* 0x0049e573 */
extern void* malloc(unsigned int size);                                /* 0x0049e4ff */
extern void  free(void* p);                                            /* 0x0049e4d0 */
extern char* strcpy(char* dst, const char* src);
#pragma intrinsic(strcpy)

/* Unmatched callees, declared with the placeholder names the tree uses. */
extern void  sub_457870(int);                                          /* 0x00457870 (Codex-F) */
extern void  sub_489ee0(void);                                         /* 0x00489ee0 (Codex-F) */
extern void  sub_48a750(void);                                         /* 0x0048a750 */
extern void  sub_48a800(void);                                         /* 0x0048a800 */
extern void  sub_48a040(void);                                         /* 0x0048a040 */
extern void  sub_46cb20(void);                                         /* 0x0046cb20 */
extern void  sub_483090(void);                                         /* 0x00483090 */
extern void  sub_49cfc0(void);                                         /* 0x0049cfc0  frees the object lists */
extern void  sub_474ed0(void);                                         /* 0x00474ed0 */

/* Version-resource imports, declared WITHOUT dllimport: the original calls
 * them through the thunks at 0x0049e3a0..0x0049e3ac. */
extern unsigned long __stdcall GetFileVersionInfoSizeA(const char* name, unsigned long* handle); /* 0x0049e3ac */
extern int __stdcall GetFileVersionInfoA(const char* name, unsigned long handle, unsigned long len, void* data); /* 0x0049e3a6 */
extern int __stdcall VerQueryValueA(const void* block, const char* sub, void** out, unsigned int* len); /* 0x0049e3a0 */
extern __declspec(dllimport) int __stdcall SystemParametersInfoA(unsigned int action, unsigned int param, void* pv, unsigned int wini); /* IAT 0x004ab2b4 */

/* ========================================================================== */

/* Switch to the front-end screens: no screen selected yet, mode 0. */
// FUNCTION: LEGOLAND 0x00458bc0
void EnterFrontEnd(void)
{
    g_game_mode = 2;
    g_cur_screen = -1;
    g_screen_mode = 0;
}

/* The script-stop path (screens3.c declares this as sub_458be0): mark the
 * current level done in the live profile (levels are 0..14 in the byte
 * array) and write the profile back. */
// FUNCTION: LEGOLAND 0x00458be0
void CompleteLevelForProfile(void)
{
    if (g_map->level < 15)
        g_cur_profile.level_done[g_map->level] = 1;
    sub_48a750();
    UpDateCurrentProfile();
}

/* Full-screen clip, then the title sprite blitted once at the origin and
 * released. */
// FUNCTION: LEGOLAND 0x004588c0
void ShowTitleScreen(void)
{
    Sprite* s;

    g_fullscreen_clip.left = 0;
    g_fullscreen_clip.top = 0;
    g_fullscreen_clip.right = g_map->w;
    g_fullscreen_clip.bottom = g_map->h;
    s = LoadSprite(g_str_title_lls, 0);
    PushRenderingStatusAndLockVideoSurface();
    PrintSprite(s, 0, 0, 0, 0);
    PopRenderingStatus();
    RenderingComplete();
    if (s)
        KillSprite(s);
}

/* Zero the controller, centre the cursor, read the Windows mouse thresholds
 * and acceleration (SPI_GETMOUSE = 3) into it, and show pointer 5. */
// FUNCTION: LEGOLAND 0x004589a0
void ResetController(void)
{
    int mouse[3];

    g_controller->x0 = 0;
    g_controller->y0 = 0;
    g_controller->x = 0;
    g_controller->y = 0;
    g_controller->dx = 0;
    g_controller->dy = 0;
    g_controller->x = g_map->w >> 1;
    g_controller->y = g_map->h >> 1;
    SystemParametersInfoA(3, 0, mouse, 0);
    g_controller->mouse0 = mouse[0];
    g_controller->mouse1 = mouse[1];
    g_controller->mouse2 = mouse[2];
    SetPointer(5);
}

/* Park start-up (screens3.c declares this as sub_458a50): clocks reset, the
 * level's object list database loaded from objlist<level>.txt, the AI and
 * visitor counters set up, play mode entered, the menu and info panel shown. */
// FUNCTION: LEGOLAND 0x00458a50
void StartPark(void)
{
    char name[52];

    if (g_map_ready)
        return;
    g_sel_def = 0;
    g_castle_placed = 0;
    FreezeGameClock();
    ResetGameClock();
    ResetSaveTimer();
    sprintf(name, g_str_objlist_fmt, g_map->level);
    sub_457870(0);
    ResetMapAI();
    g_freeplay_db = LoadLevelDatabase(name);
    sub_457870(1);
    AllocBlokeCounters(g_map->bloke_count);
    EnterParkPlayMode();
    sub_489ee0();
    g_pending_state = 0;
    UpdateMenu();
    ShowInfoPanel(1);
    SetInfoPanelText(g_script_text1, 0);
    g_map_ready = 1;
    ThawGameClock();
    sub_48a800();
}

/* Tear the running park down before a load (screens3.c declares this as
 * sub_458b20): object lists, icons, counters and markers freed, the click
 * and drag bits cleared, the park marked not ready. */
// FUNCTION: LEGOLAND 0x00458b20
void BeginParkLoad(void)
{
    if (!g_map_ready)
        return;
    ClearObjInfoList();
    RemoveObjectListIcons(0xd2);
    DelObjectList();
    FreeBlokeCounters();
    sub_48a040();
    g_map_loading = 1;
    sub_46cb20();
    g_map_loading = 0;
    sub_457870(0);
    sub_489ee0();
    sub_483090();
    ClearNewObjectMarkers();
    ResetInfoStruct();
    ClearBuildObjList();
    sub_49cfc0();
    sub_474ed0();
    sub_457870(1);
    g_ui_flags &= ~0x1400;
    g_map_ready = 0;
}

/* Copy the exe's ProductVersion string into `out` (WinMain passes the
 * global at 0x0066752c). The size and the string length share one local. */
// FUNCTION: LEGOLAND 0x00458830
void ReadExeVersionString(char* out)
{
    unsigned int  len;
    unsigned long handle;
    char*         str;
    void*         buf;

    len = GetFileVersionInfoSizeA(g_str_exe_name, &handle);
    buf = malloc(len);
    GetFileVersionInfoA(g_str_exe_name, handle, len, buf);
    VerQueryValueA(buf, g_str_version_key, (void**)&str, &len);
    strcpy(out, str);
    free(buf);
}
