/* LEGOLAND — a second batch of system-level helpers: playable-sample start,
 * the positional-audio screen placement, the UI theme switch and the two
 * CD-volume probers RES_EnsureMounted drives.
 *
 * Companion to sysmisc.c. Reconstructed from original/legoland.exe (VC6 SP3,
 * /O2 /Gy /Gd); only struct field OFFSETS, vtable slots and callee argument
 * counts are load-bearing — every name here is ours.
 */
#include "legoland.h"

/* ================================================================= audio == */

/* The Sample record, as audio2.c/audio3.c recovered it and sysmisc.c reuses
 * it. Only the fields this file touches are named. */
typedef struct IDSBuffer IDSBuffer;

typedef struct MapRef {
    int x;                      /* +0x00 */
    int y;                      /* +0x04 */
} MapRef;

typedef struct SoundSource {
    int    kind;                /* +0x00  0 none / 1 bloke / 2 map ref / 3 level xy */
    void*  obj;                 /* +0x04 */
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
    IDSBuffer*     buf;         /* +0x2c */
    void*          data30;      /* +0x30 */
    void*          data34;      /* +0x34 */
} Sample;

/* IDirectSoundBuffer, as audio3.c recovered it. Only three slots are used
 * here — Play (+0x30), SetVolume (+0x3c) and SetPan (+0x40); the padding
 * fixes their offsets. */
typedef struct IDSBufferVtbl {
    char pad00[0x30];
    long (__stdcall *Play)(IDSBuffer*, unsigned long, unsigned long,
                           unsigned long);                          /* +0x30 */
    char pad34[0x3c - 0x34];
    long (__stdcall *SetVolume)(IDSBuffer*, long);                  /* +0x3c */
    long (__stdcall *SetPan)(IDSBuffer*, long);                     /* +0x40 */
} IDSBufferVtbl;
struct IDSBuffer { IDSBufferVtbl* lpVtbl; };

#define DSBPLAY_LOOPING  1

/* Sample::flags bits (audio3.c). */
#define SF_PAUSED_SINGLY 0x0001
#define SF_UNSOURCED     0x0002
#define SF_LOOPING       0x0004

/* ---- globals ---- */
extern int g_samples_ready;     /* 0x007988c0  sample system up */

/* Start (or restart) a playable sample's DirectSound buffer.
 *
 * The guard chain is UpdateSampleSource's: the sample system must be up, the
 * sample must exist, and it must be an INSTANCE (+0x28 points at the
 * definition it plays) rather than a definition. Note the looping flag comes
 * off the instance but the buffer is loaded once, above the branch — the two
 * Play calls differ only in dwFlags, and each failure is its own `return 0`
 * (VC6 tail-duplicates the three-instruction epilogue five times over rather
 * than jumping to it).
 *
 * Success clears bit 1 of the flags word — the same bit
 * UnSourceAndFadeSample (audio3.c, 0x00496c20) tests and clears, logging as
 * it goes. Here it is cleared silently, so bit 1 reads as "an un-source is
 * pending" and starting the buffer cancels it. */
// FUNCTION: LEGOLAND 0x004928a0
int StartPlayableSample(Sample* s)
{
    if (!g_samples_ready)
        return 0;
    if (!s)
        return 0;
    if (!s->def)
        return 0;

    if (s->flags & SF_LOOPING) {
        if (s->buf->lpVtbl->Play(s->buf, 0, 0, DSBPLAY_LOOPING))
            return 0;
    } else {
        if (s->buf->lpVtbl->Play(s->buf, 0, 0, 0))
            return 0;
    }
    s->flags &= ~SF_UNSOURCED;
    return 1;
}

/* ================================================================= theme == */

