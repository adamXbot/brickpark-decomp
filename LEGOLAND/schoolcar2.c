/* LEGOLAND -- the DRIVING SCHOOL CAR MANOEUVRE SYSTEM.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere).  schoolcar.c owns the car record and the queue pop,
 * goldrush.c owns the per-frame driver StepSchoolCar; this file is the four
 * routines in between -- the two that CHOOSE the next manoeuvre and two of
 * the four that EXECUTE one.
 *
 * =========================================================================
 * THE ROAD GRAPH
 * =========================================================================
 * A driving school's roads are 4x4-map-square BLOCKS on a list at 0x004cbeac
 * (ridecb5.c/ridecb6.c own the record).  Each block carries
 *
 *      +0x08 u16 school   the owning school's packed map square
 *      +0x0c int x        the block ORIGIN, a multiple of 4
 *      +0x10 int y
 *      +0x14 u8  kind     low nibble; 6 = the school's ENTRANCE block
 *      +0x18 RoadRec* prev   the path-walk mark
 *
 * `prev` is written by DrivingSchool_WalkStep (0x004051a0): the walk is
 * seeded at the kind-6 entrance block and each block it reaches records the
 * block it was reached FROM, so following +0x18 leads back to the entrance.
 * That single pointer is the whole route graph the cars drive.
 *
 * Two helpers step a position by a whole block and by half a block:
 *
 *      0x004808d0  Pos_Step4(from, to, dir)   +/- 4 map squares
 *      0x00480840  Pos_Step2(from, to, dir)   +/- 2 map squares
 *
 * Both switch on `dir` with 1 = north, 3 = east, 5 = south, 7 = west; the
 * four EVEN headings (the diagonals of the eight-way ring 0=NW, 1=N, 2=NE,
 * 3=E, 4=SE, 5=S, 6=SW, 7=W) fall into the switch's default and leave the
 * destination untouched.  So (dir - 2) & 7 is a left turn and (dir + 2) & 7
 * a right turn, and a car's heading byte (+0xba) is always odd.
 *
 * =========================================================================
 * THE MANOEUVRE CODES
 * =========================================================================
 * StepSchoolCar asks for a code every time the car crosses its current
 * waypoint, then runs the matching routine:
 *
 *      0  nothing to do -- the car stops (speed <- 0)
 *      1  turn LEFT           SchoolCarManoeuvreA  0x00401320
 *      2  go STRAIGHT ON      SchoolCarManoeuvreB  0x004015e0
 *      3  turn RIGHT          SchoolCarManoeuvreC  0x00401080
 *      4  pull off the road   SchoolCarManoeuvreD  0x00401660
 *      5  arrive at the school: A then B, back to back
 *
 * The naming is read off the code, not guessed: A subtracts 2 from the
 * heading and C adds 2, and Pos_Step2/Pos_Step4's switch makes +2 a right
 * turn; B (0x004015e0) steps the running square twice by half a block and
 * never touches the heading, so it is the straight-through move; D
 * (0x00401660) is A and C's shape with far tighter offsets (forward 0x0d,
 * 0x21 ... against sideways 0x28, 0x54 ...) and is the ONLY one that ends
 * with `c->on_road = 1` -- StepSchoolCar clears that byte every time it asks
 * for a new manoeuvre, and code 4 is what the free-driving chooser returns
 * when there is no usable road ahead at all.
 *
 * A manoeuvre routine does not move the car.  It APPENDS the run of
 * waypoints that traces its path onto the car's queue (wp[16] at +0x30,
 * count at +0xbb), advances the car's heading (+0xba) and leaves +0x20 --
 * the square the NEXT manoeuvre starts from -- at the far end of the path.
 * schoolcar.c's SchoolCarIdleStep (0x00401cd0) pops the queue's front.
 *
 * =========================================================================
 * HOW A TURN IS TRACED  (SchoolCarManoeuvreA / SchoolCarManoeuvreC)
 * =========================================================================
 * Four curve points plus, on the wide variant, a fifth straight one.  Each
 * point is a fixed (forward, sideways) offset in 1/256ths of a map square,
 * rotated into the car's heading by 0x00401000 (east is the identity) and
 * added to the running square `p`:
 *
 *      wp[n].x = (p.x << 16) + (offset.x << 8)
 *
 * The two turn routines are the SAME CODE with the sideways component
 * negated and the heading delta flipped; A subtracts 2 from the heading and
 * C adds 2.  Each has a TIGHT and a WIDE arm selected by the global at
 * 0x004c11c0 (coaster.c's theme switch), and the two routines select them
 * the OPPOSITE way round:
 *
 *                     tight (4 points)          wide (5 points)
 *      A (left)       g != 0                    g == 0
 *      C (right)      g == 0                    g != 0
 *
 * so exactly one of the two turns is the near-side turn at a time.  The
 * tight arm's sideways offsets are 0, 0x2c, 0x7a, 0xcc; the wide arm's are
 * exactly double -- 0, 0x58, 0xf4, 0x198 -- and the wide arm also steps the
 * running square half a block BEFORE laying the curve and twice again
 * afterwards, so its fifth waypoint sits a whole block beyond the corner.
 *
 * =========================================================================
 * HOW THE NEXT MANOEUVRE IS CHOSEN
 * =========================================================================
 * SchoolCarNextManoeuvre (0x00402150) is the FOLLOW-THE-ROUTE chooser: it
 * looks at the block ahead and at that block's `prev` pointer, and returns
 * the turn that keeps the car on the walked path.  It falls back to
 * SchoolCarNextManoeuvreHorn (0x00401f30) whenever the route runs out --
 * no block under the car's start square has a route mark, no block ahead,
 * a block ahead belonging to a different school, or a junction the route
 * does not resolve.
 *
 * SchoolCarNextManoeuvreHorn is the FREE-DRIVING chooser: it collects the
 * three exits of the block ahead (left / straight on / right, each of which
 * must exist, belong to this school and not be the entrance block), builds
 * a 3-bit mask 1|2|4 out of them and picks one at random.  It returns 4 --
 * the give-way manoeuvre -- when there is no usable block ahead at all.
 * StepSchoolCar picks this chooser while the car's horn timer (+0xc0) is
 * still running, which is what makes a hooted-at car wander off the route.
 * ========================================================================= */

