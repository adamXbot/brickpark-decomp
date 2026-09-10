/* LEGOLAND — map AI tick, worker repair jobs and the path direction mask.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS are load-bearing; names are ours. Types are defined locally
 * (legoland.h is shared and untouched).
 *
 * ---------------------------------------------------------------------------
 * THE REPAIR JOB STATE MACHINE (Garderner_Repair [sic] / Mechanics_Repair)
 *
 * Handler-table slots 0x15/0x16 (see blokeai.c). Both are one switch on
 * Bloke::action (+0x60), the Repair twins of Gardener_Build / Mechanic_Build
 * (workers2.c). The current order is at Bloke +0x50; the order carries the
 * tile at +0x08, the heading the worker faces while repairing at +0x2c, the
 * "no money" flag at +0x30, and the running bricks owed / per-tick top-up at
 * +0x34/+0x38 (the same pair IterateNoneWorkersRepairOrders uses).
 *
 * Gardener (actions 0..10, no 0x64 base):
 *   0    ask the path finder for the next leg: 2 = walk it, action 6|(!f64&1);
 *        1 = walk it, action 6 only when f64 bit 0 is set; 0 = no route: walk
 *        the order's span (0x00499720), give it back, plan 0x10, action 0,
 *        state 4 with a 0x10 wait
 *   6    wait 0x70 ticks in state 4, then back to 0
 *   7    flags 0x108, animation 1, face the order's heading, advance
 *   8    play the animation; when it completes go back to the walk animation,
 *        clear 0x100, advance
 *   9    the repair tick: only while the cell is an object cell (flags 0x88);
 *        charge (int)amount bricks when amount >= 1.0, add the fraction plus
 *        the top-up back, run RepairCellTick and — once life has reached the
 *        class maximum — clear CF_REPAIRORDER (0x4000), restore the life and
 *        advance; cannot afford it: order->f30 = 1 and try again next tick
 *   10   clear flag 8, free the order, next job (RunGardenerJob) or idle 0x10
 *
 * Mechanic (actions 0, 0x0b, 0x64, 0x6a, 0x6b, 0x6c):
 *   0x00 aim at the saved target: CalcMoveLine, state 12, turn, action 0x0b
 *   0x0b arrived within 0x10000 squared world units? -> 0x6b : 0x64
 *   0x64 path finder as above (0x6a|..; give back -> plan 0x11, wait 0x70)
 *   0x6a wait 0x70 ticks, then back to 0x64
 *   0x6b the repair tick as above, but a non-object cell just advances and
 *        running out of bricks also raises message 2
 *   0x6c clear flag 8, f46 = 1, free the order, next job or idle 0x11
 *
 * ---------------------------------------------------------------------------
 * THE MAP AI TICK (DoMapAI)
 *
 * The 0x3f0-byte state block at 0x00832800 (objmap.c's MapAI) carries a scan
 * cursor (+0x04/+0x08), a phase (+0x0c) and six 0x2c-byte category records
 * from +0x10, indexed by the object class's category (short at +0x20).
 * Each tick spends up to 256 steps:
 *   phase 0  zero the four per-category accumulators (+0x1c..+0x28), rewind
 *            the cursor, phase 1
 *   phase 1  one cell per step: a path cell bumps cat[0]; an object cell
 *            (flags 0x88) with a categorised class bumps its category's
 *            count and, on the object's anchor cell, its "working" count,
 *            salvage value and staff (short at class +0x2e) when
 *            0x0044f360 says it is running; advance the cursor, phase 2 at
 *            the end of the map
 *   phase 2  publish the accumulators into +0x00/+0x08/+0x0c/+0x10, sum
 *            the published object counts into +0x118, derive the park
 *            income (+0x11c, clamped between +0x124 and +0x120 and the
 *            map header's +0x1a) and go back to phase 0. Its six-record loop shares the STEP COUNTER (the
 *            original leaves it at 6, so the tick resumes at step 7).
 * When the map-screen flag byte has none of its low six bits set the tick
 * instead recounts the object definitions per category into +0x14.
 * --------------------------------------------------------------------------- */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* A "bloke" (worker/visitor); allocation stride 172 (0xac). */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00 */
    void*          person;      /* +0x04 */
    unsigned char  pad08[4];    /* +0x08..0x0b */
    unsigned short plan;        /* +0x0c  long-term action (plan) */
    unsigned short state;       /* +0x0e  low-level AI state */
    unsigned char  pad10[0x14]; /* +0x10..0x23 */
    Pos            target;      /* +0x24  walk target (24.8 world units) */
    Pos            saved;       /* +0x2c  saved target (the job's tile) */
    unsigned char  pad34[0x12]; /* +0x34..0x45 */
    unsigned short f46;         /* +0x46 */
    unsigned char  pad48[8];    /* +0x48..0x4f */
    struct RepairJob* order;    /* +0x50  current work order */
    unsigned char  pad54[8];    /* +0x54..0x5b */
    int            wait;        /* +0x5c  ticks to wait in state 4 */
    unsigned char  action;      /* +0x60  progress code within the plan */
    unsigned char  pad61;       /* +0x61 */
    unsigned short flags62;     /* +0x62 */
    unsigned char  f64;         /* +0x64 */
    unsigned char  pad65[3];    /* +0x65..0x67 */
    Pos            world;       /* +0x68  world position, 24.8 */
    unsigned short f70;         /* +0x70 */
    unsigned char  dir;         /* +0x72  current heading */
    unsigned char  new_dir;     /* +0x73  requested heading */
    unsigned char  pad74[0x24]; /* +0x74..0x97 */
    unsigned char  path[0x14];  /* +0x98  move-line scratch (CalcMoveLine) */
} Bloke;

