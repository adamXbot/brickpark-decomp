/* LEGOLAND — unclustered mid-level routines: the work-order map overlay, the
 * pop-up panel's helpers, and the LLIDB element-picker dialog.
 *
 * Nothing here belongs to a ride class; these are the named functions that
 * sit between subsystems.
 *
 *  0x0047b890  LLIDB_SelectDlgProc  — Win32 DialogProc for template 0x75
 *  0x004720a0  DrawPopUpMock        — the "you have a new object" panel
 *  0x0049ac50  RenderWorkOrders     — one order list's map markers
 *  0x004723f0  PopUpCanDelete       — probe: would the destroy tool accept it?
 *  0x00471610  ClosePopUpIcons      — reset every pop-up icon to its off sprite
 *  0x004718c0  ClampPopUpToScreen   — keep the info pop-up on screen
 *  0x00471840  MeasurePopUpTitle    — width steps the title needs
 *  0x004717a0  MeasurePopUpBody     — width steps the wrapped body needs
 *
 * Cross-file note: bighelp.c owns the PopUpUI block at 0x007fdea4 as one
 * struct; the members this file needs are declared here one at a time (same
 * addresses, same types) because each file keeps its own view.
 */
#include "legoland.h"

/* ------------------------------------------------------- Win32 / commctrl */

__declspec(dllimport) int   __stdcall EndDialog(void* dlg, int result);          /* [0x4ab2f4] */
__declspec(dllimport) long  __stdcall SendMessageA(void* wnd, unsigned int msg,
                                                   unsigned int wp, long lp);    /* [0x4ab2f8] */
__declspec(dllimport) void* __stdcall GetDlgItem(void* dlg, int id);             /* [0x4ab2fc] */

/* The two commctrl structures, spelled out so their sizes are pinned: the
 * frame is exactly one of each (0x20 + 0x28 = 0x48 bytes of locals). */
typedef struct LvColumn {
    unsigned int mask;        /* +0x00 */
    int          fmt;         /* +0x04 */
    int          cx;          /* +0x08 */
    char*        pszText;     /* +0x0c */
    int          cchTextMax;  /* +0x10 */
    int          iSubItem;    /* +0x14 */
    int          iImage;      /* +0x18 */
    int          iOrder;      /* +0x1c */
} LvColumn;                   /* sizeof == 0x20 */

typedef struct LvItem {
    unsigned int mask;        /* +0x00 */
    int          iItem;       /* +0x04 */
    int          iSubItem;    /* +0x08 */
    unsigned int state;       /* +0x0c */
    unsigned int stateMask;   /* +0x10 */
    char*        pszText;     /* +0x14 */
    int          cchTextMax;  /* +0x18 */
    int          iImage;      /* +0x1c */
    long         lParam;      /* +0x20 */
    int          iIndent;     /* +0x24 */
} LvItem;                     /* sizeof == 0x28 */

#define LVM_GETITEMA      0x1005
#define LVM_SETITEMA      0x1006
#define LVM_INSERTITEMA   0x1007
#define LVM_GETNEXTITEM   0x100c
#define LVM_INSERTCOLUMNA 0x101b

extern int LLIDB_GetElement(unsigned int idx, LLElem** out);   /* 0x0047b2e0 */

extern int          g_llidb_select_filter;   /* 0x007fdb88 */
extern unsigned int g_llidb_select_index;    /* 0x007fdca0 */

/* Modal element picker (memdb.c's LLIDB_SelectElement runs it on template
 * 0x75).  The dialog holds one list-view control, id 0x441, with three
 * 180-pixel columns: ID (the element's name), File (its asset filename) and
 * Type (the human name of its type_flags & 0xfff0).
 *
 * WM_INITDIALOG fills the list with every element the caller's filter mask
 * selects (g_llidb_select_filter & elem->type_flags), stashing the element's
 * DATABASE index in the row's lParam so the row order need not match it.
 * WM_COMMAND: IDOK reads the focused/selected row back, publishes its lParam
 * in g_llidb_select_index and ends with 1; IDCANCEL ends with 0; control
 * 0x29a ends with -1 (LLIDB_SelectElement maps that to its -1 "dialog
 * failed" return).
 *
 * Original quirk: on a type value the switch does not name, lvi.pszText is
 * left holding the *previous* subitem's string, so such a row's Type column
 * repeats its File column.  Reproduced.
 */
