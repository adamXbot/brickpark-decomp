/* LEGOLAND -- the fourth driving-school manoeuvre, and the COASTER 3D TRACK
 * MESH pipeline that schoolcar.c reaches through externs.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is
 * owned elsewhere).  schoolcar.c owns the car record and the queue pop,
 * schoolcar2.c owns the two manoeuvre choosers and the two turn manoeuvres,
 * goldrush.c owns the per-frame driver StepSchoolCar, roads2.c owns the road
 * graph the cars drive.
 *
 * =========================================================================
 * PART 1 -- MANOEUVRE D (0x00401660)
 * =========================================================================
 * The fifth of the driving school's five manoeuvre codes (0 stop, 1 left,
 * 2 straight, 3 right, 4 pull off the road, 5 arrive).  See schoolcar2.c for
 * the manoeuvre system as a whole; the note above the function covers this
 * one.
 *
 * =========================================================================
 * PART 2 -- THE COASTER TRACK MESH  (0x004234e0, 0x00428cb0, 0x00428f00)
 * =========================================================================
 * schoolcar.c calls these three `Coaster3D_DrawMesh`, `Coaster3D_BuildTrackMesh` and `Coaster3D_InitTrackTopology`.
 * They are one pipeline, and naming them is part of this file's job:
 *
 *   Coaster3D_InitTrackTopology  0x00428f00  once, from CoasterSceneInit
 *   Coaster3D_BuildTrackMesh     0x00428cb0  per draw: geometry -> vertices
 *   Coaster3D_DrawMesh           0x004234e0  per draw: vertices -> spans
 *
 * THE MODEL.  A stretch of roller-coaster track is drawn as a TUBE swept
 * along the track: a hexagonal RING of six points is placed at each of up to
 * 30 track segments and the quads between consecutive rings are filled.
 *
 *   * The ring is fixed: six points 60 degrees apart on a circle of radius
 *     1.8, lying in the plane perpendicular to the track direction.  Its
 *     positions live at 0x006121c8 (three floats each, x = 0) and its
 *     outward normals at 0x006126d8 (two floats each, the in-plane cos/sin).
 *   * The topology is fixed too: 12 triangles and 24 index PAIRS per
 *     segment, stamped out 30 times at init into 0x00612708 and 0x006148b8.
 *     A triangle's index is not a vertex number but a pair reference with
 *     bit 31 selecting which half of the pair -- the seam encoding that lets
 *     one triangle list close the tube around its own wrap.
 *   * Per draw, the object being drawn supplies each segment's position and
 *     direction through a pair of hooks at +0x4c, indexed by a slot number.
 *     Those become a 4x4 transform, composed with the camera matrix at
 *     0x008299fc, and the six ring points are projected through it into the
 *     shared vertex buffer at 0x006139c8 (0x14 bytes per vertex: screen x,
 *     y, z, a clip word and a light value).
 *   * The light is flat per segment: the view direction dotted with rows 1
 *     and 2 of the segment's own basis gives two coefficients, and vertex j
 *     of that segment gets  a*cos[j] + b*sin[j] + ambient.
 *   * Drawing then walks the 12*(n-1) triangles, rejects the back-facing
 *     ones by the sign of the 2D cross product and the wholly-clipped ones
 *     by the OR of the three vertices' clip words, and queues the survivors.
 *
 * Both draw halves time themselves with RDTSC into the coaster module's
 * per-frame counters (0x00615f68 and 0x0060f8fc) that schoolcar.c's
 * Coaster3D_SampleStats reads, which is why they both carry an ebp frame.
 * ========================================================================= */

#include <math.h>

#pragma intrinsic(cos, sin)

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
 * comes back in edx:eax. */
extern CarPos RotateByHeading(int fwd, int side, int dir);      /* 0x00401000 */

/* Steps a map square 2 squares (half a road block) in the heading `dir`;
 * even headings leave `to` untouched. */
extern void   Pos_Step2(CarPos* from, CarPos* to, int dir);     /* 0x00480840 */

/* coaster.c's theme switch: it decides which way round the manoeuvre
 * routines take their two arms, i.e. which side of the road the driving
 * school teaches. */
extern int g_4c11c0;                                            /* 0x004c11c0 */

/* =========================================================================
 * 0x00401660 -- SchoolCarManoeuvreD: pull off the road (code 4).
 *
 * The manoeuvre StepSchoolCar runs when the free-driving chooser
 * (SchoolCarNextManoeuvreHorn) finds no usable road block ahead at all: the
 * car swings off the carriageway and stops there.  It is A's and C's shape
 * with far tighter offsets -- forward 0x0d and 0x21, sideways 0x28 and 0x54,
 * against a turn's 0x68/0x10c/0x1a8/0x200 -- only TWO curve points instead of
 * four, and it is the ONLY manoeuvre that ends with `c->on_road = 1`, the
 * byte StepSchoolCar clears every time it asks for a new manoeuvre.
 *
 * Unlike A and C, whose two arms are a TIGHT and a WIDE curve, D's two arms
 * are the SAME curve mirrored: the theme switch at 0x004c11c0 only decides
 * which SIDE of the road the car pulls off towards (sideways +0x28/+0x54 with
 * a +2 heading delta, or -0x28/-0x54 with -2).  Then it steps the running
 * square half a block along the NEW heading and drops a third waypoint there,
 * so the car ends up one half-block off the road, parked across it.
 *
 * *** NEW LEVER (this closed the function on the first structural try) ***
 * A PENDING cdecl `add esp` CANNOT CROSS A BRANCH JOIN, so a shared tail that
 * contains a CALL proves the tail was written TWICE.  The original cleans all
 * three calls' arguments with a single `add esp,0x24` placed AFTER the
 * Pos_Step2 in the shared tail.  Written as an if/else whose tail is shared,
 * VC6 flushes each arm's own `add esp,0x18` before the join (and mid-store,
 * at index 37, which is where this build first diverged) and then Pos_Step2's
 * `add esp,0xc` separately -- 88/116, and three surplus instructions from the
 * two zero-extends the split allocation then needs.  Writing the whole tail
 * -- Pos_Step2, the third waypoint, `c->start`, `c->nwp` and `c->on_road` --
 * inside BOTH arms and letting VC6 cross-jump it is 113/113 exactly, with the
 * merged `add esp,0x24` and the original's `and eax,0xff` zero-extend in
 * place.  So: where a merged stack cleanup spans a join, duplicate the tail
 * in the source; the cross-jump puts it back.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x00401660
void SchoolCarManoeuvreD(SchoolCar* c)
{
    CarPos p;
    CarPos o;
    int    n;

    n = c->nwp;
    p = c->start;
    if (g_4c11c0) {
        o = RotateByHeading(0x0d, 0x28, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        o = RotateByHeading(0x21, 0x54, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        c->turn = (unsigned char)((c->turn + 2) & 7);
        Pos_Step2(&p, &p, c->turn);
        c->wp[n].x = p.x << 16;
        c->wp[n].y = p.y << 16;
        c->start = p;
        c->nwp = (unsigned char)(n + 1);
        c->on_road = 1;
    } else {
        o = RotateByHeading(0x0d, -0x28, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        o = RotateByHeading(0x21, -0x54, c->turn);
        c->wp[n].x = ((p.x << 8) + o.x) << 8;
        c->wp[n].y = ((p.y << 8) + o.y) << 8;
        n++;
        c->turn = (unsigned char)((c->turn - 2) & 7);
        Pos_Step2(&p, &p, c->turn);
        c->wp[n].x = p.x << 16;
        c->wp[n].y = p.y << 16;
        c->start = p;
        c->nwp = (unsigned char)(n + 1);
        c->on_road = 1;
    }
}

/* =========================================================================
 * THE COASTER'S 3D TRACK MESH  (0x00423480 .. 0x00429150)
 * =========================================================================
 * schoolcar.c reaches three routines in this region through externs and
 * calls them Coaster3D_BuildTrackMesh, Coaster3D_DrawMesh and Coaster3D_InitTrackTopology.  They are one pipeline.
 * ========================================================================= */

