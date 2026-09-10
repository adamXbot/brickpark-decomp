/* LEGOLAND — playable-sample control: volume / pan / frequency, positional
 * sourcing, lifetime (create / kill / delete), the master directory lookups
 * and the volume-slider push.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only the
 * struct field offsets, vtable slots and callee arg counts are load-bearing;
 * names are ours.
 */

/* ---- the Sample record ------------------------------------------------- */
/* One record type serves both roles. A *definition* (CreateSampleFromWAV) is a
 * Sample whose +0x28 is null (or points at the definition it aliases — Create-
 * PlayableSample walks that chain to the root); a *playable* instance is a
 * Sample whose +0x28 points at its definition. The instance list hangs off
 * g_playable_list through +0x00.
 *   +0x04 refcount   (definition: live instances; instance: bumped to 1)
 *   +0x08 fade rate
 *   +0x0c..0x1b SoundSource {kind, obj, x, y} — PlayInstanceOfSample copies the
 *         caller's 16-byte record straight in; kind 0 none / 1 bloke /
 *         2 map ref / 3 level xy
 *   +0x1c u16 flags  bit0 singly paused, bit1 (cleared by UnSourceAndFade),
 *         bit3 fading, bit4 has callback, bit5
 *   +0x28 definition (instance) / alias parent (definition)
 *   +0x2c IDirectSoundBuffer (definition: the master; instance: a duplicate)
 *   +0x30, +0x34 heap blocks owned by a definition (freed by DeleteSampleDef)
 */
typedef struct IDSBuffer IDSBuffer;
typedef struct IDSound   IDSound;

/* A map / level position. SourcePlayableSampleToMapRef and ...ToLevelXY take
 * one BY VALUE (8 bytes on the stack): the callee copies it into the source
 * record as a unit, which is what lets VC6 schedule the `push` for
 * UpdateSampleSource between the two halves of the copy. */
typedef struct MapRef {
    int x;       /* +0x00 */
    int y;       /* +0x04 */
} MapRef;

typedef struct SoundSource {
    int    kind; /* +0x00 */
    void*  obj;  /* +0x04 */
    MapRef pos;  /* +0x08 */
} SoundSource;

typedef struct Sample {
    struct Sample* next;     /* +0x00 */
    int            refcount; /* +0x04 */
    int            fade;     /* +0x08 */
    SoundSource    src;      /* +0x0c */
    unsigned short flags;    /* +0x1c */
    short          pad1e;    /* +0x1e */
    unsigned int   due;      /* +0x20 */
    void*          callback; /* +0x24 */
    struct Sample* def;      /* +0x28 */
    IDSBuffer*     buf;      /* +0x2c */
    void*          data30;   /* +0x30 */
    void*          data34;   /* +0x34 */
} Sample;

/* ---- DirectSound vtables ----------------------------------------------- */
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

typedef struct IDSoundVtbl {
    long          (__stdcall *QueryInterface)(IDSound*, const void*, void**);  /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDSound*);                              /* +0x04 */
    unsigned long (__stdcall *Release)(IDSound*);                             /* +0x08 */
    long          (__stdcall *CreateSoundBuffer)(IDSound*, const void*, IDSBuffer**, void*); /* +0x0c */
    long          (__stdcall *GetCaps)(IDSound*, void*);                      /* +0x10 */
    long          (__stdcall *DuplicateSoundBuffer)(IDSound*, IDSBuffer*, IDSBuffer**); /* +0x14 */
} IDSoundVtbl;
struct IDSound { IDSoundVtbl* lpVtbl; };

/* ---- globals ----------------------------------------------------------- */
extern int      g_samples_ready;   /* 0x007988c0  sample system up */
extern int      g_sfx_muted;       /* 0x007988c4  Mute_SFX / UnMute_FX latch */
extern int      g_sfx_paused;      /* 0x007988c8  new instances start paused */
extern Sample*  g_playable_list;   /* 0x007988cc  head of live instances */
extern int      g_sfx_master_db;   /* 0x007988a0  SFX master attenuation */
extern IDSound* g_dsound;          /* 0x007cad40  IDirectSound */
extern IDSBuffer* g_music_buf;     /* 0x007cad4c  music stream buffer */
extern void*    g_music_sys;       /* 0x004bf774 */
extern int      g_music_ready;     /* 0x0079a694 */
extern int      g_vol_speech;      /* 0x0080ffc4  slider 0..100 */
extern int      g_vol_music;       /* 0x0080ffc8  slider 0..100 */
extern int      g_vol_sfx;         /* 0x0080ffcc  slider 0..100 */

