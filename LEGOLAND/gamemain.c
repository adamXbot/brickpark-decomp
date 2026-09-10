/* LEGOLAND -- scope Q: the session -- WinMain's init routine calls RunGame,
 * which plays the intro, initialises the map and loops on GameFrame
 * (gameframe.c, scope P) until it returns 0.
 * VC6 SP3 /O2 /Gy /Gd. Types describe only the fields used here.
 * Verification and recovered mechanics: docs/lanes/scope-q.md.
 */
typedef struct CurProfile { char bytes[0x110]; } CurProfile;      /* layout in bigscreens.c */
typedef struct SimTuning { int a, b, rest[9]; } SimTuning;        /* 0x2c-byte records */

extern int         g_game_mode;                    /* 0x008119b4  2 = front-end screens */
extern CurProfile  g_cur_profile;                  /* 0x0080ffa0 */
extern int         g_vol_speech;                   /* 0x0080ffc4  CurProfile +0x24, slider 0..100 */
extern int         g_vol_music;                    /* 0x0080ffc8  CurProfile +0x28 */
extern int         g_vol_sfx;                      /* 0x0080ffcc  CurProfile +0x2c */
extern SimTuning   g_sim_tuning[6];                /* 0x00832824  (bigsim.c sees it as g_map_ai.cat[k]+0x14; movie3.c calls the reset ResetSimTuning) */
extern char        g_level_end_sequence1[0x100];   /* 0x00832998 */
extern char        g_level_end_sequence2[0x100];   /* 0x00832a98 */

extern void  KillCurrentScreen(void);              /* 0x004585c0 */
extern char* strncpy(char* dst, const char* src, unsigned int n);   /* 0x004a0110  CRT (it tests each byte for NUL: not memcpy) */
extern void* memset(void*, int, unsigned int);

typedef struct MapHdr { char pad00[0x14]; unsigned short w, h; } MapHdr;   /* +0x14/+0x16: map size in cells */
typedef struct Cell { char pad00[0xc]; unsigned short f0c; char pad0e[6]; } Cell;   /* 0x14-byte map cell */
typedef struct LLElem { char pad00[8]; unsigned int flags; } LLElem;
typedef struct Sprite Sprite;

extern void*      g_map_block;                     /* 0x00667c9c  the calloc'd map grid */
extern Sprite*    g_arrow2;                        /* 0x00667c88 */
extern Sprite*    g_arrow1;                        /* 0x00667c8c */
extern Sprite*    g_arrow4;                        /* 0x00667c90 */
extern Sprite*    g_arrow3;                        /* 0x00667c94 */
extern int        g_map_loaded;                    /* 0x00667d50 */
extern MapHdr*    g_map;                           /* 0x004bcbf4 */
extern Cell**     g_map_rows;                      /* 0x00801400 */
extern LLElem*    g_terrain_elem_2;                /* 0x00801404 */
extern LLElem*    g_tsm_mapping_elem;              /* 0x0080140c */
extern LLElem*    g_terrain_elem;                  /* 0x00801410 */
extern void**     g_array_A;                       /* 0x00801a68 */
extern void**     g_array_B;                       /* 0x00801a70  the level's loaded object-data pointers */
extern int        g_perim_aux;                     /* 0x00801a74  count of g_array_B */

extern void  HeapFree_w(void* p);                                            /* 0x0049e4d0 */
#ifndef LEGOLAND_PORTABLE
extern void  KillSprite(Sprite* s);                                          /* 0x00497bd0 */
#else
extern int KillSprite(Sprite* s);                                          /* 0x00497bd0 */
#endif
extern int   LLIDB_FindElement(const char* name, LLElem** out, unsigned int* idx); /* 0x0047b330 */
extern int   LLIDB_UnLoadData(LLElem* e);                                    /* 0x0047d450 */
extern int   LLIDB_GetCount(void);                                           /* 0x0047b2d0 */
#ifndef LEGOLAND_PORTABLE
extern void  LLIDB_GetElement(int i, LLElem** out);                          /* 0x0047b2e0 */
#else
extern int LLIDB_GetElement(int i, LLElem** out);                          /* 0x0047b2e0 */
#endif
extern int   LLIDB_FindElementFromDataPtr(void* data, LLElem** out, unsigned int* idx); /* 0x0047b410 */
extern void  ResetBuildStats(void);                                          /* 0x00459880 */
extern void  ClearOverlays(void);                                            /* 0x00462ce0 */
extern void  sub_4828f0(void);                                               /* 0x004828f0 */

