/* LEGOLAND — advisor movie / clip control (scope AC).
 *
 * InitAdvisorMovies / KillAdvisorMovies (gamemain.c) and StartAdvisorClip
 * (screens3.c). SetAdvisorPose and RenderScriptEndIcon live elsewhere —
 * declare only.
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
    char  pad00[0x14];
    void* getframe;                 /* +0x14  PGETFRAME */
    char  pad18[0x1c - 0x18];
    void* video;                    /* +0x1c  PAVISTREAM */
    void (*stop)(struct AdvisorClip*); /* +0x20 */
    void (*tick)(void);             /* +0x24  set to AdvisorMovieTick */
} AdvisorClip;

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

/* Start (or clear) the current advisor clip; opens a GETFRAME on the BMI. */
// FUNCTION: LEGOLAND 0x00443dc0
void StartAdvisorClip(AdvisorClip* clip)
{
    g_advisor_a = 0;
    g_advisor_b = 0;
    g_advisor_c = 0;
    g_advisor_d = 0;
    if (clip->stop)
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