extern const char kUnSourceFadeFmt[];  /* 0x004bfe58 */
extern const char kUnSourceClearFmt[]; /* 0x004bfe30 */

/* ---- callees ----------------------------------------------------------- */
extern int     StartPlayableSample(Sample* s);   /* 0x004928a0 (internal) */
#ifndef LEGOLAND_PORTABLE
extern void    UpdateSampleSource(Sample* s);    /* 0x004966a0 (internal) */
#else
extern int UpdateSampleSource(Sample* s);    /* 0x004966a0 (internal) */
#endif
#ifndef LEGOLAND_PORTABLE
extern void    ClearSampleSource(Sample* s);     /* 0x00496660 (internal) */
#else
extern int ClearSampleSource(Sample* s);     /* 0x00496660 (internal) */
#endif
extern Sample* MakePlayable(void);               /* 0x00492110 (internal) */
extern int     PlaySample(Sample* s, int a, int b); /* 0x00492710 */
extern int     PauseSingleSample(Sample* s);     /* 0x00492800 */
extern void    RefreshSampleVolumes(void);       /* 0x004967b0 (internal) */
extern void    SetSpeechVolume(long db);         /* 0x00498900 (internal) */
extern void    DBPrintf(const char* fmt, ...);   /* 0x00453a20 */
#ifndef LEGOLAND_PORTABLE
extern int     HeapFree_w(void* p);              /* 0x0049e4d0 */
#else
extern void HeapFree_w(void* p);              /* 0x0049e4d0 */
#endif
extern int     _stricmp(const char* a, const char* b); /* 0x004aab90 (CRT) */
extern int     rand(void);                       /* 0x0049e4b2 (CRT) */

int  FreePlayableSample(Sample* s);   /* 0x00492b20 (internal, below) */
int  GetSampleFrequency(Sample* s);   /* 0x00492a60 (internal, below) */
long VolToDB(int slider);             /* 0x00495a50 (internal, below) */

/* ---- mute -------------------------------------------------------------- */

/* Restart every live sample and clear the mute latch (mirror of Mute_SFX). */
// FUNCTION: LEGOLAND 0x00492950
void UnMute_FX(void)
{
    Sample* s = g_playable_list;

    while (s) {
        StartPlayableSample(s);
        s = s->next;
    }
    g_sfx_muted = 0;
}

/* ---- volume / pan / frequency ------------------------------------------ */

/* vol is 0..100; DirectSound wants hundredths of a dB of attenuation, so
 * (vol-100)*32 maps 0 -> -3200 and 100 -> 0. TRUE on DS_OK. */
// FUNCTION: LEGOLAND 0x004929a0
int SetSampleVolume(Sample* s, int vol)
{
    if (!g_samples_ready)
        return 0;
    if (!s)
        return 0;
    if (!s->def)
        return 0;
    return s->buf->lpVtbl->SetVolume(s->buf, (vol - 100) << 5) == 0;
}

/* pan is -100..100 (percent); DirectSound pan is -10000..10000. */
// FUNCTION: LEGOLAND 0x004929e0
int SetSamplePan(Sample* s, int pan)
{
    if (!g_samples_ready)
        return 0;
    if (!s)
        return 0;
    if (!s->def)
        return 0;
    return s->buf->lpVtbl->SetPan(s->buf, pan * 100) == 0;
}

// FUNCTION: LEGOLAND 0x00492a20
int SetSampleFrequency(Sample* s, unsigned int freq)
{
    if (!g_samples_ready)
        return 0;
    if (!s)
        return 0;
    if (!s->def)
        return 0;
    return s->buf->lpVtbl->SetFrequency(s->buf, freq) == 0;
}

/* Current playback frequency, or 0 when the buffer refuses. (internal) */
// FUNCTION: LEGOLAND 0x00492a60
int GetSampleFrequency(Sample* s)
{
    unsigned long freq;

    if (!g_samples_ready)
        return 0;
    if (!s)
        return 0;
    if (!s->def)
        return 0;
    return s->buf->lpVtbl->GetFrequency(s->buf, &freq) == 0 ? freq : 0;
}

/* Randomly detune a sample by up to +/- range percent. */
// FUNCTION: LEGOLAND 0x00492aa0
void AdjustPSampleFreq(Sample* s, unsigned short range)
{
    int pct = 100 + rand() % (range * 2) - range;

    SetSampleFrequency(s, GetSampleFrequency(s) * pct / 100);
}

/* ---- positional sourcing ----------------------------------------------- */

// FUNCTION: LEGOLAND 0x00496a30
int SourcePlayableSampleToBloke(Sample* s, void* bloke)
{
    if (!g_samples_ready)
        return 0;
    if (!s)
        return 0;
    if (!s->def)
        return 0;
    s->src.kind = 1;
    s->src.obj = bloke;
    UpdateSampleSource(s);
    return 1;
}

