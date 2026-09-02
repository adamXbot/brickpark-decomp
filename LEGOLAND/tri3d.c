/* LEGOLAND -- the software 3D triangle rasterisers: the whole of the game's
 * software 3D renderer, plus the render-target, Z-buffer, reciprocal-table,
 * pixel-format and shading-ramp state the four fillers run on.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field offsets, global addresses and callee arg counts are load-bearing;
 * the names are ours.  No legoland.h: every type is defined locally.
 *
 * The only caller is person3d.c's Draw3DPersonModel (via rin.c's
 * Render3DPerson): a bloke's model is rasterised triangle by triangle into a
 * 160x120 window of the locked 16bpp DirectDraw surface, with its own private
 * 128x120 Z buffer.  Nothing else in the game uses these.
 *
 * ===========================================================================
 * THE RASTERISATION CONTRACT
 * ===========================================================================
 *
 * VERTEX FORMAT (Vertex2D, 0x1c bytes, EVERYTHING 16.16 fixed except u/v)
 *
 *     +0x00 int    x        screen x inside the render target
 *     +0x04 int    y        screen y
 *     +0x08 int    z        DEPTH KEY -- larger is NEARER (see Z BUFFER)
 *     +0x0c float  u        texture u, [0,1]   (a FLOAT, not fixed point)
 *     +0x10 float  v        texture v, [0,1]
 *     +0x14 int    shade    diffuse term, 16.16, person3d biases it by 0x3333
 *     +0x18 int    -        unused by the rasterisers
 *
 * The three vertices are passed by pointer and the rasteriser SORTS THEM IN
 * PLACE IN ITS OWN ARGUMENT SLOTS by y (three compare-and-`xchg` steps, so
 * a.y <= b.y <= c.y).  The caller's Vertex2D records are not moved -- except
 * for one destructive write, see ORIGINAL QUIRKS.
 *
 * SHADE -> RAMP INDEX.  A shade is turned into a ramp index by `shade << 6`
 * and taking the HIGH 16 bits, i.e. index = shade >> 10.  A shade of 1.0
 * (0x10000) therefore indexes entry 64 -- one PAST a 64-level ramp -- and
 * nothing clamps, so the caller must keep shade below 1.0.  DrawFlatTri and
 * DrawFlatTexTri take that index from vertex a only; the Gouraud pair carry
 * (shade << 6) as a 16.16 quantity down the edges and across the span exactly
 * like z, and index the ramp with its high word at every pixel.
 *
 * EDGE / SCANLINE STEPPING (identical in all four)
 *
 *   1. Sort by y.  y0 = a.y >> 16, y1 = b.y >> 16, y2 = c.y >> 16.
 *   2. Two halves.  Upper half: edge A = a->b, edge B = a->c, both stepped
 *      from y0 to y1.  Lower half: edge A is re-aimed at b->c while edge B
 *      (a->c) and its accumulators CONTINUE -- the short edge's accumulator
 *      is NOT reloaded at vertex b, it simply arrives there with whatever
 *      error the stepping accumulated.  If y0 == y1 the upper half is skipped
 *      and both edges are set up from the flat top (a->c and b->c).
 *   3. Per edge, every interpolant steps by (end - start) * recip[dy] >> 16
 *      per scanline, where recip[] is the 16.16 table BuildRecipTable fills
 *      (recip[n] = 65536/n) -- x, z, and, for the Gouraud/textured variants,
 *      shade and u and v.  ALL of them are `imul` + `shrd eax,edx,16`, i.e.
 *      a full 32x32->64 multiply with a 16-bit shift; there is no
 *      perspective correction anywhere -- interpolation is affine in screen
 *      space.
 *   4. VERTICAL CLIP.  Before the scanline loop the whole set of accumulators
 *      is fast-forwarded by n = min(g_clip_y0 - y, y1 - y) scanlines when
 *      that is positive (one `mul` per interpolant, plus n * g_pitch on the
 *      row pointer).  The loop then runs while y < y_end AND y <= g_clip_y1.
 *   5. Per scanline the two edge x's decide left and right; the span's
 *      per-pixel deltas are (right_value - left_value) * recip[(dx>>16)+1]
 *      >> 16.  Note the +1: the reciprocal is taken over the pixel count
 *      INCLUSIVE of the last one, and recip[] only covers n = 1..99, so a
 *      triangle wider or taller than 99 pixels reads past the table.
 *   6. HORIZONTAL CLIP.  If the left x is left of g_clip_x0 the start is
 *      moved to g_clip_x0 and the interpolants are advanced by the skipped
 *      pixel count (`mul`).  The pixel pointer runs from
 *      row + x*2 to row + g_clip_x1*2 and the loop stops on either
 *      x >= right_x or ptr > end_ptr.
 *
 * THE Z BUFFER
 *
 *   128 x 120 dwords (g_zbw x g_zbh, allocated by InitRasterZBuffer as one
 *   0xf000-byte block at g_zbuf), one dword per pixel, addressed as
 *       zbuf[(y << 9) + x*4]        -- 0x200 bytes = 128 dwords per row
 *   so the Z buffer is indexed by the RENDER TARGET's x and y even though it
 *   is only 128 wide: a model window wider than 128 pixels would alias.  The
 *   test is
 *       if ((unsigned)z >= (unsigned)zbuf[y][x]) { zbuf[y][x] = z; write; }
 *   -- UNSIGNED, greater-or-equal wins, so a LARGER key is nearer and equal
 *   keys overwrite (later triangles win ties).  person3d.c builds the key as
 *   (tint << 24) + depth, which is why a whole person can be pushed in front
 *   of another by one byte while its own triangles still sort among
 *   themselves.  RenderZBufferObject seeds the buffer from a sprite's RLE Z
 *   frame (bigrender.c's ZBufferHelper) so the scenery occludes the model.
 *
 * TEXTURE ADDRESSING (the two textured fillers)
 *
 *   g_texture points at the descriptor SetTexture selected out of the
 *   0x00798190 texture table:
 *       +0x00 int    ushift    log2 of the texture's u extent
 *       +0x04 int    vshift    log2 of the v extent
 *       +0x08 u8*    texels    one byte per texel
 *       +0x0c Shade** ramps    per texel VALUE, a 64-level shading ramp
 *       +0x10 int    umask     (DrawGouraudTexTri only)
 *       +0x14 int    vmask     (DrawGouraudTexTri only)
 *
 *   Each vertex's float u,v is converted once at entry:
 *       uv16 = ((int)(uv * 65536.0f) & 0xffff) << shift
 *   -- the & 0xffff drops the integer part, so u == 1.0 WRAPS TO 0, and the
 *   shift turns the normalised coordinate into a 16.16 TEXEL coordinate.
 *   Those are then interpolated down the edges and across the span like z.
 *   Per pixel:
 *       texel = texels[(high16(u) << ushift) + high16(v)]
 *       pixel = ramps[texel]->table[high16(shade << 6)]
 *   DrawGouraudTexTri masks both halves first (`& umask`, `& vmask`), so it
 *   tiles; DrawFlatTexTri does NOT mask and will read outside the texture if
 *   an interpolated coordinate leaves the [0,1) range.
 *
 * SHADING RAMPS (MakeShadedColour and the cache)
 *
 *   A ramp is { int levels; unsigned short table[levels]; }: `levels/2`
 *   entries fading BLACK -> the base colour and `levels/2` fading the base
 *   colour -> WHITE, in the surface's own 16-bit pixel format.  The colour
 *   key is a 3-byte triple; BuildChannelTables shows byte[0] lands in the LOW
 *   5 bits and byte[2] in the TOP 5, so in a DirectDraw 5-6-5 surface the
 *   triple is stored BLUE, GREEN, RED.  Ramps are cached on g_shade_head and
 *   shared by every face with the same triple (FindShadedColour /
 *   AddShadedColour), so SetFlatColour's "colour" argument is really a
 *   Shade* handle, and a texture's palette is an array of them.
 *
 * PIXEL PACKING (BuildChannelTables, 0x004860f0)
 *
 *   g_chan[256] is a 4-entry-wide table of pre-shifted field values:
 *       c0 = i*31/256                       bits 4..0
 *       c1 = (i*max/256) << 5               bits 4+g_green_bits..5
 *       c2 = (i*31/256) << (g_green_bits+5) the top 5 bits
 *   with max = 0xff >> (8 - g_green_bits).  g_green_bits == 6 gives 5-6-5,
 *   == 5 gives 5-5-5.  A pixel is one lookup per channel OR'd together,
 *   which is how MakeShadedColour packs without knowing the format.
 *
 * THE MOUSE PICK
 *
 *   SetMousePixel(base, pos) clears g_raster_hit and parks the surface
 *   address of the pixel under the mouse in g_mouse_pixel.  Every filler
 *   compares its output pointer against it and ORs the equality into
 *   g_raster_hit, so after a model is drawn the caller knows whether the
 *   mouse is over any of its pixels -- picking for free, with no extra pass.
 *
 * SET-UP ORDER (what a browser runtime must reproduce)
 *
 *   Render_SetPixelFormat(greenBits)   -> BuildChannelTables, InitRasterZBuffer,
 *                                         BuildRecipTable
 *   SetRenderTarget(surface, pitch, width, height)  per lock
 *   SetMousePixel(surface, &mousePos)               per frame
 *   RenderZBufferObject(sprite, x, y)               per model: clear + seed Z
 *   SetFlatColour(ramp) | SetTexture(id)            per triangle
 *   DrawFlatTri | DrawGouraudTri | DrawFlatTexTri | DrawGouraudTexTri
 *
 * ===========================================================================
 * ORIGINAL QUIRKS, reproduced faithfully
 * ===========================================================================
 *  - DrawFlatTri SHIFTS THE CALLER'S VERTEX: `shl dword ptr [a+0x14], 6`
 *    writes the shifted shade back into vertex a.  The other three copy the
 *    field out first.  Drawing the same Vertex2D twice with DrawFlatTri
 *    therefore shifts its shade twice and indexes the ramp 64x further out.
 *  - recip[] only covers 1..99, and the span reciprocal is indexed with
 *    (dx>>16)+1, so a 99-pixel span already reads recip[100].
 *  - Neither the ramp index nor the texel coordinates are clamped.
 *  - DrawFlatTexTri does not apply the descriptor's u/v masks (the Gouraud
 *    variant does), so it can address outside the texture.
 *  - FreeShadedColour frees a ramp without unlinking its cache node, leaving
 *    a dangling entry on g_shade_head.
 *  - MakeShadedColour never checks the second MemAlloc, and on a failed first
 *    MemAlloc still calls AddShadedColour(0, rgb), caching a null ramp.
 *
 * ===========================================================================
 * HOW THIS FILE IS WRITTEN (VC6 SP3 notes)
 * ===========================================================================
 * The four fillers are HAND-WRITTEN ASSEMBLY.  Two things prove it: `xchg
 * dword ptr [ebp+0xc], eax` (no compiler emits xchg with memory) and, in
 * DrawGouraudTri / DrawFlatTexTri / DrawGouraudTexTri, the callee-saved
 * `push`es sitting INSIDE the instruction stream -- `push ebx / push esi`
 * after two `mov`s and `push edi` after a `cmp`, filling Pentium pairing
 * slots.  VC6's inline assembler always emits those pushes at the top of the
 * asm region (measured), so they cannot be compiler output.  All four are
 * therefore written as `__declspec(naked)` with the prologue, the frame and
 * the epilogue spelled out; locals are raw `[ebp-N]` slots, and the three
 * parameter homes `[ebp+8] / [ebp+0xc] / [ebp+0x10]` are reused as the
 * edge-x accumulators and the scanline counter once the sort is done.
 * `__declspec(naked)` is placed on the line ABOVE the `// FUNCTION:` marker
 * so the marker still sits immediately above the signature (both verify.py
 * and audit.py take the function name from the next non-comment line).
 *
 * FRAME SLOT MAP -- DrawFlatTri (0x50 bytes); the other three follow the same
 * shape with more interpolants (0x6c, 0x90, 0xac):
 *     [ebp-0x50] the flat pixel            [ebp-0x24] y1
 *     [ebp-0x4c] current row base          [ebp-0x20] dz/dy edge A
 *     [ebp-0x48] z1                        [ebp-0x1c] dx/dy edge A
 *     [ebp-0x44] x1                        [ebp-0x18] y2
 *     [ebp-0x40] z2                        [ebp-0x14] z along the span
 *     [ebp-0x3c] x2                        [ebp-0x10] span reciprocal
 *     [ebp-0x38] y0, then dz/dy edge B     [ebp-0x0c] next row base
 *     [ebp-0x34] dx/dy edge B              [ebp-0x08] z along edge B
 *     [ebp-0x30] z0, then span left x      [ebp-0x04] z along edge A
 *     [ebp-0x2c] dz/dx                     [ebp+0x08] x along edge B
 *     [ebp-0x28] recip, then span right x  [ebp+0x0c] x along edge A
 *                                          [ebp+0x10] current scanline y
 *
 * The C functions in this file needed three levers worth recording:
 *  - MakeShadedColour: the three ramp accumulators and the blue base value
 *    only stay in x87 REGISTERS (three separate `fld` of the 0.0f literal)
 *    if `(float)half` is spilled instead -- and that happens only when the
 *    `half` variable IS the `levels` parameter shifted in place
 *    (`levels >>= 1`), which frees the parameter's stack home for the
 *    int->float conversion temporaries.  A separate `half` local costs 84 of
 *    158 instructions.  The channel index must also be spelled
 *    `(unsigned char)(int)acc` and not `(int)acc & 0xff`: the byte cast is
 *    what puts the ramp pixel in a byte-addressable register (ebx) and keeps
 *    the loop out of a spill.
 *  - BuildRecipTable: `1.0f / n` and `65536.0 / n` only share ONE `fild n`
 *    when n is first converted through a `float` LOCAL (`fn = (float)n`);
 *    written directly, VC6 emits `fild` twice and reverse-divides.
 *  - FindShadedColour: the rotated `while (p)` walk, not `if (p) do {}
 *    while (p)`, is what puts the not-found `xor eax,eax` tail before the
 *    found tail.
 * ------------------------------------------------------------------------- */

/* ---- local types -------------------------------------------------------- */

typedef struct WinRect {
    long left;     /* +0x00 */
    long top;      /* +0x04 */
    long right;    /* +0x08 */
    long bottom;   /* +0x0c */
} WinRect;

typedef struct Pos {
    int x;         /* +0x00 */
    int y;         /* +0x04 */
} Pos;

/* One cached 64-level shading ramp: `levels` 16-bit pixels from black
 * through the base colour to white. */
typedef struct Shade {
    int             levels;   /* +0x00 */
    unsigned short* table;    /* +0x04  `levels` packed 16-bit pixels */
} Shade;

/* One entry of the shading-ramp cache: the ramps are keyed by their RGB
 * triple so two faces with the same colour share one 64-level table. */
typedef struct ShadeNode {
    struct ShadeNode* next;   /* +0x00 */
    unsigned char     rgb[3]; /* +0x04..+0x06  the key (same order as the caller's) */
    unsigned char     pad07;  /* +0x07 */
    Shade*            shade;  /* +0x08 */
} ShadeNode;

/* The 256-entry channel table 0x004860f0 builds for the current 16-bit
 * surface format: entry i holds the already-shifted field value for each of
 * the three channels, so a pixel is one table lookup per channel OR'd. */
typedef struct ChanEntry {
    unsigned short c2;    /* +0x00  rgb[2]'s field, shifted to the TOP */
    unsigned short c1;    /* +0x02  rgb[1]'s field, shifted left by 5 */
    unsigned short c0;    /* +0x04  rgb[0]'s field, the LOW bits */
    unsigned short pad;   /* +0x06 */
} ChanEntry;