typedef struct Vec3f { float x; float y; float z; } Vec3f;
typedef struct Mat3  { float m[9];  } Mat3;         /* 0x24 */
typedef struct Mat4  { float m[16]; } Mat4;         /* 0x40 */

/* coaster.c's 3D view record; only the last three of its four floats are a
 * direction, and they are what the shading dot product uses. */
typedef struct ViewRec { float a; float b; float c; float d; } ViewRec;

/* One transformed screen vertex, 0x14 bytes. */
typedef struct TrackVtx {
    int x;              /* +0x00 */
    int y;              /* +0x04 */
    int z;              /* +0x08 */
    int clip;           /* +0x0c  clip / facing code bits */
    int shade;          /* +0x10  the per-vertex light this file computes */
} TrackVtx;

/* One entry of the six-point RING the track tube is swept from: the two
 * in-plane components of that point's outward normal. */
typedef struct RingNormal { float e0; float e1; } RingNormal;

typedef struct DrawObj DrawObj;

/* An object's per-slot geometry hooks, an array of PAIRS at +0x4c.
 *
 * PORT-M5 -- WHAT THE SECOND PARAMETER REALLY IS, AND WHICH SLOTS EXIST.
 * The pairs are `g_car_class_vt`'s eight-entry groups (schoolcar.c's
 * CarClassTablesInit, 0x00422210), one group per curve kind, and the bodies
 * registered in entries 0..5 -- TrackCurve_CubicOffsetPlus / CubicPosition /
 * CubicOffsetMinus / CubicTangent and the Line and Arc families -- are all
 *
 *     void f(const Curve* curve, float t, Vec3f* out)
 *
 * (schoolcar8.c, coastertiny.c).  So the dword this file passes as `elem` is
 * a FLOAT'S BIT PATTERN, the segment's curve parameter, not a pointer: the
 * PORT-M2 section 3 class, and the same fact coaster10.c's
 * TrackCurve_EvaluatePosition / _EvaluateUp already carry.  On x86 cdecl a
 * dword push is a dword push either way, which is why the `void*` spelling
 * matched; a wasm32 call_indirect whose type is not the target's traps, so
 * the portable arm casts to the bodies' real type and moves the bits across
 * unconverted.  This body is a WIP and its argument spelling is a codegen
 * lever, so the matching arm is left exactly as it was.
 *
 * `slot` at the two call sites below is a RAIL index and is never 3.  Slot 3
 * of each group is (entry 6, entry 7) = (TrackCurve_GatherParams /
 * GetLimits / GetQuarterTurnSamples, TrackCurve_NormalAt / LineUpVector /
 * CubicUpVector), and entry 6 is the COLLECTOR at vt+0x18 that schoolcar.c's
 * DrawTrackEnd_Fetch has already called to produce this routine's `list` and
 * `n`:
 *     0x00428e7d  push 0x612178          ; the float array
 *     0x00428e82  push esi               ; the object
 *     0x00428e86  call dword ptr [eax+0x18]
 *     0x00428e96  push eax               ; ... its return IS `n`
 * -- `int f(obj, float* out)`, two arguments.  Slots 0/1/2 are the three
 * rails (offset-plus, centre, offset-minus), which is the whole of what a
 * mesh sweep needs.  PORT-M3 listed this as possibly "the recovery of one of
 * the three is wrong about its parameters"; it is not -- the collectors are
 * right, they are simply not position hooks, and nothing indexes them here. */
typedef struct PosHooks {
    void (*get_pos)(DrawObj* o, void* elem, Vec3f* out);    /* +0x00 */
    void (*get_dir)(DrawObj* o, void* elem, Vec3f* out);    /* +0x04 */
} PosHooks;

#ifdef LEGOLAND_PORTABLE
/* PORT-M5: the pair's real type (see above).  Read through the slot rather
 * than retyping PosHooks, so the matching declaration is untouched. */
typedef void (*LLPosHook)(DrawObj*, float, Vec3f*);
#define LL_POS_HOOK(slot_field) (*(LLPosHook*)(void*)&(slot_field))
#endif

struct DrawObj {
    unsigned char pad00[0x4c];
    PosHooks*     hooks;        /* +0x4c */
};

/* The mesh descriptor the two halves share. */
typedef struct MeshDesc {
    int         f00;            /* +0x00 */
    int         f04;            /* +0x04 */
    int         nverts;         /* +0x08 */
    int         ntris;          /* +0x0c */
    TrackVtx*   verts;          /* +0x10  0x006139c8 */
    int       (*pairs)[2];      /* +0x14  0x006148b8 */
    int       (*tris)[3];       /* +0x18  0x00612708 */
} MeshDesc;

