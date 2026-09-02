/* LEGOLAND -- the remaining per-ride *_GetInterfaces callback-set providers.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS and global addresses are load-bearing; the names are ours.
 * Types are defined LOCALLY on purpose (legoland.h is owned elsewhere).
 *
 * ==========================================================================
 * WHAT THESE FUNCTIONS ARE
 * ==========================================================================
 * LLIDB_LoadODFData (0x0047bf70) builds a 0xd0-byte ObjDef per object class,
 * fills the callback slots at +0x8c..+0xc0 with the generic handlers
 * (SetStandardCallbacks), then calls SetCustomCallbacks (0x00452c20,
 * screen.c). That runs a name table of its own and then calls EVERY ride
 * module's *_GetInterfaces unconditionally with (elem, def). Each provider
 * re-tests the class NAME itself and, on a hit, overwrites the slots with its
 * own handlers -- which is why every function in this file is a chain of
 * name compares over one `def` pointer, and why a module that owns several
 * classes (LOG FLUME, WATER WORKS, the western town, the garden) has one
 * provider covering all of them.
 *
 * Slot meanings (screen.c / joust.c agree; every handler takes the class's
 * LLIDB element as its first argument, never the ObjDef):
 *
 *   +0x8c  tick / select-for-placement        the build UI
 *   +0x90  secondary update                   (LOG FLUME, WATER WORKS only)
 *   +0x94  secondary update 2                 (LOG FLUME only)
 *   +0x98  place one on the map               AddObjectToMap  0x00459af6
 *   +0x9c  take one off the map               RemoveObject    0x00459d1d
 *   +0xa0  draw-descriptor override           render walk, flags & 0x400
 *   +0xa4  load the class's resources         ODF loader      0x0047c602
 *   +0xa8  per-frame update                   class walk,     flags & 0x20
 *   +0xac  free the class's resources         ODF teardown    0x0047c6b1
 *   +0xb0  custom draw / interact             sprite draw, spr flags & 0x2000
 *   +0xb8  read the ride's save chunk         LoadGame
 *   +0xbc  write the ride's save chunk        SaveGame
 *   +0xc0  extra (LOG FLUME ENTRANCE only)
 *
 * ==========================================================================
 * CODEGEN SHAPES (three of them, all reproduced here)
 * ==========================================================================
 * 1. ONE class, no saved register: the eleven small providers. `elem` is read
 *    straight out of its argument slot, the compare is a call to the CRT
 *    _stricmp at 0x004aab90, and `def` is re-read from [esp+8] after the
 *    `add esp,8` -- exactly Joust_GetInterfaces in ridesave.c.
 *
 * 2. SEVERAL classes: `elem` is pinned in esi across the whole chain
 *    (`push esi / mov esi,[esp+8]`), `elem->name` is RELOADED for every
 *    compare, and each arm except the last pops esi BEFORE its run of stores
 *    and returns; the last arm falls into one shared `pop esi / ret`.
 *
 * 3. Garden_GetInterfaces compares with the plain, case-SENSITIVE strcmp
 *    intrinsic, not _stricmp -- the inline byte loop
 *    (`mov dl,[eax] / mov bl,[esi] / cmp dl,bl / ... / sbb eax,eax /
 *    sbb eax,-1`) is VC6's #pragma intrinsic(strcmp) expansion, so HEDGE and
 *    FLOWERS are the only two classes in the game whose custom callbacks do
 *    not survive a differently-cased class name in the .ODF. That is an
 *    original quirk, reproduced.
 *
 * ==========================================================================
 * NOTE FOR THE NEXT ROUND
 * ==========================================================================
 * Every extern below carries the callee's address: between them these fifteen
 * providers NAME 196 previously anonymous ride callbacks and say which slot
 * each one fills, so each ride's handler set can now be matched top-down.
 * Two cross-checks fell out of the addresses and both agree with ridesave.c:
 * GOLD RUSH's +0xbc/+0xb8 are SaveGoldWash/LoadGoldWash (0x00407800 /
 * 0x00407870) and JAIL CELL's are SaveJailCells/LoadJailCells (0x00438780 /
 * 0x004387f0).
 */

#pragma intrinsic(strcmp)

extern int NameCompare(const char* a, const char* b);   /* 0x004aab90 (_stricmp) */

typedef struct RideDef RideDef;

typedef struct RideElem {
    char*        name;          /* +0x00  class name */
    char*        image;         /* +0x04 */
    unsigned int flags;         /* +0x08 */
    RideDef*     data;          /* +0x0c  the ObjDef this fills in */
} RideElem;

struct RideDef {
    unsigned char pad00[0x8c];
    void* cb_8c;                /* +0x8c  tick / select */
    void* cb_90;                /* +0x90  update */
    void* cb_94;                /* +0x94  update2 */
    void* cb_add;               /* +0x98 */
    void* cb_remove;            /* +0x9c */
    void* cb_draw;              /* +0xa0 */
    void* cb_create;            /* +0xa4 */
    void* cb_activate;          /* +0xa8 */
    void* cb_destroy;           /* +0xac */
    void* cb_interact;          /* +0xb0 */
    void* cb_b4;                /* +0xb4 */
    void* cb_load;              /* +0xb8 */
    void* cb_save;              /* +0xbc */
    void* cb_c0;                /* +0xc0 */
};

/* ======================================================================
 * CASTLE LEVEL 1 -- 0x00403080
 * ====================================================================== */
extern void CastleLevel1_Create(void);      /* 0x00402ca0 */
extern void CastleLevel1_Destroy(void);     /* 0x00402ce0 */
extern void CastleLevel1_Tick(void);        /* 0x00402ff0 */
extern void CastleLevel1_Add(void);         /* 0x00403060 */
extern void CastleLevel1_Remove(void);      /* 0x00403030 */
extern void CastleLevel1_Activate(void);    /* 0x00402dc0 */
extern void CastleLevel1_Interact(void);    /* 0x00402d00 */

