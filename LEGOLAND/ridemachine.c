/* LEGOLAND -- Codex-E: mechanical ride state machines and record setup.
 * VC6 SP3 /O2 /Gy /Gd. Types are local views of the original records. */
typedef union RideTile { unsigned short key; struct { unsigned char x, y; } b; } RideTile;
typedef struct Bloke { char pad00[0x35]; unsigned char riding, seat; } Bloke;
typedef struct RiderNode { struct RiderNode *next, *prev; Bloke* bloke; RideTile tile; } RiderNode;
typedef struct RideDef { char pad00[0xcc]; RiderNode* riders; } RideDef;
typedef struct SoundSource { int kind; void* person; int x, y; } SoundSource;
typedef struct FXEntry { const char* name; int flags; void* sample; } FXEntry;
typedef struct SpriteObj { int f00, f04; short** lls_holder; } SpriteObj;
typedef struct SBarrelRec {
    struct SBarrelRec* next; RideTile tile; unsigned char seated, riders;
    signed char frame; char pad09[3]; unsigned flags;
    signed char revs; char pad11[3]; int half;
    unsigned char joined, pad19[3]; int timer; signed char wheel;
    unsigned char seat[19];
} SBarrelRec;
typedef struct SpiderRec {
    RideTile tile; unsigned char seated, riders; signed char frame; char pad05[3];
    unsigned flags; signed char revs; char pad0d[3]; int half;
    unsigned char joined, pad15[3]; int timer; unsigned char seat[16]; struct SpiderRec* next;
} SpiderRec;
typedef struct PlaneRec {
    RideTile tile; unsigned char seated, riders; signed char frame, wheel; char pad06[2];
    unsigned flags; signed char revs; char pad0d[3]; int half;
    unsigned char joined, pad15[3]; int timer; unsigned char seat[4]; struct PlaneRec* next;
} PlaneRec;
typedef struct SafariRec {
    RideTile tile; short pad02; int seated, riders, frame; struct SafariRec* next;
    unsigned flags; int revs, half, joined, timer;
} SafariRec;
typedef struct TowerCar {
    unsigned flags; int f04, height, state, f10, f14; RiderNode *rider_a, *rider_b;
    signed char revs; char pad21[3];
} TowerCar;
typedef struct TowerRec {
    RideTile tile; unsigned char seated, riders, joined, pad05[3]; struct TowerRec* next;
    unsigned flags; int cycle; TowerCar car[4]; unsigned char seat[8];
    signed char frame3, frame5; char padae[2]; int timer;
} TowerRec;
typedef struct CopterSeat {
    unsigned flags; signed char frame; char pad05[3]; int entry_layer, ride_layer, entry_path, ride_path;
    RiderNode* rider; signed char frames; unsigned char stage; char pad1e[2];
} CopterSeat;
typedef struct CoptersRec {
    RideTile tile; unsigned char seated, riders; struct CoptersRec* next;
    unsigned flags; int mode; unsigned char joined, pad11[3]; int timer; CopterSeat seat[6];
} CoptersRec;
typedef struct SlideQueue { struct SlideQueue* next; RiderNode* rider; } SlideQueue;
typedef struct SlideRec {
    RideTile tile; short pad02; int state; signed char frame, pad09, f0a, anim_frame;
    struct SlideRec* next; unsigned flags; int half; unsigned char queued, pad19[3]; SlideQueue *head, *tail;
} SlideRec;
typedef struct GoldRec { RideTile tile; char pad02[0x14-2]; void* pans[6]; } GoldRec;

extern FXEntry g_spacetower_fx[1];                       /* 0x004b7618  1 entry (mechrides.c:1144) */
extern FXEntry g_spider_fx[1];                           /* 0x004b4d88  1 entry (mechrides.c:1058) */
extern int rand(void);                                  /* 0x0049e4b2 */
extern void SpiderRide_ReleaseSquare(RideTile*);          /* 0x00415a20 */
extern void SpaceTower_ReleaseSquare(RideTile*);          /* 0x0043aa50 */
extern void UnSourceAndFadeAllSamplesFromSource(SoundSource*, int); /* 0x00496c80 */
extern void* PlayInstanceOfSample(void*, int, int, SoundSource*); /* 0x00496d20 */
extern GoldRec* GoldRush_FindRecord(RideTile*);           /* 0x004069e0 */

