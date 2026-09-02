/* LEGOLAND - 3D transform math and person orientation.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; the field and type names are ours.
 * Each matched function carries its original VA marker on the line above it.
 *
 * ---------------------------------------------------------------------------
 * MATRICES
 *
 * Two 3x3 matrix flavours coexist, both ROW-MAJOR, 9 consecutive words:
 *
 *   float   m[9]   MatrixMultiply / BuildYRotationMatrix / CopyMatrix /
 *                  TMNegParity.  MatrixMultiply(a, b, out) computes
 *                  out = a * b with out[i][j] = sum_k a[i][k] * b[k][j].
 *   int     m[9]   16.16 fixed point.  TransformVectorsL (hand-written
 *                  inline asm in the original) applies it to a run of
 *                  {x,y,z} int vectors: dst = M * src, each product an
 *                  imul + shrd 16.  SetPersonRotation builds one of these
 *                  in the 3D person at +0x58.
 *
 * BuildYRotationMatrix(angle, m) is the standard Y rotation
 *   [ c 0 s ; 0 1 0 ; -s 0 c ]
 * so rows are the transformed basis vectors and vectors are treated as
 * columns (M * v).  TMNegParity checks the handedness of a 3x3: the sign
 * of (row0 x row1) . row2, i.e. of the determinant.
 *
 * PERSON ORIENTATION
 *
 * SetPersonDirection maps a 0..7 compass direction to a Y angle (in the
 * order -pi/4, 3pi/2, 5pi/4, pi, 3pi/4, pi/2, pi/4, 0), stores it in the
 * person's rotation vector at +0x40..+0x48 and calls SetPersonRotation,
 * which rebuilds the fixed-point matrix at +0x58 from sin/cos * 65536 and
 * then negates the middle element (a Y flip), giving
 *   [ c 0 s ; 0 -1 0 ; -s 0 c ] in 16.16.
 *
 * SCREEN PROJECTION
 *
 * GetScreenCoordsForObject(instance, item): the FIRST argument carries the
 * map cell as two packed bytes (+0x00 x, +0x01 y); it goes through
 * GetTileBounds (2:1 iso projection, pathbuild.c) for the tile's top-left
 * pixel.  The SECOND argument carries a pixel offset pair at +0x14/+0x18,
 * which is halved (sign-symmetric shift, the same HALF as
 * AdjustOffsetForViewMode) and added.  So those offsets are stored at 2x
 * scale and the view mode halves them.  The result is an 8-byte Offset
 * returned in eax:edx.  ArcTan256 is atan2 in 256ths of a turn (0..255),
 * with the axis case (x == 0) short-circuited to 64 / 192; it rounds with
 * floor(a + 0.5) and the tail (int) conversion is emitted as `jmp __ftol`
 * (the game's own fistp helper at 0x00458930).  GetUnitDepth is a depth
 * scale for a unit step: with t = 49152/(near-far) it returns
 * depth(2)-depth(1), where depth(z) = (z-far)*t + 8192.
 *
 * CODEGEN NOTES (VC6 SP3, /O2 /Gy /Gd)
 *
 * - GetUnitDepth: the two depth() intermediates must be FLOAT locals; the
 *   original's redundant `fxch st1 / fxch st1` before the final fsubp only
 *   appears then (double locals or one big expression drop the pair).
 * - ArcTan256: the accumulator must be a `float` local (fcom/fadd against
 *   DWORD constants 0.0f / 256.0f) while the multiplier 256/(2*PI) and the
 *   rounding 0.5 stay double (QWORD).  PI is 3.1415926 - the folded
 *   constant 40.74366612653722 pins it.  The axis case is written
 *   `y < 0 ? 192 : 64` (VC6 emits setge/dec/and 0x80/add 0x40; the
 *   `y >= 0 ? 64 : 192` spelling produces setl/and al/add 0xc0).
 * - GetScreenCoordsForObject: the two pixel offsets must be read EAGERLY
 *   into int locals before either is halved, otherwise the second load is
 *   sunk past the first halving and the adds are not split per branch.
 * - RenderItem_Link / RenderItem2_Link (byte-identical twins): the
 *   insertion sort must be a `for (;;)` with the insert-before case INSIDE
 *   the loop (`if (key <= node->key) {...; return;}` then `if (!node->next)
 *   break;`) and the append after the loop.  A `while (key > node->key)`
 *   with the append as an early return (or a goto) tail-merges the two
 *   `list->head = item` stores and swaps which exit owns the shared
 *   epilogue.
 * - SetPersonRotation / TransformVectorsL are inline __asm in the original
 *   (ebp frames, fistp without __ftol, push/pop of results into memory).
 *   In SetPersonRotation the WHOLE matrix fill is asm: that is why the
 *   compiler keeps `p` in edi (the only register the asm leaves alone) and
 *   saves ebx/esi/edi in the prologue.  VC6 parks the asm-referenced int
 *   temporaries s and c in the dead parameter home slots [ebp+8]/[ebp+0xc]
 *   and puts angle/scale/m at [ebp-0xc]/[ebp-8]/[ebp-4].
 * - SetPersonDirection: `unsigned int dir` gives cmp/ja + jump table;
 *   VC6 duplicates the SetPersonRotation call into every case, and the
 *   `case 7: = 0.0f` store falls into the default's call.  The angle
 *   constants are single-precision values (pi = 0x40490fd7 = 3.14159179f,
 *   4 ulps below (float)3.1415926), so they are written as exact literals.
 * - MatrixMultiply: the natural row-by-column sum matches; VC6 reorders
 *   the commutative products itself.  NormaliseVector needs the three
 *   components copied into locals (z, y, x) so the length is built from
 *   x87 registers.  CopyMatrix is nine float assignments (VC6 copies
 *   floats through edx/eax, no fld/fstp); a struct assignment would give
 *   rep movsd.
 * --------------------------------------------------------------------------- */
