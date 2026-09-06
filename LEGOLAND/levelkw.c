/* LEGOLAND -- scope R: the level-database keyword tier, part 1.
 *
 * LoadLevelDatabase (movie.c) opens "Scripts\<level>" and ParseKeywordFile
 * (movie3.c) hands the opened file to ParseKeywordSections here, which reads
 * it line by line and dispatches each line's first word through the
 * 93-entry {keyword, handler} table at 0x004bb6f8. This file holds the
 * reader, the argument primitives every handler uses, the section keywords
 * that open and close script steps, and the first 26 handlers; scopes S and
 * T hold the rest, scope W the AddEvent_* constructors they call.
 *
 * VC6 SP3 /O2 /Gy /Gd. Types describe only the fields used here; names are
 * ours (the brief's provisional readings, renamed where the body says so
 * and recorded in docs/lanes/scope-r.md), addresses are load-bearing.
 */

/* ---- types --------------------------------------------------------------- */
typedef struct Pos  { int x, y; } Pos;
typedef struct Rect { int left, top, right, bottom; } Rect;
typedef struct ScriptStep ScriptStep;

/* A keyword line arrives as an argv-style vector: args[0] is the keyword,
 * args[1..] its words. Every handler takes the vector, its length and the
 * caller's extra argument, and returns 1 to continue, 0 to skip, -1 on a
 * fatal error. */
typedef int (*KeywordFn)(char** args, int argc, int extra);

/* ---- globals ------------------------------------------------------------- */
extern int          g_level_db_active;   /* 0x004bb5b0  the current [AGES] band includes the player */
extern int          g_level_db_flag_ac;  /* 0x004bb5ac  the last section kind (CurLevelFlags) */
extern char         g_level_name[];      /* 0x00669058  the current section's name */
extern unsigned int g_level_number;      /* 0x00669054  the current section's number / mask */
extern unsigned char g_level_flags;      /* 0x00669050  section flags: 1 objective, 2 permanent, 4 reminder, 8 purge */
extern int          g_step_serial;       /* 0x00669098  next script step id */
extern ScriptStep*  g_script_cur;        /* 0x0066879c  the step in progress */
extern void*        g_script_root;       /* 0x007fdca4  the goal list of the step in progress */
extern unsigned int g_profile_age;       /* 0x0080ffc0  CurProfile+0x20 */

/* ---- string literals (.data) --------------------------------------------- */
extern const char g_str_purge[];         /* 0x004bb9ec "PURGE" */
extern const char g_str_uninitialised[]; /* 0x004bc0a4 "Uninitialised" */
extern const char g_str_closed[];        /* 0x004bc0b4 "Closed" */

/* ---- callees ------------------------------------------------------------- */
extern int         _stricmp(const char* a, const char* b);            /* 0x004aab90 (CRT) */
extern int         atoi(const char* s);                               /* 0x004a04b9 (CRT) */
extern unsigned int strlen(const char* s);
extern char*       strcpy(char* dst, const char* src);
#pragma intrinsic(strlen, strcpy)
extern ScriptStep* NewScriptStep(int id);                             /* 0x0046b4f0 */
extern void        FreeScriptSteps(ScriptStep* s);                    /* 0x0046b590 (scope W) */
extern void        ClearObjectCounters(void);                         /* 0x00480d10 */
extern int         LoadBaseMap(const char* name);                     /* 0x00461a50 */
extern void        CalculateMapRenderOrder(void);                     /* 0x0045a4a0 */
extern void        RecheckPoweredObjects(void);                       /* 0x0045a060 */
extern int         EnsureObjectClassLoaded(const char* name);         /* 0x00478b20 */
extern void*       ElemID(const char* name);                          /* 0x0047b3f0 */
extern void        MarkElemAvailable(void* elem, int a, int b);       /* 0x00469900 */
extern int         LevelKw_GIVE(char** args, int argc, int extra);    /* 0x0047a480 (scope S) */

/* ========================================================================== */
/* The argument primitives.                                                   */
/* ========================================================================== */

