/* LEGOLAND — DirectMusic wrappers (style/band/chordmap/segment loading,
 * segment composition and playback) and the raw MIDI-file loader/player.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only the
 * struct field offsets, vtable slots and callee arg counts are load-bearing;
 * names are ours.
 */

/* ---- DirectMusic interfaces (only the slots we call) ------------------ */

typedef struct GUID_ {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} GUID_;

typedef struct IDMObj IDMObj;                 /* any COM object: Release @+0x08 */
typedef struct IDMObjVtbl {
    long          (__stdcall *QueryInterface)(IDMObj*, const void*, void**); /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDMObj*);                              /* +0x04 */
    unsigned long (__stdcall *Release)(IDMObj*);                             /* +0x08 */
} IDMObjVtbl;
struct IDMObj { IDMObjVtbl* lpVtbl; };

typedef struct IDMSegment IDMSegment;
typedef struct IDMSegmentVtbl {
    long          (__stdcall *QueryInterface)(IDMSegment*, const void*, void**); /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDMSegment*);                              /* +0x04 */
    unsigned long (__stdcall *Release)(IDMSegment*);                             /* +0x08 */
    long          (__stdcall *GetLength)(IDMSegment*, long*);                    /* +0x0c */
    long          (__stdcall *SetLength)(IDMSegment*, long);                     /* +0x10 */
    long          (__stdcall *GetRepeats)(IDMSegment*, unsigned long*);          /* +0x14 */
    long          (__stdcall *SetRepeats)(IDMSegment*, unsigned long);           /* +0x18 */
} IDMSegmentVtbl;
struct IDMSegment { IDMSegmentVtbl* lpVtbl; };

typedef struct IDMPerformance IDMPerformance;
typedef struct IDMPerformanceVtbl {
    long          (__stdcall *QueryInterface)(IDMPerformance*, const void*, void**); /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDMPerformance*);                              /* +0x04 */
    unsigned long (__stdcall *Release)(IDMPerformance*);                             /* +0x08 */
    long          (__stdcall *Init)(IDMPerformance*, void**, void*, void*);          /* +0x0c */
    long          (__stdcall *PlaySegment)(IDMPerformance*, IDMSegment*,
                                           unsigned long flags, __int64 start,
                                           void** state);                            /* +0x10 */
} IDMPerformanceVtbl;
struct IDMPerformance { IDMPerformanceVtbl* lpVtbl; };

typedef struct IDMComposer IDMComposer;
typedef struct IDMComposerVtbl {
    long          (__stdcall *QueryInterface)(IDMComposer*, const void*, void**);  /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDMComposer*);                               /* +0x04 */
    unsigned long (__stdcall *Release)(IDMComposer*);                              /* +0x08 */
    long          (__stdcall *ComposeSegmentFromTemplate)(IDMComposer*, IDMObj* style,
                                    IDMSegment* tmpl, unsigned short activity,
                                    IDMObj* chordmap, IDMSegment** seg);           /* +0x0c */
    long          (__stdcall *ComposeSegmentFromShape)(IDMComposer*, IDMObj* style,
                                    unsigned short measures, unsigned short shape,
                                    unsigned short activity, int intro, int end,
                                    IDMObj* chordmap, IDMSegment** seg);           /* +0x10 */
    long          (__stdcall *ComposeTransition)(IDMComposer*, IDMSegment*, IDMSegment*,
                                    long, unsigned short, unsigned long, IDMObj*,
                                    IDMSegment**);                                 /* +0x14 */
    long          (__stdcall *AutoTransition)(IDMComposer*, IDMPerformance*,
                                    IDMSegment* to, unsigned short command,
                                    unsigned long flags, IDMObj* chordmap,
                                    IDMSegment** transition, void** toState,
                                    void** transState);                            /* +0x18 */
} IDMComposerVtbl;
struct IDMComposer { IDMComposerVtbl* lpVtbl; };

typedef struct IDMStyle IDMStyle;
typedef struct IDMStyleVtbl {
    long          (__stdcall *QueryInterface)(IDMStyle*, const void*, void**);     /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDMStyle*);                                  /* +0x04 */
    unsigned long (__stdcall *Release)(IDMStyle*);                                 /* +0x08 */
    long          (__stdcall *GetBand)(IDMStyle*, unsigned short* name, void** band); /* +0x0c */
} IDMStyleVtbl;
struct IDMStyle { IDMStyleVtbl* lpVtbl; };

typedef struct IDMLoader IDMLoader;
typedef struct IDMLoaderVtbl {
    long          (__stdcall *QueryInterface)(IDMLoader*, const void*, void**);    /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDMLoader*);                                 /* +0x04 */
    unsigned long (__stdcall *Release)(IDMLoader*);                                /* +0x08 */
    long          (__stdcall *GetObject)(IDMLoader*, void* desc, const void* iid,
                                         void** out);                              /* +0x0c */
} IDMLoaderVtbl;
struct IDMLoader { IDMLoaderVtbl* lpVtbl; };

