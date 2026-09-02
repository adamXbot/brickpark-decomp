/* LEGOLAND — free-play lists, object info lists, pop-up info and bubble help.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field offsets, callee arg counts and global addresses are load-bearing;
 * names are ours.
 */
#include "legoland.h"

unsigned int strlen(const char*);
char* strcpy(char*, const char*);
void* memset(void*, int, unsigned int);
#pragma intrinsic(strlen, strcpy, memset)

/* ---- the icon record (see iconui.c / fpui.c) ---------------------------- */
typedef struct Icon {
    struct Icon*     next;      /* +0x00 */
    Sprite*          sprite;    /* +0x04 */
    void*            data;      /* +0x08 object descriptor */
    short            x;         /* +0x0c */
    short            y;         /* +0x0e */
    short            w;         /* +0x10 */
    short            h;         /* +0x12 */
    unsigned short   group;     /* +0x14 */
    short            f16;       /* +0x16 */
    int              f18;       /* +0x18 */
    int              f1c;       /* +0x1c */
    unsigned char    kind;      /* +0x20 */
    char             pad21[3];
    void*            f24;       /* +0x24 */
    int            (*render)(struct Icon*);  /* +0x28 */
    void*            input;     /* +0x2c */
    void*            widget;    /* +0x30 */
    unsigned int     flags;     /* +0x34 */
    char*            help;      /* +0x38 */
    int              help_id;   /* +0x3c */
} Icon;

/* ---- the indicator record (iconui.c) ------------------------------------ */
typedef struct Indicator {
    struct Indicator* next;      /* +0x00 */
    unsigned int      flags;     /* +0x04  0x01 permanent, 0x02 delete on expiry, 0x08 active */
    int               start;     /* +0x08 */
    int               duration;  /* +0x0c */
    int               arg;       /* +0x10  low 16 bits = sort key; 0x10000 fresh, 0x20000 permanent */
    Icon*             icon;      /* +0x14 */
    int               f18;       /* +0x18 */
    int               f1c;       /* +0x1c */
    char              pad20[0x28 - 0x20];
} Indicator;

/* The game record @ 0x004bcbf4 seen as the screen size (panelui.c). Read
 * zero-extended here, so unsigned. */
typedef struct ScreenDims {
    unsigned short width;    /* +0x00 */
    unsigned short height;   /* +0x02 */
} ScreenDims;
extern ScreenDims* g_screen;   /* 0x004bcbf4 */

/* The mouse point @ 0x00813a44 read as 16-bit halves (fpui.c's MousePt). */
typedef struct MousePt {
    short x;      /* +0x00 */
    short pad02;
    short y;      /* +0x04 */
    short pad06;
} MousePt;

/* The bare 16-byte rectangle the icon-bounds helper fills. */
typedef struct ClipRect {
    int left;    /* +0x00 */
    int top;     /* +0x04 */
    int right;   /* +0x08 */
    int bottom;  /* +0x0c */
} ClipRect;

/* ---- object descriptors ------------------------------------------------- */
/* The 0xd0-byte ObjDef LLIDB_LoadODFData builds (llidb_odf.c / fpui.c). Only
 * the fields read here are named. The three LLIDB elements at +0x58..+0x60
 * are the class's place in the build menus: +0x58 the menu it hangs off
 * ("BUILD MENU" for a top-level class, the parent class's element for a
 * child), +0x5c its theme ("COMMON THEME" or one of the four theme names),
 * +0x60 its sub-menu (one of the four g_submenus). */
typedef struct ObjDef {
    char     pad00[0x1c];
    unsigned int flags;   /* +0x1c 0x04000000 / 0x08000000 = research state */
    char     pad20[0x58 - 0x20];
    LLElem*  parent;      /* +0x58 */
    LLElem*  theme;       /* +0x5c */
    LLElem*  submenu;     /* +0x60 */
    char     pad64[0x68 - 0x64];
    Sprite*  icon;        /* +0x68 */
    char     pad6c[0x78 - 0x6c];
    char*    name;        /* +0x78 */
    char*    name2;       /* +0x7c */
    char     pad80[0xc4 - 0x80];
    LLElem*  elem;        /* +0xc4 this class's own LLIDB element */
} ObjDef;

/* The 12-byte node of the object / research lists (fpui.c's ObjNode). */
typedef struct ObjNode {
    struct ObjNode* next;   /* +0x00 */
    ObjDef*         obj;    /* +0x04 */
    int             keep;   /* +0x08 */
} ObjNode;

/* ---- menus -------------------------------------------------------------- */
/* The menu tables are 20-byte LLIDB element NAMES: the four theme menus @
 * 0x004bafa8 (panelui.c's g_menus, indexed by g_menu_index @ 0x004baff8,
 * 5 = none) and the four sub-menus @ 0x004baffc ("SCENERY MENU", "FOOD
 * STORES MENU", "SHOPS MENU", "ATTRACTIONS MENU"). */
typedef struct Menu {
    char name[20];
} Menu;
extern Menu g_submenus[4];      /* 0x004baffc */
extern int  g_menu_index;       /* 0x004baff8 */
extern char g_list_menu;        /* 0x00668e64 which menu the object list was built for */
extern char g_build_menu_name[];      /* 0x004bb49c "BUILD MENU" */
extern char g_common_theme_name[];    /* 0x004ba738 "COMMON THEME" */
extern char g_legoland_theme_name[];  /* 0x004ba748 "LEGOLAND THEME" */
extern char g_adv_theme_name[];       /* 0x004ba704 "ADVENTURERS THEME" */
extern char g_castle_theme_name[];    /* 0x004ba718 "CASTLE THEME" */
extern char g_western_theme_name[];   /* 0x004ba728 "WESTERN THEME" */

/* ---- the free-play panel lists ------------------------------------------ */
/* One item of a free-play panel list (0x1c bytes). Four lists, one per
 * panel: 200 @ 0x007cb3d0, 300 @ 0x007cb39c, 400 @ 0x007cb3b8, 500 @
 * 0x007cb3a4. fpui.c's FreePlayItem is the same record as AddFreePlayIcon
 * reads it (value -> icon +0x1c, data -> icon +0x08, text -> help). */
typedef struct FPItem {
    struct FPItem* next;     /* +0x00 */
    char*          name;     /* +0x04 heap copy of the class's LLIDB name (-> icon +0x1c) */
    LLElem*        elem;     /* +0x08 the class's LLIDB element (-> icon data) */
    LLElem*        parent;   /* +0x0c 0 = a header, else the header's element */
    char*          text;     /* +0x10 heap copy of the caller's text (-> icon help) */
    Sprite*        sprite;   /* +0x14 */
    int            key;      /* +0x18 sort key (headers are kept ascending) */
} FPItem;

/* The scrolling object-list panel (0x2c bytes; fpui.c's ObjListPanel as
 * RedrawObjectList moves it): the icon group, the list box built by
 * AddGBarIcons / SetupInterfacePanelIcons, the extent of the icons on it
 * (+0x0c..+0x18) and the box's own extent (+0x1c..+0x28). */
typedef struct ObjListPanel {
    unsigned short group;    /* +0x00 */
    short          pad02;
    int            f04;      /* +0x04 = 1 */
    Icon*          box;      /* +0x08 the list box icon */
    int            list_x0;  /* +0x0c */
    int            list_y0;  /* +0x10 top of the icons */
    int            list_x1;  /* +0x14 */
    int            list_y1;  /* +0x18 bottom of the icons */
    int            box_x0;   /* +0x1c */
    int            box_y0;   /* +0x20 */
    int            box_x1;   /* +0x24 */
    int            box_y1;   /* +0x28 */
} ObjListPanel;

/* The free-play object table @ 0x004bdeb8: 0x86 16-byte entries {object id
 * byte, LLIDB class name, ...}, indexed by the per-profile "unlocked" byte
 * array (the current profile's 200-byte block @ 0x0080ffe6, profiles.c). */
typedef struct FPTableEntry {
    unsigned char id;      /* +0x00 */
    char          pad01[3];
    char*         name;    /* +0x04 */
    int           f08;     /* +0x08 */
    int           f0c;     /* +0x0c */
} FPTableEntry;
extern FPTableEntry  g_fp_table[0x86];      /* 0x004bdeb8 */
extern unsigned char g_profile_unlocked[200]; /* 0x0080ffe6 */
extern LLElem*       g_fp_theme;            /* 0x007cb3bc the free-play screen's theme element */

extern FPItem* g_fp_list200;   /* 0x007cb3d0 */
extern FPItem* g_fp_list300;   /* 0x007cb39c */
extern FPItem* g_fp_list400;   /* 0x007cb3b8 */
extern FPItem* g_fp_list500;   /* 0x007cb3a4 */

/* The 5th PrintSprite argument (money.c's BlitCtx): kind plus an 8-byte
 * payload cleared as one block. */
