/* LEGOLAND -- the DRIVING SCHOOL car's heading rotation, plus five coaster
 * internals schoolcar.c reaches through externs: the route reset, the save
 * side of a route position, the z-buffer region clear, the arc car class's
 * two off-centre rail-position hooks, and the module's `.lms` model loader.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere) and follow schoolcar.c / schoolcar2.c / schoolcar3.c /
 * schoolcar6.c / coaster3d.c / coaster5.c.
 *
 * NAMES AND EXTERN TYPES vs. schoolcar.c, which reaches five of these through
 * `Sub_*` externs (its externs are left alone -- each is a caller-side lever
 * and that file matches as it stands):
 *   0x0041e500  Route_Reset            same name, same type.
 *   0x00420640  LoadLmsModel           its `void* LoadLmsModel(const char*)`.
 *   0x00421be0  CoasterArc_GetPosRail0 its `void CoasterArc_GetPosRail0(void)` -- declared
 *   0x00421c30  CoasterArc_GetPosRail2 its `void CoasterArc_GetPosRail2(void)`    only to
 *                                      fill the class table, so untyped.
 *   0x00423480  ZBuffer_ClearRegion    its `void ZBuffer_ClearRegion(CoasterRegion*)`.
 *   0x00426ec0  SaveRoutePos           its `void SaveRoutePos(RoutePos*,
 *                                      unsigned char* out)`; the out block is
 *                                      really the 0x0c-byte record below.
 * 0x00401000 keeps schoolcar2.c / schoolcar3.c's `RotateByHeading` name and
 * their exact `CarPos (int, int, int)` prototype.
 */

#include <math.h>

#pragma intrinsic(sin, cos)

/* ==========================================================================
 * 0x00401000 -- rotate a {forward, sideways} offset into a road HEADING.
 *
 * schoolcar2.c's manoeuvre builders are the caller's side: each road
 * manoeuvre is laid out in the car's OWN frame -- `fwd` along the direction
 * of travel, `side` across it -- and this turns that pair into map deltas for
 * the car's 8-way heading (`SchoolCar +0xba`, 0..7). Only the four CARDINAL
 * headings have a case:
 *
 *     dir 1 (north)   { side, -fwd }
 *     dir 3 (east)    { fwd,   side }        <- the identity
 *     dir 5 (south)   { -side, fwd  }
 *     dir 7 (west)    { -fwd, -side }
 *
 * i.e. a quarter turn per two heading steps, with EAST as the frame the
 * offsets are written in. The pair comes back in edx:eax as an 8-byte struct.
 *
 * ORIGINAL BUG, reproduced: the switch has no `default` and the result local
 * is never initialised, so the three DIAGONAL headings (2, 4, 6) and every
 * value outside 1..7 return the two UNINITIALISED dwords of the function's
 * own 8-byte frame (`mov edx,[esp+4] / mov eax,[esp]`). The `sub esp,8`
 * exists only to be read back on that path. The callers only ever pass a
 * cardinal heading, which is how it survived.
 *
 * CODEGEN: the jump table's BLOCK ORDER is the source's case order (1, 3, 5,
 * 7, then the fall-out), and the table entries for 2, 4 and 6 point at the
 * uninitialised block. VC6 forwards each real arm's stores straight into the
 * return registers -- there is no store to the frame anywhere.
 * ======================================================================== */
typedef struct CarPos { int x; int y; } CarPos;

// FUNCTION: LEGOLAND 0x00401000
CarPos RotateByHeading(int fwd, int side, int dir)
{
    CarPos r;

    switch (dir) {
    case 1: r.x = side;  r.y = -fwd;  break;
    case 3: r.x = fwd;   r.y = side;  break;
    case 5: r.x = -side; r.y = fwd;   break;
    case 7: r.x = -fwd;  r.y = -side; break;
    }
    return r;                   /* uninitialised on 2 / 4 / 6 -- the original */
}

