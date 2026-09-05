/* LEGOLAND — assorted system-level helpers: display mode and page flipping,
 * resource mounting, positional audio, mouse input creation, the 3D-person
 * screen-position update and the LLIDB .LLS unloader.
 *
 * Seven self-contained functions from five subsystems that had no natural home
 * in an existing translation unit. Reconstructed from original/legoland.exe
 * (VC6 SP3, /O2 /Gy /Gd); only struct field OFFSETS, vtable slots and callee
 * argument counts are load-bearing — every name here is ours.
 */
#include "legoland.h"

/* ================================================================= audio == */

/* The Sample record, as audio2.c/audio3.c recovered it. Only the fields this
 * file touches are named. +0x0c is a 16-byte SoundSource written as a unit by
 * the SourcePlayableSampleTo* family. */
typedef struct MapRef {
    int x;                      /* +0x00 */
    int y;                      /* +0x04 */
} MapRef;

typedef struct SoundSource {
    int    kind;                /* +0x00  0 none / 1 bloke / 2 map ref / 3 level xy */
    void*  obj;                 /* +0x04  the bloke, for kind 1 */
    MapRef pos;                 /* +0x08 */
} SoundSource;

typedef struct Sample {
    struct Sample* next;        /* +0x00 */
    int            refcount;    /* +0x04 */
    int            fade;        /* +0x08 */
    SoundSource    src;         /* +0x0c */
    unsigned short flags;       /* +0x1c */
    short          pad1e;       /* +0x1e */
    unsigned int   due;         /* +0x20 */
    void*          callback;    /* +0x24 */
    struct Sample* def;         /* +0x28 */
    void*          buf;         /* +0x2c */
    void*          data30;      /* +0x30 */
    void*          data34;      /* +0x34 */
} Sample;

/* The renderable 3D person; +0x1c/+0x20 is its screen position (anim2.c). */
typedef struct Person3D {
    unsigned char pad00[0x1c];  /* +0x00..0x1b */
    Pos           screen;       /* +0x1c  screen position */
} Person3D;

/* A bloke, as the audio source cares about it. */
typedef struct AudioBloke {
    void*      pad00;           /* +0x00 */
    Person3D*  person;          /* +0x04 */
} AudioBloke;

/* ---- globals ---- */
extern int g_samples_ready;     /* 0x007988c0  sample system up */
extern int g_scroll_x;          /* 0x00667cb4  ScrollX (24.8 fixed point) */
extern int g_scroll_y;          /* 0x00667cb8  ScrollY (24.8 fixed point) */

/* ---- callees ---- */
extern int  ClearSampleSource(Sample* s);                  /* 0x00496660 (internal) */
extern void GetTileCentre(Pos* tile, Pos* out);            /* 0x0045ad60 */
/* First named here: takes the sample's world position, subtracts half the
 * screen size (read through 0x004bcbf4) and pushes the resulting pan/volume
 * into the DirectSound buffer. */
extern int  SetSampleScreenPos(Sample* s, int x, int y);   /* 0x004965a0 (internal) */

/* Recompute a playing sample's stereo position from whatever it is sourced to.
 *
 * Note the missing `default:` — when SoundSource::kind is not 0..3 the local
 * position is passed to SetSampleScreenPos UNINITIALISED (it is whatever the
 * previous frame left in those two stack slots). Reproduced as in the original;
 * the sourcing entry points only ever store 0..3. */
/* RESIDUAL: 6 of 66. In case 3 the original emits the three loads in the order
 * scroll_x / pos.x / pos.y; ours comes out pos.y / scroll_x / pos.x. The free
 * volatile read is what keeps the pos.y load on the near side of the p.x store
 * (without it the load sinks below it, 12 mismatches), but it also pins that
 * load FIRST in the block. The following sar/sub/store differ only in register
 * naming, which follows. Scheduling residual; the tail and cases 0-2 are exact. */
