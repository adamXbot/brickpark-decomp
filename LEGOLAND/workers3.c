/* LEGOLAND -- WORK ORDERS, HIRED WORKERS AND THE ROUTE-SEARCH OPEN LIST.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere) and follow workers.c / workorder2.c / bnvmove.c / objrect.c.
 *
 *   0x00482e50  AllocBlokePool        allocate and zero the bloke pool
 *   0x0044eb50  CheckBlokeLeaving     is this visitor tired enough to go?
 *   0x0049a010  ControlMechanics      per-tick AI walk of the mechanic list
 *   0x00499fb0  ControlGardeners      ... and of the gardener list
 *   0x00499a70  GetOrderCentre        the walk-to point of an object order
 *   0x004776e0  AddOpenNode           insert into the A* open list
 *   0x00499be0  FindNearestFreeOrder  the unassigned order nearest a worker
 *
 * =========================================================================
 * 1. THE BLOKE POOL IS ONE FLAT ARRAY  (AllocBlokePool)
 * =========================================================================
 * Every bloke in the park -- visitors and hired workers alike -- lives in a
 * single array of 172-byte (0xac) records at 0x0066b57c.  Its capacity is the
 * u16 at map header +0x1a, so the map file decides how many people the park
 * can ever hold; the free-list walk in workers.c hands slots out of it.
 *
 * The allocation is `max_blokes * 0xac` (VC6 expands the constant multiply
 * into lea/sub/lea/lea/shl) and every record is then zeroed by its own
 * memset -- one `rep stosd` of 43 dwords per bloke, with the array BASE
 * re-read from the global and the map header re-read from ITS global on
 * every iteration.  Both re-reads are the original's; they fall straight out
 * of naming the globals at each use instead of caching them.
 *
 * =========================================================================
 * 2. WHEN A VISITOR GOES HOME  (CheckBlokeLeaving)
 * =========================================================================
 * A visitor carries a u16 "tiredness" at Bloke +0x7c and a per-step increment
 * at +0x80.  The four thresholds at 0x004b8334 -- 1000, 2400, 4000, 7000 --
 * bracket it into the mood tier that 0x0044eb10 returns; the LAST of them
 * (0x004b8340, 7000) is also the point at which the visitor leaves.
 *
 * Every tick, for a visitor:
 *
 *   - at or past 7000, and not currently riding or inside an attraction
 *     (flags62 & 0x28), the bloke is given long-term action 3 (go home) and
 *     nothing else happens this tick;
 *   - otherwise, one tick in every 32, tiredness grows by +0x80.
 *
 * The two tests against the same threshold are `>=` and `<=`, so a visitor
 * sitting EXACTLY on 7000 while riding still accumulates -- reproduced, and
 * the comparison really is done twice off one cached `unsigned short`.
 * 0x00832990 is the enable switch (a settings-menu case at 0x0046a086);
 * with it clear the whole check is skipped and the tier helper answers 1.
 *
 * =========================================================================
 * 3. THE TWO HIRED-WORKER TICKS  (ControlMechanics / ControlGardeners)
 * =========================================================================
 * One source compiled twice, differing only in the list global and the
 * remover it calls.  Per worker, in order:
 *
 *      tick++                              (Bloke +0x5c)
 *      if (state == 0)  DoHighLevelAI      -- pick a new plan
 *      if (state != 0)  DoLowLevelAI       -- run the current one
 *      UpdatePerson                        -- push the pose to the model
 *      if (slot == 100) RemoveAMechanic    -- 100 is the "fired" sentinel
 *
 * The two state tests are SEPARATE `if`s, not an if/else: VC6 threads the
 * first test's non-zero edge straight into the second's body, which is why
 * only one `DoLowLevelAI` call site is emitted.  `next` is read before the
 * body so a worker may remove itself, and the loop's latch tests `next`.
 *
 * =========================================================================
 * 4. WHERE A WORKER STANDS TO DO A JOB  (GetOrderCentre)
 * =========================================================================
 * An object work order carries a map cell (+0x08) and a malloc'd rect array
 * (+0x10); the mechanic's walk-to target is the middle of the LONGER side of
 * the first rect, in 24.8 world units:
 *
 *      w = right - left      h = bottom - top
 *      w >= h  ->  ( (cell.x + left)*2 + w ) << 7 , (cell.y + top) << 8
 *      w <  h  ->  (cell.x + left) << 8 , ( (cell.y + top)*2 + h ) << 7
 *
 * `(2a + span) << 7` is `(a + span/2) << 8` without the rounding loss, so
 * the long axis is halved and the short axis is left on the rect's NEAR
 * edge.  A worker therefore walks to the middle of the side it will work
 * along, standing on the top or left edge of the footprint.  The whole
 * result comes back as an 8-byte Pos in eax:edx.
 *
 * =========================================================================
 * 5. THE A* OPEN LIST  (AddOpenNode)
 * =========================================================================
 * simcore.c's route search keeps its open list at 0x00668fc0 sorted by the
 * node's f-score (+0x1c), lowest first, and this is the insertion.  It walks
 * for the first node whose f is NOT LESS than the new node's and links in
 * front of it; `prev == 0` (the new node beats the head, or the list turned
 * out to be empty) is the push-on-front case.  The empty-list case is a
 * separate leading block in the original, and it stores the head pointer
 * BEFORE the link, the opposite order to the push-on-front block -- both
 * reproduced.
 *
 * =========================================================================
 * 6. PICKING A JOB  (FindNearestFreeOrder)
 * =========================================================================
 * The nearest UNASSIGNED order to a worker, by squared distance between the
 * worker's world position shifted down to whole map squares (>> 8, an
 * arithmetic shift, so a worker at a negative coordinate rounds towards
 * minus infinity) and the order's cell.  The running best starts at
 * 0x7fffffff and the test is a strict `<`, so among equidistant orders the
 * FIRST on the list wins.
 * ========================================================================= */

