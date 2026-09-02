/* LEGOLAND -- the scaled/tiled blit worker, the Z-buffer RLE painter, the
 * recolouring software blitter and the build/destroy cursor renderer.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field offsets, global addresses and callee arg counts are load-bearing;
 * names are ours. No legoland.h: every type is defined locally.
 *
 * ---------------------------------------------------------------------------
 * SoftPrint statics (0x007fe998 .. 0x007febb0) as this file sees them
 * ---------------------------------------------------------------------------
 *   0x007fe998  g_sp_recolour     16-bit AND mask applied to every pixel
 *   0x007fe9a4  g_sp_rowlen       row length scratch (text.c SoftPrint_Clear)
 *   0x007fe9a8  g_sp_mouse_pixel  address of the surface pixel under the mouse
 *   0x007fea10  g_sp_rows_left    rows still to paint (Z-buffer loop)
 *   0x007fea14  g_sp_height       source height
 *   0x007fea1c  g_sp_width        source pitch in bytes
 *   0x007fea20  g_sp_pal16        8-bit -> 16-bit palette lookup (pal + 4)
 *   0x007fea40  g_sp_pixels       source bitmap
 *   0x007fea44  g_transparent_colour
 *   0x007fea4c  g_sp_top          source rect top
 *   0x007fea50  g_sp_left         source rect left
 *   0x007feb18  g_zb_row          current Z-buffer row
 *   0x007febac  g_sp_h            source rect height
 *   0x007febb0  g_sp_w            source rect width
 *   0x00668160  g_zb_bits         bits left in the RLE control word
 *
 * The scaled-blit worker keeps a lazily created system-memory surface
 * (0x0079861c, created flag 0x00798620) the size of the primary; it draws the
 * sprite into that with the software blitter and lets DirectDraw stretch it
 * onto the draw surface. The lock descriptor and render clip are parked in
 * 0x00798598 / 0x00798608 around the redirect.
 * ------------------------------------------------------------------------- */

__declspec(noreturn) void exit(int);
void* memset(void*, int, unsigned int);

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

/* A footprint rect list node (legoland.h Rect). */
typedef struct Rect {
    int          left;    /* +0x00 */
    int          top;     /* +0x04 */
    int          right;   /* +0x08 */
    int          bottom;  /* +0x0c */
    struct Rect* next;    /* +0x10 */
} Rect;

/* DDSURFACEDESC, 0x6c bytes. */
typedef struct DDSurfaceDesc {
    unsigned long dwSize;              /* +0x00 */
    unsigned long dwFlags;             /* +0x04 */
    unsigned long dwHeight;            /* +0x08 */
    unsigned long dwWidth;             /* +0x0c */
    long          lPitch;              /* +0x10 */
    unsigned long dwBackBufferCount;   /* +0x14 */
    unsigned long dwMipMapCount;       /* +0x18 */
    unsigned long dwAlphaBitDepth;     /* +0x1c */
    unsigned long dwReserved;          /* +0x20 */
    void*         lpSurface;           /* +0x24 */
    char          pad28[0x68 - 0x28];  /* +0x28 */
    unsigned long ddsCaps;             /* +0x68 */
} DDSurfaceDesc;

typedef struct DDColorKey {
    unsigned long low;     /* +0x00 */
    unsigned long high;    /* +0x04 */
} DDColorKey;

typedef struct DDSurface DDSurface;
typedef struct DDClipper DDClipper;
typedef struct DDraw2    DDraw2;

/* IDirectDrawSurface vtable slots used here. */
typedef struct DDSurfaceVtbl {
    char pad00[0x14];
    long(__stdcall* Blt)(DDSurface*, WinRect*, DDSurface*, WinRect*,
                         unsigned long, void*);                               /* +0x14 */
    char pad18[0x58 - 0x18];
    long(__stdcall* GetSurfaceDesc)(DDSurface*, DDSurfaceDesc*);              /* +0x58 */
    char pad5c[0x60 - 0x5c];
    long(__stdcall* IsLost)(DDSurface*);                                      /* +0x60 */
    long(__stdcall* Lock)(DDSurface*, WinRect*, DDSurfaceDesc*,
                          unsigned long, void*);                              /* +0x64 */
    char pad68[0x6c - 0x68];
    long(__stdcall* Restore)(DDSurface*);                                     /* +0x6c */
    long(__stdcall* SetClipper)(DDSurface*, DDClipper*);                      /* +0x70 */
    long(__stdcall* SetColorKey)(DDSurface*, unsigned long, DDColorKey*);     /* +0x74 */
    char pad78[0x80 - 0x78];
    long(__stdcall* Unlock)(DDSurface*, void*);                               /* +0x80 */
} DDSurfaceVtbl;
struct DDSurface { DDSurfaceVtbl* vtbl; };

/* IDirectDraw2 vtable: CreateSurface is slot 0x18. */
typedef struct DDraw2Vtbl {
    char pad00[0x18];
    long(__stdcall* CreateSurface)(DDraw2*, DDSurfaceDesc*, DDSurface**, void*); /* +0x18 */
} DDraw2Vtbl;
struct DDraw2 { DDraw2Vtbl* vtbl; };

/* sprite2.c's SpriteRec. */
typedef struct SpriteRec {
    struct SpriteRec* next;    /* +0x00 */
    DDSurface*        surface; /* +0x04 */
    void*             image;   /* +0x08 ImageRec* (or draw callback, flag 0x20) */
    int               detail;  /* +0x0c */
    unsigned int      flags;   /* +0x10 0x20 = function-drawn */
    short             w;       /* +0x14 */
    short             h;       /* +0x16 */
    short             src_x;   /* +0x18 */
    short             src_y;   /* +0x1a */
    unsigned short    refs;    /* +0x1c */
    short             pad1e;   /* +0x1e */
} SpriteRec;