extern MeshDesc g_track_mesh;                                   /* 0x004b5f60 */
extern ViewRec  g_view_cur;                                     /* 0x004b5c9c */
extern float    g_view_half_lo;                                 /* 0x0082999c */
extern float    g_view_half_hi;                                 /* 0x00829a60 */
extern Mat4     g_view_matrix;                                  /* 0x008299fc */
extern int      g_stat_c_615f68;                                /* 0x00615f68 */

extern Vec3f      g_ring_src[6];                                /* 0x006121c8 */
extern TrackVtx   g_track_verts[];                              /* 0x006139c8 */
extern RingNormal g_ring_normals[6];                            /* 0x006126d8 */

extern void MakeRotation(const Vec3f* dir, Mat3* out);          /* 0x00426560 */
extern void MakeTransform(const Vec3f* p, const Mat3* r, Mat4* o); /* 0x004264e0 */
extern void MatMul(const Mat4* a, const Mat4* b, Mat4* out);    /* 0x00426120 */
extern void TransformVerts(const Vec3f* s, TrackVtx* d, const Mat4* m, int st, int n); /* 0x00426250 */

/* =========================================================================
 * 0x00428cb0 -- Coaster3D_BuildTrackMesh (schoolcar.c's `Coaster3D_BuildTrackMesh`).
 *
 * THE NAME.  schoolcar.c's two callers (DrawTrackEnd_Fetch / _Cached) call it
 * with the value the object's own +0x4c hook slot 3 returned and cache that
 * value in g_615f6c; this body uses it as a COUNT (`lea ecx,[eax-1]`, then a
 * loop of that many), so schoolcar.c's `void* model` parameter is really an
 * `int` segment count, and the object's transform hook returns how many track
 * segments it wants drawn.  The routine walks that many entries of the
 * pointer array it is given, transforms one six-point RING per entry into the
 * shared vertex buffer, fills each vertex's SHADE, writes the mesh's vertex
 * and triangle counts into the descriptor at 0x004b5f60 and returns it.
 * Coaster3D_DrawMesh (0x004234e0, below) is what schoolcar.c calls next, on
 * exactly that descriptor.
 *
 * PER SEGMENT.  The object's hook pair for slot `slot` supplies the segment's
 * DIRECTION (+0x04) and POSITION (+0x00); the direction becomes a 3x3 basis
 * (0x00426560), the position is offset by the caller's origin, the two make a
 * 4x4 (0x004264e0), that is composed with the camera matrix at 0x008299fc
 * (0x00426120), and 0x00426250 transforms the six ring points at 0x006121c8
 * through it into six 0x14-byte screen vertices.
 *
 * THE SHADE is a Gouraud term computed once per segment and evaluated per
 * ring point:  the view direction (the last three floats of the active view
 * record at 0x004b5c9c) is dotted with rows 1 and 2 of the segment's basis,
 * each scaled by g_view_half_lo, giving two coefficients; g_view_half_hi is
 * the ambient.  Each ring point carries its own in-plane normal (e0, e1) at
 * 0x006126d8, so vertex j's light is  a*e0[j] + b*e1[j] + c, ROUNDED by the
 * game's __ftol helper -- 0x00458930 is a bare `fistp` with no control-word
 * save, so it takes the round-to-nearest mode the game leaves set, and
 * "truncated" (which this line used to say) is the wrong direction.  Scope
 * PORT-M5.  All three coefficients stay on the x87 stack across the inner loop
 * and across the __ftol calls, and are popped with three `fstp st(0)` at the
 * loop bottom -- reproduced exactly.
 *
 * THE COUNTS: with `last = n - 1` segments' worth of gaps, ntris (+0x0c) is
 * 12 * last (six quads = twelve triangles between each pair of rings) and
 * nverts (+0x08) is 18 * last + 6.  Both are written from `last`, which is
 * why `last` and not `n` is the loop bound and has its own frame slot.
 *
 * The whole body is timed with RDTSC into the coaster module's per-frame
 * counter at 0x00615f68 (schoolcar.c's Coaster3D_SampleStats reads it), which
 * is why this function has an ebp frame.
 *
 * WHAT CLOSED (all measured on this body):
 *  * The last waypoint of the frame layout came from SCOPE, not declaration
 *    order.  Declaration order is inert here (five permutations, all
 *    byte-identical), but moving a local into the LOOP's inner scope moves it
 *    DOWN the frame: `dir` inner puts it below `rot` (128 -> 126), `mvp`
 *    inner puts it below `xf` (40 -> 36) and `pos` inner puts `t` back at the
 *    top (36 -> 32).  Three separate "which of this pair is higher in the
 *    frame" questions, one lever.
 *  * The list is walked by SUBSCRIPT (`list[i]`), not by a pointer variable.
 *    A `void** p = list` coalesces with the `list` PARAMETER and keeps its
 *    own argument slot; VC6's own strength-reduced temp cannot coalesce with
 *    a parameter and lands in the dead `o` slot, which is where the original
 *    has it.  Worth 4 and it is what frees the `list` slot for the trip
 *    counter.
 *  * The inner loop is written `for (j = 0; j < 6; j++)` over the array, NOT
 *    as a pointer walk: VC6's own strength reduction emits the original's
 *    SIGNED `cmp esi,<end> / jl`, where a pointer comparison gives `jb`.
 *  * The two count stores are written nverts-then-ntris, the OPPOSITE of the
 *    order they appear in: that is what puts 9*last in edx and 3*last back
 *    into `last`'s own register.  Worth 32 -> 8.
 * ========================================================================= */
