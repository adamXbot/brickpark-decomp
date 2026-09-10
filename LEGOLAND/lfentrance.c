/* LEGOLAND -- the LOG FLUME ENTRANCE: the ride's control station.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  None of
 * these functions is exported; every extent comes from the disassembly by
 * control flow (tools/audit.py).  Struct field OFFSETS, record sizes and
 * global addresses are load-bearing; the names are ours.  Types are declared
 * LOCALLY on purpose (legoland.h is owned elsewhere).
 *
 * The ten log-flume classes, the piece graph, the run/boat records, the shape
 * encoding and the ten legal neighbour masks are documented in the headers of
 * LEGOLAND/logflume.c and LEGOLAND/logflume2.c.  This file carries the three
 * ENTRANCE callbacks those two files left behind -- the +0x98 add, the +0xa8
 * activate and the +0xb0 interact -- which are between them the whole life of
 * a rider and the whole life of a run.
 * ========================================================================= */

/* ---- shared types (same offsets as logflume.c / mechrides.c) ------------ */
typedef struct Pos { int x; int y; } Pos;

typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

typedef struct FootPart FootPart;

typedef struct Footprint {
    int       v[4];             /* +0x00 */
    FootPart* parts;            /* +0x10 */
} Footprint;                    /* 0x14 */

typedef struct SpriteRec {
    unsigned char pad00[0x10];
    unsigned int  flags;        /* +0x10 */
} SpriteRec;

/* The 0xd0-byte object definition (the ".ODF class"). */
typedef struct RideDef {
    unsigned char pad00[0x0c];
    int           base_x;       /* +0x0c */
    int           base_y;       /* +0x10 */
    int           f14;          /* +0x14 */
    int           f18;          /* +0x18 */
    unsigned int  flags1c;      /* +0x1c */
    unsigned char pad20[4];
    signed char   qx;           /* +0x24 queue/exit offset from the square */
    signed char   qy;           /* +0x25 */
    unsigned char pad26[8];
    short         capacity;     /* +0x2e */
    unsigned char pad30[0x3c - 0x30];
    Footprint     footprint;    /* +0x3c */
    unsigned char pad50[0x14];
    SpriteRec*    sprite;       /* +0x64 */
    unsigned char pad68[0xcc - 0x68];
    struct RiderNode* riders;   /* +0xcc the class-wide rider list */
} RideDef;

typedef struct RideElem {
    char*        name;          /* +0x00 */
    char*        image;         /* +0x04 */
    unsigned int flags;         /* +0x08 */
    RideDef*     data;          /* +0x0c */
} RideElem;

/* The packed {x,y} map square a rider (and a run) is keyed by. */
typedef union RideTile {
    unsigned short key;
    BPos           b;
} RideTile;

typedef struct Person3D Person3D;

typedef struct Bloke {
    unsigned char  pad00[4];
    Person3D*      person;      /* +0x04 */
    unsigned char  pad08[6];
    unsigned short state;       /* +0x0e low-level AI state (0 = idle) */
    unsigned char  pad10[0x24 - 0x10];
    Pos            target;      /* +0x24 walk target, 24.8 */
    unsigned char  pad2c[8];
    signed char    b34;         /* +0x34 path step direction, +1 / -1 */
    unsigned char  b35;
    unsigned char  seat;        /* +0x36 */
    unsigned char  pad37;
    unsigned short f38;         /* +0x38 index into the path's frame table */
    unsigned char  pad3a[0x50 - 0x3a];
    void*          anim;        /* +0x50 the path the rider is walking */
    void*          bnvpath;     /* +0x54 */
    int            f58;         /* +0x58 */
    unsigned char  pad5c[4];
    unsigned char  action;      /* +0x60 this ride's state-machine step */
    unsigned char  pad61;
    unsigned short flags;       /* +0x62 8 = on this ride, 0x80 = riding,
                                 *       0x40 = queueing */
    unsigned char  pad64[4];
    Pos            world;       /* +0x68 world position, 24.8 */
    unsigned char  pad70[2];
    unsigned char  b72;         /* +0x72 */
    unsigned char  new_dir;     /* +0x73 */
    unsigned char  pad74[0x98 - 0x74];
    unsigned char  path[0x14];  /* +0x98 CalcMoveLine scratch */
} Bloke;

/* A rider slot on the class-wide list at ObjDef+0xcc (rides.c). */
typedef struct RideOwner {
    unsigned char pad00[0x20];
    int           key;          /* +0x20 the depth key for the render list */
} RideOwner;

typedef struct RiderNode {
    struct RiderNode* next;     /* +0x00 */
    struct RiderNode* prev;     /* +0x04 */
    Bloke*            bloke;    /* +0x08 */
    unsigned short    ride_id;  /* +0x0c the packed map square it is using */
    unsigned short    pad0e;
    struct RideOwner* owner;    /* +0x10 */
} RiderNode;

typedef struct LFPiece LFPiece;
typedef struct LFRun   LFRun;

/* THE STATION QUEUE (LFRun+0x2c) -- an 8-byte header over a list of 8-byte
 * nodes {next, rider}.  +0x00 points at the thing whose FIRST INT is the
 * queue's capacity (the run's boat/seat count); +0x04 is the list head.  The
 * whole module lives at 0x00411e30..0x00412400. */
typedef struct LFQueue {
    void* path;                 /* +0x00 an LFAnimPath; its first int is the
                                 *       queue's capacity */
    void* head;                 /* +0x04 */
} LFQueue;

/* One boat: 0x24 bytes, four of them from LFRun+0x40 on. */
typedef struct LFBoat {
    int  state;                 /* +0x00 */
    unsigned char pad04[0x24 - 4];
} LFBoat;

struct LFRun {
    LFRun*        next;         /* +0x00 */
    unsigned int  flags;        /* +0x04 bit 0 = "a boat is loading" */
    LFPiece*      f08;          /* +0x08 */
    LFPiece*      f0c;          /* +0x0c */
    LFPiece*      pieces;       /* +0x10 head of this run's piece list */
    BPosW         sq;           /* +0x14 the station's map square */
    unsigned char pad16[2];
    LFPiece*      f18;          /* +0x18 */
    int           frame;        /* +0x1c */
    unsigned char pad20[0xc];
    LFQueue       queue;        /* +0x2c the boarding queue */
    unsigned char pad34[4];
    LFBoat*       boat;         /* +0x38 the boat currently at the station */
    int           f3c;          /* +0x3c how many boats the run runs */
    unsigned char pad40[0xd0 - 0x40];
    int           piece_count;  /* +0xd0 */
};                              /* 0xd4 */

/* ---- engine entry points ----------------------------------------------- */
extern int  CalcMoveLine(Pos from, Pos to, void* path);          /* 0x00480740 */
extern int  NewDirForAction(Bloke* b, unsigned char dir);        /* 0x004833d0 */
extern void BlokeSetFrame(Bloke* b, int frame);                  /* 0x00440870 */
extern void BlokeWalkAnim(Bloke* b);                             /* 0x00440910 */
extern void RemoveBlokeFromRide(RideDef* def, RiderNode* r);     /* 0x0048a100 */
extern void Ride_SetFlagToNotLetAnyoneOn(BPosW* sq);             /* 0x00442fa0 */

/* ---- log-flume helpers ------------------------------------------------- */
extern LFRun* LFStation_FindAt(const RideTile* sq);              /* 0x00408ec0 */
extern void   LFRun_TickAll(void);                               /* 0x0040bf50 */
extern void   LFQueue_AddRider(LFQueue* q, RiderNode* r);        /* 0x00411f20 */
extern int    LFQueue_IsFull(LFQueue* q);                        /* 0x00411e60 */
extern void   LFPath_StartReverse(void* path, Bloke* b);         /* 0x004122a0 */
extern void   LFPath_StepBloke(void* path, int tx, int ty,
                               Bloke* b);                        /* 0x00412300 */

