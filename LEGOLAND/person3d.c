/* LEGOLAND - the 3D person: the ".3d" model/animation file and the software
 * renderer that draws one bloke's model into the raster buffer.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; the field and type names are ours.
 * Each matched function carries its original VA marker on the line above it.
 *
 * ===========================================================================
 * HOW A BLOKE'S 3D MODEL IS ASSEMBLED
 * ===========================================================================
 *
 * A bloke is not a skeleton with skinned limbs: it is a MORPH-TARGET model.
 * One ".3d" file is one ANIMATION (ManWalk, WomanSit, GeofPour, ...) and
 * holds a complete vertex cloud and normal cloud PER FRAME, plus a single
 * triangle list shared by every frame.  Playing an animation is picking a
 * frame index; nothing is interpolated and nothing is transformed per limb.
 *
 *   data2.c's Init3DCharacters loads the set:
 *       ".\3ddata\new\<dir>\<file>"   dir = visitor | geoff | tracy
 *   into g_anim_kind1a[6] (man), g_anim_kind1b[6] (woman), g_anim_kind2[2]
 *   (Geoff), g_anim_kind3[1] (Tracy).  Person3D::kind + ::variant choose the
 *   table (blokeanim.c GetBlokeAnim3D), Person3D::anim the entry and
 *   Person3D::frame the frame inside it.
 *
 * ON-DISK LAYOUT of a .3d (every field is a separate RES_ReadFile):
 *
 *     u32    n_frames
 *     repeat n_frames:
 *        u32   n_verts     float verts  [n_verts][3]
 *        u32   n_normals   float normals[n_normals][3]
 *     u32    n_faces
 *     u32    n_gouraud                    <= n_faces
 *     u32    tris[n_faces][3]             vertex indices, one triangle each
 *     Face   faces[n_faces]               0x24 bytes each
 *
 * LoadAnim3D post-processes it in place:
 *   - every vertex's and every normal's Y is NEGATED (the authoring tool is
 *     Y-up, the renderer is Y-down);
 *   - the normals are re-normalised AS FLOATS first (NormaliseVector);
 *   - then every float component of both clouds is converted to 16.16 fixed
 *     point (x * 65536) IN PLACE - after this point the model is integer-only
 *     and the whole renderer is fixed-point;
 *   - ComputeVertexBounds writes each frame's axis-aligned bbox;
 *   - the one FaceSet {n_faces, n_gouraud, tris} is hung off EVERY frame's
 *     +0x20, which is how the topology stays shared across frames.
 *
 * THE NORMAL ARRAY IS TWO-STRIDED.  The first n_gouraud faces own THREE
 * normals each (one per corner, smooth shaded); the remaining faces own ONE
 * (flat shaded).  So face f's normals start at
 *     f <  n_gouraud :  normals + f*9        ints  (3 vectors)
 *     f >= n_gouraud :  normals + n_gouraud*6 + f*3 ints  (1 vector)
 * and Draw3DPersonModel walks the two ranges in two separate loops that call
 * two different rasterisers.
 *
 * ===========================================================================
 * WHICH TEXTURE GOES ON WHICH LIMB
 * ===========================================================================
 *
 * Materials are PER TRIANGLE, in the 0x24-byte Face record:
 *
 *     +0x00 flags     bit 0x2000 set = flat colour, clear = textured
 *     +0x04 r,g,b     only meaningful for the colour case
 *     +0x08 tex       colour handle, or texture id
 *     +0x0c uv[3][2]  float u,v per corner, clamped into [0,1] at load
 *
 * At load time a coloured face turns its RGB bytes into a 64-level shading
 * ramp handle (0x00486280), and a textured face has the CALLER'S texture base
 * added to its id - LoadAnim3D's third argument, the model context that
 * data2.c gets from GetModelContext (0x00443710).  That is the shared,
 * per-animation table.
 *
 * The PER-BLOKE table is Person3D +0x50: Draw3DPersonModel walks THAT array
 * 0x24 bytes at a time, not anim->faces.  It is a per-person copy of the face
 * table, which is how two blokes sharing one mesh get different faces and
 * shirts: blokeai.c's GetFaceTextureNameOfBloke / GetChestTextureNameOfBloke
 * pick names out of the packed list (g_texnames_boy / g_texnames_girl) with
 * LookupTextureName(list, 0=face | 1=chest, Person3D::tex_index), and
 * Person3D::leg_colour / ::arm_colour carry the two palette indices for the
 * limbs that are colour- rather than texture-shaded.  So: face and chest are
 * TEXTURES chosen per bloke, arms and legs are flat COLOURS chosen per bloke,
 * and everything else comes from the shared model.
 *
 * ===========================================================================
 * DRAW3DPERSONMODEL - the per-frame pipeline
 * ===========================================================================
 *
 * Called from rin.c's Render3DPerson, which has already clipped a 160x120
 * (0xa0 x 0x78) window at the person's screen position, handed it to
 * Render_SetViewport, pointed the rasteriser at the surface and switched the
 * x87 to control word 0x7f.  Everything below is in the model window's own
 * coordinates, 16.16 fixed.
 *
 *  1. anim   = GetBlokeAnim3DFromPerson(p);  fr = &anim->frames[p->frame]
 *     sx = scale_x * 0.447, sz = scale_z * 0.447, sy = scale_y  (0.447 ~ 1/V5
 *     is the isometric foreshortening; Y is NOT foreshortened).  All three
 *     become 16.16.
 *     ydepth (+0x38), if non-zero, becomes 16.16 << 8 - a 24.8 weight.
 *  2. RenderZBufferObject(f2c, f24, f28) primes the rasteriser's z buffer.
 *  3. mt = TRANSPOSE(p->matrix), the 16.16 rotation SetPersonRotation built.
 *     The light direction {-0x1800, -0x5000, 0x3000} (i.e. -0.094, -0.313,
 *     0.188) is pushed through mt - the transpose is the inverse of a
 *     rotation, so this takes the light INTO model space once per person
 *     instead of rotating every normal.
 *  4. The frame's bbox corners are rotated by p->matrix, scaled by
 *     (sx, sy, sz), and their x/y extent gives the window centring
 *         ox = (80 << 16) - width/2      oy = (90 << 16) - height/2
 *     so the model is centred horizontally and hung at 3/4 height.
 *  5. Every vertex is copied into the scratch array g_xverts (0x00643ee8) as
 *         x + cx,   y - bmin.y,   z + cz
 *     with cx = -(bmin.x + bmax.x)/2 and cz = -(bmin.z + bmax.z)/2: the model
 *     is centred in X and Z and stood on the ground in Y (feet at 0).
 *  6. g_xverts is rotated in place by p->matrix (TransformVectorsL's body,
 *     inlined).
 *  7. The z extent gives zscale = 0x40000000 / ((zmax - zmin) >> 5), and every
 *     vertex gets a SORT KEY in g_vert_key (0x00641004):
 *         key = (tint << 24) + ((z - zmin) * zscale [* zboost]) + y * ydepth
 *     tint (+0x34) is the high byte, so a whole person can be pushed in front
 *     of or behind another by one byte while depth still orders its own
 *     triangles.  Then each vertex is scaled by (sx, sy, sz).
 *  8. Per triangle: the three transformed vertices become screen points
 *         X = ox + 2*(x + z)      Y = oy + (y - x + z)
 *     - the classic 2:1 isometric projection, X doubled because the tile grid
 *     is 2:1.  The 2D cross product of the two edges culls back faces
 *     (drawn only when it is NEGATIVE).  Vertex order is swapped when
 *     TMNegParity says the rotation matrix is left-handed, so the winding
 *     test stays valid after a mirroring rotation.
 *  9. Shading, per vertex (pass 1) or per face (pass 2):
 *         d = dot(normal, light) in 16.16;  shade = (d < 0 ? 1 : d) + 0x3333
 *     0x3333 is the ambient floor.  Then the face is handed to one of four
 *     rasterisers by (pass, textured):
 *         gouraud + colour  0x00486590     gouraud + texture 0x00486c70
 *         flat    + colour  0x004877b0     flat    + texture 0x00487d40
 *     with SetFlatColour (0x004864e0) or SetTexture (0x004886e0) first.
 *
 * The rasteriser writes straight into the locked surface; Render3DPerson then
 * reads g_raster_hit to decide whether the mouse is over this bloke, and the
 * PRINT LIST never sees the individual triangles - a person enters the
 * depth-sorted print list once, as a whole, with Person3D::depth (+0x54) as
 * its key (blokeai.c SortBlokeIn3D, type 0x306/0x307/0x308 for kinds 1/2/3).
 * All the per-triangle ordering above happens inside the 160x120 window.
 *
 * ORIGINAL BUGS, reproduced faithfully (see the comments in the code):
 *  - the bbox corner list repeats (bmin.x, bmax.y, bmax.z) and never emits
 *    (bmax.x, bmax.y, bmin.z), so the centring box is short one corner;
 *  - the vertex depth keys, UVs and shades are written to v[0]/v[1]/v[2] in
 *    TRIANGLE INDEX order even when the parity swap has already put vertex C
 *    in v[0] - so on a mirrored person the depths and UVs no longer follow
 *    their vertices;
 *  - pass 2 calls TMNegParity once per FACE instead of once per model;
 *  - pass 2 leaves v[1].shade and v[2].shade at whatever the previous face
 *    left there (only v[0].shade is written), which is what makes the flat
 *    rasterisers flat.
 *
 * ===========================================================================
 * CODEGEN NOTES (VC6 SP3, /O2 /Gy /Gd)
 * ===========================================================================
 *
 * - The fixed-point core CANNOT be written in C.  VC6 lowers
 *   `(__int64)a * b >> 16` to `imul` plus a CALL to __allshr even when the
 *   shift is a literal 16, so the original's `imul` / `shrd eax,edx,16` pair
 *   is only reachable through inline __asm - the same reason math3d.c's
 *   SetPersonRotation and TransformVectorsL are asm.  Both functions here that
 *   touch 16.16 arithmetic therefore carry __asm macros, and that is also why
 *   they keep ebp frames and save all of ebx/esi/edi.
 * - `fistp` with no __ftol call: the game leaves the x87 in round-to-nearest
 *   with all exceptions masked for the whole draw, so float->fixed is a bare
 *   fld/fmul/fistp inside __asm.
 * - `shade = (d < 0 ? 1 : d)` is spelled `(unsigned)d >> (unsigned)(d >> 31)`:
 *   sar 31 gives 0 or 31, and shifting right by 31 turns any negative into 1.
 * - LoadAnim3D: the two dead parameter homes carry live locals - [ebp+8] is
 *   the frame count and then the face count, [ebp+0xc] is the normal count and
 *   then the 3-byte RGB buffer.  Reproducing that sharing needed the count and
 *   the colour buffer to be ONE union (a separate `unsigned char[3]` local gets
 *   its own home and pushes the frame out by a slot).
 * - ComputeVertexBounds writes its two results as whole-struct assignments
 *   (`out->bmin = mn; out->bmax = mx;`).  Element-by-element stores make VC6
 *   fold both triples onto one base register; the struct copies are what
 *   produce the original's `mov ecx,eax / add eax,0xc` pointer split.
 * --------------------------------------------------------------------------- */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* Shared topology, one per .3d file, pointed at by EVERY frame (+0x20). */
