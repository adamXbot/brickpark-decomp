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
    char           pad0c[0x20 - 0x0c];
    unsigned short type;              /* +0x20  1,3 attraction; 2 scenery; 4 food; 5 shop */
    char           pad22[0xc0 - 0x22];
    /* The class's "how many of me are there" callback and its argument. */
    int   (*loop_size)(Elem*, int);   /* +0xc0 */
    Elem* elem;                       /* +0xc4 */
};

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
 * between them, and their age spread (a four-year-old scores 4, and so on
 * down). */
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
