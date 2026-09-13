/* LEGOLAND -- ten small, self-contained leaves from eight different
 * subsystems, none of which had a natural home in an existing translation
 * unit (the third such file; sysmisc.c and sysmisc2.c hold the earlier ones).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Only
 * struct field OFFSETS, global addresses, vtable slots and callee argument
 * counts are load-bearing; every name here is ours.
 *
 * ---------------------------------------------------------------------------
 * WHAT IS IN HERE, AND WHAT IT SAYS ABOUT THE GAME
 * ---------------------------------------------------------------------------
 *   0x0044f360  IsObjectRunning        bigsim.c    is the object on this map
 *                                                  square powered and switched
 *                                                  on?  The AI's build/repair
 *                                                  scan asks this per square.
 *   0x004738b0  CreateKeyboardDevice   input2.c    DirectInput keyboard.
 *   0x0047b7b0  LLIDB_LoadDataByIndex  memdb.c     *** MISNAMED, see below ***
 *                                                  the ICM error message box.
 *   0x0045cb20  PaintPathRect          pathsq.c    stamp the two checkerboard
 *                                                  path tiles over a rect.
 *   0x00460e00  RenderGroundLayer      renderview.c repaint the terrain layer
 *                                                  and clear the dirty flags.
 *   0x0047cdd0  LLIDB_UnLoadODFData    memdb.c     the .TSF tile-set unloader
 *                                                  (llidb_load.c's
 *                                                  LLIDB_LoadTSFData undone).
 *   0x00495b00  KillMusicSystem        audiomisc.c DirectMusic teardown.
 *   0x0043fde0  FreeAnim3D             rin.c       free a .3d morph animation.
 *   0x0045f540  CheckCursorFootprint   objmap2.c   chain the "one cell of
 *                                                  clearance" ghost onto a
 *                                                  placement cursor.
 *   0x00481e60  SetPathSquareDistance  bnvmove.c   squared distance from a
 *                                                  point to a path square.
 *
 * ---------------------------------------------------------------------------
 * TWO NAMES INHERITED FROM CALLERS THAT THE BODIES CONTRADICT
 * ---------------------------------------------------------------------------
 * 1. 0x0047b7b0 is NOT a loader.  memdb.c calls it `LLIDB_LoadDataByIndex`
 *    because LLIDB_RegisterNewElementB passes it the value
 *    LLIDB_RegisterNewElement returned; but that value is an element index
 *    when positive and an ERROR CODE when negative, and the body is a
 *    six-arm switch over the codes -6..-1 that puts up a MessageBoxA titled
 *    "LEGOLAND Installation & Configuration Manager".  For a real index it
 *    falls straight through the `ja` and returns.  The right name is
 *    LLIDB_ReportICMError; the assigned one is kept so the extern in memdb.c
 *    still resolves, and the misnomer is recorded here.
 *
 *    Recovered by the same reading: the six ICM failures the registrar can
 *    report, in code order --
 *      -1 "An element of the same name already exists."
 *      -2 "LEGOLAND.ICM could not be created or updated."
 *      -3 "The given element could not be found."
 *      -4 "No identification key was given."
 *      -5 "No file name was given."
 *      -6 "Operation was canceled."
 *    and -6 is exactly what memdb.c's LLIDB_SelectElement returns when the
 *    picker dialog is cancelled, which closes that loop.
 *
 * 2. 0x0047cdd0 is the .TSF unloader, not an .ODF one.  memdb.c's
 *    LLIDB_UnLoadData routes element type 0x40 here and named it
 *    LLIDB_UnLoadODFData; the record it frees is field for field
 *    llidb_load.c's TsfData (base_slot, n_tiles, sprites, codes, second,
 *    parent), and it hands the tile-code run back with FreeTileSpace --
 *    LLIDB_LoadTSFData's AllocTileSpace undone.  So LLIDB element type 0x40
 *    is .TSF, and the name is again kept for the extern's sake.
 *
 * ---------------------------------------------------------------------------
 * MECHANICS RECOVERED
 * ---------------------------------------------------------------------------
 * THE PLACEMENT CURSOR'S CLEARANCE RING.  CheckCursorFootprint walks the
 * cursor CHAIN looking for flag 0x1000 and, only when no link carries it,
 * appends the one shared spare cursor (0x00830fc0 -- logflume.c's
 * g_lf_cursor_c, screencb2.c's g_spare_cursor) holding the cursor's own
 * footprint GROWN BY ONE CELL on every side, mode 0x1008.  So 0x1000 means
 * "this chain already carries its own clearance ghost, do not add another",
 * and every class that does not set it is placed with a one-cell margin the
 * player can see.  It returns the ghost it appended -- or the link that
 * already had the bit, or 0 for a null cursor -- and both callers
 * (castleobj.c, objmap2.c) throw the result away, which is why they declare
 * two different return types.
 *
 * THE DRIVING-SCHOOL-STYLE PATH CHECKERBOARD.  PaintPathRect fills a rect of
 * map cells with two adjacent tiles picked by the PARITY of x + y:
 * base + 1 on odd squares, base + 2 on even ones, where base is the first
 * word of the run at *g_path_tile_base.  The step is computed 32-bit, masked
 * to a byte and added to the base in 16 bits.
 *
 * THE SIMULATION'S "IS IT RUNNING" TEST is two cell flag bits and one global:
 * 0x0200 (switched off / disconnected) always means not running; 0x0100
 * (blacked out) only counts when g_power_available says the park has power
 * at all -- so during a total blackout every object is still considered
 * running and the AI does not try to repair the whole park.
 *
 * ORIGINAL BUG reproduced in IsObjectRunning: the bounds-checked cell fetch
 * returns 0 for a square off the map and the flag word is read from it
 * anyway (`mov ax,[eax+0xc]`, i.e. address 0xc).  The AI only ever asks about
 * squares it already found an object on, so it does not fire.
 *
 * ---------------------------------------------------------------------------
 * CODEGEN LEVERS USED / RECOVERED
 * ---------------------------------------------------------------------------
 * - `sete al / test al,1` guard: CreateKeyboardDevice is CreateMouseDevice
 *   (sysmisc.c 0x00473970) minus SetCooperativeLevel and minus the wheel
 *   granularity query, and needs the same `((caps.dwFlags == 0) & 1) == 0`
 *   spelling with the success arm inline and the ONE trailing `return 0`.
 * - A jump-table switch's case order is its BLOCK order: 0x0047b7b0's six
 *   arms are emitted -1, -2, -3, -4, -5, -6 and the .rdata table at
 *   0x0047b848 is in the reverse (index) order, so the source is written -1
 *   first.  Each arm ends in its own `ret` because MessageBoxA is __stdcall.
 * - NEW: an explicit `& 0xff` and a `(unsigned char)` CAST are different
 *   objects.  PaintPathRect's parity step is `neg dl / sbb edx,edx /
 *   add edx,2 / and edx,0xff` -- the ternary evaluated 32-bit and narrowed
 *   afterwards.  Spelling the narrowing as a cast (either
 *   `unsigned char step = c ? 1 : 2;` or `(unsigned char)step` at the use)
 *   propagates the byte width BACKWARDS into the ternary and gives
 *   `sbb dl,dl` + `movzx dx,dl` instead -- two instructions wrong at
 *   identical length.  `step & 0xff`, `(unsigned)step & 0xff` and an
 *   `unsigned short` intermediate all reproduce the original; only the
 *   casts do not.  Reverse of the recorded "a char `x ? 1 : -1` narrows only
 *   via a char LOCAL": there the cast was needed, here it must be avoided.
 * - The same function's `x + y` must be an INT expression, not a byte one:
 *   as an int VC6 strength-reduces it into its own `inc esi` induction
 *   variable beside the cell cursor and pushes a FOURTH callee-saved
 *   register; narrowed to `unsigned char` it recomputes `bl = dl / add bl,al`
 *   every iteration, three instructions short.
 * - NEW: a value that must survive a call in a callee-saved register cannot
 *   be a FIELD of an address-taken aggregate.  RenderGroundLayer's two
 *   shifted scroll values fill a `Pos` that is passed by address; read back
 *   out of the Pos after the call they cost two reloads and merge the
 *   `add esp,8` into the epilogue (36 of 42, 11 bytes long).  As two plain
 *   `int` locals assigned INTO the Pos they take esi/edi across the call and
 *   it is exact.  Corollary in the same body: two reads of the same u16
 *   global field do NOT CSE across a store to the address-taken aggregate --
 *   `view.right = g_map->view_w + view.left` (reading the field back) is
 *   what gives the original's single load, where naming the global twice
 *   emits it twice.
 * - Independent statements come out REVERSED: SetPathSquareDistance's two
 *   `sub`s (and with them the two `imul`s that follow) are emitted in the
 *   opposite order to the source, while the `a*a + b*b` sum's own operand
 *   order is inert.  Write dx first to get dy first.
 * - Inlining a two-argument helper on two byte fields of one pointer:
 *   `MapCellAt(at->x, at->y)` makes the POINTER the first IR temp and it
 *   takes eax; hoisting the two fields into `int x, y` locals first pushes
 *   the pointer down the eax->ecx->edx rotation to ecx, which is where the
 *   original has it (7 of 41, identical length).
 * - A counted loop whose bound is a LOCAL gets a down-counter and no reload
 *   (FreeAnim3D); one whose bound is a struct FIELD reloads it every
 *   iteration and counts up (LLIDB_UnLoadODFData).  Both shapes are in this
 *   file two functions apart, which makes the rule cheap to check.
 * ------------------------------------------------------------------------- */

