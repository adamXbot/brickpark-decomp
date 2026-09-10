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
extern PhysVec*** Romberg_Build(RombergFn fn, PhysOps* ops, int n, float t, float h); /* 0x0041f3e0 */
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
 * g_zb_base is loaded into a local and never read (the dead store).
 *
 * FRAME HOMES (LL23, from the closed ZBuffer_FillPoly): the row pointer,
 * the dead store and the pitch must be ONE aggregate local.  An aggregate
 * blocks reuse of a dead PARAMETER's home, so the three take real frame
 * slots at -0x10/-0xc/-8 and the frame is 0x60, not 0x58; `ylast` keeps
 * -4 and `color`/`y` land in the n / key argument slots, which is the
 * original layout.  `color` must be a `short`: VC6 then emits the
 * original's partial write (`mov ax, word ptr [tab+idx*2]` followed by a
 * DWORD store of eax, upper half left as grad[0]'s), where an `int`
 * costs an extra `xor`.  The interpolant pair is written store, store,
 * read-modify-write through the address-taken ed[]: that hoists e->a[0]
 * above the e->dir branch and store-to-load forwarding folds the RMW back
 * into one subtract, so both arms share the loads and duplicate the sub.
 * With all three the old free `volatile` reads of `y` and `dead` are no
 * longer wanted -- each costs an instruction. */
// FUNCTION: LEGOLAND 0x0041f8d0
void Span_FillFlat(int tag, int* grad, int n, SortKey* key, SpanEdge* edge)
{
    SpanInterp ed[4];
    struct { short* row; int dead; int pitch; } r;
    short      color;
    int        y;
    int        ylast;

    y = key[0].y;
    edge[key[n - 1].idx].y1++;
    r.row = g_raster_bits;
    ylast = edge[key[n - 1].idx].y1;
    r.dead = (int)g_zb_base;
    r.pitch = g_zb_pitch;
    g_zb_polys++;
    color = (short)g_shade_tab[tag][grad[0]];
    key[n].y = edge[key[n - 1].idx].y1;
    r.row += r.pitch * y;
    do {
        SpanEdge* e = &edge[key->idx];

        key++;
        if (e->dir) {
            ed[2].x = e->a[0];
            ed[3].x = e->d[0];
            ed[2].x -= ed[3].x;
        } else {
            ed[0].x = e->a[0];
            ed[1].x = e->d[0];
            ed[0].x -= ed[1].x;
        }
        while (y < key->y) {
            y++;
#ifndef LEGOLAND_PORTABLE
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
                mov  edi, r.row
                xchg ebx, eax
                mov  dx, color
                sub  ebx, eax
                lea  edi, [edi + eax*2]
            fill:
                mov  word ptr [edi + ebx*2], dx
                add  ebx, 1
                jle  fill
            done:
            }
#else
            {
                int ll_l, ll_r, ll_x;
                ed[0].x += ed[1].x;
                ed[2].x += ed[3].x;
                if (ed[2].x - ed[0].x >= 0x8000) {
                    ll_l = ed[0].x >> 16;
                    ll_r = ed[2].x >> 16;
                    for (ll_x = ll_l; ll_x <= ll_r; ll_x++)
                        r.row[ll_x] = color;
                }
            }
#endif
            r.row += r.pitch;
        }
    } while (y < ylast);
}

/* Flat shade + Z, table slot 0x004b564c. Same sentinel and 0x14 interpolants
 * as Span_FillFlat; the left edge also carries z, and each pixel is written
 * only when its interpolated z is not behind the z-buffer.
 * Same three levers as Span_FillFlat (see its FRAME HOMES note): the
 * {crow, zrow, pitch} aggregate for the 0x64 frame, `short color` for the
 * partial write, and the store/store/RMW interpolant pair.  Both row
 * pointers must be seeded from their globals BEFORE the pitch*y add and
 * advanced with `+=`; folding them into one `cb + pitch * y` statement
 * (with or without named base locals) spills a base to the key slot and
 * costs one to three instructions. */
