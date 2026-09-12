/* LEGOLAND — LLIDB object-definition (.ODF) loader.
 *
 * The largest per-type parser: reads Objdesc\<image>, builds a 0xd0-byte ObjDef,
 * links it onto the global object-def list (g_odf_head), resolves the class's
 * sprite/icon/build-anim/child elements, walks the localized string records
 * (english kept, others skipped), then installs callbacks (standard, optional
 * DLL library, custom) and finalizes. Its full 625-instruction body matches. */
#include "legoland.h"

int   sprintf(char*, const char*, ...);
void* malloc(unsigned int);
void  free(void*);
void* memset(void*, int, unsigned int);
int   stricmp(const char*, const char*);
void* RES_OpenFile(const char*);
int   RES_ReadFile(void* file, void* buf, int len);
#ifndef LEGOLAND_PORTABLE
void  RES_CloseFile(void* file);
#else
extern int RES_CloseFile(void* file);
#endif
int   LLIDB_FindElement(char* name, LLElem** out_elem, unsigned int* out_idx);
void* LLIDB_LoadData(LLElem* elem);
void* LoadSprite(const char* name, int flag);
void  LLSPlay(void* frames, void* hdr);
#ifndef LEGOLAND_PORTABLE
void  LLSStop(void* lls);
#else
extern int LLSStop(void* lls);
#endif
void* GetSpriteForLayer(void* sprite, int layer);
void* GetLLSForSprite(void* sprite);
void  SetStandardCallbacks(void* obj);
int   LoadObjectLibrary(void* obj, char* name);
void  SetCustomCallbacks(LLElem* elem);
/* 0x00480aa0 is pathobj2.c's SetWaterWorksClassOrigins: the "finalize the
 * ObjDef" hook the ODF loader tails into is, in the shipped build, only the
 * Water Works origin patch. The declaration keeps this file's parameter types
 * (HANDOFF section 3: the extern's types are the caller's codegen lever) --
 * both are pointers, so the call is the same instruction either way. */
extern void  ObjDefFinalize(LLElem* elem, void* obj);        /* 0x00480aa0 */
/* 0x0047f870 is sysstubs.c's DebugPrintf, the empty varargs logger. The three
 * call sites below are diagnostics on ordinary data (a class with no sprite /
 * icon / build-anim name is legal and the fallback follows), NOT an error
 * path. The definition is `(const char*, ...)`; the fixed two-parameter
 * spelling here is what this translation unit was compiled with and pushes the
 * same two dwords, so it stays. */
#ifndef LEGOLAND_PORTABLE
extern void  ODFError(const char* fmt, char* name);          /* 0x0047f870 */
#else
/* wasm32: `DebugPrintf`'s body (sysstubs.c:158) is `(const char*, ...)`, so its
 * second wasm parameter is the ADDRESS of the varargs buffer, not the name
 * pointer. Both spellings are (i32, i32) -> void, so nothing in the link can
 * see the difference -- it is PORT-M20's Spider Ride defect one argument
 * shorter, and harmless today only because that body is empty. PORT-A11, gated
 * by portable/tools/variadic_sweep.py. */
extern void  ODFError(const char* fmt, ...);                 /* 0x0047f870 */
#endif

typedef struct Anim { void* frames; int count; char pad[0xc]; int type; } Anim; /* frames@0, count@4, type@0x14 */
typedef struct Spr  { char pad[8]; Anim* hdr; char pad2[4]; unsigned int flags; } Spr; /* hdr@8, flags@0x10 */

typedef struct ObjDef {
    struct ObjDef* next;      /* 0x00 */
    int            f4;        /* 0x04 record count / header dword0 */
    int            f8;        /* 0x08 */
    char           pad0c[0x1c - 0x0c];
    unsigned int   flags;     /* 0x1c */
    char           pad20[0x2a - 0x20];
    short          f2a;       /* 0x2a */
    char           pad2c[0x4c - 0x2c];
    int            f4c;       /* 0x4c */
    void*          f50;       /* 0x50 */
    void*          f54;       /* 0x54 */
    void*          f58;       /* 0x58 */
    void*          f5c;       /* 0x5c */
    void*          f60;       /* 0x60 */
    Spr*           f64;       /* 0x64 build anim sprite */
    void*          f68;       /* 0x68 icon sprite */
    Spr*           f6c;       /* 0x6c */
    void*          f70;       /* 0x70 */
    int            f74;       /* 0x74 */
    char*          f78;       /* 0x78 */
    char*          f7c;       /* 0x7c */
    char*          f80;       /* 0x80 */
    char           pad84[0xa4 - 0x84];
    void         (*fa4)(LLElem*); /* 0xa4 */
    char           padA8[0xc4 - 0xa8];
    LLElem*        elem;      /* 0xc4 */
    char           padC8[0xd0 - 0xc8];
} ObjDef;

