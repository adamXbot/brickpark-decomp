/* LEGOLAND — three rendering routines: the recolouring RLE blitter, the
 * full-map render-order builder, and the CSP sprite loader.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field offsets, global addresses and callee argument counts are load-bearing;
 * names are ours.
 *
 * ---------------------------------------------------------------------------
 * SoftBlitRLE (0x00465ee0) — the RECOLOURING twin of softblit2.c's
 * SoftBlitRLEPlain (0x00466770)
 * ---------------------------------------------------------------------------
 * Same contract, same scratch globals, but no dispatch: where the plain
 * painter picks one of eight specialised loops from the clip geometry and the
 * mouse box, this one always calls the single recolouring frame painter
 * 0x00468040 (the twin of bigrender.c's highlight painter 0x00468410) and
 * lets it do the clipping from the globals.  Consequently it does NOT compute
 * the mouse bounding-box test at all — it only stamps g_sp_mouse_pixel.
 *
 * Everything is drawn into the currently locked DirectDraw surface described
 * by gpu.c's lock descriptor at 0x0066809c: lpSurface (+0x24) is the 16-bpp
 * pixel array and lPitch (+0x10) the row stride in BYTES.  The destination
 * pointer handed to the painter is biased LEFT by the source clip
 *
 *     p = lpSurface + dst->top * lPitch + (dst->left - src->left) * 2
 *
 * so a painter that skips `left` source pixels still lands on dst->left.
 * The four numbers of `src` reach the painter only through the globals
 * g_sp_left / g_sp_w / g_sp_top / g_sp_h.
 *
 * Frame selection is the house rule: g_frame_override (0x004b9ca8) when it is
 * >= 0, else the record's own current frame, clamped to nframes - 1.  With
 * LLSRec flags bit 0 the record opens with a BASE image: frames[0] is painted
 * first, then the record is walked `lls->frame + 1` frames on and that frame
 * is painted too.  THE OVERRIDE BUG IS REPRODUCED: the second pass re-reads
 * lls->frame instead of using the clamped/overridden value, so an override on
 * a base-image RLE sprite paints the base plus the record's own frame rather
 * than the requested one.  softblit2.c records the same bug in SoftBlitAnim
 * and notes that the plain painters get it right.
 *
 * One frame record (bigrender.c's LLSFrame):
 *     +0x00 int   total byte length of this record (add it to walk on)
 *     +0x04 int   count of 16-bit entries in the control block
 *     +0x08 int   byte length of the 16-bit pixel block
 *     +0x10       control block, then the 16-bit block, then the 8-bit block
 * which is why the painter is handed the three pointers
 * (data, data + n16*2, data + n16*2 + n2).
 *
 * ---------------------------------------------------------------------------
 * CalculateFullMapRenderOrder (0x0045a660) — the OVERVIEW-MAP render order
 * ---------------------------------------------------------------------------
 * The full-map variant of objmap2.c's CalculateMapRenderOrder (0x0045a4a0):
 * the same column-by-column scan that threads every base cell into the render
 * chain (Cell +0x06 packed next-coordinate, head at 0x007febb8) and records
 * where the scan resumes in the 0x1000-entry node table at 0x00807f60, but
 * with ONE extra emit rule — a cell whose flags carry 0x0008 and whose obj is
 * the LLIDB element "DRIVING SCHOOL ROADS" also becomes a render node.  So the
 * overview map (renderview.c's RenderFullMap) draws the driving-school road
 * network in the diagonal object order instead of leaving it to the terrain
 * pass.  See the note on the function for the frame-slot and block-layout
 * consequences of the ElemID call.
 *
 * ---------------------------------------------------------------------------
 * LoadCSPSprite (0x004978b0) — the composite-sprite (.csp) loader
 * ---------------------------------------------------------------------------
 * A .csp is a list of ordinary sprites with a per-layer (dx, dy) offset, read
 * out of ".\CompSprite\<name>" into the same 0x24-byte ILF table that
 * LLIDB_LoadILFData builds, and hung off SpriteRec::image with flag 0x8000.
 * The full file format, the automatic LLSPlay of animated layers and the two
 * original bugs (LLIDB_FreeILFTable(0) on the table-allocation failure path,
 * and the unbounded layer-name read into the 0x200-byte path buffer) are
 * documented on the function.
 */
#include "legoland.h"

