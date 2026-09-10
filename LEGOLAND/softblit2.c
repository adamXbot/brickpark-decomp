/* LEGOLAND -- the PLAIN (non-recolouring) CPU animation painters.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field offsets, global addresses and callee arg counts are load-bearing;
 * names are ours.  No legoland.h: every type is defined locally.
 *
 * These are the two painters SoftBlitSprite (softblit.c, 0x00464ee0)
 * dispatches to for ImageRec::type 2 and 3.  Their recolouring siblings --
 * the ones SoftPrint_XBltFast (bigrender.c) uses -- are SoftBlitAnim
 * (0x00465240) and SoftBlitRLE (0x00465ee0) in softblit.c.  "Plain" means
 * exactly one thing: the emitted pixel is `palette[index]`, with no
 * `and ax, g_sp_recolour` in the store path.  Everything else -- the control
 * stream, the clipping, the mouse hit test -- is identical.
 *
 * The two are built very differently, and that is worth knowing before reading
 * either.  SoftBlitAnimPlain's pixel loop is HAND-WRITTEN __asm (which is why
 * it carries an ebp frame and a full ebx/esi/edi save -- an __asm block forces
 * both); only its frame-selection prologue and pass loop are C, and the asm
 * brackets itself with its own register saves, which are NOT `pushad` but six
 * explicit pushes (esi,edi,ebx,ecx,edx,ebp) popped in mirror order.
 * SoftBlitRLEPlain has no pixel loop at all: it is pure C, esp-framed, and is
 * a DISPATCHER that stamps the scratch globals and then calls one of EIGHT
 * specialised painters (0x00466d80 .. 0x00467f00) chosen by the clip geometry
 * and by whether the cursor is inside the sprite's box.
 *
 * ---------------------------------------------------------------------------
 * The locked-surface contract
 * ---------------------------------------------------------------------------
 * Everything is drawn into the currently locked DirectDraw surface described
 * by gpu.c's lock descriptor at 0x0066809c:
 *   lpSurface (+0x24, 0x006680c0)  base of the 16-bpp pixel array
 *   lPitch    (+0x10, 0x006680ac)  BYTES per row
 * The destination pixel of `dst` is  lpSurface + dst->y*lPitch + dst->x*2
 * (the original spells the *2 as `add edi,[eax]` twice).  16 bpp throughout;
 * the painters never look at the channel masks.
 *
 * ---------------------------------------------------------------------------
 * The SoftPrint scratch globals (shared with softblit.c -- NOT re-entrant)
 * ---------------------------------------------------------------------------
 *   0x007fe9a4  g_sp_rowlen       the row step: lPitch for a surface blit
 *                                 (0x200 for the off-screen 256x? scratch
 *                                 buffer used by 0x00464a90)
 *   0x007fe9a8  g_sp_mouse_pixel  address of the surface pixel under the mouse
 *   0x007fea10  g_sp_rows_left    rows still to paint
 *   0x007fea18  g_sp_hit_armed    byte: the run being painted STARTS at or
 *                                 before the mouse pixel
 *   0x007fea20  g_sp_pal16        256-entry u16 palette
 *   0x007fea4c  g_sp_top          src->top
 *   0x007fea50  g_sp_left         src->left
 *   0x007feb14  g_blit_hit        1 when a painted run covered the mouse pixel
 *   0x007feb18  g_zb_row          address of the current output row
 *   0x007febac  g_sp_h            src->bottom - src->top
 *   0x007febb0  g_sp_w            src->right  - src->left
 *   0x00668160  g_zb_bits         2-bit codes left in the control word
 *
 * ---------------------------------------------------------------------------
 * The LLS animation record and its frames
 * ---------------------------------------------------------------------------
 *   LLSRec:    +0x00 short current frame
 *              +0x10 short frame count
 *              +0x14 dword flags (bit 0 = the list opens with a BASE image
 *                    that must be painted underneath the selected frame)
 *              +0x18 the frame list
 *   AnimFrame: +0x00 int total byte length (add it to reach the next frame)
 *              +0x04 int byte length of the 8-bit INDEX block
 *              +0x08 the index block, then
 *                    -- FRAME 0 ONLY -- a 0x200-byte 256-entry u16 palette,
 *                    then the 32-bit control stream.
 * Frame 0 owns the palette; every later frame borrows it, which is why the
 * single-pass path stamps g_sp_pal16 from frames[0] BEFORE walking to the
 * selected frame, and why the control stream of frame 0 starts 0x200 bytes
 * further on than everyone else's.
 *
 * Frame selection: g_frame_override (0x004b9ca8) when >= 0, else the record's
 * own current frame, clamped to nframes - 1.  With flags bit 0 set the whole
 * body runs TWICE: pass 0 paints frames[0] (the base image) and pass 1 paints
 * frames[frame + 1].  sprite_override.c's g_override_palette (0x006681e8)
 * replaces the frame's own palette when non-NULL.
 *
 * DIVERGENCE FROM THE RECOLOURING SoftBlitAnim, worth recording: the plain
 * painter uses the SELECTED frame index everywhere (`frame != 0` to decide
 * the 0x200 palette skip, `frame + 1` for the second pass), whereas
 * 0x00465240 re-reads lls->frame for both of those and therefore reads the
 * wrong block when an override is in force.  The plain path is the correct
 * one; the recolouring path carries the bug.
 *
 * ---------------------------------------------------------------------------
 * The control stream (2-bit codes, LSB first, 16 per 32-bit word)
 * ---------------------------------------------------------------------------
 * ebp walks 32-bit control words, ebx holds the word being consumed, and
 * g_zb_bits counts the codes left in it.  The refill idiom is
 *      mov eax, g_zb_bits / shr ebx,2 / dec eax / jns have
 *      mov eax,0Fh / mov ebx,[ebp] / add ebp,4
 *   have: mov g_zb_bits, eax
 * i.e. the shift is done unconditionally and thrown away when the word is
 * exhausted.  An 8-bit literal count is spliced out of the low byte
 * (`movzx ecx,bl` then `shr ebx,6`) and costs 4 codes (`sub eax,4`, refill
 * value 0Ch).  A code is read as the low two bits of ebx AFTER `shr ebx,2`:
 *      bit1 = 0            one literal pixel: take the next index byte
 *      bit1 = 1, bit0 = 0  one transparent pixel: just step the output
 *      bit1 = 1, bit0 = 1  an 8-bit COUNT follows; a count of ZERO ends the
 *                          row.  A second 2-bit code says what it means:
 *                            bit1 = 1            skip `count` pixels
 *                            bit1 = 0, bit0 = 0  copy `count` index bytes
 *                            bit1 = 0, bit0 = 1  repeat ONE index byte
 *                                                `count` times (the RLE run)
 * There is no transparent-colour test anywhere in the paint loops: in this
 * format transparency IS the control stream.
 *
 * Four passes over the stream: skip src->top whole rows; per row skip
 * src->left pixels (with the sub/jns/neg fix-up that splits a run straddling
 * the left edge); paint src width pixels; run the stream on to the end of the
 * row.  edx is the pixel budget throughout, edi the output pointer, esi the
 * index-byte pointer, and g_zb_row / g_sp_rows_left carry the row base and
 * the row counter across the inner loops.
 *
 * The repeat-run fast path builds a doubled dword in eax with the
 * `push eax / shl eax,16 / add eax,[esp] / add esp,4` trick and uses
 * `rep stosd` (odd leading pixel emitted by hand); a run that overflows the
 * remaining budget falls back to `rep stosw`.
 *
 * THE MOUSE HIT TEST.  Before each painted run `setbe`/`sete` arms
 * g_sp_hit_armed if edi has not yet passed g_sp_mouse_pixel; after the run
 * `g_blit_hit |= (edi > mouse) & armed`, spelled cmp/movzx/sbb ecx,-1/and.
 * printlist.c's PrintSprite reads g_blit_hit to decide which gadget the
 * pointer is over.
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

/* An LLS animation record. */
typedef struct LLSRec {
    short          frame;       /* +0x00 current frame */
    short          pad02;       /* +0x02 */
    int            w;           /* +0x04 sprite width  (RLE records only) */
    int            h;           /* +0x08 sprite height (RLE records only) */
    int            pad0c;       /* +0x0c */
    short          nframes;     /* +0x10 */
    short          pad12;       /* +0x12 */
    unsigned int   flags;       /* +0x14 bit 0 = base image first */
    char           frames[1];   /* +0x18 */
} LLSRec;

