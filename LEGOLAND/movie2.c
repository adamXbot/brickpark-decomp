/* LEGOLAND -- the movie player's audio stream: the AVI audio track is pulled
 * through Video for Windows, decoded through ACM when it is ADPCM, and fed a
 * frame's worth at a time into a KLIBAUDIO streaming DirectSound buffer that
 * RunMovie (movie.c) keeps topped up as frames are shown.  Also the movie
 * clock and the two sound-effect pause flags.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, global addresses and callee argument counts are load-bearing;
 * names are ours.
 *
 * ---------------------------------------------------------------------------
 * THE STREAMING BUFFER.  StartMovieAudio reads the audio stream's format and,
 * for PCM, creates a DirectSound buffer of `g_movie_audio_scale` frame-blocks
 * of `bytesPerSecond / fps` bytes; the stream is then read straight into it
 * `samplesPerSecond / fps` samples per block.  For ADPCM (format tags 2 and
 * 0x11) it opens an ACM conversion stream to 16-bit PCM of the same channel
 * count and rate, sizes the source read for one block of output, and keeps
 * a three-block decoded buffer plus an ACM header; the source is read 256
 * samples at a time (AVISTREAMREAD_CONVENIENT-style: a quarter block of
 * source bytes) and converted with ACM_STREAMCONVERTF_BLOCKALIGN until a
 * block's worth of PCM has accumulated, with the surplus carried over as
 * `g_mva_leftover` for the next block.
 *
 * UpdateMovieAudio(frame, prev) fills the blocks between the frame shown
 * last and the frame shown now: block `b` of the buffer is locked and filled
 * from the stream position that is tracked in g_mva_pos, then the block
 * cursor wraps at the block count.  Called with (0, 0) it primes the first
 * eleven blocks from position 0; when the caller has fallen a whole buffer
 * behind (frame - prev >= blocks) it stops the buffer, re-seeks the stream to
 * `frame`, refills eleven blocks from the block the frame maps to, and
 * restarts playback at that block's byte offset with the volume reapplied.
 * Past the end of the stream a block is filled with silence -- 0x80 bytes
 * for 8-bit audio, zero otherwise -- and the position still advances.
 *
 * PrimeMovieAudio positions the stream at its start and plays; StopMovieAudio
 * stops, frees the ACM buffers and stream and destroys the buffer.
 *
 * MovieTicks is a millisecond clock: QueryPerformanceCounter scaled by
 * 1000 / frequency when the counter exists, GetTickCount otherwise, the
 * choice latched on first use.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

#pragma intrinsic(memcpy, memset)
void* memcpy(void*, const void*, unsigned int);
void* memset(void*, int, unsigned int);

/* ---- types -------------------------------------------------------------- */

typedef struct IDSBuffer IDSBuffer;

/* movie.c's Movie record; only the fields read here. */
typedef struct Movie {
    int   frames;     /* +0x00 */
    int   fps;        /* +0x04 dwRate / dwScale, in Hz */
    int   width;      /* +0x08 */
    int   height;     /* +0x0c */
    void* file;       /* +0x10 */
    void* getframe;   /* +0x14 */
    void* audio;      /* +0x18 PAVISTREAM */
    void* video;      /* +0x1c */
} Movie;

/* WAVEFORMATEX as audio4.c spells it. */
typedef struct WaveFormat {
    unsigned short tag, channels;                  /* +0x00, +0x02 */
    unsigned long  rate, bytesPerSecond;           /* +0x04, +0x08 */
    unsigned short blockAlign, bits, extra;        /* +0x0c, +0x0e, +0x10 */
} WaveFormat;

/* ACMSTREAMHEADER (0x54 bytes) as audio4.c spells it. */
typedef struct ACMHeader {
    unsigned long size, status, user;              /* +0x00 */
    void* src; unsigned long srcLength, srcUsed, srcUser;   /* +0x0c */
    void* dst; unsigned long dstLength, dstUsed, dstUser;   /* +0x1c */
    unsigned long reserved[10];                    /* +0x2c */
} ACMHeader;

