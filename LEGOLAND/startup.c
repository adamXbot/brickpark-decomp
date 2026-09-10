/* LEGOLAND -- scope T: from the CRT entry to RunGame.  WinMain (0x00453d10,
 * not here: its SEH prologue defeats the gate's walker) reads the exe's
 * version string and calls GameMain with its four arguments; GameMain takes
 * the "LegolandGameMutex", parses the command-line switches and, when the
 * host GPU passes, runs InitSession: the log, the three resource volumes,
 * the strings, the GPU, the screen, the input system, the eight pointer
 * sprites, the ICM and the five menu elements, then RunGame (gamemain.c)
 * and the teardown in reverse.
 * VC6 SP3 /O2 /Gy /Gd. Types describe only the fields used here.
 * Verification and recovered mechanics: docs/lanes/scope-t.md.
 */
typedef struct MapHdr { char pad00[0x1e]; unsigned short in_game; char pad20[0x20]; int no_intro; } MapHdr;
typedef struct SecAttr { unsigned long length; void* descriptor; int inherit; } SecAttr;   /* SECURITY_ATTRIBUTES */
typedef struct Sprite Sprite;

extern MapHdr*      g_map;                     /* 0x004bcbf4 */
extern unsigned int g_save_time;               /* 0x00669204  last-save stamp */
extern void*        g_hinstance;               /* 0x00669208 */
extern int          g_ncmdshow;                /* 0x0066920c  (first named here) */
extern int          g_windowed;                /* 0x00667d6c */
typedef int       (*PresentFn)(void);
extern PresentFn  g_present;                  /* 0x004b9ca4  the page flip in use */
extern int          g_music_sys;               /* 0x004bf774  set here to "music enabled" (1 unless -nomusic); musicthread.c's engine instance later */
extern const char*  g_volume_names[3];         /* 0x004bcba4  Legoland.res Graphics2.res Graphics1.res (first named here) */
extern void*        g_res_volumes[3];          /* 0x007fd640  (first named here) */
extern Sprite*      g_pointer_table[9];        /* 0x007fe9c0  [0] none, then the eight pointer sprites */

extern int   GetGameTimer(void);                                    /* 0x00499430 */
extern void  DBPrintf(const char* fmt, ...);                        /* 0x00453a20 */
extern void  DebugPrintf(const char* fmt, ...);                     /* 0x0047f870 */
extern int   FlipPrimary(void);                                     /* 0x004661d0 */
extern int   CheckHostSystemGPU(void);                              /* 0x004637c0 */
extern int   RES_EnsureMounted(int volume);                         /* 0x004515e0 */
extern void* RES_OpenVolume(const char* name);                      /* 0x00489750 */
#ifndef LEGOLAND_PORTABLE
extern void  RES_CloseVolume(void* vol);                            /* 0x00489dc0 */
#else
extern int RES_CloseVolume(void* vol);                            /* 0x00489dc0 */
#endif
extern void  LoadStrings(void);                                     /* 0x00498d00  (first named here: paired with DeleteStrings) */
extern void  DeleteStrings(void);                                   /* 0x00498ff0 */
extern char* GetString(int id);                                     /* 0x00498f50 */
#ifndef LEGOLAND_PORTABLE
extern void  InitHostSystemGPU(void);                               /* 0x00463700 */
#else
extern int InitHostSystemGPU(void);                               /* 0x00463700 */
#endif
extern void  KillHostSystemGPU(void);                               /* 0x004637e0 */
extern int   InitScreen(void);                                      /* 0x00463870 */
extern int   InitInputSystem(void);                                 /* 0x00473870 */
extern void  KillInputSystem(void);                                 /* 0x00473ae0 */
extern Sprite* LoadSprite(const char* name, int mode);              /* 0x00497ab0 */
#ifndef LEGOLAND_PORTABLE
extern void  KillSprite(Sprite* s);                                 /* 0x00497bd0 */
#else
extern int KillSprite(Sprite* s);                                 /* 0x00497bd0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void  LLIDB_LoadICM(void);                                   /* 0x0047aff0 */
#else
extern int LLIDB_LoadICM(void);                                   /* 0x0047aff0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void  LLIDB_CloseICM(void);                                  /* 0x0047be00 */
#else
extern int LLIDB_CloseICM(void);                                  /* 0x0047be00 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void  LLIDB_RegisterNewElement(const char* name, int a, int b); /* 0x0047b610 */
#else
extern int LLIDB_RegisterNewElement(const char* name, int a, int b); /* 0x0047b610 */
#endif
extern void  RunGame(void);                                         /* 0x00459520  gamemain.c */
extern int   sprintf(char* buf, const char* fmt, ...);              /* 0x0049e573 */
extern void* malloc(unsigned int bytes);                            /* 0x0049e4ff */
extern void  free(void* p);                                         /* 0x0049e4d0 */
extern char* strstr(const char* s, const char* sub);                /* 0x004a0580  CRT */
extern char* _strupr(char* s);                                      /* 0x004a0600  CRT */
extern unsigned int strlen(const char* s);
extern char* strcpy(char* dst, const char* src);
#pragma intrinsic(strlen, strcpy)
__declspec(dllimport) void* __stdcall CreateMutexA(SecAttr* sa, int owner, const char* name);   /* [0x4ab10c] */
__declspec(dllimport) unsigned long __stdcall WaitForSingleObject(void* h, unsigned long ms);   /* [0x4ab110] */
__declspec(dllimport) int   __stdcall CloseHandle(void* h);                                     /* [0x4ab260] */
__declspec(dllimport) void* __stdcall GetDesktopWindow(void);                                   /* [0x4ab2d8] */
__declspec(dllimport) int   __stdcall MessageBoxA(void* hwnd, const char* text, const char* caption, unsigned int type);   /* [0x4ab2a4] */

