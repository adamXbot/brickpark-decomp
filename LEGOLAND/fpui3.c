/* LEGOLAND — the free-play side panel, the scroll buttons, the pop-up tool
 * handlers and the goal/help list tick.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field offsets, callee argument counts and global addresses are load-bearing;
 * the names are ours. Types are declared locally, reusing the shapes fpui.c /
 * fpui2.c / iconui.c / bighelp.c already recovered.
 */
#include "legoland.h"

/* ---- the icon record (0x40 bytes, iconui.c / fpui.c) --------------------- */
/* The two slots at +0x18 and +0x1c are polymorphic; a free-play list icon
 * keeps a "placed" flag byte at +0x18, its LLIDB element at +0x1c and the
 * element's id at +0x20. */
typedef struct Icon {
    struct Icon*     next;      /* +0x00 */
    Sprite*          sprite;    /* +0x04 */
    void*            data;      /* +0x08 */
    short            x;         /* +0x0c */
    short            y;         /* +0x0e */
    short            w;         /* +0x10 */
    short            h;         /* +0x12 */
    unsigned short   group;     /* +0x14 */
    short            f16;       /* +0x16 */
    union {
        char         kind;      /* +0x18  panel: 1 up, 2 down, 0xa bar */
        char         placed;    /* +0x18  free-play icon: item already placed */
        void*        owner;     /* +0x18 */
    } u18;
    union {
        char*        text;      /* +0x1c */
        void*        elem;      /* +0x1c  free-play icon: the LLIDB element */
        int          value;     /* +0x1c */
    } u1c;
    int              f20;       /* +0x20  free-play icon: the element's id */
    void*            f24;       /* +0x24 */
    int            (*render)(struct Icon*);                      /* +0x28 */
    char           (*input)(struct Icon*, int ev, short dx, short dy); /* +0x2c */
    void*            widget;    /* +0x30 */
    unsigned int     flags;     /* +0x34 */
    char*            help;      /* +0x38 */
    int              help_id;   /* +0x3c */
} Icon;

/* ---- the scrolling icon panel (fpui.c's ObjListPanel) ------------------- */
/* ScrollIconPanel reads `axis` bit 0 to choose the vertical set of limits
 * (+0x10/+0x18/+0x20/+0x28) over the horizontal one (+0x0c/+0x14/+0x1c/+0x24). */
typedef struct ObjListPanel {
    unsigned short group;      /* +0x00 */
    short          pad02;
    int            axis;       /* +0x04  bit 0: scrolls vertically */
    char           pad08[0x0c - 0x08];
    int            left;       /* +0x0c */
    int            top;        /* +0x10 */
    int            right;      /* +0x14 */
    int            bottom;     /* +0x18 */
    int            left_max;   /* +0x1c */
    int            top_max;    /* +0x20 */
    int            right_min;  /* +0x24 */
    int            bottom_min; /* +0x28 */
} ObjListPanel;

/* ---- the rendered-text cache (0x006675b8 / 0x006675c0) ------------------ */
/* 0x20-byte entries; 0x00455bb0 rasterises one and appends it, this file's
 * FindCachedText looks one up.  The KEY is the whole tuple below except the
 * size: text, format, ink, paper and font.  bighelp.c models the entry as
 * just {w, h} plus "sprite at +0x1c", which agrees with this framing. */
typedef struct TextEntry {
    int     w;        /* +0x00  rendered size */
    int     h;        /* +0x04 */
    int     format;   /* +0x08  the DrawText flags it was rendered with */
    char*   text;     /* +0x0c */
    int     ink;      /* +0x10 */
    int     paper;    /* +0x14 */
    int     font;     /* +0x18 */
    Sprite* sprite;   /* +0x1c  the rendered bitmap */
} TextEntry;          /* 0x20 */

/* ---- globals ------------------------------------------------------------ */
extern Icon*         g_side_icons;         /* 0x006687c8  icon list head */
extern unsigned long g_scroll_repeat_tick; /* 0x006688b4  last scroll-button repeat */
extern int           g_text_cache_count;   /* 0x006675b8 */
extern TextEntry     g_text_cache[];       /* 0x006675c0 */

/* ---- callees ------------------------------------------------------------ */
__declspec(dllimport) unsigned long __stdcall GetTickCount(void);  /* [0x4ab1f8] */
extern int strcmp(const char* a, const char* b);
#pragma intrinsic(strcmp)
/* 0x0046d850 (not exported): move a panel's icon group by (dx, dy), clamped
 * to whichever pair of limits the panel's axis selects. */
extern void ScrollIconPanel(ObjListPanel* w, int dx, int dy);      /* 0x0046d850 */