// FUNCTION: LEGOLAND 0x0047b890
int __stdcall LLIDB_SelectDlgProc(void* dlg, unsigned int msg, unsigned int wp, long lp)
{
    LvColumn col;
    LvItem   lvi;
    int      row = 0;

    switch (msg) {
    case 0x110: /* WM_INITDIALOG */
        {
            void*        list;
            unsigned int i;
            LLElem*      elem;

            list = GetDlgItem(dlg, 0x441);
            col.mask = 6;                    /* LVCF_WIDTH | LVCF_TEXT */
            col.pszText = "ID";
            col.cx = 180;
            SendMessageA(list, LVM_INSERTCOLUMNA, 0, (long)&col);
            col.mask = 6;
            col.pszText = "File";
            col.cx = 180;
            SendMessageA(list, LVM_INSERTCOLUMNA, 1, (long)&col);
            col.mask = 6;
            col.pszText = "Type";
            col.cx = 180;
            SendMessageA(list, LVM_INSERTCOLUMNA, 2, (long)&col);

            list = GetDlgItem(dlg, 0x441);
            for (i = 0; i < g_llidb_count; i++) {
                LLIDB_GetElement(i, &elem);
                if (elem && (g_llidb_select_filter & elem->type_flags)) {
                    lvi.mask = 5;            /* LVIF_TEXT | LVIF_PARAM */
                    lvi.iItem = row;
                    lvi.iSubItem = 0;
                    lvi.pszText = elem->name;
                    lvi.lParam = i;
                    SendMessageA(list, LVM_INSERTITEMA, 0, (long)&lvi);

                    lvi.mask = 1;            /* LVIF_TEXT */
                    lvi.iItem = row;
                    lvi.iSubItem = 1;
                    lvi.pszText = elem->image;
                    SendMessageA(list, LVM_SETITEMA, 0, (long)&lvi);

                    lvi.mask = 1;
                    lvi.iItem = row;
                    lvi.iSubItem = 2;
                    switch (elem->type_flags & 0xfff0) {
                    case 0x10:   lvi.pszText = "Object Description"; break;
                    case 0x20:   lvi.pszText = "Tile Mapping"; break;
                    case 0x40:   lvi.pszText = "Tile Set"; break;
                    case 0x80:   lvi.pszText = "Level Structure"; break;
                    case 0x100:  lvi.pszText = "Terrain Map"; break;
                    case 0x200:  lvi.pszText = "Game Defined Stub"; break;
                    case 0x400:  lvi.pszText = "Image List"; break;
                    case 0x800:  lvi.pszText = "Configuration String"; break;
                    case 0x1010: lvi.pszText = "Object Control Description"; break;
                    }
                    SendMessageA(list, LVM_SETITEMA, 0, (long)&lvi);
                    row++;
                }
            }
        }
        return 1;

    case 0x111: /* WM_COMMAND */
        switch (wp & 0xffff) {
        case 1: /* IDOK */
            {
                void* list = GetDlgItem(dlg, 0x441);
                int   sel  = SendMessageA(list, LVM_GETNEXTITEM, (unsigned int)-1, 2);

                g_llidb_select_index = sel;
                if (sel == -1) {
                    EndDialog(dlg, 0);
                    return 0;
                }
                lvi.mask = 4;                /* LVIF_PARAM */
                lvi.iItem = sel;
                lvi.iSubItem = 0;
                SendMessageA(list, LVM_GETITEMA, 0, (long)&lvi);
                g_llidb_select_index = lvi.lParam;
                EndDialog(dlg, 1);
            }
            break;
        case 2: /* IDCANCEL */
            EndDialog(dlg, 0);
            return 0;
        case 0x29a:
            EndDialog(dlg, -1);
            return 0;
        }
        break;
    }
    return 0;
}

/* ==================================================== the pop-up MOCK panel */

/* iconui.c's icon record (only the placement/flags fields are used here). */
typedef struct Icon {
    struct Icon*  next;       /* +0x00 */
    Sprite*       sprite;     /* +0x04 */
    void*         data;       /* +0x08 */
    short         x;          /* +0x0c */
    short         y;          /* +0x0e */
    short         w;          /* +0x10 */
    short         h;          /* +0x12 */
    char          pad14[0x34 - 0x14];
    unsigned int  flags;      /* +0x34  0x400 = hidden */
    char          pad38[0x40 - 0x38];
} Icon;

/* The 5th PrintSprite argument (panelui.c BlitCtx); kind 1 here. */
typedef struct BlitCtx {
    int kind;                          /* +0x00 */
    struct { void* p; int n; } sub;    /* +0x04, +0x08 */
} BlitCtx;

/* An object-definition (ODF) record as the mock panel reads it. */
typedef struct ObjDef {
    char   pad0[0x26];
    short  cost;              /* +0x26 purchase price */
    char   pad28[0x78 - 0x28];
    char*  name;              /* +0x78 display name */
    char   pad7c[0x80 - 0x7c];
    char*  desc;              /* +0x80 long description */
} ObjDef;

/* bighelp.c's PopUpUI block, viewed field by field (it owns the whole
 * struct at 0x007fdea4; these are the members the mock panel touches). */
extern Icon*   g_pu_icon_close;      /* 0x007fdfc0  g_popup.icon_close */
extern Icon*   g_pu_icon_next;       /* 0x007fdfc4  g_popup.icon_next  */
extern Icon*   g_pu_icon_prev;       /* 0x007fdfe8  g_popup.icon_prev  */

/* The "new object unlocked" carousel, inside the same block: up to 20
 * entries, one ObjDef and one preview sprite each. */
extern ObjDef* g_mock_defs[20];      /* 0x007fded4 */
extern Sprite* g_mock_sprites[20];   /* 0x007fdf24 */
extern int     g_mock_count;         /* 0x007fdf74 */
extern int     g_mock_index;         /* 0x007fdf78 */

extern Sprite* g_pu_mock;            /* 0x00668910 */
extern Sprite* g_pu_close;           /* 0x00668920 */
extern Sprite* g_pu_next;            /* 0x00668928 */
extern Sprite* g_pu_prev;            /* 0x00668930 */

extern Pos     g_input_point;        /* 0x00813a44  g_input.point */

extern int  PrintSprite(Sprite* s, int x, int y, int mode, BlitCtx* ctx);   /* 0x004853a0 */
extern void PushRenderingStatusAndUnlockVideoSurface(void);                 /* 0x00464080 */
extern void PopRenderingStatus(void);                                       /* 0x004641f0 */
extern int  Format(char* dst, const char* fmt, ...);                        /* 0x0049e573 */
extern void PrintCachedText(const char* text, int x, int y, int w, int h,
                            int f1, int f2, int ink, int paper);            /* 0x00455e50 */
extern void SetIconSprite(Icon* p, Sprite* s);                              /* 0x0046d680 */