typedef struct Pos { int x, y; } Pos;
typedef struct ClipRect { int left, top, right, bottom; } ClipRect;
typedef struct BlitCtx { int kind; struct { void* p; int n; } sub; } BlitCtx;   /* money.c's layout */
typedef struct Elem { char pad00[0xc]; void* def; } Elem;
typedef struct MSG { void* hwnd; unsigned int message, wParam; long lParam; unsigned long time; Pos pt; } MSG;

extern int        g_hit_type;                      /* 0x004bdd00  the mouse-hit record's kind */
extern Sprite*    g_ci_interface_bg;               /* 0x00668e68  InterfaceBG.lls */
extern unsigned char g_mouse_buttons;              /* 0x00813ac4  bit 0 left held, bit 1 right */
extern Pos        g_gfx_point;                     /* 0x00813a44  the mouse position */
extern int        g_game_mode_saved;               /* 0x00667c60  mode stashed by the map button */
extern int        g_map_click_time;                /* 0x00667c68  double-click detection on the map screen (first named here) */
extern int        g_map_click_x;                   /* 0x00667c70 */
extern int        g_map_click_y;                   /* 0x00667c74 */
extern int        g_map_screen_leave;                        /* 0x0080ff70 */
extern void*      g_hedge_def;                     /* 0x0081cd08  ElemID("HEDGE")->def */
extern volatile int g_music_disabled;              /* 0x007988bc  volatile as musicthread.c declares it: the load may not hoist above the cleanup */
extern int        g_detail;                        /* 0x008119a4 */
extern Sprite*    g_backdrop;                      /* 0x00810148 */

extern void  ResetHitInfo(void);                                             /* 0x00485ef0 */
extern void  PushRenderingStatusAndLockVideoSurface(void);                   /* 0x00463fc0 */
extern void  DrawMapScreen(void);                                            /* 0x004566f0 */
#ifndef LEGOLAND_PORTABLE
extern void  SetPointer(int shape);                                          /* 0x00463850 */
#else
extern int SetPointer(int shape);                                          /* 0x00463850 */
#endif
extern int   PrintSprite(Sprite* s, int x, int y, int mode, void* ctx);      /* 0x004853a0 */
extern void  UpdateIconPage(void);                                           /* 0x0046ee00  picks the icon page from the mode/edit object (first named here) */
extern void  RenderIcons(void);                                              /* 0x0046eee0 */
#ifndef LEGOLAND_PORTABLE
extern void  CheckFocussedIcon(void);                                        /* 0x0046f4c0 */
#else
extern int CheckFocussedIcon(void);                                        /* 0x0046f4c0 */
#endif
extern void  UpdateFocussedIconPtr(void);                                    /* 0x004700a0 */
extern void  PopRenderingStatus(void);                                       /* 0x004641f0 */
extern int   RenderingComplete(void);                                        /* 0x00466500 */
extern void  GetClipping(ClipRect* out);                                     /* 0x0048a630 */
extern void  SetClipping(ClipRect* r);                                       /* 0x0048a5c0 */
extern void  RenderMouseBounds(void);                                        /* 0x00456460 */
extern void  MapScreenSetScrollPos(Pos* p);                                  /* 0x004565b0 */
extern unsigned long GetTicks(void);                                         /* 0x00499450 */
extern int   abs(int);

