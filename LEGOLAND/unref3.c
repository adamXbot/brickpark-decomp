/* LEGOLAND -- dead (linker-retained) functions from the coaster track /
 * castle object translation units, 0x004238a0..0x0042a020.
 * VC6 SP3 /O2 /Gy /Gd.  Nothing live in the binary calls these; the game was
 * built without /OPT:REF so they survived into the image.  Types are local
 * and describe the original ABI; names are ours (no export carries them).
 *
 * The neighbourhood is coaster3d.c / coastermath.c / coastertiny.c /
 * schoolcar.c / coaster.c: the isometric camera at 0x008299bc, the clip
 * ring at 0x00829a3c, the 16-byte transformed-vertex buffers TransformVerts
 * (0x00426250) fills, and the coaster's own .sav blob.
 */

typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct Mat3 { float m[9]; } Mat3;
typedef struct Mat4 { float m[16]; } Mat4;
typedef struct Pos16 { short x, y; } Pos16;

/* A transformed vertex as TransformVerts leaves it when the stride is 0x10:
 * integer screen x/y/z plus the clip word.  (coaster3d.c's TrackVtx is the
 * same head with a shade word at +0x10 and a stride of 0x14.) */
typedef struct ScreenPt {
    int x;      /* +0x00 */
    int y;      /* +0x04 */
    int z;      /* +0x08 */
    int clip;   /* +0x0c */
} ScreenPt;

/* The surface descriptor GetSprite fills; the same 24-byte object coaster9.c
 * calls VideoSurfaceInfo. */
typedef struct SpriteHandle {
    long  pitch;    /* +0x00  bytes per row */
    int   width;    /* +0x04 */
    int   height;   /* +0x08 */
    void* bits;     /* +0x0c */
    int   unused;   /* +0x10 */
    int   format;   /* +0x14 */
} SpriteHandle;

/* The clip-rectangle ring node, as schoolcar.c's CoasterRegion.  +0x10 is
 * the code the rect helper at 0x004265d0 produces against a reference rect. */
typedef struct CoasterRegion {
    int                   rect[4];  /* +0x00 */
    int                   code;     /* +0x10 */
    struct CoasterRegion* f14;      /* +0x14 */
    struct CoasterRegion* next;     /* +0x18 */
} CoasterRegion;

/* ---- globals ----------------------------------------------------------- */
extern int   g_610a04;                          /* 0x00610a04  castle placed */
extern Mat4  g_view_xf;                         /* 0x008299bc */
extern Mat4  g_view_matrix;                     /* 0x008299fc */
extern Mat4  g_view_tmpl;                       /* 0x004b5c1c */
extern Mat4  g_view_base;                       /* 0x004b5c5c */
extern Vec3f g_eye;                             /* 0x008299a0 */
extern int   g_view_left;                       /* 0x008299ac */
extern int   g_view_top;                        /* 0x008299b0 */
extern int   g_view_right;                      /* 0x008299b4 */
extern int   g_view_bottom;                     /* 0x008299b8 */
extern CoasterRegion g_coaster_regions;         /* 0x00829a3c */

/* ---- callees ----------------------------------------------------------- */
extern int  GetSprite(SpriteHandle* out, void* s);              /* 0x00497c30 */
extern int  ReleaseSprite(SpriteHandle* h);                     /* 0x00497dc0 */
extern void TransformVerts(const Vec3f* s, void* d, const Mat4* m,
                           int stride, int n);                  /* 0x00426250 */
extern int  Rect_ClipCode(const int* rect, const int* ref);      /* 0x004265d0 */

/* =========================================================================
 * 0x004238a0 -- plot an array of already-transformed points.
 *
 * Locks the screen surface, and for every point strictly inside the coaster
 * view window writes one 16-bit pixel at (pitch/2)*y + x.  The stride is
 * 0x10, i.e. the layout TransformVerts produces for this module; the colour
 * is a short.  0x004237f0 (the 30-step line rasteriser) is its only caller
 * and is itself dead.
 *
 * The bounds test is STRICT on all four edges -- a point exactly on the left
 * or top edge is dropped -- and the loop is the module's `while (n-- > 0)`
 * shape (mov/dec/test/jle plus the rebuilt `lea edi,[eax+1]` trip count).
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x004238a0
void Coaster3D_PlotPoints(const ScreenPt* p, int n, short colour)
{
    SpriteHandle surf;

    if (GetSprite(&surf, 0)) {
        while (n-- > 0) {
            if (p->x > g_view_left && p->x < g_view_right &&
                p->y > g_view_top && p->y < g_view_bottom)
                ((unsigned short*)surf.bits)[(surf.pitch >> 1) * p->y + p->x] =
                    colour;
            p++;
        }
        ReleaseSprite(&surf);
    }
}

// FUNCTION: LEGOLAND 0x00423930
int Castle_IsBuilt(void)
{
    return g_610a04;
}

// FUNCTION: LEGOLAND 0x004239a0
void CastleTrack_Noop(void)
{
}

/* g_view_xf.m[6] -- the isometric basis' y-from-z term. */
// FUNCTION: LEGOLAND 0x004260e0
float Coaster3D_GetViewZScale(void)
{
    return g_view_xf.m[6];
}

// FUNCTION: LEGOLAND 0x00426230
void Coaster3D_TransformPoints(const Vec3f* src, ScreenPt* dst, int n)
{
    TransformVerts(src, dst, &g_view_matrix, 0x10, n);
}

/* Re-classify every node of the clip ring against the current view window. */
// FUNCTION: LEGOLAND 0x00426680
void CoasterRegions_UpdateClipCodes(void)
{
    CoasterRegion* p = g_coaster_regions.next;

    while (p != &g_coaster_regions) {
        p->code = Rect_ClipCode(p->rect, &g_view_left);
        p = p->next;
    }
}
