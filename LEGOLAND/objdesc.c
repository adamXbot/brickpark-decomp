/* LEGOLAND -- scope U: the object-description loader.
 *
 * One function, called once per free-play class by InitFreePlayLists
 * (fpui2.c): open "Objdesc\<image>" through RES, read the 0xd0-byte
 * description record and the length-prefixed strings that follow it, and
 * hand back the class's free-play icon sprite with its text, sort key and
 * parent element through the out pointers. Most of the file's fields are
 * read and dropped: only the record's key, the parent name, the theme name,
 * the icon file name and the description text are used.
 *
 * VC6 SP3 /O2 /Gy /Gd. Types describe only the fields used here; names are
 * ours (fpui2.c's for the function), offsets and addresses are load-bearing.
 * Verification and the file format: docs/lanes/scope-u.md.
 */

/* ---- types --------------------------------------------------------------- */
typedef struct Sprite Sprite;

/* An LLIDB element (legoland.h's LLElem); the type/flag word is read as a
 * byte here. */
typedef struct FPElem {
    char*         name;      /* +0x00 */
    char*         image;     /* +0x04  asset file name */
    unsigned char flags;     /* +0x08  0x10: may be a free-play parent */
    char          pad09[3];  /* +0x09 */
    void*         data;      /* +0x0c */
} FPElem;

/* The description record: its own size is the first dword of the file,
 * the rest of the record follows. */
typedef struct ObjDesc {
    int     size;            /* +0x00 */
    int     f04;             /* +0x04  first byte of the record body */
    char    pad08[0x1e];     /* +0x08 */
    short   key;             /* +0x26  sort key handed back to the caller */
    char    pad28[0x28];     /* +0x28 */
    void*   block1;          /* +0x50  two optional blocks, read and freed */
    void*   block2;          /* +0x54 */
    char    pad58[0x6c];     /* +0x58 */
    FPElem* elem;            /* +0xc4 */
    char    padc8[8];        /* +0xc8 */
} ObjDesc;                   /* 0xd0 */

/* ---- globals ------------------------------------------------------------- */
extern FPElem* g_fp_theme;        /* 0x007cb3bc  the free-play screen's theme element (fpui2.c) */
extern char    g_fp_icon_name[];  /* 0x007fdba0  the icon file name read from the description (first named here) */

/* ---- callees ------------------------------------------------------------- */
extern int   sprintf(char* buf, const char* fmt, ...);                      /* 0x0049e573 (CRT) */
extern void* RES_OpenFile(const char* path);                                /* 0x00489b60 */
extern int   RES_ReadFile(void* f, void* buf, int len);                     /* 0x00489cf0 */
extern void  RES_CloseFile(void* f);                                        /* 0x00489de0 */
extern void* HeapAlloc_w(unsigned int size);                                /* 0x0049e4ff (CRT malloc) */
extern void  HeapFree_w(void* p);                                           /* 0x0049e4d0 (CRT free) */
extern FPElem* ElemID(const char* name);                                    /* 0x0047b3f0 */
extern int   LLIDB_FindElement(const char* name, FPElem** out, unsigned int* idx); /* 0x0047b330 */
extern Sprite* LoadSprite(const char* name, int mode);                      /* 0x00497ab0 */
extern void* memset(void* p, int c, unsigned int n);
#pragma intrinsic(memset)

/* ========================================================================= */

/* The description file: a size dword, the record body, then length-prefixed
 * fields in this order -- two optional binary blocks, a string, the PARENT
 * element name, a string, the THEME element name, two strings, the ICON
 * file name, three strings, and the description TEXT. Every string except
 * the sixth is NUL-terminated after reading. The record is zeroed before
 * its null check (an original bug: memset of a NULL block). */
// FUNCTION: LEGOLAND 0x0047c7f0
Sprite* GetFreePlayItemInfo(FPElem* e, char** text, int* key, FPElem** parent)
{
    int      len;
    Sprite*  sprite;
    char     name[0x100];
    char     desc1[0x100];
    char     path[0x100];
    char     desc2[0x100];
    void*    f;
    ObjDesc* d;
    char*    p;
    FPElem*  pe;

    sprintf(path, "Objdesc\\%s", e->image);
    f = RES_OpenFile(path);
    if (f) {
    d = HeapAlloc_w(sizeof(ObjDesc));
    memset(d, 0, sizeof(ObjDesc));
    if (d == 0) {
        RES_CloseFile(f);
        return 0;
    }
    d->elem = e;
    e->data = 0;
    RES_ReadFile(f, &len, 4);
    RES_ReadFile(f, &d->f04, len - 4);
    RES_ReadFile(f, &len, 4);
    if (len) {
        d->block1 = HeapAlloc_w(len);
        RES_ReadFile(f, d->block1, len);
        HeapFree_w(d->block1);
    }
    d->block1 = 0;
    RES_ReadFile(f, &len, 4);
    if (len) {
        d->block2 = HeapAlloc_w(len);
        RES_ReadFile(f, d->block2, len);
        HeapFree_w(d->block2);
    } else {
        d->block2 = 0;
    }
    RES_ReadFile(f, &len, 4);
    RES_ReadFile(f, desc1, len);
    desc1[len] = 0;
    RES_ReadFile(f, &len, 4);
    RES_ReadFile(f, name, len);
    name[len] = 0;
    if (name[0]) {
        pe = ElemID(name);
        *parent = pe;
        if (!(pe->flags & 0x10))
            *parent = 0;
    } else {
        *parent = 0;
    }
    RES_ReadFile(f, &len, 4);
    RES_ReadFile(f, desc2, len);
    desc2[len] = 0;
    RES_ReadFile(f, &len, 4);
    RES_ReadFile(f, name, len);
    name[len] = 0;
    if (name[0]) {
    LLIDB_FindElement(name, &g_fp_theme, 0);
    RES_ReadFile(f, &len, 4);
    RES_ReadFile(f, name, len);
    RES_ReadFile(f, &len, 4);
    RES_ReadFile(f, name, len);
    name[len] = 0;
    RES_ReadFile(f, &len, 4);
    RES_ReadFile(f, g_fp_icon_name, len);
    g_fp_icon_name[len] = 0;
    if (g_fp_icon_name[0] == 0 || (sprite = LoadSprite(g_fp_icon_name, 4)) == 0)
        sprite = LoadSprite("InstituteIcon.lls", 4);
    RES_ReadFile(f, &len, 4);
    RES_ReadFile(f, name, len);
    name[len] = 0;
    RES_ReadFile(f, &len, 4);
    RES_ReadFile(f, name, len);
    name[len] = 0;
    RES_ReadFile(f, &len, 4);
    RES_ReadFile(f, name, len);
    name[len] = 0;
    RES_ReadFile(f, &len, 4);
    p = HeapAlloc_w(len + 1);
    RES_ReadFile(f, p, len);
    p[len] = 0;
    RES_CloseFile(f);
    *text = p;
    *key = d->key;
    HeapFree_w(d);
    return sprite;
    }
    RES_CloseFile(f);
    HeapFree_w(d);
    }
    return 0;
}
