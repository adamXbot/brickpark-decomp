/* LEGOLAND -- scope LL14: dead (unreferenced) code kept by the linker, from
 * the popup / iconbar / input / fpui neighbourhood (0x0046d3b0..0x004762f0).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). None of
 * these functions is referenced by anything live in the binary: they were
 * found by tools/inventory.py's padding sweep. They are ordinary C from the
 * same translation units as their matched neighbours (iconui.c, render2.c,
 * fpui.c, popupmisc.c, popup2.c, tinystubs.c, sysstubs.c, sweep2/3.c), and
 * the types below follow those files. Struct field OFFSETS, record sizes,
 * callee arg counts and global addresses are load-bearing; NAMES ARE OURS
 * (see docs/lanes/scope-ll14.md for the naming rationale).
 */

/* ------------------------------------------------------------------ types -- */

typedef struct SpriteRec {
    struct SpriteRec* next;    /* +0x00 */
    void*             surface; /* +0x04 */
    void*             image;   /* +0x08 */
    int               detail;  /* +0x0c */
    int               flags;   /* +0x10 */
    short             w;       /* +0x14 */
    short             h;       /* +0x16 */
    short             src_x;   /* +0x18 */
    short             src_y;   /* +0x1a */
    unsigned short    refs;    /* +0x1c */
    short             pad1e;   /* +0x1e */
} SpriteRec;

/* The 0x40-byte icon/gadget record (iconui.c Icon / fpui.c Icon / money.c
 * Gadget). The slots from +0x18 to +0x24 are polymorphic; the two dead
 * builders here use +0x18 as the caption text, and +0x20/+0x22 as the
 * caption's offset from the icon origin (a pair of SHORTS -- the stores are
 * `mov [eax+20h],dx`, and the caller loads them 16 bits wide). */
typedef struct Icon {
    struct Icon*   next;      /* +0x00 */
    SpriteRec*     sprite;    /* +0x04 */
    void*          data;      /* +0x08 */
    short          x;         /* +0x0c */
    short          y;         /* +0x0e */
    short          w;         /* +0x10 */
    short          h;         /* +0x12 */
    unsigned short group;     /* +0x14 */
    short          f16;       /* +0x16 */
    char*          text;      /* +0x18 caption (labelled icons) / owner */
    short          clip_x;    /* +0x1c */
    short          clip_y;    /* +0x1e */
    short          cap_dx;    /* +0x20 caption offset x */
    short          cap_dy;    /* +0x22 caption offset y */
    void*          f24;       /* +0x24 group callback 0 */
    int          (*render)(struct Icon*);                             /* +0x28 */
    char         (*input)(struct Icon*, int, short, short);           /* +0x2c */
    void*          widget;    /* +0x30 */
    unsigned int   flags;     /* +0x34 */
    char*          help;      /* +0x38 */
    int            help_id;   /* +0x3c */
} Icon;

typedef char (*IconInputFn)(Icon*, int, short, short);

/* The 5th PrintSprite argument (money.c / fpui.c BlitCtx). */
typedef struct BlitCtx {
    int kind;                            /* +0x00 */
    struct { void* p; int n; } owner;    /* +0x04, +0x08 */
} BlitCtx;

/* The bare 16-byte rectangle SetClipping / IntersectRect take. */
typedef struct WinRect {
    int left;    /* +0x00 */
    int top;     /* +0x04 */
    int right;   /* +0x08 */
    int bottom;  /* +0x0c */
} WinRect;

/* An LLIDB element (llidb.c / fpui.c LLElem): its name is the first field
 * and its loaded class definition hangs off +0x0c. */
typedef struct LLElem {
    char*             name;   /* +0x00 */
    char              pad04[0x0c - 0x04];
    struct ObjDef*    def;    /* +0x0c */
} LLElem;

