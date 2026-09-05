/* LEGOLAND -- ride-record helpers, Codex D.
 * Local views preserve the full allocated sizes and original VC6 codegen.
 * All keys are packed {unsigned char x,y}; no runtime generic helpers. */
typedef struct BPos { unsigned char x, y; } BPos;
typedef union RideTile { unsigned short key; BPos b; } RideTile;
typedef struct SBarrelRec {
    struct SBarrelRec* next;             /* +00 */
    RideTile tile;                       /* +04 */
    unsigned char seated, riders, frame; /* +06,07,08 */
    unsigned char pad09[0x21-9];
    unsigned char seat[0x13];            /* +21; zero-based storage */
} SBarrelRec;                            /* 0x34 */
typedef struct PlaneRec {
    RideTile tile;
    unsigned char seated, riders, frame, pad05[3];
    int flags, revs, half;               /* +08,+0c,+10 */
    unsigned char pad14[8];
    unsigned char seat[4];               /* +1c; zero-based storage */
    struct PlaneRec* next;              /* +20 */
} PlaneRec;                              /* 0x24 */
typedef struct TowerRec {
    RideTile tile;
    unsigned char pad02[6];
    struct TowerRec* next;              /* +08 */
    unsigned char pad0c[0xb4-0xc];
} TowerRec;                              /* 0xb4 */
typedef struct SpiderRec {
    RideTile tile;
    unsigned char pad02[0x1c-2];
    unsigned char seat[16];              /* +1c; zero-based storage */
    struct SpiderRec* next;             /* +2c */
} SpiderRec;                             /* 0x30 */
typedef struct SafariRec {
    RideTile tile;
    unsigned short pad02;
    int seated, riders, frame;           /* +04,+08,+0c */
    struct SafariRec* next;             /* +10 */
    int flags, revs, half, joined, timer; /* +14..+24 */
} SafariRec;                             /* 0x28 */
typedef struct CoptersRec {
    RideTile tile;
    unsigned short pad02;
    struct CoptersRec* next;
    unsigned char pad08[0xd8-8];
} CoptersRec;                            /* 0xd8 */
typedef struct RestRec {
    struct RestRec* next;
    RideTile tile;
    unsigned char seats[3], frame, pad0a[2];
} RestRec;                               /* 0x0c */
typedef struct RestRec2 {
    struct RestRec2* next;
    RideTile tile;
    unsigned char pad06[0x40-6];
} RestRec2;                              /* 0x40 */
typedef struct CarouselRec {
    struct CarouselRec* next;
    RideTile tile;
    unsigned char pad06[0x2c-6];
} CarouselRec;                           /* Full 0x2c, not shortened 0x24 view. */
typedef struct BalloonzRec {
    struct BalloonzRec* next;
    RideTile tile;
    unsigned char pad06[0x20-6];
} BalloonzRec;
typedef struct Bloke {
    unsigned char pad00[0x36];
    unsigned char seat;
    unsigned char pad37[0x4c-0x37];
    unsigned short stop_part, pad4e;
    int anim;
    unsigned char pad54[0x62-0x54];
    unsigned short flags;
} Bloke;
typedef struct RiderNode {
    struct RiderNode *next, *prev;
    Bloke* bloke;
    RideTile tile;
} RiderNode;
typedef struct RideDef { unsigned char pad00[0xcc]; RiderNode* riders; } RideDef;
typedef struct SlideQueue { struct SlideQueue* next; RiderNode* rider; } SlideQueue;
typedef struct SlideRec {
    RideTile tile;
    unsigned short pad02;
    int state;
    unsigned char frame, pad09, f0a, f0b;
    struct SlideRec* next;
    int f10, f14;
    unsigned char queued, pad19[3];
    SlideQueue *head, *tail;
} SlideRec;                              /* 0x24 */
typedef struct GoldRec {
    RideTile tile;
    unsigned char pad02[0xc-2];
    struct GoldRec* next;
    int pad10, pan[6];
} GoldRec;                               /* 0x2c */
typedef struct SoundSource { int kind; void* person; int x, y; } SoundSource;
typedef struct FXEntry { const char* name; int flags; void* sample; } FXEntry;
typedef struct TowerSeatAnim { int stop_part; void* anim; } TowerSeatAnim;
typedef struct Pump {
    unsigned short tile, school;
    int x, y;
    struct Pump* next;
} Pump;
typedef struct JcStation {
    unsigned short tile;
    unsigned char pad02[0x3c-2];
    struct JcStation* next;
    int scenery_value;
} JcStation;

