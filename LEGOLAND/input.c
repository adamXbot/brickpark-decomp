/* LEGOLAND — input / system events.
 *
 * The per-frame input pump: drain the Win32 message queue, poll the two
 * DirectInput devices (keyboard + mouse) into their raw state buffers, then
 * fold those buffers into the shared "controller" record the game logic reads.
 *
 * Struct field OFFSETS are recovered from the disassembly; the names are ours.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "legoland.h"

/* ---- DirectInput (minimal, offsets only) --------------------------------- */
typedef struct DIDevice DIDevice;
typedef struct DIDeviceVtbl {
    long (__stdcall* QueryInterface)(DIDevice*, const void*, void**);  /* +0x00 */
    long (__stdcall* AddRef)(DIDevice*);                               /* +0x04 */
    long (__stdcall* Release)(DIDevice*);                              /* +0x08 */
    long (__stdcall* GetCapabilities)(DIDevice*, void*);               /* +0x0c */
    long (__stdcall* EnumObjects)(DIDevice*, void*, void*, unsigned long); /* +0x10 */
    long (__stdcall* GetProperty)(DIDevice*, const void*, void*);      /* +0x14 */
    long (__stdcall* SetProperty)(DIDevice*, const void*, const void*);/* +0x18 */
    long (__stdcall* Acquire)(DIDevice*);                              /* +0x1c */
    long (__stdcall* Unacquire)(DIDevice*);                            /* +0x20 */
    long (__stdcall* GetDeviceState)(DIDevice*, unsigned long, void*); /* +0x24 */
    long (__stdcall* GetDeviceData)(DIDevice*, unsigned long, void*,
                                    unsigned long*, unsigned long);    /* +0x28 */
    long (__stdcall* SetDataFormat)(DIDevice*, const void*);           /* +0x2c */
    long (__stdcall* SetEventNotification)(DIDevice*, void*);          /* +0x30 */
    long (__stdcall* SetCooperativeLevel)(DIDevice*, void*, unsigned long); /* +0x34 */
} DIDeviceVtbl;
struct DIDevice { DIDeviceVtbl* lpVtbl; };

#define DIERR_NOTACQUIRED 0x8007000C
#define DIERR_INPUTLOST   0x8007001E

/* The raw DIMOUSESTATE the mouse device is polled into (@ 0x00668d78). */
typedef struct MouseState {
    int           lX;             /* +0x00 relative x this poll */
    int           lY;             /* +0x04 relative y this poll */
    int           lZ;             /* +0x08 wheel delta this poll */
    unsigned char rgbButtons[4];  /* +0x0c high bit = down */
} MouseState;

extern DIDevice*  g_di_keyboard;      /* 0x00668d8c */
extern DIDevice*  g_di_mouse;         /* 0x00668d90 */
extern MouseState g_mouse_state;      /* 0x00668d78 */
/* DIPROP_GRANULARITY of the mouse Z axis (one wheel notch, normally 120). */
extern int        g_wheel_granularity; /* 0x004bad54 */
/* 256 DirectInput key states, indexed by DIK_* scan code; high bit = down. */
extern unsigned char g_key_state[256]; /* 0x007fdda0 */

/* The shared controller record (the game's virtual mouse/cursor + buttons).
   CONTROLLERBUFFER @ 0x00813b00 holds the pointer to it. */
typedef struct Controller {
    int x0;        /* +0x00 previous x (start of this tick) */
    int y0;        /* +0x04 previous y */
    int x;         /* +0x08 current x (screen pixels) */
    int y;         /* +0x0c current y */
    int dx;        /* +0x10 x - x0 for this tick */
    int dy;        /* +0x14 y - y0 for this tick */
    int buttons;   /* +0x18 button/key bit set */
    int accel_t1;  /* +0x1c stage-1 acceleration threshold */
    int accel_t2;  /* +0x20 stage-2 acceleration threshold */
    int accel;     /* +0x24 0 = off, 1 = one stage, 2 = two stages */
} Controller;

