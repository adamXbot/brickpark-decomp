/* LEGOLAND -- scope T: the last twenty-two handlers of the level-database
 * keyword table (0x004bb6f8, 93 entries of {keyword, handler}).  Every
 * handler has the same shape: nothing unless the level database is being
 * read (g_level_db_active); KwLineApplies() tests that the line belongs to
 * the section kind the parser is in (g_level_number is that kind: 1 = the
 * [INIT] section, whose lines apply at once; 4 = a step section, whose
 * lines become script events) and that it has enough arguments; then the
 * arguments are read (atoi, ElemID, LookupNamedIndex over a name table)
 * and either applied now or queued through the keyword's AddEvent_* in
 * eventmake.c (scope W).  argv[0] is the keyword, argc counts the words
 * after it.  A handler returns 0 when the line was not for it, 1 otherwise.
 * VC6 SP3 /O2 /Gy /Gd. Types describe only the fields used here.
 * Verification and recovered mechanics: docs/lanes/scope-t.md.
 */
typedef struct MapHdr { char pad00[0x1a]; unsigned short max_blokes; } MapHdr;
typedef struct Cell { char pad00[0xc]; unsigned short flags; char pad0e[6]; } Cell;   /* 0x14 bytes; the |= on the u16 narrows to a byte or, as in pathgfx.c */
typedef struct ObjDef ObjDef;

extern int         g_level_db_active;     /* 0x004bb5b0 */
extern int         g_level_number;        /* 0x00669054  the section kind being parsed (see above) */
extern MapHdr*     g_map;                 /* 0x004bcbf4 */
extern Cell**      g_map_rows;            /* 0x00801400 */
extern int         g_visitor_cap;         /* 0x00832920 */
extern int         g_visitor_cap_extra;   /* 0x00832924 */
extern int         g_entrance_fee;        /* 0x00832970 */
extern const char* g_feature_names[12];   /* 0x004bb5f4  Terraces RideWear PlantWear AutoStud Energy Hunger Inspector Autorepair Adv_Tune MoneyBar CapacityCalc FreePlayAtEnd */
extern const char* g_report_names[25];    /* 0x004bb624 */
extern const char* g_hap_factor_names[13];/* 0x004bb688  RideFull RideBroken SeeBorder SeeScenery SeeStopNLook SeeShop SeeRide Hunger Bordom FaveFood Food FaveRide Ride */
extern const char* g_capacity_names[6];   /* 0x004bb6bc  Path Ride Scenery StopLook Shop Foodshop */
extern const char* g_button_names[9];     /* 0x004bb6d4 */

/* the parse primitives (levelkw.c, scope R) */
extern int  KwLineApplies(char** argv, int argc, int mask, int nargs);       /* 0x004786c0  (g_level_number & mask) && argc >= nargs */
extern int  KwSectionMatches(char** argv, int argc, int mask);               /* 0x004786a0  (g_level_number & mask) != 0 */
extern void ParseRectArgs(int* rect, char** argv, int first);                /* 0x00478700  four ints, normalised x0<=x1, y0<=y1 */
extern void ParsePosArgs(int* pos, char** argv, int first);                  /* 0x00478770  two ints */
extern int  LookupNamedIndex(const char* name, const char** table, int n);   /* 0x004781b0  -1 when absent */
extern int  atoi(const char* s);                                             /* 0x004a04b9  CRT */
extern int  NameCompare(const char* a, const char* b);                        /* 0x004aab90  CRT _stricmp; the tree's name */
extern ObjDef* ElemID(const char* name);                                     /* 0x0047b3f0 */

/* the event constructors (eventmake.c, scope W) */
extern void AddEvent_Place(ObjDef* def, int* pos, int count);               /* 0x0046b880 */
extern void AddEvent_Clear(int* rect);                                       /* 0x0046b8c0 */
extern void AddEvent_Unglue(int* rect);                                      /* 0x0046b900 */
extern void AddEvent_Glue(int* rect);                                        /* 0x0046b940 */
extern void AddEvent_Extendpark(int* rect);                                  /* 0x0046b980 */
extern void AddEvent_Fmv(const char* name);                                  /* 0x0046b9c0 */
extern void AddEvent_Interval(const char* s);                                /* 0x0046b9f0 */
extern void AddEvent_Message(const char* s);                                 /* 0x0046ba30 */
extern void AddEvent_Feature(int idx, int v);                                /* 0x0046ba60 */
extern void AddEvent_Report(int idx, int a, int b);                          /* 0x0046ba90 */
extern void AddEvent_Degrade(ObjDef* def, int v, int n);                     /* 0x0046bb40 */
extern void AddEvent_Capacity(int which, int v);                             /* 0x0046bb80  1 = MAX, 0 = MIN */
extern void AddEvent_Capacityscale(int idx, int v);                          /* 0x0046bbb0 */
extern void AddEvent_Capacitycap(int idx, int v);                            /* 0x0046bbe0 */
extern void AddEvent_Entrancefee(int v);                                     /* 0x0046bc10 */
extern void AddEvent_Endlevel(void);                                         /* 0x0046bc40 */
extern void AddEvent_Purge(void);                                            /* 0x0046bc60 */
extern void AddEvent_Flashbutton(int bits, int on);                          /* 0x0046bd70 */

