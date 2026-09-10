/* LEGOLAND -- coaster matrix, vector, clipping and joint micro-helpers.
 * VC6 SP3 /O2 /Gy /Gd. Types are local; offsets describe the original ABI.
 * FastSqrt/FastRSqrt use the original non-C, ST(0)-in/ST(0)-out convention.
 */

typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct Mat3 { float m[9]; } Mat3;
typedef struct Mat4 { float m[16]; } Mat4;
typedef struct PackedSquare { short x, y; } PackedSquare;
typedef struct TrackNode { int state; PackedSquare sq; } TrackNode;
typedef struct JointParams { int dir; PackedSquare offset; } JointParams;
typedef struct RideElem { int unused[3]; void* data; } RideElem;
typedef struct CtIface { RideElem* elem; void* hooks[5]; } CtIface;
typedef struct SpanRect { int top, left, bottom, right; } SpanRect;
typedef struct SqrtEnt { float add, mul; } SqrtEnt;
#ifndef LEGOLAND_PORTABLE
#define NAKED __declspec(naked)
#else
#define NAKED
#endif

extern CtIface g_ct_iface[6];                                  /* 0x0082ad20 */
extern void* g_span_context;                                   /* 0x004b55fc */
extern SqrtEnt g_sqrt_tab[64];                                 /* 0x00610c40 */
extern float g_sqrt_exp[256];                                  /* 0x00610e44 */
extern SqrtEnt g_rsqrt_tab[64];                                /* 0x00610a20 */
extern float g_rsqrt_exp[256];                                 /* 0x00611244 */
extern void Span_SetClip(const SpanRect* rect, void* context);  /* 0x0041f380 */
extern void Mat3_ToMat4(const Mat3* src, Mat4* dst);            /* 0x00426490 */
extern void ClearJointHeight(void* slot);                      /* 0x0041d1b0 */
extern void JointSlot_Set(const PackedSquare* sq, void* slot,
                          int direction);                    /* 0x0041cd80 */
extern float VecMath_ReciprocalSqrt(float value);              /* 0x00426960 */

/* The extent walker stops at the internal jump at +0x1a. The actual body
 * continues through 0x00426186 (40 instructions, 103 bytes). Keep WIP even
 * if an iteration comparison happens to report an exact prefix. */
// FUNCTION: LEGOLAND 0x00426120
void MatMul(const Mat4* a, const Mat4* b, Mat4* out)
{
    int row, col, k;
    for (row = 0; row < 4; row++) {
        for (col = 0; col < 4; col++) {
            float sum = 0.0f;
            for (k = 0; k < 4; k++)
                sum += b->m[k * 4 + col] * a->m[row * 4 + k];
            out->m[row * 4 + col] = sum;
        }
    }
}

// FUNCTION: LEGOLAND 0x00425d30
float Vec3Dot(const Vec3f* a, const Vec3f* b)
{
    return b->x * a->x + (b->y * a->y + b->z * a->z);
}

// FUNCTION: LEGOLAND 0x004260f0
void MatIdentity(Mat4* out)
{
    int row, col;
    for (row = 0; row <= 3; row++) {
        for (col = 0; col <= 3; col++) {
            out->m[row * 4 + col] = 0.0f;
            if (row == col)
                out->m[row * 4 + col] = 1.0f;
        }
    }
}

// FUNCTION: LEGOLAND 0x0041ef20
void SetSpanClip(int left, int top, int right, int bottom)
{
    SpanRect rect;
    rect.top = top;
    rect.left = left;
    rect.right = right;
    rect.bottom = bottom;
    Span_SetClip(&rect, g_span_context);
}

// FUNCTION: LEGOLAND 0x004264e0
void MakeTransform(const Vec3f* pos, const Mat3* rot, Mat4* out)
{
    Mat3_ToMat4(rot, out);
    out->m[3] = pos->x;
    out->m[7] = pos->y;
    out->m[11] = pos->z;
}

/* Zero is also returned for an unknown class, exactly as in the original. */
// FUNCTION: LEGOLAND 0x0041ebd0
int PackTrackClass(void* cls)
{
    int i;
    for (i = 0; i < 6; i++) {
        RideElem* elem = g_ct_iface[i].elem;
        if (elem && cls == elem->data)
            return i;
    }
    return 0;
}

/* These are hand-written assembly, not ordinary C float functions: there
 * is no argument slot or prologue, input arrives in ST(0), and three GPRs
 * are saved below ESP without reserving a frame. Preserve that original
 * scratch-stack convention and the unmasked sign bit in the exponent index.
 */
