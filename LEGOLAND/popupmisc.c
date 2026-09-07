/* LEGOLAND — pop-up / help / side-panel odds and ends (scope AH):
 * the icon-bar sprite load/unload pair, the pop-up info and control-bar
 * sprite teardown, the main icon list renderer that skips one group, the
 * new-object strip marker loader, the cursor error message dispatcher,
 * the theme-button close, the in-game icon unload, the "what do I place
 * next" follow-up after a build, and the button-flash accessors.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, global addresses and callee argument counts are load-bearing;
 * names are ours.
 *
 * Neighbours: fpui.c (RenderIcons, AddFreePlayIcon), bighelp.c (InitPopUpInfo),
 * popup2.c (InitPopUpTools), saveprof.c (UnLoad_PopUpInfo), screens3.c (the
 * theme buttons), fpui4.c (RenderIconsExtra / g_btnflash), fpui5.c (the
 * new-object strip), gameframe.c (InGameFrame, the build path).
 */

#include "legoland.h"

#pragma intrinsic(memset)

/* ---- local types -------------------------------------------------------- */

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

/* The 0x40-byte icon record (fpui.c / iconui.c). */
typedef struct Icon {
    struct Icon*     next;      /* +0x00 */
    SpriteRec*       sprite;    /* +0x04 */
    void*            data;      /* +0x08 */
    short            x;         /* +0x0c */
    short            y;         /* +0x0e */
    short            w;         /* +0x10 */
    short            h;         /* +0x12 */
    unsigned short   group;     /* +0x14 */
    short            f16;       /* +0x16 */
    int              f18;       /* +0x18 */
    int              f1c;       /* +0x1c */
    int              f20;       /* +0x20 */
    void*            f24;       /* +0x24 */
    int            (*render)(struct Icon*);                      /* +0x28 */
    char           (*input)(struct Icon*, int ev, short dx, short dy); /* +0x2c */
    void*            widget;    /* +0x30 */
    unsigned int     flags;     /* +0x34 */
    char*            help;      /* +0x38 */
    int              help_id;   /* +0x3c */
} Icon;

/* The 5th PrintSprite argument (fpui.c's BlitCtx): {kind, owner, 0}. */
typedef struct BlitCtx {
    int kind;                            /* +0x00 */
    struct { Icon* p; int n; } owner;    /* +0x04, +0x08 */
} BlitCtx;

/* The 0xd0-byte ObjDef (llidb_odf.c); only the fields read here are named. */
typedef struct ObjDef {
    char         pad00[0x1c];
    unsigned int flags;       /* +0x1c  0x2000000 = keep placing this class */
    char         pad20[0xc4 - 0x20];
    LLElem*      elem;        /* +0xc4 this class's own LLIDB element */
} ObjDef;

/* fpui5.c's NewObjStrip: the 20 classes on the new-object strip, their
 * preview sprites and the live count. */
typedef struct NewObjStrip {
    ObjDef*    def[20];       /* +0x00  0x007fded4 */
    SpriteRec* spr[20];       /* +0x50  0x007fdf24 */
    int        count;         /* +0xa0  0x007fdf74 */
} NewObjStrip;

/* bigscreens.c's PanelState @ 0x007fdd80. */
typedef struct PanelState {
    char f00;    /* +0x00 0x007fdd80  2 = the side panel is stowed */
    int  f04;    /* +0x04 0x007fdd84  1 = rebuild off-panel */
    int  f08;    /* +0x08 0x007fdd88 */
    char f0c;    /* +0x0c 0x007fdd8c */
} PanelState;

/* One row of the build follow-up table @ 0x004bb0a4: when the class just
 * placed is `name`, the class to carry on placing is `next` (0 = itself). */
typedef struct BuildFollowUp {
    const char* name;    /* +0x00 */
    const char* next;    /* +0x04 */
} BuildFollowUp;

/* bighelp.c's GameInput; only the flags word is touched here. */
typedef struct GameInput {
    unsigned int flags;      /* +0x00 0x813a40 */
} GameInput;

/* ---- globals ------------------------------------------------------------ */
extern Icon*      g_side_icons;          /* 0x006687c8 icon list head */
extern Icon*      g_focussed_icon;       /* 0x006687d0 icon under the mouse */
extern int        g_icon_scroll_tick2;   /* 0x006688cc  RenderIconsSkipGroup's own clock (RenderIcons' is 0x006688c8) */

