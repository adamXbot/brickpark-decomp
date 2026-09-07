/* LEGOLAND — dead (linker-retained) functions from the gamemain / sysmisc /
 * bighelp / tilehelp translation units, 0x004511e0..0x00466070.
 *
 * Nothing live in the shipped binary calls, tail-jumps to or takes the address
 * of any function in this file; the game was linked without /OPT:REF so the
 * code survived.  They are ordinary C from the same translation units as their
 * nearest matched neighbours (sysmisc.c, sysmisc2.c, memdb.c, text.c,
 * bighelp.c, bubblecache.c, tilehelp.c, blitmisc.c), so the vocabulary here is
 * theirs.  Names are OURS -- there is no export table entry for any of them.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Only
 * struct field OFFSETS, global addresses and callee argument counts are
 * load-bearing.  Types are defined locally so this file does not depend on
 * (or disturb) legoland.h.
 *
 * ---------------------------------------------------------------------------
 * THE VWIN32 VOLUME-LOCK CLUSTER (0x004511e0 .. 0x00451550)
 *
 * Eight of these functions are a Windows 95 raw-disk-access helper library.
 * They talk to the VWIN32 VxD through
 *     CreateFileA("\\\\.\\vwin32", 0,0,0,0, FILE_FLAG_DELETE_ON_CLOSE, 0)
 * and DeviceIoControl(h, VWIN32_DIOC_DOS_IOCTL == 1, &regs, 28, &regs, 28,
 * &returned, 0), where `regs` is the 28-byte DIOC_REGISTERS block
 *     +0x00 EBX  +0x04 EDX  +0x08 ECX  +0x0c EAX  +0x10 EDI  +0x14 ESI
 *     +0x18 Flags   (bit 0 = CF, i.e. the INT 21h call failed)
 * Every call is INT 21h AX=440Dh (generic IOCTL for block devices) with a
 * category/minor pair in CX:
 *     CX = 0x0848  category 08h minor 48h, DS:DX -> {BYTE op; BYTE nlocks}
 *     CX = 0x0849  category 08h minor 49h, no parameter block
 *     CX = xx4Ah   lock logical volume:   BL = drive, BH = level, DX = perms
 *     CX = xx6Ah   unlock logical volume: BL = drive
 * The two logical-volume calls try category 48h (the OSR2 FAT32 extension)
 * first and fall back to category 08h; the physical ones only ever use 08h.
 * The minor-48h parameter block's `op` is 2 to READ the outstanding lock
 * count back into `nlocks` and 1 to drop one lock -- 0x00451280 uses exactly
 * that pair to unwind however many locks are held.  Drive numbers are 1-based
 * (A: = 1), which is what 0x00451210's `toupper(letter) - 0x40` produces.
 * The names below describe that behaviour; the shipped game never calls any
 * of them (res.c mounts the CD by volume label instead).
 * ------------------------------------------------------------------------- */

/* ---- Win32 ------------------------------------------------------------- */

__declspec(dllimport) void* __stdcall CreateFileA(const char* name, unsigned long access,
                                                  unsigned long share, void* sa,
                                                  unsigned long disp, unsigned long flags,
                                                  void* templ);                    /* [0x4ab258] */
__declspec(dllimport) int   __stdcall CloseHandle(void* h);                        /* [0x4ab260] */
__declspec(dllimport) int   __stdcall DeviceIoControl(void* h, unsigned long code,
                                                      void* in, unsigned long insz,
                                                      void* out, unsigned long outsz,
                                                      unsigned long* returned,
                                                      void* overlapped);           /* [0x4ab248] */

/* ---- CRT --------------------------------------------------------------- */

extern int toupper(int c);                          /* 0x0049f34b (CRT) */
extern void* memset(void* d, int c, unsigned int n);
#pragma intrinsic(memset)

/* ---- the VWIN32 DOS-IOCTL register block ------------------------------- */

typedef struct DiocRegs {
    unsigned long reg_EBX;    /* +0x00 */
    unsigned long reg_EDX;    /* +0x04 */
    unsigned long reg_ECX;    /* +0x08 */
    unsigned long reg_EAX;    /* +0x0c */
    unsigned long reg_EDI;    /* +0x10 */
    unsigned long reg_ESI;    /* +0x14 */
    unsigned long reg_Flags;  /* +0x18  bit 0 = carry */
} DiocRegs;

