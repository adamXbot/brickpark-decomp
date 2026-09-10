/* LEGOLAND — object-library (DLL) loader, position tables, visitor names and
 * a handful of small helpers.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field OFFSETS, callee argument counts and global addresses are load-bearing;
 * type and field names are ours. Types are defined LOCALLY on purpose
 * (legoland.h is owned elsewhere).
 *
 * The object-library mechanism (LoadObjectLibrary / GetInterface):
 *
 *   An object class whose .ODF carries OC_USEDLL (0x10000) names a DLL. The
 *   loader builds ".\dlls\<name>.dll" from the ODF's dll field, points the
 *   global "current library" pointer at a scratch 16-byte record and calls
 *   LoadLibraryExA. The DLL's start-up code registers its GetInterfaces entry
 *   into that record (+0x0c) through the game's export table; the loader then
 *   either finds an already-loaded record with the same module handle (and
 *   bumps its refcount) or copies the scratch record into a fresh node at the
 *   head of the library list. Finally it calls the library's GetInterfaces
 *   with (elem, &table) and copies the 14-slot table into the class's
 *   callback slots at +0x8c..+0xc0 (slot 7 is an init callback run once).
 *
 *   GetInterface (0x0041b150) is the game-side twin of such a DLL entry: the
 *   BOATING SCHOOL classes are built in, so their GetInterfaces lives in the
 *   exe and fills the same 14-slot table. */
#include "legoland.h"

/* ---- CRT ----------------------------------------------------------------- */
void* memset(void*, int, unsigned int);
char* strcpy(char*, const char*);
char* strcat(char*, const char*);
#pragma intrinsic(memset, strcpy, strcat)
extern int  rand(void);                                        /* 0x0049e4b2 */
extern void _splitpath(const char* path, char* drive, char* dir,
                       char* fname, char* ext);                /* 0x0049ec85 */
#ifndef LEGOLAND_PORTABLE
extern void Format(char* dest, const char* fmt, ...);          /* 0x0049e573 (sprintf) */
#else
extern int Format(char* dest, const char* fmt, ...);          /* 0x0049e573 (sprintf) */
#endif
extern int  NameCompare(const char* a, const char* b);         /* 0x004aab90 (_stricmp) */

/* ---- Win32 ----------------------------------------------------------------
 * Imports must be __declspec(dllimport) so the call comes out as
 * `call dword ptr [__imp__X]`. */
__declspec(dllimport) void* __stdcall LoadLibraryExA(const char* name, void* file,
                                                     unsigned long flags);   /* [0x4ab0fc] */

/* ---- game callees ---------------------------------------------------------- */
extern void* HeapAlloc_w(unsigned int size);                   /* 0x0049e4ff */
extern void* RES_OpenFile(const char* name);                   /* 0x00489b60 */
extern int   RES_ReadFile(void* file, void* buf, int len);     /* 0x00489cf0 */
#ifndef LEGOLAND_PORTABLE
extern void  RES_CloseFile(void* file);                        /* 0x00489de0 */
#else
extern int RES_CloseFile(void* file);                        /* 0x00489de0 */
#endif
extern int   SaveGameWrite(const void* buf, unsigned int n);   /* 0x0047d760 */
extern void  AddBricks(int n);                                 /* 0x004578a0 */
extern void  LLSPlay(void* anim, void* owner);                 /* 0x0047d520 */

/* ========================================================================== */
/* BuyItem                                                                    */
/* ========================================================================== */

/* A placed map object: its class record sits at +0x0c; the class's brick
 * cost is a SIGNED 16-bit field at +0x28 of the class (movsx). */
typedef struct CostClass {
    char  pad0[0x28];
    short cost;             /* +0x28 */
} CostClass;

typedef struct CostObj {
    char       pad0[0x0c];
    CostClass* cls;         /* +0x0c */
} CostObj;

/* The packed map square the money SFX is sourced from (money.c MapPoint). */
typedef struct MapPoint {
    unsigned char x;
    unsigned char y;
} MapPoint;

extern void PlayMoneySFX(MapPoint* at, int which, int flags);  /* 0x00453950 */

