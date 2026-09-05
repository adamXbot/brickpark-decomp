/* LEGOLAND -- the ROLLER COASTER's remaining TRACK-RUN walkers, the 3D basis
 * builder shared by the track mesh and the supports, and the seated rider
 * model builder.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere) and follow coaster.c / coaster3d.c / coaster4.c / coaster5.c.
 *
 * All five names are the ones their callers already use: TrackRunSteps,
 * TrackRunSpanEnd and TrackFitEndGeom from coaster5.c, MakeRotation from
 * coaster3d.c / schoolcar3.c, CoasterCar_BuildRider from coaster5.c. Two type
 * divergences are deliberate and local: `Mat3` is spelled as three `Vec3f`
 * rows here where coaster3d.c and schoolcar3.c spell it `float m[9]` (same
 * 0x24 layout), and coaster5.c's `BlokeApp` fields `leg`/`arm` and
 * `chest`/`face` are grouped as `colour[2]` and `part[2][0x14]` at the same
 * offsets, because both are walked as arrays here.
 */

#include <string.h>

#pragma intrinsic(memcpy)

/* ==========================================================================
 * THE TRACK RUN
 * ==========================================================================
 * coaster5.c's TrackJoinPieces is the consumer of everything in this block,
 * and between the three walkers here the whole "run" abstraction falls out:
 *
 *   A RUN is a maximal stretch of pieces between two RAISED ones. `raised`
 *   is bit 0 of the class descriptor's first dword (coaster5.c's
 *   TrackDesc.raised), and every walk here is `while (!(n->desc->raised & 1))`
 *   -- so a raised piece is an ANCHOR: a station, a lift hill top, anything
 *   whose height is fixed by the build rather than derived from its
 *   neighbours. Everything between two anchors is free to be re-profiled.
 *
 *   A JOINT is SLOPED when 0x00429910 says so, from the piece's class
 *   descriptor plus its two end DIRECTION bits (jin.dir / jout.dir). The
 *   number of sloped joints in a run is its capacity to absorb a height
 *   change: TrackJoinPieces divides the total rise by that count and lays
 *   one step per sloped joint.
 *
 * TrackJoint is {dir, h, node} -- coaster5.c corrected +0x00 from "height"
 * to a four-way DIRECTION bit, and the two functions below are what pin
 * +0x04 down as the FLOAT end HEIGHT (`fcomp dword ptr [ecx+0x24]` against
 * an `fild`ed integer). schoolcar.c's LevelTrackRunBack writes exactly that
 * field on both joints when it levels a run.
 * ======================================================================== */
typedef struct TrackNode TrackNode;

typedef struct TrackDesc {
    int           raised;       /* +0x00  bit 0 = an anchored piece */
    int           h0;           /* +0x04  the tail-end height */
    int           h1;           /* +0x08  the head-end height */
    unsigned char pad0c[0x38 - 0x0c];
} TrackDesc;                    /* 0x38 */

typedef struct TrackJoint {
    int        dir;             /* +0x00  1/2/4/8, -1 = free */
    float      h;               /* +0x04  this end's height, in world units */
    TrackNode* node;            /* +0x08  the neighbour through this end */
} TrackJoint;                   /* 0x0c */

struct TrackNode {
    int           state;        /* +0x00 */
    short         sx;           /* +0x04 */
    short         sy;           /* +0x06 */
    void*         cls;          /* +0x08 */
    TrackDesc*    desc;         /* +0x0c */
    void*         owner;        /* +0x10 */
    TrackJoint    jin;          /* +0x14 */
    TrackJoint    jout;         /* +0x20 */
    unsigned char part2c[0x50 - 0x2c];
};                              /* 0x50 */

extern int TrackJointSloped(TrackDesc* d, int din, int dout);   /* 0x00429910 */

