/* LEGOLAND -- WESTERN TOWN part 2: the customer state machines and the two
 * remaining overlay draws.
 *
 * Companion to LEGOLAND/westtown.c, which holds the nine classes' resource
 * load/free, placement-cursor, add/remove and the simple overlay draws.
 * Read that file's header first: it documents the class list, the shared
 * +0xa0 draw descriptor and +0x9c remove, and the JAIL CELL record.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere).
 *
 * ==========================================================================
 * WHAT A WESTERN TOWN CUSTOMER SCRIPT IS
 * ==========================================================================
 * Every +0xa8 handler walks the class-wide rider list at ObjDef+0xcc once per
 * tick. For each rider it captures `next` FIRST (the last state frees the
 * node), reads the bloke, converts the placement's packed {x,y} square into
 * tile coordinates by adding the class base offsets (ObjDef +0x0c/+0x10),
 * skips the rider while its low-level AI state (Bloke+0x0e) is non-zero, and
 * then switches on the ACTION byte (Bloke+0x60) through a /Gy jump table.
 *
 * Each walking state writes a waypoint in 24.8 world units into Bloke
 * +0x24/+0x28 and then runs ONE shared tail (ShopGo below):
 *
 *     d = CalcMoveLine(bloke->world, bloke->target, bloke->path) + 0x10;
 *     bloke->state = 7;                  // busy for 7 ticks
 *     bloke->new_dir = d;
 *     NewDirForAction(bloke, (d >> 5) + 3);
 *     bloke->action++;                   // next step of the script
 *
 * State 0 always ORs 8 into Bloke+0x62 ("inside a building") and the LAST
 * state always clears it and calls RemoveBlokeFromRide. Everything between
 * is per-class furniture: waypoints, waits, random rolls and forced facing
 * directions. So a browser runtime needs no per-shop state at all -- just
 * the waypoint table, the jump table's length, and the few random rolls.
 *
 * ==========================================================================
 * CODEGEN LEVERS THIS FILE PAID FOR (all measured, all reproducible)
 * ==========================================================================
 * 1. WAYPOINT STORES, NOT ARGUMENTS. Writing `ShopStep(b, X, Y)` and letting
 *    the inline store the two fields makes VC6 evaluate Y FIRST (argument
 *    order) and lays the push block out differently. Writing the two stores
 *    out at the case and calling a tail helper that only reads b->target is
 *    what matches. That one change took BANK from 221 instructions to 255.
 * 2. STORE ORDER vs ALU ORDER ARE SEPARATE KNOBS. `b->target.x = A;
 *    b->target.y = B;` gives ALU order A,B and store order x,y; assigning
 *    both to locals first and then storing INVERTS the store order while
 *    keeping the ALU order. Cases that need "compute x first, store y first"
 *    only come out of the local form (Bank case 0, JAIL CELL case 0).
 * 3. BRANCH DIRECTION picks which arm is laid out first: write the arm the
 *    original falls into as the THEN. `if (--t <= 0) action = 5; else walk;`
 *    matches where `if (--t > 0) walk; else action = 5;` does not.
 * 4. AN ALIASING BARRIER CAN BE THE POINT. Shop_BrowseAndBuy re-reads the
 *    browse counter after incrementing the action byte; that only happens if
 *    the increment goes through a plain `unsigned char*`.
 * 5. TAIL-MERGE CONTROL. When two arms of an `if` end in the same stores VC6
 *    cross-jumps them and the block shrinks; moving ONE store to the end of
 *    one arm (JAIL CELL: `taken = 1` after the waypoint, `dir = 7` after the
 *    waypoint) keeps both copies, which is what the original has.
 * 6. THE BAND LOOP. See the note above ShopDrawBand: `if (n > 0) { k = n;
 *    do { ... } while (--k); }` over the PARAMETER as its own cursor is what
 *    rematerialises the count's sign extension per band; `for (i = 0; i < n;
 *    i++)` hoists one and re-registers the whole function.
 */

/* ---- shared types (same offsets as westtown.c / ridecb1.c) -------------- */

typedef struct ShopDef ShopDef;

typedef struct ShopElem {
    char*        name;         /* +0x00 */
    char*        image;        /* +0x04 */
    unsigned int type_flags;   /* +0x08 */
    ShopDef*     data;         /* +0x0c */
} ShopElem;

/* The placed object's map square, packed as two bytes. */
typedef struct MapSquare {
    unsigned char bx;          /* +0x00 */
    unsigned char by;          /* +0x01 */
} MapSquare;

/* An {x,y} pair in 24.8 world units, passed and returned by value. */
typedef struct Pos { int x; int y; } Pos;

typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;      /* +0x0e  low-level AI state (0 = idle) */
    unsigned char  pad10[0x14];
    Pos            target;     /* +0x24  walk target, 24.8 */
    unsigned char  pad2c[0x58 - 0x2c];
    int            timer;      /* +0x58  per-state countdown */
    int            wait;       /* +0x5c */
    unsigned char  action;     /* +0x60  state-machine step / occlusion band */
    unsigned char  pad61;
    unsigned short flags62;    /* +0x62  8 = using this ride */
    unsigned char  pad64[4];
    Pos            world;      /* +0x68  world position, 24.8 */
    unsigned short f70;        /* +0x70 */
    unsigned char  dir;        /* +0x72 */
    unsigned char  new_dir;    /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14]; /* +0x98  CalcMoveLine scratch */
} Bloke;

typedef struct RiderNode {
    struct RiderNode* next;    /* +0x00 */
    struct RiderNode* prev;    /* +0x04 */
    Bloke*            bloke;   /* +0x08 */
    MapSquare         square;  /* +0x0c  the placement's own map square */
    unsigned short    pad0e;
    void*             person;  /* +0x10 */
} RiderNode;

struct ShopDef {
    unsigned char  pad00[0x0c];
    int            base_x;     /* +0x0c  the class's base map square */
    int            base_y;     /* +0x10 */
    unsigned char  pad14[0x1c - 0x14];
    unsigned int   flags;      /* +0x1c */
    unsigned char  pad20[0x64 - 0x20];
    void*          sprite;     /* +0x64  the class's layer holder */
    unsigned char  pad68[0xcc - 0x68];
    RiderNode*     riders;     /* +0xcc  class-wide customer list */
};

