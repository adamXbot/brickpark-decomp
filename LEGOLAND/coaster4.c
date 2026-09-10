/* LEGOLAND -- the remaining pieces of the ROLLER COASTER: the two track-piece
 * constructors, the bloke's coaster appearance block, the second stored 3D
 * view, the scene reset, the save image's node walk, the piece-geometry
 * post-pass, and the support's ground shadow.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere) and follow coaster.c / coaster3d.c / schoolcar3.c.
 *
 * =========================================================================
 * THE TWO TRACK-PIECE CONSTRUCTORS  (0x0041d5b0 and 0x0041d630)
 * =========================================================================
 * coaster.c's TrackPlaceIfFits picks between them on the class descriptor's
 * state bit 0 -- the same bit TrackFitCheck uses to pick between the two fit
 * routines -- and hands both the shared fit block at 0x004d8250:
 *
 *     TrackFit { flags; owner; a; an; b; bn; }     0x18 bytes
 *              +0x00  +0x04  +0x08 +0x0c +0x10 +0x14
 *
 * where `a` / `b` are the head- and tail-side partner joints (-1 = none) and
 * `an` / `bn` are the pieces those joints belong to.
 *
 * Both allocate a 0xa4-byte PIECE (not the 0x50-byte node coaster.c
 * documents -- the node is the head of a bigger record), run InitTrackNode
 * over it, fill four fields, and let 0x0041d440 wire the joints. They differ
 * only in how the neighbourhood is re-joined afterwards:
 *
 *     0x0041d5b0   both partners present  ->  Join(an, bn)
 *     0x0041d630   each partner present   ->  Join(an, piece), Join(piece, bn)
 *
 * so 0x0041d630 SPLICES the new piece into the run between its two partners
 * and 0x0041d5b0 spans it -- the partners are joined to each other and the
 * new piece merely sits on the joint. Both finish by calling the class
 * descriptor's +0x28 hook on the piece and returning it.
 *
 * ORIGINAL REDUNDANCY, reproduced in both: `piece->owner = f->owner;` is
 * written here AND again as the first thing 0x0041d440 does.
 *
 * LEVER (the twins' only codegen difference beyond the calls): 0x0041d5b0's
 * single `if (f->a != -1 && f->b != -1)` materialises -1 in a register with
 * `or eax,0xffffffff` and compares against it twice, while 0x0041d630's two
 * separate `if`s each keep the immediate `cmp ...,-1`. Two tests of one
 * constant in ONE condition is what buys the register.
 * ========================================================================= */

typedef struct TrackDesc  TrackDesc;
typedef struct TrackPiece TrackPiece;

struct TrackDesc {
    unsigned char pad00[0x28];
    void (*attach)(TrackPiece* p);      /* +0x28 */
};

struct TrackPiece {
    int           state;                /* +0x00 */
    int           key;                  /* +0x04  the packed map square */
    void*         cls;                  /* +0x08  the class element */
    TrackDesc*    desc;                 /* +0x0c */
    void*         owner;                /* +0x10  the CoasterRec */
    unsigned char pad14[0xa4 - 0x14];
};                                      /* 0xa4 */

typedef struct TrackFit {
    int         flags;                  /* +0x00 */
    void*       owner;                  /* +0x04 */
    int         a;                      /* +0x08  head partner, -1 = none */
    TrackPiece* an;                     /* +0x0c */
    int         b;                      /* +0x10  tail partner, -1 = none */
    TrackPiece* bn;                     /* +0x14 */
} TrackFit;

extern void* AllocZeroed(unsigned int size, int a, void* tag, int c); /* 0x004775b0 */
extern void  InitTrackNode(TrackPiece* p);                      /* 0x0041ce30 */
extern void  TrackPieceSetJoints(TrackPiece* p, TrackFit* f);   /* 0x0041d440 */
extern void  TrackJoinPieces(TrackPiece* a, TrackPiece* b);     /* 0x00429750 */