/* --------------------------------------------------------------------------
 * 0x00429990 -- walk FORWARD to the end of the run and count its sloped
 * joints.
 *
 * `*endOut` comes back as the first RAISED piece reached through jout (which
 * is `n` itself when `n` is already raised, and then the count is 0); the
 * return value is how many of the joints crossed on the way are sloped.
 * coaster5.c's TrackJoinPieces takes the count and DISCARDS the out-value,
 * overwriting the cursor with `a->jout.node` immediately afterwards --
 * reproduced there.
 *
 * Its mirror 0x00429940 is the same body walking jin instead of jout; the
 * two are one source compiled twice with the link field changed (+0x28
 * against +0x1c), identical index for index otherwise.
 *
 * CODEGEN: the `while` guard is peeled, so the `n->desc` load feeding the
 * classifier's first argument is shared with the guard's `test byte ptr
 * [eax],1`, and the exit block is TAIL-DUPLICATED -- VC6 emits one copy for
 * the guard's failing edge and one for the latch's, and the two differ only
 * in which scratch register carries `endOut`.
 * ----------------------------------------------------------------------- */
// FUNCTION: LEGOLAND 0x00429990
int TrackRunSteps(TrackNode* n, TrackNode** endOut)
{
    int count = 0;

    while (!(n->desc->raised & 1)) {
        if (TrackJointSloped(n->desc, n->jin.dir, n->jout.dir))
            count++;
        n = n->jout.node;
    }
    *endOut = n;
    return count;
}

/* --------------------------------------------------------------------------
 * 0x004296f0 -- find the end of one SLOPED SPAN and how long it is.
 *
 * TrackJoinPieces calls this the moment it meets a sloped piece: the span
 * runs from `n`'s successor forward over every consecutive sloped joint, and
 * what comes back is
 *
 *     *steps  the number of joints in the span, counted from 1
 *     return  the LAST piece of the span -- `cur->jin.node`, where `cur` is
 *             the first piece past it
 *
 * so the caller can lay a ramp of `steps` equal rises across it and then
 * resume at the returned piece's own jout neighbour. `*steps` starts at 1
 * because the joint between `n` and its successor is already known sloped.
 *
 * CODEGEN: `*steps` is incremented THROUGH the pointer (`mov eax,[edi] / inc
 * eax / mov [edi],eax`) rather than in a register -- the classifier call may
 * alias it, so VC6 reloads it every iteration.
 * ----------------------------------------------------------------------- */
// FUNCTION: LEGOLAND 0x004296f0
TrackNode* TrackRunSpanEnd(TrackNode* n, int* steps)
{
    TrackNode* cur = n->jout.node;

    *steps = 1;
    while (TrackJointSloped(cur->desc, cur->jin.dir, cur->jout.dir)) {
        (*steps)++;
        cur = cur->jout.node;
    }
    return cur->jin.node;
}

/* --------------------------------------------------------------------------
 * 0x00429840 -- can a new piece of height `h` be JOINED to this end?
 *
 * coaster5.c's TrackFitCheckChain asks this once per end of a spliced piece:
 * the record's head_node with the descriptor's h1, the tail_node with its h0.
 * The answer is the partner's run-end geometry (which the constructors then
 * join to) or NULL for "the heights do not fit".
 *
 * The rule is the other half of TrackJoinPieces, read from the fit side:
 *
 *   * Walk to the end of the partner's run -- FORWARD through jout when the
 *     partner has a jout neighbour, BACKWARD through jin (0x00429940) when
 *     its jout end is the free one. So the walk always goes AWAY from the
 *     end being joined.
 *   * If that run contains ANY sloped joint, accept: the run can absorb the
 *     height difference when TrackJoinPieces re-profiles it.
 *   * If it contains none, the run is rigidly flat, so the new piece's end
 *     height must ALREADY equal the far end's -- `(float)h` against
 *     `jout.h` for the backward walk and `jin.h` for the forward one, i.e.
 *     against the joint the run ends on.
 *
 * TWO LEVERS, both measured here:
 *  * The out-param local must be declared in EACH ARM'S OWN BLOCK. As one
 *    function-level local the two arms' argument setup (`lea <reg>,[esp+4] /
 *    push / push`) is IDENTICAL IR and VC6 head-merges it above the branch,
 *    losing three instructions (33 against 36). Two block-scoped locals are
 *    distinct symbols at merge time, so the arms stay apart -- and the
 *    register allocator still colours both into the ONE dead `n` argument
 *    slot afterwards, which is why the function has no frame at all. The
 *    original's two arms differ only in `lea ecx` versus `lea edx`, which is
 *    itself the proof they were never merged.
 *  * `e = g;` immediately after the call is what puts the out-value in ONE
 *    register for both the compare and the return. Read only at its uses,
 *    VC6 emits a separate `[esp+4]` load per block (the local is
 *    address-taken, so nothing is CSE'd across the branch); the copy
 *    statement schedules the load into the call's return slot, BEFORE the
 *    pending `add esp,8`, exactly where the original has it.
 * Layout: `!k && h != end` puts the two `return 0`s inline and exiles the
 * single shared `return e` past both arms.
 * ----------------------------------------------------------------------- */
