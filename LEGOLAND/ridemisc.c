/* LEGOLAND -- small ride helpers pulled off the unmatched-callee frontier:
 * the MECHANICS HUT / POTTING SHED evictor, the jungle-cruise boat's route
 * step, the two visitor "favourite" randomisers, the HELICOPTERS full-flag
 * setter and the EARTH SLIDE car launcher.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  None of
 * these is exported; every extent was taken from the disassembly by control
 * flow (tools/audit.py).  Struct field OFFSETS, record sizes and global
 * addresses are load-bearing; the names are ours.  Types are declared LOCALLY
 * on purpose (legoland.h is owned elsewhere) and mirror the ones in
 * ridecb9.c, junglecruise.c, rides.c, mechrides.c and ridecb1.c, which own
 * the rest of these subsystems.
 * ========================================================================= */

/* ---- shared map types (same offsets as ridecb9.c / junglecruise.c) ------ */
typedef struct Pos { int x; int y; } Pos;

/* A packed 2-byte map square passed BY VALUE, and its 16-bit view. */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;


/* =========================================================================
 * 0x0043d7c0 -- MechanicsHut_EvictRiders.
 * ========================================================================= */

/* A person as this file sees it (ridecb1.c / ridecb9.c's Bloke, same
 * offsets); shared with EarthSlide_LaunchCar below. */
typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;           /* +0x0e  low-level AI state (7 = walking) */
    unsigned char  pad10[0x24 - 0x10];
    Pos            target;          /* +0x24  walk target, 24.8 */
    unsigned char  pad2c[0x36 - 0x2c];
    unsigned char  seat;            /* +0x36  job slot / carriage index */
    unsigned char  pad37[0x60 - 0x37];
    unsigned char  action;          /* +0x60  state-machine stage */
    unsigned char  pad61;
    unsigned short flags62;         /* +0x62  8 = using this ride, 0x40 = queued */
    unsigned char  pad64[4];
    Pos            world;           /* +0x68  world position, 24.8 */
    unsigned char  pad70[0x73 - 0x70];
    unsigned char  new_dir;         /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14];      /* +0x98  CalcMoveLine scratch */
} Bloke;

/* A rider slot (rides.c's RiderNode). */
typedef struct RiderNode {
    struct RiderNode* next;         /* +0x00 */
    struct RiderNode* prev;         /* +0x04 */
    Bloke*            bloke;        /* +0x08 */
    unsigned short    ride_id;      /* +0x0c  packed {x,y} of the instance */
    unsigned short    pad0e;
    void*             person;       /* +0x10 */
} RiderNode;

/* The 0xd0-byte class record (objmap2.c's ObjDef / ridecb9.c's RideObject);
 * only the door offset and the class-wide rider list matter here. */
typedef struct RideObject {
    unsigned char pad00[0x0c];
    int           dx;               /* +0x0c  door / entry offset */
    int           dy;               /* +0x10 */
    unsigned char pad14[0xcc - 0x14];
    RiderNode*    riders;           /* +0xcc  live rider list of the class */
} RideObject;

extern void RemoveBlokeFromList(RideObject* owner, RiderNode* slot); /* 0x0044f470 */
extern void NewLongTermAction(Bloke* b, int action);                 /* 0x0044e760 */
extern void HeapFree_w(void* p);                                     /* 0x0049e4d0 */

