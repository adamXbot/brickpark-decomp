/* LEGOLAND -- scope X: script-event ticks for kinds 38..69.
 * Return 1 when the goal is satisfied; unmet goals send rate-limited hints.
 * VC6 SP3 /O2 /Gy /Gd. See docs/lanes/scope-x.md for mechanics and gates.
 */
typedef struct Pos { int x, y; } Pos;
typedef struct Rect { int left, top, right, bottom; } Rect;
typedef struct Elem Elem;
typedef struct ObjDef ObjDef;
typedef struct ObjInst {
    struct ObjInst* next;
    struct ObjInst* prev;
    ObjDef* owner;
} ObjInst;
typedef struct Rider { struct Rider* next; } Rider;
struct ObjDef {
    ObjDef* next;               /* +0x00 */
    ObjInst* instances;          /* +0x04 */
    int count;                  /* +0x08 */
    char pad0c[0x20];
    unsigned char condition;    /* +0x2c */
    char pad2d[0x2b];
    Elem* composite;            /* +0x58 */
    Elem* range;                /* +0x5c */
    char pad60[0x60];
    int (*loop_size)(Elem*, int); /* +0xc0 */
    Elem* elem;                 /* +0xc4 */
    void* counters;             /* +0xc8 */
    Rider* riders;              /* +0xcc */
};
struct Elem {
    const char* name;
    void* image;
    int flags;
    ObjDef* data;
};
typedef struct Cell {
    Elem* elem;                 /* +0x00 */
    unsigned char x, y;         /* +0x04 */
    unsigned short next;
    unsigned short tile, ground;
    unsigned short flags;       /* +0x0c */
    unsigned short userflags;
    unsigned char rf, condition; /* +0x10 */
    unsigned short tail;
} Cell;
typedef struct Map {
    char pad00[0x14];
    unsigned short width, height;
} Map;
typedef struct Person {
    struct Person* next;
    char pad04[0x76];
    short happiness;            /* +0x7a */
    unsigned short hunger;      /* +0x7c */
} Person;
typedef struct ScriptEvent {
    struct ScriptEvent* next;
    Elem* elem;
    char* text;
    int kind;
    unsigned char flags;
    char pad11[3];
    int f14, f18, f1c;
    Pos pos;
    Rect area;
    int mode, time, hint;
} ScriptEvent;

extern ObjDef* g_objdef_head;         /* 0x00669240 */
extern Map* g_map;                   /* 0x004bcbf4 */
extern Cell** g_map_rows;            /* 0x00801400 */
extern int g_visitor_count;          /* 0x006661bc */
extern int g_area_type2;             /* 0x00667cf8 */
extern int g_area_type1;             /* 0x00667ce4 */
extern int g_area_type4;             /* 0x00667ce8 */
extern int g_area_type5;             /* 0x00667cec */
extern int g_area_total;             /* 0x00667ce0 */
extern int g_tally_percent;          /* 0x00667d08 */
extern Person* g_people_head;        /* 0x0066b574 */
extern int g_power_unserved_n;       /* 0x00832bdc */
extern int g_menu_index;             /* 0x004baff8 */
extern int g_object_list_mode;       /* 0x00668e34 */
extern int g_edit_changed;           /* 0x008119b0 */
extern ObjDef* g_edit_object;        /* 0x008119b8 */
extern ObjDef* g_env_class;          /* 0x007fd624 */

