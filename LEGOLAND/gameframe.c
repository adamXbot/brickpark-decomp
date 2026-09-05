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
extern struct SelDef* g_sel_def;         /* 0x00667c58  class under the cursor */
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
extern void* memset(void* p, int c, unsigned int n);
#pragma intrinsic(memset)

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

/* GameFrame's callees. */
extern void  InitOptionSamples(void);                                  /* 0x00492830 */
extern void  ResumePausedSamples(void);                                /* 0x00492850 */
extern void  DeletePlayableSamples(void* def);                         /* 0x00492b90  0 = all */
extern void  InitMapScreen(void);                                      /* 0x00456300 */
extern void  KillMapScreen(void);                                      /* 0x00456370 */
extern void  DisableInfoPopUPIcons(void);                              /* 0x00471580 */
extern void  DisableSidePanelIcons(void);                              /* 0x00475e90 */
extern void  EnableSidePanelIcons(void);                               /* 0x00475ed0 */
extern void  SetInGameIconHandlers(void);                              /* 0x00474880 */
extern void  SetWaitSpriteRect(int x, int y);                          /* 0x00466360 */
extern void  ClearWaitSprite(void);                                    /* 0x004663c0 */
extern int   LoadGame(const char* path);                               /* 0x0047e980 */
extern void  InitGameInterface(int a);                                 /* 0x004749d0 */
extern void  KillHelpText(void);                                       /* 0x0046c5c0 */
extern void  UpdateSoundVols(void);                                    /* 0x00495a90 */
extern void  InitScreens(char screen);                                 /* 0x00458640  declared int elsewhere; this caller pushes a byte */
extern void  ReadGameButtons(void);                                    /* 0x00452460 */
extern void  PlayMovie(const char* name, int a, int b);                /* 0x004771f0 */
extern void  UpdateHelpBar(void);                                      /* 0x0046d110 */
extern void  MapScreenFrame(void);                                     /* 0x00459360 (scope Q) */
extern void  sub_498b40(void);                                         /* 0x00498b40 */
extern void  sub_4969d0(void);                                         /* 0x004969d0 */
void InGameFrame(void);                                                /* 0x00458ee0 below */

extern int          g_map_screen_pending; /* 0x008119bc  open the map screen this frame */
extern int          g_map_screen_leave;   /* 0x0080ff70  close it this frame */
extern int          g_park_start_pending; /* 0x00667c64  start a park this frame */
extern int          g_game_load_pending;  /* 0x00667c80  ... by loading a saved game */
extern int          g_ms_options_dirty;   /* 0x00667c78 */
extern int          g_sim_frame;          /* 0x008119a4 */
extern int          g_level_end_has_movie;/* 0x00832bac */
extern char         g_level_end_movie[];  /* 0x008100c0 */
extern int          g_icons2_mode;        /* 0x00668e38 */
extern const char*  g_dbg_where;          /* 0x00667c40 */
extern void*        g_icon_handler1;      /* 0x006687bc */
extern void*        g_icon_handler2;      /* 0x006687c0 */
extern int          g_cur_save_slot_wide; /* 0x0080ffe4  CurProfile+0x44 read as a dword */
extern const char g_str_sfx[];            /* 0x004b9160 "SFX" */
extern const char g_str_save_fmt[];       /* 0x004b9164 "%s\%dsave%d.sav" */
extern const char g_str_profiles[];       /* 0x004b9174 "profiles" */

/* InGameFrame's types, callees and globals. */
typedef struct Pos { int x, y; } Pos;
typedef struct BlitCtx { int a, b, c; } BlitCtx;          /* PrintSprite's 5th argument, {1, 0, 0} here */
typedef struct HelpRect { int left, top, right, bottom; } HelpRect;
/* The class record under the cursor: its name at +0x78 and, at +0xc4, a
 * pointer whose first dword is the object ShowObjectHelp describes. */
typedef struct SelDef {
    char   pad00[0x78];
    char*  name;        /* +0x78 */
    char   pad7c[0x48];
    void** obj;         /* +0xc4 */
} SelDef;

