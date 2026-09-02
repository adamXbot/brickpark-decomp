/* LEGOLAND -- per-ride save-game serialisers and the per-ride interface
 * (callback) tables.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; names
 * are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere).
 *
 * SAVE-FILE FORMAT (every ride, over SaveGameRead/SaveGameWrite):
 *     repeat { int32 1; <record bytes> }  int32 0
 * i.e. each record of the ride's singly-linked list is preceded by a
 * "more" flag of 1 and the list ends with a flag of 0. The records are
 * written RAW (memory image, pointers included), so the loaders re-link the
 * `next` field themselves and, where a record holds pointers into other
 * live structures, patch them from an index instead:
 *
 *   ride           size  next   notes
 *   GOLD RUSH      0x2c  +0x0c
 *   JAIL CELLS     0x1c  +0x00
 *   ELEPHANT F     0x0c  +0x00
 *   WATER BLOCK    0x0c  +0x00
 *   JOUST          0x24  +0x04  +0x08 zeroed on load; instance samples patched
 *   SBARREL        0x34  +0x00  instance + map-object samples patched
 *   SAFARI RIDE    0x28  +0x10  ditto
 *   SPIDER         0x30  +0x2c  ditto
 *   TEMPLE SLIDE   0x20  +0x08  ditto
 *   ZOOMER         0x24  +0x20  ditto; +0/+1 = map square, loop sample restarted
 *   CATAPULT       0x3c  +0x04  +0x10..+0x1c: 4 instance ptrs saved as 1-based index
 *   COPTERS        0xd8  +0x04  6 x 0x20 blocks at +0x18, instance ptr at +0x18 of each
 *   SPACE TOWER    0xb4  +0x08  4 x 0x24 cars at +0x24, two instance ptrs at +0x08/+0x0c
 *
 * "instance samples patched": for every live class instance, the per-instance
 * state (+0x10 of the instance node) has a playing-sample pointer at +0x2c and
 * its 1-based table index at +0x30; the pointer is re-resolved from the
 * ride's sample table (0 index = no sample, both cleared). The instance's
 * map object (+0x08) may carry a sound record at +0x54 {sample @0, index @4}
 * re-resolved from a second table the same way.
 */

/* ---- save-game primitives (LEGOLAND/saveprof.c) -------------------------- */
extern int SaveGameRead(void* buf, unsigned int n);          /* 0x0047d730 */
extern int SaveGameWrite(const void* buf, unsigned int n);   /* 0x0047d760 */
extern void* HeapAlloc_w(unsigned int size);                 /* 0x0049e4ff */
extern int NameCompare(const char* a, const char* b);        /* 0x004aab90 (_stricmp) */

/* ---- object class descriptor -------------------------------------------
 * The 0xd0-byte ObjDef of llidb_odf.c, seen from the ride DLL side: the
 * per-class callback slots at +0x8c..+0xbc and the live instance list at
 * +0xcc. Only the slots the GetInterfaces functions fill are named. */
typedef struct RideElem {
    char* name;            /* +0x00 */
    char* image;           /* +0x04 */
    unsigned int flags;    /* +0x08 */
    struct RideDef* data;  /* +0x0c */
} RideElem;

typedef struct RideInstNode {
    struct RideInstNode* next;   /* +0x00 */
    unsigned char pad04[4];
    struct RideMapObj* mapobj;   /* +0x08 */
    unsigned char pad0c[4];
    struct RideState* state;     /* +0x10 */
} RideInstNode;

typedef struct RideDef {
    unsigned char pad00[0x8c];
    void* cb_8c;                 /* +0x8c */
    unsigned char pad90[8];
    void* cb_add;                /* +0x98 place/add handler */
    void* cb_remove;             /* +0x9c remove handler */
    void* cb_a0;                 /* +0xa0 */
    void* cb_a4;                 /* +0xa4 */
    void* cb_a8;                 /* +0xa8 */
    void* cb_ac;                 /* +0xac */
    void* cb_b0;                 /* +0xb0 */
    unsigned char padb4[4];
    void* cb_load;               /* +0xb8 */
    void* cb_save;               /* +0xbc */
    unsigned char padc0[0xc];
    RideInstNode* instances;     /* +0xcc */
} RideDef;

