/* LEGOLAND -- scope AK: the per-frame sample housekeeping (fade / auto-kill /
 * callbacks / re-positioning), the sprite-list unlink, the two narration
 * rings (encoded source ring and decoded PCM ring) with their pump, and the
 * string-table loader.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only
 * struct field OFFSETS, vtable slots and callee argument counts are
 * load-bearing; every name here is ours. Verification and recovered
 * mechanics: docs/lanes/scope-ak.md.
 *
 * THE TWO NARRATION RINGS (runtime spec):
 *   Ring A (0x10000 bytes @ 0x0079ac20) holds ENCODED speech straight from
 *   the .wav's data chunk: FillNarrationSourceRing _read()s into it while
 *   there is free space and file left; g_narr_a is its read cursor,
 *   g_narr_b its write cursor. A queue of block sizes (g_narr_queue[20],
 *   count g_narr_c) records how many encoded bytes each "block" of the
 *   source carried -- queue[0] is what is still readable from the current
 *   block, and NarrRingA_Available never hands out more than that.
 *   Ring B (0x20000 bytes @ 0x007aaca0) holds DECODED PCM: RefillNarrationRing
 *   drains ring A into g_speech_source in chunks of at most
 *   g_speech_chunk_size, acmStreamConvert()s each into g_speech_decoded and
 *   copies the result in; g_narr_d / g_narr_e are its read / write cursors.
 *   ReadDecodedNarration (movie3.c) drains ring B; PumpNarration copies
 *   0x1000-byte blocks of it into the 0xa000-byte DirectSound buffer, one
 *   block ahead of the play cursor, and stops the stream when a block with
 *   no data has been played out.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>

#pragma intrinsic(memcpy, memset, strlen, strcpy)

/* ---- CRT ------------------------------------------------------------------
 * 0x0049e4ff malloc, 0x0049e4d0 free, 0x0049f330 fopen, 0x0049f044 fread,
 * 0x0049f284 fseek, 0x0049efee fclose, 0x0049edcc _getcwd, 0x0049ebff _chdir,
 * 0x004a04b9 atoi, 0x004a02b8 exit, 0x004a0858 _isctype (ctype.h macro
 * fallback when __mb_cur_max @ 0x004c033c > 1; _pctype @ 0x004c0130),
 * 0x0049f4ca _read. The headers above give the prototypes. */
extern int _read(int fd, void* buf, unsigned int n);            /* 0x0049f4ca */

/* ---- sample records (audio3.c / audio5.c recovery) ---------------------- */
typedef struct IDSBuffer IDSBuffer;

typedef struct IDSBufferVtbl {
    char pad00[0x10];
    long (__stdcall *GetCurrentPosition)(IDSBuffer*, int*, int*);        /* +0x10 */
    char pad14[0x04];
    long (__stdcall *GetVolume)(IDSBuffer*, int*);                       /* +0x18 */
    char pad1c[0x08];
    long (__stdcall *GetStatus)(IDSBuffer*, unsigned long*);             /* +0x24 */
    char pad28[0x04];
    long (__stdcall *Lock)(IDSBuffer*, unsigned long, unsigned long,
                           void**, int*, void**, int*, unsigned long);   /* +0x2c */
    char pad30[0x0c];
    long (__stdcall *SetVolume)(IDSBuffer*, long);                       /* +0x3c */
    char pad40[0x08];
    long (__stdcall *Stop)(IDSBuffer*);                                  /* +0x48 */
    long (__stdcall *Unlock)(IDSBuffer*, void*, int, void*, int);        /* +0x4c */
} IDSBufferVtbl;
struct IDSBuffer { IDSBufferVtbl* lpVtbl; };

typedef struct SampleDef {
    char  pad00[0x10];          /* +0x00 */
    char* name;                 /* +0x10 */
} SampleDef;

typedef struct Sample Sample;
typedef int (*SampleCallback)(Sample* s);

struct Sample {
    Sample*        next;        /* +0x00 */
    int            refcount;    /* +0x04 */
    int            fade;        /* +0x08  per-tick volume delta, 0 = not fading */
    int            src_kind;    /* +0x0c  0 = unpositioned */
    int            src_obj;     /* +0x10 */
    int            src_x;       /* +0x14 */
    int            src_y;       /* +0x18 */
    unsigned short flags;       /* +0x1c  2 stopped, 8 auto-kill, 0x10 callback armed */
    short          pad1e;       /* +0x1e */
    int            due;         /* +0x20  tick the callback comes due */
    SampleCallback callback;    /* +0x24 */
    SampleDef*     def;         /* +0x28 */
    IDSBuffer*     buf;         /* +0x2c */
};

