/* LEGOLAND -- the free-play side panel's remaining pieces: the list teardown,
 * the alarm-icon overlay, the build-icon click handler, the worker pop-up, the
 * coloured single-line text printer and the panel scroller itself.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Only
 * struct field offsets, callee argument counts and global addresses are
 * load-bearing; the names are ours.  Types are declared locally, reusing the
 * shapes fpui.c / fpui2.c / fpui3.c / iconui.c / text.c already recovered.
 *
 * ---------------------------------------------------------------------------
 * WHAT THESE ARE
 * ---------------------------------------------------------------------------
 *   0x0048b4a0  RemoveFreePlayList    tear one free-play panel list down
 *   0x004760a0  RenderIconsExtra      the icon pass plus the alarm blinkers
 *   0x00470000  BuildObjectIconInput  click on a class icon in the build panel
 *   0x00481170  ClearObjInfoList      drop the info-node list and the map's
 *                                     "info shown" cell flag
 *   0x00470100  WorkerPopUp           open the gardener / mechanic pop-up
 *   0x00454d80  NewPrintColoured      DrawText one line into a by-value box
 *   0x0046d850  ScrollIconPanel       scroll a panel's icon group, clamped
 *
 * Six of the seven are exact; ScrollIconPanel is 121/121 instructions at
 * 304/304 bytes with one four-value allocation rank inverted (its note has
 * the full measurement).
 *
 * ---------------------------------------------------------------------------
 * THREE CODEGEN LEVERS RECOVERED HERE
 * ---------------------------------------------------------------------------
 * 1. THE PUSH-SINKING RULE NEEDS **ONE** `return K` IN THE GUARDED BLOCK.
 *    The recorded lever says a push sinks past a leading guard when the
 *    guarded block ends in its own `return K`; BuildObjectIconInput sharpens
 *    it.  With `return 2;` written in BOTH arms of the inner if, VC6 puts
 *    `push esi` back in the real prologue and the guard then has to read its
 *    argument before esp moves.  Written as an if/else with a SINGLE
 *    trailing `return 2;`, VC6 tail-duplicates the epilogue into both arms
 *    itself and both pushes sink -- one instruction, one byte, and exact.
 *
 * 2. WALK A LIST THROUGH ITS HEAD GLOBAL TO GET THE ACCUMULATOR-FORM STORE.
 *    ClearObjInfoList re-stores its head every iteration.  With a local
 *    cursor the stored value lives in ESI and the store is the six-byte
 *    `mov [imm32],esi`; the original has the five-byte `mov [imm32],eax`.
 *    About twenty loop spellings are all floored at 1 of 57 / 143 bytes
 *    against 142.  Writing the loop on the GLOBAL (`while (g_head != 0) {
 *    next = g_head->next; free(g_head); g_head = next; }`) makes VC6 forward
 *    the just-stored head into the loop's own register -- which is eax -- and
 *    the short accumulator encoding falls out.  Exact.
 *
 * 3. A NAMED SUM IS WHAT STOPS VC6 DOING THE ALGEBRA.
 *    `step += limit - (base + step);` written inline is folded to
 *    `step = limit - base;`, which also merges two clamp blocks into one and
 *    loses four instructions.  Assigning the sum to a local first
 *    (`t = base + step; if (t < limit) step += limit - t;`) is not the same
 *    object and the original's unsimplified `sub/add` pair comes back.  Same
 *    family as the recorded "an expression spelled twice is not a named
 *    local", from the other direction: here the temporary is what BLOCKS an
 *    optimisation rather than what enables one.
 *
 * ---------------------------------------------------------------------------
 * HOW THE SIDE PANEL SCROLLS  (recovered here, from ScrollIconPanel)
 * ---------------------------------------------------------------------------
 * A scrolling panel (fpui2.c's ObjListPanel) carries two rectangles: the
 * extent of the ICONS currently on it (+0x0c..+0x18) and the extent of the
 * BOX they live in (+0x1c..+0x28).  `axis` bit 0 picks which pair of edges a
 * scroll works on -- the vertical panels use top/bottom, the horizontal ones
 * left/right.  Three outcomes:
 *
 *   1. the icons FIT (their span is no wider than the box's): there is
 *      nothing to scroll, so the group is snapped flush to the box's origin
 *      and the function returns;
 *   2. otherwise the requested step is first handed to SnapIconScroll, which
 *      finds the icon that would land nearest the box's leading edge and
 *      returns the step that puts it exactly there -- so a scroll always
 *      comes to rest on an icon boundary rather than mid-row;
 *   3. that snapped step is then CLAMPED so the icons cannot be dragged past
 *      either end, the panel's four cached edges are advanced by it, the
 *      current menu's saved scroll position is advanced too, and MoveIcons
 *      shifts every icon of the group.
 *
 * Only the scrolling axis is snapped and clamped; the other axis's step is
 * carried through unchanged, which is why a diagonal call would move the
 * cross axis without any limit checking at all.
 *
 * ---------------------------------------------------------------------------
 * THE INFO-NODE FLAG SWEEP  (ClearObjInfoList)
 * ---------------------------------------------------------------------------
 * The "show me what this is" info list keeps one node per queried object AND
 * a per-cell flag (Cell +0x0c bit 0x400) so the render walk can label the
 * map.  Clearing the list therefore has to clear the flag on EVERY cell, and
 * the original does it with the bounds-checked cell accessor inlined -- which
 * returns 0 off-map and is then written through unchecked.  In practice the
 * loop never leaves the map, so the null store is unreachable; it is
 * reproduced as written.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

#pragma intrinsic(strlen)
extern unsigned int strlen(const char* s);

/* ---- the icon record (0x40 bytes, iconui.c / fpui.c / fpui3.c) ---------- */
typedef struct Icon {
    struct Icon*     next;      /* +0x00 */
    Sprite*          sprite;    /* +0x04 */
    void*            data;      /* +0x08  the class ObjDef for a build icon */
    short            x;         /* +0x0c */
    short            y;         /* +0x0e */
    short            w;         /* +0x10 */
    short            h;         /* +0x12 */
    unsigned short   group;     /* +0x14 */
    short            f16;       /* +0x16 */
    union {
        char         kind;      /* +0x18 */
        void*        owner;     /* +0x18 */
    } u18;
    union {
        char*        text;      /* +0x1c */
        int          value;     /* +0x1c */
    } u1c;
    int              f20;       /* +0x20 */
    void*            f24;       /* +0x24 */
    int            (*render)(struct Icon*);                      /* +0x28 */
    char           (*input)(struct Icon*, int ev, short dx, short dy); /* +0x2c */
    void*            widget;    /* +0x30 */
    unsigned int     flags;     /* +0x34 */
    char*            help;      /* +0x38 */
    int              help_id;   /* +0x3c */
} Icon;

