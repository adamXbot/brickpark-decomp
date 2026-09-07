/* LEGOLAND -- scope LL10: the DEAD (unreferenced) functions of the coaster
 * model / raster translation units, 0x00420fd0..0x004237f0.
 *
 * Nothing live in the executable calls, tail-jumps to or takes the address of
 * any of these; the linker kept them because the game was built without
 * /OPT:REF.  They are ordinary C from the same translation units as their
 * matched neighbours (coaster9.c, schoolcar4.c, schoolcar5.c, schoolcar8.c,
 * coastertiny.c, schoolcar3.c), and read as the module's developer tooling: a
 * wireframe debug painter, a debug box builder, a "dump the two model images
 * back to disk" pair, a shaded texture blitter and a mesh back-face stripper.
 *
 * Reconstructed for VC6 SP3 /O2 /Gy /Gd.  Struct field OFFSETS, record sizes
 * and global addresses are load-bearing; the names are ours.  Types are
 * defined LOCALLY (legoland.h is owned elsewhere).  See docs/lanes/scope-ll10.md.
 */

#include <string.h>
#pragma intrinsic(memset)

typedef struct Pos { int x, y; } Pos;
typedef struct Vec3f { float x, y, z; } Vec3f;

/* {image, byte length} -- schoolcar4.c's g_cc_txt / g_cc_obj pair. */
typedef struct ModelImage { void* data; int length; } ModelImage;

extern ModelImage g_cc_txt;                    /* 0x004dd758 */
extern char       g_cc_name[0x100];            /* 0x004dd760 */
extern ModelImage g_cc_obj;                    /* 0x004dd860 */
extern void*      g_shade_block;               /* 0x00829c54 */

extern int __declspec(dllimport) __cdecl wsprintfA(char* out, const char* fmt, ...); /* 0x004ab298 */
__declspec(dllimport) int __stdcall CreateFileA(const char* name,
                                                unsigned int access,
                                                unsigned int share, void* sa,
                                                unsigned int disp,
                                                unsigned int flags,
                                                void* tmpl);            /* [0x4ab258] */
__declspec(dllimport) int __stdcall WriteFile(int h, const void* buf,
                                              unsigned int n,
                                              unsigned int* written,
                                              void* ov);                /* [0x4ab254] */
__declspec(dllimport) int __stdcall CloseHandle(int h);                 /* [0x4ab260] */

extern void ModelRecord_GetName(ModelImage* image, char* out, int index); /* 0x00422390 */
extern void Free_w(void* p);                                             /* 0x004775d0 */

/* ==========================================================================
 * 0x00423750 -- a one-byte `ret`.  No callers, no arguments visible, nothing
 * to name it after; recorded as Unref_00423750.  Its neighbours are the
 * raster save/restore pair (0x00423730 Raster_RestoreFloatMode, 0x00423790
 * Raster_RestoreState, itself an empty body), so this is a third hook of the
 * same shape that the module never filled in.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x00423750
void Unref_00423750(void) {}

/* ==========================================================================
 * 0x00421530 -- store the argument in 0x004b5958, the dword that immediately
 * follows the box-model template at 0x004b58c8.  The global has exactly ONE
 * reference in the whole image (this store), so it is write-only and there is
 * no behaviour to name the function after: Unref_00421530.
 * ======================================================================== */
extern int g_4b5958;                                                   /* 0x004b5958 */
// FUNCTION: LEGOLAND 0x00421530
void Unref_00421530(int value)
{
    g_4b5958 = value;
}

