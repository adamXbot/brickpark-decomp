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
#ifndef LEGOLAND_PORTABLE
extern void  PlayNarrationFile(const char* name);           /* 0x00498630 */
#else
extern int PlayNarrationFile(const char* name);           /* 0x00498630 */
#endif
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
#ifndef LEGOLAND_PORTABLE
extern void  SetPointer(int shape);                         /* 0x00463850 */
#else
extern int SetPointer(int shape);                         /* 0x00463850 */
#endif
extern void  ProcessFrontEndHelp(void);                     /* 0x0046d080 */
extern void  UpdateFocussedIconPtr(void);                   /* 0x004700a0 */
extern char  CheckFocussedIcon(void);                       /* 0x0046f4c0 */
extern int   RenderingComplete(void);                       /* 0x00466500 */
extern void  UpdateHelpBar(void);                           /* 0x0046d110 */
extern int   IsNarrationPlaying(void);                      /* 0x00498cf0 */
#ifndef LEGOLAND_PORTABLE
extern void  PauseCurrentTrack(void);                       /* 0x00498920 */
#else
extern int PauseCurrentTrack(void);                       /* 0x00498920 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void  ResumeCurrentTrack(void);                      /* 0x00498b00 */
#else
extern int ResumeCurrentTrack(void);                      /* 0x00498b00 */
#endif
extern void  SetInGameIconHandlers(void);                   /* 0x00474880 */
extern void  sub_498b40(void);                              /* 0x00498b40 */

extern Sprite* g_backdrop;                                  /* 0x00810148 */
extern int     g_report_open;                               /* 0x0081c038 */
extern void*   g_focussed_icon;                             /* 0x006687d0 */
extern int     g_num_visitors;                              /* 0x00832bd0 */
extern int     g_appraisal_rank;                            /* 0x0083297c */
extern int     g_appraisal_rank_bias;                       /* 0x00832b9c */

#define FLAGS g_appraisal_flags

/* See the declaration of `v`: section 9's hint counter and the render
 * loop's narration count are both that same local.  VC6 packs all three
 * onto one slot anyway (`/FAs` shows `_v$ == _nnarr$` before this was
 * spelled out), and saying so in the source is worth a point of LCS:
 * it also flips `passed` below `total`, which is the original's order. */
#define nhint v
#define nnarr v

/* The page break every report line runs before it is written.  A section
 * that will not fit on what is left of the page is rewound and re-emitted
 * on a fresh page; a section that would not fit on a whole page is simply
 * broken.  LBL is the section's restart label.
 *
 * The new page's rectangle is reset TWICE, once above the rewind test and
 * once in the fall-through arm.  That is not a stylistic choice: it is what
 * the original's object says.  At 0x004456a7 the four `box` loads and two of
 * the `cur` stores sit ABOVE the `cmp ebp,ecx`, `cur.bottom`'s store among
 * them is gone (it is dead on both paths -- the rewind arm recomputes it at
 * the section label and the fall-through re-stores it), and then the
 * fall-through arm at 0x004456cb reloads `box.bottom` and `box.right` and
 * re-stores all three.  The copy above the test is what serves the rewind
 * arm, which is why the shared `rew_sectK` block at 0x004464b7 contains no
 * copy of its own.  Writing one copy in each arm instead lets VC6 sink the
 * rewind arm's copy INTO the shared block and costs four instructions at
 * every one of the ~120 sites.
 *
 * `cur.top` never reaches memory here: `[esp+0x30]` is touched exactly three
 * times in the whole original body and all three are inside the render loop.
 * The struct copy loads `box.top` straight into `edi` and leaves it there. */
