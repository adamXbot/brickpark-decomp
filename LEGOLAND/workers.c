/* LEGOLAND — workers (gardeners / mechanics), work-order slots and bloke
 * direction encoding.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; the field and type names are ours.
 * Each matched function carries its original VA marker on the line above it.
 *
 * ---------------------------------------------------------------------------
 * THE DIRECTION ENCODING
 *
 * A bloke's heading is a 3-bit direction 0..7 (Bloke::dir @+0x72), and the
 * value 8 means "no direction". The path/AI code works with a BIT MASK of
 * candidate directions instead: Dir_To_Bit (sweep2.c) maps dir -> the byte
 * g_dir_bit_table[dir] (0x004b9550), and Bit_To_Dir (here) is its inverse — a
 * linear scan of the same 8-entry table for the first entry that shares a bit
 * with the mask, 8 when none does.
 *
 * Random_Dir_From_Bits(mask) picks ONE set bit of `mask` uniformly at random
 * (popcount the mask, draw Rand_Max(n-1), then walk the mask clearing the
 * lowest set bit that many times), converts it with Bit_To_Dir and bumps the
 * per-direction pick counter g_dir_counts[dir] (0x0066b580, 9 dwords, the
 * "8 = none" slot included). An empty mask yields 8 without touching the
 * counters.
 *
 * NewDirForAction(b, dir) requests a turn: if the bloke already faces `dir`
 * (masked to 3 bits) nothing happens (0); otherwise the target heading goes to
 * +0x73, the current low-level state (+0x0e) is saved into +0x10, the state
 * becomes 5 (= "turning"), the walk delay (+0x75) is reset to 1, and 1 is
 * returned.
 *
 * HitObstacle(b, x, y) is HitPathEdge's sibling (pathbuild.c) for the
 * "blocked" RF bit (RF bit 1): off-map is an obstacle (1); a bloke that is
 * currently STANDING on a blocked cell ignores obstacles entirely (0, so it
 * can walk out of one); otherwise the destination's RF bit 1 decides.
 *
 * ---------------------------------------------------------------------------
 * WORKERS AND THE WORKER-ON-MOUSE
 *
 * Workers are ordinary blokes. The pool is sized from the map header
 * (u16 max_blokes @ g_map+0x1a) by the internal allocator at 0x00482e50
 * (InitialiseBlokes is a tail jump to it; the bloke pool base is
 * g_bloke_base @0x0066b57c). NewBlokeWOList is NewBloke's malloc'd twin: a
 * heap bloke NOT linked on the people chain ("WO list" = without list) but
 * still given its 3D person via Add3DBlokeToList(b, kind). DestroyBloke is
 * the destructor for either kind: unlink from the people chain (with a
 * "DestroyBloke: Badly linked list" OutputDebugString when it is not on it),
 * silence its samples (BlokeSoundSource {1, b}), unlink + free the 3D person,
 * clear the in-use bit and the link.
 *
 * Hired workers hang off two further lists: gardeners @0x0079a8a8 (count
 * @0x0079a8bc) and mechanics @0x0079a8ac (count @0x0079a8cc), walked each
 * tick by ControlWorkers (mechanics first, then gardeners). RemoveAGardener is
 * RemoveAMechanic's twin (workorder.c) except that a gardener NOT found on the
 * list raises bit 0x80 in g_map_dirty (0x00668610).
 *
 * Picking a worker up with the mouse: g_worker_on_mouse (0x007fdff0) is the
 * bloke, g_worker_old_x/y (0x007fdff4/8) its world position before pickup and
 * g_worker_on_mouse_type (0x007fdffc) the icon type — 0x307 = gardener,
 * anything else = mechanic. SetWorkersPositionAtMouse drops the bloke under
 * the cursor: state 13 ("carried"), the 3D person's screen position is set to
 * the cursor point and nudged by AdjustBlokePosition (-0x4b,-0x4d), and the
 * world position becomes the cursor's map cell << 8. ResetWorkersOldCoords
 * cancels the pickup: restore the old world position, clear +0x50, send the
 * worker back to its idle plan (0x10 gardener / 0x11 mechanic) and clear the
 * move-a-worker globals.
 *
 * ---------------------------------------------------------------------------
 * WORK-ORDER SLOTS
 *
 * The WorkOrder record and the list globals are documented in workorder.c.
 * GetGardenerWorkOrderAt / GetMechanicWorkOrderAt find the order whose FIRST
 * rect (biased by the order's own pos) contains the query point.
 *
 * SetGardenerWorkOrderAtPostion(g, x, y) is the "click a gardener onto a
 * tile" rule: if no order covers the tile the gardener is sent looking for
 * work (RunGardenerJob) and, finding none, back to idle; if an order exists
 * and is unassigned it is simply handed over; if it is already assigned the
 * current holder is only displaced when it has not yet started the job —
 * long-term action < 0x6b for an object order (kind 1), < 9 for a path span
 * (kind 2); any other kind, or a holder past that point, refuses (0).
 *
 * RemoveRepairOrderAT(obj, x, y) routes a cancellation by the object class
 * flags at +0x1c: 0x200000 = gardener-maintained (only while the map's
 * gardener service @g_map+0x38 is on), 0x400000 = mechanic-maintained (map
 * +0x34), else it is a park-funded repair order.
 *
 * RateBlokeOnLeaving(score) buckets a departing visitor's score against the
 * four thresholds at 0x00832928/30/34/38 into 0..4 and pushes it into the
 * 25-entry ring g_rating_hist (0x00832bb0, index @0x00832bc9).
 * ------------------------------------------------------------------------- */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* The 3D person's screen position pair at Person3D+0x1c (AdjustBlokePosition
 * takes a pointer straight at it). */