/* DMUS_OBJECTDESC (0x350 bytes) */
typedef struct DMObjectDesc {
    unsigned long  dwSize;                /* +0x000 */
    unsigned long  dwValidData;           /* +0x004 */
    GUID_          guidObject;            /* +0x008 */
    GUID_          guidClass;             /* +0x018 */
    unsigned long  ftDate[2];             /* +0x028 */
    unsigned long  vVersion[2];           /* +0x030 */
    unsigned short wszName[64];           /* +0x038 */
    unsigned short wszCategory[64];       /* +0x0b8 */
    unsigned short wszFileName[260];      /* +0x138 */
    __int64        llMemLength;           /* +0x340 */
    unsigned char* pbMemData;             /* +0x348 */
    void*          pStream;               /* +0x34c */
} DMObjectDesc;

#define DMUS_OBJ_CLASS    0x02
#define DMUS_OBJ_FILENAME 0x10

/* DMUS_SEGF_* / DMUS_COMPOSEF_* bits used here */
#define DMUS_SEGF_SECONDARY 0x80
#define DMUS_SEGF_MEASURE   0x2000
#define BLEND_COMPOSEF      0x2022   /* DMUS_COMPOSEF_MODULATE|LONG|MEASURE */

/* ---- globals ----------------------------------------------------------- */
extern void*          g_music_sys;          /* 0x004bf774  music engine instance */
extern int            g_music_ready;        /* 0x0079a694  music system up */
extern IDMLoader*     g_dm_loader;          /* 0x007cacd8  IDirectMusicLoader */
extern IDMPerformance* g_dm_performance;    /* 0x007cacdc  IDirectMusicPerformance */
extern IDMComposer*   g_dm_composer;        /* 0x007cad44  IDirectMusicComposer */

extern const GUID_ CLSID_DirectMusicStyle;     /* 0x004ab980 */
extern const GUID_ CLSID_DirectMusicBand;      /* 0x004ab8e0 */
extern const GUID_ CLSID_DirectMusicChordMap;  /* 0x004ab930 */
extern const GUID_ CLSID_DirectMusicSegment;   /* 0x004ab9f0 */
extern const GUID_ IID_IDirectMusicStyle;      /* 0x004ab610 */
extern const GUID_ IID_IDirectMusicBand;       /* 0x004ab5e0 */
extern const GUID_ IID_IDirectMusicChordMap;   /* 0x004ab600 */
extern const GUID_ IID_IDirectMusicSegment;    /* 0x004ab670 */

__declspec(dllimport) int __stdcall MultiByteToWideChar(unsigned int cp, unsigned long flags,
                                                        const char* src, int srclen,
                                                        unsigned short* dst, int dstlen);
unsigned short* __cdecl wcscpy(unsigned short* dst, const unsigned short* src);
void* __cdecl malloc(unsigned int);

/* ---- object loading ---------------------------------------------------- */
/* All four loaders are one template over IDirectMusicLoader::GetObject with
 * a DMUS_OBJECTDESC carrying CLSID + filename; TRUE on SUCCEEDED(). */

// FUNCTION: LEGOLAND 0x00495bc0
int LoadMusicStyle(const char* name, void** out)
{
    DMObjectDesc   desc;
    unsigned short wname[0x200];
    long           hr;

    if (!g_music_sys)
        return 0;
    if (!g_music_ready)
        return 0;
    *out = 0;
    MultiByteToWideChar(0, 0, name, -1, wname, 0x200);
    desc.dwSize = sizeof(DMObjectDesc);
    desc.guidClass = CLSID_DirectMusicStyle;
    wcscpy(desc.wszFileName, wname);
    desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;
    hr = g_dm_loader->lpVtbl->GetObject(g_dm_loader, &desc, &IID_IDirectMusicStyle, out);
    return hr >= 0;
}

// FUNCTION: LEGOLAND 0x00495c90
int LoadMusicBand(const char* name, void** out)
{
    DMObjectDesc   desc;
    unsigned short wname[0x200];
    long           hr;

    if (!g_music_sys)
        return 0;
    if (!g_music_ready)
        return 0;
    *out = 0;
    MultiByteToWideChar(0, 0, name, -1, wname, 0x200);
    desc.dwSize = sizeof(DMObjectDesc);
    desc.guidClass = CLSID_DirectMusicBand;
    wcscpy(desc.wszFileName, wname);
    desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;
    hr = g_dm_loader->lpVtbl->GetObject(g_dm_loader, &desc, &IID_IDirectMusicBand, out);
    return hr >= 0;
}

