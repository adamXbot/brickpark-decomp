/* LEGOLAND -- scope LL6: raster / map / track-piece callbacks.
 * VC6 SP3 /O2 /Gy /Gd. Types are local; field offsets are load-bearing.
 * Verification and recovered mechanics: docs/lanes/scope-ll6.md.
 *
 * Neighbours name several of these already: Raster_AddSpanRecord (0x00423200)
 * and Piece_InitStraight (0x00428350) in coaster3d.c; GetTrackSegment
 * (0x00424050) in renderview.c. The 0x004b5d20 / 0x004b5d58 TrackDesc
 * vtables hold the joint-fill / height / query / path hooks.
 */

typedef struct Pos { int x, y; } Pos;
typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct Mat3 { float m[9]; } Mat3;
typedef struct Mat4 { float m[16]; } Mat4;
typedef struct ClipBox { int left, top, right, bottom; } ClipBox;

typedef struct TrackNode TrackNode;
typedef struct TrackDesc TrackDesc;
typedef struct TrackJoint {
    int        dir;             /* +0x00  1/2/4/8, -1 = free */
    float      h;               /* +0x04  world height of this end */
    TrackNode* node;            /* +0x08 */
} TrackJoint;

struct TrackDesc {
    int raised;                 /* +0x00 */
    int h0;                     /* +0x04  tail-end height */
    int h1;                     /* +0x08  head-end height */
};

struct TrackNode {
    int         state;          /* +0x00 */
    short       sx;             /* +0x04 */
    short       sy;             /* +0x06 */
    void*       cls;            /* +0x08 */
    TrackDesc*  desc;           /* +0x0c */
    void*       owner;          /* +0x10 */
    TrackJoint  jin;            /* +0x14 */
    TrackJoint  jout;           /* +0x20 */
    unsigned char pad2c[0x50 - 0x2c];
};

typedef struct CoasterRec {
    int        state;           /* +0x00 */
    TrackNode  ring;            /* +0x04 */
    unsigned char pad54[0xa8 - 0x54];
    TrackNode* head_node;       /* +0xa8 */
    unsigned char padac[0xc0 - 0xac];
    TrackNode* tail_node;       /* +0xc0 */
} CoasterRec;

typedef struct PieceHooks {
    void* unused[2];
    void (*eval)(void* obj, float t, Vec3f* out); /* +0x08 */
} PieceHooks;

typedef struct PieceObj {
    unsigned char pad00[0x44];
    float t0;                   /* +0x44 */
    float t1;                   /* +0x48 */
    PieceHooks* hooks;          /* +0x4c */
} PieceObj;

typedef struct SortKey { int y; int idx; } SortKey;
typedef struct SpanEdge {
    short y0, y1;
    int dir;
    int a[5];
    int d[5];
} SpanEdge;

typedef struct PieceDesc {
    unsigned char pad00[0x58];
} PieceDesc;

extern int g_station_h1;                        /* 0x004b5b4c */
extern int g_station_height;                    /* 0x004b5b50 */
extern unsigned char g_castle_curve_a[];        /* 0x006102f8 */
extern Vec3f g_edge_mid[];                      /* 0x006117c0 */
extern Vec3f g_half_step[];                     /* 0x00611658 */
extern PieceDesc g_pieces[];                    /* 0x00828fe0 */
extern Mat4 g_view_matrix;                      /* 0x008299fc */
extern CoasterRec g_castle;                     /* 0x00829ae0 */
extern void* g_span_cursor;                     /* 0x004b5b3c (schoolcar.c: g_cmd_write) */
extern int   g_cmd_buf[];                       /* 0x004dd870, 0x6000 bytes; see Raster_AddSpanRecord */
extern int g_span_count;                        /* 0x0060f908 */
extern int g_span_overflow;                     /* 0x0060f90c */