/* The DS:DX parameter block of INT 21h AX=440Dh CX=0848h. */
typedef struct PhysLockParams {
    unsigned char op;         /* +0x00  0 lock, 1 unlock, 2 read lock count */
    unsigned char nlocks;     /* +0x01  returned by op 2 */
} PhysLockParams;

#define VWIN32_DIOC_DOS_IOCTL 1
#define DOS_GENERIC_BLOCK_IOCTL 0x440d

extern const char kVwin32Device[];   /* 0x004b8634 "\\\\.\\vwin32" */

/* The open VWIN32 handle; INVALID_HANDLE_VALUE when closed. */
extern void* g_vwin32;               /* 0x004b85c4 */


/* ============================== the text / bubble-help neighbourhood ==== */

typedef struct WinRect {
    int left;      /* +0x00 */
    int top;       /* +0x04 */
    int right;     /* +0x08 */
    int bottom;    /* +0x0c */
} WinRect;

typedef struct DDSurface DDSurface;

typedef struct DDSurfaceVtbl {
    char pad00[0x44];
    long(__stdcall* GetDC)(DDSurface*, void** hdc);      /* +0x44 */
    char pad48[0x68 - 0x48];
    long(__stdcall* ReleaseDC)(DDSurface*, void* hdc);   /* +0x68 */
} DDSurfaceVtbl;

struct DDSurface {
    DDSurfaceVtbl* vtbl;   /* +0x00 */
};

/* bubblecache.c / fpui3.c's 0x20-byte rendered-text cache entry. */
typedef struct TextEntry {
    int        w;       /* +0x00 */
    int        h;       /* +0x04 */
    int        format;  /* +0x08 */
    char*      text;    /* +0x0c */
    int        ink;     /* +0x10 */
    int        paper;   /* +0x14 */
    int        font;    /* +0x18 */
    void*      sprite;  /* +0x1c */
} TextEntry;

extern volatile int g_text_cache_count;   /* 0x006675b8 */
extern TextEntry    g_text_cache[];       /* 0x006675c0 */
extern DDSurface*   g_draw_surface;       /* 0x0066807c */
extern WinRect      g_clip_rect;          /* 0x004bdea0 */

__declspec(dllimport) void* __stdcall CreateCompatibleDC(void* dc);            /* [0x4ab094] */
__declspec(dllimport) int   __stdcall SetBkMode(void* dc, int mode);           /* [0x4ab074] */
__declspec(dllimport) void* __stdcall SelectObject(void* dc, void* obj);       /* [0x4ab080] */
__declspec(dllimport) int   __stdcall DeleteDC(void* dc);                      /* [0x4ab0a4] */
__declspec(dllimport) int   __stdcall DeleteObject(void* obj);                 /* [0x4ab09c] */
__declspec(dllimport) void* __stdcall CreateRectRgnIndirect(WinRect* rc);      /* [0x4ab0b4] */
__declspec(dllimport) int   __stdcall DrawTextA(void* dc, const char* s, int n,
                                                WinRect* rc, unsigned int fmt); /* [0x4ab2ac] */

extern unsigned int strlen(const char* s);
extern int          strcmp(const char* a, const char* b);
#pragma intrinsic(strlen, strcmp)

extern void* SelectFont(void* dc, int font);                      /* 0x00454b40 */
extern void  PushRenderingStatusAndUnlockVideoSurface(void);      /* 0x00464080 */
extern void  PopRenderingStatus(void);                            /* 0x004641f0 */

/* DT_WORDBREAK | DT_EXPANDTABS, with and without DT_CALCRECT. */
#define DT_MEASURE_WRAPPED 0x450
#define DT_DRAW_WRAPPED    0x050

/* ---- this file's own callees (text/help) ------------------------------- */

/* ---- this file's own callees ------------------------------------------- */

extern int  UnlockAllPhysicalLocks(void* h, unsigned char drive);   /* 0x00451280 */
extern int  UnlockPhysicalVolume(void* h, unsigned char drive);     /* 0x00451410 */
extern int  __stdcall UnlockLogicalVolume(void* h, unsigned char drive); /* 0x00451550 */
extern void __stdcall CloseVWin32(void* h);                         /* 0x004514a0 */
extern void VolumeLockNoOp(char letter);                            /* 0x004511f0 */

