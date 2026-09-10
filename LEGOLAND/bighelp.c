/* LEGOLAND — game-button polling, bubble help, pop-up info init, BMP loader.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only the
 * struct field offsets, global addresses and callee arg counts are
 * load-bearing; names are ours. Types are defined locally so this file does
 * not depend on (or disturb) legoland.h. */

/* ---- shared small types ------------------------------------------------- */
typedef struct Pos { int x; int y; } Pos;

/* ---- the controller record (see input.c) -------------------------------- */
typedef struct Controller {
    int x0;        /* +0x00 */
    int y0;        /* +0x04 */
    int x;         /* +0x08 current cursor x */
    int y;         /* +0x0c current cursor y */
    int dx;        /* +0x10 */
    int dy;        /* +0x14 */
    int buttons;   /* +0x18 button bit set (rebuilt every tick by ScanMouse/ScanKeyboard) */
} Controller;
extern Controller* g_controller;       /* 0x00813b00  CONTROLLERBUFFER */

/* ---- the game-button state block @ 0x00813a40 -------------------------- */
/* One {mask, state} pair per virtual button. `mask` is the Controller.buttons
 * bit the button is bound to (written once by SetupControllers, input2.c);
 * `state` is rebuilt every tick by ReadGameButtons:
 *   bit0 = pressed this tick, bit1 = released this tick, bit2 = held,
 *   bit3 = auto-repeat fired this tick, bit4 = click-latch (set by the
 *   in-game click handling below). */
typedef struct GameButton {
    int mask;
    int state;
} GameButton;

typedef struct GameInput {
    int        flags;        /* +0x00 0x813a40  bits: 2 = map ref moved, 4 = moved (not dragging),
                                8 = cursor moved, 0x10, 0x20 = edge-scroll enabled, 0x200 = idle 2s,
                                0x400 = in-game clicks enabled, 0x1000 = drag in progress */
    Pos        point;        /* +0x04 0x813a44  cursor point (screen) */
    GameButton mouse_a;      /* +0x0c 0x813a4c/50 */
    GameButton mouse_b;      /* +0x14 0x813a54/58 */
    GameButton mouse_c;      /* +0x1c 0x813a5c/60 */
    int        map_x;        /* +0x24 0x813a64  map ref under the cursor */
    int        map_y;        /* +0x28 0x813a68 */
    int        f2c;          /* +0x2c */
    int        f30;          /* +0x30 */
    int        f34;          /* +0x34 0x813a74 */
    int        f38;          /* +0x38 0x813a78 */
    int        click_x;      /* +0x3c 0x813a7c  map ref at the last click */
    int        click_y;      /* +0x40 0x813a80 */
    int        pad44[4];     /* +0x44..+0x50 */
    int        prev_buttons; /* +0x54 0x813a94  last tick's Controller.buttons */
    GameButton up;           /* +0x58 0x813a98 */
    GameButton right;        /* +0x60 0x813aa0 */
    GameButton down;         /* +0x68 0x813aa8 */
    GameButton left;         /* +0x70 0x813ab0 */
    GameButton tab;          /* +0x78 0x813ab8 */
    GameButton btn0;         /* +0x80 0x813ac0 */
    GameButton btn1;         /* +0x88 0x813ac8 */
    GameButton esc;          /* +0x90 0x813ad0 */
    GameButton ret;          /* +0x98 0x813ad8 */
    unsigned long move_tick; /* +0xa0 0x813ae0  tick of the last cursor motion */
} GameInput;
extern GameInput g_input;              /* 0x00813a40 */

/* The game record @ 0x004bcbf4 as this file sees it. */
typedef struct GameRec {
    unsigned short screen_w;   /* +0x00 */
    char           pad02[0x1c];
    unsigned short in_game;    /* +0x1e  non-zero once the map is up */
} GameRec;
extern GameRec* g_game;                /* 0x004bcbf4 */

extern int   g_icon_clicked;           /* 0x00667c48 */
extern int   g_release_swallow;        /* 0x00667108  set elsewhere: eat the next release */
extern unsigned long g_repeat_tick;    /* 0x0066710c  start of the current repeat delay */
extern unsigned long g_repeat_delay;   /* 0x006670fc  200 ms first, then 50 ms */
extern int   g_map_ready;              /* 0x00667c7c */
extern void* g_focussed_icon;          /* 0x006687d0  icon under the mouse */
extern int   g_hit_type;               /* 0x004bdd00  mouse-hit record type */