/* Index of `name` in a table of `count` names, case-insensitively, or -1. */
// FUNCTION: LEGOLAND 0x004781b0
int LookupNamedIndex(const char* name, const char** table, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        if (_stricmp(name, table[i]) == 0)
            return i;
    }
    return -1;
}

/* Record the section we are in: its name and its number (or level mask). */
// FUNCTION: LEGOLAND 0x004785d0
void CurLevelSection(const char* name, int number)
{
    strcpy(g_level_name, name);
    g_level_number = number;
}

/* Record the section kind: 1 objective, 2 permanent, 3 reminder, else none. */
// FUNCTION: LEGOLAND 0x00478610
void CurLevelFlags(int kind)
{
    g_level_db_flag_ac = kind;
    switch (kind) {
    case 1:
        g_level_flags = 1;
        break;
    case 2:
        g_level_flags = 2;
        break;
    case 3:
        g_level_flags = 4;
        break;
    default:
        g_level_flags = 0;
        break;
    }
}

/* A section line whose first word is PURGE sets flag 8; any other clears it. */
// FUNCTION: LEGOLAND 0x00478650
void IsPurgeLine(char** args, int argc)
{
    if (argc) {
        if (_stricmp(args[1], g_str_purge) == 0)
            g_level_flags |= 8;
        else
            g_level_flags &= ~8;
    }
}

/* The line has at least `n` words after the keyword. */
// FUNCTION: LEGOLAND 0x00478690
int HasArgs(char** args, int argc, int n)
{
    return argc >= n;
}

/* The current section applies to a level in `mask`. */
// FUNCTION: LEGOLAND 0x004786a0
int LevelMaskMatches(char** args, int argc, int mask)
{
    return (g_level_number & mask) != 0;
}

/* Both: the section applies and the line has enough words. */
// FUNCTION: LEGOLAND 0x004786c0
int LineApplies(char** args, int argc, int mask, int n)
{
    if (!LevelMaskMatches(args, argc, mask))
        return 0;
    return HasArgs(args, argc, n) != 0;
}

/* Four numbers from args[first..] into a rect, normalised so left <= right
 * and top <= bottom. */
// FUNCTION: LEGOLAND 0x00478700
void ParseRectArgs(Rect* r, char** args, int first)
{
    r->left = atoi(args[first]);
    r->top = atoi(args[first + 1]);
    r->right = atoi(args[first + 2]);
    r->bottom = atoi(args[first + 3]);
    if (r->left > r->right) {
        int t = r->left;
        r->left = r->right;
        r->right = t;
    }
    if (r->top > r->bottom) {
        int t = r->top;
        r->top = r->bottom;
        r->bottom = t;
    }
}

/* Two numbers from args[first..] into a position. */
// FUNCTION: LEGOLAND 0x00478770
void ParsePosArgs(Pos* p, char** args, int first)
{
    p->x = atoi(args[first]);
    p->y = atoi(args[first + 1]);
}

/* The line's first word after the keyword. */
// FUNCTION: LEGOLAND 0x004787a0
char* Arg1(char** args, int argc)
{
    return args[1];
}

/* Keyword "none": an empty first word is a blank line. */
// FUNCTION: LEGOLAND 0x004787b0
int LevelKw_none(char** args, int argc, int extra)
{
    return strlen(args[0]) == 0;
}

/* Close the step in progress. */
// FUNCTION: LEGOLAND 0x004787d0
void EndScriptStep(void)
{
    if (g_script_cur)
        FreeScriptSteps(g_script_cur);
    g_script_cur = 0;
}

/* Open a new step with the next serial; its goal list starts empty. */
// FUNCTION: LEGOLAND 0x004787f0
void BeginScriptStep(void)
{
    g_script_cur = NewScriptStep(g_step_serial++);
    g_script_root = 0;
}

/* Never referenced: a section named "Uninitialised". */
// FUNCTION: LEGOLAND 0x00478820
int LevelKw_Uninitialised(char** args, int argc, int extra)
{
    CurLevelSection(g_str_uninitialised, 0);
    return 1;
}

