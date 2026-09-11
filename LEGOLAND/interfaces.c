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
#ifndef LEGOLAND_PORTABLE
extern void CastleLevel1_Add(void);         /* 0x00403060 */
#else   /* PORT-M7: goldrush.c void CastleLevel1_Place(void* obj, Pos* pos) */
extern void CastleLevel1_Add(void* obj, void* pos);   /* 0x00403060 */
#endif
extern void CastleLevel1_Remove(void);      /* 0x00403030 */
#ifndef LEGOLAND_PORTABLE
extern void CastleLevel1_Activate(void);    /* 0x00402dc0 */
#else   /* PORT-M7: goldrush.c void CastleLevel1_TickRiders(RideElem* elem) */
extern void CastleLevel1_Activate(void* elem);   /* 0x00402dc0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void CastleLevel1_Interact(void);    /* 0x00402d00 */
#else   /* PORT-M7: goldrush.c void CastleLevel1_Draw(RideElem* elem, int x, int y, MapSquare* sq, void* clip, int mode) */
extern void CastleLevel1_Interact(void* elem, int x, int y, void* sq, void* clip, int mode);   /* 0x00402d00 */
#endif

#ifdef LEGOLAND_PORTABLE
/* PORT-M7: the same treatment for the +0xac teardown slot (and the two
 * +0xa8 handlers below it) that PORT-M3 gave screen.c and castleobj.c, for
 * the 33 western-town / garden / water-works / log-flume classes this file
 * registers.  sysmisc.c:664 calls +0xac as `d->dtor(d->dtor_arg)` and
 * renderview.c:1130 calls +0xa8 as `cls->prerender(cls->ctx)`, one argument
 * each; these bodies genuinely take none (westtown.c:384
 * `void Bank_FreeResources(void)` is typical), which was free on x86 cdecl
 * and is a call_indirect type mismatch on wasm.  An adapter of the slot's
 * own type drops the argument, so the slot holds ONE wasm type.  The
 * matched bodies are untouched. */
extern void CastleLevel1_Destroy(void);
static void ll_cb_ac_CastleLevel1_Destroy(void* ll_elem)
{
    (void)ll_elem;
    CastleLevel1_Destroy();
}
extern void Fort_Destroy(void);
static void ll_cb_ac_Fort_Destroy(void* ll_elem)
{
    (void)ll_elem;
    Fort_Destroy();
}
extern void Temple_Destroy(void);
static void ll_cb_ac_Temple_Destroy(void* ll_elem)
{
    (void)ll_elem;
    Temple_Destroy();
}
extern void GoldRush_Destroy(void);
static void ll_cb_ac_GoldRush_Destroy(void* ll_elem)
{
    (void)ll_elem;
    GoldRush_Destroy();
}
extern void Catapult_Destroy(void);
static void ll_cb_ac_Catapult_Destroy(void* ll_elem)
{
    (void)ll_elem;
    Catapult_Destroy();
}
extern void Copters_Destroy(void);
static void ll_cb_ac_Copters_Destroy(void* ll_elem)
{
    (void)ll_elem;
    Copters_Destroy();
}
extern void SpaceTower_Destroy(void);
static void ll_cb_ac_SpaceTower_Destroy(void* ll_elem)
{
    (void)ll_elem;
    SpaceTower_Destroy();
}
extern void SpinningBarrels_Destroy(void);
static void ll_cb_ac_SpinningBarrels_Destroy(void* ll_elem)
{
    (void)ll_elem;
    SpinningBarrels_Destroy();
}
extern void Hedge_Destroy(void);
static void ll_cb_ac_Hedge_Destroy(void* ll_elem)
{
    (void)ll_elem;
    Hedge_Destroy();
}
extern void Flowers_Destroy(void);
static void ll_cb_ac_Flowers_Destroy(void* ll_elem)
{
    (void)ll_elem;
    Flowers_Destroy();
}
extern void WWEntrance_Destroy(void);
static void ll_cb_ac_WWEntrance_Destroy(void* ll_elem)
{
    (void)ll_elem;
    WWEntrance_Destroy();
}
extern void WaterBlock_Destroy(void);
static void ll_cb_ac_WaterBlock_Destroy(void* ll_elem)
{
    (void)ll_elem;
    WaterBlock_Destroy();
}
extern void Shower_Destroy(void);
static void ll_cb_ac_Shower_Destroy(void* ll_elem)
{
    (void)ll_elem;
    Shower_Destroy();
}
extern void Shower_Activate(void);
static void ll_cb_a8_Shower_Activate(void* ll_elem)
{
    (void)ll_elem;
    Shower_Activate();
}
extern void ElephantFountain_Destroy(void);
static void ll_cb_ac_ElephantFountain_Destroy(void* ll_elem)
{
    (void)ll_elem;
    ElephantFountain_Destroy();
}
extern void ElephantFountain_Activate(void);
static void ll_cb_a8_ElephantFountain_Activate(void* ll_elem)
{
    (void)ll_elem;
    ElephantFountain_Activate();
}
extern void GeneralStore_Destroy(void);
static void ll_cb_ac_GeneralStore_Destroy(void* ll_elem)
{
    (void)ll_elem;
    GeneralStore_Destroy();
}
extern void Sheriff_Destroy(void);
static void ll_cb_ac_Sheriff_Destroy(void* ll_elem)
{
    (void)ll_elem;
    Sheriff_Destroy();
}
extern void JailCell_Destroy(void);
static void ll_cb_ac_JailCell_Destroy(void* ll_elem)
{
    (void)ll_elem;
    JailCell_Destroy();
}
extern void Bank_Destroy(void);
static void ll_cb_ac_Bank_Destroy(void* ll_elem)
{
    (void)ll_elem;
    Bank_Destroy();
}
extern void Saloon_Destroy(void);
static void ll_cb_ac_Saloon_Destroy(void* ll_elem)
{
    (void)ll_elem;
    Saloon_Destroy();
}
extern void Institute_Destroy(void);
static void ll_cb_ac_Institute_Destroy(void* ll_elem)
{
    (void)ll_elem;
    Institute_Destroy();
}
extern void LegoShop1_Destroy(void);
static void ll_cb_ac_LegoShop1_Destroy(void* ll_elem)
{
    (void)ll_elem;
    LegoShop1_Destroy();
}
extern void LegoShop2_Destroy(void);
static void ll_cb_ac_LegoShop2_Destroy(void* ll_elem)
{
    (void)ll_elem;
    LegoShop2_Destroy();
}
extern void MediaShop_Destroy(void);
static void ll_cb_ac_MediaShop_Destroy(void* ll_elem)
{
    (void)ll_elem;
    MediaShop_Destroy();
}
extern void LFEntrance_Destroy(void);
static void ll_cb_ac_LFEntrance_Destroy(void* ll_elem)
{
    (void)ll_elem;
    LFEntrance_Destroy();
}
extern void LFTrack_Destroy(void);
static void ll_cb_ac_LFTrack_Destroy(void* ll_elem)
{
    (void)ll_elem;
    LFTrack_Destroy();
}
extern void LFCorner1_Destroy(void);
static void ll_cb_ac_LFCorner1_Destroy(void* ll_elem)
{
    (void)ll_elem;
    LFCorner1_Destroy();
}
extern void LFCorner2_Destroy(void);
static void ll_cb_ac_LFCorner2_Destroy(void* ll_elem)
{
    (void)ll_elem;
    LFCorner2_Destroy();
}
extern void LFCorner3_Destroy(void);
static void ll_cb_ac_LFCorner3_Destroy(void* ll_elem)
{
    (void)ll_elem;
    LFCorner3_Destroy();
}
extern void LFCorner4_Destroy(void);
static void ll_cb_ac_LFCorner4_Destroy(void* ll_elem)
{
    (void)ll_elem;
    LFCorner4_Destroy();
}
extern void LFCsaw_Destroy(void);
static void ll_cb_ac_LFCsaw_Destroy(void* ll_elem)
{
    (void)ll_elem;
    LFCsaw_Destroy();
}
extern void LFTunnel_Destroy(void);
static void ll_cb_ac_LFTunnel_Destroy(void* ll_elem)
{
    (void)ll_elem;
    LFTunnel_Destroy();
}
extern void LFDrop_Destroy(void);
static void ll_cb_ac_LFDrop_Destroy(void* ll_elem)
{
    (void)ll_elem;
    LFDrop_Destroy();
}
extern void LFHoldUp_Destroy(void);
static void ll_cb_ac_LFHoldUp_Destroy(void* ll_elem)
{
    (void)ll_elem;
    LFHoldUp_Destroy();
}
#endif