/* ==========================================================================
 * 0x004225e0 -- the ".txt" twin of schoolcar8.c's CoasterModel_GetRecordName
 * (0x004225b0), which is the same three pushes against g_cc_obj.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x004225e0
void CoasterModel_GetTextureName(int index, char* out)
{
    ModelRecord_GetName(&g_cc_txt, out, index);
}

/* ==========================================================================
 * 0x004227a0 -- release both model images.  Neither pointer is cleared and
 * neither is null-checked, so this is LoadCoasterModelSet's (0x004226c0)
 * unwinder rather than a general teardown.  The two one-argument cdecl calls
 * share ONE `add esp, 8`.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x004227a0
void CoasterModel_FreeImages(void)
{
    Free_w(g_cc_txt.data);
    Free_w(g_cc_obj.data);
}

/* ==========================================================================
 * 0x00423060 -- release the shade-ramp block schoolcar5.c's CoasterShades_Init
 * (0x00422fe0) allocated at 0x00829c54.  The 1024 slot pointers into it are
 * left dangling.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x00423060
void CoasterShades_Free(void)
{
    if (g_shade_block)
        Free_w(g_shade_block);
}

/* ==========================================================================
 * 0x00422520 -- write a block to a file, whole.  CREATE_ALWAYS, no sharing,
 * FILE_FLAG_SEQUENTIAL_SCAN, and success is "WriteFile reported exactly the
 * requested byte count".  The `written` out-parameter is homed in the DEAD
 * `name` argument slot (there is no `sub esp` at all), and the shared
 * `push esi` in front of the two CloseHandle calls is hoisted above the
 * comparison's branch.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x00422520
int WriteWholeFile(const char* name, const void* data, unsigned int length)
{
    int          h;
    unsigned int written;

    if (!name)
        goto fail;
    if (!data)
        goto fail;
    h = CreateFileA(name, 0x40000000, 0, 0, 2, 0x8000000, 0);
    if (h == -1)
        goto fail;
    WriteFile(h, data, length, &written, 0);
    if (written != length) {
        CloseHandle(h);
        goto fail;
    }
    CloseHandle(h);
    return 1;
fail:
    return 0;
}

/* ==========================================================================
 * 0x00422650 -- the exact inverse of LoadCoasterModelSet (schoolcar4.c,
 * 0x004226c0): rebuild "<name>.txt" and "<name>.obj" from the base name kept
 * at 0x004dd760 and write both images back out.  The ".txt" image goes first
 * here, where the loader reads ".obj" first.
 *
 * All four cdecl calls share ONE `add esp, 0x30`, and the 0x100-byte path
 * buffer is reused for both names; wsprintfA's import thunk is hoisted into
 * esi exactly as it is in the loader.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x00422650
void CoasterModel_SaveImages(void)
{
    char path[0x100];

    wsprintfA(path, "%s.txt", g_cc_name);
    WriteWholeFile(path, g_cc_txt.data, g_cc_txt.length);
    wsprintfA(path, "%s.obj", g_cc_name);
    WriteWholeFile(path, g_cc_obj.data, g_cc_obj.length);
}

/* ==========================================================================
 * 0x004237f0 -- paint a straight line between two integer points by walking
 * THIRTY interpolated samples into a local vertex array and handing the whole
 * array to the point plotter at 0x004238a0 (which clips each sample against
 * the view bounds 0x008299ac..0x008299b8 and pokes a 16-bit pixel).
 *
 * The step is `(to - from) * (1/30)`, so the last sample is one step short of
 * `to`; both accumulators live on the x87 stack for the whole loop and each
 * sample is truncated with the game's __ftol helper at 0x00458930.  The
 * plotter's vertex stride is 0x10 and only the first two dwords are read.
 * ======================================================================== */
typedef struct PlotPoint { int x, y; int pad[2]; } PlotPoint;           /* 0x10 */
extern void Raster_PlotPoints(PlotPoint* pts, int count, int colour);   /* 0x004238a0 */

// FUNCTION: LEGOLAND 0x004237f0
void Raster_DrawLine(const Pos* from, const Pos* to, int colour)
{
    PlotPoint pts[30];
    float     x  = (float)from->x;
    float     y  = (float)from->y;
    float     sx = (float)(to->x - from->x) * 0.033333335f;
    float     sy = (float)(to->y - from->y) * 0.033333335f;
    int       i;

    for (i = 0; i < 30; i++) {
        pts[i].x = (int)x;
        pts[i].y = (int)y;
        x += sx;
        y += sy;
    }
    Raster_PlotPoints(pts, 30, colour);
}