/* ============================================================== geometry == */

typedef struct Pos { int x; int y; } Pos;

/* legoland.h's Rect: four bounds plus the chain link (20 bytes). */
typedef struct Rect {
    int          left;              /* +0x00 */
    int          top;               /* +0x04 */
    int          right;             /* +0x08 */
    int          bottom;            /* +0x0c */
    struct Rect* next;              /* +0x10 */
} Rect;                             /* 0x14 */

/* legoland.h's Cell (20 bytes); only the two fields this file reads or
 * writes are named. */
typedef struct Cell {
    unsigned char  pad00[8];
    unsigned short tile;            /* +0x08 displayed tile */
    unsigned short base;            /* +0x0a */
    unsigned short flags;           /* +0x0c 0x0100 blacked out (no power),
                                     *       0x0200 switched off */
    unsigned char  pad0e[0x14 - 0x0e];
} Cell;                             /* 0x14 */

/* The map header (renderview.c's MapHdr). */
typedef struct MapHdr {
    unsigned char  pad00[0x10];
    unsigned short view_w;          /* +0x10 view rectangle size, pixels */
    unsigned short view_h;          /* +0x12 */
    unsigned short width;           /* +0x14 map size, cells */
    unsigned short height;          /* +0x16 */
    unsigned char  pad18[0x20 - 0x18];
    unsigned short origin_x;        /* +0x20 view rectangle origin, pixels */
    unsigned short origin_y;        /* +0x22 */
} MapHdr;