/* ---- JOUST (0x00408db0) -------------------------------------------------- */
extern void Joust_A4(void);      /* 0x00407b50 */
extern void Joust_AC(void);      /* 0x00408c00 */
extern void Joust_8C(void);      /* 0x00408bc0 */
extern void Joust_A8(void);      /* 0x00407c30 */
extern void Joust_B0(void);      /* 0x00408580 */
extern void Joust_Remove(void);  /* 0x00407ad0 */
extern void Joust_Add(void);     /* 0x004079e0 */
extern void Joust_A0(void);      /* 0x00408c50 */
int SaveJoust(void);
int LoadJoust(RideElem* elem);

// FUNCTION: LEGOLAND 0x00408db0
void Joust_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("JOUST", elem->name) == 0) {
        def->cb_a4 = Joust_A4;
        def->cb_ac = Joust_AC;
        def->cb_8c = Joust_8C;
        def->cb_a8 = Joust_A8;
        def->cb_b0 = Joust_B0;
        def->cb_remove = Joust_Remove;
        def->cb_add = Joust_Add;
        def->cb_a0 = Joust_A0;
        def->cb_save = SaveJoust;
        def->cb_load = LoadJoust;
    }
}

/* ---- GOLD RUSH (gold wash) ---------------------------------------------- */
typedef struct GoldWashRec {
    unsigned char pad00[0xc];
    struct GoldWashRec* next;    /* +0x0c */
    unsigned char pad10[0x2c - 0x10];
} GoldWashRec;

extern GoldWashRec* g_goldwash_head;  /* 0x004c1204 */

// FUNCTION: LEGOLAND 0x00407800
int SaveGoldWash(void)
{
    int one = 1;
    int zero = 0;
    GoldWashRec* p;

    for (p = g_goldwash_head; p; p = p->next) {
        if (!SaveGameWrite(&one, 4))
            return 0;
        if (!SaveGameWrite(p, sizeof(GoldWashRec)))
            return 0;
    }
    return SaveGameWrite(&zero, 4) != 0;
}

// FUNCTION: LEGOLAND 0x00407870
int LoadGoldWash(void)
{
    int more;
    GoldWashRec* prev = 0;
    GoldWashRec* p;

    if (!SaveGameRead(&more, 4))
        return 0;
    while (more) {
        p = (GoldWashRec*)HeapAlloc_w(sizeof(GoldWashRec));
        if (!SaveGameRead(p, sizeof(GoldWashRec)))
            return 0;
        p->next = 0;
        if (prev)
            prev->next = p;
        else
            g_goldwash_head = p;
        prev = p;
        if (!SaveGameRead(&more, 4))
            return 0;
    }
    return 1;
}

/* ---- JOUST records ------------------------------------------------------- */
typedef struct JoustRec {
    unsigned char pad00[4];
    struct JoustRec* next;       /* +0x04 */
    int f08;                     /* +0x08 */
    unsigned char pad0c[0x24 - 0x0c];
} JoustRec;

typedef struct RideState {
    unsigned char pad00[0x2c];
    void* sample;                /* +0x2c */
    int sample_index;            /* +0x30 */
} RideState;

extern JoustRec* g_joust_head;        /* 0x004c1250 */
extern void* g_joust_samples[];       /* 0x004c123c */

// FUNCTION: LEGOLAND 0x00408c90
int SaveJoust(void)
{
    int one = 1;
    int zero = 0;
    JoustRec* p;

    for (p = g_joust_head; p; p = p->next) {
        if (!SaveGameWrite(&one, 4))
            return 0;
        if (!SaveGameWrite(p, sizeof(JoustRec)))
            return 0;
    }
    return SaveGameWrite(&zero, 4) != 0;
}

// FUNCTION: LEGOLAND 0x00408d00
int LoadJoust(RideElem* elem)
{
    RideDef* def = elem->data;
    int more;
    JoustRec* prev = 0;
    JoustRec* p;
    RideInstNode* inst;

    if (!SaveGameRead(&more, 4))
        return 0;
    while (more) {
        p = (JoustRec*)HeapAlloc_w(sizeof(JoustRec));
        if (!SaveGameRead(p, sizeof(JoustRec)))
            return 0;
        p->next = 0;
        if (prev)
            prev->next = p;
        else
            g_joust_head = p;
        p->f08 = 0;
        prev = p;
        if (!SaveGameRead(&more, 4))
            return 0;
    }
    for (inst = def->instances; inst; inst = inst->next) {
        if (inst->state->sample_index) {
            inst->state->sample = g_joust_samples[inst->state->sample_index];
        } else {
            inst->state->sample = 0;
            inst->state->sample_index = 0;
        }
    }
    return 1;
}