/* ---- callees ------------------------------------------------------------ */
extern int    CalcMoveLine(Pos from, Pos to, void* path);            /* 0x00480740 */
extern int    NewDirForAction(Bloke* b, unsigned char dir);          /* 0x004833d0 */
extern void   RemoveBlokeFromRide(ShopDef* def, RiderNode* r);       /* 0x0048a100 */
extern void   BuyItem(ShopElem* elem, MapSquare* at, int which);     /* 0x004539e0 */
extern int    rand(void);                                            /* 0x0049e4b2 (CRT) */

/* =========================================================================
 * 0x00437570 -- THE SHARED "BROWSE, THEN MAYBE BUY" STEP.
 *
 * Called from the middle state of several western-town customer scripts (the
 * one where the visitor is standing at the counter). Per tick:
 *
 *   - if the browse counter has already reached 0, advance the script;
 *   - decrement the counter, and on every 32nd tick (counter & 0x1f == 0)
 *     roll rand()%100: on 0..30 -- a 31% chance -- charge the visitor with
 *     BuyItem(elem, square, which) and advance the script again.
 *
 * So a purchase does NOT end the browse; it advances the state a SECOND time
 * on the tick it happens, which is how the same state both times out and
 * completes early. `which` is the price/stat index money.c bills against, and
 * is 1 for every western-town caller found so far.
 *
 * NOTE the counter is never initialised by this function: it is whatever the
 * previous state left in Bloke+0x58, and it is signed, so a customer that
 * misses the == 0 test keeps counting down through negative values and only
 * the & 0x1f test can still fire. Reproduced.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00437570
void Shop_BrowseAndBuy(RiderNode* r, ShopElem* elem, MapSquare* key, int which)
{
    Bloke*         b  = r->bloke;
    unsigned char* pa = &b->action;   /* see the note above: the byte pointer
                                       * is what makes VC6 re-read the counter
                                       * after the first increment */

    if (b->timer == 0)
        (*pa)++;
    if ((b->timer-- & 0x1f) == 0) {
        if (rand() % 100 <= 30) {
            BuyItem(elem, key, which);
            r->bloke->action++;
        }
    }
}

/* The tail every walking state of every western-town script shares: run the
 * move line to the waypoint the state has just written into Bloke+0x24/+0x28,
 * mark the low-level AI busy for 7 ticks, stash the raw direction byte, turn
 * to face it and advance the script by one step. */
static __inline void ShopGo(Bloke* b)
{
    unsigned char a;

    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
    b->state = 7;
    b->new_dir = a;
    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
    b->action++;
}

/* The walking tail without the script advance: used by the states that have
 * to seed a timer between the move and the advance. */
static __inline void ShopMove(Bloke* b)
{
    unsigned char a;

    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
    b->state = 7;
    b->new_dir = a;
    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
}


/* The same tail, but stepping the script BACKWARDS: used by the counter
 * states that ping-pong between two spots while a timer runs down. */
static __inline void ShopGoBack(Bloke* b)
{
    unsigned char a;

    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
    b->state = 7;
    b->new_dir = a;
    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
    b->action--;
}

/* Aim at a waypoint and go. */
static __inline void ShopStep(Bloke* b, int tx, int ty)
{
    b->target.x = tx;
    b->target.y = ty;
    ShopGo(b);
}

