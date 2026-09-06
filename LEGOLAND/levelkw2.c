/* LEGOLAND -- scope S: the level-database keyword tier, part 2.
 *
 * Thirty-seven handlers from the keyword table at 0x004bb6f8 (entries 38..70
 * and 87..90), in address order: the goal keywords REMOVE .. FOREVER, the
 * reward keywords GIVE / TAKE / ADDBRICKS and the level-setup keywords
 * THEMEICON / ADDFLAG / BRIDGES / ENDSCREENS. `ParseKeywordSections`
 * (0x00478280, scope R) calls a handler as `h(argv, argc, arg)`: `argv[0]` is
 * the keyword, `argv[1..argc]` its words, `arg` the value LoadLevelDatabase
 * passed down (0). Every handler is one shape -- give up when no level
 * database is being read, ask `KwLineApplies` whether the keyword is allowed
 * in the current section with at least N words, convert the words (`atoi`,
 * `ElemID`, `LookupNamedIndex`), then hand them to the keyword's constructor
 * (`AddEvent_*`, scope W) or, for the four level-setup keywords when the
 * current section is [INIT], act immediately. Return 1 when handled, 0 when
 * the keyword was skipped or an argument failed to parse.
 *
 * VC6 SP3 /O2 /Gy /Gd. Names are ours (the integrator's provisional readings
 * in docs/SCOPE_S_level_keywords_2.md, scope W's for the constructors, scope
 * X's for the three immediate setters); offsets and addresses are load-
 * bearing. Verification and recovered mechanics: docs/lanes/scope-s.md.
 */

typedef struct LLElem LLElem;

/* ---- globals ------------------------------------------------------------- */
/* A level database is being read (movie3.c's name). */
extern int           g_level_db_active;      /* 0x004bb5b0 */
/* movie3.c's name for 0x00669054; here it is the bit of the section being
 * read (1 = [INIT], 2 = a goal section, 4 = [REWARD]): `KwLineApplies`
 * masks it, and the four level-setup keywords act at once when it is 1. */
extern int           g_level_number;         /* 0x00669054 */
/* The event flags of the section being read (movie3.c's name); every goal
 * constructor takes it as its first, byte-wide argument. */
extern unsigned char g_level_byte_669050;    /* 0x00669050 */

/* The enumerated-word tables `LookupNamedIndex` searches (first named here). */
extern const char* const g_zoning_names[4];  /* 0x004bb5b4  LEGOLAND ADVENTURER CASTLE WESTERN */
extern const char* const g_theme_names[5];   /* 0x004bb5c4  LEGOLAND WESTERN CASTLE ADVENTURER NONE */
extern const char* const g_tab_names[2];     /* 0x004bb5d8  Build Research */
extern const char* const g_mode_names[5];    /* 0x004bb5e0  QUERY BUILD ERASE PATH MAP */

/* ---- scope R's parse primitives (declared, not defined, here) ----------- */
/* 0x004786c0: `(g_level_number & sections) && argc >= need` -- whether the
 * keyword applies in the section being read with enough words. */
extern int   KwLineApplies(char** argv, int argc, int sections, int need);      /* 0x004786c0 */
/* 0x00478690: `argc >= need` (the second half of KwLineApplies, called
 * again by REMOVE); scope R's placeholder name. */
extern int   KwHasArgs(char** argv, int argc, int need);                        /* 0x00478690 */
/* 0x00478700: atoi argv[first..first+3] into rect and order the corners. */
extern void  ParseRectArgs(int* rect, char** argv, int first);                   /* 0x00478700 */
/* 0x004781b0: index of the word in names (NameCompare), or -1. */
extern int   LookupNamedIndex(const char* word, const char* const* names, int count); /* 0x004781b0 */

/* ---- other callees ------------------------------------------------------- */
extern LLElem* ElemID(const char* name);                                         /* 0x0047b3f0 */
extern int   atoi(const char* s);                                                /* 0x004a04b9 (CRT) */
extern int   NameCompare(const char* a, const char* b);                             /* 0x004aab90 (CRT) */
extern char* strcpy(char* dst, const char* src);   /* intrinsic, no call */
extern char* strcat(char* dst, const char* src);   /* intrinsic, no call */
#pragma intrinsic(strcpy, strcat)

/* movie3.c declares this `(int, int)` and passes (0, 0); APPRAISAL passes
 * its state and the text it built. */
