/* LEGOLAND -- the .sav chunk writers and readers that SaveGame / LoadGame call.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field OFFSETS, record sizes, callee argument counts and global addresses are
 * load-bearing; the names are ours. Types are defined LOCALLY on purpose
 * (legoland.h is owned elsewhere).
 *
 * savegame.c's header has the whole file layout; this file fills in the blocks
 * it names but does not describe. Everything goes through SaveGameWrite /
 * SaveGameRead (saveprof.c) on the raw CRT descriptor, and each of these
 * chunks sits inside its own BeginMeasuredBlock / EndMeasuredBlock frame.
 *
 * The four "BlockN" names savegame.c invented turn out to be the two WORKER
 * subsystems, so they are renamed here:
 *
 *   BLK 5  SaveBlock5 /LoadBlock5   -> SaveGardeners      / LoadGardeners
 *   BLK 6  SaveBlock6 /LoadBlock6   -> SaveMechanics      / LoadMechanics
 *   BLK 7  SaveBlock7 /LoadBlock7   -> SaveGardenerOrders / LoadGardenerOrders
 *   BLK 8  SaveBlock8 /LoadBlock8   -> SaveMechanicOrders / LoadMechanicOrders
 *
 * ==========================================================================
 *  BLK 5 / BLK 6 -- the hired workers  (0x0049c140 / 0x0049c3c0,
 *                                       0x0049c630 / 0x0049c8b0)
 * ==========================================================================
 * Identical code over two lists: gardeners hang off GardenerList (0x79a8a8,
 * count 0x79a8bc) and are "seated" in the POTTING SHED; mechanics hang off
 * MechanicList (0x79a8ac, count 0x79a8cc) and in the MECHANICS HUT.
 *
 * The WRITER first evicts every worker that is currently inside its building
 * (Bloke::w0c == 5), because that state cannot be serialised:
 *     - unlink the worker's SeatSlot from the building ObjDef's +0xcc chain
 *       (RemoveBlokeFromList; the slot node itself is NOT freed -- leak in the
 *       original, reproduced),
 *     - clear Bloke::w62 bits 0x08|0x20,
 *     - a worker whose long-term action is >= 0x64 is deleted outright
 *       (RemoveAGardener / RemoveAMechanic), otherwise its saved target cell
 *       (+0x68) is reset to its current one (+0x24) and it is given action
 *       0x10 (gardener) / 0x11 (mechanic).
 * Only then is the list counted, so the count reflects the post-eviction list.
 *
 *   u32 n_workers
 *   n x u8[0xdc]        WorkerSave, staged in one stack local (see the struct)
 *
 * The record is a hand-picked subset of the 0xac-byte Bloke plus a subset of
 * its 0x94-byte Person3D. Two details matter:
 *   - WorkerSave +0x08 / +0x0c (the two LLIDB element slots the full bloke
 *     record in savegame.c fills with g_elist indices) are NEVER WRITTEN: they
 *     go to disk as whatever was on the stack. The loader reads them straight
 *     back into the same dead slots, so the file round-trips but those eight
 *     bytes are uninitialised stack. Reproduced.
 *   - WorkerSave.blk34[7] (Bloke +0x50, the WorkOrder the worker is holding)
 *     is forced to 0 -- it is a live pointer, and BLK 7 / BLK 8 relink it.
 *
 * The READER allocates each worker with HeapAlloc_w(0xac), appends it to the
 * list, allocates its Person3D with HeapAlloc_w(0x94), pushes that on the
 * 3D-person chain, and finishes with BlokeSetAnim(b, rec.anim) -- the saved
 * Person3D+0x88 is the CURRENT ANIMATION id; the restored person gets +0x88 =
 * -1 and +0x2c / +0x50 = 0 so BlokeSetAnim rebuilds the instance from scratch.
 * The list tail is stored (0x79a8b4 / 0x79a8c4 are unused for workers; the
 * roster loader instead NUL-terminates the chain with `if (b) b->next = 0`).
 * A short read mid-list returns with the chain half-built and the last node's
 * `next` still holding file garbage -- reproduced.
 *
 * ==========================================================================
 *  BLK 7 / BLK 8 -- the work orders  (0x0049cb20 / 0x0049cc10,
 *                                     0x0049cd10 / 0x0049ce00)
 * ==========================================================================
 * The gardener (0x79a8b0, tail 0x79a8b4, count 0x79a8b8) and mechanic
 * (0x79a8c0 / 0x79a8c4 / 0x79a8c8) WorkOrder lists (workorder.c).
 *
 *   u32 n_orders
 *   n x {
 *       u8   order[0x3c]     the WorkOrder verbatim, with TWO fixups:
 *                            +0x1c (the worker that took it) is replaced by
 *                            that worker's INDEX in the gardener/mechanic
 *                            list, and if the worker is not on the list at all
 *                            +0x18 (assigned) is cleared. `next`, `obj` and
 *                            `rects` go to disk as stale pointers and are
 *                            rebuilt by the loader.
 *       u32  namelen         strlen(order->obj->name)   (NO terminator)
 *       char name[namelen]   the LLIDB element name of the order's target
 *       Rect rects[nrects]   nrects x 20 bytes, taken from order->+0x14
 *   }
 *
 * On load each node is HeapAlloc_w(0x3c)'d and chained; obj = ElemID(name);
 * the worker index is walked back to a pointer and the worker's own +0x50 is
 * pointed at the order (closing the loop BLK 5 / BLK 6 deliberately broke);
 * the rect array is HeapAlloc_w(nrects*20)'d and read. The list tail pointer
 * is set from the last node. The last node's `next` is whatever the file held
 * -- correct only because the writer's tail had next == 0.
 * The two readers differ only in statement order (the mechanic one resolves
 * the rects before the name), which is why they are two functions.
 *
 * ==========================================================================
 *  SCRIPTS -- inside BLK 3  (0x0046c920 / 0x0046cb60)
 * ==========================================================================
 * The mission/script state. Both ends stamp g_script_now (0x7fe994) with
 * GetGameTimer() first, so every stored time is relative to it, and both zero
 * the error counter g_script_errors (0x6687a0) -- the serialisers for strings
 * and events report failure by BUMPING that counter as well as returning 0,
 * and LoadScripts polls it instead of testing the return value.
 *
 *   <icon state>            SaveIconStateChunk 0x00474920 / 0x00474970:
 *                           u8[0x10], one byte per interface icon saying
 *                           whether it is enabled (flag 0x400 clear)
 *   u8   text1[0x80]        0x0066861c
 *   u8   text2[0x80]        0x0066869c
 *   u32  n_strings          0x00668720
 *   n x <script string>     SaveScriptString 0x0046c620 / 0x0046c680:
 *                           u32 len (-1 == NULL) then len+1 bytes
 *   u32  elapsed            g_script_now - g_script_start; the loader sets
 *                           g_script_start = g_script_now + elapsed, clears
 *                           0x00668614 and calls ScriptSetRunning(1)
 *   u32  nbytes             always written as 10
 *   u8   bytes[10]          0x007fe930
 *                           The loader is forward-compatible here: a stored
 *                           nbytes > 10 reads 10 bytes and then DISCARDS the
 *                           surplus one byte at a time; nbytes <= 10 runs two
 *                           reset hooks first and reads exactly nbytes.
 *   <event>                 SaveScriptEvent 0x0046c700 / 0x0046c7e0 over the
 *                           pending event 0x00668784: a 0x44-byte record whose
 *                           +0x3c time field is rebased to relative for the
 *                           write and back to absolute afterwards
 *   repeat until id == -1:
 *       u32  id             ScriptStep::id (+0x04)
 *       <event> <event>     ScriptStep +0x0c and +0x10
 *       <script string>     ScriptStep +0x08
 *   u32  -1                 terminator
 *   u32  step_index         (g_script_cur - g_script_steps) + 1, or 0 when no
 *                           step is current. The steps are contiguous 20-byte
 *                           records, so the writer uses pointer subtraction
 *                           and the loader re-derives g_script_cur =
 *                           g_script_steps + (step_index - 1).
 *
 * LoadScripts allocates each step through NewScriptStep(id) (0x0046b4f0) and
 * chains it; a NULL step, or any bump of g_script_errors, aborts with 0.
 *
 * ==========================================================================
 *  MakeAnimInstance  (0x00442580) -- not a chunk, but LoadGame's last step
 * ==========================================================================
 * Builds a person's private, recoloured copy of an animation's part array.
 * `nparts` 36-byte parts are HeapAlloc_w'd and memcpy'd, then:
 *   - kind 1 (a visitor/worker) picks one of two outfit table sets from the
 *     `variant` argument (Person3D +0x84) and lazily rolls the four -1 fields
 *     +0x80 (part A index, mod n), +0x7c (part B index, mod n), +0x8c and
 *     +0x90 (two colour indices, rand() & 7, +0x8c re-rolled while it hits 7),
 *     writing the rolls back so the look is stable across calls;
 *   - kind 3 uses colours (4, 1) and kind 2 uses (0, 3);
 *   - ANY OTHER KIND leaves both colour indices UNINITIALISED. VC6 homed them
 *     in the dead `p` argument slot, so the original indexes the palette with
 *     the Person3D pointer. Reproduced (see the comment in the body).
 * For kind < 2 the two chosen part records are applied over the copy, and
 * finally the two placeholder colours -- 0x565656 and 0x000000 (0x010101 for
 * variant != 0) -- are replaced by the palette entries for the two colour
 * indices, written into the key structs in REVERSE byte order.
 *
 * KNOWN ORIGINAL DEFECTS, reproduced faithfully:
 *  - The evicted worker's SeatSlot node is unlinked but never freed.
 *  - WorkerSave +0x08/+0x0c are written from uninitialised stack.
 *  - MakeAnimInstance reads uninitialised colour indices for unknown kinds.
 *  - Every Save/Load pair here ignores short reads/writes in places where a
 *    partially built list is left behind.
 * ==========================================================================
 */

