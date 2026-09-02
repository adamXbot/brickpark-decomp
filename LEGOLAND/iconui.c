/* LEGOLAND — icon list, on-screen indicators and the help-text flow.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field offsets, callee arg counts and global addresses are load-bearing;
 * names are ours.
 */
#include "legoland.h"

unsigned int strlen(const char*);
char* strcpy(char*, const char*);
#pragma intrinsic(strlen, strcpy)

/* ---- the icon record ---------------------------------------------------- */
/* Same 0x40-byte record as panelui.c's Icon / money.c's Gadget. InsertIcon
 * clones it from the template @ 0x00668858 and links it with sub_46d440.
 *   next @+0x00, sprite @+0x04, data @+0x08 (object descriptor for in-game
 *   help), x/y/w/h signed shorts @+0x0c..+0x12, group u16 @+0x14, owner
 *   @+0x18 (ListChildrenBar stores its first argument here), render cb
 *   @+0x28, input cb @+0x2c, widget/back-pointer @+0x30, flags @+0x34,
 *   help text @+0x38, help string id @+0x3c. */
typedef struct Icon {
    struct Icon*   next;      /* +0x00 */
    Sprite*        sprite;    /* +0x04 */
    void**         data;      /* +0x08 */
    short          x;         /* +0x0c */
    short          y;         /* +0x0e */
    short          w;         /* +0x10 */
    short          h;         /* +0x12 */
    unsigned short group;     /* +0x14 */
    short          pad16;     /* +0x16 */
    int            owner;     /* +0x18 */
    char           pad1c[0x28 - 0x1c];
    int          (*render)(struct Icon*);          /* +0x28 */
    char         (*input)(struct Icon*, int);      /* +0x2c */
    void*          widget;    /* +0x30 */
    unsigned int   flags;     /* +0x34 */
    char*          help;      /* +0x38 */
    int            help_id;   /* +0x3c */
} Icon;

/* ---- the indicator record (0x28 bytes) ---------------------------------- */
/* Two singly linked lists: g_ind_inactive (0x006688d4, where Allocate*
 * pushes a fresh one) and g_ind_active (0x006688d8, walked by the renderer).
 * AddIndicator moves a record from the inactive to the active list and
 * RemoveIndicator moves it back; flag 0x08 mirrors which list it is on,
 * flag 0x01 = permanent (never times out). */
typedef struct Indicator {
    struct Indicator* next;      /* +0x00 */
    unsigned int      flags;     /* +0x04  0x01 permanent, 0x08 active */
    int               start;     /* +0x08  GetGameTimer() at allocation */
    int               duration;  /* +0x0c  timed only */
    int               arg;       /* +0x10 */
    Icon*             icon;      /* +0x14 */
    int               f18;       /* +0x18 */
    int               f1c;       /* +0x1c */
    char              pad20[0x28 - 0x20];
} Indicator;

/* The bubble-help anchor rectangle handed to HTBubbleHelp: the mouse point
 * doubled, with the top pulled up 10 pixels. */
typedef struct HelpRect {
    int x0;   /* +0x00 */
    int y0;   /* +0x04 */
    int x1;   /* +0x08 */
    int y1;   /* +0x0c */
} HelpRect;

/* ---- globals ------------------------------------------------------------ */
extern Icon       g_icon_template;     /* 0x00668858 the 0x40-byte template InsertIcon clones */
extern Icon*      g_side_icons;        /* 0x006687c8 icon list head */
extern Icon*      g_side_icons2;       /* 0x006687cc second icon list head */
extern Icon*      g_focussed_icon;     /* 0x006687d0 icon under the mouse */
extern Indicator* g_ind_inactive;      /* 0x006688d4 */
extern Indicator* g_ind_active;        /* 0x006688d8 */
extern Pos        g_gfx_point;         /* 0x00813a44 mouse point */
extern int        g_advisor_state;     /* 0x007fe040  bit0 = advisor text up */
extern int        g_advisor_arg;       /* 0x007fe044 */
extern char*      g_advisor_text;      /* 0x007fe048 */
extern int        g_advisor_start;     /* 0x007fe04c */
extern int        g_advisor_last;      /* 0x007fe050 */
extern int        g_ingame_help_tick;  /* 0x007fe054 */
extern int        g_bubble_rect;       /* 0x004b9f78 (address only) */

