/* LEGOLAND -- scope LL3: route / joint / span clip callees.
 * VC6 SP3 /O2 /Gy /Gd. Types are local; only field OFFSETS are load-bearing.
 * Verification and recovered mechanics: docs/lanes/scope-ll3.md.
 */

typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct PackedSquare { short x, y; } PackedSquare;
typedef struct JointSlot {
    int mask;
    PackedSquare sq[4];
} JointSlot;
typedef struct TrackCursor {
    float t;
    unsigned char at[0x14];
} TrackCursor;
typedef struct RoutePos {
    void* node;
    void* obj;
    Vec3f pos;
} RoutePos;
typedef struct RouteNode {
    unsigned char pad00[8];
    TrackCursor front;              /* +0x08 */
    unsigned char pad20[0x40 - 0x20];
    TrackCursor rear;               /* +0x40 */
    unsigned char pad58[0xb8 - 0x58];
    Vec3f heading;                  /* +0xb8 */
    float velocity;                 /* +0xc4 */
    unsigned char padc8[0xe8 - 0xc8];
    struct RouteNode* next;         /* +0xe8 */
} RouteNode;
typedef struct CoasterRoute {
    unsigned char pad00[0x70];
    RouteNode head;                 /* +0x70 */
} CoasterRoute;
typedef struct MapObj {
    unsigned char pad00[0x0c];
    struct ObjDef* cls;             /* +0x0c */
} MapObj;
typedef struct ObjDef {
    unsigned char pad00[0x1c];
    unsigned int flags;             /* +0x1c */
} ObjDef;
typedef struct Cell {
    MapObj* obj;                    /* +0x00 */
    unsigned char pad04[8];
    unsigned short flags;           /* +0x0c */
} Cell;
typedef struct SpanAlloc {
    unsigned char pad00[0x24];
    void (*alloc)(void* p, int n);  /* +0x24 */
} SpanAlloc;
typedef struct RouteCarSample {
    int count;                      /* +0x00  1 + 3 * nodes */
    float energy;                   /* +0x04  sum of car +0xc4 */
    Vec3f heading[1];               /* +0x08 */
} RouteCarSample;
typedef struct TrackNode TrackNode;
typedef struct TrackJoint {
    int dir;
    int f04;
    TrackNode* node;
} TrackJoint;
struct TrackNode {
    int state;
    PackedSquare sq;
    void* cls;
    void* desc;
    void* owner;
    TrackJoint jin;                 /* +0x14 */
    TrackJoint jout;                /* +0x20 */
};
typedef struct TrackDesc {
    unsigned char pad00[0x10];
    PackedSquare tail_off;          /* +0x10 */
    unsigned char pad14[4];
    PackedSquare head_off;          /* +0x18 */
} TrackDesc;
typedef struct CoasterRec {
    int state;                      /* +0x00  2 = closed circuit */
    unsigned char pad04[0xa8 - 4];
    TrackNode* head_node;           /* +0xa8 */
    JointSlot head_slot;            /* +0xac */
    TrackNode* tail_node;           /* +0xc0 */
    JointSlot tail_slot;            /* +0xc4 */
} CoasterRec;
typedef struct TrackFit {
    int flags;
    CoasterRec* owner;              /* +0x04 */
    int a;                          /* +0x08 */
    void* an;
    int b;                          /* +0x10 */
} TrackFit;

extern int g_span_vtx_stride;                       /* 0x004b5608 */
extern void* g_span_vtx;                            /* 0x004b560c */
extern PackedSquare g_joint_delta[4];               /* 0x004b5584 */
extern PackedSquare g_joint_probe;                  /* 0x004b5570 */
extern CoasterRoute* g_route_eval;                  /* 0x004d83b4 */
extern RoutePos g_route_eval_at;                    /* 0x004d83a0 */
extern void* g_env_class;                           /* 0x007fd624 */

extern int TrackPlace_TestSquare(PackedSquare* sq, PackedSquare* ctx); /* 0x0041ee40 */
extern void RouteCar_SetPosition(RouteNode* n, const RoutePos* at, float a); /* 0x0041e820 */
extern void TrackCursor_Bind(TrackCursor* c, void* at, float t); /* 0x0042a5e0 */
extern void PositionRouteCars(CoasterRoute* rt, float a, const RoutePos* at); /* 0x0041da10 */
extern float RouteCar_GetVelocity(RouteNode* n);    /* 0x0041e810 */
extern CoasterRec g_castle;                         /* 0x00829ae0 */
extern unsigned int JointBitFromIndex(int index);   /* 0x0041cc90 */

/* Stores the raster vertex stride (sizeof PolyVtx = 0x1c) and the live
 * vertex pointer the span clippers walk. */
// FUNCTION: LEGOLAND 0x0041f030
void Span_SetVertexBuf(void* v)
{
    g_span_vtx_stride = 0x1c;
    g_span_vtx = v;
}

/* Thin wrapper: JointSlot_Set probes one candidate square through the
 * placement tester. */
// FUNCTION: LEGOLAND 0x0041cd20
int JointSlot_TestSquare(PackedSquare* sq, PackedSquare* ctx)
{
    return TrackPlace_TestSquare(sq, ctx);
}

/* Copy the car's heading (RouteNode +0xb8) into an out Vec3f. */
// FUNCTION: LEGOLAND 0x0041e7f0
void RouteCar_GetHeading(RouteNode* n, Vec3f* out)
{
    n = (RouteNode*)((char*)n + 0xb8);
    out->x = ((Vec3f*)n)->x;
    out->y = ((Vec3f*)n)->y;
    out->z = ((Vec3f*)n)->z;
}

/* Hands the span allocator (n+1)*(n+2)/2 slots -- the triangular number of
 * a (n+2)-vertex ring's unique pairs. */
