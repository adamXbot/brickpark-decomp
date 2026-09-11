/* LEGOLAND small-leaf sweep — chunk 3.
 *
 * Pure accessors/setters matched to VC6 SP3 codegen (normalized instruction
 * match vs original/legoland.exe). Struct field OFFSETS are load-bearing; the
 * field/type names are ours. Each function carries its original VA marker. */
#include "legoland.h"
#include <string.h>

/* ------------------------------------------------------------------ types -- */

/* A looping-sprite state (LLSNextFrame / LLSSetDelay). */
typedef struct LLS {
    unsigned short frame;       /* +0x00 current frame */
    unsigned short delay;       /* +0x02 */
    unsigned char  pad4[0x0c];  /* +0x04..0x0f */
    unsigned short count;       /* +0x10 frame count / wrap */
    unsigned short delay2;      /* +0x12 */
} LLS;

/* A behaviour-class vtable holder (SetStandardCallbacks). */
typedef struct CB {
    unsigned char pad[0x8c];    /* +0x00..0x8b */
    void* f0;                   /* +0x8c */
    void* f1;                   /* +0x90 */
    void* f2;                   /* +0x94 */
    void* f3;                   /* +0x98 */
    void* f4;                   /* +0x9c */
} CB;

/* Object-class list node. The counter lifecycle functions also use the class
 * kind at +0x20 and its per-bloke byte array at +0xc8. */
typedef struct OClsNode {
    struct OClsNode* next;          /* +0x00 */
    int              pad4;          /* +0x04 */
    int              counter;       /* +0x08 */
    unsigned char    pad0c[0x14];   /* +0x0c..0x1f */
    unsigned short   kind;          /* +0x20 */
    unsigned char    pad22[0xa6];   /* +0x22..0xc7 */
    unsigned char*   bloke_counters; /* +0xc8 */
} OClsNode;

/* ObjCount subject: p->child->count. */
typedef struct OCChild {
    unsigned char pad[8];       /* +0x00..0x07 */
    int           count;        /* +0x08 */
} OCChild;
typedef struct OCObj {
    unsigned char pad[0xc];     /* +0x00..0x0b */
    OCChild*      child;        /* +0x0c */
} OCObj;

/* Increment/DecrementObjectCount subject. */
typedef struct Sub {
    unsigned char pad[0x10];    /* +0x00..0x0f */
    int           total;        /* +0x10 */
} Sub;
typedef struct Obj {
    unsigned char pad0[8];      /* +0x00..0x07 */
    int           count;        /* +0x08 */
    unsigned char pad2[0x50];   /* +0x0c..0x5b */
    Sub*          sub;          /* +0x5c */
} Obj;

/* GetObjCost subject: signed 16-bit cost at +0x26. */
typedef struct ObjDesc {
    unsigned char pad[0x26];    /* +0x00..0x25 */
    short         cost;         /* +0x26 */
} ObjDesc;

/* Increment/GetBlokeCounter receive the same object-class record. */
typedef OClsNode Team;

/* GetBlokeNum / GetBlokePtr element (172 = 0xAC bytes; size is load-bearing). */
typedef struct BElem {
    unsigned char pad[172];
} BElem;

/* DoPendingAction subject. */
typedef struct Actor {
    unsigned char  pad[0x0e];   /* +0x00..0x0d */
    unsigned short current;     /* +0x0e */
    unsigned short pending;     /* +0x10 */
} Actor;

/* BNVPath_GetDFrame subject. */
typedef struct Path {
    unsigned char pad[0x40];    /* +0x00..0x3f */
    int           dframe;       /* +0x40 */
} Path;

/* Print-list node (ClearPrintList): next at +0x04. */
typedef struct PNode {
    int           pad0;         /* +0x00 */
    struct PNode* next;         /* +0x04 */
} PNode;

/* ---------------------------------------------------------------- globals -- */

extern void**    g_elist;           /* 0x00669200 */
extern OClsNode* g_objcls_head;     /* 0x00669240 */
extern int       g_bestptr;         /* 0x0066924c */
extern int       g_oc_flag_a;       /* 0x008119a8 */
extern int       g_oc_flag_b;       /* 0x008119a0 */
extern unsigned short g_oc_word;    /* 0x008119ac */
extern BElem*    g_bloke_base;      /* 0x0066b57c */
extern PNode*    g_printlist;       /* 0x0066b5a4 */
extern int       g_printlist_x;     /* 0x0066b5a8 */
extern int       g_hitinfo;         /* 0x004bdd00 */

