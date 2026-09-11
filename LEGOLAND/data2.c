/* LEGOLAND — RES volumes, LLIDB .ICM save/load, WAV samples and the 3D
 * character-manager init.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only the
 * struct field offsets, callee argument counts and global addresses are
 * load-bearing; names are ours. Types are defined LOCALLY on purpose (the
 * shared header is owned elsewhere).
 */
#include <string.h>
#include "legoland.h"

#pragma intrinsic(strcpy, strlen, memcpy)

/* ---- CRT ------------------------------------------------------------------
 * Low-level CRT file descriptors (not RES handles): 0x0049f6c0 _open,
 * 0x0049f4ca _read, 0x004a63e4 _write, 0x0049f417 _close. */
extern int   _open(const char* path, int oflag, ...);
extern int   _read(int fd, void* buf, unsigned int n);
extern int   _write(int fd, const void* buf, unsigned int n);
extern int   _close(int fd);
extern void* MemAlloc(unsigned int size);                 /* 0x0049e4ff (malloc) */
extern void  MemFree(void* p);                            /* 0x0049e4d0 (free) */
extern int   _stricmp(const char* a, const char* b);      /* 0x004aab90 */
extern unsigned int mystrlen(const char* s);              /* 0x0047fc20 */

/* ======================================================================== *
 *  LLIDB .ICM serialisation                                                *
 * ======================================================================== */

/* The .ICM on disk is the raw paged element table followed by the strings:
 *
 *     u32     count
 *     LLElem  elems[count]        raw 20-byte records, page by page
 *     per element:
 *        u32 len; char name[len]   (len 0 -> null)
 *        u32 len; char image[len]  (len 0 -> null)
 *
 * The raw records carry stale pointers; the loader rebuilds name/image and
 * clears data and the loaded/on-level bits. */

extern const char kIcmFileName[];   /* 0x004bc120 "LEGOLAND.ICM" */

// FUNCTION: LEGOLAND 0x0047bc80
int LLIDB_SaveICM(void)
{
    int          fd;
    unsigned int page;
    unsigned int remaining;
    unsigned int n, i;
    unsigned int len;

    fd = _open(kIcmFileName, 0x8302, 0x180);   /* O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE */
    if (fd == -1)
        return -2;

    _write(fd, &g_llidb_count, 4);
    remaining = g_llidb_count;
    for (page = 0; page < g_llidb_capacity >> 8; page++, remaining -= 0x100) {
        if (remaining >= 0x100)
            n = 0x100;
        else
            n = remaining;
        _write(fd, g_llidb_pages[page], n * sizeof(LLElem));
    }

    remaining = g_llidb_count;
    for (page = 0; page < g_llidb_capacity >> 8; page++, remaining -= 0x100) {
        if (remaining >= 0x100)
            n = 0x100;
        else
            n = remaining;
        for (i = 0; i < n; i++) {
            len = mystrlen(g_llidb_pages[page][i].name);
            _write(fd, &len, 4);
            _write(fd, g_llidb_pages[page][i].name, len);
            len = mystrlen(g_llidb_pages[page][i].image);
            _write(fd, &len, 4);
            _write(fd, g_llidb_pages[page][i].image, len);
        }
    }
    _close(fd);
    return 0;
}

/* ======================================================================== *
 *  3D character manager                                                    *
 * ======================================================================== */

/* A loaded ".loc" location/skeleton set; +0x04 holds the model context it
 * was loaded under. */
typedef struct LocSet {
    int   pad0;      /* +0x00 */
    void* ctx;       /* +0x04 */
} LocSet;

typedef struct Anim3D Anim3D;

/* The definition's real return type, for the portable build only: a stale
 * extern name whose signature disagrees with its body makes gen_link.py
 * bridge the two with a CAST, the cast call lowers to `call_indirect`, and
 * binaryen's directize pass turns a constant-index call_indirect into an
 * invalid DIRECT call -- the module then fails validation hundreds of
 * functions away (scope PORT-M2 section 4). 0x00485fc0 is tri3d.c:371 `void Render_SetPixelFormat(int)`; the result is discarded at data2.c:159. The VC6 arm is the
 * shipped spelling and its bytes cannot move: cdecl discards EAX here. */