/* [END]: the section is "Closed", the step is finished. */
// FUNCTION: LEGOLAND 0x00478840
int LevelKw_END(char** args, int argc, int extra)
{
    CurLevelSection(g_str_closed, 0);
    EndScriptStep();
    g_script_cur = 0;
    return 1;
}

/* [INIT]: section 1. */
// FUNCTION: LEGOLAND 0x00478870
int LevelKw_INIT(char** args, int argc, int extra)
{
    CurLevelSection(args[0], 1);
    return 1;
}

/* AGES [lo [hi]]: the following lines apply when the player's age is at
 * least lo (and at most hi). Unsigned compares; a band with lo > hi is an
 * error (return 0). */
// FUNCTION: LEGOLAND 0x00478890
int LevelKw_AGES(char** args, int argc, int extra)
{
    unsigned int lo, hi;

    if (argc == 0) {
        g_level_db_active = 1;
        return 1;
    }
    if (argc == 1) {
        g_level_db_active = g_profile_age >= (unsigned int)atoi(args[1]);
        return 1;
    }
    lo = atoi(args[1]);
    hi = atoi(args[2]);
    if (lo > hi)
        return 0;
    if (g_profile_age < lo || g_profile_age > hi)
        g_level_db_active = 0;
    else
        g_level_db_active = 1;
    return 1;
}

/* [OBJECTIVE] <mask>: section 2, kind 1, and a fresh step. */
// FUNCTION: LEGOLAND 0x00478930
int LevelKw_OBJECTIVE(char** args, int argc, int extra)
{
    if (g_level_db_active) {
        if (!LevelMaskMatches(args, argc, 7))
            return 0;
        CurLevelSection(args[0], 2);
        CurLevelFlags(1);
        EndScriptStep();
        BeginScriptStep();
    }
    return 1;
}

/* [ONEOFF]: kind 0 within the current step. */
// FUNCTION: LEGOLAND 0x00478980
int LevelKw_ONEOFF(char** args, int argc, int extra)
{
    if (g_level_db_active) {
        if (!LevelMaskMatches(args, argc, 2))
            return 0;
        CurLevelFlags(0);
    }
    return 1;
}

/* [ONGOING]: kind 1. */
// FUNCTION: LEGOLAND 0x004789c0
int LevelKw_ONGOING(char** args, int argc, int extra)
{
    if (g_level_db_active) {
        if (!LevelMaskMatches(args, argc, 2))
            return 0;
        CurLevelFlags(1);
    }
    return 1;
}

/* [PERMANENT] [PURGE]: kind 2. */
// FUNCTION: LEGOLAND 0x00478a00
int LevelKw_PERMANENT(char** args, int argc, int extra)
{
    if (g_level_db_active) {
        if (!LevelMaskMatches(args, argc, 2))
            return 0;
        CurLevelFlags(2);
        IsPurgeLine(args, argc);
    }
    return 1;
}

/* [REMINDER] [PURGE]: kind 3. */
// FUNCTION: LEGOLAND 0x00478a40
int LevelKw_REMINDER(char** args, int argc, int extra)
{
    if (g_level_db_active) {
        if (!LevelMaskMatches(args, argc, 2))
            return 0;
        CurLevelFlags(3);
        IsPurgeLine(args, argc);
    }
    return 1;
}

/* [REWARD]: section 4. */
// FUNCTION: LEGOLAND 0x00478a80
int LevelKw_REWARD(char** args, int argc, int extra)
{
    if (g_level_db_active) {
        if (!LevelMaskMatches(args, argc, 2))
            return 0;
        CurLevelSection(args[0], 4);
    }
    return 1;
}

/* MAP <name>: load the base map now. -1 when it fails. */
// FUNCTION: LEGOLAND 0x00478ac0
int LevelKw_MAP(char** args, int argc, int extra)
{
    if (g_level_db_active) {
        char* name;

        if (!HasArgs(args, argc, 1))
            return 0;
        name = Arg1(args, argc);
        ClearObjectCounters();
        if (!LoadBaseMap(name))
            return -1;
        CalculateMapRenderOrder();
        RecheckPoweredObjects();
    }
    return 1;
}