extern int JointOppositeDir(int dir);           /* 0x0041cc50 */
extern int TrackNodeSlopeCode(TrackNode* n);    /* 0x0041ce60 */
extern void* GetTrackNodeWorldPos(TrackNode* n, Vec3f* out); /* 0x0041cff0 */
extern void Curve_InitLine(void* dest, const Vec3f* from, const Vec3f* to, const Vec3f* offset); /* 0x00421ab0 */
extern void MakeTransform(const Vec3f*, const Mat3*, Mat4*); /* 0x004264e0 */
extern void MatMul(const Mat4*, const Mat4*, Mat4*);         /* 0x00426120 */
extern void AddRollerCoasterPath(Pos* pos);     /* 0x0045dc50 */
extern void RemoveRollerCoasterPath(Pos* pos);  /* 0x0045dcd0 */

/* ---- table stubs ------------------------------------------------------- */

/* TrackDesc +0x2c remove of the flat piece at 0x004b5d20. */
// FUNCTION: LEGOLAND 0x004275b0
void TrackNode_RemoveNop(TrackNode* node)
{
    (void)node;
}

/* TrackDesc +0x28 place of the same flat piece. */
// FUNCTION: LEGOLAND 0x004275c0
void TrackNode_PlaceNop(TrackNode* node)
{
    (void)node;
}

/* Query hook next to the station joint inits; returns the castle entrance
 * curve at 0x006102f8 (coaster7.c's g_castle_curve_a). */
// FUNCTION: LEGOLAND 0x00423990
void* Castle_GetCurveA(void)
{
    return g_castle_curve_a;
}

/* Write the two station end-heights as floats. 0x004b5b50 is the existing
 * g_station_height; 0x004b5b4c is the paired tail height. */
// FUNCTION: LEGOLAND 0x00423970
void TrackNode_SetStationHeights(TrackNode* node)
{
    node->jin.h = (float)g_station_height;
    node->jout.h = (float)g_station_h1;
}

/* Station piece: dirs 2 and 8 (a 2<->8 pair) plus the station heights. */
// FUNCTION: LEGOLAND 0x00423940
void TrackNode_InitStationJoints(TrackNode* node)
{
    node->jin.dir = 2;
    node->jin.h = (float)g_station_height;
    node->jout.dir = 8;
    node->jout.h = (float)g_station_h1;
}

/* Build hook of the flat piece: stamp the same float onto both ends. The
 * original stores +0x24 via fstp and +0x18 via a raw dword copy. */
// FUNCTION: LEGOLAND 0x00427a80
void TrackNode_SetBothHeights(TrackNode* node, float h)
{
    node->jout.h = h;
    node->jin.h = h;
}

/* Build hook of the raised piece: desc->h0 is the tail, desc->h1 the head. */
#ifdef LEGOLAND_PORTABLE
/* PORT-M3: the TrackDesc +0x20 build slot is called with two arguments
 * (coaster.c TrackNode_Build) and the flat piece's entry,
 * TrackNode_SetBothHeights, takes (TrackNode*, float). This body ignores the
 * second dword, which x86 cdecl tolerates and a wasm call_indirect does not,
 * so the portable build exports a twin of the slot's shape over the matched
 * body. The body itself is untouched; VC6 never sees the rename. */
#define TrackNode_LoadDescHeights TrackNode_LoadDescHeights_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00427c70
void TrackNode_LoadDescHeights(TrackNode* node)
{
    TrackDesc* desc = node->desc;
    node->jout.h = (float)desc->h0;
    node->jin.h = (float)desc->h1;
}
#ifdef LEGOLAND_PORTABLE
#undef TrackNode_LoadDescHeights
void TrackNode_LoadDescHeights(TrackNode* node, float ll_unused)
{
    (void)ll_unused;
    TrackNode_LoadDescHeights_vc6_body(node);
}
#endif

/* Draw/setup hook shared by the flat and raised TrackDesc vtables: a free
 * end (-1) takes the opposite direction and the other end's height. */
// FUNCTION: LEGOLAND 0x00427a40
void TrackNode_FillOppositeJoints(TrackNode* node)
{
    int dir = node->jout.dir;
    if (dir == -1) {
        node->jout.dir = JointOppositeDir(node->jin.dir);
        node->jout.h = node->jin.h;
        return;
    }
    if (node->jin.dir == -1) {
        node->jin.dir = JointOppositeDir(dir);
        node->jin.h = node->jout.h;
    }
}

