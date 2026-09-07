/* LEGOLAND -- scope LL12: dead (unreferenced) functions the linker kept,
 * 0x00434810..0x0043f4f0.  Nothing live in the binary calls, tail-jumps to
 * or takes the address of any of these; they were found by the padding
 * sweep (tools/inventory.py, scope N).  They are ordinary C from the same
 * translation units as their nearest matched neighbours, so the layouts and
 * helper names below follow screencb2.c / screencb6.c / screencb7.c
 * (the small class callbacks), junglecruise.c / ridecb2.c (the jungle
 * cruise records and the map grid), mechrides.c (the plane ride) and
 * loaders.c.  See docs/lanes/scope-ll12.md.
 *
 * VC6 SP3 /O2 /Gy /Gd.  Struct offsets are load-bearing; names are ours.
 */

/* ---- shared small types (screencb6.c / junglecruise.c layouts) ---------- */
typedef struct Pos  { int x, y; } Pos;
typedef struct Rect { int left, top, right, bottom; struct Rect* next; } Rect;

/* A packed 2-byte map square and its 16-bit view (junglecruise.c). */
typedef struct BPos  { unsigned char x, y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

/* An object class descriptor (the 0xd0-byte ODF record).  Only the fields
 * this file touches are named; the footprint rect is at +0x3c. */
typedef struct ObjDef {
    unsigned char pad00[0x3c];
    Rect          rect;             /* +0x3c  class footprint */
} ObjDef;

/* One LLIDB element; +0x0c is the class descriptor it owns. */
typedef struct RideElem {
    unsigned char pad00[0x0c];
    ObjDef*       data;             /* +0x0c */
} RideElem;

/* A placed map object; only +0x0c (the class) is touched here. */
typedef struct MapObj {
    unsigned char pad00[0x0c];
    ObjDef*       cls;              /* +0x0c */
    unsigned char pad10[4];
} MapObj;

/* The edit / destroy cursor (objmap2.c's Cursor, ridecb2.c's view). */
typedef struct Cursor {
    unsigned char  pad0000[0x1404];
    Pos            origin;          /* +0x1404 */
    int            status;          /* +0x140c */
    int            error;           /* +0x1410 */
    Rect           rect;            /* +0x1414 footprint rect list */
    unsigned char  pad1428[0x1828 - 0x1428];
    unsigned int   flags;           /* +0x1828 */
    int            f182c;           /* +0x182c */
    struct Cursor* next;            /* +0x1830 */
} Cursor;                           /* 0x1834 */

/* ---- the map grid (junglecruise.c / objmap2.c layout) ------------------- */
typedef struct Cell {
    void*          obj;             /* +0x00 */
    unsigned short key;             /* +0x04 */
    unsigned char  pad06[0x0c - 0x06];
    unsigned short flags;           /* +0x0c  map flags */
    unsigned char  pad0e[0x10 - 0x0e];
    unsigned char  rf;              /* +0x10 */
    unsigned char  pad11[0x14 - 0x11];
} Cell;                             /* 0x14 */

typedef struct MapHdr {
    unsigned char  pad00[0x14];
    unsigned short width;           /* +0x14 */
    unsigned short height;          /* +0x16 */
    unsigned char  pad18[0x20 - 0x18];
    unsigned short origin_x;        /* +0x20 */
    unsigned short origin_y;        /* +0x22 */
} MapHdr;

/* The fourth jungle-cruise decoration record: {own square, owning station
 * square, next} -- the same head as the monkey tree (ridecb2.c's JcDeco). */
typedef struct JcDeco {
    BPosW          pos;             /* +0x00 */
    BPosW          owner;           /* +0x02 */
    struct JcDeco* next;            /* +0x04 */
} JcDeco;                           /* 0x08 */

/* ---- globals ------------------------------------------------------------ */
extern MapHdr*  g_map;                  /* 0x004bcbf4 */
extern Cell**   g_map_rows;             /* 0x00801400 */

extern int      g_edit_changed;         /* 0x008119b0 */
extern ObjDef*  g_edit_object;          /* 0x008119b8 */
extern unsigned int g_ui_flags;         /* 0x008003e8 */
extern Cursor   g_edit_cursor;          /* 0x007febc0 */
extern Pos      g_edit_cursor_origin;   /* 0x007fffc4 == g_edit_cursor.origin */
extern Rect     g_edit_cursor_rect;     /* 0x007fffd4 == g_edit_cursor.rect */
extern Cursor*  g_edit_cursor_next;     /* 0x008003f0 == g_edit_cursor.next */

extern ObjDef*  g_jc_deco_cls;          /* 0x0081cb64 */
extern JcDeco*  g_jc_deco;              /* 0x00629c34 */
/* The second footprint rect the decoration chains onto the edit cursor's
 * own rect: the same box shifted six squares to the left. */
extern Rect     g_jc_deco_rect2;        /* 0x00616168 */
/* The scratch preview cursor the decoration hangs off the edit cursor. */
extern Cursor   g_jc_deco_cursor;       /* 0x006283f8 */

/* ---- callees ------------------------------------------------------------ */
extern void  DefaultCursor(Cursor* c);                       /* 0x0045a390 */
extern void  SetEditCursorFootPrint(Rect* rect);             /* 0x0045f440 */
extern void  BasicObjectDCalcCursor(void* elem, void* pos);  /* 0x00480bb0 */
extern void  AddBasicObject(RideElem* elem, Pos* pos);       /* 0x0045efe0 */
extern void* HeapAlloc_w(unsigned int size);                 /* 0x0049e4ff */
extern void  ScreenToMapRef(int sx, Pos* out, int sy);       /* 0x0045be90 */
extern void  ValidateCursor(Cursor* c, ObjDef* cls);         /* 0x0045f810 */
extern int   CursorIsValid(Cursor* c);                       /* 0x0045f4b0 */
extern void  ResetCursorFootprint(Cursor* c);                /* 0x0045f460 */
extern void  SetCursorError(Cursor* c, int code);            /* 0x0045f480 */
extern int   JungleCruise_ProbeRiver(int x, int y, BPosW* owner); /* 0x00436fb0 */

/* The bounds-checked cell fetch every map accessor open-codes; the callers
 * below dereference the result WITHOUT a null check, exactly as the
 * original does (junglecruise.c). */
static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

/* The same fetch reading the cell straight out of a Pos, so the y read
 * stays LAZY (the original loads pos->y only after the two x tests). */
static __inline Cell* MapCellAtPos(Pos* p)
{
    if (p->x >= 0 && p->x < g_map->width && p->y >= 0 && p->y < g_map->height)
        return &g_map_rows[p->y][p->x];
    return 0;
}

/* =========================================================================
 * The fourth JUNGLE CRUISE decoration class (ObjDef at 0x0081cb64), the one
 * whose remove handler junglecruise.c already matches at 0x00434b40.  These
 * five are its remaining SetCustomCallbacks slots; the class is never
 * registered, so nothing reaches them.
 * ========================================================================= */

/* +a4 create: cache the class descriptor. */
// FUNCTION: LEGOLAND 0x00434810
void JcDeco_Create(RideElem* elem)
{
    g_jc_deco_cls = elem->data;
}

/* The "place one of these" toolbar arm (cf. JcMonkeyTree at 0x00433ce0). */
// FUNCTION: LEGOLAND 0x00434820
void JcDeco_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_jc_deco_cls;
    DefaultCursor(&g_edit_cursor);
    g_ui_flags |= 8;
    SetEditCursorFootPrint(&g_jc_deco_cls->rect);
}