/* =========================================================================
 * 0x00437c90 -- SHERIFF, +0xa8: the 8-state customer script.
 *
 * Waypoints are in tiles relative to (class base square + the placement's
 * own square), converted to 24.8 world units.
 *
 *   0  step to (x-1, y+0.5) and mark the customer "inside" (flags62 |= 8)
 *   1  step to (x-3, y+0.5) -- up to the counter
 *   2  step to (x-3, y + rand()%3), or (x-3, y+0.5) when the roll is 0,
 *      and force the facing direction to 8 (south-east): the visitor lines
 *      up somewhere along the counter and turns to face the sheriff
 *   3  browse and maybe buy (Shop_BrowseAndBuy, price index 1)
 *   4  step back to (x-3, y+0.5)
 *   5  step to (x-1, y+0.5)
 *   6  step to (x, y) -- the doorway
 *   7  leave: RemoveBlokeFromRide and clear the "inside" flag
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00437c90
void Sheriff_TickCustomers(ShopElem* elem)
{
    ShopDef*      def = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           tx;
    int           ty;
    char          rem;

    r = def->riders;
    while (r) {
        next = r->next;
        key = &r->square;
        b = r->bloke;
        tx = key->bx + def->base_x;
        ty = key->by + def->base_y;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                ShopStep(b, (tx << 8) - 0x100, (ty << 8) + 0x80);
                break;
            case 1:
                ShopStep(b, (tx - 3) << 8, (ty << 8) + 0x80);
                break;
            case 2:
                b->target.x = (tx - 3) << 8;
                rem = (char)(rand() % 3);
                if (rem != 0)
                    b->target.y = (rem + ty) << 8;
                else
                    b->target.y = (ty << 8) + 0x80;
                ShopStep(b, b->target.x, b->target.y);
                b->dir = 8;
                break;
            case 3:
                Shop_BrowseAndBuy(r, elem, key, 1);
                break;
            case 4:
                ShopStep(b, (tx - 3) << 8, (ty << 8) + 0x80);
                break;
            case 5:
                ShopStep(b, (tx << 8) - 0x100, (ty << 8) + 0x80);
                break;
            case 6:
                ShopStep(b, tx << 8, ty << 8);
                break;
            case 7:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8u;
                break;
            }
        }
        r = next;
    }
}

/* =========================================================================
 * 0x00438960 -- BANK, +0xa8: the 9-state customer script.
 *
 * The bank is the only western-town building whose script CYCLES: states 3
 * and 4 are a pair that step the customer between two spots at the counter,
 * one advancing the script and the other rewinding it, so the visitor paces
 * back and forth until the shared countdown in Bloke+0x58 runs out. That
 * countdown is seeded in state 2 with rand() % 8 (a SIGNED modulo, so a
 * negative seed means the 3/4 pair exits on its first tick).
 *
 * Waypoints marked (base) are base-square relative; states 2..4 use the
 * placement's own map square RAW, without the class base offset.
 *
 *   0  step to (base x+0.5, base y-1) and mark the customer "inside"
 *   1  step to (base x+1, base y-2)
 *   2  step to (key x+2, key y-1); timer = rand() % 8
 *   3  timer-- ; while it is still positive step to (key x, key y-1) and go
 *      to state 4, otherwise jump to state 5
 *   4  timer-- ; while it is still positive step to (key x+2, key y-1) and
 *      go BACK to state 3, otherwise jump to state 5
 *   5  step to (base x+1, base y-2)
 *   6  step to (base x+0.5, base y-1)
 *   7  step to (base x, base y) -- the doorway
 *   8  leave: RemoveBlokeFromRide and clear the "inside" flag
 *
 * Note the bank never calls Shop_BrowseAndBuy: visiting the bank costs the
 * player nothing, it is pure decoration.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00438960
void Bank_TickCustomers(ShopElem* elem)
{
    ShopDef*      def = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    int           tx;
    int           ty;

    r = def->riders;
    while (r) {
        next = r->next;
        b = r->bloke;
        tx = r->square.bx + def->base_x;
        ty = r->square.by + def->base_y;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                tx = (tx << 8) + 0x80;
                ty = (ty << 8) - 0x100;
                b->target.x = tx;
                b->target.y = ty;
                ShopGo(b);
                break;
            case 1:
                b->target.x = (tx + 1) << 8;
                b->target.y = (ty - 2) << 8;
                ShopGo(b);
                break;
            case 2:
                b->target.x = (r->square.bx + 2) << 8;
                b->target.y = (r->square.by << 8) - 0x100;
                ShopGo(b);
                b->timer = rand() % 8;
                break;
            case 3:
                if (--b->timer <= 0) {
                    b->action = 5;
                } else {
                    b->target.x = r->square.bx << 8;
                    b->target.y = (r->square.by << 8) - 0x100;
                    ShopGo(b);
                }
                break;
            case 4:
                if (--b->timer <= 0) {
                    b->action = 5;
                } else {
                    b->target.x = (r->square.bx + 2) << 8;
                    b->target.y = (r->square.by << 8) - 0x100;
                    ShopGoBack(b);
                }
                break;
            case 5:
                b->target.x = (tx + 1) << 8;
                b->target.y = (ty - 2) << 8;
                ShopGo(b);
                break;
            case 6:
                b->target.x = (tx << 8) + 0x80;
                b->target.y = (ty << 8) - 0x100;
                ShopGo(b);
                break;
            case 7:
                b->target.x = tx << 8;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 8:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8u;
                break;
            }
        }
        r = next;
    }
}


/* =========================================================================
 * 0x00439ef0 -- LEGO MEDIA SHOP, +0xa8: the 7-state customer script.
 *
 * The media shop is the browsing shop: two of its states pick one of THREE
 * display spots at random (rand() % 3), and state 3 does it by TELEPORTING
 * the customer -- it writes Bloke+0x68/+0x6c, the WORLD position, not the
 * walk target, so the visitor simply appears at the next display without
 * a move line. The three spots are (x-4, y), (x-3, y-2) and (x-4, y-2),
 * all relative to the class base square plus the placement square.
 *
 *   0  step to (x-2, y) and mark the customer "inside"
 *   1  walk to one of the three display spots, chosen at random
 *   2  browse and maybe buy (Shop_BrowseAndBuy, price index 1)
 *   3  JUMP straight to one of the three display spots (no walk) and advance
 *   4  step back to (x-2, y)   [the same waypoint as state 0]
 *   5  step to (x, y) -- the doorway
 *   6  leave: RemoveBlokeFromRide and clear the "inside" flag
 * ========================================================================= */
/* 169 of 169 instructions and every block byte-for-byte, but ONE cross-jump
 * goes the other way: the original makes case 1's middle arm jump FORWARD
 * into case 5's block (`add ebx,-3 / shl ebx,8 / mov [esi+0x24],ebx /
 * add edi,-2 / jmp 0x43a065`), keeping case 5 as the canonical copy of the
 * shared "store y, push, call" block; ours makes case 5 jump BACKWARD into
 * the arm's copy. Same instructions, mirrored. Measured and rejected: both
 * `ty -= 2` orders and the `(tx - 3) << 8` expression form in the arm, an
 * explicit `goto` into case 5's block, a `ShopGoY(b, ty)` inline that owns
 * the y store, case 0 falling through into case 4 (which is how the original
 * places case 4's entry, but it then merges the arm into case 0 instead:
 * 167 instructions), and swapping the textual order of cases 4 and 5. */
// WIP-FUNCTION: LEGOLAND 0x00439ef0  (169 of 169 instructions; one cross-jump mirrored -- see above)
void LegoMedia_TickCustomers(ShopElem* elem)
{
    ShopDef*      def = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           tx;
    int           ty;
    char          rem;

    r = def->riders;
    while (r) {
        next = r->next;
        key = &r->square;
        b = r->bloke;
        tx = key->bx + def->base_x;
        ty = key->by + def->base_y;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                tx -= 2;
                b->target.x = tx << 8;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 1:
                rem = (char)(rand() % 3);
                if (rem == 2) {
                    tx -= 4;
                    b->target.x = tx << 8;
                    b->target.y = ty << 8;
                    ShopGo(b);
                } else if (rem == 1) {
                    tx -= 3;
                    b->target.x = tx << 8;
                    ty -= 2;
                    b->target.y = ty << 8;
                    ShopGo(b);
                } else {
                    tx -= 4;
                    ty -= 2;
                    b->target.x = tx << 8;
                    b->target.y = ty << 8;
                    ShopGo(b);
                }
                break;
            case 2:
                Shop_BrowseAndBuy(r, elem, key, 1);
                break;
            case 3:
                rem = (char)(rand() % 3);
                if (rem == 2) {
                    tx -= 4;
                    b->world.x = tx << 8;
                    b->world.y = ty << 8;
                    b->action++;
                } else if (rem == 1) {
                    tx -= 3;
                    ty -= 2;
                    b->world.x = tx << 8;
                    b->world.y = ty << 8;
                    b->action++;
                } else {
                    tx -= 4;
                    ty -= 2;
                    b->world.x = tx << 8;
                    b->world.y = ty << 8;
                    b->action++;
                }
                break;
            case 4:
                tx -= 2;
                b->target.x = tx << 8;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 5:
                b->target.x = tx << 8;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 6:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8u;
                break;
            }
        }
        r = next;
    }
}