// FUNCTION: LEGOLAND 0x0041d5b0
TrackPiece* TrackCreateSpanPiece(void* cls, TrackDesc* d, const int* sq,
                                 TrackFit* f)
{
    TrackPiece* p = (TrackPiece*)AllocZeroed(0xa4, 0, 0, 0);

    if (!p)
        return 0;
    InitTrackNode(p);
    p->cls = cls;
    p->owner = f->owner;
    p->key = *sq;
    p->desc = d;
    TrackPieceSetJoints(p, f);
    if (f->a != -1 && f->b != -1)
        TrackJoinPieces(f->an, f->bn);
    p->desc->attach(p);
    return p;
}

// FUNCTION: LEGOLAND 0x0041d630
TrackPiece* TrackCreateChainPiece(void* cls, TrackDesc* d, const int* sq,
                                  TrackFit* f)
{
    TrackPiece* p = (TrackPiece*)AllocZeroed(0xa4, 0, 0, 0);

    if (!p)
        return 0;
    InitTrackNode(p);
    p->cls = cls;
    p->owner = f->owner;
    p->key = *sq;
    p->desc = d;
    TrackPieceSetJoints(p, f);
    if (f->a != -1)
        TrackJoinPieces(f->an, p);
    if (f->b != -1)
        TrackJoinPieces(p, f->bn);
    p->desc->attach(p);
    return p;
}

/* ==========================================================================
 * 0x00421890 -- build the coaster's BLOKE APPEARANCE block and return it.
 *
 * blokelist.c documented this block from the data side; this is the builder.
 * Five values are pulled out of the minifig and dropped into the 0x38-byte
 * global at 0x004b5988, whose layout falls straight out of the stores:
 *
 *     +0x00  sex          GetSexOfBloke()      normalised to 0/1
 *     +0x04  leg colour   & 0x00ffffff
 *     +0x08  arm colour   & 0x00ffffff
 *     +0x0c  chest texture name, 20 bytes
 *     +0x20  face texture name, 20 bytes
 *     +0x34  the bloke itself -- written FIRST, before any getter runs
 *
 * coaster.c declares this `void* BuildBlokeAppearance(Bloke*)` and the body agrees: the
 * `mov eax,OFFSET g_bloke_app` that VC6 schedules into the middle of the
 * second `strcpy` expansion is the RETURN VALUE, not a dead load.
 *
 * THREE CODEGEN NOTES:
 *  * The five `push ebx` argument pushes share ONE `add esp,0x14` -- the
 *    recorded "results that land in locals share one add esp", here with the
 *    results landing in GLOBALS, and the fifth push is issued before the
 *    fourth call's `strcpy` expansion.
 *  * `neg/sbb/neg` on the sex getter's result is `(f(b) != 0)`.
 *  * Both name copies are the intrinsic `strcpy` (repne scasb, rep movsd,
 *    rep movsb). The first saves the length in eax and the second in edx,
 *    because eax is busy holding the return value by then.
 * ======================================================================== */
#include <string.h>
#pragma intrinsic(strcpy)

typedef struct Bloke Bloke;

typedef struct BlokeApp {
    int   sex;                  /* +0x00 */
    int   leg;                  /* +0x04 */
    int   arm;                  /* +0x08 */
    char  chest[0x14];          /* +0x0c */
    char  face[0x14];           /* +0x20 */
    void* bloke;                /* +0x34 */
} BlokeApp;                     /* 0x38 */

extern BlokeApp g_bloke_app;                                    /* 0x004b5988 */

extern int   GetSexOfBloke(Bloke* b);                           /* 0x00443140 */
extern int   GetLegColourOfBloke(Bloke* b);                     /* 0x004431f0 */
extern int   GetArmColourOfBloke(Bloke* b);                     /* 0x00443220 */
extern char* GetChestTextureNameOfBloke(Bloke* b);              /* 0x004431a0 */
extern char* GetFaceTextureNameOfBloke(Bloke* b);               /* 0x00443150 */

