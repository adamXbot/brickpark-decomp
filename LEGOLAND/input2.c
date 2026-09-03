/* LEGOLAND — input system bring-up, the main window procedure and the
 * AVI/sample playback helpers.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only the
 * struct field offsets, vtable slots and callee arg counts are load-bearing;
 * names are ours.
 *
 * NOTE: LegoLandWindowProc is __stdcall, so its COFF symbol is
 * `_LegoLandWindowProc@16`. tools/match.py looks the name up as `X` / `_X`
 * and, failing that, falls back to the FIRST .text COMDAT of the object, so
 * the window procedure must stay the first function defined in this file.
 */
#include "legoland.h"

/* Imports must be __declspec(dllimport) so the call comes out as
 * `call dword ptr [__imp__X]` rather than a direct rel32. */
__declspec(dllimport) long  __stdcall DefWindowProcA(void* hwnd, unsigned int msg,
                                                     unsigned int wp, long lp); /* [0x4ab290] */
__declspec(dllimport) void  __stdcall PostQuitMessage(int code);                /* [0x4ab314] */
__declspec(dllimport) long  __stdcall GetWindowLongA(void* hwnd, int idx);      /* [0x4ab2f0] */
__declspec(dllimport) short __stdcall GetKeyState(int vk);                      /* [0x4ab2e8] */

#define WM_SETFOCUS    0x0007
#define WM_KILLFOCUS   0x0008
#define WM_CLOSE       0x0010
#define WM_CANCELMODE  0x001f
#define WM_CHAR        0x0102
#define GWL_HINSTANCE  (-6)
#define VK_CAPITAL     0x14

/* ---- the game record @ 0x004bcbf4 -------------------------------------- */
/* +0x1c is a u16 flags word; bit 0 = "window inactive / suspended" (the
 * ProcessSystemEvents pump idles in WaitMessage while it is set). */
typedef struct GameEnv {
    char           pad0[0x1c];
    unsigned short flags;      /* +0x1c */
} GameEnv;
extern GameEnv* g_game;        /* 0x004bcbf4 */

/* ---- input configuration block @ 0x00813a40 ---------------------------- */
/* Written once by SetupControllers. +0x00 is a dword flag set (bit 0x400 is
 * cleared by the WM_CHAR backspace handler, bit 0x20 is tested at 0x451f70);
 * the nine 8-byte slots from +0x58 carry the controller button masks the
 * keyboard/mouse passes fold into Controller.buttons. */
typedef struct InputConfig {
    int flags;        /* +0x00 (0x813a40) */
    int pad04;        /* +0x04 */
    int pad08;        /* +0x08 */
    int f0c;          /* +0x0c (0x813a4c) = 1 */
    int pad10;        /* +0x10 */
    int f14;          /* +0x14 (0x813a54) = 4 */
    int pad18;        /* +0x18 */
    int f1c;          /* +0x1c (0x813a5c) = 2 */
    int pad20;        /* +0x20 */
    int f24;          /* +0x24 (0x813a64) = 10 */
    int f28;          /* +0x28 (0x813a68) = 4 */
    int f2c;          /* +0x2c (0x813a6c) = 1 */
    int f30;          /* +0x30 (0x813a70) = 1 */
    char pad34[0x58 - 0x34];
    int mask_up;      /* +0x58 (0x813a98) = 0x20  */
    int pad5c;
    int mask_right;   /* +0x60 (0x813aa0) = 0x10  */
    int pad64;
    int mask_down;    /* +0x68 (0x813aa8) = 0x40  */
    int pad6c;
    int mask_left;    /* +0x70 (0x813ab0) = 0x08  */
    int pad74;
    int mask_tab;     /* +0x78 (0x813ab8) = 0x100 */
    int pad7c;
    int mask_btn0;    /* +0x80 (0x813ac0) = 0x01  */
    int pad84;
    int mask_btn1;    /* +0x88 (0x813ac8) = 0x02  */
    int pad8c;
    int mask_esc;     /* +0x90 (0x813ad0) = 0x200 */
    int pad94;
    int mask_return;  /* +0x98 (0x813ad8) = 0x400 */
} InputConfig;
extern InputConfig g_input_cfg;      /* 0x00813a40 */

/* The shared controller record (see input.c for the full layout); only the
 * button word at +0x18 is touched here. */