/* AVISTREAMINFOA (0x8c bytes); only dwSampleSize (+0x30) is read. */
typedef struct AviStreamInfo {
    unsigned long fccType, fccHandler, dwFlags, dwCaps;   /* +0x00 */
    unsigned short wPriority, wLanguage;                  /* +0x10 */
    unsigned long dwScale, dwRate, dwStart, dwLength;     /* +0x14 */
    unsigned long dwInitialFrames, dwSuggestedBufferSize, dwQuality, dwSampleSize; /* +0x24 */
    long rcFrame[4];                                      /* +0x34 */
    unsigned long dwEditCount, dwFormatChangeCount;       /* +0x44 */
    char szName[64];                                      /* +0x4c */
} AviStreamInfo;

/* ---- globals (the movie-audio state block, first named here) ------------- */
extern int         g_mva_acm_used;          /* 0x00668ee0 1 = decoding through ACM */
extern ACMHeader   g_mva_hdr;               /* 0x00668ee8 */
extern unsigned int g_mva_bytes_per_block;  /* 0x00668f3c PCM bytes per movie frame */
extern int         g_mva_acm_open_rc;       /* 0x00668f44 */
extern IDSBuffer*  g_mva_buffer;            /* 0x00668f48 */
extern int         g_mva_bits;              /* 0x00668f4c wBitsPerSample */
extern unsigned int g_mva_blocks;           /* 0x00668f50 blocks in the buffer (= g_movie_audio_scale) */
extern int         g_mva_dst_offset;        /* 0x00668f54 consumed offset into g_mva_dst_buf */
extern int         g_mva_start;             /* 0x00668f58 AVIStreamStart */
extern void*       g_mva_acm;               /* 0x00668f5c HACMSTREAM */
extern int         g_mva_pos;               /* 0x00668f60 stream position of the next block */
extern int         g_mva_sample_size;       /* 0x00668f64 the stream's dwSampleSize */
extern WaveFormat  g_mva_wf;                /* 0x00668f70 the PCM format we play */
extern void*       g_mva_stream;            /* 0x00668f84 PAVISTREAM */
extern int         g_mva_end;               /* 0x00668f88 start + length */
extern char*       g_mva_dst_buf;           /* 0x00668f8c decoded PCM, three blocks */
extern unsigned int g_mva_samples_per_block;/* 0x00668f90 */
extern int         g_mva_playing;           /* 0x00668f9c */
extern unsigned int g_mva_block;            /* 0x00668fa0 next buffer block to fill */
extern int         g_movie_audio_scale;     /* 0x00668fa4 */
extern int         g_movie_timer_mode;      /* 0x00668fac 0 unknown, 1 GetTickCount, 2 QueryPerformanceCounter */
extern unsigned int g_mva_leftover;         /* 0x00668fb4 decoded bytes not yet copied out */
extern float       g_movie_perf_scale;      /* 0x007fdca8 1000 / QueryPerformanceFrequency */
extern float       g_movie_volume;          /* 0x004bb4dc 1.0f */
extern int         g_sfx_paused;            /* 0x007988c8 */

/* ---- callees ------------------------------------------------------------ */
extern void* HeapAlloc_w(unsigned int n);                           /* 0x0049e4ff (CRT malloc) */
extern void  HeapFree_w(void* p);                                   /* 0x0049e4d0 */

/* AVIFile and ACM entry points, called through the linker's thunks WITHOUT
 * __declspec(dllimport) as movie.c and audio4.c spell them. */
long __stdcall AVIStreamInfoA(void* pavi, void* psi, long size);    /* 0x0049e3ee */
long __stdcall AVIStreamReadFormat(void* pavi, long pos, void* fmt, long* size); /* 0x0049e41e */
/* DECLARATION ORDER IS A LEVER HERE: PrimeMovieAudio sums the two calls in
 * one expression, and VC6 evaluates the LATER-declared callee first, so
 * Start must be declared before Length for Length to be called first
 * (relocs.py caught the swap; the instruction gate cannot). */