extern void* memset(void* d, int c, unsigned int n);
#pragma intrinsic(memset)

/* Un-hide an icon and pin it to a position.  The two coordinates are
 * evaluated as ints BEFORE the flag store, which is what keeps the sprite
 * sizes 32-bit (movsx) instead of narrowing to 16-bit arithmetic. */
static __inline void PlaceIcon(Icon** pp, int y, int x)
{
    (*pp)->flags &= ~0x400;
    (*pp)->x = (short)x;
    (*pp)->y = (short)y;
}

/* The cursor-over-icon test, unsigned so one compare covers both edges. */
static __inline int MouseOverIcon(Icon* p)
{
    return (unsigned int)(g_input_point.x - p->x) < (unsigned int)p->w &&
           (unsigned int)(g_input_point.y - p->y) < (unsigned int)p->h;
}

/* -------------------------------------------------------------------------
 * 0x004720a0 -- the "you have a new object" panel.
 *
 * A second, much simpler pop-up than DrawPopUpInfo: it is what
 * g_popup.active == 2 draws.  The panel background sprite g_pu_mock is
 * blitted at (0xbe, 0x28) as an icon-owned blit (BlitCtx.kind 1) and the
 * three navigation icons are un-hidden and pinned to it -- prev at a fixed
 * (0xc1, 0x46), next and close flush with the panel's right edge
 * (0xbb + mock->w - close->w), close also flush with its bottom
 * (0x25 + mock->h - close->h).  Note that NEXT is positioned with the CLOSE
 * icon's width, not its own; harmless while the two sprites are the same
 * size, reproduced as-is.
 *
 * Between Push/PopRenderingStatus it prints four strings, all in blue
 * (0xff0000) on white (0xffffff), against the panel's right edge
 * (0xbb + mock->w):
 *   (0xc1, 0x30)  "You have a new object" / "You have %d new objects"
 *   (0xc1, 0x4b)  the current entry's ObjDef name
 *   (0x13e,0x68)  its long description (ObjDef +0x80), 0xfc x 0x77
 *   (0xf0, 0xd1)  its price (ObjDef +0x26) as "%d", 0x43 x 0x12
 * then blits the entry's preview sprite at (0xc4, 0x64).
 *
 * RESIDUAL (2 instructions of 240).  The original computes each of the first
 * two text widths as TWO adds -- `add eax,0BBh` then `add eax,0FFFFFF3Fh`
 * (-0C1h) -- where VC6 folds `g_pu_mock->w + 0xbb - 0xc1` to a single
 * `sub reg,6`.  Everything else in the body matches index for index.  VC6 SP3
 * only leaves the pair unfolded when the `+ 0xbb` node has MORE THAN ONE
 * consumer and the extra consumer comes FIRST (measured: a store to a global
 * or a call taking the value before the subtraction gives `add/add`; a
 * consumer AFTER gives `add` + `lea [r-193]`; a single consumer always folds).
 * Ruled out this round, all still folding to `sub reg,6`: every spelling of
 * the expression (operand order, extra parentheses, `+ -0xc1`, `- 0xc0 - 1`,
 * `~0xc0`, `-(0xc1 - R)`, three-constant chains such as `w + 0xbe - 3 - 0xc1`
 * and `- (0xbe + 3)`); routing either constant through a local (single or
 * repeated assignment, `register`, across the Format if/else join, as an
 * `enum`), which VC6 constant-propagates before it folds; a `static const int`
 * (that one LOADS the constant from memory instead); four static __inline
 * shapes (a right-edge accessor, a `Sub(a,b)`, an `Add(a,b)`, and a
 * `PrintBox(s,x,y,x2,y2,f)` whose x is used twice), including one whose local
 * holds the edge and is expanded at both call sites; narrowing to `short` (16-
 * bit arithmetic plus a movsx) and every parameter type for the width slot
 * (short / unsigned / long / unsigned char).  So the missing ingredient is a
 * SECOND consumer of `g_pu_mock->w + 0xbb`, evaluated before the width, that
 * costs no instruction -- and nothing in the original's instruction stream is
 * one.  Next thing to try: whether the real prototype of PrintCachedText
 * passes the panel edge somewhere else too (a 10th argument would change the
 * `add esp,48h`, so it is not that), or whether this file used a geometry
 * accessor that also updated a global the disassembly attributes elsewhere.
 *
 * Finally each of the three icons whose sprite the cursor is NOT over is
 * reset to its "off" sprite (the hover sprite is set by the icon's own
 * input handler), and the carousel ends are greyed out: prev is hidden on
 * the first entry, next on the last.
 * ------------------------------------------------------------------------- */
