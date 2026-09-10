/* LEGOLAND -- coaster entrance track, model loader and physics callbacks.
 * VC6 SP3 /O2 /Gy /Gd. Types are local; offsets describe the original ABI.
 */

typedef struct SpanRect { int top, left, bottom, right; } SpanRect;
typedef struct ClipPlane { float a, b, c; } ClipPlane;
typedef struct ClipSet { int count; ClipPlane p[4]; } ClipSet;
typedef struct Mat3 { float m[9]; } Mat3;
typedef struct Mat4 { float m[16]; } Mat4;
typedef struct ModelImage { void* data; int len; } ModelImage;
typedef struct PhysVec { int n; float v[20]; } PhysVec;
typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct TrackNode TrackNode;
typedef struct TrackClass TrackClass;
typedef struct RouteGeom RouteGeom;
typedef struct CoasterCar CoasterCar;
typedef struct CoasterRec CoasterRec;
typedef struct CoasterRoute CoasterRoute;
typedef struct TrackDesc {
    int raised;                /* +00: bit 0 = the run terminator */
    int h0;                    /* +04 */
    int h1;                    /* +08 */
    unsigned char pad0c[0x38 - 0x0c];
} TrackDesc;                   /* 0x38 */
typedef struct TrackJoint {
    int dir;                   /* +00: 1/2/4/8, -1 = free */
    int f04;
    TrackNode* node;           /* +08 */
} TrackJoint;                  /* 0x0c */
struct TrackNode {
    int state;                 /* +00 */
    short sx;                  /* +04 */
    short sy;                  /* +06 */
    TrackClass* cls;           /* +08 */
    TrackDesc* desc;           /* +0c */
    CoasterRec* owner;         /* +10 */
    TrackJoint jin;            /* +14 */
    TrackJoint jout;           /* +20 */
    unsigned char pad2c[0x50 - 0x2c];
};                             /* 0x50 */
/* schoolcar8.c's RoutePos: a position along the track, as a piece plus a
 * geometry link plus the world point. */
typedef struct RoutePos {
    TrackNode* node;           /* +00 */
    RouteGeom* geom;           /* +04 */
    Vec3f pos;                 /* +08 */
} RoutePos;                    /* 0x14 */
/* schoolcar8.c's Curve and its RouteGeom are ONE record: 0x00421ab0 fills
 * pos/dir/offset/length and leaves the parameter range [0,1], and +0x50 /
 * +0x54 are the forward and backward links that chain segments together. */
struct RouteGeom {
    Vec3f pos;                 /* +00 */
    Vec3f dir;                 /* +0c  = to - from */
    Vec3f offset;              /* +18 */
    unsigned char pad24[0x40 - 0x24];
    float length;              /* +40  |dir| */
    float t0;                  /* +44 */
    float t1;                  /* +48 */
    void* eval;                /* +4c */
    RouteGeom* next;           /* +50 */
    RouteGeom* prev;           /* +54 */
};                             /* 0x58 */
typedef struct Pos16 { short x, y; } Pos16;
/* One entrance-track piece template. The last 0x14 bytes are a coaster5.c
 * FootPart, and they are chained together into one footprint list. */
typedef struct FootPart {
    int x0, y0, x1, y1;        /* +00 */
    struct FootPart* next;     /* +10 */
} FootPart;                    /* 0x14 */
typedef struct EntranceRec {
    Pos16 sq;                  /* +00: map square */
    int geom;                  /* +04: which of the three curve segments */
    float t0;                  /* +08: parameter range along that segment */
    float t1;                  /* +0c */
    FootPart part;             /* +10 */
} EntranceRec;                 /* 0x24 */
struct CoasterCar {
    int state;                 /* +00: 1 = waiting to board */
    void* model;
    void* rider;
    void* bloke;
    CoasterCar* prev;          /* +10 */
    CoasterCar* next;          /* +14 */
    void* seat;                /* +18 */
    int wait_start;            /* +1c: game-timer stamp of entering state 1 */
    void* rest[2];
};
/* The coaster's STATION piece is embedded at +0x04, which is why
 * schoolcar8.c's Coaster_GetStationStart takes the record as a `short*` and
 * reads its map square at short index 4 (= TrackNode.sx at +0x08). */
struct CoasterRec {
    int f00;
    TrackNode station;         /* +04 */
    unsigned char pad54[0xe4 - 0x54];
    CoasterCar cars;           /* +e4: circular list sentinel */
};

