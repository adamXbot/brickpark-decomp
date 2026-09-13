/* LEGOLAND — movie player, help bar, report screen and level-end sequence.
 *
 * The callees uimisc.c (0x0046d***, 0x00468***, 0x00490***) and mapscreen2.c /
 * mapscreen4.c (the report screen) declare but do not define.  See
 * docs/lanes/fable-d-uimisc2.md for the mechanics recovered here.
 */

unsigned int strlen(const char*);
char* strcpy(char*, const char*);
char* strcat(char*, const char*);
#pragma intrinsic(strlen, strcpy, strcat)

void* memcpy(void*, const void*, unsigned int);
#pragma intrinsic(memcpy)

/* strchr is a real CRT call in the original (0x004a0050), never inlined. */
char* strchr(const char*, int);

/* ---- types (offsets are the load-bearing part; names are ours) ---------- */

typedef struct Pos { int x, y; } Pos;
typedef struct ClipRect { int left, top, right, bottom; } ClipRect;
typedef struct Sprite Sprite;

/* uimisc.c's script-event record.  +0x38 is a PRIORITY used to keep the
 * object-help queue sorted; uimisc.c's own view of the record leaves it inside
 * the pad. */
typedef struct ScriptEvent {
    struct ScriptEvent* next;  /* +0x00 */
    void*          elem;       /* +0x04 */
    char*          text;       /* +0x08 */
    int            kind;       /* +0x0c */
    unsigned char  flags;      /* +0x10; 0x20 = text is owned/heap-allocated */
    char           pad11[0x38 - 0x11];
    int            prio;       /* +0x38 */
    int            time;       /* +0x3c */
    int            f40;        /* +0x40 */
} ScriptEvent;

/* Whatever an icon's +0x30 back-pointer points at (a pop-up / list widget);
 * only its +0x22 flag byte is read here. */
typedef struct Widget {
    unsigned char pad00[0x22];
    unsigned char flags22;    /* +0x22  bit 3 grows the box down, bit 1 right */
} Widget;

/* iconui.c's 0x40-byte icon record, as uimisc.c spells it. */
typedef struct Icon {
    struct Icon*   next;      /* +0x00 */
    Sprite*        sprite;    /* +0x04 */
    void*          data;      /* +0x08 */
    short          x;         /* +0x0c */
    short          y;         /* +0x0e */
    short          w;         /* +0x10 */
    short          h;         /* +0x12 */
    unsigned short group;     /* +0x14 */
    short          f16;       /* +0x16 */
    int            f18;       /* +0x18 */
    char*          text;      /* +0x1c */
    short          f20;       /* +0x20 */
    short          f22;       /* +0x22  extra hit height for flag 0x200 */
    void*          f24;       /* +0x24 */
    int          (*render)(struct Icon*);                      /* +0x28 */
    char         (*input)(struct Icon*, int, short, short);    /* +0x2c */
    struct Widget* widget;    /* +0x30 */
    unsigned int   flags;     /* +0x34 */
    char*          help;      /* +0x38 */
    int            help_id;   /* +0x3c */
} Icon;

/* The global game record @ 0x004bcbf4 (legoland.h's g_map / lpConfig) seen
 * through the one field this file reads. */
typedef struct GameRec {
    unsigned char  pad00[0x1a];
    unsigned short max_blokes;   /* +0x1a  AllocBlokeCounters' per-class size */
    unsigned char  pad1c[0x40 - 0x1c];
    int            no_movies;    /* +0x40  non-zero suppresses every movie */
} GameRec;

/* profiles.c's profile records.  The on-disk/list record and the live
 * ("current") profile hold the same fields in DIFFERENT layouts, which is why
 * this function is a field-by-field copy and not a struct assignment. */
#pragma pack(push, 1)
typedef struct ProfileStats {
    int            a;            /* +0x00 */
    int            b;            /* +0x04 */
    int            c;            /* +0x08 */
    unsigned short d;            /* +0x0c */
    unsigned char  e;            /* +0x0e */
} ProfileStats;                  /* 15 bytes */

typedef struct Profile {
    char           name[0x20];   /* +0x00  player name */
    int            f20;          /* +0x20 */
    unsigned char  f24;          /* +0x24 */
    unsigned char  pad25[3];     /* +0x25 */
    int            f28;          /* +0x28 */
    int            f2c;          /* +0x2c */
    int            f30;          /* +0x30 */
    ProfileStats   stats;        /* +0x34 */
    char           block[200];   /* +0x43 */
    int            tail;         /* +0x10b */
    unsigned char  f10f;         /* +0x10f */
} Profile;                       /* 0x110 */