// FUNCTION: LEGOLAND 0x0047f820
int TimeSinceSave(void)
{
    return GetGameTimer() - g_save_time;
}

/* The debug log was compiled out of the shipped build: open and close
 * succeed without doing anything, and the print (DebugPrintf, sysstubs.c)
 * is a no-op. */
// FUNCTION: LEGOLAND 0x0047f830
int OpenDebugLog(const char* name)
{
    return 1;
}

// FUNCTION: LEGOLAND 0x0047f840
int CloseDebugLog(void)
{
    return 1;
}

/* Dead: nothing names it (tools/inventory.py). */
// FUNCTION: LEGOLAND 0x0047f860
void DebugNoop(void)
{
}

/* Finds `name` in the command line, case-insensitively: both are copied and
 * upper-cased, strstr'd, and the hit is mapped back into the caller's
 * string. */
// FUNCTION: LEGOLAND 0x0047fc40
char* FindCommandSwitch(const char* cmdline, const char* name)
{
    char* p;
    char* q;
    char* r;

    p = malloc(strlen(cmdline) + 1);
    if (!p)
        return p;
    strcpy(p, cmdline);
    _strupr(p);
    q = malloc(strlen(name) + 1);
    if (!q) {
        free(p);
        return 0;
    }
    strcpy(q, name);
    _strupr(q);
    r = strstr(p, q);
    if (r)
        r += cmdline - p;
    free(p);
    free(q);
    return r;
}

