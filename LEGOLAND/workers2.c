/* LEGOLAND — work orders, worker generation and the build / repair jobs.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; the field and type names are ours.
 * Each matched function carries its original VA marker on the line above it.
 *
 * ---------------------------------------------------------------------------
 * HIRING A WORKER
 *
 * GenerateGardener / GenerateMechanic hire one worker: the hired lists
 * (0x0079a8a8 / 0x0079a8ac, counts 0x0079a8bc / 0x0079a8cc) are capped at 15
 * each. A worker is a NewBlokeWOList bloke (kind 2 = gardener, 3 = mechanic)
 * with heading 0, walk delay 1 and +0x7f = 0x18, pushed on the front of its
 * list. Where it appears depends on the second argument:
 *   in_hut != 0  the worker is spawned inside a staff hut: PutWorkerOnRide
 *                gets the hut's map cell, the world position is set from the
 *                hut tile (the mechanic half a tile to the left, the gardener
 *                after the hut position has been nudged (-2,+1) IN PLACE — the
 *                caller's Pos is modified) and the plan is 5 (leave the hut).
 *   in_hut == 0  the worker is dropped straight at the tile and goes idle
 *                (plan 0x10 gardener / 0x11 mechanic).
 * A gardener hire always raises g_map_dirty bit 0x80; the mechanic does not.
 *
 * ---------------------------------------------------------------------------
 * REPAIR ORDERS
 *
 * AddRepairOrderForObject(cls, pos) raises the repair for a worn object cell:
 * the cost is GetObjRepairCost(cls, cell->life) and the amount charged per
 * tick is cost / (max_cond - life) as a float. The order goes to the gardener
 * list (class flag 0x200000 and the map's gardener service on), else the
 * mechanic list (0x400000 / mechanic service), else to the park-funded
 * "none-worker" list which carries the per-tick amount and the class's
 * footprint rect (WClass +0x3c). The cell is fetched WITHOUT a null check —
 * an off-map position dereferences 0x11 (reproduced).
 *
 * IterateNoneWorkersRepairOrders is the per-frame tick of the park-funded
 * list: for each order it projects the embedded rect (biased by the order's
 * x/y) through GetTileBounds to find the screen centre, then either charges
 * `amount` bricks (when amount is under 1.0 nothing is charged: the (int)
 * cast is skipped and the price is 0), adds the fractional remainder plus
 * +0x24 back onto the running amount and draws the "paying" sprite at
 * 0x007fe004, and — once the cell's life has reached the class maximum —
 * clears CF_REPAIRORDER (0x4000), restores the life and frees the order; or,
 * when the park cannot afford it, animates and draws the "no money" sprite
 * at 0x007fdeb0 instead.
 *
 * ---------------------------------------------------------------------------
 * THE BUILD JOB STATE MACHINE (Gardener_Build / Mechanic_Build)
 *
 * Both handlers are one switch on Bloke::action (+0x60). The bloke's current
 * order is at +0x50, its walk target at +0x24/+0x28, and the order's tile at
 * WorkOrder +0x08. The steps (gardener / mechanic where they differ):
 *   0x00  aim at the saved target (+0x2c/+0x30): CalcMoveLine, state 12
 *         (walking), turn, action 0x0b
 *   0x0b  arrived within 0x9000 (gardener) / 0x10000 (mechanic) squared
 *         world units? -> 0x64/0x65 : 0x6b
 *   0x64/0x65  ask the path finder (0x00482710) for the next leg:
 *         2 = walk it, action 0x6a|(!f64&1); 1 = walk it, action 0x6a only
 *         when f64 bit 0 is set; 0 = no route: give the order back
 *         (0x00499e60 / 0x00499f40), plan 0x10/0x11, state 4 with a 0x70
 *         wait
 *   0x6a  wait 0x70 ticks in state 4, then back to 0x64/0x65
 *   0x6b  gardener: start the build animation (anim 1, frame 0x28), flag
 *         0x100; mechanic: try BuildObject(order->obj, &order->pos) — on
 *         success clear +0x46 and advance, else drop the order and go idle
 *         with order->f30 = 1
 *   0x6c  gardener: play the animation; when it completes go back to the walk
 *         animation, clear 0x100, advance. mechanic: wait until the object's
 *         cell no longer carries the "building" flag 0x20
 *   0x6d  gardener: BuildObject; on success free the order and look for the
 *         next job (RunGardenerJob) or idle; on failure give the order back,
 *         idle and raise message 1. mechanic: free the order, +0x46 = 1, next
 *         job or idle.
 * --------------------------------------------------------------------------- */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* A "bloke" (worker/visitor); allocation stride 172 (0xac). */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00  intrusive list link */
    void*          person;      /* +0x04  the renderable 3D person */
    unsigned char  pad08[4];    /* +0x08..0x0b */
    unsigned short plan;        /* +0x0c  long-term action (plan) */
    unsigned short state;       /* +0x0e  low-level AI state */
    unsigned char  pad10[0x14]; /* +0x10..0x23 */
    Pos            target;      /* +0x24  walk target (24.8 world units) */
    Pos            saved;       /* +0x2c  saved target (the job's tile) */
    unsigned char  pad34[0x12]; /* +0x34..0x45 */
    unsigned short f46;         /* +0x46 */
    unsigned char  pad48[8];    /* +0x48..0x4f */
    struct WorkOrder* order;    /* +0x50  current work order */
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
    unsigned char  f74;         /* +0x74 */
    unsigned char  walk_delay;  /* +0x75 */
    unsigned char  pad76[9];    /* +0x76..0x7e */
    unsigned char  f7f;         /* +0x7f */
    unsigned char  pad80[0x18]; /* +0x80..0x97 */
    unsigned char  path[0x14];  /* +0x98  move-line scratch (CalcMoveLine) */
} Bloke;

/* What the path finder (0x00482710) hands back: the next leg's tile. */
typedef struct PathLeg {
    int f0;    /* +0x00 */
    int x;     /* +0x04 */
    int y;     /* +0x08 */
} PathLeg;

/* A gardener or mechanic work order — calloc(0x3c,1); see workorder.c. */
typedef struct WorkOrder {
    struct WorkOrder* next;     /* +0x00 */
    void*             obj;      /* +0x04  target object */
    Pos               pos;      /* +0x08  map cell */
    Rect*             rects;    /* +0x10 */
    int               nrects;   /* +0x14 */
    int               assigned; /* +0x18 */
    Bloke*            worker;   /* +0x1c */
    unsigned char     kind;     /* +0x20  1 = object, 2 = path span */
    unsigned char     pad21[3]; /* +0x21..0x23 */
    int               ox;       /* +0x24 */
    int               oy;       /* +0x28 */
    int               f2c;      /* +0x2c */
    int               f30;      /* +0x30 */
    int               pad34[2]; /* +0x34..0x3b */
} WorkOrder;

/* A park-funded repair order — malloc(0x28); see workorder.c. */
typedef struct RepairOrder {
    struct RepairOrder* next;   /* +0x00 */
    Rect                rect;   /* +0x04 */
    Pos                 pos;    /* +0x18 */
    float               amount; /* +0x20  bricks owed this tick */
    float               rate;   /* +0x24  added back each tick */
} RepairOrder;

