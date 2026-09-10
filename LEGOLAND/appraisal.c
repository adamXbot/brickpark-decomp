/* LEGOLAND -- scope Y: the appraisal report screen's helper tier.
 *
 * The screen itself (0x004453a0, 8,085 instructions) is not in this scope;
 * everything here is what it calls -- the five attraction counters it puts
 * in the report, the map's cell count, its sprite loaders and the per-line
 * renderers and page buttons.
 *
 * VC6 SP3 /O2 /Gy /Gd. Types describe only the fields used here; names are
 * ours where the brief left placeholders, addresses are load-bearing.
 */

/* ---- the object database (eventtick2.c's Elem / ObjDef, fields used here) - */
typedef struct Elem   Elem;
typedef struct ObjDef ObjDef;

struct ObjDef {
    ObjDef*        next;              /* +0x00  the class list at 0x00669240 */
    void*          instances;         /* +0x04 */
    int            count;             /* +0x08  how many are in the park */
    int            origin_x;          /* +0x0c  the class's cell offset */
    int            origin_y;          /* +0x10 */
    char           pad14[0x20 - 0x14];
    unsigned short type;              /* +0x20  1,3 attraction; 2 scenery; 4 food; 5 shop */
    char           pad22[0x78 - 0x22];
    const char*    name;              /* +0x78 */
    char           pad7c[0xc0 - 0x7c];
    /* The class's "how many of me are there" callback and its argument. */
    int   (*loop_size)(Elem*, int);   /* +0xc0 */
    Elem* elem;                       /* +0xc4 */
};

/* A placed object, as the link walk reads it. */
typedef struct Cell {
    Elem*          elem;              /* +0x00 */
    unsigned char  x, y;              /* +0x04 */
    char           pad06[0x0c - 0x06];
    unsigned short flags;             /* +0x0c  0x80: a real object, not scenery fill */
} Cell;

/* A visitor, as the mood walk reads it. */
typedef struct Bloke {
    struct Bloke*  next;              /* +0x00 */
    char           pad04[0x7a - 0x04];
    short          mood;              /* +0x7a */
} Bloke;

struct Elem {
    const char* name;                 /* +0x00 */
    void*       image;                /* +0x04 */
    int         flags;                /* +0x08  bit 0: the class is loaded */
    ObjDef*     data;                 /* +0x0c */
};

/* The global game record; only the map's cell counts are read here. */
typedef struct MapHdr {
    char           pad00[0x14];
    unsigned short width;             /* +0x14 */
    unsigned short height;            /* +0x16 */
} MapHdr;

typedef struct Sprite Sprite;
typedef struct Icon   Icon;

/* An icon: its box, its click handler and its flags. 0x400 is "disabled". */
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

extern MapHdr* g_map;                                       /* 0x004bcbf4 */
extern ObjDef* g_objdef_head;                               /* 0x00669240 ObjectClassList */
extern Bloke*  g_people_head;                               /* 0x0066b574 */
/* The 22 class names that are parts of a ride, not attractions of their own. */
extern const char* g_ride_part_names[22];                   /* 0x004b7e9c */
/* The two upper mood thresholds (workers.c's HAPPINESS_ENV rates; simcore2.c
 * calls 0x00832934 g_mood_high). */
extern int     g_rate_t2;                                   /* 0x00832934 */
extern int     g_rate_t3;                                   /* 0x00832938 */

