/* =========================================================================
 * LEGOLAND -- musicthread.c
 *
 * 0x00492db0  MusicThread -- the whole interactive-music ("IMT") worker
 * thread, 3,161 instructions in 11,335 bytes.  It is by a factor of three the
 * largest function in the executable; sysstubs.c's InitMusicSystem starts it
 * and sysmisc3.c's KillMusicSystem TerminateThread()s it.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Only
 * struct field OFFSETS, global addresses, vtable slots and callee argument
 * counts are load-bearing; the names are ours.
 *
 * ---------------------------------------------------------------------------
 * EXTENT -- why 3,161 instructions is REAL and not a walker artefact
 * ---------------------------------------------------------------------------
 * The body runs 0x00492db0..0x004959f6 (11,335 bytes) and ends on
 * `jmp 0x0049567e`, the back edge of the message loop -- an unconditional
 * jump nothing branches past.  There is NO inter-function padding anywhere
 * inside that span, no export lies strictly inside it (the neighbours are
 * `KillSoundSampleSystem` at 0x00492c20 and `UpdateSoundVols` at 0x00495a90,
 * which this body calls), and every branch target is interior.  What LOOKS
 * like more code immediately after -- a `ret` at 0x00495a3e with nop runs
 * around it -- is the switch's JUMP TABLE misdisassembled: 0x004959f7 is one
 * alignment `nop`, 0x004959f8..0x00495a0b is the five-entry table that
 * `jmp dword ptr [eax*4 + 0x004959f8]` (0x00495838) indexes, 0x00495a0c is
 * four bytes of padding, and 0x00495a10 is the next (unexported) function --
 * the starter that CreateThreads THIS body and stores the handle at
 * 0x0079a698, the very handle sysmisc3.c's KillMusicSystem TerminateThreads.
 *
 * It is NOT hand-written assembly: no `xchg` against memory, no `pushad`, no
 * EBP frame -- the prologue is the ordinary /O2 `sub esp,0x3bc` with EBP
 * allocated as a general register holding the constant 1, and the epilogue is
 * the matching pop/pop/pop/pop/add/ret 4.  It is compiled C.
 *
 * ---------------------------------------------------------------------------
 * SHAPE -- 81% of it is two source-level groups written out THIRTY times
 * ---------------------------------------------------------------------------
 *   0x00492db0  273 insns  COM bring-up and a five-level failure ladder
 *   0x0049310d 1170 insns  LOAD_SEGMENT  x 30  (39 instructions each, exact)
 *   0x00494011 1381 insns  BUILD_SEGMENT x 30  (47, then 46 each, exact)
 *   0x0049553d   96 insns  download every segment, arm the notifications
 *   0x0049567e  241 insns  the message loop: two events, a four-way command
 *                          switch and the notification pump with a five-case
 *                          jump-table switch
 *
 * The two repeated groups are byte-for-byte periodic (0x80 bytes for the
 * first), differing only in a path string, a wide name and two array indices,
 * so the original source wrote them as macros -- reproduced as macros here.
 * Successive expansions of the second group alternate between two register
 * assignments (eax/ecx swap): that is VC6's scratch rotation, not two
 * different sources.
 *
 * ---------------------------------------------------------------------------
 * WHAT THE THREAD DOES, AND THE MUSIC SET IT RECOVERS
 * ---------------------------------------------------------------------------
 * The park has FIVE musical worlds -- l(egoland/theme), e(gypt), i(nca),
 * m(edieval/castle), w(est) -- each with two segments, plus a transition
 * segment for every ORDERED pair of distinct worlds.  They live in THREE
 * parallel banks, not one table -- and that is a codegen fact, not a
 * cosmetic one: the download loop reads `[edi + 0x799234]` and
 * `[edi + 0x799248]`, ONE index register with TWO link-time bases, which is
 * only what two DIFFERENT arrays indexed by the same world number produce.
 * Spelled as a single 35-slot array VC6 folds the pair into one walking
 * pointer and 300 instructions change.  The banks are
 *
 *      g_seg_first [5]  0x00799230   segtheme1, segegypt1, seginca1, ...
 *      g_seg_second[5]  0x00799244   segtheme2, segegypt2, ...
 *      g_seg_trans [25] 0x00799258   [a*5+b] = the transition a -> b
 *                                    ("letran2" = l -> e, at [0*5+1])
 *
 * with matching g_*_data / g_*_size banks.  The five diagonal transition
 * slots (a == b) are never filled and the loader skips them.  Thirty .sgt
 * files are read whole into memory, handed to IDirectMusicLoader::GetObject
 * as DMUS_OBJ_MEMORY blobs, and the blobs freed one by one as each object is
 * built.
 *
 * TWO ORIGINAL BUGS, both reproduced:
 *  1. g_seg_trans[7] (e -> i) loads "imusic\ietran2.sgt" -- the i -> e file
 *     -- while asking for the object named L"eitran2".  The file list has
 *     "ietran2" twice and no "eitran2" at all; the NAME list has both exactly
 *     once.  So the egypt->inca transition asks a segment file for a name that
 *     is not in it, its GetObject fails (logged, then ignored) and the slot
 *     stays null -- the only pair of worlds with no transition music.
 *  2. The short-read log line prints the comparison RESULT, not the byte
 *     count: "(wanted %d, got %d)" is passed (size, size != read) so the
 *     "got" column is always 1.
 *
 * A third, milder one: the DSBUFFERDESC is declared at the DX8 size (0x24)
 * but only its first five DWORDs are written, so guid3DAlgorithm goes to
 * DirectSound uninitialised.
 *
 * Then the thread arms two notification types on the performance, downloads
 * every segment it managed to build EXCEPT world 0's (the outer loop runs
 * a = 1..4 only, so g_seg_first[0], g_seg_second[0] and the four l->x
 * transitions are never downloaded), and drops into a two-event message
 * loop:
 *
 *   g_imt_event (posted by sysmisc2.c's SetTheme and friends) carries a
 *   command in g_imt_cmd:  1 = stop, 3 = play world g_imt_cmd_arg's first
 *   segment outright, 4 = go to world g_imt_cmd_arg -- same world means play
 *   its SECOND segment, a different world means raise the groove level and
 *   arm state 5 so the next bar boundary starts the transition.
 *
 *   g_imt_notify_event carries DirectMusic's own notifications.  Segment
 *   notifications are logged and SEGABORT in state 6 advances to state 7;
 *   measure-and-beat notifications drive the transition: at beat 0 in state 5
 *   the a->b transition segment starts (state 6), and at bar 3 beat 1 in
 *   state 7 the destination world's second segment starts (state 4).
 *
 * The starter that CreateThreads this body sits at 0x00495a10, immediately
 * past the switch's jump table, and is not exported.
 * ========================================================================= */

