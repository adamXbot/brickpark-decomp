/* LEGOLAND -- the CASTLE OBJ class and the roller-coaster track family.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; names
 * are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere).
 *
 * WHAT THIS MODULE IS
 * -------------------
 * `CastleObj_GetInterfaces` (0x004254d0) is the last of the eighteen
 * *_GetInterfaces providers `SetCustomCallbacks` (0x00452c20, screen.c) calls
 * at the end of every .ODF load, and it is by far the widest: it answers to
 * EIGHT class names, not one --
 *
 *     "CASTLE OBJ"                the castle itself
 *     "CASTLE_DUMMY"              the castle's invisible partner squares
 *     "SQUARE_TRACK"              flat roller-coaster track
 *     "SQUARE_TRACK_HEIGHT"       raised track (with support stick)
 *     "SQUARE_TRACK_HEIGHT_0"     raised track, zero-height variant
 *     "SQUARE_TRACK_HEIGHT_PATH"  raised track that carries a path under it
 *     "ROLLER_COASTER_LOAD"       the load-station piece
 *     "ROLLER_COASTER_SAVE"       the save-station piece
 *
 * so the castle and the whole roller-coaster builder are ONE translation unit
 * in the original. renderview.c's `RenderFullMap` already knew the five
 * SQUARE_TRACK.. / CASTLE_DUMMY names as "the ROLLER-COASTER path" of its chain
 * walk; this file is the other end of that.
 *
 * WHY THE CASTLE IS SPECIAL
 * -------------------------
 * Every other class in the game is reached only through the ODF class chain
 * (`ObjectClassList` @0x00669240) or the ride table. The CASTLE OBJ is
 * additionally resolved BY NAME straight into a global by the core:
 * `InitGameMap` (0x00459850, mapinit.c) does `ElemID("CASTLE OBJ")` and parks
 * the LLIDB element in `g_castle_obj_elem` @0x0080ff64. The core then tests
 * `obj == *(void**)0x0080ff64` in `BuildObject` and in `PutObjOnMap`
 * (popup.c's header records this) -- i.e. "is the thing being placed the
 * castle", which the game needs because the castle is the park's single
 * mandatory, one-per-map, pre-placed building. That is also why 0x0080ff64
 * doubles as `NewObjectPtr`-adjacent placing state in mapobj.c/objmap2.c: the
 * castle element is the sentinel the placement code compares against.
 *
 * THE PER-CLASS INTERFACE TABLE @0x0082ad20 (the local trick of this module)
 * -------------------------------------------------------------------------
 * The eight classes share ONE set of five ObjDef entry points. Rather than
 * install eight different callback sets, `CastleObj_GetInterfaces` installs
 * the same five GENERIC THUNKS (0x0041ec50..0x0041ed50) in ObjDef slots
 * +0x8c..+0x9c for every class, and puts the class's real implementations in
 * a six-entry dispatch table at 0x0082ad20:
 *
 *     typedef struct CtIface {          // 0x18 bytes
 *         RideElem* elem;   // +0x00  the class's LLIDB element (the key)
 *         void* tick;       // +0x04  <- reached through ObjDef +0x8c
 *         void* update;     // +0x08  <- ObjDef +0x90   (3 args)
 *         void* add;        // +0x0c  <- ObjDef +0x98   (2 args)
 *         void* update2;    // +0x10  <- ObjDef +0x94   (2 args)
 *         void* remove;     // +0x14  <- ObjDef +0x9c   (3 args)
 *     } CtIface;
 *
 * The thunks take the class element as their first argument, run
 * `CastleClassIndex` (0x0041ec20) to turn it into a table row (a linear scan
 * of the six `elem` keys; NOT FOUND FOLDS TO ROW 0, i.e. to the castle --
 * reproduced faithfully, it is how the original behaves), and tail into the
 * row's implementation. Six rows for eight names because
 * ROLLER_COASTER_LOAD/SAVE get no row at all: they only override ObjDef +0x8c
 * with the save/load routine directly (see below), so their thunk row would
 * never be consulted.
 *
 * Rows, in table order, with the class that claims each:
 *     [0] CASTLE OBJ                 [3] SQUARE_TRACK_HEIGHT
 *     [1] CASTLE_DUMMY               [4] SQUARE_TRACK_HEIGHT_0
 *     [2] SQUARE_TRACK               [5] SQUARE_TRACK_HEIGHT_PATH
 * CASTLE_DUMMY fills only its +0x10/+0x14 slots and never writes its own
 * `elem` key, so row 1 stays keyed on whatever is at 0x0082ad38 -- normally
 * NULL. `CastleClassIndex` therefore never returns 1, and every CASTLE_DUMMY
 * call falls through the scan to row 0 and runs the CASTLE OBJ handlers. That
 * looks like a genuine original bug (the store to 0x0082ad38 is simply
 * missing from the CASTLE_DUMMY arm) but it is harmless in practice because
 * the two rows the dummy does fill are the only ones its ObjDef ever reaches.
 * Reproduced as-is -- do not "fix" it.
 *
 * A second, differently ordered set of six element pointers sits directly
 * after the table at 0x0082adb0..0x0082adc4 (the table ends at 0x0082adb0,
 * which is the literal bound `CastleClassIndex` scans up to). Those are the
 * by-name caches the track editor uses to build a piece of a given class.
 *
 * ROLLER_COASTER_LOAD / ROLLER_COASTER_SAVE
 * -----------------------------------------
 * These two are not track pieces you build; they are the coaster's station
 * markers, and their arms are the shortest in the function: install the five
 * generic thunks, then immediately OVERWRITE ObjDef +0x8c (tick) with the
 * coaster load / save routine (0x00426c20 / 0x00426ce0) -- the same pair the
 * CASTLE OBJ arm installs in the +0xb8/+0xbc save-game slots. So the station
 * piece's per-frame tick IS the coaster (de)serialiser.
 *
 * WHAT IS IN THIS FILE
 * --------------------
 *   the class provider          CastleObj_GetInterfaces
 *   the generic dispatch layer  CastleClassIndex / CastleClassElem /
 *                               CastleClassRemoveByIndex / the five CtThunk_*
 *   CASTLE OBJ                  Castle_Tick / _Update / _Update2 / _Add /
 *                               _Remove / _Destroy / _Interact (its draw
 *                               pass) / _Activate (its rider tick) / _Extra,
 *                               plus GetCastleRec
 *   CASTLE_DUMMY                CastleDummy_Create / _Update2 / _Remove /
 *                               _Interact
 *   SQUARE_TRACK family         Track_Tick / _Update / _Update2 / _Add /
 *                               _Remove / _Destroy / _Interact,
 *                               TrackH_Create / _Add, TrackH0_Create,
 *                               TrackHP_Create / _Add, FindTrackDesc
 *   the coaster save format     SaveRollerCoaster / LoadRollerCoaster
 *   and, assigned to this lane but belonging to the DRIVING SCHOOL class,
 *                               DrivingSchool_TickRiders (0x00405bd0)
 *
 * VC6 CODEGEN NOTE -- three of these were NOT built with /O2
 * ----------------------------------------------------------
 * CastleObj_GetInterfaces (0x004254d0), Castle_Interact (0x00425050) and
 * Castle_Activate (0x004251c0) all carry unoptimised codegen while everything
 * around them -- the generic thunks 30 bytes away, every other handler in
 * this file -- is fully optimised. So the original build disabled
 * optimisation for those three functions specifically, and each carries a
 * `#pragma optimize("", off)` / `("", on)` pair. Note that the pragma inside
 * an /O2 compile is NOT the same as compiling the file /Od: it reproduces the
 * ebp frame, the per-value stack homes and the shared epilogue exactly, but
 * the frame SLOT ORDER still comes from the optimiser's colourer, not from
 * declaration order (see the residual note on Castle_Activate).
 *
 * The original CastleObj_GetInterfaces body pushes a frame pointer
 * (`push ebp / mov ebp,esp`), RELOADS both parameters from their stack homes
 * before every single store rotating eax->ecx->edx, and jumps every arm to
 * ONE shared `pop ebp / ret` instead of duplicating it. All three are
 * unoptimised codegen: nothing else in this address range looks like it (the
 * generic thunks 30 bytes away, and every other function in this file, are
 * fully optimised). So this one function came out of the original build with
 * optimisation disabled, and the exact, semantics-free way to say that in the
 * source is what the original almost certainly said:
 *
 *     #pragma optimize("", off)   ... the function ...   #pragma optimize("", on)
 *
 * With the pragma the function is 246/246 instructions, byte-exact; without
 * it /O2 caches both pointers in one register and tail-duplicates the
 * epilogue into all eight arms (94.5%). The pragma is confined to this ONE
 * function -- everything below it in this file is ordinary /O2 code.
 * ========================================================================== */

/* The 20-byte footprint block, copied whole (rep movsd) between the castle's
 * working copy, the query block and a cursor. */
typedef struct Footprint { int v[5]; } Footprint;
struct SpriteRec;

/* ---- the ODF class record, seen from the class-provider side -------------
 * Same 0xd0-byte ObjDef as screen.c/ridesave.c. Slot meanings (screen.c):
 * +0x8c tick/update, +0x90/+0x94 secondary updates, +0x98 place/add,
 * +0x9c remove, +0xa0 draw, +0xa4 create/attach, +0xa8 activate,
 * +0xac destroy, +0xb0 interact, +0xb8 load, +0xbc save, +0xc0 extra. */
typedef struct RideDef RideDef;

typedef struct RideElem {
    char*        name;          /* +0x00  class name */
    char*        image;         /* +0x04 */
    unsigned int flags;         /* +0x08 */
    RideDef*     data;          /* +0x0c  the ObjDef this fills in */
} RideElem;

