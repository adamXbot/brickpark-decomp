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

/* RESIDUAL: ONE register choice.  Every block, constant, call, store and
 * frame slot of the original is reproduced; the original keeps `st` in ebx
 * (pushed at entry) and `p1` -- reused for `i` -- in ebp (the deferred push),
 * this body has them the other way round, and because the scratch rotation
 * follows that choice ~200 of the 256 instructions differ index-for-index.
 * Two secondary residuals: (a) the original hoists `n - 1` into its own
 * spill slot after the loop guard (`dec eax / mov [esp+0x18],eax`, then
 * `cmp ebp,[esp+0x18]` in the loop) where VC6 here re-derives it inside the
 * loop (`mov ecx,[n] / lea eax,[ecx-1]`) -- hence the frame is 0x24 not
 * 0x28 and n/def/midy sit one slot lower; (b) the scheduler then folds the
 * loop-end differently.  The unreferenced slot at entry-0x14 is not a
 * variable: it is the 8-byte alignment hole between the 4-byte `sq` slot and
 * the Pos `b`, and this body has it too.
 *
 * What IS load-bearing and committed (each verified against the listing):
 *  - `def->footprint.v[0]`/`v[1]` read into int temps BEFORE the a.x/a.y
 *    stores, then `a.x++; a.y -= dy;` as separate statements: `a` is
 *    address-taken, so a load through `def` after an a.x store cannot be
 *    hoisted and would force a second a.x store (the sibling
 *    LFEntrance_Update has that double store; the original here does not).
 *    With the temps, indices 26..53 are exact modulo the ebx/ebp swap.
 *  - the tail square: x fully (add, inc) then y (`+ v[3] - 1` then `+= dy`
 *    keeps the `lea [edx+ecx-1] / add ecx,esi` association).
 *  - a separate `last` cursor (p1 as the cursor keeps it in a register and
 *    spills `i` instead; the original spills the cursor into the dead `elem`
 *    slot and keeps `i` in the p1 register).
 *
 * The st/p1 swap is a priority tie: measured with probes (not committed), it
 * flips to the original's allocation with +2 unconditional stores through
 * `st` outside the loop, +3 stores through `st` inside the loop, -2 of the
 * `p1->sq` byte reads, an `unsigned char` loop counter, or an inlined
 * allocating helper whose int arguments are live across LFPiece_Alloc; +1
 * ref, +2 loop refs, +2 conditional refs, -1 ref and -2 if-block stores do
 * not.  No legal spelling found that supplies the missing weight: redundant
 * `if (st)` guards, `p2->run`/`p3->run` forwarded as the call argument,
 * `st->f18` as the LinkAfter argument, byte-wise `sq` copy, p1 stamped via
 * an inlined helper (parameter or local copy), post-guard inner scopes for
 * any subset of locals, `i = 0` at nine earlier points (memory-homes i),
 * five `if (p1)`/`if (!p1)` spellings, `RideTile sq`, every order of the
 * four opening statements (dy, midy, queue.path, a) on this body, do/while
 * and while forms, unsigned/long/char counters.  For (a), ~40 spellings of
 * `n - 1` (named local, two-def `nm1 = n; nm1--`, const, block-scope,
 * unsigned/long/short/char, pointer, struct/array member, address-taken,
 * inline-helper parameter, `i + 1 == n`, `i <= n - 1`, `i < nm1 + 1`,
 * `n - i == 1`, static, register, a second dead use) all forward-substitute
 * into the loop; only `volatile`, `short` or a use after the loop keep the
 * slot, none with the original's memory-operand `cmp`.  M1-style helpers
 * and `volatile` were rejected as not the original's code.
 *
 * 2026-09-04 (third lane) -- A DECISIVE DIAGNOSTIC, not committable.
 * Declaring the loop counter `unsigned char i;` and changing NOTHING else
 * produces the ORIGINAL'S REGISTER ALLOCATION AND FRAME EXACTLY:
 *     sub esp,0x28 / push ebx / push esi / push edi ... mov ebx,eax (st) ...
 *     push ebp (deferred) / mov ebp,[...] (the scratch)
 * i.e. `st` in ebx pushed at entry and the p1/i web in the deferred ebp,
 * and the `n - 1` spill slot appears on its own (both secondary residuals
 * (a) and (b) go with it).  220 -> 157, first divergence 13, and indices
 * 0..12 and 14..34 are exact.  It is NOT the original's type -- the body
 * comes out 833 bytes against 812 because every use of `i` is widened, and
 * the original's loop test is the dword `cmp ebp,[esp+18h]` -- so it is left
 * OUT of the committed body; what it proves is the DIRECTION: the tie is
 * decided by the weight of the p1/i web against `st`, and making the counter
 * a byte is enough to tip it.  With the allocation right the only remaining
 * head differences are two SPILL-SLOT offsets (`def` at [esp+14h] in the
 * original, [esp+1ch] here; midy at [esp+40h] vs [esp+3ch]), so a frame-slot
 * lever would be the next thing to look for once the tie is broken legally.
 * `signed char i` is 187, `char` 187, `short` 205, `unsigned short` 199 --
 * all flip the frame to 0x28 but only the two `char` forms flip the
 * registers.  `unsigned`/`long` i, `short`/`unsigned short`/`long` n,
 * `i != n`, `++i`, a hand-rolled `while` loop, a named `nm1` local, an extra
 * `st->queue.path` store after the guard or after LFRun_AddPiece, reading
 * `p1->sq` once through a `BPosW` local (207), `last = p1` hoisted so
 * LFPiece_LinkAfter takes `last` (219) and `if (last == p1)` in place of
 * `if (i == 0)` (207) all leave the swap in place; combining the
 * reference-count reductions with the byte counter is worse (200-201, and
 * the frame grows to 0x2c). */
