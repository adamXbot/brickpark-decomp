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

/* fpui.c's 0xd0-byte ObjDef; only the fields the dead builder reads. */
typedef struct ObjDef {
    char       pad00[0x68];
    SpriteRec* icon;      /* +0x68 */
    char       pad6c[0x78 - 0x6c];
    char*      name;      /* +0x78 */
} ObjDef;

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

/* The DirectInput keyboard state array lives at 0x007fdda0 (sysstubs.c names
 * g_left_shift = [0x2a] and g_right_shift = [0x36] out of it). */
extern unsigned char g_left_ctrl;      /* 0x007fddbd  = key[DIK_LCONTROL] */
extern unsigned char g_right_ctrl;     /* 0x007fde3d  = key[DIK_RCONTROL] */

/* --------------------------------------------------------------- callees -- */

unsigned int strlen(const char*);
char* strcpy(char*, const char*);
#pragma intrinsic(strlen, strcpy)

extern Icon* InsertIcon(short x, short y, unsigned short group, SpriteRec* s); /* 0x0046d6c0 */
extern Icon* FindIcon(unsigned short group);                    /* 0x0046d630 */
extern void  MoveIcons(int mask, short group, short dx, short dy); /* 0x0046dcd0 */
extern void  ReferenceSprite(SpriteRec* s);                     /* 0x00497bb0 */
extern int   PrintSprite(SpriteRec* s, int x, int y, int mode, BlitCtx* ctx); /* 0x004853a0 */
extern void  PrintCent(int x, int y, int w, const char* text, int font); /* 0x00454e60 */
extern char* GetString(int id);                                 /* 0x00498f50 */
extern void  PrintCachedText(const char* text, int x, int y, int w, int h,
                             int f1, int f2, int ink, int paper); /* 0x00455e50 */
extern void  InitPopUpTools(IconInputFn ok_fn, IconInputFn close_fn); /* 0x00470950 */
extern void  UnloadPopUpTools(void);                            /* 0x00470b00 */
extern void  ResetToolIcons(void);                              /* 0x00471d60 */
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
