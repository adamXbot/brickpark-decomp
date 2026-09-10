/* LEGOLAND -- scope LL3: route / joint / span clip callees.
 * VC6 SP3 /O2 /Gy /Gd. Types are local; only field OFFSETS are load-bearing.
 * Verification and recovered mechanics: docs/lanes/scope-ll3.md.
 */

typedef struct Pos { int x, y; } Pos;
typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct BPos { unsigned char x, y; } BPos;
typedef union BPosW { unsigned short w; BPos b; } BPosW;
typedef struct BsWater {
    BPosW pos;                      /* +0x00 */
    BPosW owner;                    /* +0x02 */
    int mask;                       /* +0x04 */
    int dist;                       /* +0x08 */
    int seen;                       /* +0x0c */
} BsWater;
typedef struct PathPt { int x, y, z; } PathPt;
typedef struct LFPath { int count; PathPt* pts; } LFPath;
typedef struct RiderNode {
    unsigned char pad00[8];
    struct Bloke* bloke;
} RiderNode;
typedef struct LFQueueNode {
    struct LFQueueNode* next;
    RiderNode* rider;
} LFQueueNode;
typedef struct LFQueue {
    LFPath* path;
    LFQueueNode* head;
} LFQueue;
typedef struct Bloke {
    unsigned char pad00[0x0e];
    unsigned short state;           /* +0x0e */
    unsigned char pad10[0x24 - 0x10];
    Pos target;                     /* +0x24 */
    unsigned char pad2c[0x38 - 0x2c];
    short path_i;                   /* +0x38 */
    unsigned char pad3a[0x68 - 0x3a];
    Pos world;                      /* +0x68 */
    unsigned char pad70[0x73 - 0x70];
    unsigned char dir8;             /* +0x73 */
    unsigned char pad74[0x98 - 0x74];
    unsigned char path[0x14];       /* +0x98 */
} Bloke;
typedef struct PackedSquare { short x, y; } PackedSquare;
typedef struct JointSlot {
    int mask;
    PackedSquare sq[4];
} JointSlot;
typedef struct RoutePos {
    void* node;
    void* obj;
    Vec3f pos;
} RoutePos;
typedef struct TrackCursor {
    float t;
    RoutePos at;
} TrackCursor;
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
    unsigned char pad00[0x0c];
    RoutePos pos;                   /* +0x0c */
    int dimension;                  /* +0x20 */
    float f24;                      /* +0x24 */
    unsigned char pad28[0x70 - 0x28];
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
    unsigned char pad0e[6];
} Cell;
typedef struct MapHdr {
    unsigned char pad00[0x14];
    unsigned short w, h;
} MapHdr;
typedef struct PlaceRect {
    int x0, y0, x1, y1;
    struct PlaceRect* next;
} PlaceRect;
typedef struct SpanAlloc {
    unsigned char pad00[0x24];
    void (*alloc)(void* p, int n);  /* +0x24 */
} SpanAlloc;
typedef struct ClipPlane {
    float nx, ny, d;
} ClipPlane;
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
extern PlaceRect g_joint_probe;                     /* 0x004b5570 */
extern CoasterRoute* g_route_eval;                  /* 0x004d83b4 */
extern RoutePos g_route_eval_at;                    /* 0x004d83a0 */
extern void* g_env_class;                           /* 0x007fd624 */

int TrackPlace_TestSquare(PackedSquare* sq, PlaceRect* ctx); /* 0x0041ee40 */
extern void RouteCar_SetPosition(RouteNode* n, const RoutePos* at, float a); /* 0x0041e820 */
extern void TrackCursor_Bind(TrackCursor* c, void* at, float t); /* 0x0042a5e0 */
extern void PositionRouteCars(CoasterRoute* rt, float a, const RoutePos* at); /* 0x0041da10 */
extern float RouteCar_GetVelocity(RouteNode* n);    /* 0x0041e810 */
extern CoasterRec g_castle;                         /* 0x00829ae0 */
extern unsigned int JointBitFromIndex(int index);   /* 0x0041cc90 */
extern void RouteNode_GetTailTangent(RouteNode* n, Vec3f* dir); /* 0x0041e930 */
extern void Sub_429f30(Vec3f* dir, float step, RoutePos* from, float f40,
                       float tol, RoutePos* out, float* out_a); /* 0x00429f30 */