typedef struct Vec3i { int x; int y; int z; } Vec3i;

typedef struct FaceSet {
    int  n_faces;      /* +0x00  total triangles */
    int  n_gouraud;    /* +0x04  how many of them are smooth-shaded */
    int* tris;         /* +0x08  3 vertex indices per triangle */
} FaceSet;

/* One animation frame: a whole vertex/normal set (morph-target animation). */
typedef struct Frame3D {
    Vec3i    bmin;       /* +0x00  bounding box, 16.16 */
    Vec3i    bmax;       /* +0x0c */
    int      n_verts;    /* +0x18 */
    int*     verts;      /* +0x1c  3 ints (16.16) per vertex */
    FaceSet* faces;      /* +0x20  the shared topology */
    int      n_normals;  /* +0x24 */
    int*     normals;    /* +0x28  3 ints (16.16) per normal */
    int      pad2c[3];   /* +0x2c..+0x37  allocated, never written */
} Frame3D;

/* One triangle's material, shared across frames. */
typedef struct Face3D {
    int           flags;   /* +0x00  bit 0x2000: flat colour, else textured */
    unsigned char rgb[3];  /* +0x04..+0x06 */
    unsigned char pad07;   /* +0x07 */
    int           tex;     /* +0x08  colour handle, or texture id + base */
    float         uv[6];   /* +0x0c..+0x23  u,v per corner */
} Face3D;

