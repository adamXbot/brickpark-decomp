/* LEGOLAND -- the remaining unnamed class callbacks from screen.c's
 * SetCustomCallbacks table (the second file; screencb.c holds the first
 * fifteen).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field offsets, global addresses and callee arg counts are load-bearing;
 * names are ours.
 *
 * ---------------------------------------------------------------------------
 * WHAT THESE ARE
 * ---------------------------------------------------------------------------
 * screen.c's SetCustomCallbacks (0x00452c20) runs once per class element as
 * the object database is loaded, matches the element's class NAME and stores
 * a run of function pointers into the class's 0xd0-byte ObjDef.  The slots
 * (the numbering docs/RIDE_CALLBACKS.md uses) are:
 *
 *     +0x8c tick/select   +0x90 update      +0x94 draw-selection
 *     +0x98 add           +0x9c remove      +0xa0 draw-descriptor
 *     +0xa4 create        +0xa8 activate    +0xac destroy
 *     +0xb0 interact      +0xb8 load        +0xbc save        +0xc0 extra
 *
 * Every function here is identified by the class whose slot holds it, read
 * straight out of SetCustomCallbacks:
 *
 *   0x00405370  DRIVING SCHOOL             +0xa4  DrivingSchool_Create
 *   0x00405460  DRIVING SCHOOL             +0xac  DrivingSchool_Destroy
 *   0x00405630  DRIVING SCHOOL             +0x98  DrivingSchool_Add
 *   0x00405740  DRIVING SCHOOL             +0x90  DrivingSchool_Update
 *   0x00405b10  DRIVING SCHOOL             +0xb0  DrivingSchool_Draw
 *   0x00411cd0  DRIVING SCHOOL PUMPS       +0x90  Pump_Update
 *   0x00414880  ZEBRA CROSSING             +0x90  ZebraCrossing_Update
 *   0x00419ef0  BOATING SCHOOL             +0xac  BoatingSchool_Destroy
 *   0x0041a3d0  BOATING SCHOOL             +0x94  BoatingSchool_DrawSelection
 *   0x0042a7b0  BALLOONZ                   +0xa4  Balloonz_Create
 *   0x0042baf0  BALLOONZ                   +0xb8  Balloonz_Load
 *   0x0042c280  CAROUSEL                   +0xa4  Carousel_Create
 *   0x0042c600  CAROUSEL                   +0xb8  Carousel_Load
 *   0x0042d2f0  EARTH SLIDE RIDE           +0xbc  EarthSlide_Save
 *   0x0042e910  CASTLE BBQ                 +0xb0  CastleBbq_Draw
 *   0x00434740  JUNGLE CRUISE MONKEY FISH  +0xa0  JcMonkeyFish_GetDrawDesc
 *   0x00434e50  JUNGLE CRUISE              +0xac  JungleCruise_Destroy
 *
 * ---------------------------------------------------------------------------
 * THE FOUR SHAPES IN THIS FILE
 * ---------------------------------------------------------------------------
 * 1. +0xb8 LOAD (Carousel, Balloonz) -- one shared source compiled twice.
 * 2. +0xac DESTROY (Driving School, Boating School, Jungle Cruise) -- release
 *    the class's LLIDB elements, free every per-placement record, kill the
 *    sprites and (driving school only) clear the cached ObjDef.
 * 3. +0xa4 CREATE (Driving School, Balloonz, Carousel) -- the four standard
 *    create steps then the class's own resources.
 * 4. +0x90 UPDATE (Driving School, Pumps, Zebra Crossing) -- the placement
 *    preview: turn the mouse into a map reference, shape the ghost cursors,
 *    validate them and grade the square.
 *
 * The remaining five (+0x94 draw-selection, +0x98 add, +0xa0 draw
 * descriptor, +0xb0 draw, +0xbc save) are one of a kind here and carry their
 * own notes.
 *
 * ---------------------------------------------------------------------------
 * HOW A SAVED RIDER IS PUT BACK ON A RIDE  (the +0xb8 shape, recovered here)
 * ---------------------------------------------------------------------------
 * Both load handlers do the same two things: read the chunk's null-terminated
 * chain of per-placement records back into the class's list, and then FIX UP
 * every rider of the class, because two pointers inside a rider cannot be
 * saved:
 *
 *   - Person3D +0x2c, the depth sprite the ride lends the rider's 3D model,
 *     is restored from Person3D +0x30 -- the "a ride is driving this model"
 *     flag doubles as a ONE-BASED index into the class's z-sprite table, so
 *     `zsprite = (&g_zspr)[driven - 1]`.  A model that was not being driven
 *     gets both fields cleared instead.
 *   - Bloke +0x54, the BNV path being played back, carries the bundle it was
 *     taken from as an index at +0x04, and its +0x00 bundle pointer is
 *     re-resolved through the class's .bnv table.
 *
 * So the save format stores no pointers at all: a rider's 3D state is one
 * flag plus one small-integer bundle index, and the class's own load handler
 * turns them back into the two pointers.  Note the SECOND fixup is NOT
 * guarded by the first -- a bloke with a live BNV path but no z-sprite is
 * fixed up as well.
 * ------------------------------------------------------------------------- */

/* ---- geometry and the ODF class record (same 0xd0-byte ObjDef as screen.c) */
typedef struct BPos  { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; unsigned char c[2]; } BPosW;
typedef struct Pos   { int x; int y; } Pos;

/* The class geometry rect: the footprint and its sub-rects are chained
 * through `next`. */
typedef struct Rect {
    int          left;              /* +0x00 */
    int          top;               /* +0x04 */
    int          right;             /* +0x08 */
    int          bottom;            /* +0x0c */
    struct Rect* next;              /* +0x10 */
} Rect;                             /* 0x14 */

typedef struct Spr {
    unsigned char pad00[0x10];
    unsigned int  flags;            /* +0x10  0x2000 = draw via the +0xb0 slot */
} Spr;

typedef struct RideDef {
    unsigned char pad00[0x0c];
    int           base_x;           /* +0x0c the class's base map square */
    int           base_y;           /* +0x10 */
    int           dx;               /* +0x14 the class's draw offset */
    int           dy;               /* +0x18 */
    unsigned int  flags;            /* +0x1c  0x20 = tick via +0xa8,
                                     *        0x400 = ask +0xa0 for the desc */
    unsigned char pad20[0x3c - 0x20];
    Rect          footprint;        /* +0x3c the class footprint rect */
    unsigned char pad50[0x64 - 0x50];
    Spr*          sprite;           /* +0x64 build sprite */
    unsigned char pad68[0xc4 - 0x68];
    void*         elem;             /* +0xc4 the LLIDB element this came from */
    unsigned char padc8[0xcc - 0xc8];
    struct RiderNode* riders;       /* +0xcc the class-wide customer list */
} RideDef;

typedef struct RideElem {
    char*         name;             /* +0x00 */
    char*         image;            /* +0x04 */
    unsigned int  type_flags;       /* +0x08 */
    RideDef*      data;             /* +0x0c */
    int           refcount;         /* +0x10  (the LLIDB element is 20 bytes) */
} RideElem;

/* A packed map square as two bytes, the form the render walk passes. */
typedef struct MapSquare { unsigned char bx; unsigned char by; } MapSquare;

/* An {x,y} pair returned in eax:edx. */
typedef struct Offset { int ox; int oy; } Offset;

/* ---- the people ---------------------------------------------------------- */

/* The BNV path a ride hands a rider's model: it remembers which bundle it
 * was cut from, so a saved path can be re-pointed at load time. */
typedef struct BnvPath {
    void*         bundle;           /* +0x00 the .bnv bundle it plays */
    int           bundle_idx;       /* +0x04 that bundle's index in the class */
} BnvPath;

/* The rider's 3D model. */
typedef struct Person3D {
    unsigned char pad00[0x2c];
    void*         zsprite;          /* +0x2c the ride's depth sprite */
    int           driven;           /* +0x30 1 while a ride drives the model */
} Person3D;

typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;           /* +0x0e low-level AI state (0 = idle) */
    unsigned char  pad10[0x54 - 0x10];
    BnvPath*       bnvpath;         /* +0x54 the BNV path being played back */
    unsigned char  pad58[0x60 - 0x58];
    unsigned char  action;          /* +0x60 state-machine stage */
} Bloke;

typedef struct RiderNode {
    struct RiderNode* next;         /* +0x00 */
    struct RiderNode* prev;         /* +0x04 */
    Bloke*            bloke;        /* +0x08 */
    unsigned short    ride_id;      /* +0x0c packed {x,y} of the instance */
    unsigned short    pad0e;
    Person3D*         person;       /* +0x10 */
} RiderNode;