/* One frame of an LLS animation record. */
typedef struct AnimFrame {
    int  size;     /* +0x00 byte length; add to walk to the next frame */
    int  npixels;  /* +0x04 byte length of the index block */
    char body[1];  /* +0x08 */
} AnimFrame;

/* ---- globals ------------------------------------------------------------ */

extern LockState      g_lock;               /* 0x0066809c */
#define g_ddsd        g_lock.ddsd

extern int            g_frame_override;     /* 0x004b9ca8  (-1 = none) */
extern void*          g_override_palette;   /* 0x006681e8  (0 = none) */
extern int            g_sp_rowlen;          /* 0x007fe9a4 */
extern void*          g_sp_mouse_pixel;     /* 0x007fe9a8 */
extern int            g_sp_rows_left;       /* 0x007fea10 */
extern unsigned char  g_sp_hit_armed;       /* 0x007fea18 */
extern void*          g_sp_pal16;           /* 0x007fea20 */
extern int            g_sp_top;             /* 0x007fea4c */
extern int            g_sp_left;            /* 0x007fea50 */
extern int            g_blit_hit;           /* 0x007feb14 */
extern void*          g_zb_row;             /* 0x007feb18 */
extern int            g_sp_h;               /* 0x007febac */
extern int            g_sp_w;               /* 0x007febb0 */
extern int            g_zb_bits;            /* 0x00668160 */

/* -------------------------------------------------------------------------
 * 0x00464480 -- cdecl(lls, src, dst): paint one frame of an LLS animation
 * record (ImageRec::type 2) into the locked surface, WITHOUT recolouring.
 * See the file header for the record layout, the control stream and the
 * mouse hit test.
 *
 * Frame homes recovered from the original: `frame` lives in the DEAD `lls`
 * parameter slot at [ebp+8] (lls itself is fully enregistered in ecx and its
 * only loop-carried use, lls->frames, is hoisted to [ebp-0x14] before the
 * pass loop), and the 0x14-byte pool below ebp holds, ascending,
 * f(-0x14) pass(-0x10) ctrl(-0xc) npasses(-8) data(-4).
 * ------------------------------------------------------------------------- */

