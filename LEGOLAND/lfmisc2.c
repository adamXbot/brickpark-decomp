/* =========================================================================
 * LEGOLAND -- lfmisc2.c
 *
 * A second bag of small helpers, sibling to lfmisc.c: the log-flume boarding
 * queue's front pop/step, the fourth corner of the doubled flume tile, the
 * two POWER STATION +0x98 add callbacks and the driving school's traffic
 * light gate.  VC6 SP3 /O2 /Gy /Gd.  Struct layouts are reconstructed from
 * the offsets each body touches; the names are ours.
 *
 * The functions are unrelated to each other except by size; each carries its
 * own note.  What they share is that every one of them is reached through a
 * pointer stored in an ObjDef slot or called from an exact neighbour, so the
 * argument shapes below are pinned by the CALLER's disassembly as well as by
 * their own.
 * ========================================================================= */

/* ---- shared types -------------------------------------------------------
 * Same offsets logflume4.c / lfentrance.c / lfmisc.c use.  Only the fields
 * these four bodies touch are named.
 */
typedef struct Pos { int x; int y; } Pos;

/* The person record a rider points at.  +0x0e is the walk state word (the
 * queue stepper writes 7 into it), +0x38 the index along the queue path,
 * +0x60 the action byte and +0x62 the flag word whose 0x40 bit means
 * "standing in a ride queue". */
typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;       /* +0x0e */
    unsigned char  pad10[0x50];
    unsigned char  action;      /* +0x60 */
    unsigned char  pad61[1];
    unsigned short flags;       /* +0x62 */
} Bloke;

typedef struct RiderNode {
    unsigned char pad00[8];
    Bloke*        bloke;        /* +0x08 */
} RiderNode;

/* The 8-byte boarding-queue node (lfentrance.c's LFQueueNode). */
typedef struct LFQueueNode {
    struct LFQueueNode* next;   /* +0x00 */
    RiderNode*          rider;  /* +0x04 */
} LFQueueNode;

/* The queue header.  logflume4.c only ever needs {path, head}; the pop below
 * proves the third slot, because it hands the whole header to the generic
 * {f00, head, tail} popper logflume2.c matched as LFAnim_PopHead. */
typedef struct LFQueue {
    void*        path;          /* +0x00 the walk path, first int = capacity */
    LFQueueNode* head;          /* +0x04 */
    LFQueueNode* tail;          /* +0x08 */
} LFQueue;

/* PlayInstanceOfSample's 16-byte source descriptor (audio3.c +0x0c..0x1b).
 * kind 2 = a map square; the `obj` slot is left UNINITIALISED by both power
 * station callbacks -- reproduced, the original never writes it. */
typedef struct SoundSource {
    int   kind;                 /* +0x00 */
    void* obj;                  /* +0x04 */
    int   x;                    /* +0x08 */
    int   y;                    /* +0x0c */
} SoundSource;

/* ---- callees ------------------------------------------------------------ */
/* 0x00411f00 is logflume2.c's LFAnim_PopHead: the generic {f00, head, tail}
 * unlink-the-head.  The queue header has the same three-word shape, so the
 * flume's boarding queue reuses it -- declared here with the queue type
 * because that is what the caller passes. */
extern void  LFQueue_PopHead(LFQueue* q);                       /* 0x00411f00 */
extern void  HeapFree_w(void* p);                               /* 0x0049e4d0 */
/* 0x00411fa0 (not exported): walk one queued bloke one pace along the queue
 * path -- position = path->pts[bloke->f38] + (tx, ty) in 24.8, then
 * CalcMoveLine / NewDirForAction and f38++.  It reads tx and ty as two
 * separate int arguments. */
extern void  LFQueue_StepRider(LFQueue* q, int tx, int ty, Bloke* b);
                                                                /* 0x00411fa0 */
extern void  GetTileDimensions(int* width, int* height);        /* 0x00460540 */
#ifndef LEGOLAND_PORTABLE
extern void  AddBasicObject(void* obj, Pos* pos);               /* 0x0045efe0 */
#else
extern void AddBasicObject(void* ll_obj, void* ll_pos, void* ll_ctx);               /* 0x0045efe0 */
#define AddBasicObject(_a1, _a2) AddBasicObject((_a1), (_a2), 0)
#endif
extern int   PlayInstanceOfSample(void* sample, int a, int b,
                                  SoundSource* src);            /* 0x00496d20 */

/* screencb7.c's g_power_station_fx (0x004b8750) is a two-entry FX list of
 * 12-byte records; the resolved sample pointer sits at +0x08 of each, so
 * [0] is the crystal station's and [3] the small station's -- the same
 * [k*3] spelling catapult.c and castleobj.c use. */
extern void* g_power_station_sample[];                          /* 0x004b8758 */

/* coaster.c's junction lights: 1 while that axis has green. */
extern int   g_light_ns;                                        /* 0x004cbeb0 */
extern int   g_light_ew;                                        /* 0x004cbeb4 */

