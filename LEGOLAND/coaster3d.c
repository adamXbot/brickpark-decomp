/* LEGOLAND -- the roller coaster's 3D pipeline: the view transform, the
 * per-vertex projector/clipper, the piece geometry builder, the mid-span
 * support draw, and the polygon front end of the software rasteriser.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere).  schoolcar3.c owns the track-tube half of the same pipeline
 * (Coaster3D_InitTrackTopology / _BuildTrackMesh / _DrawMesh); coaster.c owns
 * the track graph and the piece/place/remove editor that feeds it.
 *
 * =========================================================================
 * THE PIPELINE, END TO END
 * =========================================================================
 * The castle/coaster module carries its own miniature 3D engine, entirely
 * separate from the isometric sprite renderer the rest of the game uses.  It
 * is a fixed-function pipeline in five stages, and this file holds four of
 * them:
 *
 *  1. VIEW SETUP -- Coaster3D_SetupView (0x00425e20), run whenever the module
 *     is reset.  It rebuilds the camera from the map: the base 4x4 at
 *     0x004b5c5c is copied to 0x008299bc, its top-left 2x2 is inverted in
 *     place (0x00425de0), the screen centre of the current view window is
 *     projected back onto the map with ScreenToMapRef, GetTileBounds gives
 *     that square's pixel box, and the difference between the box's centre
 *     and the screen centre becomes the world-space eye position at
 *     0x008299a0/a4/a8.  The window rect (config +0x10/+0x12 size, +0x20/+0x22
 *     origin) is published as the four clip bounds 0x008299ac..0x008299b8 and
 *     handed to the span filler's own clip setup (0x0041ef20).  Finally an
 *     identity is built at [esp] and translated by MINUS the eye, and
 *     MatMul(0x008299bc, that) leaves the world->screen matrix at 0x008299fc,
 *     which is the matrix every later stage composes with.
 *
 *  2. PIECE GEOMETRY -- Coaster3D_BuildPieceGeometry (0x004284d0), run once
 *     per track class from Track_Create.  Four 3-element float triples at
 *     0x004b5df8/e28/e58/e88 (the rail, sleeper and two post templates) are
 *     scaled by 20.0 (one map square) into 0x006117b4, 0x0061164c, 0x00611744
 *     and 0x006116d4 -- note the deliberately rotated destination fields,
 *     which is what swaps y/z as the templates go from model space into track
 *     space -- and then two nested loops stamp the results out: an outer pass
 *     of six pairs building the rail cross-sections (0x00421ce0 copies six
 *     Vec3f into one 0x58-byte record) and an inner pass over four sub-parts
 *     (0x00428350) for each of three post styles.
 *
 *  3. RING PROJECTION -- TransformVerts (0x00426250), the workhorse.  It runs
 *     a list of `n` source Vec3f through a 4x4, rounds each component to an
 *     int, and writes 0x14-byte screen vertices at an arbitrary byte stride.
 *     Each vertex is then given a CLIP WORD (+0x0c) whose low four bits say
 *     which of the four window edges the point is INSIDE (1 = right of left,
 *     2 = left of right, 4 = below top, 8 = above bottom) and whose high
 *     nibble is set to 0xf0 when the point falls wholly inside one of the
 *     renderer's clip rectangles (the ring at 0x00829a3c).  Coaster3D_DrawMesh
 *     in schoolcar3.c rejects a triangle when the OR of its three clip words
 *     does not have all four low bits.
 *
 *  4. MID-SPAN SUPPORT -- Coaster3D_DrawPieceSupport (0x00429150).  The middle
 *     of DrawTrackPiece3D's three passes: it asks the piece for its position
 *     and direction at the MIDPOINT of its parameter range, builds a basis
 *     from the direction, offsets the position by the caller's origin, and
 *     paints the support's model and its ground shadow -- in the order that
 *     row 2 of the basis (the support's own "up") faces the light, which is
 *     the module's painter's algorithm for that pair.
 *
 *  5. RASTER SUBMIT -- Raster_SubmitPoly (0x0042a2f0), the front end of the
 *     span filler.  It takes the PolyJob that Coaster3D_DrawMesh (or any
 *     other 3D caller in the module) filled, clips the index ring against the
 *     window if it has to, splits the polygon into left and right edge lists,
 *     sorts the resulting spans and calls one of the job's two fillers.
 * ========================================================================= */

/* ------------------------------------------------------------------ types */

typedef struct Vec3f { float x; float y; float z; } Vec3f;
typedef struct Mat3  { float m[9];  } Mat3;         /* 0x24 */
typedef struct Mat4  { float m[16]; } Mat4;         /* 0x40, row-major 4x4 */

/* One transformed screen vertex, 0x14 bytes (schoolcar3.c's TrackVtx). */
typedef struct TrackVtx {
    int x;              /* +0x00 */
    int y;              /* +0x04 */
    int z;              /* +0x08 */
    int clip;           /* +0x0c */
    int shade;          /* +0x10 */
} TrackVtx;

/* One node of the renderer's clip-rectangle RING.  The list head at
 * 0x00829a3c is itself a node, so the walk is
 * `for (r = g_clip_ring.next; r != &g_clip_ring; r = r->next)`. */
typedef struct ClipRect ClipRect;
struct ClipRect {
    int       left;     /* +0x00 */
    int       top;      /* +0x04 */
    int       right;    /* +0x08 */
    int       bottom;   /* +0x0c */
    int       mask;     /* +0x10  which edges this rect actually constrains */
    int       f14;      /* +0x14 */
    ClipRect* next;     /* +0x18 */
};

extern ClipRect g_clip_ring;                                    /* 0x00829a3c */

/* The window the coaster's 3D view is clipped to; Coaster3D_SetupView below
 * publishes all four from the config's view rect. */
