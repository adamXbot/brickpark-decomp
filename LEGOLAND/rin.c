/* LEGOLAND - RIN 3D model loader, palettes and 3D person rendering.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; the field and type names are ours.
 * Each matched function carries its original VA marker on the line above it.
 *
 * ---------------------------------------------------------------------------
 * THE RIN RECORD (LoadRin / UnLoadRin / RenderUsingRin)
 *
 * A .RIN is NOT a mesh: it is a "render instruction" script for a placed
 * object - a list of .lls sprite names plus, per animation frame, an ordered
 * list of which sprite slots to draw.  On disk:
 *
 *     int   n_sprites
 *     char  name[n_sprites][]      NUL-terminated, read a byte at a time
 *     int   n_frames
 *     int   frame[n_frames][n_sprites]   sprite-slot index per draw step
 *
 * Each name becomes "<dir>\<name>.lls" and is loaded with LoadSprite(.., 1).
 * The 0x20-byte in-memory record is:
 *
 *     +0x00 ox, +0x04 oy   pixel offset of the whole rin (2x, halved by the
 *                          view mode like every other object offset)
 *     +0x08 remap          optional int table: slot -> rider index
 *     +0x0c riders         optional table indexed by remap[] (0 = none)
 *     +0x10 n_sprites      +0x18 sprites[]  (Sprite*)
 *     +0x14 n_frames       +0x1c frames[]   (int[n_sprites] per frame)
 *
 * RenderUsingRin walks the frame's slot list BACKWARDS (last slot first),
 * so the file lists sprites front-to-back and they are painted back-to-front.
 * For each slot it first asks the object for the rider occupying that slot
 * (GetObjRiderN) and, when that rider's bloke has flags62 bit 7 set (it is
 * "in 3D"), draws the bloke's 3D person right there in the paint order; then
 * it sets the slot sprite's LLS frame to the rin frame and blits it at the
 * object's screen position plus the rin offset.  That is how riders appear
 * seated inside a ride: the rin script interleaves them between the ride's
 * own sprite layers.
 *
 * ---------------------------------------------------------------------------
 * THE BINV RECORD (LoadBinV)
 *
 * A .BINV (BNV path binary, magic 0x0101) is loaded with plain stdio (fopen /
 * fseek / ftell / fread), the only asset loader in the game not to go through
 * the RES layer.  It is a self-contained image whose internal pointers are
 * stored as offsets from the start of the file; LoadBinV relocates them in
 * place:
 *
 *     +0x02 frame_count (u16)     +0x20 frames -> first BNVNameList
 *     BNVNameList: +0x00 count, +0x04 head (first BNVNameNode), +0x08 next
 *     BNVNameNode: +0x04 next, +0x08 vertices, +0x0c name
 *
 * (Names as in bnvpath.c; GetBinVFrame walks the list through +0x08.)  The
 * inner loop relocates exactly `count` nodes and only follows a non-null
 * `next`; a zero offset stays zero.
 *
 * ---------------------------------------------------------------------------
 * PALETTES (LoadPalette / LoadColourTable)
 *
 * LoadPalette reads a 256-entry 8-byte-header palette file into a fresh
 * u16[256]: every entry is three bytes r, g, b packed as RGB565 when the
 * screen-depth selector (0x668088) is 2, else RGB555 - the same formulas as
 * sweep1.c's colour helpers.
 *
 * LoadColourTable reads ".\graphics\colours.tga": an 18-byte TGA header,
 * then a 256 x BGR colour map, then the 0x8000-byte 15-bit -> palette-index
 * lookup table (g_palette_lut, the table sweep1.c indexes).  The colour map
 * is turned into a PALETTEENTRY[256] (peFlags = PC_NOCOLLAPSE) for
 * IDirectDraw::CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256) and set on the
 * primary surface, and a parallel u16[256] of the same colours in RGB555.
 * Note the header and the colour map are read into the SAME 0x300-byte
 * buffer - the header is discarded.
 *
 * ---------------------------------------------------------------------------
 * RENDER3DPERSON
 *
 * The person is drawn by the software rasteriser directly into the locked
 * video surface.  The 160x120 (0xa0 x 0x78) model window at the person's
 * screen position (+0x1c/+0x20) is clipped against the game's clip rect
 * (g_clip_rect, made inclusive by the -1s), moved to window-relative
 * coordinates, and handed to Render_SetViewport; the raster target is the
 * surface pixel at the window origin (16-bit pixels, hence *2).  The x87
 * control word is switched to 0x7f (24-bit precision, all exceptions
 * masked) around the draw and restored afterwards.
 *
 * Afterwards g_raster_hit (zeroed by SetRasterOrigin and set by the
 * rasteriser when it painted the cursor pixel) says the mouse is over this
 * person; unless the selection lock is on and this is already the selected
 * bloke, the hit record {type, bloke} is filled with 0x306 / 0x307 / 0x308
 * for person kinds 1 / 2 / 3 - the same type codes SortBlokeIn3D uses.
 *
 * CODEGEN NOTES (VC6 SP3, /O2 /Gy /Gd)
 * - Render3DPerson keeps an ebp frame because of the __asm blocks; the
 *   `wait` before fnstcw is the waiting `fstcw` mnemonic.  The four call
 *   cleanups (0x20) are deferred across the asm to after Draw3DPersonModel.
 * - RenderUsingRin: the slot pointer and countdown live in the dead
 *   parameter slots of item/inst (deferred push edi, loop-only locals).
 * - LoadPalette: the r byte is homed in the dead fname slot; g and b sit
 *   below the 8-byte header.  The 565/555 packing is written with u16 casts
 *   so the byte ANDs / xor-mov widenings come out as in the original; the 565
 *   arm additionally carries a deleted-by-VC6 ghost compare that keeps its red
 *   term widened before it is masked (see the note above the function).
 * --------------------------------------------------------------------------- */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* The RIN record (see the file comment). */