typedef struct Anim3D {
    int      n_frames;   /* +0x00 */
    Frame3D* frames;     /* +0x04 */
    Face3D*  faces;      /* +0x08  n_faces entries */
    int      pad0c[6];   /* +0x0c..+0x23  allocated and zeroed, never used */
} Anim3D;

/* ------------------------------------------------------------ prototypes -- */

extern void* MemAlloc(unsigned int size);                /* 0x0049e4ff (malloc) */
extern int   sprintf(char* buf, const char* fmt, ...);   /* 0x0049e573 */
extern void* RES_OpenFile(const char* path);             /* 0x00489b60 */
extern int   RES_ReadFile(void* f, void* buf, int n);    /* 0x00489cf0 */
extern void  RES_CloseFile(void* f);                     /* 0x00489de0 */
extern void  NormaliseVector(float* v);                  /* 0x00443450 */
extern int   MakeShadedColour(int levels, unsigned char* rgb); /* 0x00486280 */

void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)

extern const char kAnim3DPath[];   /* 0x004b7b10 ".\\3ddata\\new\\%s\\%s" */

/* -------------------------------------------------------------- functions -- */

/* Clamp `count` consecutive (u, v) float pairs into [0, 1].  Called on a
 * Face3D's three corner UVs straight after the face table is read. */
// FUNCTION: LEGOLAND 0x0043fa10
void ClampUVList(float* uv, int count)
{
    int n;

    if (count <= 0)
        return;
    n = count;
    do {
        if (uv[0] < 0.0f)
            uv[0] = 0.0f;
        if (uv[0] > 1.0f)
            uv[0] = 1.0f;
        if (uv[1] < 0.0f)
            uv[1] = 0.0f;
        if (uv[1] > 1.0f)
            uv[1] = 1.0f;
        uv += 2;
    } while (--n);
}

/* Axis-aligned bounds of a frame's vertex cloud, written back into the frame
 * itself (LoadAnim3D passes the same pointer as both arguments). */
// FUNCTION: LEGOLAND 0x00440980
void ComputeVertexBounds(Frame3D* f, Frame3D* out)
{
    Vec3i mn;
    Vec3i mx;
    int*  v = f->verts;
    int   n = f->n_verts;

    mn.x = v[0];
    mn.y = v[1];
    mn.z = v[2];
    mx.x = v[0];
    mx.y = v[1];
    mx.z = v[2];
    v += 3;
    if (n > 1) {
        n--;
        do {
            int x = v[0];
            int y = v[1];
            int z = v[2];
            if (x < mn.x)
                mn.x = x;
            if (x > mx.x)
                mx.x = x;
            if (y < mn.y)
                mn.y = y;
            if (y > mx.y)
                mx.y = y;
            if (z < mn.z)
                mn.z = z;
            if (z > mx.z)
                mx.z = z;
            v += 3;
        } while (--n);
    }
    out->bmin = mn;
    out->bmax = mx;
}