/* ==========================================================================
 * 0x00421be0 and 0x00421c30 -- the ARC car class's two off-centre RAIL
 * position hooks.
 *
 * schoolcar.c's CarClassTablesInit fills three eight-slot class tables at
 * 0x004dd5e0. coaster3d.c already names the shape: the slots are PAIRS
 * {get_pos, get_dir} indexed by a rail number, so each class is
 *
 *     slot 0/1  rail 0     slot 2/3  rail 1 (the centre line)
 *     slot 4/5  rail 2     slot 6    the tessellation parameters
 *                          slot 7    the object's `up` vector
 *
 * Class 2 (slots 8..15) is a STRAIGHT segment -- 0x00421a10 is
 * `base + t*dir + offset`, 0x00421a40 the same with the offset SUBTRACTED and
 * 0x004219c0 the centre with no offset at all. Class 3 (slots 16..23), the
 * one here, is a circular ARC over the same three rails:
 *
 *     struct ArcObj {
 *         float a[3];      // +0x00  the arc's first in-plane axis
 *         float b[3];      // +0x0c  its second
 *         float c[3];      // +0x18  the arc's centre; c[2] is the whole
 *                          //        object's constant z
 *         float r0;        // +0x24  rail 0's radius scale
 *         float r2;        // +0x28  rail 2's radius scale
 *     };
 *
 *     pos(t) = cos(t)*r*a + sin(t)*r*b + c        (z taken from c[2])
 *
 * with r = +0x24 for rail 0 and r = +0x28 for rail 2; the CENTRE rail
 * (0x00421b40) is the same body with NO radius multiply, so `a` and `b`
 * already carry the centre-line radius and the two scale fields are the
 * off-centre rails' relative radii. The shared direction hook (0x00421b90,
 * slots 17 and 19) is the derivative WITHOUT the radius --
 * `-sin(t)*a + cos(t)*b`, z forced to 0 -- which is what makes the two
 * scales pure radii. Slot 22 (0x00421c80) hands back six parameters
 * 0, pi/10 .. pi/2 (a quarter circle in six samples) and slot 23 the
 * constant `up` {0,0,1}: the arc is always swept about the world z axis.
 *
 * SO THESE TWO ARE TWINS -- one source compiled twice, differing ONLY in the
 * radius field (+0x24 against +0x28); their 31 instructions are otherwise
 * identical index for index. (Diffed before writing either, per the rule
 * that equal size is not evidence.)
 *
 * CODEGEN, all of it free once the types are right:
 *  * `sin` and `cos` are the x87 INTRINSICS (`fsin` / `fcos`), and the two
 *    scaled terms live on the x87 stack for the whole loop -- popped by the
 *    two trailing `fstp st(0)`. Neither needs a stack home.
 *  * The two-iteration loop runs a record cursor anchored at `c` (+0x18) and
 *    addresses `a` and `b` as `[cur-0x18]` / `[cur-0xc]`: with one reference
 *    each the recorded tie-break takes the LAST field, and offset 0 can
 *    never win from a `->`.
 *  * The z component is an integer `mov` pair, not `fld/fstp` -- VC6 copies a
 *    float that is never computed on through a scratch register.
 * ======================================================================== */
typedef struct Vec3f { float v[3]; } Vec3f;

typedef struct ArcObj {
    float a[3];                 /* +0x00 */
    float b[3];                 /* +0x0c */
    float c[3];                 /* +0x18  centre; c[2] is the object's z */
    float r0;                   /* +0x24  rail 0's radius */
    float r2;                   /* +0x28  rail 2's radius */
} ArcObj;                       /* 0x2c */

// FUNCTION: LEGOLAND 0x00421be0
void CoasterArc_GetPosRail0(const ArcObj* o, float t, Vec3f* out)
{
    float s = (float)sin(t) * o->r0;
    float c = (float)cos(t) * o->r0;
    int   i;

    for (i = 0; i < 2; i++)
        out->v[i] = c * o->a[i] + s * o->b[i] + o->c[i];
    out->v[2] = o->c[2];
}

// FUNCTION: LEGOLAND 0x00421c30
void CoasterArc_GetPosRail2(const ArcObj* o, float t, Vec3f* out)
{
    float s = (float)sin(t) * o->r2;
    float c = (float)cos(t) * o->r2;
    int   i;

    for (i = 0; i < 2; i++)
        out->v[i] = c * o->a[i] + s * o->b[i] + o->c[i];
    out->v[2] = o->c[2];
}