typedef struct BlitCtx {
    int kind;                            /* +0x00 */
    struct { Icon* p; int n; } owner;    /* +0x04, +0x08 */
} BlitCtx;

/* Free-play sprites (InitFreePlayScreen loads them, CleanUpFreePlay drops
 * them; fpui.c's g_fp_spr0..9 by address). */
extern Sprite*    g_fp_screen_bk;    /* 0x00810148 FreePlayScreenBK.lls */
extern Sprite*    g_fp_down1;        /* 0x007cb3b4 FP_Down1.lls */
extern Sprite*    g_fp_down2;        /* 0x007cb3ac FP_Down2.lls */
extern Sprite*    g_fp_down3;        /* 0x007cb3b0 FP_Down3.lls */
extern Sprite*    g_fp_down4;        /* 0x007cb3a8 FP_Down4.lls */
extern Sprite*    g_fp_up1;          /* 0x007cb3c4 FP_Up1.lls */
extern Sprite*    g_fp_up2;          /* 0x007cb3c0 FP_Up2.lls */
extern Sprite*    g_fp_up3;          /* 0x007cb3cc FP_Up3.lls */
extern Sprite*    g_fp_up4;          /* 0x007cb3c8 FP_Up4.lls */
extern Sprite*    g_fp_tick;         /* 0x007cb398 FreePlay_Tick.lls */
extern Sprite*    g_fp_cover;        /* 0x007cb3d4 FP_Cover.lls */

/* ---- globals ------------------------------------------------------------ */
extern int        g_power_supply;    /* 0x00832bd0 (power.c) */
extern int        g_power_demand;    /* 0x00832bd4 (power.c) */
extern int        g_power_available; /* 0x0083298c  0 = no power at all: the bar is replaced by No_Energy */
extern int        g_bar_demand_len;  /* 0x006688bc displayed demand pointer offset (eased) */
extern int        g_bar_supply_len;  /* 0x006688c0 displayed supply bar length (eased) */
extern Sprite*    g_ci_no_energy;    /* 0x00668e6c No_Energy.lls (fpui.c) */
extern Sprite*    g_ci_bar_pointer;  /* 0x00668e70 Bar_pointer.lls (fpui.c) */
extern Icon*      g_side_icons;      /* 0x006687c8 icon list head */
extern Icon*      g_side_icons2;     /* 0x006687cc second icon list head */
extern Indicator* g_ind_active;      /* 0x006688d8 */

/* ---- callees ------------------------------------------------------------ */
extern void* HeapAlloc_w(unsigned int size);    /* 0x0049e4ff */
extern void  HeapFree_w(void* p);               /* 0x0049e4d0 */
__declspec(noreturn) void exit(int code);       /* 0x004a02b8 CRT exit */
extern int  GetGameTimer(void);                 /* 0x00499430 */
extern int  LLIDB_FindElement(const char* name, LLElem** out, unsigned int* outidx); /* 0x0047b330 */
extern int  LLIDB_GetCount(void);               /* 0x0047b2d0 */
extern void LLIDB_GetElement(int i, LLElem** out); /* 0x0047b2e0 */
extern void DelObjectList(void);                /* 0x004756e0 */
extern void InsertObjectNode(ObjDef* d);        /* 0x004755c0 */
extern void InsertChildIntoList(ObjDef* d);     /* 0x00475630 */
int  MakeUpObjectList(int group, int x, int y, int h);   /* 0x00475960 */
extern Icon* InsertIcon(short x, short y, unsigned short group, Sprite* s); /* 0x0046d6c0 */
extern Icon* AddGBarIcons(void* owner, int x, int y, int unused, int h, int group); /* 0x0046dbc0 */
extern void  SetNewGroup_Callbacks(void* a, void* b, void* c);  /* 0x0046d740 */
extern Icon* AddFreePlayIcon(void* owner, FPItem* d, int x, int y, int group, short f16, int f20); /* 0x0046f7a0 */
extern void  AddFullScreenIcon(int group);                   /* 0x0046d760 */
extern Icon* FindIcon(unsigned short group);                 /* 0x0046d630 */
extern void  SetIconSprite(Icon* p, Sprite* s);              /* 0x0046d680 */
extern char* GetString(int id);                              /* 0x00498f50 */
extern int   RenderFreePlayIcons(Icon* p);                   /* 0x0046e300 (fpui.c) */
extern char  FreePlayIconInput(Icon* p, int ev, short dx, short dy); /* 0x0048b000 (not exported) */
extern void  KillSprite(Sprite* s);                          /* 0x00497bd0 */
/* 0x0047c7f0 (not exported): the free-play icon sprite of a class element,
 * with its text, sort key and parent element through the out pointers;
 * 0 when the class has none. Name is ours. */
extern Sprite* GetFreePlayItemInfo(LLElem* e, char** text, int* key, LLElem** parent);
void Add2FreePlayPanelLists(Sprite* sprite, LLElem* elem, char* text, int key, LLElem* parent, int panel);
extern void StoreClipping(void);                /* 0x0048a660 */
extern void SetClipping(ClipRect* r);           /* 0x0048a5c0 */
extern void RestoreClipping(void);              /* 0x0048a690 */
extern int  PrintSprite(Sprite* s, int x, int y, int mode, BlitCtx* ctx); /* 0x004853a0 */
extern void RemoveIndicator(Indicator* p);      /* 0x0046fda0 */
extern void DeleteIndicator(Indicator* p);      /* 0x0046fe00 */
/* 0x0046f330 (not exported): is the point inside the icon's own hit test
 * (the 0x40 "custom hit" icons). 0x0046de50 (not exported): the bounds of a
 * 0x20 "clip" icon. Names are ours. */
extern int  IconHitTest(MousePt* pt, Icon* p);        /* 0x0046f330 */
extern void GetIconBounds(Icon* p, ClipRect* r);      /* 0x0046de50 */

/* ---- indicators --------------------------------------------------------- */

/* Link `p` into the active list in front of the first record with a lower
 * key (so the list stays sorted, highest key first); at the end if none. */
static __inline void InsertIndicatorSorted(Indicator* p)
{
    Indicator* q;
    Indicator* prev;
    int key;

    q = g_ind_active;
    if (q == 0) {
        g_ind_active = p;
        p->next = 0;
    } else {
        key = p->arg;
        prev = 0;
        while (q) {
            if (q->arg < key) {
                if (prev)
                    prev->next = p;
                else
                    g_ind_active = p;
                p->next = q;
                break;
            }
            prev = q;
            q = q->next;
        }
        if (q == 0) {
            prev->next = p;
            p->next = 0;
        }
    }
}

/* Age the active indicators: rebuild the active list sorted by key (highest
 * first), keeping permanent ones and those still inside their duration;
 * expired ones go back to the inactive list (and are deleted if flagged).
 * Then lay the survivors out right-to-left from the screen edge, 0x34 apart,
 * parking off-screen whatever would cross the left quarter. */
// FUNCTION: LEGOLAND 0x0046feb0
void ControlIndicators(void)
{
    Indicator* p = g_ind_active;
    Indicator* cur;
    Indicator* q;
    int now = GetGameTimer();
    int x = g_screen->width - 0x50;

    g_ind_active = 0;
    while (p) {
        p->arg &= 0xffff;
        if (now - p->start < 5000)
            p->arg |= 0x10000;
        else
            p->icon->flags &= ~8;
        if (p->flags & 1) {
            p->arg |= 0x20000;
        } else if (now - p->start >= p->duration) {
            cur = p;
            p = p->next;
            cur->next = g_ind_active;
            g_ind_active = cur;
            RemoveIndicator(cur);
            if (cur->flags & 2)
                DeleteIndicator(cur);
            continue;
        }
        cur = p;
        p = p->next;
        InsertIndicatorSorted(cur);
    }

    q = g_ind_active;
    while (q) {
        q->icon->x = (short)x;
        q->icon->y = 8;
        x -= 0x34;
        if (x <= g_screen->width / 4) {
            while (q) {
                q->icon->x = (short)0xf000;
                q = q->next;
            }
            return;
        }
        q = q->next;
    }
}

/* ---- icon focus --------------------------------------------------------- */

/* The icon under the mouse: the last sized (0x10), enabled icon containing
 * the point on either list. Custom-hit icons (0x40) set bit 2 of *outflags
 * and drop the hit unless it is a 0x800 icon whose group is the custom
 * icon's group + 2 or + 3. Clip icons (0x20) gate everything after them on
 * the point being inside their bounds. */