/* Scope I (2026-09-05): at its measured scheduling/tail-merge floor,
 * 6/66 strict (rb 3, ob 6), first 46, 175/175 bytes. Ten additional ordered
 * input/snapshot/aggregate spellings were measured. Three ordered volatile
 * reads get the desired input order but rotate the final y out of eax and
 * lose the case-1 tail merge (28 strict). Ordinary pair accumulation/copy,
 * input and scroll pairs, and named accumulators give 27-32. Keep this form;
 * changing the shared tail to improve one arm regresses the other.
 * Full measurements: docs/lanes/scope-i.md.
 */
// WIP-FUNCTION: LEGOLAND 0x004966a0  (90.9%, 6/66 strict; scheduling/tail-merge floor; first 46)
int UpdateSampleSource(Sample* s)
{
    Pos p;

    if (!g_samples_ready)
        return 0;
    if (!s)
        return 0;
    if (!s->def)
        return 0;

    switch (s->src.kind) {
    case 0:
        return ClearSampleSource(s);
    case 1:
        {
            AudioBloke* b = (AudioBloke*)s->src.obj;
            p = b->person->screen;
        }
        break;
    case 2:
        GetTileCentre((Pos*)&s->src.pos, &p);
        break;
    case 3:
        {
            int py = *(int volatile*)&s->src.pos.y;
            *(int volatile*)&p.x = s->src.pos.x - (g_scroll_x >> 8);
            *(int volatile*)&p.y = py - (g_scroll_y >> 8);
        }
        break;
    }
    return SetSampleScreenPos(s, p.x, p.y);
}

/* ================================================================= input == */

/* DirectInput, offsets only (the same shape input.c uses). */
typedef struct DIDevice DIDevice;
typedef struct DIDeviceVtbl {
    long (__stdcall* QueryInterface)(DIDevice*, const void*, void**);       /* +0x00 */
    long (__stdcall* AddRef)(DIDevice*);                                    /* +0x04 */
    long (__stdcall* Release)(DIDevice*);                                   /* +0x08 */
    long (__stdcall* GetCapabilities)(DIDevice*, void*);                    /* +0x0c */
    long (__stdcall* EnumObjects)(DIDevice*, void*, void*, unsigned long);  /* +0x10 */
    long (__stdcall* GetProperty)(DIDevice*, const void*, void*);           /* +0x14 */
    long (__stdcall* SetProperty)(DIDevice*, const void*, const void*);     /* +0x18 */
    long (__stdcall* Acquire)(DIDevice*);                                   /* +0x1c */
    long (__stdcall* Unacquire)(DIDevice*);                                 /* +0x20 */
    long (__stdcall* GetDeviceState)(DIDevice*, unsigned long, void*);      /* +0x24 */
    long (__stdcall* GetDeviceData)(DIDevice*, unsigned long, void*,
                                    unsigned long*, unsigned long);         /* +0x28 */
    long (__stdcall* SetDataFormat)(DIDevice*, const void*);                /* +0x2c */
    long (__stdcall* SetEventNotification)(DIDevice*, void*);               /* +0x30 */
    long (__stdcall* SetCooperativeLevel)(DIDevice*, void*, unsigned long); /* +0x34 */
} DIDeviceVtbl;
struct DIDevice { DIDeviceVtbl* lpVtbl; };

/* IDirectInputA — only the four slots this file uses. */
typedef struct DInput DInput;
typedef struct DInputVtbl {
    long (__stdcall* QueryInterface)(DInput*, const void*, void**);         /* +0x00 */
    long (__stdcall* AddRef)(DInput*);                                      /* +0x04 */
    long (__stdcall* Release)(DInput*);                                     /* +0x08 */
    long (__stdcall* CreateDevice)(DInput*, const void*, DIDevice**, void*);/* +0x0c */
    long (__stdcall* EnumDevices)(DInput*, unsigned long, void*, void*,
                                  unsigned long);                           /* +0x10 */
    long (__stdcall* GetDeviceStatus)(DInput*, const void*);                /* +0x14 */
} DInputVtbl;
struct DInput { DInputVtbl* lpVtbl; };

typedef struct DIPropHeader {
    unsigned long dwSize;        /* +0x00 */
    unsigned long dwHeaderSize;  /* +0x04 */
    unsigned long dwObj;         /* +0x08 */
    unsigned long dwHow;         /* +0x0c */
} DIPropHeader;