extern void* g_lf_anim_b;               /* 0x004c2af8 the 5-frame path set */


/* ---- the placed-piece record (0x38 bytes, zeroed by LFPiece_Alloc) ------ */
struct LFPiece {
    LFPiece*      next;         /* +0x00 next piece of the same run */
    LFPiece*      prev;         /* +0x04 */
    LFPiece*      fwd;          /* +0x08 next piece ALONG THE ROUTE */
    LFPiece*      back;         /* +0x0c */
    unsigned int  flags;        /* +0x10 */
    BPosW         sq;           /* +0x14 the piece's map square */
    unsigned char pad16[2];
    int           kind;         /* +0x18 3/4 = still has a free end */
    int           dir;          /* +0x1c orientation / variant, 0..3 */
    RideDef*      def;          /* +0x20 */
    LFRun*        run;          /* +0x24 */
    LFPiece*      parent;       /* +0x28 the piece this one is a sub-piece of */
    LFPiece*      sub;          /* +0x2c head of this piece's sub-list */
    LFPiece*      end_a;        /* +0x30 the sub-piece at end A */
    LFPiece*      end_b;        /* +0x34 the sub-piece at end B */
};

extern Footprint g_lf_footprint;        /* 0x004b4728  the flume's cell rect */
extern RideDef*  g_lftr_def;            /* 0x004cbe30  LOG FLUME TRACK */
extern RideElem* g_lftr_elem;           /* 0x004c74f4 */
extern RideDef*  g_lfen_def;            /* 0x004c2b9c  LOG FLUME ENTRANCE */
extern void*     g_lf_anim_a;           /* 0x004c2ae8  the 4-frame path set */

extern void     AddBasicObject(RideElem* elem, const Pos* p);    /* 0x0045efe0 */
extern void     LFRun_New(BPosW sq);                             /* 0x00408e40 */
extern LFPiece* LFPiece_Alloc(void);                             /* 0x00409010 */
extern void     LFPiece_LinkAfter(LFPiece* at, LFPiece* p);      /* 0x00409080 */
extern void     LFPiece_AddSub(LFPiece* parent, LFPiece* p);     /* 0x00409170 */
extern void     LFRun_AddPiece(LFRun* run, LFPiece* p);          /* 0x004091f0 */
extern void     LFRun_Finish(LFRun* st);                         /* 0x0040a5d0 */

/* =========================================================================
 * THE STATION'S OWN LITTLE HELPERS
 *
 * These four are only ever called from the three entrance callbacks above,
 * and between them they are the whole life cycle of an LFRun: New (open a
 * run record for a square and push it on g_lf_queue), Finish (stamp it and
 * start it), TickAll (advance every run in the park, once per frame from the
 * activate slot) and the two queue predicates.
 * ========================================================================= */

extern LFRun* g_lf_queue;               /* 0x004cbe84  every station/run */

extern void* malloc(unsigned int n);                             /* 0x0049e4ff */
extern void  LFRun_Tick(LFRun* run);                             /* 0x0040be00 */
extern void  LFRun_Start(LFRun* run);                            /* 0x0040a580 */
extern void  LFQueue_Append(LFQueue* q, void* node);             /* 0x00411e30 */

/* Open a run record for a map square: 0xd4 bytes, zeroed, keyed by the
 * square at +0x14 and pushed on the head of the park-wide run list.  A failed
 * malloc leaves the list untouched -- and every caller then discovers there
 * is no run by looking it up again. */
// FUNCTION: LEGOLAND 0x00408e40
void LFRun_New(BPosW sq)
{
    LFRun* run = (LFRun*)malloc(0xd4);

    if (run) {
        int  n = 0x35;
        int* d = (int*)run;
        while (n--)
            *d++ = 0;
        run->sq = sq;
        run->next = g_lf_queue;
        g_lf_queue = run;
    }
}

/* Close the build: four boats and a nominal length of six squares, then hand
 * the run to the machine that moves them. */
// FUNCTION: LEGOLAND 0x0040a5d0
void LFRun_Finish(LFRun* st)
{
    if (st) {
        st->f3c = 4;
        st->piece_count = 6;
        LFRun_Start(st);
    }
}

/* One tick for every run in the park.  This is the first thing the +0xa8
 * activate slot does, before it touches a single rider. */
// FUNCTION: LEGOLAND 0x0040bf50
void LFRun_TickAll(void)
{
    LFRun* run = g_lf_queue;

    while (run) {
        LFRun_Tick(run);
        run = run->next;
    }
}

/* THE BOARDING QUEUE.  The nodes are 8 bytes {next, rider}; the header's
 * +0x00 points at the path the queue is laid along, whose FIRST INT is how
 * many people fit on it -- so "the queue is full" is literally "the queue has
 * as many nodes as the path has frames". */
typedef struct LFQueueNode {
    struct LFQueueNode* next;   /* +0x00 */
    RiderNode*          rider;  /* +0x04 */
} LFQueueNode;

// FUNCTION: LEGOLAND 0x00411e60
int LFQueue_IsFull(LFQueue* q)
{
    LFQueueNode* n = q->head;
    int          count;

    if (n) {
        count = 0;
        n = q->head;            /* the original re-reads the head here, and
                                 * that is what keeps the second null test */
        while (n) {
            n = n->next;
            count++;
        }
        if (count == *(int*)q->path)
            return 1;
    }
    return 0;
}

/* Put a rider on the end of the queue: allocate the node, mark the bloke
 * "queueing" (+0x62 bit 0x40), reset its path index and step it on to the
 * next state.  A failed malloc silently drops the rider -- it stays on step 0
 * and tries again next frame. */
// FUNCTION: LEGOLAND 0x00411f20
void LFQueue_AddRider(LFQueue* q, RiderNode* r)
{
    LFQueueNode* n = (LFQueueNode*)malloc(8);

    if (n) {
        int  z = 2;             /* the same allocate-and-zero idiom as
                                 * LFPiece_Alloc; two words, so VC6 unrolls it
                                 * into a zero register and two stores */
        int* d = (int*)n;

        while (z--)
            *d++ = 0;
        n->rider = r;
        r->bloke->flags |= 0x40;
        r->bloke->f38 = 0;
        r->bloke->action++;
        LFQueue_Append(q, n);
    }
}

/* =========================================================================
 * +0x98 ADD -- BUILDING THE STATION'S OWN RUN.
 *
 * Dropping a LOG FLUME ENTRANCE does not just place a building: it opens a
 * run record and lays the whole boarding channel in one go.  In order:
 *
 *   1. the generic placer puts the building on the map (AddBasicObject);
 *   2. LFRun_New allocates a 0xd4-byte run keyed by this square and pushes
 *      it on g_lf_queue -- so LFStation_FindAt immediately after is how the
 *      handler gets hold of the record it just made (if the malloc failed
 *      there is no run and the whole rest of the handler is skipped);
 *   3. the run's boarding queue takes the FOUR-FRAME path set as its
 *      capacity reference (LFRun+0x2c): the queue is full when it holds as
 *      many riders as that path has frames;
 *   4. an END piece (kind 3, dir 0) one square right of and `dy` above the
 *      station, where `dy` is the height of ONE flume cell;
 *   5. a piece carrying the ENTRANCE class itself, on the station's own
 *      square, remembered at LFRun+0x18 and linked after the end piece;
 *   6. a COLUMN of ordinary channel pieces (kind 1, flags 7), one per flume
 *      cell down the class footprint -- `(v[3]-v[1]+1)/dy` of them -- each
 *      a SUB-piece of the entrance piece and each linked into the route
 *      chain behind the last.  The first and last of the column become the
 *      entrance piece's two ends (+0x30/+0x34), and the one straddling the
 *      vertical MIDDLE of the footprint is recorded at LFRun+0x08 with its
 *      predecessor at +0x0c -- that pair is where a boat sits at the
 *      station, i.e. the boarding point LFEntrance_Activate case 4 releases;
 *   7. a second END piece (kind 3, dir 2) below the footprint;
 *   8. LFRun_Finish stamps the run (+0x3c = 4, +0xd0 = 6) and starts it.
 *
 * The class footprint of the TRACK is re-stamped from the flume template and
 * shrunk by one cell in both axes before each AddBasicObject, exactly as
 * LFTrack_Add does, so each channel square covers the drawn cell.
 *
 * ORIGINAL BUGS, reproduced: the entrance piece (step 5) and the two end
 * pieces are used without checking LFPiece_Alloc's result in some paths --
 * step 5 dereferences p2 unconditionally, and the run's b.x/b.y are read
 * back out of p1 even when p1 is null.
 * ========================================================================= */