/* ---- TEMPLE SLIDE interfaces (0x00417a00) -------------------------------- */
extern void TempleSlide_A4(void);      /* 0x00417150 */
extern void TempleSlide_AC(void);      /* 0x00417200 */
extern void TempleSlide_8C(void);      /* 0x00417240 */
extern void TempleSlide_A8(void);      /* 0x00417430 */
extern void TempleSlide_B0(void);      /* 0x00416fa0 */
extern void TempleSlide_Remove(void);  /* 0x00417280 */
extern void TempleSlide_Add(void);     /* 0x004172d0 */
extern void TempleSlide_A0(void);      /* 0x00417300 */
int SaveTempleSlide(void);
int LoadTempleSlide(RideElem* elem);

// FUNCTION: LEGOLAND 0x00417a00
void TempleSlide_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("TEMPLE SLIDE", elem->name) == 0) {
        def->cb_a4 = TempleSlide_A4;
        def->cb_ac = TempleSlide_AC;
        def->cb_8c = TempleSlide_8C;
        def->cb_a8 = TempleSlide_A8;
        def->cb_b0 = TempleSlide_B0;
        def->cb_remove = TempleSlide_Remove;
        def->cb_add = TempleSlide_Add;
        def->cb_a0 = TempleSlide_A0;
        def->cb_save = SaveTempleSlide;
        def->cb_load = LoadTempleSlide;
    }
}

/* ---- JAIL CELLS ---------------------------------------------------------- */
typedef struct JailCellsRec {
    struct JailCellsRec* next;   /* +0x00 */
    unsigned char pad04[0x1c - 4];
} JailCellsRec;

extern JailCellsRec* g_jailcells_head;  /* 0x0062fd3c */

// FUNCTION: LEGOLAND 0x00438780
int SaveJailCells(void)
{
    int one = 1;
    int zero = 0;
    JailCellsRec* p;

    for (p = g_jailcells_head; p; p = p->next) {
        if (!SaveGameWrite(&one, 4))
            return 0;
        if (!SaveGameWrite(p, sizeof(JailCellsRec)))
            return 0;
    }
    return SaveGameWrite(&zero, 4) != 0;
}

// FUNCTION: LEGOLAND 0x004387f0
int LoadJailCells(void)
{
    int more;
    JailCellsRec* prev = 0;
    JailCellsRec* p;

    if (!SaveGameRead(&more, 4))
        return 0;
    while (more) {
        p = (JailCellsRec*)HeapAlloc_w(sizeof(JailCellsRec));
        if (!SaveGameRead(p, sizeof(JailCellsRec)))
            return 0;
        p->next = 0;
        if (prev)
            prev->next = p;
        else
            g_jailcells_head = p;
        prev = p;
        if (!SaveGameRead(&more, 4))
            return 0;
    }
    return 1;
}

/* ---- ELEPHANT F ---------------------------------------------------------- */
typedef struct ElephantFRec {
    struct ElephantFRec* next;   /* +0x00 */
    unsigned char pad04[0xc - 4];
} ElephantFRec;

extern ElephantFRec* g_elephantf_head;  /* 0x004cc034 */

// FUNCTION: LEGOLAND 0x00418b90
int Save_ElephantF(void)
{
    int one = 1;
    int zero = 0;
    ElephantFRec* p;

    for (p = g_elephantf_head; p; p = p->next) {
        if (!SaveGameWrite(&one, 4))
            return 0;
        if (!SaveGameWrite(p, sizeof(ElephantFRec)))
            return 0;
    }
    return SaveGameWrite(&zero, 4) != 0;
}

// FUNCTION: LEGOLAND 0x00418c00
int Load_ElephantF(void)
{
    int more;
    ElephantFRec* prev = 0;
    ElephantFRec* p;

    if (!SaveGameRead(&more, 4))
        return 0;
    while (more) {
        p = (ElephantFRec*)HeapAlloc_w(sizeof(ElephantFRec));
        if (!SaveGameRead(p, sizeof(ElephantFRec)))
            return 0;
        p->next = 0;
        if (prev)
            prev->next = p;
        else
            g_elephantf_head = p;
        prev = p;
        if (!SaveGameRead(&more, 4))
            return 0;
    }
    return 1;
}

/* ---- WATER BLOCK --------------------------------------------------------- */
typedef struct WaterBlockRec {
    struct WaterBlockRec* next;  /* +0x00 */
    unsigned char pad04[0xc - 4];
} WaterBlockRec;

extern WaterBlockRec* g_waterblock_head;  /* 0x004cc02c */

