/* LEGOLAND — small-leaf sweep, chunk 4. */
#include "legoland.h"

/* ------------------------------------------------------------------ *
 *  Resource / volume-file accessors (0x00489740..)                    *
 * ------------------------------------------------------------------ */

/* A resource "volume" (open .cab / archive). */
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

extern void* RES_OpenFileFromVolume(void* vol, void* rec);
extern void  RES_FreeFile(void* f);                       /* 0x0049e4d0 */
extern int __stdcall RES_LowSeek(int h, int off, int a, int b);          /* [0x4ab104] */
extern int __stdcall RES_LowRead(int h, void* buf, int n, int* got, int ov); /* [0x4ab264] */

int RES_CloseVolume(RVol* v);   /* fwd (called by RES_CloseFile) */

// FUNCTION: LEGOLAND 0x00489740
int RES_GetResourcePath(void)
{
    return 0;
}

// FUNCTION: LEGOLAND 0x00489ce0
int RES_GetFileSize(RFile* f)
{
    if (!f)
        return -1;
    return f->size;
}

// FUNCTION: LEGOLAND 0x00489db0
int RES_GetFilePointer(RFile* f)
{
    if (!f)
        return -1;
    return f->pos;
}

// FUNCTION: LEGOLAND 0x00489dc0
int RES_CloseVolume(RVol* v)
{
    if (!v)
        return -1;
    return --v->refcount;
}

// FUNCTION: LEGOLAND 0x00489de0
int RES_CloseFile(RFile* f)
{
    RVol* v;
    if (!f)
        return -1;
    v = f->vol;
    if (v->cur == f)
        v->cur = 0;
    RES_CloseVolume(v);
    RES_FreeFile(f);
    return 0;
}

// FUNCTION: LEGOLAND 0x00489e10
void* RES_OpenFileFromVolumePtr(void* vol, char* rec)
{
    if (!rec)
        return rec;
    return RES_OpenFileFromVolume(vol, rec + 8);
}

/* ------------------------------------------------------------------ *
 *  Instance list insertion (0x0048a010)                               *
 * ------------------------------------------------------------------ */

typedef struct Node {
    struct Node* prev;   /* +0x00 */
    struct Node* next;   /* +0x04 */
    struct NList* list;  /* +0x08 */
} Node;
typedef struct NList {
    int   pad0;          /* +0x00 */
    Node* tail;          /* +0x04 */
} NList;

// FUNCTION: LEGOLAND 0x0048a010
void AddInstanceToList(Node* node)
{
    NList* list = node->list;
    node->next = 0;
    node->prev = 0;
    if (list->tail == 0) {
        list->tail = node;
        return;
    }
    node->prev = list->tail;
    list->tail->next = node;
    node->list->tail = node;
}

// FUNCTION: LEGOLAND 0x0048a0f0
void HandleRideAI(void)
{
}

// FUNCTION: LEGOLAND 0x0048a2d0
void UpdateBlokesOnRide(void)
{
}

/* ------------------------------------------------------------------ *
 *  Clipping rectangle save/restore (0x0048a630..)                     *
 * ------------------------------------------------------------------ */

typedef struct ClipRect { int a, b, c, d; } ClipRect;
extern ClipRect g_clip;         /* 0x004bdea0 */
extern ClipRect g_clip_saved;   /* 0x00798630 */

// FUNCTION: LEGOLAND 0x0048a630
void GetClipping(ClipRect* out)
{
    *out = g_clip;
}

// FUNCTION: LEGOLAND 0x0048a660
void StoreClipping(void)
{
    g_clip_saved = g_clip;
}

// FUNCTION: LEGOLAND 0x0048a690
void RestoreClipping(void)
{
    g_clip = g_clip_saved;
}

/* ------------------------------------------------------------------ *
 *  Save-panel backdrop lookup (0x0048db50)                            *
 * ------------------------------------------------------------------ */

extern int g_savebk[8];   /* 0x00798708 */

// FUNCTION: LEGOLAND 0x004913e0
int ReturnFrom_ProfileDir(void)
{
    return 1;
}

/* ------------------------------------------------------------------ *
 *  Sound sample system (0x00492690..)                                 *
 * ------------------------------------------------------------------ */

extern int   g_sound_on;   /* 0x007988c0 */

typedef struct SndVtbl {
    char pad0[8];
    int  (*f8)(void*);                    /* +0x08 */
    char pad0c[0x14 - 0x0c];
    int  (*f14)(void*, void*, void*);     /* +0x14 */
    char pad18[0x30 - 0x18];
    int  (*f30)(void*, int, int, int);    /* +0x30 */
    int  (*f34)(void*, int);              /* +0x34 */
    char pad38[0x3c - 0x38];
    int  (*f3c)(void*, int);              /* +0x3c */
} SndVtbl;
typedef struct SndObj { SndVtbl* vtbl; } SndObj;   /* +0x00 vtable */

typedef struct Sample {
    char           pad0[4];
    int            refcount; /* +0x04 */
    char           pad8[0x1c - 8];
    unsigned short flags;    /* +0x1c */
    char           pad1e[0x28 - 0x1e];
    struct Sample* next;     /* +0x28 */
    SndObj*        drv;      /* +0x2c */
} Sample;

extern SndObj* g_snd_mgr;   /* 0x007cad40 */
extern void    DeletePlayableSamples(void* def);  /* 0x00492b90: SampleDef* filter, 0 = all (audio3.c) */
extern Sample* MakePlayable(void);                /* 0x00492110 */