/* the immediate forms ([INIT] lines) */
extern void PlaceScriptObject(ObjDef* def, int* pos);                        /* 0x00469bd0  eventtick.c (scope V) */
extern void SetReportMovie(const char* name);                                /* 0x00490610 */
extern void SetFeatureFlags(int idx, int v);                                 /* 0x0046a040  eventtick.c (scope V) */
extern void SetReportMode(int idx, int a, int b);                            /* 0x0046a140  eventtick.c (scope V) */
extern void SetHappinessFactor(int idx, int v);                              /* 0x00482d60 */
extern void SetSimTuningA(int i, int v);                                     /* 0x00462e50 */
extern void SetSimTuningB(int i, int v);                                     /* 0x00462e70 */
extern void FlashButton(int bits, int on);                                   /* 0x00476070 */

// FUNCTION: LEGOLAND 0x0047a5a0
int LevelKw_PLACE(char** argv, int argc)
{
    int     pos[2];
    ObjDef* def;
    int     n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 5, 3))
        return 0;
    def = ElemID(argv[1]);
    ParsePosArgs(pos, argv, 2);
    n = argc >= 4 ? atoi(argv[4]) : 0;
    if (def) {
        if (g_level_number == 1)
            PlaceScriptObject(def, pos);
        else
            AddEvent_Place(def, pos, n);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047a650
int LevelKw_CLEAR(char** argv, int argc)
{
    int rect[4];

    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 4, 4))
            return 0;
        ParseRectArgs(rect, argv, 1);
        AddEvent_Clear(rect);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047a6a0
int LevelKw_UNGLUE(char** argv, int argc)
{
    int rect[4];

    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 4, 4))
            return 0;
        ParseRectArgs(rect, argv, 1);
        AddEvent_Unglue(rect);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047a6f0
int LevelKw_GLUE(char** argv, int argc)
{
    int rect[4];
    int x, y;

    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 5, 4))
            return 0;
        ParseRectArgs(rect, argv, 1);
        if (g_level_number == 4) {
            AddEvent_Glue(rect);
        } else {
            for (y = rect[1]; y <= rect[3]; y++)
                for (x = rect[0]; x <= rect[2]; x++)
                    g_map_rows[y][x].flags |= 0x40;
        }
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047a7b0
int LevelKw_EXTENDPARK(char** argv, int argc)
{
    int rect[4];

    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 4, 4))
            return 0;
        ParseRectArgs(rect, argv, 1);
        AddEvent_Extendpark(rect);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047a800
int LevelKw_FMV(char** argv, int argc)
{
    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 5, 1))
            return 0;
        if (g_level_number == 4)
            AddEvent_Fmv(argv[1]);
        else
            SetReportMovie(argv[1]);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047a860
int LevelKw_INTERVAL(char** argv, int argc)
{
    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 4, 1))
            return 0;
        AddEvent_Interval(argv[1]);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047a8a0
int LevelKw_MESSAGE(char** argv, int argc)
{
    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 4, 1))
            return 0;
        AddEvent_Message(argv[1]);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047a8e0
int LevelKw_FEATURE(char** argv, int argc)
{
    int idx, v;

    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 5, 2))
            return 0;
        idx = LookupNamedIndex(argv[1], g_feature_names, 12);
        v = atoi(argv[2]);
        if (idx == -1)
            return 0;
        if (g_level_number == 4)
            AddEvent_Feature(idx, v);
        else
            SetFeatureFlags(idx, v);
    }
    return 1;
}

/* REPORT <name> off | REPORT <name> <a> <b>.  "HAPPY_VIS" is looked up under
 * its misspelt table entry "Happpy_Vis" (an original quirk, reproduced). */
// FUNCTION: LEGOLAND 0x0047a960
int LevelKw_REPORT(char** argv, int argc)
{
    int off, a, b, idx;

    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 5, 2))
            return 0;
        if (NameCompare(argv[2], "off")) {
            off = 0;
            if (!KwLineApplies(argv, argc, 5, 3))
                return 0;
            a = atoi(argv[2]);
            b = atoi(argv[3]);
        } else {
            off = 1;
        }
        if (NameCompare(argv[1], "HAPPY_VIS") == 0)
            idx = LookupNamedIndex("Happpy_Vis", g_report_names, 25);
        else
            idx = LookupNamedIndex(argv[1], g_report_names, 25);
        if (idx == -1)
            return 0;
        if (g_level_number == 4) {
            if (!off)
                AddEvent_Report(idx, a, b);
            else
                AddEvent_Report(idx, 0, 0);
        } else {
            if (!off)
                SetReportMode(idx, a, b);
            else
                SetReportMode(idx, 0, 0);
        }
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047aa90
int LevelKw_HAP_FACTOR(char** argv, int argc)
{
    int v, idx;

    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 1, 2))
            return 0;
        v = atoi(argv[2]);
        idx = LookupNamedIndex(argv[1], g_hap_factor_names, 13);
        if (idx == -1)
            return 0;
        SetHappinessFactor(idx, v);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047ab00
