/* LEGOLAND — the free-play bar, the interface icon panels and icon focus.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field offsets, callee arg counts and global addresses are load-bearing;
 * names are ours.
 */
#include "legoland.h"

void* memset(void*, int, unsigned int);
#pragma intrinsic(memset)

/* ---- sprites ------------------------------------------------------------ */
/* SpriteRec as sprite2.c documents it: image record @+0x08, w/h @+0x14/16. */
typedef struct ImageRec {
    void*          lls;       /* +0x00 decoded bitmap / LLS record */
    void*          pal;       /* +0x04 */
    short          w;         /* +0x08 */
    short          h;         /* +0x0a */
    unsigned short refcount;  /* +0x0c */
    char           kind;      /* +0x0e */
    char           pad0f;     /* +0x0f */
    char*          name;      /* +0x10 */
    int            type;      /* +0x14 2/3 = animated */
} ImageRec;

typedef struct SpriteRec {
    struct SpriteRec* next;   /* +0x00 */
    void*             surface;/* +0x04 */
    ImageRec*         image;  /* +0x08 */
    int               detail; /* +0x0c */
    int               flags;  /* +0x10 */
    short             w;      /* +0x14 */
    short             h;      /* +0x16 */
    short             src_x;  /* +0x18 */
    short             src_y;  /* +0x1a */
    unsigned short    refs;   /* +0x1c */
    short             pad1e;  /* +0x1e */
} SpriteRec;

/* ---- the icon record ---------------------------------------------------- */
/* Same 0x40-byte record as iconui.c / panelui.c. The two slots at +0x18 and
 * +0x1c are polymorphic: the panel code keeps a small "kind" byte (1 = scroll
 * up, 2 = scroll down, 0xa = bar) or a 16-bit colour at +0x18, the class icons
 * keep an owner pointer there; +0x1c is a text pointer for the class icons,
 * a per-item value for the free-play icons, and AddGBarClassIcon pokes a
 * BYTE into it (see the note there). */
typedef struct Icon {
    struct Icon*     next;      /* +0x00 */
    SpriteRec*       sprite;    /* +0x04 */
    void*            data;      /* +0x08 object descriptor */
    short            x;         /* +0x0c */
    short            y;         /* +0x0e */
    short            w;         /* +0x10 */
    short            h;         /* +0x12 */
    unsigned short   group;     /* +0x14 */
    short            f16;       /* +0x16 */
    union {
        char         kind;      /* +0x18 */
        short        colour;    /* +0x18 */
        void*        owner;     /* +0x18 */
    } u18;
    union {
        char*        text;      /* +0x1c */
        char         kind;      /* +0x1c */
        int          value;     /* +0x1c */
    } u1c;
    int              f20;       /* +0x20 */
    void*            f24;       /* +0x24 group callback 0 */
    int            (*render)(struct Icon*);                      /* +0x28 */
    char           (*input)(struct Icon*, int ev, short dx, short dy); /* +0x2c */
    void*            widget;    /* +0x30 */
    unsigned int     flags;     /* +0x34 */
    char*            help;      /* +0x38 */
    int              help_id;   /* +0x3c */
} Icon;

typedef char (*IconInputFn)(Icon*, int ev, short dx, short dy);

/* The 5th PrintSprite argument, as in money.c: {kind, owner, 0}. The payload
 * is a nested 8-byte struct so the list renderers can clear it as one block
 * (`xor eax,eax` + two stores); the per-icon renderers set the fields. */
typedef struct BlitCtx {
    int kind;                            /* +0x00 */
    struct { Icon* p; int n; } owner;    /* +0x04, +0x08 */
} BlitCtx;

/* The bare 16-byte clip rectangle SetClipping takes (money.c's ClipRect). */
typedef struct ClipRect {
    int left;    /* +0x00 */
    int top;     /* +0x04 */
    int right;   /* +0x08 */
    int bottom;  /* +0x0c */
} ClipRect;

/* ---- object descriptors ------------------------------------------------- */
/* The 0xd0-byte ObjDef LLIDB_LoadODFData builds (llidb_odf.c). Only the
 * fields the icon builders read are named. */
typedef struct ObjDef {
    char     pad00[0x58];
    LLElem*  parent;      /* +0x58 the parent class's LLIDB element */
    char     pad5c[0x68 - 0x5c];
    SpriteRec* icon;      /* +0x68 */
    char     pad6c[0x78 - 0x6c];
    char*    name;        /* +0x78 */
    char*    name2;       /* +0x7c */
    char     pad80[0xc4 - 0x80];
    LLElem*  elem;        /* +0xc4 this class's own LLIDB element */
} ObjDef;

/* A free-play item descriptor as AddFreePlayIcon reads it. */
typedef struct FreePlayItem {
    int        f00;       /* +0x00 */
    int        value;     /* +0x04 -> icon +0x1c */
    void*      data;      /* +0x08 -> icon +0x08 */
    int        f0c;       /* +0x0c */
    char*      text;      /* +0x10 -> icon help */
    SpriteRec* sprite;    /* +0x14 */
} FreePlayItem;

/* The 12-byte node of the object / research lists (listdel.c's ListNode with
 * its payload): next @0, the ObjDef @4, a flag @8. */
typedef struct ObjNode {
    struct ObjNode* next;   /* +0x00 */
    ObjDef*         obj;    /* +0x04 */
    int             keep;   /* +0x08 */
} ObjNode;

/* The scrolling object-list panel RedrawObjectList moves: the icon group at
 * +0x00, the panel's current top/bottom at +0x10/+0x18 and the scroll limits
 * at +0x20/+0x28. */
typedef struct ObjListPanel {
    unsigned short group;   /* +0x00 */
    short          pad02;
    char           pad04[0x10 - 0x04];
    int            top;     /* +0x10 */
    int            pad14;
    int            bottom;  /* +0x18 */
    int            pad1c;
    int            top_max; /* +0x20 */
    int            pad24;
    int            bottom_min; /* +0x28 */
} ObjListPanel;