/* =========================================================================
 * 0x00439460 -- LEGO SHOP 1, +0xa8: the 7-state customer script.
 *
 * The most randomised script in Western Town: the browsing spot is jittered
 * by one tile on BOTH axes (two separate rand() % 2 rolls), the queue spot
 * by rand() % 3 on y, and both waiting states get a random length. It is
 * also the only script that spins the customer on the spot: state 2 turns
 * the visitor one step clockwise every tenth tick of its wait (Bloke+0x72,
 * the facing direction, wrapping 0..8) so the shopper looks around while
 * queueing.
 *
 *   0  step to (x-5, y) and mark the customer "inside"
 *   1  step to (x-5 + rand()%2, y+3 + rand()%2); wait = rand() % 50
 *   2  count the wait down; when it reaches 0 advance, otherwise every tenth
 *      tick turn one step (dir++, wrapping past 8 back to 0)
 *   3  step to (x-5, y + rand()%3); wait = rand() % 30
 *   4  browse and maybe buy (Shop_BrowseAndBuy, price index 1)
 *   5  step to (x, y) -- the doorway
 *   6  leave: RemoveBlokeFromRide and clear the "inside" flag
 *
 * NOTE the wait test is `== 0`, not `<= 0`: a negative rand() % 50 (the
 * modulo is signed) makes state 2 run 2^32 ticks. Reproduced.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x00439460
void LegoShop1_TickCustomers(ShopElem* elem)
{
    ShopDef*      def = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           tx;
    int           ty;
    int           n;

    r = def->riders;
    while (r) {
        next = r->next;
        key = &r->square;
        b = r->bloke;
        tx = key->bx + def->base_x;
        ty = key->by + def->base_y;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                tx -= 5;
                b->target.x = tx << 8;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 1:
                b->target.x = (rand() % 2 + tx - 5) << 8;
                b->target.y = (rand() % 2 + ty + 3) << 8;
                ShopMove(b);
                n = rand() % 50;
                b->action++;
                b->timer = n;
                break;
            case 2:
                if (--b->timer == 0) {
                    b->action++;
                } else if (b->timer % 10 == 0) {
                    b->dir++;
                    if (b->dir > 8)
                        b->dir = 0;
                }
                break;
            case 3:
                tx -= 5;
                b->target.x = tx << 8;
                b->target.y = (rand() % 3 + ty) << 8;
                ShopMove(b);
                n = rand() % 30;
                b->action++;
                b->timer = n;
                break;
            case 4:
                Shop_BrowseAndBuy(r, elem, key, 1);
                break;
            case 5:
                b->target.x = tx << 8;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 6:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8u;
                break;
            }
        }
        r = next;
    }
}


/* =========================================================================
 * 0x004378e0 -- GENERAL STORE, +0xa8: the 12-state customer script, the
 * longest in Western Town.
 *
 *   0  step to (x-0.5, y-1) and mark the customer "inside"
 *   1  step to (x+0.5, y-2)
 *   2  step to (x+0.5, y-3)
 *   3  toss a coin (rand() % 2): heads, walk to (x-1, y-2.5) -- the far
 *      counter -- and wait rand() % 50 ticks; tails, skip straight to state
 *      6, so half the customers never queue at all
 *   4  count the wait down; when it reaches 0 advance, otherwise turn one
 *      step clockwise every tenth tick (dir 0..8, same spin as LEGO SHOP 1)
 *   5  step to (x+0.5, y-3)
 *   6  advance one step and nothing else -- the join point for the coin
 *      toss in state 3 (VC6 merges it into state 4's advance)
 *   7  browse and maybe buy (Shop_BrowseAndBuy, price index 1)
 *   8  step to (x+0.5, y-2)
 *   9  step to (x-0.5, y-1)
 *  10  step to (x+0.5, y+0.5) -- the doorway
 *  11  leave: RemoveBlokeFromRide and clear the "inside" flag
 *
 * Same `== 0` wait test as LEGO SHOP 1, so a negative rand() % 50 wedges
 * state 4 for 2^32 ticks. Reproduced.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x004378e0
void GeneralStore_TickCustomers(ShopElem* elem)
{
    ShopDef*      def = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           tx;
    int           ty;
    int           n;
    char          rem;

    r = def->riders;
    while (r) {
        next = r->next;
        key = &r->square;
        b = r->bloke;
        tx = key->bx + def->base_x;
        ty = key->by + def->base_y;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x = (tx << 8) - 0x80;
                b->target.y = (ty << 8) - 0x100;
                ShopGo(b);
                break;
            case 1:
                b->target.x = (tx << 8) + 0x80;
                ty -= 2;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 2:
                b->target.x = (tx << 8) + 0x80;
                ty -= 3;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 3:
                rem = (char)(rand() % 2);
                if (rem) {
                    b->target.x = (tx << 8) - 0x100;
                    b->target.y = (ty << 8) - 0x280;
                    ShopMove(b);
                    n = rand() % 50;
                    b->action++;
                    b->timer = n;
                } else {
                    b->action = 6;
                }
                break;
            case 4:
                if (--b->timer == 0) {
                    b->action++;
                } else if (b->timer % 10 == 0) {
                    b->dir++;
                    if (b->dir > 8)
                        b->dir = 0;
                }
                break;
            case 5:
                b->target.x = (tx << 8) + 0x80;
                ty -= 3;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 6:
                b->action++;
                break;
            case 7:
                Shop_BrowseAndBuy(r, elem, key, 1);
                break;
            case 8:
                b->target.x = (tx << 8) + 0x80;
                ty -= 2;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 9:
                b->target.x = (tx << 8) - 0x80;
                b->target.y = (ty << 8) - 0x100;
                ShopGo(b);
                break;
            case 10:
                b->target.x = (tx << 8) + 0x80;
                b->target.y = (ty << 8) + 0x80;
                ShopGo(b);
                break;
            case 11:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8u;
                break;
            }
        }
        r = next;
    }
}


/* =========================================================================
 * 0x00439950 -- LEGO SHOP 2, +0xa8: the 13-state customer script.
 *
 * Two of the thirteen slots (6 and 7) have NO body at all -- their jump-table
 * entries point straight at the loop's continue label -- so a customer that
 * lands on them is frozen for good. Reproduced.
 *
 *   0  step to (x-0.5, y) and mark the customer "inside"
 *   1  step to (x-1, y-0.5)
 *   2  step to (x-1.375, y-0.375)
 *   3  walk to one of three spots (rand() % 3): (x-3, y), (x-1, y-2) or
 *      (x-3, y-2.5)
 *   4  browse and maybe buy (Shop_BrowseAndBuy, price index 1)
 *   5  TELEPORT to one of the same three spots: the first two also set a
 *      rand() % 50 wait and jump to state 8, the third jumps to state 9
 *      with no wait at all
 *   6,7 dead slots -- no code
 *   8  count the wait down; advance when it hits exactly 0
 *   9  step to (x-1, y-0.5)   [the same waypoint as state 1; VC6 emits one
 *      block for both]
 *  10  step to (x-0.5, y)
 *  11  step to (x, y) -- the doorway -- and FALL THROUGH into 12, so the
 *      last walk and the departure happen on the same tick and state 12 is
 *      never reached on its own. Reproduced.
 *  12  leave: RemoveBlokeFromRide and clear the "inside" flag
 * ========================================================================= */
