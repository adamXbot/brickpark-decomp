/* LEGOLAND -- texture records, the detail-image registry and the texture BMP
 * loader.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field offsets, global addresses and callee argument counts are load-bearing;
 * names are ours. Types are defined LOCALLY on purpose (the shared header is
 * owned elsewhere).
 *
 * Three unrelated families live here because they are three adjacent tiers of
 * the same texture path:
 *
 *  - ObjFirstRider / ObjNextRider (0x00441870, 0x00441890): the two-call
 *    iterator over an item's rider list that rin.c's GetObjRiderN drives.
 *    Both are thin wrappers over one shared "advance the cursor to the next
 *    node whose key matches the instance" helper at 0x00441830, and the
 *    cursor is a single GLOBAL -- the iteration is not re-entrant.
 *
 *  - FindDetailImageSlot / DetailImage_AllocSlot (0x00496ff0, 0x00496f30):
 *    the ImageRec* registry sprite2.c's RegisterDetailImage/
 *    UnregisterDetailImage file detail-dependent images in. One flat array
 *    grown 0x80 slots at a time; a freed slot is a NULL hole that the next
 *    allocation reuses.
 *
 *  - ConvertSourceImage / BuildTextureRecord (0x004434d0, 0x004437d0): the
 *    8bpp-only texture loader (data3.c's LoadTextureImage = CreateSourceImage
 *    + this) and the 0x2c-byte texture record RegisterTextureImage fills from
 *    the loaded image.
 */
#include "legoland.h"

void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)

/* ---- CRT ---------------------------------------------------------------- */
extern void* HeapAlloc_w(unsigned int size);                   /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                              /* 0x0049e4d0 */
extern void* CRT_calloc(unsigned int count, unsigned int size); /* 0x004a020e */
extern void* realloc(void* p, unsigned int size);              /* 0x0049fca2 */

/* ---- RES ---------------------------------------------------------------- */
extern void* RES_OpenFile(const char* name);                   /* 0x00489b60 */
extern int   RES_ReadFile(void* file, void* buf, int len);     /* 0x00489cf0 */
#ifndef LEGOLAND_PORTABLE
extern void  RES_CloseFile(void* file);                        /* 0x00489de0 */
#else
extern int RES_CloseFile(void* file);                        /* 0x00489de0 */
#endif
extern int   RES_GetFilePointer(void* file);                   /* 0x00489db0 */
extern int   RES_SetFilePointer(void* file, int pos);          /* 0x00489d70 */

/* ------------------------------------------------------------------------ *
 *  Rider iteration                                                          *
 * ------------------------------------------------------------------------ */

/* One entry of an item's rider list. Only the link and the 16-bit key the
 * cursor helper matches on are observable from these three functions. */
typedef struct RiderNode RiderNode;
struct RiderNode {
    RiderNode*     next;      /* +0x00 */
    int            pad04[2];  /* +0x04 */
    unsigned short key;       /* +0x0c  matched against *(u16*)inst */
};

/* The item a rider list hangs off. rin.c's GetObjRiderN reads the rider
 * COUNT at +0x2e; the list head is at +0xcc. */
typedef struct RiderItem {
    unsigned char  pad00[0xcc]; /* +0x00 */
    RiderNode*     riders;      /* +0xcc */
} RiderItem;

/* The ONE cursor both iterators share -- ObjFirstRider seeds it from the
 * item, ObjNextRider steps it. 0x00441830 walks it forward to the next
 * matching node, parks it there and returns it, or clears it and returns 0.
 * First named here. */
extern RiderNode* g_rider_cursor;                              /* 0x0081c8cc */

/* 0x00441830 -- advance g_rider_cursor to the first node from the cursor on
 * whose key equals *(unsigned short*)inst. 16 instructions; its `item`
 * argument is DEAD (only `inst` is read), which is why both wrappers below
 * forward the pair unchanged. First named here. */
extern RiderNode* RiderCursorSeek(RiderItem* item, void* inst); /* 0x00441830 */

