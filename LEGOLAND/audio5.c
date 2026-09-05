/* LEGOLAND — narration (speech) stream header parsing and the positional
 * sample-source helpers that pair with sysmisc.c's UpdateSampleSource.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field OFFSETS, vtable slots and callee argument counts are load-bearing —
 * every name here is ours.
 */

/* ---- the Sample record (audio3.c's recovery, fields this file touches) --- */
typedef struct IDSBuffer IDSBuffer;

typedef struct MapRef {
    int x;                      /* +0x00 */
    int y;                      /* +0x04 */
} MapRef;

typedef struct SoundSource {
    int    kind;                /* +0x00  0 none / 1 bloke / 2 map ref / 3 xy */
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

/* IDirectSoundBuffer, the two slots this file calls (audio3.c has the rest). */
typedef struct IDSBufferVtbl {
    char pad00[0x24];
    long (__stdcall *GetStatus)(IDSBuffer*, unsigned long*);   /* +0x24 */
    char pad28[0x14];
    long (__stdcall *SetVolume)(IDSBuffer*, long);             /* +0x3c */
} IDSBufferVtbl;
struct IDSBuffer { IDSBufferVtbl* lpVtbl; };

/* ---- globals ----------------------------------------------------------- */
extern int     g_samples_ready;    /* 0x007988c0  sample system up */
extern Sample* g_playable_list;    /* 0x007988cc  head of live instances */
extern int     g_sfx_master_db;    /* 0x007988a0  SFX master attenuation */

/* A packed map square. Restaurant2_StartSound takes one BY VALUE: the original
 * forwards the caller's dword and masks each byte out of it, which is the
 * union-by-value shape, not the `unsigned short` its caller ridecb3.c
 * declares. Caller-side extern types are levers — ridecb3.c's declaration is
 * left alone. */
typedef struct BPos  { unsigned char x, y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

/* One row of a ride's FX table (stride 0xc), as audiomisc.c/loaders.c
 * recovered it. NOTE screencb.c's copy names +0x04 `sample` and +0x08 `flags`;
 * the loaded sample is at +0x08 — 0x004b6970 and 0x004b697c are entries 0 and
 * 1 of the three-entry restaurant table at 0x004b6968. */
typedef struct FXEntry {
    char* name;                 /* +0x00 */
    int   pad4;                 /* +0x04 */
    void* sample;               /* +0x08 */
} FXEntry;

extern FXEntry g_rest2_fx[3];      /* 0x004b6968 */

/* ---- callees ----------------------------------------------------------- */
extern int   UpdateSampleSource(Sample* s); /* 0x004966a0 (internal, sysmisc.c) */
extern void* PlayInstanceOfSample(void* sample, int a, int b,
                                  SoundSource* src);  /* 0x00496d20 */

/* Drop a playing instance back to the unpositioned master volume — the
 * "no source" arm of UpdateSampleSource. Note it resets only the VOLUME; the
 * pan the last positional update wrote is left as it was (original
 * behaviour, not a transcription slip). */
// FUNCTION: LEGOLAND 0x00496660
int ClearSampleSource(Sample* s)
{
    if (!g_samples_ready)
        return 0;
    if (!s)
        return 0;
    if (!s->def)
        return 0;
    s->buf->lpVtbl->SetVolume(s->buf, g_sfx_master_db);
    return 1;
}

/* Re-push every live instance's positional volume/pan after the SFX master
 * attenuation changed (UpdateSoundVols calls this straight after storing
 * g_sfx_master_db). The GetStatus call is used only as a liveness probe — the
 * status word it writes is never read. */
// FUNCTION: LEGOLAND 0x004967b0
void RefreshSampleVolumes(void)
{
    unsigned long status;
    Sample* s;

    if (!g_samples_ready)
        return;
    s = g_playable_list;
    while (s) {
        if (s->def) {
            if (s->buf->lpVtbl->GetStatus(s->buf, &status) == 0)
                UpdateSampleSource(s);
        }
        s = s->next;
    }
}

/* Start the restaurant's two looping effects, both sourced at the placement's
 * map square (source kind 2 = a map reference). The source's +0x04 (`obj`) is
 * never written — for kind 2 UpdateSampleSource never reads it. */
// FUNCTION: LEGOLAND 0x0042fb00
void Restaurant2_StartSound(BPosW square)
{
    SoundSource src;

    src.kind = 2;
    src.pos.x = square.b.x;
    src.pos.y = square.b.y;
    PlayInstanceOfSample(g_rest2_fx[0].sample, 0, 1, &src);
    PlayInstanceOfSample(g_rest2_fx[1].sample, 1, 1, &src);
}

/* ================================================== narration wave header == */

/* The WAVEFORMATEX the speech decoder feeds to acmStreamOpen (audio4.c's
 * PlayNarrationFile is the caller). +0x10 is cbSize. */
typedef struct WaveFormat {
    unsigned short tag;          /* +0x00 */
    unsigned short channels;     /* +0x02 */
    unsigned long  rate;         /* +0x04 */
    unsigned long  bytesPerSecond; /* +0x08 */
    unsigned short blockAlign;   /* +0x0c */
    unsigned short bits;         /* +0x0e */
    unsigned short extra;        /* +0x10  cbSize */
} WaveFormat;

extern int         g_speech_fd;            /* 0x007caca8 */
extern WaveFormat* g_speech_source_format; /* 0x007cacb0 */
/* First named here, from RewindNarrationSource (0x00498120), which seeks back
 * to g_speech_data_start and reloads g_speech_data_left from the size. */
extern long        g_speech_data_start;    /* 0x007cacb4  file offset of the
                                              data chunk's payload */
extern unsigned int g_speech_data_size;    /* 0x0079ac04  the data chunk's
                                              byte count */

extern void* malloc(unsigned int bytes);   /* 0x0049e4ff */
extern void  HeapFree_w(void* p);          /* 0x0049e4d0 */
extern int   _read(int fd, void* buf, unsigned int n);  /* 0x0049f4ca */
extern long  _lseek(int fd, long offset, int origin);   /* 0x004a56c3 */
extern long  _tell(int fd);                             /* 0x004aacbd */

/* Parse the speech .wav's header from the top of g_speech_fd and leave the
 * file positioned at the first byte of sample data.
 *
 * THE FORMAT IT HONOURS (runtime spec):
 *   'RIFF' <u32 size, READ AND DISCARDED> 'WAVE'
 *   then EXACTLY ONE chunk header that is assumed to be 'fmt ' — the four-byte
 *   id is read and NEVER COMPARED, so any chunk in that position is taken as
 *   the format chunk. Its payload is read whole into a malloc'd block of
 *   max(size, 18) bytes, which becomes g_speech_source_format; when the chunk
 *   is 18 bytes or shorter cbSize is forced to 0 (a 16-byte PCM 'fmt ' chunk
 *   carries no cbSize, so the pad-to-18 plus this store is what makes the
 *   block a valid WAVEFORMATEX).
 *   then a chunk walk: every chunk that is not 'data' is skipped by
 *   malloc/read/free of its whole payload (not seeked over), and the walk ends
 *   at 'data', whose u32 size goes to g_speech_data_size and whose payload
 *   offset (_tell) goes to g_speech_data_start.
 *
 * WHAT IT REJECTS: a short read anywhere, a first tag that is not 'RIFF', a
 * form type that is not 'WAVE'. Everything else — a chunk id in the 'fmt '
 * slot that is not 'fmt ', an odd-length chunk with no RIFF pad byte, a
 * missing 'data' chunk (it reads off the end and fails on the short read) — is
 * either accepted or falls out of the short-read test.
 *
 * ORIGINAL BUGS reproduced: the RIFF size is read into the same local the
 * chunk sizes use and then ignored; the 'fmt ' id is never checked; the skip
 * path allocates the whole of every unwanted chunk instead of seeking past it;
 * and both mallocs are used unchecked. */
// FUNCTION: LEGOLAND 0x00498420
int ReadNarrationWaveHeader(void)
{
    unsigned int size;
    unsigned int tag;
    void* skip;

    _lseek(g_speech_fd, 0, 0);
    if (_read(g_speech_fd, &tag, 4) != 4)
        return 0;
    if (tag != 0x46464952)                      /* 'RIFF' */
        return 0;
    if (_read(g_speech_fd, &size, 4) != 4)
        return 0;
    if (_read(g_speech_fd, &tag, 4) != 4)
        return 0;
    if (tag != 0x45564157)                      /* 'WAVE' */
        return 0;
    if (_read(g_speech_fd, &tag, 4) != 4)       /* assumed 'fmt ', not checked */
        return 0;
    if (_read(g_speech_fd, &size, 4) != 4)
        return 0;
    if (size < 0x12)
        g_speech_source_format = (WaveFormat*)malloc(0x12);
    else
        g_speech_source_format = (WaveFormat*)malloc(size);
    if ((unsigned int)_read(g_speech_fd, g_speech_source_format, size) != size)
        return 0;
    if (size <= 0x12)
        g_speech_source_format->extra = 0;
    /* The chunk walk. Two spelling levers here, both measured:
     *
     * (1) `for (;;)` with the read guard INSIDE the loop, NOT
     *     `while (_read(...) == 4) { ... } return 0;`. The two are the same
     *     CFG — VC6 rotates this form itself, peeling the read into the
     *     pre-loop copy at 0x0049854d and leaving the tag test as the loop
     *     header at 0x00498567 — but a trailing `return 0` block at the end of
     *     the function makes VC6 retarget all nine EARLIER guards' failure
     *     branches to it (`jne <shared>`, 151 instructions, 489 B); with no
     *     such block every guard keeps its own inline
     *     `xor eax,eax / pop esi / add esp,8 / ret`, which is the original's
     *     eleven copies (187 instructions).
     *
     * (2) the two chunk-size read failures must reach ONE `return 0` through a
     *     `goto`. Written as two separate `return 0;` statements the body is
     *     instruction-for-instruction exact but four bytes long: VC6 merges the
     *     skip arm's copy BACKWARDS into the first guard's block and needs a
     *     rel32 `jne`, where the original's is a two-byte `jne` forward to the
     *     data arm's copy 121 bytes away. The shared label pins the target. */
    for (;;) {
        if (_read(g_speech_fd, &tag, 4) != 4)
            return 0;
        if (tag != 0x61746164) {                /* not 'data': skip the chunk */
            if (_read(g_speech_fd, &size, 4) != 4)
                goto chunkfail;
            skip = malloc(size);
            if ((unsigned int)_read(g_speech_fd, skip, size) != size) {
                HeapFree_w(skip);
                return 0;
            }
            HeapFree_w(skip);
        } else {
            if (_read(g_speech_fd, &g_speech_data_size, 4) != 4)
                goto chunkfail;
            g_speech_data_start = _tell(g_speech_fd);
            return 1;
        }
        continue;
chunkfail:
        return 0;
    }
}
