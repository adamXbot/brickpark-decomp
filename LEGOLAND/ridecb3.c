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
#ifndef LEGOLAND_PORTABLE
extern void   ScreenToMapRef(Offset* screen, Pos* out, int mode);    /* 0x0045be90 */
#else
extern int ScreenToMapRef(Offset* screen, Pos* out, int mode);    /* 0x0045be90 */
#endif
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

/* Scope F close (2026-09-05): 378/378 instructions,
 * 1225/1225 bytes, strict/rb/ob 0/0/0.
 *
 * Keep the BNV position objects inside their owning cases. Their earlier
 * function-wide lifetime falsely constrained stores before the first escape;
 * narrowing it restores the person/flag hoists and departure short-load order.
 * The separate person-cache workaround is no longer needed.
 *
 * Keep the unshifted y product in sy, with an empty-if consumer BOTH before
 * and after sy2 = sy >> 9. Then accumulate the origin-minus-scroll delta
 * into sy2, retaining the third empty-if before the pivot subtractions.
 * The two early consumers vanish only after the compiler has kept the
 * multiplication/shift webs separate. Both are required: neither or only
 * the post-shift consumer gives Spider 34; only the pre-shift one gives 343;
 * both give zero, and the same shape closes all five BNV ride callbacks.
 *
 * The 12-byte BNV seeds still leave z uninitialized, as the original does.
 * The eight-byte spill object and its single volatile sx store reproduce
 * the original dead store and unused frame dword; do not remove them.
 * All earlier scope, register-rotation and departure-floor claims in
 * this function's former notes are superseded. See docs/lanes/scope-f.md. */
// FUNCTION: LEGOLAND 0x0042c820
void Carousel_Tick(RideElem* elem)
{
    int           sy;
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
    RiderNode*    r;
    Bloke*        b;
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

            case 1: {
                Vec3 pos;
                screen = GetScreenCoordsForObject(key, item);
                wy = b->world.y;
                wx = b->world.x;
                GetTileDimensions(&tw, &th);
                sx = (wx - wy) * tw >> 9;
                sy = (wx + wy) * th;
                if (sy) { }
                sy2 = sy >> 9;
                if (sy2) { }
                /* the original's apparently dead `mov [esp+0x24], ebp`:
                 * sx is homed here as it dies into sx2 (volatile so VC6
                 * cannot delete the store). */
                *(volatile int*)&spill.x = sx;
                sx2 = g_map_cfg->ox - Get_XScroll() + sx;
                sy2 += g_map_cfg->oy - Get_YScroll();
                if (sy2) { }
                sx2 -= g_carousel_dx / 2;
                sx2 -= screen.ox;
                sy2 -= g_carousel_dy / 2;
                sy2 -= screen.oy;
                /* The case-local seed permits the original person-load hoist. */
                pos.x = sx2 * 2;
                pos.y = sy2 * 2;
                b->person->zsprite = g_carousel_zspr;
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
            }

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

            case 7: {
                Vec3 pos2;
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
            }

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

/* Scope F close (2026-09-05): 637/637 instructions, 1993/1993 bytes,
 * strict/rb/ob 0/0/0. Declare name[8] at FUNCTION scope, with no initializer,
 * and memcpy the literal AFTER r = item->riders inside the first block.
 * This combination reproduces the complete entry schedule: load riders,
 * store name's low dword, test, store cursor, store name's high dword.
 * Function-scope initialization before the read gives 9 mismatches; keeping
 * the buffer block-local gives 5 with either copy placement. The two changes
 * must be combined. This supersedes all earlier scheduling-floor claims.
 *
 * The experiment that exposed it grouped cars/name at their original frame
 * offsets and copied after the read: the entry matched but escaped cars
 * changed later code. Separating cars again while retaining name's FUNCTION
 * scope and the copy's AFTER-read placement closes the complete function.
 *
 * Other established reconstruction details remain load-bearing:
 * - alighting/half/waiting/docked/cars/riders are one set shared by both
 *   passes; the other pass locals stay in separate blocks.
 * - pass 1's at is int, pass 2's at is signed char.
 * - read docked before the z-sprite store; take key before the bloke read.
 * - keep pass 2's hold=0 first and docked=0 last around the copy-in sequence;
 *   this emits the two original immediate zero stores.
 * Whole-file audit keeps all 7 previously exact functions and adds this one.
 * See docs/lanes/scope-f.md for the measured scope/placement matrix. */
// FUNCTION: LEGOLAND 0x0042aa90
void Balloonz_Tick(RideElem* elem)
{
    RideObject*   item = elem->data;

    char          alighting;
    char          half;
    int           waiting;
    int           docked;
    char          name[8];
    BzCars        cars;
    char          riders;
    {
    int           at;
    char          frame;
    RiderNode*    r;
    int           unloading;
    RiderNode*    next;
    Bloke*        b;
    BalloonzRec*  rec;
    MapSquare*    key;
    int           tx;
    int           ty;
    int           idx;
    unsigned char a;

    r = item->riders;
    /* Keep the copy after the first list read; name lives at function scope. */
    memcpy(name, "Bloke??", sizeof(name));
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