// WIP-FUNCTION: LEGOLAND 0x004720a0  (238 of the original's 240 instructions,
//   832B vs 842B; the ONLY residual is the two text widths -- see the note above)
void DrawPopUpMock(void)
{
    BlitCtx ctx;
    char    text[0x80];

    ctx.kind = 1;
    memset(&ctx.sub, 0, sizeof(ctx.sub));
    PrintSprite(g_pu_mock, 0xbe, 0x28, 0, &ctx);

    PlaceIcon(&g_pu_icon_prev, 0x46, 0xc1);
    PlaceIcon(&g_pu_icon_next, 0x46, g_pu_mock->w - g_pu_icon_close->w + 0xbb);
    PlaceIcon(&g_pu_icon_close, g_pu_mock->h - g_pu_icon_close->h + 0x25,
              g_pu_mock->w - g_pu_icon_close->w + 0xbb);

    PushRenderingStatusAndUnlockVideoSurface();
    if (g_mock_count == 1)
        Format(text, "You have a new object");
    else
        Format(text, "You have %d new objects", g_mock_count);

    PrintCachedText(text, 0xc1, 0x30, g_pu_mock->w + 0xbb - 0xc1, 0x14,
                    2, 5, 0xff0000, 0xffffff);
    PrintCachedText(g_mock_defs[g_mock_index]->name, 0xc1, 0x4b,
                    g_pu_mock->w + 0xbb - 0xc1, 0x14, 2, 5, 0xff0000, 0xffffff);
    PrintCachedText(g_mock_defs[g_mock_index]->desc, 0x13e, 0x68, 0xfc, 0x77,
                    2, 0x10, 0xff0000, 0xffffff);
    Format(text, "%d", g_mock_defs[g_mock_index]->cost);
    PrintCachedText(text, 0xf0, 0xd1, 0x43, 0x12, 2, 1, 0xff0000, 0xffffff);
    PopRenderingStatus();

    PrintSprite(g_mock_sprites[g_mock_index], 0xc4, 0x64, 0, 0);

    if (!MouseOverIcon(g_pu_icon_close))
        SetIconSprite(g_pu_icon_close, g_pu_close);
    if (!MouseOverIcon(g_pu_icon_next))
        SetIconSprite(g_pu_icon_next, g_pu_next);
    if (!MouseOverIcon(g_pu_icon_prev))
        SetIconSprite(g_pu_icon_prev, g_pu_prev);

    if (g_mock_index == 0)
        g_pu_icon_prev->flags |= 0x400;
    else
        g_pu_icon_prev->flags &= ~0x400;

    if (g_mock_index == g_mock_count - 1)
        g_pu_icon_next->flags |= 0x400;
    else
        g_pu_icon_next->flags &= ~0x400;
}

/* ================================================== work-order map overlay */

typedef struct TileBounds {
    int left;   /* +0x00 */
    int top;    /* +0x04 */
    int right;  /* +0x08 */
    int bottom; /* +0x0c */
} TileBounds;

/* An object class record as the overlay reads it: the class's own preview
 * sprite hangs at +0x68 (the same record popup.c calls ObjDef). */
typedef struct WClass {
    char     pad0[0x68];
    Sprite*  sprite;        /* +0x68 */
} WClass;

/* The LLIDB element a placed object is known by; its class is at +0x0c. */
typedef struct ObjElem {
    char    pad0[0x0c];
    WClass* cls;            /* +0x0c */
} ObjElem;

/* A gardener / mechanic work order (workers.c's WorkOrder, calloc(0x3c,1)),
 * with the one further field the overlay reads. */
typedef struct WorkOrder {
    struct WorkOrder* next;     /* +0x00 */
    ObjElem*          obj;      /* +0x04 */
    Pos               pos;      /* +0x08 map cell the rects are biased by */
    Rect*             rects;    /* +0x10 */
    int               nrects;   /* +0x14 */
    int               assigned; /* +0x18  1 once a worker has taken it */
    char              pad1c[0x30 - 0x1c];
    int               waiting;  /* +0x30  mechanic order not yet payable */
    char              pad34[0x3c - 0x34];
} WorkOrder;

/* A looping-sprite record: the LLS animation state hangs at +0x08. */
typedef struct WSprite {
    int    pad0;            /* +0x00 */
    int    pad4;            /* +0x04 */
    void** lls_holder;      /* +0x08 */
    char   pad0c[0x16 - 0x0c];
    short  h;               /* +0x16 */
} WSprite;

extern WorkOrder* g_gardener_orders;      /* 0x0079a8b0 */
extern WorkOrder* g_mechanic_orders;      /* 0x0079a8c0 */
extern Elem*      g_worker_path_tiles;    /* 0x0079abfc  ElemID("NORMAL PATH TILES") */
extern int*       g_basic_tiles_data;     /* 0x00801a6c  +0 = base tile slot */
extern int        g_tileset_id3;          /* 0x00805f48  the "blocked" cursor tile */
extern WSprite*   g_paying_sprite;        /* 0x007fe004 */
extern WSprite*   g_nomoney_sprite;       /* 0x007fdeb0 */

extern void GetTileBounds(Pos* tile, TileBounds* out);   /* 0x0045acc0 */
extern void LLSNextFrame(void* lls);                     /* 0x0047d5d0 */

/* bigrender.c's tile-id resolution, verbatim. */
static __inline Sprite* TileSprite(int id)
{
    return g_tile_sprites[(id & 0xff) + *g_basic_tiles_data];
}

/* The same, against the "NORMAL PATH TILES" element's own base slot. */
static __inline Sprite* PathSprite(int* base, int id)
{
    return g_tile_sprites[(id & 0xff) + *base];
}

