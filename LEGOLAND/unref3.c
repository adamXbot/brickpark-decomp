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

/* ---- the debug wire box ------------------------------------------------
 * A 0xcc-byte mesh of 8 vertices and 12 edges: the rectangle
 * (+hw,-hh) (+hw,+hh) (-hw,+hh) (-hw,-hh) at z = 0, copied to z = -depth,
 * and the twelve edges of the resulting prism as index pairs.  Only
 * 0x004267b0 (also dead) draws it. */
typedef struct WireBox {
    short colour;       /* +0x00  the 16-bit pen 0x004267b0 plots with */
    short f02;          /* +0x02 */
    int   nverts;       /* +0x04 */
    int   nedges;       /* +0x08 */
    int   edges[24];    /* +0x0c  12 index pairs */
    Vec3f verts[8];     /* +0x6c */
} WireBox;

// FUNCTION: LEGOLAND 0x00426850
void WireBox_Init(float hw, float hh, float depth, WireBox* b)
{
    int i;

    /* LEVER: the two counts must be the FIRST statements.  With them after
     * the corner stores VC6 sinks them below the zero web and compiles
     * verts[0].x as an fld/fstp pair instead of the original's GPR copy
     * (66/80); first, everything schedules (80/80).  Nine statement orders
     * measured; the next best (z stores first) is 78/80. */
    b->nverts = 8;
    b->nedges = 12;
    b->verts[0].x = hw;
    b->verts[0].y = -hh;
    b->verts[0].z = 0.0f;
    b->verts[1].x = hw;
    b->verts[1].y = hh;
    b->verts[1].z = 0.0f;
    b->verts[2].x = -hw;
    b->verts[2].y = hh;
    /* ORIGINAL BUG: verts[2].z is never written -- the other three corners
     * get their 0.0f and this one keeps whatever was in the buffer.  The
     * copy loop below overwrites verts[6].z anyway, so only verts[2] is
     * left undefined. */
    b->verts[3].x = -hw;
    b->verts[3].y = -hh;
    b->verts[3].z = 0.0f;
    for (i = 0; i < 4; i++) {
        b->verts[i + 4] = b->verts[i];
        b->verts[i + 4].z = -depth;
    }
    b->edges[0] = 0;
    b->edges[1] = 1;
    b->edges[2] = 1;
    b->edges[3] = 2;
    b->edges[4] = 2;
    b->edges[5] = 3;
    b->edges[6] = 3;
    b->edges[7] = 0;
    for (i = 0; i < 4; i++) {
        b->edges[i * 2 + 8] = b->edges[i * 2] + 4;
        b->edges[i * 2 + 9] = b->edges[i * 2 + 1] + 4;
    }
    b->edges[16] = 0;
    b->edges[17] = 4;
    b->edges[18] = 1;
    b->edges[19] = 5;
    /* ORIGINAL BUG: this vertical edge repeats (1,5); it should be (2,6). */
    b->edges[20] = 1;
    b->edges[21] = 5;
    b->edges[22] = 3;
    b->edges[23] = 7;
}

extern void MakeTransform(const Vec3f* pos, const Mat3* rot, Mat4* out); /* 0x004264e0 */
extern void TransformVec3(const Vec3f* src, Vec3f* dst, const Mat4* m,
                          int n);                               /* 0x004261c0 */
/* The 30-step line rasteriser; it interpolates into a local 30-entry
 * ScreenPt array and hands that to Coaster3D_PlotPoints above.  Its colour
 * parameter is 16-bit -- the caller only ever loads `cx`. */
extern void Coaster3D_DrawLine(const ScreenPt* a, const ScreenPt* b, short colour); /* 0x004237f0 */

/* Draw the wire box at `pos` with orientation `rot`: build the 4x4, rotate
 * the 8 corners into world space, project them to the 16-byte screen
 * vertices, then stroke the 12 index pairs. */