/* One row of a ride's FX table (stride 0xc; audiomisc.c). */
typedef struct FXEntry {
    char*      name;            /* +0x00 */
    int        pad4;            /* +0x04 */
    SampleDef* sample;          /* +0x08 */
} FXEntry;

/* The global sprite list's link (sprite2.c has the full record). */
typedef struct SpriteRec {
    struct SpriteRec* next;     /* +0x00 */
} SpriteRec;

/* The hashed string table's node (text.c). */
typedef struct StrNode {
    int             id;         /* +0x00 */
    char*           text;       /* +0x04 */
    struct StrNode* next;       /* +0x08 */
} StrNode;

/* ACMSTREAMHEADER (audio4.c). */
typedef struct ACMHeader {
    unsigned long size, status, user;
    void* src; unsigned long srcLength, srcUsed, srcUser;
    void* dst; unsigned long dstLength, dstUsed, dstUser;
    unsigned long reserved[10];
} ACMHeader;

/* ---- globals ------------------------------------------------------------ */
extern int      g_samples_ready;       /* 0x007988c0  sample system up */
extern Sample*  g_playable_list;       /* 0x007988cc  head of live instances */
extern FXEntry  g_joust_fx[];          /* 0x004b4688  "Joust Horses.wav" (joust.c) */
extern SpriteRec* g_sprites_head;      /* 0x0079a7c0 */

extern int      g_vol_speech;          /* 0x0080ffc4  slider 0..100 */
extern int      g_vol_sfx;             /* 0x0080ffcc  slider 0..100 */
extern int      g_6687b0;              /* 0x006687b0  hold-off frame counter */

/* Ring A -- the encoded source ring and its block-size queue. */
extern unsigned char g_narr_src_ring[0x10000]; /* 0x0079ac20 (first named here) */
extern int      g_narr_a;              /* 0x0079a7d8  ring A read cursor */
extern int      g_narr_b;              /* 0x0079a7dc  ring A write cursor */
extern int      g_narr_c;              /* 0x0079a7e0  blocks queued */
extern int      g_narr_queue[20];      /* 0x0079a7e4  encoded bytes per block */
/* First named here: bit 4 = the source may be rewound (looped) when it runs
 * dry, bit 8 = it has been rewound at least once. Nothing in this scope
 * clears either. */
extern int      g_narr_flags;          /* 0x0079a83c */

/* Ring B -- the decoded PCM ring (movie3.c names). */
extern unsigned char g_narr_ring[0x20000]; /* 0x007aaca0 */
extern int      g_narr_d;              /* 0x0079a834  ring B read cursor */
extern int      g_narr_e;              /* 0x0079a838  ring B write cursor */

/* The DirectSound side of the stream. */
extern int      g_speech_state;        /* 0x0079a84c  0 idle, 1 stopped, 2 wound back, 3 playing */
extern IDSBuffer* g_speech_buffer;     /* 0x0079a848  the 0xa000-byte narration buffer */
extern int      g_speech_fill_block;   /* 0x0079a840  next 0x1000-byte block to fill (0..9) */
extern int      g_speech_blocks_ready; /* 0x0079a844  blocks that carried real data */

/* The ACM decode step. savemisc2.c named 0x007aac24 g_speech_header_flag and
 * 0x007caca4 g_speech_chunk_pos from the reset alone; the bodies here show
 * them to be the read position within, and the byte count of, the decoded
 * chunk in g_speech_decoded. */
extern int      g_speech_fd;           /* 0x007caca8 */
extern int      g_speech_bytes_left;   /* 0x007cacac  encoded bytes still to _read (tinystubs.c: unsigned) */
extern void*    g_speech_acm;          /* 0x007cacb8 */
extern int      g_speech_chunk_size;   /* 0x007caca0  encoded bytes per decode */
extern void*    g_speech_source;       /* 0x0079ac0c  encoded chunk fed to the ACM */
extern void*    g_speech_decoded;      /* 0x0079ac08  the ACM's output */
extern int      g_speech_decoded_pos;  /* 0x007aac24  bytes of g_speech_decoded already ringed */
extern int      g_speech_decoded_len;  /* 0x007caca4  bytes the last convert produced */
extern ACMHeader g_speech_header;      /* 0x007aac40 */

