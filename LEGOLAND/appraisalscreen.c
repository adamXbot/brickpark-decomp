/* LEGOLAND -- scope LL16: the park-appraisal report screen.
 *
 * One function, RunAppraisalScreen (0x004453a0, 8,085 instructions, 34,662
 * bytes, a 0x23d4-byte frame taken through __chkstk).  goalstate.c's
 * AppraisalDueTick calls it; every helper it calls lives in appraisal.c
 * (scope Y) and the types below are copied from there.
 *
 * The screen builds a list of report lines in a stack array, paginates it,
 * then renders and runs its own input loop.  Each graded statistic counts
 * something in the park, compares it with a goal from the table at
 * 0x0066600c and picks one of five randomly chosen tick/cross sprites for
 * its line; a two-bit field of the control word at 0x00665ff8 selects which
 * of three phrasings the line uses.
 *
 * VC6 SP3 /O2 /Gy /Gd.
 */

typedef struct Sprite Sprite;
typedef struct Icon   Icon;

typedef char (*IconInputFn)(Icon* icon, int event, int x, int y);
struct Icon {
    char        pad00[0x0c];
    short       x;                /* +0x0c */
    short       y;                /* +0x0e */
    short       w;                /* +0x10 */
    short       h;                /* +0x12 */
    char        pad14[0x2c - 0x14];
    IconInputFn handler;          /* +0x2c */
    char        pad30[0x34 - 0x30];
    unsigned int flags;           /* +0x34 */
    const char*  text;            /* +0x38 */
    int          text_id;         /* +0x3c */
};

typedef struct Pos { int x, y; } Pos;
typedef struct AppraisalBox { int left, top, right, bottom; } AppraisalBox;

/* One line of the report: 0x4c bytes, the array lives at [esp+0x94]. */
typedef struct RepLine {
    int   page;      /* +0x00  the page this line landed on */
    int   indent;    /* +0x04  the running x indent, re-read on a page restart */
    int   ok;        /* +0x08  1 tick, 0 cross, -1/-2 bullet */
    int   step;      /* +0x0c  rand() % 5: which of the five mark sprites */
    char* text;      /* +0x10 */
    int   colour;    /* +0x14 */
    int   bar;       /* +0x18  1: this line draws a bar */
    int   value;     /* +0x1c */
    int   mark;      /* +0x20  the goal, where the bar's marker goes */
    int   range;     /* +0x24  the bar's full scale */
    int   nids;      /* +0x28  how many extra string ids follow */
    int   ids[8];    /* +0x2c..+0x48 */
} RepLine;

extern int     g_report_pages;                              /* 0x006660a0 */
extern int     g_report_page;                               /* 0x006660a4 */
extern int     g_report_page_turned;                        /* 0x0081c07c */
/* Which statistics this appraisal grades, two bits of phrasing each. */
extern int     g_appraisal_flags;                           /* 0x00665ff8 */
/* The goal table: value/scale pairs and single thresholds. */
extern int     g_goal[];                                    /* 0x0066600c */

extern int   ScriptRunning(void);                           /* 0x0046b280 */
extern void  PushRenderingStatusAndUnlockVideoSurface(void);/* 0x00464080 */
extern void  ReadGameButtons(void);                         /* 0x00452460 */
extern int   rand(void);                                    /* 0x0049e4b2 (CRT) */
extern int   sprintf(char* buf, const char* fmt, ...);      /* 0x0049e573 (CRT) */
extern char* GetString(int id);                             /* 0x00498f50 */
extern void  PlayNarrationFile(const char* name);           /* 0x00498630 */
extern void  CountAttractions(int* num, int* variety);      /* 0x00444bf0 */
extern void  CountScenery(int* num, int* variety);          /* 0x00444c70 */
extern void  CountFood(int* num, int* variety);             /* 0x00444cd0 */
extern void  CountShops(int* num, int* variety);            /* 0x00444d20 */
extern void  CountVisitors(int* v, int* mood, int* ages);   /* 0x00444d70 */
extern int   PercentObjectsLinked(void);                    /* 0x00444df0 */
extern int   CountDrivingSchools(void);                     /* 0x004442c0 */
extern int   CountBoatingSchools(void);                     /* 0x004442f0 */
extern int   CountCastles(void);                            /* 0x00444320 */
extern int   CountLogFlumes(void);                          /* 0x00444350 */
extern int   CountJungleCruises(void);                      /* 0x00444380 */
extern int   MapCellCount(void);                            /* 0x004636c0 */
extern void  BlitAppraisalSprite(int x, int y, int kind, int step);           /* 0x00444b70 */
extern void  DrawAppraisalBar(AppraisalBox box, int value, int range, int mark); /* 0x00444a70 */
extern void  LoadAppraisalTickSprites(void);                /* 0x004449b0 */
/* The original pushes one argument here; appraisal.c declares this helper
 * void(void).  The push and its matching `add esp,4` are in the original, so
 * this file has to declare the parameter. */