extern MapHdr* g_map;               /* 0x004bcbf4 */
extern Cell**  g_map_rows;          /* 0x00801400 (GameMap) */

/* =============================================== bigsim.c: IsObjectRunning = */

/* The packed {x,y} byte pair bigsim.c hands over by pointer. */
typedef struct BytePos {
    unsigned char x;                /* +0x00 */
    unsigned char y;                /* +0x01 */
} BytePos;

/* 0 = the park has no power at all (fpui2.c / renderview.c). */
extern int g_power_available;       /* 0x0083298c */

/* objmap.c's bounds-checked cell fetch, inlined here exactly as bigsim.c
 * spells it. */
static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

/* `cls` is dead: the class is not consulted, only the square.  See the header
 * for the null-cell read this reproduces. */
// FUNCTION: LEGOLAND 0x0044f360
int IsObjectRunning(void* cls, BytePos* at)
{
    int            x = at->x;
    int            y = at->y;
#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
    Cell*          c = MapCellAt(x, y);
    unsigned short f = c ? c->flags : 0;   /* QUIRKS.md B: sysmisc3.c:93 -- an off-map square reads address 0xc */
#else
    Cell*          c = MapCellAt(x, y);
    unsigned short f = c->flags;
#endif

    if (f & 0x200)
        return 0;
    if (g_power_available && (f & 0x100))
        return 0;
    return 1;
}

/* ========================================== input2.c: the DirectInput keyboard */

