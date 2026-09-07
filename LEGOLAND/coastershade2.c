/* LEGOLAND -- coaster shade / blit table callees (inventory group 4).
 * VC6 SP3 /O2 /Gy /Gd. Types are local; offsets describe the original ABI.
 * Scope LL4 (docs/SCOPE_LL4_coaster_shades.md). Notes: docs/lanes/scope-ll4.md.
 */
#include <math.h>
#pragma intrinsic(fabs)

typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct TrackNode TrackNode;
typedef struct RouteGeom RouteGeom;

struct TrackNode {
    int flags;                 /* +00 */
    unsigned int square;       /* +04 */
    void* cls;                 /* +08 */
    void* desc;                /* +0c */
    void* owner;               /* +10 */
    int jin[2];                /* +14 */
    TrackNode* prev;           /* +1c */
    int jout[2];               /* +20 */
    TrackNode* next;           /* +28 */
};

/* Render-object / geometry link. Advance walks +0x50; retreat walks +0x54
 * and, on a node change, the last +0x50 link of the previous piece. */
struct RouteGeom {
    unsigned char pad00[0x50];
    RouteGeom* next;           /* +50 */
    RouteGeom* prev;           /* +54 */
};

typedef struct RoutePos {
    TrackNode* node;           /* +00 */
    RouteGeom* geom;           /* +04 */
    Vec3f pos;                 /* +08 */
} RoutePos;

typedef struct SortKey { int y; int idx; } SortKey;
typedef struct SpanEdge {
    short y0;                  /* +00 */
    short y1;                  /* +02 */
    int dir;                   /* +04 */
    int a[5];                  /* +08 */
    int d[5];                  /* +1c */
} SpanEdge;                    /* 0x30 */
typedef struct SpanInterp { int x, r4, z, r12, r16; } SpanInterp; /* 0x14 */

typedef struct PhysVec { int n; float v[1]; } PhysVec;
typedef struct PhysOps {
    void (*add)(const PhysVec* a, const PhysVec* b, PhysVec* out); /* +00 */
    void (*sub)(const PhysVec* a, const PhysVec* b, PhysVec* out); /* +04 */
    void* op2;                                                     /* +08 */
    void (*scale)(PhysVec* v, float k);                            /* +0c */
    void* op4;                                                     /* +10 */
    void* op5;                                                     /* +14 */
    void* op6;                                                     /* +18 */
    void* op7;                                                     /* +1c */
    PhysVec** (*alloc)(int n);                                     /* +20 */
    void (*release)(PhysVec** a, int n);                           /* +24 */
} PhysOps;
typedef void (*RombergFn)(float t, PhysVec* out);

extern void* g_coaster_tab_c[];                                /* 0x004d89c8 */
extern unsigned short* g_shade_tab[0x400];                     /* 0x00829c60 */
extern short* g_raster_bits;                                   /* 0x004b5b20 */
extern short* g_zb_base;                                       /* 0x004b5b24 */
extern int g_zb_pitch;                                         /* 0x004b5b28 */
extern int g_zb_polys;                                         /* 0x0060f900 */
extern float g_one_sixth;                                      /* 0x004b5610 */
extern float g_half;                                           /* 0x004ab3d0 */
extern float g_two;                                            /* 0x004ab3d8 */
extern float g_three;                                          /* 0x004ab43c */
extern float g_four;                                           /* 0x004ab3d4 */
extern float g_third;                                          /* 0x004ab438 */
extern unsigned char* g_shade_clamp_mid;                       /* 0x004d89c4 */
extern unsigned short* g_span_ramp;                            /* 0x004d89c0 */
extern int g_span_dshade_hi;                                   /* 0x004d89b4 */
extern int g_span_dshade_lo;                                   /* 0x004d89bc */
extern RouteGeom* GetTrackNodeWorldPos(TrackNode* node, Vec3f* out); /* 0x0041cff0 */
extern PhysVec*** Romberg_Build(RombergFn fn, PhysOps* ops, int n,
                                float t, float h);              /* 0x0041f3e0 */
extern void Romberg_Release(PhysVec*** tab, PhysOps* ops, int n); /* 0x0041f4c0 */

/* Indexed reader for the .ltx table LoadCoasterData fills through
 * CoasterModel_LoadLTX. Sibling of LoadCoasterMesh / LoadCoasterMeshTex /
 * GetCoasterModelSize, which look up a name first; this one takes the
 * part index FindCoasterPart already resolved. */