/* ==========================================================================
 * 0x004237a0 -- draw every EDGE of a mesh in white (-1): one line per entry
 * of the mesh's +0x14 pair table, between the two vertices it names.  The
 * pair count is re-read from the descriptor after each call, because the call
 * may alias it.
 * ======================================================================== */
/* schoolcar3.c's MeshDesc, whose two halves (Coaster3D_BuildTrackMesh
 * 0x00428cb0 and Coaster3D_DrawMesh 0x004234e0) share this exact layout.  Its
 * +0x08 is the PAIR count, not a vertex count: Coaster3D_BuildTrackMesh sets
 * it to `18 * last + 6`, which is the pair stride, and every reader here and
 * in 0x004237a0 uses it as the bound of the +0x14 array.  There is no vertex
 * count field at all -- 0x004227c0 derives one by scanning the pairs. */
typedef struct TrackVtx { int x, y, z, clip, shade; } TrackVtx;        /* 0x14 */
typedef struct MeshDesc {
    int        f00;             /* +0x00 */
    int        maxvert;         /* +0x04 */
    int        npairs;          /* +0x08 */
    int        ntris;           /* +0x0c */
    TrackVtx*  verts;           /* +0x10 */
    int      (*pairs)[2];       /* +0x14 */
    int      (*tris)[3];        /* +0x18 */
} MeshDesc;                     /* 0x1c */

// FUNCTION: LEGOLAND 0x004237a0
void Raster_DrawWireframe(MeshDesc* m)
{
    int i;

    for (i = 0; i < m->npairs; i++)
        Raster_DrawLine((const Pos*)&m->verts[m->pairs[i][0]],
                        (const Pos*)&m->verts[m->pairs[i][1]], -1);
}

/* ==========================================================================
 * 0x00420fd0 and 0x00421130 -- build the module's DEBUG BOX: an eight-vertex,
 * twelve-triangle cuboid written into caller-supplied vertex and normal
 * arrays, with the mesh header copied wholesale from the 0x90-byte template
 * at 0x004b58c8.
 *
 * The template already carries the box's constants -- eight vertices, the six
 * face normals at 0x004b5700, and the twelve triangles at 0x004b5748 in the
 * FIRST triangle slot (+0x18/+0x1c, the pass-1 list DrawPass1 0x00420810
 * walks).  Both builders point the mesh at the caller's arrays, fill the
 * bottom quad from the three half-extents, mirror it upwards, derive one
 * normal per vertex by NORMALISING the vertex itself (a box centred on the
 * origin, so the position is the outward direction), and then move the
 * triangle list to the SECOND slot (+0x20/+0x24) pointing at the second copy
 * of the same twelve triangles at 0x004b5808 -- after stamping shade = -1 into
 * each record and copying its vertex indices over its texture indices.
 *
 * The two differ in ONE statement: 0x00420fd0 puts the top quad at z = 0, so
 * the box hangs BELOW the origin, and 0x00421130 puts it at +hz, so the box is
 * centred on it.  That one difference is worth four instructions: the literal
 * zero is needed three times in 0x00420fd0 (the top-quad z, tri1_count and the
 * null test), so VC6 hoists it into edi and has to push a FOURTH callee-saved
 * register (ebp) to carry the copy loop's temporary.  0x00421130 stores a
 * float there instead, needs the zero only twice, and pushes three.
 *
 * `model->vertex` is re-read from the mesh before EVERY component store: the
 * store through it may alias the mesh header, so the pointer cannot be cached.
 *
 * Both take an unused fourth argument between the normal array and the three
 * extents; nothing in the image calls either, so what it carried is lost.
 * ======================================================================== */
typedef struct ModelTri {
    short pad00;
    short shade;               /* +0x02 */
    short v[3];                /* +0x04  vertex indices */
    short t[3];                /* +0x0a  texture indices */
} ModelTri;                    /* 0x10 */

