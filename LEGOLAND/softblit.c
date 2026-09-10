/* LEGOLAND -- the CPU rasterisers: the plain (non-recolouring) sprite blitter
 * that every RenderSprite falls back to when DirectDraw cannot do the job,
 * the recolouring RLE animation painter, and the objective-hint text pump.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field offsets, global addresses and callee arg counts are load-bearing;
 * names are ours. No legoland.h: every type is defined locally.
 *
 * ---------------------------------------------------------------------------
 * The locked-surface contract (what a browser runtime has to reproduce)
 * ---------------------------------------------------------------------------
 * Every software blitter here draws into the CURRENTLY LOCKED DirectDraw
 * surface described by the lock descriptor at 0x0066809c (gpu.c's LockState):
 *   g_ddsd.lpSurface  base address of the 16-bpp pixel array
 *   g_ddsd.lPitch     bytes per row (NOT pixels: the row step is lPitch, the
 *                     pixel step 2)
 * The pixel format is 16 bits per pixel throughout; the blitters never look
 * at the channel masks, they only compare against and copy whole 16-bit
 * words.  Callers bracket a blit with surface.c's lock/unlock status stack.
 *
 * Both loops here are hand-written __asm in the original, which is why every
 * function in this file has an ebp frame and a full ebx/esi/edi save (an
 * __asm block forces both).  text.c's SoftPrint_Clear is the same house
 * style; bigrender.c's SoftPrint_XBltFast is the recolouring sibling of
 * SoftBlitSprite.
 *
 * ---------------------------------------------------------------------------
 * The SoftPrint scratch globals (0x007fe998 .. 0x007febb0)
 * ---------------------------------------------------------------------------
 * These are NOT re-entrant: the blitters park their working set in fixed
 * globals so the __asm can address it absolutely.
 *   0x007fe998  g_sp_recolour     16-bit AND mask applied to every pixel
 *   0x007fe9a4  g_sp_rowlen       pixels per output row (also the row pitch
 *                                 while an RLE frame is being painted)
 *   0x007fe9a8  g_sp_mouse_pixel  address of the surface pixel under the mouse
 *   0x007fea10  g_sp_rows_left    rows still to paint (RLE loop)
 *   0x007fea14  g_sp_height       source height
 *   0x007fea18  g_sp_hit_armed    byte: the run being painted STARTS at or
 *                                 before the mouse pixel
 *   0x007fea1c  g_sp_width        source row pitch in BYTES
 *   0x007fea20  g_sp_pal16        8-bit -> 16-bit palette (256 u16 entries)
 *   0x007fea40  g_sp_pixels       source bitmap
 *   0x007fea44  g_transparent_colour
 *   0x007fea4c  g_sp_top          source rect top
 *   0x007fea50  g_sp_left         source rect left
 *   0x007feb14  g_blit_hit        set to 1 when a painted run covered the
 *                                 mouse pixel (printlist.c's hit token)
 *   0x007feb18  g_zb_row          address of the current output row
 *   0x007febac  g_sp_h            source rect height
 *   0x007febb0  g_sp_w            source rect width
 *   0x00668160  g_zb_bits         nibbles left in the RLE control word
 *
 * THE MOUSE HIT TEST.  g_sp_mouse_pixel is the address of the surface pixel
 * the cursor is over.  A run painter arms g_sp_hit_armed when its output
 * pointer has not yet passed that address (`setbe`), and after the run
 * `or g_blit_hit, (edi > mouse) & armed` -- i.e. the run straddled the mouse
 * pixel, so the sprite is what the pointer is on.  printlist.c's PrintSprite
 * reads g_blit_hit to decide which gadget the mouse is over.  Note the test
 * is address-based, so it also fires for a pixel on a LATER row that happens
 * to share the address -- it cannot, because a run never spans rows.
 * ------------------------------------------------------------------------- */

/* ---- local types -------------------------------------------------------- */

typedef struct WinRect {
    long left;     /* +0x00 */
    long top;      /* +0x04 */
    long right;    /* +0x08 */
    long bottom;   /* +0x0c */
} WinRect;

typedef struct Pos { int x, y; } Pos;

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

/* The lock descriptor and the render clip are ONE object (gpu.c). */
typedef struct LockState {
    DDSurfaceDesc ddsd;         /* +0x00 0x0066809c */
    WinRect       render_clip;  /* +0x6c 0x00668108 */
} LockState;

typedef struct DDSurface DDSurface;

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
 * 1 = 16-bit raw, 2 = LLS animation, 3 = RLE frame list. */
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

/* An LLS animation record (bigrender.c). */
typedef struct LLSRec {
    short          frame;       /* +0x00 current frame */
    char           pad02[0x0e]; /* +0x02 */
    short          nframes;     /* +0x10 */
    short          pad12;       /* +0x12 */
    unsigned int   flags;       /* +0x14 bit 0 = base image first */
    char           frames[1];   /* +0x18 */
} LLSRec;

/* One frame of an LLS animation record: a length-linked variable record.
 * `body` is the 8-bit palette-index block; the 32-bit control stream follows
 * it (frame 0 has the shared 0x200-byte u16 palette in between). */
typedef struct AnimFrame {
    int  size;     /* +0x00 byte length; add to walk to the next frame */
    int  npixels;  /* +0x04 byte length of the index block */
    char body[1];  /* +0x08 */
} AnimFrame;

/* ---- globals ------------------------------------------------------------ */

extern LockState      g_lock;               /* 0x0066809c */
#define g_ddsd        g_lock.ddsd

extern int            g_frame_override;     /* 0x004b9ca8  (-1 = none) */
extern int            g_sp_recolour;        /* 0x007fe998 */
extern int            g_sp_rowlen;          /* 0x007fe9a4 */
extern void*          g_sp_mouse_pixel;     /* 0x007fe9a8 */
extern int            g_sp_rows_left;       /* 0x007fea10 */
extern int            g_sp_height;          /* 0x007fea14 */
extern unsigned char  g_sp_hit_armed;       /* 0x007fea18 */
extern int            g_sp_width;           /* 0x007fea1c */
extern void*          g_sp_pal16;           /* 0x007fea20 */
extern void*          g_sp_pixels;          /* 0x007fea40 */
extern int            g_transparent_colour; /* 0x007fea44 */
extern int            g_sp_top;             /* 0x007fea4c */
extern int            g_sp_left;            /* 0x007fea50 */
extern int            g_blit_hit;           /* 0x007feb14 */
extern void*          g_zb_row;             /* 0x007feb18 */
extern int            g_sp_h;               /* 0x007febac */
extern int            g_sp_w;               /* 0x007febb0 */
extern int            g_zb_bits;            /* 0x00668160 */

/* The input/cursor block at 0x00813a40: the mouse point at +0x04. */
extern Pos            g_mouse_point;        /* 0x00813a44 */

extern const char     g_msg_bad_image[];    /* 0x004b9d0c "BltFast:Sprite Not Available:%s\n" */