typedef struct Rin {
    int     ox;         /* +0x00 */
    int     oy;         /* +0x04 */
    int*    remap;      /* +0x08 slot -> rider index */
    int*    riders;     /* +0x0c rider table (0 = the slot IS the rider index) */
    int     n_sprites;  /* +0x10 */
    int     n_frames;   /* +0x14 */
    void**  sprites;    /* +0x18 */
    int**   frames;     /* +0x1c */
} Rin;

/* A rider slot occupant as returned by GetObjRiderN: its bloke is at +0x08. */
typedef struct Rider {
    unsigned char pad00[8];   /* +0x00 */
    struct Bloke* bloke;      /* +0x08 */
} Rider;

/* A bloke; flags62 bit 7 = drawn in 3D. */
typedef struct Bloke {
    unsigned char  pad00[0x62];
    unsigned short flags62;   /* +0x62 */
} Bloke;

/* The 3D person (blokeai.c's Person3D); only the fields touched here. */
typedef struct Person3D {
    unsigned char pad00[8];   /* +0x00 */
    int           kind;       /* +0x08 */
    Bloke*        bloke;      /* +0x0c */
    float         scale_x;    /* +0x10 */
    float         scale_y;    /* +0x14 */
    float         scale_z;    /* +0x18 */
    int           sx;         /* +0x1c screen x of the model window */
    int           sy;         /* +0x20 screen y */
} Person3D;

/* BinV image (bnvpath.c names). */
typedef struct BNVNameNode {
    unsigned char        pad00[4];
    struct BNVNameNode*  next;      /* +0x04 */
    void*                vertices;  /* +0x08 */
    char*                name;      /* +0x0c */
} BNVNameNode;

typedef struct BNVNameList {
    int                  count;     /* +0x00 */
    BNVNameNode*         head;      /* +0x04 */
    struct BNVNameList*  next;      /* +0x08 */
} BNVNameList;

typedef struct BNVBin {
    unsigned short       magic;       /* +0x00 0x0101 */
    unsigned short       frame_count; /* +0x02 */
    unsigned char        pad04[0x1c];
    BNVNameList*         frames;      /* +0x20 */
} BNVBin;

/* Win32 RECT / PALETTEENTRY. */
typedef struct WinRect {
    int left;
    int top;
    int right;
    int bottom;
} WinRect;