/* A gardener/mechanic work order as the repair handlers see it (0x3c bytes;
 * workorder.c). +0x2c is read as a BYTE here (the heading to face). */
typedef struct RepairJob {
    struct RepairJob* next;     /* +0x00 */
    void*             obj;      /* +0x04 */
    Pos               pos;      /* +0x08  the cell under repair */
    Rect*             rects;    /* +0x10 */
    int               nrects;   /* +0x14 */
    int               assigned; /* +0x18 */
    Bloke*            worker;   /* +0x1c */
    unsigned char     kind;     /* +0x20 */
    unsigned char     pad21[3]; /* +0x21..0x23 */
    int               ox;       /* +0x24 */
    int               oy;       /* +0x28 */
    unsigned char     face;     /* +0x2c  heading while repairing */
    unsigned char     pad2d[3]; /* +0x2d..0x2f */
    int               no_money; /* +0x30  1 while the park cannot pay */
    float             amount;   /* +0x34  bricks owed this tick */
    float             rate;     /* +0x38  added back each tick */
} RepairJob;

/* The object class record: max condition at +0x2c. */
typedef struct WClass {
    unsigned char pad00[0x2c];  /* +0x00..0x2b */
    unsigned char max_cond;     /* +0x2c */
} WClass;

/* A placed object as the cell points at it: its class at +0x0c. */
typedef struct MapInst {
    unsigned char pad00[0x0c];  /* +0x00..0x0b */
    WClass*       cls;          /* +0x0c */
} MapInst;

/* The object class as DoMapAI reads it: category at +0x20, staff at +0x2e. */
typedef struct AIClass {
    struct AIClass* next;       /* +0x00 */
    int             f4;         /* +0x04 */
    int             f8;         /* +0x08 */
    unsigned char   pad0c[0x14];/* +0x0c..0x1f */
    short           category;   /* +0x20 */
    unsigned char   pad22[0x0c];/* +0x22..0x2d */
    short           staff;      /* +0x2e */
} AIClass;

/* A placed object as DoMapAI reads it: its class at +0x0c. */
typedef struct AIInst {
    unsigned char pad00[0x0c];  /* +0x00..0x0b */
    AIClass*      cls;          /* +0x0c */
} AIInst;

/* The map header word at +0x1a (an income cap). */
typedef struct MapCap {
    unsigned char  pad00[0x1a]; /* +0x00..0x19 */
    unsigned short cap;         /* +0x1a */
} MapCap;

/* One category record of the map AI block (stride 0x2c). */
typedef struct AICat {
    int f00;        /* +0x00  published: objects */
    int defs;       /* +0x04  object definitions in this category */
    int f08;        /* +0x08 */
    int f0c;        /* +0x0c  published: working */
    int f10;        /* +0x10  published: salvage value */
    int f14;        /* +0x14 */
    int f18;        /* +0x18 */
    int n_objects;  /* +0x1c  accumulating */
    int n_working;  /* +0x20 */
    int value;      /* +0x24 */
    int staff;      /* +0x28 */
} AICat;