// FUNCTION: LEGOLAND 0x00495d60
int LoadMusicChordMap(const char* name, void** out)
{
    DMObjectDesc   desc;
    unsigned short wname[0x200];
    long           hr;

    if (!g_music_sys)
        return 0;
    if (!g_music_ready)
        return 0;
    *out = 0;
    MultiByteToWideChar(0, 0, name, -1, wname, 0x200);
    desc.dwSize = sizeof(DMObjectDesc);
    desc.guidClass = CLSID_DirectMusicChordMap;
    wcscpy(desc.wszFileName, wname);
    desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;
    hr = g_dm_loader->lpVtbl->GetObject(g_dm_loader, &desc, &IID_IDirectMusicChordMap, out);
    return hr >= 0;
}

// FUNCTION: LEGOLAND 0x00495e30
int LoadMusicSegment(const char* name, void** out)
{
    DMObjectDesc   desc;
    unsigned short wname[0x200];
    long           hr;

    if (!g_music_sys)
        return 0;
    if (!g_music_ready)
        return 0;
    *out = 0;
    MultiByteToWideChar(0, 0, name, -1, wname, 0x200);
    desc.dwSize = sizeof(DMObjectDesc);
    desc.guidClass = CLSID_DirectMusicSegment;
    wcscpy(desc.wszFileName, wname);
    desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;
    hr = g_dm_loader->lpVtbl->GetObject(g_dm_loader, &desc, &IID_IDirectMusicSegment, out);
    return hr >= 0;
}

/* Look a named band up inside a style (IDirectMusicStyle::GetBand). */
// FUNCTION: LEGOLAND 0x00496090
int GetMusicBand(const char* name, IDMStyle* style, void** band)
{
    unsigned short wname[0x200];
    long           hr;

    if (!g_music_sys)
        return 0;
    if (!g_music_ready)
        return 0;
    *band = 0;
    MultiByteToWideChar(0, 0, name, -1, wname, 0x200);
    hr = style->lpVtbl->GetBand(style, wname, band);
    return hr >= 0;
}

/* ---- playback ---------------------------------------------------------- */

/* Queue a (band) segment as a secondary segment on the next measure. */
// FUNCTION: LEGOLAND 0x00496250
int SetBand(IDMSegment* seg)
{
    if (!g_music_sys)
        return 0;
    if (!g_music_ready)
        return 0;
    g_dm_performance->lpVtbl->PlaySegment(g_dm_performance, seg,
                                          DMUS_SEGF_MEASURE | DMUS_SEGF_SECONDARY, 0, 0);
    return 1;
}

/* Identical body to SetBand: a motif is just another secondary segment. */
// FUNCTION: LEGOLAND 0x00496290
int PlayMotif(IDMSegment* seg)
{
    if (!g_music_sys)
        return 0;
    if (!g_music_ready)
        return 0;
    g_dm_performance->lpVtbl->PlaySegment(g_dm_performance, seg,
                                          DMUS_SEGF_MEASURE | DMUS_SEGF_SECONDARY, 0, 0);
    return 1;
}

/* Compose a primary segment from a style + template + chordmap, loop it 999
 * times, start it on the next measure and drop our reference. The composed
 * segment pointer lives in the dead `chordmap` argument slot. */
// FUNCTION: LEGOLAND 0x00496150
int PlaySegmentFromTemplate(IDMObj* style, IDMSegment* tmpl, IDMObj* chordmap)
{
    IDMSegment* seg;

    if (!g_music_sys)
        return 0;
    if (!g_music_ready)
        return 0;
    g_dm_composer->lpVtbl->ComposeSegmentFromTemplate(g_dm_composer, style, tmpl, 0,
                                                      chordmap, &seg);
    seg->lpVtbl->SetRepeats(seg, 999);
    g_dm_performance->lpVtbl->PlaySegment(g_dm_performance, seg, DMUS_SEGF_MEASURE, 0, 0);
    seg->lpVtbl->Release(seg);
    return 1;
}

/* Compose a 10-measure primary segment from a style + chordmap (shape 2,
 * activity 3, no intro/end), loop it 999 times and start it on the measure. */
// FUNCTION: LEGOLAND 0x004961d0
int PlaySegment(IDMObj* style, IDMObj* chordmap)
{
    IDMSegment* seg;

    if (!g_music_sys)
        return 0;
    if (!g_music_ready)
        return 0;
    g_dm_composer->lpVtbl->ComposeSegmentFromShape(g_dm_composer, style, 10, 2, 3, 0, 0,
                                                   chordmap, &seg);
    seg->lpVtbl->SetRepeats(seg, 999);
    g_dm_performance->lpVtbl->PlaySegment(g_dm_performance, seg, DMUS_SEGF_MEASURE, 0, 0);
    seg->lpVtbl->Release(seg);
    return 1;
}