/* ---- externals ---------------------------------------------------------- */

__declspec(dllimport) int __stdcall IsBadReadPtr(const void* p, unsigned int n); /* [0x4ab11c] */

extern int   GetSprite(SpriteHandle* out, SpriteRec* s);  /* 0x00497c30 */
extern int   ReleaseSprite(SpriteHandle* h);              /* 0x00497dc0 */
extern void  DebugPrintf(const char* fmt, ...);           /* 0x0047f870 */
/* The two plain (non-recolouring) animation painters -- the siblings of
 * bigrender.c's SoftBlitAnim (0x465240) / SoftBlitRLE (0x465ee0), which the
 * recolouring SoftPrint_XBltFast uses instead. */
extern void  SoftBlitAnimPlain(void* lls, WinRect* src, Pos* dst); /* 0x00464480 */
extern void  SoftBlitRLEPlain(void* lls, WinRect* src, Pos* dst);  /* 0x00466770 */

/* -------------------------------------------------------------------------
 * 0x00464ee0 -- __fastcall(ecx = sprite, edx = &src, stack = &dst): blit the
 * sprite-space rectangle `src` to the surface at `dst->x/dst->y`.  This is
 * the plain path: no recolour mask, no clipping of its own (RenderSprite has
 * already intersected `dst` with the clip rect and rebased `src`).
 *
 * What it does, in order:
 *  1. Pick the image.  A function-drawn sprite (flag 0x20) has no ImageRec:
 *     GetSprite() locks its own surface and hands back {pitch, w, h, pixels},
 *     and a SYNTHETIC 16-bpp ImageRec is built on the stack over it, with
 *     w = pitch/2 so the row step below comes out as the real pitch.  A
 *     normal sprite uses s->image, guarded by IsBadReadPtr on its pixels
 *     (a missing .lls prints "BltFast:Sprite Not Available:<name>").
 *  2. Bias `src` by the sprite's atlas origin (s->src_x/src_y): sprites are
 *     packed into shared surfaces, so sprite space is not surface space.
 *     NOTE this mutates the caller's rectangle in place.
 *  3. Stamp g_sp_mouse_pixel = lpSurface + lPitch*mouse.y + mouse.x*2.
 *  4. Dispatch on ImageRec::type: 2 and 3 are animation/RLE records handed
 *     to their own painters; 0 is an 8-bpp bitmap with a 4-byte-aligned row
 *     pitch ((w + 3) & ~3) and a 256-entry u16 palette at pal + 4; anything
 *     else is a raw 16-bpp bitmap with a row pitch of w * 2.
 *  5. Release the GetSprite lock if one was taken.
 *
 * Both pixel loops are the same shape: walk (src.right - src.left) pixels
 * per row for (src.bottom - src.top) rows, skip any pixel whose 16-bit value
 * equals g_transparent_colour (that IS the transparency -- there is no alpha
 * anywhere in the engine), advance the destination by lPitch - 2*rowlen and
 * the source by its own pitch - rowlen at the end of each row.  Unlike
 * SoftPrint_XBltFast neither loop ANDs with g_sp_recolour.
 *
 * The 16-bpp loop is unrolled four ways with `loop` closing it; its fourth
 * copy branches to the SECOND copy's skip label to share the pointer bump,
 * which is why the unroll looks malformed.  It is not a bug: every copy
 * handles exactly one pixel and performs exactly one `dec ecx`, so pixels
 * are still visited in order, once each.
 *
 * The 8-bpp loop keeps a dead `mov ebp, eax` (a leftover from the recolour
 * version) inside the inner loop and pushes/pops the two row steps around
 * the palette pointer every row -- reproduced verbatim.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00464ee0
void __fastcall SoftBlitSprite(SpriteRec* s, WinRect* src, Pos* dst)
{
    ImageRec     fake;
    SpriteHandle handle;
    ImageRec*    image;

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
    src->left += s->src_x;
    src->top += s->src_y;
    src->right += s->src_x;
    src->bottom += s->src_y;
    g_sp_mouse_pixel = (char*)g_ddsd.lpSurface + g_mouse_point.y * g_ddsd.lPitch
                     + g_mouse_point.x * 2;
    if (image->type == 2) {
        SoftBlitAnimPlain(image->lls, src, dst);
        return;
    } else if (image->type == 3) {
        SoftBlitRLEPlain(image->lls, src, dst);
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
        ll_blit8(g_ddsd.lpSurface, g_ddsd.lPitch, dst->x, dst->y, g_sp_pixels, g_sp_width,
                 src->left, src->top, src->right, src->bottom, g_sp_pal16,
                 g_transparent_colour, 0xffff);
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
        pixA:
            movzx   eax, word ptr [esi]
            cmp     eax, g_transparent_colour
            je      skipA
            mov     word ptr [edi], ax
        skipA:
            add     esi, 2
            add     edi, 2
            dec     ecx
            je      rowend
            movzx   eax, word ptr [esi]
            cmp     eax, g_transparent_colour
            je      skipB
            mov     word ptr [edi], ax
        skipB:
            add     esi, 2
            add     edi, 2
            dec     ecx
            je      rowend
            movzx   eax, word ptr [esi]
            cmp     eax, g_transparent_colour
            je      skipC
            mov     word ptr [edi], ax
        skipC:
            add     esi, 2
            add     edi, 2
            dec     ecx
            je      rowend
            movzx   eax, word ptr [esi]
            cmp     eax, g_transparent_colour
            je      skipB
            mov     word ptr [edi], ax
            add     esi, 2
            add     edi, 2
            loop    pixA
        rowend:
            add     esi, ebp
            add     edi, ebx
            dec     edx
            jne     row16
            popad
        }
#else
        g_sp_rowlen = (int)(src->right - src->left);
        ll_blit16(g_ddsd.lpSurface, g_ddsd.lPitch, dst->x, dst->y, g_sp_pixels, g_sp_width,
                  src->left, src->top, src->right, src->bottom,
                  g_transparent_colour, 0xffff);
#endif
    }
    if (s->flags & 0x20)
        ReleaseSprite(&handle);
}

/* -------------------------------------------------------------------------
 * 0x00465240 -- paint one frame of an LLS animation record (ImageRec::type
 * 2) into the locked surface, recoloured through g_sp_recolour.  This is the
 * one SoftPrint_XBltFast uses; 0x00464480 is the plain sibling that
 * SoftBlitSprite calls.  cdecl(lls, src, dst).
 *
 * ---------------------------------------------------------------------------
 * The animation record and its frames (recovered here)
 * ---------------------------------------------------------------------------
 *   LLSRec: +0x00 short current frame, +0x10 short frame count,
 *           +0x14 dword flags (bit 0 = the list starts with a BASE image
 *           that must be painted underneath the selected frame),
 *           +0x18 the frame list.
 *   AnimFrame: +0x00 int total byte length (add it to reach the next frame),
 *              +0x04 int byte length of the 8-bit INDEX block,
 *              +0x08 the index block itself, then
 *                    -- FRAME 0 ONLY -- a 0x200-byte, 256-entry u16 palette,
 *                    then the 32-bit control stream.
 * So frame 0 owns the palette and every later frame borrows it: the single-
 * pass path stamps g_sp_pal16 from frames[0] BEFORE walking to the selected
 * frame, and only re-stamps it when frame 0 is itself the one being drawn.
 *
 * Frame selection is the house rule: g_frame_override (0x004b9ca8) when it
 * is >= 0, else the record's own current frame, clamped to nframes - 1.
 * With flags bit 0 set the function runs TWICE -- pass 0 draws frame 0 (the
 * base image, whose data block starts 0x200 further on, past the palette)
 * and pass 1 draws frames[lls->frame + 1].  Note pass 1 walks from the
 * record's OWN frame index, not from the possibly-overridden one, and the
 * single-pass path likewise tests lls->frame (not the selected frame) to
 * decide whether to skip the palette -- reproduce both, they are the
 * original's behaviour and an override on a base-image sprite really does
 * read the wrong block.
 *
 * ---------------------------------------------------------------------------
 * The control stream
 * ---------------------------------------------------------------------------
 * ebp walks 32-bit control words, ebx holds the word being consumed and
 * g_zb_bits how many 2-bit codes are left in it (16 per word; the count is
 * reloaded as 0x10 and decremented, or dropped by 4 when an 8-bit literal
 * count is spliced out with `shrd`).  A code is the low two bits of ebx
 * after `shr ebx,2`:
 *      bit1 = 0            one literal pixel: take the next index byte
 *      bit1 = 1, bit0 = 0  one transparent pixel: just step the output
 *      bit1 = 1, bit0 = 1  an 8-bit COUNT follows in the stream; a count of
 *                          zero ends the row.  A second 2-bit code then says
 *                          what the count means:
 *                            bit1 = 1            skip `count` pixels
 *                            bit1 = 0, bit0 = 0  copy `count` index bytes
 *                            bit1 = 0, bit0 = 1  repeat ONE index byte
 *                                                `count` times (the RLE run)
 * Every emitted pixel is `palette[index] & g_sp_recolour`; there is no
 * transparency test in the paint loops because transparency is encoded in
 * the control stream itself.
 *
 * Three passes over the stream per frame: skip src->top whole rows, then per
 * row skip src->left pixels (with the sub/ja/neg fix-up that splits a run
 * straddling the left edge), paint src width pixels, then run the stream on
 * to the end of the row.  edx is the pixel budget throughout, edi the output
 * pointer, and g_zb_row/g_sp_rows_left carry the row base and the row count
 * across the inner loops.
 *
 * The mouse hit test: before each painted run `setbe`/`sete` arms
 * g_sp_hit_armed if edi has not yet passed g_sp_mouse_pixel, and after the
 * run `g_blit_hit |= (edi > mouse) & armed`.
 * ------------------------------------------------------------------------- */