/* fpui.c's 0xd0-byte ObjDef; only the fields these functions read. */
typedef struct ObjDef {
    char       pad00[0x1c];
    unsigned int flags1c; /* +0x1c */
    char       pad20[0x68 - 0x20];
    SpriteRec* icon;      /* +0x68 */
    char       pad6c[0x78 - 0x6c];
    char*      name;      /* +0x78 */
    char       pad7c[0xc4 - 0x7c];
    LLElem*    elem;      /* +0xc4 */
} ObjDef;

/* The 12-byte research/object list node (fpui.c ObjNode). */
typedef struct ObjNode {
    struct ObjNode* next;   /* +0x00 */
    ObjDef*         obj;    /* +0x04 */
    int             keep;   /* +0x08 */
} ObjNode;

/* workorder2.c's 17-record message table at 0x004ba8e0 (stride 12). */
typedef struct MessageDef {
    int topic;   /* +0x00 */
    int delay;   /* +0x04 */
    int last;    /* +0x08 */
} MessageDef;

/* iconui.c's g_gfx_point / popup2.c's g_input_point. */
typedef struct Pos { int x, y; } Pos;

/* --------------------------------------------------------------- globals -- */

extern int        g_help_face_state;   /* 0x006687a4 (tinystubs.c) */
extern void*      g_group_cb0;         /* 0x006688a8 -> icon +0x24, flag 0x04 */
extern void*      g_group_cb1;         /* 0x006688ac -> render,     flag 0x08 */
extern void*      g_group_cb2;         /* 0x006688b0 -> input,      flag 0x02 */

extern MessageDef g_messages[];        /* 0x004ba8e0 */
extern int        g_cursor_error_msg[];/* 0x004ba9ac */

/* The second control bar (the "message bar"): the same three CB_BG* slices
 * and the same two tool gadgets as popup2.c's DrawPopUpExtra, but placed at
 * its own point and captioned from its own text buffer. */
extern int        g_msgbar_active;     /* 0x00668d68 */
extern int        g_msgbar_x;          /* 0x007fe010 */
extern int        g_msgbar_y;          /* 0x007fe014 */
extern int        g_msgbar_cells;      /* 0x00668964 */
extern char       g_msgbar_text[];     /* 0x00668968 */
extern SpriteRec* g_cb_bg[3];          /* 0x00668904 .. 0x0066890c */
extern Icon*      g_cb_icon_ok;        /* 0x007fdea8 */
extern Icon*      g_cb_icon_close;     /* 0x007fe000 */
extern Pos        g_input_point;       /* 0x00813a44 */

extern ObjNode*   g_research_list;     /* 0x00668ed8 (fpui.c) */

/* The DirectInput keyboard state array lives at 0x007fdda0 (sysstubs.c names
 * g_left_shift = [0x2a] and g_right_shift = [0x36] out of it). */
extern unsigned char g_left_ctrl;      /* 0x007fddbd  = key[DIK_LCONTROL] */
extern unsigned char g_right_ctrl;     /* 0x007fde3d  = key[DIK_RCONTROL] */

/* --------------------------------------------------------------- callees -- */

void* memset(void*, int, unsigned int);
unsigned int strlen(const char*);
char* strcpy(char*, const char*);
#pragma intrinsic(strlen, strcpy, memset)

extern Icon* InsertIcon(short x, short y, unsigned short group, SpriteRec* s); /* 0x0046d6c0 */
extern Icon* FindIcon(unsigned short group);                    /* 0x0046d630 */
extern void  MoveIcons(int mask, short group, short dx, short dy); /* 0x0046dcd0 */
extern void  ReferenceSprite(SpriteRec* s);                     /* 0x00497bb0 */
extern Icon* AddGBarClassIcon(void* owner, ObjDef* d, int x, int y, int group,
                              short f16);                       /* 0x0046f690 */
extern int   PrintSprite(SpriteRec* s, int x, int y, int mode, BlitCtx* ctx); /* 0x004853a0 */
extern void  PrintCent(int x, int y, int w, const char* text, int font); /* 0x00454e60 */
extern char* GetString(int id);                                 /* 0x00498f50 */
extern void  PrintCachedText(const char* text, int x, int y, int w, int h,
                             int f1, int f2, int ink, int paper); /* 0x00455e50 */