typedef struct PersonPos {
    int x;                      /* +0x00 (Person3D +0x1c) */
    int y;                      /* +0x04 (Person3D +0x20) */
} PersonPos;

typedef struct Person3D {
    unsigned char pad00[0x1c];  /* +0x00..0x1b */
    PersonPos     pos;          /* +0x1c screen x / +0x20 screen y */
    unsigned char pad24[0x70];  /* +0x24..0x93 */
} Person3D;

/* A "bloke" (worker/visitor); allocation stride 172 (0xac). */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00  intrusive list link */
    Person3D*      person;      /* +0x04  the renderable 3D person */
    unsigned char  pad08[6];    /* +0x08..0x0d */
    unsigned short state;       /* +0x0e  low-level AI state */
    unsigned short prev_state;  /* +0x10  state saved across a turn */
    unsigned char  pad12[0x3e]; /* +0x12..0x4f */
    int            f50;         /* +0x50  cleared when a pickup is cancelled */
    unsigned char  pad54[0x0c]; /* +0x54..0x5f */
    unsigned char  action;      /* +0x60  long-term action progress code */
    unsigned char  pad61;       /* +0x61 */
    unsigned short flags62;     /* +0x62  bit 0 = slot in use */
    unsigned char  f64;         /* +0x64 */
    unsigned char  pad65[3];    /* +0x65..0x67 */
    int            wx;          /* +0x68  world x, 24.8 (256 units per tile) */
    int            wy;          /* +0x6c  world y, 24.8 */
    unsigned char  pad70[2];    /* +0x70..0x71 */
    unsigned char  dir;         /* +0x72  current heading 0..7 (8 = none) */
    unsigned char  new_dir;     /* +0x73  heading requested by NewDirForAction */
    unsigned char  pad74;       /* +0x74 */
    unsigned char  walk_delay;  /* +0x75 */
    unsigned char  pad76[0x36]; /* +0x76..0xab */
} Bloke;

/* A gardener or mechanic work order — calloc(0x3c,1); see workorder.c. */
typedef struct WorkOrder {
    struct WorkOrder* next;     /* +0x00 */
    void*             obj;      /* +0x04  target object */
    Pos               pos;      /* +0x08  map cell the rects are biased by */
    Rect*             rects;    /* +0x10  rect array; only [0] is tested here */
    int               nrects;   /* +0x14 */
    int               assigned; /* +0x18  1 once a worker has taken it */
    Bloke*            worker;   /* +0x1c */
    unsigned char     kind;     /* +0x20  1 = object, 2 = path/scenery span */
    unsigned char     pad21[0x1b]; /* +0x21..0x3b */
} WorkOrder;