/* ---- the save stream ---------------------------------------------------- */
extern int   SaveGameRead(void* buf, unsigned int n);         /* 0x0047d730 */
extern int   SaveGameWrite(const void* buf, unsigned int n);  /* 0x0047d760 */
extern void* HeapAlloc_w(unsigned int size);                  /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                             /* 0x0049e4d0 */


/* =========================================================================
 * 0x0042c600 -- CAROUSEL +0xb8   (load the class's save chunk)
 *
 * The chunk is a null-terminated chain: an int "another record follows", then
 * the 0x2c-byte CarouselRec verbatim, repeated.  Every record is appended to
 * the tail of the live list, and a short read at any of the three points
 * abandons the load with 0 through the ONE merged `return 0` block that the
 * leading guard pulls to the top of the function.
 *
 * Note the allocation is unconditional and the link is decided afterwards --
 * the opposite of EarthSlide_Load's shape (screencb.c), which allocates
 * inside the if.  Then the rider fix-up described in the file header runs.
 * ========================================================================= */

typedef struct CarouselRec {
    struct CarouselRec* next;       /* +0x00 */
    unsigned short      square;     /* +0x04 packed {x,y} */
    unsigned char       pad06[0x2c - 6];
} CarouselRec;                      /* 0x2c */

extern CarouselRec* g_carousel_recs;      /* 0x006160c4 the per-placement list */
/* The class z-sprite table Person3D +0x30 indexes ONE-BASED (element [0] is
 * g_carousel_zspr at 0x006160c0). */
extern void*        g_carousel_zspr[];    /* 0x006160c0 */
/* The class .bnv bundle table BnvPath +0x04 indexes: On / plain / Off. */
extern void*        g_carousel_bnv[];     /* 0x00616090 */

// FUNCTION: LEGOLAND 0x0042c600
int Carousel_Load(RideElem* elem)
{
    RideDef*     def = elem->data;
    CarouselRec* prev = 0;
    CarouselRec* cur;
    RiderNode*   r;
    int          more;

    if (SaveGameRead(&more, 4) == 0)
        return 0;
    while (more != 0) {
        cur = (CarouselRec*)HeapAlloc_w(0x2c);
        if (SaveGameRead(cur, 0x2c) == 0)
            return 0;
        cur->next = 0;
        if (prev != 0)
            prev->next = cur;
        else
            g_carousel_recs = cur;
        prev = cur;
        if (SaveGameRead(&more, 4) == 0)
            return 0;
    }
    for (r = def->riders; r != 0; r = r->next) {
        if (r->person->driven != 0) {
            r->person->zsprite = g_carousel_zspr[r->person->driven - 1];
        } else {
            r->person->zsprite = 0;
            r->person->driven = 0;
        }
        if (r->bloke->bnvpath != 0)
            r->bloke->bnvpath->bundle =
                g_carousel_bnv[r->bloke->bnvpath->bundle_idx];
    }
    return 1;
}


/* =========================================================================
 * 0x0042baf0 -- BALLOONZ +0xb8   (load the class's save chunk)
 *
 * Carousel_Load's source again, instruction for instruction: only the record
 * size (0x20 against 0x2c), the list head and the two class tables differ.
 * ========================================================================= */

typedef struct BalloonzRec {
    struct BalloonzRec* next;       /* +0x00 */
    unsigned short      square;     /* +0x04 packed {x,y} */
    unsigned char       pad06[0x20 - 6];
} BalloonzRec;                      /* 0x20 */

extern BalloonzRec* g_bz_recs;            /* 0x00616060 the per-placement list */
extern void*        g_bz_ridezspr[];      /* 0x00616040 */
extern void*        g_bz_bnv[];           /* 0x00616018 */

// FUNCTION: LEGOLAND 0x0042baf0
int Balloonz_Load(RideElem* elem)
{
    RideDef*     def = elem->data;
    BalloonzRec* prev = 0;
    BalloonzRec* cur;
    RiderNode*   r;
    int          more;

    if (SaveGameRead(&more, 4) == 0)
        return 0;
    while (more != 0) {
        cur = (BalloonzRec*)HeapAlloc_w(0x20);
        if (SaveGameRead(cur, 0x20) == 0)
            return 0;
        cur->next = 0;
        if (prev != 0)
            prev->next = cur;
        else
            g_bz_recs = cur;
        prev = cur;
        if (SaveGameRead(&more, 4) == 0)
            return 0;
    }
    for (r = def->riders; r != 0; r = r->next) {
        if (r->person->driven != 0) {
            r->person->zsprite = g_bz_ridezspr[r->person->driven - 1];
        } else {
            r->person->zsprite = 0;
            r->person->driven = 0;
        }
        if (r->bloke->bnvpath != 0)
            r->bloke->bnvpath->bundle =
                g_bz_bnv[r->bloke->bnvpath->bundle_idx];
    }
    return 1;
}


/* =========================================================================
 * THE +0xac DESTROY SHAPE
 *
 * A destroy handler is the exact inverse of the class's create handler and
 * runs when the object database is torn down.  All three here do the same
 * four things in the same order:
 *
 *   1. release every LLIDB element the create handler resolved by NAME --
 *      and note the idiom: LLIDB_FindElement returns 0 when the element IS
 *      resident, so the unload is on the FALSE arm, exactly as the create
 *      handler's load is;
 *   2. free every per-placement record on each of the class's lists, one
 *      list at a time, re-reading the head global each iteration so the
 *      global is always consistent if a callee walks it;
 *   3. kill the sprites and free the palettes;
 *   4. (driving school only) clear the cached ObjDef pointer.
 *
 * The boating school and the jungle cruise also STOP every boat sprite in
 * their image list first, with the same byte-narrowed subscript their create
 * handlers use to start them (`(unsigned char)i` against a full int counter,
 * so a list longer than 256 entries would wrap) -- reproduced.
 *
 * CODEGEN NOTE, and it is worth carrying to the next lane: the sprite the
 * loop stops must be a NAMED LOCAL, not the subscript nested in the call --
 * `spr = ilf->sprites[(unsigned char)i]; LLSStop(GetLLSForSprite(spr));`
 * rather than `LLSStop(GetLLSForSprite(ilf->sprites[...]))`.  The two forms
 * emit the same instructions INSIDE the loop, but the extra IR temporary
 * advances VC6's eax->ecx->edx scratch rotation by one, and the whole block
 * AFTER the loop then starts on the right register: without it both bodies
 * are one byte short and 12 / 6 mismatches, all of them the same rotation
 * offset carried to the end of the function.  Twelve other loop spellings
 * (do/while, `while`, `i & 0xff`, an unsigned counter, a hoisted `sprites`
 * pointer, an inline helper, separate element locals) and free `volatile`
 * reads on the list head, the count and the sprite globals are all inert.
 * ========================================================================= */

extern void  Kill_FXList(void* list, int count);              /* 0x00496e30 */
extern void  KillSprite(void* sprite);                        /* 0x00497bd0 */
extern int   LLIDB_FindElement(const char* name, void** out,
                               unsigned int* idx);            /* 0x0047b330 */
extern void  LLIDB_UnLoadData(void* elem);                    /* 0x0047d450 */
extern void* GetLLSForSprite(void* sprite);                   /* 0x00441e80 */
extern void  LLSStop(void* lls);                              /* 0x0047d4c0 */

/* The 0x08-byte-header image list: `count` entries of `sprites`. */
typedef struct ImageList {
    unsigned char pad00[4];
    int           count;            /* +0x04 */
    void**        sprites;          /* +0x08 */
} ImageList;

typedef struct FXEntry {
    const char* name;               /* +0x00 .wav name */
    void*       sample;             /* +0x04 filled in by Load_FXList */
    int         flags;              /* +0x08 */
} FXEntry;


/* -------------------------------------------------------------------------
 * 0x00405460 -- DRIVING SCHOOL +0xac
 *
 * Four lists come down: the cars (each retired through its own helper, which
 * unlinks it), the petrol pumps (0x00411bd0 walks and frees the whole pump
 * list itself), the road blocks and the school records.  Then the six-entry
 * FX list, the matte sprite, the three car liveries -- which are plain heap
 * blocks, not sprites, so they are freed rather than killed -- and the car
 * sprite.  The cached ObjDef is finally nulled, the only one of the three
 * destroy handlers that bothers.
 * ------------------------------------------------------------------------- */