/* -------------------------------------------------------------------------
 * 0x0049ac50 -- draw one work-order list's map markers.
 *
 * workers.c's RenderWorkerInterfaceGFX calls this with 0 (gardeners, green
 * 0x40e040) and 1 (mechanics, red 0xe04040).
 *
 * For every order in the list, and for every rect of its footprint (the rect
 * array at +0x10, biased by the order's map cell at +0x08):
 *
 *  1. EVERY cell of the rect gets the "blocked cursor" tile
 *     (TileSprite(g_tileset_id3)) tinted with the list colour once a worker
 *     has taken the order (+0x18) and with tint 1 while it is unassigned.
 *
 *  2. ONLY THE FIRST RECT (i == 0) gets the rest:
 *     a. a second pass laying a PATH tile over each cell, picked by which
 *        edges of the rect the cell sits on:
 *            code = (((x != x0) << 2 | (x != x1)) << 1)
 *                 |  ((y != y1) << 2 | (y != y0))
 *        so bit3 = "not the left column", bit2 = "not the bottom row",
 *        bit1 = "not the right column", bit0 = "not the top row" -- i.e. the
 *        four connection directions of the path artwork -- and the tile is
 *        that code + 3 within the "NORMAL PATH TILES" element's run.  The
 *        tint carries 0x1f1f80 (a blend), OR'd with the list colour when the
 *        order is assigned and with 1 when it is not.
 *     b. the CLASS PREVIEW sprite (class +0x68) centred in the rect, but
 *        only when the rect is wider on screen than twice the sprite -- the
 *        screen extent comes from four GetTileBounds probes of the rect's
 *        corners, exactly as workers2.c's IterateNoneWorkersRepairOrders
 *        does it: (x0,y1) gives the left, (x1,y0) the right, (x0,y0) the top
 *        and (x1,y1) the bottom.
 *     c. for MECHANIC orders only, a status sprite at that same centre
 *        shifted 10px right: the animated "no money" sprite (its LLS frame
 *        advanced) while the order is still waiting (+0x30), otherwise the
 *        "paying" sprite.  Note the vertical centring uses the NO-MONEY
 *        sprite's height in both cases, and that both the corner probes and
 *        the centre are computed even for a gardener list, where nothing is
 *        drawn with them.  Reproduced.
 * ------------------------------------------------------------------------- */
/* RESIDUAL (one instruction of 354).  Our body is 355 instructions to the
 * original's 354 and matches it index for index everywhere EXCEPT the rect
 * loop's landing pad: the original reloads only `i` there
 *     0049acbc  mov eax,[esp+18h]        ; i
 *     0049acc0  mov ecx,[edx+10h]        ; o->rects   -- edx still holds `o`
 * because VC6 forwards the `mov edx,[esp+10h]` the loop BOTTOM already did
 * for `i < o->nrects` across the back edge, while the pre-header jumps
 * straight to 0049acc0.  Ours reloads `o` as well (`mov edx,[esp+10h]`
 * between the two), so the pre-header jumps one instruction further and the
 * whole body shifts by one; nothing else differs and the frame is identical
 * slot for slot (0x50 bytes: o -50, l1 -4c, i -48, colour -44, r2 -40,
 * path -3c, cls -38, sw/t3 -34, sh -30, (int)sw spill -2c, pt -28, b -20,
 * r -10, with r.left/r.right never leaving ebp/edi so r's first and third
 * words are allocated but never written).
 *
 * Measured and identical (all 355, pad unchanged): every loop spelling for
 * both loops (for / while / do-while / `continue` instead of `if (i == 0)`,
 * `++i`, `!i`, `o->nrects > i`, `(n = o->nrects)`); `&o->rects[i]` vs
 * `o->rects + i` vs the index expression written out four times; splitting
 * the four `r.*` assignments into loads then `+=`; a `Pos op = o->pos` copy;
 * a second `WorkOrder* w = o` for the body (356); a `mech` copy of the
 * parameter (356); swapping the assigned/unassigned PrintSprite arms (345);
 * declaring `path` separately, later, or as `ElemData*`; declaring l1/r2/t3/
 * cx/cy/sw/sh block-scope inside the `i == 0` arm; function-level xc/yc;
 * `3 + (xc|yc)`; folding yc into xc (351); and inlining both TileSprite and
 * PathSprite by hand.
 *
 * WHAT IT IS: pure register-allocation pressure, and it is measurable --
 * making the path-tile base a fresh global read inside PathSprite (so `path`
 * stops being a memory local live across the loop) drops us to exactly 354
 * instructions with the pad reloading only `i`, and deleting the whole
 * `i == 0` arm puts `o` in edi and spills it nowhere at all.  So the five
 * memory-resident values live across the rect loop (o, i, colour, path, cls)
 * are one too many for VC6 to forward the bottom's load, and the original
 * evidently gave the allocator one fewer live value somewhere inside the
 * `i == 0` arm.  Next thing to try: a shape for that arm that needs one
 * fewer simultaneously-live temporary -- the four corner probes are the
 * obvious candidate (l1/r2/t3 plus the sign-extended sprite size).
 */
