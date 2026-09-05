/* LEGOLAND -- additional path and work-order helpers (Codex D).
 * Local layouts intentionally preserve the original VC6 behavior. */
typedef struct Pos { int x, y; } Pos;
typedef struct Rect4 { int left, top, right, bottom; } Rect4;
typedef struct PTPNode { struct PTPNode *next, *parent; int x, y; } PTPNode;
typedef struct RouteNode { struct RouteNode *next, *parent; Pos pos; } RouteNode;
typedef struct MarkedTile { unsigned short key, count; } MarkedTile;
typedef struct Walker { unsigned char pad00[0x62]; unsigned short flags; } Walker;
typedef struct PathSquare {
    struct PathSquare* next;
    int pad04;
    Rect4 rect;
    void* rect_next;
    int distance2, flags;
} PathSquare;
typedef struct WorkOrder {
    struct WorkOrder* next;
    void* object;
    Pos pos;
    Rect4* rects;
    int nrects, assigned;
    void* worker;
    unsigned char kind, pad21[3];
    int span_x, span_y;
    unsigned char f2c, pad2d[3];
    int f30;
    float charge, amount;
} WorkOrder;
typedef struct BPos { unsigned char x, y; } BPos;
typedef struct CellObject { unsigned char pad00[0xc]; void* cls; } CellObject;
typedef struct Cell {
    CellObject* object;
    int pad04;
    unsigned short tile, pad0a, flags, pad0e;
    unsigned char rf, pad11[3];
} Cell;
typedef struct Map { unsigned char pad00[0x14]; unsigned short width, height; } Map;
extern PTPNode* g_ptp_route_head;                         /* 0x0066b458 */
extern MarkedTile g_marked_tiles[128];                    /* 0x007cb3e0 */
extern RouteNode *g_route_closed, *g_route_open;          /* 0x00668fc4, 0x00668fc0 */
extern PathSquare* g_path_squares;                       /* 0x0066b44c */
extern WorkOrder *g_mechanic_orders, *g_mechanic_order_tail; /* 0x0079a8c0, 0x0079a8c4 */
extern int g_mechanic_order_count;                       /* 0x0079a8c8 */
extern WorkOrder *g_gardener_orders, *g_gardener_order_tail; /* 0x0079a8b0, 0x0079a8b4 */
extern int g_gardener_order_count;                       /* 0x0079a8b8 */
extern int g_path_gfx_batch;                             /* 0x0066b46c: force refresh */
extern Map* g_map;                                      /* 0x004bcbf4 */
extern Cell** g_map_rows;                               /* 0x00801400 */
extern int* g_path_tile_base;                           /* 0x00832bf0 */
extern void* g_env_class;                               /* 0x007fd624 */
extern void HeapFree_w(void* p);                         /* 0x0049e4d0 */
extern void* CRT_calloc(unsigned count, unsigned size);  /* 0x004a020e */
extern int DBPrintf(const char* format, ...);            /* 0x00453a20 */
extern int OverNewTile(Walker* walker, int x, int y);     /* 0x00483650 */
extern PathSquare* FindPathSquare(Pos* pos);             /* 0x00481790 */
extern void RefreshEntranceTile(int force);              /* 0x00482b20 */
extern WorkOrder* GetMechanicWorkOrderAt(int x, int y);   /* 0x0049b180 */
extern WorkOrder* GetGardenerWorkOrderAt(int x, int y);   /* 0x0049b130 */
extern void EraseMechanicOrder(WorkOrder* order);         /* 0x0049b1d0 */
extern void EraseGardenerOrder(WorkOrder* order);         /* 0x0049b230 */
extern int IsPathCell(Cell* cell);                       /* 0x0045ce10 */
extern int IsPathRectClear(Rect4* rect);                 /* 0x0045c900 */