// FUNCTION: LEGOLAND 0x00421890
void* BuildBlokeAppearance(Bloke* b)
{
    g_bloke_app.bloke = b;
    g_bloke_app.sex = (GetSexOfBloke(b) != 0);
    g_bloke_app.leg = GetLegColourOfBloke(b) & 0xffffff;
    g_bloke_app.arm = GetArmColourOfBloke(b) & 0xffffff;
    strcpy(g_bloke_app.chest, GetChestTextureNameOfBloke(b));
    strcpy(g_bloke_app.face, GetFaceTextureNameOfBloke(b));
    return &g_bloke_app;
}

/* ==========================================================================
 * 0x00425a50 -- reset the coaster module's 3D SCENE.
 *
 * Castle_Create's first initialiser (coaster.c's extern list). It rebuilds
 * the projection from its stored template, re-derives the light direction and
 * re-normalises the module's three fixed direction vectors:
 *
 *  1. The TEMPLATE 4x4 at 0x004b5c1c is written out constant by constant --
 *     sixteen `mov dword ptr [addr],imm`, which is what a global (not a local)
 *     initialiser has to be -- and then assigned WHOLE to the base matrix at
 *     0x004b5c5c that coaster3d.c's Coaster3D_SetupView copies into
 *     0x008299bc. VC6 lowers the assignment to `rep movsd` of 16 dwords and
 *     hoists its ecx/esi/edi setup above all sixteen stores.
 *
 *         [  1.60334  -1.60334   0        0       ]
 *         [  0.801688  0.801688  1.96416  0       ]
 *         [  3.19995   3.19995   0        32767.5 ]
 *         [  0         0         0        1       ]
 *
 *     Five entries of the COPY are then halved: the two x rows and the three
 *     y ones. The template keeps its full-scale values, which is why the light
 *     direction below reads the template and not the base.
 *
 *  2. The light/view direction at 0x00829990 -- the one
 *     Coaster3D_DrawPieceSupport dots against row 2 of a support's basis to
 *     decide whether the shadow is painted before or after the model -- is
 *     rebuilt as { 1, 1, -2 * m[1][0] / m[1][2] } and normalised, together
 *     with the two other fixed vectors at 0x004b5cb0 and 0x004b5cc0. All
 *     three normalise calls share one `add esp,0xc`.
 *
 *  3. SetupTrackDrawView (schoolcar.c, 0x00425bd0) latches the wide view and
 *     0x00426740 finishes the reset.
 * ======================================================================== */
typedef struct Vec3f { float x; float y; float z; } Vec3f;
typedef struct Mat4  { float m[16]; } Mat4;         /* 0x40, row-major 4x4 */

extern Mat4  g_view_tmpl;                                       /* 0x004b5c1c */
extern Mat4  g_view_base;                                       /* 0x004b5c5c */
extern Vec3f g_light_dir;                                       /* 0x00829990 */
extern Vec3f g_dir_4b5cb0;                                      /* 0x004b5cb0 */
extern Vec3f g_dir_4b5cc0;                                      /* 0x004b5cc0 */

extern void FastSqrt_InitTables(void);                                   /* 0x00426b10 */
extern void FastRSqrt_InitTables(void);                                   /* 0x004269e0 */
extern void Vec3Normalise(Vec3f* v);                            /* 0x00425d50 */
extern void SetupTrackDrawView(void);                           /* 0x00425bd0 */
extern void Sub_426740(void);                                   /* 0x00426740 */

