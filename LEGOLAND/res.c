/* LEGOLAND — RES archive layer.
 *
 * The resource ("volume") reader used by every asset loader: RES_OpenFile
 * resolves a path of the form  <volume>:<member>  (or a plain path) against the
 * master directory, and RES_ReadFile streams bytes out of the member, sharing a
 * single OS file handle between all members of a volume. */
#include <stdlib.h>
#include <string.h>

/* A resource "volume" (open archive). Layout shared with LEGOLAND/sweep4.c. */
typedef struct RVol {
    char          pad0[0x1c];
    int           handle;   /* +0x1c  OS file handle */
    struct RFile* cur;      /* +0x20  file currently positioned in the volume */
    int           refcount; /* +0x24 */
} RVol;

/* An open resource file inside a volume. */
typedef struct RFile {
    int   size;             /* +0x00 */
    int   base;             /* +0x04  offset of the file within the volume */
    RVol* vol;              /* +0x08 */
    int   pos;              /* +0x0c  current read cursor */
} RFile;

/* kernel32 SetFilePointer / ReadFile. The original calls these indirectly
 * through the IAT ("call dword ptr [0x4ab104]"), so they must be declared
 * dllimport — a plain extern compiles to a direct rel32 call and will not
 * match. Same symbol names as LEGOLAND/sweep4.c. */
__declspec(dllimport) int __stdcall RES_LowSeek(int h, int off, int a, int b);              /* [0x4ab104] */
__declspec(dllimport) int __stdcall RES_LowRead(int h, void* buf, int n, int* got, int ov); /* [0x4ab264] */

// FUNCTION: LEGOLAND 0x00489cf0
int RES_ReadFile(RFile* f, void* buf, int count)
{
    RVol* v;
    int   got;

    if (!f)
        return -1;

    v = f->vol;
    if (f != v->cur) {
        v->cur = f;
        RES_LowSeek(v->handle, f->pos + f->base, 0, 0);
    }

    if (count > f->size - f->pos)
        count = f->size - f->pos;
    if (count == 0)
        return 0;

    RES_LowRead(v->handle, buf, count, &got, 0);
    f->pos += got;
    return got;
}

/* ------------------------------------------------------------------ *
 *  RES_OpenFile (0x00489b60)                                          *
 * ------------------------------------------------------------------ */

/* One member of the master directory: the flattened index of every file in
 * every mounted volume, bucketed by directory. */
typedef struct RDirEnt {
    int             pad0;    /* +0x00 */
    struct RDirEnt* next;    /* +0x04 */
    char            pad8[0x10 - 0x08];
    RVol*           vol;     /* +0x10  volume the member lives in */
    int             size;    /* +0x14 */
    int             base;    /* +0x18  offset of the member in the volume */
    char*           name;    /* +0x1c  member name (no directory part) */
} RDirEnt;

/* A directory bucket in the master directory. */
typedef struct RDir {
    int      pad0;           /* +0x00 */
    RDirEnt* files;          /* +0x04 */
} RDir;

extern int    RES_EnsureMounted(const char* volume);          /* 0x004515e0 */
extern RDir*  RES_GetMasterDir(const char* dir);              /* 0x004894d0 */
/* 0x00489a00 — the volume-relative opener. This call site passes the member
 * path first and the volume name second (LEGOLAND/sweep4.c names its params the
 * other way round; only the types matter, and they agree). */
extern void*  RES_OpenFileFromVolume(void* member, void* vol);

// FUNCTION: LEGOLAND 0x00489b60
void* RES_OpenFile(const char* path)
{
    char     name[260];
    /* "= {0}" (not "= \"\"") — the latter makes VC6 load the byte from the
     * string literal instead of storing an immediate 0. */
    char     dir[260] = {0};
    /* Initialiser EMISSION order is load-bearing here (last, then q, then p);
     * it is only the frame layout that is insensitive to declaration order.
     * q must also be seeded before the scan loop: keeping it live across the
     * loop is what forces `last` into a stack slot, which is what the original
     * does. */
    char*    last = 0;
    char*    q = name;
    char*    p = name;
    char*    member;
    RDir*    d;
    RDirEnt* e;
    RFile*   f;
    RVol*    v;
    int      n;

    if (!RES_EnsureMounted(0))
        exit(1);

    strcpy(name, path);

    /* The redundant "p &&" is real: it is what produces the entry test
     * (lea ecx,[name] / test ecx,ecx / je) and the "inc ebx / jne" back edge. */
    for (; p && *p; p++) {
        if (*p == ':') {
            /* "<volume>:<member>" — split and hand off to the volume reader. */
            *p = 0;
            return RES_OpenFileFromVolume(p + 1, name);
        }
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

    d = RES_GetMasterDir(dir);
    if (d) {
        e = d->files;
        while (e) {
            if (stricmp(e->name, member) == 0) {
                f = (RFile*)malloc(sizeof(RFile));
                f->size = e->size;
                f->base = e->base;
                v = e->vol;
                f->vol = v;
                v->refcount++;
                f->pos = 0;
                return f;
            }
            e = e->next;
        }
    }
    return 0;
}