// FUNCTION: LEGOLAND 0x004267b0
void WireBox_Draw(const Vec3f* pos, const Mat3* rot, const WireBox* b)
{
    Mat4     m;
    ScreenPt screen[8];
    Vec3f    world[8];
    int      i;

    MakeTransform(pos, rot, &m);
    TransformVec3(b->verts, world, &m, b->nverts);
    Coaster3D_TransformPoints(world, screen, b->nverts);
    for (i = 0; i < b->nedges; i++)
        Coaster3D_DrawLine(&screen[b->edges[i * 2]],
                           &screen[b->edges[i * 2 + 1]], b->colour);
}

/* ---- the debug rail plot ----------------------------------------------
 * A drawable's slot hooks, as coaster3d.c declares them: an array of
 * {position, direction} pairs at +0x4c keyed by a rail slot.  This body only
 * uses the POSITION halves, of slots 1, 0 and 2 -- the centre rail and the
 * two side rails. */
typedef struct DrawObj DrawObj;
typedef struct PosHooks {
    void (*get_pos)(DrawObj* o, float t, Vec3f* out);       /* +0x00 */
    void (*get_dir)(DrawObj* o, float t, Vec3f* out);       /* +0x04 */
} PosHooks;

struct DrawObj {
    unsigned char pad00[0x44];
    float         t0;           /* +0x44 */
    float         t1;           /* +0x48 */
    PosHooks*     hooks;        /* +0x4c */
};

/* 30 parameter steps x 3 rails = 90 samples, and the 16-byte screen vertices
 * they project to. */
extern Vec3f    g_rail_pts[90];         /* 0x006122a0 */
extern ScreenPt g_rail_screen[90];      /* 0x006159c8 */

/* Stroke a piece's three rails as 90 white pixels: sample slot 1, slot 0 and
 * slot 2 at each of 30 evenly spaced parameter values, offset every sample
 * by the caller's origin, project the lot and plot them. */
// FUNCTION: LEGOLAND 0x00428b80
void Coaster3D_PlotPieceRails(DrawObj* o, const Vec3f* origin)
{
    float t;
    float step;
    int   i;
    int   n;

    step = (o->t1 - o->t0) * (1.0f / 30.0f);
    t = o->t0;
    n = 0;
    for (i = 0; i < 30; i++) {
        o->hooks[1].get_pos(o, t, &g_rail_pts[n]);
        g_rail_pts[n].x += origin->x;
        g_rail_pts[n].y += origin->y;
        g_rail_pts[n].z += origin->z;
        n++;
        o->hooks[0].get_pos(o, t, &g_rail_pts[n]);
        g_rail_pts[n].x += origin->x;
        g_rail_pts[n].y += origin->y;
        g_rail_pts[n].z += origin->z;
        n++;
        o->hooks[2].get_pos(o, t, &g_rail_pts[n]);
        g_rail_pts[n].x += origin->x;
        g_rail_pts[n].y += origin->y;
        g_rail_pts[n].z += origin->z;
        n++;
        t += step;
    }
    Coaster3D_TransformPoints(g_rail_pts, g_rail_screen, 90);
    Coaster3D_PlotPoints(g_rail_screen, 90, -1);
}

/* ---- walking the track for a sphere crossing ---------------------------
 * schoolcar8.c's RoutePos: a position along the track as a piece plus a
 * geometry segment plus the world point. */
typedef struct RouteGeom RouteGeom;
struct RouteGeom {
    unsigned char pad00[0x44];
    float         t0;           /* +0x44 */
    float         t1;           /* +0x48 */
};
typedef struct RoutePos {
    void*      node;            /* +0x00 */
    RouteGeom* geom;            /* +0x04 */
    Vec3f      pos;             /* +0x08 */
} RoutePos;                     /* 0x14 */

/* The probe's parameter block.  0x00429cf0 -- the distance function the
 * bracketing root finder is handed -- reads the cursor at 0x00615f84 and the
 * centre at 0x00615f8c, and the three squared radii are its comparison
 * thresholds. */
