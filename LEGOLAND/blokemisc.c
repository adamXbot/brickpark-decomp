/* LEGOLAND — bloke / worker small functions.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; the field and type names are ours.
 * Each matched function carries its original VA marker on the line above it. */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* A "bloke" (worker/person) record. The allocation stride is 172 (0xac) bytes
 * — see GetBlokeNum/GetBlokePtr in sweep3.c. Only the fields these functions
 * touch are named; the rest is padding to hold the offsets. */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00  intrusive list link (Control3DPeople) */
    void*          model;       /* +0x04  3D model / render object */
    unsigned char  pad08[6];    /* +0x08..0x0d */
    unsigned short state;       /* +0x0e  low-level AI state (DoLowLevelAI) */
    unsigned char  pad10[0x50]; /* +0x10..0x5f */
    unsigned char  action;      /* +0x60 */
    unsigned char  pad61;       /* +0x61 */
    unsigned char  flags62;     /* +0x62  bit 0x80 = suppress update */
    unsigned char  pad63[0x49]; /* +0x63..0xab  (total 172 bytes) */
} Bloke;

/* ---------------------------------------------------------------- globals -- */

/* Head of the 3D-people list (@ 0x0066b574). */
extern Bloke* g_people_head;
/* The worker currently attached to the mouse cursor (@ 0x007fdff0). */
extern Bloke* g_worker_on_mouse;
/* Low-level AI dispatch table, indexed by Bloke::state (@ 0x004bd34c). */
extern void (*g_lowlevel_ai[])(Bloke*);

/* ------------------------------------------------------------ prototypes -- */

extern void  AddBricks(int n);                  /* 0x004578a0 */
extern void  BlokeSetAnim(Bloke* b, int anim);  /* 0x004406c0 */
extern void  RenderBlokeIn3D(Bloke* b);         /* 0x0043ffb0 */
extern void  SetPathFlag(Bloke* b);             /* 0x004831d0 */
extern int   RunGardenerJob(Bloke* b);          /* 0x00499d00 (unexported) */
extern int   RunMechanicJob(Bloke* b);          /* 0x00499d30 (unexported) */
extern void  UpdatePersonPos(void* model, Bloke* b); /* 0x004401b0 (unexported) */

/* -------------------------------------------------------------- functions -- */

/* Hiring a gardener costs 30 bricks; firing one refunds them. */
// FUNCTION: LEGOLAND 0x0049a150
void RefundGardener(void)
{
    AddBricks(30);
}

// FUNCTION: LEGOLAND 0x0049a190
void RefundMechanic(void)
{
    AddBricks(30);
}

// FUNCTION: LEGOLAND 0x0049a480
void Gardener_Idle(Bloke* b)
{
    b->state = 14;
    RunGardenerJob(b);
}

// FUNCTION: LEGOLAND 0x0049a4b0
void Mechanic_Idle(Bloke* b)
{
    b->state = 14;
    RunMechanicJob(b);
}

// FUNCTION: LEGOLAND 0x00440780
void BlokeSitAnim(Bloke* b)
{
    BlokeSetAnim(b, 0);
}

// FUNCTION: LEGOLAND 0x00440960
void BlokeWalkWithPan(Bloke* b)
{
    BlokeSetAnim(b, 5);
}

// FUNCTION: LEGOLAND 0x00440290
void UpdatePerson(Bloke* b)
{
    if (!(b->flags62 & 0x80) && b->model)
        UpdatePersonPos(b->model, b);
}

// FUNCTION: LEGOLAND 0x004402b0
void Control3DPeople(void)
{
    Bloke* p = g_people_head;
    while (p) {
        UpdatePerson(p);
        p = p->next;
    }
}

// FUNCTION: LEGOLAND 0x00484920
void DoLowLevelAI(Bloke* b)
{
    SetPathFlag(b);
    g_lowlevel_ai[b->state](b);
}

// FUNCTION: LEGOLAND 0x004708c0
void RenderWorkerOnMouse(void)
{
    RenderBlokeIn3D(g_worker_on_mouse);
}