typedef struct DIPropDword {
    DIPropHeader  diph;          /* +0x00 */
    unsigned long dwData;        /* +0x10 */
} DIPropDword;                   /* 0x14 */

/* DIDEVCAPS, DirectX 3 size (0x2c). */
typedef struct DIDevCaps {
    unsigned long dwSize;        /* +0x00 */
    unsigned long dwFlags;       /* +0x04  DIDC_* — 0 means "no device" */
    unsigned long dwDevType;     /* +0x08 */
    unsigned long dwAxes;        /* +0x0c */
    unsigned long dwButtons;     /* +0x10 */
    unsigned long dwPOVs;        /* +0x14 */
    unsigned long pad18[5];      /* +0x18..0x2b */
} DIDevCaps;                     /* 0x2c */

#define DIPH_BYOFFSET        1
#define DIPROP_GRANULARITY   ((const void*)3)   /* MAKEDIPROP(3) */
#define DIMOFS_Z             8                  /* offsetof(DIMOUSESTATE, lZ) */
#define DISCL_MOUSE          5                  /* EXCLUSIVE | FOREGROUND */

extern DInput*   g_dinput;             /* 0x00668d88  IDirectInputA */
extern DIDevice* g_di_mouse;           /* 0x00668d90 */
extern int       g_wheel_granularity;  /* 0x004bad54  DIPROP_GRANULARITY of lZ */

/* dinput.lib data, statically linked into the image. */
extern const char GUID_SysMouse[16];   /* 0x004ac0a0 */
extern const char c_dfDIMouse[];       /* 0x004ab578  DIDATAFORMAT */

extern void* WNDENV_Gethwnd(void);     /* 0x0047fe60 */

/* Create and configure the DirectInput mouse. Sibling of CreateKeyboardDevice
 * (0x004738b0), which is the same body without the granularity query.
 *
 * The DIDEVCAPS test is spelled `((dwFlags == 0) & 1) == 0`: the original
 * materialises the comparison as a boolean (`xor eax,eax / test edx,edx /
 * sete al`) and then branches on `test al,1`, which a plain `dwFlags != 0`
 * never emits (that gives a bare `test edx,edx / je`, three instructions
 * short). The `& 1` is what turns the byte test into a bit-0 test, and the
 * `== 0` is what keeps the success arm inline with the failure jumping to the
 * ONE trailing `return 0` — written the other way round VC6 inverts the branch
 * and parks the failure block in the middle. */
// FUNCTION: LEGOLAND 0x00473970
int CreateMouseDevice(void)
{
    DIPropDword prop;
    DIDevCaps   caps;

    if (g_dinput->lpVtbl->CreateDevice(g_dinput, GUID_SysMouse, &g_di_mouse, 0))
        goto fail;

    g_di_mouse->lpVtbl->SetCooperativeLevel(g_di_mouse, WNDENV_Gethwnd(), DISCL_MOUSE);
    g_di_mouse->lpVtbl->SetDataFormat(g_di_mouse, c_dfDIMouse);

    prop.diph.dwSize = sizeof(DIPropDword);
    prop.diph.dwHeaderSize = sizeof(DIPropHeader);
    prop.diph.dwObj = DIMOFS_Z;
    prop.diph.dwHow = DIPH_BYOFFSET;
    if (g_di_mouse->lpVtbl->GetProperty(g_di_mouse, DIPROP_GRANULARITY, &prop.diph) == 0)
        g_wheel_granularity = prop.dwData;

    caps.dwSize = sizeof(DIDevCaps);
    g_di_mouse->lpVtbl->GetCapabilities(g_di_mouse, &caps);
    if (((caps.dwFlags == 0) & 1) == 0)
        return g_dinput->lpVtbl->GetDeviceStatus(g_dinput, GUID_SysMouse) == 0;
fail:
    return 0;
}

/* ================================================================ display == */

typedef struct DDSurface DDSurface;
typedef struct DDraw2    DDraw2;