/* Scope G, Fable pass (8 -> 6) then two re-derivations.  Still 6.
 *
 * CLOSED EARLIER: the two preheader homing stores (old indices 21-22).  The
 * vertex cursor is written as an expression of the loop counter at the TOP of
 * the body, `out = g_track_verts + i * 6;`, with no `out = g_track_verts`
 * before the loop and no `out += 6` latch.  The RULE behind it, confirmed
 * this round from both sides: VC6 creates the two derived induction variables
 * in REVERSE order of their first appearance in the body and homes them in
 * CREATION order into the dead argument slots -- first created into o's slot
 * [ebp+8], second into n's [ebp+0x14].  So `out` first in the body puts the
 * LIST cursor in [ebp+8] (the original); putting the element statement first
 * swaps both the homes (21-22) and their reloads (25-26), measured 44-48.
 *
 * OPEN (27-32): the element window.  Original:
 *      27 mov edx,[edi+0x4c]   H  hooks       31 mov [ebp-0x14],ecx   S
 *      28 mov ecx,[eax]        E  element     32 mov ecx,[ebp+0x10]   T slot
 *      29 lea eax,[ebp-0x48]   A  &dir        33..35 push A, push esi, push o
 *      30 mov esi,ecx          C  copy
 *
 * THE FRAME IS SETTLED (previous round): esp is exactly 0xc8 with no padding;
 * &dir -0x48, &rot -0x3c, &pos -0x10, &xf -0x88, &mvp -0xc8, t -4 (via __asm)
 * account for every escaping address, and NOTHING takes the address of -0x14.
 * A plain local there is deleted and the frame closes up; a non-escaped object
 * of any shape is scalarised.  So -0x14 can only be the first member of the
 * aggregate whose escape is the `&pos` passed to get_pos:
 *      struct { void* elem; Vec3f pos; } cur;   cur.elem = list[i];
 * and the element must ALSO be a plain local `e`, because both hook calls push
 * the same register.  That spelling gives 151 insns / 441 BYTES -- the
 * original's length to the byte, its exact instruction multiset, and its
 * `mov esi,<scratch>` register copy, with no volatile.  Call it build P.
 *
 * WHAT THIS ROUND ADDS.  Three measured facts that re-frame the residual.
 *
 *  1. IT IS NOT A REGISTER-CURSOR PHASE SHIFT.  Build P and the original are
 *     in the SAME phase at index 26 (both `mov eax,[ebp+8]`) and both hand ecx
 *     to the next allocation.  What differs is WHICH VALUE is allocated at
 *     each step: the original allocates E, H, A, T; build P allocates H, A, E,
 *     T.  VC6 hands out scratches on the cycle eax -> ecx -> edx; that cycle
 *     is confirmed independently on the second hook call (its H,T,A triple is
 *     the same cyclic sequence started one step later) and on MakeTransform's
 *     three address arguments (&xf, &rot, &pos in the original; &rot, &pos,
 *     &xf in build P -- the same cycle, rotated).  Everything from 27 to 78,
 *     including the reordered `pos +=` block (with &pos off eax it stops being
 *     anti-dependent on the last `fadd [eax+8]` and floats up), is downstream
 *     of that ONE decision, and 79 re-syncs.
 *
 *  2. THE REAL QUESTION IS WHICH WEB OWNS THE FIRST PUSH, and it is narrower
 *     than "make the element its own statement".  Build P emits the original's
 *     three instructions -- load into a scratch, store, copy to esi -- but in
 *     the order S,C and then pushes the SCRATCH (`push eax`), so the temp's
 *     live range reaches the push and the whole call block rotates.  The
 *     original emits C,S: the temp dies at the store, ecx is recycled for
 *     `slot` at 32, and the push therefore has to name esi.  So what is needed
 *     is the copy emitted BEFORE the store, i.e. `e`'s web owning both pushes.
 *     STATEMENT POSITION IS NOT THE LEVER: with the element statement moved
 *     ABOVE the `out` cursor VC6 still sinks the temp into the call tree
 *     (hooks=ecx, &dir=edx, element=eax) and merely breaks the preheader too.
 *
 *  3. THE PIN IS THE VOLATILE READ, NOT THE VOLATILE STORE.  Measured on the
 *     settled record: a cast-volatile STORE with a plain read does not pin at
 *     all (443B, 123); a plain store with a cast-volatile READ reproduces this
 *     build exactly (442B, 6).  So `union {void* p; void* volatile vp;}` here
 *     is just `e = *(void* volatile*)&cur.elem`, and that read is doing both
 *     jobs at once -- it fixes the allocation order to the original's E,H,A,T
 *     AND, being volatile, stops `o->hooks` and `&dir` hoisting above it
 *     (E,S,C,H,T,A against the original's H,E,A,C,S,T) and costs the byte
 *     (`mov esi,[ebp-0x14]` 3 bytes for the original's `mov esi,ecx` 2).
 *     Two exact complements: 441B with the wrong web, or the right web behind
 *     a barrier.
 *
 * CORPUS EVIDENCE.  The original's shape -- a scratch loaded from memory, then
 * COPIED to a callee-saved register BEFORE a frame store of the same scratch --
 * occurs EXACTLY ONCE in the whole 80,686-instruction .text of legoland.exe:
 * here.  Build P's shape (copy to the callee-saved register, then push the
 * SCRATCH) is what VC6 emits routinely and is present in exact bodies
 * (PositionRouteCars 0x0041da10 and JungleCruise_Draw 0x00435bd0 both give
 * `mov eax,<mem>; mov <cs>,eax; push eax`).  This site is an outlier, like
 * Joust_Update's once-in-the-binary cross-jump, so the next lens should expect
 * an unusual construct rather than another spelling of the usual one.
 *
 * RULED OUT THIS ROUND (~40 measured spellings on top of the ~145 already in
 * the log; every one is 441B/42 or 440B/121-123 unless noted):
 *  - `e = list[i]` in any position, alone or with the store before/after it,
 *    and `cur.elem = e`, `cur.elem = e = list[i]`: `e`'s def IS the load, it
 *    takes esi directly (`mov esi,[eax]`) and the store sinks below the pushes
 *    -- 440B, 121.  Reading `list[i]` twice does the same (CSE).
 *  - `e = cur.elem` in any position, `e = cur.elem = list[i]`, an initialised
 *    inner declaration, a nested block with `void* e = cur.elem;`, `const`:
 *    all exactly build P, 441B/42.
 *  - statement fusion by comma in both directions (`out = (cur.elem = L, ...)`,
 *    `cur.elem = (out = ..., L)`, one three-comma statement): 42, or 48 when
 *    the list expression ends up first and the preheader homes swap.
 *  - an EMPTY `__asm { }` between the element statement and the call is NOT a
 *    scheduling barrier -- byte-identical to no barrier (42).  Nor is `;`.
 *  - hoisting the call's own operands into locals to get them above the pin:
 *    `h = o->hooks` (before or after the element statement, one use or two),
 *    `dp = &dir` with two uses, `s = slot`, a function-pointer local, and
 *    `(o->hooks + slot)->get_dir` -- all fold back (42, or 439B/129 when `h`
 *    survives for both calls and kills the second load).
 *  - struct-typed intermediates (`struct Elem {void* p;} tmp;` with the copy
 *    and the struct store in all three orders): 440B or 441B, never the order
 *    C-then-S.  Element member typed `unsigned`, `char*`: 42.
 *  - `cur` and `e` in every scope/declaration-order combination (both outer,
 *    both inner, either order): byte-identical, 42.
 *  - both hook calls reading `cur.elem` with no `e`: VC6 parks `slot` in esi
 *    and reloads the member after get_dir (440B, 123); mixing (first call `e`,
 *    second `cur.elem`) reloads too.
 *  - `out` as a user IV (`out = g_track_verts` before the loop, `out += 6` at
 *    the bottom) combined with the element statement first, plain and volatile:
 *    44 and 8, both diverging at 21 -- the preheader rule above forbids it.
 * The 6-mismatch build is kept because 6 < 42 on the audit metric; build P is
 * the byte-exact one and is the base to start from if the next lens finds a
 * non-volatile way to make `e`'s copy precede the store.
 *
 * fgh-100d (scope-V cancel, from pickup's uncommitted m5_slot).  Reconstructed
 * with function-scope `cur` / `e` / `tc` and
 * `tc.y = (int)slot; tc.x = (int)cur.elem; tc.x += tc.y; tc.x -= tc.y;`.
 * That is 151i / 446B, aligned ~38, first 19: copy-before-store and both
 * hook e-pushes from esi, but `slot` hoists into ebx (`mov ebx,[ebp+10h]`)
 * and the verts IV is an immediate store to [ebp+14h].  Untried idea 1
 * (cancel anchored on a load through `out`: `tc.y = out->x` / `out->shade` /
 * `g_track_verts[i*6].x`, CSE recomputes, depth-2 `if (out)`, user-IV
 * `out += 6`, `int sl = slot` outside the loop) keeps ebx = verts and does
 * not hoist slot, and still emits copy-before-store + push esi, but LICM
 * leaves a dead `mov ecx,[ebx]` in the preheader and the list cursor takes
 * [ebp+14h] (out stays a register IV, so there is only one memory IV).
 * 439B / aligned ~45 / first 19, worse than the volatile 6.  Frame-address
 * and self-anchors still fold or add arithmetic, as pickup measured.  The
 * cancel lever reproduces the window or the preheader, not both; incumbent
 * 6 kept. */
