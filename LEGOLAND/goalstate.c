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
#ifndef LEGOLAND_PORTABLE
extern void PauseCurrentTrack(void);                     /* 0x00498920 */
#else
extern int PauseCurrentTrack(void);                     /* 0x00498920 */
#endif
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

typedef struct Pos {
    int x;
    int y;
} Pos;

typedef struct Bloke {
    char           pad00[0x04];
    void*          person;         /* +0x04 */
    char           pad08[0x0e - 0x08];
    unsigned short state;          /* +0x0e  low-level AI state */
    unsigned short step;           /* +0x10 */
    char           pad12[0x14 - 0x12];
    void*          current_item;   /* +0x14 */
    void*          last_elem;      /* +0x18  last attraction Elem* */
    char           pad1c[0x24 - 0x1c];
    Pos            target;         /* +0x24 */
    Pos            saved;          /* +0x2c  ride-route destination */
    char           pad34[0x35 - 0x34];
    unsigned char  b35;            /* +0x35 */
    char           pad36[0x58 - 0x36];
    int            seat_arg;       /* +0x58  third JoinSeatList arg */
    char           pad5c[0x60 - 0x5c];
    unsigned char  action;         /* +0x60  short-term action */
    char           pad61;
    unsigned short flags;          /* +0x62 */
    unsigned char  f64;            /* +0x64 */
    char           pad65[0x68 - 0x65];
    Pos            world;          /* +0x68 */
    char           pad70[0x72 - 0x70];
    unsigned char  new_dir;        /* +0x72  EnterPark stores shrunk dir */
    unsigned char  f73;            /* +0x73  LeavePark raw angle scratch */
    char           pad74[0x7a - 0x74];
    short          mood;           /* +0x7a */
    char           pad7c[0x81 - 0x7c];
    unsigned char  name_letter;    /* +0x81  published to g_cur_bloke_f81 */
    unsigned char  stuck;          /* +0x82  SuggestNextMove fail streak */
    char           pad83[0x98 - 0x83];
    unsigned char  path[0x14];     /* +0x98  CalcMoveLine scratch */
} Bloke;

typedef struct SeatSlot {
    struct SeatSlot* next;         /* +0x00 */
    struct SeatSlot* prev;         /* +0x04 */
    Bloke*           bloke;        /* +0x08 */
    unsigned short   seat;         /* +0x0c */
    unsigned short   pad0e;
    void*            person;       /* +0x10 */
} SeatSlot;

typedef struct SeatOwner {
    char      pad00[0x0c];
    int       base_x;              /* +0x0c  footprint origin */
    int       base_y;              /* +0x10 */
    char      pad14[0x2e - 0x14];
    short     rider_capacity;      /* +0x2e */
    char      pad30[0x3a - 0x30];
    short     mood_scale;          /* +0x3a  AdjustMood scale */
    char      pad3c[0x40 - 0x3c];
    int       exit_oy;             /* +0x40  leave-walk y offset */
    int       off_x;               /* +0x44  entrance approach offset */
    int       off_y;               /* +0x48 */
    char      pad4c[0x78 - 0x4c];
    const char* name;              /* +0x78 */
    char      pad7c[0xc4 - 0x7c];
    void*     elem;                /* +0xc4 */
    char      padc8[0xcc - 0xc8];
    SeatSlot* head;                /* +0xcc */
} SeatOwner;

typedef struct MapObj {
    char  pad00[0x0c];
    void* cls;                     /* +0x0c  ObjDef* / SeatOwner* */
} MapObj;

typedef struct MapCell {
    void*          obj;            /* +0x00 */
    unsigned char  x;              /* +0x04 */
    unsigned char  y;              /* +0x05 */
    char           pad06[0x0c - 0x06];
    unsigned short flags;          /* +0x0c  byte-or'd at bit 2 by JoinSeatList */
    char           pad0e[0x14 - 0x0e];
} MapCell;                         /* 0x14 */

