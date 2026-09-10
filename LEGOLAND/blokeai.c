/* LEGOLAND - bloke creation, depth sorting and high-level AI dispatch.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; the field and type names are ours.
 * Each matched function carries its original VA marker on the line above it.
 *
 * ---------------------------------------------------------------------------
 * HOW A BLOKE IS CREATED
 *
 * Blokes are not malloc'd one at a time: they live in a fixed pool at
 * g_bloke_base (0x0066b57c), 172 bytes each, whose capacity is the u16 at
 * g_map+0x1a. Bit 0 of the flags word at +0x62 marks a slot as taken.
 *
 *   MakeBloke()                       0x004830c0
 *     b = NewBloke()                  0x00482ef0  first free pool slot, zeroed,
 *                                                 flags62 = 1, pushed on the
 *                                                 g_people_head chain
 *       Add3DBlokeToList(b, 1)        0x00440680
 *         p = Create3DPerson(b, 1)    0x0043f8c0  0x94-byte 3D person: kind,
 *                                                 back pointer, random sex
 *                                                 (rand() & 1) for kind 1,
 *                                                 -1 in the texture/anim/colour
 *                                                 selectors, 2.0f scale
 *         b->person = p
 *         Add3DPersonToList(p)        0x0043f810  push on g_person_head
 *         UpdatePersonPos(p, b)       0x004401b0
 *         BlokeWalkAnim(b)            0x00440910
 *     ClearBlokeCounters(GetBlokeNum(b))          per-class visit counters
 *
 * Two intrusive lists therefore exist: the bloke chain (Bloke::next, head
 * 0x0066b574, walked by Control3DPeople/RenderPeople) and the 3D-person chain
 * (prev @+0x00 / next @+0x04, head 0x00655a3c). Find3DPersonFromBloke walks
 * the second one by the back pointer at +0x0c; Remove3DPersonFromList
 * (0x0043f840) is the unlink half.
 *
 * The sibling at 0x00482f70 is a malloc'd variant of NewBloke (172 bytes off
 * the heap, then Add3DBlokeToList(b, kind)) - not in this file.
 *
 * ---------------------------------------------------------------------------
 * DEPTH SORTING
 *
 * SortBlokeIn3D hands the bloke's 3D person to the frame's "print list":
 * SortPerson bump-allocates a 0x20-byte item out of the arena at 0x007cb600
 * (byte cursor g_printlist_x @ 0x0066b5a8 - the same cursor sweep3.c's
 * ClearPrintList resets), stamps type 0x2000 and the person into it and lets
 * InsertPrintItem (0x00485bd0) binary-tree it by the KEY, which is the
 * person's +0x54 - its projected depth. The third argument (a 12-byte
 * {0x306, bloke, 0} descriptor) is built by the caller but never read here;
 * the 0x4c-byte sibling allocator at 0x00485e40 is the sprite variant that does
 * copy its descriptors.
 *
 * ---------------------------------------------------------------------------
 * HIGH-LEVEL AI DISPATCH
 *
 * DoHighLevelAI is the whole dispatcher: the plan word at +0x0c indexes the
 * 26-entry handler table at 0x004b8368 (null-checked, so plan 5 is a no-op),
 * then walk_delay (+0x75) is forced to 1 and +0x64 cleared. The table:
 *
 *   0x00 Bloke_DoNothing       0x0d 0x0044fe80 (internal)
 *   0x01 0x0044f170 (internal) 0x0e 0x00450250 (internal)
 *   0x02 0x0044ebf0 (internal) 0x0f 0x00450450 (internal)
 *   0x03 0x0044ed70 (internal) 0x10 Gardener_Idle
 *   0x04 Bloke_DoNothing       0x11 Mechanic_Idle
 *   0x05 (null)                0x12 Gardener_Build
 *   0x06 0x0044f610 (internal) 0x13 Mechanic_Build
 *   0x07 Bloke_DoNothing       0x14 0x00450330 (internal)
 *   0x08 Bloke_DoNothing       0x15 Garderner_Repair  [sic]
 *   0x09 Bloke_DoNothing       0x16 Mechanics_Repair
 *   0x0a Bloke_DoNothing       0x17 0x0044fe10 (internal)
 *   0x0b Bloke_DoNothing       0x18 0x0049a4a0 (internal)
 *   0x0c Bloke_DoNothing       0x19 0x0049a4d0 (internal)
 *
 * ---------------------------------------------------------------------------
 * TEXTURE NAMES
 *
 * GetFace/GetChestTextureNameOfBloke pick a packed string list by the person's
 * sex (+0x84: 0 -> 0x00630100, 1 -> 0x0062feac), look up entry +0x80 in the
 * face (0) or chest (1) half via 0x004428f0, _stricmp the result against the
 * static "chest girly1" buffer at 0x004b7d24 - and then DISCARD that result
 * and return the static "chest girly2" buffer at 0x004b7d14 regardless. That
 * is what the shipped binary does; it is reproduced, not fixed. The lookup
 * pointer is only assigned for sex 0/1, so any other value reads it
 * uninitialised (VC6 parks it in the dead parameter slot).
 * --------------------------------------------------------------------------- */