/* The map AI state block (0x00832800, 0x3f0 bytes; objmap.c's MapAI). */
typedef struct MapAI {
    int    f000;                /* +0x000 */
    int    cur_x;               /* +0x004 */
    int    cur_y;               /* +0x008 */
    int    phase;               /* +0x00c */
    AICat  cat[6];              /* +0x010 .. +0x117 */
    int    total;               /* +0x118 */
    int    income;              /* +0x11c */
    int    income_max;          /* +0x120 */
    int    income_min;          /* +0x124 */
    char   pad128[0x3f0 - 0x128];
} MapAI;

/* The packed {x,y} byte pair 0x0044f360 takes by pointer. */
typedef struct BytePos {
    unsigned char x;            /* +0x00 */
    unsigned char y;            /* +0x01 */
} BytePos;

/* ---------------------------------------------------------------- globals -- */

extern int           g_map_dirty;            /* 0x00668610 */
extern MapAI         g_map_ai;               /* 0x00832800 */
extern AIClass*      g_objdef_head;          /* 0x00669240 */
extern unsigned char g_ms_flags;             /* 0x008119a4 */
extern const double  kOne;                   /* 0x004ab3a8  1.0 */

/* ------------------------------------------------------------ prototypes -- */

extern int    FindPathLeg(Pos* from, Pos* to, Pos* out);          /* 0x00482710 */
extern int    CalcMoveLine(Pos from, Pos to, void* path);         /* 0x00480740 */
extern int    NewDirForAction(Bloke* b, unsigned char dir);       /* 0x004833d0 */
extern void   NewLongTermAction(Bloke* b, int action);            /* 0x0044e760 */
extern void   BlokeSetAnim(Bloke* b, int anim);                   /* 0x004406c0 */
extern int    PlayBlokeAnim(Bloke* b);                            /* 0x004408a0 */
extern void   BlokeWalkAnim(Bloke* b);                            /* 0x00440910 */
extern int    GetBrickCount(void);                                /* 0x004578e0 */
extern void   UseBricks(int n);                                   /* 0x004578c0 */
#ifndef LEGOLAND_PORTABLE
extern void   RepairCellTick(Cell* cell, WClass* cls);            /* 0x0049b0d0 */
#else
extern int RepairCellTick(Cell* cell, WClass* cls);            /* 0x0049b0d0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void   ShowMessage(int which);                             /* 0x004735e0 */
#else
extern int ShowMessage(int which);                             /* 0x004735e0 */
#endif
/* Walk the order's span to its end (unexported). */
extern void   WalkOrderSpan(RepairJob* o);                        /* 0x00499720 */
extern void   GiveBackGardenerOrder(RepairJob* o);                /* 0x00499e60 */
extern void   GiveBackMechanicOrder(RepairJob* o);                /* 0x00499f40 */
extern void   FreeGardenerOrder(RepairJob* o);                    /* 0x00499e30 */
extern void   FreeMechanicOrder(RepairJob* o);                    /* 0x00499eb0 */
extern int    RunGardenerJob(Bloke* b);                           /* 0x00499d00 (unexported) */
extern int    RunMechanicJob(Bloke* b);                           /* 0x00499d30 (unexported) */
extern int    GetObjSalvageValue(AIClass* cls, int life);         /* 0x00480db0 */
/* Is the object of class `cls` anchored at `at` running? (unexported) */
extern int    IsObjectRunning(AIClass* cls, BytePos* at);         /* 0x0044f360 */

/* ---------------------------------------------------------- inline helpers -- */

/* Query 0x0044f360 on the anchor tile packed as two bytes. Evaluating the
 * bytes as scalar arguments keeps their loads CSE'd with the anchor compare
 * (a direct at.x/at.y fill kills the second load). */
static __inline int IsObjectRunningAt(AIClass* cls, unsigned char x, unsigned char y)
{
    BytePos at;
    at.x = x;
    at.y = y;
    return IsObjectRunning(cls, &at);
}

/* The bounds-checked cell fetch (objmap.c's MapCellAt). */
static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

/* The same fetch through a Pos*, reading y lazily (only after x passed). */
static __inline Cell* CellForPos(Pos* pos)
{
    int x = pos->x;
    int y;

    if (x >= 0 && x < g_map->width) {
        y = pos->y;
        if (y >= 0 && y < g_map->height)
            return &g_map_rows[y][x];
    }
    return 0;
}