extern int ObjCount(Elem* elem);                                  /* 0x00480d30 */
extern int DBPrintf(const char*, ...);                            /* 0x00453a20 */
extern int _stricmp(const char*, const char*);                    /* 0x004aab90 */
extern Cell* GetFirstObjectMatching(Elem* elem);                  /* 0x0045a910 */
extern unsigned short GetRideVisitCountAt(Pos* pos);              /* 0x00489fd0 */
extern void TallyBuildFootprints(void);                           /* 0x00459970 */
extern int GetBrickCount(void);                                  /* 0x004578e0 */
extern int GetGardenerCount(void);                               /* 0x00499550 */
extern int GetMechanicCount(void);                               /* 0x00499560 */
extern Cell* GetFirstRenderObject(void);                          /* 0x0045a850 */
extern Cell* GetNextRenderObject(Cell* cell);                     /* 0x0045a8b0 */
extern signed char GetLevelFlag(int which);                      /* 0x004688c0 */
extern int HintTimerDue(void);                                   /* 0x00468d10 */
extern int ShowGoalHint(ScriptEvent* e);                          /* 0x00468d30 */
extern int EventTick_Unimplemented(ScriptEvent* e);               /* 0x00469ae0 */
extern int EventTick_Unimplemented2(ScriptEvent* e);              /* 0x00469b00 */
extern void GoalCheck_Need(ScriptEvent*, Elem*, int);              /* 0x00468d80 */
extern void GoalCheck_Range(ScriptEvent*, Elem*, int, int);        /* 0x00468e40 */
extern void GoalCheck_ClearArea(ScriptEvent*, int);               /* 0x00468f00 */
extern void GoalCheck_Remove(ScriptEvent*, Elem*, int);           /* 0x00468f40 */
extern void GoalCheck_RemoveRange(ScriptEvent*, Elem*, int, int);  /* 0x00468ea0 */
extern void GoalCheck_ParkVisitors(ScriptEvent*, int);             /* 0x00468f80 */
extern void GoalCheck_Gardeners(ScriptEvent*, int);                /* 0x00468fc0 */
extern void GoalCheck_Gardeners2(ScriptEvent*, int);               /* 0x00469000 */
extern void GoalCheck_Mechanics(ScriptEvent*, int);                /* 0x00469040 */
extern void GoalCheck_Mechanics2(ScriptEvent*, int);               /* 0x00469080 */
extern void GoalCheck_Save(ScriptEvent*, int);                     /* 0x004690c0 */
extern void GoalCheck_Happiness(ScriptEvent*, int, int);           /* 0x00469100 */
extern void GoalCheck_Hunger(ScriptEvent*, int, int);              /* 0x00469140 */
extern void GoalCheck_Hunger2(ScriptEvent*, int, int);             /* 0x00469190 */
extern void GoalCheck_Rides(ScriptEvent*, int, int);               /* 0x004691e0 */
extern void GoalCheck_RideVisitors(ScriptEvent*, Elem*, int);      /* 0x00469220 */
extern void GoalCheck_Composite(ScriptEvent*, Elem*, int, int);    /* 0x00469260 */
extern void GoalCheck_Coverage(ScriptEvent*, int, int);            /* 0x00469310 */
extern void GoalCheck_PathScenery(ScriptEvent*, int);              /* 0x00469350 */
extern void GoalCheck_LoopComposite(ScriptEvent*);                 /* 0x00469390 */

// FUNCTION: LEGOLAND 0x0046a900
int EventTick_Range(ScriptEvent* e)
{
    ObjDef* def = g_objdef_head;
    int types = 0, count = 0;
    while (def) {
        if (def->range == e->elem && def->count) {
            count += def->count;
            types++;
        }
        def = def->next;
    }
    if (types >= e->f14 && count >= e->f1c)
        return 1;
    GoalCheck_Range(e, e->elem, e->f14 - types, e->f1c - count);
    return 0;
}

/* As in the original, an out-of-map coordinate becomes NULL but is then
 * dereferenced. Script rectangles must be within the map. The one volatile
 * row-table read preserves the original load inside each valid-cell arm. */
