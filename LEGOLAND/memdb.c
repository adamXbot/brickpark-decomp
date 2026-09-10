/* LEGOLAND — debug heap (tagged allocations) and LLIDB element management.
 *
 * The __DEBUG_* family is the tagged-allocation layer beneath the MemAlloc /
 * HeapAlloc_w wrappers: every block carries a 16-byte header in front of the
 * user pointer, and a running byte total lives in g_debug_heap_bytes
 * (0x00813a10 — the same word LLIDB_LoadODFData zeroes).
 *
 *   header[0..10]   last 11 characters of the tag string (right-aligned:
 *                   a long tag keeps its TAIL), unterminated
 *   header[11..15]  sprintf("%d", line) — up to four digits + NUL
 *   header + 0x10   the pointer handed back to the caller
 *
 * __DEBUG_SMALLOC ("string malloc") spends all 16 bytes on the tag and has no
 * line number; __DEBUG_TAG drops an orphan 16-byte marker "*-->" + 12 tag
 * characters into the heap as a landmark for heap dumps and never frees it.
 * __DEBUG_FREE / __DEBUG_REALLOC recover the header from the user pointer and
 * account the byte total with the CRT's _msize(). */
#include "legoland.h"

void*        malloc(unsigned int);                       /* 0x0049e4ff */
void         free(void*);                                /* 0x0049e4d0 */
void*        realloc(void*, unsigned int);               /* 0x0049fca2 */
unsigned int _msize(void*);                              /* 0x0049fdc2 */
int          sprintf(char*, const char*, ...);           /* 0x0049e573 */
unsigned int strlen(const char*);
void*        memset(void*, int, unsigned int);
#pragma intrinsic(strlen, memset)

extern int g_debug_heap_bytes;   /* 0x00813a10  live bytes (headers excluded) */

/* -------------------------------------------------------------- debug heap */

// FUNCTION: LEGOLAND 0x00453bf0
void __DEBUG_FREE(void* p)
{
    char* blk = (char*)p - 0x10;
    g_debug_heap_bytes += 0x10 - _msize(blk);
    free(blk);
}

// FUNCTION: LEGOLAND 0x00453b70
void* __DEBUG_REALLOC(void* p, unsigned int size)
{
    char* blk = (char*)p - 0x10;
    int   old = _msize(blk);
    g_debug_heap_bytes += size - old + 0x10;
    return (char*)realloc(blk, size + 0x10) + 0x10;
}

void* __DEBUG_MALLOC(char* tag, int line, unsigned int size);

// FUNCTION: LEGOLAND 0x00453bb0
void* __DEBUG_CALLOC(char* tag, int line, unsigned int n, unsigned int elem)
{
    unsigned int total = n * elem;
    void* p = __DEBUG_MALLOC(tag, line, total);
    memset(p, 0, total);
    return p;
}

// FUNCTION: LEGOLAND 0x00453a30
void __DEBUG_TAG(char* tag)
{
    char* blk = (char*)malloc(0x10);
    int   len = strlen(tag);
    int   n;
    int   i;

    /* Single-assignment arms: VC6 hoists the constant arm ahead of the
     * compare and keeps `cmp ecx,0Ch` immediate; `n = 12; if (len < 12)`
     * CSEs the 12 into esi (`cmp ecx,esi`) and a ternary flips the branch. */
    if (len >= 12)
        n = 12;
    else
        n = len;
    blk[0] = '*';
    blk[1] = '-';
    blk[2] = '-';
    blk[3] = '>';
    for (i = 0; i < n; i++)
        blk[4 + i] = tag[i];
}

// FUNCTION: LEGOLAND 0x00453b00
void* __DEBUG_SMALLOC(char* tag, unsigned int size)
{
    char* blk = (char*)malloc(size + 0x10);
    int   len = strlen(tag);
    int   n, start, i;

    g_debug_heap_bytes += size;
    if (len >= 16) {
        n = 16;
        start = len - 16;
    } else {
        n = len;
        start = 0;
    }
    for (i = 0; i < n; i++)
        blk[i] = tag[start + i];
    return blk + 0x10;
}

// FUNCTION: LEGOLAND 0x00453a80
void* __DEBUG_MALLOC(char* tag, int line, unsigned int size)
{
    char* blk = (char*)malloc(size + 0x10);
    int   len = strlen(tag);
    int   n, start, i;

    g_debug_heap_bytes += size;
    if (len >= 11) {
        n = 11;
        start = len - 11;
    } else {
        n = len;
        start = 0;
    }
    for (i = 0; i < n; i++)
        blk[i] = tag[start + i];
    sprintf(blk + 11, "%d", line);
    return blk + 0x10;
}

