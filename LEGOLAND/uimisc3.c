/* LEGOLAND -- the front-end screen buttons, the script-event plumbing and the
 * small icon/help leaves that uimisc.c, iconui.c, fpui.c/fpui3.c/fpui5.c,
 * mapscreen3.c/mapscreen4.c and sysstubs.c declare but do not own.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  None of
 * these functions is exported; every extent comes from the disassembly by
 * control flow (tools/audit.py).  Struct field OFFSETS, record sizes, callee
 * argument counts and global addresses are load-bearing; the names are ours.
 * Types are declared LOCALLY on purpose (legoland.h is owned elsewhere) and
 * follow uimisc.c, iconui.c, fpui.c, fpui2.c, fpui3.c, fpui5.c, mapscreen3.c,
 * mapscreen4.c and screens3.c.
 *
 * All 23 bodies are EXACT (audit.py [OK], index for index and byte for byte):
 *
 *   0x00490090 AdvertBillundInput        16i/ 50B    0x0046f300 PointInIconRect        21i/ 48B
 *   0x004900d0 AdvertWindsorInput        16i/ 50B    0x0046c540 EnqueueStepStartEvent  23i/ 58B
 *   0x00490110 AdvertCaliforniaInput     16i/ 50B    0x0046c580 EnqueueStepEndEvent    23i/ 58B
 *   0x00475c50 ChildrenBarInput          16i/ 62B    0x0046eaa0 RenderIndicatorIcon    24i/ 76B
 *   0x00475c90 CloseChildrenBarInput     16i/ 62B    0x0046b5d0 UnlinkScriptStep       24i/ 59B
 *   0x004902c0 CertGoBackInput           17i/ 64B    0x0048afa0 FreePlayItemRemove     25i/ 85B
 *   0x00490a20 PlayReportPageNarration   17i/ 64B    0x0048aef0 FreePlayItemAvailable  26i/ 74B
 *   0x00490a60 PlayReportHint            17i/ 64B    0x0046b290 DropFlaggedEvents      26i/ 59B
 *   0x0046cee0 ObjectHelpExpired         17i/ 57B    0x00474830 InGameSecondaryIcon    27i/ 68B
 *   0x0046cf20 AdvisorHelpExpired        17i/ 52B
 *   0x00498b00 ResumeCurrentTrack        18i/ 55B
 *   0x00490050 AdvertGoBackInput         19i/ 64B
 *   0x0044eb10 GetBlokeAgeGroup          20i/ 54B
 *   0x00490300 CertPrintInput            21i/ 67B
 *
 * =========================================================================
 * WHAT THIS FILE ADDS TO THE PICTURE  (spec notes for a browser runtime)
 * =========================================================================
 * EVERY FRONT-END BUTTON IS GATED BY g_frontend_checkbox_closed (0x004bef9c),
 * not just by its event bits.  The five handlers here
 * (AdvertGoBack/Billund/Windsor/California, CertGoBack, CertPrint) all open
 * `if (g_frontend_checkbox_closed && (ev & 2))`, which is exactly the shape
 * screens3.c's title-screen buttons have: while a modal check box is up the
 * whole front end is inert without any icon flags being touched.  Contrast
 * the two IN-GAME handlers in this file (ChildrenBarInput,
 * CloseChildrenBarInput): they test only `ev & 2`.
 *
 * THE ADVERT SCREEN'S THREE PARK BUTTONS ARE ONE SOURCE COMPILED THREE TIMES.
 * Billund/Windsor/California differ in exactly one instruction -- the movie
 * name pushed (`Billund.avi` 0x004bf58c, `Windsor.avi` 0x004bf598,
 * `California.avi` 0x004bf5a4) -- and are otherwise byte-identical:
 * SetPointer(0), PlayMovie(name, 1, 1), SetPointer(6), one merged
 * `add esp,0x14`.  They do NOT play the UI click; Go Back does.
 *
 * GO BACK FROM THE ADVERT SCREEN RESTORES A SAVED FRONT-END STATE BLOCK.
 * 0x0048f9f0 (screens3.c's `PlayTitleMovie`) SAVES g_icons2_mode,
 * (g_edit_mode, g_game_mode, 0x008119b8) and (0x0080ff80, g_cur_screen,
 * g_screen_mode) into the three blocks at 0x007cb30c / 0x007cb2f0 /
 * 0x007cb300; 0x0048fa40 is its mirror image and puts them back.  Go Back
 * calls the restore with the same three blocks, so the advert screen is a
 * pushed/popped sub-state of whatever screen invoked it.
 * ORIGINAL BUG in the restore (not ours to fix, and visible from here):
 * it latches g_screen_mode into eax at entry and writes it back over
 * g_cur_screen (`mov [0x80ff84], eax`) after g_cur_screen has already been
 * restored -- so the saved screen INDEX is discarded and replaced by the
 * old sub-mode.
 *
 * THE CERTIFICATE SCREEN'S PRINT BUTTON IS EDGE-TRIGGERED THROUGH TWO
 * COUNTERS.  CertPrintInput plays the click unconditionally but only arms the
 * save (`g_cert_saving = 2`) when g_cert_result AND g_cert_saving are both at
 * rest, so clicking again while "saving" is on screen does nothing but click.
 * CertGoBackInput drops the backdrop and the printinfo sprite, empties icon
 * group 7, returns the front end to sub-mode 1 and lowers g_cert_active.
 *
 * THE CHILDREN BAR IS A CLASS-LIST DISCLOSURE TRIANGLE.  Both bar handlers
 * carry the ObjNode of the PARENT class in the icon's +0x18 slot
 * (iconui.c's ListChildrenBar/CloseChildrenBar put it there) and toggle bit
 * 8 of that class's LLIDB element flags -- the same bit
 * fpui2.c's MakeUpObjectList reads to decide whether a run of child nodes is
 * drawn or skipped -- then rebuild the whole panel with
 * MakeUpObjectList(0xd2, 3, 0x21, 0x154).  Both also stamp the mouse-hit
 * record at 0x004bdd00 with 0x100 first, which is what stops the rebuild from
 * immediately re-focussing the icon that is about to be destroyed.
 *
 * THE HELP QUEUE HAS TWO SEPARATE EXPIRY TESTS, AND THEY ARE NOT SYMMETRIC.
 *  * AdvisorHelpExpired: the bubble dies after THIRTY SECONDS
 *    (`GetGameTimer() - g_advisor_start > 0x7530`), or early if the advisor
 *    was raised with a non-zero argument and the object-help queue's head is
 *    a non-zero KIND (i.e. something more urgent is waiting).
 *  * ObjectHelpExpired: the same "something is waiting" test, but its timer
 *    arm is `GetGameTimer() - g_advisor_last < 0` -- a test on the LAST TIME
 *    THE BUBBLE WAS DRAWN, which iconui.c's ProcessInGameHelp stamps to the
 *    current time on every frame the bubble is up.  The difference can only
 *    be negative if the stamp is in the future, which it never is, so in
 *    practice object help is dropped on the first frame after the advisor
 *    bubble closes.  Recorded as measured; the `jns` is what the shipped
 *    binary has.
 *  ObjectHelpExpired also clears the hint-is-up flag at 0x00668618, which
 *  0x0046b700 (start the current step) raises when it puts a hint string up.
 *
 * SCRIPT STEPS AND EVENTS.  A ScriptStep owns a "start" event at +0x0c and a
 * "completed" event at +0x10; EnqueueStepStartEvent / EnqueueStepEndEvent are
 * ONE SOURCE compiled twice (they differ only in that offset).  Each walks
 * g_script_event to its LAST record and appends there -- an O(n) append with
 * no tail pointer -- then NULLS the step's own slot, so the step never owns
 * the record twice.  UnlinkScriptStep is the matching removal from
 * g_script_steps and answers 1/0 for found/not-found even though every caller
 * in fpui3.c ignores it.  DropFlaggedEvents is the purge pass: it unlinks and
 * frees every event carrying flag 0x08, and is the same prev/next/`p = prev`
 * walk fpui3.c's UpdateHelpTick uses for its own flag-6 purge.
 *
 * THE REPORT SCREEN'S NARRATION IS "<prefix>%02d.wav", ONE-BASED.  Both
 * PlayReportPageNarration (page narration, prefix buffer 0x007cae80) and
 * PlayReportHint (hint narration, prefix 0x007cb1e0) format
 * `%s%02d.wav` (0x004bf688) into a 256-byte stack buffer from `index + 1`,
 * duck whatever is playing (PauseCurrentTrack), start the clip
 * (PlayNarrationFile), restart the ducked track (ResumeCurrentTrack) and set
 * the advisor face to pose 5.  The two prefixes are filled by 0x00490740 /
 * 0x00490770 from a _splitpath of the report's own file name.  uimisc.c's
 * UpdateReportPageIcons calls the page one with `g_rep_page / 14`, so page 1
 * (lines 1..14) narrates file 01.
 *
 * THE BLOKE "AGE GROUP" IS A FOUR-THRESHOLD BUCKET ON +0x7c, AND IT HAS A
 * MASTER SWITCH.  With 0x00832990 (workers3.c's g_visitor_tire) clear every
 * bloke reports group 1; otherwise the u16 at +0x7c is bracketed by the four
 * ints at 0x004b8334 -- 1000, 2400, 4000, 7000 -- giving 0..4.  rides.c calls
 * that field `hunger` and declares the SECOND threshold separately as
 * `g_initial_hunger_max` (0x004b8338), which is the same object seen from the
 * other side: InitBlokeAI seeds +0x7c with Rand_Tween(0, 2400), i.e. groups
 * 0 and 1 only.  simcore.c's Calc_Item_Attractiveness then switches on the
 * result, so groups 3 and 4 are only reachable once the field has grown.
 *
 * FREE PLAY'S BUDGET IS 20000 BRICKS AND IT IS CHECKED BEFORE THE ADD.
 * FreePlayItemAvailable answers "can this class still be selected?": the
 * class's own cost plus g_freeplay_progress must not exceed 0x4e20, and the
 * class must either be top-level (no parent), a direct child of "BUILD MENU",
 * or a child whose parent element carries flag 4 (its group is expanded).
 * It passes a NULL out-pointer to the table lookup, which is what makes the
 * lookup's optional row store optional.  FreePlayItemRemove is the refund
 * side: it subtracts the cost, clears the row's `chosen` word and, when the
 * selection count reaches zero, greys the Accept icon (flag 0x400).
 *
 * =========================================================================
 * CODEGEN FACTS RECOVERED HERE (details at the markers that carry them)
 * =========================================================================
 *  - `mov al,1` + `ret` with no `xor eax,eax` anywhere is the whole body of
 *    an icon handler whose only statement is a guarded block: `char` return,
 *    ONE textual `return 1`, and the guard's failure edge jumping straight
 *    to it.
 *  - A merged `add esp` spans EVERY call in a guarded block when no call
 *    consumes another's result: `add esp,0x14` for SetPointer/PlayMovie/
 *    SetPointer, `add esp,0x1c` for PlayInstanceOfSample + the 3-argument
 *    restore, `add esp,0x114` for sprintf + PlayNarrationFile plus the
 *    256-byte buffer.
 *  - NEW: for a two-statement block "store a global, then walk a pointer
 *    chain", the chain's head must be a FUNCTION-LEVEL local assigned after
 *    the store.  Inline, the chain takes the eax->ecx->edx rotation where
 *    the original's first dereference is in place; at BLOCK scope the
 *    rotation is right but the global store sinks below three of the four
 *    argument pushes.  Only the function-level local gets both.  Measured
 *    on ChildrenBarInput and confirmed on its twin (7 spellings).
 *  - NEW: a call result added to a global must be a NAMED LOCAL to make the
 *    global's load the `add`'s destination.  `g_x + f(a)` inline gives
 *    `add <callresult>,<globalload>`; `int c = f(a); ... g_x + c` gives the
 *    original's `mov ecx,[g_x] / add ecx,eax`.  Operand commutation does not
 *    reach it (`f(a) + g_x` is identical to the inline form).
 *    (FreePlayItemAvailable, one byte and 3 instructions.)
 *  - CONFIRMED at a new site: `i <= N-1` versus `i < N` on a strength-reduced
 *    table cursor is `jle` against `&tbl[N-1]` versus `jl` against `&tbl[N]`
 *    -- the last instruction of GetBlokeAgeGroup's residual.  And a
 *    preheader `mov ecx,1` paired with a trailing `lea eax,[ecx-1]` proves
 *    the source counted from ONE: the 0-based `for (i = 0; i < 4; i++) ...
 *    return i;` is two instructions shorter and emits no `lea` at all.
 *
 * EXTERN-TYPE DIVERGENCES (deliberate; caller-side types are levers and no
 * shared header was touched):
 *  - The three Advert*Input handlers and AdvertGoBackInput are defined here
 *    as `char (Icon*, int)`; mapscreen3.c declares them `char (Icon*, int,
 *    int, int)`.  cdecl hides it -- the same situation codex-c recorded for
 *    screen.c's empty-parameter callback declarations -- and the caller side
 *    is correctly left alone.
 *  - InGameSecondaryIcon is `char (Icon*, int, int, int)` here where
 *    sysstubs.c declares `unsigned char (void*, int, int, int)`; it forwards
 *    four dwords to 0x00475120, so OptionsIconInput needs the four-argument
 *    prototype here even though fpui.c declares it `char (Icon*, int)`.
 *  - UnlinkScriptStep and ResumeCurrentTrack really return `int` (1/0);
 *    fpui3.c and fpui5.c declare both `void` because no caller looks.  The
 *    `int` return is load-bearing for the definition (it is what emits the
 *    `mov eax,1` blocks) and inert for the callers.
 *  - FreePlayItemAvailable follows fpui3.c's `(const char*, LLElem*)`, not
 *    fpui.c's `(int, int)`; ABI-identical.
 * ========================================================================= */