#include "legoland.h"
#include <string.h>

#pragma intrinsic(strlen, memcpy)

/* ---- local types -------------------------------------------------------- */

typedef struct WinRect {
    long left;     /* +0x00 */
    long top;      /* +0x04 */
    long right;    /* +0x08 */
    long bottom;   /* +0x0c */
} WinRect;

typedef struct WinPoint {
    long x;        /* +0x00 */
    long y;        /* +0x04 */
} WinPoint;

typedef struct DDBltFx DDBltFx;
typedef struct DDSurface DDSurface;
typedef struct DDSurfaceVtbl {
    char pad00[0x14];
    long(__stdcall* Blt)(DDSurface*, WinRect*, DDSurface*, WinRect*,
                         unsigned long, DDBltFx*);                  /* +0x14 */
    char pad18[0x6c - 0x18];
    long(__stdcall* Restore)(DDSurface*);                           /* +0x6c */
} DDSurfaceVtbl;
struct DDSurface { DDSurfaceVtbl* vtbl; };

#define DDERR_SURFACELOST   0x887601c2
#define DDBLT_WAIT          0x01000000

/* An LLS animation record (layervis.c). */
typedef struct LLS LLS;
typedef struct ImageRec { LLS* lls; } ImageRec;
typedef struct SpriteRec {
    struct SpriteRec* next;    /* +0x00 */
    DDSurface*        surface; /* +0x04 */
    ImageRec*         image;   /* +0x08 */
} SpriteRec;

__declspec(dllimport) int __stdcall ClientToScreen(void* hwnd, WinPoint* pt); /* [0x4ab2dc] */