#include <math.h>
#include "legoland.h"

#pragma intrinsic(atan2, sqrt)

#define PI 3.1415926

/* ------------------------------------------------------------------ types -- */

typedef struct Vec3 {
    float x;   /* +0x00 */
    float y;   /* +0x04 */
    float z;   /* +0x08 */
} Vec3;

typedef struct Matrix {
    float m[9];        /* +0x00..+0x20, row-major */
} Matrix;

typedef struct FixedMatrix {
    int m[9];          /* +0x00..+0x20, row-major 16.16 */
} FixedMatrix;

/* The 3D person (blokeai.c's Person3D); only the fields touched here. */
typedef struct Person3D {
    unsigned char pad00[0x40];
    Vec3          rot;         /* +0x40..+0x48  rotation angles (y is used) */
    unsigned char pad4c[0x0c]; /* +0x4c..+0x57 */
    FixedMatrix   matrix;      /* +0x58..+0x78  16.16 orientation */
} Person3D;

/* A loaded position/vertex table: count at +0x04, pointer array at +0x24. */
typedef struct PosTable {
    int     pad00;             /* +0x00 */
    int     count;             /* +0x04 */
    unsigned char pad08[0x1c]; /* +0x08..+0x23 */
    void**  items;             /* +0x24 */
} PosTable;

/* The inclusive pixel rectangle GetTileBounds fills (pathbuild.c). */
typedef struct TileBounds {
    int left;   /* +0x00 */
    int top;    /* +0x04 */
    int right;  /* +0x08 */
    int bottom; /* +0x0c */
} TileBounds;

/* A placed object: its map cell is packed as two bytes at +0x00/+0x01. */
typedef struct MapObject {
    unsigned char bx;   /* +0x00 */
    unsigned char by;   /* +0x01 */
} MapObject;

/* An object instance: pixel offsets (at 2x) at +0x14/+0x18. */
typedef struct ObjInstance {
    unsigned char pad00[0x14];
    int ox;             /* +0x14 */
    int oy;             /* +0x18 */
} ObjInstance;

/* Depth-sorted render list item / holder (renderlist.c). */
typedef struct RenderItem {
    int                 key;      /* +0x00 */
    void*               payload;  /* +0x04 */
    struct RenderItem*  next;     /* +0x08 */
    struct RenderItem*  prev;     /* +0x0c */
} RenderItem;