typedef struct Controller {
    char pad0[0x18];
    int  buttons;     /* +0x18 */
} Controller;
extern Controller* g_controller;       /* 0x00813b00  CONTROLLERBUFFER */
extern int         g_controllers_ready;/* 0x00667104 */

extern int  g_edit_state;              /* 0x008119b0  2 = cursor reset pending */
extern int  g_clock_was_frozen;        /* 0x00669238  FreezeGameClock() result at focus loss */
extern char g_edit_cursor[];           /* 0x007febc0  the EditCursor block (loaders.c) */

extern int   FreezeGameClock(void);    /* 0x00499380  1 if it was already frozen */
extern void  ThawGameClock(void);      /* 0x004993c0 */
extern void  DefaultCursor(void* c);   /* 0x0045a390 */
extern void* HeapAlloc_w(unsigned int size); /* 0x0049e4ff */
extern void* WNDENV_Gethwnd(void);     /* 0x0047fe60 */

/* ---- DirectInput bring-up --------------------------------------------- */
extern void* g_dinput;                 /* 0x00668d88  IDirectInput */
/* dinput.lib's statically linked entry (0x0049d320): stdcall, 4 args. */
extern long __stdcall DirectInputCreateA(void* hinst, unsigned long ver,
                                         void** out, void* outer);
extern int CreateKeyboardDevice(void); /* 0x004738b0 (internal) */
extern int CreateMouseDevice(void);    /* 0x00473970 (internal) */

/* ---- typed-key decoding ------------------------------------------------ */
/* 256 DirectInput key states, indexed by DIK_* scan code; high bit = down. */
extern unsigned char g_key_state[256]; /* 0x007fdda0 */

/* The DIK -> character map @ 0x004bad58: 59 {scan code, char} pairs. Arrows
 * map to -11..-14 (UP/LEFT/RIGHT/DOWN), BACKSPACE -1, ESCAPE -2, RETURN and
 * NUMPADENTER -3, CAPSLOCK/LSHIFT/RSHIFT -10, letters upper case. */
typedef struct KeyMapEntry {
    unsigned char dik;   /* +0x00 */
    char          ch;    /* +0x01 */
} KeyMapEntry;
#define KEYMAP_COUNT 59
extern KeyMapEntry g_key_map[KEYMAP_COUNT];      /* 0x004bad58 */
/* Previous poll's key state per map entry (high bit = was down), so a key
 * fires once per press. Declared `char`: the (unsigned)(x & 0x80) >> 7 read
 * is what gives the original's un-extended `mov dl / shr / and 1`. */
extern char g_key_prev[KEYMAP_COUNT];            /* 0x00668da8 */

extern int IsLShiftDown(void);   /* 0x00474070  g_key_state[DIK_LSHIFT] >> 7 */
extern int IsRShiftDown(void);   /* 0x00474080  g_key_state[DIK_RSHIFT] >> 7 */
extern int tolower(int c);       /* 0x0049ef23  CRT */

/* ---- sample record (audio3.c layout) ----------------------------------- */
typedef struct IDSBuffer IDSBuffer;
typedef struct IDSound   IDSound;

typedef struct MapRef {
    int x;       /* +0x00 */
    int y;       /* +0x04 */
} MapRef;

typedef struct SoundSource {
    int    kind; /* +0x00  0 none / 1 bloke / 2 map ref / 3 level xy */
    void*  obj;  /* +0x04 */
    MapRef pos;  /* +0x08 */
} SoundSource;

typedef struct Sample {
    struct Sample* next;     /* +0x00 */
    int            refcount; /* +0x04 */
    int            fade;     /* +0x08 */
    SoundSource    src;      /* +0x0c */
    unsigned short flags;    /* +0x1c  bit2 looping, bit3 fading */
    short          pad1e;    /* +0x1e */
    unsigned int   due;      /* +0x20 */
    void*          callback; /* +0x24 */
    struct Sample* def;      /* +0x28 */
    IDSBuffer*     buf;      /* +0x2c */
} Sample;