typedef struct DsSchool {
    BPosW            key;           /* +0x00 its map square */
    int              take;          /* +0x04 accumulated takings */
    struct DsSchool* next;          /* +0x08 */
} DsSchool;                         /* 0x0c */

typedef struct RoadTile {
    struct RoadTile* next;          /* +0x00 */
    unsigned char    pad04[4];
    unsigned short   school;        /* +0x08 the owning school's map square */
    unsigned char    pad0a[2];
    int              x;             /* +0x0c map square */
    int              y;             /* +0x10 */
    unsigned char    type;          /* +0x14 low nibble kind, 0x10 = crossing */
} RoadTile;

typedef struct SchoolCar {
    struct SchoolCar* next;         /* +0x00 */
    unsigned short    school;       /* +0x04 */
} SchoolCar;

extern FXEntry    g_ds_fx[6];                                 /* 0x004b43f8 */
extern RideDef*   g_ds_def;                                   /* 0x0082c694 */
extern void*      g_road_tsm;                                 /* 0x0082c67c */
extern ImageList* g_car_images;                               /* 0x00830f9c */
extern void*      g_ds_matte;                                 /* 0x0082c6c0 */
extern void*      g_car_pal_c;                                /* 0x0082c6bc livery 1 */
extern void*      g_car_pal_b;                                /* 0x0082c6b8 livery 2 */
extern void*      g_car_pal_a;                                /* 0x0082c690 livery 3 */
extern void*      g_car_sprite;                               /* 0x00830f94 */
extern SchoolCar* g_school_cars;                              /* 0x004c10d4 */
extern RoadTile*  g_road_tiles;                               /* 0x004cbeac */
extern DsSchool*  g_ds_schools;                               /* 0x004c11bc */

extern void RetireSchoolCar(SchoolCar* car);                  /* 0x00401c60 */
extern void Pump_FreeAll(void);                               /* 0x00411bd0 */

// FUNCTION: LEGOLAND 0x00405460
void DrivingSchool_Destroy(RideElem* elem)
{
    void*     e;
    RoadTile* rt;
    DsSchool* s;

    if (LLIDB_FindElement("DSCHOOL MAPPING", &e, 0) == 0)
        LLIDB_UnLoadData(e);
    if (LLIDB_FindElement("DSCHOOL BLUE CAR", &e, 0) == 0)
        LLIDB_UnLoadData(e);
    while (g_school_cars != 0)
        RetireSchoolCar(g_school_cars);
    Pump_FreeAll();
    while (g_road_tiles != 0) {
        rt = g_road_tiles->next;
        HeapFree_w(g_road_tiles);
        g_road_tiles = rt;
    }
    while (g_ds_schools != 0) {
        s = g_ds_schools->next;
        HeapFree_w(g_ds_schools);
        g_ds_schools = s;
    }
    Kill_FXList(g_ds_fx, 6);
    KillSprite(g_ds_matte);
    HeapFree_w(g_car_pal_c);
    HeapFree_w(g_car_pal_b);
    HeapFree_w(g_car_pal_a);
    KillSprite(g_car_sprite);
    g_ds_def = 0;
}


/* -------------------------------------------------------------------------
 * 0x00419ef0 -- BOATING SCHOOL +0xac
 *
 * The mirror of BoatingSchool_Create (screencb.c): the two-entry FX list
 * ("Boat Noise.wav", "Waving Mermaid.wav"), every boat sprite stopped, the
 * tile-mapping and boat-list LLIDB elements released, the stations, boats
 * and lake squares freed, and the hull and rail masks killed.  The class's
 * cached ObjDef is deliberately NOT cleared.
 * ------------------------------------------------------------------------- */

typedef struct BsStation {
    BPosW             pos;          /* +0x00 the building's map square */
    BPosW             a;            /* +0x02 route START square (near jetty) */
    BPosW             b;            /* +0x04 route END square   (far jetty) */
    unsigned char     pad06[0x2c - 6];
    struct BsStation* next;         /* +0x2c */
} BsStation;                        /* 0x34 */

typedef struct BsWater {
    BPosW            pos;           /* +0x00 */
    BPosW            owner;         /* +0x02 the school that owns this cell */
    unsigned char    pad04[0x10 - 4];
    struct BsWater*  next;          /* +0x10 */
    unsigned char    pad14[0x1c - 0x14];
} BsWater;                          /* 0x1c */

typedef struct BsBoat { unsigned char pad00[0x3f4]; } BsBoat;

extern FXEntry    g_bs_fx[2];                                 /* 0x004b52c0 */
extern ImageList* g_bs_boat_ilf;                              /* 0x0082c65c */
extern BsStation* g_bs_stations;                              /* 0x004cc074 */
extern BsBoat*    g_bs_boats;                                 /* 0x004cc03c */
extern BsWater*   g_bs_water;                                 /* 0x004d823c */
extern void*      g_bs_hullmask;                              /* 0x0082adfc */
extern void*      g_bs_railm;                                 /* 0x0082c654 */

extern void BsBoat_Destroy(BsBoat* b);                        /* 0x00418f90 */

// FUNCTION: LEGOLAND 0x00419ef0
void BoatingSchool_Destroy(RideElem* elem)
{
    void*      e;
    int        i;
    BsStation* st;
    BsWater*   w;
    void*      spr;

    Kill_FXList(g_bs_fx, 2);
    for (i = 0; i < g_bs_boat_ilf->count; i++) {
        spr = g_bs_boat_ilf->sprites[(unsigned char)i];
        LLSStop(GetLLSForSprite(spr));
    }
    if (LLIDB_FindElement("BOATING SCHOOL TILE MAPPING", &e, 0) == 0)
        LLIDB_UnLoadData(e);
    if (LLIDB_FindElement("BOATING SCHOOL BOATS", &e, 0) == 0)
        LLIDB_UnLoadData(e);
    while (g_bs_stations != 0) {
        st = g_bs_stations->next;
        HeapFree_w(g_bs_stations);
        g_bs_stations = st;
    }
    while (g_bs_boats != 0)
        BsBoat_Destroy(g_bs_boats);
    while (g_bs_water != 0) {
        w = g_bs_water->next;
        HeapFree_w(g_bs_water);
        g_bs_water = w;
    }
    KillSprite(g_bs_hullmask);
    KillSprite(g_bs_railm);
}


/* -------------------------------------------------------------------------
 * 0x00434e50 -- JUNGLE CRUISE +0xac
 *
 * BoatingSchool_Destroy's source with the FX list dropped and the ObjDef
 * cached FIRST (this handler is the only one of the three that re-publishes
 * the class pointer on the way down).  Note the tile-mapping element it
 * releases is the BOATING SCHOOL's -- the river is drawn with the school's
 * tileset, as JungleCruise_Create records -- so tearing the jungle cruise
 * down unloads a descriptor the boating school may still be using.  That is
 * the original's behaviour and it is reproduced.
 * ------------------------------------------------------------------------- */

typedef struct JcStation {
    BPosW             pos;          /* +0x00 the station's own map square */
    BPosW             a;            /* +0x02 route START square */
    BPosW             b;            /* +0x04 route END square */
    unsigned char     pad06[0x3c - 6];
    struct JcStation* next;         /* +0x3c */
} JcStation;                        /* 0x44 */

typedef struct JcWater {
    BPosW            pos;           /* +0x00 */
    BPosW            owner;         /* +0x02 the river this cell belongs to */
    unsigned char    pad04[0x10 - 4];
    struct JcWater*  next;          /* +0x10 */
    unsigned char    pad14[0x1c - 0x14];
} JcWater;                          /* 0x1c */

typedef struct JcBoat { unsigned char pad00[0x3f8]; } JcBoat;

extern RideDef*   g_jc_def;                                   /* 0x0081cb60 */
extern ImageList* g_jc_boat_ilf;                              /* 0x0081cd00 */
extern JcStation* g_jc_stations;                              /* 0x00629c3c */
extern JcBoat*    g_jc_boats;                                 /* 0x00616164 */
extern JcWater*   g_jc_water;                                 /* 0x0062fd2c */
extern void*      g_jc_mask;                                  /* 0x0081cb5c */

extern void JcBoat_Unlink(JcBoat* b);                         /* 0x00432cb0 */