// WIP-FUNCTION: LEGOLAND 0x0049ac50  (354 of the original's 354 instructions
//   reproduced, 355 emitted: one extra `mov edx,[esp+10h]` reload at the rect
//   loop's landing pad -- see the note above; everything else is exact)
void RenderWorkOrders(int mechanic)
{
    int*       path = (int*)g_worker_path_tiles->data;
    WorkOrder* o;
    int        colour;
    WClass*    cls;
    int        i;
    Pos        pt;
    TileBounds b;
    TileBounds r;
    int        l1, r2, t3;
    int        cx, cy;
    short      sw, sh;

    if (mechanic) {
        o = g_mechanic_orders;
        colour = 0xe04040;
    } else {
        o = g_gardener_orders;
        colour = 0x40e040;
    }
    while (o) {
        cls = o->obj->cls;
        for (i = 0; i < o->nrects; i++) {
            Rect* rc = &o->rects[i];

            r.left = rc->left + o->pos.x;
            r.top = rc->top + o->pos.y;
            r.bottom = rc->bottom + o->pos.y;
            r.right = rc->right + o->pos.x;

            for (pt.y = r.top; pt.y <= r.bottom; pt.y++) {
                for (pt.x = r.left; pt.x <= r.right; pt.x++) {
                    GetTileBounds(&pt, &b);
                    if (o->assigned)
                        PrintSprite(TileSprite(g_tileset_id3), b.left, b.top, colour, 0);
                    else
                        PrintSprite(TileSprite(g_tileset_id3), b.left, b.top, 1, 0);
                }
            }
            if (i == 0) {
                for (pt.y = r.top; pt.y <= r.bottom; pt.y++) {
                    int yc = ((pt.y != r.bottom) << 2) | (pt.y != r.top);
                    for (pt.x = r.left; pt.x <= r.right; pt.x++) {
                        int xc = (((pt.x != r.left) << 2) | (pt.x != r.right)) << 1;
                        GetTileBounds(&pt, &b);
                        if (o->assigned)
                            PrintSprite(PathSprite(path, (xc | yc) + 3), b.left, b.top,
                                        colour | 0x1f1f80, 0);
                        else
                            PrintSprite(PathSprite(path, (xc | yc) + 3), b.left, b.top,
                                        0x1f1f81, 0);
                    }
                }
                sw = cls->sprite->w;
                sh = cls->sprite->h;
                pt.x = r.left;
                pt.y = r.bottom;
                GetTileBounds(&pt, &b);
                l1 = b.left;
                pt.x = r.right;
                pt.y = r.top;
                GetTileBounds(&pt, &b);
                r2 = b.right;
                if (r2 - l1 > 2 * sw) {
                    pt.x = r.left;
                    pt.y = r.top;
                    GetTileBounds(&pt, &b);
                    t3 = b.top;
                    pt.x = r.right;
                    pt.y = r.bottom;
                    GetTileBounds(&pt, &b);
                    PrintSprite(cls->sprite, (r2 - sw + l1) / 2,
                                (b.bottom - sh + t3) / 2, 0, 0);
                }
                pt.x = r.left;
                pt.y = r.bottom;
                GetTileBounds(&pt, &b);
                l1 = b.left;
                pt.x = r.right;
                pt.y = r.top;
                GetTileBounds(&pt, &b);
                r2 = b.right;
                pt.x = r.left;
                pt.y = r.top;
                GetTileBounds(&pt, &b);
                t3 = b.top;
                pt.x = r.right;
                pt.y = r.bottom;
                GetTileBounds(&pt, &b);
                cx = (r2 + l1) / 2 + 10;
                cy = (b.bottom - g_nomoney_sprite->h + t3) / 2;
                if (mechanic) {
                    if (o->waiting) {
                        LLSNextFrame(*g_nomoney_sprite->lls_holder);
                        PrintSprite((Sprite*)g_nomoney_sprite, cx, cy, 0, 0);
                    } else {
                        PrintSprite((Sprite*)g_paying_sprite, cx, cy, 0, 0);
                    }
                }
            }
        }
        o = o->next;
    }
}

/* ============================================ "is this object deletable?" */

/* The edit/destroy cursor block, 0x1834 bytes (objmap2.c's Cursor has the
 * full layout; only its SIZE matters here, because the whole block is saved
 * and restored around the probe). */
typedef struct QueryCursor {
    char raw[0x1834];
} QueryCursor;

/* The object class record with the slot the probe drives (the object
 * definition's +0x94 "update2" hook and its context pointer at +0xc4). */
typedef struct QDef {
    char  pad0[0x94];
    void  (*update2)(void* ctx, Pos* cell);   /* +0x94 */
    char  pad98[0xc4 - 0x98];
    void* ctx;                                /* +0xc4 */
} QDef;

/* A placed map object: its class sits at +0x0c. */
typedef struct QObj {
    char  pad0[0x0c];
    QDef* cls;                                /* +0x0c */
} QObj;

/* castleobj.c's query block: the cell the +0x94 hook is asked about. */
typedef struct QueryBlock {
    int x;   /* +0x00  0x00811564 */
    int y;   /* +0x04  0x00811568 */
} QueryBlock;

extern QObj*        g_popup_obj;      /* 0x007fdec4  PopUpInfo.obj  */
extern int          g_popup_ref;      /* 0x007fdec8  PopUpInfo.ref (packed cell) */
extern QDef*        g_query_class;    /* 0x00667c58  castleobj.c QueryClass */
extern QueryCursor  g_query_cursor;   /* 0x00810160  castleobj.c QueryCursor
                                       *             (objmap2.c g_destroy_cursor) */
extern QueryBlock   g_query_block;    /* 0x00811564 */

extern void BuildCursorPtr(QueryCursor* c, int a, int b);   /* 0x0045f5f0 */
extern int  CursorIsValid(QueryCursor* c);                  /* 0x0045f4b0 */

/* -------------------------------------------------------------------------
 * 0x004723f0 -- can the object the pop-up is describing be demolished?
 *
 * popup.c calls this to decide whether the pop-up shows its delete icon.
 * The answer is produced by RUNNING THE REAL DEMOLITION CURSOR over the
 * object and asking whether the result validates, so the pop-up's icon is
 * exactly as strict as the destroy tool itself.
 *
 * Because that machinery is global state, the whole 0x1834-byte query cursor
 * and the query class pointer are saved into locals first and put back
 * afterwards -- this is a PROBE with no side effects.  In between:
 *   - the pop-up's packed cell reference (PopUpInfo.ref, {x = low byte,
 *     y = high byte}) is unpacked into the query block at 0x00811564;
 *   - the pop-up object's class becomes QueryClass;
 *   - the class's object-definition "update2" hook (+0x94) is called with
 *     its context (+0xc4) and the unpacked cell, which is what refills the
 *     query cursor's footprint (castleobj.c's Castle_Update2 is one such
 *     hook);
 *   - BuildCursorPtr turns that footprint into the outline point list and
 *     CursorIsValid grades it.
 * The grade is returned untouched -- eax survives both `rep movsd` and the
 * pops, which is why the function needs no `mov eax` of its own.
 * ------------------------------------------------------------------------- */