/* One route record. schoolcar8.c calls the same object RoutePhysics when it
 * is seen through the solver: the PhysVec the solver integrates IS the
 * record's own {dimension, energy[2]} triple at +0x20, which is why
 * Route_InitPhysics stores `&rt->dimension` as the state pointer. */
struct CoasterRoute {
    int state;                 /* +00: bit 1 closed, bit 6 deadline armed */
    int pad04;
    int deadline;              /* +08: game-timer stamp */
    RoutePos here;             /* +0c: where the train is now */
    int dimension;             /* +20: the state vector's length, always 2 */
    float energy[2];           /* +24: [0] track parameter, [1] total energy */
    unsigned char pad2c[0x6c - 0x2c];
    CoasterRec* owner;         /* +6c */
    unsigned char pad70[4];    /* +70: the route node list head */
};
typedef struct PhysObj {
    void* setstate;            /* +00 */
    void* getstate;            /* +04 */
    void* deriv;               /* +08 */
    void* ops[10];             /* +0c */
    int dim;                   /* +34 */
    void* state;               /* +38 */
    CoasterRoute* route;       /* +3c */
} PhysObj;

extern int GetGameTimer(void);                                  /* 0x00499430 */

/* The clip set is a half-plane list tested as a*x + b*y >= c, so the right
 * and bottom edges are stored negated (-x >= -right, -y >= -bottom).
 * coastermath.c's SetSpanClip declares the second parameter as a bare
 * void*; here it needs the real record type. */
// FUNCTION: LEGOLAND 0x0041f380
void Span_SetClip(const SpanRect* rect, ClipSet* out)
{
    out->count = 4;
    out->p[0].a = 1.0f;
    out->p[0].b = 0.0f;
    out->p[0].c = (float)rect->left;
    out->p[1].a = -1.0f;
    out->p[1].b = 0.0f;
    out->p[1].c = -(float)rect->right;
    out->p[2].a = 0.0f;
    out->p[2].b = 1.0f;
    out->p[2].c = (float)rect->top;
    out->p[3].a = 0.0f;
    out->p[3].b = -1.0f;
    out->p[3].c = -(float)rect->bottom;
}

/* Mark the route closed, then arm or disarm its departure deadline. A zero
 * tick count clears bit 6 and leaves +0x08 alone. */
// FUNCTION: LEGOLAND 0x0041e4c0
void Route_SetDeadline(CoasterRoute* rt, int ticks)
{
    rt->state |= 2;
    if (ticks) {
        rt->state |= 0x40;
        rt->deadline = GetGameTimer() + ticks;
    } else {
        rt->state &= ~0x40;
    }
}

/* 0x004222f0 tests for a CRLF pair: (*(unsigned short*)p == 0x0a0d).
 * First named here; 0x00422300 above it copies one CRLF-terminated line. */
extern int ModelImage_IsEOL(const char* p);                     /* 0x004222f0 */

/* Records in a coaster model image are CRLF-terminated lines: walk the image
 * byte by byte, stepping two at a line break and counting it. The end bound
 * is one byte SHORT of the image so the CR test cannot read past it.
 * The `- 1` has to stay INSIDE the loop condition: as part of the end
 * initialiser VC6 folds the whole thing into one `lea ebx,[eax+esi-1]`,
 * where the original computes `data + len` and then `lea ebx,[eax-1]`. */
// FUNCTION: LEGOLAND 0x004223c0
int CountModelRecords(const ModelImage* img)
{
    const char* p = (const char*)img->data;
    const char* end = (const char*)img->data + img->len;
    int count = 0;

    while (p < end - 1) {
        if (ModelImage_IsEOL(p)) {
            count++;
            p += 2;
        } else {
            p++;
        }
    }
    return count;
}

/* Expand a 3x3 rotation into a 4x4: the source is read sequentially while
 * the destination is written down each column, so the 3x3 is stored
 * transposed relative to the 4x4's row-major layout. The bottom row and the
 * right column are zeroed and m[15] is 1.
 * The three trailing zero stores are emitted in SOURCE order here (11, 7, 3),
 * not reversed: all six permutations were measured and only the descending
 * one is exact. */
// FUNCTION: LEGOLAND 0x00426490
void Mat3_ToMat4(const Mat3* src, Mat4* dst)
{
    int i, j;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++)
            dst->m[j * 4 + i] = src->m[i * 3 + j];
        dst->m[12 + i] = 0.0f;
    }
    dst->m[11] = 0.0f;
    dst->m[7] = 0.0f;
    dst->m[3] = 0.0f;
    dst->m[15] = 1.0f;
}

