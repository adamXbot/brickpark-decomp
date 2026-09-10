/* LEGOLAND -- the ROLLER-COASTER TRACK system (and the driving school's cars
 * and traffic lights, which share this address range).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere).
 *
 * This file is the other half of castleobj.c. That file has the CASTLE OBJ
 * class provider and the per-class ObjDef handlers; this one has the track
 * data structure those handlers manipulate, the editor's preview/snap pass,
 * the coaster save image, and the DRIVING SCHOOL bits at 0x00401.../0x00402...
 * /0x00413.../0x00414... that castleobj.c reaches through externs.
 *
 * =========================================================================
 * HOW A COASTER IS STORED  (the central discovery of this lane)
 * =========================================================================
 * There is exactly ONE coaster per park and it hangs off the castle: the
 * record at 0x00829ae0 that castleobj.c calls `g_castle_rec`. That record IS
 * ITSELF A TRACK NODE -- its bytes +0x04..+0x53 have the same shape
 * InitTrackNode (0x0041ce30) fills in -- and it doubles as the LIST SENTINEL
 * for the pieces:
 *
 *     struct CoasterRec {                 // 0x00829ae0
 *         int       state;                // +0x00  0 none, 1 open, 2 closed
 *         TrackNode ring;                 // +0x04  the sentinel node
 *         ...
 *         TrackNode* head_node;           // +0xa8  outermost piece, head end
 *         JointSlot  head_slot;           // +0xac  where it may grow next
 *         TrackNode* tail_node;           // +0xc0  outermost piece, tail end
 *         JointSlot  tail_slot;           // +0xc4
 *         void*      route;               // +0xd8  the ride's route object
 *         CoasterCar cars;                // +0xe4  car list sentinel
 *     };
 *
 * A TRACK PIECE is 0x50 bytes:
 *
 *     struct TrackNode {
 *         int         state;              // +0x00  bit0 raised, bit1 in a ring
 *         u16 x, y;                       // +0x04  its map square (compared
 *                                         //        as ONE dword key)
 *         void*       cls;                // +0x08  its class element
 *         TrackDesc*  desc;               // +0x0c  the class's .rdata record
 *         CoasterRec* owner;              // +0x10  the coaster it belongs to
 *         TrackJoint  jin;                // +0x14  head-side joint
 *         TrackJoint  jout;               // +0x20  tail-side joint
 *         FootPart    part;               // +0x2c  its footprint element
 *     };
 *
 * and a JOINT is 12 bytes -- {height, ?, node} -- initialised to
 * {-1, 0, NULL} by InitTrackJoint (0x0041ce10). The third word of each joint
 * is the neighbour pointer, which is why the walk code steps by +0x1c
 * (jin.node) and +0x28 (jout.node).
 *
 * So the same two link fields carry BOTH shapes the builder needs:
 *   * state 2 -- a CLOSED CIRCUIT: a circular list through jout.node whose
 *     sentinel is `&rec->ring`. FindTrackNodeAt walks it and (faithfully
 *     reproduced) compares the sentinel's own meaningless square first.
 *   * state 1 -- an OPEN ROUTE: two NULL-terminated chains growing out of the
 *     station, one from ring.jin.node and one from ring.jout.node, whose far
 *     ends are cached in head_node / tail_node.
 * RemoveTrackNode (0x0041d7f0) is the state machine: taking a piece out of a
 * closed circuit RE-OPENS it (state drops back to 1 and both neighbours
 * become free ends); taking out an end piece just shortens that chain. Only
 * the two end pieces of an open route can be removed at all
 * (Track_CanRemoveNode, 0x0041d7c0).
 *
 * WHERE THE NEXT PIECE MAY GO -- the JointSlot (0x14 bytes)
 * ---------------------------------------------------------
 *     struct JointSlot { int mask; struct { short x, y; } sq[4]; };
 * Each end of the coaster carries four CANDIDATE squares and a four-bit
 * legality mask. That is what the editor draws ghosts on, what the mouse
 * snaps to, and what the confirm pass matches the cursor against. The two
 * slots sit immediately after their end-pointers in the record, so
 * head_node/head_slot/tail_node/tail_slot pack solid from +0xa8 to +0xd8.
 *
 * THE CLASS DESCRIPTOR (0x38 bytes of .rdata, one per buildable class)
 * -------------------------------------------------------------------
 *     +0x00 raised   +0x04/+0x08 the piece's two end heights
 *     +0x0c {0x0f,0} head-end joint parameters   <- ConnectJointHead
 *     +0x14 {0x0f,0} tail-end joint parameters   <- ConnectJointTail
 *     +0x1c draw   +0x20 build   +0x24 query   +0x28 place   +0x2c remove
 *     +0x30 carries_path
 * TrackNode_Draw / TrackNode_Build / RemoveTrackNode reach the class through
 * these five slots, which is how one node type serves four track classes.
 *
 * THE FOOTPRINT CHAIN (why the castle drags its coaster around)
 * ------------------------------------------------------------
 * The castle's placement footprint is a five-dword record whose last dword
 * points at a linked list of 0x24-byte elements chained through +0x10. The
 * castle's own parts are the static array at 0x0060f938 (== the part table at
 * 0x0060f924 plus 0x14, which is why terminating the list writes
 * g_castle_parts[count].link -- that word IS element[count-1].next).
 * RefreshCastleFootprint (0x00423de0) terminates the list there;
 * RebuildCastleFootprintChain (0x00423e20) instead walks the coaster and
 * appends every piece's own +0x2c element, so a castle being dragged carries
 * its whole track with it.
 *
 * THE EDITOR'S PER-FRAME PASS (Track_Update / TrackH_Update / TrackHP_Update)
 * --------------------------------------------------------------------------
 * The three named *_Update handlers are the same routine specialised per
 * class, and comparing them is what makes the design legible:
 *   * ALL THREE lay out one GHOST cursor per legal candidate square, taken
 *     from the static array of 0x1834-byte cursors at 0x0081ce00, chained on
 *     the preview list at 0x008003f0 and stamped with mode 0x2032, head-slot
 *     candidates first.
 *   * TrackH_Update (SQUARE_TRACK_HEIGHT) then just asks whether the piece
 *     under the mouse fits: yes clears the footprint, no raises cursor error
 *     0x0e.
 *   * TrackHP_Update (SQUARE_TRACK_HEIGHT_PATH) adds a GRADIENT limit: the
 *     two end heights of the fit are summed and a total of two or more steps
 *     is refused with error 0x0b. The path-carrying piece can climb one step.
 *   * Track_Update (flat SQUARE_TRACK) adds the two passes that make laying
 *     track feel magnetic: a SNAP pass that pulls the mouse's map square onto
 *     a candidate within one square in both axes, and a CONFIRM pass that
 *     re-walks the ghosts in creation order and only clears the error if the
 *     snapped square is one of them.
 * The "90" in the name Track_Update90 that castleobj.c invented is a
 * misreading -- 0x004275d0 is the FLAT piece's update, and it is renamed
 * Track_Update here. All three are exact.
 *
 * THE SAVE IMAGE
 * --------------
 * A saved coaster is one self-contained relocatable byte blob:
 *     +0x00 size (its own length)   +0x08 piece count
 *     +0x0c nodes  -> blob+0x20, `count` 8-byte {node, link} pairs
 *     +0x10 cars   +0x14 car array (0x20 bytes each)
 *     +0x18 extra  +0x1c extra array
 * BuildCoasterImage sizes it in 8-byte units, WriteCoasterHeader fills it,
 * EmitCoasterBlob writes it, ReadCoasterBlob reads it back (a zero size is
 * the "no coaster" marker and yields -1, not NULL), and every internal
 * pointer is converted to and from a blob-relative offset by
 * PackCoasterPtr / RelocCoasterPtr. RestoreCoasterCar re-bases the two car
 * timers against the live game clock.
 *
 * =========================================================================
 * THE DRIVING SCHOOL (0x00401ae0, 0x00401c40, 0x00402c10, 0x00413970,
 * 0x00414440) -- assigned to this lane, not part of the coaster
 * =========================================================================
 * castleobj.c's DrivingSchool_TickRiders gates a new car on
 * `5 * CountSchoolCars(sq) < CountSchoolRoadTiles(sq)` -- one car per five
 * road squares -- and then calls NewSchoolCar. A car is 0xd0 bytes, holds
 * its map square, a 16.16 world position, a random horn pitch, one of three
 * random liveries and two countdowns (9000 and 1200 ticks); TickSchoolCars
 * runs both counters down every frame and retires the car when it has left
 * the road network or its life expires, handing the road tile back a user.
 * DrawTrafficLights drives ONE global two-phase light for the whole park
 * (140 ticks green / 40 all-red per axis) and draws four corner lamps on
 * every road tile whose type nibble is 5.
 *
 * =========================================================================
 * VC6 NOTES specific to this file
 * =========================================================================
 *  * The four heap wrappers at 0x004775b0..0x00477410 and Castle_Reset
 *    (0x00425170) carry UNOPTIMISED codegen -- ebp frames, one `add esp,4`
 *    per call, `cmp [g],0` instead of a load and a test -- while everything
 *    around them is /O2. Castle_Reset sits between Castle_Interact and
 *    Castle_Activate, which castleobj.c already builds with optimisation off,
 *    so the original's `#pragma optimize("", off)` region spans all three.
 *  * DrawTrafficLights only matched once the four corner blocks were written
 *    in the original's own order: corner A computes its sprite offset BEFORE
 *    calling GetTileBounds, corners B, C and D call GetTileBounds FIRST. Two
 *    calls cannot be reordered by the compiler, so that ordering is a fact
 *    about the source, not about codegen.
 *  * NewSchoolCar needed its two map-square fields written as ONE 8-byte
 *    struct assignment (`c->start = c->cur;`): the struct copy kills the
 *    load CSE that a field-by-field copy keeps, which is what makes the
 *    original re-read `c->cur.x` for the call that follows.
 *  * The three *_Update handlers sat at 82-86% for a long time on what looked
 *    like unreachable instruction SCHEDULING inside the ghost-cursor loops.
 *    It was not scheduling; it was four separate spelling choices, and each
 *    one is a lever worth remembering:
 *      - RECOMPUTING a global array subscript beats caching a pointer. The
 *        original writes `g_track_cursors[count]` at all eight uses in the
 *        loop body; hoisting it into a `cur` local reorders the whole body
 *        (19 mismatches in TrackH_Update on its own).
 *      - CACHING a pointer beats recomputing, for the JointSlot. The same
 *        loop wants `JointSlot* slot = &g_castle.head_slot;` rather than
 *        `g_castle.head_slot.` at each use -- it decides the order of the two
 *        loop-preheader instructions. So the two rules are opposite, and
 *        which applies is per-object, not general.
 *      - Do NOT name the footprint pointer. `SetEditCursorFootPrint(
 *        &def->footprint)` plus `... .footprint = def->footprint` keeps
 *        `elem->data` alive in eax and gives the original's
 *        `mov eax,[eax+0xc] / lea esi,[eax+0x3c]`; a `Footprint* fp` local
 *        coalesces the pair into `mov esi,[eax+0xc] / add esi,0x3c`.
 *      - Do NOT name the snap-test difference. Spelling the Chebyshev test
 *        inline (`g_mapref.x - slot->sq[i].x >= -1 && ... <= 1`) rather than
 *        through a `d` temporary is what puts the induction pointer in eax
 *        and the difference in ecx, as the original has it (26 mismatches).
 * ========================================================================= */