extern void* memset(void* d, int c, unsigned int n);
#pragma intrinsic(memset)

/* ---- types (shared with bigrender.c / softblit2.c) ---------------------- */

typedef struct WinRect {
    long left;     /* +0x00 */
    long top;      /* +0x04 */
    long right;    /* +0x08 */
    long bottom;   /* +0x0c */
} WinRect;

/* DDSURFACEDESC, 0x6c bytes (gpu.c). */
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
    char          pad28[0x6c - 0x28];  /* +0x28 */
} DDSurfaceDesc;

/* An LLS animation record. */
typedef struct LLSRec {
    short          frame;       /* +0x00 current frame */
    char           pad02[0x0e]; /* +0x02 */
    short          nframes;     /* +0x10 */
    short          pad12;       /* +0x12 */
    unsigned int   flags;       /* +0x14 bit 0 = base image first */
    char           frames[1];   /* +0x18 */
} LLSRec;

/* One frame of an RLE animation record. */
typedef struct LLSFrame {
    int  size;     /* +0x00 */
    int  n16;      /* +0x04 */
    int  n2;       /* +0x08 */
    int  pad0c;    /* +0x0c */
    char data[1];  /* +0x10 */
} LLSFrame;

/* A sprite record (sprite2.c's SpriteRec); only +0x08/+0x10 are touched. */
typedef struct SpriteRec {
    char          pad0[8];       /* +0x00 */
    void*         image;         /* +0x08 ImageRec* / ILF table when flag 0x8000 */
    int           detail;        /* +0x0c */
    unsigned int  flags;         /* +0x10 */
} SpriteRec;

/* A source image record (sprite2.c's ImageRec). */
typedef struct ImageRec {
    void*         lls;           /* +0x00 */
    char          pad4[0x14 - 4];/* +0x04 */
    int           type;          /* +0x14  2/3 = animated */
} ImageRec;

/* The 0x24-byte ILF (indexed layer/frame) table LLIDB_LoadILFData builds
 * and LLIDB_FreeILFTable releases (memdb.c's IlfData). */
typedef struct IlfData {
    int    f00;       /* +0x00 */
    int    count;     /* +0x04 */
    void** sprites;   /* +0x08 */
    int*   dx;        /* +0x0c */
    int*   dy;        /* +0x10 */
    int    f14, f18, f1c, f20;
} IlfData;

/* A render node in the 0x1000-entry table (objmap2.c). */
typedef struct RenderNode {
    int            live;         /* +0x00 */
    unsigned short bpos;         /* +0x04 packed base cell */
    unsigned char  x;            /* +0x06 column to resume at */
    unsigned char  y;            /* +0x07 row to resume at */
} RenderNode;

/* A placed map object: its class sits at +0x0c (objmap2.c). */
typedef struct MapObj {
    char       pad0[0x0c];       /* +0x00 */
    ObjClass*  cls;              /* +0x0c */
} MapObj;

/* ---- globals ------------------------------------------------------------ */
extern DDSurfaceDesc  g_ddsd;               /* 0x0066809c */
extern Pos            g_mouse_point;        /* 0x00813a44 */
extern int            g_frame_override;     /* 0x004b9ca8  (-1 = none) */
extern int            g_sp_rowlen;          /* 0x007fe9a4 */
extern void*          g_sp_mouse_pixel;     /* 0x007fe9a8 */
extern int            g_sp_top;             /* 0x007fea4c */
extern int            g_sp_left;            /* 0x007fea50 */
extern int            g_sp_h;               /* 0x007febac */
extern int            g_sp_w;               /* 0x007febb0 */

extern RenderNode     g_render_nodes[0x1000];/* 0x00807f60 */
extern int            g_render_node_next;   /* 0x00801408 */
extern unsigned short g_render_head;        /* 0x007febb8 */
extern const char     kDrivingSchoolRoads[];/* 0x004b89cc "DRIVING SCHOOL ROADS" */
extern const char     kCompSpritePathFmt[]; /* 0x004bfec8 ".\\CompSprite\\%s" */

/* ---- callees ------------------------------------------------------------ */
/* 0x00468040 (not exported): paint one RLE frame with the recolour mask.
 * The twin of bigrender.c's highlight painter at 0x00468410. */