/* Throw every staff member the class has parked on ONE map square out of the
 * building: unlink the rider node, and either drop the bloke at the class's
 * door offset with a fresh long-term action, or -- if his stage byte has
 * already passed 0x64, i.e. he is in the "being fired" half of the hut's
 * state machine -- leave him where he is with job slot 0x64.  `shed` is the
 * caller's identity: the MECHANICS HUT passes 0, the POTTING SHED 1, and it
 * only picks the long-term action (0x11 = the mechanic's repair round,
 * 0x10 = the gardener's).
 *
 * NOTE the two arms BOTH call RemoveBlokeFromList: the original emits the
 * call twice with ONE shared argument push pair, which is what a source with
 * the call written in each arm produces (a pending cdecl `add esp` cannot
 * cross the join, and here the two arms clean 0xc and 8 bytes respectively).
 *
 * LEVER (worth 11 of 64): `tx`/`ty` must be an AGGREGATE.  As two plain
 * `int` locals VC6 homes the spilled `ty` in the DEAD `cls` argument slot and
 * the frame shrinks to `push ecx`; as one `Pos` (or an `int[2]`) it takes a
 * fresh high slot and the frame is the original's `sub esp,8`.  Declaration
 * order of the Pos against the two pointers is inert. */

// FUNCTION: LEGOLAND 0x0043d7c0
void MechanicsHut_EvictRiders(RideObject* cls, BPosW key, int shed)
{
    RiderNode* r;
    RiderNode* next;
    Pos        t;

    t.x = cls->dx + key.b.x;
    t.y = cls->dy + key.b.y;
    r = cls->riders;
    while (r) {
        next = r->next;
        if (r->ride_id == key.w) {
            Bloke* b = r->bloke;
            if (b->action < 100) {
                RemoveBlokeFromList(cls, r);
                HeapFree_w(r);
                b->flags62 &= (unsigned short)~0x28;
                b->world.x = t.x << 8;
                b->world.y = t.y << 8;
                if (shed)
                    NewLongTermAction(b, 0x10);
                else
                    NewLongTermAction(b, 0x11);
            } else {
                RemoveBlokeFromList(cls, r);
                b->seat = 100;
            }
        }
        r = next;
    }
}


/* =========================================================================
 * 0x004333e0 -- JcBoat_Advance: run one tick of a jungle-cruise boat's
 * DOCKING sequence.
 *
 * JcBoat_Step puts a boat into state 0x10 with `leg` = 3 the moment it
 * reaches the station's route-end square (anim2.c), and from then on
 * JungleCruise_AdvanceBoats sends it here once every 0x50 frames.  The four
 * ticks that follow are the whole arrival:
 *
 *   leg 3   animate the crossing, then FREEZE the second half of the buffer
 *           -- wob[64..79] all take wob[64]'s value, so the boat stops dead
 *           half way across the last square;
 *   leg 2   animate, freeze the FIRST half instead (wob[0..63] = wob[64]),
 *           so the boat is already parked when the buffer starts playing,
 *           and aim one square further south;
 *   leg 1   animate, aim south again, then freeze the last SEVEN steps on
 *           wob[72] -- the little settle as it ties up;
 *   leg 0   unlink and FREE the boat, and hand the caller the NEXT one.
 *
 * `from`/`to` are always 1 and 4 here (in by the north side, out by the
 * south), which is the straight run down out of the station.
 * ========================================================================= */

/* The jungle-cruise boat record (junglecruise.c / anim2.c, same offsets). */
typedef struct JcWobble { int x; int y; } JcWobble;

typedef struct JcBoat {
    BPosW           key;            /* +0x00  the station that launched it */
    unsigned char   pad02[2];
    int             cx;             /* +0x04  the map square it is on */
    int             cy;             /* +0x08 */
    int             nx;             /* +0x0c  the map square it is heading for */
    int             ny;             /* +0x10 */
    unsigned char   pad14[0x1c - 0x14];
    JcWobble        wob[0x50];      /* +0x1c   80 precomputed sub-steps */
    int             frame[0x50];    /* +0x29c  80 precomputed sprite codes */
    int             f3dc;           /* +0x3dc the side it entered this square by */
    int             state;          /* +0x3e0 */
    int             leg;            /* +0x3e4 squares left in this leg */
    unsigned char   pad3e8[0x3f4 - 0x3e8];
    struct JcBoat*  next;           /* +0x3f4 */
} JcBoat;                           /* 0x3f8 */

extern void JcBoat_Unlink(JcBoat* b);                        /* 0x00432cb0 */
extern void JcBoat_Animate(JcBoat* b, int from, int to);     /* 0x00433840 */