/* buttons (+0x18) bit map, rebuilt from scratch every tick:
 *   0x001 mouse button 0        0x002 mouse button 1      0x004 mouse button 2
 *   0x008 DIK_LEFT  (0xcb)      0x010 DIK_RIGHT (0xcd)
 *   0x020 DIK_UP    (0xc8)      0x040 DIK_DOWN  (0xd0)
 *   0x080 DIK_SPACE (0x39)      0x100 DIK_TAB   (0x0f)
 *   0x200 DIK_ESCAPE(0x01)      0x400 DIK_RETURN(0x1c)
 * The mouse pass clears bits 0..2 (&= ~0x007) and the keyboard pass clears
 * bits 3..10 (&= ~0x7f8), so bits above 0x400 survive from elsewhere.
 *
 * Per tick (ProcessSystemEvents order): pump messages -> ScanKeyboard ->
 * ScanMouse -> UpdateControllerFromMouseData -> UpdateControllerFromKeyboardData.
 * The mouse pass is the one that moves the cursor: x0/y0 <- x/y, then
 * x/y += the (optionally doubled, then doubled again) DirectInput deltas,
 * clamp to [0, screen-1], then dx/dy <- x-x0, y-y0. */

/* The global game record @ 0x004bcbf4 (legoland.h calls it g_map and uses its
   map extent at +0x14/+0x16); the cursor is clamped to the u16 pair at +0/+2. */
typedef struct Screen {
    unsigned short w;   /* +0x00 */
    unsigned short h;   /* +0x02 */
} Screen;
extern Screen* g_screen;   /* 0x004bcbf4 */

/* Icon-bar wheel scroll (0x0046db40 / 0x0046dac0). */
extern void IconBarWheelDown(void);
extern void IconBarWheelUp(void);

// FUNCTION: LEGOLAND 0x00473930
void ScanKeyboard(void)
{
    long hr;

    if (g_di_keyboard == 0)
        return;
    /* The trailing `continue` is load-bearing: without it VC6 rotates the loop
       and peels a copy of the poll into the header. */
    while (1) {
        hr = g_di_keyboard->lpVtbl->GetDeviceState(g_di_keyboard, 256, g_key_state);
        if (hr == 0)
            break;
        if (hr == DIERR_NOTACQUIRED || hr == DIERR_INPUTLOST)
            g_di_keyboard->lpVtbl->Acquire(g_di_keyboard);
        continue;
    }
}

// FUNCTION: LEGOLAND 0x00473a80
void ScanMouse(void)
{
    long hr;
    int wheel;
    int notch;

    if (g_di_mouse != 0) {
        while (1) {
            hr = g_di_mouse->lpVtbl->GetDeviceState(g_di_mouse, 16, &g_mouse_state);
            if (hr == 0)
                break;
            if (hr == DIERR_NOTACQUIRED || hr == DIERR_INPUTLOST)
                g_di_mouse->lpVtbl->Acquire(g_di_mouse);
            continue;
        }
    }
    notch = g_wheel_granularity;
    wheel = g_mouse_state.lZ;
    if (wheel <= -notch)
        IconBarWheelDown();
    else if (wheel >= notch)
        IconBarWheelUp();
}

