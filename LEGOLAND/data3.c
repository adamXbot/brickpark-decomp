/* LEGOLAND — locale ".loc" model sets, their textures, and the alt-texture
 * name lists.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only the
 * struct field offsets, callee argument counts and global addresses are
 * load-bearing; names are ours. Types are defined LOCALLY on purpose (the
 * shared header is owned elsewhere).
 *
 * data2.c holds the matched siblings (InitMan, which calls every function in
 * this file) and the loc/texture tables; blokeai.c calls LookupTextureName.
 *
 * Everything here lives under ".\3ddata\new\<dir>\": one ".loc" skeleton set
 * per character kind, a ".txt" texture-name list per sex, and the textures
 * themselves.
 */
#include <string.h>
#include "legoland.h"

#pragma intrinsic(strlen)

/* ---- CRT ---------------------------------------------------------------- */
extern void* MemAlloc(unsigned int size);             /* 0x0049e4ff (malloc) */
extern int   sprintf(char* buf, const char* fmt, ...); /* 0x0049e573 */

/* ---- RES ---------------------------------------------------------------- */
extern void* RES_OpenFile(const char* path);          /* 0x00489b60 */
extern int   RES_GetFileSize(void* f);                /* 0x00489ce0 */
extern int   RES_ReadFile(void* f, void* buf, int n); /* 0x00489cf0 */
#ifndef LEGOLAND_PORTABLE
extern void  RES_CloseFile(void* f);                  /* 0x00489de0 */
#else
extern int RES_CloseFile(void* f);                  /* 0x00489de0 */
#endif

/* ".\\3ddata\\new\\%s\\%s" — every path in this file is built with it. */
extern const char kPath3dData[];      /* 0x004b7b10 */

/* A loaded ".loc" model/skeleton set. Only the fields this file touches are
 * named; +0x04 is the model context data2.c's InitMan writes after the load.
 * +0x2c and +0x30 are FILE-RELATIVE offsets on disk, turned into absolute
 * pointers by 0x0043f970 (an 8-instruction fix-up, first named here).
 *
 * `name` is the texture-file stem: LoadLocTextures builds
 * "<dir>\<name>%04d.BMP" from it. Its length is not directly observable —
 * 0x20 is the gap between +0x0c and the first field the fix-up touches. */
typedef struct LocSet {
    int          count;    /* +0x00  number of textures in the set */
    void*        ctx;      /* +0x04  model context (written by InitMan) */
    int          pad08;    /* +0x08 */
    char         name[0x20]; /* +0x0c */
    char*        rel2c;    /* +0x2c  offset -> pointer */
    char*        rel30;    /* +0x30  offset -> pointer */
} LocSet;

extern void FixUpLocSetPointers(LocSet* set);   /* 0x0043f970 */

/* A decoded source bitmap: 16-bit pixel dimensions at +0x08 / +0x0a. */
typedef struct SrcImage {
    int   pad00[2];  /* +0x00 */
    short w;         /* +0x08 */
    short h;         /* +0x0a */
} SrcImage;

/* Every loaded texture's pixel size, indexed by absolute texture id (the same
 * table anim2.c reads). */
typedef struct TexSize { int w; int h; } TexSize;
extern TexSize g_texsize[];           /* 0x0081c0c0 */

/* The running texture-id cursor. data2.c reads it through GetModelContext
 * (0x00443710, a one-instruction load of this global); LoadLocTextures both
 * reads it and advances it once per texture. */
extern int   g_model_ctx;             /* 0x00665e8c */

/* 0x004436d0: CreateSourceImage(path, fmt) plus the 0x004434d0 conversion,
 * KillImage-ing and returning 0 if the conversion fails. First named here. */
extern SrcImage* LoadTextureImage(const char* path, int fmt);   /* 0x004436d0 */

/* 0x00488670: malloc a 0x2c-byte texture record, fill it from the image
 * (0x004437d0) and file it in g_textures[slot] (0x00798190, 256 slots).
 * First named here. */
extern void  RegisterTextureImage(SrcImage* img, int slot);     /* 0x00488670 */

#ifndef LEGOLAND_PORTABLE
extern void  KillImage(SrcImage* img);          /* 0x00497510 */
#else
extern int KillImage(SrcImage* img);          /* 0x00497510 */
#endif
extern void  DBPrintf(const char* fmt, ...);    /* 0x00453a20 */

extern const char kTexPathFmt[];   /* 0x004b7d58 "%s\\%s%04d.BMP" */
extern const char kTexFailed[];    /* 0x004b7d3c "Failed to load texture %s" */

/* Slurp "<dir>\<file>" out of the resource system into a fresh malloc block.
 *
 * ORIGINAL BUG, reproduced: when the file cannot be opened the function
 * returns the UNINITIALISED `text` local (the original reads its frame home
 * at [esp+0xc] and returns it). A second, smaller bug: when the allocation
 * fails the RES handle is leaked, because RES_CloseFile sits inside the
 * `if (text)`.
 */