/* THE FRAME-WALK IDIOM (worth carrying to every RLE/LLS painter).  The count
 * loop here is spelled `n = frame; while (n--) ...` -- the SIGNED post-
 * decrement `while`, not `for (n = frame; n != 0; n--)`.  Both walk the same
 * frames, but only the post-decrement form makes VC6 spell the counter copy
 * `lea ecx,[esi]` instead of `mov ecx,esi`; that one byte pair was the whole
 * residual of this function.  bigrender.c's ZBufferHelper (0x00464a90), the
 * Z-buffer sibling of this painter, uses the same idiom. */
// FUNCTION: LEGOLAND 0x00464480
void SoftBlitAnimPlain(LLSRec* lls, WinRect* src, Pos* dst)
{
    AnimFrame*   f;
    int          pass;
    void*        ctrl;
    int          npasses;
    void*        data;
    int          frame;
    int          n16;
    int          n;
    unsigned int k;

    g_sp_rowlen = g_ddsd.lPitch;
    npasses = 1;
    if (g_frame_override >= 0)
        frame = g_frame_override;
    else
        frame = lls->frame;
    if (frame >= lls->nframes)
        frame = lls->nframes - 1;
    if (lls->flags & 1)
        npasses = 2;
    for (pass = 0; pass < npasses; pass++) {
        f = (AnimFrame*)lls->frames;
        if (npasses == 1) {
            n16 = f->npixels;
            if (g_override_palette != 0)
                g_sp_pal16 = g_override_palette;
            else
                g_sp_pal16 = f->body + n16;
            n = frame;
            while (n--)   /* post-decrement form: VC6 copies the count with `lea` */
                f = (AnimFrame*)((char*)f + f->size);
            n16 = f->npixels;
            ctrl = f->body;
            if (frame == 0)
                data = f->body + n16 + 0x200;
            else
                data = f->body + n16;
        } else if (pass == 0) {
            n16 = f->npixels;
            ctrl = f->body;
            if (g_override_palette != 0)
                g_sp_pal16 = g_override_palette;
            else
                g_sp_pal16 = f->body + n16;
            data = f->body + n16 + 0x200;
        } else {
            k = frame + 1;
            while (k-- != 0)
                f = (AnimFrame*)((char*)f + f->size);
            n16 = f->npixels;
            ctrl = f->body;
            data = f->body + n16;
        }
#ifndef LEGOLAND_PORTABLE
        __asm {
            push    esi
            push    edi
            push    ebx
            push    ecx
            push    edx
            push    ebp
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
        /* ---- skip src->top whole rows -------------------------------- */
        srow:
            mov     eax, g_zb_bits
            shr     ebx, 2
            dec     eax
            jns     sr1
            mov     eax, 0fh
            mov     ebx, [ebp]
            add     ebp, 4
        sr1:
            mov     g_zb_bits, eax
            test    ebx, 2
            je      sr_one
            test    ebx, 1
            je      srow
            mov     eax, g_zb_bits
            shr     ebx, 2
            sub     eax, 4
            movzx   ecx, bl
            jns     sr2
            mov     eax, 0ch
            mov     ebx, [ebp]
            movzx   ecx, bl
            add     ebp, 4
        sr2:
            mov     g_zb_bits, eax
            shr     ebx, 6
            test    ecx, ecx
            je      sr_eol
            mov     eax, g_zb_bits
            shr     ebx, 2
            dec     eax
            jns     sr3
            mov     eax, 0fh
            mov     ebx, [ebp]
            add     ebp, 4
        sr3:
            mov     g_zb_bits, eax
            test    ebx, 2
            je      sr4
            jmp     srow
        sr4:
            test    ebx, 1
            je      sr_run
        sr_one:
            inc     esi
            jmp     srow
        sr_run:
            lea     esi, [esi+ecx]
            jmp     srow
        sr_eol:
            dec     edx
            jne     srow
        /* ---- the row loop -------------------------------------------- */
        rows:
            mov     edx, g_sp_h
            and     edx, edx
            je      done
        row:
            mov     g_sp_rows_left, edx
            mov     edx, g_sp_left
            and     edx, edx
            je      row_full
        /* ---- skip src->left pixels of this row ------------------------ */
        lclip:
            mov     eax, g_zb_bits
            shr     ebx, 2
            dec     eax
            jns     lc1
            mov     eax, 0fh
            mov     ebx, [ebp]
            add     ebp, 4
        lc1:
            mov     g_zb_bits, eax
            test    ebx, 2
            je      lc_one
            test    ebx, 1
            je      lc_step
            mov     eax, g_zb_bits
            shr     ebx, 2
            sub     eax, 4
            movzx   ecx, bl
            jns     lc2
            mov     eax, 0ch
            mov     ebx, [ebp]
            movzx   ecx, bl
            add     ebp, 4
        lc2:
            mov     g_zb_bits, eax
            shr     ebx, 6
            test    ecx, ecx
            je      nextrow
            mov     eax, g_zb_bits
            shr     ebx, 2
            dec     eax
            jns     lc3
            mov     eax, 0fh
            mov     ebx, [ebp]
            add     ebp, 4
        lc3:
            mov     g_zb_bits, eax
            test    ebx, 2
            je      lc4
            sub     edx, ecx
            jns     lclip
            neg     edx
            lea     edi, [edi+edx*2]
            mov     ecx, g_sp_w
            sub     ecx, edx
            js      endrow
            mov     edx, ecx
            jmp     paint
        lc4:
            test    ebx, 1
            je      lc5
            inc     esi
            sub     edx, ecx
            jns     lclip
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
        lc_one:
            inc     esi
        lc_step:
            dec     edx
            jne     lclip
            mov     edx, g_sp_w
            jmp     paint
        lc5:
            add     esi, ecx
            sub     edx, ecx
            jns     lclip
            neg     edx
            mov     ecx, edx
            mov     edx, g_sp_w
            and     edx, edx
            je      endrow
            sub     esi, ecx
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
            jns     p1
            mov     eax, 0fh
            mov     ebx, [ebp]
            add     ebp, 4
        p1:
            mov     g_zb_bits, eax
            test    ebx, 2
            je      p5
            test    ebx, 1
            je      p_one
            mov     eax, g_zb_bits
            shr     ebx, 2
            sub     eax, 4
            movzx   ecx, bl
            jns     p2
            mov     eax, 0ch
            mov     ebx, [ebp]
            movzx   ecx, bl
            add     ebp, 4
        p2:
            mov     g_zb_bits, eax
            shr     ebx, 6
            test    ecx, ecx
            je      nextrow
            mov     eax, g_zb_bits
            shr     ebx, 2
            dec     eax
            jns     p3
            mov     eax, 0fh
            mov     ebx, [ebp]
            add     ebp, 4
        p3:
            mov     g_zb_bits, eax
            cmp     edi, g_sp_mouse_pixel
            setbe   al
            mov     g_sp_hit_armed, al
            test    ebx, 2
            je      p4
            sub     edx, ecx
            js      endrow
            lea     edi, [edi+ecx*2]
            jmp     paint
        fill_back:
            sub     esi, 1
        p4:
            test    ebx, 1
            je      copy_run
            movzx   eax, byte ptr [esi]
            cmp     edx, ecx
            ja      fill_w
            add     eax, eax
            inc     esi
            add     eax, g_sp_pal16
            mov     ecx, edx
            movzx   eax, word ptr [eax]
            push    eax
            shl     eax, 10h
            add     eax, [esp]
            add     esp, 4
            shr     ecx, 1
            mov     [edi], ax
            jae     fill_d
            add     edi, 2
        fill_d:
            rep     stosd
            cmp     edi, g_sp_mouse_pixel
            movzx   eax, g_sp_hit_armed
            sbb     ecx, -1
            and     eax, ecx
            movzx   ecx, byte ptr g_blit_hit
            or      eax, ecx
            mov     byte ptr g_blit_hit, al
            jmp     endrow
        fill_w:
            add     eax, eax
            inc     esi
            add     eax, g_sp_pal16
            sub     edx, ecx
            movzx   eax, word ptr [eax]
            rep     stosw
            cmp     edi, g_sp_mouse_pixel
            movzx   eax, g_sp_hit_armed
            sbb     ecx, -1
            and     eax, ecx
            movzx   ecx, byte ptr g_blit_hit
            or      eax, ecx
            mov     byte ptr g_blit_hit, al
            jmp     paint
        p5:
            mov     ecx, 1
            cmp     edi, g_sp_mouse_pixel
            sete    al
            mov     g_sp_hit_armed, al
        copy_run:
            cmp     edx, ecx
            ja      crb
            xchg    ecx, edx
            sub     edx, ecx
        cr_loop:
            and     ecx, ecx
            je      cr_end
            movzx   eax, byte ptr [esi]
            add     eax, eax
            add     eax, g_sp_pal16
            movzx   eax, word ptr [eax]
            inc     esi
            stosw
            loop    cr_loop
            cmp     edi, g_sp_mouse_pixel
            movzx   eax, g_sp_hit_armed
            sbb     ecx, -1
            and     eax, ecx
            movzx   ecx, byte ptr g_blit_hit
            or      eax, ecx
            mov     byte ptr g_blit_hit, al
        cr_end:
            lea     esi, [esi+edx]
            jmp     endrow
        crb:
            sub     edx, ecx
        crb_loop:
            movzx   eax, byte ptr [esi]
            add     eax, eax
            add     eax, g_sp_pal16
            movzx   eax, word ptr [eax]
            inc     esi
            stosw
            loop    crb_loop
            cmp     edi, g_sp_mouse_pixel
            movzx   eax, g_sp_hit_armed
            sbb     ecx, -1
            and     eax, ecx
            movzx   ecx, byte ptr g_blit_hit
            or      eax, ecx
            mov     byte ptr g_blit_hit, al
            jmp     paint
        p_one:
            dec     edx
            add     edi, 2
            je      endrow
            jmp     paint
        /* ---- run the stream on to the end of the row ------------------ */
        endrow:
            mov     eax, g_zb_bits
            shr     ebx, 2
            dec     eax
            jns     er1
            mov     eax, 0fh
            mov     ebx, [ebp]
            add     ebp, 4
        er1:
            mov     g_zb_bits, eax
            test    ebx, 2
            je      er_one
            test    ebx, 1
            je      endrow
            mov     eax, g_zb_bits
            shr     ebx, 2
            sub     eax, 4
            movzx   ecx, bl
            jns     er2
            mov     eax, 0ch
            mov     ebx, [ebp]
            movzx   ecx, bl
            add     ebp, 4
        er2:
            mov     g_zb_bits, eax
            shr     ebx, 6
            test    ecx, ecx
            je      nextrow
            mov     eax, g_zb_bits
            shr     ebx, 2
            dec     eax
            jns     er3
            mov     eax, 0fh
            mov     ebx, [ebp]
            add     ebp, 4
        er3:
            mov     g_zb_bits, eax
            test    ebx, 2
            je      er4
            jmp     endrow
        er4:
            test    ebx, 1
            je      er_run
        er_one:
            inc     esi
            jmp     endrow
        er_run:
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
            pop     ebp
            pop     edx
            pop     ecx
            pop     ebx
            pop     edi
            pop     esi
        }
#else
    {
    /* The pixel loop of 0x00464480 in C.  Register map:
     *   edi = dp    the output pixel (16 bpp), g_zb_row = its row base
     *   esi = ip    the 8-BIT INDEX bytes -- the local called `ctrl` above,
     *               which is `f->body`; the local called `data` is the
     *               32-bit CONTROL stream, which is what `ebp` walks.  The
     *               two names are the wrong way round in the prologue and
     *               are left as they are (this is matched C).
     *   ebx/g_zb_bits = the control-word reader (LLAnimCtl)
     *   edx         = the top counter, then the row counter, then the left
     *                 skip, then the pixel budget, in that order
     *   ecx         = the current run length
     * Four passes: skip src->top whole rows; per row skip src->left pixels;
     * paint the visible span; run the stream on to the end-of-row marker.
     * The mouse hit test is the arm/test pair through g_sp_hit_armed. */
    LLAnimCtl             cs;
    unsigned char*        rowp;
    unsigned short*       dp;
    const unsigned char*  ip;
    const unsigned short* pal;
    unsigned int          code;
    unsigned int          cnt;
    unsigned int          drawn;
    unsigned int          half;
    unsigned short        v;
    int                   budget;
    int                   rows_left;
    int                   skip;

    rowp = (unsigned char*)g_ddsd.lpSurface + dst->x * 2
         + dst->y * g_ddsd.lPitch;
    g_zb_row = rowp;
    g_sp_left = src->left;
    g_sp_w = src->right - src->left;
    g_sp_top = src->top;
    g_sp_h = src->bottom - src->top;
    dp = (unsigned short*)rowp;
    ip = (const unsigned char*)ctrl;
    pal = (const unsigned short*)g_sp_pal16;
    ll_anim_open(&cs, data);
    g_zb_bits = 0;

    /* ---- srow: consume src->top whole rows ---------------------------- */
    skip = g_sp_top;
    if (skip != 0) {
        for (;;) {
            code = ll_anim_code(&cs);
            if (!(code & 2)) { ip++; continue; }     /* one index byte    */
            if (!(code & 1)) continue;               /* one transparent   */
            cnt = ll_anim_count(&cs);
            if (cnt == 0) {                          /* sr_eol            */
                if (--skip != 0) continue;
                break;
            }
            code = ll_anim_code(&cs);
            if (code & 2) continue;                  /* skip run          */
            if (code & 1) ip++;                      /* repeat run        */
            else ip += cnt;                          /* copy run          */
        }
    }

    /* ---- rows --------------------------------------------------------- */
    rows_left = g_sp_h;
    if (rows_left == 0)
        goto anim_done;

anim_row:
    g_sp_rows_left = rows_left;
    skip = g_sp_left;
    if (skip == 0) {                                 /* row_full          */
        budget = g_sp_w;
        goto anim_paint;
    }
    /* ---- lclip: skip src->left pixels of this row ---------------------- */
    for (;;) {
        code = ll_anim_code(&cs);
        if (!(code & 2)) {                           /* lc_one            */
            ip++;
            goto anim_lc_step;
        }
        if (!(code & 1))                             /* lc_step           */
            goto anim_lc_step;
        cnt = ll_anim_count(&cs);
        if (cnt == 0)
            goto anim_nextrow;
        code = ll_anim_code(&cs);
        if (code & 2) {                              /* skip run          */
            skip -= (int)cnt;
            if (skip >= 0) continue;
            skip = -skip;                            /* the visible part  */
            dp += skip;
            budget = g_sp_w - skip;
            if (budget < 0)
                goto anim_endrow;
            goto anim_paint;
        }
        if (code & 1) {                              /* lc4: repeat run   */
            ip++;
            skip -= (int)cnt;
            if (skip >= 0) continue;
            cnt = (unsigned int)(-skip);
            budget = g_sp_w;
            if (budget == 0)
                goto anim_endrow;
            g_sp_hit_armed = (unsigned char)
                ((const unsigned char*)dp
                 <= (const unsigned char*)g_sp_mouse_pixel);
            if (cnt != 0) {
                ip--;                                /* fill_back         */
                goto anim_p4;
            }
            goto anim_paint;
        }
        /* lc5: copy run */
        ip += cnt;
        skip -= (int)cnt;
        if (skip >= 0) continue;
        cnt = (unsigned int)(-skip);
        budget = g_sp_w;
        if (budget == 0)
            goto anim_endrow;
        ip -= cnt;                                   /* back to the first
                                                        visible index     */
        g_sp_hit_armed = (unsigned char)
            ((const unsigned char*)dp
             <= (const unsigned char*)g_sp_mouse_pixel);
        if (cnt != 0)
            goto anim_copy_run;
        goto anim_paint;
    anim_lc_step:
        if (--skip != 0) continue;
        budget = g_sp_w;
        goto anim_paint;
    }

    /* ---- paint: the visible span, `budget` pixels wide ----------------- */
anim_paint:
    for (;;) {
        code = ll_anim_code(&cs);
        if (!(code & 2)) {                           /* p5: one index     */
            cnt = 1;
            g_sp_hit_armed = (unsigned char)
                ((const unsigned char*)dp
                 == (const unsigned char*)g_sp_mouse_pixel);
            goto anim_copy_run;
        }
        if (!(code & 1)) {                           /* p_one             */
            /* `dec edx / add edi,2 / je endrow`.  The `je` reads the flags
             * of `add edi,2`, NOT of `dec edx` -- `add` writes ZF and the
             * output pointer plus two is never zero, so the row-end branch
             * here is DEAD and a transparent single always falls back into
             * the paint loop, budget or no budget.  Reproduced, not fixed;
             * see docs/lanes/scope-port-b3.md. */
            budget--;
            dp++;
            continue;
        }
        cnt = ll_anim_count(&cs);
        if (cnt == 0)
            goto anim_nextrow;
        code = ll_anim_code(&cs);
        g_sp_hit_armed = (unsigned char)
            ((const unsigned char*)dp
             <= (const unsigned char*)g_sp_mouse_pixel);
        if (code & 2) {                              /* skip run          */
            budget -= (int)cnt;
            if (budget < 0)
                goto anim_endrow;
            dp += cnt;
            continue;
        }
    anim_p4:
        if (!(code & 1))
            goto anim_copy_run;
        /* the repeat run.  `cmp edx,ecx / ja` is UNSIGNED. */
        if ((unsigned int)budget > cnt) {            /* fill_w: it fits   */
            v = pal[*ip];
            ip++;
            budget -= (int)cnt;
            do { *dp++ = v; } while (--cnt);         /* rep stosw         */
            ll_anim_hit(dp, g_sp_mouse_pixel, g_sp_hit_armed, &g_blit_hit);
            continue;
        }
        /* fill_d: the run fills the rest of the budget.  The asm builds a
         * doubled dword and `rep stosd`s it, emitting the odd leading pixel
         * by hand -- and the `mov [edi],ax` that does so happens BEFORE the
         * odd/even branch, so a zero budget still writes one word. */
        v = pal[*ip];
        ip++;
        drawn = (unsigned int)budget;
        *dp = v;
        if (drawn & 1)
            dp++;
        half = drawn >> 1;
        while (half--) { *dp++ = v; *dp++ = v; }
        ll_anim_hit(dp, g_sp_mouse_pixel, g_sp_hit_armed, &g_blit_hit);
        goto anim_endrow;

    anim_copy_run:
        if ((unsigned int)budget > cnt) {            /* crb: it fits      */
            budget -= (int)cnt;
            do {
                *dp++ = pal[*ip];
                ip++;
            } while (--cnt);
            ll_anim_hit(dp, g_sp_mouse_pixel, g_sp_hit_armed, &g_blit_hit);
            continue;
        }
        /* cr_loop: the run fills the rest of the budget; `xchg ecx,edx`
         * makes the drawn count the budget and leaves the clipped index
         * bytes to be stepped over at cr_end. */
        drawn = (unsigned int)budget;
        budget = (int)cnt - (int)drawn;
        if (drawn != 0) {
            cnt = drawn;
            do {
                *dp++ = pal[*ip];
                ip++;
            } while (--cnt);
            ll_anim_hit(dp, g_sp_mouse_pixel, g_sp_hit_armed, &g_blit_hit);
        }
        ip += budget;                                /* cr_end            */
        goto anim_endrow;
    }

    /* ---- endrow: run the stream on to the end-of-row marker ------------ */
anim_endrow:
    for (;;) {
        code = ll_anim_code(&cs);
        if (!(code & 2)) { ip++; continue; }
        if (!(code & 1)) continue;
        cnt = ll_anim_count(&cs);
        if (cnt == 0)
            goto anim_nextrow;
        code = ll_anim_code(&cs);
        if (code & 2) continue;
        if (code & 1) ip++;
        else ip += cnt;
    }

anim_nextrow:
    rowp += g_sp_rowlen;
    g_zb_row = rowp;
    dp = (unsigned short*)rowp;
    rows_left = g_sp_rows_left - 1;
    if (rows_left != 0)
        goto anim_row;

anim_done:
    /* The asm keeps g_zb_bits live in the global across every code read;
     * nothing else runs while this loop does, so the observable effect is
     * the value it leaves behind. */
    g_zb_bits = cs.bits;
    }
#endif
    }
}