/* ---- the car record (schoolcar.c owns it; these offsets are its) -------- */
typedef struct CarPos { int x; int y; } CarPos;

/* One queued waypoint: a world position in 16.16. */
typedef struct Waypoint { int x; int y; } Waypoint;

typedef struct SchoolCar {
    struct SchoolCar* next;      /* +0x00 */
    unsigned short    school;    /* +0x04  the school's packed map square */
    unsigned char     pad06[2];
    int               sx;        /* +0x08 */
    int               sy;        /* +0x0c */
    int               wx;        /* +0x10  world x, 16.16 */
    int               wy;        /* +0x14 */
    CarPos            cur;       /* +0x18  map square */
    CarPos            start;     /* +0x20  the square the manoeuvre starts on */
    int               vx;        /* +0x28 */
    int               vy;        /* +0x2c */
    Waypoint          wp[16];    /* +0x30  the waypoint queue */
    float             ux;        /* +0xb0 */
    float             uy;        /* +0xb4 */
    unsigned char     frame;     /* +0xb8 */
    unsigned char     b9;        /* +0xb9 */
    unsigned char     turn;      /* +0xba  8-way road heading, 0..7 */
    unsigned char     nwp;       /* +0xbb  live waypoints in wp[] */
    unsigned short    stall;     /* +0xbc */
    unsigned short    t_life;    /* +0xbe */
    unsigned short    t_horn;    /* +0xc0 */
    unsigned char     on_road;   /* +0xc2 */
    unsigned char     livery;    /* +0xc3 */
    unsigned char     manoeuvre; /* +0xc4 */
    unsigned char     c5;        /* +0xc5 */
    unsigned short    top_speed; /* +0xc6 */
    unsigned short    speed;     /* +0xc8 */
    unsigned char     padca[2];
    void*             bloke;     /* +0xcc */
} SchoolCar;                     /* 0xd0 */