// FUNCTION: LEGOLAND 0x00420780
void* GetCoasterTexture(int index)
{
    return g_coaster_tab_c[index];
}

/* Inverse of TrackCursor_AdvanceGeometry: one step back along the piece's
 * geometry chain. A live prev is stored and the function returns; falling
 * off the first object moves onto the previous track piece (via +0x1c) and
 * then walks that piece's +0x50 chain to its last object. */
// FUNCTION: LEGOLAND 0x0041f880
void TrackCursor_RetreatGeometry(RoutePos* cursor)
{
    RouteGeom* geom = cursor->geom->prev;
    if (geom) {
        cursor->geom = geom;
        return;
    }
    cursor->node = cursor->node->prev;
    geom = GetTrackNodeWorldPos(cursor->node, &cursor->pos);
    cursor->geom = geom;
    if (geom->next) {
        do {
            cursor->geom = cursor->geom->next;
        } while (cursor->geom->next);
    }
}

/* Flat shade-table span filler, table slot 0x004b5648. Sibling of
 * schoolcar6.c's ZBuffer_FillPoly: same 0x14 interpolant array, same
 * hand-written fill (`xchg`, `add ebx,1`, `jns/jmp`), but the destination
 * is g_raster_bits and the pixel comes from g_shade_tab[tag][grad[0]].
 * g_zb_base is loaded into a local and never read (the dead store). */
// WIP-FUNCTION: LEGOLAND 0x0041f8d0  (106/106i, 309/307B, 88 mismatch; 0x58 vs 0x60 frame)
void Span_FillFlat(int tag, int* grad, int n, SortKey* key, SpanEdge* edge)
{
    SpanInterp ed[4];
    int        dead;
    short*     row;
    int        pitch;
    int        color;
    int        y;
    int        ylast;

    y = key[0].y;
    edge[key[n - 1].idx].y1++;
    color = (int)g_shade_tab[tag][grad[0]];
    pitch = g_zb_pitch;
    ylast = edge[key[n - 1].idx].y1;
    g_zb_polys++;
    key[n].y = edge[key[n - 1].idx].y1;
    row = g_raster_bits + pitch * *(int volatile*)&y;
    *(int volatile*)&dead = (int)g_zb_base;
    do {
        SpanEdge* e = &edge[key->idx];

        key++;
        if (e->dir) {
            ed[3].x = e->d[0];
            ed[2].x = e->a[0] - e->d[0];
        } else {
            ed[1].x = e->d[0];
            ed[0].x = e->a[0] - e->d[0];
        }
        while (y < key->y) {
            y++;
            __asm {
                mov  eax, ed[0]
                mov  ebx, ed[40]
                add  eax, ed[20]
                add  ebx, ed[60]
                mov  ed[0], eax
                mov  ed[40], ebx
                mov  ecx, ebx
                sub  ecx, eax
                cmp  ecx, 8000h
                jns  wide
                jmp  done
            wide:
                sar  eax, 16
                sar  ebx, 16
                mov  edi, row
                xchg ebx, eax
                mov  dx, word ptr color
                sub  ebx, eax
                lea  edi, [edi + eax*2]
            fill:
                mov  word ptr [edi + ebx*2], dx
                add  ebx, 1
                jle  fill
            done:
            }
            row += pitch;
        }
    } while (y < ylast);
}

/* Flat shade + Z, table slot 0x004b564c. Same sentinel and 0x14 interpolants
 * as Span_FillFlat; the left edge also carries z, and each pixel is written
 * only when its interpolated z is not behind the z-buffer. */