/* ==========================================================================
 * 0x00426ec0 -- PACK a route position into the save blob.
 *
 * schoolcar.c's SaveCoasterRouteState calls this on the route's own
 * `RoutePos` and gives it the 12 bytes at +0x14 of the car save record. A
 * RoutePos is {piece, object, world position}, and only the first two are
 * saved -- the position is re-derived on load (coaster.c's RestoreRoutePos,
 * 0x00426f10, is exactly this function's inverse):
 *
 *     +0x00, +0x04   the piece, packed by 0x00426e80 (class id + map square)
 *     +0x08          WHICH sub-object of that piece, as an INDEX
 *
 * The index is the whole point. `GetTrackNodeWorldPos` (0x0041cff0) hands
 * back the piece's FIRST render object, and those objects are chained through
 * +0x50; this counts how many links it takes to reach the one the route is
 * actually riding. A null link ends the walk with whatever count it had, so
 * an object that is not on the chain saves as the chain's length -- the
 * original makes no attempt to signal that.
 *
 * The Vec3f the position query needs is the function's only local, and the
 * two calls' arguments share ONE `add esp,0x10`.
 * ======================================================================== */
typedef struct TrackNode TrackNode;

/* The piece's render objects, chained through +0x50. */
typedef struct TrackObj {
    unsigned char    pad00[0x50];
    struct TrackObj* next;      /* +0x50 */
} TrackObj;

typedef struct RoutePos {
    TrackNode* node;            /* +0x00 */
    TrackObj*  obj;             /* +0x04 */
    Vec3f      pos;             /* +0x08 */
} RoutePos;                     /* 0x14 */

/* coaster4.c's CoasterNodeRef with the sub-object index appended. */
typedef struct RoutePosSave {
    int node;                   /* +0x00 */
    int key;                    /* +0x04 */
    int index;                  /* +0x08 */
} RoutePosSave;                 /* 0x0c */

extern void      WriteCoasterNodeRef(TrackNode* n, RoutePosSave* out); /* 0x00426e80 */
extern TrackObj* GetTrackNodeWorldPos(TrackNode* n, Vec3f* out);       /* 0x0041cff0 */

// FUNCTION: LEGOLAND 0x00426ec0
void SaveRoutePos(RoutePos* pos, RoutePosSave* out)
{
    Vec3f     tmp;
    TrackObj* p;
    int       n = 0;

    WriteCoasterNodeRef(pos->node, out);
    p = GetTrackNodeWorldPos(pos->node, &tmp);
    while (p != pos->obj) {
        p = p->next;
        if (!p)
            break;
        n++;
    }
    out->index = n;
}

/* ==========================================================================
 * 0x00423480 -- clear one REGION of the coaster's 16-bit z-buffer.
 *
 * schoolcar.c's Coaster3D_EndFrame runs this over every region whose clip
 * code is non-zero before it walks the command buffer; the whole-buffer
 * `memset` is the other arm of that same `if`. So a region IS a dirty
 * rectangle: `rect` at +0x00 is {x0, y0, x1, y1} in pixels and both bounds
 * are INCLUSIVE (`jle` on each latch), and clearing means writing 0 -- the
 * same value ZBuffer_FillPoly paints, which is what makes the fill a
 * coverage clear rather than a colour.
 *
 * The buffer is schoolcar6.c's pair: base at 0x004b5b24, pitch in PIXELS at
 * 0x004b5b28.
 *
 * CODEGEN: both bounds are STRUCT FIELDS, so both loops reload them every
 * iteration and count UP (the recorded local-versus-field rule); the pitch
 * global is likewise re-read in the outer latch, because the stores may
 * alias it. `push ebx` sinks into the outer loop's preheader -- ebx is live
 * only inside it. No `__asm` signatures here: this is ordinary VC6 output,
 * unlike its neighbour ZBuffer_FillPoly at 0x00423350.
 * ======================================================================== */