typedef struct IDSBufferVtbl {
    long          (__stdcall *QueryInterface)(IDSBuffer*, const void*, void**); /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDSBuffer*);                             /* +0x04 */
    unsigned long (__stdcall *Release)(IDSBuffer*);                            /* +0x08 */
    long          (__stdcall *GetCaps)(IDSBuffer*, void*);                     /* +0x0c */
    long          (__stdcall *GetCurrentPosition)(IDSBuffer*, unsigned long*, unsigned long*); /* +0x10 */
    long          (__stdcall *GetFormat)(IDSBuffer*, void*, unsigned long, unsigned long*);    /* +0x14 */
    long          (__stdcall *GetVolume)(IDSBuffer*, long*);                   /* +0x18 */
    long          (__stdcall *GetPan)(IDSBuffer*, long*);                      /* +0x1c */
    long          (__stdcall *GetFrequency)(IDSBuffer*, unsigned long*);       /* +0x20 */
    long          (__stdcall *GetStatus)(IDSBuffer*, unsigned long*);          /* +0x24 */
    long          (__stdcall *Initialize)(IDSBuffer*, void*, void*);           /* +0x28 */
    long          (__stdcall *Lock)(IDSBuffer*, unsigned long, unsigned long,
                                    void**, unsigned long*, void**,
                                    unsigned long*, unsigned long);            /* +0x2c */
    long          (__stdcall *Play)(IDSBuffer*, unsigned long, unsigned long,
                                    unsigned long);                            /* +0x30 */
    long          (__stdcall *SetCurrentPosition)(IDSBuffer*, unsigned long);   /* +0x34 */
} IDSBufferVtbl;
struct IDSBuffer { IDSBufferVtbl* lpVtbl; };

typedef struct IDSoundVtbl {
    long          (__stdcall *QueryInterface)(IDSound*, const void*, void**);  /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDSound*);                              /* +0x04 */
    unsigned long (__stdcall *Release)(IDSound*);                             /* +0x08 */
    long          (__stdcall *CreateSoundBuffer)(IDSound*, const void*, IDSBuffer**, void*); /* +0x0c */
} IDSoundVtbl;
struct IDSound { IDSoundVtbl* lpVtbl; };

/* DSBUFFERDESC (DirectX 5/6 layout, 0x24 bytes). */
typedef struct DSBufferDesc {
    unsigned long dwSize;         /* +0x00 */
    unsigned long dwFlags;        /* +0x04 */
    unsigned long dwBufferBytes;  /* +0x08 */
    unsigned long dwReserved;     /* +0x0c */
    void*         lpwfxFormat;    /* +0x10 */
    char          guid3D[16];     /* +0x14 */
} DSBufferDesc;
#define DSBCAPS_CTRLFREQUENCY 0x20
#define DSBCAPS_CTRLPAN       0x40
#define DSBCAPS_CTRLVOLUME    0x80
#define DSBSTATUS_PLAYING     0x01

extern int      g_samples_ready;   /* 0x007988c0 */
extern Sample*  g_playable_list;   /* 0x007988cc */
extern IDSound* g_dsound;          /* 0x007cad40 */

/* The most recent AVI Lock's two write regions (audio2.c hands them back). */
extern void*         g_avi_lock_ptr1;  /* 0x007988a4 */
extern unsigned long g_avi_lock_len1;  /* 0x00798898 */
extern void*         g_avi_lock_ptr2;  /* 0x007988a8 */
extern unsigned long g_avi_lock_len2;  /* 0x0079889c */

extern int  FreePlayableSample(Sample* s);                          /* 0x00492b20 (internal) */
extern void UnSourceAndFadeAllSamplesFromSource(SoundSource* src, int fade); /* 0x00496c80 */

/* A packed map cell position passed BY VALUE in one argument slot. */
typedef struct CellPos {
    unsigned char x;   /* +0x00 */
    unsigned char y;   /* +0x01 */
} CellPos;
extern void StandardRemoveObject(void* obj, CellPos pos, int c);   /* 0x0045f220 */

/* ======================================================================== */
/* Window procedure                                                         */
/* ======================================================================== */

/* Messages handled: WM_CLOSE (quit), WM_CHAR backspace (reset the edit
 * cursor), WM_KILLFOCUS / WM_CANCELMODE (freeze the game clock, suspend) and
 * WM_SETFOCUS (thaw unless it was frozen before we lost focus, resume).
 * Everything else goes to DefWindowProc. Source case order fixes the block
 * layout. */