/* The 0xd0-byte ObjDef; only what a build icon touches. */
typedef struct ObjDef {
    char    pad00[0xc4];
    LLElem* elem;               /* +0xc4 this class's own LLIDB element */
} ObjDef;

/* ---- shared callees ----------------------------------------------------- */
extern void  HeapFree_w(void* p);                                 /* 0x0049e4d0 */
extern void  PlayInstanceOfSample(void* sample, int a, int b, int c); /* 0x00496d20 */


/* =========================================================================
 * 0x0048b4a0 -- tear one free-play panel list down.
 *
 * The four panels are keyed by their icon group base (200/300/400/500) and
 * each owns a list of FPItem records with two heap strings hanging off it
 * (the class name at +0x04 and the caption at +0x10).  The icons themselves
 * live in group `id + 6` and go first.
 *
 * ORIGINAL BUG, reproduced: the free loop is entered only when the head node
 * has a SUCCESSOR, so a list of exactly one item has its head pointer cleared
 * without the node or its two strings ever being freed.  The guard is a
 * genuine second test -- the loop's own latch is the `p != 0` re-test at the
 * bottom, and a plain `while (p)` walk would not emit it.
 * ========================================================================= */

/* One item of a free-play panel list (fpui2.c's FPItem). */
typedef struct FPItem {
    struct FPItem* next;        /* +0x00 */
    char*          name;        /* +0x04 heap copy of the class's LLIDB name */
    LLElem*        elem;        /* +0x08 */
    LLElem*        parent;      /* +0x0c */
    char*          text;        /* +0x10 heap copy of the caller's text */
    Sprite*        sprite;      /* +0x14 */
    int            key;         /* +0x18 */
} FPItem;                       /* 0x1c */