/* person3d.c's screen-space vertex: 0x1c bytes, 16.16 fixed throughout. */
typedef struct Vertex2D {
    int x;      /* +0x00  screen x, 16.16 */
    int y;      /* +0x04  screen y, 16.16 */
    int z;      /* +0x08  depth / sort key */
    int u;      /* +0x0c  texture u */
    int v;      /* +0x10  texture v */
    int shade;  /* +0x14  lighting term + 0x3333 */
    int pad18;  /* +0x18 */
} Vertex2D;

/* gpu.c's SpriteRec; only the fields this file touches. */
typedef struct SpriteRec {
    struct SpriteRec* next;    /* +0x00 */
    void*             surface; /* +0x04 */
    void**            image;   /* +0x08  ImageRec*, whose +0x00 is the LLS */
    int               detail;  /* +0x0c */
    unsigned int      flags;   /* +0x10 */
    short             w;       /* +0x14 */
    short             h;       /* +0x16 */
} SpriteRec;

/* ---- globals ------------------------------------------------------------ */

extern Shade* g_flat_colour;     /* 0x0066b61c  the ramp SetFlatColour picked */
extern int   g_recip[];          /* 0x00798000  reciprocal table: 65536/n, 16.16 */
extern char* g_surface;          /* 0x00797e68  locked 16bpp surface base */
extern int   g_pitch;            /* 0x00701e58  surface pitch in bytes */
extern int   g_clip_x0;          /* 0x0081c8d0  raster clip rect, left */
extern int   g_clip_y0;          /* 0x0081c8d4  top */
extern int   g_clip_x1;          /* 0x0081c8d8  right  (inclusive) */
extern int   g_clip_y1;          /* 0x0081c8dc  bottom (inclusive) */
extern char* g_mouse_pixel;      /* 0x007fe9a8  surface address under the mouse */
extern int   g_raster_hit;       /* 0x007feb14  set when a pixel lands on it
                                  *   (cleared as a dword, set with a byte OR) */
extern void* g_texture;          /* 0x0066b630  current texture (SetTexture) */
extern ShadeNode* g_shade_head;  /* 0x00797e6c  the shading-ramp cache */
extern int   g_width;            /* 0x00701e60  render target width in pixels */
extern int   g_rows;             /* 0x0066be48  render target height in rows */
extern int   g_zbpitch;          /* 0x0066be4c  Z-buffer pitch in bytes (w*4) */
extern float g_recipf[];         /* 0x00797e74  1.0f/n for n = 1..99 (index n-1) */
extern int   g_green_bits;       /* 0x007cb5e0  green field width: 6 = 565, 5 = 555 */
extern unsigned short g_clear_pixel; /* 0x007cb5e4  ClearRenderTarget's fill */
extern void* g_textures[];       /* 0x00798190 */
extern int   g_zbw;              /* 0x0066be40  Z-buffer width  (128) */
extern int   g_zbh;              /* 0x0066be44  Z-buffer height (120) */
extern void* g_zbuf;             /* 0x00701e5c  128*120 dwords */
extern ChanEntry g_chan[256];    /* 0x0066b638  built by 0x004860f0 */

/* ---- prototypes --------------------------------------------------------- */

__declspec(dllimport) int __stdcall IntersectRect(WinRect* dst, const WinRect* a,
                                                  const WinRect* b); /* [0x4ab2a0] */

extern void*  MemAlloc(unsigned int size);                         /* 0x0049e4ff (malloc) */
void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)
extern void  MemFree(void* p);                                     /* 0x0049e4d0 (free) */

void BuildChannelTables(void);                                     /* 0x004860f0 */
void InitRasterZBuffer(void);                                      /* 0x00485f60 */
int  BuildRecipTable(void);                                        /* 0x00486540 */
extern void ZBufferHelper(char* lls, WinRect* src, Pos* dst, void* zbuf); /* 0x00464a90 */

/* ---- functions ---------------------------------------------------------- */

/* 0x00485fc0 -- (re)initialise the software renderer for a pixel format.
 * Exact under tools/audit.py but its last statement is a tail CALL, so it
 * compiles to a `jmp` with no `ret` of its own and the shared tools/match.py
 * cannot bound it -- held as WIP per the tail-jump convention. */
// WIP-FUNCTION: LEGOLAND 0x00485fc0  (100% by audit.py; match.py cannot bound a tail-jmp function)
void Render_SetPixelFormat(int greenBits)
{
    g_green_bits = greenBits;
    BuildChannelTables();
    InitRasterZBuffer();
    BuildRecipTable();
}

/* 0x004860f0 -- rebuild the 256-entry channel table for the current 16-bit
 * pixel format.  entry[i] holds, pre-shifted, the field value an 8-bit
 * channel level i contributes:  c0 = i*31/256 (the LOW 5 bits), c1 =
 * (i*max/256) << 5 (the middle g_green_bits), c2 = (i*31/256) << (bits+5)
 * (the TOP 5).  max = 0xff >> (8 - g_green_bits), so bits=6 is 5-6-5 and
 * bits=5 is 5-5-5.
 *
 * WIP: 43/43 instructions, 6 mismatches, all in one place.  The original
 * keeps the first __ftol result in a SIXTEEN-bit register (`mov di, ax`)
 * and shifts it in eax (`mov eax, edi / shl eax, cl`); every spelling tried
 * -- `unsigned short` / `short` / an explicit (int) widening / a multiply by
 * (1 << shift) / storing c0 first -- keeps the value 32-bit in edi and
 * shifts through edx instead (or, when c0 is stored first, drops the
 * register altogether and comes out SHORTER than the original).  Everything
 * else, including the frame (0xc: the u64 __ftol temp and the loop counter
 * must not share a slot, which is why `i = 0` is written before `m = max`),
 * the two-step float local that stops the 1/256 * 31 constants folding
 * together, and the base+2 induction pointer, is exact. */
// WIP-FUNCTION: LEGOLAND 0x004860f0  (43/43 insns, 9 mismatches: `mov di,ax` narrowing)
void BuildChannelTables(void)
{
    unsigned int   max;
    double         m;
    int            shift;
    int            i;
    unsigned short a;
    float          t;

    shift = g_green_bits + 5;
    max = (unsigned int)0xff >> (8 - g_green_bits);
    i = 0;
    m = max;
    for (; i < 256; i++) {
        t = i * (1.0f / 256.0f);
        a = (unsigned short)(int)(t * 31.0f);
        g_chan[i].c2 = (unsigned short)(a << shift);
        g_chan[i].c1 = (unsigned short)((int)(t * m) << 5);
        g_chan[i].c0 = a;
    }
}

/* 0x004860b0 -- flood the whole render target with g_clear_pixel.  Hand
 * assembly: the fill value is a 32-bit pattern (two pixels), which `memset`
 * cannot express, and the row walk is a `rep stosd` per row. */
__declspec(naked)
// FUNCTION: LEGOLAND 0x004860b0
void ClearRenderTarget(void)
{
    __asm {
        push   ebx
        push   edi
        push   edi
        movzx  ebx, word ptr g_clear_pixel
        mov    eax, ebx
        shl    eax, 0x10
        or     eax, ebx
        mov    ebx, dword ptr g_surface
        mov    edx, dword ptr g_rows
      L0:
        mov    edi, ebx
        add    ebx, dword ptr g_pitch
        mov    ecx, dword ptr g_width
        shr    ecx, 1
        rep    stosd
        dec    edx
        jne    L0
        pop    edi
        pop    edi
        pop    ebx
        ret
    }
}

/* 0x00486540 -- build the reciprocal tables the rasterisers divide with.
 * g_recip[n] = 65536/n as 16.16 and g_recipf[n-1] = 1.0f/n, for n = 1..99 --
 * which is why no triangle may span more than 99 scanlines or 99 pixels. */
// FUNCTION: LEGOLAND 0x00486540
int BuildRecipTable(void)
{
    int   n;
    float fn;

    for (n = 1; n < 100; n++) {
        fn = (float)n;
        g_recipf[n - 1] = 1.0f / fn;
        g_recip[n] = (int)(65536.0 / fn);
    }
    return 0;
}

/* 0x00485fa0 -- release the Z buffer. */
// FUNCTION: LEGOLAND 0x00485fa0
void FreeRasterZBuffer(void)
{
    if (g_zbuf)
        MemFree(g_zbuf);
}

/* 0x00485f30 -- point the rasterisers at a locked 16bpp surface. */
// FUNCTION: LEGOLAND 0x00485f30
void SetRenderTarget(char* surface, int pitch, int width, int height)
{
    g_surface = surface;
    g_pitch = pitch;
    g_width = width;
    g_rows = height;
}

/* 0x00485f60 -- allocate the 128x120 dword Z buffer the model window uses. */
// FUNCTION: LEGOLAND 0x00485f60
void InitRasterZBuffer(void)
{
    g_zbw = 128;
    g_zbh = 120;
    g_zbuf = MemAlloc(0xf000);
    g_zbpitch = g_zbw * 4;
}

/* 0x00485f20 -- point the rasterisers at a texture descriptor. */
// FUNCTION: LEGOLAND 0x00485f20
void SetTextureBits(void* bits)
{
    g_texture = bits;
}

/* 0x004864f0 -- sample a ramp at t in [0,1]. */
// FUNCTION: LEGOLAND 0x004864f0
unsigned short ShadeLookup(Shade* s, float t)
{
    return s->table[(int)((float)(s->levels - 1) * t)];
}

/* 0x00488700 -- arm the mouse-pick test for the coming triangles. */
// FUNCTION: LEGOLAND 0x00488700
void SetMousePixel(char* base, Pos* p)
{
    g_raster_hit = 0;
    g_mouse_pixel = base + p->y * g_pitch + p->x * 2;
}

/* 0x00486190 -- look one RGB triple up in the shading-ramp cache. */
// FUNCTION: LEGOLAND 0x00486190
Shade* FindShadedColour(unsigned char* rgb)
{
    ShadeNode* n;

    n = g_shade_head;
    while (n) {
        if (n->rgb[2] == rgb[2] && n->rgb[1] == rgb[1] && n->rgb[0] == rgb[0])
            return n->shade;
        n = n->next;
    }
    return 0;
}

/* 0x004861d0 -- push a freshly built ramp onto the cache. */
// FUNCTION: LEGOLAND 0x004861d0
void AddShadedColour(Shade* s, unsigned char* rgb)
{
    ShadeNode* n;

    n = (ShadeNode*)MemAlloc(12);
    if (n) {
        memset(n, 0, 12);
        n->rgb[2] = rgb[2];
        n->rgb[1] = rgb[1];
        n->rgb[0] = rgb[0];
        n->shade = s;
        if (g_shade_head)
            n->next = g_shade_head;
        g_shade_head = n;
    }
}

/* 0x00486220 -- release one ramp (the cache node is NOT unlinked: an
 * original leak/dangling-pointer bug, reproduced). */
// FUNCTION: LEGOLAND 0x00486220
void FreeShadedColour(Shade* s)
{
    if (s) {
        if (s->table)
            MemFree(s->table);
        MemFree(s);
    }
}

// FUNCTION: LEGOLAND 0x004864e0
void SetFlatColour(int colour)
{
    g_flat_colour = (Shade*)colour;
}

// FUNCTION: LEGOLAND 0x004886e0
void SetTexture(int id)
{
    SetTextureBits(g_textures[id]);
}

// FUNCTION: LEGOLAND 0x00485fe0
void RenderZBufferObject(SpriteRec* s, int x, int y)
{
    WinRect win;
    WinRect src;
    Pos     dst;

    __asm {
        push edi
        mov  eax, g_zbw
        imul g_zbh
        mov  ecx, eax
        mov  eax, 0
        mov  edi, g_zbuf
        rep  stosd
        pop  edi
    }

    if (s == 0)
        return;
    src.left = 0;
    src.top = 0;
    src.right = s->w;
    src.bottom = s->h;
    win.left = x;
    win.top = y;
    win.right = g_zbw + x;
    win.bottom = g_zbh + y;
    if (!IntersectRect(&src, &src, &win))
        return;
    if (x < 0) {
        dst.x = -x;
        src.left = 0;
    } else {
        dst.x = 0;
    }
    if (y < 0) {
        dst.y = -y;
        src.top = 0;
    } else {
        dst.y = 0;
    }
    ZBufferHelper((char*)*s->image, &src, &dst, g_zbuf);
}

/* -------------------------------------------------------------------------
 * 0x00486280 -- build (or find) the shading ramp for one RGB triple.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00486280
Shade* MakeShadedColour(int levels, unsigned char* rgb)
{
    Shade* s;
    float  f0, f1, f2;
    float  d0, d1, d2;
    float  a0, a1, a2;
    int    i;
    unsigned short px;

    s = FindShadedColour(rgb);
    if (s)
        return s;
    s = (Shade*)MemAlloc(8);
    if (s) {
        s->levels = levels;
        s->table = (unsigned short*)MemAlloc(levels * 2);
        f2 = (float)rgb[2];
        f1 = (float)rgb[1];
        f0 = (float)rgb[0];
        levels >>= 1;
        a2 = 0.0f;
        a1 = 0.0f;
        a0 = 0.0f;
        d2 = f2 / (float)levels;
        d1 = f1 / (float)levels;
        d0 = f0 / (float)levels;
        for (i = 0; i < levels; i++) {
            px  = g_chan[(unsigned char)(int)a0].c0;
            px |= g_chan[(unsigned char)(int)a1].c1;
            px |= g_chan[(unsigned char)(int)a2].c2;
            s->table[i] = px;
            a2 += d2;
            a1 += d1;
            a0 += d0;
        }
        d2 = (255.0f - f2) / (float)levels;
        d1 = (255.0f - f1) / (float)levels;
        d0 = (255.0f - f0) / (float)levels;
        a2 = f2;
        a1 = f1;
        a0 = f0;
        for (i = 0; i < levels; i++) {
            px  = g_chan[(unsigned char)(int)a0].c0;
            px |= g_chan[(unsigned char)(int)a1].c1;
            px |= g_chan[(unsigned char)(int)a2].c2;
            s->table[levels + i] = px;
            a2 += d2;
            a1 += d1;
            a0 += d0;
        }
    }
    AddShadedColour(s, rgb);
    return s;
}

/* -------------------------------------------------------------------------
 * 0x004877b0 -- DrawFlatTri: flat-colour, Z-buffered triangle.
 * ------------------------------------------------------------------------- */