/* Credit the class's brick value (a purchase debits by handing AddBricks a
 * negative cost) and, for a non-negative effect index, ring the money sound
 * at the object's square. A zero-cost class does nothing at all. */
// FUNCTION: LEGOLAND 0x004539e0
void BuyItem(CostObj* obj, MapPoint* at, int which)
{
    int cost = obj->cls->cost;

    if (cost != 0) {
        if (which >= 0)
            PlayMoneySFX(at, which, 0);
        AddBricks(cost);
    }
}

/* ========================================================================== */
/* DefaultCursor                                                              */
/* ========================================================================== */

/* The edit cursor block (0x007febc0, 0x1834 bytes). Only the three fields
 * DefaultCursor preserves/sets are named; +0x1404/+0x1408 survive the wipe. */
typedef struct EditCursor {
    char  pad0[0x1404];
    void* keep1404;         /* +0x1404 preserved across the reset */
    void* keep1408;         /* +0x1408 preserved across the reset */
    char  pad140c[0x1828 - 0x140c];
    int   f1828;            /* +0x1828 = 0xc00 */
    char  pad182c[0x1834 - 0x182c];
} EditCursor;

extern void ResetCursorFootprint(EditCursor* c);               /* 0x0045f460 */

/* Wipe the cursor block back to its defaults, keeping the two pointer fields
 * at +0x1404/+0x1408, then rebuild its footprint. */
// FUNCTION: LEGOLAND 0x0045a390
void DefaultCursor(EditCursor* c)
{
    void* a = c->keep1404;
    void* b = c->keep1408;

    memset(c, 0, sizeof(EditCursor));
    c->keep1404 = a;
    c->f1828 = 0xc00;
    c->keep1408 = b;
    ResetCursorFootprint(c);
}

/* ========================================================================== */
/* Interface tables                                                           */
/* ========================================================================== */

/* The 14-slot callback table a library's GetInterfaces fills. LoadObjectLibrary
 * copies slot i into the class record as noted; slot 7 is not stored but
 * called once with the class's LLIDB element. */
typedef struct IfaceTable {
    void* cb_8c;             /* [0]  -> ObjDef +0x8c */
    void* cb_90;             /* [1]  -> +0x90 */
    void* cb_94;             /* [2]  -> +0x94 */
    void* cb_98;             /* [3]  -> +0x98 (place/add) */
    void* cb_9c;             /* [4]  -> +0x9c (remove) */
    void* cb_a0;             /* [5]  -> +0xa0 */
    void* cb_a8;             /* [6]  -> +0xa8 */
    void (*init)(LLElem* e); /* [7]  called after the table is installed */
    void* cb_ac;             /* [8]  -> +0xac */
    void* cb_b0;             /* [9]  -> +0xb0 */
    void* cb_b4;             /* [10] -> +0xb4 (always stored) */
    void* cb_b8;             /* [11] -> +0xb8 (always stored; load) */
    void* cb_bc;             /* [12] -> +0xbc (always stored; save) */
    void* cb_c0;             /* [13] -> +0xc0 */
} IfaceTable;

/* The BOATING SCHOOL family's built-in GetInterfaces. The callbacks are
 * referenced by address only (their bodies are other lanes'). */
// FUNCTION: LEGOLAND 0x0041b150
void GetInterface(LLElem* elem, IfaceTable* t)
{
    if (NameCompare("BOATING SCHOOL WATER", elem->name) == 0) {
        t->init  = (void (*)(LLElem*))0x41b830;
        t->cb_8c = (void*)0x41b880;
        t->cb_90 = (void*)0x41bd40;
        t->cb_94 = (void*)0x41bfb0;
        t->cb_98 = (void*)0x41b8e0;
        t->cb_9c = (void*)0x41c130;
        return;
    }
    if (NameCompare("BOATING SCHOOL", elem->name) == 0) {
        t->init  = (void (*)(LLElem*))0x419d10;
        t->cb_ac = (void*)0x419ef0;
        t->cb_8c = (void*)0x41a000;
        t->cb_90 = (void*)0x41a2f0;
        t->cb_94 = (void*)0x41a3d0;
        t->cb_98 = (void*)0x41a040;
        t->cb_9c = (void*)0x41a530;
        t->cb_a8 = (void*)0x41a720;
        t->cb_b0 = (void*)0x41abd0;
        t->cb_bc = (void*)0x41acf0;
        t->cb_b8 = (void*)0x41aee0;
        t->cb_c0 = (void*)0x41b100;
        return;
    }
    if (NameCompare("BOATING SCHOOL MERMAID", elem->name) == 0) {
        t->init  = (void (*)(LLElem*))0x41b250;
        t->cb_8c = (void*)0x41b260;
        t->cb_90 = (void*)0x41b4c0;
        t->cb_94 = (void*)0x41b6d0;
        t->cb_98 = (void*)0x41b2a0;
        t->cb_9c = (void*)0x41b6f0;
    }
}