/* -------------------------------------------------------------------------
 * The RLE frame list (ImageRec::type 3) -- recovered here
 * -------------------------------------------------------------------------
 * The record header is the same LLSRec as an animation's, but two more
 * fields are live: +0x04 int width and +0x08 int height, in PIXELS, of the
 * whole sprite.  They exist only for the mouse box test below.
 *
 * A frame is a 0x10-byte header followed by three concatenated blocks:
 *      +0x00 int size     total byte length; add it to reach the next frame
 *      +0x04 int ncodes   number of 16-bit CONTROL words
 *      +0x08 int ndata    byte length of the 8-bit index block
 *      +0x0c int          unused by the dispatcher
 *      +0x10             ncodes u16 control words   ("A")
 *      +0x10+ncodes*2    ndata index bytes          ("B")
 *      +0x10+ncodes*2+ndata  the tail block         ("C")
 * The three block pointers A, B and C are what every painter is handed; the
 * dispatcher itself never looks inside them.
 * ------------------------------------------------------------------------- */

typedef struct RleFrame {
    int  size;     /* +0x00 */
    int  ncodes;   /* +0x04 */
    int  ndata;    /* +0x08 */
    int  pad0c;    /* +0x0c */
    char body[1];  /* +0x10 */
} RleFrame;