extern SBarrelRec* g_sbarrel_head;        /* 0x0062fe08 */
extern PlaneRec* g_plane_recs;           /* 0x0062fe9c */
extern TowerRec* g_tower_recs;           /* 0x0062fda8 */
extern SpiderRec* g_spider_recs;         /* 0x004cbf58 */
extern SafariRec* g_safari_recs;         /* 0x004cbf0c */
extern CoptersRec* g_copter_recs;         /* 0x004c11b4 */
extern RestRec* g_rest1_recs;             /* 0x00616144 */
extern RestRec2* g_r2_recs;               /* 0x00616148 */
extern CarouselRec* g_carousel_recs;     /* 0x006160c4 */
extern BalloonzRec* g_bz_recs;            /* 0x00616060 */
extern SlideRec* g_slide_head;            /* 0x006160e8 */
extern GoldRec* g_gold_recs;              /* 0x004c1204 */
extern RideDef* g_safari_def;             /* 0x004cbec4 */
extern RideDef* g_copters_def;            /* 0x004c1198 */
extern Pump* g_pump_list;                 /* 0x004cbea4 */
extern JcStation* g_jc_stations;          /* 0x00629c3c */
extern FXEntry g_plane_fx[];              /* 0x004b79d0 */
extern FXEntry g_safari_fx[];             /* 0x004b4cb8 */
extern FXEntry g_rest2_fx[];              /* 0x004b6968 */
extern TowerSeatAnim g_tower_seat_anim[]; /* 0x004b7758; g_bloke_anim_ref is +4 view */

extern void* HeapAlloc_w(unsigned size);                     /* 0x0049e4ff */
extern void HeapFree_w(void* ptr);                           /* 0x0049e4d0 */
extern int rand(void);                                      /* 0x0049e4b2 */
extern void SpinningBarrels_StopRide(SBarrelRec* rec);        /* 0x0043c2f0 */
extern void PlaneRide_StopRide(PlaneRec* rec);                /* 0x0043d9f0 */
extern void SpaceTower_StopRide(TowerRec* rec);               /* 0x0043aac0 */
extern void SpiderRide_StopRide(SpiderRec* rec);              /* 0x00415a90 */
extern void SafariRide_StopRide(SafariRec* rec);              /* 0x00414b10 */
extern void Copters_InitRecord(CoptersRec* rec);              /* 0x00403e90 */
extern void Carousel_StopRide(CarouselRec* rec);              /* 0x0042c210 */
extern void EarthSlide_AppendQueue(SlideRec* rec, SlideQueue* node); /* 0x0042ce90 */
extern int SpaceTower_PickSeat(RiderNode* rider, TowerRec* rec); /* 0x0043acb0 */
extern void* PlayInstanceOfSample(void* sample, int loop, int a, SoundSource* src); /* 0x00496d20 */
extern void UnSourceAndFadeAllSamplesFromSource(SoundSource* src, int fade); /* 0x00496c80 */
extern void Pump_Remove(Pump* pump);                         /* 0x00411b20 */
extern GoldRec* GoldRush_FindRecord(RideTile* tile);          /* 0x004069e0 */
extern void* memset(void* dest, int value, unsigned size);    /* intrinsic */
extern int memcmp(const void* a, const void* b, unsigned size); /* intrinsic */
#pragma intrinsic(memset, memcmp)