/* ------------------------------------------------------------------ types -- */

typedef struct Pos      { int x; int y; } Pos;
typedef struct ClipRect { int left; int top; int right; int bottom; } ClipRect;
typedef struct Sprite   Sprite;

/* An LLIDB element (legoland.h's LLElem): +0x08 is the type/flag word this
 * file toggles bit 8 of (the object list's "children shown") and tests bit 4
 * of (free play's "group expanded"). */
typedef struct LLElem {
    char*        name;        /* +0x00 */
    char*        image;       /* +0x04 */
    unsigned int type_flags;  /* +0x08 */
    void*        data;        /* +0x0c */
    unsigned int refcount;    /* +0x10 */
} LLElem;

/* An object class record (fpui2.c's ObjDef) seen through its own element. */
typedef struct ObjDef {
    char    pad00[0xc4];
    LLElem* elem;             /* +0xc4 */
} ObjDef;

/* The 12-byte object-list node (fpui.c / fpui2.c's ObjNode). */
typedef struct ObjNode {
    struct ObjNode* next;     /* +0x00 */
    ObjDef*         obj;      /* +0x04 */
    int             keep;     /* +0x08 */
} ObjNode;

/* iconui.c's 0x40-byte icon record.  +0x18 is polymorphic; the two children
 * bars keep the parent class's ObjNode there. */
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
    ObjNode*       owner;     /* +0x18 */
    char           pad1c[0x28 - 0x1c];
    int          (*render)(struct Icon*);           /* +0x28 */
    char         (*input)(struct Icon*, int);       /* +0x2c */
    void*          widget;    /* +0x30 */
    unsigned int   flags;     /* +0x34 */
    char*          help;      /* +0x38 */
    int            help_id;   /* +0x3c */
} Icon;

