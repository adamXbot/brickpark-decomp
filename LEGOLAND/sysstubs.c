#ifdef LEGOLAND_PORTABLE
#define InitMusicSystem InitMusicSystem_vc6_body
#endif
/* LEGOLAND -- small system, save, audio, script and math helpers.
 * VC6 SP3 /O2 /Gy /Gd. Import declarations describe the original x86 ABI. */
double sin(double x);
double cos(double x);
void* memset(void* dest, int value, unsigned size);
#pragma intrinsic(sin, cos, memset)
typedef struct Pos { int x, y; } Pos;
typedef struct Vec3 { float x, y, z; } Vec3;
typedef struct SpriteRec {
    struct SpriteRec* next;
    void* surface;
    void* image;
    int detail;
    unsigned int flags;
    unsigned char pad14[0xc];
} SpriteRec;
typedef struct ImageRec {
    void* pixels;
    void* palette;
    short width, height;
    unsigned short refs;
    unsigned char kind, pad0f;
    const char* name;
    int type;
} ImageRec;
typedef struct Sample {
    struct Sample* next;       /* +00 */
    int refcount, fade;         /* +04, +08 */
    int source_kind;            /* +0c */
    unsigned char source_rest[0xc];
    unsigned short flags, pad1e; /* +1c */
    unsigned int due;           /* +20 */
    void* callback;             /* +24 */
    struct Sample* def;         /* +28 */
    void* buffer;               /* +2c */
    void* data30;               /* +30 */
    void* data34;               /* +34 */
} Sample;
typedef struct ScriptEvent {
    struct ScriptEvent* next;
    int unused04;
    char* text;
    int kind;
    unsigned int flags;
    unsigned char pad14[0x24];
    int mode;
    unsigned char pad3c[8];
} ScriptEvent;
typedef struct Menu { unsigned char bytes[20]; } Menu;
typedef struct PanelState { unsigned char bytes[12]; unsigned char restore; unsigned char pad0d[3]; } PanelState;
typedef struct Icon { unsigned char pad00[0x34]; unsigned int flags; } Icon;
typedef struct FPTableEntry { int id; const char* name; int cost, chosen; } FPTableEntry;
typedef struct DSBuffer DSBuffer;
typedef struct DSBufferVtbl {
    unsigned char pad00[0x3c];
    long (__stdcall* SetVolume)(DSBuffer*, long);
} DSBufferVtbl;
struct DSBuffer { DSBufferVtbl* vtbl; };
typedef struct MouseDevice MouseDevice;
typedef struct MouseDeviceVtbl {
    unsigned char pad00[8];
    unsigned long (__stdcall* Release)(MouseDevice*);
    unsigned char pad0c[0x14];
    long (__stdcall* Unacquire)(MouseDevice*);
} MouseDeviceVtbl;
struct MouseDevice { MouseDeviceVtbl* vtbl; };
typedef unsigned char (*IconHandler)(void*, int, int, int);