// FUNCTION: LEGOLAND 0x0046f360
Icon* GetIconAtPos(MousePt* pt, unsigned char* outflags)
{
    ClipRect rect;
    Icon*    p;
    Icon*    hit = 0;
    int      ok = 1;
    int      i;
    short    x = pt->x;
    short    y = pt->y;

    for (i = 0; i < 2; i++) {
        if (i)
            p = g_side_icons2;
        else
            p = g_side_icons;
        for (; p; p = p->next) {
            if (ok) {
                if ((p->flags & 0x10) && !(p->flags & 0x400)) {
                    if (x >= p->x && y >= p->y &&
                        x <= p->x + p->w && y <= p->y + p->h) {
                        hit = p;
                        goto bounds;
                    }
                }
                if ((p->flags & 0x40) && !(p->flags & 0x400)) {
                    if (IconHitTest(pt, p)) {
                        *outflags |= 4;
                        if (!(hit && (hit->flags & 0x800) &&
                              (p->group - 1 == hit->group - 3 ||
                               p->group - 1 == hit->group - 4)))
                            hit = 0;
                    }
                }
            }
        bounds:
            if ((p->flags & 0x20) && !(p->flags & 0x400)) {
                GetIconBounds(p, &rect);
                if (x >= rect.left && x <= rect.right &&
                    y >= rect.top && y <= rect.bottom)
                    ok = 1;
                else
                    ok = 0;
            }
        }
    }
    return hit;
}

/* ---- free-play panel lists ---------------------------------------------- */

/* Add an item to the free-play list of panel `panel` (200/300/400/500).
 * Without a parent the item is a header, kept in ascending `key` order;
 * with one it is a class entry linked directly after its parent's item (the
 * first item whose element is the parent) and dropped again when the parent
 * is not on the list. */
// FUNCTION: LEGOLAND 0x0048b110
void Add2FreePlayPanelLists(Sprite* sprite, LLElem* elem, char* text, int key,
                            LLElem* parent, int panel)
{
    FPItem*  prev = 0;
    FPItem** head = 0;
    FPItem*  q;
    FPItem*  n = (FPItem*)HeapAlloc_w(sizeof(FPItem));

    if (panel == 200) {
        q = g_fp_list200;
        head = &g_fp_list200;
    } else if (panel == 300) {
        q = g_fp_list300;
        head = &g_fp_list300;
    } else if (panel == 400) {
        q = g_fp_list400;
        head = &g_fp_list400;
    } else if (panel == 500) {
        q = g_fp_list500;
        head = &g_fp_list500;
    } else {
        q = 0;
    }
    n->text = (char*)HeapAlloc_w(strlen(text) + 1);
    strcpy(n->text, text);
    n->name = (char*)HeapAlloc_w(strlen(elem->name) + 1);
    strcpy(n->name, elem->name);
    n->elem = elem;
    n->parent = parent;
    n->sprite = sprite;
    n->key = key;
    n->next = 0;
    if (parent) {
        while (q && q->elem != parent)
            q = q->next;
        if (q == 0) {
            HeapFree_w(n->name);
            HeapFree_w(n->text);
            HeapFree_w(n);
            return;
        }
        prev = q;
        q = q->next;
    } else {
        while (q) {
            if (q->key > key)
                break;
            prev = q;
            q = q->next;
        }
    }
    if (prev)
        prev->next = n;
    else
        *head = n;
    n->next = q;
}

/* ---- the energy bar ----------------------------------------------------- */

/* The power HUD bar: the bar sprite clipped to the supply's share of the
 * gadget width (scaled against max(2*demand, supply)) with the pointer
 * sprite at the demand's share; both lengths ease 6 pixels a frame as the
 * money bar does. Without any power the No_Energy sprite is drawn instead. */
// FUNCTION: LEGOLAND 0x0046e4d0
int RenderEnergyBar(Icon* g)
{
    BlitCtx  ctx;
    ClipRect clip;
    int      supply = g_power_supply;
    int      demand = g_power_demand;
    int      scale;
    int      want_s;
    int      want_d;

    ctx.kind = 1;
    memset(&ctx.owner, 0, sizeof(ctx.owner));
    if (supply == 0 && demand == 0) {
        want_s = 0;
        want_d = 0;
    } else {
        scale = demand + demand;
        if (supply > scale)
            scale = supply;
        if (scale == 0)
            scale = 1;
        want_s = g->w * supply / scale;
        want_d = g->w * demand / scale;
    }
    if (g->sprite && g_power_available) {
        StoreClipping();
        if (want_s > g_bar_supply_len) {
            g_bar_supply_len += 6;
            if (g_bar_supply_len > want_s)
                g_bar_supply_len = want_s;
        } else {
            g_bar_supply_len -= 6;
            if (g_bar_supply_len < want_s)
                g_bar_supply_len = want_s;
        }
        if (want_d > g_bar_demand_len) {
            g_bar_demand_len += 6;
            if (g_bar_demand_len > want_d)
                g_bar_demand_len = want_d;
        } else {
            g_bar_demand_len -= 6;
            if (g_bar_demand_len < want_d)
                g_bar_demand_len = want_d;
        }
        clip.left   = g->x;
        clip.top    = g->y;
        clip.right  = clip.left + g_bar_supply_len;
        clip.bottom = clip.top + g->h;
        SetClipping(&clip);
        PrintSprite(g->sprite, g->x, g->y, 0, &ctx);
        RestoreClipping();
        PrintSprite(g_ci_bar_pointer, g->x - g_ci_bar_pointer->w / 2 + g_bar_demand_len,
                    g->y + g->h / 2, 0, &ctx);
    } else if (!g_power_available) {
        PrintSprite(g_ci_no_energy, g->x - 0x15, g->y - 6, 0, &ctx);
    }
    return 0;
}

/* ---- the object list ---------------------------------------------------- */

/* Rebuild the object list for theme menu `m`: for each sub-menu, every
 * loaded class hanging off BUILD MENU whose theme and sub-menu match (or
 * whose theme is COMMON THEME, on menu 0), then every child class of the
 * theme; then the side-panel icons. The counter starts at 1, so the list is
 * always "made up" (reproduced as the original has it).
 *
 * Residual (13 of 147, first diverging index 5): every instruction, every branch and
 * every stack slot is right; the four callee-saved registers are permuted. Ours
 * n=ebx, count=ebp, submenu-IV=edi, j=esi; the original n=edi, count=ebx,
 * submenu-IV=ebp, j=esi.
 *
 * Measured, not guessed: compiling the body with `n` deleted makes VC6 push only
 * ebp/esi/edi (ebx is NOT pushed) and gives count=ebp, j=esi, IV=edi, so VC6's
 * callee-saved preference list here is [ebp, esi, edi, ebx] - ebx is the last-resort
 * register - and the register a value gets is purely its RANK in that list. Ours
 * ranks count > j > IV > n; the original ranks IV > j > n > count. So two things have
 * to move at once: `count` from first to last, and the outer induction variable above
 * `j`. (The sibling RAndDLinkedList, which is exact, ranks the same way as the
 * original - scratch temp > j > i > count - which is what says the ranking, not the
 * dataflow, is what differs.)
 *
 * Ruled out (register map unchanged in every one): exit(n) vs exit(1); `register`;
 * declaration order of any local; n as unsigned; n = 0 / n assigned late (that only
 * moves the `mov reg,1` out of index 5 and costs 5 more diffs); n++ before the call;
 * n += 1; `if (n > 0)`; a second `count` copy for the second loop, and a per-iteration
 * `int c = count` inner bound; a dead `count = 0` before the call, and `int count = 0`;
 * j vs i for the second loop's index, j hoisted to function scope, the second loop in
 * its own block with its own index; a `Menu* p` walk bounded by `p < g_submenus + 4`
 * (that one is the only change that alters the code at all - `jl` becomes `jb`, 14
 * diffs) and the same walk with the bound cast to int (identical to the array form,
 * `jl` kept, 13 diffs); p initialised at the top of the function / before the
 * GetCount call / in the for-init; `p[i].name` through a base pointer; a `(char*)`
 * cast of the element address; while/do-while spellings of either loop; the second
 * loop moved into a static __inline helper (150 insns, worse); `4 > i`; a combined
 * `int n = 1, count, i;` declaration; `d` declared in the innermost scopes (`e`
 * declared there instead costs 18 more diffs - it must stay one function-level
 * out-param). The only thing that
 * ever produced the original's count=ebx + IV=ebp was keeping BOTH an `int i` and a
 * `Menu* p` live (5 candidates), which spills `n` to the stack: 152 insns, wrong.
 * Variants under scratchpad/fpui2/oll (A..NN) and scratchpad/fpui2/w2 (A..S, r_*). */