extern StrNode* g_string_buckets[10];  /* 0x0079a850 */

/* ---- strings ------------------------------------------------------------ */
extern const char kFmtFading[];        /* 0x004bfddc "Fading Sample (%s) (%x) to Vol %d\n" */
extern const char kFmtStopForKill[];   /* 0x004bfdb4 "\tStopping Sample for the kill %s (%x)\n" */
extern const char kFmtAutokill[];      /* 0x004bfe14 "Autokilling Sample %s (%x)\n" */
extern const char kMsgKillJoust[];     /* 0x004bfe00 "Killing Joust FX\n" */
extern const char kMsgUnlinkFail[];    /* 0x004bfeb0 "Couldn't unlink sprite\n" */
extern const char kModeR[];            /* 0x004bf2b8 "r" */
extern const char kStabPath[];         /* 0x004bfef4 ".\\strings\\stab.str" */

/* ---- callees ------------------------------------------------------------ */
extern int   DBPrintf(const char* fmt, ...);            /* 0x00453a20 */
extern int   GetTicks(void);                            /* 0x00499450 (jmp [GetTickCount]) */
extern void  UpdateSampleSource(Sample* s);             /* 0x004966a0 (internal, sysmisc.c) */
extern int   FreePlayableSample(Sample* s);             /* 0x00492b20 (audio3.c) */
extern int   UpdateSoundVols(void);                     /* 0x00495a90 */
extern void  InitOptionSamples(void);                   /* 0x00492830  pauses every live sample */
extern int   PauseCurrentTrack(void);                   /* 0x00498920  destroys the speech stream */
extern void  HeapFree_w(void* p);                       /* 0x0049e4d0 */
extern void  RewindNarrationSource(void);               /* 0x00498120 (tinystubs.c) */
extern int   StopNarrationPlayback(void);               /* 0x004988c0 (savemisc2.c) */
extern int   ReadDecodedNarration(void* dst, int len);  /* 0x004983a0 (movie3.c) */
int  __stdcall acmStreamConvert(void*, ACMHeader*, unsigned long); /* 0x0049e3ca (thunk) */

void RefillNarrationRing(void);                         /* 0x00498250 (below) */
void FillNarrationSourceRing(void);                     /* 0x00498000 (below) */
void AddString(const char* text, int id);               /* 0x00498f80 (below) */

/* ========================================================================
 *  Per-frame sample housekeeping (sub_4969d0 runs the four in order)
 * ======================================================================== */

/* Re-push the positional volume/pan of every positioned instance that is
 * actually playing (DSBSTATUS_PLAYING). Same shape as audio5.c's
 * RefreshSampleVolumes plus the source-kind and status-bit tests. */
// FUNCTION: LEGOLAND 0x00496760
void UpdatePlayingSampleSources(void)
{
    unsigned long status;
    Sample* s;

    if (!g_samples_ready)
        return;
    s = g_playable_list;
    while (s) {
        if (s->def && s->src_kind) {
            if (s->buf->lpVtbl->GetStatus(s->buf, &status) == 0 && (status & 1))
                UpdateSampleSource(s);
        }
        s = s->next;
    }
}

/* Step every fading instance by its per-tick delta. A fade that reaches 0 dB
 * ends there; one that drops to -30 dB or below is snapped to silence
 * (-100 dB), and if the instance is auto-kill (flag 8) its buffer is stopped
 * so AutoKillSamples reaps it next frame. */
// FUNCTION: LEGOLAND 0x004967f0
void FadeSamples(void)
{
    int vol;
    Sample* s;

    if (!g_samples_ready)
        return;
    s = g_playable_list;
    while (s) {
        if (s->def && !(s->flags & 2) && s->fade) {
            if (s->buf->lpVtbl->GetVolume(s->buf, &vol) == 0) {
                vol += s->fade;
                DBPrintf(kFmtFading, s->def->name, s, vol);
                if (vol >= 0) {
                    vol = 0;
                    s->fade = 0;
                } else if (vol <= -3000) {
                    vol = -10000;
                    s->fade = 0;
                    if (s->flags & 8) {
                        s->buf->lpVtbl->Stop(s->buf);
                        DBPrintf(kFmtStopForKill, s->def->name, s);
                    }
                }
                s->buf->lpVtbl->SetVolume(s->buf, vol);
            }
        }
        s = s->next;
    }
}

