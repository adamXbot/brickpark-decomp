/* LEGOLAND -- path, object footprint and construction helpers.
 * Local layouts reproduce VC6 SP3 /O2 /Gy /Gd output and original edge cases. */
typedef struct Pos { int x, y; } Pos;
typedef struct Rect4 { int left, top, right, bottom; } Rect4;
typedef struct ObjFootprint {
    unsigned char pad00[0xc];
    int entrance_x, entrance_y;
    unsigned char pad14[0x10];
    signed char exit_x, exit_y;
} ObjFootprint;
typedef struct Cell {
    unsigned char pad00[0xc];
    unsigned char flags;
    unsigned char pad0d[3];
    unsigned char rf;
} Cell;
typedef struct RoadRec {
    unsigned char pad00[0x1c];
    unsigned char kind, claims;
} RoadRec;
typedef struct PTPNode {
    struct PTPNode* next;
    struct PTPNode* parent;
    int x, y;
} PTPNode;
typedef struct BuildSlot { void* object; unsigned short key, pad06; int timer; } BuildSlot;
typedef struct MarkedTile { unsigned short key, count; } MarkedTile;
typedef struct ElemData { unsigned char pad00[8]; unsigned char flags; } ElemData;
typedef struct ObjDef {
    unsigned char pad00[0x1c];
    unsigned int flags;
    unsigned char pad20[0x38];
    ElemData* data;
} ObjDef;
typedef struct WorkOrder { unsigned char pad00[0x24]; int span_x, span_y; } WorkOrder;
extern unsigned char g_ptp_visited[0x1200];                /* 0x00669258 */
extern PTPNode* g_ptp_open_head;                           /* 0x0066b450 */
extern PTPNode* g_ptp_route_head;                          /* 0x0066b458 */
extern BuildSlot g_build_slots[256];                      /* 0x006664f8 */
extern int g_build_count;                                 /* 0x006670f8 */
extern MarkedTile g_marked_tiles[128];                     /* 0x007cb3e0 */
extern Rect4 g_obj_rects[];                               /* 0x00801a80 */
extern int g_obj_rect_count;                              /* 0x00667d3c */
extern void* memset(void* dest, int value, unsigned size);
extern void* MemAlloc(unsigned size);                     /* 0x0049e4ff */
extern void HeapFree_w(void* ptr);                        /* 0x0049e4d0 */
extern RoadRec* GetRoadRecord(int x, int y);               /* 0x004125f0 */
extern void SubtractObjRect(Rect4* rect);                  /* 0x0045d5d0 */
extern int GrowPathRectSide(Rect4* rect, int side);        /* 0x0045cbc0 */
extern int IsBuildableClass(ObjDef* cls);                  /* 0x0045ead0 */
extern void AdvanceOrderSpan(WorkOrder* order);           /* 0x00499620 */
extern int IsOrderSpanClear(WorkOrder* order);             /* 0x004996a0 */

/* The binary clears exactly 0x1200 bytes, not a presumed full map size. */
// FUNCTION: LEGOLAND 0x004821c0
void ClearPTPVisited(void)
{
    memset(g_ptp_visited, 0, sizeof(g_ptp_visited));
}

// FUNCTION: LEGOLAND 0x0045e690
int ObjHasExit(ObjFootprint* obj)
{
    if (obj->exit_x == obj->entrance_x && obj->exit_y == obj->entrance_y)
        return 0;
    return 1;
}

// FUNCTION: LEGOLAND 0x0045ce10
int IsPathCell(Cell* cell)
{
    if ((cell->rf & 1) || ((cell->flags & 0x10) && !(cell->rf & 2))) return 1;
    return 0;
}

// FUNCTION: LEGOLAND 0x004139c0
void Road_TileClaim(unsigned x, unsigned y)
{
    RoadRec* road = GetRoadRecord(x >> 8, y >> 8);
    if (road) road->claims++;
}

/* Route nodes leave parent (+4) uninitialized. */
// FUNCTION: LEGOLAND 0x00482300
PTPNode* AddPTPRouteNode(int x, int y)
{
    PTPNode* node = (PTPNode*)MemAlloc(sizeof(PTPNode));
    if (node) {
        node->next = g_ptp_route_head;
        g_ptp_route_head = node;
        node->x = x;
        node->y = y;
    }
    return node;
}

/* Scan all slots by key, including inactive slots; count can underflow. */
// FUNCTION: LEGOLAND 0x00450c00
void FreeBuildSlotAt(unsigned short key)
{
    int i;
    for (i = 0; i < 256; i++) {
        if (g_build_slots[i].key == key) {
            g_build_count--;
            g_build_slots[i].object = 0;
            return;
        }
    }
}

// FUNCTION: LEGOLAND 0x004821e0
void FreePTPOpenList(void)
{
    PTPNode* node = g_ptp_open_head;
    while (node) {
        PTPNode* next = node->next;
        HeapFree_w(node);
        node = next;
    }
    g_ptp_open_head = 0;
}

/* AL-only result: absent road = 2, nonzero kind = 3, zero kind = 1. */
// FUNCTION: LEGOLAND 0x00413990
unsigned char Road_TileCost(unsigned x, unsigned y)
{
    RoadRec* road = GetRoadRecord(x >> 8, y >> 8);
    unsigned char cost;
    if (!road) return 2;
    if (road->kind) cost = 3;
    else cost = 1;
    return cost;
}

// FUNCTION: LEGOLAND 0x00489f00
int MarkObjectTiles(Pos* pos)
{
    int i;
    for (i = 0; i < 128; i++) {
        if (g_marked_tiles[i].key == 0xffff) {
            g_marked_tiles[i].key = (unsigned short)((pos->x << 8) + pos->y);
            g_marked_tiles[i].count = 0;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x0045d730
void AddObjRectSpan(Rect4* rect)
{
    SubtractObjRect(rect);
    g_obj_rects[g_obj_rect_count++] = *rect;
}

/* Stop only after four consecutive unsuccessful sides; success resets it. */
// FUNCTION: LEGOLAND 0x0045cd00
void GrowPathRect(Rect4* rect)
{
    int failed = 0;
    int side = 0;
    do {
        if (GrowPathRectSide(rect, side)) failed = 0;
        else failed++;
        side = (side + 1) & 3;
    } while (failed < 4);
}

// FUNCTION: LEGOLAND 0x0045eaf0
int ClassNeedsPath(ObjDef* cls)
{
    if (IsBuildableClass(cls)) return 1;
    if (cls && cls->data && (cls->data->flags & 0x10) && !(cls->flags & 0x600000))
        return 1;
    return 0;
}

/* Always advance once more after testing a candidate. Stop at a clear tile
 * or after returning to the initially advanced span coordinate. */
// FUNCTION: LEGOLAND 0x00499720
void SettleOrderSpan(WorkOrder* order)
{
    int x, y, clear;
    AdvanceOrderSpan(order);
    x = order->span_x;
    y = order->span_y;
    do {
        clear = IsOrderSpanClear(order);
        AdvanceOrderSpan(order);
    } while ((order->span_x != x || order->span_y != y) && !clear);
}