/* LEVER (worth 6 of 69): the two ASCENDING fills are per-FIELD assignments,
 * the DESCENDING one is a whole-struct assignment.  All three copy the same
 * 8-byte JcWobble, but the induction variable is anchored differently: going
 * up, the original's pointer starts at `&wob[i].y` and addresses `[eax-4]` /
 * `[eax]`, which only the two scalar stores produce; the struct assignment
 * (and a pointer walk, and a y-then-x scalar pair) all anchor on `.x` and
 * cost 3 instructions' worth of operands.  Going DOWN, the struct assignment
 * is already the original's `[eax]` / `[eax+4]` and the scalar pair is wrong.
 * So the anchor follows the walk direction, and the spelling has to follow
 * the anchor -- measure both per loop. */

// FUNCTION: LEGOLAND 0x004333e0
JcBoat* JcBoat_Advance(JcBoat* b)
{
    JcBoat* next;
    int     i;

    if (b->leg == 0) {
        next = b->next;
        JcBoat_Unlink(b);
        return next;
    }
    JcBoat_Animate(b, 1, 4);
    b->f3dc = 1;
    if (b->leg == 3) {
        for (i = 64; i < 80; i++) {
            b->wob[i].x = b->wob[64].x;
            b->wob[i].y = b->wob[64].y;
        }
    } else if (b->leg == 2) {
        for (i = 0; i < 64; i++) {
            b->wob[i].x = b->wob[64].x;
            b->wob[i].y = b->wob[64].y;
        }
    }
    if (b->leg != 3)
        b->ny = b->cy + 5;
    if (b->leg == 1) {
        for (i = 79; i > 72; i--)
            b->wob[i] = b->wob[72];
    }
    b->leg--;
    return b;
}


/* =========================================================================
 * 0x0044e890 -- RandomFavouriteFood, and 0x0044e790 -- RandomFavouriteRide.
 *
 * rides.c gives every new visitor three favourite rides and one favourite
 * food; these are the two pickers, and they are ONE piece of source compiled
 * twice -- identical index for index apart from the class test in the middle.
 *
 * Both scan the LLIDB linearly from a random start for an element that is
 * (a) an ODF that is loaded on THIS level (type_flags 0x10 | 0x4) and
 * (b) of the right class kind: `type == 5` is the food/shop bucket, and
 * "not 0 and not 5" is a ride.  The scan wraps and gives up when it comes
 * back to where it started, which is the only path that returns 0.
 *
 * TWO ORIGINAL BUGS, both reproduced:
 *   1. The "tries" counter (rand() & 0x1f) is meant to pick the Nth match,
 *      but the accept arm never advances the cursor -- so the loop just
 *      re-fetches the SAME element until the counter runs out and the
 *      function always returns the FIRST match from the random start.
 *   2. When rand() & 0x1f happens to be 0 (one time in 32) the loop never
 *      runs and the function returns the UNINITIALISED local `e`, i.e. junk
 *      off the stack, which the caller stores as a favourite.  That is the
 *      `mov eax,[esp+0x10]` third epilogue.
 * ========================================================================= */

/* An object class descriptor; only the class kind at +0x20 matters here
 * (legoland.h's ObjClass). */
typedef struct FavObjDef {
    unsigned char pad00[0x20];
    short         type;             /* +0x20  5 = food/shop, 0 = not a class */
} FavObjDef;

/* One LLIDB element (legoland.h's LLElem, 0x14 bytes). */
typedef struct FavElem {
    char*         name;             /* +0x00 */
    char*         image;            /* +0x04 */
    unsigned int  type_flags;       /* +0x08  0x10 = ODF, 4 = on this level */
    FavObjDef*    data;             /* +0x0c  the parsed class record */
    unsigned int  refcount;         /* +0x10 */
} FavElem;