extern void  SetLevelGoalState(int state, const char* text);                     /* 0x0044dc70 */
extern void  SetLevelEndSequence(int which, const char* s);                      /* 0x004597e0 */
/* The immediate forms of THEMEICON / ADDFLAG / BRIDGES (scope X's names). */
extern void  SetThemeIcon(int icon, int on);                                     /* 0x00468860 */
extern void  AddLevelFlag(int flag, int on);                                     /* 0x00468890 */
extern void  SetBridges(int count, int on);                                      /* 0x004688f0 */

/* ---- the event constructors (scope W) ------------------------------------ */
/* Goal constructors (kinds 40..69): the section's flags byte first. */
extern void  AddEvent_Remove(unsigned char flags, LLElem* e, int count);         /* 0x0046bf80 */
extern void  AddEvent_Removerange(unsigned char flags, LLElem* e, int hi, int lo); /* 0x0046bfb0 */
extern void  AddEvent_Composite(unsigned char flags, LLElem* e, int count, int extra); /* 0x0046bff0 */
extern void  AddEvent_Loopcomposite(unsigned char flags, LLElem* e, int count);  /* 0x0046c030 */
extern void  AddEvent_Techlevel(unsigned char flags, LLElem* e, int level);      /* 0x0046c060 */
extern void  AddEvent_Parkvisitors(unsigned char flags, int count);              /* 0x0046c090 */
extern void  AddEvent_Ridevisitors(unsigned char flags, LLElem* e, int count);   /* 0x0046c0c0 */
extern void  AddEvent_Riders(unsigned char flags, LLElem* e, int count);         /* 0x0046c0f0 */
extern void  AddEvent_Scenerycoverage(unsigned char flags, int pct);             /* 0x0046c120 */
extern void  AddEvent_Pathscenery(unsigned char flags, int pct);                 /* 0x0046c150 */
extern void  AddEvent_Ridecoverage(unsigned char flags, int pct);                /* 0x0046c180 */
extern void  AddEvent_Shopcoverage(unsigned char flags, int pct);                /* 0x0046c1b0 */
extern void  AddEvent_Foodcoverage(unsigned char flags, int pct);                /* 0x0046c1e0 */
extern void  AddEvent_Totcoverage(unsigned char flags, int pct);                 /* 0x0046c210 */
extern void  AddEvent_Studarea(unsigned char flags, int* rect, int count);       /* 0x0046c240 */
extern void  AddEvent_Save(unsigned char flags, int slot);                       /* 0x0046c290 */
extern void  AddEvent_Happiness(unsigned char flags, int a, int b);              /* 0x0046c2c0 */
extern void  AddEvent_Needgardeners(unsigned char flags, int count);             /* 0x0046c2f0 */
extern void  AddEvent_Needmechanics(unsigned char flags, int count);             /* 0x0046c320 */
extern void  AddEvent_Hunger(unsigned char flags, int a, int b, int plus);       /* 0x0046c350 */
extern void  AddEvent_Fixrides(unsigned char flags, int a, int b);               /* 0x0046c390 */
extern void  AddEvent_Powerrides(unsigned char flags, int count);                /* 0x0046c3c0 */
extern void  AddEvent_Zoning(unsigned char flags, int zone, int count);          /* 0x0046c3f0 */
extern void  AddEvent_Checkflag(unsigned char flags, int flag, int on);          /* 0x0046c420 */
extern void  AddEvent_Selecttheme(unsigned char flags, int theme);               /* 0x0046c450 */
extern void  AddEvent_Selecttab(unsigned char flags, int tab);                   /* 0x0046c480 */
extern void  AddEvent_Selectmode(unsigned char flags, int mode);                 /* 0x0046c4b0 */
extern void  AddEvent_Forever(unsigned char flags);                              /* 0x0046c510 */
/* Step constructors (no flags byte). */
extern void  AddEvent_Themeicon(int icon, int on);                               /* 0x0046bc80 */
extern void  AddEvent_Addflag(int flag, int on);                                 /* 0x0046bcb0 */
extern void  AddEvent_Bridges(int count, int on);                                /* 0x0046bce0 */
extern void  AddEvent_Give(LLElem* e, int popup);                                /* 0x0046b790 */
extern void  AddEvent_Take(LLElem* e);                                           /* 0x0046b7f0 */
extern void  AddEvent_Addbricks(int count);                                      /* 0x0046b820 */

/* ========================================================================= */