/* ========================================================================== */
/* PlayAppropriateBuildEffect                                                 */
/* ========================================================================== */

/* The game FX table at 0x004b9228 (mapinit.c loads it): {name, pad, sample}. */
typedef struct FXEntry {
    char* name;     /* +0x00 */
    int   pad4;     /* +0x04 */
    void* sample;   /* +0x08 */
} FXEntry;
extern FXEntry g_game_fx[];                                     /* 0x004b9228 */

/* A sound source; kind 2 = a map square at (x,y). +0x04 is left alone. */
typedef struct SoundSource {
    int kind;   /* +0x00 */
    int f04;    /* +0x04 */
    int x;      /* +0x08 */
    int y;      /* +0x0c */
} SoundSource;

extern void* PlayInstanceOfSample(void* sample, int a, int b,
                                  SoundSource* src);            /* 0x00496d20 */

/* The object class as the build code sees it: the OC_* flags at +0x1c. */
typedef struct BuildClass {
    char         pad0[0x1c];
    unsigned int flags;     /* +0x1c */
} BuildClass;

/* Play game FX `which` from `src` (unsourced when src is null). Kept as an
 * inlined helper on purpose — see PlayAppropriateBuildEffect. */
static __inline void PlayGameFX(int which, SoundSource* src)
{
    PlayInstanceOfSample(g_game_fx[which].sample, 0, 1, src);
}

/* Pick the "built" sound for a class: 0x40000 -> fx[1]; 0x80000 -> fx[0] when
 * 0x200000 is also set, else one of fx[3..7] at random; otherwise fx[0].
 * A null position plays the sound unsourced.
 *
 * Codegen lever: the FX index must reach the array subscript as a VALUE (the
 * inlined helper's parameter), so the random arm computes `rand()%5 + 3`
 * before the stride multiply (`add edx,3 / lea edx,[edx+edx*2]`). Indexing
 * `g_game_fx[rand() % 5 + 3]` inline folds the +3 into the address, and
 * spelling the four calls out directly (with or without per-arm returns)
 * lets VC6 tail-merge them into a single call with a selected sample; the
 * original keeps four separate call blocks, each with its own pop/add/ret. */
// FUNCTION: LEGOLAND 0x00462d10
void PlayAppropriateBuildEffect(BuildClass* cls, Pos* pos)
{
    SoundSource  src;
    SoundSource* srcp;
    unsigned int flags;

    if (pos != 0) {
        src.kind = 2;
        src.x = pos->x;
        src.y = pos->y;
        srcp = &src;
    } else {
        srcp = 0;
    }
    flags = cls->flags;
    if (flags & 0x40000) {
        PlayGameFX(1, srcp);
    } else if (flags & 0x80000) {
        if (flags & 0x200000)
            PlayGameFX(0, srcp);
        else
            PlayGameFX(rand() % 5 + 3, srcp);
    } else {
        PlayGameFX(0, srcp);
    }
}

/* ========================================================================== */
/* AddOvSav                                                                   */
/* ========================================================================== */

/* One 20-byte perimeter record (the .MAP `n_extra` layout, pathgfx.c). */
typedef struct PerimRec {
    int          x;      /* +0x00 */
    int          y;      /* +0x04 */
    int          x2;     /* +0x08 */
    int          y2;     /* +0x0c */
    unsigned int image;  /* +0x10 image&0xff = frame, (image>>8)&0xff -> bridge bank */
} PerimRec;