extern Elem*   ElemID(const char* name);                    /* 0x0047b3f0 */
extern Sprite* LoadSprite(const char* name, int mode);      /* 0x00497ab0 */
extern int     sprintf(char* buf, const char* fmt, ...);    /* 0x0049e573 (CRT) */
extern int     LookupNamedIndex(const char* name, const char** table, int n); /* 0x004781b0 */
extern int     PrintSprite(Sprite* s, int x, int y, int mode, void* ctx);     /* 0x004853a0 */
extern void    PushRenderingStatusAndLockVideoSurface(void);                  /* 0x00463fc0 */
extern void    PopRenderingStatus(void);                                      /* 0x004641f0 */
extern signed char GetBlokeAgeGroup(Bloke* b);                                /* 0x0044eb10 */
extern Cell*   GetFirstRenderObject(void);                  /* 0x0045a850 */
extern Cell*   GetNextRenderObject(Cell* c);                /* 0x0045a8b0 */
extern void    RefreshEntranceTile(int force);              /* 0x00482b20 */
extern int     TileJoinsPathNetwork(Pos* pos);              /* 0x00482b60 */
#ifndef LEGOLAND_PORTABLE
extern int     DBPrintf(const char* fmt, ...);              /* 0x00453a20 */
#else
extern void DBPrintf(const char* fmt, ...);              /* 0x00453a20 */
#endif
extern int     GetNearestColour(int r, int g, int b);       /* 0x0044e6c0 */
extern int     RenderBlock(int x, int y, int w, int h, int colour); /* 0x004890c0 */

/* The report screen's sprites (mapscreen2.c names the page buttons). */
extern Sprite* g_rep_bar;             /* 0x0081c028 App_bar.lls */
extern Sprite* g_rep_next;            /* 0x0081c02c NextPage.lls */
extern Sprite* g_rep_barmarker;       /* 0x0081c030 App_barmarker.lls */
extern Sprite* g_rep_next_lit;        /* 0x0081c034 NextPageLit.lls */
/* One 3x5 block: the tick, cross and bullet marks, five steps each. VC6
 * strength-reduces the loop below onto its middle row, which is what puts
 * the neighbours at -0x14 and +0x14 in the original. */
extern Sprite* g_rep_mark[3][5];      /* 0x0081c040 App_tick/cross/bullet%d.lls */
extern Sprite* g_rep_prev;            /* 0x0081c080 PreviousPage.lls */
extern Sprite* g_rep_prev_lit;        /* 0x0081c084 PreviousPageLit.lls */
extern Sprite* g_backdrop;            /* 0x00810148 AppraisalBK.lls */

/* The report screen's own state. */
extern int   g_report_open;           /* 0x0081c038  the screen is up */
extern int   g_report_page_turned;    /* 0x0081c07c */
extern int   g_report_pages;          /* 0x006660a0  pages in this report */
extern int   g_report_page;           /* 0x006660a4  the page on screen */
extern Icon* g_report_next_icon;      /* 0x006660a8 */
extern Icon* g_report_prev_icon;      /* 0x006660ac */

extern IconInputFn g_icon_handler1;   /* 0x006687bc */
extern IconInputFn g_icon_handler2;   /* 0x006687c0 */
extern int   g_6687b0;                /* 0x006687b0  hold-off frame counter */
extern Pos   g_gfx_point;             /* 0x00813a44  the cursor */
extern void* g_snd_click;             /* 0x004b92c0  the UI click sample */

extern Icon* LoadSpriteIcon(const char* name, int mode, int x, int y, int group); /* 0x0046d7b0 */
extern Icon* InsertIcon(short x, short y, unsigned short group, Sprite* s);       /* 0x0046d6c0 */
extern void  SetIconSprite(Icon* p, Sprite* s);            /* 0x0046d680 */
extern void  RemoveIconGroup(unsigned short group);        /* 0x0046d520 */
extern int   UnreferenceSprite(Sprite* s);                        /* 0x00497bd0 */
extern char* GetString(int id);                            /* 0x00498f50 */
extern void  PlayInstanceOfSample(void* sample, int a, int b, void* src); /* 0x00496d20 */
extern void  PauseCurrentTrack(void);                      /* 0x00498920 */

/* ========================================================================== */
/* The five attraction counters.                                              */
/* ========================================================================== */

/* Each asks the class database for the class, and, when the class is
 * loaded, asks the class itself how many of it the park holds. */