/* Identical body, raised-class vtable at 0x004b5d74. */
// FUNCTION: LEGOLAND 0x00427c30
void TrackNode_FillOppositeJointsH(TrackNode* node)
{
    int dir = node->jout.dir;
    if (dir == -1) {
        node->jout.dir = JointOppositeDir(node->jin.dir);
        node->jout.h = node->jin.h;
        return;
    }
    if (node->jin.dir == -1) {
        node->jin.dir = JointOppositeDir(dir);
        node->jin.h = node->jout.h;
    }
}

/* Lowest set-bit index. The first test is the byte form `test cl, dl`. */
// FUNCTION: LEGOLAND 0x00428840
int LowestSetBitIndex(unsigned int mask)
{
    unsigned int bit = 1;
    int i = 0;
    if (!(mask & bit)) {
        do {
            bit += bit;
            i++;
        } while ((mask & bit) == 0);
    }
    return i;
}

/* ---- small leaves ------------------------------------------------------ */

// FUNCTION: LEGOLAND 0x00425da0
int Vec3f_Equal(const Vec3f* a, const Vec3f* b)
{
    if (a->x != b->x)
        return 0;
    if (a->y != b->y)
        return 0;
    if (a->z != b->z)
        return 0;
    return 1;
}

/* src = arg0, dest = arg1. Returning the walked dest keeps dest in eax
 * from the first instruction (before the saved-register pushes). */
// FUNCTION: LEGOLAND 0x00426190
float* Mat4_Transpose(const float* src, float* dst)
{
    int i;
    int j;
    for (i = 0; i < 4; i++) {
        const float* col = src;
        for (j = 0; j < 4; j++) {
            *dst++ = *col;
            col += 4;
        }
        src++;
    }
    return dst;
}

/* dest = arg0, src = arg1. Same walked-dest return as Mat4_Transpose. */
// FUNCTION: LEGOLAND 0x00426460
float* Mat3_FromMat4Transpose(float* dst, const float* src)
{
    int i;
    int j;
    for (i = 0; i < 3; i++) {
        const float* col = src;
        for (j = 0; j < 3; j++) {
            *dst++ = *col;
            col += 4;
        }
        src++;
    }
    return dst;
}

extern int TrackPiece_FindIndex(TrackNode* node); /* 0x004283c0 */

/* Query hook: piece-table slot from the finder at 0x004283c0. */
// FUNCTION: LEGOLAND 0x004286e0
PieceDesc* TrackNode_GetPieceDesc(TrackNode* node)
{
    return &g_pieces[TrackPiece_FindIndex(node)];
}

/* 2x2 path tiles: (x,y), (x+1,y), (x+1,y+1), (x,y+1). */
// FUNCTION: LEGOLAND 0x00427f70
void TrackNode_RemovePath(TrackNode* node)
{
    Pos p;
    p.x = node->sx;
    p.y = node->sy;
    RemoveRollerCoasterPath(&p);
    p.x = node->sx + 1;
    p.y = node->sy;
    RemoveRollerCoasterPath(&p);
    p.x = node->sx + 1;
    p.y = node->sy + 1;
    RemoveRollerCoasterPath(&p);
    p.x = node->sx;
    p.y = node->sy + 1;
    RemoveRollerCoasterPath(&p);
}

// FUNCTION: LEGOLAND 0x00427ff0
void TrackNode_AddPath(TrackNode* node)
{
    Pos p;
    p.x = node->sx;
    p.y = node->sy;
    AddRollerCoasterPath(&p);
    p.x = node->sx + 1;
    p.y = node->sy;
    AddRollerCoasterPath(&p);
    p.x = node->sx + 1;
    p.y = node->sy + 1;
    AddRollerCoasterPath(&p);
    p.x = node->sx;
    p.y = node->sy + 1;
    AddRollerCoasterPath(&p);
}

/* coaster3d.c already declares this name. off*6 is integer, then fild. */
// FUNCTION: LEGOLAND 0x00428350
void Piece_InitStraight(int dir, int side, int off, PieceDesc* out, int slot)
{
    Vec3f to;
    to.x = g_edge_mid[side].x;
    to.y = g_edge_mid[side].y;
    to.z = g_edge_mid[side].z - (float)(off * 6);
    Curve_InitLine(out, &g_edge_mid[dir], &to, &g_half_step[slot]);
}