/* The input/cursor block at 0x00813a40: the mouse point at +0x04. */
extern Pos g_mouse_point;   /* 0x00813a44 */

/* The eight specialised RLE painters this function dispatches to.  They live
 * in the 0x00466d80 .. 0x00468000 block and are not yet decompiled; the
 * naming records what the dispatcher's own clip tests prove each one is for.
 * All are cdecl.  The four "Hit" ones take four extra arguments (left, w, a
 * spare zero, and the mouse pixel) because they run the mouse hit test; the
 * other four are entered only when the cursor is outside the sprite's box,
 * and the unclipped member of that set does not even need left/w. */
extern void RLEPaintHitClipLR(void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top, int left, int w, int spare, void* mouse); /* 0x00466d80 */
extern void RLEPaintHitClipL (void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top, int left, int w, int spare, void* mouse); /* 0x00467180 */
extern void RLEPaintHitClipR (void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top, int left, int w, int spare, void* mouse); /* 0x004673f0 */
extern void RLEPaintHit      (void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top, int left, int w, int spare, void* mouse); /* 0x00467640 */
extern void RLEPaintClipLR   (void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top, int left, int w, int spare, void* mouse); /* 0x004677b0 */
extern void RLEPaintClipL    (void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top, int left, int w, int spare, void* mouse); /* 0x00467b00 */
extern void RLEPaintClipR    (void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top, int left, int w, int spare, void* mouse); /* 0x00467d10 */
extern void RLEPaintFast     (void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top);                                          /* 0x00467f00 */