static __inline Cell* CellForPos(Pos* pos)
{
    int x = pos->x;
    int y;
    if (x >= 0 && x < g_map->width) {
        y = pos->y;
        if (y >= 0 && y < g_map->height) return &g_map_rows[y][x];
    }
    return 0;
}

static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

// FUNCTION: LEGOLAND 0x00482210
void FreePTPRouteList(void)
{
    PTPNode* node = g_ptp_route_head;
    while (node) {
        PTPNode* next = node->next;
        HeapFree_w(node);
        node = next;
    }
    g_ptp_route_head = 0;
}

// FUNCTION: LEGOLAND 0x00489f50
int UnmarkObjectTiles(Pos* pos)
{
    int i = 0;
    unsigned short key = (unsigned short)((pos->x << 8) + pos->y);
    for (; i < 128; i++) {
        if (g_marked_tiles[i].key == key) {
            g_marked_tiles[i].key = 0xffff;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x004837a0
int RndWalk_LeftTile(Walker* walker, int x, int y)
{
    if (OverNewTile(walker, x, y)) {
        walker->flags &= ~4;
        return 1;
    }
    return 0;
}

/* Original requires a nonempty list containing target: no null fallback. */
// FUNCTION: LEGOLAND 0x00477760
void RemoveClosedNode(RouteNode* target)
{
    RouteNode* node = g_route_closed;
    RouteNode* prev = 0;
    while (node) {
        if (node == target) break;
        prev = node;
        node = node->next;
    }
    if (prev) prev->next = node->next;
    else g_route_closed = node->next;
}

/* Allocation failure is not checked; the list/count updates still occur. */
// FUNCTION: LEGOLAND 0x004995d0
WorkOrder* NewMechanicOrder(void)
{
    WorkOrder* order = (WorkOrder*)CRT_calloc(sizeof(WorkOrder), 1);
    if (!g_mechanic_order_tail) {
        g_mechanic_order_tail = order;
        g_mechanic_orders = order;
    } else {
        g_mechanic_order_tail->next = order;
        g_mechanic_order_tail = order;
    }
    g_mechanic_order_count++;
    return order;
}

// FUNCTION: LEGOLAND 0x004777c0
RouteNode* FindOpenRouteNode(Pos* pos)
{
    RouteNode* node = g_route_open;
    while (node) {
        if (node->pos.x == pos->x && node->pos.y == pos->y) return node;
        node = node->next;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x00477730
RouteNode* FindClosedRouteNode(Pos* pos)
{
    RouteNode* node = g_route_closed;
    while (node) {
        if (node->pos.x == pos->x && node->pos.y == pos->y) return node;
        node = node->next;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x00482b60
int TileJoinsPathNetwork(Pos* pos)
{
    PathSquare* square = FindPathSquare(pos);
    if (square) {
        RefreshEntranceTile(g_path_gfx_batch);
        g_path_gfx_batch = 0;
        if (square->flags & 2) return 1;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x004817d0
PathSquare* FindPathSquareAt(Pos* pos)
{
    PathSquare* square = g_path_squares;
    while (square) {
        if ((pos->x >> 8) >= square->rect.left &&
            (pos->x >> 8) <= square->rect.right &&
            (pos->y >> 8) >= square->rect.top &&
            (pos->y >> 8) <= square->rect.bottom) return square;
        square = square->next;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x0049b270
void EraseWorkOrdersAt(void* object, BPos pos)
{
    int x = pos.x;
    int y = pos.y;
    WorkOrder* order = GetMechanicWorkOrderAt(x, y);
    if (order) EraseMechanicOrder(order);
    order = GetGardenerWorkOrderAt(x, y);
    if (order) EraseGardenerOrder(order);
}

/* As in the mechanic list, calloc failure is not guarded. */
// FUNCTION: LEGOLAND 0x00499570
WorkOrder* NewGardenerOrder(void)
{
    WorkOrder* order = (WorkOrder*)CRT_calloc(sizeof(WorkOrder), 1);
    DBPrintf("Allocated Workorder %x\n", order);
    if (!g_gardener_order_tail) {
        g_gardener_order_tail = order;
        g_gardener_orders = order;
    } else {
        g_gardener_order_tail->next = order;
        g_gardener_order_tail = order;
    }
    g_gardener_order_count++;
    return order;
}

// FUNCTION: LEGOLAND 0x0045ce30
int PathTileIsPatterned(Pos* pos)
{
    Cell* cell = CellForPos(pos);
    if (cell && IsPathCell(cell))
        return g_map_rows[pos->y][pos->x].tile != *g_path_tile_base;
    return 0;
}

// FUNCTION: LEGOLAND 0x004996a0
int IsOrderSpanClear(WorkOrder* order)
{
    int x = order->span_x + order->pos.x + order->rects->left;
    int y = order->rects->bottom - order->span_y + order->pos.y;
    Cell* cell = MapCellAt(x, y);
    if (!cell) return 0;
    if ((cell->flags & 0x88) && cell->object->cls != g_env_class) return 0;
    return 1;
}

/* Independent first two tests allow a corner to advance onto the next side
 * in one call. Offsets include -1 and (right-left)+1 / (bottom-top)+1. */
// FUNCTION: LEGOLAND 0x00499620
void AdvanceOrderSpan(WorkOrder* order)
{
    if (order->f2c == 1) {
        if (order->span_x < order->rects->right - order->rects->left + 1)
            order->span_x++;
        else order->f2c = 7;
    }
    if (order->f2c == 7) {
        if (order->span_y < order->rects->bottom - order->rects->top + 1)
            order->span_y++;
        else order->f2c = 5;
    }
    if (order->f2c == 5) {
        if (order->span_x != -1) order->span_x--;
        else order->f2c = 3;
    } else if (order->f2c == 3) {
        if (order->span_y != -1) order->span_y--;
        else order->f2c = 1;
    }
}

/* Pos keeps the two loop coordinates in the original VC6 register web.
 * Scan row-major, high bit first; off-map copies only initialize flags/rf. */
// FUNCTION: LEGOLAND 0x0045c9c0
unsigned int ScanPathArea5x5(Pos* origin)
{
    Pos p;
    Cell cell;
    unsigned int result = 0;
    unsigned int bit = 0x1000000;
    for (p.y = origin->y; p.y < origin->y + 5; p.y++) {
        for (p.x = origin->x; p.x < origin->x + 5; p.x++) {
            if (p.x >= 0 && p.x < g_map->width && p.y >= 0 && p.y < g_map->height)
                cell = g_map_rows[p.y][p.x];
            else {
                cell.flags = 0x40;
                cell.rf = 0;
            }
            if ((cell.flags & 0x10) && !(cell.rf & 2)) result |= bit;
            bit >>= 1;
        }
    }
    return result;
}

// FUNCTION: LEGOLAND 0x0045cbc0
int GrowPathRectSide(Rect4* rect, int side)
{
    Rect4 edge;
    switch (side) {
    case 2:
        edge.top = edge.bottom = rect->top - 1;
        edge.left = rect->left;
        edge.right = rect->right;
        if (IsPathRectClear(&edge)) { rect->top--; return 1; }
        break;
    case 0:
        edge.top = edge.bottom = rect->bottom + 1;
        edge.left = rect->left;
        edge.right = rect->right;
        if (IsPathRectClear(&edge)) { rect->bottom++; return 1; }
        break;
    case 1:
        edge.top = rect->top;
        edge.bottom = rect->bottom;
        edge.left = edge.right = rect->right + 1;
        if (IsPathRectClear(&edge)) { rect->right++; return 1; }
        break;
    case 3:
        edge.top = rect->top;
        edge.bottom = rect->bottom;
        edge.left = edge.right = rect->left - 1;
        if (IsPathRectClear(&edge)) { rect->left--; return 1; }
        break;
    }
    return 0;
}