extern FPItem* g_fp_list200;   /* 0x007cb3d0 */
extern FPItem* g_fp_list300;   /* 0x007cb39c */
extern FPItem* g_fp_list400;   /* 0x007cb3b8 */
extern FPItem* g_fp_list500;   /* 0x007cb3a4 */

extern void RemoveIconGroup(int group);                           /* 0x0046d520 */

// FUNCTION: LEGOLAND 0x0048b4a0
void RemoveFreePlayList(int id)
{
    FPItem** head;
    FPItem*  p;
    FPItem*  next;

    RemoveIconGroup(id + 6);
    if (id == 200) {
        p = g_fp_list200;
        head = &g_fp_list200;
    } else if (id == 300) {
        p = g_fp_list300;
        head = &g_fp_list300;
    } else if (id == 400) {
        p = g_fp_list400;
        head = &g_fp_list400;
    } else if (id == 500) {
        p = g_fp_list500;
        head = &g_fp_list500;
    } else {
        return;
    }
    if (p != 0) {
        if (p->next != 0) {
            do {
                next = p->next;
                HeapFree_w(p->name);
                HeapFree_w(p->text);
                HeapFree_w(p);
                p = next;
            } while (p != 0);
        }
        *head = 0;
    }
}


/* =========================================================================
 * 0x004760a0 -- the icon pass's tail: the flashing interface buttons.
 *
 * fpui.c's RenderIcons calls this as its last act.  Nine of the interface's
 * buttons can be made to FLASH (the tutorial uses it to point at the control
 * the player is meant to click): the four theme tabs on the top row and the
 * five tool buttons below them.  g_btnflash[i] non-zero means "flash button
 * i", and the button is then redrawn every frame -- pressed while the global
 * blink phase is up, normal while it is down -- over whatever the icon pass
 * already put there.
 *
 * The positions are a fixed table, NOT the icons' own coordinates, so the
 * overlay lands on the button whether or not that button currently has an
 * icon; and the whole pass only runs in game mode 3 (a running park).
 *
 * A one-shot sample marks the moment a flash STARTS: `any` is raised by the
 * pressed arm only, so the sound is triggered on a frame where the blink is
 * up while the previous frame had nothing flashing.
 *
 * The two PrintSprite calls share their last four arguments -- VC6
 * cross-jumps the argument block and only the sprite push differs -- which is
 * what two textual calls differing in one argument look like.
 * ========================================================================= */

typedef struct IfIconPos { int x; int y; } IfIconPos;

/* 0x004bb04c: four theme tabs on row 379, five tool buttons on row 418. */
extern const IfIconPos kIfIconPos[9];   /* 0x004bb04c */
/* The interface button sprites, pressed and normal (fpui.c names the first
 * four of each pair g_theme_*_on / g_theme_*_off and the rest g_ci_*). */
extern Sprite* g_if_icon_pressed[9];    /* 0x007fdcc0 */
extern Sprite* g_if_icon_normal[9];     /* 0x007fdd40 */
/* savegame.c saves this as 0x24 raw bytes; it is read here as nine ints. */
extern int     g_btnflash[9];           /* 0x007fdd00 */
extern int     g_btnflash_sounding;     /* 0x00668ec0 */
extern void*   g_snd_btnflash;          /* 0x004b9338 */
extern int     g_game_mode;             /* 0x008119b4  3 = a running park */

/* 0x00476020 is a bare `ret` in the shipped build -- a hook the icon pass
 * still calls.  Reproduced as the call it is. */
extern void RenderIconsHook(void);                                /* 0x00476020 */
extern int  GetBlink(void);                                       /* 0x00499480 */
extern int  PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */

// FUNCTION: LEGOLAND 0x004760a0
void RenderIconsExtra(void)
{
    int any = 0;
    int i;

    RenderIconsHook();
    if (g_game_mode == 3) {
        for (i = 0; i < 9; i++) {
            if (g_btnflash[i] != 0) {
                if (GetBlink() != 0) {
                    PrintSprite(g_if_icon_pressed[i], kIfIconPos[i].x,
                                kIfIconPos[i].y, 0, 0);
                    any = 1;
                } else {
                    PrintSprite(g_if_icon_normal[i], kIfIconPos[i].x,
                                kIfIconPos[i].y, 0, 0);
                }
            }
        }
        if (any != 0 && g_btnflash_sounding == 0)
            PlayInstanceOfSample(g_snd_btnflash, 0, 1, 0);
        g_btnflash_sounding = any;
    }
}