/* The interactive-music ("IMT") worker thread is driven through a tiny command
 * mailbox plus an event: g_imt_cmd is the opcode, g_imt_cmd_arg its argument,
 * and SetEvent(g_imt_event) wakes the thread. 0x00492d80 posts opcode 1 the
 * same way; 0x00492ca0 posts opcode 3. g_imt_state is the thread's own state
 * word — 1/2 mean a transition is being SET UP, 5/6 that one is queued and 7
 * that one is running. */
extern int   g_imt_state;      /* 0x004bf778 */
extern void* g_imt_event;      /* 0x0079a6a0  SetEvent handle */
extern int   g_imt_cmd;        /* 0x0079a6a4 */
extern int   g_imt_cmd_arg;    /* 0x0079a6a8 */
extern int   g_imt_theme;      /* 0x0079a6ac  the theme now playing */

extern const char kThemeSame[];       /* 0x004bf7cc "IMT:Theme was same %d, continuing\n" */
extern const char kAlreadyInTrans[];  /* 0x004bf7a4 "IMT:Already in transition, continuing\n" */
extern const char kChangeBeforeTrans[]; /* 0x004bf77c "IMT:Changing theme before transition\n" */

extern void DBPrintf(const char* fmt, ...);          /* 0x00453a20 */
/* Posts opcode 3 (theme % 5) when g_imt_state is 1 or 2; a no-op otherwise. */
extern void SetThemeInTransition(int theme);         /* 0x00492ca0 (internal) */

__declspec(dllimport) int __stdcall SetEvent(void* ev);   /* [0x4ab0f8] */

/* Ask the music thread for a new theme.
 *
 * ORIGINAL INCONSISTENCY (reproduced): the fall-through path posts
 * `theme % 5` — the same wrap 0x00492ca0 applies — but the 5/6 arm stores the
 * caller's RAW theme into the mailbox argument. A theme of 5 or more queued
 * while a transition is pending therefore reaches the thread unwrapped.
 *
 * The `%` really is signed: `cdq / idiv ecx` with 5 in ecx, not an
 * and/shift pair, so `theme` is a signed int. */
// FUNCTION: LEGOLAND 0x00492ce0
void SetTheme(int theme)
{
    if (g_imt_state == 1 || g_imt_state == 2) {
        SetThemeInTransition(theme);
    } else if (theme == g_imt_theme) {
        DBPrintf(kThemeSame, g_imt_theme);
    } else if (g_imt_state == 5 || g_imt_state == 6) {
        DBPrintf(kChangeBeforeTrans);
        g_imt_cmd_arg = theme;
    } else if (g_imt_state == 7) {
        DBPrintf(kAlreadyInTrans);
    } else {
        g_imt_cmd = 4;
        g_imt_cmd_arg = theme % 5;
        SetEvent(g_imt_event);
    }
}

/* ---------------------------------------------------- positional placement */

/* The game record at 0x004bcbf4 seen through its viewport size (bigrender.c
 * calls the same object g_map, fpui3.c g_scroll_map). */
typedef struct ViewCfg {
    char           pad00[0x10];
    unsigned short view_w;      /* +0x10 */
    unsigned short view_h;      /* +0x12 */
} ViewCfg;
extern ViewCfg* g_view;         /* 0x004bcbf4 */

/* dx * 4 clamped to the DirectSound pan range [-10000, +10000]. */
extern int PanFromOffset(int dx);        /* 0x00496570 (internal) */
/* g_sfx_master_db - d2/60, clamped: anything above 0 or below -3000 dB is
 * turned into DSBVOLUME_MIN (-10000). */
extern int VolumeFromDistSq(int d2);     /* 0x00496540 (internal) */