/* ---- COM plumbing ------------------------------------------------------- */

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

/* IDirectMusicSegment.  NOTE the offsets: SetParam is at +0x4c and not the
 * +0x40 of the shipped DirectX 7 dmusici.h, so the game was built against a
 * header whose IDirectMusicSegment carries three more methods before
 * GetLength.  The argument counts (five for +0x4c, one for +0x18) are what
 * the call sites prove, and they are what the vtable below has to spell. */
typedef struct IDMSegment IDMSegment;
typedef struct IDMSegmentVtbl {
    long          (__stdcall *QueryInterface)(IDMSegment*, const void*, void**); /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDMSegment*);                              /* +0x04 */
    unsigned long (__stdcall *Release)(IDMSegment*);                             /* +0x08 */
    unsigned char pad0c[0x18 - 0x0c];
    long          (__stdcall *SetRepeats)(IDMSegment*, unsigned long);           /* +0x18 */
    unsigned char pad1c[0x4c - 0x1c];
    long          (__stdcall *SetParam)(IDMSegment*, const void* type,
                                        unsigned long groupbits,
                                        unsigned long index, long time,
                                        void* param);                            /* +0x4c */
} IDMSegmentVtbl;
struct IDMSegment { IDMSegmentVtbl* lpVtbl; };

typedef struct IDMPort IDMPort;
typedef struct IDMPortVtbl {
    long          (__stdcall *QueryInterface)(IDMPort*, const void*, void**);    /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDMPort*);                                 /* +0x04 */
    unsigned long (__stdcall *Release)(IDMPort*);                                /* +0x08 */
    unsigned char pad0c[0x3c - 0x0c];
    long          (__stdcall *Activate)(IDMPort*, int active);                   /* +0x3c */
    unsigned char pad40[0x48 - 0x40];
    long          (__stdcall *SetDirectSound)(IDMPort*, void* ds, void* dsb);    /* +0x48 */
    long          (__stdcall *GetFormat)(IDMPort*, void* wfx, unsigned long* wfxsize,
                                         unsigned long* bufsize);                /* +0x4c */
} IDMPortVtbl;
struct IDMPort { IDMPortVtbl* lpVtbl; };

typedef struct IDMusic IDMusic;
typedef struct IDMusicVtbl {
    long          (__stdcall *QueryInterface)(IDMusic*, const void*, void**);    /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDMusic*);                                 /* +0x04 */
    unsigned long (__stdcall *Release)(IDMusic*);                                /* +0x08 */
    unsigned char pad0c[0x14 - 0x0c];
    long          (__stdcall *CreatePort)(IDMusic*, const void* clsid, void* params,
                                          IDMPort** port, void* outer);          /* +0x14 */
} IDMusicVtbl;
struct IDMusic { IDMusicVtbl* lpVtbl; };

typedef struct IDMPerformance IDMPerformance;
typedef struct IDMPerformanceVtbl {
    long          (__stdcall *QueryInterface)(IDMPerformance*, const void*, void**); /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDMPerformance*);                              /* +0x04 */
    unsigned long (__stdcall *Release)(IDMPerformance*);                             /* +0x08 */
    long          (__stdcall *Init)(IDMPerformance*, IDMusic**, void* ds, void* hwnd);/* +0x0c */
    long          (__stdcall *PlaySegment)(IDMPerformance*, IDMSegment*,
                                           unsigned long flags, __int64 start,
                                           void** state);                            /* +0x10 */
    long          (__stdcall *Stop)(IDMPerformance*, void* seg, void* state,
                                    long time, unsigned long flags);                 /* +0x14 */
    unsigned char pad18[0x44 - 0x18];
    long          (__stdcall *FreePMsg)(IDMPerformance*, void* pmsg);                /* +0x44 */
    unsigned char pad48[0x50 - 0x48];
    long          (__stdcall *SetNotificationHandle)(IDMPerformance*, void* h,
                                                     __int64 minimum);                /* +0x50 */
    long          (__stdcall *GetNotificationPMsg)(IDMPerformance*, void** pmsg);     /* +0x54 */
    long          (__stdcall *AddNotificationType)(IDMPerformance*, const void* g);   /* +0x58 */
    unsigned char pad5c[0x60 - 0x5c];
    long          (__stdcall *AddPort)(IDMPerformance*, IDMPort*);                   /* +0x60 */
    unsigned char pad64[0x68 - 0x64];
    long          (__stdcall *AssignPChannelBlock)(IDMPerformance*, unsigned long blk,
                                                   IDMPort*, unsigned long group);    /* +0x68 */
    unsigned char pad6c[0x88 - 0x6c];
    long          (__stdcall *SetGlobalParam)(IDMPerformance*, const void* g,
                                              void* param, unsigned long size);       /* +0x88 */
    unsigned char pad8c[0x98 - 0x8c];
    long          (__stdcall *CloseDown)(IDMPerformance*);                           /* +0x98 */
} IDMPerformanceVtbl;
struct IDMPerformance { IDMPerformanceVtbl* lpVtbl; };