/* 2026-09-05 (scope H continuation): exact, 256 instructions / 812 bytes.
 * The two AddBasicObject coordinates are DISTINCT block-scoped Pos locals;
 * the running a/b coordinates remain non-address-taken aggregates.  A Pos
 * whose address escapes to either call prevents the original's early byte
 * reloads; replacing it with two scalars instead loses the aggregate's
 * arithmetic grouping.  Separate objects recover BOTH properties at once.
 *
 * Measured independently: a distinct tail-call Pos fixes the original head
 * and p1 block through index 82 (206 -> 105); a distinct first-call Pos also
 * fixes the loop (105 -> 52).  For p3, assign b.x AND b.y before b.x++ and
 * b.y += dy, preserving the aggregate's partial sums (52 -> 2).  Finally
 * place p3->flags |= 3 immediately after p3->dir = 2 (2 -> 0).  Moving that
 * flags update one statement earlier or later leaves two differences.
 * All stores, calls, frame homes and the ebx/ebp allocation now agree.
 *
 * This corrects the prior claim that the head and p1 requirements were
 * incompatible: aggregate arithmetic protection and address escape are
 * independent levers.  Neither volatile nor altered types are needed.
 * Original quirks remain: the p1 square is read even after allocation
 * failure; p2 is dereferenced without a null check; the class footprint is
 * overwritten globally before the first extra track object is placed.
 */
// FUNCTION: LEGOLAND 0x0040a600
void LFEntrance_Add(RideElem* elem, const Pos* pos)
{
    RideDef* def;
    int      midy;
    int      n;
    BPosW    sq;
    Pos      b;
    Pos      a;
    LFRun*   st;
    LFPiece* p1;
    LFPiece* p2;
    LFPiece* p3;
    LFPiece* p;
    LFPiece* last;
    int      dy;
    int      i;
    int      fx;
    int      fy;

    sq.b.x = (unsigned char)pos->x;
    sq.b.y = (unsigned char)pos->y;
    def = elem->data;
    AddBasicObject(elem, pos);
    LFRun_New(sq);
    st = LFStation_FindAt((RideTile*)&sq);
    if (st == 0)
        return;

    dy   = g_lf_footprint.v[3] - g_lf_footprint.v[1];
    midy = sq.b.y + (def->footprint.v[1]
                     + ((def->footprint.v[3] - def->footprint.v[1]) >> 1));
    st->queue.path = g_lf_anim_a;
    fx = def->footprint.v[0];      /* both loads BEFORE the a.x store: */
    fy = def->footprint.v[1];      /* keep the two inputs defined together */
    a.x = sq.b.x + fx;
    a.y = sq.b.y + fy;
    a.x++;
    a.y -= dy;

    p1 = LFPiece_Alloc();
    if (p1 != 0) {
        p1->dir  = 0;
        p1->kind = 3;
        p1->def  = g_lftr_def;
        p1->run  = st;
        p1->flags |= 3;
        p1->parent = 0;
        p1->sq.b.x = (unsigned char)(a.x - g_lf_footprint.v[0]);
        p1->sq.b.y = (unsigned char)(a.y - g_lf_footprint.v[1]);
    }
    LFRun_AddPiece(st, p1);

    {
        Pos first;
        first.x = p1->sq.b.x;
        first.y = p1->sq.b.y;
        g_lftr_def->footprint = g_lf_footprint;
        g_lftr_def->footprint.v[2] = g_lftr_def->footprint.v[2] - 1;
        g_lftr_def->footprint.v[3] = g_lftr_def->footprint.v[3] - 1;
        AddBasicObject(g_lftr_elem, &first);
    }
    p2 = LFPiece_Alloc();
    p2->run = st;
    p2->def = g_lfen_def;
    p2->sq.w = st->sq.w;
    LFRun_AddPiece(st, p2);
    st->f18 = p2;
    LFPiece_LinkAfter(p1, p2);

    n = (def->footprint.v[3] - def->footprint.v[1] + 1) / dy;
    last = p1;
    b.y = a.y + dy;
    for (i = 0; i < n; i++) {
        p = LFPiece_Alloc();
        if (p != 0) {
            p->kind = 1;
            p->dir  = 0;
            p->parent = p2;
            p->def  = g_lftr_def;
            p->run  = st;
            p->flags |= 7;
            p->sq.b.x = (unsigned char)(a.x - g_lf_footprint.v[0]);
            p->sq.b.y = (unsigned char)(b.y - g_lf_footprint.v[1]);
        }
        if (b.y <= midy && b.y + dy >= midy) {
            st->f08 = p;
            st->f0c = last;
        }
        LFPiece_AddSub(p2, p);
        if (i == 0)
            p2->end_a = p;
        if (i == n - 1)
            p2->end_b = p;
        LFPiece_LinkAfter(last, p);
        last = p;
        b.y += dy;
    }

    fx = def->footprint.v[0];
    fy = def->footprint.v[3];
    b.x = sq.b.x + fx;
    b.y = sq.b.y + fy - 1;
    b.x++;
    b.y += dy;
    p3 = LFPiece_Alloc();
    if (p3 != 0) {
        p3->kind = 3;
        p3->dir  = 2;
        p3->flags |= 3;
        p3->parent = 0;
        p3->def  = g_lftr_def;
        p3->run  = st;
        p3->sq.b.x = (unsigned char)(b.x - g_lf_footprint.v[0]);
        p3->sq.b.y = (unsigned char)(b.y - g_lf_footprint.v[1]);
    }
    LFRun_AddPiece(st, p3);
    {
        Pos tail;
        tail.x = p3->sq.b.x;
        tail.y = p3->sq.b.y;
        AddBasicObject(g_lftr_elem, &tail);
    }
    LFPiece_LinkAfter(last, p3);
    LFRun_Finish(st);
}