typedef struct CurProfile {
    char           name[0x20];   /* +0x00  0x0080ffa0 */
    int            f20;          /* +0x20  0x0080ffc0 */
    int            f24;          /* +0x24  0x0080ffc4 */
    int            f28;          /* +0x28  0x0080ffc8 */
    int            f2c;          /* +0x2c  0x0080ffcc */
    int            tail;         /* +0x30  0x0080ffd0 */
    ProfileStats   stats;        /* +0x34  0x0080ffd4 */
    unsigned char  profile_slot; /* +0x43  0x0080ffe3 */
    unsigned char  save_slot;    /* +0x44  0x0080ffe4 */
    unsigned char  f45;          /* +0x45  0x0080ffe5 */
    char           block[200];   /* +0x46  0x0080ffe6 */
} CurProfile;
#pragma pack(pop)

typedef struct ProfileNode {
    struct ProfileNode* next;    /* +0x000 */
    Profile             p;       /* +0x004 */
    int                 valid;   /* +0x114 */
    unsigned char       slot;    /* +0x118 */
} ProfileNode;                   /* 0x11c */

/* Win32 RECT, passed BY VALUE to the text printers (text.c's WinRect). */
typedef struct WinRect {
    long left;                /* +0x00 */
    long top;                 /* +0x04 */
    long right;               /* +0x08 */
    long bottom;              /* +0x0c */
} WinRect;

/* rin.c's mouse-hit record {type, object} (bigscreens.c's HitInfo). */
typedef struct HitInfo {
    int   type;    /* +0x00  2 = an icon is under the mouse */
    void* obj;     /* +0x04 */
} HitInfo;

/* ---- globals ------------------------------------------------------------ */

extern Sprite* g_backdrop;             /* 0x00810148 full-screen background */
extern Sprite* g_rep_next;             /* 0x0081c02c NextPage.lls */
extern Sprite* g_rep_next_lit;         /* 0x0081c034 NextPageLit.lls */
extern Sprite* g_rep_prev;             /* 0x0081c080 PreviousPage.lls */
extern Sprite* g_rep_prev_lit;         /* 0x0081c084 PreviousPageLit.lls */
extern Sprite* g_rep_hint1;            /* 0x007caf80 Rep_Hint1.lls */
extern Sprite* g_rep_hint2;            /* 0x007cb1c4 Rep_Hint2.lls */
extern Icon*   g_rep_next_icon;        /* 0x007cb2e4 */
extern Icon*   g_rep_hint_icon;        /* 0x007cb1c0 */
extern Pos     g_gfx_point;            /* 0x00813a44 mouse point */
extern Icon*   g_rep_prev_icon;        /* 0x007cb2e0 */
extern int     g_rep_page;             /* 0x004bf670 1-based first line */
extern void*   g_snd_click;            /* 0x004b92c0 */
extern ScriptEvent* g_object_help;     /* 0x00668724 */
extern Icon*   g_focussed_icon;        /* 0x006687d0 */
extern HitInfo g_hit_info;             /* 0x004bdd00 */
/* Two signed-short hit-box extensions.  Nothing in .text references either
 * address, and both sit past the raw .data, so they are zero unless something
 * writes them through a base pointer — recorded as found. */