// FUNCTION: LEGOLAND 0x00496a70
int SourcePlayableSampleToMapRef(Sample* s, MapRef ref)
{
    if (!g_samples_ready)
        return 0;
    if (!s)
        return 0;
    if (!s->def)
        return 0;
    s->src.kind = 2;
    s->src.pos = ref;
    UpdateSampleSource(s);
    return 1;
}

// FUNCTION: LEGOLAND 0x00496ac0
int SourcePlayableSampleToLevelXY(Sample* s, MapRef xy)
{
    if (!g_samples_ready)
        return 0;
    if (!s)
        return 0;
    if (!s->def)
        return 0;
    s->src.kind = 3;
    s->src.pos = xy;
    UpdateSampleSource(s);
    return 1;
}

/* How many live instances are sourced at `src`. kind 0 matches every
 * unsourced instance; kind 1 compares the bloke; kinds 2/3 compare x/y. */
// FUNCTION: LEGOLAND 0x00496b10
int CountSamplesFromSource(SoundSource* src)
{
    Sample* s = g_playable_list;
    int     n = 0;

    while (s) {
        if (s->src.kind == src->kind) {
            switch (src->kind) {
            case 0:
                n++;
                break;
            case 1:
                if (s->src.obj == src->obj)
                    n++;
                break;
            case 2:
            case 3:
                if (s->src.pos.x == src->pos.x && s->src.pos.y == src->pos.y)
                    n++;
                break;
            }
        }
        s = s->next;
    }
    return n;
}

/* Detach an instance from its source and start it fading at `fade`. */
// FUNCTION: LEGOLAND 0x00496c20
void UnSourceAndFadeSample(Sample* s, int fade)
{
    long vol;

    s->flags |= 8;
    s->fade = fade;
    s->src.kind = 0;
    s->buf->lpVtbl->GetVolume(s->buf, &vol);
    /* a definition reuses the +0x10 slot for its table name (Load_FXList) */
    DBPrintf(kUnSourceFadeFmt, s->def->src.obj, s, vol);
    if (s->flags & 2) {
        s->flags &= ~2;
        DBPrintf(kUnSourceClearFmt, s);
    }
}

/* Fade out every instance sourced at `src`. `match` is left uninitialised for
 * a source kind above 3 (the original reads whatever is in the slot). */
// FUNCTION: LEGOLAND 0x00496c80
void UnSourceAndFadeAllSamplesFromSource(SoundSource* src, int fade)
{
    Sample* s = g_playable_list;
    int     match;

    while (s) {
        if (s->src.kind == src->kind) {
            switch (src->kind) {
            case 0:
                match = 1;
                break;
            case 1:
                match = s->src.obj == src->obj;
                break;
            case 2:
            case 3:
                match = s->src.pos.x == src->pos.x && s->src.pos.y == src->pos.y;
                break;
            }
            if (match)
                UnSourceAndFadeSample(s, fade);
        }
        s = s->next;
    }
}

/* ---- lifetime ---------------------------------------------------------- */

/* Make a new playable instance of a definition: duplicate the definition's
 * buffer and link the instance to the ROOT definition. */
// FUNCTION: LEGOLAND 0x00492690
Sample* CreatePlayableSample(Sample* def)
{
    IDSBuffer* dup;
    Sample*    s;
    Sample*    root = def;

    if (!g_samples_ready)
        return 0;
    if (!root)
        return 0;
    while (root->def)
        root = root->def;
    if (g_dsound->lpVtbl->DuplicateSoundBuffer(g_dsound, root->buf, &dup) != 0)
        goto fail;
    s = MakePlayable();
    if (!s) {
        dup->lpVtbl->Release(dup);
        goto fail;
    }
    root->refcount++;
    s->def = root;
    s->refcount++;
    s->buf = dup;
    return s;
fail:
    return 0;
}

/* Release an instance's duplicate buffer, drop its definition's refcount and
 * free the record. (internal) Declared int-returning — that is what makes the
 * callers' discarded tail calls clean up with `pop ecx` instead of becoming a
 * `jmp`; the value is whatever the free wrapper left in eax. */
// FUNCTION: LEGOLAND 0x00492b20
int FreePlayableSample(Sample* s)
{
    if (s) {
        s->buf->lpVtbl->Release(s->buf);
        s->def->refcount--;
#ifndef LEGOLAND_PORTABLE
        return HeapFree_w(s);
#else
        /* HeapFree_w is the CRT's free (0x0049e4d0), which really returns
         * nothing; the `int` above is the lever that keeps this a call plus
         * `pop ecx` rather than a `jmp`, and the value returned is whatever the
         * wrapper left in eax.  This function also falls off its end, so the
         * portable arm drops the value instead of inventing one. */
        HeapFree_w(s);
        return 0;
#endif
    }
}