// FUNCTION: LEGOLAND 0x00403080
void CastleLevel1_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("CASTLE LEVEL 1", elem->name) == 0) {
        def->cb_create   = CastleLevel1_Create;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = CastleLevel1_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_CastleLevel1_Destroy;   /* PORT-M7 */
#endif
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
#ifndef LEGOLAND_PORTABLE
extern void Fort_Activate(void);    /* 0x00406660 */
#else   /* PORT-M7: goldrush.c void Fort_TickRiders(RideElem* elem) */
extern void Fort_Activate(void* elem);   /* 0x00406660 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void Fort_Interact(void);    /* 0x004062c0 */
#else   /* PORT-M7: goldrush.c void Fort_Draw(RideElem* elem, int x, int y, MapSquare* sq, void* clip, int mode) */
extern void Fort_Interact(void* elem, int x, int y, void* sq, void* clip, int mode);   /* 0x004062c0 */
#endif
extern void Fort_Remove(void);      /* 0x00406880 */
#ifndef LEGOLAND_PORTABLE
extern void Fort_Add(void);         /* 0x00406860 */
#else   /* PORT-M7: goldrush.c void Fort_Place(void* obj, Pos* pos) */
extern void Fort_Add(void* obj, void* pos);   /* 0x00406860 */
#endif

// FUNCTION: LEGOLAND 0x004068b0
void Fort_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("FORT", elem->name) == 0) {
        def->cb_create   = Fort_Create;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = Fort_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_Fort_Destroy;   /* PORT-M7 */
#endif
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
#ifndef LEGOLAND_PORTABLE
extern void Temple_Activate(void);    /* 0x00416b50 */
#else   /* PORT-M7: goldrush.c void Temple_TickRiders(RideElem* elem) */
extern void Temple_Activate(void* elem);   /* 0x00416b50 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void Temple_Interact(void);    /* 0x00416a60 */
#else   /* PORT-M7: goldrush.c void Temple_Draw(RideElem* elem, int x, int y, MapSquare* sq, void* clip, int mode) */
extern void Temple_Interact(void* elem, int x, int y, void* sq, void* clip, int mode);   /* 0x00416a60 */
#endif
extern void Temple_Remove(void);      /* 0x00416e20 */
#ifndef LEGOLAND_PORTABLE
extern void Temple_Add(void);         /* 0x00416e00 */
#else   /* PORT-M7: goldrush.c void Temple_Place(void* obj, Pos* pos) */
extern void Temple_Add(void* obj, void* pos);   /* 0x00416e00 */
#endif

// FUNCTION: LEGOLAND 0x00416e50
void Temple_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("TEMPLE", elem->name) == 0) {
        def->cb_create   = Temple_Create;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = Temple_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_Temple_Destroy;   /* PORT-M7 */
#endif
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
#ifndef LEGOLAND_PORTABLE
extern void GoldRush_Activate(void);    /* 0x004072b0 */
#else   /* PORT-M7: goldrush.c void GoldRush_TickRiders(RideElem* elem) */
extern void GoldRush_Activate(void* elem);   /* 0x004072b0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void GoldRush_Interact(void);    /* 0x00406b10 */
#else   /* PORT-M7: goldrush.c void GoldRush_Draw(RideElem* elem, int x, int y, MapSquare* sq, void* clip, int mode) */
extern void GoldRush_Interact(void* elem, int x, int y, void* sq, void* clip, int mode);   /* 0x00406b10 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void GoldRush_Add(void);         /* 0x004075f0 */
#else   /* PORT-M7: goldrush.c void GoldRush_Place(void* obj, Pos* pos) */
extern void GoldRush_Add(void* obj, void* pos);   /* 0x004075f0 */
#endif
extern void GoldRush_Remove(void);      /* 0x004076e0 */
extern int  LoadGoldWash(void);         /* 0x00407870 (ridesave.c) */
extern int  SaveGoldWash(void);         /* 0x00407800 (ridesave.c) */

#ifdef LEGOLAND_PORTABLE
/* PORT-M3: one wasm type per callback slot.  These ObjDef slots are
 * called with the instance pointer the class was registered with:
 *   cb_load +0xb8, called as (elem) by savegame.c:1365
 *   cb_save +0xbc, called as (elem) by savegame.c:941
 * and these bodies never read it -- free on x86 cdecl, where the caller
 * pushes and the caller cleans up, but a wasm call_indirect whose type is
 * not the target's traps.  The portable build registers an adapter of the
 * slot's own type which drops the argument, so the slot holds one type.
 * The matched bodies are untouched. */
extern int LoadCatapult(void);
static int ll_cb_load_LoadCatapult(void* ll_elem)
{
    (void)ll_elem;
    return LoadCatapult();
}
extern int LoadCopters(void);
static int ll_cb_load_LoadCopters(void* ll_elem)
{
    (void)ll_elem;
    return LoadCopters();
}
extern int LoadElephantFountain(void);
static int ll_cb_load_LoadElephantFountain(void* ll_elem)
{
    (void)ll_elem;
    return LoadElephantFountain();
}
extern int LoadGoldWash(void);
static int ll_cb_load_LoadGoldWash(void* ll_elem)
{
    (void)ll_elem;
    return LoadGoldWash();
}
extern int LoadJailCells(void);
static int ll_cb_load_LoadJailCells(void* ll_elem)
{
    (void)ll_elem;
    return LoadJailCells();
}
extern int LoadLogFlume(void);
static int ll_cb_load_LoadLogFlume(void* ll_elem)
{
    (void)ll_elem;
    return LoadLogFlume();
}
extern int LoadSpaceTower(void);
static int ll_cb_load_LoadSpaceTower(void* ll_elem)
{
    (void)ll_elem;
    return LoadSpaceTower();
}
extern int LoadWaterBlock(void);
static int ll_cb_load_LoadWaterBlock(void* ll_elem)
{
    (void)ll_elem;
    return LoadWaterBlock();
}
extern int SaveCatapult(void);
static int ll_cb_save_SaveCatapult(void* ll_elem)
{
    (void)ll_elem;
    return SaveCatapult();
}
extern int SaveCopters(void);
static int ll_cb_save_SaveCopters(void* ll_elem)
{
    (void)ll_elem;
    return SaveCopters();
}
extern int SaveElephantFountain(void);
static int ll_cb_save_SaveElephantFountain(void* ll_elem)
{
    (void)ll_elem;
    return SaveElephantFountain();
}
extern int SaveGoldWash(void);
static int ll_cb_save_SaveGoldWash(void* ll_elem)
{
    (void)ll_elem;
    return SaveGoldWash();
}
extern int SaveJailCells(void);
static int ll_cb_save_SaveJailCells(void* ll_elem)
{
    (void)ll_elem;
    return SaveJailCells();
}
extern int SaveLogFlume(void);
static int ll_cb_save_SaveLogFlume(void* ll_elem)
{
    (void)ll_elem;
    return SaveLogFlume();
}
extern int SavePlaneRide(void);
static int ll_cb_save_SavePlaneRide(void* ll_elem)
{
    (void)ll_elem;
    return SavePlaneRide();
}
extern int SaveSafariRide(void);
static int ll_cb_save_SaveSafariRide(void* ll_elem)
{
    (void)ll_elem;
    return SaveSafariRide();
}
extern int SaveSpaceTower(void);
static int ll_cb_save_SaveSpaceTower(void* ll_elem)
{
    (void)ll_elem;
    return SaveSpaceTower();
}
extern int SaveSpiderRide(void);
static int ll_cb_save_SaveSpiderRide(void* ll_elem)
{
    (void)ll_elem;
    return SaveSpiderRide();
}
extern int SaveSpinningBarrels(void);
static int ll_cb_save_SaveSpinningBarrels(void* ll_elem)
{
    (void)ll_elem;
    return SaveSpinningBarrels();
}
extern int SaveWaterBlock(void);
static int ll_cb_save_SaveWaterBlock(void* ll_elem)
{
    (void)ll_elem;
    return SaveWaterBlock();
}
#endif
// FUNCTION: LEGOLAND 0x004078f0
void GoldRush_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("GOLD RUSH", elem->name) == 0) {
        def->cb_create   = GoldRush_Create;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = GoldRush_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_GoldRush_Destroy;   /* PORT-M7 */
#endif
        def->cb_8c       = GoldRush_Tick;
        def->cb_activate = GoldRush_Activate;
        def->cb_interact = GoldRush_Interact;
        def->cb_add      = GoldRush_Add;
        def->cb_remove   = GoldRush_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_load     = LoadGoldWash;
#else
        def->cb_load     = ll_cb_load_LoadGoldWash;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_save     = SaveGoldWash;
#else
        def->cb_save     = ll_cb_save_SaveGoldWash;   /* PORT-M3 */
#endif
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
#ifndef LEGOLAND_PORTABLE
extern void Catapult_Add(void);         /* 0x00403970 */
#else   /* PORT-M7: catapult.c void Catapult_Place(void* obj, Pos* pos) */
extern void Catapult_Add(void* obj, void* pos);   /* 0x00403970 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void Catapult_Draw(void);        /* 0x004039e0 */
#else   /* PORT-M7: catapult.c RideDrawDesc* Catapult_GetDrawDesc(RideElem* elem, unsigned short tile) */
extern void* Catapult_Draw(void* elem, unsigned short tile);   /* 0x004039e0 */
#endif
extern int  SaveCatapult(void);         /* 0x00403a20 */
extern int  LoadCatapult(void);         /* 0x00403af0 */

// FUNCTION: LEGOLAND 0x00403bb0
void Catapult_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("CATAPULT", elem->name) == 0) {
        def->cb_create   = Catapult_Create;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = Catapult_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_Catapult_Destroy;   /* PORT-M7 */
#endif
        def->cb_8c       = Catapult_Tick;
        def->cb_activate = Catapult_Activate;
        def->cb_interact = Catapult_Interact;
        def->cb_remove   = Catapult_Remove;
        def->cb_add      = Catapult_Add;
        def->cb_draw     = Catapult_Draw;
#ifndef LEGOLAND_PORTABLE
        def->cb_save     = SaveCatapult;
#else
        def->cb_save     = ll_cb_save_SaveCatapult;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_load     = LoadCatapult;
#else
        def->cb_load     = ll_cb_load_LoadCatapult;   /* PORT-M3 */
#endif
    }
}

