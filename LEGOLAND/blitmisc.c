/* LEGOLAND -- remaining blit / present / script stubs (scope AD).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Field offsets and calling conventions are load-bearing; names are ours
 * unless a caller already named the address.
 */

#define NAKED __declspec(naked)

typedef struct WinRect {
    long left;
    long top;
    long right;
    long bottom;
} WinRect;

typedef struct Pos {
    int x;
    int y;
} Pos;

typedef struct ScreenCfg {
    unsigned short w;
    unsigned short h;
    unsigned char  pad04[0x1e - 0x04];
    unsigned short cursor;
} ScreenCfg;

typedef struct DDSurfaceDesc {
    unsigned long dwSize;
    unsigned long dwFlags;
    unsigned long dwHeight;
    unsigned long dwWidth;
    long          lPitch;
    unsigned long dwBackBufferCount;
    unsigned long dwMipMapCount;
    unsigned long dwAlphaBitDepth;
    unsigned long dwReserved;
    void*         lpSurface;
    char          pad28[0x6c - 0x28];
} DDSurfaceDesc;

typedef struct DDSurface DDSurface;

typedef struct DDSurfaceVtbl {
    char pad00[0x2c];
    long(__stdcall* Flip)(DDSurface*, DDSurface*, unsigned long);
    char pad30[0x48 - 0x30];
    long(__stdcall* GetFlipStatus)(DDSurface*, unsigned long);
    char pad4c[0x64 - 0x4c];
    long(__stdcall* Lock)(DDSurface*, WinRect*, DDSurfaceDesc*,
                          unsigned long, void*);
    char pad68[0x6c - 0x68];
    long(__stdcall* Restore)(DDSurface*);
    char pad70[0x80 - 0x70];
    long(__stdcall* Unlock)(DDSurface*, void*);
} DDSurfaceVtbl;

struct DDSurface {
    DDSurfaceVtbl* vtbl;
};

/* BITMAPINFOHEADER as AVIStreamGetFrame returns it; pixels follow at +0x28. */
typedef struct DibHeader {
    int size;
    int width;
    int height;
    char pad0c[0x28 - 0x0c];
} DibHeader;

/* bigsim.c's AICat; only the published fields this overlay reads. */
typedef struct AICat {
    int objects;    /* +0x00 */
    int defs;       /* +0x04 */
    int cap;        /* +0x08 */
    int working;    /* +0x0c */
    int salvage;    /* +0x10 */
    int pct;        /* +0x14 */
    int scale;      /* +0x18 */
    int n_objects;  /* +0x1c */
    int n_working;  /* +0x20 */
    int value;      /* +0x24 */
    int staff;      /* +0x28 */
} AICat;

#define DDERR_SURFACELOST      0x887601c2
#define DDERR_SURFACEBUSY      0x887601ae
#define DDERR_WASSTILLDRAWING  0x8876021c
#define DDLOCK_WRITEONLY_WAIT  0x21
#define DDFLIP_WAIT            1
#define DDGFS_ISFLIPDONE       2

extern int           g_dmg_game_timer;   /* 0x00667d54 */
extern int           g_dmg_clock;        /* 0x00667d58 */
extern char          g_script_text1[];   /* 0x0066861c */
extern char          g_script_text2[];   /* 0x0066869c */
extern int           g_script_string_count; /* 0x00668720 */
extern char*         g_script_strings[]; /* 0x007fe120 */
extern long          g_ddsd_pitch;       /* 0x006680ac */
extern void*         g_ddsd_bits;        /* 0x006680c0 */
extern int           g_screen_depth;     /* 0x00668088 */
extern int           g_status_stack[];   /* 0x00668164 */
extern int           g_status_sp;        /* 0x006681e4 */
extern int           g_video_locked;     /* 0x00668144 */
extern DDSurface*    g_draw_surface;     /* 0x0066807c */
extern DDSurfaceDesc g_ddsd;             /* 0x0066809c */
extern WinRect       g_render_clip;      /* 0x00668108 */
extern WinRect       g_clip_rect;        /* 0x004bdea0 */
extern int           g_transparent_colour; /* 0x007fea44 */
extern ScreenCfg*    g_screencfg;        /* 0x004bcbf4 */
extern void*         g_current_pointer;  /* 0x00668148 */
extern Pos           g_gfx_point;        /* 0x00813a44 */
extern DDSurface*    g_primary;          /* 0x00668070 */
extern DDSurface*    g_surface_78;       /* 0x00668078 */
extern unsigned int  g_frame_count;      /* 0x007cacd4 */
extern unsigned int  g_fps_base;         /* 0x006681f0 */
extern unsigned int  g_last_frame;       /* 0x006681f4 */
extern unsigned int  g_fps_frames;       /* 0x006681f8 */
extern unsigned int  g_frame_ticks;      /* 0x006681fc */
extern unsigned int  g_fps;              /* 0x007fea48 */
extern unsigned int  g_flip_time;        /* 0x00668200 */
extern AICat         g_ai_cat[6];        /* 0x00832810 */
extern int           g_visitor_limit;    /* 0x0083291c */
extern int           g_visitor_cap;      /* 0x00832920 */
extern int           g_visitor_cap_extra;/* 0x00832924 */
extern const char*   g_capacity_names[6]; /* 0x004bb6bc */
extern const char    kCapRowFmt[];       /* 0x004b9c6c */
extern const char    kCapTotFmt[];       /* 0x004b9c40 */
extern const float   kHundredth;         /* 0x004ab518  0.01f */