struct RideDef {
    unsigned char pad00[0x0c];
    int           base_x;           /* +0x0c  the class's base map square */
    int           base_y;           /* +0x10 */
    unsigned char pad14[0x28];
    Footprint     footprint;        /* +0x3c  the class footprint SetEditCursorFootPrint takes */
    unsigned char pad50[0x14];
    struct SpriteRec* sprite;       /* +0x64  the class sprite */
    unsigned char pad68[0x24];
    void* cb_8c;                /* +0x8c  tick */
    void* cb_90;                /* +0x90 */
    void* cb_94;                /* +0x94 */
    void* cb_add;               /* +0x98 */
    void* cb_remove;            /* +0x9c */
    void* cb_a0;                /* +0xa0  draw */
    void* cb_a4;                /* +0xa4  create/attach */
    void* cb_a8;                /* +0xa8  activate */
    void* cb_ac;                /* +0xac  destroy */
    void* cb_b0;                /* +0xb0  interact */
    void* cb_b4;                /* +0xb4 */
    void* cb_load;              /* +0xb8 */
    void* cb_save;              /* +0xbc */
    void* cb_c0;                /* +0xc0 */
    unsigned char padc4[8];
    struct RiderNode* riders;       /* +0xcc  live rider list of the class */
};

/* ---- the roller-coaster save blob --------------------------------------
 * SaveRollerCoaster builds a 0x14-byte descriptor on the stack, allocates a
 * blob of the size it asks for, fills it and hands it to the writer;
 * LoadRollerCoaster reads a blob back, relocates its three internal pointers
 * (they are stored as offsets into the blob) and rebuilds the live coaster.
 * The +0x0c array is `count` {node, link} pairs. */
typedef struct CoasterImage {
    int f00;                    /* +0x00 */
    unsigned int size;          /* +0x04  bytes to allocate for the blob */
    int f08;
    int f0c;
    int f10;
} CoasterImage;

typedef struct CoasterNodeRef {
    void* node;                 /* +0x00 */
    void* link;                 /* +0x04 */
} CoasterNodeRef;

typedef struct CoasterBlob {
    unsigned char   pad00[8];
    int             count;      /* +0x08  entries in `nodes` */
    CoasterNodeRef* nodes;      /* +0x0c */
    int             f10;
    void*           f14;        /* +0x14 */
    int             f18;        /* +0x18 */
    void*           f1c;        /* +0x1c */
} CoasterBlob;

/* The 0x18-byte dispatch row described in the header. */
typedef struct CtIface {
    RideElem* elem;             /* +0x00 */
    void*     tick;             /* +0x04  ObjDef +0x8c */
    void*     update;           /* +0x08  ObjDef +0x90 */
    void*     add;              /* +0x0c  ObjDef +0x98 */
    void*     update2;          /* +0x10  ObjDef +0x94 */
    void*     remove;           /* +0x14  ObjDef +0x9c */
} CtIface;

extern CtIface g_ct_iface[6];                        /* 0x0082ad20 */

/* A sprite record; only +0x10 (the flag dword) is used here. Bit 0x2000 is
 * what every *_Create handler in this file sets on its class sprite. */
typedef struct SpriteRec {
    unsigned char pad00[0x10];
    unsigned int  flags;        /* +0x10 */
} SpriteRec;

/* A map square as the track code passes it around: two bytes. */
typedef struct MapPos {
    unsigned char x;            /* +0x00 */
    unsigned char y;            /* +0x01 */
} MapPos;

/* ...and the widened, PACKED form the track/castle helpers actually take. */
typedef struct MapPos16 {
    unsigned short x;           /* +0x00 */
    unsigned short y;           /* +0x02 */
} MapPos16;

/* The UNPACKED map reference ScreenToMapRef fills in and the track handlers
 * are handed: 16-bit x at +0x00 and 16-bit y at +0x04. Every track handler
 * repacks it into a MapPos16 before calling the query helpers. */
typedef struct MapRef {
    unsigned short x;           /* +0x00 */
    unsigned short pad02;
    unsigned short y;           /* +0x04 */
} MapRef;

/* An edit cursor as this file uses it: only the three fields Castle_Remove
 * fills in a LOCAL one are named. The +0x1404/+0x1408 pair is the same
 * "keep1404/keep1408" DefaultCursor preserves across its memset (loaders.c),
 * and +0x1414 is the footprint CheckCursorFootprint counts through. */
typedef struct EditCursorRec {
    unsigned char pad0000[0x1404];
    int           x;                /* +0x1404 */
    int           y;                /* +0x1408 */
    unsigned char pad140c[8];
    Footprint     footprint;        /* +0x1414 */
    unsigned char pad1428[0x1834 - 0x1428];
} EditCursorRec;

/* The castle's placed map square lives at 0x00829ae8/0x00829aea as two
 * SEPARATE signed 16-bit globals, not as one 4-byte struct: CastleDummy_Interact
 * hoists both out of a loop and the original emits two 16-bit loads there,
 * where a struct makes VC6 fold them into one dword load. The remove path
 * reads them back as plain bytes. */

/* The query block at 0x00811564 that Castle_Update2 refills before the query
 * cursor is validated. */
typedef struct QueryBlock {
    int       x;                /* +0x00  0x00811564 */
    int       y;                /* +0x04  0x00811568 */
    int       state;            /* +0x08  0x0081156c */
    int       f0c;              /* +0x0c */
    Footprint footprint;        /* +0x10  0x00811574 */
} QueryBlock;

/* ==========================================================================
 * DRIVING SCHOOL's rider tick (0x00405bd0) -- assigned to this lane, not part
 * of the castle. screen.c's SetCustomCallbacks installs it as the DRIVING
 * SCHOOL class's ObjDef +0xa8 ("activate"), and it is the class's per-frame
 * rider state machine: it walks the class's whole rider list and advances
 * each bloke through a five-step switch on the bloke's action byte (+0x60).
 * Only riders whose low-level AI state (+0x0e) is idle are stepped.
 *   0  walk to the driving-school square (base + the rider's own square,
 *      two rows up), state 7, direction from CalcMoveLine, then a random
 *      1..8 tick wait
 *   1  wait out that counter; when it expires, check the school still has
 *      room (5 * Sub_401c40(square) < Sub_413970(square)) and ask
 *      Sub_401ae0 for a car:  0 -> got one (two samples, the second picked
 *      as g_ds_samples[(rand()%4 + 1)*3] and pitched by +10, sit animation,
 *      flags |= 0x80 "sitting", action++);  -1 -> jump straight to action 3
 *      (get off);  -2 -> retry in 2..33 ticks
 *   2  nothing (the jump table sends it straight to the loop tail) -- that
 *      is the "driving" step, driven from elsewhere
 *   3  get off: clear the sitting flag, walk animation, snap the bloke onto
 *      the square, then walk to the square's centre (+0x80 in both axes)
 *   4  leave: RemoveBlokeFromRide, clear the "using this ride" flag (8) and
 *      kill the rider's samples
 * The two BlokeSoundSource locals do NOT share a frame slot (case 1's is at
 * frame-0x20, case 4's at frame-0x10) even though the cases are disjoint, so
 * they are two function-level locals, not one per-case local.
 * ========================================================================== */

/* A person as this handler sees it (same record as ridecb1.c's Bloke). */
typedef struct Bloke {
    unsigned char  pad00[0x0e];
    unsigned short state;      /* +0x0e  low-level AI state (0 = idle) */
    unsigned char  pad10[0x14];
    struct Pos { int x; int y; } target;   /* +0x24  walk target, 24.8 */
    unsigned char  pad2c[0x58 - 0x2c];
    int            wait;       /* +0x58  ride-specific countdown */
    unsigned char  pad5c[4];
    unsigned char  action;     /* +0x60  state-machine step */
    unsigned char  pad61;
    unsigned short flags62;    /* +0x62  8 = using this ride, 0x80 = sitting */
    unsigned char  pad64[4];
    struct Pos     world;      /* +0x68  world position, 24.8 */
    unsigned char  pad70[3];
    unsigned char  new_dir;    /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14]; /* +0x98  CalcMoveLine scratch */
} Bloke;

typedef struct Pos Pos;

/* The 3D person a rider drives; only its +0x20 (the render key the castle's
 * draw pass forwards) is used here. */
typedef struct RiderPerson {
    unsigned char pad00[0x20];
    int           f20;         /* +0x20 */
} RiderPerson;

/* A rider slot; +0x0c is the packed map square of the ride it is using, read
 * both as two bytes and as one 16-bit id. */
typedef struct RiderNode {
    struct RiderNode* next;    /* +0x00 */
    struct RiderNode* prev;    /* +0x04 */
    Bloke*            bloke;   /* +0x08 */
    union {
        MapPos         sq;     /* +0x0c  as {x,y} bytes */
        unsigned short id;     /* +0x0c  as one ride id */
    } key;
    unsigned short    pad0e;
    RiderPerson*      person;  /* +0x10 */
} RiderNode;

/* An {x,y} pair returned in eax:edx. */
typedef struct Offset { int ox; int oy; } Offset;

/* The 16-bit form of that square, passed to the school's helpers by value. */
typedef struct RideId { unsigned short id; } RideId;

typedef struct BlokeSoundSource {
    int    kind;               /* +0x00  1 = a bloke */
    Bloke* bloke;              /* +0x04 */
    int    pad08;
    int    pad0c;
} BlokeSoundSource;

extern QueryBlock g_query_block;                     /* 0x00811564 */