// FUNCTION: LEGOLAND 0x00403080
void CastleLevel1_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("CASTLE LEVEL 1", elem->name) == 0) {
        def->cb_create   = CastleLevel1_Create;
        def->cb_destroy  = CastleLevel1_Destroy;
        def->cb_8c       = CastleLevel1_Tick;
        def->cb_add      = CastleLevel1_Add;
        def->cb_remove   = CastleLevel1_Remove;
        def->cb_activate = CastleLevel1_Activate;
        def->cb_interact = CastleLevel1_Interact;
    }
}

/* ======================================================================
 * FORT -- 0x004068b0
 * ====================================================================== */
extern void Fort_Create(void);      /* 0x00406240 */
extern void Fort_Destroy(void);     /* 0x004062a0 */
extern void Fort_Tick(void);        /* 0x00406820 */
extern void Fort_Activate(void);    /* 0x00406660 */
extern void Fort_Interact(void);    /* 0x004062c0 */
extern void Fort_Remove(void);      /* 0x00406880 */
extern void Fort_Add(void);         /* 0x00406860 */

// FUNCTION: LEGOLAND 0x004068b0
void Fort_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("FORT", elem->name) == 0) {
        def->cb_create   = Fort_Create;
        def->cb_destroy  = Fort_Destroy;
        def->cb_8c       = Fort_Tick;
        def->cb_activate = Fort_Activate;
        def->cb_interact = Fort_Interact;
        def->cb_remove   = Fort_Remove;
        def->cb_add      = Fort_Add;
    }
}

/* ======================================================================
 * TEMPLE -- 0x00416e50
 * ====================================================================== */
extern void Temple_Create(void);      /* 0x004169c0 */
extern void Temple_Destroy(void);     /* 0x00416a30 */
extern void Temple_Tick(void);        /* 0x00416dc0 */
extern void Temple_Activate(void);    /* 0x00416b50 */
extern void Temple_Interact(void);    /* 0x00416a60 */
extern void Temple_Remove(void);      /* 0x00416e20 */
extern void Temple_Add(void);         /* 0x00416e00 */

// FUNCTION: LEGOLAND 0x00416e50
void Temple_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("TEMPLE", elem->name) == 0) {
        def->cb_create   = Temple_Create;
        def->cb_destroy  = Temple_Destroy;
        def->cb_8c       = Temple_Tick;
        def->cb_activate = Temple_Activate;
        def->cb_interact = Temple_Interact;
        def->cb_remove   = Temple_Remove;
        def->cb_add      = Temple_Add;
    }
}

/* ======================================================================
 * GOLD RUSH -- 0x004078f0
 * (+0xbc/+0xb8 are ridesave.c's SaveGoldWash/LoadGoldWash, already matched)
 * ====================================================================== */
extern void GoldRush_Create(void);      /* 0x00406a10 */
extern void GoldRush_Destroy(void);     /* 0x00406ab0 */
extern void GoldRush_Tick(void);        /* 0x004075b0 */
extern void GoldRush_Activate(void);    /* 0x004072b0 */
extern void GoldRush_Interact(void);    /* 0x00406b10 */
extern void GoldRush_Add(void);         /* 0x004075f0 */
extern void GoldRush_Remove(void);      /* 0x004076e0 */
extern int  LoadGoldWash(void);         /* 0x00407870 (ridesave.c) */
extern int  SaveGoldWash(void);         /* 0x00407800 (ridesave.c) */

// FUNCTION: LEGOLAND 0x004078f0
void GoldRush_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("GOLD RUSH", elem->name) == 0) {
        def->cb_create   = GoldRush_Create;
        def->cb_destroy  = GoldRush_Destroy;
        def->cb_8c       = GoldRush_Tick;
        def->cb_activate = GoldRush_Activate;
        def->cb_interact = GoldRush_Interact;
        def->cb_add      = GoldRush_Add;
        def->cb_remove   = GoldRush_Remove;
        def->cb_load     = LoadGoldWash;
        def->cb_save     = SaveGoldWash;
    }
}

/* ======================================================================
 * CATAPULT -- 0x00403bb0
 * ====================================================================== */
extern void Catapult_Create(void);      /* 0x004031e0 */
extern void Catapult_Destroy(void);     /* 0x00403250 */
extern void Catapult_Tick(void);        /* 0x00403930 */
extern void Catapult_Activate(void);    /* 0x00403820 */
extern void Catapult_Interact(void);    /* 0x00403270 */
extern void Catapult_Remove(void);      /* 0x004039a0 */
extern void Catapult_Add(void);         /* 0x00403970 */
extern void Catapult_Draw(void);        /* 0x004039e0 */
extern int  SaveCatapult(void);         /* 0x00403a20 */
extern int  LoadCatapult(void);         /* 0x00403af0 */

// FUNCTION: LEGOLAND 0x00403bb0
void Catapult_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("CATAPULT", elem->name) == 0) {
        def->cb_create   = Catapult_Create;
        def->cb_destroy  = Catapult_Destroy;
        def->cb_8c       = Catapult_Tick;
        def->cb_activate = Catapult_Activate;
        def->cb_interact = Catapult_Interact;
        def->cb_remove   = Catapult_Remove;
        def->cb_add      = Catapult_Add;
        def->cb_draw     = Catapult_Draw;
        def->cb_save     = SaveCatapult;
        def->cb_load     = LoadCatapult;
    }
}

/* ======================================================================
 * COPTERS -- 0x00405110
 * ====================================================================== */
extern void Copters_Create(void);      /* 0x00403d90 */
extern void Copters_Tick(void);        /* 0x00404450 */
extern void Copters_Add(void);         /* 0x00404600 */
extern void Copters_Remove(void);      /* 0x00404580 */
extern void Copters_Activate(void);    /* 0x00404be0 */
extern void Copters_Draw(void);        /* 0x00404490 */
extern void Copters_Interact(void);    /* 0x00404290 */
extern void Copters_Destroy(void);     /* 0x00404040 */
extern int  SaveCopters(void);         /* 0x00404f60 */
extern int  LoadCopters(void);         /* 0x00405050 */