/* RESIDUAL (measured with tools/audit.py): 407/415 instructions, 1544/1544
 * bytes -- the byte length is EXACT and every instruction is the right
 * instruction in the right place.  The only difference is that two frame
 * homes are swapped: the original puts `npasses` at [ebp-0xc] and `data` at
 * [ebp-8]; we get `npasses` at [ebp-8] and `data` at [ebp-0xc].  That is the
 * whole of the 8 mismatching indices (10, 27, 28, 38, 57, 84, 107, 404) --
 * each is a correct instruction with a displacement 4 bytes out.  The frame
 * is otherwise identical: [ebp-0x18] the hoisted `lls->frames`, [ebp-0x14]
 * pass, [ebp-0x10] ctrl, [ebp-4] frame, size 0x18.
 * Ruled out (every one byte-identical to what we already emit, so do not
 * spend time here again): ALL 120 permutations of the five locals'
 * declaration order (declaration order is genuinely irrelevant for this
 * class, as DECOMP says); `register` on each of the five and on all three
 * ints together; void* vs char* for ctrl and data singly and together;
 * int/long/unsigned for npasses, unsigned for pass, unsigned for n16;
 * block-scoping the loop locals; `data = 0;` / `ctrl = 0;` / `frame = 0;`
 * priming stores; renaming the variables; `dword ptr ctrl` in the asm;
 * for/while/do-while forms of the pass loop and `npasses > pass` ordering;
 * a separate frame-walk pointer in the third arm; an extra register-only
 * local; deriving data from ctrl instead of f->body; and moving `npasses =
 * 1` / `g_sp_rowlen = ...` around (those DO change the output, but by
 * rescheduling the prologue -- 400+ mismatches -- not by moving the homes).
 * The one suggestive pattern left: in the original both POINTER locals sit
 * at 8-byte-aligned offsets (-0x10 and -8) with the four ints at -4, -0xc,
 * -0x14, -0x18; ours packs ctrl/data adjacent at -0x10/-0xc.  If a later
 * agent finds what makes VC6 8-align a spilled pointer home, that is the
 * lever.
 * Re-tested this round and also inert: swapping the `npasses`/`data`
 * declarations (confirming again that declaration order is irrelevant for
 * this class), `unsigned char*` for data, `unsigned` npasses, a named local
 * for the hoisted `lls->frames`, and a `while` form of the pass loop.  What
 * DOES move the homes is the ORDER in which `ctrl` and `data` are assigned
 * inside the three arms: 20 combinations of (arm 1 order x basepal order x
 * arm 3 order) were measured and they produce exactly three layouts, all
 * rotations of {ctrl, npasses, data} in the -0x10/-0xc/-8 run:
 *     ctrl, data, npasses   (what we emit, from the committed spelling)
 *     data, npasses, ctrl   (assign data before ctrl in arm 1)
 *     ctrl, npasses, data   (THE ORIGINAL -- not reached by any of the 20)
 * so the assignment order in the arms is the right dial; the combination
 * that lands the third rotation has not been found. */
/* MEASURED THIS ROUND.  All 8 mismatches are ONE swap of two frame homes.
 * The original's home map, from the ebp frame:
 *     [ebp-0x04] frame   [ebp-0x08] data    [ebp-0x0c] npasses
 *     [ebp-0x10] ctrl    [ebp-0x14] pass    [ebp-0x18] f
 * ours puts npasses at -8 and data at -0xc; every other slot, and every other
 * instruction, matches.  It is NOT a declaration-order effect: all orderings
 * of the six locals (including moving data and/or ctrl into the loop's block
 * scope, and moving either to the end) emit byte-identical code, and so do
 * `char*` instead of `void*` for data/ctrl, `unsigned int` for npasses, a dead
 * `data = 0;` before or after `npasses = 1;` (VC6 deletes the store but the
 * home does not move), an extra unused local, and swapping the `n16 = ...` /
 * `ctrl = ...` / `data = ...` statement order inside each of the three
 * branches.  Reference counts do not explain it either: the original gives the
 * NEARER slot (-8) to `data`, which it touches 3 times, over `npasses`, which
 * it touches 5.  Whatever orders VC6's home pool here is derived from the
 * flow graph, not from anything spellable in the declarations -- that is the
 * open question, and it is worth answering once because the same swap is the
 * whole residual. */