/* 219 emitted against 218, and every case body is instruction-for-instruction
 * the original's -- but with ebp and edi exchanged (the original keeps the
 * base-relative x in ebp and y in edi; ours the other way round), and with the
 * cross-jumps between the three "store y, push the five arguments" blocks
 * regrouped: the original keeps case 3's arm-0 copy as the canonical one and
 * makes case 0 jump FORWARD into it, ours keeps case 0's and makes the arms
 * jump back, which costs arm 1 one extra `shl` because its jump lands one
 * instruction later. Measured and rejected for the register exchange: every
 * order and split of the two `key + base` sums, both operand orders, five
 * declaration orders, `b` before `key`, storing target.y before target.x in
 * case 0, and expanding the move tail as a macro instead of a static
 * __inline. */
// WIP-FUNCTION: LEGOLAND 0x00439950  (219 emitted vs 218: ebp/edi exchanged and one cross-jump regrouped -- see above)
void LegoShop2_TickCustomers(ShopElem* elem)
{
    ShopDef*      def = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           tx;
    int           ty;
    int           n;
    int           m;

    r = def->riders;
    while (r) {
        next = r->next;
        key = &r->square;
        b = r->bloke;
        tx = key->bx + def->base_x;
        ty = key->by + def->base_y;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x = (tx << 8) - 0x80;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 1:
                b->target.x = (tx << 8) - 0x100;
                b->target.y = (ty << 8) - 0x80;
                ShopGo(b);
                break;
            case 2:
                b->target.x = (tx << 8) - 0x160;
                b->target.y = (ty << 8) - 0x60;
                ShopGo(b);
                break;
            case 3:
                n = rand() % 3;
                if (n == 2) {
                    tx -= 3;
                    b->target.x = tx << 8;
                    b->target.y = ty << 8;
                } else if (n == 1) {
                    b->target.x = (tx << 8) - 0x100;
                    ty -= 2;
                    b->target.y = ty << 8;
                } else {
                    tx -= 3;
                    b->target.x = tx << 8;
                    b->target.y = (ty << 8) - 0x280;
                }
                ShopGo(b);
                break;
            case 4:
                Shop_BrowseAndBuy(r, elem, key, 1);
                break;
            case 5:
                n = rand() % 3;
                if (n == 2) {
                    tx -= 3;
                    b->world.x = tx << 8;
                    b->world.y = ty << 8;
                    m = rand() % 50;
                    b->action = 8;
                    b->timer = m;
                } else if (n == 1) {
                    b->world.x = (tx << 8) - 0x100;
                    ty -= 2;
                    b->world.y = ty << 8;
                    m = rand() % 50;
                    b->action = 8;
                    b->timer = m;
                } else {
                    tx -= 3;
                    b->action = 9;
                    b->world.x = tx << 8;
                    b->world.y = (ty << 8) - 0x280;
                }
                break;
            case 8:
                if (--b->timer == 0)
                    b->action++;
                break;
            case 9:
                b->target.x = (tx << 8) - 0x100;
                b->target.y = (ty << 8) - 0x80;
                ShopGo(b);
                break;
            case 10:
                b->target.x = (tx << 8) - 0x80;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 11:
                b->target.x = tx << 8;
                b->target.y = ty << 8;
                ShopGo(b);
                /* falls through into case 12 -- the original has no break */
            case 12:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8u;
                break;
            }
        }
        r = next;
    }
}


/* =========================================================================
 * 0x00438f10 -- SALOON, +0xa8: the 10-state customer script.
 *
 * The saloon is the only western-town building that picks its spot at the bar
 * with a coin toss, and the only one besides SHERIFF that forces a facing
 * direction: state 3 sets Bloke+0x72 to 8 so the drinker turns to face the
 * bar whichever end they walked to.
 *
 *   0  step to (x-0.5, y) and mark the customer "inside"
 *   1  step to (x-2, y-1)
 *   2  step to (x-4, y-0.5)
 *   3  toss a coin (rand() % 2): step to (x-3.5, y+1) or (x-3.5, y-2) --
 *      the two ends of the bar -- and face direction 8
 *   4  browse and maybe buy (Shop_BrowseAndBuy, price index 1)
 *   5  step back to (x-4, y-0.5)
 *   6  step to (x-2, y-1)
 *   7  step to (x-0.5, y)
 *   8  step to (x, y) -- the doorway
 *   9  leave: RemoveBlokeFromRide and clear the "inside" flag
 * ========================================================================= */
