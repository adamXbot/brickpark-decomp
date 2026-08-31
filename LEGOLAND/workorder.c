/* LEGOLAND — gardener / mechanic work-order bookkeeping.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; the field and type names are ours.
 * Each matched function carries its original VA marker on the line above it.
 *
 * ------------------------------------------------------------------------
 * THE WORKER TASK SYSTEM
 *
 * Two workers exist: the gardener (sweeps litter / tends plants) and the
 * mechanic (repairs broken rides and scenery). Each has an intrusive singly
 * linked list of WorkOrder records, plus a head/tail/count triple of globals:
 *
 *      gardener: head @0x0079a8b0  tail @0x0079a8b4  count @0x0079a8b8
 *      mechanic: head @0x0079a8c0  tail @0x0079a8c4  count @0x0079a8c8
 *
 * A WorkOrder node is calloc(0x3c, 1) — 60 bytes (see the allocator at
 * 0x004995d0, which appends to the tail and bumps the count). Mechanic order
 * creation refuses to run once the count reaches 0xe1 (225), so the count is a
 * hard cap, not a statistic.
 *
 * Gardener and mechanic orders share ONE record layout — the two lists are
 * distinguished only by which head they hang off and by the long-term-action
 * code their handlers issue (0x10 for gardener, 0x11 for mechanic). What DOES
 * vary inside a list is WorkOrder::kind (+0x20):
 *      kind 1 = a placed object (the footprint Rect list is copied out of the
 *               ObjClass at +0x3c; rects=1). Erasing such an order calls
 *               RefreshObjectAtPos(obj, &pos) to redraw the tile it occupied.
 *      kind 2 = a path/scenery span (the rect list is built by walking the
 *               scenery chain; rects can be > 1).
 * Both GetGardenerWorkOrderAt/GetMechanicWorkOrderAt (0x0049b130/0x0049b180)
 * locate an order by testing the query point against the FIRST rect only,
 * biased by the order's own (x,y): left+x <= qx <= right+x and
 * top+y <= qy <= bottom+y.
 *
 * A third, separate list holds "none-worker" repair orders — repairs the park
 * pays for automatically rather than dispatching a mechanic:
 *
 *      repair orders: head @0x0079a8d4, node size 0x28 (40 bytes)
 *          +0x00 next
 *          +0x04 Rect (20 bytes, copied by value — 5 dwords incl. its `next`)
 *          +0x18 x        \  the base the embedded rect is biased by, and the
 *          +0x1c y        /  key RemoveNoneWorkersRepairOrderAT matches on
 *          +0x20 float    progress/scale (arg * 0x004ab480)
 *          +0x24 void*    owning object
 *
 * ------------------------------------------------------------------------
 * A BUG WE REPRODUCE FAITHFULLY
 *
 * ClearAGardenersWorkList / ClearAMechanicsWorkList detach a worker from every
 * order it held — and then INCREMENT the list's order count. The count is only
 * ever decremented when a node is actually freed (0x00499e30 / 0x00499eb0), so
 * every worker that is fired or reassigned permanently inflates it. For the
 * mechanic list, whose creation path bails out at count >= 225, this eventually
 * wedges the park into never accepting another repair order. The increment is
 * in the original; it is kept here and flagged.
 */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* A "bloke" (worker/person) record; stride 172 (0xac). Only the two fields
 * these functions touch are named. See blokemisc.c for the fuller picture. */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00  intrusive list link */
    unsigned char  pad04[0x5c]; /* +0x04..0x5f */
    unsigned char  action;      /* +0x60  current long-term action code */
    unsigned char  pad61[0x4b]; /* +0x61..0xab */
} Bloke;

/* A gardener or mechanic work order — calloc(0x3c,1) @ 0x004995d0. */
typedef struct WorkOrder {
    struct WorkOrder* next;     /* +0x00 */
    void*             obj;      /* +0x04  target object (ElemID-style record) */
    Pos               pos;      /* +0x08  map cell the rects are biased by */
    Rect*             rects;    /* +0x10  malloc'd rect array (freed with node) */
    int               nrects;   /* +0x14 */
    int               assigned; /* +0x18  1 once a worker has taken this order */
    Bloke*            worker;   /* +0x1c  the worker that took it */
    unsigned char     kind;     /* +0x20  1 = object, 2 = path/scenery span */
    unsigned char     pad21[3]; /* +0x21..0x23 */
    int               ox;       /* +0x24  x bias used when kind != 1 */
    int               oy;       /* +0x28  y bias used when kind != 1 */
    int               f2c;      /* +0x2c */
    int               f30;      /* +0x30  cleared when a mechanic detaches */
    int               pad34[2]; /* +0x34..0x3b */
} WorkOrder;

/* A "none-worker" (park-funded) repair order — malloc(0x28) @ 0x0049b690. */
typedef struct RepairOrder {
    struct RepairOrder* next;   /* +0x00 */
    Rect                rect;   /* +0x04  20 bytes, copied by value */
    int                 x;      /* +0x18 */
    int                 y;      /* +0x1c */
    float               amount; /* +0x20 */
    void*               obj;    /* +0x24 */
} RepairOrder;

/* ---------------------------------------------------------------- globals -- */