/* The 5th PrintSprite argument (money.c / popup2.c's BlitCtx). */
typedef struct BlitCtx {
    int kind;                            /* +0x00 */
    struct { Icon* p; int n; } owner;    /* +0x04, +0x08 */
} BlitCtx;

/* A 0x44-byte scripted event (fpui3.c's ScriptEvent). */
typedef struct ScriptEvent {
    struct ScriptEvent* next;    /* +0x00 */
    char          pad04[0x0c - 0x04];
    int           kind;          /* +0x0c */
    unsigned char flags;         /* +0x10  0x08 = drop me on the next purge */
    char          pad11[0x44 - 0x11];
} ScriptEvent;                   /* 0x44 */

/* One mission step (uimisc.c's ScriptStep). */
typedef struct ScriptStep {
    struct ScriptStep* next;     /* +0x00 */
    int                id;       /* +0x04 */
    char*              text;     /* +0x08 */
    ScriptEvent*       ev_start; /* +0x0c */
    ScriptEvent*       ev_end;   /* +0x10 */
} ScriptStep;

/* One row of the free-play table (fpui5.c's FPTableEntry). */
typedef struct FPTableEntry {
    unsigned char id;      /* +0x00 */
    char          pad01[3];
    char*         name;    /* +0x04 */
    int           cost;    /* +0x08 */
    int           chosen;  /* +0x0c */
} FPTableEntry;

