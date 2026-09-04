/* LEGOLAND -- the CAROUSEL, BALLOONZ and RESTAURANT 2 per-tick state machines
 * (the +0xa8 "update" callback slot) and the CAROUSEL's private helpers.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere).
 *
 * ==========================================================================
 * WHICH RIDE, WHICH SLOT
 * ==========================================================================
 * screen.c's SetCustomCallbacks (0x00452c20) maps a class NAME to a callback
 * set; joust.c documents what a set is.  Read back out of that table the
 * three big functions in this file are all the SAME slot of three different
 * classes:
 *
 *   addr        class          slot   ridecb1.c's reading   this file
 *   0x0042c820  CAROUSEL       +0xa8  per-rider tick        Carousel_Tick
 *   0x0042aa90  BALLOONZ       +0xa8  per-rider tick        Balloonz_Tick
 *   0x0042fbb0  RESTAURANT 2   +0xa8  per-rider tick        Restaurant2_Tick
 *
 * ridecb1.c recovered the shape of the slot and documented all three from the
 * disassembly without reconstructing them; this file reconstructs them and
 * the CAROUSEL's and BALLOONZ's private helpers, and corrects the record
 * layouts where the disassembly disagrees with that first reading.
 *
 * State (tools/audit.py):
 *
 *   0x0042cd20  Carousel_PickSeat          25/25    [OK]
 *   0x0042bc90  Carousel_StartRide         27/27    [OK]
 *   0x0042c800  Carousel_TickInstances     12/12    [OK]
 *   0x0042aa60  Balloonz_CarAtPlatform     17/17    [OK]
 *   0x0042bc60  Carousel_FindRec           18/18    [OK]
 *   0x0042a980  Balloonz_FindRec           18/18    [OK]
 *   0x0042fbb0  Restaurant2_Tick          658/658, 2193/2193 bytes      [OK]
 *   0x0042c820  Carousel_Tick             378/378 insns, 239 mismatches [WIP]
 *   0x0042aa90  Balloonz_Tick             637/637 insns,  78 mismatches [WIP]
 *
 * All three big handlers reproduce the original's instruction COUNT and block
 * layout exactly; RESTAURANT 2 is byte-exact, and what is left in the other
 * two is a register or frame permutation, described in the note above its
 * marker.  The levers that closed RESTAURANT 2 are written out above its
 * body and several of them transfer to any +0xa8 handler.
 *
 * A +0xa8 handler is called with the class's LLIDB element, once per frame,
 * by the class walk at 0x0045b5e2 (gated on ObjDef->flags & 0x20).  Its body
 * is always:
 *
 *     item = elem->data;                       // the shared ObjDef payload
 *     for (r = item->riders; r; r = next) {    // the CLASS-WIDE rider list
 *         next = r->next;  b = r->bloke;
 *         key  = (MapSquare*)&r->ride_id;      // the packed {x,y} of the
 *                                              // instance this rider is on
 *         rec  = <Class>_FindRec(key);         // that instance's own record
 *         if (!rec) return;                    //  <-- aborts the WHOLE walk
 *         tx = item->base_x + key->bx;
 *         ty = key->by + item->base_y;
 *         if (b->state == 0)                   // idle at the low-level AI
 *             switch (b->action) { ... }       // one step per frame
 *     }
 *     <Class>_TickInstances();                 // then the machines themselves
 *
 * The `if (!rec) return` is an original quirk worth repeating: one rider
 * standing on a square whose record has gone stops every later rider on the
 * class from being ticked that frame AND skips the instance tick.  It is
 * reproduced here (it is a `return`, not a `continue`).
 *
 * ==========================================================================
 * HOW A RIDER IS PUT ON A RIDE IN 3D  (recovered here)
 * ==========================================================================
 * Three of the state machines below share one "mount" sequence, and it is the
 * clearest statement in the game of how the 2D map and the 3D people meet:
 *
 *   1. screen = GetScreenCoordsForObject(key, item)  -- where the BUILDING is
 *      drawn.
 *   2. The rider's own world position is projected with the live isometric
 *      transform: sx = (wx - wy) * tile_w >> 9, sy = (wx + wy) * tile_h >> 9,
 *      biased by the map render origin (lpConfig +0x20/+0x22) and the current
 *      scroll (Get_XScroll / Get_YScroll).
 *   3. The ride's own rider offset (a per-class {dx,dy} pair) is subtracted
 *      HALVED, and then the building's screen origin is subtracted, leaving
 *      the rider's position RELATIVE to the building.
 *   4. That is doubled (the 3D layer works in half-pixels) and handed to
 *      NewBNVPath as the path's origin.
 *   5. The person is given the ride's z-sprite (Person3D +0x2c), told the ride
 *      is driving it (+0x30 = 1) and a depth from GetUnitDepth.
 *
 * So a ride does not move the bloke: it hands the bloke's 3D model a canned
 * BNV path and lets UpdateBlokeFromBNVPath play it back.  Person3D +0x30 is
 * the "the ride owns this model" flag and is cleared, together with the
 * z-sprite, when the rider gets off -- at which point UnAdjustBlokePosition +
 * ScreenToMapRef convert the 3D model's screen position back into a map
 * reference so the bloke can walk away.
 *
 * ==========================================================================
 * CAROUSEL RECORD  (list head 0x006160c4)
 * ==========================================================================
 * ridecb1.c read this record from the call sites it could see; the four
 * helpers below pin it down:
 *
 *   +0x00 CarouselRec* next
 *   +0x04 u16          square       the packed {x,y} it was built on
 *   +0x06 char         boarded      riders that have sat down this cycle
 *   +0x07 char         aboard       riders still on the ride
 *   +0x08 char         frame        machine animation phase (1 = turning)
 *   +0x0c int          flags        bit 0 = running, bit 0x4000 = accepting
 *   +0x14 int          f14          spin counter, reset when the ride starts
 *   +0x18 u8           visitors     bumped once per rider that claims it
 *   +0x1c int          timer        180 frames, set when a rider claims it
 *   +0x20 u8           seat[]       one byte per seat, 0 = free
 *
 * Seats are handed out ONE-BASED in the bloke (`b->seat`) and zero-based in
 * the record, which is why the release in action 13 indexes `seat[b->seat-1]`
 * -- the original folds that into a `+0x1f` displacement.
 * ========================================================================= */

/* CRT.  memcmp is expanded inline by /O2 (see Carousel_FindRec). */
extern int memcmp(const void* a, const void* b, unsigned int n);   /* CRT intrinsic */

/* ---- shared geometry ---------------------------------------------------- */

/* An 8-byte {x,y} pair returned in eax:edx (legoland.h's Offset). */
typedef struct Offset {
    int ox;
    int oy;
} Offset;

/* An {x,y} pair in 24.8 world units, passed and returned by value. */
typedef struct Pos {
    int x;
    int y;
} Pos;

/* The origin NewBNVPath is handed is a THREE-int vector, now measured rather
 * than guessed: following Carousel_Tick's control flow (jump table included)
 * and tracking esp puts `pos` at entry-0x18..-0x0d and `pos2` at
 * entry-0x0c..-0x01, i.e. exactly TWELVE bytes each with the third int
 * reserved and never written.  (A 16-byte Vec3 also reproduces that function's
 * 0x3c frame, but it would make the two origins overlap, so it is wrong.) */
typedef struct Vec3 {
    int x;
    int y;
    int z;
} Vec3;

/* A placed object's map square, packed as two bytes (math3d.c's MapObject). */
typedef struct MapSquare {
    unsigned char bx;          /* +0x00 */
    unsigned char by;          /* +0x01 */
} MapSquare;

/* The map/render config record: lpConfig @ 0x004bcbf4.  +0x20/+0x22 are the
 * map's render origin in screen pixels. */
typedef struct MapConfig {
    unsigned char  pad00[0x20];
    unsigned short ox;         /* +0x20 */
    unsigned short oy;         /* +0x22 */
} MapConfig;

/* ---- the people --------------------------------------------------------- */

typedef struct Person3D {
    unsigned char pad00[0x1c];
    Offset        screen;      /* +0x1c where the model is drawn */
    Offset        local;       /* +0x24 its own offset pair */
    void*         zsprite;     /* +0x2c the ride's depth sprite while riding */
    int           driven;      /* +0x30 1 while a ride positions the model */
    int           f34;         /* +0x34 */
    int           f38;         /* +0x38 */
    float         depth;       /* +0x3c */
} Person3D;

typedef struct Bloke {
    unsigned char  pad00[4];
    Person3D*      person;     /* +0x04 its 3D model record */
    unsigned char  pad08[0x0e - 0x08];
    unsigned short state;      /* +0x0e low-level AI state (0 = idle) */
    unsigned char  pad10[0x24 - 0x10];
    Pos            target;     /* +0x24 walk target, 24.8 */
    unsigned char  pad2c[0x35 - 0x2c];
    unsigned char  b35;        /* +0x35 which 3D pose set the ride wants */
    unsigned char  seat;       /* +0x36 seat index, ONE-BASED */
    unsigned char  band;       /* +0x37 occlusion band inside a building */
    unsigned char  pad38[0x3c - 0x38];
    short          ride_dx;    /* +0x3c per-rider offset inside a ride */
    short          ride_dy;    /* +0x3e */
    unsigned char  pad40[0x44 - 0x40];
    unsigned short saved_speed;/* +0x44 walk speed parked over a sit-down */
    unsigned short f46;        /* +0x46 */
    unsigned char  pad48[0x54 - 0x48];
    void*          bnvpath;    /* +0x54 the BNV path being played back */
    int            f58;        /* +0x58 ride-specific countdown */
    int            wait;       /* +0x5c countdown ticks */
    unsigned char  action;     /* +0x60 this ride's state-machine step */
    unsigned char  pad61;
    unsigned short flags62;    /* +0x62 8 = on this ride, 0x80 = riding,
                                *       0x100 = sitting */
    unsigned char  pad64[4];
    Pos            world;      /* +0x68 world position, 24.8 */
    unsigned short f70;        /* +0x70 */
    unsigned char  dir;        /* +0x72 */
    unsigned char  new_dir;    /* +0x73 */
    unsigned char  b74;        /* +0x74 the frame the ride holds it on */
    unsigned char  pad75[0x7f - 0x75];
    unsigned char  speed;      /* +0x7f animation speed */
    unsigned char  pad80[0x98 - 0x80];
    unsigned char  path[0x14]; /* +0x98 CalcMoveLine scratch */
} Bloke;

/* A rider slot on the class-wide list at ObjDef +0xcc. */
typedef struct RiderNode {
    struct RiderNode* next;    /* +0x00 */
    struct RiderNode* prev;    /* +0x04 */
    Bloke*            bloke;   /* +0x08 */
    unsigned short    ride_id; /* +0x0c the packed map square it is using */
    unsigned short    pad0e;
    void*             owner;   /* +0x10 */
} RiderNode;

/* ---- the class element and its ObjDef, seen from the ride side ---------- */

typedef struct RideObject {
    unsigned char  pad00[0x0c];
    int            base_x;     /* +0x0c the class's base map square */
    int            base_y;     /* +0x10 */
    unsigned char  pad14[0x2e - 0x14];
    short          capacity;   /* +0x2e riders per car */
    unsigned char  pad30[0xcc - 0x30];
    RiderNode*     riders;     /* +0xcc the class-wide rider list */
} RideObject;

typedef struct RideElem {
    char*        name;         /* +0x00 */
    char*        image;        /* +0x04 */
    unsigned int type_flags;   /* +0x08 */
    RideObject*  data;         /* +0x0c */
} RideElem;

/* money.c's SoundSource; kind 2 = "a map square".  +0x04 is never initialised
 * for that kind (reproduced -- the original leaves stack junk there). */
