/* LEGOLAND -- scope W: the script-event constructors.
 *
 * The level-database keyword handlers (levelkw.c, levelkw2.c, levelkw3.c)
 * turn most keywords into script events. Every constructor here is one
 * shape: `e = NewScriptEvent(kind, 1)`, the keyword's arguments into the
 * event's fields, then the event onto a list of the step being read
 * (`g_script_cur`). Step events (kinds 2..32) clear the flags byte and go
 * on the step's event list (+0x10) through LinkStepEvent; goal events
 * (kinds 33..69) take the section's flags byte as their first argument,
 * remember the root goal (`g_script_root`) and go on the step's goal list
 * (+0x0c) through LinkGoalEvent. Also here: the sorted step insert the
 * section closer uses, the step text setter INTRO uses, and the step-hint
 * display the script-end icon uses.
 *
 * VC6 SP3 /O2 /Gy /Gd. The ScriptEvent layout is uimisc.c/fpui3.c's with
 * the argument fields the constructors fill named f14/f18/f1c (their
 * meaning is per kind; the tick handlers in scopes V and X give them
 * theirs). Names are ours (the integrator's provisional readings in
 * docs/SCOPE_W_event_constructors.md; scope T's for the constructors it
 * already declares). Verification: docs/lanes/scope-w.md.
 */

/* ---- types --------------------------------------------------------------- */
typedef struct Pos  { int x, y; } Pos;
typedef struct Rect { int left, top, right, bottom; } Rect;

typedef struct ScriptEvent {
    struct ScriptEvent* next;    /* +0x00 */
    void*         elem;          /* +0x04  the LLIDB element the event refers to */
    char*         text;          /* +0x08  owned when flags & 0x20 */
    int           kind;          /* +0x0c */
    unsigned char flags;         /* +0x10  a goal's section flags; 0 for step events */
    char          pad11[3];      /* +0x11 */
    int           f14;           /* +0x14  per-kind arguments */
    int           f18;           /* +0x18 */
    int           f1c;           /* +0x1c */
    Pos           pos;           /* +0x20 */
    Rect          area;          /* +0x28 */
    int           mode;          /* +0x38  NewScriptEvent's second argument */
    int           time;          /* +0x3c */
    struct ScriptEvent* root;    /* +0x40  the root goal a goal event belongs to */
} ScriptEvent;                   /* 0x44 */

typedef struct ScriptStep {
    struct ScriptStep* next;     /* +0x00 */
    int           id;            /* +0x04  the step list is kept sorted on it */
    char*         text;          /* +0x08  the INTRO text */
    ScriptEvent*  goals;         /* +0x0c */
    ScriptEvent*  events;        /* +0x10 */
} ScriptStep;

/* ---- globals ------------------------------------------------------------- */
extern ScriptStep*  g_script_steps;      /* 0x00668798  head of the step list */
extern ScriptStep*  g_script_cur;        /* 0x0066879c  the step being read / in progress */
extern ScriptEvent* g_script_root;       /* 0x007fdca4  the root goal event */
extern int          g_last_hint;         /* 0x00668614 */
extern int          g_hint_up;           /* 0x00668618  a step hint string is on screen */
extern const char*  g_hint_strings[];    /* 0x007fe120 */

/* ---- callees ------------------------------------------------------------- */
extern ScriptEvent* NewScriptEvent(int kind, int mode);                        /* 0x00468910 */
extern void  SetScriptEventText(ScriptEvent* e, const char* text, int copy);   /* 0x00468b40 */
extern void* HeapAlloc_w(unsigned int size);                                   /* 0x0049e4ff (CRT malloc) */
extern void  HeapFree_w(void* p);                                              /* 0x0049e4d0 (CRT free) */
extern void  AddHelpMessage(const char* fmt, ...);                             /* 0x00468bb0 */
extern void  ShowScriptStepText(ScriptStep* s, int mode);                      /* 0x0046b6b0 */
extern void  ResetScriptTimer(void);                                           /* 0x00468d00 */
extern unsigned int strlen(const char* s);
extern char* strcpy(char* dst, const char* src);
#pragma intrinsic(strlen, strcpy)

