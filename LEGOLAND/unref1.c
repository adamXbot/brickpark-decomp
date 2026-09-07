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
    void*         rider;        /* +0x08  who is aboard */
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