/* THIS ROUND.  softblit2.c's SoftBlitAnimPlain (0x00464480), the plain sibling
 * of this function, is now matched at 100%, and its five pool homes come out in
 * exactly the order this one wants:  f, pass, ctrl, npasses, data (ascending).
 * The only structural difference between the two is that AnimPlain's `frame`
 * lands in the DEAD `lls` parameter slot -- AnimPlain never touches `lls`
 * inside the pass loop, so the parameter home is free -- whereas here `lls` is
 * re-read in arms 1 and 3 (`lls->frame`), its home stays live, and `frame`
 * takes a pool slot at -4.  Adding `frame` at the head of the pool is what
 * flips data and npasses: without it we get the original's order, with it we
 * get the reverse.  That is a real clue: whatever orders the pool is sensitive
 * to how many entries precede these two, not to how they are spelled.
 * Also measured inert this round (all byte-identical to the committed body):
 * five more declaration orders (confirming again that declaration order does
 * nothing here); `npasses = 1` moved after the clamp, spelled as an if/else,
 * as a ternary, or with g_sp_rowlen first; routing `frame` through a register
 * temp (`n = g_frame_override; if (n < 0) n = lls->frame; frame = n;`);
 * flipping arm 1's `goto basepal` polarity; giving arm 3 an `n16` temp;
 * reordering ctrl/data/n16/g_sp_pal16 inside arms 2 and 3; and every
 * post-decrement (`while (n--)`) spelling of both frame walks -- the walk
 * itself already matches here because `frame` is dead after arm 1's loop, so
 * VC6 uses its register in place and needs no copy at all. */
/* THIS ROUND: the aggregate-pinning lever was tried properly and it CAN order
 * the six homes correctly, but it cannot keep the frame at 0x18, so it is not
 * shippable.  Facts, all measured:
 *  * A local struct that the __asm block references (`q.ctrl` / `q.data`) stays
 *    a real aggregate, is laid out in DECLARATION order and is placed at the
 *    BOTTOM of the frame; a struct the asm does not reference is scalar-
 *    replaced and its members go back into the pool individually.
 *  * `struct { void* ctrl; int npasses; void* data; } q;` therefore lands
 *    exactly as the original wants those three relative to each other, with the
 *    remaining scalars pooled ABOVE it in the order f, pass, frame -- also the
 *    original's relative order.  The problem is that aggregates go to the
 *    bottom, so f and pass end up above ctrl instead of below it.
 *  * Putting all six in ONE 24-byte struct in the order f, pass, ctrl, npasses,
 *    data, frame produces the original's home map EXACTLY -- but shifted 4
 *    bytes, because VC6 then emits `sub esp,0x1c` instead of `sub esp,0x18`.
 *    An aggregate of 16 bytes or more that is not asm-referenced costs a spare
 *    4-byte slot at the top of the frame (measured at 16, 20, 24 and 28 bytes;
 *    only the 12-byte asm-referenced `q` packs exactly).  Two aggregates
 *    ({f,pass} + {ctrl,npasses,data}) do not help: the non-asm one is scalar-
 *    replaced and the frame is 0x1c again.
 * Also inert this round: five more npasses spellings (ternary, if/else,
 * `npasses += (lls->flags & 1)`, and moving `npasses = 1` after g_sp_rowlen or
 * before the override test -- the last two cost 400+), `1 == npasses`,
 * `npasses < 2`, `npasses != 2`, `npasses > pass` in the loop condition,
 * `&f->body[n16]`, deriving arm 2's data from g_sp_pal16, declaring f/n16/n/k
 * inside the pass loop's block (they are all pool entries either way, so block
 * scope changes nothing), swapping the npasses/data declarations again, and
 * renaming the locals.  The pool order here is decided by neither declaration
 * order, block scope, nor spelling. */
/* CLOSED (2026-09-03, softblit lane): the swapped npasses/data homes were a
 * CONTROL-FLOW effect after all.  Arm 1 was written with `goto basepal` into
 * the pass-0 arm's tail (`g_sp_pal16 = ..; data = .. + 0x200;`); spelling the
 * frame-0 case out in full as a plain if/else inside arm 1 -- exactly the
 * shape SoftBlitAnimPlain (softblit2.c) already had -- and letting VC6 merge
 * the two identical tails ITSELF emits byte-identical code for the tails but
 * assigns the spill homes in the original's order (data -8, npasses -0xc).
 * A source-level `goto` into a sibling arm creates the shared block before
 * the allocator runs; compiler cross-jumping merges it after the homes are
 * assigned.  Both polarities of the if (`== 0` / `!= 0`) are exact. */