#include "legoland.h"
#ifdef LEGOLAND_PORTABLE
#define MakeBloke MakeBloke_vc6_body
#endif

/* ------------------------------------------------------------------ types -- */

/* The renderable "3D person" hanging off a bloke (Bloke::person, +0x04).
 * 0x94 bytes, allocated by Create3DPerson (0x0043f8c0). Doubly linked into
 * the global person list at 0x00655a3c (prev @+0x00, next @+0x04). */
typedef struct Person3D {
    struct Person3D* prev;        /* +0x00 */
    struct Person3D* next;        /* +0x04 */
    int              kind;        /* +0x08  1 = visitor (random variant) */
    struct Bloke*    bloke;       /* +0x0c  back pointer to the owner */
    float            scale_x;     /* +0x10  2.0f at creation */
    float            scale_y;     /* +0x14 */
    float            scale_z;     /* +0x18 */
    int              f1c;         /* +0x1c */
    int              f20;         /* +0x20 */
    unsigned char    pad24[8];    /* +0x24..0x2b */
    int              f2c;         /* +0x2c */
    int              f30;         /* +0x30 */
    int              f34;         /* +0x34  0xff at creation */
    int              f38;         /* +0x38 */
    unsigned char    pad3c[0x10]; /* +0x3c..0x4b */
    int              f4c;         /* +0x4c */
    unsigned char    pad50[4];    /* +0x50..0x53 */
    int              depth;       /* +0x54  sort key for the print list */
    unsigned char    pad58[0x24]; /* +0x58..0x7b */
    int              f7c;         /* +0x7c  -1 at creation */
    int              tex_index;   /* +0x80  texture name index (-1 = none) */
    int              variant;     /* +0x84  0/1 sex (GetSexOfBloke) */
    int              anim;        /* +0x88  current animation index */
    int              leg_colour;  /* +0x8c  palette index (legs) */
    int              arm_colour;  /* +0x90  palette index (arms) */
} Person3D;

/* A "bloke" (worker/visitor); allocation stride 172 (0xac). */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00  intrusive 3D-people list link */
    Person3D*      person;      /* +0x04  the renderable 3D person */
    unsigned char  pad08[4];    /* +0x08..0x0b */
    unsigned short lt_action;   /* +0x0c  LONG-TERM action (plan) */
    unsigned char  pad0e[0x54]; /* +0x0e..0x61 */
    unsigned short flags62;     /* +0x62  bit 0 = slot in use (NewBloke) */
    unsigned char  f64;         /* +0x64  cleared after every re-plan */
    unsigned char  pad65[0x10]; /* +0x65..0x74 */
    unsigned char  walk_delay;  /* +0x75 */
    unsigned char  pad76[0x36]; /* +0x76..0xab */
} Bloke;

/* One entry of the depth-sorted "print list" (0x20 bytes, bump-allocated out
 * of the arena at 0x007cb600 by byte offset 0x0066b5a8). */