// FUNCTION: LEGOLAND 0x004723f0
int PopUpCanDelete(void)
{
    Pos          p;
    QueryCursor  saved;
    QDef*        prev;
    QDef*        def;
    int          ok;
    unsigned int ref;

    prev = g_query_class;
    ref = g_popup_ref & 0xffff;
    saved = g_query_cursor;
    p.x = ref & 0xff;
    p.y = ref >> 8;
    def = g_popup_obj->cls;
    g_query_class = def;
    g_query_block.x = p.x;
    g_query_block.y = p.y;
    def->update2(def->ctx, &p);
    BuildCursorPtr(&g_query_cursor, 0, 0);
    ok = CursorIsValid(&g_query_cursor);
    g_query_cursor = saved;
    g_query_class = prev;
    return ok;
}

/* =================================================== pop-up icon housekeeping */

extern Icon*   g_pu_icon_mech;       /* 0x007fdea4  g_popup.icon_mech     */
extern Icon*   g_pu_icon_delete2;    /* 0x007fdfcc  g_popup.icon_delete2  */
extern Icon*   g_pu_icon_delete;     /* 0x007fdfdc  g_popup.icon_delete   */
extern Icon*   g_pu_icon_gardener;   /* 0x007fdfe0  g_popup.icon_gardener */

extern Sprite* g_pu_delete;          /* 0x00668918 */
extern Sprite* g_pu_gardener;        /* 0x00668948 */
extern Sprite* g_pu_mech;            /* 0x00668950 */

/* -------------------------------------------------------------------------
 * 0x00471610 -- put every pop-up icon back to its UNLIT sprite.
 *
 * Despite the name popup.c gives it, this closes nothing: it is the tail of
 * the close path and only undoes the hover highlighting, calling
 * SetIconSprite once per icon with the icon's "off" artwork.  The icons are
 * hidden by their own `flags |= 0x400`, not here.  Order: delete, prev,
 * next, close, gardener, mechanic, delete-confirm -- and note that the
 * delete-confirm icon (icon_delete2) is reset to the SAME g_pu_delete sprite
 * as the plain delete icon, so the two share their off state.
 * ------------------------------------------------------------------------- */
// FUNCTION: LEGOLAND 0x00471610
void ClosePopUpIcons(void)
{
    SetIconSprite(g_pu_icon_delete, g_pu_delete);
    SetIconSprite(g_pu_icon_prev, g_pu_prev);
    SetIconSprite(g_pu_icon_next, g_pu_next);
    SetIconSprite(g_pu_icon_close, g_pu_close);
    SetIconSprite(g_pu_icon_gardener, g_pu_gardener);
    SetIconSprite(g_pu_icon_mech, g_pu_mech);
    SetIconSprite(g_pu_icon_delete2, g_pu_delete);
}

/* ================================================= pop-up panel placement */

/* bigscreens.c's side-panel state; only its first byte is read here. */
typedef struct PanelState {
    char f00;    /* +0x00  0x007fdd80  2 = the side panel is stowed */
    int  f04;
    int  f08;
    char f0c;
} PanelState;
extern PanelState g_panel_state;   /* 0x007fdd80 */

extern int g_popup_x;              /* 0x007fdecc  PopUpInfo.pos.x */
extern int g_popup_y;              /* 0x007fded0  PopUpInfo.pos.y */

/* -------------------------------------------------------------------------
 * 0x004718c0 -- keep the info pop-up on screen.
 *
 * `size` is the panel height in "lines"; the panel is size*0x20 + 0xc8 wide
 * and size*20 + 0x96 tall (the same two formulas popup.c's DrawPopUpInfo
 * lays its text out with).  The panel is anchored at g_popup.pos, which this
 * moves in place:
 *   x  if the panel would run past 0x27b, FLIP it to the other side of the
 *      anchor (x -= width + 5); if that puts it off the left edge, pin it at
 *      x = 5.  Then, unless the side panel is stowed (g_panel_state.f00 == 2),
 *      x is pushed right to 0x82 so the panel never covers the side bar --
 *      note the x = 5 case falls straight into that test with the 0x82
 *      comparison ELIDED, because VC6 knows 5 < 0x82.
 *   y  clamped to [0x25, 0x16f - height].
 * ------------------------------------------------------------------------- */
/* RESIDUAL (one instruction of 43).  42 emitted, index-for-index identical
 * up to the y clamp; the original materialises the low bound in a register
 * before the store
 *     0047191e  mov eax,25h
 *     ...       mov [7fdeccH],ecx  /  mov [7fded0H],eax
 * where every spelling tried makes VC6 store the immediate directly
 * (`mov dword ptr [7fded0H],25h`, one instruction).  Note the two forms are
 * the SAME LENGTH (5+5 vs 10 bytes) because an absolute store from eax uses
 * the short A3 opcode, so this is not a size choice.  Measured and still
 * folding: the constant routed through the shared `limit` variable, through
 * `y` itself, through a `limit` that is also the compare operand, as an
 * `int limit = 0x25` initialiser, via a Pos struct for the two globals, with
 * the store pair before/after the branch, with the arms swapped, with a
 * dead symmetric `if (y > limit) g_popup_y = y;` after it (VC6 range-folds
 * that away and still stores the immediate), and as a phi joined after the
 * if/else (41 instructions -- VC6 then shares one store pair instead of
 * duplicating it into both arms, which is a bigger divergence).  So the
 * original's `limit` is live in eax across the join for a reason this
 * shape does not yet reproduce; the high arm's two-def clamp
 * (`g_popup_y = limit; if (y <= limit) g_popup_y = y;`) IS exact.  Also
 * ruled out: hoisting `limit = 0x16f - h` above the branch so the low arm
 * is a SECOND def of a multi-def variable (41), and re-nesting the high
 * arm's test under an explicit `if (y >= 0x25)` after a joined phi (41). */
