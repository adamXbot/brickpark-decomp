/* LEGOLAND — audio odds and ends: the AVI sound-buffer wrappers, sound-system
 * teardown, the money SFX refcount pair and the generic FX-list loader.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only the
 * struct field offsets, vtable slots and callee arg counts are load-bearing;
 * names are ours.
 */

/* ---- DirectSound buffer vtable ----------------------------------------- */
/* Only the slots the wrappers actually call matter; the rest are here to pin
 * the offsets (Release @ +0x08, SetVolume @ +0x3c, Stop @ +0x48). */
typedef struct IDSBuffer IDSBuffer;

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

/* Release the AVI playback buffer; TRUE when the final reference went away. */
// FUNCTION: LEGOLAND 0x004964c0
int KLIBAUDIO_DestroyAVISoundBuffer(IDSBuffer* buf)
{
    return buf->lpVtbl->Release(buf) == 0;
}

/* Stop the AVI playback buffer; returns the raw HRESULT. */
// FUNCTION: LEGOLAND 0x004964d0
long KLIBAUDIO_StopAVISoundBuffer(IDSBuffer* buf)
{
    return buf->lpVtbl->Stop(buf);
}

/* Restore the AVI buffer to full volume (0 = 0 dB attenuation). */
// FUNCTION: LEGOLAND 0x004964e0
long KLIBAUDIO_SetAVIVolume(IDSBuffer* buf)
{
    return buf->lpVtbl->SetVolume(buf, 0);
}

/* ---- sound-system teardown --------------------------------------------- */

extern int KillSoundSampleSystem(void);  /* 0x00492c20 */
extern int KillMusicSystem(void);        /* 0x00495b00 */

/* Tear the sample system down first; only if that succeeded do we tear down
 * the music system.
 *
 * `return ok;` on the failure path (rather than `return 0;`) is load-bearing:
 * ok is already in eax and known to be zero, so VC6 emits a bare `ret` with no
 * `xor eax,eax` — which is why the original reads
 * `test eax,eax / jne <body> / ret`. Writing this as `a && b` instead produces
 * a normalising `mov eax,1` and does not match. */
// FUNCTION: LEGOLAND 0x00496520
int KillSoundSystem(void)
{
    int ok = KillSoundSampleSystem();
    if (!ok)
        return ok;
    return KillMusicSystem() != 0;
}

/* ---- sound-effect lists ------------------------------------------------- */

/* One row of an FX table: the base sample name and the loaded sample. The
 * tables are static arrays of these (stride 0xc) — e.g. the 0x17-entry game
 * table at 0x004b9228 that InitGameMap registers, and the 2-entry money table
 * at 0x004b87a8 below. */
typedef struct FXEntry {
    char* name;    /* +0x00 */
    int   pad4;    /* +0x04 */
    void* sample;  /* +0x08  SampleDef*, filled in by Load_FXList */
} FXEntry;

/* A loaded sample definition; only the name back-pointer at +0x10 is touched
 * here. */
typedef struct SampleDef {
    char  pad00[0x10]; /* +0x00 */
    char* name;        /* +0x10 */
} SampleDef;

/* A live sound-effect instance: a 16-bit flag word at +0x1c, the tick the
 * callback comes due at +0x20 and the callback itself at +0x24. The flag word
 * has to be 16-bit: VC6 narrows `|= 0x10` on a short to `or byte ptr [..]`,
 * while on a char it goes through a register and on an int it stays a dword. */
typedef struct SFXInstance {
    char           pad00[0x1c]; /* +0x00 */
    unsigned short flags;       /* +0x1c  0x10 = has a completion callback */
    short          pad1e;       /* +0x1e */
    unsigned int   due;         /* +0x20 */
    void*          callback;    /* +0x24 */
} SFXInstance;

extern const char kFXWavPathFmt[];    /* 0x004bfea0  "…%s.wav" */
extern const char kFXLoadFailedFmt[]; /* 0x004bfe88  load-failure message */

extern unsigned int SoundTimeMS(void);                  /* 0x00499450 */
extern SampleDef*   CreateSampleFromWAV(const char* p); /* 0x00492380 */
extern void         DeleteSampleDef(SampleDef* s);      /* 0x00492be0 */
extern void         DBPrintf(const char* fmt, ...);     /* 0x00453a20 */
int sprintf(char*, const char*, ...);

/* Arm a completion callback on a playing effect, due `delay` ticks from now. */
// FUNCTION: LEGOLAND 0x00496db0
void AddSFX_Callback(SFXInstance* sfx, int delay, void* callback)
{
    unsigned int now = SoundTimeMS();

    sfx->due = now + delay;
    sfx->flags |= 0x10;
    sfx->callback = callback;
}

/* Load every sample named by an FX table, giving each loaded sample a
 * back-pointer to its table name. */
// FUNCTION: LEGOLAND 0x00496dd0
void Load_FXList(FXEntry* list, int count)
{
    char path[100];
    int  i;

    for (i = 0; i < count; i++) {
        SampleDef* s;

        sprintf(path, kFXWavPathFmt, list->name);
        s = CreateSampleFromWAV(path);
        list->sample = s;
        if (s != 0)
            s->name = list->name;
        else
            DBPrintf(kFXLoadFailedFmt, list->name);
        list++;
    }
}

/* Free every sample an FX table holds and blank the slots. */
// FUNCTION: LEGOLAND 0x00496e30
void Kill_FXList(FXEntry* list, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        if (list->sample != 0)
            DeleteSampleDef((SampleDef*)list->sample);
        list->sample = 0;
        list++;
    }
}

/* ---- money SFX --------------------------------------------------------- */

extern FXEntry g_money_fx[];    /* 0x004b87a8 — 2 entries */
extern int     g_money_fx_refs; /* 0x00667120 */

/* Refcounted load of the two money effects. */
// FUNCTION: LEGOLAND 0x00453900
void LoadMoneySFX(void)
{
    if (g_money_fx_refs++ == 0)
        Load_FXList(g_money_fx, 2);
}

/* …and the matching release. */
// FUNCTION: LEGOLAND 0x00453930
void KillMoneySFX(void)
{
    if (--g_money_fx_refs == 0)
        Kill_FXList(g_money_fx, 2);
}