/* Load one ".3d" animation out of the RES volume.
 *
 * File layout (all little-endian; every read is a separate RES_ReadFile):
 *
 *     u32   n_frames
 *     per frame:
 *        u32   n_verts;    float verts  [n_verts][3]
 *        u32   n_normals;  float normals[n_normals][3]
 *     u32   n_faces
 *     u32   n_gouraud                (how many faces carry 3 normals)
 *     u32   tris[n_faces][3]         vertex indices
 *     Face3D faces[n_faces]          0x24 bytes each, as loaded
 *
 * Post-processing, in order: the Y of every vertex and every normal is
 * NEGATED (the authoring tool is Y-up, the renderer is Y-down), the normals
 * are re-normalised as floats first, then every float component of both
 * arrays is converted in place to 16.16 fixed point (x * 65536).  Finally
 * ComputeVertexBounds fills the frame's bbox and the shared FaceSet is
 * hung off every frame's +0x20.
 *
 * The face table is fixed up last: a face with flag bit 0x2000 turns its
 * three RGB bytes into a shading ramp handle (64 levels), and one without
 * has the caller's texture base added to its texture id.  The three corner
 * UVs are clamped into [0,1].
 */
/* Residual at 83.6%: pure VC6 spill-home colouring.  k65536/set/i rotate between
 * the homes -0x10/-0x14/-0x18; the frame stores come out [edi+ecx+N] where the
 * original has [ecx+edi+N]; and the three RGB byte loads pick al/dl/cl instead of
 * cl/dl/al, which clobbers the CSE'd &face and forces three reloads.  Frame size,
 * every spill home count, the instruction schedule and all control flow match. */
// WIP-FUNCTION: LEGOLAND 0x0043fa80  (83.6%, 266/318 insns)
Anim3D* LoadAnim3D(const char* file, const char* dir, int texbase)
{
    char          path[0x100];
    float         k65536 = 65536.0f;
    Anim3D*       anim = 0;
    void*         fp;
    FaceSet*      set;
    int           i;
    int           n;
    int           nv;
    union { int n; unsigned char rgb[4]; } nn;
    int           j;
    int           k;
    int*          vp;
    int*          np;

    sprintf(path, kAnim3DPath, dir, file);
    fp = RES_OpenFile(path);
    if (fp) {
        anim = (Anim3D*)MemAlloc(sizeof(Anim3D));
        memset(anim, 0, sizeof(Anim3D));
        set = (FaceSet*)MemAlloc(sizeof(FaceSet));
        set->n_faces = 0;
        set->n_gouraud = 0;
        set->tris = 0;
        RES_ReadFile(fp, &n, 4);
        anim->n_frames = n;
        anim->frames = (Frame3D*)MemAlloc(n * sizeof(Frame3D));
        memset(anim->frames, 0, n * sizeof(Frame3D));
        for (i = 0; i < n; i++) {
            RES_ReadFile(fp, &nv, 4);
            anim->frames[i].n_verts = nv;
            anim->frames[i].verts = (int*)MemAlloc(nv * 12);
            vp = anim->frames[i].verts;
            RES_ReadFile(fp, anim->frames[i].verts, nv * 12);
            for (k = 0; k < nv; k++)
                ((float*)anim->frames[i].verts)[k * 3 + 1] =
                    -((float*)anim->frames[i].verts)[k * 3 + 1];
            for (j = 0; j < nv * 12; j += 4) {
                __asm {
                    mov   eax, vp
                    add   eax, j
                    fld   dword ptr [eax]
                    fmul  k65536
                    fistp dword ptr [eax]
                }
            }
            RES_ReadFile(fp, &nn.n, 4);
            anim->frames[i].n_normals = nn.n;
            anim->frames[i].normals = (int*)MemAlloc(nn.n * 12);
            np = anim->frames[i].normals;
            RES_ReadFile(fp, anim->frames[i].normals, nn.n * 12);
            for (k = 0, j = 0; k < nn.n; k++, j += 12)
                NormaliseVector((float*)((char*)anim->frames[i].normals + j));
            for (k = 0; k < nn.n; k++)
                ((float*)anim->frames[i].normals)[k * 3 + 1] =
                    -((float*)anim->frames[i].normals)[k * 3 + 1];
            for (k = 0; k < nn.n * 12; k += 4) {
                __asm {
                    mov   eax, np
                    add   eax, k
                    fld   dword ptr [eax]
                    fmul  k65536
                    fistp dword ptr [eax]
                }
            }
            anim->frames[i].faces = set;
            ComputeVertexBounds(&anim->frames[i], &anim->frames[i]);
        }
        RES_ReadFile(fp, &n, 4);
        RES_ReadFile(fp, &set->n_gouraud, 4);
        set->n_faces = n;
        set->tris = (int*)MemAlloc(n * 12);
        RES_ReadFile(fp, set->tris, n * 12);
        anim->faces = (Face3D*)MemAlloc(n * sizeof(Face3D));
        RES_ReadFile(fp, anim->faces, n * sizeof(Face3D));
        RES_CloseFile(fp);
        if (anim) {
            for (i = 0; i < n; i++) {
                if (anim->faces[i].flags & 0x2000) {
                    nn.rgb[2] = anim->faces[i].rgb[2];
                    nn.rgb[1] = anim->faces[i].rgb[1];
                    nn.rgb[0] = anim->faces[i].rgb[0];
                    anim->faces[i].tex = MakeShadedColour(0x40, nn.rgb);
                } else {
                    anim->faces[i].tex += texbase;
                }
                ClampUVList(anim->faces[i].uv, 3);
            }
        }
    }
    return anim;
}