/* Longest time any car has been sitting in the boarding state. The list is
 * circular through the record's embedded sentinel; the ebx push sinks past
 * the empty-list test because the walk lives in the guarded block. */
// FUNCTION: LEGOLAND 0x00424bc0
int Coaster_GetLongestWait(CoasterRec* rec)
{
    int longest = 0;
    int now = GetGameTimer();
    CoasterCar* car = rec->cars.next;

    while (car != &rec->cars) {
        if (car->state == 1) {
            int waited = now - car->wait_start;
            if (waited > longest)
                longest = waited;
        }
        car = car->next;
    }
    return longest;
}

extern int TrackJointSloped(TrackDesc* desc, int hin, int hout); /* 0x00429910 */

/* The mirror of coaster5.c's TrackRunSteps (0x00429990): identical body,
 * walking the jin side (+0x1c) instead of the jout side (+0x28). Counts the
 * sloped joints from `node` back to the first terminator piece and hands the
 * terminator back through the out-parameter. */
// FUNCTION: LEGOLAND 0x00429940
int TrackRunStepsBack(TrackNode* node, TrackNode** endOut)
{
    int steps = 0;

    while (!(node->desc->raised & 1)) {
        if (TrackJointSloped(node->desc, node->jin.dir, node->jout.dir))
            steps++;
        node = node->jin.node;
    }
    *endOut = node;
    return steps;
}

/* 0x0041dae0 sums 0x0041e810 over every node of the route's +0x70 list --
 * the train's POTENTIAL energy. 0x0041db90 hands back the train's total mass
 * and the lift motor's power. 0x0041dd00 returns how far the train travels
 * in one tick at a given speed. 0x00426a90 is the module's float sqrt, a
 * wrapper over the g_fast_sqrt hook at 0x00829a58. All first named here. */
extern float Route_SumPotentialEnergy(CoasterRoute* rt);        /* 0x0041dae0 */
extern void Route_GetMassAndPower(CoasterRoute* rt, float* mass,
                                  float* power);                /* 0x0041db90 */
extern float Route_TravelPerTick(CoasterRoute* rt, float speed); /* 0x0041dd00 */
extern float VecMath_Sqrt(float value);                         /* 0x00426a90 */

/* The route solver's derivative callback. Two levers: `2.0f * x` comes out as
 * the original's `fadd st(0),st(0)`, and 0.05 stays a DOUBLE (`fcomp qword`)
 * where the two float thresholds are dword-pooled.
 * the state vector is
 * {track parameter, total energy} and this returns {speed, motor power}.
 * Speed comes straight out of energy conservation, v = sqrt(2*(E - PE)/m),
 * clamped at zero rather than taking the root of a negative; the motor only
 * adds energy while the train is still travelling LESS than 0.05 units per
 * tick -- a launch/lift assist that cuts out once the train is up to speed.
 * The `t` parameter of the solver's derivative signature is unused here and
 * its argument slot is left alone. */
// FUNCTION: LEGOLAND 0x0041de10
void RoutePhys_EvaluateDerivative(PhysObj* obj, float t, PhysVec* out)
{
    CoasterRoute* rt = obj->route;
    float pe, mass, power, speed;

    out->n = 2;
    pe = Route_SumPotentialEnergy(rt);
    Route_GetMassAndPower(rt, &mass, &power);
    speed = 2.0f * (rt->energy[1] - pe) / mass;
    if (speed > 0.0f)
        speed = VecMath_Sqrt(speed);
    else
        speed = 0.0f;
    out->v[0] = speed;
    out->v[1] = 0.0f;
    if (power > 1.175494351e-38f && Route_TravelPerTick(rt, speed) < 0.05)
        out->v[1] = power;
}

/* 0x0041dd50 is the train's travel in one tick at its CURRENT speed
 * (0x0041dd00 over 0x0041dca0); 0x0041dd70 sums 0x0041e7e0 over the route's
 * node list, the mirror of the potential-energy sum and so the train's total
 * mass; 0x0042a1b0 measures the track distance from one position record to
 * another. All first named here. */
extern float Route_TravelThisTick(CoasterRoute* rt);            /* 0x0041dd50 */
extern float Route_SumMass(CoasterRoute* rt);                   /* 0x0041dd70 */
#ifndef LEGOLAND_PORTABLE
extern float Track_MeasureDistance(RoutePos* from, float t, RoutePos* to,
                                   float t2, int a, int b);     /* 0x0042a1b0 */