// FUNCTION: LEGOLAND 0x0047fe90
long __stdcall LegoLandWindowProc(void* hwnd, unsigned int msg, unsigned int wp, long lp)
{
    switch (msg) {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    case WM_CHAR:
        if ((char)wp == 8) {
            g_input_cfg.flags &= ~0x400;
            g_edit_state = 2;
            DefaultCursor(g_edit_cursor);
        }
        return 0;
    case WM_KILLFOCUS:
    case WM_CANCELMODE:
        g_clock_was_frozen = FreezeGameClock();
        g_game->flags |= 1;
        break;
    case WM_SETFOCUS:
        if (!g_clock_was_frozen)
            ThawGameClock();
        g_game->flags &= ~1;
        break;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

/* ======================================================================== */
/* Input bring-up                                                           */
/* ======================================================================== */

/* Create the DirectInput object (DIRECTINPUT_VERSION 0x300) then the two
 * devices. The DirectInputCreateA result is not checked. */
// FUNCTION: LEGOLAND 0x00473870
int InitInputSystem(void)
{
    void* hinst = (void*)GetWindowLongA(WNDENV_Gethwnd(), GWL_HINSTANCE);

    DirectInputCreateA(hinst, 0x300, &g_dinput, 0);
    if (!CreateKeyboardDevice())
        return 0;
    return CreateMouseDevice() != 0;
}

/* Allocate the controller record and fill in the input configuration
 * defaults. Idempotent. */
// FUNCTION: LEGOLAND 0x00451e70
int SetupControllers(void)
{
    if (g_controllers_ready)
        return 1;
    g_controller = HeapAlloc_w(0x28);
    g_controller->buttons = 0;
    g_input_cfg.flags = 0;
    g_input_cfg.f0c = 1;
    g_input_cfg.f14 = 4;
    g_input_cfg.f1c = 2;
    g_input_cfg.mask_btn0 = 0x01;
    g_input_cfg.mask_btn1 = 0x02;
    g_input_cfg.mask_tab = 0x100;
    g_input_cfg.mask_esc = 0x200;
    g_input_cfg.mask_up = 0x20;
    g_input_cfg.mask_right = 0x10;
    g_input_cfg.mask_down = 0x40;
    g_input_cfg.mask_left = 0x08;
    g_input_cfg.mask_return = 0x400;
    g_input_cfg.f24 = 10;
    g_input_cfg.f28 = 4;
    g_input_cfg.f2c = 1;
    g_input_cfg.f30 = 1;
    if (g_controller) {
        g_controllers_ready = 1;
        return 1;
    }
    return 0;
}

/* Return the character of the key that went down since the last poll (0 if
 * none; the last map entry wins). Letters are lower-cased unless CAPS LOCK
 * xor SHIFT says otherwise. Returns `char` (that is what keeps the result in
 * eax across the loop and lets VC6 hoist the GetKeyState import into ebp). */
// FUNCTION: LEGOLAND 0x004740b0
char GetInputChar(void)
{
    int           result = 0;
    int           i;
    int           prev;
    int           caps;
    unsigned char cur;

    for (i = 0; i < KEYMAP_COUNT; i++) {
        prev = (unsigned)(g_key_prev[i] & 0x80) >> 7;
        cur = g_key_state[g_key_map[i].dik];
        g_key_prev[i] = cur;
        if ((cur & 0x80) && !prev) {
            caps = GetKeyState(VK_CAPITAL) & 1;
            if (IsLShiftDown() || IsRShiftDown())
                caps = !caps;
            result = g_key_map[i].ch;
            if (!caps)
                result = tolower(result);
        }
    }
    return result;
}

/* ======================================================================== */
/* Sample playback                                                          */
/* ======================================================================== */

/* Rewind and start a live instance; `loop` sets flag bit 2, `fade` bit 3. */
// FUNCTION: LEGOLAND 0x00492710
int PlaySample(Sample* s, int loop, int fade)
{
    if (!g_samples_ready)
        return 0;
    if (!s)
        return 0;
    if (!s->def)
        return 0;
    if (s->buf->lpVtbl->SetCurrentPosition(s->buf, 0) != 0)
        return 0;
    if (loop) {
        if (s->buf->lpVtbl->Play(s->buf, 0, 0, 1) != 0)
            return 0;
        s->flags |= 4;
    } else {
        if (s->buf->lpVtbl->Play(s->buf, 0, 0, 0) != 0)
            return 0;
        s->flags &= ~4;
    }
    if (fade) {
        s->flags |= 8;
        return 1;
    }
    s->flags &= ~8;
    return 1;
}

/* Object-removal callback for sound-emitting objects: do the standard cell
 * teardown, then fade out (-200) every sample sourced at that map cell. */
// FUNCTION: LEGOLAND 0x00452a30
void RemoveSoundObject(void* obj, CellPos pos, int c)
{
    SoundSource src;

    StandardRemoveObject(obj, pos, c);
    src.kind = 2;
    src.pos.x = pos.x;
    src.pos.y = pos.y;
    UnSourceAndFadeAllSamplesFromSource(&src, -200);
}

/* Kill (unlink + free) every live instance sourced at `src`; the source
 * kinds match CountSamplesFromSource. `match` is left uninitialised for a
 * kind above 3, as in the original (VC6 homes it in the dead `src` argument
 * slot). The for(;; s = next) form is what puts `match` in esi and `next` in
 * edi; a while loop with `s = next` at the bottom swaps them.
 *
 * All 59 body instructions match (matchfull 59/62 -- the 3 residual rows are
 * the .rdata jump table decoded as code). Held as WIP for tooling only:
 * VC6 tail-duplicates the head-unlink `g_playable_list = next; Free(s)` block
 * AFTER the final ret (reached by `je`), so tools/audit.py's extent walk runs
 * on through the jump table into UnSourceAndFadeSample (97i/256B) and can
 * neither bound the original nor certify it. */
// FUNCTION: LEGOLAND 0x00496b80
void KillAllSamplesFromSource(SoundSource* src)
{
    Sample* s;
    Sample* prev = 0;
    Sample* next;
    int     match;

    for (s = g_playable_list; s; s = next) {
        next = s->next;
        if (s->src.kind == src->kind) {
            switch (src->kind) {
            case 0:
                match = 1;
                break;
            case 1:
                match = s->src.obj == src->obj;
                break;
            case 2:
            case 3:
                match = s->src.pos.x == src->pos.x && s->src.pos.y == src->pos.y;
                break;
            }
            if (match) {
                if (prev)
                    prev->next = next;
                else
                    g_playable_list = next;
                FreePlayableSample(s);
                continue;
            }
        }
        prev = s;
    }
}

/* ======================================================================== */
/* AVI sound buffer                                                         */
/* ======================================================================== */

/* A volume/pan/frequency-controllable secondary buffer of `bytes` bytes in
 * the given WAVEFORMATEX; NULL on failure. */
// FUNCTION: LEGOLAND 0x00496360
IDSBuffer* KLIBAUDIO_CreateAVISoundBuffer(void* fmt, unsigned long bytes)
{
    DSBufferDesc desc;
    IDSBuffer*   buf;
    long         hr;

    if (g_samples_ready) {
        buf = 0;
        desc.dwSize = sizeof(DSBufferDesc);
        desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY;
        desc.dwBufferBytes = bytes;
        desc.dwReserved = 0;
        desc.lpwfxFormat = fmt;
        hr = g_dsound->lpVtbl->CreateSoundBuffer(g_dsound, &desc, &buf, 0);
        return hr == 0 ? buf : 0;
    }
    return 0;
}

/* Lock [offset, offset+len) of the streaming buffer, first spinning until the
 * play cursor has left that window (only while the buffer is playing). The
 * lock regions land in the g_avi_lock_* globals for UnLockAVISoundBuffer;
 * returns the first region's pointer or NULL. The write-cursor out-pointer is
 * homed in the dead `buf` argument slot. */
// FUNCTION: LEGOLAND 0x004963f0
void* KLIBAUDIO_LockAVISoundBuffer(IDSBuffer* buf, unsigned long offset, unsigned long len)
{
    unsigned long status;
    unsigned long play;
    unsigned long write;
    long          hr;

    if (!buf)
        return 0;
    hr = buf->lpVtbl->GetStatus(buf, &status);
    if (hr == 0 && (status & DSBSTATUS_PLAYING)) {
        hr = buf->lpVtbl->GetCurrentPosition(buf, &play, &write);
        while (hr == 0 && play >= offset && play < offset + len)
            hr = buf->lpVtbl->GetCurrentPosition(buf, &play, &write);
    }
    hr = buf->lpVtbl->Lock(buf, offset, len, &g_avi_lock_ptr1, &g_avi_lock_len1,
                           &g_avi_lock_ptr2, &g_avi_lock_len2, 0);
    return hr == 0 ? g_avi_lock_ptr1 : 0;
}