/* Rotates the fixed offset (fwd, side) -- 1/256ths of a map square -- into
 * the heading `dir`, EAST (3) being the identity.  An 8-byte struct, so it
 * comes back in edx:eax.  The four even headings fall into the switch's
 * default, which returns the two uninitialised dwords the function's own
 * `sub esp, 8` just made: an original bug, harmless because a car's heading
 * is always odd. */
extern CarPos RotateByHeading(int fwd, int side, int dir);      /* 0x00401000 */

/* Steps a map square 2 / 4 squares in the heading `dir`; even headings
 * leave `to` untouched. */
extern void   Pos_Step2(CarPos* from, CarPos* to, int dir);     /* 0x00480840 */

/* coaster.c's theme switch: it also decides which way round the two turn
 * routines take their tight and wide arms, i.e. which side of the road the
 * driving school teaches. */
extern int g_4c11c0;                                            /* 0x004c11c0 */

/* =========================================================================
 * 0x00401080 -- SchoolCarManoeuvreC: append a RIGHT turn (code 3).
 *
 * 207/207 instructions and 669/669 BYTES; the residual is 3 instructions --
 * indices 117..119, the `dir` argument of the tight arm's FIRST
 * RotateByHeading call, where the original allocates ecx
 * (`xor ecx,ecx / mov cl,[esi+0xba] / push ecx`) and this build allocates
 * edx.  Nothing else in the body differs, and from index 120 the two
 * converge again because edx and eax are pinned by the struct return.
 *
 * IT IS THE SAME ONE-STEP SCRATCH-ROTATION PHASE as SchoolCarManoeuvreA's
 * 10 (below): in both functions the SECOND arm's first temp triple starts
 * one step earlier in VC6's eax->ecx->edx rotation than this build's.  The
 * first arm of each function is exact, so the difference is the phase the
 * else block STARTS at, not anything inside it.
 *
 * Eliminated (measured on both siblings together, ~30 builds): the final
 * `n++` really is factored out of both arms (`c->nwp = n + 1`, which is
 * what puts the original's `inc bl` in the shared tail -- with `n++` at the
 * end of each arm VC6 does NOT cross-jump it and emits a spare `inc ebx`);
 * declaration order of p/o/n; `p` copied field-by-field either way (inert,
 * so the escaped-aggregate question is settled and is NOT the cause);
 * `o` block-scoped per arm; a second `o2`; caching the global in a local;
 * `!= 0` on the test; `unsigned` n; six no-code statements (`n = n`,
 * `p = p`, `o = o`, `n += 0`, `if (n) ;`, `c->turn = c->turn`) inserted at
 * the head of the else arm AND at the head and tail of the if arm -- all
 * twelve placements are byte-identical, so the phase counter is not
 * advanced by a folded tuple here; three FREE `volatile` reads (the
 * `c->turn` load and the `p.x` load the original makes anyway) are
 * byte-identical too, which by the project's one-experiment test makes this
 * a global web rank rather than a local rotation.  A `goto` spelling of the
 * two arms costs 180; `unsigned char` for `n` or for the callees' `dir`
 * parameter costs ~195; storing y before x costs 116.
 *
 * Also eliminated, after the two choosers below closed: six conversion-tuple
 * probes (dropping the `(unsigned char)` cast on the heading store, and
 * `(int)`/`(long)` casts on the heading argument, on `p.x`, on `o.x` and on
 * the whole stored expression) are byte-identical, so the no-code-tuple
 * lever that moves a scheduler window in `BuildChannelTables` does not move
 * this phase; and folding the four waypoint statements into a
 * `static __inline LayWaypoint(c, n, &p, fwd, side)` is also byte-identical
 * (by value instead of by pointer costs 204).  One DIAGNOSTIC worth
 * recording: swapping the two arms and negating the test shows the phase is
 * POSITIONAL in this build -- whichever arm is laid first gets edx for its
 * first temp and so does the second, where the original's second arm gets
 * ecx.  For the original's layout the first arm would have to consume two
 * more temps (mod 3) than ours while emitting the same instructions, and no
 * construct measured here creates a temp that emits nothing.
 * ========================================================================= */