// FUNCTION: LEGOLAND 0x004442c0
int CountDrivingSchools(void)
{
    Elem* e = ElemID("DRIVING SCHOOL");

    if (e->flags & 1)
        return e->data->loop_size(e->data->elem, 0);
    return 0;
}

// FUNCTION: LEGOLAND 0x004442f0
int CountBoatingSchools(void)
{
    Elem* e = ElemID("BOATING SCHOOL");

    if (e->flags & 1)
        return e->data->loop_size(e->data->elem, 0);
    return 0;
}

// FUNCTION: LEGOLAND 0x00444320
int CountCastles(void)
{
    Elem* e = ElemID("CASTLE OBJ");

    if (e->flags & 1)
        return e->data->loop_size(e->data->elem, 0);
    return 0;
}

// FUNCTION: LEGOLAND 0x00444350
int CountLogFlumes(void)
{
    Elem* e = ElemID("LOG FLUME ENTRANCE");

    if (e->flags & 1)
        return e->data->loop_size(e->data->elem, 0);
    return 0;
}

// FUNCTION: LEGOLAND 0x00444380
int CountJungleCruises(void)
{
    Elem* e = ElemID("JUNGLE CRUISE");

    if (e->flags & 1)
        return e->data->loop_size(e->data->elem, 0);
    return 0;
}

/* The map's cell count, walked rather than multiplied. */
// FUNCTION: LEGOLAND 0x004636c0
int MapCellCount(void)
{
    int n = 0;
    int y, x;

    for (y = 0; y < g_map->height; y++) {
        for (x = 0; x < g_map->width; x++)
            n++;
    }
    return n;
}

/* ========================================================================== */
/* The screen's sprites.                                                      */
/* ========================================================================== */

/* The five-step tick, cross and bullet marks, plus the bar and its marker. */
// FUNCTION: LEGOLAND 0x004449b0
void LoadAppraisalTickSprites(void)
{
    char name[0x20] = {0};
    int  i;

    for (i = 0; i < 5; i++) {
        sprintf(name, "App_tick%d.lls", i);
        g_rep_mark[0][i] = LoadSprite(name, 4);
        sprintf(name, "App_cross%d.lls", i);
        g_rep_mark[1][i] = LoadSprite(name, 4);
        sprintf(name, "App_bullet%d.lls", i);
        g_rep_mark[2][i] = LoadSprite(name, 4);
    }
    g_rep_barmarker = LoadSprite("App_barmarker.lls", 4);
    g_rep_bar = LoadSprite("App_bar.lls", 4);
}

/* Draw one of the three marks: a tick for a passed line, a cross for a
 * failed one, a bullet for a line that is only being counted. The bullet
 * sits five pixels in from the other two. */
// FUNCTION: LEGOLAND 0x00444b70
void BlitAppraisalSprite(int x, int y, int kind, int step)
{
    PushRenderingStatusAndLockVideoSurface();
    if (kind == 1)
        PrintSprite(g_rep_mark[0][step], x, y, 0, 0);
    else if (kind == 0)
        PrintSprite(g_rep_mark[1][step], x, y, 0, 0);
    else if (kind == -1)
        PrintSprite(g_rep_mark[2][step], x + 5, y + 5, 0, 0);
    PopRenderingStatus();
}

/* ========================================================================== */
/* What the park holds: a count and a variety for each report line.           */
/* ========================================================================== */

/* Attractions: class types 1 and 3. */
// FUNCTION: LEGOLAND 0x00444bf0
void CountAttractions(int* num, int* variety)
{
    ObjDef* d = g_objdef_head;

    *num = 0;
    *variety = 0;
    while (d) {
        int n = d->count;

        if (n && (d->type == 1 || d->type == 3)) {
            *num += n;
            (*variety)++;
        }
        d = d->next;
    }
}

/* A class that is part of a ride (its track, its water, its dummy) rather
 * than an attraction in its own right. */