/* What CheckCursorFootprint hands back: only the two counters matter here. */
typedef struct FootprintHit {
    unsigned char pad00[0x1414];
    int f1414;                  /* +0x1414 */
    int f1418;                  /* +0x1418 */
} FootprintHit;

/* The register of built track pieces: `count` {class element, class
 * descriptor} pairs. The descriptors are the 0x38-byte .rdata records at
 * 0x004b5d20 (SQUARE_TRACK), 0x004b5d58 (SQUARE_TRACK_HEIGHT), 0x004b5d90
 * (SQUARE_TRACK_HEIGHT_0) and 0x004b5dc8 (SQUARE_TRACK_HEIGHT_PATH):
 *   +0x00 raised flag (0 for flat SQUARE_TRACK, 1 for the three HEIGHT ones)
 *   +0x04 / +0x08 the piece's two node heights (0x19, 2, 0x0f)
 *   +0x0c / +0x14 0x0f, +0x10 / +0x18 0
 *   +0x1c..+0x2c five handler pointers (draw/build/query/place/remove)
 *   +0x30 carries-a-path flag (only SQUARE_TRACK_HEIGHT_PATH sets it) */
typedef struct TrackPieceReg {
    RideElem* elem;             /* +0x00 */
    void*     desc;             /* +0x04 */
} TrackPieceReg;

extern TrackPieceReg g_track_pieces[];               /* 0x00611688 */
extern int           g_track_piece_count;            /* 0x00611958 */

/* The 0x38-byte piece descriptor, in .rdata, one per buildable class. */
typedef struct TrackDesc {
    int   raised;       /* +0x00  0 for flat SQUARE_TRACK, 1 for the rest */
    int   h0;           /* +0x04  node heights (0x19 / 2 / 0x0f) */
    int   h1;           /* +0x08 */
    int   f0c;          /* +0x0c */
    int   f10;
    int   f14;
    int   f18;
    void* draw;         /* +0x1c  five handlers */
    void* build;        /* +0x20 */
    void* query;        /* +0x24 */
    void* place;        /* +0x28 */
    void* remove;       /* +0x2c */
    int   carries_path; /* +0x30  set only on SQUARE_TRACK_HEIGHT_PATH */
    int   f34;
} TrackDesc;

extern TrackDesc g_castle_desc;                      /* 0x004b5b48  CASTLE OBJ */
extern TrackDesc g_track_desc_flat;                  /* 0x004b5d20  SQUARE_TRACK */
extern TrackDesc g_track_desc_height;                /* 0x004b5d58  SQUARE_TRACK_HEIGHT */
extern TrackDesc g_track_desc_height0;               /* 0x004b5d90  SQUARE_TRACK_HEIGHT_0 */
extern TrackDesc g_track_desc_heightpath;            /* 0x004b5dc8  SQUARE_TRACK_HEIGHT_PATH */

/* The castle's part table: `g_castle_part_count` entries of 0x24 bytes, each
 * an offset from the castle's own square plus an index into the 0x58-byte
 * record array at 0x006102f8. RefreshCastleFootprint (0x00423de0) clears
 * entry +0x00 of the last one. */
typedef struct CastlePart {
    int           f00;      /* +0x00 */
    short         dx;       /* +0x04  offset from the castle square */
    short         dy;       /* +0x06 */
    int           index;    /* +0x08  row in g_castle_records */
    int           f0c;      /* +0x0c  copied into the record's +0x44 */
    int           f10;      /* +0x10  copied into the record's +0x48 */
    unsigned char pad14[0x10];
} CastlePart;

typedef struct CastleRecord {
    unsigned char pad00[0x44];
    int           f44;      /* +0x44 */
    int           f48;      /* +0x48 */
    unsigned char pad4c[0x0c];
} CastleRecord;             /* 0x58 */

typedef struct Vec3 { int x; int y; int z; } Vec3;

extern CastlePart   g_castle_parts[];                /* 0x0060f924 */
extern int          g_castle_part_count;             /* 0x00610a08 */
extern CastleRecord g_castle_records[];              /* 0x006102f8 */

/* ---- the module's own state --------------------------------------------- */
extern RideDef*  g_castle_def;            /* 0x00829bf8  CASTLE OBJ's ObjDef */
extern RideElem* g_dummy_elem;            /* 0x00829c00  CASTLE_DUMMY's element */
extern RideDef*  g_dummy_def;             /* 0x00829c34  CASTLE_DUMMY's ObjDef */
extern RideDef*  g_trackh_def;            /* 0x00829bfc  SQUARE_TRACK_HEIGHT's ObjDef */
extern RideDef*  g_trackh0_def;           /* 0x00829a64  SQUARE_TRACK_HEIGHT_0's ObjDef */
extern void*     g_castle_sprite;         /* 0x00829c04 */
extern Footprint g_castle_footprint;      /* 0x00829a80  the 20-byte working footprint */
extern short     g_castle_x;              /* 0x00829ae8  the castle's placed square */
extern short     g_castle_y;              /* 0x00829aea */
extern int       g_castle_rec;            /* 0x00829ae0  the live castle/coaster record: +0x00 is its kind (0 = absent, 1 = placed) and the whole block through +0xc4 is one object */
extern int       g_829b88;                /* 0x00829b88 */
extern int       g_829b8c;                /* 0x00829b8c */
extern int       g_829ba0;                /* 0x00829ba0 */
extern int       g_829ba4;                /* 0x00829ba4 */
extern int       g_829af8;                /* 0x00829af8 */
extern int       g_829b04;                /* 0x00829b04 */
extern void*     g_829c08;                /* 0x00829c08 */

/* The same block at 0x00829ae0 seen as ONE record. Castle_Activate walks it
 * through a local pointer, so it needs the struct view; the rest of the file
 * touches the individual fields and keeps them as separate globals (which is
 * what their codegen asks for -- see the note on g_castle_x/g_castle_y). Both
 * declarations name the same bytes. */
typedef struct CastleRec {
    unsigned char pad00[0xa8];
    void*         sound_a;    /* +0xa8  = 0x00829b88 */
    void*         slot_a;     /* +0xac  = 0x00829b8c */
    unsigned char padb0[0x10];
    void*         sound_b;    /* +0xc0  = 0x00829ba0 */
    void*         slot_b;     /* +0xc4  = 0x00829ba4 */
} CastleRec;

extern CastleRec g_castle;                /* 0x00829ae0 */
extern int       g_829ae4;                /* 0x00829ae4  the castle node's flag dword */
extern void*     g_829aec;                /* 0x00829aec */
extern void*     g_829af0;                /* 0x00829af0 */
extern void*     g_829af4;                /* 0x00829af4 */
extern void*     g_81cdec;                /* 0x0081cdec */

/* ---- core globals (names from the export table where the game has one) --- */
extern int   EditMode;                    /* 0x008119b0 */
extern void* g_edit_class;                /* 0x008119b8  the class the cursor edits */
extern int   g_8003f0;                    /* 0x008003f0 */
extern int   g_610a04;                    /* 0x00610a04  set once the castle is placed */
extern int      EditCursor;               /* 0x007febc0 */
extern int      QueryCursor;              /* 0x00810160 */
extern MapRef   g_mapref;                 /* 0x007fffc4  ScreenToMapRef's output */
extern RideDef* QueryClass;               /* 0x00667c58 */
extern int      g_8003e8;                 /* 0x008003e8 */

/* ---- core helpers -------------------------------------------------------- */
extern void  SetEditCursorFootPrint(void* footprint);           /* 0x0045f440 */
extern void  DefaultCursor(void* cursor);                       /* 0x0045a390 */
extern void  KillSprite(void* sprite);                          /* 0x00497bd0 */
extern int   LLIDB_FindElement(const char* name, void** out,
                               unsigned int* outidx);           /* 0x0047b330 */
extern void  LLIDB_UnLoadData(void* elem);                      /* 0x0047d450 */
extern RideElem* ElemID(const char* name);                      /* 0x0047b3f0 */
extern void  ScreenToMapRef(int screen, MapRef* out, int mode);  /* 0x0045be90 */
extern void  ValidateCursor(void* cursor, RideDef* cls);        /* 0x0045f810 */
extern void  ResetCursorFootprint(void* cursor);                /* 0x0045f460 */
extern void  SetCursorError(void* cursor, int code);            /* 0x0045f480 */
extern void  PropagateCursorStatus(void* cursor);               /* 0x0045f4d0 */
extern FootprintHit* CheckCursorFootprint(void* cursor);        /* 0x0045f540 */
extern void  BasicObjectDCalcCursor(RideElem* elem, MapRef* p); /* 0x00480bb0 */
extern int   CalcMoveLine(Pos from, Pos to, void* path);        /* 0x00480740 */
extern int   NewDirForAction(Bloke* b, unsigned char dir);      /* 0x004833d0 */
extern void  BlokeSitAnim(Bloke* b);                            /* 0x00440780 */
extern void  BlokeSetFrame(Bloke* b, int frame);                /* 0x00440870 */
extern void  BlokeWalkAnim(Bloke* b);                           /* 0x00440910 */
extern void  RemoveBlokeFromRide(RideDef* item, RiderNode* r);  /* 0x0048a100 */
extern void  KillAllSamplesFromSource(BlokeSoundSource* src);   /* 0x00496b80 */
extern int   PlayInstanceOfSample(void* sample, int a, int b,
                                  BlokeSoundSource* src);       /* 0x00496d20 */