extern __declspec(dllimport) unsigned long __stdcall GetTickCount(void); /* 0x004ab1f8 */
extern __declspec(dllimport) int __stdcall SetEvent(void* event); /* 0x004ab0f8 */
extern __declspec(dllimport) int __stdcall ShowWindow(void* window, int how); /* 0x004ab2c4 */
extern __declspec(dllimport) void* __stdcall CreateThread(void* security, unsigned stack_size, unsigned long (__stdcall* entry)(void*), void* arg, unsigned flags, unsigned long* id); /* 0x004ab0d8 */
extern unsigned long __stdcall MusicThread(void* arg);     /* 0x00492db0 */
extern unsigned char InGamePrimaryIcon(void*, int, int, int); /* 0x00474820 */
extern unsigned char InGameSecondaryIcon(void*, int, int, int); /* 0x00474830 */
extern Sample* NewSampleRecord(void);                     /* 0x004920e0 */
extern ScriptEvent* NewScriptEvent(int kind, int mode);    /* 0x00468910 */
extern void* MemAlloc(unsigned size);                     /* 0x0049e4ff */
extern void* calloc(unsigned count, unsigned size);       /* 0x004a020e */
extern void HeapFree_w(void* ptr);                        /* 0x0049e4d0 */
extern int vsprintf(char* dest, const char* format, char* args); /* 0x0049fdeb */
extern void DebugErrorSink(const char* text);              /* 0x00453cd0 */
extern void* WNDENV_GetWindow(void);                       /* 0x0047fe60 */
extern int TestMenu(Menu* menu);                          /* 0x00475710 */
extern int SaveGameWrite(const void* data, int size);     /* 0x0047d760 */
extern int SaveGameRead(void* data, int size);            /* 0x0047d730 */
extern int GetBrickCount(void);                          /* 0x004578e0 */
extern void UseBricks(int count);                        /* 0x004578c0 */
extern int ScriptRunning(void);                          /* 0x0046b280 */
extern void ShowScriptStepText(void* step, int mode);     /* 0x0046b6b0 */
extern void ResetScriptTimer(void);                       /* 0x00468d00 */
extern void ToggleHelpIcon(int on);                      /* 0x004748a0 */
extern void ClearMenuHelp(void);                         /* 0x00476000 */
extern void SetAdvisorPose(int pose, int arg);             /* 0x00444070 */
extern int StopSpeech(void);                             /* 0x004988c0 */
extern void SetScriptEventText(ScriptEvent* event, const char* text, int copy); /* 0x00468b40 */
extern void EnqueueScriptEvent(ScriptEvent* event);        /* 0x00468b00 */
extern ImageRec* CreateSourceImage(const char* name, int kind); /* 0x00497280 */
extern SpriteRec* CreateSprite(ImageRec* image);       /* 0x00497640 */
extern int FreePlayItemUpdate(const char* name, FPTableEntry** out); /* 0x0048a840 */
extern void* g_model_context;                            /* 0x00665e8c */
extern int g_map_ready;                                  /* 0x00667c7c */
extern IconHandler g_icon_handler1;                      /* 0x006687bc */
extern IconHandler g_icon_handler2;                      /* 0x006687c0 */
extern unsigned char g_left_shift;                       /* 0x007fddca */
extern unsigned char g_right_shift;                      /* 0x007fddd6 */
extern unsigned char g_script_bytes[10];                 /* 0x007fe930 */
extern Sample* g_playable_list;                          /* 0x007988cc */
extern void* g_music_event;                              /* 0x0079a6a0 */
extern int g_music_stop;                                 /* 0x0079a6a4 */
extern int g_clock_frozen;                               /* 0x0079a890 */
extern unsigned long g_clock_freeze_ticks;               /* 0x0079a894 */
extern unsigned int g_clock_frozen_frame;                 /* 0x0079a89c */
extern unsigned int g_clock_offset;                      /* 0x0079a8a0 */
extern unsigned int g_sim_frame;                         /* 0x008119a4 */
extern int g_zbuffer_pitch;                              /* 0x0066be4c */
extern unsigned char* g_zbuffer_pixels;                  /* 0x00701e5c */
extern SpriteRec* g_sprites_head;                        /* 0x0079a7c0 */
extern char g_error_text[];                              /* 0x00667128 */
extern Menu g_menus[];                                   /* 0x004bafa8 */
extern int g_menu_index;                                 /* 0x004baff8 */
extern int g_object_list_mode;                           /* 0x00668e34 */
extern PanelState g_panel_state;                         /* 0x007fdd80 */
extern DSBuffer* g_speech_buffer;                        /* 0x0079a848 */
extern long g_speech_volume;                             /* 0x0079a7d0 */
extern MouseDevice* g_mouse_device;                      /* 0x00668d90 */
extern int g_gardener_count;                             /* 0x0079a8bc */
extern int g_mechanic_count;                             /* 0x0079a8cc */
extern int g_brick_lock;                                 /* 0x004b90fc */
extern int g_bricks;                                     /* 0x004b90f8 */
extern void* g_script_cur;                               /* 0x0066879c */
extern int   g_music_sys; /* 0x004bf774  music ENABLED flag (startup.c:255 stores `-nomusic` == 0) */
extern unsigned long g_music_thread_id;                  /* 0x007cad48 */
extern void* g_music_thread;                             /* 0x0079a698 */
extern int g_music_disabled;                             /* 0x007988bc */
extern unsigned int g_advisor_help_flags;                /* 0x007fe040 */
extern char* g_advisor_help_text;                        /* 0x007fe048 */
extern char g_help_format_text[];                        /* 0x0066820c */
extern int g_zbuf_ready;                                 /* 0x00798590 */
extern const char g_zbuffer_name[];                      /* 0x004d8bb0 */
extern ImageRec* g_zbuffer_image;                        /* 0x0066be50 */
extern unsigned char g_zbuffer_storage[];                /* 0x00701e68 */
extern SpriteRec* g_zbuffer_sprite;                      /* 0x00701e64 */
extern int g_freeplay_progress;                          /* 0x007cb3a0 */
extern int g_freeplay_selected_count;                    /* 0x00798650 */
extern Icon* g_fp_accept_icon;                           /* 0x0079864c */