/* Unlink an instance from the live list and free it. */
// FUNCTION: LEGOLAND 0x00492b50
void KillPlayableSample(Sample* s)
{
    Sample* p = g_playable_list;

    if (p == s) {
        g_playable_list = s->next;
    } else {
        while (p) {
            if (p->next == s) {
                p->next = s->next;
                break;
            }
            p = p->next;
        }
    }
    FreePlayableSample(s);
}

/* Free every live instance — or, with a definition, only its instances. */
// FUNCTION: LEGOLAND 0x00492b90
void DeletePlayableSamples(Sample* def)
{
    Sample* s = g_playable_list;
    Sample* prev = 0;

    while (s) {
        Sample* next = s->next;

        if (def && s->def != def) {
            prev = s;
        } else {
            if (prev)
                prev->next = next;
            else
                g_playable_list = next;
            FreePlayableSample(s);
        }
        s = next;
    }
}

/* Destroy a definition: its live instances, its two heap blocks, its master
 * buffer, then the record. */
// FUNCTION: LEGOLAND 0x00492be0
void DeleteSampleDef(Sample* def)
{
    if (def) {
        DeletePlayableSamples(def);
        HeapFree_w(def->data30);
        HeapFree_w(def->data34);
        def->buf->lpVtbl->Release(def->buf);
        HeapFree_w(def);
    }
}

/* Create an instance, source it (or clear its source), start it; a new
 * instance is immediately paused while the SFX-pause latch is set. */
// FUNCTION: LEGOLAND 0x00496d20
Sample* PlayInstanceOfSample(Sample* def, int a, int b, SoundSource* src)
{
    Sample* s = CreatePlayableSample(def);

    if (!s)
        return 0;
    if (src) {
        s->src = *src;
        UpdateSampleSource(s);
    } else {
        ClearSampleSource(s);
    }
    if (!PlaySample(s, a, b)) {
        KillPlayableSample(s);
        return 0;
    }
    if (g_sfx_paused)
        PauseSingleSample(s);
    return s;
}

/* ---- volume sliders ---------------------------------------------------- */

/* Slider 0..100 -> DirectSound attenuation: 0 = -4000 (-40 dB) up to 100 = 0,
 * with the bottom of the slider snapped to DSBVOLUME_MIN (-10000). (internal) */
// FUNCTION: LEGOLAND 0x00495a50
long VolToDB(int slider)
{
    long db = slider * 4000 / 100 - 4000;

    if (db == -4000)
        db = -10000;
    return db;
}

/* Push the three slider values to the music buffer, the SFX master and the
 * speech channel. Always returns 0. */
// FUNCTION: LEGOLAND 0x00495a90
int UpdateSoundVols(void)
{
    if (g_samples_ready) {
        if (g_music_sys && g_music_ready) {
            long db = VolToDB(g_vol_music);
            g_music_buf->lpVtbl->SetVolume(g_music_buf, db);
        }
        g_sfx_master_db = VolToDB(g_vol_sfx);
        RefreshSampleVolumes();
        SetSpeechVolume(VolToDB(g_vol_speech));
    }
    return 0;
}

/* ---- master directory / volume lists ----------------------------------- */

/* A directory bucket of the master directory: name pointer at +0x08. */
typedef struct MasterDir {
    struct MasterDir* next;  /* +0x00 */
    int               pad4;  /* +0x04 */
    char*             name;  /* +0x08 */
} MasterDir;

/* A mounted volume: its name is an inline array at +0x08. */
typedef struct MasterVol {
    struct MasterVol* next;    /* +0x00 */
    int               pad4;    /* +0x04 */
    char              name[1]; /* +0x08 */
} MasterVol;

extern MasterDir* g_master_dirs;  /* 0x00798624 */
extern MasterVol* g_master_vols;  /* 0x00798628 */

// FUNCTION: LEGOLAND 0x004894d0
MasterDir* GetMasterDirPtr(const char* name)
{
    MasterDir* d = g_master_dirs;

    while (d) {
        if (_stricmp(name, d->name) == 0)
            return d;
        d = d->next;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x00489510
MasterVol* GetMasterVolPtr(const char* name)
{
    MasterVol* v = g_master_vols;

    while (v) {
        if (_stricmp(name, v->name) == 0)
            return v;
        v = v->next;
    }
    return 0;
}