// WIP-FUNCTION: LEGOLAND 0x0041fba0  (136/136i, 398/399B, 62 mismatch; row homes in arg slots)
void Span_FillFlatZ(int tag, int* grad, int n, SortKey* key, SpanEdge* edge)
{
    SpanInterp ed[4];
    short*     crow;
    short*     zrow;
    int        pitch;
    int        color;
    int        y;
    int        ylast;
    int        dz;

    y = key[0].y;
    edge[key[n - 1].idx].y1++;
    dz = grad[1];
    ylast = edge[key[n - 1].idx].y1;
    pitch = g_zb_pitch;
    g_zb_polys++;
    color = (int)g_shade_tab[tag][grad[0]];
    key[n].y = edge[key[n - 1].idx].y1;
    crow = g_raster_bits + pitch * y;
    zrow = g_zb_base + pitch * y;
    do {
        SpanEdge* e = &edge[key->idx];

        key++;
        if (e->dir) {
            ed[3].x = e->d[0];
            ed[2].x = e->a[0] - e->d[0];
        } else {
            ed[1].x = e->d[0];
            ed[1].z = e->d[1];
            ed[0].x = e->a[0] - e->d[0];
            ed[0].z = e->a[1] - e->d[1];
        }
        while (y < key->y) {
            y++;
            __asm {
                mov  eax, ed[0]
                mov  edx, ed[8]
                mov  ebx, ed[40]
                add  eax, ed[20]
                add  edx, ed[28]
                add  ebx, ed[60]
                mov  ed[0], eax
                mov  ed[8], edx
                mov  ed[40], ebx
                mov  ecx, ebx
                sub  ecx, eax
                cmp  ecx, 8000h
                jns  wide
                jmp  done
            wide:
                sar  eax, 16
                sar  ebx, 16
                mov  edi, crow
                mov  esi, zrow
                xchg ebx, eax
                sub  ebx, eax
                lea  edi, [edi + eax*2]
                lea  esi, [esi + eax*2]
            fill:
                mov  ax, word ptr color
                mov  ecx, edx
                sar  ecx, 16
                cmp  cx, word ptr [esi + ebx*2]
                jb   skip
                mov  word ptr [edi + ebx*2], ax
                mov  word ptr [esi + ebx*2], cx
            skip:
                add  edx, dz
                add  ebx, 1
                jle  fill
            done:
            }
            crow += pitch;
            zrow += pitch;
        }
    } while (y < ylast);
}

/* Gouraud shade, no Z, table slot 0x004b5658 (g_span_fillers[0]). Shade is
 * 16.16, clamped through g_shade_clamp_mid and looked up in the tag's ramp.
 * A negative dshade is negated and the inner loop uses sbb instead of adc. */
// WIP-FUNCTION: LEGOLAND 0x0041fd80  (162/162i, 507/505B, 106 mismatch; row home in arg slot)
void Span_FillShade(int tag, int* grad, int n, SortKey* key, SpanEdge* edge)
{
    SpanInterp ed[4];
    int        dead;
    short*     row;
    int        pitch;
    int        y;
    int        ylast;
    int        flip;

    y = key[0].y;
    flip = 0;
    edge[key[n - 1].idx].y1++;
    row = g_raster_bits;
    ylast = edge[key[n - 1].idx].y1;
    *(int volatile*)&dead = (int)g_zb_base;
    pitch = g_zb_pitch;
    g_span_ramp = g_shade_tab[tag];
    g_zb_polys++;
    key[n].y = edge[key[n - 1].idx].y1;
    if (grad[1] < 0) {
        grad[1] = -grad[1];
        flip = 1;
    }
    g_span_dshade_hi = grad[1] >> 16;
    g_span_dshade_lo = grad[1] << 16;
    row = row + pitch * y;
    do {
        SpanEdge* e = &edge[key->idx];

        key++;
        if (e->dir) {
            ed[3].x = e->d[0];
            ed[3].r4 = e->d[1];
            ed[2].x = e->a[0] - e->d[0];
            ed[2].r4 = e->a[1] - e->d[1];
        } else {
            ed[1].x = e->d[0];
            ed[1].r4 = e->d[1];
            ed[0].x = e->a[0] - e->d[0];
            ed[0].r4 = e->a[1] - e->d[1];
        }
        while (y < key->y) {
            y++;
            __asm {
                mov  eax, ed[0]
                mov  ebx, ed[40]
                add  eax, ed[20]
                add  ebx, ed[60]
                mov  ed[0], eax
                mov  ed[40], ebx
                mov  ecx, ebx
                sub  ecx, eax
                cmp  ecx, 8000h
                jns  wide
                mov  ecx, ed[4]
                add  ecx, ed[24]
                mov  ed[4], ecx
                jmp  done
            wide:
                sar  eax, 16
                sar  ebx, 16
                mov  edi, row
                xchg ebx, eax
                sub  ebx, eax
                lea  edi, [edi + eax*2]
                mov  ecx, ed[4]
                xor  edx, edx
                add  ecx, ed[24]
                mov  esi, g_span_ramp
                mov  ed[4], ecx
                ror  ecx, 16
                mov  eax, ecx
                and  ecx, 0ffffh
                and  eax, 0ffff0000h
                add  ecx, g_shade_clamp_mid
                cmp  edx, flip
                jne  negfill
            posfill:
                xor  edx, edx
                mov  dl, byte ptr [ecx]
                mov  dx, word ptr [esi + edx*2]
                add  eax, g_span_dshade_lo
                mov  word ptr [edi + ebx*2], dx
                adc  ecx, g_span_dshade_hi
                xor  edx, edx
                add  ebx, 1
                jle  posfill
                jmp  done
            negfill:
                xor  edx, edx
                mov  dl, byte ptr [ecx]
                mov  dx, word ptr [esi + edx*2]
                add  eax, g_span_dshade_lo
                mov  word ptr [edi + ebx*2], dx
                sbb  ecx, g_span_dshade_hi
                xor  edx, edx
                add  ebx, 1
                jle  negfill
            done:
            }
            row += pitch;
        }
    } while (y < ylast);
}