/* =========================================================================
 * 0x00470000 -- a class icon in the build panel was clicked.
 *
 * Event bit 1 is "released"; anything else is not ours (answer 1).  On a
 * release the class's price is checked against the brick count FIRST, and a
 * class the player cannot afford only buzzes.  Otherwise:
 *
 *   - icon flag 0x1000 means "this icon selects the class for building", so
 *     the class becomes the edit object;
 *   - the class's LLIDB element carries a "NEW" bit (type_flags 0x20000)
 *     that makes the class appear in the new-objects strip; clicking it
 *     clears the bit and drops the strip's entry;
 *   - either way the click sample plays and the answer is 2 (handled).
 *
 * LAYOUT, and it sharpens the recorded push-sinking rule: the guarded block
 * must end in ONE `return K`, not two.  Written with a `return 2;` in each
 * arm the body is otherwise identical but VC6 emits `push esi` in the real
 * prologue, and the guard then has to read its argument before esp moves
 * (`mov al,[esp+8] / push esi / test al,2` instead of the original's
 * `test byte ptr [esp+8],2 / je / push esi`) -- one instruction, one byte,
 * and the epilogue's `pop edi` migrates up to the compare.  As an if/else
 * with a single trailing `return 2;` VC6 tail-duplicates the epilogue into
 * both arms by itself and both pushes sink past the guard: 55/55 exact.
 * ========================================================================= */

extern int  GetObjCost(void* def);                                /* 0x00480da0 */
extern int  GetBrickCount(void);                                  /* 0x004578e0 */
extern void SetEditObject(void* def);                             /* 0x004816e0 */
/* Drop this class's entry from the "new objects" preview strip. */
extern void RemoveNewObjectMarker(void* def);                     /* 0x00471ca0 */

extern void* g_snd_click;        /* 0x004b929c  the UI click sample */
extern void* g_snd_denied;       /* 0x004b92d8  "you cannot afford that" */

// FUNCTION: LEGOLAND 0x00470000
char BuildObjectIconInput(Icon* p, int ev, short dx, short dy)
{
    LLElem* elem;

    if (ev & 2) {
        if (GetObjCost(p->data) <= GetBrickCount()) {
            if (p->flags & 0x1000)
                SetEditObject(p->data);
            elem = ((ObjDef*)p->data)->elem;
            if (elem->type_flags & 0x20000) {
                elem->type_flags &= ~0x20000;
                RemoveNewObjectMarker(p->data);
            }
            PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        } else {
            PlayInstanceOfSample(g_snd_denied, 0, 1, 0);
        }
        return 2;
    }
    return 1;
}


/* =========================================================================
 * 0x00481170 -- drop the object-info list and un-flag every map cell.
 *
 * The "what is this?" query keeps one InfoNode per queried object AND a
 * per-cell bit (Cell +0x0c, 0x400) so the render walk knows which cells are
 * already labelled.  Both have to go, and the cell sweep is the whole map.
 *
 * Two details are the original's and are reproduced.  First, THE GLOBAL IS
 * THE WALKER: the free loop's condition and its every use read
 * `g_info_head` itself rather than a local cursor, so the head is always
 * consistent for anything the free might reach.  That spelling is also the
 * whole difference between exact and not -- see the codegen note below.
 * Second, the cell fetch is the bounds-checked accessor INLINED, which
 * returns 0 off-map and is then written through with no null test.  The loop
 * bounds are the map's own extent, so the null store is unreachable in
 * practice.
 *
 * CODEGEN NOTE (new lever): a local cursor with `g_info_head = p;` as a
 * trailing statement puts the stored value in ESI and costs the one-byte
 * `mov [imm32],esi` where the original has the 5-byte `mov [imm32],eax` --
 * 1 of 57 at 143 bytes against 142, and about twenty loop spellings (both
 * store orders, `for` increments, chained assignment, a head POINTER, dead
 * and volatile kills of the successor local, do/while, single-variable
 * walks) are all floored there.  Walking the GLOBAL is what puts the value
 * in eax: VC6 forwards the just-stored head into the loop's own register,
 * which is eax, and the short accumulator-form store falls out.
 *
 * The bounds check's `x >= 0` test is done on VC6's own strength-reduced
 * x * 0x14 offset (`test edx,edx / jl`), and `g_map` is re-read on every
 * inner iteration because the store through a possibly-null Cell* may alias
 * it -- both automatic, neither is a source construct.
 * ========================================================================= */