extern short   g_icon_hit_dy;          /* 0x00668840 */
extern short   g_icon_hit_dx;          /* 0x0066884c */
extern int     g_icons2_mode;          /* 0x00668e38 */
extern int     g_game_mode;            /* 0x008119b4 */
extern int     g_cur_screen;           /* 0x0080ff84 */
extern int     g_screen_mode;          /* 0x0080ff88 sub-mode of the screen */
extern char    g_level_end_movie[];    /* 0x008100c0 */
extern int     g_level_end_has_movie;  /* 0x00832bac */
extern int     g_help_target;          /* 0x004b9f8c polymorphic id/char* */
extern int     g_help_changed;         /* 0x004b9f88 */
extern unsigned long g_help_hover_start; /* 0x007fe920 */
extern int     g_help_face_state;      /* 0x006687a4 */
extern int     g_help_requested;       /* 0x006687a8 */
extern int     g_help_force;           /* 0x006687ac */
extern int     g_6687b0;               /* 0x006687b0 hold-off frame counter */
extern int     g_help_deferred;        /* 0x006687b4 */
extern const char kFmtTextWav[];       /* 0x004ba858 "Text%04d.wav" */
extern const char kFmtWav[];           /* 0x004ba868 "%s.wav" */
extern const char kFmtZWav[];          /* 0x004ba870 "%sz.wav" */
extern int     g_movie_shown;          /* 0x00668fb0 */
extern int     g_mouse_btn_a;          /* 0x00813ad4 */
extern char    g_res_path[];           /* 0x00813b04 alternate volume prefix */
extern const char kFmtOpenMovie[];     /* 0x004bb56c "Attempting to open Movie %s" */
extern const char kFmtMovieOpened[];   /* 0x004bb554 "Movie openned OK (%s)" */
extern const char kMsgAttemptPlay[];   /* 0x004bb538 "Attempting to play movie.." */
extern const char kMsgStartingMovie[]; /* 0x004bb528 "Starting Movie\n" */
extern const char kMsgStoppingMovie[]; /* 0x004bb518 "Stopping Movie\n" */
extern const char kMsgStoppingMovie2[];/* 0x004bb508 "Stopping movie" */
extern GameRec* g_game;                /* 0x004bcbf4 */
extern void*   g_sel_def;              /* 0x00667c58 class under the cursor */
extern int     g_castle_built;         /* 0x0079a8d0 */
extern void*   g_freeplay_db;          /* 0x00667c4c the loaded level database */
extern int     g_pending_state;        /* 0x00832ba0 */
extern char    g_script_text1[];       /* 0x0066861c 0x80 bytes */
extern const char kFreePlayTestTxt[];  /* 0x004beb4c "FreePlayTest.txt" */
extern char    g_report_movie[256];    /* 0x00798778 */
extern CurProfile  g_cur_profile;      /* 0x0080ffa0 */
extern ProfileNode* g_profile_list;    /* 0x00798890 */

/* ---- callees ------------------------------------------------------------ */

extern void SetIconSprite(Icon*, Sprite*);                     /* 0x0046d680 */
#ifndef LEGOLAND_PORTABLE
extern void PlayInstanceOfSample(void*, int, int, void*);      /* 0x00496d20 */
#else
extern int PlayInstanceOfSample(void*, int, int, void*);      /* 0x00496d20 */
#endif
extern void UpdateReportPageIcons(void);                       /* 0x00490aa0 */
#ifndef LEGOLAND_PORTABLE
extern void KillSprite(Sprite*);                               /* 0x00497bd0 */
#else
extern int KillSprite(Sprite*);                               /* 0x00497bd0 */
#endif
extern void RemoveIconGroup(int group);                        /* 0x0046d520 */
extern void GetIconBounds(Icon*, ClipRect*);                   /* 0x0046de50 */
/* 0x004907a0: loads the level's help/report text for `key`, non-zero when
 * there is any (screens3.c names it LoadHelpTextFor). */
extern int  LoadHelpTextFor(const char* key);                  /* 0x004907a0 */
extern int  GetBlink(void);                                    /* 0x00499480 */
extern __declspec(dllimport) unsigned long __stdcall GetTickCount(void); /* IAT 0x004ab1f8 */
/* The media module's duck / play / restore trio (fpui5.c's names).  NOTE:
 * mapscreen.c and mapscreen2.c call 0x00498920 ResetFrontEnd; it is the same
 * function and the name divergence is left alone. */
#ifndef LEGOLAND_PORTABLE
extern void PauseCurrentTrack(void);                           /* 0x00498920 */
#else
extern int PauseCurrentTrack(void);                           /* 0x00498920 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void PlayNarrationFile(const char* path);               /* 0x00498630 */
#else
extern int PlayNarrationFile(const char* path);               /* 0x00498630 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void ResumeCurrentTrack(void);                          /* 0x00498b00 */
#else
extern int ResumeCurrentTrack(void);                          /* 0x00498b00 */
#endif
/* 0x0047f870 / 0x0047f850 are both a bare `ret` in the shipped build (the
 * debug logger compiled out); the argument pushes are still emitted. */
extern void DebugPrintf(const char* fmt, ...);                 /* 0x0047f870 */
extern void DebugFlush(void);                                  /* 0x0047f850 */
extern void DBPrintf(const char* fmt, ...);                    /* 0x00453a20 */
/* The movie player itself (names ours; nothing else in the tree calls them). */
extern void* OpenMovie(const char* path);                      /* 0x00476460 */
extern int  RunMovie(void* mv, WinRect* dst, int flags);       /* 0x004766f0 */
extern void CloseMovie(void* mv);                              /* 0x00476630 */
/* 0x00492830 walks the sample list pausing every entry; mapscreen2.c calls it
 * InitOptionSamples, which is a misnomer — the divergence is left alone. */