typedef struct TrackDesc  TrackDesc;
typedef struct TrackNode  TrackNode;
typedef struct CoasterRec CoasterRec;
typedef struct RideDef    RideDef;

/* A track joint: one end of a piece. 12 bytes, initialised {-1, 0, NULL}. */
typedef struct TrackJoint {
    int        height;      /* +0x00  -1 while the end is free */
    int        f04;         /* +0x04 */
    TrackNode* node;        /* +0x08  the piece joined at this end */
} TrackJoint;

/* A footprint element: the map squares a placed object covers, walked as a
 * singly linked list through +0x10. */
typedef struct FootPart {
    unsigned char    pad00[0x10];
    struct FootPart* next;  /* +0x10 */
    unsigned char    pad14[0x10];
} FootPart;                 /* 0x24 */

struct TrackNode {
    int            state;       /* +0x00  kind/flags */
    union {                     /* +0x04  map square, compared as one dword */
        struct { unsigned short x; unsigned short y; } sq;
        unsigned int key;
    } u;
    void*          cls;         /* +0x08  the piece's class element */
    TrackDesc*    desc;         /* +0x0c  the class's piece descriptor */
    CoasterRec*   owner;        /* +0x10  the coaster this piece belongs to */
    TrackJoint    jin;          /* +0x14  head-side joint (.node = prev) */
    TrackJoint    jout;         /* +0x20  tail-side joint (.node = next) */
    FootPart      part;         /* +0x2c  its footprint element */
};

/* ---- externs (address in the trailing comment: tools/callees.py reads it) */
extern void* CRT_calloc(unsigned int n, unsigned int size);     /* 0x004a020e */
extern void  HeapFree_w(void* p);                               /* 0x0049e4d0 */
extern void* g_668fb8;                                          /* 0x00668fb8 */

/* ==========================================================================
 * The block allocator wrappers the coaster (de)serialiser uses. These three
 * live in the game's memory module and are the ONLY functions in this file
 * built WITHOUT optimisation: every one of them keeps a full ebp frame and
 * reloads its argument from [ebp+8], which /O2 never emits for a body this
 * small. Same treatment castleobj.c gives CastleObj_GetInterfaces.
 * ======================================================================== */
#pragma optimize("", off)

/* calloc(1, size) behind a one-argument facade. castleobj.c's call site
 * passes FOUR arguments; the extra three are pushed and never read. */
// FUNCTION: LEGOLAND 0x004775b0
void* AllocZeroed(unsigned int size, int a, int b, int c)
{
    return CRT_calloc(1, size);
}

// FUNCTION: LEGOLAND 0x004775d0
void Free_w(void* p)
{
    HeapFree_w(p);
}

/* An empty hook -- the module keeps the entry point but the body is gone. */
// FUNCTION: LEGOLAND 0x004775f0
void MemScratch_Noop(void)
{
}

/* Release the one-shot scratch block at 0x00668fb8. */
// FUNCTION: LEGOLAND 0x00477410
void FreeMemScratch(void)
{
    if (g_668fb8) {
        HeapFree_w(g_668fb8);
        g_668fb8 = 0;
    }
}

#pragma optimize("", on)

/* ==========================================================================
 * THE TRACK NODE PRIMITIVES (0x0041c…/0x0041d…)
 * ======================================================================== */

/* The per-class piece descriptor (0x38 bytes of .rdata; see castleobj.c).
 * The two 8-byte joint parameter blocks at +0x0c and +0x14 are what the
 * connect helpers hand to the joint builder, one per end of the piece. */
struct TrackDesc {
    int   raised;           /* +0x00 */
    int   h0;               /* +0x04 */
    int   h1;               /* +0x08 */
    int   jp0[2];           /* +0x0c  head-end joint parameters {0x0f, 0} */
    int   jp1[2];           /* +0x14  tail-end joint parameters {0x0f, 0} */
    void (*draw)(TrackNode*);            /* +0x1c */
    void (*build)(TrackNode*, int);      /* +0x20 */
    void* query;                         /* +0x24 */
    void* place;                         /* +0x28 */
    void (*remove)(TrackNode*);          /* +0x2c */
    int   carries_path;     /* +0x30 */
    int   f34;              /* +0x34 */
};

extern void  BuildJoint(TrackNode* n, const int* jp, void* slot);   /* 0x0041d1d0 */
extern int   g_track_square_count;                                  /* 0x004d8268 */
extern int   g_track_place_enabled;                                 /* 0x004b55f4 */
#ifndef LEGOLAND_PORTABLE
extern void  AddBasicObject(void* elem, void* pos);                 /* 0x0045efe0 */
#else
extern void AddBasicObject(void* ll_obj, void* ll_pos, void* ll_ctx);                 /* 0x0045efe0 */
#define AddBasicObject(_a1, _a2) AddBasicObject((_a1), (_a2), 0)
#endif
/* A map square packed into two bytes and passed BY VALUE (VC6 forwards it as
 * one dword and masks the halves out again). */
typedef struct MapPos { unsigned char x; unsigned char y; } MapPos;

extern void  StandardRemoveObject(void* elem, MapPos sq, void* cursor);   /* 0x0045f220 */

/* Reset one end of a piece to "free": no neighbour, no height. */
// FUNCTION: LEGOLAND 0x0041ce10
void InitTrackJoint(TrackJoint* j)
{
    j->node = 0;
    j->height = -1;
    j->f04 = 0;
}

/* piece->desc->draw(piece) -- the per-class render pass of one track piece. */
// FUNCTION: LEGOLAND 0x0041cfc0
void TrackNode_Draw(TrackNode* n)
{
    n->desc->draw(n);
}

/* piece->desc->build(piece, mode) -- rebuild the piece's geometry. */
// FUNCTION: LEGOLAND 0x0041cfd0
void TrackNode_Build(TrackNode* n, int mode)
{
    n->desc->build(n, mode);
}

/* Wire the HEAD end of `n` into `slot`, using the class's head joint
 * parameters (desc +0x0c). */
// FUNCTION: LEGOLAND 0x0041d170
void ConnectJointHead(TrackNode* n, void* slot)
{
    BuildJoint(n, n->desc->jp0, slot);
}

/* Same for the TAIL end (desc +0x14). */
// FUNCTION: LEGOLAND 0x0041d190
void ConnectJointTail(TrackNode* n, void* slot)
{
    BuildJoint(n, n->desc->jp1, slot);
}

/* Clear a joint slot's height word. */
// FUNCTION: LEGOLAND 0x0041d1b0
void ClearJointHeight(TrackJoint* j)
{
    j->height = 0;
}

/* An empty hook, called on both end slots after a piece is unlinked. */
// FUNCTION: LEGOLAND 0x0041d1c0
void ReleaseJointSlot(void* slot)
{
}

/* The count of map squares the castle + its coaster currently occupy. */
// FUNCTION: LEGOLAND 0x0041d6c0
void SetTrackSquareCount(int n)
{
    g_track_square_count = n;
}

// FUNCTION: LEGOLAND 0x0041d6f0
int GetTrackSquareCount(void)
{
    return g_track_square_count;
}

/* The "actually touch the map" switch: while it is clear the track editor
 * runs its place/remove passes without writing object cells. */
// FUNCTION: LEGOLAND 0x0041ed80
void SetTrackPlaceEnabled(int on)
{
    g_track_place_enabled = on;
}

// FUNCTION: LEGOLAND 0x0041ed90
void TrackAddBasicObject(void* elem, void* pos)
{
    if (g_track_place_enabled)
        AddBasicObject(elem, pos);
}

// FUNCTION: LEGOLAND 0x0041edb0
void TrackRemoveObject(void* elem, MapPos sq, void* cursor)
{
    if (g_track_place_enabled)
        StandardRemoveObject(elem, sq, cursor);
}

/* ==========================================================================
 * THE CASTLE / COASTER RECORD  (g_castle @ 0x00829ae0)
 * --------------------------------------------------------------------------
 * The record is ITSELF a TrackNode -- the same +0x00/+0x04/+0x0c/+0x14/+0x20
 * layout InitTrackNode fills in -- with the coaster's own state hung off the
 * end of it. Its two joints double as the ring sentinel: a closed circuit is
 * a circular list through jin/jout whose sentinel is `&g_castle.node.jin`
 * (0x00829ae4), and an unfinished route is TWO null-terminated chains, one
 * growing from each end of the station.
 * ======================================================================== */

/* One END of the coaster as the track editor sees it: a 4-bit mask of which
 * of four candidate squares the next piece may go on, and those four squares.
 * 0x14 bytes; the record holds one per end, immediately after each end's
 * outermost-piece pointer. */
typedef struct JointSlot {
    int   mask;                 /* +0x00  bit i = candidate i is legal */
    struct { short x; short y; } sq[4];   /* +0x04 */
} JointSlot;

typedef struct CoasterCar CoasterCar;
struct CoasterCar {
    int         live;           /* +0x00  1 once running */
    unsigned char pad04[0x0c];
    CoasterCar* prev;           /* +0x10 */
    CoasterCar* next;           /* +0x14 */
    unsigned char pad18[4];
    int         born;           /* +0x1c  GetGameTimer() stamp */
};

struct CoasterRec {
    int           state;        /* +0x00  0 none, 1 open route, 2 closed circuit */
    TrackNode     ring;         /* +0x04  the list SENTINEL, itself shaped as a
                                 *        TrackNode: ring.jin.node  (+0x20) is
                                 *        the head-side chain, ring.jout.node
                                 *        (+0x2c) the tail-side chain. */
    unsigned char pad54[0xa8 - 0x54];
    TrackNode*    head_node;    /* +0xa8  outermost piece, head side  (0x829b88) */
    JointSlot     head_slot;    /* +0xac  where it can grow next     (0x829b8c) */
    TrackNode*    tail_node;    /* +0xc0  outermost piece, tail side  (0x829ba0) */
    JointSlot     tail_slot;    /* +0xc4                              (0x829ba4) */
    void*         d8;           /* +0xd8  the coaster's route object */
    unsigned char paddc[4];
    int           e0;           /* +0xe0 */
    CoasterCar    cars;         /* +0xe4  car list sentinel (prev @0xf4,
                                 *        next @0xf8) */
};