extern RoutePos*   g_probe_pos;         /* 0x00615f84 */
extern const Vec3f* g_probe_centre;     /* 0x00615f8c */
extern float g_probe_radius;            /* 0x00615fd0 */
extern float g_probe_band;              /* 0x00615fd4 */
extern float g_probe_r2;                /* 0x00615fd8 */
extern float g_probe_outer2;            /* 0x00615fdc */
extern float g_probe_inner2;            /* 0x00615fe0 */

extern void  TrackCursor_AdvanceGeometry(RoutePos* p);          /* 0x0041f850 */
extern float TrackProbe_Distance(float t);                      /* 0x00429cf0 */
/* 0x00429e20, reached through the module's own hook slot: bisect `f` between
 * a and b and report whether it changed sign there. */
typedef int (*FindBracketFn)(float (*f)(float), float a, float b, float* out);
extern FindBracketFn g_find_bracket;                            /* 0x004b63fc */

/* =========================================================================
 * 0x0042a020 -- walk forward from `start`, one geometry segment at a time,
 * until the probe's distance function crosses zero; the first segment is
 * entered at `from` rather than at its own t0, and the cursor that found the
 * crossing is copied out.  The seven module globals are the probe's
 * parameter block: 0x00429cf0 reads the cursor and the centre out of them,
 * and the three squared radii are its thresholds.
 *
 * THREE LEVERS, all measured on this body:
 *  * `float f0 = from;` -- a named float local for the parameter, assigned
 *    before the global stores and passed to the first call.  The local is
 *    COALESCED onto the parameter's own slot, so it costs no instruction,
 *    but it turns the argument into an x87-delivered one: the original's
 *    `push ecx` reserve plus `fld from / fstp [esp]`.  Passing `from`
 *    directly gives `mov ecx,[from] / push ecx` and the body comes out one
 *    instruction SHORT (52 of 69) -- and every one of the 24 global-store
 *    orders and the four `(float)`/`*(float*)&` spellings stays at 69.
 *    `*(volatile float*)&from` also buys the third instruction (60 of 70)
 *    but pins the schedule: a volatile access cannot cross a store, so the
 *    two argument pushes and the g_probe_centre store stay on the wrong
 *    side of it.
 *  * The global stores must be r2, centre, radius, band, pos, outer2,
 *    inner2.  VC6 SWAPS the adjacent centre/radius pair, which is why the
 *    original's emission order (radius first, centre after the pushes) is
 *    the reverse of the source's; writing them in emission order costs 2.
 *  * The first `= geom->t1` must be assigned to the SAME local the LOOP
 *    assigns `geom->t1` to, not the one it assigns `geom->t0` to.  The two
 *    float locals are homed in the dead `radius` and `start` argument
 *    slots, and sharing the t1 variable is what puts t0 at +8 and t1 at
 *    +0x0c the way the original does (97.1% -> 100%).
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x0042a020
void Track_FindSphereCrossing(const Vec3f* centre, float radius,
                              const RoutePos* start, float from, float band,
                              RoutePos* out, float* hit)
{
    RoutePos cur = *start;
    float    ta;
    float    tb;
    float    f0;
    int      found;

    tb = cur.geom->t1;
    f0 = from;
    g_probe_r2 = radius * radius;
    g_probe_centre = centre;
    g_probe_radius = radius;
    g_probe_band = band;
    g_probe_pos = &cur;
    g_probe_outer2 = (radius + band) * (radius + band);
    g_probe_inner2 = (radius - band) * (radius - band);
    found = g_find_bracket(TrackProbe_Distance, f0, tb, hit);
    while (!found) {
        TrackCursor_AdvanceGeometry(&cur);
        ta = cur.geom->t0;
        tb = cur.geom->t1;
        found = g_find_bracket(TrackProbe_Distance, ta, tb, hit);
    }
    *out = cur;
}