/* =========================================================================
 *  0x004514b0 -- lock a logical volume (minor 4Ah), FAT32 category first
 * NOTE ON THE MARKER: this body is EXACT (0 mismatches over the full 55
 * instructions / 159 bytes), but it must stay the FIRST function defined in
 * this file.  It is __stdcall, so its COFF symbol is `_LockLogicalVolume@16`;
 * tools/match.py looks names up as `X` / `_X` and, failing that, falls back to
 * the FIRST .text COMDAT of the object.  input2.c records the same constraint
 * for LegoLandWindowProc.  The two other __stdcall bodies here
 * (0x004514a0, 0x00451550) are equally exact but cannot be reached by that
 * fallback, so they carry WIP markers -- see their notes.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004514b0
int __stdcall LockLogicalVolume(void* h, unsigned char drive, unsigned char level,
                                unsigned short perms)
{
    DiocRegs      r;
    unsigned long returned;
    unsigned char cat;
    int           ok;

    memset(&r.reg_EDX, 0, sizeof(r) - sizeof(r.reg_EBX));
    cat = 0x48;
    for (;;) {
        r.reg_ECX = (unsigned short)(cat << 8) | 0x4a;
        r.reg_EAX = DOS_GENERIC_BLOCK_IOCTL;
        r.reg_EBX = (unsigned short)(level << 8) | (drive & 0xff);
        r.reg_EDX = perms & 0xffff;
        if (DeviceIoControl(h, VWIN32_DIOC_DOS_IOCTL, &r, sizeof(r), &r, sizeof(r),
                            &returned, 0)
            && (r.reg_Flags & 1) == 0) {
            ok = 1;
            break;
        }
        ok = 0;
        if (cat == 8)
            break;
        cat = 8;
    }
    return ok;
}

/* =========================================================================
 *  0x004511e0 / 0x004511f0 / 0x00451200 -- one-byte `ret` stubs
 * =========================================================================
 * Three consecutive empty functions in the sysmisc2.c neighbourhood, each a
 * single `ret` padded to a 16-byte boundary.  0x004511f0 is the only one with
 * a caller (0x00451210, below), which passes it the drive letter with a cdecl
 * `add esp` of its own, so it takes one argument; the outer two are reached
 * by nothing at all and are written void/void.  They are almost certainly the
 * release-build remains of debug hooks. */

// FUNCTION: LEGOLAND 0x004511e0
void VolumeDebugHookA(void)
{
}

// FUNCTION: LEGOLAND 0x004511f0
void VolumeLockNoOp(char letter)
{
}

// FUNCTION: LEGOLAND 0x00451200
void VolumeDebugHookB(void)
{
}

/* =========================================================================
 *  0x00451480 -- open the VWIN32 VxD
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00451480
void* OpenVWin32(void)
{
    return CreateFileA(kVwin32Device, 0, 0, 0, 0, 0x4000000, 0);
}

/* =========================================================================
 *  0x004514a0 -- close it again
 * ========================================================================= */

// WIP-FUNCTION: LEGOLAND 0x004514a0  (100% exact, 4/4i 14/14B; __stdcall COMDAT unreachable by audit.py's name lookup -- see the note above)
void __stdcall CloseVWin32(void* h)
{
    CloseHandle(h);
}

/* =========================================================================
 *  0x00451410 -- INT 21h AX=440Dh CX=0849h on one drive
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00451410
int UnlockPhysicalVolume(void* h, unsigned char drive)
{
    DiocRegs      r;
    unsigned long returned;

    memset(&r.reg_EDX, 0, sizeof(r) - sizeof(r.reg_EBX));
    r.reg_EBX = drive;
    r.reg_EAX = DOS_GENERIC_BLOCK_IOCTL;
    r.reg_ECX = 0x0849;
    if (DeviceIoControl(h, VWIN32_DIOC_DOS_IOCTL, &r, sizeof(r), &r, sizeof(r),
                        &returned, 0)) {
        if ((r.reg_Flags & 1) == 0)
            return 1;
    }
    return 0;
}

/* =========================================================================
 *  0x00451390 -- INT 21h AX=440Dh CX=0848h, parameter block op 0
 * ========================================================================= */

