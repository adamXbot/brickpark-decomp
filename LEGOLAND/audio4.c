/* LEGOLAND: MIDI chunk readers and native sample/narration lifetimes. */
typedef struct MidiTrack {
    void* owner; int length; unsigned char* data;
    int pos, pad10, time; short active, pending;
} MidiTrack;
typedef struct IDSBuffer IDSBuffer;
typedef struct IDSBufferVtbl {
    char pad00[0x3c];
    long (__stdcall *SetVolume)(IDSBuffer*, long);
    char pad40[8];
    long (__stdcall *Stop)(IDSBuffer*);
} IDSBufferVtbl;
struct IDSBuffer { IDSBufferVtbl* lpVtbl; };
typedef struct Sample {
    char pad00[0x1c]; unsigned short flags; char pad1e[0xa];
    struct Sample* def; IDSBuffer* buf;
} Sample;
typedef struct DSCaps { unsigned long size; char data[0x5c]; } DSCaps;
typedef struct IDSound IDSound;
typedef struct IDSoundVtbl {
    char pad00[8];
    unsigned long (__stdcall *Release)(IDSound*);
    void* CreateSoundBuffer;
    long (__stdcall *GetCaps)(IDSound*, DSCaps*);
    void* DuplicateSoundBuffer;
    long (__stdcall *SetCooperativeLevel)(IDSound*, void*, unsigned long);
} IDSoundVtbl;
struct IDSound { IDSoundVtbl* lpVtbl; };

extern int RES_ReadFile(void* f, void* buf, int len); /* 0x00489cf0 */
extern void* malloc(unsigned int bytes); /* 0x0049e4ff */
extern void* memset(void* dest, int value, unsigned int count); /* intrinsic, no call */
extern char* strcpy(char*, const char*); /* intrinsic, no call */
extern char* strcat(char*, const char*); /* intrinsic, no call */
#pragma intrinsic(memset, strcpy, strcat)
extern long __stdcall DirectSoundCreate(const void*, IDSound**, void*); /* 0x0049d31a */
extern int g_samples_ready; /* 0x007988c0 */
extern IDSound* g_dsound; /* 0x007cad40 */
extern DSCaps g_dsound_caps; /* 0x007cace0 */
extern void ReadBE32(void* f, void* dest); /* 0x00480150 */

typedef struct WaveFormat {
    unsigned short tag, channels;
    unsigned long rate, bytesPerSecond;
    unsigned short blockAlign, bits, extra;
} WaveFormat;
typedef struct ACMHeader {
    unsigned long size, status, user;
    void* src; unsigned long srcLength, srcUsed, srcUser;
    void* dst; unsigned long dstLength, dstUsed, dstUser;
    unsigned long reserved[10];
} ACMHeader;

extern int g_speech_state; /* 0x0079a84c */
extern IDSBuffer* g_speech_buffer; /* 0x0079a848 */
extern long g_speech_db; /* 0x0079a7d0 */
extern int g_speech_fd; /* 0x007caca8 */
extern WaveFormat* g_speech_source_format; /* 0x007cacb0 */
extern void* g_speech_acm; /* 0x007cacb8 */
extern unsigned long g_speech_chunk_size; /* 0x007caca0 */
extern void* g_speech_source; /* 0x0079ac0c */
extern void* g_speech_decoded; /* 0x0079ac08 */
extern WaveFormat g_speech_pcm; /* 0x007cacc0 */
extern ACMHeader g_speech_header; /* 0x007aac40 */
extern const char g_res_path[]; /* 0x00813b04 */

extern int __stdcall acmStreamUnprepareHeader(void*, ACMHeader*, unsigned long); /* 0x0049e3c4 */
extern int __stdcall acmStreamClose(void*, unsigned long); /* 0x0049e3b2 */
extern int __stdcall acmStreamOpen(void**, void*, WaveFormat*, WaveFormat*, void*, unsigned long, unsigned long, unsigned long); /* 0x0049e3be */
extern int __stdcall acmStreamSize(void*, unsigned long, unsigned long*, unsigned long); /* 0x0049e3b8 */
extern int __stdcall acmStreamPrepareHeader(void*, ACMHeader*, unsigned long); /* 0x0049e3d0 */
extern int _open(const char*, int, ...); /* 0x0049f6c0 */
extern int _close(int); /* 0x0049f417 */
extern int sprintf(char*, const char*, ...); /* 0x0049e573 */
extern void HeapFree_w(void*); /* 0x0049e4d0 */
extern int ReadNarrationWaveHeader(void); /* 0x00498420 */
extern void RewindNarrationSource(void); /* 0x00498120 */
extern void ResetNarrationStreamState(void); /* 0x00498870 */
#ifndef LEGOLAND_PORTABLE
extern void StopNarrationPlayback(void); /* 0x004988c0 */
#else
extern int StopNarrationPlayback(void); /* 0x004988c0 */
#endif
extern IDSBuffer* KLIBAUDIO_CreateAVISoundBuffer(WaveFormat*, unsigned long); /* 0x00496360 */
extern int KLIBAUDIO_DestroyAVISoundBuffer(IDSBuffer*); /* 0x004964c0 */

// FUNCTION: LEGOLAND 0x00480170
void ReadBE16(void* f, void* dest)
{
    RES_ReadFile(f, dest, 2);
#ifndef LEGOLAND_PORTABLE
    __asm {
        mov eax, dest
        movzx edx, word ptr [eax]
        xchg dh, dl
        mov word ptr [eax], dx
    }
#else
    *(unsigned short*)dest = (unsigned short)__builtin_bswap16(*(unsigned short*)dest);
#endif
}

