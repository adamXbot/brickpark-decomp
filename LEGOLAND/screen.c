/* LEGOLAND -- display setup, the class-callback table and the BMP/LLS loader.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field offsets, global addresses and callee arg counts are load-bearing;
 * names are ours.
 *
 * ---------------------------------------------------------------------------
 * The BMP / LLS decode path (__BMPLoader 0x0044e010) -- for the runtime
 * ---------------------------------------------------------------------------
 * LoadSourceImage (sprite2.c) = CreateSourceImage + __BMPLoader; on failure
 * the image record is KillImage'd.  __BMPLoader splits the extension off
 * image->name (_splitpath) and takes one of two paths.
 *
 * (a) ".lls" / ".llz" -- the file already holds a decoded LLS.  The whole
 *     member is slurped through RES (RES_GetFileSize + one RES_ReadFile) and
 *     used AS the record: only the animation fields are reset --
 *       lls->frame (+0x00) = 0, lls->delay (+0x02) = 4, lls->timer (+0x12) = 4
 *     -- then image->w/h come from the payload header at +0x04/+0x08 (u16),
 *     image->pal is cleared and image->type is 2 when the payload's format
 *     word (+0x0c) is 8 (single image + 256-entry palette) and 3 otherwise
 *     (0x10 = frame table).  In 565 mode LLS555To565 rewrites the palettes in
 *     place.  A `kind == 1` (detail-dependent) image whose detail-qualified
 *     path does not open is retried once with GetGFXFName(name, 0, 0).
 *
 * (b) anything else -- a Windows BMP read through RES:
 *       BITMAPFILEHEADER (14 B) then BITMAPINFO (0x2c B = BITMAPINFOHEADER +
 *       one RGBQUAD).  Honoured fields: bfOffBits, biWidth, biHeight,
 *       biBitCount, biCompression, biClrUsed.  biCompression must be 0 (any
 *       RLE is rejected) and biBitCount must be 8 or 24; anything else closes
 *       the file and returns 0.  biPlanes, biSizeImage and the pels-per-meter
 *       fields are ignored, and NEGATIVE heights are NOT handled (rows are
 *       always treated as bottom-up).  The palette offset is taken as
 *       "current file position + 40" -- i.e. a BITMAPINFOHEADER of exactly 40
 *       bytes is assumed, a V4/V5 header would mis-seek.
 *
 *       Row padding: the source stride is (width*bpp/8 + 3) & ~3.
 *
 *       8bpp: image->type = 0.  The padded rows are read verbatim into
 *         image->lls (stride*height bytes) and flipped IN PLACE row by row;
 *         only the first `width` bytes of each row are exchanged, so the pad
 *         bytes stay with the row they arrived on (harmless, but it means the
 *         buffer is not a clean bottom-up->top-down flip).  biClrUsed*4 bytes
 *         of RGBQUADs are read into a 256-entry scratch and converted into a
 *         fresh 0x208-byte record at image->pal: 256 16-bit entries starting
 *         at BYTE OFFSET 4 (the first two 16-bit slots and the last two are
 *         left untouched by the loader).  Note the conversion always walks
 *         256 entries regardless of biClrUsed, so a palette with fewer
 *         entries picks up the zeroed tail of the scratch.
 *
 *       24bpp: image->type = 1.  The padded rows are read into a scratch
 *         buffer and packed into a fresh width*height*2 buffer bottom-up, so
 *         the flip happens during conversion; image->pal stays 0.  The source
 *         row pointer is re-aligned UP to a 4-byte boundary at the start of
 *         every row rather than stepping by a precomputed stride.
 *
 *       Both 8bpp and 24bpp pack to 5-6-5 when g_screen_depth == 2 and to
 *       5-5-5 otherwise:
 *         565: ((r & ~7) << 8) | ((g & 0xfc) << 3) | (b >> 3)
 *         555: ((r & 0xf8) << 7) | ((g & 0xf8) << 2) | (b >> 3)
 *
 * ORIGINAL BUG (reproduced, do not "fix"): both BMP allocation-failure paths
 * call the game's free() on the ImageRec itself, which the caller (sprite2.c)
 * also owns and frees -- a double free on an out-of-memory BMP load.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

/* ---- CRT ---------------------------------------------------------------- */
void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)
extern void _splitpath(const char* path, char* drive, char* dir,
                       char* fname, char* ext);                /* 0x0049ec85 */
extern int  NameCompare(const char* a, const char* b);         /* 0x004aab90 (_stricmp) */

/* ---- game callees ------------------------------------------------------- */
extern void* HeapAlloc_w(unsigned int size);                   /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                              /* 0x0049e4d0 */
extern void  DBPrintf(const char* fmt, ...);                   /* 0x00453a20 */
extern void  DebugPrintf(const char* fmt, ...);                /* 0x0047f870 */
extern void* RES_OpenFile(const char* name);                   /* 0x00489b60 */
extern int   RES_ReadFile(void* file, void* buf, int len);     /* 0x00489cf0 */
#ifndef LEGOLAND_PORTABLE
extern void  RES_CloseFile(void* file);                        /* 0x00489de0 */
#else
extern int RES_CloseFile(void* file);                        /* 0x00489de0 */
#endif
extern int   RES_GetFileSize(void* file);                      /* 0x00489ce0 */
extern int   RES_GetFilePointer(void* file);                   /* 0x00489db0 */
extern int   RES_SetFilePointer(void* file, int pos);          /* 0x00489d70 */
extern char* GetGFXFName(const char* name, unsigned char type, char* buf);
                                                               /* 0x0044de90 */

/* ---- types -------------------------------------------------------------- */

/* The decoded-bitmap record (see LEGOLAND/layervis.c for the animation
 * fields). __BMPLoader fills +0x00/+0x02/+0x04/+0x08/+0x0c/+0x12 for a
 * ready-made .lls/.llz payload. */
typedef struct LLS {
    short          frame;       /* +0x00  forced to 0 */
    unsigned short delay;       /* +0x02  forced to 4 */
    short          width;       /* +0x04 */
    short          pad06;       /* +0x06 */
    short          height;      /* +0x08 */
    short          pad0a;       /* +0x0a */
    int            format;      /* +0x0c  8 = single image, 0x10 = frame table */
    short          count;       /* +0x10 */
    short          timer;       /* +0x12  forced to 4 */
    unsigned int   flags;       /* +0x14 */
} LLS;

/* ImageRec -- 0x18-byte header, the name copied inline after it (sprite2.c). */
typedef struct ImageRec {
    LLS*           lls;         /* +0x00  decoded bitmap */
    void*          pal;         /* +0x04  0x208-byte 16-bit palette record */
    short          w;           /* +0x08 */
    short          h;           /* +0x0a */
    unsigned short refs;        /* +0x0c */
    unsigned char  kind;        /* +0x0e */
    unsigned char  pad0f;       /* +0x0f */
    char*          name;        /* +0x10  -> +0x18 */
    int            type;        /* +0x14  0/1 raw BMP, 2/3 .lls */
} ImageRec;

/* The palette record hung off ImageRec+0x04: 0x208 bytes, 256 16-bit entries
 * starting at +0x04. */
typedef struct ImagePalette {
    unsigned short head[2];     /* +0x000 */
    short entry[256];           /* +0x004 */
    unsigned short tail[2];     /* +0x204 */
} ImagePalette;

typedef struct RGBQuad {
    unsigned char b, g, r, x;
} RGBQuad;

#pragma pack(push, 2)
typedef struct BmpFileHeader {
    unsigned short bfType;      /* +0x00 'BM' */
    unsigned long  bfSize;      /* +0x02 */
    unsigned short bfReserved1; /* +0x06 */
    unsigned short bfReserved2; /* +0x08 */
    unsigned long  bfOffBits;   /* +0x0a */
} BmpFileHeader;                /* 14 bytes */
#pragma pack(pop)

typedef struct BmpInfoHeader {
    unsigned long  biSize;          /* +0x00 */
    long           biWidth;         /* +0x04 */
    long           biHeight;        /* +0x08 */
    unsigned short biPlanes;        /* +0x0c */
    unsigned short biBitCount;      /* +0x0e */
    unsigned long  biCompression;   /* +0x10 */
    unsigned long  biSizeImage;     /* +0x14 */
    long           biXPelsPerMeter; /* +0x18 */
    long           biYPelsPerMeter; /* +0x1c */
    unsigned long  biClrUsed;       /* +0x20 */
    unsigned long  biClrImportant;  /* +0x24 */
} BmpInfoHeader;                    /* 40 bytes */

typedef struct BmpInfo {
    BmpInfoHeader bmiHeader;        /* +0x00 */
    RGBQuad       bmiColors[1];     /* +0x28 */
} BmpInfo;                          /* 44 bytes = the 0x2c the loader reads */

/* ---- globals ------------------------------------------------------------ */

/* 0 = 8-bit palettised, 1 = RGB555, 2 = RGB565. */
extern int g_screen_depth;                                     /* 0x00668088 */

extern void LLS555To565(LLS* lls);                             /* 0x0047d6a0 */

static const char kExtLLS[]     = ".lls";
static const char kExtLLZ[]     = ".llz";
static const char kFailPlain[]  = "Failed to load (%s)";
static const char kFailNL[]     = "Failed to load (%s)\n";
static const char kFailDot[]    = "Failed to load (%s). ";
static const char kLoadingBMP[] = "Loading BMP (%s)";