/* The object class record as seen by RemoveRepairOrderAT: flags at +0x1c. */
typedef struct ObjClassFlags {
    unsigned char pad00[0x1c];  /* +0x00..0x1b */
    unsigned int  flags;        /* +0x1c  0x200000 gardener, 0x400000 mechanic */
} ObjClassFlags;

/* The map header with the worker-service switches at +0x34/+0x38. */
typedef struct MapServices {
    unsigned char pad00[0x34];  /* +0x00..0x33 */
    int           mechanics;    /* +0x34  non-zero: mechanic orders accepted */
    int           gardeners;    /* +0x38  non-zero: gardener orders accepted */
} MapServices;

/* The sound-source descriptor KillAllSamplesFromSource takes (rides.c). */
typedef struct BlokeSoundSource {
    int    kind;                /* +0x00  1 = bloke */
    Bloke* bloke;               /* +0x04 */
    int    pad08;
    int    pad0c;
} BlokeSoundSource;

/* ---------------------------------------------------------------- globals -- */

extern unsigned char g_dir_bit_table[];      /* 0x004b9550  dir -> bit */
extern int           g_dir_counts[];         /* 0x0066b580  picks per dir (9) */

extern Bloke*        g_people_head;          /* 0x0066b574 */
extern Bloke*        g_gardener_list;        /* 0x0079a8a8 */
extern int           g_gardener_count;       /* 0x0079a8bc */
extern WorkOrder*    g_gardener_orders;      /* 0x0079a8b0 */
extern WorkOrder*    g_mechanic_orders;      /* 0x0079a8c0 */
extern void*         g_placing_obj;          /* 0x0080ff64 */
extern int           g_placed_flag;          /* 0x0079a8d0 */
extern int           g_map_dirty;            /* 0x00668610 */

extern Bloke*        g_worker_on_mouse;      /* 0x007fdff0 */
extern int           g_worker_old_x;         /* 0x007fdff4 */
extern int           g_worker_old_y;         /* 0x007fdff8 */
extern int           g_worker_on_mouse_type; /* 0x007fdffc  0x307 = gardener */
extern Pos           g_gfx_point;            /* 0x00813a44  cursor point */

extern int           g_rate_t0;              /* 0x00832928 */
extern int           g_rate_t1;              /* 0x00832930 */
extern int           g_rate_t2;              /* 0x00832934 */
extern int           g_rate_t3;              /* 0x00832938 */
/* The rating ring and its write index are ONE 26-byte record (index at
 * 0x00832bc9 = 0x00832bb0 + 25). Declaring them as one struct is load-bearing:
 * the byte store into hist[] then may-alias the index, so VC6 reloads it for
 * the increment exactly as the original does; two separate globals let it
 * keep the index in cl. */
typedef struct RatingRing {
    unsigned char hist[25];     /* +0x00  0x00832bb0 */
    signed char   idx;          /* +0x19  0x00832bc9 */
} RatingRing;
extern RatingRing    g_rating;               /* 0x00832bb0 */

/* ------------------------------------------------------------ prototypes -- */

__declspec(dllimport) void __stdcall OutputDebugStringA(const char* s); /* [0x4ab1f0] */