typedef struct ModelMesh {
    int       verts;           /* +0x00 */
    int       pad04;
    int       faces;           /* +0x08 */
    Vec3f*    vertex;          /* +0x0c */
    Vec3f*    facenormal;      /* +0x10 */
    Vec3f*    normal;          /* +0x14 */
    ModelTri* tri1;            /* +0x18 */
    int       tri1_count;      /* +0x1c */
    ModelTri* tri2;            /* +0x20 */
    int       tri2_count;      /* +0x24 */
    char      pad28[0x90 - 0x28];
} ModelMesh;                   /* 0x90 */

extern ModelMesh g_box_template;                                /* 0x004b58c8 */
extern ModelTri  g_box_tris[12];                                /* 0x004b5808 */
extern void Vec3Normalise(Vec3f* v);                            /* 0x00425d50 */

// FUNCTION: LEGOLAND 0x00420fd0
void Model_BuildBoxBelow(ModelMesh* model, Vec3f* verts, Vec3f* normals,
                         int unused, float hx, float hy, float hz)
{
    int i;

    (void)unused;
    *model = g_box_template;
    model->vertex = verts;
    model->normal = normals;
    model->vertex[0].x =  hx;
    model->vertex[0].y = -hy;
    model->vertex[0].z = -hz;
    model->vertex[1].x =  hx;
    model->vertex[1].y =  hy;
    model->vertex[1].z = -hz;
    model->vertex[2].x = -hx;
    model->vertex[2].y =  hy;
    model->vertex[2].z = -hz;
    model->vertex[3].x = -hx;
    model->vertex[3].y = -hy;
    model->vertex[3].z = -hz;
    for (i = 0; i <= 3; i++) {
        model->vertex[i + 4] = model->vertex[i];
        model->vertex[i + 4].z = 0.0f;
    }
    if (normals) {
        for (i = 0; i <= 7; i++) {
            Vec3f n = model->vertex[i];
            Vec3Normalise(&n);
            model->normal[i] = n;
        }
        for (i = 0; i < 12; i++) {
            g_box_tris[i].shade = -1;
            g_box_tris[i].t[0] = g_box_tris[i].v[0];
            g_box_tris[i].t[1] = g_box_tris[i].v[1];
            g_box_tris[i].t[2] = g_box_tris[i].v[2];
        }
        model->tri1_count = 0;
        model->tri2_count = 12;
        model->tri2 = g_box_tris;
    }
}

// FUNCTION: LEGOLAND 0x00421130
void Model_BuildBoxCentred(ModelMesh* model, Vec3f* verts, Vec3f* normals,
                           int unused, float hx, float hy, float hz)
{
    int i;

    (void)unused;
    *model = g_box_template;
    model->vertex = verts;
    model->normal = normals;
    model->vertex[0].x =  hx;
    model->vertex[0].y = -hy;
    model->vertex[0].z = -hz;
    model->vertex[1].x =  hx;
    model->vertex[1].y =  hy;
    model->vertex[1].z = -hz;
    model->vertex[2].x = -hx;
    model->vertex[2].y =  hy;
    model->vertex[2].z = -hz;
    model->vertex[3].x = -hx;
    model->vertex[3].y = -hy;
    model->vertex[3].z = -hz;
    for (i = 0; i <= 3; i++) {
        model->vertex[i + 4] = model->vertex[i];
        model->vertex[i + 4].z = hz;
    }
    if (normals) {
        for (i = 0; i <= 7; i++) {
            Vec3f n = model->vertex[i];
            Vec3Normalise(&n);
            model->normal[i] = n;
        }
        for (i = 0; i < 12; i++) {
            g_box_tris[i].shade = -1;
            g_box_tris[i].t[0] = g_box_tris[i].v[0];
            g_box_tris[i].t[1] = g_box_tris[i].v[1];
            g_box_tris[i].t[2] = g_box_tris[i].v[2];
        }
        model->tri1_count = 0;
        model->tri2_count = 12;
        model->tri2 = g_box_tris;
    }
}

