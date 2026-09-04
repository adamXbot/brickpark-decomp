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
/* EXACT (316/316, audit [OK]).  Three levers took this from 83.6% to 100%, all
 * of them "the same statement, spelled the way VC6 wants it":
 *   1. THE FACESET INIT IS A memset, NOT THREE FIELD STORES.  Three separate
 *      `set->field = 0;` assignments emit three `mov [reg+N],0` immediates;
 *      `memset(set, 0, sizeof(FaceSet))` emits `xor ecx,ecx` plus three stores
 *      OF THAT ZERO REGISTER, which is what the original has -- and because the
 *      register form is two instructions shorter, VC6 then interleaves them
 *      with the RES_ReadFile argument pushes exactly as the original does.
 *      This one lever was worth 202 of the 239 mismatches; it also re-colours
 *      the spill homes so k65536 lands at ebp-0x10 (not ebp-0x18) and the
 *      frame stores come out `[ecx+edi+N]` instead of `[edi+ecx+N]`.
 *   2. THE NORMALISE LOOP'S BYTE OFFSET IS A STRENGTH-REDUCED IV, NOT A SECOND
 *      COUNTER.  `for (k = 0, j = 0; k < n; k++, j += 12)` initialises BOTH
 *      counters before the loop guard; the original stores k's zero before the
 *      `jle` and j's zero AFTER it, i.e. in the loop PREHEADER, which is where
 *      VC6 puts an induction variable IT created.  Writing the body's offset as
 *      `k * 12` and dropping j reproduces that placement exactly.
 *   3. THE THREE RGB BYTES ARE READ THROUGH A `unsigned char*`.  Read as
 *      `anim->faces[i].rgb[n]`, VC6 allocates the byte destinations al/dl/cl in
 *      that order, so the FIRST load kills the CSE'd `lea eax,[edi+edx]` face
 *      address and the next two have to rematerialise `[esi+8]` (two extra
 *      instructions).  Hoisting `unsigned char* s = anim->faces[i].rgb;` and
 *      reading s[2]/s[1]/s[0] flips the allocation to cl/dl/al -- the pointer
 *      survives in eax until the last load -- and keeps the load/store
 *      interleave.  Measured and rejected on the way: byte or int temporaries
 *      for any subset of the three (VC6 copy-propagates them away unless ALL
 *      the loads precede ALL the stores, which then groups the loads), a
 *      `Face3D* f` for the whole loop body, and reversing the assignment order. */
// FUNCTION: LEGOLAND 0x0043fa80
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
        memset(set, 0, sizeof(FaceSet));
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
            for (k = 0; k < nn.n; k++)
                NormaliseVector((float*)((char*)anim->frames[i].normals + k * 12));
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
                    unsigned char* s = anim->faces[i].rgb;
                    nn.rgb[2] = s[2];
                    nn.rgb[1] = s[1];
                    nn.rgb[0] = s[0];
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

/* g_xverts is walked both as a flat int array and as an array of 12-byte
 * vertices; the three corners are read out with WHOLE-STRUCT copies. */
#define XV ((Vec3i*)g_xverts)

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