// WIP-FUNCTION: LEGOLAND 0x004718c0  (42 of the original's 43 instructions;
//   only the y-clamp low bound differs -- see the note above)
void ClampPopUpToScreen(int size)
{
    int x = g_popup_x;
    int y = g_popup_y;
    int w = size * 0x20 + 0xc8;
    int h = size * 20 + 0x96;
    int limit;

    if (x > 0x27b - w) {
        x += -5 - w;
        if (x < 0)
            x = 5;
    }
    if (x < 0x82 && g_panel_state.f00 != 2)
        x = 0x82;
    if (y < 0x25) {
        limit = 0x25;
        g_popup_x = x;
        g_popup_y = limit;
    } else {
        limit = 0x16f - h;
        g_popup_x = x;
        g_popup_y = limit;
        if (y <= limit)
            g_popup_y = y;
    }
}

/* ============================================== pop-up text measurement */

typedef struct WinRect {
    int left;    /* +0x00 */
    int top;     /* +0x04 */
    int right;   /* +0x08 */
    int bottom;  /* +0x0c */
} WinRect;

__declspec(dllimport) void* __stdcall CreateCompatibleDC(void* dc);            /* [0x4ab094] */
__declspec(dllimport) int   __stdcall DrawTextA(void* dc, const char* s, int n,
                                                WinRect* rc, unsigned int fmt); /* [0x4ab2ac] */
extern void* SelectFont(void* dc, int font);                                   /* 0x00454b40 */
extern unsigned int strlen(const char* s);
#pragma intrinsic(strlen)

/* -------------------------------------------------------------------------
 * 0x00471840 -- how many 32-pixel steps the pop-up must GROW to fit its
 * title on one line.
 *
 * DrawText is measured (DT_CALCRECT | DT_CENTER) with the rect's right edge
 * seeded at `w` -- but with NO DT_WORDBREAK, so the returned rect is the
 * title's natural single-line width; the answer is
 * ceil((width - w) / 32) computed as `(width - w + 31) >> 5`.
 *
 * `a`, `b` and `step` are dead: the caller passes (0x40, 0x14, 0xb0, 0x20, 1)
 * but only the 0xb0 base width and the font id reach any instruction, and
 * the 32 is hard-coded rather than taken from `step`.  Reproduced.
 *
 * ORIGINAL BUG: the memory DC from CreateCompatibleDC is never released --
 * neither the old font is selected back nor is DeleteDC called (compare
 * bighelp.c's BubbleHelp, which does both).  Every pop-up resize leaks a DC.
 * ------------------------------------------------------------------------- */
// FUNCTION: LEGOLAND 0x00471840
int MeasurePopUpTitle(const char* s, int a, int b, int w, int step, int font)
{
    WinRect rc;
    void*   dc;

    rc.left = 0;
    rc.top = 0;
    rc.right = 0;
    rc.bottom = 0;
    dc = CreateCompatibleDC(0);
    rc.right = w;
    SelectFont(dc, font);
    DrawTextA(dc, s, strlen(s), &rc, 0x401);
    return (rc.right - rc.left - w + 0x1f) >> 5;
}

/* -------------------------------------------------------------------------
 * 0x004717a0 -- how many steps the pop-up must GROW to fit its body text.
 *
 * Same memory DC, but measured with DT_WORDBREAK (0x411), so DrawText
 * returns the WRAPPED HEIGHT for the current width.  The panel is then
 * widened one step at a time until the wrapped text fits the height budget:
 * starting from height `h0` and width `w`, each step adds `dh` to the budget
 * and `dw` to the width and counts one.  The count is the return value.
 *
 * The loop re-tests the CACHED height against the new budget before going
 * round again, so a step that makes the budget large enough exits without a
 * second DrawText.  That shape is load-bearing: written
 *     do { t = DrawText(...); if (t > h) { h += dh; ...; } } while (t > h);
 * VC6 emits ONE DrawText block with a real back edge, because the top test's
 * flags already decide the bottom one on the skip path.  Every spelling with
 * an explicit `break` (`for(;;)`, `while ((t = f()) > h)`, a goto loop, or
 * hoisting the first call out) makes VC6 PEEL the first iteration and emit
 * the 14-instruction strlen+DrawText block TWICE -- 71 instructions instead
 * of 56.
 * The step counter lives in the (now dead) `h0` parameter slot; VC6 only
 * puts it there if the C assigns into the parameter after its last read
 * (`h = h0; h0 = 0;`), otherwise it takes a fifth stack slot and the frame
 * grows to 0x14.
 * Same leaked DC as MeasurePopUpTitle.
 * ------------------------------------------------------------------------- */
// FUNCTION: LEGOLAND 0x004717a0
int MeasurePopUpBody(const char* s, int h0, int dh, int w, int dw, int font)
{
    WinRect rc;
    void*   dc;
    int     h;
    int     t;

    rc.left = 0;
    rc.top = 0;
    rc.right = 0;
    rc.bottom = 0;
    dc = CreateCompatibleDC(0);
    h = h0;
    h0 = 0;                       /* the step counter, in the dead h0 slot */
    rc.right = w;
    SelectFont(dc, font);
    do {
        t = DrawTextA(dc, s, strlen(s), &rc, 0x411);
        if (t > h) {
            h += dh;
            rc.right += dw;
            h0++;
        }
    } while (t > h);
    return h0;
}