/* LOAD <class>: make sure the class's resources are in. */
// FUNCTION: LEGOLAND 0x00478b70
int LevelKw_LOAD(char** args, int argc, int extra)
{
    if (!g_level_db_active)
        return 1;
    if (!LineApplies(args, argc, 1, 1))
        return 0;
    return EnsureObjectClassLoaded(Arg1(args, argc)) != 0;
}

int LevelKw_ENABLE(char** args, int argc, int extra);

/* BLUEPRINT is ENABLE. */
// FUNCTION: LEGOLAND 0x00478bc0
int LevelKw_BLUEPRINT(char** args, int argc, int extra)
{
    return LevelKw_ENABLE(args, argc, extra);
}

/* ENABLE <class>: on level 1 load the class and mark it available; on any
 * other level it is a GIVE. */
// FUNCTION: LEGOLAND 0x00478be0
int LevelKw_ENABLE(char** args, int argc, int extra)
{
    if (!g_level_db_active)
        return 1;
    if (!LineApplies(args, argc, 5, 1))
        return 0;
    if (g_level_number == 1) {
        char* name = Arg1(args, argc);

        if (EnsureObjectClassLoaded(name))
            MarkElemAvailable(ElemID(name), 0, 1);
        return 1;
    }
    return LevelKw_GIVE(args, argc, extra);
}

/* ========================================================================== */
/* The level-1 / queued handlers.                                             */
/* ========================================================================== */

/* The map header (lpConfig) as these handlers see it. */
typedef struct MapHdr {
    unsigned short view_w;      /* +0x00  view size in pixels */
    unsigned short view_h;      /* +0x02 */
    char           pad04[0x30];
    int            mechanics;   /* +0x34  non-zero: mechanic orders accepted */
    int            gardeners;   /* +0x38  non-zero: gardener orders accepted */
} MapHdr;

typedef struct KeywordEntry { const char* keyword; KeywordFn handler; } KeywordEntry;

extern MapHdr*     g_map;                 /* 0x004bcbf4 lpConfig */
extern int         g_rate_t0[5];          /* 0x00832928 the five HAPPINESS_ENV rates */
extern int         g_scroll_x;            /* 0x00667cb4 ScrollX (24.8) */
extern int         g_scroll_y;            /* 0x00667cb8 ScrollY (24.8) */
extern int         g_line_number;         /* 0x00668fcc lines read from the current keyword file */
extern const char  g_str_all[];           /* 0x004bc0bc "ALL" */
extern const char  g_str_none[];          /* 0x004bbdcc "none" */
extern const char  g_str_check[];         /* 0x004bc090 "check" */
extern const char  g_str_delims[];        /* 0x004bc098 " ,;:(){}\xa0\n\t" */
extern const char  kEmpty[];              /* 0x004d8bb0 "" */