/* One frame's worth of dispatch, expanded three times by the original (base
 * image, second pass, and the single-frame path).  The four-way choice is
 * purely geometric:
 *   the frame fits inside the clip window   -> the unclipped painter
 *   ... else left == 0                      -> right-clip only
 *   ... else w - left fits                  -> left-clip only
 *   ... else                                -> both edges clipped
 * doubled by whether the cursor is inside the sprite's box (`inbounds`), in
 * which case the mouse-aware variants run. */
#define PAINT_RLE_FRAME(p, f, inbounds, lls)                                    \
{                                                                              \
    void* a;                                                                   \
    void* b;                                                                   \
    void* c;                                                                   \
    int   nc;                                                                  \
                                                                               \
    a = (f)->body;                                                             \
    nc = (f)->ncodes;                                                          \
    b = (f)->body + nc * 2;                                                    \
    c = (f)->body + nc * 2 + (f)->ndata;                                       \
    if (inbounds) {                                                            \
        if ((lls)->w <= g_sp_w - g_sp_left)                                    \
            RLEPaintHit(p, a, b, c, g_sp_h, g_ddsd.lPitch, g_sp_top,           \
                        g_sp_left, g_sp_w, 0, g_sp_mouse_pixel);               \
        else if (g_sp_left != 0) {                                             \
            if ((lls)->w - g_sp_left > g_sp_w)                                 \
                RLEPaintHitClipLR(p, a, b, c, g_sp_h, g_ddsd.lPitch, g_sp_top, \
                                  g_sp_left, g_sp_w, 0, g_sp_mouse_pixel);     \
            else                                                               \
                RLEPaintHitClipL(p, a, b, c, g_sp_h, g_ddsd.lPitch, g_sp_top,  \
                                 g_sp_left, g_sp_w, 0, g_sp_mouse_pixel);      \
        } else {                                                               \
            RLEPaintHitClipR(p, a, b, c, g_sp_h, g_ddsd.lPitch, g_sp_top,      \
                             0, g_sp_w, 0, g_sp_mouse_pixel);                  \
        }                                                                      \
    } else {                                                                   \
        if ((lls)->w <= g_sp_w - g_sp_left)                                    \
            RLEPaintFast(p, a, b, c, g_sp_h, g_ddsd.lPitch, g_sp_top);         \
        else if (g_sp_left != 0) {                                             \
            if ((lls)->w - g_sp_left > g_sp_w)                                 \
                RLEPaintClipLR(p, a, b, c, g_sp_h, g_ddsd.lPitch, g_sp_top,    \
                               g_sp_left, g_sp_w, 0, g_sp_mouse_pixel);        \
            else                                                               \
                RLEPaintClipL(p, a, b, c, g_sp_h, g_ddsd.lPitch, g_sp_top,     \
                              g_sp_left, g_sp_w, 0, g_sp_mouse_pixel);         \
        } else {                                                               \
            RLEPaintClipR(p, a, b, c, g_sp_h, g_ddsd.lPitch, g_sp_top,         \
                          0, g_sp_w, 0, g_sp_mouse_pixel);                     \
        }                                                                      \
    }                                                                          \
}