/* 240 emitted against 249. The prologue, the whole loop head (index-for-index,
 * including the two separate re-reads of the ObjDef from its stack home and the
 * hoisted `mov ebx,7`), the jump table, cases 2, 3 (both arms and the dir=8
 * tail), 4 and 9 are exact. The residual is entirely VC6's cross-jumping
 * between the five "push the five CalcMoveLine arguments" blocks: the original
 * gives cases 0 and 2 their own push blocks that jump to ONE shared call, lets
 * 1, 5 and 8 share a block that RELOADS target.x from memory, and gives 6 and
 * 7 a private call and post-call tail each; ours makes case 1 the canonical
 * copy and folds case 0 into case 7's. Measured and rejected: all 64
 * combinations of target.x-before-target.y vs target.y-before-target.x across
 * cases 0,1,2,5,6,7 (best 245, same first divergence), passing the waypoint
 * through the move helper's parameters instead of storing it first, and
 * assigning the shifted coordinates to locals before storing them. */
// WIP-FUNCTION: LEGOLAND 0x00438f10  (240 emitted vs 249: the push blocks are cross-jumped differently -- see above)
void Saloon_TickCustomers(ShopElem* elem)
{
    ShopDef*      def = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    int           tx;
    int           ty;
    char          rem;

    r = def->riders;
    while (r) {
        next = r->next;
        key = &r->square;
        b = r->bloke;
        tx = key->bx + def->base_x;
        ty = key->by + def->base_y;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x = (tx << 8) - 0x80;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 1:
                tx -= 2;
                b->target.x = tx << 8;
                b->target.y = (ty << 8) - 0x100;
                ShopGo(b);
                break;
            case 2:
                tx -= 4;
                b->target.x = tx << 8;
                b->target.y = (ty << 8) - 0x80;
                ShopGo(b);
                break;
            case 3:
                rem = (char)(rand() % 2);
                if (rem) {
                    b->target.x = (tx << 8) - 0x380;
                    ty++;
                    b->target.y = ty << 8;
                } else {
                    b->target.x = (tx << 8) - 0x380;
                    ty -= 2;
                    b->target.y = ty << 8;
                }
                ShopGo(b);
                b->dir = 8;
                break;
            case 4:
                Shop_BrowseAndBuy(r, elem, key, 1);
                break;
            case 5:
                tx -= 4;
                b->target.x = tx << 8;
                b->target.y = (ty << 8) - 0x80;
                ShopGo(b);
                break;
            case 6:
                tx -= 2;
                b->target.x = tx << 8;
                b->target.y = (ty << 8) - 0x100;
                ShopGo(b);
                break;
            case 7:
                b->target.x = (tx << 8) - 0x80;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 8:
                b->target.x = tx << 8;
                b->target.y = ty << 8;
                ShopGo(b);
                break;
            case 9:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8u;
                break;
            }
        }
        r = next;
    }
}

typedef struct JailCellRec {
    struct JailCellRec* next;    /* +0x00 */
    unsigned short      tile;    /* +0x04 */
    char                frame;   /* +0x06 */
    unsigned char       pad07;
    int                 f08;     /* +0x08 */
    int                 f0c;     /* +0x0c */
    int                 f10;     /* +0x10 */
    int                 f14;     /* +0x14 */
    int                 f18;     /* +0x18 */
} JailCellRec;

extern JailCellRec* JailCell_FindRecord(MapSquare* sq);   /* 0x00437f90 (westtown.c) */
extern JailCellRec* g_jailcells_head;                     /* 0x0062fd3c (ridesave.c) */