/* DDPIXELFORMAT, 0x20 bytes. */
typedef struct DDPixelFormat {
    unsigned long dwSize;            /* +0x00 */
    unsigned long dwFlags;           /* +0x04 */
    unsigned long dwFourCC;          /* +0x08 */
    unsigned long dwRGBBitCount;     /* +0x0c */
    unsigned long dwRBitMask;        /* +0x10 */
    unsigned long dwGBitMask;        /* +0x14 */
    unsigned long dwBBitMask;        /* +0x18 */
    unsigned long dwRGBAlphaBitMask; /* +0x1c */
} DDPixelFormat;

/* DDSURFACEDESC, 0x6c bytes; this file only reads the pixel format back. */
typedef struct DDSurfaceDesc {
    unsigned long dwSize;              /* +0x00 */
    char          pad04[0x48 - 0x04];  /* +0x04 */
    DDPixelFormat ddpf;                /* +0x48 */
    unsigned long dwCaps;              /* +0x68 ddsCaps.dwCaps */
} DDSurfaceDesc;

/* IDirectDraw2: GetDisplayMode 0x30, SetDisplayMode 0x54. */
typedef struct DDraw2Vtbl {
    char pad00[0x30];
    long(__stdcall* GetDisplayMode)(DDraw2*, DDSurfaceDesc*);                  /* +0x30 */
    char pad34[0x54 - 0x34];
    long(__stdcall* SetDisplayMode)(DDraw2*, unsigned long w, unsigned long h,
                                    unsigned long bpp, unsigned long refresh,
                                    unsigned long flags);                      /* +0x54 */
} DDraw2Vtbl;
struct DDraw2 { DDraw2Vtbl* vtbl; };

/* The 0x004bcbf4 record. screen.c calls it ScreenCfg, gpu.c Screen, anim2.c
 * MapHdr; this file needs the screen extent, the cursor flag and the screen
 * origin of cell (0,0), so it declares all three views as ONE struct. */
typedef struct ScreenCfg {
    unsigned short w;           /* +0x00 */
    unsigned short h;           /* +0x02 */
    unsigned char  pad04[0x1e - 0x04];
    unsigned short cursor;      /* +0x1e  0 = do not draw the game cursor */
    unsigned short origin_x;    /* +0x20  screen origin of cell (0,0) */
    unsigned short origin_y;    /* +0x22 */
} ScreenCfg;

extern ScreenCfg* g_screencfg;    /* 0x004bcbf4 */
extern int        g_windowed;     /* 0x00667d6c */
extern DDraw2*    g_ddraw;        /* 0x00667d74 */
/* 0 = 8bpp palettised, 1 = 16bpp 555, 2 = 16bpp 565 (data2.c). */
extern int        g_screen_depth; /* 0x00668088 */

/* Take the display to the game's resolution (full screen only) and record the
 * pixel format that actually came back. */
// FUNCTION: LEGOLAND 0x00463ef0
int SetScreenDisplayMode(void)
{
    DDSurfaceDesc desc;

    if (!g_windowed) {
        if (g_ddraw->vtbl->SetDisplayMode(g_ddraw, g_screencfg->w, g_screencfg->h,
                                          16, 0, 0) != 0) {
            if (g_ddraw->vtbl->SetDisplayMode(g_ddraw, g_screencfg->w, g_screencfg->h,
                                              8, 0, 0) != 0)
                return 0;
        }
    }

    desc.dwSize = sizeof(DDSurfaceDesc);
    g_ddraw->vtbl->GetDisplayMode(g_ddraw, &desc);

    switch (desc.ddpf.dwRGBBitCount) {
    case 8:
        g_screen_depth = 0;
        return 1;
    case 16:
        g_screen_depth = (desc.ddpf.dwGBitMask == 0x7e0) + 1;
        return 1;
    }
    return 0;
}

/* ================================================================= people == */

/* The 3D person record (0x94 bytes); only the fields this function writes. */
typedef struct BlokePerson {
    unsigned char pad00[0x4c];  /* +0x00..0x4b */
    int           frame;        /* +0x4c  frame index into the animation */
    unsigned char pad50[4];     /* +0x50 */
    int           depth;        /* +0x54  print-list sort key */
} BlokePerson;