extern void SpinningBarrels_StopRide(SBarrelRec*);       /* 0x0043c2f0 */
extern void SpiderRide_StopRide(SpiderRec*);             /* 0x00415a90 */
extern void SafariRide_StopRide(SafariRec*);             /* 0x00414b10 */
extern void PlaneRide_StopRide(PlaneRec*);               /* 0x0043d9f0 */
extern void SpaceTower_StopRide(TowerRec*);              /* 0x0043aac0 */

/* For every square-based sound source, person (+4) stays unwritten. */
// FUNCTION: LEGOLAND 0x0043c2f0
void SpinningBarrels_StopRide(SBarrelRec* rec)
{
    rec->half = 0; rec->frame = 0; rec->revs = 3;
    rec->joined = 0; rec->seated = 0; rec->flags &= ~0x4001u;
}
// FUNCTION: LEGOLAND 0x00415a90
void SpiderRide_StopRide(SpiderRec* rec)
{
    rec->half = 0; rec->frame = 0;
    rec->revs = rand() % 2 ? 4 : 3;
    rec->joined = 0; rec->seated = 0; rec->flags &= ~0x4001u;
    SpiderRide_ReleaseSquare(&rec->tile);
}
// FUNCTION: LEGOLAND 0x00414b10
void SafariRide_StopRide(SafariRec* rec)
{
    SoundSource source;
    rec->half = 0; rec->frame = 0; rec->revs = rand() % 2 + 3;
    rec->joined = 0; rec->seated = 0; rec->flags &= ~0x4001u;
    source.kind = 2; source.x = rec->tile.b.x; source.y = rec->tile.b.y;
    UnSourceAndFadeAllSamplesFromSource(&source, -200);
}
// FUNCTION: LEGOLAND 0x0043d9f0
void PlaneRide_StopRide(PlaneRec* rec)
{
    SoundSource source;
    rec->half = 0; rec->frame = 0; rec->revs = rand() % 2 ? 2 : 1;
    rec->joined = 0; rec->seated = 0; rec->flags &= ~0x4001u;
    source.kind = 2; source.x = rec->tile.b.x; source.y = rec->tile.b.y;
    UnSourceAndFadeAllSamplesFromSource(&source, -200);
}
/* Write the four zero heights before clearing the per-car flags: this
 * keeps the original dedicated zero register throughout the reset. */
// FUNCTION: LEGOLAND 0x0043aac0
void SpaceTower_StopRide(TowerRec* rec)
{
    rec->flags &= ~0x4001u;
    rec->car[0].height = 0; rec->car[1].height = 0;
    rec->car[2].height = 0; rec->car[3].height = 0;
    rec->car[0].flags &= ~1u; rec->car[1].flags &= ~1u;
    rec->car[2].flags &= ~1u; rec->car[3].flags &= ~1u;
    rec->car[0].revs = (char)(rand() % 3 + 7);
    rec->car[1].revs = (char)(rand() % 3 + 7);
    rec->car[2].revs = (char)(rand() % 3 + 7);
    rec->car[3].revs = (char)(rand() % 3 + 7);
    rec->joined = 0; rec->seated = 0;
    SpaceTower_ReleaseSquare(&rec->tile);
}
// FUNCTION: LEGOLAND 0x0043aa10
void SpaceTower_StartSound(TowerRec* rec)
{
    SoundSource source;
    source.kind = 2; source.x = rec->tile.b.x; source.y = rec->tile.b.y;
    PlayInstanceOfSample(g_spacetower_fx[0].sample, 1, 1, &source);
}
// FUNCTION: LEGOLAND 0x004159e0
void SpiderRide_StartSound(SpiderRec* rec)
{
    SoundSource source;
    source.kind = 2; source.x = rec->tile.b.x; source.y = rec->tile.b.y;
    PlayInstanceOfSample(g_spider_fx[0].sample, 1, 1, &source);
}
/* The original assumes at least one free slot: a full table loops forever. */
// FUNCTION: LEGOLAND 0x0043acb0
int SpaceTower_PickSeat(RiderNode* rider, TowerRec* rec)
{
    int seat = rand() % 8;
    while (rec->seat[seat]) { if (++seat >= 8) seat = 0; }
    rec->seat[seat] = 1; rider->bloke->seat = (unsigned char)seat;
    return seat;
}
// FUNCTION: LEGOLAND 0x00406e90
int GoldRush_HasFreePan(RideTile* tile)
{
    GoldRec* rec = GoldRush_FindRecord(tile);
    if (rec) { unsigned int i; for (i = 0; i < 6; ++i) if (!rec->pans[i]) return 1; }
    return 0;
}
/* Original invariant: either both links are null or tail is valid.
 * This does not set node->next or update the separate queued count. */