typedef struct DIDevice DIDevice;
typedef struct DIDeviceVtbl {
    long (__stdcall* QueryInterface)(DIDevice*, const void*, void**);       /* +0x00 */
    long (__stdcall* AddRef)(DIDevice*);                                    /* +0x04 */
    long (__stdcall* Release)(DIDevice*);                                   /* +0x08 */
    long (__stdcall* GetCapabilities)(DIDevice*, void*);                    /* +0x0c */
    long (__stdcall* EnumObjects)(DIDevice*, void*, void*, unsigned long);  /* +0x10 */
    long (__stdcall* GetProperty)(DIDevice*, const void*, void*);           /* +0x14 */
    long (__stdcall* SetProperty)(DIDevice*, const void*, const void*);     /* +0x18 */
    long (__stdcall* Acquire)(DIDevice*);                                   /* +0x1c */
    long (__stdcall* Unacquire)(DIDevice*);                                 /* +0x20 */
    long (__stdcall* GetDeviceState)(DIDevice*, unsigned long, void*);      /* +0x24 */
    long (__stdcall* GetDeviceData)(DIDevice*, unsigned long, void*,
                                    unsigned long*, unsigned long);         /* +0x28 */
    long (__stdcall* SetDataFormat)(DIDevice*, const void*);                /* +0x2c */
    long (__stdcall* SetEventNotification)(DIDevice*, void*);               /* +0x30 */
    long (__stdcall* SetCooperativeLevel)(DIDevice*, void*, unsigned long); /* +0x34 */
} DIDeviceVtbl;
struct DIDevice { DIDeviceVtbl* lpVtbl; };

typedef struct DInput DInput;
typedef struct DInputVtbl {
    long (__stdcall* QueryInterface)(DInput*, const void*, void**);         /* +0x00 */
    long (__stdcall* AddRef)(DInput*);                                      /* +0x04 */
    long (__stdcall* Release)(DInput*);                                     /* +0x08 */
    long (__stdcall* CreateDevice)(DInput*, const void*, DIDevice**, void*);/* +0x0c */
    long (__stdcall* EnumDevices)(DInput*, unsigned long, void*, void*,
                                  unsigned long);                           /* +0x10 */
    long (__stdcall* GetDeviceStatus)(DInput*, const void*);                /* +0x14 */
} DInputVtbl;
struct DInput { DInputVtbl* lpVtbl; };

/* DIDEVCAPS, DirectX 3 size (0x2c). */
typedef struct DIDevCaps {
    unsigned long dwSize;           /* +0x00 */
    unsigned long dwFlags;          /* +0x04  DIDC_* -- 0 means "no device" */
    unsigned long dwDevType;        /* +0x08 */
    unsigned long dwAxes;           /* +0x0c */
    unsigned long dwButtons;        /* +0x10 */
    unsigned long dwPOVs;           /* +0x14 */
    unsigned long pad18[5];         /* +0x18..0x2b */
} DIDevCaps;                        /* 0x2c */

extern DInput*   g_dinput;              /* 0x00668d88  IDirectInputA */
extern DIDevice* g_di_keyboard;         /* 0x00668d8c  (input.c) */

/* dinput.lib data, statically linked into the image. */
extern const char GUID_SysKeyboard[16]; /* 0x004ac090 */
extern const char c_dfDIKeyboard[];     /* 0x004ab560  DIDATAFORMAT */

/* Create the DirectInput keyboard.  sysmisc.c's CreateMouseDevice
 * (0x00473970) is the same body plus a SetCooperativeLevel and the wheel
 * granularity query; the keyboard sets neither, so it is left at DirectInput's
 * default (non-exclusive, background) cooperative level -- the reason the game
 * keeps reading keys while another window has focus. */
// FUNCTION: LEGOLAND 0x004738b0
int CreateKeyboardDevice(void)
{
    DIDevCaps caps;

    if (g_dinput->lpVtbl->CreateDevice(g_dinput, GUID_SysKeyboard, &g_di_keyboard, 0))
        goto fail;

    g_di_keyboard->lpVtbl->SetDataFormat(g_di_keyboard, c_dfDIKeyboard);

    caps.dwSize = sizeof(DIDevCaps);
    g_di_keyboard->lpVtbl->GetCapabilities(g_di_keyboard, &caps);
    if (((caps.dwFlags == 0) & 1) == 0)
        return g_dinput->lpVtbl->GetDeviceStatus(g_dinput, GUID_SysKeyboard) == 0;
fail:
    return 0;
}

/* ==================================== memdb.c: the ICM error message box == */

__declspec(dllimport) int __stdcall MessageBoxA(void* hwnd, const char* text,
                                                const char* caption,
                                                unsigned int type);  /* [0x4ab2a4] */

#define MB_ICONEXCLAMATION 0x30