// FUNCTION: LEGOLAND 0x00499450
unsigned long GetTicks(void) { return GetTickCount(); }

// FUNCTION: LEGOLAND 0x0047f850
void DebugFlush(void) {}

// FUNCTION: LEGOLAND 0x0047f870
void DebugPrintf(const char* format, ...) {}

/* Empty hook invoked by script-state loading in the shipped build. */
// FUNCTION: LEGOLAND 0x004688e0
void ScriptState_NoOp(void) {}

// FUNCTION: LEGOLAND 0x00443710
void* GetModelContext(void) { return g_model_context; }

// FUNCTION: LEGOLAND 0x00443250
float Sin(float x) { return (float)sin(x); }

// FUNCTION: LEGOLAND 0x00443260
float Cos(float x) { return (float)cos(x); }

// FUNCTION: LEGOLAND 0x00458bb0
void SetMapReady(int ready) { g_map_ready = ready; }

// FUNCTION: LEGOLAND 0x00474880
void SetInGameIconHandlers(void)
{
    g_icon_handler1 = InGamePrimaryIcon;
    g_icon_handler2 = InGameSecondaryIcon;
}

// FUNCTION: LEGOLAND 0x00474070
int IsLShiftDown(void) { return g_left_shift >> 7; }

// FUNCTION: LEGOLAND 0x00474080
int IsRShiftDown(void) { return g_right_shift >> 7; }

// FUNCTION: LEGOLAND 0x00468840
void ClearScriptStateBytes(void) { memset(g_script_bytes, 0, 10); }

// FUNCTION: LEGOLAND 0x00492110
Sample* MakePlayable(void)
{
    Sample* sample = NewSampleRecord();
    sample->next = g_playable_list;
    g_playable_list = sample;
    return sample;
}

// FUNCTION: LEGOLAND 0x00492d80
void StopMusic(void)
{
    g_music_stop = 1;
    SetEvent(g_music_event);
}

// FUNCTION: LEGOLAND 0x0047fe70
void WNDENV_Minimise(void) { ShowWindow(WNDENV_GetWindow(), 6); }

// FUNCTION: LEGOLAND 0x0047fe80
void WNDENV_Restore(void) { ShowWindow(WNDENV_GetWindow(), 9); }

// FUNCTION: LEGOLAND 0x00499460
unsigned int GetSimClock(void)
{
    unsigned int time;
    if (g_clock_frozen) time = g_clock_frozen_frame;
    else time = g_sim_frame;
    return time - g_clock_offset;
}

// FUNCTION: LEGOLAND 0x00488820
unsigned int GetZBufferPixel(int x, int y)
{
    unsigned int* row = (unsigned int*)(g_zbuffer_pixels + g_zbuffer_pitch * y);
    return row[x] >> 24;
}

// FUNCTION: LEGOLAND 0x00497580
SpriteRec* NewSprite(void)
{
    SpriteRec* sprite = (SpriteRec*)MemAlloc(sizeof(SpriteRec));
    if (sprite) {
        sprite->next = g_sprites_head;
        g_sprites_head = sprite;
    }
    return sprite;
}