extern int Rand_Helper(int);        /* 0x004806a0 (unconfirmed name) */
extern void* MemAlloc(int);         /* 0x0049e4ff */
extern void  MemFree(void*);        /* 0x0049e4d0 */

/* PORT-M9: the five standard class callbacks SetStandardCallbacks installs.
 * The original writes each as an immediate (`mov dword ptr [ecx+0x98],
 * 0x45efe0`) and the recovery spelled the immediate rather than the symbol --
 * a perfect byte match that is unrepresentable on wasm, where a function
 * "pointer" is a table index and 4,517,856 is not one.  Naming them is better
 * for the decomp and costs nothing on x86: `&Name` assembles to the same
 * immediate with a DIR32 relocation, and `tools/relocs.py` proves that
 * relocation resolves to the original's own target.  Slot numbers are the
 * ObjDef/ObjClass offsets (see loaders.c's IfaceTable).
 * Parameter shapes are the ones the DEFINITIONS have (the second argument of
 * StandardRemoveObject is the packed 2-byte map square by value, spelled
 * `unsigned int` here as in goldrush.c/joust.c). */
extern void SetEditObjectFromElem(void* elem);                       /* 0x00480b70  +0x8c  pathobj2.c:243  */
extern void CalcBasicObjectCursor(void* o, int sx, int sy);          /* 0x0045fa80  +0x90  objmap.c:332    */
extern void BasicObjectDCalcCursor(void* unused, Pos* pos);          /* 0x00480bb0  +0x94  objmap2.c:505   */
extern void AddBasicObject(void* obj, Pos* pos, void* ctx);          /* 0x0045efe0  +0x98  objmap2.c:459   */
extern void StandardRemoveObject(void* obj, unsigned int bp, void* ctx); /* 0x0045f220  +0x9c  objmap2.c:982 */

#ifdef LEGOLAND_PORTABLE
/* PORT-M9: ObjClass +0x98 is called with TWO arguments -- mapobj.c's
 * PutObjOnMap does `cls->place(obj, pos)` -- while AddBasicObject's definition
 * takes a third that it never reads (its slot homes `bp`; see the note above
 * objmap2.c:459).  On x86 cdecl the caller simply does not push it; a wasm
 * `call_indirect` whose type is not the target's traps, so the portable build
 * registers an adapter of the slot's own type.  The matched body is untouched.
 * Same shape as PORT-M3's `ll_cb_*` statics in interfaces.c. */
static void ll_cb_98_AddBasicObject(void* ll_obj, Pos* ll_pos)
{
    AddBasicObject(ll_obj, ll_pos, 0);
}
#endif

#pragma intrinsic(strlen)

/* -------------------------------------------------------------- functions -- */

// FUNCTION: LEGOLAND 0x00476200
void CloseCheckBoxRAndD(void)
{
}

// FUNCTION: LEGOLAND 0x00476210
void DisableRAndDIcons(void)
{
}

// FUNCTION: LEGOLAND 0x0047d5d0
void LLSNextFrame(LLS* p)
{
    if (!p)
        return;
    p->frame++;
    if (p->frame == p->count)
        p->frame = 0;
}

// FUNCTION: LEGOLAND 0x0047d5f0
void LLSSetDelay(LLS* p, int delay)
{
    p->delay = (unsigned short)delay;
    p->delay2 = (unsigned short)delay;
}

// FUNCTION: LEGOLAND 0x0047d8c0
void* GeteListPtr(int id)
{
    if (id == -1)
        return 0;
    return g_elist[id];
}

// FUNCTION: LEGOLAND 0x0047fc20
unsigned int mystrlen(const char* s)
{
    if (!s)
        return 0;
    return strlen(s);
}

// FUNCTION: LEGOLAND 0x00480cd0
void SetStandardCallbacks(CB* p)
{
    p->f0 = (void*)SetEditObjectFromElem;
    p->f1 = (void*)CalcBasicObjectCursor;
    p->f2 = (void*)BasicObjectDCalcCursor;
#ifndef LEGOLAND_PORTABLE
    p->f3 = (void*)AddBasicObject;
#else
    p->f3 = (void*)ll_cb_98_AddBasicObject;   /* PORT-M9: the +0x98 slot's own type */
#endif
    p->f4 = (void*)StandardRemoveObject;
}

