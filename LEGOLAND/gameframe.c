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
    char           pad04[0x10]; /* +0x04 */
    unsigned short cells_w;     /* +0x14  map size in cells */
    unsigned short cells_h;     /* +0x16 */
    unsigned short pad18;       /* +0x18 */
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
extern struct ObjDef* g_sel_def;         /* 0x00667c58  class under the cursor */
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
#ifndef LEGOLAND_PORTABLE
extern void  SetPointer(int shape);                                    /* 0x00463850 */
#else
extern int SetPointer(int shape);                                    /* 0x00463850 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void  UpDateCurrentProfile(void);                               /* 0x00491680 */
#else
extern int UpDateCurrentProfile(void);                               /* 0x00491680 */
#endif
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
#ifndef LEGOLAND_PORTABLE
extern void  SetInfoPanelText(const char* a, const char* b);           /* 0x004911c0 */
#else
extern int SetInfoPanelText(const char* a, const char* b);           /* 0x004911c0 */
#endif
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
/* The definition's real return type, for the portable build only: a stale
 * extern name whose signature disagrees with its body makes gen_link.py
 * bridge the two with a CAST, the cast call lowers to `call_indirect`, and
 * binaryen's directize pass turns a constant-index call_indirect into an
 * invalid DIRECT call -- the module then fails validation hundreds of
 * functions away (scope PORT-M2 section 4). 0x0046cb20 is gameframe2.c:166 `int UnloadParkHelp(void)`. The VC6 arm is the
 * shipped spelling and its bytes cannot move: cdecl discards EAX here. */
#ifndef LEGOLAND_PORTABLE
extern void  sub_46cb20(void);                                         /* 0x0046cb20 */
#else
extern int   sub_46cb20(void);                                         /* 0x0046cb20 */
#endif
extern void  sub_483090(void);                                         /* 0x00483090 */
extern void  sub_49cfc0(void);                                         /* 0x0049cfc0  frees the object lists */
extern void  UnLoadInGameIcons(void);                                         /* 0x00474ed0 */

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
#ifndef LEGOLAND_PORTABLE
extern void  UpdateSoundVols(void);                                    /* 0x00495a90 */
#else
extern int UpdateSoundVols(void);                                    /* 0x00495a90 */
#endif
extern void  InitScreens(char screen);                                 /* 0x00458640  declared int elsewhere; this caller pushes a byte */
extern void  ReadGameButtons(void);                                    /* 0x00452460 */
#ifndef LEGOLAND_PORTABLE
extern void  PlayMovie(const char* name, int a, int b);                /* 0x004771f0 */
#else
extern int PlayMovie(const char* name, int a, int b);                /* 0x004771f0 */
#endif
extern void  UpdateHelpBar(void);                                      /* 0x0046d110 */
extern void  MapScreenFrame(void);                                     /* 0x00459360 (scope Q) */
/* The definition's real return type, for the portable build only: a stale
 * extern name whose signature disagrees with its body makes gen_link.py
 * bridge the two with a CAST, the cast call lowers to `call_indirect`, and
 * binaryen's directize pass turns a constant-index call_indirect into an
 * invalid DIRECT call -- the module then fails validation hundreds of
 * functions away (scope PORT-M2 section 4). 0x00498b40 is narration2.c:622 `int PumpNarration(void)`. The VC6 arm is the
 * shipped spelling and its bytes cannot move: cdecl discards EAX here. */
