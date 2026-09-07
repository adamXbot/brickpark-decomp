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
};

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

extern int JointOppositeDir(int dir);           /* 0x0041cc50 */
extern void Curve_InitLine(void* dest, const Vec3f* from,
    const Vec3f* to, const Vec3f* offset);      /* 0x00421ab0 */
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
// FUNCTION: LEGOLAND 0x00427c70
void TrackNode_LoadDescHeights(TrackNode* node)
{
    TrackDesc* desc = node->desc;
    node->jout.h = (float)desc->h0;
    node->jin.h = (float)desc->h1;
}

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

/* src = arg0, dest = arg1. Column-read / row-write is a transpose.
 * Residual: dest cursor in ecx and source cursor in eax; original has the
 * pair swapped. Structure, counts and callee-saved set are right (15/21). */
// WIP-FUNCTION: LEGOLAND 0x00426190  (71.4%, dest/src cursors swapped in eax/ecx)
void Mat4_Transpose(const float* src, float* dst)
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
}

/* dest = arg0, src = arg1. 3x3 of a 4x4, same transpose walk.
 * Same eax/ecx cursor swap as Mat4_Transpose (15/21). */
// WIP-FUNCTION: LEGOLAND 0x00426460  (71.4%, dest/src cursors swapped in eax/ecx)
void Mat3_FromMat4Transpose(float* dst, const float* src)
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

/* Clip dest against clip; 0 if they miss, 0xf (all four edges) after clamp.
 * ClipRect_SetBounds (0x00426700) stores the return in dest->mask.
 * Residual: the two argument-pointer loads are swapped (clip should be
 * eax first). Registers, tests, clamps and epilogues are exact (65/66). */
// WIP-FUNCTION: LEGOLAND 0x004265d0  (98.5%, arg-pointer load order)
int ClipRect_ClipTo(ClipBox* dest, const ClipBox* clip)
{
    int left = dest->left;
    int right;
    int top;
    int bottom;
    if (left > clip->right)
        return 0;
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

/* Project n Vec3f through the first two rows of a 4x4 (screen x,y) and
 * expand an integer AABB. The 0.0 at 0x004ab390 is the sum starter. */
// WIP-FUNCTION: LEGOLAND 0x004263a0
void ProjectVertsToRect(const Vec3f* verts, const Mat4* mat, int n, ClipBox* out)
{
    int screen[2];
    out->right = (int)0x80000000;
    out->bottom = (int)0x80000000;
    out->left = 0x7fffffff;
    out->top = 0x7fffffff;
    if (n <= 0)
        return;
    do {
        const float* row = mat->m;
        const float* v = (const float*)verts;
        int axis;
        for (axis = 0; axis < 2; axis++) {
            float s = 0.0f;
            int k;
            for (k = 0; k < 3; k++)
                s += v[k] * row[k];
            s += row[3];
            screen[axis] = (int)s;
            row += 4;
        }
        if (screen[0] > out->right)
            out->right = screen[0];
        if (screen[0] < out->left)
            out->left = screen[0];
        if (screen[1] > out->bottom)
            out->bottom = screen[1];
        if (screen[1] < out->top)
            out->top = screen[1];
        verts++;
    } while (--n);
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