typedef struct IDMLoader IDMLoader;
typedef struct IDMLoaderVtbl {
    long          (__stdcall *QueryInterface)(IDMLoader*, const void*, void**);   /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDMLoader*);                                /* +0x04 */
    unsigned long (__stdcall *Release)(IDMLoader*);                               /* +0x08 */
    long          (__stdcall *GetObject)(IDMLoader*, void* desc, const void* iid,
                                         void** out);                             /* +0x0c */
    long          (__stdcall *SetObject)(IDMLoader*, void* desc);                 /* +0x10 */
    long          (__stdcall *SetSearchDirectory)(IDMLoader*, const void* cls,
                                                  const unsigned short* path,
                                                  int clear);                     /* +0x14 */
    long          (__stdcall *ScanDirectory)(IDMLoader*, const void* cls,
                                             const unsigned short* ext,
                                             const unsigned short* cache);        /* +0x18 */
    long          (__stdcall *CacheObject)(IDMLoader*, IDMObj*);                  /* +0x1c */
    long          (__stdcall *ReleaseObject)(IDMLoader*, IDMObj*);                /* +0x20 */
    long          (__stdcall *ClearCache)(IDMLoader*, const void* cls);           /* +0x24 */
    long          (__stdcall *EnableCache)(IDMLoader*, const void* cls, int on);  /* +0x28 */
    long          (__stdcall *EnumObject)(IDMLoader*, const void* cls,
                                          unsigned long index, void* desc);       /* +0x2c */
} IDMLoaderVtbl;
struct IDMLoader { IDMLoaderVtbl* lpVtbl; };

typedef struct IDSound IDSound;
typedef struct IDSoundVtbl {
    long          (__stdcall *QueryInterface)(IDSound*, const void*, void**);     /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDSound*);                                  /* +0x04 */
    unsigned long (__stdcall *Release)(IDSound*);                                 /* +0x08 */
    long          (__stdcall *CreateSoundBuffer)(IDSound*, void* desc, void** buf,
                                                 void* outer);                    /* +0x0c */
} IDSoundVtbl;
struct IDSound { IDSoundVtbl* lpVtbl; };

/* ---- the three stack aggregates ---------------------------------------- */

/* DMUS_OBJECTDESC (0x350 bytes, same as music.c's). */
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

/* DMUS_PORTPARAMS (0x20 bytes). */
typedef struct DMPortParams {
    unsigned long dwSize;                 /* +0x00 */
    unsigned long dwValidParams;          /* +0x04 */
    unsigned long dwVoices;               /* +0x08 */
    unsigned long dwChannelGroups;        /* +0x0c */
    unsigned long dwAudioChannels;        /* +0x10 */
    unsigned long dwSampleRate;           /* +0x14 */
    unsigned long dwEffectFlags;          /* +0x18 */
    int           fShare;                 /* +0x1c */
} DMPortParams;

/* DSBUFFERDESC at the 0x24-byte (guid3DAlgorithm) size. */
typedef struct DSBufferDesc {
    unsigned long dwSize;                 /* +0x00 */
    unsigned long dwFlags;                /* +0x04 */
    unsigned long dwBufferBytes;          /* +0x08 */
    unsigned long dwReserved;             /* +0x0c */
    void*         lpwfxFormat;            /* +0x10 */
    GUID_         guid3DAlgorithm;        /* +0x14 -- never written */
} DSBufferDesc;

/* DMUS_NOTIFICATION_PMSG: the DMUS_PMSG_PART header is 0x38 bytes. */
typedef struct DMNotifyMsg {
    unsigned char pad00[0x38];
    GUID_         guidNotificationType;   /* +0x38 */
    unsigned long dwNotificationOption;   /* +0x48 */
    unsigned long dwField1;               /* +0x4c */
    unsigned long dwField2;               /* +0x50 */
} DMNotifyMsg;

/* ---- globals ----------------------------------------------------------- */