extern int  GetGameTimer(void);                              /* 0x00499430 */
extern unsigned int GetSimClock(void);                       /* 0x00499460 */
extern void HeapFree_w(void* p);                             /* 0x0049e4d0 */
extern int  GetTransparentColour(void);                      /* 0x0044e690 */
extern void LLSAuto(void);                                   /* 0x0047d630 */
extern void PushRenderingStatusAndLockVideoSurface(void);    /* 0x00463fc0 */
extern void PopRenderingStatus(void);                        /* 0x004641f0 */
extern int  PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern void Print(int x, int y, const char* text, int font); /* 0x00454ba0 */
extern int  sprintf(char* buf, const char* fmt, ...);        /* 0x0049e573 */

__declspec(dllimport) int __stdcall IntersectRect(WinRect* dst, const WinRect* a,
                                                  const WinRect* b);  /* [0x4ab2a0] */
__declspec(dllimport) unsigned int __stdcall timeGetTime(void);       /* [0x4ab1f8] */

/* 0x00463560 -- stamp the damage clocks from the wall and sim timers. */
// FUNCTION: LEGOLAND 0x00463560
void ResetDamageClock(void)
{
    g_dmg_game_timer = GetGameTimer();
    g_dmg_clock = GetSimClock();
}

/* 0x00468830 -- empty both script text buffers (byte 0 only).
 * Store order is text2 then text1; a shared zero in al. */
// FUNCTION: LEGOLAND 0x00468830
void ClearScriptTexts(void)
{
    char z = 0;
    g_script_text2[0] = z;
    g_script_text1[0] = z;
}

/* 0x00468830 neighbour -- free every owned script string and zero the count.
 * esi (the walking pointer) lives in the non-empty arm, so its push sinks. */
// FUNCTION: LEGOLAND 0x004689a0
void FreeScriptStrings(void)
{
    int n = g_script_string_count;
    int i = 0;

    if (n > 0) {
        char** p = g_script_strings;
        do {
            n = (int)*p;
            if (n != 0) {
                HeapFree_w((void*)n);
                *p = 0;
            }
            n = g_script_string_count;
            i++;
            p++;
        } while (i < n);
    }
    g_script_string_count = 0;
}

/* 0x004659a0 -- blit one 16-bpp advisor DIB (bottom-up) at (x, y).
 * HAND-WRITTEN: mid-stream ebx/esi/edi pushes after pitch*y and the
 * height load, plus an ebp push that sinks into the non-zero-height
 * arm.  RGB555 is widened to RGB565 when g_screen_depth == 2. */
NAKED
// FUNCTION: LEGOLAND 0x004659a0
void BltAdvisor(DibHeader* dib, int x, int y)
{
    __asm {
        mov      edx, dword ptr [g_ddsd_pitch]
        mov      eax, dword ptr [esp+4]
        imul     edx, dword ptr [esp+0ch]
        mov      ecx, dword ptr [eax+8]
        push     ebx
        push     esi
        push     edi
        mov      edi, dword ptr [g_ddsd_bits]
        mov      esi, dword ptr [eax+4]
        add      edi, edx
        mov      edx, dword ptr [esp+14h]
        lea      edi, [edi+edx*2]
        lea      edx, [ecx-1]
        mov      ebx, edx
        imul     ebx, esi
        test     ecx, ecx
        lea      eax, [eax+ebx*2+28h]
        mov      dword ptr [esp+10h], eax
        je       L_465A34
        inc      edx
        push     ebp
        mov      dword ptr [esp+1ch], edx
    L_4659E1:
        test     esi, esi
        jle      L_465A15
        mov      edx, eax
        mov      ecx, edi
        sub      edx, edi
        mov      ebp, esi
    L_4659ED:
        mov      ebx, dword ptr [g_screen_depth]
        mov      ax, word ptr [edx+ecx]
        cmp      ebx, 2
        jne      L_465A08
        mov      ebx, eax
        and      eax, 1fh
        and      ebx, 0ffffffe0h
        shl      ebx, 1
        or       eax, ebx
    L_465A08:
        mov      word ptr [ecx], ax
        add      ecx, 2
        dec      ebp
        jne      L_4659ED
        mov      eax, dword ptr [esp+14h]
    L_465A15:
        mov      ebx, dword ptr [g_ddsd_pitch]
        mov      ecx, esi
        neg      ecx
        add      edi, ebx
        lea      eax, [eax+ecx*2]
        mov      ecx, dword ptr [esp+1ch]
        dec      ecx
        mov      dword ptr [esp+14h], eax
        mov      dword ptr [esp+1ch], ecx
        jne      L_4659E1
        pop      ebp
    L_465A34:
        pop      edi
        pop      esi
        pop      ebx
        ret
    }
}