// FUNCTION: LEGOLAND 0x0041fba0
void Span_FillFlatZ(int tag, int* grad, int n, SortKey* key, SpanEdge* edge)
{
    SpanInterp ed[4];
    struct { short* crow; short* zrow; int pitch; } r;
    short      color;
    int        y;
    int        ylast;
    int        dz;

    y = key[0].y;
    edge[key[n - 1].idx].y1++;
    ylast = edge[key[n - 1].idx].y1;
    r.pitch = g_zb_pitch;
    r.zrow = g_zb_base;
    r.crow = g_raster_bits;
    dz = grad[1];
    g_zb_polys++;
    color = (short)g_shade_tab[tag][grad[0]];
    key[n].y = edge[key[n - 1].idx].y1;
    r.crow += r.pitch * y;
    r.zrow += r.pitch * y;
    do {
        SpanEdge* e = &edge[key->idx];

        key++;
        if (e->dir) {
            ed[2].x = e->a[0];
            ed[3].x = e->d[0];
            ed[2].x -= ed[3].x;
        } else {
            ed[0].x = e->a[0];
            ed[1].x = e->d[0];
            ed[0].z = e->a[1];
            ed[1].z = e->d[1];
            ed[0].x -= ed[1].x;
            ed[0].z -= ed[1].z;
        }
        while (y < key->y) {
            y++;
#ifndef LEGOLAND_PORTABLE
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
                mov  edi, r.crow
                mov  esi, r.zrow
                xchg ebx, eax
                sub  ebx, eax
                lea  edi, [edi + eax*2]
                lea  esi, [esi + eax*2]
            fill:
                mov  ax, color
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
#else
            {
                int ll_l, ll_r, ll_x, ll_z;
                ed[0].x += ed[1].x;
                ed[0].z += ed[1].z;
                ed[2].x += ed[3].x;
                if (ed[2].x - ed[0].x >= 0x8000) {
                    ll_l = ed[0].x >> 16;
                    ll_r = ed[2].x >> 16;
                    ll_z = ed[0].z;
                    for (ll_x = ll_l; ll_x <= ll_r; ll_x++, ll_z += dz) {
                        if ((unsigned short)(ll_z >> 16) >= (unsigned short)r.zrow[ll_x]) {
                            r.crow[ll_x] = color;
                            r.zrow[ll_x] = (short)(ll_z >> 16);
                        }
                    }
                }
            }
#endif
            r.crow += r.pitch;
            r.zrow += r.pitch;
        }
    } while (y < ylast);
}

/* Gouraud shade, no Z, table slot 0x004b5658 (g_span_fillers[0]). Shade is
 * 16.16, clamped through g_shade_clamp_mid and looked up in the tag's ramp.
 * A negative dshade is negated and the inner loop uses sbb instead of adc.
 * Same levers as Span_FillFlat, plus the one this body needs on its own:
 * **`flip = 0` must be written AFTER `ylast`.**  Frame homes are handed
 * out in order of FIRST STORE, so with `flip = 0` first, flip takes the
 * real slot at -8, ylast is pushed to -0xc and the frame grows to 0x68;
 * storing ylast first leaves flip to the dead `n` argument slot at
 * +0x10 and the frame is the original's 0x64.  VC6 still schedules the
 * `mov dword ptr [ebp+0x10], 0` back up to the top of the body. */