// WIP-FUNCTION: LEGOLAND 0x00428cb0  (96.0%: 151/151 insns, 442/441 bytes, 6 mismatches -- indices 27-32; the element window.  The volatile read that pins the original's allocation order (E,H,A,T) is also a scheduling barrier and costs the byte; the complementary non-volatile record spelling is 441 bytes exact with the instruction multiset right but lets the temp own the first push, rotating 27-78.  Class: which web owns the first push, not a register-cursor phase)
MeshDesc* Coaster3D_BuildTrackMesh(DrawObj* o, const Vec3f* origin, int slot,
                                   int n, void** list)
{
    unsigned int t;
    int last;
    int i;
    union { void* p; void* volatile vp; } elem;
    TrackVtx* out;
    Mat3 rot;
    Mat4 xf;
    last = n - 1;
#ifndef LEGOLAND_PORTABLE
    __asm {
        push eax
        push edx
        rdtsc
        mov  t, eax
        pop  edx
        pop  eax
    }
#else
    t = ll_rdtsc();
#endif
    if (last >= 0) {
        for (i = 0; i <= last; i++) {
            float a;
            float b;
            float c;
            int   j;
            Vec3f dir;
            Vec3f pos;
            Mat4 mvp;
            out = g_track_verts + i * 6;
            elem.vp = list[i];
#ifndef LEGOLAND_PORTABLE
            o->hooks[slot].get_dir(o, elem.p, &dir);
#else
            LL_POS_HOOK(o->hooks[slot].get_dir)(o, LL_ASFLT(elem.p), &dir);
#endif
            MakeRotation(&dir, &rot);
#ifndef LEGOLAND_PORTABLE
            o->hooks[slot].get_pos(o, elem.p, &pos);
#else
            LL_POS_HOOK(o->hooks[slot].get_pos)(o, LL_ASFLT(elem.p), &pos);
#endif
            pos.x += origin->x;
            pos.y += origin->y;
            pos.z += origin->z;
            MakeTransform(&pos, &rot, &xf);
            MatMul(&g_view_matrix, &xf, &mvp);
            TransformVerts(g_ring_src, out, &mvp, 0x14, 6);

            a = (g_view_cur.b * rot.m[3] + g_view_cur.c * rot.m[4] +
                 g_view_cur.d * rot.m[5]) * g_view_half_lo;
            b = (g_view_cur.b * rot.m[6] + g_view_cur.c * rot.m[7] +
                 g_view_cur.d * rot.m[8]) * g_view_half_lo;
            c = g_view_half_hi;
            for (j = 0; j < 6; j++)
#ifndef LEGOLAND_PORTABLE
                out[j].shade = (int)(a * g_ring_normals[j].e0 +
                                     b * g_ring_normals[j].e1 + c);
#else
                /* PORT-M5: 0x00428ded calls 0x00458930, which ROUNDS -- the
                 * file header used to say "truncated" here; corrected. */
                out[j].shade = LL_FISTP(a * g_ring_normals[j].e0 +
                                        b * g_ring_normals[j].e1 + c);
#endif
        }
    }
    g_track_mesh.nverts = 18 * last + 6;
    g_track_mesh.ntris = 12 * last;
#ifndef LEGOLAND_PORTABLE
    __asm {
        push eax
        push edx
        rdtsc
        sub  eax, t
        mov  t, eax
        pop  edx
        pop  eax
    }
#else
    t = ll_rdtsc() - t;
#endif
    g_stat_c_615f68 += t;
    return &g_track_mesh;
}

/* One vertex of a submitted polygon, 0x1c bytes; only these four fields are
 * filled from the mesh vertex. */
typedef struct PolyVtx {
    int f00;            /* +0x00 */
    int sy;             /* +0x04 */
    int sx;             /* +0x08 */
    int shade;          /* +0x0c */
    int sz;             /* +0x10 */
    int f14;            /* +0x14 */
    int f18;            /* +0x18 */
} PolyVtx;              /* 0x1c */

/* The polygon setup record handed to the span filler. */
typedef struct PolyJob {
    int      kind;      /* +0x00  always 1 here */
    int      tag;       /* +0x04  the mesh's +0x00 */
    int      and_flags; /* +0x08  AND of the three vertices' clip words */
    int      or_flags;  /* +0x0c  OR  of the three vertices' clip words */
    int      f10;       /* +0x10 */
    float    dx1;       /* +0x14 */
    float    dx2;       /* +0x18 */
    float    dy1;       /* +0x1c */
    float    dy2;       /* +0x20 */
    float    area;      /* +0x24  the 2D cross product */
    PolyVtx* v[3];      /* +0x28 */
    int      f34;       /* +0x34 */
    void*    shader;    /* +0x38 */
} PolyJob;              /* 0x3c */