typedef struct PalEntry {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char flags;
} PalEntry;

/* surface.c's VideoSurfaceInfo. */
typedef struct VideoSurfaceInfo {
    long  pitch;    /* +0x00 */
    int   width;    /* +0x04 */
    int   height;   /* +0x08 */
    void* bits;     /* +0x0c */
    int   unused;   /* +0x10 */
    int   format;   /* +0x14 */
} VideoSurfaceInfo;

/* IDirectDraw (+0x14 CreatePalette) and IDirectDrawSurface (+0x7c SetPalette). */
typedef struct DDraw DDraw;
typedef struct DDrawVtbl {
    unsigned char pad00[0x14];
    long(__stdcall* CreatePalette)(DDraw*, unsigned int flags, PalEntry* entries,
                                   void** out, void* outer);      /* +0x14 */
} DDrawVtbl;
struct DDraw { DDrawVtbl* vtbl; };

typedef struct Surface Surface;
typedef struct SurfaceVtbl {
    unsigned char pad00[0x7c];
    long(__stdcall* SetPalette)(Surface*, void* palette);           /* +0x7c */
} SurfaceVtbl;
struct Surface { SurfaceVtbl* vtbl; };

/* The mouse-hit record {type, bloke} (0x004bdd00). */
typedef struct HitInfo {
    int    type;    /* +0x00 0x306 person, 0x307 kind 2, 0x308 kind 3 */
    Bloke* bloke;   /* +0x04 */
} HitInfo;

/* An animation record of the 3D character system (blokeanim.c's Anim3D). */
typedef struct Anim3D Anim3D;

/* ---------------------------------------------------------------- globals -- */

extern unsigned char  g_tga_buf[0x300];        /* 0x00813b20 header, then colour map */
extern unsigned short g_pal555[256];           /* 0x00813e20 */
extern unsigned char  g_palette_lut[0x8000];   /* 0x00814020 */
extern DDraw*         g_ddraw;                 /* 0x00667d74 */
extern Surface*       g_screen_surface;        /* 0x00668070 */
extern void*          g_screen_palette;        /* 0x00668084 */
extern int            g_screen_depth;          /* 0x00668088 */

extern WinRect        g_clip_rect;             /* 0x004bdea0 */
extern Pos            g_raster_origin;         /* 0x00813a44 */
extern int            g_raster_hit;            /* 0x007feb14 */
extern int            g_selection_lock;        /* 0x00668954 */
extern HitInfo        g_hit_info;              /* 0x004bdd00 */
extern unsigned short g_fpu_cw_saved;          /* 0x00638358 */
extern unsigned short g_fpu_cw_render;         /* 0x004b7abc (0x7f) */

extern void*          g_anim_ctx_kind1;        /* 0x0081c8c0 */
extern void*          g_anim_ctx_kind3;        /* 0x0081c8c4 */
extern void*          g_anim_ctx_kind2;        /* 0x0081c8c8 */
extern char*          g_texnames_boy;          /* 0x00630100 */
extern char*          g_texnames_girl;         /* 0x0062feac */
extern Anim3D*        g_anim_kind2[2];         /* 0x0062feb0 */
extern Anim3D*        g_anim_kind1a[6];        /* 0x0062febc */
extern Anim3D*        g_anim_kind1b[6];        /* 0x0062fed4 */
extern Anim3D*        g_anim_kind3[1];         /* 0x0062fef4 */

extern char           g_gfx_name_buf[];        /* 0x006660b0 */
extern char*          g_gfx_dirs[7];           /* 0x004b81c0: graphics, graphics,
                                                * small, masks, masks\small, icons,
                                                * models */

/* ------------------------------------------------------------ prototypes -- */