extern void  LoadAppraisalScreenSprites(int page);          /* 0x00445190 */
extern void  UnlightAppraisalPageButtons(void);             /* 0x00445100 */
extern void  FreeAppraisalScreenSprites(void);              /* 0x00445000 */
extern int   PrintSprite(Sprite* s, int x, int y, int mode, void* ctx);       /* 0x004853a0 */
extern void  PushRenderingStatusAndLockVideoSurface(void);  /* 0x00463fc0 */
extern void  PopRenderingStatus(void);                      /* 0x004641f0 */
extern void  NewPrintColoured(const char* text, int font, AppraisalBox rc, unsigned long colour); /* 0x00454d80 */
extern void  NewPrintCent(const char* text, int font, AppraisalBox rc, char white); /* 0x00491d60 */
extern void  RenderIcons2(unsigned short g1, unsigned short g2, unsigned short g3); /* 0x0046f010 */
extern void  ResetHitInfo(void);                            /* 0x00485ef0 */
extern void  SetPointer(int shape);                         /* 0x00463850 */
extern void  ProcessFrontEndHelp(void);                     /* 0x0046d080 */
extern void  UpdateFocussedIconPtr(void);                   /* 0x004700a0 */
extern char  CheckFocussedIcon(void);                       /* 0x0046f4c0 */
extern int   RenderingComplete(void);                       /* 0x00466500 */
extern void  UpdateHelpBar(void);                           /* 0x0046d110 */
extern int   IsNarrationPlaying(void);                      /* 0x00498cf0 */
extern void  PauseCurrentTrack(void);                       /* 0x00498920 */
extern void  ResumeCurrentTrack(void);                      /* 0x00498b00 */
extern void  SetInGameIconHandlers(void);                   /* 0x00474880 */
extern void  sub_498b40(void);                              /* 0x00498b40 */

extern Sprite* g_backdrop;                                  /* 0x00810148 */
extern int     g_report_open;                               /* 0x0081c038 */
extern void*   g_focussed_icon;                             /* 0x006687d0 */

#define FLAGS g_appraisal_flags

/* The page break every report line runs before it is written.  A section
 * that will not fit on what is left of the page is rewound and re-emitted
 * on a fresh page; a section that would not fit on a whole page is simply
 * broken.  LBL is the section's restart label. */
#define PAGE_CHECK(LBL)                                                   \
    if (y + 0x16 > 0x1b5) {                                               \
        if (page_start != sect_start) {                                   \
            page_start = sect_start;                                      \
            n = sect_start;                                               \
            indent = lines[sect_start].indent;                            \
            g_report_pages++;                                             \
            goto LBL;                                                     \
        }                                                                 \
        g_report_pages++;                                                 \
        page_start = n;                                                   \
        cur.left = box.left;                                              \
        cur.top = box.top;                                                \
        cur.right = box.right;                                            \
        cur.bottom = box.bottom;                                          \
        y = box.top;                                                      \
    }

/* A plain line: no bar, no measured value. */
#define TEXT_LINE(LBL, MARK, ID)                                          \
    PAGE_CHECK(LBL)                                                       \
    lines[n].page = g_report_pages;                                       \
    lines[n].indent = indent;                                             \
    lines[n].ok = (MARK);                                                 \
    lines[n].step = rand() % 5;                                           \
    lines[n].text = GetString(ID);                                        \
    lines[n].colour = 0;                                                  \
    lines[n].bar = 0;                                                     \
    lines[n].value = 0;                                                   \
    lines[n].mark = 0;                                                    \
    lines[n].range = 0;                                                   \
    lines[n].nids = 0;                                                     \
    n++;                                                                  \
    y += 0x18;

/* A graded line: a bar from 0 to RANGE with its marker at MARKV. */
#define BAR_LINE(LBL, ID, VALUE, MARKV, RANGE)                            \
    PAGE_CHECK(LBL)                                                       \
    lines[n].page = g_report_pages;                                       \
    lines[n].indent = indent;                                             \
    lines[n].ok = ok;                                                     \
    lines[n].step = rand() % 5;                                           \
    lines[n].text = GetString(ID);                                        \
    lines[n].colour = 0;                                                  \
    lines[n].bar = 1;                                                     \
    lines[n].value = (VALUE);                                             \
    lines[n].mark = (MARKV);                                              \
    lines[n].range = (RANGE);                                             \
    lines[n].nids = 0;                                                     \
    n++;                                                                  \
    y += 0x18;

