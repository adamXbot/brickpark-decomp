/* LEGOLAND -- the LOG FLUME's piece-tree save/load pair, its per-boat step,
 * and the two small piece draw helpers logflume2.c and lfentrance.c reach
 * through externs.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). None of
 * these functions is exported; every extent comes from the disassembly by
 * control flow (tools/audit.py). Struct field OFFSETS, record sizes and
 * global addresses are load-bearing; the names are ours. Types are declared
 * LOCALLY on purpose (legoland.h is owned elsewhere) and follow logflume.c,
 * logflume2.c, logflume4.c and lfentrance.c.
 *
 * =========================================================================
 * WHAT THIS FILE ADDS TO THE PICTURE
 * =========================================================================
 * THE PIECE TREE IS SAVED SEPARATELY FROM THE RUN, AND RECURSIVELY.
 * logflume.c's SaveLogFlume/LoadLogFlume write one 0xd4-byte run record per
 * station and call 0x00410800 / 0x00410a50 for the pieces. Those two are a
 * mirror-image pair and they do far more than the "renumbers the run's piece
 * list" that logflume.c's header guesses:
 *
 *   0x00410800  for every piece of the list, write a dword 1, a 0x38-byte
 *               copy of the piece with its four ROUTE pointers (+0x08 fwd,
 *               +0x0c back, +0x30 end_a, +0x34 end_b) turned into indices,
 *               then the length and the bytes (no NUL) of the piece class's
 *               NAME, then RECURSE into the piece's sub-list (+0x2c); a
 *               dword 0 ends each list.
 *   0x00410a50  reads the same stream back, allocating each 0x38-byte piece,
 *               chaining it through +0x00, stamping the parent (+0x24 run,
 *               +0x28 the caller's `mode`, +0x04 the previous piece), looking
 *               the class name up in the LLIDB and recursing for +0x2c.
 *
 * So a run's pieces form a TREE, not a list: every piece may own a sub-list
 * of pieces (the sub-pieces LFPiece_HasRider walks below), and the save
 * format is a pre-order walk of it.
 *
 * The 0x0040bbb0 boat step and the two draw helpers are documented above
 * their own bodies.
 * ========================================================================= */

#include <string.h>

#pragma intrinsic(strlen)

/* ---- shared types (same offsets as logflume.c / logflume4.c) ------------ */
typedef struct Pos  { int x; int y; } Pos;
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

typedef struct RiderNode RiderNode;
typedef struct LFRun     LFRun;
typedef struct LFPiece   LFPiece;
typedef struct SpriteRec SpriteRec;

/* The class element the LLIDB hands out: its first field is the class NAME,
 * which is what the piece save stream carries. */
typedef struct RideElem {
    char*        name;          /* +0x00 */
    char*        image;         /* +0x04 */
    unsigned int flags;         /* +0x08 */
    void*        data;          /* +0x0c  the RideDef */
} RideElem;

typedef struct RideDef {
    unsigned char pad00[0x14];
    int           f14;          /* +0x14  the class's two draw parameters */
    int           f18;          /* +0x18 */
    unsigned char pad1c[0xc4 - 0x1c];
    RideElem*     elem;         /* +0xc4  back-pointer to the class element */
} RideDef;

/* A PLACED PIECE of log flume: 0x38 bytes (logflume2.c). */
struct LFPiece {
    LFPiece*      next;         /* +0x00  next piece of the same list */
    LFPiece*      prev;         /* +0x04 */
    LFPiece*      fwd;          /* +0x08  next piece ALONG THE ROUTE */
    LFPiece*      back;         /* +0x0c  previous piece along the route */
    unsigned int  flags;        /* +0x10 */
    BPosW         sq;           /* +0x14  the piece's map square */
    unsigned char pad16[2];
    int           kind;         /* +0x18 */
    int           dir;          /* +0x1c  orientation / variant, 0..3 */
    RideDef*      def;          /* +0x20 */
    LFRun*        run;          /* +0x24 */
    int           f28;          /* +0x28 */
    LFPiece*      sub;          /* +0x2c  head of this piece's sub-list */
    LFPiece*      end_a;        /* +0x30  the sub-piece at end A */
    LFPiece*      end_b;        /* +0x34  the sub-piece at end B */
};