long __stdcall AVIStreamStart(void* pavi);                          /* 0x0049e42a */
long __stdcall AVIStreamLength(void* pavi);                         /* 0x0049e424 */
long __stdcall AVIStreamRead(void* pavi, long start, long samples, void* buf,
                             long size, long* bytes, long* nsamples); /* 0x0049e430 */
int  __stdcall acmStreamOpen(void**, void*, WaveFormat*, WaveFormat*, void*,
                             unsigned long, unsigned long, unsigned long); /* 0x0049e3be */
int  __stdcall acmStreamSize(void*, unsigned long, unsigned long*, unsigned long); /* 0x0049e3b8 */
int  __stdcall acmStreamClose(void*, unsigned long);                 /* 0x0049e3b2 */
int  __stdcall acmStreamPrepareHeader(void*, ACMHeader*, unsigned long);   /* 0x0049e3d0 */
int  __stdcall acmStreamConvert(void*, ACMHeader*, unsigned long);         /* 0x0049e3ca */
int  __stdcall acmStreamUnprepareHeader(void*, ACMHeader*, unsigned long); /* 0x0049e3c4 */

__declspec(dllimport) int __stdcall QueryPerformanceFrequency(__int64* f); /* IAT 0x004ab108 */
__declspec(dllimport) int __stdcall QueryPerformanceCounter(__int64* c);   /* IAT 0x004ab118 */
__declspec(dllimport) unsigned long __stdcall GetTickCount(void);          /* IAT 0x004ab1f8 */

extern IDSBuffer* KLIBAUDIO_CreateAVISoundBuffer(WaveFormat* fmt, unsigned long bytes); /* 0x00496360 */
extern int   KLIBAUDIO_DestroyAVISoundBuffer(IDSBuffer* buf);       /* 0x004964c0 */
extern long  KLIBAUDIO_StopAVISoundBuffer(IDSBuffer* buf);          /* 0x004964d0 */
/* audiomisc.c defines this with ONE parameter; both callers here push a
 * second (the volume) -- an extern-type divergence kept as the disassembly
 * needs it. */
extern long  KLIBAUDIO_SetAVIVolume(IDSBuffer* buf, int volume);    /* 0x004964e0 */
extern long  KLIBAUDIO_PlayAVISoundBuffer(IDSBuffer* buf, unsigned long pos); /* 0x004963d0 */
extern void* KLIBAUDIO_LockAVISoundBuffer(IDSBuffer* buf, unsigned long offset, unsigned long len); /* 0x004963f0 */
#ifndef LEGOLAND_PORTABLE
extern long  KLIBAUDIO_UnLockAVISoundBuffer(IDSBuffer* buf);        /* 0x00496490 */
#else
extern void KLIBAUDIO_UnLockAVISoundBuffer(IDSBuffer* buf);        /* 0x00496490 */
#endif

extern int   UpdateMovieAudio(int frame, int prev);                 /* 0x00476d20 */
/* The CRT's float-to-int helper every (int) cast of a float calls; declared
 * only so relocs.py can resolve the call target. */
extern long  _ftol(double);                                          /* 0x00458930 */


/* =========================================================================
 *  SetSfxPaused / ClearSfxPaused
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00492980
void SetSfxPaused(void)
{
    g_sfx_paused = 1;
}

// FUNCTION: LEGOLAND 0x00492990
void ClearSfxPaused(void)
{
    g_sfx_paused = 0;
}


/* =========================================================================
 *  MovieTicks -- the movie clock, in milliseconds
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00476680
unsigned int MovieTicks(void)
{
    __int64 freq;
    __int64 now;

    if (g_movie_timer_mode == 0) {
        if (QueryPerformanceFrequency(&freq) == 0) {
            g_movie_timer_mode = 1;
        } else {
            g_movie_timer_mode = 2;
            g_movie_perf_scale = 1000.0f / (float)freq;
        }
    }
    switch (g_movie_timer_mode) {
    case 2:
        QueryPerformanceCounter(&now);
#ifndef LEGOLAND_PORTABLE
        return (int)((float)now * g_movie_perf_scale);
#else
        return LL_FISTP((float)now * g_movie_perf_scale); /* PORT-M5: 0x004766e5 */