extern Elem* ElemID(const char* name);                                       /* 0x0047b3f0 */
#ifndef LEGOLAND_PORTABLE
extern void  InitSoundSystem(void);                                          /* 0x004964f0 */
#else
extern int InitSoundSystem(void);                                          /* 0x004964f0 */
#endif
extern void  SetMusicGrooveLevel(int level);                                 /* 0x00495b90 */
extern void  SuspendMusicThread(void);                                       /* 0x00492c60  SuspendThread on the music thread (first named here) */
extern void  ResumeMusicThread(void);                                        /* 0x00492c80  ResumeThread (first named here) */
#ifndef LEGOLAND_PORTABLE
extern void  SetupControllers(void);                                         /* 0x00451e70 */
#else
extern int SetupControllers(void);                                         /* 0x00451e70 */
#endif
extern void  LLIDB_ClearOnLevel(void);                                       /* 0x0047b4c0 */
extern void  ResetController(void);                                          /* 0x004589a0  gameframe.c (scope P) */
extern int   ProcessSystemEvents(void);                                      /* 0x00480050 */
#ifndef LEGOLAND_PORTABLE
extern void  PlayMovie(const char* name, int a, int b);                      /* 0x004771f0 */
#else
extern int PlayMovie(const char* name, int a, int b);                      /* 0x004771f0 */
#endif
extern void  ShowTitleScreen(void);                                           /* 0x004588c0  gameframe.c (scope P) */
extern void  SetWaitSpriteRect(int a, int b);                                /* 0x00466360 */
extern void  progress_tick(void);                                            /* 0x004663f0 */
extern void  ClearWaitSprite(void);                                          /* 0x004663c0 */
extern void  LoadIconBarGFX(void);                                           /* 0x0046f890  GBarFrame.lls, IF_Side_*.lls (first named here) */
extern void  UnloadIconBarGFX(void);                                         /* 0x0046f920  (first named here) */
extern void  LoadWorkerInterfaceGFX(void);                                   /* 0x00499530 */
extern void  LoadBubbleHelpGFX(void);                                        /* 0x00454910 */
extern void  UnloadBubbleHelpGFX(void);                                      /* 0x00454a10  unloads "SPEECH BUBBLE" (first named here) */
extern void  InitialiseBlokes(void);                                         /* 0x004830f0 */
extern void  UnInitialiseBlokes(void);                                       /* 0x00482ec0  (first named here) */
extern void  InitGameMap(void);                                              /* 0x00459850 */
extern void  KillGameMap(void);                                              /* 0x00459870 */
extern void  SetThemeInTransition(int theme);                                /* 0x00492ca0 */
extern void  Load_Interface_ControlIcons(void);                              /* 0x004741f0 */
extern void  UnLoad_Interface_ControlIcons(void);                            /* 0x004743b0 */
extern void  Load_Interface_ThemeIcons(void);                                /* 0x004745c0 */
extern void  UnLoad_Interface_ThemeIcons(void);                              /* 0x00474670 */
extern void  FreeTileSpace(unsigned short base, unsigned short n);           /* 0x0045aa90 */
#ifndef LEGOLAND_PORTABLE
extern void  LoadMapTiles(void);                                             /* 0x0045aad0 */
#else
extern int LoadMapTiles(void);                                             /* 0x0045aad0 */
#endif
extern void  InitMan(void);                                                  /* 0x00440350 */
extern void  UnInitMan(void);                                                /* 0x004405a0 */
extern void  CreateObjectClasses(void);                                      /* 0x00480d80 */
extern void  EnterFrontEnd(void);                                            /* 0x00458bc0  gameframe.c (scope P) */
extern int   GameFrame(void);                                                /* 0x00458c00  gameframe.c (scope P) */
extern void  InitAdvisorMovies(void);                                        /* 0x00444090  AD_Blink/AD_LR/AD_Phone.avi (first named here) */
extern void  KillAdvisorMovies(void);                                        /* 0x00444150  (first named here) */
#ifndef LEGOLAND_PORTABLE
extern void  PauseCurrentTrack(void);                                        /* 0x00498920  (screens3.c calls it ResetFrontEnd) */
#else
extern int PauseCurrentTrack(void);                                        /* 0x00498920  (screens3.c calls it ResetFrontEnd) */
#endif
extern void  KillControllers(void);                                          /* 0x00451f40  frees g_controller (first named here) */
extern void  FreeBlokeCounters(void);                                        /* 0x00480e60 */
extern void  KillHelp(void);                                                 /* 0x0046d100 */
#ifndef LEGOLAND_PORTABLE
extern void  KillSoundSystem(void);                                          /* 0x00496520 */
#else
extern int KillSoundSystem(void);                                          /* 0x00496520 */
#endif
extern void  KillInputSystem(void);                                          /* 0x00473ae0 */
__declspec(dllimport) int   __stdcall PeekMessageA(MSG* m, void* hwnd, unsigned int lo, unsigned int hi, unsigned int flags); /* [0x4ab2bc] */
__declspec(dllimport) void  __stdcall Sleep(unsigned long ms);                                   /* [0x4ab114] */
__declspec(dllimport) void* __stdcall LoadLibraryA(const char* name);                           /* [0x4ab124] */
__declspec(dllimport) int   __stdcall FreeLibrary(void* h);                                     /* [0x4ab128] */

