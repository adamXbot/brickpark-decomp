/* LEGOLAND -- scope E: ride record, queue, path and sound micro-helpers.
 * VC6 SP3 /O2 /Gy /Gd; see docs/lanes/scope-e.md for audit evidence.
 */
extern int memcmp(const void*, const void*, unsigned int); /* CRT intrinsic */
#pragma intrinsic(memcmp)

typedef struct Pos { int x, y; } Pos;
typedef union RideTile { unsigned short key; struct { unsigned char x, y; } b; } RideTile;
typedef struct WalkPath { short count; } WalkPath;
typedef struct Bloke {
    char pad00[0x34]; signed char walking; char pad35;
    unsigned char seat, pad37; short path_index; char pad3a[0x16];
    void* path; char pad54[4]; int pan_timer; char pad5c[4]; unsigned char action;
} Bloke;
typedef struct RiderNode { struct RiderNode* next; void* prev; Bloke* bloke; RideTile tile; } RiderNode;
typedef struct QueueNode { struct QueueNode* next; RiderNode* rider; } QueueNode;
typedef struct LFQueue { int* path; QueueNode* head; QueueNode* tail; } LFQueue;
typedef struct CoptersRec { RideTile tile; short pad02; struct CoptersRec* next; } CoptersRec;
typedef struct SafariRec { RideTile tile; char pad02[0xe]; struct SafariRec* next; } SafariRec;
typedef struct SpiderRec {
    RideTile tile; unsigned char seated, riders, frame; char pad05[3];
    unsigned int flags; int pad0c, cycle; char pad14[0x18]; struct SpiderRec* next;
} SpiderRec;
typedef struct GoldRec {
    RideTile tile; char pad02[0xa]; struct GoldRec* next; int pad10; Bloke* pans[6];
} GoldRec;
typedef struct SlideRec {
    RideTile tile; char pad02[0xa]; struct SlideRec* next;
    unsigned int flags; int pad14; unsigned char count; char pad19[3];
    QueueNode* head; QueueNode* tail;
} SlideRec;
typedef struct TowerRec {
    RideTile tile; unsigned char seated, riders; int pad04; struct TowerRec* next;
    unsigned int flags; int cycle;
} TowerRec;
typedef struct SBarrelRec {
    struct SBarrelRec* next; RideTile tile; unsigned char seated, riders;
    int pad08; unsigned int flags;
} SBarrelRec;
typedef struct PlaneRec { char pad00[0x20]; struct PlaneRec* next; } PlaneRec;
typedef struct CarouselRec CarouselRec;
typedef struct BalloonzRec BalloonzRec;
typedef struct BsBoat { unsigned short square; char pad02[0x3ee]; struct BsBoat* next; } BsBoat;
typedef struct RoadTile { char pad00[0x1d]; unsigned char users; } RoadTile;
typedef struct SoundSource { int kind; void* bloke; int x, y; } SoundSource;
typedef struct Sample { char pad00[0x1c]; unsigned short flags; } Sample;
typedef struct FXEntry { char* file; int flags; void* sample; } FXEntry;

extern CoptersRec* g_copter_recs;             /* 0x004c11b4 */
extern SafariRec* g_safari_recs;               /* 0x004cbf0c */
extern SpiderRec* g_spider_recs;               /* 0x004cbf58 */
extern GoldRec* g_gold_recs;                   /* 0x004c1204 */
extern SlideRec* g_slide_head;                /* 0x006160e8 */
extern TowerRec* g_tower_recs;                /* 0x0062fda8 */
extern SBarrelRec* g_sbarrel_head;            /* 0x0062fe08 */
extern PlaneRec* g_plane_recs;                /* 0x0062fe9c */
extern CarouselRec* g_carousel_recs;           /* 0x006160c4 */
extern BalloonzRec* g_bz_recs;                /* 0x00616060 */
extern BsBoat* g_bs_boats;                    /* 0x004cc03c */
/* The original scan covers SIX words, including the +0x14 table entry. */
extern void* g_copters_paths[6];              /* 0x004c1124 */
extern int g_fountain_sound_refs;             /* 0x00667114 */
extern int g_power_station_sound_refs;        /* 0x00667118 */
extern FXEntry g_fountain_fx[5];              /* 0x004b8710 */
extern FXEntry g_power_station_fx[2];         /* 0x004b8750 */