/* =========================================================================
 * +0xa8 ACTIVATE -- the rider state machine, one step per rider per frame.
 *
 * First tick every run in the park (LFRun_TickAll walks g_lf_queue), then
 * walk the CLASS-WIDE rider list at ObjDef+0xcc and give every rider whose
 * low-level AI is idle (Bloke+0x0e == 0) one step of its own machine
 * (Bloke+0x60).  The rider's map square is the packed word at RiderNode+0x0c,
 * and (qx,qy) = ObjDef+0x24/+0x25 is the station's queue/exit offset.
 *
 * THE TWELVE STEPS.  Everything is in 24.8 fixed point: a tile is 0x100, so
 * 0x80 is a tile centre and 0x280 is two and a half tiles.
 *
 *   0  JOIN.  Mark "on this ride" (+0x62 bit 8), find the station whose map
 *      square is this rider's, and push the rider onto that station's queue
 *      (which sets bit 0x40 "queueing", zeroes the path index and advances
 *      the rider to step 1).  If the queue is now FULL -- its node count has
 *      reached the capacity the run publishes at queue->cap -- the station's
 *      square is flagged "no more people on" so the ride stops accepting.
 *   1  wait (the queue module drives the rider on from here).
 *   2  WALK TO THE JETTY: two tiles left of the square, on its top edge.
 *   3  WALK TO THE BOAT: two and a half tiles left, half a tile down.
 *   4  BOARD.  Mark "riding" (+0x62 bit 0x80), reset to the walk animation on
 *      frame 0, and release the station: the boat sitting at +0x38 is set to
 *      state 1 (LAUNCHED), the station forgets it, and the run's "loading"
 *      bit (+0x04 bit 0) is cleared so the next boat may come in.  This is
 *      how a boat is launched onto a run: the RIDER launches it, at the
 *      instant it sits down.  A rider that finds no station still advances.
 *   5  RIDE (nothing to do: the boat carries the rider; +0x38 of the run and
 *      the piece code move it).
 *   6  DISEMBARK.  Stop riding, back to the walk animation on frame 0, and
 *      TELEPORT: the world position is set to two and a half tiles left /
 *      half a tile up of the square with facing byte +0x72 = 3, then a walk
 *      is planned to two tiles left / one tile up.
 *   7  WALK OFF the jetty: onto the square itself, one tile up.
 *   8  Hand the rider to the exit PATH (0x004c2af8, the five-frame set) with
 *      the step direction set to -1 and the frame index to the last frame --
 *      i.e. walk the path BACKWARDS out of the station.
 *   9  STEP ALONG that path: the frame's {dx,dy} added to (qx,qy) + square.
 *  10  WALK TO THE QUEUE/EXIT SQUARE, centred in the tile.
 *  11  LEAVE: off the class rider list, clear "on this ride".
 * ========================================================================= */