/* One boat: 0x24 bytes, four of them from LFRun+0x40 on (logflume4.c). */
typedef struct LFBoat {
    int           state;        /* +0x00 */
    unsigned int  flags;        /* +0x04  bit 0 = waiting at the station */
    RiderNode*    rider;        /* +0x08  who is aboard */
    unsigned char pad0c[8];
    LFPiece*      piece;        /* +0x14  the route piece it is over */
    float         z;            /* +0x18  how far along that piece, 0..1 */
    unsigned char pad1c[0x24 - 0x1c];
} LFBoat;

/* THE STATION QUEUE (LFRun+0x2c).  lfentrance.c calls it "an 8-byte header
 * over a list of 8-byte nodes {next, rider}" and logflume.c calls the same
 * twelve bytes `LFAnimRefs { int r[3] }`; 0x00412490 shows they are ONE
 * object with THREE fields -- the walk PATH, the list head and the list
 * TAIL, which is what makes the loader's append O(1). */
typedef struct LFQueuePath {
    int   count;                /* +0x00  how many people fit on the path */
    char* pts;                  /* +0x04  count * 12 bytes, right behind it */
} LFQueuePath;

typedef struct LFQueueNode {
    struct LFQueueNode* next;   /* +0x00 */
    int                 rider;  /* +0x04 */
} LFQueueNode;

typedef struct LFQueue {
    LFQueuePath* path;          /* +0x00 */
    LFQueueNode* head;          /* +0x04 */
    LFQueueNode* tail;          /* +0x08 */
} LFQueue;

struct LFRun {
    LFRun*        next;         /* +0x00 */
    unsigned int  flags;        /* +0x04 */
    LFPiece*      f08;          /* +0x08 */
    LFPiece*      f0c;          /* +0x0c */
    LFPiece*      pieces;       /* +0x10  head of this run's piece TREE */
    BPosW         sq;           /* +0x14  the station's map square */
    unsigned char pad16[2];
    LFPiece*      f18;          /* +0x18 */
    int           frame;        /* +0x1c */
    int           step;         /* +0x20 */
    int           hold;         /* +0x24 */
    int           hold_time;    /* +0x28 */
    LFQueue       queue;        /* +0x2c  path, head, TAIL */
    LFBoat*       boat;         /* +0x38  the boat at the station */
    int           boat_count;   /* +0x3c  how many boats this run runs */
    LFBoat        boats[4];     /* +0x40 .. +0xcf */
    int           piece_count;  /* +0xd0 */
};                              /* 0xd4 */

/* ---- callees ----------------------------------------------------------- */
/* 0x0040b210 (not exported): does boat `b` draw on piece `p` this frame? */
extern int  LFBoat_IsOnPiece(LFBoat* b, LFPiece* p);            /* 0x0040b210 */
/* 0x0040ae90 (not exported): queue boat `b`'s artwork over piece `p`. */
extern void LFBoat_Draw(LFBoat* b, LFPiece* p, int mode);       /* 0x0040ae90 */