/* The object class record: flags at +0x1c, max condition at +0x2c, the
 * footprint rect at +0x3c, the descriptor the order lists key on at +0xc4. */
typedef struct WClass {
    unsigned char pad00[0x1c];  /* +0x00..0x1b */
    unsigned int  flags;        /* +0x1c  0x200000 gardener, 0x400000 mechanic */
    unsigned char pad20[0x0c];  /* +0x20..0x2b */
    unsigned char max_cond;     /* +0x2c */
    unsigned char pad2d[0x0f];  /* +0x2d..0x3b */
    Rect          rect;         /* +0x3c */
    unsigned char pad50[0x74];  /* +0x50..0xc3 */
    void*         desc;         /* +0xc4 */
} WClass;

/* A placed object as the cell points at it: its class at +0x0c. */
typedef struct MapInst {
    unsigned char pad00[0x0c];  /* +0x00..0x0b */
    WClass*     cls;          /* +0x0c */
} MapInst;

/* The map header with the worker-service switches at +0x34/+0x38. */
typedef struct MapServices {
    unsigned char pad00[0x34];  /* +0x00..0x33 */
    int           mechanics;    /* +0x34 */
    int           gardeners;    /* +0x38 */
} MapServices;

/* The inclusive pixel rectangle GetTileBounds fills (pathbuild.c). */
typedef struct TileBounds {
    int left;   /* +0x00 */
    int top;    /* +0x04 */
    int right;  /* +0x08 */
    int bottom; /* +0x0c */
} TileBounds;

/* The map tile an in-progress build occupies, passed BY VALUE as two bytes,
 * and the same two bytes as the 16-bit key the build-slot table is searched
 * by (buildtick.c). */
typedef struct BuildTile {
    unsigned char x; /* +0x00 */
    unsigned char y; /* +0x01 */
} BuildTile;
typedef union BuildKey {
    BuildTile tile;
    short     w;
} BuildKey;

/* One construction slot — 12 bytes, 256 of them at 0x006664f8. */
typedef struct BuildSlot {
    void*    obj;   /* +0x00  object under construction (0 = free slot) */
    BuildKey key;   /* +0x04  map cell it was placed on */
    short    pad6;  /* +0x06 */
    int      timer; /* +0x08  ticks elapsed since construction started */
} BuildSlot;

/* The object descriptor as DoBuildEffects sees it: the progress-bar anchor
 * offsets at +0x3c..+0x48. */
typedef struct BuildObj {
    unsigned char pad00[0x3c]; /* +0x00..0x3b */
    int           bar_x0;      /* +0x3c */
    int           bar_y1;      /* +0x40 */
    int           bar_x1;      /* +0x44 */
    int           bar_y0;      /* +0x48 */
} BuildObj;

/* A sprite object: its LLS holder at +0x08 (legoland.h SpriteObj). */
typedef struct WSprite {
    int    pad0;       /* +0x00 */
    int    pad4;       /* +0x04 */
    void** lls_holder; /* +0x08 */
} WSprite;

/* A sound "source" descriptor: kind 2 = a map tile at (x,y). */
typedef struct SoundSource {
    int kind;  /* +0x00 */
    int pad4;  /* +0x04 */
    int x;     /* +0x08 */
    int y;     /* +0x0c */
} SoundSource;

/* ---------------------------------------------------------------- globals -- */

extern BuildSlot     g_build_slots[256];     /* 0x006664f8 */
extern int           g_build_fx_off;         /* 0x0066895c  non-zero: no build overlay */
extern void*         g_placing_obj;          /* 0x0080ff64 */
extern int           g_placed_flag;          /* 0x0079a8d0 */
extern int           g_build_quiet;          /* 0x00667cd8  non-zero: no placement sound */
extern void*         g_place_sample;         /* 0x004b9248 */
extern void*         g_obj_list;             /* 0x007febc0  head of the placed-object list */
extern WSprite*      g_paying_sprite;        /* 0x007fe004 */
extern WSprite*      g_nomoney_sprite;       /* 0x007fdeb0 */

/* The input/cursor block at 0x00813a40 as this file sees it. */
typedef struct CursorState {
    int           flags;        /* +0x00  0x813a40 */
    Pos           point;        /* +0x04  0x813a44  cursor point (screen) */
    int           y2;           /* +0x0c  0x813a4c */
    unsigned char buttons;      /* +0x10  0x813a50  bit 2 = worker drop */
    unsigned char pad11[0x0f];  /* +0x11..0x1f */
    unsigned char state;        /* +0x20  0x813a60  bit 2 = cancel */
} CursorState;
extern CursorState   g_cursor;               /* 0x00813a40 */
extern int           g_hit_type;             /* 0x004bdd00  mouse-hit record type */
extern int           g_icon_clicked;         /* 0x00667c48 */
extern int           g_drag_lock;            /* 0x00668954 */
extern Bloke*        g_worker_on_mouse;      /* 0x007fdff0 */
extern int           g_worker_on_mouse_type; /* 0x007fdffc  0x307 = gardener */

/* The map header again with the viewport origin at +0x20. */
typedef struct MapHdr {
    unsigned char  pad00[0x20]; /* +0x00..0x1f */
    unsigned short origin_x;    /* +0x20 */
    unsigned short origin_y;    /* +0x22 */
} MapHdr;

extern Bloke*        g_gardener_list;        /* 0x0079a8a8 */
extern Bloke*        g_mechanic_list;        /* 0x0079a8ac */
extern int           g_gardener_count;       /* 0x0079a8bc */
extern int           g_mechanic_count;       /* 0x0079a8cc */
extern RepairOrder*  g_repair_orders;        /* 0x0079a8d4 */
extern int           g_map_dirty;            /* 0x00668610 */

/* ------------------------------------------------------------ prototypes -- */

extern void   DBPrintf(const char* fmt, ...);                     /* 0x00453a20 */
extern Bloke* NewBlokeWOList(int kind);                           /* 0x00482f70 */
extern void   PutWorkerOnRide(Bloke* b, Cell* cell);              /* 0x0049a0d0 */
extern void   NewLongTermAction(Bloke* b, int action);            /* 0x0044e760 */
extern int    GetObjRepairCost(WClass* cls, int life);          /* 0x00480de0 */
extern WorkOrder* GetMechanicWorkOrderAt(int x, int y);           /* 0x0049b180 */
/* Add an order on `desc` at `pos` of `kind` to the gardener / mechanic list
 * (unexported). */
extern WorkOrder* AddGardenerWorkOrder(void* desc, Pos* pos, int kind);  /* 0x00499780 */
extern WorkOrder* AddMechanicWorkOrder(void* desc, Pos* pos, int kind);  /* 0x00499830 */
/* Add a park-funded repair order (unexported). */
extern WorkOrder* AddNoneWorkersRepairOrder(Rect* r, Pos* pos, float amount); /* 0x0049b690 */
/* Nearest broken cell a mechanic could repair from (x,y) (unexported). */
extern Cell*  FindBrokenCellNear(Bloke* m, int x, int y);         /* 0x0049b350 */
extern int    RunMechanicJob(Bloke* m);                           /* 0x00499d30 (unexported) */
extern void   AssignMechanicOrder(Bloke* m, WorkOrder* o);        /* 0x00499b60 (unexported) */