typedef struct RideSoundSource {
    int kind;                  /* +0x00 */
    int f04;                   /* +0x04 */
    int x;                     /* +0x08 */
    int y;                     /* +0x0c */
} RideSoundSource;

/* ---- shared engine entry points ----------------------------------------- */
extern Offset GetScreenCoordsForObject(MapSquare* sq, RideObject* item); /* 0x00442cc0 */
extern void   GetTileDimensions(int* out_w, int* out_h);             /* 0x00460540 */
extern short  Get_XScroll(void);                                     /* 0x004615f0 */
extern short  Get_YScroll(void);                                     /* 0x00461600 */
extern float  GetUnitDepth(float near_z, float far_z);               /* 0x0044de50 */
extern void*  NewBNVPath(void* bin, int tag, const char* name,
                         float near_z, float far_z, Vec3* origin);   /* 0x00484c20 */
extern int    UpdateBlokeFromBNVPath(Bloke* b, void* path);          /* 0x00484cd0 */
extern void   UnAdjustBlokePosition(Offset* p);                      /* 0x00442d80 */
extern void   ScreenToMapRef(Offset* screen, Pos* out, int mode);    /* 0x0045be90 */
extern int    CalcMoveLine(Pos from, Pos to, void* path);            /* 0x00480740 */
extern int    NewDirForAction(Bloke* b, unsigned char dir);          /* 0x004833d0 */
extern void   BlokeSitAnim(Bloke* b);                                /* 0x00440780 */
extern void   BlokeSetFrame(Bloke* b, int frame);                    /* 0x00440870 */
extern void   BlokeWalkAnim(Bloke* b);                               /* 0x00440910 */
extern void   SetPersonDirection(Person3D* p, int dir);              /* 0x004400b0 */
extern void   RemoveBlokeFromRide(RideObject* item, RiderNode* r);   /* 0x0048a100 */
extern void   Ride_ClearFlagToNotLetAnyoneOn(void* square);          /* 0x00443000 */
extern void   BuyItem(RideElem* elem, MapSquare* at, int which);     /* 0x004539e0 */
extern void   PlayInstanceOfSample(void* def, int a, int b,
                                   RideSoundSource* src);            /* 0x00496d20 */
extern void   HeapFree_w(void* p);                                   /* 0x0049e4d0 */
extern int    rand(void);                                            /* 0x0049e4b2 (CRT) */
extern int    sprintf(char* dst, const char* fmt, ...);              /* 0x0049e573 (CRT) */

extern MapConfig* g_map_cfg;                                         /* 0x004bcbf4 lpConfig (a POINTER) */

/* =========================================================================
 * CAROUSEL
 * ========================================================================= */

typedef struct CarouselRec {
    struct CarouselRec* next;  /* +0x00 */
    unsigned short square;     /* +0x04 packed {x,y} */
    char           boarded;    /* +0x06 */
    char           aboard;     /* +0x07 */
    char           frame;      /* +0x08 */
    unsigned char  pad09[3];
    int            flags;      /* +0x0c bit0 = running, bit 0x4000 = boarding */
    int            f10;        /* +0x10 */
    int            f14;        /* +0x14 */
    unsigned char  visitors;   /* +0x18 */
    unsigned char  pad19[3];
    int            timer;      /* +0x1c */
    unsigned char  seat[4];    /* +0x20 one byte per seat, 0 = free */
} CarouselRec;

extern CarouselRec* g_carousel_recs;   /* 0x006160c4 the per-placement list */
extern RideObject*  g_carousel_item;   /* 0x006160bc the class payload */
extern void*        g_carousel_zspr;   /* 0x006160c0 the ride's depth sprite */
extern int          g_carousel_dx;     /* 0x00616078 rider offset, x */
extern int          g_carousel_dy;     /* 0x0061607c rider offset, y */
extern void*        g_carousel_on;     /* 0x00616090 CarouselOn .bnv bundle */
extern void*        g_carousel_off;    /* 0x00616098 CarouselOff .bnv bundle */
extern void*        g_carousel_sample; /* 0x004b64e0 the running sample */
extern char         g_carousel_path[]; /* 0x004b64cc "BlokeBox??" */
extern char         g_fmt_02d[];       /* 0x004b4704 "%02d" */

/* Tick one placed carousel (its own animation and timers). */
extern void Carousel_TickInstance(CarouselRec* rec);                 /* 0x0042c6d0 */

/* ---- 0x0042bc60 -- find the record for one map square -------------------
 * The rotated list walk every ride in the game uses: the head is tested once
 * inline, then the loop advances and re-tests, so a hit on the head and a hit
 * in the loop share one `ret`.  Both key compares read the CALLER's square as
 * the memory operand. */

/* CLOSED by `memcmp(&rec->square, sq, 2)`: VC6 expands the 2-byte intrinsic
 * memcmp late (after loop-invariant hoisting), which is why the CALLER's key
 * is re-read as the compare's memory operand on every iteration, and the
 * intrinsic materialises the first operand's ADDRESS (`lea edx,[rec+4]`)
 * before the folded word load -- the "dead lea" that no volatile spelling
 * reached (the joust-style volatile levers reproduced 16/18; ~20 measured
 * variants of pointer/struct-copy/helper forms all lacked the lea).  A key at
 * offset 0 (Joust/Catapult) shows no lea because the address is the record
 * pointer itself.  No volatile needed. */
// FUNCTION: LEGOLAND 0x0042bc60
CarouselRec* Carousel_FindRec(MapSquare* sq)
{
    CarouselRec* rec = g_carousel_recs;

    if (rec != 0) {
        while (memcmp(&rec->square, sq, 2) != 0) {
            rec = rec->next;
            if (rec == 0)
                return 0;
        }
        return rec;
    }
    return 0;
}

/* ---- 0x0042cd20 -- hand out a seat --------------------------------------
 * Picks a random seat, then scans FORWARD (wrapping) for the first free one,
 * marks it taken and stores it ONE-BASED in the bloke.  Note there is no
 * "all seats taken" exit: if every seat is occupied the wrap scan spins
 * forever -- the callers only call this while the machine is accepting
 * riders, which is capped at `capacity`.  The return value is also the
 * one-based index, and it is what gets printed into the .bnv path name. */

// FUNCTION: LEGOLAND 0x0042cd20
int Carousel_PickSeat(RiderNode* r, CarouselRec* rec, char seats)
{
    int i = rand() % seats;

    while (rec->seat[i] != 0) {
        i++;
        if (i >= seats)
            i = 0;
    }
    rec->seat[i] = 1;
    r->bloke->seat = (unsigned char)(i + 1);
    return i + 1;
}

/* ---- 0x0042bc90 -- start the ride turning -------------------------------
 * Everyone who boarded is now "aboard", the machine latches running (bit 0)
 * and stops accepting (bit 0x4000 cleared), and the ride's sample is started
 * sourced at the machine's own map square. */

// FUNCTION: LEGOLAND 0x0042bc90
void Carousel_StartRide(CarouselRec* rec)
{
    RideSoundSource src;

    src.kind = 2;
    rec->aboard = rec->boarded;
    rec->flags = (rec->flags & ~0x4000) | 1;
    rec->boarded = 0;
    rec->f14 = 0;
    rec->frame = 1;
    src.x = ((MapSquare*)&rec->square)->bx;
    src.y = ((MapSquare*)&rec->square)->by;
    PlayInstanceOfSample(g_carousel_sample, 1, 1, &src);
}

/* ---- 0x0042c800 -- tick every placed carousel --------------------------- */

// FUNCTION: LEGOLAND 0x0042c800
void Carousel_TickInstances(void)
{
    CarouselRec* rec = g_carousel_recs;

    while (rec) {
        Carousel_TickInstance(rec);
        rec = rec->next;
    }
}

/* =========================================================================
 * 0x0042c820 -- CAROUSEL, the +0xa8 per-tick update.
 *
 * Fifteen actions (0..14); 3, 4, 6 and 9..12 have no case and fall to the
 * default, so the original's byte index table has seven entries pointing at
 * the join.  The cycle:
 *
 *   0   claim the machine: bump its visitor count, set the 180-frame boarding
 *       timer, take the "on this ride" flag and walk to (tx-3, ty+0.5)
 *   1   MOUNT (the shared sequence documented at the top of this file): pick a
 *       seat with Carousel_PickSeat, build the boarding path name by printing
 *       the ONE-BASED seat number into "BlokeBox??", and start the
 *       CarouselOn .bnv path with the rider's own screen position as origin
 *   2   play that path back; when it runs out free it, ask for pose set 1 and
 *       jump to action 5
 *   5   sit down: riding flag on, frame 0, re-take the z-sprite and depth,
 *       and count the boarder.  When `boarded` reaches the class capacity the
 *       machine starts (Carousel_StartRide)
 *   7   stand up: walk animation, pose set 2, and a SECOND .bnv path
 *       (CarouselOff, tag 2) whose origin is the rider's own in-ride offset
 *       (+0x3c/+0x3e) doubled -- not a screen solve, because the rider is
 *       already positioned by the machine
 *   8   play that back; when it ends free it, face direction 3, action 13
 *   13  release the seat, drop the z-sprite, convert the 3D model's screen
 *       position back to a map reference (UnAdjustBlokePosition +
 *       ScreenToMapRef), scale the result to 24.8 and walk to the tile centre
 *   14  leave: RemoveBlokeFromRide, clear the ride flag, and when the last
 *       rider is off reset `boarded` and let the machine accept riders again
 *
 * Note the class payload is read from the ride's OWN global (0x006160bc) for
 * the capacity, not from the `item` the callback was handed; they are the
 * same object, but the original spells it that way and it is reproduced.
 * ========================================================================= */