// FUNCTION: LEGOLAND 0x00418aa0
int Save_WaterBlock(void)
{
    int one = 1;
    int zero = 0;
    WaterBlockRec* p;

    for (p = g_waterblock_head; p; p = p->next) {
        if (!SaveGameWrite(&one, 4))
            return 0;
        if (!SaveGameWrite(p, sizeof(WaterBlockRec)))
            return 0;
    }
    return SaveGameWrite(&zero, 4) != 0;
}

// FUNCTION: LEGOLAND 0x00418b10
int Load_WaterBlock(void)
{
    int more;
    WaterBlockRec* prev = 0;
    WaterBlockRec* p;

    if (!SaveGameRead(&more, 4))
        return 0;
    while (more) {
        p = (WaterBlockRec*)HeapAlloc_w(sizeof(WaterBlockRec));
        if (!SaveGameRead(p, sizeof(WaterBlockRec)))
            return 0;
        p->next = 0;
        if (prev)
            prev->next = p;
        else
            g_waterblock_head = p;
        prev = p;
        if (!SaveGameRead(&more, 4))
            return 0;
    }
    return 1;
}

/* ---- rides with a per-instance sample AND a per-map-object sample -------
 * The second loader template: after the record list is rebuilt, every live
 * instance of the class has its playing-sample pointers re-resolved from the
 * ride's sample tables (pointers are not stable across a save/load; the
 * 1-based index next to each pointer is what was saved). */
typedef struct RideMapObj {
    unsigned char pad00[0x54];
    struct RideObjSound* sound;  /* +0x54 */
} RideMapObj;

typedef struct RideObjSound {
    void* sample;                /* +0x00 */
    int sample_index;            /* +0x04 */
} RideObjSound;

/* ---- SBARREL ------------------------------------------------------------- */
typedef struct SBarrelRec {
    struct SBarrelRec* next;     /* +0x00 */
    unsigned char pad04[0x34 - 4];
} SBarrelRec;

extern SBarrelRec* g_sbarrel_head;      /* 0x0062fe08 */
extern void* g_sbarrel_samples[];       /* 0x0062fe00 */
extern void* g_sbarrel_obj_samples[];   /* 0x0062fdf0 */

// FUNCTION: LEGOLAND 0x0043c620
int SaveSBarrel(void)
{
    int one = 1;
    int zero = 0;
    SBarrelRec* p;

    for (p = g_sbarrel_head; p; p = p->next) {
        if (!SaveGameWrite(&one, 4))
            return 0;
        if (!SaveGameWrite(p, sizeof(SBarrelRec)))
            return 0;
    }
    return SaveGameWrite(&zero, 4) != 0;
}

// FUNCTION: LEGOLAND 0x0043c690
int LoadSBarrel(RideElem* elem)
{
    RideDef* def = elem->data;
    int more;
    SBarrelRec* prev = 0;
    SBarrelRec* p;
    RideInstNode* inst;
    RideObjSound* snd;

    if (!SaveGameRead(&more, 4))
        return 0;
    while (more) {
        p = (SBarrelRec*)HeapAlloc_w(sizeof(SBarrelRec));
        if (!SaveGameRead(p, sizeof(SBarrelRec)))
            return 0;
        p->next = 0;
        if (prev)
            prev->next = p;
        else
            g_sbarrel_head = p;
        prev = p;
        if (!SaveGameRead(&more, 4))
            return 0;
    }
    for (inst = def->instances; inst; inst = inst->next) {
        if (inst->state->sample_index) {
            inst->state->sample = g_sbarrel_samples[inst->state->sample_index];
        } else {
            inst->state->sample = 0;
            inst->state->sample_index = 0;
        }
        snd = inst->mapobj->sound;
        if (snd)
            snd->sample = g_sbarrel_obj_samples[snd->sample_index];
    }
    return 1;
}

/* ---- SAFARI RIDE --------------------------------------------------------- */
typedef struct SafariRideRec {
    unsigned char pad00[0x10];
    struct SafariRideRec* next;  /* +0x10 */
    unsigned char pad14[0x28 - 0x14];
} SafariRideRec;

extern SafariRideRec* g_safari_head;    /* 0x004cbf0c */
extern void* g_safari_samples[];        /* 0x004cbf04 */
extern void* g_safari_obj_samples[];    /* 0x004cbef8 */

// FUNCTION: LEGOLAND 0x004157b0
int SaveSafariRide(void)
{
    int one = 1;
    int zero = 0;
    SafariRideRec* p;

    for (p = g_safari_head; p; p = p->next) {
        if (!SaveGameWrite(&one, 4))
            return 0;
        if (!SaveGameWrite(p, sizeof(SafariRideRec)))
            return 0;
    }
    return SaveGameWrite(&zero, 4) != 0;
}