extern int g_view_left;                                         /* 0x008299ac */
extern int g_view_top;                                          /* 0x008299b0 */
extern int g_view_right;                                        /* 0x008299b4 */
extern int g_view_bottom;                                       /* 0x008299b8 */

/* float -> int in place: the module leaves the x87 in round-to-nearest with
 * everything masked for the whole draw, so this is a bare fistp and never the
 * CRT's __ftol (person3d.c records the same idiom). */
#ifndef LEGOLAND_PORTABLE
#define TOINT(x) __asm { fld x } __asm { fistp dword ptr x }
#else
#define TOINT(x) do { int ll_t = LL_FISTP(LL_ASFLT(x)); LL_ASINT(x) = ll_t; } while (0)
#endif
#define ASINT(x) (*(int*)&(x))
/* float -> int into a SEPARATE int local, int -> float in place, and an
 * int scaled by a float and rounded back into its own slot.  All three are
 * bare x87 with no __ftol, for the same reason. */
#ifndef LEGOLAND_PORTABLE
#define FTOI(f, i)   __asm { fld f } __asm { fistp i }
#define TOFLT(x)     __asm { fild x } __asm { fstp dword ptr x }
#else
#define FTOI(f, i)   ((i) = LL_FISTP(f))
#define TOFLT(x)     do { float ll_f = (float)LL_ASINT(x); LL_ASFLT(x) = ll_f; } while (0)
#endif
#define ASFLT(x)     (*(float*)&(x))
#ifndef LEGOLAND_PORTABLE
#define FSCALE(x, k) __asm { fild x } __asm { fmul k } __asm { fistp x }
#else
#define FSCALE(x, k) (LL_ASINT(x) = LL_FISTPD((double)LL_ASINT(x) * (double)(k)))
#endif

/* =========================================================================
 * 0x00426250 -- TransformVerts (schoolcar3.c declares it under this name).
 *
 * dst[i] = round(M * src[i]) for i in [0, n), stepping `dst` by `stride`
 * BYTES (0x14 for the track mesh's own vertex buffer) and `src` by one
 * 12-byte Vec3f, then classifying each result against the view window and
 * the clip-rectangle ring.
 *
 * The matrix is row-major 4x4 and the vector is a column:
 *     dst[k] = sum_j src[j] * m[k][j]  +  m[k][3],   k = 0..2
 * so `m` is only ever read three rows deep -- the fourth row is never
 * touched, which is why the caller can hand it a 4x3 in a 4x4's storage.
 *
 * THE CLIP WORD (+0x0c) is built in two halves.  The low four bits are
 * INSIDE flags against the window rect: bit 0 = x >= left, bit 1 = x <= right,
 * bit 2 = y >= top, bit 3 = y <= bottom, so a point inside the window has all
 * four and Coaster3D_DrawMesh's `(or & 0xf) == 0xf` test means "no single
 * edge has all three vertices outside it".  Then the clip-rectangle ring is
 * walked; the same four bits are recomputed against each rectangle, and the
 * FIRST rectangle whose own mask (+0x10) is fully satisfied sets 0xf0 in the
 * vertex's word and ends the walk.
 *
 * Note the asymmetry between the two halves: against the WINDOW the code is
 * `|=` into the vertex's own word and comes out branchy, while against a
 * RECTANGLE the x half is an assignment to a fresh local and VC6 lowers the
 * inner ternary branchlessly (setg/dec/and 2/inc).  That is the recorded
 * "the setcc names the FALSE arm of the source ternary" rule -- setg says the
 * ternary was written `x <= r->right ? 3 : 1`.
 *
 * WHAT CLOSED (both new, both measured on this body):
 *  * The outer loop is `while (n-- > 0)`, not a counted `for`.  VC6 evaluates
 *    the guard on the PRE-decrement value and then converts the rest into a
 *    counted down-loop whose trip count it rebuilds with an `inc`, which is
 *    the original's `mov ecx,eax / dec eax / test ecx,ecx / jle ... / inc eax`
 *    exactly.  A plain `for (i = 0; i < n; i++)` simplifies the trip count to
 *    `n` and loses the dec/inc pair; `for (i = 0; i <= n - 1; i++)` gets the
 *    dec/inc but tests n-1 with `jl` instead of n with `jle`, one instruction
 *    short.  Nine other loop spellings measured (i from 1, down-counting,
 *    while/for, an explicit `if (n > 0)` guard) are all worse.
 *  * In the dot-product loop ONE pointer is subscripted and the other is
 *    walked: `acc += sp[j] * *rp++;`.  Spelling BOTH as walks (or both as
 *    subscripts) lets VC6 eliminate one induction variable -- it keeps
 *    `rp - sp` in a register and addresses `[rp_minus_sp + sp]`, which also
 *    commutes the multiply so the `fld` lands on the wrong operand.  The mixed
 *    spelling is 8 mismatches -> 0.  Twelve inner-loop shapes measured: a
 *    down-counting `j`, a `do/while`, split increments, a peeled temporary, a
 *    `volatile` source read, an `int*` row cursor and byte-offset indexing are
 *    all byte-identical to the two-walk form; a sentinel-terminated
 *    `while (p != end)` adds a frame slot; and `sp` walked with `rp[j]`
 *    subscripted is the mirror image and costs 2.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x00426250
void TransformVerts(const Vec3f* s, TrackVtx* d, const Mat4* m, int stride,
                    int n)
{
    int j;
    int k;

    while (n-- > 0) {
        ClipRect*    r   = g_clip_ring.next;
        const float* row = m->m;
        int*         o   = &d->x;

        for (k = 0; k < 3; k++) {
            float acc = 0.0f;

            const float* sp = (const float*)s;
            const float* rp = row;

            for (j = 0; j < 3; j++)
                acc += sp[j] * *rp++;
            acc += row[3];
            TOINT(acc);
            *o = ASINT(acc);
            row += 4;
            o++;
        }

        d->clip = 0;
        if (d->x < g_view_left)
            d->clip |= 2;
        else if (d->x <= g_view_right)
            d->clip |= 3;
        else
            d->clip |= 1;
        if (d->y < g_view_top)
            d->clip |= 8;
        else if (d->y <= g_view_bottom)
            d->clip |= 0xc;
        else
            d->clip |= 4;

        while (r != &g_clip_ring) {
            int c;

            if (d->x < r->left)
                c = 2;
            else
                c = (d->x <= r->right) ? 3 : 1;
            if (d->y < r->top)
                c |= 8;
            else if (d->y <= r->bottom)
                c |= 0xc;
            else
                c |= 4;
            if ((r->mask & c) == 0xf) {
                d->clip |= 0xf0;
                break;
            }
            r = r->next;
        }

        s = (const Vec3f*)((const char*)s + 12);
        d = (TrackVtx*)((char*)d + stride);
    }
}

/* ------------------------------------------------- the drawable, and hooks */