/* Only the offsets used below are named; the record is reached both as this
 * struct and as loose globals (which is what the codegen asks for). */
extern CoasterRec  g_castle;                    /* 0x00829ae0 */
extern int         g_castle_state;              /* 0x00829ae0 */
extern TrackNode*  g_castle_head_node;          /* 0x00829b88  = rec +0xa8 */
extern TrackNode*  g_castle_tail_node;          /* 0x00829ba0  = rec +0xc0 */

/* The castle's footprint: five dwords, the last a pointer to the head of the
 * element list. */
typedef struct Footprint {
    int       v[4];
    FootPart* parts;            /* +0x10 */
} Footprint;

/* The castle's static part table. Element i of the FOOTPRINT list lives at
 * &g_castle_parts[i] + 0x14, so element i's `next` pointer is the +0x00 word
 * of table entry i+1 -- which is why terminating the list writes
 * g_castle_parts[count].link. */
typedef struct CastlePart {
    FootPart*     link;         /* +0x00  = element[i-1].next */
    short         dx;           /* +0x04  offset from the castle square */
    short         dy;           /* +0x06 */
    int           index;        /* +0x08 */
    int           f0c;
    int           f10;
    unsigned char pad14[0x10];
} CastlePart;                   /* 0x24 */

extern CastlePart g_castle_parts[];             /* 0x0060f924 */
extern FootPart   g_castle_foot_elems[];        /* 0x0060f938  = parts[0]+0x14 */
extern int        g_castle_part_count;          /* 0x00610a08 */
extern Footprint  g_castle_footprint;           /* 0x00829a80 */

typedef struct RideElem {
    char*    name;
    char*    image;
    unsigned int flags;
    RideDef* data;
} RideElem;

typedef struct SpriteRec {
    unsigned char pad00[0x10];
    unsigned int  flags;        /* +0x10  0x2000 = the class's own sprite */
} SpriteRec;

struct RideDef {
    unsigned char pad00[0x1c];
    unsigned int  flags1c;      /* +0x1c */
    unsigned char pad20[0x3c - 0x20];
    Footprint     footprint;    /* +0x3c */
    unsigned char pad50[0x14];
    SpriteRec*    sprite;       /* +0x64 */
};

extern RideDef* g_castle_def;                   /* 0x00829bf8 */

/* ==========================================================================
 * The six-row per-class dispatch table castleobj.c documents at 0x0082ad20.
 * ======================================================================== */
typedef struct MapRefI { int x; int y; } MapRefI;

typedef struct CtIface {
    RideElem* elem;             /* +0x00 */
    void*     tick;             /* +0x04 */
    void*     update;           /* +0x08 */
    void    (*add)(RideElem*, MapRefI*);   /* +0x0c */
    void*     update2;          /* +0x10 */
    void*     remove;           /* +0x14 */
} CtIface;

extern CtIface g_ct_iface[6];                   /* 0x0082ad20 */

/* Place one piece of class `cls` on square `sq`, widening the packed 16-bit
 * square into the two-int MapRef the class handlers take. */
// FUNCTION: LEGOLAND 0x0041eca0
void CallTrackClassAdd(int cls, const short* sq)
{
    MapRefI pos;

    pos.x = sq[0];
    pos.y = sq[1];
    g_ct_iface[cls].add(g_ct_iface[cls].elem, &pos);
}

/* A fresh, unplaced, unconnected track piece. */
// FUNCTION: LEGOLAND 0x0041ce30
void InitTrackNode(TrackNode* n)
{
    n->state = 0;
    n->u.sq.x = 0;
    n->u.sq.y = 0;
    n->desc = 0;
    n->owner = 0;
    InitTrackJoint(&n->jout);
    InitTrackJoint(&n->jin);
}

/* Only the two END pieces of an open route can be taken away again; once the
 * circuit is closed (state 2) any piece may go, because removing one just
 * re-opens the ring. */
// FUNCTION: LEGOLAND 0x0041d7c0
int Track_CanRemoveNode(TrackNode* n)
{
    if (g_castle_state == 2 || n == g_castle_head_node || n == g_castle_tail_node)
        return 1;
    return 0;
}

/* Re-read the castle class's footprint and terminate the element list right
 * after the castle's own parts (no track pieces attached). */
// FUNCTION: LEGOLAND 0x00423de0
void RefreshCastleFootprint(void)
{
    int n = g_castle_part_count;

    g_castle_footprint = g_castle_def->footprint;
    g_castle_footprint.parts = g_castle_foot_elems;
    g_castle_parts[n].link = 0;
}

/* ---- the coaster's route object and its car list ----------------------- */
extern void* CreateCoasterRoute(CoasterRec* r);                 /* 0x0041e570 */
extern void  DestroyCoasterRoute(void** slot);                   /* 0x0041e5d0 */
extern int   Route_IsClosed(void* route);                   /* 0x0041e4a0 */
extern void  Route_Open(void* route);                   /* 0x0041e4f0 */
extern void  Route_AddNode(void* route, TrackNode* n);     /* 0x0041e3e0 */
extern int   AnyCoasterRegionFullyInside(void);                          /* 0x00426650 */
extern void  Coaster_StartQueuedCars(CoasterRec* r);                 /* 0x00424d80 */
extern void  KillCoasterCar(CoasterCar* c);             /* 0x00421980 */

/* Build the coaster's route object from the record. */
// FUNCTION: LEGOLAND 0x004249e0
void Coaster_CreateRoute(CoasterRec* r)
{
    r->d8 = CreateCoasterRoute(r);
    r->e0 = 0;
}

// FUNCTION: LEGOLAND 0x00424a00
void Coaster_DestroyRoute(CoasterRec* r)
{
    DestroyCoasterRoute(&r->d8);
}

/* Append one placed piece to its coaster's route, if the route accepts. */
// FUNCTION: LEGOLAND 0x00424a20
void Coaster_AddNodeToRoute(TrackNode* n)
{
    void* route = n->owner->d8;

    if (AnyCoasterRegionFullyInside())
        Route_AddNode(route, n);
}

/* Once the route is complete, start it and run the record's follow-up. */
// FUNCTION: LEGOLAND 0x00424ab0
void Coaster_StartIfComplete(CoasterRec* r)
{
    if (Route_IsClosed(r->d8)) {
        Route_Open(r->d8);
        Coaster_StartQueuedCars(r);
    }
}

/* The car list is an intrusive circular list whose sentinel node lives INSIDE
 * the record at +0xe4, so the sentinel's prev/next fields are the record's
 * own +0xf4/+0xf8. */
// FUNCTION: LEGOLAND 0x00424b10
void Coaster_InitCarList(CoasterRec* r)
{
    r->cars.next = &r->cars;
    r->cars.prev = &r->cars;
}

// FUNCTION: LEGOLAND 0x00424df0
void Coaster_KillAllCars(CoasterRec* r)
{
    while (r->cars.next != &r->cars)
        KillCoasterCar(r->cars.next);
}

/* ---- the coaster save blob --------------------------------------------- */
extern int SaveGameWrite(const void* buf, unsigned int n);          /* 0x0047d760 */

/* On LOAD: turn a stored offset back into a live pointer. */
// FUNCTION: LEGOLAND 0x00426ba0
void RelocCoasterPtr(void** slot, int base)
{
    if (*slot)
        *slot = (char*)*slot + base;
}

/* On SAVE: turn a live pointer into an offset from the blob base. */
// FUNCTION: LEGOLAND 0x00426bc0
void PackCoasterPtr(void** slot, int base)
{
    if (*slot)
        *slot = (char*)*slot - base;
}

/* The blob's first dword is its own byte length. */
// FUNCTION: LEGOLAND 0x00427220
void EmitCoasterBlob(unsigned int* blob)
{
    SaveGameWrite(blob, blob[0]);
}

/* ---- map square -> world position -------------------------------------- */
typedef struct Vec3f { float x; float y; float z; } Vec3f;

/* One map square is 20.0 world units; one height step is -6.0 (up is -z). */
// FUNCTION: LEGOLAND 0x00425cb0
void MapSquareToWorld(const short* sq, float h, Vec3f* out)
{
    out->x = sq[0] * 20.0f;
    out->y = sq[1] * 20.0f;
    out->z = h * -6.0f;
}

/* ---- DRIVING SCHOOL occupancy (0x00401c40 / 0x00413970) ----------------
 * castleobj.c's DrivingSchool_TickRiders gates a new car on
 * `5 * cars < roadtiles`, i.e. one car per five road squares. */
typedef struct CarPos { int x; int y; } CarPos;

typedef struct SchoolCar {
    struct SchoolCar* next;     /* +0x00 */
    unsigned short    school;   /* +0x04  the school's packed map square */
    unsigned char     pad06[0x0a];
    int               wx;       /* +0x10  world x, 16.16 */
    int               wy;       /* +0x14  world y, 16.16 */
    CarPos            cur;      /* +0x18  map square */
    CarPos            start;    /* +0x20  the square it started from */
    unsigned char     pad28[0xb8 - 0x28];
    unsigned char     b8;       /* +0xb8 */
    unsigned char     b9;       /* +0xb9 */
    unsigned char     ba;       /* +0xba */
    unsigned char     bb;       /* +0xbb */
    unsigned short    bc;       /* +0xbc */
    unsigned short    t_life;   /* +0xbe  lifetime countdown, 9000 ticks */
    unsigned short    t_horn;   /* +0xc0  secondary countdown, 1200 ticks */
    unsigned char     on_road;  /* +0xc2  1 while the car is still on a road */
    unsigned char     dir;      /* +0xc3  1..3, chosen at random */
    unsigned char     c4;       /* +0xc4 */
    unsigned char     c5;       /* +0xc5 */
    unsigned short    c6;       /* +0xc6 */
    unsigned short    c8;       /* +0xc8 */
    unsigned char     padca[2];
    void*             bloke;    /* +0xcc */
} SchoolCar;                    /* 0xd0 */

typedef struct RoadTile {
    struct RoadTile* next;      /* +0x00 */
    unsigned char    pad04[4];
    unsigned short   school;    /* +0x08 */
    unsigned char    pad0a[2];
    int              x;         /* +0x0c  map square */
    int              y;         /* +0x10 */
    unsigned char    type;      /* +0x14  low nibble; 5 = a crossroads */
    unsigned char    pad15[7];
    unsigned char    users;     /* +0x1c  cars currently on this tile */
} RoadTile;