/* sprite2.c's ImageRec (0x18-byte header). type: 0 = 8-bit paletted,
 * 1 = 16-bit raw, 2/3 = animated LLS (3 = the RLE frame list). */
typedef struct ImageRec {
    void*          lls;        /* +0x00 pixels, or the LLS record */
    void*          pal;        /* +0x04 */
    short          w;          /* +0x08 */
    short          h;          /* +0x0a */
    unsigned short refcount;   /* +0x0c */
    char           kind;       /* +0x0e */
    char           pad0f;      /* +0x0f */
    char*          name;       /* +0x10 */
    int            type;       /* +0x14 */
} ImageRec;

/* printlist.c's SpriteHandle, filled by GetSprite. */
typedef struct SpriteHandle {
    int        pitch;           /* +0x00 */
    int        w;               /* +0x04 */
    int        h;               /* +0x08 */
    void*      pixels;          /* +0x0c */
    DDSurface* surface;         /* +0x10 */
    int        bpp;             /* +0x14 */
} SpriteHandle;

/* An LLS animation record: +0x00 current frame, +0x10 frame count, +0x14
 * flags (bit 0 = frame list carries a base image first), +0x18 the frame
 * list. Each frame is {+0 byte length (link to the next), +4 count of 16-bit
 * entries, +8 byte length of the second block, +0x10 data}. */
typedef struct LLSRec {
    short          frame;       /* +0x00 */
    char           pad02[0x0e]; /* +0x02 */
    short          nframes;     /* +0x10 */
    short          pad12;       /* +0x12 */
    unsigned int   flags;       /* +0x14 */
    char           frames[1];   /* +0x18 */
} LLSRec;

typedef struct LLSFrame {
    int  size;     /* +0x00 */
    int  n16;      /* +0x04 */
    int  n2;       /* +0x08 */
    int  pad0c;    /* +0x0c */
    char data[1];  /* +0x10 */
} LLSFrame;

/* surface.c's VideoSurfaceInfo (GetVideoSurface). */
typedef struct VideoSurfaceInfo {
    long  pitch;    /* +0x00 */
    int   width;    /* +0x04 */
    int   height;   /* +0x08 */
    void* bits;     /* +0x0c */
    int   unused;   /* +0x10 not written by GetVideoSurface */
    int   format;   /* +0x14 */
} VideoSurfaceInfo;

/* pathbuild.c's TileBounds (GetTileBounds). */
typedef struct TileBounds {
    int left;   /* +0x00 */
    int top;    /* +0x04 */
    int right;  /* +0x08 */
    int bottom; /* +0x0c */
} TileBounds;

/* The map header at 0x004bcbf4 as the cursor renderer reads it. */
typedef struct MapHdr {
    char           pad00[0x10];
    unsigned short view_w;      /* +0x10 */
    unsigned short view_h;      /* +0x12 */
    unsigned short width;       /* +0x14 cells */
    unsigned short height;      /* +0x16 */
    unsigned short tile_h;      /* +0x18 */
    char           pad1a[0x20 - 0x1a];
    unsigned short origin_x;    /* +0x20 */
    unsigned short origin_y;    /* +0x22 */
} MapHdr;

/* legoland.h's Cell (20 bytes); only the flags word is read here. */
typedef struct Cell {
    char           pad0[0x0c];
    unsigned short flags;       /* +0x0c */
    char           pad0e[0x14 - 0x0e];
} Cell;

/* The edit / destroy cursor block (objmap2.c's Cursor, 0x1834 bytes). */
typedef struct Cursor {
    unsigned short count;        /* +0x0000 outline points used */
    short          px[0x400];    /* +0x0002 */
    short          py[0x400];    /* +0x0802 */
    unsigned char  kind[0x400];  /* +0x1002 bits 0-1 segment style, 0xc = ? */
    unsigned char  pad1402[2];
    Pos            origin;       /* +0x1404 map cell the footprint hangs off */
    int            status;       /* +0x140c */
    int            error;        /* +0x1410 */
    Rect           rect;         /* +0x1414 footprint rect list */
    unsigned char  style;        /* +0x1428 */
    char           pad1429[0x1828 - 0x1429];
    unsigned int   flags;        /* +0x1828 */
    int            f182c;        /* +0x182c */
    struct Cursor* next;         /* +0x1830 chained cursor */
} Cursor;

/* An object definition as the cursor renderer reads it: entrance offset at
 * +0x0c/+0x10, exit offset (bytes) at +0x24/+0x25. */
typedef struct ObjDefRec {
    char        pad00[0x0c];
    int         ent_x;           /* +0x0c */
    int         ent_y;           /* +0x10 */
    char        pad14[0x24 - 0x14];
    signed char exit_x;          /* +0x24 */
    signed char exit_y;          /* +0x25 */
} ObjDefRec;

/* The lock descriptor and the render clip are ONE object (gpu.c). */
typedef struct LockState {
    DDSurfaceDesc ddsd;         /* +0x00 0x0066809c */
    WinRect       render_clip;  /* +0x6c 0x00668108 */
} LockState;

/* ---- globals ------------------------------------------------------------ */

extern LockState      g_lock;               /* 0x0066809c */
#define g_ddsd        g_lock.ddsd
#define g_render_clip g_lock.render_clip
extern DDraw2*        g_ddraw;              /* 0x00667d74 */
extern DDSurface*     g_primary;            /* 0x00668070 */
extern DDSurface*     g_draw_surface;       /* 0x0066807c */
extern DDClipper*     g_clipper;            /* 0x00668080 */
extern WinRect        g_clip_rect;          /* 0x004bdea0 */
extern MapHdr*        g_map;                /* 0x004bcbf4 */
extern Cell**         g_map_rows;           /* 0x00801400 */
extern SpriteRec*     g_tile_sprites[];     /* 0x00805f60 */
extern int            g_frame_override;     /* 0x004b9ca8 */