/* -------------------------------------------------------------- functions -- */

// FUNCTION: LEGOLAND 0x0049ba10
void Garderner_Repair(Bloke* b)
{
    Pos     leg;
    RepairJob* o;
    Cell*   cell;
    WClass* cls;
    int     price;
    unsigned char a;

    switch (b->action) {
    case 0:
        switch (FindPathLeg(&b->world, &b->saved, &leg)) {
        case 2:
            b->target.x = leg.x;
            b->target.y = leg.y;
            a = (unsigned char)(CalcMoveLine(b->world, leg, b->path) + 0x10);
            b->state = 12;
            b->new_dir = a;
            NewDirForAction(b, (unsigned char)((a >> 5) + 3));
            b->action = (unsigned char)(6 | (~b->f64 & 1));
            break;
        case 1:
            b->target.x = leg.x;
            b->target.y = leg.y;
            a = (unsigned char)(CalcMoveLine(b->world, leg, b->path) + 0x10);
            b->state = 12;
            b->new_dir = a;
            NewDirForAction(b, (unsigned char)((a >> 5) + 3));
            if (b->f64 & 1)
                b->action = 6;
            break;
        case 0:
            WalkOrderSpan(b->order);
            GiveBackGardenerOrder(b->order);
            b->plan = 0x10;
            b->action = 0;
            b->wait = 0x10;
            b->state = 4;
            break;
        }
        break;
    case 6:
        b->wait = 0x70;
        b->state = 4;
        b->action = 0;
        break;
    case 7:
        b->flags62 |= 0x108;
        BlokeSetAnim(b, 1);
        b->dir = b->order->face;
        b->action++;
        break;
    case 8:
        if (PlayBlokeAnim(b)) {
            BlokeWalkAnim(b);
            b->flags62 &= ~0x100;
            b->action++;
        }
        break;
    case 9:
        o = b->order;
        b->flags62 |= 8;
        b->dir = o->face;
        cell = MapCellAt(o->pos.x, o->pos.y);
        if (cell->flags & 0x88) {
            if (o->amount >= kOne)
                price = (int)o->amount;
            else
                price = 0;
            if (GetBrickCount() >= price) {
                UseBricks(price);
                o->amount = o->amount - (float)price + o->rate;
                cell = CellForPos(&o->pos);
                cls = ((MapInst*)cell->obj)->cls;
                g_map_dirty |= 0x200;
                RepairCellTick(cell, cls);
                if (cell->life >= cls->max_cond) {
                    cell->flags &= ~0x4000;
                    cell->life = cls->max_cond;
                    b->action++;
                }
            } else {
                b->order->no_money = 1;
            }
        }
        break;
    case 10:
        b->flags62 &= ~8;
        FreeGardenerOrder(b->order);
        if (!RunGardenerJob(b))
            NewLongTermAction(b, 0x10);
        break;
    }
}

