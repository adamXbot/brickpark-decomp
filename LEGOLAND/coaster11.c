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
extern void Sub_429f30(Vec3f* dir, float step, RoutePos* from, float f40, float tol, RoutePos* out, float* out_a); /* 0x00429f30 */
int Span_ClipPlane(int n, void* in, void* out, void** cursor,
                   void* plane);                    /* 0x0041f050 */
extern BsWater* BsWater_FindAt(int x, int y);       /* 0x0041c890 */
extern int CalcMoveLine(Pos from, Pos to, void* path); /* 0x00480740 */
extern int NewDirForAction(Bloke* b, unsigned char dir); /* 0x004833d0 */
void BsRoute_Trace(int x, int y, int x1, int y1, BPosW* owner, int* ok); /* 0x0041c940 */
extern float RouteNode_GetAcceleration(RouteNode* n); /* 0x0041e7e0 */
/* The definition's real return type, for the portable build only: a stale
 * extern name whose signature disagrees with its body makes gen_link.py
 * bridge the two with a CAST, the cast call lowers to `call_indirect`, and
 * binaryen's directize pass turns a constant-index call_indirect into an
 * invalid DIRECT call -- the module then fails validation hundreds of
 * functions away (scope PORT-M2 section 4). 0x0041f4e0 is coastershade2.c:798 `int Romberg_Evaluate(RombergFn, PhysOps*, float, float, PhysVec*)` -- the same row PORT-M2 section 4 fixed in coaster10.c. The VC6 arm is the
 * shipped spelling and its bytes cannot move: cdecl discards EAX here. */
#ifndef LEGOLAND_PORTABLE
extern void Span_EvalRange(void (*fn)(float, RouteCarSample*), void* ops, float a, float dt, RouteCarSample* out); /* 0x0041f4e0 */
#else
extern int  Span_EvalRange(void (*fn)(float, RouteCarSample*), void* ops, float a, float dt, RouteCarSample* out); /* 0x0041f4e0 */
#endif
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
 * loop. Mass cancel on x1 is 93/135 (orig prologue) but audit ESCAPES
 * at 348B; wok is 87/130 / audit 109. Neither taken. Retired. */
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
/* Split `{f24, pos, sample[21]}` into three locals: a struct-field f24
 * store sinks past the sample lea (76/77); a scalar f24 homes first
 * (`mov [esp+0x10],ecx` then `lea edx,[esp+0x28]`). Mass_AccQ(&rt) puts
 * q in the GetAccel delay slot. Named `prt` plus cancel of `&q->head`
 * through h, then always-false `if (end != t.x) k = t.x` folds add/sub
 * without a byte store. Restore via `end = q`. */
static __inline float Mass_AccQ(RouteNode* n, float acc, CoasterRoute** prt,
                                CoasterRoute** qout)
{
    float a = RouteNode_GetAcceleration(n) * acc;
    *qout = *(CoasterRoute* volatile*)prt;
    return a;
}

// FUNCTION: LEGOLAND 0x0041db90
void Route_GetMassAndPower(CoasterRoute* rt, float* mass, float* power)
{
    float f24;
    RoutePos pos;
    float sample[21];
    RouteNode* n;
    RouteNode* end;
    float* h;
    int k;
    CoasterRoute* p = rt;
    CoasterRoute* q;

    n = &p->head;
    pos = p->pos;
    f24 = p->f24;
    g_route_eval = p;
    g_route_eval_at = p->pos;
    Span_EvalRange(Route_CollectCarSample, g_span_eval_ops, f24, 0.1f,
                   (RouteCarSample*)sample);
    *power = ((RouteCarSample*)sample)->energy;
    *mass = 0.0f;
    h = &((RouteCarSample*)sample)->heading[0].x;
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
            CoasterRoute** prt = (CoasterRoute**)&rt;
            float a = Mass_AccQ(n, acc, &rt, &q);
            (void)prt;
            {
                struct { int x, y; } t;
                t.y = (int)h;
                t.x = (int)&q->head;
                t.x += t.y;
                t.x -= t.y;
                end = (RouteNode*)t.x;
                if (end != (RouteNode*)t.x)
                    k = t.x;
            }
            *mass += a * 2.52015616e-06f;
        }
        n = n->next;
    } while (n != end);
    end = (RouteNode*)q;
    ((CoasterRoute*)end)->pos = pos;
    q->f24 = f24;
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
/* Residual: 73/186 (39.2%), 179i/604B, no ESCAPES, 2-ret, dest +0x14.
 * prev-first named sum both lerp arms: s=prev_abs; s+=na; t=prev/s or na/s
 * flds [esp+0x34] like orig. y-then-z, dest=edi, bits@0x44 remain. */
