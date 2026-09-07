/* LEGOLAND — RES volume directory unpacking, the WAV -> PCM ACM conversion
 * and the music-thread suspend / resume stubs (scope AJ).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only
 * struct field offsets, callee argument counts and global addresses are
 * load-bearing; names are ours. Types are defined LOCALLY on purpose
 * (legoland.h is owned elsewhere; data2.c / res.c carry the same RES
 * records). Extern prototype TYPES are caller-side codegen levers and are
 * declared the way THIS file's bodies need them.
 */
#include <string.h>

#pragma intrinsic(strlen, strcpy, strcat, memset)

/* ---- allocator -------------------------------------------------------------- */
extern void* MemAlloc(unsigned int size);                    /* 0x0049e4ff */
extern void  MemFree(void* p);                               /* 0x0049e4d0 */

/* =========================================================================
 *  RES volumes
 * ========================================================================= */

/* A mounted volume (data2.c's RVol, 0x28 bytes). */
typedef struct RVol {
    struct RVol*    next;      /* +0x00 */
    struct RDirEnt* files;     /* +0x04  first member of this volume */
    char            name[0x14];/* +0x08 */
    int             handle;    /* +0x1c */
    void*           cur;       /* +0x20 */
    int             refcount;  /* +0x24 */
} RVol;

/* One member of the master directory (data2.c's RDirEnt, 0x20 bytes). */
typedef struct RDirEnt {
    struct RDirEnt* anext;   /* +0x00  next on the all-members chain */
    struct RDirEnt* next;    /* +0x04  next in the directory bucket */
    struct RDirEnt* vnext;   /* +0x08  next in the volume */
    struct RDir*    dir;     /* +0x0c  directory bucket this member is filed under */
    RVol*           vol;     /* +0x10 */
    int             size;    /* +0x14 */
    int             base;    /* +0x18  offset of the member in the volume */
    char*           name;    /* +0x1c  member name (no directory part) */
} RDirEnt;

/* A directory bucket in the master directory (audio3.c's MasterDir). */
typedef struct RDir {
    struct RDir* next;       /* +0x00 */
    RDirEnt*     files;      /* +0x04 */
    char*        name;       /* +0x08 */
} RDir;

/* One node of a volume's directory image: a tree of files and directories
 * linked by IMAGE OFFSETS (-1 = none) that RES_LoadDirectory rewrites into
 * absolute pointers as it descends. A directory node has `isdir` set and
 * its members hang off `sub`; `sib` is the next entry of the same
 * directory. */
typedef struct RImgNode {
    int  sib;      /* +0x00  next entry in this directory (offset / pointer) */
    int  sub;      /* +0x04  first entry of a sub-directory (offset / pointer) */
    int  isdir;    /* +0x08  0 = file */
    int  size;     /* +0x0c */
    int  base;     /* +0x10  offset of the file within the volume */
    char name[1];  /* +0x14  inline name */
} RImgNode;

extern RDirEnt* g_res_all_files;                             /* 0x0079862c */
extern const char g_res_backslash[];                         /* 0x004bdd7c "\\" */

/* 0x00489440 (not exported): the master directory bucket for `name`,
 * created (name copied) if it does not exist yet; audio3.c's
 * GetMasterDirPtr (0x004894d0) is the look-up-only sibling. */
extern RDir* RES_GetOrAddMasterDir(const char* name);       /* 0x00489440 */

/* Files every node of a volume's directory image into the master directory
 * under `path`. `node` is the image node to start from, `base` the image
 * (offsets in the image are relative to it), `path` the directory name the
 * node's siblings are filed under. Called by RES_OpenVolume (data2.c) with
 * the image root and the root directory name.
 *
 * Every offset link is rewritten in place with `base` added before it is
 * followed, so the image is a pointer tree afterwards. A directory node
 * contributes its name + "\" to the path of its `sib` chain (the image
 * spells a directory's contents as the siblings of the directory node
 * itself, and its own children as `sub`). */
// FUNCTION: LEGOLAND 0x004895a0
void RES_LoadDirectory(RImgNode* node, RVol* v, char* base, const char* path)
{
    char     dir[260];
    RDir*    d;
    RDirEnt* e;

    strcpy(dir, path);
    d = RES_GetOrAddMasterDir(dir);
    if (node->isdir == 0) {
        e = (RDirEnt*)MemAlloc(sizeof(RDirEnt));
        e->anext = g_res_all_files;
        g_res_all_files = e;
        e->next = d->files;
        d->files = e;
        e->vnext = v->files;
        v->files = e;
        e->size = node->size;
        e->base = node->base;
        e->name = (char*)MemAlloc(strlen(node->name) + 1);
        strcpy(e->name, node->name);
        e->dir = d;
        e->vol = v;
    }
    if (node->sub != -1) {
        node->sub += (int)base;
        RES_LoadDirectory((RImgNode*)node->sub, v, base, dir);
    }
    if (node->sib != -1) {
        strcat(dir, node->name);
        strcat(dir, g_res_backslash);
        node->sib += (int)base;
        RES_LoadDirectory((RImgNode*)node->sib, v, base, dir);
    }
}

/* =========================================================================
 *  WAV -> 16-bit PCM through the ACM
 * ========================================================================= */

/* WAVEFORMATEX (data2.c's WaveFormatEx); cbSize at +0x10. Packed to its
 * 18 bytes: the original copies it as four dwords and a word. */