// FUNCTION: LEGOLAND 0x00465240
void SoftBlitAnim(LLSRec* lls, WinRect* src, Pos* dst)
{
    int          pass;
    void*        ctrl;
    void*        data;
    int          npasses;
    int          frame;
    AnimFrame*   f;
    int          n16;
    int          n;
    unsigned int k;

    npasses = 1;
    g_sp_rowlen = g_ddsd.lPitch;
    if (g_frame_override >= 0)
        frame = g_frame_override;
    else
        frame = lls->frame;
    if (frame >= lls->nframes) {
        frame = lls->nframes - 1;
    }
    if (lls->flags & 1)
        npasses = 2;
    for (pass = 0; pass < npasses; pass++) {
        f = (AnimFrame*)lls->frames;
        if (npasses == 1) {
            g_sp_pal16 = f->body + f->npixels;
            for (n = frame; n != 0; n--)
                f = (AnimFrame*)((char*)f + f->size);
            n16 = f->npixels;
            ctrl = f->body;
            if (lls->frame == 0) {
                /* frame 0 is being drawn: its own palette sits between the
                 * index block and the control stream.  Written out in full
                 * (not a goto into the pass-0 arm below): VC6 cross-jumps the
                 * identical tails itself, and the spelling decides the pool. */
                g_sp_pal16 = f->body + n16;
                data = f->body + n16 + 0x200;
            } else {
                data = f->body + n16;
            }
        } else if (pass == 0) {
            n16 = f->npixels;
            ctrl = f->body;
            g_sp_pal16 = f->body + n16;
            data = f->body + n16 + 0x200;
        } else {
            k = lls->frame + 1;
            while (k-- != 0)
                f = (AnimFrame*)((char*)f + f->size);
            ctrl = f->body;
            data = f->body + f->npixels;
        }
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
            mov     esi, ctrl
            mov     ebp, data
            xor     eax, eax
            mov     g_zb_bits, eax
            and     edx, edx
            je      rows
        /* ---- skip src->top rows -------------------------------------- */
        skip:
            shr     ebx, 2
            and     g_zb_bits, 0fh
            jne     skip1
            mov     ebx, [ebp]
            add     ebp, 4
            mov     g_zb_bits, 10h
        skip1:
            dec     g_zb_bits
            test    ebx, 2
            je      skip_one
            test    ebx, 1
            je      skip
            shr     ebx, 2
            cmp     g_zb_bits, 4
            jae     skip2
            mov     ebx, [ebp]
            add     ebp, 4
            mov     g_zb_bits, 10h
        skip2:
            shrd    ecx, ebx, 8
            shr     ebx, 6
            sub     g_zb_bits, 4
            shr     ecx, 18h
            je      skip_eol
            shr     ebx, 2
            and     g_zb_bits, 0fh
            jne     skip3
            mov     ebx, [ebp]
            add     ebp, 4
            mov     g_zb_bits, 10h
        skip3:
            dec     g_zb_bits
            test    ebx, 2
            je      skip4
            jmp     skip
        skip4:
            test    ebx, 1
            je      skip_run
        skip_one:
            inc     esi
            jmp     skip
        skip_run:
            lea     esi, [esi+ecx]
            jmp     skip
        skip_eol:
            dec     edx
            jne     skip
        /* ---- the row loop -------------------------------------------- */
        rows:
            mov     edx, g_sp_h
            and     edx, edx
            je      done
            mov     g_sp_rows_left, edx
        row:
            mov     edx, g_sp_left
            and     edx, edx
            je      row_full
        /* ---- skip src->left pixels of this row ------------------------ */
        lclip:
            shr     ebx, 2
            and     g_zb_bits, 0fh
            jne     lclip1
            mov     ebx, [ebp]
            add     ebp, 4
            mov     g_zb_bits, 10h
        lclip1:
            dec     g_zb_bits
            test    ebx, 2
            je      lclip_one
            test    ebx, 1
            je      lclip_step
            shr     ebx, 2
            cmp     g_zb_bits, 4
            jae     lclip2
            mov     ebx, [ebp]
            add     ebp, 4
            mov     g_zb_bits, 10h
        lclip2:
            shrd    ecx, ebx, 8
            shr     ebx, 6
            sub     g_zb_bits, 4
            shr     ecx, 18h
            je      nextrow
            shr     ebx, 2
            and     g_zb_bits, 0fh
            jne     lclip3
            mov     ebx, [ebp]
            add     ebp, 4
            mov     g_zb_bits, 10h
        lclip3:
            dec     g_zb_bits
            test    ebx, 2
            je      lclip4
            sub     edx, ecx
            ja      lclip
            neg     edx
            lea     edi, [edi+edx*2]
            mov     ecx, g_sp_w
            sub     ecx, edx
            jbe     endrow
            mov     edx, ecx
            jmp     paint
        lclip4:
            test    ebx, 1
            je      lclip5
            inc     esi
            sub     edx, ecx
            ja      lclip
            neg     edx
            mov     ecx, edx
            mov     edx, g_sp_w
            and     edx, edx
            je      endrow
            cmp     edi, g_sp_mouse_pixel
            setbe   al
            mov     g_sp_hit_armed, al
            and     ecx, ecx
            jne     fill_back
            jmp     paint
        lclip_one:
            inc     esi
        lclip_step:
            dec     edx
            jne     lclip
            mov     edx, g_sp_w
            jmp     paint
        lclip5:
            lea     esi, [esi+ecx]
            sub     edx, ecx
            ja      lclip
            lea     esi, [esi+edx]
            neg     edx
            mov     ecx, edx
            mov     edx, g_sp_w
            and     edx, edx
            je      endrow
            cmp     edi, g_sp_mouse_pixel
            setbe   al
            mov     g_sp_hit_armed, al
            and     ecx, ecx
            jne     copy_run
            jmp     paint
        row_full:
            mov     edx, g_sp_w
        /* ---- paint the visible pixels of this row --------------------- */
        paint:
            mov     eax, g_zb_bits
            shr     ebx, 2
            dec     eax
            jns     paint1
            mov     ebx, [ebp]
            add     ebp, 4
        paint1:
            and     eax, 0fh
            test    ebx, 2
            mov     g_zb_bits, eax
            je      one_pixel
            test    ebx, 1
            je      one_clear
            shr     ebx, 2
            sub     eax, 4
            jns     paint2
            mov     ebx, [ebp]
            add     ebp, 4
            mov     eax, 0ch
        paint2:
            movzx   ecx, bl
            shr     ebx, 6
            and     eax, 0fh
            test    ecx, ecx
            mov     g_zb_bits, eax
            je      nextrow
            shr     ebx, 2
            dec     eax
            jns     paint3
            mov     ebx, [ebp]
            add     ebp, 4
        paint3:
            and     eax, 0fh
            mov     g_zb_bits, eax
            cmp     edi, g_sp_mouse_pixel
            setbe   al
            mov     g_sp_hit_armed, al
            test    ebx, 2
            je      paint4
            sub     edx, ecx
            js      endrow
            lea     edi, [edi+ecx*2]
            jmp     paint
        fill_back:
            dec     esi
        paint4:
            test    ebx, 1
            je      copy_run
            movzx   eax, byte ptr [esi]
            add     eax, eax
            add     eax, g_sp_pal16
            movzx   eax, word ptr [eax]
            inc     esi
            cmp     edx, ecx
            ja      fill_long
            mov     ecx, edx
            or      ecx, ecx
            je      endrow
            and     ax, word ptr g_sp_recolour
        fill_short_loop:
            mov     word ptr [edi], ax
            add     edi, 2
            loop    fill_short_loop
            cmp     edi, g_sp_mouse_pixel
            movzx   ecx, byte ptr g_sp_hit_armed
            seta    al
            and     eax, ecx
            or      g_blit_hit, eax
            jmp     endrow
        fill_long:
            sub     edx, ecx
            or      ecx, ecx
            je      paint
            and     ax, word ptr g_sp_recolour
        fill_long_loop:
            mov     word ptr [edi], ax
            add     edi, 2
            loop    fill_long_loop
            cmp     edi, g_sp_mouse_pixel
            movzx   ecx, byte ptr g_sp_hit_armed
            seta    al
            and     eax, ecx
            or      g_blit_hit, eax
            jmp     paint
        one_pixel:
            mov     ecx, 1
            cmp     edi, g_sp_mouse_pixel
            sete    al
            mov     g_sp_hit_armed, al
        copy_run:
            cmp     edx, ecx
            ja      copy_long
            xchg    ecx, edx
            sub     edx, ecx
        copy_short_loop:
            and     ecx, ecx
            je      copy_short_end
            movzx   eax, byte ptr [esi]
            add     eax, eax
            add     eax, g_sp_pal16
            movzx   eax, word ptr [eax]
            inc     esi
            and     ax, word ptr g_sp_recolour
            mov     word ptr [edi], ax
            add     edi, 2
            loop    copy_short_loop
            cmp     edi, g_sp_mouse_pixel
            movzx   ecx, byte ptr g_sp_hit_armed
            seta    al
            and     eax, ecx
            or      g_blit_hit, eax
        copy_short_end:
            lea     esi, [esi+edx]
            jmp     endrow
        copy_long:
            sub     edx, ecx
        copy_long_loop:
            movzx   eax, byte ptr [esi]
            add     eax, eax
            add     eax, g_sp_pal16
            movzx   eax, word ptr [eax]
            inc     esi
            and     ax, word ptr g_sp_recolour
            mov     word ptr [edi], ax
            add     edi, 2
            loop    copy_long_loop
            cmp     edi, g_sp_mouse_pixel
            movzx   ecx, byte ptr g_sp_hit_armed
            seta    al
            and     eax, ecx
            or      g_blit_hit, eax
            jmp     paint
        one_clear:
            dec     edx
            je      endrow
            add     edi, 2
            jmp     paint
        /* ---- run the stream on to the end of the row ------------------ */
        endrow:
            shr     ebx, 2
            and     g_zb_bits, 0fh
            jne     end1
            mov     ebx, [ebp]
            add     ebp, 4
            mov     g_zb_bits, 10h
        end1:
            dec     g_zb_bits
            test    ebx, 2
            je      end_one
            test    ebx, 1
            je      endrow
            shr     ebx, 2
            cmp     g_zb_bits, 4
            jae     end2
            mov     ebx, [ebp]
            add     ebp, 4
            mov     g_zb_bits, 10h
        end2:
            shrd    ecx, ebx, 8
            shr     ebx, 6
            sub     g_zb_bits, 4
            shr     ecx, 18h
            je      nextrow
            shr     ebx, 2
            and     g_zb_bits, 0fh
            jne     end3
            mov     ebx, [ebp]
            add     ebp, 4
            mov     g_zb_bits, 10h
        end3:
            dec     g_zb_bits
            test    ebx, 2
            je      end4
            jmp     endrow
        end4:
            test    ebx, 1
            je      end_run
        end_one:
            inc     esi
            jmp     endrow
        end_run:
            lea     esi, [esi+ecx]
            jmp     endrow
        nextrow:
            mov     edi, g_zb_row
            mov     edx, g_sp_rows_left
            add     edi, g_sp_rowlen
            dec     edx
            mov     g_zb_row, edi
            jne     row
        done:
            popad
        }
#else
    LL_UNPORTED_ASM();
#endif
    }
}