typedef struct CoasterRegion {
    int                   x0;   /* +0x00 */
    int                   y0;   /* +0x04 */
    int                   x1;   /* +0x08 */
    int                   y1;   /* +0x0c */
    int                   code; /* +0x10  clip code, 15 = fully inside */
    struct CoasterRegion* f14;  /* +0x14 */
    struct CoasterRegion* next; /* +0x18 */
} CoasterRegion;

extern short* g_zb_base;                                        /* 0x004b5b24 */
extern int    g_zb_pitch;                                       /* 0x004b5b28 */

// FUNCTION: LEGOLAND 0x00423480
void ZBuffer_ClearRegion(const CoasterRegion* r)
{
    short* row = g_zb_base + g_zb_pitch * r->y0;
    int    x;
    int    y;

    for (y = r->y0; y <= r->y1; y++) {
        short* p = row + r->x0;

        for (x = r->x0; x <= r->x1; x++) {
            *p = 0;
            p++;
        }
        row += g_zb_pitch;
    }
}

/* ==========================================================================
 * 0x0041e500 -- RESET a coaster route to its start.
 *
 * schoolcar.c's CreateCoasterRoute calls this as the last step of building
 * the route object, and it is what puts the train on the track for the first
 * time. The coaster RECORD carries its own three-hook table INLINE at +0x10c
 * (schoolcar.c's InstallCastleHooks writes it at 0x00829bec, which is
 * g_castle + 0x10c), and hook 0 (0x00424850) is the START QUERY:
 *
 *     *obj = the fixed station render object (0x006103a8)
 *     *a   = 0.1f, the starting curve parameter
 *     *pos = MapSquareToWorld(rec->ring.sq, (float)g_4b5b50)
 *
 * so the station is the record's own SENTINEL node's map square. This
 * assembles those three answers into a RoutePos whose `node` is the sentinel
 * itself, hands the whole thing to 0x0041d950 (which stores route +0x24 and
 * +0x0c and then re-places every car of the train behind it), stamps the
 * start time, and seeds the route's +0x28 scalar with one step of the
 * physics:
 *
 *     +0x28 = SUM(car +0xc4)  +  SUM(car +0xc0) * dt * dt * 0.5   , dt = 0.1
 *
 * -- v + 1/2 a t^2 over the whole train, the same integration the RK4 solver
 * descriptor runs per frame. State 8 is the "reset, not yet running" flag.
 *
 * CODEGEN: the RoutePos is ONE 0x14-byte local (the whole `sub esp,0x14`),
 * and the hook's scalar out-parameter `a` takes the DEAD `rt` argument slot
 * -- the recorded "an out-param local declared in the block where it is used
 * takes a dead argument slot", with `rt` root-copied into esi at entry. `a`
 * is then handed on as a RAW DWORD (`mov ecx,[esp+0x2c] / push ecx`), so
 * BOTH prototypes here spell it `int` even though it is a float; a `float`
 * local would cost an `fld`/`fstp` pair. schoolcar.c's sibling 0x0041da10
 * (PositionRouteCars) records the same value as `float` on the CALLEE side,
 * where the float spelling is what its own match needs -- the two are
 * deliberately not aligned. All four calls' arguments share one `add esp,0x28`.
 * ======================================================================== */
typedef struct CoasterRec  CoasterRec;
typedef struct RouteNode { unsigned char pad00[0xec]; } RouteNode;

struct CoasterRec {
    int           state;        /* +0x00 */
    unsigned char ring[0x50];   /* +0x04  the list sentinel, a TrackNode */
    unsigned char pad54[0x10c - 0x54];
    void        (*get_start)(CoasterRec* rec, TrackObj** obj, int* a,
                             Vec3f* pos);                       /* +0x10c */
};

typedef struct CoasterRoute {
    int           state;        /* +0x00  8 = reset */
    int           started;      /* +0x04  game-clock stamp */
    int           deadline;     /* +0x08 */
    RoutePos      pos;          /* +0x0c */
    unsigned char pad20[4];
    float         f24;          /* +0x24 */
    float         speed;        /* +0x28 */
    unsigned char pad2c[0x6c - 0x2c];
    CoasterRec*   owner;        /* +0x6c */
    RouteNode     head;         /* +0x70  embedded car-list sentinel */
} CoasterRoute;                 /* 0x15c */