__declspec(dllimport) unsigned long __stdcall GetTickCount(void);   /* [0x4ab1f8] */
extern unsigned long GetTicks(void);                      /* 0x00499450 (jmp [GetTickCount]) */
extern int   ScrollFromKeys(void);                        /* 0x00451f70  keyboard map scroll; 1 if scrolled */
extern void  MouseScrollMap(void);                        /* 0x004614f0 */
#ifndef LEGOLAND_PORTABLE
extern void  ScreenToMapRef(Pos* screen, Pos* map, int mode); /* 0x0045be90 */
#else
extern int ScreenToMapRef(Pos* screen, Pos* map, int mode); /* 0x0045be90 */
#endif
extern void  BeginMapClick(void);                         /* 0x00452390 */
extern void  UpdateMapDrag(void);                         /* 0x00452030 */

/* Rebuild one button's state word from this tick's edge/held/repeat sets. */
static __inline void SetGameButton(GameButton* b, int pressed, int released, int held, int repeat)
{
    int mask = b->mask;
    b->state = 0;
    if (pressed & mask)
        b->state = 1;
    if (released & mask)
        b->state |= 2;
    if (held & mask)
        b->state |= 4;
    if (repeat & mask)
        b->state |= 8;
}

/* -------------------------------------------------------------------------
 * 0x00452460 -- fold the controller's button word into the game-button state
 * block: press/release edges, held, auto-repeat (200 ms then every 50 ms),
 * then the in-game cursor bookkeeping (map ref under the cursor, edge
 * scrolling, click start) and the 2-second idle flag.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00452460
void ReadGameButtons(void)
{
    int buttons;
    int changed;
    int pressed;
    int released;
    int repeat;
    Pos map;

    g_icon_clicked = 0;
    buttons  = g_controller->buttons;
    changed  = g_input.prev_buttons ^ buttons;
    pressed  = changed & buttons;
    released = changed & ~pressed;
    if (released) {
        if (g_release_swallow) {
            released = 0;
            g_release_swallow = 0;
        }
    }
    if (pressed) {
        g_repeat_tick  = GetTickCount();
        g_repeat_delay = 200;
    }
    if (GetTickCount() - g_repeat_tick > g_repeat_delay) {
        repeat = buttons;
        g_repeat_tick  = GetTickCount();
        g_repeat_delay = 50;
    } else {
        repeat = 0;
    }
    g_input.flags &= ~0x1e;
    g_input.prev_buttons = buttons;

    SetGameButton(&g_input.tab,   pressed, released, buttons, repeat);
    SetGameButton(&g_input.esc,   pressed, released, buttons, repeat);
    SetGameButton(&g_input.up,    pressed, released, buttons, repeat);
    SetGameButton(&g_input.right, pressed, released, buttons, repeat);
    SetGameButton(&g_input.down,  pressed, released, buttons, repeat);
    SetGameButton(&g_input.left,  pressed, released, buttons, repeat);
    SetGameButton(&g_input.btn0,  pressed, released, buttons, repeat);
    SetGameButton(&g_input.btn1,  pressed, released, buttons, repeat);
    SetGameButton(&g_input.ret,   pressed, released, buttons, repeat);

    if (g_game->in_game) {
        g_input.point.x = g_controller->x;
        g_input.point.y = g_controller->y;
        if (g_map_ready) {
            int scrolled = ScrollFromKeys();
            if ((g_input.flags & 0x1000) || g_focussed_icon == 0) {
                if (!scrolled && (g_input.flags & 0x20))
                    MouseScrollMap();
                if (g_controller->dx != 0 || g_controller->dy != 0 || (g_input.flags & 0x10)) {
                    g_input.flags |= 8;
                    g_input.move_tick = GetTicks();
                }
                ScreenToMapRef(&g_input.point, &map, 0);
                if (g_input.map_x != map.x || g_input.map_y != map.y) {
                    g_input.map_y = map.y;
                    g_input.map_x = map.x;
                    if (!(g_input.flags & 0x1000))
                        g_input.flags |= 4;
                    g_input.flags |= 2;
                }
            }
        }
        SetGameButton(&g_input.mouse_a, pressed, released, buttons, repeat);
        SetGameButton(&g_input.mouse_c, pressed, released, buttons, repeat);
        if ((g_input.flags & 4) && (g_input.btn0.state & 4))
            g_input.btn0.state |= 0x10;
        if ((g_input.flags & 0x400) && (g_hit_type & 0x100)) {
            if (g_input.flags & 0x1000) {
                g_input.click_x = g_input.map_x;
                g_input.click_y = g_input.map_y;
                if (g_input.btn0.state & 2) {
                    g_icon_clicked = 1;
                    g_input.btn0.state |= 0x11;
                    g_input.flags &= ~0x1000;
                } else {
                    g_input.btn0.state &= ~0x11;
                }
            } else {
                BeginMapClick();
            }
            UpdateMapDrag();
        }
    }
    if (GetTicks() - g_input.move_tick >= 2000)
        g_input.flags |= 0x200;
    else
        g_input.flags &= ~0x200;
}

/* ---- bubble help --------------------------------------------------------- */