extern int  LLIDB_GetCount(void);                    /* 0x0047b2d0 */
#ifndef LEGOLAND_PORTABLE
extern void LLIDB_GetElement(int i, FavElem** out);  /* 0x0047b2e0 */
#else
extern int LLIDB_GetElement(int i, FavElem** out);  /* 0x0047b2e0 */
#endif
extern int  rand(void);                              /* 0x0049e4b2 (CRT) */

/* LEVER (worth 19 of 70): the cursor-advance block is written TWICE, once
 * per failing test.  Spelled as one `&&` guard VC6 emits a single shared
 * miss block reached by `jne` and the whole function is 7 instructions
 * short; two guards each ending in `continue` give the original's two inline
 * copies -- and the extra references to `n` and `start` that the second copy
 * creates are also what puts them in edi/ebx rather than ebx/ebp.  (The
 * `else if` nest is byte-identical to the `continue` form.)
 * `tries` must be assigned BEFORE `start`: the other order costs the
 * `mov ebx,esi` its position, index 11. */

// FUNCTION: LEGOLAND 0x0044e890
void* RandomFavouriteFood(void)
{
    FavElem*     e;
    int          n;
    int          i;
    int          start;
    unsigned int tries;

    n = LLIDB_GetCount();
    i = rand() % n;
    tries = (unsigned int)rand() & 0x1f;
#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
    if (tries == 0)           /* QUIRKS.md Q8: a zero try count returned the uninitialised `e` */
        tries = 1;
#endif
    start = i;
    while (tries--) {
        for (;;) {
            LLIDB_GetElement(i, &e);
            if ((e->type_flags & 0x14) != 0x14) {
                if (++i >= n)
                    i = 0;
                if (i == start)
                    return 0;
                continue;
            }
            if (e->data->type != 5) {
                if (++i >= n)
                    i = 0;
                if (i == start)
                    return 0;
                continue;
            }
            break;
        }
#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
        if (++i >= n)             /* QUIRKS.md Q8: step past the accepted match so the next try finds the NEXT one */
            i = 0;
        start = i;                /* and restart the wrap sentinel there: a full circle re-finds this match instead of returning 0 */
#endif
    }
    return e;
}


/* The RIDE picker: the same source with one different test in the middle --
 * "anything that is a class and is not the food/shop bucket".  The two
 * compares share one 16-bit load of the class kind (`mov cx,[ecx+0x20]`),
 * which the `||` gives for free; the food picker's single compare reads the
 * word straight out of memory. */

/* LEVER (worth the last 2 of 73): the class record must be named in a
 * POINTER local.  With `e->data->type` spelled out at both compares the body
 * is byte-exact except that VC6 takes edx, not ecx, for the `tries--`
 * temporary at the loop back-edge (register-blind 0); `d = e->data;` first
 * advances the scratch rotation by one and ecx falls out.  A `short` or
 * `unsigned short` local for the VALUE does not do it (still 2), an `int`
 * one is worse (5), and the food twin -- one compare, so no CSE -- needs no
 * local at all.  A volatile-qualified load of the same pointer is
 * byte-identical to the plain local, i.e. the free-volatile trick and the
 * named pointer are the same lever here. */

// FUNCTION: LEGOLAND 0x0044e790
void* RandomFavouriteRide(void)
{
    FavElem*     e;
    int          n;
    int          i;
    int          start;
    unsigned int tries;
    FavObjDef*   d;

    n = LLIDB_GetCount();
    i = rand() % n;
    tries = (unsigned int)rand() & 0x1f;
#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
    if (tries == 0)           /* QUIRKS.md Q8: a zero try count returned the uninitialised `e` */
        tries = 1;
#endif
    start = i;
    while (tries--) {
        for (;;) {
            LLIDB_GetElement(i, &e);
            if ((e->type_flags & 0x14) != 0x14) {
                if (++i >= n)
                    i = 0;
                if (i == start)
                    return 0;
                continue;
            }
            d = e->data;
            if (d->type == 0 || d->type == 5) {
                if (++i >= n)
                    i = 0;
                if (i == start)
                    return 0;
                continue;
            }
            break;
        }
#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)
        if (++i >= n)             /* QUIRKS.md Q8: step past the accepted match so the next try finds the NEXT one */
            i = 0;
        start = i;                /* and restart the wrap sentinel there: a full circle re-finds this match instead of returning 0 */
#endif
    }
    return e;
}