#ifndef LEGOLAND_PORTABLE
extern void  sub_498b40(void);                                         /* 0x00498b40 */
#else
extern int   sub_498b40(void);                                         /* 0x00498b40 */
#endif
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
typedef struct BPos  { unsigned char x, y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;
/* g_hit_cell: a dword global whose low word is the packed hit cell. */
typedef union  HitCell { int i; BPosW sq; } HitCell;

/* A placed object: its class at +0x0c. */
typedef struct ObjDef ObjDef;
typedef struct MapObj { void* f0; void* f4; void* f8; ObjDef* def; } MapObj;

/* A class record (the object definition): flags at +0x1c, the footprint
 * origin offsets at +0x3c/+0x40, the name at +0x78, two callback slots and
 * the placed instance at +0xc4 (whose first dword ShowObjectHelp takes). */
struct ObjDef {
    char           pad00[0x1c];
    unsigned int   flags;      /* +0x1c  0x2000000 drag-placeable, 0x200000 clears for a path */
    char           pad20[0x1c];
    int            ox;         /* +0x3c */
    int            oy;         /* +0x40 */
    char           pad44[0x34];
    char*          name;       /* +0x78 */
    char           pad7c[0x14];
    void         (*place)(MapObj* inst, Pos* pt, int a);   /* +0x90 */
    void         (*update2)(MapObj* inst, Pos* pos);       /* +0x94 */
    char           pad98[0x2c];
    MapObj*        inst;       /* +0xc4 */
};

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
extern void  UpdateIconPage(void);                                         /* 0x0046ee00 */
extern void  sub_46cff0(void);                                         /* 0x0046cff0 */
extern int   sub_482cb0(void* value);                                    /* 0x00482cb0 */
extern void  sub_455fc0(HelpRect* r, char* text, int font, int a);     /* 0x00455fc0  HTBubbleHelp with an extra arg */
extern void  sub_450a40(void* value);                                    /* 0x00450a40 */
extern void  sub_4632b0(void);                                         /* 0x004632b0 */
/* The definition's real return type, for the portable build only: a stale
 * extern name whose signature disagrees with its body makes gen_link.py
 * bridge the two with a CAST, the cast call lowers to `call_indirect`, and
 * binaryen's directize pass turns a constant-index call_indirect into an
 * invalid DIRECT call -- the module then fails validation hundreds of
 * functions away (scope PORT-M2 section 4). 0x0044db90 is goalstate.c:234 `int AppraisalDueTick(void)`. The VC6 arm is the
 * shipped spelling and its bytes cannot move: cdecl discards EAX here. */
#ifndef LEGOLAND_PORTABLE
extern void  sub_44db90(void);                                         /* 0x0044db90  the appraisal-due tick */
#else
extern int   sub_44db90(void);                                         /* 0x0044db90  the appraisal-due tick */
#endif
void HandleMapClick(void);                                             /* 0x00457a70 below */

extern int          g_drag_lock;          /* 0x00668954  a worker is on the mouse */
extern int          g_hit_type;           /* 0x004bdd00 */
extern void*        g_icon_value;         /* 0x004bdd04 */
extern HitCell      g_hit_cell;           /* 0x004bdd08 */
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
    UnLoadInGameIcons();
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
        void* value;
        int   cell;

        hit = g_hit_type;
        value = g_icon_value;
        cell = g_hit_cell.i;
        PrintSprite(g_ci_interface_bg, 0, 0, 0, ctx);
        sub_46f100(0x2c3);
        UpdateIconPage();
        g_dbg_where = g_str_in_game_help;
        ProcessInGameHelp();
        DrawPopUpInfo();
        RenderIcons2(0x2c3, 0, 0);
        sub_46cff0();
        if (g_ui_flags & 0x1000) {
            g_hit_type = hit;
            g_icon_value = value;
            g_hit_cell.i = cell;
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
                        ShowObjectHelp(g_sel_def->inst->f0);
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
                    sub_455fc0(&rect, GetVisitorName(g_icon_value), 2, sub_482cb0(g_icon_value));
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
                    ShowObjectHelp(g_sel_def->inst->f0);
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

/* ---- HandleMapClick's types, globals and callees ------------------------- */
typedef struct Rect  { int left, top, right, bottom; struct Rect* next; } Rect;

/* A map cell (legoland.h's Cell, 20 bytes): the placed object, the packed
 * cell position, the displayed tile and the map flags. */
typedef struct Cell {
    MapObj*        obj;      /* +0x00 */
    BPosW          sq;       /* +0x04 */
    unsigned short pad06;    /* +0x06 */
    unsigned short tile;     /* +0x08 */
    unsigned short base;     /* +0x0a */
    unsigned short flags;    /* +0x0c  0x888: has an object / a work order; 0x800 work order */
    unsigned short uflags;   /* +0x0e */
    char           pad10[4]; /* +0x10 */
} Cell;

/* An edit / destroy cursor block (objmap2.c's Cursor, 0x1834 bytes). */
typedef struct Cursor {
    unsigned short count;        /* +0x0000 */
    short          px[0x400];    /* +0x0002 */
    short          py[0x400];    /* +0x0802 */
    unsigned char  kind[0x400];  /* +0x1002 */
    unsigned char  pad1402[2];
    Pos            origin;       /* +0x1404 map cell the footprint hangs off */
    int            status;       /* +0x140c */
    int            error;        /* +0x1410 */
    Rect           rect;         /* +0x1414 footprint rect list */
    unsigned char  style;        /* +0x1428 */
    char           pad1429[0x1828 - 0x1429];
    unsigned int   flags;        /* +0x1828 */
    int            pad182c;
    struct Cursor* next;         /* +0x1830 chained cursor */
} Cursor;

/* The game-input block at 0x00813a40 (bighelp.c's GameInput, with the drag
 * rectangle at +0x44..+0x50 and the footprint size at +0x2c/+0x30 named). */
typedef struct GameButton { int mask; int state; } GameButton;
typedef struct GameInput {
    unsigned int flags;      /* +0x00  0x400 in-game clicks enabled, 0x800 env class, 0x1000 drag */
    Pos          point;      /* +0x04  cursor point (screen) */
    GameButton   mouse_a;    /* +0x0c */
    GameButton   mouse_b;    /* +0x14 */
    GameButton   mouse_c;    /* +0x1c */
    Pos          map;        /* +0x24  map ref under the cursor */
    int          fp_w;       /* +0x2c  footprint step, in refs */
    int          fp_h;       /* +0x30 */
    int          f34;        /* +0x34 */
    int          f38;        /* +0x38 */
    int          click_x;    /* +0x3c */
    int          click_y;    /* +0x40 */
    int          drag_x0;    /* +0x44  drag rectangle, in refs */
    int          drag_y0;    /* +0x48 */
    int          drag_x1;    /* +0x4c */
    int          drag_y1;    /* +0x50 */
    int          prev_buttons; /* +0x54 */
    GameButton   up;         /* +0x58 */
    GameButton   right;      /* +0x60 */
    GameButton   down;       /* +0x68 */
    GameButton   left;       /* +0x70 */
    GameButton   tab;        /* +0x78 */
    GameButton   btn0;       /* +0x80  state at 0x813ac4: bit0 pressed, bit1 released, bit4 latch */
    GameButton   btn1;       /* +0x88  state at 0x813acc */
} GameInput;

/* A work order (workorder.c): its target object and the cell it is biased by. */
typedef struct WorkOrder {
    struct WorkOrder* next;   /* +0x00 */
    MapObj*           obj;    /* +0x04 */
    Pos               pos;    /* +0x08 */
} WorkOrder;

typedef struct RoadRec { char pad00[0x14]; unsigned char flags; } RoadRec;   /* +0x14 bit 4 = crossing */

/* The tile-info table at 0x00801f40 (stride 8): a range record whose flag
 * array is indexed by tile minus its base. */
typedef struct TileRange { int base; char pad04[8]; unsigned int* flags; } TileRange;
typedef struct TileInfo  { TileRange* range; int pad04; } TileInfo;

/* The 12-byte pop-up identity passed BY VALUE to PopUpInfoSetUp. */
typedef struct PopUpKey { int type; void* obj; int ref; } PopUpKey;

/* The edit state at 0x008119b0 as one aggregate: {EditMode, GameMode,
 * EditObject}. HandleMapClick's leading test reads the mode through it, and
 * that read is a different object to the compiler from g_edit_mode, so the
 * off-map compare below stays a compare on memory as in the original. */
typedef struct EditState { int mode; int game_mode; ObjDef* object; } EditState;
extern EditState   g_edit;              /* 0x008119b0 */
#ifdef LEGOLAND_PORTABLE
/* ...and that is exactly the overlap hazard for a build that is not matching:
 * g_edit.mode and g_edit_mode (declared above, 0x008119b0) are ONE word, but
 * two extern objects to clang, and HandleMapClick both reads the word through
 * g_edit and writes it through g_edit_mode. PORT-M6's -O2 probe shows clang
 * folding the second load of such a pair into the first across the store
 * (`ret 0` where the answer is 1), so the portable build spells the word once.
 * The VC6 arm keeps the lever: the leading test must stay a compare on memory.
 * The #undef is needed because g_edit_mode is declared as an extern above. */
#define g_edit_mode (g_edit.mode)       /* 0x008119b0 */
#endif
/* The mouse-hit record at 0x004bdd00 as ONE object -- {type, obj, cell} --
 * which is how this function must see it: PopUpInfoSetUp takes it by value
 * and the copy's first field comes from the cached type register. */
typedef struct HitInfo { int type; void* obj; HitCell cell; } HitInfo;
extern HitInfo     g_hit_info;          /* 0x004bdd00 */
extern GameInput   g_input;             /* 0x00813a40 */
extern MapHdr*     g_level_map;         /* 0x004bcbf4  the same object as g_map, as the loops see it */
extern int         g_drag_step_x;       /* 0x00813a38  footprint width, in refs */
extern int         g_drag_step_y;       /* 0x00813a3c  footprint height, in refs */
extern Cell**      g_map_rows;          /* 0x00801400 */
extern TileInfo    g_tile_info[];       /* 0x00801f40 */
extern BPosW       g_sel_bpos;          /* 0x00667c54  the selected object's base cell */
#ifndef LEGOLAND_PORTABLE
extern int         g_sel_bpos_wide;     /* 0x00667c54  the same word read as a dword */
#else
/* The dword spelling of the same word: a VC6 codegen lever (the original reads
 * the cell as a dword here and as a word everywhere else), and for any other
 * compiler an overlap hazard -- two extern objects over one address, written
 * through `g_sel_bpos.b` and read through `g_sel_bpos_wide`. One object in the
 * portable build; the low byte is the same byte either way. */
#define g_sel_bpos_wide (*(unsigned short*)&g_sel_bpos)   /* 0x00667c54 */
#endif
extern ObjDef*     g_drag_class;        /* 0x0080ff6c  class being placed by a drag */
extern ObjDef*     g_env_class;         /* 0x007fd624  the environment class */
extern ObjDef*     g_hedge_def;         /* 0x0081cd08 */
extern ObjDef*     g_edit_object;       /* 0x008119b8  the class the cursor edits */
extern Cursor      g_query_cursor;      /* 0x00810160 */
extern Cursor      g_edit_cursor;       /* 0x007febc0 */
extern int         g_query_extra;       /* 0x00810144 */
extern int         g_667c5c;            /* 0x00667c5c */
extern int         g_render_order_dirty;/* 0x00667cdc */
extern void*       g_place_sample;      /* 0x004b9248 */
extern unsigned int g_map_dirty;        /* 0x00668610 */
extern int         g_path_gfx_batch;    /* 0x0066b46c */
extern const char g_str_driving_school_roads[]; /* 0x004b89cc "DRIVING SCHOOL ROADS" */
extern const char g_str_zebra_crossing[];       /* 0x004b89bc "ZEBRA CROSSING" */

extern void*      ElemID(const char* name);                              /* 0x0047b3f0 */
extern WorkOrder* GetGardenerWorkOrderAt(int x, int y);                  /* 0x0049b130 */
extern WorkOrder* GetMechanicWorkOrderAt(int x, int y);                  /* 0x0049b180 */
extern RoadRec*   GetRoadRecord(int x, int y);                           /* 0x004125f0 */
extern void       SetCursorError(Cursor* c, int error);                  /* 0x0045f480 */
extern void       UpdateMapDrag(void);                                   /* 0x00452030 */
extern void       BuildCursorPtr(Cursor* c, int a, int b);               /* 0x0045f5f0 */
extern void       RenderCursor(Cursor* c);                               /* 0x0045ff00 */
extern int        CursorIsValid(Cursor* c);                              /* 0x0045f4b0 */
extern void       RemObjFromMap(ObjDef* d, MapObj* o, BPosW sq, Cursor* c); /* 0x00459c90 */
#ifndef LEGOLAND_PORTABLE
extern void       PlayInstanceOfSample(void* sample, int a, int b, void* src); /* 0x00496d20 */
#else
extern int PlayInstanceOfSample(void* sample, int a, int b, void* src); /* 0x00496d20 */
#endif
extern void       CalculateMapRenderOrder(void);                         /* 0x0045a4a0 */
extern void       ClearObjectUserFlags(MapObj* obj, Pos* pos);           /* 0x0045e850 */
extern void       RemoveObjectPathTiles(ObjDef* def, Pos* pos);          /* 0x0045d3d0 */
extern void       EraseMechanicOrder(WorkOrder* o);                      /* 0x0049b1d0 */
extern void       EraseGardenerOrder(WorkOrder* o);                      /* 0x0049b230 */
extern int        IsBuildableClass(ObjDef* def);                         /* 0x0045ead0 */
extern void       ClearCellForPath(Pos* pos);                            /* 0x004779d0 */
extern int        WorkOrderBuildObject(void* inst, Pos* pos);            /* 0x0049ab30 */
extern void       PlayAppropriateBuildEffect(ObjDef* d, Pos* pos);       /* 0x00462d10 */
#ifndef LEGOLAND_PORTABLE
extern void       PopUpInfoSetUp(HitInfo key, int x, int y);             /* 0x00471950 */
#else
/* The record-by-value spelling is the caller-side copy the original emits
 * (`sub esp,0xc` plus three stores, not three pushes), and the stack image is
 * identical to the five scalars fpui2.c's definition reads. On wasm32 a
 * by-value struct travels as a POINTER to a copy, so the 12-byte record makes
 * this a 3-parameter function while the definition is 4 parameters, and the
 * link resolves the call to a trapping stub. Hand over the five dwords the
 * callee actually reads: type, obj, the ref dword (the packed {u8 x, u8 y}
 * map cell) and the point. */
extern void       PopUpInfoSetUp(int type, void* obj, int ref, Pos pos); /* 0x00471950 */
static void ll_popup_info_setup(const HitInfo* key, int x, int y)
{
    Pos p;
    p.x = x;
    p.y = y;
    PopUpInfoSetUp(key->type, key->obj, key->cell.i, p);
}
#define PopUpInfoSetUp(_key, _x, _y) ll_popup_info_setup(&(_key), (_x), (_y))
#endif
#ifndef LEGOLAND_PORTABLE
extern int        sub_457970(int x, int y);                              /* 0x00457970  footprint clearance test (group 15) */
#else
/* PORT-M7: 0x00457970 is bubblecache.c's FootprintClearanceTest, and it takes
 * the cell as a `Pos` BY VALUE, not as two ints. This is the one row of the 72
 * whose two spellings are not a different ARITY but a different ABI: on x86
 * `(int x, int y)` and `(Pos p)` push the same two dwords, so the original's
 * two prototypes were literally the same call and nothing was ever wrong; on
 * wasm32 an 8-byte aggregate is passed INDIRECTLY, so the stale prototype's
 * (i32, i32) and the body's (i32 pointer) are incompatible and gen_link.py had
 * to bridge them with a cast that handed a by-value x where a pointer was
 * expected. Call it the way the body is written. Same shim shape as
 * ll_popup_info_setup above. */
extern int        FootprintClearanceTest(Pos p);                         /* 0x00457970 */
static int ll_footprint_clearance_test(int x, int y)
{
    Pos p;
    p.x = x;
    p.y = y;
    return FootprintClearanceTest(p);
}
#define sub_457970(_x, _y) ll_footprint_clearance_test((_x), (_y))
#endif
extern void       sub_475f40(void);                                      /* 0x00475f40 */
/* The definition's real return type, for the portable build only: a stale
 * extern name whose signature disagrees with its body makes gen_link.py
 * bridge the two with a CAST, the cast call lowers to `call_indirect`, and
 * binaryen's directize pass turns a constant-index call_indirect into an
 * invalid DIRECT call -- the module then fails validation hundreds of
 * functions away (scope PORT-M2 section 4). 0x00473640 is popupmisc.c:238 `int ShowCursorErrorMessage(int)`. The VC6 arm is the
 * shipped spelling and its bytes cannot move: cdecl discards EAX here. */
#ifndef LEGOLAND_PORTABLE
extern void       sub_473640(int error);                                 /* 0x00473640 */
#else
extern int        sub_473640(int error);                                 /* 0x00473640 */
#endif

/* The click-on-map action handler, called from InGameFrame on a button or a
 * drag. First the hit under the cursor is classified (an object, a work
 * order, a road crossing, a tile) into g_hit_type / g_icon_value /
 * g_hit_cell / g_sel_def; then the edit mode decides what the click does:
 * mode 0 = query (show the object's cursor), mode 1 = place the edit class
 * (single or a drag-filled rectangle), mode 2 = the destroy/query cursor
 * (single or a drag-filled removal). Finally a released button opens the
 * pop-up info for the hit. */
// FUNCTION: LEGOLAND 0x00457a70
void HandleMapClick(void)
{
    Pos   pos;
    void* roads;
    void* crossing;

    roads = ElemID(g_str_driving_school_roads);
    crossing = ElemID(g_str_zebra_crossing);
    if (g_edit.mode == 0 || g_edit.mode == 2) {
        g_sel_def = 0;
        if (g_hit_info.type == 0x100 || g_hit_info.type == 0x103) {
            Cell* cell;

            if (g_input.map.x >= 0 && g_input.map.x < g_map->cells_w
                    && g_input.map.y >= 0 && g_input.map.y < g_map->cells_h
                    && (cell = &g_map_rows[g_input.map.y][g_input.map.x]) != 0) {
                if (cell->flags & 0x888) {
                    if (cell->flags & 0x800) {
                        WorkOrder* o = GetGardenerWorkOrderAt(g_input.map.x, g_input.map.y);

                        g_hit_info.obj = o;
                        if (o) {
                            g_hit_info.type = 0x10b;
                        } else {
                            o = GetMechanicWorkOrderAt(g_input.map.x, g_input.map.y);
                            g_hit_info.obj = o;
                            if (o)
                                g_hit_info.type = 0x10c;
                        }
                        {
                            unsigned short sq = cell->sq.w;
                            MapObj* obj;

                            g_hit_info.cell.sq.w = sq;
                            obj = cell->obj;
                            if (obj) {
                                ObjDef* d = obj->def;

                                g_sel_bpos.w = sq;
                                g_sel_def = d;
                            }
                        }
                    } else {
                        g_hit_info.obj = cell->obj;
                        g_hit_info.cell.sq.w = cell->sq.w;
                        if (cell->flags & 0x88) {
                            g_hit_info.type = 0x103;
                            if (cell->obj == roads) {
                                RoadRec* r = GetRoadRecord(cell->sq.b.x, cell->sq.b.y);

                                if (r && (r->flags & 0x10))
                                    g_hit_info.obj = crossing;
                            }
                        }
                        g_sel_def = ((MapObj*)g_hit_info.obj)->def;
                        g_sel_bpos.w = g_hit_info.cell.sq.w;
                    }
                } else if (g_hit_info.type == 0x103) {
                    g_sel_def = ((MapObj*)g_hit_info.obj)->def;
                    g_sel_bpos.w = g_hit_info.cell.sq.w;
                } else {
                    int t;
                    TileRange* tr;

                    t = cell->tile;
                    tr = g_tile_info[t].range;

                    if (tr && (tr->flags[t - tr->base] & 0x10))
                        g_hit_info.type = 0x10d;
                    else
                        g_hit_info.type = 0x109;
                }
            } else if (g_hit_info.type == 0x103 && g_edit_mode == 2) {
                g_sel_def = ((MapObj*)g_hit_info.obj)->def;
                g_sel_bpos.w = g_hit_info.cell.sq.w;
            } else {
                g_hit_info.type = 0x10a;
            }
        }
    }

    switch (g_edit_mode) {
    case 0: {
        unsigned int w = g_hit_info.cell.i & 0xffff;

        pos.x = w & 0xff;
        pos.y = w >> 8;
        if (g_hit_info.type == 0x103) {
            ObjDef* d = g_sel_def;
            d->update2(d->inst, &pos);
        } else if (g_hit_info.type == 0x10c) {
            ObjDef* d2 = g_sel_def;
            d2->update2(d2->inst, &pos);
        } else if (g_hit_info.type == 0x10b) {
            ObjDef* d3 = g_sel_def;
            d3->update2(d3->inst, &pos);
        } else {
            memset(&g_query_cursor.rect, 0, sizeof(Rect));
            g_query_cursor.flags = 8;
            SetCursorError(&g_query_cursor, 1);
            g_query_cursor.origin = g_input.map;    /* one Pos copy: VC6 moves y before x */
            g_667c5c = 0;
            g_input.flags &= ~0x400;
            if (!(g_input.btn0.state & 2))
                g_drag_class = 0;
            break;
        }
        g_query_cursor.next = 0;
        BuildCursorPtr(&g_query_cursor, 0, 0);
        RenderCursor(&g_query_cursor);
        break;
    }
    case 1:
        if (CursorIsValid(&g_edit_cursor))
            SetPointer(4);
        else
            SetPointer(3);
        if (g_edit_object == g_env_class || g_edit_object == g_hedge_def)
            g_input.flags |= 0x800;
        else
            g_input.flags &= ~0x800;
        if (!(g_input.flags & 0x400) && g_edit_object != 0)
            g_edit_object->place(g_edit_object->inst, &g_input.point, 0x8f8);
        BuildCursorPtr(&g_edit_cursor, 0x8f8, IsBuildableClass(g_edit_object));
        if (g_input.btn0.state & 0x11) {
            if (CursorIsValid(&g_edit_cursor)) {
                if (g_edit_object->flags & 0x2000000) {
                    int x, y;

                    g_map_loading = 1;
                    g_render_order_dirty = 0;
                    if (g_edit_object == g_env_class) {
                        for (y = g_input.drag_y0; y <= g_input.drag_y1; y += g_input.fp_h) {
                            for (x = g_input.drag_x0; x <= g_input.drag_x1; x += g_input.fp_w) {
                                pos.x = x - g_edit_object->ox;
                                pos.y = y - g_edit_object->oy;
                                if (pos.x >= 0 && pos.x < g_level_map->cells_w && pos.y >= 0 && pos.y < g_level_map->cells_h) {
                                    Cell* c = &g_map_rows[pos.y][pos.x];

                                    if (c && (c->flags & 0x8a0) && (c->obj->def->flags & 0x200000))
                                        ClearCellForPath(&pos);
                                }
                            }
                        }
                    }
                    for (y = g_input.drag_y0; y <= g_input.drag_y1; y += g_input.fp_h) {
                        for (x = g_input.drag_x0; x <= g_input.drag_x1; x += g_input.fp_w) {
                            pos.x = x - g_edit_object->ox;
                            pos.y = y - g_edit_object->oy;
                            if (sub_457970(pos.x, pos.y)) {
                                if (WorkOrderBuildObject(g_edit_object->inst, &pos))
                                    g_map_dirty |= 0x10;
                                g_path_gfx_batch = 1;
                            }
                        }
                    }
                    g_map_loading = 0;
                    if (g_render_order_dirty) {
                        PlayAppropriateBuildEffect(g_edit_object, 0);
                        PlayInstanceOfSample(g_place_sample, 0, 1, 0);
                        CalculateMapRenderOrder();
                    }
                    g_map_loading = 0;      /* the original clears it twice */
                } else {
                    if (WorkOrderBuildObject(g_edit_object->inst, &g_edit_cursor.origin)) {
                        sub_475f40();
                        g_map_dirty |= 2;
                    }
                }
            } else {
                sub_473640(g_edit_cursor.error);
            }
        } else {
            RenderCursor(&g_edit_cursor);
        }
        if (g_input.btn1.state & 2) {
            g_edit_mode = 0;
            g_input.flags &= ~0x1400;
        }
        break;
    case 2: {
        unsigned int w;

        g_query_extra = 0;
        w = g_hit_info.cell.i & 0xffff;
        pos.x = w & 0xff;
        pos.y = w >> 8;
        if (g_drag_class != 0 && (g_drag_class == g_env_class || g_drag_class == g_hedge_def))
            g_input.flags |= 0x800;
        else
            g_input.flags &= ~0x800;
        if (!(g_input.flags & 0x1000)) {
        if (g_hit_info.type == 0x103) {
                ObjDef* d = g_sel_def;
                d->update2(d->inst, &pos);
            } else if (g_hit_info.type == 0x10c) {
                ObjDef* d2 = g_sel_def;
                d2->update2(d2->inst, &pos);
            } else if (g_hit_info.type == 0x10b) {
                ObjDef* d3 = g_sel_def;
                d3->update2(d3->inst, &pos);
            } else {
                memset(&g_query_cursor.rect, 0, sizeof(Rect));
                g_query_cursor.flags = 8;
                SetCursorError(&g_query_cursor, 1);
                g_query_cursor.origin.x = g_input.map.x;
                g_query_cursor.origin.y = g_input.map.y;
                g_667c5c = 0;
                g_input.flags &= ~0x400;
                if (!(g_input.btn0.state & 2))
                    g_drag_class = 0;
            }
        } else {
            UpdateMapDrag();
            g_query_cursor = g_edit_cursor;
        }
        if (g_query_extra == 0)
            g_query_cursor.next = 0;
        BuildCursorPtr(&g_query_cursor, 0, 0);
        RenderCursor(&g_query_cursor);
        if (CursorIsValid(&g_query_cursor))
            SetPointer(2);
        else
            SetPointer(1);
        if (g_input.btn0.state & 0x11) {
            if (g_drag_class != 0 && (g_drag_class->flags & 0x2000000)) {
                int x, y;

                g_map_loading = 1;
                g_render_order_dirty = 0;
                for (y = g_input.drag_y0; y <= g_input.drag_y1; y += g_drag_step_y) {
                    for (x = g_input.drag_x0; x <= g_input.drag_x1; x += g_drag_step_x) {
                        g_query_cursor.rect.next = 0;
                        g_query_cursor.origin.x = x;
                        g_query_cursor.origin.y = y;
                        if (x >= 0 && x < g_level_map->cells_w && y >= 0 && y < g_level_map->cells_h) {
                            Cell* c = &g_map_rows[y][x];

                            if (c) {
                                int bx = c->sq.b.x;
                                int by;

                                g_query_cursor.origin.x = bx;
                                g_sel_bpos.b.x = (unsigned char)bx;
                                by = c->sq.b.y;
                                g_query_cursor.origin.y = by;
                                g_sel_bpos.b.y = (unsigned char)by;
                                if ((c->flags & 0x88) && !(c->flags & 0x40)
                                        && c->obj == g_drag_class->inst)
                                    RemObjFromMap(g_drag_class, g_drag_class->inst, g_sel_bpos, &g_query_cursor);
                            }
                        }
                    }
                }
                if (g_render_order_dirty) {
                    PlayInstanceOfSample(g_place_sample, 0, 1, 0);
                    CalculateMapRenderOrder();
                }
                g_map_loading = 0;
            } else if (CursorIsValid(&g_query_cursor)) {
                if (g_hit_info.type == 0x10c) {
                    ClearObjectUserFlags(((WorkOrder*)g_hit_info.obj)->obj, &((WorkOrder*)g_hit_info.obj)->pos);
                    RemoveObjectPathTiles(((WorkOrder*)g_hit_info.obj)->obj->def, &((WorkOrder*)g_hit_info.obj)->pos);
                    EraseMechanicOrder(g_hit_info.obj);
                } else if (g_hit_info.type == 0x10b) {
                    RemoveObjectPathTiles(((WorkOrder*)g_hit_info.obj)->obj->def, &((WorkOrder*)g_hit_info.obj)->pos);
                    EraseGardenerOrder(g_hit_info.obj);
                } else {
                    Cell* c;

                    pos.x = g_sel_bpos_wide & 0xff;
                    pos.y = g_sel_bpos.b.y;
                    if (pos.x >= 0 && pos.x < g_map->cells_w && pos.y >= 0 && pos.y < g_map->cells_h)
                        c = &g_map_rows[pos.y][pos.x];
                    else
                        c = 0;
                    g_sel_def = c->obj->def;        /* ORIGINAL BUG: c is NULL when the cell is off the map */
                    if (g_query_extra == 0)
                        RemoveObjectPathTiles(g_sel_def, &pos);
                    RemObjFromMap(g_sel_def, g_sel_def->inst, g_sel_bpos, &g_query_cursor);
                }
            }
        }
        if (!(g_input.flags & 0x1000) && g_hit_info.type == 0x103) {
            if (g_sel_def->flags & 0x2000000) {
                g_input.flags |= 0x400;
                g_drag_class = g_sel_def;
            } else {
                g_input.flags &= ~0x400;
                g_drag_class = 0;
            }
        }
        if (g_input.btn1.state & 2) {
            g_edit_mode = 0;
            g_input.flags &= ~0x1400;
        }
        break;
    }
    }

    if ((g_input.mouse_a.state & 2) && !g_icon_clicked && !g_edit_mode && !g_drag_lock) {
        Pos pt = g_input.point;     /* the Pos copy sets the rotation for the record copy */

        PopUpInfoSetUp(g_hit_info, pt.x, pt.y);
        g_icon_clicked = 1;
    }
}
