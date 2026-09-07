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

/* A plain `free` wrapper in the schoolcar8.c / curve-sampling neighbourhood. */
// FUNCTION: LEGOLAND 0x0041f710
void CurveSamples_Free(void* samples)
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

/* The natural logarithm as a `float -> double` callback (VC6's intrinsic
 * `log` is fldln2/fyl2x).  Its only reference is the function pointer the
 * dead sampler at 0x0041f7f0 pushes. */
// FUNCTION: LEGOLAND 0x0041f7e0
double CurveFn_Log(float x)
{
    return log(x);
}