/* Fire the completion callbacks that have come due. A callback's non-zero
 * return re-arms it that many ticks later; zero disarms it. */
// FUNCTION: LEGOLAND 0x004968d0
void RunSampleCallbacks(void)
{
    int now = GetTicks();
    Sample* s;

    if (!g_samples_ready)
        return;
    s = g_playable_list;
    while (s) {
        if (s->def && (s->flags & 0x10) && now >= s->due) {
            int again = s->callback(s);
            if (again)
                s->due = again + now;
            else
                s->flags &= ~0x10;
        }
        s = s->next;
    }
}

/* Reap every auto-kill instance (flag 8, not explicitly stopped) whose
 * DirectSound status has gone to 0 -- i.e. it played out. Note there is no
 * g_samples_ready guard here, unlike its three siblings. */
// FUNCTION: LEGOLAND 0x00496920
void AutoKillSamples(void)
{
    unsigned long status;
    Sample* prev = 0;
    Sample* s = g_playable_list;
    Sample* next;
    unsigned short flags;

    while (s) {
        flags = s->flags;
        next = s->next;
        if ((flags & 8) && !(flags & 2)
            && s->buf->lpVtbl->GetStatus(s->buf, &status) == 0
            && ((status == 0) & 1)) {
            DBPrintf(kFmtAutokill, s->def->name, s);
            if (s->def == g_joust_fx[0].sample)
                DBPrintf(kMsgKillJoust);
            if (prev)
                prev->next = s->next;
            else
                g_playable_list = s->next;
            FreePlayableSample(s);
        } else {
            prev = s;
        }
        s = next;
    }
}

/* The per-frame sample tick GameFrame calls. */
// FUNCTION: LEGOLAND 0x004969d0
void TickSamples(void)
{
    RunSampleCallbacks();
    UpdatePlayingSampleSources();
    FadeSamples();
    AutoKillSamples();
}

/* Ramp both volume sliders down to zero in `step` units every `interval`
 * ticks (busy-waiting between steps), then pause every sample, tear the
 * speech stream down, arm the 4-frame hold-off and put the sliders back. */
// FUNCTION: LEGOLAND 0x00496e60
void FadeOutAllSound(int step, int interval)
{
    int save_speech = g_vol_speech;
    int save_sfx = g_vol_sfx;
    int until;

    while (g_vol_sfx > 0 || g_vol_speech > 0) {
        until = GetTicks();
        g_vol_sfx -= step;
        if (g_vol_sfx <= 0)
            g_vol_sfx = 0;
        g_vol_speech -= step;
        if (g_vol_speech <= 0)
            g_vol_speech = 0;
        UpdateSoundVols();
        until += interval;
        while (until > GetTicks())
            ;
    }
    InitOptionSamples();
    PauseCurrentTrack();
    g_6687b0 = 4;
    g_vol_speech = save_speech;
    g_vol_sfx = save_sfx;
    UpdateSoundVols();
}

/* ========================================================================
 *  Sprite list
 * ======================================================================== */

/* Unlink a sprite record from the global list and free it. A record that is
 * not on the list (or a null one) is logged and still freed. */
// FUNCTION: LEGOLAND 0x004975b0
void UnlinkSprite(SpriteRec* s)
{
    SpriteRec* prev = 0;
    SpriteRec* p = g_sprites_head;

    while (p != s) {
        if (!p)
            break;
        prev = p;
        p = p->next;
    }
    if (!p) {
        DBPrintf(kMsgUnlinkFail);
    } else if (!prev) {
        g_sprites_head = s->next;
    } else {
        prev->next = s->next;
    }
    HeapFree_w(s);
}

/* ========================================================================
 *  Ring A -- the encoded source ring
 * ======================================================================== */

/* Contiguous bytes writable at the write cursor (one byte is always kept
 * free so full and empty differ). */
// FUNCTION: LEGOLAND 0x00497f60
int NarrRingA_FreeSpace(void)
{
    if (g_narr_b >= g_narr_a) {
        if (g_narr_a == 0)
            return 0xffff - g_narr_b;
        return 0x10000 - g_narr_b;
    }
    return g_narr_a - g_narr_b - 1;
}