// FUNCTION: LEGOLAND 0x0049bd20
void Mechanics_Repair(Bloke* b)
{
    Pos     leg;
    RepairJob* o;
    Cell*   cell;
    WClass* cls;
    int     price;
    int     dx, dy;
    unsigned char a;

    switch (b->action) {
    case 0x00:
        b->target = b->saved;
        a = (unsigned char)(CalcMoveLine(b->world, b->saved, b->path) + 0x10);
        b->state = 12;
        b->new_dir = a;
        NewDirForAction(b, (unsigned char)((a >> 5) + 3));
        b->action = 0x0b;
        break;
    case 0x0b:
        dx = b->target.x - b->world.x;
        dy = b->target.y - b->world.y;
        b->action = (dy * dy + dx * dx < 0x10000) ? 0x6b : 0x64;
        break;
    case 0x64:
        switch (FindPathLeg(&b->world, &b->saved, &leg)) {
        case 2:
            b->target.x = leg.x;
            b->target.y = leg.y;
            a = (unsigned char)(CalcMoveLine(b->world, leg, b->path) + 0x10);
            b->state = 12;
            b->new_dir = a;
            NewDirForAction(b, (unsigned char)((a >> 5) + 3));
            b->action = (unsigned char)(0x6a | (~b->f64 & 1));
            break;
        case 1:
            b->target.x = leg.x;
            b->target.y = leg.y;
            a = (unsigned char)(CalcMoveLine(b->world, leg, b->path) + 0x10);
            b->state = 12;
            b->new_dir = a;
            NewDirForAction(b, (unsigned char)((a >> 5) + 3));
            if (b->f64 & 1)
                b->action = 0x6a;
            break;
        case 0:
            WalkOrderSpan(b->order);
            GiveBackMechanicOrder(b->order);
            b->plan = 0x11;
            b->action = 0;
            b->wait = 0x70;
            b->state = 4;
            break;
        }
        break;
    case 0x6a:
        b->wait = 0x70;
        b->state = 4;
        b->action = 0x64;
        break;
    case 0x6b:
        o = b->order;
        b->flags62 |= 8;
        b->dir = o->face;
        cell = MapCellAt(o->pos.x, o->pos.y);
        if (cell->flags & 0x88) {
            if (o->amount >= kOne)
                price = (int)o->amount;
            else
                price = 0;
            if (GetBrickCount() >= price) {
                UseBricks(price);
                o->amount = o->amount - (float)price + o->rate;
                cell = CellForPos(&o->pos);
                cls = ((MapInst*)cell->obj)->cls;
                g_map_dirty |= 0x200;
                RepairCellTick(cell, cls);
                if (cell->life >= cls->max_cond) {
                    cell->flags &= ~0x4000;
                    cell->life = cls->max_cond;
                    b->action++;
                }
            } else {
                b->order->no_money = 1;
                ShowMessage(2);
            }
        } else {
            b->action++;
        }
        break;
    case 0x6c:
        b->flags62 &= ~8;
        b->f46 = 1;
        FreeMechanicOrder(b->order);
        if (!RunMechanicJob(b))
            NewLongTermAction(b, 0x11);
        break;
    }
}

// FUNCTION: LEGOLAND 0x00462ef0
void DoMapAI(void)
{
    int      i;
    int      j;
    int      sum;
    int      v;
    Cell*    cell;
    AIInst*  inst;
    AIClass* cls;
    AIClass* d;

    if (g_ms_flags & 0x3f) {
        for (i = 0; i < 256; i++) {
            switch (g_map_ai.phase) {
            case 0:
                for (j = 0; j < 6; j++) {
                    g_map_ai.cat[j].n_working = 0;
                    g_map_ai.cat[j].n_objects = 0;
                    g_map_ai.cat[j].value = 0;
                    g_map_ai.cat[j].staff = 0;
                }
                g_map_ai.cur_x = 0;
                g_map_ai.cur_y = 0;
                g_map_ai.phase++;
                break;
            case 1:
                cell = MapCellAt(g_map_ai.cur_x, g_map_ai.cur_y);
                if (cell->rf & 1) {
                    g_map_ai.cat[0].n_working++;
                    g_map_ai.cat[0].n_objects++;
                    g_map_ai.cat[0].staff++;
                } else if (cell->flags & 0x88) {
                    inst = (AIInst*)cell->obj;
                    if (inst) {
                        cls = inst->cls;
                        if (cls->category != 0) {
                            g_map_ai.cat[cls->category].n_objects++;
                            if (cell->bx == g_map_ai.cur_x) {
                                if (cell->by == g_map_ai.cur_y) {
                                    if (IsObjectRunningAt(cls, cell->bx, cell->by)) {
                                        g_map_ai.cat[cls->category].n_working++;
                                        g_map_ai.cat[cls->category].value +=
                                            GetObjSalvageValue(cls, cell->life);
                                        g_map_ai.cat[cls->category].staff += cls->staff;
                                    }
                                }
                            }
                        }
                    }
                }
                g_map_ai.cur_x++;
                if (g_map_ai.cur_x >= g_map->width) {
                    g_map_ai.cur_x = 0;
                    g_map_ai.cur_y++;
                    if (g_map_ai.cur_y >= g_map->height)
                        g_map_ai.phase++;
                }
                break;
            case 2:
                g_map_ai.phase = 0;
                g_map_ai.total = 0;
                for (i = 0; i < 6; i++) {
                    g_map_ai.cat[i].f10 = g_map_ai.cat[i].value;
                    g_map_ai.cat[i].f0c = g_map_ai.cat[i].n_objects;
                    g_map_ai.cat[i].f00 = g_map_ai.cat[i].n_working;
                    g_map_ai.cat[i].f08 = g_map_ai.cat[i].staff;
                    g_map_ai.total += g_map_ai.cat[i].f0c;
                }
                g_map_ai.cat[0].f08 = g_map_ai.cat[0].f08 / 100;
                v = g_map_ai.cat[0].f0c * g_map_ai.cat[0].f14 / 100;
                sum = g_map_ai.cat[0].f18 * 100;
                if (v < sum)
                    sum = v;
                v = g_map_ai.cat[1].f08 * g_map_ai.cat[1].f14;
                if (v >= g_map_ai.cat[1].f18 * 100)
                    v = g_map_ai.cat[1].f18 * 100;
                sum += v;
                v = g_map_ai.cat[4].f08 * g_map_ai.cat[4].f14;
                if (v >= g_map_ai.cat[4].f18 * 100)
                    v = g_map_ai.cat[4].f18 * 100;
                sum += v;
                v = g_map_ai.cat[5].f08 * g_map_ai.cat[5].f14;
                if (v >= g_map_ai.cat[5].f18 * 100)
                    v = g_map_ai.cat[5].f18 * 100;
                sum += v;
                sum = sum / 100;
                g_map_ai.income = sum;
                if (sum < g_map_ai.income_min) {
                    sum = g_map_ai.income_min;
                    g_map_ai.income = sum;
                }
                if (sum > g_map_ai.income_max) {
                    sum = g_map_ai.income_max;
                    g_map_ai.income = sum;
                }
                if (sum > ((MapCap*)g_map)->cap)
                    g_map_ai.income = ((MapCap*)g_map)->cap;
                break;
            }
        }
    } else {
        for (i = 0; i < 6; i++) {
            if (i)
                g_map_ai.cat[i].defs = 0;
        }
        d = g_objdef_head;
        while (d) {
            if (d->category && d->f8)
                g_map_ai.cat[d->category].defs++;
            d = d->next;
        }
    }
}