extern void PauseAllSamples(void);                             /* 0x00492830 */
extern void ResumePausedSamples(void);                         /* 0x00492850 */
extern void StopMusic(void);                                   /* 0x00492d80 */
extern void RestartMusic(void);                                /* 0x00492da0 */
extern void PushRenderingStatusAndUnlockVideoSurface(void);    /* 0x00464080 */
extern void PopRenderingStatus(void);                          /* 0x004641f0 */
extern int  ProcessSystemEvents(void);                         /* 0x00480050 */
extern void ReadGameButtons(void);                             /* 0x00452460 */
extern void HeapFree_w(void*);                                 /* 0x0049e4d0 */
extern void* HeapAlloc_w(unsigned int);                        /* 0x0049e4ff */
extern int  sprintf(char*, const char*, ...);                  /* 0x0049e573 (CRT) */
extern int  PauseGameTimer(void);                              /* 0x00499380 */
extern void ResetGameClock(void);                              /* 0x00499410 */
extern void ResetSaveTimer(void);                              /* 0x0047f810 */
extern void ResetMapAI(void);                                  /* 0x00462dd0 */
extern void* LoadLevelDatabase(const char* name);              /* 0x0047afb0 */
extern void sub_457870(int);                                   /* 0x00457870 */
extern void sub_48ab60(void);                                  /* 0x0048ab60 */
extern void AllocBlokeCounters(int count);                     /* 0x00480e10 */
extern void EnterParkPlayMode(void);                                  /* 0x00458940 */
extern void sub_489ee0(void);                                  /* 0x00489ee0 */
extern void UpdateMenu(void);                                  /* 0x004758c0 */
extern void ClearWaitSprite(void);                             /* 0x004663c0 */
extern void ShowInfoPanel(int kind);                           /* 0x00490600 */
#ifndef LEGOLAND_PORTABLE
extern void SetInfoPanelText(const char* a, const char* b);    /* 0x004911c0 */
#else
extern int SetInfoPanelText(const char* a, const char* b);    /* 0x004911c0 */
#endif
extern void SetMapReady(int);                                   /* 0x00458bb0 */
extern void ThawGameClock(void);                               /* 0x004993c0 */
#ifndef LEGOLAND_PORTABLE
extern void UpdateSoundVols(void);                             /* 0x00495a90 */
#else
extern int UpdateSoundVols(void);                             /* 0x00495a90 */
#endif
extern void NewPrintCent(const char* text, int font, WinRect rc, char white); /* 0x00491d60 */
/* 0x00490fa0: screens2.c calls this one PrintCursor (it draws the blinking
 * name-entry cursor); it is NewPrintCent's twin and is the SMALL-font report
 * line printer here. */
extern void PrintCursor(const char* text, int font, WinRect rc, char white);  /* 0x00490fa0 */

/* =========================================================================
 *  Report screen — page buttons
 * ========================================================================= */

/* The mirror of uimisc.c's ReportNextPageInput (0x00490b20).  The Next handler
 * forwards to ReportAcceptInput when its icon is greyed (flag 0x400); the
 * Previous handler has no such arm, so its `ev` is only ever tested and VC6
 * reads it as a byte. */
#ifdef LEGOLAND_PORTABLE
/* PORT-M3: the Icon +0x2c input slot is called with four arguments
 * (fpui.c CheckFocussedIcon, uimisc.c RestoreFreePlaySelections) and so
 * are g_icon_handler1/2; this body reads only the first two, which x86
 * cdecl tolerates and a wasm call_indirect does not. The matched body is
 * renamed for the portable build only and a twin of the slot's shape is
 * exported over it. VC6 compiles the #ifndef world unchanged. */
#define ReportPrevPageInput ReportPrevPageInput_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00490b90
char ReportPrevPageInput(Icon* p, int ev)
{
    SetIconSprite(g_rep_prev_icon, g_rep_prev_lit);
    if (ev & 2) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_rep_page -= 14;
        UpdateReportPageIcons();
    }
    return 1;
}
#ifdef LEGOLAND_PORTABLE
#undef ReportPrevPageInput
char ReportPrevPageInput(Icon* p, int ev, int ll_dx, int ll_dy)
{
    (void)ll_dx;
    (void)ll_dy;
    return ReportPrevPageInput_vc6_body(p, ev);
}
#endif