/* Contiguous bytes readable at the read cursor. */
// FUNCTION: LEGOLAND 0x00497f90
int NarrRingA_Contiguous(void)
{
    if (g_narr_b < g_narr_a)
        return 0x10000 - g_narr_a;
    return g_narr_b - g_narr_a;
}

/* Bytes readable in total, capped at what is left of the current source
 * block. A block whose count has hit zero is retired first (the queue is
 * shifted down and the count decremented).
 *
 * ORIGINAL QUIRK reproduced: queue[0] is read ONCE, before the shift, so on
 * the frame a block retires the cap is the stale zero and the function
 * returns 0 (the original re-uses the tested register both for the
 * queue[19] = 0 store and for the cap; a fresh read after the shift is one
 * instruction and nine bytes longer). Declaring `cur` before `avail` is
 * what puts the read cursor in edi (the other order gives esi). */
// FUNCTION: LEGOLAND 0x00497fb0
int NarrRingA_Available(void)
{
    int cur = g_narr_queue[0];
    int avail = (g_narr_b - g_narr_a) & 0xffff;

    if (cur == 0) {
        memcpy(g_narr_queue, g_narr_queue + 1, 19 * sizeof(int));
        g_narr_queue[19] = 0;
        if (g_narr_c)
            g_narr_c--;
    }
    if (avail > cur)
        avail = cur;
    return avail;
}

/* Wind the source back to the start of its data and remember that we did. */
// FUNCTION: LEGOLAND 0x00498100
void RewindNarrationAndFlag(void)
{
    RewindNarrationSource();
    g_narr_flags |= 8;
}

/* _read() encoded source into ring A while it has room and the file has
 * bytes. Every read is credited to the current block's queue entry. A read
 * that comes up short ends the block: with the loop flag set the source is
 * rewound once it is exhausted (and a new block opened), otherwise a new
 * block is opened and the fill stops. */
// FUNCTION: LEGOLAND 0x00498000
void FillNarrationSourceRing(void)
{
    int space;
    int got;

    if (g_speech_bytes_left == 0)
        return;
    space = NarrRingA_FreeSpace();
    while (space) {
        while (space) {
            /* Two full calls in the source (VC6 cross-jumps them into one;
             * a ternary in the argument position emits only one push set). */
            if (space > g_speech_bytes_left)
                got = _read(g_speech_fd, g_narr_src_ring + g_narr_b, g_speech_bytes_left);
            else
                got = _read(g_speech_fd, g_narr_src_ring + g_narr_b, space);
            if (got != -1) {
                g_speech_bytes_left -= got;
                g_narr_queue[g_narr_c] += got;
                g_narr_b = (g_narr_b + got) & 0xffff;
            }
            if (got < space) {
                if (g_narr_flags & 4) {
                    if (g_speech_bytes_left == 0) {
                        RewindNarrationAndFlag();
                        g_narr_c++;
                    }
                } else {
                    g_narr_c++;
                    return;
                }
            }
            space -= got;
        }
        space = NarrRingA_FreeSpace();
    }
}

/* Drain up to `len` encoded bytes out of ring A into `dst`, topping the ring
 * up from the file before (when short) and after. movie3.c's
 * ReadDecodedNarration is the same source over ring B, minus the block
 * accounting. */
// FUNCTION: LEGOLAND 0x00498150
int ReadNarrationSource(void* dst, int len)
{
    int avail;
    int total;
    int chunk;

    if (len > NarrRingA_Available()) {
        FillNarrationSourceRing();
        avail = NarrRingA_Available();
        if (len > avail)
            len = avail;
    }
    total = len;
    if (len) {
        do {
            chunk = NarrRingA_Contiguous();
            if (chunk > len)
                chunk = len;
            memcpy(dst, g_narr_src_ring + g_narr_a, chunk);
            len -= chunk;
            dst = (char*)dst + chunk;
            g_narr_a = (g_narr_a + chunk) & 0xffff;
            g_narr_queue[0] -= chunk;
        } while (len);
    }
    FillNarrationSourceRing();
    return total;
}

/* ========================================================================
 *  Ring B -- the decoded PCM ring
 * ======================================================================== */

/* Contiguous bytes writable at the write cursor. */
// FUNCTION: LEGOLAND 0x004981e0
int NarrationFreeSpace(void)
{
    if (g_narr_e >= g_narr_d) {
        if (g_narr_d == 0)
            return 0x1ffff - g_narr_e;
        return 0x20000 - g_narr_e;
    }
    return g_narr_d - g_narr_e - 1;
}