/* Win32 RECT. */
typedef struct WinRect {
    int left;
    int top;
    int right;
    int bottom;
} WinRect;

/* The 5th PrintSprite argument (money.c / panelui.c): {kind, 8-byte payload};
 * kind 5 here. The payload is cleared as one block (see money.c). */
typedef struct BlitCtx {
    int kind;                          /* +0x00 */
    struct { void* p; int n; } sub;    /* +0x04, +0x08 */
} BlitCtx;

/* A sprite as this file sees it: size at +0x14/+0x16. */
typedef struct Sprite {
    char  pad[0x14];
    short w;           /* +0x14 */
    short h;           /* +0x16 */
} Sprite;

/* The "SPEECH BUBBLE" element data @ 0x00813a0c (text.c loads it): its sprite
 * table at +0x08 holds the six bubble-frame pieces —
 *   [0] tail pointing up (bubble below the anchor), [1] tail pointing down,
 *   [2] top-left, [3] top-right, [4] bottom-left, [5] bottom-right corner. */
typedef struct BubbleData {
    int      pad0;       /* +0x00 */
    int      pad4;       /* +0x04 */
    Sprite** sprites;    /* +0x08 */
} BubbleData;
extern BubbleData* g_bubble_data;      /* 0x00813a0c */

/* A rendered-text cache entry (0x455d40 looks one up, 0x455bb0 makes one):
 * size at +0x00/+0x04, the rendered sprite at +0x1c. */
typedef struct TextEntry {
    int w;               /* +0x00 */
    int h;               /* +0x04 */
} TextEntry;

/* The bubble anchor rectangle (iconui.c's HelpRect): x0,y0,x1,y1. */
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
#ifndef LEGOLAND_PORTABLE
extern void  RenderBlock(int x, int y, int w, int h, int colour);             /* 0x004890c0 */
#else
extern int RenderBlock(int x, int y, int w, int h, int colour);             /* 0x004890c0 */
#endif
extern int   GetNearestColour(int r, int g, int b);                           /* 0x0044e6c0 */
extern int   PrintSprite(Sprite* s, int x, int y, int mode, BlitCtx* ctx);    /* 0x004853a0 */
#pragma intrinsic(strlen, memset)
extern unsigned int strlen(const char* s);
extern void* memset(void* d, int c, unsigned int n);

#define DT_WORDBREAK 0x10
#define DT_CALCRECT  0x400
#define BUBBLE_INK   0xd6dede