/* A bloke (0xac bytes); the walk position and render state live at +0x68. */
typedef struct WalkBloke {
    unsigned char  pad00[0x62]; /* +0x00..0x61 */
    unsigned short flags62;     /* +0x62  bit 0x100 = keep the current frame */
    unsigned char  pad64[4];    /* +0x64..0x67 */
    int            x;           /* +0x68  map position, 24.8 */
    int            y;           /* +0x6c */
    unsigned short height;      /* +0x70  sprite height, in pixels */
    unsigned char  dir;         /* +0x72  heading, 0..7 */
    unsigned char  pad73;       /* +0x73 */
    unsigned char  frame;       /* +0x74  animation frame */
} WalkBloke;

extern void  SetPersonDirection(BlokePerson* p, unsigned int dir); /* 0x004400b0 */
extern void  GetTileDimensions(int* out_w, int* out_h);            /* 0x00460540 */
extern short Get_XScroll(void);                                    /* 0x004615f0 */
extern short Get_YScroll(void);                                    /* 0x00461600 */
extern void  AdjustBlokePosition(Pos* p);                          /* 0x00442d60 */
extern void  SetPersonPosition(BlokePerson* p, int x, int y);      /* 0x00440190 */

/* Project a bloke's 24.8 map position onto the screen and push it into its 3D
 * person: isometric transform, scroll, the map origin and a half-height lift,
 * then the sprite fudge AdjustBlokePosition applies to every walking bloke. */
/* Exact, Scope I (2026-09-05): the two unscaled isometric coordinates are
 * one Pos local. Defining both components before their scaled stores gives
 * the original lea ecx,[ebx+ebp], rather than destroying bx with add ebp,ebx.
 * With that source shape, ordinary pos.x -= Get_XScroll() also reproduces
 * the original scroll loads; the former volatile read was compensating for
 * the wrong coordinate web and must be removed. 74 instructions / 213 bytes.
 */
// FUNCTION: LEGOLAND 0x004401b0
void UpdatePersonPos(BlokePerson* p, WalkBloke* b)
{
    Pos pos;
    Pos projected;
    int tw, th;
    int by, bx;

    SetPersonDirection(p, b->dir);
    by = b->y;
    bx = b->x;
    GetTileDimensions(&tw, &th);
    projected.x = bx - by;
    projected.y = by + bx;
    pos.x = (projected.x * tw) >> 9;
    pos.y = (projected.y * th) >> 9;
    pos.x -= Get_XScroll();
    pos.y -= Get_YScroll();
    p->depth = pos.y;
    pos.x += g_screencfg->origin_x;
    pos.y += g_screencfg->origin_y - (b->height >> 1);
    AdjustBlokePosition(&pos);
    SetPersonPosition(p, pos.x, pos.y);
    if (!(b->flags62 & 0x100))
        p->frame = b->frame;
}

/* =============================================================== resources == */

extern int  RES_FindVolumeOnAnyDrive(const char* vol);  /* 0x00450f30 (internal) */
extern int  RES_FindVolumeOnResPath(const char* vol);   /* 0x004510e0 (internal) */
extern void DebugPrintf(const char* fmt, ...);          /* 0x0047f870 */
extern void DebugFlush(void);                           /* 0x0047f850 */
extern void WNDENV_Minimise(void);                      /* 0x0047fe70  ShowWindow(SW_MINIMIZE) */
extern void WNDENV_Restore(void);                       /* 0x0047fe80  ShowWindow(SW_RESTORE) */
extern int  sprintf(char* buf, const char* fmt, ...);   /* 0x0049e573 (CRT) */

__declspec(dllimport) int __stdcall MessageBoxA(void* hwnd, const char* text,
                                                const char* caption,
                                                unsigned int type);   /* [0x4ab2a4] */

/* The drive prefix RES_FindVolumeOnResPath probes ("d:\" and friends). */
extern char g_res_path[];                    /* 0x00813b04 */