// FUNCTION: LEGOLAND 0x00434e50
void JungleCruise_Destroy(RideElem* elem)
{
    void*      e;
    int        i;
    JcStation* st;
    JcWater*   w;
    void*      spr;

    g_jc_def = elem->data;
    if (LLIDB_FindElement("BOATING SCHOOL TILE MAPPING", &e, 0) == 0)
        LLIDB_UnLoadData(e);
    for (i = 0; i < g_jc_boat_ilf->count; i++) {
        spr = g_jc_boat_ilf->sprites[(unsigned char)i];
        LLSStop(GetLLSForSprite(spr));
    }
    if (LLIDB_FindElement("JUNGLE CRUISE BOATS", &e, 0) == 0)
        LLIDB_UnLoadData(e);
    while (g_jc_stations != 0) {
        st = g_jc_stations->next;
        HeapFree_w(g_jc_stations);
        g_jc_stations = st;
    }
    while (g_jc_boats != 0)
        JcBoat_Unlink(g_jc_boats);
    while (g_jc_water != 0) {
        w = g_jc_water->next;
        HeapFree_w(g_jc_water);
        g_jc_water = w;
    }
    KillSprite(g_jc_mask);
}


/* =========================================================================
 * THE +0xa4 CREATE SHAPE
 *
 * A create handler is called by the ODF loader immediately after the callback
 * table is installed.  The standard opening (screencb.c documents it) is
 *
 *     def = elem->data;                 cache the ObjDef in a module global
 *     def->flags |= 0x20 | 0x400;       0x20 arms +0xa8, 0x400 arms +0xa0
 *     spr = def->sprite;                the class's build sprite (ObjDef+0x64)
 *     spr->flags |= 0x2000;             arm the custom draw in +0xb0
 *
 * after which the class loads its own resources and parks any animated
 * layers of the build sprite with the three-call HideLayer +
 * StopLayerPlaying + LLSSetFrame(GetLLSForLayer(...), 0) idiom.
 * ========================================================================= */

extern void* LoadSprite(const char* name, int mode);          /* 0x00497ab0 */
extern void* LoadPalette(const char* name);                   /* 0x00441f20 */
extern void* LoadBinV(const char* name);                      /* 0x0044dc90 */
extern void  Load_FXList(void* list, int count);              /* 0x00496dd0 */
extern void  HideLayer(Spr* sprite, int layer);               /* 0x00497de0 */
extern void  StopLayerPlaying(Spr* sprite, int layer);        /* 0x00441f00 */
extern void* GetLLSForLayer(Spr* sprite, int layer);          /* 0x00441ea0 */
extern void  LLSSetFrame(void* lls, int frame);               /* 0x0047d5a0 */
extern void* LLIDB_LoadData(void* elem);                      /* 0x0047d3a0 */


/* -------------------------------------------------------------------------
 * 0x00405370 -- DRIVING SCHOOL +0xa4
 *
 * The school resolves two LLIDB elements by name -- "DSCHOOL MAPPING", the
 * road tileset the road records index, and "DSCHOOL BLUE CAR", the car image
 * list -- then loads the matte, the THREE car liveries (blue = livery 1,
 * yellow = 2, red = 3, the order the car's heading byte selects them in) and
 * the car sprite itself, whose animation is stopped so a parked car shows a
 * still frame.
 *
 * The class arms 0x420, i.e. it overrides the draw descriptor as well.  Both
 * null tests on the tail are the original's: the sprite may fail to load and
 * the decoded bitmap may be absent, and neither is reported.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00405370
void DrivingSchool_Create(RideElem* elem)
{
    void* e;
    void* lls;

    g_ds_def = elem->data;
    if (LLIDB_FindElement("DSCHOOL MAPPING", &e, 0) == 0)
        g_road_tsm = LLIDB_LoadData(e);
    if (LLIDB_FindElement("DSCHOOL BLUE CAR", &e, 0) == 0)
        g_car_images = (ImageList*)LLIDB_LoadData(e);
    g_ds_def->flags |= 0x420;
    Load_FXList(g_ds_fx, 6);
    g_ds_matte = LoadSprite("DSchool Matte.lls", 1);
    g_car_pal_c = LoadPalette(".\\3ddata\\blu.col");
    g_car_pal_b = LoadPalette(".\\3ddata\\yel.col");
    g_car_pal_a = LoadPalette(".\\3ddata\\red.col");
    g_car_sprite = LoadSprite("ds_car&m.lls", 1);
    if (g_car_sprite != 0) {
        lls = GetLLSForSprite(g_car_sprite);
        if (lls != 0)
            LLSStop(lls);
    }
}


/* -------------------------------------------------------------------------
 * 0x0042a7b0 -- BALLOONZ +0xa4
 *
 * Three base mattes, three car mattes (red, green, blue), the ride's depth
 * sprite and the wheel's .bnv bundle, then layers 2 and 1 of the build
 * sprite parked.  Both the z-sprite and the bundle are stored TWICE: once
 * under their own name and once as element [0] of the class tables the load
 * handler indexes (see the file header).
 *
 * ORIGINAL BUG, reproduced: the 0x2000 "draw through the +0xb0 slot" flag is
 * OR'd into the ObjDef's own flags word (+0x1c) instead of the build
 * sprite's (+0x10) -- `mov ecx,[eax+1ch] / or ch,20h / mov [eax+1ch],ecx`
 * with eax still holding the ObjDef.  Every sibling create handler in the
 * game ORs it into the sprite it has just cached, and the statement here sits
 * exactly where that one does, so the wrong object was named.  The ObjDef
 * therefore ends up with 0x2420 and the build sprite is never armed.
 * ------------------------------------------------------------------------- */

extern RideDef* g_bz_def;                                     /* 0x0081cde4 */
extern Spr*     g_bz_layers;                                  /* 0x00616044 */
extern void*    g_bz_base_m1;                                 /* 0x00616048 */
extern void*    g_bz_base_m2;                                 /* 0x0061604c */
extern void*    g_bz_base_m3;                                 /* 0x00616050 */
extern void*    g_bz_car_red;                                 /* 0x00616054 */
extern void*    g_bz_car_green;                               /* 0x00616058 */
extern void*    g_bz_car_blue;                                /* 0x0061605c */
extern void*    g_bz_zspr;                                    /* 0x0081cde8 */
extern void*    g_bz_bnv0;                                    /* 0x00616010 */

// FUNCTION: LEGOLAND 0x0042a7b0
void Balloonz_Create(RideElem* elem)
{
    RideDef* def;

    def = elem->data;
    g_bz_def = def;
    def->flags |= 0x420;
    g_bz_layers = g_bz_def->sprite;
    /* ORIGINAL BUG: the build sprite was meant here, not the ObjDef. */
    g_bz_def->flags |= 0x2000;
    g_bz_base_m1 = LoadSprite("Ballbasem1.lls", 1);
    g_bz_base_m2 = LoadSprite("Ballbasem2.lls", 1);
    g_bz_base_m3 = LoadSprite("Ballbasem3.lls", 1);
    g_bz_car_red = LoadSprite("BZRedCarM1.lls", 1);
    g_bz_car_green = LoadSprite("BZGreenCarM1.lls", 1);
    g_bz_car_blue = LoadSprite("BZBlueCarM1.lls", 1);
    g_bz_zspr = LoadSprite("z_Balloon2.lls", 1);
    g_bz_ridezspr[0] = g_bz_zspr;
    g_bz_bnv0 = LoadBinV("Zbuffers\\balloonz.bnv");
    g_bz_bnv[0] = g_bz_bnv0;
    HideLayer(g_bz_layers, 2);
    StopLayerPlaying(g_bz_layers, 2);
    LLSSetFrame(GetLLSForLayer(g_bz_layers, 2), 0);
    HideLayer(g_bz_layers, 1);
    StopLayerPlaying(g_bz_layers, 1);
    LLSSetFrame(GetLLSForLayer(g_bz_layers, 1), 0);
}


/* -------------------------------------------------------------------------
 * 0x0042c280 -- CAROUSEL +0xa4
 *
 * The only create handler in the file that asks the build sprite where one of
 * its layers is drawn: GetLayer fills a layer record and the ride keeps that
 * layer's offset, biased by (-0x58, -0xcd), as the point a rider's 3D model
 * is anchored to (ridecb3.c's g_carousel_dx/dy, subtracted halved when a
 * rider mounts).  The third argument to GetLayer is a constant 0 whose push
 * VC6 hoists to the top of the function, which is why the merged
 * `add esp,40h` covers it.
 *
 * Resources: the ride's depth sprite, three .bnv bundles (running / plain /
 * stopped -- the load handler's table, in that order) and two entrance
 * mattes.  Every one of the four "table" resources is ALSO stored under its
 * own scalar name; the scalar is written first.
 *
 * Layers 2 and 0 are parked with the full three-call idiom; layer 1 is only
 * hidden, never stopped or rewound -- the same asymmetry Restaurant2_Create
 * has on its tower layer, reproduced as written.
 *
 * The ObjDef global is re-read from memory at every one of its four uses,
 * including for GetLayer's sprite argument where the sprite had just been
 * cached in g_carousel_layers.  Written through a local it costs the match.
 * ------------------------------------------------------------------------- */