/* Residual 63.3% (matchfull 648/1023; audit strict mismatch 377).  ONE
 * reconstruction error was found this round and it closed the whole 24..37 head
 * clump; the frame permutation is now 97% of what is left.
 *     metric (index-for-index / LCS over the 1023)   before   ->   after
 *     mnemonic only                               1008/1019  ->  1016/1021
 *     ebp-offset-blind                             944/ 956  ->   957/ 962
 *     register+offset-blind                        947/ 958  ->   957/ 963
 *     strict                                       582/ 588  ->   590/ 592
 *     audit strict mismatch                             386  ->        377
 * Instruction count is 1023, the original's, and 3511 bytes against 3523 (the
 * 12 are disp8/disp32 encodings of the wrong frame offsets).  With registers,
 * [ebp-N] offsets and immediates ALL blinded only SEVEN of the 1023 differ (was
 * 21); the alignment is exact from 0 to 88 and from 139 to the end -- one
 * unbroken 884-instruction run -- with a single 2-slot window at 131..138.
 * Classifying the 377 index-for-index mismatches: 367 differ ONLY in their
 * [ebp-N] (the frame permutation), 10 are the two clumps under WHAT IS LEFT.
 * Tools: scratchpad/w9person3d/{t9,ofs3,cls2,syn9,sw1..sw10,e1..e16}.py --
 * t9.py applies source substitutions and reports 5 metrics, the audit mismatch
 * (relocation-only differences excluded) and the frame array order; syn9.py +
 * sw*.py are a SYNTHETIC MODEL of this function's frame (LIGHT 12 / MT 36 /
 * BOX 96 / SC 96 / V 84 with matching reference kinds) that reproduces our
 * array order exactly and was used to measure the layout rules below.
 * Also scratchpad/w8person3d/{dump,mreg,cls,drift}.py and
 * scratchpad/w7person3d/{ofs,ofs2}.py.
 *
 * FIXED THIS ROUND (one reconstruction error, with the disassembly evidence):
 *  1. `nverts = fr->n_verts;` IS READ BETWEEN `ngour = set->n_gouraud;` AND
 *     `tris = set->tris;` -- not after all three FaceSet fields.  Evidence, the
 *     original at 24..43: it issues `fld [edi+0x38]` (p->ydepth) at 24, BEFORE
 *     `mov eax,[esi+0x20]`, then runs the three FaceSet loads back to back at
 *     29,30,31 with `mov eax,[eax+8]` KILLING the set pointer, and stores in the
 *     order nfaces(32), tris(34), ngour(36), nverts(38).  With nverts read last
 *     we emitted the set load and the `fr` spill BEFORE the fld, split the trio
 *     (two loads, a store, then the third load into edx instead of eax) and
 *     stored nfaces, ngour, nverts, tris.  Reading nverts third makes 24..43
 *     instruction-for-instruction exact.  ALL 120 orders of the five head
 *     assignments were compiled: this one is the unique best (377), the old
 *     order was 386 and the next best is 379.  Every structural measure rose
 *     with it (mnemonic +8, offset-blind +13, register-blind +10), so it is an
 *     honest fix and not a compensating one.
 *
 * WHAT IS LEFT.  Two things, and one of them is 97% of it.
 *  1. THE FRAME PERMUTATION -- 367 of the 377.  Every one of those instructions
 *     agrees with the original in mnemonic, registers and immediates and differs
 *     ONLY in its [ebp-N].  See THE FRAME below.
 *  2. Two scheduling clumps, 10 instructions:
 *      - 88..89 (2): the original loads `fr->bmax.x` into edi and then
 *        `fr->bmin.x` into ebx for the cx sum; we load them the other way round.
 *        Same registers, same `add edi,ebx`, same `neg edi` -- purely which
 *        field lands in the accumulator.  The cz sum already matches (both load
 *        bmax.z then bmin.z and accumulate into ebx).  RE-MEASURED THIS ROUND on
 *        the new baseline: both operand orders x2 both statement orders (8
 *        variants) are BYTE-IDENTICAL for cx-first and 4 worse for cz-first; a
 *        split `cx = a+b; cx = -cx>>1;`, a fully split negate-then-shift, `(0 -
 *        ...)`, a `t = fr->bmax.x` temp, and five placements of the cz statement
 *        are identical or much worse.  The two-operand sum canonicalises.
 *      - 131..138 (8): the original sinks `neg ebx / sar edi,1 / sar ebx,1` INTO
 *        the box-init run -- after box[5].z's LOAD at 130 but BEFORE its store
 *        at 134 -- and issues `push 1` at 136; we emit the same four
 *        instructions two positions later.  This is the only alignment drift
 *        left (-2, closed again at 139).  Five placements of
 *        `TransformVectorsL(light, light, mt, 1)` (before cx, before box[0],
 *        mid-box, swapped with the box call) lose 30 to 599; five placements of
 *        the cz statement lose 19 to 31; box[0] moved above cx/cz loses 36.
 *
 * ===========================================================================
 * THE FRAME.  Target map, re-confirmed instruction by instruction (every
 * [ebp-N] the original touches was enumerated with ofs3.py and attributed; the
 * per-slot reference COUNTS of light, mt, box, sc, v and a/b/c are identical to
 * ours slot for slot, which is the whole difficulty):
 *     -0x00c light[3]        (top of frame, NOTHING above it)
 *     -0x030..-0x010   9 scalars (refs 16,16,15,17,13,12,11,12,12 near->far)
 *     -0x054 mt[9]
 *     -0x078..-0x058   3 scalars (9,8,7), c (at -0x6c), b (at -0x78)
 *     -0x0d8 box[24]
 *     -0x0f0..-0x0dc   3 scalars (4,4,4) and a (at -0xf0)
 *     -0x150 sc[24]
 *     -0x1a4 v[3]        (v[1] -0x188, v[2] -0x16c: Vertex2D is 28 bytes and
 *                        v[2]+28 == sc, so v is provably 84 bytes; its per-slot
 *                        profile 12,6,2,2,2 / 9,4,2,2,2 / 9,6,2,2,2 is OURS
 *                        exactly, so v is neither a different size nor a
 *                        different object)
 *     -0x1b4..-0x1a8   4 scalars: fr -0x1b4 (2 refs), nfaces, tris, ngour
 * OURS: light -0x20, mt -0x78, v -0xe8, box -0x154, sc -0x1b4, with a(-0x94),
 * b/a(-0x54) and c(-0x48) pooled and swapped between the loops exactly as the
 * original pools them (profiles (3,-,8) (6,-,10) (8,-,10) both sides).  Same
 * frame size (0x1b4), same 79 referenced slots, same TOTAL reference count
 * (384 = 384).  The array order is light, mt, v, box, sc against the original's
 * light, mt, box, sc, v.
 *
 * SEEN AS A LIST, ours and the original differ ONLY in where the four plain
 * arrays are inserted into an otherwise IDENTICALLY SHAPED 49-slot pool:
 *     ORIG: [light] 9sc [mt] 3sc c b [box] 3sc a [sc] v(21) 4sc
 *     OURS: 5sc [light] 7sc c b [mt] 4sc a v(21) 3sc [box] [sc]
 * -- same [12 scalars][c][b][n scalars][a][v][m scalars] pool spine, with the
 * arrays one "chunk" lower in ours.
 *
 * FRAME RULES -- what is now MEASURED (synthetic model syn9.py + sw1..sw10.py,
 * ~250 compiles, plus ~60 on the real file).  Do not re-derive:
 *  (a) DECLARATION ORDER IS INERT.  Re-confirmed this round: all four
 *      permutations of the sc/box/mt/light declaration block are byte-identical.
 *  (b) Any `__asm` block reverses the whole frame-object order; POSITION and
 *      COUNT are inert (previous lane, 10 placements, 1/2/5 blocks).  NEW: an
 *      asm block inside the `v` block specifically is also inert, and the
 *      reversal is all-or-nothing -- asm only in the outer function, only in the
 *      loop, or both all give the same reversed order.
 *  (c) The default (asm-reversed) order is ASCENDING BYTE SIZE from ebp down --
 *      bytes, not elements.  Ours is exactly that: 12, 36, 84, 96, 96.
 *  (d) *** ASM `lea X` REFERENCES CARRY ZERO WEIGHT. ***  In the synthetic,
 *      deleting 4, 8, 12, 16, 20 or ALL 24 of the `__asm { lea eax, BOX }`
 *      references changes NOTHING -- byte-identical frame.  Only C-level
 *      references rank an array.  (This supersedes the previous lane's "low
 *      weight": it is exactly nil.)
 *  (e) C-level references PROMOTE an array toward ebp, and enough of them beat
 *      size.  Measured on the synthetic at our exact configuration (V 84 bytes,
 *      15 distinct slots, 64 references; BOX/SC 96 bytes, 24 asm-leas each):
 *          +8  C refs on BOX -> unchanged        (LIGHT,MT,V,BOX,SC)
 *          +12 C refs on BOX -> LIGHT,MT,BOX,V,SC
 *          +20 C refs on BOX -> LIGHT,MT,BOX,SC,V   <-- the ORIGINAL's order
 *          +12 C refs on SC  -> LIGHT,MT,BOX,SC,V   <-- also the original's
 *      BUT THE SYNTHETIC'S SENSITIVITY DOES NOT TRANSFER.  On the REAL file,
 *      measured this round: 4/8/12/16/20/24 extra LIVE C reads of box (or of
 *      sc) placed before the transform, after the FMULA run, or after the scan
 *      leave the array order UNCHANGED; so does replacing every
 *      `__asm { lea eax, box }` (and `lea edx, sc`, and SHADE's `lea edx, v`)
 *      with a pointer local, which deletes 24 asm references outright.  The
 *      ONE real-file construct that reaches the original's order is 24 live C
 *      reads of `box` INTERLEAVED WITH the FMULA run itself -- one per macro
 *      expansion (`cx += s[(n)/12].x;` inside FMULA, keeping the asm lea):
 *      that gives light, mt, box, sc, v exactly.  Bunching the same 24 reads
 *      anywhere else does nothing, and each of them costs an instruction, so
 *      there is no zero-cost spelling: the promotion needs box to be READ IN C
 *      at each of the 24 points where the original only leas it in asm.
 *  (f) *** AND THAT IS WHY IT IS UNREACHABLE FROM SOURCE: the weight is the
 *      SURVIVING-IR reference count. ***  Two decisive negatives this round:
 *      (i) routing every C use of `v` through an alias pointer (`Vertex2D* w =
 *      v;`, so `v` is named once) is BYTE-IDENTICAL -- VC6 copy-propagates and
 *      counts after; (ii) 8, 16 or 24 DEAD C reads of box or of sc are
 *      byte-identical too -- they are dead-code-eliminated before the count.
 *      Since our surviving IR reference profile equals the original's slot for
 *      slot (384 = 384, 79 = 79), NO source spelling can change the weights.
 *  (g) Block scope is worth about ONE step of the same currency, and the
 *      threshold is a joint function of distinct slots touched and reference
 *      count.  Full synthetic table for an 84-byte struct array (position 1 =
 *      nearest ebp; there are five positions):
 *          fn-level  D=3: 4 everywhere;  D=8/15: 4 at R<=8, 3 at R>=16
 *          in-block  D=3/8: 4 everywhere;  D=15: 4 at R<=16, 3 at R>=24
 *          inline    D=3/8/15: 5 at R<=2, 4 at R<=16, 3 at R>=24 (D=15)
 *      Our v is D=15, R=64 -> position 3, which is what we get.
 *  (h) NEW CLASS FOUND, and it is the only construct that reaches the far end
 *      on its own: an array born inside a `static __inline` HELPER (an
 *      inline-expansion temporary) sits one step lower than the same array
 *      block-scoped, and at R<=2 it goes PAST both 96-byte arrays to position 5.
 *      It is NOT usable here: the helper would have to contain the SHADE macro,
 *      whose `__asm { imul dword ptr light[0] }` and `__asm { lea edx, v[vo] }`
 *      name `light` and `v` as MASM symbols, so both must be lexically inside
 *      the function that emits them -- and `light` is one object shared by both
 *      loops and initialised before them.  Passing light as a pointer changes
 *      the emitted SHADE.  At R=64 the inline class is at position 3 anyway.
 *  (i) Array SIZE probes (semantics-breaking, diagnostic only): v[2]=56B ->
 *      position 2, v[3]=84B -> 3, v[4]=112B -> 3, v[5]=140B -> 4, v[8]=224B ->
 *      4.  Position 5 is NEVER reached by size, not even at 224 bytes, and 112
 *      bytes still sits above two 96-byte arrays -- so the order is not a size
 *      sort with weight as a tie-break, and `v` cannot be pushed down by being
 *      "really" a bigger object.
 *
 * RULED OUT ON THE REAL FILE THIS ROUND (all measured, do not re-derive):
 *  - Declaration order of the four arrays: 4 permutations, byte-identical.
 *  - Array TYPES: `Vec3i sc[8]` for `int sc[24]`, and `int box[24]` with
 *    `*(Vec3i*)&box[0] = fr->bmin;` for `Vec3i box[8]` -- both byte-identical.
 *    The element type is not a lever; only total bytes are.
 *  - Scope subsets: v at function level (order light,v,mt,box,sc, 417); a/b/c
 *    and/or tp at function level (389); everything at function level (405);
 *    `sc` in a block round the FMULA run and the scan (399, order
 *    light,mt,v,sc,box); `box` in a block (compile shape collapses, 1001);
 *    both (1001).  NOTE, correcting the previous round's note: `v` in ONE block
 *    spanning both loops is NO LONGER byte-identical to one block per loop --
 *    it is now 3 WORSE (389).  Keep one block per loop.
 *  - Alias pointer over `v` (see (f)(i)): byte-identical.
 *  - Dead C reads of box/sc (see (f)(ii)): byte-identical.
 *  - Block-scoping dx1..dy2 and/or crs1/crs2, and giving the second loop its own
 *    crs pair: 385 to 393, order unchanged.  (Re-measured on the new baseline;
 *    the previous round's verdict stands.)
 *  - The sc scan written with DIRECT `sc[i*3]` indexing instead of the `q`
 *    pointer walk: VC6 strength-reduces it back to the same pointer walk and the
 *    instruction count stays 1023, but it is 3 worse (380) and the array order
 *    does NOT move -- the extra 8 C references to sc are not enough (rule (e)
 *    wants 12).  Also tried: an explicit `i`/`cnt` countdown form and re-deriving
 *    `q` from `sc` inside the loop -- both 380.
 *  - Head statement order: all 120 permutations of the five assignments were
 *    compiled (see FIXED THIS ROUND); dropping the `set` local in favour of
 *    `fr->faces->...` three times is byte-identical; moving `set` before `ydep`
 *    or `k65536` later is byte-identical; `fy` early, `face` last and
 *    nverts/nrm before the trio are 62 to 70 worse.
 *
 * FIXED IN EARLIER ROUNDS -- the shapes below are established, do not undo them:
 *  1. `cr2` IS A MASM CONTROL REGISTER.  `FMUL(cr2, dx2, dy1)` expanded to
 *     `__asm { mov cr2, eax }` and MASM emitted the privileged `0f 22 d0`, so
 *     the local was NEVER written and the following compare read an
 *     uninitialised dead-argument slot -- the shipped body would have faulted at
 *     ring 3.  Renamed crs1/crs2.  Swept again this round: no `__asm` operand in
 *     this function collides with a MASM reserved name, and the /FAcs listing
 *     contains no control-register, debug-register or segment move.
 *  2. The INLINED TransformVectorsL's FIRST ROW is spelled the other way round
 *     from the other two and from math3d.c's copy: `mov eax,[esi] / imul dword
 *     ptr [ebx]` at 0x00440e2c against `mov eax,[ebx] / imul dword ptr [esi]` at
 *     0x004433c1.
 *  3. THE THREE CORNERS ARE WHOLE-STRUCT COPIES (`a = XV[tp[0]];`).  VC6
 *     materialises the corner address into a fixed register and dereferences it
 *     three times (566..579); field-wise assignment folds the first field into
 *     `[ecx*4+g]` and emits no `mov ebx,ecx`.  It also reproduces the a<->b
 *     pool-slot swap between the two loops.
 *  4. The screen-x expression is `ox + 2 * (a.x + a.z)`, NOT `(a.z + a.x)`:
 *     with the struct copy in place that operand order stops canonicalising.
 *  5. `Vertex2D v[3]` and `int* tp` are declared INSIDE EACH FACE LOOP.  This is
 *     a SCHEDULING fix, not a frame lever: at function level VC6 will not hoist
 *     a load through `tp`/`face` above a store into the address-taken array and
 *     the key-index and UV blocks lose the original's three-deep software
 *     pipeline at four sites.
 *  6. `box[0] = fr->bmin;` is a WHOLE-STRUCT copy and the other seven corners
 *     are element-wise off `fr` (`mov eax,esi` at 94 then three moves through
 *     eax).  Re-measured this round: field-wise corner 0, in x,y,z or x,z,y
 *     order, is 593 worse.
 *  7. The transpose reads `p->matrix[k]` DIRECTLY, not through `mp`.
 *  8. `vptr` is RECOMPUTED from the index as the FIRST statement of the key
 *     loop, not walked with `vptr += 3`.
 *  9. `light[0..2]` is initialised BEFORE the transpose.
 *
 * STRUCT CHECK.  The `pad2c[4]` bug found in another file's Person3D is NOT
 * here: this file's local Person3D has no padding after +0x24, so f2c/f30/zboost
 * sit at +0x2c/+0x30/+0x3c, and every `p->` offset this body emits was compared
 * against the original one for one.  The field another lane calls `depth` is
 * +0x3c (this file's `zboost`); `Person3D::depth` here is the print-list sort
 * key at +0x54.  `i` and `t` share the dead parameter home [ebp+8] in both
 * bodies (`mov ecx,[ebp+8]` at 685 is `__asm { mov ecx, i }`).
 * NEW CHECK (scratchpad/w9person3d/pmap.py): every one of the 367 offset-only
 * mismatches was tested for a CONSISTENT slot bijection.  All five arrays map
 * one-to-one (-0xc->-0x20, -0x54->-0x78, -0xd8->-0x154, -0x150->-0x1b4,
 * -0x1a4->-0xe8) and the only multi-valued entries are small scalar slots that
 * VC6 coalesces differently (e.g. the original's `fr` at -0x1b4 has its own
 * 2-reference slot while ours shares one with the second loop's `tp`).  No
 * mismatch differs outside its [ebp-N], so there is no wrong struct field
 * offset hiding inside the frame residual.
 *
 * NOT ASSEMBLY.  Checked, per the standing question: the ebp frame and the
 * unconditional ebx/esi/edi save come from the `__asm` macros above, the pushes
 * are at the top of the prologue (not inside the stream), there is no `xchg`
 * against memory anywhere in the 1023 instructions, and the body is ordinary
 * scheduled C around them.  This is mixed C + inline asm, not a naked routine. */