/* Push a sample's on-screen position into its DirectSound buffer as a pan and
 * a volume. Called from every arm of UpdateSampleSource (sysmisc.c).
 *
 * The pan is taken from the RAW offset from the screen centre; the volume from
 * the squared distance to the nearest EDGE of the viewport — x and y are each
 * folded to zero inside a band of half the viewport, so a sample anywhere in
 * the middle of the screen plays at full volume.
 *
 * `view_w >> 1` comes out as `shr` and `-view_w >> 1` as `neg` + `sar`: the
 * unsigned short field stays known-non-negative until the negate widens it.
 * An odd viewport width therefore rounds the two thresholds in opposite
 * directions, which is what makes the pair of shifts visible at all.
 *
 * Two levers here. (a) The offset pair must be ONE `Pos` aggregate: as two
 * plain `int` locals VC6 builds the squared distance as `x*x` into edx and
 * `y*y` into eax where the original has them the other way round, and no
 * spelling of the sum reaches it (both operand orders, four named-temporary
 * shapes, an inlined `Sq()`/`DistSq()` helper and six free volatile reads all
 * floor at 4, or 2 at one instruction too many). As aggregate members `p.y`
 * becomes the destination symbol of its own product and both sum orders are
 * exact. (b) The volume must be a NAMED local: written as a nested call
 * argument VC6 hoists `s->buf` and its vtable above the inner call, spills the
 * vtable pointer and takes a fourth callee-saved register (90 instructions). */
// FUNCTION: LEGOLAND 0x004965a0
int SetSampleScreenPos(Sample* s, int x, int y)
{
    Pos p;
    int pan;
    int vol;

    p.x = x - (g_view->view_w >> 1);
    p.y = y - (g_view->view_h >> 1);
    pan = PanFromOffset(p.x);

    if (p.x < -g_view->view_w >> 1)
        p.x += g_view->view_w >> 1;
    else if (p.x > g_view->view_w >> 1)
        p.x -= g_view->view_w >> 1;
    else
        p.x = 0;

    if (p.y < -g_view->view_h >> 1)
        p.y += g_view->view_h >> 1;
    else if (p.y > g_view->view_h >> 1)
        p.y -= g_view->view_h >> 1;
    else
        p.y = 0;

    vol = VolumeFromDistSq(p.x * p.x + p.y * p.y);
    if (s->buf->lpVtbl->SetVolume(s->buf, vol))
        return 0;
    return s->buf->lpVtbl->SetPan(s->buf, pan) == 0;
}

/* ============================================================= resources == */

/* The drive prefix RES_EnsureMounted's non-zero branch probes; only its first
 * character (the drive letter) is used. */
extern char g_res_path[];              /* 0x00813b04 */
extern const char kCDFS[];             /* 0x004b85ec "CDFS" */

extern const char kCheckingAllDrives[]; /* 0x004b8610 "Checking all drives (Mask = %d)" */
extern const char kGettingInfo[];       /* 0x004b85f4 "Getting Info on drive %s" */
extern const char kDriveHasCd[];        /* 0x004b85c8 "Drive %s contains the correct CD" */

int    strcmp(const char* a, const char* b);
char*  strcpy(char* d, const char* s);
#pragma intrinsic(strcmp, strcpy)
extern int  toupper(int c);            /* 0x0049f34b (CRT) */
extern void DebugPrintf(const char* fmt, ...);  /* 0x0047f870 */
extern void DebugFlush(void);                   /* 0x0047f850 */

#define DRIVE_CDROM 5

__declspec(dllimport) int __stdcall GetVolumeInformationA(
    const char* root, char* volname, unsigned long volname_size,
    unsigned long* serial, unsigned long* maxcomp, unsigned long* fsflags,
    char* fsname, unsigned long fsname_size);          /* [0x4ab214] */

/* Is `vol` the label of the CD in the drive g_res_path names?
 *
 * ORIGINAL BUG (reproduced): the drive letter is never upper-cased. The
 * `toupper` call is there with the right argument, but its result is thrown
 * away — the raw character was already stored into the root path a statement
 * earlier, so the author wrote `toupper(root[0]);` where he meant
 * `root[0] = toupper(root[0]);`. GetVolumeInformationA ignores case, so the
 * bug never shows.
 *
 * A null `vol` means "any CD will do": only the CDFS check has to pass.
 *
 * `char root[4] = "c:\\";` is the four-byte initialiser: VC6 copies a
 * string-literal array initialiser of exactly one dword out of .rdata
 * is why "c:\" appears twice in .rdata — once per prober. */