// FUNCTION: LEGOLAND 0x00405110
void Copters_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("COPTERS", elem->name) == 0) {
        def->cb_create   = Copters_Create;
        def->cb_8c       = Copters_Tick;
        def->cb_add      = Copters_Add;
        def->cb_remove   = Copters_Remove;
        def->cb_activate = Copters_Activate;
        def->cb_draw     = Copters_Draw;
        def->cb_interact = Copters_Interact;
        def->cb_destroy  = Copters_Destroy;
        def->cb_save     = SaveCopters;
        def->cb_load     = LoadCopters;
    }
}

/* ======================================================================
 * SAFARI RIDE -- 0x00415030
 * ====================================================================== */
extern void SafariRide_Create(void);      /* 0x00414d90 */
extern void SafariRide_Destroy(void);     /* 0x00414ea0 */
extern void SafariRide_Tick(void);        /* 0x00414f00 */
extern void SafariRide_Activate(void);    /* 0x00415220 */
extern void SafariRide_Interact(void);    /* 0x00414b80 */
extern void SafariRide_Remove(void);      /* 0x00414f40 */
extern void SafariRide_Add(void);         /* 0x00414fc0 */
extern void SafariRide_Draw(void);        /* 0x00414ff0 */
extern int  SaveSafariRide(void);         /* 0x004157b0 */
extern int  LoadSafariRide(void);         /* 0x00415820 */

// FUNCTION: LEGOLAND 0x00415030
void SafariRide_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("SAFARI RIDE", elem->name) == 0) {
        def->cb_create   = SafariRide_Create;
        def->cb_destroy  = SafariRide_Destroy;
        def->cb_8c       = SafariRide_Tick;
        def->cb_activate = SafariRide_Activate;
        def->cb_interact = SafariRide_Interact;
        def->cb_remove   = SafariRide_Remove;
        def->cb_add      = SafariRide_Add;
        def->cb_draw     = SafariRide_Draw;
        def->cb_save     = SaveSafariRide;
        def->cb_load     = LoadSafariRide;
    }
}

/* ======================================================================
 * SPIDER RIDE -- 0x00416160
 * ====================================================================== */
extern void SpiderRide_Create(void);      /* 0x00415e80 */
extern void SpiderRide_Destroy(void);     /* 0x00415fd0 */
extern void SpiderRide_Tick(void);        /* 0x00416060 */
extern void SpiderRide_Activate(void);    /* 0x00416330 */
extern void SpiderRide_Interact(void);    /* 0x00415ae0 */
extern void SpiderRide_Remove(void);      /* 0x004160a0 */
extern void SpiderRide_Add(void);         /* 0x004160f0 */
extern void SpiderRide_Draw(void);        /* 0x00416120 */
extern int  SaveSpiderRide(void);         /* 0x00416880 */
extern int  LoadSpiderRide(void);         /* 0x004168f0 */

// FUNCTION: LEGOLAND 0x00416160
void SpiderRide_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("SPIDER RIDE", elem->name) == 0) {
        def->cb_create   = SpiderRide_Create;
        def->cb_destroy  = SpiderRide_Destroy;
        def->cb_8c       = SpiderRide_Tick;
        def->cb_activate = SpiderRide_Activate;
        def->cb_interact = SpiderRide_Interact;
        def->cb_remove   = SpiderRide_Remove;
        def->cb_add      = SpiderRide_Add;
        def->cb_draw     = SpiderRide_Draw;
        def->cb_save     = SaveSpiderRide;
        def->cb_load     = LoadSpiderRide;
    }
}

/* ======================================================================
 * SPACE TOWER RIDE -- 0x0043b780
 * ====================================================================== */
extern void SpaceTower_Create(void);      /* 0x0043b2b0 */
extern void SpaceTower_Tick(void);        /* 0x0043b420 */
extern void SpaceTower_Activate(void);    /* 0x0043bac0 */
extern void SpaceTower_Draw(void);        /* 0x0043b4e0 */
extern void SpaceTower_Interact(void);    /* 0x0043af50 */
extern void SpaceTower_Remove(void);      /* 0x0043b460 */
extern void SpaceTower_Add(void);         /* 0x0043b4b0 */
extern void SpaceTower_Destroy(void);     /* 0x0043b570 */
extern int  SaveSpaceTower(void);         /* 0x0043b5d0 */
extern int  LoadSpaceTower(void);         /* 0x0043b6a0 */

// FUNCTION: LEGOLAND 0x0043b780
void SpaceTower_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("SPACE TOWER RIDE", elem->name) == 0) {
        def->cb_create   = SpaceTower_Create;
        def->cb_8c       = SpaceTower_Tick;
        def->cb_activate = SpaceTower_Activate;
        def->cb_draw     = SpaceTower_Draw;
        def->cb_interact = SpaceTower_Interact;
        def->cb_remove   = SpaceTower_Remove;
        def->cb_add      = SpaceTower_Add;
        def->cb_destroy  = SpaceTower_Destroy;
        def->cb_save     = SaveSpaceTower;
        def->cb_load     = LoadSpaceTower;
    }
}

/* ======================================================================
 * SPINNING BARRELS RIDE -- 0x0043c760
 * ====================================================================== */
extern void SpinningBarrels_Create(void);      /* 0x0043c340 */
extern void SpinningBarrels_Tick(void);        /* 0x0043c490 */
extern void SpinningBarrels_Activate(void);    /* 0x0043c950 */
extern void SpinningBarrels_Interact(void);    /* 0x0043be70 */
extern void SpinningBarrels_Remove(void);      /* 0x0043c4f0 */
extern void SpinningBarrels_Add(void);         /* 0x0043c540 */
extern void SpinningBarrels_Destroy(void);     /* 0x0043c5b0 */
extern void SpinningBarrels_Draw(void);        /* 0x0043c570 */
extern int  SaveSpinningBarrels(void);         /* 0x0043c620 */
extern int  LoadSpinningBarrels(void);         /* 0x0043c690 */