// WIP-FUNCTION: LEGOLAND 0x00451390  (64%, 14/39 strict, first diverging index 8: allocation rotation)
int LockPhysicalVolume(void* h, unsigned long drive)
{
    DiocRegs       r;
    PhysLockParams p;
    unsigned long  returned;

    memset(&r.reg_EDX, 0, sizeof(r) - sizeof(r.reg_EBX));
    p.nlocks = 0;
    p.op = 0;
    r.reg_EBX = *(volatile unsigned long*)&drive & 0xff;
    r.reg_EDX = (unsigned long)&p;
    r.reg_EAX = DOS_GENERIC_BLOCK_IOCTL;
    r.reg_ECX = 0x0848;
    if (DeviceIoControl(h, VWIN32_DIOC_DOS_IOCTL, &r, sizeof(r), &r, sizeof(r),
                        &returned, 0)) {
        if ((r.reg_Flags & 1) == 0)
            return 1;
    }
    return 0;
}

/* =========================================================================
 *  0x00451550 -- unlock a logical volume (minor 6Ah), FAT32 category first
 * ========================================================================= */

// WIP-FUNCTION: LEGOLAND 0x00451550  (100% exact, 49/49i 132/132B; __stdcall COMDAT unreachable by audit.py's name lookup -- see the note above)
int __stdcall UnlockLogicalVolume(void* h, unsigned char drive)
{
    DiocRegs       r;
    unsigned long  returned;
    unsigned char  cat;
    unsigned short cx;
    int            ok;

    memset(&r.reg_EDX, 0, sizeof(r) - sizeof(r.reg_EBX));
    cat = 0x48;
    for (;;) {
        cx = (unsigned short)(cat << 8);
        cx |= 0x6a;
        r.reg_ECX = cx;
        r.reg_EAX = DOS_GENERIC_BLOCK_IOCTL;
        r.reg_EBX = drive;
        if (DeviceIoControl(h, VWIN32_DIOC_DOS_IOCTL, &r, sizeof(r), &r, sizeof(r),
                            &returned, 0)
            && (r.reg_Flags & 1) == 0) {
            ok = 1;
            break;
        }
        ok = 0;
        if (cat == 8)
            break;
        cat = 8;
    }
    return ok;
}

/* =========================================================================
 *  0x00451280 -- drop every outstanding physical lock on one drive
 * ========================================================================= */

// WIP-FUNCTION: LEGOLAND 0x00451280  (48%, 46/88 strict, first diverging index 1: push ebx does not sink)
int UnlockAllPhysicalLocks(void* h, unsigned char drive)
{
    DiocRegs       r;
    PhysLockParams p;
    unsigned long  returned;
    int            ok;

    p.nlocks = 0;
    memset(&r.reg_EDX, 0, sizeof(r) - sizeof(r.reg_EBX));
    p.op = 2;
    r.reg_EDX = (unsigned long)&p;
    r.reg_EAX = DOS_GENERIC_BLOCK_IOCTL;
    r.reg_EBX = drive;
    r.reg_ECX = 0x0848;
    ok = DeviceIoControl(h, VWIN32_DIOC_DOS_IOCTL, &r, sizeof(r), &r, sizeof(r),
                         &returned, 0);
    if (!ok)
        return 0;
    if (r.reg_Flags & 1) {
        if (r.reg_EAX != 0xb0 && r.reg_EAX != 1)
            return 0;
        ok = 1;
    }
    {
        int i;

        for (i = 0; i < p.nlocks; i++) {
            p.op = 1;
            r.reg_EDX = (unsigned long)&p;
            r.reg_EAX = DOS_GENERIC_BLOCK_IOCTL;
            r.reg_EBX = drive;
            r.reg_ECX = 0x0848;
            if (!DeviceIoControl(h, VWIN32_DIOC_DOS_IOCTL, &r, sizeof(r), &r, sizeof(r),
                                 &returned, 0))
                return 0;
            if (r.reg_Flags & 1)
                return 0;
            ok = 1;
        }
    }
    return ok;
}