typedef struct DrawObj DrawObj;

/* An object's per-slot geometry hooks: an ARRAY OF PAIRS at +0x4c, indexed by
 * a slot number, each pair {position, direction}.  schoolcar3.c declares the
 * same table with a `void* elem` second parameter because its slot (3) is
 * keyed by an element pointer; slot 1, the one used here, is keyed by a FLOAT
 * -- the piece's own curve parameter -- so this file's prototype takes a
 * float.  The types are deliberately not "aligned": each is a caller-side
 * codegen lever, and the float spelling is what puts the parameter in edi
 * across both calls here. */
typedef struct PosHooks {
    void (*get_pos)(DrawObj* o, float t, Vec3f* out);       /* +0x00 */
    void (*get_dir)(DrawObj* o, float t, Vec3f* out);       /* +0x04 */
} PosHooks;

struct DrawObj {
    unsigned char pad00[0x44];
    float         t0;           /* +0x44  start of the piece's parameter run */
    float         t1;           /* +0x48  end of it */
    PosHooks*     hooks;        /* +0x4c */
};

/* The direction the module orders its two support passes against. */
extern Vec3f g_light_dir;                                       /* 0x00829990 */

extern void  MakeRotation(const Vec3f* dir, Mat3* out);         /* 0x00426560 */
extern float Vec3Dot(const Vec3f* a, const Vec3f* b);           /* 0x00425d30 */
/* The support's solid model (0x00420e90 through two model tables) and its
 * ground shadow (a flat z = 0 ring offset by the position). */
extern void  DrawSupportModel(const Vec3f* p, const Mat3* r);   /* 0x00429490 */
extern void  DrawSupportShadow(const Vec3f* p, const Mat3* r);  /* 0x004292f0 */

/* =========================================================================
 * 0x00429150 -- Coaster3D_DrawPieceSupport (coaster.c's `Coaster3D_DrawPieceSupport`).
 *
 * THE NAME.  DrawTrackPiece3D (0x004294f0) runs three passes over a track
 * piece: the tube from one end (0x00428e70), THIS, then the tube from the
 * other end (0x00428ec0).  This one asks the piece's slot-1 hook pair for the
 * position and direction at the MIDPOINT of its parameter range -- literally
 * `(t1 + t0) * 0.5` off the piece record's +0x44/+0x48 -- turns the direction
 * into a basis, offsets the position by the caller's origin, and paints the
 * two halves of the thing that stands at the middle of a span: its solid
 * model and its ground shadow.
 *
 * THE PAINTER'S ORDER is the whole point of the body.  Three cases:
 *   * pos.z > -0.1  -- the support is at or above the ground plane, so only
 *     the model is drawn and the shadow is skipped entirely.
 *   * otherwise the module dots the global light/view direction at 0x00829990
 *     with ROW 2 of the support's own basis (`rot.m[6..8]`, the row the
 *     3x3 shares its storage with the frame slot at [esp+0x30]).  Negative
 *     means the support leans away, so the shadow is painted FIRST and the
 *     model over it; non-negative paints the model first and the shadow over
 *     it.  Both arms drop the shadow when `mode == 1`.
 * The -0.1 is a pooled DOUBLE, so the source compares the float against a
 * double literal.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x00429150
void Coaster3D_DrawPieceSupport(DrawObj* o, const Vec3f* origin, int mode)
{
    Vec3f pos;
    Vec3f dir;
    Mat3  rot;
    float t;

    t = (o->t1 + o->t0) * 0.5f;
    o->hooks[1].get_dir(o, t, &dir);
    MakeRotation(&dir, &rot);
    o->hooks[1].get_pos(o, t, &pos);
    pos.x += origin->x;
    pos.y += origin->y;
    pos.z += origin->z;
    if (pos.z > -0.1) {
        DrawSupportModel(&pos, &rot);
    } else if (Vec3Dot(&g_light_dir, (const Vec3f*)&rot.m[6]) < 0.0f) {
        if (mode != 1)
            DrawSupportShadow(&pos, &rot);
        DrawSupportModel(&pos, &rot);
    } else {
        DrawSupportModel(&pos, &rot);
        if (mode != 1)
            DrawSupportShadow(&pos, &rot);
    }
}

/* ------------------------------------------------------- the camera state */

typedef struct Pos { int x; int y; } Pos;

typedef struct TileBounds {
    int left;                   /* +0x00 */
    int top;                    /* +0x04 */
    int right;                  /* +0x08 */
    int bottom;                 /* +0x0c */
} TileBounds;

/* Only the view window matters here: size at +0x10/+0x12, origin at
 * +0x20/+0x22, all four unsigned short. */
typedef struct Config {
    unsigned char  pad00[0x10];
    unsigned short w;           /* +0x10 */
    unsigned short h;           /* +0x12 */
    unsigned char  pad14[0x0c];
    unsigned short x;           /* +0x20 */
    unsigned short y;           /* +0x22 */
} Config;