/* An animation header: 16-bit frame count at +0x10. */
typedef struct AnimHdr {
    char  pad0[0x10];
    short frames;        /* +0x10 */
} AnimHdr;

/* A sprite definition: anim header at +0, kind at +0x14 (2/3 animate). */
typedef struct SpriteDef {
    AnimHdr* anim;       /* +0x00 */
    char     pad4[0x10];
    int      kind;       /* +0x14 */
} SpriteDef;

typedef struct BankEntry {
    char       pad0[8];
    SpriteDef* def;      /* +0x08 */
} BankEntry;

/* A loaded LLIDB image bank: count at +0x04, entry table at +0x08. */
typedef struct SpriteBank {
    int         pad0;
    int         count;   /* +0x04 */
    BankEntry** entries; /* +0x08 */
} SpriteBank;

/* The terrain render object — 0x24 bytes, singly linked through +0x1c. */
typedef struct TerrainObj {
    PerimRec           rec;    /* +0x00 */
    int                sx;     /* +0x14 */
    int                sy;     /* +0x18 */
    struct TerrainObj* next;   /* +0x1c */
    BankEntry*         sprite; /* +0x20 */
} TerrainObj;

extern TerrainObj* g_terrain_objs;   /* 0x00667ca8 */
extern SpriteBank* g_terrain_bank;   /* 0x00667cac */
extern SpriteBank* g_bridge_bank;    /* 0x00667cb0 */

/* Append one terrain/overlay object to the render list AND bind its sprite
 * straight away (the load-time twin, BuildPerimeterObject 0x462c00, defers
 * the binding to BindTerrainObjectSprites). A bridge piece with no bridge
 * bank loaded is dropped. */
// FUNCTION: LEGOLAND 0x00462b30
void AddOvSav(PerimRec* rec)
{
    TerrainObj* tail = g_terrain_objs;
    TerrainObj* obj;
    BankEntry*  s;
    SpriteDef*  d;

    while (tail != 0 && tail->next != 0)
        tail = tail->next;

    if (g_terrain_bank == 0)
        return;
    if ((rec->image & 0xff00) && g_bridge_bank == 0)
        return;

    obj = (TerrainObj*)HeapAlloc_w(0x24);
    obj->next = 0;
    if (tail != 0)
        tail->next = obj;
    else
        g_terrain_objs = obj;

    obj->sx = rec->x;
    obj->sy = rec->y;
    obj->rec = *rec;

    if (rec->image & 0xff00)
        s = g_bridge_bank->entries[rec->image & 0xff];
    else
        s = g_terrain_bank->entries[rec->image & 0xff];
    obj->sprite = s;
    d = s->def;
    if (d->kind == 2 || d->kind == 3) {
        if (d->anim->frames > 1)
            LLSPlay(d->anim, d);
    }
}

/* ========================================================================== */
/* GetVisitorName                                                             */
/* ========================================================================== */

/* A visitor (bloke): its class at +0x04, first-name index at +0x83 and
 * surname index at +0x84 (both bytes, rolled by the randomiser next door:
 * rand()%83 or %90 for the first name by gender, rand()%107 for the
 * surname). The class's +0x84 word is the gender switch (nonzero = female). */
typedef struct BlokeClass {
    char pad0[0x84];
    int  female;            /* +0x84 */
} BlokeClass;

typedef struct Bloke {
    char          pad0[4];
    BlokeClass*   cls;      /* +0x04 */
    char          pad08[0x83 - 0x08];
    unsigned char first;    /* +0x83 */
    unsigned char surname;  /* +0x84 */
} Bloke;

extern char* g_male_names[];     /* 0x004bcecc, 83 entries "Aaron".."Walt" */
extern char* g_female_names[];   /* 0x004bd018, 90 entries "Abbie".."Wendy" */
extern char* g_surnames[];       /* 0x004bd180, 107 entries "Adams".."Zennon" */
extern char  g_name_buf[];       /* 0x0066b470 */
extern char  g_name_space[];     /* 0x004b8ad4 " " */