/* First compare as an inlined helper with clip first so VC6 loads arg1
 * into eax before dest. */
static __inline int ClipMissesRight(const ClipBox* clip, const ClipBox* dest)
{
    return dest->left > clip->right;
}

/* Clip dest against clip; 0 if they miss, 0xf after clamp.
 * ClipRect_SetBounds stores the return in dest->mask. */
// FUNCTION: LEGOLAND 0x004265d0
int ClipRect_ClipTo(ClipBox* dest, const ClipBox* clip)
{
    int left;
    int right;
    int top;
    int bottom;
    if (ClipMissesRight(clip, dest))
        return 0;
    left = dest->left;
    right = dest->right;
    if (right < clip->left)
        return 0;
    top = dest->top;
    if (top > clip->bottom)
        return 0;
    bottom = dest->bottom;
    if (bottom < clip->top)
        return 0;
    if (left < clip->left)
        dest->left = clip->left;
    if (right > clip->right)
        dest->right = clip->right;
    if (top < clip->top)
        dest->top = clip->top;
    if (bottom > clip->bottom)
        dest->bottom = clip->bottom;
    return 0xf;
}

#ifndef LEGOLAND_PORTABLE
#define TOINT(x) __asm { fld x } __asm { fistp dword ptr x }
#else
#define TOINT(x) do { int ll_t = LL_FISTP(x); __builtin_memcpy(&(x), &ll_t, 4); } while (0)
#endif
#define ASINT(x) (*(int*)&(x))

/* Same 2-row walk as TransformVerts (0x00426250): while(n-- > 0), one
 * pointer subscripted and the other walked, convert-in-place then ASINT.
 * screen[4] keeps the original 0x14-byte frame (two live ints + 8-byte
 * ballast + the trip count). */
// FUNCTION: LEGOLAND 0x004263a0
void ProjectVertsToRect(const Vec3f* verts, const Mat4* mat, int n, ClipBox* out)
{
    int k;
    int j;
    int screen[4];
    out->right = (int)0x80000000;
    out->bottom = (int)0x80000000;
    out->left = 0x7fffffff;
    out->top = 0x7fffffff;
    while (n-- > 0) {
        const float* row = mat->m;
        int* pix = screen;
        for (k = 0; k < 2; k++) {
            float acc = 0.0f;
            const float* sp = (const float*)verts;
            const float* rp = row;
            for (j = 0; j < 3; j++)
                acc += sp[j] * *rp++;
            acc += row[3];
            TOINT(acc);
            *pix = ASINT(acc);
            row += 4;
            pix++;
        }
        if (screen[0] > out->right)
            out->right = screen[0];
        if (screen[0] < out->left)
            out->left = screen[0];
        {
            int y = screen[1];
            int b = out->bottom;
            if (y > b)
                out->bottom = y;
            if (y < out->top)
                out->top = y;
        }
        verts = (const Vec3f*)((const char*)verts + 12);
    }
}

/* Model pos/rot composed with the view matrix, then eight verts → AABB. */
// FUNCTION: LEGOLAND 0x00426750
void Model_ProjectClipRect(const Vec3f* verts, const Vec3f* pos, const Mat3* rot, ClipBox* out)
{
    Mat4 local;
    Mat4 view;
    MakeTransform(pos, rot, &local);
    MatMul(&g_view_matrix, &local, &view);
    ProjectVertsToRect(verts, &view, 8, out);
}

/* Per-piece half of GetTrackSegment. `*p1 = *tile` (not field stores)
 * is what frees ecx for the jout.node reload and the memory `add -16`. */