/* ---- this file ----------------------------------------------------------- */
void LinkGoalEvent(ScriptEvent* e, ScriptStep* step);
void LinkStepEvent(ScriptEvent* e, ScriptStep* step);

/* ========================================================================= */

/* Insert a finished step into the step list, sorted on id (the brief's
 * `FreeScriptSteps`; EndScriptStep 0x004787d0 calls it). A step that sorts
 * before the current head REPLACES the whole list: the "no predecessor"
 * arm stores it as the head with a NULL link -- an original bug kept. */
// FUNCTION: LEGOLAND 0x0046b590
void InsertScriptStep(ScriptStep* s)
{
    ScriptStep* step;
    ScriptStep* prev = 0;

    for (step = g_script_steps; step; step = step->next) {
        if (step->id >= s->id)
            break;
        prev = step;
    }
    if (prev) {
        s->next = prev->next;
        prev->next = s;
    } else {
        g_script_steps = s;
        s->next = 0;
    }
}

/* Push a goal event onto the step's goal list, remembering the root goal. */
// FUNCTION: LEGOLAND 0x0046b610
void LinkGoalEvent(ScriptEvent* e, ScriptStep* step)
{
    e->root = g_script_root;
    if (step->goals)
        e->next = step->goals;
    step->goals = e;
}

/* Push a step event onto the step's event list. */
// FUNCTION: LEGOLAND 0x0046b630
void LinkStepEvent(ScriptEvent* e, ScriptStep* step)
{
    if (step->events) {
        e->next = step->events;
        step->events = e;
    } else {
        step->events = e;
    }
}

/* INTRO: replace the step's text with a heap copy (the brief's
 * `AddEvent_Intro`; LevelKw_INTRO passes (argv[1], g_script_cur) -- the
 * text is the FIRST argument, the step the second). */
// FUNCTION: LEGOLAND 0x0046b650
void SetScriptStepText(const char* text, ScriptStep* step)
{
    if (step->text)
        HeapFree_w(step->text);
    step->text = HeapAlloc_w(strlen(text) + 1);
    strcpy(step->text, text);
}

/* The script-end icon: put the pending hint string up as advisor help, or
 * the current step's text; 0 when no step is in progress. */
// FUNCTION: LEGOLAND 0x0046b700
int ShowStepHint(void)
{
    if (g_script_cur) {
        if (g_last_hint) {
            AddHelpMessage("%s", g_hint_strings[g_last_hint]);
            g_hint_up = 1;
            return 1;
        }
        ShowScriptStepText(g_script_cur, 1);
        ResetScriptTimer();
        return 1;
    }
    return 0;
}

/* ---- step events (kinds 2..32): flags cleared, LinkStepEvent ------------- */

// FUNCTION: LEGOLAND 0x0046b790
void AddEvent_Give(void* elem, int popup)
{
    ScriptEvent* e = NewScriptEvent(2, 1);
    e->elem = elem;
    e->flags = 0;
    e->f14 = popup;
    LinkStepEvent(e, g_script_cur);
}