/* One info node (fpui2.c's InfoNode; objmap.c's KeyNode). */
typedef struct InfoNode {
    struct InfoNode* next;      /* +0x00 */
    void*            cls;       /* +0x04 */
    unsigned short   pos;       /* +0x08 packed origin */
    char             pad0a[2];
    int              x;         /* +0x0c */
    int              y;         /* +0x10 */
    int              f14;       /* +0x14 */
    unsigned char    ex;        /* +0x18 */
    unsigned char    ey;        /* +0x19 */
    char             pad1a[2];
} InfoNode;                     /* 0x1c */

extern InfoNode* g_info_head;                                     /* 0x00669248 */

static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

// FUNCTION: LEGOLAND 0x00481170
void ClearObjInfoList(void)
{
    InfoNode* next;
    int       x;
    int       y;

    while (g_info_head != 0) {
        next = g_info_head->next;
        HeapFree_w(g_info_head);
        g_info_head = next;
    }
    for (y = 0; y < g_map->height; y++)
        for (x = 0; x < g_map->width; x++)
            MapCellAt(x, y)->flags &= ~0x400;
}


/* =========================================================================
 * 0x00470100 -- pick a worker up: open the gardener / mechanic pop-up.
 *
 * fpui2.c's PopUpInfoSetUp routes worker types 0x307 (gardener) and 0x308
 * (mechanic) here.  The worker is published in the module's own "worker being
 * carried" globals (0x007fdff0..0x007fdffc, a block just past PopUpInfo), its
 * low-level AI state is forced to 13 ("held by the player"), its heading to 5,
 * and the drag lock is raised so the rest of the UI knows something is in the
 * player's hand.
 *
 * Then the SAME three-call sequence runs per kind -- clear the work list,
 * start long-term action 0x18 (gardener) or 0x19 (mechanic), clear the work
 * list AGAIN -- the second clear being there because starting the action
 * queues one.  The work order is then cleared twice, once through the
 * parameter and once through the global (the same object), and the long-term
 * action is started a SECOND time.  Both duplications are the original's and
 * are reproduced.
 *
 * Every use after the publish reads the GLOBAL, not the parameter, which is
 * what gives the single `mov eax,[0x7fdff0]` feeding the three field accesses
 * and a fresh load at every call site; and the two arms' first call shares one
 * `push` because VC6 cross-jumps the identical argument setup.
 * ========================================================================= */

/* A worker (workers2.c's Bloke); only the fields the pop-up touches. */
typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;       /* +0x0e  low-level AI state; 13 = held */
    unsigned char  pad10[0x50 - 0x10];
    void*          order;       /* +0x50  current work order */
    unsigned char  pad54[0x68 - 0x54];
    Pos            world;       /* +0x68  world position, 24.8 */
    unsigned char  pad70[2];
    unsigned char  dir;         /* +0x72  current heading */
} Bloke;

extern Bloke* g_wp_obj;         /* 0x007fdff0 the worker in the player's hand */
extern int    g_wp_x;           /* 0x007fdff4 where it was picked up */
extern int    g_wp_y;           /* 0x007fdff8 */
extern int    g_wp_kind;        /* 0x007fdffc 0x307 gardener, else mechanic */
extern int    g_drag_lock;      /* 0x00668954 */

extern void DBPrintf(const char* fmt, ...);                       /* 0x00453a20 */
extern void ClearAGardenersWorkList(Bloke* w);                    /* 0x0049b550 */
extern void ClearAMechanicsWorkList(Bloke* w);                    /* 0x0049b510 */
extern void NewLongTermAction(Bloke* w, int action);              /* 0x0044e760 */