/* -------------------------------------------------------------------------
 * 0x0044e010 -- decode an image record's backing file into image->lls.
 *
 * Two paths, chosen by the extension of image->name:
 *
 *   .lls / .llz  the file already IS an LLS: slurp it whole, reset the
 *                animation fields (frame 0, delay 4, timer 4), take w/h from
 *                the payload header, tag the record type 2 (format 8) or 3
 *                (frame table) and, in 565 mode, rewrite the palettes.
 *
 *   anything else  a Windows BMP: BITMAPFILEHEADER + BITMAPINFO, uncompressed
 *                only (biCompression must be 0), 8bpp or 24bpp only. Rows are
 *                DWORD-padded and stored bottom-up.
 *
 *                8bpp  the padded rows are read verbatim into image->lls and
 *                      flipped in place row by row (only the first `w` bytes
 *                      of each row are swapped -- the pad bytes keep whatever
 *                      row they came in on); the BMP palette is converted to
 *                      555/565 into a fresh 0x208-byte record at image->pal.
 *                24bpp the padded rows are read into a scratch buffer and
 *                      packed into a w*h 16-bit image bottom-up, so the flip
 *                      happens during conversion; image->pal stays 0.
 *
 * NOTE (original bug, reproduced): both BMP allocation-failure paths call
 * free() on the ImageRec itself, which the caller also owns and frees.
 *
 * ---------------------------------------------------------------------------
 * MATCH STATE: 470/470 instructions, 1386/1386 bytes, index for index.
 *
 * The last two residuals (a 10-byte size excess and two swapped stack-home
 * pairs) both fell to type/scope levers, not to register tweaks:
 *
 *  1. The 16-bit destination is SIGNED.  `ImagePalette::entry` and the 24bpp
 *     packer's output pointers are `short`, not `unsigned short`.  With an
 *     unsigned destination VC6's narrowing pass takes the red channel's
 *     `& ~7` down to the 16-bit operand width (`and dx, 0FFF8h`, 5 bytes);
 *     with a signed one it leaves the mask 32-bit (`and edx, -8`, 3 bytes) on
 *     top of the same 16-bit `movzx dx, byte ptr [..]` load the original has.
 *     Those 2 x 3 bytes (0x0044e359 and 0x0044e496) were also what pushed the
 *     `jne` at 0x0044e4d7 from rel8 to rel32, i.e. all 10 excess bytes.  The
 *     red channel must still be spelled `(r & ~7) * 32` with `r` an unsigned
 *     short: the MULTIPLY is what widens the byte before masking, and the
 *     `<< 5` spelling masks the byte instead (`and dl, 0F8h`).
 *  2. The 24bpp packer's frame homes follow the SCOPE nesting, not the
 *     declaration order.  `y` is declared in the block that encloses both
 *     depth branches while `row`/`p`/`src`/`x` are declared inside each
 *     branch, so `y` is the enclosing block-scope variable live across two
 *     disjoint inner blocks -- the shape DECOMP.md's frame-layout rule
 *     describes -- and VC6 gives that OUTER variable the LOWER home
 *     (y at +10h, row at +14h).  With all six in one block the pair comes out
 *     swapped, and no permutation of their declaration order moves them.
 *     Narrowing the inner scopes also re-widened the block pool by one slot,
 *     which is what put `offbits` at +14h and `palpos` at +1Ch (they were
 *     swapped too, and their own declaration order never mattered).
 *
 * Resolved earlier (each was worth 50-150 instructions):
 *  - The 8bpp flip's row stride must NOT be a named local.  As `int stride`
 *    it gets a home slot, VC6 emits a DEAD spill of it inside the loop, and
 *    the extra frame pressure flips the whole function's two callee-saved
 *    webs ({path, .lls size, raw} and {.lls file, BMP size}) from ebp/ebx to
 *    ebx/ebp -- 41 instructions differing by nothing but that substitution.
 *    Written twice as an expression it is CSE'd into ebx with no home and
 *    the register assignment falls into place.
 *  - Which of two same-width locals lands in edx and which in edi follows
 *    the order they are ASSIGNED, so the flip loop assigns the bottom
 *    walker first, and the 24bpp packer steps p in the for-increment (after
 *    x) rather than with `*p++` in the body.
 *  - The `.lls`/`.llz` test must be written as two `goto native` statements
 *    followed by `goto bitmap` -- the equivalent `if (a && b) goto bitmap;`
 *    makes VC6 emit je/je and inline the BMP block, where the original has
 *    je <native> / jne <bitmap> with the .lls block as the fall-through.
 *  - The BMP open-failure block must appear BEFORE the .lls block in the
 *    SOURCE (as a `bmpfail:` label the BMP path gotos back to).  VC6's
 *    cross-jumping always keeps the SOURCE-LATER copy of two identical
 *    tails, and the original keeps the .lls one.
 *  - In the 8bpp switch arm, `type` must be assigned AFTER the size
 *    computation: with both arms ending in the same `and`/`imul` pair VC6
 *    cross-jumps them into the join, where the original keeps both copies.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0044e010
int __BMPLoader(ImageRec* image)
{
    RGBQuad        palbuf[256];
    char           ext[256];
    BmpFileHeader  fhdr;
    BmpInfo        bmi;
    void*          f;
    void*          fl;
    char*          path;
    LLS*           lls;
    unsigned char* raw;
    int            size;
    int            llssize;
    int            type;
    int            bits;
    int            offbits;
    int            palpos;
    int            i;

    _splitpath(image->name, 0, 0, 0, ext);
    path = GetGFXFName(image->name, image->kind, 0);

    /* Written with explicit gotos: VC6 SP3 lowers the equivalent
     * `if (a != 0 && b != 0) goto bitmap;` with the BMP block INLINE and the
     * .lls block sunk to the end.  The two-goto form is the only spelling
     * that keeps the .lls block as the fall-through, as the original has it
     * (je <native> / jne <bitmap>). */
    if (NameCompare(kExtLLS, ext) == 0)
        goto native;
    if (NameCompare(kExtLLZ, ext) == 0)
        goto native;
    goto bitmap;

bmpfail:
    DebugPrintf(kFailDot, path);
    DBPrintf(kFailNL, path);
    return 0;

native:
    {
        fl = RES_OpenFile(path);
        if (!fl) {
            if (image->kind == 1) {
                path = GetGFXFName(image->name, 0, 0);
                fl = RES_OpenFile(path);
            }
            if (!fl) {
                DebugPrintf(kFailPlain, path);
                DBPrintf(kFailNL, path);
                return 0;
            }
        }
        llssize = RES_GetFileSize(fl);
        lls = (LLS*)HeapAlloc_w(llssize);
        RES_ReadFile(fl, lls, llssize);
        lls->frame = 0;
        lls->delay = 4;
        lls->timer = 4;
        RES_CloseFile(fl);
        image->pal = 0;
        image->w = lls->width;
        image->h = lls->height;
        if (lls->format == 8)
            image->type = 2;
        else
            image->type = 3;
        image->lls = lls;
        if (g_screen_depth == 2)
            LLS555To565(lls);
        return 1;
    }

bitmap:
    f = RES_OpenFile(path);
    if (!f)
        goto bmpfail;
    DebugPrintf(kLoadingBMP, path);

    RES_ReadFile(f, &fhdr, sizeof(fhdr));
    offbits = fhdr.bfOffBits;
    palpos = RES_GetFilePointer(f) + 40;
    RES_ReadFile(f, &bmi, sizeof(bmi));

    if (bmi.bmiHeader.biCompression) {
        RES_CloseFile(f);
        return 0;
    }

    bits = bmi.bmiHeader.biBitCount;
    switch (bits) {
    case 8:
        size = ((bmi.bmiHeader.biWidth + 3) & ~3) * bmi.bmiHeader.biHeight;
        type = 0;
        break;
    case 24:
        type = 1;
        size = ((bmi.bmiHeader.biWidth * 3 + 3) & ~3) * bmi.bmiHeader.biHeight;
        break;
    default:
        RES_CloseFile(f);
        return 0;
    }

    RES_SetFilePointer(f, fhdr.bfOffBits);
    raw = (unsigned char*)HeapAlloc_w(size);
    if (!raw) {
        HeapFree_w(image);
        RES_CloseFile(f);
        return 0;
    }

    image->pal = 0;
    image->w = (short)bmi.bmiHeader.biWidth;
    image->h = (short)bmi.bmiHeader.biHeight;
    image->type = type;

    if (bmi.bmiHeader.biBitCount == 8) {
        unsigned char* top;
        unsigned char* bot;
        unsigned char* pb;      /* walks the bottom row */
        unsigned char* pt;      /* walks the top row */

        RES_SetFilePointer(f, palpos);
        memset(palbuf, 0, sizeof(palbuf));
        RES_ReadFile(f, palbuf, bmi.bmiHeader.biClrUsed * 4);
        RES_SetFilePointer(f, offbits);
        RES_ReadFile(f, raw, size);

        top = raw;
        bot = raw + (image->h - 1) * ((image->w + 3) & ~3);
        image->lls = (LLS*)raw;
        while (bot > top) {
            /* The stride must NOT be a named local: giving it a home slot
             * costs a dead spill inside the loop and (because that slot
             * contends with the rest of the frame) flips the whole
             * function's ebx/ebp assignment.  Spelled twice, VC6 CSEs it
             * into ebx with no home, exactly as the original.  The bottom
             * walker is assigned FIRST so it lands in edx and the top
             * walker in edi, and the counter is incremented before the two
             * pointers, which is what the original's inc order shows. */
            pb = bot;
            pt = top;
            top += (image->w + 3) & ~3;
            bot -= (image->w + 3) & ~3;
            i = 0;
            while (i < image->w) {
                unsigned char c1 = *pb;
                unsigned char c2 = *pt;
                *pt = c1;
                *pb = c2;
                i++;
                pb++;
                pt++;
            }
        }

        image->pal = HeapAlloc_w(0x208);
        if (g_screen_depth == 2) {
            for (i = 0; i < 256; i++) {
                unsigned short r = palbuf[i].r;
                unsigned char  g = (unsigned char)(palbuf[i].g & 0xfc);
                unsigned char  bl = (unsigned char)(palbuf[i].b >> 3);
                ((ImagePalette*)image->pal)->entry[i] =
                    (short)((((r & ~7) * 32) | g) * 8 | bl);
            }
        } else {
            for (i = 0; i < 256; i++) {
                unsigned char  r = (unsigned char)(palbuf[i].r & 0xf8);
                unsigned char  g = (unsigned char)(palbuf[i].g & 0xf8);
                unsigned char  bl = (unsigned char)(palbuf[i].b >> 3);
                ((ImagePalette*)image->pal)->entry[i] =
                    (short)((((unsigned short)r << 5
                             | (unsigned short)g) << 2)
                            | (unsigned short)bl);
            }
        }
        RES_CloseFile(f);
        return 1;
    }

    RES_ReadFile(f, raw, size);
    {
    short* out;
    int             y;
    /* Through `size` (dead since the read above): passing the product
     * straight to the call computes it in ecx, the original uses eax. */
    size = bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight * 2;
    image->lls = (LLS*)HeapAlloc_w(size);
    out = (short*)image->lls;
    if (!out) {
        HeapFree_w(raw);
        HeapFree_w(image);
        RES_CloseFile(f);
        return 0;
    }

    if (g_screen_depth == 2) {
        short* row;
        short* p;
        unsigned char* src;
        int x;

        row = out + (image->h - 1) * image->w;
        src = raw;
        for (y = 0; y < image->h; y++) {
            src = (unsigned char*)(((unsigned int)src + 3) & ~3);
            p = row;
            row = p - image->w;
            /* p and src step in the for-increment, after x: with `*p++`
             * and `src += 3` in the body VC6 gives the row pointer edx and
             * the counter edi, the reverse of the original, and schedules
             * the three increments the other way round. */
            for (x = 0; x < image->w; x++, p++, src += 3) {
                unsigned short r = src[2];
                unsigned char  g = (unsigned char)(src[1] & 0xfc);
                unsigned char  b = (unsigned char)(src[0] >> 3);
                *p = (short)((((r & ~7) * 32) | g) * 8 | b);
            }
        }
    } else {
        short* row;
        short* p;
        unsigned char* src;
        int x;

        row = out + (image->h - 1) * image->w;
        src = raw;
        for (y = 0; y < image->h; y++) {
            src = (unsigned char*)(((unsigned int)src + 3) & ~3);
            p = row;
            row = p - image->w;
            for (x = 0; x < image->w; x++, p++, src += 3) {
                unsigned char r = (unsigned char)(src[2] & 0xf8);
                unsigned char g = (unsigned char)(src[1] & 0xf8);
                unsigned char b = (unsigned char)(src[0] >> 3);
                *p = (short)((((unsigned short)r << 5
                                | (unsigned short)g) << 2)
                               | (unsigned short)b);
            }
        }
    }

    HeapFree_w(raw);
    RES_CloseFile(f);
    return 1;
    }
}

/* =========================================================================
 * 0x00452c20 -- SetCustomCallbacks: the class-name -> callback-set table.
 *
 * LLIDB_LoadODFData calls this at the end of every .ODF load (after
 * SetStandardCallbacks and the optional LoadObjectLibrary).  It is a straight
 * if/else-if chain of case-insensitive name compares against the class name
 * (RideElem +0x00) that installs per-class handlers into the 0xd0-byte ObjDef
 * hanging off RideElem +0x0c; the slots are the same +0x8c..+0xc0 that the
 * ride *_GetInterfaces functions in ridesave.c fill.
 *
 * Slot meanings (as far as the ride code shows): +0x8c tick/update,
 * +0x90/+0x94 secondary updates, +0x98 place/add, +0x9c remove, +0xa0 draw,
 * +0xa4 create/attach, +0xa8 activate, +0xac destroy, +0xb0 interact,
 * +0xb8 load, +0xbc save, +0xc0 extra.
 *
 * Three groups share one handler set through an OR of name compares
 * (FOUNTAIN 1/2/3, "Dino Big"/"Dino Small"/"Dino Mini") -- VC6 sinks the
 * `||` arm's block to the end of the function, which is exactly where the
 * original has them (0x00453828 and 0x0045384e).
 *
 * After the chain, EVERY ride module's *_GetInterfaces is called
 * unconditionally with (elem, def); each one re-tests the class name itself
 * (see Joust_GetInterfaces in ridesave.c) so only the matching module fills
 * anything in.  That is why the ride files carry their own name compares.
 *
 * Note the argument order of the compares flips halfway through the table:
 * the first ten are NameCompare(elem->name, "..."), the rest are
 * NameCompare("...", elem->name).  Both orders are load-bearing (they decide
 * which operand is pushed first).
 * ========================================================================= */

/* The 0xd0-byte ObjDef and the LLIDB element that owns it (same layout as
 * LEGOLAND/ridesave.c, which fills the same slots from the ride side). */
typedef struct RideDef RideDef;

typedef struct RideElem {
    char*        name;          /* +0x00  class name */
    char*        image;         /* +0x04 */
    unsigned int flags;         /* +0x08 */
    RideDef*     data;          /* +0x0c  the ObjDef this fills in */
} RideElem;