// WIP-FUNCTION: LEGOLAND 0x00475720  (147/147 insns, 409/409 bytes, 13 mismatches: a callee-saved permutation, see above)
int ObjectLinkedList(Menu* m)
{
    LLElem* e;
    LLElem* e_theme;
    LLElem* e_build;
    LLElem* e_menu;
    LLElem* e_common;
    ObjDef* d;
    int     count;
    int     i;
    int     n = 1;

    DelObjectList();
    if (LLIDB_FindElement(g_build_menu_name, &e_build, 0))
        exit(n);
    if (LLIDB_FindElement(g_common_theme_name, &e_common, 0))
        exit(1);
    if (LLIDB_FindElement(m->name, &e_theme, 0))
        exit(1);
    count = LLIDB_GetCount();
    for (i = 0; i < 4; i++) {
        int j;
        if (LLIDB_FindElement(g_submenus[i].name, &e_menu, 0))
            exit(1);
        for (j = 0; j < count; j++) {
            LLIDB_GetElement(j, &e);
            if ((e->type_flags & 0x13) == 0x13) {
                d = (ObjDef*)e->data;
                if (d->parent == e_build &&
                    ((d->theme == e_theme && d->submenu == e_menu) ||
                     (d->theme == e_common && g_menu_index == 0 && d->submenu == e_menu))) {
                    InsertObjectNode(d);
                    n++;
                }
            }
        }
    }
    for (i = 0; i < count; i++) {
        LLIDB_GetElement(i, &e);
        if ((e->type_flags & 0x13) == 0x13) {
            d = (ObjDef*)e->data;
            if (d->theme == e_theme && d->parent != e_build && d->parent != 0)
                InsertChildIntoList(d);
        }
    }
    g_list_menu = (char)g_menu_index;
    if (n) {
        MakeUpObjectList(0xd2, 3, 0x21, 0x154);
        return 1;
    }
    return 0;
}

/* The research (R&D) variant: DLL-backed classes (0x10000) only, the
 * COMMON THEME entries only under the first sub-menu, and every class not
 * yet researched (neither 0x04000000 nor 0x08000000) gets 0x04000000. No
 * icons are built; the list menu is recorded as g_menu_index + 4. */
/* This body is EXACT - 148/148 instructions, 444/444 bytes, index for index. It is
 * NOT 18 instructions short: audit.py simply cannot BOUND it. The original's last
 * block is the shared 'push 1; call exit' stub (0x00475e85) that the in-loop
 * FindElement guard jumps to; it sits past the function's only ret (0x00475e84) and
 * ends in a noreturn call, so the extent walker steps over the four nops and swallows
 * the whole of the NEXT function - SetIconGroupFlag400 @ 0x00475e90, itself already
 * matched in panelui.c - which is exactly the 18 instructions / 57 bytes audit.py
 * reports on top (170i/501B vs the true 148i/444B). Our own 152i/448B is the 148
 * real instructions plus 4 bytes of COMDAT padding. Verify with
 *   python3 tools/matchfull.py LEGOLAND/fpui2.c RAndDLinkedList 0x475cd0
 * which prints FULL MATCH 148/148 = 100.0%. */
/* Exact: 148/148 instructions, 444/444 bytes. The body ends in a noreturn
 * exit with no ret, which tools/audit.py could not bound until it learned
 * to treat an inter-function padding run as a terminator. */
// FUNCTION: LEGOLAND 0x00475cd0
int RAndDLinkedList(Menu* m)
{
    LLElem* e;
    LLElem* e_build;
    LLElem* e_theme;
    LLElem* e_menu;
    LLElem* e_common;
    ObjDef* d;
    int     count;
    int     i;

    DelObjectList();
    if (LLIDB_FindElement(g_build_menu_name, &e_build, 0))
        exit(1);
    if (LLIDB_FindElement(g_common_theme_name, &e_common, 0))
        exit(1);
    if (LLIDB_FindElement(m->name, &e_theme, 0))
        exit(1);
    count = LLIDB_GetCount();
    for (i = 0; i < 4; i++) {
        int j;
        if (LLIDB_FindElement(g_submenus[i].name, &e_menu, 0))
            exit(1);
        for (j = 0; j < count; j++) {
            LLIDB_GetElement(j, &e);
            if ((e->type_flags & 0x10011) == 0x10011) {
                d = (ObjDef*)e->data;
                if (d->parent == e_build) {
                    if (d->theme == e_theme && d->submenu == e_menu) {
                        if (!(d->flags & 0xc000000))
                            d->flags |= 0x4000000;
                        InsertObjectNode(d);
                    } else if (d->theme == e_common && g_menu_index == 0 && i == 0) {
                        if (!(d->flags & 0xc000000))
                            d->flags |= 0x4000000;
                        InsertObjectNode(d);
                    }
                }
            }
        }
    }
    for (i = 0; i < count; i++) {
        LLIDB_GetElement(i, &e);
        if ((e->type_flags & 0x10011) == 0x10011) {
            d = (ObjDef*)e->data;
            if (d->parent != e_build && d->parent != 0 && d->theme == e_theme) {
                if (!(d->flags & 0xc000000))
                    d->flags |= 0x4000000;
                InsertChildIntoList(d);
            }
        }
    }
    g_list_menu = (char)g_menu_index + 4;
    return 1;
}

/* ---- the free-play object lists ----------------------------------------- */

/* Build the icon panel for free-play list `panel` (200/300/400/500) as
 * group `group` at (x, y), h high: the list box, one AddFreePlayIcon per
 * item 0x42 apart, then the panel's own scroll buttons re-skinned with the
 * panel's up/down sprites. An empty list just gets the cover sprite. The
 * list-head selection has no default, so an unknown panel reads an
 * uninitialised head [sic]; the panel record is never freed. The list head
 * itself is the loop cursor: a separate item pointer adds a web and costs
 * `group` its register (x then takes ebp and group is re-read). */
// FUNCTION: LEGOLAND 0x0048b2a0
int FreePlayObjectList(int group, int x, int y, int h, int panel)
{
    ObjListPanel* p;
    FPItem*       list;
    Icon*         icon;
    Sprite*       down;
    Sprite*       up;

    p = (ObjListPanel*)HeapAlloc_w(sizeof(ObjListPanel));
    if (!p)
        return 0;
    if (panel == 200) {
        down = g_fp_down1;
        up = g_fp_up1;
        list = g_fp_list200;
    } else if (panel == 500) {
        down = g_fp_down2;
        up = g_fp_up2;
        list = g_fp_list500;
    } else if (panel == 400) {
        down = g_fp_down3;
        up = g_fp_up3;
        list = g_fp_list400;
    } else if (panel == 300) {
        down = g_fp_down4;
        up = g_fp_up4;
        list = g_fp_list300;
    }
    if (!list) {
        InsertIcon(x - 3, 0x15, 7, g_fp_cover);
        return 0;
    }
    icon = AddGBarIcons(p, x, y, 1, h, group);
    p->box = icon;
    x = icon->x;
    p->box_x0 = x;
    p->list_x0 = x;
    y = icon->y;
    p->box_y0 = y;
    p->list_y0 = y;
    p->box_x1 = icon->x + icon->w;
    p->box_y1 = icon->y + icon->h;
    p->f04 = 1;
    p->group = group;
    SetNewGroup_Callbacks(0, RenderFreePlayIcons, FreePlayIconInput);
    for (; list; list = list->next) {
        AddFreePlayIcon(p, list, x, y, group, 1, (int)list->parent);
        y += 0x42;
    }
    AddFullScreenIcon(group + 6);
    p->list_x1 = x;
    p->list_y1 = y;
    icon = FindIcon(group + 4);
    if (icon) {
        SetIconSprite(icon, down);
        icon->x -= 9;
        icon->help_id = 0x94;
        icon->help = GetString(0x94);
    }
    icon = FindIcon(group + 3);
    if (icon) {
        SetIconSprite(icon, up);
        icon->x -= 9;
        icon->help_id = 0x95;
        icon->help = GetString(0x95);
    }
    return 1;
}

/* Fill the four free-play panel lists from the current profile: every
 * unlocked object id is looked up in the free-play table, its class
 * resolved, and the item filed under the panel of its theme (LEGOLAND and
 * COMMON on 200, ADVENTURERS 300, CASTLE 400, WESTERN 500). */