/* The mouse point @ 0x00813a44 (iconui.c's g_gfx_point) read as 16-bit
 * halves: the focus code does the deltas in 16-bit arithmetic. */
typedef struct MousePt {
    short x;      /* +0x00 */
    short pad02;
    short y;      /* +0x04 */
    short pad06;
} MousePt;

/* ---- globals ------------------------------------------------------------ */
extern Icon*     g_side_icons;          /* 0x006687c8 icon list head */
extern Icon*     g_side_icons2;         /* 0x006687cc second icon list head */
extern Icon*     g_focussed_icon;       /* 0x006687d0 icon under the mouse */
extern IconInputFn g_icon_handler1;     /* 0x006687bc */
extern IconInputFn g_icon_handler2;     /* 0x006687c0 */

extern SpriteRec* g_gbar_item_sprite;   /* 0x00668828 */
extern SpriteRec* g_scrollup_sprite;    /* 0x0066882c */
extern SpriteRec* g_scrolldown_sprite;  /* 0x00668830 */
extern SpriteRec* g_scrollbar_sprite;   /* 0x00668834 */
extern void*      g_group_cb0;          /* 0x006688a8 -> icon +0x24, flag 0x04 */
extern void*      g_group_cb1;          /* 0x006688ac -> render, flag 0x08 */
extern void*      g_group_cb2;          /* 0x006688b0 -> input, flag 0x02 */
extern int        g_scroll_flags;       /* 0x006688b8 bit0 = up lit, bit1 = down lit */
extern int        g_icon_scroll_tick;   /* 0x006688c8 */

extern ObjNode*   g_object_list;        /* 0x00668e40 */
extern int        g_object_list_mode;   /* 0x00668e34 */
extern ObjNode*   g_research_list;      /* 0x00668ed8 */

extern int        g_mouse_btn_a;        /* 0x00813ad4 */
extern int        g_mouse_ev;           /* 0x00813adc */
extern int        g_mouse_ev2;          /* 0x00813ac4 */
extern MousePt    g_mouse;              /* 0x00813a44 */
extern int        g_drag_lock;          /* 0x00668954 */
extern int        g_icon_clicked;       /* 0x00667c48 */

extern int        g_freeplay_progress;  /* 0x007cb3a0 */
extern SpriteRec* g_freeplay_marker;    /* 0x007cb398 */
extern SpriteRec* g_fp_spr0;            /* 0x007cb3b4 */
extern SpriteRec* g_fp_spr1;            /* 0x007cb3ac */
extern SpriteRec* g_fp_spr2;            /* 0x007cb3b0 */
extern SpriteRec* g_fp_spr3;            /* 0x007cb3a8 */
extern SpriteRec* g_fp_spr4;            /* 0x007cb3c4 */
extern SpriteRec* g_fp_spr5;            /* 0x007cb3c0 */
extern SpriteRec* g_fp_spr6;            /* 0x007cb3cc */
extern SpriteRec* g_fp_spr7;            /* 0x007cb3c8 */
extern SpriteRec* g_fp_spr9;            /* 0x007cb3d4 */

/* Theme icons (Load/UnLoad_Interface_ThemeIcons). */
extern int        g_theme_icons_loaded; /* 0x00668eb4 */
extern SpriteRec* g_theme_legoland_on;  /* 0x007fdcc0 */
extern SpriteRec* g_theme_legoland_off; /* 0x007fdd40 */
extern SpriteRec* g_theme_castle_on;    /* 0x007fdcc8 */
extern SpriteRec* g_theme_castle_off;   /* 0x007fdd48 */
extern SpriteRec* g_theme_western_on;   /* 0x007fdcc4 */
extern SpriteRec* g_theme_western_off;  /* 0x007fdd44 */
extern SpriteRec* g_theme_adv_on;       /* 0x007fdccc */
extern SpriteRec* g_theme_adv_off;      /* 0x007fdd4c */

/* Control icons (Load/UnLoad_Interface_ControlIcons). */
extern int        g_control_icons_loaded;  /* 0x00668ea4 */
extern SpriteRec* g_ci_interface_bg;       /* 0x00668e68 InterfaceBG.lls */
extern SpriteRec* g_ci_no_energy;          /* 0x00668e6c No_Energy.lls */
extern SpriteRec* g_ci_bar_pointer;        /* 0x00668e70 Bar_pointer.lls */
extern SpriteRec* g_ci_path_pressed;       /* 0x007fdcd0 IF_PathIconPressed.lls */
extern SpriteRec* g_ci_path;               /* 0x007fdd50 IF_PathIcon.lls */
extern SpriteRec* g_ci_query_pressed;      /* 0x007fdcd4 IF_QueryIconPressed.lls */
extern SpriteRec* g_ci_query;              /* 0x007fdd54 IF_Queryicon.lls */
extern SpriteRec* g_ci_eraser_pressed;     /* 0x007fdcd8 IF_EraserIconPressed.lls */
extern SpriteRec* g_ci_eraser;             /* 0x007fdd58 IF_EraserIcon.lls */
extern SpriteRec* g_ci_map_pressed;        /* 0x007fdcdc IF_MapIconPressed.lls */
extern SpriteRec* g_ci_map;                /* 0x007fdd5c IF_Mapicon.lls */
extern SpriteRec* g_ci_options_pressed;    /* 0x007fdce0 IF_OptionsIconPressed.lls */
extern SpriteRec* g_ci_options;            /* 0x007fdd60 IF_OptionsIcon.lls */
extern SpriteRec* g_ci_attract_hl_on;      /* 0x00668e74 Attract_Highlight_On.lls */
extern SpriteRec* g_ci_attract_hl_off;     /* 0x00668e78 Attract_Highlight_Off.lls */
extern SpriteRec* g_ci_attract_new_off;    /* 0x00668e7c Attract_New_Off.lls */
extern SpriteRec* g_ci_attract_new_on;     /* 0x00668e80 Attract_New_On.lls */
extern SpriteRec* g_ci_scrolldown_lit;     /* 0x00668e84 Side_ScrollDown_Lit.lls */
extern SpriteRec* g_ci_scrollup_lit;       /* 0x00668e88 Side_ScrollUp_Lit.lls */
extern SpriteRec* g_ci_link_middle;        /* 0x00668e8c Link_Middle.lls */
extern SpriteRec* g_ci_link_bottom;        /* 0x00668e90 Link_Bottom.lls */
extern SpriteRec* g_ci_brief2;             /* 0x00668e94 BriefIcon2.lls */
extern SpriteRec* g_ci_brief;              /* 0x00668e98 BriefIcon.lls */
extern SpriteRec* g_ci_script_end;         /* 0x00668ea0 ScriptEnd.lls */
/* The 16-byte control-icon state block @ 0x007fdd70, cleared as a unit (the
 * original's `xor eax,eax` + four dword stores is an unrolled memset). */