// FUNCTION: LEGOLAND 0x00444c40
int IsRidePartClass(ObjDef* d)
{
    return LookupNamedIndex(d->elem->name, g_ride_part_names, 22) >= 0;
}

/* Scenery: class type 2, less the ride parts. */
// FUNCTION: LEGOLAND 0x00444c70
void CountScenery(int* num, int* variety)
{
    ObjDef* d = g_objdef_head;

    *num = 0;
    *variety = 0;
    while (d) {
        if (d->count && d->type == 2 && !IsRidePartClass(d)) {
            *num += d->count;
            (*variety)++;
        }
        d = d->next;
    }
}

/* Food: class type 4. */
// FUNCTION: LEGOLAND 0x00444cd0
void CountFood(int* num, int* variety)
{
    ObjDef* d = g_objdef_head;

    *num = 0;
    *variety = 0;
    while (d) {
        int n = d->count;

        if (n && d->type == 4) {
            *num += n;
            (*variety)++;
        }
        d = d->next;
    }
}

/* Shops: class type 5. */
// FUNCTION: LEGOLAND 0x00444d20
void CountShops(int* num, int* variety)
{
    ObjDef* d = g_objdef_head;

    *num = 0;
    *variety = 0;
    while (d) {
        int n = d->count;

        if (n && d->type == 5) {
            *num += n;
            (*variety)++;
        }
        d = d->next;
    }
}

/* The visitors: how many there are, how many mood thresholds they are over
 * between them, and their age-group score (4 minus each visitor's group
 * index). */
// FUNCTION: LEGOLAND 0x00444d70
void CountVisitors(int* visitors, int* mood, int* ages)
{
    Bloke* b = g_people_head;

    *visitors = 0;
    *mood = 0;
    *ages = 0;
    while (b) {
        (*visitors)++;
        if (b->mood > g_rate_t2)
            (*mood)++;
        if (b->mood > g_rate_t3)
            (*mood)++;
        *ages += 4 - GetBlokeAgeGroup(b);
        b = b->next;
    }
}

/* ========================================================================== */
/* The screen's two page buttons and its Go Back icon.                        */
/* ========================================================================== */

void UpdateAppraisalPageButtons(void);   /* 0x00445310, below */

/* Go Back: leave the report. Also the handler the next-page button runs
 * once there is no next page. */
// FUNCTION: LEGOLAND 0x00444eb0
char AppraisalGoBack(Icon* icon, int event, int x, int y)
{
    if (event & 2) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_report_open = 0;
        g_report_next_icon = 0;
        g_report_prev_icon = 0;
    }
    return 1;
}

/* Next page. The lit sprite is never drawn: the guard below is the
 * original's, and `!icon->flags & 0x400` parses as `(!icon->flags) & 0x400`,
 * which is always zero. */
// FUNCTION: LEGOLAND 0x00444ef0
char AppraisalNextPage(Icon* icon, int event, int x, int y)
{
    if (g_report_next_icon) {
        if (!g_report_next_icon->flags & 0x400)
            SetIconSprite(g_report_next_icon, g_rep_next_lit);
        if (event & 2) {
            if (g_report_next_icon->flags & 0x400)
                return AppraisalGoBack(0, event, 0, 0);
            g_report_page_turned = 1;
            PlayInstanceOfSample(g_snd_click, 0, 1, 0);
            PauseCurrentTrack();
            g_6687b0 = 4;
            if (g_report_page < g_report_pages - 1)
                g_report_page++;
            UpdateAppraisalPageButtons();
        }
    }
    return 1;
}

/* Previous page. */
// FUNCTION: LEGOLAND 0x00444f90
char AppraisalPrevPage(Icon* icon, int event, int x, int y)
{
    SetIconSprite(g_report_prev_icon, g_rep_prev_lit);
    if (event & 2) {
        g_report_page_turned = 1;
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        if (g_report_page)
            g_report_page--;
        PauseCurrentTrack();
        g_6687b0 = 4;
        UpdateAppraisalPageButtons();
    }
    return 1;
}