/* Contiguous bytes readable at the read cursor. */
// FUNCTION: LEGOLAND 0x00498210
int NarrationContiguous(void)
{
    if (g_narr_e < g_narr_d)
        return 0x20000 - g_narr_d;
    return g_narr_e - g_narr_d;
}

/* Bytes readable in total. */
// FUNCTION: LEGOLAND 0x00498230
int NarrationBytesReady(void)
{
    return (g_narr_e - g_narr_d) & 0x1ffff;
}

/* Top ring B up: first whatever is left of the last decoded chunk, then
 * decode further chunks (at most g_speech_chunk_size encoded bytes each,
 * never more than the current source block holds) until the ring is full.
 * A decode that produces LESS than the remaining space is copied whole and
 * the function returns at once (no final source-ring top-up, and the write
 * cursor is advanced without the wrap mask -- it cannot wrap there). */
// FUNCTION: LEGOLAND 0x00498250
void RefillNarrationRing(void)
{
    int space;
    int n;

    FillNarrationSourceRing();
    space = NarrationFreeSpace();
    while (space) {
        while (space) {
            n = g_speech_decoded_len - g_speech_decoded_pos;
            if (n == 0)
                break;
            if (space < n)
                n = space;
            memcpy(g_narr_ring + g_narr_e,
                   (char*)g_speech_decoded + g_speech_decoded_pos, n);
            g_narr_e = (g_narr_e + n) & 0x1ffff;
            space -= n;
            g_speech_decoded_pos += n;
        }
        if (space) {
            n = g_narr_queue[0];
            if (n >= g_speech_chunk_size)
                n = g_speech_chunk_size;
            g_speech_header.srcLength = n;
            ReadNarrationSource(g_speech_source, n);
            acmStreamConvert(g_speech_acm, &g_speech_header, 0x10);
            n = g_speech_header.dstUsed;
            g_speech_decoded_pos = 0;
            g_speech_decoded_len = n;
            if (n < space) {
                memcpy(g_narr_ring + g_narr_e, g_speech_decoded, n);
                g_speech_decoded_pos = n;
                g_narr_e = g_narr_e + n;
                return;
            }
            memcpy(g_narr_ring + g_narr_e, g_speech_decoded, space);
            g_speech_decoded_pos = space;
            g_narr_e = (g_narr_e + space) & 0x1ffff;
        }
        space = NarrationFreeSpace();
    }
    FillNarrationSourceRing();
}

/* ========================================================================
 *  The per-frame pump into the DirectSound buffer
 * ======================================================================== */

/* Called every frame. States 0/1 do nothing; state 2 (wound back, not yet
 * playing) only keeps ring B topped up. State 3: while the play cursor is
 * not inside the block we would fill next, lock that 0x1000-byte block,
 * fill it from ring B (zero-padding a short read), advance the fill index
 * mod 10, and count a block DOWN for every block written; when the count
 * hits zero the buffer has played its last real block and the stream is
 * stopped. Returns 1 when the play cursor has caught up with the fill
 * block, 0 otherwise. */
// FUNCTION: LEGOLAND 0x00498b40
int PumpNarration(void)
{
    char block[0x1000];
    void* ptr;
    int play;
    int bytes;
    int write;
    int n;

    if (g_speech_state == 0 || g_speech_state == 1)
        return 0;
    if (g_speech_state == 2) {
        RefillNarrationRing();
        return 0;
    }
    /* The catch-up test is written TWICE in the source -- a guard and the
     * do/while latch. A single `while (...)` is not loop-inverted by VC6
     * when the condition carries a call, and comes out 20 instructions
     * short with the `return 1` exiled. */
    if (g_speech_buffer->lpVtbl->GetCurrentPosition(g_speech_buffer, &play, &write) == 0
        && play >= (g_speech_fill_block << 12)
        && play < ((g_speech_fill_block + 1) << 12))
        return 1;
    do {
        if (g_speech_buffer->lpVtbl->Lock(g_speech_buffer, g_speech_fill_block << 12, 0x1000,
                                          &ptr, &bytes, 0, 0, 0) == 0) {
            n = ReadDecodedNarration(block, bytes);
            if (n == 0x1000) {
                memcpy(ptr, block, 0x1000);
            } else {
                memcpy(ptr, block, n);
                memset((char*)ptr + n, 0, 0x1000 - n);
            }
            if (n)
                g_speech_blocks_ready++;
            g_speech_buffer->lpVtbl->Unlock(g_speech_buffer, ptr, bytes, 0, 0);
            g_speech_fill_block++;
            if (g_speech_fill_block >= 10)
                g_speech_fill_block = 0;
        }
        if (--g_speech_blocks_ready == 0) {
            StopNarrationPlayback();
            return 0;
        }
    } while (!(g_speech_buffer->lpVtbl->GetCurrentPosition(g_speech_buffer, &play, &write) == 0
               && play >= (g_speech_fill_block << 12)
               && play < ((g_speech_fill_block + 1) << 12)));
    return 1;
}