/* -------------------------------------------------------------------------
 * 0x00469400 -- RENAMED.  iconui.c declares this address as
 * `UpdateHelpCursor` and calls it first thing in ProcessInGameHelp, but the
 * disassembly shows it has nothing to do with the cursor: it drains the
 * park's list of UNMET OBJECTIVES (0x00668728) and turns each one into an
 * advisor line ("You need to build %d more of object %s", "You need %d more
 * gardeners to look after your park", ...).  Named UpdateGoalHelpText here.
 *
 * The loop is a drain, not a walk: it re-reads the list HEAD every time,
 * formats the head goal, then calls RemoveGoals(code) which unlinks and
 * frees EVERY node carrying that code -- so a code with several nodes is
 * reported once and all of its nodes disappear.  The list must therefore be
 * finite and RemoveGoals must always remove the head, or this spins forever.
 *
 * The GetGameTimer() call at the top has its result discarded; it is kept
 * because the original keeps it.
 *
 * The Goal record (fields this function touches):
 *   +0x00 next
 *   +0x04 target -- either an object instance, read as target->cls->name
 *                   (+0x0c then +0x78), or a "range" record whose +0x00 is
 *                   the range name.  Which one depends on the code.
 *   +0x0c code   -- 0..0x13 selects the message; anything else is silently
 *                   dropped
 *   +0x14 amount -- the second numeric field (level / new-count / squares)
 *   +0x18 variant-- sub-selector for codes 15 and 19
 *   +0x1c count  -- the first numeric field; signed, and codes 8/9 report
 *                   "%d fewer" for a negative one
 *   +0x40 strid  -- index into the ready-made hint strings at 0x007fe120
 *                   (code 0); also stashed in 0x00668614
 *
 * Layout notes: the case blocks come out in SOURCE order, not sorted by
 * value -- which is why case 12 sits between cases 1 and 2 and the inner
 * switch of case 10 runs 0,1,4,5,2,3.  Every three-argument case is
 * cross-jumped into the LAST one laid out (the WESTERN zoning message), so
 * they all end `jmp` into its `call`; the one- and two-argument cases keep
 * their own copy of the RemoveGoals tail because their `add esp,N` differs.
 * ------------------------------------------------------------------------- */

typedef struct ObjCls {
    char  pad00[0x78];
    char* name;                 /* +0x78 */
} ObjCls;

typedef struct GoalTarget {
    char*   range_name;         /* +0x00 (range goals) */
    char    pad04[0x0c - 0x04];
    ObjCls* cls;                /* +0x0c (object goals) */
} GoalTarget;

typedef struct Goal {
    struct Goal* next;          /* +0x00 */
    GoalTarget*  target;        /* +0x04 */
    char         pad08[4];
    int          code;          /* +0x0c */
    char         pad10[4];
    int          amount;        /* +0x14 */
    int          variant;       /* +0x18 */
    int          count;         /* +0x1c */
    char         pad20[0x40 - 0x20];
    int          strid;         /* +0x40 */
} Goal;

extern Goal*       g_goal_list;        /* 0x00668728 */
extern int         g_last_hint;        /* 0x00668614 */
extern const char* g_hint_strings[];   /* 0x007fe120 */

extern int  GetGameTimer(void);                       /* 0x00499430 */
extern void AddHelpMessage(const char* fmt, ...);     /* 0x00468bb0 */
extern void RemoveGoals(int code);                    /* 0x004693b0 */

extern const char kFmtStr[];             /* 0x004b8bbc "%s" */
extern const char m_build_more_of[];     /* 0x004ba6bc */
extern const char m_research[];          /* 0x004ba69c */
extern const char m_connect_one[];       /* 0x004ba674 */
extern const char m_connect_all[];       /* 0x004ba648 */
extern const char m_link_one[];          /* 0x004ba60c */
extern const char m_link_all[];          /* 0x004ba5c8 */
extern const char m_build_new_range[];   /* 0x004ba590 */
extern const char m_build_more_range[];  /* 0x004ba558 */
extern const char m_delete_all_range[];  /* 0x004ba510 */
extern const char m_delete_range[];      /* 0x004ba4dc */
extern const char m_remove_items[];      /* 0x004ba4b0 */
extern const char m_delete_obj[];        /* 0x004ba48c */
extern const char m_attract_people[];    /* 0x004ba45c */
extern const char m_more_gardeners[];    /* 0x004ba428 */
extern const char m_fewer_gardeners[];   /* 0x004ba3fc */
extern const char m_more_mechanics[];    /* 0x004ba3cc */
extern const char m_fewer_mechanics[];   /* 0x004ba3a0 */
extern const char m_cover_objects[];     /* 0x004ba370 */
extern const char m_cover_rides[];       /* 0x004ba340 */
extern const char m_cover_shops[];       /* 0x004ba310 */
extern const char m_cover_food[];        /* 0x004ba2dc */
extern const char m_cover_scenery[];     /* 0x004ba2ac */
extern const char m_cover_wonder[];      /* 0x004ba26c */
extern const char m_path_scenery[];      /* 0x004ba234 */
extern const char m_save_coins[];        /* 0x004ba210 */
extern const char m_happiness[];         /* 0x004ba1dc */
extern const char m_hunger_fewer[];      /* 0x004ba198 */
extern const char m_hunger_more[];       /* 0x004ba15c */
extern const char m_repairs[];           /* 0x004ba110 */
extern const char m_ride_people[];       /* 0x004ba0e4 */
extern const char m_parts_diff[];        /* 0x004ba0b4 */
extern const char m_parts_more[];        /* 0x004ba08c */
extern const char m_zone_legoland[];     /* 0x004ba050 */
extern const char m_zone_adventurer[];   /* 0x004ba010 */
extern const char m_zone_castle[];       /* 0x004b9fd4 */
extern const char m_zone_western[];      /* 0x004b9f98 */