extern void*           g_music_sys;          /* 0x004bf774  music engine instance */
extern volatile int    g_music_disabled;     /* 0x007988bc */
extern int             g_music_ready;        /* 0x0079a694  DMusicInitialised */
extern IDSound*        g_dsound;             /* 0x007cad40  IDirectSound */
extern void*           g_music_buf;          /* 0x007cad4c  music stream buffer */
extern IDMLoader*      g_dm_loader;          /* 0x007cacd8  IDirectMusicLoader */
extern IDMPerformance* g_dm_performance;     /* 0x007cacdc  IDirectMusicPerformance */
extern IDMObj*         g_dm_composer;        /* 0x007cad44  IDirectMusicComposer */

/* The IMT command mailbox (sysmisc2.c names all four). */
extern int             g_imt_state;          /* 0x004bf778 */
extern void*           g_imt_notify_event;   /* 0x0079a69c  DirectMusic notifications */
extern void*           g_imt_event;          /* 0x0079a6a0  command posted */
extern int             g_imt_cmd;            /* 0x0079a6a4 */
extern int             g_imt_cmd_arg;        /* 0x0079a6a8 */
extern int             g_imt_theme;          /* 0x0079a6ac  the world now playing */

extern IDMObj*         g_styles[];           /* 0x007988d0  every .sty in imusic\ */

/* THREE parallel banks of five/five/twenty-five, not one array of 35: the
 * download loop's `[edi + 0x799234]` / `[edi + 0x799248]` pair -- one index
 * register with two link-time bases -- only comes out of two DIFFERENT arrays
 * indexed by the same world number.  The transition bank is indexed
 * a * 5 + b and its five diagonal slots (a == b) are never filled. */
extern IDMSegment*     g_seg_first[];        /* 0x00799230  world W, segment 1 */
extern IDMSegment*     g_seg_second[];       /* 0x00799244  world W, segment 2 */
extern IDMSegment*     g_seg_trans[];        /* 0x00799258  transition a -> b  */
extern unsigned char*  g_first_data[];       /* 0x00799c1c  each .sgt read whole */
extern unsigned char*  g_second_data[];      /* 0x00799c30 */
extern unsigned char*  g_trans_data[];       /* 0x00799c44 */
extern int             g_first_size[];       /* 0x0079a608 */
extern int             g_second_size[];      /* 0x0079a61c */
extern int             g_trans_size[];       /* 0x0079a630 */
extern int             g_getobj_calls;       /* 0x0079a6b0  GetObject call counter */

extern const GUID_ GUID_NULL;                     /* 0x004acfd0 */
extern const GUID_ CLSID_DirectMusicComposer;     /* 0x004ab920 */
extern const GUID_ IID_IDirectMusicComposer;      /* 0x004ab5f0 */
extern const GUID_ CLSID_DirectMusicPerformance;  /* 0x004aba00 */
extern const GUID_ IID_IDirectMusicPerformance;   /* 0x004ab640 */
extern const GUID_ CLSID_DirectMusicLoader;       /* 0x004ab900 */
extern const GUID_ IID_IDirectMusicLoader;        /* 0x004ab6a0 */
extern const GUID_ CLSID_DirectMusicSegment;      /* 0x004ab9f0 */
extern const GUID_ IID_IDirectMusicSegment;       /* 0x004ab670 */
extern const GUID_ CLSID_DirectMusicStyle;        /* 0x004ab980 */
extern const GUID_ IID_IDirectMusicStyle;         /* 0x004ab610 */
extern const GUID_ GUID_DirectMusicAllTypes;      /* 0x004ab8b0 */
extern const GUID_ GUID_Download;                 /* 0x004ab7b0 */
extern const GUID_ GUID_PerfMasterGrooveLevel;    /* 0x004ab6d0 */
extern const GUID_ GUID_NOTIFICATION_MEASUREANDBEAT; /* 0x004ab880 */
extern const GUID_ GUID_NOTIFICATION_SEGMENT;     /* 0x004ab8a0 */

/* ---- callees ----------------------------------------------------------- */

extern void  DBPrintf(const char* fmt, ...);                    /* 0x00453a20 */
extern void  UpdateSoundVols(void);                             /* 0x00495a90 */
extern void* malloc(unsigned int size);                         /* 0x0049e4ff (CRT) */
extern void  free(void* p);                                     /* 0x0049e4d0 (CRT) */
extern int   _open(const char* path, int mode, ...);            /* 0x0049f6c0 (CRT) */
extern int   _close(int fd);                                    /* 0x0049f417 (CRT) */
extern int   _read(int fd, void* buf, unsigned int n);          /* 0x0049f4ca (CRT) */
extern long  _filelength(int fd);                               /* 0x004aacce (CRT) */
unsigned short* __cdecl wcscpy(unsigned short* dst, const unsigned short* src);
int __cdecl memcmp(const void* a, const void* b, unsigned int n);
#pragma intrinsic(memcmp)

__declspec(dllimport) long __stdcall CoInitialize(void* reserved);      /* [0x4ab34c] */
__declspec(dllimport) long __stdcall CoCreateInstance(const void* clsid, void* outer,
                                                      unsigned long ctx,
                                                      const void* iid,
                                                      void** out);      /* [0x4ab350] */
__declspec(dllimport) void* __stdcall CreateEventA(void* sa, int manual, int initial,
                                                   const char* name);   /* [0x4ab0e8] */
__declspec(dllimport) int __stdcall ResetEvent(void* h);                /* [0x4ab0e4] */
__declspec(dllimport) int __stdcall SetEvent(void* h);                  /* [0x4ab0f8] */
__declspec(dllimport) unsigned long __stdcall GetLastError(void);       /* [0x4ab0ec] */
__declspec(dllimport) unsigned long __stdcall
    WaitForSingleObject(void* h, unsigned long ms);                     /* [0x4ab110] */