/* Drop every sprite the screen loaded and take its icons away. */
// FUNCTION: LEGOLAND 0x00445000
void FreeAppraisalScreenSprites(void)
{
    int i;

    for (i = 0; i < 5; i++) {
        if (g_rep_mark[0][i]) {
            UnreferenceSprite(g_rep_mark[0][i]);
            g_rep_mark[0][i] = 0;
        }
        if (g_rep_mark[1][i]) {
            UnreferenceSprite(g_rep_mark[1][i]);
            g_rep_mark[1][i] = 0;
        }
        if (g_rep_mark[2][i]) {
            UnreferenceSprite(g_rep_mark[2][i]);
            g_rep_mark[2][i] = 0;
        }
    }
    if (g_rep_barmarker) {
        UnreferenceSprite(g_rep_barmarker);
        g_rep_barmarker = 0;
    }
    if (g_rep_bar) {
        UnreferenceSprite(g_rep_bar);
        g_rep_bar = 0;
    }
    if (g_backdrop) {
        UnreferenceSprite(g_backdrop);
        g_backdrop = 0;
    }
    if (g_rep_next) {
        UnreferenceSprite(g_rep_next);
        g_rep_next = 0;
    }
    if (g_rep_next_lit) {
        UnreferenceSprite(g_rep_next_lit);
        g_rep_next_lit = 0;
    }
    if (g_rep_prev) {
        UnreferenceSprite(g_rep_prev);
        g_rep_prev = 0;
    }
    if (g_rep_prev_lit) {
        UnreferenceSprite(g_rep_prev_lit);
        g_rep_prev_lit = 0;
    }
    RemoveIconGroup(1);
}

/* Unlight either page button the cursor has left. */
// FUNCTION: LEGOLAND 0x00445100
void UnlightAppraisalPageButtons(void)
{
    if (g_gfx_point.x < g_report_next_icon->x
        || g_gfx_point.x > g_report_next_icon->x + g_report_next_icon->w
        || g_gfx_point.y < g_report_next_icon->y
        || g_gfx_point.y > g_report_next_icon->y + g_report_next_icon->h)
        SetIconSprite(g_report_next_icon, g_rep_next);
    if (g_gfx_point.x < g_report_prev_icon->x
        || g_gfx_point.x > g_report_prev_icon->x + g_report_prev_icon->w
        || g_gfx_point.y < g_report_prev_icon->y
        || g_gfx_point.y > g_report_prev_icon->y + g_report_prev_icon->h)
        SetIconSprite(g_report_prev_icon, g_rep_prev);
}

/* Load the screen's own sprites and put its three icons up. */
// FUNCTION: LEGOLAND 0x00445190
void LoadAppraisalScreenSprites(void)
{
    Icon* icon;

    g_rep_next = LoadSprite("NextPage.lls", 4);
    g_rep_next_lit = LoadSprite("NextPageLit.lls", 4);
    g_rep_prev = LoadSprite("PreviousPage.lls", 4);
    g_rep_prev_lit = LoadSprite("PreviousPageLit.lls", 4);
    g_backdrop = LoadSprite("AppraisalBK.lls", 0);

    icon = LoadSpriteIcon("GoBack_on_Report.lls", 4, 0x1fb, 0x161, 1);
    icon->text_id = 0xde;
    icon->text = GetString(0xde);
    icon->flags |= 0x6002;
    icon->handler = AppraisalGoBack;
    g_icon_handler2 = AppraisalGoBack;

    g_report_next_icon = InsertIcon(0x1b9, 0x1ae, 1, g_rep_next);
    g_report_next_icon->text_id = 0xdc;
    g_report_next_icon->text = GetString(0xdc);
    g_report_next_icon->flags |= 0x2000;
    g_report_next_icon->flags |= 0x4002;
    g_report_next_icon->handler = AppraisalNextPage;
    g_icon_handler1 = AppraisalNextPage;

    g_report_prev_icon = InsertIcon(6, 0x1ae, 1, g_rep_prev);
    g_report_prev_icon->text_id = 0xdd;
    g_report_prev_icon->text = GetString(0xdd);
    g_report_prev_icon->flags |= 0x2000;
    g_report_prev_icon->flags |= 0x4002;
    g_report_prev_icon->handler = AppraisalPrevPage;
    UpdateAppraisalPageButtons();
    g_report_open = 1;
}