/* ==========================================================================
 * 0x0040b390 -- draw every boat that lands on this piece, and say how many.
 *
 * lfentrance.c and logflume2.c both call it `LFPiece_HasRider` because they
 * only test the result against zero; what it actually does is walk the run's
 * boats (`run->boats[0 .. boat_count-1]`, the 0x24-byte records from +0x40)
 * and, for each one 0x0040b210 says lands here, queue its artwork.
 *
 * A piece with SUB-PIECES (+0x2c) is tested one sub-piece at a time -- the
 * boat is drawn over the sub-piece it is actually on -- and a piece with none
 * is tested itself. That `if (sub) { walk } else { one }` shape is the whole
 * block layout: the walk is the fall-through and the single-piece case sits
 * after it, sharing the loop's tail.
 *
 * `run->boat_count` is RE-READ at every latch (the draw calls could change
 * it), the boat pointer is VC6's own strength reduction of `&run->boats[i]`
 * (`run + 0x40`, then `+= 0x24`), and the loop counter lives in the dead `p`
 * argument slot -- `p` is copied into ebp at entry and the slot is free from
 * then on, even though `p` itself is used throughout.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x0040b390
int LFPiece_HasRider(LFPiece* p)
{
    LFRun*   run = p->run;
    LFPiece* s;
    int      i;
    int      n = 0;

    for (i = 0; i < run->boat_count; i++) {
        s = p->sub;
        if (s) {
            while (s) {
                if (LFBoat_IsOnPiece(&run->boats[i], s)) {
                    LFBoat_Draw(&run->boats[i], s, 1);
                    n++;
                }
                s = s->next;
            }
        } else {
            if (LFBoat_IsOnPiece(&run->boats[i], p)) {
                LFBoat_Draw(&run->boats[i], p, 1);
                n++;
            }
        }
    }
    return n;
}

/* ---- the piece-tree save stream ---------------------------------------- */
extern int      SaveGameWrite(const void* buf, unsigned int n);  /* 0x0047d760 */
extern int      SaveGameRead(void* buf, unsigned int n);         /* 0x0047d730 */
extern void*    HeapAlloc_w(unsigned int n);                     /* 0x0049e4ff */
/* 0x004107b0: the piece's ORDINAL within `head`'s list, as a fake pointer. */
extern LFPiece* LFPiece_ToIndex(LFPiece* head, LFPiece* p);      /* 0x004107b0 */
extern int      LLIDB_FindElement(const char* name, RideElem** out,
                                  unsigned int* idx);            /* 0x0047b330 */

/* ==========================================================================
 * 0x00410800 -- WRITE one level of the piece tree.
 *
 * logflume.c calls it `LFRun_PrepareSave` and describes it as renumbering the
 * run's list; it is really the piece writer, and it recurses. Per piece:
 *
 *     dword 1
 *     0x38 bytes -- the piece itself, with fwd/back/end_a/end_b replaced by
 *                   their indices within st->pieces (the run record's own
 *                   +0x10 anchor, which is written out unchanged)
 *     dword     -- strlen of the piece class's name
 *     n bytes   -- the name, WITHOUT its terminator
 *     <the piece's sub-list, written by the same function>
 *
 * and a dword 0 closes the level. `st->pieces` is re-read before all four
 * index conversions because the calls could move it.
 *
 * FRAME: the 0x38-byte copy takes the top of the 0x40-byte frame with the
 * two constant tags below it, and the name LENGTH lives in the dead `p`
 * argument slot -- `p` is copied into ebx at entry, so the slot is free from
 * the first instruction even though `p` itself drives the loop. The pushes
 * of ebp/esi/edi sink into the non-empty-list path.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x00410800
void LFRun_PrepareSave(LFRun* st, LFPiece* p)
{
    LFPiece buf;
    int     one = 1;
    int     zero = 0;
    int     len;

    while (p) {
        SaveGameWrite(&one, 4);
        buf = *p;
        buf.fwd   = LFPiece_ToIndex(st->pieces, buf.fwd);
        buf.back  = LFPiece_ToIndex(st->pieces, buf.back);
        buf.end_a = LFPiece_ToIndex(st->pieces, buf.end_a);
        buf.end_b = LFPiece_ToIndex(st->pieces, buf.end_b);
        SaveGameWrite(&buf, 0x38);
        len = strlen(buf.def->elem->name);
        SaveGameWrite(&len, 4);
        SaveGameWrite(buf.def->elem->name, len);
        LFRun_PrepareSave(st, p->sub);
        p = p->next;
    }
    SaveGameWrite(&zero, 4);
}

/* ==========================================================================
 * 0x00410a50 -- READ one level of the piece tree back, and return its head.
 *
 * The exact mirror of 0x00410800. Each record allocates a fresh 0x38-byte
 * piece, chains it onto the previous one through +0x00, reads the saved
 * bytes straight over it and then re-stamps the three fields that cannot
 * come out of the stream: the back link (+0x04), the owning run (+0x24) and
 * the PARENT PIECE (+0x28) -- which is what the second parameter really is.
 * logflume.c declares it `int mode` and passes 0 for the top level, which is
 * right by value and wrong by type; the recursive call here passes the piece.
 *
 * The class is recovered by NAME: the saved bytes are terminated in place
 * (`name[len] = 0`) and, if the name is not empty, looked up in the LLIDB;
 * the element's +0x0c is the piece's RideDef. A piece saved with an empty
 * name keeps whatever `def` the stream contained.
 *
 * FRAME: 0x210 -- the 0x200-byte name buffer at the top, then the element
 * out-pointer, the length, the list head and the tag. Both exits return the
 * head; VC6 constant-propagates it to zero on the empty-stream exit, which is
 * why that one is `mov eax,ebp` against the other's frame load.
 *
 * LEVER: the four initialisers must be written `tag, p, head, prev`. All 24
 * orders were measured; that one alone puts `tag = 1`'s store before the two
 * register zeroes and `head = 0`'s store between them (three mismatches for
 * the natural head/p/prev/tag order, two for most of the rest). Same family
 * as popup.c's "initialiser ORDER decides where a spill store lands".
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x00410a50
LFPiece* LFRun_LoadPieces(LFRun* st, LFPiece* parent)
{
    char      name[0x200];
    RideElem* elem;
    int       len;
    int       tag = 1;
    LFPiece*  p = 0;
    LFPiece*  head = 0;
    LFPiece*  prev = 0;

    SaveGameRead(&tag, 4);
    while (tag) {
        if (p == 0) {
            p = (LFPiece*)HeapAlloc_w(0x38);
            head = p;
        } else {
            p->next = (LFPiece*)HeapAlloc_w(0x38);
            p = p->next;
        }
        SaveGameRead(p, 0x38);
        p->prev = prev;
        p->run = st;
        p->f28 = (int)parent;
        SaveGameRead(&len, 4);
        SaveGameRead(name, len);
        name[len] = 0;
        if (name[0]) {
            LLIDB_FindElement(name, &elem, 0);
            p->def = (RideDef*)elem->data;
        }
        p->sub = LFRun_LoadPieces(st, p);
        SaveGameRead(&tag, 4);
        prev = p;
    }
    return head;
}

/* ---- the piece draw helpers -------------------------------------------- */
/* Three parallel tables indexed by a BYTE OFFSET, not an element index --
 * the original computes `shape * 4` once and reuses it for all three, which
 * is why the bases are typed raw (logflume.c records the same idiom). */