extern int        g_iconbar_loaded;      /* 0x006688d0 */
extern SpriteRec* g_gbar_item_sprite;    /* 0x00668828  GBarFrame.lls */
extern SpriteRec* g_scrollup_sprite;     /* 0x0066882c  IF_Side_BUp.lls */
extern SpriteRec* g_scrolldown_sprite;   /* 0x00668830  IF_Side_BDown.lls */
extern SpriteRec* g_scrollbar_sprite;    /* 0x00668834  IF_Sidebar1.lls */

extern int        g_popup_loaded;        /* 0x00668958 */
extern SpriteRec* g_pu_bg[9];            /* 0x006688e0 .. 0x00668900 */
extern SpriteRec* g_cb_bg[3];            /* 0x00668904 .. 0x0066890c */
extern SpriteRec* g_pu_delete_on;        /* 0x00668914 */
extern SpriteRec* g_pu_delete;           /* 0x00668918 */
extern SpriteRec* g_pu_close_on;         /* 0x0066891c */
extern SpriteRec* g_pu_close;            /* 0x00668920 */
extern SpriteRec* g_pu_next_on;          /* 0x00668924 */
extern SpriteRec* g_pu_next;             /* 0x00668928 */
extern SpriteRec* g_pu_prev_on;          /* 0x0066892c */
extern SpriteRec* g_pu_prev;             /* 0x00668930 */
extern SpriteRec* g_pu_ok_on;            /* 0x00668934  PU_OKON.lls */
extern SpriteRec* g_pu_ok;               /* 0x00668938  PU_OK.lls */
extern SpriteRec* g_cb_close;            /* 0x0066893c  CB_Close.lls */
extern SpriteRec* g_cb_close_on;         /* 0x00668940  CB_CloseON.lls */
extern SpriteRec* g_pu_gardener_on;      /* 0x00668944 */
extern SpriteRec* g_pu_gardener;         /* 0x00668948 */
extern SpriteRec* g_pu_mech_on;          /* 0x0066894c */
extern SpriteRec* g_pu_mech;             /* 0x00668950 */

/* bighelp.c's PopUpUI sprites, read as their own globals here. */
extern SpriteRec* g_pu_spr_full;         /* 0x007fdeac */
extern SpriteRec* g_pu_spr_norepair;     /* 0x007fdeb0 */
extern SpriteRec* g_pu_spr_sad;          /* 0x007fdfc8 */
extern SpriteRec* g_pu_spr_hungry;       /* 0x007fdfd0 */
extern SpriteRec* g_pu_spr_norm;         /* 0x007fdfe4 */
extern SpriteRec* g_pu_spr_repairok;     /* 0x007fe004 */
extern SpriteRec* g_pu_spr_peckish;      /* 0x007fe008 */
extern SpriteRec* g_pu_spr_happy;        /* 0x007fe018 */

extern NewObjStrip g_newobj;             /* 0x007fded4 */

extern int        g_cursor_error_msg[];  /* 0x004ba9ac  cursor error -> ShowMessage topic */

extern int        g_theme_closed[4];     /* 0x004bb094 legoland, castle, western, adventurers */
extern Icon*      g_active_theme_icon;   /* 0x00668eb0 */
extern SpriteRec* g_theme_legoland_off;  /* 0x007fdd40 */
extern SpriteRec* g_theme_western_off;   /* 0x007fdd44 */
extern SpriteRec* g_theme_castle_off;    /* 0x007fdd48 */
extern SpriteRec* g_theme_adv_off;       /* 0x007fdd4c */

extern int        g_interface_loaded;    /* 0x00668ebc */
extern Icon*      g_script_end_icon;     /* 0x00668eb8 */
extern Icon*      g_brief_icon;          /* 0x00668e9c */
extern PanelState g_panel_state;         /* 0x007fdd80 */

extern ObjDef*    g_edit_object;         /* 0x008119b8 */
extern int        g_edit_changed;        /* 0x008119b0 */
extern int        g_release_swallow;     /* 0x00667108 */
extern GameInput  g_input;               /* 0x00813a40 */
extern BuildFollowUp g_build_followups[29]; /* 0x004bb0a4 .. 0x004bb18c */

extern int        g_btnflash[9];         /* 0x007fdd00 */

extern char g_lls_gbarframe[];           /* 0x004ba8c8 "GBarFrame.lls" */
extern char g_lls_side_bup[];            /* 0x004ba8b8 "IF_Side_BUp.lls" */
extern char g_lls_side_bdown[];          /* 0x004ba8a4 "IF_Side_BDown.lls" */
extern char g_lls_sidebar1[];            /* 0x004ba894 "IF_Sidebar1.lls" */
extern char g_fmt_newobj_bmp[];          /* 0x004bacd8 "NewObjIcons\\%s.bmp" */
extern char g_msg_newobj_fail[];         /* 0x004bacb4 "Failied to open New Obj graphic %s\n" */