/* ---------------------------------------------------------------- types -- */

typedef struct Pos { int x; int y; } Pos;

typedef struct Rect {
    int          left;           /* +0x00 */
    int          top;            /* +0x04 */
    int          right;          /* +0x08 */
    int          bottom;         /* +0x0c */
    struct Rect* next;           /* +0x10 */
} Rect;

typedef struct Bloke Bloke;

/* A gardener or mechanic work order -- calloc(0x3c,1) @ 0x004995d0
 * (workorder2.c owns the record). */
typedef struct WorkOrder {
    struct WorkOrder* next;      /* +0x00 */
    void*             obj;       /* +0x04 */
    Pos               pos;       /* +0x08  map cell the rects are biased by */
    Rect*             rects;     /* +0x10  malloc'd rect array */
    int               nrects;    /* +0x14 */
    int               assigned;  /* +0x18  1 once a worker has taken it */
    Bloke*            worker;    /* +0x1c */
    unsigned char     kind;      /* +0x20  1 = object, 2 = path/scenery span */
    unsigned char     pad21[0x3c - 0x21];
} WorkOrder;

/* One person -- 172 bytes, and the whole park lives in one array of them. */
struct Bloke {
    Bloke*         next;         /* +0x00  intrusive list link */
    void*          person;       /* +0x04  the renderable 3D person */
    unsigned char  pad08[6];     /* +0x08..0x0d */
    unsigned short state;        /* +0x0e  low-level AI state, 0 = idle */
    unsigned char  pad10[0x26];  /* +0x10..0x35 */
    unsigned char  slot;         /* +0x36  ride/job slot; 100 = discard me */
    unsigned char  pad37[0x25];  /* +0x37..0x5b */
    int            tick;         /* +0x5c  per-bloke tick counter */
    unsigned char  pad60[2];     /* +0x60..0x61 */
    unsigned short flags62;      /* +0x62  0x08|0x20 = riding / inside */
    unsigned char  pad64[4];     /* +0x64..0x67 */
    int            wx;           /* +0x68  world x, 24.8 */
    int            wy;           /* +0x6c  world y, 24.8 */
    unsigned char  pad70[0x0c];  /* +0x70..0x7b */
    unsigned short tired;        /* +0x7c  grows towards the leave threshold */
    unsigned char  pad7e[2];     /* +0x7e..0x7f */
    unsigned char  tired_step;   /* +0x80  added every 32nd tick */
    unsigned char  pad81[0xac - 0x81];
};                               /* 0xac */