__declspec(naked)
// FUNCTION: LEGOLAND 0x004877b0
void DrawFlatTri(Vertex2D* a, Vertex2D* b, Vertex2D* c)
{
    __asm {
        push   ebp
        mov    ebp, esp
        sub    esp, 0x50
        push   ebx
        push   esi
        push   edi
        mov    ebx, dword ptr g_flat_colour
        mov    ebx, dword ptr [ebx + 4]
        mov    edx, dword ptr [ebp+0x8]
        shl    dword ptr [edx + 0x14], 6
        movzx  edx, word ptr [edx + 0x16]
        movzx  edx, word ptr [ebx + edx*2]
        mov    dword ptr [ebp-0x50], edx
        mov    edx, dword ptr [ebp+0x8]
        mov    ecx, dword ptr [ebp+0xc]
        mov    eax, dword ptr [edx + 4]
        mov    esi, dword ptr [ecx + 4]
        cmp    eax, esi
        jle    L0
        mov    eax, dword ptr [ebp+0x8]
        xchg   dword ptr [ebp+0xc], eax
        mov    dword ptr [ebp+0x8], eax
        mov    ecx, dword ptr [ebp+0xc]
        mov    edx, dword ptr [ebp+0x8]
      L0:
        mov    ebx, dword ptr [ebp+0x10]
        mov    eax, dword ptr [ecx + 4]
        cmp    eax, dword ptr [ebx + 4]
        jle    L1
        mov    eax, dword ptr [ebp+0xc]
        xchg   dword ptr [ebp+0x10], eax
        mov    dword ptr [ebp+0xc], eax
        mov    ebx, dword ptr [ebp+0x10]
        mov    ecx, dword ptr [ebp+0xc]
      L1:
        mov    eax, dword ptr [edx + 4]
        mov    esi, dword ptr [ecx + 4]
        cmp    eax, esi
        jle    L2
        mov    eax, dword ptr [ebp+0x8]
        xchg   dword ptr [ebp+0xc], eax
        mov    dword ptr [ebp+0x8], eax
        mov    ecx, dword ptr [ebp+0xc]
        mov    edx, dword ptr [ebp+0x8]
      L2:
        mov    edi, dword ptr [ebx + 4]
        mov    eax, dword ptr [edx + 4]
        sar    edi, 0x10
        mov    dword ptr [ebp-0x18], edi
        mov    edi, dword ptr g_pitch
        sar    eax, 0x10
        imul   edi, eax
        add    edi, dword ptr g_surface
        mov    esi, dword ptr [ecx + 4]
        sar    esi, 0x10
        mov    dword ptr [ebp-0xc], edi
        mov    edi, dword ptr [edx]
        mov    dword ptr [ebp+0x8], edi
        mov    edi, dword ptr [ecx]
        mov    ecx, dword ptr [ecx + 8]
        mov    dword ptr [ebp-0x44], edi
        mov    edi, dword ptr [ebx]
        cmp    eax, esi
        mov    dword ptr [ebp-0x3c], edi
        mov    edi, dword ptr [edx + 8]
        mov    edx, dword ptr [ebx + 8]
        mov    dword ptr [ebp-0x38], eax
        mov    dword ptr [ebp-0x24], esi
        mov    dword ptr [ebp-0x30], edi
        mov    dword ptr [ebp-0x48], ecx
        mov    dword ptr [ebp-0x40], edx
        mov    dword ptr [ebp+0x10], eax
        je     L13
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x24]
        sub    ecx, dword ptr [ebp-0x38]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x4c], eax
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x18]
        sub    ecx, dword ptr [ebp-0x38]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x44]
        sub    eax, dword ptr [ebp+0x8]
        mov    ecx, dword ptr [ebp-0x4c]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x1c], eax
        mov    eax, dword ptr [ebp-0x3c]
        sub    eax, dword ptr [ebp+0x8]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x34], eax
        mov    eax, dword ptr [ebp-0x48]
        sub    eax, dword ptr [ebp-0x30]
        mov    ecx, dword ptr [ebp-0x4c]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x20], eax
        mov    eax, dword ptr [ebp-0x40]
        sub    eax, dword ptr [ebp-0x30]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x38], eax
        mov    eax, dword ptr [ebp+0x8]
        mov    dword ptr [ebp-0x4], edi
        mov    dword ptr [ebp+0xc], eax
        mov    dword ptr [ebp+0x8], eax
        mov    dword ptr [ebp-0x8], edi
        mov    ebx, dword ptr g_clip_y0
        mov    ecx, dword ptr [ebp+0x10]
        mov    edx, dword ptr [ebp-0x24]
        sub    ebx, ecx
        jle    L4
        sub    edx, ecx
        jle    L4
        cmp    ebx, edx
        mov    ecx, edx
        jns    L3
        mov    ecx, ebx
      L3:
        add    dword ptr [ebp+0x10], ecx
        mov    eax, dword ptr [ebp-0x1c]
        mul    ecx
        add    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x34]
        mul    ecx
        add    dword ptr [ebp+0x8], eax
        mov    eax, dword ptr [ebp-0x20]
        mul    ecx
        add    dword ptr [ebp-0x4], eax
        mov    eax, dword ptr [ebp-0x38]
        mul    ecx
        add    dword ptr [ebp-0x8], eax
        mov    eax, dword ptr g_pitch
        mul    ecx
        add    dword ptr [ebp-0xc], eax
      L4:
        cmp    dword ptr [ebp+0x10], esi
        jge    L12
      L5:
        mov    eax, dword ptr [ebp+0x10]
        mov    ecx, dword ptr g_clip_y1
        cmp    eax, ecx
        jg     L12
        mov    eax, dword ptr [ebp-0xc]
        mov    ecx, dword ptr g_pitch
        mov    esi, dword ptr [ebp+0xc]
        mov    edi, dword ptr [ebp+0x8]
        mov    dword ptr [ebp-0x4c], eax
        add    eax, ecx
        cmp    esi, edi
        mov    dword ptr [ebp-0xc], eax
        jle    L6
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0xc]
        sub    ecx, dword ptr [ebp+0x8]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x10], eax
        mov    eax, dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp-0x8]
        sub    eax, ecx
        mov    dword ptr [ebp-0x14], ecx
        mov    ecx, dword ptr [ebp-0x10]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x2c], eax
        sar    edi, 0x10
        sar    esi, 0x10
        mov    dword ptr [ebp-0x30], edi
        mov    dword ptr [ebp-0x28], esi
        jmp    L7
      L6:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0x8]
        sub    ecx, dword ptr [ebp+0xc]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x10], eax
        mov    eax, dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp-0x4]
        sub    eax, ecx
        mov    dword ptr [ebp-0x14], ecx
        mov    ecx, dword ptr [ebp-0x10]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x2c], eax
        sar    esi, 0x10
        sar    edi, 0x10
        mov    dword ptr [ebp-0x30], esi
        mov    dword ptr [ebp-0x28], edi
      L7:
        push   esi
        push   edi
        mov    ecx, dword ptr [ebp-0x30]
        mov    edi, ecx
        mov    ebx, dword ptr g_clip_x0
        sub    ecx, ebx
        jns    L8
        mov    edi, ebx
        neg    ecx
        mov    eax, dword ptr [ebp-0x2c]
        mul    ecx
        add    dword ptr [ebp-0x14], eax
      L8:
        mov    esi, edi
        shl    esi, 1
        add    esi, dword ptr [ebp-0x4c]
        mov    eax, edi
        mov    edi, dword ptr g_clip_x1
        shl    edi, 1
        add    edi, dword ptr [ebp-0x4c]
        cmp    eax, dword ptr [ebp-0x28]
        jge    L11
        cmp    esi, edi
        jg     L11
      L9:
        mov    ebx, dword ptr g_zbuf
        mov    ecx, dword ptr [ebp+0x10]
        lea    ebx, [ebx + eax*4]
        shl    ecx, 9
        mov    edx, dword ptr [ebp-0x14]
        cmp    edx, dword ptr [ebx + ecx]
        jb     L10
        cmp    esi, dword ptr g_mouse_pixel
        mov    dword ptr [ebx + ecx], edx
        mov    edx, dword ptr [ebp-0x50]
        sete   cl
        mov    word ptr [esi], dx
        or     byte ptr g_raster_hit, cl
      L10:
        inc    eax
        mov    ebx, dword ptr [ebp-0x2c]
        add    dword ptr [ebp-0x14], ebx
        add    esi, 2
        cmp    eax, dword ptr [ebp-0x28]
        jge    L11
        cmp    esi, edi
        jg     L11
        jmp    L9
      L11:
        pop    edi
        pop    esi
        mov    ecx, dword ptr [ebp-0x1c]
        mov    eax, dword ptr [ebp+0xc]
        mov    edi, dword ptr [ebp-0x4]
        mov    edx, dword ptr [ebp-0x34]
        mov    ebx, dword ptr [ebp+0x8]
        mov    esi, dword ptr [ebp-0x8]
        add    eax, ecx
        mov    ecx, dword ptr [ebp-0x38]
        mov    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x20]
        add    edi, eax
        mov    eax, dword ptr [ebp+0x10]
        add    ebx, edx
        add    esi, ecx
        mov    ecx, dword ptr [ebp-0x24]
        inc    eax
        cmp    eax, ecx
        mov    dword ptr [ebp+0x8], ebx
        mov    dword ptr [ebp-0x4], edi
        mov    dword ptr [ebp-0x8], esi
        mov    dword ptr [ebp+0x10], eax
        jl     L5
      L12:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x18]
        sub    ecx, dword ptr [ebp-0x24]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x28], eax
        mov    eax, dword ptr [ebp-0x3c]
        sub    eax, dword ptr [ebp-0x44]
        mov    ecx, dword ptr [ebp-0x28]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x1c], eax
        mov    eax, dword ptr [ebp-0x40]
        sub    eax, dword ptr [ebp-0x48]
        mov    ecx, dword ptr [ebp-0x28]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x20], eax
        jmp    L14
      L13:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x18]
        sub    ecx, dword ptr [ebp-0x38]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x18]
        sub    ecx, dword ptr [ebp-0x24]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x28], eax
        mov    eax, dword ptr [ebp-0x3c]
        sub    eax, dword ptr [ebp+0x8]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x1c], eax
        mov    eax, dword ptr [ebp-0x3c]
        sub    eax, dword ptr [ebp-0x44]
        mov    ecx, dword ptr [ebp-0x28]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x34], eax
        mov    eax, dword ptr [ebp-0x40]
        sub    eax, dword ptr [ebp-0x30]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x20], eax
        mov    eax, dword ptr [ebp-0x40]
        sub    eax, dword ptr [ebp-0x48]
        mov    ecx, dword ptr [ebp-0x28]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x38], eax
        mov    edx, dword ptr [ebp+0x8]
        mov    eax, dword ptr [ebp-0x44]
        mov    ecx, dword ptr [ebp-0x48]
        mov    dword ptr [ebp+0xc], edx
        mov    dword ptr [ebp+0x8], eax
        mov    dword ptr [ebp-0x4], edi
        mov    dword ptr [ebp-0x8], ecx
      L14:
        mov    eax, dword ptr [ebp+0x10]
        mov    ecx, dword ptr g_clip_y1
        cmp    eax, ecx
        jg     L24
        mov    esi, dword ptr [ebp-0x18]
        cmp    eax, esi
        jge    L24
        mov    ebx, dword ptr g_clip_y0
        mov    ecx, dword ptr [ebp+0x10]
        mov    edx, dword ptr [ebp-0x18]
        sub    ebx, ecx
        jle    L16
        sub    edx, ecx
        jle    L16
        cmp    ebx, edx
        mov    ecx, edx
        jns    L15
        mov    ecx, ebx
      L15:
        add    dword ptr [ebp+0x10], ecx
        mov    eax, dword ptr [ebp-0x1c]
        mul    ecx
        add    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x34]
        mul    ecx
        add    dword ptr [ebp+0x8], eax
        mov    eax, dword ptr [ebp-0x20]
        mul    ecx
        add    dword ptr [ebp-0x4], eax
        mov    eax, dword ptr [ebp-0x38]
        mul    ecx
        add    dword ptr [ebp-0x8], eax
        mov    eax, dword ptr g_pitch
        mul    ecx
        add    dword ptr [ebp-0xc], eax
      L16:
        cmp    dword ptr [ebp+0x10], esi
        jge    L24
      L17:
        mov    edx, dword ptr [ebp+0x10]
        mov    eax, dword ptr g_clip_y1
        cmp    edx, eax
        jg     L24
        mov    eax, dword ptr [ebp-0xc]
        mov    ecx, dword ptr g_pitch
        mov    esi, dword ptr [ebp+0xc]
        mov    edi, dword ptr [ebp+0x8]
        mov    dword ptr [ebp-0x4c], eax
        add    eax, ecx
        cmp    esi, edi
        mov    dword ptr [ebp-0xc], eax
        jle    L18
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0xc]
        sub    ecx, dword ptr [ebp+0x8]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x10], eax
        mov    eax, dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp-0x8]
        sub    eax, ecx
        mov    dword ptr [ebp-0x14], ecx
        mov    ecx, dword ptr [ebp-0x10]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x2c], eax
        sar    edi, 0x10
        sar    esi, 0x10
        mov    dword ptr [ebp-0x30], edi
        mov    dword ptr [ebp-0x28], esi
        jmp    L19
      L18:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0x8]
        sub    ecx, dword ptr [ebp+0xc]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x10], eax
        mov    eax, dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp-0x4]
        sub    eax, ecx
        mov    dword ptr [ebp-0x14], ecx
        mov    ecx, dword ptr [ebp-0x10]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x2c], eax
        sar    esi, 0x10
        sar    edi, 0x10
        mov    dword ptr [ebp-0x30], esi
        mov    dword ptr [ebp-0x28], edi
      L19:
        push   esi
        push   edi
        mov    ecx, dword ptr [ebp-0x30]
        mov    edi, ecx
        mov    ebx, dword ptr g_clip_x0
        sub    ecx, ebx
        jns    L20
        mov    edi, ebx
        neg    ecx
        mov    eax, dword ptr [ebp-0x2c]
        mul    ecx
        add    dword ptr [ebp-0x14], eax
      L20:
        mov    esi, edi
        shl    esi, 1
        add    esi, dword ptr [ebp-0x4c]
        mov    eax, edi
        mov    edi, dword ptr g_clip_x1
        shl    edi, 1
        add    edi, dword ptr [ebp-0x4c]
        cmp    eax, dword ptr [ebp-0x28]
        jge    L23
        cmp    esi, edi
        jg     L23
      L21:
        mov    ebx, dword ptr g_zbuf
        mov    ecx, dword ptr [ebp+0x10]
        lea    ebx, [ebx + eax*4]
        shl    ecx, 9
        mov    edx, dword ptr [ebp-0x14]
        cmp    edx, dword ptr [ebx + ecx]
        jb     L22
        cmp    esi, dword ptr g_mouse_pixel
        mov    dword ptr [ebx + ecx], edx
        mov    edx, dword ptr [ebp-0x50]
        sete   cl
        mov    word ptr [esi], dx
        or     byte ptr g_raster_hit, cl
      L22:
        inc    eax
        mov    ebx, dword ptr [ebp-0x2c]
        add    dword ptr [ebp-0x14], ebx
        add    esi, 2
        cmp    eax, dword ptr [ebp-0x28]
        jge    L23
        cmp    esi, edi
        jg     L23
        jmp    L21
      L23:
        pop    edi
        pop    esi
        mov    eax, dword ptr [ebp-0x1c]
        mov    ecx, dword ptr [ebp+0xc]
        mov    esi, dword ptr [ebp-0x8]
        mov    ebx, dword ptr [ebp+0x8]
        mov    edx, dword ptr [ebp-0x20]
        mov    edi, dword ptr [ebp-0x4]
        add    ecx, eax
        mov    eax, dword ptr [ebp-0x38]
        mov    dword ptr [ebp+0xc], ecx
        mov    ecx, dword ptr [ebp-0x34]
        add    esi, eax
        mov    eax, dword ptr [ebp+0x10]
        add    ebx, ecx
        mov    ecx, dword ptr [ebp-0x18]
        add    edi, edx
        inc    eax
        cmp    eax, ecx
        mov    dword ptr [ebp+0x8], ebx
        mov    dword ptr [ebp-0x4], edi
        mov    dword ptr [ebp-0x8], esi
        mov    dword ptr [ebp+0x10], eax
        jl     L17
      L24:
        pop    edi
        pop    esi
        pop    ebx
        mov    esp, ebp
        pop    ebp
        ret
    }
}

/* -------------------------------------------------------------------------
 * 0x00486590 -- DrawGouraudTri: Gouraud-shaded, Z-buffered triangle.
 * ------------------------------------------------------------------------- */

