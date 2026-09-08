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

/* The render-object walk section 7 uses: obj->p->q->type is a short, and
 * obj->kind (a short at +4) is handed to IsObjectRunning by address. */
typedef struct RObjQ { char pad00[0x20]; short type; } RObjQ;
typedef struct RObjP { char pad00[0x0c]; RObjQ* q; } RObjP;
typedef struct RObj  { RObjP* p; short kind; } RObj;
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
extern RObj* GetFirstRenderObject(void);                    /* 0x0045a850 */
extern RObj* GetNextRenderObject(RObj* o);                  /* 0x0045a8b0 */
extern int   IsObjectRunning(RObjQ* q, short* kind);        /* 0x0044f360 */
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
extern int     g_num_visitors;                              /* 0x00832bd0 */
extern int     g_appraisal_rank;                            /* 0x0083297c */
extern int     g_appraisal_rank_bias;                       /* 0x00832b9c */

#define FLAGS g_appraisal_flags

/* The page break every report line runs before it is written.  A section
 * that will not fit on what is left of the page is rewound and re-emitted
 * on a fresh page; a section that would not fit on a whole page is simply
 * broken.  LBL is the section's restart label.
 *
 * The new page's rectangle is reset on BOTH arms.  The rewind arm needs it
 * as much as the other one: it jumps back to the section label, whose first
 * act is this same page check, so without a fresh `y` the check would fire a
 * second time and bump `g_report_pages` twice.  The original's object shows
 * the duplication plainly -- VC6 speculates the four `box` loads and two of
 * the stores above the `cmp ebp,esi`, then re-emits the stores in the
 * fall-through arm (0x00445450 and 0x00445481).
 *
 * `cur.top` is deliberately not among them: `[esp+0x30]` is touched exactly
 * three times in the whole original body and all three are inside the render
 * loop.  The build's page break leaves the new top in `y` alone. */