/* -------------------------------------------------------------------------
 * 0x00455370 -- draw a speech-bubble help box for `text` anchored on `r`:
 * measure (or fetch the cached rendering of) the text, centre the bubble on
 * the anchor and clamp it to the screen, put it above the anchor unless
 * there is no room (then below), draw the box, corners, side wings and tail,
 * blit the text, and mark the hit record when the cursor is over the bubble.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00455370
void BubbleHelp(HelpRect* r, char* text, int font)
{
    TextEntry* ent;
    WinRect    rc;
    BlitCtx    ctx;
    int        h;
    int        tw;
    int        bx;
    struct { int x0, y0, x1, y1; } b;
    int        wx0, wy0, wy1, wy2;
    int        below;
    short      w2;
    short      w0;
    short      h2;
    void*      dc;
    void*      oldfont;

    ctx.kind = 5;
    memset(&ctx.sub, 0, sizeof(ctx.sub));
    rc.left = 0;
    rc.top = 0;
    rc.bottom = 0;
    rc.right = 200;
    ent = FindCachedText(text, font, DT_WORDBREAK, BUBBLE_INK, 0);
    if (ent == 0) {
        dc = CreateCompatibleDC(0);
        SetBkMode(dc, 1);
        oldfont = SelectFont(dc, font);
        h = DrawTextA(dc, text, strlen(text), &rc, DT_WORDBREAK | DT_CALCRECT);
        rc.top = r->y0;
        rc.bottom = rc.top + h;
        SelectObject(dc, oldfont);
        DeleteDC(dc);
        ent = RasterizeText(text, rc.right - rc.left, h, font, DT_WORDBREAK, BUBBLE_INK, 0);
    } else {
        rc.left = 0;
        rc.top = 0;
        rc.right = ent->w;
        rc.bottom = ent->h;
        h = ent->h;
    }

    w0 = g_bubble_data->sprites[0]->w;
    bx = (r->x1 - w0 + r->x0) >> 1;
    if (bx < w0)
        bx = w0;
    else if (bx > g_game->screen_w - w0)
        bx = g_game->screen_w - w0;
    w2 = g_bubble_data->sprites[2]->w;
    h2 = g_bubble_data->sprites[2]->h;

    tw = rc.right - rc.left;
    rc.left = bx - (tw >> 1);
    rc.right = bx + ((tw + 1) >> 1);
    if (rc.left < w2) {
        rc.left = w2;
        rc.right = tw + rc.left;
    } else if (rc.right >= g_game->screen_w - w2) {
        rc.right = g_game->screen_w - w2;
        rc.left = rc.right - tw;
    }
    if (r->y0 < rc.bottom - rc.top + 8) {
        below = 1;
        rc.top = r->y1 + 6;
        rc.bottom = rc.top + h;
    } else {
        below = 0;
        rc.bottom = r->y0 - 6;
        rc.top = rc.bottom - h;
    }

    b.x1 = rc.right + 4;
    b.x0 = rc.left - 4;
    b.y0 = rc.top - 4;
    b.y1 = rc.bottom + 4;
    RenderBlock(b.x0, b.y0, b.x1 - b.x0, 1, 0);
    RenderBlock(b.x0, b.y0 + 1, b.x1 - b.x0, b.y1 - b.y0 - 1, GetNearestColour(0xde, 0xde, 0xd6));
    RenderBlock(b.x0, b.y1, b.x1 - b.x0, 1, 0);
    if (below)
        PrintSprite(g_bubble_data->sprites[0], bx, b.y0 - h2, 0, &ctx);
    else
        PrintSprite(g_bubble_data->sprites[1], bx, b.y1, 0, &ctx);

    w2 = g_bubble_data->sprites[2]->w;
    h2 = g_bubble_data->sprites[2]->h;
    wx0 = b.x0 - w2;
    PrintSprite(g_bubble_data->sprites[2], wx0, b.y0, 0, &ctx);
    wy1 = b.y1 - h2;
    wy0 = b.y0 + h2;
    wy2 = wy1 + 1;
    PrintSprite(g_bubble_data->sprites[4], wx0, wy2, 0, &ctx);
    RenderBlock(wx0, wy0, 1, wy1 - wy0 + 1, 0);
    RenderBlock(wx0 + 1, wy0, w2 - 1, wy1 - wy0 + 1, GetNearestColour(0xde, 0xde, 0xd6));
    PrintSprite(g_bubble_data->sprites[3], b.x1, b.y0, 0, &ctx);
    PrintSprite(g_bubble_data->sprites[5], b.x1, wy2, 0, &ctx);
    RenderBlock(b.x1 + w2 - 1, wy0, 1, wy1 - wy0 + 1, 0);
    RenderBlock(b.x1, wy0, w2 - 1, wy1 - wy0 + 1, GetNearestColour(0xde, 0xde, 0xd6));
    PrintCachedEntry(ent, rc.left, rc.top);

    if (g_input.point.x >= b.x0 && g_input.point.x <= b.x1 &&
        g_input.point.y >= b.y0 && g_input.point.y <= b.y1)
        g_hit_type = 5;
    if (g_input.point.x >= wx0 && g_input.point.x <= b.x1 + w2 &&
        g_input.point.y >= wy0 && g_input.point.y <= wy1)
        g_hit_type = 5;
}

/* ---- pop-up info panel ---------------------------------------------------- */

