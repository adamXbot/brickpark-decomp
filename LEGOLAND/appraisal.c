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
    char  pad00[0xc0];
    /* The class's "how many of me are there" callback and its argument. */
    int   (*loop_size)(Elem*, int);   /* +0xc0 */
    Elem* elem;                       /* +0xc4 */
};

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

extern Elem*   ElemID(const char* name);                    /* 0x0047b3f0 */
extern Sprite* LoadSprite(const char* name, int mode);      /* 0x00497ab0 */
extern int     sprintf(char* buf, const char* fmt, ...);    /* 0x0049e573 (CRT) */

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