/* ======================================================================
 * COPTERS -- 0x00405110
 * ====================================================================== */
extern void Copters_Create(void);      /* 0x00403d90 */
extern void Copters_Tick(void);        /* 0x00404450 */
#ifndef LEGOLAND_PORTABLE
extern void Copters_Add(void);         /* 0x00404600 */
#else   /* PORT-M7: mechrides.c void Copters_Place(void* obj, Pos* pos) */
extern void Copters_Add(void* obj, void* pos);   /* 0x00404600 */
#endif
extern void Copters_Remove(void);      /* 0x00404580 */
extern void Copters_Activate(void);    /* 0x00404be0 */
#ifndef LEGOLAND_PORTABLE
extern void Copters_Draw(void);        /* 0x00404490 */
#else   /* PORT-M7: mechrides.c RideDrawDesc* Copters_GetDrawDesc(RideElem* elem, unsigned short tile) */
extern void* Copters_Draw(void* elem, unsigned short tile);   /* 0x00404490 */
#endif
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
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = Copters_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_Copters_Destroy;   /* PORT-M7 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_save     = SaveCopters;
#else
        def->cb_save     = ll_cb_save_SaveCopters;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_load     = LoadCopters;
#else
        def->cb_load     = ll_cb_load_LoadCopters;   /* PORT-M3 */