extern void   GetTileBounds(Pos* tile, TileBounds* out);         /* 0x0045acc0 */
/* Always 0 in the shipped binary (0x00450c70, unexported): the build overlay
 * is compiled out. */
extern int    WantBuildEffects(void* obj);                        /* 0x00450c70 */
extern int    GetBuildTime(void* obj);                            /* 0x00450c40 */
extern int    RenderBlock(int x, int y, int w, int h, int colour); /* 0x004890c0 */
extern void   RenderBox(int x, int y, int w, int h, int colour);  /* 0x00489410 */
extern int    GetNearestColour(int r, int g, int b);              /* 0x0044e6c0 */
extern int    GetObjCost(void* cls);                              /* 0x00480da0 */
extern int    GetBrickCount(void);                                /* 0x004578e0 */
extern void   UseBricks(int n);                                   /* 0x004578c0 */
extern void   SetObjRectFlags(void* desc, Pos* pos, int flags);   /* 0x0045dee0 */
extern int    BuildObject(void* desc, Pos* pos);                  /* 0x0045eb30 */
extern void   ShowMessage(int which);                             /* 0x004735e0 */
extern void   ClearObjFootprint(void* desc, Pos* pos);            /* 0x0045e300 */
extern void   RefreshObjList(void* head);                         /* 0x0045d770 */
extern void*  PlayInstanceOfSample(void* def, int a, int b, SoundSource* src); /* 0x00496d20 */
extern Bloke* FindFreeGardener(Pos* pos);                         /* 0x00499c40 (unexported) */
extern void   AssignGardenerOrder(Bloke* g, WorkOrder* o);        /* 0x00499ac0 (unexported) */
extern int    PrintSprite(WSprite* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern void   LLSNextFrame(void* lls);                            /* 0x0047d5d0 */
/* Wear one point of life back onto a cell under repair; returns the new life
 * (unexported). */
extern void   RepairCellTick(Cell* cell, WClass* cls);            /* 0x0049b0d0 */
extern void   FreeRepairOrder(RepairOrder* r);                    /* 0x0049b6e0 */

/* Next leg of a route from `from` to `to` (world coords), unexported:
 * 0 = no route, 1 = a leg that keeps the current heading, 2 = a new leg. */
extern int    FindPathLeg(Pos* from, Pos* to, Pos* out);             /* 0x00482710 */
/* Set up the straight-line walk from (x0,y0) to (x1,y1); returns the
 * heading angle (0..255). */
extern int    CalcMoveLine(Pos from, Pos to, void* path);         /* 0x00480740 */
extern int    NewDirForAction(Bloke* b, unsigned char dir);       /* 0x004833d0 */
extern void   BlokeSetAnim(Bloke* b, int anim);                   /* 0x004406c0 */
extern void   BlokeSetFrame(Bloke* b, int frame);                 /* 0x00440870 */
extern int    PlayBlokeAnim(Bloke* b);                            /* 0x004408a0 */
extern void   BlokeWalkAnim(Bloke* b);                            /* 0x00440910 */
extern int    RunGardenerJob(Bloke* g);                           /* 0x00499d00 (unexported) */
/* Put an order back on its list unassigned (unexported). */
extern void   GiveBackGardenerOrder(WorkOrder* o);                /* 0x00499e60 */
extern void   GiveBackMechanicOrder(WorkOrder* o);                /* 0x00499f40 */
extern void   FreeGardenerOrder(WorkOrder* o);                    /* 0x00499e30 */
extern void   FreeMechanicOrder(WorkOrder* o);                    /* 0x00499eb0 */
extern void   ResetWorkersOldCoords(void);                        /* 0x004708d0 */
extern void   SetWorkersPositionAtMouse(void);                    /* 0x004701f0 */
extern void   ScreenToMapRef(Pos* screen, Pos* map, int mode);    /* 0x0045be90 */
extern int    SetGardenerWorkOrderAtPostion(Bloke* g, int x, int y); /* 0x0049b2c0 */
/* Mouse-hit helpers for a carried worker (unexported): is the hit a ride the
 * worker can be put on; the work order under the hit (its tile in *cell). */
extern int    WorkerHitOnRide(void);                              /* 0x00470270 */
extern WorkOrder* WorkOrderUnderHit(Pos* cell);                   /* 0x00470410 */
extern WorkOrder* WorkOrderNearHit(Pos* cell);                    /* 0x004704b0 */

/* ---------------------------------------------------------- inline helpers -- */

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

/* Raise the repair order for the worn object of class `cls` at `pos`. */
// FUNCTION: LEGOLAND 0x0049b930
WorkOrder* AddRepairOrderForObject(WClass* cls, Pos pos)
{
    int   life = MapCellAt(pos.x, pos.y)->life;   /* no null check (original) */
    int   cost = GetObjRepairCost(cls, life);
    float amount = (float)cost / (float)(cls->max_cond - life);

    if ((cls->flags & 0x200000) && ((MapServices*)g_map)->gardeners)
        return AddGardenerWorkOrder(cls->desc, &pos, 2);
    if ((cls->flags & 0x400000) && ((MapServices*)g_map)->mechanics)
        return AddMechanicWorkOrder(cls->desc, &pos, 2);
    return AddNoneWorkersRepairOrder(&cls->rect, &pos, amount);
}

/* Hire a mechanic (at most 15). */
// FUNCTION: LEGOLAND 0x0049a340
Bloke* GenerateMechanic(Pos* pos, int in_hut)
{
    Bloke* b;

    if (g_mechanic_count >= 15)
        return 0;
    b = NewBlokeWOList(3);
    b->f70 = 0;
    b->dir = 0;
    b->f74 = 0;
    b->walk_delay = 1;
    b->f7f = 0x18;
    b->next = g_mechanic_list;
    b->f46 = 1;
    g_mechanic_list = b;
    g_mechanic_count++;
    if (in_hut) {
        Cell* cell = CellForPos(pos);
        PutWorkerOnRide(b, cell);
        b->world.x = (pos->x << 8) - 0x80;
        b->target.x = b->world.x;
        b->world.y = pos->y << 8;
        b->target.y = b->world.y;
        NewLongTermAction(b, 5);
    } else {
        b->world.x = pos->x << 8;
        b->world.y = pos->y << 8;
        NewLongTermAction(b, 0x11);
    }
    return b;
}

/* Hire a gardener (at most 15). */
// FUNCTION: LEGOLAND 0x0049a1a0
Bloke* GenerateGardener(Pos* pos, int in_hut)
{
    Bloke* b;
    Cell*  cell;

    if (g_gardener_count >= 15)
        return 0;
    DBPrintf("Generating Gardener\n");
    b = NewBlokeWOList(2);
    cell = 0;
    if (b) {
        b->f70 = 0;
        b->dir = 0;
        b->f74 = 0;
        b->walk_delay = 1;
        b->f7f = 0x18;
        b->next = g_gardener_list;
        g_gardener_list = b;
        g_gardener_count++;
        if (in_hut) {
            int x = pos->x;
            if (x >= 0 && x < g_map->width) {
                int y = pos->y;
                if (y >= 0 && y < g_map->height)
                    cell = &g_map_rows[y][x];
            }
            DBPrintf("   Gardener Generated in hut\n");
            PutWorkerOnRide(b, cell);
            pos->x -= 2;
            pos->y += 1;
            b->world.x = pos->x << 8;
            b->target.x = b->world.x;
            b->world.y = pos->y << 8;
            b->target.y = b->world.y;
            NewLongTermAction(b, 5);
        } else {
            DBPrintf("   Gardener Generated at (%d,%d)\n", pos->x, pos->y);
            b->world.x = pos->x << 8;
            b->world.y = pos->y << 8;
            NewLongTermAction(b, 0x10);
        }
        g_map_dirty |= 0x80;
    } else {
        DBPrintf("   Failed to Generate Gardener\n");
    }
    return b;
}

/* Click a mechanic onto a tile: take over the order there (displacing a
 * holder that has not started), raise a repair for a broken object nearby,
 * or go looking for work. SetGardenerWorkOrderAtPostion's twin (workers.c);
 * note both order kinds use the 0x6b "not started" threshold here. */
// FUNCTION: LEGOLAND 0x0049b430
int SetMechanicsOrderAtPostion(Bloke* m, int x, int y)
{
    WorkOrder* o = GetMechanicWorkOrderAt(x, y);

    if (!o) {
        Cell* cell = FindBrokenCellNear(m, x, y);
        if (cell) {
            /* `at` is built BEFORE the class is fetched: reading cls first
             * swaps the eax/edx roles of the x and class pushes. */
            WClass*    cls;
            Pos        at;
            WorkOrder* r;
            at.x = cell->bx;
            at.y = cell->by;
            cls = ((MapInst*)cell->obj)->cls;
            r = AddRepairOrderForObject(cls, at);
            if (r) {
                cell->flags |= 0x4000;
                AssignMechanicOrder(m, r);
                return 1;
            }
        } else {
            if (!RunMechanicJob(m)) {
                NewLongTermAction(m, 0x11);
                return 1;
            }
        }
    } else {
        if (o->assigned) {
            if (o->kind == 1 && o->worker->action < 0x6b)
                NewLongTermAction(o->worker, 0x11);
            else if (o->kind == 2 && o->worker->action < 0x6b)
                NewLongTermAction(o->worker, 0x11);
            else
                return 0;
        }
        AssignMechanicOrder(m, o);
    }
    return 1;
}

/* Draw the construction progress bar over a building object: a black block
 * 65x11 centred between the object's two anchor tiles, a white outline and a
 * green fill proportional to the slot's elapsed time. The helper at
 * 0x00450c70 always returns 0, so this never draws in the shipped game. */
// FUNCTION: LEGOLAND 0x00450d90
void DoBuildEffects(BuildObj* obj, BuildKey key)
{
    Pos        p;
    TileBounds b;
    int        left1;
    int        top1;
    int        cx;
    int        cy;
    int        x0, y0, x1, y1, w, h;
    int        i;

    if (!WantBuildEffects(obj))
        return;
    if (g_build_fx_off)
        return;
    p.x = obj->bar_x0 + key.tile.x;
    p.y = obj->bar_y0 + key.tile.y;
    GetTileBounds(&p, &b);
    left1 = b.left;
    top1 = b.top;
    p.x = obj->bar_x1 + key.tile.x;
    p.y = obj->bar_y1 + key.tile.y;
    GetTileBounds(&p, &b);
    cx = (left1 + b.right) / 2;
    cy = (top1 + b.bottom) / 2;
    x0 = cx - 32;
    x1 = cx + 32;
    y0 = cy - 5;
    y1 = cy + 5;
    w = x1 - x0 + 1;
    h = y1 - y0 + 1;
    for (i = 0; i < 256; i++) {
        if (g_build_slots[i].key.w == key.w)
            break;
    }
    if (i < 256) {
        RenderBlock(x0, y0, w, h, 0);
        RenderBox(x0, y0, w, h, GetNearestColour(255, 255, 255));
        RenderBlock(x0 + 1, y0 + 1,
                    (w - 2) * g_build_slots[i].timer / GetBuildTime(obj),
                    h - 2, GetNearestColour(0, 255, 0));
    }
}

/* Place an object through the work-order system: a gardener-built object
 * (class flag 0x200000 with the gardener service on) becomes a kind-1
 * gardener order with its footprint flagged 0x800 and is handed to a free
 * gardener; anything else is paid for and built on the spot (message 3 when
 * the park cannot afford it). Both paths play the placement sample unless
 * the quiet flag is set. Returns non-zero when placed. */
// FUNCTION: LEGOLAND 0x0049ab30
int WorkOrderBuildObject(MapInst* desc, Pos* pos)
{
    WClass*     cls = desc->cls;
    int         built;
    WorkOrder*  o;
    SoundSource src;

    if (desc == g_placing_obj && g_placed_flag)
        return 0;
    if ((cls->flags & 0x200000) && ((MapServices*)g_map)->gardeners) {
        o = AddGardenerWorkOrder(desc, pos, 1);
        built = o != 0;
        if (built) {
            Bloke* g;
            SetObjRectFlags(desc, pos, 0x800);
            g = FindFreeGardener(pos);
            if (g)
                AssignGardenerOrder(g, o);
        }
    } else {
        int cost = GetObjCost(cls);
        if (GetBrickCount() < cost) {
            ShowMessage(3);
            return 0;
        }
        ClearObjFootprint(desc, pos);
        RefreshObjList(&g_obj_list);
        built = BuildObject(desc, pos);
    }
    if (built && !g_build_quiet) {
        src.kind = 2;
        src.x = pos->x;
        src.y = pos->y;
        PlayInstanceOfSample(g_place_sample, 0, 1, &src);
    }
    return built;
}

/* Per-frame tick of the park-funded repair orders (see the header). */
// FUNCTION: LEGOLAND 0x0049b750
void IterateNoneWorkersRepairOrders(void)
{
    RepairOrder* p = g_repair_orders;
    RepairOrder* next;
    Pos          pt;
    TileBounds   b;
    TileBounds   r;      /* the order's rect in map cells */
    int          l1, r2, t3;
    int          cx, cy;
    int          price;
    Cell*        cell;
    WClass*      cls;

    while (p) {
        next = p->next;
        r.left = p->rect.left + p->pos.x;
        r.top = p->rect.top + p->pos.y;
        r.right = p->rect.right + p->pos.x;
        r.bottom = p->rect.bottom + p->pos.y;
        pt.x = r.left;
        pt.y = r.bottom;
        GetTileBounds(&pt, &b);
        l1 = b.left;
        pt.x = r.right;
        pt.y = r.top;
        GetTileBounds(&pt, &b);
        r2 = b.right;
        pt.x = r.left;
        pt.y = r.top;
        GetTileBounds(&pt, &b);
        t3 = b.top;
        pt.x = r.right;
        pt.y = r.bottom;
        GetTileBounds(&pt, &b);
        cx = (l1 + r2) / 2;
        cy = (t3 + b.bottom) / 2;
        if (p->amount >= 1.0)
            price = (int)p->amount;
        else
            price = 0;
        if (GetBrickCount() >= price) {
            UseBricks(price);
            p->amount = p->amount - (float)price + p->rate;
            PrintSprite(g_paying_sprite, cx, cy, 0, 0);
            cell = CellForPos(&p->pos);
            cls = ((MapInst*)cell->obj)->cls;
            g_map_dirty |= 0x200;
            RepairCellTick(cell, cls);
            if (cell->life >= cls->max_cond) {
                cell->flags &= ~0x4000;
                cell->life = cls->max_cond;
                FreeRepairOrder(p);
            }
        } else {
            LLSNextFrame(*g_nomoney_sprite->lls_holder);
            PrintSprite(g_nomoney_sprite, cx, cy, 0, 0);
        }
        p = next;
    }
}

// FUNCTION: LEGOLAND 0x0049a4e0
void Gardener_Build(Bloke* b)
{
    Pos     leg;
    Pos     pos;
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
        b->action = (dy * dy + dx * dx < 0x9000) ? 0x6b : 0x64;
        break;
    case 0x64: {
        
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
            GiveBackGardenerOrder(b->order);
            b->plan = 0x10;
            b->action = 0x64;
            b->wait = 0x70;
            b->state = 4;
            break;
        }
        break;
    }
    case 0x6a:
        b->wait = 0x70;
        b->state = 4;
        b->action = 0x64;
        break;
    case 0x6b:
        b->flags62 |= 0x100;
        BlokeSetAnim(b, 1);
        BlokeSetFrame(b, 0x28);
        b->action++;
        break;
    case 0x6c:
        if (PlayBlokeAnim(b)) {
            BlokeWalkAnim(b);
            b->flags62 &= ~0x100;
            b->action++;
        }
        break;
    case 0x6d:
        b->flags62 &= ~8;
        pos = b->order->pos;
        if (BuildObject(b->order->obj, &pos)) {
            FreeGardenerOrder(b->order);
            if (!RunGardenerJob(b))
                NewLongTermAction(b, 0x10);
        } else {
            GiveBackGardenerOrder(b->order);
            NewLongTermAction(b, 0x10);
            ShowMessage(1);
        }
        break;
    }
}