#ifndef LEGOLAND_PORTABLE
extern void*    InitRasterBuffer(int mode);                                   /* 0x00485fc0 */
#else
extern void     InitRasterBuffer(int mode);                                   /* 0x00485fc0 */
#endif
extern void*    GetModelContext(void);                                        /* 0x00443710 (0x665e8c) */
extern LocSet*  LoadLocSet(const char* file, const char* dir);                /* 0x0043f990 */
extern void     LoadLocTextures(LocSet* set, const char* dir);                /* 0x00443720 */
extern Anim3D*  LoadAnim3D(const char* file, const char* dir, void* ctx);     /* 0x0043fa80 */
extern void     LoadAltTextures(const char* alt, const char* base, const char* dir,
                                int sex, void* ctx);                          /* 0x00442980 */
extern char*    LoadTextFile(const char* dir, const char* file);              /* 0x004402d0 */

extern int      g_screen_depth;          /* 0x00668088 */
extern LocSet*  g_anim_ctx_kind1;        /* 0x0081c8c0 */
extern LocSet*  g_anim_ctx_kind3;        /* 0x0081c8c4 */
extern LocSet*  g_anim_ctx_kind2;        /* 0x0081c8c8 */
extern char*    g_texnames_boy;          /* 0x00630100 */
extern char*    g_texnames_girl;         /* 0x0062feac */
extern Anim3D*  g_anim_kind2[2];         /* 0x0062feb0 */
extern Anim3D*  g_anim_kind1a[6];        /* 0x0062febc */
extern Anim3D*  g_anim_kind1b[6];        /* 0x0062fed4 */
extern Anim3D*  g_anim_kind3[1];         /* 0x0062fef4 */

extern const char kDirVisitor[];         /* 0x004b7cec "visitor" */
extern const char kLocVisitor[];         /* 0x004b7cdc "NewProject.loc" */
extern const char kAnimWomanWalk[];      /* 0x004b7cc4 */
extern const char kAnimWomanSit[];       /* 0x004b7cac */
extern const char kAnimWomanWave[];      /* 0x004b7c94 */
extern const char kAnimWomanStand[];     /* 0x004b7c78 */
extern const char kAnimWomanPanWalk[];   /* 0x004b7c5c */
extern const char kAnimWomanPan[];       /* 0x004b7c44 */
extern const char kAnimManWalk[];        /* 0x004b7c30 */
extern const char kAnimManSit[];         /* 0x004b7c1c */
extern const char kAnimManWave[];        /* 0x004b7c08 */
extern const char kAnimManStand[];       /* 0x004b7bf0 */
extern const char kAnimManPanWalk[];     /* 0x004b7bd4 */
extern const char kAnimManPan[];         /* 0x004b7bc0 */
extern const char kTxtVisitor[];         /* 0x004b7bb0 "NewProject.txt" */
extern const char kTxtAltMan[];          /* 0x004b7ba4 "altman.txt" */
extern const char kTxtAltWoman[];        /* 0x004b7b94 "altwoman.txt" */
extern const char kDirGeoff[];           /* 0x004b7b8c "geoff" */
extern const char kLocGeoff[];           /* 0x004b7b80 "geoff.loc" */
extern const char kAnimGeofWalk[];       /* 0x004b7b68 */
extern const char kAnimGeofPour[];       /* 0x004b7b50 */
extern const char kDirTracy[];           /* 0x004b7b48 "tracy" */
extern const char kLocTracy[];           /* 0x004b7b3c "tracy.loc" */
extern const char kAnimTracyWalk[];      /* 0x004b7b24 */

/* Bring up the 3D person system: the raster buffer (16-bit mode 6 when the
 * screen depth selector is 2, else 5), then the visitor, geoff and tracy
 * skeleton sets with their animations. Pairs with UnInitMan (rin.c). */