#endif
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
#ifndef LEGOLAND_PORTABLE
extern void SafariRide_Add(void);         /* 0x00414fc0 */
#else   /* PORT-M7: mechrides.c void SafariRide_Place(void* obj, Pos* pos) */
extern void SafariRide_Add(void* obj, void* pos);   /* 0x00414fc0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void SafariRide_Draw(void);        /* 0x00414ff0 */
#else   /* PORT-M7: mechrides.c RideDrawDesc* SafariRide_GetDrawDesc(RideElem* elem, unsigned short arg) */
extern void* SafariRide_Draw(void* elem, unsigned short tile);   /* 0x00414ff0 */
#endif
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
#ifndef LEGOLAND_PORTABLE
        def->cb_save     = SaveSafariRide;
#else
        def->cb_save     = ll_cb_save_SaveSafariRide;   /* PORT-M3 */
#endif
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
#ifndef LEGOLAND_PORTABLE
extern void SpiderRide_Add(void);         /* 0x004160f0 */
#else   /* PORT-M7: mechrides.c void SpiderRide_Place(void* obj, Pos* pos) */
extern void SpiderRide_Add(void* obj, void* pos);   /* 0x004160f0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void SpiderRide_Draw(void);        /* 0x00416120 */
#else   /* PORT-M7: mechrides.c RideDrawDesc* SpiderRide_GetDrawDesc(RideElem* elem, unsigned short arg) */
extern void* SpiderRide_Draw(void* elem, unsigned short tile);   /* 0x00416120 */
#endif
extern int  SaveSpiderRide(void);         /* 0x00416880 */
#ifndef LEGOLAND_PORTABLE
extern int  LoadSpiderRide(void);         /* 0x004168f0 */
#else   /* PORT-M7: ridesave.c int LoadSpider(RideElem* elem) */
extern int  LoadSpiderRide(void* elem);   /* 0x004168f0 */
#endif

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
#ifndef LEGOLAND_PORTABLE
        def->cb_save     = SaveSpiderRide;
#else
        def->cb_save     = ll_cb_save_SaveSpiderRide;   /* PORT-M3 */
#endif
        def->cb_load     = LoadSpiderRide;
    }
}

/* ======================================================================
 * SPACE TOWER RIDE -- 0x0043b780
 * ====================================================================== */
extern void SpaceTower_Create(void);      /* 0x0043b2b0 */
extern void SpaceTower_Tick(void);        /* 0x0043b420 */
extern void SpaceTower_Activate(void);    /* 0x0043bac0 */
#ifndef LEGOLAND_PORTABLE
extern void SpaceTower_Draw(void);        /* 0x0043b4e0 */
#else   /* PORT-M7: mechrides.c RideDrawDesc* SpaceTower_GetDrawDesc(RideElem* elem, unsigned short tile) */
extern void* SpaceTower_Draw(void* elem, unsigned short tile);   /* 0x0043b4e0 */
#endif
extern void SpaceTower_Interact(void);    /* 0x0043af50 */
extern void SpaceTower_Remove(void);      /* 0x0043b460 */
#ifndef LEGOLAND_PORTABLE
extern void SpaceTower_Add(void);         /* 0x0043b4b0 */
#else   /* PORT-M7: mechrides.c void SpaceTower_Place(void* obj, Pos* pos) */
extern void SpaceTower_Add(void* obj, void* pos);   /* 0x0043b4b0 */
#endif
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
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = SpaceTower_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_SpaceTower_Destroy;   /* PORT-M7 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_save     = SaveSpaceTower;
#else
        def->cb_save     = ll_cb_save_SaveSpaceTower;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_load     = LoadSpaceTower;
#else
        def->cb_load     = ll_cb_load_LoadSpaceTower;   /* PORT-M3 */
#endif
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
#ifndef LEGOLAND_PORTABLE
extern void SpinningBarrels_Add(void);         /* 0x0043c540 */
#else   /* PORT-M7: mechrides.c void SpinningBarrels_Place(void* obj, Pos* pos) */
extern void SpinningBarrels_Add(void* obj, void* pos);   /* 0x0043c540 */
#endif
extern void SpinningBarrels_Destroy(void);     /* 0x0043c5b0 */
#ifndef LEGOLAND_PORTABLE
extern void SpinningBarrels_Draw(void);        /* 0x0043c570 */
#else   /* PORT-M7: mechrides.c RideDrawDesc* SpinningBarrels_GetDrawDesc(RideElem* elem, unsigned short arg) */
extern void* SpinningBarrels_Draw(void* elem, unsigned short tile);   /* 0x0043c570 */
#endif
extern int  SaveSpinningBarrels(void);         /* 0x0043c620 */
#ifndef LEGOLAND_PORTABLE
extern int  LoadSpinningBarrels(void);         /* 0x0043c690 */
#else   /* PORT-M7: ridesave.c int LoadSBarrel(RideElem* elem) */
extern int  LoadSpinningBarrels(void* elem);   /* 0x0043c690 */
#endif

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
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = SpinningBarrels_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_SpinningBarrels_Destroy;   /* PORT-M7 */
#endif
        def->cb_draw     = SpinningBarrels_Draw;