/* REMOVE <object> <count>: checks the word count a second time through the
 * primitive KwLineApplies already used. */
// FUNCTION: LEGOLAND 0x00479550
int LevelKw_REMOVE(char** argv, int argc, int arg)
{
    LLElem* e;
    int     n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 2))
        return 0;
    if (!KwHasArgs(argv, argc, 2))
        return 0;
    e = ElemID(argv[1]);
    n = atoi(argv[2]);
    if (e)
        AddEvent_Remove(g_level_byte_669050, e, n);
    return 1;
}

/* REMOVERANGE <object> <lo> [<hi>]: hi defaults to -1 (no upper bound). */
// FUNCTION: LEGOLAND 0x004795c0
int LevelKw_REMOVERANGE(char** argv, int argc, int arg)
{
    LLElem* e;
    int     lo, hi;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 2))
        return 0;
    e  = ElemID(argv[1]);
    lo = atoi(argv[2]);
    if (argc >= 3)
        hi = atoi(argv[3]);
    else
        hi = -1;
    if (e)
        AddEvent_Removerange(g_level_byte_669050, e, hi, lo);
    return 1;
}

/* COMPOSITE <object> <count> [<extra>]: a zero count means one. */
// FUNCTION: LEGOLAND 0x00479640
int LevelKw_COMPOSITE(char** argv, int argc, int arg)
{
    LLElem* e;
    int     n, extra;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 2))
        return 0;
    e = ElemID(argv[1]);
    n = atoi(argv[2]);
    if (n == 0)
        n = 1;
    if (argc >= 3)
        extra = atoi(argv[3]);
    else
        extra = 0;
    if (e)
        AddEvent_Composite(g_level_byte_669050, e, n, extra);
    return 1;
}

/* LOOPCOMPOSITE <object> <count>: a zero count means one. */
// FUNCTION: LEGOLAND 0x004796d0
int LevelKw_LOOPCOMPOSITE(char** argv, int argc, int arg)
{
    LLElem* e;
    int     n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 2))
        return 0;
    e = ElemID(argv[1]);
    n = atoi(argv[2]);
    if (n == 0)
        n = 1;
    if (e)
        AddEvent_Loopcomposite(g_level_byte_669050, e, n);
    return 1;
}

/* TECHLEVEL <object> <level> */
// FUNCTION: LEGOLAND 0x00479740
int LevelKw_TECHLEVEL(char** argv, int argc, int arg)
{
    LLElem* e;
    int     n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 2))
        return 0;
    e = ElemID(argv[1]);
    n = atoi(argv[2]);
    if (e)
        AddEvent_Techlevel(g_level_byte_669050, e, n);
    return 1;
}

/* RESEARCH <object> [<n>]: parses its words and does nothing with them --
 * the keyword is a stub in the shipped game. */
// FUNCTION: LEGOLAND 0x004797b0
int LevelKw_RESEARCH(char** argv, int argc, int arg)
{
    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 1))
        return 0;
    ElemID(argv[1]);
    if (argc >= 2)
        atoi(argv[2]);
    return 1;
}

/* PARKVISITORS <count> */
// FUNCTION: LEGOLAND 0x00479800
int LevelKw_PARKVISITORS(char** argv, int argc, int arg)
{
    int n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 1))
        return 0;
    n = atoi(argv[1]);
    AddEvent_Parkvisitors(g_level_byte_669050, n);
    return 1;
}

/* RIDEVISITORS <ride> <count> */
// FUNCTION: LEGOLAND 0x00479850
int LevelKw_RIDEVISITORS(char** argv, int argc, int arg)
{
    LLElem* e;
    int     n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 2))
        return 0;
    e = ElemID(argv[1]);
    n = atoi(argv[2]);
    if (e)
        AddEvent_Ridevisitors(g_level_byte_669050, e, n);
    return 1;
}

/* RIDERS <ride> <count> */
// FUNCTION: LEGOLAND 0x004798c0
int LevelKw_RIDERS(char** argv, int argc, int arg)
{
    LLElem* e;
    int     n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 2))
        return 0;
    e = ElemID(argv[1]);
    n = atoi(argv[2]);
    if (e)
        AddEvent_Riders(g_level_byte_669050, e, n);
    return 1;
}

/* SCENERYCOVERAGE <percent> */
// FUNCTION: LEGOLAND 0x00479930
int LevelKw_SCENERYCOVERAGE(char** argv, int argc, int arg)
{
    int n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 1))
        return 0;
    n = atoi(argv[1]);
    AddEvent_Scenerycoverage(g_level_byte_669050, n);
    return 1;
}