extern int TrackRunStepsBack(TrackNode* n, TrackNode** endOut);  /* 0x00429940 */

// FUNCTION: LEGOLAND 0x00429840
TrackNode* TrackFitEndGeom(TrackNode* n, int h)
{
    TrackNode* e;

    if (n->jout.node == 0) {
        TrackNode* g;
        int        k = TrackRunStepsBack(n, &g);

        e = g;
        if (!k && (float)h != e->jout.h)
            return 0;
    } else {
        TrackNode* g2;
        int        k = TrackRunSteps(n, &g2);

        e = g2;
        if (!k && (float)h != e->jin.h)
            return 0;
    }
    return e;
}

/* ==========================================================================
 * 0x00426560 -- build a 3x3 ROTATION BASIS from one direction vector.
 *
 * coaster3d.c (the piece supports) and schoolcar3.c (the track tube mesh)
 * both call this on the direction their geometry hook returned, and then
 * hand the basis to 0x004264e0 to make a 4x4. It is Gram-Schmidt against the
 * WORLD UP axis:
 *
 *     row2 = {0, 0, 1}            a temporary: world up
 *     row1 = row2 x dir           across the direction, horizontal
 *     row2 = dir  x row1          the true up for this direction
 *     row0 = dir
 *     normalise all three
 *
 * so row 0 is forward, row 1 is left/right and row 2 is up -- and row 2 is
 * the row coaster3d.c dots the light direction against to decide whether a
 * support's shadow is painted before or after its model. The basis is
 * DEGENERATE when `dir` is vertical (both crosses collapse to zero); nothing
 * guards against that, because a track direction is never straight up.
 *
 * The three normalise calls are one loop over a 0x0c-byte row cursor with
 * its own down-counter; `row0 = *dir` is a 12-byte struct assignment, whose
 * lowering is what materialises the destination address into a SECOND
 * register (`mov eax,esi`) while esi stays the loop cursor.
 * ======================================================================== */
typedef struct Vec3f { float x; float y; float z; } Vec3f;
typedef struct Mat3  { Vec3f r[3]; } Mat3;                      /* 0x24 */

extern void Vec3Cross(const Vec3f* a, const Vec3f* b, Vec3f* out); /* 0x00425cf0 */
extern void Vec3Normalise(Vec3f* v);                               /* 0x00425d50 */

// FUNCTION: LEGOLAND 0x00426560
void MakeRotation(const Vec3f* dir, Mat3* out)
{
    int i;

    out->r[2].x = 0.0f;
    out->r[2].y = 0.0f;
    out->r[2].z = 1.0f;
    Vec3Cross(&out->r[2], dir, &out->r[1]);
    Vec3Cross(dir, &out->r[1], &out->r[2]);
    out->r[0] = *dir;
    for (i = 0; i < 3; i++)
        Vec3Normalise(&out->r[i]);
}