// FUNCTION: LEGOLAND 0x00440350
void InitMan(void)
{
    void* ctx;
    int   mode;

    /* Two constant arms -> `sete al / add eax,5`. The same value spelled as
     * `(g_screen_depth == 2) + 5` or a ternary also folds to sete, but leaves
     * every scratch-register choice in the function rotated by one. */
    if (g_screen_depth == 2)
        mode = 6;
    else
        mode = 5;
    InitRasterBuffer(mode);

    ctx = GetModelContext();
    g_anim_ctx_kind1 = LoadLocSet(kLocVisitor, kDirVisitor);
    g_anim_ctx_kind1->ctx = ctx;
    LoadLocTextures(g_anim_ctx_kind1, kDirVisitor);
    g_anim_kind1b[1] = LoadAnim3D(kAnimWomanWalk, kDirVisitor, ctx);
    g_anim_kind1b[0] = LoadAnim3D(kAnimWomanSit, kDirVisitor, ctx);
    g_anim_kind1b[2] = LoadAnim3D(kAnimWomanWave, kDirVisitor, ctx);
    g_anim_kind1b[3] = LoadAnim3D(kAnimWomanStand, kDirVisitor, ctx);
    g_anim_kind1b[5] = LoadAnim3D(kAnimWomanPanWalk, kDirVisitor, ctx);
    g_anim_kind1b[4] = LoadAnim3D(kAnimWomanPan, kDirVisitor, ctx);
    g_anim_kind1a[1] = LoadAnim3D(kAnimManWalk, kDirVisitor, ctx);
    g_anim_kind1a[0] = LoadAnim3D(kAnimManSit, kDirVisitor, ctx);
    g_anim_kind1a[2] = LoadAnim3D(kAnimManWave, kDirVisitor, ctx);
    g_anim_kind1a[3] = LoadAnim3D(kAnimManStand, kDirVisitor, ctx);
    g_anim_kind1a[5] = LoadAnim3D(kAnimManPanWalk, kDirVisitor, ctx);
    g_anim_kind1a[4] = LoadAnim3D(kAnimManPan, kDirVisitor, ctx);
    LoadAltTextures(kTxtAltMan, kTxtVisitor, kDirVisitor, 0, ctx);
    LoadAltTextures(kTxtAltWoman, kTxtVisitor, kDirVisitor, 1, ctx);
    g_texnames_boy = LoadTextFile(kDirVisitor, kTxtAltMan);
    g_texnames_girl = LoadTextFile(kDirVisitor, kTxtAltWoman);

    ctx = GetModelContext();
    g_anim_ctx_kind2 = LoadLocSet(kLocGeoff, kDirGeoff);
    g_anim_ctx_kind2->ctx = ctx;
    LoadLocTextures(g_anim_ctx_kind2, kDirGeoff);
    g_anim_kind2[0] = LoadAnim3D(kAnimGeofWalk, kDirGeoff, ctx);
    g_anim_kind2[1] = LoadAnim3D(kAnimGeofPour, kDirGeoff, ctx);

    ctx = GetModelContext();
    g_anim_ctx_kind3 = LoadLocSet(kLocTracy, kDirTracy);
    g_anim_ctx_kind3->ctx = ctx;
    LoadLocTextures(g_anim_ctx_kind3, kDirTracy);
    g_anim_kind3[0] = LoadAnim3D(kAnimTracyWalk, kDirTracy, ctx);
}

/* ======================================================================== *
 *  RES volumes                                                             *
 * ======================================================================== */

/* A mounted volume (0x28 bytes), chained from g_master_vols. */
typedef struct RVol {
    struct RVol*    next;      /* +0x00 */
    struct RDirEnt* files;     /* +0x04  first member of this volume */
    char            name[0x14];/* +0x08  upper-cased volume name */
    int             handle;    /* +0x1c  OS file handle */
    struct RFile*   cur;       /* +0x20  file currently positioned */
    int             refcount;  /* +0x24 */
} RVol;

/* An open resource file inside a volume. */
typedef struct RFile {
    int   size;   /* +0x00 */
    int   base;   /* +0x04 */
    RVol* vol;    /* +0x08 */
    int   pos;    /* +0x0c */
} RFile;