/* A park visitor, seen through the one field this file reads.  rides.c calls
 * +0x7c `hunger`; here it is the value the four age-group thresholds
 * bracket.  Same object, two readings -- see the file header. */
typedef struct Bloke {
    char           pad00[0x7c];
    unsigned short age;    /* +0x7c */
} Bloke;

/* The narration buffer's DirectSound interface (audio4.c's IDSBuffer), seen
 * through Play at vtable +0x30. */
typedef struct IDSBuffer IDSBuffer;
typedef struct IDSBufferVtbl {
    char pad00[0x30];
    long (__stdcall* Play)(IDSBuffer*, unsigned long, unsigned long,
                           unsigned long);            /* +0x30 */
} IDSBufferVtbl;
struct IDSBuffer { IDSBufferVtbl* vtbl; };

/* ---------------------------------------------------------------- globals -- */

extern int    g_frontend_checkbox_closed; /* 0x004bef9c  0 while a modal box is up */
extern void*  g_snd_click;                /* 0x004b92c0  the UI click sample */

/* The advert screen's three clip names (writable .data string literals). */
extern char   g_avi_billund[];            /* 0x004bf58c "Billund.avi" */
extern char   g_avi_windsor[];            /* 0x004bf598 "Windsor.avi" */
extern char   g_avi_california[];         /* 0x004bf5a4 "California.avi" */

/* The three saved front-end state blocks 0x0048f9f0 fills and 0x0048fa40
 * restores (screens3.c names the same three ints). */
extern int    g_movie_state_a;            /* 0x007cb30c */
extern int    g_movie_state_b;            /* 0x007cb300 */
extern int    g_movie_state_c;            /* 0x007cb2f0 */

extern int    g_screen_mode;              /* 0x0080ff88  front-end sub-mode */
extern int    g_cert_active;              /* 0x00798770  1 while the certificate screen is up */
extern int    g_cert_result;              /* 0x00798768 */
extern int    g_cert_saving;              /* 0x0079876c  2 = start the bitmap save */

extern int    g_hit_type;                 /* 0x004bdd00  the mouse-hit record's kind */

extern int    g_advisor_arg;              /* 0x007fe044 */
extern int    g_advisor_start;            /* 0x007fe04c  GetGameTimer at DisplayAdvisorHelp */
extern int    g_advisor_last;             /* 0x007fe050  GetGameTimer of the last drawn bubble */
extern int    g_hint_up;                  /* 0x00668618  a step hint string is on screen */
extern ScriptEvent* g_object_help;        /* 0x00668724  the object-help queue head */

extern ScriptEvent* g_script_event;       /* 0x00668784  pending event list */
extern ScriptStep*  g_script_steps;       /* 0x00668798  head of the step list */

extern int    g_speech_state;             /* 0x0079a84c  0 idle, 3 playing */
extern IDSBuffer* g_speech_buffer;        /* 0x0079a848 */

extern int    g_visitor_tire;             /* 0x00832990  the age-group master switch */
extern int    g_bloke_age_limits[4];      /* 0x004b8334  1000, 2400, 4000, 7000 */

extern int    g_freeplay_progress;        /* 0x007cb3a0  bricks spent in free play */
extern int    g_freeplay_selected_count;  /* 0x00798650 */
extern Icon*  g_fp_accept_icon;           /* 0x0079864c */
extern char   g_build_menu_name[];        /* 0x004bb49c "BUILD MENU" */

extern int    g_drag_lock;                /* 0x00668954 */

/* The two report-narration filename prefixes 0x00490740 / 0x00490770 build. */
extern char   g_rep_page_narration[];     /* 0x007cae80 */
extern char   g_rep_hint_narration[];     /* 0x007cb1e0 */
extern const char g_fmt_wav[];            /* 0x004bf688 "%s%02d.wav" */

/* ---------------------------------------------------------------- callees -- */

extern int  sprintf(char* dst, const char* fmt, ...);              /* 0x0049e573 (CRT) */

#ifndef LEGOLAND_PORTABLE
extern void SetPointer(int shape);                                 /* 0x00463850 */
#else
extern int SetPointer(int shape);                                 /* 0x00463850 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void PlayMovie(const char* name, int a, int b);             /* 0x004771f0 */
#else
extern int PlayMovie(const char* name, int a, int b);             /* 0x004771f0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void PlayInstanceOfSample(void* def, int a, int b, void* src); /* 0x00496d20 */
#else
extern int PlayInstanceOfSample(void* def, int a, int b, void* src); /* 0x00496d20 */
#endif
extern int  GetGameTimer(void);                                    /* 0x00499430 */
extern int  GetBlink(void);                                        /* 0x00499480 */
extern int  PrintSprite(Sprite* s, int x, int y, int mode, BlitCtx* ctx); /* 0x004853a0 */
extern int  MakeUpObjectList(int group, int x, int y, int h);      /* 0x00475960 */
extern LLElem* ElemID(const char* name);                           /* 0x0047b3f0 */
extern int  FreePlayItemUpdate(const char* name, FPTableEntry** out); /* 0x0048a840 */
extern void FreeScriptEvent(ScriptEvent* e);                       /* 0x00468940 */
#ifndef LEGOLAND_PORTABLE
extern int  CheckWorkerOnMouseStatus(int mode);                    /* 0x00470620 */
#else
extern void CheckWorkerOnMouseStatus(int mode);                    /* 0x00470620 */
#endif
extern char OptionsIconInput(Icon* p, int ev, int dx, int dy);     /* 0x00475120 */