// FUNCTION: LEGOLAND 0x0042ce90
void EarthSlide_AppendQueue(SlideRec* rec, SlideQueue* node)
{
    if (!rec->head && !rec->tail) { rec->head = node; rec->tail = node; }
    else { rec->tail->next = node; rec->tail = node; }
}

extern RideDef* g_safari_def;                           /* 0x004cbec4 */
extern RideDef* g_spider_def;                           /* 0x004cbf20 */
extern RideDef* g_plane_def;                            /* 0x0062fe58 */
extern RideDef* g_sbarrel_def;                          /* 0x0062fde4 */
extern RideDef* g_spacetower_def;                       /* 0x0062fd74 */
extern RideDef* g_copters_def;                          /* 0x004c1198 */
extern RideDef* g_earthslide_def;                       /* 0x006160d0 */
extern void* g_safari_bnv_run;                          /* 0x004cbef4 */
extern void* g_spider_bnv0;                             /* 0x004cbf10 */
extern void* g_plane_bnv0;                              /* 0x0062fe90 */
extern void* g_sbarrel_bnv0;                            /* 0x0062fde8 */
extern SpriteObj* g_safari_zspr_obj;                   /* 0x0082c66c */
extern SpriteObj* g_spider_matte;                      /* 0x0082c668 */
extern SpriteObj* g_plane_matte;                       /* 0x0081cae0 */
extern SpriteObj* g_sbarrel_zspr_obj;                  /* 0x0062fe00 */
extern char g_safari_rider_name[];                     /* 0x004b4cac: manbox?? */
extern char g_spider_rider_name[];                     /* 0x004b4d94: manbox?? */
extern char g_plane_rider_name[];                      /* 0x004b79bc: manbox?? */
extern char g_sbarrel_rider_name[];                    /* 0x004b78b4: BoxBloke?? */
extern int sprintf(char*, const char*, ...);           /* 0x0049e573 */
extern int GetAllBlokesOffRide(RideDef*, unsigned short); /* 0x0048a390 */
extern void Ride_SetFlagToNotLetAnyoneOn(RideTile*);    /* 0x00442fa0 */
extern void SafariRide_SetFull(SafariRec*);             /* 0x00414ab0 */
extern void SpiderRide_SetFull(SpiderRec*);             /* 0x00415a60 */
extern void PlaneRide_SetFull(PlaneRec*);               /* 0x0043d990 */
extern void SpinningBarrels_SetFull(SBarrelRec*);       /* 0x0043c320 */
extern void SpaceTower_SetFull(TowerRec*);              /* 0x0043aa90 */
extern void Copters_SetFull(CoptersRec*);               /* 0x004048b0 */
extern void Put3DBlokesOnRide2(RideDef*, void*);         /* 0x00441ad0 */
extern void SetBlokePositionFromBNV(void*, Bloke*, const char*, int, float, float, int); /* 0x00484a70 */

// FUNCTION: LEGOLAND 0x004150c0
void SafariRide_StepMachine(SafariRec* rec)
{
    RiderNode* rider = g_safari_def->riders;
    if (rec->flags & 1) {
        ++rec->half;
        if (!rec->revs) {
            if (GetAllBlokesOffRide(g_safari_def, rec->tile.key))
                SafariRide_StopRide(rec);
            return;
        }
        if (rec->half >= 2) {
            rec->half = 0;
            if (++rec->frame >= 48) { rec->frame = 0; --rec->revs; }
        }
    } else {
        if (rec->flags & 0x4000) {
            if (rec->seated == rec->joined) {
                rec->flags &= ~0x4000u;
                SafariRide_SetFull(rec);
                return;
            }
        } else if (rec->seated) {
                if (!rec->timer) {
                    rec->flags |= 0x4000;
                    Ride_SetFlagToNotLetAnyoneOn(&rec->tile);
                } else --rec->timer;
        }
    }
    while (rider) {
        if (rec->tile.key == rider->tile.key && rider->bloke->riding == 1) {
            sprintf(g_safari_rider_name + 6, "%02d", rider->bloke->seat + 1);
            SetBlokePositionFromBNV(g_safari_bnv_run, rider->bloke, g_safari_rider_name,
                                    rec->frame, -1617787.0f, -1618006.0f, 0);
        }
        rider = rider->next;
    }
    **g_safari_zspr_obj->lls_holder = (short)rec->frame;
    Put3DBlokesOnRide2(g_safari_def, rec);
}