extern void*  HeapAlloc_w(unsigned int size);                     /* 0x0049e4ff */
extern void   HeapFree_w(void* p);                                /* 0x0049e4d0 */
extern int    KillSprite(void* sprite);                           /* 0x00497bd0 */
extern void*  LoadSprite(const char* name, int flag);             /* 0x00497ab0 */
extern void*  RES_OpenFile(const char* path);                     /* 0x00489b60 */
extern int    RES_ReadFile(void* f, void* buf, int len);          /* 0x00489cf0 */
extern int    RES_CloseFile(void* f);                             /* 0x00489de0 */
extern Offset GetScreenCoordsForObject(void* inst, void* item);   /* 0x00442cc0 */
extern void   AdjustOffsetForViewMode(Offset* o);                 /* 0x00442d30 */
extern Rider* GetObjRiderN(int n, void* item, void* inst);        /* 0x004418c0 */
extern void   IP_RenderBlokeIn3DNow(Bloke* b);                    /* 0x00440010 */
extern void*  GetLLSForSprite(void* sprite);                      /* 0x00441e80 */
extern void   LLSSetFrame(void* lls, int frame);                  /* 0x0047d5a0 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern int    GetVideoSurface(VideoSurfaceInfo* out);             /* 0x00464310 */
extern void   SetRasterTarget(void* bits, int pitch, int w, int h); /* 0x00485f30 */
extern void   SetRasterOrigin(void* bits, Pos* origin);           /* 0x00488700 */
extern void   Render_SetViewport(WinRect* r);                     /* 0x00441800 */
extern void   Draw3DPersonModel(Person3D* p);                     /* 0x00440a30 */
extern Bloke* GetSelectedBloke(void);                             /* 0x004700f0 */
extern void   FreeAnim3D(Anim3D* a);                              /* 0x0043fde0 */
extern void   UnInitModelTextures(void);                          /* 0x00442c70 */
extern void   UnInit3DPrintList(void);                            /* 0x00486250 */
extern void   UnInitRasterTables(void);                           /* 0x004886a0 */
extern void   FreeRasterBuffer(void);                             /* 0x00485fa0 */

__declspec(dllimport) int __stdcall IntersectRect(WinRect* dst, const WinRect* a,
                                                  const WinRect* b);   /* [0x4ab2a0] */
__declspec(dllimport) int __stdcall OffsetRect(WinRect* r, int dx, int dy); /* [0x4ab29c] */

int   sprintf(char*, const char*, ...);
void* fopen(const char*, const char*);
int   fseek(void*, long, int);
long  ftell(void*);
unsigned int fread(void*, unsigned int, unsigned int, void*);
int   fclose(void*);
void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)

/* -------------------------------------------------------------- functions -- */

// FUNCTION: LEGOLAND 0x00441cf0
void UnLoadRin(Rin* rin)
{
    int i;

    for (i = 0; i < rin->n_sprites; i++)
        KillSprite(rin->sprites[i]);
    HeapFree_w(rin->sprites);
    for (i = 0; i < rin->n_frames; i++)
        HeapFree_w(rin->frames[i]);
    HeapFree_w(rin->frames);
    HeapFree_w(rin);
}

/* Ends in `jmp FreeRasterBuffer` (a void call in tail position), no ret:
 * audit.py bounds it and certifies all 72 instructions; the shared match.py /
 * verify.py cannot bound a tail-jmp function, hence the WIP marker. */
// FUNCTION: LEGOLAND 0x004405a0
void UnInitMan(void)
{
    int i;

    if (g_anim_ctx_kind1)
        HeapFree_w(g_anim_ctx_kind1);
    if (g_anim_ctx_kind2)
        HeapFree_w(g_anim_ctx_kind2);
    if (g_anim_ctx_kind3)
        HeapFree_w(g_anim_ctx_kind3);
    if (g_texnames_boy)
        HeapFree_w(g_texnames_boy);
    if (g_texnames_girl)
        HeapFree_w(g_texnames_girl);
    for (i = 0; i < 6; i++) {
        if (g_anim_kind1a[i])
            FreeAnim3D(g_anim_kind1a[i]);
    }
    for (i = 0; i < 6; i++) {
        if (g_anim_kind1b[i])
            FreeAnim3D(g_anim_kind1b[i]);
    }
    for (i = 0; i < 2; i++) {
        if (g_anim_kind2[i])
            FreeAnim3D(g_anim_kind2[i]);
    }
    if (g_anim_kind3[0])
        FreeAnim3D(g_anim_kind3[0]);
    UnInitModelTextures();
    UnInit3DPrintList();
    UnInitRasterTables();
    FreeRasterBuffer();
}