/* Compose "<first> <surname>" into the shared name buffer and return it.
 *
 * Codegen lever: the strcpy must be written in EACH arm. VC6 tail-merges the
 * two inline expansions into one (so the object still shows a single
 * repne scasb / rep movsd after the join) but allocates the per-arm index
 * loads for the merged copy — that is what puts the second arm's index in
 * ecx. Selecting the name into a local and copying once picks eax there. */
// FUNCTION: LEGOLAND 0x00482ba0
char* GetVisitorName(Bloke* b)
{
    if (b->cls->female == 0)
        strcpy(g_name_buf, g_male_names[b->first]);
    else
        strcpy(g_name_buf, g_female_names[b->first]);
    strcat(g_name_buf, g_name_space);
    strcat(g_name_buf, g_surnames[b->surname]);
    return g_name_buf;
}

/* ========================================================================== */
/* Copters_Save                                                               */
/* ========================================================================== */

typedef struct RideInstNode {
    struct RideInstNode* next;   /* +0x00 */
    char pad04[0x10];
} RideInstNode;

typedef struct RideDef {
    char pad00[0xcc];
    RideInstNode* instances;     /* +0xcc */
} RideDef;

/* COPTERS record: 0xd8 bytes, next at +0x04, six 0x20-byte seat blocks at
 * +0x18 each holding an instance pointer at +0x18. */
typedef struct CopterSeat {
    char          pad00[0x18];
    RideInstNode* inst;          /* +0x18 */
    char          pad1c[4];
} CopterSeat;

typedef struct CoptersRec {
    char                pad00[4];
    struct CoptersRec*  next;    /* +0x04 */
    char                pad08[0x18 - 8];
    CopterSeat          seat[6]; /* +0x18 .. +0xd8 */
} CoptersRec;

extern CoptersRec* g_copters_head;     /* 0x004c11b4 */
extern RideDef*    g_copters_def;      /* 0x004c1198 */

/* Unlike Catapult_Save (which serialises a copy) this patches the SIX seat
 * instance pointers IN PLACE to 1-based indices, writes the live record, then
 * restores the pointers from a local save array. The class pointer is re-read
 * from the global for every seat and the search target is read eagerly.
 *
 * Codegen levers: (1) the same double-read head guard as Catapult_Save, but
 * here `p` is spilled to the frame, and its spill store lands AFTER the
 * one/zero stores only when `p` is declared after them; (2) `saved[i]` must
 * be assigned from the seat field BEFORE the slot pointer is formed — that
 * orders the two induction increments (`add edi,4` before `add edx,0x20`). */
// FUNCTION: LEGOLAND 0x00404f60
int Copters_Save(void)
{
    int            one = 1;
    int            zero = 0;
    CoptersRec*    p = g_copters_head;
    RideInstNode*  saved[6];
    RideInstNode*  inst;
    RideInstNode*  target;
    RideInstNode** slot;
    int            i;
    int            idx;

    if (g_copters_head) {
    for (; p; p = p->next) {
        if (!SaveGameWrite(&one, 4))
            return 0;
        for (i = 0; i < 6; i++) {
            saved[i] = p->seat[i].inst;
            slot = &p->seat[i].inst;
            target = saved[i];
            inst = g_copters_def->instances;
            idx = 0;
            while (inst && inst != target) {
                inst = inst->next;
                idx++;
            }
            if (inst)
                *slot = (RideInstNode*)(idx + 1);
            else
                *slot = 0;
        }
        if (!SaveGameWrite(p, sizeof(CoptersRec)))
            return 0;
        for (i = 0; i < 6; i++)
            p->seat[i].inst = saved[i];
    }
    }
    return SaveGameWrite(&zero, 4) != 0;
}

/* ========================================================================== */
/* LoadPos                                                                    */
/* ========================================================================== */

typedef struct Matrix {
    float m[9];
} Matrix;

extern void BuildYRotationMatrix(float angle, Matrix* m);          /* 0x00443360 */
extern void MatrixMultiply(Matrix* a, Matrix* b, Matrix* out);     /* 0x00443270 */
extern void CopyMatrix(Matrix* src, Matrix* dst);                  /* 0x00443490 */

