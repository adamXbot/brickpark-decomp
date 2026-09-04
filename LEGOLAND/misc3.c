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
 * The two caption widths are `panel right edge - text margin`, and the
 * original does NOT fold the two constants: it emits `add eax,0BBh` then
 * `add eax,0FFFFFF3Fh` (-0C1h) where a plain `g_pu_mock->w + 0xbb - 0xc1`
 * compiles to one `sub reg,6`.  VC6 SP3 reassociates `(X + c1) - c2` only
 * when the `X + c1` node has a SINGLE consumer; give it a second consumer
 * that is evaluated first and both adds survive (measured: a second call
 * argument, a store, or -- at zero instructions -- a TEST of the value in
 * an `if` condition whose body is empty, which VC6 deletes only after the
 * fold decision).  The empty guard below is that second consumer: it emits
 * nothing, and `if (right)`, `if (right < 0)`, `if (right > 0)`,
 * `if (right != 0)`, `if (right < 0xc1)`, `if (right >= 0x27f)` and
 * `if (right > 0xc1)` all give a byte-identical body, so the original's
 * source had some body-less (or compiled-out) test of the panel's right
 * edge between the edge and the two captions.  Everything that does NOT
 * work, for the record: every spelling of the expression itself (operand
 * order, `+ -0xc1`, `~0xc0`, `-(0xc1 - R)`, three-constant chains), casts
 * that do not change width (`(unsigned)`, `(long)`, `| 0`, `^ 0`, `* 1`),
 * a `short` local (adds a movsx), routing either constant through a local
 * -- single assignment, repeated assignment, `const`, `enum` -- four
 * `static __inline` shapes including a right-edge accessor and a
 * `PrintBox(s,x,y,x2,y2,...)`, a shared `right` used by both captions (VC6
 * spills it across the call: +2 instructions and a worse body) or by the
 * icon placements as well, a `volatile` read of the width field (blocks the
 * fold but changes the schedule), and a degenerate ternary (same).
 *
 * Finally each of the three icons whose sprite the cursor is NOT over is
 * reset to its "off" sprite (the hover sprite is set by the icon's own
 * input handler), and the carousel ends are greyed out: prev is hidden on
 * the first entry, next on the last.
 * ------------------------------------------------------------------------- */
