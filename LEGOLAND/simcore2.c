/* LEGOLAND -- visitor and route-search helpers. VC6 SP3 /O2 /Gy /Gd. */
typedef struct RouteNode { struct RouteNode* next; } RouteNode;
typedef struct MapHdr {
    unsigned char pad00[0x14];
    unsigned short width, height;
} MapHdr;
typedef struct Bloke {
    unsigned char pad00[0x62];
    unsigned char flags;
    unsigned char pad63[0xf];
    unsigned char tick_phase;
    unsigned char pad73;
    unsigned char ai_phase;
    unsigned char walk_delay;
    unsigned char pad76[2];
    short stay_timer, mood;
} Bloke;
extern RouteNode* g_route_closed;                        /* 0x00668fc4 */
extern RouteNode* g_route_open;                          /* 0x00668fc0 */
extern MapHdr* g_map;                                   /* 0x004bcbf4 */
extern int g_popup_kind;                                /* 0x007fdf9c */
extern Bloke* g_watched_bloke;                           /* 0x007fdf8c */
extern int g_mood_low;                                  /* 0x0083292c */
extern int g_mood_high;                                 /* 0x00832934 */
extern int g_mood_adjustments[];                         /* 0x0083293c */
extern int g_visitor_spawn_clock;                       /* 0x006661c8 */
extern int g_visitor_count;                             /* 0x006661bc */
extern int GetVisitorLimit(void);                       /* 0x0044ea40 */
extern int HasBlokeStayedTooLong(Bloke* bloke);          /* 0x0044eab0 */
extern void NewLongTermAction(Bloke* bloke, int action); /* 0x0044e760 */
extern Bloke* MakeBloke(int kind);                      /* 0x004830c0 */
extern void InitBlokeAI(Bloke* bloke);                   /* 0x0044e920 */
extern int rand(void);                                  /* 0x0049e4b2 */

// FUNCTION: LEGOLAND 0x004776c0
void AddClosedNode(RouteNode* node)
{
    node->next = g_route_closed;
    g_route_closed = node;
}

// FUNCTION: LEGOLAND 0x00477980
int RouteTurnCost(int from, int to)
{
    return (from & to) ? 0 : 4;
}

// FUNCTION: LEGOLAND 0x004700c0
int IsWatchedBloke(Bloke* bloke)
{
    if (g_popup_kind == 0x306 && g_watched_bloke == bloke) return 1;
    return 0;
}

// FUNCTION: LEGOLAND 0x00482d30
int GetBlokeMood(Bloke* bloke)
{
    int mood = bloke->mood;
    if (mood < g_mood_low) return 3;
    if (mood < g_mood_high) return 10;
    return 2;
}

// FUNCTION: LEGOLAND 0x0044eae0
void UpdateBlokeStay(Bloke* bloke)
{
    unsigned char flags = bloke->flags;
    bloke->stay_timer++;
    if (!(flags & 8) && HasBlokeStayedTooLong(bloke))
        NewLongTermAction(bloke, 3);
}

/* Zero for diagonal/no movement; 1 for a y step, 2 for an x step. */
// FUNCTION: LEGOLAND 0x004779a0
int RouteStepAxis(int x1, int y1, int x2, int y2)
{
    /* Preserve the independent x subtraction before the y calculation. */
    int dx = *(volatile int*)&x1 - x2;
    int dy = y1 - y2;
    if (dx) return dy ? 0 : 2;
    return dy != 0;
}

/* Original precondition: target must be in the nonempty open list.
 * The missing/empty cases dereference a null node, deliberately retained. */
// FUNCTION: LEGOLAND 0x00477790
void RemoveOpenNode(RouteNode* target)
{
    RouteNode* node = g_route_open;
    RouteNode* prev = 0;
    while (node) {
        if (node == target) break;
        prev = node;
        node = node->next;
    }
    if (prev) prev->next = node->next;
    else g_route_open = node->next;
}

// FUNCTION: LEGOLAND 0x00477680
int RouteInBounds(int x, int y)
{
    if (x < 0) return 0;
    if (y < 0) return 0;
    if (x >= g_map->width) return 0;
    return y < g_map->height;
}

/* Event table access is unchecked; signed scaling precedes the mood clamp. */
// FUNCTION: LEGOLAND 0x00482df0
int AdjustMood(Bloke* bloke, int event, int scale)
{
    int mood = bloke->mood + g_mood_adjustments[event] * scale / 100;
    if (mood < -30000) mood = -30000;
    else if (mood > 30000) mood = 30000;
    bloke->mood = (short)mood;
    return bloke->mood;
}

/* A failed capacity/allocation check leaves the elapsed clock accumulating. */
// FUNCTION: LEGOLAND 0x0044ea50
void SpawnVisitor(void)
{
    if (++g_visitor_spawn_clock >= 30 && g_visitor_count < GetVisitorLimit()) {
        Bloke* bloke = MakeBloke(0);
        if (bloke) {
            g_visitor_spawn_clock = 0;
            bloke->tick_phase = rand() & 7;
            bloke->ai_phase = rand() & 7;
            bloke->walk_delay = 1;
            InitBlokeAI(bloke);
        }
    }
}