/* ---- callees ------------------------------------------------------------ */
extern SpriteRec* LoadSprite(const char* name, int mode);         /* 0x00497ab0 */
extern void       KillSprite(SpriteRec* s);                       /* 0x00497bd0 */
extern int        GetTicks(void);                                 /* 0x00499450 */
extern int        PrintSprite(SpriteRec* s, int x, int y, int mode, BlitCtx* ctx); /* 0x004853a0 */
extern void       StoreClipping(void);                            /* 0x0048a660 */
extern void       RestoreClipping(void);                          /* 0x0048a690 */
extern void       LLSPlayOnce(void* lls, ImageRec* img);          /* 0x0047d580 */
extern int        RenderFullScreenIcon(Icon* p);                  /* 0x0046df60 */
extern void       UpdateSidePanelScroll(int step);                /* 0x0046ec50 */
extern void       RenderIconsExtra(void);                         /* 0x004760a0 */
extern int        sprintf(char* buf, const char* fmt, ...);       /* 0x0049e573 */
extern void       DBPrintf(const char* fmt, ...);                 /* 0x00453a20 */
extern int        ShowMessage(int which);                         /* 0x004735e0 */
extern void       SetIconSprite(Icon* p, SpriteRec* s);           /* 0x0046d680 */
extern void       RemoveIconGroup(int group);                     /* 0x0046d520 */
extern void       RemoveIconGroupRange(int group);                /* 0x0046d590  groups [g, g+7) on both lists */
extern void       UnLoad_PopUpInfo(void);                         /* 0x00471450 */
extern int        NameCompare(const char* a, const char* b);      /* 0x004aab90 (_stricmp) */
extern LLElem*    ElemID(const char* name);                       /* 0x0047b3f0 */
extern void       SetEditObject(void* def);                       /* 0x004816e0 */

/* =========================================================================
 *  Button flash (fpui4.c's RenderIconsExtra reads g_btnflash)
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00476030
void SetButtonFlash(int which, int on)
{
    if (which >= 0 && which < 9)
        g_btnflash[which] = on;
}

// FUNCTION: LEGOLAND 0x00476050
void ClearButtonFlash(void)
{
    int i;

    for (i = 0; i < 9; i++)
        SetButtonFlash(i, 0);
}

/* =========================================================================
 *  Cursor error -> advisor message
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00473640
int ShowCursorErrorMessage(int error)
{
    return ShowMessage(g_cursor_error_msg[error]);
}

/* =========================================================================
 *  The icon bar's sprites
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0046f890
void LoadIconBarGFX(void)
{
    if (g_iconbar_loaded)
        return;
    if (!g_gbar_item_sprite)
        g_gbar_item_sprite = LoadSprite(g_lls_gbarframe, 4);
    if (!g_scrollup_sprite)
        g_scrollup_sprite = LoadSprite(g_lls_side_bup, 4);
    if (!g_scrolldown_sprite)
        g_scrolldown_sprite = LoadSprite(g_lls_side_bdown, 4);
    if (!g_scrollbar_sprite)
        g_scrollbar_sprite = LoadSprite(g_lls_sidebar1, 4);
    g_iconbar_loaded = 1;
}

// FUNCTION: LEGOLAND 0x0046f920
void UnloadIconBarGFX(void)
{
    if (!g_iconbar_loaded)
        return;
    g_iconbar_loaded = 0;
    if (g_gbar_item_sprite) {
        KillSprite(g_gbar_item_sprite);
        g_gbar_item_sprite = 0;
    }
    if (g_scrollup_sprite) {
        KillSprite(g_scrollup_sprite);
        g_scrollup_sprite = 0;
    }
    if (g_scrolldown_sprite) {
        KillSprite(g_scrolldown_sprite);
        g_scrolldown_sprite = 0;
    }
    if (g_scrollbar_sprite) {
        KillSprite(g_scrollbar_sprite);
        g_scrollbar_sprite = 0;
    }
}

/* =========================================================================
 *  The pop-up's control bar sprites (loaded by popup2.c's InitPopUpTools)
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00470b00
void UnloadPopUpTools(void)
{
    if (g_pu_ok) {
        KillSprite(g_pu_ok);
        g_pu_ok = 0;
    }
    if (g_pu_ok_on) {
        KillSprite(g_pu_ok_on);
        g_pu_ok_on = 0;
    }
    if (g_cb_close_on) {
        KillSprite(g_cb_close_on);
        g_cb_close_on = 0;
    }
    if (g_cb_close) {
        KillSprite(g_cb_close);
        g_cb_close = 0;
    }
    if (g_cb_bg[0]) {
        KillSprite(g_cb_bg[0]);
        g_cb_bg[0] = 0;
    }
    if (g_cb_bg[1]) {
        KillSprite(g_cb_bg[1]);
        g_cb_bg[1] = 0;
    }
    if (g_cb_bg[2]) {
        KillSprite(g_cb_bg[2]);
        g_cb_bg[2] = 0;
    }
}

/* =========================================================================
 *  The theme buttons
 * ========================================================================= */