typedef struct RenderList {
    RenderItem* head;             /* +0x00 */
} RenderList;

/* Skew/depth records for GetZSkew: only the fields read.  The middle
 * parameter is unused.  Result: b10^2 / ((2*a14 - 1)*b10 - b0c*a14). */
typedef struct SkewA {
    unsigned char pad00[0x14];
    float f14;          /* +0x14 */
} SkewA;

typedef struct SkewB {
    unsigned char pad00[0x0c];
    float f0c;          /* +0x0c */
    float f10;          /* +0x10 */
} SkewB;

/* ------------------------------------------------------------ prototypes -- */

extern float Sin(float a);                                  /* 0x00443250 */
extern float Cos(float a);                                  /* 0x00443260 */
extern void  CrossProduct(Vec3* a, Vec3* b, Vec3* out);     /* 0x00442da0 */
extern float DotProduct(Vec3* a, Vec3* b);                  /* 0x00442de0 */
extern void  GetTileBounds(Pos* tile, TileBounds* out);     /* 0x0045acc0 */
extern void  HeapFree_w(void* p);                           /* 0x0049e4d0 */

void SetPersonRotation(Person3D* p, Vec3* rot);

/* Sign-symmetric halving: rounds toward zero on both sides. */
#define HALF(v) ((v) < 0 ? -((-(v)) >> 1) : ((v) >> 1))

/* -------------------------------------------------------------- functions -- */

// FUNCTION: LEGOLAND 0x0044de20
float GetZSkew(SkewA* a, int unused, SkewB* b)
{
    return b->f10 * b->f10 / ((a->f14 + a->f14 - 1.0f) * b->f10 - b->f0c * a->f14);
}

// FUNCTION: LEGOLAND 0x0044de50
float GetUnitDepth(float a, float b)
{
    float t = 49152.0f / (a - b);
    float u = (float)((2.0 - b) * t + 8192.0);
    float v = (float)((1.0 - b) * t + 8192.0);
    return u - v;
}

// FUNCTION: LEGOLAND 0x00443490
void CopyMatrix(Matrix* src, Matrix* dst)
{
    dst->m[0] = src->m[0];
    dst->m[1] = src->m[1];
    dst->m[2] = src->m[2];
    dst->m[3] = src->m[3];
    dst->m[4] = src->m[4];
    dst->m[5] = src->m[5];
    dst->m[6] = src->m[6];
    dst->m[7] = src->m[7];
    dst->m[8] = src->m[8];
}

// FUNCTION: LEGOLAND 0x00442d30
void AdjustOffsetForViewMode(Offset* o)
{
    o->ox = HALF(o->ox);
    o->oy = HALF(o->oy);
}

// FUNCTION: LEGOLAND 0x00443360
void BuildYRotationMatrix(float angle, Matrix* m)
{
    float s = Sin(angle);
    float c = Cos(angle);

    m->m[0] = c;
    m->m[1] = 0.0f;
    m->m[2] = s;
    m->m[3] = 0.0f;
    m->m[4] = 1.0f;
    m->m[5] = 0.0f;
    m->m[6] = -s;
    m->m[7] = 0.0f;
    m->m[8] = c;
}

// FUNCTION: LEGOLAND 0x0043f7d0
void UnloadPos(PosTable* t)
{
    int i;

    for (i = 0; i < t->count; i++)
        HeapFree_w(t->items[i]);
    HeapFree_w(t->items);
    HeapFree_w(t);
}

// FUNCTION: LEGOLAND 0x004806e0
int ArcTan256(int x, int y)
{
    float a;

    if (x == 0)
        return y < 0 ? 192 : 64;

    a = (float)(atan2((double)y, (double)x) * (256.0 / (2 * PI)));
    if (a < 0.0f)
        a += 256.0f;
    return (int)floor(a + 0.5);
}

// FUNCTION: LEGOLAND 0x00443450
void NormaliseVector(Vec3* v)
{
    float z = v->z;
    float y = v->y;
    float x = v->x;
    float len = (float)sqrt(x * x + y * y + z * z);

    v->x /= len;
    v->y /= len;
    v->z /= len;
}