/* What GetLayer fills in: the layer's sprite and its draw offset.  The
 * caller's record is 0x18 bytes although only the first three fields are
 * ever written. */
typedef struct LayerInfo {
    void*         sprite;           /* +0x00 */
    int           dx;               /* +0x04 */
    int           dy;               /* +0x08 */
    unsigned char pad0c[0x18 - 0x0c];
} LayerInfo;

extern void GetLayer(Spr* sprite, LayerInfo* out, int layer); /* 0x00497e80 */

extern FXEntry  g_carousel_fx[2];                             /* 0x004b64d8 */
extern RideDef* g_carousel_item;                              /* 0x006160bc */
extern Spr*     g_carousel_layers;                            /* 0x00616068 */
extern int      g_carousel_dx;                                /* 0x00616078 */
extern int      g_carousel_dy;                                /* 0x0061607c */
extern void*    g_carousel_zspr_s;                            /* 0x006160b8 */
extern void*    g_carousel_on_s;                              /* 0x00616080 */
extern void*    g_carousel_run_s;                             /* 0x0061608c */
extern void*    g_carousel_off_s;                             /* 0x00616084 */
extern void*    g_carousel_matte;                             /* 0x0061606c */
extern void*    g_carousel_matte2;                            /* 0x00616070 */

// FUNCTION: LEGOLAND 0x0042c280
void Carousel_Create(RideElem* elem)
{
    LayerInfo li;

    g_carousel_item = elem->data;
    Load_FXList(g_carousel_fx, 2);
    g_carousel_item->flags |= 0x420;
    g_carousel_layers = g_carousel_item->sprite;
    g_carousel_layers->flags |= 0x2000;
    GetLayer(g_carousel_item->sprite, &li, 0);
    g_carousel_dx = li.dx - 0x58;
    g_carousel_dy = li.dy - 0xcd;
    g_carousel_zspr_s = LoadSprite("z_Carousel.lls", 1);
    g_carousel_zspr[0] = g_carousel_zspr_s;
    g_carousel_on_s = LoadBinV("Zbuffers\\CarouselOn.bnv");
    g_carousel_bnv[0] = g_carousel_on_s;
    g_carousel_run_s = LoadBinV("Zbuffers\\Carousel.bnv");
    g_carousel_bnv[1] = g_carousel_run_s;
    g_carousel_off_s = LoadBinV("Zbuffers\\CarouselOff.bnv");
    g_carousel_bnv[2] = g_carousel_off_s;
    g_carousel_matte = LoadSprite("Carousel Entrance Matte.lls", 1);
    g_carousel_matte2 = LoadSprite("Carousel Entrance Matte2.lls", 1);
    HideLayer(g_carousel_layers, 2);
    StopLayerPlaying(g_carousel_layers, 2);
    LLSSetFrame(GetLLSForLayer(g_carousel_layers, 2), 0);
    HideLayer(g_carousel_layers, 0);
    StopLayerPlaying(g_carousel_layers, 0);
    LLSSetFrame(GetLLSForLayer(g_carousel_layers, 0), 0);
    HideLayer(g_carousel_layers, 1);
}


/* =========================================================================
 * 0x0042d2f0 -- EARTH SLIDE RIDE +0xbc   (write the class's save chunk)
 *
 * The exact producer of what EarthSlide_Load (screencb.c) reads back: for
 * every placed slide, the int 1 ("another record follows"), the 0x24-byte
 * SlideRec verbatim, the queue LENGTH, and then that many rider INDICES --
 * each queue node's RiderNode is written as its position in the class's
 * rider list, which is what makes the chunk pointer-free.  A terminating 0
 * closes the chain and its write is the function's result.
 *
 * The queue is walked TWICE per record, once to count and once to write; the
 * count is kept in an address-taken local so the increment is a
 * read-modify-write of the frame slot the length is later written from.
 * Every failed write abandons the chunk through one shared `return 0`.
 * ========================================================================= */

typedef struct SlideNode {
    struct SlideNode* next;         /* +0x00 */
    void*             rider;        /* +0x04 the RiderNode this seat holds */
} SlideNode;                        /* 0x08 */

typedef struct SlideRec {
    unsigned char     pad00[0x0c];
    struct SlideRec*  next;         /* +0x0c */
    unsigned char     pad10[0x1c - 0x10];
    SlideNode*        queue;        /* +0x1c head of the queue chain */
    SlideNode*        tail;         /* +0x20 its last node, used while loading */
} SlideRec;                         /* 0x24 */

extern SlideRec* g_slide_head;                                /* 0x006160e8 */
extern RideDef*  g_slide_item;                                /* 0x006160d0 */

/* The inverse of screencb.c's NthRiderNode: how far down the list a node is. */
extern int NthRiderNodeIndex(void* list, void* node);         /* 0x0042d3e0 */

// FUNCTION: LEGOLAND 0x0042d2f0
int EarthSlide_Save(void)
{
    SlideRec*  rec;
    SlideNode* q;
    int        one = 1;
    int        zero = 0;
    int        n;
    int        idx;

    rec = g_slide_head;
    while (rec != 0) {
        if (SaveGameWrite(&one, 4) == 0)
            return 0;
        if (SaveGameWrite(rec, 0x24) == 0)
            return 0;
        n = 0;
        for (q = rec->queue; q != 0; q = q->next)
            n++;
        if (SaveGameWrite(&n, 4) == 0)
            return 0;
        for (q = rec->queue; q != 0; q = q->next) {
            idx = NthRiderNodeIndex(g_slide_item->riders, q->rider);
            if (SaveGameWrite(&idx, 4) == 0)
                return 0;
        }
        rec = rec->next;
    }
    return SaveGameWrite(&zero, 4) != 0;
}


/* =========================================================================
 * 0x0042e910 -- CASTLE BBQ +0xb0   (the class's custom draw)
 *
 * The +0xb0 slot is what the build sprite's 0x2000 flag arms: the render
 * walk hands the class the whole square.  The BBQ's is the minimal form of
 * BoatingSchool_Draw (screencb.c) -- render every customer standing on THIS
 * square in 3D (no stage filter: the BBQ has no interior), then blit layer 1
 * of the build sprite over them at the layer's own view-adjusted offset.
 *
 * GetSpriteForLayer is called TWICE with the same arguments, once to test
 * for the layer and once as PrintSprite's argument; that duplicate is the
 * original's and is what the merged `add esp,1ch` covers.  The `x`/`y`
 * parameters the render walk passes are ignored.
 * ========================================================================= */

extern Offset GetScreenCoordsForObject(MapSquare* sq, RideDef* def); /* 0x00442cc0 */
extern Offset GetRenderOffsetForLayer(Spr* sprite, int layer);/* 0x00441ee0 */
extern void   AdjustOffsetForViewMode(Offset* o);             /* 0x00442d30 */
extern void*  GetSpriteForLayer(Spr* sprite, int layer);      /* 0x00441ec0 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern void   IP_RenderBlokeIn3DNow(Bloke* b);                /* 0x00440010 */

extern Spr* g_castlebbq_layers;                               /* 0x0081cd10 */

// FUNCTION: LEGOLAND 0x0042e910
void CastleBbq_Draw(RideElem* elem, int x, int y, MapSquare* sq,
                    void* clip, int mode)
{
    RideDef*   def = elem->data;
    RiderNode* r = def->riders;
    Offset     screen;
    Offset     off;

    while (r != 0) {
        if (*(unsigned short*)sq == r->ride_id)
            IP_RenderBlokeIn3DNow(r->bloke);
        r = r->next;
    }
    screen = GetScreenCoordsForObject(sq, def);
    off = GetRenderOffsetForLayer(g_castlebbq_layers, 1);
    AdjustOffsetForViewMode(&off);
    if (GetSpriteForLayer(g_castlebbq_layers, 1) != 0)
        PrintSprite(GetSpriteForLayer(g_castlebbq_layers, 1),
                    screen.ox + off.ox, screen.oy + off.oy, mode, 0);
}