typedef struct LFImageList {
    unsigned char pad00[8];
    char*         sprites;      /* +0x08  SpriteRec*, one per shape index */
    char*         ox;           /* +0x0c  render x, in half pixels */
    char*         oy;           /* +0x10  render y, in half pixels */
} LFImageList;

/* logflume.c declares 0x004cbe50 as a plain `void* g_lf_endylist_data`; this
 * file needs it typed, and the type is the caller-side lever that folds the
 * three table reads onto one scaled index. */
extern LFImageList* g_lf_endylist_data;                         /* 0x004cbe50 */

extern Pos  GetScreenCoordsForObject(BPosW* sq, RideDef* def);  /* 0x00442cc0 */
extern void AdjustOffsetForViewMode(Pos* o);                    /* 0x00442d30 */
extern int  PrintSprite(SpriteRec* s, int x, int y, int mode, void* ctx);
                                                                /* 0x004853a0 */

/* ==========================================================================
 * 0x0040cca0 -- draw the END-Y join sprite over a piece.
 *
 * The class's two draw parameters (+0x14/+0x18) are cleared first -- so the
 * join is drawn unshifted whatever the last caller left there -- and `p->def`
 * is re-read for each of the three uses, because a store through it may alias
 * the field itself.
 *
 * The shape comes from the piece's orientation through a switch whose four
 * cases each yield their own index, i.e. an IDENTITY map written out long
 * hand. That is not cosmetic: it is what puts the four `mov eax,K` blocks in
 * the layout and, more to the point, leaves the DEFAULT with no value at all.
 *
 * ORIGINAL BUG, reproduced: a `p->dir` outside 0..3 falls through the switch
 * with `shape` UNINITIALISED, and the function then indexes the sprite table
 * with whatever the dead `p` argument slot happens to hold (masked to a byte,
 * so it is a wild but in-range-looking table read). VC6 homes the variable in
 * that slot, which is why the default arm is a bare `mov eax,[esp+0x18]`.
 * `p->dir` is only ever 0..3 in practice, so the arm is unreachable.
 *
 * The `and eax,0xff` on the way into the tables says the value is masked to a
 * byte at the USE while the switch arms assign full dwords, so the variable
 * is an `int` subscripted through an `(unsigned char)` cast; a plain
 * `unsigned char` local instead makes the arms byte moves and costs 45.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x0040cca0
void LFPiece_DrawJoin(LFPiece* p)
{
    Pos          off;
    Pos          s;
    LFImageList* il;
    SpriteRec*   spr;
    int          shape;
    int          idx;

    p->def->f18 = 0;
    p->def->f14 = 0;
    s = GetScreenCoordsForObject(&p->sq, p->def);
    switch (p->dir) {
    case 0: shape = 0; break;
    case 1: shape = 1; break;
    case 2: shape = 2; break;
    case 3: shape = 3; break;
    }
    il = g_lf_endylist_data;
    idx = (unsigned char)shape * 4;
    spr = *(SpriteRec**)(il->sprites + idx);
    off.x = *(int*)(il->ox + idx) >> 1;
    off.y = *(int*)(il->oy + idx) >> 1;
    AdjustOffsetForViewMode(&off);
    if (spr)
        PrintSprite(spr, s.x + off.x, s.y + off.y, 0, 0);
}

/* ==========================================================================
 * 0x00412490 -- read the station QUEUE back from the save stream.
 *
 * logflume.c calls it `LFAnim_LoadRefs(void* set, int* refs)` and treats the
 * twelve bytes at LFRun+0x2c as three opaque "animation references"; this is
 * what they really are:
 *
 *   dword n                       how many points the queue's walk path has
 *   n * 12 bytes                  the path points themselves
 *   dword m                       how many riders are queueing
 *   m * dword                     each rider's saved id
 *
 * The path is allocated as ONE `n*12 + 8` block whose first two words are the
 * count and a pointer to the bytes right behind them, so the header and the
 * points are one allocation. The riders are appended to a singly-linked list
 * of 8-byte {next, rider} nodes, and it is the TAIL pointer at +0x08 that
 * makes the append O(1) -- the same head/tail pair lfentrance.c's queue
 * primitives use. Each saved id goes through 0x00412470 to become the live
 * rider pointer.
 *
 * LEVER (worth 90 of 93): the count, the loop counter and the second count
 * are ONE `int n`, and because `&n` is handed to SaveGameRead the variable is
 * address-taken and cannot be enregistered. With separate locals VC6 gives
 * the loop counter EDI and manufactures an EBX induction variable for the
 * `i * 12` byte offset (91 instructions, a full prologue push of edi); with
 * the single reused variable the counter is reloaded from its home and
 * `i * 3 / lea [base + i*4]` is recomputed every iteration -- the original --
 * and EDI is then only needed by the second loop, so its push SPLITS into
 * that loop's guarded path. An address-taken counter is the way to turn
 * VC6's strength reduction off.
 * ======================================================================== */