int Span_ClipPlane(int n, void* in, void* out, void** cursor,
                   ClipPlane* plane);                /* 0x0041f050 */
extern BsWater* BsWater_FindAt(int x, int y);       /* 0x0041c890 */
extern int CalcMoveLine(Pos from, Pos to, void* path); /* 0x00480740 */
extern int NewDirForAction(Bloke* b, unsigned char dir); /* 0x004833d0 */
void BsRoute_Trace(int x, int y, int x1, int y1, BPosW* owner, int* ok); /* 0x0041c940 */
extern float RouteNode_GetAcceleration(RouteNode* n); /* 0x0041e7e0 */
extern void Span_EvalRange(void (*fn)(float, RouteCarSample*), void* ops,
                           float a, float dt, RouteCarSample* out); /* 0x0041f4e0 */
extern char g_span_eval_ops[];                      /* 0x004d8270 */
extern float g_mass_hist[];                         /* 0x004d829c */
extern int g_mass_hist_i;                           /* 0x004d83c0 */
extern void* g_span_rows[];                         /* 0x004d88cc */

typedef struct SpanOps2 {
    unsigned char pad00[4];
    void (*combine)(void* a, void* b, void* c); /* +0x04 */
    unsigned char pad08[0x20 - 8];
    void* (*alloc)(int n);                      /* +0x20 */
} SpanOps2;

extern void* g_span_context;                        /* 0x004b55fc */
extern int g_track_place_enabled;                   /* 0x004b55f4 */
extern MapHdr* g_map;                               /* 0x004bcbf4 */
extern Cell** g_map_rows;                           /* 0x00801400 */
extern int g_clip_stage[];                          /* 0x004d884c */
extern void* g_clip_ping[2];                        /* 0x004b5600 */
extern void* g_clip_cursor;                         /* 0x004d87c4 */
extern int g_clip_buf;                              /* 0x004d83c4 */

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
int JointSlot_TestSquare(PackedSquare* sq, PlaceRect* ctx)
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
/* Which of the slot's four candidate squares equals `sq`.  Returns the
 * index, or -1.  Only bits set in the slot mask are considered.
 * `bit = 1` before `mask = *p` is the TEST operand-order lever
 * (`test ecx,esi`); mask-first emits `test esi,ecx`. */