/* =========================================================================
 * 0x00434740 -- JUNGLE CRUISE MONKEY FISH +0xa0  (the draw descriptor)
 *
 * The +0xa0 slot is asked for the block the render walk would otherwise build
 * itself (ObjDef +0x64/+0x14/+0x18 plus the caller's packed square); this one
 * substitutes a DIFFERENT sprite while the fish is jumping, so the leaping
 * frames are drawn from their own image instead of the class's.
 *
 * The state machine is one bit per fish (+0x04) driven off the class sprite's
 * OWN animation frame, so every fish on the map jumps in step with the class
 * animation but starts independently:
 *   - jumping and the class frame is back at 0  -> stop jumping;
 *   - not jumping and the class frame is 1      -> start with probability 1/7.
 * While jumping the jump sprite is forced to the class sprite's frame, which
 * is what keeps the two animations locked together.
 *
 * IT RETURNS THE SUBSYSTEM'S ONE SHARED BLOCK, g_shop_draw at 0x0082c6a0 --
 * the same static westtown.c's Shop_GetDrawDesc and Joust_GetDrawDesc fill,
 * so it is valid only until the next class asks for its descriptor.  Unlike
 * those two it does NOT re-arm the sprite's 0x2000 flag.
 *
 * ORIGINAL BUG, reproduced: the record search walks off the end of the list
 * and the result is dereferenced with no null test, so a monkey fish drawn
 * for a square that has no record reads +0x04 out of address 4.
 * ========================================================================= */

typedef struct LLS {
    short         frame;            /* +0x00  the frame being shown */
    unsigned char pad02[0x10 - 2];
    short         count;            /* +0x10 */
} LLS;

/* The block the +0xa0 slot returns (westtown.c's ShopDrawDesc). */
typedef struct DrawDesc {
    void*          sprite;          /* +0x00 */
    int            f04;             /* +0x04 */
    int            f08;             /* +0x08 */
    unsigned short f0c;             /* +0x0c */
} DrawDesc;

typedef struct JcMonkeyFish {
    BPosW                pos;       /* +0x00 */
    BPosW                owner;     /* +0x02 */
    int                  jumping;   /* +0x04 */
    struct JcMonkeyFish* next;      /* +0x08 */
} JcMonkeyFish;                     /* 0x0c */

extern JcMonkeyFish* g_jc_fish;                               /* 0x00629c30 */
extern void*         g_jc_fish_jump;                          /* 0x0081cb6c */
extern DrawDesc      g_shop_draw;                             /* 0x0082c6a0 */
extern int           rand(void);                              /* 0x0049e4b2 (CRT) */

// FUNCTION: LEGOLAND 0x00434740
DrawDesc* JcMonkeyFish_GetDrawDesc(RideElem* elem, unsigned short arg)
{
    RideDef*      def = elem->data;
    JcMonkeyFish* f = g_jc_fish;
    LLS*          lls;
    void*         jump_lls;

    while (f != 0) {
        if (f->pos.w == arg)
            break;
        f = f->next;
    }
    lls = (LLS*)GetLLSForSprite(def->sprite);
    if (f->jumping != 0) {
        if (lls->frame == 0)
            f->jumping = 0;
    } else if (lls->frame == 1) {
        if (rand() % 7 == 0)
            f->jumping = 1;
    }
    if (f->jumping != 0) {
        jump_lls = GetLLSForSprite(g_jc_fish_jump);
        LLSSetFrame(jump_lls, lls->frame);
        g_shop_draw.sprite = g_jc_fish_jump;
    } else {
        g_shop_draw.sprite = def->sprite;
    }
    g_shop_draw.f04 = def->dx;
    g_shop_draw.f08 = def->dy;
    g_shop_draw.f0c = arg;
    return &g_shop_draw;
}


/* =========================================================================
 * 0x00405b10 -- DRIVING SCHOOL +0xb0   (the class's custom draw)
 *
 * Render every customer standing on this square in 3D, then blit the
 * school's matte over them.  Two details are worth naming:
 *
 *  - THE STAGE FILTER IS A HOLE, not a range: a rider is drawn when its
 *    stage is <= 1 or >= 3, i.e. everybody except stage 2 -- the one spent
 *    inside the building, where the walls would have to occlude them.  The
 *    original spells it as two unsigned compares against 1 and 3 rather than
 *    one against 2, and that is what is reproduced.
 *  - THE MATTE IS PLACED FROM THE TILE BOUNDS, not from the object's screen
 *    position: the square's bounding box plus HALF the class's draw offset,
 *    halved with a signed division (neg/sar/neg on the negative arm), so an
 *    odd offset rounds toward zero.
 * ========================================================================= */

typedef struct TileBounds {
    int left;                       /* +0x00 */
    int top;                        /* +0x04 */
    int right;                      /* +0x08 */
    int bottom;                     /* +0x0c */
} TileBounds;

extern void GetTileBounds(Pos* tile, TileBounds* out);        /* 0x0045acc0 */

/* Halve IN PLACE, rounding toward zero -- the branchy neg/sar/neg form, not
 * VC6's cdq/sub/sar lowering of `/ 2`, so the original wrote the shift and
 * the sign case by hand.  It has to take a POINTER: the pair it halves then
 * gets a memory home, which is what makes the two sums below accumulate into
 * the freshly loaded tile-bounds register instead of into the halved value
 * (5 mismatches otherwise, and no by-value spelling or free `volatile` read
 * reaches it -- 20 measured). */
static __inline void HalfTowardZero(int* v)
{
    if (*v < 0)
        *v = -((-*v) >> 1);
    else
        *v = *v >> 1;
}

/* CODEGEN NOTE: the class's draw offset must be copied into ONE `Pos`
 * aggregate before it is halved.  As two plain `int` locals VC6 interleaves
 * each load with its own halving (16 mismatches) and swaps the ebx/edi
 * assignment of `sq` and `def` in the prologue; as an aggregate both loads
 * come out first and the parameter ranking flips to the original's. */
// FUNCTION: LEGOLAND 0x00405b10
void DrivingSchool_Draw(RideElem* elem, int x, int y, MapSquare* sq,
                        void* clip, int mode)
{
    RideDef*   def = elem->data;
    RiderNode* r = def->riders;
    Bloke*     b;
    Pos        p;
    TileBounds tb;
    Pos        d;

    p.x = sq->bx;
    p.y = sq->by;
    while (r != 0) {
        if (*(unsigned short*)sq == r->ride_id) {
            b = r->bloke;
            if (b->action <= 1 || b->action >= 3)
                IP_RenderBlokeIn3DNow(b);
        }
        r = r->next;
    }
    GetTileBounds(&p, &tb);
    if (g_ds_matte != 0) {
        d.x = def->dx;
        d.y = def->dy;
        HalfTowardZero(&d.x);
        HalfTowardZero(&d.y);
        PrintSprite(g_ds_matte, tb.left + d.x, tb.top + d.y, mode, 0);
    }
}


/* =========================================================================
 * THE +0x90 UPDATE SHAPE -- the placement preview
 *
 * The +0x90 slot runs every frame while the player is dragging the class out
 * of the build panel.  All three here share the opening:
 *
 *     g_edit_cursor.rect = def->footprint;   (a five-dword rep movsd)
 *     g_edit_cursor.next = 0;                (drop last frame's chain)
 *     ScreenToMapRef(screen, &g_edit_cursor.origin, mode);
 *
 * and then differ in how they shape and grade the result.  The GHOST CURSORS
 * are a linked chain hanging off g_edit_cursor +0x1830: the render walk
 * follows it and draws every cursor on it, so a class that needs to preview
 * more than one block builds the chain here and clears it again next frame.
 * ========================================================================= */

typedef struct Cursor {
    unsigned char  pad0000[0x1404];
    Pos            origin;          /* +0x1404 map square under the mouse */
    unsigned char  pad140c[0x1414 - 0x140c];
    Rect           rect;            /* +0x1414 the footprint being previewed */
    unsigned char  pad1428[0x1828 - 0x1428];
    unsigned int   flags;           /* +0x1828 */
    unsigned char  pad182c[4];
    struct Cursor* next;            /* +0x1830 chained preview cursors */
} Cursor;                           /* 0x1834 */

extern Cursor g_edit_cursor;                                  /* 0x007febc0 */
extern Cursor g_ds_prev_a;                                    /* 0x0082f760 */
extern Cursor g_ds_prev_b;                                    /* 0x0082c6e0 */
extern Cursor g_ds_prev_c;                                    /* 0x0082df20 */