// FUNCTION: LEGOLAND 0x0044e580
void LoadColourTable(void)
{
    PalEntry pe[256];
    void*    f;
    int      i;

    f = RES_OpenFile(".\\graphics\\colours.tga");
    RES_ReadFile(f, g_tga_buf, 0x12);
    RES_ReadFile(f, g_tga_buf, 0x300);
    for (i = 0; i < 256; i++) {
        unsigned char b;
        pe[i].r = g_tga_buf[i * 3 + 2];
        pe[i].g = g_tga_buf[i * 3 + 1];
        b = g_tga_buf[i * 3];
        pe[i].b = b;
        g_pal555[i] = (unsigned short)((((unsigned short)(pe[i].r & 0xf8) << 5)
                                        | (unsigned short)(pe[i].g & 0xf8)) << 2)
                    | (unsigned short)(b >> 3);
        pe[i].flags = 4;   /* PC_NOCOLLAPSE; last so the store sinks past the loads */
    }
    RES_ReadFile(f, g_palette_lut, 0x8000);
    g_ddraw->vtbl->CreatePalette(g_ddraw, 0x44, pe, &g_screen_palette, 0);
    g_screen_surface->vtbl->SetPalette(g_screen_surface, g_screen_palette);
    RES_CloseFile(f);
}

// FUNCTION: LEGOLAND 0x0044dc90
BNVBin* LoadBinV(const char* fname)
{
    void*        f;
    int          size;
    BNVBin*      bin;
    BNVNameList* list;
    BNVNameNode* node;
    int          i;
    int          j;

    f = fopen(fname, "rb");
    if (f) {
        fseek(f, 0, 2);
        size = ftell(f);
        fseek(f, 0, 0);
        bin = (BNVBin*)HeapAlloc_w(size);
        fread(bin, 1, size, f);
        fclose(f);
        if (bin->magic != 0x101) {
            HeapFree_w(bin);
        } else {
            bin->frames = (BNVNameList*)((char*)bin + (int)bin->frames);
            list = bin->frames;
            for (i = 0; i < bin->frame_count; i++) {
                list->head = (BNVNameNode*)((char*)bin + (int)list->head);
                node = list->head;
                for (j = 0; j < list->count; j++) {
                    node->vertices = (char*)bin + (int)node->vertices;
                    node->name = (char*)bin + (int)node->name;
                    if (node->next) {
                        node->next = (BNVNameNode*)((char*)bin + (int)node->next);
                        node = node->next;
                    }
                }
                if (list->next) {
                    list->next = (BNVNameList*)((char*)bin + (int)list->next);
                    list = list->next;
                }
            }
            return bin;
        }
    }
    return 0;
}

/* MATCHED 2026-09-04: 101/101 instructions, 273/273 bytes, index for index.
 *
 * This is the same 565/555 palette-packing idiom as __BMPLoader (screen.c,
 * 0x44e352 / 0x44e496), and the C that matches there transfers verbatim:
 * red is an `unsigned short` local, green and blue are `unsigned char` locals
 * masked/shifted at their declarations, and the pack is spelled with
 * MULTIPLIES -- `(((r & ~7) * 32) | g) * 8 | b`.  The multiply is what widens
 * the byte before masking; the `<< 5` spelling masks the byte instead
 * (`and dl,0F8h`) and loses the original's `movzx dx, byte ptr [..]`.
 *
 * WHAT CLOSED THE LAST MISMATCH (index 51, worth 3 bytes): THE 16-BIT
 * DESTINATION IS SIGNED.  `p` is a `short*`, not an `unsigned short*`.  With
 * an unsigned destination VC6's narrowing pass takes the red channel's `& ~7`
 * down to the 16-bit operand width (`and edx,0FFF8h`, 6 B); with a signed one
 * it leaves the mask 32-bit and unclean (`and edx,-8`, 3 B) on top of the same
 * 16-bit movzx.  The rule was already recorded in screen.c's header for
 * __BMPLoader -- the mask width is decided by the SIGNEDNESS OF THE STORE, not
 * by any spelling of the mask itself.  Every K in `t & K` (0xf8, ~7, 0xfff8,
 * 0xfffffff8, -8, (t>>3)<<3, ...) canonicalises to one node before the
 * narrowing pass runs, which is why ~75 red spellings across two earlier
 * passes never moved it; nor do `int` / `unsigned` temp types.  The earlier
 * `unsigned short t = r; if (t != r) ...` ghost is no longer needed: with the
 * signed store the u16 local keeps its movzx on its own.
 *
 * The function returns `unsigned short*`, so `p` is `(short*)pal`.  The buffer
 * is 0x200 bytes = 256 entries, zeroed with an intrinsic memset (a bare
 * rep stosd), then filled a byte at a time from the open file -- r, g and b
 * are three separate one-byte RES_ReadFile calls into address-taken byte
 * locals, and `r` is homed in the dead `fname` argument slot.  The 8-byte
 * header read at the top is discarded. */