/* ======================================================================== *
 *  The 3D person renderer                                                  *
 * ======================================================================== */

/* One vertex handed to the software rasteriser (0x1c bytes).  Only the six
 * named words are written here; +0x18 is never touched. */
typedef struct Vertex2D {
    int x;      /* +0x00  screen x, 16.16 */
    int y;      /* +0x04  screen y, 16.16 */
    int z;      /* +0x08  depth / sort key from g_vert_key[] */
    int u;      /* +0x0c  texture u (raw float bits from the face's UVs) */
    int vv;     /* +0x10  texture v */
    int shade;  /* +0x14  lighting term + 0x3333 */
    int pad18;  /* +0x18 */
} Vertex2D;

/* The 3D person, blokeai.c's 0x94-byte record; only what this file uses.
 * +0x50 is the PER-PERSON face/material table: blokeanim.c calls it `inst`,
 * but Draw3DPersonModel walks it 0x24 bytes at a time exactly like the
 * Face3D array LoadAnim3D builds, which is how one bloke gets its own face
 * and chest textures over the shared mesh. */
typedef struct Person3D {
    unsigned char pad00[0x10];
    float         scale_x;   /* +0x10 */
    float         scale_y;   /* +0x14 */
    float         scale_z;   /* +0x18 */
    unsigned char pad1c[8];  /* +0x1c..+0x23 */
    int           f24;       /* +0x24 \                                  */
    int           f28;       /* +0x28  > RenderZBufferObject(f2c,f24,f28) */
    int           f2c;       /* +0x2c / also: non-zero = apply f3c        */
    int           f30;       /* +0x30 */
    int           tint;      /* +0x34  high byte of every vertex key      */
    float         ydepth;    /* +0x38  y -> depth weight (0 = ignore y)   */
    float         zboost;    /* +0x3c  extra depth gain when f2c is set   */
    unsigned char pad40[0xc];/* +0x40..+0x4b  rotation vector (math3d.c)  */
    int           frame;     /* +0x4c  frame index into the animation     */
    Face3D*       faces;     /* +0x50  per-person face/material table     */
    int           depth;     /* +0x54  print-list sort key                */
    int           matrix[9]; /* +0x58  16.16 rotation (SetPersonRotation) */
} Person3D;

/* The two scratch arrays the renderer works in; both are plain .bss. */
extern int g_vert_key[];       /* 0x00641004  one key per vertex */
extern int g_xverts[];         /* 0x00643ee8  3 ints per transformed vertex */

extern Anim3D* GetBlokeAnim3DFromPerson(Person3D* p);            /* 0x00440800 */
extern void    RenderZBufferObject(int a, int b, int c);         /* 0x00485fe0 */
extern void    TransformVectorsL(int* src, int* dst, int* m, int count); /* 0x004433b0 */
extern int     TMNegParity(int* m);                              /* 0x00442e00 */
extern void    SetFlatColour(int colour);                        /* 0x004864e0 */
extern void    SetTexture(int tex);                              /* 0x004886e0 */
extern void    DrawGouraudTri(Vertex2D* a, Vertex2D* b, Vertex2D* c);    /* 0x00486590 */
extern void    DrawGouraudTexTri(Vertex2D* a, Vertex2D* b, Vertex2D* c); /* 0x00486c70 */
extern void    DrawFlatTri(Vertex2D* a, Vertex2D* b, Vertex2D* c);       /* 0x004877b0 */
extern void    DrawFlatTexTri(Vertex2D* a, Vertex2D* b, Vertex2D* c);    /* 0x00487d40 */

/* The whole fixed-point core of this function is INLINE ASM in the original,
 * and it has to be here too: VC6 lowers `(__int64)a * b >> 16` to an `imul`
 * plus a CALL to __allshr even when the shift count is a constant, so the
 * `imul` / `shrd eax,edx,16` pair the original uses cannot be reached from C.
 * SetPersonRotation and TransformVectorsL in math3d.c are asm for the same
 * reason.  r = (a * b) >> 16, all three 16.16 lvalues. */
#define FMUL(r, a, b) \
    __asm { mov  eax, a } __asm { mov  ecx, b } __asm { imul ecx } \
    __asm { shrd eax, edx, 16 } __asm { mov  r, eax }

