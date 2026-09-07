/* LEGOLAND -- the ROLLER COASTER's TRACK JOINTS: the two fit checkers, the
 * car constructor, the module's two fast-square-root tables, the joint
 * splicer and the joint wiring pass.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere) and follow coaster.c / coaster3d.c / coaster4.c.
 */

typedef struct TrackNode  TrackNode;
typedef struct TrackDesc  TrackDesc;
typedef struct TrackClass TrackClass;
typedef struct CoasterRec CoasterRec;

/* coaster.c's TrackDesc, only as far as this file reads it. */
struct TrackDesc {
    int   raised;               /* +0x00 */
    int   h0;                   /* +0x04  the piece's tail-end height */
    int   h1;                   /* +0x08  the piece's head-end height */
    int   jp0[2];               /* +0x0c  head-end joint parameters */
    int   jp1[2];               /* +0x14  tail-end joint parameters */
    unsigned char pad1c[0x38 - 0x1c];
};                              /* 0x38 */

/* coaster.c's footprint element, now with its first four dwords named: they
 * are the element's map RECT, and TrackPieceSetJoints below is where a
 * piece's copy is derived from its class's. */
typedef struct FootPart {
    int              x0;        /* +0x00 */
    int              y0;        /* +0x04 */
    int              x1;        /* +0x08 */
    int              y1;        /* +0x0c */
    struct FootPart* next;      /* +0x10 */
    unsigned char    pad14[0x24 - 0x14];
} FootPart;                     /* 0x24 */

/* The buildable track CLASS element; only its footprint template is read
 * here. */
struct TrackClass {
    unsigned char pad00[0x3c];
    FootPart      part;         /* +0x3c */
};

/* One END of a track piece. coaster.c calls +0x00 `height`; the two helpers
 * that write it here say what it really is -- 0x0041cc90 is `1 << index` and
 * 0x0041cc50 maps 1 <-> 4 and 2 <-> 8 -- so it is a four-way DIRECTION BIT
 * and 0x0041cc50 is "the opposite way". Renamed accordingly; the offsets are
 * coaster.c's. */
typedef struct TrackJoint {
    int        dir;             /* +0x00  1/2/4/8, -1 = free */
    int        f04;             /* +0x04 */
    TrackNode* node;            /* +0x08 */
} TrackJoint;                   /* 0x0c */

/* coaster.c's TrackNode; the 0xa4-byte PIECE coaster4.c allocates has this
 * as its head. Its map square is one dword key that is also read as two
 * shorts, which is what the joint wiring below does. */
struct TrackNode {
    int           state;        /* +0x00 */
    short         sx;           /* +0x04 */
    short         sy;           /* +0x06 */
    TrackClass*   cls;          /* +0x08 */
    TrackDesc*    desc;         /* +0x0c */
    CoasterRec*   owner;        /* +0x10 */
    TrackJoint    jin;          /* +0x14 */
    TrackJoint    jout;         /* +0x20 */
    FootPart      part;         /* +0x2c  its footprint element */
};                              /* 0x50 */

typedef struct JointSlot {
    int   mask;                 /* +0x00 */
    struct { short x; short y; } sq[4];  /* +0x04 */
} JointSlot;                    /* 0x14 */

struct CoasterRec {
    int           state;        /* +0x00  0 none, 1 open, 2 closed */
    TrackNode     ring;         /* +0x04  the list sentinel */
    unsigned char pad54[0xa8 - 0x54];
    TrackNode*    head_node;    /* +0xa8  outermost piece, head side */
    JointSlot     head_slot;    /* +0xac */
    TrackNode*    tail_node;    /* +0xc0  outermost piece, tail side */
    JointSlot     tail_slot;    /* +0xc4 */
    unsigned char padd8[0x100 - 0xd8];
};

/* The one shared fit block at 0x004d8250. coaster4.c named the last four
 * fields from the constructors' side; the two routines below are what FILLS
 * `an` and `bn`. */
typedef struct TrackFit {
    int         flags;          /* +0x00 */
    CoasterRec* owner;          /* +0x04 */
    int         a;              /* +0x08  head partner candidate, -1 = none */
    TrackNode*  an;             /* +0x0c  the head partner's geometry */
    int         b;              /* +0x10  tail partner candidate, -1 = none */
    TrackNode*  bn;             /* +0x14 */
} TrackFit;