/* =========================================================================
 * 0x00438430 -- JAIL CELL, +0xa8: the 11-state customer script AND the
 * per-cell door animation, in one function.
 *
 * This is the only western-town tick that touches the class's own save
 * record, and it explains what the six fields in that 0x1c-byte record are
 * for (ridesave.c writes them all to the .sav):
 *
 *   +0x06 frame  the cell door's layer-1 LLS frame, 0 (open) .. 9 (shut).
 *                JailCell_LoadResources starts it at 9 and
 *                JailCell_DrawOverlay feeds it to LLSSetFrame.
 *   +0x08 taken  a visitor is using this cell
 *   +0x0c closing  the door is swinging shut (frame counting UP to 9)
 *   +0x10 shut     the door has reached 9
 *   +0x14 opening  the door is swinging open (frame counting DOWN to 0)
 *   +0x18 open     the door has reached 0
 *
 * The rider loop copies the whole record into locals, runs one step, and
 * writes all six fields back -- so a customer both reads and drives the door
 * state. The SECOND loop then walks EVERY jail-cell record in the park and
 * advances whichever door is in motion:
 *
 *   if (opening && --frame == 0) { open = 1; opening = 0; }
 *   if (closing && ++frame == 9) { shut = 1; closing = 0; }
 *
 * Note both tests run on the same record in the same tick, so a cell that is
 * somehow marked opening AND closing has its frame decremented and then
 * incremented -- a no-op. Reproduced.
 *
 * The customer script:
 *   0  mark "inside"; if the cell is free, claim it (taken = 1) and walk to
 *      (x-2, y-1) -- inside the cell; if it is already taken, stop at
 *      (x-0.5, y) facing direction 7. Either way wait rand() % 100 ticks.
 *   1  start the door OPENING (opening = 1), face direction 3, advance
 *   2  wait for `open`; then count the wait down. On zero, start the door
 *      CLOSING (closing = 1), clear `open`, advance. While waiting, every
 *      tenth tick face direction 2 or 4 at random -- the prisoner shifting
 *      about behind the bars.
 *   3  wait for `shut`, then advance
 *   4  walk to (x-1, y-0.5), release the cell (taken = 0), jump to state 8
 *   5,6 dead slots -- no code
 *   7  count the wait down; advance when it hits exactly 0
 *   8  walk to (x+0.5, y+0.5) -- the doorway -- and jump straight to state 10
 *   9  dead slot
 *  10  leave: RemoveBlokeFromRide and clear the "inside" flag
 *
 * A rider whose square has NO jail-cell record aborts the WHOLE function
 * with a `return`, so the door animation does not run that frame either.
 * Reproduced.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x00438430
void JailCell_TickCustomers(ShopElem* elem)
{
    ShopDef*      def = elem->data;
    RiderNode*    r;
    RiderNode*    next;
    Bloke*        b;
    MapSquare*    key;
    JailCellRec*  rec;
    unsigned char frame;
    int           f08;
    int           f0c;
    int           f10;
    int           f14;
    int           f18;
    int           tx;
    int           ty;
    int           n;

    r = def->riders;
    while (r) {
        next = r->next;
        b = r->bloke;
        key = &r->square;
        rec = JailCell_FindRecord(key);
        if (rec == 0)
            return;
        frame = rec->frame;
        f08 = rec->f08;
        f0c = rec->f0c;
        f10 = rec->f10;
        f14 = rec->f14;
        f18 = rec->f18;
        tx = key->bx + def->base_x;
        ty = key->by + def->base_y;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                if (f08 == 0) {
                    tx -= 2;
                    f08 = 1;
                    b->target.x = tx << 8;
                    b->target.y = (ty << 8) - 0x100;
                } else {
                    b->target.x = (tx << 8) - 0x80;
                    b->target.y = ty << 8;
                    b->dir = 7;
                }
                ShopMove(b);
                n = rand() % 100;
                b->action++;
                b->timer = n;
                break;
            case 1:
                b->action++;
                f14 = 1;
                b->dir = 3;
                break;
            case 2:
                if (f18 != 0) {
                    if (--b->timer == 0) {
                        f0c = 1;
                        f18 = 0;
                        b->action++;
                    } else if (b->timer % 10 == 0) {
                        b->dir = (unsigned char)(rand() % 2 ? 4 : 2);
                    }
                }
                break;
            case 3:
                if (f10 != 0)
                    b->action++;
                break;
            case 4:
                b->target.y = (ty << 8) - 0x80;
                b->target.x = (tx << 8) - 0x100;
                ShopMove(b);
                b->action = 8;
                f08 = 0;
                break;
            case 7:
                if (--b->timer == 0)
                    b->action++;
                break;
            case 8:
                b->target.x = (tx << 8) + 0x80;
                b->target.y = (ty << 8) + 0x80;
                ShopMove(b);
                b->action = 0xa;
                break;
            case 10:
                RemoveBlokeFromRide(def, r);
                b->flags62 &= ~8u;
                break;
            }
        }
        rec->frame = frame;
        rec->f08 = f08;
        rec->f0c = f0c;
        rec->f10 = f10;
        rec->f14 = f14;
        rec->f18 = f18;
        r = next;
    }
    rec = g_jailcells_head;
    while (rec) {
        f14 = rec->f14;
        frame = rec->frame;
        f08 = rec->f08;
        f0c = rec->f0c;
        f10 = rec->f10;
        f18 = rec->f18;
        if (f14 != 0) {
            if (--frame == 0) {
                f18 = 1;
                f14 = 0;
            }
        }
        if (f0c != 0) {
            if (++frame == 9) {
                f10 = 1;
                f0c = 0;
            }
        }
        rec->frame = frame;
        rec->f08 = f08;
        rec->f0c = f0c;
        rec->f10 = f10;
        rec->f14 = f14;
        rec->f18 = f18;
        rec = rec->next;
    }
}


/* ==========================================================================
 * THE TWO OVERLAY DRAWS THAT NEED MORE THAN A MATTE (+0xb0)
 *
 * westtown.c holds the other seven. These two are the ones whose sprite is
 * fetched through the LAYER path rather than blitted at the object position:
 * the object's screen position and the layer's own render offset are added
 * together, so the building's upper storey lands above the customers.
 *
 * THE BAND-LOOP SPELLING IS A LEVER, and this file is where it was found --
 * see the note above ShopDrawBand: an explicit `if (n > 0)` guard around a
 * `do { } while (--k)` over the parameter used as its own cursor makes VC6
 * rematerialise `movsx edi,bl` inside every band, which is what the original
 * does; a plain `for (i = 0; i < n; i++)` hoists ONE sign-extension for the
 * whole function and re-registers everything downstream. Applying it to
 * westtown.c's three banded draws took them from 196/161/96 mismatches to
 * 27/22/16.
 * ========================================================================== */

typedef struct Offset { int ox; int oy; } Offset;

/* The placement square read as ONE word: the collect loops compare the packed
 * {x,y} pair, not the two bytes. */
typedef union ShopTile {
    unsigned short key;
    MapSquare      sq;
} ShopTile;

extern void   IP_RenderBlokeIn3DNow(Bloke* b);                         /* 0x00440010 */
extern void*  GetLLSForLayer(void* obj, int layer);                    /* 0x00441ea0 */
extern void*  GetSpriteForLayer(void* obj, int layer);                 /* 0x00441ec0 */
extern Offset GetRenderOffsetForLayer(void* obj, int layer);           /* 0x00441ee0 */
extern Offset GetScreenCoordsForObject(MapSquare* sq, ShopDef* def);   /* 0x00442cc0 */
extern void   AdjustOffsetForViewMode(Offset* o);                      /* 0x00442d30 */
extern void   LLSSetFrame(void* lls, int frame);                       /* 0x0047d5a0 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern void*  g_jail_sprite;                                           /* 0x0062fd40 */
extern void*  g_jail_matte;                                            /* 0x0081cb0c  "JailCellMask.LLS" */
extern void*  g_legoshop2_matte;                                       /* 0x0081cb20  "Lego Shop 2 Matte.LLS" */

/* Draw every collected customer whose action byte is `band`. The `if (n > 0)`
 * guard plus the do/while over the PARAMETER (no named cursor local -- a
 * named one costs a register and spills the count to the frame) is what makes
 * VC6 emit the original's per-band `test bl,bl / jle / lea esi,queue /
 * movsx edi,bl` instead of hoisting one sign-extension. */
