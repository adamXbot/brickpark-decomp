/* LEGOLAND -- the LOG FLUME's run tick, its boat-path geometry tables and the
 * TRACK class's second (depth-sorted) draw pass.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  None of
 * these functions is exported; every extent comes from the disassembly by
 * control flow (tools/audit.py).  Struct field OFFSETS, record sizes and
 * global addresses are load-bearing; the names are ours.  Types are declared
 * LOCALLY on purpose (legoland.h is owned elsewhere).
 *
 *   0x0040be00  LFRun_Tick            123/123 insns, 325/325 bytes, exact
 *   0x004113d0  LFTrack_BuildGeometry 161/161 insns, 625/625 bytes, exact
 *   0x0040ca60  LFTrack_DrawAlt       144/144 insns, 401/401 bytes, exact
 *                                     (a register allocation floor; see the
 *                                      note above its marker)
 *
 * The ten log-flume classes, the piece graph, the run/boat records and the
 * shape encoding are documented in LEGOLAND/logflume.c, logflume2.c,
 * logflume3.c and lfentrance.c.  Three codegen facts this file adds, each
 * measured against a sweep of the alternatives:
 *
 *  - `LFQueue_StepFront`'s target is declared here as a `Pos` BY VALUE even
 *    though the callee reads two separate int arguments.  (int, int) is
 *    ABI-identical but evaluates the two sums right to left; the by-value Pos
 *    evaluates them in field order, which is the original's schedule.  It
 *    only works together with reading the class global directly at both uses:
 *    a `RideDef* def` local costs 76 of 123, the two-int form 13.
 *  - `tw <<= 1` and `tw = tw * 2` are NOT the same object: the in-place shift
 *    is what the original has, the multiply lowers to a two-operand `lea`
 *    into a fresh register (6 of 161).  And the two quarter-steps derived
 *    from the doubled tile are emitted in REVERSE source order, so writing
 *    `dy` before `dx` is worth another 26.
 *  - Naming ONE of two byte-wide operands of a compare duplicated across both
 *    arms of an `if` (`unsigned char px = p->sq.b.x;`) is what makes VC6
 *    hoist the whole three-load block out of the arms; with both operands
 *    spelled as fields the loads are duplicated into each arm and the second
 *    `test` disappears (71 of 144).  Naming BOTH forces frame homes for two
 *    byte locals and grows the frame from `push ecx` to `sub esp,8` (94).
 * ========================================================================= */

/* ---- shared types (same offsets as logflume.c / lfentrance.c) ----------- */
typedef struct Pos { int x; int y; } Pos;

typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

typedef struct SpriteRec SpriteRec;

typedef struct RideDef {
    unsigned char pad00[0x0c];
    int           base_x;       /* +0x0c */
    int           base_y;       /* +0x10 */
    unsigned char pad14[0xd0 - 0x14];
} RideDef;

typedef struct RiderNode RiderNode;
typedef struct LFRun     LFRun;

/* The placed-piece record (0x38 bytes; the full struct is in logflume.c). */
typedef struct LFPiece {
    struct LFPiece* next;       /* +0x00 */
    struct LFPiece* prev;       /* +0x04 */
    struct LFPiece* fwd;        /* +0x08 next piece ALONG THE ROUTE */
    struct LFPiece* back;       /* +0x0c */
    unsigned int    flags;      /* +0x10 */
    BPosW           sq;         /* +0x14 the piece's map square */
    unsigned char   pad16[2];
    int             kind;       /* +0x18 */
    int             dir;        /* +0x1c orientation / variant, 0..3 */
    void*           def;        /* +0x20 */
    LFRun*          run;        /* +0x24 */
    unsigned char   pad28[0x38 - 0x28];
} LFPiece;

/* The station's boarding queue (LFRun+0x2c): {path, head}.  The path's first
 * int is how many people fit on it. */
typedef struct LFQueue {
    void* path;                 /* +0x00 */
    void* head;                 /* +0x04 */
} LFQueue;