/* One position/orientation entry (0x30 bytes): three scalars then a 3x3
 * float matrix, all read raw from the file. */
typedef struct PosItem {
    int    a;        /* +0x00 */
    int    b;        /* +0x04 */
    int    c;        /* +0x08 */
    Matrix m;        /* +0x0c */
} PosItem;

/* The table (0x28 bytes): per-frame item count at +0, frame count at +4,
 * a unit scale triple at +8, two zeroed ints, item-array pointer at +0x24
 * (math3d.c UnloadPos frees it). */
typedef struct PosTable {
    int       per;       /* +0x00 items per frame */
    int       count;     /* +0x04 frames */
    float     sx;        /* +0x08 = 1.0 */
    float     sy;        /* +0x0c = 1.0 */
    float     sz;        /* +0x10 = 1.0 */
    int       f14;       /* +0x14 = 0 */
    int       f18;       /* +0x18 = 0 */
    char      pad1c[0x24 - 0x1c];
    PosItem** items;     /* +0x24 count arrays of `per` items */
} PosTable;

/* Read a .POS resource: {int per, int count, count x per x {a,b,c,3x3 m}}.
 * Every matrix is rotated a quarter turn about Y on the way in. */
// FUNCTION: LEGOLAND 0x0043f660
PosTable* LoadPos(const char* name)
{
    void*     file;
    PosTable* t;
    PosItem*  item;
    Matrix    rot;
    Matrix    tmp;
    int       i;
    int       j;
    int       r;
    int       c;

    file = RES_OpenFile(name);
    t = (PosTable*)HeapAlloc_w(sizeof(PosTable));
    t->sx = 1.0f;
    t->sy = 1.0f;
    t->sz = 1.0f;
    t->f14 = 0;
    t->f18 = 0;
    RES_ReadFile(file, &t->per, 4);
    RES_ReadFile(file, &t->count, 4);
    t->items = (PosItem**)HeapAlloc_w(t->count * 4);
    for (i = 0; i < t->count; i++) {
        t->items[i] = (PosItem*)HeapAlloc_w(t->per * sizeof(PosItem));
        item = t->items[i];
        for (j = 0; j < t->per; j++) {
            RES_ReadFile(file, &item[j].a, 4);
            RES_ReadFile(file, &item[j].b, 4);
            RES_ReadFile(file, &item[j].c, 4);
            for (r = 0; r < 3; r++)
                for (c = 0; c < 3; c++)
                    RES_ReadFile(file, &item[j].m.m[r * 3 + c], 4);
            BuildYRotationMatrix(1.5707963f, &rot);
            MatrixMultiply(&rot, &item[j].m, &tmp);
            CopyMatrix(&tmp, &item[j].m);
        }
    }
    RES_CloseFile(file);
    return t;
}

/* ========================================================================== */
/* LoadObjectLibrary                                                          */
/* ========================================================================== */

/* A loaded object library (16 bytes, list head at 0x00669244). */
typedef struct ObjLib {
    struct ObjLib* next;                                  /* +0x00 */
    void*          handle;                                /* +0x04 HMODULE */
    int            refcount;                              /* +0x08 */
    void         (*get_interfaces)(LLElem*, IfaceTable*); /* +0x0c registered by the DLL */
} ObjLib;

/* The class record as the loader fills it: library at +0x88, callback slots
 * +0x8c..+0xc0, its LLIDB element at +0xc4. */
typedef struct LibClass {
    char     pad0[0x88];
    ObjLib*  lib;      /* +0x88 */
    void*    cb_8c;    /* +0x8c */
    void*    cb_90;    /* +0x90 */
    void*    cb_94;    /* +0x94 */
    void*    cb_98;    /* +0x98 */
    void*    cb_9c;    /* +0x9c */
    void*    cb_a0;    /* +0xa0 */
    void*    cb_a4;    /* +0xa4 */
    void*    cb_a8;    /* +0xa8 */
    void*    cb_ac;    /* +0xac */
    void*    cb_b0;    /* +0xb0 */
    void*    cb_b4;    /* +0xb4 */
    void*    cb_b8;    /* +0xb8 */
    void*    cb_bc;    /* +0xbc */
    void*    cb_c0;    /* +0xc0 */
    LLElem*  elem;     /* +0xc4 */
} LibClass;