/* NOTE (2026-09-04, lane K): 378/378 instructions, 1225/1225 BYTES, the whole
 * block layout, both jump tables and the ENTIRE FRAME MAP are reproduced.
 * Audit mismatch 29/378 -- but read the next paragraph before "improving" it:
 * this body is 29 where an earlier one was 27, and it is nevertheless MUCH
 * closer to the original.
 *
 * MODULO A RENAMING OF THE THREE CALLEE-SAVED REGISTERS the body is 8
 * mismatches, not 29.  Apply the permutation ebx->ebp, ebp->edi, edi->ebx to
 * our stream (widths included: bx/bp/di) and only indices 123-127 and
 * 228/229/231 differ; the whole
 * Y block (110-122, 128-138) becomes instruction-for-instruction identical,
 * INCLUDING the original's `sub edx,ecx`, its `mov ecx,[esi+4]` at 119 and its
 * in-place `add <sy2>,edx` at 120.  Measured for the three candidate bodies:
 *     this one   audit 29    8 real mismatches after the renaming
 *     `sy2 = sy + (oy - Get_YScroll())`   audit 27   27 real (nothing renames)
 *     `sy2 = sy - Get_YScroll(); sy2 += oy;`  audit 13   13 real
 *   (scratchpad/laneK/permrank.py computes that column; the strict count
 *    ranks these three 13 < 27 < 29 and the disassembly ranks them the other
 *    way round, so RANK CANDIDATES ON THE POST-RENAMING COUNT.)
 * So the source shape below is the original's and the residual is ONE
 * allocator question, stated at the bottom.
 *
 * WHAT THE SOURCE SHAPE IS, and why (this closed 27 -> "9 modulo renaming",
 * and it is the same shape mechrides.c proved for its four BNV _Activate
 * callbacks -- see mechrides.c ROUND 5/6; do not re-derive it):
 *  (1) THE Y CHAIN IS ONE WEB.  `sy2 = (wx + wy) * th >> 9;` then
 *      `sy2 += g_map_cfg->oy - Get_YScroll();`.  The compound assignment is
 *      in-place on the shift's own web, which is what puts the accumulate in
 *      sy2's register (`add ebx,edx` in the original) with the parenthesised
 *      delta folded.  Spelled `sy2 = sy + (oy - Get_YScroll())` the delta is a
 *      compiler TEMPORARY and wins the add-rank destination copy, so the sum
 *      lands in a scratch register (`add ecx,ebx`) and ecx is then busy from
 *      113 to 137, which ALSO costs the b->person hoist.  X is deliberately
 *      NOT compound -- `sx2 = g_map_cfg->ox - Get_XScroll() + sx;` gives
 *      `add edi,ebp` (destination = the ox load's register, sx folded), which
 *      is what the original has.  The asymmetry is real and load-bearing.
 *  (2) `p = b->person;` NAMES THE FIRST b->person READ ONLY.  The original
 *      hoists that `mov ecx,[esi+4]` twenty instructions, above the two
 *      escaped `pos` stores; a store to an escaped local is a schedule
 *      barrier for a pointer load, so it cannot climb on its own.  Caching it
 *      for the f30/depth reads as well is much worse -- the original really
 *      does reload after each store through the pointer.
 *  (3) THE PHANTOM HOME (from the previous pass, 278 -> 34, unchanged).
 *      `struct { int x, y; } spill; *(volatile int*)&spill.x = sx;` reserves
 *      the never-touched entry-0x34 dword and emits the original's dead
 *      `mov [esp+0x24],ebp`.  Frame: -0x3c item  -0x38 key + the dead sx store
 *      -0x34 NEVER TOUCHED  -0x30 tw  -0x2c th  -0x28 rec  -0x24 next
 *      -0x20/-0x1c screen  -0x18/-0x14 pos  -0x10/-0x0c pos2  +0x04 r.
 *  (4) `sx = (wx - wy) * tw >> 9;` BEFORE the sum (difference-before-sum:
 *      `mov ebp,ebx / add ebx,edi / sub ebp,edi`; sum-first gives `lea` and
 *      one instruction fewer), case 7's two pos2 stores through block-scope
 *      temporaries, and `b->action++` before `b->flags62 |= 0x80`.
 *
 * WHAT IS LEFT (3 causes, 8 indices).
 *  (a) THE CALLEE-SAVED ROTATION (indices 86-138, and it is the whole
 *      difference between 29 and single digits).  Ours colours wy=ebp,
 *      wx=ebx... i.e. ours is the original's assignment rotated by one:
 *      original (wy=edi, wx/sum/sy2=ebx, sx=ebp), ours (wy=ebp,
 *      wx/sum/sy2=edi, sx=ebx).  Merging the Y chain into the wx web is what
 *      drops that web to the LAST callee-saved register; with the Y chain
 *      split at any point the priority order is the original's.  The residual
 *      is exactly "give the merged sy2 web ebx".  THIS IS THE SAME WALL AS
 *      mechrides.c's four _Activate callbacks -- solving it there solves it
 *      here.  Inert against it (measured this pass, ~45 variants, ALL exactly
 *      29): both world-read orders; `register`/`unsigned`/`long` on sy2 and
 *      wx; commuting either product; all 24 orders of the four subtraction
 *      statements; `pos.y = sy2 + sy2` / `<< 1` (an extra reference is NOT
 *      the driver -- and the two-statement form, which DOES get the right
 *      colouring, differs only by having one); a name boundary at every point
 *      of the Y chain (before the `sar`, before the imul, on `wx += wy`) --
 *      all byte-identical to the merged form, so it is the WEB, not the name;
 *      `if (v) { }` consumers on sy2/sx2/wx/th; `p` at three other positions;
 *      spill.y as the volatile target; b35 moved.
 *  (b) INDICES 123-127: we hoist the `screen.ox` load into the edx freed by
 *      the cdq at 121 (`mov edx,[esp+0x44]` at 123, `sub ebp,edx` at 127);
 *      the original leaves edx idle and re-uses eax serially
 *      (`sar/sub/mov eax,[esp+0x44]/sub/mov eax,[dy]`).  The original has the
 *      same register free, so this is a scheduler-priority difference, very
 *      probably a knock-on of (a).
 *  (c) INDICES 228/229/231 (case 7): the two `movsx` come out in DESCENDING
 *      displacement order (+0x3e then +0x3c) where the original has
 *      ascending, and our second doubling is `add eax,eax` where the original
 *      has `shl eax,1`.  Registers, shift order and store order are already
 *      the original's.  Storing pos2.y first DOES fix the load order but then
 *      reverses the stores (15); `*2` / `<<1` / `x+x` are byte-identical in
 *      every position; so are doubled temps, direct field reads, a `short*`
 *      walk, and a static __inline setter with either argument order (both
 *      inert -- 13 variants).
 *
 * MEASURED AND NOT COMMITTED (recorded so they are not re-derived):
 *  - `sy2 = sy - Get_YScroll(); sy2 += g_map_cfg->oy; if (sy2) { }` scores 13
 *    and gets the callee-saved colouring right (the name boundary keeps the
 *    wx web short), but the original's index-118 instruction is `sub edx,ecx`
 *    = oy - yscroll, and this body emits `sub ebx,ecx` = sy - yscroll: a
 *    DIFFERENT computation in a different register, which then displaces the
 *    two GetUnitDepth pushes.  It is 13 real mismatches after any renaming,
 *    against this body's 9.  The empty `if` is also required (without it VC6
 *    flattens the two later `sy2 -=` back into the sum: 251, and all 24
 *    subtraction orders are identical at 251).  Not the original's source.
 *  - `pos.y` before `pos.x` scores 26 by shifting two stores into accidental
 *    alignment; the original emits the whole X chain first.
 *  - Forcing the b->person load early with a scoped `Person3D* p` around the
 *    pos stores (120), and `sx = -(wy - wx) * tw >> 9` (inserts a `neg`).
 *  - All difference/delta spellings canonicalise: `sy2 = sy - (Get_YScroll()
 *    - oy)`, `sy2 = sy; sy2 += delta;`, `sy2 = sy; sy2 -= ys - oy;`, a named
 *    delta local on either side, and both operand orders of the `+` are
 *    byte-identical to `sy2 = sy + (oy - Get_YScroll())`.
 * Data confirmed here: lpConfig (0x004bcbf4) is a POINTER, not a struct, and
 * MapConfig.ox/oy at +0x20/+0x22 are `unsigned short` (zero-extended) while
 * Get_XScroll/Get_YScroll return `short` (movsx).
 *
 * PASS N+1 (2026-09-04, lane w7ticks).  NO CHANGE -- still 29 strict / 8 real,
 * same first diverging index 86, same residual indices {123..127, 228, 229,
 * 231}.  ~130 further variants, all measured with the permutation-aware
 * ranker (scratchpad/w7ticks/pk.py, a parallel permrank; ~0.3 s per build, so
 * these were cheap and the whole matrix is reproducible).
 *
 * THE HARD FACT THIS PASS ADDS: the permutation is INVARIANT.  Every single
 * one of the ~130 variants -- including ones scoring 15, 22, 254, 274 and 293
 * -- came back with permrank perm `bpdibxsi` (our ebx = the original's ebp,
 * our ebp = its edi, our edi = its ebx).  Only four variants moved it at all,
 * and all four are structurally destroyed bodies (>= 248 real).  Nothing that
 * keeps the original's instruction sequence changes the colouring, which is
 * why "one more spelling of the Y chain" is not the way in.
 *
 * NEWLY RULED OUT (all still exactly 29 strict / 8 real unless noted):
 *   - THE FOUR NEW CROSS-LANE LEVERS, none of which reaches this wall:
 *       * a named local holding a CSE'd byte field -- tried on every CSE'd
 *         byte in the function: `rec->boarded` (`char nb = ++rec->boarded;`
 *         and a block-scoped form), `b->b74`, `b->seat`, `b->action` fed to
 *         the switch.  All 29/8, permutation unmoved.  (`nb` written as the
 *         two-statement `nb = rec->boarded + 1; rec->boarded = nb;` is 36 and
 *         an int `nb` is 188 -- both change case 5, neither the colouring.)
 *       * spelling a memory operand INLINE rather than through a named temp:
 *         `b->world.x` and/or `b->world.y` inline in both products (16 / 22 /
 *         22 -- the loads move, the colouring does not).
 *       * the add-destination RANK rule at every site in the block: X written
 *         compound (`sx2 = sx; sx2 += ox - Get_XScroll();`), X reversed
 *         (`sx + (ox - Get_XScroll())`), Y written flat
 *         (`sy2 = oy - Get_YScroll() + sy2;`), the delta named on either
 *         side.  ALL 29/8 -- none of them moves the colouring.
 *   - BUT ONE SITE HERE DOES DECIDE THE DISPUTED TIE-BREAK, and the answer is
 *     LAST-DEFINED-WINS.  `wx + wy` at index 96 has two named locals as its
 *     operands and nothing else; `wx` is the read-first operand in BOTH
 *     spellings because the difference `(wx - wy)` is written first either
 *     way, so only the DEFINITION order varies:
 *         wy = b->world.y; wx = b->world.x;   ->  add <wx's reg>, <wy's reg>
 *         wx = b->world.x; wy = b->world.y;   ->  add <wy's reg>, <wx's reg>
 *     The destination follows the local defined LAST, not the one read
 *     first, and read-first-wins is therefore refuted at this site.  (The
 *     original's `add ebx,edi` with ebx = `b->world.x` says its source
 *     defines wy before wx, which is the order committed here.)
 *   - the 24 orders of the four subtraction statements RE-RUN under permrank
 *     (the old note recorded them as "all exactly 29"; they are also all
 *     exactly 8 real, and the perm never moves) -- 18 of the 24 stay at 8, six
 *     go to 20 and two to 33.
 *   - the 24 orders of the four case-1 SETUP statements (screen =, wy =,
 *     wx =, GetTileDimensions): the committed order is the unique minimum;
 *     the next best is 13.
 *   - VARIABLE IDENTITY IS NOT A LEVER (this is the cleanest new negative):
 *     reusing `tx`/`ty` as `wx`/`wy`, `tx` as `sx`, `tx`/`ty` as `sy2`/`sx2`,
 *     `seat` as `wy` -- all byte-identical to the separate-name forms or
 *     worse (14).  VC6 allocates webs, and MERGING two names into one is as
 *     inert as splitting one into two.
 *   - block scope for {wx, wy, sx, sx2, sy2} (all five, the two screen
 *     locals, the two world locals, `sx` alone), `register` on all five and
 *     on each one alone, and ten declaration-order permutations: every one
 *     byte-identical.
 *   - volatile spellings and positions of the phantom home: the store moved
 *     to five other points (15 / 16 / 35 / 35), `spill.y` as the target (9 --
 *     one worse, it adds index 102), both slots stored (264), and no store at
 *     all (274, which is what the phantom home is worth).
 *   - case 7 (indices 228/229/231) attacked on its own with 30 variants:
 *     direct field reads, one temp, doubled temps, `<< 1` / `+ x` / `2 * x`
 *     mixed per site, `short` temps, a `short*` walk, per-axis nested blocks,
 *     `int d[2]` and a `Vec3` carrier, a volatile read, a `pos2.z` filler.
 *     The committed form is the minimum; the load order (descending
 *     displacement, +0x3e before +0x3c) is VC6's canonical sort of two
 *     structurally identical `movsx`+`shl` operands and nothing reverses it
 *     without also reversing the stores.
 * WHERE A FUTURE PASS SHOULD LOOK.  Not at case 1's source.  The four webs
 * (wy, wx/sum/sy2, sx, sx2) have IDENTICAL reference counts and live ranges
 * in both builds -- the emitted streams are instruction-for-instruction equal
 * modulo the renaming over 86..122 and 128..138 -- so the allocator's INPUT
 * is the same and only its choice differs.  Modelling both builds as
 * "priority = refs / live-range length, first free register in a fixed
 * preference order" fits the ORIGINAL exactly with the order (ebp, edi, ebx)
 * and CANNOT be made to fit ours; so the preference order itself, or the
 * priority key, is decided by something outside this switch arm.  The next
 * experiment worth running is a diagnostic one: perturb case 5 or case 13
 * (whose webs also live in ebx and ebp) and see whether case 1's colouring
 * moves at all.  If it does, the driver is function-wide and case 1 is the
 * wrong place to look.
 *
 * PASS w8rides (2026-09-04).  NO CHANGE (29 strict / 8 real / register-blind
 * 3), but the diagnostic the paragraph above asks for HAS NOW BEEN RUN, and
 * it comes back negative.
 *
 *  THE DRIVER IS NOT FUNCTION-WIDE.  Case 13 was perturbed four ways -- the
 *  two `b->world` shifts swapped, `tx` hoisted above `ty`, the two
 *  `b->target` stores swapped, and both target shifts moved below the two
 *  world shifts.  Two of the four change case 13's own emitted code (the
 *  world swap adds four mismatches at 317/318/323/324, the target swap six at
 *  315-320), so the perturbation really did reach the allocator.  Case 1's
 *  colouring did not move a hair in ANY of them: identical best permutation
 *  `bpdibxsi`, identical first divergence at index 86, identical residual set
 *  {123, 124, 125, 126, 127, 228, 229, 231}.  So the answer to the question
 *  this note posed is "no": disturbing another switch arm whose webs live in
 *  the same two callee-saved registers does not shift case 1 at all.  With
 *  the allocator's input already shown identical to the original's, there is
 *  now no block left that source can reach -- neither case 1 (invariant over
 *  ~130 variants) nor a neighbour (invariant over these four).
 *
 *  CLUSTER (c) IS A TWIN, AND THE TWIN IS ALSO STUCK.  Indices 228/229/231
 *  are byte-for-byte the same phenomenon as SpiderRide_Activate's indices
 *  195/196/198 in mechrides.c -- same two `movsx` off +0x3c/+0x3e, same
 *  descending emission order, same `shl edx,1` / `add eax,eax` split, same
 *  three-index residual -- reached from two independently written source
 *  spellings.  Nine further spellings were measured on the SpiderRide side
 *  this pass (pre-doubled temps in both store orders, plain field reads in
 *  both orders, `+ x`, `<< 1`, a second temp pair, a reversed temp
 *  declaration order); every one either ties at the same residual or
 *  ESCAPES.  Together with the 30 already measured here that is 39 spellings
 *  across two functions and two lanes.  (THE DIRECTION IN THIS SENTENCE IS
 *  WRONG and is corrected in the PASS w9rides paragraph below: the ORIGINAL
 *  is ascending, ours is descending.  The cluster is still unreachable.)
 *
 *  RECOMMENDED FOR RETIREMENT.  8 real mismatches in 378 instructions, in
 *  two clusters, both now shown invariant under everything source can
 *  express.
 *
 * PASS w9rides (2026-09-04).  NO CHANGE (29 strict / 8 real /
 * register-blind 3).  Both clusters are now stated correctly.
 *
 *  CLUSTER (b), 123-127, IS THE FIVE-FUNCTION FAMILY SLOT, and this
 *  function is where it is cleanest.  The idiom is
 *      mov eax,[g_carousel_dx] / cdq / sub eax,edx / <<<SLOT>>> /
 *      sar eax,1 / sub edi,eax / mov eax,[esp+0x44] / sub edi,eax
 *  and it occurs twice, once per axis.  The original leaves the X slot
 *  EMPTY (screen.ox is loaded serially into eax at index 125) and fills the
 *  Y slot with `mov edx,[g_carousel_zspr]`; we fill BOTH, the X one with
 *  `mov edx,[esp+0x44]` at 123.  That single hoist, plus the one-slot
 *  knock-on it gives `mov eax,[g_carousel_dy]`, is the entire cluster.  The
 *  same slot -- same instructions, same registers either side -- is
 *  SpinningBarrels 113-132, SafariRide 86-124 and SpiderRide 87-97 /
 *  PlaneRide 86-96 (where the miss runs the other way: the original fills
 *  the Y slot with `or [esi+0x62],0x80` + `mov edx,[esi+4]` and we leave it
 *  empty).  The full five-function table and the negatives are in the PASS
 *  w9rides paragraph of SpinningBarrels_Activate in mechrides.c.
 *  Measured HERE this pass and byte-identical in every cell: the x tail and
 *  the y tail as flat three-term expressions (separately and together),
 *  named halves, named `screen.ox`/`.oy` locals, an `Offset*` for both, the
 *  four subtractions interleaved and y-first, a `zs = g_carousel_zspr;`
 *  cache at three points (VC6 sinks it every time -- local pressure cannot
 *  be raised from source), a second `p2` cache, and the two adjacent
 *  halving globals respelled as ONE object (`Offset g_carousel_ofs;` at
 *  0x616078, and `int g_carousel_d[2]`).  Storing `pos.y` before `pos.x`
 *  DOES move the slot -- it emits the y chain first and leaves that chain's
 *  slot empty -- but it is `screen.ox` that is hoisted in BOTH orders, so
 *  the choice belongs to that one load and not to chain position; it scores
 *  26 strict / 11 real and reverses the seed stores, so it is a
 *  compensating error and is not committed.
 *
 *  CLUSTER (c), 228/229/231, WAS READ BACKWARDS in the previous pass and
 *  the correction matters.  The ORIGINAL is ASCENDING:
 *      orig: movsx edx,[esi+0x3c] / movsx eax,[esi+0x3e] / shl edx,1 /
 *            shl eax,1
 *      ours: movsx eax,[esi+0x3e] / movsx edx,[esi+0x3c] / shl edx,1 /
 *            add eax,eax
 *  The final registers and both stores agree; only the two loads are
 *  swapped, and `add eax,eax` follows from loading eax first.  So
 *  "descending is VC6's canonical sort" described OUR output, not the
 *  original's, and the target was stated wrongly.  The mechanism is right
 *  though: VC6 evaluates the LAST store's operand FIRST, which is why
 *  x-store-first forces the +0x3e load first.  The corrected target
 *  {ascending loads, x-store first} is still not reachable -- 20 further
 *  spellings this pass, all measured here: y-store-first gives ascending
 *  loads but swaps the registers and the stores (10 real); pre-doubled
 *  temps (`int px = b->ride_dx * 2;`) give BOTH `shl` and the ORIGINAL's
 *  registers but keep the loads reversed and cost 8 indices elsewhere at
 *  1224 bytes; and byte-identical are `2 * x` at either or both sites, a
 *  `volatile short` read of either field (or both -- 9/10, it adds
 *  mismatches), a `short*` walk over the pair, `r->bloke->` on either
 *  field, a `static __inline` two-int seeder, comma operators, nested and
 *  reversed scopes.  `short` temps are 152 and a `pos2.z = 0;` filler 153.
 *  So the cluster is unreachable, but the reason is a source-order rule
 *  (last store evaluated first) that the original's compiler did not
 *  follow, not a canonical sort we were matching.
 *
 *  STILL RECOMMENDED FOR RETIREMENT: 8 real mismatches in 378 instructions,
 *  in two clusters, both now shown invariant under everything source can
 *  express and both now correctly described. */