/* One boat: 0x24 bytes, four of them from LFRun+0x40 on. */
typedef struct LFBoat {
    int           state;        /* +0x00 */
    unsigned int  flags;        /* +0x04 bit 0 = waiting at the station */
    RiderNode*    rider;        /* +0x08 who is aboard */
    unsigned char pad0c[8];
    LFPiece*      piece;        /* +0x14 the route piece it is over */
    float         z;            /* +0x18 how far along that piece, 0..1 */
    unsigned char pad1c[0x24 - 0x1c];
} LFBoat;

struct LFRun {
    LFRun*        next;         /* +0x00 */
    unsigned int  flags;        /* +0x04 bit 0 = a boat is loading,
                                 *       bit 1 = the doors are held open */
    LFPiece*      f08;          /* +0x08 */
    LFPiece*      f0c;          /* +0x0c */
    LFPiece*      pieces;       /* +0x10 */
    BPosW         sq;           /* +0x14 the station's map square */
    unsigned char pad16[2];
    LFPiece*      f18;          /* +0x18 */
    int           frame;        /* +0x1c the run-wide animation frame, 0..15 */
    int           step;         /* +0x20 counts DOWN to the next boat step */
    int           hold;         /* +0x24 how long the doors have been held */
    int           hold_time;    /* +0x28 how long they are held for */
    LFQueue       queue;        /* +0x2c */
    unsigned char pad34[4];
    LFBoat*       boat;         /* +0x38 the boat at the station */
    int           boat_count;   /* +0x3c how many boats this run runs */
    LFBoat        boats[4];     /* +0x40 .. +0xcf */
    int           piece_count;  /* +0xd0 */
};                              /* 0xd4 */

/* ---- callees ------------------------------------------------------------ */
extern int  LFRun_IsComplete(LFRun* run);                        /* 0x0040ba80 */
extern void LFRun_Start(LFRun* run);                             /* 0x0040a580 */
/* 0x0040bb50 (not exported): scan the run's boats for the one parked at the
 * station -- the boat whose `piece` (+0x14) matches the piece hanging off
 * run->f08 at +0x08, which carries nobody (rider == 0) and whose flag bit 0
 * is set.  Returns 1 and writes it to *out. */
extern int  LFRun_FindWaitingBoat(LFRun* run, LFBoat** out);     /* 0x0040bb50 */
/* 0x0040bbb0 (not exported): advance boat `idx` of the run one step along the
 * route (run + idx*0x24 + 0x40). */
extern void LFBoat_Step(LFRun* run, int idx);                    /* 0x0040bbb0 */
/* 0x00411e90 (not exported): is anybody queueing?  (q->head != 0) */
extern int  LFQueue_HasRider(LFQueue* q);                        /* 0x00411e90 */
/* 0x00411ea0 (not exported): has the front rider finished walking the queue
 * path?  (its bloke's +0x38 frame index == *(int*)q->path - 1) */
extern int  LFQueue_FrontIsReady(LFQueue* q);                    /* 0x00411ea0 */
/* 0x00412060 (not exported): take the front rider off the queue -- bump its
 * action byte, clear the "queueing" flag (+0x62 bit 0x40), hand it back
 * through *out and free the node. */
extern void LFQueue_PopFront(LFQueue* q, RiderNode** out);       /* 0x00412060 */
/* 0x004120a0 (not exported): walk the front rider one step toward `t`.  The
 * callee reads two separate int arguments, so (int, int) is ABI-identical --
 * but only the by-VALUE Pos spelling schedules the two sums the way the
 * original does (x first, into the base_x register), and only with the class
 * global read directly at both uses: a `RideDef* def` local costs 76 of 123. */
#ifndef LEGOLAND_PORTABLE
extern void LFQueue_StepFront(LFQueue* q, Pos t);                /* 0x004120a0 */
#else
/* The by-value spelling is the evaluation-order lever noted at the top of
 * this file; on x86 both forms push the same two dwords. On wasm32 a by-value
 * struct is passed as a POINTER to a copy, which makes it a different function
 * type from lfmisc2.c's `(LFQueue*, int, int)` definition, so the one call
 * below resolves to a trapping stub. Unpack the point at the call. */