extern SchoolCar* g_school_cars;                /* 0x004c10d4 */
extern RoadTile*  g_road_tiles;                 /* 0x004cbeac */

// FUNCTION: LEGOLAND 0x00401c40
int CountSchoolCars(unsigned short school)
{
    SchoolCar* p = g_school_cars;
    int n = 0;

    while (p) {
        if (p->school == school)
            n++;
        p = p->next;
    }
    return n;
}

// FUNCTION: LEGOLAND 0x00413970
int CountSchoolRoadTiles(unsigned short school)
{
    RoadTile* p = g_road_tiles;
    int n = 0;

    while (p) {
        if (p->school == school)
            n++;
        p = p->next;
    }
    return n;
}

/* ---- riders on the coaster ---------------------------------------------- */
typedef struct Bloke {
    unsigned char  pad00[0x62];
    unsigned short flags62;     /* +0x62  8 = using this ride */
} Bloke;

typedef struct RiderNode {
    struct RiderNode* next;     /* +0x00 */
    unsigned char     pad04[4];
    Bloke*            bloke;    /* +0x08 */
} RiderNode;

typedef struct ClassDef {
    unsigned char pad00[0xcc];
    RiderNode*    riders;       /* +0xcc */
} ClassDef;

extern ClassDef* CastleClassDef(int index);                     /* 0x0041ec00 */
extern void RemoveBlokeFromRide(ClassDef* def, RiderNode* r);   /* 0x0048a100 */
extern int  GetGameTimer(void);                                 /* 0x00499430 */
extern CoasterCar* CoasterCar_Create(void* a);                         /* 0x004215d0 */
extern void*       BuildBlokeAppearance(Bloke* b);                        /* 0x00421890 */

/* Throw every rider off the coaster (the castle class, row 0). */
// FUNCTION: LEGOLAND 0x00424e20
void Coaster_EjectAllRiders(void)
{
    ClassDef*  def = CastleClassDef(0);
    RiderNode* r = def->riders;

    while (r) {
        Bloke*     b = r->bloke;
        RiderNode* next = r->next;

        b->flags62 &= ~8;
        RemoveBlokeFromRide(def, r);
        r = next;
    }
}

/* Append a car for `b` to the tail of the coaster's car list. */
// FUNCTION: LEGOLAND 0x00421930
CoasterCar* Coaster_AddCar(Bloke* b, CoasterRec* r)
{
    CoasterCar* c = CoasterCar_Create(BuildBlokeAppearance(b));

    r->cars.prev->next = c;
    c->prev = r->cars.prev;
    c->next = &r->cars;
    r->cars.prev = c;
    c->born = GetGameTimer();
    c->live = 1;
    return c;
}

/* ---- the {node, link} pair array inside a saved coaster blob ------------ */
typedef struct CoasterNodeRef {
    void* node;                 /* +0x00 */
    void* link;                 /* +0x04 */
} CoasterNodeRef;

extern void LoadCoasterCar(CoasterNodeRef* p, void* ctx);              /* 0x00427070 */

// FUNCTION: LEGOLAND 0x00427100
void WalkCoasterNodeRefs(CoasterNodeRef* p, int count, void* ctx)
{
    int i;

    for (i = 0; i < count; i++)
        LoadCoasterCar(p++, ctx);
}

/* ==========================================================================
 * Finding a piece by map square.
 * A CLOSED circuit is a circular list through jout.node whose sentinel is the
 * record's own embedded node; an OPEN route is a null-terminated chain that
 * starts at the record's outermost tail piece. Note that the closed walk
 * begins AT the sentinel, so the sentinel's own (meaningless) square is
 * compared first -- reproduced as the original has it.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x0041d060
TrackNode* FindTrackNodeAt(CoasterRec* rec, const unsigned int* key)
{
    TrackNode* n;

    if (rec->state & 2) {
        n = &rec->ring;
        while (n->u.key != *key) {
            n = n->jout.node;
            if (n == &rec->ring)
                return 0;
        }
        return n;
    }

    n = rec->tail_node;
    do {
        if (n->u.key == *key)
            return n;
        n = n->jout.node;
    } while (n);
    return 0;
}

extern int TrackNodeCoversSquare(TrackNode* n, void* p);                   /* 0x0041d0b0 */

/* The same walk, filtered on the piece's class AND a per-piece predicate.
 * Unlike FindTrackNodeAt this one tests at the top of the loop, so the
 * sentinel is examined by the predicate too. */
// FUNCTION: LEGOLAND 0x0041d100
TrackNode* FindTrackNodeOfClass(CoasterRec* rec, void* cls, void* p)
{
    TrackNode* n;

    if (rec->state & 2) {
        n = &rec->ring;
        for (;;) {
            if (n->cls == cls && TrackNodeCoversSquare(n, p))
                return n;
            n = n->jout.node;
            if (n == &rec->ring)
                return 0;
        }
    }

    n = rec->tail_node;
    for (;;) {
        if (n->cls == cls && TrackNodeCoversSquare(n, p))
            return n;
        n = n->jout.node;
        if (!n)
            return 0;
    }
}

/* ==========================================================================
 * Building and tearing down the whole coaster.
 * ======================================================================== */
extern void RemoveTrackNode(TrackNode* n);              /* 0x0041d7f0 */
#ifndef LEGOLAND_PORTABLE
extern void AddTrackSquareCount(int d);                 /* 0x0041d6d0 */
#else
extern int AddTrackSquareCount(int d);                 /* 0x0041d6d0 */
#endif
extern RideElem* g_dummy_elem;                          /* 0x00829c00 */

/* Take every piece off the coaster. A closed circuit is one ring walk; an
 * open route is two: outward along each chain from the station. */
// FUNCTION: LEGOLAND 0x00423ec0
void RemoveAllTrackNodes(CoasterRec* rec)
{
    TrackNode* n;
    TrackNode* next;

    if (g_castle_state == 2) {
        n = g_castle.ring.jout.node;
        while (n != &g_castle.ring) {
            next = n->jout.node;
            RemoveTrackNode(n);
            n = next;
        }
        return;
    }

    n = g_castle.tail_node;
    while (n != &rec->ring) {
        next = n->jout.node;
        RemoveTrackNode(n);
        n = next;
    }
    n = g_castle.head_node;
    while (n != &rec->ring) {
        next = n->jin.node;
        RemoveTrackNode(n);
        n = next;
    }
}

/* Rebuild the castle's placement footprint so it covers the castle's own
 * parts AND every track piece currently attached: element[count-1] of the
 * static table is re-pointed at the first piece's element and the pieces are
 * chained on through FootPart.next. */
// FUNCTION: LEGOLAND 0x00423e20
void RebuildCastleFootprintChain(void)
{
    FootPart*  tail;
    TrackNode* p;
    int        n;

    g_castle_footprint = g_castle_def->footprint;
    n = g_castle_part_count;
    g_castle_parts[n].link = 0;
    g_castle_footprint.parts = g_castle_foot_elems;
    tail = &g_castle_foot_elems[n - 1];

    if (g_castle.state == 2) {
        p = g_castle.ring.jout.node;
        while (p != &g_castle.ring) {
            tail->next = &p->part;
            tail = &p->part;
            p = p->jout.node;
        }
        tail->next = 0;
        return;
    }

    p = g_castle.ring.jout.node;
    while (p) {
        tail->next = &p->part;
        tail = &p->part;
        p = p->jout.node;
    }
    p = g_castle.ring.jin.node;
    while (p) {
        tail->next = &p->part;
        tail = &p->part;
        p = p->jin.node;
    }
    tail->next = 0;
}

/* Drop a CASTLE_DUMMY object on every square the castle covers. */
// FUNCTION: LEGOLAND 0x004245b0
void PlaceCastleDummies(const short* sq)
{
    MapRefI pos;
    int i;

    for (i = 0; i < g_castle_part_count; i++) {
        pos.x = g_castle_parts[i].dx + sq[0];
        pos.y = g_castle_parts[i].dy + sq[1];
        TrackAddBasicObject(g_dummy_elem, &pos);
    }
    AddTrackSquareCount(g_castle_part_count);
}

/* ==========================================================================
 * Fitting a piece: can this class of track sit on this square, and how?
 * The answer goes into ONE shared result block (0x004d8250) that the caller
 * reads back; the function returns its address.
 *   flags bit0  the head-side joint has a partner
 *   flags bit1  the tail-side joint has a partner
 * ======================================================================== */
typedef struct TrackFit {
    int           flags;        /* +0x00 */
    int           f04;          /* +0x04  the joined piece / geometry key */
    int           a;            /* +0x08  head partner, -1 = none */
    unsigned char pad0c[4];
    int           b;            /* +0x10  tail partner, -1 = none */
} TrackFit;

extern TrackFit g_track_fit;                                    /* 0x004d8250 */
extern int  TrackFitCheckChain(TrackNode* n, void* p, TrackFit* out);   /* 0x0041d2e0 */
extern int  TrackFitCheckSpan(TrackNode* n, void* p, TrackFit* out);   /* 0x0041d350 */
extern void* TrackCreateChainPiece(void* a, TrackNode* n, void* p, TrackFit* f); /* 0x0041d630 */
extern void* TrackCreateSpanPiece(void* a, TrackNode* n, void* p, TrackFit* f); /* 0x0041d5b0 */

/* NOTE the duplicated `flags = 0`: it is stored once before the test and
 * AGAIN on the failure path, so the failure path stores zero twice. That is
 * what the original does -- reproduced, not tidied. */
// FUNCTION: LEGOLAND 0x0041d3b0
TrackFit* TrackFitCheck(TrackNode* n, void* p)
{
    int ok;

    if (n->state & 1)
        ok = TrackFitCheckChain(n, p, &g_track_fit);
    else
        ok = TrackFitCheckSpan(n, p, &g_track_fit);

    g_track_fit.flags = 0;
    if (ok) {
        if (g_track_fit.a != -1)
            g_track_fit.flags = 1;
        if (g_track_fit.b != -1)
            g_track_fit.flags |= 2;
        return &g_track_fit;
    }
    g_track_fit.flags = 0;
    return &g_track_fit;
}

/* Place one piece if it fits, counting the square it eats. */
// FUNCTION: LEGOLAND 0x0041d700
void* TrackPlaceIfFits(void* a, TrackNode* n, void* p)
{
    if (TrackFitCheck(n, p)->flags != 0) {
        AddTrackSquareCount(1);
        if (n->state & 1)
            return TrackCreateChainPiece(a, n, p, &g_track_fit);
        return TrackCreateSpanPiece(a, n, p, &g_track_fit);
    }
    return 0;
}

