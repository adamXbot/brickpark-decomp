/* LEGOLAND -- scope X: timed goal hints and the level-state primitives.
 * VC6 SP3 /O2 /Gy /Gd. Recovered mechanics and verification are recorded
 * in docs/lanes/scope-x.md. Layouts are local to this translation unit.
 */
typedef struct ScriptEvent {
    struct ScriptEvent* next;    /* +0x00 */
    void* elem;                 /* +0x04 */
    char* text;                 /* +0x08 */
    int kind;                   /* +0x0c */
    unsigned char flags;        /* +0x10 */
    char pad11[3];
    int f14, f18, f1c;           /* +0x14 */
    char pad20[0x18];
    int mode;                   /* +0x38 */
    int time;                   /* +0x3c */
    int hint;                   /* +0x40  hint-string index in these uses */
} ScriptEvent;
typedef struct Icon {
    char pad00[0x34];
    unsigned int flags;          /* +0x34 */
} Icon;

extern char g_script_text1[128];             /* 0x0066861c */
extern char g_script_text2[128];             /* 0x0066869c */
extern signed char g_script_bytes[10];       /* 0x007fe930 */
extern ScriptEvent* g_goal_list;             /* 0x00668728 */
extern int g_goal_kind_count[];              /* 0x0066872c */
extern int g_script_start;                   /* 0x00668780 */
extern const char* g_hint_strings[];         /* 0x007fe120 */
extern Icon* g_theme_icon[4];                /* 0x007fdd70 */
extern unsigned char g_profile_themes[4];    /* 0x0080ffd0 */
extern int g_bricks;                         /* 0x004b90f8 */
extern int g_appraisal_minutes;              /* 0x00832978 */
extern int g_instant_appraisal;              /* 0x00666098 */
extern int g_mood_adjustments[];             /* 0x0083293c */

extern char* strncpy(char*, const char*, unsigned int);           /* 0x004a0110 */
extern void TriggerSwitch(int which);                            /* 0x00460560 */
extern ScriptEvent* NewScriptEvent(int kind, int mode);           /* 0x00468910 */
extern int GetGameTimer(void);                                   /* 0x00499430 */
extern void ResetScriptTimer(void);                              /* 0x00468d00 */
extern void SetButtonFlash(int which, int on);                    /* 0x00476030 */
#ifndef LEGOLAND_PORTABLE
extern void UpDateCurrentProfile(void);                          /* 0x00491680 */
#else
extern int UpDateCurrentProfile(void);                          /* 0x00491680 */
#endif
extern void SetThemeIconEnabled(int which, int on);               /* 0x00476140 */
extern void QueuePendingEvent(ScriptEvent* e);                    /* 0x00468c80 */
extern ScriptEvent* NewTimedEvent(int kind, int mode);            /* 0x00468cd0 */
extern int HintTimerDue(void);                                   /* 0x00468d10 */
extern int ShowGoalHint(ScriptEvent* e);                          /* 0x00468d30 */

// FUNCTION: LEGOLAND 0x004687f0
void SetBriefingFile(const char* name)
{
    strncpy(g_script_text1, name, 128);
    g_script_text1[127] = 0;
}

// FUNCTION: LEGOLAND 0x00468810
void SetHintsFile(const char* name)
{
    strncpy(g_script_text2, name, 128);
    g_script_text2[127] = 0;
}

/* The original tests only the upper bound; negative indices remain unchecked. */
// FUNCTION: LEGOLAND 0x00468860
void SetThemeIcon(int which, signed char on)
{
    if (which < 10) {
        g_script_bytes[which] = on;
        if (which < 4)
            SetThemeIconEnabled(which, on);
    }
}

// FUNCTION: LEGOLAND 0x00468890
signed char AddLevelFlag(int which, signed char amount)
{
    if (which < 10)
        return g_script_bytes[which] += amount;
    return 0;
}

// FUNCTION: LEGOLAND 0x004688c0
signed char GetLevelFlag(int which)
{
    if (which < 10)
        return g_script_bytes[which];
    return 0;
}

// FUNCTION: LEGOLAND 0x004688f0
void SetBridges(int which)
{
    if (which < 4)
        TriggerSwitch(which);
}

/* Original bug: inserting ahead of the head discards the old list instead
 * of linking it after e, and its per-kind counts are left unchanged. */