// FUNCTION: LEGOLAND 0x00480150
void ReadBE32(void* f, void* dest)
{
    RES_ReadFile(f, dest, 4);
#ifndef LEGOLAND_PORTABLE
    __asm {
        mov eax, dest
        mov edx, dword ptr [eax]
        bswap edx
        mov dword ptr [eax], edx
    }
#else
    *(unsigned int*)dest = __builtin_bswap32(*(unsigned int*)dest);
#endif
}

// FUNCTION: LEGOLAND 0x004927b0
int StopPlayableSample(Sample* s)
{
    if (!g_samples_ready) return 0;
    if (!s) return 0;
    if (!s->def) return 0;
    if (s->buf->lpVtbl->Stop(s->buf) != 0) return 0;
    s->flags |= 2;
    return 1;
}

// FUNCTION: LEGOLAND 0x004801a0
MidiTrack* ReadMidiTrack(void* f)
{
    int chunk, length;
    MidiTrack* t = (MidiTrack*)malloc(sizeof(MidiTrack));
    RES_ReadFile(f, &chunk, 4);
    ReadBE32(f, &length);
    t->length = length;
    t->data = (unsigned char*)malloc(length);
    RES_ReadFile(f, t->data, length);
    t->active = 0;
    return t;
}

/* Historical caller name retained: this DESTROYS speech, not a resumable pause. */
// FUNCTION: LEGOLAND 0x00498920
int PauseCurrentTrack(void)
{
    if (!g_speech_state) return 0;
    StopNarrationPlayback();
    acmStreamUnprepareHeader(g_speech_acm, &g_speech_header, 0);
    acmStreamClose(g_speech_acm, 0);
    _close(g_speech_fd);
    KLIBAUDIO_DestroyAVISoundBuffer(g_speech_buffer);
    g_speech_buffer = 0;
    HeapFree_w(g_speech_source);
    HeapFree_w(g_speech_decoded);
    HeapFree_w(g_speech_source_format);
    g_speech_state = 0;
    return 1;
}

// FUNCTION: LEGOLAND 0x00492130
int InitSoundSampleSystem(void* hwnd)
{
    if (DirectSoundCreate(0, &g_dsound, 0) != 0) goto fail;
    if (g_dsound->lpVtbl->SetCooperativeLevel(g_dsound, hwnd, 1) != 0)
        goto release;
    memset(&g_dsound_caps, 0, sizeof(g_dsound_caps));
    g_dsound_caps.size = sizeof(g_dsound_caps);
    if (g_dsound->lpVtbl->GetCaps(g_dsound, &g_dsound_caps) != 0) {
release:
        g_dsound->lpVtbl->Release(g_dsound);
fail:
        g_dsound = 0;
        g_samples_ready = 0;
        return 0;
    }
    g_samples_ready = 1;
    return 1;
}

/* Prepare only; ResumeCurrentTrack starts the new speech stream. Original
 * unchecked allocations/API results and unbounded path operations retained. */
// FUNCTION: LEGOLAND 0x00498630
int PlayNarrationFile(const char* name)
{
    char path[1024];
    unsigned long output_size;
    if (!g_samples_ready) return 0;
    strcpy(path, "speech\\");
    strcat(path, name);
    if (g_speech_state) return 0;
    g_speech_fd = _open(path, 0x8000);
    if (g_speech_fd == -1) {
        sprintf(path, "%s%s%s", g_res_path, "speech\\", name);
        g_speech_fd = _open(path, 0x8000);
        if (g_speech_fd == -1) return 0;
    }
    if (ReadNarrationWaveHeader()) {
        RewindNarrationSource();
        g_speech_chunk_size = g_speech_source_format->blockAlign * 10;
        g_speech_source = malloc(g_speech_chunk_size);
        g_speech_decoded = 0;
        g_speech_pcm.tag = 1;
        g_speech_pcm.channels = g_speech_source_format->channels;
        g_speech_pcm.rate = g_speech_source_format->rate;
        g_speech_pcm.bits = 16;
        g_speech_pcm.blockAlign = g_speech_pcm.channels * 2;
        g_speech_pcm.bytesPerSecond = g_speech_pcm.blockAlign * g_speech_pcm.rate;
        g_speech_pcm.extra = 0;
        acmStreamOpen(&g_speech_acm, 0, g_speech_source_format, &g_speech_pcm, 0, 0, 0, 4);
        acmStreamSize(g_speech_acm, g_speech_chunk_size, &output_size, 0);
        g_speech_decoded = malloc(output_size);
        g_speech_header.src = g_speech_source;
        g_speech_header.size = sizeof(g_speech_header);
        g_speech_header.status = 0;
        g_speech_header.srcLength = g_speech_chunk_size;
        g_speech_header.dst = g_speech_decoded;
        g_speech_header.dstLength = output_size;
        acmStreamPrepareHeader(g_speech_acm, &g_speech_header, 0);
        g_speech_buffer = KLIBAUDIO_CreateAVISoundBuffer(&g_speech_pcm, 0xa000);
        g_speech_buffer->lpVtbl->SetVolume(g_speech_buffer, g_speech_db);
        g_speech_state = 1;
        ResetNarrationStreamState();
        return 1;
    }
    _close(g_speech_fd);
    return 0;
}