/* The info pop-up panel icons (Load_PopUpInfo's ten gadgets). */
extern Icon* g_info_icon_a;   /* 0x007fdfdc */
extern Icon* g_info_icon_b;   /* 0x007fdfc0 */
extern Icon* g_info_icon_c;   /* 0x007fdfd8 */
extern Icon* g_info_icon_d;   /* 0x007fdea8 */
extern Icon* g_info_icon_e;   /* 0x007fe000 */
extern Icon* g_info_icon_f;   /* 0x007fdfe0 */
extern Icon* g_info_icon_g;   /* 0x007fdea4 */
extern Icon* g_info_icon_h;   /* 0x007fdfcc */
extern Icon* g_info_icon_i;   /* 0x007fdfc4 */
extern Icon* g_info_icon_j;   /* 0x007fdfe8 */

/* The info struct ResetInfoStruct clears (0x007fdf7c..0x007fdfac). */
extern int  g_info_f7c;   /* 0x007fdf7c */
extern int  g_info_f84;   /* 0x007fdf84 */
extern int  g_info_f8c;   /* 0x007fdf8c */
extern int  g_info_f98;   /* 0x007fdf98 */
extern int  g_info_f9c;   /* 0x007fdf9c */
extern int  g_info_fa0;   /* 0x007fdfa0 */
extern int  g_info_fa4;   /* 0x007fdfa4 */
extern int  g_info_fa8;   /* 0x007fdfa8 */
extern char g_info_fac;   /* 0x007fdfac */

extern char g_children_bar_lls[];       /* 0x004bb4a8 */
extern char g_close_children_bar_lls[]; /* 0x004bb4bc */

/* ---- callees ------------------------------------------------------------ */
extern void*   HeapAlloc_w(unsigned int size);                /* 0x0049e4ff */
extern void    HeapFree_w(void* p);                           /* 0x0049e4d0 */
extern Sprite* LoadSprite(const char* name, int mode);        /* 0x00497ab0 */
extern void    ReferenceSprite(Sprite* s);                    /* 0x00497bb0 */
extern void    KillSprite(Sprite* s);                         /* 0x00497bd0 */
extern char*   GetString(int id);                             /* 0x00498f50 */
extern int     GetGameTimer(void);                            /* 0x00499430 */
extern void    LinkIcon(Icon* p);                             /* 0x0046d440 */
extern void    UnlinkIcon(Icon** link);                       /* 0x0046d460 */
extern void    UnlinkIcon2(Icon** link);                      /* 0x0046d4a0 */
extern void    RemoveIcon(Icon* p);                           /* 0x0046d4e0 */
extern int     RenderIndicatorIcon(Icon* p);                  /* 0x0046eaa0 */
extern char    IndicatorInput(Icon* p, int ev);               /* 0x0046fbc0 */
extern char    ChildrenBarInput(Icon* p, int ev);             /* 0x00475c50 */
extern char    CloseChildrenBarInput(Icon* p, int ev);        /* 0x00475c90 */
extern void    RenderHelpIcons(void);                         /* 0x0046f200 */
extern void    HTBubbleHelp(HelpRect* r, char* text, int mode); /* 0x004557c0 */
extern void    BubbleHelp(int* r, char* text, int mode);      /* 0x00455370 */
extern void    ShowObjectHelp(void* obj);                     /* 0x0046d340 */
extern void    ShowIdHelp(int id);                            /* 0x0046d230 */
extern void    KillHelpText(void);                            /* 0x0046c5c0 */
extern void    ResetInfoPopUp(void);                          /* 0x004714a0 */
extern void    ToggleHelpIcon(int on);                        /* 0x004748a0 */
extern void    UpdateHelpCursor(void);                        /* 0x00469400 */
extern void    UpdateHelpTick(void);                          /* 0x0046b2d0 */
extern int     AdvisorHelpExpired(void);                      /* 0x0046cf20 */
extern void    KillAdvisorHelp(void);                         /* 0x0046ce20 */
extern int     ObjectHelpExpired(void);                       /* 0x0046cee0 */
extern void    KillObjectHelp(void);                          /* 0x00468c00 */
extern void    ProcessHelpKeys(void);                         /* 0x00473660 */