extern Config* lpConfig;                                        /* 0x004bcbf4 */

/* The module's fixed world->screen basis (a 4x4 whose top-left 2x2 is the
 * isometric projection) and the live copy it is edited into. */
extern Mat4  g_view_base;                                       /* 0x004b5c5c */
extern Mat4  g_view_xf;                                         /* 0x008299bc */
extern Vec3f g_eye;                                             /* 0x008299a0 */
extern Mat4  g_view_matrix;                                     /* 0x008299fc */

/* Inverts the 2x2 {a, b, c, d} in place (times the pooled 1.0 at 0x004ab38c),
 * i.e. {d, -b, -c, a} / (ad - bc). */
extern void Invert2x2(float* m);                                /* 0x00425de0 */
extern void MatIdentity(Mat4* out);                             /* 0x004260f0 */
extern void MatMul(const Mat4* a, const Mat4* b, Mat4* out);    /* 0x00426120 */
extern void ScreenToMapRef(Pos* screen, Pos* map, int mode);    /* 0x0045be90 */
extern void GetTileBounds(Pos* tile, TileBounds* out);          /* 0x0045acc0 */
extern void SetSpanClip(int l, int t, int r, int b);            /* 0x0041ef20 */

/* =========================================================================
 * 0x00425e20 -- Coaster3D_SetupView (coaster.c's `Coaster3D_SetupView`, called from
 * Castle_Reset).
 *
 * Rebuilds every piece of camera state the rest of this file reads, from the
 * config's view window and the map scroll position.  In order:
 *
 *  1. The fixed basis at 0x004b5c5c is copied wholesale (rep movsd, i.e. a
 *     struct assignment) into the live transform at 0x008299bc, and the
 *     top-left 2x2 of that -- m[0], m[1], m[4], m[5] -- is copied into a
 *     scratch and INVERTED (0x00425de0).  The inverse is the screen->world
 *     map for the two axes that survive the isometric projection.
 *  2. The centre of the view window becomes the transform's own translation:
 *     m[3] = x + w/2, m[7] = y + h/2, both as floats.  (0x008299c8 and
 *     0x008299d8 are those two fields, not separate globals.)
 *  3. That screen centre is projected back onto the map with ScreenToMapRef,
 *     and GetTileBounds gives the pixel box of the square it lands in.  The
 *     eye is the residual: the screen centre less the box's own centre
 *     ((left + right)/2, top), run through the inverted 2x2, plus the map
 *     square's own world position at 20.0 units per square.  g_eye.z is 0.
 *  4. The window rect is published as the four clip bounds TransformVerts
 *     reads and handed to the span filler (0x0041ef20).
 *  5. An identity 4x4 gets -eye in its translation column and
 *     MatMul(basis, translate) leaves the world->screen matrix at 0x008299fc.
 *
 * All SEVEN cdecl calls share ONE `add esp,0x38`: no result is passed
 * straight into another call and no branch joins the region, so VC6 defers
 * every clean-up to the end.
 *
 * WHAT CLOSED, AND WHAT IS LEFT.
 *  * `dx`/`dy` are built by converting EACH int to a float local first
 *    (`cxf = (float)centre.x; hf = (float)half; dx = cxf - hf;`).  Written as
 *    `(float)centre.x - (float)half` VC6 folds the second conversion into a
 *    single `fisub` and the body comes out two instructions short; the two
 *    float locals are what force the original's `fild / fild / fsubp` pair.
 *    A `(double)` spelling gets one of the two, a negated `-(b - a)` gets the
 *    count but not the schedule.
 *  * The two screen-centre components are BOTH computed before either is
 *    stored into the transform (x, y, m[3], m[7], not x, m[3], y, m[7]):
 *    worth 16 -> 10, because the y half's `movzx` pair is what VC6 schedules
 *    into the first conversion's x87 latency gap.
 *  * ONE free `volatile` read, on the FIRST of the four 2x2 copies.  Without
 *    it VC6 forward-propagates the struct copy and hoists all four loads ABOVE
 *    the `rep movsd` (30 mismatches, first divergence at index 1); the barrier
 *    on the first load alone pins the group after the copy and the other three
 *    then schedule exactly.  Making all four volatile is worse (21) because
 *    each load then has to stay glued to its own store instead of being
 *    batched.  A `volatile` STORE through the copy, a `volatile Mat4*` cursor,
 *    a separate float-array alias of the same global, a `Mat4*`/`float*` local
 *    and every one of the 4! fill orders were measured; none reaches it.
 * ========================================================================= */