// FUNCTION: LEGOLAND 0x0049a7f0
void Mechanic_Build(Bloke* b)
{
    Pos     leg;
    Pos     pos;
    Cell*   cell;
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
        b->action = (dy * dy + dx * dx < 0x10000) ? 0x6b : 0x65;
        break;
    case 0x65: {
        
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
            GiveBackMechanicOrder(b->order);
            b->plan = 0x11;
            b->action = 0x65;
            b->wait = 0x70;
            b->state = 4;
            break;
        }
        break;
    }
    case 0x6a:
        b->wait = 0x70;
        b->state = 4;
        b->action = 0x65;
        break;
    case 0x6b:
        b->flags62 |= 8;
        pos = b->order->pos;
        if (BuildObject(b->order->obj, &pos)) {
            b->f46 = 0;
            b->action++;
        } else {
            GiveBackMechanicOrder(b->order);
            NewLongTermAction(b, 0x11);
            b->order->f30 = 1;
        }
        break;
    case 0x6c:
        b->order->f30 = 0;
        pos = b->order->pos;
        cell = MapCellAt(pos.x, pos.y);     /* no null check (original) */
        if (!(cell->flags & 0x20))
            b->action++;
        break;
    case 0x6d:
        b->flags62 &= ~8;
        b->f46 = 1;
        FreeMechanicOrder(b->order);
        if (!RunMechanicJob(b))
            NewLongTermAction(b, 0x11);
        break;
    }
}

