/* LEGOLAND — small-leaf sweep (chunk 1).
 *
 * A batch of tiny exported accessors/setters and a few short branch functions,
 * recovered from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct field
 * offsets are the only load-bearing detail; type/field names are ours. */
#include "legoland.h"

/* ---- globals touched (declared once each) ------------------------------- */
extern int   g_vp_left;        /* 0x0081c8d0 */
extern int   g_vp_top;         /* 0x0081c8d4 */
extern int   g_vp_right;       /* 0x0081c8d8 */
extern int   g_vp_bottom;      /* 0x0081c8dc */

extern void* g_render_a;       /* 0x0062feec */
extern int   g_render_b;       /* 0x00655a4c */
extern void* g_render2_a;      /* 0x0062fef0 */
extern int   g_render2_b;      /* 0x00655a50 */

extern int   g_colour_mode;    /* 0x00668088 */
extern unsigned char g_palette_lut[]; /* 0x00814020 */

extern int   g_brick_lock;     /* 0x004b90fc */
extern int   g_bricks;         /* 0x004b90f8 */

extern int   g_controllers;    /* 0x00667104 */
extern int   g_buildcount;     /* 0x006670f8 */

/* ---- local struct layouts (offsets load-bearing only) ------------------- */
typedef struct Person {
    char pad0[0x1c];
    int  x;            /* +0x1c */
    int  y;            /* +0x20 */
} Person;

typedef struct Viewport {
    int left;          /* +0x00 */
    int top;           /* +0x04 */
    int right;         /* +0x08 */
    int bottom;        /* +0x0c */
} Viewport;

typedef struct Bloke {
    int x;             /* +0x00 */
    int y;             /* +0x04 */
} Bloke;

typedef struct BlokeRec {
    char       pad0[4];
    struct BlokeData* data;   /* +0x04 */
} BlokeRec;
typedef struct BlokeData {
    char pad0[0x84];
    int  sex;          /* +0x84 */
} BlokeData;

typedef struct BinNode {
    char pad0[8];
    struct BinNode* next;   /* +0x08 */
} BinNode;
typedef struct Bin {
    char           pad0[2];
    unsigned short count;   /* +0x02 */
    char           pad4[0x1c];
    BinNode*       head;     /* +0x20 */
} Bin;

typedef struct NameNode {
    char pad0[4];
    struct NameNode* next;  /* +0x04 */
    char pad8[4];
    char* name;             /* +0x0c */
} NameNode;
typedef struct NameList {
    int       count;        /* +0x00 */
    NameNode* head;         /* +0x04 */
} NameList;

typedef struct Vertex { char b[20]; } Vertex;   /* 20-byte record */
typedef struct Mesh {
    int     count;          /* +0x00 */
    char    pad4[4];
    Vertex* verts;          /* +0x08 */
} Mesh;

typedef struct Favs {
    char pad0[0x88];
    int  a;            /* +0x88 */
    int  b;            /* +0x8c */
    int  c;            /* +0x90 */
    int  food;         /* +0x94 */
} Favs;

typedef struct Actor {
    char           pad0[8];
    unsigned char  cur_flag;    /* +0x08 */
    char           pad9[1];
    unsigned short saved_act;   /* +0x0a */
    unsigned short cur_act;     /* +0x0c */
    char           pad_e[0x52];
    unsigned char  saved_flag;  /* +0x60 */
} Actor;

typedef struct ListNode {
    struct ListNode* next;   /* +0x00 */
    struct ListNode* back;   /* +0x04 */
} ListNode;
typedef struct Container {
    char      pad0[0xcc];
    ListNode* head;          /* +0xcc */
} Container;

typedef struct BuildEntry {
    int            obj;      /* +0x00 */
    unsigned short type;     /* +0x04 */
    unsigned short pad6;     /* +0x06 */
    int            extra;    /* +0x08 */
} BuildEntry;                /* 12 bytes */
extern BuildEntry g_buildlist[256];   /* 0x006664f8, ends at 0x006670f8 */

extern int   NameCompare(const char* a, const char* b);   /* 0x004aab90 */
extern void* AllocControllers(int size);                  /* 0x0049e4ff */


// FUNCTION: LEGOLAND 0x00440190
void SetPersonPosition(Person* p, int x, int y)
{
    p->x = x;
    p->y = y;
}

// FUNCTION: LEGOLAND 0x00441800
void Render_SetViewport(Viewport* r)
{
    g_vp_left   = r->left;
    g_vp_right  = r->right;
    g_vp_top    = r->top;
    g_vp_bottom = r->bottom;
}