static const char kICMTitle[]    = "LEGOLAND Installation & Configuration Manager";
static const char kICMExists[]   = "An element of the same name already exists.";
static const char kICMWriteICM[] = "LEGOLAND.ICM could not be created or updated.";
static const char kICMNoElem[]   = "The given element could not be found.";
static const char kICMNoKey[]    = "No identification key was given.";
static const char kICMNoFile[]   = "No file name was given.";
static const char kICMCancel[]   = "Operation was canceled.";

/* MISNAMED -- this is LLIDB_ReportICMError(int code); see the file header. */
// FUNCTION: LEGOLAND 0x0047b7b0
void LLIDB_LoadDataByIndex(int code)
{
    switch (code) {
    case -1:
        MessageBoxA(0, kICMExists, kICMTitle, MB_ICONEXCLAMATION);
        break;
    case -2:
        MessageBoxA(0, kICMWriteICM, kICMTitle, MB_ICONEXCLAMATION);
        break;
    case -3:
        MessageBoxA(0, kICMNoElem, kICMTitle, MB_ICONEXCLAMATION);
        break;
    case -4:
        MessageBoxA(0, kICMNoKey, kICMTitle, MB_ICONEXCLAMATION);
        break;
    case -5:
        MessageBoxA(0, kICMNoFile, kICMTitle, MB_ICONEXCLAMATION);
        break;
    case -6:
        MessageBoxA(0, kICMCancel, kICMTitle, MB_ICONEXCLAMATION);
        break;
    }
}

/* ================================================ pathsq.c: PaintPathRect == */

/* pathsq.c's PathRect: a plain four-int rect with no chain link. */
typedef struct PathRect {
    int left;                       /* +0x00 */
    int top;                        /* +0x04 */
    int right;                      /* +0x08 */
    int bottom;                     /* +0x0c */
} PathRect;

/* The base of the path tile run.  pathsq.c declares this `int*` (it reads the
 * whole slot in UpdatePathNeighbours); the paint loop only wants the low
 * half and the original's `add dx,[ebx]` is a 16-bit add, so the caller-side
 * spelling here is `unsigned short*`.  Same object, deliberately different
 * extern -- do not align them. */
extern unsigned short* g_path_tile_base;                 /* 0x00832bf0 */

/* Paint the checkerboard: base + 1 on odd (x + y) squares, base + 2 on even.
 * Both globals are re-read every iteration because the tile store may alias
 * them, which is exactly what a direct global read gives. */
// FUNCTION: LEGOLAND 0x0045cb20
void PaintPathRect(PathRect* r)
{
    int x;
    int y;

    for (y = r->top; y <= r->bottom; y++) {
        for (x = r->left; x <= r->right; x++) {
            int step = ((x + y) & 1) ? 1 : 2;

            g_map_rows[y][x].tile =
                (unsigned short)((step & 0xff) + *g_path_tile_base);
        }
    }
}

/* ======================================= renderview.c: RenderGroundLayer == */

typedef struct WinRect { int left; int top; int right; int bottom; } WinRect;

extern int g_scroll_x;              /* 0x00667cb4  ScrollX, 24.8 */
extern int g_scroll_y;              /* 0x00667cb8  ScrollY, 24.8 */
extern int g_bg_dirty_x;            /* 0x00667cd0 */
extern int g_bg_dirty_y;            /* 0x00667cd4 */
extern int g_bg_full_update;        /* 0x004b9220 */
/* The scroll position the terrain layer was last painted at; the scrolling
 * blitter reads them back to work out how far the layer has to shift. */
extern int g_ground_last_x;         /* 0x004b95e8 */
extern int g_ground_last_y;         /* 0x004b95ec */

/* Paint the terrain/tile grid for one view rectangle (unexported). */
extern void PaintTileLayer(Pos* scroll, WinRect* view);  /* 0x004608c0 */

/* Repaint the whole terrain layer for the current view and clear the three
 * background-dirty flags. */
// FUNCTION: LEGOLAND 0x00460e00
void RenderGroundLayer(void)
{
    Pos     scroll;
    WinRect view;
    int     sx = g_scroll_x >> 8;
    int     sy = g_scroll_y >> 8;

    scroll.x = sx;
    scroll.y = sy;
    view.left = g_map->origin_x;
    view.top = g_map->origin_y;
    view.right = g_map->view_w + view.left;
    view.bottom = g_map->view_h + view.top;
    PaintTileLayer(&scroll, &view);
    g_ground_last_y = sy;
    g_ground_last_x = sx;
    g_bg_dirty_x = 0;
    g_bg_dirty_y = 0;
    g_bg_full_update = 0;
}