#define PAGE_CHECK(LBL)                                                   \
    cur.bottom = cur.top + 0x16;                                          \
    if (cur.bottom > 0x1b5) {                                             \
        cur = box;                                                        \
        if (page_start != sect_start)                                     \
            goto rew_##LBL;                                               \
        g_report_pages++;                                                 \
        page_start = n;                                                   \
        cur = box;                                                        \
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
    cur.top += 0x18;

/* A graded line: a bar from 0 to RANGE with its marker at MARKV. */
#define BAR_LINE(LBL, ID, VALUE, MARKV, RANGE)                            \
    PAGE_CHECK(LBL)                                                       \
    lines[n].page = g_report_pages;                                       \
    lines[n].indent = indent;                                             \
    lines[n].ok = line_ok;                                                     \
    lines[n].step = rand() % 5;                                           \
    lines[n].text = GetString(ID);                                        \
    lines[n].colour = 0;                                                  \
    lines[n].bar = 1;                                                     \
    lines[n].value = (VALUE);                                             \
    lines[n].mark = (MARKV);                                              \
    lines[n].range = (RANGE);                                             \
    lines[n].nids = 0;                                                     \
    n++;                                                                  \
    cur.top += 0x18;

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

/* Section 8's lines update `cur.bottom` at the LINE END and check it bare.
 * The original says so at 0x00448661: `add edi,0x18 / lea eax,[edi+0x16] /
 * mov [esp+0x38],eax` sits BEFORE the `test byte ptr [esp+0x48],2` of the
 * next piece of advice, whose check is then `cmp [esp+0x38],0x1b5`.  The
 * section header's own check reads the value computed at 0x00447e6c, just
 * above the `sect8:` label. */
#define PAGE_CHECK8(LBL)                                                  \
    if (cur.bottom > 0x1b5) {                                             \
        cur = box;                                                        \
        if (page_start != sect_start)                                     \
            goto rew_##LBL;                                               \
        g_report_pages++;                                                 \
        page_start = n;                                                   \
        cur = box;                                                        \
    }

/* The nine section HEADS specialise the rewind arm.  At a head
 * `sect_start == n` and `indent` has not moved since the section's own
 * `lines[n].indent = indent`, so the general rewind's `n = sect_start` and
 * `indent = lines[sect_start].indent` are both no-ops there and only
 * `page_start = n; g_report_pages++;` survives; the original emits exactly
 * that inline at all nine head sites and jumps straight at the SECTION
 * label rather than at the shared rewind block:
 *
 *     0x0044546c  s1   ... mov [esp+0x10],ebp ... jmp 0x445422 (sect1)
 *     0x00445594  s2   jne 0x44546c  -- into section ONE's copy (the bug)
 *     0x00446795  s3   jmp 0x446a3f  -- into rew_sect3's tail
 *     0x00446bd8  s4   jmp 0x446e7c        0x00447012  s5  jmp 0x4472b3
 *     0x00447449  s6   jmp 0x44772e        0x004478c4  s7  jmp 0x447d44
 *     0x00447eb8  s8   ... mov [esp+0x10],ebp ... jmp 0x447e73 (sect8)
 *     0x0044ad00  s9   ... mov [esp+0x10],ebp ... jmp 0x44acbb (sect9)
 *
 * Sections 3-7 are guarded, so their arm tail-merges with the shared rewind
 * block's `mov [esp+0x10],ebp / mov ecx,[FLAGS] / test / jne` suffix -- the
 * block re-tests the section guard because `goto sectK` lands on the `if`.
 * Sections 1, 8 and 9 have nothing to merge with and keep the store inline.
 * Worth 131 on the mismatch counter and 2.0 points of LCS. */
#define PAGE_CHECK_H(LBL)                                                 \
    if (cur.bottom > 0x1b5) {                                             \
        cur = box;                                                        \
        if (page_start != sect_start) {                                   \
            page_start = n;                                               \
            g_report_pages++;                                             \
            goto LBL;                                                     \
        }                                                                 \
        g_report_pages++;                                                 \
        page_start = n;                                                   \
        cur = box;                                                        \
    }

/* A section HEADER line.  The original's nine section labels are all
 * entered with `cur.bottom` already computed: at every one of them the
 * `lea eax,[edi+0x16]` sits ABOVE the label and the head's own check is a
 * bare `cmp eax,0x1b5` (0x00445530/0x00445566, 0x00446727/0x00446760,
 * 0x00446b6a/0x00446ba3, 0x00446fa4/0x00446fdd, 0x004473db/0x00447414,
 * 0x00447856/0x0044788f, 0x00447e6c/0x00447e88, 0x0044acb8/0x0044acd0;
 * section one's comes from the `box` initialiser at 0x0044541e).  That is
 * what lets the rewind arm's `cur = box` feed the check through `eax` and
 * is why the hoisted copy never stores `cur.bottom`. */
#define TEXT_LINE_H(LBL, MARK, ID)                                        \
    PAGE_CHECK_H(LBL)                                                      \
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
    n++;                                                                  \
    cur.top += 0x18;

#define LINE8_BODY(MARK, TEXTEXPR)                                        \
    lines[n].page = g_report_pages;                                       \
    lines[n].indent = indent;                                             \
    lines[n].ok = (MARK);                                                 \
    lines[n].step = rand() % 5;                                           \
    lines[n].text = (TEXTEXPR);                                           \
    lines[n].colour = 0;                                                  \
    lines[n].bar = 0;                                                     \
    lines[n].value = 0;                                                   \
    lines[n].mark = 0;                                                    \
    lines[n].range = 0;                                                   \
    lines[n].nids = 0;                                                    \
    n++;

#define TEXT_LINE8(LBL, MARK, ID)                                         \
    PAGE_CHECK8(LBL)                                                      \
    LINE8_BODY(MARK, GetString(ID))                                       \
    cur.top += 0x18;                                                      \
    cur.bottom = cur.top + 0x16;

#define TEXT_LINE_NOY8(LBL, MARK, ID)                                     \
    PAGE_CHECK8(LBL)                                                      \
    LINE8_BODY(MARK, GetString(ID))

#define BUF_LINE8(LBL, MARK)                                              \
    PAGE_CHECK8(LBL)                                                      \
    LINE8_BODY(MARK, textbuf)

// WIP-FUNCTION: LEGOLAND 0x004453a0  (8181/8085 insns emitted, 34928/34662 bytes, frame 0x23d4 exact, first diverging index 8, mismatch 7845, index-for-index MATCH 240 in 104 runs, true LCS 64.5%, difflib 55.1% -- all four lane bests; audit prints ESCAPES only because the body is 96 instructions longer than the original.  See the CLOSING ASSESSMENT in docs/lanes/scope-ll16.md.  The nine section HEADS now specialise their rewind arm the way the original does (PAGE_CHECK_H), which also closed section 7's header -- it is the original instruction for instruction, address spills included.  The +96 is three zones: SECTION 9 at a flat +2 per hint line (the original compares page_start straight out of its home 39 times because ebp holds the line offset there, and keeps box.left in edx across the branch), one 17-instruction tail-merge the original makes in the closing block, and register naming in section 2 and at the five guarded heads.  Residual 1 costs no instructions and ~6 LCS points: the THREE-SLOT ROTATION -- our n*0x4c byte-offset CSE temp holds 0x10 with 221 references where the original puts page_start, so page_start, indent and sect_start all sit one slot high.  VC6 orders this frame by descending reference weight per dword and the ORIGINAL breaks that rule at exactly these three slots: its temp has 222 references and still sits at 0x18, below page_start's 173 and indent's 171.  Weight arithmetic, web decomposition and loop weighting are all refuted; only a non-weight mechanism can move it)
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
    /* The frame's 0x1c..0x44 block, spelled as ONE non-escaping aggregate.
     * VC6 lays this frame out in descending reference weight per dword (the
     * lane doc's tenth pass) and ranks an aggregate as one object, so an
     * aggregate is the only handle there is on where a group of slots lands.
     * The original has `box` at 0x1c, `cur` at 0x2c, `ok`/`obj` at 0x3c and
     * `passed` at 0x40; separate locals with these weights get scattered
     * across the frame (`cur` at 0x14, `box` at 0x44 before this).  Eleven
     * dwords carrying 1,611 references rank at ~147 per dword, which is
     * below `indent` and above `total`, and puts all four of them on the
     * original's own displacements.  The `all_total` member is the eleventh
     * dword: it is what makes the object rank low enough, and it costs the
     * frame nothing because it is a scalar that would otherwise need a slot
     * of its own.  `ok` and `obj` are a union because they share a slot in
     * the original too (their live ranges are disjoint) and giving `obj` a
     * dword of its own breaks the frame to 0x23d8 -- measured.
     * This is only free once `bar` has lost its stack home (see the render
     * loop): inside an aggregate `box`'s home is live to the end, so `bar`
     * can no longer share it and the frame grows by 0x10. */
    struct {
        AppraisalBox box, cur;
        union { int line_ok; RObj* obj; } u;
        int passed;
        int all_total;
    } r;
#define box r.box
#define cur r.cur
#define line_ok r.u.line_ok
#define obj r.u.obj
#define passed r.passed
#define all_total r.all_total
    int n;
    int page_start;
    int indent;
    int sect_start;
    int failmask;
    int total;
    int all_passed;
    /* `v` and section 9's hint counter are ONE local, and they have to be.
     * The original's frame holds exactly 33 scalar dwords below `lines`;
     * with the page reset hoisted (above) `indent` loses its register and
     * needs a whole slot of its own, so a 34th dword appears and pushes
     * `lines` off 0x94.  VC6 pays for the original's 34th name by packing it
     * onto a CSE temp -- five of the original's slots carry a temp and a
     * named local -- and nothing in this source persuades it to do the same,
     * so the two uses share a name instead.  They never overlap: every write
     * of `v` (sections 2 and 7 and the closing block) is dead before section
     * 9's unconditional `nhint = 0`, and nothing reads the hint counter until
     * after it. */
    int v;
    int nattr, vattr;
    int nscen, vscen;
    int nfood, vfood;
    int nshop, vshop;
    int nvis, vmood, vages;
    int nrun;
    short kind;
    int i;
    int narr_cur;
    AppraisalBox title;
    AppraisalBox bar;

    /* This order is the original's, and it is not free: VC6 emits the
     * entry zero-stores in source order, and the original's is
     * `mov [esp+0x14],ebx / xor esi,esi / mov [esp+0x10],ebp / ...` --
     * indent, then n, then page_start.  Any other order costs two
     * index-for-index matches. */
    indent = 0;
    n = 0;
    page_start = 0;
    all_passed = 0;
    all_total = 0;
    failmask = 0;
    if (ScriptRunning())
        return 0;

    g_report_pages = 0;
    g_report_page = 0;
    g_report_page_turned = 1;
    PushRenderingStatusAndUnlockVideoSurface();
    ReadGameButtons();

    /* `box` is seeded THROUGH `cur`.  Written as four constant stores of its
     * own, `box` has a single reaching definition and VC6 constant-folds
     * 0x50/0x6d/0x1a4/0x83 into every page reset, dead-stores `box.bottom`
     * away and keeps `box.left` in a register for the whole build.  Copying
     * it out of `cur`, which has ~120 further definitions, makes it opaque:
     * the entry becomes the original's four-registers-then-four-stores shape
     * and the page-break sites reload all four fields as the original does.
     * `box`+`cur` go from 901 references to 1,501 (the original's 1,448). */
    cur.left   = 0x50;
    cur.top    = 0x6d;
    cur.right  = 0x1a4;
    cur.bottom = 0x83;
    box = cur;
    cur.top = 0x6d;

    /* ---- the report's title line ------------------------------------- */
sect1:
    sect_start = n;
    if (FLAGS & 0xf) {
        lines[n].indent = indent;
        TEXT_LINE_H(sect1, 0, 0x12c)
        lines[n - 1].ok = 1;
        cur.bottom = cur.top + 0x16;
    }

    /* ---- what the park holds ----------------------------------------- */
sect2:
    if (FLAGS & 0x4fff0) {
        total = 0;
        passed = 0;
        sect_start = n;
        lines[n].indent = indent;
        TEXT_LINE_H(sect1, 0, 0x131)         /* the original restarts at sect1 here */
        indent += 0x30;
        CountAttractions(&nattr, &vattr);
        if (FLAGS & 0x4000) {
            total++;
            line_ok = nattr >= g_goal[5];
            if (line_ok) passed++; else failmask |= 0x10;
            BAR_LINE(sect2, 0x132, nattr, g_goal[5], g_goal[6])
        }
        if (FLAGS & 0x8000) {
            total++;
            line_ok = vattr >= g_goal[7];
            if (line_ok) passed++; else failmask |= 0x20;
            BAR_LINE(sect2, 0x133, vattr, g_goal[7], g_goal[8])
        }
        if (FLAGS & 0x40000) {
            v = PercentObjectsLinked();
            total++;
            line_ok = v >= g_goal[9];
            if (line_ok) passed++; else failmask |= 0x40;
            BAR_LINE(sect2, 0x134, v, g_goal[9], g_goal[10])
        }
        if (FLAGS & 0x30) {
            total++;
            line_ok = CountCastles() >= g_goal[0];
            if (line_ok) passed++; else failmask |= 0x80;
            switch ((FLAGS >> 4) & 3) {
            case 1: TEXT_LINE(sect2, line_ok, 0x135) break;
            case 2: TEXT_LINE(sect2, line_ok, 0x136) break;
            case 3: TEXT_LINE(sect2, line_ok, 0x137) break;
            }
        }
        if (FLAGS & 0xc0) {
            total++;
            line_ok = CountDrivingSchools() >= g_goal[1];
            if (line_ok) passed++; else failmask |= 0x100;
            switch ((FLAGS >> 6) & 3) {
            case 1: TEXT_LINE(sect2, line_ok, 0x138) break;
            case 2: TEXT_LINE(sect2, line_ok, 0x139) break;
            case 3: TEXT_LINE(sect2, line_ok, 0x13a) break;
            }
        }
        if (FLAGS & 0x300) {
            total++;
            line_ok = CountLogFlumes() >= g_goal[2];
            if (line_ok) passed++; else failmask |= 0x200;
            switch ((FLAGS >> 8) & 3) {
            case 1: TEXT_LINE(sect2, line_ok, 0x13b) break;
            case 2: TEXT_LINE(sect2, line_ok, 0x13c) break;
            case 3: TEXT_LINE(sect2, line_ok, 0x13d) break;
            }
        }
        if (FLAGS & 0xc00) {
            total++;
            line_ok = CountBoatingSchools() >= g_goal[3];
            if (line_ok) passed++; else failmask |= 0x400;
            switch ((FLAGS >> 10) & 3) {
            case 1: TEXT_LINE(sect2, line_ok, 0x13e) break;
            case 2: TEXT_LINE(sect2, line_ok, 0x13f) break;
            case 3: TEXT_LINE(sect2, line_ok, 0x140) break;
            }
        }
        if (FLAGS & 0x3000) {
            total++;
            line_ok = CountJungleCruises() >= g_goal[4];
            if (line_ok) passed++; else failmask |= 0x800;
            switch ((FLAGS >> 12) & 3) {
            case 1: TEXT_LINE(sect2, line_ok, 0x141) break;
            case 2: TEXT_LINE(sect2, line_ok, 0x142) break;
            case 3: TEXT_LINE(sect2, line_ok, 0x143) break;
            }
        }
        lines[sect_start].ok = (passed == total);
        all_passed += passed;
        indent -= 0x30;
        all_total += total;
        cur.bottom = cur.top + 0x16;
    }

    /* ---- scenery ------------------------------------------------------ */
sect3:
    if (FLAGS & 0x38000000) {
        total = 0;
        passed = 0;
        sect_start = n;
        lines[n].indent = indent;
        TEXT_LINE_H(sect3, line_ok, 0x144)
        indent += 0x30;
        CountScenery(&nscen, &vscen);
        if (FLAGS & 0x8000000) {
            total++;
            line_ok = nscen >= g_goal[25];
            if (line_ok) passed++; else failmask |= 0x1000;
            BAR_LINE(sect3, 0x132, nscen, g_goal[25], g_goal[26])
        }
        /* The 0x20000000 bit of the section guard has no statistic behind
         * it and fail bit 0x4000 is never set: original, left alone. */
        if (FLAGS & 0x10000000) {
            total++;
            line_ok = vscen >= g_goal[27];
            if (line_ok) passed++; else failmask |= 0x2000;
            BAR_LINE(sect3, 0x133, vscen, g_goal[27], g_goal[28])
        }
        lines[sect_start].ok = (passed == total);
        all_total += total;
        indent -= 0x30;
        all_passed += passed;
        cur.bottom = cur.top + 0x16;
    }

    /* ---- food --------------------------------------------------------- */
sect4:
    if (FLAGS & 0xc0000000) {
        total = 0;
        passed = 0;
        sect_start = n;
        lines[n].indent = indent;
        TEXT_LINE_H(sect4, line_ok, 0x145)
        indent += 0x30;
        CountFood(&nfood, &vfood);
        if (FLAGS & 0x40000000) {
            total++;
            line_ok = nfood >= g_goal[31];
            if (line_ok) passed++; else failmask |= 0x8000;
            BAR_LINE(sect4, 0x132, nfood, g_goal[31], g_goal[32])
        }
        if (FLAGS & 0x80000000) {
            total++;
            line_ok = vfood >= g_goal[33];
            if (line_ok) passed++; else failmask |= 0x10000;
            BAR_LINE(sect4, 0x133, vfood, g_goal[33], g_goal[34])
        }
        lines[sect_start].ok = (passed == total);
        all_total += total;
        indent -= 0x30;
        all_passed += passed;
        cur.bottom = cur.top + 0x16;
    }

    /* ---- shops -------------------------------------------------------- */
sect5:
    if (FLAGS & 0x30000) {
        total = 0;
        passed = 0;
        sect_start = n;
        lines[n].indent = indent;
        TEXT_LINE_H(sect5, line_ok, 0x146)
        indent += 0x30;
        CountShops(&nshop, &vshop);
        if (FLAGS & 0x10000) {
            total++;
            line_ok = nshop >= g_goal[13];
            if (line_ok) passed++; else failmask |= 0x20000;
            BAR_LINE(sect5, 0x132, nshop, g_goal[13], g_goal[14])
        }
        if (FLAGS & 0x20000) {
            total++;
            line_ok = vshop >= g_goal[15];
            if (line_ok) passed++; else failmask |= 0x40000;
            BAR_LINE(sect5, 0x133, vshop, g_goal[15], g_goal[16])
        }
        lines[sect_start].ok = (passed == total);
        all_total += total;
        indent -= 0x30;
        all_passed += passed;
        cur.bottom = cur.top + 0x16;
    }

    /* ---- visitors ----------------------------------------------------- */
sect6:
    if (FLAGS & 0x5080000) {
        total = 0;
        passed = 0;
        sect_start = n;
        lines[n].indent = indent;
        TEXT_LINE_H(sect6, line_ok, 0x147)
        indent += 0x30;
        CountVisitors(&nvis, &vmood, &vages);
        if (FLAGS & 0x80000) {
            total++;
            line_ok = nvis >= g_goal[11];
            if (line_ok) passed++; else failmask |= 0x80000;
            /* Original bug: this statistic advances the report's y by a line
             * but never writes one, so it leaves a blank gap. */
            cur.top += 0x18;
        }
        if (FLAGS & 0x1000000) {
            total++;
            line_ok = vmood >= g_goal[17];
            if (line_ok) passed++; else failmask |= 0x100000;
            BAR_LINE(sect6, 0x148, vmood, g_goal[17], g_goal[18])
        }
        if (FLAGS & 0x4000000) {
            total++;
            /* Original bug: graded against the mood goal, not its own. */
            line_ok = vages >= g_goal[17];
            if (line_ok) passed++; else failmask |= 0x200000;
            BAR_LINE(sect6, 0x149, vages, g_goal[17], g_goal[18])
        }
        lines[sect_start].ok = (passed == total);
        all_total += total;
        indent -= 0x30;
        all_passed += passed;
        cur.bottom = cur.top + 0x16;
    }

    /* ---- the park at work --------------------------------------------- */
sect7:
    if (FLAGS & 0xe00000) {
        total = 0;
        passed = 0;
        sect_start = n;
        lines[n].indent = indent;
        TEXT_LINE_H(sect7, line_ok, 0x14a)
        indent += 0x30;
        if (FLAGS & 0x200000) {
            total++;
            line_ok = g_num_visitors >= g_goal[19];
            if (line_ok) passed++; else failmask |= 0x400000;
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
            line_ok = nrun >= g_goal[21];
            if (line_ok) passed++; else failmask |= 0x800000;
            BAR_LINE(sect7, 0x14c, nrun, g_goal[21], g_goal[22])
        }
        if (FLAGS & 0x800000) {
            v = MapCellCount();
            total++;
            line_ok = v >= g_goal[23];
            if (line_ok) passed++; else failmask |= 0x1000000;
            BAR_LINE(sect7, 0x14d, v, g_goal[23], g_goal[24])
        }
        lines[sect_start].ok = (passed == total);
        all_total += total;
        indent -= 0x30;
        all_passed += passed;
        cur.bottom = cur.top + 0x16;
    }

    /* ---- the advice ---------------------------------------------------- */
    /* Unguarded: every report ends with a verdict line and one piece of
     * advice per failed statistic.  line_ok is the bullet form here: -2 the
     * verdict/plain bullet, -1 a piece of advice, -3 its continuation. */
    cur.bottom = cur.top + 0x16;
sect8:
    sect_start = n;
    lines[n].indent = indent;
    TEXT_LINE8(sect8, -2, 0x230)
    indent += 0x30;
    if (all_passed < all_total / 2) {
        TEXT_LINE8(sect8, -2, 0x14f)
        NARR(0x14f)
        TEXT_LINE8(sect8, -2, 0x150)
        NARR(0x150)
    } else if (all_passed < all_total) {
        TEXT_LINE8(sect8, -2, 0x151)
        NARR(0x151)
        TEXT_LINE8(sect8, -2, 0x150)
    } else {
        TEXT_LINE8(sect8, -2, 0x153)
        NARR(0x153)
        TEXT_LINE8(sect8, -2, 0x154)
    }
    if (failmask & 0x1) {
        TEXT_LINE8(sect8, -1, 0x155)
        NARR(0x155)
    }
    if (failmask & 0x2) {
        TEXT_LINE8(sect8, -1, 0x156)
        NARR(0x156)
    }
    if (failmask & 0x4) {
        TEXT_LINE8(sect8, -1, 0x157)
        NARR(0x157)
    }
    if (failmask & 0x8) {
        TEXT_LINE8(sect8, -1, 0x158)
        NARR(0x158)
    }
    switch (failmask & 0x30) {
    case 0x10:
        TEXT_LINE8(sect8, -1, 0x159)
        NARR(0x159)
        break;
    case 0x20:
        TEXT_LINE8(sect8, -1, 0x15a)
        NARR(0x15a)
        break;
    case 0x30:
        TEXT_LINE8(sect8, -1, 0x15b)
        NARR(0x15b)
        TEXT_LINE8(sect8, -3, 0x133)
        break;
    }
    if (failmask & 0x40) {
        TEXT_LINE8(sect8, -1, 0x15c)
        NARR(0x15c)
        TEXT_LINE8(sect8, -3, 0x15d)
    }
    if (failmask & 0x80) {
        TEXT_LINE8(sect8, -1, 0x15e)
        NARR(0x15e)
    }
    if (failmask & 0x100) {
        TEXT_LINE8(sect8, -1, 0x15f)
        NARR(0x15f)
    }
    if (failmask & 0x200) {
        TEXT_LINE8(sect8, -1, 0x160)
        NARR(0x160)
    }
    if (failmask & 0x400) {
        TEXT_LINE8(sect8, -1, 0x161)
        NARR(0x161)
    }
    if (failmask & 0x800) {
        TEXT_LINE8(sect8, -1, 0x162)
        NARR(0x162)
    }
    switch (failmask & 0x3000) {
    case 0x1000:
        TEXT_LINE8(sect8, -1, 0x163)
        NARR(0x163)
        break;
    case 0x2000:
        TEXT_LINE8(sect8, -1, 0x164)
        NARR(0x164)
        break;
    case 0x3000:
        TEXT_LINE8(sect8, -1, 0x165)
        NARR(0x165)
        break;
    }
    switch (failmask & 0x18000) {
    case 0x8000:
        TEXT_LINE8(sect8, -1, 0x166)
        NARR(0x166)
        break;
    case 0x10000:
        TEXT_LINE8(sect8, -1, 0x167)
        NARR(0x167)
        break;
    case 0x18000:
        TEXT_LINE8(sect8, -1, 0x168)
        NARR(0x168)
        break;
    }
    switch (failmask & 0x60000) {
    case 0x20000:
        TEXT_LINE8(sect8, -1, 0x169)
        NARR(0x169)
        break;
    case 0x40000:
        TEXT_LINE8(sect8, -1, 0x16a)
        NARR(0x16a)
        break;
    case 0x60000:
        TEXT_LINE8(sect8, -1, 0x16b)
        NARR(0x16b)
        TEXT_LINE8(sect8, -3, 0x231)
        break;
    }
    if (failmask & 0x80000) {
        TEXT_LINE8(sect8, -1, 0x16c)
        NARR(0x16c)
    }
    if (failmask & 0x100000) {
        TEXT_LINE8(sect8, -1, 0x16d)
        NARR(0x16d)
    }
    /* String id 0x16e is skipped: original. */
    if (failmask & 0x200000) {
        TEXT_LINE8(sect8, -1, 0x16f)
        NARR(0x16f)
    }
    if (failmask & 0x400000) {
        TEXT_LINE8(sect8, -1, 0x170)
        NARR(0x170)
    }
    if (failmask & 0x800000) {
        TEXT_LINE8(sect8, -1, 0x171)
        NARR(0x171)
        TEXT_LINE8(sect8, -3, 0x172)
    }
    if (failmask & 0x1000000) {
        TEXT_LINE8(sect8, -1, 0x173)
        NARR(0x173)
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
            BUF_LINE8(sect9, -2)
            NARR(0x235)
            NARR(v + 0x514)
            NARR(0x236)
            TEXT_LINE_NOY8(sect9, -2, 0x236)
        } else if (v > 0) {
            TEXT_LINE_NOY8(sect9, -2, 0x514)
            NARR(0x514)
            TEXT_LINE_NOY8(sect9, -2, 0x236)
        } else {
            TEXT_LINE_NOY8(sect9, -2, 0x237)
            NARR(0x237)
            TEXT_LINE_NOY8(sect9, -2, 0x238)
        }
    }
    indent -= 0x30;
    cur.bottom = cur.top + 0x16;