/* ------------------------------------------------------------------ LLIDB */

int  LLIDB_RegisterNewElement(char* name, char* image, unsigned int type);  /* 0x0047b610 */
int  LLIDB_GetElement(unsigned int idx, LLElem** out);                      /* 0x0047b2e0 */
#ifndef LEGOLAND_PORTABLE
extern int LLIDB_LoadDataByIndex(int idx);                                  /* 0x0047b7b0 (internal) */
#else
extern void LLIDB_LoadDataByIndex(int idx);                                  /* 0x0047b7b0 (internal) */
#endif

/* Register + immediately load. Index 0 is treated as failure. */
// FUNCTION: LEGOLAND 0x0047b860
int LLIDB_RegisterNewElementB(char* name, char* image, unsigned int type)
{
    int idx = LLIDB_RegisterNewElement(name, image, type);
    if (idx)
        LLIDB_LoadDataByIndex(idx);
    return idx;
}

/* Drop the "loaded as a class on this level" bit (0x4) from every element. */
// FUNCTION: LEGOLAND 0x0047b4c0
void LLIDB_ClearOnLevel(void)
{
    unsigned int i;
    for (i = 0; i < g_llidb_count; i++)
        g_llidb_pages[i >> 8][i & 0xff].type_flags &= ~4;
}

__declspec(dllimport) void* __stdcall GetDesktopWindow(void);                                   /* [0x4ab2d8] */
__declspec(dllimport) int   __stdcall DialogBoxParamA(void* inst, char* tmpl, void* parent,
                                                      void* proc, long lparam);                 /* [0x4ab304] */
void* WNDENV_GethInstance(void);                                                                /* 0x0047fe40 */
extern int __stdcall LLIDB_SelectDlgProc(void* dlg, unsigned int msg, unsigned int wp, long lp); /* 0x0047b890 */

extern int          g_llidb_select_filter;   /* 0x007fdb88  passed to the dialog */
extern unsigned int g_llidb_select_index;    /* 0x007fdca0  the dialog's pick */

/* Modal element picker (dialog template 0x75). 1 = OK: hand back the chosen
 * element; -1 = dialog failed; anything else (cancel) = -6. */
// FUNCTION: LEGOLAND 0x0047bc20
int LLIDB_SelectElement(int filter, LLElem** out)
{
    int r;

    g_llidb_select_filter = filter;
    r = DialogBoxParamA(WNDENV_GethInstance(), (char*)0x75, GetDesktopWindow(),
                        LLIDB_SelectDlgProc, 0);
    switch (r) {
    case 1:
        if (out)
            LLIDB_GetElement(g_llidb_select_index, out);
        return 0;
    case -1:
        return -1;
    default:
        return -6;
    }
}

/* .ILF/.CSP descriptor as LEGOLAND/llidb_load.c builds it (36 bytes). */
typedef struct IlfData {
    int    f00;       /* +0x00 */
    int    count;     /* +0x04 */
    void** sprites;   /* +0x08 */
    int*   dx;        /* +0x0c */
    int*   dy;        /* +0x10 */
    int    f14, f18, f1c, f20;
} IlfData;

int KillSprite(void* s);   /* 0x00497bd0 (UnreferenceSprite) */

/* Declared int-returning (it never sets eax) so LLIDB_UnLoadData's discarded
 * tail call cleans up with `pop ecx`; see audio3.c FreePlayableSample. */
// FUNCTION: LEGOLAND 0x0047bef0
int LLIDB_FreeILFTable(IlfData* t)
{
    if (t) {
        if (t->dx)
            free(t->dx);
        if (t->dy)
            free(t->dy);
        if (t->sprites) {
            int i;
            for (i = 0; i < t->count; i++) {
                if (t->sprites[i]) {
                    KillSprite(t->sprites[i]);
                    t->sprites[i] = 0;
                }
            }
            free(t->sprites);
        }
        free(t);
    }
}

int LLIDB_UnLoadODFData(LLElem* e);   /* 0x0047cdd0 */
#ifndef LEGOLAND_PORTABLE
int LLIDB_UnLoadTSMData(LLElem* e);   /* 0x0047cf80 */
#else
extern void LLIDB_UnLoadTSMData(LLElem* e);   /* 0x0047cf80 */
#endif
int LLIDB_UnLoadLLSData(LLElem* e);   /* 0x0047c6a0 */

/* Drop a reference; on the last one clear loaded/on-level bits (0x1 and
 * 0x30000) and free the parsed data by type. */
