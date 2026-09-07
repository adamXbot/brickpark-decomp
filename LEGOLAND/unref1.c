/* LEGOLAND -- scope LL9: UNREFERENCED (dead) functions, 0x00401e00..0x004207b0.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere) and are copies of the shapes coaster.c, coaster8.c,
 * coastertiny.c, posstep.c, mappath.c, logflume*.c and schoolcar*.c already
 * established.
 *
 * tools/inventory.py classifies every function in this file as DEAD: nothing
 * live in the binary calls, tail-jumps to or takes the address of it.  The
 * game shipped without /OPT:REF, so the linker kept them.  They are ordinary
 * C from the same translation units as their neighbours; several call each
 * other, which is why some of them have a `callers` column at all.
 *
 * Notes, mechanics and levers: docs/lanes/scope-ll9.md.
 * ========================================================================= */

/* ---------------------------------------------------------------- types -- */

typedef struct Pos { int x, y; } Pos;
typedef struct Vec3f { float x, y, z; } Vec3f;

/* A packed 2-byte map square (posstep.c). */
typedef struct BPos  { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

typedef struct LFPiece LFPiece;
typedef struct LFRun   LFRun;

/* A PLACED PIECE of log flume (0x38 bytes; logflume2.c owns the record). */
struct LFPiece {
    LFPiece*      next;         /* +0x00  next piece of the same run */
    LFPiece*      prev;         /* +0x04 */
    LFPiece*      fwd;          /* +0x08  next piece ALONG THE ROUTE */
    LFPiece*      back;         /* +0x0c  previous piece along the route */
    unsigned int  flags;        /* +0x10 */
    BPosW         sq;           /* +0x14  the piece's map square */
    unsigned char pad16[2];
    int           kind;         /* +0x18 */
    int           dir;          /* +0x1c */
    void*         def;          /* +0x20 */
    LFRun*        run;          /* +0x24 */
    int           f28;          /* +0x28 */
    LFPiece*      sub;          /* +0x2c  head of this piece's sub-list */
    LFPiece*      end_a;        /* +0x30 */
    LFPiece*      end_b;        /* +0x34 */
};

/* One boat: 0x24 bytes, four of them from LFRun +0x40 on (logflume4.c). */
typedef struct LFBoat {
    int           state;        /* +0x00 */
    unsigned int  flags;        /* +0x04  bit 0 = waiting at the station */
    struct RiderNode* rider;    /* +0x08  who is aboard */
    Pos           pos;          /* +0x0c  the quadrant point it entered at */
    LFPiece*      piece;        /* +0x14  the route piece it is over */
    float         z;            /* +0x18  how far along that piece, 0..1 */
    int           f1c;          /* +0x1c */
    float         speed;        /* +0x20  fraction of a piece per step */
} LFBoat;

/* mappath.c's expanded walk path. */
typedef struct WalkNode { Pos pos; int state; } WalkNode;
typedef struct WalkPath { int count; WalkNode* nodes; } WalkPath;

/* coaster.c's class descriptor, only as far as this file reaches it. */
typedef struct TrackNode TrackNode;
typedef struct TrackDesc {
    int   raised;                        /* +0x00 */
    int   h0;                            /* +0x04 */
    int   h1;                            /* +0x08 */
    int   jp0[2];                        /* +0x0c */
    int   jp1[2];                        /* +0x14 */
    void (*draw)(TrackNode*);            /* +0x1c */
    void (*build)(TrackNode*, int);      /* +0x20 */
    void* query;                         /* +0x24 */
    void (*place)(TrackNode*);           /* +0x28 */
    void (*remove)(TrackNode*);          /* +0x2c */
    int   carries_path;                  /* +0x30 */
    int   f34;                           /* +0x34 */
} TrackDesc;

/* coaster.c's 0x50-byte track piece, as far as this file reaches it. */
typedef struct TrackJoint { int height; int f04; TrackNode* node; } TrackJoint;
struct TrackNode {
    int           state;        /* +0x00 */
    BPosW         sq;           /* +0x04 */
    unsigned char pad06[2];
    void*         cls;          /* +0x08 */
    TrackDesc*    desc;         /* +0x0c */
    void*         owner;        /* +0x10 */
    TrackJoint    jin;          /* +0x14 */
    TrackJoint    jout;         /* +0x20 */
    unsigned char part[0x50 - 0x2c];  /* +0x2c */
};

/* -------------------------------------------------------------- globals -- */

extern void* g_coaster_colours;                  /* 0x004d8bac */

/* -------------------------------------------------------------- callees -- */

extern void Free_w(void*);                       /* 0x004775d0 */

double log(double);
#pragma intrinsic(log)

/* =========================================================================
 * THE BOATING SCHOOL'S IN-EXE LIBRARY ENTRY POINT
 * =========================================================================
 * loaders.c recovered the object-library mechanism: a class whose ODF sets
 * OC_USEDLL loads ".\dlls\<name>.dll", whose start-up code registers its
 * GetInterfaces into the scratch record g_objlib_cur points at (+0x0c).
 * This is that start-up code for the BOATING SCHOOL family, compiled into
 * the exe alongside the GetInterface (0x0041b150) it registers -- a DllMain
 * that never runs because the classes are built in.
 * ========================================================================= */

typedef struct LLElem   LLElem;
typedef struct IfaceTable IfaceTable;
typedef struct ObjLib {
    struct ObjLib* next;                                  /* +0x00 */
    void*          handle;                                /* +0x04 */
    int            refcount;                              /* +0x08 */
    void         (*get_interfaces)(LLElem*, IfaceTable*); /* +0x0c */
} ObjLib;

extern ObjLib* g_objlib_cur;                                    /* 0x007fd620 */
extern void    GetInterface(LLElem* elem, IfaceTable* t);       /* 0x0041b150 */

// FUNCTION: LEGOLAND 0x0041b130
int __stdcall BoatingSchoolLibMain(void* module, unsigned long reason,
                                   void* reserved)
{
    (void)module; (void)reserved;
    switch (reason) {
    case 1:
        g_objlib_cur->get_interfaces = GetInterface;
        break;
    }
    return 1;
}

/* =========================================================================
 * ONE-BYTE STUBS
 * =========================================================================
 * Three `ret`-only bodies with no surviving call site: an empty void
 * function each.  Nothing in the disassembly says what they were for, so
 * they carry the brief's fallback Unref_<VA> names.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00411e20
void Unref_00411e20(void) {}

// FUNCTION: LEGOLAND 0x0041ef10
void Unref_0041ef10(void) {}

// FUNCTION: LEGOLAND 0x00420520
void Unref_00420520(void) {}

/* =========================================================================
 * SMALL LEAVES
 * ========================================================================= */

/* The counterpart of GetCoasterColours (0x004207c0, coastertiny.c): release
 * the coaster module's colour table. */
// FUNCTION: LEGOLAND 0x004207b0
void FreeCoasterColours(void)
{
    Free_w(g_coaster_colours);
}

/* Release a difference table built by BuildDifferenceTable: the pointer
 * array and its triangular data are one allocation. */
// FUNCTION: LEGOLAND 0x0041f710
void DiffTable_Free(void* samples)
{
    Free_w(samples);
}

/* Run a track piece's class-level PLACE handler (TrackDesc +0x28). */
// FUNCTION: LEGOLAND 0x0041d040
void TrackNode_ClassPlace(TrackNode* node)
{
    node->desc->place(node);
}

/* Run a track piece's class-level REMOVE handler (TrackDesc +0x2c). */
// FUNCTION: LEGOLAND 0x0041d050
void TrackNode_ClassRemove(TrackNode* node)
{
    node->desc->remove(node);
}

/* Is this boat's current route piece exactly `piece`?  The live predicate
 * LFBoat_IsOnPiece (0x0040b210, posstep.c) also answers yes for the
 * neighbour piece; this one is the strict test. */
// FUNCTION: LEGOLAND 0x0040b270
int LFBoat_IsAtPiece(LFBoat* boat, LFPiece* piece)
{
    return boat->piece == piece;
}

/* The index-th expanded step of a walk path, as an 8-byte Pos in eax:edx. */
// FUNCTION: LEGOLAND 0x004120e0
Pos WalkPath_GetPoint(WalkPath* path, int index)
{
    return path->nodes[index].pos;
}

/* The natural logarithm as a scalar `float -> double` sample function (VC6's
 * intrinsic `log` is fldln2/fyl2x).  Its only reference is the function
 * pointer MathSelfTest (0x0041f7f0) hands to NumericDerivative. */
// FUNCTION: LEGOLAND 0x0041f7e0
float TestFn_Log(float x)
{
    return (float)log(x);
}

/* =========================================================================
 * THE ROUTE'S SEAT TABLE  (the dead half of coaster8.c's mechanism)
 * =========================================================================
 * coaster8.c recovered the live half: a RouteNode is 0xec bytes with two
 * 0x20-byte seats inline at +0x78/+0x98 and its ring links at +0xe4/+0xe8,
 * and a seat carries FOUR inline method slots at +0x10..+0x1c
 * (occupied / attach / detach / update).  These four dead bodies are the
 * rest of that interface: seat one car, take one car back, empty a node,
 * and the two ring-wide wrappers built on the first two.
 * ========================================================================= */

typedef struct RouteSeat RouteSeat;
typedef struct CoasterCar CoasterCar;
struct RouteSeat {
    Vec3f       pos;                          /* +0x00 */
    CoasterCar* car;                          /* +0x0c */
    int (*occupied)(RouteSeat*);              /* +0x10 */
    void (*attach)(RouteSeat*, CoasterCar*);  /* +0x14 */
    void (*detach)(RouteSeat*);               /* +0x18 */
    void (*update)(RouteSeat*);               /* +0x1c */
};                                            /* 0x20 */

typedef struct RouteNodeSeats {
    int           flags, kind;
    unsigned char pad08[0x78 - 0x08];
    RouteSeat     seat[2];                    /* +0x78, +0x98 */
    unsigned char padb8[0xe4 - 0xb8];
    struct RouteNodeSeats* prev;              /* +0xe4 */
    struct RouteNodeSeats* next;              /* +0xe8 */
} RouteNodeSeats;                             /* 0xec */

/* schoolcar7.c / coaster8.c's CoasterRoute, only as far as this file goes. */
typedef struct CoasterRoute {
    unsigned char pad00[0x70];
    RouteNodeSeats head;                      /* +0x70  embedded ring sentinel */
} CoasterRoute;

/* coastertiny.c's 0x004273e0: clear the seat's car slot and hand it back. */
extern CoasterCar* RouteSeat_TakeCar(RouteSeat* seat);          /* 0x004273e0 */

/* Put `car` in this node's first empty seat.  The counterpart of
 * RouteNode_FindFreeSeat (0x0041e760, coaster8.c), which returns the seat
 * instead of filling it. */
// FUNCTION: LEGOLAND 0x0041e720
int RouteNode_SeatCar(RouteNodeSeats* node, CoasterCar* car)
{
    int i;
    RouteSeat* seat = &node->seat[0];
    for (i = 0; i <= 1; i++, seat++) {
        if (!seat->occupied(seat)) {
            seat->attach(seat, car);
            return 1;
        }
    }
    return 0;
}

/* Take the car out of this node's first OCCUPIED seat.  The failure arm's
 * `return 0` costs no `xor eax,eax`: the loop can only fall out with the
 * last `occupied()` result -- zero -- still in eax. */
// FUNCTION: LEGOLAND 0x0041e790
CoasterCar* RouteNode_UnseatCar(RouteNodeSeats* node)
{
    int i;
    RouteSeat* seat = &node->seat[0];
    for (i = 0; i <= 1; i++, seat++) {
        if (seat->occupied(seat))
            return RouteSeat_TakeCar(seat);
    }
    return 0;
}

/* Empty both of a node's seats through their detach slots. */
// FUNCTION: LEGOLAND 0x0041e7c0
void RouteNode_ClearSeats(RouteNodeSeats* node)
{
    int i = 2;
    RouteSeat* seat = &node->seat[0];
    do {
        seat->detach(seat);
        seat++;
    } while (--i);
}

/* Seat `car` in the first node of the route's ring that has room. */
// FUNCTION: LEGOLAND 0x0041e260
int Route_SeatCar(CoasterRoute* route, CoasterCar* car)
{
    RouteNodeSeats* node = &route->head;
    do {
        if (RouteNode_SeatCar(node, car))
            return 1;
        node = node->next;
    } while (node != &route->head);
    return 0;
}

/* Take back the first car seated anywhere on the route's ring. */
// FUNCTION: LEGOLAND 0x0041e2f0
CoasterCar* Route_UnseatCar(CoasterRoute* route)
{
    RouteNodeSeats* node = &route->head;
    CoasterCar* car;
    do {
        car = RouteNode_UnseatCar(node);
        if (car)
            return car;
        node = node->next;
    } while (node != &route->head);
    return 0;
}

/* =========================================================================
 * THE COPTERS' SAVE-INDEX INVERSE
 * =========================================================================
 * ridetiny.c's Copters_StepRider (0x00403d30) replaces a rider's animation
 * path POINTER with its ordinal in the five-entry table at 0x004c1124 (or
 * -1) so the ordinal can go into a save file.  This is the load-side twin,
 * and like its live partner it walks SIX slots of a five-entry table.
 * ========================================================================= */

typedef struct Bloke {
    unsigned char pad00[0x50];
    void*         path;         /* +0x50 */
    unsigned char pad54[0x60 - 0x54];
    unsigned char action;       /* +0x60  the bloke's script step */
} Bloke;

typedef struct RiderNode {
    struct RiderNode* next;     /* +0x00 */
    struct RiderNode* prev;     /* +0x04 */
    Bloke*            bloke;    /* +0x08 */
} RiderNode;

extern void* g_copters_paths[6];                 /* 0x004c1124 */

/* Turn a saved path ordinal back into the path pointer; an out-of-range
 * ordinal (which is what Copters_StepRider's -1 miss produces) clears the
 * path instead. */
// FUNCTION: LEGOLAND 0x00403d60
void Copters_RestoreRider(RiderNode* rider)
{
    Bloke* bloke = rider->bloke;
    int index = (int)bloke->path;

    if (index >= 0 && index < 6)
        bloke->path = g_copters_paths[index];
    else
        bloke->path = 0;
}

/* =========================================================================
 * THE LOG FLUME'S BOARDING QUEUE
 * ========================================================================= */

typedef struct LFQueueNode {
    struct LFQueueNode* next;   /* +0x00 */
    RiderNode*          rider;  /* +0x04 */
} LFQueueNode;

typedef struct LFQueue {
    void*        path;          /* +0x00 the capacity reference */
    LFQueueNode* head;          /* +0x04 */
} LFQueue;

/* Is `bloke` the rider at the FRONT of the boarding queue -- i.e. the one
 * whose turn it is to take the next boat?  An empty queue answers no. */
// FUNCTION: LEGOLAND 0x00411f70
int LFQueue_IsFrontRider(LFQueue* q, Bloke* bloke)
{
    LFQueueNode* n = q->head;

    if (n && n->rider->bloke == bloke)
        return 1;
    return 0;
}

/* =========================================================================
 * THE OPEN ROUTE'S END LENGTHS, PER PIECE
 * =========================================================================
 * schoolcar.c's GetOpenEndSteps (0x0041cf70) asks the same question of the
 * whole coaster record and answers 0/0 for a closed circuit.  This dead twin
 * asks it of ONE piece, through that piece's two joints, and its "no
 * neighbour" answer is -1 rather than 0.
 * ========================================================================= */

extern int Track_CountHeadPieces(TrackNode* n);  /* 0x0041cee0 */
extern int Track_CountTailPieces(TrackNode* n);  /* 0x0041cf00 */

// FUNCTION: LEGOLAND 0x0041cf20
void GetNodeEndSteps(TrackNode* node, int* tail_steps, int* head_steps)
{
    *head_steps = -1;
    *tail_steps = -1;
    if (node->jin.node)
        *head_steps = Track_CountHeadPieces(node->jin.node);
    if (node->jout.node)
        *tail_steps = Track_CountTailPieces(node->jout.node);
}

/* =========================================================================
 * THE NUMERICAL-METHODS CORNER  (0x0041f2b0..0x0041fa10)
 * =========================================================================
 * coaster7.c recovered the live half of this block -- Span_SetClip
 * (0x0041f380) and the physics plumbing.  The dead half is a small NUMERICAL
 * LIBRARY plus its own self test, all of it unreferenced:
 *
 *   BuildDifferenceTable   sample f at n+1 equally spaced points centred on
 *                          x and build the forward-difference triangle
 *   NumericDerivative      f'(x) from rows 1 and 3 of that triangle
 *   PhysVec_Derivative4    the same derivative for a VECTOR-valued function,
 *                          through the PhysOps hooks, on the four-point
 *                          stencil {-1, -1/3, +1/3, +1} with the weights
 *                          {1/16, -27/16, +27/16, -1/16}
 *   TestFn_Log / ExpDerivs the two sample functions the self test uses
 *   MathSelfTest           calls each of them once and throws the results
 *                          away -- a scratch `main`, left in the shipped exe
 *
 * The triangle is ONE allocation: (n+1) row pointers followed by
 * (n+1)(n+2)/2 floats, the rows shortening by one each time.
 * ========================================================================= */

typedef struct SpanRect { int top, left, bottom, right; } SpanRect;
typedef struct ClipPlane { float a, b, c; } ClipPlane;
typedef struct ClipSet { int count; ClipPlane p[4]; } ClipSet;
typedef struct PhysVec { int n; float v[20]; } PhysVec;         /* 0x54 */
typedef struct PhysOps {
    void  (*add)(PhysVec*, PhysVec*, PhysVec*);   /* +0x00 */
    void*  op1;                                   /* +0x04 */
    void*  op2;                                   /* +0x08 */
    void  (*scale)(PhysVec*, float);              /* +0x0c */
    void*  op4[4];                                /* +0x10 */
    PhysVec** (*alloc)(int);                      /* +0x20 */
    void  (*free)(PhysVec**, int);                /* +0x24 */
    int    dim;                                   /* +0x28 */
} PhysOps;                                        /* 0x2c */

/* The four-point stencil: sample offsets in units of h, and the matching
 * derivative weights.  Both are read as ONE array each; the first element of
 * each is peeled out of the loop, which is why the loop's base displacement
 * is the SECOND element. */
extern float g_deriv_offsets[4];                 /* 0x004b5614 */
extern float g_deriv_weights[4];                 /* 0x004b5624 */

extern void* AllocZeroed(unsigned int size, int a, int b, int c); /* 0x004775b0 */
/* The Sutherland-Hodgman clipper behind Span_SetClip's half-plane list. */
extern void* ClipPolygonPlanes(int count, void* verts, int* out_count,
                               int nplanes, const ClipPlane* planes); /* 0x0041f2b0 */
extern void  CarPoolInit(void);                                 /* 0x00421470 */
extern void  PhysVec_InitOps(PhysOps* out, int dimension);      /* 0x00421540 */
/* The vector derivative MathSelfTest actually calls: the difference-table
 * form (0x0041f3e0 builds the vector triangle), NOT the four-point stencil
 * PhysVec_Derivative4 below. */
extern int   PhysVec_DerivativeTable(void (*fn)(float, PhysVec*), PhysOps* ops,
                                     float x, float h, PhysVec* out); /* 0x0041f4e0 */

double exp(double);
#pragma intrinsic(exp)

/* Clip a polygon against a ClipSet, unpacking the set into the plane count
 * and the plane array the clipper actually takes. */
// FUNCTION: LEGOLAND 0x0041f350
void* Span_ClipPolygonToSet(int count, void* verts, int* out_count,
                            const ClipSet* clip)
{
    return ClipPolygonPlanes(count, verts, out_count, clip->count, clip->p);
}

/* Sample `f` at n+1 points spaced `h` apart and centred on `x`, then reduce
 * the samples in place into a forward-difference triangle:
 *     tab[0][i] = f(x + (i - n/2) * h)
 *     tab[i][j] = tab[i-1][j+1] - tab[i-1][j]
 * The pointer array and the triangle are one zeroed block. */
// FUNCTION: LEGOLAND 0x0041f650
float** BuildDifferenceTable(float (*f)(float), int n, float x, float h)
{
    float** tab;
    float*  p;
    int     m = n + 1;
    int     i, j, len;

    tab = (float**)AllocZeroed((((m + 1) * m >> 1) + m) * 4, 0, 0, 0);
    if (!tab)
        return tab;

    p = (float*)(tab + m);
    len = m;
    for (i = 0; i <= n; i++, len--) {
        tab[i] = p;
        p += len;
    }

    x -= (float)(n >> 1) * h;
    for (i = 0; i <= n; i++) {
        tab[0][i] = f(x);
        x += h;
    }

    len = n;
    for (i = 1; i <= n; i++) {
        for (j = 0; j < len; j++)
            tab[i][j] = tab[i - 1][j + 1] - tab[i - 1][j];
        len--;
    }
    return tab;
}

/* f'(x) from a five-point difference triangle:
 *     ((D1[2] + D1[1]) - (D3[1] + D3[0]) / 6) / (2 * (h/2))
 * A failed allocation returns 0 with the eax the table call left there. */
// FUNCTION: LEGOLAND 0x0041f720
int NumericDerivative(float (*f)(float), float x, float h, float* out)
{
    float** tab;
    float   r;

    h *= 0.5f;
    tab = BuildDifferenceTable(f, 4, x, h);
    if (!tab)
        return 0;
    r = ((tab[1][2] + tab[1][1])
         - (tab[3][1] + tab[3][0]) * 0.16666667f) * 0.5f / h;
    DiffTable_Free(tab);
    *out = r;
    return 1;
}

/* The self test's vector sample function: a three-component state whose
 * components are exp(x), 2*exp(x) and 2*exp(2x). */
// FUNCTION: LEGOLAND 0x0041f790
void ExpDerivs(float x, PhysVec* out)
{
    double a = exp(x);
    double b;

    out->n = 3;
    out->v[0] = (float)a;
    out->v[1] = (float)(a + a);
    b = exp(x + x);
    out->v[2] = (float)(b + b);
}

/* The numerical corner's self test: differentiate log at 3 with h = 1, then
 * differentiate the vector sample function at 3 with h = 0.01.  Every result
 * is discarded and every cdecl cleanup merges into one `add esp,0xb0`. */
// FUNCTION: LEGOLAND 0x0041f7f0
void MathSelfTest(void)
{
    float   d;
    PhysOps ops;
    PhysVec state;

    NumericDerivative(TestFn_Log, 3.0f, 1.0f, &d);
    CarPoolInit();
    PhysVec_InitOps(&ops, 3);
    PhysVec_DerivativeTable(ExpDerivs, &ops, 3.0f, 0.01f, &state);
}

/* =========================================================================
 * THE LOG FLUME'S DEAD LOOK-UPS
 * ========================================================================= */

typedef struct Footprint { int v[4]; void* parts; } Footprint;  /* 0x14 */
typedef struct TileBounds { int left, top, right, bottom; } TileBounds;

/* The full run record (0xd4 bytes; logflume4.c / lfentrance.c own it). */
struct LFRun {
    LFRun*        next;         /* +0x00 */
    unsigned int  flags;        /* +0x04  bit 0 = a boat is loading */
    LFPiece*      f08;          /* +0x08 */
    LFPiece*      f0c;          /* +0x0c */
    LFPiece*      pieces;       /* +0x10 */
    BPosW         sq;           /* +0x14 */
    unsigned char pad16[2];
    LFPiece*      f18;          /* +0x18 */
    int           frame;        /* +0x1c */
    unsigned char pad20[0x38 - 0x20];
    LFBoat*       boat;         /* +0x38 */
    int           boat_count;   /* +0x3c */
    LFBoat        boats[4];     /* +0x40 */
    int           piece_count;  /* +0xd0 */
};                              /* 0xd4 */

extern LFRun* g_lf_queue;                        /* 0x004cbe84 */
extern Footprint  g_lf_footprint;                /* 0x004b4728 */

extern void GetTileBounds(const Pos* tile, TileBounds* out);  /* 0x0045acc0 */

/* The flume piece whose one-cell footprint covers the map square (x, y).
 * posstep.c's live LFTrack_FindPiece compares the square exactly; this dead
 * twin accepts anything inside the flume cell's own span, so a piece drawn
 * larger than one square still answers for its whole extent. */
/* WIP: 40/50 strict, first divergence at index 1; the block layout, the two
 * zero registers, all four compares and both epilogues are index-for-index
 * right.  The whole residual is one register-allocation choice: the original
 * spills the RUN cursor into the single stack dword (`push ecx`) and keeps
 * both footprint spans in ebp/edi; VC6 gives us edi for `run` and spills the
 * y-span instead, which costs the two outer-loop reloads and adds one inner
 * reload.  Ruled out (identical 40): while/for/do-while spellings, reading
 * the head first or last, two-definition (`h = v3; h -= v1;`) spans, a
 * Footprint pointer local, function-scope cursors, a `volatile` run (48,
 * worse), split `continue` guards, swapping the two span initialisers, named
 * `v[0]`/`v[1]` locals, a guarded `if (run) do {...} while (run)`, and the
 * two spans as one two-element array (43, worse). */
// WIP-FUNCTION: LEGOLAND 0x00408f90  (50/50 insns, 40 strict mismatches; run vs h spill choice)
LFPiece* LFTrack_FindPieceCovering(int x, int y)
{
    LFRun* run = g_lf_queue;
    int    h = g_lf_footprint.v[3] - g_lf_footprint.v[1];
    int    w = g_lf_footprint.v[2] - g_lf_footprint.v[0];

    while (run) {
        LFPiece* p = run->pieces;
        while (p) {
            int px = p->sq.b.x;
            int py = p->sq.b.y;
            if (x >= px && x <= px + w && y >= py && y <= py + h)
                return p;
            p = p->next;
        }
        run = run->next;
    }
    return 0;
}

/* Where a screen row falls down a piece's own drawn height, as a shade band.
 * The class footprint's two corners are turned into tile bounds; `y` is
 * measured from the TOP corner's top edge against the BOTTOM corner's bottom
 * edge, scaled, clamped to 0..1 and mapped onto 0x20..0xe0 -- the game's
 * darkness ramp, which is why the multiplier is -192 and the base is 32. */
/* WIP: 68 of the original's 69 instructions, first divergence at index 3.
 * Everything from the second GetTileBounds on is right, including the frame
 * (one Pos + one TileBounds reused by both calls), the merged `add esp,0x10`,
 * the two spills into the dead arg-2 slot, the fild/fmul/fdivp order, both
 * clamp arms and the `0x20 - __ftol(r * -192)` tail.  The deficit is ONE
 * callee-saved push: the original holds seven values at once (fp, sq.x, sq.y
 * and all four corner sums) and so pushes ebx/ebp/esi/edi, putting x0 in edx
 * and y0 in edi; VC6 lets sq.x die before it loads v[1], needs only six, and
 * pushes three.  Ruled out (all 65-67): loading all four v[] before the adds,
 * every permutation of the four sum statements, named sq.x/sq.y locals, a
 * `const int* v = fp->v` cursor, storing the first point early or y-before-x,
 * and moving the `(void)unused`. */
// WIP-FUNCTION: LEGOLAND 0x0040adb0  (68/69 insns, 66 strict; one callee-saved push short)
int LFPiece_ShadeForRow(BPos sq, const Footprint* fp, void* unused,
                        int y, float scale)
{
    Pos        t;
    TileBounds tb;
    int        x0, x1, y0, y1, top, bottom;
    float      r;

    (void)unused;
    x0 = fp->v[0] + sq.x;
    x1 = fp->v[2] + sq.x;
    y0 = fp->v[1] + sq.y;
    y1 = fp->v[3] + sq.y;

    t.x = x0;
    t.y = y0;
    GetTileBounds(&t, &tb);
    top = tb.top;

    t.x = x1;
    t.y = y1;
    GetTileBounds(&t, &tb);
    bottom = tb.bottom;

    r = (float)(y - top) / ((float)(bottom - top + 1) * scale);
    if (r < 0.0f)
        r = 0.0f;
    else if (r > 1.0f)
        r = 1.0f;
    return 0x20 - (int)(r * -192.0f);
}

/* =========================================================================
 * TWO MORE DEAD FLUME BODIES
 * ========================================================================= */

/* 0x0040bab0 (not exported, not matched): may boat `idx` of this run move on
 * to the next route piece?  It compares this boat's position along its piece
 * with every other boat's, using the same 0.5-of-a-piece spacing the draw
 * code uses. */
extern int LFBoat_IsWayClear(LFRun* run, int idx);              /* 0x0040bab0 */

/* The dead twin of LFBoat_Step (0x0040bbb0): the same one-boat advance, but
 * with the STATION handling folded in.  A boat with the "moving" bit set
 * drops its rider when it reaches run->f0c (the exit piece: the rider's
 * script step is bumped and the seat cleared), parks with a 50-frame timer
 * when it reaches run->f08 (the station), and otherwise creeps forward one
 * piece whenever the way is clear.  A parked boat counts its timer down and
 * restarts -- unless a boat is already loading (run flag bit 0) -- resetting
 * the timer to 1 either way.
 *
 * The leading `if (boat)` is a null test on `&run->boats[idx]`, which can
 * never be null; the original tests it anyway and so do we. */
// FUNCTION: LEGOLAND 0x0040bd40
void LFBoat_StepAtStation(LFRun* run, int idx)
{
    LFBoat* boat = &run->boats[idx];

    if (!boat)
        return;
    if (boat->flags & 1) {
        if (boat->piece == run->f0c) {
            if (boat->rider) {
                Bloke* b = boat->rider->bloke;
                b->action++;
                boat->rider = 0;
            }
        }
        if (boat->piece == run->f08) {
            boat->state = 0x32;
            boat->flags &= ~1u;
            return;
        }
        if (boat->piece->fwd) {
            if (LFBoat_IsWayClear(run, idx))
                boat->piece = boat->piece->fwd;
        }
        return;
    }
    if (--boat->state < 0) {
        if (!(run->flags & 1)) {
            if (boat->piece->fwd) {
                if (LFBoat_IsWayClear(run, idx)) {
                    boat->flags |= 1;
                    boat->piece = boat->piece->fwd;
                }
            }
        }
        boat->state = 1;
    }
}

/* Move every boat standing on `p` (or on one of its sub-pieces) onto the
 * neighbouring piece, so `p` can be taken out of the route.
 *
 * ORIGINAL BUG, reproduced: when the piece has neither a previous nor a next
 * piece, `dest` is never assigned and the function reads it uninitialised.
 * VC6 homes it in the incoming argument's own stack slot, so in practice it
 * reads back `p` itself and every boat on `p` is "moved" onto `p`. */
/* WIP: 64/64 instructions and 143 bytes, first divergence at index 1, 57
 * strict.  The block layout is right (the two destination stores CROSS-JUMP
 * into one; the sub-list arm is exiled past the `mov eax,1` epilogue; ebx and
 * esi are pushed INSIDE the boat-loop path; the boat count is re-read from
 * the run local in the latch).  The residual is one allocation choice: the
 * original enregisters the PIECE in ebp and homes the destination cursor in
 * the parameter's own stack slot; VC6 does the reverse for us whichever way
 * the two are spelled.  Ruled out (57-63): a plain `dest` local left
 * uninitialised on the no-neighbour path, reassigning the parameter itself
 * (this form), a merged `n = prev; if (!n) n = next;` temporary, declaring
 * the copy before the run, and indexing `run->boats[i]` instead of walking a
 * cursor. */
// WIP-FUNCTION: LEGOLAND 0x0040c250  (64/64 insns, 57 strict; piece vs dest spill choice)
int LFPiece_MoveBoatsOff(LFPiece* p)
{
    LFPiece* piece = p;
    LFRun*   run = piece->run;
    LFBoat*  boat;
    int      i;

    if (piece->prev)
        p = piece->prev;
    else if (piece->next)
        p = piece->next;
    if (!p)
        return 0;
    if (p->sub)
        p = p->sub;

    boat = run->boats;
    for (i = 0; i < run->boat_count; i++, boat++) {
        LFPiece* s = piece->sub;
        if (!s) {
            if (LFBoat_IsAtPiece(boat, piece))
                boat->piece = p;
        } else {
            do {
                if (LFBoat_IsAtPiece(boat, s)) {
                    boat->piece = p;
                    break;
                }
                s = s->next;
            } while (s);
        }
    }
    return 1;
}

/* =========================================================================
 * THE DRIVING SCHOOL'S WAYPOINT PUSH
 * =========================================================================
 * The mirror image of schoolcar4.c's SchoolCarIdleStep (0x00401cd0): that
 * one shifts wp[1..16] DOWN over wp[0] and drops the count, this one shifts
 * wp[0..15] UP to make room at the front and raises it.  Both carry the same
 * one-past-the-end bug at their own end of the array -- here the first copy
 * WRITES wp[16], i.e. over the cached unit heading at +0xb0/+0xb4 -- and
 * both are written out as sixteen separate 8-byte struct assignments (a
 * counted loop compiles to 19 instructions; VC6 SP3 does not unroll it).
 * ========================================================================= */

typedef struct Waypoint { int x, y; } Waypoint;
typedef struct CarPos { int x, y; } CarPos;
typedef struct SchoolCar {
    struct SchoolCar* next;     /* +0x00 */
    unsigned short school;      /* +0x04 */
    unsigned char  pad06[2];
    int            sx, sy;      /* +0x08 */
    int            wx, wy;      /* +0x10 */
    CarPos         cur;         /* +0x18 */
    CarPos         start;       /* +0x20 */
    int            vx, vy;      /* +0x28 */
    Waypoint       wp[16];      /* +0x30 */
    float          ux, uy;      /* +0xb0 */
    unsigned char  frame;       /* +0xb8 */
    unsigned char  b9;          /* +0xb9 */
    unsigned char  turn;        /* +0xba */
    unsigned char  nwp;         /* +0xbb */
} SchoolCar;

// FUNCTION: LEGOLAND 0x00401e00
void SchoolCarPushWaypoint(SchoolCar* c)
{
    c->wp[16] = c->wp[15];      /* one past the end -- the original's bug */
    c->wp[15] = c->wp[14];
    c->wp[14] = c->wp[13];
    c->wp[13] = c->wp[12];
    c->wp[12] = c->wp[11];
    c->wp[11] = c->wp[10];
    c->wp[10] = c->wp[9];
    c->wp[9]  = c->wp[8];
    c->wp[8]  = c->wp[7];
    c->wp[7]  = c->wp[6];
    c->wp[6]  = c->wp[5];
    c->wp[5]  = c->wp[4];
    c->wp[4]  = c->wp[3];
    c->wp[3]  = c->wp[2];
    c->wp[2]  = c->wp[1];
    c->wp[1]  = c->wp[0];
    c->nwp++;
}

/* The vector four-point derivative.  Two scratch vectors are taken from the
 * ops pool but only the second is used: the caller's `out` doubles as the
 * accumulator, so the first sample is written straight into it and every
 * later sample is scaled in `work` and added on.  The stencil is
 * {-1, -1/3, +1/3, +1} in units of h/2 with the weights
 * {1/16, -27/16, +27/16, -1/16}; the whole sum is finally divided by h/2.
 * The first stencil point is peeled out of the loop, which is why the loop's
 * table displacements are the SECOND entry of each array. */
// FUNCTION: LEGOLAND 0x0041f5a0
int PhysVec_Derivative4(void (*fn)(float, PhysVec*), PhysOps* ops,
                        float x, float h, PhysVec* out)
{
    PhysVec** tmp;
    PhysVec*  work;
    int       i;

    h *= 0.5f;
    tmp = ops->alloc(2);
    work = tmp[1];
    fn(x + g_deriv_offsets[0] * h, out);
    ops->scale(out, g_deriv_weights[0]);
    for (i = 1; i <= 3; i++) {
        fn(x + g_deriv_offsets[i] * h, work);
        ops->scale(work, g_deriv_weights[i]);
        ops->add(work, out, out);
    }
    ops->scale(out, 1.0f / h);
    ops->free(tmp, 2);
    return 1;
}

/* =========================================================================
 * 0x0041fa10 -- THE SHADED, Z-BUFFERED SPAN FILLER
 * =========================================================================
 * The dead twin of schoolcar6.c's ZBuffer_FillPoly (0x00423350).  Same
 * two-chain scanline filler, same key/edge arrays, same pre-biased 16.16
 * chains, same half-pixel span test -- and the same hand-written assembly
 * region, on all three of schoolcar6.c's proofs: an EBP frame in an /O2
 * file, `xchg ebx,eax` for a register swap, `add ebx,1` where VC6 emits
 * `inc ebx`, and the `cmp ecx,8000h / jns wide / jmp done` pair where a
 * single `js done` would do.
 *
 * What this one adds over its live twin:
 *   * it PAINTS instead of clearing: the pixel value is one entry of one of
 *     the 0x400 shade ramps at 0x00829c60, picked by the ramp index
 *     (argument 1) and the shade level (`src[0]`);
 *   * the LEFT chain carries a second interpolant, a 16.16 depth, whose
 *     value and step live at +0x08 of the same 0x14-byte records; and
 *   * it writes BOTH targets: the colour goes to 0x004b5b20 (the global
 *     schoolcar6.c's ZBuffer_FillPoly copies and never reads -- so THIS is
 *     what it is: the 16-bit colour target's base) and the depth, taken from
 *     the left chain and stepped by `src[1]` per pixel, goes to the z-buffer
 *     at 0x004b5b24.  Both rows advance by `pitch * 2` bytes, which VC6
 *     hoists into the dead `n` argument slot.
 * ========================================================================= */

typedef struct ZKey { int y; int idx; } ZKey;   /* 8 */

typedef struct ZEdge {
    short         f00;          /* +0x00 */
    short         ylast;        /* +0x02 */
    int           side;         /* +0x04  non-zero = the right-hand chain */
    int           x;            /* +0x08  16.16 */
    int           z;            /* +0x0c  16.16 */
    unsigned char pad10[0x1c - 0x10];
    int           step;         /* +0x1c */
    int           zstep;        /* +0x20 */
    unsigned char pad24[0x30 - 0x24];
} ZEdge;                        /* 0x30 */

/* One span-edge interpolant record: five ints, of which this fill uses the
 * first (x) and the third (z).  The `__asm` block addresses them by BYTE
 * offset, because MSVC's inline assembler does not scale a bracketed index
 * by the element size. */
typedef struct ZInterp { int x; int rest[4]; } ZInterp;   /* 0x14 */

extern short*          g_zb_colour;              /* 0x004b5b20 */
extern short*          g_zb_base;                /* 0x004b5b24 */
extern int             g_zb_pitch;               /* 0x004b5b28 */
extern int             g_zb_polys;               /* 0x0060f900 */
extern unsigned short* g_shade_tab[0x400];       /* 0x00829c60 */

/* WIP: 134/134 instructions and 393/393 bytes, 31 strict mismatches, first
 * divergence at index 22.  The whole hand-written region is index-for-index
 * exact, as is the entire set-up down to the two row bases and the outer
 * loop's key walk.  The residual is two things and nothing else:
 *
 *  1. TWELVE frame homes: the original keeps `row`, `zrow`, `pitch` and
 *     `ylast` in the frame at -0x14/-0x10/-0xc/-8 and gives the three dead
 *     argument slots to `colour` (+8), the z step (+0xc) and VC6's own
 *     hoisted `pitch*2` (+0x10).  We get `colour` at +8 right, but VC6 hands
 *     the two POINTERS the other two argument slots and pushes the z step and
 *     the hoisted stride into the frame instead.  Measured and inert: all
 *     seven declaration orders of the five scalars (schoolcar6.c records the
 *     same negative for its twin), `int` vs `short` for the colour, and
 *     making the z step reuse the `src` parameter's own home by declaring
 *     that parameter `int` and assigning into it -- that DOES move the z step
 *     to +0xc but pushes `colour` out of +8, for the same total of 43 before
 *     the base-load fix and no better after it.
 *  2. FOURTEEN instructions in the per-key edge switch: the original hoists
 *     `e->x` into the block above the branch and loads `e->step` inside each
 *     arm; VC6 hoists `e->step` for us.  All six orderings of the two arms'
 *     stores are inert (VC6 reorders the adjacent stores anyway), and naming
 *     the hoisted value (`int ex = e->x;`) makes it worse, not better -- 55
 *     with the steps-first arms, 70 values-first, 103 with both named.
 *
 * The set-up's own scheduling WAS reachable and is worth recording: writing
 * the two row pointers as `zrow = g_zb_base; row = g_zb_colour;` immediately
 * after `pitch`, and adding `pitch * y` to them at the END of the set-up,
 * takes the body from 105 strict to 31.  Computing them in one statement
 * each (`row = g_zb_colour + pitch * y;`) leaves both base loads at the
 * bottom where the original has them at the top, and costs 74. */
// WIP-FUNCTION: LEGOLAND 0x0041fa10  (134/134 insns, 393/393 B, 31 strict; frame homes + one CSE hoist)
void ZBuffer_FillShadedPoly(int ramp, const int* src, int n,
                            ZKey* key, ZEdge* edge)
{
    ZInterp ed[4];
    short*  row;
    short*  zrow;
    int     pitch;
    int     ylast;
    int     y;
    short   colour;
    int     zstep;

    y = key[0].y;
    edge[key[n - 1].idx].ylast++;
    ylast = edge[key[n - 1].idx].ylast;
    pitch = g_zb_pitch;
    zrow = g_zb_base;
    row  = g_zb_colour;
    zstep = src[1];
    g_zb_polys++;
    colour = g_shade_tab[ramp][src[0]];
    key[n].y = edge[key[n - 1].idx].ylast;
    row  += pitch * y;
    zrow += pitch * y;
    do {
        ZEdge* e = &edge[key->idx];

        key++;
        if (e->side) {
            ed[2].x = e->x - e->step;
            ed[3].x = e->step;
        } else {
            ed[0].x = e->x - e->step;
            ed[0].rest[1] = e->z - e->zstep;
            ed[1].x = e->step;
            ed[1].rest[1] = e->zstep;
        }
        while (y < key->y) {
            y++;
            __asm {
                mov  eax, ed[0]                 /* left.x             */
                mov  edx, ed[8]                 /* left.z             */
                mov  ebx, ed[40]                /* right.x            */
                add  eax, ed[20]                /* += left.step       */
                add  edx, ed[28]                /* += left.zstep      */
                add  ebx, ed[60]                /* += right.step      */
                mov  ed[0], eax
                mov  ed[8], edx
                mov  ed[40], ebx
                mov  ecx, ebx
                sub  ecx, eax
                cmp  ecx, 8000h                 /* half a pixel?      */
                jns  wide
                jmp  done
            wide:
                sar  eax, 16
                sar  ebx, 16
                mov  edi, row
                mov  esi, zrow
                xchg ebx, eax
                sub  ebx, eax                   /* left - right, <= 0 */
                lea  edi, [edi + eax*2]         /* &row[right]        */
                lea  esi, [esi + eax*2]
                mov  ax, colour
            fill:
                mov  ecx, edx
                mov  word ptr [edi + ebx*2], ax
                add  edx, zstep
                sar  ecx, 16
                mov  word ptr [esi + ebx*2], cx
                add  ebx, 1
                jle  fill
            done:
            }
            row  += pitch;
            zrow += pitch;
        }
    } while (y < ylast);
}