/* ---- per-piece 3D placement -------------------------------------------- */
extern void  SetupTrackDrawView(void);                                  /* 0x00425bd0 */
extern void  DrawTrackEnd_Fetch(void* a, void* b, int c);               /* 0x00428e70 */
extern void  Coaster3D_DrawPieceSupport(void* a, void* b, int c);               /* 0x00429150 */
extern void  DrawTrackEnd_Cached(void* a, void* b, int c);               /* 0x00428ec0 */
extern int   TrackNodeSlopeCode(TrackNode* n);                          /* 0x0041ce60 */
extern void* GetTrackNodeWorldPos(TrackNode* n, Vec3f* out);              /* 0x0041cff0 */
extern int   g_611710[];                                        /* 0x00611710 */

/* Two orderings of the same three passes; `mode` picks which end is done
 * first (the piece's two ends run 0-then-2 or 2-then-0). */
// FUNCTION: LEGOLAND 0x004294f0
void DrawTrackPiece3D(void* a, void* b, int mode, int c)
{
    SetupTrackDrawView();
    if (mode == 1) {
        DrawTrackEnd_Fetch(a, b, 0);
        Coaster3D_DrawPieceSupport(a, b, c);
        DrawTrackEnd_Cached(a, b, 2);
    } else {
        DrawTrackEnd_Fetch(a, b, 2);
        Coaster3D_DrawPieceSupport(a, b, c);
        DrawTrackEnd_Cached(a, b, 0);
    }
}

// FUNCTION: LEGOLAND 0x00428700
void DrawTrackNode(TrackNode* n)
{
    Vec3f v;
    int   kind = TrackNodeSlopeCode(n);
    void* obj = GetTrackNodeWorldPos(n, &v);

    if (n->state & 6)
        DrawTrackPiece3D(obj, &v, g_611710[kind], 0);
    else
        DrawTrackPiece3D(obj, &v, g_611710[kind], 1);
}

/* ==========================================================================
 * The coaster SAVE image.
 * SaveRollerCoaster (castleobj.c) sizes the blob with BuildCoasterImage,
 * allocates it, fills it with WriteCoasterHeader and writes it out. Every
 * pointer inside the blob is stored as an OFFSET from the blob base (see
 * PackCoasterPtr / RelocCoasterPtr), so the whole coaster is one relocatable
 * byte image:
 *      +0x00  size          the blob's own byte length
 *      +0x08  count         track pieces
 *      +0x0c  nodes         -> blob+0x20, `count` {node, link} pairs
 *      +0x10  cars          0 or 1
 *      +0x14  cars ptr      -> after the node array, 0x20 bytes each
 *      +0x18  extra         extra-record count
 *      +0x1c  extra ptr     -> after the car array
 * ======================================================================== */
typedef struct CoasterImage {
    CoasterRec*  rec;           /* +0x00 */
    unsigned int size;          /* +0x04 */
    int          count;         /* +0x08 */
    int          cars;          /* +0x0c */
    int          extra;         /* +0x10 */
} CoasterImage;

typedef struct CoasterBlob {
    unsigned int    size;       /* +0x00 */
    unsigned char   pad04[4];
    int             count;      /* +0x08 */
    CoasterNodeRef* nodes;      /* +0x0c */
    int             cars;       /* +0x10 */
    void*           cars_ptr;   /* +0x14 */
    int             extra;      /* +0x18 */
    void*           extra_ptr;  /* +0x1c */
} CoasterBlob;                  /* header is 0x20 bytes */

extern int  CountCoasterNodes(CoasterRec* r);                          /* 0x00427150 */
extern int  CountCoasterCars(CoasterRec* r);                          /* 0x00427130 */
extern void WriteCoasterNodes(CoasterRec* r, CoasterNodeRef* out);     /* 0x00427190 */
extern void SaveCoasterRouteState(void* route, void* out);                 /* 0x00426f40 */
extern void SaveCoasterCars(CoasterRec* r, void* out);               /* 0x004270c0 */
extern int  SaveGameRead(void* buf, unsigned int n);            /* 0x0047d730 */

/* Work out how big the blob must be: a 4-entry header plus one 8-byte slot
 * per piece, 4 more if a car is running, plus the extra records -- all
 * measured in 8-byte units. */
// FUNCTION: LEGOLAND 0x00426d80
void BuildCoasterImage(CoasterRec* rec, CoasterImage* img)
{
    img->rec = rec;
    img->count = CountCoasterNodes(rec);
    if (Route_IsClosed(rec->d8))
        img->cars = 1;
    else
        img->cars = 0;
    img->extra = CountCoasterCars(rec);
    img->size = (img->count + img->cars * 4 + 4 + img->extra) * 8;
}

// FUNCTION: LEGOLAND 0x00426de0
void WriteCoasterHeader(CoasterImage* img, CoasterBlob* blob)
{
    char* p;

    blob->size = img->size;
    blob->count = img->count;
    blob->nodes = (CoasterNodeRef*)(blob + 1);
    WriteCoasterNodes(img->rec, blob->nodes);
    p = (char*)blob->nodes + blob->count * 8;
    if (Route_IsClosed(img->rec->d8)) {
        void* route = img->rec->d8;

        blob->cars = 1;
        blob->cars_ptr = p;
        SaveCoasterRouteState(route, p);
        p = (char*)blob->cars_ptr + blob->cars * 32;
    } else {
        blob->cars_ptr = 0;
    }
    blob->extra = img->extra;
    if (blob->extra) {
        blob->extra_ptr = p;
        SaveCoasterCars(img->rec, p);
    } else {
        blob->extra_ptr = 0;
    }
}

/* Read a blob back: the size comes first, the rest follows it. A zero size
 * is the "no coaster saved" marker and gives -1, not NULL. */
// FUNCTION: LEGOLAND 0x00427240
CoasterBlob* ReadCoasterBlob(void)
{
    unsigned int size;
    CoasterBlob* blob;

    SaveGameRead(&size, 4);
    if (size == 0)
        return (CoasterBlob*)-1;
    blob = (CoasterBlob*)AllocZeroed(size, 0, 0, 0);
    if (!blob)
        return 0;
    blob->size = size;
    SaveGameRead((char*)blob + 4, size - 4);
    return blob;
}

/* ---- restoring one saved car onto its route ---------------------------- */
typedef struct CoasterCarSave {
    int           f00;          /* +0x00 */
    int           f04;          /* +0x04 */
    int           f08;          /* +0x08 */
    int           elapsed;      /* +0x0c  ms already run when saved */
    int           remaining;    /* +0x10  ms still to run */
    unsigned char sub[0x0c];    /* +0x14 */
} CoasterCarSave;               /* 0x20 */

typedef struct CoasterRoute {
    int           f00;          /* +0x00 */
    int           started;      /* +0x04  now - elapsed */
    int           deadline;     /* +0x08  now + remaining */
    unsigned char sub[0x0c];    /* +0x0c */
} CoasterRoute;

extern void RestoreRoutePos(const unsigned char* src, unsigned char* dst, CoasterRec* r); /* 0x00426f10 */
#ifndef LEGOLAND_PORTABLE
extern void PositionRouteCars(CoasterRoute* rt, int a, unsigned char* sub);                 /* 0x0041da10 */
extern void Route_SetSpeed(CoasterRoute* rt, int a);                                     /* 0x0041dad0 */
#else
/* schoolcar.c defines both of these with a `float` second parameter; this
 * file spells it `int` because the value comes straight out of the save
 * record as a raw dword and the original pushes it without touching the FPU.
 * On x86 the two prototypes push identical bytes; on wasm32 f32 and i32 are
 * different function types. LL_ASFLT re-reads the save-record dword as the
 * float it is, so the callee sees exactly the original's bits -- a numeric
 * `(float)` conversion here would pass a completely different value. */
extern void PositionRouteCars(CoasterRoute* rt, float a, unsigned char* sub);               /* 0x0041da10 */
extern void Route_SetSpeed(CoasterRoute* rt, float a);                                   /* 0x0041dad0 */
#define PositionRouteCars(_rt, _a, _sub) PositionRouteCars((_rt), LL_ASFLT(_a), (_sub))
#define Route_SetSpeed(_rt, _a)          Route_SetSpeed((_rt), LL_ASFLT(_a))
#endif

/* The two timers are stored RELATIVE to the save point and re-based against
 * the live game clock on load. */
// FUNCTION: LEGOLAND 0x00426f90
void RestoreCoasterCar(const CoasterCarSave* src, CoasterRec* rec)
{
    CoasterRoute* rt = (CoasterRoute*)rec->d8;

    rt->f00 = src->f00;
    RestoreRoutePos(src->sub, rt->sub, rec);
    PositionRouteCars(rt, src->f04, rt->sub);
    Route_SetSpeed(rt, src->f08);
    rt->started = GetGameTimer() - src->elapsed;
    rt->deadline = GetGameTimer() + src->remaining;
}

/* ---- creating a track class ------------------------------------------- */
extern void*     g_829c08;                                      /* 0x00829c08 */
extern void*     g_829980;                                      /* 0x00829980 */
extern int   LLIDB_FindElement(const char* name, void** out, unsigned int* idx); /* 0x0047b330 */
extern void* LLIDB_LoadData(void* elem);                        /* 0x0047d3a0 */
extern void  Coaster3D_BuildPieceGeometry(void);                                  /* 0x004284d0 */
extern void  InitTrackDrawModes(void);                                  /* 0x00428750 */

/* SQUARE_TRACK's create hook: claim the class sprite, make sure the basic
 * tile set is loaded, then build the piece geometry. The out-parameter of
 * LLIDB_FindElement is homed in the dead argument slot. */
// FUNCTION: LEGOLAND 0x00427aa0
void Track_Create(RideElem* elem)
{
    void*    found;
    RideDef* def = elem->data;

    g_829c08 = def;
    def->sprite->flags |= 0x2000;
    if (LLIDB_FindElement("BASIC TILES 1", &found, 0) == 0)
        g_829980 = LLIDB_LoadData(found);
    Coaster3D_BuildPieceGeometry();
    InitTrackDrawModes();
}

/* ==========================================================================
 * CASTLE OBJ create / reset.
 * ======================================================================== */