/* Per-tick handling of the worker carried on the mouse (see header).
 *
 * Codegen notes.  Every block is in the original's order and the trimmed body
 * is exactly 672 bytes, as the original; a difflib alignment puts 181 of the
 * 184 instructions together (98.4%) with exactly two difference hunks (see
 * RESIDUAL).
 *  - The work order found under/near the hit is a SEPARATE local from the
 *    `o` parameter: the original homes it in the (by then dead) parameter
 *    slot [esp+18h] and reloads it at the `place` join even on the path
 *    where nothing was found, so the value read there is the uninitialised
 *    local -- harmless because `found` is 0 on that path. Reusing `o` for it
 *    instead enregisters the parameter in edi, costs a fourth callee-saved
 *    push and hoists a `2` into ecx for the flag tests.
 *  - VC6 only lifts the constant 1 into a callee-saved register (ebp here,
 *    rematerialised wherever ebp is reused as scratch) once the function
 *    contains at least four separate stores of 1 to memory; that is why the
 *    0x103 arm's failure spells out `g_drag_lock = 1; ...` instead of
 *    sharing the `fail:` label. It is exactly equivalent to `goto fail`.
 *  - The path-flag arm below carries NO gotos: VC6 jump-threads it. After
 *    `g_drag_lock = 1` the join's `if (g_drag_lock)` is known true, so the
 *    two exits become direct jumps to the tail's call (0x47078b/0x470795 ->
 *    0x4708b4); after `g_drag_lock = 0` with the type already known to be
 *    0x307 the join and the type test are both threaded away and the jump
 *    lands inside the place block, on the gardener arm (0x4707a1 ->
 *    0x4707db).  Spelling those threads out as `goto tail` / `goto gardener`
 *    compiles to the same code but is not what the original says.
 *  - The out-of-range row arm must be the bare `g_drag_lock = 1;` shown
 *    below.  Writing what it threads into (`SetWorkersPositionAtMouse();
 *    return;`) makes VC6 SINK the whole arm past the place block to sit in
 *    front of `fail:`, which costs an extra jmp and puts eight instructions
 *    in the wrong place (that was the previous 677-byte reconstruction).
 *  - RESIDUAL, two instructions (both single-instruction spelling choices;
 *    every block, every branch target and the 672-byte length are right):
 *      (1) 0x4707a3: the original rematerialises the constant into ebp for
 *          the out-of-range arm's store -- `mov ebp,1; mov [g_drag_lock],ebp`
 *          where we emit the one-instruction `mov [g_drag_lock],1`.  ebp
 *          holds the row count on entry to the arm (`xor ebp,ebp;
 *          mov bp,[map+16h]`), so a remat is needed in both; VC6 folds ours
 *          into the immediate because the constant dies at the arm's end
 *          (the arm threads straight into the tail's call and epilogue).
 *          The register form appears exactly where the constant is LIVE-OUT:
 *          the path-flag arm above (0x470774) falls through to the join and
 *          keeps `mov ebp,1`, in ours as in the original.
 *          Ruled out: a named `one` local (constant-propagated away),
 *          `found + 1`, `g_icon_clicked`, `goto fail` / `goto tail` from the
 *          arm, `fail:` moved inside the arm, swapping the two arms, an
 *          `if/else` written as `if (y >= h) ... else ...`, a FIFTH store of
 *          1 elsewhere in the function, and (new this round) dead stores in
 *          the arm meant to keep the constant alive (`found = 1;`, `r = 1;`,
 *          `w = 0;` -- all DCE'd before the fold), `found = 1;
 *          g_drag_lock = found;`, and spelling the threaded tail out inside
 *          the arm with or without its `if (g_drag_lock)` guard (both sink
 *          the arm past the place block: 677 bytes).
 *          Also measured: making BOTH the 0x103 arm's failure and this arm
 *          `goto fail` collapses the function to 173 instructions / 656
 *          bytes -- the two spelled-out copies are what keep VC6 from
 *          merging everything, so they must stay.
 *      (2) 0x470891: the original tests the placement call's result with
 *          `test eax,eax`; we get `cmp eax,edi` off the zero register that
 *          serves the other five compares against 0.  VC6 emits `test
 *          eax,eax` for a call result consumed DIRECTLY (it does so at
 *          0x470656, `if (WorkerHitOnRide())`, in ours too) and `cmp
 *          reg,edi` for one that went through a variable -- but writing
 *          `if (SetGardenerWorkOrderAtPostion(..))` / `if
 *          (SetMechanicsOrderAtPostion(..))` in the two arms gives the
 *          `test` and LOSES the cross-jump that folds both arms onto one
 *          `add esp,0Ch; test; jne` (678 bytes, and the gardener arm grows
 *          its own copy of the fail block + epilogue).  The original has
 *          both, so the merge must survive a direct test somehow.
 *          Unaffected: r's type (int/unsigned/long/void*), `!r` / `r == 0` /
 *          `r != 0`, `if (r == 0) { fail: g_drag_lock = 1; }` with no goto,
 *          `if (!r) goto fail; goto tail;`, and r's declaration order.
 *    Because (1) is a missing instruction, every index from 102 on is off by
 *    one and audit.py reports 82 index-for-index mismatches; a difflib
 *    alignment still puts 181 of the 184 instructions together (98.4%).
 *
 *    PASS N+1 - what 0x4707a3 ACTUALLY IS.  Compare it byte for byte with
 *    `fail:` at 0x470895:
 *        0x470895  mov [g_drag_lock],ebp / call SetWorkersPositionAtMouse /
 *                  pop edi / pop esi / pop ebp / add esp,8 / ret
 *        0x4707a3  mov ebp,1 / mov [g_drag_lock],ebp / call ... / pop edi /
 *                  pop esi / pop ebp / add esp,8 / ret
 *    The out-of-range arm is a TAIL-DUPLICATED COPY of the fail block, with
 *    `mov ebp,1` prepended because the row-count load (`xor ebp,ebp;
 *    mov bp,[map+16h]`) clobbered the constant register on the way in.  That
 *    is why the store is the register form and why no `g_drag_lock = 1`
 *    spelling reproduces it: the store was never compiled from a `= 1` in
 *    that arm at all, it is fail's own store copied.  Writing the arm as
 *    `goto fail;` makes VC6 SHARE the block instead of copying it (181
 *    instructions / 665 bytes - exactly the 12-byte copy replaced by a 5-byte
 *    jmp), so the open question is what makes VC6 duplicate rather than jump.
 *
 *    Ruled out this pass, all measured: `g_drag_lock = 1; goto tail;` in the
 *    arm (66 mismatches, 184 insns but 677 bytes - the arm SINKS past the
 *    place block, same failure as the spelled-out tail); the same with a dead
 *    `w = 0;` (identical); an explicit `join:` label between the arm and the
 *    test (183/671, unchanged); `else if (found == 0)`, `w = (WorkOrder*)0;`
 *    and `found = 1;` inside the arm (all DCE'd, 183/671, unchanged);
 *    swapping the two arms so the out-of-range case is the `if` (98
 *    mismatches, first at index 84 - the whole locked arm moves); hoisting the
 *    height test out as `if (cell.y >= g_map->height) g_drag_lock = 1;` after
 *    the block (147 mismatches, 662 bytes); and a shared `int one = 1;` local
 *    feeding every store of 1 in the function, and feeding only the arm and
 *    fail - both constant-propagated away (183/671, unchanged), confirming
 *    the earlier `one` finding.
 *
 *    Residual (2), re-measured: `if (SetGardener...(...)) goto tail; goto
 *    fail;` in BOTH arms DOES give `test eax,eax` and lands on 184
 *    instructions, but 678 bytes - the gardener arm grows its own copy of the
 *    fail block plus the epilogue where the original cross-jumps into the
 *    mechanic arm's `add esp,0Ch` (`jmp 0x47088e`).  `if (... == 0) goto
 *    fail;` with a trailing `goto tail` is worse still (681 bytes).  Keeping
 *    the `r` variable is what buys the cross-jump; the direct call test is
 *    what buys `test eax,eax`; nothing tried yet buys both.
 *
 *    NEXT STEP for (1): stop trying to spell the arm's store and instead find
 *    what makes VC6 tail-duplicate a `goto fail;` here rather than share it -
 *    e.g. a fail block that is cheaper to copy than to jump to, or an arm
 *    whose predecessor edge is the only one reaching fail from a
 *    register-clobbering path.  Both residuals are one instruction each; fix
 *    (1) alone and audit.py drops from 82 to 1.
 *
 *    PASS N+2 (2026-09-04) - 82 -> 13 by audit, and the body is now the
 *    original's SIZE exactly (184 instructions / 672 bytes).
 *    THE MECHANISM behind residual (1) is now understood: the constant 1 is a
 *    REGISTER WEB in ebp whose members are armA's store (0x470765), fail's
 *    store (0x470895) and the `place` preheader's def (0x4707ca).  armA's
 *    `mov ebp,1` survives because ebp=1 is LIVE-OUT of armA: its lock-clearing
 *    exit jumps to 0x4707db, inside place, and place falls through the two
 *    placement calls (ebp is callee-saved) into fail's `mov [lock],ebp`.  The
 *    out-of-range arm's def has no such consumer -- the arm threads straight
 *    into the tail's call and epilogue -- so VC6 folds it to an immediate.
 *    The original's register form means that at REGISTER-ALLOCATION time the
 *    arm still fell into the join and the constant was live through it; the
 *    threading and the tail duplication happened afterwards.
 *    The one spelling found that reproduces the size and the instruction
 *    count is comparing the join against the CONSTANT (`if (g_drag_lock == 1)`
 *    or `>= 1`, both identical objects; `!= 0`, `> 0` and a bare test are all
 *    the old 183/671).  It keeps the 1 live into the join, so `place` no
 *    longer needs its own def -- BUT it moves the cost rather than removing
 *    it: our join becomes `mov eax,[lock] / mov ebp,1 / cmp eax,ebp / je`
 *    where the original has `cmp [lock],edi / jne`, and the arm still stores
 *    the immediate.  So THE ORIGINAL'S JOIN IS `!= 0` and this spelling is a
 *    count-alignment trade, not the answer: it is 6 aligned differences
 *    against 3 for `!= 0`, but 13 index-for-index against 82.  A future lane
 *    that finds the real lever for the arm should put the join back to
 *    `if (g_drag_lock)`.  It is safe: on this path the lock is only ever 0 or
 *    1 (cleared at the top of the button-2 branch, set only by the two arms;
 *    the one callee in between, ScreenToMapRef at 0x0045be90, neither
 *    references 0x00668954 nor calls anything).
 *    Also ruled out this pass: the arm as a degenerate branch on `found`, `o`,
 *    `cell.x` or `g_worker_on_mouse_type` (all merged away, 183/671); the arm
 *    as `g_drag_lock = 1; goto tail;` on top of the `== 1` join (185/678);
 *    the join moved inside the in-range arm with the else arm carrying its own
 *    `goto tail` (184 but 677 bytes, 66 mismatches); the height read into an
 *    `int h` local first (identical object); and re-testing residual (2)'s
 *    direct call test on top of the new join (193 instructions / 699 bytes).
 *
 *    PASS N+3 (2026-09-04, sweep6 lane).  No change to the code; the lever
 *    for residual (1) is still not found, so the `== 1` join STAYS
 *    [SUPERSEDED -- the join was reverted to `!= 0` in PASS N+5 below] -- but the
 *    mechanism is now pinned down, and the note's old figures are corrected.
 *    CORRECTION: `!= 0` is no longer 183/671.  On the current body audit.py
 *    reports 184 instructions / 672 bytes for BOTH spellings (the real body
 *    is 183 instructions ending in `ret`; audit trims the compiled COMDAT to
 *    the original's 184-instruction extent, which picks up one byte of /Gy
 *    alignment padding).  So the honest `!= 0` costs nothing in size any
 *    more: it is 82 index-for-index against 13, but only THREE
 *    difflib-aligned differences against eight, and those three are exactly
 *      - `mov ebp,1` + `mov [g_drag_lock],ebp` (original, 0x183) against our
 *        single `mov dword ptr [g_drag_lock],1`, and
 *      - `test eax,eax` (original, 0x271) against our `cmp eax,edi`.
 *    Everything else in the function is byte-for-byte identical under `!= 0`.
 *    Whoever reads this next: 82 vs 13 is an ALIGNMENT artefact of one
 *    missing instruction, not 69 wrong instructions.
 *    WHAT THE ARM'S REGISTER STORE ACTUALLY NEEDS -- proved this pass.  Give
 *    the out-of-range arm a SECOND LIVE consumer of the constant 1 and VC6
 *    immediately emits `mov ebp,1 / mov [g_drag_lock],ebp` there: adding
 *    `g_icon_clicked = 1;` to the arm (a no-op on this path, that global is
 *    set to 1 at the top of the button-2 branch and nothing between touches
 *    it) drops the aligned distance from 6 to 4 -- i.e. it fixes residual (1)
 *    outright -- and costs three bytes for the extra store, so it is not the
 *    original.  Every DEAD second consumer is eliminated before allocation
 *    and changes nothing: `g_drag_lock = 1;` twice, `if (g_drag_lock != 1)
 *    g_drag_lock = 1;`, `found = 1;`, `r = 1;`, a `static __inline void
 *    LockDrag(void) { g_drag_lock = 1; }` used in the arm alone or in both
 *    arms, and a degenerate `else if (found) ... else ...`.  So the search is
 *    narrow and precise: find a consumer of the constant 1 on that path that
 *    the original ALSO has and that emits no instruction of its own.
 *    Also re-measured this pass on the `!= 0` baseline: swapping the two arms
 *    (98); `if (r != 0)` (identical); `unsigned r` (identical); `short r`
 *    (one instruction short); `char r` / `unsigned char r` (176 -- it forces
 *    a fourth callee-saved push and renames everything, and it would also be
 *    a semantic change, the callees' return values are not known to be 0/1).
 *
 *    PASS N+4 (2026-09-04, laneB).  The lever for residual (1) is still not
 *    found.  [At the time of writing this pass the `== 1` join was left in
 *    place; PASS N+5 below reverts it.]  New this pass:
 *
 *    THE CONSTANT-1 WEB, READ OFF THE ORIGINAL IN FULL.  ebp is ONE web for
 *    the literal 1 across the whole button-2 branch: defined at 0x470653 for
 *    `g_icon_clicked = 1` (0x47065d), consumed by `found = 1` TWICE as
 *    `mov esi,ebp` (0x47069c, 0x4706c2), and consumed by fail's
 *    `mov [g_drag_lock],ebp` at 0x470895 -- which is why the four early fail
 *    paths (hit type 0x10a/2, both cursor.y2 tests, the origin_x test, the
 *    cell.x/cell.y tests) can jump straight to a store with no immediate.
 *    ebp is then CLOBBERED twice on the in-range path -- `xor ebp,ebp /
 *    mov bp,[edx+0x16]` for the height (0x470750) and `mov ebp,[g_map_rows]`
 *    (0x47075a) -- so the web is rematerialised three times: 0x47077a (the
 *    in-range locked arm), 0x4707a3 (the out-of-range arm, residual (1)) and
 *    0x4707ca (the `place` preheader, for the join edge on which ebp holds
 *    the row table).  We reproduce the def at 0x47077a and the one at
 *    0x4707ca exactly; only the arm's is folded to an immediate.
 *    NOTE what that implies: because `place` redefines ebp after the merge,
 *    the arm's `mov ebp,1` is DEAD in the original -- a rematerialisation the
 *    allocator inserted while the arm still fell into the join and never
 *    removed.  A dead-but-emitted instruction, like the `and dx,0x20`
 *    merged-arm ghost in DECOMP.md.
 *
 *    RULED OUT THIS PASS, all on the honest `!= 0` baseline (all 82 strict /
 *    3 difflib-aligned, i.e. no change): `!= 0`, `> 0`, `!(== 0)`,
 *    `0 != lock` and `lock ? 1 : 0` as the join; the arm without braces, with
 *    a trailing `;`, with `found = 0`, `w = 0` or `found = 1` after the
 *    store; `one`-local variants feeding icon_clicked + the arm + fail, or
 *    every store of 1 in the function (all constant-propagated back);
 *    `if (r != 0)`, `!(r == 0)`, `!!r`, `switch (r) { case 0: break; default:
 *    goto tail; }`, `if (r == 0) goto fail; goto tail;` and `found = r;` for
 *    residual (2).
 *    NEW and worth recording: making the out-of-range arm `goto fail;` (with
 *    the honest `!= 0` join) does NOT tail-duplicate fail here -- VC6 SHARES
 *    the block, drops to 181 instructions / 668 bytes and folds EVERY store
 *    of 1 to an immediate, while growing its own duplicate of fail for one of
 *    the early exits (133 mismatches).  Hoisting the height test to a
 *    `if (cell.y >= height) goto fail;` guard gives the same 133.  The direct
 *    call test in one or both placement arms still ESCAPES the extent (678
 *    bytes).  `g_drag_lock = g_icon_clicked;` in the arm (a semantic
 *    obfuscation, not a candidate) gets the register form but as a bare
 *    `mov [lock],ebp` with no rematerialisation -- 667 bytes.
 *
 *    THE HONEST SPELLING, stated plainly for whoever reads this next.  The
 *    original's join is `cmp dword ptr [0x668954], edi / jne` -- a compare
 *    against the ZERO register, which is VC6's lowering of `if (g_drag_lock)`
 *    / `!= 0`.  A `== 1` source cannot produce it: ours compiles to
 *    `mov eax,[lock] / mov ebp,1 / cmp eax,ebp / je`, three instructions the
 *    original does not have.  So `!= 0` IS the original and `== 1` is a
 *    count-alignment trade: it buys 184/672 and 13 strict at the price of
 *    three wrong instructions, where `!= 0` is 183 instructions / 671 bytes
 *    and is byte-identical to the original EXCEPT the missing `mov ebp,1` at
 *    0x4707a3 and `cmp eax,edi` for `test eax,eax` at 0x470891.  The 82 is
 *    one missing instruction shifting every later index, not 69 wrong ones.
 *
 *    PASS N+5 (2026-09-04, laneB).  THE JOIN IS REVERTED to `if (g_drag_lock)`
 *    on the evidence above: a shape we can prove is not the original does not
 *    belong in the tree merely because it scores better, and the strict
 *    index-for-index count actively misleads once one instruction is missing
 *    and every later index shifts.
 *
 *    THE RESIDUAL, in full and final form.  audit.py: 184/184 instructions,
 *    672/672 bytes, mismatch 82 -- but the compiled body is 183 real
 *    instructions ending in `ret` (audit trims our COMDAT to the original's
 *    184-instruction extent, which picks up one byte of /Gy alignment
 *    padding), and difflib-aligned it is THREE lines, first at index 102:
 *      (1) index 102/103 -- original `mov ebp,1` + `mov [g_drag_lock],ebp`
 *          at 0x4707a3 against our single `mov dword ptr [g_drag_lock],1`.
 *          ONE missing instruction, one byte.  Everything after it shifts,
 *          which is the whole of the 82.
 *      (2) index 166 -- original `test eax,eax` at 0x470891 against our
 *          `cmp eax,edi` (same 2 bytes; VC6 uses the live zero register for a
 *          test on a VARIABLE and `test` for a test on a call result
 *          directly, but the direct spelling costs the cross-jump -- see
 *          residual (2) above).
 *    Every other instruction in the function is byte-for-byte the original's.
 *
 *    AND NOTE WHAT (1) IS.  Because `place` redefines ebp after the merge
 *    (0x4707ca), the arm's `mov ebp,1` at 0x4707a3 is DEAD in the original:
 *    a rematerialisation of the single ebp constant-1 web that the allocator
 *    inserted while the arm still fell into the join, and that survived the
 *    later jump threading and tail duplication.  It is an allocator leftover
 *    in the same class as the `and dx,0x20` merged-arm ghost recorded in
 *    docs/DECOMP.md -- a dead instruction that is nonetheless emitted.  So
 *    the thing still to find is not a better way to spell the store; it is
 *    whatever keeps our const-1 web live into that arm at allocation time.
 */