/* d[n] = (s[n] * m) >> 16 for two ARRAY locals, byte offset n. */
#define FMULA(d, s, n, m) \
    __asm { lea  eax, s } __asm { mov  eax, [eax+n] } __asm { mov  ecx, m } \
    __asm { imul ecx } __asm { shrd eax, edx, 16 } \
    __asm { lea  edx, d } __asm { mov  [edx+n], eax }

/* p[n] = (p[n] * m) >> 16 through a POINTER local, byte offset n. */
#define FMULP(p, n, m) \
    __asm { mov  eax, p } __asm { mov  eax, [eax+n] } __asm { mov  ecx, m } \
    __asm { imul ecx } __asm { shrd eax, edx, 16 } \
    __asm { mov  edx, p } __asm { mov  [edx+n], eax }

/* float -> 16.16 in place.  Render3DPerson leaves the x87 in round-to-nearest
 * with everything masked (CW 0x7f) for the whole draw, so this is a bare
 * fistp and never the CRT's __ftol. */
#define TOFIX(x) \
    __asm { fld   x } __asm { fmul  k65536 } __asm { fistp dword ptr x }

#define FIXV(x)   (*(int*)&(x))

/* The per-vertex diffuse term: dot(normal, light) in 16.16 with the normal
 * pointer already in ebx, negatives folded to 1 by `(unsigned)d >> (d >> 31)`,
 * biased by 0x3333 and dropped into vertex record vo (+0x14 = shade). */
#define SHADE(nb, vo) \
    __asm { mov  eax, [ebx+nb] } __asm { imul dword ptr light[0] } \
    __asm { shrd eax, edx, 16 } __asm { mov  ecx, eax } \
    __asm { mov  eax, [ebx+nb+4] } __asm { imul dword ptr light[4] } \
    __asm { shrd eax, edx, 16 } __asm { add  ecx, eax } \
    __asm { mov  eax, [ebx+nb+8] } __asm { imul dword ptr light[8] } \
    __asm { shrd eax, edx, 16 } __asm { add  ecx, eax } \
    __asm { mov  eax, ecx } __asm { sar  ecx, 31 } __asm { shr  eax, cl } \
    __asm { mov  ecx, eax } __asm { add  ecx, 0x3333 } \
    __asm { lea  edx, v[vo] } __asm { mov  [edx+20], ecx }

/* Residual at 33.3%: the semantics are complete and every block is present in the
 * original's order and shape; the frame is the right size (0x1b4) and the array
 * homes for v[] (-0x1a4), sc[] (-0x150) and box[] (-0xd8) are exact.  What is left
 * is spill-home COLOURING - the original spreads 28 scalar slots through the gaps
 * between those arrays and this reconstruction colours them differently, so most
 * [ebp-N] operands and a handful of register tie-breaks are off by a slot.  Nothing
 * structural is known to be wrong; continue by pinning the scalar homes (mt[] wants
 * -0x54 and light[] -0xc, which alone would land the four SHADE blocks). */