/* PATHSCENERY <percent> */
// FUNCTION: LEGOLAND 0x00479980
int LevelKw_PATHSCENERY(char** argv, int argc, int arg)
{
    int n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 1))
        return 0;
    n = atoi(argv[1]);
    AddEvent_Pathscenery(g_level_byte_669050, n);
    return 1;
}

/* RIDECOVERAGE <percent> */
// FUNCTION: LEGOLAND 0x004799d0
int LevelKw_RIDECOVERAGE(char** argv, int argc, int arg)
{
    int n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 1))
        return 0;
    n = atoi(argv[1]);
    AddEvent_Ridecoverage(g_level_byte_669050, n);
    return 1;
}

/* SHOPCOVERAGE <percent> */
// FUNCTION: LEGOLAND 0x00479a20
int LevelKw_SHOPCOVERAGE(char** argv, int argc, int arg)
{
    int n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 1))
        return 0;
    n = atoi(argv[1]);
    AddEvent_Shopcoverage(g_level_byte_669050, n);
    return 1;
}

/* FOODCOVERAGE <percent> */
// FUNCTION: LEGOLAND 0x00479a70
int LevelKw_FOODCOVERAGE(char** argv, int argc, int arg)
{
    int n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 1))
        return 0;
    n = atoi(argv[1]);
    AddEvent_Foodcoverage(g_level_byte_669050, n);
    return 1;
}

/* TOTCOVERAGE <percent> */
// FUNCTION: LEGOLAND 0x00479ac0
int LevelKw_TOTCOVERAGE(char** argv, int argc, int arg)
{
    int n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 1))
        return 0;
    n = atoi(argv[1]);
    AddEvent_Totcoverage(g_level_byte_669050, n);
    return 1;
}

/* APPRAISAL [<state> [<text> [<text2>]]] -- [INIT] only: the goal state and
 * "<text>;<text2>" go to SetLevelGoalState at once. The 0x200-byte buffer
 * is initialised from the pooled "" (one byte copied, the rest zeroed). */
// FUNCTION: LEGOLAND 0x00479b10
int LevelKw_APPRAISAL(char** argv, int argc, int arg)
{
    char buf[0x200] = "";
    int  state;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 1, 0))
        return 0;
    if (argc >= 1)
        state = atoi(argv[1]);
    else
        state = 0;
    if (argc >= 2)
        strcpy(buf, argv[2]);
    strcat(buf, ";");
    if (argc >= 3)
        strcat(buf, argv[3]);
    SetLevelGoalState(state, buf);
    return 1;
}

/* STUDAREA <x0> <y0> <x1> <y1> <count> */
// FUNCTION: LEGOLAND 0x00479c40
int LevelKw_STUDAREA(char** argv, int argc, int arg)
{
    int rect[4];
    int n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 5))
        return 0;
    ParseRectArgs(rect, argv, 1);
    n = atoi(argv[5]);
    AddEvent_Studarea(g_level_byte_669050, rect, n);
    return 1;
}

/* SAVE <slot> */
// FUNCTION: LEGOLAND 0x00479cb0
int LevelKw_SAVE(char** argv, int argc, int arg)
{
    int n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 1))
        return 0;
    n = atoi(argv[1]);
    AddEvent_Save(g_level_byte_669050, n);
    return 1;
}

/* HAPPINESS <a> <b> */
// FUNCTION: LEGOLAND 0x00479d00
int LevelKw_HAPPINESS(char** argv, int argc, int arg)
{
    int a, b;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 2))
        return 0;
    a = atoi(argv[1]);
    b = atoi(argv[2]);
    AddEvent_Happiness(g_level_byte_669050, a, b);
    return 1;
}

/* NEEDGARDENERS <count> */
// FUNCTION: LEGOLAND 0x00479d60
int LevelKw_NEEDGARDENERS(char** argv, int argc, int arg)
{
    int n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 1))
        return 0;
    n = atoi(argv[1]);
    AddEvent_Needgardeners(g_level_byte_669050, n);
    return 1;
}

/* NEEDMECHANICS <count> */
// FUNCTION: LEGOLAND 0x00479db0
int LevelKw_NEEDMECHANICS(char** argv, int argc, int arg)
{
    int n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 1))
        return 0;
    n = atoi(argv[1]);
    AddEvent_Needmechanics(g_level_byte_669050, n);
    return 1;
}