/* One neighbour probe: copy the cell (an off-map cell stands in as
 * flags=0x40/rf=0) and apply SetPathFlag's walkable-path predicate. A MACRO
 * over one function-level `cell`, not a helper with its own local: VC6 keeps
 * each off-map `cell.rf = 0` store alive because the NEXT probe reads
 * cell.rf back after its copy (it does not treat the rep movsd as a kill),
 * and drops only the last probe's — exactly the original's pattern. */
#define PROBE(px, py) \
    if ((px) >= 0 && (px) < g_map->width && (py) >= 0 && (py) < g_map->height) { \
        cell = g_map_rows[(py)][(px)]; \
    } else { \
        cell.flags = 0x40; \
        cell.rf = 0; \
    } \
    rf = cell.rf; \
    if ((rf & 1) || ((cell.flags & 0x10) && !(rf & 2)))

/* The 8-neighbour path mask (see pathtile2.c's header for the bit layout):
 * N, S, E, W then NE, SE, SW, NW; the orthogonal and diagonal hit counts go
 * back through the optional pointers. Twin of GetPathNeighbours 0x0045c440.
 * bnvmove.c declares the two pointer parameters as int (it passes 0, 0). */
// FUNCTION: LEGOLAND 0x0045c050
unsigned char Get_Path_Directions(Pos* pos, unsigned char* n_ortho, unsigned char* n_diag)
{
    Cell cell;
    unsigned char rf;
    unsigned char diag = 0;
    unsigned char ortho = 0;
    unsigned char mask = 0;
    int x = pos->x;
    int y = pos->y;
    int ym1 = y - 1;
    int yp1;
    int xp1;
    int xm1;

    PROBE(x, ym1)   { ortho++; mask |= 0x01; }
    yp1 = y + 1;
    PROBE(x, yp1)   { ortho++; mask |= 0x10; }
    xp1 = x + 1;
    PROBE(xp1, y)   { ortho++; mask |= 0x04; }
    xm1 = x - 1;
    PROBE(xm1, y)   { ortho++; mask |= 0x40; }
    PROBE(xp1, ym1) { diag++;  mask |= 0x02; }
    PROBE(xp1, yp1) { diag++;  mask |= 0x08; }
    PROBE(xm1, yp1) { diag++;  mask |= 0x20; }
    PROBE(xm1, ym1) { diag++;  mask |= 0x80; }
    if (n_ortho)
        *n_ortho = ortho;
    if (n_diag)
        *n_diag = diag;
    return mask;
}