/* ==========================================================================
 * 0x0041d2e0 and 0x0041d350 -- the two FIT CHECKERS.
 *
 * coaster.c's TrackFitCheck picks between them on the class descriptor's
 * `raised` bit 0, exactly as TrackPlaceIfFits picks between coaster4.c's two
 * constructors -- and the pairing is the same one: bit 0 set means the new
 * piece is SPLICED between its two partners (0x0041d630) and its two ends
 * are checked INDEPENDENTLY here; bit 0 clear means the piece SPANS the gap
 * (0x0041d5b0) and the two ends are checked TOGETHER.
 *
 * Both start with 0x0041d210, which is the geometric half: it refuses
 * outright while the coaster is a closed circuit, stamps the fit block's
 * owner with &g_castle, and turns each end's joint offset (desc +0x18/+0x1a
 * for the head, +0x10/+0x12 for the tail) into a candidate index through
 * 0x0041cd40 against that end's JointSlot -- head against g_castle.head_slot
 * (0x00829b8c), tail against g_castle.tail_slot (0x00829ba4). Either index
 * being -1 means that end is free; BOTH being -1 means the piece touches
 * nothing and the fit fails.
 *
 * What is left is the HEIGHT check, and that is what these two do:
 *   * 0x0041d2e0 asks 0x00429840 per end, with the descriptor's own end
 *     height (h1 for the head, h0 for the tail) -- the callee re-derives the
 *     partner's live geometry and refuses unless the end heights agree (it
 *     compares `(float)h` against the geometry's +0x24 / +0x18).
 *   * 0x0041d350 asks 0x004298a0 ONCE for the whole span, handing it the
 *     descriptor and both output slots -- because a spanning piece has to
 *     reach from one partner to the other and the two ends are not
 *     independent. It is skipped entirely unless BOTH ends have a partner,
 *     which is why the span piece can only be laid across a gap.
 * The geometry each check produces is stored in the fit block as `an` / `bn`
 * and that is what coaster4.c's constructors then join to.
 *
 * LEVER (the twins' only codegen difference): 0x0041d350 tests -1 TWICE in
 * ONE condition, so VC6 materialises it with `or eax,0xffffffff` and compares
 * against the register; 0x0041d2e0's two separate `if`s each keep the
 * immediate `cmp ...,-1`. The same pair of shapes coaster4.c records for the
 * two constructors, one function earlier in the same call chain.
 * ======================================================================== */
extern int  TrackFitFindPartners(const short* sq, TrackFit* f,
                                 TrackDesc* d);                 /* 0x0041d210 */
extern TrackNode* TrackFitEndGeom(TrackNode* n, int h);         /* 0x00429840 */
extern int  TrackFitSpanGeom(TrackDesc* d, CoasterRec* rec,
                             TrackNode** an, TrackNode** bn);   /* 0x004298a0 */

