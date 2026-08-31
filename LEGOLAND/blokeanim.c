/* LEGOLAND — bloke 3D animation frame control.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; the field and type names are ours.
 * Each matched function carries its original VA marker on the line above it. */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* The resource an animation was loaded from; BlokeSetAnim dereferences
 * owner->handle to get the model/skeleton it hands to MakeAnimInstance. */
typedef struct AnimOwner {
    unsigned char pad00[0x20];  /* +0x00..0x1f */
    void**        handle;       /* +0x20  -> the loaded model/skeleton */
} AnimOwner;

/* One animation of the 3D character system. The frame count (+0x00) is what
 * BlokeSetFrame/BlokeAnimNextFrame reduce a frame index modulo, and what
 * PlayBlokeAnim compares against. */
typedef struct Anim3D {
    int         frames;         /* +0x00  number of frames in this animation */
    AnimOwner*  owner;          /* +0x04 */
    void*       data;           /* +0x08  per-animation payload */
} Anim3D;

/* The renderable "3D person" hanging off a bloke (Bloke::person, +0x04).
 * Field names are ours; the offsets come from the disassembly. */
typedef struct Person3D {
    unsigned char pad00[8];     /* +0x00..0x07 */
    int           kind;         /* +0x08  character kind: 1, 2, 3 select the
                                 *        animation table (see GetBlokeAnim3D) */
    unsigned char pad0c[0x40];  /* +0x0c..0x4b */
    int           frame;        /* +0x4c  current frame within the animation */
    void*         inst;         /* +0x50  live animation instance (BlokeSetAnim) */
    unsigned char pad54[0x30];  /* +0x54..0x83 */
    int           variant;      /* +0x84  sub-kind for kind 1 (sex) */
    int           anim;         /* +0x88  index of the current animation */
} Person3D;

/* A bloke; only the link to its 3D person matters here. Allocation stride is
 * 172 (0xac) bytes — see GetBlokeNum/GetBlokePtr in sweep3.c. */
typedef struct Bloke {
    void*      pad00;           /* +0x00 */
    Person3D*  person;          /* +0x04 */
} Bloke;

/* ---------------------------------------------------------------- globals -- */

/* Four animation tables, laid out back to back in zero-fill data (nothing is
 * stored for them in the image; they are populated by the loader). Each is an
 * array of Anim3D* indexed by Person3D::anim. The entry counts below are
 * inferred from the gaps between the four base addresses.
 *   0x0062feb0  kind 2                     (3 entries) */
extern Anim3D* g_anim_kind2[];
/*   0x0062febc  kind 1, variant 0          (6 entries) */
extern Anim3D* g_anim_kind1a[];
/*   0x0062fed4  kind 1, variant non-zero   (8 entries) */
extern Anim3D* g_anim_kind1b[];
/*   0x0062fef4  kind 3 */
extern Anim3D* g_anim_kind3[];

/* -------------------------------------------------------------- functions -- */

/* Pick the current animation for a bloke's 3D person.
 *
 * Note the missing `default:` — when Person3D::kind is not 1, 2 or 3 the local
 * `table` is never written, and VC6 reads the (unallocated) slot it assigned to
 * it, which with /Gy and no stack frame aliases the incoming first parameter.
 * That is exactly the `mov eax, [esp+4]` in the original's fall-through arm. */
// FUNCTION: LEGOLAND 0x00440790
Anim3D* GetBlokeAnim3D(Bloke* b)
{
    Anim3D** table;
    Anim3D* anim = 0;
    Person3D* p = b->person;

    if (p) {
        switch (p->kind) {
        case 1:
            if (p->variant == 0)
                table = g_anim_kind1a;
            else
                table = g_anim_kind1b;
            break;
        case 2:
            table = g_anim_kind2;
            break;
        case 3:
            table = g_anim_kind3;
            break;
        }
        anim = table[p->anim];
    }
    return anim;
}

/* Set the 3D person's frame within its current animation, wrapping into range.
 * Signed remainder — the original uses cdq/idiv, so `frames` is a signed int. */
// FUNCTION: LEGOLAND 0x00440870
void BlokeSetFrame(Bloke* b, int frame)
{
    Person3D* p = b->person;

    if (p)
        p->frame = frame % GetBlokeAnim3D(b)->frames;
}