// FUNCTION: LEGOLAND 0x00415820
int LoadSafariRide(RideElem* elem)
{
    RideDef* def = elem->data;
    int more;
    SafariRideRec* prev = 0;
    SafariRideRec* p;
    RideInstNode* inst;
    RideObjSound* snd;

    if (!SaveGameRead(&more, 4))
        return 0;
    while (more) {
        p = (SafariRideRec*)HeapAlloc_w(sizeof(SafariRideRec));
        if (!SaveGameRead(p, sizeof(SafariRideRec)))
            return 0;
        p->next = 0;
        if (prev)
            prev->next = p;
        else
            g_safari_head = p;
        prev = p;
        if (!SaveGameRead(&more, 4))
            return 0;
    }
    for (inst = def->instances; inst; inst = inst->next) {
        if (inst->state->sample_index) {
            inst->state->sample = g_safari_samples[inst->state->sample_index];
        } else {
            inst->state->sample = 0;
            inst->state->sample_index = 0;
        }
        snd = inst->mapobj->sound;
        if (snd)
            snd->sample = g_safari_obj_samples[snd->sample_index];
    }
    return 1;
}

/* ---- SPIDER -------------------------------------------------------------- */
typedef struct SpiderRec {
    unsigned char pad00[0x2c];
    struct SpiderRec* next;      /* +0x2c */
} SpiderRec;

extern SpiderRec* g_spider_head;        /* 0x004cbf58 */
extern void* g_spider_samples[];        /* 0x004cbf38 */
extern void* g_spider_obj_samples[];    /* 0x004cbf30 */

// FUNCTION: LEGOLAND 0x00416880
int SaveSpider(void)
{
    int one = 1;
    int zero = 0;
    SpiderRec* p;

    for (p = g_spider_head; p; p = p->next) {
        if (!SaveGameWrite(&one, 4))
            return 0;
        if (!SaveGameWrite(p, sizeof(SpiderRec)))
            return 0;
    }
    return SaveGameWrite(&zero, 4) != 0;
}

// FUNCTION: LEGOLAND 0x004168f0
int LoadSpider(RideElem* elem)
{
    RideDef* def = elem->data;
    int more;
    SpiderRec* prev = 0;
    SpiderRec* p;
    RideInstNode* inst;
    RideObjSound* snd;

    if (!SaveGameRead(&more, 4))
        return 0;
    while (more) {
        p = (SpiderRec*)HeapAlloc_w(sizeof(SpiderRec));
        if (!SaveGameRead(p, sizeof(SpiderRec)))
            return 0;
        p->next = 0;
        if (prev)
            prev->next = p;
        else
            g_spider_head = p;
        prev = p;
        if (!SaveGameRead(&more, 4))
            return 0;
    }
    for (inst = def->instances; inst; inst = inst->next) {
        if (inst->state->sample_index) {
            inst->state->sample = g_spider_samples[inst->state->sample_index];
        } else {
            inst->state->sample = 0;
            inst->state->sample_index = 0;
        }
        snd = inst->mapobj->sound;
        if (snd)
            snd->sample = g_spider_obj_samples[snd->sample_index];
    }
    return 1;
}

/* ---- TEMPLE SLIDE records ------------------------------------------------ */
typedef struct TempleSlideRec {
    unsigned char pad00[8];
    struct TempleSlideRec* next; /* +0x08 */
    unsigned char pad0c[0x20 - 0xc];
} TempleSlideRec;

extern TempleSlideRec* g_templeslide_head;   /* 0x004cbfd4 */
extern void* g_templeslide_samples[];        /* 0x004cbfcc */
extern void* g_templeslide_obj_samples[];    /* 0x004cbfb8 */

// FUNCTION: LEGOLAND 0x004178c0
int SaveTempleSlide(void)
{
    int one = 1;
    int zero = 0;
    TempleSlideRec* p;

    for (p = g_templeslide_head; p; p = p->next) {
        if (!SaveGameWrite(&one, 4))
            return 0;
        if (!SaveGameWrite(p, sizeof(TempleSlideRec)))
            return 0;
    }
    return SaveGameWrite(&zero, 4) != 0;
}