sect9:
    sect_start = n;
    lines[n].indent = indent;
    TEXT_LINE_H(sect9, -2, 0x174)
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

    goto build_done;
    /* No rew_sect1: section one is one line long, so its only page check is
     * the head's, and the head's arm is the specialised one above. */
rew_sect2:
    page_start = sect_start;
    n = sect_start;
    indent = lines[sect_start].indent;
    g_report_pages++;
    goto sect2;
rew_sect3:
    page_start = sect_start;
    n = sect_start;
    indent = lines[sect_start].indent;
    g_report_pages++;
    goto sect3;
rew_sect4:
    page_start = sect_start;
    n = sect_start;
    indent = lines[sect_start].indent;
    g_report_pages++;
    goto sect4;
rew_sect5:
    page_start = sect_start;
    n = sect_start;
    indent = lines[sect_start].indent;
    g_report_pages++;
    goto sect5;
rew_sect6:
    page_start = sect_start;
    n = sect_start;
    indent = lines[sect_start].indent;
    g_report_pages++;
    goto sect6;
rew_sect7:
    page_start = sect_start;
    n = sect_start;
    indent = lines[sect_start].indent;
    g_report_pages++;
    goto sect7;
rew_sect8:
    page_start = sect_start;
    n = sect_start;
    indent = lines[sect_start].indent;
    g_report_pages++;
    goto sect8;