extern unsigned int  Rand_Max(unsigned int max);                  /* 0x004806a0 */
extern unsigned char GetCurrentRFFlags(int x, int y);             /* 0x00461630 */
extern void   NewLongTermAction(Bloke* b, short action);          /* 0x0044e760 */
extern void   RefreshObjectAtPos(void* obj, Pos* pos);            /* 0x0045e4a0 */
extern void   FreeMechanicOrder(WorkOrder* o);                    /* 0x00499eb0 */
extern void   RemoveGardenersWorkOrderAt(int x, int y);           /* 0x0049b5f0 */
extern void   RemoveMechanicsWorkOrderAt(int x, int y);           /* 0x0049b640 */
extern void   RemoveNoneWorkersRepairOrderAT(int x, int y);       /* 0x0049b720 */
extern void   IterateNoneWorkersRepairOrders(void);               /* 0x0049b750 */
/* Draw one work-order list's markers: 0 = gardener (green), 1 = mechanic. */
extern void   RenderWorkOrders(int mechanic);                     /* 0x0049ac50 (unexported) */
/* Per-tick AI walk of the two hired-worker lists. */
extern void   ControlMechanics(void);                             /* 0x0049a010 (unexported) */
extern void   ControlGardeners(void);                             /* 0x00499fb0 (unexported) */
/* Allocate and zero the bloke pool (g_map->max_blokes * 172 bytes). */
extern void   AllocBlokePool(void);                               /* 0x00482e50 (unexported) */
/* Look for work for a gardener; non-zero when it found some. */
extern int    RunGardenerJob(Bloke* b);                           /* 0x00499d00 (unexported) */
/* Hand an existing order to a gardener. */
extern void   AssignGardenerOrder(Bloke* b, WorkOrder* o);        /* 0x00499ac0 (unexported) */
extern void   Add3DBlokeToList(Bloke* b, int kind);               /* 0x00440680 */
extern void   Remove3DPersonFromList(Person3D* p);                /* 0x0043f840 */
extern void   Free3DPerson(Person3D* p);                          /* 0x0043f870 (unexported) */
extern void   KillAllSamplesFromSource(BlokeSoundSource* src);    /* 0x00496b80 */
extern void   AdjustBlokePosition(PersonPos* p);                  /* 0x00442d60 */
extern void   ScreenToMapRef(Pos* screen, Pos* map, int mode);    /* 0x0045be90 */
extern void   ResetMoveAWorkerStruct(void);                       /* 0x00470930 */
extern void*  MemAlloc(int size);                                 /* 0x0049e4ff */
extern void   MemFree(void* p);                                   /* 0x0049e4d0 */
void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)

/* -------------------------------------------------------------- functions -- */

/* Tail jump to the pool allocator; audit.py bounds the jmp, match.py cannot. */
// WIP-FUNCTION: LEGOLAND 0x004830f0  (100% by audit.py; match.py cannot bound a tail-jmp function)
void InitialiseBlokes(void)
{
    AllocBlokePool();
}

// WIP-FUNCTION: LEGOLAND 0x0049a070  (100% by audit.py; match.py cannot bound a tail-jmp function)
void ControlWorkers(void)
{
    ControlMechanics();
    ControlGardeners();
}

// WIP-FUNCTION: LEGOLAND 0x0049b0b0  (100% by audit.py; match.py cannot bound a tail-jmp function)
void RenderWorkerInterfaceGFX(void)
{
    RenderWorkOrders(0);
    RenderWorkOrders(1);
    IterateNoneWorkersRepairOrders();
}

/* Request a turn towards `dir` (3 bits); 0 when already facing that way. */
// FUNCTION: LEGOLAND 0x004833d0
int NewDirForAction(Bloke* b, unsigned char dir)
{
    dir &= 7;
    if (b->dir != dir) {
        /* Source order prev_state-then-new_dir is load-bearing: VC6 swaps
         * the two adjacent stores to open a load-use gap on `state`, and
         * only this order leaves the loaded state in dx. */
        b->prev_state = b->state;
        b->new_dir = dir;
        b->state = 5;
        b->walk_delay = 1;
        return 1;
    }
    return 0;
}

/* Inverse of Dir_To_Bit: first direction whose table bit is in `mask`. */
// FUNCTION: LEGOLAND 0x0045c020
unsigned char Bit_To_Dir(unsigned char mask)
{
    unsigned char i;

    for (i = 0; i < 8; i++) {
        if (g_dir_bit_table[i] & mask)
            return i;
    }
    return 8;
}

/* Put a picked-up worker back where it was and send it idle. */
// FUNCTION: LEGOLAND 0x004708d0
void ResetWorkersOldCoords(void)
{
    if (g_worker_on_mouse) {
        g_worker_on_mouse->wx = g_worker_old_x;
        g_worker_on_mouse->wy = g_worker_old_y;
        g_worker_on_mouse->f50 = 0;
        if (g_worker_on_mouse_type == 0x307)
            NewLongTermAction(g_worker_on_mouse, 0x10);
        else
            NewLongTermAction(g_worker_on_mouse, 0x11);
        ResetMoveAWorkerStruct();
    }
}