extern int          strcmp(const char* a, const char* b);
#pragma intrinsic(strcmp)
extern char*        strchr(const char* s, int c);                         /* 0x004a0050 (CRT) */
extern unsigned int strspn(const char* s, const char* set);               /* 0x004a03f0 (CRT) */
extern unsigned int strcspn(const char* s, const char* set);              /* 0x004a03b0 (CRT) */
extern int          toupper(int c);                                       /* 0x0049f34b (CRT) */
extern int          RES_ReadFile(void* f, void* buf, int n);              /* 0x00489cf0 */
extern void         progress_tick(void);                                  /* 0x004663f0 */
extern void         GetTileDimensions(int* w, int* h);                    /* 0x00460540 */
extern void*        GenerateGardener(Pos* pos, int in_hut);               /* 0x0049a1a0 */
extern void*        GenerateMechanic(Pos* pos, int in_hut);               /* 0x0049a340 */
extern void*        NewScriptEvent(void* a, void* b, void* c);            /* 0x004689f0 */
extern void         SetCurrency(int amount);                              /* 0x00457900 */
extern void         LoadBriefingFile(const char* name);                   /* 0x004687f0 */
extern void         LoadHintsFile(const char* name);                      /* 0x00468810 */
extern void         AddEvent_Intro(const char* text, ScriptStep* step);   /* 0x0046b650 (scope W) */
extern void         AddEvent_Currency(int amount);                        /* 0x0046b850 (scope W) */
extern void         AddEvent_Gardener_Mechanic(int kind, int n, Pos* pos);/* 0x0046bad0 (scope W) */
extern void         AddEvent_Workers(int gardeners, int mechanics);       /* 0x0046bb10 (scope W) */
extern void         AddEvent_Breifingfile_Briefingfile(const char* name); /* 0x0046bd10 (scope W) */
extern void         AddEvent_Hintsfile(const char* name);                 /* 0x0046bd40 (scope W) */
extern void         AddEvent_Lookat(Pos* pos);                            /* 0x0046bda0 (scope W) */
extern void         AddEvent_Need(unsigned char flags, void* elem, int n);/* 0x0046bdd0 (scope W) */
extern void         AddEvent_Needat(unsigned char flags, void* elem, Pos* pos); /* 0x0046be00 (scope W) */
extern void         AddEvent_Needin(unsigned char flags, void* elem, int n, Rect* r); /* 0x0046be40 (scope W) */
extern void         AddEvent_Connect(unsigned char flags, void* elem);    /* 0x0046be90 (scope W) */
extern void         AddEvent_Link(unsigned char flags, void* elem);       /* 0x0046bec0 (scope W) */
extern void         AddEvent_Range(unsigned char flags, void* elem, int hi, int lo); /* 0x0046bef0 (scope W) */
extern void         AddEvent_Cleararea(unsigned char flags, Rect* r, int n); /* 0x0046bf30 (scope W) */

/* CURRENCY amount: set it now on level 1, else queue it. */
// FUNCTION: LEGOLAND 0x00478c60
int LevelKw_CURRENCY(char** args, int argc, int extra)
{
    if (g_level_db_active) {
        if (!LineApplies(args, argc, 5, 1))
            return 0;
        if (g_level_number == 1) {
            SetCurrency(atoi(args[1]));
            return 1;
        }
        AddEvent_Currency(atoi(args[1]));
    }
    return 1;
}

/* HAPPINESS_ENV a b c d e: the five rates, level 1 only. */
// FUNCTION: LEGOLAND 0x00478cd0
int LevelKw_HAPPINESS_ENV(char** args, int argc, int extra)
{
    if (g_level_db_active) {
        if (!LineApplies(args, argc, 5, 5))
            return 0;
        if (g_level_number == 1) {
            int i;

            for (i = 0; i < 5; i++)
                g_rate_t0[i] = atoi(args[i + 1]);
        }
    }
    return 1;
}

/* LOOKAT x y: centre the view on a map cell now (level 1) or later. The
 * level-1 arm projects the cell (in 1/256ths) to the screen with the tile
 * size and subtracts half the view. */
// FUNCTION: LEGOLAND 0x00478d30
int LevelKw_LOOKAT(char** args, int argc, int extra)
{
    Pos pos;

    if (g_level_db_active) {
        if (!LineApplies(args, argc, 5, 2))
            return 0;
        ParsePosArgs(&pos, args, 1);
        pos.x <<= 8;
        pos.y <<= 8;
        if (g_level_number == 1) {
            int w, h;
            int y = pos.y;
            int x = pos.x;

            GetTileDimensions(&w, &h);
            pos.x = ((x - y) * w) >> 9;
            pos.y = ((x + y) * h) >> 9;
            g_scroll_x = (pos.x - (g_map->view_w >> 1)) << 8;
            g_scroll_y = (pos.y - (g_map->view_h >> 1)) << 8;
            return 1;
        }
        AddEvent_Lookat(&pos);
    }
    return 1;
}

/* BREIFINGFILE [name]: load the briefing now on level 1, else queue it.
 * (The keyword is misspelt in the table; BRIEFINGFILE is a synonym.) */