// FUNCTION: LEGOLAND 0x0041fd80
void Span_FillShade(int tag, int* grad, int n, SortKey* key, SpanEdge* edge)
{
    SpanInterp ed[4];
    struct { short* row; int dead; int pitch; } r;
    int        y;
    int        ylast;
    int        flip;

    y = key[0].y;
    edge[key[n - 1].idx].y1++;
    r.row = g_raster_bits;
    ylast = edge[key[n - 1].idx].y1;
    flip = 0;
    r.dead = (int)g_zb_base;
    r.pitch = g_zb_pitch;
    g_span_ramp = g_shade_tab[tag];
    g_zb_polys++;
    key[n].y = edge[key[n - 1].idx].y1;
    if (grad[1] < 0) {
        grad[1] = -grad[1];
        flip = 1;
    }
    g_span_dshade_hi = grad[1] >> 16;
    g_span_dshade_lo = grad[1] << 16;
    r.row += r.pitch * y;
    do {
        SpanEdge* e = &edge[key->idx];

        key++;
        if (e->dir) {
            ed[2].x = e->a[0];
            ed[3].x = e->d[0];
            ed[2].r4 = e->a[1];
            ed[3].r4 = e->d[1];
            ed[2].x -= ed[3].x;
            ed[2].r4 -= ed[3].r4;
        } else {
            ed[0].x = e->a[0];
            ed[1].x = e->d[0];
            ed[0].r4 = e->a[1];
            ed[1].r4 = e->d[1];
            ed[0].x -= ed[1].x;
            ed[0].r4 -= ed[1].r4;
        }
        while (y < key->y) {
            y++;
#ifndef LEGOLAND_PORTABLE
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
                mov  edi, r.row
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
#else
            /* PORT-M5 -- the asm arm above, instruction for instruction.
             *
             * THE CARRY-CHAINED SHADE.  The 16.16 shade is split by
             * `ror ecx,16` into two registers: ecx keeps the INTEGER half in
             * its low word (`and ecx,0ffffh`, so it is taken UNSIGNED) and is
             * then turned into a POINTER by adding g_shade_clamp_mid, while
             * eax keeps the FRACTION in its HIGH half (`and eax,0ffff0000h`).
             * Per pixel `add eax,g_span_dshade_lo` (the fraction step, which is
             * grad[1] << 16) sets CF, and `adc ecx,g_span_dshade_hi` walks the
             * clamp pointer by the integer step plus that carry -- a 48-bit
             * accumulator across two registers, with the table lookup free.
             *
             * A NEGATIVE dshade was negated into `grad[1]` and `flip` raised,
             * and the negative arm uses `sbb` for the integer half while still
             * ADDING the fraction.  That is deliberate, not a slip: carries out
             * of `frac + |step|` occur at the same rate as borrows out of
             * `frac - |step|`, so the pointer walks backwards at the right
             * speed.  Reproduced exactly.
             *
             * The span is [x0, x1] inclusive with both ends biased to x1 and a
             * negative index counted up to zero, as in Span_FillFlat.  The
             * shade interpolant is stepped in BOTH arms (the narrow arm does
             * `mov ecx,ed[4] / add ecx,ed[24] / mov ed[4],ecx` before it
             * leaves).  The original splits the loop on `flip` before entering
             * it; one loop with the test inside is the same arithmetic. */
            {
                int ll_l, ll_r, ll_n, ll_sh;
                unsigned int ll_frac;
                const unsigned char* ll_cp;
                unsigned short* ll_ramp;
                short* ll_row;

                ed[0].x += ed[1].x;
                ed[2].x += ed[3].x;
                if (ed[2].x - ed[0].x < 0x8000) {
                    ed[0].r4 += ed[1].r4;
                } else {
                    ll_l = ed[0].x >> 16;
                    ll_r = ed[2].x >> 16;
                    ll_n = ll_l - ll_r;
                    ll_row = r.row + ll_r;
                    ed[0].r4 += ed[1].r4;
                    ll_sh = ed[0].r4;
                    ll_ramp = g_span_ramp;
                    ll_frac = (unsigned int)ll_sh << 16;
                    ll_cp = g_shade_clamp_mid
                          + (((unsigned int)ll_sh >> 16) & 0xffffu);
                    do {
                        unsigned int ll_t;
                        ll_row[ll_n] = (short)ll_ramp[*ll_cp];
                        ll_t = ll_frac + (unsigned int)g_span_dshade_lo;
                        if (flip)
                            ll_cp -= (unsigned int)g_span_dshade_hi
                                   + (ll_t < ll_frac);
                        else
                            ll_cp += (unsigned int)g_span_dshade_hi
                                   + (ll_t < ll_frac);
                        ll_frac = ll_t;
                        ll_n += 1;
                    } while (ll_n <= 0);
                }
            }
#endif
            r.row += r.pitch;
        }
    } while (y < ylast);
}

/* Gouraud shade + Z, table slot 0x004b565c (g_span_fillers[1]). Left edge
 * carries x, shade and z; dshade_lo packs the shade fraction with the
 * z-step so one add advances both. The inner loop steals EBP for the ramp.
 * Levers: the {crow, zrow, pitch} aggregate, `flip = 0` after `ylast`, and
 * the store/store/RMW interpolant triple (x, r4 in the dir arm; x, r4, z
 * in the else arm) -- the original's [ebp-0x60] scratch reload in the else
 * arm is exactly the RMW's store-then-reload of ed[0].r4.  crow before
 * zrow in the seed (zrow first costs 2). */