extern void  HandleRideAI(void);                                       /* 0x0048a0f0 */
extern void  DoMapAI(void);                                            /* 0x00462ef0 */
extern void  ControlPeople(void);                                      /* 0x00450990 */
extern void  ControlWorkers(void);                                     /* 0x0049a070 */
extern void  CheckWorkerOnMouseStatus(int a);                          /* 0x00470620 */
extern void  ProcessBuildingTimes(void);                               /* 0x00450c80 */
extern void  ProcessDamage(void);                                      /* 0x00463580 */
extern void  ResetHitInfo(void);                                       /* 0x00485ef0 */
extern void  RenderView(void);                                         /* 0x0045b180 */
extern void  ProcessInGameHelp(void);                                  /* 0x0046cf60 */
extern void  DrawPopUpInfo(void);                                      /* 0x004724a0 */
extern void  RenderIcons2(unsigned short g1, unsigned short g2, unsigned short g3); /* 0x0046f010 */
extern void  HTBubbleHelp(HelpRect* r, char* text, int font);          /* 0x004557c0 */
extern char* GetString(int id);                                        /* 0x00498f50 */
extern void  ShowIdHelp(int id);                                       /* 0x0046d230 */
extern void  ShowObjectHelp(void* obj);                                /* 0x0046d340 */
extern char* GetVisitorName(void* b);                                  /* 0x00482ba0 */
extern void  KillAdvisorHelp(void);                                    /* 0x0046ce20 */
extern void  UpdateFocussedIconPtr(void);                              /* 0x004700a0 */
extern char  CheckFocussedIcon(void);                                  /* 0x0046f4c0 */
extern void  RenderWorkerOnMouse(void);                                /* 0x004708c0 */
extern int   IsLShiftDown(void);                                       /* 0x00474070 */
extern int   IsRShiftDown(void);                                       /* 0x00474080 */
extern void  sub_46f100(int group);                                    /* 0x0046f100 */
extern void  sub_46ee00(void);                                         /* 0x0046ee00 */
extern void  sub_46cff0(void);                                         /* 0x0046cff0 */
extern int   sub_482cb0(int value);                                    /* 0x00482cb0 */
extern void  sub_455fc0(HelpRect* r, char* text, int font, int a);     /* 0x00455fc0  HTBubbleHelp with an extra arg */
extern void  sub_450a40(int value);                                    /* 0x00450a40 */
extern void  sub_4632b0(void);                                         /* 0x004632b0 */
extern void  sub_44db90(void);                                         /* 0x0044db90  the appraisal-due tick */
void HandleMapClick(void);                                             /* 0x00457a70 below */

extern int          g_drag_lock;          /* 0x00668954  a worker is on the mouse */
extern int          g_hit_type;           /* 0x004bdd00 */
extern int          g_icon_value;         /* 0x004bdd04 */
extern int          g_hit_cell;           /* 0x004bdd08 */
extern void*        g_ci_interface_bg;    /* 0x00668e68  InterfaceBG.lls */
extern int          g_edit_mode;          /* 0x008119b0 */
extern Pos          g_gfx_point;          /* 0x00813a44 */
extern int          g_mouse_ev2;          /* 0x00813ac4  bit 2 = held */
extern int          g_icon_clicked;       /* 0x00667c48 */
extern void*        g_focussed_icon;      /* 0x006687d0 */
extern int          g_show_capacity;      /* 0x00832994 */
extern const char g_str_ai[];             /* 0x004b91e4 "AI" */
extern const char g_str_process_stuff[];  /* 0x004b91d4 "ProcessStuff" */
extern const char g_str_zoning[];         /* 0x004b91cc "Zoning" */
extern const char g_str_rendering[];      /* 0x004b91c0 "Rendering" */
extern const char g_str_in_game_help[];   /* 0x004b91b0 "In Game Help" */
extern const char g_str_appraisals[];     /* 0x004b91a4 "Appraisals" */
extern const char g_str_appraisals_over[];/* 0x004b9194 "Appraisals Over" */
extern const char g_str_exiting[];        /* 0x004b9180 "Exiting GameProc" */

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

/* One frame of the game, whatever mode it is in. First the pending mode
 * changes (open/close the map screen, start a park -- from a saved game or
 * fresh), then the mode's own frame, then the game buttons and the level-end
 * state machine. Returns 0 only from mode 0, which ends the main loop. */