/* Gouraud shade + Z, table slot 0x004b565c (g_span_fillers[1]). Left edge
 * carries x, shade and z; dshade_lo packs the shade fraction with the
 * z-step so one add advances both. The inner loop steals EBP for the ramp. */
// WIP-FUNCTION: LEGOLAND 0x0041ff80  (202/202i, 633/633B, 62 mismatch; row homes vs original frame)
void Span_FillShadeZ(int tag, int* grad, int n, SortKey* key, SpanEdge* edge)
{
    SpanInterp ed[4];
    short*     crow;
    short*     zrow;
    int        pitch;
    int        y;
    int        ylast;
    int        flip;

    y = key[0].y;
    flip = 0;
    edge[key[n - 1].idx].y1++;
    crow = g_raster_bits;
    zrow = g_zb_base;
    ylast = edge[key[n - 1].idx].y1;
    pitch = g_zb_pitch;
    g_span_ramp = g_shade_tab[tag];
    g_zb_polys++;
    key[n].y = edge[key[n - 1].idx].y1;
    if (grad[1] < 0) {
        grad[1] = -grad[1];
        flip = 1;
    }
    g_span_dshade_hi = grad[1] >> 16;
    g_span_dshade_lo = (grad[1] & 0xffffff00) << 16;
    g_span_dshade_lo |= (grad[2] >> 8) & 0xffffff;
    crow = crow + pitch * y;
    zrow = zrow + pitch * y;
    do {
        SpanEdge* e = &edge[key->idx];

        key++;
        if (e->dir) {
            ed[3].x = e->d[0];
            ed[3].r4 = e->d[1];
            ed[2].x = e->a[0] - e->d[0];
            ed[2].r4 = e->a[1] - e->d[1];
        } else {
            ed[1].x = e->d[0];
            ed[1].r4 = e->d[1];
            ed[1].z = e->d[2];
            ed[0].x = e->a[0] - e->d[0];
            ed[0].r4 = e->a[1] - e->d[1];
            ed[0].z = e->a[2] - e->d[2];
        }
        while (y < key->y) {
            y++;
            __asm {
                mov  eax, ed[0]
                mov  edx, ed[8]
                mov  ebx, ed[40]
                add  eax, ed[20]
                add  edx, ed[28]
                add  ebx, ed[60]
                mov  ed[0], eax
                mov  ed[8], edx
                mov  ed[40], ebx
                mov  ecx, ebx
                sub  ecx, eax
                cmp  ecx, 8000h
                jns  wide
                mov  ecx, ed[4]
                add  ecx, ed[24]
                mov  ed[4], ecx
                jmp  done
            wide:
                sar  eax, 16
                sar  ebx, 16
                mov  edi, crow
                mov  esi, zrow
                xchg ebx, eax
                sub  ebx, eax
                lea  edi, [edi + eax*2]
                lea  esi, [esi + eax*2]
                mov  ecx, ed[4]
                mov  eax, edx
                add  ecx, ed[24]
                sar  eax, 8
                mov  ed[4], ecx
                and  eax, 0ffffffh
                xor  edx, edx
                sar  ecx, 16
                add  ecx, g_shade_clamp_mid
                cmp  edx, flip
                jne  negfill
                push ebp
                mov  ebp, g_span_ramp
            posfill:
                xor  edx, edx
                mov  edx, eax
                sar  edx, 8
                cmp  dx, word ptr [esi + ebx*2]
                jb   posskip
                mov  word ptr [esi + ebx*2], dx
                xor  edx, edx
                mov  dl, byte ptr [ecx]
                mov  dx, word ptr [ebp + edx*2]
                mov  word ptr [edi + ebx*2], dx
            posskip:
                add  eax, g_span_dshade_lo
                adc  ecx, g_span_dshade_hi
                and  eax, 0feffffffh
                add  ebx, 1
                jle  posfill
                pop  ebp
                jmp  done
            negfill:
                push ebp
                mov  ebp, g_span_ramp
            neglp:
                mov  edx, eax
                sar  edx, 8
                cmp  dx, word ptr [esi + ebx*2]
                jb   negskip
                mov  word ptr [esi + ebx*2], dx
                xor  edx, edx
                mov  dl, byte ptr [ecx]
                mov  dx, word ptr [ebp + edx*2]
                mov  word ptr [edi + ebx*2], dx
            negskip:
                add  eax, g_span_dshade_lo
                sbb  ecx, g_span_dshade_hi
                and  eax, 0feffffffh
                add  ebx, 1
                jle  neglp
                pop  ebp
            done:
            }
            crow += pitch;
            zrow += pitch;
        }
    } while (y < ylast);
}