struct RideDef {
    unsigned char pad00[0x8c];
    void* cb_8c;                /* +0x8c */
    void* cb_90;                /* +0x90 */
    void* cb_94;                /* +0x94 */
    void* cb_add;               /* +0x98 */
    void* cb_remove;            /* +0x9c */
    void* cb_a0;                /* +0xa0 */
    void* cb_a4;                /* +0xa4 */
    void* cb_a8;                /* +0xa8 */
    void* cb_ac;                /* +0xac */
    void* cb_b0;                /* +0xb0 */
    void* cb_b4;                /* +0xb4 */
    void* cb_load;              /* +0xb8 */
    void* cb_save;              /* +0xbc */
    void* cb_c0;                /* +0xc0 */
};

/* ---- the ride/class callback providers the tail calls dispatch to ----- */
extern void CastleLevel1_GetInterfaces     (RideElem* elem, RideDef* def); /* 0x00403080  CASTLE LEVEL 1 */
extern void LogFlume_GetInterfaces         (RideElem* elem, RideDef* def); /* 0x00410d60  LOG FLUME * */
extern void Copters_GetInterfaces          (RideElem* elem, RideDef* def); /* 0x00405110  COPTERS */
extern void Fort_GetInterfaces             (RideElem* elem, RideDef* def); /* 0x004068b0  FORT */
extern void GoldRush_GetInterfaces         (RideElem* elem, RideDef* def); /* 0x004078f0  GOLD RUSH */
extern void Temple_GetInterfaces           (RideElem* elem, RideDef* def); /* 0x00416e50  TEMPLE */
extern void Catapult_GetInterfaces         (RideElem* elem, RideDef* def); /* 0x00403bb0  CATAPULT */
extern void Joust_GetInterfaces            (RideElem* elem, RideDef* def); /* 0x00408db0  JOUST */
extern void TempleSlide_GetInterfaces      (RideElem* elem, RideDef* def); /* 0x00417a00  TEMPLE SLIDE */
extern void SpiderRide_GetInterfaces       (RideElem* elem, RideDef* def); /* 0x00416160  SPIDER RIDE */
extern void SafariRide_GetInterfaces       (RideElem* elem, RideDef* def); /* 0x00415030  SAFARI RIDE */
extern void WaterWorks_GetInterfaces       (RideElem* elem, RideDef* def); /* 0x00418c80  WATER WORKS * */
extern void Garden_GetInterfaces           (RideElem* elem, RideDef* def); /* 0x004329c0  HEDGE / FLOWERS */
extern void WesternTown_GetInterfaces      (RideElem* elem, RideDef* def); /* 0x0043a400  GENERAL STORE / SHERIFF / JAIL CELL / BANK */
extern void SpaceTower_GetInterfaces       (RideElem* elem, RideDef* def); /* 0x0043b780  SPACE TOWER RIDE */
extern void SpinningBarrels_GetInterfaces  (RideElem* elem, RideDef* def); /* 0x0043c760  SPINNING BARRELS RIDE */
extern void PlaneRide_GetInterfaces        (RideElem* elem, RideDef* def); /* 0x0043e220  PLANE RIDE */
extern void CastleObj_GetInterfaces        (RideElem* elem, RideDef* def); /* 0x004254d0  CASTLE OBJ / CASTLE_DUMMY */

/* ---- path-control and the shared sound-object handlers ---------------- */
extern void AddBasicPath             ();  /* 0x0045dbe0 */
extern void RemoveBasicPath          ();  /* 0x0045dc90 */
extern void DrawBasicPath            ();  /* 0x0045dcf0 */
extern void Fountain_Add             ();  /* 0x004529e0 */
#ifndef LEGOLAND_PORTABLE
extern void Fountain_AC              ();  /* 0x004529c0 */
#else
extern void Fountain_AC(void);  /* 0x004529c0 */
#endif
extern void Fountain_InitSound       ();  /* 0x00452990 */
extern void RemoveSoundObject        ();  /* 0x00452a30 */
extern void CrystalPowerStation_Add  ();  /* 0x00452b20 */
extern void SmallPowerStation_Add    ();  /* 0x00452ad0 */
#ifndef LEGOLAND_PORTABLE
extern void PowerStation_AC          ();  /* 0x00452ab0 */
#else
extern void PowerStation_AC(void);  /* 0x00452ab0 */
#endif
extern void PowerStation_InitSound   ();  /* 0x00452a80 */
extern void Dino_Add                 ();  /* 0x00452bc0 */
#ifndef LEGOLAND_PORTABLE
extern void Dino_AC                  ();  /* 0x00452ba0 */
#else
extern void Dino_AC(void);  /* 0x00452ba0 */
#endif
extern void Dino_InitSound           ();  /* 0x00452b70 */

