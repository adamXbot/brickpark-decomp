#pragma intrinsic(memcmp)
int memcmp(const void*, const void*, unsigned);
/* LEGOLAND -- Codex-F: copter and space-tower rider/car updates.
 * VC6 SP3 /O2 /Gy /Gd. Types mirror ridemachine.c (Codex-E).
 * Verification and recovered mechanics: docs/lanes/codex-f.md.
 */
typedef union RideTile { unsigned short key; struct { unsigned char x, y; } b; } RideTile;
typedef struct Offset { int ox, oy; } Offset;
typedef struct Bloke {
    char pad00[0x36];
    unsigned char seat;          /* +0x36 */
    char pad37[0x62 - 0x37];
    unsigned short flags;        /* +0x62; 0x80 = riding */
} Bloke;
typedef struct RiderNode {
    struct RiderNode *next, *prev;
    Bloke* bloke;
    unsigned short ride_id;      /* +0x0c packed map square */
    unsigned short pad0e;
    void* person;                /* +0x10 */
} RiderNode;
typedef struct SoundSource { int kind; void* person; int x, y; } SoundSource;
typedef struct TowerCar {
    unsigned flags;
    int f04, height, state, f10, f14;
    RiderNode *rider_a, *rider_b;
    signed char revs;
    char pad21[3];
} TowerCar; /* 0x24 */
typedef struct TowerRec {
    RideTile tile;
    unsigned char seated, riders, joined, pad05[3];
    struct TowerRec* next;
    unsigned flags;
    int cycle;
    TowerCar car[4];
} TowerRec;
typedef struct TowerCarGeom {
    Offset side[2];              /* +0 / +8 screen seat pairs */
    int dir;                     /* +0x10 */
} TowerCarGeom; /* 20 */
typedef struct RideDef {
    char pad00[0xcc];
    RiderNode* riders;           /* +0xcc */
} RideDef;
typedef struct CopterSeat {
    unsigned flags;
    signed char frame;
    char pad05[3];
    int entry_layer, ride_layer, entry_path, ride_path;
    RiderNode* rider;
    signed char frames;
    signed char stage;
    char pad1e[2];
} CopterSeat; /* 0x20 */
typedef struct CoptersRec {
    RideTile tile;
    unsigned char seated, riders;
    struct CoptersRec* next;
    unsigned flags;
    int mode;
    unsigned char joined, pad11[3];
    int timer;
    CopterSeat seat[6];
} CoptersRec;

extern void UnSourceAndFadeAllSamplesFromSource(SoundSource*, int); /* 0x00496c80 */
extern void* PlayInstanceOfSample(void*, int, int, SoundSource*); /* 0x00496d20 */
extern void* g_copters_sample;                      /* 0x004b4160 */

extern RideDef* g_spacetower_def;                   /* 0x0062fd74 */
extern RideDef* g_copters_def;                      /* 0x004c1198 */
extern void* g_copters_layers;                      /* 0x004c1138 */
extern Offset g_spacetower_car_ofs[4];              /* 0x0062fd88 */
extern TowerCarGeom g_tower_car_geom[4];            /* 0x004b7798 */
extern Offset GetScreenCoordsForObject(void*, RideDef*); /* 0x00442cc0 */
extern void AdjustOffsetForViewMode(Offset*);       /* 0x00442d30 */
extern void AdjustBlokePosition(Offset*);           /* 0x00442d60 */
extern void SetPersonPosition(void*, int, int);     /* 0x00440190 */
extern void SetPersonDirection(void*, int);         /* 0x004400b0 */
extern Offset GetRenderOffsetForLayer(void*, int);  /* 0x00441ee0 */
extern void* GetSpriteForLayer(void*, int);         /* 0x00441ec0 */
typedef struct SprHdr { char pad[0x14]; short dim; } SprHdr;
typedef struct PosTable { char pad[0x24]; float** slots; } PosTable;
extern PosTable* g_copters_postable;               /* 0x00830f98; +0x24 = per-anim POS frame arrays */

/* In-place float*k -> int through the game's masked-FPU fistp (no __ftol). */
#ifndef LEGOLAND_PORTABLE
#define FSCALEF(x, k) __asm { fld x } __asm { fmul k } __asm { fistp x }
#else
#define FSCALEF(x, k) LL_FISTP_SCALE_INPLACE(x, k)
#endif

extern int g_copter_ord_a[3];                       /* 0x004b42a0 = {0,1,2} */
extern int g_copter_ord_b[3];                       /* 0x004b42ac = {0,2,1} */
extern int g_copter_sign_a[3];                      /* 0x004b42b8 = {1,-1,-1} */
extern int g_copter_sign_b[3];                      /* 0x004b42c4 = {-1,1,1} */

typedef struct Person3D { char pad[0x58]; int matrix[9]; } Person3D;
#define RIDE_TILE(r) ((RideTile*)&(r)->ride_id)