extern void LFQueue_StepFront(LFQueue* q, int tx, int ty);       /* 0x004120a0 */
#define LFQueue_StepFront(_q, _t) LFQueue_StepFront((_q), (_t).x, (_t).y)
#endif
extern int  LFQueue_IsFull(LFQueue* q);                          /* 0x00411e60 */
extern void Ride_SetFlagToNotLetAnyoneOn(BPosW* sq);             /* 0x00442fa0 */
extern void Ride_ClearFlagToNotLetAnyoneOn(BPosW* sq);           /* 0x00443000 */

extern RideDef* g_lfen_def;             /* 0x004c2b9c  LOG FLUME ENTRANCE */

/* =========================================================================
 * ONE TICK OF ONE RUN
 *
 * Called from LFRun_TickAll (lfentrance.c 0x0040bf50) for every run in the
 * park, once per frame from the ENTRANCE's +0xa8 activate slot.  In order:
 *
 *  1. If the doors are held open (flags bit 1), count `hold` up to
 *     `hold_time` and drop the bit when it gets there.
 *  2. If the run is not yet a closed circuit (LFRun_IsComplete), call
 *     LFRun_Start to (re)build it and do nothing else this frame.
 *  3. Otherwise count `step` DOWN; the boats only move on the tick it goes
 *     negative, and it is then reset to 2 -- so the flume advances one route
 *     square every third frame.  On that tick every boat is stepped.
 *  4. Then, unless a boat is already loading (flags bit 0), look for the
 *     boat waiting at the station; if there is one it becomes run->boat, and
 *     if somebody is queueing AND has reached the head of the queue path,
 *     the "loading" bit goes up, the rider is popped off the queue and put
 *     aboard that boat.
 *  5. Every frame, whatever happened above: step the front of the queue one
 *     pace toward the station's door (the ENTRANCE class's +0x0c/+0x10 base
 *     offset plus the station's own map square), set or clear the ride's
 *     "no more riders" flag from whether the queue is full, and advance the
 *     run-wide animation frame, which wraps at 16.
 *
 * `rider` and `boat` are both live only inside step 4 and VC6 homes `boat`
 * in the dead `run` argument slot.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0040be00
void LFRun_Tick(LFRun* run)
{
    RiderNode* rider;
    LFBoat*    boat;
    int        i;
    Pos        t;

    if (run->flags & 2) {
        run->hold++;
        if (run->hold >= run->hold_time)
            run->flags &= ~2;
    }
    if (LFRun_IsComplete(run)) {
        run->step--;
        if (run->step < 0) {
            run->step = 2;
            for (i = 0; i < run->boat_count; i++)
                LFBoat_Step(run, i);
            if (!(run->flags & 1)) {
                boat = 0;
                if (LFRun_FindWaitingBoat(run, &boat)) {
                    rider = 0;
                    run->boat = boat;
                    if (LFQueue_HasRider(&run->queue)
                        && LFQueue_FrontIsReady(&run->queue)) {
                        run->flags |= 1;
                        LFQueue_PopFront(&run->queue, &rider);
                        boat->rider = rider;
                    }
                }
            }
        }
    } else {
        LFRun_Start(run);
    }
    t.x = g_lfen_def->base_x + run->sq.b.x;
    t.y = g_lfen_def->base_y + run->sq.b.y;
    LFQueue_StepFront(&run->queue, t);
    if (LFQueue_IsFull(&run->queue))
        Ride_SetFlagToNotLetAnyoneOn(&run->sq);
    else
        Ride_ClearFlagToNotLetAnyoneOn(&run->sq);
    run->frame = (run->frame + 1) & 0xf;
}

/* =========================================================================
 * THE BOAT PATHS THROUGH ONE TILE
 *
 * A flume tile is drawn at DOUBLE tile scale, so the four points a boat can
 * enter or leave a tile at are the centres of the four quadrants of the
 * doubled tile.  Four tiny helpers hand those back as a Pos in eax:edx
 * (each re-derives them from GetTileDimensions):
 *
 *   0x00411290  ( tw,  th)   top-left       0x00411220  (3tw,  th)  top-right
 *   0x004112c0  ( tw, 3th)   bottom-left    0x00411250  (3tw, 3th)  bottom-right
 *
 * `LFTrack_BuildGeometry` fills the six control polygons the boat mover
 * interpolates along, each a {count, points} pair in .data:
 *
 *   struct  points  n  path
 *   4c2b58  4cbe38  2  top-right -> bottom-left     (straight through)
 *   4c2b00  4c8d58  2  bottom-right -> top-left     (straight through)
 *   4c2be8  4cbde8  4  top-right -> bottom-right    (a quarter turn)
 *   4c2bc0  4c2b30  4  bottom-right -> bottom-left
 *   4c2c10  4c2bc8  4  bottom-left -> top-left
 *   4c2c08  4c2b78  4  top-left -> top-right
 *
 * The straight paths are just their two endpoints.  A turn is a four-point
 * cubic control polygon: p[0] and p[3] are the entry and exit quadrant
 * centres and p[1] / p[2] are those points pulled a QUARTER of the doubled
 * tile (dx = 2*tw >> 2, dy = 2*th >> 2) toward the middle of the tile, with
 * the sign pattern rotating one quadrant per turn:
 *
 *   tr->br   p1 = p0 + (-dx, +dy)   p2 = p3 + (-dx, -dy)
 *   br->bl   p1 = p0 + (-dx, -dy)   p2 = p3 + (+dx, -dy)
 *   bl->tl   p1 = p0 + (+dx, -dy)   p2 = p3 + (+dx, +dy)
 *   tl->tr   p1 = p0 + (+dx, +dy)   p2 = p3 + (-dx, +dy)
 *
 * so the four turns are the same curve rotated through the ring
 * tr -> br -> bl -> tl -> tr.  Called once, from LFTrack_Create.
 *
 * The doubled tile size is written BACK into the two GetTileDimensions
 * out-parameters and then never read again -- dead stores VC6 keeps because
 * the locals' addresses escaped.  Reproduced.
 * ========================================================================= */