// FUNCTION: LEGOLAND 0x00442cc0
Offset GetScreenCoordsForObject(MapObject* inst, ObjInstance* item)
{
    Pos        pos;
    TileBounds bounds;
    Offset     out;
    int        ox;
    int        oy;

    pos.x = inst->bx;
    pos.y = inst->by;
    GetTileBounds(&pos, &bounds);
    ox = item->ox;
    oy = item->oy;
    ox = HALF(ox);
    oy = HALF(oy);
    out.ox = bounds.left + ox;
    out.oy = bounds.top + oy;
    return out;
}

/* Insertion-sort `item` into the list by DESCENDING key (walk while key is
 * greater; insert before the first node whose key is >= key, or append).
 * `key` is the caller's copy - item->key is never re-read.  RenderItem2_Link
 * is the byte-identical list-2 twin. */
// FUNCTION: LEGOLAND 0x00442eb0
void RenderItem_Link(RenderList* list, RenderItem* item, int key)
{
    RenderItem* node;

    item->next = 0;
    item->prev = 0;
    node = list->head;
    if (!node) {
        list->head = item;
        return;
    }
    for (;;) {
        if (key <= node->key) {
            item->next = node;
            item->prev = node->prev;
            if (node->prev)
                node->prev->next = item;
            node->prev = item;
            if (node == list->head)
                list->head = item;
            return;
        }
        if (!node->next)
            break;
        node = node->next;
    }
    node->next = item;
    item->prev = node;
}

// FUNCTION: LEGOLAND 0x00443080
void RenderItem2_Link(RenderList* list, RenderItem* item, int key)
{
    RenderItem* node;

    item->next = 0;
    item->prev = 0;
    node = list->head;
    if (!node) {
        list->head = item;
        return;
    }
    for (;;) {
        if (key <= node->key) {
            item->next = node;
            item->prev = node->prev;
            if (node->prev)
                node->prev->next = item;
            node->prev = item;
            if (node == list->head)
                list->head = item;
            return;
        }
        if (!node->next)
            break;
        node = node->next;
    }
    node->next = item;
    item->prev = node;
}

/* 1 if the matrix is left-handed (negative determinant): (row0 x row1) . row2
 * < 0.  Rows are copied into three stack vectors because CrossProduct /
 * DotProduct take Vec3 pointers; the row-2 copy reuses vector `a` (the frame
 * is exactly three vectors). */
// FUNCTION: LEGOLAND 0x00442e00
int TMNegParity(Matrix* m)
{
    Vec3 a;
    Vec3 b;
    Vec3 c;

    a.x = m->m[0];
    a.y = m->m[1];
    a.z = m->m[2];
    b.x = m->m[3];
    b.y = m->m[4];
    b.z = m->m[5];
    CrossProduct(&a, &b, &c);
    a.x = m->m[6];
    a.y = m->m[7];
    a.z = m->m[8];
    if (DotProduct(&c, &a) < 0.0)
        return 1;
    return 0;
}

/* Copy the rotation vector into the person and rebuild its 16.16 matrix from
 * rot.y: [c 0 s; 0 1 0; -s 0 c] * 65536, then negate m[4] (+0x68) - the
 * shipped code really does write 0x10000 and immediately flip it. */
// FUNCTION: LEGOLAND 0x00440020
void SetPersonRotation(Person3D* p, Vec3* rot)
{
    float scale = 65536.0f;
    float angle;
    int*  m;
    int   s;
    int   c;

    p->rot.x = rot->x;
    p->rot.y = rot->y;
    p->rot.z = rot->z;
    angle = rot->y;
    m = p->matrix.m;
    __asm {
        push esi
        fld  angle
        fsin
        fmul scale
        fistp s
        fld  angle
        fcos
        fmul scale
        fistp c
        mov  eax, m
        xor  ebx, ebx
        mov  ecx, 10000h
        mov  edx, s
        mov  esi, c
        mov  [eax], esi
        mov  [eax+4], ebx
        mov  [eax+8], edx
        mov  [eax+0ch], ebx
        mov  [eax+10h], ecx
        mov  [eax+14h], ebx
        neg  edx
        mov  [eax+18h], edx
        mov  [eax+1ch], ebx
        mov  [eax+20h], esi
        pop  esi
    }
    p->matrix.m[4] = -p->matrix.m[4];
}