__declspec(dllimport) unsigned long __stdcall
    WaitForMultipleObjects(unsigned long n, void* const* h, int all,
                           unsigned long ms);                           /* [0x4ab0f4] */

/* ---- the two repeated groups ------------------------------------------- */

/* Read one .sgt whole into g_seg_data[i].  The short-read log prints the
 * COMPARISON RESULT in the "got" column; reproduced. */
#define LOAD_SEGMENT(size, data, i, path)                                     \
    fd = _open(path, 0x8000);                                                 \
    size[i] = _filelength(fd);                                                \
    data[i] = (unsigned char*)malloc(size[i]);                                \
    if (data[i] != 0) {                                                       \
        bad = (_read(fd, data[i], size[i]) != size[i]);                       \
        if (bad)                                                              \
            DBPrintf("Not Enough Data %s (wanted %d, got %d)\n",              \
                     path, size[i], bad);                                     \
    } else {                                                                  \
        DBPrintf("Failed to allocate Music Object %s\n", path);               \
    }                                                                         \
    _close(fd);

/* Hand one in-memory blob to the loader as segment `i`, then free the blob. */
#define BUILD_SEGMENT(size, data, bank, i, wname)                             \
    g_getobj_calls++;                                                         \
    desc.dwSize = sizeof(desc);                                               \
    desc.dwValidData = 0x406;                                                 \
    desc.guidClass = CLSID_DirectMusicSegment;                                \
    desc.llMemLength = size[i];                                               \
    desc.pbMemData = data[i];                                                 \
    wcscpy(desc.wszName, wname);                                              \
    hr = g_dm_loader->lpVtbl->GetObject(g_dm_loader, &desc,                   \
                                        &IID_IDirectMusicSegment,             \
                                        (void**)&bank[i]);                    \
    if (hr != 0)                                                              \
        DBPrintf("Error Getting music object (Call %d) (Ret = %d) (Error = %d)\n", \
                 g_getobj_calls, hr, GetLastError());                         \
    free(data[i]);

/* Download one segment's instruments into the performance. */
#define DOWNLOAD_SEGMENT(e)                                                   \
    seg = e;                                                                  \
    seg->lpVtbl->SetParam(seg, &GUID_Download, 0xffffffff, 0, 0,              \
                          g_dm_performance);                                  \
    (e)->lpVtbl->SetRepeats(e, 0);

/* 3,161 of 3,161 instructions and 11,325 of 11,335 bytes; audit.py mismatch 7,
 * ALL of them one residual with no source-level handle found:
 *
 *   The original caches `__imp__WaitForSingleObject` in ESI and
 *   `__imp__ResetEvent` in EDI and reloads BOTH in TWO places -- once in the
 *   loop preheader (0x0049566f, order esi/edi) and once in the LATCH
 *   (0x004959e2, order edi/esi) -- because only the notification-pump path
 *   runs the `repe cmpsd` that clobbers esi and edi; the two other back edges
 *   (0x004957e5, 0x00495806) reach the head with them still live, so a repair
 *   copy on the killing edge alone is enough.  Ours loads them ONCE at the
 *   top of the loop body instead, which is correct and two instructions
 *   shorter but places the reload on every edge.  First diverging index 2917.
 *
 * Ruled out, each re-measured: `while (1)` vs `for (;;)`; a trailing
 * `continue`; swapping the two dllimport declarations; moving the `ev[]`
 * fill after the DBPrintf (19); a `volatile` free read of both event globals
 * (inert); `volatile int beat` (71).  Everything else in the body -- block
 * layout, all four callee-saved assignments, every frame home and both
 * thirty-fold macro expansions -- is index-for-index exact.
 *
 * `g_music_disabled` (0x007988bc) is declared VOLATILE here and that is
 * load-bearing, worth 9 of the 16 mismatches that stood before it: without it
 * VC6 hoists the two event-handle loads above the `= 1` store at 0x0049563c
 * and sinks the shutdown pair into the epilogue's pops.  It is the flag the
 * starter polls after CreateThread, so the qualifier is also what the
 * original must have meant; other files declare it plain `int`. */
/* Scope LL18 (2026-09-08): still 7, first 2917, 11325/11335 bytes -- AT ITS
 * FLOOR, and now with the MECHANISM.  A 96-instruction stand-alone model of
 * the message loop (two imports called twice each, the pump loop with the
 * memcmp intrinsic, `beat`; docs/lanes/scope-ll18.md) shows VC6 hoists an
 * `__imp__` load to the outer-loop PREHEADER only when it can give the web a
 * callee-saved register that is free across the WHOLE outer loop (drop
 * `beat` and Wait hoists into ebx; add a third call site and it hoists into
 * ebp), and otherwise defines the web at the loop HEAD -- our shape.  Here
 * every candidate is taken inside the pump (esi/edi by `repe cmpsd`, ebx by
 * the const-4 web, ebp by the promoted `beat`), so the original's esi/edi
 * hoist WITH a split (reload on the pump's exit edge, in the allocator's
 * edi/esi order rather than the scheduler's esi/edi) is an allocation-order
 * or cost-tie outcome the visible code does not determine.  Measured inert
 * on the real body (byte-identical objects): `if (beat) ;` block splits
 * before the loop, at the head and inside the pump; `beat = beat;` twice;
 * dead `if (0) { WaitForSingleObject(..); ResetEvent(..); }` before the loop
 * and inside the pump; `if (WaitForSingleObject && ResetEvent) ;`; a named
 * `guid` pointer for the memcmp; `beat` declared first; the two dllimport
 * declarations moved ahead of memcmp's; all five other orders of the
 * compare-chain `switch (g_imt_cmd)` cases.  Worse: any other order of the
 * jump-table notify switch (16 / 18 -- the source order IS the block order)
 * and swapping case 4's arms (33).  In the model `for (;;)`, `while (1)`,
 * `do .. while (1)`, a goto loop, a guarded do-while pump and a for/break
 * pump are byte-identical, so no loop syntax reaches it. */
