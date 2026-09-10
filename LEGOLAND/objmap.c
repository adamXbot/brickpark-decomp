/* LEGOLAND — object classes, object instances and the map render-object walk.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; field and type names are ours.
 */
#include "legoland.h"

void* memset(void*, int, unsigned int);

/* ------------------------------------------------------------------ types -- */

/* The packed {x,y} map coordinate that threads the render chain through the
 * grid (Cell +0x06/+0x07, and the chain head at 0x007febb8). */
typedef union CellRef {
    unsigned short w;
    struct { unsigned char x, y; } b;
} CellRef;

/* An object instance record (0x14 bytes) — CreateObjectInstance allocates it,
 * RemoveInstanceFromList unlinks it from its owner's doubly linked list. */
typedef struct ObjInst {
    struct ObjInst*  next;   /* +0x00 */
    struct ObjInst*  prev;   /* +0x04 */
    struct ObjOwner* owner;  /* +0x08 the class whose list this is on */
    short            f0c;    /* +0x0c */
    short            key;    /* +0x0e */
    int              f10;    /* +0x10 */
} ObjInst;

/* The owning list head: its instance list starts at +0x04. */
typedef struct ObjOwner {
    int      f0;             /* +0x00 */
    ObjInst* head;           /* +0x04 */
} ObjOwner;

/* An object class / definition record (0xd0 bytes, the ODF ObjDef). */
typedef struct ObjDefRec {
    struct ObjDefRec* next;  /* +0x00 */
    int               f4;    /* +0x04 */
    int               f8;    /* +0x08 */
    char              pad0c[0x1c - 0x0c];
    unsigned int      flags; /* +0x1c bit 0x02000000 tested by SetEditObject */
    char              pad20[0xd0 - 0x20];
} ObjDefRec;

/* A placed map object: its class record sits at +0x0c (footprint at +0x3c). */
typedef struct MapObj {
    char      pad0[0x0c];    /* +0x00 */
    ObjClass* cls;           /* +0x0c */
} MapObj;

/* ShuffleObjKeys' list node: next at +0, +0x04 payload, +0x0c/+0x10 map
 * position, +0x14 sort key. */
typedef struct KeyNode {
    struct KeyNode* next;    /* +0x00 */
    void*           obj;     /* +0x04 */
    int             f8;      /* +0x08 */
    int             x;       /* +0x0c */
    int             y;       /* +0x10 */
    int             key;     /* +0x14 */
} KeyNode;

/* Tile slot info (0x00801f40, stride 8): {desc, 16-bit code}. */
typedef struct TileSlot {
    void*          elem;     /* +0x00 */
    unsigned short code;     /* +0x04 */
    unsigned short pad6;     /* +0x06 */
} TileSlot;

/* The .TSF descriptor as AllocTileSpace reads it: codes[] at +0x0c. */
typedef struct TsfDesc {
    char           pad0[0x0c];
    unsigned int*  codes;    /* +0x0c */
} TsfDesc;

/* The map AI state block (0x00832800, 0x3f0 bytes) as ResetMapAI writes it. */
typedef struct MapAI {
    int           f000;          /* +0x000 = 0x1fff */
    char          pad004[0x11c - 0x004];
    int           f11c;          /* +0x11c */
    char          pad120[0x128 - 0x120];
    int           f128;          /* +0x128 = -8000 */
    int           f12c;          /* +0x12c = -1000 */
    int           f130;          /* +0x130 = 100 */
    int           f134;          /* +0x134 = 1000 */
    int           f138;          /* +0x138 = 5000 */
    char          pad13c[0x184 - 0x13c];
    int           f184;          /* +0x184 */
    char          pad188[0x3b0 - 0x188];
    unsigned char b3b0[25];      /* +0x3b0 filled with 2 */
    unsigned char b3c9;          /* +0x3c9 */
    char          pad3ca[0x3f0 - 0x3ca];
} MapAI;

/* ---------------------------------------------------------------- globals -- */