/* =========================================================================
 * 0x00412060 -- take the front rider off the boarding queue
 *
 * LFRun_Tick (logflume4.c 0x0040be00) calls this once it has decided the
 * front rider may board: the bloke's action byte is bumped on to its next
 * state, the "standing in a queue" flag (+0x62 bit 0x40) comes down, the
 * rider is handed back through *out, the node is unlinked by the shared
 * three-word head-popper and then freed.
 *
 * Two things the original does that a tidier version would not:
 *  - an EMPTY queue writes nothing to *out, so the caller's `rider = 0`
 *    initialiser is load-bearing (LFRun_Tick has it);
 *  - `n->rider` is read TWICE.  The `flags &= ~0x40` store to the bloke may
 *    alias the node, so VC6 cannot keep the first load alive across it -- the
 *    reload is free and comes out of the natural spelling.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00412060
void LFQueue_PopFront(LFQueue* q, RiderNode** out)
{
    LFQueueNode* n = q->head;

    if (n) {
        Bloke* b = n->rider->bloke;
        b->action++;
        b->flags &= ~0x40;
        *out = n->rider;
        LFQueue_PopHead(q);
        HeapFree_w(n);
    }
}

/* =========================================================================
 * 0x00411250 -- the bottom-right quadrant centre of the doubled flume tile
 *
 * The fourth of lfmisc.c's four corner helpers (top-left 0x00411290,
 * top-right 0x00411220, bottom-left 0x004112c0).  A flume tile is drawn at
 * double tile scale, so the boat entry/exit points are the centres of the
 * four quadrants of the doubled tile: (tw, th), (3tw, th), (tw, 3th) and --
 * here -- (3tw, 3th), where tw = 2*width >> 1.
 *
 * The doubled sizes are written back into the two GetTileDimensions
 * out-parameters and never read again; the addresses escaped, so VC6 keeps
 * the in-place shifts.  `w <<= 1` and `w = w * 2` are NOT the same object
 * (the multiply lowers to a two-operand lea) -- the in-place shift is what
 * the original has, and both halves keep their signed `>> 1`.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00411250
Pos LFQuadBottomRight(void)
{
    int width, height;
    Pos out;
    GetTileDimensions(&width, &height);
    width <<= 1;
    height <<= 1;
    out.x = (width >> 1) + width;
    out.y = (height >> 1) + height;
    return out;
}

/* =========================================================================
 * 0x00452ad0 / 0x00452b20 -- the two POWER STATION +0x98 add callbacks
 *
 * screen.c's SetCustomCallbacks installs SmallPowerStation_Add on "small
 * power station" and CrystalPowerStation_Add on "crystal power station";
 * both share PowerStation_AC (screencb7.c 0x00452ab0) and
 * PowerStation_InitSound.  The two bodies are the SAME SOURCE compiled twice
 * against a different FX entry: place the object the standard way, then play
 * the station's hum from the square it was dropped on.
 *
 * The 16-byte SoundSource is a local aggregate initialiser -- `sub esp,0x10`
 * with stores and no matching `add esp` -- and its `obj` word is never
 * written, so the sound source carries whatever was on the stack there.  The
 * two cdecl calls share ONE merged `add esp,0x18`: nothing branches between
 * them, so the pending adjustment of AddBasicObject's two arguments survives
 * to the end.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00452ad0
void SmallPowerStation_Add(void* obj, Pos* pos)
{
    SoundSource src;

    AddBasicObject(obj, pos);
    src.kind = 2;
    src.x = pos->x;
    src.y = pos->y;
    PlayInstanceOfSample(g_power_station_sample[3], 1, 1, &src);
}

// FUNCTION: LEGOLAND 0x00452b20
void CrystalPowerStation_Add(void* obj, Pos* pos)
{
    SoundSource src;

    AddBasicObject(obj, pos);
    src.kind = 2;
    src.x = pos->x;
    src.y = pos->y;
    PlayInstanceOfSample(g_power_station_sample[0], 1, 1, &src);
}

/* =========================================================================
 * 0x00402340 -- may a school car travelling in `dir` cross the junction?
 *
 * goldrush3.c's SchoolCarAtTarget asks this when the block four squares
 * ahead is a traffic-light block.  `dir` is a 0..7 compass heading out of
 * g_dir16to8.  coaster.c drives the pair of lights through a four-phase
 * cycle, so "both zero" is the all-red gap between phases and stops
 * everything -- including a car whose heading is one of the four DIAGONALS
 * (0, 2, 4, 6), which no green ever admits either: the two greens only
 * name 3/7 and 1/5.  A diagonal therefore passes only while a light is
 * green, whichever one it is.
 *
 * Three separate `xor eax,eax / ret` blocks, so three textual `return 0`s --
 * a single flag joined at the end would collapse them into one zero
 * register.  g_light_ns is read once and reused by the all-red test because
 * nothing between the two uses can store to it; spelling both uses as the
 * global directly is what lets VC6 keep the one load.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00402340
int TrafficLightAllows(int dir)
{
    if (g_light_ns && (dir == 7 || dir == 3))
        return 0;
    if (g_light_ew && (dir == 1 || dir == 5))
        return 0;
    if (!g_light_ns && !g_light_ew)
        return 0;
    return 1;
}

/* =========================================================================
 * 0x004120a0 -- step EVERY rider in the boarding queue one pace
 *
 * Despite what the caller's shape suggests this is not just the front rider:
 * LFRun_Tick calls it once a frame and it walks the whole node list, moving
 * each bloke whose walk state (+0x0e) is 0 toward the station door at
 * (tx, ty).  A bloke already mid-animation (state != 0) is skipped for that
 * frame -- LFQueue_StepRider is what sets the state to 7, so the queue
 * shuffles up one bloke at a time rather than all at once.
 *
 * `tx`/`ty` arrive as two separate int arguments even though logflume4.c
 * declares the extern as a by-value `Pos` (ABI-identical, and the by-value
 * spelling is what schedules the caller's two sums correctly).  Both are
 * loaded ONCE, hoisted out of the loop, into the two registers whose pushes
 * sink past the null guard: an empty queue costs `push esi / push edi` only.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004120a0
void LFQueue_StepFront(LFQueue* q, int tx, int ty)
{
    LFQueueNode* n = q->head;

    while (n) {
        Bloke* b = n->rider->bloke;
        if (b->state == 0)
            LFQueue_StepRider(q, tx, ty, b);
        n = n->next;
    }
}