/* Heap-allocated bloke that is NOT linked on the people chain. */
// FUNCTION: LEGOLAND 0x00482f70
Bloke* NewBlokeWOList(int kind)
{
    Bloke* b = (Bloke*)MemAlloc(sizeof(Bloke));

    if (b) {
        memset(b, 0, sizeof(Bloke));
        b->f64 = 0;
        b->flags62 = 1;
        Add3DBlokeToList(b, kind);
    }
    return b;
}

/* Bucket a departing visitor's score into the 25-entry rating ring. */
// FUNCTION: LEGOLAND 0x004633f0
void RateBlokeOnLeaving(int score)
{
    int rating;

    if (score < g_rate_t0)
        rating = 0;
    else if (score < g_rate_t1)
        rating = 1;
    else if (score < g_rate_t2)
        rating = 2;
    else
        rating = (score >= g_rate_t3) + 3;
    g_rating.hist[g_rating.idx] = (unsigned char)rating;
    g_rating.idx++;
    if (g_rating.idx == 25)
        g_rating.idx = 0;
}

/* Tear down one mechanic order (EraseGardenerOrder's twin, workorder.c),
 * also dropping the "placed" flag when the order was on the object currently
 * being placed. */
// FUNCTION: LEGOLAND 0x0049b1d0
void EraseMechanicOrder(WorkOrder* o)
{
    if (o->assigned)
        NewLongTermAction(o->worker, 0x11);
    if (o->obj == g_placing_obj) {
        if (g_placed_flag)
            g_placed_flag = 0;
    }
    if (o->kind == 1)
        RefreshObjectAtPos(o->obj, &o->pos);
    FreeMechanicOrder(o);
}

/* Drop the carried worker under the cursor. */
// FUNCTION: LEGOLAND 0x004701f0
void SetWorkersPositionAtMouse(void)
{
    Pos cell;

    g_worker_on_mouse->state = 13;
    g_worker_on_mouse->person->pos.x = g_gfx_point.x;
    g_worker_on_mouse->person->pos.y = g_gfx_point.y;
    AdjustBlokePosition(&g_worker_on_mouse->person->pos);
    ScreenToMapRef(&g_gfx_point, &cell, 0);
    g_worker_on_mouse->wx = cell.x << 8;
    g_worker_on_mouse->wy = cell.y << 8;
}

/* Route a repair cancellation to the list that owns it. */
// FUNCTION: LEGOLAND 0x0049b580
void RemoveRepairOrderAT(ObjClassFlags* cls, int x, int y)
{
    /* Both cls->flags and g_map are re-read lazily at each use: VC6 CSEs
     * them (flags in eax, g_map in ecx, hoisted above the first test).
     * Hoisting either into a named local swaps the register roles. */
    if ((cls->flags & 0x200000) && ((MapServices*)g_map)->gardeners)
        RemoveGardenersWorkOrderAt(x, y);
    else if ((cls->flags & 0x400000) && ((MapServices*)g_map)->mechanics)
        RemoveMechanicsWorkOrderAt(x, y);
    else
        RemoveNoneWorkersRepairOrderAT(x, y);
}