// FUNCTION: LEGOLAND 0x00470100
void WorkerPopUp(int kind, Bloke* w)
{
    DBPrintf("Picking up worker (%x) Workorder = %x\n", w, w->order);
    g_wp_obj = w;
    g_wp_kind = kind;
    w->state = 13;
    g_wp_x = g_wp_obj->world.x;
    g_wp_y = g_wp_obj->world.y;
    g_wp_obj->dir = 5;
    g_drag_lock = 1;
    if (g_wp_kind == 0x307) {
        ClearAGardenersWorkList(g_wp_obj);
        NewLongTermAction(g_wp_obj, 0x18);
        ClearAGardenersWorkList(g_wp_obj);
    } else {
        ClearAMechanicsWorkList(g_wp_obj);
        NewLongTermAction(g_wp_obj, 0x19);
        ClearAMechanicsWorkList(g_wp_obj);
    }
    w->order = 0;
    g_wp_obj->order = 0;
    if (g_wp_kind == 0x307)
        NewLongTermAction(g_wp_obj, 0x18);
    else
        NewLongTermAction(g_wp_obj, 0x19);
}


/* =========================================================================
 * 0x00454d80 -- DrawText one single line into a caller-supplied box, in a
 * caller-supplied colour.  text.c's unmatched neighbour: the same GDI dance
 * as PrintLimitedText, but the box arrives BY VALUE (four argument slots)
 * instead of being built from x / y / w, and the format is the constant 0x20
 * (DT_SINGLELINE).
 *
 * mapscreen3.c's PrintScreenMode6 is the only caller and calls it THREE
 * times, once per colour: the current level in 0, a finished level in
 * 0x323232 and a locked one in 0xa0a0a0.  Those really are three textual
 * calls -- the original emits two complete copies of the by-value rect's
 * argument block, one built through edx and one through ecx, with only the
 * font / text / call tail merged, which a single call taking a colour
 * variable could never produce.
 *
 * The saved font handle is homed in the (now dead) `colour` argument slot,
 * and the three SelectObject calls share ONE hoisted IAT load in ebx -- both
 * VC6's own doing, neither needs a source construct.
 * ========================================================================= */

/* Win32 RECT; the game's clip window and the DrawText boxes use it. */
typedef struct WinRect {
    long left;                  /* +0x00 */
    long top;                   /* +0x04 */
    long right;                 /* +0x08 */
    long bottom;                /* +0x0c */
} WinRect;

typedef struct DDSurface DDSurface;

/* IDirectDrawSurface's vtable; only the two DC slots are named (text.c). */
typedef struct DDSurfaceVtbl {
    char pad00[0x44];                                       /* +0x00 */
    long(__stdcall* GetDC)(DDSurface*, void** hdc);         /* +0x44 */
    char pad48[0x68 - 0x48];                                /* +0x48 */
    long(__stdcall* ReleaseDC)(DDSurface*, void* hdc);      /* +0x68 */
} DDSurfaceVtbl;

struct DDSurface {
    DDSurfaceVtbl* vtbl;        /* +0x00 */
};

extern DDSurface* g_draw_surface;         /* 0x0066807c */
extern WinRect    g_clip_rect;            /* 0x004bdea0 */

__declspec(dllimport) void*         __stdcall CreateRectRgn(int l, int t, int r, int b); /* [0x4ab0b8] */
__declspec(dllimport) void*         __stdcall SelectObject(void* dc, void* obj);         /* [0x4ab080] */
__declspec(dllimport) int           __stdcall SetBkMode(void* dc, int mode);             /* [0x4ab074] */
__declspec(dllimport) unsigned long __stdcall SetTextColor(void* dc, unsigned long c);   /* [0x4ab0b0] */
__declspec(dllimport) int           __stdcall DrawTextA(void* dc, const char* s, int n,
                                                        WinRect* rc, unsigned int fmt);  /* [0x4ab2ac] */
__declspec(dllimport) int           __stdcall DeleteObject(void* obj);                   /* [0x4ab09c] */

extern void  PushRenderingStatusAndUnlockVideoSurface(void);      /* 0x00464080 */
extern void  PopRenderingStatus(void);                            /* 0x004641f0 */
extern void* SelectFont(void* dc, int font);                      /* 0x00454b40 */