// FUNCTION: LEGOLAND 0x004510e0
int RES_FindVolumeOnResPath(const char* vol)
{
    char          root[4] = "c:\\";
    unsigned long maxcomp;
    unsigned long serial;
    unsigned long fsflags;
    char          fsname[0x100];
    char          volname[0x100];
    int           found = 0;

    root[0] = g_res_path[0];
    toupper(root[0]);

    if (GetVolumeInformationA(root, volname, 0x100, &serial, &maxcomp,
                              &fsflags, fsname, 0x100)) {
        if (strcmp(fsname, kCDFS) == 0) {
            if (!vol || strcmp(volname, vol) == 0)
                found = 1;
        }
    }
    return found;
}

__declspec(dllimport) unsigned long __stdcall GetLogicalDrives(void); /* [0x4ab218] */
__declspec(dllimport) unsigned int __stdcall GetDriveTypeA(const char* root); /* [0x4ab210] */

/* Look for the named volume in every CD drive the system has.
 *
 * ORIGINAL BEHAVIOUR (reproduced): the loop never breaks. All 32 mask bits are
 * walked even after a hit, so with two CD drives holding a matching disc
 * g_res_path ends up naming the LAST one and "Drive %s contains the correct
 * CD" is logged twice. The side effect is the point: a hit rewrites
 * g_res_path, which is what makes the sibling prober work on later calls.
 *
 * A null `vol` means "any CD will do", exactly as in RES_FindVolumeOnResPath.
 *
 * Three levers, all measured here (about 85 variants; see the lane notes).
 * (a) The loop lives inside `if (mask) { ... }`, not behind an early
 * `if (!mask) return found;`: with the early return VC6 merges `found = 0`
 * with the loop counter's `i = 0` into one hoisted zero register, stores the
 * flag straight out to a stack slot and never enregisters it again (the frame
 * grows to 0x21c). (b) Given (a), `found = 1;` must come BEFORE the strcpy —
 * written after it the flag is spilled anyway and the incoming `vol` takes
 * ebp instead. (c) `bit <<= 1` belongs in the for's INCREMENT, which is what
 * interleaves `inc ebx` between the shifted value's load and its shift; in
 * the body, the load / shift / store come out as one block. */
// FUNCTION: LEGOLAND 0x00450f30
int RES_FindVolumeOnAnyDrive(const char* vol)
{
    char          root[4] = "c:\\";
    unsigned long bit;
    unsigned long mask;
    unsigned long fsflags;
    unsigned long serial;
    unsigned long maxcomp;
    char          fsname[0x100];
    char          volname[0x100];
    int           i;
    int           found = 0;

    mask = GetLogicalDrives();
    DebugPrintf(kCheckingAllDrives, mask);
    DebugFlush();
    if (mask) {
        bit = 1;
        for (i = 0; i < 32; i++, bit <<= 1) {
            if (bit & mask) {
                root[0] = (char)(i + 'A');
                if (GetDriveTypeA(root) == DRIVE_CDROM) {
                    DebugPrintf(kGettingInfo, root);
                    DebugFlush();
                    if (GetVolumeInformationA(root, volname, 0x100, &serial,
                                              &maxcomp, &fsflags, fsname, 0x100)) {
                        if (strcmp(fsname, kCDFS) == 0) {
                            if (!vol || strcmp(volname, vol) == 0) {
                                found = 1;
                                strcpy(g_res_path, root);
                                DebugPrintf(kDriveHasCd, g_res_path);
                                DebugFlush();
                            }
                        }
                    }
                }
            }
        }
    }
    return found;
}