extern void      ScreenToMapRef(int screen, Pos* out, int mode); /* 0x0045be90 */
extern void      ValidateCursor(Cursor* c, RideDef* def);     /* 0x0045f810 */
extern void      PropagateCursorStatus(Cursor* c);            /* 0x0045f4d0 */
extern void      ResetCursorFootprint(Cursor* c);             /* 0x0045f460 */
extern void      SetCursorError(Cursor* c, int code);         /* 0x0045f480 */
extern int       CursorIsValid(Cursor* c);                    /* 0x0045f4b0 */
extern int       GetObjCost(RideDef* def);                    /* 0x00480da0 */
extern int       GetBrickCount(void);                         /* 0x004578e0 */
extern RoadTile* GetRoadRecord(int x, int y);                 /* 0x004125f0 */
extern void*     Pump_FindAt(int x, int y);                   /* 0x00411aa0 */
/* Snap the cursor onto the road block one square east of it and return that
 * block; 0 when there is no plain road there. */
extern RoadTile* Pump_SnapToRoad(Cursor* c);                  /* 0x00411dc0 */

/* The 4x4 block one road record covers (0x004b4bf0). */
static const Rect kRoadBlockRect = { 0, 0, 3, 3, 0 };


/* -------------------------------------------------------------------------
 * 0x00411cd0 -- DRIVING SCHOOL PUMPS +0x90
 *
 * A petrol pump is placed against a road block, so the update snaps the
 * cursor onto the block east of the mouse first and only then grades it.
 * Three outcomes: too few bricks (error 2), a legal spot (chain the road
 * block's own 4x4 preview on so the player sees which block the pump will
 * attach to, with mode 0x2034), or no block / an invalid square (error 0xe
 * and an empty chain).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00411cd0
void Pump_Update(RideElem* elem, int screen, int mode)
{
    RideDef*  def = elem->data;
    RoadTile* rec;

    g_edit_cursor.rect = def->footprint;
    g_edit_cursor.next = 0;
    ScreenToMapRef(screen, &g_edit_cursor.origin, mode);
    rec = Pump_SnapToRoad(&g_edit_cursor);
    ValidateCursor(&g_edit_cursor, def);
    if (GetBrickCount() < GetObjCost(def)) {
        SetCursorError(&g_edit_cursor, 2);
        return;
    }
    if (rec != 0 && CursorIsValid(&g_edit_cursor)) {
        g_edit_cursor.next = &g_ds_prev_a;
        g_ds_prev_a.rect = kRoadBlockRect;
        g_ds_prev_a.next = 0;
        g_ds_prev_a.origin.x = rec->x;
        g_ds_prev_a.origin.y = rec->y;
        g_ds_prev_a.flags = 0x2034;
    } else {
        g_edit_cursor.next = 0;
        SetCursorError(&g_edit_cursor, 0xe);
    }
}


/* -------------------------------------------------------------------------
 * 0x00414880 -- ZEBRA CROSSING +0x90
 *
 * A crossing is painted ONTO an existing road block, so the update starts by
 * failing (error 0xe) and only clears the error when everything holds: the
 * player can afford it, there IS a road block under the mouse, that block
 * does not already carry a crossing (type bit 0x10), its kind is not one of
 * the four junction kinds 3..6, and it has no petrol pump attached at
 * (x - 1, y + 1).  The cursor is also SNAPPED to the block's own square, so
 * the preview jumps to the block rather than following the mouse exactly.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00414880
void ZebraCrossing_Update(RideElem* elem, int screen, int mode)
{
    RideDef*      def = elem->data;
    RoadTile*     rec;
    unsigned char t;

    g_edit_cursor.rect = def->footprint;
    g_edit_cursor.next = 0;
    ScreenToMapRef(screen, &g_edit_cursor.origin, mode);
    rec = GetRoadRecord(g_edit_cursor.origin.x, g_edit_cursor.origin.y);
    SetCursorError(&g_edit_cursor, 0xe);
    if (GetBrickCount() < GetObjCost(def))
        return;
    if (rec == 0)
        return;
    g_edit_cursor.origin.x = rec->x;
    g_edit_cursor.origin.y = rec->y;
    t = rec->type;
    if (t & 0x10)
        return;
    t &= 0xf;
    if (t == 3 || t == 4 || t == 5 || t == 6)
        return;
    if (Pump_FindAt(rec->x - 1, rec->y + 1) != 0)
        return;
    ResetCursorFootprint(&g_edit_cursor);
}


/* -------------------------------------------------------------------------
 * 0x00405740 -- DRIVING SCHOOL +0x90
 *
 * The school previews FOUR blocks at once: its own footprint on the edit
 * cursor plus three ghosts, each with its own fixed rect and mode, all
 * anchored at the class footprint's top-left corner offset from the mouse
 * square:
 *
 *   A (0x0082f760, mode 0x4108)  {0,-4, 8, 7}  the car park / road apron
 *   B (0x0082c6e0, mode 0x4208)  {-3,-4, 0,-1} the pump bay to its west
 *   C (0x0082df20, mode 0x5008)  {-1, 0, 4, 8} the track below it
 *
 * Every one is built with its `next` CLEARED, then all four are validated
 * INNERMOST FIRST (C, B, A, then the edit cursor), and only afterwards are
 * they chained together and the whole chain graded in one pass.  Building
 * the chain before validating would make each ValidateCursor walk the ones
 * behind it, so the order is load-bearing, not incidental.
 *
 * The three ghosts share their origin expression -- the class footprint's
 * top-left plus the map reference -- and the ObjDef global and both halves
 * of the map reference are each read ONCE and kept in registers across all
 * three blocks, which is what a plain repeated global read gives.
 * ------------------------------------------------------------------------- */

/* The three ghost rects (0x004b4440 / 0x004b4458 / 0x004b4470). */
static const Rect kDsPrevA = {  0, -4, 8,  7, 0 };
static const Rect kDsPrevB = { -3, -4, 0, -1, 0 };
static const Rect kDsPrevC = { -1,  0, 4,  8, 0 };

// FUNCTION: LEGOLAND 0x00405740
void DrivingSchool_Update(RideElem* elem, int screen, int mode)
{
    RideDef* def = elem->data;

    g_edit_cursor.rect = def->footprint;
    g_edit_cursor.next = 0;
    g_edit_cursor.flags = 0x4408;
    ScreenToMapRef(screen, &g_edit_cursor.origin, mode);

    g_ds_prev_a.rect = kDsPrevA;
    g_ds_prev_a.next = 0;
    g_ds_prev_a.flags = 0x4108;
    g_ds_prev_a.origin.x = g_ds_def->footprint.left + g_edit_cursor.origin.x;
    g_ds_prev_a.origin.y = g_ds_def->footprint.top + g_edit_cursor.origin.y;

    g_ds_prev_b.rect = kDsPrevB;
    g_ds_prev_b.next = 0;
    g_ds_prev_b.flags = 0x4208;
    g_ds_prev_b.origin.x = g_ds_def->footprint.left + g_edit_cursor.origin.x;
    g_ds_prev_b.origin.y = g_ds_def->footprint.top + g_edit_cursor.origin.y;

    g_ds_prev_c.rect = kDsPrevC;
    g_ds_prev_c.next = 0;
    g_ds_prev_c.flags = 0x5008;
    g_ds_prev_c.origin.x = g_ds_def->footprint.left + g_edit_cursor.origin.x;
    g_ds_prev_c.origin.y = g_ds_def->footprint.top + g_edit_cursor.origin.y;

    ValidateCursor(&g_ds_prev_c, def);
    ValidateCursor(&g_ds_prev_b, def);
    ValidateCursor(&g_ds_prev_a, def);
    ValidateCursor(&g_edit_cursor, def);
    g_edit_cursor.next = &g_ds_prev_a;
    g_ds_prev_a.next = &g_ds_prev_b;
    g_ds_prev_b.next = &g_ds_prev_c;
    PropagateCursorStatus(&g_edit_cursor);
}