extern void Copters_RemoveRecord(CoptersRec*);            /* 0x00403c80 */
extern void SafariRide_RemoveRecord(SafariRec*);          /* 0x00414a00 */
extern void SpiderRide_RemoveRecord(SpiderRec*);          /* 0x00415930 */
extern void GoldRush_FreeRecord(GoldRec*);                /* 0x00406960 */
extern void SpaceTower_RemoveRecord(TowerRec*);           /* 0x0043abc0 */
extern void SpinningBarrels_RemoveRecord(SBarrelRec*);    /* 0x0043be00 */
extern void PlaneRide_RemoveRecord(PlaneRec*);            /* 0x0043d8c0 */
extern void Carousel_FreeRec(CarouselRec*);               /* 0x0042bc00 */
extern void Balloonz_FreeRec(BalloonzRec*);               /* 0x0042a9b0 */
extern void Copters_StepMachine(CoptersRec*);             /* 0x00404a90 */
extern void SafariRide_StepMachine(SafariRec*);           /* 0x004150c0 */
extern void SpiderRide_StepMachine(SpiderRec*);           /* 0x004161f0 */
extern void SpaceTower_StepMachine(TowerRec*);            /* 0x0043b990 */
extern void SpinningBarrels_StepMachine(SBarrelRec*);      /* 0x0043c7f0 */
extern void PlaneRide_StepMachine(PlaneRec*);              /* 0x0043e2b0 */
extern void EarthSlide_StepMachine(SlideRec*);            /* 0x0042d560 */
extern int ResumeSinglyPausedSample(Sample*);             /* 0x00492910 */
extern int rand(void);                                   /* 0x0049e4b2 */
extern void Load_FXList(FXEntry*, int);                   /* 0x00496dd0 */
extern void SpaceTower_CountSeated(TowerRec*);            /* 0x0043a9b0 */
extern void SpaceTower_StartSound(TowerRec*);             /* 0x0043aa10 */
extern void SpiderRide_StartSound(SpiderRec*);            /* 0x004159e0 */
extern void UnSourceAndFadeAllSamplesFromSource(SoundSource*, int); /* 0x00496c80 */
extern RoadTile* GetRoadRecord(unsigned int, unsigned int); /* 0x004125f0 */
extern int GoldRush_HasFreePan(RideTile*);                /* 0x00406e90 */
extern void Ride_SetFlagToNotLetAnyoneOn(RideTile*);      /* 0x00442fa0 */
extern void Ride_ClearFlagToNotLetAnyoneOn(RideTile*);    /* 0x00443000 */
extern void GetTileDimensions(int*, int*);                /* 0x00460540 */

/* The historical name is misleading: this retrieves the path pointer. */
// FUNCTION: LEGOLAND 0x004122f0
void* WalkPath_IndexOf(Bloke* b) { return b->path; }
// FUNCTION: LEGOLAND 0x00496d10
void SetSampleLooping(Sample* s) { s->flags |= 0x20; }
// FUNCTION: LEGOLAND 0x004048a0
int Copters_ResumeSFX(Sample* s) { ResumeSinglyPausedSample(s); return 0; }
// FUNCTION: LEGOLAND 0x00411e90
int LFQueue_HasRider(LFQueue* q) { return q->head != 0; }
// FUNCTION: LEGOLAND 0x0043c320
void SpinningBarrels_SetFull(SBarrelRec* r)
{
    r->riders = r->seated; r->seated = 0; r->flags = (r->flags & ~0x4000u) | 1;
}
// FUNCTION: LEGOLAND 0x00407230
void GoldRush_RollPanTimer(RiderNode* r) { r->bloke->pan_timer = rand() % 31 + 15; }

/* Removal updates the shared head; re-read it after each call. */
// FUNCTION: LEGOLAND 0x0043d940
void PlaneRide_FreeAllRecords(void) { while (g_plane_recs) PlaneRide_RemoveRecord(g_plane_recs); }
// FUNCTION: LEGOLAND 0x0043c4d0
void SpinningBarrels_FreeAllRecords(void) { while (g_sbarrel_head) SpinningBarrels_RemoveRecord(g_sbarrel_head); }
// FUNCTION: LEGOLAND 0x0043ac20
void SpaceTower_FreeAllRecords(void) { while (g_tower_recs) SpaceTower_RemoveRecord(g_tower_recs); }
// FUNCTION: LEGOLAND 0x00415990
void SpiderRide_FreeAllRecords(void) { while (g_spider_recs) SpiderRide_RemoveRecord(g_spider_recs); }
// FUNCTION: LEGOLAND 0x00414a60
void SafariRide_FreeAllRecords(void) { while (g_safari_recs) SafariRide_RemoveRecord(g_safari_recs); }
// FUNCTION: LEGOLAND 0x004069c0
void GoldRush_FreeAllRecords(void) { while (g_gold_recs) GoldRush_FreeRecord(g_gold_recs); }
// FUNCTION: LEGOLAND 0x00403ce0
void Copters_FreeAllRecords(void) { while (g_copter_recs) Copters_RemoveRecord(g_copter_recs); }
// FUNCTION: LEGOLAND 0x0042bc40
void Carousel_FreeRecords(void) { while (g_carousel_recs) Carousel_FreeRec(g_carousel_recs); }
// FUNCTION: LEGOLAND 0x0042a9f0
void Balloonz_FreeRecords(void) { while (g_bz_recs) Balloonz_FreeRec(g_bz_recs); }