// FUNCTION: LEGOLAND 0x0041ff80
void Span_FillShadeZ(int tag, int* grad, int n, SortKey* key, SpanEdge* edge)
{
    SpanInterp ed[4];
    struct { short* crow; short* zrow; int pitch; } r;
    int        y;
    int        ylast;
    int        flip;

    y = key[0].y;
    edge[key[n - 1].idx].y1++;
    r.crow = g_raster_bits;
    r.zrow = g_zb_base;
    ylast = edge[key[n - 1].idx].y1;
    flip = 0;
    r.pitch = g_zb_pitch;
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
    r.crow += r.pitch * y;
    r.zrow += r.pitch * y;
    do {
        SpanEdge* e = &edge[key->idx];

        key++;
        if (e->dir) {
            ed[2].x = e->a[0];
            ed[3].x = e->d[0];
            ed[2].r4 = e->a[1];
            ed[3].r4 = e->d[1];
            ed[2].x -= ed[3].x;
            ed[2].r4 -= ed[3].r4;
        } else {
            ed[0].x = e->a[0];
            ed[1].x = e->d[0];
            ed[0].r4 = e->a[1];
            ed[1].r4 = e->d[1];
            ed[0].z = e->a[2];
            ed[1].z = e->d[2];
            ed[0].x -= ed[1].x;
            ed[0].r4 -= ed[1].r4;
            ed[0].z -= ed[1].z;
        }
        while (y < key->y) {
            y++;
#ifndef LEGOLAND_PORTABLE
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
                mov  edi, r.crow
                mov  esi, r.zrow
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
#else
            /* PORT-M5 -- the asm arm above, instruction for instruction.
             *
             * ONE REGISTER CARRIES THE Z AND THE SHADE FRACTION.  eax is
             * seeded with `(z >> 8) & 0xffffff` -- the 16.16 Z shifted down
             * eight, so its integer part sits in bits 8..23 -- and
             * g_span_dshade_lo packs `(grad[1] & 0xffffff00) << 16` (the top
             * eight bits of the shade fraction, into bits 24..31) with
             * `(grad[2] >> 8) & 0xffffff` (the Z step, into bits 0..23).  One
             * `add` therefore advances both, `and eax,0feffffffh` after the
             * carry clears bit 24 so a Z carry cannot pollute the shade, and
             * `adc ecx,g_span_dshade_hi` walks the clamp POINTER exactly as in
             * Span_FillShade (see its note for the `sbb` arm).
             *
             * The Z key is `sar edx,8` of that register, whose low word is
             * bits 8..23 = z >> 16, compared 16-bit UNSIGNED (`jb posskip`):
             * drawn on `>=`, an equal key overwrites.  Unlike Span_FillShade
             * the integer shade is taken SIGNED here (`sar ecx,16`, no mask),
             * so a negative shade indexes BELOW g_shade_clamp_mid -- which is
             * what the "mid" in that pointer's name is for. */
            {
                int ll_l, ll_r, ll_n, ll_sh;
                unsigned int ll_acc;
                const unsigned char* ll_cp;
                unsigned short* ll_ramp;
                short* ll_crow;
                short* ll_zrow;

                ed[0].x += ed[1].x;
                ed[0].z += ed[1].z;
                ed[2].x += ed[3].x;
                if (ed[2].x - ed[0].x < 0x8000) {
                    ed[0].r4 += ed[1].r4;
                } else {
                    ll_l = ed[0].x >> 16;
                    ll_r = ed[2].x >> 16;
                    ll_n = ll_l - ll_r;
                    ll_crow = r.crow + ll_r;
                    ll_zrow = r.zrow + ll_r;
                    ed[0].r4 += ed[1].r4;
                    ll_sh = ed[0].r4;
                    ll_acc = (unsigned int)(ed[0].z >> 8) & 0xffffffu;
                    ll_cp = g_shade_clamp_mid + (ll_sh >> 16);
                    ll_ramp = g_span_ramp;
                    do {
                        unsigned short ll_key = (unsigned short)(ll_acc >> 8);
                        unsigned int ll_t;

                        if (ll_key >= (unsigned short)ll_zrow[ll_n]) {
                            ll_zrow[ll_n] = (short)ll_key;
                            ll_crow[ll_n] = (short)ll_ramp[*ll_cp];
                        }
                        ll_t = ll_acc + (unsigned int)g_span_dshade_lo;
                        if (flip)
                            ll_cp -= (unsigned int)g_span_dshade_hi
                                   + (ll_t < ll_acc);
                        else
                            ll_cp += (unsigned int)g_span_dshade_hi
                                   + (ll_t < ll_acc);
                        ll_acc = ll_t & 0xfeffffffu;
                        ll_n += 1;
                    } while (ll_n <= 0);
                }
            }
#endif
            r.crow += r.pitch;
            r.zrow += r.pitch;
        }
    } while (y < ylast);
}