/* ---- per-class handlers that live in the ride translation units ------- */
extern void DrivingSchool_Create();                              /* 0x00405370 */
extern void DrivingSchool_Destroy();                              /* 0x00405460 */
extern void DrivingSchool_SelectForPlacement();                              /* 0x00405570 */
extern void DrivingSchool_Add();                              /* 0x00405630 */
extern void DrivingSchool_Update();                              /* 0x00405740 */
extern void DrivingSchool_DrawSelection();                              /* 0x004058a0 */
extern void DrivingSchool_Remove();                              /* 0x00405940 */
extern void DrivingSchool_GetDrawDesc();                              /* 0x00405ad0 */
extern void DrivingSchool_Draw();                              /* 0x00405b10 */
extern void DrivingSchool_TickRiders();                              /* 0x00405bd0 */
#ifndef LEGOLAND_PORTABLE
extern void SaveDrivingSchool();                              /* 0x00405e70 */
#else
extern int SaveDrivingSchool(void);                              /* 0x00405e70 */
#endif
extern void DrivingSchool_BestTake();                              /* 0x00406050 */
#ifndef LEGOLAND_PORTABLE
extern void LoadDrivingSchool();                              /* 0x00406070 */
#else
extern int LoadDrivingSchool(void);                              /* 0x00406070 */
#endif
extern void Pump_LoadResources();                              /* 0x00411a10 */
extern void Pump_SelectForPlacement();                              /* 0x00411a20 */
extern void Pump_Add();                              /* 0x00411bf0 */
extern void Pumps_Remove();                              /* 0x00411c70 */
extern void Pump_Update();                              /* 0x00411cd0 */
extern void Roads_LoadResources();                              /* 0x00413a10 */
#ifndef LEGOLAND_PORTABLE
extern void Roads_FreeResources();                              /* 0x00413a80 */
#else
extern void Roads_FreeResources(void);                              /* 0x00413a80 */
#endif
extern void Roads_SelectForPlacement();                              /* 0x00413ad0 */
extern void Roads_CalcCursor();                              /* 0x00413b50 */
extern void Road_DrawSelection();                              /* 0x00413fa0 */
extern void Roads_Add();                              /* 0x00414020 */
extern void Roads_Remove();                              /* 0x00414220 */
extern void ZebraCrossing_SelectForPlacement();                              /* 0x00414830 */
extern void ZebraCrossing_Update();                              /* 0x00414880 */
extern void ZebraCrossing_LoadResources();                              /* 0x00414940 */
extern void ZebraCrossing_Add();                              /* 0x00414950 */
extern void BoatingSchool_Create();                              /* 0x00419d10 */
extern void BoatingSchool_Destroy();                              /* 0x00419ef0 */
extern void BoatingSchool_SelectForPlacement();                              /* 0x0041a000 */
extern void BoatingSchool_Add();                              /* 0x0041a040 */
extern void BoatingSchool_Update();                              /* 0x0041a2f0 */
extern void BoatingSchool_DrawSelection();                              /* 0x0041a3d0 */
extern void BoatingSchool_Remove();                              /* 0x0041a530 */
#ifndef LEGOLAND_PORTABLE
extern void BoatingSchool_Tick();                              /* 0x0041a720 */
#else
extern void BoatingSchool_Tick(void);                              /* 0x0041a720 */
#endif
extern void BoatingSchool_Draw();                              /* 0x0041abd0 */
#ifndef LEGOLAND_PORTABLE
extern void SaveBoatingSchool();                              /* 0x0041acf0 */
#else
extern int SaveBoatingSchool(void);                              /* 0x0041acf0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void LoadBoatingSchool();                              /* 0x0041aee0 */
#else
extern int LoadBoatingSchool(void);                              /* 0x0041aee0 */
#endif
extern void BoatingSchool_BestTake();                              /* 0x0041b100 */
extern void Mermaid_LoadResources();                              /* 0x0041b250 */
extern void Mermaid_SelectForPlacement();                              /* 0x0041b260 */
extern void Mermaid_Add();                              /* 0x0041b2a0 */
extern void Mermaid_CalcCursor();                              /* 0x0041b4c0 */
extern void Mermaid_CalcCursor2();                              /* 0x0041b6d0 */
extern void BsMermaid_Remove();                              /* 0x0041b6f0 */
extern void BsWater_LoadResources();                              /* 0x0041b830 */
extern void BsWater_SelectForPlacement();                              /* 0x0041b880 */
extern void BsWater_Add();                              /* 0x0041b8e0 */
extern void BsWater_CalcCursor();                              /* 0x0041bd40 */
extern void BsWater_DrawSelection();                              /* 0x0041bfb0 */
extern void BoatingSchoolWater_Remove();                              /* 0x0041c130 */
extern void Balloonz_Create();                              /* 0x0042a7b0 */
extern void Balloonz_Add();                              /* 0x0042a950 */
extern void Balloonz_Remove();                              /* 0x0042aa10 */
extern void Balloonz_Tick();                              /* 0x0042aa90 */
extern void Balloonz_GetDrawDesc();                              /* 0x0042b2a0 */
extern void Balloonz_Draw();                              /* 0x0042b2e0 */
#ifndef LEGOLAND_PORTABLE
extern void Balloonz_Destroy();                              /* 0x0042b9d0 */
#else
extern void Balloonz_Destroy(void);                              /* 0x0042b9d0 */
#endif
extern void Balloonz_SelectForPlacement();                              /* 0x0042ba40 */
#ifndef LEGOLAND_PORTABLE
extern void Balloonz_Save();                              /* 0x0042ba80 */
#else
extern int Balloonz_Save(void);                              /* 0x0042ba80 */
#endif
extern void Balloonz_Load();                              /* 0x0042baf0 */
extern void Carousel_Draw();                              /* 0x0042bcf0 */
extern void Carousel_Create();                              /* 0x0042c280 */
extern void Carousel_Destroy();                              /* 0x0042c3f0 */
extern void Carousel_SelectForPlacement();                              /* 0x0042c460 */
extern void Carousel_Remove();                              /* 0x0042c4a0 */
extern void Carousel_Add();                              /* 0x0042c520 */
extern void Carousel_GetDrawDesc();                              /* 0x0042c550 */
#ifndef LEGOLAND_PORTABLE
extern void Carousel_Save();                              /* 0x0042c590 */
#else
extern int Carousel_Save(void);                              /* 0x0042c590 */
#endif
extern void Carousel_Load();                              /* 0x0042c600 */
extern void Carousel_Tick();                              /* 0x0042c820 */
extern void EarthSlide_Draw();                              /* 0x0042d070 */
extern void EarthSlide_Create();                              /* 0x0042d100 */
extern void EarthSlide_FreeResources();                              /* 0x0042d1f0 */
extern void EarthSlide_SelectForPlacement();                              /* 0x0042d230 */
extern void EarthSlide_Remove();                              /* 0x0042d270 */
extern void EarthSlide_Add();                              /* 0x0042d2c0 */
#ifndef LEGOLAND_PORTABLE
extern void EarthSlide_Save();                              /* 0x0042d2f0 */
#else
extern int EarthSlide_Save(void);                              /* 0x0042d2f0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void EarthSlide_Load();                              /* 0x0042d400 */
#else
extern int EarthSlide_Load(void);                              /* 0x0042d400 */
#endif
extern void EarthSlide_Tick();                              /* 0x0042d610 */
extern void Entrance1_Draw();                              /* 0x0042d9c0 */
extern void Entrance1_Create();                              /* 0x0042de50 */
extern void Entrance1_Destroy();                              /* 0x0042def0 */
extern void Entrance1_Remove();                              /* 0x0042df70 */
extern void Entrance1_Tick();                              /* 0x0042dfa0 */
extern void ChuckWagon_LoadResources();                              /* 0x0042e220 */
#ifndef LEGOLAND_PORTABLE
extern void ChuckWagon_FreeResources();                              /* 0x0042e250 */
#else
extern void ChuckWagon_FreeResources(void);                              /* 0x0042e250 */
#endif
extern void ChuckWagon_DrawOverlay();                              /* 0x0042e260 */
extern void ChuckWagon_TickCustomers();                              /* 0x0042e2a0 */
extern void Brolly_LoadResources();                              /* 0x0042e460 */
#ifndef LEGOLAND_PORTABLE
extern void Brolly_FreeResources();                              /* 0x0042e4b0 */
#else
extern void Brolly_FreeResources(void);                              /* 0x0042e4b0 */
#endif
extern void Brolly_SelectForPlacement();                              /* 0x0042e4c0 */
extern void Brolly_Add();                              /* 0x0042e500 */
extern void Brolly_GetDrawDesc();                              /* 0x0042e560 */
extern void SharkCafe_LoadResources();                              /* 0x0042e5d0 */
#ifndef LEGOLAND_PORTABLE
extern void SharkCafe_FreeResources();                              /* 0x0042e600 */
#else
extern void SharkCafe_FreeResources(void);                              /* 0x0042e600 */
#endif
extern void SharkCafe_Tick();                              /* 0x0042e610 */
extern void FoodcartDrink_LoadResources();                              /* 0x0042e770 */
#ifndef LEGOLAND_PORTABLE
extern void FoodcartDrink_FreeResources();                              /* 0x0042e7a0 */
#else
extern void FoodcartDrink_FreeResources(void);                              /* 0x0042e7a0 */
#endif
extern void FoodcartIcecream_LoadResources();                              /* 0x0042e7b0 */
#ifndef LEGOLAND_PORTABLE
extern void FoodcartIcecream_FreeResources();                              /* 0x0042e7e0 */
#else
extern void FoodcartIcecream_FreeResources(void);                              /* 0x0042e7e0 */
#endif
extern void FoodcartFood_LoadResources();                              /* 0x0042e7f0 */
#ifndef LEGOLAND_PORTABLE
extern void FoodcartFood_FreeResources();                              /* 0x0042e820 */
#else
extern void FoodcartFood_FreeResources(void);                              /* 0x0042e820 */
#endif
extern void Foodcart_DrawOverlay();                              /* 0x0042e830 */
extern void CastleBbq_LoadResources();                              /* 0x0042e870 */
#ifndef LEGOLAND_PORTABLE
extern void CastleBbq_FreeResources();                              /* 0x0042e8b0 */
#else
extern void CastleBbq_FreeResources(void);                              /* 0x0042e8b0 */
#endif
extern void Food_SelectForPlacement();                              /* 0x0042e8d0 */
extern void CastleBbq_Draw();                              /* 0x0042e910 */
extern void CastleBbq_Add();                              /* 0x0042e9c0 */
extern void CastleBbq_Remove();                              /* 0x0042ea10 */
extern void CastleBbq_Tick();                              /* 0x0042ea60 */
extern void FoodcartDrink_Tick();                              /* 0x0042ec10 */
extern void FoodcartFood_Tick();                              /* 0x0042ed70 */
extern void Restaurant1_Add();                              /* 0x0042ef10 */
extern void Restaurant1_Remove();                              /* 0x0042efb0 */
extern void Restaurant1_Create();                              /* 0x0042f030 */
extern void Restaurant1_Tick();                              /* 0x0042f1a0 */
extern void Restaurant1_Draw();                              /* 0x0042f4c0 */
#ifndef LEGOLAND_PORTABLE
extern void Restaurant1_FreeResources();                              /* 0x0042f720 */
#else
extern void Restaurant1_FreeResources(void);                              /* 0x0042f720 */
#endif
extern void Restaurant2_Create();                              /* 0x0042f770 */
extern void Restaurant2_Add();                              /* 0x0042f9a0 */
extern void Restaurant2_Remove();                              /* 0x0042fa40 */
extern void Restaurant2_Tick();                              /* 0x0042fbb0 */
extern void Restaurant2_GetDrawDesc();                              /* 0x004304a0 */
extern void Restaurant2_Draw();                              /* 0x00430b10 */
#ifndef LEGOLAND_PORTABLE
extern void Restaurant2_Destroy();                              /* 0x00431120 */
#else
extern void Restaurant2_Destroy(void);                              /* 0x00431120 */
#endif
extern void FoodcartIcecream_Tick();                              /* 0x00431170 */
extern void FoodService_Remove();                              /* 0x004312c0 */
extern void OctopusCafe_Create();                              /* 0x00431300 */
extern void OctopusCafe_Add();                              /* 0x004314f0 */
#ifndef LEGOLAND_PORTABLE
extern void OctopusCafe_Destroy();                              /* 0x00431520 */
#else
extern void OctopusCafe_Destroy(void);                              /* 0x00431520 */
#endif
extern void OctopusCafe_Tick();                              /* 0x004316f0 */
extern void OctopusCafe_Draw();                              /* 0x00431d00 */
#ifndef LEGOLAND_PORTABLE
extern void Restaurant1_Save();                              /* 0x004322a0 */
#else
extern int Restaurant1_Save(void);                              /* 0x004322a0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void Restaurant1_Load();                              /* 0x00432310 */
#else
extern int Restaurant1_Load(void);                              /* 0x00432310 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void Restaurant2_Save();                              /* 0x00432390 */
#else
extern int Restaurant2_Save(void);                              /* 0x00432390 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void Restaurant2_Load();                              /* 0x00432400 */
#else
extern int Restaurant2_Load(void);                              /* 0x00432400 */
#endif
extern void JcMonkeyTree_Create();                              /* 0x00433ca0 */
#ifndef LEGOLAND_PORTABLE
extern void JungleCruiseMonkeyTree_Destroy();                              /* 0x00433cd0 */
#else
extern void JungleCruiseMonkeyTree_Destroy(void);                              /* 0x00433cd0 */
#endif
extern void JcMonkeyTree_SelectForPlacement();                              /* 0x00433ce0 */
extern void MonkeyTree_Add();                              /* 0x00433d20 */
extern void MonkeyTree_CalcCursor();                              /* 0x00433d90 */
extern void JungleCruiseMonkeyTree_DrawSelection();                              /* 0x00433fa0 */
extern void MonkeyTree_Remove();                              /* 0x00433fc0 */
extern void JcMonkeyTree_GetDrawDesc();                              /* 0x00434040 */
extern void JcMonkeyFish_Create();                              /* 0x00434080 */
#ifndef LEGOLAND_PORTABLE
extern void JungleCruiseMonkeyFish_Destroy();                              /* 0x004340b0 */
#else
extern void JungleCruiseMonkeyFish_Destroy(void);                              /* 0x004340b0 */
#endif
extern void JcMonkeyFish_SelectForPlacement();                              /* 0x004340c0 */
extern void MonkeyFish_Add();                              /* 0x00434100 */
extern void MonkeyFish_CalcCursor();                              /* 0x00434330 */
extern void JungleCruiseMonkeyFish_DrawSelection();                              /* 0x00434650 */
extern void MonkeyFish_Remove();                              /* 0x00434670 */
extern void JcMonkeyFish_GetDrawDesc();                              /* 0x00434740 */
extern void JungleCruise_Create();                              /* 0x00434cb0 */
extern void JungleCruise_Destroy();                              /* 0x00434e50 */
extern void JungleCruise_SelectForPlacement();                              /* 0x00434f50 */
extern void JungleCruise_Add();                              /* 0x00434f90 */
extern void JungleCruise_Update();                              /* 0x00435150 */
extern void JungleCruise_DrawSelection();                              /* 0x00435230 */
extern void JungleCruise_Remove();                              /* 0x00435470 */
#ifndef LEGOLAND_PORTABLE
extern void JungleCruise_Tick();                              /* 0x00435750 */
#else
extern void JungleCruise_Tick(void);                              /* 0x00435750 */
#endif
extern void JungleCruise_Draw();                              /* 0x00435bd0 */
#ifndef LEGOLAND_PORTABLE
extern void SaveJungleCruise();                              /* 0x00435c70 */
#else
extern int SaveJungleCruise(void);                              /* 0x00435c70 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void LoadJungleCruise();                              /* 0x00435ec0 */
#else
extern int LoadJungleCruise(void);                              /* 0x00435ec0 */
#endif
extern void JungleCruise_BestValue();                              /* 0x00436160 */
extern void JungleCruiseWater_Create();                              /* 0x00436190 */
extern void JcWater_SelectForPlacement();                              /* 0x004361a0 */
extern void JcWater_CalcCursor();                              /* 0x00436200 */
extern void JcWater_DrawSelection();                              /* 0x00436470 */
extern void JcWater_Add();                              /* 0x004365f0 */
extern void JungleCruiseWater_Remove();                              /* 0x00436a40 */
extern void PottingShed_Create();                              /* 0x0043ce60 */
extern void PottingShed_Add();                              /* 0x0043ceb0 */
extern void PottingShed_Remove();                              /* 0x0043ced0 */
extern void PottingShed_Tick();                              /* 0x0043cf00 */
extern void PottingShed_Draw();                              /* 0x0043d0b0 */
#ifndef LEGOLAND_PORTABLE
extern void PottingShed_Destroy();                              /* 0x0043d1c0 */
#else
extern void PottingShed_Destroy(void);                              /* 0x0043d1c0 */
#endif
extern void PottingShed_Select();                              /* 0x0043d1d0 */
extern void PottingShed_GetDrawDesc();                              /* 0x0043d210 */
extern void MechanicsHut_Create();                              /* 0x0043d250 */
extern void MechanicsHut_Add();                              /* 0x0043d2a0 */
extern void MechanicsHut_Remove();                              /* 0x0043d2c0 */
extern void MechanicsHut_Tick();                              /* 0x0043d2f0 */
extern void MechanicsHut_Draw();                              /* 0x0043d580 */
#ifndef LEGOLAND_PORTABLE
extern void MechanicsHut_Destroy();                              /* 0x0043d730 */
#else
extern void MechanicsHut_Destroy(void);                              /* 0x0043d730 */
#endif
extern void MechanicsHut_Select();                              /* 0x0043d740 */
extern void MechanicsHut_GetDrawDesc();                              /* 0x0043d780 */

/* class-name -> callback-set table */
static const char kPathControl[]                 = "PATH CONTROL";
static const char kFountain1[]                   = "FOUNTAIN 1";
static const char kFountain2[]                   = "FOUNTAIN 2";
static const char kFountain3[]                   = "FOUNTAIN 3";
static const char kCrystalPowerStation[]         = "crystal power station";
static const char kSmallPowerStation[]           = "small power station";
static const char kDinoBig[]                     = "Dino Big";
static const char kDinoSmall[]                   = "Dino Small";
static const char kDinoMini[]                    = "Dino Mini";
static const char kDrivingSchoolPumps[]          = "DRIVING SCHOOL PUMPS";
static const char kDrivingSchool[]               = "DRIVING SCHOOL";
static const char kDrivingSchoolRoads[]          = "DRIVING SCHOOL ROADS";
static const char kZebraCrossing[]               = "ZEBRA CROSSING";
static const char kEntrance1[]                   = "ENTRANCE 1";
static const char kPottingShed[]                 = "POTTING SHED";
static const char kMechanicsHut[]                = "MECHANICS HUT";
static const char kCarousel[]                    = "CAROUSEL";
static const char kBalloonz[]                    = "BALLOONZ";
static const char kEarthSlideRide[]              = "EARTH SLIDE RIDE";
static const char kCastleBbq[]                   = "CASTLE BBQ";
static const char kFoodcartDrink[]               = "FOODCART DRINK";
static const char kFoodcartFood[]                = "FOODCART FOOD";
static const char kFoodcartIcecream[]            = "FOODCART ICECREAM";
static const char kOctopusCafe[]                 = "OCTOPUS CAFE";
static const char kRestaurant1[]                 = "RESTAURANT 1";
static const char kRestaurant2[]                 = "RESTAURANT 2";
static const char kChuckWagon[]                  = "CHUCK WAGON";
static const char kSharkCafe[]                   = "SHARK CAFE";
static const char kSharkCafeBrolly[]             = "SHARK CAFE BROLLY";
static const char kBoatingSchoolWater[]          = "BOATING SCHOOL WATER";
static const char kBoatingSchool[]               = "BOATING SCHOOL";
static const char kBoatingSchoolMermaid[]        = "BOATING SCHOOL MERMAID";
static const char kJungleCruiseWater[]           = "JUNGLE CRUISE WATER";
static const char kJungleCruise[]                = "JUNGLE CRUISE";
static const char kJungleCruiseMonkeyTree[]      = "JUNGLE CRUISE MONKEY TREE";
static const char kJungleCruiseMonkeyFish[]      = "JUNGLE CRUISE MONKEY FISH";