/* RESIDUAL (10 of 222 instructions, 672 of 673 bytes).  All 222 instructions,
 * the twelve-entry jump table, every block, every constant and every store
 * are reproduced index for index EXCEPT the ten-instruction loop preamble
 * (original indices 14..24), and the whole of that difference is ONE
 * instruction: the original forms tx as `lea ecx,[edx+eax]` (qx in edx,
 * tile x in eax, the sum in a THIRD register) where VC6 here coalesces the
 * sum into qx's own register and emits `add ecx,eax` -- one byte shorter,
 * hence 672 vs 673.  That single choice decides which of edx/ecx carries
 * `next` and therefore renames every register in the preamble.
 *
 * Measured and rejected (two lanes, ~230 variants): all 160 legal statement
 * orders of {b, tile, tx, next, ty} x both operand orders of both sums on
 * this body (best 10, next best 58, most 94-96); explicit `qx`/`qy` int
 * locals, including `qx` with a second use in case 10 (CSE'd back into tx);
 * explicit `x`/`y` locals (int and unsigned char) with case 2 reading `x`;
 * tx via a Pos/struct/array local, a Pos-returning inline helper, unsigned,
 * long; two-def forms (`tx = def->qx; tx += tile->b.x` and every mix);
 * `next` as int, unsigned, void pointer, struct member, array, through a pointer, inner
 * scope, `b` in an inner scope, `for (; r; r = next)`, `b->state == 0`; a
 * `signed char*` view of qx/qy; `volatile` loads of qx (9: only reorders the
 * loads, still `add`), of tile x (breaks the CSE with case 2) and of `next`
 * (8: same).  Case 10 written with `(def->qx + tile->b.x)` CSEs to tx and is
 * byte-identical; case 9 with the expressions is not.  Diagnosis: VC6 gives
 * an `add` the register of whichever operand dies at it (`x` lives on into
 * case 2), even when that operand is a separate volatile load; the original
 * had qx and tx as non-coalesced webs.  No C spelling found that keeps qx
 * alive past the sum or pre-assigns tx a third register.
 *
 * 2026-09-04 (third lane).  Still 10, but the residual REDUCES TO ONE LOAD
 * ORDER, which is a smaller target than "the lea":
 *   original 14..18: movsx edx,[def+24h] (qx) / mov ecx,[r] (next) /
 *                    lea edi,[r+0ch] (tile) / movsx ebx,[def+25h] (qy) /
 *                    mov esi,[r+8] (b)          -- ALTERNATING def, r, def, r
 *   ours     14..18: mov edx,[r] (next) / mov esi,[r+8] (b) /
 *                    movsx ecx,[def+24h] (qx) / movsx ebx,[def+25h] (qy) /
 *                    lea edi,[r+0ch]            -- GROUPED by base register
 * In BOTH bodies the first-emitted scratch load takes edx and the second
 * ecx.  So qx lands in edx in the original and in ecx here, and the rest is
 * forced: with qx in edx the sum cannot be in place and VC6 emits
 * `lea ecx,[edx+eax]` into the register the `next` spill has just freed;
 * with qx in ecx it emits `add ecx,eax`.  Nothing about the sum's spelling
 * has to change - the LOAD ORDER does.  The original's alternation looks
 * like Pentium U/V pairing of two loads off different base registers.
 * Measured this round and still 10 (byte-identical): a `void*` cast on the
 * tile lea, `next = *(RiderNode**)r`, a `RideDef* d = def` local, a `qxl`
 * local read before `b`; `next` moved last is 94, `tx` first 59, `ty`
 * before `tx` 94, and qx/qy through a `Pos` aggregate 207 (the aggregate
 * forces an ebp frame).
 * CORPUS CHECK (scratchpad/sweep4/scan_lea3.py: 47 three-register `lea`s in
 * the matched corpus): the closest analogue is objmap2.c BuildCursorPtr
 * 0x0045f7a5, `lea ecx,[esi+edx] / xor edx,edx / mov dx,[ebp]` - the same
 * "sum into a third register, then immediately recycle one operand's
 * register" shape - and there BOTH operands (sy and h2) really are read
 * again in later blocks.  Here qx is not, so the lea is not explained by
 * liveness at all; it is explained by qx being in the WRONG register.
 * *** THAT LIVENESS CLAIM IS WRONG AND IS RETRACTED (2026-09-04). ***  The
 * widened `tile->b.x` in eax IS live past the sum: case 2's first
 * instruction is `add eax,-2` at 0x0040c01f, reading the SAME eax the
 * preamble loaded at 0x0040bfac, with no reload.  So this site is the same
 * class as the corpus analogue BuildCursorPtr after all - one operand of the
 * sum survives it - and the three-register `lea` is a liveness effect, not
 * an anomaly.  Our body reproduces that liveness (index 41 `add eax,-2`
 * matches); what differs is only which register holds qx, and with eax
 * unavailable the destination falls to the OTHER operand when it is dead
 * (ours, `add ecx,eax`) or to a free register when that operand's register
 * is wanted for the y byte (the original, `lea ecx,[edx+eax]` leaving edx
 * for `xor edx,edx / mov dl,[edi+1]`).  Both bodies are internally
 * consistent; the whole residual is still qx's register.
 *
 * 2026-09-04 (fourth lane).  Still 10, but the LOAD-ORDER diagnosis above is
 * WRONG and is retracted; the residual is one COALESCING decision.
 *  - PROBE (scratchpad/laneF/probe_reg.py over all 40 legal statement
 *    orders): qx lands in ecx in 40 of 40, whatever position it is emitted
 *    in - including the orders where its load is emitted FIRST.  Emission
 *    order does not pick the register.  What picks it is that ecx is `tx`'s
 *    register in every body (ours AND the original, `lea ecx,[edx+eax]` /
 *    case 9 `push ecx` / case 10 `shl ecx,8`), and VC6 coalesces the qx load
 *    into tx's register when it can.  The original does NOT coalesce, so the
 *    sum needs a third register and gets `lea`.
 *  - WHY it cannot coalesce there: in the original ecx belongs to `next`
 *    from index 15 until its spill store at index 21, and qx is loaded at
 *    14, so qx's range overlaps next's and must take edx; tx is born at 22,
 *    after next dies, so tx gets ecx.  Here VC6 schedules the same spill
 *    store IMMEDIATELY after the load run (index 19, before `xor eax,eax`),
 *    which ends next's range before qx's load and lets qx have ecx.  So the
 *    one thing to move is the POSITION OF THE `next` SPILL STORE, not the
 *    position of any load and not the sum's spelling.
 *  - The original's load order IS reachable, and it is not enough:
 *    `tx = *(volatile signed char*)&def->qx; next; tile; ty = ..qy; b;
 *    tx += tile->b.x; ty += tile->b.y;` emits qx / next / lea tile / qy / b
 *    exactly as the original does (scratchpad/laneF/a_V4_vol_qx_first.c) and
 *    is still `add`, because the two-def spelling makes tx and qx ONE web by
 *    construction; X=20, and the volatile also rotates the callee-saved
 *    pushes.  `next` volatile-read (X=8, rb=10) likewise puts qx first and
 *    still coalesces.  Load order and non-coalescing are not obtainable from
 *    the same spelling by anything found.
 *  - CORPUS CHECK #2, AND ITS RETRACTION.  A first scan
 *    (scratchpad/laneF/scan_loadorder2.py) demanded CONSECUTIVE loads and
 *    found only 1 base-alternating run of length >=4 in the 1542 exact
 *    bodies against 3 grouped, which looked like strong evidence the shape
 *    was unreachable.  THAT WAS A SCAN ARTEFACT.  Re-run allowing one
 *    non-load instruction inside a run (scratchpad/laneF/scan_loadorder3.py,
 *    GAP=1) the count is 5 alternating against 9 grouped, and at GAP=2 it is
 *    26 against 14.  Base alternation is ordinary; do not argue from its
 *    absence.  The witnesses the strict scan missed include misc3.c
 *    DrawPopUpMock 0x472149 (edx/ecx/ecx/edx/edx off two live-in bases) and,
 *    decisively, goldrush.c Fort_TickRiders 0x004066ac.
 *  - THE TWIN: `Fort_TickRiders` (goldrush.c, audit-exact) is this function
 *    with the same shape - `rd = item->riders; while (rd) { next = rd->next;
 *    b = rd->bloke; key = (MapSquare*)&rd->ride_id; kx = key->bx;
 *    px = item->base_x + kx; py = item->base_y + key->by;
 *    if (b->state == 0) switch (b->action) ... }` - and its preamble comes
 *    out GROUPED by base exactly as ours does (`mov edx,[edi] /
 *    mov esi,[edi+8] / mov ecx,[ebx+0xc] / xor eax,eax / mov al,[edi+0xc]`).
 *    So the grouped order is what this source shape produces, and the
 *    original's alternation must come from a different source shape, not
 *    from a different compiler mood.  Fort also shows what a NAMED byte
 *    local does here: `kx` (used again in case 1) becomes a callee-saved
 *    ebp with a `mov ebp,eax` copy, and its sum `add ecx,ebp` takes the
 *    INLINE MEMORY REFERENCE's register as the destination, not the local's.
 *    Transplanting Fort's exact `kx` shape onto this body (kx read once,
 *    reused by case 2, sum `def->qx + kx`) is byte-identical to this code
 *    when `next` keeps its position and 95 otherwise - VC6 forward-
 *    substitutes the single-use local.
 *  - Also measured and inert this round: a non-address-taken `Pos` aggregate
 *    carrying tx and ty (byte-identical), the same with a two-def leading
 *    pair, an `((unsigned char*)tile)[0]/[1]` byte view (byte-identical), a
 *    two-argument inline `lf_sum` helper for either or both sums (212-217,
 *    ESCAPES), a (def,tile) helper, `short`/`signed char` tx and ty (173 /
 *    208), a `const signed char* q = &def->qx` local (58), a volatile read
 *    of `b` (10) and of `b` placed last (64), and all 40 orders again with
 *    the tile address inlined into the tx/ty expressions instead of the
 *    `tile` local (188+ - the inlined address does not CSE with `tile`).
 *
 * 2026-09-04 (fifth lane).  Still 10; the previous lane's "the one thing to
 * move is the POSITION OF THE `next` SPILL STORE" is CONFIRMED and the
 * load-order theory is now definitively dead.
 *  - `int qx = *(volatile signed char*)&def->qx;` as a NAMED local consumed
 *    by the sum, with the original's statement order, emits qx FIRST
 *    (index 14 `movsx ecx,[ebx+24h]`, exactly where the original has it) and
 *    is STILL `add ecx,eax`, still 10, 672/673 bytes
 *    (scratchpad/w7small/act_A1.c; act_A2.c is the same with our statement
 *    order and scores identically).  A volatile load cannot be
 *    forward-substituted, so this is the strongest available "separate web"
 *    spelling -- and qx STILL takes ecx.  Conclusion: emission order does
 *    not pick the register and neither does web separation; what picks it is
 *    that ecx is free at qx's load.
 *  - THE MECHANISM, stated exactly.  In the ORIGINAL, `next` occupies ECX
 *    from its load (index 15) until its spill store (index 21), which
 *    straddles qx's live range (14..22), so qx cannot have ecx and takes
 *    edx; tx is then born after next dies and gets ecx, needing the
 *    three-register `lea ecx,[edx+eax]`.  In EVERY body we can produce the
 *    spill store lands immediately after the load run (index 18/19), ecx is
 *    free when qx is loaded, qx coalesces into tx's ecx, and the sum is
 *    `add ecx,eax` -- one byte shorter.  The target is therefore a source
 *    shape that delays the `next` spill store past the `xor eax,eax /
 *    mov al,[edi]` pair, i.e. one basic-block scheduling slot.
 *  - Also measured this round and worse: `cur = r; r = r->next;` with `r`
 *    as the spilled loop variable and `cur` in ebp (212, ESCAPES); the
 *    `tile->key` 16-bit view with `(unsigned char)(w >> 8)` for the y byte
 *    (213, ESCAPES); `((unsigned char*)&r->ride_id)[1]` for the y byte (10,
 *    rb 7 -- byte-identical to the base); the tx sum written inline off
 *    `((RideTile*)&r->ride_id)->b.x` with the tile local kept for y (206).
 *  - Secondary observation for whoever picks this up: the original computes
 *    the x group and the y group SEQUENTIALLY (`xor eax,eax / mov al,[edi] /
 *    <spill> / lea ecx / xor edx,edx / mov dl,[edi+1] / add edx,ebx`) while
 *    every body here hoists both `xor`s together
 *    (`xor eax / xor edx / mov al / mov dl / add / add`).  That grouping is
 *    the same "VC6 groups the two byte-field reads of one 2-byte object"
 *    effect recorded for `key.b.x`/`key.b.y`, and breaking it is probably
 *    the same edit as delaying the spill store.
 *
 * 2026-09-05 (sixth lane).  Still 10.  The fifth lane's mechanism is
 * CONFIRMED and sharpened, and the search space is narrowed by a negative
 * that should stop the next lane re-running spellings.
 *  - RESTATED: every register in the window follows from ONE binary choice.
 *    `tx` is ecx and `ty` is edx in BOTH bodies; what differs is which of
 *    {qx, next} takes which.  With qx in ecx (ours) the sum is in place
 *    (`add ecx,eax`, 2 bytes) and `next` in edx must be spilled before
 *    `xor edx,edx`, which is why our spill store lands at index 19; with qx
 *    in edx (the original) the sum needs a third register
 *    (`lea ecx,[edx+eax]`, 3 bytes -- the whole 672/673 byte difference)
 *    and `next` in ecx is spilled at index 21, one slot before the lea.
 *    The store position is an EFFECT of the register, not a cause: in both
 *    bodies the spill sits exactly where its register is next needed.
 *  - The real requirement is therefore "qx must be a web SEPARATE from tx".
 *    qx is a single-use operand that dies at the sum, so VC6 evaluates the
 *    whole tree into tx's register.  Confirmed again this round: the 12
 *    remaining statement orders not covered by the fifth lane's 40 (all
 *    still 10 or 59-118), both operand orders of BOTH sums (`tile->b.x +
 *    def->qx`, `def->qy + tile->b.y`, and both together -- all three
 *    BYTE-IDENTICAL to the base), and a named `qxl` volatile local placed
 *    first (still ecx).
 *  - Two shapes DO break the coalescing and both need a store shim, so
 *    neither is committed: `next` held in a one-field struct written through
 *    `*(RiderNode* volatile*)&nx.p` scores 9 (rb 6) and emits qx FIRST at
 *    index 14 -- the original's emission order -- while still giving qx
 *    ecx; and a volatile READ of `r->next` scores 8.  Both are strictly
 *    better than 10 and both are shims, recorded here only as evidence that
 *    the emission order and the coalescing are INDEPENDENT: getting qx
 *    emitted first is easy, getting it a register of its own is what is not
 *    reachable.  What is wanted is a plain-C reason for qx to be live past
 *    the sum, and the function offers none (case 10 re-reads qy from a
 *    reloaded `def`, and qx is never read again).
 */
