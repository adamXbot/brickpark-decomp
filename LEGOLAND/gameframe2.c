/* LEGOLAND — scope LL8: gameframe / icon-UI leftovers (inventory 12+13+14).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only
 * struct field offsets, callee argument counts and global addresses are
 * load-bearing; names are ours. Types are defined locally (legoland.h is
 * owned elsewhere). Extern prototype TYPES are caller-side codegen levers.
 *
 * 0x004689f0 is the script-string intern, not sysstubs.c's NewScriptEvent
 * at 0x00468910. movie3.c / levelkw.c declare this address under that
 * colliding name; the definition here is AddScriptString.
 *
 * Verification and recovered mechanics: docs/lanes/scope-ll8.md.
 */

#pragma intrinsic(strlen, strcmp)

/* ---- CRT ---------------------------------------------------------------- */
extern void* MemAlloc(unsigned int size);                    /* 0x0049e4ff */
extern int   sprintf(char* buf, const char* fmt, ...);       /* 0x0049e573 */
int          strcmp(const char*, const char*);
unsigned int strlen(const char*);

/* ---- types -------------------------------------------------------------- */
typedef struct Sprite Sprite;

typedef struct Pos { int x, y; } Pos;

typedef struct HelpRect {
    int x0;
    int y0;
    int x1;
    int y1;
} HelpRect;

typedef struct Controller {
    int x0, y0;
    int x, y;
} Controller;

typedef struct Bloke {
    struct Bloke*  next;
    void*          model;
    unsigned char  pad08[6];
    unsigned short state;        /* +0x0e */
} Bloke;

typedef struct MapObj {
    void* f0;                    /* +0x00  first dword ShowInstHelp takes */
} MapObj;

typedef struct ObjDef {
    char    pad00[0x78];
    char*   name;                /* +0x78 */
    char    pad7c[0xc4 - 0x7c];
    MapObj* inst;                /* +0xc4 */
} ObjDef;

/* Icon record: page-toggle icons stash two sprites at +0x1c/+0x20 and a
 * byte page id at +0x18. next at +0x00 so Icon* doubles as Icon** to next. */
typedef struct Icon {
    struct Icon*   next;         /* +0x00 */
    Sprite*        sprite;       /* +0x04 */
    ObjDef*        data;         /* +0x08 */
    short          x;            /* +0x0c */
    short          y;            /* +0x0e */
    short          w;            /* +0x10 */
    short          h;            /* +0x12 */
    unsigned short group;        /* +0x14 */
    short          pad16;
    unsigned char  page;         /* +0x18 */
    unsigned char  pad19[3];
    Sprite*        spr_on;       /* +0x1c */
    Sprite*        spr_off;      /* +0x20 */
    char           pad24[0x34 - 0x24];
    unsigned int   flags;        /* +0x34 */
    char*          help;         /* +0x38 */
    int            help_id;      /* +0x3c */
} Icon;

typedef struct WorkOrder WorkOrder;
typedef struct RepairOrder RepairOrder;

/* ---- globals ------------------------------------------------------------ */
extern Icon*         g_side_icons;           /* 0x006687c8 */
extern Icon*         g_side_icons2;          /* 0x006687cc */
extern Icon*         g_focussed_icon;        /* 0x006687d0 */
extern int           g_drag_lock;            /* 0x00668954 */
extern int           g_last_hint;            /* 0x00668614 */
extern int           g_script_string_count;  /* 0x00668720 */
extern char*         g_script_strings[];     /* 0x007fe120 */
extern int           g_advisor_last;         /* 0x007fe050 */
extern int           g_ingame_help_tick;     /* 0x007fe054 */
extern int           g_help_target;          /* 0x004b9f8c */
extern int           g_help_changed;         /* 0x004b9f88 */
extern unsigned long g_help_hover_start;     /* 0x007fe920 */
extern int           g_help_face_state;      /* 0x006687a4 */
extern int           g_help_requested;       /* 0x006687a8 */
extern int           g_game_mode;            /* 0x008119b4 */
extern int           g_edit_mode;            /* 0x008119b0 */
extern int           g_screen_mode;          /* 0x0080ff88 */
extern int           g_state_810140;         /* 0x00810140 */
extern ObjDef*       g_edit_object;          /* 0x008119b8 */
extern Controller*   g_controller;           /* 0x00813b00 */
extern Pos           g_gfx_point;            /* 0x00813a44 */
extern WorkOrder*    g_gardener_orders;      /* 0x0079a8b0 */
extern WorkOrder*    g_mechanic_orders;      /* 0x0079a8c0 */
extern RepairOrder*  g_repair_orders;        /* 0x0079a8d4 */
extern Bloke*        g_gardener_list;        /* 0x0079a8a8 */
extern Bloke*        g_mechanic_list;        /* 0x0079a8ac */