typedef struct LFPath {
    int  n;                     /* +0x00 how many control points */
    Pos* pts;                   /* +0x04 */
} LFPath;

extern LFPath g_lf_path_tr_bl;          /* 0x004c2b58 */
extern LFPath g_lf_path_br_tl;          /* 0x004c2b00 */
extern LFPath g_lf_path_tr_br;          /* 0x004c2be8 */
extern LFPath g_lf_path_br_bl;          /* 0x004c2bc0 */
extern LFPath g_lf_path_bl_tl;          /* 0x004c2c10 */
extern LFPath g_lf_path_tl_tr;          /* 0x004c2c08 */

extern Pos g_lf_pts_tr_bl[2];           /* 0x004cbe38 */
extern Pos g_lf_pts_br_tl[2];           /* 0x004c8d58 */
extern Pos g_lf_pts_tr_br[4];           /* 0x004cbde8 */
extern Pos g_lf_pts_br_bl[4];           /* 0x004c2b30 */
extern Pos g_lf_pts_bl_tl[4];           /* 0x004c2bc8 */
extern Pos g_lf_pts_tl_tr[4];           /* 0x004c2b78 */

extern void GetTileDimensions(int* w, int* h);                   /* 0x00460540 */
extern Pos  LFQuadTopLeft(void);                                 /* 0x00411290 */
extern Pos  LFQuadTopRight(void);                                /* 0x00411220 */
extern Pos  LFQuadBottomLeft(void);                              /* 0x004112c0 */
extern Pos  LFQuadBottomRight(void);                             /* 0x00411250 */