/* +94 draw-selection: the standard cursor calculation. */
// FUNCTION: LEGOLAND 0x00434b20
void JcDeco_DrawSelection(void* elem, void* pos)
{
    BasicObjectDCalcCursor(elem, pos);
}

/* cb_add: record the decoration against the river square three cells to its
 * left, then set map flag 0x40 on the three cells SIX squares left of the
 * placement cell -- the mirror image of JcDeco_Remove's clear.  Off-map
 * cells reach `or word [0+0xc]` and fault: original. */
// FUNCTION: LEGOLAND 0x00434860
void JcDeco_Add(RideElem* elem, Pos* pos)
{
    BPosW   bp;
    BPosW   owner;
    JcDeco* d;

    bp.b.x = (unsigned char)pos->x;
    bp.b.y = (unsigned char)pos->y;
    g_edit_cursor_next = 0;
    JungleCruise_ProbeRiver(pos->x - 3, pos->y, &owner);
    d = (JcDeco*)HeapAlloc_w(8);
    if (d) {
        d->pos = bp;
        d->owner = owner;
        d->next = g_jc_deco;
        g_jc_deco = d;
        AddBasicObject(elem, pos);
        pos->x -= 6;
        MapCellAtPos(pos)->flags |= 0x40;
        pos->y--;
        MapCellAtPos(pos)->flags |= 0x40;
        pos->y += 2;
        MapCellAtPos(pos)->flags |= 0x40;
    }
}