#ifdef LEGOLAND_PORTABLE
/* PORT-M3: one wasm type per callback slot.  These ObjDef slots are
 * called with the instance pointer the class was registered with:
 *   cb_a8 +0xa8, called as (ctx) by renderview.c:1130
 *   cb_ac +0xac, called as (elem) by sysmisc.c:664
 *   cb_load +0xb8, called as (elem) by savegame.c:1365
 *   cb_save +0xbc, called as (elem) by savegame.c:941
 * and these bodies never read it -- free on x86 cdecl, where the caller
 * pushes and the caller cleans up, but a wasm call_indirect whose type is
 * not the target's traps.  The portable build registers an adapter of the
 * slot's own type which drops the argument, so the slot holds one type.
 * The matched bodies are untouched. */
extern void Balloonz_Destroy(void);
static void ll_cb_ac_Balloonz_Destroy(void* ll_elem)
{
    (void)ll_elem;
    Balloonz_Destroy();
}
extern int Balloonz_Save(void);
static int ll_cb_save_Balloonz_Save(void* ll_elem)
{
    (void)ll_elem;
    return Balloonz_Save();
}
extern void BoatingSchool_Tick(void);
static void ll_cb_a8_BoatingSchool_Tick(void* ll_elem)
{
    (void)ll_elem;
    BoatingSchool_Tick();
}
extern void Brolly_FreeResources(void);
static void ll_cb_ac_Brolly_FreeResources(void* ll_elem)
{
    (void)ll_elem;
    Brolly_FreeResources();
}
extern int Carousel_Save(void);
static int ll_cb_save_Carousel_Save(void* ll_elem)
{
    (void)ll_elem;
    return Carousel_Save();
}
extern void CastleBbq_FreeResources(void);
static void ll_cb_ac_CastleBbq_FreeResources(void* ll_elem)
{
    (void)ll_elem;
    CastleBbq_FreeResources();
}
extern void ChuckWagon_FreeResources(void);
static void ll_cb_ac_ChuckWagon_FreeResources(void* ll_elem)
{
    (void)ll_elem;
    ChuckWagon_FreeResources();
}
extern void Dino_AC(void);
static void ll_cb_ac_Dino_AC(void* ll_elem)
{
    (void)ll_elem;
    Dino_AC();
}
extern int EarthSlide_Load(void);
static int ll_cb_load_EarthSlide_Load(void* ll_elem)
{
    (void)ll_elem;
    return EarthSlide_Load();
}
extern int EarthSlide_Save(void);
static int ll_cb_save_EarthSlide_Save(void* ll_elem)
{
    (void)ll_elem;
    return EarthSlide_Save();
}
extern void FoodcartDrink_FreeResources(void);
static void ll_cb_ac_FoodcartDrink_FreeResources(void* ll_elem)
{
    (void)ll_elem;
    FoodcartDrink_FreeResources();
}
extern void FoodcartFood_FreeResources(void);
static void ll_cb_ac_FoodcartFood_FreeResources(void* ll_elem)
{
    (void)ll_elem;
    FoodcartFood_FreeResources();
}
extern void FoodcartIcecream_FreeResources(void);
static void ll_cb_ac_FoodcartIcecream_FreeResources(void* ll_elem)
{
    (void)ll_elem;
    FoodcartIcecream_FreeResources();
}
extern void Fountain_AC(void);
static void ll_cb_ac_Fountain_AC(void* ll_elem)
{
    (void)ll_elem;
    Fountain_AC();
}
extern void JungleCruiseMonkeyFish_Destroy(void);
static void ll_cb_ac_JungleCruiseMonkeyFish_Destroy(void* ll_elem)
{
    (void)ll_elem;
    JungleCruiseMonkeyFish_Destroy();
}
extern void JungleCruiseMonkeyTree_Destroy(void);
static void ll_cb_ac_JungleCruiseMonkeyTree_Destroy(void* ll_elem)
{
    (void)ll_elem;
    JungleCruiseMonkeyTree_Destroy();
}
extern void JungleCruise_Tick(void);
static void ll_cb_a8_JungleCruise_Tick(void* ll_elem)
{
    (void)ll_elem;
    JungleCruise_Tick();
}
extern int LoadBoatingSchool(void);
static int ll_cb_load_LoadBoatingSchool(void* ll_elem)
{
    (void)ll_elem;
    return LoadBoatingSchool();
}
extern int LoadDrivingSchool(void);
static int ll_cb_load_LoadDrivingSchool(void* ll_elem)
{
    (void)ll_elem;
    return LoadDrivingSchool();
}
extern int LoadJungleCruise(void);
static int ll_cb_load_LoadJungleCruise(void* ll_elem)
{
    (void)ll_elem;
    return LoadJungleCruise();
}
extern void MechanicsHut_Destroy(void);
static void ll_cb_ac_MechanicsHut_Destroy(void* ll_elem)
{
    (void)ll_elem;
    MechanicsHut_Destroy();
}
extern void OctopusCafe_Destroy(void);
static void ll_cb_ac_OctopusCafe_Destroy(void* ll_elem)
{
    (void)ll_elem;
    OctopusCafe_Destroy();
}
extern void PottingShed_Destroy(void);
static void ll_cb_ac_PottingShed_Destroy(void* ll_elem)
{
    (void)ll_elem;
    PottingShed_Destroy();
}
extern void PowerStation_AC(void);
static void ll_cb_ac_PowerStation_AC(void* ll_elem)
{
    (void)ll_elem;
    PowerStation_AC();
}
extern void Restaurant1_FreeResources(void);
static void ll_cb_ac_Restaurant1_FreeResources(void* ll_elem)
{
    (void)ll_elem;
    Restaurant1_FreeResources();
}
extern int Restaurant1_Load(void);
static int ll_cb_load_Restaurant1_Load(void* ll_elem)
{
    (void)ll_elem;
    return Restaurant1_Load();
}
extern int Restaurant1_Save(void);
static int ll_cb_save_Restaurant1_Save(void* ll_elem)
{
    (void)ll_elem;
    return Restaurant1_Save();
}
extern void Restaurant2_Destroy(void);
static void ll_cb_ac_Restaurant2_Destroy(void* ll_elem)
{
    (void)ll_elem;
    Restaurant2_Destroy();
}
extern int Restaurant2_Load(void);
static int ll_cb_load_Restaurant2_Load(void* ll_elem)
{
    (void)ll_elem;
    return Restaurant2_Load();
}
extern int Restaurant2_Save(void);
static int ll_cb_save_Restaurant2_Save(void* ll_elem)
{
    (void)ll_elem;
    return Restaurant2_Save();
}
extern void Roads_FreeResources(void);
static void ll_cb_ac_Roads_FreeResources(void* ll_elem)
{
    (void)ll_elem;
    Roads_FreeResources();
}
extern int SaveBoatingSchool(void);
static int ll_cb_save_SaveBoatingSchool(void* ll_elem)
{
    (void)ll_elem;
    return SaveBoatingSchool();
}
extern int SaveDrivingSchool(void);
static int ll_cb_save_SaveDrivingSchool(void* ll_elem)
{
    (void)ll_elem;
    return SaveDrivingSchool();
}
extern int SaveJungleCruise(void);
static int ll_cb_save_SaveJungleCruise(void* ll_elem)
{
    (void)ll_elem;
    return SaveJungleCruise();
}
extern void SharkCafe_FreeResources(void);
static void ll_cb_ac_SharkCafe_FreeResources(void* ll_elem)
{
    (void)ll_elem;
    SharkCafe_FreeResources();
}

/* PORT-M5: the ONE +0xa0 slot PORT-M3 left open, and it is not a recovery
 * error -- it is an ORIGINAL DEFECT that the shipped build never reaches.
 *
 *   0x00452c51  mov dword ptr [esi+0xa0], 0x45dcf0
 *
 * is what the original does, so the callee is right (this is NOT the wrong-
 * callee class PORT-M2 found at 1a).  But +0xa0's contract is fixed by its
 * only consumer, renderview.c:1221 --
 *
 *   if (def->flags & 0x400) { ... desc = def->draw(def->ctx, base); ... }
 *   SpriteDesc* (*draw)(void* ctx, BPos base);     renderview.c:304
 *
 * -- and the nine other bodies registered in it are all
 * `<desc>* <Class>_GetDrawDesc(elem, base)`.  pathtile2.c's DrawBasicPath is
 * `void DrawBasicPath(int tile, int x, int y, int mode)`: a four-argument
 * PAINTER in a two-argument GETTER slot, and it is referenced nowhere else in
 * the image, so there is no second caller that would fit it.
 *
 * Why the shipped game survives it: +0xa0 is read only behind `flags & 0x400`,
 * and the PATH CONTROL class never raises that bit.  Bit 10 is set by a
 * class's own +0xa4 create handler (`def->flags |= 0x420`, e.g. mechrides.c,
 * catapult.c, joust.c, ridecb8.c) and PATH CONTROL registers no create
 * handler at all; and it is not in the data either -- over all 155 `.odf`
 * members of the three shipped RES volumes the class-flags dword at ODF+0x1c
 * uses only bits 0-2 and 16-25, so bit 10 is never loaded from disk.  Were it
 * ever reached, the original would call DrawBasicPath(ctx, base, <garbage>,
 * <garbage>) and index g_tile_sprites with a pointer.
 *
 * The adapter therefore has the slot's type (so the slot holds ONE wasm type
 * and nothing else in the table has to move) and traps if the impossible
 * happens, rather than forwarding two invented arguments into a wild read. */