extern const char g_fmt_s[];                 /* 0x004b8bbc "%s" */
extern const char g_fmt_scs[];               /* 0x004b9f90 "%s%c%s" */
extern const char g_str_path[];              /* 0x004ba888 "Path" */

/* ---- callees ------------------------------------------------------------ */
extern void  ResetScriptTimer(void);                                 /* 0x00468d00 */
extern void  FreeScriptStrings(void);                                /* 0x004689a0 */
extern void  KillHelpText(void);                                     /* 0x0046c5c0 */
extern void  KillAdvisorHelp(void);                                  /* 0x0046ce20 */
extern void  UnloadSaveGameMap(void);                                /* 0x0047f760 */
extern int   ResetLevelObjects(void);                                /* 0x004629e0 */
extern int   IsNarrationPlaying(void);                               /* 0x00498cf0 */
extern __declspec(dllimport) unsigned long __stdcall GetTickCount(void); /* IAT 0x004ab1f8 */
extern void  HTBubbleHelp(HelpRect* r, char* text, int font);        /* 0x004557c0 */
extern void  ShowIdHelp(int id);                                     /* 0x0046d230 */
extern void  UnlinkIcon(Icon** link);                                /* 0x0046d460 */
extern void  UnlinkIcon2(Icon** link);                               /* 0x0046d4a0 */
extern Icon* FindIcon(int group);                                    /* 0x0046d630 */
extern void  SetIconSprite(Icon* p, Sprite* s);                      /* 0x0046d680 */
extern char  ScrollUpInput(Icon* p, int ev, short dx, short dy);     /* 0x0046d980 */
extern char  ScrollDownInput(Icon* p, int ev, short dx, short dy);   /* 0x0046da20 */
extern void  FreeGardenerOrder(WorkOrder* o);                        /* 0x00499e30 */
extern void  FreeMechanicOrder(WorkOrder* o);                        /* 0x00499eb0 */
extern void  FreeRepairOrder(RepairOrder* r);                        /* 0x0049b6e0 */
extern void  RemoveAGardener(Bloke* g);                              /* 0x0049a2d0 */
extern void  RemoveAMechanic(Bloke* m);                              /* 0x0049a430 */

/* =========================================================================
 *  High-level AI table slots 0x18 / 0x19: set low-level state to idle (14).
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0049a4a0
void Bloke_GoIdle(Bloke* b)
{
    b->state = 14;
}

// FUNCTION: LEGOLAND 0x0049a4d0
void Bloke_GoIdle2(Bloke* b)
{
    b->state = 14;
}

/* Reset the script clock and the two in-game help stamps. */
// FUNCTION: LEGOLAND 0x0046ce00
void ResetInGameHelp(void)
{
    ResetScriptTimer();
    g_advisor_last = 0;
    g_ingame_help_tick = 0;
}

/* Tear down script strings, help and the loaded map before a park load. */
// FUNCTION: LEGOLAND 0x0046cb20
int UnloadParkHelp(void)
{
    int from_save = g_state_810140;

    g_last_hint = 0;
    if (from_save)
        UnloadSaveGameMap();
    else
        ResetLevelObjects();
    FreeScriptStrings();
    KillHelpText();
    KillAdvisorHelp();
    ResetInGameHelp();
    return 1;
}