/* =========================================================================
 * 0x00405630 -- DRIVING SCHOOL +0x98   (place one school on the map)
 *
 * Placing a driving school builds five ROAD BLOCKS as well as the building,
 * and it takes their coordinates from the FIRST GHOST CURSOR the update slot
 * left chained on the edit cursor (g_edit_cursor.next, i.e. g_ds_prev_a),
 * not from the `pos` argument -- so the track that appears is exactly the
 * one the player was shown while dragging.  Relative to that ghost's origin
 * the five blocks are
 *
 *     (x-3, y-4) shape 6 rot 1     the entry corner
 *     (x+1, y-4) shape 0 rot 1     the straight between them
 *     (x+5, y-4) shape 3 rot 1     the far corner
 *     (x+5, y  ) shape 0 rot 0     and two straights running back down
 *     (x+5, y+4) shape 0 rot 0
 *
 * The school's own record is keyed by the packed {x,y} built from the LOW
 * BYTES of the placement position, and that two-byte key lives in the (now
 * dead) `pos` argument slot -- it is written there as two byte stores and
 * read back as a word for the record and as the by-value argument of all six
 * later calls.
 *
 * Finally the "Driving School Roads" class is bumped FIVE times, once per
 * block, so the object counter matches what was really built -- but only if
 * that element exists and is flagged buildable.  The school starts with a
 * takings figure of 5.
 * ========================================================================= */

extern void  AddBasicObject(void* obj, Pos* pos);             /* 0x0045efe0 */
extern void  NewRoadRecord(BPosW school, int x, int y,
                           int shape, int rot);               /* 0x004132a0 */
extern RideElem* ElemID(const char* name);                    /* 0x0047b3f0 */
extern void  IncrementObjectCount(RideDef* def);              /* 0x00480d40 */
extern void  DrivingSchool_ResetPaths(BPosW school);          /* 0x00405310 */

// FUNCTION: LEGOLAND 0x00405630
void DrivingSchool_Add(void* obj, Pos* pos)
{
    DsSchool* rec;
    RideElem* e;
    BPosW     key;
    int       x;
    int       y;

    key.b.x = (unsigned char)pos->x;
    key.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    rec = (DsSchool*)HeapAlloc_w(0xc);
    rec->key.w = key.w;
    rec->next = g_ds_schools;
    g_ds_schools = rec;
    x = g_edit_cursor.next->origin.x;
    y = g_edit_cursor.next->origin.y;
    NewRoadRecord(key, x - 3, y - 4, 6, 1);
    NewRoadRecord(key, x + 1, y - 4, 0, 1);
    NewRoadRecord(key, x + 5, y - 4, 3, 1);
    NewRoadRecord(key, x + 5, y, 0, 0);
    NewRoadRecord(key, x + 5, y + 4, 0, 0);
    e = ElemID("Driving School Roads");
    if (e != 0 && (e->type_flags & 4) != 0) {
        IncrementObjectCount(e->data);
        IncrementObjectCount(e->data);
        IncrementObjectCount(e->data);
        IncrementObjectCount(e->data);
        IncrementObjectCount(e->data);
    }
    rec->take = 5;
    DrivingSchool_ResetPaths(key);
}


/* =========================================================================
 * 0x0041a3d0 -- BOATING SCHOOL +0x94   (the ride's selection painter)
 *
 * This is the RIDE-side handler screencb.c's BsWater_DrawSelection delegates
 * to once it works out that the mouse is on one of the school's jetties.
 * The +0x94 slot has TWO callers and this body serves both: the selection
 * walk, and misc3.c's PopUpCanDelete, which drives the same slot as the map
 * QUERY hook (`def->update2(def->ctx, &p)`).  That is why it does two
 * unrelated-looking things:
 *
 *  1. IT ANSWERS THE QUERY.  The spare cursor at 0x00830fc0 (logflume.c's
 *     g_lf_cursor_c -- one shared scratch cursor, not a log-flume object) is
 *     given the query block's own square and a ONE-CELL-WIDE VERTICAL STRIP
 *     immediately EAST of the queried footprint, mode 0x1008, and is chained
 *     onto the query cursor; the "an extra cursor is on the chain" flag at
 *     0x00810144 is set so the query caller knows to use it.  That strip is
 *     the lane the boats leave by, so demolishing a school also demands the
 *     column beside it be clear.
 *  2. IT PAINTS THE SELECTION.  Every lake square and every mermaid whose
 *     owner is the square under the edit cursor is stamped into the water
 *     class's scratch cursor (the same 0x0082ae20 the jungle cruise uses)
 *     with the fixed 5x5 water footprint and rendered -- so selecting a
 *     boating school lights up its whole lake and all its decorations in one
 *     pass, exactly as JungleCruise_DrawSelection (ridecb9.c) does for the
 *     river.
 *
 * Both list heads are read BEFORE BasicObjectDCalcCursor, so a head that
 * call changed would be missed -- the same eager-read shape the rest of the
 * family has.
 * ========================================================================= */

typedef struct BsMermaid {
    BPosW             key;          /* +0x00 its own map square */
    BPosW             owner;        /* +0x02 the school it belongs to */
    struct BsMermaid* next;         /* +0x04 */
} BsMermaid;                        /* 0x0c */

/* castleobj.c's query block: the cell the +0x94 hook is asked about. */
typedef struct QueryBlock {
    int  x;                         /* +0x00 */
    int  y;                         /* +0x04 */
    int  state;                     /* +0x08 */
    int  f0c;                       /* +0x0c */
    Rect footprint;                 /* +0x10 */
} QueryBlock;

extern QueryBlock g_query_block;                              /* 0x00811564 */
extern Cursor     g_query_cursor;                             /* 0x00810160 */
/* The one shared scratch cursor; logflume.c calls the same object
 * g_lf_cursor_c. */
extern Cursor     g_spare_cursor;                             /* 0x00830fc0 */
/* Set by a +0x94 handler that has chained an extra cursor onto the query
 * cursor; the map-query caller (0x00458096) tests it before falling back to
 * its own footprint, and clears the chain again afterwards. */
extern int        g_query_extra;                              /* 0x00810144 */
/* The water class's scratch placement cursor, shared with the jungle
 * cruise (ridecb9.c's g_jc_water_cursor). */
extern Cursor     g_bs_water_cursor;                          /* 0x0082ae20 */
extern BsMermaid* g_bs_mermaids;                              /* 0x004d2164 */
extern BPosW      g_sel_bpos;                                 /* 0x00667c54 */

extern void BasicObjectDCalcCursor(RideElem* elem, Pos* p);   /* 0x00480bb0 */
extern void DefaultCursor(Cursor* c);                         /* 0x0045a390 */
extern void BuildCursorPtr(Cursor* c, int a, int b);          /* 0x0045f5f0 */
extern void RenderCursor(Cursor* c);                          /* 0x0045ff00 */

/* The fixed 5x5 footprint one lake square always takes (0x004b53c0). */
static const Rect kBsWaterRect = { -2, -2, 2, 2, 0 };

// FUNCTION: LEGOLAND 0x0041a3d0
void BoatingSchool_DrawSelection(RideElem* elem, Pos* p)
{
    BsWater*   w = g_bs_water;
    BsMermaid* m = g_bs_mermaids;

    BasicObjectDCalcCursor(elem, p);
    g_spare_cursor.origin.x = g_query_block.x;
    g_spare_cursor.origin.y = g_query_block.y;
    g_spare_cursor.rect.left = g_query_block.footprint.right + 1;
    g_spare_cursor.rect.right = g_query_block.footprint.right + 1;
    g_spare_cursor.rect.top = g_query_block.footprint.top;
    g_spare_cursor.rect.bottom = g_query_block.footprint.bottom;
    g_spare_cursor.rect.next = 0;
    g_spare_cursor.flags = 0x1008;
    g_spare_cursor.next = 0;
    g_query_cursor.next = &g_spare_cursor;
    g_query_extra = 1;
    DefaultCursor(&g_bs_water_cursor);
    g_bs_water_cursor.rect = kBsWaterRect;

    while (w != 0) {
        if (w->owner.w == g_sel_bpos.w) {
            g_bs_water_cursor.origin.x = w->pos.b.x;
            g_bs_water_cursor.origin.y = w->pos.b.y;
            ResetCursorFootprint(&g_bs_water_cursor);
            g_bs_water_cursor.flags = 8;
            BuildCursorPtr(&g_bs_water_cursor, 0, 0);
            RenderCursor(&g_bs_water_cursor);
        }
        w = w->next;
    }

    while (m != 0) {
        if (m->owner.w == g_sel_bpos.w) {
            g_bs_water_cursor.origin.x = m->key.b.x;
            g_bs_water_cursor.origin.y = m->key.b.y;
            ResetCursorFootprint(&g_bs_water_cursor);
            g_bs_water_cursor.flags = 8;
            BuildCursorPtr(&g_bs_water_cursor, 0, 0);
            RenderCursor(&g_bs_water_cursor);
        }
        m = m->next;
    }
}