extern CellRef    g_render_head;      /* 0x007febb8 */
extern TileSlot   g_tile_slots[];     /* 0x00801f40 */
extern ObjDefRec* g_objdef_head;      /* 0x00669240 */
extern KeyNode*   g_key_head;         /* 0x00669248 */
extern KeyNode*   g_key_cursor;       /* 0x0066924c */
extern int        g_edit_changed;     /* 0x008119b0 */
extern ObjDefRec* g_edit_object;      /* 0x008119b8 */
extern unsigned int g_ui_flags;       /* 0x00813a40 */

extern char       g_edit_cursor;      /* 0x007febc0 */
extern int        g_cursor_mapref;    /* 0x007fffc4 */
extern Rect       g_edit_footprint;   /* 0x007fffd4 */

extern MapAI      g_map_ai;           /* 0x00832800 */

/* ------------------------------------------------------------ prototypes -- */

extern void* MemAlloc(int size);                         /* 0x0049e4ff */
extern void  GetTileDimensions(int* out_w, int* out_h);  /* 0x00460540 */
extern void  DefaultCursor(void* cursor);                /* 0x0045a390 */
#ifndef LEGOLAND_PORTABLE
extern void  ScreenToMapRef(int sx, void* out, int sy);  /* 0x0045be90 */
#else
extern int ScreenToMapRef(int sx, void* out, int sy);  /* 0x0045be90 */
#endif
extern void  ValidateCursor(void* cursor, ObjClass* cls);/* 0x0045f810 */

Cell* GetFirstRenderObject(void);
Cell* GetNextRenderObject(Cell* c);

/* ---------------------------------------------------------- inline helpers -- */

/* The bounds-checked cell fetch every map accessor open-codes.  Keeping it an
 * inlined helper (rather than writing the chain out) is a codegen lever: the
 * caller's own guards then get a separate `return 0` block and the callee-
 * saved push is deferred to the expansion point (see GetNextRenderObject). */
static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

/* Same check, but copies the cell out by value (rep movsd, 5 dwords) and
 * reads the coordinates lazily through the Pos* (y only after x passed). */
static __inline int CopyMapCell(Pos* pos, Cell* out)
{
    int x = pos->x;
    int y;

    if (x >= 0 && x < g_map->width) {
        y = pos->y;
        if (y >= 0 && y < g_map->height) {
            *out = g_map_rows[y][x];
            return 1;
        }
    }
    return 0;
}

/* -------------------------------------------------------------- functions -- */

/* Head of the render chain.  A zero pair is the chain terminator, so cell
 * (0,0) is only reported when its own flags say it carries an object. */
// FUNCTION: LEGOLAND 0x0045a850
Cell* GetFirstRenderObject(void)
{
    int   x = g_render_head.b.x;
    int   y = g_render_head.b.y;
    Cell* c;

    if (x < 0)               goto fail;
    if (x >= g_map->width)   goto fail;
    if (y < 0)               goto fail;
    if (y >= g_map->height)  goto fail;
    c = &g_map_rows[y][x];
    if (c == 0)              goto fail;
    if (g_render_head.w != 0) return c;
    if (c->flags & 0xa8)     return c;
fail:
    return 0;
}

/* Follow the packed link at Cell+0x06.  The two guards share one `return 0`
 * and the bounds check lives in the inlined MapCellAt: that split is what
 * defers `push esi` past the guards (the guards' `xor eax,eax / ret` has no
 * pop) and gives the bounds failures their own `xor / pop esi / ret`. */
// FUNCTION: LEGOLAND 0x0045a8b0
Cell* GetNextRenderObject(Cell* c)
{
    if (c == 0 || *(unsigned short*)&c->nx == 0)
        return 0;
    {
        int x = c->nx;
        int y = c->ny;
        return MapCellAt(x, y);
    }
}