// FUNCTION: LEGOLAND 0x0043c760
void SpinningBarrels_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("SPINNING BARRELS RIDE", elem->name) == 0) {
        def->cb_create   = SpinningBarrels_Create;
        def->cb_8c       = SpinningBarrels_Tick;
        def->cb_activate = SpinningBarrels_Activate;
        def->cb_interact = SpinningBarrels_Interact;
        def->cb_remove   = SpinningBarrels_Remove;
        def->cb_add      = SpinningBarrels_Add;
        def->cb_destroy  = SpinningBarrels_Destroy;
        def->cb_draw     = SpinningBarrels_Draw;
        def->cb_save     = SaveSpinningBarrels;
        def->cb_load     = LoadSpinningBarrels;
    }
}

/* ======================================================================
 * PLANE RIDE -- 0x0043e220
 * ====================================================================== */
extern void PlaneRide_Create(void);      /* 0x0043dda0 */
extern void PlaneRide_Destroy(void);     /* 0x0043dee0 */
extern void PlaneRide_Tick(void);        /* 0x0043df50 */
extern void PlaneRide_Activate(void);    /* 0x0043e410 */
extern void PlaneRide_Interact(void);    /* 0x0043da60 */
extern void PlaneRide_Remove(void);      /* 0x0043df90 */
extern void PlaneRide_Add(void);         /* 0x0043dfe0 */
extern void PlaneRide_Draw(void);        /* 0x0043e010 */
extern int  LoadPlaneRide(void);         /* 0x0043e110 */
extern int  SavePlaneRide(void);         /* 0x0043e0a0 */

// FUNCTION: LEGOLAND 0x0043e220
void PlaneRide_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("PLANE RIDE", elem->name) == 0) {
        def->cb_create   = PlaneRide_Create;
        def->cb_destroy  = PlaneRide_Destroy;
        def->cb_8c       = PlaneRide_Tick;
        def->cb_activate = PlaneRide_Activate;
        def->cb_interact = PlaneRide_Interact;
        def->cb_remove   = PlaneRide_Remove;
        def->cb_add      = PlaneRide_Add;
        def->cb_draw     = PlaneRide_Draw;
        def->cb_load     = LoadPlaneRide;
        def->cb_save     = SavePlaneRide;
    }
}

/* ======================================================================
 * HEDGE / FLOWERS -- 0x004329c0
 *
 * The odd one out: the two compares are the case-SENSITIVE strcmp
 * intrinsic, inlined as VC6's two-bytes-per-iteration loop, not the
 * _stricmp at 0x004aab90 every other provider calls. Reproduced as written.
 * Both classes are pure scenery: no update, no interact, no save chunk;
 * FLOWERS does not even have a remove handler (its records are never taken
 * off the map by this path) -- an original asymmetry, not a transcription
 * slip.
 * ====================================================================== */
extern void Hedge_Create(void);      /* 0x00432480 */
extern void Hedge_Tick(void);        /* 0x004324d0 */
extern void Hedge_Add(void);         /* 0x004325e0 */
extern void Hedge_Remove(void);      /* 0x00432700 */
extern void Hedge_Draw(void);        /* 0x00432810 */
extern void Hedge_Destroy(void);     /* 0x004324c0 */
extern void Flowers_Create(void);    /* 0x00432870 */
extern void Flowers_Tick(void);      /* 0x004328c0 */
extern void Flowers_Add(void);       /* 0x00432900 */
extern void Flowers_Draw(void);      /* 0x00432960 */
extern void Flowers_Destroy(void);   /* 0x004328b0 */

// FUNCTION: LEGOLAND 0x004329c0
void Garden_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (strcmp(elem->name, "HEDGE") == 0) {
        def->cb_create  = Hedge_Create;
        def->cb_8c      = Hedge_Tick;
        def->cb_add     = Hedge_Add;
        def->cb_remove  = Hedge_Remove;
        def->cb_draw    = Hedge_Draw;
        def->cb_destroy = Hedge_Destroy;
    } else if (strcmp(elem->name, "FLOWERS") == 0) {
        def->cb_create  = Flowers_Create;
        def->cb_8c      = Flowers_Tick;
        def->cb_add     = Flowers_Add;
        def->cb_draw    = Flowers_Draw;
        def->cb_destroy = Flowers_Destroy;
    }
}

/* ======================================================================
 * WATER WORKS * -- 0x00418c80  (five classes)
 *
 * The water park: an ENTRANCE, the WATER BLOCK that is the pool itself, and
 * three set-dressing fountains. Only the WATER BLOCK and the ELEPHANT
 * FOUNTAIN own a save chunk (ridesave.c's 0x0c-byte "WATER BLOCK" and
 * "ELEPHANT F" record lists). The CROCODILE FOUNTAIN is the thinnest class
 * in the game: an update, an add and a remove and nothing else -- no
 * create, so it never loads resources of its own.
 * ====================================================================== */
extern void WWEntrance_Create(void);        /* 0x00417c00 */
extern void WWEntrance_Destroy(void);       /* 0x00417ae0 */
extern void WWEntrance_Add(void);           /* 0x00417c20 */
extern void WWEntrance_Remove(void);        /* 0x00417c70 */

extern void WaterBlock_Create(void);        /* 0x00417d30 */
extern void WaterBlock_Add(void);           /* 0x004181e0 */
extern void WaterBlock_Remove(void);        /* 0x00418230 */
extern void WaterBlock_Activate(void);      /* 0x00417f90 */
extern void WaterBlock_Destroy(void);       /* 0x00417e40 */
extern void WaterBlock_Draw(void);          /* 0x00418110 */
extern void WaterBlock_Interact(void);      /* 0x004181a0 */
extern void WaterBlock_Update(void);        /* 0x00417dd0 */
extern int  SaveWaterBlock(void);           /* 0x00418aa0 */
extern int  LoadWaterBlock(void);           /* 0x00418b10 */