// FUNCTION: LEGOLAND 0x004113d0
void LFTrack_BuildGeometry(void)
{
    int tw, th, dx, dy;

    /* `<<= 1` (an in-place shift), not `= tw * 2` (which VC6 lowers to a
     * two-operand `lea` into a fresh register and costs 6); and dy BEFORE dx,
     * which is worth 26 more -- the two quarter-steps are emitted in reverse
     * source order and that decides which of esi/edi each lands in. */
    GetTileDimensions(&tw, &th);
    tw <<= 1;
    th <<= 1;
    dy = th >> 2;
    dx = tw >> 2;

    g_lf_path_tr_bl.n = 2;
    g_lf_path_tr_bl.pts = g_lf_pts_tr_bl;
    g_lf_path_tr_bl.pts[0] = LFQuadTopRight();
    g_lf_path_tr_bl.pts[1] = LFQuadBottomLeft();

    g_lf_path_br_tl.n = 2;
    g_lf_path_br_tl.pts = g_lf_pts_br_tl;
    g_lf_path_br_tl.pts[0] = LFQuadBottomRight();
    g_lf_path_br_tl.pts[1] = LFQuadTopLeft();

    g_lf_path_tr_br.n = 4;
    g_lf_path_tr_br.pts = g_lf_pts_tr_br;
    g_lf_path_tr_br.pts[0] = LFQuadTopRight();
    g_lf_path_tr_br.pts[3] = LFQuadBottomRight();
    g_lf_path_tr_br.pts[1].x = g_lf_path_tr_br.pts[0].x - dx;
    g_lf_path_tr_br.pts[1].y = g_lf_path_tr_br.pts[0].y + dy;
    g_lf_path_tr_br.pts[2].x = g_lf_path_tr_br.pts[3].x - dx;
    g_lf_path_tr_br.pts[2].y = g_lf_path_tr_br.pts[3].y - dy;

    g_lf_path_br_bl.n = 4;
    g_lf_path_br_bl.pts = g_lf_pts_br_bl;
    g_lf_path_br_bl.pts[0] = LFQuadBottomRight();
    g_lf_path_br_bl.pts[3] = LFQuadBottomLeft();
    g_lf_path_br_bl.pts[1].x = g_lf_path_br_bl.pts[0].x - dx;
    g_lf_path_br_bl.pts[1].y = g_lf_path_br_bl.pts[0].y - dy;
    g_lf_path_br_bl.pts[2].x = g_lf_path_br_bl.pts[3].x + dx;
    g_lf_path_br_bl.pts[2].y = g_lf_path_br_bl.pts[3].y - dy;

    g_lf_path_bl_tl.n = 4;
    g_lf_path_bl_tl.pts = g_lf_pts_bl_tl;
    g_lf_path_bl_tl.pts[0] = LFQuadBottomLeft();
    g_lf_path_bl_tl.pts[3] = LFQuadTopLeft();
    g_lf_path_bl_tl.pts[1].x = g_lf_path_bl_tl.pts[0].x + dx;
    g_lf_path_bl_tl.pts[1].y = g_lf_path_bl_tl.pts[0].y - dy;
    g_lf_path_bl_tl.pts[2].x = g_lf_path_bl_tl.pts[3].x + dx;
    g_lf_path_bl_tl.pts[2].y = g_lf_path_bl_tl.pts[3].y + dy;

    g_lf_path_tl_tr.n = 4;
    g_lf_path_tl_tr.pts = g_lf_pts_tl_tr;
    g_lf_path_tl_tr.pts[0] = LFQuadTopLeft();
    g_lf_path_tl_tr.pts[3] = LFQuadTopRight();
    g_lf_path_tl_tr.pts[1].x = g_lf_path_tl_tr.pts[0].x + dx;
    g_lf_path_tl_tr.pts[1].y = g_lf_path_tl_tr.pts[0].y + dy;
    g_lf_path_tl_tr.pts[2].x = g_lf_path_tl_tr.pts[3].x - dx;
    g_lf_path_tl_tr.pts[2].y = g_lf_path_tl_tr.pts[3].y + dy;
}