#define PAGE_CHECK(LBL)                                                   \
    if (y + 0x16 > 0x1b5) {                                               \
        if (page_start != sect_start) {                                   \
            page_start = sect_start;                                      \
            n = sect_start;                                               \
            indent = lines[sect_start].indent;                            \
            g_report_pages++;                                             \
            cur.right = box.right;                                        \
            cur.left = box.left;                                          \
            cur.bottom = box.bottom;                                      \
            y = box.top;                                                  \
            goto LBL;                                                     \
        }                                                                 \
        g_report_pages++;                                                 \
        page_start = n;                                                   \
        cur.right = box.right;                                            \
        cur.left = box.left;                                              \
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

/* The five closing lines below do NOT advance y: the original never emits
 * `y += 0x18` for them, so each one's page check re-reads the previous
 * line's `cur.bottom` and they all land on the same row.  Reproduced. */
#define TEXT_LINE_NOY(LBL, MARK, ID)                                      \
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
    lines[n].nids = 0;                                                    \
    n++;

/* The one line whose text is the sprintf buffer rather than a string id. */
#define BUF_LINE(LBL, MARK)                                               \
    PAGE_CHECK(LBL)                                                       \
    lines[n].page = g_report_pages;                                       \
    lines[n].indent = indent;                                             \
    lines[n].ok = (MARK);                                                 \
    lines[n].step = rand() % 5;                                           \
    lines[n].text = textbuf;                                              \
    lines[n].colour = 0;                                                  \
    lines[n].bar = 0;                                                     \
    lines[n].value = 0;                                                   \
    lines[n].mark = 0;                                                    \
    lines[n].range = 0;                                                   \
    lines[n].nids = 0;                                                    \
    n++;

/* Append a narration string id to the line that was just written.  The
 * render loop copies these into the narration queue when the page turns. */
#define NARR(ID)                                                          \
    lines[n - 1].ids[lines[n - 1].nids] = (ID);                           \
    lines[n - 1].nids++;

// WIP-FUNCTION: LEGOLAND 0x004453a0  (6821/8085 insns emitted, 29309/34662 bytes, frame 0x23d4 exact, first diverging index 6, mismatch 8005; the build's page break constant-propagates box where the original reloads it, and indent is enregistered where the original spills it and keeps page_start in ebp)
int RunAppraisalScreen(void)
{
    RepLine lines[100];
    char  namebuf[0x80];
    /* The 0x200-byte line buffer at frame offset 0x1ec4. */
    char  textbuf[0x200];
    /* 200, not 196: the frame's top is 0x23e4 and the queue starts at
     * 0x20c4.  nnarr and narr_cur are deliberately NOT zeroed here -- the
     * original leaves them undefined until the render loop's first page
     * turn, which is what lets VC6 pack them onto build-phase slots. */
    int   narr[200];
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
    int nscen, vscen;
    int nfood, vfood;
    int nshop, vshop;
    int nvis, vmood, vages;
    int nhint;
    int nrun;
    RObj* obj;
    short kind;
    int i;
    int nnarr, narr_cur;
    AppraisalBox title;

    n = 0;
    page_start = 0;
    indent = 0;
    all_total = 0;
    all_passed = 0;
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
        all_passed += passed;
        indent -= 0x30;
        all_total += total;
    }

    /* ---- scenery ------------------------------------------------------ */
sect3:
    if (FLAGS & 0x38000000) {
        total = 0;
        passed = 0;
        sect_start = n;
        lines[n].indent = indent;
        TEXT_LINE(sect3, ok, 0x144)
        indent += 0x30;
        CountScenery(&nscen, &vscen);
        if (FLAGS & 0x8000000) {
            total++;
            ok = nscen >= g_goal[25];
            if (ok) passed++; else failmask |= 0x1000;
            BAR_LINE(sect3, 0x132, nscen, g_goal[25], g_goal[26])
        }
        /* The 0x20000000 bit of the section guard has no statistic behind
         * it and fail bit 0x4000 is never set: original, left alone. */
        if (FLAGS & 0x10000000) {
            total++;
            ok = vscen >= g_goal[27];
            if (ok) passed++; else failmask |= 0x2000;
            BAR_LINE(sect3, 0x133, vscen, g_goal[27], g_goal[28])
        }
        lines[sect_start].ok = (passed == total);
        all_total += total;
        indent -= 0x30;
        all_passed += passed;
    }

    /* ---- food --------------------------------------------------------- */
sect4:
    if (FLAGS & 0xc0000000) {
        total = 0;
        passed = 0;
        sect_start = n;
        lines[n].indent = indent;
        TEXT_LINE(sect4, ok, 0x145)
        indent += 0x30;
        CountFood(&nfood, &vfood);
        if (FLAGS & 0x40000000) {
            total++;
            ok = nfood >= g_goal[31];
            if (ok) passed++; else failmask |= 0x8000;
            BAR_LINE(sect4, 0x132, nfood, g_goal[31], g_goal[32])
        }
        if (FLAGS & 0x80000000) {
            total++;
            ok = vfood >= g_goal[33];
            if (ok) passed++; else failmask |= 0x10000;
            BAR_LINE(sect4, 0x133, vfood, g_goal[33], g_goal[34])
        }
        lines[sect_start].ok = (passed == total);
        all_total += total;
        indent -= 0x30;
        all_passed += passed;
    }

    /* ---- shops -------------------------------------------------------- */
sect5:
    if (FLAGS & 0x30000) {
        total = 0;
        passed = 0;
        sect_start = n;
        lines[n].indent = indent;
        TEXT_LINE(sect5, ok, 0x146)
        indent += 0x30;
        CountShops(&nshop, &vshop);
        if (FLAGS & 0x10000) {
            total++;
            ok = nshop >= g_goal[13];
            if (ok) passed++; else failmask |= 0x20000;
            BAR_LINE(sect5, 0x132, nshop, g_goal[13], g_goal[14])
        }
        if (FLAGS & 0x20000) {
            total++;
            ok = vshop >= g_goal[15];
            if (ok) passed++; else failmask |= 0x40000;
            BAR_LINE(sect5, 0x133, vshop, g_goal[15], g_goal[16])
        }
        lines[sect_start].ok = (passed == total);
        all_total += total;
        indent -= 0x30;
        all_passed += passed;
    }

    /* ---- visitors ----------------------------------------------------- */
sect6:
    if (FLAGS & 0x5080000) {
        total = 0;
        passed = 0;
        sect_start = n;
        lines[n].indent = indent;
        TEXT_LINE(sect6, ok, 0x147)
        indent += 0x30;
        CountVisitors(&nvis, &vmood, &vages);
        if (FLAGS & 0x80000) {
            total++;
            ok = nvis >= g_goal[11];
            if (ok) passed++; else failmask |= 0x80000;
            /* Original bug: this statistic advances the report's y by a line
             * but never writes one, so it leaves a blank gap. */
            y += 0x18;
        }
        if (FLAGS & 0x1000000) {
            total++;
            ok = vmood >= g_goal[17];
            if (ok) passed++; else failmask |= 0x100000;
            BAR_LINE(sect6, 0x148, vmood, g_goal[17], g_goal[18])
        }
        if (FLAGS & 0x4000000) {
            total++;
            /* Original bug: graded against the mood goal, not its own. */
            ok = vages >= g_goal[17];
            if (ok) passed++; else failmask |= 0x200000;
            BAR_LINE(sect6, 0x149, vages, g_goal[17], g_goal[18])
        }
        lines[sect_start].ok = (passed == total);
        all_total += total;
        indent -= 0x30;
        all_passed += passed;
    }

    /* ---- the park at work --------------------------------------------- */
sect7:
    if (FLAGS & 0xe00000) {
        total = 0;
        passed = 0;
        sect_start = n;
        lines[n].indent = indent;
        TEXT_LINE(sect7, ok, 0x14a)
        indent += 0x30;
        if (FLAGS & 0x200000) {
            total++;
            ok = g_num_visitors >= g_goal[19];
            if (ok) passed++; else failmask |= 0x400000;
            BAR_LINE(sect7, 0x14b, g_num_visitors, g_goal[19], g_goal[20])
        }
        if (FLAGS & 0x400000) {
            nrun = 0;
            obj = GetFirstRenderObject();
            while (obj) {
                kind = obj->kind;
                if (obj->p->q->type != 0 && obj->p->q->type != 2) {
                    if (IsObjectRunning(obj->p->q, &kind))
                        nrun++;
                }
                obj = GetNextRenderObject(obj);
            }
            total++;
            ok = nrun >= g_goal[21];
            if (ok) passed++; else failmask |= 0x800000;
            BAR_LINE(sect7, 0x14c, nrun, g_goal[21], g_goal[22])
        }
        if (FLAGS & 0x800000) {
            v = MapCellCount();
            total++;
            ok = v >= g_goal[23];
            if (ok) passed++; else failmask |= 0x1000000;
            BAR_LINE(sect7, 0x14d, v, g_goal[23], g_goal[24])
        }
        lines[sect_start].ok = (passed == total);
        all_total += total;
        indent -= 0x30;
        all_passed += passed;
    }

    /* ---- the advice ---------------------------------------------------- */
    /* Unguarded: every report ends with a verdict line and one piece of
     * advice per failed statistic.  ok is the bullet form here: -2 the
     * verdict/plain bullet, -1 a piece of advice, -3 its continuation. */
sect8:
    sect_start = n;
    lines[n].indent = indent;
    TEXT_LINE(sect8, -2, 0x230)
    indent += 0x30;
    if (all_passed < all_total / 2) {
        TEXT_LINE(sect8, -2, 0x14f)
        NARR(0x14f)
        TEXT_LINE(sect8, -2, 0x150)
    } else if (all_passed < all_total) {
        TEXT_LINE(sect8, -2, 0x151)
        NARR(0x151)
        TEXT_LINE(sect8, -2, 0x150)
    } else {
        TEXT_LINE(sect8, -2, 0x153)
        NARR(0x153)
        TEXT_LINE(sect8, -2, 0x154)
    }
    if (failmask & 0x1) {
        TEXT_LINE(sect8, -1, 0x155)
    }
    if (failmask & 0x2) {
        TEXT_LINE(sect8, -1, 0x156)
    }
    if (failmask & 0x4) {
        TEXT_LINE(sect8, -1, 0x157)
    }
    if (failmask & 0x8) {
        TEXT_LINE(sect8, -1, 0x158)
    }
    switch (failmask & 0x30) {
    case 0x10:
        TEXT_LINE(sect8, -1, 0x159)
        break;
    case 0x20:
        TEXT_LINE(sect8, -1, 0x15a)
        break;
    case 0x30:
        TEXT_LINE(sect8, -1, 0x15b)
        TEXT_LINE(sect8, -3, 0x133)
        break;
    }
    if (failmask & 0x40) {
        TEXT_LINE(sect8, -1, 0x15c)
        NARR(0x15c)
        TEXT_LINE(sect8, -3, 0x15d)
    }
    if (failmask & 0x80) {
        TEXT_LINE(sect8, -1, 0x15e)
    }
    if (failmask & 0x100) {
        TEXT_LINE(sect8, -1, 0x15f)
    }
    if (failmask & 0x200) {
        TEXT_LINE(sect8, -1, 0x160)
    }
    if (failmask & 0x400) {
        TEXT_LINE(sect8, -1, 0x161)
    }
    if (failmask & 0x800) {
        TEXT_LINE(sect8, -1, 0x162)
    }
    switch (failmask & 0x3000) {
    case 0x1000:
        TEXT_LINE(sect8, -1, 0x163)
        break;
    case 0x2000:
        TEXT_LINE(sect8, -1, 0x164)
        break;
    case 0x3000:
        TEXT_LINE(sect8, -1, 0x165)
        break;
    }
    switch (failmask & 0x18000) {
    case 0x8000:
        TEXT_LINE(sect8, -1, 0x166)
        break;
    case 0x10000:
        TEXT_LINE(sect8, -1, 0x167)
        break;
    case 0x18000:
        TEXT_LINE(sect8, -1, 0x168)
        break;
    }
    switch (failmask & 0x60000) {
    case 0x20000:
        TEXT_LINE(sect8, -1, 0x169)
        break;
    case 0x40000:
        TEXT_LINE(sect8, -1, 0x16a)
        break;
    case 0x60000:
        TEXT_LINE(sect8, -1, 0x16b)
        NARR(0x16b)
        TEXT_LINE(sect8, -3, 0x231)
        break;
    }
    if (failmask & 0x80000) {
        TEXT_LINE(sect8, -1, 0x16c)
    }
    if (failmask & 0x100000) {
        TEXT_LINE(sect8, -1, 0x16d)
    }
    /* String id 0x16e is skipped: original. */
    if (failmask & 0x200000) {
        TEXT_LINE(sect8, -1, 0x16f)
    }
    if (failmask & 0x400000) {
        TEXT_LINE(sect8, -1, 0x170)
    }
    if (failmask & 0x800000) {
        TEXT_LINE(sect8, -1, 0x171)
        NARR(0x171)
        TEXT_LINE(sect8, -3, 0x172)
    }
    if (failmask & 0x1000000) {
        TEXT_LINE(sect8, -1, 0x173)
    }

    /* The closing "next time" line.  Its page checks restart at sect9, not
     * at sect8: original.  A page break here therefore throws the whole
     * advice section away instead of re-emitting it. */
    if (failmask != 0 && g_appraisal_rank != 0) {
        if (g_appraisal_rank_bias < 0)
            v = g_appraisal_rank_bias + g_appraisal_rank - 1;
        else
            v = g_appraisal_rank - 1;
        if (v > 1) {
            sprintf(textbuf, GetString(0x235), GetString(v + 0x514));
            BUF_LINE(sect9, -2)
            NARR(0x235)
            NARR(v + 0x514)
            NARR(0x236)
            TEXT_LINE_NOY(sect9, -2, 0x236)
        } else if (v > 0) {
            TEXT_LINE_NOY(sect9, -2, 0x514)
            NARR(0x514)
            TEXT_LINE_NOY(sect9, -2, 0x236)
        } else {
            TEXT_LINE_NOY(sect9, -2, 0x237)
            NARR(0x237)
            TEXT_LINE_NOY(sect9, -2, 0x238)
        }
    }
    indent -= 0x30;
sect9:
    sect_start = n;
    lines[n].indent = indent;
    TEXT_LINE(sect9, -2, 0x174)
    NARR(0x174)
    nhint = 0;
    indent += 0x30;
    /* One hint per failed group, its phrasing picked at random.  Every hint
     * line is also queued for narration. */
    if (failmask & 0xf) {
        switch (rand() & 3) {
        case 0:
            TEXT_LINE(sect9, -1, 0x17c) NARR(0x17c)
            TEXT_LINE(sect9, -3, 0x17d) NARR(0x17d)
            nhint++;
            break;
        case 1:
            TEXT_LINE(sect9, -1, 0x187) NARR(0x187)
            TEXT_LINE(sect9, -3, 0x188) NARR(0x188)
            nhint++;
            break;
        case 2:
            TEXT_LINE(sect9, -1, 0x190) NARR(0x190)
            TEXT_LINE(sect9, -3, 0x191) NARR(0x191)
            TEXT_LINE(sect9, -3, 0x192) NARR(0x192)
            nhint++;
            break;
        case 3:
            TEXT_LINE(sect9, -1, 0x19a) NARR(0x19a)
            TEXT_LINE(sect9, -3, 0x19b) NARR(0x19b)
            nhint++;
            break;
        }
    }
    if (failmask & 0x70) {
        switch (rand() & 3) {
        case 0:
            TEXT_LINE(sect9, -1, 0x1a4) NARR(0x1a4)
            TEXT_LINE(sect9, -3, 0x1a5) NARR(0x1a5)
            TEXT_LINE(sect9, -3, 0x1a6) NARR(0x1a6)
            nhint++;
            break;
        case 1:
            TEXT_LINE(sect9, -1, 0x1ae) NARR(0x1ae)
            TEXT_LINE(sect9, -3, 0x1af) NARR(0x1af)
            TEXT_LINE(sect9, -3, 0x1b0) NARR(0x1b0)
            nhint++;
            break;
        case 2:
            TEXT_LINE(sect9, -1, 0x1b8) NARR(0x1b8)
            TEXT_LINE(sect9, -3, 0x1b9) NARR(0x1b9)
            nhint++;
            break;
        }
    }
    if (failmask & 0x7000) {
        switch (rand() % 3) {
        case 0:
            TEXT_LINE(sect9, -1, 0x1c2) NARR(0x1c2)
            TEXT_LINE(sect9, -3, 0x1c3) NARR(0x1c3)
            TEXT_LINE(sect9, -3, 0x1c4) NARR(0x1c4)
            nhint++;
            break;
        case 1:
            TEXT_LINE(sect9, -1, 0x1cc) NARR(0x1cc)
            TEXT_LINE(sect9, -3, 0x1cd) NARR(0x1cd)
            nhint++;
            break;
        }
    }
    if (failmask & 0x18000) {
        switch (rand() % 3) {
        case 0:
            TEXT_LINE(sect9, -1, 0x1d6) NARR(0x1d6)
            TEXT_LINE(sect9, -3, 0x1d7) NARR(0x1d7)
            TEXT_LINE(sect9, -3, 0x1d8) NARR(0x1d8)
            nhint++;
            break;
        case 1:
            TEXT_LINE(sect9, -1, 0x1e0) NARR(0x1e0)
            TEXT_LINE(sect9, -3, 0x1e1) NARR(0x1e1)
            nhint++;
            break;
        }
    }
    if (failmask & 0x260000) {
        switch (rand() % 3) {
        case 0:
            if (failmask & 0x200000) {
                TEXT_LINE(sect9, -1, 0x1ea) NARR(0x1ea)
                TEXT_LINE(sect9, -3, 0x1eb) NARR(0x1eb)
                TEXT_LINE(sect9, -3, 0x1ec) NARR(0x1ec)
                nhint++;
            }
            break;
        case 1:
            TEXT_LINE(sect9, -1, 0x1f4) NARR(0x1f4)
            TEXT_LINE(sect9, -3, 0x1f5) NARR(0x1f5)
            nhint++;
            break;
        case 2:
            if (FLAGS & 0xf) {
                TEXT_LINE(sect9, -1, 0x1fe) NARR(0x1fe)
                TEXT_LINE(sect9, -3, 0x1ff) NARR(0x1ff)
                nhint++;
            }
            break;
        }
    }
    if (failmask & 0x1080000) {
        if (rand() & 1) {
            TEXT_LINE(sect9, -1, 0x208) NARR(0x208)
        } else {
            TEXT_LINE(sect9, -1, 0x212) NARR(0x212)
            TEXT_LINE(sect9, -3, 0x213) NARR(0x213)
        }
        nhint++;
    }
    if (failmask & 0xc00000) {
        if (rand() & 1) {
            if (failmask & 0x800000) {
                TEXT_LINE(sect9, -1, 0x21c) NARR(0x21c)
                TEXT_LINE(sect9, -3, 0x21d) NARR(0x21d)
                nhint++;
            }
        } else {
            if (failmask & 0x400000) {
                TEXT_LINE(sect9, -1, 0x226) NARR(0x226)
                TEXT_LINE(sect9, -3, 0x227) NARR(0x227)
                nhint++;
            }
        }
    }

    /* ================================================================== */
    /* Put the screen up and run it.                                      */
    /* ================================================================== */

    /* Only the lines up to the last hint are kept. */
    if (nhint == 0)
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
                cur.left = lines[i].indent + 0x28;
            else
                cur.left = lines[i].indent + 0x50;
            BlitAppraisalSprite(cur.left - 0x28, cur.top, lines[i].ok, lines[i].step);
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
