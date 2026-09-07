/* LEGOLAND -- Codex-F: copter and space-tower rider/car updates.
 * VC6 SP3 /O2 /Gy /Gd. Types mirror ridemachine.c (Codex-E).
 * Verification and recovered mechanics: docs/lanes/codex-f.md.
 */
typedef union RideTile { unsigned short key; struct { unsigned char x, y; } b; } RideTile;
typedef struct RiderNode { struct RiderNode *next, *prev; void* bloke; RideTile tile; } RiderNode;
typedef struct SoundSource { int kind; void* person; int x, y; } SoundSource;
typedef struct TowerCar {
    unsigned flags;
    int f04, height, state, f10, f14;
    RiderNode *rider_a, *rider_b;
    signed char revs;
    char pad21[3];
} TowerCar; /* 0x24 */
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

/* Clear flying flags (paired 0/1 then 3/2 then 4), park frames at frames-1
 * in order 1,0,2,3,4, drop riders, clear ride bits 0x4001. silent!=0 skips
 * the fade/play pair (InitRecord path).
 * Residual: flag-pair register assignment swaps seat0/1 and seat2/3 loads
 * (85.9%). */ 
// WIP-FUNCTION: LEGOLAND 0x004049a0  (85.9%, flag-pair edx/ebx swap)
void Copters_StopRide(CoptersRec* rec, int silent)
{
    unsigned mask = ~1u;
    rec->seat[0].flags &= mask;
    rec->seat[1].flags &= mask;
    rec->seat[3].flags &= mask;
    rec->seat[2].flags &= mask;
    rec->seat[4].flags &= mask;
    rec->seat[1].frame = (signed char)(rec->seat[1].frames - 1);
    rec->seat[0].frame = (signed char)(rec->seat[0].frames - 1);
    rec->seat[2].frame = (signed char)(rec->seat[2].frames - 1);
    rec->seat[3].frame = (signed char)(rec->seat[3].frames - 1);
    rec->seat[4].frame = (signed char)(rec->seat[4].frames - 1);
    rec->seat[1].rider = 0;
    rec->seat[0].rider = 0;
    rec->seat[2].rider = 0;
    rec->seat[3].rider = 0;
    rec->seat[4].rider = 0;
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

/* Space-tower car state machine: 1 waits on f10, 2 ascends by revs then
 * descends by 2 until height hits 0 and clears state. */
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