// FUNCTION: LEGOLAND 0x00458c00
int GameFrame(void)
{
    char path[256];

    sub_498b40();
    if (g_map_screen_pending != 0) {
        FreezeGameClock();
        InitOptionSamples();
        g_icon_handler1 = 0;
        g_icon_handler2 = 0;
        g_ui_flags &= ~0x20;
        InitMapScreen();
        DisableInfoPopUPIcons();
        DisableSidePanelIcons();
        g_map_screen_pending = 0;
    } else if (g_map_screen_leave != 0) {
        ResumePausedSamples();
        g_icon_handler1 = 0;
        g_icon_handler2 = 0;
        g_ui_flags |= 0x20;
        KillMapScreen();
        SetInGameIconHandlers();
        EnableSidePanelIcons();
        g_map_screen_leave = 0;
        ThawGameClock();
    } else if (g_park_start_pending != 0) {
        FreezeGameClock();
        ResetGameClock();
        ResetSaveTimer();
        g_icon_handler1 = 0;
        g_icon_handler2 = 0;
        g_castle_placed = 0;
        g_ms_options_dirty = 1;
        InitOptionSamples();
        if (g_game_load_pending != 0) {
            DeletePlayableSamples(0);
            sprintf(path, g_str_save_fmt, g_str_profiles, g_cur_profile.profile_slot,
                    g_cur_save_slot_wide & 0xff);
            SetWaitSpriteRect(0, 0);
            LoadGame(path);
            g_game_load_pending = 0;
            ClearWaitSprite();
            InitGameInterface(0);
            SetInGameIconHandlers();
        } else {
            KillHelpText();
            StartPark();
        }
        UpdateSoundVols();
        g_park_start_pending = 0;
        g_pending_state = 0;
        g_game_mode = 3;
        SetInGameIconHandlers();
        ThawGameClock();
    }

    switch (g_game_mode) {
    case 2:
        SetPointer(5);
        InitScreens(g_screen_mode);
        break;
    case 3:
        if (g_ms_options_dirty != 0) {
            ResumePausedSamples();
            g_ms_options_dirty = 0;
        }
        InGameFrame();
        break;
    case 1:
        MapScreenFrame();
        break;
    case 0:
        return 0;
    }

    if (g_park_start_pending == 0) {
        g_dbg_where = g_str_sfx;
        sub_4969d0();
        ReadGameButtons();
        g_sim_frame++;
        if (g_pending_state != 0 && g_game_mode == 3) {
            if (g_level_end_has_movie != 0) {
                SetPointer(0);
                PlayMovie(g_level_end_movie, 1, 1);
                g_level_end_has_movie = 0;
                SetPointer(5);
            }
            if (g_pending_state != 3) {
                if (g_pending_state == 1)
                    g_map->level++;
                sub_48a750();
                BeginParkLoad();
                g_icons2_mode = 1;
                g_game_mode = 2;
                g_cur_screen = -1;
                g_screen_mode = 6;
            } else {
                g_icons2_mode = 0;
                g_game_mode = 2;
                g_cur_screen = -1;
                g_screen_mode = 1;
                BeginParkLoad();
            }
        }
        UpdateHelpBar();
    }
    return 1;
}

/* The frame from the building timers to the bubble help, inlined into
 * InGameFrame after its guard: the callee-saved pushes belong here. */