/* ================================= memdb.c: the .TSF tile-set unloader ==== */

/* legoland.h's LLIDB element. */
typedef struct LLElem {
    char*        name;              /* +0x00 */
    char*        image;             /* +0x04 */
    unsigned int type_flags;        /* +0x08 */
    void*        data;              /* +0x0c */
    unsigned int refcount;          /* +0x10 */
} LLElem;

/* llidb_load.c's TsfData (36 bytes), with the base slot spelled 16-bit
 * because FreeTileSpace takes it that way. */
typedef struct TsfData {
    unsigned short base_slot;       /* +0x00 AllocTileSpace's index */
    unsigned short pad02;
    int            n_tiles;         /* +0x04 */
    void**         sprites;         /* +0x08 one .lls per tile */
    unsigned int*  codes;           /* +0x0c */
    unsigned int*  second;          /* +0x10 */
    LLElem*        parent;          /* +0x14 the tile set this one extends */
    int            f18, f1c, f20;
} TsfData;

extern void FreeTileSpace(unsigned short base, unsigned short n);  /* 0x0045aa90 */
extern int  KillSprite(void* sprite);                              /* 0x00497bd0 */
extern int  LLIDB_UnLoadData(LLElem* e);                           /* 0x0047d450 */
extern void free(void*);                                           /* 0x0049e4d0 (CRT) */

/* MISNAMED -- this is the .TSF unloader; see the file header.  Declared
 * int-returning and never setting eax, like the rest of the LLIDB_UnLoad*
 * family (memdb.c's LLIDB_UnLoadData tail-returns it).  Note that neither
 * `codes` nor `second` is null-checked and `e->data` is not cleared, both
 * unlike LLIDB_UnLoadLLSData. */
// FUNCTION: LEGOLAND 0x0047cdd0
int LLIDB_UnLoadODFData(LLElem* e)
{
    TsfData* d = (TsfData*)e->data;
    int      i;

    free(d->codes);
    free(d->second);
    for (i = 0; i < d->n_tiles; i++)
        KillSprite(d->sprites[i]);
    FreeTileSpace(d->base_slot, (unsigned short)d->n_tiles);
    if (d->parent)
        LLIDB_UnLoadData(d->parent);
    free(d);
}

/* =========================================== audiomisc.c: KillMusicSystem == */

typedef struct IDMObj IDMObj;
typedef struct IDMObjVtbl {
    long          (__stdcall* QueryInterface)(IDMObj*, const void*, void**); /* +0x00 */
    unsigned long (__stdcall* AddRef)(IDMObj*);                              /* +0x04 */
    unsigned long (__stdcall* Release)(IDMObj*);                             /* +0x08 */
} IDMObjVtbl;
struct IDMObj { IDMObjVtbl* lpVtbl; };

/* IDirectMusicLoader; only ClearCache and Release are called. */
typedef struct IDMLoader IDMLoader;
typedef struct IDMLoaderVtbl {
    long          (__stdcall* QueryInterface)(IDMLoader*, const void*, void**); /* +0x00 */
    unsigned long (__stdcall* AddRef)(IDMLoader*);                              /* +0x04 */
    unsigned long (__stdcall* Release)(IDMLoader*);                             /* +0x08 */
    long          (__stdcall* GetObject)(IDMLoader*, void*, const void*, void**);/* +0x0c */
    long          (__stdcall* SetObject)(IDMLoader*, void*);                    /* +0x10 */
    long          (__stdcall* SetSearchDirectory)(IDMLoader*, const void*,
                                                  unsigned short*, int);        /* +0x14 */
    long          (__stdcall* ScanDirectory)(IDMLoader*, const void*,
                                             unsigned short*, unsigned short*); /* +0x18 */
    long          (__stdcall* CacheObject)(IDMLoader*, IDMObj*);                /* +0x1c */
    long          (__stdcall* ReleaseObject)(IDMLoader*, IDMObj*);              /* +0x20 */
    long          (__stdcall* ClearCache)(IDMLoader*, const void*);             /* +0x24 */
} IDMLoaderVtbl;
struct IDMLoader { IDMLoaderVtbl* lpVtbl; };