// FUNCTION: LEGOLAND 0x0041f4c0
void Span_AllocPairs(void** p, SpanAlloc* a, int n)
{
    int m = n + 1;
    a->alloc(*p, (m * (m + 1)) >> 1);
}

/* Seat one car, then re-bind both bogie cursors from the values
 * RouteCar_SetPosition just wrote. */
// FUNCTION: LEGOLAND 0x0041e8f0
void RouteCar_PlaceAndBind(RouteNode* n, const RoutePos* at, float a)
{
    RouteCar_SetPosition(n, at, a);
    TrackCursor_Bind(&n->front, &n->front.at, n->front.t);
    TrackCursor_Bind(&n->rear, &n->rear.at, n->rear.t);
}

/* Which of the slot's four candidate squares equals `sq`.  Returns the
 * index, or -1.  Only bits set in the slot mask are considered. */
/* Residual: slot walker in ecx and bit in edx; original has slot in edx
 * and bit in ecx (`test ecx,esi` / `add edx,4`). Same 27i/55B. */
// WIP-FUNCTION: LEGOLAND 0x0041cd40  (70%, slot/bit ecx/edx vs edx/ecx)
int JointSlot_Find(const PackedSquare* sq, JointSlot* slot)
{
    int mask = *(int*)slot;
    int i;
    int bit;

    slot = (JointSlot*)((int*)slot + 1);
    i = 0;
    bit = 1;
    do {
        if (mask & bit) {
            if (*(const int*)sq == *(int*)slot)
                return i;
        }
        bit <<= 1;
        i++;
        slot = (JointSlot*)((int*)slot + 1);
    } while (i <= 3);
    return -1;
}

/* Fill a JointSlot from a base square and a four-bit direction mask.
 * Each live direction writes base + g_joint_delta[i] and keeps the bit
 * only when the placement probe returns 1. */
// FUNCTION: LEGOLAND 0x0041cd80
void JointSlot_Set(const PackedSquare* sq, JointSlot* slot, int direction)
{
    int i;
    int bit = 1;

    slot->mask = 0;
    for (i = 0; i <= 3; i++) {
        if (direction & bit) {
            slot->sq[i].x = (short)(sq->x + g_joint_delta[i].x);
            slot->sq[i].y = (short)(sq->y + g_joint_delta[i].y);
            if (JointSlot_TestSquare(&slot->sq[i], &g_joint_probe) == 1)
                slot->mask |= bit;
        }
        bit <<= 1;
    }
}

/* Place the eval-route train, then pack every car's velocity sum and
 * heading into the sample block.  count = 1 + 3 * nodes (the energy
 * float plus three heading components each).  Includes the +0x70 head. */
// FUNCTION: LEGOLAND 0x0041db20
void Route_CollectCarSample(float a, RouteCarSample* out)
{
    CoasterRoute* rt = g_route_eval;
    int count = 1;
    RouteNode* n = &rt->head;
    Vec3f* dst;

    PositionRouteCars(rt, a, &g_route_eval_at);
    out->energy = 0.0f;
    dst = out->heading;
    do {
        out->energy += RouteCar_GetVelocity(n);
        RouteCar_GetHeading(n, dst);
        n = n->next;
        count += 3;
        dst++;
    } while (n != &g_route_eval->head);
    out->count = count;
}

/* Can a track piece occupy this map cell?  NULL and flag 0x40 (blocked)
 * succeed; a cell with none of the 0x8f8 object/path bits, the environment
 * class, flag 0x800, or a 0x200000 ("may be built over") class on a 0x88
 * cell all refuse. */
// FUNCTION: LEGOLAND 0x0041ede0
int MapCell_AllowTrack(Cell* cell)
{
    unsigned short flags;
    MapObj* obj;
    ObjDef* cls;

    if (cell == 0)
        return 1;
    flags = cell->flags;
    if ((flags & 0x8f8) == 0)
        return 0;
    if (flags & 0x40)
        return 1;
    obj = cell->obj;
    if (obj)
        cls = obj->cls;
    else
        cls = 0;
    if (flags & 8) {
        if (obj) {
            if (cls == g_env_class)
                return 0;
        }
    }
    if (flags & 0x800)
        return 0;
    if (flags & 0x88) {
        if (cls) {
            if (cls->flags & 0x200000)
                return 0;
        }
    }
    return 1;
}

/* Geometric half of the two fit checkers: refuse a closed circuit, stamp
 * the fit owner with &g_castle, and turn each end's joint offset into a
 * candidate index against that end's JointSlot.  Either index -1 means
 * that end is free; both -1 means the piece touches nothing and the fit
 * fails.  A hit also writes 1<<index into the live end node's dir. */
// FUNCTION: LEGOLAND 0x0041d210
int TrackFitFindPartners(const PackedSquare* sq, TrackFit* f, TrackDesc* d)
{
    PackedSquare at;
    int idx;

    if (g_castle.state != 2) {
        f->owner = &g_castle;
        at.x = (short)(sq->x + d->head_off.x);
        at.y = (short)(sq->y + d->head_off.y);
        idx = JointSlot_Find(&at, &g_castle.head_slot);
        f->a = idx;
        if (idx != -1)
            g_castle.head_node->jout.dir = (int)JointBitFromIndex(idx);
        at.x = (short)(sq->x + d->tail_off.x);
        at.y = (short)(sq->y + d->tail_off.y);
        idx = JointSlot_Find(&at, &g_castle.tail_slot);
        f->b = idx;
        if (idx != -1)
            g_castle.tail_node->jin.dir = (int)JointBitFromIndex(idx);
        if (f->a != -1 || f->b != -1)
            return 1;
    }
    return 0;
}