/* ==========================================================================
 * 0x00421660 -- build the SEATED RIDER model for one coaster car.
 *
 * coaster5.c's CoasterCar_Create calls this on the appearance block
 * coaster4.c's BuildBlokeAppearance has just filled, and stores the result in
 * the car's +0x08 (a null result sets the car's `broken` flag). It is the
 * module's model-INSTANCING routine, and the mechanism is a private copy of a
 * shared template with two small substitution tables applied to it:
 *
 *  1. Pick the seat model by the appearance's normalised sex flag -- the same
 *     two names CoasterCar_Create uses, "sit.lomansit" (0x004b59f8) for 0 and
 *     "sit.logirlsit" (0x004b59e8) for 1 -- and fetch THREE things for it,
 *     one per table LoadCoasterData built:
 *         0x004206b0  the `.lms` MESH        (g_coaster_tab_a)  -> which
 *                     records of the instance are parts/colours
 *         0x00420710  the `.lfm` TEMPLATE    (g_coaster_tab_b)  -> the bytes
 *                     that get copied
 *         0x00420730  its SIZE in bytes      (g_coaster_tab_b2)
 *     and, from the same branch, the DEFAULTS this sex's template was authored
 *     with: two part names ("Chest visitor1"/"Face01" for the man,
 *     "Chest girly2"/"Face01" for the girl, at 0x004b596c / 0x004b597c) and
 *     two colours (0x191919 + 0xf11a22 for the man, 0xf11a22 + 0x008b4a for
 *     the girl, at 0x004b5964 / 0x004b5974). The four tables are laid out
 *     colours-then-names per sex, adjacent in .data.
 *  2. Allocate `size` bytes; a failed allocation returns 0 with no clean-up.
 *  3. Build the two 2-entry substitution tables, {default -> wanted}:
 *         parts   = { Part(default_name[k]),   Part(app->part[k])   }
 *         colours = { Colour(default_rgb[k]),  Colour(app->colour[k]) }
 *     which is what identifies BlokeApp's fields: +0x0c and +0x20 are two
 *     0x14-byte part NAMES (chest, face) and +0x04/+0x08 are two RGB COLOURS
 *     (legs, arms), handed to 0x004207d0, the palette search that masks to
 *     24 bits.
 *  4. `memcpy` the template into the new block.
 *  5. Rewrite the copy: the `.lms` mesh carries three lists of 0x10-byte
 *     records whose first field is a SHORT INDEX into the template's array of
 *     0x0c-byte records; for every listed record, the pointer at its +0x00 is
 *     looked up in a substitution table and replaced if it matches. The
 *     +0x28/+0x2c list uses the PART table, the +0x18/+0x1c and +0x20/+0x24
 *     lists the COLOUR table. Anything not in the table is copied through
 *     unchanged, so the template's own parts survive.
 *
 * So a rider is the shared seat template with the bloke's chest, face, leg
 * colour and arm colour patched in -- four substitutions, no other
 * per-instance state.
 *
 * WHAT THE CODEGEN SAYS (all of it exact):
 *  * The three name-keyed loads share ONE `add esp,0xc`, and the four
 *    allocator arguments one `add esp,0x10`.
 *  * `push esi` sinks past the allocation guard, and that guard's `return 0`
 *    needs no `xor` -- VC6 knows eax still holds the null it just tested.
 *  * Both table-build loops are `for (k = 0; k < 2; k++)` down-counters whose
 *    FIRST argument is `*names++` / `*colours++` (the cursor advances BEFORE
 *    the call, the recorded `&arr[n++]` rule) while the second is a
 *    latch-advanced cursor over the appearance's own array. Register
 *    pressure decides where each loop's counter lives: the part loop has
 *    four callee-saved values already, so ITS counter is a frame slot
 *    reloaded and stored every iteration, while the colour loop gets ebp and
 *    spills the `colours` cursor to the frame instead.
 *  * Each substitution table is written `from` then `to` through a cursor
 *    anchored on `.to`, storing `[esi-4]` / `[esi]` -- the recorded
 *    ascending-walk anchor, which only two per-field assignments produce.
 *
 * THE ORIGINAL'S THREE SUBSTITUTION LOOPS ARE NOT THE SAME CODE. Loops 1 and
 * 3 are 0x42 bytes and loop 2 is 0x40: loops 1 and 3 MATERIALISE the record
 * address (`mov edx,[list] / mov eax,edi / add eax,edx`) where loop 2 folds
 * one add away (`mov eax,[list] / add eax,edi`), and neither folds the sum
 * into the `movsx` addressing mode the way a plain `list[i].idx` does. Since
 * VC6 is deterministic, the three loops were not written identically. That is
 * reproduced here with one free volatile read per loop, at a site the
 * original loads on every iteration either way: loops 1 and 3 read the record
 * through a `const volatile MeshRef*`, loop 2 reads the list pointer through
 * a volatile cast. Both are free; without either, VC6 folds `list + i*16`
 * into the `movsx` and the body is 5 instructions short.
 *
 * MEASURED AND INERT on the fold (about 30 spellings): `list[i].idx`, a named
 * `&list[i]` / `list + i` / `i + list` pointer (also declared at function
 * level, and as a separate statement before its use), a `const short*` to the
 * field, byte arithmetic through `char*`, an integer-typed address with the
 * offset written first, `void*` / `char*` / `int` list fields cast at the use,
 * a 2-byte element indexed `[i*8]`, an explicit byte-offset local (which also
 * moves the IV's `xor edi,edi` above the zero-trip guard -- proof the offset
 * is VC6's own strength reduction, not a source variable), a whole-record
 * struct copy, a `{ptr,count}` array member, a `static __inline` helper taking
 * the pair by pointer, a `short` index local, all five declaration orders of
 * the body's locals, and duplicating the index expression at the store.
 *
 * WIP RESIDUAL: 191/191 instructions, 549/549 BYTES -- every encoding is
 * right -- and 12 strict mismatches which are also 12 register-blind and 12
 * offset-blind: three pure SCHEDULING windows, one per loop. In loops 1 and 3
 * the original hides the address's AGI by issuing `mov ecx,[template]` and
 * `lea edx,[table]` between the `add` and the `movsx`; a volatile read cannot
 * be moved, so ours issues the `movsx` immediately and fills afterwards (5
 * each). In loop 2 the same two instructions are `add eax,edi` and
 * `mov ecx,[template]` in the other order (2). Making the template read
 * volatile too pins it correctly but re-ranks esi/edi in the PROLOGUE (22),
 * and hoisting the table cursor into its own local, splitting the body into
 * declarations plus statements, and folding the index into the template
 * subscript are all inert.
 * ======================================================================== */