/* Advance one copter car: bump frame, wrap, then stage; clear flying at 0. */
// FUNCTION: LEGOLAND 0x00404860
void Copters_StepCar(CoptersRec* rec, int index)
{
    CopterSeat* car = &rec->seat[index];
    if (car->flags & 1) {
        car->frame++;
        if (car->frame >= car->frames) {
            car->frame = 0;
            car->stage--;
            if (car->stage < 0)
                car->flags &= ~1u;
        }
    }
}

/* Stop the ride: clear flying flags, drop riders, park frames at frames-1,
 * clear ride bits 0x4001. Every per-seat group is in the copter enumeration
 * order 1,0,2,3,4 (ridemachine.c). silent!=0 skips the fade/play pair
 * (InitRecord path).
 * Levers: plain `&= ~1u` statements in 1,0,2,3,4 order -- VC6 pairs them
 * (edx,ebx) and emits the SECOND of each pair first, so the emitted
 * 0/1,3/2 picture is source order 1,0,2,3,4; explicit edx/ebx temporaries
 * in emitted order come out mirrored. The `xor ebx,ebx` sits right after
 * the seat[4] flags store only when the rider stores PRECEDE the frame
 * stores in source (frames-then-riders puts it after the first `dec`). */
// FUNCTION: LEGOLAND 0x004049a0
void Copters_StopRide(CoptersRec* rec, int silent)
{
    rec->seat[1].flags &= ~1u;
    rec->seat[0].flags &= ~1u;
    rec->seat[2].flags &= ~1u;
    rec->seat[3].flags &= ~1u;
    rec->seat[4].flags &= ~1u;
    rec->seat[1].rider = 0;
    rec->seat[0].rider = 0;
    rec->seat[2].rider = 0;
    rec->seat[3].rider = 0;
    rec->seat[4].rider = 0;
    rec->seat[1].frame = (signed char)(rec->seat[1].frames - 1);
    rec->seat[0].frame = (signed char)(rec->seat[0].frames - 1);
    rec->seat[2].frame = (signed char)(rec->seat[2].frames - 1);
    rec->seat[3].frame = (signed char)(rec->seat[3].frames - 1);
    rec->seat[4].frame = (signed char)(rec->seat[4].frames - 1);
    rec->mode = 0;
    rec->joined = 0;
    rec->seated = 0;
    rec->flags &= ~0x4001u;
    if (silent == 0) {
        SoundSource source;
        source.kind = 2;
        source.x = rec->tile.b.x;
        source.y = rec->tile.b.y;
        UnSourceAndFadeAllSamplesFromSource(&source, -200);
        PlayInstanceOfSample(g_copters_sample, 0, 1, &source);
    }
}

// FUNCTION: LEGOLAND 0x0043a940
void SpaceTower_StepCar(TowerCar* car)
{
    if (!(car->flags & 1))
        return;
    switch (car->state) {
    case 2:
        if (car->f14 == 0) {
            car->height += car->revs;
            if (car->height > 0xc8) {
                car->height = 0xc8;
                car->f14 = 1;
            }
        } else {
            car->height += -2;
            if (car->height < 0) {
                car->height = 0;
                car->f14 = 0;
                car->state = 0;
            }
        }
        break;
    case 1:
        if (car->f10 == 0)
            car->f10 = -1;
        else
            car->state = 2;
        break;
    }
}

/* Bind riding blokes to car seat pointers and park their 3D persons on the
 * car anchor plus the per-side screen seat pair from g_tower_car_geom.
 * Levers: memcmp(&ride_id) for lea-before-cmp (RC01); screen+car_ofs add
 * order for the dirty-stack pos store before AdjustOffset cleanup. */
// FUNCTION: LEGOLAND 0x0043b810
void SpaceTower_UpdateRiders(TowerRec* rec)
{
    RiderNode* rider = g_spacetower_def->riders;
    Offset screen;
    Offset car_ofs;
    Offset seat_ofs;
    Offset pos;
    void* person;
    int car;
    int side;

    rec->car[0].rider_a = 0;
    rec->car[0].rider_b = 0;
    rec->car[1].rider_a = 0;
    rec->car[1].rider_b = 0;
    rec->car[2].rider_a = 0;
    rec->car[2].rider_b = 0;
    rec->car[3].rider_a = 0;
    rec->car[3].rider_b = 0;

    screen = GetScreenCoordsForObject(rec, g_spacetower_def);
    if (rider == 0)
        return;

    do {
        if (memcmp(RIDE_TILE(rider), &rec->tile, 2) == 0) {
            Bloke* b = rider->bloke;
            if (b->flags & 0x80) {
                unsigned seat = b->seat;
                car = (int)seat >> 1;
                side = (int)seat & 1;
                if (side == 0)
                    rec->car[car].rider_a = rider;
                else
                    rec->car[car].rider_b = rider;

                car_ofs = g_spacetower_car_ofs[car];
                car_ofs.oy -= rec->car[car].height;
                AdjustOffsetForViewMode(&car_ofs);
                pos.ox = screen.ox + car_ofs.ox;
                pos.oy = screen.oy + car_ofs.oy;
                if (side == 0)
                    seat_ofs = g_tower_car_geom[car].side[0];
                else
                    seat_ofs = g_tower_car_geom[car].side[1];
                AdjustOffsetForViewMode(&seat_ofs);
                pos.ox += seat_ofs.ox;
                pos.oy += seat_ofs.oy;
                AdjustBlokePosition(&pos);
                person = rider->person;
                SetPersonPosition(person, pos.ox, pos.oy);
                SetPersonDirection(person, g_tower_car_geom[car].dir);
            }
        }
        rider = rider->next;
    } while (rider != 0);
}