Icon* InsertIcon(short x, short y, unsigned short group, Sprite* s);
void  DisableInfoPopUPIcons(void);

/* ---- help --------------------------------------------------------------- */

/* matchfull: 1/1. Tail `jmp` with no ret; audit.py bounds it, match.py cannot. */
// WIP-FUNCTION: LEGOLAND 0x0046d100  (100% by audit.py; match.py cannot bound a tail-jmp function)
void KillHelp(void)
{
    KillHelpText();
}

/* Puts the advisor text `text` up (once): copies it, stamps the timer and
 * lights the help icon. 0 if advisor help is already up. */
// FUNCTION: LEGOLAND 0x0046ce60
int DisplayAdvisorHelp(const char* text, int arg)
{
    if (g_advisor_state & 1)
        return 0;
    g_advisor_state |= 1;
    g_advisor_text = (char*)HeapAlloc_w(strlen(text) + 1);
    strcpy(g_advisor_text, text);
    g_advisor_start = GetGameTimer();
    g_advisor_arg = arg;
    ToggleHelpIcon(0);
    return 1;
}

/* Per-frame in-game help: cursor/tick updates, advisor text or bubble help,
 * the help icons, then the help keys. */
// WIP-FUNCTION: LEGOLAND 0x0046cf60  (100% by audit.py; match.py cannot bound a tail-jmp function)
void ProcessInGameHelp(void)
{
    UpdateHelpCursor();
    if (GetGameTimer() - g_ingame_help_tick > 200) {
        UpdateHelpTick();
        g_ingame_help_tick = GetGameTimer();
    }
    if (g_advisor_state & 1) {
        if (AdvisorHelpExpired()) {
            KillAdvisorHelp();
            g_advisor_last = GetGameTimer();
        } else {
            BubbleHelp(&g_bubble_rect, g_advisor_text, 2);
            g_advisor_last = GetGameTimer();
        }
    } else {
        if (ObjectHelpExpired())
            KillObjectHelp();
    }
    RenderHelpIcons();
    ProcessHelpKeys();
}

/* Front-end help: the focussed icon's bubble text at the mouse, then its
 * object (0x1000) or string-id help. */
// FUNCTION: LEGOLAND 0x0046d080
void ProcessFrontEndHelp(void)
{
    HelpRect r;

    RenderHelpIcons();
    if (g_focussed_icon) {
        r.x0 = g_gfx_point.x;
        r.y0 = g_gfx_point.y - 10;
        r.x1 = g_gfx_point.x;
        r.y1 = g_gfx_point.y;
        if (g_focussed_icon->flags & 0x2000) {
            HTBubbleHelp(&r, g_focussed_icon->help, 2);
            if (g_focussed_icon->flags & 0x1000)
                ShowObjectHelp(*g_focussed_icon->data);
            else
                ShowIdHelp(g_focussed_icon->help_id);
        }
    }
}

/* ---- the info pop-up ---------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00471580
void DisableInfoPopUPIcons(void)
{
    g_info_icon_a->flags |= 0x400;
    g_info_icon_b->flags |= 0x400;
    g_info_icon_c->flags |= 0x400;
    g_info_icon_d->flags |= 0x400;
    g_info_icon_e->flags |= 0x400;
    g_info_icon_f->flags |= 0x400;
    g_info_icon_g->flags |= 0x400;
    g_info_icon_h->flags |= 0x400;
    g_info_icon_i->flags |= 0x400;
    g_info_icon_j->flags |= 0x400;
}

/* matchfull: full. Tail `jmp` with no ret; audit.py bounds it, match.py cannot. */
// WIP-FUNCTION: LEGOLAND 0x00471510  (100% by audit.py; match.py cannot bound a tail-jmp function)
void ResetInfoStruct(void)
{
    g_info_f7c = 0;
    g_info_f8c = 0;
    g_info_f84 = 0;
    g_info_f9c = 0;
    g_info_fa0 = 0;
    g_info_fa4 = 0;
    g_info_fa8 = 0;
    g_info_f98 = 0;
    g_info_fac = 0;
    DisableInfoPopUPIcons();
    ResetInfoPopUp();
}