extern const char kVolLegoland[];            /* 0x004b86d0 "LEGOLAND" */
extern const char kMinimisingGame[];         /* 0x004b86c0 "Minimising Game" */
extern const char kCdMissing[];              /* 0x004b86b4 "CD Missing" */
extern const char kInsertCd[];               /* 0x004b8680 "Please insert the LEGOLAND CD-ROM into the CD drive" */
extern const char kMaximisingGame[];         /* 0x004b8670 "Maximising Game" */
extern const char kInsertCdInDrive[];        /* 0x004b8640 "Please insert the LEGOLAND CD-ROM into drive %s" */

#define MB_INSERT_CD 0x50015   /* RETRYCANCEL|ICONHAND|SETFOREGROUND|TOPMOST */
#define IDCANCEL     2

/* Block until the LEGOLAND CD is in a drive, nagging with a retry/cancel box.
 *
 * ORIGINAL BUG (reproduced): the `volume` parameter's VALUE is never used —
 * it only selects which prober runs, and both branches probe the hard-coded
 * volume name "LEGOLAND". res.c's only caller passes 0, so the drive-specific
 * branch is the one that runs in the shipped game.
 *
 * Both arms are written out IN FULL, down to their own copy of the retry
 * box's cancel handler and their own `if (minimised) ... return 1;` tail.
 * That is what the layout proves: VC6 cross-jumps the two cancel bodies into
 * ONE copy parked after the first loop and the two restore bodies into ONE
 * copy parked after the second, while each arm keeps its own inline
 * `test edi,edi / je` — a shared tail written once after the if/else instead
 * places the tail between the arms and loses both of those tests (90 of 92). */
// FUNCTION: LEGOLAND 0x004515e0
int RES_EnsureMounted(const char* volume)
{
    char msg[256];
    int  minimised = 0;

    if (volume) {
        while (!RES_FindVolumeOnAnyDrive(kVolLegoland)) {
            if (!minimised) {
                DebugPrintf(kMinimisingGame);
                DebugFlush();
                WNDENV_Minimise();
                minimised = 1;
            }
            if (MessageBoxA(WNDENV_Gethwnd(), kInsertCd, kCdMissing,
                            MB_INSERT_CD) == IDCANCEL) {
                DebugPrintf(kMaximisingGame);
                DebugFlush();
                WNDENV_Restore();
                return 0;
            }
        }
        if (minimised) {
            DebugPrintf(kMaximisingGame);
            DebugFlush();
            WNDENV_Restore();
        }
        return 1;
    } else {
        while (!RES_FindVolumeOnResPath(kVolLegoland)) {
            if (!minimised) {
                DebugPrintf(kMinimisingGame);
                DebugFlush();
                WNDENV_Minimise();
                minimised = 1;
            }
            sprintf(msg, kInsertCdInDrive, g_res_path);
            if (MessageBoxA(WNDENV_Gethwnd(), msg, kCdMissing,
                            MB_INSERT_CD) == IDCANCEL) {
                DebugPrintf(kMaximisingGame);
                DebugFlush();
                WNDENV_Restore();
                return 0;
            }
        }
        if (minimised) {
            DebugPrintf(kMaximisingGame);
            DebugFlush();
            WNDENV_Restore();
        }
        return 1;
    }
}

/* =============================================================== the flip == */

typedef struct WinRect {
    long left;      /* +0x00 */
    long top;       /* +0x04 */
    long right;     /* +0x08 */
    long bottom;    /* +0x0c */
} WinRect;

typedef struct WinPoint {
    long x;         /* +0x00 */
    long y;         /* +0x04 */
} WinPoint;

/* IDirectDrawSurface: Blt 0x14, Restore 0x6c (gpu.c). */
typedef struct DDSurfaceVtbl {
    char pad00[0x14];
    long(__stdcall* Blt)(DDSurface*, WinRect*, DDSurface*, WinRect*,
                         unsigned long, void*);                    /* +0x14 */
    char pad18[0x6c - 0x18];
    long(__stdcall* Restore)(DDSurface*);                          /* +0x6c */
} DDSurfaceVtbl;
struct DDSurface { DDSurfaceVtbl* vtbl; };