/* Unreferenced in the shipped exe (swept dead); kind 3 has no keyword. */
// FUNCTION: LEGOLAND 0x0046b7c0
void AddEvent_Kind3(void* elem, int v)
{
    ScriptEvent* e = NewScriptEvent(3, 1);
    e->elem = elem;
    e->f18 = v;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046b7f0
void AddEvent_Take(void* elem)
{
    ScriptEvent* e = NewScriptEvent(4, 1);
    e->flags = 0;
    e->elem = elem;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046b820
void AddEvent_Addbricks(int count)
{
    ScriptEvent* e = NewScriptEvent(5, 1);
    e->flags = 0;
    e->f1c = count;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046b850
void AddEvent_Currency(int amount)
{
    ScriptEvent* e = NewScriptEvent(6, 1);
    e->flags = 0;
    e->f1c = amount;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046b880
void AddEvent_Place(void* def, Pos* pos, int count)
{
    ScriptEvent* e = NewScriptEvent(7, 1);
    e->elem = def;
    e->pos = *pos;
    e->f18 = count;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046b8c0
void AddEvent_Clear(Rect* r)
{
    ScriptEvent* e = NewScriptEvent(8, 1);
    e->area = *r;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046b900
void AddEvent_Unglue(Rect* r)
{
    ScriptEvent* e = NewScriptEvent(9, 1);
    e->area = *r;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046b940
void AddEvent_Glue(Rect* r)
{
    ScriptEvent* e = NewScriptEvent(10, 1);
    e->area = *r;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046b980
void AddEvent_Extendpark(Rect* r)
{
    ScriptEvent* e = NewScriptEvent(11, 1);
    e->area = *r;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

/* The text events leave the flags byte to SetScriptEventText (0x20: owned). */
// FUNCTION: LEGOLAND 0x0046b9c0
void AddEvent_Fmv(const char* name)
{
    ScriptEvent* e = NewScriptEvent(12, 1);
    SetScriptEventText(e, name, 1);
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046b9f0
void AddEvent_Interval(const char* s)
{
    ScriptEvent* e = NewScriptEvent(13, 1);
    SetScriptEventText(e, s, 1);
    e->f1c = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046ba30
void AddEvent_Message(const char* s)
{
    ScriptEvent* e = NewScriptEvent(14, 1);
    SetScriptEventText(e, s, 1);
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046ba60
void AddEvent_Feature(int idx, int v)
{
    ScriptEvent* e = NewScriptEvent(15, 1);
    e->f1c = idx;
    e->f14 = v;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046ba90
void AddEvent_Report(int idx, int a, int b)
{
    ScriptEvent* e = NewScriptEvent(24, 1);
    e->f18 = idx;
    e->f1c = a;
    e->f14 = b;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

/* GARDENER and MECHANIC share it: who, how many, where. */
// FUNCTION: LEGOLAND 0x0046bad0
void AddEvent_Gardener_Mechanic(int who, int count, Pos* pos)
{
    ScriptEvent* e = NewScriptEvent(16, 1);
    e->f1c = count;
    e->pos = *pos;
    e->f14 = who;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bb10
void AddEvent_Workers(int a, int b)
{
    ScriptEvent* e = NewScriptEvent(17, 1);
    e->f1c = a;
    e->f14 = b;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bb40
void AddEvent_Degrade(void* def, int v, int n)
{
    ScriptEvent* e = NewScriptEvent(18, 1);
    e->elem = def;
    e->f14 = v;
    e->f1c = n;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

/* MAXCAPACITY/MAXVISITORS (which = 1) and MINCAPACITY/MINVISITORS (0);
 * levelkw3.c's name. */
// FUNCTION: LEGOLAND 0x0046bb80
void AddEvent_Capacity(int which, int v)
{
    ScriptEvent* e = NewScriptEvent(19, 1);
    e->f1c = v;
    e->f14 = which;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bbb0
void AddEvent_Capacityscale(int idx, int v)
{
    ScriptEvent* e = NewScriptEvent(20, 1);
    e->f1c = v;
    e->f14 = idx;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bbe0
void AddEvent_Capacitycap(int idx, int v)
{
    ScriptEvent* e = NewScriptEvent(21, 1);
    e->f1c = v;
    e->f14 = idx;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bc10
void AddEvent_Entrancefee(int v)
{
    ScriptEvent* e = NewScriptEvent(22, 1);
    e->flags = 0;
    e->f1c = v;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bc40
void AddEvent_Endlevel(void)
{
    ScriptEvent* e = NewScriptEvent(32, 1);
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bc60
void AddEvent_Purge(void)
{
    ScriptEvent* e = NewScriptEvent(31, 1);
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bc80
void AddEvent_Themeicon(int icon, int on)
{
    ScriptEvent* e = NewScriptEvent(25, 1);
    e->f1c = on;
    e->f14 = icon;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bcb0
void AddEvent_Addflag(int flag, int on)
{
    ScriptEvent* e = NewScriptEvent(26, 1);
    e->f1c = on;
    e->f14 = flag;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bce0
void AddEvent_Bridges(int count, int on)
{
    ScriptEvent* e = NewScriptEvent(27, 1);
    e->f1c = on;
    e->f14 = count;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

/* BREIFINGFILE and BRIEFINGFILE (both spellings are in the keyword table). */
// FUNCTION: LEGOLAND 0x0046bd10
void AddEvent_Breifingfile_Briefingfile(const char* name)
{
    ScriptEvent* e = NewScriptEvent(28, 1);
    SetScriptEventText(e, name, 1);
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bd40
void AddEvent_Hintsfile(const char* name)
{
    ScriptEvent* e = NewScriptEvent(29, 1);
    SetScriptEventText(e, name, 1);
    LinkStepEvent(e, g_script_cur);
}

/* FLASHBUTTON (on = 1) and FLASHBUTTOFF (0); levelkw3.c's name. */
// FUNCTION: LEGOLAND 0x0046bd70
void AddEvent_Flashbutton(int bits, int on)
{
    ScriptEvent* e = NewScriptEvent(30, 1);
    e->f1c = bits;
    e->f14 = on;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bda0
void AddEvent_Lookat(Pos* pos)
{
    ScriptEvent* e = NewScriptEvent(23, 1);
    e->pos = *pos;
    e->flags = 0;
    LinkStepEvent(e, g_script_cur);
}

/* ---- goal events (kinds 33..69): section flags byte, LinkGoalEvent ------- */

// FUNCTION: LEGOLAND 0x0046bdd0
void AddEvent_Need(unsigned char flags, void* elem, int count)
{
    ScriptEvent* e = NewScriptEvent(33, 1);
    e->elem = elem;
    e->f1c = count;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046be00
void AddEvent_Needat(unsigned char flags, void* elem, Pos* pos)
{
    ScriptEvent* e = NewScriptEvent(34, 1);
    e->elem = elem;
    e->pos = *pos;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046be40
void AddEvent_Needin(unsigned char flags, void* elem, int count, Rect* r)
{
    ScriptEvent* e = NewScriptEvent(35, 1);
    e->elem = elem;
    e->f1c = count;
    e->area = *r;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046be90
void AddEvent_Connect(unsigned char flags, void* elem)
{
    ScriptEvent* e = NewScriptEvent(36, 1);
    e->elem = elem;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bec0
void AddEvent_Link(unsigned char flags, void* elem)
{
    ScriptEvent* e = NewScriptEvent(37, 1);
    e->elem = elem;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bef0
void AddEvent_Range(unsigned char flags, void* elem, int a, int b)
{
    ScriptEvent* e = NewScriptEvent(38, 1);
    e->elem = elem;
    e->f1c = a;
    e->f14 = b;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bf30
void AddEvent_Cleararea(unsigned char flags, Rect* r, int count)
{
    ScriptEvent* e = NewScriptEvent(39, 1);
    e->area = *r;
    e->f14 = count;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bf80
void AddEvent_Remove(unsigned char flags, void* elem, int count)
{
    ScriptEvent* e = NewScriptEvent(40, 1);
    e->elem = elem;
    e->f1c = count;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bfb0
void AddEvent_Removerange(unsigned char flags, void* elem, int hi, int lo)
{
    ScriptEvent* e = NewScriptEvent(41, 1);
    e->elem = elem;
    e->f1c = hi;
    e->f14 = lo;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046bff0
void AddEvent_Composite(unsigned char flags, void* elem, int count, int extra)
{
    ScriptEvent* e = NewScriptEvent(42, 1);
    e->elem = elem;
    e->f1c = count;
    e->f14 = extra;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c030
void AddEvent_Loopcomposite(unsigned char flags, void* elem, int count)
{
    ScriptEvent* e = NewScriptEvent(43, 1);
    e->elem = elem;
    e->f1c = count;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c060
void AddEvent_Techlevel(unsigned char flags, void* elem, int level)
{
    ScriptEvent* e = NewScriptEvent(44, 1);
    e->elem = elem;
    e->f18 = level;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c090
void AddEvent_Parkvisitors(unsigned char flags, int count)
{
    ScriptEvent* e = NewScriptEvent(45, 1);
    e->f1c = count;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c0c0
void AddEvent_Ridevisitors(unsigned char flags, void* elem, int count)
{
    ScriptEvent* e = NewScriptEvent(46, 1);
    e->elem = elem;
    e->f1c = count;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c0f0
void AddEvent_Riders(unsigned char flags, void* elem, int count)
{
    ScriptEvent* e = NewScriptEvent(47, 1);
    e->elem = elem;
    e->f1c = count;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

/* The six coverage goals keep their percentage at +0x14, not +0x1c. */
// FUNCTION: LEGOLAND 0x0046c120
void AddEvent_Scenerycoverage(unsigned char flags, int pct)
{
    ScriptEvent* e = NewScriptEvent(48, 1);
    e->f14 = pct;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c150
void AddEvent_Pathscenery(unsigned char flags, int pct)
{
    ScriptEvent* e = NewScriptEvent(49, 1);
    e->f14 = pct;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c180
void AddEvent_Ridecoverage(unsigned char flags, int pct)
{
    ScriptEvent* e = NewScriptEvent(50, 1);
    e->f14 = pct;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c1b0
void AddEvent_Shopcoverage(unsigned char flags, int pct)
{
    ScriptEvent* e = NewScriptEvent(51, 1);
    e->f14 = pct;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c1e0
void AddEvent_Foodcoverage(unsigned char flags, int pct)
{
    ScriptEvent* e = NewScriptEvent(52, 1);
    e->f14 = pct;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c210
void AddEvent_Totcoverage(unsigned char flags, int pct)
{
    ScriptEvent* e = NewScriptEvent(53, 1);
    e->f14 = pct;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c240
void AddEvent_Studarea(unsigned char flags, Rect* r, int count)
{
    ScriptEvent* e = NewScriptEvent(55, 1);
    e->area = *r;
    e->f14 = count;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c290
void AddEvent_Save(unsigned char flags, int slot)
{
    ScriptEvent* e = NewScriptEvent(56, 1);
    e->f1c = slot;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c2c0
void AddEvent_Happiness(unsigned char flags, int a, int b)
{
    ScriptEvent* e = NewScriptEvent(57, 1);
    e->f1c = a;
    e->f14 = b;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c2f0
void AddEvent_Needgardeners(unsigned char flags, int count)
{
    ScriptEvent* e = NewScriptEvent(58, 1);
    e->f1c = count;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c320
void AddEvent_Needmechanics(unsigned char flags, int count)
{
    ScriptEvent* e = NewScriptEvent(59, 1);
    e->f1c = count;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c350
void AddEvent_Hunger(unsigned char flags, int a, int b, int plus)
{
    ScriptEvent* e = NewScriptEvent(60, 1);
    e->f14 = b;
    e->f1c = a;
    e->f18 = plus;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c390
void AddEvent_Fixrides(unsigned char flags, int a, int b)
{
    ScriptEvent* e = NewScriptEvent(61, 1);
    e->f1c = a;
    e->f14 = b;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c3c0
void AddEvent_Powerrides(unsigned char flags, int count)
{
    ScriptEvent* e = NewScriptEvent(62, 1);
    e->f1c = count;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c3f0
void AddEvent_Zoning(unsigned char flags, int zone, int count)
{
    ScriptEvent* e = NewScriptEvent(63, 1);
    e->f1c = zone;
    e->f14 = count;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c420
void AddEvent_Checkflag(unsigned char flags, int flag, int on)
{
    ScriptEvent* e = NewScriptEvent(64, 1);
    e->f1c = on;
    e->f14 = flag;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c450
void AddEvent_Selecttheme(unsigned char flags, int theme)
{
    ScriptEvent* e = NewScriptEvent(65, 1);
    e->f1c = theme;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c480
void AddEvent_Selecttab(unsigned char flags, int tab)
{
    ScriptEvent* e = NewScriptEvent(66, 1);
    e->f1c = tab;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c4b0
void AddEvent_Selectmode(unsigned char flags, int mode)
{
    ScriptEvent* e = NewScriptEvent(67, 1);
    e->f1c = mode;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

/* Unreferenced in the shipped exe (swept dead); kind 68 has no keyword.
 * The brief's `sub_46c4e0`. */
// FUNCTION: LEGOLAND 0x0046c4e0
void AddEvent_Kind68(unsigned char flags, void* elem)
{
    ScriptEvent* e = NewScriptEvent(68, 1);
    e->elem = elem;
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}

// FUNCTION: LEGOLAND 0x0046c510
void AddEvent_Forever(unsigned char flags)
{
    ScriptEvent* e = NewScriptEvent(69, 1);
    e->flags = flags;
    LinkGoalEvent(e, g_script_cur);
}