/* Compose a new 10-measure segment and let the composer auto-transition the
 * running performance into it. The middle parameter is never read. */
// FUNCTION: LEGOLAND 0x004962d0
int BlendMusic(IDMObj* style, void* unused, IDMObj* chordmap)
{
    IDMSegment* seg;
    IDMSegment* trans;

    if (!g_music_sys)
        return 0;
    if (!g_music_ready)
        return 0;
    g_dm_composer->lpVtbl->ComposeSegmentFromShape(g_dm_composer, style, 10, 2, 3, 0, 0,
                                                   chordmap, &seg);
    seg->lpVtbl->SetRepeats(seg, 999);
    g_dm_composer->lpVtbl->AutoTransition(g_dm_composer, g_dm_performance, seg, 0,
                                          BLEND_COMPOSEF, chordmap, &trans, 0, 0);
    seg->lpVtbl->Release(seg);
    trans->lpVtbl->Release(trans);
    return 1;
}

/* ---- raw MIDI file ----------------------------------------------------- */

typedef struct MidiFile MidiFile;

/* One MTrk chunk (0x1c bytes, built by the track reader at 0x004801a0). */
typedef struct MidiTrack {
    MidiFile*      owner;     /* +0x00 */
    int            length;    /* +0x04 */
    unsigned char* data;      /* +0x08 */
    int            pos;       /* +0x0c */
    int            pad10;     /* +0x10 */
    int            time;      /* +0x14 */
    short          active;    /* +0x18 */
    short          pending;   /* +0x1a */
} MidiTrack;

/* Loaded MIDI file (0x18 bytes). */
struct MidiFile {
    int            tick_scale; /* +0x00  division * 20000 */
    int            tempo;      /* +0x04 */
    int            time;       /* +0x08 */
    short          ntracks;    /* +0x0c */
    short          pad0e;      /* +0x0e */
    MidiTrack**    tracks;     /* +0x10 */
    short          playing;    /* +0x14 */
    short          pad16;      /* +0x16 */
};

extern void*      RES_OpenFile(const char* path);            /* 0x00489b60 */
extern int        RES_ReadFile(void* f, void* buf, int len); /* 0x00489cf0 */
extern void       RES_CloseFile(void* f);                    /* 0x00489de0 */
extern void       ReadBE32(void* f, void* out);              /* 0x00480150 (internal) */
extern void       ReadBE16(void* f, void* out);              /* 0x00480170 (internal) */
extern MidiTrack* ReadMidiTrack(void* f);                    /* 0x004801a0 (internal) */

extern MidiFile* g_midi_current;   /* 0x007fd634 */

/* Parse the MThd header and every MTrk chunk of a resource MIDI file.
 * `division` is a 16-bit big-endian read through the word helper into an int
 * that is zeroed first (the one `mov [esp+..],ebp` in the prologue), so the
 * full dword scaled into tick_scale is clean. Frame: length -4, chunk -8,
 * division -0xc, format -0xe — which local carries the `= 0` decides the
 * chunk/division order; declaration order and names do not. */
// FUNCTION: LEGOLAND 0x00480200
MidiFile* LoadMIDIFile(const char* name)
{
    MidiFile* m;
    void*     f;
    int       chunk;
    int       length;
    short     format;
    int       division = 0;
    int       i;

    m = (MidiFile*)malloc(sizeof(MidiFile));
    f = RES_OpenFile(name);
    RES_ReadFile(f, &chunk, 4);
    ReadBE32(f, &length);
    ReadBE16(f, &format);
    ReadBE16(f, &m->ntracks);
    ReadBE16(f, &division);
    m->tick_scale = division * 20000;
    m->tracks = (MidiTrack**)malloc(m->ntracks * 4);
    for (i = 0; i < m->ntracks; i++) {
        m->tracks[i] = ReadMidiTrack(f);
        m->tracks[i]->owner = m;
    }
    m->tempo = 0x100;
    RES_CloseFile(f);
    return m;
}

/* Rewind a loaded file and make it the current one: clear its clock, mark it
 * playing and reset every track's position/clock/flags. */
// FUNCTION: LEGOLAND 0x004805d0
void PlayMIDI(MidiFile* m)
{
    int i;

    g_midi_current = m;
    m->time = 0;
    m->playing = 1;
    for (i = 0; i < m->ntracks; i++) {
        m->tracks[i]->time = 0;
        m->tracks[i]->active = 1;
        m->tracks[i]->pos = 0;
        m->tracks[i]->pending = 1;
    }
}