/* ==========================================================================
 * 0x00423080 -- blit one of the module's textures to the 16-bit target at
 * (x, y), running every source byte through that colour's SHADE RAMP at the
 * caller's brightness.
 *
 * The texture comes from the module's texture table (0x004d89c8, schoolcar.c's
 * g_coaster_tab_c) through 0x00420780 and is {int w; int h; unsigned char
 * pixels[]}, one palette index per pixel.  Each index selects a 128-byte ramp
 * through g_shade_tab (0x00829c60, schoolcar5.c's CoasterShades_Init builds
 * it) and `shade` picks the 16-bit entry inside the ramp.
 *
 * The destination is 0x004b5b20 as a 16-bit base with 0x004b5b28 as the pitch
 * in PIXELS -- schoolcar6.c calls 0x004b5b20 `int g_zb_4b5b20` and uses
 * 0x004b5b24 as its base, so this file's `unsigned short*` view of it is a
 * deliberate divergence; do NOT align schoolcar6.c's declaration.
 *
 * Raster_SaveState's result is DISCARDED here, unlike Coaster3D_DrawModel
 * (0x00420e90) which skips the whole paint when it fails.
 *
 * FRAME: three of the four arguments are dead after the prologue and every one
 * of their homes is reused -- `index` holds the row counter and `y` holds the
 * one-byte pixel temporary, which is written as a BYTE and read back as an
 * aligned DWORD with `and 0xff` (the slot is four bytes wide).
 * ======================================================================== */
typedef struct Sprite { int width, height; unsigned char pix[1]; } Sprite;
typedef struct VideoSurfaceInfo {
    long pitch; int width, height; void* bits; int unused, format;
} VideoSurfaceInfo;                                              /* 0x18 */

extern unsigned short* g_raster_bits;                            /* 0x004b5b20 */
extern int             g_zb_pitch;                               /* 0x004b5b28 */
extern unsigned short* g_shade_tab[0x400];                       /* 0x00829c60 */

extern Sprite* CoasterModel_GetTexture(int index);               /* 0x00420780 */
extern int  Raster_SaveState(VideoSurfaceInfo*);                 /* 0x00423760 */
extern void Raster_RestoreState(VideoSurfaceInfo*);              /* 0x00423790 */

// FUNCTION: LEGOLAND 0x00423080
void Raster_BlitTextureShaded(int x, int y, int index, int shade)
{
    VideoSurfaceInfo surface;
    Sprite*          tex = CoasterModel_GetTexture(index);

    if (tex) {
        const unsigned char* src = tex->pix;
        unsigned short*      dst;
        int                  row, col;

        Raster_SaveState(&surface);
        dst = g_raster_bits + (g_zb_pitch * y + x);
        for (row = 0; row < tex->height; row++) {
            unsigned short* p = dst;
            for (col = 0; col < tex->width; col++) {
                unsigned char c = *src++;
                *p++ = g_shade_tab[c][shade];
            }
            dst += g_zb_pitch;
        }
        Raster_RestoreState(&surface);
    }
}