/* 0x004640f0 -- push lock status, unlock if locked, then lock.
 * Tail-jumped from PushSetTarget (gpu.c). Rect is built first; the status
 * push sits between right and bottom so eax can carry the lock flag into
 * the unlock guard. dwSize is written with the IntersectRect pushes. */
// WIP-FUNCTION: LEGOLAND 0x004640f0
void PushRenderingStatusAndRelockVideoSurface(void)
{
    WinRect    rect;
    int        status;
    ScreenCfg* s;

    s = g_screencfg;
    rect.left = 0;
    rect.top = 0;
    rect.right = s->w - 1;
    status = g_video_locked;
    g_status_stack[g_status_sp++] = status;
    rect.bottom = s->h - 1;
    if (status != 0) {
        if (g_draw_surface->vtbl->Unlock(g_draw_surface, g_ddsd.lpSurface)
                == DDERR_SURFACELOST) {
            g_draw_surface->vtbl->Restore(g_draw_surface);
            g_draw_surface->vtbl->Unlock(g_draw_surface, g_ddsd.lpSurface);
        }
    }
    IntersectRect(&g_render_clip, &rect, &g_clip_rect);
    g_ddsd.dwSize = 0x6c;
    if (g_draw_surface->vtbl->Lock(g_draw_surface, 0, &g_ddsd,
                                   DDLOCK_WRITEONLY_WAIT, 0) == DDERR_SURFACELOST) {
        g_draw_surface->vtbl->Restore(g_draw_surface);
        g_draw_surface->vtbl->Lock(g_draw_surface, 0, &g_ddsd,
                                   DDLOCK_WRITEONLY_WAIT, 0);
    }
    g_transparent_colour = GetTransparentColour();
    g_video_locked = 1;
}

/* 0x00466080 -- Flip-based presenter (g_present's initial value).
 * Cursor stamp, 28 ms frame floor, Flip+GetFlipStatus with lost-surface
 * restore of both the primary and the back buffer. */
// FUNCTION: LEGOLAND 0x00466080
int PresentFlip(void)
{
    long         hr;
    unsigned int now;

    LLSAuto();
    if (g_screencfg->cursor && g_current_pointer) {
        PushRenderingStatusAndLockVideoSurface();
        PrintSprite(g_current_pointer, g_gfx_point.x, g_gfx_point.y, 0, 0);
        PopRenderingStatus();
    }

    while (timeGetTime() - g_flip_time < 0x1c)
        ;
    g_flip_time = timeGetTime();

    hr = g_primary->vtbl->Flip(g_primary, 0, DDFLIP_WAIT);
    while (hr != 0) {
        if (hr == (long)DDERR_SURFACELOST) {
            g_primary->vtbl->Restore(g_primary);
            g_surface_78->vtbl->Restore(g_surface_78);
            return 0;
        }
        if (hr != (long)DDERR_SURFACEBUSY && hr != (long)DDERR_WASSTILLDRAWING)
            return 0;
        hr = g_primary->vtbl->Flip(g_primary, 0, DDFLIP_WAIT);
    }

    hr = g_surface_78->vtbl->GetFlipStatus(g_surface_78, DDGFS_ISFLIPDONE);
    while (hr != 0)
        hr = g_surface_78->vtbl->GetFlipStatus(g_surface_78, DDGFS_ISFLIPDONE);

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

/* 0x004632b0 -- Shift+capacity overlay: one line per AI category, then a total.
 * Called from gameframe when g_show_capacity and either shift key is down. */
// WIP-FUNCTION: LEGOLAND 0x004632b0
void ShowCapacityOverlay(void)
{
    char         buf[0x1e4];
    int          y;
    int          acc;
    AICat*       cat;
    const char** names;

    acc = 0;
    y = 0x14;
    names = g_capacity_names;
    cat = g_ai_cat;
    do {
        int product = cat->cap * cat->pct;
        int clamped = product;
        if (clamped >= cat->scale * 100)
            clamped = cat->scale * 100;
        sprintf(buf, kCapRowFmt, *names, cat->objects, cat->cap, cat->pct,
                product * kHundredth, cat->scale, clamped * kHundredth);
        Print(g_clip_rect.left + 8, g_clip_rect.top + y, buf, 2);
        acc += clamped;
        y += 0x14;
        cat++;
        names++;
    } while (y < 0x8c);
    sprintf(buf, kCapTotFmt, acc * kHundredth, g_visitor_cap_extra,
            g_visitor_cap, g_visitor_limit);
    Print(g_clip_rect.left + 8, g_clip_rect.top + 0x96, buf, 2);
}