extern void SoftBlitRLEFrameRecolour(void* dst, void* ctrl, void* p16, void* p8,
                                     int h, int pitch, int top, int left, int w,
                                     int zero, void* mouse); /* 0x00468040 */
extern void* ElemID(const char* name);                         /* 0x0047b3f0 */
extern void  TakeRenderNodeByPos(unsigned short bpos, Pos* out); /* 0x0045a430 */
extern void  TakeRenderNodeInColumn(Pos* p);                   /* 0x0045a3e0 */
extern int   sprintf(char* buf, const char* fmt, ...);         /* 0x0049e573 */
extern void* HeapAlloc_w(unsigned int size);                   /* 0x0049e4ff */
extern void* RES_OpenFile(const char* path);                   /* 0x00489b60 */
extern int   RES_ReadFile(void* f, void* buf, int n);          /* 0x00489cf0 */
extern int   RES_CloseFile(void* f);                           /* 0x00489de0 */
extern int   LLIDB_FreeILFTable(IlfData* t);                   /* 0x0047bef0 */
extern void  LLSPlay(void* lls, void* anim);                   /* 0x0047d520 */
extern SpriteRec* LoadSprite(const char* name, int kind);      /* 0x00497ab0 */

/* The bounds-checked cell fetch every map accessor open-codes (objmap.c). */
static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

/* =========================================================================
 *  SoftBlitRLE — ImageRec::type 3, recolouring path
 * ========================================================================= */

static __inline void DrawRLEFrame(void* dst, LLSFrame* f)
{
    SoftBlitRLEFrameRecolour(dst, f->data, f->data + f->n16 * 2,
                             f->data + f->n16 * 2 + f->n2, g_sp_h,
                             g_ddsd.lPitch, g_sp_top, g_sp_left, g_sp_w, 0,
                             g_sp_mouse_pixel);
}

/* CODEGEN LEVER, transferred from bigrender.c's RenderSpriteX (lever 6) and
 * confirmed here on a DIFFERENT site: exactly one of the three painter calls
 * must be written out with `f->n16` read into a named local.  Through the
 * inline helper the two `f->n16` reads are CSE'd into a compiler TEMPORARY
 * which the scheduler treats as a critical-path root and hoists — with its
 * `lea` — to the top of the block, ahead of the g_sp_mouse_pixel load.  That
 * costs 14 here, and it also makes the block's p8 chain land in the same
 * registers as the else arm's, so VC6 cross-jumps FOUR MORE instructions into
 * the shared tail than the original does (our body came out 137 instructions
 * against 141).  In RenderSpriteX the site that needs the local is the third
 * (the else arm); here it is the SECOND (the post-walk call in the
 * base-image arm) — i.e. the site whose block begins after a call, where the
 * reload of the scratch globals is what the hoist jumps over.  Test per site,
 * never per function. */
// FUNCTION: LEGOLAND 0x00465ee0
void SoftBlitRLE(LLSRec* lls, WinRect* src, WinRect* dst)
{
    LLSFrame*    f;
    int          frame;
    int          n;
    unsigned int k;
    char*        p;

    g_sp_rowlen = g_ddsd.lPitch;
    f = (LLSFrame*)lls->frames;
    g_sp_left = src->left;
    g_sp_w = src->right - src->left;
    g_sp_top = src->top;
    g_sp_h = src->bottom - src->top;
    n = g_frame_override;
    if (n < 0)
        n = lls->frame;
    frame = n;
    if (frame >= lls->nframes)
        frame = lls->nframes - 1;
    g_sp_mouse_pixel = (char*)g_ddsd.lpSurface + g_mouse_point.y * g_ddsd.lPitch
                     + g_mouse_point.x * 2;
    p = (char*)g_ddsd.lpSurface + dst->top * g_ddsd.lPitch
      + (dst->left - g_sp_left) * 2;
    if (lls->flags & 1) {
        DrawRLEFrame(p, f);
        k = lls->frame + 1;   /* the override bug: re-reads the record */
        while (k-- != 0)
            f = (LLSFrame*)((char*)f + f->size);
        {
            int nn = f->n16;
            SoftBlitRLEFrameRecolour(p, f->data, f->data + nn * 2,
                                     f->data + nn * 2 + f->n2, g_sp_h,
                                     g_ddsd.lPitch, g_sp_top, g_sp_left,
                                     g_sp_w, 0, g_sp_mouse_pixel);
        }
    } else {
        n = frame;
        while (n--)
            f = (LLSFrame*)((char*)f + f->size);
        DrawRLEFrame(p, f);
    }
}