// FUNCTION: LEGOLAND 0x00425a50
void Coaster3D_ResetScene(void)
{
    g_view_tmpl.m[0]  =  1.60334f;
    g_view_tmpl.m[1]  = -1.60334f;
    g_view_tmpl.m[2]  =  0.0f;
    g_view_tmpl.m[3]  =  0.0f;
    g_view_tmpl.m[4]  =  0.801688f;
    g_view_tmpl.m[5]  =  0.801688f;
    g_view_tmpl.m[6]  =  1.96416f;
    g_view_tmpl.m[7]  =  0.0f;
    g_view_tmpl.m[8]  =  3.19995f;
    g_view_tmpl.m[9]  =  3.19995f;
    g_view_tmpl.m[10] =  0.0f;
    g_view_tmpl.m[11] =  32767.5f;
    g_view_tmpl.m[12] =  0.0f;
    g_view_tmpl.m[13] =  0.0f;
    g_view_tmpl.m[14] =  0.0f;
    g_view_tmpl.m[15] =  1.0f;
    g_view_base = g_view_tmpl;
    /* The volatile read is FREE -- the original loads m[0] here anyway -- and
     * it is what stops VC6 forward-propagating the constant it just stored in
     * the template THROUGH the `rep movsd` and folding `1.60334f * 0.5f` into
     * a single `mov`. It propagates into the FIRST field read back after a
     * struct copy and no further, which is why only this one needs it; an
     * inline `Half(float*)` helper and a `memcpy` in place of the assignment
     * both still fold (52 of 55). */
    g_view_base.m[0] = *(float volatile*)&g_view_base.m[0] * 0.5f;
    g_view_base.m[1] *= 0.5f;
    g_view_base.m[4] *= 0.5f;
    g_view_base.m[5] *= 0.5f;
    g_view_base.m[6] *= 0.5f;
    FastSqrt_InitTables();
    FastRSqrt_InitTables();
    g_light_dir.x = 1.0f;
    g_light_dir.y = 1.0f;
    g_light_dir.z = g_view_tmpl.m[4] * -2.0f / g_view_tmpl.m[6];
    Vec3Normalise(&g_light_dir);
    Vec3Normalise(&g_dir_4b5cb0);
    Vec3Normalise(&g_dir_4b5cc0);
    SetupTrackDrawView();
    Sub_426740();
}

/* ==========================================================================
 * 0x00427190 -- write the coaster's TRACK PIECES into the save blob's node
 * array.
 *
 * WriteCoasterHeader (coaster.c, 0x00426de0) points blob->nodes at the first
 * byte after the 0x20-byte header and hands it here. Entry 0 is the coaster
 * RECORD ITSELF -- its ring sentinel's class element, packed to an id by
 * 0x0041ebd0, and the sentinel's own square key -- and then one entry per
 * piece, each written by 0x00426e80.
 *
 * The walk is coaster.c's two shapes, chosen on the record's state exactly as
 * that file describes them:
 *   * state 2, a CLOSED CIRCUIT: one lap of the jout chain, stopping at the
 *     sentinel `&rec->ring`. VC6 builds that sentinel by adding 4 to the
 *     record pointer IN PLACE, because `rec` is dead afterwards.
 *   * otherwise an OPEN ROUTE: the jout chain to its NULL end, then the jin
 *     chain to its NULL end -- the two arms growing out of the station.
 * The starting node `rec->ring.jout.node` is read ONCE, before the state
 * test, and serves both shapes.
 *
 * `out++` is spelled in the call argument, which is why every iteration emits
 * the `mov eax,esi / add esi,8 / push eax` copy: the cursor advances BEFORE
 * the call.
 *
 * LEVER: the walk pointer must be declared in EACH ARM'S OWN SCOPE, not once
 * at function level. As one function-level local it out-ranks the `out`
 * parameter -- it has twice the loop-weighted references -- and takes esi,
 * putting `out` in edi and swapping the two registers through all 24
 * instructions of the three loops (36 of 60, register-blind 0). Two
 * block-scope declarations split the web, `out` takes esi, and VC6 STILL
 * hoists the shared `rec->ring.jout.node` load above the state test, so the
 * body is otherwise identical. A local copy of the parameter (35), a hoisted
 * sentinel local (36), `for` instead of `while` (36), a free volatile read on
 * `out` (36) and inverting the state test (30) are all worse.
 * ======================================================================== */