/* The icon record (iconui.c): only +0x2c/+0x34/+0x38/+0x3c are touched here. */
typedef struct Icon {
    struct Icon*   next;      /* +0x00 */
    Sprite*        sprite;    /* +0x04 */
    void*          data;      /* +0x08 */
    short          x;         /* +0x0c */
    short          y;         /* +0x0e */
    short          w;         /* +0x10 */
    short          h;         /* +0x12 */
    unsigned short group;     /* +0x14 */
    short          pad16;     /* +0x16 */
    int            f18;
    int            f1c;
    int            f20;
    int            f24;
    int          (*render)(struct Icon*);          /* +0x28 */
    char         (*input)(struct Icon*, int);      /* +0x2c */
    void*          widget;    /* +0x30 */
    unsigned int   flags;     /* +0x34 */
    char*          help;      /* +0x38  GetString(help_id) */
    int            help_id;   /* +0x3c */
} Icon;

typedef struct PopUpUI {
    Icon*   icon_mech;       /* 0x007fdea4 */
    int     pad_a8;          /* 0x007fdea8 */
    Sprite* spr_full;        /* 0x007fdeac */
    Sprite* spr_norepair;    /* 0x007fdeb0 */
    int     pad_b4[3];       /* 0x007fdeb4 */
    int     zero[0x3c];      /* 0x007fdec0 .. 0x007fdfb0  cleared here */
    void*   elem_shed;       /* 0x007fdfb0 */
    void*   elem_hut;        /* 0x007fdfb4 */
    void*   elem_path;       /* 0x007fdfb8 */
    void*   elem_entrance;   /* 0x007fdfbc */
    Icon*   icon_close;      /* 0x007fdfc0 */
    Icon*   icon_next;       /* 0x007fdfc4 */
    Sprite* spr_sad;         /* 0x007fdfc8 */
    Icon*   icon_delete2;    /* 0x007fdfcc */
    Sprite* spr_hungry;      /* 0x007fdfd0 */
    int     pad_d4;          /* 0x007fdfd4 */
    Icon*   icon_corner;     /* 0x007fdfd8 */
    Icon*   icon_delete;     /* 0x007fdfdc */
    Icon*   icon_gardener;   /* 0x007fdfe0 */
    Sprite* spr_norm;        /* 0x007fdfe4 */
    Icon*   icon_prev;       /* 0x007fdfe8 */
    int     pad_ec[6];       /* 0x007fdfec .. 0x007fe004 */
    Sprite* spr_repairok;    /* 0x007fe004 */
    Sprite* spr_peckish;     /* 0x007fe008 */
    int     pad_0c[3];       /* 0x007fe00c */
    Sprite* spr_happy;       /* 0x007fe018 */
} PopUpUI;
extern PopUpUI g_popup;                /* 0x007fdea4 */

extern int     g_popup_loaded;         /* 0x00668958 */
extern Sprite* g_pu_bg[9];             /* 0x006688e0 .. 0x00668900 */
extern Sprite* g_pu_mock;              /* 0x00668910 */
extern Sprite* g_pu_delete_on;         /* 0x00668914 */
extern Sprite* g_pu_delete;            /* 0x00668918 */
extern Sprite* g_pu_close_on;          /* 0x0066891c */
extern Sprite* g_pu_close;             /* 0x00668920 */
extern Sprite* g_pu_gardener_on;       /* 0x00668944 */
extern Sprite* g_pu_gardener;          /* 0x00668948 */
extern Sprite* g_pu_mech_on;           /* 0x0066894c */
extern Sprite* g_pu_mech;              /* 0x00668950 */
extern Sprite* g_pu_next;              /* 0x00668928 */
extern Sprite* g_pu_next_on;           /* 0x00668924 */
extern Sprite* g_pu_prev;              /* 0x00668930 */
extern Sprite* g_pu_prev_on;           /* 0x0066892c */

extern const char g_s_shed[];     /* 0x004b89ac "POTTING SHED" */
extern const char g_s_hut[];      /* 0x004b899c "MECHANICS HUT" */
extern const char g_s_path[];     /* 0x004b8a70 "PATH CONTROL" */
extern const char g_s_entrance[]; /* 0x004b83d0 "ENTRANCE 1" */
extern const char g_lls_bgmain[], g_lls_bgct[], g_lls_bgrt[], g_lls_bglm[], g_lls_bgcm[], g_lls_bgrm[],
    g_lls_bglb[], g_lls_bgcb[], g_lls_bgrb[], g_lls_mock[], g_lls_repairok[], g_lls_norepair[],
    g_lls_delete_on[], g_lls_delete[], g_lls_close_on[], g_lls_close[], g_lls_gardener_on[],
    g_lls_gardener[], g_lls_mech_on[], g_lls_mech[], g_lls_next[], g_lls_next_on[], g_lls_prev[],
    g_lls_prev_on[], g_lls_sad[], g_lls_norm[], g_lls_happy[], g_lls_hungry[], g_lls_peckish[],
    g_lls_full[], g_lls_corner[];