extern void  InitPopUpTools(IconInputFn ok_fn, IconInputFn close_fn); /* 0x00470950 */
extern void  UnloadPopUpTools(void);                            /* 0x00470b00 */
extern void  ResetToolIcons(void);                              /* 0x00471d60 */
extern void* HeapAlloc_w(unsigned int size);                    /* 0x0049e4ff */
extern int   SaveGameWrite(const void* buf, unsigned int n);    /* 0x0047d760 */
extern int   SaveGameRead(void* buf, unsigned int n);           /* 0x0047d730 */
extern LLElem* ElemID(const char* name);                        /* 0x0047b3f0 */
__declspec(dllimport) int __stdcall IntersectRect(WinRect* dst, const WinRect* a,
                                                  const WinRect* b); /* [0x4ab2a0] */

/* ------------------------------------------------------- tiny leaf stubs -- */

/* tinystubs.c's SetHelpFaceTalking (0x0046d3a0) sets the same global to 4;
 * this is its dead sibling for state 6. */
// FUNCTION: LEGOLAND 0x0046d3b0
void SetHelpFaceState6(void) { g_help_face_state = 6; }

// FUNCTION: LEGOLAND 0x004755b0
char Unref_4755b0(void) { return 1; }

// FUNCTION: LEGOLAND 0x004761e0
void Unref_4761e0(void) {}

// FUNCTION: LEGOLAND 0x00476220
char Unref_476220(void) { return 1; }

// FUNCTION: LEGOLAND 0x00476230
char Unref_476230(void) { return 1; }

// FUNCTION: LEGOLAND 0x00476240
char Unref_476240(void) { return 1; }

/* Is either Control key down? Same shape as sysstubs.c's IsLShiftDown /
 * IsRShiftDown, but folded into one test-either function. */
// FUNCTION: LEGOLAND 0x00474090
int IsCtrlDown(void)
{
    if (g_left_ctrl & 0x80)
        return 1;
    return g_right_ctrl >> 7;
}

/* Clear the "last shown" tick of every message in the 17-record table, so
 * every message may be shown again immediately. */
// FUNCTION: LEGOLAND 0x004735c0
void ResetMessageTimers(void)
{
    int i;

    for (i = 0; i < 17; i++)
        g_messages[i].last = 0;
}

/* Copy the cursor-error message's text into `out`. */
// FUNCTION: LEGOLAND 0x00473560
void GetCursorErrorText(char* out, int err)
{
    strcpy(out, GetString(g_messages[g_cursor_error_msg[err]].topic));
}

/* ------------------------------------------------ the labelled-icon pair -- */

/* The render callback 0x0046d800 installs: the icon's sprite, then its
 * caption centred in a 0x40-wide box at the icon origin plus the caption
 * offset held at +0x20/+0x22. */
// FUNCTION: LEGOLAND 0x0046dfd0
int RenderLabelledIcon(Icon* g)
{
    BlitCtx ctx;

    ctx.kind = 2;
    ctx.owner.p = g;
    ctx.owner.n = 0;
    if (g->sprite)
        PrintSprite(g->sprite, g->x, g->y, 0, &ctx);
    PrintCent(g->x + g->cap_dx, g->y + g->cap_dy, 0x40, g->text, 1);
    return 0;
}

/* Build an icon with a caption underneath it. */
// FUNCTION: LEGOLAND 0x0046d800
Icon* AddLabelledIcon(SpriteRec* s, int x, int y, short cap_dx, short cap_dy,
                      int group, char* text)
{
    Icon* p = InsertIcon(x, y, group, s);
    if (p) {
        p->text = text;
        p->cap_dx = cap_dx;
        p->cap_dy = cap_dy;
        p->render = RenderLabelledIcon;
        p->flags |= 0x208;
    }
    return p;
}

/* Move every icon of `group` by (dx, dy) and pull the group's clip icon
 * (group + 2) the other way, so its clip origin stays put. */