/* =========================================================================
 *  Script events
 * ========================================================================= */

/* Insert e into the object-help queue, kept in DESCENDING priority order.
 *
 * ORIGINAL BUG, reproduced: the head case is written as a plain assignment,
 * so inserting an event whose priority is >= the current head's DISCARDS the
 * whole existing queue instead of linking in front of it (the `else` arm is
 * shared with the empty-list case, where it is correct).
 */
// FUNCTION: LEGOLAND 0x00468b00
void EnqueueObjectHelp(ScriptEvent* e)
{
    ScriptEvent* prev = 0;
    ScriptEvent* cur = g_object_help;
    while (cur) {
        if (cur->prio <= e->prio) break;
        prev = cur;
        cur = cur->next;
    }
    if (prev) {
        e->next = prev->next;
        prev->next = e;
    } else {
#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
        e->next = g_object_help;  /* QUIRKS.md Q9: link in FRONT of the head instead of discarding the queue */
        g_object_help = e;
#else
        g_object_help = e;
        e->next = 0;
#endif
    }
}

/* =========================================================================
 *  Icons
 * ========================================================================= */

/* Destroy one detached icon record.  Icon flag 0x80 means the +0x30 widget
 * back-pointer is a heap block this icon owns.  The record is freed BEFORE the
 * two "is it still referenced?" checks, so both compare against a dangling
 * pointer — harmless, but it is the original's order and is kept.
 * uimisc.c's UnlinkIcon clears g_focussed_icon on the icon's SUCCESSOR; this
 * is where the removed record itself is cleared. */
// FUNCTION: LEGOLAND 0x0046d3c0
void FreeIcon(Icon* p)
{
    if (p) {
        if (p->sprite) {
            KillSprite(p->sprite);
            p->sprite = 0;
        }
        if ((p->flags & 0x80) && p->widget)
            HeapFree_w(p->widget);
        HeapFree_w(p);
        if (g_focussed_icon == p)
            g_focussed_icon = 0;
        if (g_hit_info.type == 2 && g_hit_info.obj == p) {
            g_hit_info.obj = 0;
            g_hit_info.type = 0x100;
        }
    }
}

/* =========================================================================
 *  Free play
 * ========================================================================= */

/* Bring up the free-play park.  The level database is always the fixed file
 * "FreePlayTest.txt", copied into a 0x34-byte stack buffer through sprintf
 * with no conversions at all — the original never varies the name.
 *
 * Every one of the nine cdecl arguments in this body is cleaned by the single
 * `add esp,0x58` in the epilogue, which is why the stack buffer is addressed
 * as [esp] at the sprintf and as [esp+8] at the loader. */
// FUNCTION: LEGOLAND 0x0048abb0
void StartFreePlayPark(void)
{
    char name[0x34];

    g_sel_def = 0;
    sprintf(name, kFreePlayTestTxt);
    PauseGameTimer();
    ResetGameClock();
    ResetSaveTimer();
    g_castle_built = 0;
    ResetMapAI();
    g_freeplay_db = LoadLevelDatabase(name);
    sub_457870(0);
    sub_48ab60();
    AllocBlokeCounters(g_game->max_blokes);
    EnterParkPlayMode();
    g_pending_state = 0;
    sub_489ee0();
    UpdateMenu();
    ClearWaitSprite();
    ShowInfoPanel(1);
    SetInfoPanelText(g_script_text1, 0);
    SetMapReady(1);
    ThawGameClock();
    UpdateSoundVols();
}

/* =========================================================================
 *  Report screen — the movie played when the report is accepted
 * ========================================================================= */

/* Copy the .AVI name into the 256-byte g_report_movie buffer.
 * The over-long path is TRUNCATED, not rejected: 0x100 bytes are copied and
 * byte 255 forced to NUL.  The short path is strcpy'd and then NUL-terminated
 * a second time at strlen(name) — redundant, and it costs a third inline
 * strlen; kept because it is what the original does. */
/* The 0x100-byte copy is a CALL to the CRT memcpy in the original, not the
 * inlined rep movsd every other copy in this file gets, so the intrinsic is
 * switched off across this one body. */
#pragma function(memcpy)
// FUNCTION: LEGOLAND 0x00490610
void SetReportMovie(const char* name)
{
    if (strlen(name) > 0xff) {
        memcpy(g_report_movie, name, 0x100);
        g_report_movie[0xff] = 0;
    } else {
        strcpy(g_report_movie, name);
        g_report_movie[strlen(name)] = 0;
    }
}