typedef struct PrintItem {
    unsigned char pad00[8];     /* +0x00..0x07  left/right links (InsertPrintItem) */
    int           key;          /* +0x08  sort key */
    int           type;         /* +0x0c  0x2000 = 3D person */
    unsigned char pad10[0x0c];  /* +0x10..0x1b */
    void*         person;       /* +0x1c */
} PrintItem;

/* The SortPerson caller's descriptor. SortPerson never reads it. */
typedef struct SortInfo {
    int    type;                /* +0x00  0x306 */
    Bloke* bloke;               /* +0x04 */
    short  w;                   /* +0x08 */
} SortInfo;

/* The map header seen with its bloke capacity at +0x1a (legoland.h's Map
 * only names width/height); g_map is cast to this in NewBloke. */
typedef struct MapEx {
    unsigned char  pad00[0x1a]; /* +0x00..0x19 */
    unsigned short max_blokes;  /* +0x1a */
} MapEx;

/* ---------------------------------------------------------------- globals -- */

extern Bloke*        g_people_head;         /* 0x0066b574 */
extern Bloke*        g_bloke_base;          /* 0x0066b57c */

extern Person3D*     g_person_head;         /* 0x00655a3c */
extern unsigned char g_printlist_arena[];   /* 0x007cb600 */
extern int           g_printlist_x;         /* 0x0066b5a8 */
extern void        (*g_lt_action_handlers[])(Bloke*);  /* 0x004b8368 */
extern char*         g_texnames_boy;        /* 0x00630100 */
extern char*         g_texnames_girl;       /* 0x0062feac */
extern char          s_texname_result[16];  /* 0x004b7d14 */
extern char          s_texname_cmp[16];     /* 0x004b7d24 */

/* ------------------------------------------------------------ prototypes -- */

extern Bloke*    NewBloke(void);                          /* 0x00482ef0 */
extern int       GetBlokeNum(Bloke* b);                   /* 0x00482fb0 */
extern void      ClearBlokeCounters(int index);           /* 0x00480e90 */
extern void      InsertPrintItem(PrintItem* it);          /* 0x00485bd0 */
extern Person3D* Create3DPerson(Bloke* b, int kind);      /* 0x0043f8c0 */
extern void      Add3DPersonToList(Person3D* p);          /* 0x0043f810 */
extern void      UpdatePersonPos(Person3D* p, Bloke* b);  /* 0x004401b0 */
extern void      BlokeWalkAnim(Bloke* b);                 /* 0x00440910 */
extern char*     LookupTextureName(char* list, int kind, int index); /* 0x004428f0 */
extern int       _stricmp(const char* a, const char* b); /* 0x004aab90 (CRT) */
extern void*     MemAlloc(int size);                      /* 0x0049e4ff */
extern int       rand(void);                              /* 0x0049e4b2 (CRT) */
void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)

/* -------------------------------------------------------------- functions -- */

// FUNCTION: LEGOLAND 0x004830c0
Bloke* MakeBloke(void)
{
    Bloke* b = NewBloke();

    if (b)
        ClearBlokeCounters(GetBlokeNum(b));
    return b;
}

// FUNCTION: LEGOLAND 0x004504d0
void DoHighLevelAI(Bloke* b)
{
    void (*handler)(Bloke*) = g_lt_action_handlers[b->lt_action];

    if (handler)
        handler(b);
    b->walk_delay = 1;
    b->f64 = 0;
}

// FUNCTION: LEGOLAND 0x00485e00
void SortPerson(void* person, int key, SortInfo* info)
{
    int        x  = g_printlist_x;
    PrintItem* it;

    g_printlist_x += 0x20;
    it = (PrintItem*)(g_printlist_arena + x);
    it->key = key;
    it->type = 0x2000;
    it->person = person;
    InsertPrintItem(it);
}

// FUNCTION: LEGOLAND 0x0043f890
Person3D* Find3DPersonFromBloke(Bloke* b)
{
    Person3D* p = g_person_head;

    if (p == 0)
        return 0;
    while (p->bloke != b) {
        p = p->next;
        if (p == 0)
            return 0;
    }
    return p;
}