typedef struct RasterState { unsigned char b[0x18]; } RasterState;

#ifndef LEGOLAND_PORTABLE
extern void Raster_SaveState(RasterState* g);                   /* 0x00423760 */
#else
extern int Raster_SaveState(RasterState* g);                   /* 0x00423760 */
#endif
extern void Raster_RestoreState(RasterState* g);                /* 0x00423790 */
extern void Raster_SubmitPoly(int nverts, PolyJob* job);        /* 0x0042a2f0 */
extern void* g_span_fillers[2];                                 /* 0x004b5658 */
extern int   g_stat_c_60f8fc;                                   /* 0x0060f8fc */

/* =========================================================================
 * 0x004234e0 -- Coaster3D_DrawMesh (schoolcar.c's `Coaster3D_DrawMesh`).
 *
 * The other half of the pipeline: it takes the descriptor
 * Coaster3D_BuildTrackMesh just filled and hands every FRONT-FACING, not
 * trivially rejected triangle of it to the span filler.  schoolcar.c calls it
 * with `&g_4b5f60` immediately after building the mesh, and it is bracketed
 * by the renderer state save/restore pair at 0x00423760 / 0x00423790 and
 * timed with RDTSC into the coaster module's second per-frame counter
 * (0x0060f8fc).  It always returns 1.
 *
 * THE INDEX INDIRECTION.  A triangle is three entries of the mesh's +0x18
 * array; each entry is not a vertex number but a reference into the +0x14
 * table of PAIRS, with bit 31 choosing which half of the pair:
 *
 *      e & 0x80000000  ->  pairs[e & 0x7fffffff][1]
 *      otherwise       ->  pairs[e & 0x7fffffff][0]
 *
 * That is the seam encoding: one pair per shared ring point, holding the two
 * vertex numbers that point has on the two sides of the seam, so one triangle
 * list serves both.  Coaster3D_InitTrackTopology (0x00428f00, below) builds
 * both tables and writes the same bit 31 into its own entries.
 *
 * THE TWO REJECTS, in order:
 *  1. FACING.  The 2D cross product of the two screen-space edge vectors,
 *     dy2*dx1 - dx2*dy1, must be > 0.  All five floats are written into the
 *     job record for the span filler, so they are struct FIELDS and not
 *     temporaries -- which is why the four integer deltas go through the
 *     `[ebp-4]` fild scratch and straight into the record.
 *  2. CLIPPING.  Each mesh vertex carries a clip word at +0x0c whose low four
 *     bits say which of the four frustum edges it is INSIDE.  The OR of the
 *     three must have all four bits set -- i.e. no single edge has all three
 *     vertices outside it -- otherwise the triangle is dropped.  The AND is
 *     accumulated at the same time and stored in the record but never tested
 *     here: it is the span filler's "trivially accepted, no clipping needed"
 *     flag.  Both masks start from `and = 0xff`, `or = 0`, and VC6 folds the
 *     first `and 0xff` into the first vertex's load.
 *
 * A surviving triangle has its three vertices copied into the job's own three
 * 0x1c-byte slots -- y, x, shade, z into +0x04, +0x08, +0x0c, +0x10 -- and
 * Raster_SubmitPoly(3, &job) queues it.  `tri[3] = tri[0]` closes the ring
 * for the filler; the vertex scratch is four slots wide even though only
 * three are ever written.
 *
 * WHAT CLOSED (all measured):
 *  * The four edge deltas are written with the vertices SUBSCRIPTED
 *    (`m->verts[tri[1]].x - m->verts[tri[0]].x`), with the va/vb/vc pointers
 *    the later clip test uses declared AFTER them.  Worth 141 -> 23: with the
 *    pointers first VC6 folds every subtraction into a memory operand and
 *    comes out two instructions short, where the original loads the first
 *    vertex's x and y into a register first.  The same rewrite of the emit
 *    loop (`m->verts[tri[k]].y` instead of an `s` pointer) is worth another 2
 *    and fixes the order of its two preheader `lea`s.
 *  * The three `job.v[k] = &v[k]` stores are written in NATURAL 0,1,2 order;
 *    VC6 emits them 1,0,2, which is the original.  Writing them in the
 *    original's apparent 1,0,2 order gives 0,1,2 and costs 4.  Adjacent
 *    stores are reordered, so read the source order off the OUTPUT order
 *    inverted, not off the disassembly.
 *  * NEW (wave fourteen): the index-resolution loop reads the triangle
 *    through a WALKED pointer (`int e = *s++;`) while the destination stays a
 *    SUBSCRIPT (`tri[k] = ...`).  That mixed spelling is worth 17 -> 8.  The
 *    same rule was found from the other side in coaster3d.c's TransformVerts:
 *    two lockstep cursors spelled the SAME way (both walks, or both
 *    subscripts) let VC6 eliminate one induction variable, and spelling one
 *    of them differently keeps both.  Here the original keeps the source and
 *    eliminates the destination, so the mixed form is only half the answer --
 *    but it removes nine of the seventeen.
 * ========================================================================= */
/* Scope G continuation: explicitly share the byte displacement between the
 * walked source and tri. The offset is invariant across the three stores;
 * advance the source after either arm. This reproduces the original
 * sub esi,ecx / [esi+ecx] addressing form, closing all eight mismatches.
 * Integer address arithmetic avoids subtracting pointers to separate arrays.
 * Full-body exact: 173 instructions / 514 bytes, no escaped branches.
 */