/* RESIDUAL (measured with tools/audit.py / a difflib alignment): ours is 314
 * instructions / 1056 bytes against the original's 363 / 1155, and 282 of
 * the original's 363 instructions (77.7%) align exactly.  ONE structural
 * difference accounts for all of it: the original INLINES the
 * `RemoveGoals(g->code)` tail into every one- and two-argument case block
 *     call AddHelpMessage / mov edx,[esi+0xc] / add esp,8 / push edx /
 *     call RemoveGoals / add esp,4 / jmp <loop top>
 * -- 20 copies, plus ONE shared copy that every THREE-argument case
 * cross-jumps into (they all `jmp` straight at its `call`).  We emit the one
 * shared copy and a `jmp` from each case.
 * WHY IT IS HARD.  The two shapes need contradictory things from VC6:
 *   * to get the tail duplicated you must write `RemoveGoals(g->code);` in
 *     every case (a "v2" source; that does give exactly 363 instructions),
 *   * but then the two calls are adjacent in one basic block and VC6 folds
 *     their two cdecl cleanups into a single `add esp,0xc` -- the original
 *     keeps `add esp,8` and `add esp,4` -- and the changed tails then
 *     cross-jump differently.  Measured: v2 aligns only 191/363 (52.6%),
 *     worse than the shared-tail form kept here, so the honest reconstruction
 *     is the natural C.
 * The fold is unconditional: probes in scratchpad/softblit/ show VC6 SP3 /O2
 * merges two consecutive cdecl call cleanups through a K&R or variadic
 * declaration of either callee, an intervening global store, an intervening
 * volatile read, a named temporary for the second argument, a do/while(0)
 * wrapper, a goto+label between them, a `switch (0)` wrapper, and both
 * `while`/`for(;;)`/`goto` spellings of the loop.  The ONLY thing that stops
 * it is a real basic-block boundary between the calls (probe r3), and a
 * boundary is exactly what the shared-tail form has -- which is why the
 * shared-tail form gets the `mov edx,[esi+0xc] / add esp,8` scheduling right
 * and the duplicated form does not.  So what is missing is VC6 duplicating
 * the shared tail into its predecessors, which it does not do here at /O2
 * for a five-instruction block containing a call.  Next agent: look for what
 * makes VC6 tail-duplicate such a block, not for another spelling of the
 * two calls -- that space is exhausted.
 * WHAT IS ALREADY RIGHT: the record layout, every format string and argument
 * order, the branch directions, and the case block LAYOUT ORDER -- which
 * follows SOURCE order here, not the case values (case 12 really does sit
 * between cases 1 and 2, and case 10's inner switch really is written
 * 0,1,4,5,2,3).  That contradicts bigrender.c's "switch case order is not a
 * lever" note for this shape, so both readings are on the record. */
/* PROGRESS NOTE (this round): the body is now 359 instructions against the
 * original's 363 (it was 314).  Writing the `RemoveGoals(g->code);` tail
 * EXPLICITLY at the end of every case, followed by `continue;` -- instead of
 * once after the switch -- is what reproduces the original's per-case tail;
 * left as one shared statement, VC6 keeps a single tail and the body comes
 * out 49 instructions short.  The trailing `RemoveGoals(g->code);` after the
 * switch is kept because the out-of-range default path needs it, exactly as
 * the original's `ja` arm does.  VC6 still cross-jumps the three-argument
 * cases into one shared `call AddHelpMessage / add esp,0xc / tail` block, as
 * the original does at 0x00469868.
 *
 * WHAT IS LEFT (11 instructions, and the index shift behind the reported
 * mismatch): in each per-case tail the original keeps the two stack pops
 * apart --
 *     call AddHelpMessage / ... / add esp,8 /
 *     mov edx,[esi+0xc] / push edx / call RemoveGoals / add esp,4
 * -- whereas VC6 here pushes RemoveGoals' argument while AddHelpMessage's
 * arguments are still on the stack and MERGES the two into one `add esp,0xc`.
 * Ruled out: a volatile read for `g->code`, a `for(;;)` loop with the test
 * hoisted, a `goto next;` label form, and an extra `continue` after the
 * shared tail (the last two both collapse back to 307 instructions).  Note
 * the reported mismatch went 328 -> 335 even though the body got 45
 * instructions closer: the strict index-for-index compare is dominated by the
 * one-instruction-per-case shift, so read the instruction count here, not the
 * mismatch. */