// FUNCTION: LEGOLAND 0x004161f0
void SpiderRide_StepMachine(SpiderRec* rec)
{
    RiderNode* rider = g_spider_def->riders;
    if (rec->flags & 1) {
        ++rec->half;
        if (!rec->revs) {
            if (GetAllBlokesOffRide(g_spider_def, rec->tile.key))
                SpiderRide_StopRide(rec);
            return;
        }
        if (rec->half >= 2) {
            rec->half = 0;
            if (++rec->frame >= 32) { rec->frame = 0; --rec->revs; }
        }
    } else {
        if (rec->flags & 0x4000) {
            if (rec->seated == rec->joined) {
                rec->flags &= ~0x4000u;
                SpiderRide_SetFull(rec);
                return;
            }
        } else if (rec->seated) {
                /* Original quirk: zero is decremented too, becoming -1. */
                if (!rec->timer) {
                    rec->flags |= 0x4000;
                    Ride_SetFlagToNotLetAnyoneOn(&rec->tile);
                }
                --rec->timer;
        }
    }
    while (rider) {
        if (rec->tile.key == rider->tile.key && rider->bloke->riding == 1) {
            sprintf(g_spider_rider_name + 6, "%02d", rider->bloke->seat);
            SetBlokePositionFromBNV(g_spider_bnv0, rider->bloke, g_spider_rider_name,
                                    rec->frame, -1617787.75f, -1618096.5f, 0);
        }
        rider = rider->next;
    }
    **g_spider_matte->lls_holder = (short)rec->frame;
}

// FUNCTION: LEGOLAND 0x0043e2b0
void PlaneRide_StepMachine(PlaneRec* rec)
{
    RiderNode* rider = g_plane_def->riders;
    if (++rec->wheel >= 24) rec->wheel = 0;
    if (rec->flags & 1) {
        ++rec->half;
        if (!rec->revs) {
            if (GetAllBlokesOffRide(g_plane_def, rec->tile.key))
                PlaneRide_StopRide(rec);
            return;
        }
        if (rec->half >= 2) {
            rec->half = 0;
            if (++rec->frame >= 97) { rec->frame = 0; --rec->revs; }
        }
    } else {
        if (rec->flags & 0x4000) {
            if (rec->seated == rec->joined) {
                rec->flags &= ~0x4000u;
                PlaneRide_SetFull(rec);
                return;
            }
        } else if (rec->seated) {
                if (!rec->timer) {
                    rec->flags |= 0x4000;
                    Ride_SetFlagToNotLetAnyoneOn(&rec->tile);
                } else --rec->timer;
        }
    }
    while (rider) {
        if (rec->tile.key == rider->tile.key && rider->bloke->riding == 1) {
            sprintf(g_plane_rider_name + 6, "%02d", rider->bloke->seat);
            SetBlokePositionFromBNV(g_plane_bnv0, rider->bloke, g_plane_rider_name,
                                    rec->frame, -1617706.75f, -1617948.625f, 0);
        }
        rider = rider->next;
    }
    **g_plane_matte->lls_holder = (short)rec->frame;
}