/* =========================================================================
 *  0x00451210 -- release every lock on the drive and close VWIN32
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00451210
void ReleaseVolumeLocks(char letter)
{
    char drive;

    VolumeLockNoOp(letter);
    if (g_vwin32 != (void*)-1) {
        drive = (char)(toupper(letter) - 0x40);
        UnlockAllPhysicalLocks(g_vwin32, drive);
        UnlockPhysicalVolume(g_vwin32, drive);
        UnlockLogicalVolume(g_vwin32, drive);
        CloseVWin32(g_vwin32);
        g_vwin32 = (void*)-1;
    }
}

/* =========================================================================
 *  0x004551a0 -- height of `text` wrapped into a `width`-wide column
 * =========================================================================
 * text.c's PrintCentColref neighbourhood.  A throwaway memory DC measures the
 * string with DT_CALCRECT | DT_WORDBREAK | DT_EXPANDTABS into a rectangle
 * seeded at `width - 1`, and the answer is the measured height plus one.
 *
 * ORIGINAL BUG (reproduced): the font selected into the memory DC is never
 * selected back out before DeleteDC -- SelectFont's result is discarded.  It
 * is harmless because the DC is a scratch one, and PrintTextBoxOnSurface
 * below (which shares the measuring block) makes the same call.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004551a0
int MeasureWrappedTextHeight(const char* text, int font, int width)
{
    WinRect rc;
    void*   dc;

    rc.left = 0;
    rc.top = 0;
    rc.right = 0;
    rc.bottom = 0;
    dc = CreateCompatibleDC(0);
    rc.right = width - 1;
    SetBkMode(dc, 1);
    SelectFont(dc, font);
    DrawTextA(dc, text, strlen(text), &rc, DT_MEASURE_WRAPPED);
    DeleteDC(dc);
    return rc.bottom - rc.top + 1;
}

/* =========================================================================
 *  0x00455220 -- wrap `text` into a `width` column and paint it at (x, y)
 * =========================================================================
 * bighelp.c's BubbleHelp neighbourhood, and the dead sibling of
 * MeasureWrappedTextHeight: the same measuring pass, then the measured
 * rectangle is offset to (x, y), the video surface is unlocked, a GDI DC is
 * taken on the draw surface and the text is drawn into it through a clipping
 * region built from g_clip_rect.  Both the region and the font are selected
 * back out and the region is deleted before the DC is released.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00455220
void PrintWrappedTextOnSurface(int x, int y, char* text, int font, int width)
{
    WinRect rc;
    void*   dc;
    void*   hdc;
    void*   rgn;
    void*   oldrgn;
    void*   oldfont;

    rc.left = 0;
    rc.top = 0;
    rc.right = 0;
    rc.bottom = 0;
    dc = CreateCompatibleDC(0);
    rgn = CreateRectRgnIndirect(&g_clip_rect);
    rc.right = width - 1;
    SetBkMode(dc, 1);
    SelectFont(dc, font);
    DrawTextA(dc, text, strlen(text), &rc, DT_MEASURE_WRAPPED);
    DeleteDC(dc);
    rc.left += x;
    rc.top += y;
    rc.right += x;
    rc.bottom += y;
    PushRenderingStatusAndUnlockVideoSurface();
    g_draw_surface->vtbl->GetDC(g_draw_surface, &hdc);
    SetBkMode(hdc, 1);
    oldrgn = SelectObject(hdc, rgn);
    oldfont = SelectFont(hdc, font);
    DrawTextA(hdc, text, strlen(text), &rc, DT_DRAW_WRAPPED);
    SelectObject(hdc, oldfont);
    SelectObject(hdc, oldrgn);
    DeleteObject(rgn);
    g_draw_surface->vtbl->ReleaseDC(g_draw_surface, hdc);
    PopRenderingStatus();
}

/* =========================================================================
 *  0x00455de0 -- find a rendered-text cache entry by its string alone
 * =========================================================================
 * bubblecache.c's PrintCachedText neighbourhood.  The weakest of the three
 * cache lookups in the tree: FindCachedText (0x00455d40) also matches font,
 * format and colours and FindCachedTextBox (0x00455c80) the size as well;
 * this one compares only the text.  The cursor anchors at the +0x0c text
 * pointer (the only field the loop reads) and the count is re-read in the
 * latch, as in every other walker over this array.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00455de0
TextEntry* FindCachedTextByString(const char* text)
{
    int i;

    for (i = 0; i < *(volatile int*)&g_text_cache_count; i++) {
        if (strcmp(g_text_cache[i].text, text) == 0)
            return &g_text_cache[i];
    }
    return 0;
}

/* =========================================================================
 *  0x00466070 -- one-byte `ret` stub in the blitmisc.c neighbourhood
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00466070
void PresentNoOp(void)
{
}
