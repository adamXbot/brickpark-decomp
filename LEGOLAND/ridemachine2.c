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
extern void* g_copters_postable;                   /* 0x00830f98; +0x24 = anim slots */

extern int g_copter_ord_a[3];                       /* 0x004b42a0 = {0,1,2} */
extern int g_copter_ord_b[3];                       /* 0x004b42ac = {0,2,1} */
extern int g_copter_sign_a[3];                      /* 0x004b42b8 = {1,-1,-1} */
extern int g_copter_sign_b[3];                      /* 0x004b42c4 = {-1,1,1} */

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
 * bake a signed fixed-point 3x3 into person+0x58 from the POS matrix.
 *
 * Jump table (NOT identity): 0->(3,0xd7) 1->(0,0xeb) 2->(4,0xe1)
 * 3->(1,0xe6) 4->(2,0xe6).
 *
 * Levers recovered:
 *  - `*(CopterSeat* volatile*)&rec = seat` keeps seat in [ebp+8]
 *  - `sel = *(volatile int*)&index` + mode via `*(volatile*)&index`
 *    yields ebx=anim, [ebp+0xc]=mode, jmp [eax*4], esi=layer
 *  - residual: prologue wants `lea edi,[ecx+eax+0x18]` with early def
 *    load; volatile seat store currently splits the lea. Plain assign
 *    raises LCS (~20-24%) but steals anim into edi. */
// WIP-FUNCTION: LEGOLAND 0x00404630
void Copters_UpdateCarRider(CoptersRec* rec, int index)
{
    Offset screen;
    Offset layer_ofs;
    Offset lift;
    Offset pos;
    CoptersRec* inst;
    void* sprite;
    void* person;
    float* kf;
    float scale;
    int anim;
    int layer;
    void** slots;
    int* out_base;
    int* ord;
    typedef struct SprHdr { char pad[0x14]; short dim; } SprHdr;
#define SEAT ((CopterSeat*)rec)

    inst = rec;
    *(CopterSeat* volatile*)&rec = &inst->seat[index];
    screen = GetScreenCoordsForObject(inst, g_copters_def);
    if (SEAT->rider == 0)
        return;

    layer = SEAT->ride_layer;
    {
        int sel = *(volatile int*)&index;
        anim = 1;
        switch (sel) {
        case 1: anim = 0; *(volatile int*)&index = 0xeb; break;
        case 3: anim = 1; *(volatile int*)&index = 0xe6; break;
        case 4: anim = 2; *(volatile int*)&index = 0xe6; break;
        case 0: anim = 3; *(volatile int*)&index = 0xd7; break;
        case 2: anim = 4; *(volatile int*)&index = 0xe1; break;
        }
    }

    layer_ofs = GetRenderOffsetForLayer(g_copters_layers, layer);
    sprite = GetSpriteForLayer(g_copters_layers, layer);
    AdjustOffsetForViewMode(&layer_ofs);

    slots = *(void***)((char*)g_copters_postable + 0x24);
    kf = (float*)((char*)slots[anim] + SEAT->frame * 0x30);
    lift.oy = (int)kf[1] + index;
    AdjustOffsetForViewMode(&lift);

    pos.ox = screen.ox + layer_ofs.ox + (((SprHdr*)sprite)->dim >> 1);
    pos.oy = screen.oy + layer_ofs.oy + lift.oy;
    AdjustBlokePosition(&pos);
    person = SEAT->rider->person;
    SetPersonPosition(person, pos.ox, pos.oy);

    scale = 65536.0f;
    kf = (float*)((char*)slots[anim] + SEAT->frame * 0x30);
    {
        float a = kf[5] * kf[7];
        float b = kf[8] * kf[4];
        kf[9] = a - b;
        a = kf[8] * kf[3];
        b = kf[6] * kf[5];
        kf[10] = a - b;
        a = kf[6] * kf[4];
        b = kf[3] * kf[7];
        kf[11] = a - b;
    }

    ord = g_copter_ord_a;
    out_base = (int*)((char*)person + 0x58);
    while (ord < g_copter_ord_b) {
        int oa = *ord;
        int* out = out_base;
        int off;
        for (off = 0; off < 0xc; off += 4) {
            int idx, v;
            float f;
            idx = (oa + SEAT->frame * 4 + 1) * 3
                + *(int*)((char*)g_copter_ord_b + off);
            f = ((float*)slots[anim])[idx];
            *(float*)&index = f;
            *(float*)&index = *(float*)&index * scale;
            __asm {
                fld dword ptr [index]
                fistp dword ptr [index]
            }
            v = index;
            *out = *(int*)((char*)g_copter_sign_a + off)
                 * g_copter_sign_b[oa] * v;
            out += 3;
        }
        ord++;
        out_base++;
    }
#undef SEAT
}