/* Enable or grey each page button for the page we are on. */
// FUNCTION: LEGOLAND 0x00445310
void UpdateAppraisalPageButtons(void)
{
    if (g_report_pages > 1 && g_report_page < g_report_pages - 1) {
        g_report_next_icon->handler = AppraisalNextPage;
        g_report_next_icon->flags &= ~0x400;
    } else {
        g_report_next_icon->handler = 0;
        g_report_next_icon->flags |= 0x400;
    }
    if (g_report_page != 0) {
        g_report_prev_icon->handler = AppraisalPrevPage;
        g_report_prev_icon->flags &= ~0x400;
    } else {
        g_report_prev_icon->handler = 0;
        g_report_prev_icon->flags |= 0x400;
    }
}

/* What share of the park's attractions, food and shops the paths reach.
 * An empty park counts as fully linked. */
// FUNCTION: LEGOLAND 0x00444df0
int PercentObjectsLinked(void)
{
    Cell* c = GetFirstRenderObject();
    int   linked = 0;
    int   total = 0;

    RefreshEntranceTile(1);
    while (c) {
        if (c->flags & 0x80) {
            ObjDef* d = c->elem->data;

            if (d->type == 1 || d->type == 4 || d->type == 5) {
                Pos pos;

                pos.x = c->x + d->origin_x;
                pos.y = c->y + d->origin_y;
                total++;
                if (TileJoinsPathNetwork(&pos))
                    linked++;
                else
                    DBPrintf("Unlinked Object %s\n", d->name);
            }
        }
        c = GetNextRenderObject(c);
    }
    if (total != 0)
        return linked * 100 / total;
    return 100;
}

typedef struct AppraisalBox { int left, top, right, bottom; } AppraisalBox;

/* Draw a report bar and target marker. A negative range mirrors the value
 * and target; values above the range are capped before that reflection.
 * Original behavior: no lower clamp and no guard against a zero range.
 * The box is passed as one aggregate: this keeps its dead right-coordinate
 * parameter slot unavailable for the shared top+2 temporary. Four scalar
 * coordinates remove the original's four-byte local frame (96 vs 98 insns).
 * Assign negative in both arms so the zero stays below the sign test; keep
 * the green branch first to preserve the original's jl to the red branch. */
// FUNCTION: LEGOLAND 0x00444a70
void DrawAppraisalBar(AppraisalBox box, int value, int range, int mark)
{
    int negative;
    int colour;
    int width;

    if (range < 0) {
        negative = 1;
        range = -range;
        mark = range - mark;
    } else
        negative = 0;
    if (value > range)
        value = range;
    if (negative)
        value = range - value;
    if (value >= mark)
        colour = GetNearestColour(0, 0xff, 0);
    else
        colour = GetNearestColour(0xff, 0, 0);

    width = (box.right - box.left) * value / range;
    PrintSprite(g_rep_bar, box.left, box.top, 0, 0);
    RenderBlock(box.left + 3, box.top + 2, width - 2, 1, colour);
    RenderBlock(box.left + 2, box.top + 3, width, box.bottom - box.top - 1, colour);
    PrintSprite(g_rep_barmarker, (box.right - box.left - 2) * mark / range + box.left + 2,
                box.top + 2, 0, 0);
}