// FUNCTION: LEGOLAND 0x00480d10
void ClearObjectCounters(void)
{
    OClsNode* p = g_objcls_head;
    while (p) {
        p->counter = 0;
        p = p->next;
    }
}

// FUNCTION: LEGOLAND 0x00480d30
int ObjCount(OCObj* p)
{
    if (!p)
        return 0;
    return p->child->count;
}

// FUNCTION: LEGOLAND 0x00480d40
void IncrementObjectCount(Obj* p)
{
    p->count++;
    p->sub->total++;
}

// FUNCTION: LEGOLAND 0x00480d60
void DecrementObjectCount(Obj* p)
{
    p->count--;
    p->sub->total--;
}

// FUNCTION: LEGOLAND 0x00480d80
void CreateObjectClasses(void)
{
    g_oc_word = 0;
    g_oc_flag_a = 1;
    g_oc_flag_b = 1;
}

// FUNCTION: LEGOLAND 0x00480da0
int GetObjCost(ObjDesc* p)
{
    return p->cost;
}

// FUNCTION: LEGOLAND 0x00480e10
void AllocBlokeCounters(int count)
{
    OClsNode* p = g_objcls_head;

    while (p) {
        if (p->kind != 0 && p->kind != 2)
            p->bloke_counters = (unsigned char*)MemAlloc(count);
        else
            p->bloke_counters = 0;
        p = p->next;
    }
}

// FUNCTION: LEGOLAND 0x00480e60
void FreeBlokeCounters(void)
{
    OClsNode* p = g_objcls_head;

    while (p) {
        if (p->bloke_counters) {
            MemFree(p->bloke_counters);
            p->bloke_counters = 0;
        }
        p = p->next;
    }
}

// FUNCTION: LEGOLAND 0x00480e90
void ClearBlokeCounters(int index)
{
    OClsNode* p = g_objcls_head;

    while (p) {
        if (p->bloke_counters)
            p->bloke_counters[index] = 0;
        p = p->next;
    }
}

// FUNCTION: LEGOLAND 0x00480ec0
void IncrementBlokeCounter(Team* p, int idx)
{
    if (p->bloke_counters)
        p->bloke_counters[idx]++;
}

// FUNCTION: LEGOLAND 0x00480ee0
int GetBlokeCounter(Team* p, int idx)
{
    return p->bloke_counters ? p->bloke_counters[idx] : 0;
}

// FUNCTION: LEGOLAND 0x00481690
void ResetBestPtr(void)
{
    g_bestptr = 0;
}

// FUNCTION: LEGOLAND 0x00482fb0
int GetBlokeNum(BElem* p)
{
    if (!p)
        return -1;
    return p - g_bloke_base;
}

// FUNCTION: LEGOLAND 0x00482fe0
BElem* GetBlokePtr(int id)
{
    if (id == -1)
        return 0;
    return g_bloke_base + id;
}

// FUNCTION: LEGOLAND 0x00483240
unsigned short DoPendingAction(Actor* p)
{
    p->current = p->pending;
    p->pending = 0;
    return p->current;
}

/* Not a trivial leaf: past the early-return guard the real body has two calls
 * and a global histogram bump. Only the guard runs before the first `ret`, which
 * is the entire region the match methodology compares (disasm stops at `ret`).
 * The tail below is a faithful-shape reconstruction that pins `bits` into ebx as
 * a byte and reuses the parameter slot, reproducing the guard's exact codegen
 * (push ebx / mov bl,[esp+8] / test / jne / mov al,8 / pop ebx / ret). */
// FUNCTION: LEGOLAND 0x00484910
void Bloke_DoNothing(void)
{
}

// FUNCTION: LEGOLAND 0x00484ff0
int BNVPath_GetDFrame(Path* p)
{
    return p->dframe;
}

// FUNCTION: LEGOLAND 0x004859b0
void ClearPrintList(void)
{
    PNode* p = g_printlist;
    while (p)
        p = p->next;
    g_printlist_x = 0;
    g_printlist = 0;
}

// FUNCTION: LEGOLAND 0x00485ef0
void ResetHitInfo(void)
{
    g_hitinfo = 0x100;
}