#ifndef LEGOLAND_PORTABLE
        def->cb_save     = SaveSpinningBarrels;
#else
        def->cb_save     = ll_cb_save_SaveSpinningBarrels;   /* PORT-M3 */
#endif
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
#ifndef LEGOLAND_PORTABLE
extern void PlaneRide_Add(void);         /* 0x0043dfe0 */
#else   /* PORT-M7: mechrides.c void PlaneRide_Place(void* obj, Pos* pos) */
extern void PlaneRide_Add(void* obj, void* pos);   /* 0x0043dfe0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void PlaneRide_Draw(void);        /* 0x0043e010 */
#else   /* PORT-M7: mechrides.c RideDrawDesc* PlaneRide_GetDrawDesc(RideElem* elem, unsigned short arg) */
extern void* PlaneRide_Draw(void* elem, unsigned short tile);   /* 0x0043e010 */
#endif
#ifndef LEGOLAND_PORTABLE
extern int  LoadPlaneRide(void);         /* 0x0043e110 */
#else   /* PORT-M7: ridesave.c int LoadZoomer(RideElem* elem) */
extern int  LoadPlaneRide(void* elem);   /* 0x0043e110 */
#endif
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
#ifndef LEGOLAND_PORTABLE
        def->cb_save     = SavePlaneRide;
#else
        def->cb_save     = ll_cb_save_SavePlaneRide;   /* PORT-M3 */
#endif
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
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy = Hedge_Destroy;
#else
        def->cb_destroy = ll_cb_ac_Hedge_Destroy;   /* PORT-M7 */