// FUNCTION: LEGOLAND 0x0048ad00
void InitFreePlayLists(void)
{
    int           key = 0;
    LLElem*       e_cat = 0;
    LLElem*       e_build = 0;
    LLElem*       e_common = 0;
    LLElem*       e_legoland = 0;
    LLElem*       e_adv = 0;
    LLElem*       e_castle = 0;
    LLElem*       e_western = 0;
    LLElem*       parent;
    char*         text;
    Sprite*       sprite;
    char*         theme;
    unsigned char i;
    int           k;
    FPTableEntry* ent;

    if (LLIDB_FindElement(g_build_menu_name, &e_build, 0))
        exit(1);
    if (LLIDB_FindElement(g_common_theme_name, &e_common, 0))
        exit(1);
    if (LLIDB_FindElement(g_legoland_theme_name, &e_legoland, 0))
        exit(1);
    if (LLIDB_FindElement(g_adv_theme_name, &e_adv, 0))
        exit(1);
    if (LLIDB_FindElement(g_castle_theme_name, &e_castle, 0))
        exit(1);
    if (LLIDB_FindElement(g_western_theme_name, &e_western, 0))
        exit(1);
    for (i = 0; i < 200; i++) {
        if (g_profile_unlocked[i] == 0)
            continue;
        for (k = 0; k < 0x86; k++) {
            ent = &g_fp_table[k];
            if (ent->id == i &&
                LLIDB_FindElement(ent->name, &e_cat, 0) == 0)
                break;
        }
        if (k == 0x86)
            continue;
        sprite = GetFreePlayItemInfo(e_cat, &text, &key, &parent);
        if (sprite == 0)
            continue;
        theme = g_fp_theme->name;
        if (theme == e_legoland->name || theme == e_common->name)
            Add2FreePlayPanelLists(sprite, e_cat, text, key, parent, 200);
        else if (theme == e_adv->name)
            Add2FreePlayPanelLists(sprite, e_cat, text, key, parent, 300);
        else if (theme == e_castle->name)
            Add2FreePlayPanelLists(sprite, e_cat, text, key, parent, 400);
        else if (theme == e_western->name)
            Add2FreePlayPanelLists(sprite, e_cat, text, key, parent, 500);
        else
            KillSprite(sprite);
        HeapFree_w(text);
    }
}

/* ---- the free-play screen -------------------------------------------- */
extern int         g_icons2_mode;        /* 0x00668e38 */
extern int         g_freeplay_progress;  /* 0x007cb3a0 */
extern int         g_fp_798650;          /* 0x00798650 */
extern Icon*       g_fp_accept_icon;     /* 0x0079864c */
extern void*       g_icon_handler1;      /* 0x006687bc */
extern void*       g_icon_handler2;      /* 0x006687c0 */
extern char g_lls_fp_screen_bk[];   /* 0x004beb34 "FreePlayScreenBK.lls" */
extern char g_lls_fp_down1[];       /* 0x004beb24 */
extern char g_lls_fp_down2[];       /* 0x004beb14 */
extern char g_lls_fp_down3[];       /* 0x004beb04 */
extern char g_lls_fp_down4[];       /* 0x004beaf4 */
extern char g_lls_fp_up1[];         /* 0x004beae8 */
extern char g_lls_fp_up2[];         /* 0x004beadc */
extern char g_lls_fp_up3[];         /* 0x004bead0 */
extern char g_lls_fp_up4[];         /* 0x004beac4 */
extern char g_lls_fp_tick[];        /* 0x004beab0 "FreePlay_Tick.lls" */
extern char g_lls_fp_cover[];       /* 0x004beaa0 "FP_Cover.lls" */
extern char g_lls_fp_goback[];      /* 0x004bea88 "GoBack_on_FreePlay.lls" */
extern char g_lls_fp_accept[];      /* 0x004bea70 "Accept_On_FreePlay.lls" */
extern char g_lls_bar_rides[];      /* 0x004bea60 "Bar_Rides.lls" */
extern char g_path_control_name[];  /* 0x004b8a70 "PATH CONTROL" */
extern Sprite* LoadSprite(const char* name, int mode);                            /* 0x00497ab0 */
extern Icon*   LoadSpriteIcon(const char* name, int mode, int x, int y, int group); /* 0x0046d7b0 */
extern int     LLIDB_UnLoadLLSData(LLElem* e);   /* 0x0047c6a0 */
extern void    LLIDB_ClearOnLevel(void);         /* 0x0047b4c0 */
extern int     RenderFreePlayBar(Icon* p);       /* 0x0046e7b0 (fpui.c) */
extern void    FreePlayInit_48b6c0(void);        /* 0x0048b6c0 (not exported) */
extern void    FreePlayInit_48a790(void);        /* 0x0048a790 (not exported) */
extern char    FreePlayGoBackInput(Icon* p, int ev);   /* 0x0048fb80 */
extern char    FreePlayAcceptInput(Icon* p, int ev);   /* 0x0048ac60 */
void InitFreePlayLists(void);
int  FreePlayObjectList(int group, int x, int y, int h, int panel);

// FUNCTION: LEGOLAND 0x0048a8a0
void InitFreePlayScreen(void)
{
    LLElem* e_path;
    LLElem* e;
    Icon*   p;
    int     count;
    int     i;

    g_icons2_mode = 0;
    FreePlayInit_48b6c0();
    g_freeplay_progress = 0;
    g_fp_798650 = 0;
    g_fp_screen_bk = LoadSprite(g_lls_fp_screen_bk, 0);
    g_fp_down1 = LoadSprite(g_lls_fp_down1, 4);
    g_fp_down2 = LoadSprite(g_lls_fp_down2, 4);
    g_fp_down3 = LoadSprite(g_lls_fp_down3, 4);
    g_fp_down4 = LoadSprite(g_lls_fp_down4, 4);
    g_fp_up1 = LoadSprite(g_lls_fp_up1, 4);
    g_fp_up2 = LoadSprite(g_lls_fp_up2, 4);
    g_fp_up3 = LoadSprite(g_lls_fp_up3, 4);
    g_fp_up4 = LoadSprite(g_lls_fp_up4, 4);
    g_fp_tick = LoadSprite(g_lls_fp_tick, 4);
    g_fp_cover = LoadSprite(g_lls_fp_cover, 4);
    p = LoadSpriteIcon(g_lls_fp_goback, 4, 0xd, 0x137, 7);
    p->help_id = 0x26;
    p->help = GetString(0x26);
    p->flags |= 0x6002;
    p->input = FreePlayGoBackInput;
    g_icon_handler2 = FreePlayGoBackInput;
    p = LoadSpriteIcon(g_lls_fp_accept, 4, 0x1e4, 0x14b, 7);
    p->help_id = 0x32;
    p->help = GetString(0x32);
    p->flags |= 0x6002;
    p->input = FreePlayAcceptInput;
    g_icon_handler1 = FreePlayAcceptInput;
    p->flags |= 0x400;
    g_fp_accept_icon = p;
    p = LoadSpriteIcon(g_lls_bar_rides, 4, 0xd1, 0x156, 7);
    p->render = RenderFreePlayBar;
    p->flags |= 0x400a;
    count = LLIDB_GetCount();
    LLIDB_FindElement(g_path_control_name, &e_path, 0);
    for (i = 0; i < count; i++) {
        LLIDB_GetElement(i, &e);
        if ((e->type_flags & 0x10) && (e->type_flags & 1) && e_path != e && e->refcount) {
            e->refcount--;
            if (e->refcount == 0 && (e->type_flags & 1)) {
                e->type_flags &= 0xfffcfff0;
                if ((e->type_flags & 0xfff0) == 0x10 || (e->type_flags & 0xfff0) == 0x1010)
                    LLIDB_UnLoadLLSData(e);
            }
        }
    }
    LLIDB_ClearOnLevel();
    InitFreePlayLists();
    FreePlayObjectList(200, 0x2a, 0x41, 0xec, 200);
    FreePlayObjectList(500, 0xbb, 0x41, 0xec, 500);
    FreePlayObjectList(400, 0x14c, 0x41, 0xec, 400);
    FreePlayObjectList(300, 0x1dd, 0x41, 0xec, 300);
    FreePlayInit_48a790();
}

/* ---- bubble help ------------------------------------------------------- */
/* The Windows RECT DrawText fills. */
typedef struct WinRect {
    int left;    /* +0x00 */
    int top;     /* +0x04 */
    int right;   /* +0x08 */
    int bottom;  /* +0x0c */
} WinRect;

/* A rendered-text cache entry (bighelp.c's TextEntry): size at +0x00/+0x04. */
typedef struct TextEntry {
    int w;               /* +0x00 */
    int h;               /* +0x04 */
} TextEntry;

/* The bubble anchor rectangle (iconui.c's HelpRect). */
typedef struct HelpRect {
    int x0;   /* +0x00 */
    int y0;   /* +0x04 */
    int x1;   /* +0x08 */
    int y1;   /* +0x0c */
} HelpRect;

__declspec(dllimport) void* __stdcall CreateCompatibleDC(void* dc);            /* [0x4ab094] */
__declspec(dllimport) int   __stdcall SetBkMode(void* dc, int mode);           /* [0x4ab074] */
__declspec(dllimport) void* __stdcall SelectObject(void* dc, void* obj);       /* [0x4ab080] */
__declspec(dllimport) int   __stdcall DeleteDC(void* dc);                      /* [0x4ab0a4] */
__declspec(dllimport) int   __stdcall DrawTextA(void* dc, const char* s, int n,
                                                WinRect* rc, unsigned int fmt); /* [0x4ab2ac] */
extern TextEntry* FindCachedText(const char* text, int font, int f1, int ink, int paper); /* 0x00455d40 */
extern TextEntry* RasterizeText(const char* text, int w, int h, int font, int f1,
                                int ink, int paper);                           /* 0x00455bb0 */