// WIP-FUNCTION: LEGOLAND 0x0041f050  (39.2%, no ESCAPES, 2-ret, dest +0x14, ebx=n, and ebx, frame 0x2c, plane ecx)
int Span_ClipPlane(int n, void* in_v, void* out_v, void** cursor, void* plane_v)
{
    void** in = (void**)in_v;
    void* dst;
    void** cur;
    int out_n = 0;
    int prev_sign;
    int cls;
    int abs_r;
    void* prev;
    void* nxt;
    union { float f; int i; } bits;
    struct { int i; int pad; } prev_abs;
    int left;

    /* cur-home first, then dest=*cur (not *cursor). n colours ebx before
     * dest is born — +1 vs dest-then-cur-home. */
    *(void** volatile*)&cur = cursor;
    dst = *cur;
    in[n] = in[0];
    nxt = in[0];
    {
        ClipPlane* plane = *(ClipPlane* volatile*)&plane_v;
        int* p = (int*)nxt;
        bits.f = (float)p[2] * plane->nx;
        bits.f += (float)p[1] * plane->ny;
        bits.f = plane->d - bits.f;
    }
    /* abs primary, sign is the copy (orig mov ebp / mov esi,ebp). */
    abs_r = bits.i;
    prev_sign = abs_r;
    abs_r &= 0x7fffffff;
    prev_sign &= 0x80000000;
    bits.i = abs_r;
    if (n >= 1) {
        cls = prev_sign;
        in++;
        left = n;
        do {
            prev_sign = cls;
            prev = nxt;
            *(volatile int*)&prev_abs.i = abs_r;
            nxt = *in;
            {
                union { float f; int i; } na;
                int next_sign;
                ClipPlane* plane = *(ClipPlane* volatile*)&plane_v;
                int* p = (int*)nxt;
                int destrel;

                cls = (prev_sign >> 1) & 0x40000000;
                /* destrel after fsubr (before fstp): dest live through
                 * the fild so and-ebx holds; ecx stays plane. */
                bits.f = (float)p[2] * plane->nx;
                bits.f += (float)p[1] * plane->ny;
                bits.f = plane->d - bits.f;
                destrel = (char*)dst - (char*)nxt;
                n = bits.i;
                next_sign = n;
                n &= 0x7fffffff;
                next_sign &= 0x80000000;
                bits.i = n;
                abs_r = n;
                cls |= next_sign;
                na.i = n;
                /* je ENTER / je BOTH / fallthrough LEAVE */
                if (cls != (int)0x80000000) {
                    if (cls != (int)0xc0000000) {
                        if (cls == (int)0x40000000) {
                            float s;
                            float t;
                            int k;
                            s = *(float*)&prev_abs.i;
                            s += na.f;
                            t = *(float*)&prev_abs.i / s;
                            k = 0;
                            *(void**)out_v = prev;
                            out_v = (char*)out_v + 4;
                            if ((int)g_span_vtx >= 0) {
                                int* src = (int*)prev;
                                /* reload homes each iter — orig sub ecx,eax + [esp+0x18] */
                                int dest_home = (char*)dst - (char*)prev;
                                int delta_home = (char*)nxt - (char*)prev;
                                do {
                                    int dest = dest_home;
                                    int delta = delta_home;
                                    int dword = src[0];
                                    {
                                        int dlt = *(int*)((char*)src + delta) - dword;
#ifndef LEGOLAND_PORTABLE
                                        *(int*)((char*)src + dest) = dword + (int)((float)dlt * t);
#else
                                        /* PORT-M5: 0x0041f17a is a call to the
                                         * ROUNDING helper 0x00458930. */
                                        *(int*)((char*)src + dest) = dword + LL_FISTP((float)dlt * t);
#endif
                                    }
                                    src++;
                                    k++;
                                } while (k <= (int)g_span_vtx);
                            }
                            *(void**)out_v = dst;
                            out_v = (char*)out_v + 4;
                            dst = (char*)dst + g_span_vtx_stride;
                            out_n += 2;
                        }
                    } else {
                        *(void**)out_v = prev;
                        out_v = (char*)out_v + 4;
                        out_n++;
                    }
                } else {
                    float s;
                    float t;
                    s = *(float*)&prev_abs.i;
                    s += na.f;
                    t = na.f / s;
                    {
                        int k;
                        if ((int)g_span_vtx >= 0) {
                            int* src = (int*)nxt;
                            int delta = (char*)prev - (char*)nxt;
                            int dest = destrel;
                            k = 0;
                            do {
                                int dword = src[0];
                                {
                                    int dlt = *(int*)((char*)src + delta) - dword;
#ifndef LEGOLAND_PORTABLE
                                    *(int*)((char*)src + dest) = dword + (int)((float)dlt * t);
#else
                                    /* PORT-M5: 0x0041f215, the same helper. */
                                    *(int*)((char*)src + dest) = dword + LL_FISTP((float)dlt * t);
#endif
                                }
                                src++;
                                k++;
                            } while (k <= (int)g_span_vtx);
                        }
                    }
                    *(void**)out_v = dst;
                    out_v = (char*)out_v + 4;
                    dst = (char*)dst + g_span_vtx_stride;
                    out_n++;
                }
            }
            in++;
        } while (--left);
    }
    *cur = dst;
    return out_n;
}