#define DDBLT_WAIT            0x01000000
#define DDERR_SURFACELOST     0x887601c2

extern DDSurface* g_primary;        /* 0x00668070 */
extern DDSurface* g_surface_78;     /* 0x00668078  what the frame was drawn into */
extern void*      g_current_pointer;/* 0x00668148  the cursor sprite (sweep2.c) */
extern Pos        g_gfx_point;      /* 0x00813a44  cursor point */
/* screen.c calls 0x007cacd4 g_init_flag because InitScreen zeroes it; it is
 * the frame counter this function bumps. Named for what it does here. */
extern unsigned int g_frame_count;  /* 0x007cacd4 */
extern unsigned int g_fps_base;     /* 0x006681f0  start of the current fps second */
extern unsigned int g_last_frame;   /* 0x006681f4  timeGetTime at the last flip */
extern unsigned int g_fps_frames;   /* 0x006681f8  frames since g_fps_base */
extern unsigned int g_frame_ticks;  /* 0x006681fc  ticks this frame (pathtile2.c) */
extern unsigned int g_fps;          /* 0x007fea48  last completed second's count */
extern unsigned int g_flip_time;    /* 0x00668200  timeGetTime at the last blit */

extern void LLSAuto(void);                                    /* 0x0047d630 */
extern void PushRenderingStatusAndLockVideoSurface(void);     /* 0x00463fc0 */
extern void PopRenderingStatus(void);                         /* 0x004641f0 */
extern int  PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */

__declspec(dllimport) unsigned int __stdcall timeGetTime(void);            /* [0x4ab1f8] */
__declspec(dllimport) int __stdcall GetClientRect(void* hwnd, WinRect* rc); /* [0x4ab2ec] */
__declspec(dllimport) int __stdcall ClientToScreen(void* hwnd, WinPoint* p);/* [0x4ab2dc] */
__declspec(dllimport) int __stdcall OffsetRect(WinRect* rc, int dx, int dy);/* [0x4ab29c] */

/* Present the frame: run the animation pump, stamp the cursor on, wait out the
 * 28 ms frame floor, then blit the back surface onto the primary at the
 * window's screen position. Installed as g_present (0x004b9ca4) in full
 * screen; the windowed build uses a different presenter. */
// FUNCTION: LEGOLAND 0x004661d0
int FlipPrimary(void)
{
    WinRect dst = { 0, 0, 640, 480 };
    WinRect rc;
    unsigned int now;
    long hr;

    LLSAuto();

    if (g_screencfg->cursor && g_current_pointer) {
        PushRenderingStatusAndLockVideoSurface();
        PrintSprite(g_current_pointer, g_gfx_point.x, g_gfx_point.y, 0, 0);
        PopRenderingStatus();
    }

    while (timeGetTime() - g_flip_time < 0x1c)
        ;
    g_flip_time = timeGetTime();

    GetClientRect(WNDENV_Gethwnd(), &rc);
    ClientToScreen(WNDENV_Gethwnd(), (WinPoint*)&rc);
    OffsetRect(&dst, rc.left, rc.top);

    hr = g_primary->vtbl->Blt(g_primary, &dst, g_surface_78, 0, DDBLT_WAIT, 0);
    if (hr == (long)DDERR_SURFACELOST) {
        g_primary->vtbl->Restore(g_primary);
        hr = g_primary->vtbl->Blt(g_primary, &dst, g_surface_78, 0, DDBLT_WAIT, 0);
    }
    if (hr != 0)
        return 0;

    now = timeGetTime();
    g_frame_count++;
    g_fps_frames++;
    if (now - g_fps_base >= 1000) {
        g_fps = g_fps_frames;
        g_fps_frames = 0;
        g_fps_base = now;
    }
    g_frame_ticks = now - g_last_frame;
    g_last_frame = now;
    return 1;
}

/* ================================================================== LLIDB == */

/* An image record: a .lls animation hangs off +0x00, and +0x14 is its kind
 * (2 and 3 are the two animated kinds, which own a playing LLS). */