/* One member of the master directory (layout as LEGOLAND/res.c, plus the
 * per-volume chain at +0x08 and the owning directory bucket at +0x0c). */
typedef struct RDirEnt {
    int             pad0;    /* +0x00 */
    struct RDirEnt* next;    /* +0x04  next in the directory bucket */
    struct RDirEnt* vnext;   /* +0x08  next in the volume */
    void*           dir;     /* +0x0c  directory bucket this member is filed under */
    RVol*           vol;     /* +0x10 */
    int             size;    /* +0x14 */
    int             base;    /* +0x18 */
    char*           name;    /* +0x1c */
} RDirEnt;

/* 0x00489550: look a directory name up inside one volume; returns the
 * directory bucket (or 0) and hands back the volume's first member. */
extern void* RES_FindVolumeDir(const char* vol, const char* dir, RDirEnt** first);

/* Open "<dir>\<member>" inside the named volume: the same split as
 * RES_OpenFile (res.c) but the search is confined to the volume's member
 * chain. */
// FUNCTION: LEGOLAND 0x00489a00
RFile* RES_OpenFileFromVolume(const char* path, const char* volname)
{
    char     name[260];
    /* `last` must be initialised BEFORE dir (the zero it seeds into ebx is
     * what the dir[0] = 0 byte store reuses); res.c's RES_OpenFile has the
     * opposite order because its `last` lives in a stack slot. */
    char*    last = 0;
    char     dir[260] = {0};
    char*    q = name;
    char*    p = name;
    char*    member;
    RDirEnt* e;
    void*    d;
    RFile*   f;
    RVol*    v;
    int      n;

    strcpy(name, path);

    for (; p && *p; p++) {
        if (*p == '\\')
            last = p;
    }

    if (last) {
        n = (int)(last - name) + 1;
        while (q[0] == '.' && q[1] == '\\') {
            q += 2;
            n -= 2;
        }
        memcpy(dir, q, n);
        dir[n] = 0;
        member = last + 1;
    } else {
        member = name;
    }

    d = RES_FindVolumeDir(volname, dir, &e);
    if (d && e) {
        while (e) {
            if (e->dir == d && _stricmp(e->name, member) == 0) {
                f = (RFile*)MemAlloc(sizeof(RFile));
                f->size = e->size;
                f->base = e->base;
                v = e->vol;
                f->vol = v;
                v->refcount++;
                f->pos = 0;
                return f;
            }
            e = e->vnext;
        }
    }
    return 0;
}

/* ---- variadic debug log ---------------------------------------------------
 * 0x0047f870 / 0x0047f850 are both a bare `ret` in the shipped build (the
 * log was compiled out) but the calls and their argument cleanup remain. */
extern void DebugPrintf(const char* fmt, ...);   /* 0x0047f870 */
extern void DebugFlush(void);                    /* 0x0047f850 */

extern void _splitpath(const char* path, char* drive, char* dir,
                       char* fname, char* ext);  /* 0x0049ec85 */
extern int  toupper(int c);                      /* 0x0049f34b */
extern int  sprintf(char* buf, const char* fmt, ...); /* 0x0049e573 */

__declspec(dllimport) int __stdcall CreateFileA(const char* name, unsigned int access,
                                                unsigned int share, void* sa, unsigned int disp,
                                                unsigned int flags, void* tmpl);   /* [0x4ab258] */
__declspec(dllimport) int __stdcall GetFileSize(int h, unsigned int* hi);          /* [0x4ab25c] */
__declspec(dllimport) int __stdcall CloseHandle(int h);                            /* [0x4ab260] */
__declspec(dllimport) int __stdcall ReadFile(int h, void* buf, unsigned int n,
                                             unsigned int* got, void* ov);         /* [0x4ab264] */
__declspec(dllimport) int __stdcall SetFilePointer(int h, int off, int* hi,
                                                   unsigned int method);           /* [0x4ab104] */

/* 0x004895a0: walk a volume's directory image and file every member into the
 * master directory under `v`. */