/* IDirectMusicPerformance; only Stop, CloseDown and Release are called. */
typedef struct IDMPerformance IDMPerformance;
typedef struct IDMPerformanceVtbl {
    long          (__stdcall* QueryInterface)(IDMPerformance*, const void*, void**);/* +0x00 */
    unsigned long (__stdcall* AddRef)(IDMPerformance*);                             /* +0x04 */
    unsigned long (__stdcall* Release)(IDMPerformance*);                            /* +0x08 */
    long          (__stdcall* Init)(IDMPerformance*, void**, void*, void*);         /* +0x0c */
    long          (__stdcall* PlaySegment)(IDMPerformance*, void*, unsigned long,
                                           __int64, void**);                        /* +0x10 */
    long          (__stdcall* Stop)(IDMPerformance*, void* segment, void* state,
                                    long time, unsigned long flags);                /* +0x14 */
    unsigned char pad18[0x98 - 0x18];
    long          (__stdcall* CloseDown)(IDMPerformance*);                          /* +0x98 */
} IDMPerformanceVtbl;
struct IDMPerformance { IDMPerformanceVtbl* lpVtbl; };

extern int             g_music_sys; /* 0x004bf774  music ENABLED flag (startup.c:255 stores `-nomusic` == 0) */
extern int             g_music_ready;       /* 0x0079a694  music system up */
/* The thread the MIDI/DirectMusic pump runs on; killed outright rather than
 * signalled, which is why the loader cache has to be cleared afterwards. */
extern void*           g_music_thread;      /* 0x0079a698 */
extern IDMLoader*      g_dm_loader;         /* 0x007cacd8  IDirectMusicLoader */
extern IDMPerformance* g_dm_performance;    /* 0x007cacdc  IDirectMusicPerformance */
extern IDMObj*         g_dm_composer;       /* 0x007cad44  IDirectMusicComposer */

/* GUID_DirectMusicAllTypes {d2ac2893-b39b-11d1-8704-00600893b1bd}. */
extern const char GUID_DirectMusicAllTypes[16];   /* 0x004ab8b0 */

__declspec(dllimport) int __stdcall TerminateThread(void* thread,
                                                    unsigned long code); /* [0x4ab0e0] */

/* Tear DirectMusic down.  Always reports success -- audiomisc.c's
 * KillSoundSystem tests the result, so a park with no music at all still
 * counts as a clean shutdown. */
// FUNCTION: LEGOLAND 0x00495b00
int KillMusicSystem(void)
{
    if (g_music_sys && g_music_ready) {
        TerminateThread(g_music_thread, 0);
        g_dm_loader->lpVtbl->ClearCache(g_dm_loader, GUID_DirectMusicAllTypes);
        g_dm_loader->lpVtbl->Release(g_dm_loader);
        g_dm_performance->lpVtbl->Stop(g_dm_performance, 0, 0, 0, 0);
        g_dm_performance->lpVtbl->CloseDown(g_dm_performance);
        g_dm_performance->lpVtbl->Release(g_dm_performance);
        g_dm_composer->lpVtbl->Release(g_dm_composer);
        g_music_ready = 0;
    }
    return 1;
}

/* ====================================================== rin.c: FreeAnim3D == */

/* person3d.c's .3d model records.  One FaceSet is SHARED by every frame
 * (each Frame3D::faces points at the same object), so it is freed once
 * through frame 0 after the per-frame arrays have gone. */
typedef struct FaceSet {
    int  n_faces;                   /* +0x00 */
    int  n_gouraud;                 /* +0x04 */
    int* tris;                      /* +0x08 3 vertex indices per triangle */
} FaceSet;

typedef struct Frame3D {
    unsigned char pad00[0x1c];      /* +0x00 bounding box + vertex count */
    int*          verts;            /* +0x1c 3 ints (16.16) per vertex */
    FaceSet*      faces;            /* +0x20 the shared topology */
    int           n_normals;        /* +0x24 */
    int*          normals;          /* +0x28 3 ints (16.16) per normal */
    int           pad2c[3];         /* +0x2c..+0x37 */
} Frame3D;                          /* 0x38 */

typedef struct Anim3D {
    int      n_frames;              /* +0x00 */
    Frame3D* frames;                /* +0x04 */
    void*    faces;                 /* +0x08 Face3D[], the materials */
    int      pad0c[6];              /* +0x0c..+0x23 */
} Anim3D;

extern void HeapFree_w(void* p);    /* 0x0049e4d0 */