/* SOLVED THIS ROUND (the diagnosis, not the codegen).  The original's source
 * is the SHARED-TAIL form -- one `RemoveGoals(g->code);` after the switch, not
 * one per case -- and VC6 duplicated that five-instruction tail into all 21
 * predecessors during final block layout.  Three independent proofs:
 *   (a) VC6 ALWAYS folds two adjacent __cdecl cleanups.  Measured on isolated
 *       one-line functions: `A(f,x); R(y);` gives `call A / mov / push / call R
 *       / add esp,0Ch` with no `add esp,8`, and that holds with a statement
 *       between them, with an unprototyped callee, and with a value-returning
 *       callee.  The ONLY thing that splits the pair is a basic-block boundary
 *       (`if (c) R(y);` gives `add esp,8 ... add esp,4`).  So the original's 21
 *       split pairs CANNOT come from source-level `AddHelpMessage(); 
 *       RemoveGoals();` in one block, however it is spelled.
 *   (b) All 21 of the original's tails are byte-identical
 *       (`mov edx,[esi+0Ch] / push edx / call 4693B0 / add esp,4 /
 *        jmp 469406`) yet are NOT cross-jumped, while our per-case form gets
 *       cross-jumped into 9 shared blocks grouped by cleanup size.  Identical
 *       blocks that survive cross-jumping can only have been created AFTER
 *       that pass -- i.e. by late duplication.
 *   (c) The arithmetic closes exactly.  Rewriting the body with one shared tail
 *       (each case just `break;`) compiles to 285 instructions whose cleanup
 *       histogram is the original's minus the duplicates: 18x `add esp,8`,
 *       1x `add esp,0Ch`, 3x `add esp,4` against the original's 18 / 1 / 23.
 *       Un-rotating the loop saves 2 (the rotated latch `mov esi,g /
 *       add esp,4 / test / jne` becomes `add esp,4 / jmp header`), and
 *       duplicating the 5-instruction tail over 20 predecessors that currently
 *       end in `jmp tail` adds 4 each:  285 - 2 + 80 = 363, the original's
 *       exact count.
 * WHAT BLOCKS IT: our VC6 build ROTATES the loop, pulling `mov esi,g_goal_list
 * / test esi,esi / jne` into the latch so the tail is no longer a block ending
 * in an unconditional jump, and the duplication never fires.  Measured and all
 * still rotated: `while ((g = g_goal_list) != 0)`, `for (;;) { g = ...; if (!g)
 * break; }`, the same with `return` instead of `break`, a `goto` loop, testing
 * the global and loading `g` separately, hoisting `g = g_goal_list` to the
 * bottom of the body, a `volatile` global, an extra `continue` back edge, and
 * wrapping the tail in `if (g != 0)`.  Find what stops the rotation and this
 * function falls out at 363 exactly.
 * The body left in place is the per-case form (359 instructions) because it is
 * the closer of the two by instruction count and reproduces every case's
 * argument sequence; the shared-tail form scores 285 but is, per the above,
 * the true source. */
/* CLOSED (2026-09-03, softblit lane): the loop scaffold was the lever.
 *     while (1) { g = g_goal_list; if (!g) break; switch (..) {..} RemoveGoals(g->code); }
 * A `while (1)` keeps a (folded) constant-true test block as the loop HEADER,
 * so the `if (!g) break` exit sits in the block after it and VC6's CFG loop
 * inversion -- which only fires when the header itself ends in the exit
 * conditional -- never runs.  The shared `RemoveGoals` tail therefore stays a
 * five-instruction block ending in `jmp header`, and VC6's late jump
 * optimisation duplicates it (after register allocation, hence `edx` in
 * every copy; before scheduling, hence `mov edx,[esi+0xc]` above each
 * `add esp,8`) into the 20 predecessors that ended in `jmp tail`, leaving
 * the fall-through copy for the three-argument cases and the `ja` default.
 * `for (;;)` and `do {..} while (1)` with the same break, `while ((g = ..))`,
 * `goto top` and every other form measured earlier are inverted (the test
 * copied into the latch, tail not duplicated: 314 instructions).  Also exact:
 * a `while ((g = ..) != 0)` loop whose tail ends `goto again;` to a label
 * ABOVE the while, and a `goto` into a label at the end of the body -- all
 * three put an extra block in front of the test.  Cases end with `break`;
 * the single RemoveGoals after the switch is the true source shape (the
 * per-case form folds the two cleanups into `add esp,0xc`). */
// FUNCTION: LEGOLAND 0x00469400
void UpdateGoalHelpText(void)
{
    Goal* g;

    GetGameTimer();
    while (1) {
        g = g_goal_list;
        if (!g)
            break;
        switch (g->code) {
        case 0:
            AddHelpMessage(kFmtStr, g_hint_strings[g->strid]);
            g_last_hint = g->strid;
            break;
        case 1:
            AddHelpMessage(m_build_more_of, g->count, g->target->cls->name);
            break;
        case 12:
            AddHelpMessage(m_research, g->target->cls->name);
            break;
        case 2:
            if (g->count == 0) {
                if (g->target)
                    AddHelpMessage(m_connect_one, g->target->cls->name);
                else
                    AddHelpMessage(m_connect_all);
            } else {
                if (g->target)
                    AddHelpMessage(m_link_one, g->target->cls->name);
                else
                    AddHelpMessage(m_link_all);
            }
            break;
        case 3:
            if (g->amount != 0)
                AddHelpMessage(m_build_new_range, g->amount, g->target->range_name);
            else
                AddHelpMessage(m_build_more_range, g->count, g->target->range_name);
            break;
        case 4:
            if (g->amount != 0)
                AddHelpMessage(m_delete_all_range, g->amount, g->target->range_name);
            else
                AddHelpMessage(m_delete_range, g->count, g->target->range_name);
            break;
        case 5:
            AddHelpMessage(m_remove_items, g->count);
            break;
        case 6:
            AddHelpMessage(m_delete_obj, g->count, g->target->cls->name);
            break;
        case 7:
            AddHelpMessage(m_attract_people, g->count);
            break;
        case 8:
            if (g->count > 0)
                AddHelpMessage(m_more_gardeners, g->count);
            else
                AddHelpMessage(m_fewer_gardeners, -g->count);
            break;
        case 9:
            if (g->count > 0)
                AddHelpMessage(m_more_mechanics, g->count);
            else
                AddHelpMessage(m_fewer_mechanics, -g->count);
            break;
        case 10:
            switch (g->count) {
            case 0:
                AddHelpMessage(m_cover_objects, g->amount);
                break;
            case 1:
                AddHelpMessage(m_cover_rides, g->amount);
                break;
            case 4:
                AddHelpMessage(m_cover_shops, g->amount);
                break;
            case 5:
                AddHelpMessage(m_cover_food, g->amount);
                break;
            case 2:
                AddHelpMessage(m_cover_scenery, g->amount);
                break;
            case 3:
                AddHelpMessage(m_cover_wonder, g->amount);
                break;
            }
            break;
        case 11:
            AddHelpMessage(m_path_scenery, g->amount);
            break;
        case 13:
            AddHelpMessage(m_save_coins, g->count);
            break;
        case 14:
            AddHelpMessage(m_happiness, g->count, g->amount);
            break;
        case 15:
            if (g->variant != 0)
                AddHelpMessage(m_hunger_fewer, g->count, g->amount);
            else
                AddHelpMessage(m_hunger_more, g->count, g->amount);
            break;
        case 16:
            AddHelpMessage(m_repairs, g->count, g->amount);
            break;
        case 17:
            AddHelpMessage(m_ride_people, g->count, g->target->cls->name);
            break;
        case 18:
            if (g->amount != 0)
                AddHelpMessage(m_parts_diff, g->amount, g->target->cls->name);
            else
                AddHelpMessage(m_parts_more, g->count, g->target->cls->name);
            break;
        case 19:
            switch (g->variant) {
            case 0:
                AddHelpMessage(m_zone_legoland, g->count, g->amount);
                break;
            case 1:
                AddHelpMessage(m_zone_adventurer, g->count, g->amount);
                break;
            case 2:
                AddHelpMessage(m_zone_castle, g->count, g->amount);
                break;
            case 3:
                AddHelpMessage(m_zone_western, g->count, g->amount);
                break;
            }
            break;
        }
        RemoveGoals(g->code);
    }
}