/* =========================================================================
 * 0x004048b0 -- Copters_SetFull: the HELICOPTERS machine starts its run.
 *
 * Reached from two places in the ride's own update (0x00404b68 when the
 * timer runs out with at least one rider aboard, and 0x00404d5f the moment
 * the seated count reaches the class's capacity at ObjDef+0x2e), both of
 * which have just cleared the 0x4000 "still filling" bit themselves.
 *
 * What it does: mark every OCCUPIED copter (of the FIVE the ride flies -- the
 * sixth seat record is never touched here) as flying, move the seated count
 * over to the "on the ride" count, put the machine into mode 2 with the
 * running bit up, reset all five copters' stage byte to 3 and their shared
 * animation frame to 0, and start the ride's two sounds: the one-shot at
 * FX slot 0 and the LOOP at slot 1, which is immediately paused and given a
 * 0xb54 ms completion callback (0x004048a0) that resumes it -- i.e. the loop
 * is delayed by exactly the length of the start-up sound.
 *
 * The sound source is the record's own map square (kind 2), and its +0x04 is
 * left UNINITIALISED, the same original habit money.c records.
 * ========================================================================= */

/* One copter seat (mechrides.c / joust2.c's CopterSeat, 0x20 bytes).  NOTE
 * the +0x00 flags are a DWORD here where joust2.c's drawing view reads only
 * the low byte -- this body ORs the whole dword. */
typedef struct CopterSeat {
    unsigned int  flags;            /* +0x00  bit 0 = this copter is flying */
    signed char   frame;            /* +0x04  shared animation frame */
    unsigned char pad05[0x18 - 5];
    void*         rider;            /* +0x18  the RiderNode in this copter */
    unsigned char pad1c;            /* +0x1c */
    unsigned char stage;            /* +0x1d  per-copter flight stage */
    unsigned char pad1e[2];
} CopterSeat;                       /* 0x20 */

/* The per-placement HELICOPTERS record (mechrides.c's CoptersRec, 0xd8). */
typedef struct CoptersRec {
    unsigned char      x;           /* +0x00  the map square this copy sits on */
    unsigned char      y;           /* +0x01 */
    unsigned char      seated;      /* +0x02  how many riders are seated */
    unsigned char      riders;      /* +0x03  how many are on the ride at all */
    struct CoptersRec* next;        /* +0x04 */
    unsigned int       flags;       /* +0x08  1 = running, 0x4000 = filling */
    int                mode;        /* +0x0c  2 = flying */
    unsigned char      joined;      /* +0x10 */
    unsigned char      pad11[3];
    int                timer;       /* +0x14 */
    CopterSeat         seat[6];     /* +0x18 .. +0xd8 */
} CoptersRec;                       /* 0xd8 */

/* Where a sound comes from (money.c's SoundSource; kind 2 = "a map square",
 * +0x04 never initialised). */
typedef struct RideSoundSource {
    int kind;                       /* +0x00 */
    int f04;                        /* +0x04 */
    int x;                          /* +0x08 */
    int y;                          /* +0x0c */
} RideSoundSource;

/* One entry of an FX table (audiomisc.c's FXEntry, 12 bytes). */
typedef struct FXEntry {
    char* name;                     /* +0x00 */
    int   pad4;                     /* +0x04 */
    void* sample;                   /* +0x08  filled in by Load_FXList */
} FXEntry;

extern FXEntry g_copters_fx[4];                              /* 0x004b4140 */

extern void* PlayInstanceOfSample(void* sample, int a, int b,
                                  RideSoundSource* src);     /* 0x00496d20 */