// FUNCTION: LEGOLAND 0x0045a910
Cell* GetFirstObjectMatching(void* obj)
{
    Cell* c = GetFirstRenderObject();

    while (c) {
        if (c->obj == obj)
            return c;
        c = GetNextRenderObject(c);
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x0045a940
Cell* GetNextObjectMatching(Cell* c, void* obj)
{
    c = GetNextRenderObject(c);
    while (c) {
        if (c->obj == obj)
            return c;
        c = GetNextRenderObject(c);
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x004816e0
void SetEditObject(ObjDefRec* obj)
{
    g_edit_changed = 1;
    g_edit_object = obj;
    if (obj) {
        if (obj->flags & 0x2000000)
            g_ui_flags |= 0x400;
        else
            g_ui_flags &= ~0x400;
    }
}

// FUNCTION: LEGOLAND 0x004017c0
Offset MapToPlayfield(int x, int y)
{
    int    w, h;
    Offset r;

    GetTileDimensions(&w, &h);
    r.ox = ((x - y) * w) >> 9;
    r.oy = ((x + y) * h) >> 9;
    return r;
}

/* Inverse of MapToPlayfield.  Reusing the parameters as the quotients (rather
 * than fresh locals) is what puts qx in ecx and qy in eax for the final lea. */
// FUNCTION: LEGOLAND 0x0045a970
Offset PlayfieldToMap(int x, int y)
{
    int    w, h;
    Offset r;

    GetTileDimensions(&w, &h);
    x = x / w;
    y = y / h;
    r.ox = x + y;
    r.oy = y - x;
    return r;
}

// FUNCTION: LEGOLAND 0x004816a0
ObjInst* CreateObjectInstance(ObjOwner* owner, short* key)
{
    ObjInst* p = (ObjInst*)MemAlloc(0x14);

    if (p) {
        memset(p, 0, 0x14);
        p->key = *key;
        p->owner = owner;
    }
    return p;
}

/* Allocate a zeroed 0xd0-byte class record and push it on the class list.
 * [sic] The original zeroes the block BEFORE testing the allocation for null
 * (the `rep stosd` runs ahead of the `jne`), so an allocation failure writes
 * 0xd0 zero bytes at address 0 before returning 0.  Reproduced faithfully. */
// FUNCTION: LEGOLAND 0x00480990
ObjDefRec* AddNewObjectClass(void)
{
    ObjDefRec* obj = (ObjDefRec*)MemAlloc(0xd0);

    memset(obj, 0, 0xd0);      /* runs even when obj == 0 — original bug */
    if (obj == 0)
        return 0;
    obj->next = g_objdef_head;
    g_objdef_head = obj;
    obj->f4 = 0;
    obj->f8 = 0;
    obj->flags = 0;
    return obj;
}

/* The whole 0x3f0-byte AI block at 0x00832800 is ONE object: the 25-byte fill
 * of 2s at +0x3b0 and the byte at +0x3c9 lie inside the region the first
 * memset clears.  Modelling it as one struct is load-bearing — with separate
 * globals VC6 hoists the second `rep stosd` above the two dword stores. */
// FUNCTION: LEGOLAND 0x00462dd0
void ResetMapAI(void)
{
    memset(&g_map_ai, 0, sizeof(g_map_ai));
    g_map_ai.f11c = 0;
    g_map_ai.f000 = 0x1fff;
    memset(g_map_ai.b3b0, 2, 25);
    g_map_ai.f128 = -8000;
    g_map_ai.f12c = -1000;
    g_map_ai.f130 = 100;
    g_map_ai.f134 = 1000;
    g_map_ai.f138 = 5000;
    g_map_ai.b3c9 = 0;
    g_map_ai.f184 = 0;
}

// FUNCTION: LEGOLAND 0x0048a080
void RemoveInstanceFromList(ObjInst* n)
{
    if (n->prev == 0) {
        n->owner->head = n->next;
        if (n->owner->head)
            n->owner->head->prev = 0;
    } else {
        n->prev->next = n->next;
    }
    if (n->next)
        n->next->prev = n->prev;
    n->next = 0;
    n->prev = 0;
}

// FUNCTION: LEGOLAND 0x0045fa80
void CalcBasicObjectCursor(MapObj* o, int sx, int sy)
{
    ObjClass* cls = o->cls;

    DefaultCursor(&g_edit_cursor);
    ScreenToMapRef(sx, &g_cursor_mapref, sy);
    g_edit_footprint = cls->rect;
    ValidateCursor(&g_edit_cursor, o->cls);
}

/* Bounds-check pos, copy the cell (rep movsd, 5 dwords), hand back the
 * packed base position at Cell+0x04 and return the class of the object the
 * cell carries (flags 0x88).  The copy must come from an inlined helper that
 * takes the Pos* itself: that puts `pos` in eax and x in ecx as the original
 * has them; every open-coded form swaps the two. */
// FUNCTION: LEGOLAND 0x00461850
ObjClass* GetObjectClassAndInstance(Pos* pos, unsigned short* out_bpos)
{
    Cell cell;

    if (!CopyMapCell(pos, &cell))
        return 0;
    if (out_bpos)
        *out_bpos = *(unsigned short*)&cell.bx;
    if (cell.flags & 0x88) {
        MapObj* obj = (MapObj*)cell.obj;
        if (obj)
            return obj->cls;
    }
    return 0;
}

/* One bubble-sort pass per call over the key list, from the head up to the
 * cursor: adjacent nodes whose keys are out of order are swapped in place
 * (`link` is the address of the pointer that reaches `n`).  When the node
 * before the cursor is reached the cursor moves back onto it and its map
 * position (<<8, tile units) and payload are returned; 0 once the cursor has
 * walked all the way back to the head.  The cursor global is re-read every
 * iteration because the swaps store through pointers that may alias it. */
// FUNCTION: LEGOLAND 0x00481610
int ShuffleObjKeys(Pos* out_pos, void** out_obj)
{
    KeyNode*  n    = g_key_head;
    KeyNode** link = &g_key_head;
    KeyNode*  next;

    if (n == g_key_cursor)
        return 0;
    while (n) {
        next = n->next;
        if (next == g_key_cursor) {
            g_key_cursor = n;
            out_pos->x = n->x << 8;
            out_pos->y = n->y << 8;
            *out_obj = n->obj;
            return 1;
        }
        if (n->key > next->key) {
            *link = next;
            n->next = next->next;
            next->next = n;
            link = &next->next;
        } else {
            link = &n->next;
            n = next;
        }
    }
    return 0;
}

/* First-fit a run of n free (-1) slots in g_tile_sprites[] (0x800 slots),
 * zero the run, and fill the parallel g_tile_slots[] with {desc, low 16 bits
 * of desc->codes[i]} (or {0,0} when desc is null).  Returns the run and its
 * base index through *out_base; 0 when no run fits.  `n` must be used as the
 * unsigned short parameter itself — naming an int copy before `base` flips
 * the operand order of the `lea eax,[edx+esi]` end-of-run compare. */
// FUNCTION: LEGOLAND 0x0045a9b0
void* AllocTileSpace(TsfDesc* desc, unsigned short n, unsigned short* out_base)
{
    int      base = 0;
    int      i;
    Sprite** p;

    while (base + n <= 0x800) {
        p = &g_tile_sprites[base];
        i = 0;
        while (*p == (Sprite*)-1) {
            i++;
            p++;
            if (i >= n)
                goto found;
        }
        base += i + 1;
    }
    return 0;

found:
    p = &g_tile_sprites[base];
    memset(p, 0, n * 4);
    for (i = 0; i < n; i++) {
        g_tile_slots[base + i].elem = desc;
        if (desc)
            g_tile_slots[base + i].code = (unsigned short)desc->codes[i];
        else
            g_tile_slots[base + i].code = 0;
    }
    *out_base = (unsigned short)base;
    return p;
}