#else
/* coaster13.c defines the last parameter `float offset`. On x86 `push 0` is
 * the same dword either way (0 and 0.0f share a bit pattern), so the `int`
 * spelling above costs nothing; on wasm32 it is a different function type.
 * The single call site below passes the literal 0, which this prototype
 * converts to 0.0f -- the same bits the original pushed. */
extern float Track_MeasureDistance(RoutePos* from, float t, RoutePos* to,
                                   float t2, int a, float b);   /* 0x0042a1b0 */
#endif
extern void MapSquareToWorld(const short* square, float height,
                             Vec3f* out);                       /* 0x00425cb0 */
/* schoolcar8.c declares this `unsigned char g_station_geometry[]`; it is
 * the THIRD castle curve segment, and Castle_InitEntranceTrack below both
 * builds it and links it. The array spelling is load-bearing -- as a
 * pointer variable the `push OFFSET` becomes a load. */
extern RouteGeom g_station_geometry[];                          /* 0x006103a8 */
extern int g_station_height;                                    /* 0x004b5b50 */

/* The STATION brake, installed over the free-running derivative while a
 * train is pulling in: run the ordinary derivative first, then overwrite the
 * energy rate with the constant deceleration that stops the train exactly at
 * the station. With v the travel this tick and d the remaining track
 * distance to the station square, a = v*v / (2*d) and dE/dt = -m*a*v.
 * The `1, 0` tail of the distance call is 0x0042a1b0's direction/flag pair.
 *
 * LEVER: the negated quotient must be its OWN block-scoped local, not a
 * re-assignment of `vv`. Re-assigned, VC6 keeps the whole chain on the x87
 * stack, sinks `v*v` past the call and flushes `add esp,0x28` before the
 * first fmul (49 of 57); the extra IR temporary forces `vv`'s memory home and
 * puts `fmul [vv]` back before the cleanup -- 57/57. A free volatile read of
 * `v` at the last multiply gets 56 and no further. */
// FUNCTION: LEGOLAND 0x004248b0
void Coaster_StationDerivative(PhysObj* obj, float t, PhysVec* out)
{
    CoasterRoute* rt = obj->route;
    CoasterRec* rec = rt->owner;
    RoutePos stop;
    float v, vv, d;

    RoutePhys_EvaluateDerivative(obj, t, out);
    v = Route_TravelThisTick(rt);
    MapSquareToWorld(&rec->station.sx, (float)g_station_height, &stop.pos);
    vv = v * v;
    stop.node = &rec->station;
    stop.geom = g_station_geometry;
    d = Track_MeasureDistance(&stop, 0.1f, &rt->here, rt->energy[0], 1, 0) *
        0.0015875f;
    {
        float brake = -(vv / (d + d));
        out->v[1] = Route_SumMass(rt) * brake * v;
    }
}

__declspec(dllimport) int __stdcall CreateFileA(const char* name,
                                                unsigned int access,
                                                unsigned int share, void* sa,
                                                unsigned int disp,
                                                unsigned int flags,
                                                void* tmpl);            /* [0x4ab258] */
__declspec(dllimport) int __stdcall GetFileSize(int h, unsigned int* hi); /* [0x4ab25c] */
__declspec(dllimport) int __stdcall CloseHandle(int h);                  /* [0x4ab260] */
__declspec(dllimport) int __stdcall ReadFile(int h, void* buf, unsigned int n,
                                             unsigned int* got, void* ov); /* [0x4ab264] */

/* 0x00420530 is a null-checked SetCurrentDirectoryA wrapper (schoolcar.c
 * calls it Sub_420530 and passes exactly these two strings). */
extern void SetWorkingDirectory(const char* path);              /* 0x00420530 */
extern void* AllocZeroed(unsigned int size, int a, void* tag, int c); /* 0x004775b0 */
extern void Free_w(void* p);                                    /* 0x004775d0 */
extern char g_alloc_tag[];                                      /* 0x004d8bb0 */

/* Read one coaster model file whole, from the module's own data directory.
 * The body is schoolcar5.c's LoadWholeFile (0x00422470) with the chdir pair
 * wrapped round it -- every one of the four exits restores the directory, so
 * the "..\..\.." call is written out FOUR times and none of them merge.
 *
 * schoolcar8.c declares the second parameter `int mode`; it is really an
 * OPTIONAL `unsigned int*` byte-length out-pointer, and the null test at the
 * end is what makes CoasterModel_LoadLTX's literal 0 legal. The divergence
 * is left in place -- do not align schoolcar8.c's extern. */