// WIP-FUNCTION: LEGOLAND 0x00425e20  (92.2%: 129/129 insns and 466/466 BYTES -- every encoding is right -- with 10 strict mismatches in three allocation clusters and nothing structural left. (a) indices 7-8: the m[0] and m[1] loads are the same two instructions with eax and ecx swapped; the free volatile pins m[0] first where the original loads m[1] first. (b) indices 51/53/54: `add ecx,eax / sar ecx,1 / mov [half],ecx` against our `add eax,ecx / sar eax,1 / mov [half],eax` -- the same three instructions accumulating into the other register; both operand orders of the sum, a two-statement `half = a + b; half >>= 1;`, per-operand shifts and volatile reads of either bound were measured and none flips it. (c) indices 76-80: a pure scheduling permutation -- the original threads `mov eax,[lpConfig] / xor edx,edx / xor ecx,ecx` into the `fmul/fadd/fstp` latency gaps and this build emits the three x87 ops back to back. Register-blind the whole residual is ~0: it is an allocation/scheduling floor, not a missing construct)
void Coaster3D_SetupView(void)
{
    int        half;
    Pos        centre;
    Pos        square;
    float      inv[4];
    TileBounds bounds;
    Mat4       t;
    float      dx;
    float      dy;
    float      cxf;
    float      hf;
    float      cyf;
    float      tf;

    g_view_xf = g_view_base;
    inv[0] = *(volatile float*)&g_view_xf.m[0];
    inv[1] = g_view_xf.m[1];
    inv[2] = g_view_xf.m[4];
    inv[3] = g_view_xf.m[5];
    Invert2x2(inv);

    centre.x = lpConfig->x + (lpConfig->w >> 1);
    centre.y = lpConfig->y + (lpConfig->h >> 1);
    g_view_xf.m[3] = (float)centre.x;
    g_view_xf.m[7] = (float)centre.y;
    ScreenToMapRef(&centre, &square, 0);
    GetTileBounds(&square, &bounds);

    half = (bounds.right + bounds.left) >> 1;
    g_eye.z = 0.0f;
    cxf = (float)centre.x;
    hf = (float)half;
    dx = cxf - hf;
    cyf = (float)centre.y;
    tf = (float)bounds.top;
    dy = cyf - tf;
    g_eye.x = dx * inv[0] + dy * inv[1];
    g_eye.y = dx * inv[2] + dy * inv[3];
    g_eye.x += square.x * 20.0f;
    g_eye.y += square.y * 20.0f;

    g_view_left = lpConfig->x;
    g_view_top = lpConfig->y;
    g_view_right = lpConfig->w + lpConfig->x;
    g_view_bottom = lpConfig->h + lpConfig->y;

    MatIdentity(&t);
    t.m[3] = -g_eye.x;
    t.m[7] = -g_eye.y;
    t.m[11] = -g_eye.z;
    MatMul(&g_view_xf, &t, &g_view_matrix);
    SetSpanClip(g_view_left, g_view_top, g_view_right, g_view_bottom);
}

/* --------------------------------------------------- the track piece table */

/* One prototype track piece, 0x58 bytes, in the table at 0x00828fe0.  This is
 * the same record Coaster3D_DrawPieceSupport walks (+0x44/+0x48 are its
 * parameter range and +0x4c its hook table), so only the fields the two
 * builders fill are named here. */
typedef struct PieceDesc {
    Vec3f     d0;               /* +0x00  the piece's entry direction */
    Vec3f     d1;               /* +0x0c  its exit direction */
    Vec3f     org;              /* +0x18  where in the tile it sits */
    float     r0;               /* +0x24 */
    float     r1;               /* +0x28 */
    unsigned char pad2c[0x18];  /* +0x2c */
    float     t0;               /* +0x44  always 0 */
    float     t1;               /* +0x48  always pi/2 for a curve */
    PosHooks* hooks;            /* +0x4c  0x004dd620 for a curve */
    int       f50;              /* +0x50 */
    int       f54;              /* +0x54 */
} PieceDesc;                    /* 0x58 */

typedef struct IPair { int a; int b; } IPair;
typedef struct FPair { float a; float b; } FPair;

/* The model-space templates (unit tile) and the world-space copies, one map
 * square = 20.0 units.  All four are four Vec3f and the sources are one
 * contiguous 0x30-byte-stride block at 0x004b5e00. */
extern Vec3f g_edge_mid_src[4];                                 /* 0x004b5e00 */
extern Vec3f g_edge_mid[4];                                     /* 0x006117c0 */
extern Vec3f g_half_step_src[4];                                /* 0x004b5e30 */
extern Vec3f g_half_step[4];                                    /* 0x00611658 */
extern Vec3f g_heading_src[4];                                  /* 0x004b5e60 */
extern Vec3f g_heading[4];                                      /* 0x00611750 */
extern Vec3f g_corner_src[4];                                   /* 0x004b5e90 */
extern Vec3f g_corner[4];                                       /* 0x006116e0 */

extern int   g_straight_dir[4];                                 /* 0x004b5ec0 */
extern int   g_straight_off[5];                                 /* 0x004b5ee0 */
extern IPair g_corner_dirs[4];                                  /* 0x004b5ef4 */
extern FPair g_corner_radii[4];                                 /* 0x004b5f14 */
extern PieceDesc g_pieces[];                                    /* 0x00828fe0 */

/* Fills one prototype piece: three Vec3f, the two radii, and the fixed
 * 0..pi/2 parameter range and hook table every curve shares. */
extern void Piece_InitCurve(const Vec3f* d0, const Vec3f* d1,
    const Vec3f* org, PieceDesc* out, float r0, float r1);      /* 0x00421ce0 */
extern void Piece_InitStraight(int dir, int side, int off,
    PieceDesc* out, int slot);                                  /* 0x00428350 */