extern int     LLIDB_FindElement(const char* name, void** out, unsigned int* idx); /* 0x0047b330 */
__declspec(noreturn) void exit(int code);                                          /* 0x004a02b8 */
extern Sprite* LoadSprite(const char* name, int mode);                             /* 0x00497ab0 */
extern Icon*   InsertIcon(short x, short y, unsigned short group, Sprite* s);      /* 0x0046d6c0 */
extern Icon*   LoadSpriteIcon(const char* name, int mode, int x, int y, int group);/* 0x0046d7b0 */
extern char*   GetString(int id);                                                  /* 0x00498f50 */
extern void    InitPopUpTools(void* a, void* b);                                   /* 0x00470950 */
extern char PU_GardenerInput(Icon*, int);   /* 0x004733f0 */
extern char PU_MechInput(Icon*, int);       /* 0x00473460 */
extern char PU_DeleteInput(Icon*, int);     /* 0x004731a0 */
extern char PU_CloseInput(Icon*, int);      /* 0x004730f0 */
extern char PU_NextInput(Icon*, int);       /* 0x00473360 */
extern char PU_PrevInput(Icon*, int);       /* 0x004733b0 */
extern char PU_Delete2Input(Icon*, int);    /* 0x004734d0 */
extern char PU_ToolA(Icon*, int);           /* 0x00473310 */
extern char PU_ToolB(Icon*, int);           /* 0x004731e0 */

#define PU_GROUP 0x2c3