// FUNCTION: LEGOLAND 0x00441f20
unsigned short* LoadPalette(const char* fname)
{
    unsigned short* pal;
    short*          p;        /* SIGNED: see the note above -- it decides the
                               * width of the 565 red mask. */
    void*           f;
    char            hdr[8];
    int             i;

    pal = (unsigned short*)HeapAlloc_w(0x200);
    if (pal) {
        memset(pal, 0, 0x200);
        f = RES_OpenFile(fname);
        if (f) {
            unsigned char r;   /* homed in the dead fname slot */
            unsigned char g;
            unsigned char b;
            RES_ReadFile(f, hdr, 8);
            p = (short*)pal;
            for (i = 0; i < 256; i++) {
                RES_ReadFile(f, &r, 1);
                RES_ReadFile(f, &g, 1);
                RES_ReadFile(f, &b, 1);
                if (g_screen_depth == 2) {
                    unsigned short rr = r;
                    unsigned char  gg = (unsigned char)(g & 0xfc);
                    unsigned char  bb = (unsigned char)(b >> 3);
                    *p = (short)((((rr & ~7) * 32) | gg) * 8 | bb);
                } else {
                    unsigned char  r5 = (unsigned char)(r & 0xf8);
                    unsigned char  g5 = (unsigned char)(g & 0xf8);
                    unsigned char  b5 = (unsigned char)(b >> 3);
                    *p = (short)((((unsigned short)r5 << 5
                                 | (unsigned short)g5) << 2)
                                | (unsigned short)b5);
                }
                p++;
            }
            RES_CloseFile(f);
        }
    }
    return pal;
}

// FUNCTION: LEGOLAND 0x00441d60
void RenderUsingRin(Rin* rin, int frame, void* item, void* inst)
{
    Offset pos;
    Offset o;
    int*   list;
    int    f;
    int    i;
    int    idx;
    Rider* rider;
    void*  spr;

    pos = GetScreenCoordsForObject(inst, item);
    f = frame;
    if (frame >= rin->n_frames)
        f = frame % rin->n_frames;
    list = rin->frames[f];
    for (i = rin->n_sprites - 1; i >= 0; i--) {
        idx = list[i];
        if (rin->riders == 0) {
            rider = GetObjRiderN(idx, item, inst);
        } else {
            int k = rin->remap[idx];
            rider = GetObjRiderN(rin->riders[k], item, inst);
        }
        if (rider && (rider->bloke->flags62 & 0x80))
            IP_RenderBlokeIn3DNow(rider->bloke);
        spr = rin->sprites[idx];
        if (spr)
            LLSSetFrame(GetLLSForSprite(spr), frame);
        o.ox = rin->ox;
        o.oy = rin->oy;
        AdjustOffsetForViewMode(&o);
        spr = rin->sprites[idx];
        if (spr)
            PrintSprite(spr, pos.ox + o.ox, pos.oy + o.oy, 0, 0);
    }
}