extern void  AdjustPSampleFreq(int handle, int delta);          /* 0x00492aa0 */
extern int   rand(void);                                        /* 0x0049e4b2 (CRT) */
extern void  Sub_402c10(void);                                  /* 0x00402c10 */
extern void  Sub_414440(void);                                  /* 0x00414440 */
extern int   Sub_401c40(RideId id);                             /* 0x00401c40 */
extern int   Sub_413970(RideId id);                             /* 0x00413970 */
extern int   Sub_401ae0(RideId id, Bloke* b);                   /* 0x00401ae0 */
extern void* g_ds_samples[];                                    /* 0x004b4400 */
extern void   RenderItems_New(void);                            /* 0x00442e90 */
extern void   AddBlokeToRenderList(void* list, RiderNode* r, int key); /* 0x00442f20 */
extern void   RenderBlokeList(void* list);                      /* 0x00442f70 */
extern Offset GetScreenCoordsForObject(void* inst, RideDef* item); /* 0x00442cc0 */
extern Offset GetRenderOffsetForLayer(void* obj, int layer);    /* 0x00441ee0 */
extern void   AdjustOffsetForViewMode(Offset* o);               /* 0x00442d30 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern int    g_610a18;                                         /* 0x00610a18  the castle's render list head */
extern int    Sub_421930(Bloke* b, CastleRec* rec);             /* 0x00421930 */
extern void   Sub_425170(RideElem* elem);                       /* 0x00425170 */

/* ---- neighbouring-module helpers (other lanes) --------------------------- */
extern void  Sub_41ce10(void* p);                               /* 0x0041ce10 */
extern void  Sub_41d1b0(void* p);                               /* 0x0041d1b0 */
extern void  Sub_41d7f0(void* p);                               /* 0x0041d7f0 */
extern void* Sub_41d060(void* rec, MapPos16* p);                /* 0x0041d060 */
extern int*  Sub_41d700(void* a, void* desc, MapPos16* p);      /* 0x0041d700 */
extern void  Sub_428700(void* p);                               /* 0x00428700 */
extern void  Sub_424a20(void* p);                               /* 0x00424a20 */
extern void  RefreshCastleFootprint(void);                      /* 0x00423de0 */
extern void  RefreshCastleQuery(void);                          /* 0x00423e20 */
extern void* Sub_41d100(void* rec, RideDef* def, MapPos16* p);  /* 0x0041d100 */
extern int   Sub_41d7c0(void* piece);                           /* 0x0041d7c0 */
extern void  Sub_41d6c0(int a);                                 /* 0x0041d6c0 */
extern int   Sub_41d6f0(void);                                  /* 0x0041d6f0 */
extern void  Sub_41ed90(RideElem* elem, MapRef* p);             /* 0x0041ed90 */
extern void  Sub_4245b0(MapPos16* sq);                          /* 0x004245b0 */
extern void  Sub_41ce30(void* node);                            /* 0x0041ce30 */
extern void  Sub_41d170(void* node, void* slot);                /* 0x0041d170 */
extern void  Sub_41d190(void* node, void* slot);                /* 0x0041d190 */
extern void  Sub_41d1c0(void* slot);                            /* 0x0041d1c0 */
extern void  Sub_41cfc0(void* node);                            /* 0x0041cfc0 */
extern void  Sub_41cfd0(void* node, int a);                     /* 0x0041cfd0 */
extern void  Sub_4249e0(void* rec);                             /* 0x004249e0 */
extern void  Sub_424b10(void* rec);                             /* 0x00424b10 */

/* ---- the roller-coaster (de)serialiser's helpers ------------------------ */
/* 0x004775b0 is a one-argument wrapper (it forwards {1, size} to the CRT
 * allocator at 0x004a020e), but this call site passes FOUR arguments -- the
 * extra three zeros are pushed and never read. Reproduced as the original
 * has it. */
extern void* AllocZeroed(unsigned int size, int a, int b, int c); /* 0x004775b0 */
extern void  Free_w(void* p);                                   /* 0x004775d0 (free wrapper) */
extern int   SaveGameWrite(const void* buf, unsigned int n);    /* 0x0047d760 */
extern void  BuildCoasterImage(void* rec, CoasterImage* out);   /* 0x00426d80 */
extern void  WriteCoasterHeader(CoasterImage* img, CoasterBlob* blob); /* 0x00426de0 */
extern void  PackCoasterList(void* slot, CoasterBlob* blob);    /* 0x00426bc0 */
extern void  EmitCoasterBlob(CoasterBlob* blob);                /* 0x00427220 */
extern CoasterBlob* ReadCoasterBlob(void);                      /* 0x00427240 */
extern void  RelocCoasterPtr(void* slot, CoasterBlob* blob);    /* 0x00426ba0 */
extern void  Sub_41ed80(int a);                                 /* 0x0041ed80 */
extern void  Sub_41eca0(void* a, void** b);                     /* 0x0041eca0 */
extern void  Sub_426f90(void* p, void* rec);                    /* 0x00426f90 */
extern void  Sub_427100(void* p, int a, void* rec);             /* 0x00427100 */
extern void  Sub_424ab0(void* rec);                             /* 0x00424ab0 */
extern void  Sub_423ec0(void* rec);                             /* 0x00423ec0 */
extern void  Sub_424a00(void* rec);                             /* 0x00424a00 */
extern void  Sub_424df0(void* rec);                             /* 0x00424df0 */
extern void  Sub_424e20(void);                                  /* 0x00424e20 */
extern void  Sub_41edb0(RideElem* elem, MapPos sq, EditCursorRec* cur); /* 0x0041edb0 */
extern void  Sub_424620(short* pos);                        /* 0x00424620 */
extern void  Sub_4775f0(void);                                  /* 0x004775f0 */
extern void  Sub_477410(void);                                  /* 0x00477410 */
extern void  Sub_425cb0(short* pos, float f, Vec3* out);        /* 0x00425cb0 */
extern void  Sub_4294f0(CastleRecord* rec, Vec3* v, int a, int b); /* 0x004294f0 */
extern int*  Sub_41d3b0(void* desc, MapPos16* p);               /* 0x0041d3b0 */

/* The by-name element caches that sit immediately after the table. */
extern RideElem* g_elem_square_track;                /* 0x0082adb0 */
extern RideElem* g_elem_square_track_height_path;    /* 0x0082adb4 */
extern RideElem* g_elem_castle_obj;                  /* 0x0082adb8 */
extern RideElem* g_elem_castle_dummy;                /* 0x0082adbc */
extern RideElem* g_elem_square_track_height_0;       /* 0x0082adc0 */
extern RideElem* g_elem_square_track_height;         /* 0x0082adc4 */

extern int NameCompare(const char* a, const char* b);           /* 0x004aab90 (_stricmp) */

/* ---- the five generic thunks installed in ObjDef +0x8c..+0x9c -----------
 * Defined below; the ObjDef slots take their addresses. */
typedef void (*CtTick)(RideElem* elem);
typedef void (*CtUpdate)(RideElem* elem, int a, int b);
typedef void (*CtAdd)(RideElem* elem, int a);

int  CastleClassIndex(RideElem* elem);
void CtThunk_Tick(RideElem* elem);
void CtThunk_Update(RideElem* elem, int a, int b);
void CtThunk_Update2(RideElem* elem, int a);
void CtThunk_Add(RideElem* elem, int a);
void CtThunk_Remove(RideElem* elem, int a, int b);

/* ---- CASTLE OBJ --------------------------------------------------------- */
extern void Castle_Create(void);                     /* 0x00424150 */
void Castle_Destroy(void);                           /* 0x004241e0 */
void Castle_Activate(RideElem* elem);                /* 0x004251c0 */
void Castle_Interact(RideElem* elem, int b, int c, MapPos* sq, int e, int mode); /* 0x00425050 */
int  Castle_Extra(int a, int b);                     /* 0x004246e0 */
void Castle_Tick(RideElem* elem);                    /* 0x00424240 */
void Castle_Update(RideElem* elem, int a, int b);    /* 0x00424280 */
void Castle_Add(RideElem* elem, MapRef* p);          /* 0x00424320 */
void Castle_Update2(RideElem* elem, int a);          /* 0x00424440 */
void Castle_Remove(RideElem* elem, MapPos p, int c);  /* 0x004244b0 */

/* ---- CASTLE_DUMMY ------------------------------------------------------- */
void CastleDummy_Create(RideElem* elem);             /* 0x00424830 */
void CastleDummy_Interact(int a, int b, int c, MapPos* p); /* 0x00424700 */
void CastleDummy_Update2(RideElem* elem, int a);     /* 0x00424820 */
void CastleDummy_Remove(RideElem* elem, MapPos p, int c); /* 0x00424800 */

/* ---- SQUARE_TRACK ------------------------------------------------------- */
extern void Track_Create(void);                      /* 0x00427aa0 */
void Track_Destroy(RideElem* elem);                  /* 0x00427af0 */
void Track_Interact(int a, int b, int c, MapPos* p); /* 0x00427a00 */
extern void Track_Update90(void);                    /* 0x004275d0 */
void Track_Tick(RideElem* elem);                     /* 0x00427940 */
void Track_Update(RideElem* elem, int a, int b);     /* 0x00427b20 */
void Track_Update2(RideElem* elem, MapRef* p);       /* 0x00427970 */
void Track_Add(RideElem* elem, MapRef* node);          /* 0x00427bc0 */
void Track_Remove(RideElem* elem, int a, int b);     /* 0x004279f0 */

/* ---- SQUARE_TRACK_HEIGHT / _0 / _PATH ----------------------------------- */
void TrackH_Create(RideElem* elem);                  /* 0x00427ef0 */
extern void TrackH_Update(void);                     /* 0x00427c90 */
void TrackH_Add(RideElem* elem, MapRef* node);       /* 0x00427ea0 */
void TrackH0_Create(RideElem* elem);                 /* 0x00427f30 */
void TrackHP_Create(RideElem* elem);                 /* 0x00428070 */
extern void TrackHP_Update(void);                    /* 0x004280b0 */
void TrackHP_Add(RideElem* elem, MapRef* node);      /* 0x00428300 */