typedef struct BlokeApp {
    int   sex;                  /* +0x00  0 man, 1 girl */
    int   colour[2];            /* +0x04  legs, arms -- 24-bit RGB */
    char  part[2][0x14];        /* +0x0c  chest, face -- part names */
    void* bloke;                /* +0x34 */
} BlokeApp;                     /* 0x38 */

/* One {default -> wanted} pair of an instance's substitution table. */
typedef struct SubEnt { void* from; void* to; } SubEnt;         /* 8 */

/* The `.lfm` template / instance: an array of 0x0c-byte records whose first
 * dword is the part or colour this record draws with. */
typedef struct ModelRec { void* p; int f04; int f08; } ModelRec; /* 0x0c */
typedef struct RiderModel { ModelRec rec[1]; } RiderModel;

/* One entry of a `.lms` reference list: which instance record to patch. */
typedef struct MeshRef {
    short         idx;          /* +0x00  index into RiderModel.rec[] */
    unsigned char pad02[0x10 - 2];
} MeshRef;                      /* 0x10 */

typedef struct SeatMesh {
    unsigned char pad00[0x18];
    MeshRef*      list_b;       /* +0x18  colour-keyed records */
    int           n_b;          /* +0x1c */
    MeshRef*      list_c;       /* +0x20  colour-keyed records */
    int           n_c;          /* +0x24 */
    MeshRef*      list_a;       /* +0x28  part-keyed records */
    int           n_a;          /* +0x2c */
} SeatMesh;