/* Gardener work-order list: head, and the node count (also the list's cap). */
extern WorkOrder*   g_gardener_orders;      /* 0x0079a8b0 */
extern int          g_gardener_order_count; /* 0x0079a8b8 */
/* Mechanic work-order list: head, and the node count (capped at 225). */
extern WorkOrder*   g_mechanic_orders;      /* 0x0079a8c0 */
extern int          g_mechanic_order_count; /* 0x0079a8c8 */
/* Park-funded repair orders (no worker dispatched). */
extern RepairOrder* g_repair_orders;        /* 0x0079a8d4 */
/* The hired mechanics themselves, and how many there are. */
extern Bloke*       g_mechanic_list;        /* 0x0079a8ac (export MechanicList) */
extern int          g_mechanic_count;       /* 0x0079a8cc */

/* ------------------------------------------------------------ prototypes -- */

/* Cancel a bloke's current job and give it a new long-term action code.
 * 0x10 = "gardener job finished/aborted", 0x11 = the mechanic equivalent. */
extern void  NewLongTermAction(Bloke* b, int action);        /* 0x0044e760 */
/* Redraw the object at a map position (used when an order over it is torn
 * down so the tile stops showing the "needs work" overlay). */
extern void  RefreshObjectAtPos(void* obj, Pos* pos);        /* 0x0045e4a0 */
/* Unlink + free a work-order node (and its rect array), decrementing the
 * owning list's count. */
extern void  FreeGardenerOrder(WorkOrder* o);                /* 0x00499e30 */
extern void  FreeMechanicOrder(WorkOrder* o);                /* 0x00499eb0 */
/* Unlink + free a park-funded repair order. */
extern void  FreeRepairOrder(RepairOrder* r);                /* 0x0049b6e0 */
/* The game's free() wrapper. */
extern void  MemFree(void* p);                               /* 0x0049e4d0 */

/* -------------------------------------------------------------- functions -- */

/* Detach `worker` from every gardener order it holds. NOTE: the count bump is
 * the original's bug — see the header comment. */
// FUNCTION: LEGOLAND 0x0049b550
void ClearAGardenersWorkList(Bloke* worker)
{
    WorkOrder* p = g_gardener_orders;

    while (p) {
        if (p->worker == worker) {
            p->assigned = 0;
            p->worker = 0;
            g_gardener_order_count++;   /* original bug: never balanced */
        }
        p = p->next;
    }
}

/* Same for the mechanic list, plus clearing the order's +0x30 scratch field. */
// FUNCTION: LEGOLAND 0x0049b510
void ClearAMechanicsWorkList(Bloke* worker)
{
    WorkOrder* p = g_mechanic_orders;

    while (p) {
        if (p->worker == worker) {
            p->assigned = 0;
            p->f30 = 0;
            p->worker = 0;
            g_mechanic_order_count++;   /* original bug: never balanced */
        }
        p = p->next;
    }
}

/* Drop the park-funded repair order registered at (x,y), if any. */
// FUNCTION: LEGOLAND 0x0049b720
void RemoveNoneWorkersRepairOrderAT(int x, int y)
{
    RepairOrder* p = g_repair_orders;

    while (p) {
        if (p->x == x && p->y == y) {
            FreeRepairOrder(p);
            return;
        }
        p = p->next;
    }
}

/* Tear down one gardener order: release its worker, redraw the object it was
 * attached to (object orders only), then free the node. */
// FUNCTION: LEGOLAND 0x0049b230
void EraseGardenerOrder(WorkOrder* o)
{
    if (o->assigned)
        NewLongTermAction(o->worker, 0x10);
    if (o->kind == 1)
        RefreshObjectAtPos(o->obj, &o->pos);
    FreeGardenerOrder(o);
}

/* Cancel the gardener order whose position is exactly (x,y). Unlike
 * EraseGardenerOrder this does NOT redraw the object — the caller is removing
 * the map content itself. */
// FUNCTION: LEGOLAND 0x0049b5f0
void RemoveGardenersWorkOrderAt(int x, int y)
{
    WorkOrder* p = g_gardener_orders;

    while (p) {
        if (p->pos.x == x && p->pos.y == y) {
            if (p->assigned)
                NewLongTermAction(p->worker, 0x10);
            FreeGardenerOrder(p);
            return;
        }
        p = p->next;
    }
}

// FUNCTION: LEGOLAND 0x0049b640
void RemoveMechanicsWorkOrderAt(int x, int y)
{
    WorkOrder* p = g_mechanic_orders;

    while (p) {
        if (p->pos.x == x && p->pos.y == y) {
            if (p->assigned)
                NewLongTermAction(p->worker, 0x11);
            FreeMechanicOrder(p);
            return;
        }
        p = p->next;
    }
}

/* Unlink a mechanic from the hired-mechanic list and free the record. */
// FUNCTION: LEGOLAND 0x0049a430
void RemoveAMechanic(Bloke* m)
{
    Bloke* p = g_mechanic_list;
    Bloke* prev;

    if (!p)
        return;
    if (p == m) {
        g_mechanic_list = p->next;
    } else {
        prev = p;
        p = p->next;
        while (p) {
            if (p == m)
                goto unlink;
            prev = p;
            p = p->next;
        }
        return;                 /* not on the list — nothing to free */
unlink:
        prev->next = p->next;
    }
    MemFree(p);
    g_mechanic_count--;
}