/* =========================================================================
 * THE TRACK CLASS'S SECOND DRAW PASS
 *
 * A plain TRACK square is normally drawn by LFTrack_DrawNormal (logflume2.c
 * 0x0040cc00), which just blits the shape's sprite.  This is the version used
 * when boats have to be interleaved with the channel artwork: the boats on
 * this square are split into a "behind the rail" and an "in front of the
 * rail" list, and the rail sprite is drawn between the two.
 *
 *  1. Reset render list 2's bump allocator (RenderItems2_New) and empty both
 *     local lists.
 *  2. Walk the run's four boats.  LFBoat_IsOnPiece (0x0040b210) says whether
 *     this boat's artwork lands on this square at all -- it does when the
 *     boat is ON the piece, and also when it is on the piece's neighbour and
 *     far enough along (or not far enough) to overhang.  For a boat on a
 *     NEIGHBOURING piece the sort key is shifted a whole square: -1.0 if the
 *     neighbour is this piece's `back`, +1.0 if it is its `fwd` -- which puts
 *     the overhanging boat on the right side of this square's rail.  Boats
 *     with a key above 0.5 (the far half of the square) go on the FAR list,
 *     the rest on the NEAR list.  `n` counts how many landed anywhere; if
 *     none did there is nothing to draw and the function returns.
 *  3. The rail sprite is fc1_m3.lls for orientation 0 and fc3_m3.lls for any
 *     other, and it is drawn at the piece's own screen position.
 *  4. Which list is drawn first depends on the piece's orientation AND on
 *     whether the NEXT piece along the route is in the same map column:
 *
 *        dir == 0, next square in a different column : far, RAIL, near
 *        dir == 0, next square in the same column    : near, RAIL, far
 *        dir != 0, next square in a different column : near, RAIL, far
 *        dir != 0, next square in the same column    : far, RAIL, near
 *
 *     VC6 merges the two middle arms, which are identical.
 *
 * ORIGINAL INCONSISTENCY, reproduced: only the FIRST of those four arms
 * passes the caller's `mode` to PrintSprite; the other three hard-code 0.
 * Nothing distinguishes them otherwise, and `mode` is the parameter's only
 * use in the whole function -- it reads as three quarters of a copy-paste
 * left un-edited.
 * ========================================================================= */

typedef struct RenderItem RenderItem;
typedef struct RenderList { RenderItem* head; } RenderList;   /* one word */

/* The two depth buckets this pass sorts the square's boats into. */
extern RenderList g_lf_boats_far;       /* 0x004c8d74 */
extern RenderList g_lf_boats_near;      /* 0x004ca5ac */

extern SpriteRec* g_lfc1_spr_m3;        /* 0x004cbe1c  "fc1_m3.lls" */
extern SpriteRec* g_lfc3_spr_m3;        /* 0x004c8d68  "fc3_m3.lls" */

extern void RenderItems2_New(void);                              /* 0x00443060 */
extern void RenderItem2_AddItem(RenderList* list, void* payload,
                                int key);                        /* 0x004430f0 */
/* 0x0040b210 (not exported): does boat `b` draw on piece `p` this frame? */
extern int  LFBoat_IsOnPiece(LFBoat* b, LFPiece* p);             /* 0x0040b210 */
/* 0x0040ca30 (not exported): draw every boat on one render list over `p`. */
extern void LFDrawBoatList(RenderList* list, LFPiece* p);        /* 0x0040ca30 */
extern Pos  LFPiece_ScreenPos(LFPiece* p);                       /* 0x0040cfd0 */
extern int  LFPiece_ShapeIndex(LFPiece* p);                      /* 0x0040ad50 */
extern SpriteRec* g_lf_track_sprites[];  /* 0x004c2abc  ten loaded sprites */
extern int  PrintSprite(SpriteRec* s, int x, int y, int mode, void* ctx);
                                                                 /* 0x004853a0 */