/* ---- the coaster (de)serialisers ---------------------------------------- */
int  LoadRollerCoaster(void);                        /* 0x00426c20 */
int  SaveRollerCoaster(void);                        /* 0x00426ce0 */

/* See "VC6 CODEGEN NOTE" in the file header: the `volatile` parameters and
 * the empty `__asm` block are the two levers that reproduce this function's
 * unoptimised original codegen under /O2. They change no semantics. */
#pragma optimize("", off)
// FUNCTION: LEGOLAND 0x004254d0
void CastleObj_GetInterfaces(RideElem* elem, RideDef* def)
{
    if (NameCompare("CASTLE OBJ", elem->name) == 0) {
        def->cb_8c = CtThunk_Tick;
        def->cb_90 = CtThunk_Update;
        def->cb_94 = CtThunk_Update2;
        def->cb_add = CtThunk_Add;
        def->cb_remove = CtThunk_Remove;
        def->cb_a4 = Castle_Create;
        def->cb_ac = Castle_Destroy;
        def->cb_a8 = Castle_Activate;
        def->cb_b0 = Castle_Interact;
        def->cb_save = SaveRollerCoaster;
        def->cb_load = LoadRollerCoaster;
        def->cb_c0 = Castle_Extra;
        g_ct_iface[0].elem = elem;
        g_ct_iface[0].tick = Castle_Tick;
        g_ct_iface[0].update = Castle_Update;
        g_ct_iface[0].update2 = Castle_Update2;
        g_ct_iface[0].add = Castle_Add;
        g_ct_iface[0].remove = Castle_Remove;
        g_elem_castle_obj = elem;
    } else if (NameCompare("CASTLE_DUMMY", elem->name) == 0) {
        def->cb_8c = CtThunk_Tick;
        def->cb_90 = CtThunk_Update;
        def->cb_94 = CtThunk_Update2;
        def->cb_add = CtThunk_Add;
        def->cb_remove = CtThunk_Remove;
        def->cb_a4 = CastleDummy_Create;
        def->cb_b0 = CastleDummy_Interact;
        g_ct_iface[1].update2 = CastleDummy_Update2;
        g_ct_iface[1].remove = CastleDummy_Remove;
        g_elem_castle_dummy = elem;
    } else if (NameCompare("SQUARE_TRACK", elem->name) == 0) {
        def->cb_8c = CtThunk_Tick;
        def->cb_90 = CtThunk_Update;
        def->cb_94 = CtThunk_Update2;
        def->cb_add = CtThunk_Add;
        def->cb_remove = CtThunk_Remove;
        def->cb_a4 = Track_Create;
        def->cb_ac = Track_Destroy;
        def->cb_b0 = Track_Interact;
        def->cb_90 = Track_Update90;
        g_ct_iface[2].elem = elem;
        g_ct_iface[2].tick = Track_Tick;
        g_ct_iface[2].update = Track_Update;
        g_ct_iface[2].update2 = Track_Update2;
        g_ct_iface[2].add = Track_Add;
        g_ct_iface[2].remove = Track_Remove;
        g_elem_square_track = elem;
    } else if (NameCompare("SQUARE_TRACK_HEIGHT", elem->name) == 0) {
        def->cb_8c = CtThunk_Tick;
        def->cb_90 = CtThunk_Update;
        def->cb_94 = CtThunk_Update2;
        def->cb_add = CtThunk_Add;
        def->cb_remove = CtThunk_Remove;
        def->cb_a4 = TrackH_Create;
        def->cb_b0 = Track_Interact;
        g_ct_iface[3].elem = elem;
        g_ct_iface[3].tick = Track_Tick;
        g_ct_iface[3].update = TrackH_Update;
        g_ct_iface[3].update2 = Track_Update2;
        g_ct_iface[3].add = TrackH_Add;
        g_ct_iface[3].remove = Track_Remove;
        g_elem_square_track_height = elem;
    } else if (NameCompare("SQUARE_TRACK_HEIGHT_0", elem->name) == 0) {
        def->cb_8c = CtThunk_Tick;
        def->cb_90 = CtThunk_Update;
        def->cb_94 = CtThunk_Update2;
        def->cb_add = CtThunk_Add;
        def->cb_remove = CtThunk_Remove;
        def->cb_a4 = TrackH0_Create;
        def->cb_b0 = Track_Interact;
        g_ct_iface[4].elem = elem;
        g_ct_iface[4].tick = Track_Tick;
        g_ct_iface[4].update = TrackH_Update;
        g_ct_iface[4].update2 = Track_Update2;
        g_ct_iface[4].add = TrackH_Add;
        g_ct_iface[4].remove = Track_Remove;
        g_elem_square_track_height_0 = elem;
    } else if (NameCompare("SQUARE_TRACK_HEIGHT_PATH", elem->name) == 0) {
        def->cb_8c = CtThunk_Tick;
        def->cb_90 = CtThunk_Update;
        def->cb_94 = CtThunk_Update2;
        def->cb_add = CtThunk_Add;
        def->cb_remove = CtThunk_Remove;
        def->cb_a4 = TrackHP_Create;
        def->cb_b0 = Track_Interact;
        g_ct_iface[5].elem = elem;
        g_ct_iface[5].tick = Track_Tick;
        g_ct_iface[5].update = TrackHP_Update;
        g_ct_iface[5].update2 = Track_Update2;
        g_ct_iface[5].add = TrackHP_Add;
        g_ct_iface[5].remove = Track_Remove;
        g_elem_square_track_height_path = elem;
    } else if (NameCompare("ROLLER_COASTER_LOAD", elem->name) == 0) {
        def->cb_8c = CtThunk_Tick;
        def->cb_90 = CtThunk_Update;
        def->cb_94 = CtThunk_Update2;
        def->cb_add = CtThunk_Add;
        def->cb_remove = CtThunk_Remove;
        def->cb_8c = LoadRollerCoaster;
    } else if (NameCompare("ROLLER_COASTER_SAVE", elem->name) == 0) {
        def->cb_8c = CtThunk_Tick;
        def->cb_90 = CtThunk_Update;
        def->cb_94 = CtThunk_Update2;
        def->cb_add = CtThunk_Add;
        def->cb_remove = CtThunk_Remove;
        def->cb_8c = SaveRollerCoaster;
    }
}
#pragma optimize("", on)

/* ==========================================================================
 * The generic dispatch layer (0x0041ec20..0x0041ed50)
 *
 * Five one-line thunks -- one per ObjDef callback slot the eight classes
 * share -- plus the row lookup and two small accessors. Each thunk turns the
 * class element into a table row and tails into that row's implementation.
 * The `add esp` after the indirect call cleans up BOTH the row-lookup
 * argument and the forwarded arguments in one go, which is what writing the
 * lookup INSIDE the call expression gives.
 * ========================================================================== */

/* Class element -> dispatch row. A linear scan of the six `elem` keys; a
 * miss falls out of the loop and returns row 0 (the castle). The trip test
 * is against the END OF THE TABLE, 0x0082adb0, which is also the address of
 * `g_elem_square_track` -- i.e. the table's bound is the start of the by-name
 * element cache that follows it. */