extern void  PrintCachedEntry(TextEntry* e, int x, int y);                    /* 0x00455ec0 */
extern void* SelectFont(void* dc, int font);                                  /* 0x00454b40 */
extern void  RenderBlock(int x, int y, int w, int h, int colour);             /* 0x004890c0 */
extern int   GetNearestColour(int r, int g, int b);                           /* 0x0044e6c0 */

// FUNCTION: LEGOLAND 0x004557c0
void HTBubbleHelp(HelpRect* r, char* text, int font)
{
    TextEntry* ent;
    WinRect    rc;
    int        colour;
    int        h;
    int        tw;
    int        bx;
    HelpRect   box;

    rc.left = 0;
    rc.top = 0;
    rc.right = 0;
    rc.bottom = 0;
    colour = GetNearestColour(0xda, 0xc6, 0x96);
    if (text == 0)
        return;
    rc.right = 200;
    ent = FindCachedText(text, font, 0x10, 0x96c6da, 0);
    if (ent == 0) {
        void* dc;
        void* oldfont;
        dc = CreateCompatibleDC(0);
        SetBkMode(dc, 1);
        oldfont = SelectFont(dc, font);
        h = DrawTextA(dc, text, strlen(text), &rc, 0x410);
        rc.top = r->y0;
        rc.bottom = h + rc.top;
        SelectObject(dc, oldfont);
        DeleteDC(dc);
        ent = RasterizeText(text, rc.right - rc.left, h, font, 0x10, 0x96c6da, 0);
    } else {
        rc.left = 0;
        rc.top = 0;
        rc.right = ent->w;
        rc.bottom = ent->h;
        h = ent->h;
    }
    bx = (r->x1 + r->x0) >> 1;
    if (bx < 0)
        bx = 0;
    else if (bx > g_screen->width)
        bx = g_screen->width;
    tw = rc.right - rc.left;
    rc.left = bx - (tw >> 1);
    rc.right = bx + ((tw + 1) >> 1);
    if (rc.left < 0)
        rc.left = 0;
    else if (rc.right >= g_screen->width)
        rc.left = g_screen->width - tw;
    rc.right = rc.left + tw;
    if (r->y0 < rc.bottom - rc.top + 8) {
        rc.top = r->y1 + 6;
        rc.bottom = rc.top + h;
    } else {
        rc.bottom = r->y0 - 6;
        rc.top = rc.bottom - h;
    }
    box.x1 = rc.right + 4;
    box.x0 = rc.left - 4;
    box.y0 = rc.top - 4;
    box.y1 = rc.bottom + 4;
    RenderBlock(box.x0 + 1, box.y0 + 1, box.x1 - box.x0, box.y1 - box.y0 - 1, colour);
    RenderBlock(box.x0, box.y0, box.x1 - box.x0, 1, 0);
    RenderBlock(box.x0, box.y1, box.x1 - box.x0, 1, 0);
    RenderBlock(box.x0, box.y0, 1, box.y1 - box.y0, 0);
    RenderBlock(box.x1, box.y0, 1, box.y1 - box.y0, 0);
    PrintCachedEntry(ent, rc.left, rc.top);
}


/* ---- the build-object icon ---------------------------------------------- */
/* The list-menu record @ 0x00668e64 is written as a byte (ObjectLinkedList,
 * RAndDLinkedList) and read back as a dword masked to the low byte
 * (MakeUpObjectList) — one union so the two views alias. */
typedef union ListMenu {
    char         menu;    /* which menu the object list was built for */
    unsigned int word;
} ListMenu;
extern ListMenu g_list_menu_u;         /* 0x00668e64 */
extern int      g_list_scroll[];       /* 0x00668e44 per-menu saved scroll */
extern int      g_scroll_flags;        /* 0x006688b8 bit0 = up lit, bit1 = down lit (fpui.c) */
extern int      g_edit_changed;        /* 0x008119b0 (objmap.c) */
extern ObjDef*  g_edit_object;         /* 0x008119b8 (objmap.c) */
extern const unsigned int g_dim_mode;  /* 0x004ba884 = 0xff000000, PrintSprite's dimmed mode */
extern char     g_str_path[];          /* 0x004ba888 "Path" */
extern char     g_fmt_int[];           /* 0x004b8a80 "%d" */
extern Sprite*  g_ci_attract_hl_on;    /* 0x00668e74 Attract_Highlight_On.lls */
extern Sprite*  g_ci_attract_hl_off;   /* 0x00668e78 Attract_Highlight_Off.lls */
extern Sprite*  g_ci_attract_new_off;  /* 0x00668e7c Attract_New_Off.lls */
extern Sprite*  g_ci_attract_new_on;   /* 0x00668e80 Attract_New_On.lls */
extern Sprite*  g_ci_link_middle;      /* 0x00668e8c Link_Middle.lls */
extern Sprite*  g_ci_link_bottom;      /* 0x00668e90 Link_Bottom.lls */
extern int      GetObjCost(ObjDef* d);                /* 0x00480da0 */
extern int      GetBrickCount(void);                  /* 0x004578e0 */
extern int      GetBlink(void);                       /* 0x00499480 */
extern void     Format(char* dest, const char* fmt, ...); /* 0x0049e573 */
extern void     PrintCachedText(const char* text, int x, int y, int w, int h,
                                int f1, int f2, int ink, int paper); /* 0x00455e50 */
extern int strcmp(const char* a, const char* b);
#pragma intrinsic(strcmp)

/* Draw one object-list icon: its sprite (dimmed when it costs more bricks
 * than the player has), the cost, the child link (kind 1 middle / 2 bottom),
 * the highlight while it is the object being placed (unless that is a path)
 * and the "new" flash while its class is still flagged 0x20000. Off-panel
 * icons only feed the scroll-arrow lights. */
/* Codegen notes: `buf` must be a char array of 9..11 (not 12): VC6 lays the frame's
 * aggregates out by DECLARED size ascending (each rounded up to a 12/16.. slot), so a
 * char[12] ties with the 12-byte BlitCtx and loses the tie, putting ctx at +0 instead
 * of buf. The two highlight/new sprites are picked by writing the call TWICE, once per
 * arm: VC6 merges the identical arms, hoists the four common pushes above the branch
 * and leaves only the sprite register selected by it - a `s = A; if (c) s = B;` local
 * or a ternary both emit the branch before the pushes instead. */
// FUNCTION: LEGOLAND 0x0046e0a0
int RenderBuildObjectIcon(Icon* p)
{
    char    buf[10];
    BlitCtx ctx;

    ctx.kind = 2;
    ctx.owner.p = p;
    ctx.owner.n = 0;
    if (g_edit_changed == 1 && g_edit_object) {
        if (g_edit_object->elem == ((ObjDef*)p->data)->elem)
            ((ObjDef*)p->data)->elem->type_flags &= ~0x20000;
    }
    if (p->y > 0 && p->y < 0x174) {
        int     cost = GetObjCost((ObjDef*)p->data);
        ObjDef* d;
        int     blink;
        if (p->sprite) {
            if (cost <= GetBrickCount())
                PrintSprite(p->sprite, p->x, p->y, 0, &ctx);
            else
                PrintSprite(p->sprite, p->x, p->y, g_dim_mode, &ctx);
        }
        if (p->data) {
            Format(buf, g_fmt_int, cost);
            PrintCachedText(buf, p->x + 0x3c, p->y + 0x14, 0x41, 0x14, 1, 1, 0, 0xffffff);
        }
        switch (p->kind) {
        case 1:
            PrintSprite(g_ci_link_middle, p->x, p->y, 0, 0);
            break;
        case 2:
            PrintSprite(g_ci_link_bottom, p->x, p->y, 0, 0);
            break;
        }
        if (g_edit_changed == 1) {
            d = g_edit_object;
            if (d && strcmp(g_str_path, d->name) != 0 &&
                d->elem == ((ObjDef*)p->data)->elem) {
                blink = GetBlink();
                if (!blink)
                    PrintSprite(g_ci_attract_hl_on, p->x, p->y, 0, 0);
                else
                    PrintSprite(g_ci_attract_hl_off, p->x, p->y, 0, 0);
            }
        }
        if (((ObjDef*)p->data)->elem->type_flags & 0x20000) {
            blink = GetBlink();
            if (blink)
                PrintSprite(g_ci_attract_new_off, p->x, p->y, 0, 0);
            else
                PrintSprite(g_ci_attract_new_on, p->x, p->y, 0, 0);
        }
    }
    if (((ObjDef*)p->data)->elem->type_flags & 0x20000) {
        if (p->y < 0x3e) {
            g_scroll_flags |= 1;
            return 0;
        }
        if (p->h + p->y > 0x156)
            g_scroll_flags |= 2;
    }
    return 0;
}