/* Adaptive Simpson quadrature. Track_MeasureDistance (0x0042a1b0) drives
 * this. s holds fa+fb+4*odds+2*evens; approx is s*h; the return scales by
 * 1/3. The empty __asm forces the original's EBP frame so every temporary
 * stays in the 0x28 home list. `#pragma optimize("g", off)` is load-bearing:
 * it restores the jmp-to-test `jg` for, the two `add esp,4` cleanups, the
 * integer push of x, and the global-first fmul forms. The assign-expression
 * `x = a + (h = half*h)` is the `fst` / `fadd a` chain. Residual vs original
 * is only `call fn(a)` / `add esp,4` / `fstp fa` instead of `fstp` then
 * `add esp,4` — Og-off always cleans before the float store.
 * LL23 re-probe (2026-09-10), all still 79/81 or worse, AT ITS FLOOR:
 * the full `#pragma optimize` letter matrix ("y"/"gy"/"gp"/"ga"/"gw"/
 * "gs"/"a"/"w"/"" off, with and without the `__asm {}`) — "y" off alone
 * gives an EBP frame but 85i, "a"/"w" off 82i, "gt" off 85i, and
 * "g"/"gy"/"gp"/"ga"/"gw"/"gs"/"" off are all the same 81i/258B body;
 * store forms `f.s = (f.fa = fn(a)) + fn(b)` (82i), the comma statement,
 * a `volatile float*` dest (84i), a block temp (83i), `*(float*)&`,
 * a NON-volatile frame struct, `f.s = f.fa; f.s = f.s + fn(b)` (83i),
 * `fn(b) + f.fa`, `f.s = fn(b); f.s += f.fa` (82i), a duplicated store,
 * `if (1)` / `do {} while (0)` wrappers (84i), a block-local function
 * pointer (83i), an argument temp (83i), and `fa` as a separate volatile
 * or non-volatile local declared before or after a 9-field frame struct
 * (all 79/81, homes correct).  The empty `__asm {}` is now INERT for the
 * frame — `#pragma optimize("g", off)` alone gives the same 81i/258B
 * body — so only the two-instruction cleanup/store template remains. */
typedef struct SimpsonFrame {
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

#pragma optimize("g", off)
// FUNCTION: LEGOLAND 0x00420200
float IntegrateSimpson(float (*fn)(float), float a, float b, float tol)
{
    volatile SimpsonFrame f;

#ifndef LEGOLAND_PORTABLE
    __asm {}
#endif
    f.n = 1;
    f.h = b - a;
    f.s = fn(a) + fn(b);
    f.odd = 0.0f;
    f.approx = f.s * f.h * g_half * g_three;
    do {
        f.old = f.approx;
        f.s = f.s - g_two * f.odd;
        f.odd = 0.0f;
        f.step = f.h;
        f.x = a + (f.h = g_half * f.h);
        for (f.i = 1; f.i <= f.n; f.i = f.i + 1) {
            f.odd += fn(f.x);
            f.x += f.step;
        }
        f.s = f.s + g_four * f.odd;
        f.approx = f.s * f.h;
        f.n = f.n + f.n;
    } while (fabs(f.approx - f.old) > tol * fabs(f.old));
    return g_third * f.approx;
}
#pragma optimize("", on)

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