extern void RES_LoadDirectory(char* image, RVol* v, char* base, void* root);

extern RVol* g_master_vols;       /* 0x00798628 */
extern char  g_res_path[];        /* 0x00813b04  alternate volume directory prefix */
extern char  g_res_root[];        /* 0x004d8bb0  root of the master directory */

extern const char kResAttempting[];   /* 0x004bde74 "Attempting to open Resource %s" */
extern const char kResVolumePath[];   /* 0x004bde60 ".\\volumes\\%s.res" */
extern const char kResTrying[];       /* 0x004bde48 "Trying to open from %s" */
extern const char kResAltPath[];      /* 0x004bde3c "%s%s.res" */
extern const char kResOpened[];       /* 0x004bde28 "Openned resource %s" */
extern const char kResFileSize[];     /* 0x004bde04 "FileSize = %x, Directory is at %x" */
extern const char kResAllocFail[];    /* 0x004bdddc "Failed to allocate space for directory" */
extern const char kResAlreadyOpen[];  /* 0x004bddc8 "Volume Already open" */
extern const char kResDirShort[];     /* 0x004bdd94 "Failed to load directory fully (%x of %x loaded)" */
extern const char kResDirOK[];        /* 0x004bdd80 "Directory read OK" */

/* Mount "<name>.res": a volume file starts with a u32 offset of its directory
 * image, which occupies the rest of the file. The image is read whole,
 * unpacked into the master directory by RES_LoadDirectory and freed. The
 * volume name (upper-cased, 20 chars) is what "<vol>:<member>" paths and
 * RES_OpenFileFromVolume look up. */
// FUNCTION: LEGOLAND 0x00489750
RVol* RES_OpenVolume(const char* path)
{
    char         fname[256];
    char         path2[260];
    int          dirsize;
    unsigned int got;
    RVol*        v;
    RVol*        p;
    char*        dir;
    int          size;
    int          len;
    int          i;

    v = (RVol*)MemAlloc(sizeof(RVol));
    p = g_master_vols;
    _splitpath(path, 0, 0, fname, 0);
    DebugPrintf(kResAttempting, fname);
    DebugFlush();

    len = strlen(fname);
    for (i = 0; i < 0x14; i++) {
        if (i < len)
            v->name[i] = (char)toupper(fname[i]);
        else
            v->name[i] = 0;
    }

    while (p) {
        if (_stricmp(p->name, v->name) == 0) {
            MemFree(v);
            DebugPrintf(kResAlreadyOpen);
            return p;
        }
        p = p->next;
    }

    sprintf(path2, kResVolumePath, fname);
    DebugPrintf(kResTrying, path2);
    DebugFlush();
    v->handle = CreateFileA(path2, 0x80000000, 1, 0, 3, 0x8000000, 0);
    if (v->handle == -1) {
        sprintf(path2, kResAltPath, g_res_path, fname);
        DebugPrintf(kResTrying, path2);
        DebugFlush();
        v->handle = CreateFileA(path2, 0x80000000, 1, 0, 3, 0x8000000, 0);
    }
    if (v->handle != -1) {
        DebugPrintf(kResOpened, path2);
        size = GetFileSize(v->handle, 0);
        SetFilePointer(v->handle, 0, 0, 0);
        ReadFile(v->handle, &dirsize, 4, &got, 0);
        DebugPrintf(kResFileSize, size, dirsize);
        dir = (char*)MemAlloc(size - dirsize);
        if (!dir) {
            DebugPrintf(kResAllocFail);
        } else {
            SetFilePointer(v->handle, dirsize, 0, 0);
            ReadFile(v->handle, dir, size - dirsize, &got, 0);
            if (got != (unsigned int)(size - dirsize)) {
                DebugPrintf(kResDirShort, got, size - dirsize);
                MemFree(dir);
            } else {
                DebugPrintf(kResDirOK);
                v->files = 0;
                RES_LoadDirectory(dir, v, dir, g_res_root);
                MemFree(dir);
                v->next = g_master_vols;
                g_master_vols = v;
                v->refcount = 1;
                v->cur = 0;
                return v;
            }
        }
        CloseHandle(v->handle);
    }
    MemFree(v);
    return 0;
}