/* cb_calc_cursor: the placement preview.  The footprint is the class rect
 * plus a second copy six squares to the left; the decoration may only go
 * next to river, so the river two cells to the right must have a WEST arm
 * (bit 8), and then a scratch cursor shows the square it will attach to. */
// WIP-FUNCTION: LEGOLAND 0x004349b0  (92.6%, strict 7/94, first divergence i66:
//   the scratch cursor's rect.top temp lands in ecx where the original uses
//   eax -- the whole tail is one scratch register out of phase (i66/70/74 and
//   i84-87); every block, every store order and every frame slot agree.)
void JcDeco_CalcCursor(MapObj* o, int sx, int sy)
{
    ObjDef* cls;
    BPosW   owner;
    int     found;

    cls = o->cls;
    g_edit_cursor_rect = cls->rect;
    ScreenToMapRef(sx, &g_edit_cursor_origin, sy);
    found = JungleCruise_ProbeRiver(g_edit_cursor_origin.x + 2,
                                    g_edit_cursor_origin.y, &owner);
    g_edit_cursor_rect.next = &g_jc_deco_rect2;
    g_jc_deco_rect2.top = g_edit_cursor_rect.top;
    g_jc_deco_rect2.bottom = g_edit_cursor_rect.bottom;
    g_jc_deco_rect2.left = g_edit_cursor_rect.left - 6;
    g_jc_deco_rect2.right = g_edit_cursor_rect.right - 6;
    g_jc_deco_rect2.next = 0;
    g_edit_cursor_next = 0;
    if (found == 0) {
        SetCursorError(&g_edit_cursor, 14);
        return;
    }
    ValidateCursor(&g_edit_cursor, cls);
    if (CursorIsValid(&g_edit_cursor)) {
        DefaultCursor(&g_jc_deco_cursor);
        g_jc_deco_cursor.rect = g_edit_cursor_rect;
        g_jc_deco_cursor.rect.top--;
        g_jc_deco_cursor.rect.bottom++;
        g_jc_deco_cursor.rect.left -= 5;
        g_jc_deco_cursor.rect.right--;
        g_jc_deco_cursor.rect.next = 0;
        g_jc_deco_cursor.flags = 0x34;
        ResetCursorFootprint(&g_jc_deco_cursor);
        if (found & 8) {
            g_jc_deco_cursor.origin.x = g_edit_cursor_origin.x;
            g_jc_deco_cursor.origin.y = g_edit_cursor_origin.y;
            g_edit_cursor_next = &g_jc_deco_cursor;
        }
    }
}

/* =========================================================================
 * A modal text-entry dialog (loaders.c neighbourhood).  Nothing reaches it;
 * it is a self-contained "ask the user for a string" pump, one redraw per
 * frame, that runs until GetInputChar reports RETURN (-3).
 * ========================================================================= */

/* The dialog panel rectangle: position and SIZE (not corners). */
typedef struct IRect { int x, y, w, h; } IRect;

/* The edit field's corners (four ints, 0x10 bytes on the caller's frame). */
typedef struct Box { int left, top, right, bottom; } Box;

extern char GetInputChar(void);                                 /* 0x004740b0 */
extern void PrintLimitedText(int x, int y, int w, const char* text, int font,
                             unsigned long colour, unsigned int format); /* 0x00454c70 */
extern int  ProcessSystemEvents(void);                          /* 0x00480050 */
extern void ReadGameButtons(void);                              /* 0x00452460 */
extern void PushRenderingStatusAndLockVideoSurface(void);       /* 0x00463fc0 */
extern int  PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern int  GetNearestColour(int r, int g, int b);              /* 0x0044e6c0 */
extern int  RenderBlock(int x, int y, int w, int h, int colour); /* 0x004890c0 */
extern void RenderThickBox(int x, int y, int w, int h, int t, int colour); /* 0x00489390 */
extern int  RenderingComplete(void);                            /* 0x00466500 */
extern void PopRenderingStatus(void);                           /* 0x004641f0 */

