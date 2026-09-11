/* LEGOLAND — advisor movie / clip control (scope AC).
 *
 * InitAdvisorMovies / KillAdvisorMovies (gamemain.c) and SetVidAnim
 * (screens3.c). SetAdvisorPose and RenderScriptEndIcon live elsewhere —
 * declare only.
 *
 * 0x00443dc0 IS CALLED `SetVidAnim` BY THE SHIPPED BINARY (scope PORT-M5).
 * RenderAdvisorIcon (0x00443e30) stamps a debug breadcrumb into 0x00667c40
 * immediately before each operation it performs, and the four strings are
 * "SetVidAnim", "AVI GetFrame", "BltAdvisor", "Exit Advisor":
 *   0x00443e93  mov dword ptr [0x667c40], 0x4b7dc4   ; "SetVidAnim"
 *   0x00443e9d  call 0x443dc0
 *   0x00443eb6  mov dword ptr [0x667c40], 0x4b7db4   ; "AVI GetFrame"
 *   0x00443ec5  call 0x49e418                        ; AVIStreamGetFrame
 *   0x00443ecc  mov dword ptr [0x667c40], 0x4b7da8   ; "BltAdvisor"
 *   0x00443ee6  call 0x4659a0                        ; BltAdvisor
 * The third breadcrumb names a function we had ALREADY recovered as
 * `BltAdvisor`, which is what proves the convention names the callee rather
 * than the caller.  `StartAdvisorClip` was ours; this is the game's own name.
 *
 * VC6 SP3 /O2 /Gy /Gd. Addresses are load-bearing; names are ours except the
 * three already named by callers.
 */
#include "legoland.h"

/* BITMAPINFOHEADER fields InitAdvisorBmi writes (wanted format for
 * AVIStreamGetFrameOpen). biSize/biPlanes/biCompression live in .data and
 * are not touched here. */
typedef struct AdvisorBmi {
    unsigned long  biSize;          /* +0x00  0x004b7d70 */
    long           biWidth;         /* +0x04 */
    long           biHeight;        /* +0x08 */
    unsigned short biPlanes;        /* +0x0c */
    unsigned short biBitCount;      /* +0x0e */
    unsigned long  biCompression;   /* +0x10 */
    unsigned long  biSizeImage;     /* +0x14 */
} AdvisorBmi;

typedef struct AdvisorClip {
    int   frames;                   /* +0x00 */
    int   fps;                      /* +0x04 */
    int   width;                    /* +0x08 */
    int   height;                   /* +0x0c */
    void* file;                     /* +0x10 */
    void* getframe;                 /* +0x14 */
    void* audio;                    /* +0x18  unused here */
    void* video;                    /* +0x1c */
    void (*stop)(struct AdvisorClip*); /* +0x20 */
    void (*tick)(void);             /* +0x24 */
} AdvisorClip;

typedef struct AVIFILEINFO {
    unsigned long dwMaxBytesPerSec;
    unsigned long dwFlags;
    unsigned long dwCaps;
    long          dwStreams;
    unsigned long dwSuggestedBufferSize;
    unsigned long dwWidth;
    unsigned long dwHeight;
    unsigned long dwScale;
    unsigned long dwRate;
    unsigned long dwLength;
    unsigned long dwEditCount;
    char          szFileType[64];
} AVIFILEINFO;

typedef struct AVISTREAMINFO {
    unsigned long  fccType;
    unsigned long  fccHandler;
    unsigned long  dwFlags;
    unsigned long  dwCaps;
    unsigned short wPriority;
    unsigned short wLanguage;
    unsigned long  dwScale;
    unsigned long  dwRate;
    unsigned long  dwStart;
    unsigned long  dwLength;
    unsigned long  dwInitialFrames;
    unsigned long  dwSuggestedBufferSize;
    unsigned long  dwQuality;
    unsigned long  dwSampleSize;
    struct { long left, top, right, bottom; } rcFrame;
    unsigned long  dwEditCount;
    unsigned long  dwFormatChangeCount;
    char           szName[64];
} AVISTREAMINFO;