__declspec(naked)
// FUNCTION: LEGOLAND 0x00486590
void DrawGouraudTri(Vertex2D* a, Vertex2D* b, Vertex2D* c)
{
    __asm {
        push   ebp
        mov    ebp, esp
        sub    esp, 0x6c
        mov    edx, dword ptr [ebp+0x8]
        mov    ecx, dword ptr [ebp+0xc]
        push   ebx
        push   esi
        mov    eax, dword ptr [edx + 4]
        mov    esi, dword ptr [ecx + 4]
        cmp    eax, esi
        push   edi
        jle    L0
        mov    eax, dword ptr [ebp+0x8]
        xchg   dword ptr [ebp+0xc], eax
        mov    dword ptr [ebp+0x8], eax
        mov    ecx, dword ptr [ebp+0xc]
        mov    edx, dword ptr [ebp+0x8]
      L0:
        mov    ebx, dword ptr [ebp+0x10]
        mov    eax, dword ptr [ecx + 4]
        cmp    eax, dword ptr [ebx + 4]
        jle    L1
        mov    eax, dword ptr [ebp+0xc]
        xchg   dword ptr [ebp+0x10], eax
        mov    dword ptr [ebp+0xc], eax
        mov    ebx, dword ptr [ebp+0x10]
        mov    ecx, dword ptr [ebp+0xc]
      L1:
        mov    eax, dword ptr [edx + 4]
        mov    esi, dword ptr [ecx + 4]
        cmp    eax, esi
        jle    L2
        mov    eax, dword ptr [ebp+0x8]
        xchg   dword ptr [ebp+0xc], eax
        mov    dword ptr [ebp+0x8], eax
        mov    ecx, dword ptr [ebp+0xc]
        mov    edx, dword ptr [ebp+0x8]
      L2:
        mov    edi, dword ptr [ebx + 4]
        mov    eax, dword ptr [edx + 4]
        sar    edi, 0x10
        mov    dword ptr [ebp-0x28], edi
        mov    edi, dword ptr g_pitch
        sar    eax, 0x10
        imul   edi, eax
        add    edi, dword ptr g_surface
        mov    esi, dword ptr [ecx + 4]
        sar    esi, 0x10
        mov    dword ptr [ebp-0x18], edi
        mov    edi, dword ptr [edx]
        mov    dword ptr [ebp+0x8], edi
        mov    edi, dword ptr [ecx]
        mov    dword ptr [ebp-0x64], edi
        mov    edi, dword ptr [ebx]
        mov    dword ptr [ebp-0x5c], edi
        mov    edi, dword ptr [edx + 8]
        mov    edx, dword ptr [edx + 0x14]
        mov    dword ptr [ebp-0x24], edi
        mov    edi, dword ptr [ecx + 8]
        mov    ecx, dword ptr [ecx + 0x14]
        mov    dword ptr [ebp-0x68], edi
        mov    edi, dword ptr [ebx + 8]
        mov    dword ptr [ebp-0x14], edx
        mov    edx, dword ptr [ebx + 0x14]
        mov    dword ptr [ebp-0x54], eax
        mov    dword ptr [ebp-0x40], esi
        mov    dword ptr [ebp-0x60], edi
        mov    dword ptr [ebp-0x30], ecx
        mov    dword ptr [ebp-0x2c], edx
        shl    dword ptr [ebp-0x14], 6
        shl    dword ptr [ebp-0x30], 6
        shl    dword ptr [ebp-0x2c], 6
        mov    edi, eax
        cmp    eax, esi
        mov    dword ptr [ebp+0x10], edi
        je     L13
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x40]
        sub    ecx, dword ptr [ebp-0x54]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x58], eax
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x28]
        sub    ecx, dword ptr [ebp-0x54]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x64]
        sub    eax, dword ptr [ebp+0x8]
        mov    ecx, dword ptr [ebp-0x58]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x34], eax
        mov    eax, dword ptr [ebp-0x5c]
        sub    eax, dword ptr [ebp+0x8]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x50], eax
        mov    eax, dword ptr [ebp-0x30]
        sub    eax, dword ptr [ebp-0x14]
        mov    ecx, dword ptr [ebp-0x58]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x38], eax
        mov    eax, dword ptr [ebp-0x2c]
        sub    eax, dword ptr [ebp-0x14]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x54], eax
        mov    eax, dword ptr [ebp-0x68]
        sub    eax, dword ptr [ebp-0x24]
        mov    ecx, dword ptr [ebp-0x58]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x3c], eax
        mov    eax, dword ptr [ebp-0x60]
        sub    eax, dword ptr [ebp-0x24]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x58], eax
        mov    eax, dword ptr [ebp+0x8]
        mov    dword ptr [ebp+0xc], eax
        mov    dword ptr [ebp+0x8], eax
        mov    eax, dword ptr [ebp-0x14]
        mov    dword ptr [ebp-0x4], eax
        mov    dword ptr [ebp-0x8], eax
        mov    eax, dword ptr [ebp-0x24]
        mov    dword ptr [ebp-0xc], eax
        mov    dword ptr [ebp-0x10], eax
        mov    ebx, dword ptr g_clip_y0
        mov    ecx, dword ptr [ebp+0x10]
        mov    edx, dword ptr [ebp-0x40]
        sub    ebx, ecx
        jle    L4
        sub    edx, ecx
        jle    L4
        cmp    ebx, edx
        mov    ecx, edx
        jns    L3
        mov    ecx, ebx
      L3:
        add    dword ptr [ebp+0x10], ecx
        mov    eax, dword ptr [ebp-0x34]
        mul    ecx
        add    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x50]
        mul    ecx
        add    dword ptr [ebp+0x8], eax
        mov    eax, dword ptr [ebp-0x38]
        mul    ecx
        add    dword ptr [ebp-0x4], eax
        mov    eax, dword ptr [ebp-0x54]
        mul    ecx
        add    dword ptr [ebp-0x8], eax
        mov    eax, dword ptr [ebp-0x3c]
        mul    ecx
        add    dword ptr [ebp-0xc], eax
        mov    eax, dword ptr [ebp-0x58]
        mul    ecx
        add    dword ptr [ebp-0x10], eax
        mov    eax, dword ptr g_pitch
        mul    ecx
        add    dword ptr [ebp-0x18], eax
      L4:
        cmp    dword ptr [ebp+0x10], esi
        jge    L12
      L5:
        mov    eax, dword ptr [ebp+0x10]
        mov    ecx, dword ptr g_clip_y1
        cmp    eax, ecx
        jg     L12
        mov    eax, dword ptr [ebp-0x18]
        mov    ecx, dword ptr g_pitch
        mov    esi, dword ptr [ebp+0xc]
        mov    edi, dword ptr [ebp+0x8]
        mov    dword ptr [ebp-0x6c], eax
        add    eax, ecx
        cmp    esi, edi
        mov    dword ptr [ebp-0x18], eax
        jle    L6
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0xc]
        sub    ecx, dword ptr [ebp+0x8]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x14], eax
        mov    eax, dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp-0x8]
        sub    eax, ecx
        mov    dword ptr [ebp-0x24], ecx
        mov    ecx, dword ptr [ebp-0x14]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x48], eax
        mov    eax, dword ptr [ebp-0xc]
        mov    ecx, dword ptr [ebp-0x10]
        sub    eax, ecx
        mov    dword ptr [ebp-0x20], ecx
        mov    ecx, dword ptr [ebp-0x14]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x44], eax
        sar    edi, 0x10
        sar    esi, 0x10
        mov    dword ptr [ebp-0x4c], edi
        mov    dword ptr [ebp-0x1c], esi
        jmp    L7
      L6:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0x8]
        sub    ecx, dword ptr [ebp+0xc]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x14], eax
        mov    eax, dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp-0x4]
        sub    eax, ecx
        mov    dword ptr [ebp-0x24], ecx
        mov    ecx, dword ptr [ebp-0x14]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x48], eax
        mov    eax, dword ptr [ebp-0x10]
        mov    ecx, dword ptr [ebp-0xc]
        sub    eax, ecx
        mov    dword ptr [ebp-0x20], ecx
        mov    ecx, dword ptr [ebp-0x14]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x44], eax
        sar    esi, 0x10
        sar    edi, 0x10
        mov    dword ptr [ebp-0x4c], esi
        mov    dword ptr [ebp-0x1c], edi
      L7:
        push   esi
        push   edi
        mov    ecx, dword ptr [ebp-0x4c]
        mov    edi, ecx
        mov    ebx, dword ptr g_clip_x0
        sub    ecx, ebx
        jns    L8
        mov    edi, ebx
        neg    ecx
        mov    eax, dword ptr [ebp-0x48]
        mul    ecx
        add    dword ptr [ebp-0x24], eax
        mov    eax, dword ptr [ebp-0x44]
        mul    ecx
        add    dword ptr [ebp-0x20], eax
      L8:
        mov    esi, edi
        shl    esi, 1
        add    esi, dword ptr [ebp-0x6c]
        mov    eax, edi
        mov    edi, dword ptr g_clip_x1
        shl    edi, 1
        add    edi, dword ptr [ebp-0x6c]
        cmp    eax, dword ptr [ebp-0x1c]
        jge    L11
        cmp    esi, edi
        jg     L11
      L9:
        mov    ebx, dword ptr g_zbuf
        mov    ecx, dword ptr [ebp+0x10]
        lea    ebx, [ebx + eax*4]
        shl    ecx, 9
        mov    edx, dword ptr [ebp-0x20]
        cmp    edx, dword ptr [ebx + ecx]
        jb     L10
        mov    dword ptr [ebx + ecx], edx
        mov    ebx, dword ptr g_flat_colour
        mov    ebx, dword ptr [ebx + 4]
        cmp    esi, dword ptr g_mouse_pixel
        movzx  edx, word ptr [ebp-0x22]
        sete   cl
        movzx  edx, word ptr [ebx + edx*2]
        or     byte ptr g_raster_hit, cl
        mov    word ptr [esi], dx
      L10:
        inc    eax
        mov    ebx, dword ptr [ebp-0x48]
        add    dword ptr [ebp-0x24], ebx
        mov    ebx, dword ptr [ebp-0x44]
        add    dword ptr [ebp-0x20], ebx
        add    esi, 2
        cmp    eax, dword ptr [ebp-0x1c]
        jge    L11
        cmp    esi, edi
        jg     L11
        jmp    L9
      L11:
        pop    edi
        pop    esi
        mov    ecx, dword ptr [ebp-0x34]
        mov    edi, dword ptr [ebp+0xc]
        mov    edx, dword ptr [ebp-0x50]
        mov    esi, dword ptr [ebp+0x8]
        mov    eax, dword ptr [ebp-0x38]
        mov    ebx, dword ptr [ebp-0x8]
        add    edi, ecx
        mov    ecx, dword ptr [ebp-0x4]
        add    esi, edx
        mov    edx, dword ptr [ebp-0x3c]
        mov    dword ptr [ebp+0x8], esi
        mov    esi, dword ptr [ebp-0x10]
        add    ecx, eax
        mov    eax, dword ptr [ebp-0x58]
        mov    dword ptr [ebp+0xc], edi
        mov    edi, dword ptr [ebp-0xc]
        mov    dword ptr [ebp-0x4], ecx
        mov    ecx, dword ptr [ebp-0x54]
        add    esi, eax
        mov    eax, dword ptr [ebp+0x10]
        add    ebx, ecx
        mov    ecx, dword ptr [ebp-0x40]
        add    edi, edx
        inc    eax
        cmp    eax, ecx
        mov    dword ptr [ebp-0x8], ebx
        mov    dword ptr [ebp-0xc], edi
        mov    dword ptr [ebp-0x10], esi
        mov    dword ptr [ebp+0x10], eax
        jl     L5
      L12:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x28]
        sub    ecx, dword ptr [ebp-0x40]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x1c], eax
        mov    eax, dword ptr [ebp-0x5c]
        sub    eax, dword ptr [ebp-0x64]
        mov    ecx, dword ptr [ebp-0x1c]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x34], eax
        mov    eax, dword ptr [ebp-0x2c]
        sub    eax, dword ptr [ebp-0x30]
        mov    ecx, dword ptr [ebp-0x1c]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x38], eax
        mov    eax, dword ptr [ebp-0x60]
        sub    eax, dword ptr [ebp-0x68]
        mov    ecx, dword ptr [ebp-0x1c]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x3c], eax
        mov    edi, dword ptr [ebp+0x10]
        jmp    L14
      L13:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x28]
        sub    ecx, dword ptr [ebp-0x54]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x28]
        sub    ecx, dword ptr [ebp-0x40]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x1c], eax
        mov    eax, dword ptr [ebp-0x5c]
        sub    eax, dword ptr [ebp+0x8]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x34], eax
        mov    eax, dword ptr [ebp-0x5c]
        sub    eax, dword ptr [ebp-0x64]
        mov    ecx, dword ptr [ebp-0x1c]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x50], eax
        mov    eax, dword ptr [ebp-0x2c]
        sub    eax, dword ptr [ebp-0x14]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x38], eax
        mov    eax, dword ptr [ebp-0x2c]
        sub    eax, dword ptr [ebp-0x30]
        mov    ecx, dword ptr [ebp-0x1c]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x54], eax
        mov    eax, dword ptr [ebp-0x60]
        sub    eax, dword ptr [ebp-0x24]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x3c], eax
        mov    eax, dword ptr [ebp-0x60]
        sub    eax, dword ptr [ebp-0x68]
        mov    ecx, dword ptr [ebp-0x1c]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x58], eax
        mov    ecx, dword ptr [ebp+0x8]
        mov    edx, dword ptr [ebp-0x64]
        mov    eax, dword ptr [ebp-0x14]
        mov    dword ptr [ebp+0xc], ecx
        mov    ecx, dword ptr [ebp-0x30]
        mov    dword ptr [ebp+0x8], edx
        mov    edx, dword ptr [ebp-0x24]
        mov    dword ptr [ebp-0x4], eax
        mov    eax, dword ptr [ebp-0x68]
        mov    dword ptr [ebp-0x8], ecx
        mov    dword ptr [ebp-0xc], edx
        mov    dword ptr [ebp-0x10], eax
      L14:
        cmp    edi, dword ptr g_clip_y1
        jg     L24
        mov    esi, dword ptr [ebp-0x28]
        cmp    edi, esi
        jge    L24
        mov    ebx, dword ptr g_clip_y0
        mov    ecx, dword ptr [ebp+0x10]
        mov    edx, dword ptr [ebp-0x28]
        sub    ebx, ecx
        jle    L16
        sub    edx, ecx
        jle    L16
        cmp    ebx, edx
        mov    ecx, edx
        jns    L15
        mov    ecx, ebx
      L15:
        add    dword ptr [ebp+0x10], ecx
        mov    eax, dword ptr [ebp-0x34]
        mul    ecx
        add    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x50]
        mul    ecx
        add    dword ptr [ebp+0x8], eax
        mov    eax, dword ptr [ebp-0x38]
        mul    ecx
        add    dword ptr [ebp-0x4], eax
        mov    eax, dword ptr [ebp-0x54]
        mul    ecx
        add    dword ptr [ebp-0x8], eax
        mov    eax, dword ptr [ebp-0x3c]
        mul    ecx
        add    dword ptr [ebp-0xc], eax
        mov    eax, dword ptr [ebp-0x58]
        mul    ecx
        add    dword ptr [ebp-0x10], eax
        mov    eax, dword ptr g_pitch
        mul    ecx
        add    dword ptr [ebp-0x18], eax
      L16:
        cmp    dword ptr [ebp+0x10], esi
        jge    L24
      L17:
        mov    ecx, dword ptr [ebp+0x10]
        mov    eax, dword ptr g_clip_y1
        cmp    ecx, eax
        jg     L24
        mov    eax, dword ptr [ebp-0x18]
        mov    ecx, dword ptr g_pitch
        mov    esi, dword ptr [ebp+0xc]
        mov    edi, dword ptr [ebp+0x8]
        mov    dword ptr [ebp-0x6c], eax
        add    eax, ecx
        cmp    esi, edi
        mov    dword ptr [ebp-0x18], eax
        jle    L18
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0xc]
        sub    ecx, dword ptr [ebp+0x8]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x14], eax
        mov    eax, dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp-0x8]
        sub    eax, ecx
        mov    dword ptr [ebp-0x24], ecx
        mov    ecx, dword ptr [ebp-0x14]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x48], eax
        mov    eax, dword ptr [ebp-0xc]
        mov    ecx, dword ptr [ebp-0x10]
        sub    eax, ecx
        mov    dword ptr [ebp-0x20], ecx
        mov    ecx, dword ptr [ebp-0x14]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x44], eax
        sar    edi, 0x10
        sar    esi, 0x10
        mov    dword ptr [ebp-0x4c], edi
        mov    dword ptr [ebp-0x1c], esi
        jmp    L19
      L18:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0x8]
        sub    ecx, dword ptr [ebp+0xc]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x14], eax
        mov    eax, dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp-0x4]
        sub    eax, ecx
        mov    dword ptr [ebp-0x24], ecx
        mov    ecx, dword ptr [ebp-0x14]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x48], eax
        mov    eax, dword ptr [ebp-0x10]
        mov    ecx, dword ptr [ebp-0xc]
        sub    eax, ecx
        mov    dword ptr [ebp-0x20], ecx
        mov    ecx, dword ptr [ebp-0x14]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x44], eax
        sar    esi, 0x10
        sar    edi, 0x10
        mov    dword ptr [ebp-0x4c], esi
        mov    dword ptr [ebp-0x1c], edi
      L19:
        push   esi
        push   edi
        mov    ecx, dword ptr [ebp-0x4c]
        mov    edi, ecx
        mov    ebx, dword ptr g_clip_x0
        sub    ecx, ebx
        jns    L20
        mov    edi, ebx
        neg    ecx
        mov    eax, dword ptr [ebp-0x48]
        mul    ecx
        add    dword ptr [ebp-0x24], eax
        mov    eax, dword ptr [ebp-0x44]
        mul    ecx
        add    dword ptr [ebp-0x20], eax
      L20:
        mov    esi, edi
        shl    esi, 1
        add    esi, dword ptr [ebp-0x6c]
        mov    eax, edi
        mov    edi, dword ptr g_clip_x1
        shl    edi, 1
        add    edi, dword ptr [ebp-0x6c]
        cmp    eax, dword ptr [ebp-0x1c]
        jge    L23
        cmp    esi, edi
        jg     L23
      L21:
        mov    ebx, dword ptr g_zbuf
        mov    ecx, dword ptr [ebp+0x10]
        lea    ebx, [ebx + eax*4]
        shl    ecx, 9
        mov    edx, dword ptr [ebp-0x20]
        cmp    edx, dword ptr [ebx + ecx]
        jb     L22
        mov    dword ptr [ebx + ecx], edx
        mov    ebx, dword ptr g_flat_colour
        mov    ebx, dword ptr [ebx + 4]
        cmp    esi, dword ptr g_mouse_pixel
        movzx  edx, word ptr [ebp-0x22]
        sete   cl
        movzx  edx, word ptr [ebx + edx*2]
        or     byte ptr g_raster_hit, cl
        mov    word ptr [esi], dx
      L22:
        inc    eax
        mov    ebx, dword ptr [ebp-0x48]
        add    dword ptr [ebp-0x24], ebx
        mov    ebx, dword ptr [ebp-0x44]
        add    dword ptr [ebp-0x20], ebx
        add    esi, 2
        cmp    eax, dword ptr [ebp-0x1c]
        jge    L23
        cmp    esi, edi
        jg     L23
        jmp    L21
      L23:
        pop    edi
        pop    esi
        mov    eax, dword ptr [ebp-0x50]
        mov    ecx, dword ptr [ebp+0x8]
        mov    edx, dword ptr [ebp-0x34]
        mov    esi, dword ptr [ebp+0xc]
        add    ecx, eax
        mov    eax, dword ptr [ebp-0x4]
        mov    edi, dword ptr [ebp-0xc]
        mov    dword ptr [ebp+0x8], ecx
        mov    ecx, dword ptr [ebp-0x38]
        mov    ebx, dword ptr [ebp-0x8]
        add    eax, ecx
        mov    ecx, dword ptr [ebp-0x58]
        add    esi, edx
        mov    edx, dword ptr [ebp-0x54]
        mov    dword ptr [ebp-0x4], eax
        mov    eax, dword ptr [ebp-0x3c]
        mov    dword ptr [ebp+0xc], esi
        mov    esi, dword ptr [ebp-0x10]
        add    edi, eax
        mov    eax, dword ptr [ebp+0x10]
        add    ebx, edx
        add    esi, ecx
        mov    ecx, dword ptr [ebp-0x28]
        inc    eax
        cmp    eax, ecx
        mov    dword ptr [ebp-0x8], ebx
        mov    dword ptr [ebp-0xc], edi
        mov    dword ptr [ebp-0x10], esi
        mov    dword ptr [ebp+0x10], eax
        jl     L17
      L24:
        pop    edi
        pop    esi
        pop    ebx
        mov    esp, ebp
        pop    ebp
        ret
    }
}

