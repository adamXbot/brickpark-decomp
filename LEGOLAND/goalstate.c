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
    char           pad18[0x24 - 0x18];
    Pos            target;         /* +0x24 */
    char           pad2c[0x35 - 0x2c];
    unsigned char  b35;            /* +0x35 */
    char           pad36[0x58 - 0x36];
    int            seat_arg;       /* +0x58  third JoinSeatList arg */
    char           pad5c[0x60 - 0x5c];
    unsigned char  action;         /* +0x60  short-term action */
    char           pad61;
    unsigned short flags;          /* +0x62 */
    char           pad64[0x68 - 0x64];
    Pos            world;          /* +0x68 */
    char           pad70[0x72 - 0x70];
    unsigned char  new_dir;        /* +0x72 */
    char           pad73[0x98 - 0x73];
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
    char      pad30[0x44 - 0x30];
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
    void* data;                    /* +0x0c  ObjDef* */
} Elem;

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
extern int  g_enter_off_x;                                           /* 0x004b8318 */
extern int  g_enter_off_y;                                           /* 0x004b831c */

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

/* True when world tile (pos>>8) is the footprint origin of an instance of
 * def, found by probing the four orthogonal neighbour cells. Called from
 * sub_44f610.
 *
 * Residual: flat four-probe form is ~46% (register/landing-pad differences
 * vs the original's g_map reload pads between probes). Nested above/below
 * (GetObjectUID shape) dropped to 33%. Needs the same probe-entry g_map
 * landing-pad treatment recorded on GetObjectUID in objmap2.c. */
// WIP-FUNCTION: LEGOLAND 0x0044f180  (46%, g_map landing pads between probes)
int PosOnObjectFootprint(Pos* pos, SeatOwner* def)
{
    int x = pos->x >> 8;
    int y = pos->y >> 8;
    MapCell* c;
    MapObj* obj;
    Pos p;

    if (x >= 0 && x < g_map->width && y - 1 >= 0 && y - 1 < g_map->height) {
        c = &g_map_rows[y - 1][x];
        if (c && (c->flags & 0x80) && c->obj) {
            obj = (MapObj*)c->obj;
            if (obj->cls == def) {
                p.x = c->x + def->base_x;
                p.y = c->y + def->base_y;
                if (p.x == x && p.y == y)
                    return 1;
            }
        }
    }
    if (x >= 0 && x < g_map->width && y + 1 >= 0 && y + 1 < g_map->height) {
        c = &g_map_rows[y + 1][x];
        if (c && (c->flags & 0x80) && c->obj) {
            obj = (MapObj*)c->obj;
            if (obj->cls == def) {
                p.x = c->x + def->base_x;
                p.y = c->y + def->base_y;
                if (p.x == x && p.y == y)
                    return 1;
            }
        }
    }
    if (x - 1 >= 0 && x - 1 < g_map->width && y >= 0 && y < g_map->height) {
        c = &g_map_rows[y][x - 1];
        if (c && (c->flags & 0x80) && c->obj) {
            obj = (MapObj*)c->obj;
            if (obj->cls == def) {
                p.x = c->x + def->base_x;
                p.y = c->y + def->base_y;
                if (p.x == x && p.y == y)
                    return 1;
            }
        }
    }
    if (x + 1 >= 0 && x + 1 < g_map->width && y >= 0 && y < g_map->height) {
        c = &g_map_rows[y][x + 1];
        if (c && (c->flags & 0x80) && c->obj) {
            obj = (MapObj*)c->obj;
            if (obj->cls == def) {
                p.x = c->x + def->base_x;
                p.y = c->y + def->base_y;
                if (p.x == x && p.y == y)
                    return 1;
            }
        }
    }
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