static void* ll_cb_a0_DrawBasicPath(void* ll_ctx, unsigned short ll_base)
{
    (void)ll_ctx;
    (void)ll_base;
    LL_DEBUGBREAK();   /* unreachable: PATH CONTROL never sets flags & 0x400 */
    return 0;
}
#endif
// FUNCTION: LEGOLAND 0x00452c20
void SetCustomCallbacks(RideElem* elem)
{
    RideDef* def = elem->data;

    if (NameCompare(elem->name, kPathControl) == 0) {
        def->cb_add = AddBasicPath;
        def->cb_remove = RemoveBasicPath;
#ifndef LEGOLAND_PORTABLE
        def->cb_a0 = DrawBasicPath;
#else
        def->cb_a0 = ll_cb_a0_DrawBasicPath;   /* PORT-M5 */
#endif
    } else if (NameCompare(elem->name, kFountain1) == 0
            || NameCompare(elem->name, kFountain2) == 0
            || NameCompare(elem->name, kFountain3) == 0) {
        def->cb_add = Fountain_Add;
        def->cb_remove = RemoveSoundObject;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = Fountain_AC;
#else
        def->cb_ac = ll_cb_ac_Fountain_AC;   /* PORT-M3 */
#endif
        Fountain_InitSound(elem);
    } else if (NameCompare(elem->name, kCrystalPowerStation) == 0) {
        def->cb_add = CrystalPowerStation_Add;
        def->cb_remove = RemoveSoundObject;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = PowerStation_AC;
#else
        def->cb_ac = ll_cb_ac_PowerStation_AC;   /* PORT-M3 */
#endif
        PowerStation_InitSound(elem);
    } else if (NameCompare(elem->name, kSmallPowerStation) == 0) {
        def->cb_add = SmallPowerStation_Add;
        def->cb_remove = RemoveSoundObject;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = PowerStation_AC;
#else
        def->cb_ac = ll_cb_ac_PowerStation_AC;   /* PORT-M3 */
#endif
        PowerStation_InitSound(elem);
    } else if (NameCompare(elem->name, kDinoBig) == 0
            || NameCompare(elem->name, kDinoSmall) == 0
            || NameCompare(elem->name, kDinoMini) == 0) {
        def->cb_add = Dino_Add;
        def->cb_remove = RemoveSoundObject;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = Dino_AC;
#else
        def->cb_ac = ll_cb_ac_Dino_AC;   /* PORT-M3 */
#endif
        Dino_InitSound(elem);
    } else if (NameCompare(kDrivingSchoolPumps, elem->name) == 0) {
        def->cb_a4 = Pump_LoadResources;
        def->cb_8c = Pump_SelectForPlacement;
        def->cb_90 = Pump_Update;
        def->cb_add = Pump_Add;
        def->cb_remove = Pumps_Remove;
    } else if (NameCompare(kDrivingSchool, elem->name) == 0) {
        def->cb_a4 = DrivingSchool_Create;
        def->cb_8c = DrivingSchool_SelectForPlacement;
        def->cb_90 = DrivingSchool_Update;
        def->cb_94 = DrivingSchool_DrawSelection;
        def->cb_add = DrivingSchool_Add;
        def->cb_remove = DrivingSchool_Remove;
        def->cb_a8 = DrivingSchool_TickRiders;
        def->cb_a0 = DrivingSchool_GetDrawDesc;
        def->cb_b0 = DrivingSchool_Draw;
#ifndef LEGOLAND_PORTABLE
        def->cb_save = SaveDrivingSchool;
#else
        def->cb_save = ll_cb_save_SaveDrivingSchool;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_load = LoadDrivingSchool;
#else
        def->cb_load = ll_cb_load_LoadDrivingSchool;   /* PORT-M3 */
#endif
        def->cb_ac = DrivingSchool_Destroy;
        def->cb_c0 = DrivingSchool_BestTake;
    } else if (NameCompare(kDrivingSchoolRoads, elem->name) == 0) {
        def->cb_a4 = Roads_LoadResources;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = Roads_FreeResources;
#else
        def->cb_ac = ll_cb_ac_Roads_FreeResources;   /* PORT-M3 */
#endif
        def->cb_8c = Roads_SelectForPlacement;
        def->cb_90 = Roads_CalcCursor;
        def->cb_94 = Road_DrawSelection;
        def->cb_add = Roads_Add;
        def->cb_remove = Roads_Remove;
    } else if (NameCompare(kZebraCrossing, elem->name) == 0) {
        def->cb_a4 = ZebraCrossing_LoadResources;
        def->cb_8c = ZebraCrossing_SelectForPlacement;
        def->cb_90 = ZebraCrossing_Update;
        def->cb_94 = Road_DrawSelection;
        def->cb_add = ZebraCrossing_Add;
        def->cb_remove = Roads_Remove;
    } else if (NameCompare(kEntrance1, elem->name) == 0) {
        def->cb_a4 = Entrance1_Create;
        def->cb_ac = Entrance1_Destroy;
        def->cb_a8 = Entrance1_Tick;
        def->cb_b0 = Entrance1_Draw;
        def->cb_remove = Entrance1_Remove;
    } else if (NameCompare(kPottingShed, elem->name) == 0) {
        def->cb_a4 = PottingShed_Create;
        def->cb_8c = PottingShed_Select;
        def->cb_add = PottingShed_Add;
        def->cb_remove = PottingShed_Remove;
        def->cb_a8 = PottingShed_Tick;
        def->cb_b0 = PottingShed_Draw;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = PottingShed_Destroy;
#else
        def->cb_ac = ll_cb_ac_PottingShed_Destroy;   /* PORT-M3 */
#endif
        def->cb_a0 = PottingShed_GetDrawDesc;
    } else if (NameCompare(kMechanicsHut, elem->name) == 0) {
        def->cb_a4 = MechanicsHut_Create;
        def->cb_8c = MechanicsHut_Select;
        def->cb_add = MechanicsHut_Add;
        def->cb_remove = MechanicsHut_Remove;
        def->cb_a8 = MechanicsHut_Tick;
        def->cb_b0 = MechanicsHut_Draw;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = MechanicsHut_Destroy;
#else
        def->cb_ac = ll_cb_ac_MechanicsHut_Destroy;   /* PORT-M3 */
#endif
        def->cb_a0 = MechanicsHut_GetDrawDesc;
    } else if (NameCompare(kCarousel, elem->name) == 0) {
        def->cb_a4 = Carousel_Create;
        def->cb_ac = Carousel_Destroy;
        def->cb_8c = Carousel_SelectForPlacement;
        def->cb_a8 = Carousel_Tick;
        def->cb_b0 = Carousel_Draw;
        def->cb_remove = Carousel_Remove;
        def->cb_add = Carousel_Add;
        def->cb_a0 = Carousel_GetDrawDesc;
        def->cb_load = Carousel_Load;
#ifndef LEGOLAND_PORTABLE
        def->cb_save = Carousel_Save;
#else
        def->cb_save = ll_cb_save_Carousel_Save;   /* PORT-M3 */
#endif
    } else if (NameCompare(kBalloonz, elem->name) == 0) {
        def->cb_a4 = Balloonz_Create;
        def->cb_8c = Balloonz_SelectForPlacement;
        def->cb_add = Balloonz_Add;
        def->cb_remove = Balloonz_Remove;
        def->cb_a8 = Balloonz_Tick;
        def->cb_b0 = Balloonz_Draw;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = Balloonz_Destroy;
#else
        def->cb_ac = ll_cb_ac_Balloonz_Destroy;   /* PORT-M3 */
#endif
        def->cb_a0 = Balloonz_GetDrawDesc;
#ifndef LEGOLAND_PORTABLE
        def->cb_save = Balloonz_Save;
#else
        def->cb_save = ll_cb_save_Balloonz_Save;   /* PORT-M3 */
#endif
        def->cb_load = Balloonz_Load;
    } else if (NameCompare(kEarthSlideRide, elem->name) == 0) {
        def->cb_a4 = EarthSlide_Create;
        def->cb_ac = EarthSlide_FreeResources;
        def->cb_8c = EarthSlide_SelectForPlacement;
        def->cb_a8 = EarthSlide_Tick;
        def->cb_b0 = EarthSlide_Draw;
        def->cb_remove = EarthSlide_Remove;
        def->cb_add = EarthSlide_Add;
#ifndef LEGOLAND_PORTABLE
        def->cb_save = EarthSlide_Save;
#else
        def->cb_save = ll_cb_save_EarthSlide_Save;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_load = EarthSlide_Load;
#else
        def->cb_load = ll_cb_load_EarthSlide_Load;   /* PORT-M3 */
#endif
    } else if (NameCompare(kCastleBbq, elem->name) == 0) {
        def->cb_a4 = CastleBbq_LoadResources;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = CastleBbq_FreeResources;
#else
        def->cb_ac = ll_cb_ac_CastleBbq_FreeResources;   /* PORT-M3 */
#endif
        def->cb_8c = Food_SelectForPlacement;
        def->cb_a8 = CastleBbq_Tick;
        def->cb_b0 = CastleBbq_Draw;
        def->cb_add = CastleBbq_Add;
        def->cb_remove = CastleBbq_Remove;
    } else if (NameCompare(kFoodcartDrink, elem->name) == 0) {
        def->cb_a4 = FoodcartDrink_LoadResources;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = FoodcartDrink_FreeResources;
#else
        def->cb_ac = ll_cb_ac_FoodcartDrink_FreeResources;   /* PORT-M3 */
#endif
        def->cb_8c = Food_SelectForPlacement;
        def->cb_a8 = FoodcartDrink_Tick;
        def->cb_remove = FoodService_Remove;
        def->cb_b0 = Foodcart_DrawOverlay;
    } else if (NameCompare(kFoodcartFood, elem->name) == 0) {
        def->cb_a4 = FoodcartFood_LoadResources;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = FoodcartFood_FreeResources;
#else
        def->cb_ac = ll_cb_ac_FoodcartFood_FreeResources;   /* PORT-M3 */
#endif
        def->cb_8c = Food_SelectForPlacement;
        def->cb_a8 = FoodcartFood_Tick;
        def->cb_remove = FoodService_Remove;
        def->cb_b0 = Foodcart_DrawOverlay;
    } else if (NameCompare(kFoodcartIcecream, elem->name) == 0) {
        def->cb_a4 = FoodcartIcecream_LoadResources;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = FoodcartIcecream_FreeResources;
#else
        def->cb_ac = ll_cb_ac_FoodcartIcecream_FreeResources;   /* PORT-M3 */
#endif
        def->cb_8c = Food_SelectForPlacement;
        def->cb_a8 = FoodcartIcecream_Tick;
        def->cb_remove = FoodService_Remove;
        def->cb_b0 = Foodcart_DrawOverlay;
    } else if (NameCompare(kOctopusCafe, elem->name) == 0) {
        def->cb_a4 = OctopusCafe_Create;
        def->cb_add = OctopusCafe_Add;
        def->cb_8c = Food_SelectForPlacement;
        def->cb_a8 = OctopusCafe_Tick;
        def->cb_remove = FoodService_Remove;
        def->cb_b0 = OctopusCafe_Draw;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = OctopusCafe_Destroy;
#else
        def->cb_ac = ll_cb_ac_OctopusCafe_Destroy;   /* PORT-M3 */
#endif
    } else if (NameCompare(kRestaurant1, elem->name) == 0) {
        def->cb_a4 = Restaurant1_Create;
        def->cb_8c = Food_SelectForPlacement;
        def->cb_a8 = Restaurant1_Tick;
        def->cb_add = Restaurant1_Add;
        def->cb_remove = Restaurant1_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = Restaurant1_FreeResources;
#else
        def->cb_ac = ll_cb_ac_Restaurant1_FreeResources;   /* PORT-M3 */
#endif
        def->cb_b0 = Restaurant1_Draw;
#ifndef LEGOLAND_PORTABLE
        def->cb_save = Restaurant1_Save;
#else
        def->cb_save = ll_cb_save_Restaurant1_Save;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_load = Restaurant1_Load;
#else
        def->cb_load = ll_cb_load_Restaurant1_Load;   /* PORT-M3 */
#endif
    } else if (NameCompare(kRestaurant2, elem->name) == 0) {
        def->cb_a4 = Restaurant2_Create;
        def->cb_8c = Food_SelectForPlacement;
        def->cb_a8 = Restaurant2_Tick;
        def->cb_add = Restaurant2_Add;
        def->cb_remove = Restaurant2_Remove;
        def->cb_a0 = Restaurant2_GetDrawDesc;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = Restaurant2_Destroy;
#else
        def->cb_ac = ll_cb_ac_Restaurant2_Destroy;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_save = Restaurant2_Save;
#else
        def->cb_save = ll_cb_save_Restaurant2_Save;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_load = Restaurant2_Load;
#else
        def->cb_load = ll_cb_load_Restaurant2_Load;   /* PORT-M3 */
#endif
        def->cb_b0 = Restaurant2_Draw;
    } else if (NameCompare(kChuckWagon, elem->name) == 0) {
        def->cb_a4 = ChuckWagon_LoadResources;
        def->cb_8c = Food_SelectForPlacement;
        def->cb_remove = FoodService_Remove;
        def->cb_a8 = ChuckWagon_TickCustomers;
        def->cb_b0 = ChuckWagon_DrawOverlay;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = ChuckWagon_FreeResources;
#else
        def->cb_ac = ll_cb_ac_ChuckWagon_FreeResources;   /* PORT-M3 */
#endif
    } else if (NameCompare(kSharkCafe, elem->name) == 0) {
        def->cb_a4 = SharkCafe_LoadResources;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = SharkCafe_FreeResources;
#else
        def->cb_ac = ll_cb_ac_SharkCafe_FreeResources;   /* PORT-M3 */
#endif
        def->cb_8c = Food_SelectForPlacement;
        def->cb_remove = FoodService_Remove;
        def->cb_a8 = SharkCafe_Tick;
        def->cb_b0 = Foodcart_DrawOverlay;
    } else if (NameCompare(kSharkCafeBrolly, elem->name) == 0) {
        def->cb_a4 = Brolly_LoadResources;
        def->cb_8c = Brolly_SelectForPlacement;
        def->cb_add = Brolly_Add;
        def->cb_a0 = Brolly_GetDrawDesc;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = Brolly_FreeResources;
#else
        def->cb_ac = ll_cb_ac_Brolly_FreeResources;   /* PORT-M3 */
#endif
    } else if (NameCompare(kBoatingSchoolWater, elem->name) == 0) {
        def->cb_a4 = BsWater_LoadResources;
        def->cb_8c = BsWater_SelectForPlacement;
        def->cb_90 = BsWater_CalcCursor;
        def->cb_94 = BsWater_DrawSelection;
        def->cb_add = BsWater_Add;
        def->cb_remove = BoatingSchoolWater_Remove;
    } else if (NameCompare(kBoatingSchool, elem->name) == 0) {
        def->cb_a4 = BoatingSchool_Create;
        def->cb_ac = BoatingSchool_Destroy;
        def->cb_8c = BoatingSchool_SelectForPlacement;
        def->cb_90 = BoatingSchool_Update;
        def->cb_94 = BoatingSchool_DrawSelection;
        def->cb_add = BoatingSchool_Add;
        def->cb_remove = BoatingSchool_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_a8 = BoatingSchool_Tick;
#else
        def->cb_a8 = ll_cb_a8_BoatingSchool_Tick;   /* PORT-M3 */
#endif
        def->cb_b0 = BoatingSchool_Draw;
#ifndef LEGOLAND_PORTABLE
        def->cb_save = SaveBoatingSchool;
#else
        def->cb_save = ll_cb_save_SaveBoatingSchool;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_load = LoadBoatingSchool;
#else
        def->cb_load = ll_cb_load_LoadBoatingSchool;   /* PORT-M3 */
#endif
        def->cb_c0 = BoatingSchool_BestTake;
    } else if (NameCompare(kBoatingSchoolMermaid, elem->name) == 0) {
        def->cb_a4 = Mermaid_LoadResources;
        def->cb_8c = Mermaid_SelectForPlacement;
        def->cb_90 = Mermaid_CalcCursor;
        def->cb_94 = Mermaid_CalcCursor2;
        def->cb_add = Mermaid_Add;
        def->cb_remove = BsMermaid_Remove;
    } else if (NameCompare(kJungleCruiseWater, elem->name) == 0) {
        def->cb_a4 = JungleCruiseWater_Create;
        def->cb_8c = JcWater_SelectForPlacement;
        def->cb_90 = JcWater_CalcCursor;
        def->cb_94 = JcWater_DrawSelection;
        def->cb_add = JcWater_Add;
        def->cb_remove = JungleCruiseWater_Remove;
    } else if (NameCompare(kJungleCruise, elem->name) == 0) {
        def->cb_a4 = JungleCruise_Create;
        def->cb_ac = JungleCruise_Destroy;
        def->cb_8c = JungleCruise_SelectForPlacement;
        def->cb_90 = JungleCruise_Update;
        def->cb_94 = JungleCruise_DrawSelection;
        def->cb_add = JungleCruise_Add;
        def->cb_remove = JungleCruise_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_a8 = JungleCruise_Tick;
#else
        def->cb_a8 = ll_cb_a8_JungleCruise_Tick;   /* PORT-M3 */
#endif
        def->cb_b0 = JungleCruise_Draw;
#ifndef LEGOLAND_PORTABLE
        def->cb_save = SaveJungleCruise;
#else
        def->cb_save = ll_cb_save_SaveJungleCruise;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_load = LoadJungleCruise;
#else
        def->cb_load = ll_cb_load_LoadJungleCruise;   /* PORT-M3 */
#endif
        def->cb_c0 = JungleCruise_BestValue;
    } else if (NameCompare(kJungleCruiseMonkeyTree, elem->name) == 0) {
        def->cb_a4 = JcMonkeyTree_Create;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = JungleCruiseMonkeyTree_Destroy;
#else
        def->cb_ac = ll_cb_ac_JungleCruiseMonkeyTree_Destroy;   /* PORT-M3 */
#endif
        def->cb_8c = JcMonkeyTree_SelectForPlacement;
        def->cb_90 = MonkeyTree_CalcCursor;
        def->cb_94 = JungleCruiseMonkeyTree_DrawSelection;
        def->cb_add = MonkeyTree_Add;
        def->cb_remove = MonkeyTree_Remove;
        def->cb_a0 = JcMonkeyTree_GetDrawDesc;
    } else if (NameCompare(kJungleCruiseMonkeyFish, elem->name) == 0) {
        def->cb_a4 = JcMonkeyFish_Create;
#ifndef LEGOLAND_PORTABLE
        def->cb_ac = JungleCruiseMonkeyFish_Destroy;
#else
        def->cb_ac = ll_cb_ac_JungleCruiseMonkeyFish_Destroy;   /* PORT-M3 */
#endif
        def->cb_8c = JcMonkeyFish_SelectForPlacement;
        def->cb_90 = MonkeyFish_CalcCursor;
        def->cb_94 = JungleCruiseMonkeyFish_DrawSelection;
        def->cb_add = MonkeyFish_Add;
        def->cb_remove = MonkeyFish_Remove;
        def->cb_a0 = JcMonkeyFish_GetDrawDesc;
    }

    CastleLevel1_GetInterfaces(elem, def);
    LogFlume_GetInterfaces(elem, def);
    Copters_GetInterfaces(elem, def);
    Fort_GetInterfaces(elem, def);
    GoldRush_GetInterfaces(elem, def);
    Temple_GetInterfaces(elem, def);
    Catapult_GetInterfaces(elem, def);
    Joust_GetInterfaces(elem, def);
    TempleSlide_GetInterfaces(elem, def);
    SpiderRide_GetInterfaces(elem, def);
    SafariRide_GetInterfaces(elem, def);
    WaterWorks_GetInterfaces(elem, def);
    Garden_GetInterfaces(elem, def);
    WesternTown_GetInterfaces(elem, def);
    SpaceTower_GetInterfaces(elem, def);
    SpinningBarrels_GetInterfaces(elem, def);
    PlaneRide_GetInterfaces(elem, def);
    CastleObj_GetInterfaces(elem, def);
}