#pragma intrinsic(memcpy)

/* =========================================================================
 *  Report screen — one line of the report text
 * ========================================================================= */

/* Centre one report line in the box (x, y)-(x + 0x1cc, y + h).  `big` picks
 * font 3 through NewPrintCent; otherwise font 2 through its twin at
 * 0x00490fa0.  The box is always 0x1cc (460) pixels wide.
 *
 * The rect's four field assignments must be written top/bottom/left/right:
 * a by-value WinRect's values take the eax->ecx->edx->esi ring in SOURCE
 * assignment order, and the natural left/top/right/bottom puts all four one
 * ring position off (13 of 43 at identical byte length).  Same lever as
 * mapscreen2.c's PrintScreenMode8. */
// FUNCTION: LEGOLAND 0x00491080
void PrintReportLine(char* text, int x, int y, int h, int big)
{
    WinRect rc;

    if (text) {
        rc.top = y;
        rc.bottom = y + h;
        rc.left = x;
        rc.right = x + 0x1cc;
        if (big) NewPrintCent(text, 3, rc, 0);
        else PrintCursor(text, 2, rc, 0);
    }
}

/* Attach text to a script event.  With `copy` the text is duplicated onto the
 * heap and flag 0x20 records that the event owns it; without it the caller's
 * pointer is stored and 0x20 cleared.  uimisc.c's ShowScriptStepText is the
 * caller that hands over ownership by clearing the step's own pointer. */
// FUNCTION: LEGOLAND 0x00468b40
void SetScriptEventText(ScriptEvent* e, const char* text, int copy)
{
    if (copy) {
        char* buf = (char*)HeapAlloc_w(strlen(text) + 1);
        e->text = buf;
        strcpy(buf, text);
        e->flags |= 0x20;
    } else {
        e->text = (char*)text;
        e->flags &= ~0x20;
    }
}

/* =========================================================================
 *  Profiles
 * ========================================================================= */

/* Re-read the live profile from its node in the profile list, undoing any
 * unsaved edits.  uimisc.c's KillCurrentScreen calls this when the profile
 * screen (front-end screen 0) is torn down.  The current save slot and the
 * +0x45 flag are reset to 0 rather than restored — the list node's own
 * Profile.f24 is never read back. */
// FUNCTION: LEGOLAND 0x0048d230
void RestoreCurrentProfileFromList(void)
{
    ProfileNode* n = g_profile_list;

    while (n) {
        if (n->slot == g_cur_profile.profile_slot) {
            strcpy(g_cur_profile.name, n->p.name);
            g_cur_profile.f20 = n->p.f20;
            g_cur_profile.save_slot = 0;
            g_cur_profile.f24 = n->p.f28;
            g_cur_profile.f28 = n->p.f2c;
            g_cur_profile.f2c = n->p.f30;
            g_cur_profile.f45 = 0;
            g_cur_profile.stats = n->p.stats;
            memcpy(g_cur_profile.block, n->p.block, sizeof(g_cur_profile.block));
            g_cur_profile.tail = n->p.tail;
            return;
        }
        n = n->next;
    }
}

/* Tear down front-end screen 7.  The seven sprite slots are written out one
 * block at a time (no loop or table); with seven literal zeros VC6 hoists the
 * zero into esi, which is why every guard is `cmp eax,esi` and not
 * `test eax,eax`. */
// FUNCTION: LEGOLAND 0x004908b0
void KillReportScreenSprites(void)
{
    if (g_backdrop) { KillSprite(g_backdrop); g_backdrop = 0; }
    if (g_rep_next) { KillSprite(g_rep_next); g_rep_next = 0; }
    if (g_rep_next_lit) { KillSprite(g_rep_next_lit); g_rep_next_lit = 0; }
    if (g_rep_prev) { KillSprite(g_rep_prev); g_rep_prev = 0; }
    if (g_rep_prev_lit) { KillSprite(g_rep_prev_lit); g_rep_prev_lit = 0; }
    if (g_rep_hint1) { KillSprite(g_rep_hint1); g_rep_hint1 = 0; }
    if (g_rep_hint2) { KillSprite(g_rep_hint2); g_rep_hint2 = 0; }
    RemoveIconGroup(7);
}