/* 0x00412470: turn a saved rider id back into the live rider. */
extern int LFAnim_FromId(void* set, int id);                    /* 0x00412470 */

// FUNCTION: LEGOLAND 0x00412490
void LFAnim_LoadRefs(void* set, LFQueue* q)
{
    int n, id;

    SaveGameRead(&n, 4);
    q->path = (LFQueuePath*)HeapAlloc_w(n * 12 + 8);
    q->path->count = n;
    q->path->pts = (char*)q->path + 8;
    for (n = 0; n < q->path->count; n++)
        SaveGameRead(q->path->pts + n * 12, 12);
    q->head = 0;
    q->tail = 0;
    SaveGameRead(&n, 4);
    while (n--) {
        if (q->tail == 0) {
            q->tail = (LFQueueNode*)HeapAlloc_w(8);
            q->head = q->tail;
        } else {
            q->tail->next = (LFQueueNode*)HeapAlloc_w(8);
            q->tail = q->tail->next;
        }
        q->tail->next = 0;
        SaveGameRead(&id, 4);
        q->tail->rider = LFAnim_FromId(set, id);
    }
}

/* ==========================================================================
 * 0x0040bbb0 -- advance ONE boat of the run by one route square.
 *
 * LFRun_Tick calls it for every boat on the tick its `step` counter goes
 * negative, i.e. every third frame. The boat's two flag bits pick the mode:
 *
 *   bit 1 set  -- the boat is FALLING down a drop; 0x00411810 runs the fall
 *                 and nothing else happens this frame.
 *   bit 0 set  -- the boat is already moving under the "at a junction" rule:
 *                 it needs a piece ahead (piece->fwd), 0x0040bab0 has to
 *                 agree the route is clear, and 0x00411680 has to accept the
 *                 move; only then does the boat actually step onto
 *                 piece->fwd. 0x00411650 then says whether the new square is
 *                 a DROP -- if so bit 1 goes up and the fall takes over next
 *                 frame.
 *   neither    -- the boat is coasting: `state` counts DOWN and nothing
 *                 happens until it goes negative. It may then start a move
 *                 (same three tests, with bit 0 going up first), and either
 *                 way `state` is reset to 1.
 *
 * TWO SPECIAL SQUARES end a lap, and both tests are written out TWICE -- once
 * per mode -- which is why the same six-instruction block appears at
 * 0x0040bc4a and 0x0040bcea with different register allocation (the first
 * copy is under enough pressure that VC6 saves and restores EBX for a single
 * compare; the second folds the compare into a memory operand):
 *
 *   piece == run->f0c->fwd   the UNLOADING square: the rider's bloke gets its
 *                            release byte (+0x60) bumped, the boat is emptied
 *                            and parked with state 0x32 and bit 0 cleared;
 *   piece == run->f08->fwd   the STATION square: parked the same way, rider
 *                            or no rider.
 *
 * The `if (b)` at the top can never be false -- `b` is `run + idx*36 + 0x40`
 * -- but VC6 cannot know that, so the test and its shared epilogue are real.
 * ======================================================================== */