static __inline void InGameFrameBody(BlitCtx* ctx)
{
    HelpRect rect;
    int      hit;

    g_dbg_where = g_str_process_stuff;
    ProcessBuildingTimes();
    ProcessDamage();
    hit = g_hit_type;
    g_dbg_where = g_str_zoning;
    SetPointer(5);
    ResetHitInfo();
    g_dbg_where = g_str_rendering;
    PushRenderingStatusAndLockVideoSurface();
    RenderView();
    if ((hit & 0x100) || (g_ui_flags & 0x1000))
        HandleMapClick();
    {
        int value, cell;

        hit = g_hit_type;
        value = g_icon_value;
        cell = g_hit_cell;
        PrintSprite(g_ci_interface_bg, 0, 0, 0, ctx);
        sub_46f100(0x2c3);
        sub_46ee00();
        g_dbg_where = g_str_in_game_help;
        ProcessInGameHelp();
        DrawPopUpInfo();
        RenderIcons2(0x2c3, 0, 0);
        sub_46cff0();
        if (g_ui_flags & 0x1000) {
            g_hit_type = hit;
            g_icon_value = value;
            g_hit_cell = cell;
        }
    }
    hit = g_hit_type;
    if (hit & 0x100) {
        if (g_edit_mode == 0 || g_edit_mode == 2) {
            int edit = g_edit_mode;
            rect.left = g_gfx_point.x;
            rect.top = g_gfx_point.y - 10;
            rect.right = g_gfx_point.x;
            rect.bottom = g_gfx_point.y;
            if (edit == 0) {
                switch (hit) {
                case 0x103:
                    SetPointer(8);
                    if (g_sel_def) {
                        HTBubbleHelp(&rect, g_sel_def->name, 2);
                        ShowObjectHelp(*g_sel_def->obj);
                    }
                    break;
                case 0x10a:
                    HTBubbleHelp(&rect, GetString(0xd4), 2);
                    ShowIdHelp(0xd4);
                    SetPointer(8);
                    break;
                case 0x10b:
                    HTBubbleHelp(&rect, GetString(0xd2), 2);
                    ShowIdHelp(0xd2);
                    SetPointer(7);
                    break;
                case 0x10c:
                    HTBubbleHelp(&rect, GetString(0xd3), 2);
                    ShowIdHelp(0xd3);
                    SetPointer(7);
                    break;
                case 0x10d:
                    HTBubbleHelp(&rect, GetString(0x7e4), 2);
                    ShowIdHelp(0x7e4);
                    SetPointer(8);
                    break;
                case 0x308:
                    HTBubbleHelp(&rect, GetString(0x92), 2);
                    ShowIdHelp(0x92);
                    SetPointer(8);
                    sub_450a40(g_icon_value);
                    break;
                case 0x307:
                    HTBubbleHelp(&rect, GetString(0x90), 2);
                    ShowIdHelp(0x90);
                    SetPointer(8);
                    sub_450a40(g_icon_value);
                    break;
                case 0x306:
                    sub_455fc0(&rect, GetVisitorName((void*)g_icon_value), 2, sub_482cb0(g_icon_value));
                    SetPointer(8);
                    sub_450a40(g_icon_value);
                    break;
                default:
                    SetPointer(7);
                    break;
                }
            } else {
                if (g_sel_def) {
                    HTBubbleHelp(&rect, g_sel_def->name, 2);
                    ShowObjectHelp(*g_sel_def->obj);
                }
            }
        }
    }
}

/* One frame of the running park, phase by phase in g_dbg_where: the AI, the
 * building/damage timers, the render (with the click handler on a button or
 * a drag), the interface overlay and popup, the bubble help for whatever the
 * mouse is over (by hit type, or by the selected class in edit mode 2), the
 * appraisal tick, the focussed icon, and the flip. The hit triple is saved
 * around the overlay render during a drag. */
// FUNCTION: LEGOLAND 0x00458ee0
void InGameFrame(void)
{
    BlitCtx ctx;

    ctx.a = 1;
    memset(&ctx.b, 0, 8);      /* a sub-object memset: its zero fill is not a constant-web use */
    g_dbg_where = g_str_ai;
    HandleRideAI();
    DoMapAI();
    ControlPeople();
    ControlWorkers();
    if (g_drag_lock)
        CheckWorkerOnMouseStatus(0);
    InGameFrameBody(&ctx);
    if (g_hit_type == 5 && (g_mouse_ev2 & 2)) {
        KillAdvisorHelp();
        g_icon_clicked = 1;
    }
    g_dbg_where = g_str_appraisals;
    sub_44db90();
    g_dbg_where = g_str_appraisals_over;
    UpdateFocussedIconPtr();
    if (!(g_ui_flags & 0x1000) && !g_icon_clicked) {
        if (g_focussed_icon && !g_drag_lock)
            SetPointer(6);
        CheckFocussedIcon();
    }
    if (g_drag_lock)
        RenderWorkerOnMouse();
    PopRenderingStatus();
    if (g_show_capacity) {
        if (IsLShiftDown() || IsRShiftDown())
            sub_4632b0();
    }
    RenderingComplete();
    g_dbg_where = g_str_exiting;
}