// FUNCTION: LEGOLAND 0x0043fe50
void Render3DPerson(Person3D* p)
{
    WinRect          r;
    WinRect          v;
    VideoSurfaceInfo vs;

    p->scale_x = 1.0f;
    p->scale_y = 1.0f;
    p->scale_z = 1.0f;
    v.left = p->sx;
    v.top = p->sy;
    v.right = p->sx + 0xa0;
    v.bottom = p->sy + 0x78;
    r = g_clip_rect;
    r.right--;
    r.bottom--;
    if (!IntersectRect(&r, &v, &r))
        return;
    OffsetRect(&r, -p->sx, -p->sy);
    if (!GetVideoSurface(&vs))
        return;
    SetRasterTarget((char*)vs.bits + p->sy * vs.pitch + p->sx * 2, vs.pitch, vs.width, vs.height);
    SetRasterOrigin(vs.bits, &g_raster_origin);
    Render_SetViewport(&r);
    __asm {
        fstcw word ptr [g_fpu_cw_saved]
        fldcw word ptr [g_fpu_cw_render]
    }
    Draw3DPersonModel(p);
    __asm fldcw word ptr [g_fpu_cw_saved]
    if (g_raster_hit == 0)
        return;
    if (g_selection_lock) {
        if (p->bloke == GetSelectedBloke())
            return;
    }
    switch (p->kind) {
    case 2:  g_hit_info.type = 0x307; break;
    case 3:  g_hit_info.type = 0x308; break;
    default: g_hit_info.type = 0x306; break;
    }
    g_hit_info.bloke = p->bloke;   /* VC6 tail-duplicates this into every case */
}

// FUNCTION: LEGOLAND 0x00441ba0
Rin* LoadRin(const char* fname, const char* dir)
{
    Rin*  rin;
    void* f;
    char  name[264];   /* 0x108 each: the frame is 4 + 4 + 0x108 + 0x108 = 0x218 */
    char  path[264];
    char  c;
    char* q;
    int   i;

    rin = (Rin*)HeapAlloc_w(sizeof(Rin));
    f = RES_OpenFile(fname);
    if (f) {
        memset(rin, 0, sizeof(Rin));
        RES_ReadFile(f, &rin->n_sprites, 4);
        rin->sprites = (void**)HeapAlloc_w(rin->n_sprites * 4);
        for (i = 0; i < rin->n_sprites; i++) {
            q = name;
            do {
                RES_ReadFile(f, &c, 1);
                *q++ = c;
            } while (c);
            sprintf(path, "%s\\%s.lls", dir, name);
            rin->sprites[i] = LoadSprite(path, 1);
        }
        RES_ReadFile(f, &rin->n_frames, 4);
        rin->frames = (int**)HeapAlloc_w(rin->n_frames * 4);
        if (rin->frames) {
            for (i = 0; i < rin->n_frames; i++) {
                rin->frames[i] = (int*)HeapAlloc_w(rin->n_sprites * 4);
                RES_ReadFile(f, rin->frames[i], rin->n_sprites * 4);
            }
        }
        RES_CloseFile(f);
    }
    return rin;
}

// FUNCTION: LEGOLAND 0x0044de90
char* GetGFXFName(const char* name, unsigned char type, char* buf)
{
    if (!buf)
        buf = g_gfx_name_buf;
    switch (type) {
    case 0: sprintf(buf, "%s%s", g_gfx_dirs[0], name); break;
    case 1: sprintf(buf, "%s%s", g_gfx_dirs[2], name); break;
    case 2: sprintf(buf, "%s%s", g_gfx_dirs[1], name); break;
    case 3: sprintf(buf, "%s%s", g_gfx_dirs[2], name); break;
    case 4: sprintf(buf, "%s%s", g_gfx_dirs[5], name); break;
    case 5: sprintf(buf, "%s%s", g_gfx_dirs[3], name); break;
    case 6: sprintf(buf, "%s%s", g_gfx_dirs[4], name); break;
    case 7: sprintf(buf, ".\\graphics\\duke\\%s", name); break;
    case 8: sprintf(buf, "%s%s.MDL", g_gfx_dirs[6], name); break;
    case 9: sprintf(buf, "%s%s", g_gfx_dirs[6], name); break;
    }
    return buf;
}