/* 109 of the original's 109 instructions in order; 7 differ (280 vs 272 bytes):
 * the two low clamps.  The original holds the constant 0 in edx across them
 *      mov  eax,[ecx+8] / xor edx,edx / cmp eax,edx / jge / mov [ecx+8],edx
 *      mov  edi,[g_screen] ... / cmp dword [ecx+0ch],edx / jge / mov [ecx+0ch],edx
 * while ours emits `test eax,eax` + `mov dword ptr [..],0` and takes the second
 * g_screen reload in edx instead of edi.
 *
 * Everything else -- the split prologue (push ebx / mov ebx,[ecx+24h] /
 * push esi / push edi straddling the null guard), the stage-1 threshold
 * re-read (`mov edx,[ecx+1ch]` before each compare), stage 2's threshold in
 * ebx, `add esi,esi` vs `shl esi,1` for the two doublings, x0/y0 in edi/esi
 * for the delta write-back, `and al,0f8h`, `mov dl,80h` for the three button
 * tests and the pops interleaved with the first test -- is exact.
 *
 * What the residual is (measured, scratchpad/ucfm/):
 *  - The volatile cast on the second stage-1 read is the lever that stops VC6
 *    global-CSEing the two reads of accel_t1 (same lever as renderinit.c).
 *    Without it VC6 keeps the CSE temp in ebp (push/pop ebp, 111 insns) and
 *    renames three later temps onto ebp.  Nothing else breaks that CSE: two
 *    `if`s, `else if`, goto, De Morgan, ternary, do/while(0), switch, a hoisted
 *    `int t1 = c->accel_t1` at any scope (VC6 sinks the load back to the use),
 *    `static __inline` reader/compare helpers, int/long/enum unions and casts
 *    at the same offset, an overlay struct, dead `if (c == 0) goto` edges into
 *    the second read (threaded away before CSE), `register`, `long`+`labs`.
 *  - The 0 is a register-allocation mode effect, not a spelling: VC6 puts the
 *    0 in edx (and the second g_screen load in edi) exactly when ebp is in the
 *    allocator's pool -- either used by some candidate (our CSE build, or any
 *    value live across stage 1) or reserved as the frame pointer.  With ebp
 *    reserved (/Oy-, or an empty `__asm {}`) this body compiles to 108/112:
 *    every instruction of the original plus only push ebp/mov ebp,esp/[ebp+8]/
 *    pop ebp.  With ebp free and unused the 0 stays an immediate whatever the
 *    clamps look like (`< 0`, `0 >`, `!(>= 0)`, ternary, `int lo = 0` at any
 *    scope, opaque zeros such as `dx - dx`, `>= w` vs `> w - 1`), while -1 and
 *    1 ARE enregistered in the same position, so it is specific to the 0.
 *    The original is FPO (`mov ecx,[esp+4]`) and uses no ebp, so its compiler
 *    had ebp in the pool without spending it; no /O2-compatible flag or pragma
 *    reproduces that state here (/Oy-, /Og-, /Ot-, /Oi-, /Ob0-2, /Os, /G3-/G6, /Gs, /Ge,
 *    /Gh, /GX, /Op, /Za, /TP, /MD, /MT, /QI0f, /QIfdiv all tried).
 * Best attempt and the experiment log live in scratchpad/wipfix.c.
 */
// WIP-FUNCTION: LEGOLAND 0x00473b00  (109/109 insns, 102/109 = 93.6%; the two low clamps hold 0 in edx in the original -- see note)
void UpdateControllerFromMouseData(Controller* c)
{
    int dx;
    int dy;
    int accel;

    if (c == 0)
        return;
    c->x0 = c->x;
    c->y0 = c->y;
    dx = g_mouse_state.lX;
    dy = g_mouse_state.lY;
    accel = c->accel;
    if (accel != 0) {
        /* The volatile cast is a codegen lever, not semantics: it is what
           makes VC6 re-read accel_t1 for the second half of the `||` as the
           original does, instead of CSEing it into a fourth callee-saved
           register (see the note above). */
        if (abs(dx) > c->accel_t1 || abs(dy) > *(volatile int*)&c->accel_t1) {
            dx += dx;
            dy += dy;
        }
        if (accel == 2) {
            if (abs(dx) > c->accel_t2 || abs(dy) > c->accel_t2) {
                dx <<= 1;
                dy <<= 1;
            }
        }
    }
    c->x += dx;
    c->y += dy;
    if (c->x >= g_screen->w)
        c->x = g_screen->w - 1;
    if (c->x < 0)
        c->x = 0;
    if (c->y >= g_screen->h)
        c->y = g_screen->h - 1;
    if (c->y < 0)
        c->y = 0;
    c->dx = c->x - c->x0;
    c->dy = c->y - c->y0;
    c->buttons &= ~7;
    if (g_mouse_state.rgbButtons[0] & 0x80)
        c->buttons |= 1;
    if (g_mouse_state.rgbButtons[2] & 0x80)
        c->buttons |= 4;
    if (g_mouse_state.rgbButtons[1] & 0x80)
        c->buttons |= 2;
}