/* =========================================================================
 * 0x004284d0 -- Coaster3D_BuildPieceGeometry (coaster.c's `Coaster3D_BuildPieceGeometry`, run
 * from Track_Create when the SQUARE_TRACK class is created).
 *
 * Builds the 28 prototype track pieces at 0x00828fe0 that the editor picks
 * from, and the world-space geometry tables they are built out of.
 *
 * 1. FOUR SCALED TABLES.  Four four-element Vec3f templates in model space --
 *    the tile's four EDGE MIDPOINTS (1,0)(2,1)(1,2)(0,1), the four HALF STEPS
 *    (+-0.5 in x and y), the four unit HEADINGS (+x, -y, -x, +y) and the four
 *    tile CORNERS (0,0)(2,0)(2,2)(0,2) -- are each multiplied by 20.0, one
 *    map square, into their live copies.  The four loops are written out in
 *    full rather than nested, which is why each gets its own `xor eax,eax`
 *    and its own `cmp eax,0x24`.
 * 2. EIGHT CURVES.  For each of the four tile corners, two quarter-turn
 *    pieces are stamped: entry/exit headings from the pair table at
 *    0x004b5ef4 ((0,3) (2,3) (2,1) (0,1)) and the two radii from 0x004b5f14
 *    (1.5 and 0.5), then the SAME piece with both pairs swapped -- the inner
 *    and outer curve through that corner.
 * 3. TWENTY STRAIGHTS.  Four headings (0, 2, 1, 3) times five lateral offsets
 *    (-4, -2, 0, 2, 4), each also handed the heading rotated by two quarters
 *    (`(dir - 2) & 3`) and the heading's own index.
 *
 * *** AN ORIGINAL QUIRK, REPRODUCED ***
 * The curve loop carries a DEGENERATE branch: `cmp esi,5 / jne <next
 * instruction>`.  Both arms of the test are the identical second
 * Piece_InitCurve call, so VC6 cross-jumped them into one block and the
 * compare survives with nothing to do.  The source really did test the piece
 * counter against 5 and then do the same thing either way -- most likely a
 * special case for the fifth piece that was written and then never
 * differentiated.  It is reproduced here as the same degenerate if/else,
 * because removing it removes the compare and the branch.
 *
 * WHAT CLOSED.
 *  * `&g_pieces[n++]` in the straight loop's ARGUMENT, not `...&g_pieces[n]);
 *    n++;` after it.  The post-increment lets VC6 advance the cursor BEFORE
 *    the call, which forces the copy of the pre-increment value into ecx that
 *    the original has (`mov ecx,edi / push ecx` rather than `push edi`) and is
 *    the difference between 141 and 143 instructions, 522 and 528 bytes.  In
 *    the CURVE loop the same rewrite is worse (38 -> 42), so it has to be
 *    tested per call site.
 *  * The two radii are `float` locals with r1 declared FIRST: that is what
 *    puts r0 at [esp+0x10] and r1 at [esp+0x14] and makes VC6 spill and
 *    reload BOTH (the original pushes each from a reload, not from the live
 *    register).  Declared r0-first one of them stays live and the body is one
 *    instruction short; an `FPair` struct copy, direct global reads at both
 *    call sites and split declaration/assignment forms are all worse.
 *  * The entry/exit heading pointers are declared b-then-a, which is what puts
 *    arg0 in eax and arg1 in ebp.  All nine orderings of the four locals were
 *    measured.
 * ========================================================================= */
// WIP-FUNCTION: LEGOLAND 0x004284d0  (73.4%: 143/143 insns and 528/528 BYTES over the true body 0x004284d0..0x004286df, 38 strict mismatches, first divergence at index 60; audit.py now bounds it correctly since the 2026-09-05 walker fix. The residual: all four scale loops (indices 0-59) and the entire straight-piece double loop (indices 106-142) are exact index for index; everything left is register allocation in the curve loop's argument setup -- the original keeps the entry heading in eax and 3*b in ecx where this build uses ecx and edx, and the two `inc esi` land one slot later. Free `volatile` reads at all four load sites move it by at most 1 (38 -> 37), which by the recorded one-experiment test makes it a global web rank, not a reachable construct)
void Coaster3D_BuildPieceGeometry(void)
{
    int i;
    int j;
    int k;
    int n;

    n = 0;
    for (i = 0; i <= 3; i++) {
        g_edge_mid[i].x = g_edge_mid_src[i].x * 20.0f;
        g_edge_mid[i].y = g_edge_mid_src[i].y * 20.0f;
        g_edge_mid[i].z = g_edge_mid_src[i].z * 20.0f;
    }
    for (i = 0; i <= 3; i++) {
        g_half_step[i].x = g_half_step_src[i].x * 20.0f;
        g_half_step[i].y = g_half_step_src[i].y * 20.0f;
        g_half_step[i].z = g_half_step_src[i].z * 20.0f;
    }
    for (i = 0; i <= 3; i++) {
        g_heading[i].x = g_heading_src[i].x * 20.0f;
        g_heading[i].y = g_heading_src[i].y * 20.0f;
        g_heading[i].z = g_heading_src[i].z * 20.0f;
    }
    for (i = 0; i <= 3; i++) {
        g_corner[i].x = g_corner_src[i].x * 20.0f;
        g_corner[i].y = g_corner_src[i].y * 20.0f;
        g_corner[i].z = g_corner_src[i].z * 20.0f;
    }

    for (i = 0; i <= 3; i++) {
        float        r1 = g_corner_radii[i].b;
        float        r0 = g_corner_radii[i].a;
        const Vec3f* b = &g_heading[g_corner_dirs[i].b];
        const Vec3f* a = &g_heading[g_corner_dirs[i].a];

        Piece_InitCurve(a, b, &g_corner[i], &g_pieces[n], r0, r1);
        n++;
        /* An ORIGINAL no-op, reproduced: `cmp esi,5 / jne +0` -- a test whose
         * body assigns the counter the value the test just proved it has, so
         * VC6 folds the body away and leaves the compare and a zero-distance
         * branch.  Only the SELF-ASSIGNMENT spelling survives: `n = n;`, an
         * empty block, a bare `;` and a dead local store are all deleted
         * whole, and an if/else with two identical calls merges into one with
         * no branch at all. */
        if (n == 5)
            n = 5;
        Piece_InitCurve(b, a, &g_corner[i], &g_pieces[n], r1, r0);
        n++;
    }

    for (j = 0; j <= 3; j++) {
        for (k = 0; k < 5; k++)
            Piece_InitStraight(g_straight_dir[j], (g_straight_dir[j] - 2) & 3,
                               g_straight_off[k], &g_pieces[n++], j);
    }
}

/* ------------------------------------------------- the span-filler front end */

/* One vertex handed to the rasteriser, 0x1c bytes.  Everything from +0x08 on
 * is an interpolated ATTRIBUTE, and both the edge record and the gradient
 * scratch below repeat the same layout at the same offset, which is why one
 * byte offset walks all of them at once. */
typedef struct PolyVtx {
    int f00;            /* +0x00 */
    int y;              /* +0x04  screen y */
    int a[5];           /* +0x08  a[0] x, a[1] shade, a[2] z */
} PolyVtx;              /* 0x1c */