// FUNCTION: LEGOLAND 0x004400b0
void SetPersonDirection(Person3D* p, unsigned int dir)
{
    p->rot.x = 0.0f;
    p->rot.z = 0.0f;
    switch (dir) {
    case 0: p->rot.y = -0.785397947f; break;
    case 1: p->rot.y = 4.71238756f; break;
    case 2: p->rot.y = 3.92698979f; break;
    case 3: p->rot.y = 3.14159179f; break;
    case 4: p->rot.y = 2.35619378f; break;
    case 5: p->rot.y = 1.57079589f; break;
    case 6: p->rot.y = 0.785397947f; break;
    case 7: p->rot.y = 0.0f; break;
    }
    SetPersonRotation(p, &p->rot);
}

/* dst[i] = M * src[i] for `count` {x,y,z} 16.16 vectors; every product is
 * imul (64-bit) then shrd 16.  Hand-written asm in the original: the first
 * two rows are pushed and popped straight into dst. */
// FUNCTION: LEGOLAND 0x004433b0
void TransformVectorsL(int* src, int* dst, int* m, int count)
{
    __asm {
        push esi
        push edi
        mov  ebx, m
        mov  edi, count
    next_vec:
        mov  esi, src
        mov  eax, [ebx]
        imul dword ptr [esi]
        shrd eax, edx, 16
        mov  ecx, eax
        mov  eax, [ebx+4]
        imul dword ptr [esi+4]
        shrd eax, edx, 16
        add  ecx, eax
        mov  eax, [ebx+8]
        imul dword ptr [esi+8]
        shrd eax, edx, 16
        add  ecx, eax
        push ecx
        mov  eax, [ebx+12]
        imul dword ptr [esi]
        shrd eax, edx, 16
        mov  ecx, eax
        mov  eax, [ebx+16]
        imul dword ptr [esi+4]
        shrd eax, edx, 16
        add  ecx, eax
        mov  eax, [ebx+20]
        imul dword ptr [esi+8]
        shrd eax, edx, 16
        add  ecx, eax
        push ecx
        mov  eax, [ebx+24]
        imul dword ptr [esi]
        shrd eax, edx, 16
        mov  ecx, eax
        mov  eax, [ebx+28]
        imul dword ptr [esi+4]
        shrd eax, edx, 16
        add  ecx, eax
        mov  eax, [ebx+32]
        imul dword ptr [esi+8]
        shrd eax, edx, 16
        add  ecx, eax
        mov  esi, dst
        mov  [esi+8], ecx
        pop  dword ptr [esi+4]
        pop  dword ptr [esi]
        add  src, 12
        add  dst, 12
        dec  edi
        jne  next_vec
        pop  edi
        pop  esi
    }
}

// FUNCTION: LEGOLAND 0x00443270
void MatrixMultiply(Matrix* a, Matrix* b, Matrix* out)
{
    out->m[0] = a->m[0] * b->m[0] + a->m[1] * b->m[3] + a->m[2] * b->m[6];
    out->m[1] = a->m[0] * b->m[1] + a->m[1] * b->m[4] + a->m[2] * b->m[7];
    out->m[2] = a->m[0] * b->m[2] + a->m[1] * b->m[5] + a->m[2] * b->m[8];
    out->m[3] = a->m[3] * b->m[0] + a->m[4] * b->m[3] + a->m[5] * b->m[6];
    out->m[4] = a->m[3] * b->m[1] + a->m[4] * b->m[4] + a->m[5] * b->m[7];
    out->m[5] = a->m[3] * b->m[2] + a->m[4] * b->m[5] + a->m[5] * b->m[8];
    out->m[6] = a->m[6] * b->m[0] + a->m[7] * b->m[3] + a->m[8] * b->m[6];
    out->m[7] = a->m[6] * b->m[1] + a->m[7] * b->m[4] + a->m[8] * b->m[7];
    out->m[8] = a->m[6] * b->m[2] + a->m[7] * b->m[5] + a->m[8] * b->m[8];
}