/* -------------------------------------------------------------------------
 * 0x00487d40 -- DrawFlatTexTri: textured, single shade level, Z-buffered.
 * ------------------------------------------------------------------------- */

__declspec(naked)
// FUNCTION: LEGOLAND 0x00487d40
void DrawFlatTexTri(Vertex2D* a, Vertex2D* b, Vertex2D* c)
{
    __asm {
        push   ebp
        mov    ebp, esp
        sub    esp, 0x90
        mov    ecx, dword ptr [ebp+0x8]
        push   ebx
        push   esi
        push   edi
        mov    eax, dword ptr [ecx + 0x14]
        mov    edx, dword ptr [ecx + 4]
        shl    eax, 6
        mov    dword ptr [ebp-0x90], eax
        mov    eax, dword ptr [ebp+0xc]
        mov    dword ptr [ebp-0x14], 0x47800000
        cmp    edx, dword ptr [eax + 4]
        jle    L0
        mov    eax, dword ptr [ebp+0x8]
        xchg   dword ptr [ebp+0xc], eax
        mov    dword ptr [ebp+0x8], eax
        mov    eax, dword ptr [ebp+0xc]
        mov    ecx, dword ptr [ebp+0x8]
      L0:
        mov    ebx, dword ptr [ebp+0x10]
        mov    edx, dword ptr [eax + 4]
        cmp    edx, dword ptr [ebx + 4]
        jle    L1
        mov    eax, dword ptr [ebp+0xc]
        xchg   dword ptr [ebp+0x10], eax
        mov    dword ptr [ebp+0xc], eax
        mov    ebx, dword ptr [ebp+0x10]
        mov    eax, dword ptr [ebp+0xc]
      L1:
        mov    edx, dword ptr [ecx + 4]
        mov    esi, dword ptr [eax + 4]
        cmp    edx, esi
        jle    L2
        mov    eax, dword ptr [ebp+0x8]
        xchg   dword ptr [ebp+0xc], eax
        mov    dword ptr [ebp+0x8], eax
        mov    eax, dword ptr [ebp+0xc]
        mov    ecx, dword ptr [ebp+0x8]
      L2:
        mov    edx, dword ptr [ebx + 4]
        mov    esi, dword ptr [ecx + 4]
        sar    edx, 0x10
        mov    dword ptr [ebp-0x38], edx
        mov    edx, dword ptr g_pitch
        sar    esi, 0x10
        imul   edx, esi
        add    edx, dword ptr g_surface
        mov    edi, dword ptr [eax + 4]
        sar    edi, 0x10
        mov    dword ptr [ebp-0x34], edx
        mov    edx, dword ptr [ecx]
        mov    dword ptr [ebp-0x28], edx
        mov    edx, dword ptr [eax]
        mov    ecx, dword ptr [ecx + 8]
        mov    dword ptr [ebp-0x88], edx
        mov    edx, dword ptr [ebx]
        mov    dword ptr [ebp-0x30], esi
        mov    dword ptr [ebp-0x7c], edx
        mov    edx, dword ptr [eax + 8]
        mov    eax, dword ptr [ebx + 8]
        mov    dword ptr [ebp-0x6c], edi
        mov    dword ptr [ebp-0x20], ecx
        mov    dword ptr [ebp-0x80], edx
        mov    dword ptr [ebp-0x84], eax
        mov    ecx, dword ptr [ebp+0x8]
        fld    dword ptr [ecx + 0xc]
        fmul   dword ptr [ebp-0x14]
        fistp  dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp+0xc]
        fld    dword ptr [ecx + 0xc]
        fmul   dword ptr [ebp-0x14]
        fistp  dword ptr [ebp-0x58]
        mov    ecx, dword ptr [ebp+0x10]
        fld    dword ptr [ecx + 0xc]
        fmul   dword ptr [ebp-0x14]
        fistp  dword ptr [ebp-0x4c]
        mov    ecx, dword ptr [ebp+0x8]
        fld    dword ptr [ecx + 0x10]
        fmul   dword ptr [ebp-0x14]
        fistp  dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp+0xc]
        fld    dword ptr [ecx + 0x10]
        fmul   dword ptr [ebp-0x14]
        fistp  dword ptr [ebp-0x54]
        mov    ecx, dword ptr [ebp+0x10]
        fld    dword ptr [ecx + 0x10]
        fmul   dword ptr [ebp-0x14]
        fistp  dword ptr [ebp-0x44]
        mov    ebx, dword ptr g_texture
        mov    ecx, dword ptr [ebx]
        mov    edx, dword ptr [ebp-0x4]
        and    edx, 0xffff
        shl    edx, cl
        mov    dword ptr [ebp-0x4], edx
        mov    ecx, dword ptr [ebx + 4]
        mov    eax, dword ptr [ebp-0x8]
        and    eax, 0xffff
        shl    eax, cl
        mov    dword ptr [ebp-0x8], eax
        mov    ebx, dword ptr g_texture
        mov    ecx, dword ptr [ebx]
        mov    edx, dword ptr [ebp-0x58]
        and    edx, 0xffff
        shl    edx, cl
        mov    dword ptr [ebp-0x58], edx
        mov    ecx, dword ptr [ebx + 4]
        mov    eax, dword ptr [ebp-0x54]
        and    eax, 0xffff
        shl    eax, cl
        mov    dword ptr [ebp-0x54], eax
        mov    ebx, dword ptr g_texture
        mov    ecx, dword ptr [ebx]
        mov    edx, dword ptr [ebp-0x4c]
        and    edx, 0xffff
        shl    edx, cl
        mov    dword ptr [ebp-0x4c], edx
        mov    ecx, dword ptr [ebx + 4]
        mov    eax, dword ptr [ebp-0x44]
        and    eax, 0xffff
        shl    eax, cl
        mov    dword ptr [ebp-0x44], eax
        cmp    esi, edi
        mov    dword ptr [ebp-0x10], esi
        je     L13
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x6c]
        sub    ecx, dword ptr [ebp-0x30]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0x8], eax
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x38]
        sub    ecx, dword ptr [ebp-0x30]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x88]
        sub    eax, dword ptr [ebp-0x28]
        mov    ecx, dword ptr [ebp+0x8]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x3c], eax
        mov    eax, dword ptr [ebp-0x7c]
        sub    eax, dword ptr [ebp-0x28]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x74], eax
        mov    eax, dword ptr [ebp-0x58]
        sub    eax, dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp+0x8]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x40], eax
        mov    eax, dword ptr [ebp-0x4c]
        sub    eax, dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x68], eax
        mov    eax, dword ptr [ebp-0x54]
        sub    eax, dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp+0x8]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x48], eax
        mov    eax, dword ptr [ebp-0x44]
        sub    eax, dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x70], eax
        mov    eax, dword ptr [ebp-0x80]
        sub    eax, dword ptr [ebp-0x20]
        mov    ecx, dword ptr [ebp+0x8]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x50], eax
        mov    eax, dword ptr [ebp-0x84]
        sub    eax, dword ptr [ebp-0x20]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x30], eax
        mov    eax, dword ptr [ebp-0x28]
        mov    dword ptr [ebp+0x8], eax
        mov    dword ptr [ebp+0x10], eax
        mov    eax, dword ptr [ebp-0x4]
        mov    dword ptr [ebp-0x18], eax
        mov    dword ptr [ebp-0x2c], eax
        mov    eax, dword ptr [ebp-0x8]
        mov    dword ptr [ebp-0x1c], eax
        mov    dword ptr [ebp-0x24], eax
        mov    eax, dword ptr [ebp-0x20]
        mov    dword ptr [ebp-0x8], eax
        mov    dword ptr [ebp-0x4], eax
        mov    ebx, dword ptr g_clip_y0
        mov    ecx, dword ptr [ebp-0x10]
        mov    edx, dword ptr [ebp-0x6c]
        sub    ebx, ecx
        jle    L4
        sub    edx, ecx
        jle    L4
        cmp    ebx, edx
        mov    ecx, edx
        jns    L3
        mov    ecx, ebx
      L3:
        add    dword ptr [ebp-0x10], ecx
        mov    eax, dword ptr [ebp-0x3c]
        mul    ecx
        add    dword ptr [ebp+0x8], eax
        mov    eax, dword ptr [ebp-0x74]
        mul    ecx
        add    dword ptr [ebp+0x10], eax
        mov    eax, dword ptr [ebp-0x40]
        mul    ecx
        add    dword ptr [ebp-0x18], eax
        mov    eax, dword ptr [ebp-0x68]
        mul    ecx
        add    dword ptr [ebp-0x2c], eax
        mov    eax, dword ptr [ebp-0x48]
        mul    ecx
        add    dword ptr [ebp-0x1c], eax
        mov    eax, dword ptr [ebp-0x70]
        mul    ecx
        add    dword ptr [ebp-0x24], eax
        mov    eax, dword ptr [ebp-0x50]
        mul    ecx
        add    dword ptr [ebp-0x8], eax
        mov    eax, dword ptr [ebp-0x30]
        mul    ecx
        add    dword ptr [ebp-0x4], eax
        mov    eax, dword ptr g_pitch
        mul    ecx
        add    dword ptr [ebp-0x34], eax
      L4:
        cmp    dword ptr [ebp-0x10], edi
        jge    L12
      L5:
        mov    ecx, dword ptr [ebp-0x10]
        mov    eax, dword ptr g_clip_y1
        cmp    ecx, eax
        jg     L12
        mov    eax, dword ptr [ebp-0x34]
        mov    ecx, dword ptr g_pitch
        mov    esi, dword ptr [ebp+0x8]
        mov    edi, dword ptr [ebp+0x10]
        mov    dword ptr [ebp-0x8c], eax
        add    eax, ecx
        cmp    esi, edi
        mov    dword ptr [ebp-0x34], eax
        jle    L6
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0x8]
        sub    ecx, dword ptr [ebp+0x10]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x18]
        mov    ecx, dword ptr [ebp-0x2c]
        sub    eax, ecx
        mov    dword ptr [ebp-0x14], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x78], eax
        mov    eax, dword ptr [ebp-0x1c]
        mov    ecx, dword ptr [ebp-0x24]
        sub    eax, ecx
        mov    dword ptr [ebp-0x28], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x64], eax
        mov    eax, dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp-0x4]
        sub    eax, ecx
        mov    dword ptr [ebp-0x20], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x60], eax
        sar    edi, 0x10
        sar    esi, 0x10
        mov    dword ptr [ebp-0x5c], edi
        mov    dword ptr [ebp-0xc], esi
        jmp    L7
      L6:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0x10]
        sub    ecx, dword ptr [ebp+0x8]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x2c]
        mov    ecx, dword ptr [ebp-0x18]
        sub    eax, ecx
        mov    dword ptr [ebp-0x14], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x78], eax
        mov    eax, dword ptr [ebp-0x24]
        mov    ecx, dword ptr [ebp-0x1c]
        sub    eax, ecx
        mov    dword ptr [ebp-0x28], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x64], eax
        mov    eax, dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp-0x8]
        sub    eax, ecx
        mov    dword ptr [ebp-0x20], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x60], eax
        sar    esi, 0x10
        sar    edi, 0x10
        mov    dword ptr [ebp-0x5c], esi
        mov    dword ptr [ebp-0xc], edi
      L7:
        push   esi
        push   edi
        mov    ecx, dword ptr [ebp-0x5c]
        mov    edi, ecx
        mov    ebx, dword ptr g_clip_x0
        sub    ecx, ebx
        jns    L8
        mov    edi, ebx
        neg    ecx
        mov    eax, dword ptr [ebp-0x78]
        mul    ecx
        add    dword ptr [ebp-0x14], eax
        mov    eax, dword ptr [ebp-0x64]
        mul    ecx
        add    dword ptr [ebp-0x28], eax
        mov    eax, dword ptr [ebp-0x60]
        mul    ecx
        add    dword ptr [ebp-0x20], eax
      L8:
        mov    esi, edi
        shl    esi, 1
        add    esi, dword ptr [ebp-0x8c]
        mov    eax, edi
        mov    edi, dword ptr g_clip_x1
        shl    edi, 1
        add    edi, dword ptr [ebp-0x8c]
        cmp    eax, dword ptr [ebp-0xc]
        jge    L11
        cmp    esi, edi
        jg     L11
      L9:
        mov    ebx, dword ptr g_zbuf
        mov    ecx, dword ptr [ebp-0x10]
        lea    ebx, [ebx + eax*4]
        shl    ecx, 9
        mov    edx, dword ptr [ebp-0x20]
        cmp    edx, dword ptr [ebx + ecx]
        jb     L10
        mov    dword ptr [ebx + ecx], edx
        mov    ebx, dword ptr g_texture
        mov    ecx, dword ptr [ebx]
        movzx  edx, word ptr [ebp-0x26]
        shl    edx, cl
        add    edx, dword ptr [ebx + 8]
        mov    ebx, dword ptr [ebx + 0xc]
        movzx  ecx, word ptr [ebp-0x12]
        add    edx, ecx
        movzx  edx, byte ptr [edx]
        mov    ebx, dword ptr [ebx + edx*4]
        mov    ebx, dword ptr [ebx + 4]
        cmp    esi, dword ptr g_mouse_pixel
        movzx  edx, word ptr [ebp-0x8e]
        sete   cl
        movzx  edx, word ptr [ebx + edx*2]
        or     byte ptr g_raster_hit, cl
        mov    word ptr [esi], dx
      L10:
        inc    eax
        mov    ebx, dword ptr [ebp-0x78]
        add    dword ptr [ebp-0x14], ebx
        mov    ebx, dword ptr [ebp-0x64]
        add    dword ptr [ebp-0x28], ebx
        mov    ebx, dword ptr [ebp-0x60]
        add    dword ptr [ebp-0x20], ebx
        add    esi, 2
        cmp    eax, dword ptr [ebp-0xc]
        jge    L11
        cmp    esi, edi
        jg     L11
        jmp    L9
      L11:
        pop    edi
        pop    esi
        mov    edx, dword ptr [ebp-0x3c]
        mov    eax, dword ptr [ebp+0x8]
        mov    ebx, dword ptr [ebp+0x10]
        mov    ecx, dword ptr [ebp-0x40]
        mov    edi, dword ptr [ebp-0x18]
        mov    esi, dword ptr [ebp-0x2c]
        add    eax, edx
        mov    edx, dword ptr [ebp-0x68]
        mov    dword ptr [ebp+0x8], eax
        mov    eax, dword ptr [ebp-0x74]
        add    ebx, eax
        mov    eax, dword ptr [ebp-0x48]
        add    edi, ecx
        mov    ecx, dword ptr [ebp-0x1c]
        add    esi, edx
        mov    edx, dword ptr [ebp-0x50]
        mov    dword ptr [ebp-0x2c], esi
        mov    esi, dword ptr [ebp-0x4]
        add    ecx, eax
        mov    eax, dword ptr [ebp-0x30]
        mov    dword ptr [ebp+0x10], ebx
        mov    ebx, dword ptr [ebp-0x24]
        mov    dword ptr [ebp-0x18], edi
        mov    edi, dword ptr [ebp-0x8]
        mov    dword ptr [ebp-0x1c], ecx
        mov    ecx, dword ptr [ebp-0x70]
        add    esi, eax
        mov    eax, dword ptr [ebp-0x10]
        add    ebx, ecx
        mov    ecx, dword ptr [ebp-0x6c]
        add    edi, edx
        inc    eax
        cmp    eax, ecx
        mov    dword ptr [ebp-0x24], ebx
        mov    dword ptr [ebp-0x8], edi
        mov    dword ptr [ebp-0x4], esi
        mov    dword ptr [ebp-0x10], eax
        jl     L5
      L12:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x38]
        sub    ecx, dword ptr [ebp-0x6c]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0xc], eax
        mov    eax, dword ptr [ebp-0x7c]
        sub    eax, dword ptr [ebp-0x88]
        mov    ecx, dword ptr [ebp-0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x3c], eax
        mov    eax, dword ptr [ebp-0x4c]
        sub    eax, dword ptr [ebp-0x58]
        mov    ecx, dword ptr [ebp-0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x40], eax
        mov    eax, dword ptr [ebp-0x44]
        sub    eax, dword ptr [ebp-0x54]
        mov    ecx, dword ptr [ebp-0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x48], eax
        mov    eax, dword ptr [ebp-0x84]
        sub    eax, dword ptr [ebp-0x80]
        mov    ecx, dword ptr [ebp-0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x50], eax
        mov    esi, dword ptr [ebp-0x10]
        jmp    L14
      L13:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x38]
        sub    ecx, dword ptr [ebp-0x30]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x38]
        sub    ecx, dword ptr [ebp-0x6c]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0xc], eax
        mov    eax, dword ptr [ebp-0x7c]
        sub    eax, dword ptr [ebp-0x28]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x3c], eax
        mov    eax, dword ptr [ebp-0x7c]
        sub    eax, dword ptr [ebp-0x88]
        mov    ecx, dword ptr [ebp-0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x74], eax
        mov    eax, dword ptr [ebp-0x4c]
        sub    eax, dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x40], eax
        mov    eax, dword ptr [ebp-0x4c]
        sub    eax, dword ptr [ebp-0x58]
        mov    ecx, dword ptr [ebp-0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x68], eax
        mov    eax, dword ptr [ebp-0x44]
        sub    eax, dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x48], eax
        mov    eax, dword ptr [ebp-0x44]
        sub    eax, dword ptr [ebp-0x54]
        mov    ecx, dword ptr [ebp-0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x70], eax
        mov    eax, dword ptr [ebp-0x84]
        sub    eax, dword ptr [ebp-0x20]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x50], eax
        mov    eax, dword ptr [ebp-0x84]
        sub    eax, dword ptr [ebp-0x80]
        mov    ecx, dword ptr [ebp-0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x30], eax
        mov    ecx, dword ptr [ebp-0x28]
        mov    edx, dword ptr [ebp-0x88]
        mov    eax, dword ptr [ebp-0x4]
        mov    dword ptr [ebp+0x8], ecx
        mov    ecx, dword ptr [ebp-0x58]
        mov    dword ptr [ebp+0x10], edx
        mov    edx, dword ptr [ebp-0x8]
        mov    dword ptr [ebp-0x18], eax
        mov    eax, dword ptr [ebp-0x54]
        mov    dword ptr [ebp-0x2c], ecx
        mov    ecx, dword ptr [ebp-0x20]
        mov    dword ptr [ebp-0x1c], edx
        mov    edx, dword ptr [ebp-0x80]
        mov    dword ptr [ebp-0x24], eax
        mov    dword ptr [ebp-0x8], ecx
        mov    dword ptr [ebp-0x4], edx
      L14:
        cmp    esi, dword ptr g_clip_y1
        jg     L24
        mov    edi, dword ptr [ebp-0x38]
        cmp    esi, edi
        jge    L24
        mov    ebx, dword ptr g_clip_y0
        mov    ecx, dword ptr [ebp-0x10]
        mov    edx, dword ptr [ebp-0x38]
        sub    ebx, ecx
        jle    L16
        sub    edx, ecx
        jle    L16
        cmp    ebx, edx
        mov    ecx, edx
        jns    L15
        mov    ecx, ebx
      L15:
        add    dword ptr [ebp-0x10], ecx
        mov    eax, dword ptr [ebp-0x3c]
        mul    ecx
        add    dword ptr [ebp+0x8], eax
        mov    eax, dword ptr [ebp-0x74]
        mul    ecx
        add    dword ptr [ebp+0x10], eax
        mov    eax, dword ptr [ebp-0x40]
        mul    ecx
        add    dword ptr [ebp-0x18], eax
        mov    eax, dword ptr [ebp-0x68]
        mul    ecx
        add    dword ptr [ebp-0x2c], eax
        mov    eax, dword ptr [ebp-0x48]
        mul    ecx
        add    dword ptr [ebp-0x1c], eax
        mov    eax, dword ptr [ebp-0x70]
        mul    ecx
        add    dword ptr [ebp-0x24], eax
        mov    eax, dword ptr [ebp-0x50]
        mul    ecx
        add    dword ptr [ebp-0x8], eax
        mov    eax, dword ptr [ebp-0x30]
        mul    ecx
        add    dword ptr [ebp-0x4], eax
        mov    eax, dword ptr g_pitch
        mul    ecx
        add    dword ptr [ebp-0x34], eax
      L16:
        cmp    dword ptr [ebp-0x10], edi
        jge    L24
      L17:
        mov    eax, dword ptr [ebp-0x10]
        mov    ecx, dword ptr g_clip_y1
        cmp    eax, ecx
        jg     L24
        mov    eax, dword ptr [ebp-0x34]
        mov    ecx, dword ptr g_pitch
        mov    esi, dword ptr [ebp+0x8]
        mov    edi, dword ptr [ebp+0x10]
        mov    dword ptr [ebp-0x8c], eax
        add    eax, ecx
        cmp    esi, edi
        mov    dword ptr [ebp-0x34], eax
        jle    L18
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0x8]
        sub    ecx, dword ptr [ebp+0x10]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x18]
        mov    ecx, dword ptr [ebp-0x2c]
        sub    eax, ecx
        mov    dword ptr [ebp-0x14], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x78], eax
        mov    eax, dword ptr [ebp-0x1c]
        mov    ecx, dword ptr [ebp-0x24]
        sub    eax, ecx
        mov    dword ptr [ebp-0x28], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x64], eax
        mov    eax, dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp-0x4]
        sub    eax, ecx
        mov    dword ptr [ebp-0x20], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x60], eax
        sar    edi, 0x10
        sar    esi, 0x10
        mov    dword ptr [ebp-0x5c], edi
        mov    dword ptr [ebp-0xc], esi
        jmp    L19
      L18:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0x10]
        sub    ecx, dword ptr [ebp+0x8]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x2c]
        mov    ecx, dword ptr [ebp-0x18]
        sub    eax, ecx
        mov    dword ptr [ebp-0x14], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x78], eax
        mov    eax, dword ptr [ebp-0x24]
        mov    ecx, dword ptr [ebp-0x1c]
        sub    eax, ecx
        mov    dword ptr [ebp-0x28], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x64], eax
        mov    eax, dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp-0x8]
        sub    eax, ecx
        mov    dword ptr [ebp-0x20], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x60], eax
        sar    esi, 0x10
        sar    edi, 0x10
        mov    dword ptr [ebp-0x5c], esi
        mov    dword ptr [ebp-0xc], edi
      L19:
        push   esi
        push   edi
        mov    ecx, dword ptr [ebp-0x5c]
        mov    edi, ecx
        mov    ebx, dword ptr g_clip_x0
        sub    ecx, ebx
        jns    L20
        mov    edi, ebx
        neg    ecx
        mov    eax, dword ptr [ebp-0x78]
        mul    ecx
        add    dword ptr [ebp-0x14], eax
        mov    eax, dword ptr [ebp-0x64]
        mul    ecx
        add    dword ptr [ebp-0x28], eax
        mov    eax, dword ptr [ebp-0x60]
        mul    ecx
        add    dword ptr [ebp-0x20], eax
      L20:
        mov    esi, edi
        shl    esi, 1
        add    esi, dword ptr [ebp-0x8c]
        mov    eax, edi
        mov    edi, dword ptr g_clip_x1
        shl    edi, 1
        add    edi, dword ptr [ebp-0x8c]
        cmp    eax, dword ptr [ebp-0xc]
        jge    L23
        cmp    esi, edi
        jg     L23
      L21:
        mov    ebx, dword ptr g_zbuf
        mov    ecx, dword ptr [ebp-0x10]
        lea    ebx, [ebx + eax*4]
        shl    ecx, 9
        mov    edx, dword ptr [ebp-0x20]
        cmp    edx, dword ptr [ebx + ecx]
        jb     L22
        mov    dword ptr [ebx + ecx], edx
        mov    ebx, dword ptr g_texture
        mov    ecx, dword ptr [ebx]
        movzx  edx, word ptr [ebp-0x26]
        shl    edx, cl
        add    edx, dword ptr [ebx + 8]
        mov    ebx, dword ptr [ebx + 0xc]
        movzx  ecx, word ptr [ebp-0x12]
        add    edx, ecx
        movzx  edx, byte ptr [edx]
        mov    ebx, dword ptr [ebx + edx*4]
        mov    ebx, dword ptr [ebx + 4]
        cmp    esi, dword ptr g_mouse_pixel
        movzx  edx, word ptr [ebp-0x8e]
        sete   cl
        movzx  edx, word ptr [ebx + edx*2]
        or     byte ptr g_raster_hit, cl
        mov    word ptr [esi], dx
      L22:
        inc    eax
        mov    ebx, dword ptr [ebp-0x78]
        add    dword ptr [ebp-0x14], ebx
        mov    ebx, dword ptr [ebp-0x64]
        add    dword ptr [ebp-0x28], ebx
        mov    ebx, dword ptr [ebp-0x60]
        add    dword ptr [ebp-0x20], ebx
        add    esi, 2
        cmp    eax, dword ptr [ebp-0xc]
        jge    L23
        cmp    esi, edi
        jg     L23
        jmp    L21
      L23:
        pop    edi
        pop    esi
        mov    ecx, dword ptr [ebp-0x3c]
        mov    ebx, dword ptr [ebp+0x8]
        mov    edx, dword ptr [ebp-0x74]
        mov    edi, dword ptr [ebp+0x10]
        mov    eax, dword ptr [ebp-0x40]
        mov    esi, dword ptr [ebp-0x18]
        add    ebx, ecx
        mov    ecx, dword ptr [ebp-0x68]
        add    edi, edx
        mov    edx, dword ptr [ebp-0x2c]
        add    edx, ecx
        add    esi, eax
        mov    eax, dword ptr [ebp-0x1c]
        mov    dword ptr [ebp-0x2c], edx
        mov    edx, dword ptr [ebp-0x48]
        mov    ecx, dword ptr [ebp-0x50]
        add    eax, edx
        mov    edx, dword ptr [ebp-0x30]
        mov    dword ptr [ebp+0x8], ebx
        mov    ebx, dword ptr [ebp-0x24]
        mov    dword ptr [ebp-0x1c], eax
        mov    eax, dword ptr [ebp-0x70]
        mov    dword ptr [ebp+0x10], edi
        mov    edi, dword ptr [ebp-0x8]
        mov    dword ptr [ebp-0x18], esi
        mov    esi, dword ptr [ebp-0x4]
        add    ebx, eax
        mov    eax, dword ptr [ebp-0x10]
        add    edi, ecx
        mov    ecx, dword ptr [ebp-0x38]
        add    esi, edx
        inc    eax
        cmp    eax, ecx
        mov    dword ptr [ebp-0x24], ebx
        mov    dword ptr [ebp-0x8], edi
        mov    dword ptr [ebp-0x4], esi
        mov    dword ptr [ebp-0x10], eax
        jl     L17
      L24:
        pop    edi
        pop    esi
        pop    ebx
        mov    esp, ebp
        pop    ebp
        ret
    }
}