// FUNCTION: LEGOLAND 0x00468c80
void QueuePendingEvent(ScriptEvent* e)
{
    ScriptEvent* p = g_goal_list;
    ScriptEvent* prev = 0;
    while (p) {
        if (p->mode > e->mode)
            break;
        prev = p;
        p = p->next;
    }
    if (prev) {
        e->next = prev->next;
        prev->next = e;
    } else {
        g_goal_list = e;
        e->next = 0;
    }
    g_goal_kind_count[e->kind]++;
}

// FUNCTION: LEGOLAND 0x00468cd0
ScriptEvent* NewTimedEvent(int kind, int mode)
{
    ScriptEvent* e = NewScriptEvent(kind, mode);
    if (e)
        e->time = GetGameTimer();
    return e;
}

// FUNCTION: LEGOLAND 0x00468d10
int HintTimerDue(void)
{
    if (GetGameTimer() - g_script_start > 50000) {
        ResetScriptTimer();
        return 1;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x00468d30
int ShowGoalHint(ScriptEvent* e)
{
    ScriptEvent* h;
    if (e->hint && g_hint_strings[e->hint]) {
        if (e->flags & 4)
            h = NewTimedEvent(0, 0);
        else
            h = NewTimedEvent(0, 1);
        h->hint = e->hint;
        QueuePendingEvent(h);
        return 1;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x00468d80
void GoalCheck_Need(ScriptEvent* e, void* elem, int count)
{
    ScriptEvent* h;
    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(1, 1);
        h->elem = elem;
        h->f1c = count;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00468dc0
void GoalCheck_Connect(ScriptEvent* e, void* elem)
{
    ScriptEvent* h;
    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(2, 1);
        h->elem = elem;
        h->f1c = 0;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00468e00
void GoalCheck_Link(ScriptEvent* e, void* elem)
{
    ScriptEvent* h;
    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(2, 1);
        h->elem = elem;
        h->f1c = 1;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00468e40
void GoalCheck_Range(ScriptEvent* e, void* elem, int types, int count)
{
    ScriptEvent* h;
    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(3, 1);
        if (types < 0) types = 0;
        if (count < 0) count = 0;
        h->elem = elem;
        h->f14 = types;
        h->f1c = count;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00468ea0
void GoalCheck_RemoveRange(ScriptEvent* e, void* elem, int types, int count)
{
    ScriptEvent* h;
    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(4, 1);
        if (types < 0) types = 0;
        if (count < 0) count = 0;
        h->elem = elem;
        h->f14 = types;
        h->f1c = count;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00468f00
void GoalCheck_ClearArea(ScriptEvent* e, int count)
{
    ScriptEvent* h;
    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(5, 1);
        h->f1c = count;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00468f40
void GoalCheck_Remove(ScriptEvent* e, void* elem, int count)
{
    ScriptEvent* h;
    if (HintTimerDue() && !ShowGoalHint(e)) {
        h = NewTimedEvent(6, 1);
        h->f1c = count;
        h->elem = elem;
        QueuePendingEvent(h);
    }
}

// FUNCTION: LEGOLAND 0x00476070
void FlashButton(int buttons, int on)
{
    int i, bit;
    for (i = 0, bit = 1; i < 9; i++, bit <<= 1)
        if (buttons & bit)
            SetButtonFlash(i, on);
}

// FUNCTION: LEGOLAND 0x00476140
void SetThemeIconEnabled(int which, int on)
{
    Icon* icon = g_theme_icon[which];
    if (icon) {
        if (on) {
            icon->flags &= ~0x400;
            g_profile_themes[which] = 1;
            UpDateCurrentProfile();
        } else {
            icon->flags |= 0x400;
        }
    }
}

// FUNCTION: LEGOLAND 0x00457900
void SetCurrency(int bricks)
{
    g_bricks = bricks;
}

// FUNCTION: LEGOLAND 0x0044db40
void ResetAppraisalDeadline(void)
{
    int deadline;
    if (g_appraisal_minutes) {
        deadline = GetGameTimer();
        deadline += g_appraisal_minutes * 60000;
    } else {
        deadline = 0;
    }
    g_instant_appraisal = deadline;
}

// FUNCTION: LEGOLAND 0x00482d60
void SetHappinessFactor(int which, int value)
{
    g_mood_adjustments[which] = value;
}