/* One node of the auto-path route search (objrect.c / simcore.c); 0x28
 * bytes, and +0x1c is the open list's sort key. */
typedef struct RouteNode {
    struct RouteNode* next;      /* +0x00  open/closed list link */
    struct RouteNode* parent;    /* +0x04 */
    Pos               pos;       /* +0x08 */
    int               cost;      /* +0x10 */
    int               g;         /* +0x14 */
    int               h;         /* +0x18 */
    int               f;         /* +0x1c  the open-list sort key */
    int               axis;      /* +0x20 */
    int               dir;       /* +0x24 */
} RouteNode;

/* The map header; only the bloke capacity is used here. */
typedef struct MapHdr {
    unsigned char  pad00[0x1a];  /* +0x00..0x19 */
    unsigned short max_blokes;   /* +0x1a */
} MapHdr;

/* --------------------------------------------------------------- globals -- */

extern MapHdr*     g_map;                    /* 0x004bcbf4 */
extern Bloke*      g_bloke_base;             /* 0x0066b57c */
extern Bloke*      g_gardener_list;          /* 0x0079a8a8 */
extern Bloke*      g_mechanic_list;          /* 0x0079a8ac */
extern RouteNode*  g_open_head;              /* 0x00668fc0  open list, by f */

/* The visitors-get-tired switch, written by the settings dispatcher's case
 * at 0x0046a086.  Clear = nobody ever goes home. */
extern int         g_visitor_tire;           /* 0x00832990 */
/* g_bloke_tier_limits[3] -- the last of the four mood brackets at 0x004b8334
 * ({1000, 2400, 4000, 7000}), which is also the leave threshold. */
extern int         g_bloke_leave_limit;      /* 0x004b8340 */
/* Read here as a DWORD (`mov edx,[mem] / and edx,0x1f`), so declared int as
 * gpu.c and sprite2.c do; bigsim.c and mapscreen.c call the same address
 * `unsigned char g_ms_flags`, which would cost the load's width. */
extern int         g_detail;                 /* 0x008119a4 */

/* --------------------------------------------------------------- callees -- */

extern void* MemAlloc(int size);                                 /* 0x0049e4ff */
/* workers.c declares `short action`; the constant push is identical. */
extern void  NewLongTermAction(Bloke* b, int action);            /* 0x0044e760 */
extern void  DoHighLevelAI(Bloke* b);                            /* 0x004504d0 */
extern void  DoLowLevelAI(Bloke* b);                             /* 0x00484920 */
extern void  UpdatePerson(Bloke* b);                             /* 0x00440290 */
extern void  RemoveAMechanic(Bloke* m);                          /* 0x0049a430 */
extern void  RemoveAGardener(Bloke* g);                          /* 0x0049a2d0 */

void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)