extern void* g_829abc;                                  /* 0x00829abc */
extern void* g_castle_sprite;                           /* 0x00829c04 */
extern int   g_610a04;                                  /* 0x00610a04  castle placed */
extern void* LoadSprite(const char* name, int mode);    /* 0x00497ab0 */
#ifndef LEGOLAND_PORTABLE
extern void  MemScratchInit(void);                          /* 0x00477400 */
#else
extern int MemScratchInit(void);                          /* 0x00477400 */
#endif
extern void  Coaster3D_ResetScene(void);                          /* 0x00425a50 */
extern void  CarPoolInit(void);                          /* 0x00421470 */
extern void  LoadCoasterData(void);                          /* 0x00420440 */
extern void  CoasterGeomInit(void);                          /* 0x00423740 */
extern void  CarClassTablesInit(void);                          /* 0x00422210 */
extern void  CoasterSceneInit(void);                          /* 0x00428b70 */
#ifndef LEGOLAND_PORTABLE
extern void  RouteSystemInit(void);                          /* 0x0041ef00 */
#else
extern int RouteSystemInit(void);                          /* 0x0041ef00 */
#endif
extern void  CoasterFxPoolInit(void);                          /* 0x0042a2e0 */
extern void  InstallCastleHooks(void);                          /* 0x00423db0 */
extern void  RouteNodePoolInit(void);                          /* 0x0041e620 */
extern void  LoadCoasterTrainModels(void);                          /* 0x0041eb70 */
extern void  LoadCoasterWheelModel(void);                          /* 0x0042a780 */

/* The CASTLE OBJ class's create hook: reset "castle placed", grab the class
 * ObjDef, claim its sprite, load the castle matte and run the eleven
 * sub-system initialisers of the castle/coaster module in order.
 * (castleobj.c declares this `void Castle_Create(void)`; it really takes the
 * class element.) */
// FUNCTION: LEGOLAND 0x00424150
void Castle_Create(RideElem* elem)
{
    g_610a04 = 0;
    MemScratchInit();
    g_829abc = elem;
    g_castle_def = elem->data;
    if (g_castle_def->sprite)
        g_castle_def->sprite->flags |= 0x2000;
    g_castle_def->flags1c |= 0x20;
    g_castle_sprite = LoadSprite("Castle Matte.lls", 1);
    Coaster3D_ResetScene();
    CarPoolInit();
    LoadCoasterData();
    CoasterGeomInit();
    CarClassTablesInit();
    CoasterSceneInit();
    RouteSystemInit();
    CoasterFxPoolInit();
    InstallCastleHooks();
    RouteNodePoolInit();
    LoadCoasterTrainModels();
    LoadCoasterWheelModel();
}

extern void Coaster3D_SampleStats(void);                           /* 0x00424e80 */
extern void Coaster3D_SetupView(void);                           /* 0x00425e20 */
extern void Coaster3D_EndFrame(int a);                          /* 0x00423140 */
extern void Coaster_TickLoadingBay(CoasterRec* r);                  /* 0x00424c70 */
extern void Coaster_TickRoute(CoasterRec* r);                  /* 0x00424a50 */
extern void Coaster_EvictRidingCars(CoasterRec* r);                  /* 0x00424dc0 */

/* This one sits BETWEEN Castle_Interact (0x00425050) and Castle_Activate
 * (0x004251c0), both of which castleobj.c already builds with optimisation
 * disabled -- and it carries the same unoptimised codegen (an ebp frame with
 * no locals, one `add esp,4` per call, `cmp [g],0` rather than a load and a
 * test). So the original's `#pragma optimize("", off)` region spans all
 * three; this is the middle one. */
#pragma optimize("", off)

/* Tear the castle down: reset the module, then, if a castle is placed, run
 * the record's three teardown passes. The class element is ignored. */
// FUNCTION: LEGOLAND 0x00425170
void Castle_Reset(RideElem* elem)
{
    Coaster3D_SampleStats();
    Coaster3D_SetupView();
    Coaster3D_EndFrame(0);
    if (g_610a04) {
        Coaster_TickLoadingBay(&g_castle);
        Coaster_TickRoute(&g_castle);
        Coaster_EvictRidingCars(&g_castle);
    }
}

#pragma optimize("", on)

/* ==========================================================================
 * Taking one piece off the coaster -- the heart of the track editor.
 *
 * Three cases, and they are the whole state machine:
 *   CLOSED CIRCUIT (state 2)  removing any piece re-OPENS the ring: the two
 *       pieces that flanked it become the new free ends, both get their free
 *       joint rebuilt against the record's end slots, both are redrawn, and
 *       the record drops back to state 1.
 *   HEAD END       the piece is the record's head_node: its inward neighbour
 *       becomes the new head end.
 *   TAIL END       otherwise: the outward neighbour becomes the new tail end.
 * Then both end slots are released, the class's own remove handler runs and
 * the node is freed.
 * ======================================================================== */
extern JointSlot g_castle_head_slot;                    /* 0x00829b8c */
extern JointSlot g_castle_tail_slot;                    /* 0x00829ba4 */
extern void RemoveTrackNodeObject(TrackNode* n);                   /* 0x0041d760 */
extern void LevelTrackRunBack(TrackNode* n);                   /* 0x004299e0 */
extern void LevelTrackRun(TrackNode* n);                   /* 0x00429a30 */
extern void Castle_StartCoasterIfComplete(CoasterRec* r);                  /* 0x00424e70 */

/* NOTE the original reads n->owner BEFORE testing n for NULL -- a real bug in
 * the original, harmless only because no caller passes NULL. Reproduced. */
// FUNCTION: LEGOLAND 0x0041d7f0
void RemoveTrackNode(TrackNode* n)
{
    CoasterRec* rec = n->owner;

    if (!n)
        return;

    RemoveTrackNodeObject(n);
    AddTrackSquareCount(-1);

    if (rec->state == 2) {
        InitTrackJoint(&n->jout.node->jin);
        InitTrackJoint(&n->jin.node->jout);
        rec->head_node = n->jin.node;
        ConnectJointHead(n->jin.node, &g_castle_head_slot);
        rec->tail_node = n->jout.node;
        ConnectJointTail(n->jout.node, &g_castle_tail_slot);
        LevelTrackRunBack(rec->head_node);
        LevelTrackRun(rec->tail_node);
        TrackNode_Draw(rec->head_node);
        TrackNode_Draw(rec->tail_node);
        rec->state = 1;
        Castle_StartCoasterIfComplete(rec);
    } else if (n == rec->head_node) {
        InitTrackJoint(&n->jin.node->jout);
        rec->head_node = n->jin.node;
        ConnectJointHead(n->jin.node, &g_castle_head_slot);
        LevelTrackRunBack(rec->head_node);
        TrackNode_Draw(rec->head_node);
    } else {
        InitTrackJoint(&n->jout.node->jin);
        rec->tail_node = n->jout.node;
        ConnectJointTail(n->jout.node, &g_castle_tail_slot);
        LevelTrackRun(rec->tail_node);
        TrackNode_Draw(rec->tail_node);
    }

    ReleaseJointSlot(&g_castle_head_slot);
    ReleaseJointSlot(&g_castle_tail_slot);
    n->desc->remove(n);
    Free_w(n);
}

/* ==========================================================================
 * DRIVING SCHOOL: the per-frame car tick.
 * Every car runs down two counters; when it has left the road network (no
 * road record under it and its on_road flag already cleared) or its life
 * counter has expired, it is retired -- and the road tile it was standing on
 * loses one user. Otherwise it drives another step.
 * ======================================================================== */
extern RoadTile* GetRoadRecord(int x, int y);           /* 0x004125f0 */
extern void      RetireSchoolCar(SchoolCar* c);         /* 0x00401c60 */
extern void      StepSchoolCar(SchoolCar* c);           /* 0x00402780 */

// FUNCTION: LEGOLAND 0x00402c10
void TickSchoolCars(void)
{
    SchoolCar* c = g_school_cars;

    while (c) {
        SchoolCar* next = c->next;
        RoadTile*  road;

        if (c->t_life)
            c->t_life--;
        if (c->t_horn)
            c->t_horn--;
        road = GetRoadRecord(c->cur.x, c->cur.y);
        if ((road == 0 && c->on_road == 0) || c->t_life == 0) {
            RetireSchoolCar(c);
            if (road)
                road->users--;
        } else {
            StepSchoolCar(c);
        }
        c = next;
    }
}

/* The edit cursor as this file uses it (0x1834 bytes; same record loaders.c's
 * DefaultCursor fills in). */
typedef struct EditCursorRec {
    unsigned char pad0000[0x1404];
    int           x;                /* +0x1404 */
    int           y;                /* +0x1408 */
    unsigned char pad140c[8];
    Footprint     footprint;        /* +0x1414 */
    unsigned char pad1428[0x1828 - 0x1428];
    int           f1828;            /* +0x1828  0x2032 for a track preview */
    unsigned char pad182c[4];
    void*         link;             /* +0x1830  previous head of the preview
                                     *          chain rooted at 0x008003f0 */
} EditCursorRec;

extern RideDef* g_dummy_def;                            /* 0x00829c34 */

/* The mirror of PlaceCastleDummies: take the CASTLE_DUMMY object off every
 * square of the castle's footprint, driving a throw-away edit cursor loaded
 * with the dummy class's own footprint, and give the squares back. */
// FUNCTION: LEGOLAND 0x00424620
void RemoveCastleDummies(const short* sq)
{
    EditCursorRec cur;
    MapPos        p;
    int           i;

    for (i = 0; i < g_castle_part_count; i++) {
        p.x = (unsigned char)(g_castle_parts[i].dx + sq[0]);
        p.y = (unsigned char)(g_castle_parts[i].dy + sq[1]);
        cur.x = p.x;
        cur.y = p.y;
        cur.footprint = g_dummy_def->footprint;
        cur.footprint.parts = 0;
        TrackRemoveObject(g_dummy_elem, p, &cur);
    }
    AddTrackSquareCount(-g_castle_part_count);
}

/* ==========================================================================
 * DRIVING SCHOOL: spawning a car.
 * The school record supplies the entry square; the car is dropped two squares
 * in (unless the 0x004c11c0 switch is set), four squares along, with its
 * world position at 16.16 of that square, a random horn pitch, one of three
 * random liveries/directions and a 9000/1200-tick pair of countdowns.
 * Returns 0 on success, -1 if the allocation failed, -2 if the car could not
 * be seated on the map (in which case the block is freed again).
 * ======================================================================== */
typedef struct SchoolRec {
    unsigned char pad00[0x0c];
    int           x;            /* +0x0c  entry square */
    int           y;            /* +0x10 */
} SchoolRec;

extern SchoolRec* GetSchoolRecord(int school);          /* 0x00412650 */
extern void* HeapAlloc_w(unsigned int n);               /* 0x0049e4ff */
extern int   FindSchoolCarNear(SchoolCar* c, int x, int y);    /* 0x00401970 */
extern void  SchoolCarManoeuvreC(SchoolCar* c);                  /* 0x00401080 */
extern void  SchoolCarAccelerate(SchoolCar* c);                  /* 0x004019c0 */
extern int   g_4c11c0;                                  /* 0x004c11c0 */
extern int   rand(void);                                /* 0x0049e4b2 (CRT) */