// FUNCTION: LEGOLAND 0x00478e20
int LevelKw_BREIFINGFILE_BRIEFINGFILE(char** args, int argc, int extra)
{
    const char* name = kEmpty;

    if (!g_level_db_active)
        return 1;
    if (!LineApplies(args, argc, 5, 0))
        return 0;
    if (argc >= 1)
        name = args[1];
    if (g_level_number == 1)
        LoadBriefingFile(name);
    else
        AddEvent_Breifingfile_Briefingfile(name);
    return 1;
}

/* HINTSFILE [name]: the same for the hints. */
// FUNCTION: LEGOLAND 0x00478e90
int LevelKw_HINTSFILE(char** args, int argc, int extra)
{
    const char* name = kEmpty;

    if (!g_level_db_active)
        return 1;
    if (!LineApplies(args, argc, 5, 0))
        return 0;
    if (argc >= 1)
        name = args[1];
    if (g_level_number == 1)
        LoadHintsFile(name);
    else
        AddEvent_Hintsfile(name);
    return 1;
}

/* WORKERS gardeners mechanics: enable either kind (negative leaves it) now
 * on level 1, else queue it. */
// FUNCTION: LEGOLAND 0x00478f00
int LevelKw_WORKERS(char** args, int argc, int extra)
{
    int gardeners, mechanics;

    if (!g_level_db_active)
        return 1;
    if (!LineApplies(args, argc, 5, 2))
        return 0;
    gardeners = atoi(args[1]);
    mechanics = atoi(args[2]);
    if (g_level_number == 1) {
        if (gardeners >= 0)
            g_map->gardeners = gardeners != 0;
        if (mechanics >= 0)
            g_map->mechanics = mechanics != 0;
    } else {
        AddEvent_Workers(gardeners, mechanics);
    }
    return 1;
}

/* GARDENER x y [n]: n gardeners (at least one) at a cell, now on level 1,
 * else queued. */
// FUNCTION: LEGOLAND 0x00478fa0
int LevelKw_GARDENER(char** args, int argc, int extra)
{
    Pos pos;
    int n;

    if (g_level_db_active) {
        if (!LineApplies(args, argc, 5, 2))
            return 0;
        pos.x = atoi(args[1]);
        pos.y = atoi(args[2]);
        if (argc < 3 || (n = atoi(args[3])) < 1)
            n = 1;
        if (g_level_number == 1) {
            int i = n;

            while (i != 0) {
                GenerateGardener(&pos, 0);
                i--;
            }
            return 1;
        }
        AddEvent_Gardener_Mechanic(1, n, &pos);
    }
    return 1;
}

/* MECHANIC x y [n]: the same for mechanics. */
// FUNCTION: LEGOLAND 0x00479060
int LevelKw_MECHANIC(char** args, int argc, int extra)
{
    Pos pos;
    int n;

    if (g_level_db_active) {
        if (!LineApplies(args, argc, 5, 2))
            return 0;
        pos.x = atoi(args[1]);
        pos.y = atoi(args[2]);
        if (argc < 3 || (n = atoi(args[3])) < 1)
            n = 1;
        if (g_level_number == 1) {
            int i = n;

            while (i != 0) {
                GenerateMechanic(&pos, 0);
                i--;
            }
            return 1;
        }
        AddEvent_Gardener_Mechanic(0, n, &pos);
    }
    return 1;
}

/* ========================================================================== */
/* The goal handlers.                                                         */
/* ========================================================================== */

/* PROMPT [text [text2]]: the step's prompt event, or none. */
// FUNCTION: LEGOLAND 0x00479120
int LevelKw_PROMPT(char** args, int argc, int extra)
{
    if (!g_level_db_active)
        return 1;
    if (!LineApplies(args, argc, 2, 0))
        return 0;
    if (argc > 0) {
        if (argc > 1)
            g_script_root = NewScriptEvent(args[1], args[2], (void*)1);
        else
            g_script_root = NewScriptEvent(args[1], 0, (void*)1);
    } else {
        g_script_root = 0;
    }
    return 1;
}

/* INTRO text: the step's intro text. */
// FUNCTION: LEGOLAND 0x004791a0
int LevelKw_INTRO(char** args, int argc, int extra)
{
    if (g_level_db_active) {
        if (!LineApplies(args, argc, 2, 1))
            return 0;
        if (g_script_cur)
            AddEvent_Intro(args[1], g_script_cur);
    }
    return 1;
}