// FUNCTION: LEGOLAND 0x0046de10
void MoveIconGroupWithClip(int group, int dx, int dy)
{
    Icon* p;

    MoveIcons(0xffff, group, dx, dy);
    p = FindIcon(group + 2);
    if (p) {
        p->clip_x -= (short)dx;
        p->clip_y -= (short)dy;
    }
}

/* ------------------------------------------- the clipped class-icon pair -- */

/* The render callback 0x0046f860 installs over AddGBarClassIcon's: draw the
 * icon's sprite only when the icon's box still meets the owning widget's
 * rectangle (the 16-byte WinRect at widget +0x1c). Note the sprite test is
 * the SECOND half of the guard, so a null sprite still costs the intersect.
 * The bounds are built exactly as uimisc.c's GetIconBounds builds them. */
// FUNCTION: LEGOLAND 0x0046ea10
int RenderClippedClassIcon(Icon* g)
{
    BlitCtx ctx;
    WinRect box;
    WinRect out;

    ctx.kind = 2;
    ctx.owner.p = g;
    ctx.owner.n = 0;
    box.left = g->x;
    box.top = g->y;
    box.right = g->w + g->x;
    box.bottom = g->h + g->y;
    if (IntersectRect(&out, &box, (WinRect*)((char*)g->widget + 0x1c))
        && g->sprite)
        PrintSprite(g->sprite, g->x, g->y, 0, &ctx);
    return 0;
}

/* An icon for a build class, captioned with the class's name and centred
 * under the class's own icon sprite (w/2 across, h+2 down). The sprite is
 * referenced BEFORE the builder takes its own reference, so the class's
 * sprite outlives the icon. */
// FUNCTION: LEGOLAND 0x0046f5e0
Icon* AddLabelledClassIcon(ObjDef* d, int x, int y, int group, short f16)
{
    Icon* p;
    short w;
    short h;

    ReferenceSprite(d->icon);
    w = d->icon->w;
    h = d->icon->h;
    p = AddLabelledIcon(d->icon, x, y, w / 2, h + 2, group, d->name);
    if (p) {
        p->flags |= 0x1000;
        p->data = d;
        p->f16 = f16;
        if (g_group_cb0) {
            p->f24 = g_group_cb0;
            p->flags |= 4;
        }
        if (g_group_cb1) {
            p->render = (int (*)(Icon*))g_group_cb1;
            p->flags |= 8;
        }
        if (g_group_cb2) {
            p->input = (IconInputFn)g_group_cb2;
            p->flags |= 2;
        }
    }
    return p;
}

/* A class icon that clips itself to its owning widget: AddGBarClassIcon with
 * its render callback swapped. ORIGINAL BUG: the returned icon is stored
 * through without a null check, so an allocation failure faults here (every
 * other builder in this family guards). Reproduced. */
// FUNCTION: LEGOLAND 0x0046f860
Icon* AddClippedClassIcon(void* owner, ObjDef* d, int x, int y, int group,
                          int f16)
{
    Icon* p = AddGBarClassIcon(owner, d, x, y, group, f16);
    p->render = RenderClippedClassIcon;
    return p;
}

/* ------------------------------------------------- the second message bar -- */

/* Open the message bar: keep its own copy of the caption, remember where it
 * goes, build the two control-bar gadgets with the caller's handlers and
 * arm the renderer below. */
// FUNCTION: LEGOLAND 0x00473680
void OpenMessageBar(int x, int y, const char* text, IconInputFn ok_fn,
                    IconInputFn close_fn)
{
    strcpy(g_msgbar_text, text);
    g_msgbar_x = x;
    g_msgbar_y = y;
    InitPopUpTools(ok_fn, close_fn);
    g_msgbar_active = 1;
}

// FUNCTION: LEGOLAND 0x004736e0
void CloseMessageBar(void)
{
    UnloadPopUpTools();
    g_msgbar_active = 0;
}