// WIP-FUNCTION: LEGOLAND 0x0042c820  (378/378 instructions, 1225/1225 bytes and the whole frame map, audit mismatch 29/378 but only 8 modulo a renaming of the three callee-saved registers; first diff at index 86: the merged Y web takes the LAST callee-saved register instead of ebx -- the same wall as mechrides.c's four _Activate callbacks)
void Carousel_Tick(RideElem* elem)
{
    RideObject*   item = elem->data;
    MapSquare*    key;
    int           sx;
    /* Reserves the original's two low frame dwords: `.y` is the slot at
     * entry-0x34 that no instruction in the original ever touches, and
     * `.x` is lifetime-coloured onto `key`'s home, which is where the
     * original's dead store of the dying `sx` (index 102) lands. */
    struct { int x, y; } spill;
    int           tw;
    int           th;
    CarouselRec*  rec;
    RiderNode*    next;
    Offset        screen;
    Vec3          pos;
    Vec3          pos2;
    RiderNode*    r;
    Bloke*        b;
    Person3D*     p;
    int           tx;
    int           ty;
    int           wx;
    int           wy;
    int           seat;
    unsigned char a;
    int           sx2;
    int           sy2;

    r = item->riders;
    while (r) {
        next = r->next;
        b = r->bloke;
        key = (MapSquare*)&r->ride_id;
        rec = Carousel_FindRec(key);
        if (!rec)
            return;
        tx = item->base_x + key->bx;
        ty = key->by + item->base_y;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                tx -= 3;
                rec->visitors++;
                ty = (ty << 8) + 0x80;
                tx <<= 8;
                rec->timer = 180;
                b->flags62 |= 8;
                b->target.x = tx;
                b->target.y = ty;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->f58 = 0;
                b->action++;
                break;

            case 1:
                screen = GetScreenCoordsForObject(key, item);
                wy = b->world.y;
                wx = b->world.x;
                GetTileDimensions(&tw, &th);
                sx = (wx - wy) * tw >> 9;
                sy2 = (wx + wy) * th >> 9;
                /* the original's apparently dead `mov [esp+0x24], ebp`:
                 * sx is homed here as it dies into sx2 (volatile so VC6
                 * cannot delete the store). */
                *(volatile int*)&spill.x = sx;
                sx2 = g_map_cfg->ox - Get_XScroll() + sx;
                sy2 += g_map_cfg->oy - Get_YScroll();
                sx2 -= g_carousel_dx / 2;
                sx2 -= screen.ox;
                sy2 -= g_carousel_dy / 2;
                sy2 -= screen.oy;
                /* `p` names the FIRST b->person read only: the original
                 * hoists its `mov ecx,[esi+4]` twenty instructions, above the
                 * two escaped `pos` stores, and a store to an escaped local is
                 * a schedule barrier for a pointer load, so the load cannot
                 * climb on its own.  The other two reads stay spelled
                 * `b->person` because the original really does reload after
                 * each store through the pointer. */
                p = b->person;
                pos.x = sx2 * 2;
                pos.y = sy2 * 2;
                p->zsprite = g_carousel_zspr;
                b->person->driven = 1;
                b->person->depth = GetUnitDepth(-1617853.25f, -1618109.0f);
                b->b35 = 0;
                seat = Carousel_PickSeat(r, rec, (char)g_carousel_item->capacity);
                sprintf(g_carousel_path + 8, "%02d", seat);
                b->bnvpath = NewBNVPath(g_carousel_on, 0, g_carousel_path,
                                        -1617853.25f, -1618109.0f, &pos);
                UpdateBlokeFromBNVPath(b, b->bnvpath);
                b->action++;
                b->flags62 |= 0x80;
                break;

            case 2:
                if (UpdateBlokeFromBNVPath(b, b->bnvpath) == 0) {
                    b->b35 = 1;
                    b->action = 5;
                    HeapFree_w(b->bnvpath);
                    b->bnvpath = 0;
                }
                BlokeSetFrame(b, b->b74);
                break;

            case 5:
                b->flags62 |= 0x80;
                BlokeSetFrame(b, 0);
                b->b35 = 1;
                b->person->zsprite = g_carousel_zspr;
                b->person->driven = 1;
                b->person->depth = GetUnitDepth(-1617853.25f, -1618109.0f);
                b->action++;
                rec->boarded++;
                if ((short)rec->boarded == g_carousel_item->capacity)
                    Carousel_StartRide(rec);
                break;

            case 7:
                {
                int rdx = b->ride_dx;
                int rdy = b->ride_dy;
                pos2.x = rdx * 2;
                pos2.y = rdy * 2;
                }
                BlokeWalkAnim(b);
                BlokeSetFrame(b, 0);
                b->flags62 |= 0x80;
                b->person->zsprite = g_carousel_zspr;
                b->person->driven = 1;
                b->person->depth = GetUnitDepth(-1617853.25f, -1618109.0f);
                b->b35 = 2;
                sprintf(g_carousel_path + 8, "%02d", b->seat);
                b->bnvpath = NewBNVPath(g_carousel_off, 2, g_carousel_path,
                                        -1617853.25f, -1618109.0f, &pos2);
                b->action++;
                break;

            case 8:
                if (UpdateBlokeFromBNVPath(b, b->bnvpath) == 0) {
                    b->b35 = 2;
                    b->action = 13;
                    HeapFree_w(b->bnvpath);
                    b->bnvpath = 0;
                    b->dir = 3;
                }
                BlokeSetFrame(b, b->b74);
                break;

            case 13:
                rec->seat[b->seat - 1] = 0;
                b->flags62 &= ~0x80;
                b->person->zsprite = 0;
                b->person->driven = 0;
                UnAdjustBlokePosition(&b->person->screen);
                ScreenToMapRef(&b->person->screen, &b->world, 0);
                b->person->f34 = 0;
                ty = (ty << 8) + 0x80;
                b->world.x <<= 8;
                b->world.y <<= 8;
                tx = (tx << 8) + 0x80;
                b->target.x = tx;
                b->target.y = ty;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 14:
                RemoveBlokeFromRide(item, r);
                b->flags62 &= ~8;
                if (--rec->aboard == 0) {
                    rec->boarded = 0;
                    Ride_ClearFlagToNotLetAnyoneOn(&rec->square);
                }
                break;
            }
        }
        r = next;
    }
    Carousel_TickInstances();
}

