/* LEGOLAND — side panel, menu and icon UI.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field offsets, callee arg counts and global addresses are load-bearing;
 * names are ours.
 */
#include "legoland.h"

/* ---- the icon record ---------------------------------------------------- */
/* One entry of the side-panel icon list (head @ 0x006687c8; a second list
 * head @ 0x006687cc is walked by RemoveIconGroup). 0x40 bytes, allocated by
 * InsertIcon (0x0046d6c0) from a 0x40-byte template @ 0x00668858 (rep movsd,
 * 0x10 dwords). money.c's Gadget is the same record seen by the renderers:
 * next @+0x00, sprite @+0x04, x/y/w/h as signed shorts @+0x0c..+0x12, group
 * id (word) @+0x14, render callback @+0x28, flags dword @+0x34.
 *   flags: 0x01/0x08/0x20 set by AddFullScreenIcon, 0x10 "has sprite size"
 *   (InsertIcon), 0x200/0x08 by the box icon, 0x400 = DISABLED (the side-panel
 *   enable/disable pair below), 0x2002 by the help icon toggle (0x004748a0). */
typedef struct Icon {
    struct Icon*   next;      /* +0x00 */
    Sprite*        sprite;    /* +0x04 */
    int            f08;       /* +0x08 */
    short          x;         /* +0x0c */
    short          y;         /* +0x0e */
    short          w;         /* +0x10 */
    short          h;         /* +0x12 */
    unsigned short group;     /* +0x14 */
    char           pad16[0x28 - 0x16];
    int          (*render)(struct Icon*);  /* +0x28 */
    int            f2c;       /* +0x2c */
    int            f30;       /* +0x30 */
    unsigned int   flags;     /* +0x34 */
    char           pad38[0x40 - 0x38];
} Icon;

/* The 5th PrintSprite argument, as in money.c: {kind, owner, 0}; kind 2 is
 * "owned by an icon". */
typedef struct BlitCtx {
    int   kind;    /* +0x00 */
    Icon* owner;   /* +0x04 */
    int   f08;     /* +0x08 */
} BlitCtx;

/* The global game record @ 0x004bcbf4 (legoland.h calls it g_map and uses its
 * map extent at +0x14/+0x16). Its first two words are the screen size in
 * pixels, which is what a full-screen icon is sized to. */
typedef struct ScreenDims {
    short width;    /* +0x00 */
    short height;   /* +0x02 */
} ScreenDims;
extern ScreenDims* g_screen;   /* 0x004bcbf4 — the same object as g_map */

/* ---- menus -------------------------------------------------------------- */
/* The menu table @ 0x004bafa8: 20-byte records, indexed by g_menu_index
 * (0x004baff8); index 5 means "no menu open". Only the record's address is
 * used here, so the layout is opaque. */
typedef struct Menu {
    int f[5];
} Menu;
extern Menu g_menus[];          /* 0x004bafa8 */
extern int  g_menu_index;       /* 0x004baff8 */

/* ---- globals ------------------------------------------------------------ */
extern Icon* g_side_icons;             /* 0x006687c8 icon list head */
extern int   g_frontend_checkbox_closed; /* 0x004bef9c */
extern int   g_savebk[8];              /* 0x00798708 */

/* ---- callees ------------------------------------------------------------ */
extern int   ObjectLinkedList(Menu* m);                       /* 0x00475720 */
extern void  RemoveIconGroup(int group);                      /* 0x0046d520 */
extern void  KillFrontEndCheckBoxSprite(void);                /* 0x0048c9a0 */
extern void  UnLoad_Interface_ControlIcons(void);             /* 0x004743b0 */
extern void  RemoveObjectListIcons(int group);                /* 0x0046fb40 — FindIcon(group+5), frees the group */
extern void  UnLoad_PopUpInfo(void);                          /* 0x00471450 */
extern Icon* InsertIcon(int a, int b, int group, Sprite* s);  /* 0x0046d6c0 */
extern int   RenderFullScreenIcon(Icon* icon);                /* 0x0046df60 */
extern int   PrintSprite(Sprite* s, int x, int y, int mode, BlitCtx* ctx); /* 0x004853a0 */