/* SCOPE H FIRST PASS, 2026-09-05.  Unchanged at 10/222 mismatches, first
 * index 14, 672/673B.  Four bounded tests of the newer named-pointer lever:
 * name &r->ride_id before the RideTile cast; name &tile->b for the two sums;
 * name &r->next before loading next; name the current node before its three
 * field reads.  All four reproduce the baseline instruction stream exactly.
 * The original preamble and every case were re-read; no reconstruction error
 * found.  A named field-address or node pointer does not separate qx from tx
 * here.  No previously recorded volatile shim was added or re-swept.
 * Evidence: scratchpad/scope-h/animation/activate-*-sbs.txt. */
/* SCOPE H, 2026-09-06 (this round).  Still ten, first index 14, 672/673B; no
 * spelling committed.  The round did NOT sweep the divergence again -- it
 * re-derived the register decision from the disassembly and from an exact
 * twin, and the result RETIRES the "coalescing" framing that the fourth,
 * fifth and sixth lanes worked under.
 *
 * THE RULE (three independent witnesses, all measured here).  When the two
 * dead scratch temps of this block -- `qx` (the movsx of def->qx) and `next`
 * (the load of r->next that feeds the home store) -- have OVERLAPPING ranges
 * in the emitted block, VC6 gives EDX to the one whose load is emitted
 * FIRST and ECX to the other.  When they do not overlap, both take ECX.
 *   - ours (b, tile, tx, next, ty): `mov edx,[ebp]` at 14, `movsx ecx,
 *     [ebx+24h]` at 16, store at 19 -> ranges overlap, next is first -> next
 *     EDX, qx ECX, and the sum folds in place, `add ecx,eax` (2 bytes).
 *   - goldrush.c Fort_TickRiders 0x004066ac, AUDIT-EXACT and the same source
 *     shape: `mov edx,[edi]` (next) at +0, `mov ecx,[ebx+0ch]` (item->base_x)
 *     at +2, next's store five slots later -> overlap, next first -> next
 *     EDX, base_x ECX, `add ecx,ebp`.
 *   - THE ORIGINAL: `movsx edx,[ebx+24h]` (qx) at 14, `mov ecx,[ebp]` (next)
 *     at 15, store at 21 -> overlap, QX FIRST -> qx EDX, next ECX, and the
 *     sum needs a third register, `lea ecx,[edx+eax]` (3 bytes -- the whole
 *     672/673 byte difference).
 *   So the crosswise packing is NOT a coalescing decision, not a rank
 *   decision and not a liveness decision.  It is one thing only: WHICH OF
 *   THE TWO LOADS THE LIST SCHEDULER EMITS FIRST.  The late spill store, the
 *   sequential x-then-y groups at 19-25 and the three-register lea are all
 *   downstream of that single swap; get `movsx edx,[ebx+24h]` emitted before
 *   `mov ecx,[ebp]` and the other nine mismatches close with it.
 * CONFIRMED BY CONSTRUCTION: a probe that gives `next` a second use after
 * the sums (`tx += (int)next & 1`, semantics-changing, not committed)
 * reproduces the original's `mov ecx,[ebp]` at index 15 with next in ECX.
 * An extended next range does produce the original's packing; this function
 * offers no semantics-preserving second use, because every other read of
 * `next` is after a call and therefore reads the home, not the register.
 *
 * WHAT IS NOW KNOWN TO BE UNREACHABLE.  In every non-volatile spelling this
 * body admits, the scheduler emits the `next` load FIRST in the loop-body
 * block; it is the highest-priority ready node of the DAG and no source
 * permutation demotes it.  Measured this round, all inert or worse:
 *   - all 40 legal statement orders re-run (best is the committed BTXNY=10;
 *     next best TBXNY/TXBNY/TXNBY=59 at 669B, then 63, 92-124).  Three
 *     orders reach the right byte count (TBNXY/TNBXY/TNXBY, 673B) but at 118.
 *   - the tile assignment EMBEDDED in the tx expression so that `tx` is the
 *     first statement (`tx = def->qx + (tile = (RideTile*)&r->ride_id)->b.x`)
 *     -- 10 with b second/third, and with `b` last it reaches rb=9/672B but
 *     flips the callee-saved assignment order (esi takes `def`), first=2.
 *     Same for `if (!(b = r->bloke)->state)`.  `b` must be defined before
 *     `tile` in the source or the prologue breaks; that constraint plus
 *     "tile before both sums" leaves only the 40 orders already swept.
 *   - no `tile` local at all, the cast inlined at every use, all 24 orders:
 *     77 at best, and the entry block moves (first=2/3).
 *   - block-scoped declarations inside the loop, with and without a comma
 *     initialiser carrying `next` (both byte-identical to the base).
 *   - every "temp plus a late store" shape: `{RiderNode* t = r->next; tx =
 *     ...; next = t;}` is BYTE-IDENTICAL to the base and `{int q = def->qx;
 *     next = r->next; tx = q + tile->b.x;}` compiles to the plain next-first
 *     order (95).  VC6 forward-substitutes a single-use local across a store
 *     to another local's home, so the load always sinks to its use.
 *   - an ADDRESSABLE home for `next` does not block that substitution: a
 *     one-element array `RiderNode* next[1]` and a one-field struct written
 *     as `nx.p` both behave exactly like the scalar (10 / 95 / 94).
 *   - rank probes: `tx` given ONE reference (case 10 recomputing the x sum)
 *     is inert at 10; `ty` given TWO (case 10 using ty) is catastrophic at
 *     207 -- ty escapes to a callee-saved ebp.  So neither temp's register
 *     is decided by the variable's reference count.
 *   - IL-symbol views, both objects: a local `struct {unsigned char x,y;}`
 *     view of `tile->b` (named local and inline cast), `*(const unsigned
 *     char*)&tile->b.x` / `&tile->b.y`, an inline `struct {signed char x,y;}`
 *     view of `def->qx` -- all BYTE-IDENTICAL; the same view through a NAMED
 *     pointer local is 58.  `next` through a folding non-zero-offset view
 *     (`*(RiderNode**)((char*)&r->prev - 4)`) is just the next-first order.
 *   - both sum operand orders (`tile->b.x + def->qx`) byte-identical.
 *   - `for (r = def->riders; r; r = next)` only sinks the riders load past
 *     LFRun_TickAll (12, first=6); no `b` local at all is 190.
 * CORPUS: a scan of every `// FUNCTION:` body in LEGOLAND/*.c for a
 * three-register `lea rD,[rA+rB]` with a byte-load temp as an operand finds
 * ZERO instances.  The shape at index 22 has no witness anywhere in the
 * matched corpus, so there is nothing to copy; it must be derived.
 * VOLATILE SHIMS re-measured, NOT committed: a volatile read of `r->next` in
 * the base position scores 8 (it pins the load AFTER the sum and indices 16
 * and 23-25 fall into place); both loads volatile scores 9; a volatile `qx`
 * alone stays at 10.  None of them is the original's shape -- they reach a
 * different local optimum (next late) rather than the original's (next early
 * with a long range).
 * NEXT LANE: the target is one sentence -- make the list scheduler emit the
 * `def->qx` load before the `r->next` load in the loop-body block, with the
 * two ranges still overlapping.  Do not re-sweep statement orders, symbol
 * identity, pointer/byte views, operand orders, storage classes, loop shapes,
 * declaration scope or reference counts; all are measured dead above.
 * CLOSED (fgh-100b, 222/222, 673/673): it took TWO of the "dead" levers at
 * once.  Case 10 spells its x target as `((def->qx + tile->b.x) << 8) + 0x80`
 * -- VC6 CSEs that with the head's sum, so the code is the same but `tx` is
 * now a one-reference local -- AND the head reads `next` before `tx`.  With
 * both, `next` is allocated first and takes ecx for six instructions, the
 * `qx` load can no longer coalesce into `tx`'s register and goes to edx, and
 * the sum becomes the original's `lea ecx,[edx+eax]` after the spill.  Either
 * change alone is inert at 10 (the rank probe above measured the case-10
 * change with the old head order).  Differential execution 1200/1200.
 */