// WIP-FUNCTION: LEGOLAND 0x00440a30  (63.3%, 648/1023 insns; audit mismatch 377,
//                                    of which 367 are frame offsets alone)
void Draw3DPersonModel(Person3D* p)
{
    int      sc[24];
    Vec3i    box[8];
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
    float    k65536;
    int      lo, hi;
    float    fy;
    float    fz;
    Anim3D*  anim;
    FaceSet* set;
    float    zb;
    int      i;
    int      cx, cz, ox, oy;
    /* NOT `cr2`: MASM resolves the bare name `cr2` in `__asm { mov cr2, eax }`
     * to the CONTROL REGISTER, so FMUL(cr2,...) silently assembled to a
     * privileged `mov cr2,eax` (0f 22 d0) and the local was never written.
     * The original stores to a real frame slot (0x00440e... `mov [ebp-0x14],eax`). */
    int      crs1, crs2;
    int      yy;

    anim = GetBlokeAnim3DFromPerson(p);
    k65536 = 65536.0f;
    fx = p->scale_x * 0.447f;
    fz = p->scale_z * 0.447f;
    fr = &anim->frames[p->frame];
    face = p->faces;
    ydep = p->ydepth;
    set = fr->faces;
    nfaces = set->n_faces;
    ngour = set->n_gouraud;
    nverts = fr->n_verts;
    tris = set->tris;
    nrm = fr->normals;
    fy = p->scale_y;
    if (ydep != 0.0f) {
        TOFIX(ydep);
        FIXV(ydep) <<= 8;
    }
    RenderZBufferObject(p->f2c, p->f24, p->f28);

    mp = p->matrix;
    mpp = p->matrix;
    /* light first: the original emits its three stores at 73..75, between
     * mt[5] and mt[6].  The nine transpose reads go through `p->matrix[k]`,
     * NOT through `mp`: `mt[k] = mp[j]` makes VC6 do `add edi,0x58` and lose
     * `p`, where the original keeps p in edi and does `lea eax,[edi+0x58]`. */
    light[0] = -0x1800;
    light[1] = -0x5000;
    light[2] = 0x3000;
    mt[0] = p->matrix[0];
    mt[1] = p->matrix[3];
    mt[2] = p->matrix[6];
    mt[3] = p->matrix[1];
    mt[4] = p->matrix[4];
    mt[5] = p->matrix[7];
    mt[6] = p->matrix[2];
    mt[7] = p->matrix[5];
    mt[8] = p->matrix[8];
    TOFIX(fx);
    TOFIX(fy);
    TOFIX(fz);

    cx = -(fr->bmax.x + fr->bmin.x) >> 1;
    cz = -(fr->bmin.z + fr->bmax.z) >> 1;

    /* Corner 0 is a WHOLE-STRUCT copy (`mov eax,esi` + three moves through it at
     * 94..101) and the other seven are element-wise off fr: that split is read
     * straight off the original, and it is also what stops VC6 CSEing the
     * `fr->bmin.x` this block loads with the one the cx sum above loaded. */
    box[0] = fr->bmin;
    box[1].x = fr->bmin.x; box[1].y = fr->bmin.y; box[1].z = fr->bmax.z;
    box[2].x = fr->bmin.x; box[2].y = fr->bmax.y; box[2].z = fr->bmin.z;
    box[3].x = fr->bmin.x; box[3].y = fr->bmax.y; box[3].z = fr->bmax.z;
    box[4].x = fr->bmax.x; box[4].y = fr->bmin.y; box[4].z = fr->bmin.z;
    box[5].x = fr->bmax.x; box[5].y = fr->bmin.y; box[5].z = fr->bmax.z;
    /* ORIGINAL BUG: corner 6 should be (bmax.x, bmax.y, bmin.z); the shipped
     * code repeats corner 3, so the model box is short one corner. */
    box[6].x = fr->bmin.x; box[6].y = fr->bmax.y; box[6].z = fr->bmax.z;
    box[7].x = fr->bmax.x; box[7].y = fr->bmax.y; box[7].z = fr->bmax.z;

    TransformVectorsL(light, light, mt, 1);
    TransformVectorsL((int*)box, (int*)box, mp, 8);

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
        /* NOTE: the FIRST row is spelled the other way round from the other
         * two and from math3d.c's TransformVectorsL -- vertex into eax,
         * matrix as the imul memory operand.  Read straight off 0x00440e2c
         * (`mov eax,[esi] / imul dword ptr [ebx]`) against the library copy
         * at 0x004433c1 (`mov eax,[ebx] / imul dword ptr [esi]`). */
        mov  eax, [esi]
        imul dword ptr [ebx]
        shrd eax, edx, 16
        mov  ecx, eax
        mov  eax, [esi+4]
        imul dword ptr [ebx+4]
        shrd eax, edx, 16
        add  ecx, eax
        mov  eax, [esi+8]
        imul dword ptr [ebx+8]
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

    for (i = 0; i < nverts; i++) {
        /* vptr is RECOMPUTED from i, not walked: the original has no vptr
         * increment in its loop tail and strength-reduces this to
         * `lea edx,[esi-8]` off the esi that walks g_xverts.  It must be the
         * first statement of the body (15 offset-blind worse after `t`). */
        vptr = &g_xverts[i * 3];
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
    }

    parity = TMNegParity(mpp);

    /* Pass 1: the smooth-shaded faces (three normals each). */
    for (i = 0; i < ngour; i++) {
        Vertex2D v[3];
        Vec3i a;
        Vec3i b;
        Vec3i c;
        int*     tp;

        /* WHOLE-STRUCT copies, not field-by-field: the original materialises
         * each corner's address into one register and dereferences it three
         * times (`lea ecx,[ecx*4+g] / mov ebx,ecx / mov ecx,[ebx] / ...` at
         * 566..579), which field-wise assignment cannot produce -- it folds the
         * first field into `[ecx*4+g]` and needs no `mov ebx,ecx`.  It is also
         * what makes VC6 home +0 and +8 and keep +4 live in every corner block,
         * and what reproduces the a/b pool-slot swap between the two loops.
         * `v` and `tp` must stay block-scope here: with `v` at function level
         * VC6 will not hoist a load through `tp`/`face` above a store into the
         * address-taken array, and the key-index and UV blocks lose the
         * original's three-deep software pipeline. */
        tp = &tris[i * 3];
        a = XV[tp[0]];
        b = XV[tp[1]];
        c = XV[tp[2]];
        if (!parity) {
            v[2].x = ox + 2 * (a.x + a.z);
            v[2].y = a.y - a.x + a.z + oy;
            v[1].x = ox + 2 * (b.x + b.z);
            v[1].y = b.y - b.x + b.z + oy;
            v[0].x = ox + 2 * (c.x + c.z);
            v[0].y = c.y - c.x + c.z + oy;
        } else {
            v[0].x = ox + 2 * (a.x + a.z);
            v[0].y = a.y - a.x + a.z + oy;
            v[1].x = ox + 2 * (b.x + b.z);
            v[1].y = b.y - b.x + b.z + oy;
            v[2].x = ox + 2 * (c.x + c.z);
            v[2].y = c.y - c.x + c.z + oy;
        }
        dx1 = v[1].x - v[0].x;
        dy1 = v[1].y - v[0].y;
        dx2 = v[2].x - v[0].x;
        dy2 = v[2].y - v[0].y;
        FMUL(crs1, dx1, dy2);
        FMUL(crs2, dx2, dy1);
        if (crs1 - crs2 < 0) {
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
        Vertex2D v[3];
        Vec3i a;
        Vec3i b;
        Vec3i c;
        int*     tp;

        tp = &tris[i * 3];
        a = XV[tp[0]];
        b = XV[tp[1]];
        c = XV[tp[2]];
        if (!TMNegParity(mpp)) {
            v[2].x = ox + 2 * (a.x + a.z);
            v[2].y = a.y - a.x + a.z + oy;
            v[1].x = ox + 2 * (b.x + b.z);
            v[1].y = b.y - b.x + b.z + oy;
            v[0].x = ox + 2 * (c.x + c.z);
            v[0].y = c.y - c.x + c.z + oy;
        } else {
            v[0].x = ox + 2 * (a.x + a.z);
            v[0].y = a.y - a.x + a.z + oy;
            v[1].x = ox + 2 * (b.x + b.z);
            v[1].y = b.y - b.x + b.z + oy;
            v[2].x = ox + 2 * (c.x + c.z);
            v[2].y = c.y - c.x + c.z + oy;
        }
        dx1 = v[1].x - v[0].x;
        dy1 = v[1].y - v[0].y;
        dx2 = v[2].x - v[0].x;
        dy2 = v[2].y - v[0].y;
        FMUL(crs1, dx1, dy2);
        FMUL(crs2, dx2, dy1);
        if (crs1 - crs2 < 0) {
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
