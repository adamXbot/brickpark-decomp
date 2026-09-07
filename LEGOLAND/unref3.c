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

/* ---- the track's map squares ------------------------------------------ */
/* One entry of the track's square table: the piece type and the map square
 * it occupies.  Eight bytes, the same shape coastermath.c's TrackNode has. */
typedef struct TrackSquare {
    int   type;     /* +0x00 */
    short x;        /* +0x04 */
    short y;        /* +0x06 */
} TrackSquare;

/* The blob the two .sav helpers below move: a self-describing image whose
 * first dword is its own byte length, with the square table at +0x08/+0x0c. */
typedef struct TrackBlob {
    unsigned int size;      /* +0x00  total byte length, what WriteFile sends */
    int          f04;       /* +0x04 */
    int          count;     /* +0x08  number of squares */
    TrackSquare* squares;   /* +0x0c */
} TrackBlob;

extern int   g_track_place_enabled;                     /* 0x004b55f4 */
extern unsigned short* g_basic_tiles;                   /* 0x00829980 */
extern char  g_coaster_save_path[];                     /* 0x004b5cf4 */
extern char  g_alloc_tag[];                             /* 0x004d8bb0 */

extern void  RestoreBaseMap(int x, int y);                      /* 0x0045da60 */
extern void  SetMapTile(int x, int y, unsigned short tile);     /* 0x00461780 */
extern void  DBPrintf(const char* fmt, ...);                    /* 0x00453a20 */
extern void* AllocZeroed(unsigned int size, int a, void* tag, int c); /* 0x004775b0 */
extern void  Free_w(void* p);                                   /* 0x004775d0 */

__declspec(dllimport) int __stdcall CreateFileA(const char* name,
                                                unsigned int access,
                                                unsigned int share, void* sa,
                                                unsigned int disp,
                                                unsigned int flags,
                                                void* tmpl);            /* [0x4ab258] */
__declspec(dllimport) int __stdcall WriteFile(int h, const void* buf,
                                              unsigned int n,
                                              unsigned int* put,
                                              void* ov);                /* [0x4ab254] */
__declspec(dllimport) int __stdcall GetFileSize(int h, unsigned int* hi); /* [0x4ab25c] */
__declspec(dllimport) int __stdcall CloseHandle(int h);                  /* [0x4ab260] */
__declspec(dllimport) int __stdcall ReadFile(int h, void* buf, unsigned int n,
                                             unsigned int* got, void* ov); /* [0x4ab264] */

/* Every square of the piece plus its three neighbours to the right and below
 * -- the 2x2 the track's own tiles cover -- is repainted from the base map. */
// FUNCTION: LEGOLAND 0x00427570
void Track_RestoreSquareBaseMap(const Pos16* sq)
{
    int x = sq->x;
    int y = sq->y;

    if (g_track_place_enabled) {
        RestoreBaseMap(x, y);
        RestoreBaseMap(x + 1, y);
        RestoreBaseMap(x, y + 1);
        RestoreBaseMap(x + 1, y + 1);
    }
}

/* The inverse: stamp the 2x2 with the second tile of the "BASIC TILES 1"
 * element Track_Create loaded (its first short plus one).  The global is
 * re-read for every call because SetMapTile may write through it. */
// FUNCTION: LEGOLAND 0x004274f0
void Track_StampSquareTiles(const Pos16* sq)
{
    if (g_track_place_enabled) {
        SetMapTile(sq->x, sq->y, (unsigned short)(*g_basic_tiles + 1));
        SetMapTile(sq->x + 1, sq->y, (unsigned short)(*g_basic_tiles + 1));
        SetMapTile(sq->x, sq->y + 1, (unsigned short)(*g_basic_tiles + 1));
        SetMapTile(sq->x + 1, sq->y + 1, (unsigned short)(*g_basic_tiles + 1));
    }
}

/* Debug dump of the square table; the format string is
 * "Track %2x, Type %2x at (%2x, %2x)\n" at 0x004b5cd0. */