// FUNCTION: LEGOLAND 0x0047d450
int LLIDB_UnLoadData(LLElem* e)
{
    if (e->refcount) {
        e->refcount--;
        if (e->refcount == 0) {
            if (e->type_flags & 1) {
                e->type_flags &= 0xfffcfff0;
                switch (e->type_flags & 0xfff0) {
                case 0x40:
                    return LLIDB_UnLoadODFData(e);
                case 0x10:
                case 0x1010:
                    return LLIDB_UnLoadLLSData(e);
                case 0x20:
#ifndef LEGOLAND_PORTABLE
                    return LLIDB_UnLoadTSMData(e);
#else
                    /* savemisc2.c defines this one `void` because 0x0047cf80
                     * really does fall off its end, so the original's `jmp`
                     * here propagates whatever EAX happens to hold -- and so
                     * does this function, which also falls off its end.  The
                     * portable arm drops the value instead of inventing one. */
                    LLIDB_UnLoadTSMData(e);
                    break;
#endif
                case 0x400:
                    LLIDB_FreeILFTable((IlfData*)e->data);
                    break;
                }
            }
        }
    }
}

/* Tear the whole database down: every element's name/image strings, then
 * each 256-element page, then the page table. */
// FUNCTION: LEGOLAND 0x0047be00
int LLIDB_CloseICM(void)
{
    unsigned int page;
    unsigned int remaining = g_llidb_count;
    unsigned int n, i;

    /* `remaining -= 0x100` must sit in the increment clause (after page++):
     * as the last body statement VC6 schedules the `sub ebp` ahead of the
     * `shr esi,8` of the loop test. The if/else (not a ternary) gives the
     * `jb` with the 0x100 arm laid out first. */
    for (page = 0; page < g_llidb_capacity >> 8; page++, remaining -= 0x100) {
        if (remaining >= 0x100)
            n = 0x100;
        else
            n = remaining;
        for (i = 0; i < n; i++) {
            if (g_llidb_pages[page][i].name)
                free(g_llidb_pages[page][i].name);
            if (g_llidb_pages[page][i].image)
                free(g_llidb_pages[page][i].image);
        }
    }
    for (page = 0; page < g_llidb_capacity >> 8; page++)
        free(g_llidb_pages[page]);
    free(g_llidb_pages);
    return 0;
}

/* --------------------------------------------------------------------- RES */

typedef struct RVol {
    char          pad0[0x1c];
    int           handle;   /* +0x1c */
    struct RFile* cur;      /* +0x20 */
    int           refcount; /* +0x24 */
} RVol;

typedef struct RFile {
    int   size;   /* +0x00 */
    int   base;   /* +0x04 */
    RVol* vol;    /* +0x08 */
    int   pos;    /* +0x0c */
} RFile;

__declspec(dllimport) int __stdcall RES_LowSeek(int h, int off, int a, int b);   /* [0x4ab104] SetFilePointer */

// FUNCTION: LEGOLAND 0x00489d70
int RES_SetFilePointer(RFile* f, int pos)
{
    RVol* vol;

    if (f && pos >= 0 && pos < f->size) {
        vol = f->vol;
        f->pos = pos;
        vol->cur = f;
        RES_LowSeek(vol->handle, f->base + f->pos, 0, 0);
        return pos;
    }
    return -1;
}

/* ------------------------------------------------------- object libraries */

/* A loaded object DLL, shared between classes and refcounted. */
typedef struct ObjLib {
    struct ObjLib* next;       /* +0x00 */
    void*          hmodule;    /* +0x04 */
    int            refcount;   /* +0x08 */
} ObjLib;

typedef struct ObjDefL {
    char    pad[0x88];
    ObjLib* lib;               /* +0x88 */
} ObjDefL;

__declspec(dllimport) int __stdcall FreeLibrary(void* h);   /* [0x4ab128] */
extern ObjLib* g_objlib_head;                               /* 0x00669244 */

// FUNCTION: LEGOLAND 0x004810f0
void UnLoadObjectLibrary(ObjDefL* obj)
{
    ObjLib* p;

    if (obj->lib->refcount == 0)
        return;
    FreeLibrary(obj->lib->hmodule);
    obj->lib->refcount--;
    if (obj->lib->refcount == 0) {
        p = g_objlib_head;
        if (p == obj->lib) {
            g_objlib_head = p->next;
            free(p);
        } else {
            while (p && p->next != obj->lib)
                p = p->next;
            if (p) {
                p->next = p->next->next;
                free(obj->lib);
            }
        }
    }
}