/* =========================================================================
 * THE BLOKE POOL
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00482e50
void AllocBlokePool(void)
{
    int i;

    g_bloke_base = (Bloke*)MemAlloc(g_map->max_blokes * sizeof(Bloke));
    for (i = 0; i < g_map->max_blokes; i++)
        memset(&g_bloke_base[i], 0, sizeof(Bloke));
}

/* =========================================================================
 * THE VISITOR LEAVE CHECK
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0044eb50
void CheckBlokeLeaving(Bloke* b)
{
    unsigned short t;

    if (g_visitor_tire) {
        t = b->tired;
        if ((int)t >= g_bloke_leave_limit && (b->flags62 & 0x28) == 0) {
            NewLongTermAction(b, 3);
            return;
        }
        if ((g_detail & 0x1f) == 0xf && (int)t <= g_bloke_leave_limit)
            b->tired = t + b->tired_step;
    }
}

/* =========================================================================
 * THE TWO HIRED-WORKER TICKS
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0049a010
void ControlMechanics(void)
{
    Bloke* m = g_mechanic_list;
    Bloke* next;

    while (m) {
        next = m->next;
        m->tick++;
        if (m->state == 0)
            DoHighLevelAI(m);
        if (m->state != 0)
            DoLowLevelAI(m);
        UpdatePerson(m);
        if (m->slot == 100)
            RemoveAMechanic(m);
        m = next;
    }
}

// FUNCTION: LEGOLAND 0x00499fb0
void ControlGardeners(void)
{
    Bloke* g = g_gardener_list;
    Bloke* next;

    while (g) {
        next = g->next;
        g->tick++;
        if (g->state == 0)
            DoHighLevelAI(g);
        if (g->state != 0)
            DoLowLevelAI(g);
        UpdatePerson(g);
        if (g->slot == 100)
            RemoveAGardener(g);
        g = next;
    }
}

/* =========================================================================
 * WORK ORDERS
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00499a70
Pos GetOrderCentre(WorkOrder* o)
{
    Pos   p;
    Rect* r = o->rects;
    int   x0 = r->left;
    int   w = r->right - x0;
    int   y0 = r->top;
    int   h = r->bottom - y0;

    if (w >= h) {
        p.x = (w + (o->pos.x + x0) * 2) << 7;
        p.y = (o->pos.y + y0) << 8;
    } else {
        p.x = (o->pos.x + x0) << 8;
        p.y = (h + (o->pos.y + y0) * 2) << 7;
    }
    return p;
}

// FUNCTION: LEGOLAND 0x00499be0
WorkOrder* FindNearestFreeOrder(WorkOrder* list, Bloke* b)
{
    WorkOrder* best = 0;
    WorkOrder* o = list;
    int        bestd = 0x7fffffff;

    while (o) {
        if (o->assigned == 0) {
            int dx = (b->wx >> 8) - o->pos.x;
            int dy = (b->wy >> 8) - o->pos.y;
            int d = dy * dy + dx * dx;

            if (d < bestd) {
                bestd = d;
                best = o;
            }
        }
        o = o->next;
    }
    return best;
}

/* =========================================================================
 * THE A* OPEN LIST
 * ========================================================================= */

/* Read the list head GLOBAL at all three uses instead of caching it in a
 * local.  VC6 CSEs the three reads into ONE load in edi (so the tail's
 * `n->next = <old head>` costs nothing), but the leading guard's "head is
 * null" knowledge does NOT flow onto `p`, so the `while (p)` peel survives as
 * the original's redundant `mov eax,edi / test eax,eax`.  A `head` local
 * propagates it away and comes out two instructions short, and every other
 * shape measured (an inline helper, a guarded do/while, the `if (prev == 0)`
 * inversion, a `for`, a short-circuit `&&` condition) does the same; a free
 * volatile read keeps the test but reloads the global instead of copying it.
 * `n->next = 0;` is stored through the just-tested edi -- VC6 propagating the
 * compared constant into the arm. */
// FUNCTION: LEGOLAND 0x004776e0
void AddOpenNode(RouteNode* n)
{
    RouteNode* p;
    RouteNode* prev;

    if (g_open_head == 0) {
        g_open_head = n;
        n->next = 0;
        return;
    }
    p = g_open_head;
    prev = 0;
    while (p) {
        if (p->f >= n->f)
            break;
        prev = p;
        p = p->next;
    }
    if (prev) {
        n->next = prev->next;
        prev->next = n;
        return;
    }
    n->next = g_open_head;
    g_open_head = n;
}