#pragma intrinsic(strlen)
extern unsigned int strlen(const char* s);

/* One keystroke of the field plus its redraw.  GetInputChar's control codes:
 * -3 RETURN (returns 1, the caller's "done"), -2 ESCAPE (returns 0 WITHOUT
 * redrawing), -1 BACKSPACE.  `font` is dead -- the caller passes 0 and the
 * text is always printed in font 2. */
// FUNCTION: LEGOLAND 0x0043f460
int TextEntryFieldStep(Box* box, int font, char* buf, int maxlen, int* plen)
{
    char c = GetInputChar();

    if (c != 0) {
        if (c != -3) {
            if (c == -2)
                return 0;
            if (c != -1) {
                if (*plen < maxlen - 1) {
                    buf[*plen] = c;
                    (*plen)++;
                    buf[*plen] = 0;
                }
            } else {
                if (*plen > 0)
                    (*plen)--;
                buf[*plen] = 0;
            }
        } else {
            return 1;
        }
    }
    PrintLimitedText(box->left + 4, box->top + 4,
                     box->right - box->left - 8, buf, 2, 0, 0);
    return 0;
}

/* The pump: a light panel with a dark blue title bar over `backdrop`, the
 * prompt text in the bar and the edit field inset {8, 0x20, w-9, h-9}.
 * Returns the final length of `buf`; a ProcessSystemEvents failure (the app
 * is quitting) breaks out with whatever has been typed so far. */
// FUNCTION: LEGOLAND 0x0043f4f0
int RunTextEntryDialog(void* backdrop, IRect* r, const char* title,
                       char* buf, int maxlen)
{
    int len;
    Box box;
    int done;

    len = strlen(buf);
    box.left = r->x + 8;
    box.top = r->y + 0x20;
    box.right = r->x + r->w - 9;
    box.bottom = r->y + r->h - 9;
    done = 0;
    while (!done) {
        if (!ProcessSystemEvents())
            break;
        ReadGameButtons();
        PushRenderingStatusAndLockVideoSurface();
        PrintSprite(backdrop, 0, 0, 0, 0);
        RenderBlock(r->x, r->y, r->w, r->h, GetNearestColour(0xef, 0xef, 0xef));
        RenderThickBox(r->x, r->y, r->w, r->h, 2, 0);
        RenderBlock(r->x + 2, r->y + 2, r->w - 4, 0x18,
                    GetNearestColour(0, 0x3f, 0x7f));
        PrintLimitedText(r->x + 2, r->y + 2, r->w - 4, title, 0, 0xefefef, 0);
        RenderThickBox(box.left, box.top, box.right - box.left,
                       box.bottom - box.top, 2, 0);
        done = TextEntryFieldStep(&box, 0, buf, maxlen, &len);
        RenderingComplete();
        PopRenderingStatus();
    }
    return len;
}

/* =========================================================================
 * A debug/tuning VERTICAL SLIDER widget and the panel that drives it
 * (mechrides.c neighbourhood).  Also dead.
 * ========================================================================= */

extern int g_mouse_ev2;                 /* 0x00813ac4  bit 2 = button held */
extern Pos g_gfx_point;                 /* 0x00813a44  the mouse point */
/* Set while the pointer grabbed a slider; one flag shared by every slider,
 * so only one may be dragged at a time. */
extern int g_slider_grabbed;            /* 0x0062fea4 */

/* Draws the slider track and its red 3-pixel marker, then returns the value
 * the pointer is asking for: `value` unless the slider is grabbed. */
// FUNCTION: LEGOLAND 0x0043e930
int Slider_Track(Box* r, int lo, int hi, int value)
{
    int h = r->bottom - r->top;
    int pos = h * value / (hi - lo);
    int held;
    int my;

    RenderThickBox(r->left, r->top, r->right - r->left, r->bottom - r->top, 2, 0);
    RenderBlock(r->left, r->top + pos - 1, r->right - r->left, 3,
                GetNearestColour(0xff, 0, 0));
    held = g_mouse_ev2 & 4;
    if (held && g_gfx_point.x >= r->left && g_gfx_point.x <= r->right
        && g_gfx_point.y >= r->top && g_gfx_point.y <= r->bottom) {
        g_slider_grabbed = 1;
    } else if (g_slider_grabbed == 0) {
        return value;
    }
    if (held != 0) {
        my = g_gfx_point.y;
        if (my < r->top)
            my = r->top;
        else if (my > r->bottom)
            my = r->bottom;
        return (my - r->top) * (hi - lo) / (r->bottom - r->top);
    }
    g_slider_grabbed = 0;
    return value;
}

