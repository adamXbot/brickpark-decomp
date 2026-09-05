/* LEGOLAND -- the DRIVING SCHOOL CAR's three remaining leaf routines: the
 * road-occupancy gate, the STRAIGHT-ON manoeuvre and the driver's own draw
 * callback.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  None of
 * these functions is exported; every extent comes from the disassembly by
 * control flow (tools/audit.py).  Struct field OFFSETS, record sizes and
 * global addresses are load-bearing; the names are ours.  Types are declared
 * LOCALLY on purpose (legoland.h is owned elsewhere) and follow goldrush.c
 * (which is the caller of all three and states their contracts),
 * schoolcar.c, schoolcar2.c and ridecb5.c / ridecb6.c / ridecb8.c.
 *
 *   0x00402430  SchoolCarMayEnterSquare  41/41 insns,  96/96 B, exact
 *   0x004015e0  SchoolCarManoeuvreB      42/42 insns, 128/128 B, exact
 *   0x00402550  SchoolCar_DrawDriver     43/43 insns, 125/125 B, exact
 *
 * All three closed first try from the shapes schoolcar2.c and goldrush.c
 * already record; the per-function notes below say which lever did what.
 *
 * =========================================================================
 * WHAT THIS FILE ADDS TO THE PICTURE  (spec notes for a browser runtime)
 * =========================================================================
 * A ROAD BLOCK CARRIES TWO OCCUPANCY COUNTERS, and they are the whole
 * traffic model.  Both live in the four bytes ridecb5.c/ridecb6.c spell as
 * `pad1c`, and both are zeroed by NewRoadRecord (0x004132a0):
 *
 *   +0x1c  CARS on this 4x4 block.  Maintained exclusively by
 *          SchoolCarMayEnterSquare: incremented on the block a car moves
 *          ONTO, decremented on the block it came from.
 *   +0x1d  PEDESTRIAN claims.  Bumped by Road_TileClaim (0x004139c0) and
 *          Road_TileRelease (0x004139e0) -- the two road-tile hooks
 *          Roads_LoadResources patches into the "TILES FOR DSCHOOL" object
 *          definition (ridecb8.c), so every bloke that walks over a road
 *          block claims it while it is there.
 *
 * THE GIVE-WAY RULE.  A car is refused a block iff ALL THREE hold:
 *
 *      the block's kind byte (+0x14) has bit 0x10 set   (a crossing block)
 *   && the block currently has NO car on it             (+0x1c == 0)
 *   && at least one pedestrian is claiming it           (+0x1d != 0)
 *
 * So the FIRST car to arrive at a crossing must wait for it to clear, but
 * once a car is on it another may follow: the check reads the counter
 * BEFORE the increment, and a non-zero car count short-circuits the whole
 * test.  StepSchoolCar treats a refusal as "roll the whole move back" --
 * position, velocity integration and map square are all restored -- so a
 * blocked car simply does not move that frame.
 *
 * The gate is also a no-op whenever the two squares resolve to the SAME
 * block (the common case: a 4x4 block is four map squares across, so most
 * frames the car does not leave it) or when the destination is not a road
 * block at all.  Both of those answer "yes" without touching a counter,
 * which is why a car driven off the road never leaks a count.
 *
 * MANOEUVRE 2, STRAIGHT ON, is the simplest of the four: two half-block
 * steps in the car's current heading, ONE waypoint at the far end, and the
 * heading is not touched.  Two half blocks is one whole block, so a
 * straight-on manoeuvre moves the car exactly one road block -- the same
 * distance the two turns cover, which is what keeps every car on the 4x4
 * lattice.  Note it lays only ONE waypoint where the turns lay four or five:
 * a straight line needs no intermediate control points.
 *
 * THE DRIVER IS DRAWN AS A SECOND SPRITE OVER THE CAR BODY, from
 * SortSpriteWithCallback's callback slot, so it lands in the same depth
 * bucket as the body and is blitted immediately after it.  Three facts:
 *  - the bloke's 3D model is rendered FIRST, before the palette is even
 *    chosen, so the person is under the driver sprite;
 *  - the driver uses the SAME per-livery palette as the body (1 -> 0x82c6bc,
 *    2 -> 0x82c6b8, 3 -> 0x82c690) and the SAME sprite (0x00830f94);
 *  - its frame is the car's LAST DRAWN body frame (+0xb9, which StepSchoolCar
 *    copies from +0xb8 just before it queues the sprite) plus 0x10.  So the
 *    image list holds sixteen car-body frames followed by sixteen matching
 *    driver frames, and the driver is always in step with the body.
 * ========================================================================= */

