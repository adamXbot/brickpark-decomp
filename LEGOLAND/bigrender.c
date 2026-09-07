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
 * ---------------------------------------------------------------------------
 * Recovered formats and rules (for the browser runtime)
 * ---------------------------------------------------------------------------
 * LLS animation record (ImageRec::lls when ImageRec::type is 2 or 3):
 *     +0x00 short  current frame index
 *     +0x10 short  frame count
 *     +0x14 dword  flags; bit 0 = the frame list starts with a BASE image
 *                  that must be painted before the selected frame
 *     +0x18        the frame list itself
 * Each frame is a variable-length record:
 *     +0x00 int    byte length of this record (add it to walk to the next)
 *     +0x04 int    count of 16-bit entries in the first (control) block
 *     +0x08 int    byte length of the second block
 *     +0x10        data: control[+0x04 entries * 2 bytes], then the 16-bit
 *                  pixel block (second length), then the 8-bit block
 * SoftBlitRLEFrame (0x468410) takes those three pointers plus
 * (h, pitch, top, left, w, 0, mouse_pixel) -- it clips against the
 * g_sp_top/g_sp_left/g_sp_w/g_sp_h window the caller has just set.
 * Frame selection is the same rule everywhere in this file: take
 * g_frame_override (0x4b9ca8) when it is >= 0, else the record's own
 * current frame, then clamp to nframes - 1.
 *
 * Z buffer (ZBufferHelper): 0x200 bytes per row (128 dwords), one dword per
 * pixel with the Z value in the top byte.  Frame 0 of the list carries a
 * 0x200-byte table ahead of its RLE data (so the data starts at +0x208);
 * later frames start at +8.  The control stream is read four bits at a
 * time; the first pass skips src->top rows without painting, the second
 * paints src height rows clipped to src->left / src width.
 *
 * SoftPrint_XBltFast picks its path from ImageRec::type: 2 -> SoftBlitAnim,
 * 3 -> SoftBlitRLE unless the caller's colour has bit 31 set, in which case
 * it paints the "highlight" pass itself (mask = ~GetNearestColour(15,15,15))
 * through SoftBlitRLEFrame; 0 -> the 8-bit __asm loop with the palette at
 * ImageRec::pal + 4 and a row pitch of (w + 3) & ~3; anything else -> the
 * 16-bit __asm loop with a row pitch of w * 2.  Both loops skip
 * g_transparent_colour and AND every surviving pixel with g_sp_recolour.
 * A sprite with flag 0x20 is drawn from a GetSprite lock described by a
 * synthetic 16-bit ImageRec built on the stack and released at the end.
 *
 * Cursor block (0x1834 bytes, see the Cursor struct): point count at +0,
 * the outline point arrays at +2 / +0x802, a per-point kind byte at
 * +0x1002 (bits 0-1 pick the segment shape, bits 2-3 the "special"
 * colour), the map cell the footprint hangs off at +0x1404, the footprint
 * Rect list at +0x1414, a style byte at +0x1428, flags at +0x1828 and the
 * chained cursor at +0x1830.  Flag bits used here: 0x10 = do not paint the
 * footprint tiles, 0x02 / 0x04 = force the "blocked" / "special" colour,
 * 0x06 = paint every cell with tileset id2, 0x20 = use the A segment
 * renderer instead of B, 0x400 = also draw the object's entrance arrow,
 * 0x800 = and its exit arrow.  A cell is drawable when it is on the map
 * and none of the bits 0x8f8 are set in cell+0xc; the tile is id0 when
 * style & 0xc, else id1, and id3 (tinted 0xff0000) for a blocked cell.
 * Every tile id resolves as g_tile_sprites[(id & 0xff) + *g_basic_tiles_data].
 *
 * ---------------------------------------------------------------------------
 * VC6 SP3 levers learned in this file (general -- worth reusing elsewhere)
 * ---------------------------------------------------------------------------
 * EXIT-BLOCK ORDER.  VC6 lays a `goto` LABEL's block out BEFORE the block the
 * function merely falls through into at the end -- the opposite of source
 * order.  So the textbook
 *     ... if (X) goto fail;  EP; return 1;  fail: EP; return 0;
 * always comes out [fail][return 1], and no re-spelling of that CFG (inline
 * arms, duplicated returns, an epilogue helper, either textual order) moves
 * it.  To get [return 1][return 0] the SUCCESS epilogue has to be the
 * labelled goto target -- put it INSIDE the last `if` as the true arm, label
 * it there, and let every failure fall out of the enclosing `if` into ONE
 * trailing `return 0`, so the `goto ok` jumps INTO a compound statement.
 * That is what finished RenderSpriteScaledOffset (0x00488c80).
 *
 * SWITCH CASE ORDER IS NOT A LEVER: VC6 sorts a switch's cases by value, so
 * permuting the case labels in the source produces a byte-identical object.
 *
 * (OBSOLETE, kept for the record: `loop`/`loope`/`loopne` used to defeat
 * tools/audit.py, whose norm2() rewrote branch targets only for mnemonics
 * starting with `call`/`j`, capping ZBufferHelper and SoftPrint_XBltFast at
 * mismatch=2.  audit.py now normalises `loop` targets; ZBufferHelper prints
 * [OK] and SoftPrint_XBltFast's honest target is ZERO mismatches.)
 *
 * CROSS-JUMPING (tail merging).  Per join, VC6 SP3 picks exactly ONE
 * canonical tail -- the LAST predecessor block in layout order, the one the
 * join falls through from -- and merges every other predecessor into it, at
 * whatever depth each one's own suffix matches (mid-block entry is fine and
 * different predecessors enter at different depths).  It never merges two
 * NON-canonical predecessors with each other, however long their shared
 * tail.  An allocator-inserted reload sitting in the canonical block (a
 * `mov reg,[esp+N]` between a `call` and its `add esp,N`) makes that block
 * unmergeable at every depth after the reload, so a switch whose last-laid-
 * out case spills a callee-saved register merges nothing at all.  Derived
 * from the standalone repro pair in scratchpad/bigrender/run2/ -- see the
 * RenderCursor note for the full statement and for the case it does NOT
 * explain.
 *
 * ---------------------------------------------------------------------------
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
 * No `ret` follows the noreturn call. tools/audit.py bounds it at 13i/46B and
 * reports mismatch=0 -- the body is exact -- because it treats the
 * inter-function padding run as a terminator. The SHARED tools/match.py does
 * not: with no `ret` to stop at it decodes on into the next function and
 * scores this 50%, so promoting the marker to `// FUNCTION:` turns verify.py
 * red. Held as WIP for the same tooling reason as the void tail-jump wrappers
 * (see "Tail-jump functions" in docs/DECOMP.md); promote it when match.py
 * adopts audit.py's terminator rules.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00488c50
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

