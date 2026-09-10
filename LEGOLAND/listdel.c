/* LEGOLAND — singly-linked "delete the whole list" helpers, plus the two RES /
 * class-list lookups that live nearby.
 *
 * Four exported Delete*List entry points (objects, research, saved games,
 * profiles) are byte-for-byte the same routine over four different list heads:
 * every node is a plain malloc'd block whose FIRST dword is the next pointer,
 * so the walker needs no type at all beyond that. They are almost certainly one
 * hand-copied source idiom rather than a macro or a shared helper — a macro
 * would have been reused elsewhere, and a helper taking the head by address
 * would not produce the constant-0 store in the empty-list path. */
#include <stdlib.h>

/* A list node as these walkers see it: next pointer first, payload after. */
typedef struct ListNode {
    struct ListNode* next;   /* +0x00 */
} ListNode;

/* 0x0049e4d0 — the game's free() wrapper (same address other lanes call
 * MemFree / HeapFree_w / RES_FreeFile). */
extern void MemFree(void* p);

extern ListNode* g_object_list;      /* 0x00668e40 */
extern ListNode* g_research_list;    /* 0x00668ed8 */
extern ListNode* g_savedgame_list;   /* 0x00798734 */
extern ListNode* g_profile_list;     /* 0x00798890 */

/* The shape, once, for all four:
 *
 *   mov eax,[head] / test eax,eax / je <empty>   <- rotated while, guard peeled
 *   push esi                                     <- prologue SUNK past the guard
 * loop:
 *   mov esi,[eax] / push eax / call MemFree / add esp,4
 *   mov eax,esi / test esi,esi / jne loop
 *   mov [head],esi / pop esi / ret               <- stores the register it KNOWS is 0
 * empty:
 *   mov [head],0 / ret                           <- same store, immediate form
 *
 * The two different encodings of the *same* "head = NULL" assignment are the
 * tell that this is one plain `while (p) { ... }` followed by one store, not an
 * if/else: VC6 duplicated the trailing store into both cold and hot exits and
 * constant-folded `p` to 0 only on the path where it never left the head load. */

// FUNCTION: LEGOLAND 0x004756e0
void DelObjectList(void)
{
    ListNode* p = g_object_list;

    while (p) {
        ListNode* next = p->next;
        MemFree(p);
        p = next;
    }
    g_object_list = 0;
}

/* [sic] The export table really does spell it "DeleteReseachList". */
// FUNCTION: LEGOLAND 0x00476420
void DeleteReseachList(void)
{
    ListNode* p = g_research_list;

    while (p) {
        ListNode* next = p->next;
        MemFree(p);
        p = next;
    }
    g_research_list = 0;
}

// FUNCTION: LEGOLAND 0x0048e160
void DeleteSavedGameList(void)
{
    ListNode* p = g_savedgame_list;

    while (p) {
        ListNode* next = p->next;
        MemFree(p);
        p = next;
    }
    g_savedgame_list = 0;
}

// FUNCTION: LEGOLAND 0x00491b50
void DeleteProfileList(void)
{
    ListNode* p = g_profile_list;

    while (p) {
        ListNode* next = p->next;
        MemFree(p);
        p = next;
    }
    g_profile_list = 0;
}

/* ------------------------------------------------------------------ *
 *  RES_FileExists (0x00489e30)                                        *
 * ------------------------------------------------------------------ */

/* Both from LEGOLAND/res.c's layer; the opener returns a malloc'd RFile. */
extern void* RES_OpenFile(const char* path);   /* 0x00489b60 */
#ifndef LEGOLAND_PORTABLE
extern void  RES_CloseFile(void* f);           /* 0x00489de0 */
#else
extern int RES_CloseFile(void* f);           /* 0x00489de0 */
#endif

/* Open-and-immediately-close is the whole implementation: there is no cheaper
 * "is it in the master directory" query, because RES_OpenFile is what walks the
 * directory bucket. The failure path returns the null handle itself rather than
 * a fresh 0 (`ret` straight after `test eax,eax`), which is just VC6 reusing
 * eax — `return 0` written literally still compiles to this. */
// FUNCTION: LEGOLAND 0x00489e30
int RES_FileExists(const char* path)
{
    void* f = RES_OpenFile(path);

    if (!f)
        return 0;

    RES_CloseFile(f);
    return 1;
}

/* ------------------------------------------------------------------ *
 *  GetInstanceOfClass (0x0048a0c0)                                    *
 * ------------------------------------------------------------------ */

/* An instance record in a class's instance list. Only two fields are touched
 * here, and they pin the layout: the next pointer is FIRST (same convention as
 * the Delete*List nodes above) and a 16-bit class id sits at +0x0e. */
typedef struct Instance {
    struct Instance* next;   /* +0x00 */
    char             pad4[0x0e - 0x04];
    unsigned short   klass;  /* +0x0e  class id */
} Instance;

/* The owner of an instance list. +0x00 is something else (unread here); the
 * list head is at +0x04. */
typedef struct InstanceList {
    int       pad0;          /* +0x00 */
    Instance* head;          /* +0x04 */
} InstanceList;

/* The class id arrives BY ADDRESS, not by value — `mov ecx,[esp+8] /
 * mov cx,[ecx]` — so the caller passes a pointer to (the first field of) a
 * class/descriptor record and only its leading u16 is ever read.
 *
 * The dereference happens AFTER the empty-list guard: that lazy read is what
 * keeps the load out of the je-taken path.
 *
 * The loop shape is load-bearing and is NOT the obvious
 * `for (p = head; p; p = p->next) if (match) return p;` — that form compiles
 * with the back edge landing on the compare and the empty-list exit FALLING
 * INTO the `xor eax,eax`. The original instead duplicates the compare at the
 * loop bottom (`cmp [eax+0xe],cx / jne <load next>`) and jumps to a shared
 * cold `xor eax,eax; ret`, which is what you get when the empty-list test is an
 * explicit early return and the loop's own condition is the KEY compare, with
 * the "ran off the end" check written inside the body. */
// FUNCTION: LEGOLAND 0x0048a0c0
Instance* GetInstanceOfClass(InstanceList* list, const unsigned short* klass)
{
    Instance* p = list->head;

    if (!p)
        return 0;

    while (p->klass != *klass) {
        p = p->next;
        if (!p)
            return 0;
    }
    return p;
}