// FUNCTION: LEGOLAND 0x0047f880
int InitSession(void)
{
    char   msg[0x400];
    int    i;
    void** p;

    g_map->in_game = 1;
    if (!OpenDebugLog("legoland.log"))
        return 1;
    if (!RES_EnsureMounted(1))
        return 1;
    for (i = 0; i < sizeof(g_volume_names) / sizeof(g_volume_names[0]); i++) {
        g_res_volumes[i] = RES_OpenVolume(g_volume_names[i]);
        if (!g_res_volumes[i]) {
            sprintf(msg, "Failed to open resource %s", g_volume_names[i]);
            MessageBoxA(GetDesktopWindow(), msg, "LEGOLAND Error", 0x30);
            if (i > 0) {
                p = g_res_volumes;
                do {
                    RES_CloseVolume(*p);
                    p++;
                } while (--i);
            }
            return 1;
        }
    }
    LoadStrings();
    InitHostSystemGPU();
    if (!InitScreen()) {
        MessageBoxA(GetDesktopWindow(), GetString(0xcc), GetString(0xcb), 0x30);
        KillHostSystemGPU();
        for (p = g_res_volumes; p < g_res_volumes + 3; p++)
            RES_CloseVolume(*p);
        return 1;
    }
    if (!InitInputSystem()) {
        MessageBoxA(GetDesktopWindow(), GetString(0x9c4), GetString(0xcb), 0x30);
        KillInputSystem();
        KillHostSystemGPU();
        for (p = g_res_volumes; p < g_res_volumes + 3; p++)
            RES_CloseVolume(*p);
        return 1;
    }
    g_pointer_table[0] = 0;
    g_pointer_table[1] = LoadSprite("erase it.lls", 0);
    g_pointer_table[2] = LoadSprite("erase it2.lls", 0);
    g_pointer_table[3] = LoadSprite("no build.lls", 0);
    g_pointer_table[4] = LoadSprite("yes build.lls", 0);
    g_pointer_table[5] = LoadSprite("rab over icon.lls", 0);
    g_pointer_table[6] = LoadSprite("rab over icon2.lls", 0);
    g_pointer_table[7] = LoadSprite("question it.lls", 0);
    g_pointer_table[8] = LoadSprite("question it2.lls", 0);
    LLIDB_LoadICM();
    LLIDB_RegisterNewElement("BUILD MENU", 0, 0x200);
    LLIDB_RegisterNewElement("ATTRACTIONS MENU", 0, 0x200);
    LLIDB_RegisterNewElement("FOOD STORES MENU", 0, 0x200);
    LLIDB_RegisterNewElement("SCENERY MENU", 0, 0x200);
    LLIDB_RegisterNewElement("SHOPS MENU", 0, 0x200);
    RunGame();
    KillHostSystemGPU();
    for (p = g_res_volumes; p < g_res_volumes + 3; p++)
        if (*p)
            RES_CloseVolume(*p);
    DebugPrintf("Finished shutting stuff down");
    CloseDebugLog();
    DeleteStrings();
    LLIDB_CloseICM();
    if (g_pointer_table[3]) { KillSprite(g_pointer_table[3]); g_pointer_table[3] = 0; }
    if (g_pointer_table[4]) { KillSprite(g_pointer_table[4]); g_pointer_table[4] = 0; }
    if (g_pointer_table[7]) { KillSprite(g_pointer_table[7]); g_pointer_table[7] = 0; }
    if (g_pointer_table[8]) { KillSprite(g_pointer_table[8]); g_pointer_table[8] = 0; }
    if (g_pointer_table[1]) { KillSprite(g_pointer_table[1]); g_pointer_table[1] = 0; }
    if (g_pointer_table[2]) { KillSprite(g_pointer_table[2]); g_pointer_table[2] = 0; }
    if (g_pointer_table[5]) { KillSprite(g_pointer_table[5]); g_pointer_table[5] = 0; }
    if (g_pointer_table[6]) { KillSprite(g_pointer_table[6]); g_pointer_table[6] = 0; }
    return 0;
}

/* WinMain's body: one instance at a time, the switches, the session. */
// FUNCTION: LEGOLAND 0x0047fd10
int GameMain(void* hinst, void* hprev, char* cmdline, int ncmdshow)
{
    SecAttr sa;
    void*   mutex;
    int     r;

    sa.length = sizeof(SecAttr);
    sa.descriptor = 0;
    sa.inherit = 0;
    mutex = CreateMutexA(&sa, 1, "LegolandGameMutex");
    if (!mutex) {
        DBPrintf("Couldn't create Mutex\n");
        return 0;
    }
    if (WaitForSingleObject(mutex, 0) == 0x102) {
        DBPrintf("Program already running.\n");
        CloseHandle(mutex);
        return 0;
    }
    if (FindCommandSwitch(cmdline, "WINDEBUG")) {
        g_present = FlipPrimary;
        g_windowed = 1;
    } else if (FindCommandSwitch(cmdline, "BLT")) {
        g_present = FlipPrimary;
    }
    if (FindCommandSwitch(cmdline, "-nointro"))
        g_map->no_intro = 1;
    else
        g_map->no_intro = 0;
    g_music_sys = FindCommandSwitch(cmdline, "-nomusic") == 0;
    g_hinstance = hinst;
    g_ncmdshow = ncmdshow;
    r = CheckHostSystemGPU();
    if (!r)
        return r;
    return InitSession();
}