extern void*        AllocZeroed(unsigned int size, int a, int b, int c); /* 0x004775b0 */
extern SeatMesh*    LoadCoasterMesh(const char* name);          /* 0x004206b0 */
/* coaster5.c's name for 0x00420710; it is really the `.lfm` INSTANCE
 * TEMPLATE table, not a texture set. */
extern RiderModel*  LoadCoasterMeshTex(const char* name);       /* 0x00420710 */
extern unsigned int GetCoasterModelSize(const char* name);      /* 0x00420730 */
extern void*        FindCoasterPart(const char* name);          /* 0x00420790 */
extern void*        FindCoasterColour(int rgb);                 /* 0x004207d0 */

extern char        g_seat_name_girl[];                          /* 0x004b59e8 */
extern char        g_seat_name_man[];                           /* 0x004b59f8 */
extern const int   g_rider_colour_man[2];                       /* 0x004b5964 */
extern const char* g_rider_part_man[2];                         /* 0x004b596c */
extern const int   g_rider_colour_girl[2];                      /* 0x004b5974 */
extern const char* g_rider_part_girl[2];                        /* 0x004b597c */

// WIP-FUNCTION: LEGOLAND 0x00421660  (191/191 insns, 549/549 B -- byte-exact; 12 mismatches, all three loops' address/AGI scheduling window, strict == register-blind == offset-blind)
void* CoasterCar_BuildRider(BlokeApp* app)
{
    SubEnt       sub_part[2];
    SubEnt       sub_colour[2];
    SeatMesh*    mesh;
    RiderModel*  model;
    RiderModel*  out;
    unsigned int size;
    const char** names;
    const int*   colours;
    int          i;
    int          j;

    if (app->sex == 0) {
        mesh    = LoadCoasterMesh(g_seat_name_man);
        model   = LoadCoasterMeshTex(g_seat_name_man);
        size    = GetCoasterModelSize(g_seat_name_man);
        names   = g_rider_part_man;
        colours = g_rider_colour_man;
    } else {
        mesh    = LoadCoasterMesh(g_seat_name_girl);
        model   = LoadCoasterMeshTex(g_seat_name_girl);
        size    = GetCoasterModelSize(g_seat_name_girl);
        names   = g_rider_part_girl;
        colours = g_rider_colour_girl;
    }
    out = (RiderModel*)AllocZeroed(size, 0, 0, 0);
    if (!out)
        return 0;
    for (i = 0; i < 2; i++) {
        sub_part[i].from = FindCoasterPart(*names++);
        sub_part[i].to   = FindCoasterPart(app->part[i]);
    }
    for (i = 0; i < 2; i++) {
        sub_colour[i].from = FindCoasterColour(*colours++);
        sub_colour[i].to   = FindCoasterColour(app->colour[i]);
    }
    memcpy(out, model, size);
    /* The volatile record read is FREE -- the original loads the index every
     * iteration -- and is what stops VC6 folding `list + i*16` into the
     * `movsx`. See the note above: loop 2 needs the other spelling. */
    for (i = 0; i < mesh->n_a; i++) {
        const volatile MeshRef* rp = &mesh->list_a[i];
        int   k = rp->idx;
        void* v = model->rec[k].p;

        for (j = 0; j <= 1; j++)
            if (sub_part[j].from == v) {
                v = sub_part[j].to;
                break;
            }
        out->rec[k].p = v;
    }
    for (i = 0; i < mesh->n_b; i++) {
        const MeshRef* rp = *(MeshRef* volatile*)&mesh->list_b + i;
        int   k = rp->idx;
        void* v = model->rec[k].p;

        for (j = 0; j <= 1; j++)
            if (sub_colour[j].from == v) {
                v = sub_colour[j].to;
                break;
            }
        out->rec[k].p = v;
    }
    for (i = 0; i < mesh->n_c; i++) {
        const volatile MeshRef* rp = &mesh->list_c[i];
        int   k = rp->idx;
        void* v = model->rec[k].p;

        for (j = 0; j <= 1; j++)
            if (sub_colour[j].from == v) {
                v = sub_colour[j].to;
                break;
            }
        out->rec[k].p = v;
    }
    return out;
}