/* ========================================================================
 *  String table
 * ======================================================================== */

/* The next byte of the loaded file, or 0 at its end (pos is NOT advanced
 * then). Inlined at every site in the original. */
#define NEXT_CHAR() (pos < size ? buf[pos++] : 0)

/* Load .\strings\stab.str into the string table.
 *
 * THE FORMAT IT HONOURS (runtime spec): a free-form text of tokens. A run of
 * digits sets the CURRENT ID (atoi). A token starting with a letter or
 * punctuation character is a string: it is copied into `str` up to the
 * second unpaired '"' -- so `"..."` yields the text between the quotes, a
 * doubled `""` inside yields one '"', and the last char before the second
 * quote is not stored -- and AddString(str, id) files it under the current
 * id. Anything else (whitespace) is skipped. The whole file is read into
 * one heap buffer first, sized by a feof/ferror read loop.
 *
 * ORIGINAL BUGS reproduced: a missing stab.str is exit(1); a string that
 * never closes its quotes runs past the end of the buffer (NEXT_CHAR yields
 * 0 forever) and off the end of `str`; more than three digits overflow the
 * four-byte `num`; a number that ends AT the end of the file backs `pos`
 * up onto its own last digit (the "unread the terminator" step does not
 * know no terminator was read) and the parse loops forever; an unquoted
 * string can never see a second quote; `id` is used before any number has
 * been seen; a failed _chdir back leaks the buffer. */
// FUNCTION: LEGOLAND 0x00498d00
void LoadStrings(void)
{
    /* Declaration order is a lever here: `buf` declared before `size` is
     * what gives buf the loop register (edi) and leaves size in its frame
     * slot; the other way round swaps them (34-line residual). */
    char* buf;
    int size = 0;
    int pos = 0;
    char cwd[0x100];
    char str[0xf0] = { 0 };
    char num[4];
    int id;
    FILE* f;
    char c;
    int n;
    int quotes;

    if (!_getcwd(cwd, 0x100))
        return;
    f = fopen(kStabPath, kModeR);
    if (!f)
        exit(1);
    while (!feof(f)) {
        n = fread(str, 1, 0xf0, f);
        if (ferror(f))
            break;
        size += n;
    }
    buf = (char*)malloc(size + 1);
    fseek(f, 0, 0);
    fread(buf, 1, size, f);
    fclose(f);
    if (_chdir(cwd))
        return;

    for (;;) {
        c = NEXT_CHAR();
        if (!c)
            break;
        if (isdigit(c)) {
            n = 0;
            while (isdigit(c)) {
                num[n++] = c;
                c = NEXT_CHAR();
            }
            if (pos)
                pos--;
            num[n] = 0;
            id = atoi(num);
        } else if (isalpha(c) | ispunct(c)) {
            n = 0;
            quotes = 0;
            for (;;) {
                if (c == '"') {
                    c = NEXT_CHAR();
                    if (c == '"')
                        c = '"';        /* doubled quote: a literal '"' (the
                                         * no-op store is a layout lever) */
                    else
                        quotes++;
                }
                if (quotes == 2)
                    break;
                str[n++] = c;
                c = NEXT_CHAR();
            }
            str[n] = 0;
            AddString(str, id);
        }
    }
    free(buf);
}

/* Allocate a node for (id, text) and push it on its id % 10 bucket. */
// FUNCTION: LEGOLAND 0x00498f80
void AddString(const char* text, int id)
{
    StrNode* node = (StrNode*)malloc(sizeof(StrNode));
    int h = id % 10;

    node->next = g_string_buckets[h];
    g_string_buckets[h] = node;
    node->text = (char*)malloc(strlen(text) + 1);
    strcpy(node->text, text);
    node->id = id;
}