/* -------------------------------------------------------------------------
 * 0x00486c70 -- DrawGouraudTexTri: textured + Gouraud shading, Z-buffered.
 * ------------------------------------------------------------------------- */

__declspec(naked)
// FUNCTION: LEGOLAND 0x00486c70
void DrawGouraudTexTri(Vertex2D* a, Vertex2D* b, Vertex2D* c)
{
    __asm {
        push   ebp
        mov    ebp, esp
        sub    esp, 0xac
        mov    edx, dword ptr [ebp+0x8]
        mov    eax, dword ptr [ebp+0xc]
        push   ebx
        push   esi
        mov    ecx, dword ptr [edx + 4]
        mov    esi, dword ptr [eax + 4]
        cmp    ecx, esi
        push   edi
        mov    dword ptr [ebp-0x18], 0x47800000
        jle    L0
        mov    eax, dword ptr [ebp+0x8]
        xchg   dword ptr [ebp+0xc], eax
        mov    dword ptr [ebp+0x8], eax
        mov    eax, dword ptr [ebp+0xc]
        mov    edx, dword ptr [ebp+0x8]
      L0:
        mov    ebx, dword ptr [ebp+0x10]
        mov    ecx, dword ptr [eax + 4]
        cmp    ecx, dword ptr [ebx + 4]
        jle    L1
        mov    eax, dword ptr [ebp+0xc]
        xchg   dword ptr [ebp+0x10], eax
        mov    dword ptr [ebp+0xc], eax
        mov    ebx, dword ptr [ebp+0x10]
        mov    eax, dword ptr [ebp+0xc]
      L1:
        mov    ecx, dword ptr [edx + 4]
        mov    esi, dword ptr [eax + 4]
        cmp    ecx, esi
        jle    L2
        mov    eax, dword ptr [ebp+0x8]
        xchg   dword ptr [ebp+0xc], eax
        mov    dword ptr [ebp+0x8], eax
        mov    eax, dword ptr [ebp+0xc]
        mov    edx, dword ptr [ebp+0x8]
      L2:
        mov    ecx, dword ptr [ebx + 4]
        mov    esi, dword ptr [edx + 4]
        sar    ecx, 0x10
        mov    dword ptr [ebp-0x40], ecx
        mov    ecx, dword ptr g_pitch
        sar    esi, 0x10
        imul   ecx, esi
        add    ecx, dword ptr g_surface
        mov    edi, dword ptr [eax + 4]
        sar    edi, 0x10
        mov    dword ptr [ebp-0x3c], ecx
        mov    ecx, dword ptr [edx]
        mov    dword ptr [ebp-0x2c], ecx
        mov    ecx, dword ptr [eax]
        mov    dword ptr [ebp-0xa0], ecx
        mov    ecx, dword ptr [ebx]
        mov    dword ptr [ebp-0x9c], ecx
        mov    ecx, dword ptr [edx + 8]
        mov    dword ptr [ebp-0x24], ecx
        mov    ecx, dword ptr [eax + 8]
        mov    dword ptr [ebp-0xa4], ecx
        mov    ecx, dword ptr [ebx + 8]
        mov    dword ptr [ebp-0x38], esi
        mov    dword ptr [ebp-0x80], edi
        mov    dword ptr [ebp-0xa8], ecx
        mov    ecx, dword ptr [ebp+0x8]
        fld    dword ptr [ecx + 0xc]
        fmul   dword ptr [ebp-0x18]
        fistp  dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp+0xc]
        fld    dword ptr [ecx + 0xc]
        fmul   dword ptr [ebp-0x18]
        fistp  dword ptr [ebp-0x54]
        mov    ecx, dword ptr [ebp+0x10]
        fld    dword ptr [ecx + 0xc]
        fmul   dword ptr [ebp-0x18]
        fistp  dword ptr [ebp-0x5c]
        mov    ecx, dword ptr [ebp+0x8]
        fld    dword ptr [ecx + 0x10]
        fmul   dword ptr [ebp-0x18]
        fistp  dword ptr [ebp-0xc]
        mov    ecx, dword ptr [ebp+0xc]
        fld    dword ptr [ecx + 0x10]
        fmul   dword ptr [ebp-0x18]
        fistp  dword ptr [ebp-0x64]
        mov    ecx, dword ptr [ebp+0x10]
        fld    dword ptr [ecx + 0x10]
        fmul   dword ptr [ebp-0x18]
        fistp  dword ptr [ebp-0x6c]
        mov    edx, dword ptr [edx + 0x14]
        mov    eax, dword ptr [eax + 0x14]
        mov    ecx, dword ptr [ebx + 0x14]
        mov    dword ptr [ebp-0x8], edx
        mov    dword ptr [ebp-0x48], eax
        mov    dword ptr [ebp-0x70], ecx
        mov    ebx, dword ptr g_texture
        mov    ecx, dword ptr [ebx]
        mov    edx, dword ptr [ebp-0x4]
        and    edx, 0xffff
        shl    edx, cl
        mov    dword ptr [ebp-0x4], edx
        mov    ecx, dword ptr [ebx + 4]
        mov    eax, dword ptr [ebp-0xc]
        and    eax, 0xffff
        shl    eax, cl
        mov    dword ptr [ebp-0xc], eax
        shl    dword ptr [ebp-0x8], 6
        mov    ebx, dword ptr g_texture
        mov    ecx, dword ptr [ebx]
        mov    edx, dword ptr [ebp-0x54]
        and    edx, 0xffff
        shl    edx, cl
        mov    dword ptr [ebp-0x54], edx
        mov    ecx, dword ptr [ebx + 4]
        mov    eax, dword ptr [ebp-0x64]
        and    eax, 0xffff
        shl    eax, cl
        mov    dword ptr [ebp-0x64], eax
        shl    dword ptr [ebp-0x48], 6
        mov    ebx, dword ptr g_texture
        mov    ecx, dword ptr [ebx]
        mov    edx, dword ptr [ebp-0x5c]
        and    edx, 0xffff
        shl    edx, cl
        mov    dword ptr [ebp-0x5c], edx
        mov    ecx, dword ptr [ebx + 4]
        mov    eax, dword ptr [ebp-0x6c]
        and    eax, 0xffff
        shl    eax, cl
        mov    dword ptr [ebp-0x6c], eax
        shl    dword ptr [ebp-0x70], 6
        cmp    esi, edi
        mov    dword ptr [ebp-0x14], esi
        je     L13
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x80]
        sub    ecx, dword ptr [ebp-0x38]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0x8], eax
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x40]
        sub    ecx, dword ptr [ebp-0x38]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0xa0]
        sub    eax, dword ptr [ebp-0x2c]
        mov    ecx, dword ptr [ebp+0x8]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x4c], eax
        mov    eax, dword ptr [ebp-0x9c]
        sub    eax, dword ptr [ebp-0x2c]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x98], eax
        mov    eax, dword ptr [ebp-0x54]
        sub    eax, dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp+0x8]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x50], eax
        mov    eax, dword ptr [ebp-0x5c]
        sub    eax, dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x94], eax
        mov    eax, dword ptr [ebp-0x64]
        sub    eax, dword ptr [ebp-0xc]
        mov    ecx, dword ptr [ebp+0x8]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x58], eax
        mov    eax, dword ptr [ebp-0x6c]
        sub    eax, dword ptr [ebp-0xc]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x78], eax
        mov    eax, dword ptr [ebp-0x48]
        sub    eax, dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp+0x8]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x60], eax
        mov    eax, dword ptr [ebp-0x70]
        sub    eax, dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x7c], eax
        mov    eax, dword ptr [ebp-0xa4]
        sub    eax, dword ptr [ebp-0x24]
        mov    ecx, dword ptr [ebp+0x8]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x68], eax
        mov    eax, dword ptr [ebp-0xa8]
        sub    eax, dword ptr [ebp-0x24]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x38], eax
        mov    eax, dword ptr [ebp-0x2c]
        mov    dword ptr [ebp+0x8], eax
        mov    dword ptr [ebp+0x10], eax
        mov    eax, dword ptr [ebp-0x4]
        mov    dword ptr [ebp-0x28], eax
        mov    dword ptr [ebp-0x1c], eax
        mov    eax, dword ptr [ebp-0xc]
        mov    dword ptr [ebp-0x30], eax
        mov    dword ptr [ebp-0x34], eax
        mov    eax, dword ptr [ebp-0x8]
        mov    dword ptr [ebp-0x20], eax
        mov    dword ptr [ebp-0x8], eax
        mov    eax, dword ptr [ebp-0x24]
        mov    dword ptr [ebp-0xc], eax
        mov    dword ptr [ebp-0x4], eax
        mov    ebx, dword ptr g_clip_y0
        mov    ecx, dword ptr [ebp-0x14]
        mov    edx, dword ptr [ebp-0x80]
        sub    ebx, ecx
        jle    L4
        sub    edx, ecx
        jle    L4
        cmp    ebx, edx
        mov    ecx, edx
        jns    L3
        mov    ecx, ebx
      L3:
        add    dword ptr [ebp-0x14], ecx
        mov    eax, dword ptr [ebp-0x4c]
        mul    ecx
        add    dword ptr [ebp+0x8], eax
        mov    eax, dword ptr [ebp-0x98]
        mul    ecx
        add    dword ptr [ebp+0x10], eax
        mov    eax, dword ptr [ebp-0x60]
        mul    ecx
        add    dword ptr [ebp-0x20], eax
        mov    eax, dword ptr [ebp-0x7c]
        mul    ecx
        add    dword ptr [ebp-0x8], eax
        mov    eax, dword ptr [ebp-0x50]
        mul    ecx
        add    dword ptr [ebp-0x28], eax
        mov    eax, dword ptr [ebp-0x94]
        mul    ecx
        add    dword ptr [ebp-0x1c], eax
        mov    eax, dword ptr [ebp-0x58]
        mul    ecx
        add    dword ptr [ebp-0x30], eax
        mov    eax, dword ptr [ebp-0x78]
        mul    ecx
        add    dword ptr [ebp-0x34], eax
        mov    eax, dword ptr [ebp-0x68]
        mul    ecx
        add    dword ptr [ebp-0xc], eax
        mov    eax, dword ptr [ebp-0x38]
        mul    ecx
        add    dword ptr [ebp-0x4], eax
        mov    eax, dword ptr g_pitch
        mul    ecx
        add    dword ptr [ebp-0x3c], eax
      L4:
        cmp    dword ptr [ebp-0x14], edi
        jge    L12
      L5:
        mov    edx, dword ptr [ebp-0x14]
        mov    eax, dword ptr g_clip_y1
        cmp    edx, eax
        jg     L12
        mov    eax, dword ptr [ebp-0x3c]
        mov    ecx, dword ptr g_pitch
        mov    esi, dword ptr [ebp+0x8]
        mov    edi, dword ptr [ebp+0x10]
        mov    dword ptr [ebp-0xac], eax
        add    eax, ecx
        cmp    esi, edi
        mov    dword ptr [ebp-0x3c], eax
        jle    L6
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0x8]
        sub    ecx, dword ptr [ebp+0x10]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x28]
        mov    ecx, dword ptr [ebp-0x1c]
        sub    eax, ecx
        mov    dword ptr [ebp-0x2c], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x74], eax
        mov    eax, dword ptr [ebp-0x30]
        mov    ecx, dword ptr [ebp-0x34]
        sub    eax, ecx
        mov    dword ptr [ebp-0x24], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x8c], eax
        mov    eax, dword ptr [ebp-0x20]
        mov    ecx, dword ptr [ebp-0x8]
        sub    eax, ecx
        mov    dword ptr [ebp-0x18], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x90], eax
        mov    eax, dword ptr [ebp-0xc]
        mov    ecx, dword ptr [ebp-0x4]
        sub    eax, ecx
        mov    dword ptr [ebp-0x44], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x84], eax
        sar    edi, 0x10
        sar    esi, 0x10
        mov    dword ptr [ebp-0x88], edi
        mov    dword ptr [ebp-0x10], esi
        jmp    L7
      L6:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0x10]
        sub    ecx, dword ptr [ebp+0x8]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x1c]
        mov    ecx, dword ptr [ebp-0x28]
        sub    eax, ecx
        mov    dword ptr [ebp-0x2c], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x74], eax
        mov    eax, dword ptr [ebp-0x34]
        mov    ecx, dword ptr [ebp-0x30]
        sub    eax, ecx
        mov    dword ptr [ebp-0x24], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x8c], eax
        mov    eax, dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp-0x20]
        sub    eax, ecx
        mov    dword ptr [ebp-0x18], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x90], eax
        mov    eax, dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp-0xc]
        sub    eax, ecx
        mov    dword ptr [ebp-0x44], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x84], eax
        sar    esi, 0x10
        sar    edi, 0x10
        mov    dword ptr [ebp-0x88], esi
        mov    dword ptr [ebp-0x10], edi
      L7:
        push   esi
        push   edi
        mov    ecx, dword ptr [ebp-0x88]
        mov    edi, ecx
        mov    ebx, dword ptr g_clip_x0
        sub    ecx, ebx
        jns    L8
        mov    edi, ebx
        neg    ecx
        mov    eax, dword ptr [ebp-0x90]
        mul    ecx
        add    dword ptr [ebp-0x18], eax
        mov    eax, dword ptr [ebp-0x74]
        mul    ecx
        add    dword ptr [ebp-0x2c], eax
        mov    eax, dword ptr [ebp-0x8c]
        mul    ecx
        add    dword ptr [ebp-0x24], eax
        mov    eax, dword ptr [ebp-0x84]
        mul    ecx
        add    dword ptr [ebp-0x44], eax
      L8:
        mov    esi, edi
        shl    esi, 1
        add    esi, dword ptr [ebp-0xac]
        mov    eax, edi
        mov    edi, dword ptr g_clip_x1
        shl    edi, 1
        add    edi, dword ptr [ebp-0xac]
        cmp    eax, dword ptr [ebp-0x10]
        jge    L11
        cmp    esi, edi
        jg     L11
      L9:
        mov    ebx, dword ptr g_zbuf
        mov    ecx, dword ptr [ebp-0x14]
        lea    ebx, [ebx + eax*4]
        shl    ecx, 9
        mov    edx, dword ptr [ebp-0x44]
        cmp    edx, dword ptr [ebx + ecx]
        jb     L10
        mov    dword ptr [ebx + ecx], edx
        mov    ebx, dword ptr g_texture
        mov    ecx, dword ptr [ebx]
        movzx  edx, word ptr [ebp-0x22]
        and    edx, dword ptr [ebx + 0x10]
        shl    edx, cl
        add    edx, dword ptr [ebx + 8]
        movzx  ecx, word ptr [ebp-0x2a]
        and    ecx, dword ptr [ebx + 0x14]
        mov    ebx, dword ptr [ebx + 0xc]
        add    edx, ecx
        movzx  edx, byte ptr [edx]
        mov    ebx, dword ptr [ebx + edx*4]
        mov    ebx, dword ptr [ebx + 4]
        cmp    esi, dword ptr g_mouse_pixel
        movzx  edx, word ptr [ebp-0x16]
        sete   cl
        movzx  edx, word ptr [ebx + edx*2]
        or     byte ptr g_raster_hit, cl
        mov    word ptr [esi], dx
      L10:
        inc    eax
        mov    ebx, dword ptr [ebp-0x90]
        add    dword ptr [ebp-0x18], ebx
        mov    ebx, dword ptr [ebp-0x74]
        add    dword ptr [ebp-0x2c], ebx
        mov    ebx, dword ptr [ebp-0x8c]
        add    dword ptr [ebp-0x24], ebx
        mov    ebx, dword ptr [ebp-0x84]
        add    dword ptr [ebp-0x44], ebx
        add    esi, 2
        cmp    eax, dword ptr [ebp-0x10]
        jge    L11
        cmp    esi, edi
        jg     L11
        jmp    L9
      L11:
        pop    edi
        pop    esi
        mov    ecx, dword ptr [ebp-0x98]
        mov    edx, dword ptr [ebp+0x10]
        mov    eax, dword ptr [ebp-0x4c]
        mov    esi, dword ptr [ebp+0x8]
        add    edx, ecx
        mov    ebx, dword ptr [ebp-0x1c]
        mov    ecx, dword ptr [ebp-0x58]
        mov    edi, dword ptr [ebp-0x30]
        add    esi, eax
        mov    eax, dword ptr [ebp-0x28]
        mov    dword ptr [ebp+0x10], edx
        mov    edx, dword ptr [ebp-0x50]
        add    eax, edx
        mov    edx, dword ptr [ebp-0x78]
        mov    dword ptr [ebp-0x28], eax
        mov    eax, dword ptr [ebp-0x94]
        mov    dword ptr [ebp+0x8], esi
        mov    esi, dword ptr [ebp-0x34]
        add    ebx, eax
        mov    eax, dword ptr [ebp-0x60]
        add    edi, ecx
        mov    ecx, dword ptr [ebp-0x20]
        add    esi, edx
        mov    edx, dword ptr [ebp-0x68]
        mov    dword ptr [ebp-0x34], esi
        mov    esi, dword ptr [ebp-0x4]
        add    ecx, eax
        mov    eax, dword ptr [ebp-0x38]
        mov    dword ptr [ebp-0x1c], ebx
        mov    ebx, dword ptr [ebp-0x8]
        mov    dword ptr [ebp-0x30], edi
        mov    edi, dword ptr [ebp-0xc]
        mov    dword ptr [ebp-0x20], ecx
        mov    ecx, dword ptr [ebp-0x7c]
        add    esi, eax
        mov    eax, dword ptr [ebp-0x14]
        add    ebx, ecx
        mov    ecx, dword ptr [ebp-0x80]
        add    edi, edx
        inc    eax
        cmp    eax, ecx
        mov    dword ptr [ebp-0x8], ebx
        mov    dword ptr [ebp-0xc], edi
        mov    dword ptr [ebp-0x4], esi
        mov    dword ptr [ebp-0x14], eax
        jl     L5
      L12:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x40]
        sub    ecx, dword ptr [ebp-0x80]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x10], eax
        mov    eax, dword ptr [ebp-0x9c]
        sub    eax, dword ptr [ebp-0xa0]
        mov    ecx, dword ptr [ebp-0x10]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x4c], eax
        mov    eax, dword ptr [ebp-0x5c]
        sub    eax, dword ptr [ebp-0x54]
        mov    ecx, dword ptr [ebp-0x10]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x50], eax
        mov    eax, dword ptr [ebp-0x6c]
        sub    eax, dword ptr [ebp-0x64]
        mov    ecx, dword ptr [ebp-0x10]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x58], eax
        mov    eax, dword ptr [ebp-0x70]
        sub    eax, dword ptr [ebp-0x48]
        mov    ecx, dword ptr [ebp-0x10]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x60], eax
        mov    eax, dword ptr [ebp-0xa8]
        sub    eax, dword ptr [ebp-0xa4]
        mov    ecx, dword ptr [ebp-0x10]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x68], eax
        mov    esi, dword ptr [ebp-0x14]
        jmp    L14
      L13:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x40]
        sub    ecx, dword ptr [ebp-0x38]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp-0x40]
        sub    ecx, dword ptr [ebp-0x80]
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp-0x10], eax
        mov    eax, dword ptr [ebp-0x9c]
        sub    eax, dword ptr [ebp-0x2c]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x4c], eax
        mov    eax, dword ptr [ebp-0x9c]
        sub    eax, dword ptr [ebp-0xa0]
        mov    ecx, dword ptr [ebp-0x10]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x98], eax
        mov    eax, dword ptr [ebp-0x5c]
        sub    eax, dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x50], eax
        mov    eax, dword ptr [ebp-0x5c]
        sub    eax, dword ptr [ebp-0x54]
        mov    ecx, dword ptr [ebp-0x10]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x94], eax
        mov    eax, dword ptr [ebp-0x6c]
        sub    eax, dword ptr [ebp-0xc]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x58], eax
        mov    eax, dword ptr [ebp-0x6c]
        sub    eax, dword ptr [ebp-0x64]
        mov    ecx, dword ptr [ebp-0x10]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x78], eax
        mov    eax, dword ptr [ebp-0x70]
        sub    eax, dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x60], eax
        mov    eax, dword ptr [ebp-0x70]
        sub    eax, dword ptr [ebp-0x48]
        mov    ecx, dword ptr [ebp-0x10]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x7c], eax
        mov    eax, dword ptr [ebp-0xa8]
        sub    eax, dword ptr [ebp-0x24]
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x68], eax
        mov    eax, dword ptr [ebp-0xa8]
        sub    eax, dword ptr [ebp-0xa4]
        mov    ecx, dword ptr [ebp-0x10]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x38], eax
        mov    ecx, dword ptr [ebp-0x2c]
        mov    edx, dword ptr [ebp-0xa0]
        mov    eax, dword ptr [ebp-0x4]
        mov    dword ptr [ebp+0x8], ecx
        mov    ecx, dword ptr [ebp-0x54]
        mov    dword ptr [ebp+0x10], edx
        mov    edx, dword ptr [ebp-0xc]
        mov    dword ptr [ebp-0x28], eax
        mov    eax, dword ptr [ebp-0x64]
        mov    dword ptr [ebp-0x1c], ecx
        mov    ecx, dword ptr [ebp-0x8]
        mov    dword ptr [ebp-0x30], edx
        mov    edx, dword ptr [ebp-0x48]
        mov    dword ptr [ebp-0x34], eax
        mov    eax, dword ptr [ebp-0x24]
        mov    dword ptr [ebp-0x20], ecx
        mov    ecx, dword ptr [ebp-0xa4]
        mov    dword ptr [ebp-0x8], edx
        mov    dword ptr [ebp-0xc], eax
        mov    dword ptr [ebp-0x4], ecx
      L14:
        cmp    esi, dword ptr g_clip_y1
        jg     L24
        mov    edi, dword ptr [ebp-0x40]
        cmp    esi, edi
        jge    L24
        mov    ebx, dword ptr g_clip_y0
        mov    ecx, dword ptr [ebp-0x14]
        mov    edx, dword ptr [ebp-0x40]
        sub    ebx, ecx
        jle    L16
        sub    edx, ecx
        jle    L16
        cmp    ebx, edx
        mov    ecx, edx
        jns    L15
        mov    ecx, ebx
      L15:
        add    dword ptr [ebp-0x14], ecx
        mov    eax, dword ptr [ebp-0x4c]
        mul    ecx
        add    dword ptr [ebp+0x8], eax
        mov    eax, dword ptr [ebp-0x98]
        mul    ecx
        add    dword ptr [ebp+0x10], eax
        mov    eax, dword ptr [ebp-0x60]
        mul    ecx
        add    dword ptr [ebp-0x20], eax
        mov    eax, dword ptr [ebp-0x7c]
        mul    ecx
        add    dword ptr [ebp-0x8], eax
        mov    eax, dword ptr [ebp-0x50]
        mul    ecx
        add    dword ptr [ebp-0x28], eax
        mov    eax, dword ptr [ebp-0x94]
        mul    ecx
        add    dword ptr [ebp-0x1c], eax
        mov    eax, dword ptr [ebp-0x58]
        mul    ecx
        add    dword ptr [ebp-0x30], eax
        mov    eax, dword ptr [ebp-0x78]
        mul    ecx
        add    dword ptr [ebp-0x34], eax
        mov    eax, dword ptr [ebp-0x68]
        mul    ecx
        add    dword ptr [ebp-0xc], eax
        mov    eax, dword ptr [ebp-0x38]
        mul    ecx
        add    dword ptr [ebp-0x4], eax
        mov    eax, dword ptr g_pitch
        mul    ecx
        add    dword ptr [ebp-0x3c], eax
      L16:
        cmp    dword ptr [ebp-0x14], edi
        jge    L24
      L17:
        mov    edx, dword ptr [ebp-0x14]
        mov    eax, dword ptr g_clip_y1
        cmp    edx, eax
        jg     L24
        mov    eax, dword ptr [ebp-0x3c]
        mov    ecx, dword ptr g_pitch
        mov    esi, dword ptr [ebp+0x8]
        mov    edi, dword ptr [ebp+0x10]
        mov    dword ptr [ebp-0xac], eax
        add    eax, ecx
        cmp    esi, edi
        mov    dword ptr [ebp-0x3c], eax
        jle    L18
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0x8]
        sub    ecx, dword ptr [ebp+0x10]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x28]
        mov    ecx, dword ptr [ebp-0x1c]
        sub    eax, ecx
        mov    dword ptr [ebp-0x2c], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x74], eax
        mov    eax, dword ptr [ebp-0x30]
        mov    ecx, dword ptr [ebp-0x34]
        sub    eax, ecx
        mov    dword ptr [ebp-0x24], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x8c], eax
        mov    eax, dword ptr [ebp-0x20]
        mov    ecx, dword ptr [ebp-0x8]
        sub    eax, ecx
        mov    dword ptr [ebp-0x18], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x90], eax
        mov    eax, dword ptr [ebp-0xc]
        mov    ecx, dword ptr [ebp-0x4]
        sub    eax, ecx
        mov    dword ptr [ebp-0x44], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x84], eax
        sar    edi, 0x10
        sar    esi, 0x10
        mov    dword ptr [ebp-0x88], edi
        mov    dword ptr [ebp-0x10], esi
        jmp    L19
      L18:
        lea    ebx, g_recip
        mov    ecx, dword ptr [ebp+0x10]
        sub    ecx, dword ptr [ebp+0x8]
        shr    ecx, 0x10
        inc    ecx
        mov    eax, dword ptr [ebx + ecx*4]
        mov    dword ptr [ebp+0xc], eax
        mov    eax, dword ptr [ebp-0x1c]
        mov    ecx, dword ptr [ebp-0x28]
        sub    eax, ecx
        mov    dword ptr [ebp-0x2c], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x74], eax
        mov    eax, dword ptr [ebp-0x34]
        mov    ecx, dword ptr [ebp-0x30]
        sub    eax, ecx
        mov    dword ptr [ebp-0x24], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x8c], eax
        mov    eax, dword ptr [ebp-0x8]
        mov    ecx, dword ptr [ebp-0x20]
        sub    eax, ecx
        mov    dword ptr [ebp-0x18], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x90], eax
        mov    eax, dword ptr [ebp-0x4]
        mov    ecx, dword ptr [ebp-0xc]
        sub    eax, ecx
        mov    dword ptr [ebp-0x44], ecx
        mov    ecx, dword ptr [ebp+0xc]
        imul   ecx
        shrd   eax, edx, 0x10
        mov    dword ptr [ebp-0x84], eax
        sar    esi, 0x10
        sar    edi, 0x10
        mov    dword ptr [ebp-0x88], esi
        mov    dword ptr [ebp-0x10], edi
      L19:
        push   esi
        push   edi
        mov    ecx, dword ptr [ebp-0x88]
        mov    edi, ecx
        mov    ebx, dword ptr g_clip_x0
        sub    ecx, ebx
        jns    L20
        mov    edi, ebx
        neg    ecx
        mov    eax, dword ptr [ebp-0x90]
        mul    ecx
        add    dword ptr [ebp-0x18], eax
        mov    eax, dword ptr [ebp-0x74]
        mul    ecx
        add    dword ptr [ebp-0x2c], eax
        mov    eax, dword ptr [ebp-0x8c]
        mul    ecx
        add    dword ptr [ebp-0x24], eax
        mov    eax, dword ptr [ebp-0x84]
        mul    ecx
        add    dword ptr [ebp-0x44], eax
      L20:
        mov    esi, edi
        shl    esi, 1
        add    esi, dword ptr [ebp-0xac]
        mov    eax, edi
        mov    edi, dword ptr g_clip_x1
        shl    edi, 1
        add    edi, dword ptr [ebp-0xac]
        cmp    eax, dword ptr [ebp-0x10]
        jge    L23
        cmp    esi, edi
        jg     L23
      L21:
        mov    ebx, dword ptr g_zbuf
        mov    ecx, dword ptr [ebp-0x14]
        lea    ebx, [ebx + eax*4]
        shl    ecx, 9
        mov    edx, dword ptr [ebp-0x44]
        cmp    edx, dword ptr [ebx + ecx]
        jb     L22
        mov    dword ptr [ebx + ecx], edx
        mov    ebx, dword ptr g_texture
        mov    ecx, dword ptr [ebx]
        movzx  edx, word ptr [ebp-0x22]
        and    edx, dword ptr [ebx + 0x10]
        shl    edx, cl
        add    edx, dword ptr [ebx + 8]
        movzx  ecx, word ptr [ebp-0x2a]
        and    ecx, dword ptr [ebx + 0x14]
        mov    ebx, dword ptr [ebx + 0xc]
        add    edx, ecx
        movzx  edx, byte ptr [edx]
        mov    ebx, dword ptr [ebx + edx*4]
        mov    ebx, dword ptr [ebx + 4]
        cmp    esi, dword ptr g_mouse_pixel
        movzx  edx, word ptr [ebp-0x16]
        sete   cl
        movzx  edx, word ptr [ebx + edx*2]
        or     byte ptr g_raster_hit, cl
        mov    word ptr [esi], dx
      L22:
        inc    eax
        mov    ebx, dword ptr [ebp-0x90]
        add    dword ptr [ebp-0x18], ebx
        mov    ebx, dword ptr [ebp-0x74]
        add    dword ptr [ebp-0x2c], ebx
        mov    ebx, dword ptr [ebp-0x8c]
        add    dword ptr [ebp-0x24], ebx
        mov    ebx, dword ptr [ebp-0x84]
        add    dword ptr [ebp-0x44], ebx
        add    esi, 2
        cmp    eax, dword ptr [ebp-0x10]
        jge    L23
        cmp    esi, edi
        jg     L23
        jmp    L21
      L23:
        pop    edi
        pop    esi
        mov    ecx, dword ptr [ebp-0x98]
        mov    edx, dword ptr [ebp+0x10]
        mov    eax, dword ptr [ebp-0x4c]
        mov    esi, dword ptr [ebp+0x8]
        add    edx, ecx
        mov    ebx, dword ptr [ebp-0x1c]
        mov    ecx, dword ptr [ebp-0x58]
        mov    edi, dword ptr [ebp-0x30]
        add    esi, eax
        mov    eax, dword ptr [ebp-0x28]
        mov    dword ptr [ebp+0x10], edx
        mov    edx, dword ptr [ebp-0x50]
        add    eax, edx
        mov    edx, dword ptr [ebp-0x78]
        mov    dword ptr [ebp-0x28], eax
        mov    eax, dword ptr [ebp-0x94]
        mov    dword ptr [ebp+0x8], esi
        mov    esi, dword ptr [ebp-0x34]
        add    ebx, eax
        mov    eax, dword ptr [ebp-0x60]
        add    edi, ecx
        mov    ecx, dword ptr [ebp-0x20]
        add    esi, edx
        mov    edx, dword ptr [ebp-0x68]
        mov    dword ptr [ebp-0x34], esi
        mov    esi, dword ptr [ebp-0x4]
        add    ecx, eax
        mov    eax, dword ptr [ebp-0x38]
        mov    dword ptr [ebp-0x1c], ebx
        mov    ebx, dword ptr [ebp-0x8]
        mov    dword ptr [ebp-0x30], edi
        mov    edi, dword ptr [ebp-0xc]
        mov    dword ptr [ebp-0x20], ecx
        mov    ecx, dword ptr [ebp-0x7c]
        add    esi, eax
        mov    eax, dword ptr [ebp-0x14]
        add    ebx, ecx
        mov    ecx, dword ptr [ebp-0x40]
        add    edi, edx
        inc    eax
        cmp    eax, ecx
        mov    dword ptr [ebp-0x8], ebx
        mov    dword ptr [ebp-0xc], edi
        mov    dword ptr [ebp-0x4], esi
        mov    dword ptr [ebp-0x14], eax
        jl     L17
      L24:
        pop    edi
        pop    esi
        pop    ebx
        mov    esp, ebp
        pop    ebp
        ret
    }
}