// FUNCTION: LEGOLAND 0x00420550
void* CoasterModel_LoadFile(const char* name, unsigned int* len)
{
    int          h;
    void*        p;
    unsigned int n;
    unsigned int got;

    if (!name)
        return 0;
    SetWorkingDirectory("RollerCoaster\\RollerCoaster\\CreatedData");
    h = CreateFileA(name, 0x80000000, 1, 0, 3, 0x8000000, 0);
    if (h == -1) {
        SetWorkingDirectory("..\\..\\..");
        return 0;
    }
    n = GetFileSize(h, 0);
    p = AllocZeroed(n, 0, g_alloc_tag, 0);
    if (!p) {
        CloseHandle(h);
        SetWorkingDirectory("..\\..\\..");
        return 0;
    }
    ReadFile(h, p, n, &got, 0);
    if (got != n) {
        Free_w(p);
        CloseHandle(h);
        SetWorkingDirectory("..\\..\\..");
        return 0;
    }
    CloseHandle(h);
    SetWorkingDirectory("..\\..\\..");
    if (len)
        *len = n;
    return p;
}

/* 0x00421ab0 builds a STRAIGHT segment: dir = to - from, length = |dir|,
 * pos = from, offset = the sideways displacement, and the parameter range is
 * left at [0, 1]. 0x00421ce0 is its CORNER counterpart, taking three points
 * and two shape constants. Both first named here. */
extern void Curve_InitLine(RouteGeom* curve, const Vec3f* from,
                           const Vec3f* to, const Vec3f* offset); /* 0x00421ab0 */
extern void Curve_InitCorner(const Vec3f* a, const Vec3f* b, const Vec3f* c,
                             RouteGeom* curve, float k0, float k1); /* 0x00421ce0 */
extern void Castle_GetFirstCorner(const Pos16* square, Pos16* out);  /* 0x004239b0 */
extern void Castle_GetSecondCorner(const Pos16* square, Pos16* out); /* 0x004239e0 */

extern EntranceRec g_entrance[];                                /* 0x0060f928 */
extern int g_entrance_count;                                    /* 0x00610a08 */
extern RouteGeom g_castle_curve_a;                              /* 0x006102f8 */
extern RouteGeom g_castle_curve_b;                              /* 0x00610350 */

/* Build the castle's ENTRANCE TRACK: an L of three curve segments -- a
 * straight run down in y, a corner, and a straight run along x -- chained
 * through their +0x50/+0x54 links, plus a table of one 0x24-byte piece
 * template per map square, each carrying the slice of its segment's [0,1]
 * parameter range that it covers. The templates' footprints are finally
 * threaded into one FootPart list, terminated on the last entry.
 *
 * Castle_GetFirstCorner/GetSecondCorner both read the FootPart rect at +0x3c
 * of `g_castle_def` (0x00829bf8, the CASTLE OBJ's ObjDef) -- four ints, of
 * which only the low words are used: the first corner is
 * (x1 + sq.x, y0 + sq.y - 2) and the second (x0 + sq.x - 2, y1 + sq.y).
 * Called here with a {0,0} square, so the entrance track is laid out in the
 * castle's own footprint coordinates.
 *
 * THREE LEVERS, each measured here:
 *  * `i = 0;` as an explicit statement at the TOP of the function, with the
 *    first loop written `for (; i < n; i++)`. The running index is what VC6
 *    coalesces its SECOND zero register with (`mov [off.y], esi` and
 *    `mov [off.z], esi` are stores of `i`), so without the early definition
 *    the function runs on one zero register and comes out an instruction
 *    short: 170 of 230 against 187 of 231.
 *  * `sq = c2;` must sit BETWEEN `n = ...` and `step = ...`. Written after
 *    `step`, VC6 reuses `sq`'s escaped frame slot for the `fidiv` spill of
 *    `n` instead of sharing `step`'s slot, and the whole frame shifts (197
 *    -> 201).
 *  * The six curve-chain stores go BEFORE `sq = corner;`, not after. With the
 *    copy first, ebx (holding `corner`) frees early, VC6 takes it as the
 *    zero register instead of a scratch edx, and every register from there to
 *    the end of the body is one position off -- 201 of 231 against 231/231.
 *    prev-then-next within each pair is also load-bearing (227 the other way).
 *  * `cy2` is a `short`: as an `int` VC6 sign-extends `c1.y` eagerly at the
 *    definition (`movsx edi,ax`) and the later `movsx ecx,di` disappears, one
 *    instruction short. */