// WIP-FUNCTION: LEGOLAND 0x0040a600  (14% by audit.py, 220/256 index mismatches, body 250 insns; st/p1 in the opposite callee-saved pair, n-1 not hoisted)
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
    fy = def->footprint.v[1];      /* `a` is address-taken, def may alias */
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

    b.x = p1->sq.b.x;
    b.y = p1->sq.b.y;
    g_lftr_def->footprint = g_lf_footprint;
    g_lftr_def->footprint.v[2] = g_lftr_def->footprint.v[2] - 1;
    g_lftr_def->footprint.v[3] = g_lftr_def->footprint.v[3] - 1;
    AddBasicObject(g_lftr_elem, &b);

    p2 = LFPiece_Alloc();
    p2->run = st;
    p2->def = g_lfen_def;
    p2->sq.w = st->sq.w;
    LFRun_AddPiece(st, p2);
    st->f18 = p2;
    LFPiece_LinkAfter(p1, p2);

    last = p1;
    n = (def->footprint.v[3] - def->footprint.v[1] + 1) / dy;
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
    b.x++;
    b.y = sq.b.y + fy - 1;
    b.y += dy;
    p3 = LFPiece_Alloc();
    if (p3 != 0) {
        p3->flags |= 3;
        p3->kind = 3;
        p3->dir  = 2;
        p3->parent = 0;
        p3->def  = g_lftr_def;
        p3->run  = st;
        p3->sq.b.x = (unsigned char)(b.x - g_lf_footprint.v[0]);
        p3->sq.b.y = (unsigned char)(b.y - g_lf_footprint.v[1]);
    }
    LFRun_AddPiece(st, p3);
    a.x = p3->sq.b.x;
    a.y = p3->sq.b.y;
    AddBasicObject(g_lftr_elem, &a);
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
 * liveness at all; it is explained by qx being in the WRONG register. */
// WIP-FUNCTION: LEGOLAND 0x0040bf70  (95.5%, 222/222 insns; the tx sum is `add` not `lea`, which renames the 10-instruction preamble)
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
        tx   = def->qx + tile->b.x;
        next = r->next;
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
                b->target.x = (tx << 8) + 0x80;
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