// WIP-FUNCTION: LEGOLAND 0x00492db0  (3161/3161 insns, 11325/11335 bytes, mismatch 7; see the note above)
unsigned long __stdcall MusicThread(void* param)
{
    DMObjectDesc   desc;
    DMPortParams   pp;
    DSBufferDesc   dsbd;
    void*          ev[2];
    IDMusic*       dm;
    IDMPort*       port;
    IDMSegment*    seg;
    IDMObj**       style;
    DMNotifyMsg*   pmsg;
    void*          wfx;
    unsigned long  wfxsize;
    unsigned long  bufsize;
    char           groove;
    long           hr;
    int            fd;
    int            bad;
    int            n;
    int            a;
    int            b;
    int            beat;

    beat = -1;
    port = 0;
    dm = 0;
    pp.dwSize = sizeof(pp);
    pp.dwValidParams = 0;
    pp.dwVoices = 24;
    pp.dwChannelGroups = 1;
    pp.dwAudioChannels = 2;
    pp.dwSampleRate = 22050;
    pp.dwEffectFlags = 0;
    pp.fShare = 0;
    if (!g_music_sys) {
        g_music_disabled = 1;
        return 0;
    }
    desc.dwSize = sizeof(desc);
    CoInitialize(0);
    if (CoCreateInstance(&CLSID_DirectMusicComposer, 0, 3,
                         &IID_IDirectMusicComposer, (void**)&g_dm_composer) < 0)
        goto shutdown;
    if (CoCreateInstance(&CLSID_DirectMusicPerformance, 0, 3,
                         &IID_IDirectMusicPerformance,
                         (void**)&g_dm_performance) < 0)
        goto rel_composer;
    if (g_dm_performance->lpVtbl->Init(g_dm_performance, &dm, g_dsound, 0) < 0)
        goto rel_perf;
    dm->lpVtbl->CreatePort(dm, &GUID_NULL, &pp, &port, 0);
    /* CreatePort's result is used unconditionally here but null-checked
     * before the Release below -- the original's own inconsistency. */
    port->lpVtbl->GetFormat(port, 0, &wfxsize, &bufsize);
    /* The two arms must each push their own argument (the original's
     * `push 0x12 / jmp / push eax` pair cross-jumped onto one call); a
     * ternary folds them into one `mov eax,0x12`. */
    if (wfxsize < 0x12)
        wfx = malloc(0x12);
    else
        wfx = malloc(wfxsize);
    port->lpVtbl->GetFormat(port, wfx, &wfxsize, &bufsize);
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = 0x80;
    dsbd.dwBufferBytes = bufsize;
    dsbd.dwReserved = 0;
    dsbd.lpwfxFormat = wfx;
    /* guid3DAlgorithm is left uninitialised although dwSize claims 0x24.
     * And this failure jumps STRAIGHT to the exit, releasing neither the
     * performance nor the composer and leaking `wfx` -- the one rung of the
     * ladder that skips its own cleanup.  Both reproduced. */
    if (g_dsound->lpVtbl->CreateSoundBuffer(g_dsound, &dsbd, &g_music_buf, 0) != 0)
        goto shutdown;
    free(wfx);
    port->lpVtbl->SetDirectSound(port, g_dsound, g_music_buf);
    port->lpVtbl->Activate(port, 1);
    g_dm_performance->lpVtbl->AddPort(g_dm_performance, port);
    g_dm_performance->lpVtbl->AssignPChannelBlock(g_dm_performance, 0, port, 1);
    if (port)
        port->lpVtbl->Release(port);
    if (CoCreateInstance(&CLSID_DirectMusicLoader, 0, 3,
                         &IID_IDirectMusicLoader, (void**)&g_dm_loader) < 0)
        goto stop_perf;
    UpdateSoundVols();
    g_imt_notify_event = CreateEventA(0, 1, 0, 0);
    g_imt_event = CreateEventA(0, 1, 0, 0);
    if (g_imt_event == 0) {
        g_dm_loader->lpVtbl->ClearCache(g_dm_loader, &GUID_DirectMusicAllTypes);
        g_dm_loader->lpVtbl->Release(g_dm_loader);
stop_perf:
        g_dm_performance->lpVtbl->Stop(g_dm_performance, 0, 0, 0, 0);
        g_dm_performance->lpVtbl->CloseDown(g_dm_performance);
rel_perf:
        g_dm_performance->lpVtbl->Release(g_dm_performance);
rel_composer:
        g_dm_composer->lpVtbl->Release(g_dm_composer);
shutdown:
        g_music_ready = 0;
        g_music_disabled = 1;
        return 0;
    }

    g_dm_loader->lpVtbl->SetSearchDirectory(g_dm_loader, &GUID_DirectMusicAllTypes,
                                            L"imusic", 1);
    g_dm_loader->lpVtbl->EnableCache(g_dm_loader, &GUID_DirectMusicAllTypes, 1);
    if (g_dm_loader->lpVtbl->ScanDirectory(g_dm_loader, &CLSID_DirectMusicSegment,
                                           L"sgt", 0) == 0) {
        if (g_dm_loader->lpVtbl->ScanDirectory(g_dm_loader, &CLSID_DirectMusicStyle,
                                               L"sty", 0) == 0) {
            DBPrintf("Loading Styles\n");
            n = 0;
            if (g_dm_loader->lpVtbl->EnumObject(g_dm_loader, &CLSID_DirectMusicStyle,
                                                0, &desc) == 0) {
                style = g_styles;
                do {
                    g_dm_loader->lpVtbl->GetObject(g_dm_loader, &desc,
                                                   &IID_IDirectMusicStyle,
                                                   (void**)style);
                    n++;
                    style++;
                } while (g_dm_loader->lpVtbl->EnumObject(g_dm_loader,
                             &CLSID_DirectMusicStyle, n, &desc) == 0);
            }
        }

        DBPrintf("Loading Segments\n");
        LOAD_SEGMENT(g_first_size, g_first_data, 0, "imusic\\segtheme1.sgt")
        LOAD_SEGMENT(g_first_size, g_first_data, 1, "imusic\\segegypt1.sgt")
        LOAD_SEGMENT(g_first_size, g_first_data, 2, "imusic\\seginca1.sgt")
        LOAD_SEGMENT(g_first_size, g_first_data, 3, "imusic\\segmed1.sgt")
        LOAD_SEGMENT(g_first_size, g_first_data, 4, "imusic\\segwest1.sgt")
        LOAD_SEGMENT(g_second_size, g_second_data, 0, "imusic\\segtheme2.sgt")
        LOAD_SEGMENT(g_second_size, g_second_data, 1, "imusic\\segegypt2.sgt")
        LOAD_SEGMENT(g_second_size, g_second_data, 2, "imusic\\seginca2.sgt")
        LOAD_SEGMENT(g_second_size, g_second_data, 3, "imusic\\segmed2.sgt")
        LOAD_SEGMENT(g_second_size, g_second_data, 4, "imusic\\segwest2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data,  1, "imusic\\letran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data,  2, "imusic\\litran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data,  3, "imusic\\lmtran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data,  4, "imusic\\lwtran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data,  5, "imusic\\eltran2.sgt")
        /* ORIGINAL BUG: slot 7 is EGYPT -> INCA and should read
         * "eitran2.sgt"; the source pasted the i->e path instead. */
        LOAD_SEGMENT(g_trans_size, g_trans_data,  7, "imusic\\ietran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data,  8, "imusic\\emtran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data,  9, "imusic\\ewtran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data, 10, "imusic\\iltran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data, 11, "imusic\\ietran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data, 13, "imusic\\imtran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data, 14, "imusic\\iwtran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data, 15, "imusic\\mltran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data, 16, "imusic\\metran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data, 17, "imusic\\mitran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data, 19, "imusic\\mwtran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data, 20, "imusic\\wltran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data, 21, "imusic\\wetran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data, 22, "imusic\\witran2.sgt")
        LOAD_SEGMENT(g_trans_size, g_trans_data, 23, "imusic\\wmtran2.sgt")

        BUILD_SEGMENT(g_first_size, g_first_data, g_seg_first, 0, L"themeintro")
        BUILD_SEGMENT(g_second_size, g_second_data, g_seg_second, 0, L"theme")
        BUILD_SEGMENT(g_first_size, g_first_data, g_seg_first, 1, L"segegypt1")
        BUILD_SEGMENT(g_first_size, g_first_data, g_seg_first, 2, L"seginca1")
        BUILD_SEGMENT(g_first_size, g_first_data, g_seg_first, 3, L"segmed1")
        BUILD_SEGMENT(g_first_size, g_first_data, g_seg_first, 4, L"segwest1")
        BUILD_SEGMENT(g_second_size, g_second_data, g_seg_second, 1, L"segegypt2")
        BUILD_SEGMENT(g_second_size, g_second_data, g_seg_second, 2, L"seginca2")
        BUILD_SEGMENT(g_second_size, g_second_data, g_seg_second, 3, L"segmed2")
        BUILD_SEGMENT(g_second_size, g_second_data, g_seg_second, 4, L"segwest2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans,  1, L"letran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans,  2, L"litran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans,  3, L"lmtran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans,  4, L"lwtran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans,  5, L"eltran2")
        /* the other half of the slot-7 bug: the NAME asked for is right. */
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans,  7, L"eitran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans,  8, L"emtran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans,  9, L"ewtran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans, 10, L"iltran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans, 11, L"ietran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans, 13, L"imtran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans, 14, L"iwtran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans, 15, L"mltran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans, 16, L"metran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans, 17, L"mitran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans, 19, L"mwtran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans, 20, L"wltran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans, 21, L"wetran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans, 22, L"witran2")
        BUILD_SEGMENT(g_trans_size, g_trans_data, g_seg_trans, 23, L"wmtran2")
    }

    g_dm_performance->lpVtbl->SetNotificationHandle(g_dm_performance,
                                                    g_imt_notify_event, 0);
    g_dm_performance->lpVtbl->AddNotificationType(g_dm_performance,
                                                  &GUID_NOTIFICATION_MEASUREANDBEAT);
    g_dm_performance->lpVtbl->AddNotificationType(g_dm_performance,
                                                  &GUID_NOTIFICATION_SEGMENT);
    /* Download world 1..4's two segments and every transition OUT of them.
     * World 0 (the theme) and its four transitions are never downloaded. */
    for (a = 1; a < 5; a++) {
        DOWNLOAD_SEGMENT(g_seg_first[a])
        DOWNLOAD_SEGMENT(g_seg_second[a])
        for (b = 0; b < 5; b++) {
            if (b != a) {
                DOWNLOAD_SEGMENT(g_seg_trans[a * 5 + b])
            }
        }
    }
    g_music_disabled = 1;
    g_music_ready = 1;
    ev[0] = g_imt_notify_event;
    ev[1] = g_imt_event;
    DBPrintf("Entering IMT Control\n");

    for (;;) {
        WaitForMultipleObjects(2, ev, 0, 0xffffffff);
        if (WaitForSingleObject(g_imt_event, 0) == 0) {
            ResetEvent(g_imt_event);
            if (g_imt_cmd != 0) {
                switch (g_imt_cmd) {
                case 4:
                    if (g_imt_theme == g_imt_cmd_arg) {
                        g_imt_theme = g_imt_cmd_arg;
                        groove = 0;
                        g_dm_performance->lpVtbl->SetGlobalParam(g_dm_performance,
                            &GUID_PerfMasterGrooveLevel, &groove, 1);
                        g_dm_performance->lpVtbl->PlaySegment(g_dm_performance,
                            g_seg_second[g_imt_theme], 0x2000, 0, 0);
                    } else {
                        groove = 1;
                        g_imt_cmd = 5;
                        beat = -1;
                        g_dm_performance->lpVtbl->SetGlobalParam(g_dm_performance,
                            &GUID_PerfMasterGrooveLevel, &groove, 1);
                    }
                    break;
                case 3:
                    g_imt_theme = g_imt_cmd_arg;
                    groove = 0;
                    g_dm_performance->lpVtbl->SetGlobalParam(g_dm_performance,
                        &GUID_PerfMasterGrooveLevel, &groove, 1);
                    g_dm_performance->lpVtbl->PlaySegment(g_dm_performance,
                        g_seg_first[g_imt_theme], 0x2000, 0, 0);
                    break;
                case 1:
                    g_dm_performance->lpVtbl->Stop(g_dm_performance, 0, 0, 0, 0);
                    break;
                }
                g_imt_state = g_imt_cmd;
                g_imt_cmd = 0;
            }
        }
        if (WaitForSingleObject(g_imt_notify_event, 0) == 0) {
            ResetEvent(g_imt_notify_event);
            while (g_dm_performance->lpVtbl->GetNotificationPMsg(g_dm_performance,
                                                    (void**)&pmsg) == 0) {
                if (memcmp(&pmsg->guidNotificationType,
                           &GUID_NOTIFICATION_SEGMENT, 16) == 0) {
                    switch (pmsg->dwNotificationOption) {
                    case 4:
                        DBPrintf("IMT:Segment stopped\n");
                        if (g_imt_state == 6)
                            g_imt_state = 7;
                        break;
                    case 2:
                        DBPrintf("IMT:Segment almost end\n");
                        if (g_imt_state == 3 || g_imt_state == 4) {
                            g_imt_cmd = 4;
                            g_imt_cmd_arg = g_imt_theme;
                            SetEvent(g_imt_event);
                        }
                        break;
                    case 1:
                        DBPrintf("IMT:Segment end\n");
                        break;
                    case 3:
                        DBPrintf("IMT:Segment looped\n");
                        break;
                    case 0:
                        DBPrintf("IMT:Segment started\n");
                        break;
                    }
                } else if (g_imt_state == 7) {
                    if (pmsg->dwField1 == 3 && pmsg->dwField2 == 1) {
                        g_imt_theme = g_imt_cmd_arg;
                        g_dm_performance->lpVtbl->PlaySegment(g_dm_performance,
                            g_seg_second[g_imt_cmd_arg], 0x2000, 0, 0);
                        groove = 0;
                        g_dm_performance->lpVtbl->SetGlobalParam(g_dm_performance,
                            &GUID_PerfMasterGrooveLevel, &groove, 1);
                        g_imt_state = 4;
                        DBPrintf("IMT_INTERACTIVE command\n");
                    }
                } else {
                    if (g_imt_state == 5 && beat == -1)
                        beat = pmsg->dwField1;
                    if (pmsg->dwField1 == 0 && g_imt_state == 5) {
                        if (beat) {
                            g_imt_state = 6;
                            g_dm_performance->lpVtbl->PlaySegment(g_dm_performance,
                                g_seg_trans[g_imt_theme * 5 + g_imt_cmd_arg],
                                0x2000, 0, 0);
                        } else {
                            beat = 1;
                        }
                    }
                }
                g_dm_performance->lpVtbl->FreePMsg(g_dm_performance, pmsg);
            }
        }
    }
}