/* NEED class [n]: a goal for n (at least one) of a class. */
// FUNCTION: LEGOLAND 0x004791f0
int LevelKw_NEED(char** args, int argc, int extra)
{
    void* elem;
    int   n;

    if (!g_level_db_active)
        return 1;
    if (!LineApplies(args, argc, 2, 1))
        return 0;
    elem = ElemID(args[1]);
    if (argc <= 1 || (n = atoi(args[2])) == 0)
        n = 1;
    if (elem)
        AddEvent_Need(g_level_flags, elem, n);
    return 1;
}

/* NEEDAT class x y: a goal for a class at a cell. */
// FUNCTION: LEGOLAND 0x00479270
int LevelKw_NEEDAT(char** args, int argc, int extra)
{
    Pos   pos;
    void* elem;

    if (!g_level_db_active)
        return 1;
    if (!LineApplies(args, argc, 2, 3))
        return 0;
    elem = ElemID(args[1]);
    pos.x = atoi(args[2]);
    pos.y = atoi(args[3]);
    if (elem)
        AddEvent_Needat(g_level_flags, elem, &pos);
    return 1;
}

/* NEEDIN class n l t r b: a goal for n of a class inside a rect. */
// FUNCTION: LEGOLAND 0x00479300
int LevelKw_NEEDIN(char** args, int argc, int extra)
{
    Rect  r;
    void* elem;
    int   n;

    if (!g_level_db_active)
        return 1;
    if (!LineApplies(args, argc, 2, 6))
        return 0;
    elem = ElemID(args[1]);
    n = atoi(args[2]);
    ParseRectArgs(&r, args, 3);
    if (elem)
        AddEvent_Needin(g_level_flags, elem, n, &r);
    return 1;
}

/* CONNECT class: a goal to connect a class. */
// FUNCTION: LEGOLAND 0x00479390
int LevelKw_CONNECT(char** args, int argc, int extra)
{
    if (g_level_db_active) {
        void* elem;

        if (!LineApplies(args, argc, 2, 1))
            return 0;
        elem = ElemID(args[1]);
        if (elem)
            AddEvent_Connect(g_level_flags, elem);
    }
    return 1;
}

/* LINK class|ALL: a goal to link a class, or every class; an unknown class
 * name skips the line. */
// FUNCTION: LEGOLAND 0x004793e0
int LevelKw_LINK(char** args, int argc, int extra)
{
    if (g_level_db_active) {
        void* elem;

        if (!LineApplies(args, argc, 2, 1))
            return 0;
        if (_stricmp(args[1], g_str_all) == 0) {
            elem = 0;
        } else {
            elem = ElemID(args[1]);
            if (!elem)
                return 0;
        }
        AddEvent_Link(g_level_flags, elem);
    }
    return 1;
}

/* RANGE class lo [hi]: a goal on a class's count between lo and hi. */
// FUNCTION: LEGOLAND 0x00479450
int LevelKw_RANGE(char** args, int argc, int extra)
{
    void* elem;
    int   lo, hi;

    if (!g_level_db_active)
        return 1;
    if (!LineApplies(args, argc, 2, 2))
        return 0;
    elem = ElemID(args[1]);
    lo = atoi(args[2]);
    if (argc >= 3)
        hi = atoi(args[3]);
    else
        hi = 0;
    if (elem)
        AddEvent_Range(g_level_flags, elem, hi, lo);
    return 1;
}

/* CLEARAREA l t r b [n]: a goal to clear a rect. */
// FUNCTION: LEGOLAND 0x004794d0
int LevelKw_CLEARAREA(char** args, int argc, int extra)
{
    Rect r;
    int  n;

    if (g_level_db_active) {
        if (!LineApplies(args, argc, 2, 4))
            return 0;
        ParseRectArgs(&r, args, 1);
        if (argc >= 5)
            n = atoi(args[5]);
        else
            n = 0;
        AddEvent_Cleararea(g_level_flags, &r, n);
    }
    return 1;
}