/* The game record's flag byte at +0x1c: bit 0 = "window is inactive / paused",
   set by the WM_ACTIVATEAPP handler; the pump idles in WaitMessage while set. */
typedef struct GameEnv {
    char          pad0[0x1c];
    unsigned char flags;      /* +0x1c bit0 = suspended */
} GameEnv;
extern GameEnv* g_game;       /* 0x004bcbf4 (same record as g_screen/g_map) */

/* The one shared controller (CONTROLLERBUFFER @ 0x00813b00). */
extern Controller* g_controller;

extern void* WNDENV_Gethwnd(void);        /* 0x0047fe60 */
extern void ResendPalette(void);          /* 0x0044e670 */
extern void UpdateControllerFromKeyboardData(Controller* c);  /* 0x00473c10 */

// FUNCTION: LEGOLAND 0x00480050
int ProcessSystemEvents(void)
{
    MSG msg;
    int slept;

    WNDENV_Gethwnd();
    slept = 0;
    do {
        while (PeekMessageA(&msg, (HWND)WNDENV_Gethwnd(), 0, 0, PM_NOREMOVE)) {
            if (!GetMessageA(&msg, (HWND)WNDENV_Gethwnd(), 0, 0))
                return 1;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            SetCursor(0);
        }
        if (g_game->flags & 1) {
            slept = 1;
            WaitMessage();
        }
    } while (g_game->flags & 1);
    if (slept)
        ResendPalette();
    ScanKeyboard();
    ScanMouse();
    UpdateControllerFromMouseData(g_controller);
    UpdateControllerFromKeyboardData(g_controller);
    return 1;
}

/* --- cheat-code plumbing --------------------------------------------------
   Every printable key the player types is pushed into a 20-byte ring; each
   cheat is a strnicmp/memcmp of the ring's TAIL against a ":WORD" literal, so
   a code fires the moment its last character lands. */
extern char g_type_buf[20];      /* 0x00668d94 */
extern int  g_game_mode;         /* 0x008119b4 — 3 == in a running park */
extern int  g_pending_state;     /* 0x00832ba0 — 2 == restart at env->level */
extern int  g_ride_wear;         /* 0x00832980 — 0 disables ride wear */
extern int  g_instant_appraisal; /* 0x00666098 */
extern int  g_show_capacity;     /* 0x00832994 */

extern char GetTypedChar(void);        /* 0x00474130 */
extern void SetTheme(int theme);       /* 0x00492ce0 */
extern void StopMusic(void);           /* 0x00492d80 */
extern void AddBricks(int amount);     /* 0x004578a0 */
extern void EndLevel(int outcome);     /* 0x00459820 */
extern void StopScript(int stop);      /* 0x0046b240 */
extern void TriggerSwitch(int which);  /* 0x00460560 */
extern void DBPrintf(const char* fmt, ...);  /* 0x00453a20 */

/* The record at 0x004bcbf4 again: +0x28 is the level number. */
typedef struct GameLevel {
    char pad0[0x28];
    int  level;    /* +0x28 */
} GameLevel;
extern GameLevel* g_level_rec;   /* 0x004bcbf4 */