typedef struct TrackNode TrackNode;

typedef struct TrackJoint {
    int        height;          /* +0x00 */
    int        f04;             /* +0x04 */
    TrackNode* node;            /* +0x08 */
} TrackJoint;                   /* 0x0c */

struct TrackNode {
    int           state;        /* +0x00 */
    int           key;          /* +0x04  the packed map square */
    void*         cls;          /* +0x08 */
    void*         desc;         /* +0x0c */
    void*         owner;        /* +0x10 */
    TrackJoint    jin;          /* +0x14 */
    TrackJoint    jout;         /* +0x20 */
    unsigned char part[0x50 - 0x2c];
};                              /* 0x50 */

typedef struct CoasterRec {
    int       state;            /* +0x00  0 none, 1 open, 2 closed */
    TrackNode ring;             /* +0x04  the list sentinel */
} CoasterRec;

typedef struct CoasterNodeRef {
    int node;                   /* +0x00 */
    int key;                    /* +0x04 */
} CoasterNodeRef;               /* 8 */

extern int  PackTrackClass(void* cls);                          /* 0x0041ebd0 */
extern void WriteCoasterNodeRef(TrackNode* n, CoasterNodeRef* out); /* 0x00426e80 */

// FUNCTION: LEGOLAND 0x00427190
void WriteCoasterNodes(CoasterRec* rec, CoasterNodeRef* out)
{
    out->node = PackTrackClass(rec->ring.cls);
    out->key = rec->ring.key;
    out++;
    if (rec->state == 2) {
        TrackNode* n = rec->ring.jout.node;

        while (n != &rec->ring) {
            WriteCoasterNodeRef(n, out++);
            n = n->jout.node;
        }
    } else {
        TrackNode* n = rec->ring.jout.node;

        while (n) {
            WriteCoasterNodeRef(n, out++);
            n = n->jout.node;
        }
        n = rec->ring.jin.node;
        while (n) {
            WriteCoasterNodeRef(n, out++);
            n = n->jin.node;
        }
    }
}