typedef struct CtrlIconState {
    int f00;    /* +0x00 */
    int f04;    /* +0x04 */
    int f08;    /* +0x08 */
    int f0c;    /* +0x0c */
} CtrlIconState;
extern CtrlIconState g_ci_state;           /* 0x007fdd70 */

/* Sprite names (.rdata). */
extern char g_lls_legoland_on[];       /* 0x004bb464 "legoland_themeON.lls" */
extern char g_lls_legoland_off[];      /* 0x004bb44c "legoland_themeOFF.lls" */
extern char g_lls_castle_on[];         /* 0x004bb438 "castle_themeON.lls" */
extern char g_lls_castle_off[];        /* 0x004bb424 "castle_themeOFF.lls" */
extern char g_lls_western_on[];        /* 0x004bb410 "western_themeON.lls" */
extern char g_lls_western_off[];       /* 0x004bb3f8 "western_themeOFF.lls" */
extern char g_lls_adv_on[];            /* 0x004bb3e0 "adventurers_themeON.lls" */
extern char g_lls_adv_off[];           /* 0x004bb3c4 "adventurers_themeOFF.lls" */

extern char g_lls_interface_bg[];      /* 0x004bb3b4 */
extern char g_lls_no_energy[];         /* 0x004bb3a4 */
extern char g_lls_bar_pointer[];       /* 0x004bb394 */
extern char g_lls_path_pressed[];      /* 0x004bb37c */
extern char g_lls_path[];              /* 0x004bb36c */
extern char g_lls_query_pressed[];     /* 0x004bb354 */
extern char g_lls_query[];             /* 0x004bb340 */
extern char g_lls_eraser_pressed[];    /* 0x004bb324 */
extern char g_lls_eraser[];            /* 0x004bb310 */
extern char g_lls_map_pressed[];       /* 0x004bb2f8 */
extern char g_lls_map[];               /* 0x004bb2e8 */
extern char g_lls_options_pressed[];   /* 0x004bb2cc */
extern char g_lls_options[];           /* 0x004bb2b8 */
extern char g_lls_attract_hl_on[];     /* 0x004bb29c */
extern char g_lls_attract_hl_off[];    /* 0x004bb280 */
extern char g_lls_attract_new_off[];   /* 0x004bb26c */
extern char g_lls_attract_new_on[];    /* 0x004bb258 */
extern char g_lls_scrolldown_lit[];    /* 0x004bb240 */
extern char g_lls_scrollup_lit[];      /* 0x004bb228 */
extern char g_lls_link_middle[];       /* 0x004bb218 */
extern char g_lls_link_bottom[];       /* 0x004bb208 */
extern char g_lls_brief2[];            /* 0x004bb1f8 */
extern char g_lls_brief[];             /* 0x004bb1e8 */
extern char g_lls_script_end[];        /* 0x004bb1d8 */

/* ---- callees ------------------------------------------------------------ */
extern void*      HeapAlloc_w(unsigned int size);                 /* 0x0049e4ff */
extern void       HeapFree_w(void* p);                            /* 0x0049e4d0 */
extern SpriteRec* LoadSprite(const char* name, int mode);         /* 0x00497ab0 */
extern void       KillSprite(SpriteRec* s);                       /* 0x00497bd0 */
extern char*      GetString(int id);                              /* 0x00498f50 */
extern int        GetTicks(void);                                 /* 0x00499450 */
extern int        GetBlink(void);                                 /* 0x00499480 */
extern int        GetNearestColour(int r, int g, int b);          /* 0x0044e6c0 */
extern void       RenderThickBox(int x, int y, int w, int h, int t, int colour); /* 0x00489390 */
extern int        PrintSprite(SpriteRec* s, int x, int y, int mode, BlitCtx* ctx); /* 0x004853a0 */
extern void       StoreClipping(void);                            /* 0x0048a660 */
extern void       SetClipping(ClipRect* r);                       /* 0x0048a5c0 */
extern void       RestoreClipping(void);                          /* 0x0048a690 */
extern void       LLSPlayOnce(void* lls, ImageRec* img);          /* 0x0047d580 */
extern int        GetObjCost(ObjDef* d);                          /* 0x00480da0 */
static __inline int ObjCost(ObjDef* d) { return GetObjCost(d); }

extern Icon*      InsertIcon(short x, short y, unsigned short group, SpriteRec* s); /* 0x0046d6c0 */
extern void       SetIconSprite(Icon* p, SpriteRec* s);           /* 0x0046d680 */
extern void       AddFullScreenIcon(int group);                   /* 0x0046d760 */
extern int        RenderFullScreenIcon(Icon* p);                  /* 0x0046df60 */
extern int        RenderBoxIcon(Icon* p);                         /* 0x0046df70 */
extern int        RenderGBarSpriteIcon(Icon* p);                  /* 0x0046e850 */
extern char       ScrollUpInput(Icon* p, int ev, short dx, short dy);   /* 0x0046d980 */
extern char       ScrollDownInput(Icon* p, int ev, short dx, short dy); /* 0x0046da20 */
extern int        RenderGBarSprite(Icon* p);                      /* 0x0046e9d0 */
extern char       DefaultIconInput(Icon* p, int ev, short dx, short dy); /* 0x0046f2e0 */
extern void       RemoveObjectListIcons(int group);               /* 0x0046fb40 */
extern void       RemoveFreePlayList(int group);                  /* 0x0048b4a0 */
extern void       UpdateSidePanelScroll(int step);                /* 0x0046ec50 */
extern void       RenderIconsExtra(void);                         /* 0x004760a0 */
extern void       FreePlayItemUpdate(int value, int a);           /* 0x0048a840 */
extern int        FreePlayItemAvailable(int value, int b);        /* 0x0048aef0 */
extern void       InsertObjectNode(ObjDef* d);                    /* 0x004755c0 */
extern void       MoveIcons(int mask, short group, short dx, short dy); /* 0x0046dcd0 */