/* The icon's plain bounds (GetIconBounds) grown into the rectangle the mouse
 * is actually tested against:
 *   flag 0x20   inflate by 3 on all four sides;
 *   flag 0x40   raise the top by 0x16 (the pop-up title bar) and, if the icon
 *               carries a widget, grow down / right by the widget's own two
 *               extension globals;
 *   flag 0x200  make sure the box spans [y + f22, y + f22 + 0x28] vertically.
 * uimisc.c's IconHitTest is the caller. */
// FUNCTION: LEGOLAND 0x0046de90
void GetIconHitBounds(Icon* p, ClipRect* r)
{
    GetIconBounds(p, r);
    if (p->flags & 0x20) {
        r->left -= 3;
        r->right += 3;
        r->top -= 3;
        r->bottom += 3;
    }
    if (p->flags & 0x40) {
        Widget* w;
        r->top -= 0x16;
        w = p->widget;
        if (w) {
            if (w->flags22 & 8) r->bottom += g_icon_hit_dy;
            if (w->flags22 & 2) r->right += g_icon_hit_dx;
        }
    }
    if (p->flags & 0x200) {
        int base = p->f22 + p->y;
        int lim = base + 0x28;
        if (r->top > base) r->top = base;
        if (r->bottom < lim) r->bottom = lim;
    }
}

/* =========================================================================
 *  Level end
 * ========================================================================= */

/* uimisc.c's EndLevel hands one of two ";"-separated strings to this.  The
 * part BEFORE the semicolon is the help-text key; the part after it, if any,
 * is the .AVI 0x00458dc0 plays on the way out (through g_level_end_movie /
 * g_level_end_has_movie).  The separator is written back into the caller's
 * string after the key has been copied out, so the argument survives intact.
 *
 * ORIGINAL BUG, reproduced: when the string has NO semicolon, `key` is never
 * written and the uninitialised 0x80-byte stack buffer is passed to
 * LoadHelpTextFor.  Both shipped strings contain one, so it never fires. */
// FUNCTION: LEGOLAND 0x00459710
void RunLevelEndSequence(char* text)
{
    char key[0x80];
    char* p = strchr(text, ';');

    if (p) {
        *p = 0;
        strcpy(key, text);
        *p++ = ';';
        if (strlen(p)) {
            strcpy(g_level_end_movie, p);
            g_level_end_has_movie = 1;
        }
    }
    ShowInfoPanel(0);
    if (LoadHelpTextFor(key)) {
        g_icons2_mode = 1;
        g_game_mode = 2;
        g_cur_screen = -1;
        g_screen_mode = 7;
    }
}

/* Per-frame sprite refresh for the report screen's three page/hint buttons:
 * while the mouse is NOT over a button it is drawn in its resting sprite, and
 * the Next button additionally BLINKS between its plain and lit sprites.  The
 * hit test is written out inline at all three sites (the right/bottom edges
 * are only computed when the earlier terms pass), and each icon global is read
 * directly at every use rather than through a local — GetBlink() clobbers eax,
 * which is why the Next icon is reloaded inside the blink arms. */
// FUNCTION: LEGOLAND 0x00490ea0
void BlinkReportPageIcons(void)
{
    if (g_gfx_point.x < g_rep_next_icon->x ||
        g_gfx_point.x > g_rep_next_icon->w + g_rep_next_icon->x ||
        g_gfx_point.y < g_rep_next_icon->y ||
        g_gfx_point.y > g_rep_next_icon->h + g_rep_next_icon->y) {
        if (GetBlink()) SetIconSprite(g_rep_next_icon, g_rep_next);
        else SetIconSprite(g_rep_next_icon, g_rep_next_lit);
    }
    if (g_gfx_point.x < g_rep_prev_icon->x ||
        g_gfx_point.x > g_rep_prev_icon->w + g_rep_prev_icon->x ||
        g_gfx_point.y < g_rep_prev_icon->y ||
        g_gfx_point.y > g_rep_prev_icon->h + g_rep_prev_icon->y)
        SetIconSprite(g_rep_prev_icon, g_rep_prev);
    if (g_gfx_point.x < g_rep_hint_icon->x ||
        g_gfx_point.x > g_rep_hint_icon->w + g_rep_hint_icon->x ||
        g_gfx_point.y < g_rep_hint_icon->y ||
        g_gfx_point.y > g_rep_hint_icon->h + g_rep_hint_icon->y)
        SetIconSprite(g_rep_hint_icon, g_rep_hint1);
}

/* =========================================================================
 *  Help bar / advisor narration
 * ========================================================================= */