extern int   PauseSingleSample(void* s);                     /* 0x00492800 */
extern void  AddSFX_Callback(void* sfx, int delay, void* cb); /* 0x00496db0 */
/* Resumes the paused loop when the start-up sound has finished. */
extern int   Copters_ResumeSFX(void* sfx);                   /* 0x004048a0 */

/* ORIGINAL ODDITY, reproduced (worth 7 of 75, the whole residual): all THREE
 * five-line runs list copter 1 BEFORE copter 0 and then 2, 3, 4.  It is not
 * an adjacent-store reversal -- writing 0,1,2,3,4 emits 0,1,2,3,4 -- the
 * source really enumerates the seats in that order, three times over.  (The
 * ride's draw pass has its own fixed order too: joust2.c records 0,2,3,4,1.) */

// FUNCTION: LEGOLAND 0x004048b0
void Copters_SetFull(CoptersRec* rec)
{
    RideSoundSource src;
    void*           s;

    if (rec->seat[1].rider)
        rec->seat[1].flags |= 1;
    if (rec->seat[0].rider)
        rec->seat[0].flags |= 1;
    if (rec->seat[2].rider)
        rec->seat[2].flags |= 1;
    if (rec->seat[3].rider)
        rec->seat[3].flags |= 1;
    if (rec->seat[4].rider)
        rec->seat[4].flags |= 1;
    rec->riders = rec->seated;
    rec->seated = 0;
    rec->flags = (rec->flags & ~0x4000) | 1;
    rec->mode = 2;
    rec->seat[1].stage = 3;
    rec->seat[0].stage = 3;
    rec->seat[2].stage = 3;
    rec->seat[3].stage = 3;
    rec->seat[4].stage = 3;
    rec->seat[1].frame = 0;
    rec->seat[0].frame = 0;
    rec->seat[2].frame = 0;
    rec->seat[3].frame = 0;
    rec->seat[4].frame = 0;
    src.x = rec->x;
    src.y = rec->y;
    src.kind = 2;
    PlayInstanceOfSample(g_copters_fx[0].sample, 0, 1, &src);
    s = PlayInstanceOfSample(g_copters_fx[1].sample, 1, 1, &src);
    PauseSingleSample(s);
    AddSFX_Callback(s, 0xb54, Copters_ResumeSFX);
}


/* =========================================================================
 * 0x0042cf70 -- EarthSlide_LaunchCar: the front of the EARTH SLIDE queue
 * boards, and everybody behind shuffles up one place.
 *
 * EarthSlide_Tick calls this when the queue's leader has reached the head
 * spot (ridecb1.c case 2).  It clears that bloke's "queued" bit (0x40), steps
 * his state machine on so the ride's own arms take him, pops him off the
 * 8-byte queue chain, and then re-walks WHAT IS LEFT of the chain sending
 * each remaining queuer to the NEXT spot up.
 *
 * The four queue spots are a const table of {dx, dy} pairs at 0x004b65c0 --
 * (0,4) (0,3) (0,2) (0,1), i.e. four cells due south of the ride's base
 * square, closest last.  The shuffle walk is NOT bounded by 4: it indexes
 * the table by the queuer's position in the list, so a fifth queuer would
 * read the string constants that follow the table.  The queue length byte
 * (+0x18) is what keeps that from happening in practice; the bug is latent
 * and is reproduced as written.
 * ========================================================================= */

/* One 8-byte queue node (ridecb1.c: "a chain of 8-byte nodes {next, rider}"). */
typedef struct SlideQueueNode {
    struct SlideQueueNode* next;    /* +0x00 */
    RiderNode*             rider;   /* +0x04 */
} SlideQueueNode;

/* The per-square EARTH SLIDE record (ridecb1.c's SlideRec); +0x20 is the
 * queue TAIL, which ridecb1.c's view stops just short of. */