/* Original shared unbounded formatting buffer, even with the no-op sink. */
// FUNCTION: LEGOLAND 0x00453ce0
void DBError(const char* format, ...)
{
    char* args = (char*)(&format + 1);
    vsprintf(g_error_text, format, args);
    DebugErrorSink(g_error_text);
}

// FUNCTION: LEGOLAND 0x00475f10
void RestoreCurrentMenu(void)
{
    if (g_menu_index != 5) {
        TestMenu(&g_menus[g_menu_index]);
        g_panel_state.restore = 3;
    }
}

// FUNCTION: LEGOLAND 0x00498900
void SetSpeechVolume(long volume)
{
    g_speech_volume = volume;
    if (g_speech_buffer)
        g_speech_buffer->vtbl->SetVolume(g_speech_buffer, volume);
}

// FUNCTION: LEGOLAND 0x00442de0
float DotProduct(Vec3* a, Vec3* b)
{
    return a->z*b->z + a->y*b->y + a->x*b->x;
}

// FUNCTION: LEGOLAND 0x00474190
void SaveSidePanelState(void)
{
    SaveGameWrite(&g_menu_index, 4);
    SaveGameWrite(&g_object_list_mode, 4);
    SaveGameWrite(&g_panel_state, 16);
}

// FUNCTION: LEGOLAND 0x004741c0
void LoadSidePanelState(void)
{
    SaveGameRead(&g_menu_index, 4);
    SaveGameRead(&g_object_list_mode, 4);
    SaveGameRead(&g_panel_state, 16);
}

/* Do not clear the global: the original leaves the released pointer there. */
// FUNCTION: LEGOLAND 0x00473a60
void KillMouseDevice(void)
{
    if (g_mouse_device) {
        g_mouse_device->vtbl->Unacquire(g_mouse_device);
        g_mouse_device->vtbl->Release(g_mouse_device);
    }
}

