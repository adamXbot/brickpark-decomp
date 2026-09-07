/* LEGOLAND -- scope AA: report / goal-state table tier.
 * VC6 SP3 /O2 /Gy /Gd. Recovered mechanics and verification are recorded
 * in docs/lanes/scope-aa.md. Layouts are local to this translation unit.
 */

typedef struct LevelCfg {
    char pad00[0x30];
    int  f30;                         /* +0x30 */
} LevelCfg;

extern int       g_sim_832b9c;         /* 0x00832b9c */
extern int       g_appraisal_minutes;  /* 0x00832978 */
extern int       g_instant_appraisal;  /* 0x00666098 */
extern int       g_level_goal_state;   /* 0x0083297c */
extern int       g_appraisal_result;   /* 0x0066609c */
extern int       g_6687b0;             /* 0x006687b0 hold-off frame counter */
extern LevelCfg* g_level_cfg;          /* 0x004bcbf4 */

extern int  GetGameTimer(void);                          /* 0x00499430 */
extern int  ScriptRunning(void);                         /* 0x0046b280 */
extern int  PauseGameTimer(void);                        /* 0x00499380 */
extern void PauseAllSamples(void);                       /* 0x00492830 */
extern void PauseCurrentTrack(void);                     /* 0x00498920 */
extern int  RunAppraisalScreen(void);                    /* 0x004453a0 */
extern void StopScript(int stop);                        /* 0x0046b240 */
extern void sub_48a750(void);                            /* 0x0048a750 */
extern void EndLevel(int outcome);                       /* 0x00459820 */
extern void ResetAppraisalDeadline(void);                /* 0x0044db40 */
extern void ThawGameClock(void);                         /* 0x004993c0 */
extern void ResumePausedSamples(void);                   /* 0x00492850 */
extern void SetLevelEndSequence(int which, const char* s); /* 0x004597e0 */
extern int  sprintf(char* buf, const char* fmt, ...);      /* 0x0049e573 */
extern void DBPrintf(const char* fmt, ...);                /* 0x00453a20 */

typedef struct Bloke {
    char           pad00[0x0e];
    unsigned short state;          /* +0x0e  low-level AI state */
} Bloke;

typedef struct SeatSlot {
    struct SeatSlot* next;         /* +0x00 */
    char             pad04[8];
    unsigned short   seat;         /* +0x0c */
} SeatSlot;

typedef struct SeatOwner {
    char      pad00[0x2e];
    short     rider_capacity;      /* +0x2e */
    char      pad30[0xcc - 0x30];
    SeatSlot* head;                /* +0xcc */
} SeatOwner;

extern char* g_bloke_msg_slot[8];          /* 0x004b8348 */
extern char  g_bloke_msg_bank[];           /* 0x006661cc  8 x 100-byte rows */
extern int   g_bloke_msg_rot;              /* 0x006664ec */
extern int   g_cur_bloke_f81;              /* 0x00813b08 */
extern const char g_fmt_bloke_msg[];       /* 0x004b8404 "%c:%s" */
extern const char g_fmt_bloke_log[];       /* 0x004b83f0 "[Bloke %c] - %s\n" */

/* movie3.c ResetLevelGlobals zeroes the sim counter at 0x00832b9c. */
// FUNCTION: LEGOLAND 0x0044db20
void ClearSim832b9c(void)
{
    g_sim_832b9c = 0;
}

/* Dead getter; matched for completeness. */
// FUNCTION: LEGOLAND 0x0044db30
int GetSim832b9c(void)
{
    return g_sim_832b9c;
}

/* Zeroes the appraisal minutes and the instant-appraisal deadline. */
// FUNCTION: LEGOLAND 0x0044db80
void ClearAppraisalState(void)
{
    int z = 0;
    g_appraisal_minutes = z;
    g_instant_appraisal = z;
}

/* gameframe.c's appraisal-due tick: when the deadline is reached, pause the
 * sim, run the appraisal screen, and either stop the script (pass) or count
 * toward EndLevel(2) (fail). */
// FUNCTION: LEGOLAND 0x0044db90
int AppraisalDueTick(void)
{
    int now;
    int result;
    int sim;
    int goal;

    now = GetGameTimer();
    if (!ScriptRunning()) {
        if (g_instant_appraisal) {
            if (g_instant_appraisal <= now) {
                PauseGameTimer();
                PauseAllSamples();
                PauseCurrentTrack();
                g_6687b0 = 4;
                result = RunAppraisalScreen();
                g_appraisal_result = result;
                if (result) {
                    if (g_sim_832b9c > 0)
                        g_sim_832b9c++;
                    else
                        g_sim_832b9c = 1;
                    StopScript(1);
                    g_level_cfg->f30 = 1;
                    sub_48a750();
                } else {
                    sim = g_sim_832b9c;
                    if (sim < 0)
                        sim--;
                    else
                        sim = -1;
                    goal = g_level_goal_state;
                    g_sim_832b9c = sim;
                    if (goal) {
                        if (sim <= -goal)
                            EndLevel(2);
                    }
                }
                g_instant_appraisal = 0;
                ResetAppraisalDeadline();
                ThawGameClock();
                ResumePausedSamples();
                return 1;
            }
        }
    }
    return 0;
}

/* Stores the goal state, clears the sim counter, and seeds end-sequence 0. */
// FUNCTION: LEGOLAND 0x0044dc70
void SetLevelGoalState(int state, const char* text)
{
    g_level_goal_state = state;
    ClearSim832b9c();
    SetLevelEndSequence(0, text);
}

/* Rotate the eight 100-byte bloke-message scratch slots, sprintf the caller's
 * text into the last one, and mirror it through DBPrintf. Called from the
 * long-term action handlers and from sub_44f610. */
// FUNCTION: LEGOLAND 0x0044ed00
void FormatBlokeMessage(const char* text)
{
    int rot;
    int i;
    char* dest;

    for (i = 0, rot = g_bloke_msg_rot; i < 8; i++)
        g_bloke_msg_slot[i] = &g_bloke_msg_bank[((rot + i) & 7) * 100];
    g_bloke_msg_rot = rot + 1;
    dest = g_bloke_msg_slot[7];
    sprintf(dest, g_fmt_bloke_msg, g_cur_bloke_f81, text);
    DBPrintf(g_fmt_bloke_log, g_cur_bloke_f81, text);
}

/* Long-term action table slot 0x01: set the bloke's low-level AI state to 4. */
// FUNCTION: LEGOLAND 0x0044f170
void BlokeAction_SetState4(Bloke* b)
{
    b->state = 4;
}

/* rides.c CountBlokesAtRideID: how many seat-list slots share ride_id. */
// FUNCTION: LEGOLAND 0x0044f3d0
int CountBlokesAtRideID(SeatOwner* owner, unsigned short* ride_id)
{
    SeatSlot* s;
    int count;
    unsigned short id;

    count = 0;
    s = owner->head;
    if (!s)
        return 0;
    id = *ride_id;
    do {
        if (s->seat == id)
            count++;
        s = s->next;
    } while (s);
    return count;
}

/* True when the seat list already holds rider_capacity entries for ride_id. */
// FUNCTION: LEGOLAND 0x0044f400
int SeatListFull(SeatOwner* owner, unsigned short* ride_id)
{
    return CountBlokesAtRideID(owner, ride_id) >= owner->rider_capacity;
}