// FUNCTION: LEGOLAND 0x00423a10
void Castle_InitEntranceTrack(void)
{
    Pos16 sq;
    Pos16 sq2;
    Pos16 origin;
    Pos16 c2;
    Pos16 c1;
    Pos16 corner;
    Vec3f off;
    Vec3f b;
    Vec3f a;
    Vec3f c;
    short cy2;
    int n, i, j, k;
    float t, step;

    i = 0;
    origin.x = 0;
    origin.y = 0;
    Castle_GetSecondCorner(&origin, &c2);
    Castle_GetFirstCorner(&origin, &c1);

    corner.x = c2.x;
    sq.x = (short)(c2.x + 1);
    sq2.x = (short)(c2.x + 1);
    corner.y = c1.y;
    sq.y = (short)(c2.y + 2);
    cy2 = c1.y + 2;
    sq2.y = (short)cy2;
    MapSquareToWorld(&sq.x, 0.0f, &a);
    MapSquareToWorld(&sq2.x, 0.0f, &b);
    off.x = -10.0f;
    off.y = 0.0f;
    off.z = 0.0f;
    Curve_InitLine(&g_castle_curve_a, &a, &b, &off);

    n = (c2.y - corner.y) >> 1;
    sq = c2;
    t = g_castle_curve_a.t0;
    step = (g_castle_curve_a.t1 - g_castle_curve_a.t0) / n;
    for (; i < n; i++) {
        g_entrance[i].sq = sq;
        sq.y = (short)(sq.y - 2);
        g_entrance[i].geom = 0;
        g_entrance[i].t0 = t;
        t += step;
        g_entrance[i].t1 = t;
    }

    sq.x = -1;
    sq.y = 0;
    MapSquareToWorld(&sq.x, 0.0f, &a);
    sq.x = 0;
    sq.y = -1;
    MapSquareToWorld(&sq.x, 0.0f, &b);
    sq.x = (short)(corner.x + 2);
    sq.y = (short)cy2;
    MapSquareToWorld(&sq.x, 0.0f, &c);
    Curve_InitCorner(&a, &b, &c, &g_castle_curve_b, 1.5f, 0.5f);

    g_entrance[i].sq = corner;
    g_entrance[i].geom = 1;
    g_entrance[i].t0 = g_castle_curve_b.t0;
    g_entrance[i].t1 = g_castle_curve_b.t1;
    i++;

    sq.x = (short)(corner.x + 2);
    sq.y = (short)(corner.y + 1);
    sq2.x = (short)(c1.x + 2);
    sq2.y = (short)(c1.y + 1);
    MapSquareToWorld(&sq.x, 0.0f, &a);
    MapSquareToWorld(&sq2.x, 0.0f, &b);
    off.x = 0.0f;
    off.y = -10.0f;
    off.z = 0.0f;
    Curve_InitLine(g_station_geometry, &a, &b, &off);

    n = (c1.x - corner.x) >> 1;
    t = g_station_geometry[0].t0;
    step = (g_station_geometry[0].t1 - g_station_geometry[0].t0) / n;
    g_castle_curve_a.prev = 0;
    g_castle_curve_a.next = &g_castle_curve_b;
    g_castle_curve_b.prev = &g_castle_curve_a;
    g_castle_curve_b.next = g_station_geometry;
    g_station_geometry[0].prev = &g_castle_curve_b;
    g_station_geometry[0].next = 0;
    sq = corner;
    sq.x = (short)(corner.x + 2);
    for (j = 0; j < n; j++) {
        g_entrance[i].sq = sq;
        sq.x = (short)(sq.x + 2);
        g_entrance[i].geom = 2;
        g_entrance[i].t0 = t;
        t += step;
        g_entrance[i].t1 = t;
        i++;
    }

    for (k = 0; k < i; k++) {
        g_entrance[k].part.x0 = g_entrance[k].sq.x;
        g_entrance[k].part.y0 = g_entrance[k].sq.y;
        g_entrance[k].part.x1 = g_entrance[k].sq.x + 1;
        g_entrance[k].part.y1 = g_entrance[k].sq.y + 1;
        g_entrance[k].part.next = &g_entrance[k + 1].part;
    }
    g_entrance_count = i;
    /* Original bug, reproduced: the terminator is indexed off the LOOP
     * variable, so an empty table (i == 0) writes four bytes in front of
     * g_entrance instead of leaving it alone. */
    g_entrance[k - 1].part.next = 0;
}