/* =========================================================================
 * BALLOONZ  (the ferris wheel)
 *
 * A BALLOONZ machine is a wheel of SIX gondolas.  Its per-placement record
 * (list head 0x00616060) is:
 *
 *   +0x00 BalloonzRec* next
 *   +0x04 u16    square
 *   +0x08 int    waiting      riders that have queued but not boarded
 *   +0x0c char   riders       riders currently on the wheel (max 6)
 *   +0x0d char   cars[6]      per-gondola state, indexed by gondola:
 *                             0 free, 1 claimed/occupied, 2 boarding,
 *                             3 finished (wants to get off)
 *   +0x13 char   half         which half-turn the wheel is in, 0 or 1
 *   +0x14 char   wheel        wheel angle, 0..0x17 (24 steps per half)
 *   +0x15 char   zframe       the frame pushed into the z-sprite; it is the
 *                             PREVIOUS tick's `wheel`, so the depth buffer
 *                             lags the wheel by one frame (reproduced)
 *   +0x17 char   alighting    riders in the "get off" phase
 *   +0x18 int    docked       a gondola is at the platform this tick
 *   +0x1c int    unloading    a finished gondola is at the platform
 *
 * Balloonz_CarAtPlatform(wheel, half) = wheel / 8 + half * 3 is the gondola
 * currently at the boarding platform: 24 wheel steps / 8 = three gondolas per
 * half turn, two halves, six gondolas.  (Its `half == 1 && wheel > 0x17`
 * guard can never fire from either caller -- `wheel` is capped at 0x17 -- so
 * it is dead defensive code; reproduced.)
 *
 * The BNV animation frame a rider is placed at is `zframe + half * 24`, i.e.
 * one continuous 48-frame path around the whole wheel, and the .bnv slot name
 * is "Bloke" plus the two-digit GONDOLA number, printed into a stack copy of
 * the template string "Bloke??" (0x004b64bc) -- the same trick the carousel
 * plays on its global "BlokeBox??".
 *
 * cb_a8 is TWO passes.  Pass 1 walks the class rider list and runs each
 * rider's 16-step machine, copying nine record fields into the frame on entry
 * and writing them back on exit.  Pass 2 walks the record list and turns the
 * wheel: it only advances while at least one rider is aboard AND nothing is
 * holding the wheel -- a gondola at the platform that is claimed, boarding or
 * finished holds it, and so does the 1-in-3 chance the machine takes to stop
 * and pick up a new customer.
 * ========================================================================= */

/* The six gondola states, copied in and out of the record as ONE 6-byte unit
 * (VC6 moves it as a dword plus a word). */
typedef struct BzCars {
    unsigned char c[6];
} BzCars;

typedef struct BalloonzRec {
    struct BalloonzRec* next;  /* +0x00 */
    unsigned short square;     /* +0x04 */
    unsigned char  pad06[2];
    int            waiting;    /* +0x08 */
    char           riders;     /* +0x0c */
    BzCars         cars;       /* +0x0d */
    char           half;       /* +0x13 */
    char           wheel;      /* +0x14 */
    char           zframe;     /* +0x15 */
    unsigned char  pad16;
    char           alighting;  /* +0x17 */
    int            docked;     /* +0x18 */
    int            unloading;  /* +0x1c */
} BalloonzRec;

/* A per-layer sprite object; its LLS is *(+0x08) (legoland.h's SpriteObj). */
typedef struct SpriteObj {
    int    pad0;
    int    pad4;
    short** lls_holder;        /* +0x08 */
} SpriteObj;

extern void SetBlokePositionFromBNV(void* bin, Bloke* b, const char* name,
                                    int frame, float near_z, float far_z,
                                    int flag);                       /* 0x00484a70 */

extern BalloonzRec* g_bz_recs;     /* 0x00616060 the per-placement list */
extern void*        g_bz_bnv;      /* 0x00616010 the wheel's .bnv bundle */
extern void*        g_bz_ridezspr; /* 0x00616040 the z-sprite riders take */
extern SpriteObj*   g_bz_zspr;     /* 0x0081cde8 the wheel's own depth sprite */

/* ---- 0x0042a980 -- find the record for one map square ------------------- */

/* Byte-for-byte the same body as Carousel_FindRec against a different list
 * head; closed by the same intrinsic memcmp (see the note there). */
// FUNCTION: LEGOLAND 0x0042a980
BalloonzRec* Balloonz_FindRec(MapSquare* sq)
{
    BalloonzRec* rec = g_bz_recs;

    if (rec != 0) {
        while (memcmp(&rec->square, sq, 2) != 0) {
            rec = rec->next;
            if (rec == 0)
                return 0;
        }
        return rec;
    }
    return 0;
}

/* ---- 0x0042aa60 -- which gondola is at the boarding platform ------------ */

// FUNCTION: LEGOLAND 0x0042aa60
int Balloonz_CarAtPlatform(char wheel, char half)
{
    if (half == 1 && wheel > 0x17)
        return 0;
    return wheel / 8 + half * 3;
}

/* =========================================================================
 * 0x0042aa90 -- BALLOONZ, the +0xa8 per-tick update.
 *
 * PASS 1, per rider (16 actions, all sixteen present):
 *   0   claim the ride and walk to the queue head, (tx - 0.61, ty + 0.98);
 *       give the rider a 500-tick patience
 *   1-5 shuffle up the queue: (tx-1, ty+7), (tx-2, ty+7), (tx-2, ty+1),
 *       (tx-4, ty+1), (tx-6, ty+2) -- five fixed queue positions, so the
 *       queue is exactly five deep
 *   6   step up to the platform (tx - 6.2, ty + 3) and count one more
 *       customer as waiting
 *   7   face the wheel; then, ONLY if a gondola is docked, walk onto it
 *       (tx - 5.4, ty + 3), claim the gondola at the platform, take a
 *       rand()%3 + 4 half-second ride timer (x50 ticks) and move one
 *       customer from `waiting` to `riders`
 *   8   walk to the gondola door, (tx - 5, ty + 2.41)
 *   9   MOUNT: riding flag on, take the ride's z-sprite and depth, and hand
 *       the 3D model the wheel's .bnv path at frame 0
 *  10   ride: mark the gondola "boarding", keep the model on the wheel's
 *       current frame, and count the ride timer down
 *  11   the ride is over: mark the gondola "finished" and count one more
 *       rider as alighting
 *  12   wait for the wheel to bring THIS rider's gondola back to the
 *       platform while the machine is unloading, then drop the z-sprite and
 *       walk off to (tx - 5.4, ty + 3); the gondola goes back to "claimed"
 *  13   walk clear, (tx - 6.2, ty + 3)
 *  14   walk away, (tx - 5, ty + 9); free the gondola and count one fewer
 *       rider alighting
 *  15   leave the ride
 *
 * The `if (!rec) return` quirk of the slot applies here too, and it also
 * skips PASS 2 entirely for that frame.
 * ========================================================================= */