// FUNCTION: LEGOLAND 0x004122a0
void LFPath_StartReverse(WalkPath* path, Bloke* b)
{
    b->path = path; b->path_index = path->count - 1; b->walking = -1; ++b->action;
}
// FUNCTION: LEGOLAND 0x00452a80
void PowerStation_InitSound(void)
{
    if (g_power_station_sound_refs++ == 0) Load_FXList(g_power_station_fx, 2);
}
// FUNCTION: LEGOLAND 0x00452990
void Fountain_InitSound(void)
{
    if (g_fountain_sound_refs++ == 0) Load_FXList(g_fountain_fx, 5);
}

/* These walks use the live next link AFTER stepping the current machine. */
// FUNCTION: LEGOLAND 0x0043e3f0
void PlaneRide_TickMachine(void)
{
    PlaneRec* r; for (r = g_plane_recs; r; r = r->next) PlaneRide_StepMachine(r);
}
// FUNCTION: LEGOLAND 0x0043c930
void SpinningBarrels_TickMachine(void)
{
    SBarrelRec* r; for (r = g_sbarrel_head; r; r = r->next) SpinningBarrels_StepMachine(r);
}
// FUNCTION: LEGOLAND 0x0043baa0
void SpaceTower_TickMachine(void)
{
    TowerRec* r; for (r = g_tower_recs; r; r = r->next) SpaceTower_StepMachine(r);
}
// FUNCTION: LEGOLAND 0x00416310
void SpiderRide_TickMachine(void)
{
    SpiderRec* r; for (r = g_spider_recs; r; r = r->next) SpiderRide_StepMachine(r);
}
// FUNCTION: LEGOLAND 0x00415200
void SafariRide_TickMachine(void)
{
    SafariRec* r; for (r = g_safari_recs; r; r = r->next) SafariRide_StepMachine(r);
}
// FUNCTION: LEGOLAND 0x00404bc0
void Copters_TickMachine(void)
{
    CoptersRec* r; for (r = g_copter_recs; r; r = r->next) Copters_StepMachine(r);
}
// FUNCTION: LEGOLAND 0x0042d5f0
void EarthSlide_TickInstances(void)
{
    SlideRec* r; for (r = g_slide_head; r; r = r->next) EarthSlide_StepMachine(r);
}
/* A missing node returns the list length, not -1. */
// FUNCTION: LEGOLAND 0x0042d3e0
int NthRiderNodeIndex(RiderNode* head, RiderNode* target)
{
    int i = 0; while (head) { if (head == target) break; head = head->next; ++i; } return i;
}
/* Pop only unlinks the node; freeing it belongs to the caller. */
// FUNCTION: LEGOLAND 0x0042d040
void EarthSlide_PopQueue(SlideRec* r)
{
    QueueNode* front = r->head;
    if (front) { r->head = front->next; if (r->tail == front) r->tail = 0; --r->count; }
}
// FUNCTION: LEGOLAND 0x0042cf40
int EarthSlide_IsFrontOfQueue(SlideRec* r, Bloke* bloke)
{
    if (r->head && r->head->rider->bloke == bloke) return 1; return 0;
}
// FUNCTION: LEGOLAND 0x0043aa90
void SpaceTower_SetFull(TowerRec* r)
{
    r->riders = r->seated; r->flags |= 1; r->cycle = 0;
    SpaceTower_CountSeated(r); SpaceTower_StartSound(r);
}
// FUNCTION: LEGOLAND 0x00415a60
void SpiderRide_SetFull(SpiderRec* r)
{
    r->riders = r->seated; r->seated = 0; r->flags = (r->flags & ~0x4000u) | 1;
    r->frame = 0; r->cycle = 0; SpiderRide_StartSound(r);
}
/* SoundSource.bloke is intentionally unwritten for a map-square source. */
// FUNCTION: LEGOLAND 0x0043aa50
void SpaceTower_ReleaseSquare(RideTile* tile)
{
    SoundSource src; src.kind = 2; src.x = tile->b.x; src.y = tile->b.y;
    UnSourceAndFadeAllSamplesFromSource(&src, -200);
}
// FUNCTION: LEGOLAND 0x00415a20
void SpiderRide_ReleaseSquare(RideTile* tile)
{
    SoundSource src; src.kind = 2; src.x = tile->b.x; src.y = tile->b.y;
    UnSourceAndFadeAllSamplesFromSource(&src, -200);
}