/* Paint the message bar. Same three-slice strip, the same two gadgets and
 * the same leave-the-strip hit test as popup2.c's DrawPopUpExtra, but with
 * its own placement globals and its own caption buffer -- and no
 * DisablePopUpInputs at the end. */
// FUNCTION: LEGOLAND 0x004736f0
void DrawMessageBar(void)
{
    BlitCtx ctx;
    int     x, y, i;
    struct { int left, top, right, bottom; } box;

    if (!g_msgbar_active)
        return;

    ctx.kind = 1;
    memset(&ctx.owner, 0, sizeof(ctx.owner));
    y = g_msgbar_y;
    x = g_msgbar_x;
    PrintSprite(g_cb_bg[0], x, y, 0, &ctx);
    x += 0x7a;
    for (i = 0; i < g_msgbar_cells; i++) {
        PrintSprite(g_cb_bg[1], x, y, 0, &ctx);
        x += 0x20;
    }
    PrintSprite(g_cb_bg[2], x, y, 0, &ctx);
    x += 0x4e;

    g_cb_icon_ok->flags &= 0xfffffbff;
    g_cb_icon_ok->x = (short)(x - 0x4b);
    g_cb_icon_ok->y = (short)(g_msgbar_y + 3);
    g_cb_icon_close->flags &= 0xfffffbff;
    g_cb_icon_close->x = (short)(x - 0x27);
    g_cb_icon_close->y = (short)(g_msgbar_y + 3);

    box.left = g_msgbar_x + 0xc;
    box.top = g_msgbar_y + 6;
    box.right = box.left + g_msgbar_cells * 20 + 0x7a;
    box.bottom = box.top + 0x1b;
    PrintCachedText(g_msgbar_text, box.left, box.top, box.right - box.left,
                    box.bottom - box.top, 1, 5, 0xff0000, 0xffffff);

    box.bottom = y + 0x1b;
    box.left = g_cb_icon_ok->x;
    box.right = g_cb_icon_close->x + 0x24;
    if (box.right < g_input_point.x || g_input_point.x < box.left)
        ResetToolIcons();
    if (box.bottom < g_input_point.y || g_input_point.y < y)
        ResetToolIcons();
}

/* ---------------------------------------- the research list's save chunk -- */

/* Write the research list: the node count, then per node the length of its
 * class element's name, the name itself (unterminated) and the node's keep
 * flag. */
// FUNCTION: LEGOLAND 0x00476250
void SaveResearchList(void)
{
    int      count;
    int      len;
    ObjNode* p;

    count = 0;
    for (p = g_research_list; p; p = p->next)
        count++;
    SaveGameWrite(&count, 4);
    for (p = g_research_list; p; p = p->next) {
        len = strlen(p->obj->elem->name);
        SaveGameWrite(&len, 4);
        SaveGameWrite(p->obj->elem->name, len);
        SaveGameWrite(&p->keep, 4);
    }
}

/* Read the research list back. The name is read into a 512-byte stack buffer
 * and NUL-terminated at the stored length, looked up with ElemID, and the
 * class it resolves to gets flag 0x04000000 cleared and 0x08000000 set. */
// FUNCTION: LEGOLAND 0x004762f0
void LoadResearchList(void)
{
    int      n;
    int      len;
    char     name[0x200];
    ObjNode* p;

    p = 0;
    g_research_list = 0;
    SaveGameRead(&n, 4);
    while (n-- != 0) {
        if (p) {
            p->next = (ObjNode*)HeapAlloc_w(0xc);
            p = p->next;
        } else {
            p = (ObjNode*)HeapAlloc_w(0xc);
            g_research_list = p;
        }
        SaveGameRead(&len, 4);
        SaveGameRead(name, len);
        name[len] = 0;
        p->obj = ElemID(name)->def;
        p->obj->flags1c &= 0xfbffffff;
        p->obj->flags1c |= 0x8000000;
        SaveGameRead(&p->keep, 4);
    }
    if (p)
        p->next = 0;
}