/* =========================================================================
 *  CalculateFullMapRenderOrder — the overview-map variant of objmap2.c's
 *  CalculateMapRenderOrder (0x0045a4a0)
 * ========================================================================= */

/* Instruction for instruction the same scan as CalculateMapRenderOrder, with
 * ONE extra rule and the frame slot it costs: a cell also becomes a render
 * node when it carries flag 0x0008 AND its obj is the LLIDB element
 * "DRIVING SCHOOL ROADS".  Ordinary rendering leaves those tiles to the
 * terrain pass; the full-map render (renderview.c's RenderFullMap, which
 * paints the whole park into the 640x340 map-screen sprite) wants them in the
 * diagonal object order so the road network is drawn over the ground.  The
 * ElemID handle is looked up once, into a local, and lives in the frame slot
 * between `link` and `p`.
 *
 * Because that call sits between the `p.x = 0` store and the loop guard, p.x
 * cannot stay in a register across it: the guard reads the home slot
 * (`cmp ecx,eax`) where the sibling can test the still-live zero
 * (`test eax,eax`), and VC6 then keeps the zero register live into a
 * DUPLICATED exit block, so the terminator store exists twice — `*link = 0`
 * as an immediate on the loop-exit path and as `mov word ptr [esi],bp` on the
 * guard-failed path.  Both are the one `*link = 0;` statement.
 *
 * The three levers objmap2.c records for the sibling all transfer unchanged:
 * the emit body written out TWICE so VC6 tail-merges it between the two
 * tests; the render-node store order bpos, x, y, live; and the setup order
 * p.x / p.y / node_next / memset.  The one that does NOT transfer is where
 * `link` is assigned: the sibling wants it LAST, this one wants it FIRST,
 * because the ElemID call in between is a scheduling barrier the sibling does
 * not have — written last the pair `mov esi,0x7febb8 / mov [esp+0x10],esi`
 * sinks past the loop guard (2 mismatches, the whole residual). */
// FUNCTION: LEGOLAND 0x0045a660
void CalculateFullMapRenderOrder(void)
{
    Pos             p;
    unsigned short* link;
    Cell*           cell;
    Cell*           base;
    ObjClass*       def;
    RenderNode*     node;
    void*           elem;
    int             bx, by;

    link = &g_render_head;
    p.x = 0;
    p.y = 0;
    elem = ElemID(kDrivingSchoolRoads);
    g_render_node_next = 0;
    memset(g_render_nodes, 0, sizeof(g_render_nodes));
    while (p.x < g_map->width) {
        cell = MapCellAt(p.x, p.y);
        if (!(cell->flags & 0xa0)
         && !((cell->flags & 8) && cell->obj == elem)) {
            p.y++;
        } else {
            bx = cell->bx;
            by = cell->by;
            base = MapCellAt(bx, by);
            node = &g_render_nodes[g_render_node_next];
            def = ((MapObj*)base->obj)->cls;
            g_render_node_next++;
            if (g_render_node_next == 0x1000)
                g_render_node_next = 0;
            node->bpos = *(unsigned short*)&cell->bx;
            node->x = (unsigned char)p.x;
            node->y = (unsigned char)(def->rect.bottom + by + 1);
            node->live = 1;
            /* Written out twice on purpose — see objmap2.c's note. */
            if (p.x == def->rect.right + bx) {
                *link = *(unsigned short*)&base->bx;
                link = (unsigned short*)&base->nx;
                TakeRenderNodeByPos(*(unsigned short*)&cell->bx, &p);
            } else if (p.x == g_map->width - 1) {
                *link = *(unsigned short*)&base->bx;
                link = (unsigned short*)&base->nx;
                TakeRenderNodeByPos(*(unsigned short*)&cell->bx, &p);
            } else {
                p.x++;
                TakeRenderNodeInColumn(&p);
            }
        }
        while (p.y >= g_map->height) {
            p.x++;
            TakeRenderNodeInColumn(&p);
        }
    }
    *link = 0;
}


/* =========================================================================
 *  LoadCSPSprite — build a composite (.csp) sprite over an empty record
 * ========================================================================= */