/* The polygon setup record (schoolcar3.c's PolyJob). */
typedef struct PolyJob {
    int       kind;     /* +0x00 */
    int       tag;      /* +0x04 */
    int       and_flags;/* +0x08  AND of the vertices' clip words */
    int       or_flags; /* +0x0c  OR of them */
    int       f10;      /* +0x10 */
    float     dx1;      /* +0x14 */
    float     dx2;      /* +0x18 */
    float     dy1;      /* +0x1c */
    float     dy2;      /* +0x20 */
    float     area;     /* +0x24  the 2D cross product */
    PolyVtx*  v[4];     /* +0x28  three used, the fourth closes the ring */
    void**    shader;   /* +0x38  two entry points, indexed by `mode` */
} PolyJob;              /* 0x3c */

/* One active edge, 0x30 bytes: the two scanline bounds, which way it runs,
 * the attribute start values in 16.16 and their per-scanline steps. */
typedef struct SpanEdge {
    short y0;           /* +0x00 */
    short y1;           /* +0x02 */
    int   dir;          /* +0x04  0 = downward, 1 = upward */
    int   a[5];         /* +0x08  start values, 16.16 */
    int   d[5];         /* +0x1c  per-scanline steps */
} SpanEdge;             /* 0x30 */

/* The flat-gradient scratch: 24 bytes laid out so its `a[]` sits at the same
 * +0x08 as a vertex's, which is what lets one byte offset walk the three
 * source vertices and this record together.  The two leading ints are the
 * routine's own difference temporaries -- the record's first two fields are
 * never read as gradients, and the original uses them as scratch. */
typedef struct GradRec { int d1; int d2; int a[4]; } GradRec;

/* The edge sort key: the edge's top scanline and its index. */
typedef struct SortKey { int y; int idx; } SortKey;

/* Clips the vertex ring against the mask, returns the (possibly new) vertex
 * pointer array and writes the surviving count. */
extern PolyVtx** Raster_ClipPoly(
    PolyVtx** v, int* count, int mask, int n);                  /* 0x0041ef60 */
extern void Raster_AddSpanRecord(
    int ne, int y, SortKey* keys, SpanEdge* e);                 /* 0x00423200 */
typedef void (*SpanFiller)(int tag, int* grad, int ne, SortKey* keys,
                           SpanEdge* edges);

/* =========================================================================
 * 0x0042a2f0 -- Raster_SubmitPoly (schoolcar3.c declares it under this name).
 *
 * The front end of the module's software rasteriser: it takes the PolyJob a
 * 3D caller filled -- three vertex pointers, the four screen-space edge
 * deltas, the cross product and the two clip masks -- and turns it into an
 * ACTIVE EDGE LIST for one of the job's two span fillers.  It is a plain
 * compiled function, not one of tri3d.c's `__declspec(naked)` inner loops:
 * standard `push ebp / mov ebp,esp / sub esp,0x200` with the callee-saved
 * pushes in the prologue and no `xchg` anywhere in the body.
 *
 * ATTRIBUTE COUNT.  `n` arrives as the vertex count but is used as the number
 * of INTERPOLATED ATTRIBUTES.  If the OR of the clip words has its whole high
 * nibble set -- every vertex inside one of the renderer's clip rectangles --
 * `mode` is 1 and all three attributes (x, shade, z) are carried; otherwise
 * `mode` is 0, `n` drops by one and z is left out.  `mode` is also the index
 * into the job's pair of fillers at +0x38.
 *
 * CLIPPING.  If the AND of the clip words does not have all four low bits the
 * ring is run through 0x0041ef60 first and the returned array replaces the
 * job's own; a zero survivor count returns immediately.  Either way
 * `v[count] = v[0]` closes the ring so the edge walk can read pairs.
 *
 * THE FLAT GRADIENTS.  One PolyVtx-shaped scratch carries the per-pixel
 * gradient of every attribute across the triangle:
 *      g.a[m] = (d1 * dy2 - d2 * dy1) * (65536 / area)
 * with d1 and d2 the attribute's differences from vertex 0 to vertices 1 and
 * 2.  `g.a[0]` is not computed but copied from the job's +0x10, and `&g.a[0]`
 * is what the filler is handed.  The two int differences live in the
 * scratch's own unused +0x00 and +0x04 -- the record is addressed by the same
 * byte offset as the vertices, which is why VC6 forms the base as
 * `&g.a[1] - 0xc`.
 *
 * *** AN ORIGINAL BUG, REPRODUCED ***
 * The gradient is range-checked AFTER it has already been stored, and the
 * clamp assigns a variable that is then dead:
 *      g[m] = t;  if (t < -0x200000 || t > 0x200000) t = 0;
 * so the check does nothing at all -- an out-of-range gradient reaches the
 * span filler unchanged.  The store `mov [ebp-0x10],0` survives in the
 * original with no reader, which is the proof.
 *
 * THE EDGE LIST.  Every ring pair with a non-zero dy becomes an edge: the two
 * scanline bounds as SHORTS in the order the edge runs, `dir` 0 or 1, and the
 * attributes seeded at the upper vertex in 16.16 with their per-scanline
 * steps.  The DOWNWARD arm carries all `n` attributes in a loop; the UPWARD
 * arm carries only a[0].  That asymmetry is the original's.
 *
 * Finally the keys are bubble-sorted by top scanline (whole-struct swaps
 * through a temporary), the last edge's second bound is handed to 0x00423200
 * when the job asks for it and `mode` is 1, and the filler is called.
 *
 * WHAT CLOSED (all four measured on this body, all four new):
 *  * THREE DIFFERENT x87 idioms, one per site, none of them `(int)`:
 *    `fild x / fstp x` converts the scanline height to a float IN PLACE (a
 *    plain `(float)dy` folds into an `fdivr` against the pooled constant and
 *    the body comes out two instructions short); `fild x / fmul k / fistp x`
 *    scales an int attribute difference and rounds it back into its own slot
 *    (a float temporary costs two instructions at each of the two edge
 *    sites); and `fld f / fistp i` moves the flat gradient into a SEPARATE
 *    int.  Getting these three right is worth 24 bytes.
 *  * The 65536 the edge loop divides by is a `volatile float` LOCAL.  Written
 *    as a plain `float k = 65536.0f;` VC6 propagates the constant and reads
 *    the .rdata pool, losing the original's `mov [ebp-0x40],0x47800000`; the
 *    volatile qualifier is free here because the original loads from the slot
 *    on every iteration anyway.  That single store is what makes the body
 *    byte-exact (740 -> 743).
 *  * `&job->v[0]` is captured into its OWN pointer before the clip call and
 *    the gradient loop reads the three vertices through it (`[esi]`,
 *    `[esi+4]`, `[esi+8]`), while the clipped array lives in a second
 *    variable.  Reading `job->v[1]` directly costs the shared base.
 *  * The two attribute differences read vertex 0 through a NAMED local, so
 *    VC6 loads it once into a register and subtracts twice; spelled
 *    `v1->a[m] - v0->a[m]` twice it folds both into memory operands.
 * ========================================================================= */