// FUNCTION: LEGOLAND 0x0046a960
int EventTick_Cleararea(ScriptEvent* e)
{
    int count = 0;
    int x, y;
    for (y = e->area.top; y <= e->area.bottom; y++) {
        for (x = e->area.left; x <= e->area.right; x++) {
            Cell* cell;
            if (x < 0 || x >= g_map->width || y < 0 || y >= g_map->height)
                cell = 0;
            else
                cell = &(*(Cell** volatile*)&g_map_rows)[y][x];
            if ((cell->flags & 0x80) && cell->x == x && cell->y == y)
                count++;
        }
    }
    if (count > e->f14) {
        GoalCheck_ClearArea(e, count - e->f14);
        return 0;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046aa30
int EventTick_Remove(ScriptEvent* e)
{
    int count = ObjCount(e->elem);
    if (count > e->f1c) {
        GoalCheck_Remove(e, e->elem, count - e->f1c);
        return 0;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046aa70
int EventTick_Removerange(ScriptEvent* e)
{
    ObjDef* def = g_objdef_head;
    int types = 0, count = 0;
    while (def) {
        if (def->range == e->elem && def->count) {
            count += def->count;
            types++;
        }
        def = def->next;
    }
    if (e->f14 == -1) types = -1;
    if (e->f1c == -1) count = -1;
    if (types <= e->f14 && count <= e->f1c)
        return 1;
    GoalCheck_RemoveRange(e, e->elem, types - e->f14, count - e->f1c);
    return 0;
}

// FUNCTION: LEGOLAND 0x0046aae0
int EventTick_Composite(ScriptEvent* e)
{
    ObjDef* def = g_objdef_head;
    int types, count;
    if (ObjCount(e->elem) < 1) {
        GoalCheck_Need(e, e->elem, 1);
        return 0;
    }
    types = 0;
    count = 0;
    while (def) {
        if (def->composite == e->elem && def->count) {
            count += def->count;
            types++;
        }
        def = def->next;
    }
    if (types >= e->f14 && count >= e->f1c)
        return 1;
    GoalCheck_Composite(e, e->elem, e->f1c - count, e->f14 - types);
    return 0;
}

// FUNCTION: LEGOLAND 0x0046ab70
int EventTick_Loopcomposite(ScriptEvent* e)
{
    ObjDef* def = e->elem->data;
    if (def->loop_size) {
        if (def->loop_size(e->elem, 1) < e->f1c) {
            GoalCheck_LoopComposite(e);
            return 0;
        }
    } else {
        DBPrintf("Cannot evaluate LoopSize for %s\n", e->elem->name);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046abc0
int EventTick_Techlevel(ScriptEvent* e)
{
    return EventTick_Unimplemented(e);
}

// FUNCTION: LEGOLAND 0x0046abd0
int EventTick_Parkvisitors(ScriptEvent* e)
{
    if (g_visitor_count < e->f1c) {
        GoalCheck_ParkVisitors(e, e->f1c - g_visitor_count);
        return 0;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046ac00
int EventTick_Riders(ScriptEvent* e)
{
    int count = 0;
    Elem* elem = e->elem;
    Rider* rider = elem->data->riders;
    while (rider) {
        if (++count >= e->f1c)
            return 1;
        rider = rider->next;
    }
    GoalCheck_RideVisitors(e, elem, e->f1c - count);
    return 0;
}

// FUNCTION: LEGOLAND 0x0046ac50
int EventTick_Ridevisitors(ScriptEvent* e)
{
    Pos pos;
    int best = 0;
    ObjInst* instance = e->elem->data->instances;
    while (instance) {
        if (!_stricmp(instance->owner->elem->name, e->elem->name)) {
            Cell* cell = GetFirstObjectMatching(instance->owner->elem);
            if (cell) {
                int count;
                pos.x = cell->x;
                pos.y = cell->y;
                count = GetRideVisitCountAt(&pos);
                if (count >= e->f1c)
                    return 1;
                if (count > best)
                    best = count;
            }
        }
        instance = instance->next;
    }
    GoalCheck_RideVisitors(e, e->elem, e->f1c - best);
    return 0;
}

// FUNCTION: LEGOLAND 0x0046ad00
int EventTick_Scenerycoverage(ScriptEvent* e)
{
    if (g_area_type2 < e->f14) {
        GoalCheck_Coverage(e, 2, e->f14 - g_area_type2);
        return 0;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046ad30
int EventTick_Pathscenery(ScriptEvent* e)
{
    TallyBuildFootprints();
    if (g_tally_percent < e->f14) {
        GoalCheck_PathScenery(e, e->f14 - g_tally_percent);
        return 0;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046ad60
int EventTick_Ridecoverage(ScriptEvent* e)
{
    if (g_area_type1 < e->f14) {
        GoalCheck_Coverage(e, 1, e->f14 - g_area_type1);
        return 0;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046ad90
int EventTick_Shopcoverage(ScriptEvent* e)
{
    if (g_area_type4 < e->f14) {
        GoalCheck_Coverage(e, 4, e->f14 - g_area_type4);
        return 0;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046adc0
int EventTick_Foodcoverage(ScriptEvent* e)
{
    if (g_area_type5 < e->f14) {
        GoalCheck_Coverage(e, 5, e->f14 - g_area_type5);
        return 0;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046adf0
int EventTick_Totcoverage(ScriptEvent* e)
{
    if (g_area_total < e->f14) {
        GoalCheck_Coverage(e, 0, e->f14 - g_area_total);
        return 0;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046ae20
int EventTick_Kind54(ScriptEvent* e)
{
    return 1;
}

// FUNCTION: LEGOLAND 0x0046ae30
int EventTick_Studarea(ScriptEvent* e)
{
    return EventTick_Unimplemented(e);
}

// FUNCTION: LEGOLAND 0x0046ae40
int EventTick_Save(ScriptEvent* e)
{
    if (GetBrickCount() >= e->f1c)
        return 1;
    GoalCheck_Save(e, e->f1c - GetBrickCount());
    return 0;
}

// FUNCTION: LEGOLAND 0x0046ae70
int EventTick_Happiness(ScriptEvent* e)
{
    Person* person = g_people_head;
    int count = 0;
    while (person) {
        if (person->happiness >= e->f14) {
            if (++count >= e->f1c)
                return 1;
        }
        person = person->next;
    }
    GoalCheck_Happiness(e, e->f1c - count, e->f14);
    return 0;
}

// FUNCTION: LEGOLAND 0x0046aec0
int EventTick_Needgardeners(ScriptEvent* e)
{
    int count = GetGardenerCount();
    if (e->f1c > 0) {
        if (e->f1c <= count)
            return 1;
        GoalCheck_Gardeners(e, e->f1c - count);
        return 0;
    } else {
        if (-e->f1c >= count)
            return 1;
        GoalCheck_Gardeners2(e, e->f1c + count);
        return 0;
    }
}

// FUNCTION: LEGOLAND 0x0046af10
int EventTick_Needmechanics(ScriptEvent* e)
{
    int count = GetMechanicCount();
    if (e->f1c > 0) {
        if (e->f1c <= count)
            return 1;
        GoalCheck_Mechanics(e, e->f1c - count);
        return 0;
    } else {
        if (-e->f1c >= count)
            return 1;
        GoalCheck_Mechanics2(e, e->f1c + count);
        return 0;
    }
}

// FUNCTION: LEGOLAND 0x0046af60
int EventTick_Hunger(ScriptEvent* e)
{
    Person* person = g_people_head;
    int count = 0;
    while (person) {
        if (e->f18) {
            if (person->hunger >= e->f14)
                count++;
        } else {
            if (person->hunger <= e->f14)
                count++;
        }
        person = person->next;
    }
    if (e->f18) {
        if (count > e->f1c) {
            GoalCheck_Hunger(e, count - e->f1c, e->f14);
            return 0;
        }
    } else {
        if (count < e->f1c) {
            GoalCheck_Hunger2(e, e->f1c - count, e->f14);
            return 0;
        }
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046afe0
int EventTick_Fixrides(ScriptEvent* e)
{
    int count = 0;
    Cell* cell = GetFirstRenderObject();
    while (cell) {
        if (cell->condition) {
            ObjDef* def = cell->elem->data;
            int condition;
            if (def->condition)
                condition = cell->condition * 100 / def->condition;
            else
                condition = 100;
            if (e->f14 == 25 && (cell->flags & 4))
                condition = 100;
            if (condition < e->f14)
                count++;
        }
        cell = GetNextRenderObject(cell);
    }
    if (count > e->f1c) {
        GoalCheck_Rides(e, count - e->f1c, e->f14);
        return 0;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046b080
int EventTick_Powerrides(ScriptEvent* e)
{
    if (g_power_unserved_n > e->f1c) {
        GoalCheck_Rides(e, g_power_unserved_n - e->f1c, 0);
        return 0;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046b0b0
int EventTick_Zoning(ScriptEvent* e)
{
    return 1;
}

// FUNCTION: LEGOLAND 0x0046b0c0
int EventTick_Checkflag(ScriptEvent* e)
{
    if (GetLevelFlag(e->f14) >= e->f1c)
        return 1;
    if (HintTimerDue())
        ShowGoalHint(e);
    return 0;
}

// FUNCTION: LEGOLAND 0x0046b100
int EventTick_Selecttheme(ScriptEvent* e)
{
    if (e->f1c == g_menu_index)
        return 1;
    if (HintTimerDue())
        ShowGoalHint(e);
    return 0;
}

// FUNCTION: LEGOLAND 0x0046b130
int EventTick_Selecttab(ScriptEvent* e)
{
    if (g_menu_index != 5) {
        if (e->f1c && g_object_list_mode)
            return 1;
        if (!e->f1c && !g_object_list_mode)
            return 1;
    }
    if (HintTimerDue())
        ShowGoalHint(e);
    return 0;
}

// FUNCTION: LEGOLAND 0x0046b180
int EventTick_Selectmode(ScriptEvent* e)
{
    if (e->f1c == 3) {
        if (g_edit_changed == 1 && g_edit_object == g_env_class)
            return 1;
    } else {
        if (g_edit_changed == e->f1c)
            return 1;
    }
    if (HintTimerDue())
        ShowGoalHint(e);
    return 0;
}

// FUNCTION: LEGOLAND 0x0046b1e0
int EventTick_Kind68(ScriptEvent* e)
{
    return EventTick_Unimplemented2(e);
}

// FUNCTION: LEGOLAND 0x0046b1f0
int EventTick_Forever(ScriptEvent* e)
{
    return 0;
}