// FUNCTION: LEGOLAND 0x0043c7f0
void SpinningBarrels_StepMachine(SBarrelRec* rec)
{
    RiderNode* rider = g_sbarrel_def->riders;
    if (++rec->wheel >= 32) rec->wheel = 0;
    if (rec->flags & 1) {
        ++rec->half;
        if (!rec->revs) {
            if (GetAllBlokesOffRide(g_sbarrel_def, rec->tile.key))
                SpinningBarrels_StopRide(rec);
            return;
        }
        if (rec->half > 2) {
            rec->half = 0;
            if (++rec->frame >= 64) { rec->frame = 0; --rec->revs; }
        }
    } else {
        if (rec->flags & 0x4000) {
            if (rec->seated == rec->joined) {
                rec->flags &= ~0x4000u;
                SpinningBarrels_SetFull(rec);
                return;
            }
        } else if (rec->seated) {
                if (!rec->timer) {
                    rec->flags |= 0x4000;
                    Ride_SetFlagToNotLetAnyoneOn(&rec->tile);
                } else --rec->timer;
        }
    }
    while (rider) {
        if (rec->tile.key == rider->tile.key && rider->bloke->riding == 1) {
            sprintf(g_sbarrel_rider_name + 8, "%02d", rider->bloke->seat);
            SetBlokePositionFromBNV(g_sbarrel_bnv0, rider->bloke, g_sbarrel_rider_name,
                                    rec->frame, -1617922.25f, -1618065.75f, 0);
        }
        rider = rider->next;
    }
    **g_sbarrel_zspr_obj->lls_holder = (short)rec->frame;
}

extern void SpaceTower_StepCar(TowerCar*);              /* 0x0043a940 */
extern void SpaceTower_UpdateRiders(TowerRec*);         /* 0x0043b810 */
extern void Copters_StepCar(CoptersRec*, int);          /* 0x00404860 */
extern void Copters_UpdateCarRider(CoptersRec*, int);   /* 0x00404630 */
extern void Copters_StopRide(CoptersRec*, int);         /* 0x004049a0 */

// FUNCTION: LEGOLAND 0x0043b990
void SpaceTower_StepMachine(TowerRec* rec)
{
    if (++rec->frame5 >= 16) rec->frame5 = 0;
    if (++rec->frame3 >= 16) rec->frame3 = 0;
    if (rec->flags & 1) {
        SpaceTower_StepCar(&rec->car[0]); SpaceTower_StepCar(&rec->car[1]);
        SpaceTower_StepCar(&rec->car[2]); SpaceTower_StepCar(&rec->car[3]);
        if (!rec->car[0].state && !rec->car[1].state && !rec->car[2].state && !rec->car[3].state) {
            if (GetAllBlokesOffRide(g_spacetower_def, rec->tile.key)) SpaceTower_StopRide(rec);
            return;
        }
    }
    SpaceTower_UpdateRiders(rec);
    Put3DBlokesOnRide2(g_spacetower_def, rec);
    if (!(rec->flags & 1)) {
        if (rec->flags & 0x4000) {
            if (rec->seated == rec->joined) {
                rec->flags &= ~0x4000u; SpaceTower_SetFull(rec); return;
            }
        } else if (rec->seated) {
            if (!rec->timer) {
                rec->flags |= 0x4000; Ride_SetFlagToNotLetAnyoneOn(&rec->tile); return;
            } else --rec->timer;
        }
    }
}

/* Both five-call runs deliberately order copter 1 before 0, then 2,3,4. */
// FUNCTION: LEGOLAND 0x00404a90
void Copters_StepMachine(CoptersRec* rec)
{
    if (--rec->mode < 0) {
        rec->mode = 2;
        Copters_StepCar(rec, 1); Copters_StepCar(rec, 0); Copters_StepCar(rec, 2);
        Copters_StepCar(rec, 3); Copters_StepCar(rec, 4);
        if ((rec->flags & 1) && !(rec->seat[1].flags & 1) && !(rec->seat[0].flags & 1)
            && !(rec->seat[2].flags & 1) && !(rec->seat[3].flags & 1) && !(rec->seat[4].flags & 1)) {
            if (GetAllBlokesOffRide(g_copters_def, rec->tile.key)) Copters_StopRide(rec, 0);
            return;
        }
    }
    Copters_UpdateCarRider(rec, 1); Copters_UpdateCarRider(rec, 0); Copters_UpdateCarRider(rec, 2);
    Copters_UpdateCarRider(rec, 3); Copters_UpdateCarRider(rec, 4);
    if (!(rec->flags & 1)) {
        if (rec->flags & 0x4000) {
            if (rec->seated == rec->joined) {
                rec->flags &= ~0x4000u; Copters_SetFull(rec); return;
            }
        } else if (rec->seated) {
            if (!rec->timer) {
                rec->flags |= 0x4000;
                Ride_SetFlagToNotLetAnyoneOn(&rec->tile);
            } else --rec->timer;
        }
    }
    Put3DBlokesOnRide2(g_copters_def, rec);
}