/* Mark the open theme's menu closed and put its button back to the OFF
 * sprite. The closed flags run legoland, castle, western, adventurers
 * (screens3.c). Only the first open theme is closed. */
// FUNCTION: LEGOLAND 0x00474750
void CloseActiveThemeButton(void)
{
    if (!g_theme_closed[0]) {
        g_theme_closed[0] = 1;
        SetIconSprite(g_active_theme_icon, g_theme_legoland_off);
    } else if (!g_theme_closed[1]) {
        g_theme_closed[1] = 1;
        SetIconSprite(g_active_theme_icon, g_theme_castle_off);
    } else if (!g_theme_closed[2]) {
        g_theme_closed[2] = 1;
        SetIconSprite(g_active_theme_icon, g_theme_western_off);
    } else if (!g_theme_closed[3]) {
        g_theme_closed[3] = 1;
        SetIconSprite(g_active_theme_icon, g_theme_adv_off);
    }
}

/* =========================================================================
 *  The in-game interface icons
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00474ed0
void UnLoadInGameIcons(void)
{
    if (!g_interface_loaded)
        return;
    g_interface_loaded = 0;
    RemoveIconGroup(0x93);
    g_script_end_icon = 0;
    RemoveIconGroup(0x9a);
    g_brief_icon = 0;
    RemoveIconGroupRange(0xd2);
    UnLoad_PopUpInfo();
    g_panel_state.f00 = 2;
    g_panel_state.f04 = 1;
    g_panel_state.f08 = 0;
    g_panel_state.f0c = (char)0x86;
}

/* =========================================================================
 *  The new-object strip (fpui5.c / misc3.c)
 * ========================================================================= */

/* Load "NewObjIcons\<class>.bmp" and append the class to the strip. */
// FUNCTION: LEGOLAND 0x00471c10
void AddNewObjectMarker(ObjDef* cls)
{
    char buf[0x200];

    if (g_newobj.count >= 20)
        return;
    sprintf(buf, g_fmt_newobj_bmp, cls->elem->name);
    g_newobj.spr[g_newobj.count] = LoadSprite(buf, 0);
    if (g_newobj.spr[g_newobj.count]) {
        g_newobj.def[g_newobj.count] = cls;
        g_newobj.count++;
    } else {
        DBPrintf(g_msg_newobj_fail, buf);
    }
}

/* =========================================================================
 *  What to place next, after a successful build
 * ========================================================================= */

/* A class flagged 0x2000000 keeps itself; otherwise the follow-up table maps
 * the class name to the element to carry on with (its own element when the
 * row's `next` is 0). The table is walked to its END, so a later row wins.
 * An element flagged 2 becomes the edit object; anything else drops the
 * cursor: clear the click/drag bits, edit mode 0, swallow the release. */
// FUNCTION: LEGOLAND 0x00475f40
void SelectNextBuildObject(void)
{
    ObjDef*  def = g_edit_object;
    LLElem*  elem = def->elem;
    LLElem*  next = 0;

    if (def->flags & 0x2000000) {
        next = elem;
    } else {
        const char** e;

        /* Biased cursor: anchored at the row's `next` (LP08). */
        for (e = &g_build_followups[0].next; e < &g_build_followups[29].next; e += 2) {
            if (NameCompare(e[-1], elem->name) == 0) {
                if (e[0])
                    next = ElemID(e[0]);
                else
                    next = elem;
            }
        }
    }
    if (next && (next->type_flags & 2)) {
        SetEditObject(next->data);
        return;
    }
    g_edit_changed = 0;
    g_release_swallow = 1;
    g_input.flags &= ~0x1400;
}

/* =========================================================================
 *  The main icon list, all but one group
 * ========================================================================= */

/* fpui.c's RenderIcons with the group `skip` left out (InGameFrame draws the
 * pop-up group 0x2c3 afterwards through RenderIcons2) and no focus box. */