// WIP-FUNCTION: LEGOLAND 0x00401080  (98.6%: 207/207 insns, 669/669 bytes, 3 mismatches at indices 117-119 -- the else arm's first scratch temp is ecx in the original and edx here, one step of VC6's eax->ecx->edx rotation)
void SchoolCarManoeuvreC(SchoolCar* c)
{
    CarPos p;
    CarPos o;
    int    n;

    n = c->nwp;
    p = c->start;
    if (g_4c11c0) {
        Pos_Step2(&p, &p, c->turn);
        o = RotateByHeading(0x68, 0, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        o = RotateByHeading(0x10c, 0x58, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        o = RotateByHeading(0x1a8, 0xf4, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        o = RotateByHeading(0x200, 0x198, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        Pos_Step2(&p, &p, c->turn);
        c->turn = (unsigned char)((c->turn + 2) & 7);
        Pos_Step2(&p, &p, c->turn);
        c->wp[n].x = p.x << 16;
        c->wp[n].y = p.y << 16;
    } else {
        o = RotateByHeading(0x68, 0, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        o = RotateByHeading(0x10c, 0x2c, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        o = RotateByHeading(0x1a8, 0x7a, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        o = RotateByHeading(0x200, 0xcc, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        Pos_Step2(&p, &p, c->turn);
        c->turn = (unsigned char)((c->turn + 2) & 7);
    }
    c->start = p;
    c->nwp = (unsigned char)(n + 1);
}

/* =========================================================================
 * 0x00401320 -- SchoolCarManoeuvreA: append a LEFT turn (code 1).
 *
 * 207/207 instructions, 669/669 BYTES, 10 mismatches -- indices 94..100 and
 * 102..104, which are the WIDE arm's leading `Pos_Step2(&p, &p, c->turn)`
 * argument triple and the `dir` of the first RotateByHeading after it.  The
 * original allocates (dir, to, from) = (ecx, edx, eax); this build allocates
 * (edx, eax, ecx) -- the same triple, rotated one step.  From index 105 the
 * two converge.  It is SchoolCarManoeuvreC's 3-instruction residual seen
 * through a longer second arm: identical cause, identical eliminations (see
 * the note above SchoolCarManoeuvreC -- every experiment was run on both).
 *
 * The two bodies are the same code with the sideways offsets negated, the
 * heading delta flipped from +2 to -2, and THE TWO ARMS EXCHANGED, so the
 * tight arm is the `if` here and the `else` in C.  Every construct that was
 * corrected on one transferred verbatim to the other.
 * ========================================================================= */
// WIP-FUNCTION: LEGOLAND 0x00401320  (95.2%: 207/207 insns, 669/669 bytes, 10 mismatches at indices 94-100 and 102-104 -- the same one-step scratch-rotation phase at the else arm's entry as SchoolCarManoeuvreC, seen through a longer arm)
void SchoolCarManoeuvreA(SchoolCar* c)
{
    CarPos p;
    CarPos o;
    int    n;

    n = c->nwp;
    p = c->start;
    if (g_4c11c0) {
        o = RotateByHeading(0x68, 0, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        o = RotateByHeading(0x10c, -0x2c, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        o = RotateByHeading(0x1a8, -0x7a, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        o = RotateByHeading(0x200, -0xcc, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        Pos_Step2(&p, &p, c->turn);
        c->turn = (unsigned char)((c->turn - 2) & 7);
    } else {
        Pos_Step2(&p, &p, c->turn);
        o = RotateByHeading(0x68, 0, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        o = RotateByHeading(0x10c, -0x58, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        o = RotateByHeading(0x1a8, -0xf4, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        o = RotateByHeading(0x200, -0x198, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        Pos_Step2(&p, &p, c->turn);
        c->turn = (unsigned char)((c->turn - 2) & 7);
        Pos_Step2(&p, &p, c->turn);
        c->wp[n].x = p.x << 16;
        c->wp[n].y = p.y << 16;
    }
    c->start = p;
    c->nwp = (unsigned char)(n + 1);
}

/* ---- the road graph (ridecb5.c / ridecb6.c own these records) ----------- */

/* A packed 2-byte map square passed BY VALUE, and its 16-bit view. */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

/* One 4x4-map-square road block.  ridecb5.c/ridecb6.c spell +0x0c/+0x10 as
 * two separate `int x, y` fields; the two functions below take the ADDRESS
 * of the pair and hand it to Pos_Step4, so here it has to be one CarPos. */
typedef struct RoadRec {
    struct RoadRec* next;           /* +0x00 */
    int             f04;            /* +0x04 */
    BPosW           school;         /* +0x08  the owning driving school */
    unsigned char   pad0a[2];
    CarPos          pos;            /* +0x0c  the block origin, a multiple of 4 */
    unsigned char   kind;           /* +0x14  low nibble = piece kind */
    unsigned char   f15;            /* +0x15  steps from the entrance block */
    unsigned char   pad16[2];
    struct RoadRec* prev;           /* +0x18  the block the path walk came from */
    unsigned char   pad1c[4];
} RoadRec;                          /* 0x20 */

/* The 4x4 block covering (x, y) / whose ORIGIN is exactly (x, y). */
extern RoadRec* GetRoadRecord(int x, int y);                    /* 0x004125f0 */
extern RoadRec* Road_FindAt(int x, int y);                      /* 0x004125a0 */
/* The school's kind-6 ENTRANCE block, where a lesson starts. */
extern RoadRec* Road_FindStartPiece(BPosW school);              /* 0x00412650 */
/* Steps a map square a whole block (4 squares) in the heading `dir`. */
extern void     Pos_Step4(CarPos* from, CarPos* to, int dir);   /* 0x004808d0 */

/* The free-driving chooser.
 *
 * NOTE ON THE `school` PARAMETER TYPE.  goldrush.c declares both choosers
 * `(unsigned short school, ...)`; here they take the road family's `BPosW`,
 * the same 2-byte-struct-by-value that ridecb6.c gives Road_FindStartPiece.
 * The two are ABI-identical -- a 2-byte aggregate and a u16 both occupy one
 * dword argument slot and are read back as words -- and BOTH spellings
 * compile these two functions to 0 mismatches, measured.  BPosW is kept so
 * that the road record, Road_FindStartPiece and the choosers all agree; no
 * other file was touched. */
int SchoolCarNextManoeuvreHorn(BPosW school, CarPos* start, int dir); /* 0x00401f30 DEFINED below */

/* =========================================================================
 * 0x00402150 -- SchoolCarNextManoeuvre: follow the walked route.
 *
 * `start` is the car's manoeuvre square (+0x20) and `dir` its heading
 * (+0xba).  The block under `start` and the block one whole block ahead of
 * it are looked up; the answer is decided by where the block AHEAD was
 * reached from during DrivingSchool_ResetPaths' walk (its +0x18), which is
 * the only route information the game stores:
 *
 *   ahead->prev is one block LEFT of ahead    -> 1 (turn left)
 *   ahead->prev is one block RIGHT of ahead   -> 3 (turn right)
 *   ahead->prev is one block BEYOND ahead     -> 2 (straight on)
 *   ahead->prev is the block we are on        -> 2 (straight on)
 *   none of those                             -> 0 (stop)
 *
 * Four route-less cases hand over to the free-driving chooser instead: no
 * block under the car, the car's block is the entrance (prev == 0, so the
 * walk never reached it), no block ahead or a block ahead owned by another
 * school, and -- unless the block ahead is a kind 4 or 5 junction -- any
 * block ahead that is not the one immediately EAST of the school's own
 * entrance block.  A kind-6 block ahead is the school itself: manoeuvre 5,
 * the arrival pair.
 *
 * TWO BLOCK-LAYOUT LEVERS were needed to reach 209/209 exactly, and both are
 * new:
 *
 *  1. The leading `if (rec == 0)` and the function's last statement return
 *     the SAME constant, and VC6 merges the two `return 0` blocks -- but it
 *     puts the merged block at the FIRST site, inline after the guard, where
 *     the original has it at the very end with a `je rel32` reaching forward
 *     to it.  Writing the guard as `goto fail;` with `fail: return 0;` as the
 *     last statement pins the block at the last site.  (This is the mirror of
 *     the recorded `goto`-target rule: a labelled target that IS the final
 *     fall-through block stays last.)
 *
 *  2. `if (a == 0 || b != c) return X;` EXILES the X block past the whole
 *     rest of the function -- both tests then take a `rel32` to it -- where
 *     the original has X inline between the guard and the body, with the
 *     second test branching AROUND it.  Splitting the short-circuit and
 *     jumping INTO the second `if`'s compound statement,
 *
 *         if (a == 0) goto horn2;
 *         if (b != c) { horn2: return X; }
 *
 *     puts X back inline, because a single-test `if (cond) return X;` always
 *     makes X the test's fall-through.  Worth 157 -> 0 here.  Splitting it
 *     into two separate `if`s with two textual returns instead costs 173, and
 *     the `if (a && b) goto ok; return X; ok:` inversion is inert.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x00402150
int SchoolCarNextManoeuvre(BPosW school, CarPos* start, int dir)
{
    RoadRec* rec;
    RoadRec* ahead;
    RoadRec* home;
    RoadRec* prev;
    CarPos   p;

    rec = GetRoadRecord(start->x, start->y);
    if (rec == 0)
        goto fail;
    if (rec->prev == 0)
        return SchoolCarNextManoeuvreHorn(school, start, dir);

    Pos_Step4(&rec->pos, &p, dir);
    ahead = Road_FindAt(p.x, p.y);
    if (ahead == 0)
        goto horn2;
    if (ahead->school.w != school.w) {
horn2:
        return SchoolCarNextManoeuvreHorn(school, start, dir);
    }
    if ((ahead->kind & 0xf) == 6)
        return 5;

    home = Road_FindStartPiece(school);
    if (home->pos.y != ahead->pos.y || home->pos.x + 4 != ahead->pos.x) {
        if ((ahead->kind & 0xf) != 4 && (ahead->kind & 0xf) != 5)
            return SchoolCarNextManoeuvreHorn(school, start, dir);
    }

    prev = ahead->prev;

    Pos_Step4(&rec->pos, &p, dir);
    Pos_Step4(&p, &p, (dir - 2) & 7);
    if (prev->pos.x == p.x && prev->pos.y == p.y)
        return 1;

    Pos_Step4(&rec->pos, &p, dir);
    Pos_Step4(&p, &p, (dir + 2) & 7);
    if (prev->pos.x == p.x && prev->pos.y == p.y)
        return 3;

    Pos_Step4(&rec->pos, &p, dir);
    Pos_Step4(&p, &p, dir);
    if (prev->pos.x == p.x && prev->pos.y == p.y)
        return 2;

    if (prev->pos.x == rec->pos.x && prev->pos.y == rec->pos.y)
        return 2;
fail:
    return 0;
}

extern int rand(void);                                          /* 0x0049e4b2 (CRT) */

/* =========================================================================
 * 0x00401f30 -- SchoolCarNextManoeuvreHorn: pick an exit at random.
 *
 * The chooser StepSchoolCar uses while the car's horn timer (+0xc0) is
 * running, and the fallback SchoolCarNextManoeuvre drops into whenever the
 * walked route cannot answer.  It ignores the route entirely: it takes the
 * block one whole block ahead and asks which of that block's LEFT, STRAIGHT
 * ON and RIGHT neighbours are usable -- present, this school's, and not the
 * kind-6 entrance -- then builds a mask
 *
 *      1 = left free    2 = straight on free    4 = right free
 *
 * and picks one of the free exits with two bits of a fresh rand().  With no
 * usable block ahead at all it returns 4, the give-way manoeuvre; with a
 * usable block ahead but no free exit beyond it (mask 0) it returns 2 and
 * drives straight into it.
 *
 * `flags` lives in the dead `start` argument slot and `rand()` is called
 * BEFORE the null test, so a car that finds no road under itself still
 * consumes a random number -- reproduced.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x00401f30
int SchoolCarNextManoeuvreHorn(BPosW school, CarPos* start, int dir)
{
    /* The three exits, as ONE ARRAY: only nb[EX_ON] is ever spilled, but the
     * array reserves all three homes, which is what makes the frame 0x14 with
     * eight dead bytes in it and pushes `flags` out into the dead `start`
     * argument slot.  Three separate pointer locals give a 0xc frame and put
     * the wrong one in the argument slot (50 mismatches).  Which of the other
     * two indices is left and which is right is NOT recoverable -- they stay
     * in registers, so both assignments are byte-identical. */
    enum { EX_LEFT = 0, EX_RIGHT = 1, EX_ON = 2 };
    RoadRec* rec;
    RoadRec* ahead;
    RoadRec* nb[3];
    CarPos   p;
    int      flags;
    int      r;

    rec = GetRoadRecord(start->x, start->y);
    r = rand() & 3;
    flags = 0;
    if (rec == 0)
        return 0;

    Pos_Step4(&rec->pos, &p, dir);
    ahead = Road_FindAt(p.x, p.y);
    if (ahead == 0 || ahead->school.w != school.w || (ahead->kind & 0xf) == 6)
        return 4;

    Pos_Step4(&ahead->pos, &p, dir);
    nb[EX_ON] = Road_FindAt(p.x, p.y);
    if (nb[EX_ON] != 0 &&
        (nb[EX_ON]->school.w != school.w || (nb[EX_ON]->kind & 0xf) == 6))
        nb[EX_ON] = 0;

    Pos_Step4(&ahead->pos, &p, (dir - 2) & 7);
    nb[EX_LEFT] = Road_FindAt(p.x, p.y);
    if (nb[EX_LEFT] != 0 &&
        (nb[EX_LEFT]->school.w != school.w || (nb[EX_LEFT]->kind & 0xf) == 6))
        nb[EX_LEFT] = 0;

    Pos_Step4(&ahead->pos, &p, (dir + 2) & 7);
    nb[EX_RIGHT] = Road_FindAt(p.x, p.y);
    if (nb[EX_RIGHT] != 0 &&
        (nb[EX_RIGHT]->school.w != school.w || (nb[EX_RIGHT]->kind & 0xf) == 6))
        nb[EX_RIGHT] = 0;

    if (nb[EX_LEFT])
        flags = 1;
    if (nb[EX_RIGHT])
        flags |= 4;
    if (nb[EX_ON])
        flags |= 2;

    /* CASE ORDER IS THE BLOCK ORDER: 1, 4, 5, 3, 6, 7 is the original's, and
     * writing them 1, 3, 4, 5, 6, 7 costs 17.  Every two-way choice is spelled
     * `if (r & 2) return A; return B;` -- VC6 turns that into the original's
     * branchless bit trick (`not / movsx / and / or`, and `shr` for case 6
     * because it knows the value is small).  The TERNARY spelling
     * `(r & 2) ? A : B` produces the same trick but computes the complement
     * into a fresh register (`mov al,bl / not al`) instead of in place, which
     * is 18 mismatches and 3 bytes; case 3 is the one arm where the two are
     * identical, because there the constants differ in bit 0 and VC6 uses
     * neg/sbb/neg instead. */
    switch (flags) {
    case 1:
        return 1;
    case 4:
        return 3;
    case 5:
        if (r & 2)
            return 1;
        return 3;
    case 3:
        if (r & 2)
            return 2;
        return 1;
    case 6:
        if (r & 2)
            return 2;
        return 3;
    case 7:
        if (r & 2)
            return 2;
        if (r & 1)
            return 1;
        return 3;
    }
    return 2;
}
