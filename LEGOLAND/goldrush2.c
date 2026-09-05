/* ---------------------------------------------------------------------------
 * goldrush2.c -- gold rush people
 *
 * Two functions from the gold-rush frontier, both reached from goldrush.c:
 *
 *   SetPerson3DHeading (0x004025d0) -- the SIXTEEN-way sibling of math3d.c's
 *   already-matched SetPersonDirection (0x004400b0).  Both write the person's
 *   rotation vector at Person3D +0x40..+0x48 (x, y, z as three floats) and
 *   hand it straight to SetPersonRotation, which builds the 16.16 matrix at
 *   +0x58.  x and z are always zero; the heading only ever turns the bloke
 *   about y.  Where SetPersonDirection quantises a heading to 8 compass
 *   points (pi/4 apart), this one quantises to 16 (pi/8 apart) -- the school
 *   car's driver, whose `dir` field runs 0..15.
 *
 *   Fort_StepVisitor (0x004064d0) -- the in-fort behaviour a gold-rush rider
 *   runs once StepGoldRushRider hands it off.
 *
 * Mechanics recovered by this file are in docs/lanes/fable-a-goldrush2.md.
 * --------------------------------------------------------------------------- */

#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* Three floats; the rotation vector the 3D people carry at +0x40.  Named the
 * way math3d.c and anim2.c name it so SetPersonRotation's second parameter
 * keeps the same shape. */
typedef struct Vec3 {
    float x;   /* +0x00 */
    float y;   /* +0x04 */
    float z;   /* +0x08 */
} Vec3;

/* The 3D person record (blokeai.c's 0x94-byte record); only the part this
 * file touches.  +0x40 is the rotation vector, +0x58 the matrix that
 * SetPersonRotation fills from it. */
typedef struct Person3D {
    unsigned char pad00[0x40];
    Vec3          rot;       /* +0x40  rotation vector, radians */
    int           f4c;       /* +0x4c */
} Person3D;

extern void SetPersonRotation(Person3D* p, Vec3* rot);               /* 0x00440020 */

/* -------------------------------------------------------- SetPerson3DHeading */

/* Quantised heading -> y rotation, 16 compass points pi/8 apart.  The angles
 * are single-precision literals, not (float)(k*PI/8) folds: the original
 * stores the bit patterns directly (pi is 0x40490fd7 = 3.14159179f, four ulps
 * below (float)3.1415926), exactly as SetPersonDirection does.
 *
 * Read as multiples of pi/8 the table is -2, -3, 12, 11, 10, 9, 8, 7, 6, 5,
 * 4, 3, 2, 1, 0, -1 -- i.e. a plain descending sweep from case 2 down to case
 * 15 that wraps, with cases 0 and 1 sitting one turn below.  That is a
 * screen-space heading (0 = down-right on the isometric grid), not a maths
 * angle, and it is the same convention SetPersonDirection uses at half the
 * resolution: its case 0 is likewise -pi/4 and its case 1 is 3*pi/2.
 *
 * There is no `default:` in the source -- an out-of-range heading leaves
 * rot.y at whatever it was and still rebuilds the matrix.  VC6 duplicates the
 * SetPersonRotation tail into all sixteen case blocks and lets case 15's
 * store fall into the shared copy that the default path also enters. */
// FUNCTION: LEGOLAND 0x004025d0
void SetPerson3DHeading(Person3D* p, unsigned int dir)
{
    p->rot.x = 0.0f;
    p->rot.z = 0.0f;
    switch (dir) {
    case  0: p->rot.y = -0.785397947f; break;
    case  1: p->rot.y = -1.17809689f;  break;
    case  2: p->rot.y = 4.71238756f;   break;
    case  3: p->rot.y = 4.3196888f;    break;
    case  4: p->rot.y = 3.92698979f;   break;
    case  5: p->rot.y = 3.53429079f;   break;
    case  6: p->rot.y = 3.14159179f;   break;
    case  7: p->rot.y = 2.74889278f;   break;
    case  8: p->rot.y = 2.35619378f;   break;
    case  9: p->rot.y = 1.9634949f;    break;
    case 10: p->rot.y = 1.57079589f;   break;
    case 11: p->rot.y = 1.17809689f;   break;
    case 12: p->rot.y = 0.785397947f;  break;
    case 13: p->rot.y = 0.392698973f;  break;
    case 14: p->rot.y = 0.0f;          break;
    case 15: p->rot.y = -0.392698973f; break;
    }
    SetPersonRotation(p, &p->rot);
}

/* ------------------------------------------------------- Fort_StepVisitor -- */

/* The rider list node and the packed map square it carries, as goldrush.c
 * models them: RiderNode +0x0c is the placement's {x,y} as two bytes. */
typedef struct MapSquare {
    unsigned char bx;            /* +0x00 */
    unsigned char by;            /* +0x01 */
} MapSquare;

typedef struct Bloke Bloke;

typedef struct RiderNode {
    struct RiderNode* next;      /* +0x00 */
    struct RiderNode* prev;      /* +0x04 */
    Bloke*            bloke;     /* +0x08 */
    unsigned short    ride_id;   /* +0x0c  packed {x,y} of the placement */
    unsigned short    pad0e;
    void*             person;    /* +0x10 */
} RiderNode;

typedef struct Pos8 { int x, y; } Pos8;

/* The visitor bloke.  Same offsets goldrush.c uses; only the fields this
 * function touches are named. */
struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;        /* +0x0e  low-level AI state (7 = walking) */
    unsigned char  pad10[0x24 - 0x10];
    Pos8           target;       /* +0x24  walk target, 24.8 */
    unsigned char  pad2c[0x40 - 0x2c];
    short          f40;          /* +0x40  in-fort sub-state */
    short          f42;          /* +0x42  sub-state to resume with */
    unsigned char  pad44[0x58 - 0x44];
    int            wander;       /* +0x58  ticks until the next decision */
    unsigned char  pad5c[4];
    unsigned char  action;       /* +0x60  the ride's own state-machine step */
    unsigned char  pad61[0x68 - 0x61];
    Pos8           world;        /* +0x68  world position, 24.8 */
    unsigned char  pad70[2];
    unsigned char  dir;          /* +0x72  facing, 0..15 */
    unsigned char  new_dir;      /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14];   /* +0x98  CalcMoveLine scratch */
};

/* The fort's interior, in tiles relative to the placement's own square:
 * x from -1 to +3, y from -3 to +3.  Four initialised ints that are read as
 * one rectangle, so they are declared as one object. */
extern struct FortArea { int x0, y0, x1, y1; } g_fort_area;  /* 0x004b4580 */

extern int  CalcMoveLine(Pos8 from, Pos8 to, void* path);            /* 0x00480740 */
extern int  NewDirForAction(Bloke* b, unsigned char dir);            /* 0x004833d0 */
extern int  rand(void);                                              /* 0x0049e4b2 (CRT) */

/* The in-fort behaviour of a gold-rush visitor, reached from Fort_TickRiders
 * (0x00406660) once the bloke has walked in through the gate.  Bloke +0x40 is
 * a sub-state private to this ride; +0x42 is the sub-state to resume with
 * once the +0x58 timer runs out.
 *
 *   1  standing still: count +0x58 down and, when it expires, resume +0x42
 *      (which the decision step always sets to 3, so it always resumes with
 *      "decide again")
 *   2  walk to a RANDOM point inside the fort's interior rectangle, then go
 *      to sub-state 3
 *   3  decide: a 2-in-4 chance of facing a random one of the sixteen
 *      headings and standing still for 3..18 ticks, 1-in-4 of walking to a
 *      new random spot, 1-in-4 of leaving this step of the ride entirely
 *      (the rider's own action counter advances, so Fort_TickRiders moves on
 *      to walking to the centre of the placement square)
 *
 * The random point is `origin + (rand() & 255) / 255.0f * span`, in 24.8
 * fixed point; the span is pre-shifted as an INT and converted per axis, and
 * the divisor is the single-precision 1/255 at 0x004ab380.  Both random
 * fractions are drawn BEFORE either coordinate is built, which is why the
 * second one sits on the x87 stack across the whole x computation and is
 * discarded with a bare `fstp st(0)` just before the call.
 *
 * Two codegen notes, both measured:
 *  - `key` MUST be declared at the top of the function, above the switch,
 *    even though it is only used in sub-state 2.  That is what forces the
 *    eager root copy `mov edi,[esp+14h]` at index 4, and freeing both dead
 *    parameter homes that early is also what puts the pre-shifted x span in
 *    arg2's slot [esp+18h] and the fild scratch in arg1's [esp+14h].  With
 *    `key` declared inside the case the load sinks to index 44 and the two
 *    slots come out swapped -- one declaration placement, 12 mismatches.
 *  - in sub-state 3 the `rd` parameter is dead, so VC6 reuses edi (its root
 *    copy) for the roll itself, and then knows edi == 2 on the `r == 2` arm
 *    and stores the REGISTER: `mov word ptr [esi+40h],di` comes out of a
 *    plain `b->f40 = 2;`, not from assigning the variable. */
// FUNCTION: LEGOLAND 0x004064d0
void Fort_StepVisitor(RiderNode* rd, Bloke* b)
{
    MapSquare* key = (MapSquare*)&rd->ride_id;

    switch (b->f40) {
    case 1:
        if (--b->wander <= 0)
            b->f40 = b->f42;
        break;

    case 2:
        {
            int   dx = (g_fort_area.x1 - g_fort_area.x0) << 8;
            int   dy = (g_fort_area.y1 - g_fort_area.y0) << 8;
            float fx = (rand() & 255) * (1.0f / 255.0f);
            float fy = (rand() & 255) * (1.0f / 255.0f);
            unsigned char a;

            b->target.x = (int)(((g_fort_area.x0 + key->bx) << 8) + dx * fx);
            b->target.y = (int)(((g_fort_area.y0 + key->by) << 8) + dy * fy);
            a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
            b->state = 7;
            b->new_dir = a;
            NewDirForAction(b, (unsigned char)((a >> 5) + 3));
            b->f40 = 3;
        }
        break;

    case 3:
        {
            int r = rand() & 3;

            if (r == 0 || r == 1) {
                /* Source ORDER: the two constant stores must come after the
                 * timer statement.  Written before it they cannot sink past
                 * its rand() call and land adjacent; written after it VC6
                 * hoists them into the gaps of the timer's own arithmetic,
                 * which is the original's interleave. */
                b->dir = (unsigned char)(rand() & 15);
                b->wander = (rand() & 15) + 3;
                b->f40 = 1;
                b->f42 = 3;
            }
            if (r == 2) {
                b->f40 = 2;
                break;
            }
            if (r == 3)
                b->action++;
        }
        break;
    }
}