/* ======================================================================== *
 *  LLIDB_LoadICM                                                           *
 * ======================================================================== */

extern LLElem*    ElemID(const char* name);   /* 0x0047b3f0 */
extern char       g_language[];               /* 0x004bc0ec "english" */
extern const char kLanguageElem[];            /* 0x004bc114 "LANGUAGE" */

/* Load LEGOLAND.ICM (see LLIDB_SaveICM for the layout). A missing file is
 * created empty (count 0) and the call returns 0 without a database. */
// FUNCTION: LEGOLAND 0x0047aff0
int LLIDB_LoadICM(void)
{
    int          fd;
    unsigned int page;
    unsigned int remaining;
    unsigned int n, i;
    unsigned int len;
    unsigned int zero;
    LLElem*      lang;

    fd = _open(kIcmFileName, 0x8000);          /* O_BINARY */
    if (fd == -1) {
        fd = _open(kIcmFileName, 0x8302, 0x180);
        if (fd == -1)
            return -2;
        zero = 0;
        _write(fd, &zero, 4);
        _close(fd);
        return 0;
    }

    _read(fd, &g_llidb_count, 4);
    g_llidb_capacity = (g_llidb_count + 0xff) & ~0xff;
    g_llidb_pages = (LLElem**)MemAlloc((g_llidb_capacity >> 8) * 4);
    for (page = 0; page < g_llidb_capacity >> 8; page++)
        g_llidb_pages[page] = (LLElem*)MemAlloc(0x1400);

    remaining = g_llidb_count;
    for (page = 0; page < g_llidb_capacity >> 8; page++, remaining -= 0x100) {
        if (remaining >= 0x100)
            n = 0x100;
        else
            n = remaining;
        _read(fd, g_llidb_pages[page], n * sizeof(LLElem));
    }

    remaining = g_llidb_count;
    for (page = 0; page < g_llidb_capacity >> 8; page++, remaining -= 0x100) {
        if (remaining >= 0x100)
            n = 0x100;
        else
            n = remaining;
        for (i = 0; i < n; i++) {
            g_llidb_pages[page][i].type_flags &= ~0xa;
            _read(fd, &len, 4);
            if (len == 0) {
                g_llidb_pages[page][i].name = 0;
            } else {
                g_llidb_pages[page][i].name = (char*)MemAlloc(len + 1);
                g_llidb_pages[page][i].name[len] = 0;
                _read(fd, g_llidb_pages[page][i].name, len);
            }
            _read(fd, &len, 4);
            if (len == 0) {
                g_llidb_pages[page][i].image = 0;
            } else {
                g_llidb_pages[page][i].image = (char*)MemAlloc(len + 1);
                g_llidb_pages[page][i].image[len] = 0;
                _read(fd, g_llidb_pages[page][i].image, len);
            }
            g_llidb_pages[page][i].type_flags &= ~1;
            /* The raw record's stale `data` pointer is NOT cleared — only the
             * loaded bit and the refcount are; LoadData rebuilds it. */
            g_llidb_pages[page][i].refcount = 0;
        }
    }
    _close(fd);

    lang = ElemID(kLanguageElem);
    if (lang)
        strcpy(g_language, lang->image);
    return 0;
}

/* ======================================================================== *
 *  CreateSampleFromWAV                                                     *
 * ======================================================================== */

/* The Sample record as LEGOLAND/audio3.c lays it out (0x38 bytes). */
typedef struct IDSBuffer IDSBuffer;
typedef struct IDSound   IDSound;

typedef struct Sample {
    struct Sample* next;     /* +0x00 */
    int            refcount; /* +0x04 */
    char           pad08[0x28 - 0x08];
    struct Sample* def;      /* +0x28 */
    IDSBuffer*     buf;      /* +0x2c */
    void*          fmt;      /* +0x30  WAVEFORMATEX */
    void*          data;     /* +0x34  PCM bytes */
} Sample;

