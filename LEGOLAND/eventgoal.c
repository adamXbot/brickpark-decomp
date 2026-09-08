/* LEGOLAND -- scope V: the sixteen goal checks.
 *
 * Every goal tick handler (eventtick2.c, scope X) that finds its goal unmet
 * calls one of these with the event and the live numbers. Each is one
 * shape: when the hint timer has run (HintTimerDue, a 50 s clock reset by
 * ResetScriptTimer) and the goal has no ready-made hint string of its own
 * (ShowGoalHint), build a timed hint event of the check's kind (7..19)
 * carrying the numbers and queue it on the pending list.
 *
 * VC6 SP3 /O2 /Gy /Gd. The four primitives belong to scope X and are
 * declared here with our reading of their bodies. The ScriptEvent fields
 * are eventmake.c's (f14/f18/f1c: per-kind arguments). Verification:
 * docs/lanes/scope-v.md.
 */

typedef struct ScriptEvent {
    struct ScriptEvent* next;    /* +0x00 */
    void*         elem;          /* +0x04 */
    char*         text;          /* +0x08 */
    int           kind;          /* +0x0c */
    unsigned char flags;         /* +0x10 */
    char          pad11[3];
    int           f14;           /* +0x14 */
    int           f18;           /* +0x18 */
    int           f1c;           /* +0x1c */
    char          pad20[0x24];   /* +0x20 */
} ScriptEvent;                   /* 0x44 */

/* ---- scope X's goal primitives (declared, not defined, here) ------------ */
/* 0x00468d10: GetGameTimer() - g_script_start > 50000, resetting the clock
 * when so. */
extern int          HintTimerDue(void);                                  /* 0x00468d10 */
/* 0x00468d30: queue the goal's own hint string (strid at +0x40) when it has
 * one; 1 when it did. */
extern int          ShowGoalHint(ScriptEvent* e);                        /* 0x00468d30 */
/* 0x00468cd0: NewScriptEvent(kind, mode) stamped with GetGameTimer(). */
extern ScriptEvent* NewTimedEvent(int kind, int mode);                   /* 0x00468cd0 */
/* 0x00468c80: insert into the pending list (0x00668728) sorted on mode and
 * bump the per-kind count. */
extern void         QueuePendingEvent(ScriptEvent* e);                   /* 0x00468c80 */

/* ========================================================================= */

// FUNCTION: LEGOLAND 0x00468f80
void GoalCheck_ParkVisitors(ScriptEvent* e, int count)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(7, 1);
        h->f1c = count;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00468fc0
void GoalCheck_Gardeners(ScriptEvent* e, int count)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(8, 1);
        h->f1c = count;
        QueuePendingEvent(h);
    }
}

/* The "too many" twin: the count goes in negated. */
// FUNCTION: LEGOLAND 0x00469000
void GoalCheck_Gardeners2(ScriptEvent* e, int count)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(8, 1);
        h->f1c = -count;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00469040
void GoalCheck_Mechanics(ScriptEvent* e, int count)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(9, 1);
        h->f1c = count;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00469080
void GoalCheck_Mechanics2(ScriptEvent* e, int count)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(9, 1);
        h->f1c = -count;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x004690c0
void GoalCheck_Save(ScriptEvent* e, int slot)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(13, 1);
        h->f1c = slot;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00469100
void GoalCheck_Happiness(ScriptEvent* e, int a, int b)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(14, 1);
        h->f1c = a;
        h->f14 = b;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00469140
void GoalCheck_Hunger(ScriptEvent* e, int a, int b)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(15, 1);
        h->f1c = a;
        h->f14 = b;
        h->f18 = 1;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00469190
void GoalCheck_Hunger2(ScriptEvent* e, int a, int b)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(15, 1);
        h->f1c = a;
        h->f14 = b;
        h->f18 = 0;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x004691e0
void GoalCheck_Rides(ScriptEvent* e, int a, int b)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(16, 1);
        h->f1c = a;
        h->f14 = b;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00469220
void GoalCheck_RideVisitors(ScriptEvent* e, void* elem, int count)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(17, 1);
        h->elem = elem;
        h->f1c = count;
        QueuePendingEvent(h);
    }
}

/* Both numbers are clamped at zero before they go in. */
// FUNCTION: LEGOLAND 0x00469260
void GoalCheck_Composite(ScriptEvent* e, void* elem, int a, int b)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(18, 1);
        if (a < 0)
            a = 0;
        if (b < 0)
            b = 0;
        h->elem = elem;
        h->f14 = b;
        h->f1c = a;
        QueuePendingEvent(h);
    }
}

/* Unreferenced in the shipped exe (swept dead). */
// FUNCTION: LEGOLAND 0x004692c0
void GoalCheck_Unused(ScriptEvent* e, int a, int b, int c)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(19, 1);
        h->f18 = a;
        h->f14 = b;
        h->f1c = c;
        QueuePendingEvent(h);
    }
}

/* Shared by the five coverage goals: which coverage, and by how much. */
// FUNCTION: LEGOLAND 0x00469310
void GoalCheck_Coverage(ScriptEvent* e, int which, int amount)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(10, 1);
        h->f14 = amount;
        h->f1c = which;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00469350
void GoalCheck_PathScenery(ScriptEvent* e, int amount)
{
    ScriptEvent* h;

    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(11, 1);
        h->f14 = amount;
        QueuePendingEvent(h);
    }
}

/* LOOPCOMPOSITE has no generic hint of its own. */
// FUNCTION: LEGOLAND 0x00469390
void GoalCheck_LoopComposite(ScriptEvent* e)
{
    if (HintTimerDue())
        ShowGoalHint(e);
}