// FUNCTION: LEGOLAND 0x004234e0
int Coaster3D_DrawMesh(const MeshDesc* m)
{
    unsigned int t;
    PolyVtx      v[4];
    RasterState  g;
    int          tri[4];
    PolyJob      job;
    int          i;
    int          k;

    Raster_SaveState(&g);
#ifndef LEGOLAND_PORTABLE
    __asm {
        push eax
        push edx
        rdtsc
        mov  t, eax
        pop  edx
        pop  eax
    }
#else
    t = ll_rdtsc();
#endif
    job.v[0] = &v[0];
    job.v[1] = &v[1];
    job.v[2] = &v[2];
    job.kind = 1;
    job.shader = g_span_fillers;
    for (i = 0; i < m->ntris; i++) {
        TrackVtx* va;
        TrackVtx* vb;
        TrackVtx* vc;

        {
            /* The source cursor also addresses tri through one invariant
             * byte offset. Integer addresses preserve the original 32-bit
             * subtraction without subtracting pointers to separate arrays. */
            unsigned int s = (unsigned int)&m->tris[i][0];
            unsigned int diff = (unsigned int)tri - s;

            for (k = 0; k < 3; k++) {
                int e = *(const int*)s;

                if (e & 0x80000000)
                    *(int*)(s + diff) = m->pairs[e & 0x7fffffff][1];
                else
                    *(int*)(s + diff) = m->pairs[e & 0x7fffffff][0];
                s += 4;
            }
        }
        tri[3] = tri[0];
        job.dx1 = (float)(m->verts[tri[1]].x - m->verts[tri[0]].x);
        job.dy1 = (float)(m->verts[tri[1]].y - m->verts[tri[0]].y);
        job.dx2 = (float)(m->verts[tri[2]].x - m->verts[tri[0]].x);
        job.dy2 = (float)(m->verts[tri[2]].y - m->verts[tri[0]].y);
        va = &m->verts[tri[0]];
        vb = &m->verts[tri[1]];
        vc = &m->verts[tri[2]];
        job.area = job.dy2 * job.dx1 - job.dx2 * job.dy1;
        if (job.area > 0.0f) {
            job.and_flags = 0xff;
            job.or_flags = 0;
            job.or_flags |= va->clip;
            job.and_flags &= va->clip;
            job.or_flags |= vb->clip;
            job.and_flags &= vb->clip;
            job.or_flags |= vc->clip;
            job.and_flags &= vc->clip;
            if ((job.or_flags & 0xf) == 0xf) {
                for (k = 0; k < 3; k++) {
                    v[k].sy = m->verts[tri[k]].y;
                    v[k].sx = m->verts[tri[k]].x;
                    v[k].shade = m->verts[tri[k]].shade;
                    v[k].sz = m->verts[tri[k]].z;
                }
                job.tag = m->f00;
                Raster_SubmitPoly(3, &job);
            }
        }
    }
#ifndef LEGOLAND_PORTABLE
    __asm {
        push eax
        push edx
        rdtsc
        sub  eax, t
        mov  t, eax
        pop  edx
        pop  eax
    }
#else
    t = ll_rdtsc() - t;
#endif
    g_stat_c_60f8fc += t;
    Raster_RestoreState(&g);
    return 1;
}

/* The static topology the track mesh is drawn from. */
extern int   g_seam_pairs[24][2];                               /* 0x00613908 */
extern int   g_tri_template[12][3];                             /* 0x00613878 */
extern int   g_mesh_pairs[][2];                                 /* 0x006148b8 */
extern int   g_mesh_tris[][3];                                  /* 0x00612708 */

#define SEAM ((int)0x80000000)

/* =========================================================================
 * 0x00428f00 -- Coaster3D_InitTrackTopology (schoolcar.c's `Coaster3D_InitTrackTopology`).
 *
 * Run ONCE, from schoolcar.c's CoasterSceneInit (0x00428b70), and it builds
 * every static table the two routines above read.  Nothing here depends on
 * the level: it is pure geometry.
 *
 * 1. THE SEAM PAIR TEMPLATE (24 entries at 0x00613908).  Each entry is the
 *    two vertex numbers one topological point has on the two sides of a
 *    seam, and the triangle list indexes it (with bit 31 choosing the half).
 *      [0..4]   {k, k+1}         the five plain ring steps
 *      [5]      {5, 0}           the ring's wrap
 *      [6..15]  {d, d+6} and {d, d+7} for d = 0..4   the tube's quad seams
 *      [16]     {5, 11}, [17] {5, 6}                 the same, wrapped
 *      [18..22] {d+6, d+7}       the far ring's steps
 *      [23]     {11, 6}          the far ring's wrap
 *
 * 2. THE TRIANGLE TEMPLATE (12 triangles at 0x00613878).  Twelve triangles
 *    -- two per quad -- tile the six-sided tube between two rings.  The seam
 *    flag 0x80000000 is ADDED, not OR-ed: VC6 folds `x + 0x80000000` into a
 *    single `lea reg,[reg + 0x7fffffee]`-style constant, which is only legal
 *    for an addition, so the original's source adds it.
 *
 * 3. THE THIRTY INSTANCES.  The coaster draws up to 30 segments, so the
 *    template is stamped out 30 times: pair entry j of instance s becomes
 *    the template's entry plus 6*s (six vertices per ring), and triangle
 *    index j becomes the template's index part plus 18*s with bit 31 carried
 *    through untouched.  Instance 29 -- the LAST -- copies 24 pairs instead
 *    of 18, i.e. it also gets the far ring's own six steps, because there is
 *    no instance 30 to supply them.  That is the whole reason the pair table
 *    has 24 entries and the per-instance stride is 18.
 *
 * 4. THE RING (0x006121c8 and 0x006126d8).  Six points 60 degrees apart
 *    (0x004ab470 is (float)1.0472 = 2*pi/6): the source vertex is
 *    (0, 1.8*cos, 1.8*sin) -- x is zero because x runs ALONG the track and
 *    0x004ab474 is (float)1.8, the tube radius -- and the matching normal is
 *    the bare (cos, sin), which is what Coaster3D_BuildTrackMesh dots with
 *    the view direction to shade each vertex.  The running angle stays in an
 *    x87 register for the whole loop and is popped by the trailing
 *    `fstp st(0)`.
 *
 * NOTE FOR THE CALLERS' TYPES: the ring SOURCE vertex is 12 bytes (three
 * floats), not the 0x14-byte screen vertex; 0x14 is the DESTINATION stride
 * passed to 0x00426250.
 *
 * NEW (wave fourteen): `6 * s` is hoisted into a local used by both pair
 *    stores and `18 * s` is spelled `3 * six` off that local.  Worth 75 -> 53
 *    and two bytes.  Hoisting `18 * s` into its own local instead (86), both
 *    into their own locals (80, the earlier wave's 84 measured again), and
 *    `six * 3` / `six + six + six` (identical to `3 * six`) are all worse or
 *    inert; declaring `eig = 3 * six` as a local rather than spelling it at
 *    the use costs 21.
 * ========================================================================= */