/* Place one seated copter rider: screen+layer offset, POS-frame lift, then
 * bake a signed fixed-point 3x3 into person->matrix from the POS matrix.
 *
 * Jump table (NOT identity): 0->(3,0xd7) 1->(0,0xeb) 2->(4,0xe1)
 * 3->(1,0xe6) 4->(2,0xe6).  The POS slot for an anim is an array of
 * 12-float frames (pos, a, b, n); the frame's normal is rebuilt as b x a
 * before the bake, and the bake reads vector 1+oa of the frame.
 *
 * Two zero-cost levers close this body; both are documented in
 * docs/lanes/codex-f.md (ninth sweep) and generalise to other lanes:
 *
 *  - `kg = kf` and reading four of the twelve cross-product operands through
 *    the alias.  VC6 canonicalises a commutative `fmul` whose operands are
 *    two constant offsets off ONE pointer, which reversed the fld/fmul order
 *    of the two products containing kf[8]; two different base pointers can
 *    no longer be ordered, so source order survives.  Source operand order
 *    is itself inert -- all 64 permutations measured.
 *
 *  - The scope-V cancelled pair anchors on the link-time address constant
 *    `(int)g_copter_ord_a`, hoisted out of the loop.  An anchor receives an
 *    allocator priority bump, so it must not be a value whose ranking
 *    matters: anchoring on `oa` ranked oa above the store cursor and cost an
 *    11-line register swap, and anchoring on `anim` rotated the callee-saved
 *    trio instead.  An address constant is already materialised and bumps
 *    nothing.  (The constant must be assigned to the struct MEMBER first --
 *    a constant used directly in the cancel folds in the front end.) */
// FUNCTION: LEGOLAND 0x00404630
void Copters_UpdateCarRider(CoptersRec* rec, int index)
{
    CopterSeat* seat = &rec->seat[index];
    Offset screen;
    Offset layer_ofs;
    Offset ofs;
    Offset pos;
    SprHdr* sprite;
    Person3D* person;
    float* kf1;
    float scale;
    float f;
    int anim;
    int mode;
    int layer;
    float* kf;
    float* kg;
    int i, j;
    int oa;
    Offset t;

    screen = GetScreenCoordsForObject(rec, g_copters_def);
    if (seat->rider == 0)
        return;

    layer = seat->ride_layer;
    anim = 1;
    switch (index) {
    case 1: anim = 0; mode = 0xeb; break;
    case 3: anim = 1; mode = 0xe6; break;
    case 4: anim = 2; mode = 0xe6; break;
    case 0: anim = 3; mode = 0xd7; break;
    case 2: anim = 4; mode = 0xe1; break;
    }

    layer_ofs = GetRenderOffsetForLayer(g_copters_layers, layer);
    sprite = (SprHdr*)GetSpriteForLayer(g_copters_layers, layer);
    AdjustOffsetForViewMode(&layer_ofs);

    kf1 = (float*)((char*)g_copters_postable->slots[anim] + seat->frame * 0x30);
#ifndef LEGOLAND_PORTABLE
    ofs.oy = (int)kf1[1] + mode;
#else
    ofs.oy = LL_FISTP(kf1[1]) + mode;   /* PORT-M5: 0x00404705 rounds */
#endif
    AdjustOffsetForViewMode(&ofs);
    ofs.ox = sprite->dim >> 1;
    pos.ox = layer_ofs.ox + screen.ox + ofs.ox;
    pos.oy = layer_ofs.oy + screen.oy + ofs.oy;
    AdjustBlokePosition(&pos);
    person = (Person3D*)seat->rider->person;
    SetPersonPosition(person, pos.ox, pos.oy);

    scale = 65536.0f;
    kf = (float*)((char*)g_copters_postable->slots[anim] + seat->frame * 0x30);
    kg = kf;
    kf[9] = kf[5] * kf[7] - kg[8] * kg[4];
    kf[10] = kg[8] * kg[3] - kf[6] * kf[5];
    kf[11] = kf[6] * kf[4] - kg[3] * kg[7];

    t.oy = (int)g_copter_ord_a;
    for (i = 0; i < 3; i++) {
        oa = g_copter_ord_a[i];
        for (j = 0; j < 3; j++) {
            t.ox = (oa + seat->frame * 4 + 1) * 3 + g_copter_ord_b[j];
            t.ox += t.oy;
            t.ox -= t.oy;
            f = g_copters_postable->slots[anim][t.ox];
            FSCALEF(f, scale);
            person->matrix[j * 3 + i] = g_copter_sign_a[j] * g_copter_sign_b[oa] * *(int*)&f;
        }
    }
}