extern int   GetTicks(void);                                    /* 0x00499450 */
extern void  LLSAdvanceFrame(LLS* lls);                         /* 0x0047d610 */
extern void  PushRenderingStatusAndLockVideoSurface(void);      /* 0x00463fc0 */
extern void  PopRenderingStatus(void);                          /* 0x004641f0 */
extern int   PrintSprite(SpriteRec* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern void* WNDENV_Gethwnd(void);                              /* 0x0047fe60 */

extern int        g_progress_active;   /* 0x00668204 */
extern SpriteRec* g_progress_sprite;   /* 0x00668208 */
extern int        g_progress_last;     /* 0x00667d60 */
extern WinRect    g_progress_rect;     /* 0x007fea30 */
extern DDSurface* g_primary;           /* 0x00668070 */
extern DDSurface* g_surface_78;        /* 0x00668078 */

// FUNCTION: LEGOLAND 0x004663f0
void progress_tick(void)
{
    WinRect rc;

    if (!g_progress_active)
        return;
    if (GetTicks() - g_progress_last <= 200)
        return;

    rc = g_progress_rect;
    g_progress_last = GetTicks();
    LLSAdvanceFrame(g_progress_sprite->image->lls);
    PushRenderingStatusAndLockVideoSurface();
    PrintSprite(g_progress_sprite, g_progress_rect.left, g_progress_rect.top, 0, 0);
    PopRenderingStatus();
    ClientToScreen(WNDENV_Gethwnd(), (WinPoint*)&rc.left);
    ClientToScreen(WNDENV_Gethwnd(), (WinPoint*)&rc.right);
    if (g_primary->vtbl->Blt(g_primary, &rc, g_surface_78, &g_progress_rect,
                             DDBLT_WAIT, 0) == DDERR_SURFACELOST) {
        g_primary->vtbl->Restore(g_primary);
        g_primary->vtbl->Blt(g_primary, &rc, g_surface_78, 0, DDBLT_WAIT, 0);
    }
}

/* ---- work orders (workorder.c) ------------------------------------------ */

typedef struct Vec3i { int a, b, c; } Vec3i;
typedef struct Vec2i { int a, b; } Vec2i;

/* The 3D person record hanging off a bloke (savegame.c), 0x94 bytes. Only the
 * fields the gardener/mechanic chunk touches are named. */
typedef struct Person3D {
    struct Person3D* prev;        /* +0x00 */
    struct Person3D* next;        /* +0x04 */
    int              kind;        /* +0x08 */
    struct Bloke*    bloke;       /* +0x0c */
    Vec3i            v10;         /* +0x10 */
    Vec2i            v1c;         /* +0x1c */
    Vec2i            v24;         /* +0x24 */
    int              f2c;         /* +0x2c */
    int              f30;         /* +0x30 */
    int              f34;         /* +0x34 */
    int              f38;         /* +0x38 */
    int              f3c;         /* +0x3c */
    Vec3i            v40;         /* +0x40 */
    int              f4c;         /* +0x4c */
    int              f50;         /* +0x50 */
    int              f54;         /* +0x54 */
    int              blk58[9];    /* +0x58..0x7b */
    int              f7c;         /* +0x7c */
    int              f80;         /* +0x80 */
    int              f84;         /* +0x84 */
    int              f88;         /* +0x88 */
    int              f8c;         /* +0x8c */
    int              f90;         /* +0x90 */
} Person3D;                       /* 0xac stride in the pool, 0x94 used */

/* A bloke (person/worker); stride 172 (0xac). Same layout savegame.c
 * recovered. blk34[7] (+0x50) is the WorkOrder the worker currently holds and
 * blk34[8] (+0x54) its BNV path. */
typedef struct Bloke {
    struct Bloke*  next;          /* +0x00 */
    Person3D*      person;        /* +0x04 */
    unsigned char  b08;           /* +0x08 */
    unsigned char  pad09;         /* +0x09 */
    unsigned short w0a;           /* +0x0a */
    unsigned short w0c;           /* +0x0c */
    unsigned short w0e;           /* +0x0e */
    unsigned short w10;           /* +0x10 */
    unsigned char  pad12[2];      /* +0x12 */
    LLElem*        e14;           /* +0x14 */
    LLElem*        e18;           /* +0x18 */
    int            f1c, f20;      /* +0x1c, +0x20 */
    Pos            p24;           /* +0x24, +0x28  current target cell */
    int            f2c, f30;      /* +0x2c, +0x30 */
    int            blk34[10];     /* +0x34..0x5b */
    int            f5c;           /* +0x5c */
    unsigned char  b60;           /* +0x60  long-term action code */
    unsigned char  pad61;         /* +0x61 */
    unsigned short w62;           /* +0x62  flags */
    unsigned char  b64;           /* +0x64 */
    unsigned char  pad65[3];      /* +0x65 */
    Pos            p68;           /* +0x68, +0x6c  saved target cell */
    unsigned short w70;           /* +0x70 */
    unsigned char  b72, b73, b74, b75;  /* +0x72..0x75 */
    unsigned char  pad76[2];      /* +0x76 */
    unsigned short w78;           /* +0x78 */
    unsigned short w7a;           /* +0x7a */
    unsigned short w7c;           /* +0x7c */
    unsigned char  b7e, b7f, b80, b81, b82;  /* +0x7e..0x82 */
    unsigned char  pad83[5];      /* +0x83 */
    LLElem*        e88;           /* +0x88 */
    LLElem*        e8c;           /* +0x8c */
    LLElem*        e90;           /* +0x90 */
    LLElem*        e94;           /* +0x94 */
    int            blk98[5];      /* +0x98..0xab */
} Bloke;                          /* 0xac */

/* One seat/queue slot on a ride or building (blokelist.c); 20 bytes. */
typedef struct SeatSlot {
    struct SeatSlot* next;   /* +0x00 */
    struct SeatSlot* prev;   /* +0x04 */
    Bloke*           bloke;  /* +0x08 */
    unsigned short   seat;   /* +0x0c */
    unsigned short   pad0e;  /* +0x0e */
    void*            owner;  /* +0x10 */
} SeatSlot;

/* The ObjDef that owns a seat-slot list; the head is at +0xcc. */
typedef struct SeatOwner {
    unsigned char pad00[0xcc]; /* +0x00..0xcb */
    SeatSlot*     head;        /* +0xcc */
} SeatOwner;

typedef struct WorkOrder {
    struct WorkOrder* next;      /* +0x00 */
    LLElem*           obj;       /* +0x04 target element (name at +0x00) */
    Pos               pos;       /* +0x08 */
    Rect*             rects;     /* +0x10 */
    int               nrects;    /* +0x14 */
    int               assigned;  /* +0x18 */
    Bloke*            worker;    /* +0x1c */
    unsigned char     kind;      /* +0x20 */
    unsigned char     pad21[3];
    int               ox;        /* +0x24 */
    int               oy;        /* +0x28 */
    int               f2c;       /* +0x2c */
    int               f30;       /* +0x30 */
    int               pad34[2];  /* +0x34 */
} WorkOrder;

extern int   SaveGameRead(void* buf, unsigned int n);         /* 0x0047d730 */
extern int   SaveGameWrite(const void* buf, unsigned int n);  /* 0x0047d760 */
extern void* HeapAlloc_w(unsigned int size);                  /* 0x0049e4ff */
extern LLElem* ElemID(const char* name);                      /* 0x0047b3f0 */

extern WorkOrder* g_gardener_orders;       /* 0x0079a8b0 */
extern WorkOrder* g_gardener_orders_tail;  /* 0x0079a8b4 */
extern int        g_gardener_order_count;  /* 0x0079a8b8 */
extern Bloke*     g_gardener_list;         /* 0x0079a8a8 */

// FUNCTION: LEGOLAND 0x0049cb20
void SaveGardenerOrders(void)
{
    int        n;
    int        len;
    WorkOrder  rec;
    WorkOrder* o;

    n = 0;
    for (o = g_gardener_orders; o; o = o->next)
        n++;
    SaveGameWrite(&n, 4);

    for (o = g_gardener_orders; o; o = o->next) {
        rec = *o;
        if (rec.assigned) {
            Bloke* p = g_gardener_list;
            int    i = 0;
            while (p != 0 && p != rec.worker) {
                p = p->next;
                i++;
            }
            if (p == 0)
                rec.assigned = 0;
            else
                rec.worker = (Bloke*)i;
        }
        SaveGameWrite(&rec, 0x3c);
        len = strlen(rec.obj->name);
        SaveGameWrite(&len, 4);
        SaveGameWrite(rec.obj->name, len);
        SaveGameWrite(rec.rects, rec.nrects * 20);
    }
}

// FUNCTION: LEGOLAND 0x0049cc10
void LoadGardenerOrders(void)
{
    int        n;
    int        len;
    char       name[0x200];
    WorkOrder* o = 0;

    g_gardener_orders = 0;
    SaveGameRead(&n, 4);
    g_gardener_order_count = n;
    while (n-- != 0) {
        if (o) {
            o->next = (WorkOrder*)HeapAlloc_w(0x3c);
            o = o->next;
        } else {
            o = (WorkOrder*)HeapAlloc_w(0x3c);
            g_gardener_orders = o;
        }
        SaveGameRead(o, 0x3c);
        SaveGameRead(&len, 4);
        SaveGameRead(name, len);
        name[len] = 0;
        o->obj = ElemID(name);
        if (o->assigned) {
            unsigned int i = (unsigned int)o->worker;
            Bloke*       p = g_gardener_list;
            while (i-- != 0)
                p = p->next;
            o->worker = p;
            p->blk34[7] = (int)o;
        }
        o->rects = (Rect*)HeapAlloc_w(o->nrects * 20);
        SaveGameRead(o->rects, o->nrects * 20);
    }
    g_gardener_orders_tail = o;
}

extern WorkOrder* g_mechanic_orders;       /* 0x0079a8c0 */
extern WorkOrder* g_mechanic_orders_tail;  /* 0x0079a8c4 */
extern int        g_mechanic_order_count;  /* 0x0079a8c8 */
extern Bloke*     g_mechanic_list;         /* 0x0079a8ac */

// FUNCTION: LEGOLAND 0x0049cd10
void SaveMechanicOrders(void)
{
    int        n;
    int        len;
    WorkOrder  rec;
    WorkOrder* o;

    n = 0;
    for (o = g_mechanic_orders; o; o = o->next)
        n++;
    SaveGameWrite(&n, 4);

    for (o = g_mechanic_orders; o; o = o->next) {
        rec = *o;
        if (rec.assigned) {
            Bloke* p = g_mechanic_list;
            int    i = 0;
            while (p != 0 && p != rec.worker) {
                p = p->next;
                i++;
            }
            if (p == 0)
                rec.assigned = 0;
            else
                rec.worker = (Bloke*)i;
        }
        SaveGameWrite(&rec, 0x3c);
        len = strlen(rec.obj->name);
        SaveGameWrite(&len, 4);
        SaveGameWrite(rec.obj->name, len);
        SaveGameWrite(rec.rects, rec.nrects * 20);
    }
}

// FUNCTION: LEGOLAND 0x0049ce00
void LoadMechanicOrders(void)
{
    int        n;
    int        len;
    char       name[0x200];
    WorkOrder* o = 0;

    g_mechanic_orders = 0;
    SaveGameRead(&n, 4);
    g_mechanic_order_count = n;
    while (n-- != 0) {
        if (o) {
            o->next = (WorkOrder*)HeapAlloc_w(0x3c);
            o = o->next;
        } else {
            o = (WorkOrder*)HeapAlloc_w(0x3c);
            g_mechanic_orders = o;
        }
        SaveGameRead(o, 0x3c);
        SaveGameRead(&len, 4);
        SaveGameRead(name, len);
        name[len] = 0;
        o->rects = (Rect*)HeapAlloc_w(o->nrects * 20);
        SaveGameRead(o->rects, o->nrects * 20);
        o->obj = ElemID(name);
        if (o->assigned) {
            unsigned int i = (unsigned int)o->worker;
            Bloke*       p = g_mechanic_list;
            while (i-- != 0)
                p = p->next;
            o->worker = p;
            p->blk34[7] = (int)o;
        }
    }
    g_mechanic_orders_tail = o;
}

/* ---- worker roster chunks (blocks 5 and 6) ------------------------------- */

/* The flattened gardener/mechanic record that hits the file: 0xdc bytes staged
 * in one stack local. NOTE the two LLIDB slots at +0x08/+0x0c are NEVER
 * written by the saver (see SaveGardeners) -- they go to disk as stack
 * garbage, and the loader reads them back into the same dead slots. */
typedef struct WorkerSave {
    unsigned short w0c;        /* +0x00 */
    unsigned short w0e;        /* +0x02 */
    unsigned short w10;        /* +0x04 */
    unsigned char  pad06[2];   /* +0x06 */
    int            e14;        /* +0x08  never written */
    int            e18;        /* +0x0c  never written */
    int            f1c;        /* +0x10 */
    int            f20;        /* +0x14 */
    int            f24;        /* +0x18 */
    int            f28;        /* +0x1c */
    int            f2c;        /* +0x20 */
    int            f30;        /* +0x24 */
    int            blk34[10];  /* +0x28..0x4f  ([7] = the live WorkOrder: zeroed) */
    int            f5c;        /* +0x50 */
    unsigned char  b60;        /* +0x54 */
    unsigned char  pad55;      /* +0x55 */
    unsigned short w62;        /* +0x56 */
    unsigned char  b64;        /* +0x58 */
    unsigned char  b7f;        /* +0x59 */
    unsigned char  b82;        /* +0x5a */
    unsigned char  pad5b;      /* +0x5b */
    int            f68;        /* +0x5c */
    int            f6c;        /* +0x60 */
    unsigned short w70;        /* +0x64 */
    unsigned char  b72;        /* +0x66 */
    unsigned char  b73;        /* +0x67 */
    unsigned char  b74;        /* +0x68 */
    unsigned char  b75;        /* +0x69 */
    unsigned char  pad6a[2];   /* +0x6a */
    int            blk98[5];   /* +0x6c..0x7f */
    int            p08;        /* +0x80 */
    Vec3i          s10;        /* +0x84 */
    Vec2i          s1c;        /* +0x90 */
    Vec3i          s40;        /* +0x98 */
    int            p4c;        /* +0xa4 */
    int            p88;        /* +0xa8 */
    int            p54;        /* +0xac */
    int            blk58[9];   /* +0xb0..0xd3 */
    int            w0a;        /* +0xd4  widened from the u16 field */
    int            b08;        /* +0xd8  widened from the u8 field */
} WorkerSave;                  /* 0xdc */

extern void RemoveBlokeFromList(SeatOwner* owner, SeatSlot* slot); /* 0x0044f470 */
extern void RemoveAGardener(Bloke* g);                             /* 0x0049a2d0 */
extern void NewLongTermAction(Bloke* b, int action);               /* 0x0044e760 */

extern const char kPottingShed[];   /* 0x004b89ac "POTTING SHED" */

// FUNCTION: LEGOLAND 0x0049c140
void SaveGardeners(void)
{
    int        n;
    WorkerSave rec;
    SeatOwner* shed;
    Bloke*     b;
    Bloke*     next;

    shed = (SeatOwner*)ElemID(kPottingShed)->data;
    n = 0;
    b = g_gardener_list;
    while (b) {
        next = b->next;
        if (b->w0c == 5) {
            SeatSlot* s = shed->head;
            while (s) {
                if (s->bloke == b) {
                    RemoveBlokeFromList(shed, s);
                    break;
                }
                s = s->next;
            }
            b->w62 &= 0xffd7;
            if (b->b60 >= 0x64) {
                RemoveAGardener(b);
            } else {
                b->p68 = b->p24;
                NewLongTermAction(b, 0x10);
            }
        }
        b = next;
    }
    for (b = g_gardener_list; b; b = b->next)
        n++;
    if (SaveGameWrite(&n, 4)) {
        for (b = g_gardener_list; b; b = b->next) {
            rec.w0c = b->w0c;
            rec.w0e = b->w0e;
            rec.w10 = b->w10;
            rec.f1c = b->f1c;
            rec.f20 = b->f20;
            rec.f24 = b->p24.x;
            rec.f28 = b->p24.y;
            rec.f2c = b->f2c;
            rec.f30 = b->f30;
            memcpy(rec.blk34, b->blk34, sizeof(rec.blk34));
            rec.f5c = b->f5c;
            rec.b60 = b->b60;
            rec.w62 = b->w62;
            rec.b64 = b->b64;
            rec.b7f = b->b7f;
            rec.b82 = b->b82;
            rec.f68 = b->p68.x;
            rec.f6c = b->p68.y;
            rec.w70 = b->w70;
            rec.b72 = b->b72;
            rec.b73 = b->b73;
            rec.b74 = b->b74;
            rec.b75 = b->b75;
            memcpy(rec.blk98, b->blk98, sizeof(rec.blk98));
            rec.p08 = b->person->kind;
            rec.s10 = b->person->v10;
            rec.s1c = b->person->v1c;
            rec.s40 = b->person->v40;
            rec.p4c = b->person->f4c;
            rec.p88 = b->person->f88;
            rec.p54 = b->person->f54;
            memcpy(rec.blk58, b->person->blk58, sizeof(rec.blk58));
            rec.w0a = b->w0a;
            rec.b08 = b->b08;
            rec.blk34[7] = 0;
            if (!SaveGameWrite(&rec, 0xdc))
                break;
        }
    }
}

extern void Add3DPersonToList(Person3D* p);   /* 0x0043f810 */
extern void BlokeSetAnim(Bloke* b, int anim); /* 0x004406c0 */

extern int g_gardener_count;                  /* 0x0079a8bc */

// FUNCTION: LEGOLAND 0x0049c3c0
void LoadGardeners(void)
{
    int        n = 0;
    WorkerSave rec;
    Bloke*     b = 0;

    if (!SaveGameRead(&n, 4))
        return;
    g_gardener_list = 0;
    g_gardener_count = n;
    while (n-- != 0) {
        if (b != 0) {
            b->next = (Bloke*)HeapAlloc_w(0xac);
            b = b->next;
        } else {
            g_gardener_list = (Bloke*)HeapAlloc_w(0xac);
            b = g_gardener_list;
        }
        if (!SaveGameRead(&rec, 0xdc))
            return;
        b->w0c = rec.w0c;
        b->w0e = rec.w0e;
        b->w10 = rec.w10;
        b->f1c = rec.f1c;
        b->f20 = rec.f20;
        b->p24.x = rec.f24;
        b->p24.y = rec.f28;
        b->f2c = rec.f2c;
        b->f30 = rec.f30;
        memcpy(b->blk34, rec.blk34, sizeof(b->blk34));
        b->f5c = rec.f5c;
        b->b60 = rec.b60;
        b->w62 = rec.w62;
        b->b64 = rec.b64;
        b->b7f = rec.b7f;
        b->b82 = rec.b82;
        b->p68.x = rec.f68;
        b->p68.y = rec.f6c;
        b->w70 = rec.w70;
        b->b72 = rec.b72;
        b->b73 = rec.b73;
        b->b74 = rec.b74;
        b->b75 = rec.b75;
        memcpy(b->blk98, rec.blk98, sizeof(b->blk98));
        b->person = (Person3D*)HeapAlloc_w(0x94);
        Add3DPersonToList(b->person);
        b->person->bloke = b;
        b->person->kind = rec.p08;
        b->person->v10 = rec.s10;
        b->person->v1c = rec.s1c;
        b->person->v40 = rec.s40;
        b->person->f4c = rec.p4c;
        b->person->f88 = -1;
        b->person->f54 = rec.p54;
        memcpy(b->person->blk58, rec.blk58, sizeof(b->person->blk58));
        b->w0a = (unsigned short)rec.w0a;
        b->b08 = (unsigned char)rec.b08;
        b->person->f2c = 0;
        b->person->f50 = 0;
        BlokeSetAnim(b, rec.p88);
    }
    if (b)
        b->next = 0;
}

extern void RemoveAMechanic(Bloke* m);        /* 0x0049a430 */
extern int  g_mechanic_count;                 /* 0x0079a8cc */

extern const char kMechanicsHut[];  /* 0x004b899c "MECHANICS HUT" */

// FUNCTION: LEGOLAND 0x0049c630
void SaveMechanics(void)
{
    int        n;
    WorkerSave rec;
    SeatOwner* hut;
    Bloke*     b;
    Bloke*     next;

    hut = (SeatOwner*)ElemID(kMechanicsHut)->data;
    n = 0;
    b = g_mechanic_list;
    while (b) {
        next = b->next;
        if (b->w0c == 5) {
            SeatSlot* s = hut->head;
            while (s) {
                if (s->bloke == b) {
                    RemoveBlokeFromList(hut, s);
                    break;
                }
                s = s->next;
            }
            b->w62 &= 0xffd7;
            if (b->b60 >= 0x64) {
                RemoveAMechanic(b);
            } else {
                b->p68 = b->p24;
                NewLongTermAction(b, 0x11);
            }
        }
        b = next;
    }
    for (b = g_mechanic_list; b; b = b->next)
        n++;
    if (SaveGameWrite(&n, 4)) {
        for (b = g_mechanic_list; b; b = b->next) {
            rec.w0c = b->w0c;
            rec.w0e = b->w0e;
            rec.w10 = b->w10;
            rec.f1c = b->f1c;
            rec.f20 = b->f20;
            rec.f24 = b->p24.x;
            rec.f28 = b->p24.y;
            rec.f2c = b->f2c;
            rec.f30 = b->f30;
            memcpy(rec.blk34, b->blk34, sizeof(rec.blk34));
            rec.f5c = b->f5c;
            rec.b60 = b->b60;
            rec.w62 = b->w62;
            rec.b64 = b->b64;
            rec.b7f = b->b7f;
            rec.b82 = b->b82;
            rec.f68 = b->p68.x;
            rec.f6c = b->p68.y;
            rec.w70 = b->w70;
            rec.b72 = b->b72;
            rec.b73 = b->b73;
            rec.b74 = b->b74;
            rec.b75 = b->b75;
            memcpy(rec.blk98, b->blk98, sizeof(rec.blk98));
            rec.p08 = b->person->kind;
            rec.s10 = b->person->v10;
            rec.s1c = b->person->v1c;
            rec.s40 = b->person->v40;
            rec.p4c = b->person->f4c;
            rec.p88 = b->person->f88;
            rec.p54 = b->person->f54;
            memcpy(rec.blk58, b->person->blk58, sizeof(rec.blk58));
            rec.w0a = b->w0a;
            rec.b08 = b->b08;
            rec.blk34[7] = 0;
            if (!SaveGameWrite(&rec, 0xdc))
                break;
        }
    }
}

// FUNCTION: LEGOLAND 0x0049c8b0
void LoadMechanics(void)
{
    int        n = 0;
    WorkerSave rec;
    Bloke*     b = 0;

    if (!SaveGameRead(&n, 4))
        return;
    g_mechanic_list = 0;
    g_mechanic_count = n;
    while (n-- != 0) {
        if (b != 0) {
            b->next = (Bloke*)HeapAlloc_w(0xac);
            b = b->next;
        } else {
            g_mechanic_list = (Bloke*)HeapAlloc_w(0xac);
            b = g_mechanic_list;
        }
        if (!SaveGameRead(&rec, 0xdc))
            return;
        b->w0c = rec.w0c;
        b->w0e = rec.w0e;
        b->w10 = rec.w10;
        b->f1c = rec.f1c;
        b->f20 = rec.f20;
        b->p24.x = rec.f24;
        b->p24.y = rec.f28;
        b->f2c = rec.f2c;
        b->f30 = rec.f30;
        memcpy(b->blk34, rec.blk34, sizeof(b->blk34));
        b->f5c = rec.f5c;
        b->b60 = rec.b60;
        b->w62 = rec.w62;
        b->b64 = rec.b64;
        b->b7f = rec.b7f;
        b->b82 = rec.b82;
        b->p68.x = rec.f68;
        b->p68.y = rec.f6c;
        b->w70 = rec.w70;
        b->b72 = rec.b72;
        b->b73 = rec.b73;
        b->b74 = rec.b74;
        b->b75 = rec.b75;
        memcpy(b->blk98, rec.blk98, sizeof(b->blk98));
        b->person = (Person3D*)HeapAlloc_w(0x94);
        Add3DPersonToList(b->person);
        b->person->bloke = b;
        b->person->kind = rec.p08;
        b->person->v10 = rec.s10;
        b->person->v1c = rec.s1c;
        b->person->v40 = rec.s40;
        b->person->f4c = rec.p4c;
        b->person->f88 = -1;
        b->person->f54 = rec.p54;
        memcpy(b->person->blk58, rec.blk58, sizeof(b->person->blk58));
        b->w0a = (unsigned short)rec.w0a;
        b->b08 = (unsigned char)rec.b08;
        b->person->f2c = 0;
        b->person->f50 = 0;
        BlokeSetAnim(b, rec.p88);
    }
    if (b)
        b->next = 0;
}

/* ---- the script / mission chunk (block 3, inside world state) ------------ */

/* One scripted step; the array at g_script_steps is chained through +0x00 and
 * indexed by the tail pointer, so the nodes are contiguous 20-byte records. */
typedef struct ScriptStep {
    struct ScriptStep* next;   /* +0x00 */
    int                id;     /* +0x04  step id, -1 terminates the file list */
    char*              text;   /* +0x08  saved with SaveScriptString */
    void*              ev0c;   /* +0x0c  0x44-byte event record */
    void*              ev10;   /* +0x10  0x44-byte event record */
} ScriptStep;                  /* 0x14 */

/* Every script serialiser reports failure by BUMPING this counter as well as
 * returning 0; LoadScripts polls it after each helper instead of testing the
 * return value. */
extern int         g_script_errors;        /* 0x006687a0 */
extern int         g_script_now;           /* 0x007fe994  GetGameTimer() at save/load */
extern char        g_script_text1[];       /* 0x0066861c  0x80 bytes */
extern char        g_script_text2[];       /* 0x0066869c  0x80 bytes */
extern int         g_script_string_count;  /* 0x00668720 */
extern char*       g_script_strings[];     /* 0x007fe120 */
extern int         g_script_start;         /* 0x00668780  absolute start time */
extern void*       g_script_event;         /* 0x00668784  the pending event */
extern char        g_script_bytes[];       /* 0x007fe930  10 bytes */
extern ScriptStep* g_script_steps;         /* 0x00668798  first step */
extern ScriptStep* g_script_cur;           /* 0x0066879c  step we are on */
extern int         g_script_614;           /* 0x00668614 */

extern int   GetGameTimer(void);                  /* 0x00499430 */
extern int   SaveIconStateChunk(void);            /* 0x00474920 */
extern int   LoadIconStateChunk(void);            /* 0x00474970 */
extern int   SaveScriptString(const char* s);     /* 0x0046c620 */
extern char* LoadScriptString(void);              /* 0x0046c680 */
extern int   SaveScriptEvent(void* ev);           /* 0x0046c700 */
extern void* LoadScriptEvent(void);               /* 0x0046c7e0 */
extern ScriptStep* NewScriptStep(int id);         /* 0x0046b4f0 */
extern void  ScriptSetRunning(int on);            /* 0x004748a0 */
extern void  sub_468840(void);                    /* 0x00468840 */
extern void  sub_4688e0(void);                    /* 0x004688e0 */

/* 0x0046c920 -- SaveScripts. Every instruction is right (166 of 166, same
 * opcodes, same operands, same order within each block) EXCEPT for where VC6
 * parks the ONE shared `return 0` block that the two loops branch to.
 *
 * VC6 emits a `return 0` as `xor eax,eax` + epilogue, deletes the `xor` at any
 * site it reaches by fall-through with eax already zero, and keeps exactly one
 * full copy for the sites that need a branch. Measured on this toolchain (see
 * scratchpad/savechunks): that surviving copy is ALWAYS placed at the LAST
 * `return 0` statement in the function -- verified on six probe shapes, and it
 * is what makes LoadScripts below match exactly. The original places it at the
 * FIRST guard instead (0x0046c941, reached by `je`/`jne` from all five loop
 * failures), which no arrangement of `return 0` / `goto fail` / nesting I could
 * find reproduces: a label sinks the block to just before the final block, and
 * nesting the body merges all nine inline copies away.
 *
 * So ours has the `xor eax,eax` at the last guard and the original has it at
 * the first; every other instruction lines up, and the 12-byte length gap is
 * the four short-vs-near loop branches that follow from it. Left WIP rather
 * than claimed. */
// WIP-FUNCTION: LEGOLAND 0x0046c920  (166 insns, one displaced `xor eax,eax`)
int SaveScripts(void)
{
    int         n;   /* the string loop counter, then the 10-byte block size */
    ScriptStep* s;

    g_script_errors = 0;
    g_script_now = GetGameTimer();
    if (!SaveIconStateChunk())
        return 0;
    if (!SaveGameWrite(g_script_text1, 0x80))
        return 0;
    if (!SaveGameWrite(g_script_text2, 0x80))
        return 0;
    if (!SaveGameWrite(&g_script_string_count, 4))
        return 0;
    for (n = 0; n < g_script_string_count; n++) {
        if (!SaveScriptString(g_script_strings[n]))
            return 0;
    }
    {
        int v = g_script_now - g_script_start;
        if (!SaveGameWrite(&v, 4))
            return 0;
    }
    n = 10;
    if (!SaveGameWrite(&n, 4))
        return 0;
    if (!SaveGameWrite(g_script_bytes, 10))
        return 0;
    if (!SaveScriptEvent(g_script_event))
        return 0;
    for (s = g_script_steps; s; s = s->next) {
        if (!SaveGameWrite(&s->id, 4))
            return 0;
        if (!SaveScriptEvent(s->ev0c))
            return 0;
        if (!SaveScriptEvent(s->ev10))
            return 0;
        if (!SaveScriptString(s->text))
            return 0;
    }
    {
        int v = -1;
        if (!SaveGameWrite(&v, 4))
            return 0;
    }
    {
        int v;
        if (g_script_cur)
            v = (g_script_cur - g_script_steps) + 1;
        else
            v = 0;
        return SaveGameWrite(&v, 4) != 0;
    }
}

// FUNCTION: LEGOLAND 0x0046cb60
int LoadScripts(void)
{
    char        c;
    int         i;
    int         id;
    int         v;
    ScriptStep* s;
    ScriptStep* prev;

    g_script_errors = 0;
    g_script_now = GetGameTimer();
    if (g_script_errors)
        return 0;
    if (!LoadIconStateChunk())
        return 0;
    if (!SaveGameRead(g_script_text1, 0x80))
        return 0;
    if (!SaveGameRead(g_script_text2, 0x80))
        return 0;
    if (!SaveGameRead(&g_script_string_count, 4))
        return 0;
    for (i = 0; i < g_script_string_count; i++) {
        g_script_strings[i] = LoadScriptString();
        if (g_script_errors)
            return 0;
    }
    if (!SaveGameRead(&v, 4))
        return 0;
    g_script_start = g_script_now + v;
    g_script_614 = 0;
    ScriptSetRunning(1);
    if (!SaveGameRead(&i, 4))
        return 0;
    if (i > 10) {
        if (!SaveGameRead(g_script_bytes, 10))
            return 0;
        i -= 10;
        while (i != 0) {
            if (!SaveGameRead(&c, 1))
                return 0;
            i--;
        }
    } else {
        sub_468840();
        sub_4688e0();
        if (!SaveGameRead(g_script_bytes, i))
            return 0;
    }
    g_script_event = LoadScriptEvent();
    if (g_script_errors)
        return 0;
    prev = 0;
    if (!SaveGameRead(&id, 4))
        return 0;
    while (id != -1) {
        s = NewScriptStep(id);
        if (s == 0)
            return 0;
        s->ev0c = LoadScriptEvent();
        if (g_script_errors)
            return 0;
        s->ev10 = LoadScriptEvent();
        if (g_script_errors)
            return 0;
        s->text = LoadScriptString();
        if (g_script_errors)
            return 0;
        if (prev)
            prev->next = s;
        else
            g_script_steps = s;
        if (!SaveGameRead(&id, 4))
            return 0;
        prev = s;
    }
    if (!SaveGameRead(&v, 4))
        return 0;
    if (v)
        g_script_cur = g_script_steps + (v - 1);
    else
        g_script_cur = 0;
    return 1;
}

/* ---- 3D person model instancing (0x00442580) ---------------------------- */

/* One entry of the bloke colour palette at 0x004b7ac0 (blokelist.c). */
typedef struct BlokeColour {
    unsigned char r;   /* +0x00 */
    unsigned char g;   /* +0x01 */
    unsigned char b;   /* +0x02 */
} BlokeColour;

/* A colour key/replacement as the recolour pass wants it: three bytes in the
 * REVERSE order of the palette entry. */
typedef struct Colour3 {
    unsigned char c0;  /* +0x00  gets the palette's b */
    unsigned char c1;  /* +0x01  gets the palette's g */
    unsigned char c2;  /* +0x02  gets the palette's r */
} Colour3;

extern BlokeColour g_bloke_palette[];   /* 0x004b7ac0 */
extern int   rand(void);                /* 0x0049e4b2 (CRT) */

/* Per-part transform/attach pass over the copied model. */
extern void AnimApplyPart(void* c, void* p, void* q, void* i, int n); /* 0x00442040 */
/* Replace colour key1 with rep1 and key2 with rep2 across the instance's
 * `n` 36-byte parts. */
extern void RecolourModelParts(const Colour3* k1, const Colour3* r1, const Colour3* k2, const Colour3* r2, void* inst, int n); /* 0x004424e0 */

/* Two parallel outfit sets, picked by the person's `variant` (Person3D+0x84).
 * Each set is {part table, count} twice plus two palettes. */
extern void** g_outfitA_tab0;   /* 0x00655a38 */
extern void*  g_outfitA_pal0;   /* 0x00641000 */
extern int    g_outfitA_n0;     /* 0x0063810c */
extern void** g_outfitB_tab0;   /* 0x0062fea8 */
extern void*  g_outfitB_pal0;   /* 0x0064cd90 */
extern int    g_outfitB_n0;     /* 0x0062feb8 */
extern void** g_outfitA_tab1;   /* 0x0062fef8 */
extern void*  g_outfitA_pal1;   /* 0x00638110 */
extern int    g_outfitA_n1;     /* 0x0064cd88 */
extern void** g_outfitB_tab1;   /* 0x0064cd8c */
extern void*  g_outfitB_pal1;   /* 0x00638108 */
extern int    g_outfitB_n1;     /* 0x0063835c */

// FUNCTION: LEGOLAND 0x00442580
void* MakeAnimInstance(Person3D* p, void* ctx, const void* src, int nparts,
                       int variant)
{
    Colour3      key1;
    Colour3      repB;
    Colour3      repA;
    void**       tabA;
    int          idxA;
    void*        palA;
    void**       tabB;
    int          idxB;
    void*        palB;
    Colour3      key2;
    int          ca;
    int          cb;
    int          na;
    int          nb;
    void*        inst;
    unsigned int size;

    if (p->kind == 1) {
        if (variant == 0) {
            tabA = g_outfitA_tab0;
            tabB = g_outfitB_tab0;
            palA = g_outfitA_pal0;
            na   = g_outfitA_n0;
            nb   = g_outfitB_n0;
            palB = g_outfitB_pal0;
            key1.c2 = 0x56;
            key1.c1 = 0x56;
            key1.c0 = 0x56;
            key2.c2 = 0;
            key2.c1 = 0;
            key2.c0 = 0;
        } else {
            tabA = g_outfitA_tab1;
            tabB = g_outfitB_tab1;
            na   = g_outfitA_n1;
            nb   = g_outfitB_n1;
            palA = g_outfitA_pal1;
            palB = g_outfitB_pal1;
            key1.c2 = 0x56;
            key1.c1 = 0x56;
            key1.c0 = 0x56;
            key2.c2 = 1;
            key2.c1 = 1;
            key2.c0 = 1;
        }
        if (p->f80 == -1) {
            idxA = rand() % na;
            p->f80 = idxA;
        } else {
            idxA = p->f80;
        }
        if (p->f7c == -1) {
            idxB = rand() % nb;
            p->f7c = idxB;
        } else {
            idxB = p->f7c;
        }
        cb = p->f8c;
        if (cb == -1) {
            do {
                cb = rand() & 7;
            } while (cb == 7);
            p->f8c = cb;
        }
        ca = p->f90;
        if (ca == -1) {
            ca = rand() & 7;
            p->f90 = ca;
        }
    } else if (p->kind == 3) {
        ca = 4;
        cb = 1;
        key1.c2 = 0x56;
        key1.c1 = 0x56;
        key1.c0 = 0x56;
        key2.c2 = 0;
        key2.c1 = 0;
        key2.c0 = 0;
    } else if (p->kind == 2) {
        ca = 0;
        cb = 3;
        key1.c2 = 0x56;
        key1.c1 = 0x56;
        key1.c0 = 0x56;
        key2.c2 = 0;
        key2.c1 = 0;
        key2.c0 = 0;
    }
    /* [sic] for any other kind ca/cb are never assigned; the original reads
     * their (uninitialised) stack homes, which VC6 put in the dead `p`
     * argument slot -- so the palette is indexed by the pointer value. */
    repA.c2 = g_bloke_palette[ca].r;
    repA.c1 = g_bloke_palette[ca].g;
    repA.c0 = g_bloke_palette[ca].b;
    repB.c2 = g_bloke_palette[cb].r;
    repB.c1 = g_bloke_palette[cb].g;
    repB.c0 = g_bloke_palette[cb].b;
    size = nparts * 36;
    inst = HeapAlloc_w(size);
    if (inst) {
        memcpy(inst, src, size);
        if (p->kind < 2) {
            AnimApplyPart(ctx, palA, tabA[idxA], inst, nparts);
            AnimApplyPart(ctx, palB, tabB[idxB], inst, nparts);
        }
        RecolourModelParts(&key1, &repA, &key2, &repB, inst, nparts);
    }
    return inst;
}