// FUNCTION: LEGOLAND 0x004402d0
char* LoadTextFile(const char* dir, const char* file)
{
    char* text;
    char  path[256];
    void* f;
    int   len;

    sprintf(path, kPath3dData, dir, file);

    f = RES_OpenFile(path);
    if (f) {
        len = RES_GetFileSize(f);
        text = (char*)MemAlloc(len);
        if (text) {
            RES_ReadFile(f, text, len);
            RES_CloseFile(f);
        }
    }
    return text;   /* uninitialised when the open failed — see note above */
}

/* Read "<dir>\<file>.loc" whole, relocate its two internal offsets, and hand
 * back the block. Returns 0 if the file is missing OR the allocation fails —
 * and in the latter case the RES handle is leaked, as in LoadTextFile. */
// FUNCTION: LEGOLAND 0x0043f990
LocSet* LoadLocSet(const char* file, const char* dir)
{
    char    path[256];
    void*   f;
    LocSet* set;
    int     len;

    sprintf(path, kPath3dData, dir, file);

    f = RES_OpenFile(path);
    if (f) {
        len = RES_GetFileSize(f);
        set = (LocSet*)MemAlloc(len);
        if (set) {
            RES_ReadFile(f, set, len);
            RES_CloseFile(f);
            FixUpLocSetPointers(set);
            return set;
        }
    }
    return 0;
}

/* Load every texture of a ".loc" set as "<dir>\<name>%04d.BMP", register it
 * under the running texture id, record its pixel size and drop the source
 * image again. The texture id advances even when a texture fails to load, so
 * the ids stay in step with the set's own numbering.
 *
 * `set` is re-read from its argument slot in the latch because ebp carries
 * &set->name across the whole loop — that is the shape, not a spill. */
// FUNCTION: LEGOLAND 0x00443720
void LoadLocTextures(LocSet* set, const char* dir)
{
    char      path[64];
    int       i;
    SrcImage* img;

    for (i = 0; i < set->count; i++) {
        sprintf(path, kTexPathFmt, dir, set->name, i);
        img = LoadTextureImage(path, 9);
        if (img == 0) {
            DBPrintf(kTexFailed, path);
        } else {
            RegisterTextureImage(img, g_model_ctx);
            g_texsize[g_model_ctx].w = img->w;
            g_texsize[g_model_ctx].h = img->h;
            KillImage(img);
        }
        g_model_ctx++;
    }
}

/* ------------------------------------------------------------------------ *
 *  The packed texture-name lists                                            *
 * ------------------------------------------------------------------------ *
 *
 * LoadTextFile slurps "altman.txt" / "altwoman.txt" whole; the file is a
 * packed list of NUL-terminated strings in two halves:
 *
 *     <title>\0  <u32 n>  <face name>\0 ... \0  ""\0
 *     <title>\0  <u32>    <chest name>\0 ...
 *
 * i.e. a title string, a 4-byte count, then the entries, the FACE half
 * terminated by an empty string. LookupTextureName returns the `index`'th
 * entry of the face half (kind != 1) or of the chest half (kind == 1).
 * blokeai.c's GetFaceTextureNameOfBloke / GetChestTextureNameOfBloke are the
 * only callers.
 */

/* 0x004428c0: step `n` NUL-terminated strings forward. First named here. */
extern char* SkipStrings(char* p, int n);   /* 0x004428c0 */

/* ORIGINAL BUGS, reproduced (both are uninitialised reads that VC6 resolves
 * to a stack home, so they are visible in the machine code):
 *
 *   - `list == 0` leaves `face` unset. VC6 homes it in the `index` argument
 *     slot, so the call becomes SkipStrings((char*)index, index).
 *   - a zero count leaves `chest` unset. VC6 homes it in the `list` argument
 *     slot, which is dead by then and still holds the caller's pointer, so
 *     the chest lookup silently walks from the head of the file.
 *
 * The `switch` is load-bearing: `if (kind == 1)` compares the argument slot
 * in place (`cmp dword ptr [esp+0x10],1`), where the original loads it and
 * tests with `dec eax` — the one-case switch is what emits the load+dec.
 */
// FUNCTION: LEGOLAND 0x004428f0
char* LookupTextureName(char* list, int kind, int index)
{
    char* face;
    char* chest;
    char* q;
    int   n;

    if (list) {
        q = list + strlen(list) + 1;   /* past the title */
        n = *(int*)q;
        q += 4;
        face = q;
        if (n != 0) {
            do {
                q += strlen(q) + 1;
            } while (strlen(q) != 0);   /* stop on the empty terminator */
            q++;
            chest = q + strlen(q) + 1 + 4;   /* past the second title+count */
        }
    }

    switch (kind) {
    case 1:
        return SkipStrings(chest, index);
    default:
        return SkipStrings(face, index);
    }
}