// FUNCTION: LEGOLAND 0x004594e0
void KillFrontEndScreenIfActive(void)
{
    switch (g_game_mode) {
    case 2:
        KillCurrentScreen();
        break;
    }
}

// FUNCTION: LEGOLAND 0x004594f0
void ResetCurProfileDefaults(void)
{
    memset(&g_cur_profile, 0, sizeof(CurProfile));
    g_vol_music = 100;
    g_vol_speech = 75;
    g_vol_sfx = 75;
}

// FUNCTION: LEGOLAND 0x00462e50
void SetSimTuningA(int i, int v)
{
    g_sim_tuning[i].a = v;
}

// FUNCTION: LEGOLAND 0x00462e70
void SetSimTuningB(int i, int v)
{
    g_sim_tuning[i].b = v;
}

// FUNCTION: LEGOLAND 0x00462e90
void ResetSimTuning(void)
{
    g_sim_tuning[0].a = 0x32;
    g_sim_tuning[0].b = 0x14;
    g_sim_tuning[1].a = 0x21;
    g_sim_tuning[1].b = 0x32;
    g_sim_tuning[2].a = 0;
    g_sim_tuning[2].b = 0;
    g_sim_tuning[3].a = 0;
    g_sim_tuning[3].b = 0;
    g_sim_tuning[4].a = 0x21;
    g_sim_tuning[4].b = 0x28;
    g_sim_tuning[5].a = 0x21;
    g_sim_tuning[5].b = 0x28;
}

// FUNCTION: LEGOLAND 0x004597e0
void SetLevelEndSequence(int which, const char* s)
{
    char* buf = which ? g_level_end_sequence1 : g_level_end_sequence2;

    if (s) {
        strncpy(buf, s, 0x100);
        buf[0xff] = 0;
    } else {
        buf[0] = 0;
    }
}

// FUNCTION: LEGOLAND 0x0045ac20
int UnloadSessionSprites(void)
{
    LLElem* e;

    if (g_map_block)
        HeapFree_w(g_map_block);
    LLIDB_FindElement("MAPPING 1", &e, 0);
    LLIDB_UnLoadData(e);
    if (g_arrow1) { KillSprite(g_arrow1); g_arrow1 = 0; }
    if (g_arrow2) { KillSprite(g_arrow2); g_arrow2 = 0; }
    if (g_arrow3) { KillSprite(g_arrow3); g_arrow3 = 0; }
    if (g_arrow4) { KillSprite(g_arrow4); g_arrow4 = 0; }
    return 1;
}

// FUNCTION: LEGOLAND 0x004629e0
int ResetLevelObjects(void)
{
    LLElem* d;
    LLElem* e;
    int x, y, i, n;

    if (g_map_loaded == 0)
        return 0;
    ResetBuildStats();
    for (y = 0; y < g_map->h; y++)
        for (x = 0; x < g_map->w; x++)
            g_map_rows[y][x].f0c = 0;
    LLIDB_UnLoadData(g_tsm_mapping_elem);
    LLIDB_UnLoadData(g_terrain_elem);
    if (g_terrain_elem_2) {
        LLIDB_UnLoadData(g_terrain_elem_2);
        g_terrain_elem_2 = 0;
    }
    n = LLIDB_GetCount();
    for (i = 0; i < n; i++) {
        LLIDB_GetElement(i, &e);
        if ((e->flags & 0x10) && (e->flags & 1))
            LLIDB_UnLoadData(e);
    }
    HeapFree_w(g_array_A);
    for (i = 0; i < g_perim_aux; i++) {
        LLIDB_FindElementFromDataPtr(g_array_B[i], &d, 0);
        d->flags &= ~0x3000e;
        LLIDB_UnLoadData(d);
    }
    HeapFree_w(g_array_B);
    ClearOverlays();
    sub_4828f0();
    g_map_loaded = 0;
    return 1;
}

