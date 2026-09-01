/* LEGOLAND — subsystem init / shutdown.
 *
 * The paired Init / Kill entry points for the map-audio table, DirectInput,
 * the worker-interface graphics, the MIDI manager and the sound system.
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd); only the
 * struct field offsets, vtable slots and callee arg counts are load-bearing —
 * names are ours.
 *
 * Init/shutdown ordering recovered here:
 *   sound   InitSoundSystem  = hwnd -> samples -> music   (0x004964f0)
 *           KillSoundSystem  = samples -> music           (0x00496520, util-side)
 *   input   KillInputSystem  = keyboard -> mouse -> IDirectInput
 *   MIDI    InitMIDIManager  = timeSetEvent tick then midiOutOpen;
 *           KillMIDIManager  (LEGOLAND/util.c) unwinds in the same order.
 */
#include "legoland.h"

/* ------------------------------------------------------------- map SFX table */

/* One row of an FX table (stride 0xc) — same layout as LEGOLAND/audiomisc.c's
 * FXEntry; repeated locally because structs are not shared through
 * legoland.h. */
typedef struct FXEntry {
    char* name;    /* +0x00 */
    int   pad4;    /* +0x04 */
    void* sample;  /* +0x08  SampleDef*, filled in by Load_FXList */
} FXEntry;

extern void Kill_FXList(FXEntry* list, int count);   /* 0x00496e30 */

/* The 0x17-entry game-map FX table; InitGameMap (0x00459850) loads it with
 * Load_FXList and stashes ElemID("CASTLE OBJ") alongside. */
extern FXEntry g_map_fx[];   /* 0x004b9228 — 23 entries */

/* Teardown is just the table release — the ElemID handle InitGameMap caches is
 * owned by the element bank and is not freed here. */
// FUNCTION: LEGOLAND 0x00459870
void KillGameMap(void)
{
    Kill_FXList(g_map_fx, 23);
}

/* ---------------------------------------------------------------- DirectInput */

/* Only Release (+0x08) is used here; the leading slots pin its offset. */
typedef struct IDInput IDInput;
typedef struct IDInputVtbl {
    long (__stdcall* QueryInterface)(IDInput*, const void*, void**);  /* +0x00 */
    long (__stdcall* AddRef)(IDInput*);                               /* +0x04 */
    long (__stdcall* Release)(IDInput*);                              /* +0x08 */
} IDInputVtbl;
struct IDInput { IDInputVtbl* lpVtbl; };

/* The three DirectInput globals live in one run: the IDirectInput object then
 * its two devices. KillKeyboardDevice/KillMouseDevice release 0x00668d8c and
 * 0x00668d90 (the mouse is Unacquire'd @ +0x20 first); the object itself goes
 * last, so the devices never outlive their parent. */
extern IDInput* g_dinput;              /* 0x00668d88 */
extern void KillKeyboardDevice(void);  /* 0x00473a50 */
extern void KillMouseDevice(void);     /* 0x00473a60 */

// FUNCTION: LEGOLAND 0x00473ae0
void KillInputSystem(void)
{
    KillKeyboardDevice();
    KillMouseDevice();
    if (g_dinput != 0)
        g_dinput->lpVtbl->Release(g_dinput);
}

/* ------------------------------------------------------- worker interface GFX */

extern Elem* ElemID(const char* name);   /* 0x0047b3f0 */

/* Resolved element handle for the worker overlay's path artwork. */
extern Elem* g_worker_path_tiles;   /* 0x0079abfc */

// FUNCTION: LEGOLAND 0x00499530
void LoadWorkerInterfaceGFX(void)
{
    g_worker_path_tiles = ElemID("NORMAL PATH TILES");
}

/* ------------------------------------------------------------- MIDI manager */

/* Imports must be __declspec(dllimport) so the call comes out as
 * `call dword ptr [__imp__X]` rather than a direct rel32. */
typedef void (__stdcall* MMTimeProc)(unsigned int id, unsigned int msg,
                                     unsigned long user, unsigned long dw1,
                                     unsigned long dw2);