extern void Shower_Create(void);            /* 0x004182e0 */
extern void Shower_Add(void);               /* 0x004184e0 */
extern void Shower_Remove(void);            /* 0x00418510 */
extern void Shower_Destroy(void);           /* 0x00418330 */
extern void Shower_Draw(void);              /* 0x00418540 */
extern void Shower_Interact(void);          /* 0x00418450 */
extern void Shower_Activate(void);          /* 0x004183a0 */
extern void Shower_Update(void);            /* 0x004185c0 */

extern void ElephantFountain_Create(void);    /* 0x004186b0 */
extern void ElephantFountain_Add(void);       /* 0x004188d0 */
extern void ElephantFountain_Remove(void);    /* 0x00418910 */
extern void ElephantFountain_Destroy(void);   /* 0x004186f0 */
extern void ElephantFountain_Interact(void);  /* 0x004188c0 */
extern void ElephantFountain_Activate(void);  /* 0x004187f0 */
extern void ElephantFountain_Update(void);    /* 0x00418950 */
extern int  SaveElephantFountain(void);       /* 0x00418b90 */
extern int  LoadElephantFountain(void);       /* 0x00418c00 */

extern void CrocodileFountain_Update(void);   /* 0x00418a30 */
extern void CrocodileFountain_Add(void);      /* 0x004189c0 */
extern void CrocodileFountain_Remove(void);   /* 0x00418a10 */

// FUNCTION: LEGOLAND 0x00418c80
void WaterWorks_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("WATER WORKS ENTRANCE", elem->name) == 0) {
        def->cb_create   = WWEntrance_Create;
        def->cb_destroy  = WWEntrance_Destroy;
        def->cb_add      = WWEntrance_Add;
        def->cb_remove   = WWEntrance_Remove;
    } else if (NameCompare("WATER WORKS WATER BLOCK", elem->name) == 0) {
        def->cb_create   = WaterBlock_Create;
        def->cb_add      = WaterBlock_Add;
        def->cb_remove   = WaterBlock_Remove;
        def->cb_activate = WaterBlock_Activate;
        def->cb_destroy  = WaterBlock_Destroy;
        def->cb_draw     = WaterBlock_Draw;
        def->cb_interact = WaterBlock_Interact;
        def->cb_90       = WaterBlock_Update;
        def->cb_save     = SaveWaterBlock;
        def->cb_load     = LoadWaterBlock;
    } else if (NameCompare("WATER WORKS SHOWER", elem->name) == 0) {
        def->cb_create   = Shower_Create;
        def->cb_add      = Shower_Add;
        def->cb_remove   = Shower_Remove;
        def->cb_destroy  = Shower_Destroy;
        def->cb_draw     = Shower_Draw;
        def->cb_interact = Shower_Interact;
        def->cb_activate = Shower_Activate;
        def->cb_90       = Shower_Update;
    } else if (NameCompare("WATER WORKS ELEPHANT FOUNTAIN", elem->name) == 0) {
        def->cb_create   = ElephantFountain_Create;
        def->cb_add      = ElephantFountain_Add;
        def->cb_remove   = ElephantFountain_Remove;
        def->cb_destroy  = ElephantFountain_Destroy;
        def->cb_interact = ElephantFountain_Interact;
        def->cb_activate = ElephantFountain_Activate;
        def->cb_90       = ElephantFountain_Update;
        def->cb_save     = SaveElephantFountain;
        def->cb_load     = LoadElephantFountain;
    } else if (NameCompare("WATER WORKS CROCODILE FOUNTAIN", elem->name) == 0) {
        def->cb_90       = CrocodileFountain_Update;
        def->cb_add      = CrocodileFountain_Add;
        def->cb_remove   = CrocodileFountain_Remove;
    }
}

/* ======================================================================
 * WESTERN TOWN / the shops -- 0x0043a400  (nine classes)
 *
 * Nine building classes served by one provider. Seven of them are pure
 * shopfronts and share TWO handlers outright -- the draw at 0x0043a390 and
 * the remove at 0x0043a3d0 -- which is why those two addresses appear in
 * every arm. Only the three classes that own state have their own place
 * handler: JAIL CELL (whose +0xbc/+0xb8 are ridesave.c's already-matched
 * SaveJailCells/LoadJailCells, the 0x1c-byte record list), LEGO SHOP 1 and
 * LEGO MEDIA SHOP.
 * ====================================================================== */
extern void Shop_Draw(void);           /* 0x0043a390  shared by all nine */
extern void Shop_Remove(void);         /* 0x0043a3d0  shared by six */

extern void GeneralStore_Create(void);     /* 0x004375d0 */
extern void GeneralStore_Destroy(void);    /* 0x00437610 */
extern void GeneralStore_Tick(void);       /* 0x00437630 */
extern void GeneralStore_Activate(void);   /* 0x004378e0 */
extern void GeneralStore_Interact(void);   /* 0x00437670 */

extern void Sheriff_Create(void);          /* 0x00437ba0 */
extern void Sheriff_Destroy(void);         /* 0x00437bd0 */
extern void Sheriff_Tick(void);            /* 0x00437bf0 */
extern void Sheriff_Activate(void);        /* 0x00437c90 */
extern void Sheriff_Interact(void);        /* 0x00437c30 */

extern void JailCell_Create(void);         /* 0x00438070 */
extern void JailCell_Destroy(void);        /* 0x004380f0 */
extern void JailCell_Tick(void);           /* 0x00438110 */
extern void JailCell_Activate(void);       /* 0x00438430 */
extern void JailCell_Add(void);            /* 0x00437f60 */
extern void JailCell_Remove(void);         /* 0x00438020 */
extern void JailCell_Interact(void);       /* 0x00438150 */
extern int  LoadJailCells(void);           /* 0x004387f0 (ridesave.c) */
extern int  SaveJailCells(void);           /* 0x00438780 (ridesave.c) */

extern void Bank_Create(void);             /* 0x00438870 */
extern void Bank_Destroy(void);            /* 0x004388a0 */
extern void Bank_Tick(void);               /* 0x004388c0 */
extern void Bank_Activate(void);           /* 0x00438960 */
extern void Bank_Interact(void);           /* 0x00438900 */