/* Rebuild the object linked list for menu `m`; 1 on success. */
// FUNCTION: LEGOLAND 0x00475710
int TestMenu(Menu* m)
{
    return ObjectLinkedList(m);
}

// FUNCTION: LEGOLAND 0x0048cc10
void CloseFontEndCheckBox(void)
{
    RemoveIconGroup(0xe);
    KillFrontEndCheckBoxSprite();
    g_frontend_checkbox_closed = 1;
}

// FUNCTION: LEGOLAND 0x004758c0
void UpdateMenu(void)
{
    if (g_menu_index != 5)
        TestMenu(&g_menus[g_menu_index]);
}

/* The four side-panel icon groups 0xd2, 0xd5, 0xd6, 0xd7 get the DISABLED
 * flag (0x400) set / cleared. */
// FUNCTION: LEGOLAND 0x00475e90
void DisableSidePanelIcons(void)
{
    Icon* p;

    for (p = g_side_icons; p; p = p->next) {
        if (p->group == 0xd2 || p->group == 0xd5 ||
            p->group == 0xd6 || p->group == 0xd7)
            p->flags |= 0x400;
    }
}

// FUNCTION: LEGOLAND 0x00475ed0
void EnableSidePanelIcons(void)
{
    Icon* p;

    for (p = g_side_icons; p; p = p->next) {
        if (p->group == 0xd2 || p->group == 0xd5 ||
            p->group == 0xd6 || p->group == 0xd7)
            p->flags &= ~0x400;
    }
}

/* matchfull: 5/5 = 100%, and the original's true extent is exactly these five
 * instructions (23 bytes, 0x00474800..0x00474816). It is marked WIP only
 * because tools/audit.py cannot bound it: the function ends in a tail
 * `jmp UnLoad_PopUpInfo` with NO `ret`, so audit's "first ret not jumped past"
 * walk runs through the padding into the next function (0x00474820) and
 * reports orig=19i/47B. Nothing in the C can change that -- any non-tail form
 * would emit call/ret and stop matching. Promote to FUNCTION once audit.py
 * treats an unconditional jmp out of the function as a terminator. */
// FUNCTION: LEGOLAND 0x00474800
void UnLoad_Interface_Icons(void)
{
    UnLoad_Interface_ControlIcons();
    RemoveObjectListIcons(0xd2);
    UnLoad_PopUpInfo();
}

/* A sprite-less icon covering the whole screen (group `group`). */
// FUNCTION: LEGOLAND 0x0046d760
void AddFullScreenIcon(int group)
{
    Icon* p = InsertIcon(0, 0, group, 0);
    if (p) {
        p->w = g_screen->width;
        p->h = g_screen->height;
        p->render = RenderFullScreenIcon;
        p->flags |= 0x29;
    }
}

// FUNCTION: LEGOLAND 0x0046e850
int RenderGBarSpriteIcon(Icon* icon)
{
    BlitCtx ctx;

    ctx.kind = 2;
    ctx.owner = icon;
    ctx.f08 = 0;
    if (icon->sprite)
        PrintSprite(icon->sprite, icon->x, icon->y, 0, &ctx);
    return 0;
}

/* Save-panel backdrop for slot 1..8; 0 for anything else. */
// FUNCTION: LEGOLAND 0x0048db50
int GetSavePanelBK(char slot)
{
    switch (slot) {
    case 1: return g_savebk[0];
    case 2: return g_savebk[1];
    case 3: return g_savebk[2];
    case 4: return g_savebk[3];
    case 5: return g_savebk[4];
    case 6: return g_savebk[5];
    case 7: return g_savebk[6];
    case 8: return g_savebk[7];
    }
    return 0;
}