/* ==========================================================================
 * 0x004227c0 -- rebuild a mesh with its BACK-FACING triangles, and everything
 * that only they used, removed.  Returns a freshly allocated MeshDesc whose
 * header, pair table, triangle table and vertex array are ONE block, or null.
 *
 * Five passes over three mark arrays -- one flag per triangle, per pair and
 * per vertex, all allocated and zeroed up front and all freed on the way out:
 *
 *  1. Mark every triangle whose 2D cross product `dy2*dx1 - dx2*dy1` is
 *     NEGATIVE.  That is Coaster3D_DrawMesh's (schoolcar3.c 0x004234e0)
 *     facing test with the sense flipped, and it is taken on the SIGN BIT of
 *     the float rather than by comparing against zero, so -0.0 counts as
 *     back-facing too.  The three vertex numbers come out of the pair table
 *     through the same bit-31 seam encoding schoolcar3.c documents.
 *  2. A pair used by a marked triangle is itself marked unless some UNMARKED
 *     triangle also uses it (compared with `(a ^ b) & 0x7fffffff`, so the two
 *     seam directions of one pair count as the same pair).
 *  3. A vertex named by a marked pair is marked unless some unmarked pair
 *     also names it.
 *  4. Count the marked pairs and vertices, and size the new block:
 *     28 + kept_pairs*8 + kept_tris*12 + kept_verts*20.
 *  5. Compact.  Each mark array is REUSED as its own renumbering table: as an
 *     item survives, its slot is overwritten with `old_index - new_index`, so
 *     the later passes renumber with a subtraction.  Vertices are compacted
 *     first because the pairs need their table, pairs next because the
 *     triangles need theirs.
 *
 * ORIGINAL BUG, reproduced: the new descriptor's +0x08 (pair count) and +0x04
 * are written as `kept - 1`, one short of the count every reader -- this
 * function's own first pass, and Raster_DrawWireframe -- treats +0x08 as.  The
 * triangle count at +0x0c has no such `- 1`.  The header's +0x00 is never
 * written at all, so it keeps whatever the allocator left there.
 *
 * The three mark arrays are freed in a deliberately asymmetric shape: a null
 * triangle mark skips its own free, and the other two are null-checked.
 * ======================================================================== */
extern void* HeapAlloc_w(unsigned int size);                    /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                               /* 0x0049e4d0 */

/* RESIDUAL (515 of 521 aligned; audit 521i/1613B vs 521i/1613B -- instruction
 * and byte counts both exact).  Six mismatches, all the same one byte: the six
 * `out->tris[n][k]` stores of the triangle-compaction loop (both arms of each
 * component, original indices 455/459/469/473/483/487) address the row as
 * `[edi + ecx]` -- the reloaded `out->tris` as SIB BASE and the stride-12
 * induction variable `n*12` as INDEX -- where this build emits `[ecx + edi]`,
 * the same two registers with the roles swapped.  Same length, same registers,
 * same schedule; the SIB byte differs.
 *
 * THE CROSS-PRODUCT BLOCK IS CLOSED, and the lever is a BLOCK BOUNDARY, not a
 * spelling of the expression.  The original's `fild dx1 / fild dy1 / fild dx2
 * / fild dy2 / fmul st(3) / fxch st(1) / fmul st(2) / fsubp st(1)` plus the
 * two dead `fstp st(0)` pops is the signature of four x87-ENREGISTERED float
 * locals (exactly Raster_DrawLine's x/y/sx/sy above, whose four dead pops sit
 * after its loop): VC6 forward-substitutes a single-use float local into its
 * consumer ONLY within one basic block, and with the definitions and the
 * product in different blocks it materialises each `fild` at the definition
 * site, in declaration order, multiplies across the stack and kills the two
 * values it cannot pop (they are not on top) at the end of the block -- which
 * is why the pops land just before the `je`.  An empty `if (k) ;` between
 * `dy2 = ...` and `area = ...` is the cheapest source of that boundary
 * (docs/LEVERS.md SA07, the DrawPopUpMock empty test: an empty integer test
 * is a block-split handle that costs no instructions); `if (i) ;`,
 * `if (v[2]) ;`, `if (deadtris) ;` and
 * `if (trimark[i]) ;` are byte-identical to it.  The boundary must sit AFTER
 * all four deltas: after dy1 it is 505/520, after dx2 504/521, before dx1
 * inert (491).  With the block split in place `float` deltas are required
 * (the earlier `double` was a proxy that only bought scheduling), and `c`
 * must be materialised AFTER dy1 -- either `c = &m->verts[v[2]]` assigned
 * between dy1 and dx2 (this body) or `m->verts[v[2]].x - a->x` spelled inline
 * (identical); `c` assigned with a and b before dx1 is 512/520.  The sign
 * test's `mov eax,[area] / test eax,0x80000000` falls out of the same split.
 * The forty-odd consumer-side spellings the previous note listed (types,
 * operand orders, accumulators, named products, an inline four-float helper,
 * `register`, a `Delta()` inline helper, an unreferenced label) are all
 * inert because none of them creates a block boundary; do not re-run them.
 *
 * THE SIX SIB BYTES are a store-address operand-rank difference this build
 * did not reach.  Diagnostics: every LOAD through a 12-stride IV in this
 * function (`m->tris[j][k]` in pass 2, `m->tris[i][k]` here) already emits
 * `[ptr + iv]` and matches; every STORE this build emits puts the IV first,
 * whichever IV it is (a diagnostic `out->tris[i]` on the primary counter,
 * and `m->tris[n]` through the parameter, both keep the IV as base).  Inert,
 * all 515: the counter as `n`, `j`, `k`, a block-scope `int t`, `unsigned t`;
 * `for (i = 0, n = 0; ...)`; a `while` form; `((int*)out->tris)[n*3+k]`;
 * `*(int*)((char*)out->tris + n*12 + 4k)`; a `TriIdx {a,b,c}` struct view;
 * `*(out->tris[n] + k)`; `(*(out->tris + n))[k]`; declaration order of `n`
 * and of `out`.  A user-level byte offset (`off += 12`) keeps the swap and
 * costs 3 elsewhere (512).  A single store after the if/else join (value in
 * `e`, or a ternary) is far worse (427/515) -- VC6 keeps ONE store with a
 * join, so the original's per-arm stores with the value in eax in one arm
 * and edi in the other are the source shape, as written here.
 */