typedef struct LLSImage {
    void* lls;                  /* +0x00 */
    char  pad04[0x14 - 0x04];   /* +0x04 */
    int   kind;                 /* +0x14 */
} LLSImage;

/* A sprite, as this file reads it: its image sits at +0x08. */
typedef struct LLSSprite {
    char      pad00[8];         /* +0x00 */
    LLSImage* image;            /* +0x08 */
} LLSSprite;

/* The object-class record an LLIDB element of type 0x10 / 0x1010 parses into
 * (memdb.c's LLIDB_UnLoadData dispatches both codes here). It is a node of
 * the ObjectClassList at 0x00669240, linked through +0x00. */
typedef struct ObjClassRec {
    struct ObjClassRec* next;   /* +0x00  ObjectClassList link */
    char          pad04[0x1c - 0x04];
    unsigned int  flags;        /* +0x1c  bit 0x10000 = owns an object library */
    char          pad20[0x64 - 0x20];
    LLSSprite*    sprite0;      /* +0x64 */
    LLSSprite*    sprite1;      /* +0x68 */
    LLSSprite*    sprite2;      /* +0x6c */
    void*         tiles;        /* +0x70  a second LLIDB element to unload */
    char          pad74[4];     /* +0x74 */
    void*         block78;      /* +0x78  owned heap blocks */
    void*         block7c;      /* +0x7c */
    void*         block80;      /* +0x80 */
    char          pad84[0xac - 0x84];
    void        (*dtor)(void*); /* +0xac  per-class destructor */
    char          padb0[0xc4 - 0xb0];
    void*         dtor_arg;     /* +0xc4 */
} ObjClassRec;

extern ObjClassRec* g_objclass_head;                  /* 0x00669240 ObjectClassList */

extern void UnLoadObjectLibrary(ObjClassRec* d);      /* 0x004810f0 */
extern void LLSStop(void* lls);                       /* 0x0047d4c0 */
extern int  KillSprite(LLSSprite* s);                 /* 0x00497bd0 */
extern int  LLIDB_UnLoadData(void* elem);             /* 0x0047d450 */
extern void free(void*);                              /* 0x0049e4d0 (CRT) */

/* Free the object class an LLS-typed LLIDB element parsed into: run its
 * destructor, drop its object library, unlink it from ObjectClassList, kill
 * its three sprites (stopping any animation each one is playing), unload its
 * tile element and free its three owned blocks.
 *
 * Declared int-returning and never setting eax, like the rest of the
 * LLIDB_UnLoad* family: memdb.c's LLIDB_UnLoadData does `return
 * LLIDB_UnLoadLLSData(e);` and passes the garbage straight on. */
// FUNCTION: LEGOLAND 0x0047c6a0
int LLIDB_UnLoadLLSData(LLElem* e)
{
    ObjClassRec* d = (ObjClassRec*)e->data;
    ObjClassRec* p;

    if (d) {
        if (d->dtor)
            d->dtor(d->dtor_arg);

        if (d->flags & 0x10000)
            UnLoadObjectLibrary(d);

        p = g_objclass_head;
        if (p == d) {
            g_objclass_head = d->next;
        } else {
            while (p) {
                if (p->next == d)
                    break;
                p = p->next;
            }
            if (p)
                p->next = d->next;
        }

        if (d->sprite0) {
            LLSImage* im = d->sprite0->image;
            if (im->kind == 2 || im->kind == 3)
                LLSStop(im->lls);
            KillSprite(d->sprite0);
        }
        if (d->sprite1) {
            LLSImage* im = d->sprite1->image;
            if (im->kind == 2 || im->kind == 3)
                LLSStop(im->lls);
            KillSprite(d->sprite1);
        }
        if (d->sprite2) {
            LLSImage* im = d->sprite2->image;
            if (im->kind == 2 || im->kind == 3)
                LLSStop(im->lls);
            KillSprite(d->sprite2);
        }

        if (d->tiles)
            LLIDB_UnLoadData(d->tiles);
        if (d->block78)
            free(d->block78);
        if (d->block7c)
            free(d->block7c);
        if (d->block80)
            free(d->block80);
        free(d);
        e->data = 0;
    }
}