/* RESIDUAL: 2 of 144 instructions, 401/401 BYTES, first diverging
 * index 84 -- the observed difference is the register selected for one
 * load and its use.  Only indices 84 and 86 diverge:
 *     original   mov ecx, [esp+1Ch] / push 0 / push ecx
 *     ours       mov eax, [esp+1Ch] / push 0 / push eax
 * i.e. WHICH scratch register carries the `mode` argument in the ONE arm that
 * passes it.  Same slot, same encoding length, same everything else.
 *
 * Why eax should be busy there, and why it is not: the byte temp
 * `fwd->sq.b.x` is defined at index 74 (`mov al,[edx+14h]`) and used by BOTH
 * arms' `cmp al,cl` (indices 76 and 88), so its interval spans index 84 in
 * the linear layout -- which is exactly what would make the original reach
 * for ecx.  Our body has that same instruction at the same index with the
 * same two uses, and VC6 still hands eax to the `mode` read.
 *
 * ~75 spellings measured at 2; these measurements do not establish a floor:
 *   - every way of sourcing the value: the parameter directly, through an
 *     int / void* / short / unsigned local copied at the top, an
 *     UNINITIALISED local homed in the dead argument slot, and
 *     `*(volatile int*)&mode` (which moves the load below the `push 0` and
 *     costs one more, so it is not even a rotation lever here);
 *   - extern prototype types: `mode` as int/unsigned/long/short/void*,
 *     PrintSprite's 4th parameter as int/unsigned/long, its 5th as int,
 *     PrintSprite returning void, and LFDrawBoatList / RenderItem2_AddItem /
 *     RenderItems2_New declared to return int so a call would pseudo-define
 *     eax -- all inert;
 *   - ten free-`volatile` placements at sites where the original already
 *     loads (p->run, p->fwd, p->dir at both tests, p->sq.b.x, b->z, b->piece,
 *     run->boat_count, both rail sprites).  Per the §6B one-experiment test,
 *     none of them moves this index, so the residual is a web RANK, not a
 *     local eax->ecx->edx rotation;
 *   - block shape: all four arm orders of the 2x2 nest, the equivalent
 *     `&&` if/else-if chain, an early `return` versus a wrapping `if (n)`,
 *     `if (spr)` vs `if (spr != 0)`, and a `static __inline` blit helper
 *     taking (Sprite*, Pos, int) or (Sprite*, int, int, int);
 *   - eight declaration permutations, `z` as float / double / long double,
 *     `px` as unsigned char / char / signed char / unsigned short / a
 *     one-member struct / a BPosW member, and the compare written both ways.
 * WHAT DID MOVE, and is now in the body: the `unsigned char px` local (below)
 * -- without it VC6 duplicates the three-load block into both arms and drops
 * the second `test eax,eax`, which is 146 instructions and 71 mismatches. */
/* Scope H, 2026-09-05: six newer pointer/temporary forms are also inert:
 * a pointer to mode inside the sprite guard or before LFDrawBoatList;
 * a pointer to fwd's BPos; named far/near list pointers in the mode arm;
 * a pointer to the final sprite-array slot; and a named boat-piece value.
 * Each emits the baseline's identical relocated function bytes. Strict 2,
 * register-blind 0, first index 84, 144i/401B; two exact neighbours preserved.
 * The baseline remains; no new reconstruction error was found.
 * Reproduction and measurements: docs/lanes/scope-h.md. */