// FUNCTION: LEGOLAND 0x0040bf70
void LFEntrance_Activate(RideElem* elem)
{
    RideDef*      def = elem->data;
    RiderNode*    r   = def->riders;
    RiderNode*    next;
    RideTile*     tile;
    LFRun*        st;
    Bloke*        b;
    int           tx;
    int           ty;
    unsigned char dir;

    LFRun_TickAll();
    while (r) {
        b    = r->bloke;
        tile = (RideTile*)&r->ride_id;
        next = r->next;
        tx   = def->qx + tile->b.x;
        ty   = tile->b.y + def->qy;
        if (!b->state) {
            switch (b->action) {
            case 0:
                b->flags |= 8;
                st = LFStation_FindAt(tile);
                if (st) {
                    LFQueue_AddRider(&st->queue, r);
                    if (LFQueue_IsFull(&st->queue))
                        Ride_SetFlagToNotLetAnyoneOn(&st->sq);
                }
                break;
            case 2:
                b->target.x = (tile->b.x - 2) << 8;
                b->target.y = tile->b.y << 8;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 3:
                b->target.x = (tile->b.x << 8) - 0x280;
                b->target.y = (tile->b.y << 8) + 0x80;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 4:
                b->flags |= 0x80;
                BlokeWalkAnim(b);
                BlokeSetFrame(b, 0);
                st = LFStation_FindAt(tile);
                if (st) {
                    if (st->boat) {
                        st->boat->state = 1;
                        st->boat = 0;
                    }
                    st->flags &= ~1u;
                }
                b->action++;
                break;
            case 6:
                b->flags &= (unsigned short)~0x80u;
                BlokeWalkAnim(b);
                BlokeSetFrame(b, 0);
                b->world.x = (tile->b.x << 8) - 0x280;
                b->world.y = (tile->b.y << 8) - 0x80;
                b->b72 = 3;
                b->target.x = (tile->b.x - 2) << 8;
                b->target.y = (tile->b.y << 8) - 0x100;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 7:
                b->target.x = tile->b.x << 8;
                b->target.y = (tile->b.y << 8) - 0x100;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 8:
                LFPath_StartReverse(g_lf_anim_b, b);
                break;
            case 9:
                LFPath_StepBloke(g_lf_anim_b, tx, ty, b);
                break;
            case 10:
                b->target.x = ((def->qx + tile->b.x) << 8) + 0x80;
                b->target.y = ((tile->b.y + def->qy) << 8) + 0x80;
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;
            case 11:
                RemoveBlokeFromRide(def, r);
                b->flags &= (unsigned short)~8u;
                break;
            }
        }
        r = next;
    }
}

/* =========================================================================
 * +0xb0 INTERACT -- the station's own draw pass.
 *
 * Like every ride whose build sprite carries flag 0x2000, the entrance
 * paints itself instead of letting the sprite walk blit the class image, and
 * it is the most elaborate of them: the station building is drawn in five
 * horizontal SLICES with the queue split into five DEPTH BANDS between them,
 * so a visitor standing in the queue is occluded by exactly the part of the
 * building in front of him.
 *
 * The bands are cut by the rider's world position against the station square
 * (tx,ty) = ObjDef base + the square, all in 24.8:
 *
 *   band 0  world.x <= (tx<<8) - 0x280            (left of the jetty)
 *   band 1  world.x >  (tx<<8) - 0x280 and (world.y>>8) <= ty-9
 *   band 2  ... and (world.y>>8) is ty-6, ty-7 or ty-8
 *   band 3  ... and (world.y>>8) == ty-3
 *   band 4  ... and (world.y>>8) is ty, ty-1 or ty-2
 *
 * and each band is a fresh RenderItems_New / list-head reset / walk of the
 * CLASS rider list (ObjDef+0xcc) keeping only riders on THIS square who are
 * not currently riding (+0x62 bit 0x80), then RenderBlokeList.  Between the
 * bands go the four building sprites lf_entrance3, lf_entrance1,
 * lf_entrance2 and lf_sign, each at a fixed offset (-100,-209) except
 * lf_entrance1 at (8,-113), and the whole thing is closed by
 * lf_enta1_matte2 at the class's own layer-2 render offset.
 *
 * ORIGINAL BUG, reproduced verbatim: `frame` is never initialised and never
 * stored.  It is set only inside `if (st)` and inside the `if (lls)` arms,
 * so on every path that misses those the code reads the variable's home --
 * which VC6 placed on top of the LIVE `sq` argument slot, so the "animation
 * frame" handed to LLSSetFrame (and the flag that decides whether the barrel
 * matte is drawn at all) is the address of the map square.  It is non-zero,
 * which is why the first slice always draws.  Written here the way the
 * original reads: an uninitialised local, with the fallback spelled out.
 * ========================================================================= */

typedef struct Offset { int ox; int oy; } Offset;
typedef struct LLS    { short frame; } LLS;

typedef struct RenderList { void* head; } RenderList;

extern Offset GetScreenCoordsForObject(RideTile* sq, RideDef* def); /* 0x00442cc0 */
extern Offset GetRenderOffsetForLayer(void* sprite, int layer);     /* 0x00441ee0 */
extern void*  GetSpriteForLayer(void* sprite, int layer);           /* 0x00441ec0 */
extern LLS*   GetLLSForSprite(void* spr);                           /* 0x00441e80 */
extern void   LLSSetFrame(LLS* lls, int frame);                     /* 0x0047d5a0 */
extern void   AdjustOffsetForViewMode(Offset* o);                   /* 0x00442d30 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* c);/* 0x004853a0 */
extern void   RenderItems_New(void);                                /* 0x00442e90 */
extern void   AddBlokeToRenderList(RenderList* l, RiderNode* r, int key);/* 0x00442f20 */
extern void   RenderBlokeList(RenderList* l);                       /* 0x00442f70 */
extern int    LFPiece_HasRider(LFPiece* p);                         /* 0x0040b390 */

extern RenderList g_lf_bloke_list;      /* 0x004cbe70 */
extern void*      g_spr_enta3_m;        /* 0x004cbe4c  "enta3_m.lls" */
extern void*      g_spr_ent_matte2;     /* 0x004cbe88  "lf_enta1_matte2.lls" */
extern void*      g_spr_sign;           /* 0x004cbe8c  "lf_sign.lls" */
extern void*      g_spr_ent2;           /* 0x004cbe90  "lf_entrance2.lls" */
extern void*      g_spr_ent3;           /* 0x004cbe94  "lf_entrance3.lls" */
extern void*      g_spr_ent1;           /* 0x004cbe98  "lf_entrance1.lls" */