/* castleobj.c declares this `int NewSchoolCar(RideId id, Bloke* b)` -- the
 * packed square arrives in a 4-byte slot and only its low half is kept. */
// FUNCTION: LEGOLAND 0x00401ae0
int NewSchoolCar(int school, void* bloke)
{
    SchoolRec* rec = GetSchoolRecord(school);
    SchoolCar* c = (SchoolCar*)HeapAlloc_w(0xd0);

    if (!c)
        return -1;

    c->school = (unsigned short)school;
    c->bloke = bloke;
    if (g_4c11c0) {
        c->cur.x = rec->x;
        c->wx = rec->x << 16;
    } else {
        c->cur.x = rec->x + 2;
        c->wx = (rec->x + 2) << 16;
    }
    c->cur.y = rec->y + 4;
    c->start = c->cur;
    c->wy = (rec->y + 4) << 16;
    if (FindSchoolCarNear(c, c->cur.x, c->cur.y)) {
        HeapFree_w(c);
        return -2;
    }

    c->next = g_school_cars;
    g_school_cars = c;
    c->bb = 0;
    c->b8 = 0;
    c->on_road = 1;
    c->ba = 1;
    c->c6 = (unsigned short)(((rand() & 0xf) + 0x10) << 8);
    c->c8 = 0x1000;
    c->bc = 0;
    switch (rand() % 3) {
    case 0:
        c->dir = 3;
        break;
    case 1:
        c->dir = 1;
        break;
    case 2:
        c->dir = 2;
        break;
    }
    c->c4 = 0;
    c->t_life = 9000;
    c->t_horn = 1200;
    SchoolCarManoeuvreC(c);
    SchoolCarAccelerate(c);
    return 0;
}

/* ==========================================================================
 * The track EDITOR's per-frame preview pass.
 *
 * SQUARE_TRACK_HEIGHT's ObjDef "update" runs every frame while the player is
 * dragging a piece around. It does three things:
 *   1. arm the shared edit cursor with the class footprint and turn the mouse
 *      position into a map reference;
 *   2. lay out a GHOST cursor on every legal continuation square of BOTH ends
 *      of the coaster -- each end offers up to four candidate squares and a
 *      four-bit legality mask (JointSlot). The ghosts are taken from the
 *      static cursor array at 0x0081ce00 (0x1834 bytes each), consecutively,
 *      head-end candidates first; each is chained onto the preview list at
 *      0x008003f0 and stamped with mode 0x2032;
 *   3. validate the real cursor, then ask whether the piece under the mouse
 *      actually fits -- clearing the footprint if it does, and raising cursor
 *      error 0x0e if it does not.
 * ======================================================================== */
typedef struct MapPos16 { unsigned short x; unsigned short y; } MapPos16;

/* ScreenToMapRef's output: two INTs (the track code reads them as dwords when
 * it does arithmetic on them and as words when it packs them into a
 * MapPos16). */
typedef struct MapRef { int x; int y; } MapRef;

extern EditCursorRec g_track_cursors[];                 /* 0x0081ce00 */
extern void* g_8003f0;                                  /* 0x008003f0  preview chain */
extern int   g_8003e8;                                  /* 0x008003e8 */
extern MapRef g_mapref;                                 /* 0x007fffc4 */
extern int    EditCursor;                               /* 0x007febc0 */
extern void SetEditCursorFootPrint(Footprint* fp);      /* 0x0045f440 */
#ifndef LEGOLAND_PORTABLE
extern void ScreenToMapRef(int screen, MapRef* out, int mode); /* 0x0045be90 */
#else
extern int ScreenToMapRef(int screen, MapRef* out, int mode); /* 0x0045be90 */
#endif
extern void DefaultCursor(EditCursorRec* cur);          /* 0x0045a390 */
extern void ResetCursorFootprint(void* cur);            /* 0x0045f460 */
extern void SetCursorError(void* cur, int code);        /* 0x0045f480 */
extern void ValidateCursor(void* cur, RideDef* def);    /* 0x0045f810 */
extern TrackNode* FindTrackDesc(RideElem* elem);           /* 0x00427c00 */

// FUNCTION: LEGOLAND 0x00427c90
void TrackH_Update(RideElem* elem, int screen, int mode)
{
    MapPos16       sq;
    JointSlot*     slot;
    RideDef*       def = elem->data;
    TrackNode*     n;
    int            count;
    int            mask;
    int            i;

    SetEditCursorFootPrint(&def->footprint);
    g_8003f0 = 0;
    ScreenToMapRef(screen, &g_mapref, mode);

    count = 0;
    mask = 1;
    slot = &g_castle.head_slot;
    for (i = 0; i <= 3; i++) {
        if (mask & slot->mask) {
            DefaultCursor(&g_track_cursors[count]);
            g_track_cursors[count].link = g_8003f0;
            g_8003f0 = &g_track_cursors[count];
            g_track_cursors[count].x = slot->sq[i].x;
            g_track_cursors[count].y = slot->sq[i].y;
            g_track_cursors[count].footprint = def->footprint;
            g_track_cursors[count].f1828 = 0x2032;
            ResetCursorFootprint(&g_track_cursors[count]);
            count++;
        }
        mask <<= 1;
    }

    mask = 1;
    slot = &g_castle.tail_slot;
    for (i = 0; i <= 3; i++) {
        if (mask & slot->mask) {
            DefaultCursor(&g_track_cursors[count]);
            g_track_cursors[count].link = g_8003f0;
            g_8003f0 = &g_track_cursors[count];
            g_track_cursors[count].x = slot->sq[i].x;
            g_track_cursors[count].y = slot->sq[i].y;
            g_track_cursors[count].footprint = def->footprint;
            g_track_cursors[count].f1828 = 0x2032;
            ResetCursorFootprint(&g_track_cursors[count]);
            count++;
        }
        mask <<= 1;
    }

    ValidateCursor(&EditCursor, elem->data);
    sq.x = (unsigned short)g_mapref.x;
    sq.y = (unsigned short)g_mapref.y;
    g_8003e8 |= 8;
    n = FindTrackDesc(elem);
    if (n) {
        if (TrackFitCheck(n, &sq)->flags)
            ResetCursorFootprint(&EditCursor);
        else
            SetCursorError(&EditCursor, 0xe);
    }
}

/* SQUARE_TRACK_HEIGHT_PATH's update: the same preview pass as TrackH_Update,
 * plus a GRADIENT check. GetOpenEndSteps turns the fit's geometry key into the two
 * end heights; a piece that would need a total rise of two or more steps is
 * rejected with cursor error 0x0b (the flat/height-path piece can only climb
 * one step), where a piece that does not fit at all is error 0x0e. */
extern void GetOpenEndSteps(int key, int* h0, int* h1);      /* 0x0041cf70 */

// FUNCTION: LEGOLAND 0x004280b0
void TrackHP_Update(RideElem* elem, int screen, int mode)
{
    MapPos16       sq;
    JointSlot*     slot;
    RideDef*       def = elem->data;
    TrackNode*     n;
    TrackFit*      fit;
    int            total = 0;
    int            h0;
    int            h1;
    int            count;
    int            mask;
    int            i;

    g_8003f0 = 0;
    SetEditCursorFootPrint(&def->footprint);
    ScreenToMapRef(screen, &g_mapref, mode);

    count = 0;
    mask = 1;
    slot = &g_castle.head_slot;
    for (i = 0; i <= 3; i++) {
        if (mask & slot->mask) {
            DefaultCursor(&g_track_cursors[count]);
            g_track_cursors[count].link = g_8003f0;
            g_8003f0 = &g_track_cursors[count];
            g_track_cursors[count].x = slot->sq[i].x;
            g_track_cursors[count].y = slot->sq[i].y;
            g_track_cursors[count].footprint = def->footprint;
            g_track_cursors[count].f1828 = 0x2032;
            ResetCursorFootprint(&g_track_cursors[count]);
            count++;
        }
        mask <<= 1;
    }

    mask = 1;
    slot = &g_castle.tail_slot;
    for (i = 0; i <= 3; i++) {
        if (mask & slot->mask) {
            DefaultCursor(&g_track_cursors[count]);
            g_track_cursors[count].link = g_8003f0;
            g_8003f0 = &g_track_cursors[count];
            g_track_cursors[count].x = slot->sq[i].x;
            g_track_cursors[count].y = slot->sq[i].y;
            g_track_cursors[count].footprint = def->footprint;
            g_track_cursors[count].f1828 = 0x2032;
            ResetCursorFootprint(&g_track_cursors[count]);
            count++;
        }
        mask <<= 1;
    }

    ValidateCursor(&EditCursor, elem->data);
    sq.x = (unsigned short)g_mapref.x;
    sq.y = (unsigned short)g_mapref.y;
    g_8003e8 |= 8;
    n = FindTrackDesc(elem);
    if (n) {
        fit = TrackFitCheck(n, &sq);
        if (fit->flags == 0) {
            SetCursorError(&EditCursor, 0xe);
            return;
        }
        GetOpenEndSteps(fit->f04, &h0, &h1);
        if (h0 >= 0)
            total = h0;
        if (h1 >= 0)
            total += h1;
        if (total >= 2) {
            SetCursorError(&EditCursor, 0xb);
            return;
        }
        ResetCursorFootprint(&EditCursor);
    }
}

/* ==========================================================================
 * SQUARE_TRACK's update -- the FLAT piece, and the most complete of the three.
 * It adds two things the raised variants do not:
 *   * a SNAP pass. Before anything is drawn the mouse's map square is pulled
 *     onto the nearest legal continuation square, if it is within one square
 *     of one (Chebyshev distance 1 in both axes). That is what makes laying
 *     track feel magnetic.
 *   * a final CONFIRM pass. After the ghosts are laid out the (snapped) map
 *     reference is compared against every ghost's square, walking the ghosts
 *     in exactly the order they were created -- head-slot bits 0..3 then
 *     tail-slot bits 0..3, skipping the bits the mask clears. A hit clears
 *     the cursor error; anything else leaves error 0x0e standing.
 * When there is no piece under the mouse the flat class's own .rdata
 * descriptor (0x004b5d20) is handed to TrackFitCheck in place of a node --
 * its first dword is the `raised` flag, which is 0, so the check takes the
 * un-raised path. Reproduced as the original has it.
 * ======================================================================== */
extern Footprint g_edit_footprint;                      /* 0x007fffd4 */
extern TrackDesc g_track_desc_flat;                     /* 0x004b5d20 */
extern int  CursorIsValid(void* cur);                   /* 0x0045f4b0 */