// FUNCTION: LEGOLAND 0x0041d2e0
int TrackFitCheckChain(TrackDesc* d, const short* sq, TrackFit* f)
{
    if (TrackFitFindPartners(sq, f, d)) {
        if (f->a == -1
            || (f->an = TrackFitEndGeom(f->owner->head_node, d->h1)) != 0) {
            if (f->b == -1
                || (f->bn = TrackFitEndGeom(f->owner->tail_node, d->h0)) != 0)
                return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x0041d350
int TrackFitCheckSpan(TrackDesc* d, const short* sq, TrackFit* f)
{
    if (TrackFitFindPartners(sq, f, d)) {
        if (f->a == -1 || f->b == -1)
            return 1;
        return TrackFitSpanGeom(d, f->owner, &f->an, &f->bn) != 0;
    }
    return 0;
}

/* ==========================================================================
 * 0x004215d0 -- build one COASTER CAR from a bloke's appearance block.
 *
 * coaster.c's Coaster_AddCar calls this on the block coaster4.c's
 * BuildBlokeAppearance has just filled, and then links the result onto the
 * record's car list. The car is 0x28 bytes:
 *
 *     +0x00  broken   1 if the seated-rider model could not be built
 *     +0x04  model    the seat model,   0x004206b0 by name
 *     +0x08  rider    the seated rider, 0x00421660 from the appearance
 *     +0x0c  bloke    the appearance block's own +0x34
 *     +0x10  prev     coaster.c's list links
 *     +0x14  next
 *     +0x1c  born     GetGameTimer() stamp, written by Coaster_AddCar
 *     +0x20  draw     0x00421560 -- paints the seat through 0x00420e90
 *     +0x24  kill     0x00421980 -- KillCoasterCar
 *
 * so a car is a two-hook object: the list owns it, the ride draws it through
 * +0x20 and destroys it through +0x24. The seat model is chosen on the
 * appearance's normalised sex flag -- "sit.lomansit" (0x004b59f8) for 0 and
 * "sit.logirlsit" (0x004b59e8) for 1 -- and the SAME name goes to both
 * loaders, 0x004206b0 for the mesh and 0x00420710 for its texture set.
 *
 * ORIGINAL REDUNDANCY, reproduced: `c->rider` is written TWICE, first with
 * 0x00420710's result and then, unconditionally, with 0x00421660's. The
 * first store is dead -- the texture handle is thrown away -- and only the
 * second is ever read. (0x00421660 re-picks the same two names itself, which
 * is presumably how the duplicate survived.)
 *
 * TWO CODEGEN NOTES:
 *  * `push edi` sinks past the allocation guard because the guarded block
 *    ends in its own `return c;` -- and the guard's `return 0` needs no
 *    `xor`, because VC6 knows eax still holds the allocation result that the
 *    test proved null (`mov esi,eax` then `test esi,esi`).
 *  * The two arms each call 0x004206b0 and 0x00420710, and VC6 cross-jumps
 *    the SECOND call: only the `push <name>` stays in each arm. The
 *    0x004206b0 argument's clean-up is still pending at the join, so the two
 *    calls share one `add esp,8` -- legal here because both arms leave the
 *    stack the same depth.
 * ======================================================================== */
typedef struct BlokeApp {
    int   sex;                  /* +0x00  0 man, 1 girl */
    int   leg;                  /* +0x04 */
    int   arm;                  /* +0x08 */
    char  chest[0x14];          /* +0x0c */
    char  face[0x14];           /* +0x20 */
    void* bloke;                /* +0x34 */
} BlokeApp;                     /* 0x38 */

typedef struct CoasterCar CoasterCar;
struct CoasterCar {
    int   broken;               /* +0x00 */
    void* model;                /* +0x04 */
    void* rider;                /* +0x08 */
    void* bloke;                /* +0x0c */
    unsigned char pad10[0x20 - 0x10];
    void (*draw)(CoasterCar* c, void* a);       /* +0x20 */
    void (*kill)(CoasterCar* c);                /* +0x24 */
};                              /* 0x28 */

extern void* AllocZeroed(unsigned int size, int a, void* tag, int c); /* 0x004775b0 */
extern void* LoadCoasterMesh(const char* name);                 /* 0x004206b0 */
extern void* LoadCoasterMeshTex(const char* name);              /* 0x00420710 */
extern void* CoasterCar_BuildRider(BlokeApp* app);              /* 0x00421660 */
extern void  CoasterCar_Draw(CoasterCar* c, void* a);           /* 0x00421560 */
extern void  KillCoasterCar(CoasterCar* c);                     /* 0x00421980 */

extern char g_seat_name_girl[];                                 /* 0x004b59e8 */
extern char g_seat_name_man[];                                  /* 0x004b59f8 */

// FUNCTION: LEGOLAND 0x004215d0
CoasterCar* CoasterCar_Create(BlokeApp* app)
{
    CoasterCar* c = (CoasterCar*)AllocZeroed(0x28, 0, 0, 0);

    if (!c)
        return 0;
    c->broken = 0;
    if (app->sex == 0) {
        c->model = LoadCoasterMesh(g_seat_name_man);
        c->rider = LoadCoasterMeshTex(g_seat_name_man);
    } else {
        c->model = LoadCoasterMesh(g_seat_name_girl);
        c->rider = LoadCoasterMeshTex(g_seat_name_girl);
    }
    /* The dead store above is the original's; only this one is ever read. */
    c->rider = CoasterCar_BuildRider(app);
    if (!c->rider)
        c->broken = 1;
    c->draw = CoasterCar_Draw;
    c->kill = KillCoasterCar;
    c->bloke = app->bloke;
    return c;
}

/* ==========================================================================
 * 0x00426b10 and 0x004269e0 -- the module's FAST SQUARE ROOT and FAST
 * INVERSE SQUARE ROOT tables.
 *
 * coaster4.c's Coaster3D_ResetScene calls these two, in this order, right
 * after it has rebuilt the projection; each fills two tables and then plants
 * a function pointer that the rest of the 3D code calls instead of `fsqrt`.
 * The two implementations they publish (0x00426ab0 and 0x00426980) are what
 * name the tables, because their four instructions ARE the interpolation:
 *
 *     e = bits >> 23            the biased exponent, 0..255
 *     k = (bits >> 17) & 0x3f   the top six mantissa bits
 *     m = (bits & 0x7fffff) | 0x3f800000     the mantissa as 1.0 <= m < 2.0
 *     result = (m * tab[k].mul + tab[k].add) * exp[e]
 *
 * -- a per-bucket TANGENT LINE of the function on [1,2), times the exact
 * value for the exponent alone. Both tables fall straight out of that:
 *
 *   sqrt   (tab 0x00610c40 x64x8, exp 0x00610e44 x256x4, hook 0x00829a58)
 *       c = 1 + k/64, v = sqrt(c)
 *       tab[k] = { v/2, 1/(2v) }              d/dm sqrt(m) = 1/(2 sqrt m)
 *       exp[e] = sqrt(2^(e-127))
 *
 *   rsqrt  (tab 0x00610a20 x64x8, exp 0x00611244 x256x4, hook 0x00829a5c)
 *       tab[k] = { 3/(2v), -1/(2v^3) }        d/dm m^-1/2 = -1/(2 m^1.5)
 *       exp[e] = 1/sqrt(2^(e-127)),  exp[0] forced to 1.0
 *
 * so the sqrt table's entry is the tangent of sqrt at the bucket's left
 * edge and the rsqrt table's is the tangent of m^-1/2 there. Only the rsqrt
 * exponent row needs the special case: e = 0 would be 2^127 and it is
 * pinned to 1.0 instead, which is why that loop starts at 1.
 *
 * FOUR THINGS THE CODEGEN SAYS:
 *  * The EBP frame (`push ebp / mov ebp,esp / push ecx`) is forced by the
 *    trailing `__asm` block; without it these two are ordinary esp-frame
 *    leaf loops. The block itself is unmistakable: `lea eax,[0x00426ab0]` is
 *    how the inline assembler takes a function's address (C would emit
 *    `mov dword ptr [g],OFFSET f` in ONE instruction), and the `push eax` /
 *    `pop eax` around it is the hand-written save.
 *  * `pow` is the INTRINSIC form -- `fld qword [2.0] / fild <exp> /
 *    call __CIpow` with both arguments already on the x87 stack.
 *  * Both exponent loops are `i <= 255`, not `i < 256`: the strength-reduced
 *    pointer bound is then `&exp[255]` and the latch is `jle`. Written
 *    `i < 256` it is `jl` against `&exp[256]` -- the ONE instruction between
 *    exact and not.
 *  * ORIGINAL QUIRK, reproduced: 0x004269e0's first loop opens with
 *    `if (i == 0x1d) i = 0x1d;`, a self-assignment left in as a breakpoint
 *    hook. VC6 deletes `i = i;` outright, but propagates the COMPARED
 *    CONSTANT into the arm and stores the register, so the arm survives as
 *    `cmp eax,0x1d / jne +5 / mov [ebp-4],eax` -- a store of `i` over `i`.
 *    The recorded "a degenerate cmp/jne +0 comes only from a self-assignment
 *    arm" rule, with the extra store that a memory-homed counter adds.
 * ======================================================================== */
#include <math.h>
#pragma intrinsic(sqrt, pow)

typedef struct SqrtEnt {
    float add;                  /* +0x00 */
    float mul;                  /* +0x04 */
} SqrtEnt;                      /* 8 */

extern SqrtEnt g_sqrt_tab[64];          /* 0x00610c40 */
extern float   g_sqrt_exp[256];         /* 0x00610e44 */
extern void*   g_fast_sqrt;             /* 0x00829a58 */
extern void    FastSqrt(void);          /* 0x00426ab0 */
#ifdef LEGOLAND_PORTABLE
extern float   ll_FastSqrt(float);       /* coastermath.c, C ABI */
#endif

extern SqrtEnt g_rsqrt_tab[64];         /* 0x00610a20 */
extern float   g_rsqrt_exp[256];        /* 0x00611244 */
extern void*   g_fast_rsqrt;            /* 0x00829a5c */
extern void    FastRSqrt(void);         /* 0x00426980 */
#ifdef LEGOLAND_PORTABLE
extern float   ll_FastRSqrt(float);      /* coastermath.c, C ABI */
#endif

// FUNCTION: LEGOLAND 0x00426b10
void FastSqrt_InitTables(void)
{
    int i;

    for (i = 0; i < 64; i++) {
        float v = (float)sqrt(i * 0.015625f + 1.0f);

        g_sqrt_tab[i].add = v * 0.5f;
        g_sqrt_tab[i].mul = 0.5f / v;
    }
    for (i = 0; i <= 255; i++)
        g_sqrt_exp[i] = (float)sqrt(pow(2.0, i - 127));
#ifndef LEGOLAND_PORTABLE
    __asm {
        push eax
        lea  eax, FastSqrt
        mov  g_fast_sqrt, eax
        pop  eax
    }
#else
    g_fast_sqrt = (void*)ll_FastSqrt;
#endif
}

// FUNCTION: LEGOLAND 0x004269e0
void FastRSqrt_InitTables(void)
{
    int i;

    for (i = 0; i < 64; i++) {
        float v;

        if (i == 0x1d)                  /* the breakpoint hook, reproduced */
            i = 0x1d;
        v = (float)sqrt(i * 0.015625f + 1.0f);
        g_rsqrt_tab[i].add = 3.0f / (v + v);
        g_rsqrt_tab[i].mul = -1.0f / (v * v * v + v * v * v);
    }
    g_rsqrt_exp[0] = 1.0f;
    for (i = 1; i <= 255; i++)
        g_rsqrt_exp[i] = 1.0f / (float)sqrt(pow(2.0, i - 127));
#ifndef LEGOLAND_PORTABLE
    __asm {
        push eax
        lea  eax, FastRSqrt
        mov  g_fast_rsqrt, eax
        pop  eax
    }
#else
    g_fast_rsqrt = (void*)ll_FastRSqrt;
#endif
}

/* ==========================================================================
 * 0x00429750 -- SPLICE a run of track between two pieces and spread the
 * height change across it.
 *
 * coaster4.c's two constructors call this after 0x0041d440 has wired the new
 * piece's joints: the SPAN constructor joins the two partners to each other,
 * the CHAIN constructor joins each partner to the new piece. What "joining"
 * means is here -- the run of pieces from `a`'s tail-side neighbour up to
 * `b` is re-profiled so it climbs linearly from `a`'s tail height to `b`'s
 * head height:
 *
 *     n  = the number of SLOPED joints between them   (0x00429990, which
 *          also hands back the run's end node)
 *     d  = (b->desc->h1 - a->desc->h0) / n            one step's rise
 *
 * and then one pass along the chain. Each piece is classified by 0x00429910
 * from its class and its two joint heights:
 *   * SLOPED -- 0x004296f0 walks forward over the whole sloped span and
 *     returns its last piece plus the number of steps `k` in it, 0x00429560
 *     lays the ramp z .. z + k*d over that span, and the cursor jumps to the
 *     span's own tail neighbour.
 *   * FLAT -- 0x00429690 levels the piece at z and returns the next one.
 * `z` therefore only advances across sloped spans, which is exactly what
 * makes the total rise come out at n*d = the height difference.
 *
 * Two guards return early: no sloped joints at all (n == 0, nothing to
 * spread) and a->jout.node already being `b` (the two are adjacent).
 *
 * THREE LEVERS, all measured here:
 *  * The cursor IS 0x00429990's out-parameter. Written as a separate local
 *    with a dummy out-pointer, VC6 enregisters the cursor in a fourth
 *    callee-saved register (ebp), pushes four registers instead of three and
 *    the whole body drifts: 66 of 79 and 19 bytes short. Passing `&cur`
 *    makes the cursor ESCAPED, so it lives in the dead `a` argument slot and
 *    is reloaded after every call -- which frees ebx for the float `z`'s
 *    dword copy and reproduces the original's three pushes exactly (66 -> 8
 *    at 237/237 bytes). The dead store the original then makes over that
 *    out-value (`cur = a->jout.node;`) is reproduced with it.
 *  * `k` must be declared in the IF-BLOCK, not at function level. As a
 *    function-level local it takes a frame slot and the `k * d` product's
 *    temporary is pushed into the dead `b` argument slot instead; in the
 *    inner scope `k` takes that slot (the recorded "address-taken
 *    out-pointer locals are homed in dead argument slots" rule) and the
 *    product's temporary takes the frame slot. That scope alone is the last
 *    8 mismatches.
 *  * DECLARATION ORDER of the four function-level locals is inert: all 24
 *    permutations are byte-identical, and all six with `k` in the inner
 *    scope are exact. Scope is the lever; order is not.
 * ======================================================================== */
extern int        TrackRunSteps(TrackNode* n, TrackNode** endOut);   /* 0x00429990 */
extern int        TrackJointSloped(TrackDesc* d, int hin, int hout); /* 0x00429910 */
extern TrackNode* TrackRunSpanEnd(TrackNode* n, int* steps);         /* 0x004296f0 */
extern void       TrackRunSetSlope(TrackNode* n, TrackNode* e, int steps,
                                   float z, float dz);               /* 0x00429560 */
extern TrackNode* TrackRunSetLevel(TrackNode* n, float z,
                                   TrackNode* end);                  /* 0x00429690 */

// FUNCTION: LEGOLAND 0x00429750
void TrackJoinPieces(TrackNode* a, TrackNode* b)
{
    TrackNode* cur;
    int        n;
    float      z;
    float      d;

    n = TrackRunSteps(a->jout.node, &cur);
    z = (float)a->desc->h0;
    if (!n)
        return;
    d = (float)(b->desc->h1 - a->desc->h0) / n;
    cur = a->jout.node;                 /* the out-value above is discarded */
    while (cur != b) {
        if (TrackJointSloped(cur->desc, cur->jin.dir, cur->jout.dir)) {
            int        k;
            TrackNode* nx = TrackRunSpanEnd(cur, &k);
            float      dz = k * d;

            TrackRunSetSlope(cur, nx, k, z, dz);
            z += dz;
            cur = nx->jout.node;
        } else {
            cur = TrackRunSetLevel(cur, z, b);
        }
    }
}

/* ==========================================================================
 * 0x0041d440 -- WIRE a freshly created piece's joints into the coaster.
 *
 * Both of coaster4.c's constructors call this immediately after they have
 * filled the piece's four header fields, and it is what turns a loose record
 * into a member of the run. The fit block says which of the piece's two ends
 * has a partner (`a` and `b`, -1 = none) and each end is wired the same way,
 * mirrored between the record's head and tail:
 *
 *     TrackLinkNodes(rec->head_node, p)      splice into the doubly-linked
 *                                            chain (0x0041d430 writes
 *                                            a->jout.node and b->jin.node)
 *     rec->head_node->jout.dir = 1 << f->a   the partner now leaves through
 *                                            the candidate slot that fitted
 *     p->jin.dir = opposite(that)            and we enter through its mirror
 *     TrackNode_Build(p, partner joint +4)   rebuild our geometry from it
 *     rec->head_node = p                     we are the new outermost piece
 *     ConnectJointHead(p, &rec->head_slot)   recompute where the NEXT piece
 *                                            may go from our free end
 *
 * with head/jout/0x0041d170 for the `a` end and tail/jin/0x0041d190 for the
 * `b` end. Note the argument order of the link call: the head end links
 * (partner -> p), the tail end links (p -> partner), which is what keeps the
 * chain's jin/jout sense consistent as the run grows from both ends.
 *
 * Then the state machine. BOTH ends having had a partner means the run has
 * just been closed into a circuit: state 2 and the ride is notified
 * (0x00424e60). Otherwise the piece is drawn and both end slots are released
 * -- and note that BOTH are released even when only one end was wired, which
 * is coaster.c's ReleaseJointSlot being an empty hook.
 *
 * Finally the piece's FOOTPRINT element (its +0x2c FootPart) is derived from
 * its class's template (class +0x3c) by re-basing the template's map rect
 * from the class's own origin to the piece's square, relative to the
 * castle's:  piece.rect = class.rect - castle.square + piece.square, with
 * the element's `next` link cleared so RebuildCastleFootprintChain can
 * re-thread it. The x pair is written before the y pair.
 *
 * ORIGINAL REDUNDANCY, reproduced: `p->owner = f->owner;` is the first thing
 * this does and BOTH constructors have already done it.
 *
 * THREE CODEGEN NOTES:
 *  * -1 is materialised once with `or ebp,0xffffffff` and compared four
 *    times, because the four `!= -1` tests all read one constant. coaster4.c
 *    records the two-test version of the same effect one call up.
 *  * The class's footprint must be read through a NAMED POINTER
 *    (`const FootPart* cf = &p->cls->part;`). Subscripted straight off
 *    `p->cls` VC6 folds every displacement into the loads and the body comes
 *    out ONE instruction short (25 of 114 at 355 bytes); the named pointer
 *    makes VC6 materialise the cursor with `add eax,0x3c` -- after folding
 *    the FIRST read into `[eax+0x3c]`, because that use is scheduled before
 *    the cursor is needed -- and it is 115/115 at 358 bytes. A `TrackClass*`
 *    local for the class itself does NOT do it; the pointer has to be INTO
 *    the sub-record. (`const int*` with [0]/[2]/[1]/[3] subscripts is
 *    byte-identical to the struct-pointer spelling.)
 *  * `rec->ring.sx` is re-read for each of the two x terms while `p->sx` is
 *    read once: the stores through `p` may alias `rec`, so the castle's
 *    square is reloaded after every store while the piece's own fields are
 *    known not to alias its own footprint. That is VC6's alias analysis, not
 *    a source shape -- do not name a local for it.
 * ======================================================================== */
extern void  TrackLinkNodes(TrackNode* a, TrackNode* b);        /* 0x0041d430 */
extern int   JointBitFromIndex(int i);                          /* 0x0041cc90 */
extern int   JointOppositeDir(int dir);                         /* 0x0041cc50 */
extern void  TrackNode_Build(TrackNode* n, int mode);           /* 0x0041cfd0 */
extern void  ConnectJointHead(TrackNode* n, JointSlot* slot);   /* 0x0041d170 */
extern void  ConnectJointTail(TrackNode* n, JointSlot* slot);   /* 0x0041d190 */
extern void  ReleaseJointSlot(JointSlot* slot);                 /* 0x0041d1c0 */
extern void  TrackNode_Draw(TrackNode* n);                      /* 0x0041cfc0 */
extern void  TrackNode_PlaceObject(TrackNode* n);               /* 0x0041ceb0 */
extern void  Coaster_OnCircuitClosed(CoasterRec* rec);          /* 0x00424e60 */

// FUNCTION: LEGOLAND 0x0041d440
void TrackPieceSetJoints(TrackNode* p, TrackFit* f)
{
    CoasterRec*     rec = f->owner;
    const FootPart* cf;

    p->owner = rec;                     /* the constructors did this already */
    if (f->a != -1) {
        TrackLinkNodes(rec->head_node, p);
        rec->head_node->jout.dir = JointBitFromIndex(f->a);
        p->jin.dir = JointOppositeDir(rec->head_node->jout.dir);
        TrackNode_Build(p, rec->head_node->jout.f04);
        rec->head_node = p;
        ConnectJointHead(p, &rec->head_slot);
    }
    if (f->b != -1) {
        TrackLinkNodes(p, rec->tail_node);
        rec->tail_node->jin.dir = JointBitFromIndex(f->b);
        p->jout.dir = JointOppositeDir(rec->tail_node->jin.dir);
        TrackNode_Build(p, rec->tail_node->jin.f04);
        rec->tail_node = p;
        ConnectJointTail(p, &rec->tail_slot);
    }
    if (f->a != -1 && f->b != -1) {
        rec->state = 2;
        Coaster_OnCircuitClosed(rec);
    } else {
        TrackNode_Draw(p);
        ReleaseJointSlot(&rec->head_slot);
        ReleaseJointSlot(&rec->tail_slot);
    }
    TrackNode_PlaceObject(p);
    cf = &p->cls->part;
    p->part.x0 = cf->x0 - rec->ring.sx + p->sx;
    p->part.x1 = cf->x1 - rec->ring.sx + p->sx;
    p->part.y0 = cf->y0 - rec->ring.sy + p->sy;
    p->part.y1 = cf->y1 - rec->ring.sy + p->sy;
    p->part.next = 0;
}