// FUNCTION: LEGOLAND 0x0041ec20
int CastleClassIndex(RideElem* elem)
{
    int i;

    for (i = 0; i < 6; i++) {
        if (elem == g_ct_iface[i].elem)
            return i;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x0041ec40
RideElem* CastleClassElem(int index)
{
    return g_ct_iface[index].elem;
}

// FUNCTION: LEGOLAND 0x0041ec50
void CtThunk_Tick(RideElem* elem)
{
    ((CtTick)g_ct_iface[CastleClassIndex(elem)].tick)(elem);
}

// FUNCTION: LEGOLAND 0x0041ec70
void CtThunk_Update(RideElem* elem, int a, int b)
{
    ((CtUpdate)g_ct_iface[CastleClassIndex(elem)].update)(elem, a, b);
}

// FUNCTION: LEGOLAND 0x0041ece0
void CtThunk_Add(RideElem* elem, int a)
{
    ((CtAdd)g_ct_iface[CastleClassIndex(elem)].add)(elem, a);
}

// FUNCTION: LEGOLAND 0x0041ed00
void CtThunk_Update2(RideElem* elem, int a)
{
    ((CtAdd)g_ct_iface[CastleClassIndex(elem)].update2)(elem, a);
}

// FUNCTION: LEGOLAND 0x0041ed50
void CtThunk_Remove(RideElem* elem, int a, int b)
{
    ((CtUpdate)g_ct_iface[CastleClassIndex(elem)].remove)(elem, a, b);
}

/* Remove by ROW rather than by element: the row supplies its own element and
 * the third argument is forced to 0. Both table reads are off one scaled
 * index, so VC6 materialises `row*0x18` once. */
// FUNCTION: LEGOLAND 0x0041ed20
void CastleClassRemoveByIndex(int index, int a)
{
    ((CtUpdate)g_ct_iface[index].remove)(g_ct_iface[index].elem, a, 0);
}

/* ==========================================================================
 * The class implementations
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x004279f0
void Track_Remove(RideElem* elem, int a, int b)
{
    Sub_41d7f0(g_81cdec);
}

// FUNCTION: LEGOLAND 0x00424820
void CastleDummy_Update2(RideElem* elem, int a)
{
    Castle_Update2(0, 0);
}

/* The dummy's remove is the castle's remove with an all-zero map square. */
// FUNCTION: LEGOLAND 0x00424800
void CastleDummy_Remove(RideElem* elem, MapPos p, int c)
{
    MapPos zero;

    zero.x = 0;
    zero.y = 0;
    Castle_Remove(0, zero, 0);
}

/* RESIDUAL (audit: 5 of 9 instructions mismatch, but every one of the five
 * differs ONLY in which register holds which value): the original keeps
 * `elem` in eax and `elem->data` in ecx; every C spelling tried here comes out
 * with the pair swapped (elem in ecx, def in eax) and is otherwise
 * instruction-for-instruction identical --
 *     orig: mov eax,[esp+4] / mov ecx,[eax+0xc] / mov [g_dummy_elem],eax
 *           / mov [g_dummy_def],ecx / mov eax,[ecx+0x64] / ...
 *     ours: mov ecx,[esp+4] / mov eax,[ecx+0xc] / mov [g_dummy_elem],ecx
 *           / mov [g_dummy_def],eax / mov eax,[eax+0x64] / ...
 * VC6 only puts the PARAMETER in eax here when it has a third use (a
 * post-store reload, or a trailing dead store), both of which cost an extra
 * instruction. ~25 spellings tried: statement permutations, an extra CSE'd
 * load, a struct for the two globals, store-forwarding the def through the
 * global, an inline helper, a return value, `#pragma optimize("a"/"w", on)`.
 * The sibling TrackH_Create/TrackH0_Create -- same shape plus a registry
 * append -- match exactly with the ordinary spelling, so the tie-break is the
 * two values' equal-length live ranges here. */
// WIP-FUNCTION: LEGOLAND 0x00424830  (44.4% strict, 5 of 9 instructions differ only by the elem/def register pair; see note)
void CastleDummy_Create(RideElem* elem)
{
    RideDef* def = elem->data;

    g_dummy_elem = elem;
    g_dummy_def = def;
    def->sprite->flags |= 0x2000;
}

// FUNCTION: LEGOLAND 0x00427940
void Track_Tick(RideElem* elem)
{
    RideDef* def = elem->data;

    EditMode = 1;
    g_edit_class = def;
    SetEditCursorFootPrint(&def->footprint);
    g_8003f0 = 0;
}

// FUNCTION: LEGOLAND 0x00427ef0
void TrackH_Create(RideElem* elem)
{
    RideDef* def = elem->data;

    g_trackh_def = def;
    def->sprite->flags |= 0x2000;
    g_track_pieces[g_track_piece_count].elem = elem;
    g_track_pieces[g_track_piece_count].desc = &g_track_desc_height;
    g_track_piece_count++;
}

// FUNCTION: LEGOLAND 0x00427f30
void TrackH0_Create(RideElem* elem)
{
    RideDef* def = elem->data;

    g_trackh0_def = def;
    def->sprite->flags |= 0x2000;
    g_track_pieces[g_track_piece_count].elem = elem;
    g_track_pieces[g_track_piece_count].desc = &g_track_desc_height0;
    g_track_piece_count++;
}

/* The PATH variant keeps no ObjDef global of its own. */
// FUNCTION: LEGOLAND 0x00428070
void TrackHP_Create(RideElem* elem)
{
    elem->data->sprite->flags |= 0x2000;
    g_track_pieces[g_track_piece_count].elem = elem;
    g_track_pieces[g_track_piece_count].desc = &g_track_desc_heightpath;
    g_track_piece_count++;
}

/* Drop the shared "BASIC TILES 1" tileset when a track class is torn down. */
// FUNCTION: LEGOLAND 0x00427af0
void Track_Destroy(RideElem* elem)
{
    void* tiles;

    if (!LLIDB_FindElement("BASIC TILES 1", &tiles, 0))
        LLIDB_UnLoadData(tiles);
}

// FUNCTION: LEGOLAND 0x00427a00
void Track_Interact(int a, int b, int c, MapPos* p)
{
    MapPos16 sq;
    void* piece;

    sq.x = p->x;
    sq.y = p->y;
    piece = Sub_41d060(&g_castle_rec, &sq);
    Sub_428700(piece);
    Sub_424a20(piece);
}

// FUNCTION: LEGOLAND 0x00427bc0
void Track_Add(RideElem* elem, MapRef* node)
{
    MapPos16 sq;
    int* cell;

    sq.x = node->x;
    sq.y = node->y;
    cell = Sub_41d700(g_829c08, &g_track_desc_flat, &sq);
    if (cell)
        *cell |= 6;
}

/* Pick the castle's edit cursor up again, unless the castle is already down. */
// FUNCTION: LEGOLAND 0x00424240
void Castle_Tick(RideElem* elem)
{
    if (!g_610a04) {
        EditMode = 1;
        g_edit_class = g_castle_def;
        DefaultCursor(&EditCursor);
        RefreshCastleFootprint();
        SetEditCursorFootPrint(&g_castle_footprint);
    }
}

// FUNCTION: LEGOLAND 0x004241e0
void Castle_Destroy(void)
{
    KillSprite(g_castle_sprite);
    g_829b8c = 0;
    g_829ba4 = 0;
    Sub_41d1b0(&g_829b8c);
    Sub_41d1b0(&g_829ba4);
    g_829b88 = 0;
    g_829ba0 = 0;
    Sub_41ce10(&g_829af8);
    Sub_41ce10(&g_829b04);
    g_castle_rec = 0;
}

/* The registry lookup the SQUARE_TRACK_HEIGHT* add handlers use: class element
 * -> its 0x38-byte piece descriptor, or 0 if the class never registered. */
// FUNCTION: LEGOLAND 0x00427c00
void* FindTrackDesc(RideElem* elem)
{
    int i;

    for (i = 0; i < g_track_piece_count; i++) {
        if (g_track_pieces[i].elem == elem)
            return g_track_pieces[i].desc;
    }
    return 0;
}

/* Mark the square the piece occupies as blocked (bits 1|2). No null check on
 * the returned cell -- the flat SQUARE_TRACK handler has one, these do not. */
// FUNCTION: LEGOLAND 0x00427ea0
void TrackH_Add(RideElem* elem, MapRef* node)
{
    MapPos16 sq;
    RideDef* def = elem->data;
    void* desc;
    int* cell;

    sq.x = node->x;
    sq.y = node->y;
    desc = FindTrackDesc(elem);
    if (desc) {
        cell = Sub_41d700(def, desc, &sq);
        *cell |= 6;
    }
}

/* The PATH variant CLEARS the same two bits instead: a raised piece with a
 * path under it leaves the square walkable. */
// FUNCTION: LEGOLAND 0x00428300
void TrackHP_Add(RideElem* elem, MapRef* node)
{
    MapPos16 sq;
    RideDef* def = elem->data;
    void* desc;
    int* cell;

    sq.x = node->x;
    sq.y = node->y;
    desc = FindTrackDesc(elem);
    if (desc) {
        cell = Sub_41d700(def, desc, &sq);
        *cell &= ~6;
    }
}

/* Query-mode hover: find the track piece under the cursor and either latch it
 * (0x0081cdec) or clear the latch and flag the query cursor. */
// FUNCTION: LEGOLAND 0x00427970
void Track_Update2(RideElem* elem, MapRef* p)
{
    MapPos16 sq;
    RideDef* def = elem->data;
    void* piece;

    BasicObjectDCalcCursor(elem, p);
    sq.x = p->x;
    sq.y = p->y;
    piece = Sub_41d100(&g_castle_rec, def, &sq);
    if (!Sub_41d7c0(piece)) {
        SetCursorError(&QueryCursor, 1);
        g_81cdec = 0;
    } else {
        ResetCursorFootprint(&QueryCursor);
        g_81cdec = piece;
    }
}

/* Build-mode hover for the flat track: re-arm the cursor footprint, project
 * the mouse onto the map, validate, then ask the flat piece descriptor
 * whether this square can take a piece. */
// FUNCTION: LEGOLAND 0x00427b20
void Track_Update(RideElem* elem, int a, int b)
{
    MapPos16 sq;
    int* ok;

    SetEditCursorFootPrint(&elem->data->footprint);
    ScreenToMapRef(a, &g_mapref, b);
    ValidateCursor(&EditCursor, elem->data);
    g_8003e8 |= 8;
    sq.x = g_mapref.x;
    sq.y = g_mapref.y;
    ok = Sub_41d3b0(&g_track_desc_flat, &sq);
    if (*ok)
        ResetCursorFootprint(&EditCursor);
    else
        SetCursorError(&EditCursor, 0xe);
}

/* Build-mode hover for the castle. */
// FUNCTION: LEGOLAND 0x00424280
void Castle_Update(RideElem* elem, int a, int b)
{
    FootprintHit* hit;

    RefreshCastleFootprint();
    SetEditCursorFootPrint(&g_castle_footprint);
    ScreenToMapRef(a, &g_mapref, b);
    g_8003f0 = 0;
    hit = CheckCursorFootprint(&EditCursor);
    if (hit) {
        hit->f1418++;
        hit->f1414++;
    }
    ResetCursorFootprint(&EditCursor);
    ValidateCursor(&EditCursor, elem->data);
    if (g_610a04) {
        SetCursorError(&EditCursor, 0xf);
        PropagateCursorStatus(&EditCursor);
    }
}

/* Query-mode hover for the castle: refill the query block from the castle's
 * placed square and footprint, then decide whether the thing under the query
 * cursor is the castle's own dummy. */
// FUNCTION: LEGOLAND 0x00424440
void Castle_Update2(RideElem* elem, int a)
{
    RefreshCastleQuery();
    g_query_block.footprint = g_castle_footprint;
    g_query_block.x = g_castle_x;
    g_query_block.y = g_castle_y;
    if (QueryClass != ElemID("CASTLE_DUMMY")->data)
        ResetCursorFootprint(&QueryCursor);
    else
        g_query_block.state = 0;
}

/* The castle record's public accessor: the record only exists once the castle
 * has been placed, so a null-or-record answer. Written as a conditional so
 * VC6 emits the branchless neg/sbb/and the original has. */
// FUNCTION: LEGOLAND 0x00424140
void* GetCastleRec(void)
{
    return g_610a04 ? &g_castle_rec : 0;
}

/* Place the castle: build the map node at 0x00829ae0, hang the two sound
 * slots off it, register it and latch "the castle is down" (0x00610a04).
 * The node is self-referential -- +0x14 points back at the record's own head
 * -- and +0x10 carries the castle's 0x38-byte piece descriptor at 0x004b5b48,
 * the same record shape the four SQUARE_TRACK classes register. */
// FUNCTION: LEGOLAND 0x00424320
void Castle_Add(RideElem* elem, MapRef* p)
{
    MapPos16 sq;

    Sub_41d6c0(0);
    Sub_41ed90(elem, p);
    sq.x = p->x;
    sq.y = p->y;
    Sub_4245b0(&sq);
    Sub_41ce30(&g_829ae4);
    g_castle_rec = 1;
    g_castle_x = p->x;
    g_castle_y = p->y;
    g_829af0 = &g_castle_desc;
    g_829af4 = &g_castle_rec;
    g_829aec = g_castle_def;
    g_829b88 = (int)&g_829ae4;
    g_829ba0 = (int)&g_829ae4;
    g_829b8c = 0;
    g_829ba4 = 0;
    Sub_41d170(&g_829ae4, &g_829b8c);
    Sub_41d190(&g_829ae4, &g_829ba4);
    Sub_41d1c0(&g_829b8c);
    Sub_41d1c0(&g_829ba4);
    Sub_41cfc0(&g_829ae4);
    Sub_41cfd0(&g_829ae4, 0);
    Sub_4249e0(&g_castle_rec);
    Sub_424b10(&g_castle_rec);
    g_610a04 = 1;
    g_829ae4 |= 6;
}

/* Ends in a tail `jmp`, so tools/match.py cannot bound it; audit.py can. */
// WIP-FUNCTION: LEGOLAND 0x004246e0  (100% by audit.py; match.py cannot bound a tail-jmp function)
int Castle_Extra(int a, int b)
{
    if (b != 0 && g_castle_rec != 2)
        return 0;
    return Sub_41d6f0();
}

/* The DRIVING SCHOOL rider tick -- see the block comment near the top.
 *
 * RESIDUAL (134 of 210 instructions; 210/210 instructions and 641/641 bytes
 * in the ORIGINAL's extent, so the control flow, the jump table, the merged
 * `add esp` groups and every operand form are right). What is left is one
 * systemic scratch-register difference: in the two coordinate-building cases
 * (0 and 3) and in the sample block of case 1 the original puts the class's
 * base coordinate in eax and the zero-extended map byte in ecx, where VC6
 * gives us the reverse, so `add eax,edx` comes out as `add edx,ecx` and every
 * later temp in the block is renamed with it. The same root cause makes case
 * 1 push the literal 0 as `push eax` (VC6 still knows the Sub_401ae0 result
 * in eax is zero, because it used edx and not eax for the `lea &src`) --
 * one byte, twice.
 * Tried without effect: both operand orders on every `base + square` sum, an
 * accumulate form (`t = base; t += sq.x;`), `* 256` instead of `<< 8`, an
 * explicit `(int)` cast, unsigned base fields, `!b->state` vs `== 0`, an
 * `(int)` cast on the switch value, per-case scopes for the temporaries,
 * swapping the x/y statement order (that one is worse: 181) and reordering
 * the two loop-head loads. The allocation is fixed at 76 mismatches across
 * all of them, which points at a whole-function virtual-register ordering
 * decision rather than at any one statement. */
// WIP-FUNCTION: LEGOLAND 0x00405bd0  (63.8%, exact instruction count/length/flow; eax<->ecx scratch renaming in cases 0/3 -- see note)
void DrivingSchool_TickRiders(RideElem* elem)
{
    BlokeSoundSource src;
    BlokeSoundSource src2;
    RideDef* item = elem->data;
    RiderNode* rider = item->riders;
    RiderNode* next;
    Bloke* b;

    Sub_402c10();
    Sub_414440();
    while (rider) {
        b = rider->bloke;
        next = rider->next;
        if (b->state == 0) {
            switch (b->action) {
            case 0:
            {
                unsigned char d;
                b->target.x = (item->base_x + rider->key.sq.x) << 8;
                b->target.y = (rider->key.sq.y + item->base_y - 3) << 8;
                d = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = d;
                NewDirForAction(b, (unsigned char)((d >> 5) + 3));
                b->wait = (rand() & 7) + 1;
                b->action++;
                break;
            }
            case 1:
            {
                int car;
                int k;
                int h;
                if (b->wait == 0) {
                    if (Sub_401c40(*(RideId*)&rider->key) * 5 <
                        Sub_413970(*(RideId*)&rider->key)) {
                        car = Sub_401ae0(*(RideId*)&rider->key, b);
                        if (car == 0) {
                            src.kind = 1;
                            src.bloke = b;
                            PlayInstanceOfSample(g_ds_samples[0], 0, 1, &src);
                            k = rand() % 4 + 1;
                            h = PlayInstanceOfSample(g_ds_samples[k * 3], 1, 1, &src);
                            AdjustPSampleFreq(h, 0xa);
                            BlokeSitAnim(b);
                            BlokeSetFrame(b, 0);
                            b->action++;
                            b->flags62 |= 0x80;
                        } else if (car == -1) {
                            b->action = 3;
                        } else if (car == -2) {
                            b->wait = (rand() & 0x1f) + 2;
                        }
                    }
                } else {
                    b->wait--;
                }
                break;
            }
            case 3:
            {
                unsigned char d;
                b->flags62 &= ~0x80;
                BlokeWalkAnim(b);
                BlokeSetFrame(b, 0);
                b->world.x = (rider->key.sq.x + item->base_x) << 8;
                b->world.y = (rider->key.sq.y + item->base_y - 3) << 8;
                b->target.x = ((rider->key.sq.x + item->base_x) << 8) + 0x80;
                b->target.y = ((rider->key.sq.y + item->base_y) << 8) + 0x80;
                d = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                b->new_dir = d;
                NewDirForAction(b, (unsigned char)((d >> 5) + 3));
                b->action++;
                break;
            }
            case 4:
                RemoveBlokeFromRide(item, rider);
                b->flags62 &= ~8;
                src2.kind = 1;
                src2.bloke = b;
                KillAllSamplesFromSource(&src2);
                break;
            }
        }
        rider = next;
    }
}

/* Serialise the placed coaster. With no castle down there is no coaster, and
 * the whole chunk is a single zero int -- the same "empty list" marker the
 * per-ride savers in ridesave.c write. */
// FUNCTION: LEGOLAND 0x00426ce0
int SaveRollerCoaster(void)
{
    int zero;
    CoasterImage img;
    void* rec;
    CoasterBlob* blob;

    rec = GetCastleRec();
    if (rec) {
        BuildCoasterImage(rec, &img);
        blob = (CoasterBlob*)AllocZeroed(img.size, 0, 0, 0);
        if (!blob)
            return 0;
        WriteCoasterHeader(&img, blob);
        PackCoasterList(&blob->nodes, blob);
        PackCoasterList(&blob->f14, blob);
        PackCoasterList(&blob->f1c, blob);
        EmitCoasterBlob(blob);
        Free_w(blob);
        return 1;
    }
    zero = 0;
    SaveGameWrite(&zero, 4);
    return 1;
}

/* Rebuild the coaster from the blob. A null blob is a failure; the sentinel
 * -1 means "the chunk said there is no coaster", which is a success. */
// FUNCTION: LEGOLAND 0x00426c20
int LoadRollerCoaster(void)
{
    CoasterBlob* blob = ReadCoasterBlob();
    void* rec;
    int i;

    if (!blob)
        return 0;
    if (blob == (CoasterBlob*)-1)
        return 1;
    RelocCoasterPtr(&blob->nodes, blob);
    RelocCoasterPtr(&blob->f14, blob);
    RelocCoasterPtr(&blob->f1c, blob);
    Sub_41ed80(0);
    for (i = 0; i < blob->count; i++)
        Sub_41eca0(blob->nodes[i].node, &blob->nodes[i].link);
    rec = GetCastleRec();
    if (blob->f14)
        Sub_426f90(blob->f14, rec);
    if (blob->f1c)
        Sub_427100(blob->f1c, blob->f18, rec);
    Free_w(blob);
    Sub_41ed80(1);
    return 1;
}

/* Pull the castle back off the map. Everything hangs off the module's own
 * record, so neither the element nor the square the caller passes is used:
 * the square is re-read from the record's own +0x08/+0x0a. A whole 0x182c-byte
 * edit cursor is built on the stack (which is what the 0x183c-byte frame is
 * for) and handed to the un-place helper with the castle's class element. */
// FUNCTION: LEGOLAND 0x004244b0
void Castle_Remove(RideElem* elem, MapPos p, int c)
{
    MapPos sq;
    EditCursorRec cur;
    RideElem* e0;

    if (g_610a04 != 0) {
        sq.x = (unsigned char)g_castle_x;
        sq.y = (unsigned char)g_castle_y;
        cur.x = sq.x;
        cur.y = sq.y;
        cur.footprint = g_castle_def->footprint;
        cur.footprint.v[4] = 0;
        Sub_424ab0(&g_castle_rec);
        Sub_423ec0(&g_castle_rec);
        Sub_41d1b0(&g_829b8c);
        Sub_41d1b0(&g_829ba4);
        g_829b88 = 0;
        g_829ba0 = 0;
        Sub_424a00(&g_castle_rec);
        Sub_424df0(&g_castle_rec);
        Sub_424e20();
        e0 = CastleClassElem(0);
        Sub_41edb0(e0, sq, &cur);
        Sub_424620(&g_castle_x);
        g_610a04 = 0;
        Sub_4775f0();
        Sub_477410();
        Sub_41d6c0(0);
    }
}

/* RESIDUAL (56 of 69 instructions; 69/69 instructions and 256/256 bytes, so
 * the frame, the dead-parameter-slot home for the packed square, the two
 * hoisted 16-bit castle coordinates, the 0x24-byte part stride, the
 * ecx*11*8 record indexing and the 0x58-byte rep-movsd copy are all right).
 * What is left is the same tie-break that blocks CB_405bd0: the original
 * keeps the part count in edx and the wanted square in esi and hoists the
 * castle's y before its x, where VC6 gives us the two registers the other way
 * round and hoists x first. Tried: both operand orders on each sum, x/y
 * statement order (worse), `sq.id == want` vs `want == sq.id`, folding the
 * `want` read into the for-init, and a local for the count. All 13. */
/* The dummy's "interact": work out which of the castle's parts the clicked
 * square is (the part table holds offsets from the castle's own square), and
 * if it is one of them, hand its 0x58-byte record -- patched with the part's
 * two extra fields -- to the presenter. Not finding a part is not an error;
 * the tail runs either way. */
// WIP-FUNCTION: LEGOLAND 0x00424700  (81.2%, exact length/flow; edx<->esi and the hoist order of the two castle coordinates -- see note)
void CastleDummy_Interact(int a, int b, int c, MapPos* p)
{
    Vec3 out;
    CastleRecord rec;
    union { MapPos16 p; unsigned int id; } sq;
    unsigned int want;
    int i;

    Sub_425cb0(&g_castle_x, (float)g_castle_desc.h1, &out);
    sq.p.x = p->x;
    sq.p.y = p->y;
    want = sq.id;
    for (i = 0; i < g_castle_part_count; i++) {
        sq.p.x = (unsigned short)(g_castle_parts[i].dx + g_castle_x);
        sq.p.y = (unsigned short)(g_castle_parts[i].dy + g_castle_y);
        if (want == sq.id) {
            rec = g_castle_records[g_castle_parts[i].index];
            rec.f44 = g_castle_parts[i].f0c;
            rec.f48 = g_castle_parts[i].f10;
            Sub_4294f0(&rec, &out, 1, 0);
            break;
        }
    }
    Sub_424a20(&g_829ae4);
}

/* CASTLE OBJ's ObjDef +0xb0. Despite the slot's usual "interact" meaning this
 * is the castle's DRAW pass: it collects every rider standing on this square
 * whose action byte is outside 0x10..0x20 into a render list, draws them, and
 * then blits the castle's own sprite at the class's layer-2 render offset.
 *
 * Like CastleObj_GetInterfaces this function was built UNOPTIMISED in the
 * original -- an ebp frame, every value spilled to its own stack home, and
 * `cmp dword ptr [ebp-8], 0` instead of a register test -- so it carries the
 * same `#pragma optimize("", off)`. See the note in the file header. */
#pragma optimize("", off)
// FUNCTION: LEGOLAND 0x00425050
void Castle_Interact(RideElem* elem, int b, int c, MapPos* sq, int e, int mode)
{
    RideDef* def = elem->data;
    RiderNode* r;
    Bloke* bl;

    RenderItems_New();
    g_610a18 = 0;
    r = def->riders;
    while (r != 0) {
        if (*(unsigned short*)sq == r->key.id) {
            bl = r->bloke;
            if (bl->action < 0x10)
                AddBlokeToRenderList(&g_610a18, r, r->person->f20);
            if (bl->action >= 0x21)
                AddBlokeToRenderList(&g_610a18, r, r->person->f20);
        }
        r = r->next;
    }
    RenderBlokeList(&g_610a18);
    if (g_castle_sprite != 0) {
        /* FRAME LAYOUT: both Offsets must live in THIS block, not at function
         * level. Under `optimize("",off)` VC6 gives block-scope aggregates the
         * lowest homes (ebp-0x1c and ebp-0x14) and leaves the function-level
         * scalars above them (bl at ebp-0xc); declaring them at function level
         * puts `bl` between the two and costs 8 of 97 instructions. */
        Offset off;
        Offset pos;

        pos = GetScreenCoordsForObject(sq, def);
        off = GetRenderOffsetForLayer(def->sprite, 2);
        AdjustOffsetForViewMode(&off);
        PrintSprite(g_castle_sprite, pos.ox + off.ox, pos.oy + off.oy, mode, 0);
    }
}
#pragma optimize("", on)

/* CASTLE OBJ's ObjDef +0xa8 -- the castle's own rider tick, the same shape as
 * the DRIVING SCHOOL one above but driven by a SPARSE action byte: the switch
 * is a two-level table (a 0x41-byte index at 0x00425488 selecting one of seven
 * blocks at 0x0042546c) over actions 0, 1, 0x10, 0x21, 0x22 and 0x40, with
 * everything else falling through.
 *   0     claim the castle (flags |= 8) and walk to (x-8, y-1)
 *   1     when that walk finishes, jump to action 0x10
 *   0x10  hand the bloke to the castle interior (0x00421930) and set 0x20
 *   0x21  walk back out to the centre of the square (+0x80 in both axes)
 *   0x22  when that walk finishes, jump to action 0x40
 *   0x40  clear the "using this ride" flag and RemoveBlokeFromRide
 * The two sound slots on the record are re-armed before the walk, and the
 * whole pass ends in 0x00425170.
 *
 * Built UNOPTIMISED in the original, like CastleObj_GetInterfaces and
 * Castle_Interact -- ebp frame, every value in its own stack home, the switch
 * value materialised into a local before the range check.
 *
 * RESIDUAL (156 of 218 instructions): 218/218 instructions and 684/684 bytes,
 * and with the `[ebp-N]` displacements masked out the two listings are
 * IDENTICAL line for line -- every instruction, both jump tables, the
 * two-level switch, the operand forms. The only difference is which frame
 * home each of the nine locals gets:
 *     orig  -0x04 r  -0x08 sq  -0x0c rec  -0x10 item  -0x14 y
 *           -0x18 x  -0x1c b   -0x20 next -0x24 rc    -0x28 switch temp
 *     ours  -0x04 rc -0x08 b   -0x0c r    -0x10 item  -0x14 sq
 *           -0x18 rec -0x1c x  -0x20 y    -0x24 next  -0x28 switch temp
 * Declaration order does not move them (two orders, one the exact inverse of
 * the other, produce byte-identical objects), and neither does hoisting the
 * loop locals into the while body or the dead `rc` into its own case block.
 * That is the tell that `#pragma optimize("", off)` inside an /O2 compile
 * still hands the frame to the OPTIMISER's slot colourer rather than to the
 * /Od "declaration order from ebp-4 down" allocator -- the same reason
 * Castle_Interact needed its two Offsets moved into a block scope to land on
 * the original's homes. `item` and the switch temp already agree; the other
 * eight would need whatever steers that colourer. */
#pragma optimize("", off)
// WIP-FUNCTION: LEGOLAND 0x004251c0  (71.6%, exact length/flow/operands; only the nine locals' frame homes differ -- see note)
void Castle_Activate(RideElem* elem)
{
    RiderNode* r;
    MapPos* sq;
    CastleRec* rec;
    RideDef* item;
    int y;
    int x;
    Bloke* b;
    RiderNode* next;
    int rc;

    item = elem->data;
    r = item->riders;
    rec = &g_castle;
    if (rec->sound_a != 0)
        Sub_41d170(rec->sound_a, &rec->slot_a);
    if (rec->sound_b != 0)
        Sub_41d190(rec->sound_b, &rec->slot_b);
    while (r != 0) {
        next = r->next;
        sq = &r->key.sq;
        x = sq->x + item->base_x;
        y = sq->y + item->base_y;
        b = r->bloke;
        switch (b->action) {
        case 0:
            b->flags62 |= 8;
            if (b->state == 0) {
                b->target.x = (x - 8) << 8;
                b->target.y = (y - 1) << 8;
                b->new_dir = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                NewDirForAction(b, (unsigned char)(((int)b->new_dir >> 5) + 3));
                b->action++;
            }
            break;
        case 1:
            if (b->state == 0)
                b->action = 0x10;
            break;
        case 0x10:
            rc = Sub_421930(b, &g_castle);
            b->action = 0x20;
            break;
        case 0x21:
            if (b->state == 0) {
                b->target.x = (x << 8) + 0x80;
                b->target.y = (y << 8) + 0x80;
                b->new_dir = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
                b->state = 7;
                NewDirForAction(b, (unsigned char)(((int)b->new_dir >> 5) + 3));
                b->action++;
            }
            break;
        case 0x22:
            if (b->state == 0)
                b->action = 0x40;
            break;
        case 0x40:
            b->flags62 &= ~8;
            RemoveBlokeFromRide(item, r);
            break;
        }
        r = next;
    }
    Sub_425170(elem);
}
#pragma optimize("", on)