// WIP-FUNCTION: LEGOLAND 0x00470620  (184/184 insns, 672/672 bytes, 82 by audit but only 3 difflib-aligned, first diff at index 102; TWO defects only -- the dead `mov ebp,1` rematerialisation at 0x4707a3, whose absence shifts every later index and is the whole of the 82, and `cmp eax,edi` for `test eax,eax` at 0x470891; every other instruction is byte-identical)
void CheckWorkerOnMouseStatus(WorkOrder* o)
{
    Pos  cell;
    WorkOrder* w;
    int  found = 0;
    int  r;

    if ((g_cursor.state & 2) || o != 0) {
        ResetWorkersOldCoords();
    } else if (g_cursor.buttons & 2) {
        g_icon_clicked = 1;
        g_drag_lock = 0;
        if (g_hit_type == 0x103) {
            if (WorkerHitOnRide()) {
                g_drag_lock = 0;
                goto tail;
            }
            w = WorkOrderUnderHit(&cell);
            if (w) {
                g_drag_lock = 0;
                found = 1;
                goto place;
            }
            w = WorkOrderNearHit(&cell);
            if (w) {
                g_drag_lock = 0;
                found = 1;
                goto place;
            }
            g_drag_lock = 1;
            goto tail;
        }
        if (g_hit_type == 0x10a || g_hit_type == 2)
            goto fail;
        if (g_cursor.y2 < 0x20 || g_cursor.y2 >= 0x174)
            goto fail;
        if (g_cursor.point.x < ((MapHdr*)g_map)->origin_x + 9)
            goto fail;
        ScreenToMapRef(&g_cursor.point, &cell, 0);
        if (cell.x < 0 || cell.x >= g_map->width || cell.y < 0)
            goto fail;
        if (cell.y < g_map->height) {
            Cell* c = &g_map_rows[cell.y][cell.x];
            if (c && (c->rf & 2)) {
                g_drag_lock = 1;
                if (g_worker_on_mouse_type == 0x307 && (c->flags & 0x800))
                    g_drag_lock = 0;
            }
        } else {
            /* Out-of-range row: just the lock. Everything the original does
             * after it (the tail's call and epilogue, inlined at 0x4707a3)
             * is VC6 threading the join below through a known-true test. */
            g_drag_lock = 1;
        }
        /* The join is `!= 0`, which is what the original has: `cmp dword ptr
         * [g_drag_lock], edi` against the zero register.  An earlier pass
         * spelled it `== 1` because that lines the instruction COUNT up with
         * the `mov ebp,1` we are missing in the out-of-range arm above, but it
         * compiles to `mov eax,[lock] / mov ebp,1 / cmp eax,ebp / je` -- three
         * instructions the original does not contain -- so it was a scoring
         * trade, not a reconstruction.  See the note above this function. */
        if (g_drag_lock)
            goto tail;
place:
        if (g_worker_on_mouse_type == 0x307) {
            g_worker_on_mouse->world.x = (cell.x << 8) + 0x80;
            g_worker_on_mouse->world.y = (cell.y << 8) + 0x80;
            r = SetGardenerWorkOrderAtPostion(g_worker_on_mouse, cell.x, cell.y);
        } else {
            if (found) {
                g_worker_on_mouse->world.x = ((w->ox + cell.x) << 8) + 0x80;
                g_worker_on_mouse->world.y = ((cell.y - w->oy) << 8) + 0x80;
            } else {
                g_worker_on_mouse->world.x = (cell.x << 8) + 0x80;
                g_worker_on_mouse->world.y = (cell.y << 8) + 0x80;
            }
            r = SetMechanicsOrderAtPostion(g_worker_on_mouse, cell.x, cell.y);
        }
        if (r)
            goto tail;
fail:
        g_drag_lock = 1;
    }
tail:
    if (g_drag_lock)
        SetWorkersPositionAtMouse();
}