/* Find the gardener order whose first rect (biased by its pos) holds (x,y). */
// FUNCTION: LEGOLAND 0x0049b130
WorkOrder* GetGardenerWorkOrderAt(int x, int y)
{
    WorkOrder* p = g_gardener_orders;
    Rect*      r;

    while (p) {
        r = p->rects;
        if (r->left + p->pos.x <= x && x <= r->right + p->pos.x &&
            r->top + p->pos.y <= y && y <= r->bottom + p->pos.y)
            return p;
        p = p->next;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x0049b180
WorkOrder* GetMechanicWorkOrderAt(int x, int y)
{
    WorkOrder* p = g_mechanic_orders;
    Rect*      r;

    while (p) {
        r = p->rects;
        if (r->left + p->pos.x <= x && x <= r->right + p->pos.x &&
            r->top + p->pos.y <= y && y <= r->bottom + p->pos.y)
            return p;
        p = p->next;
    }
    return 0;
}

/* Unlink a bloke from the people chain, silence it, free its 3D person and
 * release the pool slot. */
// FUNCTION: LEGOLAND 0x00483010
void DestroyBloke(Bloke* b)
{
    Bloke**          link = &g_people_head;
    BlokeSoundSource src;

    /* Walk by the link slot alone (no separate `p`): the original reloads
     * the next bloke through the link register, which a `p = p->next`
     * form copy-propagates into `mov eax,[eax]`. */
    while (*link && *link != b)
        link = &(*link)->next;
    if (*link)
        *link = b->next;
    else
        OutputDebugStringA("DestroyBloke: Badly linked list\n");
    src.kind = 1;
    src.bloke = b;
    KillAllSamplesFromSource(&src);
    Remove3DPersonFromList(b->person);
    Free3DPerson(b->person);
    b->flags62 &= ~1;
    b->next = 0;
}

/* Unlink a gardener from the hired list and free it; a gardener that is not
 * on the list flags the map dirty (bit 0x80) instead. */
// FUNCTION: LEGOLAND 0x0049a2d0
void RemoveAGardener(Bloke* g)
{
    Bloke* p = g_gardener_list;
    Bloke* prev;

    if (!p)
        return;
    if (p == g) {
        g_gardener_list = p->next;
        MemFree(p);
        g_gardener_count--;
        return;
    }
    prev = p;
    p = p->next;
    while (p) {
        if (p == g) {
            prev->next = p->next;
            MemFree(p);
            g_gardener_count--;
            return;
        }
        prev = p;
        p = p->next;
    }
    g_map_dirty |= 0x80;
}

/* Would stepping to world (x,y) hit a blocked cell (RF bit 1)? A bloke that
 * is already standing on a blocked cell ignores obstacles. */
// FUNCTION: LEGOLAND 0x00483510
int HitObstacle(Bloke* b, int x, int y)
{
    if (x >= 0 && x < (g_map->width << 8) &&
        y >= 0 && y < (g_map->height << 8)) {
        /* One '&&' with a single return 1 inside: two separate
         * 'if (f & 2) return' statements make VC6 extract the bit
         * ((f >> 1) & 1) instead of keeping the two exit blocks. */
        if (!(GetCurrentRFFlags(b->wx, b->wy) & 2) &&
            (GetCurrentRFFlags(x, y) & 2))
            return 1;
        return 0;
    }
    return 1;
}

/* Click a gardener onto a tile: take over the order there (displacing a
 * holder that has not started yet), or go looking for work. */
// FUNCTION: LEGOLAND 0x0049b2c0
int SetGardenerWorkOrderAtPostion(Bloke* g, int x, int y)
{
    WorkOrder* o = GetGardenerWorkOrderAt(x, y);

    if (!o) {
        if (!RunGardenerJob(g)) {
            NewLongTermAction(g, 0x10);
            return 1;
        }
    } else {
        if (o->assigned) {
            /* The '&&' chain is the original's shape: a kind-1 holder past
             * 0x6b falls into the kind==2 test (the 'cmp al,2' after the
             * jae), and VC6 tail-merges the two identical calls. */
            if (o->kind == 1 && o->worker->action < 0x6b)
                NewLongTermAction(o->worker, 0x10);
            else if (o->kind == 2 && o->worker->action < 9)
                NewLongTermAction(o->worker, 0x10);
            else
                return 0;
        }
        AssignGardenerOrder(g, o);
    }
    return 1;
}

/* Pick one set bit of `mask` uniformly at random and return its direction,
 * counting the pick in g_dir_counts. Empty mask -> 8. */
// FUNCTION: LEGOLAND 0x00483400
unsigned char Random_Dir_From_Bits(unsigned char mask)
{
    unsigned char n;
    unsigned char t;
    unsigned char bit;
    unsigned char dir;
    int           r;

    if (mask == 0)
        return 8;
    n = 1;
    t = mask & (mask - 1);
    while (t) {
        n++;
        t &= t - 1;
    }
    r = Rand_Max(n - 1);
    bit = 1;
    while (!(mask & bit))
        bit <<= 1;
    while (r) {
        mask &= ~bit;
        while (!(mask & bit))
            bit <<= 1;
        r--;
    }
    dir = Bit_To_Dir(bit);
    g_dir_counts[dir]++;
    return dir;
}