/* RECOVERED LEVER (scope H, Fable, 2026-09-06; closes a 2-mismatch residual
 * that eight lenses and ~250 spellings had priced as a register floor).
 *
 * The residual was the register carrying the `mode` reload in the one arm
 * that passes it (original `mov ecx,[esp+1Ch] / push ecx`, ours eax).  The
 * earlier passes established that VC6 gives that reload the register of the
 * most recently freed temporary -- the right operand of the last `cmp` --
 * PROVIDED that operand is an anonymous temporary; a named `px` bound to ECX
 * across the split forces EAX.  They also established that without a named
 * byte the direction split's join block is empty, so VC6 folds it into the
 * sprite test (no second `test eax,eax`) and duplicates the three loads into
 * both arms (146 insns).  Both facts are right; what they missed is that the
 * emptiness of that join is decided by the SPRITE SELECT above it, not by the
 * byte compare.  `spr = fc1; if (p->dir) spr = fc3;` puts the only
 * conditional store in a side block and leaves the join with nothing but the
 * re-test of `p->dir`, which is then merged away.  Written as a two-way
 * choice -- `if (p->dir == 0) spr = fc1; else spr = fc3;` (or the `?:` with
 * the same polarity) -- the select is if-converted to the identical
 * `mov edi,[fc1] / test eax,eax / je / mov edi,[fc3]`, but the join block it
 * flows into survives: VC6 re-tests `eax`, and the instructions that head
 * BOTH arms (`mov edx,[esp+18h]`, `mov cl,[esi+14h]`, `mov al,[edx+14h]`,
 * allocated identically in each arm) are hoisted into the slot before the
 * `jne`.  Both bytes stay anonymous per-arm temporaries, `cmp al,cl` frees
 * CL last, and the `mode` reload takes ECX: 144/144, 401/401, 0 mismatches.
 *
 * Polarity is load-bearing: `p->dir ? fc3 : fc1` and `if (p->dir) fc3 else
 * fc1` keep everything else exact but lower as "load fc3, skip fc1 unless
 * dir == 0", flipping index 69 to `jne` (1 mismatch).  `spr = p->dir ?
 * fc3 : spr` after a default store folds back to the override shape (146).
 * A named `px` on top of the two-way select returns the EAX body (2).
 * Also measured this pass and inert or worse: a boolean flag for the byte
 * compare (VC6 materialises it with setcc, 158 insns), and copies of `spr`
 * or `pos` in the join as anchors (propagated away before the CFG cleanup,
 * 146).  Semantics are unchanged: the select still yields fc1 for dir 0 and
 * fc3 otherwise, and both compares read the same two bytes. */
// FUNCTION: LEGOLAND 0x0040ca60
void LFTrack_DrawAlt(LFPiece* p, int mode)
{
    LFRun*     run;
    LFPiece*   fwd;
    LFBoat*    b;
    SpriteRec* spr;
    Pos        pos;
    double     z;
    int        i, n, idx;

    n = 0;
    run = p->run;
    RenderItems2_New();
    g_lf_boats_far.head = 0;
    g_lf_boats_near.head = 0;
    for (i = 0; i < run->boat_count; i++) {
        b = &run->boats[i];
        if (LFBoat_IsOnPiece(b, p)) {
            z = b->z;
            if (b->piece != p) {
                if (p->back == b->piece)
                    z = b->z - 1.0;
                if (p->fwd == b->piece)
                    z = b->z + 1.0;
            }
            if (z <= 0.5)
                RenderItem2_AddItem(&g_lf_boats_far, b, 0);
            else
                RenderItem2_AddItem(&g_lf_boats_near, b, 0);
            n++;
        }
    }
    if (n == 0)
        return;
    fwd = p->fwd;
    pos = LFPiece_ScreenPos(p);
    if (p->dir == 0)
        spr = g_lfc1_spr_m3;
    else
        spr = g_lfc3_spr_m3;
    if (p->dir == 0) {
        if (fwd->sq.b.x != p->sq.b.x) {
            LFDrawBoatList(&g_lf_boats_far, p);
            if (spr)
                PrintSprite(spr, pos.x, pos.y, mode, 0);
            LFDrawBoatList(&g_lf_boats_near, p);
        } else {
            LFDrawBoatList(&g_lf_boats_near, p);
            if (spr)
                PrintSprite(spr, pos.x, pos.y, 0, 0);
            LFDrawBoatList(&g_lf_boats_far, p);
        }
    } else {
        if (fwd->sq.b.x != p->sq.b.x) {
            LFDrawBoatList(&g_lf_boats_near, p);
            if (spr)
                PrintSprite(spr, pos.x, pos.y, 0, 0);
            LFDrawBoatList(&g_lf_boats_far, p);
        } else {
            LFDrawBoatList(&g_lf_boats_far, p);
            if (spr)
                PrintSprite(spr, pos.x, pos.y, 0, 0);
            LFDrawBoatList(&g_lf_boats_near, p);
        }
    }
    idx = LFPiece_ShapeIndex(p);
    pos = LFPiece_ScreenPos(p);
    spr = g_lf_track_sprites[idx];
    if (spr)
        PrintSprite(spr, pos.x, pos.y, 0, 0);
}