#pragma pack(push, 2)
typedef struct WaveFormatEx {
    unsigned short wFormatTag;      /* +0x00 */
    unsigned short nChannels;       /* +0x02 */
    unsigned int   nSamplesPerSec;  /* +0x04 */
    unsigned int   nAvgBytesPerSec; /* +0x08 */
    unsigned short nBlockAlign;     /* +0x0c */
    unsigned short wBitsPerSample;  /* +0x0e */
    unsigned short cbSize;          /* +0x10 */
} WaveFormatEx;
#pragma pack(pop)

/* ACMSTREAMHEADER (audio4.c's ACMHeader), 0x54 bytes. */
typedef struct ACMHeader {
    unsigned long cbStruct;         /* +0x00 */
    unsigned long fdwStatus;        /* +0x04 */
    unsigned long dwUser;           /* +0x08 */
    void*         pbSrc;            /* +0x0c */
    unsigned long cbSrcLength;      /* +0x10 */
    unsigned long cbSrcLengthUsed;  /* +0x14 */
    unsigned long dwSrcUser;        /* +0x18 */
    void*         pbDst;            /* +0x1c */
    unsigned long cbDstLength;      /* +0x20 */
    unsigned long cbDstLengthUsed;  /* +0x24 */
    unsigned long dwDstUser;        /* +0x28 */
    unsigned long reserved[10];     /* +0x2c */
} ACMHeader;

/* ACM entry points, called through the linker's thunks WITHOUT
 * __declspec(dllimport), as audio4.c and movie2.c spell them. */
int __stdcall acmStreamOpen(void**, void*, WaveFormatEx*, WaveFormatEx*, void*,
                            unsigned long, unsigned long, unsigned long);      /* 0x0049e3be */
int __stdcall acmStreamSize(void*, unsigned long, unsigned long*, unsigned long); /* 0x0049e3b8 */
int __stdcall acmStreamPrepareHeader(void*, ACMHeader*, unsigned long);        /* 0x0049e3d0 */
int __stdcall acmStreamConvert(void*, ACMHeader*, unsigned long);              /* 0x0049e3ca */
int __stdcall acmStreamUnprepareHeader(void*, ACMHeader*, unsigned long);      /* 0x0049e3c4 */

/* Runs a WAV data chunk through the ACM into 16-bit PCM of the same rate
 * and channel count. Returns the new buffer, frees `data`, rewrites *len to
 * the converted length and *fmt to the PCM format; 0 (with `data` intact)
 * when the ACM refuses.
 *
 * ORIGINAL BUG, reproduced: the ACM stream is never acmStreamClose()d, so a
 * stream handle leaks per converted sample. */
// FUNCTION: LEGOLAND 0x004921c0
void* ConvertWAVToPCM(void* data, WaveFormatEx* fmt, unsigned long* len)
{
    unsigned long dstlen;
    WaveFormatEx  pcm;
    ACMHeader     hdr;
    void*         has;
    void*         dst;

    pcm = *fmt;
    pcm.wFormatTag = 1;                                 /* WAVE_FORMAT_PCM */
    pcm.nBlockAlign = fmt->nChannels * 2;
    pcm.nAvgBytesPerSec = fmt->nSamplesPerSec * pcm.nBlockAlign;
    pcm.wBitsPerSample = 16;
    pcm.cbSize = 0;
    if (acmStreamOpen(&has, 0, fmt, &pcm, 0, 0, 0, 4) != 0)   /* ACM_STREAMOPENF_NONREALTIME */
        return 0;
    if (acmStreamSize(has, *len, &dstlen, 0) != 0)             /* ACM_STREAMSIZEF_SOURCE */
        return 0;
    dst = MemAlloc(dstlen);
    if (!dst)
        return 0;
    memset(&hdr, 0, sizeof(hdr));
    hdr.cbStruct = sizeof(hdr);
    hdr.pbSrc = data;
    hdr.cbSrcLength = *len;
    hdr.pbDst = dst;
    hdr.cbDstLength = dstlen;
    if (acmStreamPrepareHeader(has, &hdr, 0) != 0) {
        MemFree(dst);
        return 0;
    }
    if (acmStreamConvert(has, &hdr, 0x10) != 0) {              /* ACM_STREAMCONVERTF_START */
        MemFree(dst);
        return 0;
    }
    MemFree(data);
    *len = hdr.cbDstLengthUsed;
    *fmt = pcm;
    acmStreamUnprepareHeader(has, &hdr, 0);
    return dst;
}

/* =========================================================================
 *  Music thread
 * ========================================================================= */

extern void* g_music_sys;                                    /* 0x004bf774  music engine instance */
extern void* g_music_thread;                                 /* 0x0079a698 */

__declspec(dllimport) unsigned long __stdcall SuspendThread(void* thread);   /* [0x4ab100] */
__declspec(dllimport) unsigned long __stdcall ResumeThread(void* thread);    /* [0x4ab0f0] */

/* Suspend the music thread if the music system is up. */
// FUNCTION: LEGOLAND 0x00492c60
void SuspendMusicThread(void)
{
    if (g_music_sys)
        SuspendThread(g_music_thread);
}

/* Resume it. */
// FUNCTION: LEGOLAND 0x00492c80
void ResumeMusicThread(void)
{
    if (g_music_sys)
        ResumeThread(g_music_thread);
}