/* ---- the car record (schoolcar.c owns it; these offsets are its) -------- */
typedef struct CarPos   { int x; int y; } CarPos;
typedef struct Waypoint { int x; int y; } Waypoint;

typedef struct SchoolCar {
    struct SchoolCar* next;      /* +0x00 */
    unsigned short    school;    /* +0x04  the school's packed map square */
    unsigned char     pad06[2];
    int               sx;        /* +0x08  screen position, this frame */
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
    unsigned char     frame;     /* +0xb8  body frame, 0..15 */
    unsigned char     b9;        /* +0xb9  last drawn body frame */
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

/* ---- the road graph (ridecb5.c / ridecb6.c own the record) -------------- */

/* One 4x4-map-square road block.  ridecb5.c and ridecb6.c both stop at
 * `pad1c[4]`; the two occupancy counters live in the first two bytes of it,
 * so they are named here and nowhere else.  The block ORIGIN is spelled as
 * two separate ints in both of those files; this file never takes its
 * address, so either spelling works and theirs is kept. */
typedef struct RoadRec {
    struct RoadRec* next;           /* +0x00 */
    int             f04;            /* +0x04 */
    unsigned short  school;         /* +0x08 the owning driving school */
    unsigned char   pad0a[2];
    int             x;              /* +0x0c the block origin, a multiple of 4 */
    int             y;              /* +0x10 */
    unsigned char   kind;           /* +0x14 low nibble = piece kind,
                                     *       bit 0x10 = give way to walkers */
    unsigned char   f15;            /* +0x15 */
    unsigned char   pad16[2];
    struct RoadRec* prev;           /* +0x18 the path-walk mark */
    unsigned char   cars;           /* +0x1c cars on this block */
    unsigned char   walkers;        /* +0x1d pedestrian claims */
    unsigned char   pad1e[2];
} RoadRec;                          /* 0x20 */

/* The 4x4 block COVERING (x, y), or 0 (ridecb5.c owns it). */
extern RoadRec* GetRoadRecord(int x, int y);                    /* 0x004125f0 */

/* =========================================================================
 * 0x00402430 -- MAY THE CAR MOVE FROM `from` TO `to`?
 *
 * See the file header for the rule and for what the two counters are.  The
 * function is not a pure predicate: on the "yes" answer it also TRANSFERS
 * the car's occupancy from one block to the other, so StepSchoolCar must
 * call it exactly once per frame and must roll the move back itself when the
 * answer is 0.
 *
 * The `from` block is decremented only when it exists AND its count is
 * non-zero -- a saturating decrement, which is what stops a car that was
 * spawned off-road (or driven onto a block created since it arrived) from
 * wrapping the unsigned byte to 0xff.
 *
 * LEVERS:
 *  - The two guards are ONE short-circuit `&&` around the whole body, so
 *    both `je`s target the same trailing `mov eax,1`.  Two separate
 *    `if (...) return 1;` guards would merge that constant at the FIRST
 *    site and pull it to the top of the function.
 *  - The refusal is a three-term `&&` with `return 0;` as its
 *    FALL-THROUGH, which is the shape a single-test `if (cond) return 0;`
 *    always emits -- the recorded exile rule's positive case.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x00402430
int SchoolCarMayEnterSquare(CarPos* to, CarPos* from)
{
    RoadRec* a = GetRoadRecord(to->x, to->y);
    RoadRec* b = GetRoadRecord(from->x, from->y);

    if (b != a && a != 0) {
        if ((a->kind & 0x10) && a->cars == 0 && a->walkers != 0)
            return 0;
        if (b != 0 && b->cars != 0)
            b->cars--;
        a->cars++;
    }
    return 1;
}

/* Steps a map square 2 map squares (half a road block) in the heading
 * `dir`; the four even headings leave `to` untouched (schoolcar2.c). */
extern void Pos_Step2(CarPos* from, CarPos* to, int dir);       /* 0x00480840 */

/* =========================================================================
 * 0x004015e0 -- SchoolCarManoeuvreB: append the STRAIGHT-ON move (code 2).
 *
 * The template is schoolcar2.c's SchoolCarManoeuvreA / SchoolCarManoeuvreC
 * with the curve removed: read the waypoint count and the manoeuvre square,
 * step the square twice by half a block in the CURRENT heading, hand the
 * square back as the next manoeuvre's start, lay ONE waypoint at it and bump
 * the count.  The heading (+0xba) is never written, which is what makes this
 * the straight-through move.
 *
 * LEVERS (all inherited from the A/C pair and confirmed here):
 *  - `int n = c->nwp;` -- an `int` local for the u8 field is what buys
 *    `xor ebx,ebx / mov bl,[..]`, and `c->nwp = (unsigned char)(n + 1)`
 *    narrows the store back to `inc bl / mov [..],bl`.
 *  - `c->start = p;` is a WHOLE-STRUCT assignment (two register moves out of
 *    the frame pair), and it is written BEFORE the waypoint stores: the
 *    original emits start.x, start.y, wp.x, wp.y and re-loads `p.y` from the
 *    frame for the second waypoint rather than reusing the register the
 *    struct copy left it in.
 *  - Both `Pos_Step2(&p, &p, c->turn)` calls take the SAME address twice and
 *    the original materialises it with two separate `lea`s per call; the
 *    heading is re-read from the record for the second call.  Their six
 *    argument pushes are cleaned by ONE `add esp,0x18`.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x004015e0
void SchoolCarManoeuvreB(SchoolCar* c)
{
    CarPos p;
    int    n;

    n = c->nwp;
    p = c->start;
    Pos_Step2(&p, &p, c->turn);
    Pos_Step2(&p, &p, c->turn);
    c->start = p;
    c->wp[n].x = p.x << 16;
    c->wp[n].y = p.y << 16;
    c->nwp = (unsigned char)(n + 1);
}

/* ---- the driver's draw pass (goldrush.c owns the caller) ---------------- */
extern void IP_RenderBlokeIn3DNow(void* bloke);                  /* 0x00440010 */
extern void SetOverridePalette(void* pal);                       /* 0x00464400 */
extern void SetOverrideFrame(int frame);                         /* 0x00464420 */
extern void ClearOverrideFrame(void);                            /* 0x00464440 */
extern void ClearOverridePalette(void);                          /* 0x00464450 */
extern int  PrintSprite(void* s, int x, int y, int m, void* c); /* 0x004853a0 */

extern void* g_car_sprite;      /* 0x00830f94 */
extern void* g_car_pal_a;       /* 0x0082c690  livery 3 */
extern void* g_car_pal_b;       /* 0x0082c6b8  livery 2 */
extern void* g_car_pal_c;       /* 0x0082c6bc  livery 1 */

/* =========================================================================
 * 0x00402550 -- SchoolCar_DrawDriver: SortSpriteWithCallback's callback.
 *
 * StepSchoolCar (goldrush.c) hands this to SortSpriteWithCallback together
 * with `(int)c`, so the parameter really is the car pointer smuggled through
 * an `int` -- the extern in goldrush.c is `void (*)(int)` and that is the
 * type the print list stores.  See the file header for what it draws.
 *
 * LEVERS:
 *  - The livery `switch` lowers to `dec eax / je / dec eax / je / dec eax /
 *    jne`, so case 3 is the fall-through and cases 2 and 1 follow it in that
 *    order -- the recorded compare-chain rule, where the layout follows the
 *    case VALUES and the natural 1, 2, 3 source order is what produces it.
 *    All three arms only PUSH their palette and jump to one shared
 *    `call SetOverridePalette`: VC6 cross-jumps the identical call tails by
 *    itself, so this is one textual call per case, not a colour variable.
 *  - SetOverrideFrame's argument push is NOT cleaned on its own -- its
 *    `add esp,4` merges into PrintSprite's `add esp,0x18` (24 bytes = five
 *    pushes plus the pending one).  The palette call's cleanup does not
 *    merge, because the three-way join sits between it and the next call.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x00402550
void SchoolCar_DrawDriver(int arg)
{
    SchoolCar* c = (SchoolCar*)arg;

    IP_RenderBlokeIn3DNow(c->bloke);
    switch (c->livery) {
    case 1:
        SetOverridePalette(g_car_pal_c);
        break;
    case 2:
        SetOverridePalette(g_car_pal_b);
        break;
    case 3:
        SetOverridePalette(g_car_pal_a);
        break;
    }
    SetOverrideFrame(c->b9 + 0x10);
    PrintSprite(g_car_sprite, c->sx, c->sy, 0, 0);
    ClearOverrideFrame();
    ClearOverridePalette();
}