/* EXIT-BLOCK ORDER (the lever that finished this function, worth recording):
 * VC6 SP3 lays a `goto` LABEL's block out BEFORE the block that the function
 * simply falls through into at the end -- the opposite of source order.  So
 * `... if (X) goto fail; EP; return 1; fail: EP; return 0;` always came out
 * as [fail][return 1] (and every other goto/inline/duplicate spelling of the
 * same CFG normalised to it, ~15 variants).  The original is [return 1]
 * (0x488fcb) then [return 0] (0x489026), i.e. the SUCCESS epilogue is a
 * labelled goto target and the FAILURE epilogue is the function's trailing
 * fall-through block.  That is only reachable by putting the success
 * epilogue INSIDE the last `if` as its true arm, labelling it, and letting
 * every failure fall out of the `if (rc == DDERR_SURFACELOST)` body into one
 * trailing `return 0` -- so `goto ok` jumps INTO a compound statement.  With
 * that shape all 272 instructions and 1022 bytes are exact.
 * The three epilogue copies are real: 0x488f21 keeps its own, differently
 * scheduled, copy (VC6 cross-jumps only the two that came out identical to
 * the trailing block), so do NOT "simplify" them into one. */
// FUNCTION: LEGOLAND 0x00488c80
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
    if (rc == 0)
        goto ok;
    if (rc == DDERR_SURFACELOST) {
        if (g_scaled_surface->vtbl->Restore(g_scaled_surface) != 0) {
            g_draw_surface->vtbl->SetClipper(g_draw_surface, 0);
            g_ddsd = g_scaled_saved_ddsd;
            g_render_clip = g_scaled_saved_clip;
            return 0;
        }
        MakeSprite(s);
        if (g_primary->vtbl->IsLost(g_primary) == DDERR_SURFACELOST) {
            if (g_primary->vtbl->Restore(g_primary) != 0) {
                g_draw_surface->vtbl->SetClipper(g_draw_surface, 0);
                g_ddsd = g_scaled_saved_ddsd;
                g_render_clip = g_scaled_saved_clip;
                return 0;
            }
        }
        if (g_draw_surface->vtbl->Blt(g_draw_surface, &F.dst, g_scaled_surface, &F.src, 0x8000, 0) == 0) {
ok:
            g_draw_surface->vtbl->SetClipper(g_draw_surface, 0);
            g_ddsd = g_scaled_saved_ddsd;
            g_render_clip = g_scaled_saved_clip;
            return 1;
        }
    }
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

/* Exact: 303/303 instructions, 1099/1099 bytes, zero mismatches. The earlier
 * note here blamed tools/audit.py for not normalising `loop` branch targets
 * (their mnemonic does not start with "j", so neither norm() nor norm2()
 * rewrote them and an identical body still reported two mismatches). That
 * gap is fixed; the diagnosis was correct. */
// FUNCTION: LEGOLAND 0x00464a90
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

#ifndef LEGOLAND_PORTABLE
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
#else
    LL_UNPORTED_ASM(); /* ZBufferHelper: the RLE z-buffer walker */
#endif
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

/* EXACT since 2026-09-04 (audit.py [OK], 392/392 instructions, 1174/1174
 * bytes).  It sat at 22 mismatches from index 132 for two rounds; five
 * levers, in the order they were found, took it to zero.  Keep every one of
 * them -- removing any single one costs 8-12 mismatches (measured).
 *   1. `image = &fake;` BEFORE the four fake.* stores, so the
 *      `lea esi,[ebp-0x30]` is emitted ahead of them.
 *   2. `f = lls->frames` derived IMMEDIATELY after `lls = image->lls`, not
 *      where it is first used.  That early derivation is what makes VC6
 *      coalesce `image` with `f` (whose two frame-walk loops carry the loop
 *      weight) into esi and `src` with `row` into edi.  Derived late, esi
 *      goes to `src`, edi to `image`+`lls`, `row` is spilled into the dst
 *      argument slot, and every register in the C part changes name.
 *   3. the two-def frame clamp `colour = frame; if (frame >= nframes)
 *      colour = nframes - 1;` together with the volatile flag-byte read.
 *      These two are COUPLED: either alone shifts every later index by one.
 *   4. (2026-09-04) the FIRST g_sp_mouse_pixel assignment is spelled
 *      `g_mouse_point.y * g_ddsd.lPitch` and the second, otherwise
 *      identical one `g_ddsd.lPitch * g_mouse_point.y`.  Written the same
 *      way round in both, VC6 links them as ONE textual common
 *      subexpression, and that demotes the pitch at the second site from a
 *      rank-1 TEMPORARY to a register-candidate SYMBOL -- so the memory
 *      operand becomes the destination copy (`mov ecx,[mouse.y] /
 *      imul ecx,edx`) where the original copies the live pitch register and
 *      folds the memory operand (`mov ecx,edx / imul ecx,[mouse.y]`).  This
 *      is the residual two agents recorded as "VC6's operand
 *      canonicalisation of a commutative imul, unreachable from C": it is
 *      reachable, but only from the OTHER statement.
 *   5. (2026-09-04) the `dst` PARAMETER is read as volatile at the
 *      `dst->left` access in the row expression, and only there.  Reads of a
 *      stack argument are CSE'd function-wide into one root copy; the
 *      original re-reads the home slot for that access, and the volatile
 *      read of the parameter (not of the field) is what splits the CSE.
 *      Worth 8: it fixes the load order dst->left/g_sp_left, keeps the
 *      g_sp_left load below the g_sp_mouse_pixel store and un-swaps ecx/edi
 *      through the whole row computation.  The same shim on `dst->top`, or
 *      on both accesses, is worse; `*(volatile int*)&dst->left` (the FIELD
 *      volatile) only gets the order right, not the registers.
 *   6. (2026-09-04) the THIRD DrawRLEFrame site (the else arm) must be
 *      written out with `f->n16` read into a local.  With the repeated
 *      `f->n16` of the inline helper the two reads are CSE'd into a compiler
 *      TEMPORARY, which the scheduler treats as a critical-path root and
 *      hoists together with its `lea` to the top of the block, ahead of the
 *      g_sp_mouse_pixel load (8 mismatches).  The other two sites want the
 *      CSE and keep the helper.
 * Inert (do not re-derive; all byte-identical): commuting the second
 * multiply, `g_sp_rowlen` for the pitch in either or both statements, a
 * named `int pitch`/`int my`/`int mx`/product local anywhere, inline helpers
 * for the mouse-pixel or row expression in any parameter order, `(long)`/
 * `(int)`/`(unsigned)` casts on either multiply operand, declaring lPitch
 * `int` or g_sp_rowlen `long`, splitting g_mouse_point into two plain int
 * externs, every row-expression re-association, `n = dst->left - g_sp_left;`
 * as its own statement, a `WinRect* d = dst` block local, the doubled char*
 * row form, all else-arm loop shapes (`while (n--)`, do/while, for/break,
 * a separate walker), textual expansion of the other two call sites, a
 * shared trailing DrawRLEFrame after the if/else (VC6 merges it: 362
 * instructions) and the same via `goto`, and making any of the g_sp_* stores
 * or reads volatile other than the two shims above. */