// WIP-FUNCTION: LEGOLAND 0x00440a30  (33.3%, 337/1013 insns of 1023)
void Draw3DPersonModel(Person3D* p)
{
    Vertex2D v[3];
    int      sc[24];
    int      box[24];
    int      mt[9];
    int      light[3];
    Frame3D* fr;
    int      nfaces;
    int*     tris;
    int      ngour;
    int*     vptr;
    int*     nrm;
    int      zscale;
    int*     mpp;
    int*     mp;
    int      parity;
    Face3D*  face;
    int      cnt;
    float    ydep;
    int*     q;
    float    fx;
    int      lo2, hi2;
    int      t;
    int      dx1, dy1, dx2, dy2;
    int      nverts;
    float    k65536 = 65536.0f;
    int      lo, hi;
    float    fy;
    float    fz;
    Anim3D*  anim;
    FaceSet* set;
    float    zb;
    int      i;
    int      cx, cz, ox, oy;
    int      cr1, cr2;
    int*     tp;
    int      yy;

    anim = GetBlokeAnim3DFromPerson(p);
    fx = p->scale_x * 0.447f;
    fz = p->scale_z * 0.447f;
    face = p->faces;
    fr = &anim->frames[p->frame];
    ydep = p->ydepth;
    set = fr->faces;
    nfaces = set->n_faces;
    tris = set->tris;
    ngour = set->n_gouraud;
    nverts = fr->n_verts;
    nrm = fr->normals;
    fy = p->scale_y;
    if (ydep != 0.0f) {
        TOFIX(ydep);
        FIXV(ydep) <<= 8;
    }
    RenderZBufferObject(p->f2c, p->f24, p->f28);

    mp = p->matrix;
    mpp = p->matrix;
    mt[0] = mp[0];
    mt[3] = mp[1];
    mt[6] = mp[2];
    mt[1] = mp[3];
    mt[4] = mp[4];
    mt[7] = mp[5];
    mt[2] = mp[6];
    mt[5] = mp[7];
    mt[8] = mp[8];
    light[0] = -0x1800;
    light[1] = -0x5000;
    light[2] = 0x3000;
    TOFIX(fx);
    TOFIX(fy);
    TOFIX(fz);

    cx = -(fr->bmax.x + fr->bmin.x) >> 1;
    cz = -(fr->bmin.z + fr->bmax.z) >> 1;

    box[0]  = fr->bmin.x; box[1]  = fr->bmin.y; box[2]  = fr->bmin.z;
    box[3]  = fr->bmin.x; box[4]  = fr->bmin.y; box[5]  = fr->bmax.z;
    box[6]  = fr->bmin.x; box[7]  = fr->bmax.y; box[8]  = fr->bmin.z;
    box[9]  = fr->bmin.x; box[10] = fr->bmax.y; box[11] = fr->bmax.z;
    box[12] = fr->bmax.x; box[13] = fr->bmin.y; box[14] = fr->bmin.z;
    box[15] = fr->bmax.x; box[16] = fr->bmin.y; box[17] = fr->bmax.z;
    /* ORIGINAL BUG: corner 6 should be (bmax.x, bmax.y, bmin.z); the shipped
     * code repeats corner 3, so the model box is short one corner. */
    box[18] = fr->bmin.x; box[19] = fr->bmax.y; box[20] = fr->bmax.z;
    box[21] = fr->bmax.x; box[22] = fr->bmax.y; box[23] = fr->bmax.z;

    TransformVectorsL(light, light, mt, 1);
    TransformVectorsL(box, box, mp, 8);

    FMULA(sc, box,  0, fx);
    FMULA(sc, box,  4, fy);
    FMULA(sc, box,  8, fz);
    FMULA(sc, box, 12, fx);
    FMULA(sc, box, 16, fy);
    FMULA(sc, box, 20, fz);
    FMULA(sc, box, 24, fx);
    FMULA(sc, box, 28, fy);
    FMULA(sc, box, 32, fz);
    FMULA(sc, box, 36, fx);
    FMULA(sc, box, 40, fy);
    FMULA(sc, box, 44, fz);
    FMULA(sc, box, 48, fx);
    FMULA(sc, box, 52, fy);
    FMULA(sc, box, 56, fz);
    FMULA(sc, box, 60, fx);
    FMULA(sc, box, 64, fy);
    FMULA(sc, box, 68, fz);
    FMULA(sc, box, 72, fx);
    FMULA(sc, box, 76, fy);
    FMULA(sc, box, 80, fz);
    FMULA(sc, box, 84, fx);
    FMULA(sc, box, 88, fy);
    FMULA(sc, box, 92, fz);

    lo = sc[0];
    hi = sc[0];
    q = &sc[4];
    lo2 = sc[1];
    hi2 = sc[1];
    cnt = 7;
    do {
        t = q[-1];
        if (t < lo)
            lo = t;
        if (t > hi)
            hi = t;
        t = q[0];
        if (t < lo2)
            lo2 = t;
        if (t > hi2)
            hi2 = t;
        q += 3;
    } while (--cnt);
    ox = 0x500000 - ((hi - lo) >> 1);
    oy = 0x5a0000 - ((hi2 - lo2) >> 1);

    for (i = 0; i < nverts; i++) {
        g_xverts[i * 3]     = fr->verts[i * 3] + cx;
        g_xverts[i * 3 + 1] = fr->verts[i * 3 + 1] - fr->bmin.y;
        g_xverts[i * 3 + 2] = fr->verts[i * 3 + 2] + cz;
    }

    /* TransformVectorsL's body, inlined with src == dst == g_xverts. */
    __asm {
        mov  ebx, mpp
        mov  edi, nverts
        lea  esi, g_xverts
    xf_next:
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
        mov  [esi+8], ecx
        pop  dword ptr [esi+4]
        pop  dword ptr [esi]
        add  esi, 12
        dec  edi
        jne  xf_next
    }

    lo = g_xverts[2];
    hi = g_xverts[2];
    for (i = 0; i < nverts; i++) {
        t = g_xverts[i * 3 + 2];
        if (t < lo)
            lo = t;
        if (t > hi)
            hi = t;
    }
    zscale = 0x40000000 / ((hi - lo) >> 5);
    zb = p->zboost;
    TOFIX(zb);

    cnt = nverts;
    vptr = g_xverts;
    for (i = 0; i < nverts; i++) {
        t = g_xverts[i * 3 + 2] - lo;
        FMUL(t, t, zscale);
        if (p->f2c) {
            FMUL(t, t, zb);
        }
        yy = g_xverts[i * 3 + 1];
        FMUL(yy, yy, ydep);
        g_vert_key[i] = (p->tint << 24) + t + yy;
        FMULP(vptr, 0, fx);
        FMULP(vptr, 4, fy);
        FMULP(vptr, 8, fz);
        vptr += 3;
    }

    parity = TMNegParity(mpp);

    /* Pass 1: the smooth-shaded faces (three normals each). */
    for (i = 0; i < ngour; i++) {
        Vec3i a;
        Vec3i b;
        Vec3i c;

        tp = &tris[i * 3];
        a.x = g_xverts[tp[0] * 3];
        a.y = g_xverts[tp[0] * 3 + 1];
        a.z = g_xverts[tp[0] * 3 + 2];
        b.x = g_xverts[tp[1] * 3];
        b.y = g_xverts[tp[1] * 3 + 1];
        b.z = g_xverts[tp[1] * 3 + 2];
        c.x = g_xverts[tp[2] * 3];
        c.y = g_xverts[tp[2] * 3 + 1];
        c.z = g_xverts[tp[2] * 3 + 2];
        if (parity) {
            v[0].x = ox + 2 * (a.z + a.x);
            v[0].y = a.y - a.x + a.z + oy;
            v[1].x = ox + 2 * (b.z + b.x);
            v[1].y = b.y - b.x + b.z + oy;
            v[2].x = ox + 2 * (c.z + c.x);
            v[2].y = c.y - c.x + c.z + oy;
        } else {
            v[2].x = ox + 2 * (a.z + a.x);
            v[2].y = a.y - a.x + a.z + oy;
            v[1].x = ox + 2 * (b.z + b.x);
            v[1].y = b.y - b.x + b.z + oy;
            v[0].x = ox + 2 * (c.z + c.x);
            v[0].y = c.y - c.x + c.z + oy;
        }
        dx1 = v[1].x - v[0].x;
        dy1 = v[1].y - v[0].y;
        dx2 = v[2].x - v[0].x;
        dy2 = v[2].y - v[0].y;
        FMUL(cr1, dx1, dy2);
        FMUL(cr2, dx2, dy1);
        if (cr1 - cr2 < 0) {
            v[0].z = g_vert_key[tp[0]];
            v[1].z = g_vert_key[tp[1]];
            v[2].z = g_vert_key[tp[2]];
            __asm {
                mov ecx, i
                lea ebx, [ecx+ecx*8]
                lea ebx, [ebx*4]
                mov ecx, fr
                add ebx, [ecx+40]
            }
            SHADE(0, 0);
            SHADE(12, 28);
            SHADE(24, 56);
            if (face->flags & 0x2000) {
                SetFlatColour(face->tex);
                DrawGouraudTri(&v[0], &v[1], &v[2]);
            } else {
                v[0].u  = *(int*)&face->uv[0];
                v[0].vv = *(int*)&face->uv[1];
                v[1].u  = *(int*)&face->uv[2];
                v[1].vv = *(int*)&face->uv[3];
                v[2].u  = *(int*)&face->uv[4];
                v[2].vv = *(int*)&face->uv[5];
                SetTexture(face->tex);
                DrawGouraudTexTri(&v[0], &v[1], &v[2]);
            }
        }
        face++;
    }

    /* Pass 2: the flat-shaded remainder (one normal per face). */
    nrm += i * 6;
    for (; i < nfaces; i++) {
        Vec3i a;
        Vec3i b;
        Vec3i c;

        tp = &tris[i * 3];
        a.x = g_xverts[tp[0] * 3];
        a.y = g_xverts[tp[0] * 3 + 1];
        a.z = g_xverts[tp[0] * 3 + 2];
        b.x = g_xverts[tp[1] * 3];
        b.y = g_xverts[tp[1] * 3 + 1];
        b.z = g_xverts[tp[1] * 3 + 2];
        c.x = g_xverts[tp[2] * 3];
        c.y = g_xverts[tp[2] * 3 + 1];
        c.z = g_xverts[tp[2] * 3 + 2];
        if (TMNegParity(mpp)) {
            v[0].x = ox + 2 * (a.z + a.x);
            v[0].y = a.y - a.x + a.z + oy;
            v[1].x = ox + 2 * (b.z + b.x);
            v[1].y = b.y - b.x + b.z + oy;
            v[2].x = ox + 2 * (c.z + c.x);
            v[2].y = c.y - c.x + c.z + oy;
        } else {
            v[2].x = ox + 2 * (a.z + a.x);
            v[2].y = a.y - a.x + a.z + oy;
            v[1].x = ox + 2 * (b.z + b.x);
            v[1].y = b.y - b.x + b.z + oy;
            v[0].x = ox + 2 * (c.z + c.x);
            v[0].y = c.y - c.x + c.z + oy;
        }
        dx1 = v[1].x - v[0].x;
        dy1 = v[1].y - v[0].y;
        dx2 = v[2].x - v[0].x;
        dy2 = v[2].y - v[0].y;
        FMUL(cr1, dx1, dy2);
        FMUL(cr2, dx2, dy1);
        if (cr1 - cr2 < 0) {
            v[0].z = g_vert_key[tp[0]];
            v[1].z = g_vert_key[tp[1]];
            v[2].z = g_vert_key[tp[2]];
            __asm {
                mov ecx, i
                lea ecx, [ecx+ecx*2]
                mov ebx, nrm
                lea ebx, [ebx+ecx*4]
            }
            SHADE(0, 0);
            if (face->flags & 0x2000) {
                SetFlatColour(face->tex);
                DrawFlatTri(&v[0], &v[1], &v[2]);
            } else {
                v[0].u  = *(int*)&face->uv[0];
                v[0].vv = *(int*)&face->uv[1];
                v[1].u  = *(int*)&face->uv[2];
                v[1].vv = *(int*)&face->uv[3];
                v[2].u  = *(int*)&face->uv[4];
                v[2].vv = *(int*)&face->uv[5];
                SetTexture(face->tex);
                DrawFlatTexTri(&v[0], &v[1], &v[2]);
            }
        }
        face++;
    }
}