/* Two-byte intrinsic memcmp retains the original +4 address materialization. */
// FUNCTION: LEGOLAND 0x0043be40
SBarrelRec* SpinningBarrels_FindRecord(RideTile* tile)
{
    SBarrelRec* rec = g_sbarrel_head;
    if (rec) {
        while (memcmp(&rec->tile, tile, 2) != 0) {
            rec = rec->next;
            if (!rec) return 0;
        }
        return rec;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x0042f9d0
RestRec2* Restaurant2_FindRec(RideTile* tile)
{
    RestRec2* rec = g_r2_recs;
    if (rec) {
        while (memcmp(&rec->tile, tile, 2) != 0) {
            rec = rec->next;
            if (!rec) return 0;
        }
        return rec;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x0042ef40
RestRec* Restaurant1_FindRec(RideTile* tile)
{
    RestRec* rec = g_rest1_recs;
    if (rec) {
        while (memcmp(&rec->tile, tile, 2) != 0) {
            rec = rec->next;
            if (!rec) return 0;
        }
        return rec;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x0043d960
PlaneRec* PlaneRide_FindRecord(RideTile* tile)
{
    PlaneRec* rec = g_plane_recs;
    if (rec) {
        while (memcmp(&rec->tile, tile, 2) != 0) {
            rec = rec->next;
            if (!rec) return 0;
        }
        return rec;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x0043ac40
TowerRec* SpaceTower_FindRecord(RideTile* tile)
{
    TowerRec* rec = g_tower_recs;
    if (rec) {
        while (memcmp(&rec->tile, tile, 2) != 0) {
            rec = rec->next;
            if (!rec) return 0;
        }
        return rec;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x0043bdb0
void SpinningBarrels_AddRecord(RideTile* tile)
{
    SBarrelRec* rec = (SBarrelRec*)HeapAlloc_w(sizeof(SBarrelRec));
    if (rec) {
        memset(rec, 0, sizeof(SBarrelRec));
        rec->tile.key = tile->key;
        rec->frame = 0;
        rec->next = g_sbarrel_head;
        g_sbarrel_head = rec;
        SpinningBarrels_StopRide(rec);
    }
}

// FUNCTION: LEGOLAND 0x0043d880
void PlaneRide_AddRecord(RideTile* tile)
{
    PlaneRec* rec = (PlaneRec*)HeapAlloc_w(sizeof(PlaneRec));
    if (rec) {
        memset(rec, 0, sizeof(PlaneRec));
        rec->tile.key = tile->key;
        rec->next = g_plane_recs;
        g_plane_recs = rec;
        PlaneRide_StopRide(rec);
    }
}

// FUNCTION: LEGOLAND 0x0043ab70
void SpaceTower_AddRecord(RideTile* tile)
{
    TowerRec* rec = (TowerRec*)HeapAlloc_w(sizeof(TowerRec));
    if (rec) {
        memset(rec, 0, sizeof(TowerRec));
        rec->tile.key = tile->key;
        rec->next = g_tower_recs;
        g_tower_recs = rec;
        SpaceTower_StopRide(rec);
    }
}

// FUNCTION: LEGOLAND 0x004158f0
void SpiderRide_AddRecord(RideTile* tile)
{
    SpiderRec* rec = (SpiderRec*)HeapAlloc_w(sizeof(SpiderRec));
    if (rec) {
        memset(rec, 0, sizeof(SpiderRec));
        rec->tile.key = tile->key;
        rec->next = g_spider_recs;
        g_spider_recs = rec;
        SpiderRide_StopRide(rec);
    }
}

// FUNCTION: LEGOLAND 0x004149c0
void SafariRide_AddRecord(RideTile* tile)
{
    SafariRec* rec = (SafariRec*)HeapAlloc_w(sizeof(SafariRec));
    if (rec) {
        memset(rec, 0, sizeof(SafariRec));
        rec->tile.key = tile->key;
        rec->next = g_safari_recs;
        g_safari_recs = rec;
        SafariRide_StopRide(rec);
    }
}

/* Original bug: reset is unconditional, including after allocation failure. */
// FUNCTION: LEGOLAND 0x00403c40
void Copters_AddRecord(RideTile* tile)
{
    CoptersRec* rec = (CoptersRec*)HeapAlloc_w(sizeof(CoptersRec));
    if (rec) {
        memset(rec, 0, sizeof(CoptersRec));
        rec->tile.key = tile->key;
        rec->next = g_copter_recs;
        g_copter_recs = rec;
    }
    Copters_InitRecord(rec);
}

// FUNCTION: LEGOLAND 0x0042cd70
void EarthSlide_NewRecord(RideTile* tile)
{
    SlideRec* rec = (SlideRec*)HeapAlloc_w(sizeof(SlideRec));
    if (rec) {
        memset(rec, 0, sizeof(SlideRec));
        rec->tile.key = tile->key;
        rec->next = g_slide_head;
        rec->f10 = 0;
        rec->f14 = 0;
        rec->f0b = 0;
        rec->frame = 0;
        rec->f0a = 0;
        rec->state = 1;
        g_slide_head = rec;
    }
}

// FUNCTION: LEGOLAND 0x0042eec0
void Restaurant1_NewRecord(RideTile* tile)
{
    RestRec* rec = (RestRec*)HeapAlloc_w(sizeof(RestRec));
    if (rec) {
        memset(rec, 0, sizeof(RestRec));
        rec->tile.key = tile->key;
        rec->next = g_rest1_recs;
        memset(rec->seats, 0, sizeof(rec->seats));
        rec->frame = 0;
        g_rest1_recs = rec;
    }
}

// FUNCTION: LEGOLAND 0x0042bbc0
void Carousel_NewRecord(RideTile* tile)
{
    CarouselRec* rec = (CarouselRec*)HeapAlloc_w(sizeof(CarouselRec));
    if (rec) {
        memset(rec, 0, sizeof(CarouselRec));
        rec->tile.key = tile->key;
        rec->next = g_carousel_recs;
        g_carousel_recs = rec;
        Carousel_StopRide(rec);
    }
}

// FUNCTION: LEGOLAND 0x00406920
void GoldRush_NewRecord(RideTile* tile)
{
    GoldRec* rec = (GoldRec*)HeapAlloc_w(sizeof(GoldRec));
    if (rec) {
        memset(rec, 0, sizeof(GoldRec));
        rec->tile.key = tile->key;
        rec->next = g_gold_recs;
        g_gold_recs = rec;
    }
}

/* Original: null head is dereferenced; rec is freed even when not found. */
// FUNCTION: LEGOLAND 0x0042fa00
void Restaurant2_FreeRec(RestRec2* rec)
{
    if (g_r2_recs == rec) {
        g_r2_recs = rec->next;
    } else {
        RestRec2* node = g_r2_recs;
        while (node->next != rec) {
            node = *(RestRec2* volatile*)&node->next;
            if (!node) break;
        }
        if (node) node->next = rec->next;
    }
    HeapFree_w(rec);
}

/* Original: null head is dereferenced; rec is freed even when not found. */
// FUNCTION: LEGOLAND 0x0042ef70
void Restaurant1_FreeRec(RestRec* rec)
{
    if (g_rest1_recs == rec) {
        g_rest1_recs = rec->next;
    } else {
        RestRec* node = g_rest1_recs;
        while (node->next != rec) {
            node = *(RestRec* volatile*)&node->next;
            if (!node) break;
        }
        if (node) node->next = rec->next;
    }
    HeapFree_w(rec);
}

/* Original: null head is dereferenced; rec is freed even when not found. */
// FUNCTION: LEGOLAND 0x0042bc00
void Carousel_FreeRec(CarouselRec* rec)
{
    if (g_carousel_recs == rec) {
        g_carousel_recs = rec->next;
    } else {
        CarouselRec* node = g_carousel_recs;
        while (node->next != rec) {
            node = *(CarouselRec* volatile*)&node->next;
            if (!node) break;
        }
        if (node) node->next = rec->next;
    }
    HeapFree_w(rec);
}

/* Original: null head is dereferenced; rec is freed even when not found. */
// FUNCTION: LEGOLAND 0x0042a9b0
void Balloonz_FreeRec(BalloonzRec* rec)
{
    if (g_bz_recs == rec) {
        g_bz_recs = rec->next;
    } else {
        BalloonzRec* node = g_bz_recs;
        while (node->next != rec) {
            node = *(BalloonzRec* volatile*)&node->next;
            if (!node) break;
        }
        if (node) node->next = rec->next;
    }
    HeapFree_w(rec);
}

// FUNCTION: LEGOLAND 0x00415760
int SafariRide_SeatOf(RiderNode* rider, RideTile* tile)
{
    int seat = 0;
    RiderNode* node = g_safari_def->riders;
    while (node) {
        if (memcmp(&node->tile, tile, 2) == 0) {
            if (node == rider) {
                rider->bloke->seat = (unsigned char)seat;
                return seat;
            }
            seat++;
        }
        node = node->next;
    }
    return 0;
}

/* Original precondition: positive capacity, within storage, with a free seat.
 * Zero divides by zero; full occupancy loops forever. Returns one-based ID. */
// FUNCTION: LEGOLAND 0x0043e050
int PlaneRide_SeatOf(RiderNode* rider, PlaneRec* rec, signed char capacity)
{
    int i = rand() % capacity;
    while (rec->seat[i] != 0) {
        i++;
        if (i >= capacity) i = 0;
    }
    rec->seat[i] = 1;
    rider->bloke->seat = (unsigned char)(i + 1);
    return i + 1;
}

/* Original precondition: positive capacity, within storage, with a free seat.
 * Zero divides by zero; full occupancy loops forever. Returns one-based ID. */
// FUNCTION: LEGOLAND 0x0043ce10
int SpinningBarrels_SeatOf(RiderNode* rider, SBarrelRec* rec, signed char capacity)
{
    int i = rand() % capacity;
    while (rec->seat[i] != 0) {
        i++;
        if (i >= capacity) i = 0;
    }
    rec->seat[i] = 1;
    rider->bloke->seat = (unsigned char)(i + 1);
    return i + 1;
}

/* Original precondition: positive capacity, within storage, with a free seat.
 * Zero divides by zero; full occupancy loops forever. Returns one-based ID. */
// FUNCTION: LEGOLAND 0x00416830
int SpiderRide_SeatOf(RiderNode* rider, SpiderRec* rec, signed char capacity)
{
    int i = rand() % capacity;
    while (rec->seat[i] != 0) {
        i++;
        if (i >= capacity) i = 0;
    }
    rec->seat[i] = 1;
    rider->bloke->seat = (unsigned char)(i + 1);
    return i + 1;
}

// FUNCTION: LEGOLAND 0x00404f20
int Copters_CopterOf(RiderNode* rider, RideTile* tile)
{
    int seat = 0;
    RiderNode* node = g_copters_def->riders;
    while (node) {
        if (memcmp(&node->tile, tile, 2) == 0) {
            if (node == rider) return seat;
            seat++;
        }
        node = node->next;
    }
    return 0;
}

/* Source.person is unused for kind2 and deliberately left uninitialized. */
// FUNCTION: LEGOLAND 0x0043d990
void PlaneRide_SetFull(PlaneRec* rec)
{
    SoundSource src;
    rec->riders = rec->seated;
    rec->seated = 0;
    rec->flags = (rec->flags & ~0x4000) | 1;
    rec->frame = 0;
    rec->half = 0;
    src.kind = 2;
    src.x = rec->tile.b.x;
    src.y = rec->tile.b.y;
    PlayInstanceOfSample(g_plane_fx[0].sample, 1, 1, &src);
}

/* Source.person is unused for kind2 and deliberately left uninitialized. */
// FUNCTION: LEGOLAND 0x00414ab0
void SafariRide_SetFull(SafariRec* rec)
{
    SoundSource src;
    rec->riders = rec->seated;
    rec->seated = 0;
    rec->flags = (rec->flags & ~0x4000) | 1;
    rec->frame = 0;
    rec->half = 0;
    src.kind = 2;
    src.x = rec->tile.b.x;
    src.y = rec->tile.b.y;
    PlayInstanceOfSample(g_safari_fx[0].sample, 1, 1, &src);
}

/* The +0 next-field shape; preserves null-head and free-if-absent defects. */
// FUNCTION: LEGOLAND 0x0043be00
void SpinningBarrels_RemoveRecord(SBarrelRec* rec)
{
    if (g_sbarrel_head == rec) {
        g_sbarrel_head = rec->next;
    } else {
        SBarrelRec* node = g_sbarrel_head;
        while (node->next != rec) {
            node = *(SBarrelRec* volatile*)&node->next;
            if (!node) break;
        }
        if (node) node->next = rec->next;
    }
    HeapFree_w(rec);
}

// FUNCTION: LEGOLAND 0x0042ce50
void EarthSlide_JoinQueue(SlideRec* rec, RiderNode* rider)
{
    SlideQueue* node = (SlideQueue*)HeapAlloc_w(sizeof(SlideQueue));
    if (node) {
        memset(node, 0, sizeof(SlideQueue));
        node->rider = rider;
        rider->bloke->flags |= 0x40;
        EarthSlide_AppendQueue(rec, node);
        rec->queued++;
    }
}

// FUNCTION: LEGOLAND 0x0043ac70
void SpaceTower_TakeSeat(RiderNode* rider, RideTile* tile)
{
    TowerRec* rec = SpaceTower_FindRecord(tile);
    if (rec) {
        int seat = SpaceTower_PickSeat(rider, rec);
        rider->bloke->anim = seat;
        rider->bloke->stop_part = (unsigned short)g_tower_seat_anim[seat].stop_part;
    }
}

/* Packed square by value; fade first, then play the elevator-stop sample. */
// FUNCTION: LEGOLAND 0x0042fb60
void Restaurant2_StopSound(RideTile tile)
{
    SoundSource src;
    src.kind = 2;
    src.x = tile.b.x;
    src.y = tile.b.y;
    UnSourceAndFadeAllSamplesFromSource(&src, -200);
    PlayInstanceOfSample(g_rest2_fx[2].sample, 0, 1, &src);
}

// FUNCTION: LEGOLAND 0x00411ba0
void Pump_RemoveAllForSchool(RideTile school)
{
    Pump* pump = g_pump_list;
    while (pump) {
        Pump* next = pump->next;
        if (pump->school == school.key) Pump_Remove(pump);
        pump = next;
    }
}

// FUNCTION: LEGOLAND 0x00436130
void JungleCruise_AddValue(RideTile tile, int value)
{
    JcStation* station = g_jc_stations;
    while (station) {
        if (station->tile == tile.key) break;
        station = station->next;
    }
    if (station) station->scenery_value += value;
}

// FUNCTION: LEGOLAND 0x00406ec0
void GoldRush_ClaimPan(RiderNode* rider, RideTile* tile)
{
    GoldRec* rec = GoldRush_FindRecord(tile);
    if (rec) {
        unsigned i;
        for (i = 0; i < 6; i++) {
            if (!rec->pan[i]) {
                rec->pan[i] = 1;
                rider->bloke->seat = (unsigned char)i;
                return;
            }
        }
    }
}