/* ShowObjectHelp's face-state-1 sibling (instance / named-object help). */
// FUNCTION: LEGOLAND 0x0046d2f0
void ShowInstHelp(void* obj)
{
    if (IsNarrationPlaying())
        goto requested;
    if (!obj)
        return;
    if ((int)obj != g_help_target) {
        g_help_target = (int)obj;
        g_help_hover_start = GetTickCount();
        g_help_face_state = 1;
        g_help_changed = 1;
    }
requested:
    g_help_requested = 1;
}

/* Front-end column from the cursor, else the in-game object-list group. */
static __inline int IconBarWheelGroup(void)
{
    int x;
    int group;

    if (g_game_mode == 2 && g_screen_mode == 3) {
        x = g_controller->x;
        if (x < 0xb2)
            group = 0xc8;
        else if (x < 0x143)
            group = 0x1f4;
        else
            group = (x < 0x1d4) ? 0x190 : 0x12c;
    } else {
        group = 0xd2;
    }
    return group;
}

// FUNCTION: LEGOLAND 0x0046dac0
void IconBarWheelDown(void)
{
    Icon* p = FindIcon(IconBarWheelGroup() + 3);

    if (p)
        ScrollUpInput(p, 1, 0, 0);
}

// FUNCTION: LEGOLAND 0x0046db40
void IconBarWheelUp(void)
{
    Icon* p = FindIcon(IconBarWheelGroup() + 4);

    if (p)
        ScrollDownInput(p, 1, 0, 0);
}

/* In-game twin of ProcessFrontEndHelp: bubble at the mouse, then instance
 * help (flag 0x1000, via the class at +0x08 -> inst +0xc4) or string-id. */
// FUNCTION: LEGOLAND 0x0046cff0
void ProcessInGameIconHelp(void)
{
    Icon*    p = g_focussed_icon;
    HelpRect r;

    if (g_focussed_icon && (p->flags & 0x2000) && !g_drag_lock) {
        r.x0 = g_gfx_point.x;
        r.y0 = g_gfx_point.y - 10;
        r.x1 = g_gfx_point.x;
        r.y1 = g_gfx_point.y;
        HTBubbleHelp(&r, p->help, 2);
        p = g_focussed_icon;
        if (p->flags & 0x1000) {
            ShowInstHelp(p->data->inst->f0);
            return;
        }
        ShowIdHelp(p->help_id);
    }
}

/* Drain the five worker / order lists. BeginParkLoad's "free the lists". */
// FUNCTION: LEGOLAND 0x0049cfc0
void FreeWorkerLists(void)
{
    while (g_gardener_orders)
        FreeGardenerOrder(g_gardener_orders);
    while (g_mechanic_orders)
        FreeMechanicOrder(g_mechanic_orders);
    while (g_repair_orders)
        FreeRepairOrder(g_repair_orders);
    while (g_gardener_list)
        RemoveAGardener(g_gardener_list);
    while (g_mechanic_list)
        RemoveAMechanic(g_mechanic_list);
}

/* Unlink every icon whose group is in [group, group+7) on both lists.
 * The first list keeps a dead unlink count (ebp); the second does not. */