typedef struct MapHdr {
    char           pad00[0x14];
    unsigned short width;          /* +0x14 */
    unsigned short height;         /* +0x16 */
} MapHdr;

typedef struct Elem {
    char  pad00[0x0c];
    void* data;                    /* +0x0c  ObjDef* / SeatOwner* */
    char  pad10[0x1c - 0x10];
    int   flags;                   /* +0x1c  bit 0x100000 = queue-join style */
} Elem;

typedef struct BytePos {
    unsigned char x;
    unsigned char y;
} BytePos;

typedef struct InstFlags {
    char           pad00[0x0c];
    unsigned short flags;          /* +0x0c */
} InstFlags;

extern char* g_bloke_msg_slot[8];          /* 0x004b8348 */
extern char  g_bloke_msg_bank[];           /* 0x006661cc  8 x 100-byte rows */
extern int   g_bloke_msg_rot;              /* 0x006664ec */
extern int   g_cur_bloke_f81;              /* 0x00813b08 */
extern const char g_fmt_bloke_msg[];       /* 0x004b8404 "%c:%s" */
extern const char g_fmt_bloke_log[];       /* 0x004b83f0 "[Bloke %c] - %s\n" */
extern const char g_fmt_no_alloc[];        /* 0x004b8458 */
extern const char g_fmt_no_instance[];     /* 0x004b8480 */
extern Elem*      g_entrance_elem;         /* 0x006661c4 */
extern MapCell**  g_map_rows;              /* 0x00801400 */
extern MapHdr*    g_map;                   /* 0x004bcbf4 */
extern int        g_map_dirty;             /* 0x00668610 */

extern void* HeapAlloc_w(unsigned int size);                         /* 0x0049e4ff */
extern MapCell* GetFirstObjectMatching(void* obj);                   /* 0x0045a910 */
extern int  BumpSlotCounter(int* xy);                                /* 0x00489f90 */
extern unsigned short GetObjectUID(Pos* wpos, SeatOwner* def);       /* 0x0048a3e0 */
extern void PutBlokeInList(SeatOwner* owner, SeatSlot* slot);        /* 0x0044f430 */
extern void* memset(void*, int, unsigned);                          /* 0x004a0320 */
#pragma intrinsic(memset)
extern void NewLongTermAction(Bloke* b, int action);                 /* 0x0044e760 */
extern void PushLongTermAction(Bloke* b);                            /* 0x0044ebb0 */
extern int  CalcMoveLine(Pos from, Pos to, void* path);              /* 0x00480740 */
extern int  SuggestNextMove(Pos* from, Pos* to, Pos* out);           /* 0x00482050 */
extern int  PTPSuggestNextMove(Pos* from, Pos* to, Pos* out);        /* 0x004824d0 */
extern int  NewDirForAction(Bloke* b, unsigned char dir);            /* 0x004833d0 */
extern void DestroyBloke(Bloke* b);                                  /* 0x00483010 */
extern void RateBlokeOnLeaving(int score);                           /* 0x004633f0 */
extern int  g_enter_off_x;                                           /* 0x004b8318 */
extern int  g_enter_off_y;                                           /* 0x004b831c */
/* The spot LeavePark routes to is a whole Pos: mapobj.c and savegame.c write
 * its y as g_entrance_y (0x004b8324). Declared as a lone int, the portable
 * closure gave x four bytes of its own, so the Pos read took y from alignment
 * padding -- every leaving visitor aimed at map row 0. Same address, same
 * bytes; only the extent the closure sees changes. */
extern Pos  g_entrance_x;                                            /* 0x004b8320 Pos (y is g_entrance_y) */
extern int  g_leave_dx;                                              /* 0x004b8328 */
extern int  g_leave_dy;                                              /* 0x004b832c */
extern int  g_visitor_count;                                         /* 0x006661bc */
extern const char g_fmt_stuck_ptp[];                                 /* 0x004b8434 */
extern const char g_fmt_wandering[];                                 /* 0x004b8424 */
extern const char g_fmt_kill_minifig[];                              /* 0x004b840c */