/* HUNGER <a> <b> [+]: a third word starting with '+' sets the plus flag. */
// FUNCTION: LEGOLAND 0x00479e00
int LevelKw_HUNGER(char** argv, int argc, int arg)
{
    int a, b, plus;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 2))
        return 0;
    a = atoi(argv[1]);
    b = atoi(argv[2]);
    if (argc >= 3)
        plus = argv[3][0] == '+';
    else
        plus = 0;
    AddEvent_Hunger(g_level_byte_669050, a, b, plus);
    return 1;
}

/* FIXRIDES <a> <b> */
// FUNCTION: LEGOLAND 0x00479e80
int LevelKw_FIXRIDES(char** argv, int argc, int arg)
{
    int a, b;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 2))
        return 0;
    a = atoi(argv[1]);
    b = atoi(argv[2]);
    AddEvent_Fixrides(g_level_byte_669050, a, b);
    return 1;
}

/* POWERRIDES <count> */
// FUNCTION: LEGOLAND 0x00479ee0
int LevelKw_POWERRIDES(char** argv, int argc, int arg)
{
    int n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 1))
        return 0;
    n = atoi(argv[1]);
    AddEvent_Powerrides(g_level_byte_669050, n);
    return 1;
}

/* ZONING <LEGOLAND|ADVENTURER|CASTLE|WESTERN> <count> */
// FUNCTION: LEGOLAND 0x00479f30
int LevelKw_ZONING(char** argv, int argc, int arg)
{
    int zone, n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 2))
        return 0;
    zone = LookupNamedIndex(argv[1], g_zoning_names, 4);
    if (zone == -1)
        return 0;
    n = atoi(argv[2]);
    AddEvent_Zoning(g_level_byte_669050, zone, n);
    return 1;
}

/* CHECKFLAG <flag> [<on>]: on defaults to 1. */
// FUNCTION: LEGOLAND 0x00479fa0
int LevelKw_CHECKFLAG(char** argv, int argc, int arg)
{
    int flag, on;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 1))
        return 0;
    flag = atoi(argv[1]);
    if (argc >= 2)
        on = atoi(argv[2]);
    else
        on = 1;
    AddEvent_Checkflag(g_level_byte_669050, flag, on);
    return 1;
}

/* THEMEICON <icon> [<on>]: immediate in [INIT], an event elsewhere. */
// FUNCTION: LEGOLAND 0x0047a020
int LevelKw_THEMEICON(char** argv, int argc, int arg)
{
    int icon, on;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 5, 1))
        return 0;
    icon = atoi(argv[1]);
    if (argc >= 2)
        on = atoi(argv[2]);
    else
        on = 1;
    if (g_level_number == 1)
        SetThemeIcon(icon, on);
    else
        AddEvent_Themeicon(icon, on);
    return 1;
}

/* ADDFLAG <flag> [<on>]: immediate in [INIT], an event elsewhere. */
// FUNCTION: LEGOLAND 0x0047a0b0
int LevelKw_ADDFLAG(char** argv, int argc, int arg)
{
    int flag, on;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 5, 1))
        return 0;
    flag = atoi(argv[1]);
    if (argc >= 2)
        on = atoi(argv[2]);
    else
        on = 1;
    if (g_level_number == 1)
        AddLevelFlag(flag, on);
    else
        AddEvent_Addflag(flag, on);
    return 1;
}

/* BRIDGES <count> [<on>]: the count is one-based in the script and
 * zero-based inside (a positive count is decremented). */
// FUNCTION: LEGOLAND 0x0047a140
int LevelKw_BRIDGES(char** argv, int argc, int arg)
{
    int count, on;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 5, 1))
        return 0;
    count = atoi(argv[1]);
    if (count > 0)
        count--;
    if (argc >= 2)
        on = atoi(argv[2]);
    else
        on = 1;
    if (g_level_number == 1)
        SetBridges(count, on);
    else
        AddEvent_Bridges(count, on);
    return 1;
}

/* ENDSCREENS <which> [<text> [<text2>]]: "<text>;<text2>" becomes the level
 * end sequence, but only in [INIT]; in any other section the words are
 * parsed and dropped. */