typedef struct IDSBufferVtbl {
    long          (__stdcall *QueryInterface)(IDSBuffer*, const void*, void**); /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDSBuffer*);                             /* +0x04 */
    unsigned long (__stdcall *Release)(IDSBuffer*);                            /* +0x08 */
    void*         pad0c[8];                                                    /* +0x0c..0x28 */
    long          (__stdcall *Lock)(IDSBuffer*, unsigned long, unsigned long,
                                    void**, unsigned long*, void**,
                                    unsigned long*, unsigned long);            /* +0x2c */
    void*         pad30[7];                                                    /* +0x30..0x48 */
    long          (__stdcall *Unlock)(IDSBuffer*, void*, unsigned long,
                                      void*, unsigned long);                   /* +0x4c */
} IDSBufferVtbl;
struct IDSBuffer { IDSBufferVtbl* lpVtbl; };

typedef struct IDSoundVtbl {
    void*         pad00[3];                                                    /* +0x00..0x08 */
    long          (__stdcall *CreateSoundBuffer)(IDSound*, const void*, IDSBuffer**, void*); /* +0x0c */
} IDSoundVtbl;
struct IDSound { IDSoundVtbl* lpVtbl; };

/* DSBUFFERDESC (DirectX 7: 0x24 bytes with the 3D algorithm GUID). */
typedef struct DSBufferDesc {
    unsigned int  dwSize;          /* +0x00 */
    unsigned int  dwFlags;         /* +0x04 */
    unsigned int  dwBufferBytes;   /* +0x08 */
    unsigned int  dwReserved;      /* +0x0c */
    void*         lpwfxFormat;     /* +0x10 */
    unsigned char guid3DAlgorithm[16]; /* +0x14 */
} DSBufferDesc;

/* WAVEFORMATEX; cbSize at +0x10. */
typedef struct WaveFormatEx {
    unsigned short wFormatTag;      /* +0x00 */
    unsigned short nChannels;       /* +0x02 */
    unsigned int   nSamplesPerSec;  /* +0x04 */
    unsigned int   nAvgBytesPerSec; /* +0x08 */
    unsigned short nBlockAlign;     /* +0x0c */
    unsigned short wBitsPerSample;  /* +0x0e */
    unsigned short cbSize;          /* +0x10 */
} WaveFormatEx;

extern int      g_samples_ready;   /* 0x007988c0 */
extern IDSound* g_dsound;          /* 0x007cad40 */

extern void*   RES_OpenFile(const char* path);            /* 0x00489b60 */
extern int     RES_ReadFile(void* f, void* buf, int n);   /* 0x00489cf0 */
extern int     RES_CloseFile(void* f);                    /* 0x00489de0 */
/* 0x004921c0: run the chunk through the ACM into 16-bit PCM; returns the new
 * buffer (and rewrites *len), 0 on failure. */
extern void*   ConvertWAVToPCM(void* data, WaveFormatEx* fmt, unsigned int* len);
extern Sample* NewSampleRecord(void);                     /* 0x004920e0 */

/* Load a RIFF/WAVE resource into a static DirectSound buffer and return its
 * master Sample definition. Chunks before "data" are read and discarded; the
 * format chunk is kept (padded to a full WAVEFORMATEX) as the definition's
 * +0x30 block and the converted PCM as +0x34. */