// FUNCTION: LEGOLAND 0x00454d80
void NewPrintColoured(const char* text, int font, WinRect rc, unsigned long colour)
{
    void* hdc;
    void* rgn;
    void* oldrgn;
    void* oldfont;

    rgn = CreateRectRgn(g_clip_rect.left, g_clip_rect.top,
                        g_clip_rect.right, g_clip_rect.bottom);
    PushRenderingStatusAndUnlockVideoSurface();
    g_draw_surface->vtbl->GetDC(g_draw_surface, &hdc);
    SetBkMode(hdc, 1);
    SetTextColor(hdc, colour);
    oldrgn = SelectObject(hdc, rgn);
    oldfont = SelectFont(hdc, font);
    DrawTextA(hdc, text, strlen(text), &rc, 0x20);   /* DT_SINGLELINE */
    SelectObject(hdc, oldfont);
    SelectObject(hdc, oldrgn);
    g_draw_surface->vtbl->ReleaseDC(g_draw_surface, hdc);
    PopRenderingStatus();
    DeleteObject(rgn);
}


/* =========================================================================
 * 0x0046d850 -- scroll a panel's icon group by (dx, dy), clamped.
 *
 * fpui3.c's two scroll buttons are the only callers: a vertical panel gets
 * (0, +-6) and a horizontal one (+-0x20, 0).  `axis` bit 0 selects which pair
 * of edges the work is done on, and the body is that choice twice -- once to
 * decide whether there is anything to scroll at all, once to clamp.
 *
 * The step actually applied is NOT the caller's: it is SnapIconScroll's
 * answer, which picks the icon nearest the box's leading edge and returns the
 * step that lands it exactly there.  Note the snap is asked for the VERTICAL
 * geometry whichever axis is scrolling -- the horizontal path then throws the
 * answer away and clamps the caller's `dx` instead, but the panel's y edges
 * and the menu's saved scroll are still advanced by it.  That asymmetry is
 * the original's.
 *
 * TYPES ARE THE FUNCTION HERE: SnapIconScroll's and MoveIcons's middle
 * parameters are `short`, which is what makes VC6 load the panel's `int`
 * edges 16 bits at a time (`mov ax,[esi+10h]`, `mov dx,di`) and finish the
 * sums with a 32-bit `lea`/`sub` whose upper half is deliberately garbage --
 * the callee only reads 16 bits.  With `int` parameters every one of those
 * becomes a full load and the body does not match.
 * ========================================================================= */

/* The scrolling object-list panel (fpui2.c / fpui3.c's ObjListPanel). */
typedef struct ObjListPanel {
    unsigned short group;       /* +0x00 the icon group it owns */
    short          pad02;
    int            axis;        /* +0x04 bit 0: scrolls vertically */
    Icon*          box;         /* +0x08 the list box icon */
    int            list_x0;     /* +0x0c the extent of the icons on it */
    int            list_y0;     /* +0x10 */
    int            list_x1;     /* +0x14 */
    int            list_y1;     /* +0x18 */
    int            box_x0;      /* +0x1c the box's own extent */
    int            box_y0;      /* +0x20 */
    int            box_x1;      /* +0x24 */
    int            box_y1;      /* +0x28 */
} ObjListPanel;                 /* 0x2c */

/* fpui2.c models 0x00668e64 as a byte "which menu the object list was built
 * for"; it is the index into the per-menu saved scroll positions. */
extern unsigned char g_list_menu;       /* 0x00668e64 */
extern int           g_list_scroll[];   /* 0x00668e44 */

/* Shift every unlocked icon of the masked group (iconui.c). */
extern void MoveIcons(int mask, short group, short dx, short dy);  /* 0x0046dcd0 */
/* 0x0046dd10 (not exported): find the icon of the group that would land
 * closest to `edge` once `delta` is applied without passing `limit`, and
 * return the delta that puts it exactly there. */
extern int  SnapIconScroll(int mask, short limit, short edge, short group,
                           int delta);                             /* 0x0046dd10 */