/* -------------------------------------------------------------------------
 * 0x00466770 -- cdecl(lls, src, dst): paint one RLE frame (ImageRec::type 3)
 * into the locked surface.  Unlike every other blitter in the pair of files
 * this one is a pure-C DISPATCHER with no pixel loop of its own: it stamps
 * the SoftPrint scratch globals, decides which of eight specialised painters
 * fits the clip rectangle and the mouse, and calls it.
 *
 * The destination pointer is biased LEFT by the clip:
 *      p = lpSurface + dst->y*lPitch + (dst->x - src->left)*2
 * so a painter that has to skip `left` source pixels can advance the output
 * pointer over them and still land on dst->x.  `src` is not otherwise
 * consulted by the painters -- its four numbers reach them through the
 * globals (g_sp_left, g_sp_w, g_sp_top, g_sp_h).
 *
 * THE MOUSE BOX TEST.  Before anything else the function decides whether the
 * cursor is inside the sprite's bounding box at all
 *      dst->x <= mouse.x <= dst->x + lls->w  &&
 *      dst->y <= mouse.y <= dst->y + lls->h
 * (inclusive on both ends -- an off-by-one the original owns) and only then
 * picks a painter that runs the per-run hit test.  That is what lls->w and
 * lls->h at +0x04/+0x08 exist for.
 *
 * Frame selection is the house rule, with the SAME override bug as
 * SoftBlitAnim: g_frame_override when >= 0 else lls->frame, clamped to
 * nframes-1; but on a base-image record (flags bit 0) the second pass walks
 * `lls->frame + 1` frames, re-reading the record rather than using the
 * clamped/overridden value.  So an override on a base-image RLE sprite paints
 * the base image plus the record's own frame, not the requested one.
 * ------------------------------------------------------------------------- */