typedef struct SlideRec {
    BPosW            square;        /* +0x00  packed {x,y} */
    unsigned char    pad02[0x18 - 2];
    signed char      queued;        /* +0x18  how many are in the queue */
    unsigned char    pad19[3];
    SlideQueueNode*  queue;         /* +0x1c  head of the queue chain */
    SlideQueueNode*  tail;          /* +0x20 */
} SlideRec;

/* The EARTH SLIDE class object; +0x0c/+0x10 is its base map square. */
typedef struct SlideDef {
    unsigned char pad00[0x0c];
    int           base_x;           /* +0x0c */
    int           base_y;           /* +0x10 */
} SlideDef;

extern SlideDef* g_slide_item;                       /* 0x006160d0 */
/* The four queue standing spots, closest to the ride LAST. */
extern const Pos kSlideQueueSpots[4];                /* 0x004b65c0 */

/* Unlink the head queue node (and fix the tail pointer and the count). */
extern void EarthSlide_PopQueue(SlideRec* rec);      /* 0x0042d040 */
extern int  CalcMoveLine(Pos from, Pos to, void* path); /* 0x00480740 */
extern int  NewDirForAction(Bloke* b, unsigned char dir); /* 0x004833d0 */

/* LEVERS.  Two, both about the loop's SECOND induction variable -- the
 * cursor into the queue-spot table:
 *
 *  1. It must be a POINTER walked with `++`, not `kSlideQueueSpots[i]` with
 *     an index.  The index form loses one instruction (74 of 75, 199 of 201
 *     bytes): VC6 folds the strength-reduced address into the two loads and
 *     never materialises the cursor, so the `mov eax,ecx` that preserves the
 *     computed target.y disappears with it.
 *  2. The pointer must be an `int*` parked on the entry's Y half, read as
 *     `spot[-1]` / `spot[0]` and stepped by 2.  VC6 anchors an ASCENDING
 *     walk's induction variable on the LAST field it touches -- the same
 *     +4 bias JcBoat_Advance's two ascending fills show from the store side
 *     -- and nothing done to a `const Pos*` cursor reaches it: `sp->x`/
 *     `sp->y`, `sp[0].x`, a whole-struct copy `Pos s = *sp;`, a named local
 *     for either field, `(sp++)->y`, the `for (...; q = q->next, sp++)`
 *     header form, `sp++` before the list step, a `do/while`, a flat
 *     `int[8]` view walked as `ip[0]`/`ip[1]`, and `kSlideQueueSpots + 1`
 *     with `sp[-1].x`/`sp[-1].y` are all either 2 mismatches (the two
 *     displacements plus the table constant) or worse.  Only the biased
 *     int cursor is exact -- and it is the same walk, byte for byte.
 *
 * Also measured and inert: the operand order of both sums (canonicalised),
 * `* 256` for `<< 8`, separate `<<= 8` statements, naming the path or the
 * world Pos in a local, `&b->path[0]`, and a volatile store on either target
 * field. Writing the Y assignment FIRST costs 13. */

// FUNCTION: LEGOLAND 0x0042cf70
void EarthSlide_LaunchCar(SlideRec* rec)
{
    SlideQueueNode* q;
    Bloke*          b;
    Pos             t;
    const int*      spot;
    unsigned char   a;

    q = rec->queue;
    if (!q)
        return;
    b = q->rider->bloke;
    b->flags62 &= (unsigned short)~0x40;
    b->action++;
    EarthSlide_PopQueue(rec);
    q = rec->queue;
    if (!q)
        return;
    t.x = g_slide_item->base_x + rec->square.b.x;
    t.y = g_slide_item->base_y + rec->square.b.y;
    spot = &kSlideQueueSpots[0].y;
    while (q) {
        b = q->rider->bloke;
        b->target.x = (t.x + spot[-1]) << 8;
        b->target.y = (t.y + spot[0]) << 8;
        a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
        b->state = 7;
        b->new_dir = a;
        NewDirForAction(b, (unsigned char)((a >> 5) + 3));
        q = q->next;
        spot += 2;
    }
}