/* `a` is a float in the original; it is spelled `int` here so the caller
 * copies it as a raw dword (see the codegen note above). */
extern void  Route_SetTrainAt(CoasterRoute* rt, int a,
                              const RoutePos* at);              /* 0x0041d950 */
extern int   GetGameTimer(void);                                /* 0x00499430 */
extern float Route_SumCarVelocity(CoasterRoute* rt);            /* 0x0041dae0 */
extern float Route_AccelDistance(CoasterRoute* rt, float dt);   /* 0x0041ddb0 */

// FUNCTION: LEGOLAND 0x0041e500
void Route_Reset(CoasterRoute* rt)
{
    CoasterRec* rec = rt->owner;
    RoutePos    at;
    int         a;              /* a float, homed in the dead `rt` slot */

    rt->state = 8;
    at.node = (TrackNode*)rec->ring;
    rec->get_start(rec, &at.obj, &a, &at.pos);
    Route_SetTrainAt(rt, a, &at);
    rt->started = GetGameTimer();
    rt->speed = Route_SumCarVelocity(rt);
    rt->speed += Route_AccelDistance(rt, 0.1f);
}

/* ==========================================================================
 * 0x00420640 -- load one `.lms` MODEL from the coaster's data set.
 *
 * schoolcar.c's LoadCoasterData drives three parallel tables through three
 * of these loaders, one per file extension, all built on 0x00420550 (the
 * archive read) and all formatting their name into a 0x100-byte stack
 * buffer with `wsprintfA`:
 *
 *     0x00420640   "%s.lms"   -> g_coaster_tab_a  (0x004d8a40)   the MESH
 *     0x004206d0   "%s.lfm"   -> g_coaster_tab_b  (0x004d8abc) + a second
 *                                table at 0x004d8b34 (an out-parameter)
 *     0x00420750   "%s.ltx"   -> g_coaster_tab_c  (0x004d89c8)   the TEXTURE
 *
 * The `.lms` file is loaded as ONE self-contained block whose internal
 * pointers are stored as OFFSETS FROM THE BLOCK BASE; this is the relocation
 * pass. Six of them are fixed up, +0x0c, +0x14, +0x10, +0x18, +0x20, +0x28 --
 * in that order, which VC6 pairs (0x0c,0x14), (0x10,0x18), (0x20,0x28) and
 * emits as three load/load/add/add/store/store groups. `.ltx` needs no
 * fix-ups at all, which is why 0x00420750 merges its two `add esp` into one.
 *
 * CODEGEN: the failure arm must be the EXILED one -- `if (m) { ...; return m;
 * } return 0;` gives the original's `je <end>` with the fix-ups inline and a
 * materialised `xor eax,eax` at the end. Written `if (!m) return 0;` VC6
 * knows eax is already zero on that edge, emits a bare `ret` and loses the
 * `xor` (36 instructions against 37).
 * ======================================================================== */
__declspec(dllimport) int __cdecl wsprintfA(char* out, const char* fmt, ...);

typedef struct LmsModel {
    unsigned char pad00[0x0c];
    char*         p0c;          /* +0x0c */
    char*         p10;          /* +0x10 */
    char*         p14;          /* +0x14 */
    char*         p18;          /* +0x18 */
    int           f1c;
    char*         p20;          /* +0x20 */
    int           f24;
    char*         p28;          /* +0x28 */
} LmsModel;

extern void* Sub_420550(const char* path, void** out2);         /* 0x00420550 */

// FUNCTION: LEGOLAND 0x00420640
LmsModel* LoadLmsModel(const char* name)
{
    char      buf[0x100];
    LmsModel* m;

    wsprintfA(buf, "%s.lms", name);
    m = (LmsModel*)Sub_420550(buf, 0);
    if (m) {
        m->p0c = (char*)m + (int)m->p0c;
        m->p14 = (char*)m + (int)m->p14;
        m->p10 = (char*)m + (int)m->p10;
        m->p18 = (char*)m + (int)m->p18;
        m->p20 = (char*)m + (int)m->p20;
        m->p28 = (char*)m + (int)m->p28;
        return m;
    }
    return 0;
}