/* =========================================================================
 * 0x00463870 -- InitScreen: register the window class, build the four UI
 * fonts, create the game window and bring up the DirectDraw surfaces.
 *
 * Runs after InitHostSystemGPU (gpu.c) has produced g_ddraw (IDirectDraw2 @
 * 0x00667d74).  g_windowed (0x00667d6c) picks the branch:
 *
 *  FULLSCREEN (g_windowed == 0)
 *    g_screen_depth = 2 (RGB565)
 *    CreateWindowEx(WS_EX_TOPMOST, "LEGOLANDMAIN", "LEGOLAND",
 *                   WS_POPUP|WS_VISIBLE, 0,0, screen w x h, GetDesktopWindow())
 *    SetCooperativeLevel(EXCLUSIVE|FULLSCREEN), SetScreenDisplayMode()
 *    g_present = FlipPrimary (0x004661d0)
 *    primary  = CreateSurface(DDSCAPS_PRIMARYSURFACE|DDSCAPS_VIDEOMEMORY)
 *    LoadColourTable()
 *    back     = CreateSurface(0x800  = DDSCAPS_OFFSCREENPLAIN? see below)
 *               -> g_draw_surface (0x0066807c) is the SAME surface
 *    third    = CreateSurface(0x4000 = DDSCAPS_VIDEOMEMORY)   -> 0x00668074
 *    CreateClipper -> 0x00668080, SetClipping(0,0,w,h), then a one-rectangle
 *    RGNDATA (g_clip_rect twice: bound + list) handed to SetClipList.
 *
 *  WINDOWED (g_windowed != 0)
 *    AdjustWindowRect({0,0,w-1,h-1}, WS_OVERLAPPEDWINDOW|WS_VISIBLE)
 *    CreateWindowEx(0, ..., that style, right-left+1 x bottom-top+1)
 *    SetCooperativeLevel(DDSCL_NORMAL), SetScreenDisplayMode()
 *    CreateClipper, clipper->SetHWnd(hwnd), primary = CreateSurface(
 *    DDSCAPS_PRIMARYSURFACE), primary->SetClipper(clipper), then two
 *    system-memory surfaces (DDSCAPS_SYSTEMMEMORY 0x40) of screen size.
 *
 * Every failure path returns 0; the windowed one DestroyWindow()s first.
 * The `1` return is shared by both branches (0x00463ca3).
 *
 * The four fonts are one LOGFONT filled once and re-poked between calls:
 *   0x00668090  h 24 w 0 weight 700   (bold heading)
 *   0x00668098  h 28 w 0 weight 400
 *   0x0066808c  h 20 w 0 weight 700
 *   0x00668094  h 18 w 0 weight 600
 * all DEFAULT_CHARSET(1) / PROOF_QUALITY(2) in the 5-byte face name "Lego"
 * at 0x004b86e0 (the .ttf AddFontResource'd by InitHostSystemGPU).
 * ========================================================================= */

typedef struct WinRect {
    long left;                 /* +0x00 */
    long top;                  /* +0x04 */
    long right;                /* +0x08 */
    long bottom;               /* +0x0c */
} WinRect;

/* WNDCLASSEXA, 0x30 bytes. */
typedef struct WndClassEx {
    unsigned int cbSize;           /* +0x00 */
    unsigned int style;            /* +0x04 */
    void*        lpfnWndProc;      /* +0x08 */
    int          cbClsExtra;       /* +0x0c */
    int          cbWndExtra;       /* +0x10 */
    void*        hInstance;        /* +0x14 */
    void*        hIcon;            /* +0x18 */
    void*        hCursor;          /* +0x1c */
    void*        hbrBackground;    /* +0x20 */
    const char*  lpszMenuName;     /* +0x24 */
    const char*  lpszClassName;    /* +0x28 */
    void*        hIconSm;          /* +0x2c */
} WndClassEx;

/* LOGFONTA, 0x3c bytes. */
typedef struct LogFont {
    long          lfHeight;         /* +0x00 */
    long          lfWidth;          /* +0x04 */
    long          lfEscapement;     /* +0x08 */
    long          lfOrientation;    /* +0x0c */
    long          lfWeight;         /* +0x10 */
    unsigned char lfItalic;         /* +0x14 */
    unsigned char lfUnderline;      /* +0x15 */
    unsigned char lfStrikeOut;      /* +0x16 */
    unsigned char lfCharSet;        /* +0x17 */
    unsigned char lfOutPrecision;   /* +0x18 */
    unsigned char lfClipPrecision;  /* +0x19 */
    unsigned char lfQuality;        /* +0x1a */
    unsigned char lfPitchAndFamily; /* +0x1b */
    char          lfFaceName[32];   /* +0x1c */
} LogFont;

/* The 5-byte face name copied into lfFaceName as one object (dword + byte). */
typedef struct FontFace { char c[5]; } FontFace;

/* DDSURFACEDESC, 0x6c bytes -- only dwSize/dwFlags/dwHeight/dwWidth and
 * ddsCaps.dwCaps (+0x68) are written here (same view as gpu.c). */
typedef struct DDSurfaceDesc {
    unsigned long dwSize;              /* +0x00 */
    unsigned long dwFlags;             /* +0x04 */
    unsigned long dwHeight;            /* +0x08 */
    unsigned long dwWidth;             /* +0x0c */
    char          pad10[0x68 - 0x10];  /* +0x10 */
    unsigned long dwCaps;              /* +0x68 ddsCaps.dwCaps */
} DDSurfaceDesc;

/* RGNDATA with one rectangle. */
typedef struct RgnData {
    unsigned long dwSize;      /* +0x00 sizeof(RGNDATAHEADER) */
    unsigned long iType;       /* +0x04 RDH_RECTANGLES */
    unsigned long nCount;      /* +0x08 */
    unsigned long nRgnSize;    /* +0x0c */
    WinRect       rcBound;     /* +0x10 */
    WinRect       rect;        /* +0x20 */
} RgnData;

typedef struct DDSurface DDSurface;
typedef struct DDClipper DDClipper;
typedef struct DDraw2    DDraw2;

/* IDirectDrawSurface: Release 0x08, SetClipper 0x70. */
typedef struct DDSurfaceVtbl {
    char pad00[0x08];
    long(__stdcall* Release)(DDSurface*);                        /* +0x08 */
    char pad0c[0x70 - 0x0c];
    long(__stdcall* SetClipper)(DDSurface*, DDClipper*);         /* +0x70 */
} DDSurfaceVtbl;
struct DDSurface { DDSurfaceVtbl* vtbl; };