/* ==========================================================================
 * 0x00428750 -- build the module's TRACK DRAW-ORDER table.
 *
 * Track_Create (coaster.c, 0x00427aa0) runs this straight after
 * Coaster3D_BuildPieceGeometry. coaster.c's DrawTrackNode reads the result:
 * `TrackNodeSlopeCode(n)` turns a piece into a small KIND code and `g_611710[kind]`
 * is the mode DrawTrackPiece3D uses to decide which end of the piece is
 * drawn first. This is where that table is filled in.
 *
 * The driver is a six-entry local table of {in, out, mode} triples over the
 * four joint heights the track uses (1, 2, 4 and 8):
 *
 *      1 -> 4 : 2      2 -> 8 : 2      1 -> 2 : 1
 *      2 -> 4 : 2      4 -> 8 : 1      8 -> 1 : 1
 *
 * Each triple is applied TWICE from ONE scratch piece: once as written, and
 * once with the two joint heights swapped, and the reversed direction gets
 * the opposite mode -- 1 where the forward mode was 2, and 2 otherwise. So
 * the twelve entries of the table are six pairs, and running a piece
 * backwards always reverses which end is painted first.
 *
 * The scratch piece is a full 0xa4-byte record on the stack, and only the two
 * joint HEIGHTS (+0x14 and +0x20, the first word of each TrackJoint) are ever
 * written -- everything TrackNodeSlopeCode reads it through is those two fields. The
 * frame is exactly [counter 4][table 0x48][piece 0xa4] = 0xf0, with the trip
 * counter spilled because all four callee-saved registers hold the cursor and
 * the triple.
 *
 * WIP RESIDUAL: 60/60 instructions and everything but the table cursor's
 * BIAS. The original anchors it on the triple's first field (`lea esi,&tbl[0]`
 * then `[esi]/[esi+4]/[esi+8]`); ours anchors on the last (`lea esi,&tbl[0]+8`
 * then `[esi-8]/[esi-4]/[esi]`), which is the recorded anchor rule -- most
 * references, ties broken by the LAST reference -- with all three fields read
 * exactly once. Measured: a flat `const int*` with subscripts (56), a bare
 * `*p` for the first field (55), reading the fields in reverse order (52),
 * `tbl[6-i]` subscripts (48), a whole-triple struct copy (41), and giving
 * `in` or `out` a second reference before the first call to break the tie
 * (51/53/50 -- VC6 reloads instead of reusing, because the scratch piece is
 * address-taken). `*p++` three times DOES anchor at +0 and matches the first
 * two loads, but emits three `add esi,4` where the original has one
 * `add esi,0xc` (58 of 62), and no mixture of `*p++` with subscripts merges
 * them.
 *
 * ALL SIX read orders were measured, and the anchor they produce is a clean
 * rule that DOES NOT REACH +0 from a subscript or a `->` access:
 *
 *     in,out,mode +8   out,in,mode +8   out,mode,in +8
 *     in,mode,out +4   mode,in,out +4   mode,out,in +4
 *
 * -- the anchor is the LATER of the +4 and +8 fields in source order, and the
 * +0 field never wins. A duplicated `e->in` reference DOES move the anchor to
 * +0 (that combination is the only one that reaches the original's `lea`),
 * but it also permutes the three loads into out/in/mode order and costs more
 * than it buys (51). Worth carrying: for a strength-reduced record cursor,
 * offset 0 is unreachable as the anchor unless the field has strictly the
 * most references.
 *
 * Scope G recheck (2026-09-05): the four differences remain at indices
 * 27/29/30/31; resolving the table cursor's +8 bias makes all accesses agree.
 * A named row, int[6][3] table with pointer-to-row, byte cursor, named first
 * field pointer, one-int aggregate copy, moving e++ to the for increment,
 * and one free volatile read on each field are inert. Postincrementing a
 * named row gives +0 but costs an extra cursor copy/update schedule (48
 * mismatches). A diagnostic cancelling e->in-in in out's initializer also
 * reaches +0 and preserves in/out/mode load order, but swaps the cursor/out
 * register webs and perturbs constant initialization (27 mismatches); sum,
 * subtraction and unsigned forms behave identically. It is not a retained
 * implementation. Reassigning in after all three reads gives 10 mismatches.
 * Current best remains four; the cursor bias can be reached, but not with
 * the original allocation in these bounded probes.
 * ======================================================================== */
typedef struct DrawModeEnt {
    int in;                     /* +0x00 */
    int out;                    /* +0x04 */
    int mode;                   /* +0x08 */
} DrawModeEnt;                  /* 0x0c */

extern int TrackPieceKind(TrackPiece* p);                       /* 0x0041ce60 */
extern int g_track_draw_mode[];                                 /* 0x00611710 */

/* Scope G continuation: copy the adjacent in/out fields as one eight-byte
 * record before extracting its two values. The aggregate load fixes both
 * cursor bias and load order: all 60 instructions / 228 bytes now match.
 * Copying the out/mode pair also closes the function; scalarizing all three
 * fields or copying the whole triple loses the required cursor placement.
 */
// FUNCTION: LEGOLAND 0x00428750
void InitTrackDrawModes(void)
{
    DrawModeEnt        tbl[6] = { { 1, 4, 2 }, { 2, 8, 2 }, { 1, 2, 1 },
                                  { 2, 4, 2 }, { 4, 8, 1 }, { 8, 1, 1 } };
    TrackPiece         node;
    const DrawModeEnt* e = tbl;
    int                i;

    for (i = 6; i != 0; i--) {
        /* Copy the joint-height pair together: this keeps the cursor at
         * the first word while retaining the original in/out load order. */
        struct Heights { int in, out; } heights = *(const struct Heights*)&e->in;
        int in = heights.in;
        int out = heights.out;
        int mode = e->mode;
        int k;

        *(int*)((char*)&node + 0x14) = in;
        *(int*)((char*)&node + 0x20) = out;
        g_track_draw_mode[TrackPieceKind(&node)] = mode;
        *(int*)((char*)&node + 0x14) = out;
        *(int*)((char*)&node + 0x20) = in;
        k = TrackPieceKind(&node);
        if (mode == 2)
            g_track_draw_mode[k] = 1;
        else
            g_track_draw_mode[k] = 2;
        e++;
    }
}