typedef struct Rin { char pad00[0x14]; int frame_count; } Rin;
extern Rin* g_slide_rin;                                /* 0x006160d4 */
extern void* g_slide_anim;                              /* 0x006160e4 */
extern void Put3DBlokesOnRide(RideDef*, void*, int, void*); /* 0x00441a60 */
// FUNCTION: LEGOLAND 0x0042d560
void EarthSlide_StepMachine(SlideRec* rec)
{
    if (rec->flags & 1) {
        if (++rec->half > 2) {
            rec->half = 0;
            if (++rec->anim_frame >= g_slide_rin->frame_count) {
                rec->anim_frame = 0;
                rec->flags &= ~1u;
                GetAllBlokesOffRide(g_earthslide_def, rec->tile.key);
                rec->state = 1;
                return;
            }
        }
        Put3DBlokesOnRide(g_earthslide_def, rec, rec->anim_frame, g_slide_anim);
    }
    Put3DBlokesOnRide2(g_earthslide_def, rec);
}

typedef struct LLS { char pad00[0x10]; signed char frames; } LLS;
extern void* g_copters_layers;                        /* 0x004c1138 */
extern void* GetSpriteForLayer(void*, int);             /* 0x00441ec0 */
extern LLS* GetLLSForSprite(void*);                     /* 0x00441e80 */

/* The five car templates use order 1,0,2,3,4. Each caches the sprite
 * frame count once before either store; failure leaves both bytes intact.
 * Seat 5 is not initialized here. */
// FUNCTION: LEGOLAND 0x00403e90
void Copters_InitRecord(CoptersRec* rec)
{
    void* sprite;
    LLS* lls;
    rec->seat[1].entry_layer = 10;
    rec->seat[1].ride_layer = 3;
    rec->seat[1].entry_path = 2;
    rec->seat[1].ride_path = 7;
    rec->seat[1].flags = 0;
    sprite = GetSpriteForLayer(g_copters_layers, 3);
    if (sprite) {
        lls = GetLLSForSprite(sprite);
        if (lls) { signed char frames = lls->frames; rec->seat[1].frames = frames; rec->seat[1].frame = frames - 1; }
    }
    rec->seat[0].entry_layer = 2;
    rec->seat[0].ride_layer = 1;
    rec->seat[0].entry_path = 0;
    rec->seat[0].ride_path = 6;
    rec->seat[0].flags = 0;
    sprite = GetSpriteForLayer(g_copters_layers, 1);
    if (sprite) {
        lls = GetLLSForSprite(sprite);
        if (lls) { signed char frames = lls->frames; rec->seat[0].frames = frames; rec->seat[0].frame = frames - 1; }
    }
    rec->seat[2].entry_layer = 4;
    rec->seat[2].ride_layer = 11;
    rec->seat[2].entry_path = 4;
    rec->seat[2].ride_path = 8;
    rec->seat[2].flags = 0;
    sprite = GetSpriteForLayer(g_copters_layers, 11);
    if (sprite) {
        lls = GetLLSForSprite(sprite);
        if (lls) { signed char frames = lls->frames; rec->seat[2].frames = frames; rec->seat[2].frame = frames - 1; }
    }
    rec->seat[3].entry_layer = 5;
    rec->seat[3].ride_layer = 6;
    rec->seat[3].entry_path = 3;
    rec->seat[3].ride_path = 5;
    rec->seat[3].flags = 0;
    sprite = GetSpriteForLayer(g_copters_layers, 6);
    if (sprite) {
        lls = GetLLSForSprite(sprite);
        if (lls) { signed char frames = lls->frames; rec->seat[3].frames = frames; rec->seat[3].frame = frames - 1; }
    }
    rec->seat[4].entry_layer = 8;
    rec->seat[4].ride_layer = 7;
    rec->seat[4].entry_path = 1;
    rec->seat[4].ride_path = 9;
    rec->seat[4].flags = 0;
    sprite = GetSpriteForLayer(g_copters_layers, 7);
    if (sprite) {
        lls = GetLLSForSprite(sprite);
        if (lls) { signed char frames = lls->frames; rec->seat[4].frames = frames; rec->seat[4].frame = frames - 1; }
    }
    Copters_StopRide(rec, 1);
}