// FUNCTION: LEGOLAND 0x0041cd40
int JointSlot_Find(const PackedSquare* sq, JointSlot* slot)
{
    int* p = (int*)slot;
    int i = 0;
    int bit = 1;
    int mask = *p;
    const int* want = (const int*)sq;

    p++;
    do {
        if (bit & mask) {
            if (*want == *p)
                return i;
        }
        bit <<= 1;
        i++;
        p++;
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

/* Put the whole train on the track from one (at, a).  Same walk as
 * PositionRouteCars but seats each car with RouteCar_PlaceAndBind so both
 * bogie cursors are rebound. */
// FUNCTION: LEGOLAND 0x0041d950
void Route_SetTrainAt(CoasterRoute* rt, float a, const RoutePos* at)
{
    Vec3f dir;
    RoutePos prev_at;
    RoutePos next_at;
    RouteNode* head = &rt->head;
    RouteNode* n = rt->head.next;
    RouteNode* prev = head;
    float spacing;
    float next_a;

    rt->f24 = a;
    rt->pos = *at;
    RouteCar_PlaceAndBind(prev, at, a);
    while (n != head) {
        spacing = prev->rear.t;
        prev_at = prev->rear.at;
        RouteNode_GetTailTangent(prev, &dir);
        Sub_429f30(&dir, 30.0f, &prev_at, spacing, 4.8f, &next_at, &next_a);
        RouteCar_PlaceAndBind(n, &next_at, next_a);
        prev = n;
        n = n->next;
    }
}

/* Copy n vertex pointers into the staging ring, then clip the ring against
 * each plane, ping-ponging between g_clip_ping[2].  Writes the survivor
 * count and returns the last output buffer, or 0 if a plane empties it. */
// FUNCTION: LEGOLAND 0x0041f2b0
void* Raster_ClipAgainstPlanes(int n, int* src, int* out_count, int nplanes,
                               void* planes)
{
    int i;

    if (n > 0) {
        char* d = (char*)g_clip_stage;
        int delta = (char*)src - d;
        int c = n;
        do {
            *(int*)d = *(int*)(d + delta);
            d += 4;
        } while (--c);
    }
    g_clip_cursor = &g_clip_buf;
    i = 0;
    while (i < nplanes) {
        n = Span_ClipPlane(
            n,
            g_clip_ping[i & 1],
            g_clip_ping[(i - 1) & 1],
            &g_clip_cursor,
            planes);
        if (n == 0) {
            *out_count = 0;
            return 0;
        }
        i++;
        planes = (char*)planes + 0xc;
    }
    *out_count = n;
    return g_clip_ping[i & 1];
}

/* Walk each PlaceRect of the probe (offset by `sq`) and refuse if any
 * in-bounds cell returns MapCell_AllowTrack != 0.  Disabled placement
 * and an empty list both succeed. */
/* x = sq->x then lim = sq->x + ctx->x1 (textual sq->x repeat) emits
 * `mov esi,eax / add esi,edx`; a named sx fuses to `add esi,eax`. */
// FUNCTION: LEGOLAND 0x0041ee40
int TrackPlace_TestSquare(PackedSquare* sq, PlaceRect* ctx)
{
    if (g_track_place_enabled) {
        do {
            int y = ctx->y0;
            int y1 = ctx->y1;
            int sy = sq->y;
            y += sy;
            y1 += sy;
            if (y < y1) {
                do {
                    int x = sq->x;
                    int lim = sq->x + ctx->x1;
                    x += ctx->x0;
                    if (x < lim) {
                        int off = x * 20;
                        do {
                            Cell* cell;
                            if (off < 0 || x >= g_map->w || y < 0
                                || y >= g_map->h)
                                cell = 0;
                            else
                                cell = (Cell*)((char*)g_map_rows[y] + off);
                            if (MapCell_AllowTrack(cell))
                                return 0;
                            x++;
                            off += 20;
                        } while (x < sq->x + ctx->x1);
                    }
                    y++;
                } while (y < sq->y + ctx->y1);
            }
            ctx = ctx->next;
        } while (ctx);
    }
    return 1;
}

/* Clip a 3-vertex ring against the span clip set.  mask 0xf is a no-op
 * (*count = 3).  Otherwise set the vertex stride and ping-pong the ring
 * through the planes named by the low/high nibble of the mask. */
/* `if (1) { switch { return Clip(); } } return count` is the layout
 * hammer: constant-true wrapper keeps `dec/jne` + case-3-inline and
 * stops the three all-inline tails merging, so case 1 colors g in ecx. */
// FUNCTION: LEGOLAND 0x0041ef60
void* Raster_ClipPoly(void** v, int* count, int mask, int n)
{
    int flags = 0;

    if (mask == 0xf) {
        *count = 3;
        return v;
    }
    Span_SetVertexBuf((void*)(n + 1));
    if ((mask & 3) < 3)
        flags = 1;
    if ((mask & 0xc) < 0xc)
        flags |= 2;
    if (1) {
        switch (flags) {
        case 1:
            return Raster_ClipAgainstPlanes(3, (int*)v, count, 2,
                                            (char*)g_span_context + 4);
        case 2:
            return Raster_ClipAgainstPlanes(3, (int*)v, count, 2,
                                            (char*)g_span_context + 0x1c);
        case 3:
            return Raster_ClipAgainstPlanes(3, (int*)v, count, 4,
                                            (char*)g_span_context + 4);
        }
    }
    return count;
}

/* Walk one queued bloke one pace along the queue path: world target =
 * path->pts[index] + (tx, ty) in 24.8, then CalcMoveLine / NewDirForAction
 * and index++.  Clamp at the last point; if another bloke already occupies
 * the new index, step back. */
/* Field stores into b->target then CalcMoveLine(..., b->target, pathp)
 * keep both stores before the pushes and emit mov eax,ecx / reload to.x. */
// FUNCTION: LEGOLAND 0x00411fa0
void LFQueue_StepRider(LFQueue* q, int tx, int ty, Bloke* b)
{
    LFPath* path = q->path;
    PathPt* pt = &path->pts[b->path_i];
    unsigned char a;
    LFQueueNode* n;
    short i;
    Pos to;
    void* pathp;

    to.x = pt->x + tx;
    to.y = pt->y + ty;
    pathp = b->path;
    to.x <<= 8;
    to.y <<= 8;
    b->target.x = to.x;
    b->target.y = to.y;
    a = (unsigned char)(CalcMoveLine(b->world, b->target, pathp) + 0x10);
    b->state = 7;
    b->dir8 = a;
    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
    b->path_i++;
    i = b->path_i;
    if ((int)i >= path->count) {
        b->path_i = (short)path->count - 1;
        return;
    }
    n = q->head;
    while (n) {
        Bloke* o = n->rider->bloke;
        if (o->path_i == i) {
            if (o != b) {
                i--;
                b->path_i = i;
                return;
            }
        }
        n = n->next;
    }
}

/* Recursive four-way flood from (x, y) over the school's water graph
 * (neighbours five squares away).  Sets *ok when it reaches (x1, y1).
 * West is a tail-loop: x -= 5 and restart, not a recursive call. */
/* Twin of JungleCruise_TraceRoute: west arm is a tail-call that VC6 turns
 * into a loop. Volatile shims pin w/owner to frame homes so the body is
 * byte-exact (339B) even if allocation still disagrees. */
#define BS_W_MEM     (*(BsWater* volatile*)&w)
#define BS_OWNER_MEM (*(BPosW*   volatile*)&owner)
/* FLOOR: 130i/339B byte-exact, 105 mismatches. Same NG22 phase-order
 * ALLOCATION as JungleCruise_TraceRoute (jcroute.c): original allocates
 * esi/edi/ebp/ebx to x/y/x1/y1 with w in the dead arg3 slot; our
 * tail-call-to-loop conversion runs before allocation and ranks the
 * pointers first. Dead trailing statements restore the ints but lose the
 * loop. Retired.
 * LL23 2026-09-10 re-verified against the binary and the floor HOLDS: the
 * twin `JungleCruise_TraceRoute` (jcroute.c 0x00437260) audits at exactly
 * the same 130i/339B and 105 mismatches, so this is one shared allocation
 * floor reached from two independent reconstructions, not a spelling miss
 * in either.  The original's prologue is `mov eax,[esp+0x18]; push ebx;
 * push ebp; push esi; mov ecx,[eax]; push edi; cmp ecx,1; je` and only
 * THEN loads ebx=y1, ebp=x1, esi=x, edi=y, with `w` written into x1's dead
 * home at [esp+0x1c]: four int parameters ranked above every pointer.
 * Not re-run. */
// WIP-FUNCTION: LEGOLAND 0x0041c940  (FLOOR, 130i/339B byte-exact, 105 NG22)
void BsRoute_Trace(int x, int y, int x1, int y1, BPosW* owner, int* ok)
{
    BsWater* w;
    BsWater* p;

    if (*ok == 1)
        return;
    w = BsWater_FindAt(x, y);
    if (!w)
        return;
    if (w->owner.w != BS_OWNER_MEM->w)
        return;
    if (x == x1 && y == y1) {
        *ok = 1;
        return;
    }
    w->seen = 1;
    if (w->mask & 1) {
        p = BsWater_FindAt(x, y - 5);
        if (p && p->seen == 0)
            BsRoute_Trace(x, y - 5, x1, y1, BS_OWNER_MEM, ok);
    }
    if (BS_W_MEM->mask & 2) {
        p = BsWater_FindAt(x + 5, y);
        if (p && p->seen == 0)
            BsRoute_Trace(x + 5, y, x1, y1, BS_OWNER_MEM, ok);
    }
    if (BS_W_MEM->mask & 4) {
        p = BsWater_FindAt(x, y + 5);
        if (p && p->seen == 0)
            BsRoute_Trace(x, y + 5, x1, y1, BS_OWNER_MEM, ok);
    }
    if (BS_W_MEM->mask & 8) {
        int nx = x - 5;
        p = BsWater_FindAt(nx, y);
        if (p && p->seen == 0)
            BsRoute_Trace(nx, y, x1, y1, BS_OWNER_MEM, ok);
    }
}

/* Snapshot the train, run the shade evaluator over CollectCarSample, then
 * fold each car's heading*heading * acceleration * K into *mass.  *power
 * is the sample energy.  The mass is also pushed into a 64-slot ring. */
/* Residual: 77i/259B, matchfull 65/77 (84%), audit 42 mis. Dest-coalesce
 * `mov ebx,eax` / sunk `add ebx,0x70` vs `lea ebx,[eax+0x70]`.
 * ClipPlane imm8 store through `&p->head` after the snapshot DOES force
 * delay-slot lea ebx with eax live for [eax+0x24] (66/77) — but the store
 * is coupled: every spelling that drops `mov [ebx],0` dest-coalesces again.
 * Known-zero / dead-alias / g_route_eval from n-0x70 / SetTrainAt next /
 * if(n) / Fst / EvalRange commas CSE away or steal edx. Cannot exact with
 * the extra store; cannot lea without it on this body. q after add esp,4;
 * hist ecx/edx swap sticky.
 * LL23 2026-09-10, the Codex-F scope-V CANCELLED-PAIR anchor is INERT here.
 * `struct { RouteNode* np; } t; t.np = &p->head; t.np += A; t.np -= A;
 * n = t.np;` with A = p / rt / mass / power / &fr (values already in a
 * register) folds completely in the front end -- 77i/259B and 65/77,
 * bit-identical to the tip, so the web is NOT kept separate the way scope
 * V's `t.x = bx` was.  With A a link-time address constant assigned to a
 * second struct member first (&g_route_eval, &g_route_eval_at,
 * g_span_eval_ops, &g_mass_hist[0], (int)p), the anchor costs a real
 * `mov reg,imm32` -- 78i/262B, worse than the tip and past the extent.
 * The lever needs an anchor the body already materialises for its own
 * reasons; this body has none live at the `&p->head` site. */ 
// WIP-FUNCTION: LEGOLAND 0x0041db90  (84%, lea ebx coupled to imm8 store)
void Route_GetMassAndPower(CoasterRoute* rt, float* mass, float* power)
{
    struct {
        float f24;
        RoutePos pos;
        float sample[21];
    } fr;
    RouteNode* n;
    float* h;
    int k;
    CoasterRoute* p = rt;
    CoasterRoute* q;

    n = &p->head;
    fr.pos = p->pos;
    fr.f24 = p->f24;
    g_route_eval = p;
    g_route_eval_at = p->pos;
    Span_EvalRange(Route_CollectCarSample, g_span_eval_ops, fr.f24, 0.1f,
                   (RouteCarSample*)fr.sample);
    *power = ((RouteCarSample*)fr.sample)->energy;
    *mass = 0.0f;
    h = &((RouteCarSample*)fr.sample)->heading[0].x;
    do {
        float acc;
        acc = 0.0f;
        k = 3;
        do {
            float v = *h;
            acc += v * v;
            h++;
        } while (--k);
        {
            float a = RouteNode_GetAcceleration(n) * acc;
            q = *(CoasterRoute* volatile*)&rt;
            *mass += a * 2.52015616e-06f;
        }
        n = n->next;
    } while (n != &q->head);
    q->pos = fr.pos;
    q->f24 = fr.f24;
    g_mass_hist[g_mass_hist_i & 0x3f] = *mass;
    g_mass_hist_i++;
}

/* Alloc a triangular pair table, evaluate `eval` at n+1 samples centred on
 * a, then combine adjacent pairs down the rows.  Returns the row-pointer
 * table at 0x004d88cc. */
/* Latch `left` must be block-local after the inner combine: load the dead
 * `a` slot, then count--, row++, dec, store. A function-scope `left` steals
 * ebx/ebp (65%). `--*(volatile int*)&a` after the pair uses ecx (5 mis). */
// FUNCTION: LEGOLAND 0x0041f3e0
void** Span_FillEvalTable(void (*eval)(float, void*), SpanOps2* ops, int n,
                          float a, float b)
{
    int count = n;
    int m;
    void* mem;
    int i;
    void** row;

    m = count + 1;
    mem = ops->alloc((m * (m + 1)) >> 1);

    if (mem == 0)
        return 0;
    if (count >= 0) {
        void** p = g_span_rows;
        int stride = m * 4;
        do {
            *p = mem;
            mem = (char*)mem + stride;
            p++;
            stride -= 4;
        } while (--m);
    }
    i = 0;
    a = a - (float)(count >> 1) * b;
    if (count >= 0) {
        do {
            eval(a, ((void**)g_span_rows[0])[i]);
            a = a + b;
            i++;
        } while (i <= count);
    }
    if (count >= 1) {
        row = &g_span_rows[1];
        *(volatile int*)&a = count;
        do {
            for (i = 0; i < count; i++) {
                void** prev = (void**)row[-1];
                void** cur = (void**)row[0];
                ops->combine(prev[i + 1], prev[i], cur[i]);
            }
            {
                int left = *(volatile int*)&a;
                count--;
                row++;
                left--;
                *(volatile int*)&a = left;
                if (left == 0)
                    break;
            }
        } while (1);
    }
    return g_span_rows;
}

/* Sutherland-Hodgman one-plane clip of a vertex-pointer ring.  in[n] is
 * closed to in[0].  Negative plane distance is inside.  Crossing edges
 * lerp every dword 0..g_span_vtx of the PolyVtx into *cursor and emit
 * that cursor pointer; both-inside emits the previous vertex. */
/* LL23 2026-09-10 — ESCAPES fixed by rebuilding, not by a pin.  The old
 * body was 189i (10 past the original's 179): a fabricated `destrel` +
 * volatile-pad store in the loop header (3), pa/na colouring copies (2), a
 * plane reload (1) and a register-materialised out_n (1) pushed the second
 * epilogue past the extent, so the early-out `jl` targeted an address the
 * original does not have.  Rebuilt index-based (prev[k]/nxt[k]/dst[k], one
 * cursor + two memory offsets, ENTER's k outside the vtx guard and LEAVE's
 * ebp counter), 179i/588B, no ESCAPES, audit 137 (was 173).
 * Residual is the frame and one register: VC6 gives us `sub esp,0x24`
 * (7 local slots + a hole) where the original has 0x2c (9 locals + the
 * unused +0x30/+0x38 pair), because the two dead parameter homes go to
 * `bits`/`pabs`+`k` here and to `in`/`t` in the original; and `plane`
 * takes ebx as a whole-function web instead of the original's ecx that is
 * reloaded from [esp+0x50] after each __ftol loop.  Every slot-numbered
 * operand therefore differs.  Ruled out this wave: declaration order (5
 * orders, inert); `pabs` as a 12-byte padded struct, address-taken or not
 * (frame unchanged); `ClipPlane* volatile plane` (does free ebp and lands
 * `xor ebp,ebp`, but costs 4 reload instructions -> 183i); the Codex-F
 * operand-alias lever on the fild pair (`q = nxt`, a `char*` cast, an
 * alias on plane) — the fild order is already correct here and the alias
 * is inert; every lerp spelling that might flip the cursor base from nxt
 * to prev (separate temps, pre-loaded delta, negated delta, repeated
 * subscript) — inert, VC6 always bases the cursor on the subtraction's
 * left operand; `*(volatile int*)&out_n = 0` (177i, worse). */
// WIP-FUNCTION: LEGOLAND 0x0041f050  (33.5%, frame 0x24 vs 0x2c, plane in ebx)
int Span_ClipPlane(int n, void* in_v, void* out_v, void** cursor, ClipPlane* plane)
{
    void** in = (void**)in_v;
    int* prev;
    int* nxt;
    int* dst;
    int out_n = 0;
    int prev_sign;
    int cls;
    int abs_r;
    int left;
    union { float f; int i; } bits;
    union { float f; int i; } pabs;

    dst = (int*)*cursor;
    in[n] = in[0];
    left = n;
    nxt = (int*)in[0];
    bits.f = plane->d - ((float)nxt[2] * plane->nx + (float)nxt[1] * plane->ny);
    abs_r = bits.i;
    prev_sign = abs_r;
    abs_r &= 0x7fffffff;
    prev_sign &= (int)0x80000000;
    if (n >= 1) {
        cls = prev_sign;
        in++;
        do {
            int next_sign;
            prev_sign = cls;
            prev = nxt;
            pabs.i = abs_r;
            nxt = (int*)*in;
            cls = (prev_sign >> 1) & 0x40000000;
            bits.f = plane->d - ((float)nxt[2] * plane->nx + (float)nxt[1] * plane->ny);
            abs_r = bits.i;
            next_sign = abs_r;
            abs_r &= 0x7fffffff;
            next_sign &= (int)0x80000000;
            bits.i = abs_r;
            cls |= next_sign;
            if (cls != (int)0x80000000) {
                if (cls != (int)0xc0000000) {
                    if (cls == 0x40000000) {
                        float t = pabs.f / (pabs.f + bits.f);
                        int k = 0;
                        *(void**)out_v = prev;
                        out_v = (char*)out_v + 4;
                        if ((int)g_span_vtx >= 0) {
                                do {
                                    int a = prev[k];
                                    int dd = nxt[k] - a;
                                    dst[k] = a + (int)((float)dd * t);
                                    k++;
                                } while (k <= (int)g_span_vtx);
                        }
                        *(void**)out_v = dst;
                        out_v = (char*)out_v + 4;
                        dst = (int*)((char*)dst + g_span_vtx_stride);
                        out_n += 2;
                    }
                } else {
                    *(void**)out_v = prev;
                    out_v = (char*)out_v + 4;
                    out_n++;
                }
            } else {
                float t = bits.f / (pabs.f + bits.f);
                int k = 0;
                if ((int)g_span_vtx >= 0) {
                            do {
                                int a = nxt[k];
                                int dd = prev[k] - a;
                                dst[k] = a + (int)((float)dd * t);
                                k++;
                            } while (k <= (int)g_span_vtx);
                }
                *(void**)out_v = dst;
                out_v = (char*)out_v + 4;
                dst = (int*)((char*)dst + g_span_vtx_stride);
                out_n++;
            }
            in++;
        } while (--left);
    }
    *cursor = dst;
    return out_n;
}
