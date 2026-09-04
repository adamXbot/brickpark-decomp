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

/* Residual 52.0% (matchfull 525/1010 of 1023; audit strict mismatch 687).  Up from
 * 44.3%/447 and mismatch 981 this round -- THREE source-shape errors were found and
 * fixed, all read straight off the disassembly, and all committed:
 *     metric (LCS over the 1023)      before -> after
 *     mnemonic only                    963  ->  980   (96%)
 *     ebp-offset-blind                 707  ->  820   (80%)
 *     register+offset-blind            838  ->  871
 *     strict                           390  ->  473
 *     strict, index-for-index          298  ->  305   (audit mismatch 981 -> 687)
 * The FIRST divergence is still index 14, `mov [ebp-0x1c],0x47800000` against our
 * `mov [ebp-8],...` -- a frame offset (indices 8 and 10 are relocation
 * placeholders, not mismatches).  Offset-blind index-for-index agreement is now
 * 500 against a strict 305, so ~195 of the 687 are instructions that agree in
 * mnemonic, registers and immediates and differ ONLY in their [ebp-N]: the frame
 * permutation is still the single largest item.  Tools: scratchpad/laneI/{sc,
 * regions,pair,frame,tryx}.py and scratchpad/w7person3d/{t,ofs,ofs2,syn,s1..s8,
 * b1..b24}.py -- ofs.py prints the ORIGINAL's per-slot reference map, ofs2.py
 * prints ours and the original's side by side, syn.py+s*.py are the frame testbed,
 * t.py applies a source mutation and reports frame map + 5 metrics.
 *
 * FIXED THIS ROUND (four changes, each with the disassembly evidence):
 *  1. `box` is `Vec3i box[8]` and CORNER 0 IS A WHOLE-STRUCT COPY, `box[0] =
 *     fr->bmin;`.  The original materialises the source address (`mov eax,esi` at
 *     94) and does three moves through eax at 96..101, then goes back to esi for
 *     the other seven corners -- the signature of one struct assignment followed by
 *     element-wise stores.  It is also what breaks VC6's CSE of `fr->bmin.x`
 *     between the cx sum and box[0]: with 24 scalar stores we loaded bmin.x first
 *     and reused it, which mis-scheduled the whole 53-instruction box init.  Worth
 *     +36 offset-blind.  (`int box[24]` with `*(Vec3i*)&box[0] = fr->bmin;` is
 *     byte-identical -- the struct-array type is not itself a lever, the copy is.)
 *  2. The transpose reads `p->matrix[k]` DIRECTLY, not through the `mp` pointer
 *     local.  With `mt[k] = mp[j]` VC6 emits `add edi,0x58`, clobbering `p`, and
 *     addresses the matrix as [edi+4..0x20]; the original emits `lea eax,[edi+0x58]`
 *     (58), stores it to BOTH pointer homes (60/61), keeps p in edi and addresses
 *     the matrix as [edi+0x5c..0x78].  Worth +49 offset-blind, +15 mnemonic, and it
 *     is what took the audit mismatch from 980 to 741.  `mp`/`mpp` are still needed
 *     for the two calls; only the nine transpose reads change.
 *  3. `vptr` is RECOMPUTED FROM THE INDEX inside the key loop -- `vptr =
 *     &g_xverts[i * 3];` as the FIRST statement of the body -- not walked with
 *     `vptr += 3`.  The original has NO vptr increment in its loop tail (535..540
 *     is `mov eax,cnt / add esi,0xc / add edi,4 / dec eax / mov cnt,eax / jne`); it
 *     strength-reduces vptr to `lea edx,[esi-8]` off the esi that walks g_xverts by
 *     12 and re-stores it every iteration.  Worth +71 strict, mismatch 741 -> 687.
 *     Position matters: assigning vptr AFTER the `t = ... - lo` statement is 15
 *     offset-blind worse than assigning it first.
 *  4. `light[0..2]` is initialised BEFORE the transpose, not after: the original
 *     emits the three light stores at 73..75, in the middle of the mt run (mt[6..8]
 *     follow at 76..78).  Worth +6 offset-blind index-for-index; the two placements
 *     `light` before `mp = p->matrix` and `light` between the two blocks are
 *     byte-identical.
 *
 * THE FRAME.  Target map, unchanged and re-confirmed instruction by instruction
 * (every [ebp-N] the original touches was enumerated with ofs.py and attributed):
 *     -0x00c light[3]        (top of frame, NOTHING above it)
 *     -0x030..-0x010   9 scalars, the 9 hottest (11..17 refs; k65536 -0x1c,
 *                      nverts -0x20, fy -0x10)
 *     -0x054 mt[9]
 *     -0x078..-0x058   b (Vec3i at -0x78, .y never homed), c (at -0x6c, likewise),
 *                      and 3 scalars: -0x58 ydep(9), -0x5c mp(8), -0x60 (7)
 *     -0x0d8 box[24]
 *     -0x0f0..-0x0dc   a (Vec3i at -0xf0) + 3 scalars: -0xe4 nrm, -0xe0, -0xdc mpp
 *     -0x150 sc[24]      (only sc[0] 25x, sc[1] and &sc[4] are ever named)
 *     -0x1a4 v[3]        (v[1] -0x188, v[2] -0x16c: Vertex2D is 28 bytes, and
 *                        v[2]+28 == sc, so v is provably 84 bytes)
 *     -0x1b4..-0x1a8   the 4 coldest scalars, identified from indices 26..36:
 *                      fr -0x1b4, nfaces -0x1b0, tris -0x1ac, ngour -0x1a8
 * Ours: light -0x1c, v -0xac, mt -0xd8, box -0x150, sc -0x1b4.  Same size (0x1b4),
 * same objects, same 79 referenced slots, and -- verified slot by slot with ofs2.py
 * -- the same per-slot reference counts.  It is a pure permutation and only `v` is
 * out of place: we get light, v, mt, box, sc where the original has light, mt, box,
 * sc, v.
 *
 * FRAME LAYOUT, WHAT IS NOW PROVEN (synthetic testbed scratchpad/w7person3d/
 * {syn,s1..s8}.py plus scratchpad/laneI/{gen,ph,wt}.py, ~70 compiles):
 *  (a) DECLARATION ORDER IS INERT (re-confirmed on this file, 3 permutations).
 *  (b) ANY `__asm` block reverses the whole frame order (previous lane).  NEW: the
 *      POSITION and COUNT of asm blocks are inert -- 10 placements from before the
 *      first statement to after the last, and 1/2/5 blocks, all byte-identical.
 *      Only presence matters.
 *  (c) The asm-world order is ASCENDING BYTE SIZE.  NEW: it is BYTES, not elements
 *      -- `struct {int f0..f6;} A[3]` and `int A[21]` give byte-identical frames.
 *  (d) NEW, and it corrects the previous lane: BLOCK SCOPE IS NOT WORTH "exactly
 *      one position".  In the testbed an 84-byte array declared in ONE block goes to
 *      the FAR END, past two 96-byte arrays; the same array declared in TWO disjoint
 *      blocks behaves exactly like a function-level local.  The previous lane
 *      measured "one position and no farther" with `Vertex2D v[3]` declared inside
 *      BOTH face loops -- which is precisely the case that cancels the effect.
 *  (e) NEW: it is a WEIGHT effect, and scope is only one unit of it.  With two
 *      96-byte arrays at 24 references each, the 84-byte array goes last at <=16 of
 *      its own references and rises to its size position at >=32; block scope buys
 *      about one step of the same currency, and adding references to the 96-byte
 *      arrays pushes the 84-byte one down just as well.
 *  (f) NEW: references made ONLY through `lea X` inside an `__asm` block carry LOW
 *      weight.  With the two 96-byte arrays referenced only that way -- exactly what
 *      FMULA does to `box` and `sc` -- the 84-byte array rises above them at every
 *      weight tested.  This is why `v` sits where it does for us.
 * MEASURED ON THIS FILE, and this is the dead end:
 *  - `Vertex2D v[3]` in ONE block spanning both face loops moves it exactly one
 *    step (light, mt, v, box, sc) and improves every LCS measure (strict 473->481,
 *    ofs 817->833, reg 868->878, mnem 980->982) -- but leaves index-for-index
 *    agreement at 305, so the audit number does not move.  NOT COMMITTED: it does
 *    not reproduce the original's placement, so it is not what the original had,
 *    and the gain is diffuse register/offset luck, not a structural fix.
 *  - Cutting v's references by ~24 (dropping the parity else-arms, or the 12 UV
 *    stores, or both -- semantics-breaking probes) moves it exactly one step and NO
 *    FARTHER, with or without the block scope.  The two levers DO NOT STACK: block,
 *    weight, and block+weight all land on light, mt, v, box, sc.
 *  - Block-scoping any subset of {v, sc, mt, light} (all 15 combinations) never
 *    yields light, mt, box, sc, v.  Putting `box` in a block changes the frame SIZE
 *    and costs 18 instructions, so it is not usable at all.
 *  - Statement order does not move any array (light before/after mt, cx/cz moved).
 *  - `Vertex2D v0, v1, v2` as three separate objects: MEASURED, not argued -- they
 *    scatter to -0x74/-0x90/-0xd8 and are never contiguous, so they cannot be the
 *    original's 28-byte-stride run ending exactly at sc.  (It scored higher than
 *    the array at the time, purely through register allocation; not committed.)
 * SO: our reference profile is IDENTICAL to the original's, slot for slot, which
 * means no weight argument can distinguish the two layouts -- the original's `v` is
 * NOT reachable from any weight, scope, size, order or asm knob measured here.  The
 * one construct that still produces the original's ARRAY sequence is making v and
 * sc a single 180-byte object (`struct { Vertex2D v[3]; int sc[24]; }`), which
 * gives light, mt, box, w exactly; it lands w at -0x1b0 where the original needs
 * -0x1a4, because only `fr` colours below it where the original puts all four cold
 * scalars.  It is not credible source and it is not committed, but the coincidence
 * (sc == v + 84 in the original) is still the only thing that reproduces the order.
 *
 * SCALAR COLOURING, newly measured and a clean rule: the original's 19 scalar slots
 * are in STRICTLY DESCENDING reference count as you walk down from ebp --
 * 17,16,16,15,13,12,12,12,11 | 9,8,7 | 4,4,4 | 3,3,3,2 -- with the arrays inserted
 * between the groups.  Ours obeys the same rule except at the top, where VC6
 * coalesces several of our scalars into FOUR slots ABOVE light (14,14,11,16 refs)
 * that the original does not have at all; the original's light[3] is the topmost
 * object with nothing above it.  Closing that is worth the top of the frame.
 *
 * WHAT IS LEFT, in order of size:
 *  1. The frame permutation, ~195 instructions (see above).
 *  2. The two triangle-setup blocks, orig 557..660 and 811..905, ~30 instructions.
 *     The original works under more register pressure than we do: it re-loads b.z,
 *     c.z and c.x from their homes (598,600,602,608,610) and uses a DESTRUCTIVE
 *     `add edi,esi` for `a.z + a.x` at 583, then re-loads a.z at 585; we keep them
 *     live and use `lea edi,[eax+esi]`.  Every spelling of the two screen-coordinate
 *     expressions canonicalises (6 association variants of `a.y - a.x + a.z + oy`,
 *     2 of `ox + 2*(a.z + a.x)`, and swapping the .x/.y store order, which is 80
 *     offset-blind WORSE), so this is allocation, not source.  Three separate corner
 *     pointers, and reading tp[0..2] into locals first, are both neutral.
 *  3. The deferred `neg ebx / sar edi,1 / sar ebx,1` at 131..133: the original
 *     computes the two bbox sums at 91/93, sinks the negate and both shifts past the
 *     53-instruction box init, and keeps cx in edi and cz in ebx across it while we
 *     spill cz.  Every split spelling was measured (`cx = a+b; ...; cx = -cx>>1`,
 *     the same for cz alone, cx alone, and `-(a+b)` then `>>= 1`) and all lose 20 to
 *     60 offset-blind; the four operand orders canonicalise.  It is scheduling.
 *  4. The lo/hi scan keeps `hi` in memory in the original and in a register for us
 *     (3 instructions at 374..377), and `set->n_faces/n_gouraud/tris` are read in
 *     one run there and split by a store here (2 instructions at 33).
 *
 * STRUCT CHECK.  The `pad2c[4]` bug found in another file's Person3D is NOT here:
 * this file's local Person3D has no padding after +0x24, so f2c/f30/zboost sit at
 * +0x2c/+0x30/+0x3c, and every p-> offset this body emits was compared against the
 * original one for one.  Note the naming differs between files: the field another
 * lane calls `depth` is +0x3c (this file's `zboost`); `Person3D::depth` here is the
 * print-list sort key at +0x54, a different field.
 *
 * NOT ASSEMBLY.  Checked, per the standing question: the ebp frame and the
 * unconditional ebx/esi/edi save come from the `__asm` macros above, the pushes
 * are at the top of the prologue (not inside the stream), there is no `xchg`
 * against memory anywhere in the 1023 instructions, and the body is ordinary
 * scheduled C around them.  This is mixed C + inline asm, not a naked routine. */