static __inline void ShopDrawBand(Bloke** list, char n, int band)
{
    int k;

    if (n > 0) {
        k = n;
        do {
            if ((*list)->action == band)
                IP_RenderBlokeIn3DNow(*list);
            list++;
        } while (--k);
    }
}

/* THE ONE RESIDUAL IN BOTH DRAWS BELOW, measured precisely: a two-instruction
 * TRANSPOSITION repeated once per band. The original emits
 *     test bl,bl / jle <next band> / lea esi,queue / movsx edi,bl
 * and ours emits
 *     test bl,bl / lea esi,queue / jle <next band> / movsx edi,bl
 * -- VC6 speculates the queue-address `lea` up into the slot between the
 * compare and its branch. The instruction count, the byte length of every
 * block, the band order, the frame layout and both sprite paths are exact;
 * only that one pair is swapped, 9-10 times per function. Measured and
 * rejected: a named cursor local assigned inside the guard (spills the count
 * to the frame, 219 mismatches), the same as a macro, the guard moved to the
 * call site, `while (n--)` and `while (k > 0)` loop forms, a pointer-pair
 * (`p != end`) loop, an early `return` guard, `int k = n` before the guard,
 * and a void* parameter cast inside the guard.
 * ========================================================================== */

/* =========================================================================
 * 0x00439760 -- LEGO SHOP 2, +0xb0: nine occlusion bands and one matte.
 *
 * Bands 4,5,3,2,9,10,11 are drawn first, then the shop front, then bands 1
 * and 12 in front of it. The matte is positioned the LAYER way -- the
 * object's screen position plus the render offset of layer 0 of the class's
 * own sprite -- which is why this draw is not one of westtown.c's simple
 * ones.
 * ========================================================================= */
// WIP-FUNCTION: LEGOLAND 0x00439760  (180 of 180 instructions; one lea/jle transposition per band -- see above)
void LegoShop2_DrawOverlay(ShopElem* elem, int x, int y, MapSquare* sq,
                           void* clip, int mode)
{
    ShopDef*   def = elem->data;
    char       n = 0;
    RiderNode* r = def->riders;
    Bloke*     queue[10] = { 0 };
    Offset     off;
    Offset     screen;

    while (r) {
        if (((ShopTile*)sq)->key == ((ShopTile*)&r->square)->key)
            queue[n++] = r->bloke;
        r = r->next;
    }
    if (n) {
    ShopDrawBand(queue, n, 4);
    ShopDrawBand(queue, n, 5);
    ShopDrawBand(queue, n, 3);
    ShopDrawBand(queue, n, 2);
    ShopDrawBand(queue, n, 9);
    ShopDrawBand(queue, n, 10);
    ShopDrawBand(queue, n, 11);
    off = GetRenderOffsetForLayer(def->sprite, 0);
    screen = GetScreenCoordsForObject(sq, def);
    AdjustOffsetForViewMode(&off);
    PrintSprite(g_legoshop2_matte, off.ox + screen.ox, off.oy + screen.oy, mode, 0);
    ShopDrawBand(queue, n, 1);
    ShopDrawBand(queue, n, 12);
    }
}

/* =========================================================================
 * 0x00438150 -- JAIL CELL, +0xb0: the only overlay draw that needs the
 * class's own save record.
 *
 * It looks the record up FIRST and draws nothing at all if this square has
 * none. Then, with customers present:
 *   bands 2 and 3            (behind the bars)
 *   the CELL DOOR: layer 1 of the jail sprite, its LLS frame set from the
 *                  record's +0x06 byte -- this is what makes the saved
 *                  jail-cell byte visible, and why JAIL CELL is the only
 *                  western-town class with a save chunk
 *   bands 1, 4 and 8
 *   the cell mask (JailCellMask.LLS) at layer 0's offset
 *   bands 0, 7 and 10
 * With no customers on the square the whole thing collapses to just the
 * door draw -- the same block, tail-duplicated by VC6.
 * ========================================================================= */
// WIP-FUNCTION: LEGOLAND 0x00438150  (256 of 256 instructions; one lea/jle transposition per band -- see above)
void JailCell_DrawOverlay(ShopElem* elem, int x, int y, MapSquare* sq,
                          void* clip, int mode)
{
    ShopDef*     def = elem->data;
    char         n = 0;
    RiderNode*   r = def->riders;
    Bloke*       queue[10] = { 0 };
    Offset       screen;
    Offset       off;
    JailCellRec* rec;

    screen = GetScreenCoordsForObject(sq, def);
    rec = JailCell_FindRecord(sq);
    if (rec == 0)
        return;
    while (r) {
        if (((ShopTile*)sq)->key == ((ShopTile*)&r->square)->key)
            queue[n++] = r->bloke;
        r = r->next;
    }
    if (n) {
        ShopDrawBand(queue, n, 2);
        ShopDrawBand(queue, n, 3);
        LLSSetFrame(GetLLSForLayer(g_jail_sprite, 1), rec->frame);
        off = GetRenderOffsetForLayer(g_jail_sprite, 1);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(g_jail_sprite, 1),
                    screen.ox + off.ox, screen.oy + off.oy, mode, 0);
        ShopDrawBand(queue, n, 1);
        ShopDrawBand(queue, n, 4);
        ShopDrawBand(queue, n, 8);
        off = GetRenderOffsetForLayer(g_jail_sprite, 0);
        AdjustOffsetForViewMode(&off);
        PrintSprite(g_jail_matte, screen.ox + off.ox, screen.oy + off.oy, mode, 0);
        ShopDrawBand(queue, n, 0);
        ShopDrawBand(queue, n, 7);
        ShopDrawBand(queue, n, 10);
    } else {
        LLSSetFrame(GetLLSForLayer(g_jail_sprite, 1), rec->frame);
        off = GetRenderOffsetForLayer(g_jail_sprite, 1);
        AdjustOffsetForViewMode(&off);
        PrintSprite(GetSpriteForLayer(g_jail_sprite, 1),
                    screen.ox + off.ox, screen.oy + off.oy, mode, 0);
    }
}