extern ObjDef*       g_odf_head;   /* 0x669240 */
extern int           g_debug_heap_bytes;  /* 0x00813a10  memdb.c's live-heap counter */
/* The two FLC/FLI player settings the export table names, set around each
 * LoadSprite so the video player knows what the sprite it is handed is for. */
extern unsigned char NEWFLC_PauseType;  /* 0x0080ff78 */
extern int           NEWFLC_AutoPlay;   /* 0x0080ff74 */

// FUNCTION: LEGOLAND 0x0047bf70
void* LLIDB_LoadODFData(LLElem* elem)
{
    int     len;
    LLElem* found;
    int     count;
    char    name[256];
    char    dllname[256];
    char    fname[256];
    char    desc[256];
    int     i;
    void*   file;
    ObjDef* obj;
    Spr*    sprite;
    void*   sp;
    void*   lls;

    g_debug_heap_bytes = 0;
    sprintf(fname, "Objdesc\\%s", elem->image);
    file = RES_OpenFile(fname);
    if (file == 0)
        return 0;

    obj = (ObjDef*)malloc(0xd0);
    {
        int c = 0x34;
        int* p = (int*)obj;
        while (c--) *p++ = 0;
    }
    if (obj == 0)
        return 0;

    obj->elem = elem;
    elem->data = obj;
    RES_ReadFile(file, &len, 4);
    RES_ReadFile(file, &obj->f4, len - 4);
    obj->f4c = 0;
    obj->next = g_odf_head;
    g_odf_head = obj;
    count = obj->f4;
    obj->f4 = 0;
    obj->f8 = 0;

    RES_ReadFile(file, &len, 4);
    if (len != 0) {
        obj->f50 = malloc(len);
        RES_ReadFile(file, obj->f50, len);
        free(obj->f50);
    } else {
        obj->f50 = 0;
    }

    RES_ReadFile(file, &len, 4);
    if (len != 0) {
        obj->f54 = malloc(len);
        RES_ReadFile(file, obj->f54, len);
        free(obj->f54);
    } else {
        obj->f54 = 0;
    }

    RES_ReadFile(file, &len, 4);
    RES_ReadFile(file, desc, len);
    desc[len] = 0;
    RES_ReadFile(file, &len, 4);
    RES_ReadFile(file, name, len);
    name[len] = 0;
    if (name[0] != 0) {
        LLIDB_FindElement(name, &found, 0);
        obj->f58 = found;
    } else {
        obj->f58 = 0;
    }

    RES_ReadFile(file, &len, 4);
    RES_ReadFile(file, dllname, len);
    dllname[len] = 0;
    RES_ReadFile(file, &len, 4);
    RES_ReadFile(file, name, len);
    name[len] = 0;
    if (name[0] != 0) {
        LLIDB_FindElement(name, &found, 0);
        obj->f5c = found;
    } else {
        obj->f5c = 0;
    }

    RES_ReadFile(file, &len, 4);
    RES_ReadFile(file, name, len);
    name[len] = 0;
    if (name[0] != 0) {
        LLIDB_FindElement(name, &found, 0);
        obj->f60 = found;
    } else {
        obj->f60 = 0;
    }

    NEWFLC_PauseType = 1;
    NEWFLC_AutoPlay = 1;
    RES_ReadFile(file, &len, 4);
    RES_ReadFile(file, name, len);
    name[len] = 0;
    if (obj->flags & 0x40000) {
        obj->f64 = 0;
    } else if (name[0] != 0) {
        sprite = (Spr*)LoadSprite(name, 1);
        obj->f64 = sprite;
        if (sprite != 0 && sprite->hdr != 0) {
            if (!(sprite->flags & 0x8000) &&
                (sprite->hdr->type == 2 || sprite->hdr->type == 3))
                LLSPlay(sprite->hdr->frames, sprite->hdr);
            obj->flags |= 4;
        }
    } else {
        ODFError("Class %s has no sprite name.", obj->elem->name);
        obj->f64 = 0;
    }

    NEWFLC_PauseType = 2;
    NEWFLC_AutoPlay = 0;
    RES_ReadFile(file, &len, 4);
    RES_ReadFile(file, name, len);
    name[len] = 0;
    if (name[0] != 0) {
        obj->f68 = LoadSprite(name, 4);
    } else {
        ODFError("Class %s has no icon name.", obj->elem->name);
        obj->f68 = 0;
    }
    if (obj->f68 == 0)
        obj->f68 = LoadSprite("InstituteIcon.lls", 4);

    RES_ReadFile(file, &len, 4);
    RES_ReadFile(file, name, len);
    name[len] = 0;
    if (name[0] != 0) {
        sprite = (Spr*)LoadSprite(name, 1);
        obj->f6c = sprite;
        if (sprite != 0 && (sprite->flags & 0x8000)) {
            for (i = 0; i < obj->f6c->hdr->count; i++) {
                sp = GetSpriteForLayer(obj->f6c, i);
                if (sp != 0) {
                    lls = GetLLSForSprite(sp);
                    if (lls != 0)
                        LLSStop(lls);
                }
            }
        }
    } else {
        ODFError("Class %s has no build anim name.", obj->elem->name);
        obj->f6c = 0;
    }

    RES_ReadFile(file, &len, 4);
    RES_ReadFile(file, name, len);
    name[len] = 0;
    obj->f70 = 0;
    if (name[0] != 0 && !(obj->flags & 0x40000)) {
        LLIDB_FindElement(name, &found, 0);
        obj->f70 = found;
        LLIDB_LoadData(found);
    }

    for (i = 0; i < count; i++) {
        RES_ReadFile(file, &len, 4);
        RES_ReadFile(file, name, len);
        name[len] = 0;
        if (i != 0 && stricmp("english", name) != 0) {
            RES_ReadFile(file, &len, 4);
            RES_ReadFile(file, name, len);
            RES_ReadFile(file, &len, 4);
            RES_ReadFile(file, name, len);
            RES_ReadFile(file, &len, 4);
            RES_ReadFile(file, name, len);
        } else {
            if (obj->f78 != 0)
                free(obj->f78);
            RES_ReadFile(file, &len, 4);
            obj->f78 = (char*)malloc(len + 1);
            RES_ReadFile(file, obj->f78, len);
            obj->f78[len] = 0;
            if (obj->f7c != 0)
                free(obj->f7c);
            RES_ReadFile(file, &len, 4);
            obj->f7c = (char*)malloc(len + 1);
            RES_ReadFile(file, obj->f7c, len);
            obj->f7c[len] = 0;
            if (obj->f80 != 0)
                free(obj->f80);
            RES_ReadFile(file, &len, 4);
            obj->f80 = (char*)malloc(len + 1);
            RES_ReadFile(file, obj->f80, len);
            obj->f80[len] = 0;
        }
    }

    obj->f74 = 0;
    RES_CloseFile(file);
    SetStandardCallbacks(obj);
    if ((obj->flags & 0x10000) && dllname[0] != 0) {
        if (LoadObjectLibrary(obj, dllname) == 0) {
            obj->flags &= 0xfffeffff;
            SetCustomCallbacks(obj->elem);
            if (obj->fa4 != 0)
                obj->fa4(obj->elem);
            ODFError("Class %s has OC_USEDLL attribute and DLL failed to load.", obj->elem->name);
        }
    } else {
        SetCustomCallbacks(obj->elem);
    }

    if (obj->flags & 0x40000) {
        if (obj->f50 != 0) {
            free(obj->f50);
            obj->f50 = 0;
        }
        if (obj->f54 != 0) {
            free(obj->f54);
            obj->f54 = 0;
        }
    }

    if (obj->f2a < 1)
        obj->f2a = 1;

    ObjDefFinalize(elem, obj);
    return obj;
}