/* ---- the icon list ------------------------------------------------------ */

/* First icon of `group` on either list that is not hidden (0x100). */
// FUNCTION: LEGOLAND 0x0046d630
Icon* FindIcon(unsigned short group)
{
    Icon* p;

    for (p = g_side_icons; p; p = p->next) {
        if (p->group == group && !(p->flags & 0x100))
            return p;
    }
    for (p = g_side_icons2; p; p = p->next) {
        if (p->group == group && !(p->flags & 0x100))
            return p;
    }
    return 0;
}

/* Clone the icon template, bind (and reference) the sprite, size the icon
 * to it, link it in. */
// FUNCTION: LEGOLAND 0x0046d6c0
Icon* InsertIcon(short x, short y, unsigned short group, Sprite* s)
{
    Icon* p = (Icon*)HeapAlloc_w(0x40);
    if (p) {
        *p = g_icon_template;
        p->x = x;
        p->y = y;
        p->group = group;
        ReferenceSprite(s);
        p->sprite = s;
        if (s) {
            short w = s->w;
            short h = s->h;
            p->w = w;
            p->h = h;
            p->flags |= 0x10;
        }
    }
    LinkIcon(p);
    return p;
}

/* Load a sprite by name and make an icon of it; the icon holds its own
 * reference, so the load's is dropped again. */
// FUNCTION: LEGOLAND 0x0046d7b0
Icon* LoadSpriteIcon(const char* name, int mode, int x, int y, int group)
{
    Icon* p = 0;
    Sprite* s = LoadSprite(name, mode);
    if (s)
        p = InsertIcon(x, y, group, s);
    KillSprite(s);
    return p;
}

/* Unlink and free every icon of `group` on both lists.
 *
 * The original keeps a count of the icons unlinked from the first list
 * (`xor ebx,ebx` / `inc ebx`) although nothing observable reads it. VC6 only
 * deletes the increments when the reader is already gone at dead-store
 * elimination time; a loop whose sole effect is on the count is removed
 * later than that and leaves the counter behind. The count was evidently
 * consumed by a loop that is empty in the release build (a compiled-out
 * debug walk); the trailing empty countdown below reproduces exactly that
 * retained counter and emits no code of its own. */
// FUNCTION: LEGOLAND 0x0046d520
void RemoveIconGroup(unsigned short group)
{
    Icon** link;
    Icon*  p;
    int    n = 0;

    link = &g_side_icons;
    for (p = g_side_icons; p; ) {
        if (p->group == group) {
            p = p->next;
            UnlinkIcon(link);
            n++;
        } else {
            link = &p->next;
            p = p->next;
        }
    }
    link = &g_side_icons2;
    for (p = g_side_icons2; p; ) {
        if (p->group == group) {
            p = p->next;
            UnlinkIcon2(link);
        } else {
            link = &p->next;
            p = p->next;
        }
    }
    while (n--)           /* see the note above: keeps `n` live */
        ;
}

/* Shift every unlocked (flag 0x01 clear) icon whose masked group matches. */
// FUNCTION: LEGOLAND 0x0046dcd0
void MoveIcons(int mask, short group, short dx, short dy)
{
    Icon* p;

    for (p = g_side_icons; p; p = p->next) {
        if (!(p->flags & 1) && (short)(p->group & mask) == group) {
            p->x += dx;
            p->y += dy;
        }
    }
}

/* ---- the children bar --------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00475bb0
void ListChildrenBar(int owner, int group, int x, int y)
{
    Icon* p = LoadSpriteIcon(g_children_bar_lls, 4, x, y, group);
    p->help_id = 0x64;
    p->help = GetString(0x64);
    p->flags |= 0x2002;
    p->input = ChildrenBarInput;
    p->owner = owner;
}

// FUNCTION: LEGOLAND 0x00475c00
void CloseChildrenBar(int owner, int group, int x, int y)
{
    Icon* p = LoadSpriteIcon(g_close_children_bar_lls, 4, x, y, group);
    p->help_id = 0x65;
    p->help = GetString(0x65);
    p->flags |= 0x2002;
    p->input = CloseChildrenBarInput;
    p->owner = owner;
}

/* ---- indicators --------------------------------------------------------- */