// FUNCTION: LEGOLAND 0x00473c10
void UpdateControllerFromKeyboardData(Controller* c)
{
    char ch;

    c->buttons &= ~0x7f8;
    ch = GetTypedChar();
    if (ch != 0) {
        memcpy(g_type_buf, g_type_buf + 1, 19);
        g_type_buf[19] = ch;
        if (strnicmp(":THEME", &g_type_buf[14], 6) == 0) {
            SetTheme(0);
            DBPrintf("CHEATTHEME=THEME\n");
        } else if (strnicmp(":EGYPT", &g_type_buf[14], 6) == 0) {
            SetTheme(1);
            DBPrintf("CHEATTHEME=EGYPTIAN\n");
        } else if (strnicmp(":INCA", &g_type_buf[15], 5) == 0) {
            SetTheme(2);
            DBPrintf("CHEATTHEME=INCA\n");
        } else if (strnicmp(":CASTLE", &g_type_buf[13], 7) == 0) {
            SetTheme(3);
            DBPrintf("CHEATTHEME=CASTLE\n");
        } else if (strnicmp(":WEST", &g_type_buf[15], 5) == 0) {
            SetTheme(4);
            DBPrintf("CHEATTHEME=WEST\n");
        } else if (strnicmp(":STOP", &g_type_buf[15], 5) == 0) {
            StopMusic();
            DBPrintf("CHEAT:STOPMUSIC\n");
        } else if (strnicmp("::DIE", &g_type_buf[15], 5) == 0) {
            exit(1);
        }
        if (g_game_mode == 3) {
            if (strnicmp(":ILIKETOTRAVEL", &g_type_buf[4], 14) == 0) {
                ch = g_type_buf[18];
                if (ch == '1') {
                    if (g_type_buf[19] == '0') {
                        g_level_rec->level = 15;
                        DBPrintf("CHEAT:Level %d\n", 10);
                        g_pending_state = 2;
                    }
                } else if (ch == '0') {
                    ch = g_type_buf[19];
                    if (ch >= '1' && ch <= '9') {
                        g_level_rec->level = ch - 0x2b;
                        DBPrintf("CHEAT:Level %d\n", g_level_rec->level);
                        g_pending_state = 2;
                    }
                } else if (ch == 'T') {
                    ch = g_type_buf[19];
                    if (ch >= '1' && ch <= '5') {
                        g_level_rec->level = ch - '0';
                        DBPrintf("CHEAT:Level %d\n", g_level_rec->level);
                        g_pending_state = 2;
                    }
                }
            } else if (strnicmp(":COLDHARDCASH", &g_type_buf[7], 13) == 0) {
                DBPrintf("CHEAT:More Money\n");
                AddBricks(5000);
            } else if (strnicmp(":HARDASNAILS", &g_type_buf[8], 12) == 0) {
                DBPrintf("CHEAT:No Ride Wear\n");
                g_ride_wear = 0;
            } else if (strnicmp(":PRAISEME", &g_type_buf[11], 9) == 0) {
                DBPrintf("CHEAT:Instant Appraisal\n");
                g_instant_appraisal = 1;
            } else if (strnicmp(":WELOVELEGOLAND", &g_type_buf[5], 15) == 0) {
                DBPrintf("CHEAT:Win Level\n", g_level_rec->level);
                EndLevel(1);
            } else if (memcmp(&g_type_buf[10], ":IMPROVISE", 10) == 0) {
                DBPrintf("CHEAT:Stop Script\n", g_level_rec->level);
                StopScript(1);
            } else if (strnicmp(":DIGGER", &g_type_buf[13], 7) == 0) {
                DBPrintf("CHEAT:Set Switch 1\n");
                TriggerSwitch(0);
                TriggerSwitch(1);
                TriggerSwitch(2);
                TriggerSwitch(3);
            } else if (memcmp(&g_type_buf[7], ":SHOWCAPACITY", 13) == 0) {
                g_show_capacity = 1;
                DBPrintf("CHEAT: Capacity Calcs visible\n");
            }
        }
    }
    if (g_key_state[0xcb] & 0x80)
        c->buttons |= 0x08;
    if (g_key_state[0xcd] & 0x80)
        c->buttons |= 0x10;
    if (g_key_state[0xc8] & 0x80)
        c->buttons |= 0x20;
    if (g_key_state[0xd0] & 0x80)
        c->buttons |= 0x40;
    if (g_key_state[0x39] & 0x80)
        c->buttons |= 0x80;
    if (g_key_state[0x0f] & 0x80)
        c->buttons |= 0x100;
    if (g_key_state[0x01] & 0x80)
        c->buttons |= 0x200;
    if (g_key_state[0x1c] & 0x80)
        c->buttons |= 0x400;
}