/* =========================================================================
 * The scrolling LIST PICKER these dialogs are built on, and the LLIDB
 * front-end that fills it.  All dead.
 * ========================================================================= */

/* One LLIDB element as this picker reads it. */
typedef struct LLElem {
    const char*  name;      /* +0x00 */
    void*        f04;       /* +0x04 */
    unsigned int flags;     /* +0x08  bit 0 = "happy", the caller's mask */
} LLElem;

extern int   LLIDB_GetCount(void);                          /* 0x0047b2d0 */
extern void  LLIDB_GetElement(int i, LLElem** out);         /* 0x0047b2e0 */
extern void* LoadSprite(const char* name, int mode);        /* 0x00497ab0 */
extern void  KillSprite(void* sprite);                      /* 0x00497bd0 */
extern void  HeapFree_w(void* p);                           /* 0x0049e4d0 */
extern int   NameCompare(const char* a, const char* b);     /* 0x004aab90 (_stricmp) */

/* scope LL13 owns these two: wrapped-text measure and draw. */
extern int   MeasureWrappedText(const char* text, int font, int width); /* 0x004551a0 */
extern void  DrawWrappedText(int x, int y, const char* text, int font,
                             int width);                    /* 0x00455220 */

int RunListPicker(char** items, const char* title, void* backdrop, IRect* r,
                  void (*overlay)(int sel), void** icons, int a7, int a8,
                  int keep_scroll);                         /* 0x0043ea30 */

/* Builds the picker's string array from every LLIDB element whose flags
 * carry `mask`, sorts it by name and runs the picker; returns the chosen
 * element (0 if the user backed out).  The two sprites are loaded and
 * killed but never handed to the picker -- `icons` is one of its three
 * DEAD parameters. */
// FUNCTION: LEGOLAND 0x0043eee0
LLElem* PickLLIDBElement(const char* title, void* backdrop, IRect* r,
                         unsigned int mask, int keep_scroll)
{
    int      n;
    LLElem*  elem;
    LLElem** list;
    char**   names;
    void**   icons;
    void*    spr_happy;
    void*    spr_poor;
    int      total;
    int      matches;
    int      i;
    int      pass;
    int      sel;
    LLElem*  result;

    total = LLIDB_GetCount();
    matches = 0;
    spr_happy = LoadSprite("happy.lls", 0);
    spr_poor = LoadSprite("poor.lls", 0);
    for (i = 0; i < total; i++) {
        LLIDB_GetElement(i, &elem);
        if (elem->flags & mask)
            matches++;
    }
    names = (char**)HeapAlloc_w(matches * 4 + 4);
    list = (LLElem**)HeapAlloc_w(matches * 4);
    icons = (void**)HeapAlloc_w(matches * 4);
    n = 0;
    for (i = 0; i < total; i++) {
        LLIDB_GetElement(i, &elem);
        if (elem->flags & mask) {
            list[n] = elem;
            n++;
        }
    }
    names[n] = 0;
    for (pass = 0; pass < n - 1; pass++) {
        int j;
        int swapped = 0;

        for (j = n - 2; j >= pass; j--) {
            if (NameCompare(list[j + 1]->name, list[j]->name) < 0) {
                LLElem* t = list[j + 1];

                list[j + 1] = list[j];
                list[j] = t;
                swapped = 1;
            }
        }
        if (!swapped)
            break;
    }
    for (i = 0; i < n; i++) {
        names[i] = (char*)list[i]->name;
        if (list[i]->flags & 1)
            icons[i] = spr_happy;
        else
            icons[i] = spr_poor;
    }
    sel = RunListPicker(names, title, backdrop, r, 0, icons, 0x2e, 0x28,
                        keep_scroll);
    if (sel != -1)
        result = list[sel];
    else
        result = 0;
    HeapFree_w(names);
    HeapFree_w(list);
    HeapFree_w(icons);
    KillSprite(spr_happy);
    KillSprite(spr_poor);
    return result;
}