// FUNCTION: LEGOLAND 0x0047a1d0
int LevelKw_ENDSCREENS(char** argv, int argc, int arg)
{
    char buf[0x200] = "";
    int  which;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 5, 1))
        return 0;
    which = atoi(argv[1]);
    if (argc >= 2)
        strcpy(buf, argv[2]);
    strcat(buf, ";");
    if (argc >= 3)
        strcat(buf, argv[3]);
    if (g_level_number == 1)
        SetLevelEndSequence(which, buf);
    return 1;
}

/* SELECTTHEME [<LEGOLAND|WESTERN|CASTLE|ADVENTURER|NONE>]: no word means
 * theme 0; an unknown word fails the line. The -1 test is written over both
 * arms of the ternary: VC6 threads the constant arm past it, which is the
 * original's exiled `xor eax,eax / jmp` into the call (docs/lanes/scope-s.md). */
// FUNCTION: LEGOLAND 0x0047a2f0
int LevelKw_SELECTTHEME(char** argv, int argc, int arg)
{
    int theme;

    if (!g_level_db_active)
        return 1;
    if (KwLineApplies(argv, argc, 2, 0)) {
        theme = argc ? LookupNamedIndex(argv[1], g_theme_names, 5) : 0;
        if (theme != -1) {
            AddEvent_Selecttheme(g_level_byte_669050, theme);
            return 1;
        }
    }
    return 0;
}

/* SELECTTAB [<Build|Research>]: no word means tab 0 (same shape as
 * SELECTTHEME). */
// FUNCTION: LEGOLAND 0x0047a360
int LevelKw_SELECTTAB(char** argv, int argc, int arg)
{
    int tab;

    if (!g_level_db_active)
        return 1;
    if (KwLineApplies(argv, argc, 2, 0)) {
        tab = argc ? LookupNamedIndex(argv[1], g_tab_names, 2) : 0;
        if (tab != -1) {
            AddEvent_Selecttab(g_level_byte_669050, tab);
            return 1;
        }
    }
    return 0;
}

/* SELECTMODE [<QUERY|BUILD|ERASE|PATH|MAP>]: no word means mode argc, i.e.
 * 0 = QUERY (unreachable through KwLineApplies's need of 1, kept as
 * written). The original reloads argc from its stack home for that arm
 * (`mov eax,[esp+0x10]`) instead of using the esi copy or threading the
 * known zero the way SELECTTHEME does; the volatile re-read of the
 * parameter slot is the lever that reproduces it (docs/lanes/scope-s.md). */
// FUNCTION: LEGOLAND 0x0047a3d0
int LevelKw_SELECTMODE(char** argv, int argc, int arg)
{
    int mode;

    if (!g_level_db_active)
        return 1;
    if (KwLineApplies(argv, argc, 2, 1)) {
        mode = argc ? LookupNamedIndex(argv[1], g_mode_names, 5)
                    : *(volatile int*)&argc;
        if (mode != -1) {
            AddEvent_Selectmode(g_level_byte_669050, mode);
            return 1;
        }
    }
    return 0;
}

/* FOREVER */
// FUNCTION: LEGOLAND 0x0047a440
int LevelKw_FOREVER(char** argv, int argc, int arg)
{
    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 2, 0))
        return 0;
    AddEvent_Forever(g_level_byte_669050);
    return 1;
}

/* GIVE <object> [NOPOPUP] -- [REWARD]; also called by ENABLE (scope R). */
// FUNCTION: LEGOLAND 0x0047a480
int LevelKw_GIVE(char** argv, int argc, int arg)
{
    LLElem* e;
    int     popup = 1;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 4, 1))
        return 0;
    e = ElemID(argv[1]);
    if (argc >= 2 && NameCompare(argv[2], "NOPOPUP") == 0)
        popup = 0;
    if (e)
        AddEvent_Give(e, popup);
    return 1;
}

/* TAKE <object> -- [REWARD] */
// FUNCTION: LEGOLAND 0x0047a500
int LevelKw_TAKE(char** argv, int argc, int arg)
{
    LLElem* e;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 4, 1))
        return 0;
    e = ElemID(argv[1]);
    if (e)
        AddEvent_Take(e);
    return 1;
}

/* ADDBRICKS <count> -- [REWARD]; a non-positive count adds nothing. */
// FUNCTION: LEGOLAND 0x0047a550
int LevelKw_ADDBRICKS(char** argv, int argc, int arg)
{
    int n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 4, 1))
        return 0;
    n = atoi(argv[1]);
    if (n > 0)
        AddEvent_Addbricks(n);
    return 1;
}