/* A timed indicator: its icon (group 0xe000, hidden+disabled+... 0x40a) is
 * created here and points back at the record. */
// FUNCTION: LEGOLAND 0x0046fbf0
Indicator* AllocateTimedIndicator(Sprite* s, int duration, int arg)
{
    Indicator* ind = (Indicator*)HeapAlloc_w(0x28);
    ind->next = g_ind_inactive;
    ind->flags = 0;
    ind->start = GetGameTimer();
    ind->duration = duration;
    ind->arg = arg;
    ReferenceSprite(s);
    ind->icon = InsertIcon(0, 0, 0xe000, s);
    ind->icon->render = RenderIndicatorIcon;
    ind->icon->input = IndicatorInput;
    ind->icon->flags |= 0x40a;
    ind->icon->widget = ind;
    ind->f18 = 0;
    ind->f1c = 0;
    g_ind_inactive = ind;
    return ind;
}

// FUNCTION: LEGOLAND 0x0046fc80
Indicator* AllocatePermanentIndicator(Sprite* s, int arg)
{
    Indicator* ind = (Indicator*)HeapAlloc_w(0x28);
    ind->next = g_ind_inactive;
    ind->flags = 1;
    ind->start = GetGameTimer();
    ind->arg = arg;
    ReferenceSprite(s);
    ind->icon = InsertIcon(0, 0, 0xe000, s);
    ind->icon->render = RenderIndicatorIcon;
    ind->icon->input = IndicatorInput;
    ind->icon->flags |= 0x40a;
    ind->icon->widget = ind;
    ind->f18 = 0;
    ind->f1c = 0;
    g_ind_inactive = ind;
    return ind;
}

/* Move `p` from the inactive list to the head of the active list, park its
 * icon off-screen and enable it. */
// FUNCTION: LEGOLAND 0x0046fd40
void AddIndicator(Indicator* p)
{
    Indicator* q = g_ind_inactive;

    if (q == p) {
        g_ind_inactive = q->next;
    } else {
        while (q) {
            if (q->next == p) {
                q->next = p->next;
                break;
            }
            q = q->next;
        }
    }
    if (q) {
        p->next = g_ind_active;
        g_ind_active = p;
        p->flags |= 8;
        p->icon->x = (short)0xf000;
        p->icon->flags &= ~0x400;
    }
}

/* Move `p` from the active list back to the inactive list and disable its
 * icon. */
// FUNCTION: LEGOLAND 0x0046fda0
void RemoveIndicator(Indicator* p)
{
    Indicator* q = g_ind_active;

    if (q == p) {
        g_ind_active = q->next;
    } else {
        while (q) {
            if (q->next == p) {
                q->next = p->next;
                break;
            }
            q = q->next;
        }
    }
    if (q) {
        p->next = g_ind_inactive;
        g_ind_inactive = p;
        p->flags &= ~8;
        p->icon->flags |= 0x400;
    }
}

/* Drop the indicator's icon, unlink the record from whichever list holds
 * it, free it. */
// FUNCTION: LEGOLAND 0x0046fe00
void DeleteIndicator(Indicator* p)
{
    Indicator* q;

    RemoveIcon(p->icon);
    if (p->flags & 8) {
        q = g_ind_active;
        if (q == p) {
            g_ind_active = q->next;
        } else {
            while (q) {
                if (q->next == p) {
                    q->next = p->next;
                    break;
                }
                q = q->next;
            }
        }
    } else {
        q = g_ind_inactive;
        if (q == p) {
            g_ind_inactive = q->next;
        } else {
            while (q) {
                if (q->next == p) {
                    q->next = p->next;
                    break;
                }
                q = q->next;
            }
        }
    }
    HeapFree_w(p);
}