/* -------------------------------------------------------------------------
 * 0x00441870 -- start a rider walk over one item.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00441870
RiderNode* ObjFirstRider(RiderItem* item, void* inst)
{
    g_rider_cursor = item->riders;
    return RiderCursorSeek(item, inst);
}

/* -------------------------------------------------------------------------
 * 0x00441890 -- step the shared rider cursor on by one node.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00441890
RiderNode* ObjNextRider(RiderItem* item, void* inst)
{
    RiderNode* p = g_rider_cursor;

    /* The failure arm is EXILED past the return: `if (p == 0) return 0;` on
     * the value just loaded into eax emits the bare inline `ret` instead. */
    if (p) {
        g_rider_cursor = p->next;
        return RiderCursorSeek(item, inst);
    }
    return 0;
}

/* ------------------------------------------------------------------------ *
 *  The detail-image registry                                                *
 * ------------------------------------------------------------------------ */

extern void** g_detail_images;                                 /* 0x0079a7c4 */
extern int    g_detail_images_cap;                             /* 0x0079a7c8 */
extern int    g_detail_images_count;                           /* 0x0079a7cc */

/* -------------------------------------------------------------------------
 * 0x00496ff0 -- the slot one image occupies, or 0. UnregisterDetailImage
 * (0x00497020) NULLs what this returns.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00496ff0
void** FindDetailImageSlot(void* image)
{
    void** p = g_detail_images;
    int    i;

    /* p++ in the INCREMENT clause after i++: the counter's update has to be
     * generated before the other induction variable's (inc ecx / add eax,4). */
    for (i = 0; i < g_detail_images_cap; i++, p++) {
        if (*p == image)
            return p;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * 0x00496f30 -- hand out a free registry slot, growing the array by 0x80 when
 * every slot is taken.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00496f30
void** DetailImage_AllocSlot(void)
{
    void** list;
    void** grown;
    int    i;

    if (g_detail_images_count == g_detail_images_cap) {
        if (g_detail_images == 0) {
            grown = (void**)CRT_calloc(0x80, 4);
        } else {
            grown = (void**)realloc(g_detail_images,
                                    g_detail_images_cap * 4 + 0x200);
            if (grown == 0)
                return 0;
            memset(grown + g_detail_images_cap, 0, 0x80 * 4);
        }
        if (grown == 0)
            return 0;
        g_detail_images = grown;
        g_detail_images_cap += 0x80;
        return &g_detail_images[g_detail_images_cap - 0x80];
    }

    list = g_detail_images;
    for (i = 0; i < g_detail_images_cap; i++, list++) {
        if (*list == 0)
            return list;
    }
    return 0;
}

/* ------------------------------------------------------------------------ *
 *  Textures                                                                 *
 * ------------------------------------------------------------------------ */

/* A decoded source bitmap -- sprite2.c's ImageRec, seen from the texture
 * side: what CreateSourceImage returns and ConvertSourceImage fills. The
 * +0x00 block is a raw 8bpp w*h image here, not an LLS. */
typedef struct SrcImage {
    unsigned char* pixels;     /* +0x00 */
    void*          pal;        /* +0x04 */
    short          w;          /* +0x08 */
    short          h;          /* +0x0a */
    unsigned short refs;       /* +0x0c */
    unsigned char  kind;       /* +0x0e */
    unsigned char  pad0f;      /* +0x0f */
    char*          name;       /* +0x10 */
    int            type;       /* +0x14 */
} SrcImage;

/* One cached 64-level shading ramp (tri3d.c). */
typedef struct Shade {
    int             levels;    /* +0x00 */
    unsigned short* table;     /* +0x04 */
} Shade;

/* The 0x2c-byte texture descriptor RegisterTextureImage (0x00488670) mallocs
 * and files in g_textures[slot]; tri3d.c's two textured fillers read it as
 *   texel = texels[(high16(v) << ushift) + high16(u)]
 *   pixel = ramps[texel]->table[high16(shade << 6)]
 * -- V is the ROW and +0x00 is the row shift, which is exactly what the
 * row-major fill in BuildTextureRecord below does (`texels[img->w * y + x]`,
 * and +0x00 is set from img->w).  PORT-B5 found this line transposed here and
 * in tri3d.c's file header; PORT-M5 corrected both.  Nothing in the C moved:
 * this body is exact, so `texels[w*y + x]` IS the original's addressing
 * (0x00443933 `imul eax, edx` with edx = y and eax = img->w, then 0x00443949
 * `mov byte ptr [esi+edi], cl` with edi = x).
 * Only the six fields this file fills are known; the remaining 0x14 bytes are
 * left as the malloc found them. */
typedef struct Texture {
    int            ushift;     /* +0x00  log2 of the u extent */
    int            vshift;     /* +0x04  log2 of the v extent */
    unsigned char* texels;     /* +0x08  w*h, one byte per texel */
    Shade**        ramps;      /* +0x0c  0x404 bytes, one ramp per texel VALUE */
    int            umask;      /* +0x10  w - 1 */
    int            vmask;      /* +0x14  h - 1 */
    int            pad18[5];   /* +0x18 */
} Texture;

typedef struct RGBQuad { unsigned char b, g, r, x; } RGBQuad;

/* The 256-entry BMP palette ConvertSourceImage reads every texture's colour
 * table into -- a single GLOBAL scratch, so BuildTextureRecord must run
 * before the next texture is loaded. tri3d.c's note on the ramp key confirms
 * the triple is stored BLUE, GREEN, RED, i.e. RGBQUAD order. First named
 * here. */
extern RGBQuad g_texture_palette[256];                         /* 0x0081c4c0 */

extern Shade* MakeShadedColour(int levels, unsigned char* rgb); /* 0x00486280 */

/* -------------------------------------------------------------------------
 * 0x004437d0 -- fill a fresh texture descriptor from a loaded source image.
 *
 * The u/v shifts come from a nine-case switch over the pixel dimension: a
 * texture must be a power of two from 1 to 256, and a dimension that is NOT
 * one of those leaves the shift field as malloc left it (RegisterTextureImage
 * does not clear the record) -- reproduced, not fixed.
 *
 * ORIGINAL BUG, reproduced: `if (img)` is tested only AFTER img->w, img->h
 * and img->w*img->h have already been read through it, so the guard can only
 * ever fire after a null dereference has already crashed.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004437d0
void BuildTextureRecord(SrcImage* img, Texture* tex)
{
    int           x, y;
    unsigned char c;

#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
    if (!img)                /* QUIRKS.md B: texture.c:244 -- img was tested only after being read */
        return;
#endif
    tex->umask = img->w - 1;
    tex->vmask = img->h - 1;

    switch (img->w) {
    case 1:   tex->ushift = 0; break;
    case 2:   tex->ushift = 1; break;
    case 4:   tex->ushift = 2; break;
    case 8:   tex->ushift = 3; break;
    case 16:  tex->ushift = 4; break;
    case 32:  tex->ushift = 5; break;
    case 64:  tex->ushift = 6; break;
    case 128: tex->ushift = 7; break;
    case 256: tex->ushift = 8; break;
    }

    switch (img->h) {
    case 1:   tex->vshift = 0; break;
    case 2:   tex->vshift = 1; break;
    case 4:   tex->vshift = 2; break;
    case 8:   tex->vshift = 3; break;
    case 16:  tex->vshift = 4; break;
    case 32:  tex->vshift = 5; break;
    case 64:  tex->vshift = 6; break;
    case 128: tex->vshift = 7; break;
    case 256: tex->vshift = 8; break;
    }

    tex->texels = (unsigned char*)HeapAlloc_w(img->h * img->w);
    if (img) {
        tex->ramps = (Shade**)HeapAlloc_w(0x404);
        memset(tex->ramps, 0, 0x404);
        for (y = 0; y < img->h; y++) {
            for (x = 0; x < img->w; x++) {
                c = img->pixels[img->w * y + x];
                tex->texels[img->w * y + x] = c;
                if (tex->ramps[c] == 0)
                    tex->ramps[c] = MakeShadedColour(
                        0x40, (unsigned char*)&g_texture_palette[c]);
            }
        }
    }
}

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