extern DDSurfaceDesc  g_scaled_saved_ddsd;  /* 0x00798598 */
extern WinRect        g_scaled_saved_clip;  /* 0x00798608 */
extern DDSurface*     g_scaled_surface;     /* 0x0079861c */
extern int            g_scaled_created;     /* 0x00798620 */

extern int            g_sp_recolour;        /* 0x007fe998 */
extern int            g_sp_rowlen;          /* 0x007fe9a4 */
extern void*          g_sp_mouse_pixel;     /* 0x007fe9a8 */
extern int            g_sp_rows_left;       /* 0x007fea10 */
extern int            g_sp_height;          /* 0x007fea14 */
extern int            g_sp_width;           /* 0x007fea1c */
extern void*          g_sp_pal16;           /* 0x007fea20 */
extern void*          g_sp_pixels;          /* 0x007fea40 */
extern int            g_transparent_colour; /* 0x007fea44 */
extern int            g_sp_top;             /* 0x007fea4c */
extern int            g_sp_left;            /* 0x007fea50 */
extern void*          g_zb_row;             /* 0x007feb18 */
extern int            g_sp_h;               /* 0x007febac */
extern int            g_sp_w;               /* 0x007febb0 */
extern int            g_zb_bits;            /* 0x00668160 */

/* The input/cursor block at 0x00813a40: the mouse point at +0x04. */
extern Pos            g_mouse_point;        /* 0x00813a44 */

extern int*           g_basic_tiles_data;   /* 0x00801a6c -> "BASIC TILES 1" desc, +0 = base slot */
extern int            g_tileset_id0;        /* 0x008003f8 */
extern int            g_tileset_id1;        /* 0x00801b20 */
extern int            g_tileset_id2;        /* 0x0080ff60 */
extern int            g_tileset_id3;        /* 0x00805f48 */
extern int            g_default_tile;       /* 0x00667ca4 */
extern SpriteRec*     g_arrow1;             /* 0x00667c8c */
extern SpriteRec*     g_arrow2;             /* 0x00667c88 */
extern SpriteRec*     g_arrow3;             /* 0x00667c94 */
extern SpriteRec*     g_arrow4;             /* 0x00667c90 */
extern int            g_edit_changed;       /* 0x008119b0 */
extern ObjDefRec*     g_edit_object;        /* 0x008119b8 */

extern const char     g_cursor_col_a[];     /* 0x004b95cc */
extern const char     g_cursor_col_b[];     /* 0x004b95c4 */
extern const char     g_cursor_col_c[];     /* 0x004b95d4 */
extern const char     g_cursor_col_d[];     /* 0x004b95dc */

extern const char     g_msg_neg_src[];      /* 0x004bdd74 */
extern const char     g_msg_bad_image[];    /* 0x004b9d0c */

/* ---- externals ---------------------------------------------------------- */

__declspec(dllimport) int __stdcall IntersectRect(WinRect* dst, const WinRect* a,
                                                  const WinRect* b);  /* [0x4ab2a0] */
__declspec(dllimport) int __stdcall IsBadReadPtr(const void* p, unsigned int n); /* [0x4ab11c] */

extern int   GetTransparentColour(void);                  /* 0x0044e690 */
extern int   GetNearestColour(int r, int g, int b);       /* 0x0044e6c0 */
extern void  SoftPrint_Clear(void);                       /* 0x004651d0 */
extern void  __fastcall SoftBlitSprite(SpriteRec* s, WinRect* src, Pos* at); /* 0x00464ee0 */
extern int   MakeSprite(SpriteRec* s);                    /* 0x00497b70 */
extern int   GetSprite(SpriteHandle* out, SpriteRec* s);  /* 0x00497c30 */
extern int   ReleaseSprite(SpriteHandle* h);              /* 0x00497dc0 */
extern int   printf(const char* fmt, ...);                /* 0x0049e5c5 */
extern void  DebugPrintf(const char* fmt, ...);           /* 0x0047f870 */
extern void  SoftBlitAnim(void* lls, WinRect* src, WinRect* dst);     /* 0x00465240 */
extern void  SoftBlitRLE(void* lls, WinRect* src, WinRect* dst);      /* 0x00465ee0 */
/* 0x00468410: paint one RLE frame recoloured. */
extern void  SoftBlitRLEFrame(void* dst, void* ctrl, void* p16, void* p8,
                              int h, int pitch, int top, int left, int w,
                              int zero, void* mouse);