rew_sect9:
    page_start = sect_start;
    n = sect_start;
    indent = lines[sect_start].indent;
    g_report_pages++;
    goto sect9;
build_done:
    /* Only the lines up to the last hint are kept. */
    if (nhint == 0)
        n--;
    cur.bottom = cur.top + 0x16;
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

        cur = box;

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
                /* Top and bottom FIRST: with the two constants written first VC6
                 * holds `cur.top + 8` in a register that the argument pushes then
                 * clobber and spills it, which gives `bar` a stack home of its own
                 * (`_bar$` stops sharing `_box$`).  The original has no `bar` slot
                 * at all -- it builds the rectangle straight onto the pushed
                 * arguments at 0x0044d9d5 -- and this order reproduces that. */
                bar.top = cur.top;
                bar.bottom = cur.top + 8;
                bar.left = 0x126;
                bar.right = 0x1a4;
                DrawAppraisalBar(bar, lines[i].value, lines[i].range, lines[i].mark);
            }
            cur.top += 0x18;
            i++;
        }

        if (g_report_page_turned)
            g_report_page_turned = 0;
        if (narr_cur >= nnarr) {
            if (!IsNarrationPlaying())
                UpdateHelpBar();
        } else if (!IsNarrationPlaying() && narr[narr_cur] != -1) {
            sprintf(namebuf, "TEXT%04d.WAV", narr[narr_cur]);
            narr_cur++;
            PauseCurrentTrack();
            PlayNarrationFile(namebuf);
            ResumeCurrentTrack();
        }

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