/* ==========================================================================
 * 0x004292f0 -- DrawSupportShadow, the second half of
 * Coaster3D_DrawPieceSupport (coaster3d.c, 0x00429150).
 *
 * That routine paints a mid-span support's solid model and its GROUND SHADOW
 * in the order row 2 of the support's basis faces the light; this is the
 * shadow. It builds TWELVE vertices in the module's shadow buffer at
 * 0x006137e8 from twelve template points at 0x00612210, in three groups of
 * four:
 *
 *   0..3   the shadow QUAD: template x/y offset by the support's position
 *          SNAPPED with `(int)pos->x * 0.2 * 5.0`, and the template's own z.
 *   4..7   the same four points at the raw position with z = 0 -- the ring
 *          where the support meets the ground.
 *   8..11  the same x/y again, with z solved from the PLANE through the
 *          support's origin whose normal is row 2 of its basis:
 *              z = (n.pos - x*n.x - y*n.y) / n.z
 *          the dot product being 0x00425d30 on `pos` and `&rot->m[6]`.
 *
 * So the shadow is the ground ring lifted onto the support's own plane and
 * the quad it casts, and the whole thing is handed to the same model painter
 * the solid support uses (0x00420e90, two model tables) at the ORIGIN with an
 * IDENTITY basis -- the vertices already carry the world positions.
 *
 * ORIGINAL BUG, reproduced: the snap is `(float)((int)pos->x * 0.2) * 5.0f`,
 * i.e. the cast is applied to the coordinate and the two scales then cancel
 * exactly, so it truncates to whole units instead of snapping to the 5-unit
 * grid the constants describe. `(int)(pos->x * 0.2) * 5.0f` is what would
 * have snapped; the emitted order (`fld`, `__ftol`, `fild`, `fmul 0.2`,
 * `fmul 5.0`) is unambiguous about which one was written.
 *
 * THREE LEVERS, all measured here:
 *  * The PARENTHESES around `(int)pos->x * 0.2` are load-bearing. Written
 *    flat, `(int)pos->x * 0.2 * 5.0f`, VC6 reassociates the two constants,
 *    folds `0.2 * 5.0` to exactly 1.0 and emits NEITHER multiply (94 of 109);
 *    the explicit sub-expression stops the reassociation and both survive.
 *  * The intermediate must be cast back to FLOAT and the two snapped
 *    coordinates held in `float` locals. As doubles the second constant is
 *    pooled as a double (`fmul qword` instead of the original's
 *    `fmul dword ptr [5.0f]`) and the loop's adds swap which operand is
 *    `fld`ed: 106 of 111 for the all-double spelling, 111/111 for the float
 *    one. Neither local is ever stored -- both live on the x87 stack across
 *    the first loop.
 *  * The two model tables are pushed as ADDRESSES (`push 0x4b6150`), so they
 *    are arrays, not pointer variables; declared as `void*` scalars the call
 *    loads their contents and costs four instructions.
 *
 * FRAME: an ebp frame forced by the two `__asm` RDTSC blocks, 0x30 of locals
 * holding just the identity basis and the origin, the elapsed-cycle counter
 * in the DEAD `rot` argument slot and every `__ftol` result in the DEAD `pos`
 * slot -- both read by push depth, not by their `[ebp+N]` displacement.
 * ======================================================================== */
typedef struct Mat3 { float m[9]; } Mat3;       /* 0x24 */