#endif
    } else if (strcmp(elem->name, "FLOWERS") == 0) {
        def->cb_create  = Flowers_Create;
        def->cb_8c      = Flowers_Tick;
        def->cb_add     = Flowers_Add;
        def->cb_draw    = Flowers_Draw;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy = Flowers_Destroy;
#else
        def->cb_destroy = ll_cb_ac_Flowers_Destroy;   /* PORT-M7 */
#endif
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
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = WWEntrance_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_WWEntrance_Destroy;   /* PORT-M7 */
#endif
        def->cb_add      = WWEntrance_Add;
        def->cb_remove   = WWEntrance_Remove;
    } else if (NameCompare("WATER WORKS WATER BLOCK", elem->name) == 0) {
        def->cb_create   = WaterBlock_Create;
        def->cb_add      = WaterBlock_Add;
        def->cb_remove   = WaterBlock_Remove;
        def->cb_activate = WaterBlock_Activate;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = WaterBlock_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_WaterBlock_Destroy;   /* PORT-M7 */
#endif
        def->cb_draw     = WaterBlock_Draw;
        def->cb_interact = WaterBlock_Interact;
        def->cb_90       = WaterBlock_Update;
#ifndef LEGOLAND_PORTABLE
        def->cb_save     = SaveWaterBlock;
#else
        def->cb_save     = ll_cb_save_SaveWaterBlock;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_load     = LoadWaterBlock;
#else
        def->cb_load     = ll_cb_load_LoadWaterBlock;   /* PORT-M3 */
#endif
    } else if (NameCompare("WATER WORKS SHOWER", elem->name) == 0) {
        def->cb_create   = Shower_Create;
        def->cb_add      = Shower_Add;
        def->cb_remove   = Shower_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = Shower_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_Shower_Destroy;   /* PORT-M7 */
#endif
        def->cb_draw     = Shower_Draw;
        def->cb_interact = Shower_Interact;
#ifndef LEGOLAND_PORTABLE
        def->cb_activate = Shower_Activate;
#else
        def->cb_activate = ll_cb_a8_Shower_Activate;   /* PORT-M7 */
#endif
        def->cb_90       = Shower_Update;
    } else if (NameCompare("WATER WORKS ELEPHANT FOUNTAIN", elem->name) == 0) {
        def->cb_create   = ElephantFountain_Create;
        def->cb_add      = ElephantFountain_Add;
        def->cb_remove   = ElephantFountain_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = ElephantFountain_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_ElephantFountain_Destroy;   /* PORT-M7 */
#endif
        def->cb_interact = ElephantFountain_Interact;
#ifndef LEGOLAND_PORTABLE
        def->cb_activate = ElephantFountain_Activate;
#else
        def->cb_activate = ll_cb_a8_ElephantFountain_Activate;   /* PORT-M7 */
#endif
        def->cb_90       = ElephantFountain_Update;
#ifndef LEGOLAND_PORTABLE
        def->cb_save     = SaveElephantFountain;
#else
        def->cb_save     = ll_cb_save_SaveElephantFountain;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_load     = LoadElephantFountain;
#else
        def->cb_load     = ll_cb_load_LoadElephantFountain;   /* PORT-M3 */
#endif
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
#ifndef LEGOLAND_PORTABLE
extern void Shop_Draw(void);           /* 0x0043a390  shared by all nine */
#else   /* PORT-M7: westtown.c ShopDrawDesc* Shop_GetDrawDesc(ShopElem* elem, unsigned short arg) */
extern void* Shop_Draw(void* elem, unsigned short tile);   /* 0x0043a390 */
#endif
extern void Shop_Remove(void);         /* 0x0043a3d0  shared by six */

#ifndef LEGOLAND_PORTABLE
extern void GeneralStore_Create(void);     /* 0x004375d0 */
#else   /* PORT-M7: westtown.c void GeneralStore_LoadResources(ShopElem* elem) */
extern void GeneralStore_Create(void* elem);   /* 0x004375d0 */
#endif
extern void GeneralStore_Destroy(void);    /* 0x00437610 */
extern void GeneralStore_Tick(void);       /* 0x00437630 */
#ifndef LEGOLAND_PORTABLE
extern void GeneralStore_Activate(void);   /* 0x004378e0 */
#else   /* PORT-M7: westtown2.c void GeneralStore_TickCustomers(ShopElem* elem) */
extern void GeneralStore_Activate(void* elem);   /* 0x004378e0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void GeneralStore_Interact(void);   /* 0x00437670 */
#else   /* PORT-M7: westtown.c void GeneralStore_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq, void* clip, int mode) */
extern void GeneralStore_Interact(void* elem, int x, int y, void* sq, void* clip, int mode);   /* 0x00437670 */
#endif

#ifndef LEGOLAND_PORTABLE
extern void Sheriff_Create(void);          /* 0x00437ba0 */
#else   /* PORT-M7: westtown.c void Sheriff_LoadResources(ShopElem* elem) */
extern void Sheriff_Create(void* elem);   /* 0x00437ba0 */
#endif
extern void Sheriff_Destroy(void);         /* 0x00437bd0 */
extern void Sheriff_Tick(void);            /* 0x00437bf0 */
#ifndef LEGOLAND_PORTABLE
extern void Sheriff_Activate(void);        /* 0x00437c90 */
#else   /* PORT-M7: westtown2.c void Sheriff_TickCustomers(ShopElem* elem) */
extern void Sheriff_Activate(void* elem);   /* 0x00437c90 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void Sheriff_Interact(void);        /* 0x00437c30 */
#else   /* PORT-M7: westtown.c void Sheriff_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq, void* clip, int mode) */
extern void Sheriff_Interact(void* elem, int x, int y, void* sq, void* clip, int mode);   /* 0x00437c30 */
#endif

#ifndef LEGOLAND_PORTABLE
extern void JailCell_Create(void);         /* 0x00438070 */
#else   /* PORT-M7: westtown.c void JailCell_LoadResources(ShopElem* elem) */
extern void JailCell_Create(void* elem);   /* 0x00438070 */
#endif
extern void JailCell_Destroy(void);        /* 0x004380f0 */
extern void JailCell_Tick(void);           /* 0x00438110 */
#ifndef LEGOLAND_PORTABLE
extern void JailCell_Activate(void);       /* 0x00438430 */
#else   /* PORT-M7: westtown2.c void JailCell_TickCustomers(ShopElem* elem) */
extern void JailCell_Activate(void* elem);   /* 0x00438430 */
#endif
extern void JailCell_Add(void);            /* 0x00437f60 */
extern void JailCell_Remove(void);         /* 0x00438020 */
#ifndef LEGOLAND_PORTABLE
extern void JailCell_Interact(void);       /* 0x00438150 */
#else   /* PORT-M7: westtown2.c void JailCell_DrawOverlay(ShopElem* elem, int x, int y, MapSquare* sq, void* clip, int mode) */
extern void JailCell_Interact(void* elem, int x, int y, void* sq, void* clip, int mode);   /* 0x00438150 */
#endif
extern int  LoadJailCells(void);           /* 0x004387f0 (ridesave.c) */
extern int  SaveJailCells(void);           /* 0x00438780 (ridesave.c) */

#ifndef LEGOLAND_PORTABLE
extern void Bank_Create(void);             /* 0x00438870 */
#else   /* PORT-M7: westtown.c void Bank_LoadResources(ShopElem* elem) */
extern void Bank_Create(void* elem);   /* 0x00438870 */
#endif
extern void Bank_Destroy(void);            /* 0x004388a0 */
extern void Bank_Tick(void);               /* 0x004388c0 */
#ifndef LEGOLAND_PORTABLE
extern void Bank_Activate(void);           /* 0x00438960 */
#else   /* PORT-M7: westtown2.c void Bank_TickCustomers(ShopElem* elem) */
extern void Bank_Activate(void* elem);   /* 0x00438960 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void Bank_Interact(void);           /* 0x00438900 */
#else   /* PORT-M7: westtown.c void Bank_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq, void* clip, int mode) */
extern void Bank_Interact(void* elem, int x, int y, void* sq, void* clip, int mode);   /* 0x00438900 */
#endif

#ifndef LEGOLAND_PORTABLE
extern void Saloon_Create(void);           /* 0x00438c60 */
#else   /* PORT-M7: westtown.c void Saloon_LoadResources(ShopElem* elem) */
extern void Saloon_Create(void* elem);   /* 0x00438c60 */
#endif
extern void Saloon_Destroy(void);          /* 0x00438ca0 */
extern void Saloon_Tick(void);             /* 0x00438cc0 */
#ifndef LEGOLAND_PORTABLE
extern void Saloon_Activate(void);         /* 0x00438f10 */
#else   /* PORT-M7: westtown2.c void Saloon_TickCustomers(ShopElem* elem) */
extern void Saloon_Activate(void* elem);   /* 0x00438f10 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void Saloon_Interact(void);         /* 0x00438d00 */
#else   /* PORT-M7: westtown.c void Saloon_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq, void* clip, int mode) */
extern void Saloon_Interact(void* elem, int x, int y, void* sq, void* clip, int mode);   /* 0x00438d00 */
#endif

#ifndef LEGOLAND_PORTABLE
extern void Institute_Create(void);        /* 0x0043a0f0 */
#else   /* PORT-M7: westtown.c void Explorers_LoadResources(ShopElem* elem) */
extern void Institute_Create(void* elem);   /* 0x0043a0f0 */
#endif
extern void Institute_Destroy(void);       /* 0x0043a120 */
extern void Institute_Tick(void);          /* 0x0043a140 */
#ifndef LEGOLAND_PORTABLE
extern void Institute_Activate(void);      /* 0x0043a1e0 */
#else   /* PORT-M7: westtown.c void Explorers_TickCustomers(ShopElem* elem) */
extern void Institute_Activate(void* elem);   /* 0x0043a1e0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void Institute_Interact(void);      /* 0x0043a180 */
#else   /* PORT-M7: westtown.c void Explorers_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq, void* clip, int mode) */
extern void Institute_Interact(void* elem, int x, int y, void* sq, void* clip, int mode);   /* 0x0043a180 */
#endif

#ifndef LEGOLAND_PORTABLE
extern void LegoShop1_Create(void);        /* 0x00439200 */
#else   /* PORT-M7: westtown.c void LegoShop1_LoadResources(ShopElem* elem) */
extern void LegoShop1_Create(void* elem);   /* 0x00439200 */
#endif
extern void LegoShop1_Add(void);           /* 0x00439320 */
extern void LegoShop1_Remove(void);        /* 0x00439350 */
extern void LegoShop1_Destroy(void);       /* 0x004393e0 */
extern void LegoShop1_Tick(void);          /* 0x004393a0 */
#ifndef LEGOLAND_PORTABLE
extern void LegoShop1_Activate(void);      /* 0x00439460 */
#else   /* PORT-M7: westtown2.c void LegoShop1_TickCustomers(ShopElem* elem) */
extern void LegoShop1_Activate(void* elem);   /* 0x00439460 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void LegoShop1_Interact(void);      /* 0x00439400 */
#else   /* PORT-M7: westtown.c void LegoShop1_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq, void* clip, int mode) */
extern void LegoShop1_Interact(void* elem, int x, int y, void* sq, void* clip, int mode);   /* 0x00439400 */
#endif

#ifndef LEGOLAND_PORTABLE
extern void LegoShop2_Create(void);        /* 0x004396d0 */
#else   /* PORT-M7: westtown.c void LegoShop2_LoadResources(ShopElem* elem) */
extern void LegoShop2_Create(void* elem);   /* 0x004396d0 */
#endif
extern void LegoShop2_Destroy(void);       /* 0x00439700 */
extern void LegoShop2_Tick(void);          /* 0x00439720 */
#ifndef LEGOLAND_PORTABLE
extern void LegoShop2_Activate(void);      /* 0x00439950 */
#else   /* PORT-M7: westtown2.c void LegoShop2_TickCustomers(ShopElem* elem) */
extern void LegoShop2_Activate(void* elem);   /* 0x00439950 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void LegoShop2_Interact(void);      /* 0x00439760 */
#else   /* PORT-M7: westtown2.c void LegoShop2_DrawOverlay(ShopElem* elem, int x, int y, MapSquare* sq, void* clip, int mode) */
extern void LegoShop2_Interact(void* elem, int x, int y, void* sq, void* clip, int mode);   /* 0x00439760 */
#endif

#ifndef LEGOLAND_PORTABLE
extern void MediaShop_Create(void);        /* 0x00439c20 */
#else   /* PORT-M7: westtown.c void LegoMedia_LoadResources(ShopElem* elem) */
extern void MediaShop_Create(void* elem);   /* 0x00439c20 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void MediaShop_Add(void);           /* 0x00439c60 */
#else   /* PORT-M7: westtown.c void LegoMedia_Add(ShopElem* elem, Pos* at) */
extern void MediaShop_Add(void* elem, void* at);   /* 0x00439c60 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void MediaShop_Remove(void);        /* 0x00439c90 */
#else   /* PORT-M7: westtown.c void LegoMedia_Remove(void* obj, ShopTile tile, void* ctx) */
extern void MediaShop_Remove(void* obj, unsigned short tile, void* ctx);   /* 0x00439c90 */
#endif
extern void MediaShop_Destroy(void);       /* 0x00439ce0 */
extern void MediaShop_Tick(void);          /* 0x00439d00 */
#ifndef LEGOLAND_PORTABLE
extern void MediaShop_Activate(void);      /* 0x00439ef0 */
#else   /* PORT-M7: westtown2.c void LegoMedia_TickCustomers(ShopElem* elem) */
extern void MediaShop_Activate(void* elem);   /* 0x00439ef0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void MediaShop_Interact(void);      /* 0x00439d40 */
#else   /* PORT-M7: westtown.c void LegoMedia_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq, void* clip, int mode) */
extern void MediaShop_Interact(void* elem, int x, int y, void* sq, void* clip, int mode);   /* 0x00439d40 */
#endif

// FUNCTION: LEGOLAND 0x0043a400
void WesternTown_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("GENERAL STORE", elem->name) == 0) {
        def->cb_create   = GeneralStore_Create;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = GeneralStore_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_GeneralStore_Destroy;   /* PORT-M7 */
#endif
        def->cb_8c       = GeneralStore_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = GeneralStore_Activate;
        def->cb_remove   = Shop_Remove;
        def->cb_interact = GeneralStore_Interact;
    } else if (NameCompare("SHERIFF", elem->name) == 0) {
        def->cb_create   = Sheriff_Create;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = Sheriff_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_Sheriff_Destroy;   /* PORT-M7 */
#endif
        def->cb_8c       = Sheriff_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = Sheriff_Activate;
        def->cb_remove   = Shop_Remove;
        def->cb_interact = Sheriff_Interact;
    } else if (NameCompare("JAIL CELL", elem->name) == 0) {
        def->cb_create   = JailCell_Create;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = JailCell_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_JailCell_Destroy;   /* PORT-M7 */
#endif
        def->cb_8c       = JailCell_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = JailCell_Activate;
        def->cb_add      = JailCell_Add;
        def->cb_remove   = JailCell_Remove;
        def->cb_interact = JailCell_Interact;
#ifndef LEGOLAND_PORTABLE
        def->cb_load     = LoadJailCells;
#else
        def->cb_load     = ll_cb_load_LoadJailCells;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_save     = SaveJailCells;
#else
        def->cb_save     = ll_cb_save_SaveJailCells;   /* PORT-M3 */
#endif
    } else if (NameCompare("BANK", elem->name) == 0) {
        def->cb_create   = Bank_Create;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = Bank_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_Bank_Destroy;   /* PORT-M7 */
#endif
        def->cb_8c       = Bank_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = Bank_Activate;
        def->cb_remove   = Shop_Remove;
        def->cb_interact = Bank_Interact;
    } else if (NameCompare("SALOON", elem->name) == 0) {
        def->cb_create   = Saloon_Create;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = Saloon_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_Saloon_Destroy;   /* PORT-M7 */
#endif
        def->cb_8c       = Saloon_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = Saloon_Activate;
        def->cb_remove   = Shop_Remove;
        def->cb_interact = Saloon_Interact;
    } else if (NameCompare("EXPLORERS INSTITUTE", elem->name) == 0) {
        def->cb_create   = Institute_Create;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = Institute_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_Institute_Destroy;   /* PORT-M7 */
#endif
        def->cb_8c       = Institute_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = Institute_Activate;
        def->cb_remove   = Shop_Remove;
        def->cb_interact = Institute_Interact;
    } else if (NameCompare("LEGO SHOP 1", elem->name) == 0) {
        def->cb_create   = LegoShop1_Create;
        def->cb_add      = LegoShop1_Add;
        def->cb_remove   = LegoShop1_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = LegoShop1_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_LegoShop1_Destroy;   /* PORT-M7 */
#endif
        def->cb_8c       = LegoShop1_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = LegoShop1_Activate;
        def->cb_interact = LegoShop1_Interact;
    } else if (NameCompare("LEGO SHOP 2", elem->name) == 0) {
        def->cb_create   = LegoShop2_Create;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = LegoShop2_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_LegoShop2_Destroy;   /* PORT-M7 */
#endif
        def->cb_8c       = LegoShop2_Tick;
        def->cb_draw     = Shop_Draw;
        def->cb_activate = LegoShop2_Activate;
        def->cb_remove   = Shop_Remove;
        def->cb_interact = LegoShop2_Interact;
    } else if (NameCompare("LEGO MEDIA SHOP", elem->name) == 0) {
        def->cb_create   = MediaShop_Create;
        def->cb_add      = MediaShop_Add;
        def->cb_remove   = MediaShop_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = MediaShop_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_MediaShop_Destroy;   /* PORT-M7 */
#endif
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
#ifndef LEGOLAND_PORTABLE
extern void LFHoldUp_Remove(void);         /* 0x00410160 */
#else   /* PORT-M10: logflume.c:926 defines it (void*, void*, void*) -- the
         * cb_remove slot's own shape, which every other remove handler in this
         * table already carries. */
extern void LFHoldUp_Remove(void*, void*, void*);  /* 0x00410160 */
#endif
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
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = LFEntrance_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_LFEntrance_Destroy;   /* PORT-M7 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_save     = SaveLogFlume;
#else
        def->cb_save     = ll_cb_save_SaveLogFlume;   /* PORT-M3 */
#endif
#ifndef LEGOLAND_PORTABLE
        def->cb_load     = LoadLogFlume;
#else
        def->cb_load     = ll_cb_load_LoadLogFlume;   /* PORT-M3 */
#endif
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
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = LFTrack_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_LFTrack_Destroy;   /* PORT-M7 */
#endif
    } else if (NameCompare("LOG FLUME SPECIAL CORNER 1", elem->name) == 0) {
        def->cb_create   = LFCorner1_Create;
        def->cb_8c       = LFCorner1_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFCorner1_Interact;
        def->cb_90       = LFCorner1_Update;
        def->cb_94       = LFCorner1_Update2;
        def->cb_add      = LFCorner1_Add;
        def->cb_remove   = LFCorner1_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = LFCorner1_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_LFCorner1_Destroy;   /* PORT-M7 */
#endif
    } else if (NameCompare("LOG FLUME SPECIAL CORNER 2", elem->name) == 0) {
        def->cb_create   = LFCorner2_Create;
        def->cb_8c       = LFCorner2_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFCorner2_Interact;
        def->cb_90       = LFCorner2_Update;
        def->cb_94       = LFCorner2_Update2;
        def->cb_add      = LFCorner2_Add;
        def->cb_remove   = LFCorner2_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = LFCorner2_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_LFCorner2_Destroy;   /* PORT-M7 */
#endif
    } else if (NameCompare("LOG FLUME SPECIAL CORNER 3", elem->name) == 0) {
        def->cb_create   = LFCorner3_Create;
        def->cb_8c       = LFCorner3_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFCorner3_Interact;
        def->cb_90       = LFCorner3_Update;
        def->cb_94       = LFCorner3_Update2;
        def->cb_add      = LFCorner3_Add;
        def->cb_remove   = LFCorner3_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = LFCorner3_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_LFCorner3_Destroy;   /* PORT-M7 */
#endif
    } else if (NameCompare("LOG FLUME SPECIAL CORNER 4", elem->name) == 0) {
        def->cb_create   = LFCorner4_Create;
        def->cb_8c       = LFCorner4_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFCorner4_Interact;
        def->cb_90       = LFCorner4_Update;
        def->cb_94       = LFCorner4_Update2;
        def->cb_add      = LFCorner4_Add;
        def->cb_remove   = LFCorner4_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = LFCorner4_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_LFCorner4_Destroy;   /* PORT-M7 */
#endif
    } else if (NameCompare("LOG FLUME CSAW", elem->name) == 0) {
        def->cb_create   = LFCsaw_Create;
        def->cb_8c       = LFCsaw_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFCsaw_Interact;
        def->cb_90       = LFCsaw_Update;
        def->cb_94       = LFCsaw_Update2;
        def->cb_add      = LFCsaw_Add;
        def->cb_remove   = LFCsaw_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = LFCsaw_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_LFCsaw_Destroy;   /* PORT-M7 */
#endif
    } else if (NameCompare("LOG FLUME TUNNEL", elem->name) == 0) {
        def->cb_create   = LFTunnel_Create;
        def->cb_8c       = LFTunnel_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFTunnel_Interact;
        def->cb_90       = LFTunnel_Update;
        def->cb_94       = LFTunnel_Update2;
        def->cb_add      = LFTunnel_Add;
        def->cb_remove   = LFTunnel_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = LFTunnel_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_LFTunnel_Destroy;   /* PORT-M7 */
#endif
    } else if (NameCompare("LOG FLUME DROP", elem->name) == 0) {
        def->cb_create   = LFDrop_Create;
        def->cb_8c       = LFDrop_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFDrop_Interact;
        def->cb_90       = LFDrop_Update;
        def->cb_94       = LFDrop_Update2;
        def->cb_add      = LFDrop_Add;
        def->cb_remove   = LFDrop_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = LFDrop_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_LFDrop_Destroy;   /* PORT-M7 */
#endif
    } else if (NameCompare("LOG FLUME HOLD UP", elem->name) == 0) {
        def->cb_create   = LFHoldUp_Create;
        def->cb_8c       = LFHoldUp_Tick;
        def->cb_draw     = LFPiece_Draw;
        def->cb_interact = LFHoldUp_Interact;
        def->cb_90       = LFHoldUp_Update;
        def->cb_94       = LFHoldUp_Update2;
        def->cb_add      = LFHoldUp_Add;
        def->cb_remove   = LFHoldUp_Remove;
#ifndef LEGOLAND_PORTABLE
        def->cb_destroy  = LFHoldUp_Destroy;
#else
        def->cb_destroy  = ll_cb_ac_LFHoldUp_Destroy;   /* PORT-M7 */
#endif
    }
}