// FUNCTION: LEGOLAND 0x0043ffd0
void SortBlokeIn3D(Bloke* b)
{
    SortInfo  info;
    Person3D* p;

    info.type = 0x306;
    info.bloke = b;
    info.w = 0;
    p = b->person;
    if (p)
        SortPerson(p, p->depth, &info);
}

// FUNCTION: LEGOLAND 0x00440680
void Add3DBlokeToList(Bloke* b, int kind)
{
    Person3D* p = Create3DPerson(b, kind);

    b->person = p;
    if (p) {
        Add3DPersonToList(p);
        UpdatePersonPos(p, b);
        BlokeWalkAnim(b);
    }
}

// FUNCTION: LEGOLAND 0x00443150
char* GetFaceTextureNameOfBloke(Bloke* b)
{
    Person3D* p = b->person;
    char*     list;
    char*     name;

    switch (p->variant) {
    case 0:  list = g_texnames_boy;  break;
    case 1:  list = g_texnames_girl; break;
    }
    name = LookupTextureName(list, 0, p->tex_index);
    _stricmp(name, s_texname_cmp);
    return s_texname_result;
}

// FUNCTION: LEGOLAND 0x004431a0
char* GetChestTextureNameOfBloke(Bloke* b)
{
    Person3D* p = b->person;
    char*     list;
    char*     name;

    switch (p->variant) {
    case 0:  list = g_texnames_boy;  break;
    case 1:  list = g_texnames_girl; break;
    }
    name = LookupTextureName(list, 1, p->tex_index);
    _stricmp(name, s_texname_cmp);
    return s_texname_result;
}

// FUNCTION: LEGOLAND 0x0043f810
void Add3DPersonToList(Person3D* p)
{
    p->prev = 0;
    p->next = 0;
    if (g_person_head) {
        p->next = g_person_head;
        g_person_head->prev = p;
    }
    g_person_head = p;
}

// FUNCTION: LEGOLAND 0x0043f840
void Remove3DPersonFromList(Person3D* p)
{
    if (p->prev)
        p->prev->next = p->next;
    else
        g_person_head = p->next;
    if (p->next)
        p->next->prev = p->prev;
}

// FUNCTION: LEGOLAND 0x0043f8c0
Person3D* Create3DPerson(Bloke* b, int kind)
{
    Person3D* p = (Person3D*)MemAlloc(sizeof(Person3D));

    if (p) {
        memset(p, 0, sizeof(Person3D));
        if (kind == 1)
            p->variant = rand() & 1;
        else
            p->variant = 0;
        p->kind = kind;
        p->bloke = b;
        p->f7c = -1;
        p->tex_index = -1;
        p->anim = -1;
        p->arm_colour = -1;
        p->leg_colour = -1;
        p->scale_x = 2.0f;
        p->scale_y = 2.0f;
        p->scale_z = 2.0f;
        p->prev = 0;
        p->next = 0;
        p->f4c = 0;
        p->f1c = 0;
        p->f20 = 0;
        p->f2c = 0;
        p->f30 = 0;
        p->f34 = 0xff;
        p->f38 = 0;
    }
    return p;
}

// FUNCTION: LEGOLAND 0x00482ef0
Bloke* NewBloke(void)
{
    Bloke* b = 0;
    int    i;

    for (i = 0; i < ((MapEx*)g_map)->max_blokes; i++) {
        if (!(g_bloke_base[i].flags62 & 1)) {
            b = &g_bloke_base[i];
            if (b) {
                memset(b, 0, sizeof(Bloke));
                b->flags62 = 1;
                b->f64 = 0;
                b->next = g_people_head;
                g_people_head = b;
                Add3DBlokeToList(b, 1);
            }
            break;
        }
    }
    return b;
}

#ifdef LEGOLAND_PORTABLE
/* MakeBloke is called with 1 argument(s) the original ignores: the body
 * at this address never reads them, and in cdecl the caller cleans them up.
 * On wasm the argument count is part of the function type, so the exported
 * name is this forwarder and the matched body keeps its own.  */
#undef MakeBloke
Bloke* MakeBloke(int ll_a1) { (void)ll_a1; return MakeBloke_vc6_body(); }
#endif