/* Adaptive Simpson quadrature. Track_MeasureDistance (0x0042a1b0) drives
 * this. s holds fa+fb+4*odds+2*evens; approx is s*h; the return scales by
 * 1/3. The empty __asm forces the original's EBP frame so every temporary
 * stays in the 0x28 home list. */
typedef struct SimpsonFrame {
    float fa;
    int   i;
    float x;
    float step;
    int   n;
    float odd;
    float approx;
    float h;
    float old;
    float s;
} SimpsonFrame;

// WIP-FUNCTION: LEGOLAND 0x00420200  (81/81i, 251/258B, 61 mismatch; add esp merged, for-latch is jle not jg)
float IntegrateSimpson(float (*fn)(float), float a, float b, float tol)
{
    volatile SimpsonFrame f;

    __asm {}
    f.n = 1;
    f.h = b - a;
    f.fa = fn(a);
    f.s = f.fa + fn(b);
    f.odd = 0.0f;
    f.approx = f.s * f.h * g_half * g_three;
    do {
        f.old = f.approx;
        f.s = f.s - g_two * f.odd;
        f.odd = 0.0f;
        f.step = f.h;
        f.h = g_half * f.h;
        f.x = a + f.h;
        for (f.i = 1; f.i <= f.n; f.i = f.i + 1) {
            f.odd += fn(f.x);
            f.x += f.step;
        }
        f.s = f.s + g_four * f.odd;
        f.approx = f.s * f.h;
        f.n = f.n + f.n;
    } while ((float)fabs(f.approx - f.old) > tol * (float)fabs(f.old));
    return g_third * f.approx;
}

/* 4-level Romberg combine. Route_GetMassAndPower (0x0041db90) calls this
 * with the 0x0041db20 callback, the 0x004d8270 PhysOps block, the live
 * route parameter, dt=0.1 and an output vector. 0x0041f3e0 / 0x0041f4c0
 * (LL3) build and free the tableau. */
// FUNCTION: LEGOLAND 0x0041f4e0
int Romberg_Evaluate(RombergFn fn, PhysOps* ops, float t, float h, PhysVec* out)
{
    float half = h * 0.5f;
    PhysVec*** tab = Romberg_Build(fn, ops, 4, t, half);
    PhysVec** pair = ops->alloc(2);
    PhysVec* a;
    PhysVec* b;

    a = pair[0];
    b = pair[1];
    if (!tab)
        return 0;
    ops->add(tab[1][1], tab[1][2], a);
    ops->add(tab[3][0], tab[3][1], b);
    ops->scale(b, g_one_sixth);
    ops->add(a, b, out);
    ops->scale(out, 0.5f / half);
    ops->release(pair, 2);
    Romberg_Release(tab, ops, 4);
    return 1;
}