// FUNCTION: LEGOLAND 0x00492380
Sample* CreateSampleFromWAV(const char* path)
{
    unsigned int  tag;
    unsigned int  len;
    IDSBuffer*    buf = 0;
    WaveFormatEx* fmt;
    void*         data;
    void*         conv;
    void*         chunk;
    void*         ptr1;
    unsigned long bytes1;
    DSBufferDesc  desc;
    Sample*       s;
    void*         f;
    int           n;

    if (!g_samples_ready)
        return 0;
    f = RES_OpenFile(path);
    if (!f)
        goto done;      /* shares the final return 0 (not an inline copy) */

    if (RES_ReadFile(f, &tag, 4) != 4)
        goto close;
    if (tag != 0x46464952)                      /* 'RIFF' */
        goto close;
    if (RES_ReadFile(f, &len, 4) != 4)
        goto close;
    if (RES_ReadFile(f, &tag, 4) != 4)
        goto close;
    if (tag != 0x45564157)                      /* 'WAVE' */
        goto close;
    if (RES_ReadFile(f, &tag, 4) != 4)          /* 'fmt ' */
        goto close;
    if (RES_ReadFile(f, &len, 4) != 4)
        goto close;

    /* 0x12 literal: sizeof(WAVEFORMATEX) is padded to 0x14 in a C struct. */
    if (len < 0x12)
        fmt = (WaveFormatEx*)MemAlloc(0x12);
    else
        fmt = (WaveFormatEx*)MemAlloc(len);
    if ((unsigned int)RES_ReadFile(f, fmt, len) != len)
        goto free_fmt;
    if (len <= 0x12)
        fmt->cbSize = 0;

    /* Chunk walk. The shape is a plain rotated while(read == 4) loop (entry
     * read + `jne fail`, bottom read + `je top / jmp fail`, `push 4` hoisted
     * above the tag test because both arms start a RES_ReadFile(f,&len,4)),
     * with the "data" arm placed after the loop via goto.
     *
     * LEVER (VC6 epilogue mode): the leading `return 0` keeps its own inline
     * pop/pop/pop/xor/pop/add/ret copy in the original while every later
     * failure shares the trailing one. VC6 either cross-jumps ALL identical
     * `return 0` blocks into the final one (a `while (cond)` loop with goto
     * exits does this) or duplicates the epilogue into EVERY return (then
     * `goto done` is what selects the shared copy). What flips it here is the
     * redundant `if (n != 4) goto free_fmt;` after the loop: it is threaded
     * away on both read-failure exits (identical compare), so it costs no
     * code, but its presence puts the function in inline-epilogue mode. A
     * `while(1)` + goto spelling also gives inline mode but loses the
     * conditional back-edge; a do/while gets rotated around the tag test. */
    while ((n = RES_ReadFile(f, &tag, 4)) == 4) {
        if (tag == 0x61746164)                  /* 'data' */
            goto data_found;
        if (RES_ReadFile(f, &len, 4) != 4)
            goto free_fmt;
        chunk = MemAlloc(len);
        if ((unsigned int)RES_ReadFile(f, chunk, len) != len) {
            MemFree(chunk);
            goto free_fmt;
        }
        MemFree(chunk);
    }
    if (n != 4)                                 /* always true: see LEVER above */
        goto free_fmt;

data_found:
    if (RES_ReadFile(f, &len, 4) != 4)
        goto free_fmt;
    data = MemAlloc(len);
    if ((unsigned int)RES_ReadFile(f, data, len) != len)
        goto free_data;
    conv = ConvertWAVToPCM(data, fmt, &len);
    if (!conv)
        goto free_data;
    data = conv;

    desc.dwBufferBytes = len;
    desc.dwSize = sizeof(DSBufferDesc);
    desc.dwFlags = 0xe0;                        /* CTRLVOLUME|CTRLPAN|CTRLFREQUENCY */
    desc.dwReserved = 0;
    desc.lpwfxFormat = fmt;
    if (g_dsound->lpVtbl->CreateSoundBuffer(g_dsound, &desc, &buf, 0) != 0)
        goto free_data;
    if (buf->lpVtbl->Lock(buf, 0, 0, &ptr1, &bytes1, 0, 0, 2) != 0)   /* DSBLOCK_ENTIREBUFFER */
        goto release;
    memcpy(ptr1, conv, bytes1);
    buf->lpVtbl->Unlock(buf, ptr1, bytes1, 0, 0);
    s = NewSampleRecord();
    if (!s)
        goto release;
    s->refcount++;
    s->def = 0;
    s->buf = buf;
    s->fmt = fmt;
    s->data = conv;
    RES_CloseFile(f);
    return s;

release:
    buf->lpVtbl->Release(buf);
free_data:
    MemFree(data);
free_fmt:
    MemFree(fmt);
close:
    RES_CloseFile(f);
done:
    return 0;
}