#pragma intrinsic(abs)
// FUNCTION: LEGOLAND 0x00459360
void MapScreenFrame(void)
{
    BlitCtx ctx;
    int     now;

    ctx.kind = 1;
    memset(&ctx.sub, 0, sizeof(ctx.sub));   /* two plain zero stores web with the three zero
                                              * pushes below into a callee-saved esi (an extra push) */
    ResetHitInfo();
    PushRenderingStatusAndLockVideoSurface();
    DrawMapScreen();
    SetPointer(5);
    PrintSprite(g_ci_interface_bg, 0, 0, 0, &ctx);
    UpdateIconPage();
    RenderIcons();
    CheckFocussedIcon();
    if (g_hit_type == 2) {
        SetPointer(6);
        UpdateFocussedIconPtr();
        PopRenderingStatus();
        RenderingComplete();
        return;
    }
    if (g_hit_type & 0x100) {
        ClipRect clip = { 0, 0x20, 0x280, 0x174 };
        ClipRect saved;

        GetClipping(&saved);
        SetClipping(&clip);
        RenderMouseBounds();
        SetClipping(&saved);
        if (g_mouse_buttons & 1)
            MapScreenSetScrollPos(&g_gfx_point);
        if (g_mouse_buttons & 2) {
            now = GetTicks();
            if (now - g_map_click_time < 500 &&
                abs(g_gfx_point.x - g_map_click_x) < 5 &&
                abs(g_gfx_point.y - g_map_click_y) < 5) {
                g_map_screen_leave = 1;
                g_game_mode = g_game_mode_saved;
                g_game_mode_saved = 1;
            }
            g_map_click_x = g_gfx_point.x;
            g_map_click_y = g_gfx_point.y;
            g_map_click_time = now;
        }
    }
    UpdateFocussedIconPtr();
    PopRenderingStatus();
    RenderingComplete();
}

// FUNCTION: LEGOLAND 0x00459520
void RunGame(void)
{
    MSG   msg;
    void* lib;
    int   r;

    g_hedge_def = ElemID("HEDGE")->def;
    ResetCurProfileDefaults();
    InitSoundSystem();
    SetMusicGrooveLevel(1);
    SuspendMusicThread();
    SetupControllers();
    LLIDB_ClearOnLevel();
    ResetController();
    SetPointer(0);
    ProcessSystemEvents();
    PlayMovie("lmi.avi", 0, 1);
    ShowTitleScreen();
    ResumeMusicThread();
    SetWaitSpriteRect(0, 0);
    while (g_music_disabled == 0) {
        PeekMessageA(&msg, 0, 0, 0, 0);
        Sleep(100);
        progress_tick();
    }
    LoadIconBarGFX();
    LoadWorkerInterfaceGFX();
    LoadBubbleHelpGFX();
    InitialiseBlokes();
    InitGameMap();
    SetPointer(0);
    PlayMovie("Intro.avi", 1, 0);
    SetThemeInTransition(0);
    SetPointer(5);
    g_detail = 0;
    Load_Interface_ControlIcons();
    progress_tick();
    Load_Interface_ThemeIcons();
    FreeTileSpace(0, 0x800);
    g_game_mode = 3;
    LoadMapTiles();
    progress_tick();
    InitMan();
    progress_tick();
    CreateObjectClasses();
    progress_tick();
    EnterFrontEnd();
    progress_tick();
    lib = LoadLibraryA("Ir50_32.dll");
    progress_tick();
    InitAdvisorMovies();
    progress_tick();
    ClearWaitSprite();
    ResumeMusicThread();
    r = GameFrame();
    while (r)
        r = GameFrame();
    KillFrontEndScreenIfActive();
    PauseCurrentTrack();
    if (g_backdrop) {
        KillSprite(g_backdrop);
        g_backdrop = 0;
    }
    KillControllers();
    UnloadIconBarGFX();
    UnloadBubbleHelpGFX();
    UnInitialiseBlokes();
    KillGameMap();
    UnLoad_Interface_ControlIcons();
    UnLoad_Interface_ThemeIcons();
    UnloadSessionSprites();
    UnInitMan();
    KillAdvisorMovies();
    FreeLibrary(lib);
    FreeBlokeCounters();
    KillHelp();
    KillSoundSystem();
    KillInputSystem();
}