// FUNCTION: LEGOLAND 0x0040b420
void LFEntrance_Interact(RideElem* elem, int x, int y, RideTile* sq,
                         void* clip, int mode)
{
    RideDef*   def = elem->data;
    Pos        t;
    Offset     screen;
    LFRun*     st;
    RiderNode* r;
    Bloke*     bl;
    int        have;

    t.x = def->base_x + sq->b.x;
    t.y = sq->b.y + def->base_y;
    st = LFStation_FindAt(sq);
    screen = GetScreenCoordsForObject(sq, def);
    if (st)
        have = LFPiece_HasRider(st->f18);
    if (have) {
        int    frame;               /* uninitialised -- see the note above */
        LLS*   lls;
        Offset s;
        Offset off;

        lls = GetLLSForSprite(GetSpriteForLayer(def->sprite, 0));
        if (lls)
            frame = lls->frame;
        lls = GetLLSForSprite(g_spr_enta3_m);
        if (lls)
            LLSSetFrame(lls, frame);
        s = GetScreenCoordsForObject(sq, def);
        off = GetRenderOffsetForLayer(def->sprite, 0);
        AdjustOffsetForViewMode(&off);
        if (g_spr_enta3_m)
            PrintSprite(g_spr_enta3_m, s.ox + off.ox, s.oy + off.oy, mode, 0);
    }

    {
        void*  spr = GetSpriteForLayer(def->sprite, 1);
        Offset s   = GetScreenCoordsForObject(sq, def);
        Offset off;

        off = GetRenderOffsetForLayer(def->sprite, 1);
        AdjustOffsetForViewMode(&off);
        PrintSprite(spr, s.ox + off.ox, s.oy + off.oy, mode, 0);
    }

    /* band 0 -------------------------------------------------------- */
    RenderItems_New();
    g_lf_bloke_list.head = 0;
    r = def->riders;
    while (r) {
        if (sq->key == r->ride_id) {
            bl = r->bloke;
            if (!(bl->flags & 0x80) && bl->world.x <= (t.x << 8) - 0x280)
                AddBlokeToRenderList(&g_lf_bloke_list, r, r->owner->key);
        }
        r = r->next;
    }
    RenderBlokeList(&g_lf_bloke_list);

    /* band 1 -------------------------------------------------------- */
    RenderItems_New();
    g_lf_bloke_list.head = 0;
    r = def->riders;
    while (r) {
        if (sq->key == r->ride_id) {
            bl = r->bloke;
            if (!(bl->flags & 0x80) && bl->world.x > (t.x << 8) - 0x280
                    && (bl->world.y >> 8) <= t.y - 9)
                AddBlokeToRenderList(&g_lf_bloke_list, r, r->owner->key);
        }
        r = r->next;
    }
    RenderBlokeList(&g_lf_bloke_list);

    if (g_spr_ent3) {
        int    frame;            /* uninitialised -- see the note above */
        LLS*   lls;
        void*  spr;
        Offset off;

        off.ox = -100;
        off.oy = -209;
        spr = GetSpriteForLayer(def->sprite, 2);
        if (spr) {
            lls = GetLLSForSprite(spr);
            if (lls)
                frame = lls->frame;
        }
        AdjustOffsetForViewMode(&off);
        lls = GetLLSForSprite(g_spr_ent3);
        if (lls)
            LLSSetFrame(lls, frame);
        PrintSprite(g_spr_ent3, off.ox + screen.ox, off.oy + screen.oy,
                    mode, 0);
    }

    /* band 2 -------------------------------------------------------- */
    RenderItems_New();
    g_lf_bloke_list.head = 0;
    r = def->riders;
    while (r) {
        if (sq->key == r->ride_id) {
            bl = r->bloke;
            if (!(bl->flags & 0x80) && bl->world.x > (t.x << 8) - 0x280
                    && ((bl->world.y >> 8) == t.y - 6
                        || (bl->world.y >> 8) == t.y - 7
                        || (bl->world.y >> 8) == t.y - 8))
                AddBlokeToRenderList(&g_lf_bloke_list, r, r->owner->key);
        }
        r = r->next;
    }
    RenderBlokeList(&g_lf_bloke_list);

    if (g_spr_ent1) {
        int    frame;            /* uninitialised -- see the note above */
        LLS*   lls;
        void*  spr;
        Offset off;

        off.ox = 8;
        off.oy = -113;
        spr = GetSpriteForLayer(def->sprite, 2);
        if (spr) {
            lls = GetLLSForSprite(spr);
            if (lls)
                frame = lls->frame;
        }
        AdjustOffsetForViewMode(&off);
        lls = GetLLSForSprite(g_spr_ent1);
        if (lls)
            LLSSetFrame(lls, frame);
        PrintSprite(g_spr_ent1, off.ox + screen.ox, off.oy + screen.oy,
                    mode, 0);
    }

    /* band 3 -------------------------------------------------------- */
    RenderItems_New();
    g_lf_bloke_list.head = 0;
    r = def->riders;
    while (r) {
        if (sq->key == r->ride_id) {
            bl = r->bloke;
            if (!(bl->flags & 0x80) && bl->world.x > (t.x << 8) - 0x280
                    && (bl->world.y >> 8) == t.y - 3)
                AddBlokeToRenderList(&g_lf_bloke_list, r, r->owner->key);
        }
        r = r->next;
    }
    RenderBlokeList(&g_lf_bloke_list);

    if (g_spr_ent2) {
        int    frame;            /* uninitialised -- see the note above */
        LLS*   lls;
        void*  spr;
        Offset off;

        off.ox = -100;
        off.oy = -209;
        spr = GetSpriteForLayer(def->sprite, 2);
        if (spr) {
            lls = GetLLSForSprite(spr);
            if (lls)
                frame = lls->frame;
        }
        AdjustOffsetForViewMode(&off);
        lls = GetLLSForSprite(g_spr_ent2);
        if (lls)
            LLSSetFrame(lls, frame);
        PrintSprite(g_spr_ent2, off.ox + screen.ox, off.oy + screen.oy,
                    mode, 0);
    }

    /* band 4 -------------------------------------------------------- */
    RenderItems_New();
    g_lf_bloke_list.head = 0;
    r = def->riders;
    while (r) {
        if (sq->key == r->ride_id) {
            bl = r->bloke;
            if (!(bl->flags & 0x80) && bl->world.x > (t.x << 8) - 0x280
                    && ((bl->world.y >> 8) == t.y - 2
                        || (bl->world.y >> 8) == t.y - 1
                        || (bl->world.y >> 8) == t.y))
                AddBlokeToRenderList(&g_lf_bloke_list, r, r->owner->key);
        }
        r = r->next;
    }
    RenderBlokeList(&g_lf_bloke_list);

    if (g_spr_sign) {
        int    frame;            /* uninitialised -- see the note above */
        LLS*   lls;
        void*  spr;
        Offset off;

        off.ox = -100;
        off.oy = -209;
        AdjustOffsetForViewMode(&off);
        spr = GetSpriteForLayer(def->sprite, 2);
        if (spr) {
            lls = GetLLSForSprite(spr);
            if (lls)
                frame = lls->frame;
        }
        lls = GetLLSForSprite(g_spr_sign);
        if (lls)
            LLSSetFrame(lls, frame);
        PrintSprite(g_spr_sign, off.ox + screen.ox, off.oy + screen.oy,
                    mode, 0);
    }

    if (g_spr_ent_matte2) {
        Offset off;

        off = GetRenderOffsetForLayer(def->sprite, 2);
        AdjustOffsetForViewMode(&off);
        PrintSprite(g_spr_ent_matte2, off.ox + screen.ox,
                    off.oy + screen.oy, mode, 0);
    }
}