int  RenderScroll_Icons(Icon* p);
Icon* AddGBarClassIcon(void* owner, ObjDef* d, int x, int y, int group, short f16);

/* ---- lists -------------------------------------------------------------- */

/* Drop the FIRST research node whose keep flag is clear. Note the head is
 * dereferenced unguarded: the list must be non-empty. */
// FUNCTION: LEGOLAND 0x004763d0
void CleanUpReseachList(void)
{
    ObjNode* p = g_research_list;
    ObjNode* prev;

    if (p->keep == 0) {
        g_research_list = p->next;
        HeapFree_w(p);
        return;
    }
    prev = p;
    for (p = p->next; p; p = p->next) {
        if (p->keep == 0) {
            prev->next = p->next;
            HeapFree_w(p);
            return;
        }
        prev = p;
    }
}

/* ---- theme icons -------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004745c0
void Load_Interface_ThemeIcons(void)
{
    if (!g_theme_icons_loaded) {
        g_theme_icons_loaded = 1;
        g_theme_legoland_on  = LoadSprite(g_lls_legoland_on, 4);
        g_theme_legoland_off = LoadSprite(g_lls_legoland_off, 4);
        g_theme_castle_on    = LoadSprite(g_lls_castle_on, 4);
        g_theme_castle_off   = LoadSprite(g_lls_castle_off, 4);
        g_theme_western_on   = LoadSprite(g_lls_western_on, 4);
        g_theme_western_off  = LoadSprite(g_lls_western_off, 4);
        g_theme_adv_on       = LoadSprite(g_lls_adv_on, 4);
        g_theme_adv_off      = LoadSprite(g_lls_adv_off, 4);
    }
}

// FUNCTION: LEGOLAND 0x00474670
void UnLoad_Interface_ThemeIcons(void)
{
    if (g_theme_icons_loaded) {
        g_theme_icons_loaded = 0;
        if (g_theme_legoland_on)  { KillSprite(g_theme_legoland_on);  g_theme_legoland_on  = 0; }
        if (g_theme_legoland_off) { KillSprite(g_theme_legoland_off); g_theme_legoland_off = 0; }
        if (g_theme_castle_on)    { KillSprite(g_theme_castle_on);    g_theme_castle_on    = 0; }
        if (g_theme_castle_off)   { KillSprite(g_theme_castle_off);   g_theme_castle_off   = 0; }
        if (g_theme_western_on)   { KillSprite(g_theme_western_on);   g_theme_western_on   = 0; }
        if (g_theme_western_off)  { KillSprite(g_theme_western_off);  g_theme_western_off  = 0; }
        if (g_theme_adv_on)       { KillSprite(g_theme_adv_on);       g_theme_adv_on       = 0; }
        if (g_theme_adv_off)      { KillSprite(g_theme_adv_off);      g_theme_adv_off      = 0; }
    }
}

/* ---- the free-play bar -------------------------------------------------- */

/* The free-play time bar: the bar sprite clipped to progress/20000 of its
 * width (g_freeplay_progress runs 0..20000). The two-step float local is
 * load-bearing: a single `(int)(w * c * p)` expression makes VC6 reassociate
 * the product to (w * p) * c and fold a negation into the constant
 * (`sub edi,eax` against -5e-5); the original keeps w * c first. */
// FUNCTION: LEGOLAND 0x0046e7b0
int RenderFreePlayBar(Icon* g)
{
    BlitCtx  ctx;
    ClipRect clip;
    float    f;

    ctx.kind = 2;
    ctx.owner.p = g;
    ctx.owner.n = 0;
    StoreClipping();
    f = g->w * 0.00005f;
    f = f * g_freeplay_progress;
    clip.left   = g->x;
    clip.right  = clip.left + (int)f;
    clip.top    = g->y;
    clip.bottom = clip.top + g->h;
    SetClipping(&clip);
    if (g->sprite)
        PrintSprite(g->sprite, g->x, g->y, 0, &ctx);
    RestoreClipping();
    return 0;
}

/* ---- the object-list panel ---------------------------------------------- */

/* Scroll the panel's icons by `dy` (clamped to the panel's limits) and `dx`. */
// FUNCTION: LEGOLAND 0x004758e0
void RedrawObjectList(ObjListPanel* l, int dx, int dy)
{
    if (dy == 0) {
        MoveIcons(0xffff, l->group, 0, 0);
        return;
    }
    if (l->top + dy > l->top_max)
        dy = l->top_max - l->top;
    else if (l->bottom + dy < l->bottom_min)
        dy = l->bottom_min - l->bottom;
    l->top += dy;
    l->bottom += dy;
    MoveIcons(0xffff, l->group, dx, dy);
}

/* ---- icon builders ------------------------------------------------------ */

/* A free-play item icon on the bar. `unused` is never read. The two arms of
 * the sprite test call the same helper; VC6 cross-jumps them into a single
 * call with a conditional push. */