/* ---- the object list panel ---------------------------------------------- */
/* The side-panel state @ 0x007fdd80 (bigscreens.c's PanelState). */
typedef struct PanelState {
    char f00;      /* +0x00 0x007fdd80 */
    char pad[3];
    int  f04;      /* +0x04 0x007fdd84  1 = rebuild off-panel (x = -122) */
} PanelState;
extern PanelState g_panel_state;       /* 0x007fdd80 */
extern ObjNode*   g_object_list;       /* 0x00668e40 */
extern void  RemoveObjectListIcons(int group);                       /* 0x0046fb40 */
extern Icon* SetupInterfacePanelIcons(void* owner, int x, int y, int unused, int h, int group); /* 0x0046eaf0 */
extern Icon* AddGBarClassIcon(void* owner, ObjDef* d, int x, int y, int group, short f16);      /* 0x0046f690 */
extern void  ListChildrenBar(ObjNode* owner, int group, int x, int y);   /* 0x00475bb0 */
extern void  CloseChildrenBar(ObjNode* owner, int group, int x, int y);  /* 0x00475c00 */
extern void  RedrawObjectList(ObjListPanel* l, int dx, int dy);          /* 0x004758e0 */
extern char  BuildObjectIconInput(Icon* p, int ev, short dx, short dy);  /* 0x00470000 (not exported) */
int RenderBuildObjectIcon(Icon* p);

/* Lay the object list out as icon group `group` at (x, y), h high: the list
 * box, one class icon per kept node 0x42 apart, and for each run of child
 * nodes (keep == 0) either a close bar plus the children (kinds 1/2 draw the
 * link) when the parent class is expanded (elem flag 8) or a list bar that
 * skips them. Off-panel lists move the scroll arrows and restore the saved
 * scroll. The parent of the first child run is whatever `prev` holds —
 * uninitialised when the list starts with a child [sic]. */
// FUNCTION: LEGOLAND 0x00475960
int MakeUpObjectList(int group, int x, int y, int h)
{
    ObjListPanel* p;
    ObjNode*      node = g_object_list;
    ObjNode*      prev;
    Icon*         icon;
    Icon*         last;
    int           st;
    int           ix;
    int           iy;

    RemoveObjectListIcons(group);
    p = (ObjListPanel*)HeapAlloc_w(sizeof(ObjListPanel));
    if (!p)
        return 0;
    st = g_panel_state.f04;
    ix = x;
    if (st == 1) {
        ix = -0x7a;
        g_panel_state.f00 = 0;
    }
    icon = SetupInterfacePanelIcons(p, ix, y, 1, h, group);
    p->box = icon;
    ix = icon->x;
    p->box_x0 = ix;
    p->list_x0 = ix;
    iy = icon->y;
    p->box_y0 = iy;
    p->list_y0 = iy;
    p->box_x1 = icon->x + icon->w;
    p->box_y1 = icon->y + icon->h;
    p->f04 = 1;
    p->group = group;
    SetNewGroup_Callbacks(0, RenderBuildObjectIcon, BuildObjectIconInput);
    while (node) {
        if (node->keep) {
            AddGBarClassIcon(p, node->obj, ix, iy, group, 1);
            prev = node;
            node = node->next;
            iy += 0x42;
        } else if (prev->obj->elem->type_flags & 8) {
            iy -= 0xa;
            CloseChildrenBar(prev, group, ix, iy);
            iy += 0x1a;
            while (node && node->keep == 0) {
                last = AddGBarClassIcon(p, node->obj, ix, iy, group, 1);
                last->kind = 1;
                prev = node;
                node = node->next;
                iy += 0x38;
            }
            last->kind = 2;
            iy += 0xa;
        } else {
            iy -= 0xa;
            ListChildrenBar(prev, group, ix, iy);
            iy += 0x24;
            while (node && node->keep == 0) {
                prev = node;
                node = node->next;
            }
        }
    }
    AddFullScreenIcon(group + 6);
    p->list_x1 = ix;
    p->list_y1 = iy;
    if (iy < p->box->y + p->box->h) {
        icon = FindIcon(group + 4);
        if (icon) {
            icon->y = (short)(y + h - 0x1e);
            icon->flags |= 0x400;
        }
        icon = FindIcon(group + 3);
        if (icon)
            icon->flags |= 0x400;
        g_list_scroll[g_list_menu_u.word & 0xff] = 0;
    } else {
        RedrawObjectList(p, 0, g_list_scroll[g_list_menu_u.word & 0xff]);
    }
    g_panel_state.f04 = 0;
    return 1;
}

/* ---- the object info list ----------------------------------------------- */
/* The class record as the info list reads it: origin offsets @+0x0c/+0x10,
 * type @+0x20, exit offsets @+0x24/+0x25 and the footprint rect chain @+0x3c
 * (legoland.h's ObjClass, plus the fields this walk reads). */
typedef struct InfoCls {
    char      pad00[0x0c];
    int       ox;            /* +0x0c */
    int       oy;            /* +0x10 */
    char      pad14[0x20 - 0x14];
    short     type;          /* +0x20 */
    char      pad22[2];
    unsigned char ex;        /* +0x24 */
    unsigned char ey;        /* +0x25 */
    char      pad26[0x3c - 0x26];
    Rect      rect;          /* +0x3c */
} InfoCls;
/* A placed map object: its class record sits at +0x0c. */
typedef struct InfoMapObj {
    char      pad[0x0c];
    InfoCls*  cls;           /* +0x0c */
} InfoMapObj;
/* The map cell with the object's origin as a packed {x,y} pair @+0x04. */
typedef union CellPos {
    struct { unsigned char x, y; } b;
    unsigned short w;
    Pos            p;
} CellPos;
typedef struct InfoCell {
    InfoMapObj*    obj;      /* +0x00 */
    struct { unsigned char x, y; } origin;   /* +0x04 */
    char           pad06[0x0c - 0x06];
    unsigned short flags;    /* +0x0c 0x80 has object, 0x400 info done */
    char           pad0e[0x14 - 0x0e];
} InfoCell;
/* One info node (0x1c bytes; objmap.c's KeyNode @ 0x00669248). */
typedef struct InfoNode {
    struct InfoNode* next;   /* +0x00 */
    InfoCls*         cls;    /* +0x04 */
    unsigned short   pos;    /* +0x08 packed origin */
    char             pad0a[2];
    int              x;      /* +0x0c */
    int              y;      /* +0x10 */
    int              f14;    /* +0x14 */
    unsigned char    ex;     /* +0x18 */
    unsigned char    ey;     /* +0x19 */
    char             pad1a[2];
} InfoNode;
extern InfoNode* g_info_head;          /* 0x00669248 */
extern void ClearObjInfoList(void);     /* 0x00481170 (not exported) */
extern int  rand(void);                 /* 0x0049e4b2 (CRT) */

static __inline InfoCell* InfoCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return (InfoCell*)&g_map_rows[y][x];
    return 0;
}

/* Rebuild the object info list: for every cell holding an object of a type
 * other than 0/2 whose origin cell is not yet done, find or make the class's
 * node (an existing one is re-aimed with probability 80/256), mark the
 * origin done, skip to the end of the footprint rect on this row, and clear
 * the mark again on the footprint's bottom row (the deepest rect bottom). */
/* Codegen notes: the two loop counters are ONE `Pos cur` (an 8-byte aggregate), not two
 * ints - that is what puts the 28-byte frame in the original's order (oc +0, pos +4,
 * cur +0xc, origin +0x14; aggregates are laid out by size ascending after the scalars).
 * The rect scan adds origin.x through the scalar copy `ox`: adding the aggregate member
 * `origin.x` makes VC6 treat both operands of `r->left + origin.x` as memory and emit
 * the register first, while a scalar gives the original's `mov ecx,[eax]; add ecx,ebx`. */