/* ---- AVI / CRT ---------------------------------------------------------- */
extern void  __stdcall AVIFileInit(void);                               /* 0x0049e406 */
extern void  __stdcall AVIFileExit(void);                               /* 0x0049e3d6 */
extern long  __stdcall AVIFileOpenA(void**, const char*, unsigned, const void*); /* 0x0049e400 */
extern long  __stdcall AVIFileInfoA(void*, void*, long);                /* 0x0049e3fa */
extern long  __stdcall AVIFileGetStream(void*, void**, unsigned long, long); /* 0x0049e3f4 */
extern long  __stdcall AVIStreamInfoA(void*, void*, long);              /* 0x0049e3ee */
extern unsigned long __stdcall AVIStreamAddRef(void*);                  /* 0x0049e3e8 */
extern unsigned long __stdcall AVIStreamRelease(void*);                 /* 0x0049e3e2 */
extern void* __stdcall AVIStreamGetFrameOpen(void*, const void*);       /* 0x0049e412 */
extern long  __stdcall AVIStreamGetFrameClose(void*);                   /* 0x0049e40c */
extern void* HeapAlloc_w(unsigned int n);                               /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                                       /* 0x0049e4d0 */

/* ---- globals ------------------------------------------------------------ */
extern unsigned char g_report_state[];      /* 0x00665ff8 */
extern AdvisorBmi    g_advisor_bmi;         /* 0x004b7d70 */
extern int           g_avi_open_count;      /* 0x00665f48 — advisor's open tally */
extern AdvisorClip*  g_advisor_clip;        /* 0x00665f5c */
extern int           g_advisor_a;           /* 0x00665f68 */
extern int           g_advisor_b;           /* 0x00665f6c */
extern int           g_advisor_c;           /* 0x00665f64 */
extern int           g_advisor_d;           /* 0x00665eec */
extern int           g_advisor_pose;        /* 0x00665fec */
extern int           g_advisor_pose_prev;   /* 0x00665fe8 */
extern int           g_advisor_pose_arg;    /* 0x0081c09c */
extern int           g_advisor_pose_timer;  /* 0x0081c088 */
extern void*         g_advisor_pose_clip;   /* 0x00665f60 */
extern AdvisorClip*  g_ad_blink;            /* 0x0081c08c */
extern AdvisorClip*  g_ad_lr;               /* 0x0081c094 */
extern AdvisorClip*  g_ad_phone;            /* 0x0081c0a0 */
extern AdvisorClip*  g_ad_phone_gesture;    /* 0x0081c0a4 */
extern AdvisorClip*  g_ad_phone_down;       /* 0x0081c098 */
extern AdvisorClip*  g_ad_wobble;           /* 0x0081c090 */

extern const char kAdBlink[];               /* 0x004b7e24 "AD_Blink.avi" */
extern const char kAdLR[];                  /* 0x004b7e18 "AD_LR.avi" */
extern const char kAdPhone[];               /* 0x004b7e08 "AD_Phone.avi" */
extern const char kAdPhoneGesture[];        /* 0x004b7df4 "AD_PhoneGesture.avi" */
extern const char kAdPhoneDown[];           /* 0x004b7de0 "AD_PhoneDown.avi" */
extern const char kAdWobble[];              /* 0x004b7dd0 "AD_Wobble.avi" */

extern void SetAdvisorPose(int pose, int arg); /* 0x00444070 */
extern unsigned long __stdcall AVIFileRelease(void* pfile); /* 0x0049e3dc */

void InitAdvisorBmi(void);
void AdvisorMovieTick(void);
void SetVidAnim(AdvisorClip* clip);

/* Open one advisor .AVI (video stream only) into a 0x28-byte clip record.
 * Declaration order mirrors OpenMovie without the audio = 0 home: only
 * video = 0, so ebx is the shared zero / video carrier. frames/fps/w/h stay
 * uninitialized (OpenMovie precedent). +0x18 (audio) is never written. */