// WIP-FUNCTION: LEGOLAND 0x004227c0  (98.8%, 521/521 instructions and 1613/1613 bytes; six SIB base/index swaps on the triangle-compaction stores, indices 455-487)
MeshDesc* Mesh_DropBackFaces(MeshDesc* m)
{
    MeshDesc* out = 0;
    int*      trimark;
    int*      pairmark;
    int*      vertmark;
    int       npairs = m->npairs;
    int       nverts = 0;
    int       deadtris = 0;
    int       deadpairs = 0;
    int       deadverts = 0;
    int       i, j, k, n;

    for (i = 0; i < m->ntris; i++) {
        for (k = 0; k < 3; k++) {
            const int* pr = m->pairs[m->tris[i][k] & 0x7fffffff];

            if (pr[0] > nverts)
                nverts = pr[0];
            if (pr[1] > nverts)
                nverts = pr[1];
        }
    }
    nverts++;

    trimark  = (int*)HeapAlloc_w(m->ntris * 4);
    pairmark = (int*)HeapAlloc_w(npairs * 4);
    vertmark = (int*)HeapAlloc_w(nverts * 4);
    if (trimark) {
        if (pairmark && vertmark) {
            memset(trimark, 0, m->ntris * 4);
            memset(pairmark, 0, npairs * 4);
            memset(vertmark, 0, nverts * 4);
            for (i = 0; i < m->ntris; i++) {
                int        v[3];
                float      dx1, dy1, dx2, dy2;
                float      area;
                TrackVtx*  a; TrackVtx* b; TrackVtx* c;

                for (k = 0; k < 3; k++) {
                    int e = m->tris[i][k];

                    if (e & 0x80000000)
                        v[k] = m->pairs[e & 0x7fffffff][1];
                    else
                        v[k] = m->pairs[e & 0x7fffffff][0];
                }
                a = &m->verts[v[0]];
                b = &m->verts[v[1]];
                dx1 = (float)(b->x - a->x);
                dy1 = (float)(b->y - a->y);
                c = &m->verts[v[2]];
                dx2 = (float)(c->x - a->x);
                dy2 = (float)(c->y - a->y);
                if (k) ;                      /* block split: see RESIDUAL note */
                area = dy2 * dx1 - dx2 * dy1;
                if (*(int*)&area & 0x80000000) {
                    trimark[i] = 1;
                    deadtris++;
                }
            }
            for (i = 0; i < m->ntris; i++) {
                if (trimark[i]) {
                    for (k = 0; k < 3; k++) {
                        for (j = 0; j < m->ntris; j++) {
                            if (trimark[j] == 0) {
                                if (((m->tris[j][0] ^ m->tris[i][k]) & 0x7fffffff) == 0)
                                    goto next_pair;
                                if (((m->tris[j][1] ^ m->tris[i][k]) & 0x7fffffff) == 0)
                                    goto next_pair;
                                if (((m->tris[j][2] ^ m->tris[i][k]) & 0x7fffffff) == 0)
                                    goto next_pair;
                            }
                        }
                        pairmark[m->tris[i][k] & 0x7fffffff] = 1;
next_pair: ;
                    }
                }
            }
            for (i = 0; i < npairs; i++) {
                if (pairmark[i]) {
                    const int* pv = m->pairs[i];

                    for (k = 0; k < 2; k++) {
                        for (j = 0; j < npairs; j++) {
                            if (pairmark[j] == 0) {
                                if (pv[k] == m->pairs[j][0])
                                    goto next_vert;
                                if (pv[k] == m->pairs[j][1])
                                    goto next_vert;
                            }
                        }
                        vertmark[pv[k]] = 1;
next_vert: ;
                    }
                }
            }
            for (i = 0; i < npairs; i++)
                if (pairmark[i])
                    deadpairs++;
            for (i = 0; i < nverts; i++)
                if (vertmark[i])
                    deadverts++;
            out = (MeshDesc*)HeapAlloc_w(sizeof(MeshDesc) +
                                         (m->ntris - deadtris) * 12 +
                                         (npairs - deadpairs) * 8 +
                                         (nverts - deadverts) * 20);
            if (out) {
                out->npairs = npairs - deadpairs - 1;
                out->ntris = m->ntris - deadtris;
                out->maxvert = nverts - deadverts - 1;
                out->pairs = (int (*)[2])((char*)out + sizeof(MeshDesc));
                out->tris = (int (*)[3])(out->pairs + (npairs - deadpairs));
                out->verts = (TrackVtx*)(out->tris + (m->ntris - deadtris));
                n = 0;
                for (i = 0; i < nverts; i++) {
                    if (vertmark[i] == 0) {
                        out->verts[n] = m->verts[i];
                        vertmark[i] = i - n;
                        n++;
                    }
                }
                for (i = 0, n = 0; i < npairs; i++) {
                    if (pairmark[i] == 0) {
                        out->pairs[n][0] = m->pairs[i][0] - vertmark[m->pairs[i][0]];
                        out->pairs[n][1] = m->pairs[i][1] - vertmark[m->pairs[i][1]];
                        pairmark[i] = i - n;
                        n++;
                    }
                }
                n = 0;
                for (i = 0; i < m->ntris; i++) {
                    if (trimark[i] == 0) {
                        int e;

                        e = m->tris[i][0];
                        if (e & 0x80000000)
                            out->tris[n][0] = (e - pairmark[e & 0x7fffffff]) | 0x80000000;
                        else
                            out->tris[n][0] = e - pairmark[e];
                        e = m->tris[i][1];
                        if (e & 0x80000000)
                            out->tris[n][1] = (e - pairmark[e & 0x7fffffff]) | 0x80000000;
                        else
                            out->tris[n][1] = e - pairmark[e];
                        e = m->tris[i][2];
                        if (e & 0x80000000)
                            out->tris[n][2] = (e - pairmark[e & 0x7fffffff]) | 0x80000000;
                        else
                            out->tris[n][2] = e - pairmark[e];
                        n++;
                    }
                }
            }
        }
        HeapFree_w(trimark);
    }
    if (pairmark)
        HeapFree_w(pairmark);
    if (vertmark)
        HeapFree_w(vertmark);
    return out;
}