// FUNCTION: LEGOLAND 0x0046d590
void RemoveIconGroupRange(int group)
{
    Icon** link;
    Icon*  p;
    int    n = 0;

    link = &g_side_icons;
    for (p = g_side_icons; p; ) {
        if (p->group >= (unsigned short)group &&
            (int)(unsigned short)p->group < (int)((unsigned short)group + 7)) {
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
        if (p->group >= (unsigned short)group &&
            (int)(unsigned short)p->group < (int)((unsigned short)group + 7)) {
            p = p->next;
            UnlinkIcon2(link);
        } else {
            link = &p->next;
            p = p->next;
        }
    }
    while (n--)
        ;
}

/* Pick the side-bar page from the mode / edit class and swap group 0x93. */
// FUNCTION: LEGOLAND 0x0046ee00
void UpdateIconPage(void)
{
    Icon*   p;
    int     page;
    ObjDef* obj;

    p = g_side_icons;
    if (g_game_mode == 1)
        page = 4;
    else {
        switch ((unsigned int)g_edit_mode) {
        case 1:
            obj = g_edit_object;
            if (obj && strcmp(g_str_path, obj->name) == 0)
                page = 1;
            else
                page = 5;
            break;
        case 0:
            page = 2;
            break;
        case 2:
            page = 3;
            break;
        case 3:
            page = 5;
            break;
        case 4:
            page = 4;
            break;
        default:
            page = 5;
            break;
        }
    }
    for (; p; p = p->next) {
        if (p->group == 0x93 && p->spr_on && p->spr_off) {
            if ((unsigned int)p->page == (unsigned int)page)
                SetIconSprite(p, p->spr_on);
            else
                SetIconSprite(p, p->spr_off);
        }
    }
}

/* Intern a script / hint string and return its table index.
 * copy==0 stores `a` as-is; copy && !a stores NULL; copy && a && !b
 * strdup's a; both strings are joined with '@' (0x40) via "%s%c%s".
 * !a shares the malloc-fail bump via goto so the duplicated increment
 * is eax-primary (`mov ecx,eax / pop esi / inc ecx`). !copy stores
 * through a slot pointer so the index lands in EDX. */
// FUNCTION: LEGOLAND 0x004689f0
int AddScriptString(char* a, char* b, int copy)
{
    char*  p;
    char** slot;
    int    n;

    if (copy) {
        if (a) {
            if (b) {
                p = (char*)MemAlloc(strlen(a) + strlen(b) + 2);
                g_script_strings[g_script_string_count] = p;
                if (!p)
                    goto bump;
                sprintf(p, g_fmt_scs, a, 0x40, b);
            } else {
                p = (char*)MemAlloc(strlen(a) + 1);
                g_script_strings[g_script_string_count] = p;
                if (!p)
                    goto bump;
                sprintf(p, g_fmt_s, a);
            }
            n = g_script_string_count;
            g_script_string_count++;
            return n;
        }
        g_script_strings[g_script_string_count] = 0;
        goto bump;
    }
    slot = &g_script_strings[g_script_string_count];
    *slot = a;
bump:
    n = g_script_string_count;
    g_script_string_count++;
    return n;
}

/* Find the unlocked icon of `group` that would land closest to `edge`
 * once `delta` is applied without passing `limit`, and return the delta
 * that puts it exactly there. Y-axis only (icon +0x0e). */
// FUNCTION: LEGOLAND 0x0046dd10
int SnapIconScroll(int mask, short limit, short edge, short group, int delta)
{
    Icon* best;
    Icon* p;
    int   best_y;
    int   y;
    int   shifted;

    p = g_side_icons;
    best = 0;
    if (delta > 0) {
        best_y = -100000;
        if (!p)
            goto fail;
        for (; p; p = p->next) {
            if (p->flags & 1)
                continue;
            if ((short)(p->group & mask) != group)
                continue;
            y = p->y;
            shifted = y - edge + delta;
            if (shifted > limit)
                continue;
            if (y <= best_y)
                continue;
            best = p;
            best_y = y;
        }
    } else {
        if (delta >= 0)
            goto fail;
        best_y = 100000;
        if (!p)
            goto fail;
        for (; p; p = p->next) {
            if (p->flags & 1)
                continue;
            if ((short)(p->group & mask) != group)
                continue;
            y = p->y;
            shifted = y - edge + delta;
            if (shifted <= limit)
                continue;
            if (y >= best_y)
                continue;
            best = p;
            best_y = y;
        }
    }
    if (best) {
        if (best->y == limit)
            return 0;
        return (int)limit - (int)best->y + (int)edge;
    }
fail:
    return 0;
}