// FUNCTION: LEGOLAND 0x00426ab0
NAKED void FastSqrt(void)
{
#ifndef LEGOLAND_PORTABLE
    __asm {
        fstp dword ptr [esp-10h]
        mov [esp-8], ebx
        mov [esp-0ch], edx
        mov edx, [esp-10h]
        mov ebx, 7fffffh
        mov [esp-4], eax
        mov eax, edx
        shr eax, 11h
        and ebx, edx
        and eax, 3fh
        or ebx, 3f800000h
        mov [esp-10h], ebx
        mov ebx, [esp-8]
        fld dword ptr [esp-10h]
        fmul dword ptr [g_sqrt_tab+eax*8+4]
        shr edx, 17h
        fadd dword ptr [g_sqrt_tab+eax*8]
        mov eax, [esp-4]
        fmul dword ptr [g_sqrt_exp+edx*4]
        mov edx, [esp-0ch]
        ret
    }
#else
    LL_UNPORTED_ASM(); /* ST(0) ABI; the portable build calls ll_FastSqrt */
#endif
}

// FUNCTION: LEGOLAND 0x00426980
NAKED void FastRSqrt(void)
{
#ifndef LEGOLAND_PORTABLE
    __asm {
        fstp dword ptr [esp-10h]
        mov [esp-8], ebx
        mov [esp-0ch], edx
        mov edx, [esp-10h]
        mov ebx, 7fffffh
        mov [esp-4], eax
        mov eax, edx
        shr eax, 11h
        and ebx, edx
        and eax, 3fh
        or ebx, 3f800000h
        mov [esp-10h], ebx
        mov ebx, [esp-8]
        fld dword ptr [esp-10h]
        fmul dword ptr [g_rsqrt_tab+eax*8+4]
        shr edx, 17h
        fadd dword ptr [g_rsqrt_tab+eax*8]
        mov eax, [esp-4]
        fmul dword ptr [g_rsqrt_exp+edx*4]
        mov edx, [esp-0ch]
        ret
    }
#else
    LL_UNPORTED_ASM(); /* ST(0) ABI; the portable build calls ll_FastRSqrt */
#endif
}

// FUNCTION: LEGOLAND 0x00425de0
void Invert2x2(float* m)
{
    float inv = 1.0f / (m[3] * m[0] - m[2] * m[1]);
    float first = m[0];
    m[0] = m[3] * inv;
    m[3] = first * inv;
    m[1] = -(m[1] * inv);
    m[2] = -(m[2] * inv);
}

// FUNCTION: LEGOLAND 0x0041d1d0
void BuildJoint(TrackNode* node, const JointParams* params, void* slot)
{
    PackedSquare sq;
    ClearJointHeight(slot);
    sq.x = node->sq.x + params->offset.x;
    sq.y = node->sq.y + params->offset.y;
    JointSlot_Set(&sq, slot, params->dir);
}

/* Normalizes in place using the module's reciprocal-square-root hook. */
// FUNCTION: LEGOLAND 0x00425d50
void Vec3_Normalize(Vec3f* vec)
{
    float length2 = 0.0f;
    float inv;
    float* v = (float*)vec;
    int i;
    for (i = 0; i < 3; i++)
        length2 += v[i] * v[i];
    inv = VecMath_ReciprocalSqrt(length2);
    for (i = 0; i < 3; i++)
        v[i] *= inv;
}

#ifdef LEGOLAND_PORTABLE
/* C-ABI versions of the ST(0) helpers above; coaster5.c installs them in
 * g_fast_sqrt / g_fast_rsqrt. The original indexes the exponent table with
 * the unmasked sign bit (a negative input reads past it); the port masks. */
float ll_FastSqrt(float value)
{
    unsigned int bits, mant;
    float        m;
    __builtin_memcpy(&bits, &value, 4);
    mant = (bits & 0x7fffffu) | 0x3f800000u;
    __builtin_memcpy(&m, &mant, 4);
    return (m * g_sqrt_tab[(bits >> 17) & 0x3f].mul + g_sqrt_tab[(bits >> 17) & 0x3f].add)
           * g_sqrt_exp[(bits >> 23) & 0xff];
}

float ll_FastRSqrt(float value)
{
    unsigned int bits, mant;
    float        m;
    __builtin_memcpy(&bits, &value, 4);
    mant = (bits & 0x7fffffu) | 0x3f800000u;
    __builtin_memcpy(&m, &mant, 4);
    return (m * g_rsqrt_tab[(bits >> 17) & 0x3f].mul + g_rsqrt_tab[(bits >> 17) & 0x3f].add)
           * g_rsqrt_exp[(bits >> 23) & 0xff];
}
#endif
