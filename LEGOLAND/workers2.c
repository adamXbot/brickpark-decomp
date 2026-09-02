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
    MapInst*     inst;
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
 *  - RESIDUAL, two instructions:
 *      (1) 0x4707a3: the original rematerialises the constant into ebp for
 *          the out-of-range arm's store -- `mov ebp,1; mov [g_drag_lock],ebp`
 *          (11 bytes) where we emit the one-byte-cheaper immediate store
 *          `mov [g_drag_lock],1`.  ebp holds the row count on entry to the
 *          arm (`xor ebp,ebp; mov bp,[map+16h]`), so a remat is needed in
 *          both; VC6 folds ours to the immediate because the constant is
 *          dead at the arm's end.  Ruled out as levers: a named `one` local
 *          (constant-propagated away), `found + 1`, `g_icon_clicked`,
 *          `goto fail` / `goto tail` from the arm, putting the `fail:` label
 *          inside the arm, and swapping the two arms.  A probe that adds a
 *          FIFTH store of 1 to the function leaves the arm on the immediate
 *          too, so the count threshold in the note above is not what decides
 *          this one: the register form is used in the blocks where the
 *          constant is still LIVE-OUT (the path-flag arm falls through to
 *          `place` and on to `fail`, which needs it), and folded in this arm
 *          because it dead-ends in a return.
 *      (2) 0x470891: the original tests the placement call's result with
 *          `test eax,eax`; we get `cmp eax,edi` off the zero register that
 *          serves the other five compares against 0 in this function.
 *          Unaffected by r's type (int/unsigned/long/void*), by `!r` /
 *          `r == 0` / `r != 0`, by declaration order, or by dropping `r` and
 *          testing each call directly (which then stops VC6 merging the two
 *          `add esp,0Ch; test; jne` tails).
 *    Because (1) is a missing instruction, every index from 102 on is off by
 *    one and audit.py reports 82 index-for-index mismatches.
 */
// WIP-FUNCTION: LEGOLAND 0x00470620  (181/184 aligned = 98.4%, 672/672 bytes; two difference hunks, at 0x4707a3 and 0x470891, see above; the missing instruction shifts every later index so audit.py counts 82)
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