// WIP-FUNCTION: LEGOLAND 0x0042a2f0  (252/252 instructions and 743/743 BYTES -- every encoding, immediate and addressing form is right -- with 231 strict mismatches. The residual is FRAME and REGISTER allocation, not structure: every block is present in the original's order (the two clip tests, the clipper call, the ring close, the flat-gradient loop, the edge loop with its asymmetric arms, the bubble sort, the conditional 0x00423200 call and the indirect filler call), but this build's scalar frame is 0x1fc against 0x200 -- fifteen homed dwords against sixteen -- so every `[ebp-N]` is four bytes off and each counts as a mismatch. Measured and inert: `int g[3]`/`g[4]`/`g[5]`/`g[6]`/`g[7]` for the gradient scratch (g[6] gets `sub esp,0x200` but costs 3 bytes), a 24-byte struct holding the two difference temporaries and the gradients together (frame exact, 11 bytes short), moving the float/int temporaries and the two `inv` locals between block and function scope, and swapping the two vertex-array pointers' declaration order. The one homed dword this build cannot account for is a compiler temporary, so the next wave should look for a value the original keeps in memory that this build enregisters, not for a missing statement)
void Raster_SubmitPoly(int n, PolyJob* job)
{
    int       cnt;
    int       ne;
    int       mode;
    int       i;
    int       m;
    PolyVtx** vp;
    PolyVtx** jv;
    SortKey*  kp;
    int       d1;
    int       d2;
    int       g[5];
    volatile float k65536;
    SortKey   keys[8];
    SpanEdge  edges[8];

    ne = 0;
    cnt = 3;
    k65536 = 65536.0f;
    kp = keys;
    jv = job->v;
    vp = jv;
    if ((job->or_flags & 0xf0) == 0xf0) {
        mode = 1;
    } else {
        mode = 0;
        n--;
    }
    if ((job->and_flags & 0xf) != 0xf) {
        cnt = 0;
        vp = Raster_ClipPoly(jv, &cnt, job->and_flags & 0xf, n);
        if (cnt == 0)
            return;
    }
    vp[cnt] = vp[0];
    g[0] = job->f10;
    if (n > 1) {
        float inv = 65536.0f / job->area;

        for (m = 1; m < n; m++) {
            float f;
            int   t;
            int   a0 = jv[0]->a[m];

            d1 = jv[1]->a[m] - a0;
            d2 = jv[2]->a[m] - a0;
            f = ((float)d1 * job->dy2 - (float)d2 * job->dy1) * inv;
            FTOI(f, t);
            g[m] = t;
            /* Original bug: `t` is dead after this, so the clamp is lost. */
            if (t < -0x200000 || t > 0x200000)
                t = 0;
        }
    }
    if (cnt > 0) {
        SpanEdge* ecur = edges;
        PolyVtx** p = vp;

        for (i = 0; i < cnt; i++) {
            PolyVtx* va = p[0];
            PolyVtx* vb = p[1];
            int      dy = vb->y - va->y;

            if (dy != 0) {
                SpanEdge* e = ecur;
                float     inv;

                TOFLT(dy);
                inv = k65536 / ASFLT(dy);
                kp->idx = ne;
                if (dy < 0) {
                    ne++;
                    ecur++;
                    kp->y = vb->y;
                    e->dir = 0;
                    e->y0 = (short)vb->y;
                    e->y1 = (short)va->y;
                    kp++;
                    for (m = 0; m < n; m++) {
                        int d;

                        e->a[m] = vb->a[m] << 16;
                        d = vb->a[m] - va->a[m];
                        FSCALE(d, inv);
                        e->d[m] = d;
                    }
                } else {
                    int d;

                    ne++;
                    ecur++;
                    kp->y = va->y;
                    e->dir = 1;
                    e->y0 = (short)va->y;
                    e->y1 = (short)vb->y;
                    kp++;
                    e->a[0] = va->a[0] << 16;
                    d = vb->a[0] - va->a[0];
                    FSCALE(d, inv);
                    e->d[0] = d;
                }
            }
            p++;
        }
    }
    if (ne != 0) {
        int j;

        for (i = ne - 1; i >= 0; i--) {
            for (j = 0; j < i; j++) {
                if (keys[j].y > keys[j + 1].y) {
                    SortKey t = keys[j];

                    keys[j] = keys[j + 1];
                    keys[j + 1] = t;
                }
            }
        }
        if ((job->kind & 1) && mode == 1)
            Raster_AddSpanRecord(ne, edges[keys[ne - 1].idx].y1, keys, edges);
        ((SpanFiller)job->shader[mode])(job->tag, &g[0], ne, keys, edges);
    }
}