extern void  GetClipping(WinRect* out);                   /* 0x0048a630 */
extern void  SetClipping(WinRect* r);                     /* 0x0048a5c0 */
extern void  GetTileBounds(Pos* tile, TileBounds* out);   /* 0x0045acc0 */
extern int   GetVideoSurface(VideoSurfaceInfo* out);      /* 0x00464310 */
extern int   PrintSprite(SpriteRec* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern void  DrawCursorSegmentA(VideoSurfaceInfo* vs, int kind, int x, int y,
                                const char* col, int h);  /* 0x0045fca0 */
extern void  DrawCursorSegmentB(VideoSurfaceInfo* vs, int kind, int x, int y,
                                const char* col, int h);  /* 0x0045fad0 */
extern int   ObjHasEntrance(ObjDefRec* o);                /* 0x0045e620 */
extern int   GetObjEntranceDir(ObjDefRec* o);             /* 0x0045e6b0 */
extern int   ObjHasExit(ObjDefRec* o);                    /* 0x0045e690 */
extern int   GetObjExitDir(ObjDefRec* o);                 /* 0x0045e710 */

#define DDERR_SURFACELOST   0x887601c2

/* -------------------------------------------------------------------------
 * 0x00488c50 -- the exported tiled blit. In the shipped build this is a
 * stub: it builds the destination rect and calls exit(1) -- tiling was never
 * finished (PrintTiledSprite still routes here). The rect stores survive
 * because the rect is address-taken by the unreachable code after the exit.
 * No `ret` follows the noreturn call, so tools/audit.py cannot bound the
 * function (its extent walk runs on into 0x00488c80); matchfull scores the
 * 13-instruction body exact.
 * ------------------------------------------------------------------------- */

// WIP-FUNCTION: LEGOLAND 0x00488c50  (100% by matchfull; body ends in a noreturn exit(1) with no ret, audit.py cannot bound it)
int RenderTiledSprite(SpriteRec* s, int x, int y, int w, int h, int e, int f)
{
    WinRect rc;

    rc.left = x;
    rc.top = y;
    rc.right = x + w;
    rc.bottom = y + h;
    exit(1);
    /* unreachable: the original's tiling body was stubbed out */
    return IntersectRect(&rc, &rc, &g_clip_rect);
}

/* -------------------------------------------------------------------------
 * 0x00488c80 -- the scaled blit worker (RenderScaledSprite's body): draw the
 * sprite into a system-memory scratch surface with the software blitter,
 * then let DirectDraw stretch it to {x+ox, y+oy, x+ox+w+1, y+oy+h+1} with a
 * source colour key. The scratch surface is created once, the size of the
 * primary (dwFlags CAPS|HEIGHT|WIDTH|PIXELFORMAT, 1280x960, system memory
 * offscreen plain), keyed on the transparent colour. Returns 1 when the Blt
 * succeeded, 0 when it failed (after one Restore retry).
 * ------------------------------------------------------------------------- */

// WIP-FUNCTION: LEGOLAND 0x00488c80
int RenderSpriteScaledOffset(SpriteRec* s, int x, int y, int w, int h, Pos* off)
{
    struct {
        DDColorKey    ck;     /* +0x00 also the software blit origin */
        WinRect       src;    /* +0x08 */
        WinRect       dst;    /* +0x18 */
        WinRect       full;   /* +0x28 */
        WinRect       src2;   /* +0x38 */
        DDSurfaceDesc ddsd;   /* +0x48 */
        DDSurfaceDesc lock;   /* +0xb4 */
    } F;
    long          rc;

    F.dst.left = off->x + x;
    F.dst.top = off->y + y;
    F.dst.right = off->x + x + w + 1;     /* re-spelled from the params: keeps x in a scratch reg */
    F.dst.bottom = off->y + y + h + 1;
    F.src.top = 0;
    F.src.left = 0;
    F.src.right = s->w;
    F.src.bottom = s->h;
    if (g_scaled_created == 0) {
        memset(&F.ddsd, 0, sizeof(F.ddsd));
        g_scaled_created = 1;
        F.ddsd.dwSize = sizeof(F.ddsd);
        if (g_draw_surface->vtbl->GetSurfaceDesc(g_draw_surface, &F.ddsd) == 0) {
            F.ddsd.dwFlags = 0x1007;
            F.ddsd.ddsCaps = 0x840;
            F.ddsd.dwWidth = 1280;
            F.ddsd.dwHeight = 960;
            g_ddraw->vtbl->CreateSurface(g_ddraw, &F.ddsd, &g_scaled_surface, 0);
            F.ck.high = GetTransparentColour();
            F.ck.low = F.ck.high;
            g_scaled_surface->vtbl->SetColorKey(g_scaled_surface, 8, &F.ck);
        }
    }
    g_scaled_saved_ddsd = g_ddsd;
    g_scaled_saved_clip = g_render_clip;
    memset(&F.lock, 0, sizeof(F.lock));
    F.lock.dwSize = sizeof(F.lock);
    if (g_scaled_surface->vtbl->Lock(g_scaled_surface, 0, &F.lock, 0x21, 0) == 0) {
        F.full.left = 0;
        F.full.right = F.lock.dwWidth;
        F.full.top = 0;
        F.full.bottom = F.lock.dwHeight;
        IntersectRect(&g_render_clip, &F.full, &g_clip_rect);
        F.ck.low = 0;
        F.ck.high = 0;
        g_transparent_colour = GetTransparentColour();
        g_ddsd.lpSurface = F.lock.lpSurface;
        g_ddsd.dwWidth = s->w;
        g_ddsd.dwHeight = s->h;
        g_ddsd.lPitch = F.lock.lPitch;
        SoftPrint_Clear();
        F.src2.left = 0;
        F.src2.right = s->w;
        F.src2.top = 0;
        F.src2.bottom = s->h;
        if (s->src_x < 0 || s->src_y < 0)
            printf(g_msg_neg_src);
        SoftBlitSprite(s, &F.src2, (Pos*)&F.ck);
        g_scaled_surface->vtbl->Unlock(g_scaled_surface, F.lock.lpSurface);
    }
    g_draw_surface->vtbl->SetClipper(g_draw_surface, g_clipper);
    rc = g_draw_surface->vtbl->Blt(g_draw_surface, &F.dst, g_scaled_surface, &F.src, 0x1008000, 0);
    if (rc != 0) {
        if (rc != DDERR_SURFACELOST)
            goto fail;
        if (g_scaled_surface->vtbl->Restore(g_scaled_surface) != 0) {
            g_draw_surface->vtbl->SetClipper(g_draw_surface, 0);
            g_ddsd = g_scaled_saved_ddsd;
            g_render_clip = g_scaled_saved_clip;
            return 0;
        }
        MakeSprite(s);
        if (g_primary->vtbl->IsLost(g_primary) == DDERR_SURFACELOST) {
            if (g_primary->vtbl->Restore(g_primary) != 0)
                goto fail;
        }
        if (g_draw_surface->vtbl->Blt(g_draw_surface, &F.dst, g_scaled_surface, &F.src, 0x8000, 0) != 0)
            goto fail;
    }
    g_draw_surface->vtbl->SetClipper(g_draw_surface, 0);
    g_ddsd = g_scaled_saved_ddsd;
    g_render_clip = g_scaled_saved_clip;
    return 1;
fail:
    g_draw_surface->vtbl->SetClipper(g_draw_surface, 0);
    g_ddsd = g_scaled_saved_ddsd;
    g_render_clip = g_scaled_saved_clip;
    return 0;
}

/* -------------------------------------------------------------------------
 * 0x00464a90 -- paint one Z-buffer sprite frame into the 128-dword-wide Z
 * buffer (0x200 bytes per row, one dword per pixel, the Z value in the top
 * byte). The frame is picked like the blitter does (override, else the
 * LLS's own, clamped to the last), then the frame list is walked. Frame 0
 * carries a 0x200-byte table before its RLE data (hence +0x208), later
 * frames do not (+8). The RLE control stream is read four bits at a time
 * from `p`; `rows` is the literal byte stream. The first loop skips the rows
 * above src->top without painting, the second paints src height rows
 * clipped to src->left / src width.
 * ------------------------------------------------------------------------- */

// WIP-FUNCTION: LEGOLAND 0x00464a90  (100% by matchfull: 303/303 insns, 1099/1099 bytes identical; audit.py's norm() cannot normalise the rel8 target of `loop` (0x464ddf/0x464df4 are byte-identical e2 f2 / e2 f6) so it reports 2 mismatches)
void ZBufferHelper(char* lls, WinRect* src, Pos* dst, void* zbuf)
{
    char* rows;
    char* p;
    int   frame;
    int   nframes;
    int   n;

    g_sp_rowlen = 0x200;
    frame = g_frame_override;
    if (frame < 0)
        frame = *(short*)lls;
    nframes = *(short*)(lls + 0x10);
    if (frame >= nframes)
        frame = nframes - 1;
    p = lls + 0x18;
    n = frame;
    while (n--)                     /* post-decrement form: VC6 copies the count with `lea edx,[ecx]` */
        p += *(int*)p;
    n = *(int*)(p + 4);
    rows = p + 8;
    if (frame == 0)
        lls = p + n + 0x208;
    else
        lls = p + n + 8;

    __asm {
        pushad
        mov     eax, dst
        mov     edi, zbuf
        mov     ecx, [eax]
        shl     ecx, 2
        add     edi, ecx
        mov     ecx, [eax+4]
        imul    ecx, ecx, 200h
        add     edi, ecx
        mov     g_zb_row, edi
        mov     eax, src
        mov     ecx, [eax]
        mov     g_sp_left, ecx
        mov     ebx, [eax+8]
        sub     ebx, ecx
        mov     g_sp_w, ebx
        mov     ecx, [eax+4]
        mov     g_sp_top, ecx
        mov     edx, ecx
        mov     ebx, [eax+0ch]
        sub     ebx, ecx
        mov     g_sp_h, ebx
        mov     esi, rows
        mov     ebp, lls
        xor     eax, eax
        mov     g_zb_bits, eax
        and     edx, edx
        je      rows_start
    skip_loop:
        shr     ebx, 2
        and     g_zb_bits, 0fh
        jne     s1
        mov     ebx, [ebp]
        add     ebp, 4
        mov     g_zb_bits, 10h
    s1:
        dec     g_zb_bits
        test    ebx, 2
        je      s_inc
        test    ebx, 1
        je      skip_loop
        shr     ebx, 2
        cmp     g_zb_bits, 4
        jae     s2
        mov     ebx, [ebp]
        add     ebp, 4
        mov     g_zb_bits, 10h
    s2:
        shrd    ecx, ebx, 8
        shr     ebx, 6
        sub     g_zb_bits, 4
        shr     ecx, 18h
        je      s_next_row
        shr     ebx, 2
        and     g_zb_bits, 0fh
        jne     s3
        mov     ebx, [ebp]
        add     ebp, 4
        mov     g_zb_bits, 10h
    s3:
        dec     g_zb_bits
        test    ebx, 2
        je      s4
        jmp     skip_loop
    s4:
        test    ebx, 1
        je      s5
    s_inc:
        inc     esi
        jmp     skip_loop
    s5:
        lea     esi, [esi+ecx]
        jmp     skip_loop
    s_next_row:
        dec     edx
        jne     skip_loop
    rows_start:
        mov     edx, g_sp_h
        and     edx, edx
        je      done
    row_loop:
        mov     g_sp_rows_left, edx
        mov     edx, g_sp_left
        and     edx, edx
        je      r_nocol
    c_loop:
        shr     ebx, 2
        and     g_zb_bits, 0fh
        jne     c1
        mov     ebx, [ebp]
        add     ebp, 4
        mov     g_zb_bits, 10h
    c1:
        dec     g_zb_bits
        test    ebx, 2
        je      c_inc
        test    ebx, 1
        je      c_dec
        shr     ebx, 2
        cmp     g_zb_bits, 4
        jae     c2
        mov     ebx, [ebp]
        add     ebp, 4
        mov     g_zb_bits, 10h
    c2:
        shrd    ecx, ebx, 8
        shr     ebx, 6
        sub     g_zb_bits, 4
        shr     ecx, 18h
        je      row_end
        shr     ebx, 2
        and     g_zb_bits, 0fh
        jne     c3
        mov     ebx, [ebp]
        add     ebp, 4
        mov     g_zb_bits, 10h
    c3:
        dec     g_zb_bits
        test    ebx, 2
        je      c4
        sub     edx, ecx
        jns     c_loop
        neg     edx
        lea     edi, [edi+edx*2]
        mov     ecx, g_sp_w
        sub     ecx, edx
        js      tail_skip
        mov     edx, ecx
        jmp     draw_loop
    c4:
        test    ebx, 1
        je      c5
        inc     esi
        sub     edx, ecx
        jns     c_loop
        neg     edx
        dec     esi
        mov     ecx, edx
        mov     edx, g_sp_w
        and     edx, edx
        je      tail_skip
        jmp     d_run
    c_inc:
        inc     esi
    c_dec:
        dec     edx
        jne     c_loop
        mov     edx, g_sp_w
        jmp     draw_loop
    c5:
        lea     esi, [esi+ecx]
        sub     edx, ecx
        jns     c_loop
        lea     esi, [esi+edx]
        neg     edx
        mov     ecx, edx
        mov     edx, g_sp_w
        and     edx, edx
        je      tail_skip
        jmp     d_lit
    r_nocol:
        mov     edx, g_sp_w
    draw_loop:
        mov     eax, g_zb_bits
        shr     ebx, 2
        dec     eax
        jns     d1
        mov     ebx, [ebp]
        add     ebp, 4
    d1:
        and     eax, 0fh
        test    ebx, 2
        mov     g_zb_bits, eax
        je      d_one
        test    ebx, 1
        je      d_skip1
        shr     ebx, 2
        sub     eax, 4
        jns     d2
        mov     ebx, [ebp]
        add     ebp, 4
        mov     eax, 0ch
    d2:
        movzx   ecx, bl
        shr     ebx, 6
        and     eax, 0fh
        test    ecx, ecx
        mov     g_zb_bits, eax
        je      row_end
        shr     ebx, 2
        dec     eax
        jns     d3
        mov     ebx, [ebp]
        add     ebp, 4
    d3:
        and     eax, 0fh
        mov     g_zb_bits, eax
        test    ebx, 2
        je      d_run
        sub     edx, ecx
        js      tail_skip
        lea     edi, [edi+ecx*4]
        jmp     draw_loop
    d_run:
        test    ebx, 1
        je      d_lit
        movzx   eax, byte ptr [esi]
        shl     eax, 18h
        inc     esi
        cmp     edx, ecx
        ja      d_run2
        mov     ecx, edx
        rep stosd
        jmp     tail_skip
    d_run2:
        sub     edx, ecx
        rep stosd
        jmp     draw_loop
    d_one:
        mov     ecx, 1
    d_lit:
        cmp     edx, ecx
        ja      d_lit2
        xchg    ecx, edx          ; 87 ca: MASM's reg/rm order is reversed from capstone's
        sub     edx, ecx
    d_lit_loop:
        and     ecx, ecx
        je      d_lit_end
        movzx   eax, byte ptr [esi]
        shl     eax, 18h
        inc     esi
        stosd
        loop    d_lit_loop
    d_lit_end:
        lea     esi, [esi+edx]
        jmp     tail_skip
    d_lit2:
        sub     edx, ecx
    d_lit2_loop:
        movzx   eax, byte ptr [esi]
        shl     eax, 18h
        inc     esi
        stosd
        loop    d_lit2_loop
        jmp     draw_loop
    d_skip1:
        dec     edx
        je      tail_skip
        add     edi, 4
        jmp     draw_loop
    tail_skip:
        shr     ebx, 2
        and     g_zb_bits, 0fh
        jne     t1
        mov     ebx, [ebp]
        add     ebp, 4
        mov     g_zb_bits, 10h
    t1:
        dec     g_zb_bits
        test    ebx, 2
        je      t_inc
        test    ebx, 1
        je      tail_skip
        shr     ebx, 2
        cmp     g_zb_bits, 4
        jae     t2
        mov     ebx, [ebp]
        add     ebp, 4
        mov     g_zb_bits, 10h
    t2:
        shrd    ecx, ebx, 8
        shr     ebx, 6
        sub     g_zb_bits, 4
        shr     ecx, 18h
        je      row_end
        shr     ebx, 2
        and     g_zb_bits, 0fh
        jne     t3
        mov     ebx, [ebp]
        add     ebp, 4
        mov     g_zb_bits, 10h
    t3:
        dec     g_zb_bits
        test    ebx, 2
        je      t4
        jmp     tail_skip
    t4:
        test    ebx, 1
        je      t5
    t_inc:
        inc     esi
        jmp     tail_skip
    t5:
        lea     esi, [esi+ecx]
        jmp     tail_skip
    row_end:
        mov     edi, g_zb_row
        mov     edx, g_sp_rows_left
        add     edi, g_sp_rowlen
        dec     edx
        mov     g_zb_row, edi
        jne     row_loop
    done:
        popad
    }
}

/* -------------------------------------------------------------------------
 * 0x00465a40 -- the recolouring software blitter behind RenderSpriteX.
 * `src` is in sprite space and is translated by the sprite's sub-rect
 * origin; `dst` is the clipped screen rect; `colour` is an RGB triple whose
 * nearest palette entry becomes the AND mask every drawn pixel is combined
 * with (0xff000000 on an RLE image selects the "highlight" pass instead).
 * Function-drawn sprites (flag 0x20) are locked through GetSprite and
 * described by a synthetic 16-bit ImageRec; the lock is released at the end
 * of the two pixel-loop paths only.
 * ------------------------------------------------------------------------- */

static __inline void DrawRLEFrame(void* dst, LLSFrame* f)
{
    SoftBlitRLEFrame(dst, f->data, f->data + f->n16 * 2,
                     f->data + f->n16 * 2 + f->n2, g_sp_h, g_ddsd.lPitch,
                     g_sp_top, g_sp_left, g_sp_w, 0, g_sp_mouse_pixel);
}

// WIP-FUNCTION: LEGOLAND 0x00465a40
void SoftPrint_XBltFast(SpriteRec* s, WinRect* src, WinRect* dst, int colour)
{
    SpriteHandle handle;
    ImageRec     fake;
    ImageRec*    image;
    LLSRec*      lls;
    LLSFrame*    f;
    int          frame;
    int          nframes;
    int          n;
    unsigned short* row;

    if (s->flags & 0x20) {
        GetSprite(&handle, s);
        fake.lls = handle.pixels;
        fake.w = (short)handle.pitch / 2;
        fake.h = (short)handle.h;
        fake.type = 1;
        image = &fake;
    } else {
        image = (ImageRec*)s->image;
        if (IsBadReadPtr(image->lls, 1)) {
            DebugPrintf(g_msg_bad_image, image->name);
            return;
        }
    }
    g_sp_recolour = GetNearestColour((colour >> 16) & 0xff, (colour >> 8) & 0xff, colour & 0xff);
    src->left += s->src_x;
    src->top += s->src_y;
    src->right += s->src_x;
    src->bottom += s->src_y;
    g_sp_mouse_pixel = (char*)g_ddsd.lpSurface + g_ddsd.lPitch * g_mouse_point.y + g_mouse_point.x * 2;
    if (image->type == 2) {
        SoftBlitAnim(image->lls, src, dst);
        return;
    } else if (image->type == 3) {
        if (colour & 0xff000000) {
            g_sp_recolour = ~GetNearestColour(0xf, 0xf, 0xf);
            g_sp_rowlen = g_ddsd.lPitch;
            lls = (LLSRec*)image->lls;
            g_sp_left = src->left;
            g_sp_w = src->right - src->left;
            g_sp_top = src->top;
            g_sp_h = src->bottom - src->top;
            frame = g_frame_override;
            if (frame < 0)
                frame = lls->frame;
            nframes = lls->nframes;
            if (frame >= nframes)
                frame = nframes - 1;
            colour = frame;
            g_sp_mouse_pixel = (char*)g_ddsd.lpSurface + g_ddsd.lPitch * g_mouse_point.y + g_mouse_point.x * 2;
            row = (unsigned short*)((char*)g_ddsd.lpSurface + dst->top * g_ddsd.lPitch) + (dst->left - g_sp_left);
            f = (LLSFrame*)lls->frames;
            if (lls->flags & 1) {
                unsigned int k;
                DrawRLEFrame(row, f);
                k = lls->frame + 1;
                while (k-- != 0)
                    f = (LLSFrame*)((char*)f + f->size);
                DrawRLEFrame(row, f);
            } else {
                for (n = colour; n != 0; n--)
                    f = (LLSFrame*)((char*)f + f->size);
                DrawRLEFrame(row, f);
            }
        } else {
            SoftBlitRLE(image->lls, src, dst);
        }
        return;
    } else if (image->type == 0) {
        g_sp_pixels = image->lls;
        g_sp_width = (image->w + 3) & ~3;
        g_sp_height = image->h;
        g_sp_pal16 = (char*)image->pal + 4;
        __asm {
            pushad
            mov     eax, dst
            mov     edi, g_ddsd.lpSurface
            add     edi, [eax]
            add     edi, [eax]
            mov     ecx, [eax+4]
            imul    ecx, g_ddsd.lPitch
            add     edi, ecx
            mov     eax, src
            mov     edx, [eax+8]
            sub     edx, [eax]
            mov     g_sp_rowlen, edx
            mov     edx, [eax+0ch]
            sub     edx, [eax+4]
            mov     esi, g_sp_pixels
            add     esi, [eax]
            mov     ecx, [eax+4]
            imul    ecx, g_sp_width
            add     esi, ecx
            mov     ebx, g_ddsd.lPitch
            sub     ebx, g_sp_rowlen
            sub     ebx, g_sp_rowlen
            mov     ebp, g_sp_width
            sub     ebp, g_sp_rowlen
        row8:
            mov     ecx, g_sp_rowlen
            push    ebp
            push    ebx
            mov     ebx, g_sp_pal16
        pix8:
            movzx   eax, byte ptr [esi]
            movzx   eax, word ptr [ebx+eax*2]
            mov     ebp, eax
            cmp     eax, g_transparent_colour
            je      skip8
            and     ax, word ptr g_sp_recolour
            mov     word ptr [edi], ax
        skip8:
            inc     esi
            add     edi, 2
            loop    pix8
            pop     ebx
            pop     ebp
            add     esi, ebp
            add     edi, ebx
            dec     edx
            jne     row8
            popad
        }
    } else {
        g_sp_pixels = image->lls;
        g_sp_width = image->w * 2;
        g_sp_height = image->h;
        __asm {
            pushad
            mov     eax, dst
            mov     edi, g_ddsd.lpSurface
            add     edi, [eax]
            add     edi, [eax]
            mov     ecx, [eax+4]
            imul    ecx, g_ddsd.lPitch
            add     edi, ecx
            mov     eax, src
            mov     edx, [eax+8]
            sub     edx, [eax]
            mov     g_sp_rowlen, edx
            mov     edx, [eax+0ch]
            sub     edx, [eax+4]
            mov     esi, g_sp_pixels
            add     esi, [eax]
            add     esi, [eax]
            mov     ecx, [eax+4]
            imul    ecx, g_sp_width
            add     esi, ecx
            mov     ebx, g_ddsd.lPitch
            sub     ebx, g_sp_rowlen
            sub     ebx, g_sp_rowlen
            mov     ebp, g_sp_width
            sub     ebp, g_sp_rowlen
            sub     ebp, g_sp_rowlen
        row16:
            mov     ecx, g_sp_rowlen
        pix16:
            movzx   eax, word ptr [esi]
            cmp     eax, g_transparent_colour
            je      skip16
            and     ax, word ptr g_sp_recolour
            mov     word ptr [edi], ax
        skip16:
            add     esi, 2
            add     edi, 2
            loop    pix16
            add     esi, ebp
            add     edi, ebx
            dec     edx
            jne     row16
            popad
        }
    }
    if (s->flags & 0x20)
        ReleaseSprite(&handle);
}

/* -------------------------------------------------------------------------
 * 0x0045ff00 -- draw a build/destroy cursor: the footprint tiles (green /
 * blocked-red / special), the outline segments, any chained cursor, and the
 * entrance / exit arrows of the object being placed.
 * ------------------------------------------------------------------------- */

static __inline SpriteRec* TileSprite(int id)
{
    return g_tile_sprites[(id & 0xff) + *g_basic_tiles_data];
}

// WIP-FUNCTION: LEGOLAND 0x0045ff00
void RenderCursor(Cursor* c)
{
    WinRect          view;
    WinRect          saved;
    Rect             r;
    Rect*            p;
    Pos              t;
    TileBounds       tb;
    VideoSurfaceInfo vs;
    MapHdr*          map;
    Cell*            cell;
    int              x;
    int              y;
    int              i;
    short            th;
    unsigned int     flags;
    const char*      col;
    ObjDefRec*       o;

    map = g_map;
    view.left = map->origin_x;
    view.top = map->origin_y;
    view.right = map->view_w + map->origin_x;
    view.bottom = map->view_h + map->origin_y;
    GetClipping(&saved);
    SetClipping(&view);

    p = &c->rect;
    do {
        r = *p;
        if (!(c->flags & 0x10)) {
            for (y = r.top; y <= r.bottom; y++) {
                for (x = r.left; x <= r.right; x++) {
                    t.x = c->origin.x + x;
                    t.y = c->origin.y + y;
                    GetTileBounds(&t, &tb);
                    if (c->flags & 6) {
                        PrintSprite(TileSprite(g_tileset_id2), tb.left, tb.top, 0, 0);
                    } else if (t.x >= 0 && t.x < g_map->width && t.y >= 0 && t.y < g_map->height
                               && (cell = &g_map_rows[t.y][t.x]) != 0 && !(cell->flags & 0x8f8)) {
                        if (c->style & 0xc)
                            PrintSprite(TileSprite(g_tileset_id0), tb.left, tb.top, 0, 0);
                        else
                            PrintSprite(TileSprite(g_tileset_id1), tb.left, tb.top, 0, 0);
                    } else {
                        PrintSprite(TileSprite(g_tileset_id3), tb.left, tb.top, 0xff0000, 0);
                    }
                }
            }
        }
        p = r.next;
    } while (p);

    if (!GetVideoSurface(&vs))
        return;
    th = g_tile_sprites[g_default_tile]->h;
    for (i = 0; i < c->count; i++) {
        GetTileBounds(&c->origin, &tb);
        x = c->px[i] + tb.left;
        y = c->py[i] + tb.top;
        flags = c->flags;
        if (flags & 4)
            col = g_cursor_col_a;
        else if (flags & 2)
            col = g_cursor_col_b;
        else if (c->kind[i] & 0xc)
            col = g_cursor_col_c;
        else
            col = g_cursor_col_d;
        if (flags & 0x20) {
            switch (c->kind[i] & 3) {
            case 0: DrawCursorSegmentA(&vs, 0, x, y, col, g_map->tile_h); break;
            case 1: DrawCursorSegmentA(&vs, 1, x, y, col, th); break;
            case 2: DrawCursorSegmentA(&vs, 2, x, y, col, th); break;
            }
        } else {
            switch (c->kind[i] & 3) {
            case 0: DrawCursorSegmentB(&vs, 0, x, y, col, g_map->tile_h); break;
            case 1: DrawCursorSegmentB(&vs, 1, x, y, col, th); break;
            case 2: DrawCursorSegmentB(&vs, 2, x, y, col, th); break;
            }
        }
    }
    SetClipping(&saved);
    if (c->next)
        RenderCursor(c->next);
    if (c->flags & 0x400) {
        if (ObjHasEntrance(g_edit_object) && g_edit_changed == 1) {
            o = g_edit_object;
            t.x = o->ent_x + c->origin.x;
            t.y = o->ent_y + c->origin.y;
            GetTileBounds(&t, &tb);
            switch (GetObjEntranceDir(g_edit_object)) {
            case 1: PrintSprite(g_arrow1, tb.left, tb.top, 0, 0); break;
            case 2: PrintSprite(g_arrow3, tb.left, tb.top, 0, 0); break;
            case 4: PrintSprite(g_arrow4, tb.left, tb.top, 0, 0); break;
            case 8: PrintSprite(g_arrow2, tb.left, tb.top, 0, 0); break;
            }
            if (c->flags & 0x800) {
                if (ObjHasExit(g_edit_object)) {
                    o = g_edit_object;
                    t.x = o->exit_x + c->origin.x;
                    t.y = o->exit_y + c->origin.y;
                    GetTileBounds(&t, &tb);
                    switch (GetObjExitDir(g_edit_object)) {
                    case 0x10: PrintSprite(g_arrow1, tb.left, tb.top, 0, 0); break;
                    case 0x20: PrintSprite(g_arrow3, tb.left, tb.top, 0, 0); break;
                    case 0x40: PrintSprite(g_arrow4, tb.left, tb.top, 0, 0); break;
                    case 0x80: PrintSprite(g_arrow2, tb.left, tb.top, 0, 0); break;
                    }
                }
            }
        }
    }
}