/* 0x0048ffb0 (not exported): drop the front-end backdrop and empty icon
 * group 7 -- the advert screen's teardown. */
extern void KillAdvertScreenSprites(void);                         /* 0x0048ffb0 */
/* 0x00490270 (not exported): the same teardown plus the printinfo sprite. */
extern void KillCertScreenSprites(void);                           /* 0x00490270 */
/* 0x0048fa40 (not exported): the mirror image of screens3.c's
 * `PlayTitleMovie` (0x0048f9f0) -- put the three saved front-end state
 * blocks back.  See the file header for the bug it carries. */
extern void RestoreFrontEndState(int* a, int* b, int* c);          /* 0x0048fa40 */
/* 0x00473130 (not exported): if the info pop-up is open, close it through
 * PU_CloseInput(0, 2, 0, 0) and answer 1; 0 if there was nothing open. */
extern int  CloseInfoPopUpIfOpen(void);                            /* 0x00473130 */
/* 0x004989b0 (not exported): stop the narration buffer, rewind it and refill
 * it from the ACM stream. */
#ifndef LEGOLAND_PORTABLE
extern void RewindNarrationBuffer(void);                           /* 0x004989b0 */
#else
extern int RewindNarrationBuffer(void);                           /* 0x004989b0 */
#endif
/* 0x00498920 / 0x00498630 (not exported): audio4.c's duck / play pair. */
extern int  PauseCurrentTrack(void);                               /* 0x00498920 */
extern int  PlayNarrationFile(const char* name);                   /* 0x00498630 */
/* 0x0046d390 (not exported): g_help_face_state (0x006687a4) = 5. */
extern void SetHelpFaceState5(void);                               /* 0x0046d390 */

/* =========================================================================
 *  The advert screen's four buttons (mapscreen3.c's InitScreen9 installs them)
 * =========================================================================
 * The three park buttons are one source compiled three times: hide the
 * pointer, run the clip full-screen, bring the pointer back as shape 6.  With
 * nothing consuming another call's result all three cleanups defer into one
 * `add esp,0x14`.
 */

#ifdef LEGOLAND_PORTABLE
/* PORT-M3: the Icon +0x2c input slot is called with four arguments
 * (fpui.c CheckFocussedIcon, uimisc.c RestoreFreePlaySelections) and so
 * are g_icon_handler1/2; this body reads only the first two, which x86
 * cdecl tolerates and a wasm call_indirect does not. The matched body is
 * renamed for the portable build only and a twin of the slot's shape is
 * exported over it. VC6 compiles the #ifndef world unchanged. */
#define AdvertBillundInput AdvertBillundInput_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00490090
char AdvertBillundInput(Icon* p, int ev)
{
    if (g_frontend_checkbox_closed && (ev & 2)) {
        SetPointer(0);
        PlayMovie(g_avi_billund, 1, 1);
        SetPointer(6);
    }
    return 1;
}
#ifdef LEGOLAND_PORTABLE
#undef AdvertBillundInput
char AdvertBillundInput(Icon* p, int ev, int ll_dx, int ll_dy)
{
    (void)ll_dx;
    (void)ll_dy;
    return AdvertBillundInput_vc6_body(p, ev);
}
#endif

#ifdef LEGOLAND_PORTABLE
/* PORT-M3: the Icon +0x2c input slot is called with four arguments
 * (fpui.c CheckFocussedIcon, uimisc.c RestoreFreePlaySelections) and so
 * are g_icon_handler1/2; this body reads only the first two, which x86
 * cdecl tolerates and a wasm call_indirect does not. The matched body is
 * renamed for the portable build only and a twin of the slot's shape is
 * exported over it. VC6 compiles the #ifndef world unchanged. */
#define AdvertWindsorInput AdvertWindsorInput_vc6_body
#endif
// FUNCTION: LEGOLAND 0x004900d0
char AdvertWindsorInput(Icon* p, int ev)
{
    if (g_frontend_checkbox_closed && (ev & 2)) {
        SetPointer(0);
        PlayMovie(g_avi_windsor, 1, 1);
        SetPointer(6);
    }
    return 1;
}
#ifdef LEGOLAND_PORTABLE
#undef AdvertWindsorInput
char AdvertWindsorInput(Icon* p, int ev, int ll_dx, int ll_dy)
{
    (void)ll_dx;
    (void)ll_dy;
    return AdvertWindsorInput_vc6_body(p, ev);
}
#endif

#ifdef LEGOLAND_PORTABLE
/* PORT-M3: the Icon +0x2c input slot is called with four arguments
 * (fpui.c CheckFocussedIcon, uimisc.c RestoreFreePlaySelections) and so
 * are g_icon_handler1/2; this body reads only the first two, which x86
 * cdecl tolerates and a wasm call_indirect does not. The matched body is
 * renamed for the portable build only and a twin of the slot's shape is
 * exported over it. VC6 compiles the #ifndef world unchanged. */