// FUNCTION: LEGOLAND 0x004720a0
void DrawPopUpMock(void)
{
    BlitCtx ctx;
    char    text[0x80];
    int     right;

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

    /* `right` is the panel's right edge; the empty guard is what stops VC6
     * folding the two constants into one `sub` (see the note above). */
    right = g_pu_mock->w + 0xbb;
    if (right > 0xc1) { }
    PrintCachedText(text, 0xc1, 0x30, right - 0xc1, 0x14,
                    2, 5, 0xff0000, 0xffffff);
    right = g_pu_mock->w + 0xbb;
    if (right > 0xc1) { }
    PrintCachedText(g_mock_defs[g_mock_index]->name, 0xc1, 0x4b,
                    right - 0xc1, 0x14, 2, 5, 0xff0000, 0xffffff);
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
/* CLOSED (was 355 emitted against the original's 354: one extra
 * `mov edx,[esp+10h]` reload in the rect loop's landing pad).  The original
 * reloads only `i` there
 *     0049acbc  mov eax,[esp+18h]        ; i
 *     0049acc0  mov ecx,[edx+10h]        ; o->rects  -- edx still holds `o`
 * because it keeps `o` in edx across the back edge, where ours reloaded it
 * too and the pre-header's `jmp` therefore landed one instruction further on.
 *
 * The fix is the SOURCE ORDER of the two independent entry stores: writing
 * `colour` before `o` in both arms of the `mechanic` test.  VC6 reorders the
 * pair anyway (it emits colour first either way), so the emitted prologue is
 * identical, but the order the two webs are CREATED in decides their
 * allocation priority, and with `colour` first `o`'s web survives in edx over
 * the whole rect loop and the landing pad loses its reload.  Frame, block
 * layout and every other instruction were already exact.
 *
 * Two other shapes also produced the one-instruction pad, both at the cost of
 * moving code: reading `sw`/`sh` after the first corner probe (354/1107B but
 * 15 mismatches around index 185), and declaring `cx`/`cy` inside
 * `if (mechanic)` (354 but VC6 then hoists the mechanic test above the
 * centre computation, 22 mismatches from index 291).  Inert, for the record:
 * every loop spelling for both loops; `&o->rects[i]` vs `o->rects + i` vs the
 * index written out; a function-level `rc`; `!i` for `i == 0`; block scope
 * for l1/r2/t3, cx/cy, sw/sh, pt/b, r, or all of them; a named widened
 * `sw2`; a `Sprite* sp = cls->sprite`; `sw << 1`; `(l1 + r2 - sw)/2`;
 * a `Probe(&pt, &b, x, y)` inline helper for the four corner probes;
 * `unsigned colour`; reading `cls` as `o->obj->cls` at each use; declaring
 * `path` last; and zero-cost dead tests (`if (path) { }` and friends) at the
 * top or bottom of the `i == 0` arm.
 */
// FUNCTION: LEGOLAND 0x0049ac50
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

    /* colour BEFORE o in both arms: the two stores are independent and VC6
     * reorders them anyway, but their source order decides the order the two
     * webs are created in, and that is what puts `o` in edx across the rect
     * loop's back edge (see the note above). */
    if (mechanic) {
        colour = 0xe04040;
        o = g_mechanic_orders;
    } else {
        colour = 0x40e040;
        o = g_gardener_orders;
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
/* RESIDUAL (3 of 43; was 16 of 42).  The function RETURNS the y bound it
 * settled on -- popup.c's only call site declares it `void` and ignores the
 * value, but nothing else explains the two eax defs: the low arm's
 * `mov eax,25h` feeds BOTH the store to g_popup_y and the return, and the
 * high arm's eax is `0x16f - h` for the same two consumers.  With the
 * `return` in place the body is 43/43 instructions and the low arm is
 * emitted exactly (`mov eax,25h / pop edi / mov [7fdeccH],ecx /
 * mov [7fded0H],eax / pop esi / ret`).
 *
 * What is left (first diverging index 25, 143 vs 144 bytes) is WHERE the
 * constant's def sits:
 *     original   cmp esi,25h / jge <high> / mov eax,25h
 *     ours       mov eax,25h / cmp esi,eax / jge <high>
 * VC6 builds a constant WEB as soon as a constant has two register uses, and
 * the web then ABSORBS the compare's immediate occurrence and parks the def
 * at the common dominator (one byte shorter, hence the 143).  Proof of the
 * mechanism, both measured: change only the compare's constant (`y < 0x26`)
 * and the def drops into the low arm with everything else identical
 * (mismatch 1, the compare); write the test as `y - 0x25 < 0` (lea/test) and
 * the low arm is again exact.  The one-register-use case does NOT web --
 * buildtick.c's GetBuildTime is `cmp eax,32h / jge / mov eax,32h` from
 * `if (cost < 50) return 50;` -- so the original's source must spell the
 * compare's 0x25 as a constant distinct from the arm's, and no C spelling
 * found does that: VC6 keys constants by VALUE only.  Ruled out (all give
 * the same hoist): `int top = 0x25` used for the compare, for the arm, or
 * for both; a `const` local; a two-def `limit` dominating the branch;
 * `static __inline int Top(void) { return 0x25; }`; `37`, `'\045'`,
 * `0x26 - 1`, `-(-0x25)`, `0x25u`, `0x25L`; unsigned/short/char/long return
 * types; an unsigned alias of the global for the store; `return
 * (g_popup_y = 0x25)`, the comma form, a read-back `return g_popup_y`,
 * `y = 0x25; return y;`; an inlined `SetPos(x, 0x25)` helper; `!(y >= 0x25)`,
 * `0x25 > y`, `g_popup_y < 0x25`, `(unsigned)y < 0x25u` (gives `jae`);
 * `y + 1 < 0x26` and `y - 1 < 0x24` (VC6 keeps the inc/dec: 34 instructions);
 * a `goto` that puts the low arm's TEXT before the compare; and zero-cost
 * dead tests (`if (w) { }`, `if (h) { }`, `if (x) { }`, `if (y) { }`, before
 * the clamp or inside the arm) meant to keep eax busy at the compare -- the
 * trick that closed DrawPopUpMock is inert here because the web's placement,
 * not a fold, is what moves.  Also inert: a DEGENERATE branch defining the
 * bound on both arms (`if (g_panel_state.f00 == 2) top = 0x25; else top =
 * 0x25;`, and the same on `x`/`size`) in the hope that the phi would hide the
 * constant from the compare until after web placement -- VC6 folds those at
 * the front end; carrying the bound in a reassigned PARAMETER (`size`) or in
 * a dead local (`x`, `w`, `h`) after its last use; and `*(volatile int*)&top`
 * at the compare (a real reload, 35 instructions).
 * Also still ruled out from the earlier void-returning shape: routing the
 * constant through `limit`, a Pos struct for the two globals, swapped arms,
 * a dead symmetric `if (y > limit) g_popup_y = y;` (range-folded away), and
 * a joined phi after the if/else (41 instructions -- one shared store pair).
 * Only the three instructions at 25..27 differ; the whole x clamp and the
 * whole high arm are index-for-index exact.
 *
 * Measured rule, worth keeping: a constant with ONE register use plus an
 * immediate use stays split (GetBuildTime); with TWO register uses it becomes
 * a web that swallows every other occurrence, including `cmp reg,imm`, and
 * the def lands at their common dominator.  renderinit.c's
 * SetBridgeDrawOffsets is the same web seen from the other side -- `mov
 * eax,6Dh` exists there only because 0x6d is stored to two globals.
 *
 * 2026-09-04, endgame lane.  The corpus scan the method asks for was written
 * and run (scratchpad/endgame/scan_const2.py: every exact body holding BOTH
 * `cmp r,K` and a separate `mov r,K` for the same non-trivial K).  17 sites,
 * and one of them is the worked example that states both regimes in its own
 * note: memdb.c's `__DEBUG_TAG` (0x453a30) --
 *     `if (len >= 12) n = 12; else n = len;`  keeps `cmp ecx,0Ch` IMMEDIATE
 *     `n = 12; if (len < 12) n = len;`        CSEs the 12, `cmp ecx,esi`
 * i.e. single-assignment ARMS keep the immediate.  Every other split site
 * (GetBuildTime, RenderIcons, CalculateViewRideCode, Calc_Item_Attractiveness,
 * LLIDB_SaveICM/LoadICM/CloseICM, __DEBUG_MALLOC/SMALLOC, UnInitMan,
 * MakeAnimInstance) is a clamp whose constant has exactly ONE register use.
 * There is NO site anywhere in the exact corpus where a constant with two
 * register uses stays split, so the rule above survives the scan intact.
 *  - Applying the __DEBUG_TAG shape here is inert.  Measured, all
 *    byte-identical to the committed body (3): routing the low bound through
 *    the high arm's `limit` in four statement orders, assigning it before the
 *    branch, reusing `y`/`x`/`size`/`w`/`h` as the carrier, a two-arm phi with
 *    the tails written out in full, and FOUR SINGLE-EXIT forms (one
 *    `return limit;` after a real if/else join, both arm orders).  VC6
 *    constant-propagates the carrier back to a literal before web building in
 *    every one of them.  Restructurings that do change the code are all worse:
 *    a joined phi with the `y >= 0x25 &&` guard 15-17, swapped arms 14, a
 *    fall-through low arm 14.
 *  - NEW AND USEFUL: the web CAN be broken, and the mechanism is register
 *    occupancy, not spelling.  Hoisting `limit = 0x16f - h;` above the y test
 *    ("min_form") leaves eax holding the limit across the compare; VC6 then
 *    cannot park the constant's def there, the compare KEEPS `cmp esi,25h`,
 *    the low arm gets an immediate store plus its own `mov eax,25h`, and the
 *    body is 43 instructions / 144 BYTES -- the original's byte count exactly
 *    (though 18 mismatches, because the limit computation and the g_popup_x
 *    store move above the branch).  So the question is now sharp: in the
 *    original, eax and edi are BOTH free at index 25 and VC6 still did not
 *    hoist.  Zero-cost dead tests cannot occupy a register, and there is no
 *    value in this function that is live across the compare, so no C spelling
 *    reproduces the occupancy.
 *  - SECOND NEW FACT, and it isolates the trigger exactly.  Replace the low
 *    arm's `return 0x25;` with `return *(volatile int*)&g_popup_y;` (a probe,
 *    not a candidate -- the returned value is unobservable but the reload is
 *    not the original's code): indices 0..26 then match INCLUDING
 *    `cmp esi,25h / jge`, and only the four instructions of the low arm are
 *    wrong (4 mismatches).  So the compare's immediate survives precisely
 *    when the constant needs NO register in the low arm.  With one register
 *    materialisation VC6 always webs it with the compare and hoists the def.
 *    The original has BOTH -- `cmp esi,25h` immediate AND `mov eax,25h` in
 *    the arm feeding a register store -- which is only consistent with
 *    `mov eax,25h` being a DEF OF THE RETURN-VALUE WEB (whose other def is
 *    the high arm's `0x16f - h`) rather than a constant materialisation, the
 *    same shape as buildtick.c's GetBuildTime.  Every attempt to build that
 *    web here fails on constant propagation: the low arm's early `return`
 *    means its own def dominates both uses, so VC6 folds the carrier back to
 *    a literal, and the only way to give the value two reaching defs -- a
 *    real if/else join -- needs an extra `y >= 0x25` guard, because on the
 *    low path `y <= bound` is true and the merged form would store `y`
 *    instead of 0x25.  That guard costs instructions (15-17 mismatches).
 *    Unless a matched function turns up with a two-register-use constant that
 *    stays split, treat this as exhausted.
 *
 * 2026-09-04, lane H.  ~35 more variants; the residual is UNCHANGED at 3, but
 * the mechanism is now isolated completely and the USE COUNT is measured, not
 * inferred.  Three controls, all on the committed body with one statement
 * changed:
 *   - `return 0x25;` with NO store (`retonly`): `cmp esi,25h` KEPT, the def
 *     drops into the arm.  So constant + compare + ONE register use does not
 *     web (GetBuildTime's shape, reproduced here).
 *   - `g_popup_y = 0x25;` with NO return (`storeonly`): `cmp esi,25h` KEPT and
 *     the store is an IMMEDIATE `mov dword ptr [7fded0h],25h`.  So a STORE of
 *     the constant is not a register use at all.
 *   - both (the real body): 3 uses -> web, def hoisted.  The original has all
 *     three (`cmp esi,25h`, `mov [7fded0h],eax`, `ret` with eax) and no web,
 *     which this VC6 never does for a three-use constant.
 * *** THE SHARPEST MEASUREMENT: `if (y <= 0x24)` (identically `if (!(y >
 * 0x24))`) gives 2 mismatches and 144/144 BYTES, with indices 27..42 -- the
 * whole low arm including `mov eax,25h` -- index-for-index exact. ***  Only
 * the compare pair is wrong (`cmp esi,24h / jg` for `cmp esi,25h / jge`).
 * That proves the residual is nothing but "the compare's constant equals the
 * arm's": make them different by ANY amount and the def lands in the arm.
 * NOT COMMITTED, and deliberately: the original plainly wrote `y < 0x25`
 * (`cmp esi,25h`), so `y <= 0x24` would trade one wrong instruction pair for
 * another and make the source less faithful for a cosmetic 3 -> 2.
 * Newly ruled out this round (all reproduce the committed body's 3 exactly
 * unless noted): every same-width TYPE barrier on the compare -- `y < 0x25L`,
 * `(long)y < 0x25L`, `y < (int)0x25u`, `y < (int)(short)0x25`, a `#define`,
 * an `enum` constant, a `long y` local (with `(int)y` at the kept-y store);
 * the same barriers on the ARM's constant -- `0x25L`, `(int)0x25u`,
 * `sizeof(struct { char _p[0x25]; })`, an `unsigned top` carrier; carriers
 * `int top` / `limit` / `y = 0x25` in the arm; `return g_popup_y = 0x25;`;
 * an inlined `static __inline int SetPopUp(int px, int py) { g_popup_x = px;
 * g_popup_y = py; return py; }` used by BOTH arms; a degenerate
 * `y < 0x25 ? 0x25 : 0x25` return; swapping the two stores in the arm (5);
 * hoisting `g_popup_x = x;` above the y test (14, VC6 does not sink it);
 * `-y > -0x25` and `y >= 0 && y < 0x25` (17-18, extra compares).
 * CONCLUSION unchanged and now evidence-backed: the original's low arm has a
 * three-use 0x25 whose def sits in the arm; no VC6 SP3 spelling reaches that,
 * because this build webs at three uses and keys constants by value only. */
// WIP-FUNCTION: LEGOLAND 0x004718c0  (43/43 instructions, 143/144 bytes,
//   mismatch 3: the y-clamp low bound's constant def is one block too early)
int ClampPopUpToScreen(int size)
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
        g_popup_x = x;
        g_popup_y = 0x25;
        return 0x25;
    }
    limit = 0x16f - h;
    g_popup_x = x;
    g_popup_y = limit;
    /* ORIGINAL BUG: the y-kept path still returns the limit, not y. */
    if (y <= limit)
        g_popup_y = y;
    return limit;
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