// WIP-FUNCTION: LEGOLAND 0x00440a30  (52.0%, 525/1010 insns of 1023)
void Draw3DPersonModel(Person3D* p)
{
    Vertex2D v[3];
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
    int      cr1, cr2;
    int*     tp;
    int*     gp;
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
    tris = set->tris;
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
        Vec3i a;
        Vec3i b;
        Vec3i c;

        tp = &tris[i * 3];
        gp = &g_xverts[tp[0] * 3];
        a.x = gp[0];
        a.y = gp[1];
        a.z = gp[2];
        gp = &g_xverts[tp[1] * 3];
        b.x = gp[0];
        b.y = gp[1];
        b.z = gp[2];
        gp = &g_xverts[tp[2] * 3];
        c.x = gp[0];
        c.y = gp[1];
        c.z = gp[2];
        if (!parity) {
            v[2].x = ox + 2 * (a.z + a.x);
            v[2].y = a.y - a.x + a.z + oy;
            v[1].x = ox + 2 * (b.z + b.x);
            v[1].y = b.y - b.x + b.z + oy;
            v[0].x = ox + 2 * (c.z + c.x);
            v[0].y = c.y - c.x + c.z + oy;
        } else {
            v[0].x = ox + 2 * (a.z + a.x);
            v[0].y = a.y - a.x + a.z + oy;
            v[1].x = ox + 2 * (b.z + b.x);
            v[1].y = b.y - b.x + b.z + oy;
            v[2].x = ox + 2 * (c.z + c.x);
            v[2].y = c.y - c.x + c.z + oy;
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
        gp = &g_xverts[tp[0] * 3];
        a.x = gp[0];
        a.y = gp[1];
        a.z = gp[2];
        gp = &g_xverts[tp[1] * 3];
        b.x = gp[0];
        b.y = gp[1];
        b.z = gp[2];
        gp = &g_xverts[tp[2] * 3];
        c.x = gp[0];
        c.y = gp[1];
        c.z = gp[2];
        if (!TMNegParity(mpp)) {
            v[2].x = ox + 2 * (a.z + a.x);
            v[2].y = a.y - a.x + a.z + oy;
            v[1].x = ox + 2 * (b.z + b.x);
            v[1].y = b.y - b.x + b.z + oy;
            v[0].x = ox + 2 * (c.z + c.x);
            v[0].y = c.y - c.x + c.z + oy;
        } else {
            v[0].x = ox + 2 * (a.z + a.x);
            v[0].y = a.y - a.x + a.z + oy;
            v[1].x = ox + 2 * (b.z + b.x);
            v[1].y = b.y - b.x + b.z + oy;
            v[2].x = ox + 2 * (c.z + c.x);
            v[2].y = c.y - c.x + c.z + oy;
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