/* ========================================================================== */
/* The reader.                                                                */
/* ========================================================================== */

/* Split a line into words at the delimiters, in place; a word may be
 * quoted. Returns the count. */
// FUNCTION: LEGOLAND 0x00478110
int SplitWords(char* s, const char* delims, char** words)
{
    int n = 0;

    while (*s) {
        char* p;

        s += strspn(s, delims);
        if (*s == 0)
            continue;
        if (*s == '"') {
            *s++ = 0;
            words[n++] = s;
            p = strchr(s, '"');
            if (!p)
                return n;
            *p = 0;
            s = p + 1;
        } else {
            int len;

            words[n++] = s;
            len = strcspn(s, delims);
            p = strchr(s, '"');
            if (p && p - s < len) {
                s = p;
                continue;
            }
            s += len;
            if (*s == 0)
                return n;
            *s++ = 0;
        }
    }
    return n;
}

/* Read one line of a resource file into buf (at most max chars), dropping
 * the CR/LF. Returns buf, or 0 at the end of the file. */
// FUNCTION: LEGOLAND 0x00489e60
char* ReadLine(void* f, char* buf, int max)
{
    char  c;
    int   eof = 0;
    int   n = 0;
    char* p = buf;

    while (1) {
        if (!RES_ReadFile(f, &c, 1)) {
            eof = 1;
            break;
        }
        if (c == '\r')
            break;
        if (c == '\n')
            break;
        *p++ = c;
        if (++n >= max)
            break;
    }
    if (c == '\r')
        RES_ReadFile(f, &c, 1);
    *p = 0;
    if (eof && n == 0)
        return 0;
    return buf;
}

/* Upper-case a word in place; returns its length. */
// FUNCTION: LEGOLAND 0x00499300
int UpcaseString(char* s)
{
    int i = 0;

    while ((s[i] = (char)toupper(s[i])) != 0)
        i++;
    return i;
}

/* Read the opened keyword file line by line. Each line is split into words,
 * the first word upper-cased and looked up in the table; its handler gets
 * the words, the count after the keyword and the caller's extra argument.
 * A table whose first keyword is "none" names a fallback for unknown
 * words; one whose last keyword is "check" names a handler that runs at the
 * end with the count of lines whose handler returned 0. A negative handler
 * result aborts and is returned; otherwise the skipped count is. */
// FUNCTION: LEGOLAND 0x00478280
int ParseKeywordSections(void* f, KeywordEntry* table, int count, int extra)
{
    int       skipped = 0;
    int       rc = 0;
    KeywordFn fallback = 0;
    int       nwords;
    char      handled;
    char      found;
    char*     words[20];
    char      line[0x400];

    g_line_number = 0;
    if (strcmp(table[0].keyword, g_str_none) == 0)
        fallback = table[0].handler;
    while (ReadLine(f, line, 0x400)) {
        char* p;

        g_line_number++;
        progress_tick();
        p = strchr(line, '#');
        if (p)
            *p = 0;
        nwords = SplitWords(line, g_str_delims, words);
        if (nwords) {
            int  i;

            UpcaseString(words[0]);
            found = 0;
            handled = 0;
            for (i = 0; i < count; i++) {
                if (strcmp(words[0], table[i].keyword) == 0) {
                    found = 1;
                    rc = table[i].handler(words, nwords - 1, extra);
                    if (rc == 0)
                        skipped++;
                    break;
                }
            }
            /* Two flags, merged here: with a single flag set before the
             * `break`, VC6 folds its test away and jumps each path straight
             * past the fallback; the or of the two (both zeroed above, both
             * living in bl) is what keeps the original's `test bl,bl / jne`
             * and, with it, the whole shape of the loop and its epilogue. */
            handled = found | handled;
            if (!handled && fallback)
                rc = fallback(words, nwords - 1, extra);
        }
        if (rc < 0)
            break;
    }
    if (rc >= 0 && strcmp(table[count - 1].keyword, g_str_check) == 0)
        rc = table[count - 1].handler(0, skipped, extra);
    if (rc < 0)
        return rc;
    return skipped;
}