#endif
    default:
        return GetTickCount();
    }
}


/* =========================================================================
 *  SetMovieVolume -- 0x004771e0, the undeclared setter beside the updater
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004771e0
int SetMovieVolume(float volume)
{
    g_movie_volume = volume;
    return 1;
}


/* =========================================================================
 *  StopMovieAudio
 * ========================================================================= */

/* movie.c declares this void; the body returns 0 when nothing was playing
 * and the destroy result otherwise. */
// FUNCTION: LEGOLAND 0x00476c90
int StopMovieAudio(void)
{
    if (g_mva_playing) {
        g_mva_playing = 0;
        KLIBAUDIO_StopAVISoundBuffer(g_mva_buffer);
        if (g_mva_acm_used) {
            if (g_mva_hdr.src) {
                HeapFree_w(g_mva_hdr.src);
                g_mva_hdr.src = 0;
            }
            if (g_mva_dst_buf) {
                HeapFree_w(g_mva_dst_buf);
                g_mva_dst_buf = 0;
            }
            acmStreamClose(g_mva_acm, 0);
        }
        return KLIBAUDIO_DestroyAVISoundBuffer(g_mva_buffer);
    }
    return 0;
}


/* =========================================================================
 *  PrimeMovieAudio -- seek to the stream's start, fill and play
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00476bf0
int PrimeMovieAudio(Movie* mv)
{
    int start;
    int end;

    if (mv->audio == 0 || g_mva_buffer == 0)
        return 0;
    start = AVIStreamStart(mv->audio);
    /* vfw.h's AVIStreamEnd(): the original calls Length, then Start.  The
     * order comes from the two prototypes' declaration order (see them), not
     * from the operand order; split into two statements it swaps edi/esi. */
    end = AVIStreamStart(mv->audio) + AVIStreamLength(mv->audio);
    g_mva_start = start;
    g_mva_pos = start;
    g_mva_end = end;
    g_mva_playing = 1;
    UpdateMovieAudio(0, 0);
#ifndef LEGOLAND_PORTABLE
    KLIBAUDIO_SetAVIVolume(g_mva_buffer, (int)g_movie_volume);
#else
    KLIBAUDIO_SetAVIVolume(g_mva_buffer, LL_FISTP(g_movie_volume)); /* PORT-M5 */
#endif
    KLIBAUDIO_PlayAVISoundBuffer(g_mva_buffer, 0);
    return 1;
}


/* =========================================================================
 *  StartMovieAudio -- open the stream's format and build the buffer
 * ========================================================================= */

/* The PCM WaveFormat fill is written out twice in the original -- once
 * before the format-tag test and again inside the ADPCM arm -- with the same
 * `mov ecx,0x10` serving both `bits` stores. */
#define FILL_PCM_FORMAT(fmt) \
    g_mva_wf.tag = 1; \
    g_mva_wf.channels = (fmt)->channels; \
    g_mva_wf.rate = (fmt)->rate; \
    g_mva_wf.blockAlign = (fmt)->channels * 2; \
    g_mva_wf.bytesPerSecond = g_mva_wf.blockAlign * (fmt)->rate; \
    g_mva_wf.bits = 16; \
    g_mva_wf.extra = 0;