extern void BuildObjInfoList(void);                                  /* 0x00481200 */
extern void CalculateRideCodes(Bloke* b);                            /* 0x004815e0 */
extern void ResetBestPtr(void);                                      /* 0x00481690 */
extern int  ShuffleObjKeys(Pos* out_pos, void** out_obj);            /* 0x00481610 */
extern int  Calc_Item_Attractiveness(SeatOwner* item, Bloke* b, int viewing); /* 0x004814c0 */
extern int  GetBlokeNum(Bloke* b);                                   /* 0x00482fb0 */
extern int  GetBlokeCounter(SeatOwner* item, int idx);               /* 0x00480ee0 */
extern void IncrementBlokeCounter(SeatOwner* item, int idx);         /* 0x00480ec0 */
extern MapCell* GetNextObjectMatching(MapCell* c, void* obj);        /* 0x0045a940 */
extern int  IsObjectRunning(SeatOwner* cls, BytePos* at);            /* 0x0044f360 */
extern InstFlags* GetInstanceOfClass(void* cls, BytePos* key);       /* 0x0048a0c0 */
extern int  AdjustMood(Bloke* b, int event, int scale);              /* 0x00482df0 */
extern int  rand(void);                                              /* 0x0049e4b2 */
extern const char g_fmt_just_been[];                                 /* 0x004b858c */
extern const char g_fmt_not_worth[];                                 /* 0x004b856c */
extern const char g_fmt_ill_go[];                                    /* 0x004b8554 */
extern const char g_fmt_go_home[];                                   /* 0x004b8524 */
extern const char g_fmt_ride_full[];                                 /* 0x004b8500 */
extern const char g_fmt_not_working[];                               /* 0x004b84d4 */
extern const char g_fmt_going_on[];                                  /* 0x004b84bc */
extern const char g_fmt_cant_get_on[];                               /* 0x004b84a0 */

int JoinSeatList(Bloke* bloke, SeatOwner* owner, int seat_arg);

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

/* Long-term action table slot 0x03: route toward the entrance, PTP when stuck,
 * wander hand-off, join the exit seat list, walk out, rate, and destroy.
 *
 * CalcMoveLine arms mirror Garderner_Repair (bigsim.c): assign target from the
 * local `out`, then CalcMoveLine(b->world, out, path) so VC6 keeps edi as
 * &world and interleaves the target.y store into the arg pushes.
 *
 * Case 1: `lim = 5` byte local keeps the JT bound in ecx so stuck prefers dl
 * and `mov [action],cl`; `++b->stuck == 8` gives inc/mov al,dl/store/cmp.
 * Case 5: ternary `(f64&1) ? 6 : 0xa` yields and dl,1/neg/sbb/and dl,0xfc/
 * add dl,0xa. Cases 11/12 end in `break` (not `return`): the IR-level suffix
 * merge then hosts the CalcMoveLine tail in the layout-LAST arm (case 11 jmps
 * forward) and keeps the deferred `add esp,0x20`; with `return` the arms get
 * inline epilogues and only the post-codegen cross-jump fires (backward jmp
 * into case 11). */