// FUNCTION: LEGOLAND 0x00417930
int LoadTempleSlide(RideElem* elem)
{
    RideDef* def = elem->data;
    int more;
    TempleSlideRec* prev = 0;
    TempleSlideRec* p;
    RideInstNode* inst;
    RideObjSound* snd;

    if (!SaveGameRead(&more, 4))
        return 0;
    while (more) {
        p = (TempleSlideRec*)HeapAlloc_w(sizeof(TempleSlideRec));
        if (!SaveGameRead(p, sizeof(TempleSlideRec)))
            return 0;
        p->next = 0;
        if (prev)
            prev->next = p;
        else
            g_templeslide_head = p;
        prev = p;
        if (!SaveGameRead(&more, 4))
            return 0;
    }
    for (inst = def->instances; inst; inst = inst->next) {
        if (inst->state->sample_index) {
            inst->state->sample = g_templeslide_samples[inst->state->sample_index];
        } else {
            inst->state->sample = 0;
            inst->state->sample_index = 0;
        }
        snd = inst->mapobj->sound;
        if (snd)
            snd->sample = g_templeslide_obj_samples[snd->sample_index];
    }
    return 1;
}

/* ---- ZOOMER -------------------------------------------------------------- */
/* Where a sound comes from (money.c's SoundSource; 16 bytes, +0x04 never
 * initialised for kind 2 = "a map square"). */
typedef struct RideSoundSource {
    int kind;   /* +0x00 */
    int f04;    /* +0x04 */
    int x;      /* +0x08 */
    int y;      /* +0x0c */
} RideSoundSource;

extern void* PlayInstanceOfSample(void* sample, int a, int b,
                                  RideSoundSource* src);  /* 0x00496d20 */
extern int   PauseSingleSample(void* s);                  /* 0x00492800 */

typedef struct ZoomerRec {
    unsigned char x;             /* +0x00 map square of the zoomer */
    unsigned char y;             /* +0x01 */
    unsigned char pad02[0x20 - 2];
    struct ZoomerRec* next;      /* +0x20 */
} ZoomerRec;

extern ZoomerRec* g_zoomer_head;        /* 0x0062fe9c */
extern void* g_zoomer_samples[];        /* 0x0062fe94 */
extern void* g_zoomer_obj_samples[];    /* 0x0062fe84 */
extern void* g_zoomer_loop_sample;      /* 0x004b79d8 */

// FUNCTION: LEGOLAND 0x0043e0a0
int SaveZoomer(void)
{
    int one = 1;
    int zero = 0;
    ZoomerRec* p;

    for (p = g_zoomer_head; p; p = p->next) {
        if (!SaveGameWrite(&one, 4))
            return 0;
        if (!SaveGameWrite(p, sizeof(ZoomerRec)))
            return 0;
    }
    return SaveGameWrite(&zero, 4) != 0;
}

/* Each restored zoomer also gets its looping sample started at its map
 * square and immediately paused (so it exists to be resumed later).
 *
 * The sound-source setup must go through this inlined helper taking x/y as
 * SCALAR parameters: the original loads both bytes of the square before the
 * first store into the source record (`mov al,[esi] / mov cl,[esi+1]` then
 * `mov [src.x],eax`), which is the argument-evaluation order of an inlined
 * call. Writing the three stores inline in LoadZoomer, in any order, or an
 * inline helper taking the record pointer, leaves the y load after the x
 * store. Same shape as money.c's PlayMoneySFX. */
static __inline void* PlaySampleAtSquare(int x, int y)
{
    RideSoundSource src;
    src.kind = 2;
    src.x = x;
    src.y = y;
    return PlayInstanceOfSample(g_zoomer_loop_sample, 1, 1, &src);
}

// FUNCTION: LEGOLAND 0x0043e110
int LoadZoomer(RideElem* elem)
{
    RideDef* def = elem->data;
    int more;
    ZoomerRec* prev = 0;
    ZoomerRec* p;
    RideInstNode* inst;
    RideObjSound* snd;

    if (!SaveGameRead(&more, 4))
        return 0;
    while (more) {
        p = (ZoomerRec*)HeapAlloc_w(sizeof(ZoomerRec));
        if (!SaveGameRead(p, sizeof(ZoomerRec)))
            return 0;
        p->next = 0;
        if (prev)
            prev->next = p;
        else
            g_zoomer_head = p;
        PauseSingleSample(PlaySampleAtSquare(p->x, p->y));
        prev = p;
        if (!SaveGameRead(&more, 4))
            return 0;
    }
    for (inst = def->instances; inst; inst = inst->next) {
        if (inst->state->sample_index) {
            inst->state->sample = g_zoomer_samples[inst->state->sample_index];
        } else {
            inst->state->sample = 0;
            inst->state->sample_index = 0;
        }
        snd = inst->mapobj->sound;
        if (snd)
            snd->sample = g_zoomer_obj_samples[snd->sample_index];
    }
    return 1;
}