/* sprite2.c's LoadSprite dispatches a ".csp" name here.  A CSP is a COMPOSITE
 * sprite: a list of ordinary sprites with a per-layer (dx, dy) offset, held in
 * the 0x24-byte ILF table (memdb.c's IlfData) that becomes SpriteRec::image
 * with flag 0x8000 set — the same representation LLIDB_LoadILFData produces,
 * which is why LLIDB_FreeILFTable can release it.
 *
 * FILE FORMAT (".\CompSprite\<name>", read through the RES volume layer):
 *     u16  layer count
 *     u16  (read and discarded)
 *     u32  n, then n bytes (read and discarded — a header/comment block)
 *     count * { s32 dx; s32 dy; }        the layer offsets, dx and dy
 *                                        interleaved per layer
 *     count * { u32 n; char name[n]; }   the layer sprite names, NOT
 *                                        NUL-terminated in the file: the
 *                                        loader terminates them itself
 * Each layer name is loaded with LoadSprite (recursively — the `kind` is
 * passed straight down), and a layer whose image is an animation (ImageRec
 * type 2 or 3) is STARTED immediately with LLSPlay, so a composite sprite's
 * animated parts run without the caller doing anything.
 *
 * ORIGINAL BUGS, both reproduced:
 *  - the three layer arrays are allocated before any of them is checked, and
 *    the failure path calls LLIDB_FreeILFTable, so a partial allocation is
 *    cleaned up correctly — but the allocation of the TABLE itself is checked
 *    by jumping to that same label, which calls LLIDB_FreeILFTable(0).  That
 *    is harmless only because the callee tolerates a null table.
 *  - both the ".csp" path and every layer name are read into the SAME 0x200
 *    byte buffer, and the name length comes from the file with no bound
 *    check: a layer name of 512 bytes or more overruns the frame.  The
 *    terminator store `path[len] = 0` is the overrun's first write.
 * There is no check that the layer count matches the number of name records,
 * and no check on RES_ReadFile's return value anywhere. */
/* CODEGEN LEVER: the three allocation checks must be ONE `||` chain, not three
 * separate `if (!x) goto fail;` statements.  Written separately the first two
 * branch forward as the original does but the THIRD pulls the shared `fail`
 * block inline behind it (13 mismatches, and the body still comes out 169
 * instructions).  The short-circuit chain exiles the block past the whole
 * function, which is the recorded exile rule applied in the direction that
 * KEEPS a block out of line rather than bringing it in. */
// FUNCTION: LEGOLAND 0x004978b0
int LoadCSPSprite(SpriteRec* s, const char* name, int kind)
{
    int      count;
    int      len;
    short    skip;
    char     path[0x200];
    char     block[0x200];
    void*    f;
    IlfData* t;
    int      i;

    count = 0;
    if (!s)
        goto out0;
    sprintf(path, kCompSpritePathFmt, name);
    f = RES_OpenFile(path);
    if (!f)
        goto out0;
    t = (IlfData*)HeapAlloc_w(0x24);
    if (!t)
        goto fail;
    RES_ReadFile(f, &count, 2);
    RES_ReadFile(f, &skip, 2);
    t->dx = (int*)HeapAlloc_w(count * 4);
    t->dy = (int*)HeapAlloc_w(count * 4);
    t->sprites = (void**)HeapAlloc_w(count * 4);
    t->count = count;
    if (!t->dx || !t->dy || !t->sprites)
        goto fail;
    RES_ReadFile(f, &len, 4);
    RES_ReadFile(f, block, len);
    for (i = 0; i < count; i++) {
        RES_ReadFile(f, &t->dx[i], 4);
        RES_ReadFile(f, &t->dy[i], 4);
    }
    for (i = 0; i < count; i++) {
        ImageRec* im;
        RES_ReadFile(f, &len, 4);
        RES_ReadFile(f, path, len);
        path[len] = 0;   /* no bound check -- see the note above */
        t->sprites[i] = LoadSprite(path, kind);
        if (t->sprites[i]) {
            im = (ImageRec*)((SpriteRec*)t->sprites[i])->image;
            if (im->type == 2 || im->type == 3)
                LLSPlay(im->lls, im);
        }
    }
    RES_CloseFile(f);
    s->image = t;
    s->flags |= 0x8000;
    return 1;
fail:
    LLIDB_FreeILFTable(t);
    RES_CloseFile(f);
out0:
    return 0;
}