// FUNCTION: LEGOLAND 0x00423f40
int GetTrackSegmentPiece(Pos* tile, float* h0, Pos* p1, float* h1,
                         TrackNode* node, int* link)
{
    TrackNode* n = node;
    TrackNode* next;
    Vec3f at;
    Vec3f world;
    PieceObj* obj;

    {
        Vec3f* w = &world;
        *link = n->jin.node == &g_castle.ring;
        obj = (PieceObj*)GetTrackNodeWorldPos(n, w);
    }
    obj->hooks->eval(obj, (obj->t1 + obj->t0) * 0.5f, &at);
    at.z += world.z;
    *h0 = at.z * -2.0f;

    next = n->jout.node;
    if (next && next != &g_castle.ring) {
        obj = (PieceObj*)GetTrackNodeWorldPos(next, &world);
        obj->hooks->eval(obj, (obj->t1 + obj->t0) * 0.5f, &at);
        at.z += world.z;
        *h1 = at.z * -2.0f;
        p1->x = next->sx;
        p1->y = next->sy;
        return 1;
    }
    *p1 = *tile;
    if (n->jout.node == &g_castle.ring)
        p1->y += -16;
    *h1 = 0.0f;
    return 1;
}

/* Walk the live coaster for the piece on `tile`. Closed = jout ring;
 * open = tail via jout then head via jin. nxt is loaded after sx and
 * assigned back before the sentinel test; open loads the tile pointer
 * before the tail==ring guard. Tail-match stays pending until after
 * head so both latches invert and fail2 survives.
 * LL23 2026-09-10: the body is really 84 instructions, not 87 -- audit's
 * 87i/232B counts three bytes of COMDAT padding, because the missing
 * block is the four-instruction fail2 (`pop edi; pop esi; xor eax,eax;
 * ret`) minus the extra `jmp` our inverted head latch emits.  In the
 * original fail1 (0x424090) serves the closed-ring empty test, the closed
 * loop's fall-through AND the head-empty test (a BACKWARD branch), while
 * the head loop's exhaustion falls through into its own copy at 0x4240f1
 * -- the LEVERS rule "two return K sites merge at the FIRST; the
 * fall-through copy of a shared tail survives".  New negative: writing
 * that literally, with `goto fail1` from the head-empty test jumping INTO
 * the `state == 2` block to a label on its trailing `return 0`, is
 * bit-identical to the tip (84i/229B, 53/87).  VC6 canonicalises the two
 * identical returns before layout, so the source cannot separate them;
 * our head latch stays `je loop / jmp fail1`. */
// WIP-FUNCTION: LEGOLAND 0x00424050  (head fail2 / call-site order)
int GetTrackSegment(Pos* tile, float* h0, Pos* p1, float* h1, int* link)
{
    TrackNode* n;
    TrackNode* nxt;
    Pos* t;
    int tx;
    int sx;
    int from_tail = 0;
    if (g_castle.state == 2) {
        n = g_castle.ring.jout.node;
        if (n == &g_castle.ring)
            return 0;
        t = tile;
        tx = t->x;
        do {
            sx = (int)n->sx;
            nxt = n->jout.node;
            if (sx == tx && (int)n->sy == t->y)
                goto did_match;
            n = nxt;
        } while (nxt != &g_castle.ring);
        return 0;
    }
    n = g_castle.tail_node;
    t = tile;
    if (n == &g_castle.ring)
        goto head;
    from_tail = 1;
    tx = t->x;
    do {
        sx = (int)n->sx;
        nxt = n->jout.node;
        if (sx == tx && (int)n->sy == t->y)
            goto did_match;
        n = nxt;
    } while (nxt != &g_castle.ring);
head:
    from_tail = 0;
    n = g_castle.head_node;
    if (n == &g_castle.ring)
        return 0;
    tx = t->x;
    do {
        sx = (int)n->sx;
        nxt = n->jin.node;
        if (sx == tx && (int)n->sy == t->y)
            goto did_match;
        n = nxt;
    } while (nxt != &g_castle.ring);
    return 0;
did_match:
    if (from_tail)
        return GetTrackSegmentPiece(t, h0, p1, h1, n, link);
    return GetTrackSegmentPiece(t, h0, p1, h1, n, link);
}

/* Map a live piece onto the 28-entry prototype table. Add in the arm
 * and return after the chain so VC6 cannot fold `off+K` into lea. */