/* NOTE: 637/637 instructions, 1993/1993 BYTES, both passes, all sixteen case
 * blocks and every frame home reproduced; audit mismatch 5, all five in the
 * ENTRY block (indices 10..14).  History: 567+ESCAPES -> 78 (2026-09-04, see
 * the levers below) -> 5 (2026-09-04, second lane).
 *
 * WHAT CLOSED 78 -> 5, in order:
 *  (e) THE BIG ONE: the six locals both passes copy in and out --
 *      `alighting`, `half`, `waiting`, `docked`, `cars` and `riders` -- are
 *      ONE set of FUNCTION-LEVEL locals shared by the two loops, not two
 *      block-local sets.  The original gives them the same frame home in both
 *      passes (0x12/0x14/0x18/0x20/0x34/0x48) precisely because they ARE the
 *      same variables; declaring them once above both blocks put every frame
 *      slot in pass 1 exactly where the original has it and made pass 1
 *      byte-identical (102 mismatches, all downstream).  The other pass-2
 *      locals (`wheel`, `hold`, `unloading`, `at`, `rec`) must stay
 *      block-local: pass-2 `unloading` lives in ebp where pass-1 `unloading`
 *      is spilled at 0x2c, so they cannot be one variable.
 *  (f) pass 2's `at` is a signed `char`, not an `int`: the original stores it
 *      `mov byte ptr [esp+0x13], al` and re-widens with `movsx eax, al` for
 *      `cars.c[at]`.  (Pass 1's `at` stays `int` -- lever (b) below.)
 *  (g) `hold = 0;` must be the FIRST statement of the pass-2 loop body and
 *      `docked = 0;` the LAST of the copy-in, with the seven reads between
 *      them.  Then and only then does VC6 emit the two zero stores as
 *      `mov dword ptr [..], 0` IMMEDIATES instead of `xor eax,eax` + two
 *      register stores -- exactly the 1 instruction and 10 bytes that were
 *      missing.  All 128 placements of the two assignments among the seven
 *      reads were measured on the (e) baseline: only the two extreme ones
 *      (hold in the first three slots AND docked last, or mirrored) give the
 *      immediates.  On the OLD baseline all placements were identical, so
 *      this search must be re-run after any other change.
 *  (h) the seven pass-2 copy-in reads are in the SAME order as pass 1's
 *      (waiting, riders, cars, half, wheel, alighting, unloading).  All 720
 *      orders with `waiting` first were measured; this one is joint-best and
 *      it is the one that fixes the `half` store / `wheel` argument-home
 *      store pair at indices 543/546.
 *
 * EARLIER LEVERS THAT STILL HOLD (do not undo):
 *  (a) `docked = rec->docked;` is read BEFORE the
 *      `**g_bz_zspr->lls_holder = rec->zframe;` store: that store is a
 *      may-alias barrier that puts the six queue blocks' three-step
 *      eax/ecx/edx rotation into phase, which decides WHICH pair of case
 *      blocks gets cross-jumped into case 13's copy (the original merges
 *      cases 2 and 5).  Diagnostic: the register of the FIRST
 *      `lea r,[esi+0x98]` -- ecx is right (scratchpad/ridecb3/pr.py).
 *  (b) pass 1's `at` must be `int`: `at = Balloonz_CarAtPlatform(..);
 *      b->seat = (char)at; cars.c[(unsigned char)at] = 1;`.  A `char` there
 *      round-trips the index through its home.
 *  (c) `key` is taken from the rider BEFORE `b = r->bloke`, which keeps the
 *      cursor in eax for the original's `lea ebp,[eax+0xc]`.
 *  (d) the four record copy-in / copy-out blocks are store-schedule levers
 *      (scratchpad/ridecb3/climb.py hill-climbs them; re-run after any
 *      other change).
 *
 * WHAT IS LEFT (5 mismatches, indices 10..14, a pure SCHEDULE permutation of
 * six instructions in the entry block; register-blind edit distance 1):
 *      orig  st item / LD item->riders / st name[0..3] / test / st cursor /
 *            st name[4..7]
 *      ours  st item / st name[0..3] / LD item->riders / st name[4..7] /
 *            test / st cursor
 * i.e. both halves of the `char name[8] = "Bloke??"` initialiser need to sink
 * one slot later.  RULED OUT (all measured, all byte-identical to the current
 * body unless noted): declaring `name` in an inner scope entered AFTER
 * `r = item->riders;`, at function scope (9, moves them EARLIER), first or
 * last in the block; `r` initialised in its declaration at function scope,
 * before or after `name`; `item` declared inside the pass-1 block (9) or
 * assigned as a statement (10); a `for` loop, `while (r != 0)`, `do/while`
 * (626), an `if (r == 0) return;` guard (113); all six permutations of the
 * next/key/b loop-head reads (5 or 15); `unsigned char name[8]` and a
 * one-member struct wrapper; a volatile read of `item->riders` (8) or of
 * `elem->data` (12); `{'B','l',...}` and a 7-byte initialiser (both change
 * the initialiser's SHAPE and cost instructions).  VC6 hoists EVERY local
 * aggregate initialiser to the prologue -- its two template loads sit at
 * indices 3/4, between the pushes -- so no scope or statement placement can
 * move it in the IR; the residual is a scheduler priority, not source order.
 *
 * 2026-09-04, endgame lane -- ANOTHER 25 measured variants, all inert or
 * worse, plus the corpus scan the method asks for:
 *  - CORPUS SCAN (scratchpad/endgame/scan_init.py, "two absolute-address
 *    dword loads whose registers are stored to adjacent stack homes"): the
 *    WHOLE 1541-function exact corpus contains exactly ONE local aggregate
 *    initialiser of this shape, `UpDateCurrentProfile` (profiles.c
 *    0x491680), and it is a long software-pipelined ld/st/ld/st run with no
 *    pointer load anywhere near it.  scan_prohoist.py finds 92 functions
 *    that hoist an absolute load into the callee-saved push run, and every
 *    one of them is a plain global read.  There is NO worked example of this
 *    shape in the corpus: the calibration answer here is "corpus scan
 *    produced nothing".
 *  - The initialiser does NOT have to be an initialiser for VC6 to hoist it.
 *    Spelling it as two explicit `*(int*)&name[k] = *(const int*)&tmpl[k];`
 *    stores placed AFTER `r = item->riders;` (or before, or with the halves
 *    swapped), as a `static const` 8-byte struct assignment, or as a
 *    declaration-initialiser from that struct, all give byte-identical code:
 *    the store that is FIRST in IR is always emitted at index 10, ahead of
 *    the load.  So IR position is NOT the tie-break here.
 *  - `volatile` on either side of that copy: 10..13.  A union
 *    {char c[8]; int w[2];} wrapper: 5 (identical).  Declaring `name` INSIDE
 *    the while body: 16 -- VC6 does not hoist it, it emits the whole copy in
 *    the loop head, and the entry block then comes out as EXACTLY the
 *    original's core (st item / LD riders / test / st cursor), which is what
 *    proves the residual is only where the two stores get inserted.
 *  - No-code tuples do not exist for this block: `if (item) { }`,
 *    `if (!item) { }`, `if (elem) { }`, `if (name[0]) { }`,
 *    `if (item->riders) { }`, `if (r) { }`, `if (!r) { }`, `if (r == 0) { }`
 *    before and after the load, an `(int)` cast on the load, and a dead
 *    float-product tuple are ALL byte-identical, so the Pentium scheduler's
 *    window boundary cannot be moved from C here.
 *  - Isolated probes (scratchpad/endgame/p1.c..p3.c) reproduce the shape in
 *    ~40 instructions: VC6 ALWAYS emits both initialiser stores between the
 *    `mov edi,[elem+0xc]` and the `[edi+0xcc]` load (a two-slot AGI gap),
 *    and a `volatile` riders field, a second aggregate in the pool and a
 *    second pointer load in the entry block do not move them.  Cycle
 *    counting says the two schedules are EQUAL on P5 (both 5 cycles from the
 *    edi def), so this is a pure tie-break inside VC6's scheduler that no
 *    source form reaches.  Treat as exhausted unless a matched function
 *    turns up with an aggregate initialiser next to a pointer load.
 *
 * 2026-09-04, lane C -- 33 MORE measured variants, every one byte-identical
 * to the committed body (mism 5, first diff 10), so this residual is now
 * three-lane exhausted:
 *  - the fill spelled as two explicit `*(int*)&name[k] = *(const int*)"..."`
 *    stores placed after / before / split around / swapped around
 *    `r = item->riders;` (re-run from scratch, all 5);
 *  - `memcpy(name, "Bloke??", 8)` with the CRT intrinsic, before and after
 *    the load -- VC6 lowers it to the SAME two dword moves in the same slots;
 *  - `char name[] = "Bloke??"`, `name` first / middle / last in the block;
 *  - `r` initialised in its DECLARATION inside the pass-1 block, declared
 *    before or after `name` (the earlier note only covered function scope);
 *  - a `{ RiderNode* r0 = item->riders; r = r0; }` scratch copy, and a
 *    `for` loop instead of `while`;
 *  - NO-CODE TUPLES, the whole class, since the recorded window rule says the
 *    scheduler counts tuples from the FUNCTION START and there is nothing
 *    earlier than these two statements to put them in: `(RideObject*)`,
 *    `(RideObject*)(char*)`, `(RideObject*)(char*)(void*)` on `elem->data`;
 *    the same three casts on `item->riders`; `item = item;`, `r = r;`,
 *    `r = (RiderNode*)r;`, `(void)item;`, `(void)name[0];`, a comma
 *    expression `((void)0, item->riders)`, and two- and three-deep chains of
 *    empty `if` blocks.  Not one of them moves a single byte.
 *  Reading `elem->data` twice (`r = ((RideObject*)elem->data)->riders;`) is
 *  the only variant that changes anything, and it is worse (9, from index 2).
 * The pass-2 local set deliberately stays in its own block: that is what lets
 * `hold` colour onto pass 1's `frame` slot (0x1c) and the pass-2 wheel copy
 * onto pass 1's `next` slot (0x30).
 *
 * 2026-09-04, lane K: checked against the levers added since (the switch
 * goto-flip, difference-before-sum, the add-destination rank, the
 * callee-saved-zero width rule, the static __inline argument reorder) -- NONE
 * of them has a foothold here: the residual is five instructions with no
 * `add`, no switch tail, no constant zero and no helper call in it, just the
 * order of two spill stores around one load, and the register-blind distance
 * is already 1.  Re-measured: moving the `name` declaration to the end of the
 * block is byte-identical (confirming the declaration-order rule), and a
 * brace initialiser `{'B','l','o','k','e','?','?',0}` is far worse (623 --
 * it stores byte by byte).  Leave it at 5.
 *
 * 2026-09-04, lane w7ticks -- the quick check the brief asked for, and it
 * agrees: 5 strict, 5 real under permrank (perm = IDENTITY, so no register
 * question is hiding here), register-blind 1.  Twenty more placements of the
 * `name` declaration -- after each of the twelve inner declarations, at the
 * top of the inner block, and in the outer block (9, worse) -- are all
 * byte-identical, as are `*(int*)&name[0] = *(const int*)"Blok"` style
 * explicit dword stores (VC6 canonicalises them to the same pair).  Five
 * spellings of the rider-list read -- `RiderNode* r = item->riders;` as a
 * declaration initialiser at four positions and at the outer level -- are
 * likewise byte-identical; `while ((r = item->riders) != 0)` is 619.  The
 * residual is one instruction of Pentium pairing (`mov eax,[edi+0xcc]` ahead
 * of the first name store) with no C-visible handle.  Do not spend more. */
/* PASS w8rides (2026-09-04): not worked, per the lane brief (three-lane
 * exhausted).  Re-measured only: 5 strict, 5 real under the IDENTITY
 * permutation, and register-blind distance 1 -- the whole residual is a
 * single scheduling window at indices 10-14 with no register difference
 * anywhere in 637 instructions.  Nothing here for a colouring or a
 * reconstruction pass to hold on to. */