// FUNCTION: LEGOLAND 0x00476910
int StartMovieAudio(Movie* mv)
{
    struct {
        long          fmtsize;
        AviStreamInfo si;
    } f;
    WaveFormat*   fmt;
    unsigned int  bpb;
    unsigned int  blocks;

    if (mv->audio == 0)
        return 0;
    if (AVIStreamInfoA(mv->audio, &f.si, 0x8c) != 0)
        return 0;
    g_mva_sample_size = f.si.dwSampleSize;
    AVIStreamReadFormat(mv->audio, 0, 0, &f.fmtsize);
    fmt = (WaveFormat*)HeapAlloc_w(f.fmtsize);
    if (fmt == 0)
        return 0;
    AVIStreamReadFormat(mv->audio, 0, fmt, &f.fmtsize);
    FILL_PCM_FORMAT(fmt)
    if (fmt->tag == 2 || fmt->tag == 0x11) {
        g_mva_acm_used = 1;
        FILL_PCM_FORMAT(fmt)
        g_mva_acm_open_rc = acmStreamOpen(&g_mva_acm, 0, fmt, &g_mva_wf, 0, 0, 0, 4);
        if (g_mva_acm_open_rc != 0)
            return 0;
        g_mva_playing = 0;
        blocks = g_movie_audio_scale;
        g_mva_blocks = blocks;
        bpb = g_mva_wf.bytesPerSecond / mv->fps;
        g_mva_bytes_per_block = bpb;
        g_mva_samples_per_block = g_mva_wf.rate / mv->fps;
        g_mva_bits = g_mva_wf.bits;
        g_mva_stream = mv->audio;
        g_mva_buffer = KLIBAUDIO_CreateAVISoundBuffer(&g_mva_wf, bpb * blocks);
        memset(&g_mva_hdr, 0, sizeof(g_mva_hdr));
        g_mva_hdr.size = sizeof(g_mva_hdr);
        g_mva_hdr.dstLength = g_mva_bytes_per_block;
        acmStreamSize(g_mva_acm, g_mva_bytes_per_block, &g_mva_hdr.srcLength, 1);
        g_mva_hdr.src = HeapAlloc_w(g_mva_hdr.srcLength);
        g_mva_dst_buf = (char*)HeapAlloc_w(g_mva_hdr.dstLength * 3);
        g_mva_hdr.dst = g_mva_dst_buf;
        g_mva_dst_offset = 0;
        if (g_mva_hdr.src == 0 || g_mva_hdr.dst == 0) {
            acmStreamClose(g_mva_acm, 0);
            if (g_mva_hdr.src)
                HeapFree_w(g_mva_hdr.src);
            if (g_mva_hdr.dst)
                HeapFree_w(g_mva_hdr.dst);
            return 0;
        }
        return 1;
    }
    g_mva_acm_used = 0;
    g_mva_playing = 0;
    blocks = g_movie_audio_scale;
    g_mva_blocks = blocks;
    bpb = fmt->bytesPerSecond / mv->fps;
    g_mva_bytes_per_block = bpb;
    g_mva_samples_per_block = fmt->rate / mv->fps;
    g_mva_bits = fmt->bits;
    g_mva_stream = mv->audio;
    g_mva_buffer = KLIBAUDIO_CreateAVISoundBuffer(fmt, bpb * blocks);
    return 1;
}


/* =========================================================================
 *  UpdateMovieAudio -- fill the buffer blocks between two frames
 * ========================================================================= */

/* One decode step: read a quarter block of source samples from the stream at
 * g_mva_pos, convert it into the decoded buffer at `fill`, account for it.
 * The dead `prev` parameter's slot is the samples-read out-pointer. */
#define DECODE_STEP() \
    AVIStreamRead(g_mva_stream, g_mva_pos, 0x100, g_mva_hdr.src, \
                  g_mva_bytes_per_block >> 2, &bytes, (long*)&prev); \
    g_mva_hdr.dst = g_mva_dst_buf + fill; \
    acmStreamPrepareHeader(g_mva_acm, &g_mva_hdr, 0); \
    acmStreamConvert(g_mva_acm, &g_mva_hdr, 0x10); \
    acmStreamUnprepareHeader(g_mva_acm, &g_mva_hdr, 0); \
    fill += g_mva_hdr.dstUsed; \
    g_mva_leftover += g_mva_hdr.dstUsed; \
    g_mva_pos++;