/* Per-frame help update.  g_help_target is POLYMORPHIC — face state 0 formats
 * it as a string id ("Text%04d.wav"), states 1 and 2 as a char* base name
 * ("%s.wav" / "%sz.wav") — which is why uimisc.c's ShowIdHelp and
 * ShowObjectHelp both store into the same int and set a different face state.
 * States >= 3 (the advisor is already talking) suppress the whole update.
 * The narration only starts once the hover has lasted 0x1f4 ms, unless
 * g_help_force short-circuits the wait. */
// FUNCTION: LEGOLAND 0x0046d110
void UpdateHelpBar(void)
{
    char path[0x100];

    if (!g_help_requested) g_help_target = -1;
    if (!g_help_force && g_6687b0 != 0) {
        g_6687b0--;
        return;
    }
    if (g_help_face_state >= 3) return;
    if (!g_help_requested) return;
    g_help_requested = 0;
    if (!g_help_changed) return;
    if (GetTickCount() - g_help_hover_start < 0x1f4 && !g_help_force) {
        g_help_deferred = 1;
        return;
    }
    g_help_deferred = 0;
    g_help_changed = 0;
    g_help_force = 0;
    g_help_hover_start = GetTickCount();
    PauseCurrentTrack();
    if (g_help_target != -1) {
        switch (g_help_face_state) {
        case 0: sprintf(path, kFmtTextWav, g_help_target); break;
        case 1: sprintf(path, kFmtWav, g_help_target); break;
        case 2: sprintf(path, kFmtZWav, g_help_target); break;
        }
        PlayNarrationFile(path);
        ResumeCurrentTrack();
    }
}

/* =========================================================================
 *  The movie player  (runtime spec)
 * ========================================================================= */

/* Play one full-screen .AVI.
 *
 * MECHANICS.  The destination is always the fixed 320x240 rectangle
 * {0, 0, 0x140, 0xf0} — the movie is never scaled to the window.  The file is
 * looked for in "FMV\<name>" first and, failing that, under g_res_path
 * (the alternate-volume prefix data2.c/sysmisc2.c use for the CD), so a movie
 * plays off the install directory or off the disc.  Around the playback the
 * game ducks its whole audio stack (the CD/streaming track, every playing
 * sample, and the music thread), unlocks the video surface, runs the movie,
 * then restores all three; afterwards it spins on ProcessSystemEvents /
 * ReadGameButtons until all three mouse buttons are released, so the click
 * that skipped the movie is not delivered to the screen underneath.
 *
 * Returns 0 when the movie was suppressed (g_movie_shown already set with
 * force == 0, or the game record's +0x40 "no movies" flag), 1 when the file
 * could not be opened at either prefix, and otherwise whatever RunMovie
 * returns.  uimisc.c declares this `void` — a caller-side divergence, left
 * alone. */
// FUNCTION: LEGOLAND 0x004771f0
int PlayMovie(const char* name, int flags, int force)
{
    WinRect rc = { 0, 0, 0x140, 0xf0 };
    char path[0x80];
    void* mv;
    int played;

    if (force) g_movie_shown = 0;
    else if (g_movie_shown) return 0;
    if (!g_game->no_movies) {
        PauseCurrentTrack();
        g_6687b0 = 4;
        strcpy(path, "FMV\\");   /* the literal, not a named array: VC6's inline
                                   strcpy knows its length and copies it as
                                   one dword plus one byte */
        strcat(path, name);
        DebugPrintf(kFmtOpenMovie, path);
        mv = OpenMovie(path);
        if (!mv) {
            strcpy(path, g_res_path);
            strcat(path, name);
            DebugPrintf(kFmtOpenMovie, path);
            mv = OpenMovie(path);
        }
        DebugFlush();
        if (mv) {
            DebugPrintf(kFmtMovieOpened, path);
            DebugFlush();
            PauseAllSamples();
            StopMusic();
            PushRenderingStatusAndUnlockVideoSurface();
            DebugPrintf(kMsgAttemptPlay);
            DebugFlush();
            DBPrintf(kMsgStartingMovie);
            played = RunMovie(mv, &rc, flags);
            DBPrintf(kMsgStoppingMovie);
            DebugPrintf(kMsgStoppingMovie2);
            DebugFlush();
            CloseMovie(mv);
            PopRenderingStatus();
            do {
                ProcessSystemEvents();
                ReadGameButtons();
            } while (g_mouse_btn_a & 7);
            ResumePausedSamples();
            RestartMusic();
            return played;
        }
        return 1;
    }
    return 0;
}