/* THREE LEVERS this body needed, all recorded because they recur in every
 * frame-list painter:
 *  1. the frame walk is `n = frame; while (n--)`, the SIGNED post-decrement
 *     `while` -- that is what makes VC6 load `frame` into the register `f`
 *     wants and spell the counter copy `lea ebp,[ebx]`.  `for (n = frame;
 *     n != 0; n--)` hoists the `f` load above the guard instead and needs no
 *     copy, which is three instructions and a whole block-layout different.
 *  2. `f->ncodes` is hoisted into a local (`nc`) inside the dispatch block.
 *     Reading the field twice lets VC6 schedule the `f->ndata` load two slots
 *     early and pick the other free register for `f`; with the local it reuses
 *     one register for both field values, exactly as the original does.
 *  3. the four-way clip test is written success-first
 *     (`if (w <= g_sp_w - left) <unclipped>; else if (left != 0) {...} else
 *     <right-clip>;`), which is what keeps the unclipped call INLINE and the
 *     three clipped ones out of line, in the original's block order.
 * The dispatch is a MACRO, not a `static __inline` helper: VC6 inlines the
 * first two calls and then gives up on the third, emitting a real call and
 * losing 150 instructions. */
// FUNCTION: LEGOLAND 0x00466770
void SoftBlitRLEPlain(LLSRec* lls, WinRect* src, Pos* dst)
{
    RleFrame*    f;
    int          frame;
    int          inbounds;
    int          n;
    unsigned int k;
    char*        p;

    g_sp_rowlen = g_ddsd.lPitch;
    f = (RleFrame*)lls->frames;
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
    inbounds = 0;
    g_sp_mouse_pixel = (char*)g_ddsd.lpSurface + g_mouse_point.y * g_ddsd.lPitch
                     + g_mouse_point.x * 2;
    if (g_mouse_point.x >= dst->x && g_mouse_point.x <= dst->x + lls->w
     && g_mouse_point.y >= dst->y && g_mouse_point.y <= dst->y + lls->h)
        inbounds = 1;
    p = (char*)g_ddsd.lpSurface + dst->y * g_ddsd.lPitch
      + (dst->x - g_sp_left) * 2;
    if (lls->flags & 1) {
        PAINT_RLE_FRAME(p, f, inbounds, lls)
        k = lls->frame + 1;
        while (k-- != 0)
            f = (RleFrame*)((char*)f + f->size);
        PAINT_RLE_FRAME(p, f, inbounds, lls)
    } else {
        n = frame;
        while (n--)   /* post-decrement form: VC6 copies the count with `lea` */
            f = (RleFrame*)((char*)f + f->size);
        PAINT_RLE_FRAME(p, f, inbounds, lls)
    }
}