// FUNCTION: LEGOLAND 0x004283c0
int TrackPiece_FindIndex(TrackNode* node)
{
    int slope = TrackNodeSlopeCode(node);
#ifndef LEGOLAND_PORTABLE
    int off = ((int)node->jout.h - (int)node->jin.h) >> 1;
#else
    /* PORT-M5: 0x004283d2 / 0x004283dc round the two joint HEIGHTS (both
     * `float h`), and the piece index is picked off their difference. */
    int off = (LL_FISTP(node->jout.h) - LL_FISTP(node->jin.h)) >> 1;
#endif
    off += 2;
    if (node->jin.dir == JointOppositeDir(node->jout.dir)) {
        if (slope == 8)
            off += slope;
        else if (slope == 2)
            off += 13;
        else if (slope == 13)
            off += 18;
        else if (slope == 7)
            off += 23;
        return off;
    }
    switch (slope) {
    case 12: return 0;
    case 3:  return 1;
    case 4:  return 2;
    case 1:  return 3;
    case 6:  return 4;
    case 9:  return 5;
    case 14: return 6;
    case 11: return 7;
    }
    return -1;
}

/* Append one span-group to the software-rasteriser's edge table.
 * Count-before-cursor is the push-ecx / mov ecx,[cursor] prologue.
 * Residual is ebx vs esi, the edx saved copy, and the keys-1 IV.
 * LL23 2026-09-10, two more families ruled out.  The Codex-F scope-V
 * CANCELLED PAIR cannot split `saved` from `cur`: anchored on a link-time
 * address constant through a second struct member it costs the anchor's
 * `mov reg,imm32` (63i/187B); anchored on `n` it folds and is inert
 * (61i/174B, 24/61, below the tip); a `volatile` reload of cur is 64i and
 * ESCAPES; `saved` first with `cur = saved` is 62i.  The keys cursor is
 * inert to spelling: `int ky = keys->y` before the increment (61i but
 * 179B, 17/61), a `char*` step with `*(short*)((char*)keys - 8)`, a
 * volatile read of keys[-1].y, `(keys - 1)->y`, and folding the subscript
 * into the SpanEdge address all reproduce the tip's `lea edx,[eax-8]`
 * biased cursor byte for byte.  Tip stays 26/61 index-for-index. */
// WIP-FUNCTION: LEGOLAND 0x00423200  (61i/175B, ebx/edx/keys IV)
void Raster_AddSpanRecord(int ne, int y, SortKey* keys, SpanEdge* edges)
{
    char* cur;
    char* saved;
    int n;
    int keep;

    n = g_span_count;
    cur = (char*)g_span_cursor;
    saved = cur;
    n++;
    g_span_count = n;
#ifndef LEGOLAND_PORTABLE
    if ((unsigned)(cur + 0x10) > (unsigned)0x004e3870) {
#else
    /* PORT-M9: 0x004e3870 is the END of the command buffer this appends to --
     * schoolcar.c's Coaster3D_EndFrame names the pair, `g_cmd_buf` at
     * 0x004dd870 (0x6000 bytes) and `g_zbuffer` at 0x004e3870 right behind it,
     * and our `g_span_cursor` is its `g_cmd_write`.  The original compares
     * against that absolute VA, which only means "the end of the buffer"
     * because of where the linker put the two objects; in the portable build
     * they are separate C objects at unrelated linear addresses, so the raw
     * bound is meaningless -- it either never fires (and the writes below run
     * off the end of the buffer) or always fires (and no span is ever
     * recorded).  The bound expressed from the buffer itself is the same number
     * on x86 and the right one everywhere. */
    if ((char*)cur + 0x10 > (char*)g_cmd_buf + 0x6000) {
#endif
        g_span_overflow = 1;
        return;
    }
    if (ne == 0)
        return;
    *(int*)cur = ne;
    *(int*)(saved + 4) = y;
    keep = 0;
    if (ne > 0) {
        char* dst = saved + 0xa;
        n = ne;
        keep = ne;
        do {
            int idx = keys->idx;
            SpanEdge* e = (SpanEdge*)((char*)edges + idx * 48);
            keys++;
            *(short*)(dst - 2) = (short)(e->a[0] >> 16);
            *(int*)(dst + 2) = e->d[0];
            *(short*)dst = (short)keys[-1].y;
            if (e->dir == 1)
                *(short*)dst = (short)-*(short*)dst;
            dst += 8;
        } while (--n);
    }
    g_span_cursor = *(char* volatile*)&saved + 8 + keep * 8;
}