__declspec(dllimport) unsigned int __stdcall timeSetEvent(unsigned int delay,
                                                          unsigned int res,
                                                          MMTimeProc cb,
                                                          unsigned long user,
                                                          unsigned int flags);  /* [0x4ab330] */
__declspec(dllimport) unsigned int __stdcall midiOutOpen(void** phmo,
                                                         unsigned int devid,
                                                         unsigned long cb,
                                                         unsigned long inst,
                                                         unsigned long flags);  /* [0x4ab334] */

/* Same three globals KillMIDIManager (LEGOLAND/util.c) unwinds. 0x007fd634 is
 * cleared here and there as "no sequence playing"; the timer callback at
 * 0x00480570 in fact dereferences it as the active-sequence pointer. */
extern unsigned int g_midi_timer_id;   /* 0x007fd630  timeSetEvent handle */
extern int          g_midi_running;    /* 0x007fd634  manager-active flag */
extern void*        g_midi_out;        /* 0x007fd638  HMIDIOUT */

extern void __stdcall MIDITimerTick(unsigned int id, unsigned int msg,
                                    unsigned long user, unsigned long dw1,
                                    unsigned long dw2);   /* 0x00480570 */

/* 20 ms period at 10 ms resolution, TIME_PERIODIC; the MIDI out port is the
 * MIDI mapper (-1) with no callback. Always reports success. */
// FUNCTION: LEGOLAND 0x00480630
int InitMIDIManager(void)
{
    g_midi_running = 0;
    g_midi_timer_id = timeSetEvent(20, 10, MIDITimerTick, 0, 1);
    midiOutOpen(&g_midi_out, (unsigned int)-1, 0, 0, 0);
    return 1;
}

/* ------------------------------------------------------------- sound system */

extern void* WNDENV_Gethwnd(void);              /* 0x0047fe60 */
extern int   InitSoundSampleSystem(void* hwnd); /* 0x00492130 */
extern int   InitMusicSystem(void* hwnd);       /* 0x00495a10 */

/* The window the sound system was brought up against; re-read (not kept in a
 * register) for the music-system call, which is why the original reloads
 * [0x007988b0] after the first call. */
extern void* g_snd_hwnd;   /* 0x007988b0 */

/* Mirror of KillSoundSystem (0x00496520, LEGOLAND/audiomisc.c): `return ok;`
 * on the failure path — not `return 0;` — is load-bearing. ok is already in
 * eax and known zero, so VC6 emits a bare `ret` with no `xor eax,eax`, giving
 * the original's `test eax,eax / jne <body> / ret`. */
// FUNCTION: LEGOLAND 0x004964f0
int InitSoundSystem(void)
{
    int ok;

    g_snd_hwnd = WNDENV_Gethwnd();
    ok = InitSoundSampleSystem(g_snd_hwnd);
    if (!ok)
        return ok;
    return InitMusicSystem(g_snd_hwnd) != 0;
}

/* ------------------------------------------------------ sample system teardown */

/* Only Release (+0x08) is used; the leading slots pin its offset. */
typedef struct IDSound IDSound;
typedef struct IDSoundVtbl {
    long (__stdcall* QueryInterface)(IDSound*, const void*, void**);  /* +0x00 */
    long (__stdcall* AddRef)(IDSound*);                               /* +0x04 */
    long (__stdcall* Release)(IDSound*);                              /* +0x08 */
} IDSoundVtbl;
struct IDSound { IDSoundVtbl* lpVtbl; };

extern void DeletePlayableSamples(int flag);   /* 0x00492b90 */

extern int      g_samples_ready;   /* 0x007988c0  non-zero once brought up */
extern IDSound* g_dsound;          /* 0x007cad40  IDirectSound object */

/* Idempotent: a second call sees the cleared ready flag and reports failure.
 * Note the object pointer is released and blanked BEFORE the ready flag, so a
 * re-entrant call during Release would still take the "up" path — reproduced
 * as written. */
// FUNCTION: LEGOLAND 0x00492c20
int KillSoundSampleSystem(void)
{
    if (!g_samples_ready)
        return 0;
    DeletePlayableSamples(0);
    g_dsound->lpVtbl->Release(g_dsound);
    g_dsound = 0;
    g_samples_ready = 0;
    return 1;
}
