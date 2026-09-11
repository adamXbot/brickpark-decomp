/* LEGOLAND — sample playback control (pause/resume/mute, fade, 3D sourcing),
 * the music-band/groove wrappers and the AVI sound-buffer play/unlock pair.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only the
 * struct field offsets, vtable slots and callee arg counts are load-bearing;
 * names are ours.
 */

/* ---- playable sample record -------------------------------------------- */
/* One live instance of a loaded sample (allocated by MakePlayable, freed by
 * KillPlayableSample). Singly linked through +0x00 from g_playable_list.
 *   +0x08 fade rate      (SetSampleFade)
 *   +0x0c source mode    0 = none, 1 = bloke, 2 = map ref   (UpdateSampleSource)
 *   +0x10 source bloke   (SourcePlayableSampleToBloke)
 *   +0x14/+0x18 source map x/y (SourcePlayableSampleToMapRef)
 *   +0x1c flags          bit 0 = singly paused
 *   +0x28 owning sample definition (refcount lives at def+0x04)
 *   +0x2c IDirectSoundBuffer
 */
typedef struct IDSBuffer IDSBuffer;
typedef struct SampleDef SampleDef;

typedef struct PlayableSample {
    struct PlayableSample* next;    /* +0x00 */
    int                    pad04;   /* +0x04 */
    int                    fade;    /* +0x08 */
    int                    srcmode; /* +0x0c */
    void*                  srcobj;  /* +0x10 */
    int                    srcx;    /* +0x14 */
    int                    srcy;    /* +0x18 */
    unsigned short         flags;   /* +0x1c  bit0 = singly paused */
    short                  pad1e;   /* +0x1e */
    char                   pad20[0x28 - 0x20];
    SampleDef*             def;     /* +0x28 */
    IDSBuffer*             buf;     /* +0x2c */
} PlayableSample;

/* ---- DirectSound buffer vtable ----------------------------------------- */
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
    long          (__stdcall *SetFormat)(IDSBuffer*, const void*);             /* +0x38 */
    long          (__stdcall *SetVolume)(IDSBuffer*, long);                    /* +0x3c */
    long          (__stdcall *SetPan)(IDSBuffer*, long);                       /* +0x40 */
    long          (__stdcall *SetFrequency)(IDSBuffer*, unsigned long);        /* +0x44 */
    long          (__stdcall *Stop)(IDSBuffer*);                               /* +0x48 */
    long          (__stdcall *Unlock)(IDSBuffer*, void*, unsigned long,
                                      void*, unsigned long);                   /* +0x4c */
    long          (__stdcall *Restore)(IDSBuffer*);                            /* +0x50 */
} IDSBufferVtbl;

struct IDSBuffer { IDSBufferVtbl* lpVtbl; };

/* ---- sample-system globals --------------------------------------------- */
extern int             g_samples_ready;   /* 0x007988c0  sample system up */
extern int             g_sfx_muted;       /* 0x007988c4  Mute_SFX / UnMute_FX */
extern PlayableSample* g_playable_list;   /* 0x007988cc  head of live samples */

extern int  StopPlayableSample(PlayableSample* s);   /* 0x004927b0 (internal) */
extern int  StartPlayableSample(PlayableSample* s);  /* 0x004928a0 (internal) */
#ifndef LEGOLAND_PORTABLE
extern void UpdateSampleSource(PlayableSample* s);   /* 0x004966a0 (internal) */
#else
extern int UpdateSampleSource(PlayableSample* s);   /* 0x004966a0 (internal) */
#endif

/* ---- pause / resume / mute --------------------------------------------- */

/* Stop a sample and mark it singly paused, unless it already is. */
// FUNCTION: LEGOLAND 0x00492800
int PauseSingleSample(PlayableSample* s)
{
    if (!(s->flags & 1)) {
        if (StopPlayableSample(s)) {
            s->flags |= 1;
            return 1;
        }
    }
    return 0;
}

/* Stop every live sample and latch the mute flag; resumed samples stay silent
 * until UnMute_FX. */
// FUNCTION: LEGOLAND 0x00492870
void Mute_SFX(void)
{
    PlayableSample* s = g_playable_list;

    while (s) {
        StopPlayableSample(s);
        s = s->next;
    }
    g_sfx_muted = 1;
}

/* Clear the singly-paused bit; the sample only actually restarts while SFX are
 * not muted. */