int LevelKw_CAPACITYSCALE(char** argv, int argc)
{
    int idx, v;

    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 5, 2))
            return 0;
        idx = LookupNamedIndex(argv[1], g_capacity_names, 6);
        v = atoi(argv[2]);
        if (idx == -1)
            return 0;
        if (g_level_number == 1)
            SetSimTuningA(idx, v);
        else
            AddEvent_Capacityscale(idx, v);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047ab80
int LevelKw_CAPACITYCAP(char** argv, int argc)
{
    int idx, v;

    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 5, 2))
            return 0;
        idx = LookupNamedIndex(argv[1], g_capacity_names, 6);
        v = atoi(argv[2]);
        if (idx == -1)
            return 0;
        if (g_level_number == 1)
            SetSimTuningB(idx, v);
        else
            AddEvent_Capacitycap(idx, v);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047ac00
int LevelKw_DEGRADE(char** argv, int argc)
{
    ObjDef* def;
    int     v, n;

    if (!g_level_db_active)
        return 1;
    if (!KwLineApplies(argv, argc, 4, 2))
        return 0;
    def = ElemID(argv[1]);
    v = atoi(argv[2]);
    n = argc >= 3 ? atoi(argv[3]) : 0;
    if (def)
        AddEvent_Degrade(def, v, n);
    return 1;
}

// FUNCTION: LEGOLAND 0x0047ac80
int LevelKw_MAXBLOKES(char** argv, int argc)
{
    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 1, 1))
            return 0;
        g_map->max_blokes = (unsigned short)atoi(argv[1]);
        g_visitor_cap = g_map->max_blokes;
        g_visitor_cap_extra = 0;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047ace0
int LevelKw_MAXCAPACITY_MAXVISITORS(char** argv, int argc)
{
    int v;

    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 5, 1))
            return 0;
        v = atoi(argv[1]);
        if (g_level_number == 1)
            g_visitor_cap = v;
        else
            AddEvent_Capacity(1, v);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047ad40
int LevelKw_MINCAPACITY_MINVISITORS(char** argv, int argc)
{
    int v;

    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 5, 1))
            return 0;
        v = atoi(argv[1]);
        if (g_level_number == 1)
            g_visitor_cap_extra = v;
        else
            AddEvent_Capacity(0, v);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047ada0
int LevelKw_ENTRANCEFEE(char** argv, int argc)
{
    int v;

    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 5, 1))
            return 0;
        v = atoi(argv[1]);
        if (g_level_number == 1)
            g_entrance_fee = v;
        else
            AddEvent_Entrancefee(v);
    }
    return 1;
}

/* FLASHBUTTON <name|bit>...: each word is a button name (bit index) or a
 * literal mask; the words are OR-ed. */
// FUNCTION: LEGOLAND 0x0047ae00
int LevelKw_FLASHBUTTON(char** argv, int argc)
{
    int bits = 0;
    int idx, i;

    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 5, 1))
            return 0;
        for (i = 1; i <= argc; i++) {
            idx = LookupNamedIndex(argv[i], g_button_names, 9);
            if (idx != -1)
                bits |= 1 << idx;
            else
                bits |= atoi(argv[i]);
        }
        if (g_level_number == 1)
            FlashButton(bits, 1);
        else
            AddEvent_Flashbutton(bits, 1);
    }
    return 1;
}

/* FLASHBUTTOFF with no words turns every button off (-1). */
// FUNCTION: LEGOLAND 0x0047aea0
int LevelKw_FLASHBUTTOFF(char** argv, int argc)
{
    int bits = 0;
    int idx, i;

    if (g_level_db_active) {
        if (!KwLineApplies(argv, argc, 5, 0))
            return 0;
        if (argc == 0)
            bits = -1;
        for (i = 1; i <= argc; i++) {
            idx = LookupNamedIndex(argv[i], g_button_names, 9);
            if (idx != -1)
                bits |= 1 << idx;
            else
                bits |= atoi(argv[i]);
        }
        if (g_level_number == 1)
            FlashButton(bits, 0);
        else
            AddEvent_Flashbutton(bits, 0);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047af50
int LevelKw_PURGE(char** argv, int argc)
{
    if (g_level_db_active) {
        if (!KwSectionMatches(argv, argc, 4))
            return 0;
        AddEvent_Purge();
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0047af80
int LevelKw_ENDLEVEL(char** argv, int argc)
{
    if (g_level_db_active) {
        if (!KwSectionMatches(argv, argc, 4))
            return 0;
        AddEvent_Endlevel();
    }
    return 1;
}