// FUNCTION: LEGOLAND 0x00442d60
void AdjustBlokePosition(Bloke* p)
{
    p->x += -0x4b;
    p->y += -0x4d;
}

// FUNCTION: LEGOLAND 0x00442d80
void UnAdjustBlokePosition(Bloke* p)
{
    p->x += 0x4b;
    p->y += 0x4d;
}

// FUNCTION: LEGOLAND 0x00442e90
void RenderItems_New(void)
{
    g_render_a = (void*)0x630108;
    g_render_b = 0;
}

// FUNCTION: LEGOLAND 0x00443060
void RenderItems2_New(void)
{
    g_render2_a = (void*)0x638218;
    g_render2_b = 0;
}

// FUNCTION: LEGOLAND 0x00443140
int GetSexOfBloke(BlokeRec* p)
{
    return p->data->sex;
}

// FUNCTION: LEGOLAND 0x0044dd70
BinNode* GetBinVFrame(Bin* p, int n)
{
    BinNode* node;
    int count, steps;
    if (!p) return 0;
    node = p->head;
    count = p->count;
    if (n >= count) return 0;
    steps = count - 1;
    if (steps > n) {
        steps -= n;
        do {
            node = node->next;
        } while (--steps);
    }
    return node;
}

// FUNCTION: LEGOLAND 0x0044ddf0
Vertex* GetVertex(Mesh* p, int n)
{
    if (!p) return 0;
    if (n >= p->count) return 0;
    return &p->verts[n];
}

// FUNCTION: LEGOLAND 0x0044e690
int GetTransparentColour(void)
{
    switch (g_colour_mode) {
    case 0: return 0xfe;
    case 1: return 0x3ff;
    case 2: return 0x7ff;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x0044e6c0
int GetNearestColour(int r, int g, int b)
{
    switch (g_colour_mode) {
    case 0:
        return g_palette_lut[((((r & 0xf8) << 5) | (g & 0xf8)) << 2) | ((b >> 3) & 0x1f)];
    case 1:
        return ((((r & 0xf8) << 5) | (g & 0xf8)) << 2) | ((b >> 3) & 0x1f);
    case 2:
        return ((((r & 0xf8) << 5) | (g & 0xfc)) << 3) | ((b >> 3) & 0x1f);
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x0044e830
int IsFavouriteAttraction(Favs* p, int v)
{
    if (p->a == v) return 1;
    if (p->b == v) return 1;
    return p->c == v;
}

// FUNCTION: LEGOLAND 0x0044e870
int IsFavouriteFood(Favs* p, int v)
{
    return p->food == v;
}

// FUNCTION: LEGOLAND 0x0044ebb0
void PushLongTermAction(Actor* p)
{
    unsigned short act = p->cur_act;
    unsigned char  flg = p->saved_flag;
    p->saved_act = act;
    p->cur_flag  = flg;
}

// FUNCTION: LEGOLAND 0x0044ebd0
void PopLongTermAction(Actor* p)
{
    unsigned short act = p->saved_act;
    unsigned char  flg = p->cur_flag;
    p->cur_act    = act;
    p->saved_flag = flg;
}

// FUNCTION: LEGOLAND 0x0044f430
void PutBlokeInList(Container* p, ListNode* item)
{
    ListNode* n;
    if (p->head == 0) {
        p->head = item;
        return;
    }
    n = p->head;
    while (n->next)
        n = n->next;
    n->next = item;
    item->back = n;
}

// FUNCTION: LEGOLAND 0x00450b90
int AddObjectToBuildList(int obj, short type)
{
    int i;
    if (g_buildcount >= 0x100) return 0;
    for (i = 0; i < 256; i++)
        if (g_buildlist[i].obj == 0) break;
    if (i >= 0x100) return 0;
    g_buildlist[i].obj = obj;
    g_buildlist[i].type = (unsigned short)type;
    g_buildlist[i].extra = 0;
    g_buildcount++;
    return 1;
}

// FUNCTION: LEGOLAND 0x00450f10
void ClearBuildObjList(void)
{
    int i;
    for (i = 0; i < 256; i++)
        g_buildlist[i].obj = 0;
}

// FUNCTION: LEGOLAND 0x00453a20
void DBPrintf(void)
{
}

// FUNCTION: LEGOLAND 0x004578a0
void AddBricks(int n)
{
    if (!g_brick_lock)
        g_bricks += n;
}

// FUNCTION: LEGOLAND 0x004578c0
void UseBricks(int n)
{
    if (!g_brick_lock)
        g_bricks -= n;
}

// FUNCTION: LEGOLAND 0x004578e0
int GetBrickCount(void)
{
    return g_brick_lock ? 0x7fffffff : g_bricks;
}