// WIP-FUNCTION: LEGOLAND 0x0042aa90  (637/637 instructions and 1993/1993 bytes, audit mismatch 5/637, first diff at index 10: the two halves of the name[8] initialiser are scheduled one slot too early around the rider-list load)
void Balloonz_Tick(RideElem* elem)
{
    RideObject*   item = elem->data;

    char          alighting;
    char          half;
    int           waiting;
    int           docked;
    BzCars        cars;
    char          riders;
    {
    int           at;
    char          frame;
    RiderNode*    r;
    int           unloading;
    RiderNode*    next;
    char          name[8] = "Bloke??";
    Bloke*        b;
    BalloonzRec*  rec;
    MapSquare*    key;
    int           tx;
    int           ty;
    int           idx;
    unsigned char a;

    r = item->riders;
    while (r) {
        next = r->next;
        key = (MapSquare*)&r->ride_id;
        b = r->bloke;
        rec = Balloonz_FindRec(key);
        if (!rec)
            return;
        waiting = rec->waiting;
        riders = rec->riders;
        cars = rec->cars;
        half = rec->half;
        frame = rec->wheel;
        alighting = rec->alighting;
        unloading = rec->unloading;
        docked = rec->docked;
        **g_bz_zspr->lls_holder = rec->zframe;
        tx = item->base_x + key->bx;
        ty = key->by + item->base_y;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                ty = (ty << 8) + 0xfa;
                tx = (tx << 8) - 0x9c;
                b->flags62 |= 8;
                b->target.x = tx;
                b->target.y = ty;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->wait = 500;
                b->action++;
                break;

            case 1:
                b->target.x = (tx << 8) - 0x100;
                b->target.y = (ty + 7) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 2:
                b->target.x = (tx - 2) << 8;
                b->target.y = (ty + 7) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 3:
                b->target.x = (tx - 2) << 8;
                b->target.y = (ty + 1) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 4:
                b->target.x = (tx - 4) << 8;
                b->target.y = (ty + 1) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 5:
                b->target.x = (tx - 6) << 8;
                b->target.y = (ty + 2) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 6:
                b->target.x = (tx << 8) - 0x632;
                b->target.y = (ty + 3) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                waiting++;
                break;

            case 7:
                b->dir = 3;
                if (docked == 1) {
                    b->target.x = (tx << 8) - 0x564;
                    b->target.y = (ty + 3) << 8;
                    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                    b->state = 7;
                    b->new_dir = a;
                    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                    at = Balloonz_CarAtPlatform(frame, half);
                    b->seat = (char)at;
                    cars.c[(unsigned char)at] = 1;
                    b->f58 = (rand() % 3 + 4) * 50;
                    riders++;
                    waiting--;
                    b->action++;
                }
                break;

            case 8:
                b->target.x = (tx - 5) << 8;
                b->target.y = (ty << 8) + 0x26a;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 9:
                b->flags62 |= 0x80;
                b->person->zsprite = g_bz_ridezspr;
                b->person->driven = 1;
                b->person->depth = GetUnitDepth(-1617692.375f, -1617904.25f);
                sprintf(name + 5, "%02d", b->seat);
                SetBlokePositionFromBNV(g_bz_bnv, b, name, 0,
                                        -1617692.375f, -1617904.25f, 0);
                b->action++;
                break;

            case 10:
                cars.c[b->seat] = 2;
                sprintf(name + 5, "%02d", b->seat);
                SetBlokePositionFromBNV(g_bz_bnv, b, name, frame + half * 24,
                                        -1617692.375f, -1617904.25f, 0);
                if (--b->f58 == 0)
                    b->action++;
                break;

            case 11:
                sprintf(name + 5, "%02d", b->seat);
                SetBlokePositionFromBNV(g_bz_bnv, b, name, frame + half * 24,
                                        -1617692.375f, -1617904.25f, 0);
                b->action++;
                cars.c[b->seat] = 3;
                alighting++;
                break;

            case 12:
                sprintf(name + 5, "%02d", b->seat);
                SetBlokePositionFromBNV(g_bz_bnv, b, name, frame + half * 24,
                                        -1617692.375f, -1617904.25f, 0);
                if (unloading == 1) {
                    idx = (char)Balloonz_CarAtPlatform(frame, half);
                    if (idx == b->seat) {
                        b->flags62 &= ~0x80;
                        b->target.x = (tx << 8) - 0x564;
                        b->target.y = (ty + 3) << 8;
                        b->new_dir = (unsigned char)
                            (CalcMoveLine(b->world, b->target, b->path) + 0x10);
                        b->state = 7;
                        b->person->zsprite = 0;
                        b->person->driven = 0;
                        NewDirForAction(b, (unsigned char)((b->new_dir >> 5) + 3));
                        b->action++;
                        cars.c[idx] = 1;
                        riders--;
                    }
                }
                break;

            case 13:
                b->target.x = (tx << 8) - 0x632;
                b->target.y = (ty + 3) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 14:
                b->target.x = (tx - 5) << 8;
                b->target.y = (ty + 9) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                cars.c[b->seat] = 0;
                alighting--;
                break;

            case 15:
                RemoveBlokeFromRide(item, r);
                b->flags62 &= ~8;
                break;
            }
        }

        rec->waiting = waiting;
        rec->riders = riders;
        rec->cars = cars;
        rec->half = half;
        rec->zframe = frame;
        rec->alighting = alighting;
        rec->docked = docked;
        rec->unloading = unloading;
        r = next;
    }
    }

    /* ---- PASS 2: turn every wheel ------------------------------------- */
    {
    char          at;
    char          wheel;
    int           hold;
    int           unloading;
    BalloonzRec*  rec;

    rec = g_bz_recs;
    while (rec) {
        hold = 0;
        waiting = rec->waiting;
        riders = rec->riders;
        cars = rec->cars;
        half = rec->half;
        wheel = rec->wheel;
        alighting = rec->alighting;
        unloading = rec->unloading;
        docked = 0;

        if (wheel % 8 == 0 || wheel == 0) {
            at = Balloonz_CarAtPlatform(wheel, half);
            if (waiting != 0 && riders < 6 && cars.c[at] == 0 && rand() % 3 == 0) {
                docked = 1;
                hold = 1;
            }
            if (cars.c[at] == 1)
                hold = 1;
            if (alighting != 0 && cars.c[at] == 3) {
                unloading = 1;
                hold = 1;
            }
        }
        if (riders != 0 && hold == 0) {
            if (++wheel > 0x17) {
                wheel = 0;
                if (++half > 1)
                    half = 0;
            }
            unloading = 0;
            docked = 0;
        }

        rec->riders = riders;
        rec->waiting = waiting;
        rec->cars = cars;
        rec->half = half;
        rec->wheel = wheel;
        rec->alighting = alighting;
        rec->docked = docked;
        rec->unloading = unloading;
        rec = rec->next;
    }
    }
}

/* =========================================================================
 * RESTAURANT 2
 *
 * The biggest handler in the 0x0042xxxx block (658 instructions) and the one
 * with the most state.  A RESTAURANT 2 has a THREE-SEAT table, a queue at the
 * door and a WAITER that walks a 32-step canned path out to the table and
 * back; the per-placement record (list head 0x00616148, found by
 * Restaurant2_FindRec 0x0042f9d0) carries all of it:
 *
 *   +0x00 RestRec2* next
 *   +0x04 u16   square
 *   +0x06 char  swing        door/serve dwell counter, 0..8   (pass 2 only)
 *   +0x07 char  step         the waiter's step along its path, 0..0x20
 *   +0x08 char  serve        serving dwell counter, 0..8
 *   +0x09 char  frame        the building's animation frame, wraps at 0x1f
 *                            (pass 2 only)
 *   +0x0c int   walking      customers walking in
 *   +0x10 char  queued       customers in the queue, capped at 3
 *   +0x11 char  seated       customers at the table, capped at 3
 *   +0x14 int   ready        customers that have sat down; forced to 3 when
 *                            the last one arrives                (pass 1 only)
 *   +0x18 int   phase        the RESTAURANT's own machine, 0..5
 *   +0x1c int   idle         idle frames before the waiter is sent out
 *   +0x20 int   seating      the "you may walk to a seat" gate
 *   +0x24 int   called       the waiter has been called
 *   +0x28 int   serving      the waiter is at the table
 *   +0x2c int   leaving      the "you may get up" gate
 *   +0x30 int   clearing     the "you may walk out" gate
 *   +0x34 int   returning    the waiter is on its way back    (pass 2 only)
 *   +0x38 int   waiter_x     the waiter's world position, 24.8
 *   +0x3c int   waiter_y
 *
 * The waiter's path is TWO int tables of 33 entries each, 0x004b685c (x step)
 * and 0x004b68e0 (y step), walked forward one entry per frame from
 * waiter_x = 0x143: `waiter_x -= dx[step]; waiter_y += dy[step]`.  The same
 * 0x004b68e0 table is the seat-offset table Restaurant2_SeatCustomer
 * (0x0042fa90) applies to a customer's world position, so the customers and
 * the waiter share one geometry table.
 *
 * Like BALLOONZ this is TWO passes over two different lists, each with its own
 * copy-in / copy-out of the record: pass 1 runs the CUSTOMER machine (16
 * actions) over the class rider list, pass 2 runs the RESTAURANT machine (6
 * phases) over the record list and advances the building's own animation
 * frame.  Pass 1 copies fifteen fields, pass 2 copies seventeen -- which is
 * why the frame is 0x44 bytes, the largest in the file.
 * ========================================================================= */

typedef struct RestRec2 {
    struct RestRec2* next;     /* +0x00 */
    unsigned short   square;   /* +0x04 */
    char             swing;    /* +0x06 */
    char             step;     /* +0x07 */
    char             serve;    /* +0x08 */
    char             frame;    /* +0x09 */
    unsigned char    pad0a[2];
    int              walking;  /* +0x0c */
    unsigned char    queued;   /* +0x10 */
    unsigned char    seated;   /* +0x11 */
    unsigned char    pad12[2];
    int              ready;    /* +0x14 */
    int              phase;    /* +0x18 */
    int              idle;     /* +0x1c */
    int              seating;  /* +0x20 */
    int              called;   /* +0x24 */
    int              serving;  /* +0x28 */
    int              leaving;  /* +0x2c */
    int              clearing; /* +0x30 */
    int              returning;/* +0x34 */
    int              waiter_x; /* +0x38 */
    int              waiter_y; /* +0x3c */
} RestRec2;

extern RestRec2* Restaurant2_FindRec(MapSquare* sq);                 /* 0x0042f9d0 */
/* Apply waypoint `step` of table `which` (1 = 0x4b6860, else 0x4b68e0) to the
 * rider's world position. */
extern void Restaurant2_SeatCustomer(RiderNode* r, int which, int step); /* 0x0042fa90 */
/* Start / stop the restaurant's sample, sourced at the packed map square. */
extern void Restaurant2_StartSound(unsigned short square);           /* 0x0042fb00 */
extern void Restaurant2_StopSound(unsigned short square);            /* 0x0042fb60 */

extern RestRec2* g_r2_recs;        /* 0x00616148 the per-placement list */
extern int       g_r2_path_dx[];   /* 0x004b685c the waiter's x steps */
extern int       g_r2_path_dy[];   /* 0x004b68e0 the waiter's y steps */

/* =========================================================================
 * 0x0042fbb0 -- RESTAURANT 2, the +0xa8 per-tick update.
 *
 * PASS 1, the CUSTOMER machine:
 *   0   claim the ride, walk to the door at (tx + 3.91, ty - 2), park the
 *       bloke's walk speed in +0x44 and slow it to 0x15, 300-tick patience,
 *       and count one more customer walking in
 *   1   join the queue: (tx + 3.91, ty - 4 + 100 * walking) -- the queue is
 *       spaced 100 world units per waiting customer
 *   2   only while fewer than three are queued AND the seating gate is open:
 *       step to (tx + 2.80, ty - 3.61) and count one more queued
 *   3   through the door, (tx + 1.41, ty - 3.61)
 *   4   to the table, (tx + 1.41, ty + 100*queued - 5.32); count this one out
 *       of `walking` and into `ready`, and when the third one arrives (or the
 *       last walker leaves) shut the seating gate and force ready = 3
 *   5   when three are ready open the `called` gate and reset ready; face the
 *       rider's 3D model direction 5 (SetPersonDirection)
 *   6   wait for the waiter: while the restaurant is serving and not yet at
 *       the table, keep applying seat waypoint `step`; otherwise advance
 *   7   wait for the "get up" gate, then walk to (tx - 7.61, ty - 12.61)
 *   8   walk to (tx - 10, ty - 12.61)
 *   9   count one out of the queue; the last one shuts the leaving gate
 *  10   the meal: count `wait` down and, at exactly 250, charge for it with
 *       BuyItem(elem, square, 1)
 *  11   sit at the table: teleport to (tx - 10.61, ty + 100*seated - 13.17)
 *       and count one more seated (only while phase <= 1 and fewer than 3)
 *  12   the long wait: once nobody is walking in, count 100 idle frames and
 *       then send the waiter (phase = 2, waiter_x = 0x143, everything reset);
 *       while the waiter is out, keep applying seat waypoint `step`
 *  13   wait for the "walk out" gate, then (tx - 2, ty - 4.41)
 *  14   walk off, (tx - 3, ty - 4.41), restore the parked walk speed and
 *       count one fewer seated
 *  15   leave the ride
 *
 * PASS 2, the RESTAURANT machine (phase, +0x18):
 *   0   idle: once somebody is walking in, dwell 8 frames, then open the
 *       seating gate and go to phase 1
 *   1   once called, count the serve dwell back down; when it goes negative
 *       start the sample, reset the waiter to step 0 / x = 0x143 and go to
 *       phase 2
 *   2   the waiter walks its 33-step path (waiter_x -= dx[step],
 *       waiter_y += dy[step]); past step 0x20 it stops the sample, marks
 *       itself serving and goes to phase 4
 *   3   the waiter walks BACK (step counts down); at the end it stops the
 *       sample and resets to phase 0
 *   4   at the table: dwell 8 frames, then open the leaving gate and the
 *       walk-out gate and go to phase 5
 *   5   once the queue and the table are both empty, close both gates, count
 *       the dwell back down and then start the sample and go to phase 3
 * Every pass-2 iteration also advances the building's own frame (0..0x1f).
 * ========================================================================= */