extern void Saloon_Create(void);           /* 0x00438c60 */
extern void Saloon_Destroy(void);          /* 0x00438ca0 */
extern void Saloon_Tick(void);             /* 0x00438cc0 */
extern void Saloon_Activate(void);         /* 0x00438f10 */
extern void Saloon_Interact(void);         /* 0x00438d00 */

extern void Institute_Create(void);        /* 0x0043a0f0 */
extern void Institute_Destroy(void);       /* 0x0043a120 */
extern void Institute_Tick(void);          /* 0x0043a140 */
extern void Institute_Activate(void);      /* 0x0043a1e0 */
extern void Institute_Interact(void);      /* 0x0043a180 */

extern void LegoShop1_Create(void);        /* 0x00439200 */
extern void LegoShop1_Add(void);           /* 0x00439320 */
extern void LegoShop1_Remove(void);        /* 0x00439350 */
extern void LegoShop1_Destroy(void);       /* 0x004393e0 */
extern void LegoShop1_Tick(void);          /* 0x004393a0 */
extern void LegoShop1_Activate(void);      /* 0x00439460 */
extern void LegoShop1_Interact(void);      /* 0x00439400 */

extern void LegoShop2_Create(void);        /* 0x004396d0 */
extern void LegoShop2_Destroy(void);       /* 0x00439700 */
extern void LegoShop2_Tick(void);          /* 0x00439720 */
extern void LegoShop2_Activate(void);      /* 0x00439950 */
extern void LegoShop2_Interact(void);      /* 0x00439760 */

extern void MediaShop_Create(void);        /* 0x00439c20 */
extern void MediaShop_Add(void);           /* 0x00439c60 */
extern void MediaShop_Remove(void);        /* 0x00439c90 */
extern void MediaShop_Destroy(void);       /* 0x00439ce0 */
extern void MediaShop_Tick(void);          /* 0x00439d00 */
extern void MediaShop_Activate(void);      /* 0x00439ef0 */
extern void MediaShop_Interact(void);      /* 0x00439d40 */

// FUNCTION: LEGOLAND 0x0043a400
void WesternTown_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("GENERAL STORE", elem->name) == 0) {
        def->cb_create   = GeneralStore_Create;
        def->cb_destroy  = GeneralStore_Destroy;
        def->cb_8c       = GeneralStore_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = GeneralStore_Activate;
        def->cb_remove   = Shop_Remove;
        def->cb_interact = GeneralStore_Interact;
    } else if (NameCompare("SHERIFF", elem->name) == 0) {
        def->cb_create   = Sheriff_Create;
        def->cb_destroy  = Sheriff_Destroy;
        def->cb_8c       = Sheriff_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = Sheriff_Activate;
        def->cb_remove   = Shop_Remove;
        def->cb_interact = Sheriff_Interact;
    } else if (NameCompare("JAIL CELL", elem->name) == 0) {
        def->cb_create   = JailCell_Create;
        def->cb_destroy  = JailCell_Destroy;
        def->cb_8c       = JailCell_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = JailCell_Activate;
        def->cb_add      = JailCell_Add;
        def->cb_remove   = JailCell_Remove;
        def->cb_interact = JailCell_Interact;
        def->cb_load     = LoadJailCells;
        def->cb_save     = SaveJailCells;
    } else if (NameCompare("BANK", elem->name) == 0) {
        def->cb_create   = Bank_Create;
        def->cb_destroy  = Bank_Destroy;
        def->cb_8c       = Bank_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = Bank_Activate;
        def->cb_remove   = Shop_Remove;
        def->cb_interact = Bank_Interact;
    } else if (NameCompare("SALOON", elem->name) == 0) {
        def->cb_create   = Saloon_Create;
        def->cb_destroy  = Saloon_Destroy;
        def->cb_8c       = Saloon_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = Saloon_Activate;
        def->cb_remove   = Shop_Remove;
        def->cb_interact = Saloon_Interact;
    } else if (NameCompare("EXPLORERS INSTITUTE", elem->name) == 0) {
        def->cb_create   = Institute_Create;
        def->cb_destroy  = Institute_Destroy;
        def->cb_8c       = Institute_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = Institute_Activate;
        def->cb_remove   = Shop_Remove;
        def->cb_interact = Institute_Interact;
    } else if (NameCompare("LEGO SHOP 1", elem->name) == 0) {
        def->cb_create   = LegoShop1_Create;
        def->cb_add      = LegoShop1_Add;
        def->cb_remove   = LegoShop1_Remove;
        def->cb_destroy  = LegoShop1_Destroy;
        def->cb_8c       = LegoShop1_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = LegoShop1_Activate;
        def->cb_interact = LegoShop1_Interact;
    } else if (NameCompare("LEGO SHOP 2", elem->name) == 0) {
        def->cb_create   = LegoShop2_Create;
        def->cb_destroy  = LegoShop2_Destroy;
        def->cb_8c       = LegoShop2_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = LegoShop2_Activate;
        def->cb_remove   = Shop_Remove;
        def->cb_interact = LegoShop2_Interact;
    } else if (NameCompare("LEGO MEDIA SHOP", elem->name) == 0) {
        def->cb_create   = MediaShop_Create;
        def->cb_add      = MediaShop_Add;
        def->cb_remove   = MediaShop_Remove;
        def->cb_destroy  = MediaShop_Destroy;
        def->cb_8c       = MediaShop_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = MediaShop_Activate;
        def->cb_interact = MediaShop_Interact;
    }
}

/* ======================================================================
 * LOG FLUME * -- 0x00410d60  (ten classes, the biggest provider)
 *
 * The log flume is built out of ten classes: the ENTRANCE (the only one with
 * a save chunk and the only user of the +0xc0 extra slot in this file), the
 * plain TRACK, four SPECIAL CORNERs, and the CSAW / TUNNEL / DROP / HOLD UP
 * set pieces. Every class except the ENTRANCE fills the same eight-and-nine
 * slot shape -- create, tick, draw, interact, update, update2, add, remove,
 * destroy -- and the nine non-ENTRANCE, non-TRACK classes all share ONE draw
 * handler, 0x0040ed50.
 *
 * The TRACK arm has the file's only side effect beyond slot stores: it
 * caches the class's own ObjDef in a module global at 0x0082c688 before
 * filling the slots, so the track code can reach its ObjDef without an
 * element lookup. The store is emitted BEFORE the `pop esi`, i.e. it is the
 * first statement of the arm in the source.
 * ====================================================================== */