// FUNCTION: LEGOLAND 0x004275d0
void Track_Update(RideElem* elem, int screen, int mode)
{
    RideDef*       def = elem->data;
    EditCursorRec* gc;
    JointSlot*     slot;
    TrackNode*     n;
    int            count = 0;
    int            mask;
    int            bit;
    int            i;
    int            j;
    int            my;
    int            mx;

    g_edit_footprint = def->footprint;
    g_8003f0 = 0;
    ScreenToMapRef(screen, &g_mapref, mode);
    if (g_castle_state != 2) {

    mask = 1;
    for (i = 0; i <= 3; i++) {
        if (mask & g_castle.head_slot.mask) {
            if (g_mapref.x - g_castle.head_slot.sq[i].x >= -1
                && g_mapref.x - g_castle.head_slot.sq[i].x <= 1) {
                if (g_mapref.y - g_castle.head_slot.sq[i].y >= -1
                    && g_mapref.y - g_castle.head_slot.sq[i].y <= 1)
                    goto snap_head;
            }
        }
        mask <<= 1;
    }
    mask = 1;
    for (i = 0; i <= 3; i++) {
        if (mask & g_castle.tail_slot.mask) {
            if (g_mapref.x - g_castle.tail_slot.sq[i].x >= -1
                && g_mapref.x - g_castle.tail_slot.sq[i].x <= 1) {
                if (g_mapref.y - g_castle.tail_slot.sq[i].y >= -1
                    && g_mapref.y - g_castle.tail_slot.sq[i].y <= 1)
                    goto snap_tail;
            }
        }
        mask <<= 1;
    }
    goto snapped;
snap_head:
    g_mapref.x = g_castle.head_slot.sq[i].x;
    g_mapref.y = g_castle.head_slot.sq[i].y;
    goto snapped;
snap_tail:
    g_mapref.x = g_castle.tail_slot.sq[i].x;
    g_mapref.y = g_castle.tail_slot.sq[i].y;
snapped:

    ValidateCursor(&EditCursor, def);

    mask = 1;
    slot = &g_castle.head_slot;
    for (i = 0; i <= 3; i++) {
        if (mask & slot->mask) {
            DefaultCursor(&g_track_cursors[count]);
            g_track_cursors[count].link = g_8003f0;
            g_8003f0 = &g_track_cursors[count];
            g_track_cursors[count].x = slot->sq[i].x;
            g_track_cursors[count].y = slot->sq[i].y;
            g_track_cursors[count].footprint = def->footprint;
            g_track_cursors[count].f1828 = 0x2032;
            ResetCursorFootprint(&g_track_cursors[count]);
            count++;
        }
        mask <<= 1;
    }
    mask = 1;
    slot = &g_castle.tail_slot;
    for (i = 0; i <= 3; i++) {
        if (mask & slot->mask) {
            DefaultCursor(&g_track_cursors[count]);
            g_track_cursors[count].link = g_8003f0;
            g_8003f0 = &g_track_cursors[count];
            g_track_cursors[count].x = slot->sq[i].x;
            g_track_cursors[count].y = slot->sq[i].y;
            g_track_cursors[count].footprint = def->footprint;
            g_track_cursors[count].f1828 = 0x2032;
            ResetCursorFootprint(&g_track_cursors[count]);
            count++;
        }
        mask <<= 1;
    }

    {
        MapPos16 sq;

        sq.x = (unsigned short)g_mapref.x;
        sq.y = (unsigned short)g_mapref.y;
        n = FindTrackDesc(elem);
        if (!n)
            n = (TrackNode*)&g_track_desc_flat;
        if (TrackFitCheck(n, &sq)->flags == 0)
            SetCursorError(&EditCursor, 0xe);
    }
    if (!CursorIsValid(&EditCursor))
        return;
    SetCursorError(&EditCursor, 0xe);

    my = g_mapref.y;
    mx = g_mapref.x;
    bit = 1;
    slot = &g_castle.head_slot;
    gc = g_track_cursors;
    for (j = 0; j < 8; j++) {
        if (j == 4) {
            bit = 1;
            slot = &g_castle.tail_slot;
        }
        if (slot->mask & bit) {
            if (mx == gc->x && my == gc->y)
                goto found;
            gc++;
        }
        bit <<= 1;
    }
    return;
found:
    ResetCursorFootprint(&EditCursor);
    return;
    }
    SetCursorError(&EditCursor, 0xe);
}

/* ==========================================================================
 * DRIVING SCHOOL: the traffic lights.
 *
 * One global two-phase light drives every crossroads in the park. A single
 * countdown (0x004b4c04) steps a four-state cycle (0x004cbeb8):
 *      0  north-south goes red    (0x004cbeb0 = 0), 0x28 ticks of all-red
 *      1  east-west goes green    (0x004cbeb4 = 1), 0x8c ticks of green
 *      2  east-west goes red      (0x004cbeb4 = 0), 0x28 ticks of all-red
 *      3  north-south goes green  (0x004cbeb0 = 1), 0x8c ticks of green
 * so the pattern is 140 ticks green / 40 ticks all-red on each axis.
 *
 * From the two lamp flags (and the 0x004c11c0 theme switch, which picks
 * between two whole sets of lamp sprites: 4..7 and 8..15) four tile-sprite
 * indices are chosen -- one per corner of the junction -- and every road
 * tile whose type nibble is 5 (a crossroads) gets all four drawn, at its own
 * square and at +3 in each axis. The offsets are half the sprite's own tile
 * size, run through AdjustOffsetForViewMode, and the sort key is the tile's
 * top edge less the config's +0x22 horizon.
 * ======================================================================== */
typedef struct Pos { int x; int y; } Pos;

typedef struct TileBounds {
    int left;                   /* +0x00 */
    int top;                    /* +0x04 */
    int right;                  /* +0x08 */
    int bottom;                 /* +0x0c */
} TileBounds;

typedef struct TileSprites {
    unsigned char pad00[8];
    void**        sprite;       /* +0x08 */
    int*          w;            /* +0x0c */
    int*          h;            /* +0x10 */
} TileSprites;

typedef struct Config {
    unsigned char  pad00[0x22];
    unsigned short horizon;     /* +0x22 */
} Config;

extern TileSprites* g_tile_sprites;                     /* 0x0082c680 */
extern Config*      lpConfig;                           /* 0x004bcbf4 */
extern int          g_light_timer;                      /* 0x004b4c04 */
extern int          g_light_ns;                         /* 0x004cbeb0 */
extern int          g_light_ew;                         /* 0x004cbeb4 */
extern int          g_light_state;                      /* 0x004cbeb8 */
extern void GetTileDimensions(int* out_w, int* out_h);  /* 0x00460540 */
extern void GetTileBounds(Pos* tile, TileBounds* out);  /* 0x0045acc0 */
extern void AdjustOffsetForViewMode(Pos* o);            /* 0x00442d30 */
extern void SortSprite(void* sprite, int x, int y, int key, int mode, void* ctx); /* 0x00485d70 */

/* The per-corner setup: pick the lamp sprite, halve its tile size, run the
 * result through the view-mode adjust, and get the pixel bounds of the
 * corner's map square. Written as a macro rather than a helper so the four
 * expansions share ONE set of frame homes, as the original does. */
#define LIGHT_OFFSET(IDX)                                                  \
    k = (unsigned char)(IDX) * 4;                                          \
    off.x = *(int*)((char*)g_tile_sprites->w + k) >> 1;                    \
    off.y = *(int*)((char*)g_tile_sprites->h + k) >> 1;                    \
    AdjustOffsetForViewMode(&off)

#define LIGHT_BOUNDS(TX, TY)                                               \
    pos.x = (TX);                                                          \
    pos.y = (TY);                                                          \
    GetTileBounds(&pos, &out)

#define LIGHT_SPRITE   (*(void**)((char*)g_tile_sprites->sprite + k))

// FUNCTION: LEGOLAND 0x00414440
void DrawTrafficLights(void)
{
    RoadTile*    t = g_road_tiles;
    int          s0;
    int          s2;
    int          s3;
    int          tw;
    int          th;
    Pos          off;
    Pos          pos;
    TileBounds   out;
    int          s1;
    unsigned int k;

    if (--g_light_timer <= 0) {
        int state = g_light_state;

        switch (state) {
        case 0:
            g_light_ns = 0;
            g_light_timer = 0x28;
            break;
        case 1:
            g_light_ew = 1;
            g_light_timer = 0x8c;
            break;
        case 2:
            g_light_ew = 0;
            g_light_timer = 0x28;
            break;
        case 3:
            g_light_ns = 1;
            g_light_timer = 0x8c;
            break;
        }
        g_light_state = (state + 1) & 3;
    }

    if (g_4c11c0) {
        if (g_light_ns) {
            s1 = 6;
            s2 = 1;
            s3 = 2;
            s0 = 5;
        } else if (g_light_ew) {
            s1 = 7;
            s2 = 0;
            s3 = 3;
            s0 = 4;
        } else {
            s1 = 7;
            s2 = 1;
            s3 = 3;
            s0 = 5;
        }
    } else {
        if (g_light_ns) {
            s1 = 9;
            s2 = 0xa;
            s3 = 0xd;
            s0 = 0xe;
        } else if (g_light_ew) {
            s1 = 8;
            s2 = 0xb;
            s3 = 0xc;
            s0 = 0xf;
        } else {
            s1 = 9;
            s2 = 0xb;
            s3 = 0xd;
            s0 = 0xf;
        }
    }

    while (t) {
        if ((t->type & 0xf) == 5) {
            GetTileDimensions(&tw, &th);

            LIGHT_OFFSET(s0);
            LIGHT_BOUNDS(t->x, t->y);
            SortSprite(LIGHT_SPRITE, out.left + off.x, out.top + off.y,
                       out.top - lpConfig->horizon, 0, 0);

            LIGHT_BOUNDS(t->x + 3, t->y);
            LIGHT_OFFSET(s1);
            SortSprite(LIGHT_SPRITE, out.left + off.x, out.top + off.y,
                       ((out.top + out.bottom) >> 1) - lpConfig->horizon, 0, 0);

            LIGHT_BOUNDS(t->x + 3, t->y + 3);
            LIGHT_OFFSET(s2);
            SortSprite(LIGHT_SPRITE, out.left + off.x, out.top + off.y,
                       out.bottom - lpConfig->horizon, 0, 0);

            LIGHT_BOUNDS(t->x, t->y + 3);
            LIGHT_OFFSET(s3);
            SortSprite(LIGHT_SPRITE, out.left + off.x, out.top + off.y,
                       ((out.top + out.bottom) >> 1) - lpConfig->horizon, 0, 0);
        }
        t = t->next;
    }
}