/* ---- rides whose records point at class instances -----------------------
 * The third template. These records hold pointers into the class's live
 * instance list, which are not stable across save/load: on save each pointer
 * is replaced by its 1-based position in def->instances (0 = not found), and
 * on load the position is walked back into a pointer. */

/* ---- CATAPULT ------------------------------------------------------------ */
typedef struct CatapultRec {
    unsigned char pad00[4];
    struct CatapultRec* next;    /* +0x04 */
    unsigned char pad08[8];
    RideInstNode* inst[4];       /* +0x10 */
    unsigned char pad20[0x3c - 0x20];
} CatapultRec;

extern CatapultRec* g_catapult_head;   /* 0x004c1118 */
extern RideDef* g_catapult_def;        /* 0x004c10f4 */

/* Saves a COPY of each record with the instance pointers turned into
 * indices; the live list is left untouched.
 *
 * Codegen levers (both this and SpaceTower_Save): the list head is read into
 * `p` at declaration and the loop is guarded by a SECOND read of the head
 * global -- VC6 CSEs the two reads into an eax temp, tests that, and copies
 * it into the loop register (`mov eax,[head] / test eax,eax / mov ebp,eax`);
 * a plain `for (p = head; p; ...)` loads the register directly and cannot
 * match. `def` is hoisted into a local once per record (the original reloads
 * it after the first write, not per seat), and the search target is read
 * into `target` BEFORE the null test on `inst` (eager read). */
// FUNCTION: LEGOLAND 0x00403a20
int Catapult_Save(void)
{
    CatapultRec* p = g_catapult_head;
    int one = 1;
    int zero = 0;
    CatapultRec copy;
    RideInstNode* inst;
    int i;
    int idx;
    RideInstNode* target;
    RideDef* def;

    if (g_catapult_head) {
        for (; p; p = p->next) {
            copy = *p;
            if (!SaveGameWrite(&one, 4))
                return 0;
            def = g_catapult_def;
            for (i = 0; i < 4; i++) {
                inst = def->instances;
                target = copy.inst[i];
                idx = 0;
                while (inst && inst != target) {
                    inst = inst->next;
                    idx++;
                }
                if (inst)
                    copy.inst[i] = (RideInstNode*)(idx + 1);
                else
                    copy.inst[i] = 0;
            }
            if (!SaveGameWrite(&copy, sizeof(CatapultRec)))
                return 0;
        }
    }
    return SaveGameWrite(&zero, 4) != 0;
}

// FUNCTION: LEGOLAND 0x00403af0
int Catapult_Load(void)
{
    int more;
    CatapultRec* prev = 0;
    CatapultRec* p;
    RideInstNode* inst;
    int i;
    int idx;

    if (!SaveGameRead(&more, 4))
        return 0;
    while (more) {
        p = (CatapultRec*)HeapAlloc_w(sizeof(CatapultRec));
        if (!SaveGameRead(p, sizeof(CatapultRec)))
            return 0;
        p->next = 0;
        if (prev)
            prev->next = p;
        else
            g_catapult_head = p;
        prev = p;
        for (i = 0; i < 4; i++) {
            idx = (int)p->inst[i];
            inst = g_catapult_def->instances;
            if (idx) {
                while (--idx)
                    inst = inst->next;
                p->inst[i] = inst;
            } else {
                p->inst[i] = 0;
            }
        }
        if (!SaveGameRead(&more, 4))
            return 0;
    }
    return 1;
}

/* ---- COPTERS ------------------------------------------------------------- */
/* Six 0x20-byte per-copter blocks at +0x18; the instance pointer sits at
 * +0x18 of each (so at +0x30, +0x50, ... +0xd0 of the record). */
typedef struct CopterSeat {
    unsigned char pad00[0x18];
    RideInstNode* inst;          /* +0x18 */
    unsigned char pad1c[4];
} CopterSeat;

typedef struct CoptersRec {
    unsigned char pad00[4];
    struct CoptersRec* next;     /* +0x04 */
    unsigned char pad08[0x18 - 8];
    CopterSeat seat[6];          /* +0x18 .. +0xd8 */
} CoptersRec;

extern CoptersRec* g_copters_head;     /* 0x004c11b4 */
extern RideDef* g_copters_def;         /* 0x004c1198 */