#define AdvertCaliforniaInput AdvertCaliforniaInput_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00490110
char AdvertCaliforniaInput(Icon* p, int ev)
{
    if (g_frontend_checkbox_closed && (ev & 2)) {
        SetPointer(0);
        PlayMovie(g_avi_california, 1, 1);
        SetPointer(6);
    }
    return 1;
}
#ifdef LEGOLAND_PORTABLE
#undef AdvertCaliforniaInput
char AdvertCaliforniaInput(Icon* p, int ev, int ll_dx, int ll_dy)
{
    (void)ll_dx;
    (void)ll_dy;
    return AdvertCaliforniaInput_vc6_body(p, ev);
}
#endif

/* Go Back: click, tear the screen down and pop the saved front-end state. */
#ifdef LEGOLAND_PORTABLE
/* PORT-M3: the Icon +0x2c input slot is called with four arguments
 * (fpui.c CheckFocussedIcon, uimisc.c RestoreFreePlaySelections) and so
 * are g_icon_handler1/2; this body reads only the first two, which x86
 * cdecl tolerates and a wasm call_indirect does not. The matched body is
 * renamed for the portable build only and a twin of the slot's shape is
 * exported over it. VC6 compiles the #ifndef world unchanged. */
#define AdvertGoBackInput AdvertGoBackInput_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00490050
char AdvertGoBackInput(Icon* p, int ev)
{
    if (g_frontend_checkbox_closed && (ev & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        KillAdvertScreenSprites();
        RestoreFrontEndState(&g_movie_state_a, &g_movie_state_b,
                             &g_movie_state_c);
    }
    return 1;
}
#ifdef LEGOLAND_PORTABLE
#undef AdvertGoBackInput
char AdvertGoBackInput(Icon* p, int ev, int ll_dx, int ll_dy)
{
    (void)ll_dx;
    (void)ll_dy;
    return AdvertGoBackInput_vc6_body(p, ev);
}
#endif

/* =========================================================================
 *  The certificate screen's two buttons (mapscreen4.c's InitScreen8)
 * ========================================================================= */

#ifdef LEGOLAND_PORTABLE
/* PORT-M3: the Icon +0x2c input slot is called with four arguments
 * (fpui.c CheckFocussedIcon, uimisc.c RestoreFreePlaySelections) and so
 * are g_icon_handler1/2; this body reads only the first two, which x86
 * cdecl tolerates and a wasm call_indirect does not. The matched body is
 * renamed for the portable build only and a twin of the slot's shape is
 * exported over it. VC6 compiles the #ifndef world unchanged. */
#define CertGoBackInput CertGoBackInput_vc6_body
#endif
// FUNCTION: LEGOLAND 0x004902c0
char CertGoBackInput(Icon* p, int ev)
{
    if (g_frontend_checkbox_closed && (ev & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        KillCertScreenSprites();
        g_screen_mode = 1;
        g_cert_active = 0;
    }
    return 1;
}
#ifdef LEGOLAND_PORTABLE
#undef CertGoBackInput
char CertGoBackInput(Icon* p, int ev, int ll_dx, int ll_dy)
{
    (void)ll_dx;
    (void)ll_dy;
    return CertGoBackInput_vc6_body(p, ev);
}
#endif

/* Arm the bitmap save, but only from rest: mapscreen2.c's PrintScreenMode8
 * counts g_cert_saving down into SaveCertificateBitmap and then counts
 * g_cert_result back towards zero while the result is displayed. */
#ifdef LEGOLAND_PORTABLE
/* PORT-M3: the Icon +0x2c input slot is called with four arguments
 * (fpui.c CheckFocussedIcon, uimisc.c RestoreFreePlaySelections) and so
 * are g_icon_handler1/2; this body reads only the first two, which x86
 * cdecl tolerates and a wasm call_indirect does not. The matched body is
 * renamed for the portable build only and a twin of the slot's shape is
 * exported over it. VC6 compiles the #ifndef world unchanged. */
#define CertPrintInput CertPrintInput_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00490300
char CertPrintInput(Icon* p, int ev)
{
    if (g_frontend_checkbox_closed && (ev & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        if (!g_cert_result && !g_cert_saving)
            g_cert_saving = 2;
    }
    return 1;
}
#ifdef LEGOLAND_PORTABLE
#undef CertPrintInput
char CertPrintInput(Icon* p, int ev, int ll_dx, int ll_dy)
{
    (void)ll_dx;
    (void)ll_dy;
    return CertPrintInput_vc6_body(p, ev);
}
#endif

/* =========================================================================
 *  The object list's children bar (iconui.c's ListChildrenBar pair)
 * =========================================================================
 * NEW LEVER, measured here on both twins: the parent node MUST be a
 * FUNCTION-LEVEL local assigned AFTER the global store.
 *   - spelled inline (`p->owner->obj->elem->type_flags |= 8`) the chain takes
 *     the eax->ecx->edx rotation, where the original's first dereference is
 *     in place (`mov eax,[eax+0x18]`): 3 wrong at identical length.
 *   - as a BLOCK-SCOPE `ObjNode* n = p->owner;` the rotation is right but the
 *     `g_hit_type` store sinks below three of the four argument pushes: 3
 *     wrong the other way.
 *   - function-level declaration, store first, `n = p->owner;` second: exact.
 * Scope decides the SCHEDULE here, not just the frame slot.
 */

#ifdef LEGOLAND_PORTABLE
/* PORT-M3: the Icon +0x2c input slot is called with four arguments
 * (fpui.c CheckFocussedIcon, uimisc.c RestoreFreePlaySelections) and so
 * are g_icon_handler1/2; this body reads only the first two, which x86
 * cdecl tolerates and a wasm call_indirect does not. The matched body is
 * renamed for the portable build only and a twin of the slot's shape is
 * exported over it. VC6 compiles the #ifndef world unchanged. */
#define ChildrenBarInput ChildrenBarInput_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00475c50
char ChildrenBarInput(Icon* p, int ev)
{
    ObjNode* n;

    if (ev & 2) {
        g_hit_type = 0x100;
        n = p->owner;
        n->obj->elem->type_flags |= 8;
        MakeUpObjectList(0xd2, 3, 0x21, 0x154);
    }
    return 1;
}
#ifdef LEGOLAND_PORTABLE
#undef ChildrenBarInput
char ChildrenBarInput(Icon* p, int ev, int ll_dx, int ll_dy)
{
    (void)ll_dx;
    (void)ll_dy;
    return ChildrenBarInput_vc6_body(p, ev);
}
#endif

#ifdef LEGOLAND_PORTABLE
/* PORT-M3: the Icon +0x2c input slot is called with four arguments
 * (fpui.c CheckFocussedIcon, uimisc.c RestoreFreePlaySelections) and so
 * are g_icon_handler1/2; this body reads only the first two, which x86
 * cdecl tolerates and a wasm call_indirect does not. The matched body is
 * renamed for the portable build only and a twin of the slot's shape is
 * exported over it. VC6 compiles the #ifndef world unchanged. */
#define CloseChildrenBarInput CloseChildrenBarInput_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00475c90
char CloseChildrenBarInput(Icon* p, int ev)
{
    ObjNode* n;

    if (ev & 2) {
        g_hit_type = 0x100;
        n = p->owner;
        n->obj->elem->type_flags &= ~8;
        MakeUpObjectList(0xd2, 3, 0x21, 0x154);
    }
    return 1;
}
#ifdef LEGOLAND_PORTABLE
#undef CloseChildrenBarInput
char CloseChildrenBarInput(Icon* p, int ev, int ll_dx, int ll_dy)
{
    (void)ll_dx;
    (void)ll_dy;
    return CloseChildrenBarInput_vc6_body(p, ev);
}
#endif

/* =========================================================================
 *  Help expiry (iconui.c's ProcessInGameHelp asks both)
 * ========================================================================= */

/* One textual `return 0` inline and one `return 1` block in the tail: the
 * whole test is spelled as the NEGATIVE ("still live"), so every failing
 * short-circuit edge falls into the shared tail. */
// FUNCTION: LEGOLAND 0x0046cee0
int ObjectHelpExpired(void)
{
    if (GetGameTimer() - g_advisor_last < 0
        && (!g_object_help || !g_object_help->kind)
        && !g_hint_up)
        return 0;
    g_hint_up = 0;
    return 1;
}

// FUNCTION: LEGOLAND 0x0046cf20
int AdvisorHelpExpired(void)
{
    if (GetGameTimer() - g_advisor_start <= 0x7530
        && (!g_advisor_arg || !g_object_help || !g_object_help->kind))
        return 0;
    return 1;
}

/* =========================================================================
 *  Narration playback
 * ========================================================================= */

/* Restart the ducked narration buffer.  State 3 is "already playing" and 0 is
 * "nothing loaded", so both are no-ops. */
// FUNCTION: LEGOLAND 0x00498b00
int ResumeCurrentTrack(void)
{
    if (g_speech_state == 0 || g_speech_state == 3)
        return 0;
    RewindNarrationBuffer();
    g_speech_buffer->vtbl->Play(g_speech_buffer, 0, 0, 1);
    g_speech_state = 3;
    return 1;
}

/* =========================================================================
 *  The bloke age bucket (popup.c / rides.c / simcore.c all call it)
 * ========================================================================= */

/* The counter runs 1..4 over the four thresholds and the answer is
 * `group - 1`: the preheader's `mov ecx,1` with a trailing
 * `lea eax,[ecx-1]` is what proves the source counted from ONE (a 0-based
 * `for (i = 0; i < 4; i++) ... return i;` is two instructions shorter and
 * has no `lea` at all).  The bound must be `group < 5`, not `group <= 4`:
 * on the strength-reduced cursor those are `jl` against `&limits[4]` and
 * `jle` against `&limits[3]` -- the one instruction between exact and not. */
// FUNCTION: LEGOLAND 0x0044eb10
signed char GetBlokeAgeGroup(Bloke* b)
{
    int group;

    if (!g_visitor_tire)
        return 1;
    for (group = 1; group < 5; group++) {
        if (b->age < g_bloke_age_limits[group - 1])
            return (signed char)(group - 1);
    }
    return 4;
}

/* =========================================================================
 *  Icon geometry and the indicator renderer
 * ========================================================================= */

/* uimisc.c's IconHitTest fills the ClipRect from the icon and asks this. */
// FUNCTION: LEGOLAND 0x0046f300
int PointInIconRect(Pos* pt, ClipRect* r)
{
    if (pt->x >= r->left && pt->x <= r->right
        && pt->y >= r->top && pt->y <= r->bottom)
        return 1;
    return 0;
}

/* An indicator icon is drawn only on the blink's ON phase, and it publishes
 * itself as the hit owner so a click lands on the indicator, not the map. */
// FUNCTION: LEGOLAND 0x0046eaa0
int RenderIndicatorIcon(Icon* p)
{
    BlitCtx ctx;

    ctx.kind = 2;
    ctx.owner.p = p;
    ctx.owner.n = 0;
    if (GetBlink())
        PrintSprite(p->sprite, p->x, p->y, 0, &ctx);
    return 1;
}

/* =========================================================================
 *  Script steps and events
 * =========================================================================
 * Append the step's start (+0x0c) or completion (+0x10) event to the tail of
 * the pending event list and give up ownership.  The `s->ev = 0` tail is one
 * store, so VC6 copies it into both arms rather than jumping to it.
 */

// FUNCTION: LEGOLAND 0x0046c540
void EnqueueStepStartEvent(ScriptStep* s)
{
    ScriptEvent* p = g_script_event;

    if (p) {
        while (p->next)
            p = p->next;
        p->next = s->ev_start;
    } else {
        g_script_event = s->ev_start;
    }
    s->ev_start = 0;
}

// FUNCTION: LEGOLAND 0x0046c580
void EnqueueStepEndEvent(ScriptStep* s)
{
    ScriptEvent* p = g_script_event;

    if (p) {
        while (p->next)
            p = p->next;
        p->next = s->ev_end;
    } else {
        g_script_event = s->ev_end;
    }
    s->ev_end = 0;
}

/* Unlink a step from g_script_steps.  The head case is its own inline block;
 * the search's not-found edge is jump-threaded into the shared `return 0`,
 * which is why the found block re-tests a pointer that cannot be null. */
// FUNCTION: LEGOLAND 0x0046b5d0
int UnlinkScriptStep(ScriptStep* s)
{
    ScriptStep* p;

    if (g_script_steps == s) {
        g_script_steps = s->next;
        return 1;
    }
    for (p = g_script_steps; p; p = p->next) {
        if (p->next == s)
            break;
    }
    if (p) {
        p->next = s->next;
        return 1;
    }
    return 0;
}

/* The purge pass: unlink and free every pending event flagged 0x08.  Same
 * `p = prev` walk fpui3.c's UpdateHelpTick uses for its flag-6 purge. */
// FUNCTION: LEGOLAND 0x0046b290
void DropFlaggedEvents(void)
{
    ScriptEvent* p;
    ScriptEvent* prev;
    ScriptEvent* next;

    prev = 0;
    for (p = g_script_event; p; p = next) {
        next = p->next;
        if (p->flags & 8) {
            if (prev)
                prev->next = next;
            else
                g_script_event = next;
            FreeScriptEvent(p);
            p = prev;
        }
        prev = p;
    }
}

/* =========================================================================
 *  Report-screen narration
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00490a20
void PlayReportPageNarration(int page)
{
    char path[256];

    sprintf(path, g_fmt_wav, g_rep_page_narration, page + 1);
    PauseCurrentTrack();
    PlayNarrationFile(path);
    ResumeCurrentTrack();
    SetHelpFaceState5();
}

// FUNCTION: LEGOLAND 0x00490a60
void PlayReportHint(int index)
{
    char path[256];

    sprintf(path, g_fmt_wav, g_rep_hint_narration, index + 1);
    PauseCurrentTrack();
    PlayNarrationFile(path);
    ResumeCurrentTrack();
    SetHelpFaceState5();
}

/* =========================================================================
 *  Free play: can this class be taken, and give one back
 * ========================================================================= */

/* The lookup's result MUST be a named local: `g_freeplay_progress + f(...)`
 * spelled inline makes the call's return value the `add`'s destination
 * (`add eax,edx`, one byte short of the original); with `cost` named, the
 * global's fresh load is the first temporary and takes the destination
 * (`mov ecx,[g_freeplay_progress] / add ecx,eax`), which is what the
 * original has. */
// FUNCTION: LEGOLAND 0x0048aef0
int FreePlayItemAvailable(const char* name, LLElem* parent)
{
    int cost = FreePlayItemUpdate(name, 0);

#ifdef LEGOLAND_PORTABLE
    /* -ll-freeplay-all (ll_portable.h's LL_QOL): no 20000 budget; a child
     * class still waits for its parent to be ticked. */
    if (LL_QOL(LL_QOL_FREEPLAY_ALL))
        return !parent || parent == ElemID(g_build_menu_name)
               || (parent->type_flags & 4) != 0;
#endif
    if (g_freeplay_progress + cost <= 0x4e20
        && (!parent || parent == ElemID(g_build_menu_name)
            || (parent->type_flags & 4)))
        return 1;
    return 0;
}

// FUNCTION: LEGOLAND 0x0048afa0
void FreePlayItemRemove(const char* name)
{
    FPTableEntry* entry = 0;

    g_freeplay_progress -= FreePlayItemUpdate(name, &entry);
    if (entry)
        entry->chosen = 0;
    if (--g_freeplay_selected_count == 0)
        g_fp_accept_icon->flags |= 0x400;
}

/* =========================================================================
 *  The in-game right-button icon handler (sysstubs.c installs it)
 * =========================================================================
 * Right-click closes an open info pop-up and stops there; with nothing open
 * and no drag lock it falls through to the options handler, whose answer is
 * returned unchanged.  While the drag lock is set it instead pokes the
 * worker-on-mouse state and answers 1.
 */

// FUNCTION: LEGOLAND 0x00474830
char InGameSecondaryIcon(Icon* p, int ev, int dx, int dy)
{
    if ((ev & 2) && !CloseInfoPopUpIfOpen()) {
        if (g_drag_lock)
            CheckWorkerOnMouseStatus(1);
        else
            return OptionsIconInput(p, ev, dx, dy);
    }
    return 1;
}