// FUNCTION: LEGOLAND 0x00465a40
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
        image = &fake;
        fake.lls = handle.pixels;
        fake.w = (short)handle.pitch / 2;
        fake.h = (short)handle.h;
        fake.type = 1;
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
    /* CODEGEN LEVER (2026-09-04): the product is spelled `y * pitch` HERE and
     * `pitch * y` at the second, identical assignment inside the type-3 arm.
     * Written the same way round in both, VC6 links the two occurrences as one
     * textual common subexpression, which demotes the pitch at the second site
     * from a rank-1 temporary to a register-candidate symbol -- so the memory
     * operand becomes the destination copy there (`mov ecx,[mouse.y] / imul
     * ecx,edx`) where the original copies the live pitch register and folds the
     * memory operand (`mov ecx,edx / imul ecx,[mouse.y]`).  Both orders are the
     * same value; only this spelling reproduces the original.  Emitted code for
     * THIS statement is identical either way. */
    g_sp_mouse_pixel = (char*)g_ddsd.lpSurface + g_mouse_point.y * g_ddsd.lPitch + g_mouse_point.x * 2;
    if (image->type == 2) {
        SoftBlitAnim(image->lls, src, dst);
        return;
    } else if (image->type == 3) {
        if (colour & 0xff000000) {
            g_sp_recolour = ~GetNearestColour(0xf, 0xf, 0xf);
            lls = (LLSRec*)image->lls;
            f = (LLSFrame*)lls->frames;
            g_sp_rowlen = g_ddsd.lPitch;
            g_sp_left = src->left;
            g_sp_w = src->right - src->left;
            g_sp_top = src->top;
            g_sp_h = src->bottom - src->top;
            frame = g_frame_override;
            if (frame < 0)
                frame = lls->frame;
            nframes = lls->nframes;
            colour = frame;
            if (frame >= nframes)
                colour = nframes - 1;
            g_sp_mouse_pixel = (char*)g_ddsd.lpSurface + g_ddsd.lPitch * g_mouse_point.y + g_mouse_point.x * 2;
            /* CODEGEN LEVER (2026-09-04): the ORIGINAL re-reads the `dst`
             * argument from its home slot for the left field
             * (`mov ecx,[ebp+0x10]` at 0x465bdd and again at 0x465be9), while
             * every plain spelling lets VC6 CSE the two reads into one root
             * copy and then rematerialise it -- which swaps ecx/edi through
             * the whole row computation and hoists the g_sp_left load above
             * the g_sp_mouse_pixel store (8 mismatches).  Reading the
             * PARAMETER (not the field) as volatile at the left access only
             * splits the CSE and reproduces the original exactly; the same
             * shim on the top access instead, or on both, is worse. */
            row = (unsigned short*)((char*)g_ddsd.lpSurface + dst->top * g_ddsd.lPitch)
                  + ((*(WinRect* volatile*)&dst)->left - g_sp_left);
            /* CODEGEN LEVER: the original loads the flag byte into a
             * register first (`mov cl,[ebx+0x14]` / `test cl,1`); every
             * plain spelling -- lls->flags & 1, an unsigned char local, a
             * ((unsigned char*)lls)[0x14] read -- folds into `test byte
             * ptr [ebx+0x14],1`, one instruction short.  The volatile read
             * is confined to this one test and means exactly the same
             * thing (bit 0 of the little-endian dword at +0x14). */
            if (*(volatile unsigned char*)&lls->flags & 1) {
                unsigned int k;
                DrawRLEFrame(row, f);
                k = lls->frame + 1;
                while (k-- != 0)
                    f = (LLSFrame*)((char*)f + f->size);
                DrawRLEFrame(row, f);
            } else {
                /* CODEGEN LEVER (2026-09-04): this third call site must NOT go
                 * through DrawRLEFrame.  Written with the repeated `f->n16`
                 * the two reads are CSE'd into a compiler TEMPORARY, which the
                 * scheduler treats as a critical-path root and hoists (with
                 * its `lea`) to the top of the block, ahead of the
                 * g_sp_mouse_pixel load -- 8 mismatches.  Naming the field in
                 * a local makes it an ordinary symbol whose load stays in
                 * program order, exactly as the original has it.  The two call
                 * sites in the other arm want the CSE and must keep the
                 * inline helper. */
                int nn;
                for (n = colour; n != 0; n--)
                    f = (LLSFrame*)((char*)f + f->size);
                nn = f->n16;
                SoftBlitRLEFrame(row, f->data, f->data + nn * 2,
                                 f->data + nn * 2 + f->n2, g_sp_h, g_ddsd.lPitch,
                                 g_sp_top, g_sp_left, g_sp_w, 0, g_sp_mouse_pixel);
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
#ifndef LEGOLAND_PORTABLE
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
#else
        g_sp_rowlen = (int)(src->right - src->left);
        ll_blit8(g_ddsd.lpSurface, g_ddsd.lPitch, dst->left, dst->top, g_sp_pixels, g_sp_width,
                 src->left, src->top, src->right, src->bottom, g_sp_pal16,
                 g_transparent_colour, g_sp_recolour);
#endif
    } else {
        g_sp_pixels = image->lls;
        g_sp_width = image->w * 2;
        g_sp_height = image->h;
#ifndef LEGOLAND_PORTABLE
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
#else
        g_sp_rowlen = (int)(src->right - src->left);
        ll_blit16(g_ddsd.lpSurface, g_ddsd.lPitch, dst->left, dst->top, g_sp_pixels, g_sp_width,
                  src->left, src->top, src->right, src->bottom,
                  g_transparent_colour, g_sp_recolour);
#endif
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

/* Residual (re-measured 2026-09-03 with tools/audit.py): 181/454
 * instructions, 1438 vs 1429 bytes, mismatch=273.  The mismatching indices
 * are exactly
 *     48-57  63-65  68-69  114-115  122-124  127-129   (23, residual (a))
 *     204-453                                          (250, residual (b))
 * so residual (b) -- nine bytes of un-cross-jumped switch tail -- is what
 * shifts every later index and costs 250 of the 273.  Fix (b) and the score
 * is 431/454; fix both and it is exact.
 *
 *   (a) From the first instruction of the tile-loop body (0x45ffa3) to the
 *       end of that loop our scratch registers are the original's renamed by
 *       a 3-cycle (ours eax->orig edx, ours ecx->orig eax, ours edx->orig
 *       ecx): the original opens the block with `mov edx,[c+0x1404]` where we
 *       emit `mov eax,...`.  The schedule, the frame slots and the loop
 *       registers (esi = x, edi = y, ebp = c, ebx = i) all match; only the
 *       colouring of the four block temps t.x / t.y / &tb / &t differs, and
 *       &t always reuses t.x's register in both.  The rotation carries into
 *       the `c->flags & 6` arm, whose tile id therefore lands in eax instead
 *       of edx -- and THAT is why VC6 cross-jumps that arm's TileSprite tail
 *       into the g_tileset_id1 arm (which also holds its id in eax) instead
 *       of into the red g_tileset_id3 arm as the original does (0x4600b3).
 *       The id registers of the three other arms (ecx / eax / edx for
 *       g_tileset_id0 / _id1 / _id3) already match; only the flags&6 arm is
 *       wrong, and only because of the rotation.
 *       IMPORTANT (established 2026-09-03): (a) is INDEPENDENT of (b).  Two
 *       diagnostic builds that completely restructure the switch region
 *       (`peel0` and `diagBsplit` in scratchpad/bigrender/run/) leave
 *       48-57 bit-for-bit unchanged.  So they must be attacked separately;
 *       do not assume fixing one fixes the other.
 *       Tried without effect (2026-09-03 additions first): a
 *       `static __inline void GTB(TileBounds*, Pos*)` wrapper that reverses
 *       the two argument temporaries, `Pos* pt = &t` / `TileBounds* pb = &tb`
 *       block locals, `t.x = t.x + x` instead of `t.x += x`, `t.x -= -x`,
 *       t and tb in ONE aggregate (does not compile as written -- redo it if
 *       you want it), tb declared before t, and t/tb declared first of all
 *       the function-level locals.  Earlier: `t.x = x + t.x`, a redundant
 *       cast on &tb, a `tb.left = tb.left;` probe store, t.y before t.x in
 *       the load pair, every order of the four t.x/t.y statements, `t =
 *       c->origin` as a struct copy, named int temporaries, volatile reads of
 *       the origin fields, pointer locals for &t and &tb, a static __inline
 *       helper for the whole "build t then call GetTileBounds" step, separate
 *       Pos/TileBounds locals for the tile loop, separate x/y locals for the
 *       two loops, hoisting &vs into a pointer local, and reordering the
 *       switch cases.  Note `t.x = c->origin.x + x` (the fused form) is NOT
 *       equivalent -- see the LEVER at the bottom.
 *       WHAT THE COLOURING ACTUALLY IS (2026-09-03).  The block allocates
 *       four temps in creation order and the pattern is always the same:
 *       temp1 = first free scratch, temp2 = next free, temp3 = the one
 *       remaining free scratch, temp4 (&t) = temp1's register, which the
 *       store on the line above has just freed.  Ours comes out
 *       (eax, ecx, edx, eax); the original is (edx, eax, ecx, edx).  So
 *       the whole residual is the choice of temp1, and across roughly
 *       twenty further spellings measured today temp1 is ALWAYS eax -- no
 *       statement order, temporary, pointer local or cast moves it.  The
 *       lever therefore has to make eax unavailable (or less preferred) at
 *       0x45ffa3, i.e. it is a whole-function allocation-order lever, not
 *       a re-spelling of these five lines.  Ruled out today on top of the
 *       list above: `Pos* org = &c->origin;`, a comma-expression argument,
 *       `t.x -= -x`, `t.x = t.x + x`, both t.y-first orderings, named int
 *       temporaries for either coordinate, and (in the flags&6 arm, to try
 *       to force its tile id into edx directly) a `SpriteRec* sp =
 *       TileSprite(g_tileset_id2);` local, an `int id2` local and a
 *       volatile read of g_tileset_id2 -- all three byte-identical.
 *       DO NOT move t/tb into a block scope: it is the CURRENT
 *       function-level declaration that produces the original's frame.
 *       Measured map (S = esp just after `sub esp,0x68`): S+0x00 compiler
 *       temp (the c->py walk pointer), S+0x04 t, S+0x0c tb, S+0x1c view,
 *       S+0x2c saved, S+0x3c r, S+0x50 vs, and `i`'s spill home is S+0x6c
 *       -- the incoming `c` ARGUMENT slot, which VC6 reuses because c
 *       lives in ebp.  Declaring t/tb inside the tile loop moves t to
 *       S+0x00 and shifts everything (measured: 302 mismatches).
 *
 *   (b) The original cross-jumps the tails of cases 1 and 2 of the
 *       DrawCursorSegmentA switch (0x4601c0 `push 2` / `jmp 0x4601cd`, with
 *       `lea eax,[esp+0x74]; push eax; call; add esp,0x18; jmp` shared) -- and
 *       does NOT do the same for the DrawCursorSegmentB switch.  We emit both
 *       switches unmerged: 4 extra instructions and 9 extra bytes.
 *       The `lea eax,[esp+0x74]` after the `push <kind>` is a CONSEQUENCE of
 *       the merge (the original's own unmerged blocks all have
 *       `lea eax,[esp+0x70]` BEFORE the push imm, e.g. 0x460212), so the
 *       lever is whatever makes VC6's cross-jumper accept the two blocks.
 *
 *       WHAT THE 2026-09-03 REPRO MATRIX ESTABLISHED (scratchpad/bigrender/
 *       run/{k,l,m,n}.py drive it; repro_probe.py counts `push <imm>`
 *       immediately followed by `jmp`, the cross-jump signature):
 *         * This VC6 build merges a switch's case tails ONLY when EVERY case
 *           of that switch shares the tail, and only for the LAST switch
 *           before the join.  A strict subset never merges.
 *           - 2-case switch (no case 0)                 -> merges (1 site)
 *           - 3 cases, all with the same 6th argument   -> merges (2 sites)
 *           - 3 cases, case 0 differs but no register
 *             compensation needed                       -> merges (2 sites)
 *           - 3 cases, case 0 needs the `mov ebx,[esp+N]`
 *             loop-counter reload                       -> NO merge
 *           - two switches, both mergeable              -> only the SECOND
 *         * The blocker in RenderCursor is that compensation block:
 *           `g_map->tile_h` in case 0 costs two registers
 *           (`mov ebx,[g_map]` + `xor edx,edx; mov dx,[ebx+0x18]`), every
 *           other register is live (eax=x, ecx=y, edx=kind/h, esi=col,
 *           edi=th, ebp=c), so VC6 clobbers ebx=i and inserts the shared
 *           `mov ebx,[esp+0x94]; add esp,0x18` block at 0x460257.  That
 *           splits the join's predecessors into two suffix groups and the
 *           merge is abandoned.
 *         * THE PARADOX, stated precisely so nobody re-derives it: the
 *           ORIGINAL has that same compensation block (0x460257 is reached
 *           by `jmp` from A0 and by fall-through from B0) AND still merges
 *           A1/A2 -- a strict subset, in the FIRST switch.  No spelling
 *           found so far reproduces that combination.  The next attempt
 *           should look for whatever makes VC6 run its cross-jumper on the
 *           first group, not for a different way to write the switch.
 *       Ruled out 2026-09-03, all byte-identical to what we already emit:
 *         `(int)g_map->tile_h`, `(*g_map).tile_h`, `(g_map)->tile_h`;
 *         `switch ((int)(c->kind[i] & 3))`, `switch ((unsigned char)(...))`,
 *         `if ((flags & 0x20) != 0)`, `if (c->flags & 0x20)`; callee
 *         prototypes with `void*` first arg, `unsigned int` kind, `const
 *         void*` col, an `int` return, and no prototype at all; a `goto`
 *         past the else arm; the braceless if/else.
 *       Ruled out because they change the output for the WRONG reason
 *       (measured, do not be tempted):
 *         * `map->tile_h` (the local) instead of `g_map->tile_h`: 427.
 *         * `switch (c->kind[i] % 4)`: 305 and ESCAPES.
 *         * `kk = c->kind[i] & 3;` hoisted above the `if`: merges, but the
 *           `and` is hoisted with it (the original masks inside EACH arm)
 *           and the frame grows 0x68 -> 0x6c.  330.
 *         * `unsigned char kb = c->kind[i];` + `switch (kb & 3)`: 329.
 *         * two sequential `if (flags & 0x20) {A} if (!(flags & 0x20)) {B}`:
 *           merges all three cases of the FIRST switch and scores 189, but
 *           only because `flags` must survive the first switch, which spills
 *           the loop counter to memory and re-lays the whole prologue.
 *         * `if ((c->kind[i] & 3) == 0) {case0} else switch (...) {1,2}`:
 *           scores 100 (!) and merges, and is semantically identical -- but
 *           it is the WRONG SHAPE and must not be committed.  VC6 fuses the
 *           zero test into the mask (`test dl,3 / jne`, or `and edx,3 / jne`
 *           with a named kk), whereas the original emits `and edx,3 /
 *           sub edx,0 / je`, which is a three-case switch's lowering and
 *           nothing else.  It scores well only because dropping an
 *           instruction happens to re-align indices 270+.  Recorded so the
 *           next agent does not "discover" it and commit a lie.
 *         * `case 0` peeled into a nested `default: switch (kk) {1,2}`: 279.
 *         * `case 1: kind = 1; break; case 2: kind = 2; break;` join feeding
 *           one shared call: spells the constant into a register, +22 bytes.
 *         * every permutation of the case labels in either switch: VC6 sorts
 *           switch cases by value, so case ORDER is not a lever at all.
 *         * making BOTH arms call DrawCursorSegmentA, and deleting the else
 *           arm entirely: still no merge, so the second switch is not the
 *           blocker.
 *         * `else if (flags & 0x40)`: 268, ESCAPES.
 *
 *       HOW VC6 SP3's CROSS-JUMPER ACTUALLY WORKS (established 2026-09-03
 *       from a 40-line standalone repro -- scratchpad/bigrender/run2/
 *       repro_nomerge.c reproduces OUR output for this region exactly and
 *       repro_merge.c the merging one; drive them with run2/sw.py).  This
 *       supersedes the "only when EVERY case shares the tail / only the
 *       LAST switch" model above, which was an artefact of too-simple
 *       repros:
 *         1. Per join, VC6 chooses exactly ONE canonical tail: the LAST
 *            predecessor block in layout order -- the one the join falls
 *            through from.  Every other predecessor merges into THAT block,
 *            entering it at whatever depth its own suffix matches; different
 *            predecessors enter at different depths and mid-block entry is
 *            fine (repro_merge.c: case 2 enters the shared tail at the
 *            `lea`, case 1 four instructions later at the `call`).
 *         2. It NEVER merges two non-canonical predecessors with each other,
 *            even when they share a five-instruction tail and identical
 *            registers.  That is exactly what A1/A2 would need.
 *         3. An allocator-inserted reload in the canonical block -- our
 *            `mov ebx,[esp+0x94]` sitting between the `call` and its
 *            `add esp,0x18` -- makes the canonical unmergeable at every
 *            depth AFTER the reload.  Only a predecessor whose own tail
 *            contains the same reload can still merge, which is precisely
 *            the one merge we do get (A0 into B0's reload+cleanup).  Make
 *            the case-0 argument a SIGNED short (`movsx edx,[..]`, one
 *            register, no ebx clobber, no reload) and the second switch
 *            immediately merges all three cases -- measured, and not
 *            committable because the original really does emit
 *            `xor edx,edx / mov dx,[ebx+0x18]`.
 *         4. Corollary: only the arm laid out LAST can merge.  Confirmed
 *            three ways: a third `else if` arm merges only the third arm;
 *            `if (!(flags & 0x20)) {B} else {A}` merges A (which VC6 then
 *            lays out second); and replacing the B arm with an if/else-if
 *            chain -- where case 0 is laid out FIRST -- merges that arm's
 *            cases 1/2 despite the reload, because the canonical is then
 *            the clean case-2 block.
 *       THE PARADOX, restated with that model: the original merges the
 *       0x45fca0 arm, which is the FALL-THROUGH (first) arm of
 *       `test dl,0x20 / je <0x45fad0 arm>`.  Under rule 1 the merged arm
 *       must be the one laid out LAST.  No C spelling tried reaches that
 *       combination: the negated condition, the goto form, `continue` in
 *       the then-arm, two sequential ifs and else-if all normalise to the
 *       same [0x45fca0 arm][0x45fad0 arm] layout with `je` to the second.
 *       So the next attempt must find a construct that emits the 0x45fca0
 *       arm SECOND while keeping `je` to the 0x45fad0 dispatch -- or must
 *       accept that this arm's case 0 is not part of that switch in the
 *       original.  Iterate in the repro (a build is ~0.3 s), not on the
 *       full function.
 *       Also ruled out 2026-09-03, all byte-identical to what we emit:
 *       `default: break;` in either or both switches, braces round each
 *       case body, a block-scope `unsigned short h = g_map->tile_h;` in
 *       case 0, an added empty `case 3:`, `switch (c->kind[i])` unmasked,
 *       either or both case-0 arms calling a third function, and extra
 *       code added before or after the segment loop.
 *
 *       ===== 2026-09-04 =====
 *       THE BLOCKER IS A LENGTH THRESHOLD ON THE SHARED SUFFIX, and the
 *       mechanism is now reproducible on demand.  Give cases 1 and 2 of the
 *       FIRST arm ONE extra shared trailing instruction (e.g. a
 *       `g_sp_rowlen = 0;` after each call) and VC6 immediately emits the
 *       ORIGINAL'S EXACT SHAPE: `... push 2 / jmp T` and `... push 1` falling
 *       into `T: lea eax,[esp+0x74] / push eax / call / add esp,0x18 / jmp`.
 *       Measured twice: in the standalone repro (mergesites 0 -> 1, 140 ->
 *       137 insns) and in the real function (`Q1_Atail`: 342 -> 320
 *       mismatches, 523 -> 521 insns).  So our shared suffix is FIVE
 *       instructions (lea / push eax / call / add esp,0x18 / jmp) and this
 *       VC6 wants SIX; the original merged at five.
 *       Not a BYTE threshold: our two case blocks' `jmp`s are both 5-byte
 *       near jumps (verified in our object at 0x2ce and 0x2e9), so our
 *       candidate suffix is 4+1+5+3+5 = 18 bytes -- exactly the size of the
 *       original's shared tail at 0x4601cd -- and it still does not merge.
 *       The merge runs BEFORE the scheduler: the `lea` can only be inside
 *       the shared block if it was still after the `push <imm>` when the
 *       merge ran, and in every UNMERGED block (ours and the original's B
 *       arm and case 0s) the scheduler hoists that `lea` above the
 *       `push <imm>`.  So "lea eax,[esp+0x74] after the push" is a
 *       consequence of the merge, as the older note guessed -- now proved by
 *       construction, not inferred.
 *       The threshold is a COUNT, not bytes: a 2-byte `th = 0;` in both case
 *       bodies triggers it exactly as a 10-byte store does, while any tail
 *       statement VC6 dead-code-eliminates (`flags = 0;`, `col = 0;`, `x++;`)
 *       leaves the object byte-identical.  And the threshold DEPENDS ON THE
 *       ROLE of the block: merging INTO the canonical (the predecessor the
 *       join falls through from) happens at depth 4 -- repro_merge.c's second
 *       arm merges `lea / push eax / call / add esp,0x18` -- while merging two
 *       NON-canonical predecessors with each other, which is what A1/A2 need,
 *       takes 6.  The canonical for this join is always the ebx-reload block
 *       that the B arm's case 0 falls into, and no C spelling makes the A arm
 *       the fall-through predecessor while keeping `je` to the B dispatch.
 *       => What is needed is a SEMANTICALLY VALID sixth shared instruction
 *       that disappears again before the final code (a register copy the
 *       allocator coalesces away, or a split `add esp,0x14 / add esp,4` pair
 *       that a later peephole recombines -- see the DECOMP note that a call
 *       result pushed straight into another call splits the `add esp`), or a
 *       construct that lowers VC6's threshold.  Do NOT keep re-spelling the
 *       switch: the shape space is exhausted (below).
 *       Newly ruled out 2026-09-04 (repro + full function, all byte-identical
 *       to the plain if/else unless noted; the repro is
 *       scratchpad/bigrender/run2/repro_nomerge.c driven by
 *       scratchpad/bigrender/s0904/rp.py, ~0.3 s a build):
 *         * every CFG re-shaping of the two arms -- `goto` past either arm,
 *           `goto` with the A arm written second, `do { ... break; ... }
 *           while (0)`, `continue` inside the cases or after the switch, the
 *           negated condition -- 140 insns / 387 bytes, identical.
 *         * `case K: f(&vs, kk, ..)` with `kk` the switch value is
 *           byte-identical to `case K: f(&vs, K, ..)`: VC6 constant-
 *           propagates the switch value into every arm.  (Useful fact, no
 *           merge.)
 *         * no-op IR differences on the OTHER arm's arguments (`(int)th`,
 *           `(short)th`, `(VS*)&vs`, `(const char*)col`, `(int)x`, on either
 *           of its cases): byte-identical.  There is no "one group per join"
 *           effect to exploit.
 *         * shifting the instruction stream upstream (1..9 volatile stores
 *           before the loop or inside it, before the if/else) never moves the
 *           merge or the `lea`: the Pentium-window lever does not apply here.
 *         * making the OTHER arm's tail one instruction longer merges THAT
 *           arm, not this one.
 *         * a fourth `case 3:` sharing the same tail does not help, so the
 *           threshold is per PAIR, not on the total saving.
 *         * `static __inline` wrappers round the call, in either parameter
 *           order.
 *         * all six case-label permutations in either arm, re-measured on the
 *           full function: byte-identical (confirms the old note).
 *       For (a), newly ruled out 2026-09-04: volatile reads of the origin
 *       FIELDS singly and together (byte-identical), a `Pos* org` block
 *       local, a cast on `&tb`, `while` forms of either loop or both
 *       (byte-identical -- loop form is not a lever here), and `t.y` before
 *       `t.x` in the accumulate pair (worse, 346).  The parameter-volatile
 *       shim that closed SoftPrint_XBltFast (`(*(Cursor* volatile*)&c)`)
 *       forces a reload of `c` from its home and costs 140 more (484).
 *       ===== 2026-09-04, lane H =====
 *       Residual UNCHANGED at 273.  Three new results, two of them useful.
 *       (i) READ OFF THE ORIGINAL, and it reframes the whole thing: the
 *       ELSE arm (DrawCursorSegmentB) is NOT merged in the original either.
 *       0x0046020b and 0x00460223 are two full, separate case blocks with
 *       the `lea eax,[esp+70h]` already hoisted above `push 2` / `push 1`
 *       exactly as we emit them, and each has its own `call 0x45fad0 /
 *       add esp,18h / jmp 0x460261`.  So the original's compiler merged
 *       EXACTLY ONE pair in this function -- cases 1 and 2 of the FIRST arm
 *       -- and left the structurally identical pair in the second arm alone.
 *       That is the exact opposite of this build's "only the LAST switch
 *       merges" rule, and it means the difference is not a threshold we can
 *       satisfy by lengthening a suffix but a different choice of WHICH
 *       candidate pair the pass takes.  Anything that makes our second arm
 *       merge instead is therefore also wrong.
 *       (ii) The shared-tail construct IS reachable from C, just not with
 *       immediate pushes.  Writing the original's block shape literally --
 *         case 1: kd = 1; goto a12;
 *         case 2: kd = 2;  a12: DrawCursorSegmentA(&vs, kd, x, y, col, th);
 *       -- does produce one merged block fed by two tiny arms, but `kd`
 *       becomes a phi in a register, so the arms are `mov edx,2 / jmp` and
 *       `mov edx,1` and the merged block starts at `movsx ebx,bx` with
 *       `push edx` for the kind: 274 X, 1451 bytes, 391 real instructions.
 *       VC6 does not sink the two constants back into `push imm`.  Both
 *       label placements (label in case 1 or in case 2) are identical.
 *       `if (kk == 0) {..} else if (kk == 1 || kk == 2) {..}` is 289.
 *       (iii) For residual (a) the accumulate pair was swept exhaustively:
 *       all SIX legal interleavings of {t.x = c->origin.x, t.y =
 *       c->origin.y, t.x += x, t.y += y} plus both `t = c->origin;`
 *       struct-copy forms.  Only three outcomes -- 273 (the committed order,
 *       `t = c->origin` and `lylxayax`), 276 (`lxlyayax`, `lylxaxay`) and
 *       397 + ESCAPES (either fully-sequential coordinate form).  The
 *       3-cycle rename eax->edx->ecx at index 48 is untouched by statement
 *       order, so it is an allocation-order fact about the block's first
 *       temp, not a source-order one.
 *
 *       ===== 2026-09-04, lane M =====
 *       Residual UNCHANGED at 273.  Two results, and the second one probably
 *       CLOSES THE QUESTION.
 *       (i) THE GOTO-FLIP LEVER DOES NOT REACH THIS FUNCTION.  Eleven
 *       spellings of it in the repro (scratchpad/laneM/rc1.py): `goto endsw`
 *       in all six cases, in the A arm only, in A case 0 / 1 / 2 alone, in A
 *       cases 1+2, in the B arm only, and the flat `if (!(flags & 0x20)) goto
 *       barm; ... goto endsw; barm: ... endsw:;` restructuring with and
 *       without case gotos.  Every one is 140 insns / 387 bytes / 0 merge
 *       sites -- byte-identical to the plain if/else.  Inside a switch a
 *       `goto join` and a `break` are literally the same instruction, so no
 *       asymmetry can arise from them; the sibling's lever needs a join the
 *       switch does not already own.
 *       (ii) THE CORPUS SAYS THE DEPTH-5 MERGE HAPPENS NOWHERE ELSE IN THE
 *       BINARY.  scratchpad/laneM/xj.py / xj2.py / xj3.py scan every matched
 *       body for the cross-jump signature (`push <imm>` immediately followed
 *       by `jmp T` where the instruction before T is also a `push <imm>`) and
 *       measure the SHARED SUFFIX DEPTH of each site (T to the host block's
 *       terminating jmp/ret).  89 sites in 24 functions.  The histogram is
 *           5:1  7:16  9:10  11:1  13:37  15:1  16:2  18:8  27:4  29:1
 *           32:3  40:1  41:1  43:1  53:1  76:1
 *       and the single depth-5 site IS 0x45ff00's own.  There is no depth-6
 *       site either.  So the original's compiler performed, in this one place,
 *       a merge two instructions shallower than anything it did in the other
 *       88 sites of the program, and this build's floor of 6 (measured) is
 *       already BELOW the corpus minimum of 7.  Treat residual (b) as a
 *       build difference, not a spelling we have not found, unless someone
 *       turns up a second depth-5 or depth-6 site.
 *       (iii) Rule 1 of the model above is WRONG as stated and should be
 *       read as "the host is the LAST block IN LAYOUT ORDER OF THE MERGED
 *       GROUP", not "the predecessor the join falls through from".  The
 *       witness is ridecb1.c `Restaurant1_Tick` 0x0042f1a0, matched: three
 *       one-instruction case blocks `push 0 / jmp` , `push 1 / jmp`, `push 2`
 *       falling into a NINE-instruction shared tail that itself ends `jmp
 *       0x42f45a` -- the host is not the join's fall-through predecessor, and
 *       two non-canonical predecessors merge into it.  (Its tail is long
 *       because the varying argument is the callee's LAST parameter, hence
 *       pushed FIRST; RenderCursor's `kind` is the SECOND parameter, which is
 *       what caps our shared suffix at five.)  screens2.c `PrintExitCheckBox`
 *       0x0048f2d0 is a second matched witness (if / else-if / else, depths 13
 *       and 13).
 *       (iv) Newly ruled out as the "sixth instruction that vanishes"
 *       (scratchpad/laneM/rc2.py, all 140/387/0 in the repro): `goto n; n:`
 *       inside each case, `goto` to a label after the if/else, an empty
 *       `for` loop, `col = col;`, `flags = c->flags;`, `x = x + 0;`, a bare
 *       `;`, `if (0) A(...);`, `(void)th;`.  What DOES trigger the merge is
 *       anything that really lengthens the tail: `if (c->count < 0) return;`
 *       (138/381/1) and -- the `add esp` split from the DECOMP lever --
 *       making an argument a call result, `A(&vs,K,x,y,col,Th())` (132/361/1),
 *       `Idn(th)` (136/373/1) or even `(VS*)Idn((int)&vs)` (139/380/1).  All
 *       four are semantically wrong here; they only confirm the mechanism.
 *       (v) For residual (a), newly ruled out 2026-09-04 (lane M; driver
 *       scratchpad/laneM/pv.py, a whole-file patch-batch runner over
 *       bigrender.c).  The note above left "t and tb in ONE aggregate (does
 *       not compile as written -- redo it if you want it)" open: it is now
 *       measured, with every `t.`/`tb.`/`&t`/`&tb` in the body rewritten.
 *       Both member orders and an added pad member give X=367 with first
 *       divergence 7 -- the aggregate goes to the TOP of the frame and re-lays
 *       everything, exactly as the "DO NOT move t/tb into a block scope"
 *       warning predicts.  Do not revisit it.  Also inert, all byte-identical
 *       to the committed body (X=342 / first=48 on pv.py's whole-function
 *       metric): commuting either `view.right`/`view.bottom` addition,
 *       reading g_map directly instead of the `map` local in all four view
 *       assignments, a `void*`/`const Pos*` prototype for GetTileBounds, an
 *       `int` return on it, no prototype at all, TileSprite as a MACRO instead
 *       of a static __inline, commuting TileSprite's index addition, and an
 *       `unsigned int` parameter on it.  Reordering the four view assignments
 *       does move things -- and breaks the prologue (first divergence 5 and 8,
 *       X=359 and 355), so the prologue's statement order is already right.
 *       temp1 is still ALWAYS eax.
 *
 * LEVER already applied (worth 124 mismatches and the ESCAPES failure):
 * `t.x = c->origin.x; t.y = c->origin.y; t.x += x; t.y += y;` -- the
 * separate load-then-accumulate form.  Written `t.x = c->origin.x + x`,
 * VC6 emits `mov edx,esi / add edx,ebx` (two extra instructions per
 * coordinate) and lays the loop out so a branch escapes the extent.
 *
 * 2026-09-04 (fifth lane) -- CONFIRMED, no change, and deliberately not
 * re-ground.  Residual (b) has retirement-grade evidence already recorded
 * here: the corpus scan of every matched body for this cross-jump signature
 * found 89 sites across 24 functions with shared-suffix depths 5, 7, 9, 11,
 * 13, 15, 16, 18 and 27, the ONLY depth-5 site in the whole binary is this
 * function's own, and there is no depth-6 site anywhere -- so this build's
 * measured merge floor of 6 sits below the corpus minimum of 7 and the
 * original merged at 5.  That is a build difference, not an unfound spelling,
 * and the eleven byte-identical `goto join` / `break` spellings inside the
 * switch confirm the goto-flip lever cannot supply the required asymmetry
 * there.  Residual (a) (23 mismatches, a three-cycle rename of the tile
 * loop's scratch temps) is the only part still worth anyone's time, and it is
 * worth 23, not 273: the 273 is (b)'s four-instruction shift propagating
 * through every later index.  Cost any future work on this function against
 * 23, not against 273.
 */
/* Scope I (2026-09-05): at its measured scratch-rotation floor plus
 * the already-retired depth-5 merge limit: 273/454 strict, first 48,
 * 1438/1429 bytes. A named x accumulator, named y accumulator, and separate
 * origin Pos copy are each byte-identical. They do not change the first
 * scratch temporary. No sixth tail instruction or extra callback was added;
 * the documented corpus proof for the five-instruction merge still stands.
 * Full measurements: docs/lanes/scope-i.md.
 */
// WIP-FUNCTION: LEGOLAND 0x0045ff00  (39.9%, 273/454 strict; scratch rotation plus retired merge limit; first 48)
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
                    t.x = c->origin.x;
                    t.y = c->origin.y;
                    t.x += x;
                    t.y += y;
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