// FUNCTION: LEGOLAND 0x0046f7a0
Icon* AddFreePlayIcon(void* unused, FreePlayItem* d, int x, int y, int group,
                      short f16, int f20)
{
    Icon* p = InsertIcon(x, y, group, g_gbar_item_sprite);
    if (p) {
        if (d->sprite)
            SetIconSprite(p, d->sprite);
        else
            SetIconSprite(p, 0);
        p->render = RenderGBarSpriteIcon;
        p->help = d->text;
        p->help_id = -1;
        p->flags |= 0x3008;
        p->f16 = f16;
        p->data = d->data;
        p->u18.kind = 0;
        p->u1c.value = d->value;
        p->f20 = f20;
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

/* An object-class icon on the bar. The help text is the class's own name for
 * DLL-backed classes (LLIDB type flag 0x10000) and the second string
 * otherwise; flag 0x20000 pokes a BYTE into +0x1c, clobbering the low byte
 * of the pointer just stored there -- reproduced as the original does it. */
// FUNCTION: LEGOLAND 0x0046f690
Icon* AddGBarClassIcon(void* owner, ObjDef* d, int x, int y, int group, short f16)
{
    Icon* p = InsertIcon(x, y, group, g_gbar_item_sprite);
    if (p) {
        char* text;
        if (d->icon) {
            short w = d->icon->w;
            short h = d->icon->h;
            SetIconSprite(p, d->icon);
            p->h = h;
            p->w = w;
        } else {
            SetIconSprite(p, 0);
        }
        p->u18.owner = d->name;
        text = (d->elem->type_flags & 0x10000) ? d->name : d->name2;
        p->help = text;
        p->u1c.text = text;
        p->help_id = -1;
        if (d->elem->type_flags & 0x20000)
            p->u1c.kind = 0x14;
        p->widget = owner;
        p->render = RenderGBarSpriteIcon;
        p->flags &= ~0x200;
        p->flags |= 0x3008;
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

/* The scroll-bar frame of a side panel: a full-screen catch-all (group+6),
 * scroll-up (group+3) and scroll-down (group+4) buttons 30 high, and the
 * 121-wide list box (group+5) between them. `unused` is never read. */
// FUNCTION: LEGOLAND 0x0046dbc0
Icon* AddGBarIcons(void* owner, int x, int y, int unused, int h, int group)
{
    Icon* p;

    AddFullScreenIcon(group + 6);
    p = InsertIcon(x, y, group + 3, g_scrollup_sprite);
    p->widget = owner;
    p->help_id = 0x95;
    p->help = GetString(0x95);
    p->h = 0x1e;
    p->flags |= 0x2013;
    p->input = ScrollUpInput;
    p = InsertIcon(x, h + y - 0x1e, group + 4, g_scrolldown_sprite);
    p->widget = owner;
    p->help_id = 0x94;
    p->help = GetString(0x94);
    p->h = 0x1e;
    p->flags |= 0x2013;
    p->input = ScrollDownInput;
    p = InsertIcon(x, y + 0x1e, group + 5, 0);
    p->widget = owner;
    p->w = 0x79;
    p->h = h - 0x3c;
    p->u18.colour = GetNearestColour(0, 0xa, 0x14);
    p->flags |= 0x29;
    p->render = RenderBoxIcon;
    return p;
}

/* The in-game side panel: as AddGBarIcons but with the scroll-bar track
 * (kind 0xa) and lit scroll buttons (kinds 1/2, RenderScroll_Icons). */
// FUNCTION: LEGOLAND 0x0046eaf0
Icon* SetupInterfacePanelIcons(void* owner, int x, int y, int unused, int h, int group)
{
    Icon* p;

    g_scroll_flags = 0;
    AddFullScreenIcon(group + 6);
    p = InsertIcon(x - 3, 0x20, group, g_scrollbar_sprite);
    p->render = RenderGBarSprite;
    p->u18.kind = 0xa;
    p->flags |= 9;
    p = InsertIcon(x - 1, y, group + 3, g_scrollup_sprite);
    p->widget = owner;
    p->help_id = 0x95;
    p->help = GetString(0x95);
    p->w = 0x79;
    p->h = 0x1e;
    p->flags |= 0x201b;
    p->render = RenderScroll_Icons;
    p->input = ScrollUpInput;
    p->u18.kind = 1;
    p = InsertIcon(x - 1, h + y - 0x20, group + 4, g_scrolldown_sprite);
    p->widget = owner;
    p->help_id = 0x94;
    p->help = GetString(0x94);
    p->w = 0x79;
    p->h = 0x1e;
    p->flags |= 0x201b;
    p->render = RenderScroll_Icons;
    p->input = ScrollDownInput;
    p->u18.kind = 2;
    p = InsertIcon(x, y + 0x1e, group + 5, 0);
    p->widget = owner;
    p->w = 0x79;
    p->h = h - 0x3e;
    p->u18.colour = GetNearestColour(0, 0xa, 0x14);
    p->flags |= 0x29;
    p->render = RenderBoxIcon;
    return p;
}

/* ---- icon renderers ----------------------------------------------------- */

/* A side-panel scroll button: its sprite, plus the "lit" overlay for one
 * blink when the matching g_scroll_flags bit is set (bit1 = down, kind 2;
 * bit0 = up, kind 1); the bit is consumed. */
// FUNCTION: LEGOLAND 0x0046e400
int RenderScroll_Icons(Icon* p)
{
    BlitCtx ctx;

    ctx.kind = 2;
    ctx.owner.p = p;
    ctx.owner.n = 0;
    if (p->sprite)
        PrintSprite(p->sprite, p->x, p->y, 0, &ctx);
    if (GetBlink()) {
        if ((g_scroll_flags & 2) && p->u18.kind == 2) {
            PrintSprite(g_ci_scrolldown_lit, p->x, p->y, 0, &ctx);
            g_scroll_flags &= ~2;
        } else if ((g_scroll_flags & 1) && p->u18.kind == 1) {
            PrintSprite(g_ci_scrollup_lit, p->x, p->y, 0, &ctx);
            g_scroll_flags &= ~1;
        }
    }
    return 0;
}

/* A free-play bar item: only drawn inside the bar's 0..300 band. Kind 1 is
 * the header (its sprite plus the marker 70 pixels right); other kinds are
 * items, drawn ghosted (0xff000000) when not yet available. */
// FUNCTION: LEGOLAND 0x0046e300
int RenderFreePlayIcons(Icon* p)
{
    BlitCtx ctx;

    ctx.kind = 2;
    ctx.owner.p = p;
    ctx.owner.n = 0;
    if (p->y < 0 || p->y > 300)
        return 0;
    if (p->u18.kind != 1) {
        if (p->sprite) {
            FreePlayItemUpdate(p->u1c.value, 0);
            if (FreePlayItemAvailable(p->u1c.value, p->f20))
                PrintSprite(p->sprite, p->x, p->y, 0, &ctx);
            else
                PrintSprite(p->sprite, p->x, p->y, 0xff000000, &ctx);
        }
    } else {
        if (p->sprite)
            PrintSprite(p->sprite, p->x, p->y, 0, &ctx);
        PrintSprite(g_freeplay_marker, p->x + 70, p->y, 0, &ctx);
    }
    return 0;
}

/* The second icon list (help / pop-up icons): every enabled icon inside
 * 0 < x < 480, then a green 2-pixel box round the focussed one if it wants
 * it (0x8000). */
// FUNCTION: LEGOLAND 0x0046f200
void RenderHelpIcons(void)
{
    BlitCtx ctx;
    Icon*   p = g_side_icons2;

    ctx.kind = 2;
    memset(&ctx.owner, 0, sizeof(ctx.owner));
    StoreClipping();
    RenderFullScreenIcon(0);
    for (; p; p = p->next) {
        if (p->flags & 0x400)
            continue;
        if (p->x >= 480 || p->x <= 0)
            continue;
        if (p->flags & 8) {
            p->render(p);
        } else if (p->sprite) {
            ctx.owner.p = p;
            PrintSprite(p->sprite, p->x, p->y, 0, &ctx);
        }
        if (p == g_focussed_icon && (p->flags & 0x8000))
            RenderThickBox(p->x, p->y, p->w, p->h, 2, GetNearestColour(0, 0xff, 0));
    }
    RestoreClipping();
}

/* The main icon list: scroll the side panel by the elapsed ticks (5/33 of a
 * pixel per ms, capped at 990 ms), draw every enabled icon, restart the
 * focussed icon's animation and box it if it wants it. */
// FUNCTION: LEGOLAND 0x0046eee0
void RenderIcons(void)
{
    BlitCtx ctx;
    Icon*   p = g_side_icons;
    int     dt;

    ctx.kind = 2;
    memset(&ctx.owner, 0, sizeof(ctx.owner));
    StoreClipping();
    RenderFullScreenIcon(0);
    dt = GetTicks() - g_icon_scroll_tick;
    if (dt > 990)
        dt = 990;
    UpdateSidePanelScroll(dt * 5 / 33);
    g_icon_scroll_tick = GetTicks();
    for (; p; p = p->next) {
        if (p->flags & 0x400)
            continue;
        if (p->flags & 8) {
            p->render(p);
        } else if (p->sprite) {
            ctx.owner.p = p;
            PrintSprite(p->sprite, p->x, p->y, 0, &ctx);
        }
        if (p == g_focussed_icon) {
            if (p->sprite) {
                ImageRec* img = p->sprite->image;
                if (img->type == 3 || img->type == 2)
                    LLSPlayOnce(img->lls, img);
            }
            if (p->flags & 0x8000)
                RenderThickBox(p->x, p->y, p->w, p->h, 2, GetNearestColour(0, 0xff, 0));
        }
    }
    RenderIconsExtra();
    RestoreClipping();
}

/* As RenderIcons for just three icon groups, inside 0 < y < 480, no box. */
// FUNCTION: LEGOLAND 0x0046f010
void RenderIcons2(unsigned short g1, unsigned short g2, unsigned short g3)
{
    BlitCtx ctx;
    Icon*   p = g_side_icons;

    ctx.kind = 2;
    memset(&ctx.owner, 0, sizeof(ctx.owner));
    StoreClipping();
    RenderFullScreenIcon(0);
    for (; p; p = p->next) {
        if (!(p->flags & 0x400) && p->y > 0 && p->y < 480 &&
            (p->group == g1 || p->group == g2 || p->group == g3)) {
            if (p->flags & 8) {
                p->render(p);
            } else if (p->sprite) {
                ctx.owner.p = p;
                PrintSprite(p->sprite, p->x, p->y, 0, &ctx);
            }
        }
        if (p == g_focussed_icon && p->sprite) {
            ImageRec* img = p->sprite->image;
            if (img->type == 3 || img->type == 2)
                LLSPlayOnce(img->lls, img);
        }
    }
    RestoreClipping();
}

/* ---- focus -------------------------------------------------------------- */

/* Route this frame's mouse event: a global handler first (buttons in
 * g_mouse_btn_a), else the focussed icon's own input callback (or the
 * default one) with the mouse offset inside the icon; a button event while
 * dragging, or no button at all, goes through as the secondary event with
 * g_icon_clicked raised. Then the fallback global handler. */
// FUNCTION: LEGOLAND 0x0046f4c0
char CheckFocussedIcon(void)
{
    Icon* p;
    int   ev;

    if (g_icon_handler2 && (g_mouse_btn_a & 7))
        return g_icon_handler2(0, g_mouse_btn_a, 0, 0);
    p = g_focussed_icon;
    if (p) {
        if ((p->flags & 2) && p->input) {
            ev = g_mouse_ev;
            if ((ev & 7) && !g_drag_lock)
                return p->input(p, ev, g_mouse.x - p->x, g_mouse.y - p->y);
            g_icon_clicked = 1;
            ev = g_mouse_ev2;
            return p->input(p, ev, g_mouse.x - p->x, g_mouse.y - p->y);
        }
        ev = g_mouse_ev;
        if (!(ev & 7) || g_drag_lock) {
            ev = g_mouse_ev2;
            g_icon_clicked = 1;
        }
        return DefaultIconInput(p, ev, g_mouse.x - p->x, g_mouse.y - p->y);
    }
    if (g_icon_handler1 && (g_mouse_ev & 7))
        return g_icon_handler1(0, g_mouse_ev, 0, 0);
    return 0;
}

/* ---- the object list ---------------------------------------------------- */

/* Link a child class into the object list after its parent, sorted by cost
 * among the siblings that share that parent. Without a parent on the list
 * the node is dropped again (and, in list mode, the class is added on its
 * own).
 *
 * SEMANTIC FIX (the previous note was wrong): this takes ONE parameter, not two.
 * The `mov [esp+0x1c],edx` spill inside the loop is emitted between a `push` and
 * its `add esp,8`, so it names a frame home 8 bytes LOWER -- [esp+0x14] in the
 * body, which with four prologue pushes and no `sub esp` is the FIRST argument.
 * VC6 is reusing the (by then dead) `d` slot as the spill home for the first
 * GetObjCost result; there is no second argument. The old `int unused` parameter
 * was that misreading (docs/DECOMP.md "READING esp"); both call sites in fpui2.c
 * always passed one argument.
 *
 * WIP at 49/68 (72%), 68/68 instructions, 168B vs 171B, first diverging index 36.
 * Instructions 0..35 (prologue, node fill, parent search, the whole not-found
 * exit) and 56..67 (the walk step and both stores + epilogue) are exact,
 * index for index; only the found-block head and the compare loop differ:
 *
 *   ours   test esi,esi / mov ebp,esi / je notfound   <- two instructions the
 *   orig   mov ebp,esi                                   original does not have
 *   ours   mov ecx,[edi+4] ... push eax / call / mov ebx,eax / mov eax,[edi+4]
 *   orig   mov ebx,[edi+4] ... mov edx,eax / push edx / call / mov edx,eax /
 *          push ebx / mov [esp+0x1c],edx / call / mov ecx,[esp+0x1c]
 *
 * i.e. the original holds n->obj in ebx ACROSS the first call and spills the
 * first cost to d's slot; ours keeps the cost in ebx and reloads n->obj.
 *
 * What was measured this round (all with tools in scratchpad/lists):
 *  - `push ebp` is in the ORIGINAL prologue. VC6 sinks that push to the found
 *    block whenever prev's first definition is there; the ONLY spelling that
 *    keeps it at entry is `prev = 0` before the search loop plus a real
 *    `if (prev)` test after it (a dead `prev = 0`, or `prev = p` inside the
 *    search loop, or a `goto found`, all let it sink again). That test costs
 *    the two extra instructions above -- VC6 cannot prove p != 0 on the break
 *    path, so it emits `test esi,esi / je`. Keeping the sink instead costs a
 *    whole-body index shift (mismatch 67), so this shape is strictly better.
 *  - Holding n->obj in a local (`ObjDef* a = n->obj;`) DOES produce the
 *    original's spill sequence -- `mov edx,eax / push edx / ... /
 *    mov [esp+0x1c],edx / mov edx,[esp+0x1c]` appears verbatim with the
 *    expression form `if (GetObjCost(a) <= GetObjCost(p->obj))` -- but VC6 then
 *    ranks `a` above `n`, so a takes edi and n takes ebx (the original is the
 *    other way round) AND prev loses ebp, which undoes the prologue. ~90
 *    spellings tried: a at loop/function scope, a assigned before/after the
 *    parent compare, a+b locals, both compare operand orders, cost as local /
 *    function-scope / expression temp / two separate c1,c2, the three field
 *    store orders, alloc-before/after the head read, `p = prev->next` stepping,
 *    __inline Cost(ObjNode*), Cost(ObjDef*,ObjDef*), Dearer(node,node) and a
 *    whole-sorted-insert __inline. NONE gives n=edi together with a=ebx.
 *  - Also ruled out: while/for/do-while/for(;;) forms of both loops, goto-found
 *    vs goto-notfound vs single-exit `goto done`, the found body inlined in the
 *    search loop, `ObjNode** link` instead of prev, `if (!p)` after a break
 *    (adds a jmp + test), and `prev = p` at the search-loop top (that keeps the
 *    prologue push but hoists `mov ebp,esi / mov esi,[esi]` into the search
 *    loop: 65 insns, first divergence 17).
 *
 * The sibling InsertObjectNode (0x004755c0, not ours) is the same list insert
 * with `prev = 0` + `if (!prev)` and DOES sink `push ebp` -- so the two
 * functions really do differ in that one source detail, and this shape is the
 * one that reproduces the original's prologue. Variants: scratchpad/lists/. */
// WIP-FUNCTION: LEGOLAND 0x00475630  (72%, 68/68 insns, 168B vs 171B; found-block head + compare-loop register allocation, see note)
void InsertChildIntoList(ObjDef* d)
{
    ObjNode* p = g_object_list;
    ObjNode* prev = 0;
    ObjNode* n = (ObjNode*)HeapAlloc_w(sizeof(ObjNode));
    int cost;

    n->obj = d;
    n->keep = 0;
    n->next = 0;
    for (; p; p = p->next) {
        if (p->obj->elem == d->parent) {
            prev = p;
            break;
        }
    }
    if (prev) {
        p = p->next;
        while (p) {
            if (n->obj->parent != p->obj->parent)
                break;
            cost = GetObjCost(p->obj);
            if (GetObjCost(n->obj) <= cost)
                break;
            prev = p;
            p = p->next;
        }
        prev->next = n;
        n->next = p;
        return;
    }
    if (g_object_list_mode)
        InsertObjectNode(d);
    HeapFree_w(n);
}

/* ---- control icons ------------------------------------------------------ */

// FUNCTION: LEGOLAND 0x004741f0
void Load_Interface_ControlIcons(void)
{
    if (!g_control_icons_loaded) {
        g_control_icons_loaded = 1;
        g_ci_interface_bg    = LoadSprite(g_lls_interface_bg, 4);
        g_ci_no_energy       = LoadSprite(g_lls_no_energy, 4);
        g_ci_bar_pointer     = LoadSprite(g_lls_bar_pointer, 4);
        g_ci_path_pressed    = LoadSprite(g_lls_path_pressed, 4);
        g_ci_path            = LoadSprite(g_lls_path, 4);
        g_ci_query_pressed   = LoadSprite(g_lls_query_pressed, 4);
        g_ci_query           = LoadSprite(g_lls_query, 4);
        g_ci_eraser_pressed  = LoadSprite(g_lls_eraser_pressed, 4);
        g_ci_eraser          = LoadSprite(g_lls_eraser, 4);
        g_ci_map_pressed     = LoadSprite(g_lls_map_pressed, 4);
        g_ci_map             = LoadSprite(g_lls_map, 4);
        g_ci_options_pressed = LoadSprite(g_lls_options_pressed, 4);
        g_ci_options         = LoadSprite(g_lls_options, 4);
        g_ci_attract_hl_on   = LoadSprite(g_lls_attract_hl_on, 4);
        g_ci_attract_hl_off  = LoadSprite(g_lls_attract_hl_off, 4);
        g_ci_attract_new_off = LoadSprite(g_lls_attract_new_off, 4);
        g_ci_attract_new_on  = LoadSprite(g_lls_attract_new_on, 4);
        g_ci_scrolldown_lit  = LoadSprite(g_lls_scrolldown_lit, 4);
        g_ci_scrollup_lit    = LoadSprite(g_lls_scrollup_lit, 4);
        g_ci_link_middle     = LoadSprite(g_lls_link_middle, 4);
        g_ci_link_bottom     = LoadSprite(g_lls_link_bottom, 4);
        g_ci_brief2          = LoadSprite(g_lls_brief2, 4);
        g_ci_brief           = LoadSprite(g_lls_brief, 4);
        g_ci_script_end      = LoadSprite(g_lls_script_end, 4);
    }
}

/* Unconditional KillSprite on all 24 (no null checks, unlike the theme
 * icons); the four control-icon state words are cleared first. */
// FUNCTION: LEGOLAND 0x004743b0
void UnLoad_Interface_ControlIcons(void)
{
    if (g_control_icons_loaded) {
        g_control_icons_loaded = 0;
        memset(&g_ci_state, 0, sizeof(g_ci_state));
        KillSprite(g_ci_interface_bg);    g_ci_interface_bg = 0;
        KillSprite(g_ci_no_energy);       g_ci_no_energy = 0;
        KillSprite(g_ci_bar_pointer);     g_ci_bar_pointer = 0;
        KillSprite(g_ci_path_pressed);    g_ci_path_pressed = 0;
        KillSprite(g_ci_path);            g_ci_path = 0;
        KillSprite(g_ci_query_pressed);   g_ci_query_pressed = 0;
        KillSprite(g_ci_query);           g_ci_query = 0;
        KillSprite(g_ci_eraser_pressed);  g_ci_eraser_pressed = 0;
        KillSprite(g_ci_eraser);          g_ci_eraser = 0;
        KillSprite(g_ci_map_pressed);     g_ci_map_pressed = 0;
        KillSprite(g_ci_map);             g_ci_map = 0;
        KillSprite(g_ci_options_pressed); g_ci_options_pressed = 0;
        KillSprite(g_ci_options);         g_ci_options = 0;
        KillSprite(g_ci_attract_new_off); g_ci_attract_new_off = 0;
        KillSprite(g_ci_attract_new_on);  g_ci_attract_new_on = 0;
        KillSprite(g_ci_attract_hl_on);   g_ci_attract_hl_on = 0;
        KillSprite(g_ci_attract_hl_off);  g_ci_attract_hl_off = 0;
        KillSprite(g_ci_scrolldown_lit);  g_ci_scrolldown_lit = 0;
        KillSprite(g_ci_scrollup_lit);    g_ci_scrollup_lit = 0;
        KillSprite(g_ci_link_middle);     g_ci_link_middle = 0;
        KillSprite(g_ci_link_bottom);     g_ci_link_bottom = 0;
        KillSprite(g_ci_brief2);          g_ci_brief2 = 0;
        KillSprite(g_ci_brief);           g_ci_brief = 0;
        KillSprite(g_ci_script_end);      g_ci_script_end = 0;
    }
}

/* ---- free play ---------------------------------------------------------- */

/* Drop the ten free-play sprites and the four free-play icon groups
 * (200/300/400/500) with their lists. */
// FUNCTION: LEGOLAND 0x0048b540
void CleanUpFreePlay(void)
{
    if (g_fp_spr0) { KillSprite(g_fp_spr0); g_fp_spr0 = 0; }
    if (g_fp_spr1) { KillSprite(g_fp_spr1); g_fp_spr1 = 0; }
    if (g_fp_spr2) { KillSprite(g_fp_spr2); g_fp_spr2 = 0; }
    if (g_fp_spr3) { KillSprite(g_fp_spr3); g_fp_spr3 = 0; }
    if (g_fp_spr4) { KillSprite(g_fp_spr4); g_fp_spr4 = 0; }
    if (g_fp_spr5) { KillSprite(g_fp_spr5); g_fp_spr5 = 0; }
    if (g_fp_spr6) { KillSprite(g_fp_spr6); g_fp_spr6 = 0; }
    if (g_fp_spr7) { KillSprite(g_fp_spr7); g_fp_spr7 = 0; }
    if (g_freeplay_marker) { KillSprite(g_freeplay_marker); g_freeplay_marker = 0; }
    if (g_fp_spr9) { KillSprite(g_fp_spr9); g_fp_spr9 = 0; }
    RemoveObjectListIcons(200);
    RemoveObjectListIcons(300);
    RemoveObjectListIcons(400);
    RemoveObjectListIcons(500);
    RemoveFreePlayList(200);
    RemoveFreePlayList(300);
    RemoveFreePlayList(400);
    RemoveFreePlayList(500);
}