// FUNCTION: LEGOLAND 0x00443bd0
AdvisorClip* LoadAdvisorMovie(const char* path)
{
    void*         pfile;
    int           fps;
    int           i;
    void*         stream;
    AVISTREAMINFO si;
    AVIFILEINFO   fi;
    void*         video = 0;
    int           frames;
    int           w;
    int           h;
    AdvisorClip*  clip;

    if (g_avi_open_count == 0)
        AVIFileInit();
    if (AVIFileOpenA(&pfile, path, 0, 0) != 0) {
        if (g_avi_open_count == 0)
            AVIFileExit();
        return 0;
    }
    fi.dwStreams = 0;
    AVIFileInfoA(pfile, &fi, 0x6c);
    for (i = 0; i < fi.dwStreams; i++) {
        if (AVIFileGetStream(pfile, &stream, 0, i) != 0)
            break;
        if (AVIStreamInfoA(stream, &si, 0x8c) != 0)
            continue;
        if (si.fccType == 0x73646976) {
            video = stream;
            AVIStreamAddRef(stream);
            frames = (int)si.dwLength;
            fps = (int)(si.dwRate / si.dwScale);
            w = si.rcFrame.right - si.rcFrame.left;
            h = si.rcFrame.bottom - si.rcFrame.top;
        }
    }
    if (!video) {
        AVIFileRelease(pfile);
        if (g_avi_open_count == 0)
            AVIFileExit();
        return 0;
    }
    clip = (AdvisorClip*)HeapAlloc_w(0x28);
    if (!clip) {
        AVIStreamRelease(video);
        AVIFileRelease(pfile);
        if (g_avi_open_count == 0)
            AVIFileExit();
        return 0;
    }
    clip->frames = frames;
    clip->fps = fps;
    clip->width = w;
    clip->height = h;
    clip->file = pfile;
    clip->getframe = 0;
    clip->video = video;
    clip->stop = 0;
    clip->tick = 0;
    g_avi_open_count++;
    return clip;
}

/* Bring up the six AD_*.avi clips and start on blink. */
// FUNCTION: LEGOLAND 0x00444090
void InitAdvisorMovies(void)
{
    InitAdvisorBmi();
    g_ad_blink = LoadAdvisorMovie(kAdBlink);
    if (g_ad_blink)
        g_ad_blink->tick = AdvisorMovieTick;
    g_ad_lr = LoadAdvisorMovie(kAdLR);
    if (g_ad_lr)
        g_ad_lr->tick = AdvisorMovieTick;
    g_ad_phone = LoadAdvisorMovie(kAdPhone);
    if (g_ad_phone)
        g_ad_phone->tick = AdvisorMovieTick;
    g_ad_phone_gesture = LoadAdvisorMovie(kAdPhoneGesture);
    if (g_ad_phone_gesture)
        g_ad_phone_gesture->tick = AdvisorMovieTick;
    g_ad_phone_down = LoadAdvisorMovie(kAdPhoneDown);
    if (g_ad_phone_down)
        g_ad_phone_down->tick = AdvisorMovieTick;
    g_ad_wobble = LoadAdvisorMovie(kAdWobble);
    if (g_ad_wobble)
        g_ad_wobble->tick = AdvisorMovieTick;
    SetVidAnim(g_ad_blink);
}

/* Clear the first dword of the report-state block. */
// FUNCTION: LEGOLAND 0x004441f0
void ClearReportState(void)
{
    *(int*)g_report_state = 0;
}

/* Seed the advisor AVI wanted-format BMI: 112x96, 16 bpp, sizeimage 0x5400. */
// FUNCTION: LEGOLAND 0x00443d90
void InitAdvisorBmi(void)
{
    g_advisor_bmi.biBitCount = 0x10;
    g_advisor_bmi.biWidth = 0x70;
    g_advisor_bmi.biHeight = 0x60;
    g_advisor_bmi.biSizeImage = 0x5400;
}

/* Map pose 1..5 onto the six loaded clips; anything else is blink. */
// FUNCTION: LEGOLAND 0x00443f90
AdvisorClip* GetAdvisorClipByPose(int pose)
{
    switch (pose) {
    case 1: return g_ad_lr;
    case 2: return g_ad_phone;
    case 3: return g_ad_phone_gesture;
    case 4: return g_ad_phone_down;
    case 5: return g_ad_wobble;
    default: return g_ad_blink;
    }
}

/* Pose transition helper used when the current pose equals the previous. */
// FUNCTION: LEGOLAND 0x00443fe0
int NextAdvisorPose(int pose)
{
    switch (pose) {
    case 1: return 1;
    case 2: return 3;
    case 3: return 4;
    case 4: return 5;
    case 5: return 5;
    default: return 0;
    }
}