// FUNCTION: LEGOLAND 0x00481200
void BuildObjInfoList(void)
{
    InfoCell* c;
    InfoCell* oc;
    InfoCls*  cls;
    InfoNode* n;
    Rect*     r;
    CellPos   pos;
    Pos       origin;
    Pos       cur;
    int       ox;
    int       top;

    ClearObjInfoList();
    for (cur.y = 0; cur.y < g_map->height; cur.y++) {
        for (cur.x = 0; cur.x < g_map->width; cur.x++) {
            c = InfoCellAt(cur.x, cur.y);
            if (!(c->flags & 0x80))
                continue;
            pos.b.x = c->origin.x;
            origin.x = pos.b.x;
            ox = origin.x;
            pos.b.y = c->origin.y;
            origin.y = pos.b.y;
            oc = InfoCellAt(origin.x, origin.y);
            cls = oc->obj->cls;
            if (cls->type != 0 && cls->type != 2 && !(oc->flags & 0x400)) {
                for (n = g_info_head; n; n = n->next)
                    if (n->cls == cls)
                        break;
                if (n) {
                    if (rand() % 256 < 0x50) {
                        n->x = cls->ox + origin.x;
                        n->y = cls->oy + origin.y;
                        n->ex = cls->ex + origin.x;
                        n->ey = cls->ey + origin.y;
                    }
                } else {
                    n = (InfoNode*)HeapAlloc_w(sizeof(InfoNode));
                    n->next = g_info_head;
                    g_info_head = n;
                    n->cls = cls;
                    n->pos = pos.w;
                    n->x = cls->ox + origin.x;
                    n->y = cls->oy + origin.y;
                    n->ex = cls->ex + origin.x;
                    n->ey = cls->ey + origin.y;
                }
                oc->flags |= 0x400;
            }
            for (r = &cls->rect; ; r = r->next) {
                if (cur.x >= r->left + ox && cur.x <= r->right + ox)
                    break;
            }
            cur.x = r->right + ox;
            top = cls->rect.bottom;
            for (r = cls->rect.next; r; r = r->next)
                if (r->bottom > top)
                    top = r->bottom;
            if (cur.y - origin.y == top)
                oc->flags &= 0xfbff;
        }
    }
}

/* ---- the pop-up info request -------------------------------------------- */
/* The pop-up record @ 0x007fdec0 .. 0x007fdfc0 (bighelp.c's PopUpUI from
 * its +0x1c): the request PopUpInfoSetUp fills, the state ResetInfoStruct
 * clears and the class elements it compares against. */
typedef struct PopUpInfo {
    int       type;       /* +0x00 0x007fdec0 0x103 object, 0x306..0x308 workers */
    void*     obj;        /* +0x04 0x007fdec4 */
    int       ref;        /* +0x08 0x007fdec8 */
    Pos       pos;        /* +0x0c 0x007fdecc */
    char      pad14[0xbc - 0x14];
    ObjDef*   cls;        /* +0xbc 0x007fdf7c */
    int       pad_c0;
    Cell*     cell;       /* +0xc4 0x007fdf84 */
    unsigned short cellpos; /* +0xc8 0x007fdf88 packed cell */
    char      pad_ca[2];
    void*     worker;     /* +0xcc 0x007fdf8c */
    int       w_f1c;      /* +0xd0 0x007fdf90 */
    int       w_f20;      /* +0xd4 0x007fdf94 */
    int       pad_d8;
    int       kind;       /* +0xdc 0x007fdf9c */
    int       active;     /* +0xe0 0x007fdfa0 */
    int       pad_e4;
    int       resize;     /* +0xe8 0x007fdfa8 */
    int       pad_ec;
    LLElem*   elem_shed;  /* +0xf0 0x007fdfb0 gardener's shed */
    LLElem*   elem_hut;   /* +0xf4 0x007fdfb4 mechanic's hut */
    void*     elem_path;  /* +0xf8 0x007fdfb8 */
    void*     elem_entr;  /* +0xfc 0x007fdfbc */
} PopUpInfo;
extern PopUpInfo g_popup;               /* 0x007fdec0 */
extern void*     g_sample_hire;         /* 0x004b92e4 */
extern void*     g_sample_fire;         /* 0x004b9308 */
/* A placed object / a worker as the pop-up reads it. */
typedef struct WorkerRec {
    char pad[0x1c];
    int  f1c;     /* +0x1c */
    int  f20;     /* +0x20 */
} WorkerRec;
typedef struct WorkOrder {
    char pad[8];
    Pos  pos;     /* +0x08 */
} WorkOrder;
typedef struct PopUpObj {
    int         f00;
    WorkerRec*  rec;      /* +0x04 */
    int         f08;
    union {
        ObjDef* cls;      /* +0x0c placed object: its class */
        short   kind;     /* +0x0c worker: its kind */
    } u;
    char        pad10[0x50 - 0x10];
    WorkOrder*  order;    /* +0x50 */
    char        pad54[0x60 - 0x54];
    unsigned char cond;   /* +0x60 */
} PopUpObj;
extern void  ResetInfoStruct(void);                      /* 0x00471510 */
extern int   CanHireGardener(void);                      /* 0x0049a120 (not exported) */
extern int   CanHireMechanic(void);                      /* 0x0049a160 (not exported) */
extern void* GenerateGardener(Pos* pos, int in_hut);     /* 0x0049a1a0 */
extern void* GenerateMechanic(Pos* pos, int in_hut);     /* 0x0049a340 */
extern void  PlayInstanceOfSample(void* def, int a, int b, void* src); /* 0x00496d20 */
extern void  WorkerPopUp(int type, PopUpObj* obj);       /* 0x00470100 (not exported) */
extern void  FreeMechanicOrder(WorkOrder* o);            /* 0x00499eb0 */

static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}
/* Same, reading the coordinates lazily through the Pos* (y after x passed). */
static __inline Cell* MapCellAtPos(Pos* p)
{
    if (p->x >= 0 && p->x < g_map->width && p->y >= 0 && p->y < g_map->height)
        return &g_map_rows[p->y][p->x];
    return 0;
}

/* Open the pop-up for `type`: a placed object (0x103: hire from a shed or
 * hut straight away, otherwise queue the object info), a worker (0x306
 * copies its record; 0x307 hires, 0x308 fires — a mechanic on a job first
 * has the job's cell unflagged and the order freed). `ref` packs the cell;
 * the by-value `pos` slots are reused as the hire position. */
/* The class element is deliberately read back through the global ('g_popup.cls->elem',
 * not 'cls->elem') AFTER both stores, and that spelling is load-bearing. The original
 * stores cls then cell then loads the element, so the load has to come last in the
 * source (VC6 will not sink a load through a pointer past stores to a global). But
 * written as 'ce = cls->elem' the load is then the LAST use of cls, so VC6 coalesces
 * the load into the dying esi ('mov esi,[esi+0xc4]'); that takes ce out of the
 * allocation pool, the shared constant zero of the 'obj == 0' guard lands in eax
 * instead of ecx, and the gardener arm needs an extra 'xor ecx,ecx' before
 * 'mov cl,[edi+4]' - 193 instructions and 8 diffs. Re-reading g_popup.cls keeps the
 * base in the global's web, VC6 forwards the just-stored value (no reload), no
 * coalescing happens, and the original's rotation (edx=elem_shed, eax=elem, ecx=zero)
 * comes back exactly. */
// FUNCTION: LEGOLAND 0x00471950
void PopUpInfoSetUp(int type, PopUpObj* obj, int ref, Pos pos)
{
    Cell*   cell;
    ObjDef* cls;
    LLElem* e;
    LLElem* ce;
    unsigned short w = (unsigned short)ref;
    int cx = w & 0xff;
    int cy = w >> 8;

    cell = MapCellAt(cx, cy);
    ResetInfoStruct();
    g_popup.active = 1;
    g_popup.resize = 1;
    g_popup.pos = pos;
    g_popup.cellpos = w;
    g_popup.type = type;
    g_popup.obj = obj;
    g_popup.ref = ref;
    switch (type) {
    case 0x306:
        g_popup.kind = type;
        g_popup.worker = obj;
        g_popup.w_f1c = obj->rec->f1c;
        g_popup.w_f20 = obj->rec->f20;
        return;
    case 0x103:
        if (obj == 0)
            return;
        if (obj == g_popup.elem_path || obj == g_popup.elem_entr)
            break;
        cls = obj->u.cls;
        e = g_popup.elem_shed;
        g_popup.cls = cls;
        g_popup.cell = cell;
        ce = g_popup.cls->elem;
        if (ce == e) {
            g_popup.active = 0;
            g_popup.kind = 0xa;
            pos.x = cell->bx;
            pos.y = cell->by;
            if (!CanHireGardener())
                return;
            GenerateGardener(&pos, 1);
            return;
        }
        if (ce == g_popup.elem_hut) {
            g_popup.active = 0;
            g_popup.kind = 0x14;
            pos.x = cell->bx;
            pos.y = cell->by;
            if (!CanHireMechanic())
                return;
            GenerateMechanic(&pos, 1);
            return;
        }
        g_popup.kind = 0x103;
        return;
    case 0x307:
        if (obj->u.kind == 5)
            break;
        PlayInstanceOfSample(g_sample_hire, 0, 1, 0);
        WorkerPopUp(0x307, obj);
        ResetInfoStruct();
        return;
    case 0x308:
        if (obj->u.kind == 5)
            break;
        if ((obj->u.kind == 0x13 && obj->cond >= 0x6b) ||
            (obj->u.kind == 0x16 && obj->cond >= 0x6b)) {
            WorkOrder* o = obj->order;
            MapCellAtPos(&o->pos)->flags &= 0xbfff;
            FreeMechanicOrder(o);
        }
        PlayInstanceOfSample(g_sample_fire, 0, 1, 0);
        WorkerPopUp(0x308, obj);
        break;
    }
    ResetInfoStruct();
}