/* Scope G closure (wave sixteen): the whole residual was the REGISTER
 * ALLOCATION of the replication loop, and it is decided by the PAIR loop.
 *
 * Three loop-carried values -- `base`, `out` and `s` -- compete for the one
 * callee-saved register the inner loops leave free (EBX; EDI is `eighteen`,
 * ESI the trip counters, EBP/EAX/ECX/EDX the loop temps).  The original keeps
 * `base` in EBX and homes `out` at [esp+0x10] and `s` at [esp+0x14].  Written
 * with `base` only touched outside the pair loop (a cursor seeded from it,
 * `base += cnt` beside it), VC6's weights rank s > out > base and it homes
 * `base` instead, adding a loop-entry `jmp` -- which is what the two volatile
 * shims (the `s` latch read, the `out` self-read) were compensating for, and
 * the self-read is a scheduling barrier, so the template constant could only
 * land before the `out` reload (index 120) or sink to the preheader's end
 * (123), never in the original's slot after it.
 *
 * THE LEVER: `base` is the pair loop's own ROW INDEX, bumped once per pair
 * (`g_mesh_pairs[base][0] = ...; g_mesh_pairs[base][1] = ...; base++;`
 * inside a counted `for (i = 0; i < cnt; i++)`).  Referenced at depth 2 it
 * outweighs `out` and `s`, wins EBX, and VC6 then ELIMINATES it from the loop
 * by final-value replacement -- `add ebx,esi` in the preheader is `base +=
 * cnt` computed by the compiler, and `dec esi / jne` is the counted `i`
 * reversed into the trip count -- so the loop body is untouched.  With `out`
 * and `s` homed naturally there is no barrier, and the list scheduler's
 * critical-path order puts the template constant right after the `out`
 * reload, closing 120..123 with the PLAIN triangle loop (`for j / for k`,
 * `g_tri_template[j][k++]`, `dst = &out[0][0]; out += 12;`) and a plain
 * `s++` latch: no volatile anywhere, no cast bound.
 *
 * The one companion rule: the seam reads must be INDEXED FROM A LOCAL
 * POINTER ANCHORED AT THE SECOND FIELD (`seam = &g_seam_pairs[0][1]`,
 * `seam[2*i-1]`, `seam[2*i]`).  A local pointer anchors the induction
 * variable at its own address and neither load folds into its add
 * (`mov ebp,[ecx-8]; add ebp,edx`); indexing the global `g_seam_pairs[i][k]`
 * directly anchors the variable at the last use and folds that bare load
 * (`mov ebp,edx; add ebp,[ecx-8]`, 2 mismatches); anchoring at the first
 * field puts the variable at 0x613908 and costs the displacement byte
 * (581B).  Both cursors sit on the second field in the original for the same
 * reason.  Ruled out on the way: `base++` in a `do/while (--cnt)` (no
 * final-value replacement, `inc ebx` stays in the loop), an explicit walking
 * cursor inside the counted `for` (a guard and a spill), `i++, base++` in
 * the for clause (the IV registers swap), temps or derefs on the loads
 * (folded back), and every address-taking, aggregate and volatile spelling
 * of `out`/`s` recorded in the earlier waves.
 */
// FUNCTION: LEGOLAND 0x00428f00
void Coaster3D_InitTrackTopology(void)
{
    int    a;
    int    d;
    int    n;
    int    p;
    int    q;
    int    t;
    int    s;
    int    j;
    int    k;
    int    base;
    int  (*out)[3];
    float  ang;

    t = 0;
    {
        int* row = &g_seam_pairs[0][1];
        for (a = 0; a <= 4; a++, row += 2) {
            row[-1] = a;
            row[0] = a + 1;
        }
    }
    g_seam_pairs[5][0] = a;
    g_seam_pairs[5][1] = 0;

    n = 6;
    for (d = 0; d <= 4; d++) {
        g_seam_pairs[n][0] = d;
        g_seam_pairs[n][1] = d + 6;
        n++;
        g_seam_pairs[n][0] = d;
        g_seam_pairs[n][1] = d + 7;
        n++;
    }
    g_seam_pairs[n][0] = d;
    g_seam_pairs[n][1] = d + 6;
    n++;
    g_seam_pairs[n][0] = d;
    g_seam_pairs[n][1] = 6;
    n++;
    for (d = 0; d <= 4; d++) {
        g_seam_pairs[n][0] = d + 6;
        g_seam_pairs[n][1] = d + 7;
        n++;
    }
    g_seam_pairs[n][0] = d + 6;
    g_seam_pairs[n][1] = 6;

    p = 6;
    for (q = 18; q < 23; q++) {
        g_tri_template[t][0] = (q - 18) + SEAM;
        g_tri_template[t][1] = p + 1;
        g_tri_template[t][2] = (p + 2) + SEAM;
        t++;
        g_tri_template[t][0] = p;
        g_tri_template[t][1] = q;
        g_tri_template[t][2] = (p + 1) + SEAM;
        t++;
        p += 2;
    }
    g_tri_template[t][0] = 5 + SEAM;
    g_tri_template[t][1] = p + 1;
    g_tri_template[t][2] = 6 + SEAM;
    t++;
    g_tri_template[t][0] = p;
    g_tri_template[t][1] = q;
    g_tri_template[t][2] = (p + 1) + SEAM;

    base = 0;
    out = g_mesh_tris;
    for (s = 0; s < 30; s++) {
        int cnt;
        int six = 6 * s;
        int eighteen = 18 * s;

        cnt = 18;
        if (s == 29)
            cnt = 24;
        {
            int i;
            /* The seam table is read through a pointer anchored at its
             * SECOND field, and `base` is the row index, bumped per pair. */
            const int* seam = &g_seam_pairs[0][1];
            for (i = 0; i < cnt; i++) {
                g_mesh_pairs[base][0] = seam[2 * i - 1] + six;
                g_mesh_pairs[base][1] = seam[2 * i] + six;
                base++;
            }
        }
        {
            int* dst = &out[0][0];
            out += 12;
            for (j = 0; j < 12; j++)
                for (k = 0; k < 3;) {
                    int value = g_tri_template[j][k++];
                    *dst++ = ((value & 0x7fffffff) + eighteen) | (value & SEAM);
                }
        }
    }

    ang = 0.0f;
    for (j = 0; j < 6; j++) {
        float cs;
        float sn;

        cs = (float)cos(ang);
        g_ring_src[j].x = 0.0f;
        g_ring_src[j].y = cs * 1.8f;
        sn = (float)sin(ang);
        g_ring_src[j].z = sn * 1.8f;
        g_ring_normals[j].e0 = cs;
        g_ring_normals[j].e1 = sn;
        ang += 1.0472f;
    }
}