// FUNCTION: LEGOLAND 0x00405050
int Copters_Load(void)
{
    int more;
    CoptersRec* prev = 0;
    CoptersRec* p;
    RideInstNode* inst;
    int i;
    int idx;

    if (!SaveGameRead(&more, 4))
        return 0;
    while (more) {
        p = (CoptersRec*)HeapAlloc_w(sizeof(CoptersRec));
        if (!SaveGameRead(p, sizeof(CoptersRec)))
            return 0;
        p->next = 0;
        if (prev)
            prev->next = p;
        else
            g_copters_head = p;
        prev = p;
        for (i = 0; i < 6; i++) {
            idx = (int)p->seat[i].inst;
            inst = g_copters_def->instances;
            if (idx) {
                while (--idx)
                    inst = inst->next;
                p->seat[i].inst = inst;
            } else {
                p->seat[i].inst = 0;
            }
        }
        if (!SaveGameRead(&more, 4))
            return 0;
    }
    return 1;
}

/* ---- SPACE TOWER --------------------------------------------------------- */
/* Four 0x24-byte car blocks at +0x24; each holds two instance pointers at
 * +0x08/+0x0c (so at +0x2c/+0x30, +0x50/+0x54, ... of the record). */
typedef struct SpaceTowerCar {
    unsigned char pad00[8];
    RideInstNode* a;             /* +0x08 */
    RideInstNode* b;             /* +0x0c */
    unsigned char pad10[0x24 - 0x10];
} SpaceTowerCar;

typedef struct SpaceTowerRec {
    unsigned char pad00[8];
    struct SpaceTowerRec* next;  /* +0x08 */
    unsigned char pad0c[0x24 - 0xc];
    SpaceTowerCar car[4];        /* +0x24 .. +0xb4 */
} SpaceTowerRec;

extern SpaceTowerRec* g_spacetower_head;  /* 0x0062fda8 */
extern RideDef* g_spacetower_def;         /* 0x0062fd74 */

/* Converts the instance pointers to indices IN PLACE and does not convert
 * them back after writing -- an original bug, faithfully reproduced: after a
 * save the live cars hold small integers where instance pointers were.
 * Same guarded-loop lever as Catapult_Save (see there); here the class
 * pointer is re-read from the global for every seat (not hoisted), and the
 * search target is read lazily through `*slot`. */
// FUNCTION: LEGOLAND 0x0043b5d0
int SpaceTower_Save(void)
{
    SpaceTowerRec* p = g_spacetower_head;
    int one = 1;
    int zero = 0;
    RideInstNode* inst;
    RideInstNode** slot;
    int i;
    int idx;

    if (g_spacetower_head) {
        for (; p; p = p->next) {
            if (!SaveGameWrite(&one, 4))
                return 0;
            for (i = 0; i < 8; i++) {
                if (i & 1)
                    slot = &p->car[i >> 1].b;
                else
                    slot = &p->car[i >> 1].a;
                inst = g_spacetower_def->instances;
                idx = 0;
                while (inst && inst != *slot) {
                    inst = inst->next;
                    idx++;
                }
                if (inst)
                    *slot = (RideInstNode*)(idx + 1);
                else
                    *slot = 0;
            }
            if (!SaveGameWrite(p, sizeof(SpaceTowerRec)))
                return 0;
        }
    }
    return SaveGameWrite(&zero, 4) != 0;
}

// FUNCTION: LEGOLAND 0x0043b6a0
int SpaceTower_Load(void)
{
    int more;
    SpaceTowerRec* prev = 0;
    SpaceTowerRec* p;
    RideInstNode* inst;
    RideInstNode** slot;
    int i;
    int idx;

    if (!SaveGameRead(&more, 4))
        return 0;
    while (more) {
        p = (SpaceTowerRec*)HeapAlloc_w(sizeof(SpaceTowerRec));
        if (!SaveGameRead(p, sizeof(SpaceTowerRec)))
            return 0;
        p->next = 0;
        if (prev)
            prev->next = p;
        else
            g_spacetower_head = p;
        prev = p;
        for (i = 0; i < 8; i++) {
            if (i & 1)
                slot = &p->car[i >> 1].b;
            else
                slot = &p->car[i >> 1].a;
            idx = (int)*slot;
            inst = g_spacetower_def->instances;
            if (idx) {
                while (--idx)
                    inst = inst->next;
                *slot = inst;
            } else {
                *slot = 0;
            }
        }
        if (!SaveGameRead(&more, 4))
            return 0;
    }
    return 1;
}