/* IDirectDrawClipper: SetHWnd 0x20, SetClipList 0x1c. */
typedef struct DDClipperVtbl {
    char pad00[0x1c];
    long(__stdcall* SetClipList)(DDClipper*, RgnData*, unsigned long); /* +0x1c */
    long(__stdcall* SetHWnd)(DDClipper*, unsigned long, void*);        /* +0x20 */
} DDClipperVtbl;
struct DDClipper { DDClipperVtbl* vtbl; };

/* IDirectDraw2: CreateClipper 0x10, CreateSurface 0x18,
 * SetCooperativeLevel 0x50. */
typedef struct DDraw2Vtbl {
    char pad00[0x10];
    long(__stdcall* CreateClipper)(DDraw2*, unsigned long, DDClipper**, void*); /* +0x10 */
    char pad14[0x18 - 0x14];
    long(__stdcall* CreateSurface)(DDraw2*, DDSurfaceDesc*, DDSurface**, void*); /* +0x18 */
    char pad1c[0x50 - 0x1c];
    long(__stdcall* SetCooperativeLevel)(DDraw2*, void*, unsigned long);        /* +0x50 */
} DDraw2Vtbl;
struct DDraw2 { DDraw2Vtbl* vtbl; };

/* The 0x004bcbf4 record through its screen-extent + flags view. */
typedef struct ScreenCfg {
    unsigned short w;          /* +0x00 */
    unsigned short h;          /* +0x02 */
    char           pad04[0x1c - 0x04];
    unsigned char  busy;       /* +0x1c bit0: keep pumping ShowWindow */
    char           pad1d;      /* +0x1d */
    unsigned short cursor;     /* +0x1e 0 = hide the system cursor */
} ScreenCfg;

extern ScreenCfg* g_screencfg;                     /* 0x004bcbf4 */
extern int        g_windowed;                      /* 0x00667d6c */
extern DDraw2*    g_ddraw;                         /* 0x00667d74 */
extern DDSurface* g_primary;                       /* 0x00668070 */
extern DDSurface* g_surface_74;                    /* 0x00668074 */
extern DDSurface* g_surface_78;                    /* 0x00668078 */
extern DDSurface* g_draw_surface;                  /* 0x0066807c */
extern DDClipper* g_clipper;                       /* 0x00668080 */
extern void*      g_font_20;                       /* 0x0066808c */
extern void*      g_font_24;                       /* 0x00668090 */
extern void*      g_font_18;                       /* 0x00668094 */
extern void*      g_font_28;                       /* 0x00668098 */
extern int        g_init_flag;                     /* 0x007cacd4 */
typedef int     (*PresentFn)(void);
extern PresentFn  g_present;                       /* 0x004b9ca4 */
extern WinRect    g_clip_rect;                     /* 0x004bdea0 */
extern FontFace   g_font_face;                     /* 0x004b86e0 "Lego" */

extern const char kWndClass[];                     /* 0x004b9cfc "LEGOLANDMAIN" */
extern const char kWndTitleFull[];                 /* 0x004b86d0 "LEGOLAND" */
extern const char kWndTitleWin[];                  /* 0x004b9cf0 "Lego Land" */

__declspec(dllimport) void* __stdcall LoadIconA(void* inst, const char* name);   /* [0x4ab2e4] */
__declspec(dllimport) void* __stdcall LoadCursorA(void* inst, const char* name); /* [0x4ab2e0] */
__declspec(dllimport) void* __stdcall GetStockObject(int i);                     /* [0x4ab068] */
__declspec(dllimport) unsigned short __stdcall RegisterClassExA(const WndClassEx* wc); /* [0x4ab2d0] */
__declspec(dllimport) void* __stdcall CreateFontIndirectA(const LogFont* lf);    /* [0x4ab078] */
__declspec(dllimport) void* __stdcall GetDesktopWindow(void);                    /* [0x4ab2d8] */
__declspec(dllimport) void* __stdcall CreateWindowExA(unsigned long ex, const char* cls,
        const char* name, unsigned long style, int x, int y, int w, int h,
        void* parent, void* menu, void* inst, void* param);                      /* [0x4ab2d4] */
__declspec(dllimport) int __stdcall ShowWindow(void* hwnd, int cmd);             /* [0x4ab2c4] */
__declspec(dllimport) int __stdcall ShowCursor(int show);                        /* [0x4ab2cc] */
__declspec(dllimport) int __stdcall DestroyWindow(void* hwnd);                   /* [0x4ab2b8] */
__declspec(dllimport) int __stdcall AdjustWindowRect(WinRect* rc, unsigned long style,
                                                     int menu);                  /* [0x4ab2c8] */

extern void* WNDENV_GethInstance(void);            /* 0x0047fe40 */
extern void  WNDENV_Sethwnd(void* hwnd);           /* 0x0047fe50 */
extern void* WNDENV_Gethwnd(void);                 /* 0x0047fe60 */
/* input2.c's LegoLandWindowProc -- the registered window procedure, exported
 * as `_LegoLandWindowProc@16`. Declared on one line so linkreport.py's
 * line-oriented extern scanner sees the address comment. */
extern long  __stdcall LegoLandWindowProc(void* h, unsigned int m, unsigned int w, long l); /* 0x0047fe90 */
extern int   ProcessSystemEvents(void);            /* 0x00480050 */
extern void  SetClipping(WinRect* r);              /* 0x0048a5c0 */
extern void  LoadColourTable(void);                /* 0x0044e580 */
extern int   SetScreenDisplayMode(void);           /* 0x00463ef0 (internal) */
extern int   FlipPrimary(void);                    /* 0x004661d0 */

// FUNCTION: LEGOLAND 0x00463870
int InitScreen(void)
{
    WinRect       adj;
    LogFont       lf;
    WinRect       rc;
    DDSurfaceDesc ddsd;
    WndClassEx    wc;

    rc.left = 0;
    rc.top = 0;
    rc.right = g_screencfg->w;
    rc.bottom = g_screencfg->h;
    g_init_flag = 0;

    wc.cbSize = sizeof(WndClassEx);
    wc.style = 0;
    wc.lpfnWndProc = LegoLandWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = WNDENV_GethInstance();
    wc.hIcon = LoadIconA(WNDENV_GethInstance(), (const char*)0x65);
    wc.hCursor = LoadCursorA(WNDENV_GethInstance(), (const char*)0x7d);
    wc.hbrBackground = GetStockObject(5);
    wc.lpszMenuName = 0;
    wc.lpszClassName = kWndClass;
    wc.hIconSm = LoadIconA(WNDENV_GethInstance(), (const char*)0x65);
    RegisterClassExA(&wc);

    lf.lfHeight = 24;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = 700;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = 1;
    lf.lfOutPrecision = 0;
    lf.lfClipPrecision = 0;
    lf.lfQuality = 2;
    lf.lfPitchAndFamily = 0;
    *(FontFace*)lf.lfFaceName = g_font_face;
    g_font_24 = CreateFontIndirectA(&lf);

    lf.lfWeight = 400;
    lf.lfHeight = 28;
    lf.lfWidth = 0;
    g_font_28 = CreateFontIndirectA(&lf);

    lf.lfHeight = 20;
    lf.lfWidth = 0;
    lf.lfWeight = 700;
    g_font_20 = CreateFontIndirectA(&lf);

    lf.lfWeight = 600;
    lf.lfHeight = 18;
    lf.lfWidth = 0;
    *(FontFace*)lf.lfFaceName = g_font_face;
    g_font_18 = CreateFontIndirectA(&lf);

    if (g_windowed == 0) {
        g_screen_depth = 2;
        WNDENV_Sethwnd(CreateWindowExA(8, kWndClass, kWndTitleFull, 0x90000000,
                                       0, 0, g_screencfg->w, g_screencfg->h,
                                       GetDesktopWindow(), 0,
                                       WNDENV_GethInstance(), 0));
        if (!WNDENV_Gethwnd())
            return 0;
        if (g_ddraw->vtbl->SetCooperativeLevel(g_ddraw, WNDENV_Gethwnd(), 0x11))
            return 0;
        if (!SetScreenDisplayMode())
            return 0;
        while (g_screencfg->busy & 1) {
            ProcessSystemEvents();
            ShowWindow(WNDENV_Gethwnd(), 3);
        }
        if (g_screencfg->cursor == 0)
            ShowCursor(0);

        g_present = FlipPrimary;
        ddsd.dwSize = sizeof(DDSurfaceDesc);
        ddsd.dwFlags = 1;
        ddsd.dwCaps = 0x4200;
        if (g_ddraw->vtbl->CreateSurface(g_ddraw, &ddsd, &g_primary, 0))
            return 0;

        LoadColourTable();

        ddsd.dwSize = sizeof(DDSurfaceDesc);
        ddsd.dwFlags = 7;
        ddsd.dwCaps = 0x800;
        ddsd.dwWidth = g_screencfg->w;
        ddsd.dwHeight = g_screencfg->h;
        if (g_ddraw->vtbl->CreateSurface(g_ddraw, &ddsd, &g_surface_78, 0))
            return 0;
        g_draw_surface = g_surface_78;

        ddsd.dwSize = sizeof(DDSurfaceDesc);
        ddsd.dwFlags = 7;
        ddsd.dwCaps = 0x4000;
        ddsd.dwWidth = g_screencfg->w;
        ddsd.dwHeight = g_screencfg->h;
        if (g_ddraw->vtbl->CreateSurface(g_ddraw, &ddsd, &g_surface_74, 0))
            return 0;

        g_ddraw->vtbl->CreateClipper(g_ddraw, 0, &g_clipper, 0);
        SetClipping(&rc);
        {
            RgnData* rgn = (RgnData*)HeapAlloc_w(0x33);
            rgn->dwSize = 0x20;
            rgn->iType = 1;
            rgn->nCount = 1;
            rgn->nRgnSize = 0x10;
            rgn->rcBound = g_clip_rect;
            rgn->rect = g_clip_rect;
            g_clipper->vtbl->SetClipList(g_clipper, rgn, 0);
            HeapFree_w(rgn);
        }
        goto done;
    }

    adj.left = 0;
    adj.top = 0;
    adj.right = g_screencfg->w - 1;
    adj.bottom = g_screencfg->h - 1;
    AdjustWindowRect(&adj, 0x10cf0000, 0);
    WNDENV_Sethwnd(CreateWindowExA(0, kWndClass, kWndTitleWin, 0x10cf0000,
                                   0, 0, adj.right - adj.left + 1,
                                   adj.bottom - adj.top + 1, 0, 0,
                                   WNDENV_GethInstance(), 0));
    if (!WNDENV_Gethwnd())
        return 0;
    if (g_ddraw->vtbl->SetCooperativeLevel(g_ddraw, WNDENV_Gethwnd(), 8)) {
        DestroyWindow(WNDENV_Gethwnd());
        return 0;
    }
    if (!SetScreenDisplayMode()) {
        DestroyWindow(WNDENV_Gethwnd());
        return 0;
    }
    g_ddraw->vtbl->CreateClipper(g_ddraw, 0, &g_clipper, 0);
    g_clipper->vtbl->SetHWnd(g_clipper, 0, WNDENV_Gethwnd());

    ddsd.dwSize = sizeof(DDSurfaceDesc);
    ddsd.dwFlags = 1;
    ddsd.dwCaps = 0x200;
    if (g_ddraw->vtbl->CreateSurface(g_ddraw, &ddsd, &g_primary, 0)) {
        DestroyWindow(WNDENV_Gethwnd());
        return 0;
    }
    g_primary->vtbl->SetClipper(g_primary, g_clipper);

    ddsd.dwSize = sizeof(DDSurfaceDesc);
    ddsd.dwFlags = 7;
    ddsd.dwCaps = 0x40;
    ddsd.dwWidth = g_screencfg->w;
    ddsd.dwHeight = g_screencfg->h;
    if (g_ddraw->vtbl->CreateSurface(g_ddraw, &ddsd, &g_surface_78, 0)) {
        g_primary->vtbl->Release(g_primary);
        DestroyWindow(WNDENV_Gethwnd());
        return 0;
    }
    g_draw_surface = g_surface_78;

    ddsd.dwSize = sizeof(DDSurfaceDesc);
    ddsd.dwFlags = 7;
    ddsd.dwCaps = 0x40;
    ddsd.dwWidth = g_screencfg->w;
    ddsd.dwHeight = g_screencfg->h;
    if (g_ddraw->vtbl->CreateSurface(g_ddraw, &ddsd, &g_surface_74, 0))
        return 0;
done:
    return 1;
}