extern RideDef* g_logflume_track_def;   /* 0x0082c688 */

extern void LFEntrance_Create(void);       /* 0x0040a2e0 */
extern void LFEntrance_Tick(void);         /* 0x0040a540 */
extern void LFEntrance_Update(void);       /* 0x0040a930 */
extern void LFEntrance_Update2(void);      /* 0x0040aac0 */
extern void LFEntrance_Add(void);          /* 0x0040a600 */
extern void LFEntrance_Remove(void);       /* 0x0040abf0 */
extern void LFEntrance_Activate(void);     /* 0x0040bf70 */
extern void LFEntrance_Interact(void);     /* 0x0040b420 */
extern void LFEntrance_Destroy(void);      /* 0x0040a410 */
extern int  SaveLogFlume(void);            /* 0x00410930 */
extern int  LoadLogFlume(void);            /* 0x00410c10 */
extern void LFEntrance_Extra(void);        /* 0x004119c0 */

extern void LFTrack_Create(void);          /* 0x0040c350 */
extern void LFTrack_Tick(void);            /* 0x0040dbb0 */
extern void LFTrack_Draw(void);            /* 0x0040c970 */
extern void LFTrack_Interact(void);        /* 0x0040cc50 */
extern void LFTrack_Update(void);          /* 0x0040c4a0 */
extern void LFTrack_Update2(void);         /* 0x0040c6c0 */
extern void LFTrack_Add(void);             /* 0x0040c780 */
extern void LFTrack_Remove(void);          /* 0x0040c8d0 */
extern void LFTrack_Destroy(void);         /* 0x0040c430 */

extern void LFPiece_Draw(void);            /* 0x0040ed50  shared by nine */

extern void LFCorner1_Create(void);        /* 0x0040e8b0 */
extern void LFCorner1_Tick(void);          /* 0x0040e630 */
extern void LFCorner1_Interact(void);      /* 0x0040edb0 */
extern void LFCorner1_Update(void);        /* 0x0040e6f0 */
extern void LFCorner1_Update2(void);       /* 0x0040e830 */
extern void LFCorner1_Add(void);           /* 0x0040ead0 */
extern void LFCorner1_Remove(void);        /* 0x0040ec90 */
extern void LFCorner1_Destroy(void);       /* 0x0040ea30 */

extern void LFCorner2_Create(void);        /* 0x0040e920 */
extern void LFCorner2_Tick(void);          /* 0x0040e660 */
extern void LFCorner2_Interact(void);      /* 0x0040ee60 */
extern void LFCorner2_Update(void);        /* 0x0040e740 */
extern void LFCorner2_Update2(void);       /* 0x0040e850 */
extern void LFCorner2_Add(void);           /* 0x0040eb40 */
extern void LFCorner2_Remove(void);        /* 0x0040ecc0 */
extern void LFCorner2_Destroy(void);       /* 0x0040ea60 */

extern void LFCorner3_Create(void);        /* 0x0040e970 */
extern void LFCorner3_Tick(void);          /* 0x0040e690 */
extern void LFCorner3_Interact(void);      /* 0x0040ef00 */
extern void LFCorner3_Update(void);        /* 0x0040e790 */
extern void LFCorner3_Update2(void);       /* 0x0040e870 */
extern void LFCorner3_Add(void);           /* 0x0040ebb0 */
extern void LFCorner3_Remove(void);        /* 0x0040ecf0 */
extern void LFCorner3_Destroy(void);       /* 0x0040ea80 */

extern void LFCorner4_Create(void);        /* 0x0040e9e0 */
extern void LFCorner4_Tick(void);          /* 0x0040e6c0 */
extern void LFCorner4_Interact(void);      /* 0x0040efb0 */
extern void LFCorner4_Update(void);        /* 0x0040e7e0 */
extern void LFCorner4_Update2(void);       /* 0x0040e890 */
extern void LFCorner4_Add(void);           /* 0x0040ec20 */
extern void LFCorner4_Remove(void);        /* 0x0040ed20 */
extern void LFCorner4_Destroy(void);       /* 0x0040eab0 */

extern void LFCsaw_Create(void);           /* 0x0040f8b0 */
extern void LFCsaw_Tick(void);             /* 0x0040fa00 */
extern void LFCsaw_Interact(void);         /* 0x0040f920 */
extern void LFCsaw_Update(void);           /* 0x0040fa20 */
extern void LFCsaw_Update2(void);          /* 0x0040fa50 */
extern void LFCsaw_Add(void);              /* 0x0040fa60 */
extern void LFCsaw_Remove(void);           /* 0x0040fab0 */
extern void LFCsaw_Destroy(void);          /* 0x0040f900 */

extern void LFTunnel_Create(void);         /* 0x0040f3e0 */
extern void LFTunnel_Tick(void);           /* 0x0040f4f0 */
extern void LFTunnel_Interact(void);       /* 0x0040f450 */
extern void LFTunnel_Update(void);         /* 0x0040f510 */
extern void LFTunnel_Update2(void);        /* 0x0040f5a0 */
extern void LFTunnel_Add(void);            /* 0x0040f540 */
extern void LFTunnel_Remove(void);         /* 0x0040f580 */
extern void LFTunnel_Destroy(void);        /* 0x0040f430 */

extern void LFDrop_Create(void);           /* 0x004103e0 */
extern void LFDrop_Tick(void);             /* 0x004106e0 */
extern void LFDrop_Interact(void);         /* 0x004104b0 */
extern void LFDrop_Update(void);           /* 0x00410700 */
extern void LFDrop_Update2(void);          /* 0x00410730 */
extern void LFDrop_Add(void);              /* 0x00410740 */
extern void LFDrop_Remove(void);           /* 0x00410790 */
extern void LFDrop_Destroy(void);          /* 0x00410450 */