extern ObjLib* g_objlib_head;    /* 0x00669244 */
extern ObjLib* g_objlib_cur;     /* 0x007fd620 — the record the DLL registers into */
extern ObjLib  g_objlib_tmp;     /* 0x007fd610 */
extern char    g_dll_path_fmt[]; /* 0x004bcea4 ".\\dlls\\%s.dll" */

/* Load (or re-reference) the class's DLL and install its interface table.
 *
 * Matches the original exactly — all 138 instructions / 489 bytes, including
 * the `found` block VC6 places out of line AFTER the final `ret`
 * (0x004810ca..0x004810e4), which ends in a backward `jmp` into the join.
 * Held as WIP only for tooling: tools/audit.py bounds a function by its last
 * `ret` or an EXTERNAL `jmp`, so on the original side it runs past that
 * internal backward `jmp` into the nop padding and the next routine
 * (0x004810f0) and reports 172i/578B. Flip to `// FUNCTION:` once audit.py
 * also stops at a trailing backward jmp whose target is inside the function.
 *
 * Codegen levers: `path` is _MAX_PATH (260) bytes — that is the frame's extra
 * 4 bytes over two 0x100 buffers; the found path's three stores + `goto` from
 * inside the search loop are what VC6 hoists to the end of the function; the
 * new-record path re-reads g_objlib_cur/g_objlib_head after every store
 * through a pointer (they may alias), giving the original's reloads; the
 * 16-byte record copy is four register moves; the three unconditional slot
 * stores (+0xb4/+0xb8/+0xbc) are reordered by VC6 from source order. */
/* Exact over its full extent (138i/489B) including the out-of-line `found`
 * block that ends in a backward jmp into the body; tools/audit.py bounds it
 * since it learned that an unconditional jmp nothing jumps past ends the
 * function. match.py stops at the earlier ret and matches up to there. */
// FUNCTION: LEGOLAND 0x00480f00
int LoadObjectLibrary(LibClass* obj, char* name)
{
    IfaceTable t;
    char       fname[0x100];
    char       path[260];    /* _MAX_PATH */
    void*      h;
    ObjLib*    lib;

    _splitpath(name, 0, 0, fname, 0);
    g_objlib_cur = &g_objlib_tmp;
    Format(path, g_dll_path_fmt, fname);
    g_objlib_cur->handle = LoadLibraryExA(path, 0, 0);
    h = g_objlib_cur->handle;
    if (h == 0)
        return 0;

    for (lib = g_objlib_head; lib; lib = lib->next) {
        if (lib->handle == h) {
            g_objlib_cur = lib;
            lib->refcount++;
            obj->lib = lib;
            goto found;
        }
    }
    g_objlib_cur->next = g_objlib_head;
    g_objlib_cur->refcount = 1;
    lib = (ObjLib*)HeapAlloc_w(sizeof(ObjLib));
    g_objlib_head = lib;
    *lib = *g_objlib_cur;
    obj->lib = g_objlib_head;
    lib = g_objlib_head;

found:
    memset(&t, 0, sizeof(t));
    lib->get_interfaces(obj->elem, &t);
    if (t.cb_8c) obj->cb_8c = t.cb_8c;
    if (t.cb_90) obj->cb_90 = t.cb_90;
    if (t.cb_94) obj->cb_94 = t.cb_94;
    if (t.cb_98) obj->cb_98 = t.cb_98;
    if (t.cb_9c) obj->cb_9c = t.cb_9c;
    if (t.cb_a0) obj->cb_a0 = t.cb_a0;
    if (t.cb_a8) obj->cb_a8 = t.cb_a8;
    if (t.cb_ac) obj->cb_ac = t.cb_ac;
    if (t.cb_c0) obj->cb_c0 = t.cb_c0;
    obj->cb_b4 = t.cb_b4;
    obj->cb_b8 = t.cb_b8;
    obj->cb_bc = t.cb_bc;
    if (t.cb_b0) obj->cb_b0 = t.cb_b0;
    if (t.init)
        t.init(obj->elem);
    return 1;
}