/* Advance one frame, wrapping back to 0 at the end of the animation. */
// FUNCTION: LEGOLAND 0x004408e0
void BlokeAnimNextFrame(Bloke* b)
{
    Person3D* p = b->person;

    if (p)
        p->frame = (p->frame + 1) % GetBlokeAnim3D(b)->frames;
}

/* Same table selection as GetBlokeAnim3D, but starting from the 3D person
 * rather than from the bloke that owns it. */
// FUNCTION: LEGOLAND 0x00440800
Anim3D* GetBlokeAnim3DFromPerson(Person3D* p)
{
    Anim3D** table;
    Anim3D* anim = 0;

    if (p) {
        switch (p->kind) {
        case 1:
            if (p->variant == 0)
                table = g_anim_kind1a;
            else
                table = g_anim_kind1b;
            break;
        case 2:
            table = g_anim_kind2;
            break;
        case 3:
            table = g_anim_kind3;
            break;
        }
        anim = table[p->anim];
    }
    return anim;
}

/* Advance a non-looping playback: step one frame and report completion.
 * Returns 1 (and rewinds to frame 0) on the tick the animation runs out. */
// FUNCTION: LEGOLAND 0x004408a0
int PlayBlokeAnim(Bloke* b)
{
    Person3D* p = b->person;

    if (p) {
        Anim3D* anim = GetBlokeAnim3DFromPerson(p);
        p->frame++;
        if (p->frame >= anim->frames) {
            p->frame = 0;
            return 1;
        }
    }
    return 0;
}

/* Per-kind animation-instance context, three consecutive pointers. Declared in
 * address order: kind 1 @ 0x0081c8c0, kind 3 @ 0x0081c8c4, kind 2 @ 0x0081c8c8. */
extern void* g_anim_ctx_kind1;
extern void* g_anim_ctx_kind3;
extern void* g_anim_ctx_kind2;

extern void  ReleaseAnimInstance(void* inst);                           /* 0x0049e4d0 */
extern void* MakeAnimInstance(Person3D* p, void* ctx, void* data,
                              void* model, int variant);                /* 0x00442580 */

/* Switch a bloke to animation `anim`, rebuilding its playback instance.
 * No-op when it is already playing that animation. Same missing `default:`
 * on both switches as GetBlokeAnim3D. */
// FUNCTION: LEGOLAND 0x004406c0
void BlokeSetAnim(Bloke* b, int anim)
{
    Anim3D** table;
    void* ctx;
    Anim3D* a;
    Person3D* p = b->person;

    if (p->anim == anim)
        return;
    p->anim = anim;

    switch (p->kind) {
    case 1:
        if (p->variant == 0)
            table = g_anim_kind1a;
        else
            table = g_anim_kind1b;
        break;
    case 2:
        table = g_anim_kind2;
        break;
    case 3:
        table = g_anim_kind3;
        break;
    }
    a = table[anim];

    if (p->kind == 1 && p->inst)
        ReleaseAnimInstance(p->inst);

    switch (p->kind) {
    case 1:
        ctx = g_anim_ctx_kind1;
        break;
    case 2:
        ctx = g_anim_ctx_kind2;
        break;
    case 3:
        ctx = g_anim_ctx_kind3;
        break;
    }
    {
        void* data = a->data;
        AnimOwner* o = a->owner;
        p->inst = MakeAnimInstance(p, ctx, data, *o->handle, p->variant);
    }
}

/* Walk: animation 0 for kinds 2 and 3, animation 1 for everything else, and
 * restart from frame 0. VC6 tail-duplicates the whole call pair into both arms,
 * which is why the constant is materialised in a register before the push. */
// FUNCTION: LEGOLAND 0x00440910
void BlokeWalkAnim(Bloke* b)
{
    int anim;

    switch (b->person->kind) {
    case 2:
        anim = 0;
        break;
    case 3:
        anim = 0;
        break;
    default:
        anim = 1;
        break;
    }
    BlokeSetAnim(b, anim);
    BlokeSetFrame(b, 0);
}

/* "Walk carrying a pan" pose. */
// FUNCTION: LEGOLAND 0x00440970
void BlokePanWithPan(Bloke* b)
{
    BlokeSetAnim(b, 4);
}