// FUNCTION: LEGOLAND 0x00499380
int FreezeGameClock(void)
{
    if (!g_clock_frozen) {
        g_clock_frozen = 1;
        g_clock_freeze_ticks = GetTickCount();
        g_clock_frozen_frame = g_sim_frame;
        return 0;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x00468910
ScriptEvent* NewScriptEvent(int kind, int mode)
{
    ScriptEvent* event = (ScriptEvent*)calloc(1, sizeof(ScriptEvent));
    if (event) {
        event->next = 0;
        event->text = 0;
        event->kind = kind;
        event->mode = mode;
    }
    return event;
}

/* These eligibility helpers actually charge the 30 bricks on success. */
// FUNCTION: LEGOLAND 0x0049a120
int CanHireGardener(void)
{
    if (GetBrickCount() < 30) return 0;
    if (g_gardener_count >= 15) return 0;
    UseBricks(30);
    return 1;
}

// FUNCTION: LEGOLAND 0x0049a160
int CanHireMechanic(void)
{
    if (GetBrickCount() < 30) return 0;
    if (g_mechanic_count >= 15) return 0;
    UseBricks(30);
    return 1;
}

// FUNCTION: LEGOLAND 0x00457910
int SaveCurrency(void)
{
    if (!SaveGameWrite(&g_brick_lock, 4)) return 0;
    return SaveGameWrite(&g_bricks, 4) != 0;
}

// FUNCTION: LEGOLAND 0x00457940
int LoadCurrency(void)
{
    if (!SaveGameRead(&g_brick_lock, 4)) return 0;
    return SaveGameRead(&g_bricks, 4) != 0;
}

// FUNCTION: LEGOLAND 0x0046b760
int RestoreScriptStepHelp(void)
{
    if (!ScriptRunning() && g_script_cur) {
        ShowScriptStepText(g_script_cur, 1);
        ResetScriptTimer();
        return 1;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x00468940
void FreeScriptEvent(ScriptEvent* event)
{
    if ((event->flags & 0x20) && event->text) HeapFree_w(event->text);
    HeapFree_w(event);
}

/* No allocation check; unlisted fields stay uninitialized. */
// FUNCTION: LEGOLAND 0x004920e0
Sample* NewSampleRecord(void)
{
    Sample* sample = (Sample*)MemAlloc(sizeof(Sample));
    sample->next = 0;
    sample->refcount = 0;
    sample->source_kind = 0;
    sample->def = 0;
    sample->buffer = 0;
    sample->data30 = 0;
    sample->data34 = 0;
    sample->fade = 0;
    sample->flags = 0;
    sample->callback = 0;
    sample->due = 0;
    return sample;
}

/* A null thread handle is still reported as success when the engine exists. */
// FUNCTION: LEGOLAND 0x00495a10
int InitMusicSystem(void)
{
    if (g_music_sys) {
        g_music_thread = CreateThread(0, 0x4000, MusicThread, 0, 0, &g_music_thread_id);
        return 1;
    }
    g_music_disabled = 1;
    return 0;
}

// FUNCTION: LEGOLAND 0x0046ce20
void KillAdvisorHelp(void)
{
    if (g_advisor_help_flags & 3) {
        HeapFree_w(g_advisor_help_text);
        ToggleHelpIcon(1);
    }
    g_advisor_help_flags &= ~3;
    ClearMenuHelp();
    SetAdvisorPose(0, 0);
    StopSpeech();
}

/* Sequential stores retain the original behavior when output aliases input. */
// FUNCTION: LEGOLAND 0x00442da0
void CrossProduct(Vec3* a, Vec3* b, Vec3* out)
{
    out->x = b->z*a->y - a->z*b->y;
    out->y = a->z*b->x - a->x*b->z;
    out->z = a->x*b->y - b->x*a->y;
}

/* Explicit sign branches reproduce shifts and the original INT_MIN edge. */
// FUNCTION: LEGOLAND 0x00456770
void HalfPos(Pos* pos)
{
    int x = pos->x;
    int y = pos->y;
    if (x < 0) x = -((-x) >> 1);
    else x >>= 1;
    if (y < 0) y = -((-y) >> 1);
    else y >>= 1;
    pos->x = x;
    pos->y = y;
}

/* Formatting is unbounded into the original shared scratch buffer. */
// FUNCTION: LEGOLAND 0x00468bb0
ScriptEvent* AddHelpMessage(const char* format, ...)
{
    ScriptEvent* event = NewScriptEvent(0, 1);
    if (event) {
        char* args = (char*)(&format + 1);
        vsprintf(g_help_format_text, format, args);
        SetScriptEventText(event, g_help_format_text, 1);
        EnqueueScriptEvent(event);
    }
    return event;
}

/* Original dereferences both allocation results without null checks. */
// FUNCTION: LEGOLAND 0x004887a0
void InitZBuffer(void)
{
    g_zbuf_ready = 1;
    g_zbuffer_image = CreateSourceImage(g_zbuffer_name, 0);
    g_zbuffer_image->pixels = g_zbuffer_storage;
    g_zbuffer_image->width = 640;
    g_zbuffer_image->height = 480;
    g_zbuffer_image->refs = 1;
    g_zbuffer_image->palette = 0;
    g_zbuffer_image->type = 1;
    g_zbuffer_sprite = CreateSprite(g_zbuffer_image);
    g_zbuffer_sprite->flags |= 0x208;
}

/* Unknown names still advance the selected count; no rollback or cap. */
// FUNCTION: LEGOLAND 0x0048af40
void FreePlayItemAdd(const char* name)
{
    FPTableEntry* entry = 0;
    g_freeplay_progress += FreePlayItemUpdate(name, &entry);
    if (entry) entry->chosen = 1;
    if (g_freeplay_selected_count++ == 0)
        g_fp_accept_icon->flags &= ~0x400;
}

#ifdef LEGOLAND_PORTABLE
/* InitMusicSystem is called with 1 argument(s) the original ignores: the body
 * at this address never reads them, and in cdecl the caller cleans them up.
 * On wasm the argument count is part of the function type, so the exported
 * name is this forwarder and the matched body keeps its own.  */
#undef InitMusicSystem
int InitMusicSystem(int ll_a1) { (void)ll_a1; return InitMusicSystem_vc6_body(); }
#endif