extern char* GetGFXFName(const char* name, unsigned char type, char* buf);
                                                               /* 0x0044de90 */

/* -------------------------------------------------------------------------
 * 0x004434d0 -- load an image record's backing BMP as a texture source.
 *
 * The texture cousin of screen.c's __BMPLoader: same BITMAPFILEHEADER +
 * BITMAPINFO read through RES, same "palette lives at the current file
 * position + 40" assumption, same uncompressed-only rule -- but 8bpp ONLY,
 * and the destination is an UNPADDED w*h byte image rather than the padded
 * rows kept in place. The padded rows are read into a scratch block and
 * copied out row by row from the LAST row backwards, so the bottom-up BMP
 * order is flipped and the pad bytes dropped during the copy. The colour
 * table goes into the single global g_texture_palette, where the matching
 * BuildTextureRecord call picks it up.
 *
 * ORIGINAL BUGS, reproduced:
 *  - both allocation-failure paths call free() on the ImageRec itself, which
 *    data3.c's LoadTextureImage then KillImage's -- the same double free
 *    screen.c's __BMPLoader carries;
 *  - biBitCount is tested against 8 a SECOND time after the allocations,
 *    although the function has already returned 0 for anything else, so the
 *    else edge of that test is unreachable;
 *  - img->pal is given a fresh 0x400-byte block that nothing in this function
 *    ever writes (the palette goes to the global instead).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004434d0
int ConvertSourceImage(SrcImage* img)
{
    unsigned char* dst;
    unsigned char* raw;
    int            palpos;
    int            offbits;
    int            stride;
    BmpFileHeader  fhdr;
    BmpInfo        bmi;
    void*          f;
    unsigned char* row;
    int            size;
    int            x;
    int            y;

    f = RES_OpenFile(GetGFXFName(img->name, img->kind, 0));
    if (!f)
        return 0;

    RES_ReadFile(f, &fhdr, sizeof(fhdr));
    offbits = fhdr.bfOffBits;
    palpos = RES_GetFilePointer(f) + 40;
    RES_ReadFile(f, &bmi, sizeof(bmi));

    if (bmi.bmiHeader.biCompression) {
        RES_CloseFile(f);
        return 0;
    }
    if (bmi.bmiHeader.biBitCount != 8) {
        RES_CloseFile(f);
        return 0;
    }

    stride = (bmi.bmiHeader.biWidth + 3) & ~3;
    size = bmi.bmiHeader.biHeight * stride;
    RES_SetFilePointer(f, fhdr.bfOffBits);
    raw = (unsigned char*)HeapAlloc_w(size);
    if (!raw) {
        HeapFree_w(img);              /* the ImageRec, not the block -- bug */
        RES_CloseFile(f);
        return 0;
    }

    img->pal = HeapAlloc_w(0x400);
    img->w = (short)bmi.bmiHeader.biWidth;
    img->h = (short)bmi.bmiHeader.biHeight;
    img->type = 1;
    img->pixels = (unsigned char*)HeapAlloc_w(bmi.bmiHeader.biHeight
                                              * bmi.bmiHeader.biWidth);
    if (img->pixels == 0) {
        HeapFree_w(raw);
        HeapFree_w(img);              /* the ImageRec again -- same bug */
        RES_CloseFile(f);
        return 0;
    }

    if (bmi.bmiHeader.biBitCount == 8) {   /* already proved above */
        row = raw + size;
        dst = img->pixels;
        RES_SetFilePointer(f, palpos);
        memset(g_texture_palette, 0, 0x400);
        RES_ReadFile(f, g_texture_palette, bmi.bmiHeader.biClrUsed * 4);
        RES_SetFilePointer(f, offbits);
        RES_ReadFile(f, raw, size);
        for (y = 0; y < img->h; y++) {
            /* `p` must be its own cursor: `row[x]` leaves row loop-invariant
             * and VC6 addresses the source as [row+x] instead of stepping a
             * pointer.  `p` coalesces into row's register, which is why the
             * row home is written once per ROW and never inside the copy.
             * And the two increments have to be split the original's way --
             * `*dst++ = *p++` emits the store before p's increment. */
            unsigned char* p;
            row -= stride;
            p = row;
            for (x = 0; x < img->w; x++) {
                *dst = *p++;
                dst++;
            }
        }
        HeapFree_w(raw);
        RES_CloseFile(f);
        return 1;
    }
    /* The tail is written TWICE on purpose. Two calls plus an argument block
     * is over VC6's tail-duplication threshold, so the two sites share one
     * copy of the calls and keep only their own `push raw` -- the original's
     * `push ecx / jmp` beside `push edi`, the else edge still holding raw in
     * edi. One copy after the `if` merges the two pushes into a reload and
     * loses three instructions. */
    HeapFree_w(raw);
    RES_CloseFile(f);
    return 1;
}