/* =========================================================================
 *  The side panel's two scroll buttons
 * =========================================================================
 * Both are ordinary icon input handlers: event bit 0 is "pressed", bit 2 is
 * "held".  A press scrolls once and stamps g_scroll_repeat_tick; while held,
 * the same step repeats every 250 ms.  Either way the handler answers 2
 * ("handled, redraw"); an event that is neither press nor hold answers 1.
 * The step is one row (6) when the panel scrolls vertically and one column
 * (0x20) when it scrolls horizontally — ScrollDownInput is the same function
 * with both steps negated.
 */

// FUNCTION: LEGOLAND 0x0046d980
char ScrollUpInput(Icon* p, int ev, short dx, short dy)
{
    ObjListPanel* w;

    if (ev & 1) {
        w = (ObjListPanel*)p->widget;
        g_scroll_repeat_tick = GetTickCount();
        if (w->axis & 1)
            ScrollIconPanel(w, 0, 6);
        else
            ScrollIconPanel(w, 0x20, 0);
        return 2;
    }
    if (ev & 4) {
        if (GetTickCount() - g_scroll_repeat_tick >= 250) {
            w = (ObjListPanel*)p->widget;
            g_scroll_repeat_tick = GetTickCount();
            if (w->axis & 1)
                ScrollIconPanel(w, 0, 6);
            else
                ScrollIconPanel(w, 0x20, 0);
        }
        return 2;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046da20
char ScrollDownInput(Icon* p, int ev, short dx, short dy)
{
    ObjListPanel* w;

    if (ev & 1) {
        w = (ObjListPanel*)p->widget;
        g_scroll_repeat_tick = GetTickCount();
        if (w->axis & 1)
            ScrollIconPanel(w, 0, -6);
        else
            ScrollIconPanel(w, -0x20, 0);
        return 2;
    }
    if (ev & 4) {
        if (GetTickCount() - g_scroll_repeat_tick >= 250) {
            w = (ObjListPanel*)p->widget;
            g_scroll_repeat_tick = GetTickCount();
            if (w->axis & 1)
                ScrollIconPanel(w, 0, -6);
            else
                ScrollIconPanel(w, -0x20, 0);
        }
        return 2;
    }
    return 1;
}

/* =========================================================================
 *  The rendered-text cache lookup
 * =========================================================================
 * Text that goes through the bubble/panel printers is rasterised once into a
 * sprite and kept in a flat array; every draw looks the whole key up here
 * first.  Note the key does NOT include the requested WIDTH — the entry's
 * w/h are the measured result, not part of the identity — so two calls that
 * differ only in the box they were measured against share one rendering.
 * The count global is re-read on every iteration (the array and the count
 * may alias), and a miss walks the whole table.
 *
 * The `volatile` on the count is FREE — the original loads it both in the
 * guard and in the latch, exactly as the plain spelling does — and it is the
 * whole residual: without it VC6's scheduler hoists the guard's load above
 * the four callee-saved pushes (a volatile access cannot cross the pushes'
 * stores), which is 9 of 67 at the identical 153 bytes.
 */

// FUNCTION: LEGOLAND 0x00455d40
TextEntry* FindCachedText(const char* text, int font, int format, int ink, int paper)
{
    int i;

    for (i = 0; i < *(volatile int*)&g_text_cache_count; i++) {
        if (g_text_cache[i].format == format
            && g_text_cache[i].ink == ink
            && g_text_cache[i].paper == paper
            && g_text_cache[i].font == font
            && strcmp(g_text_cache[i].text, text) == 0)
            return &g_text_cache[i];
    }
    return 0;
}

/* =========================================================================
 *  The info pop-up's control bar: the OK (demolish) button
 * =========================================================================
 * popup2.c's InitPopUpTools builds two gadgets and wires PU_ToolA to the
 * close button and PU_ToolB — this one — to the OK button.  Every event first
 * un-lights both gadgets (ResetToolIcons) and then lights THIS one, so the
 * bar highlights on hover; only event bit 1 (release) actually does the work.
 *
 * The work is misc3.c's PopUpCanDelete probe run for real: the whole
 * 0x1834-byte query cursor and the query class pointer are saved into locals,
 * the pop-up's packed cell reference (PopUpInfo.ref, {x = low byte, y = high
 * byte}) is unpacked into the query block AND into the hover square at
 * 0x00667c54, the object's class becomes QueryClass, its object-definition
 * "update2" hook (+0x94) refills the cursor footprint for that cell,
 * BuildCursorPtr turns it into an outline and CursorIsValid grades it.  If it
 * grades, the object really is removed — its path tiles first
 * (RemoveObjectPathTiles) and then the object itself (RemObjFromMap) — after
 * which the saved cursor and class are put back and the pop-up's own state is
 * cleared (ResetInfoStruct), which closes the panel.
 *
 * Two things this shares with the probe: the two calls after the validity
 * test share ONE `add esp,0x18` with each other, and the three calls before
 * it share another; and RemObjFromMap's square argument is read straight out
 * of the global as a WORD (`mov cx,[0x667c54] / push ecx`), which is what an
 * `unsigned short`-wide by-value packed pair does at a call site.
 */

/* The edit/destroy cursor block (objmap2.c's Cursor has the full layout; only
 * its SIZE matters here, because the whole block is saved and restored). */
typedef struct QueryCursor {
    char raw[0x1834];
} QueryCursor;

/* The object class record: the object definition's +0x94 "update2" hook and
 * its context pointer at +0xc4 (misc3.c's QDef). */
typedef struct QDef {
    char  pad0[0x94];
    void  (*update2)(void* ctx, Pos* cell);   /* +0x94 */
    char  pad98[0xc4 - 0x98];
    void* ctx;                                /* +0xc4 */
} QDef;

/* A placed map object: its class sits at +0x0c. */
typedef struct QObj {
    char  pad0[0x0c];
    QDef* cls;                                /* +0x0c */
} QObj;

/* castleobj.c's query block: the cell the +0x94 hook is asked about. */
typedef struct QueryBlock {
    int x;   /* +0x00  0x00811564 */
    int y;   /* +0x04  0x00811568 */
} QueryBlock;

/* The packed map square (anim2.c's BPos / BPosW). */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

extern QObj*        g_popup_obj;      /* 0x007fdec4  PopUpInfo.obj */
extern int          g_popup_ref;      /* 0x007fdec8  PopUpInfo.ref (packed cell) */
extern QDef*        g_query_class;    /* 0x00667c58  castleobj.c QueryClass */
extern QueryCursor  g_query_cursor;   /* 0x00810160  castleobj.c QueryCursor */
#ifndef LEGOLAND_PORTABLE
extern QueryBlock   g_query_block;    /* 0x00811564 */
#else
/* g_query_block is the 8 bytes at +0x1404 of the query cursor, and the
 * `saved = g_query_cursor` / `g_query_cursor = saved` pair around the probe
 * copies the WHOLE 0x1834-byte block -- the two bytes ranges overlap. As two
 * extern objects clang may sink the 6196-byte copy past the two stores below
 * it, which would snapshot the NEW cell and restore it, leaving the query
 * block holding the probe's cell instead of the caller's (PORT-M6 measured a
 * same-storage store/load pair being folded at -O2; the copy happens to stay
 * in program order at -O3 today, so this is latent, not live). Addressing the
 * block through the record makes it one object; the VC6 arm is untouched. */
#define g_query_block (*(QueryBlock*)&g_query_cursor.raw[0x1404]) /* 0x00811564 */
#endif
extern BPosW        g_hover_sq;       /* 0x00667c54  logflume.c g_lf_hover_sq */
extern Sprite*      g_pu_ok_on;       /* 0x00668934  PU_OKON.lls */

extern void SetIconSprite(Icon* p, Sprite* s);              /* 0x0046d680 */
/* 0x00471d60 (not exported, popup2.c): put the control bar's two gadgets
 * back to their unlit sprites. */
extern void ResetToolIcons(void);                           /* 0x00471d60 */
/* 0x00471510 (not exported, iconui.c): clear the pop-up's info block. */
extern void ResetInfoStruct(void);                          /* 0x00471510 */
extern void BuildCursorPtr(QueryCursor* c, int a, int b);   /* 0x0045f5f0 */
extern int  CursorIsValid(QueryCursor* c);                  /* 0x0045f4b0 */
extern void RemoveObjectPathTiles(QDef* def, Pos* pos);     /* 0x0045d3d0 */
extern void RemObjFromMap(QDef* d, void* o, BPos sq, QueryCursor* c); /* 0x00459c90 */

#ifdef LEGOLAND_PORTABLE
/* The Icon +0x2c input slot is called with four arguments (fpui.c
 * CheckFocussedIcon, uimisc.c RestoreFreePlaySelections) and this body reads
 * two, which x86 cdecl tolerates and a wasm call_indirect does not: the first
 * hover over the OK gadget trapped with "function signature mismatch", so no
 * object could be demolished from its pop-up. InitPopUpTools takes the handler
 * through a void*, which is how PORT-M3's slot census missed it. The matched
 * body is renamed for the portable build and a twin of the slot's shape is
 * exported over it (PORT-M3's method; uimisc.c does the same for PU_ToolA). */
#define PU_ToolB PU_ToolB_vc6_body
#endif
// FUNCTION: LEGOLAND 0x004731e0
char PU_ToolB(Icon* p, int ev)
{
    Pos          pt;
    QueryCursor  saved;
    QDef*        prev;
    QDef*        def;
    unsigned int ref;

    ResetToolIcons();
    SetIconSprite(p, g_pu_ok_on);
    if (ev & 2) {
        prev = g_query_class;
        ref = g_popup_ref & 0xffff;
        saved = g_query_cursor;
        pt.x = ref & 0xff;
        pt.y = ref >> 8;
        def = g_popup_obj->cls;
        g_query_class = def;
        g_query_block.x = pt.x;
        g_query_block.y = pt.y;
        g_hover_sq.b.x = (unsigned char)pt.x;
        g_hover_sq.b.y = (unsigned char)pt.y;
        def->update2(def->ctx, &pt);
        BuildCursorPtr(&g_query_cursor, 0, 0);
        if (CursorIsValid(&g_query_cursor)) {
            RemoveObjectPathTiles(g_query_class, &pt);
            RemObjFromMap(g_query_class, g_query_class->ctx, g_hover_sq.b, &g_query_cursor);
        }
        g_query_cursor = saved;
        g_query_class = prev;
        ResetInfoStruct();
    }
    return 1;
}
#ifdef LEGOLAND_PORTABLE
#undef PU_ToolB
char PU_ToolB(Icon* p, int ev, int ll_dx, int ll_dy)
{
    (void)ll_dx;
    (void)ll_dy;
    return PU_ToolB_vc6_body(p, ev);
}
#endif

/* =========================================================================
 *  A free-play list icon: tick / untick one class
 * =========================================================================
 * The free-play screen is a set of scrolling lists of buildable classes; a
 * click (event bit 1) toggles whether the class is in the player's park.
 *
 * TICK.  FreePlayItemAvailable weighs the class against the 0x4e20 budget
 * (and, for a child class, against its parent's "chosen" bit), and on a
 * refusal only the "no" sample is played.  On success the click sample is
 * played unless a bulk update is in progress (g_fp_bulk_update, set around
 * fpui2.c's whole-list passes so a hundred items do not make a hundred
 * clicks), FreePlayItemAdd charges the class, the icon's own "placed" byte
 * goes to 1 and the class element's LLIDB type flag 4 is set — that flag is
 * exactly what FreePlayItemAvailable tests on a CHILD class's parent.
 *
 * UNTICK.  Clearing a class first walks the WHOLE side-icon list and unticks
 * every icon whose parent element (+0x20) is this class — untick a category
 * and its children go with it — and only then unticks the icon itself and
 * clears the element's flag 4.  Note the children are unticked without the
 * remove sample (it is played once, after the walk) and that the walk tests
 * `placed == 1` exactly, not merely non-zero.
 */

extern int   g_fp_bulk_update;   /* 0x00798648  1 while a whole-list pass runs */
extern void* g_snd_close;        /* 0x004b929c  the UI click sample */
extern void* g_snd_fp_denied;    /* 0x004b92d8 */
extern void* g_snd_fp_remove;    /* 0x004b92cc */

extern LLElem* ElemID(const char* name);                          /* 0x0047b3f0 */
#ifndef LEGOLAND_PORTABLE
extern void  PlayInstanceOfSample(void* def, int a, int b, void* src); /* 0x00496d20 */
#else
extern int PlayInstanceOfSample(void* def, int a, int b, void* src); /* 0x00496d20 */
#endif
/* 0x0048aef0 / 0x0048af40 / 0x0048afa0 (not exported): can this class still
 * be afforded, and charge / refund it (both go through 0x0048a840). */
extern int   FreePlayItemAvailable(const char* name, LLElem* parent); /* 0x0048aef0 */
extern void  FreePlayItemAdd(const char* name);                   /* 0x0048af40 */
extern void  FreePlayItemRemove(const char* name);                /* 0x0048afa0 */

// FUNCTION: LEGOLAND 0x0048b000
char FreePlayIconInput(Icon* p, int ev, short dx, short dy)
{
    Icon*   q;
    LLElem* e;

    if (ev & 2) {
        if (p->u18.placed == 0) {
            if (FreePlayItemAvailable((const char*)p->u1c.elem, (LLElem*)p->f20)) {
                if (g_fp_bulk_update == 0)
                    PlayInstanceOfSample(g_snd_close, 0, 1, 0);
                FreePlayItemAdd((const char*)p->u1c.elem);
                p->u18.placed = 1;
                ElemID((const char*)p->u1c.elem)->type_flags |= 4;
            } else {
                PlayInstanceOfSample(g_snd_fp_denied, 0, 1, 0);
            }
        } else {
            e = ElemID((const char*)p->u1c.elem);
            for (q = g_side_icons; q; q = q->next) {
                if (q->f20 && q->u18.placed == 1 && (LLElem*)q->f20 == e) {
                    FreePlayItemRemove((const char*)q->u1c.elem);
                    q->u18.placed = 0;
                }
            }
            PlayInstanceOfSample(g_snd_fp_remove, 0, 1, 0);
            FreePlayItemRemove((const char*)p->u1c.elem);
            p->u18.placed = 0;
            ElemID((const char*)p->u1c.elem)->type_flags &= ~4;
        }
        return 1;
    }
    return 1;
}

#ifdef LEGOLAND_PORTABLE
/* -ll-freeplay-all (ll_portable.h's LL_QOL): tick every class on the picker.
 * fpui2.c's InitFreePlayScreen calls this after RestoreFreePlaySelections, and
 * only a picker that came back with nothing ticked is filled, so whatever
 * RestoreFreePlaySelections brought back is kept.
 *
 * Each tick is the icon's own FreePlayIconInput, so the cost, the Accept cover
 * (FreePlayItemAdd) and the class element's flag 4 move exactly as they do for
 * a click, and g_fp_bulk_update keeps it as quiet as RestoreFreePlaySelections.
 * A child class is only available once its parent has flag 4, whichever order
 * g_side_icons holds them in, so the walk repeats until a pass ticks nothing.
 * A class icon is one in RestoreFreePlaySelections' four groups whose input is
 * FreePlayIconInput (FreePlayObjectList's SetNewGroup_Callbacks). */
static int ll_is_freeplay_class_icon(const Icon* p)
{
    return (p->group == 200 || p->group == 300 || p->group == 400 || p->group == 500)
        && p->input == FreePlayIconInput;
}

void ll_freeplay_tick_all(void)
{
    Icon* p;
    int   ticked;

    for (p = g_side_icons; p; p = p->next)
        if (ll_is_freeplay_class_icon(p) && p->u18.placed)
            return;
    g_fp_bulk_update = 1;
    do {
        ticked = 0;
        for (p = g_side_icons; p; p = p->next) {
            if (!ll_is_freeplay_class_icon(p) || p->u18.placed)
                continue;
            if (!FreePlayItemAvailable((const char*)p->u1c.elem, (LLElem*)p->f20))
                continue;
            p->input(p, 2, 0, 0);
            if (p->u18.placed)
                ticked = 1;
        }
    } while (ticked);
    g_fp_bulk_update = 0;
}
#endif

/* =========================================================================
 *  The side panel slide-in / slide-out
 * =========================================================================
 * fpui.c's RenderIcons calls this once per frame with the elapsed ticks
 * scaled to pixels (dt * 5 / 33, capped) — a signed CHAR, which is why the
 * whole body does its arithmetic in byte and word widths.  It slides the
 * four side-panel icon groups (0xd2 list, 0xd5/0xd6 scroll buttons, 0xd7
 * frame) horizontally and, when the slide finishes, commits the change to the
 * in-game viewport.
 *
 * g_panel_state.f00 is the animation state: 0 = sliding IN, 1 = sliding OUT,
 * 2 = fully out (nothing on screen), 3 = fully in.  States 2 and 3 are steady
 * so the function returns at once.  The local `state` is a copy that the two
 * arrival paths update and that is written back at the end — the loop itself
 * keeps reading the GLOBAL, so a state change made inside the loop takes
 * effect for the icons after it, while the write-back preserves it past the
 * function.
 *
 * ARRIVING IN (state 0).  Every icon moves right by `step`; when the LIST
 * icon (kind 0xa) passes x = 3 the slide is over: it is snapped to x = 0, the
 * remaining icons are moved by the snap correction instead (`step` is
 * rewritten to p->x - x0, i.e. the leftover), state becomes 3, and the game
 * viewport shrinks from 640 to 0x207 with the scroll origin pushed 0x79 tiles
 * right so the visible map does not jump.
 *
 * ARRIVING OUT (state 1).  The mirror image: icons move left by `step`, and
 * when the list icon would pass x = -0x7a the panel is gone — state 2, the
 * marker byte f0c = 0x86, and the list icons are freed.  Until then the
 * partial slide is fed to ClampScrollToMap so the viewport tracks the panel
 * edge.  The one asymmetry is the g_panel_state.f08 latch: if the viewport
 * was still committed when the outward slide starts, the FIRST such icon
 * restores the viewport (0x280 wide, scroll origin back) and returns
 * immediately — without writing `state` back, so the state stays 1 and the
 * slide resumes on the next frame.
 *
 * Three levers this body cost, all measured here:
 *  - `p = g_side_icons;` must be assigned BEFORE the state guard (its load is
 *    in the original's push block) and the guard must test the GLOBAL, not
 *    the `state` copy — testing the copy makes VC6 emit a `mov al,cl` and
 *    costs the whole head.
 *  - `p->x += step;` with the following test reading `p->x` again is exact;
 *    naming the sum in a `short` local swaps the eax/edx pair of
 *    {x0, sum} and, because x0 then dies before `mov eax,[g_map]`, moves four
 *    more instructions.  14 of 121 at identical bytes.
 *  - The order of the nine statements inside the commit block is inert.
 */

/* The side-panel scroll state @ 0x007fdd80 (bigscreens.c's PanelState). */
typedef struct PanelState {
    char f00;    /* +0x00  0 in, 1 out, 2 gone, 3 arrived */
    int  f04;    /* +0x04 */
    int  f08;    /* +0x08  1 = the viewport is committed to the panel */
    char f0c;    /* +0x0c */
} PanelState;

/* The game record @ 0x004bcbf4 seen through the viewport fields (mapscreen.c
 * calls the same object g_scroll_map). */
typedef struct ScrollMap {
    char           pad0[0x10];
    unsigned short view_w;   /* +0x10 */
    unsigned short view_h;   /* +0x12 */
    char           pad14[0x20 - 0x14];
    unsigned short f20;      /* +0x20  the panel's width in view pixels */
} ScrollMap;

extern PanelState g_panel_state;      /* 0x007fdd80 */
extern ScrollMap* g_scroll_map;       /* 0x004bcbf4 (legoland.h's g_map) */
extern int        g_scroll_x;         /* 0x00667cb4  8.8 fixed point */
extern int        g_bg_full_update;   /* 0x004b9220  BGFullUpdate */

extern void ClampScrollToMap(int vw, int vh, int pad_x, int pad_y); /* 0x00461290 */
extern void RemoveObjectListIcons(int group);                       /* 0x0046fb40 */

/* fpui.c declares this `void UpdateSidePanelScroll(int step)`; the definition
 * takes a CHAR.  Both are ABI-identical under __cdecl and the caller pushes a
 * dword either way, but the width is load-bearing HERE: every use is a
 * `movsx` from bl.  Left as the disassembly needs it, as the extern-type rule
 * requires; fpui.c is not touched. */
// FUNCTION: LEGOLAND 0x0046ec50
void UpdateSidePanelScroll(char step)
{
    Icon* p;
    char  state;
    int   x0;
    int   nx;

    p = g_side_icons;
    state = g_panel_state.f00;
    if (g_panel_state.f00 == 2 || g_panel_state.f00 == 3)
        return;
    for (; p; p = p->next) {
        if (p->group == 0xd2 || p->group == 0xd5 || p->group == 0xd6 || p->group == 0xd7) {
            if (g_panel_state.f00 == 0) {
                x0 = p->x;
                p->x += step;
                if (p->u18.kind == 0xa && p->x >= 3) {
                    p->x = 0;
                    step = (char)(p->x - x0);
                    g_panel_state.f0c = 3;
                    state = 3;
                    g_scroll_map->f20 = 0x79;
                    g_scroll_x += 0x7900;
                    g_scroll_map->view_w = 0x207;
                    g_bg_full_update = 1;
                    g_panel_state.f08 = 1;
                }
            } else if (g_panel_state.f00 == 1) {
                if (g_panel_state.f08 != 0) {
                    g_scroll_map->f20 = 0;
                    g_scroll_x -= 0x7900;
                    g_scroll_map->view_w = 0x280;
                    g_panel_state.f08 = 0;
                    g_bg_full_update = 1;
                    return;
                }
                if (p->u18.kind == 0xa) {
                    nx = p->x - step;
                    if (nx < -0x7a) {
                        g_panel_state.f00 = 2;
                        state = 2;
                        g_panel_state.f0c = (char)0x86;
                        RemoveObjectListIcons(0xd2);
                        break;
                    }
                    nx += 0x7a;
                    ClampScrollToMap((g_scroll_map->view_w - nx) << 8,
                                     g_scroll_map->view_h << 8, nx << 8, 0);
                }
                p->x -= step;
            }
        }
    }
    g_panel_state.f00 = state;
}

/* =========================================================================
 *  The script / mission event tick
 * =========================================================================
 * iconui.c's ProcessInGameHelp calls this every 200 ms of game time (hence
 * the name it is declared under there); what it actually drives is the
 * scripted-mission machinery that puts the advisor's goals up.
 *
 * TWO LISTS.
 *  * g_script_event (0x00668784) is a singly linked list of 0x44-byte EVENT
 *    records — the same records savechunks.c serialises.  Each carries a
 *    `kind` at +0x0c that indexes a parallel pair of tables: the per-kind tick
 *    handler at 0x004b9d44 (an event whose slot is null is inert and simply
 *    skipped) and the dirty-mask at 0x004b9e5c that ScriptEventDue tests
 *    against g_map_dirty.  +0x3c is the record's last-serviced time, rebased
 *    on every service; +0x10 is its flag byte.
 *  * g_script_steps (0x00668798) is the list of mission STEPS.  The current
 *    one is g_script_cur (0x0066879c); a step carries a "start" event at +0x0c
 *    and a "completed" event at +0x10, which are spliced onto the event list
 *    as the script advances.
 *
 * ONE PASS over the events.  ScriptEventDue gates the call (a fresh step, a
 * matching dirty bit, or more than 5000 ms since the record was last
 * serviced); the handler's answer decides the record's fate:
 *    handler != 0, flags & 7 == 0  -> the event is finished: unlinked and freed
 *    handler != 0, flags & 7       -> keep it, but mark it done (flag 0x80)
 *    handler == 0, flags & 4       -> mark it done (flag 0x80)
 *    handler == 0, otherwise       -> still live: count it, clear flag 0x80
 *    not due, flag 0x80 clear      -> still live: count it
 * `live` is therefore "how many events this step is still waiting on".
 *
 * ADVANCING.  When there are steps left and `live` has fallen to zero the step
 * is complete: the advisor text is dropped, every event that is not marked
 * 0x02/0x04 (the two "survives a step" bits) is freed, and the current step is
 * retired — its completion event is appended to the event list, the step is
 * unlinked and freed, and `again` is set so the WHOLE function runs a second
 * time over the new step's events without waiting for the next tick.  The new
 * current step is the list head; starting it queues its start event, raises
 * g_script_step_started (which makes ScriptEventDue fire everything once) and
 * restamps the script clock.
 *
 * g_map_dirty ends at -1 (every dirty bit set, so the next pass services
 * everything) on a repeat and at 0x100 on the final pass.
 *
 * Three shapes worth recording:
 *  - The two `flags |= 0x80` arms merge into ONE block placed at the SECOND
 *    site, so the FIRST of them has to be the arm that jumps: written
 *    `if ((flags & 7) == 0) { unlink } else { flags |= 0x80; }`.  The natural
 *    polarity puts the OR inline and costs the whole block layout.
 *  - The first walk unlinks with `p->next` (two full copies of the free) while
 *    the second uses the already-loaded `next` (one shared copy) — a real
 *    difference in the original, reproduced.
 *  - EBP is both the function's constant zero and the first walk's `next`
 *    pointer; the loop exit leaves it zero, which is why every later
 *    "!= 0" test is `cmp mem, ebp`.  Nothing in the source asks for that.
 */

/* A 0x44-byte scripted event (savechunks.c's SaveScriptEvent record). */
typedef struct ScriptEvent {
    struct ScriptEvent* next;    /* +0x00 */
    char          pad04[0x0c - 0x04];
    int           kind;          /* +0x0c  index into the two kind tables */
    unsigned char flags;         /* +0x10  0x02/0x04 survive a step, 0x80 done */
    char          pad11[0x3c - 0x11];
    int           time;          /* +0x3c  last serviced (game time) */
} ScriptEvent;                   /* 0x44 */

/* One mission step (savechunks.c's ScriptStep). */
typedef struct ScriptStep ScriptStep;

typedef int (*EventTickFn)(ScriptEvent*);

extern EventTickFn  g_event_tick[];      /* 0x004b9d44  per-kind tick handlers */
extern ScriptEvent* g_script_event;      /* 0x00668784  pending event list */
extern int          g_script_tick;       /* 0x00668794  GetGameTimer of this pass */
extern int          g_map_dirty;         /* 0x00668610 */
extern int          g_menu_dirty;        /* 0x0066871c */
extern int          g_script_purge;      /* 0x00668788  drop the flag-8 events */
extern int          g_script_step_started; /* 0x00668790 */
extern ScriptStep*  g_script_steps;      /* 0x00668798  head of the step list */
extern ScriptStep*  g_script_cur;        /* 0x0066879c  the step in progress */

/* 0x00471bf0 (not exported): clear the info pop-up's selection unless it is
 * already in state 2. */
extern void ResetInfoSelection(void);                       /* 0x00471bf0 */
extern int  ScriptRunning(void);                            /* 0x0046b280 */
extern void RefreshEntranceTile(int force);                 /* 0x00482b20 */
extern int  GetGameTimer(void);                             /* 0x00499430 */
/* 0x0046b200 (not exported): is this event ready to be serviced? */
extern int  ScriptEventDue(ScriptEvent* e);                 /* 0x0046b200 */
/* 0x00468940 (not exported): free an event record and its owned string. */
extern void FreeScriptEvent(ScriptEvent* e);                /* 0x00468940 */
extern void UpdateMenu(void);                               /* 0x004758c0 */
/* 0x00471d40 (not exported): close the info pop-up if one is open. */
extern void CloseInfoPopUp(void);                           /* 0x00471d40 */
/* 0x0046b290 (not exported): unlink and free every event flagged 0x08. */
extern void DropFlaggedEvents(void);                        /* 0x0046b290 */
/* 0x0046b6b0 (not exported): put the step's text up as advisor help. */
extern void ShowScriptStepText(ScriptStep* s, int mode);    /* 0x0046b6b0 */
/* 0x00468d00 (not exported): g_script_start = GetGameTimer(). */
extern void ResetScriptTimer(void);                         /* 0x00468d00 */
extern void KillAdvisorHelp(void);                          /* 0x0046ce20 */
/* 0x0046c580 / 0x0046c540 (not exported): append the step's completion (+0x10)
 * or start (+0x0c) event to the tail of the event list. */
extern void EnqueueStepEndEvent(ScriptStep* s);             /* 0x0046c580 */
extern void EnqueueStepStartEvent(ScriptStep* s);           /* 0x0046c540 */
/* 0x0046b5d0 / 0x0046b520 (not exported): unlink a step, and free it with the
 * two event records it owns. */
#ifndef LEGOLAND_PORTABLE
extern void UnlinkScriptStep(ScriptStep* s);                /* 0x0046b5d0 */
#else
extern int UnlinkScriptStep(ScriptStep* s);                /* 0x0046b5d0 */
#endif
extern void FreeScriptStep(ScriptStep* s);                  /* 0x0046b520 */

// FUNCTION: LEGOLAND 0x0046b2d0
void UpdateHelpTick(void)
{
    ScriptEvent* p;
    ScriptEvent* prev;
    ScriptEvent* next;
    int          live;
    int          again;

    do {
        p = g_script_event;
        prev = 0;
        live = 0;
        again = 0;
        ResetInfoSelection();
        if (ScriptRunning())
            return;
        if (g_map_dirty & 0x10)
            RefreshEntranceTile(1);
        g_script_tick = GetGameTimer();
        while (p) {
            next = p->next;
            if (g_event_tick[p->kind]) {
                if (ScriptEventDue(p)) {
                    p->time = g_script_tick;
                    if (g_event_tick[p->kind](p)) {
                        if ((p->flags & 7) == 0) {
                            if (prev)
                                prev->next = p->next;
                            else
                                g_script_event = p->next;
                            FreeScriptEvent(p);
                            p = prev;
                        } else {
                            p->flags |= 0x80;
                        }
                    } else if (p->flags & 4) {
                        p->flags |= 0x80;
                    } else {
                        live++;
                        p->flags &= 0x7f;
                    }
                } else if (!(p->flags & 0x80)) {
                    live++;
                }
            }
            prev = p;
            p = next;
        }
        if (g_menu_dirty) {
            UpdateMenu();
            g_menu_dirty = 0;
            CloseInfoPopUp();
        }
        if (g_script_purge) {
            g_script_purge = 0;
            DropFlaggedEvents();
        }
        if (g_script_step_started) {
            g_script_step_started = 0;
            if (live) {
                ShowScriptStepText(g_script_cur, 1);
                ResetScriptTimer();
            }
        }
        if (g_script_steps && live == 0) {
            KillAdvisorHelp();
            prev = 0;
            for (p = g_script_event; p; p = next) {
                next = p->next;
                if (!(p->flags & 6)) {
                    if (prev)
                        prev->next = next;
                    else
                        g_script_event = next;
                    FreeScriptEvent(p);
                    p = prev;
                }
                prev = p;
            }
            if (g_script_cur) {
                EnqueueStepEndEvent(g_script_cur);
                UnlinkScriptStep(g_script_cur);
                FreeScriptStep(g_script_cur);
                again = 1;
            }
            g_script_cur = g_script_steps;
            if (g_script_steps) {
                g_script_step_started = 1;
                EnqueueStepStartEvent(g_script_steps);
                ResetScriptTimer();
            }
        }
        g_map_dirty = -1;
    } while (again);
    g_map_dirty = 0x100;
}