extern void LFBoat_Fall(LFBoat* b);                              /* 0x00411810 */
extern int  LFRun_BoatHasRoom(LFRun* run, int idx);                    /* 0x0040bab0 */
extern int  LFBoat_Advance(LFBoat* b);                              /* 0x00411680 */
extern int  LFBoat_IsOnDrop(LFBoat* b);                              /* 0x00411650 */

/* The driver record; only the release byte this function bumps is named. */
typedef struct Bloke { unsigned char pad00[0x60]; unsigned char free_flag; } Bloke;

struct RiderNode {
    RiderNode*     next;        /* +0x00 */
    RiderNode*     prev;        /* +0x04 */
    Bloke*         bloke;       /* +0x08 */
    unsigned short ride_id;     /* +0x0c */
    unsigned short pad0e;
    void*          owner;       /* +0x10 */
};

// FUNCTION: LEGOLAND 0x0040bbb0
void LFBoat_Step(LFRun* run, int idx)
{
    LFBoat* b = &run->boats[idx];

    if (b) {
        if (b->flags & 2) {
            LFBoat_Fall(b);
            return;
        }
        if (b->flags & 1) {
            if (b->piece->fwd == 0)
                return;
            if (!LFRun_BoatHasRoom(run, idx))
                return;
            if (!LFBoat_Advance(b))
                return;
            b->piece = b->piece->fwd;
            if (LFBoat_IsOnDrop(b)) {
                b->flags |= 2;
                return;
            }
            if (b->piece == run->f0c->fwd && b->rider) {
                b->rider->bloke->free_flag++;
                b->rider = 0;
                b->state = 0x32;
                b->flags &= ~1;
            }
            if (b->piece == run->f08->fwd) {
                b->state = 0x32;
                b->flags &= ~1;
            }
            return;
        }
        b->state--;
        if (b->state < 0) {
            if (!(run->flags & 1) && b->piece->fwd != 0 && LFRun_BoatHasRoom(run, idx)) {
                b->flags |= 1;
                if (LFBoat_Advance(b)) {
                    b->piece = b->piece->fwd;
                    if (b->piece == run->f0c->fwd && b->rider) {
                        b->rider->bloke->free_flag++;
                        b->rider = 0;
                        b->state = 0x32;
                        b->flags &= ~1;
                    }
                    if (b->piece == run->f08->fwd) {
                        b->state = 0x32;
                        b->flags &= ~1;
                    }
                }
            }
            b->state = 1;
        }
    }
}