/* -------------------------------------------------------------------------
 * 0x00470bb0 -- set up the pop-up info panel: look up the four object-info
 * elements (fatal if missing), load the panel sprites once, then create the
 * panel's icons (gardener/mechanic/delete/close/next/prev/corner mask/delete
 * confirm) in group 0x2c3 with their help strings and input handlers.
 * The three `flags |=` are separate statements: VC6 hoists the three
 * constants into ebx/edi/esi and re-reads the icon pointer for each.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00470bb0
void InitPopUpInfo(void)
{
    Icon* p;

    memset(g_popup.zero, 0, 0x100);
    if (LLIDB_FindElement(g_s_shed, &g_popup.elem_shed, 0))
        exit(1);
    if (LLIDB_FindElement(g_s_hut, &g_popup.elem_hut, 0))
        exit(1);
    if (LLIDB_FindElement(g_s_path, &g_popup.elem_path, 0))
        exit(1);
    if (LLIDB_FindElement(g_s_entrance, &g_popup.elem_entrance, 0))
        exit(1);

    if (!g_popup_loaded) {
        g_popup_loaded = 1;
        g_pu_bg[0] = LoadSprite(g_lls_bgmain, 4);
        g_pu_bg[1] = LoadSprite(g_lls_bgct, 4);
        g_pu_bg[2] = LoadSprite(g_lls_bgrt, 4);
        g_pu_bg[3] = LoadSprite(g_lls_bglm, 4);
        g_pu_bg[4] = LoadSprite(g_lls_bgcm, 4);
        g_pu_bg[5] = LoadSprite(g_lls_bgrm, 4);
        g_pu_bg[6] = LoadSprite(g_lls_bglb, 4);
        g_pu_bg[7] = LoadSprite(g_lls_bgcb, 4);
        g_pu_bg[8] = LoadSprite(g_lls_bgrb, 4);
        g_pu_mock = LoadSprite(g_lls_mock, 4);
        g_popup.spr_repairok = LoadSprite(g_lls_repairok, 4);
        g_popup.spr_norepair = LoadSprite(g_lls_norepair, 4);
        g_pu_delete_on = LoadSprite(g_lls_delete_on, 4);
        g_pu_delete = LoadSprite(g_lls_delete, 4);
        g_pu_close_on = LoadSprite(g_lls_close_on, 4);
        g_pu_close = LoadSprite(g_lls_close, 4);
        g_pu_gardener_on = LoadSprite(g_lls_gardener_on, 4);
        g_pu_gardener = LoadSprite(g_lls_gardener, 4);
        g_pu_mech_on = LoadSprite(g_lls_mech_on, 4);
        g_pu_mech = LoadSprite(g_lls_mech, 4);
        g_pu_next = LoadSprite(g_lls_next, 4);
        g_pu_next_on = LoadSprite(g_lls_next_on, 4);
        g_pu_prev = LoadSprite(g_lls_prev, 4);
        g_pu_prev_on = LoadSprite(g_lls_prev_on, 4);
        g_popup.spr_sad = LoadSprite(g_lls_sad, 0);
        g_popup.spr_norm = LoadSprite(g_lls_norm, 0);
        g_popup.spr_happy = LoadSprite(g_lls_happy, 0);
        g_popup.spr_hungry = LoadSprite(g_lls_hungry, 0);
        g_popup.spr_peckish = LoadSprite(g_lls_peckish, 0);
        g_popup.spr_full = LoadSprite(g_lls_full, 0);
    }

    p = InsertIcon(0, 0, PU_GROUP, g_pu_gardener);
    g_popup.icon_gardener = p;
    p->help_id = 0x6e;
    g_popup.icon_gardener->help = GetString(0x6e);
    g_popup.icon_gardener->flags |= 0x2000;
    g_popup.icon_gardener->flags |= 0x4002;
    g_popup.icon_gardener->flags |= 0x400;
    g_popup.icon_gardener->input = PU_GardenerInput;

    p = InsertIcon(0, 0, PU_GROUP, g_pu_mech);
    g_popup.icon_mech = p;
    p->help_id = 0x6f;
    g_popup.icon_mech->help = GetString(0x6f);
    g_popup.icon_mech->flags |= 0x2000;
    g_popup.icon_mech->flags |= 0x4002;
    g_popup.icon_mech->flags |= 0x400;
    g_popup.icon_mech->input = PU_MechInput;

    p = InsertIcon(0, 0, PU_GROUP, g_pu_delete);
    g_popup.icon_delete = p;
    p->help_id = 0x70;
    g_popup.icon_delete->help = GetString(0x70);
    g_popup.icon_delete->flags |= 0x2000;
    g_popup.icon_delete->flags |= 0x4002;
    g_popup.icon_delete->flags |= 0x400;
    g_popup.icon_delete->input = PU_DeleteInput;

    p = InsertIcon(0, 0, PU_GROUP, g_pu_close);
    g_popup.icon_close = p;
    p->help_id = 0x73;
    g_popup.icon_close->help = GetString(0x73);
    g_popup.icon_close->flags |= 0x2000;
    g_popup.icon_close->flags |= 0x4002;
    g_popup.icon_close->flags |= 0x400;
    g_popup.icon_close->input = PU_CloseInput;

    p = InsertIcon(0, 0, PU_GROUP, g_pu_next);
    g_popup.icon_next = p;
    p->help_id = 0x88e;
    g_popup.icon_next->help = GetString(0x88e);
    g_popup.icon_next->flags |= 0x2000;
    g_popup.icon_next->flags |= 0x4002;
    g_popup.icon_next->flags |= 0x400;
    g_popup.icon_next->input = PU_NextInput;

    p = InsertIcon(0, 0, PU_GROUP, g_pu_prev);
    g_popup.icon_prev = p;
    p->help_id = 0x88f;
    g_popup.icon_prev->help = GetString(0x88f);
    g_popup.icon_prev->flags |= 0x2000;
    g_popup.icon_prev->flags |= 0x4002;
    g_popup.icon_prev->flags |= 0x400;
    g_popup.icon_prev->input = PU_PrevInput;

    p = LoadSpriteIcon(g_lls_corner, 4, 0, 0, PU_GROUP);
    g_popup.icon_corner = p;
    p->flags |= 0x400;

    p = InsertIcon(0, 0, PU_GROUP, g_pu_delete);
    g_popup.icon_delete2 = p;
    p->help_id = 0xa1;
    g_popup.icon_delete2->help = GetString(0xa1);
    g_popup.icon_delete2->flags |= 0x2000;
    g_popup.icon_delete2->flags |= 0x4002;
    g_popup.icon_delete2->flags |= 0x400;
    g_popup.icon_delete2->input = PU_Delete2Input;
    InitPopUpTools(PU_ToolB, PU_ToolA);   /* ok_fn = PU_ToolB (0x4731e0), close_fn = PU_ToolA
                                           * (0x473310): the original pushes 0x473310 first */
}