/* Free one morph-target animation.  The frame count is cached in a local:
 * the original reads it once and runs a down-counter, where a re-read of
 * a->n_frames in the condition would reload it after every free. */
// FUNCTION: LEGOLAND 0x0043fde0
void FreeAnim3D(Anim3D* a)
{
    if (a) {
        int n = a->n_frames;
        int i;

        for (i = 0; i < n; i++) {
            HeapFree_w(a->frames[i].verts);
            HeapFree_w(a->frames[i].normals);
        }
        HeapFree_w(a->frames[0].faces->tris);
        HeapFree_w(a->frames[0].faces);
        HeapFree_w(a->frames);
        HeapFree_w(a->faces);
        HeapFree_w(a);
    }
}

/* ============================== objmap2.c: CheckCursorFootprint =========== */

/* The placement / edit cursor (objmap2.c's Cursor). */
typedef struct Cursor {
    unsigned char  pad0000[0x1404];
    Pos            origin;          /* +0x1404 map square under the mouse */
    unsigned char  pad140c[0x1414 - 0x140c];
    Rect           rect;            /* +0x1414 the footprint being previewed */
    unsigned char  pad1428[0x1828 - 0x1428];
    unsigned int   flags;           /* +0x1828 0x1000 = already has a ghost */
    unsigned char  pad182c[4];
    struct Cursor* next;            /* +0x1830 */
} Cursor;                           /* 0x1834 */

/* The one shared spare preview cursor (logflume.c's g_lf_cursor_c,
 * screencb2.c's g_spare_cursor). */
extern Cursor g_spare_cursor;                       /* 0x00830fc0 */

extern void ResetCursorFootprint(Cursor* c);        /* 0x0045f460 */

/* Give the cursor chain a one-cell clearance ghost, unless a link on it
 * already carries flag 0x1000.  Returns the ghost (or the link that had the
 * bit, or 0); castleobj.c and objmap2.c both discard it. */
// FUNCTION: LEGOLAND 0x0045f540
Cursor* CheckCursorFootprint(Cursor* c)
{
    Cursor* p;
    Cursor* last;

    if (!c)
        return 0;

    p = c;
    while (p) {
        last = p;
        if (p->flags & 0x1000)
            return p;
        p = p->next;
    }

    ResetCursorFootprint(&g_spare_cursor);
    g_spare_cursor.origin.x = c->origin.x;
    g_spare_cursor.origin.y = c->origin.y;
    g_spare_cursor.rect.left = c->rect.left - 1;
    g_spare_cursor.rect.top = c->rect.top - 1;
    g_spare_cursor.rect.right = c->rect.right + 1;
    g_spare_cursor.rect.bottom = c->rect.bottom + 1;
    g_spare_cursor.flags = 0x1008;
    g_spare_cursor.next = 0;
    last->next = &g_spare_cursor;
    return &g_spare_cursor;
}

/* ========================== bnvmove.c: SetPathSquareDistance ============== */

/* pathsq.c's path square: a rect of map cells plus the cached squared
 * distance the mover sorts on. */
typedef struct PathSquare {
    struct PathSquare* next;        /* +0x00 */
    int                pad4;        /* +0x04 */
    Rect               rect;        /* +0x08 (cells) */
    int                distance2;   /* +0x1c */
    int                flags;       /* +0x20 */
} PathSquare;                       /* 0x24 */

/* Squared distance in 24.8 world units from `from` to the nearest point of
 * the square's rect, the rect being taken as the CENTRES of its corner cells
 * (cell << 8 plus half a cell). */
// FUNCTION: LEGOLAND 0x00481e60
void SetPathSquareDistance(Pos* from, PathSquare* sq)
{
    int px = from->x;
    int lox = (sq->rect.left << 8) + 0x80;
    int hix = (sq->rect.right << 8) + 0x80;
    int loy = (sq->rect.top << 8) + 0x80;
    int hiy = (sq->rect.bottom << 8) + 0x80;
    int cx;
    int cy;
    int dx;
    int dy;

    if (px < lox)
        cx = lox;
    else if (px > hix)
        cx = hix;
    else
        cx = px;

    if (from->y < loy)
        cy = loy;
    else if (from->y > hiy)
        cy = hiy;
    else
        cy = from->y;

    dx = cx - px;
    dy = cy - from->y;
    sq->distance2 = dx * dx + dy * dy;
}