// FUNCTION: LEGOLAND 0x00476d20
int UpdateMovieAudio(int frame, int prev)
{
    long          bytes;
    unsigned int  i;
    int           restart;
    unsigned int  restart_pos;
    unsigned int  count;
    char*         lock;
    int           fill;
    unsigned int  need;

    restart = 0;
    if (g_mva_playing != 0 && g_mva_stream != 0) {
        if (frame == 0 && prev == 0) {
            count = 11;
            g_mva_pos = 0;
            g_mva_block = 0;
        } else {
            count = frame - prev;
            if (count >= g_mva_blocks) {
                KLIBAUDIO_StopAVISoundBuffer(g_mva_buffer);
                g_mva_pos = g_mva_samples_per_block * frame;
                restart = 1;
                count = 11;
                g_mva_block = frame % g_mva_blocks;
                restart_pos = g_mva_samples_per_block * g_mva_block;
            }
        }
        if (count != 0) {
            i = count;
            do {
                lock = (char*)KLIBAUDIO_LockAVISoundBuffer(g_mva_buffer,
                                                           g_mva_bytes_per_block * g_mva_block,
                                                           g_mva_bytes_per_block);
                if (g_mva_pos < g_mva_end && g_mva_pos >= 0) {
                    if (g_mva_acm_used) {
                        fill = 0;
                        if (g_mva_leftover) {
                            if (g_mva_leftover < g_mva_bytes_per_block) {
                                memcpy(lock, g_mva_dst_buf + g_mva_dst_offset, g_mva_leftover);
                                need = g_mva_bytes_per_block - g_mva_leftover;
                                g_mva_leftover = 0;
                                g_mva_dst_offset = 0;
                                if (g_mva_pos >= g_mva_end) {
                                    if (g_mva_bits == 8)
                                        memset(lock + g_mva_bytes_per_block - need, 0x80, need);
                                    else
                                        memset(lock + g_mva_bytes_per_block - need, 0, need);
                                } else {
                                    while (g_mva_leftover < g_mva_bytes_per_block) {
                                        DECODE_STEP()
                                    }
                                    memcpy(lock + g_mva_bytes_per_block - need, g_mva_dst_buf, need);
                                    g_mva_dst_offset += need;
                                    g_mva_leftover -= need;
                                }
                            } else {
                                memcpy(lock, g_mva_dst_buf + g_mva_dst_offset, g_mva_bytes_per_block);
                                g_mva_leftover -= g_mva_bytes_per_block;
                                if (g_mva_leftover != 0)
                                    g_mva_dst_offset += g_mva_bytes_per_block;
                                else
                                    g_mva_dst_offset = 0;
                            }
                        } else {
                            while (g_mva_leftover < g_mva_bytes_per_block) {
                                DECODE_STEP()
                            }
                            memcpy(lock, g_mva_dst_buf + g_mva_dst_offset, g_mva_bytes_per_block);
                            g_mva_dst_offset += g_mva_bytes_per_block;
                            g_mva_leftover -= g_mva_bytes_per_block;
                        }
                    } else {
                        AVIStreamRead(g_mva_stream, g_mva_pos, g_mva_samples_per_block, lock,
                                      g_mva_bytes_per_block, &bytes, (long*)&prev);
                    }
                } else {
                    if (g_mva_bits == 8)
                        memset(lock, 0x80, g_mva_bytes_per_block);
                    else
                        memset(lock, 0, g_mva_bytes_per_block);
                    if (g_mva_acm_used)
                        g_mva_pos++;
                }
                KLIBAUDIO_UnLockAVISoundBuffer(g_mva_buffer);
                if (g_mva_acm_used == 0)
                    g_mva_pos += g_mva_samples_per_block;
                g_mva_block++;
                if (g_mva_block >= g_mva_blocks)
                    g_mva_block = 0;
            } while (--i);
            if (restart) {
                KLIBAUDIO_PlayAVISoundBuffer(g_mva_buffer, restart_pos);
#ifndef LEGOLAND_PORTABLE
                KLIBAUDIO_SetAVIVolume(g_mva_buffer, (int)g_movie_volume);
#else
                KLIBAUDIO_SetAVIVolume(g_mva_buffer, LL_FISTP(g_movie_volume)); /* PORT-M5 */
#endif
            }
        }
        return 1;
    }
    return 0;
}