/* Advance the pose machine when pose == pose_prev, then pick the clip. */
// FUNCTION: LEGOLAND 0x00444020
void AdvisorMovieTick(void)
{
    int pose = g_advisor_pose;
    if (pose == g_advisor_pose_prev) {
        pose = NextAdvisorPose(g_advisor_pose_prev);
        g_advisor_pose = pose;
    }
    g_advisor_pose_prev = pose;
    g_advisor_pose_timer = g_advisor_pose_arg;
    g_advisor_pose_arg = 0;
    g_advisor_pose_clip = GetAdvisorClipByPose(pose);
}

/* Free one advisor clip (getframe, stream, record) and maybe AVIFileExit. */
// FUNCTION: LEGOLAND 0x00443d50
void FreeAdvisorClip(AdvisorClip* clip)
{
    if (clip->getframe)
        AVIStreamGetFrameClose(clip->getframe);
    if (clip->video)
        AVIStreamRelease(clip->video);
    HeapFree_w(clip);
    if (--g_avi_open_count == 0)
        AVIFileExit();
}

/* Start (or clear) the current advisor clip; opens a GETFRAME on the BMI.
 * The game's own name, from RenderAdvisorIcon's debug breadcrumb (file
 * header); screens3.c has always declared it `SetVidAnim`.
 *
 * A SHIPPED BUG, NOT A RECOVERY ERROR (scope PORT-M6).  `clip->stop` is read
 * with no null test and `if (clip)` is tested three statements later, and the
 * original does exactly that -- the dereference precedes the test in the
 * instruction stream:
 *   0x00443dc1  mov esi, [esp+8]            ; esi = clip
 *   0x00443dc6  xor edi, edi
 *   0x00443dc8..0x00443dda                  ; the four stores, unconditional
 *   0x00443de0  mov eax, [esi+0x20]         ; <-- clip->stop, UNGUARDED
 *   0x00443de5  je  0x443ded
 *   0x00443e08  cmp esi, edi                ; <-- if (clip), three later
 *   0x00443e10  je  0x443e23
 * `InitAdvisorMovies` ends with `SetVidAnim(g_ad_blink)` and `g_ad_blink` is 0
 * whenever AD_Blink.avi did not load, so as shipped that is an access
 * violation on Windows.  The instruction stream is what the gates check, so
 * the test stays where the author put it in the VC6 arm; the portable arm
 * hoists it, because on wasm the read at offset 0x20 of address 0 succeeds and
 * clang then uses it to PROVE clip != NULL and folds the later `if (clip)`
 * away, calling AVIStreamGetFrameOpen with the word at address 0x1c
 * (docs/lanes/scope-port-b6.md G1; it is why avifil32.c's
 * AVIStreamGetFrameOpen never follows its argument). */
// FUNCTION: LEGOLAND 0x00443dc0
void SetVidAnim(AdvisorClip* clip)
{
    g_advisor_a = 0;
    g_advisor_b = 0;
    g_advisor_c = 0;
    g_advisor_d = 0;
#ifndef LEGOLAND_PORTABLE
    if (clip->stop)
#else
    if (clip && clip->stop)
#endif
        clip->stop(clip);
    if (g_advisor_clip) {
        AVIStreamGetFrameClose(g_advisor_clip->getframe);
        g_advisor_clip->getframe = 0;
    }
    g_advisor_clip = clip;
    if (clip)
        clip->getframe = AVIStreamGetFrameOpen(clip->video, &g_advisor_bmi);
}

/* Tear down all six advisor movies. */
// FUNCTION: LEGOLAND 0x00444150
void KillAdvisorMovies(void)
{
    if (g_ad_blink) {
        FreeAdvisorClip(g_ad_blink);
        g_ad_blink = 0;
    }
    if (g_ad_lr) {
        FreeAdvisorClip(g_ad_lr);
        g_ad_lr = 0;
    }
    if (g_ad_phone) {
        FreeAdvisorClip(g_ad_phone);
        g_ad_phone = 0;
    }
    if (g_ad_phone_gesture) {
        FreeAdvisorClip(g_ad_phone_gesture);
        g_ad_phone_gesture = 0;
    }
    if (g_ad_phone_down) {
        FreeAdvisorClip(g_ad_phone_down);
        g_ad_phone_down = 0;
    }
    if (g_ad_wobble) {
        FreeAdvisorClip(g_ad_wobble);
        g_ad_wobble = 0;
    }
}