// FUNCTION: LEGOLAND 0x00426be0
void TrackBlob_Dump(const TrackBlob* b)
{
    int i;

    for (i = 0; i < b->count; i++)
        DBPrintf("Track %2x, Type %2x at (%2x, %2x)\n", i, b->squares[i].type,
                 b->squares[i].x, b->squares[i].y);
}

/* Write the blob whole to RollerCoaster\RollerCoaster.sav.
 * ORIGINAL BUG: the length is read out of the blob BEFORE the null test on
 * the blob pointer, so the guard is dead for a null argument. */
// FUNCTION: LEGOLAND 0x004272a0
int Coaster_WriteSaveBlob(TrackBlob* b)
{
    int          h;
    unsigned int put;
    unsigned int n = b->size;

    if (g_coaster_save_path && b) {
        h = CreateFileA(g_coaster_save_path, 0x40000000, 0, 0, 2, 0x8000000, 0);
        if (h != -1) {
            WriteFile(h, b, n, &put, 0);
            if (put != n) {
                CloseHandle(h);
                return 0;
            }
            CloseHandle(h);
            return 1;
        }
    }
    return 0;
}

/* Read it back whole into a fresh heap block. */
// FUNCTION: LEGOLAND 0x00427310
void* Coaster_ReadSaveBlob(void)
{
    int          h;
    void*        p;
    unsigned int n;
    unsigned int got;

    if (!g_coaster_save_path)
        return 0;
    h = CreateFileA(g_coaster_save_path, 0x80000000, 1, 0, 3, 0x8000000, 0);
    if (h == -1)
        return 0;
    n = GetFileSize(h, 0);
    p = AllocZeroed(n, 0, g_alloc_tag, 0);
    if (!p) {
        CloseHandle(h);
        return 0;
    }
    ReadFile(h, p, n, &got, 0);
    if (got != n) {
        Free_w(p);
        CloseHandle(h);
        return 0;
    }
    CloseHandle(h);
    return p;
}

/* ---- the flat (unscrolled) camera setup -------------------------------- */
/* Only the view window matters; same shape coaster3d.c declares. */
typedef struct Config {
    unsigned char  pad00[0x10];
    unsigned short w;           /* +0x10 */
    unsigned short h;           /* +0x12 */
    unsigned char  pad14[0x0c];
    unsigned short x;           /* +0x20 */
    unsigned short y;           /* +0x22 */
} Config;

extern Config* lpConfig;                                        /* 0x004bcbf4 */
extern void MatIdentity(Mat4* out);                             /* 0x004260f0 */
extern void MatMul(const Mat4* a, const Mat4* b, Mat4* out);    /* 0x00426120 */

/* Coaster3D_SetupView (0x00425e20) with the map scroll taken out: the basis
 * is chosen by the argument, its translation column is zeroed instead of
 * being centred on the view window, the eye is the origin, and the
 * world->screen matrix is the basis times a plain identity.  No
 * ScreenToMapRef / GetTileBounds / SetSpanClip. */
// FUNCTION: LEGOLAND 0x00426000
void Coaster3D_SetupFlatView(int use_base)
{
    Mat4 t;

    /* LEVER: the selection must be an if/else over TWO struct assignments,
     * not `g_view_xf = use_base ? g_view_base : g_view_tmpl` and not a
     * `Mat4*` select.  Both pointer forms schedule the rep-movsd count
     * (`mov ecx,0x10`) after the lpConfig load and hoist the zero-extension
     * `xor edx,edx` above the copy: 48/51 either way.  The if/else form is
     * 51/51. */
    if (use_base)
        g_view_xf = g_view_base;
    else
        g_view_xf = g_view_tmpl;
    g_view_xf.m[3] = 0.0f;
    g_view_xf.m[7] = 0.0f;
    g_view_left = lpConfig->x;
    g_view_top = lpConfig->y;
    g_view_right = lpConfig->w + lpConfig->x;
    g_view_bottom = lpConfig->h + lpConfig->y;
    g_eye.x = 0.0f;
    g_eye.y = 0.0f;
    g_eye.z = 0.0f;
    MatIdentity(&t);
    t.m[3] = 0.0f;
    t.m[7] = 0.0f;
    t.m[11] = 0.0f;
    MatMul(&g_view_xf, &t, &g_view_matrix);
}