extern Vec3f g_shadow_src[12];                                  /* 0x00612210 */
extern Vec3f g_shadow_v[12];                                    /* 0x006137e8 */
extern int   g_stat_c_615f68;                                   /* 0x00615f68 */
extern void* g_support_model_a[];                               /* 0x004b6150 */
extern void* g_support_model_b[];                               /* 0x00615f70 */

extern float Vec3Dot(const Vec3f* a, const Vec3f* b);           /* 0x00425d30 */
/* NAME FIX (PORT-M2): this declaration stood as `DrawSupportModel` while its
 * address comment, the relocation and the argument list all say 0x00420e90 =
 * Coaster3D_DrawModel (defined in coaster9.c, spelled that way by coaster10.c,
 * coaster13.c and coastertiny.c). DrawSupportModel is a DIFFERENT function,
 * 0x00429490 in coastertiny.c, which takes two arguments and itself calls
 * 0x00420e90. Invisible to every byte gate -- the address comment and the
 * relocation target were both correct -- but the portable link collapsed this
 * call onto coastertiny.c's two-argument function, so it called the wrong
 * code. Evidence: 0x00429476, the last call in DrawSupportShadow below, is
 * `call 0x420e90`, and 0x00429490's own body is `call 0x420e90`. */
extern void  Coaster3D_DrawModel(void* a, void* b, const Vec3f* p,
                                 const Mat3* r, int mode);      /* 0x00420e90 */

// FUNCTION: LEGOLAND 0x004292f0
void DrawSupportShadow(const Vec3f* pos, const Mat3* rot)
{
    unsigned int t;
    Mat3         ident;
    Vec3f        org;
    const Vec3f* up = (const Vec3f*)&rot->m[6];
    float        bx, by, d;
    int          i;

#ifndef LEGOLAND_PORTABLE
    __asm {
        push eax
        push edx
        rdtsc
        mov  t, eax
        pop  edx
        pop  eax
    }
#else
    t = ll_rdtsc();
#endif
    bx = (float)((int)pos->x * 0.2) * 5.0f;
    by = (float)((int)pos->y * 0.2) * 5.0f;
    for (i = 0; i <= 3; i++) {
        g_shadow_v[i].x = bx + g_shadow_src[i].x;
        g_shadow_v[i].y = by + g_shadow_src[i].y;
        g_shadow_v[i].z = g_shadow_src[i].z;
    }
    for (i = 0; i <= 3; i++) {
        g_shadow_v[i + 4].x = g_shadow_src[i + 4].x + pos->x;
        g_shadow_v[i + 4].y = g_shadow_src[i + 4].y + pos->y;
        g_shadow_v[i + 4].z = 0.0f;
    }
    d = Vec3Dot(pos, up);
    for (i = 0; i <= 3; i++) {
        g_shadow_v[i + 8].x = g_shadow_src[i + 8].x + pos->x;
        g_shadow_v[i + 8].y = g_shadow_src[i + 8].y + pos->y;
        g_shadow_v[i + 8].z = (float)((d - g_shadow_v[i + 8].x * up->x
                                         - g_shadow_v[i + 8].y * up->y)
                                      / up->z);
    }
#ifndef LEGOLAND_PORTABLE
    __asm {
        push eax
        push edx
        rdtsc
        sub  eax, t
        mov  t, eax
        pop  edx
        pop  eax
    }
#else
    t = ll_rdtsc() - t;
#endif
    g_stat_c_615f68 += t;
    org.x = 0.0f;
    org.y = 0.0f;
    org.z = 0.0f;
    ident.m[0] = 1.0f;
    ident.m[1] = 0.0f;
    ident.m[2] = 0.0f;
    ident.m[3] = 0.0f;
    ident.m[4] = 1.0f;
    ident.m[5] = 0.0f;
    ident.m[6] = 0.0f;
    ident.m[7] = 0.0f;
    ident.m[8] = 1.0f;
    Coaster3D_DrawModel(g_support_model_a, g_support_model_b, &org, &ident, 1);
}