// FUNCTION: LEGOLAND 0x0046f100
void RenderIconsSkipGroup(unsigned short skip)
{
    BlitCtx ctx;
    Icon*   p;
    int     dt;

    ctx.kind = 2;
    memset(&ctx.owner, 0, sizeof(ctx.owner));
    dt = GetTicks() - g_icon_scroll_tick2;
    if (dt > 990)
        dt = 990;
    UpdateSidePanelScroll(dt * 5 / 33);
    g_icon_scroll_tick2 = GetTicks();
    p = g_side_icons;
    StoreClipping();
    RenderFullScreenIcon(0);
    for (; p; p = p->next) {
        if (!(p->flags & 0x400) && p->group != skip) {
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
    RenderFullScreenIcon(0);
    RenderIconsExtra();
    RestoreClipping();
}

/* =========================================================================
 *  The pop-up info sprites (loaded by bighelp.c's InitPopUpInfo)
 * ========================================================================= */

/* Tail-jumped from saveprof.c's UnLoad_PopUpInfo. Kills the nine panel
 * slices (in slot order 0,1,2,4,3,5..8), the two repair sprites, the seven
 * gadget pairs, the six mood sprites, then the control bar's set. */
// FUNCTION: LEGOLAND 0x00471170
void KillPopUpInfoSprites(void)
{
    if (!g_popup_loaded)
        return;
    g_popup_loaded = 0;
    if (g_pu_bg[0]) { KillSprite(g_pu_bg[0]); g_pu_bg[0] = 0; }
    if (g_pu_bg[1]) { KillSprite(g_pu_bg[1]); g_pu_bg[1] = 0; }
    if (g_pu_bg[2]) { KillSprite(g_pu_bg[2]); g_pu_bg[2] = 0; }
    if (g_pu_bg[4]) { KillSprite(g_pu_bg[4]); g_pu_bg[4] = 0; }
    if (g_pu_bg[3]) { KillSprite(g_pu_bg[3]); g_pu_bg[3] = 0; }
    if (g_pu_bg[5]) { KillSprite(g_pu_bg[5]); g_pu_bg[5] = 0; }
    if (g_pu_bg[6]) { KillSprite(g_pu_bg[6]); g_pu_bg[6] = 0; }
    if (g_pu_bg[7]) { KillSprite(g_pu_bg[7]); g_pu_bg[7] = 0; }
    if (g_pu_bg[8]) { KillSprite(g_pu_bg[8]); g_pu_bg[8] = 0; }
    if (g_pu_spr_repairok) { KillSprite(g_pu_spr_repairok); g_pu_spr_repairok = 0; }
    if (g_pu_spr_norepair) { KillSprite(g_pu_spr_norepair); g_pu_spr_norepair = 0; }
    if (g_pu_delete_on) { KillSprite(g_pu_delete_on); g_pu_delete_on = 0; }
    if (g_pu_delete) { KillSprite(g_pu_delete); g_pu_delete = 0; }
    if (g_pu_close_on) { KillSprite(g_pu_close_on); g_pu_close_on = 0; }
    if (g_pu_close) { KillSprite(g_pu_close); g_pu_close = 0; }
    if (g_pu_next) { KillSprite(g_pu_next); g_pu_next = 0; }
    if (g_pu_next_on) { KillSprite(g_pu_next_on); g_pu_next_on = 0; }
    if (g_pu_prev) { KillSprite(g_pu_prev); g_pu_prev = 0; }
    if (g_pu_prev_on) { KillSprite(g_pu_prev_on); g_pu_prev_on = 0; }
    if (g_pu_gardener_on) { KillSprite(g_pu_gardener_on); g_pu_gardener_on = 0; }
    if (g_pu_gardener) { KillSprite(g_pu_gardener); g_pu_gardener = 0; }
    if (g_pu_mech_on) { KillSprite(g_pu_mech_on); g_pu_mech_on = 0; }
    if (g_pu_mech) { KillSprite(g_pu_mech); g_pu_mech = 0; }
    if (g_pu_spr_sad) { KillSprite(g_pu_spr_sad); g_pu_spr_sad = 0; }
    if (g_pu_spr_norm) { KillSprite(g_pu_spr_norm); g_pu_spr_norm = 0; }
    if (g_pu_spr_happy) { KillSprite(g_pu_spr_happy); g_pu_spr_happy = 0; }
    if (g_pu_spr_hungry) { KillSprite(g_pu_spr_hungry); g_pu_spr_hungry = 0; }
    if (g_pu_spr_peckish) { KillSprite(g_pu_spr_peckish); g_pu_spr_peckish = 0; }
    if (g_pu_spr_full) { KillSprite(g_pu_spr_full); g_pu_spr_full = 0; }
    UnloadPopUpTools();
}