extern void LFHoldUp_Create(void);         /* 0x0040ff30 */
extern void LFHoldUp_Tick(void);           /* 0x004100b0 */
extern void LFHoldUp_Interact(void);       /* 0x0040ffd0 */
extern void LFHoldUp_Update(void);         /* 0x004100d0 */
extern void LFHoldUp_Update2(void);        /* 0x00410100 */
extern void LFHoldUp_Add(void);            /* 0x00410110 */
extern void LFHoldUp_Remove(void);         /* 0x00410160 */
extern void LFHoldUp_Destroy(void);        /* 0x0040ffa0 */

// FUNCTION: LEGOLAND 0x00410d60
void LogFlume_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("LOG FLUME ENTRANCE", elem->name) == 0) {
        def->cb_create   = LFEntrance_Create;
        def->cb_8c       = LFEntrance_Tick;
        def->cb_90       = LFEntrance_Update;
        def->cb_94       = LFEntrance_Update2;
        def->cb_add      = LFEntrance_Add;
        def->cb_remove   = LFEntrance_Remove;
        def->cb_activate = LFEntrance_Activate;
        def->cb_interact = LFEntrance_Interact;
        def->cb_destroy  = LFEntrance_Destroy;
        def->cb_save     = SaveLogFlume;
        def->cb_load     = LoadLogFlume;
        def->cb_c0       = LFEntrance_Extra;
    } else if (NameCompare("LOG FLUME TRACK", elem->name) == 0) {
        g_logflume_track_def = elem->data;
        def->cb_create   = LFTrack_Create;
        def->cb_8c       = LFTrack_Tick;
        def->cb_draw     = LFTrack_Draw;
        def->cb_interact = LFTrack_Interact;
        def->cb_90       = LFTrack_Update;
        def->cb_94       = LFTrack_Update2;
        def->cb_add      = LFTrack_Add;
        def->cb_remove   = LFTrack_Remove;
        def->cb_destroy  = LFTrack_Destroy;
    } else if (NameCompare("LOG FLUME SPECIAL CORNER 1", elem->name) == 0) {
        def->cb_create   = LFCorner1_Create;
        def->cb_8c       = LFCorner1_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFCorner1_Interact;
        def->cb_90       = LFCorner1_Update;
        def->cb_94       = LFCorner1_Update2;
        def->cb_add      = LFCorner1_Add;
        def->cb_remove   = LFCorner1_Remove;
        def->cb_destroy  = LFCorner1_Destroy;
    } else if (NameCompare("LOG FLUME SPECIAL CORNER 2", elem->name) == 0) {
        def->cb_create   = LFCorner2_Create;
        def->cb_8c       = LFCorner2_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFCorner2_Interact;
        def->cb_90       = LFCorner2_Update;
        def->cb_94       = LFCorner2_Update2;
        def->cb_add      = LFCorner2_Add;
        def->cb_remove   = LFCorner2_Remove;
        def->cb_destroy  = LFCorner2_Destroy;
    } else if (NameCompare("LOG FLUME SPECIAL CORNER 3", elem->name) == 0) {
        def->cb_create   = LFCorner3_Create;
        def->cb_8c       = LFCorner3_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFCorner3_Interact;
        def->cb_90       = LFCorner3_Update;
        def->cb_94       = LFCorner3_Update2;
        def->cb_add      = LFCorner3_Add;
        def->cb_remove   = LFCorner3_Remove;
        def->cb_destroy  = LFCorner3_Destroy;
    } else if (NameCompare("LOG FLUME SPECIAL CORNER 4", elem->name) == 0) {
        def->cb_create   = LFCorner4_Create;
        def->cb_8c       = LFCorner4_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFCorner4_Interact;
        def->cb_90       = LFCorner4_Update;
        def->cb_94       = LFCorner4_Update2;
        def->cb_add      = LFCorner4_Add;
        def->cb_remove   = LFCorner4_Remove;
        def->cb_destroy  = LFCorner4_Destroy;
    } else if (NameCompare("LOG FLUME CSAW", elem->name) == 0) {
        def->cb_create   = LFCsaw_Create;
        def->cb_8c       = LFCsaw_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFCsaw_Interact;
        def->cb_90       = LFCsaw_Update;
        def->cb_94       = LFCsaw_Update2;
        def->cb_add      = LFCsaw_Add;
        def->cb_remove   = LFCsaw_Remove;
        def->cb_destroy  = LFCsaw_Destroy;
    } else if (NameCompare("LOG FLUME TUNNEL", elem->name) == 0) {
        def->cb_create   = LFTunnel_Create;
        def->cb_8c       = LFTunnel_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFTunnel_Interact;
        def->cb_90       = LFTunnel_Update;
        def->cb_94       = LFTunnel_Update2;
        def->cb_add      = LFTunnel_Add;
        def->cb_remove   = LFTunnel_Remove;
        def->cb_destroy  = LFTunnel_Destroy;
    } else if (NameCompare("LOG FLUME DROP", elem->name) == 0) {
        def->cb_create   = LFDrop_Create;
        def->cb_8c       = LFDrop_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFDrop_Interact;
        def->cb_90       = LFDrop_Update;
        def->cb_94       = LFDrop_Update2;
        def->cb_add      = LFDrop_Add;
        def->cb_remove   = LFDrop_Remove;
        def->cb_destroy  = LFDrop_Destroy;
    } else if (NameCompare("LOG FLUME HOLD UP", elem->name) == 0) {
        def->cb_create   = LFHoldUp_Create;
        def->cb_8c       = LFHoldUp_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFHoldUp_Interact;
        def->cb_90       = LFHoldUp_Update;
        def->cb_94       = LFHoldUp_Update2;
        def->cb_add      = LFHoldUp_Add;
        def->cb_remove   = LFHoldUp_Remove;
        def->cb_destroy  = LFHoldUp_Destroy;
    }
}
