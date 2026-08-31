/* LEGOLAND — timer, RNG and small utility routines.
 *
 * Everything here is a leaf helper: the game clock (GetTickCount with a
 * freeze/offset pair), the linear-congruential wrappers around the CRT rand(),
 * the MIDI manager teardown and a couple of one-line predicates. */
#include "legoland.h"
#include <stdlib.h>
#include <string.h>

/* Imports must be declared __declspec(dllimport) so the call comes out as
 * `call dword ptr [__imp__X]` — a plain extern compiles to a direct rel32
 * call and will not match. */
__declspec(dllimport) unsigned long __stdcall GetTickCount(void);   /* [0x4ab1f8] */

/* ------------------------------------------------------------------ timer */

extern int g_clock_frozen;   /* 0x0079a890  non-zero: clock is held */
extern int g_clock_held;     /* 0x0079a894  tick value captured at freeze */
extern int g_clock_base;     /* 0x0079a898  epoch subtracted from the clock */

// FUNCTION: LEGOLAND 0x00499430
int GetGameTimer(void)
{
    int now;
    if (g_clock_frozen) {
        now = g_clock_held;
    } else {
        now = GetTickCount();
    }
    return now - g_clock_base;
}

/* The raw tick wrapper at 0x00499450 — `jmp dword ptr [GetTickCount]`. */
extern unsigned long GetTicks(void);   /* 0x00499450 */

// FUNCTION: LEGOLAND 0x00499480
int GetBlink(void)
{
    return (GetTicks() >> 9) & 1;
}

/* -------------------------------------------------------------------- RNG */

// FUNCTION: LEGOLAND 0x004806a0
unsigned int Rand_Max(unsigned int max)
{
    return rand() % (max + 1);
}

// FUNCTION: LEGOLAND 0x004806c0
int Rand_Tween(int lo, int hi)
{
    return Rand_Max(hi - lo) + lo;
}

/* ------------------------------------------------------------ MIDI teardown */

__declspec(dllimport) unsigned int __stdcall timeKillEvent(unsigned int id);  /* [0x4ab32c] */
__declspec(dllimport) unsigned int __stdcall midiOutClose(void* hmo);         /* [0x4ab33c] */

extern unsigned int g_midi_timer_id;   /* 0x007fd630  timeSetEvent handle */
extern int          g_midi_running;    /* 0x007fd634  manager-active flag */
extern void*        g_midi_out;        /* 0x007fd638  HMIDIOUT */

// FUNCTION: LEGOLAND 0x00480670
void KillMIDIManager(void)
{
    unsigned int id = g_midi_timer_id;
    g_midi_running = 0;
    timeKillEvent(id);
    midiOutClose(g_midi_out);
}

/* --------------------------------------------------------------- host GPU */

/* The DirectDraw host-state block at 0x00667d70, 0xf6 dwords wide; its first
 * two slots are the DirectDraw object and the primary surface (see
 * InitHostSystemGPU / KillHostSystemGPU). */
extern char g_gpu_state[0x3d8];   /* 0x00667d70 */
extern int  InitHostSystemGPU(void);   /* 0x00463700 */

// FUNCTION: LEGOLAND 0x004637c0
int CheckHostSystemGPU(void)
{
    memset(g_gpu_state, 0, sizeof(g_gpu_state));
    return InitHostSystemGPU();
}

/* --------------------------------------------------------------- clipping */

/* The Win32 RECT (exclusive right/bottom) that the DirectDraw blitter and
 * IntersectRect work on — distinct from the chained inclusive `Rect` in
 * legoland.h. */
typedef struct WinRect {
    long left;     /* +0x00 */
    long top;      /* +0x04 */
    long right;    /* +0x08 */
    long bottom;   /* +0x0c */
} WinRect;

__declspec(dllimport) int __stdcall IntersectRect(WinRect* dst, const WinRect* a,
                                                  const WinRect* b);  /* [0x4ab2a0] */

extern WinRect g_clip_rect;   /* 0x004bdea0  current clipping window */

// FUNCTION: LEGOLAND 0x0048a6c0
int ClipThisRect(WinRect* rc)
{
    return IntersectRect(rc, rc, &g_clip_rect);
}

/* ------------------------------------------------------------------- misc */

// FUNCTION: LEGOLAND 0x0044dd60
void FreeBinV(void* p)
{
    if (p) {
        free(p);
    }
}

extern char Get_RFFlags(int x, int y);   /* 0x00461610 */

// FUNCTION: LEGOLAND 0x00401800
int IsSemiPermiable(int x, int y)
{
    return (Get_RFFlags(x, y) & 3) == 3;
}