// FUNCTION: LEGOLAND 0x0044ed70
void BlokeAction_LeavePark(Bloke* b)
{
    Pos out;
    char msg[100];
    MapCell* cell;
    SeatOwner* def;
    unsigned char a;
    int r;

    switch (b->action) {
    case 0:
        b->flags |= 8;
        b->stuck = 0;
        b->action = 1;
        /* fallthrough */
    case 1: {
        unsigned char lim = 5;
        r = SuggestNextMove(&b->world, (Pos*)&g_entrance_x, &out) + 3;
        if ((unsigned)r > lim)
            return;
        switch (r) {
        case 1: /* SuggestNextMove == -2 */
            b->action = lim;
            return;
        case 0:
        case 2:
        case 3: /* -3 / -1 / 0 */ {
            b->state = 4;
            b->action = 2;
            if (++b->stuck == 8)
                b->action = lim;
            return;
        }
        case 5: /* == 2 */
            b->target.x = out.x;
            b->target.y = out.y;
            a = (unsigned char)(CalcMoveLine(b->world, out, b->path) + 0x10);
            b->state = 6;
            b->f73 = a;
            NewDirForAction(b, (unsigned char)((a >> 5) + 3));
            if (b->f64) {
                b->state = 4;
                b->action = 2;
            } else {
                b->action = 0xa;
            }
            return;
        case 4: /* == 1 */
            b->target.x = out.x;
            b->target.y = out.y;
            a = (unsigned char)(CalcMoveLine(b->world, out, b->path) + 0x10);
            b->state = 6;
            b->f73 = a;
            NewDirForAction(b, (unsigned char)((a >> 5) + 3));
            if (b->f64) {
                b->state = 4;
                b->action = 2;
            } else {
                b->action = 0;
            }
            return;
        }
        return;
    }
    case 2:
        b->flags |= 8;
        b->action = 1;
        return;
    case 5:
        b->flags |= 8;
        sprintf(msg, g_fmt_stuck_ptp);
        FormatBlokeMessage(msg);
        r = PTPSuggestNextMove(&b->world, (Pos*)&g_entrance_x, &out);
        switch (r) {
        case 2:
            b->target.x = out.x;
            b->target.y = out.y;
            a = (unsigned char)(CalcMoveLine(b->world, out, b->path) + 0x10);
            b->state = 0xb;
            b->f73 = a;
            NewDirForAction(b, (unsigned char)((a >> 5) + 3));
            /* Ternary 6:0xa (not 0xa+(-4)) yields and dl,0xfc after sbb. */
            b->action = (unsigned char)((b->f64 & 1) ? 6 : 0xa);
            return;
        case 1:
            b->target.x = out.x;
            b->target.y = out.y;
            a = (unsigned char)(CalcMoveLine(b->world, out, b->path) + 0x10);
            b->state = 0xb;
            b->f73 = a;
            NewDirForAction(b, (unsigned char)((a >> 5) + 3));
            if (b->f64 & 1)
                b->action = 6;
            return;
        case 0:
            b->state = 4;
            b->action = 6;
            return;
        }
        return;
    case 6:
        sprintf(msg, g_fmt_wandering);
        FormatBlokeMessage(msg);
        b->state = 4;
        b->action = 5;
        return;
    case 10:
        if (JoinSeatList(b, (SeatOwner*)g_entrance_elem->data, 0)) {
            a = b->action;
            b->flags |= 8;
            a++;
            b->current_item = g_entrance_elem;
            b->action = a;
            PushLongTermAction(b);
            NewLongTermAction(b, 5);
        }
        return;
    case 11:
        cell = GetFirstObjectMatching(g_entrance_elem);
        def = (SeatOwner*)g_entrance_elem->data;
        b->target.x = (cell->x + def->off_x + 6) << 8;
        b->target.y = (cell->y + def->exit_oy + 8) << 8;
        a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
        b->state = 7;
        b->f73 = a;
        NewDirForAction(b, (unsigned char)((a >> 5) + 3));
        b->action++;
        break;
    case 12:
        RateBlokeOnLeaving(b->mood);
        b->target.x += g_leave_dx;
        b->target.y += g_leave_dy;
        a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
        b->state = 7;
        b->f73 = a;
        NewDirForAction(b, (unsigned char)((a >> 5) + 3));
        b->action++;
        break;
    case 13:
        DBPrintf(g_fmt_kill_minifig, b);
        DestroyBloke(b);
        g_visitor_count--;
        return;
    }
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

/* blokelist's seat-list join: malloc a 20-byte slot, bind the bloke to the
 * ride, stamp a ride_id (entrance cell or GetObjectUID), mark the map cell,
 * and link via PutBlokeInList. */
// FUNCTION: LEGOLAND 0x0044f4a0
int JoinSeatList(Bloke* bloke, SeatOwner* owner, int seat_arg)
{
    SeatSlot* slot;
    MapCell* cell;
    int xy[2];
    int x;
    int y;

    slot = (SeatSlot*)HeapAlloc_w(0x14);
    if (slot) {
        cell = GetFirstObjectMatching(owner->elem);
        if (cell) {
            xy[0] = cell->x;
            xy[1] = cell->y;
            BumpSlotCounter(xy);

            memset(slot, 0, 0x14);

            slot->bloke = bloke;
            bloke->seat_arg = seat_arg;
            slot->person = bloke->person;
            bloke->flags |= 0x20;
            bloke->state = 0;
            bloke->step = 0;
            bloke->b35 = 0;

            if (owner == (SeatOwner*)g_entrance_elem->data) {
                cell = GetFirstObjectMatching(g_entrance_elem);
                *(unsigned char*)&slot->seat = cell->x;
                *((unsigned char*)&slot->seat + 1) = cell->y;
            } else {
                slot->seat = GetObjectUID(&bloke->world, owner);
            }

            x = *(unsigned char*)&slot->seat;
            y = *((unsigned char*)&slot->seat + 1);
            xy[0] = x;
            xy[1] = y;
            if (x < 0 || x >= g_map->width || y < 0 || y >= g_map->height)
                cell = 0;
            else
                cell = &g_map_rows[y][x];
            /* Original bug: the flag write is unguarded, so an OOB ride_id ors [0xc]. */
            cell->flags |= 4;

            PutBlokeInList(owner, slot);
            g_map_dirty |= 0x20;
            return 1;
        }
        DBPrintf(g_fmt_no_instance, owner->name);
        return 0;
    }
    DBPrintf(g_fmt_no_alloc, owner->name);
    return 0;
}

/* Cell lookup; (y, x) arg order so VC6 emits x-shift before y-shift and a
 * separate `test eax,eax` (see objmap2.c CellYX). */
static __inline MapCell* FootCell(int y, int x)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

/* Both footprint sums through a Pos so VC6 computes them before either cmp. */
static __inline int FootHit(MapCell* c, SeatOwner* def, int x, int y)
{
    Pos p;

    p.x = c->x + def->base_x;
    p.y = c->y + def->base_y;
    return p.x == x && p.y == y;
}

/* True when world tile (pos>>8) is the footprint origin of an instance of
 * def, found by probing the four orthogonal neighbour cells. Called from
 * sub_44f610. Flat probes (unlike GetObjectUID's nested above/below). */
// FUNCTION: LEGOLAND 0x0044f180
int PosOnObjectFootprint(Pos* pos, SeatOwner* def)
{
    MapCell* c;

    c = FootCell((pos->y >> 8) - 1, pos->x >> 8);
    if (c && (c->flags & 0x80) && c->obj && ((MapObj*)c->obj)->cls == def
        && FootHit(c, def, pos->x >> 8, pos->y >> 8))
        return 1;

    c = FootCell((pos->y >> 8) + 1, pos->x >> 8);
    if (c && (c->flags & 0x80) && c->obj && ((MapObj*)c->obj)->cls == def
        && FootHit(c, def, pos->x >> 8, pos->y >> 8))
        return 1;

    c = FootCell(pos->y >> 8, (pos->x >> 8) - 1);
    if (c && (c->flags & 0x80) && c->obj && ((MapObj*)c->obj)->cls == def
        && FootHit(c, def, pos->x >> 8, pos->y >> 8))
        return 1;

    c = FootCell(pos->y >> 8, (pos->x >> 8) + 1);
    if (c && (c->flags & 0x80) && c->obj && ((MapObj*)c->obj)->cls == def
        && FootHit(c, def, pos->x >> 8, pos->y >> 8))
        return 1;

    return 0;
}

/* Long-term action table slot 0x02: walk to the park entrance, then join its
 * seat list and hand off to LT action 5. */
// FUNCTION: LEGOLAND 0x0044ebf0
void BlokeAction_EnterPark(Bloke* b)
{
    MapCell* cell;
    SeatOwner* def;
    unsigned char dir;

    switch (b->action) {
    case 0:
        cell = GetFirstObjectMatching(g_entrance_elem);
        def = (SeatOwner*)g_entrance_elem->data;
        b->flags |= 8;
        b->target.x = (cell->x + def->off_x + 6) << 8;
        b->target.y = (cell->y + def->off_y - 5) << 8;
        b->world.x = b->target.x + g_enter_off_x;
        b->world.y = b->target.y + g_enter_off_y;
        dir = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
        dir = (unsigned char)((dir >> 5) + 3);
        b->state = 7;
        b->new_dir = dir;
        b->action++;
        break;
    case 1:
        b->current_item = g_entrance_elem;
        if (JoinSeatList(b, (SeatOwner*)g_entrance_elem->data, 0)) {
            b->action++;
            PushLongTermAction(b);
            NewLongTermAction(b, 5);
            g_map_dirty |= 0x40;
        }
        break;
    case 2:
        b->flags &= ~8;
        NewLongTermAction(b, 6);
        break;
    }
}

/* Long-term action table slot 0x06: pick a ride, walk to it, join seat list.
 * Frame 0x78: {obj, uid, pad, next, msg[100]}. ebx=&saved/&world, edi=more,
 * ebp from case-1 def. The four `action = 2` exits are `break`s into one
 * post-switch store; the default arm must survive early jump threading (see
 * lane notes) so the `ja` lands on the shared epilogue, not on that store. */
// FUNCTION: LEGOLAND 0x0044f610
void BlokeAction_PickRide(Bloke* b)
{
    struct {
        void*          obj;
        unsigned short uid;
        unsigned short _u;
        int            _pad;
        Pos            next;
        char           msg[100];
    } fr;
    Pos* dest;
    int more;
    int y;
    MapCell* cell;

    g_cur_bloke_f81 = b->name_letter;

    switch (b->action) {
    case 0:
        BuildObjInfoList();
        CalculateRideCodes(b);
        ResetBestPtr();
        dest = &b->saved;
        if (ShuffleObjKeys(dest, &fr.obj) == 0)
            break;
        more = 1;
        do {
            if (b->last_elem == ((SeatOwner*)fr.obj)->elem) {
                sprintf(fr.msg, g_fmt_just_been, ((SeatOwner*)fr.obj)->name);
                FormatBlokeMessage(fr.msg);
            } else if (Calc_Item_Attractiveness((SeatOwner*)fr.obj, b, 0) > 10) {
                sprintf(fr.msg, g_fmt_ill_go,
                        Calc_Item_Attractiveness((SeatOwner*)fr.obj, b, 0),
                        ((SeatOwner*)fr.obj)->name);
                FormatBlokeMessage(
                    (b->current_item = ((SeatOwner*)fr.obj)->elem, fr.msg));
                b->action = 1;
                b->stuck = 0;
                if (more)
                    return;
                break;
            } else {
                sprintf(fr.msg, g_fmt_not_worth, ((SeatOwner*)fr.obj)->name);
                FormatBlokeMessage(fr.msg);
                if (!GetBlokeCounter((SeatOwner*)fr.obj, GetBlokeNum(b)))
                    IncrementBlokeCounter((SeatOwner*)fr.obj, GetBlokeNum(b));
            }
            more = ShuffleObjKeys(dest, &fr.obj);
        } while (more);
        sprintf(fr.msg, g_fmt_go_home);
        FormatBlokeMessage(fr.msg);
        NewLongTermAction(b, 3);
        return;

    case 1:
        dest = &b->saved;
        switch (SuggestNextMove(&b->world, dest, &fr.next) + 3) {
        case 1:
            b->state = 0xa;
            return;
        case 0:
        case 2:
        case 3:
            if (PosOnObjectFootprint(dest, (SeatOwner*)((Elem*)b->current_item)->data) == 0)
                goto state4;
            more = (b->saved.x >> 8) - ((SeatOwner*)((Elem*)b->current_item)->data)->base_x;
            {
                int y = (b->saved.y >> 8) - ((SeatOwner*)((Elem*)b->current_item)->data)->base_y;
                MapCell* cell;
                SeatOwner* def = (SeatOwner*)((Elem*)b->current_item)->data;
                if (more >= 0 && more < (int)g_map->width
                    && y >= 0 && y < (int)g_map->height)
                    cell = &g_map_rows[y][more];
                else
                    cell = 0;
                cell = GetNextObjectMatching(cell, b->current_item);
                if (!cell)
                    cell = GetFirstObjectMatching(b->current_item);
                if (!cell) {
                    sprintf(fr.msg, g_fmt_wandering);
                    FormatBlokeMessage(fr.msg);
                    b->state = 4;
                    b->action = (unsigned char)(b->action + 1);
                    return;
                }
                {
                    int cx, cy;
                    cx = (unsigned char)cell->x;
                    cy = (unsigned char)cell->y;
                    if (cx != more || cy != y) {
                        b->saved.x = ((def->base_x + cx) << 8) + 0x80;
                        b->saved.y = ((def->base_y + cy) << 8) + 0x80;
                        return;
                    }
                }
            }
        state4:
            b->state = 4;
            return;
        case 5:
            b->target.x = fr.next.x;
            b->target.y = fr.next.y;
            b->f73 = (unsigned char)(CalcMoveLine(b->world, fr.next, b->path) + 0x10);
            b->state = 6;
            NewDirForAction(b, (unsigned char)((b->f73 >> 5) + 3));
            b->action = (unsigned char)(b->f64 ? 0 : 0xa);
            return;
        case 4:
            b->target.x = fr.next.x;
            b->target.y = fr.next.y;
            b->f73 = (unsigned char)(CalcMoveLine(b->world, fr.next, b->path) + 0x10);
            b->state = 6;
            NewDirForAction(b, (unsigned char)((b->f73 >> 5) + 3));
            b->action = (unsigned char)(b->f64 == 0);
            return;
        }
        return;

    case 2:
    case 3:
        sprintf(fr.msg, g_fmt_wandering);
        FormatBlokeMessage(fr.msg);
        b->state = 4;
        b->action = (unsigned char)(b->action + 1);
        return;

    case 4:
        b->action = 0;
        return;

    case 5:
        sprintf(fr.msg, g_fmt_stuck_ptp);
        FormatBlokeMessage(fr.msg);
        dest = &b->saved;
        switch (PTPSuggestNextMove(&b->world, dest, &fr.next)) {
        case 2:
            b->target.x = fr.next.x;
            b->target.y = fr.next.y;
            b->f73 = (unsigned char)(CalcMoveLine(b->world, fr.next, b->path) + 0x10);
            b->state = 0xb;
            NewDirForAction(b, (unsigned char)((b->f73 >> 5) + 3));
            b->action = (unsigned char)((b->f64 & 1) ? 6 : 0xa);
            return;
        case 1:
            b->target.x = fr.next.x;
            b->target.y = fr.next.y;
            b->f73 = (unsigned char)(CalcMoveLine(b->world, fr.next, b->path) + 0x10);
            b->state = 0xb;
            NewDirForAction(b, (unsigned char)((b->f73 >> 5) + 3));
            if (b->f64 & 1)
                b->action = 6;
            return;
        case 0:
            b->state = 4;
            b->action = 6;
            return;
        }
        return;

    case 6:
        sprintf(fr.msg, g_fmt_wandering);
        FormatBlokeMessage(fr.msg);
        b->state = 4;
        b->action = 5;
        return;

    case 10:
        dest = &b->world;
        if (PosOnObjectFootprint(dest, (SeatOwner*)((Elem*)b->current_item)->data) == 0)
            break;
        fr.uid = GetObjectUID(dest, (SeatOwner*)((Elem*)b->current_item)->data);
        if (SeatListFull((SeatOwner*)((Elem*)b->current_item)->data, &fr.uid)) {
            sprintf(fr.msg, g_fmt_ride_full);
            FormatBlokeMessage(fr.msg);
            AdjustMood(b, 0, ((SeatOwner*)((Elem*)b->current_item)->data)->mood_scale);
            if (!GetBlokeCounter((SeatOwner*)((Elem*)b->current_item)->data, GetBlokeNum(b)))
                IncrementBlokeCounter((SeatOwner*)((Elem*)b->current_item)->data, GetBlokeNum(b));
        } else {
            if (IsObjectRunning((SeatOwner*)((Elem*)b->current_item)->data, (BytePos*)&fr.uid) == 0) {
                sprintf(fr.msg, g_fmt_not_working);
                FormatBlokeMessage(fr.msg);
                AdjustMood(b, 1, ((SeatOwner*)((Elem*)b->current_item)->data)->mood_scale);
                if (!GetBlokeCounter((SeatOwner*)((Elem*)b->current_item)->data, GetBlokeNum(b)))
                    IncrementBlokeCounter((SeatOwner*)((Elem*)b->current_item)->data, GetBlokeNum(b));
                break;
            }
            more = ((unsigned char*)&fr.uid)[0];
            y = ((unsigned char*)&fr.uid)[1];
            if (more >= 0 && more < (int)g_map->width
                && y >= 0 && y < (int)g_map->height)
                cell = &g_map_rows[y][more];
            else
                cell = 0;
            if (GetInstanceOfClass(((MapObj*)cell->obj)->cls,
                                   (BytePos*)&fr.uid)->flags & 2)
                goto cant_get_on;
            sprintf(fr.msg, g_fmt_going_on);
            FormatBlokeMessage(fr.msg);
            NewLongTermAction(b, 5);
            if (((Elem*)b->current_item)->flags & 0x100000) {
                if (!JoinSeatList(b, (SeatOwner*)((Elem*)b->current_item)->data, 0))
                    goto cant_get_on;
                if (!SeatListFull((SeatOwner*)((Elem*)b->current_item)->data, &fr.uid))
                    return;
                more = ((unsigned char*)&fr.uid)[0];
                y = ((unsigned char*)&fr.uid)[1];
                if (more >= 0 && more < (int)g_map->width
                    && y >= 0 && y < (int)g_map->height)
                    cell = &g_map_rows[y][more];
                else
                    cell = 0;
                {
                    InstFlags* p = GetInstanceOfClass(((MapObj*)cell->obj)->cls, (BytePos*)&fr.uid);
                    p->flags |= 2;
                }
                return;
            }
            more = ((unsigned char*)&fr.uid)[0];
            y = ((unsigned char*)&fr.uid)[1];
            if (more >= 0 && more < (int)g_map->width
                && y >= 0 && y < (int)g_map->height)
                cell = &g_map_rows[y][more];
            else
                cell = 0;
            GetInstanceOfClass(((MapObj*)cell->obj)->cls,
                               (BytePos*)&fr.uid);
            if (JoinSeatList(b, (SeatOwner*)((Elem*)b->current_item)->data,
                             (rand() & 0x1ff) + 0xc8))
                return;
        cant_get_on:
            sprintf(fr.msg, g_fmt_cant_get_on);
            FormatBlokeMessage(fr.msg);
            AdjustMood(b, 0, ((SeatOwner*)((Elem*)b->current_item)->data)->mood_scale);
            if (!GetBlokeCounter((SeatOwner*)((Elem*)b->current_item)->data, GetBlokeNum(b)))
                IncrementBlokeCounter((SeatOwner*)((Elem*)b->current_item)->data, GetBlokeNum(b));
            b->action = 2;
            return;
        }
        break;
    default:
        /* Always-true (unsigned char >= 0) so this arm is non-trivial in the
         * early threading pass; the fold happens late, after `ja` has been
         * bound to the epilogue and the post-store edge has been removed. */
        if (b->action >= 0)
            goto done;
    }
    b->action = 2;
done:
    ;
}