/* WIP note (2026-09-05): 121/121 instructions and 304/304 BYTES, strict
 * mismatch 35, register-blind 10.  Indices 0..61 -- the whole axis choice,
 * both "it fits" early exits and the entire SnapIconScroll argument block,
 * 16-bit loads and dirty upper halves included -- are exact, and so is the
 * tail from the saved-scroll update on.  The residual is ONE allocation
 * decision, taken at index 62 and carried to the end:
 *
 *   original:  list_x0 -> edx (scratch), list_y0 -> edi, nx -> ebp, ny -> ebx
 *   ours:      list_x0 -> ebp,           list_y0 -> ebx, nx -> edx, ny -> edi
 *
 * i.e. the four values are ranked in exactly the reverse order, so the two
 * LOADS take the callee-saved registers and the two SUMS take the scratch.
 * The register-blind 10 are all in the write-back block and follow from it:
 * with list_x0 in a scratch register the original must reuse edx for the
 * list_x1 and list_y1 loads and therefore interleaves load/add/store, where
 * ours (holding both loads in callee-saved registers) batches the loads.
 *
 * Ruled out, all measured on this baseline and all still 34-35: all 24
 * orders of the four write-backs (34 for six of them, 35 for the rest);
 * both operand orders of each sum; declaring the sums first, last, as one
 * aggregate, as `register`, with two separate temporaries, with the
 * temporary scoped inside each arm, and reusing the sums as the temporary;
 * naming the two loads as their own locals (and as an aggregate); an extra
 * algebraic use of a sum (VC6 folds it away); a separate `sx` for the
 * horizontal step; reusing the `dy` parameter as the step instead of a
 * local; inverting the clamp conditions; and free volatile reads on both
 * `axis` sites and on both box edges -- the volatile test moves nothing,
 * which is the sign of a global web rank rather than a local rotation.
 *
 * What DID get it here, and is worth carrying: (1) both sums must be NAMED
 * LOCALS computed before the axis test -- written inline VC6 folds
 * `step += box - (list + step)` algebraically to `step = box - list`, merges
 * the two clamps into one block and comes out 4 instructions short (117);
 * (2) the inner limit test needs its own named temporary for the same
 * reason; (3) MoveIcons's and SnapIconScroll's middle parameters must be
 * `short`, which is what produces the 16-bit field loads and the
 * deliberately dirty upper halves. */
/* Scope I (2026-09-05): at its measured four-value allocation floor,
 * 35/121 strict, first 62, 304/304 bytes. In addition to the recorded
 * nx/ny aggregate and write-back sweeps, carrying SnapIconScroll's RESULT
 * in a one-field struct or Pos is byte-identical. Its web rank does not
 * move. Keep the asymmetric horizontal/vertical snapping and short ABI.
 * Full measurements: docs/lanes/scope-i.md.
 */
// WIP-FUNCTION: LEGOLAND 0x0046d850  (71.1%, 35/121 strict; four-value allocation floor; first 62)
void ScrollIconPanel(ObjListPanel* w, int dx, int dy)
{
    int step;
    int nx;
    int ny;
    int t;

    if (w->axis & 1) {
        if (w->list_y1 - w->list_y0 <= w->box_y1 - w->box_y0) {
            MoveIcons(0xffff, w->group, 0, w->box_y0 - w->list_y0);
            return;
        }
    } else {
        if (w->list_x1 - w->list_x0 <= w->box_x1 - w->box_x0) {
            MoveIcons(0xffff, w->group, w->box_x0 - w->list_x0, 0);
            return;
        }
    }
    step = SnapIconScroll(0xffff, w->box_y0 - w->list_y0 - dy,
                          w->list_y0 + dy, w->group, dy);
    nx = w->list_x0 + dx;
    ny = w->list_y0 + step;
    if (w->axis & 1) {
        if (ny > w->box_y0) {
            step += w->box_y0 - ny;
        } else {
            t = w->list_y1 + step;
            if (t < w->box_y1)
                step += w->box_y1 - t;
        }
    } else {
        if (nx > w->box_x0) {
            dx += w->box_x0 - nx;
        } else {
            t = w->list_x1 + dx;
            if (t < w->box_x1)
                dx += w->box_x1 - t;
        }
    }
    w->list_x0 += dx;
    w->list_y0 += step;
    w->list_x1 += dx;
    w->list_y1 += step;
    g_list_scroll[g_list_menu] += step;
    MoveIcons(0xffff, w->group, dx, step);
}