/* CLOSED 2026-09-04 (262 -> 0).  Five levers, in the order they were found;
 * every one of them is worth trying on the other +0xa8 handlers:
 *
 *  1. The two sound helpers take an `unsigned short`, NOT an int (262 -> 56
 *     on its own).  The original loads the square with `mov dx, word ptr
 *     [esi+4]` and pushes edx with the top half still dirty; an `int`
 *     parameter makes VC6 zero-extend first (`xor edx,edx`), which is four
 *     extra instructions across the four call sites.  Extern prototype types
 *     really are caller-side codegen levers.
 *  2. Pass 2's six phase blocks are laid out in the source order 0,1,2,4,5,3
 *     -- VC6 emits switch case blocks in SOURCE order, and the original's
 *     layout has case 3 last (245 vs 260 for 0,1,2,4,3,5).
 *  3. Every phase's counter test is written with the COUNTER STEP as the
 *     `if` arm and the state change as the `else` (`if (s <= 8) serve = s+1;
 *     else {...}`), so the step falls through and the state change is jumped
 *     to.  Written the other way round VC6 inverts the whole block and, in
 *     four of the six cases, folds the step into a memory `inc`/`dec` instead
 *     of the original's load/step/store through a register.
 *  4. Action 12's three-way join needs `if (phase == 2 && serving == 0) goto
 *     seat; if (phase != 2) goto notserving; goto advance;` -- the redundant
 *     second phase test is what makes VC6 put the join BEFORE the seat call
 *     and reproduce the original's two-entry block (`je 0x430036` enters at
 *     the `xor eax,eax`, `jne 0x430038` enters one instruction later, because
 *     the phase!=0 path already has eax = 0).  The straight
 *     `if (phase != 2) .. if (serving != 0) ..` spelling emits the seat call
 *     first and jumps backwards into it.
 *  5. `key` is taken from the rider BEFORE `b = r->bloke` (40 -> 22 and the
 *     byte count exact).  That keeps the cursor in eax and gives the
 *     original's `lea ebp,[eax+0xc]`; with `b` first VC6 puts the cursor in
 *     ebp and consumes it with `add ebp,0xc`.
 *  Plus: `if (!(queued > 0) && !(seated > 0))` for the two `unsigned char`
 *  flags (the original tests them with `test cl,cl / ja`, not `jne`), and the
 *  four record copy-in / copy-out blocks in the store orders below, which are
 *  the hill-climbed optima (scratchpad/ridecb3/climb.py) -- the last two
 *  mismatches were `b->speed` and `b->action++` the wrong way round in
 *  action 14.
 * Structure facts: pass 1 and pass 2 share ONE set of function-level locals
 * (the original colours pass 2's `returning` onto pass 1's rider slot and its
 * animation frame onto the dead `elem` argument slot). */
// FUNCTION: LEGOLAND 0x0042fbb0
void Restaurant2_Tick(RideElem* elem)
{
    char          step;
    char          serve;
    int           walking;
    unsigned char queued;
    unsigned char seated;
    int           leaving;
    int           seating;
    int           waiter_y;
    int           ready;
    int           serving;
    int           phase;
    int           idle;
    int           called;
    int           clearing;
    int           waiter_x;
    RiderNode*    r;
    RideObject*   item = elem->data;
    RiderNode*    next;
    RestRec2*     rec;
    char          swing;
    char          frame;
    int           returning;
    MapSquare*    key;
    Bloke*        b;
    int           tx;
    int           ty;
    unsigned char a;

    r = item->riders;
    while (r) {
        next = r->next;
        key = (MapSquare*)&r->ride_id;
        b = r->bloke;
        rec = Restaurant2_FindRec(key);
        if (!rec)
            return;
        step = rec->step;
        serve = rec->serve;
        walking = rec->walking;
        queued = rec->queued;
        seated = rec->seated;
        ready = rec->ready;
        phase = rec->phase;
        idle = rec->idle;
        seating = rec->seating;
        called = rec->called;
        serving = rec->serving;
        leaving = rec->leaving;
        clearing = rec->clearing;
        waiter_x = rec->waiter_x;
        waiter_y = rec->waiter_y;
        tx = item->base_x + key->bx;
        ty = key->by + item->base_y;

        if (b->state == 0) {
            switch (b->action) {
            case 0:
                b->flags62 |= 8;
                b->target.x = (tx << 8) + 0x3c8;
                b->target.y = (ty - 2) << 8;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->wait = 300;
                b->saved_speed = b->speed;
                b->speed = 0x15;
                b->action++;
                walking++;
                break;

            case 1:
                b->target.x = (tx << 8) + 0x3c8;
                b->target.y = ((ty - 4) << 8) + walking * 100;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 2:
                if (queued < 3 && seating != 0) {
                    b->target.y = (ty << 8) - 0x39c;
                    b->target.x = (tx << 8) + 0x2ce;
                    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                    b->state = 7;
                    b->new_dir = a;
                    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                    b->action++;
                    queued++;
                }
                break;

            case 3:
                b->target.x = (tx << 8) + 0x16a;
                b->target.y = (ty << 8) - 0x39c;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 4:
                b->target.x = (tx << 8) + 0x16a;
                b->target.y = (ty << 8) + queued * 100 - 0x532;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                walking--;
                ready++;
                if (queued == 3 || walking == 0) {
                    seating = 0;
                    ready = 3;
                }
                break;

            case 5:
                if (ready >= 3) {
                    called = 1;
                    ready = 0;
                }
                SetPersonDirection(r->owner, 5);
                b->action++;
                break;

            case 6:
                if (phase == 2) {
                    if (serving == 0) {
                        Restaurant2_SeatCustomer(r, 1, step);
                        break;
                    }
                } else if (serving == 0) {
                    break;
                }
                b->action++;
                break;

            case 7:
                if (leaving != 0) {
                    b->target.x = (tx << 8) - 0x79c;
                    b->target.y = (ty << 8) - 0xc9c;
                    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                    b->state = 7;
                    b->new_dir = a;
                    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                    b->action++;
                }
                break;

            case 8:
                b->target.x = (tx - 10) << 8;
                b->target.y = (ty << 8) - 0xc9c;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->action++;
                break;

            case 9:
                if (queued-- == 0)
                    leaving = 0;
                b->action++;
                break;

            case 10:
                if (b->wait-- < 0) {
                    b->action++;
                    break;
                }
                if (b->wait == 250)
                    BuyItem(elem, key, 1);
                break;

            case 11:
                if (phase <= 1 && seated < 3) {
                    b->world.x = (tx << 8) - 0xa9c;
                    b->world.y = (ty << 8) + seated * 100 - 0xd2c;
                    b->action++;
                    seated++;
                }
                break;

            case 12:
                if (seated != 0 && phase == 0) {
                    if (walking != 0)
                        goto r2_notserving;
                    if (idle++ > 100) {
                        phase = 2;
                        step = 0;
                        waiter_x = 0x143;
                        waiter_y = 0;
                        leaving = 0;
                        serving = 0;
                        goto r2_seat;
                    }
                    goto r2_notserving;
                }
                /* The phase test is deliberately written twice: it is what
                 * makes VC6 emit this join BEFORE the seat call and enter it
                 * at two different instructions (see the note above).  Do not
                 * "simplify" to if (phase != 2) .. if (serving != 0) .. -- that
                 * spelling puts the call first and jumps backwards into it. */
                if (phase == 2 && serving == 0)
                    goto r2_seat;
                if (phase != 2)
                    goto r2_notserving;
                goto r2_advance;
r2_seat:
                Restaurant2_SeatCustomer(r, 2, step);
                break;
r2_notserving:
                if (serving == 0)
                    break;
r2_advance:
                b->action++;
                break;

            case 13:
                if (clearing != 0) {
                    b->target.x = (tx - 2) << 8;
                    b->target.y = (ty << 8) - 0x46a;
                    a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                    b->state = 7;
                    b->new_dir = a;
                    NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                    b->action++;
                }
                break;

            case 14:
                b->target.x = (tx - 3) << 8;
                b->target.y = (ty << 8) - 0x46a;
                a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = a;
                NewDirForAction(b, (unsigned char)((a >> 5) + 3));
                b->speed = (unsigned char)b->saved_speed;
                b->action++;
                seated--;
                break;

            case 15:
                RemoveBlokeFromRide(item, r);
                b->flags62 &= ~8;
                break;
            }
        }

        rec->step = step;
        rec->serve = serve;
        rec->walking = walking;
        rec->queued = queued;
        rec->seated = seated;
        rec->ready = ready;
        rec->phase = phase;
        rec->idle = idle;
        rec->seating = seating;
        rec->called = called;
        rec->serving = serving;
        rec->leaving = leaving;
        rec->clearing = clearing;
        rec->waiter_x = waiter_x;
        rec->waiter_y = waiter_y;
        r = next;
    }

    /* ---- PASS 2: the restaurants themselves --------------------------- */
    rec = g_r2_recs;
    while (rec) {
        step = rec->step;
        frame = rec->frame;
        serve = rec->serve;
        walking = rec->walking;
        queued = rec->queued;
        seated = rec->seated;
        idle = rec->idle;
        phase = rec->phase;
        seating = rec->seating;
        called = rec->called;
        leaving = rec->leaving;
        swing = rec->swing;
        serving = rec->serving;
        clearing = rec->clearing;
        returning = rec->returning;
        waiter_x = rec->waiter_x;
        waiter_y = rec->waiter_y;

        switch (phase) {
        case 0:
            if (walking != 0) {
                char s = serve;
                if (s <= 8) {
                    serve = (char)(s + 1);
                } else {
                    phase = 1;
                    serve = 8;
                    seating = 1;
                }
            }
            break;

        case 1:
            if (called != 0) {
                char s = serve;
                seating = 0;
                if (s >= 0) {
                    serve = (char)(s - 1);
                } else {
                    swing = 0;
                    called = 0;
                    phase = 2;
                    step = 0;
                    waiter_x = 0x143;
                    waiter_y = 0;
                    Restaurant2_StartSound(rec->square);
                }
            }
            break;

        case 2:
            if (step <= 0x20) {
                step++;
                waiter_x -= g_r2_path_dx[step];
                waiter_y += g_r2_path_dy[step];
            } else {
                serving = 1;
                idle = 0;
                phase = 4;
                swing = 0;
                Restaurant2_StopSound(rec->square);
            }
            break;

        case 4:
            if (swing <= 8) {
                swing++;
            } else {
                phase = 5;
                leaving = 1;
                clearing = 1;
                swing = 8;
            }
            break;

        case 5:
            /* `!(x > 0)` and not `x == 0`: both flags are unsigned char and
             * the original tests them with `test cl,cl / ja`, not `jne`. */
            if (!(queued > 0) && !(seated > 0)) {
                clearing = 0;
                leaving = 0;
                if (swing >= 0) {
                    swing--;
                } else {
                    returning = 1;
                    phase = 3;
                    step = 0x20;
                    Restaurant2_StartSound(rec->square);
                }
            }
            break;        case 3:
            if (returning != 0) {
                if (step >= 0) {
                    serving = 0;
                    step--;
                } else {
                    serving = 0;
                    phase = 0;
                    swing = 0;
                    returning = 0;
                    step = 0;
                    serve = 0;
                    Restaurant2_StopSound(rec->square);
                }
            }
            break;


        }

        if (++frame > 0x1f)
            frame = 0;

        rec->step = step;
        rec->serve = serve;
        rec->frame = frame;
        rec->walking = walking;
        rec->queued = queued;
        rec->seated = seated;
        rec->idle = idle;
        rec->seating = seating;
        rec->called = called;
        rec->leaving = leaving;
        rec->clearing = clearing;
        rec->swing = swing;
        rec->phase = phase;
        rec->serving = serving;
        rec->returning = returning;
        rec->waiter_x = waiter_x;
        rec->waiter_y = waiter_y;
        rec = rec->next;
    }
}