// FUNCTION: LEGOLAND 0x00492910
int ResumeSinglyPausedSample(PlayableSample* s)
{
    if (g_samples_ready && s && (s->flags & 1)) {
        s->flags &= ~1;
        if (!g_sfx_muted)
            StartPlayableSample(s);
        return 1;
    }
    return 0;
}

/* ---- fade / 3D sourcing ------------------------------------------------ */

/* Set the per-tick fade rate on a live sample. The bare `ret` after the null
 * check (no `xor eax,eax`) is VC6 reusing eax, which it knows is zero there. */
// FUNCTION: LEGOLAND 0x00492af0
int SetSampleFade(PlayableSample* s, int fade)
{
    if (!g_samples_ready)
        return 0;
    if (!s)
        return 0;
    if (!s->def)
        return 0;
    s->fade = fade;
    return 1;
}

/* Detach a live sample from whatever it was positionally sourced to. */
// FUNCTION: LEGOLAND 0x004969f0
int UnSourcePlayableSample(PlayableSample* s)
{
    if (!g_samples_ready)
        return 0;
    if (!s)
        return 0;
    if (!s->def)
        return 0;
    s->srcmode = 0;
    UpdateSampleSource(s);
    return 1;
}

/* ---- DirectMusic ------------------------------------------------------- */

typedef struct IDMPerformance IDMPerformance;
typedef struct IDMPerformanceVtbl {
    char pad00[0x88];
    long (__stdcall *SetGlobalParam)(IDMPerformance*, const void* guid,
                                     void* param, unsigned long size); /* +0x88 */
} IDMPerformanceVtbl;
struct IDMPerformance { IDMPerformanceVtbl* lpVtbl; };

typedef struct IDMBand IDMBand;
typedef struct IDMBandVtbl {
    long (__stdcall *QueryInterface)(IDMBand*, const void*, void**); /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDMBand*);                     /* +0x04 */
    unsigned long (__stdcall *Release)(IDMBand*);                    /* +0x08 */
    long (__stdcall *CreateSegment)(IDMBand*, void** segment);       /* +0x0c */
} IDMBandVtbl;
struct IDMBand { IDMBandVtbl* lpVtbl; };

extern int            g_music_sys; /* 0x004bf774  music ENABLED flag (startup.c:255 stores `-nomusic` == 0) */
extern int            g_music_ready;       /* 0x0079a694  music system up */
extern IDMPerformance* g_dm_performance;   /* 0x007cacdc  IDirectMusicPerformance */
extern const char     GUID_PerfMasterGrooveLevel[]; /* 0x004ab6d0 */

/* Push a new groove level to the performance (GUID_PerfMasterGrooveLevel takes
 * a single byte). */
// FUNCTION: LEGOLAND 0x00495b90
void SetMusicGrooveLevel(char level)
{
    if (g_music_sys && g_music_ready)
        g_dm_performance->lpVtbl->SetGlobalParam(g_dm_performance,
                                                 GUID_PerfMasterGrooveLevel,
                                                 &level, 1);
}

/* Build a segment out of a band; TRUE on SUCCEEDED(). */
// FUNCTION: LEGOLAND 0x00496110
int CreateMusicBandSegment(IDMBand* band, void** segment)
{
    if (!g_music_sys)
        return 0;
    if (!g_music_ready)
        return 0;
    return band->lpVtbl->CreateSegment(band, segment) >= 0;
}

/* ---- AVI sound buffer -------------------------------------------------- */

/* The most recent Lock's two write regions, kept in globals so the unlock can
 * hand them back. */
extern void*         g_avi_lock_ptr1;  /* 0x007988a4 */
extern unsigned long g_avi_lock_len1;  /* 0x00798898 */
extern void*         g_avi_lock_ptr2;  /* 0x007988a8 */
extern unsigned long g_avi_lock_len2;  /* 0x0079889c */

/* Seek then play (looping) — Play only runs if the seek succeeded. */
// FUNCTION: LEGOLAND 0x004963d0
long KLIBAUDIO_PlayAVISoundBuffer(IDSBuffer* buf, unsigned long pos)
{
    long hr = buf->lpVtbl->SetCurrentPosition(buf, pos);
    if (hr != 0)
        return hr;
    return buf->lpVtbl->Play(buf, 0, 0, 1);
}

/* Give back the regions of the last Lock. */
// FUNCTION: LEGOLAND 0x00496490
void KLIBAUDIO_UnLockAVISoundBuffer(IDSBuffer* buf)
{
    if (buf)
        buf->lpVtbl->Unlock(buf, g_avi_lock_ptr1, g_avi_lock_len1,
                            g_avi_lock_ptr2, g_avi_lock_len2);
}