// WIP-FUNCTION: LEGOLAND 0x004453a0  (1286/8085 insns emitted, first diverging index 5, frame 0x2190 vs 0x23d4; report sections 1-2 plus the render and input loops)
int RunAppraisalScreen(void)
{
    RepLine lines[100];
    char  namebuf[0x80];
    /* The 0x200-byte line buffer at frame offset 0x1ec4; the report section
     * that sprintf()s into it (0x0044a75d) is not transcribed yet. */
    char  textbuf[0x200];
    int   narr[196];
    AppraisalBox box;
    AppraisalBox cur;
    int n;
    int y;
    int page_start;
    int indent;
    int sect_start;
    int failmask;
    int passed, total;
    int all_passed, all_total;
    int ok;
    int v;
    int nattr, vattr;
    int i;
    int x;
    int nclose;
    int nnarr, narr_cur;
    AppraisalBox title;

    (void)textbuf;      /* until the section at 0x0044a75d is written */
    n = 0;
    page_start = 0;
    indent = 0;
    all_total = 0;
    all_passed = 0;
    nclose = 0;
    nnarr = 0;
    narr_cur = 0;
    failmask = 0;
    if (ScriptRunning())
        return 0;

    g_report_pages = 0;
    g_report_page = 0;
    g_report_page_turned = 1;
    PushRenderingStatusAndUnlockVideoSurface();
    ReadGameButtons();

    y = 0x6d;
    box.left = 0x50;
    box.top = 0x6d;
    box.right = 0x1a4;
    box.bottom = 0x83;

    /* ---- the report's title line ------------------------------------- */
sect1:
    sect_start = n;
    if (FLAGS & 0xf) {
        lines[n].indent = indent;
        TEXT_LINE(sect1, 0, 0x12c)
        lines[n - 1].ok = 1;
    }

    /* ---- what the park holds ----------------------------------------- */
sect2:
    if (FLAGS & 0x4fff0) {
        total = 0;
        passed = 0;
        sect_start = n;
        lines[n].indent = indent;
        TEXT_LINE(sect1, 0, 0x131)         /* the original restarts at sect1 here */
        indent += 0x30;
        CountAttractions(&nattr, &vattr);
        if (FLAGS & 0x4000) {
            total++;
            ok = nattr >= g_goal[5];
            if (ok) passed++; else failmask |= 0x10;
            BAR_LINE(sect2, 0x132, nattr, g_goal[5], g_goal[6])
        }
        if (FLAGS & 0x8000) {
            total++;
            ok = vattr >= g_goal[7];
            if (ok) passed++; else failmask |= 0x20;
            BAR_LINE(sect2, 0x133, vattr, g_goal[7], g_goal[8])
        }
        if (FLAGS & 0x40000) {
            v = PercentObjectsLinked();
            total++;
            ok = v >= g_goal[9];
            if (ok) passed++; else failmask |= 0x40;
            BAR_LINE(sect2, 0x134, v, g_goal[9], g_goal[10])
        }
        if (FLAGS & 0x30) {
            total++;
            ok = CountCastles() >= g_goal[0];
            if (ok) passed++; else failmask |= 0x80;
            switch ((FLAGS >> 4) & 3) {
            case 1: TEXT_LINE(sect2, ok, 0x135) break;
            case 2: TEXT_LINE(sect2, ok, 0x136) break;
            case 3: TEXT_LINE(sect2, ok, 0x137) break;
            }
        }
        if (FLAGS & 0xc0) {
            total++;
            ok = CountDrivingSchools() >= g_goal[1];
            if (ok) passed++; else failmask |= 0x100;
            switch ((FLAGS >> 6) & 3) {
            case 1: TEXT_LINE(sect2, ok, 0x138) break;
            case 2: TEXT_LINE(sect2, ok, 0x139) break;
            case 3: TEXT_LINE(sect2, ok, 0x13a) break;
            }
        }
        if (FLAGS & 0x300) {
            total++;
            ok = CountLogFlumes() >= g_goal[2];
            if (ok) passed++; else failmask |= 0x200;
            switch ((FLAGS >> 8) & 3) {
            case 1: TEXT_LINE(sect2, ok, 0x13b) break;
            case 2: TEXT_LINE(sect2, ok, 0x13c) break;
            case 3: TEXT_LINE(sect2, ok, 0x13d) break;
            }
        }
        if (FLAGS & 0xc00) {
            total++;
            ok = CountBoatingSchools() >= g_goal[3];
            if (ok) passed++; else failmask |= 0x400;
            switch ((FLAGS >> 10) & 3) {
            case 1: TEXT_LINE(sect2, ok, 0x13e) break;
            case 2: TEXT_LINE(sect2, ok, 0x13f) break;
            case 3: TEXT_LINE(sect2, ok, 0x140) break;
            }
        }
        if (FLAGS & 0x3000) {
            total++;
            ok = CountJungleCruises() >= g_goal[4];
            if (ok) passed++; else failmask |= 0x800;
            switch ((FLAGS >> 12) & 3) {
            case 1: TEXT_LINE(sect2, ok, 0x141) break;
            case 2: TEXT_LINE(sect2, ok, 0x142) break;
            case 3: TEXT_LINE(sect2, ok, 0x143) break;
            }
        }
        lines[sect_start].ok = (passed == total);
        all_passed = passed;
        indent -= 0x30;
        all_total = total;
    }

    /* ================================================================== */
    /* Put the screen up and run it.                                      */
    /* ================================================================== */

    if (nclose == 0)
        n--;
    cur.bottom = y + 0x16;
    cur.left = indent + 0x20;
    cur.right = 0x1a4;
    g_report_pages++;
    LoadAppraisalScreenSprites(g_report_pages);
    LoadAppraisalTickSprites();

    while (g_report_open) {
        sub_498b40();
        SetPointer(5);
        ReadGameButtons();
        ResetHitInfo();
        PushRenderingStatusAndLockVideoSurface();
        PrintSprite(g_backdrop, 0, 0, 0, 0);
        UnlightAppraisalPageButtons();
        RenderIcons2(1, 0, 0);
        title.left = 0x28;
        title.top = 0x45;
        title.right = 0x1a4;
        title.bottom = 0x6d;
        NewPrintCent(GetString(0x228), 3, title, 0);

        cur.left = box.left;
        cur.top = box.top;
        cur.right = box.right;
        cur.bottom = box.bottom;

        i = 0;
        while (i < n && lines[i].page != g_report_page)
            i++;
        if (g_report_page_turned) {
            narr_cur = 0;
            nnarr = 0;
        }
        while (i < n && lines[i].page == g_report_page) {
            if (g_report_page_turned && lines[i].nids != 0) {
                narr[nnarr++] = lines[i].ids[0];
                if (lines[i].nids > 1) narr[nnarr++] = lines[i].ids[1];
                if (lines[i].nids > 2) narr[nnarr++] = lines[i].ids[2];
                if (lines[i].nids > 3) narr[nnarr++] = lines[i].ids[3];
                if (lines[i].nids > 4) narr[nnarr++] = lines[i].ids[4];
                if (lines[i].nids > 5) narr[nnarr++] = lines[i].ids[5];
                if (lines[i].nids > 6) narr[nnarr++] = lines[i].ids[6];
                if (lines[i].nids > 7) narr[nnarr++] = lines[i].ids[7];
            }
            if (lines[i].ok == -2)
                x = lines[i].indent + 0x28;
            else
                x = lines[i].indent + 0x50;
            cur.left = x;
            BlitAppraisalSprite(x - 0x28, cur.top, lines[i].ok, lines[i].step);
            cur.right = 0x1a4;
            cur.bottom = cur.top + 0x16;
            NewPrintColoured(lines[i].text, 2, cur, lines[i].colour);
            if (lines[i].bar) {
                box.left = 0x126;
                box.top = cur.top;
                box.right = 0x1a4;
                box.bottom = cur.top + 8;
                DrawAppraisalBar(box, lines[i].value, lines[i].range, lines[i].mark);
            }
            cur.top += 0x18;
            i++;
        }

        if (g_report_page_turned)
            g_report_page_turned = 0;
        if (narr_cur < nnarr) {
            if (!IsNarrationPlaying() && narr[narr_cur] != -1) {
                sprintf(namebuf, "TEXT%04d.WAV", narr[narr_cur]);
                narr_cur++;
                PauseCurrentTrack();
                PlayNarrationFile(namebuf);
                ResumeCurrentTrack();
            }
        } else if (!IsNarrationPlaying())
            UpdateHelpBar();

        ProcessFrontEndHelp();
        UpdateFocussedIconPtr();
        PopRenderingStatus();
        if (g_focussed_icon)
            SetPointer(6);
        CheckFocussedIcon();
        RenderingComplete();
    }
    FreeAppraisalScreenSprites();
    PopRenderingStatus();
    PauseCurrentTrack();
    SetInGameIconHandlers();
    for (i = 0; i < n; i++) {
        if (lines[i].ok == 0)
            return 0;
    }
    return 1;
}