/* The two-byte memcmp intrinsic expands after loop-invariant hoisting,
 * so each iteration loads the record key and compares the query in memory.
 * Scalar equality hoists the query; a volatile query reverses the operands.
 * This is the established JailCell_FindRecord / Carousel_FindRec shape. */
// FUNCTION: LEGOLAND 0x004159b0
SpiderRec* SpiderRide_FindRecord(RideTile* tile)
{
    SpiderRec* r = g_spider_recs;
    if (r) {
        while (memcmp(&r->tile, tile, 2) != 0) {
            r = r->next;
            if (!r) return 0;
        }
        return r;
    } else return 0;
}
// FUNCTION: LEGOLAND 0x00414a80
SafariRec* SafariRide_FindRecord(RideTile* tile)
{
    SafariRec* r = g_safari_recs;
    if (r) {
        while (memcmp(&r->tile, tile, 2) != 0) {
            r = r->next;
            if (!r) return 0;
        }
        return r;
    } else return 0;
}
// FUNCTION: LEGOLAND 0x00403d00
CoptersRec* Copters_FindRecord(RideTile* tile)
{
    CoptersRec* r = g_copter_recs;
    if (r) {
        while (memcmp(&r->tile, tile, 2) != 0) {
            r = r->next;
            if (!r) return 0;
        }
        return r;
    } else return 0;
}
// FUNCTION: LEGOLAND 0x004069e0
GoldRec* GoldRush_FindRecord(RideTile* tile)
{
    GoldRec* r = g_gold_recs;
    if (r) {
        while (memcmp(&r->tile, tile, 2) != 0) {
            r = r->next;
            if (!r) return 0;
        }
        return r;
    } else return 0;
}
// FUNCTION: LEGOLAND 0x0042ce20
SlideRec* EarthSlide_FindRec(RideTile* tile)
{
    SlideRec* r = g_slide_head;
    if (r) {
        while (memcmp(&r->tile, tile, 2) != 0) {
            r = r->next;
            if (!r) return 0;
        }
        return r;
    } else return 0;
}
// FUNCTION: LEGOLAND 0x00406f00
void GoldRush_ReleasePan(RiderNode* rider)
{
    int seat = rider->bloke->seat;
    GoldRec* rec = GoldRush_FindRecord(&rider->tile);
    if (rec) rec->pans[seat] = 0;
}
/* Replace the path pointer with its save-table ordinal, or -1 if absent,
 * and return that ordinal. The returned index occupies eax; initialize it
 * before reading the bloke to preserve the original scratch-register order. */
// FUNCTION: LEGOLAND 0x00403d30
int Copters_StepRider(RiderNode* rider)
{
    int i = 0;
    Bloke* bloke = rider->bloke;
    for (; i < 6; ++i) if (bloke->path == g_copters_paths[i]) goto found;
    i = -1;
found:
    bloke->path = (void*)i;
    return i;
}
/* Original quirk: ASSIGN the supplied square to every boat, then count
 * nonzero assignments. The square is re-read each time (it may alias a boat). */
// FUNCTION: LEGOLAND 0x004192d0
int BoatingSchool_CountWater(RideTile* tile)
{
    BsBoat* b = g_bs_boats;
    int n = 0;
    while (b) { if ((b->square = tile->key) != 0) ++n; b = b->next; }
    return n;
}
// FUNCTION: LEGOLAND 0x004139e0
void Road_TileRelease(unsigned int x, unsigned int y)
{
    RoadTile* road = GetRoadRecord(x >> 8, y >> 8);
    if (road && road->users) --road->users;
}
// FUNCTION: LEGOLAND 0x00406f30
void GoldRush_UpdateFullFlag(RideTile* tile)
{
    if (!GoldRush_HasFreePan(tile)) Ride_SetFlagToNotLetAnyoneOn(tile);
    else Ride_ClearFlagToNotLetAnyoneOn(tile);
}
// FUNCTION: LEGOLAND 0x00411ea0
int LFQueue_FrontIsReady(LFQueue* q)
{
    if (q->head && q->head->rider->bloke->path_index == *q->path - 1) return 1;
    return 0;
}
// FUNCTION: LEGOLAND 0x004112c0
Pos LFQuadBottomLeft(void)
{
    int width, height; Pos out;
    GetTileDimensions(&width, &height); width <<= 1; height <<= 1;
    out.x = width >> 1; out.y = (height >> 1) + height; return out;
}
