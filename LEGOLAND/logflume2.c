/* LEGOLAND -- the LOG FLUME ride subsystem, part 2:
 * the shared piece machinery, the station (LOG FLUME ENTRANCE) behaviour and
 * the track-piece PLACEMENT/SHAPE system.
 *
 * Companion to logflume.c, which holds the ten classes' callback shims.  See
 * that file's header for the class table and the LFRun / LFPiece records.
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd); none of
 * these functions is exported, so every extent came from control flow
 * (tools/audit.py).  Struct field OFFSETS and global addresses are
 * load-bearing; names are ours.  Types are declared LOCALLY on purpose.
 *
 * =========================================================================
 * THE FOUR-SLOT NEIGHBOUR ARRAY  (g_lf_nb, 0x004cbe20)
 *
 * Everything about laying flume goes through ONE fixed four-pointer array at
 * 0x004cbe20, indexed by compass direction:
 *
 *      slot 0 = NORTH   slot 1 = EAST   slot 2 = SOUTH   slot 3 = WEST
 *
 * LFTrack_ProbeNeighbours clears it, fills it from the four map squares
 * around a given square, and hands back its ADDRESS -- so every caller is
 * looking at the same array and a nested probe destroys the outer one.
 *
 * The array is summarised as a MASK with two bits per direction:
 *
 *      N = 0x01   E = 0x04   S = 0x10   W = 0x40
 *
 * so 0x11 is a north-south straight, 0x44 an east-west straight, 0x05 a
 * north-east corner, and so on.  LFTrack_MaskIsLegal enumerates the exactly
 * ten masks a flume square may have -- the four single ends (1, 4, 0x10,
 * 0x40), the two straights (0x11, 0x44) and the four corners (5, 0x14, 0x50,
 * 0x41) -- i.e. a square may have one or two connections and never three.
 *
 * A neighbour "has a free end" iff its kind field LFPiece +0x18 is 3 or 4;
 * LFTrack_DropFullNeighbours nulls out every slot that does not, and
 * LFTrack_MaskIsLegal ALSO re-tests the survivors and fails the whole square
 * if any is full.  (Both spellings are kept: the original really does the
 * test twice, once destructively and once not.)
 *
 * =========================================================================
 * THE PER-CLASS GEOMETRY QUADRUPLE
 *
 * Each set-piece class hands the shared helpers four small callbacks:
 *
 *   SHAPE(piece, ends[4])  write the piece's two connection fields (+0x30,
 *                          +0x34) into the direction slots this class
 *                          occupies.  TUNNEL and DROP are north-south
 *                          (slots 0 and 2); CSAW and HOLD UP are east-west
 *                          (slots 3 and 1); the SPECIAL CORNERs switch on
 *                          g_lf_corner_index and fill an adjacent PAIR
 *                          (d, d+1 mod 4) -- which is exactly what makes
 *                          them corners.
 *   PROBE(nb[4])           may this class go on a square with this
 *                          neighbourhood?  Straight pieces accept only the
 *                          masks in their own axis, and DROP additionally
 *                          refuses if the piece north of it already has a
 *                          +0x08 link or the piece south of it a +0x0c link.
 *   GEOM(sq, out)          the screen rectangle/《depth》 record for the
 *                          class, built from the class ObjDef's footprint
 *                          bytes at +0x3c/+0x40 and the map-origin globals
 *                          0x004b4728..0x004b4734.
 *   PLACE(...)             the class's own placement pass.
 *
 * =========================================================================
 * WHAT IS IN THIS FILE  (45 exact, 11 with a measured residual)
 *
 *   0x00409410   LFTrack_NeighbourMask        18 insns
 *   0x00409470   LFTrack_CountNeighbours      18 insns
 *   0x00409510   LFTrack_DropFullNeighbours   39 insns
 *   0x00409580   LFTrack_MaskIsLegal          61 insns
 *   0x004119a0   LFRun_AddCount                8 insns
 *   0x004091f0   LFRun_AddPiece               11 insns
 *   0x00409170   LFPiece_AddSub               11 insns
 *   0x00410910   LFRun_BoatIndex              12 insns
 *   0x00408ec0   LFStation_FindAt             18 insns   [14 of 18 differ]
 *   0x00408ef0   LFPiece_FindAt              21 insns   [17 of 21 differ]
 *   0x00408e80   LFStation_Unlink             25 insns
 *   0x00409010   LFPiece_Alloc                16 insns
 *   0x00409440   LFTrack_ProbeNeighbours      12 insns
 *   0x0040f300   LFTunnel_Shape               12 insns
 *   0x004102e0   LFDrop_Shape                 12 insns
 *   0x0040f7d0   LFCsaw_Shape                 12 insns
 *   0x0040fe50   LFHoldUp_Shape               12 insns
 *   0x0040e340   LFCorner_Shape               31 insns
 *   0x0040f330   LFTunnel_Probe               14 insns
 *   0x0040f800   LFCsaw_Probe                 14 insns
 *   0x0040fe80   LFHoldUp_Probe               14 insns
 *   0x00410310   LFDrop_Probe                 35 insns
 *   0x0040ad50   LFPiece_ShapeIndex           20 insns
 *   0x0040cdf0   LFPiece_IsVisible            16 insns
 *   0x004107b0   LFPiece_ToIndex              37 insns
 *   0x00410b60   LFPiece_FromIndex            31 insns
 *   0x004090e0   LFPiece_RouteHead            18 insns   [4 of 18 differ]
 *   0x00409110   LFPiece_ReverseRoute         14 insns
 *   0x00409140   LFPiece_CursorFits           20 insns
 *   0x0040ba80   LFRun_IsComplete             14 insns
 *   0x004094b0   LFRun_KeepOnly               27 insns
 *   0x00409220   LFRun_UnlinkPiece            28 insns
 *   0x004091a0   LFPiece_UnlinkSub            28 insns
 *   0x00409270   LFRun_RemovePiece            28 insns
 *   0x0040f360   LFTunnel_Geom                37 insns   [25 of 37 differ]
 *   0x00410360   LFDrop_Geom                  36 insns   [15 of 36 differ]
 *   0x0040f830   LFCsaw_Geom                  39 insns   [36 of 39 differ]
 *   0x0040feb0   LFHoldUp_Geom                39 insns   [35 of 39 differ]
 *   0x00411f00   LFAnim_PopHead               11 insns
 *   0x00411ed0   LFAnim_Release               17 insns
 *   0x0040cd70   LFPiece_MarkDrawn            51 insns
 *   0x0040e3b0   LFCorner_Probe               42 insns
 *   0x00410bb0   LFRun_RelinkPieces           37 insns
 *   0x0040cc00   LFTrack_DrawNormal           28 insns
 *   0x00409040   LFPiece_LinkBefore           19 insns
 *   0x00409080   LFPiece_LinkAfter            19 insns
 *   0x00410180   LFDrop_Place                111 insns   [58 of 111 differ]
 *   0x0040d3b0   LFPiece_TickCommon           22 insns
 *   0x0040cfd0   LFPiece_ScreenPos            67 insns
 *   0x0040d210   LFTrack_FindPieceAt          71 insns   [66 of 71 differ]
 *   0x0040d090   LFPiece_QueryRect           135 insns
 *   0x0040d2d0   LFPiece_RefreshAt            75 insns
 *   0x0040e440   LFCorner_Geom               142 insns   [15 of 142 differ]
 *   0x004090c0   LFPiece_SpliceBetween         8 insns
 *   0x00409b70   LFRoute_Join                 72 insns   [6 of 72 differ]
 *   0x00409c20   LFPiece_SetShape            381 insns
 *
 * The residuals are all register permutations or one scheduling choice --
 * every one of them reproduces the original's instruction COUNT, and all but
 * four its byte length exactly.  The note above each marker says precisely
 * what differs and which levers were tried.
 *
 * =========================================================================
 * WHAT IS STILL MISSING FROM THE LOG FLUME  (declared extern here or in
 * logflume.c, addresses from tools/callees.py)
 *
 *   0x0040dc00  LFCorner_Place      574   the SPECIAL CORNER placement pass
 *   0x0040b420  LFEntrance_Interact 532   the station's draw pass
 *   0x0040fad0  LFHoldUp_Place      287
 *   0x0040a600  LFEntrance_Add      256   builds the station's own run
 *   0x0040bf70  LFEntrance_Activate 222   the twelve-case rider state machine
 *   0x0040f050  LFTunnel_Place      217
 *   0x0040f5b0  LFCsaw_Place        172
 *   0x004113d0  LFTrack_BuildGeometry 161
 *   0x0040ca60  LFTrack_DrawAlt     144   (x87; the depth-sorted draw)
 *   0x00410a50  LFRun_LoadPieces     87
 *   0x00410800  LFRun_PrepareSave    87
 *   0x00409360  LFTrack_FillNeighbours    the probe's inner loop
 *   0x00408f30  LFTrack_FindPiece    39   (same dead-lea idiom as FindAt)
 *   0x0040b390  LFPiece_HasRider          walks the run's boats
 *   0x0040d520  LFTrack_CommitPlacement
 *   0x0040d6f0/0x0040d900/0x0040db00  the shared Update/Add/Remove helpers
 *   0x0040c2e0  LFPiece_HasCursor
 *   0x0040cca0  LFPiece_DrawJoin
 * ========================================================================= */

/* ---- shared types (same offsets as logflume.c) -------------------------- */
typedef struct Pos { int x; int y; } Pos;

/* A packed 2-byte map square, passed BY VALUE as a dword. */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

typedef struct FootPart FootPart;

typedef struct Footprint {
    int       v[4];             /* +0x00 */
    FootPart* parts;            /* +0x10 */
} Footprint;                    /* 0x14 */

typedef struct SpriteRec {
    unsigned char pad00[8];
    int           f08;
    unsigned char pad0c[4];
    unsigned int  flags;        /* +0x10 */
} SpriteRec;

typedef struct RideDef {
    unsigned char pad00[8];
    int           f08;
    unsigned char pad0c[8];
    int           f14;
    int           f18;
    unsigned int  flags1c;
    unsigned char pad20[0x1c];
    Footprint     footprint;    /* +0x3c */
    unsigned char pad50[0x14];
    SpriteRec*    sprite;       /* +0x64 */
} RideDef;

typedef struct RideElem {
    char*        name;          /* +0x00 */
    char*        image;         /* +0x04 */
    unsigned int flags;         /* +0x08 */
    RideDef*     data;          /* +0x0c */
} RideElem;

typedef struct LFPiece LFPiece;
typedef struct LFRun   LFRun;

/* A PLACED PIECE of log flume: one map square with a piece definition on it.
 * 0x38 bytes, zeroed by LFPiece_Alloc. */
struct LFPiece {
    LFPiece*      next;         /* +0x00  next piece of the same run */
    LFPiece*      prev;         /* +0x04 */
    LFPiece*      fwd;          /* +0x08  next piece ALONG THE ROUTE */
    LFPiece*      back;         /* +0x0c  previous piece along the route */
    unsigned int  flags;        /* +0x10 */
    BPosW         sq;           /* +0x14  the piece's map square */
    unsigned char pad16[2];
    int           kind;         /* +0x18  3/4 = still has a free end */
    int           dir;          /* +0x1c  orientation / variant, 0..3 */
    RideDef*      def;          /* +0x20 */
    LFRun*        run;          /* +0x24 */
    int           f28;          /* +0x28 */
    LFPiece*      sub;          /* +0x2c  head of this piece's sub-list */
    LFPiece*      end_a;        /* +0x30  the sub-piece at end A */
    LFPiece*      end_b;        /* +0x34  the sub-piece at end B */
};

struct LFRun {
    LFRun*        next;         /* +0x00 */
    unsigned char flags;        /* +0x04 */
    unsigned char pad05[3];
    LFPiece*      f08;          /* +0x08 */
    LFPiece*      f0c;          /* +0x0c */
    LFPiece*      pieces;       /* +0x10  head of this run's piece list */
    BPosW         sq;           /* +0x14  the station's map square */
    unsigned char pad16[2];
    LFPiece*      f18;          /* +0x18 */
    int           frame;        /* +0x1c */
    unsigned char pad20[0xc];
    unsigned char pad2c[0xc];   /* +0x2c  three animation references */
    void*         f38;          /* +0x38  the active boat */
    unsigned char pad3c[0xd0 - 0x3c];
    int           piece_count;  /* +0xd0 */
};                              /* 0xd4 */

/* ---- module globals ---------------------------------------------------- */
extern LFRun*   g_lf_queue;             /* 0x004cbe84  head of the run list */
extern LFPiece* g_lf_nb[4];             /* 0x004cbe20  the neighbour array */
extern int      g_lf_corner_index;      /* 0x004c2af4 */

/* ---- engine / CRT ------------------------------------------------------ */
extern void  free(void* p);                                      /* 0x0049e4d0 */
int   memcmp(const void* a, const void* b, unsigned int n);      /* CRT, intrinsic */
#pragma intrinsic(memcmp)
extern void* malloc(unsigned int n);                             /* 0x0049e4ff */

/* ---- log-flume helpers that live in logflume.c or later here ----------- */
extern LFPiece* LFTrack_FindPiece(const BPos* sq);               /* 0x00408f30 */
extern void     LFTrack_FillNeighbours(BPos sq);                 /* 0x00409360 */
extern LFPiece* LFPiece_FindAt(const BPosW* sq);                 /* 0x00408ef0 (defined below) */
extern int      LFPiece_HasRider(LFPiece* p);                    /* 0x0040b390 */

/* =========================================================================
 * THE NEIGHBOUR ARRAY
 * ========================================================================= */

/* Pack the four neighbour slots into a two-bit-per-direction mask. */
// FUNCTION: LEGOLAND 0x00409410
int LFTrack_NeighbourMask(LFPiece** nb)
{
    int mask = 0;

    if (nb[0])
        mask = 1;
    if (nb[1])
        mask |= 4;
    if (nb[2])
        mask |= 0x10;
    if (nb[3])
        mask |= 0x40;
    return mask;
}

/* How many of the four slots are occupied. Reads the GLOBAL array, not the
 * pointer it is handed -- the parameter is dead. */
// FUNCTION: LEGOLAND 0x00409470
int LFTrack_CountNeighbours(LFPiece** nb)
{
    int n = 0;

    if (g_lf_nb[0])
        n = 1;
    if (g_lf_nb[1])
        n++;
    if (g_lf_nb[2])
        n++;
    if (g_lf_nb[3])
        n++;
    return n;
}

/* Discard every neighbour that has no free end (kind 3 or 4). */
// FUNCTION: LEGOLAND 0x00409510
void LFTrack_DropFullNeighbours(LFPiece** nb)
{
    if (nb[0] != 0 && nb[0]->kind != 3 && nb[0]->kind != 4)
        nb[0] = 0;
    if (nb[1] != 0 && nb[1]->kind != 3 && nb[1]->kind != 4)
        nb[1] = 0;
    if (nb[2] != 0 && nb[2]->kind != 3 && nb[2]->kind != 4)
        nb[2] = 0;
    if (nb[3] != 0 && nb[3]->kind != 3 && nb[3]->kind != 4)
        nb[3] = 0;
}

/* The ten legal neighbourhoods, plus a non-destructive re-test that every
 * surviving neighbour still has a free end. */
// FUNCTION: LEGOLAND 0x00409580
int LFTrack_MaskIsLegal(int mask, LFPiece** nb)
{
    int ok = 0;

    if (mask == 1 || mask == 4 || mask == 0x10 || mask == 0x40 ||
        mask == 0x11 || mask == 0x44 || mask == 5 || mask == 0x14 ||
        mask == 0x50 || mask == 0x41) {
        ok = 1;
        if (nb[0] && nb[0]->kind != 3 && nb[0]->kind != 4)
            ok = 0;
        if (nb[1] && nb[1]->kind != 3 && nb[1]->kind != 4)
            ok = 0;
        if (nb[2] && nb[2]->kind != 3 && nb[2]->kind != 4)
            ok = 0;
        if (nb[3] && nb[3]->kind != 3 && nb[3]->kind != 4)
            ok = 0;
    }
    return ok;
}

/* =========================================================================
 * THE RUN LIST
 * ========================================================================= */

/* Add n squares to a run's length. Tolerates a null run. */
// FUNCTION: LEGOLAND 0x004119a0
void LFRun_AddCount(LFRun* run, int n)
{
    if (run)
        run->piece_count += n;
}

/* Push a piece onto the front of a run's piece list (+0x10). */
// FUNCTION: LEGOLAND 0x004091f0
void LFRun_AddPiece(LFRun* run, LFPiece* p)
{
    p->next = run->pieces;
    p->prev = 0;
    if (run->pieces)
        run->pieces->prev = p;
    run->pieces = p;
}

/* The same push onto a PIECE's sub-list (+0x2c). */
// FUNCTION: LEGOLAND 0x00409170
void LFPiece_AddSub(LFPiece* parent, LFPiece* p)
{
    p->next = parent->sub;
    p->prev = 0;
    if (parent->sub)
        parent->sub->prev = p;
    parent->sub = p;
}

/* Which of the run's four 0x24-byte boat records +0x38 currently points at,
 * or -1.  This is the index the save code writes. */
// FUNCTION: LEGOLAND 0x00410910
int LFRun_BoatIndex(LFRun* run)
{
    int   i = 0;
    void* boat = run->f38;
    char* p    = (char*)run + 0x40;

    for (; i < 4; i++, p += 0x24) {
        if (boat == (void*)p)
            return i;
    }
    return -1;
}

/* Find the station/run whose map square is sq.
 *
 * Closed (18/18) with waterworks.c's WW_ListFind lever: the square compare
 * is an INTRINSIC memcmp of length 2, not a 16-bit '=='.  VC6 expands it to
 * 'lea edx,[run+0x14] / mov dx,[run+0x14] / cmp dx,[sq]' -- the lea is the
 * intrinsic's first-operand address, left DEAD once the load folds the
 * addressing mode back onto the base, and the second operand stays a memory
 * operand instead of being hoisted out of the loop.  Every '==' spelling
 * (volatile included) hoists *sq and drops the leas.  The same fix applies
 * to ridecb3.c's Carousel_FindRec / Balloonz_FindRec and westtown.c's
 * JailCell_FindRecord (not this lane's files). */
// FUNCTION: LEGOLAND 0x00408ec0
LFRun* LFStation_FindAt(const BPosW* sq)
{
    LFRun* run = g_lf_queue;

    if (run) {
        while (memcmp(&run->sq, sq, 2) != 0) {
            run = run->next;
            if (!run)
                return 0;
        }
        return run;
    }
    return 0;
}

/* The same walk one level deeper: for every run, over its piece list.
 * Closed (21/21) by the same intrinsic memcmp as LFStation_FindAt. */
// FUNCTION: LEGOLAND 0x00408ef0
LFPiece* LFPiece_FindAt(const BPosW* sq)
{
    LFRun*   run = g_lf_queue;
    LFPiece* p;

    while (run) {
        p = run->pieces;
        while (p) {
            if (memcmp(&p->sq, sq, 2) == 0)
                return p;
            p = p->next;
        }
        run = run->next;
    }
    return 0;
}

/* Unlink a run from the global list and free it.
 *
 * The walk is a POINTER-TO-POINTER walk through the link slot: because
 * LFRun::next sits at offset 0, `(LFRun**)p` and `&p->next` are the same
 * address, and the original really is spelled that way -- that is what makes
 * VC6 emit `cmp dword ptr [eax],ecx` (a single-use memory compare) instead
 * of loading p->next into a register and reusing it for the advance.
 * The walk derefs without a leading null check, so a call with an EMPTY
 * global list and a non-null run faults; reproduced. */
// FUNCTION: LEGOLAND 0x00408e80
void LFStation_Unlink(LFRun* run)
{
    LFRun** pp;
    LFRun*  p = g_lf_queue;

    if (p == run) {
        g_lf_queue = run->next;
        goto done;
    }
    pp = (LFRun**)p;
    while (*pp != run) {
        pp = (LFRun**)*pp;
        if (pp == 0)
            break;
    }
    if (pp)
        *pp = run->next;
done:
    free(run);
}

/* Allocate and zero a 0x38-byte piece record. */
// FUNCTION: LEGOLAND 0x00409010
LFPiece* LFPiece_Alloc(void)
{
    LFPiece* p = (LFPiece*)malloc(0x38);

    if (p) {
        int  n = 0xe;
        int* d = (int*)p;
        while (n--)
            *d++ = 0;
    }
    return p;
}

/* Fill the global four-slot neighbour array for a square and hand back its
 * address.  Every caller shares the one array. */
// FUNCTION: LEGOLAND 0x00409440
void LFTrack_ProbeNeighbours(BPos sq, LFPiece*** out)
{
    g_lf_nb[0] = 0;
    g_lf_nb[1] = 0;
    g_lf_nb[2] = 0;
    g_lf_nb[3] = 0;
    LFTrack_FillNeighbours(sq);
    *out = g_lf_nb;
}

/* =========================================================================
 * THE PER-CLASS SHAPE CALLBACKS
 *
 * ends[dir] is the piece this square offers a connection to in direction
 * dir.  TUNNEL and DROP publish north/south; CSAW and HOLD UP publish
 * west/east; a SPECIAL CORNER publishes an adjacent pair chosen by
 * g_lf_corner_index, which is what makes it a corner.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0040f300
void LFTunnel_Shape(LFPiece* p, LFPiece** ends)
{
    ends[0] = 0;
    ends[1] = 0;
    ends[2] = 0;
    ends[3] = 0;
    ends[0] = p->end_a;
    ends[2] = p->end_b;
}

// FUNCTION: LEGOLAND 0x004102e0
void LFDrop_Shape(LFPiece* p, LFPiece** ends)
{
    ends[0] = 0;
    ends[1] = 0;
    ends[2] = 0;
    ends[3] = 0;
    ends[0] = p->end_a;
    ends[2] = p->end_b;
}

// FUNCTION: LEGOLAND 0x0040f7d0
void LFCsaw_Shape(LFPiece* p, LFPiece** ends)
{
    ends[0] = 0;
    ends[1] = 0;
    ends[2] = 0;
    ends[3] = 0;
    ends[3] = p->end_a;
    ends[1] = p->end_b;
}

// FUNCTION: LEGOLAND 0x0040fe50
void LFHoldUp_Shape(LFPiece* p, LFPiece** ends)
{
    ends[0] = 0;
    ends[1] = 0;
    ends[2] = 0;
    ends[3] = 0;
    ends[3] = p->end_a;
    ends[1] = p->end_b;
}

// FUNCTION: LEGOLAND 0x0040e340
void LFCorner_Shape(LFPiece* p, LFPiece** ends)
{
    ends[0] = 0;
    ends[1] = 0;
    ends[2] = 0;
    ends[3] = 0;
    switch (p->dir) {
    case 0:
        ends[0] = p->end_a;
        ends[1] = p->end_b;
        break;
    case 1:
        ends[1] = p->end_a;
        ends[2] = p->end_b;
        break;
    case 2:
        ends[2] = p->end_a;
        ends[3] = p->end_b;
        break;
    case 3:
        ends[3] = p->end_a;
        ends[0] = p->end_b;
        break;
    }
}

/* =========================================================================
 * THE PER-CLASS PROBE CALLBACKS -- may this class go on this square?
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0040f330
int LFTunnel_Probe(LFPiece** nb)
{
    int mask = LFTrack_NeighbourMask(nb);

    if (mask != 0x11 && mask != 1 && mask != 0x10)
        return 0;
    return 1;
}

// FUNCTION: LEGOLAND 0x0040f800
int LFCsaw_Probe(LFPiece** nb)
{
    int mask = LFTrack_NeighbourMask(nb);

    if (mask != 0x44 && mask != 4 && mask != 0x40)
        return 0;
    return 1;
}

// FUNCTION: LEGOLAND 0x0040fe80
int LFHoldUp_Probe(LFPiece** nb)
{
    int mask = LFTrack_NeighbourMask(nb);

    if (mask != 0x44 && mask != 4 && mask != 0x40)
        return 0;
    return 1;
}

/* The DROP additionally refuses a square whose northern neighbour already
 * has an A link or whose southern neighbour already has a B link -- a drop
 * has to own both ends of its fall. */
// FUNCTION: LEGOLAND 0x00410310
int LFDrop_Probe(LFPiece** nb)
{
    int mask = LFTrack_NeighbourMask(nb);

    if (mask != 0x11 && mask != 1 && mask != 0x10)
        return 0;
    if ((mask & 1) && nb[0]->fwd != 0)
        return 0;
    if ((mask & 0x10) && nb[2]->back != 0)
        return 0;
    return 1;
}

/* =========================================================================
 * PIECE LOOK-UPS
 * ========================================================================= */

/* The sprite table a piece draws from: one table per kind, indexed by the
 * piece's orientation.  The three tables OVERLAP in .rdata (0x004b473c,
 * 0x004b474c and 0x004b4754 are eight bytes apart), which is reproduced by
 * indexing one array. */
extern int g_lf_shape_tbl[];        /* 0x004b473c */
extern int g_lf_shape_end;          /* 0x004b4764 */

// FUNCTION: LEGOLAND 0x0040ad50
int LFPiece_ShapeIndex(LFPiece* p)
{
    int r = 0;

    if (p) {
        switch (p->kind) {
        case 1:
            r = g_lf_shape_tbl[4 + p->dir];
            break;
        case 2:
            r = g_lf_shape_tbl[6 + p->dir];
            break;
        case 3:
            r = g_lf_shape_tbl[p->dir];
            break;
        case 4:
            r = g_lf_shape_end;
            break;
        }
    }
    return r;
}

/* Is there a piece on this square, and is it carrying a rider? */
// FUNCTION: LEGOLAND 0x0040cdf0
int LFPiece_IsVisible(const BPos* sq)
{
    int      r = 0;
    LFPiece* p = LFPiece_FindAt((const BPosW*)sq);

    if (p)
        r = LFPiece_HasRider(p);
    return r;
}

/* =========================================================================
 * THE SAVE-FILE INDEX PAIR
 *
 * A pointer into the run graph is saved as ONE dword holding two 1-based
 * 16-bit indices: the low half counts down the outer list from the head,
 * the high half counts down the selected node's +0x2c sub-list.  0 and
 * 0xffff both mean "nothing"; -1 (0xffffffff) is the not-found answer.
 * ========================================================================= */

/* Exact under tools/audit.py: 37 instructions, 71 bytes, zero mismatches.
 * Held as WIP for a TOOLING reason only. This function is RECURSIVE, and a
 * self-call inside its own COMDAT is not relocated: our object file encodes it
 * as a direct relative call, so it disassembles with a small numeric target
 * while the original shows an absolute address. audit.py normalises both
 * (norm2 rewrites bare-numeric branch targets); the shared tools/match.py
 * normalises only 0x-prefixed ones, so it reports a single mismatch and
 * verify.py drops below 100%. Promote when match.py adopts the same rule. */
// FUNCTION: LEGOLAND 0x004107b0
int LFPiece_ToIndex(LFPiece* head, LFPiece* target)
{
    int      n = 1;
    LFPiece* p = head;

    while (p) {
        int sub;
        if (p == target)
            return n;
        sub = LFPiece_ToIndex(p->sub, target);
        if (sub != -1)
            return (sub << 16) | n;
        p = p->next;
        n++;
    }
    return -1;
}

// FUNCTION: LEGOLAND 0x00410b60
LFPiece* LFPiece_FromIndex(LFPiece* head, unsigned int packed)
{
    unsigned int lo = packed & 0xffff;
    unsigned int hi = packed >> 16;
    LFPiece*     p;

    if (lo == 0)
        return 0;
    if (lo == 0xffff)
        return 0;
    lo--;
    if (lo != 0) {
        unsigned int n = lo;
        p = head;
        do {
            p = p->next;
        } while (--n);
    } else {
        p = head;
    }
    if (hi != 0 && hi != 0xffff) {
        p = p->sub;
        while (--hi)
            p = p->next;
    }
    return p;
}

/* =========================================================================
 * THE ROUTE -- the SECOND linkage
 *
 * Besides the run's flat piece list (+0x00/+0x04), every piece carries a
 * doubly-linked ROUTE link: +0x08 forward, +0x0c back.  That chain is the
 * order water actually travels in, and it is what the entrance's start
 * (LFRun +0x08) and end (LFRun +0x0c) pieces bracket.
 * ========================================================================= */

/* Walk back to the head of a piece's route.  Returns the piece itself when
 * the route is a closed ring (the walk comes back round), and 0 when the
 * chain runs into a null in the middle.
 *
 * Closed (18/18) by re-reading the link in the loop CONDITION: 'while
 * (p->back) { p = p->back; ... }' lets VC6 CSE the load into ecx, copy it
 * into eax FIRST and compare eax with start; the explicit-walker form
 * 'q = p->back; while (q) { p = q; if (p == start) ...' compares ecx before
 * the copy.  The redundant 'if (p == 0) return 0;' is really there (test
 * eax,eax after the ring compare) -- keep it. */
// FUNCTION: LEGOLAND 0x004090e0
LFPiece* LFPiece_RouteHead(LFPiece* p)
{
    LFPiece* start = p;

    while (p->back) {
        p = p->back;
        if (p == start)
            return start;
        if (p == 0)
            return 0;
    }
    return p;
}

/* Reverse a whole route in place by swapping every piece's two links. */
// FUNCTION: LEGOLAND 0x00409110
void LFPiece_ReverseRoute(LFPiece* p)
{
    LFPiece* q = LFPiece_RouteHead(p);

    while (q) {
        LFPiece* f = q->fwd;
        q->fwd  = q->back;
        q->back = f;
        q = f;
    }
}

/* Does this piece's route reach the run's START piece (LFRun +0x08)?  That
 * is the test the cursor code uses to decide whether a square the mouse is
 * over belongs to the route the station feeds. */
// FUNCTION: LEGOLAND 0x00409140
int LFPiece_CursorFits(LFPiece* piece)
{
    LFPiece* p = LFPiece_RouteHead(piece);

    while (p) {
        if (p == piece->run->f08)
            return 1;
        p = p->fwd;
    }
    return 0;
}

/* A run is COMPLETE when its route walks from the start piece all the way
 * to the end piece. */
// FUNCTION: LEGOLAND 0x0040ba80
int LFRun_IsComplete(LFRun* run)
{
    LFPiece* p = run->f08;

    while (p) {
        p = p->fwd;
        if (p == run->f0c)
            return 1;
    }
    return 0;
}

/* Drop every neighbour that belongs to a different run. */
// FUNCTION: LEGOLAND 0x004094b0
void LFRun_KeepOnly(LFRun* run, LFPiece** nb)
{
    if (nb[0] && nb[0]->run != run)
        nb[0] = 0;
    if (nb[1] && nb[1]->run != run)
        nb[1] = 0;
    if (nb[2] && nb[2]->run != run)
        nb[2] = 0;
    if (nb[3] && nb[3]->run != run)
        nb[3] = 0;
}

/* Take a piece out of a run's flat list AND out of its route. */
// FUNCTION: LEGOLAND 0x00409220
void LFRun_UnlinkPiece(LFRun* run, LFPiece* p)
{
    if (p->prev != 0)
        p->prev->next = p->next;
    if (p->next != 0)
        p->next->prev = p->prev;
    if (p == run->pieces)
        run->pieces = p->next;
    if (p->back != 0)
        p->back->fwd = 0;
    if (p->fwd != 0)
        p->fwd->back = 0;
}

/* The same for a sub-piece of a compound piece. */
// FUNCTION: LEGOLAND 0x004091a0
void LFPiece_UnlinkSub(LFPiece* parent, LFPiece* p)
{
    if (p->prev != 0)
        p->prev->next = p->next;
    if (p->next != 0)
        p->next->prev = p->prev;
    if (p == parent->sub)
        parent->sub = p->next;
    if (p->back != 0)
        p->back->fwd = 0;
    if (p->fwd != 0)
        p->fwd->back = 0;
}

/* Destroy a piece: free its whole sub-list first, then unlink and free it. */
// FUNCTION: LEGOLAND 0x00409270
void LFRun_RemovePiece(LFRun* run, LFPiece* p)
{
    LFPiece* s = p->sub;

    if (s) {
        do {
            LFPiece* next = s->next;
            LFPiece_UnlinkSub(p, s);
            free(s);
            s = next;
        } while (s);
    }
    LFRun_UnlinkPiece(run, p);
    free(p);
}

/* =========================================================================
 * THE PER-CLASS GEOM CALLBACK
 *
 * GEOM(sq, out) fills a 9-int record: a four-bit direction mask (N=1, E=2,
 * S=4, W=8) followed by FOUR (x,y) connection points, one per direction --
 * the same four slots the SHAPE callback fills with pieces.  So a class's
 * shape says WHICH sides it connects on and its geom says WHERE on the map
 * those connections sit.
 *
 * Both coordinates start from the map square biased by the class's own
 * footprint origin (the low BYTES of ObjDef +0x3c/+0x40 -- the addition is
 * done in 8 bits and wraps), and the far point is pushed out by the
 * difference between the CLASS footprint's span and the flume cell's span
 * (g_lf_footprint), so a piece whose art is taller/wider than one cell
 * still hands back a connection point on its own far edge.
 *
 *   TUNNEL  mask 5  (N|S)  N=(x+6,y)  S=(x+2, y+1+dh)
 *   DROP    mask 5  (N|S)  N=(x+2,y)  S=(x+2, y+1+dh)
 *   CSAW    mask a  (E|W)  W=(x,y+3)  E=(x+1+dw, y+1)
 *   HOLD UP mask a  (E|W)  W=(x,y+9)  E=(x+1+dw, y+7)
 * ========================================================================= */

/* RESIDUAL, all four: instruction count and byte length are exact, but ecx
 * and edx are exchanged throughout -- the original keeps the flume-cell span
 * in ecx and rotates edx through the byte temporary and the two coordinates,
 * this reconstruction the other way round.  Reordering the declarations,
 * splitting the span into two globals, hoisting the footprint bytes into
 * unsigned char locals and moving the span computation below the byte adds
 * all leave the pair swapped or change the schedule outright. */

typedef struct LFGeom {
    int dirs;                   /* +0x00  N=1 E=2 S=4 W=8 */
    Pos pt[4];                  /* +0x04  one connection point per direction */
} LFGeom;                       /* 0x24 */

extern Footprint g_lf_footprint;        /* 0x004b4728  the flume cell rect */
extern RideDef*  g_lftu_def;            /* 0x004cbe18  LOG FLUME TUNNEL */
extern RideDef*  g_lfdr_def;            /* 0x004c8d6c  LOG FLUME DROP */
extern RideDef*  g_lfcs_def;            /* 0x004c2bf0  LOG FLUME CSAW */
extern RideDef*  g_lfhu_def;            /* 0x004c2b60  LOG FLUME HOLD UP */

/* Closed (37/37) like LFDrop_Geom: coordinates read straight from the byte
 * parameter at each store, in the order N.x, N.y, S.x; only the y of the
 * final sum goes through a named int.  Store order matters here: N.x, S.x,
 * N.y (18 differ) and any 'int x' routing (23-25) both lose. */
// FUNCTION: LEGOLAND 0x0040f360
void LFTunnel_Geom(BPos sq, LFGeom* out)
{
    int      h = g_lf_footprint.v[3] - g_lf_footprint.v[1];
    RideDef* def = g_lftu_def;
    int      y;

    sq.x = (unsigned char)(sq.x + (unsigned char)def->footprint.v[0]);
    sq.y = (unsigned char)(sq.y + (unsigned char)def->footprint.v[1]);
    out->dirs = 5;
    out->pt[0].x = sq.x + 6;
    out->pt[0].y = sq.y;
    out->pt[2].x = sq.x + 2;
    y = sq.y;
    out->pt[2].y = (g_lftu_def->footprint.v[3] - g_lftu_def->footprint.v[1])
                   - h + y + 1;
}

/* Closed (36/36): the coordinates are read straight from the byte PARAMETER
 * at each store ('sq.x + 2', 'sq.y'); only the y used in the final sum goes
 * through a named int.  Routing x through 'int x = sq.x' gave the temporary
 * a higher allocation priority than the span and swapped ecx/edx throughout. */
// FUNCTION: LEGOLAND 0x00410360
void LFDrop_Geom(BPos sq, LFGeom* out)
{
    int      h = g_lf_footprint.v[3] - g_lf_footprint.v[1];
    RideDef* def = g_lfdr_def;
    int      y;

    sq.x = (unsigned char)(sq.x + (unsigned char)def->footprint.v[0]);
    sq.y = (unsigned char)(sq.y + (unsigned char)def->footprint.v[1]);
    out->dirs = 5;
    out->pt[0].x = sq.x + 2;
    out->pt[0].y = sq.y;
    out->pt[2].x = sq.x + 2;
    y = sq.y;
    out->pt[2].y = (g_lfdr_def->footprint.v[3] - g_lfdr_def->footprint.v[1])
                   - h + y + 1;
}

/* Closed (39/39).  Three things had to be right at once: the span is a
 * named local at the top (VC6 then schedules its two loads after the byte
 * adds and sinks the sub to the tail by itself); the W point is stored
 * straight from the byte parameter ('sq.x', 'sq.y + 3') with x and y copied
 * into ints only afterwards; and E.x is stored BEFORE E.y, so that the
 * 'inc esi / store' of y+1 lands between the class-span loads and the final
 * lea.  E.y first (any spelling) leaves 20-38 differing. */
// FUNCTION: LEGOLAND 0x0040f830
void LFCsaw_Geom(BPos sq, LFGeom* out)
{
    int      w = g_lf_footprint.v[2] - g_lf_footprint.v[0];
    RideDef* def = g_lfcs_def;
    int      x;
    int      y;

    sq.x = (unsigned char)(sq.x + (unsigned char)def->footprint.v[0]);
    sq.y = (unsigned char)(sq.y + (unsigned char)def->footprint.v[1]);
    out->dirs = 0xa;
    out->pt[3].x = sq.x;
    out->pt[3].y = sq.y + 3;
    x = sq.x;
    y = sq.y;
    out->pt[1].x = (g_lfcs_def->footprint.v[2] - g_lfcs_def->footprint.v[0])
                   - w + x + 1;
    out->pt[1].y = y + 1;
}

/* Closed (39/39) with exactly LFCsaw_Geom's spelling. */
// FUNCTION: LEGOLAND 0x0040feb0
void LFHoldUp_Geom(BPos sq, LFGeom* out)
{
    int      w = g_lf_footprint.v[2] - g_lf_footprint.v[0];
    RideDef* def = g_lfhu_def;
    int      x;
    int      y;

    sq.x = (unsigned char)(sq.x + (unsigned char)def->footprint.v[0]);
    sq.y = (unsigned char)(sq.y + (unsigned char)def->footprint.v[1]);
    out->dirs = 0xa;
    out->pt[3].x = sq.x;
    out->pt[3].y = sq.y + 9;
    x = sq.x;
    y = sq.y;
    out->pt[1].x = (g_lfhu_def->footprint.v[2] - g_lfhu_def->footprint.v[0])
                   - w + x + 1;
    out->pt[1].y = y + 7;
}

/* =========================================================================
 * THE RUN'S ANIMATION REFERENCE LIST  (LFRun +0x2c)
 * ========================================================================= */

typedef struct LFAnimNode LFAnimNode;
struct LFAnimNode { LFAnimNode* next; };

typedef struct LFAnimRefs {
    int         f00;            /* +0x00 */
    LFAnimNode* head;           /* +0x04 */
    LFAnimNode* tail;           /* +0x08 */
} LFAnimRefs;

// FUNCTION: LEGOLAND 0x00411f00
void LFAnim_PopHead(LFAnimRefs* r)
{
    LFAnimNode* h = r->head;

    if (h) {
        r->head = h->next;
        if (r->tail == h)
            r->tail = 0;
    }
}

/* Pop and free every reference the run holds. */
// FUNCTION: LEGOLAND 0x00411ed0
void LFAnim_Release(LFAnimRefs* r)
{
    LFAnimNode* n = r->head;

    while (n) {
        LFAnim_PopHead(r);
        free(n);
        n = r->head;
    }
}

/* =========================================================================
 * DRAWING AND REPAIR
 * ========================================================================= */

extern void  LFPiece_DrawJoin(LFPiece* p);                       /* 0x0040cca0 */
extern void* g_lf_sprites[];                                     /* 0x004c2abc */
extern void  PrintSprite(void* spr, int x, int y, int mode,
                         int flag);                              /* 0x004853a0 */

/* Repaint the two sub-pieces that bracket a compound piece, but only the
 * ones whose orientation faces the way the caller is drawing: with mode set
 * an orientation of 0 or 3 is skipped, without it 1 or 2 is.  A piece with
 * no sub-list has no joins to draw. */
// FUNCTION: LEGOLAND 0x0040cd70
void LFPiece_MarkDrawn(LFPiece* p, int mode)
{
    LFPiece* a;
    LFPiece* b;

    if (p->sub == 0)
        return;
    a = p->end_a;
    b = p->end_b;
    if (a->kind == 3) {
        int d = a->dir;
        if (mode) {
            if (d != 0 && d != 3)
                LFPiece_DrawJoin(a);
        } else if (d != 1 && d != 2) {
            LFPiece_DrawJoin(a);
        }
    }
    if (b->kind == 3) {
        int d = b->dir;
        if (mode) {
            if (d != 0 && d != 3)
                LFPiece_DrawJoin(b);
        } else if (d != 1 && d != 2) {
            LFPiece_DrawJoin(b);
        }
    }
}

/* The SPECIAL CORNER probe: each of the four corner classes accepts its own
 * three masks -- the elbow itself and either of its two single ends. */
// FUNCTION: LEGOLAND 0x0040e3b0
int LFCorner_Probe(LFPiece** nb)
{
    int mask = LFTrack_NeighbourMask(nb);

    switch (g_lf_corner_index) {
    case 0:
        if (mask == 5 || mask == 1 || mask == 4)
            return 1;
        break;
    case 1:
        if (mask == 0x14 || mask == 4 || mask == 0x10)
            return 1;
        break;
    case 2:
        if (mask == 0x50 || mask == 0x10 || mask == 0x40)
            return 1;
        break;
    case 3:
        if (mask == 0x41 || mask == 0x40 || mask == 1)
            return 1;
        break;
    }
    return 0;
}

/* After a load, every pointer field in a piece holds the packed index pair
 * it was SAVED as; walk the list turning them back into pointers, sub-lists
 * included. */
/* Exact under tools/audit.py: 37 instructions, 88 bytes, zero mismatches.
 * Held as WIP for a TOOLING reason only. This function is RECURSIVE, and a
 * self-call inside its own COMDAT is not relocated: our object file encodes it
 * as a direct relative call, so it disassembles with a small numeric target
 * while the original shows an absolute address. audit.py normalises both
 * (norm2 rewrites bare-numeric branch targets); the shared tools/match.py
 * normalises only 0x-prefixed ones, so it reports a single mismatch and
 * verify.py drops below 100%. Promote when match.py adopts the same rule. */
// FUNCTION: LEGOLAND 0x00410bb0
void LFRun_RelinkPieces(LFPiece* head, LFPiece* p)
{
    while (p) {
        p->fwd   = LFPiece_FromIndex(head, (unsigned int)p->fwd);
        p->back  = LFPiece_FromIndex(head, (unsigned int)p->back);
        p->end_a = LFPiece_FromIndex(head, (unsigned int)p->end_a);
        p->end_b = LFPiece_FromIndex(head, (unsigned int)p->end_b);
        LFRun_RelinkPieces(head, p->sub);
        p = p->next;
    }
}

/* Screen position of a piece: 8 bytes, returned in eax:edx. */
extern Pos LFPiece_ScreenPos(LFPiece* p);                        /* 0x0040cfd0 */

/* Draw one plain flume square: pick its sprite from the shape table and
 * blit it at the piece's screen position. */
// FUNCTION: LEGOLAND 0x0040cc00
void LFTrack_DrawNormal(LFPiece* p, int mode)
{
    int   idx;
    Pos   pos;
    void* spr;

    if (!LFPiece_HasRider(p))
        return;
    idx = LFPiece_ShapeIndex(p);
    pos = LFPiece_ScreenPos(p);
    spr = g_lf_sprites[idx];
    if (spr)
        PrintSprite(spr, pos.x, pos.y, mode, 0);
}

/* =========================================================================
 * SPLICING THE ROUTE
 *
 * Both splices WARN (through the CRT debug hook at 0x0049e5c5) when the end
 * they are attaching to is already occupied -- and then overwrite it anyway.
 * That is the original behaviour and is reproduced.
 * ========================================================================= */

extern void DebugPrint(const char* msg);                         /* 0x0049e5c5 */
extern char g_lf_link_warning[];                                 /* 0x004b48e0 */

// FUNCTION: LEGOLAND 0x00409040
void LFPiece_LinkBefore(LFPiece* a, LFPiece* b)
{
    if (a->back)
        a->back->fwd = b;
    b->fwd = a;
    if (a->back)
        DebugPrint(g_lf_link_warning);
    a->back = b;
}

// FUNCTION: LEGOLAND 0x00409080
void LFPiece_LinkAfter(LFPiece* a, LFPiece* b)
{
    if (a->fwd)
        a->fwd->back = b;
    b->back = a;
    if (a->fwd)
        DebugPrint(g_lf_link_warning);
    a->fwd = b;
}

/* =========================================================================
 * BUILDING A SET PIECE OUT OF FLUME SQUARES
 *
 * A DROP is not one object: its ObjDef covers 24 map units of height, and
 * LFDrop_Place fills that height with (24 / cell_height) ordinary flume
 * sub-pieces threaded onto the parent's sub-list AND onto a route of their
 * own -- an end piece (kind 3, orientation 0) at the top, plain channel
 * (kind 1) in the middle and another end piece (kind 3, orientation 2) at
 * the bottom.  The parent records the two ends at +0x30/+0x34, which is
 * exactly what LFDrop_Shape then publishes north and south.
 *
 * Every sub-piece is stamped with the TRACK class definition (not the
 * drop's own), the parent's run, a back-pointer to the parent at +0x28 and
 * flag bit 2 -- "this square belongs to a set piece", which is what stops
 * it being billed and demolished on its own.
 * ========================================================================= */

extern RideDef* g_lftr_def;             /* 0x004cbe30  LOG FLUME TRACK */

/* The sub-piece constructor, as a macro: VC6 expands it three times here
 * with no call, which is how the original reads.
 *
 * The STORE ORDER below is not the original's: it is the order that, given
 * the volatile shim, reproduces most of the original's emitted stream.  VC6
 * reorders adjacent stores freely, so the source order here is a knob, not
 * a fact -- see the residual note for which order the evidence points at.
 *
 * The `volatile` read of X is a SHIM, not the original's spelling -- see the
 * residual note on LFDrop_Place below.  It costs nothing semantically (a
 * volatile-qualified read of a plain local); it exists only to keep X out of
 * the register race so the running Y wins ebx, as the original has it. */
#define LF_MAKE_SUB(kind_, dir_)                                          \
    sub = LFPiece_Alloc();                                                \
    if (sub) {                                                            \
        sub->sq.b.x = *(volatile unsigned char*)&x;                       \
        sub->dir   = (dir_);                                              \
        sub->run   = parent->run;                                         \
        sub->f28   = (int)parent;                                         \
        sub->def   = g_lftr_def;                                          \
        sub->flags |= 4;                                                  \
        sub->kind  = (kind_);                                             \
        sub->sq.b.y = y;                                                  \
    }                                                                     

/* RESIDUAL: 111/111 instructions, 344 of 341 bytes, mismatch 82 -> 58 -> 45
 * by audit.py.  LCS-aligned register+offset-blind distance 18 of 111.
 * First diverging index 1.
 *
 * THE ALLOCATION (found in an earlier round, unchanged).  The function has
 * seven call-crossing values (parent, sub, prev, cellh, n, x, y) and only
 * four callee-saved registers, so exactly one of the two byte coordinates
 * gets ebx and the other is spilled into the DEAD parameter home at
 * [esp+0x1c].  The original gives ebx to the running Y -- `add bl, byte ptr
 * [esp+0x10]` is the loop's `y += cellh` -- and spills X at its definition
 * (`mov byte ptr [esp+0x1c], al`), reloading it at the TOP of each of the
 * three sub-piece blocks.  Every plain-C spelling picks the OTHER one: X in
 * ebx, Y spilled, which costs three extra instructions at each `y += cellh`.
 * The `volatile` READ of X in the macro above is the shim that takes X out
 * of that race; with it, `add bl,[esp+0x10]`, the whole flags/def store
 * group and the frame layout are exact.  Declaring Y an `int` also flips the
 * ranking (proof that TYPE, not spelling, is what the allocator reads) but
 * then emits `and ebx,0xff` and a dword add, so it cannot reach 0.
 *
 * 2026-09-04 (lane D): 58 -> 45, and the residual is now a SINGLE COUPLED
 * SWAP.  The gain came from re-running the store-order sweep with the
 * volatile read moved to the FIRST statement of the block and ranking the
 * 720 results by LCS-aligned structural distance rather than by the strict
 * index count -- the strict count had previously selected an order that is
 * worse on every structural measure (committed order was
 * kind,dir,x,run,def,flags,f28,y: strict 58 / structural 25; the order now
 * in the macro is strict 45 / structural 18, better on BOTH).
 *   With `sub->sq.b.x = *(volatile ...)&x;` written first, its load lands at
 *   the block top exactly where the original has it, and after the sweep the
 *   ONLY per-block difference left is that `mov byte ptr [esi+0x14], dl` and
 *   `mov dword ptr [esi+0x18], <kind>` are EXCHANGED: the original emits the
 *   kind store first and the x store second-to-last; we emit the x store
 *   first and the kind store second-to-last.  That is exactly the pair the
 *   shim couples -- VC6 will not hoist a volatile load, so the load and its
 *   store cannot be separated, and one of the two must be wrong.  Nothing
 *   else in blocks 1 and 2 differs, f28 included.
 *
 * MEASURED AND RULED OUT (re-run on the CURRENT baseline unless noted):
 *   - the full 720 orders of the eight stores with X first, ranked
 *     structurally; and the earlier 720 with X free, ranked strictly.
 *   - a block-scope temp `unsigned char xv = *(volatile ...)&x;` as the first
 *     statement with the store anywhere later, over four xv types
 *     (char/unsigned char/int/unsigned) x the store orders: best 61.  The
 *     temp costs a register; it does NOT decouple the load from the store.
 *   - X CANNOT BE MADE MEMORY-RESIDENT WITHOUT `volatile`: a
 *     `union { unsigned char b; unsigned short w; }` (read either way), a
 *     two-field struct with a live second field, `*(unsigned char*)&x`, a
 *     `static __inline` helper taking `&x`, `unsigned char x[1]` and `x[2]`
 *     are all scalarised straight back into the 82-point plain-C attractor.
 *     Declaring the whole variable `volatile unsigned char x` is worse (105)
 *     because the STORE is pinned too.  A `volatile unsigned char*` local
 *     assigned `&x` is byte-identical to the read shim.
 *   - the full x/y TYPE cross-product (6 x 6: char, unsigned char, short,
 *     unsigned short, int, unsigned, with the wrap written
 *     `y = (unsigned char)(y + cellh)` wherever y is wider), measured
 *     without the shim: best 82, both narrow.  With the shim only
 *     `unsigned char`/`char` y holds; short 108, int/unsigned 101.
 *     `unsigned char cellh` is 84.
 *   - THE TRIP COUNT IS CLOSED.  `24 / cellh - 2u`, `+ 0xfffffffeu`,
 *     `+ (int)0xfffffffe`, `+ -2`, `(unsigned)(24/cellh) + 0xfffffffeu`,
 *     `+ ~1`, `+ (0 - 2)`, a separate `n -= 2;`, a separate `n = n + (-2);`
 *     and the `while` loop form ALL compile byte-identically: VC6 folds
 *     every spelling, unsigned wrap included, to one SUB node.  The
 *     original's `add eax, -2` (index 45) is not reachable from any C
 *     spelling of this expression and must mean the value arrives from a
 *     different computation.
 *   - the head: all 12 dependency-legal orders of {cellh, def, px, py,
 *     x-calc, y-calc}, re-run.  Putting `cellh` first, or inlining
 *     `g_lfdr_def` at both uses, each FIXES index 1 (the eax<->ecx swap of
 *     the two head loads) and costs three elsewhere -- 48 either way.
 *     Swapping px/py is byte-identical; the y calc first is 71.
 *   - `if (x) {}` / `if (y) {}` empty-body extra-use probes (inert); four
 *     loop forms; splitting the cellh subtraction.
 *
 * STILL UNEXPLAINED, in order of size:
 *   1. the coupled x-store/kind-store swap (3 blocks, 6 indices) -- needs the
 *      shim gone;
 *   2. the head's px/py load-store interleave at indices 11..19;
 *   3. `add eax,-2` vs `sub eax,2` at index 45;
 *   4. the last block: VC6 sinks the final `y += cellh` past LFPiece_Alloc
 *      and emits `mov al,[esp+0x10] / add al,bl / or edx,4` where the
 *      original has `add bl,[esp+0x10]` BEFORE the call and `or al,4` after.
 *      That is where the three extra bytes are.
 *
 * NEXT: find the plain-C construct that lowers X's allocation priority below
 * Y's -- then the shim comes out, the reload hoists on its own and the store
 * order can go back to the natural kind,dir,run,f28,def,flags,x,y (which,
 * with the shim, gives EXACTLY 341/341 bytes but strict 66).  Every
 * construct that merely takes X's address is scalarised; what is wanted is
 * something that lowers its RANK, and the two-way unsigned-char tie-break
 * lever does not apply because the competitor here is a pointer, not a
 * second char.  Sibling LFTunnel_Place (0x0040f050, not yet ported) has the
 * IDENTICAL head shape with `add al, 6` instead of `add al, 2`, so whatever
 * closes this closes that too.
 *
 * 2026-09-04 (fifth lane).  Unchanged at 45; not re-ground, but one
 * correction to the note above, because it points the next lane at the wrong
 * lever.  The note says "the two-way unsigned-char tie-break lever does not
 * apply because the competitor here is a pointer, not a second char" -- that
 * is wrong on its own evidence: the note's own analysis says X and Y are BOTH
 * `unsigned char` locals and exactly one of them gets ebx while the other is
 * spilled, which IS the recorded two-way tie-break ("An `unsigned char` local
 * loses a two-way callee-saved tie-break to another `unsigned char` even with
 * strictly more, loop-nested references; retyping the winner `int` flips
 * it").  The 6 x 6 type cross-product recorded above did measure every type
 * pairing, so the lever has been exercised even though it was mis-attributed
 * -- but the framing matters for what to try next: the question is not "lower
 * a char's rank against a pointer", it is "what separates two `unsigned char`
 * locals in VC6's ranking", which is an OPEN question with a worked example
 * elsewhere in the corpus and is worth attacking as a general rule rather
 * than on this function.
 */
// WIP-FUNCTION: LEGOLAND 0x00410180  (111/111 insns, 344/341B, 45 by audit, 18 structural; the coupled x-store/kind-store swap the volatile shim forces)
void LFDrop_Place(LFPiece* parent)
{
    int           cellh;
    RideDef*      def;
    unsigned char x;
    unsigned char y;
    unsigned char px;
    unsigned char py;
    LFPiece*      sub;
    LFPiece*      prev;
    int           n;

    def = g_lfdr_def;
    cellh = g_lf_footprint.v[3] - g_lf_footprint.v[1];
    px = parent->sq.b.x;
    py = parent->sq.b.y;
    x = (unsigned char)((unsigned char)def->footprint.v[0] + px + 2);
    y = (unsigned char)((unsigned char)def->footprint.v[1] + py);

    LF_MAKE_SUB(3, 0)
    LFPiece_AddSub(parent, sub);
    parent->end_a = sub;
    prev = sub;

    for (n = 24 / cellh + (-2); n > 0; n--) {
        y += (unsigned char)cellh;
        LF_MAKE_SUB(1, 0)
        LFPiece_AddSub(parent, sub);
        LFPiece_LinkAfter(prev, sub);
        prev = sub;
    }
    y += (unsigned char)cellh;
    LF_MAKE_SUB(3, 2)
    LFPiece_AddSub(parent, sub);
    LFPiece_LinkAfter(prev, sub);
    parent->end_b = sub;
}

/* =========================================================================
 * THE SHARED TICK  (+0x8c) AND THE SCREEN POSITION OF A PIECE
 * ========================================================================= */

typedef struct EditCursorRec EditCursorRec;

extern EditCursorRec g_edit_cursor;     /* 0x007febc0 */
extern unsigned int  g_ui_flags;        /* 0x008003e8 */
extern int           g_cursor_mode;     /* 0x008119b0 */
extern RideDef*      g_cursor_def;      /* 0x008119b8 */
extern int           g_lf_tool_a;       /* 0x004cbdd8 */
extern int           g_lf_tool_b;       /* 0x004c2a88 */
extern int           g_lf_tool_c;       /* 0x004c5c90 */
extern int           g_lf_tool_d;       /* 0x004c74c8 */

extern void DefaultCursor(EditCursorRec* c);                     /* 0x0045a390 */
extern void BuildCursorPtr(EditCursorRec* c, int id, int flag);  /* 0x0045f5f0 */
extern void SetEditCursorFootPrint(Footprint* fp);               /* 0x0045f440 */

/* Arm the edit cursor for one set-piece class: point it at the class's
 * ObjDef, give it the class footprint and stamp the same tool code 0x2034
 * into all four log-flume tool slots. */
// FUNCTION: LEGOLAND 0x0040d3b0
void LFPiece_TickCommon(RideDef* def, Footprint* fp)
{
    g_cursor_mode = 1;
    g_cursor_def = def;
    DefaultCursor(&g_edit_cursor);
    g_ui_flags |= 8;
    BuildCursorPtr(&g_edit_cursor, 0x8f8, 0);
    SetEditCursorFootPrint(fp);
    g_lf_tool_a = g_lf_tool_b = g_lf_tool_c = g_lf_tool_d = 0x2034;
}

typedef struct LFOffsetTables {
    unsigned char pad00[0xc];
    int*          dx;           /* +0x0c  half-pixel x offset per shape */
    int*          dy;           /* +0x10 */
} LFOffsetTables;

extern RideDef*        g_lfen_def;      /* 0x004c2b9c  LOG FLUME ENTRANCE */
extern LFOffsetTables* g_lf_offsets;    /* 0x004c2b68 */

extern Pos  GetScreenCoordsForObject(const BPosW* sq, RideDef* def); /* 0x00442cc0 */
extern void AdjustOffsetForViewMode(Pos* off);                       /* 0x00442d30 */

/* Where a piece draws.  A piece belonging to the ENTRANCE class is drawn at
 * its RUN's square (the station), a plain TRACK square gets the per-shape
 * half-pixel nudge from the offset tables (and has its class draw
 * parameters cleared first), and anything else is drawn at its own square.
 * The result is an 8-byte Pos returned in eax:edx. */
// FUNCTION: LEGOLAND 0x0040cfd0
Pos LFPiece_ScreenPos(LFPiece* p)
{
    RideDef* def = p->def;
    Pos      base;
    Pos      off;
    int      idx;

    if (def == g_lfen_def)
        return GetScreenCoordsForObject(&p->run->sq, g_lfen_def);
    if (def == g_lftr_def) {
        def->f18 = 0;
        p->def->f14 = 0;
        base = GetScreenCoordsForObject(&p->sq, p->def);
        idx = LFPiece_ShapeIndex(p);
        idx = (idx & 0xff) * 4;
        off.x = *(int*)((char*)g_lf_offsets->dx + idx) >> 1;
        off.y = *(int*)((char*)g_lf_offsets->dy + idx) >> 1;
        AdjustOffsetForViewMode(&off);
        base.x += off.x;
        base.y += off.y;
        return base;
    }
    return GetScreenCoordsForObject(&p->sq, def);
}

/* =========================================================================
 * HIT-TESTING A MAP POINT AGAINST THE PLACED FLUME
 * ========================================================================= */

/* Hand back the footprint rectangle a piece occupies and the square it is
 * anchored at.  A sub-piece of a set piece (+0x28 == -1) reports its
 * PARENT's square, and the rectangle is the class footprint of whichever
 * log-flume class placed it. */
extern void LFPiece_QueryRect(LFPiece* p, Footprint** out_fp,
                              BPos* out_sq);                     /* 0x0040d090 */
extern void DBPrintf(const char* fmt);                           /* 0x00453a20 */
extern char g_lf_norect_msg[];                                   /* 0x004b4a24 */

/* Which placed piece covers map point (x,y)?  Walks every run and every
 * piece and tests the point against the piece's own footprint rectangle
 * offset by its square.  A piece whose rectangle cannot be resolved is
 * reported to the debug log and skipped. */
/* Closed (71/71) by the 'head read at declaration + a second read in the
 * guard' lever: 'run = g_lf_queue; if (g_lf_queue) { while (run) ... }'.
 * VC6 CSEs the two global reads into eax, tests THAT, and gives the walker
 * its own copy ('mov ecx,eax') which is what gets spilled.  A do/while
 * inside the guard adds an instruction; testing 'run' itself coalesces the
 * copy away. */
// FUNCTION: LEGOLAND 0x0040d210
LFPiece* LFTrack_FindPieceAt(int x, int y)
{
    LFRun*     run = g_lf_queue;
    LFPiece*   p;
    Footprint* fp;
    BPos       sq;

    if (g_lf_queue) {
        while (run) {
            p = run->pieces;
            while (p) {
                LFPiece_QueryRect(p, &fp, &sq);
                if (fp) {
                    int bx = sq.x;
                    int by = sq.y;
                    if (x >= fp->v[0] + bx && x <= fp->v[2] + bx &&
                        y >= fp->v[1] + by && y <= fp->v[3] + by)
                        return p;
                } else {
                    DBPrintf(g_lf_norect_msg);
                }
                p = p->next;
            }
            run = run->next;
        }
    }
    return 0;
}

/* =========================================================================
 * WHICH RECTANGLE DOES A PIECE OCCUPY?
 *
 * Every placed piece carries a back-pointer at +0x28: 0 or -1 for a piece
 * that stands on its own, otherwise the compound piece it belongs to.  The
 * rectangle reported is:
 *
 *   +0x28 == -1        the STATION: the square is the run's square and the
 *                      rectangle is the ENTRANCE class footprint with its
 *                      right edge pushed out by two flume cells (the two
 *                      channel columns the station feeds).
 *   TRACK class        the flume cell rect shrunk by one in each axis (the
 *                      same 3/1 adjustment the cursor code makes).
 *   any other class    that class's own footprint, straight out of its
 *                      ObjDef -- checked in a fixed order: DROP, HOLD UP,
 *                      SPECIAL CORNER 1..4, TUNNEL, CSAW.
 *   unknown class      no rectangle at all (the caller logs it).
 *
 * Both computed rectangles live in module-scope scratch (0x004c2aa8 and
 * 0x004c8d38), so two callers cannot hold rectangles at once.
 * ========================================================================= */

extern RideDef*  g_lfc1_def;            /* 0x004c445c  SPECIAL CORNER 1 */
extern RideDef*  g_lfc2_def;            /* 0x004c2aa0  SPECIAL CORNER 2 */
extern RideDef*  g_lfc3_def;            /* 0x004c2b0c  SPECIAL CORNER 3 */
extern RideDef*  g_lfc4_def;            /* 0x004c74d4  SPECIAL CORNER 4 */
extern Footprint g_lfen_rect;           /* 0x004c2aa8  station scratch rect */
extern Footprint g_lftr_rect;           /* 0x004c8d38  track scratch rect */

// FUNCTION: LEGOLAND 0x0040d090
void LFPiece_QueryRect(LFPiece* p, Footprint** out_fp, BPos* out_sq)
{
    LFPiece* owner = (LFPiece*)p->f28;
    RideDef* def;

    if (owner == 0 || owner == (LFPiece*)-1)
        owner = p;
    out_sq->x = owner->sq.b.x;
    out_sq->y = owner->sq.b.y;
    if (owner->f28 == -1) {
        out_sq->x = owner->run->sq.b.x;
        out_sq->y = owner->run->sq.b.y;
        g_lfen_rect = g_lfen_def->footprint;
        g_lfen_rect.v[2] = g_lfen_rect.v[0] +
                           (g_lf_footprint.v[2] - g_lf_footprint.v[0]) * 2;
        *out_fp = &g_lfen_rect;
        return;
    }
    def = owner->def;
    if (def == g_lftr_def) {
        g_lftr_rect = g_lf_footprint;
        g_lftr_rect.v[2] = g_lf_footprint.v[2] - 1;
        g_lftr_rect.v[3] = g_lftr_rect.v[3] - 1;
        *out_fp = &g_lftr_rect;
        return;
    }
    if (def == g_lfdr_def) {
        *out_fp = &g_lfdr_def->footprint;
        return;
    }
    if (def == g_lfhu_def) {
        *out_fp = &g_lfhu_def->footprint;
        return;
    }
    if (def == g_lfc1_def) {
        *out_fp = &g_lfc1_def->footprint;
        return;
    }
    if (def == g_lfc2_def) {
        *out_fp = &g_lfc2_def->footprint;
        return;
    }
    if (def == g_lfc3_def) {
        *out_fp = &g_lfc3_def->footprint;
        return;
    }
    if (def == g_lfc4_def) {
        *out_fp = &g_lfc4_def->footprint;
        return;
    }
    if (def == g_lftu_def) {
        *out_fp = &g_lftu_def->footprint;
        return;
    }
    if (def == g_lfcs_def) {
        *out_fp = &g_lfcs_def->footprint;
        return;
    }
    *out_fp = 0;
}

/* =========================================================================
 * THE +0x94 "this square changed" SLOT
 * ========================================================================= */

struct EditCursorRec {
    unsigned char pad0000[0x1404];
    int           x;                    /* +0x1404 */
    int           y;                    /* +0x1408 */
    unsigned char pad140c[8];
    Footprint     footprint;            /* +0x1414 */
    unsigned char pad1428[0x1828 - 0x1428];
    int           f1828;                /* +0x1828 */
};                                      /* 0x1834 */

extern EditCursorRec g_ghost_cursor;    /* 0x00810160  the preview cursor */

extern void SetCursorError(EditCursorRec* c, int code);          /* 0x0045f480 */
extern void ResetCursorFootprint(EditCursorRec* c);              /* 0x0045f460 */
extern int  LFPiece_HasCursor(LFPiece* p);                       /* 0x0040c2e0 */

/* Paint the PREVIEW cursor over whatever flume square the map point falls
 * in: the piece's square and rectangle, error code 1, tool 8.  The cursor's
 * footprint is then RESET (i.e. the preview is withdrawn) unless the piece
 * is flagged, already owns a cursor, or -- when both of its ends are fully
 * linked -- sits on a run that is not yet complete. */
// FUNCTION: LEGOLAND 0x0040d2d0
void LFPiece_RefreshAt(const Pos* pos)
{
    LFPiece*   p = LFTrack_FindPieceAt(pos->x, pos->y);
    Footprint* fp;
    BPos       sq;

    if (p) {
        g_ghost_cursor.x = p->sq.b.x;
        g_ghost_cursor.y = p->sq.b.y;
        LFPiece_QueryRect(p, &fp, &sq);
        g_ghost_cursor.footprint = *fp;
        g_ghost_cursor.f1828 = 8;
        SetCursorError(&g_ghost_cursor, 1);
        if (LFPiece_CursorFits(p->sub)) {
            if (p->end_a->back && p->end_a->fwd &&
                p->end_b->back && p->end_b->fwd) {
                if (!LFRun_IsComplete(p->run))
                    return;
            }
            if (!(p->flags & 1) && !LFPiece_HasCursor(p))
                ResetCursorFootprint(&g_ghost_cursor);
        }
    }
}

/* =========================================================================
 * THE SPECIAL CORNER GEOM
 *
 * One body for all four corner classes, switched on g_lf_corner_index.  The
 * direction masks are the four elbows -- 3 = N|E, 6 = E|S, 0xc = S|W,
 * 9 = W|N -- and each case fills the two matching connection points.  The
 * point on the far side of the piece is pushed out by (class span - flume
 * cell span) in that axis, exactly as the straight pieces do.
 * ========================================================================= */

/* Closed (142/142) by ONE statement swap in SPECIAL CORNER 2's arm: the
 * source stores E.y (y+2) BEFORE S.x (x+2).  VC6 had already hoisted the
 * x+2 add, so y+2 is born while x+2, y and out are all live and lands in
 * esi; the two adjacent stores are then swapped back by the peephole.  The
 * register allocation is function-wide, so that one change also put
 * SPECIAL CORNER 3's byte updates into the four-register parallel form --
 * every arm-local spelling of corner 3 (dirs position, statement order,
 * direct byte reads, byte-update order) had left it sequential. */
// FUNCTION: LEGOLAND 0x0040e440
void LFCorner_Geom(BPos sq, LFGeom* out)
{
    int cellw = g_lf_footprint.v[2] - g_lf_footprint.v[0];
    int cellh = g_lf_footprint.v[3] - g_lf_footprint.v[1];
    int x;
    int y;

    switch (g_lf_corner_index) {
    case 0:
        sq.x = (unsigned char)(sq.x + (unsigned char)g_lfc1_def->footprint.v[0]);
        sq.y = (unsigned char)(sq.y + (unsigned char)g_lfc1_def->footprint.v[1]);
        out->dirs = 3;
        x = sq.x;
        out->pt[0].x = x + 3;
        y = sq.y;
        out->pt[0].y = y;
        out->pt[1].x = (g_lfc1_def->footprint.v[2] - g_lfc1_def->footprint.v[0])
                       - cellw + x + 1;
        out->pt[1].y = y + 4;
        break;
    case 1:
        sq.x = (unsigned char)(sq.x + (unsigned char)g_lfc2_def->footprint.v[0]);
        sq.y = (unsigned char)(sq.y + (unsigned char)g_lfc2_def->footprint.v[1]);
        out->dirs = 6;
        x = sq.x;
        out->pt[1].x = (g_lfc2_def->footprint.v[2] - g_lfc2_def->footprint.v[0])
                       - cellw + x + 1;
        y = sq.y;
        out->pt[1].y = y + 2;
        out->pt[2].x = x + 2;
        out->pt[2].y = (g_lfc2_def->footprint.v[3] - g_lfc2_def->footprint.v[1])
                       - cellh + y + 1;
        break;
    case 2:
        sq.x = (unsigned char)(sq.x + (unsigned char)g_lfc3_def->footprint.v[0]);
        sq.y = (unsigned char)(sq.y + (unsigned char)g_lfc3_def->footprint.v[1]);
        out->dirs = 0xc;
        x = sq.x;
        out->pt[2].x = x + 4;
        y = sq.y;
        out->pt[2].y = (g_lfc3_def->footprint.v[3] - g_lfc3_def->footprint.v[1])
                       - cellh + y + 1;
        out->pt[3].x = x;
        out->pt[3].y = y + 3;
        break;
    case 3:
        sq.x = (unsigned char)(sq.x + (unsigned char)g_lfc4_def->footprint.v[0]);
        sq.y = (unsigned char)(sq.y + (unsigned char)g_lfc4_def->footprint.v[1]);
        out->dirs = 9;
        x = sq.x;
        y = sq.y;
        out->pt[3].x = x;
        out->pt[3].y = y + 4;
        out->pt[0].x = x + 4;
        out->pt[0].y = y;
        break;
    }
}

/* =========================================================================
 * SPLICING A NEW SQUARE INTO THE ROUTE  --  LFPiece_SetShape
 *
 * This is the heart of the track-building system.  After LFTrack_Add has
 * put a piece on the map it calls SetShape with the four-slot neighbour
 * array, and SetShape decides how the new square joins the existing route.
 * It switches on the SAME ten legal neighbour masks LFTrack_MaskIsLegal
 * accepts (VC6 compiles them as a 0x50-entry byte index table at 0x00409fb8
 * feeding an eleven-entry jump table at 0x00409f8c):
 *
 *   ONE neighbour (1, 4, 0x10, 0x40) -- extend that neighbour's route:
 *       append if it already has a back link, prepend if it does not.
 *
 *   TWO neighbours -- the straights (0x11 N|S, 0x44 E|W) and the four
 *       elbows (5 N|E, 0x14 E|S, 0x50 S|W, 0x41 N|W).  If the two
 *       neighbours are two ENDS OF THE SAME KIND (both have a forward link
 *       and no back link, or both a back link and no forward link) the two
 *       routes have to be turned to face each other first, which is
 *       LFRoute_Join's job; otherwise the new piece is simply spliced
 *       between them, in whichever order leaves the chain consistent.
 *
 * ORIGINAL BUG, reproduced: the N|W arm (mask 0x41) tests the SAME
 * condition twice.  Every other two-neighbour arm's second test is the
 * mirror of the first (`!fwd && !fwd && back && back`); 0x41's is a
 * verbatim copy of the first, so an N|W corner laid between two route ENDS
 * that both have a back link is spliced instead of joined.
 * ========================================================================= */

/* Insert `piece` between two pieces already on a route. */
// FUNCTION: LEGOLAND 0x004090c0
void LFPiece_SpliceBetween(LFPiece* before, LFPiece* after, LFPiece* piece)
{
    piece->back  = before;
    piece->fwd   = after;
    before->fwd  = piece;
    after->back  = piece;
}

/* Join the routes of nb[i] and nb[j] with `piece` in the middle, reversing
 * whichever of the two runs the wrong way first.  Both calls to the debug
 * hook are in the original, and so is the DEGENERATE branch: when nb[i] is
 * not on the run's route the code reverses nb[i] either way, in two
 * separate blocks. */
extern char g_lf_join_msg[];                                     /* 0x004b4910 */
extern char g_lf_both_msg[];                                     /* 0x004b48e4 */

/* Closed (72/72) by the block ORDER of the three reverse arms: the original
 * lays out 'both free' first, then 'reverse nb[j]', then 'reverse nb[i]'
 * falling through -- which is a leading '&&' guard (VC6 jump-threads its
 * two failure edges straight into the later tests) followed by a three-arm
 * chain whose FIRST arm is the '!oi && !oj' one.  The nested
 * 'if (oi) { if (oj) ...; reverse j } else if (!oj) ... else ...' form puts
 * the reverse-nb[j] block right after the bail-out. */
// FUNCTION: LEGOLAND 0x00409b70
void LFRoute_Join(int i, int j, LFPiece** nb, LFPiece* piece)
{
    int oi;
    int oj;

    DebugPrint(g_lf_join_msg);
    oi = LFPiece_CursorFits(nb[i]);
    oj = LFPiece_CursorFits(nb[j]);
    if (oi && oj) {
        DebugPrint(g_lf_both_msg);
        return;
    }
    if (!oi && !oj)
        LFPiece_ReverseRoute(nb[i]);
    else if (oi)
        LFPiece_ReverseRoute(nb[j]);
    else
        LFPiece_ReverseRoute(nb[i]);   /* the degenerate arm, see above */
    if (!nb[i]->fwd && !nb[j]->back)
        LFPiece_SpliceBetween(nb[i], nb[j], piece);
    else
        LFPiece_SpliceBetween(nb[j], nb[i], piece);
}

// FUNCTION: LEGOLAND 0x00409c20
void LFPiece_SetShape(LFPiece* piece, LFPiece** nb)
{
    LFPiece* a;

    switch (LFTrack_NeighbourMask(nb)) {
    case 1:
        a = nb[0];
        if (a->back == 0)
            LFPiece_LinkBefore(a, piece);
        else
            LFPiece_LinkAfter(a, piece);
        break;
    case 0x10:
        a = nb[2];
        if (a->back == 0)
            LFPiece_LinkBefore(a, piece);
        else
            LFPiece_LinkAfter(a, piece);
        break;
    case 4:
        a = nb[1];
        if (a->back == 0)
            LFPiece_LinkBefore(a, piece);
        else
            LFPiece_LinkAfter(a, piece);
        break;
    case 0x40:
        a = nb[3];
        if (a->back == 0)
            LFPiece_LinkBefore(a, piece);
        else
            LFPiece_LinkAfter(a, piece);
        break;
    case 0x11:
        if ((nb[0]->fwd && nb[2]->fwd && !nb[0]->back && !nb[2]->back) ||
            (!nb[0]->fwd && !nb[2]->fwd && nb[0]->back && nb[2]->back))
            LFRoute_Join(0, 2, nb, piece);
        else if (!nb[0]->fwd && !nb[2]->back)
            LFPiece_SpliceBetween(nb[0], nb[2], piece);
        else
            LFPiece_SpliceBetween(nb[2], nb[0], piece);
        break;
    case 0x44:
        if ((nb[1]->fwd && nb[3]->fwd && !nb[1]->back && !nb[3]->back) ||
            (!nb[1]->fwd && !nb[3]->fwd && nb[1]->back && nb[3]->back))
            LFRoute_Join(1, 3, nb, piece);
        else if (!nb[1]->back && !nb[3]->fwd)
            LFPiece_SpliceBetween(nb[3], nb[1], piece);
        else
            LFPiece_SpliceBetween(nb[1], nb[3], piece);
        break;
    case 5:
        if ((nb[0]->fwd && nb[1]->fwd && !nb[0]->back && !nb[1]->back) ||
            (!nb[0]->fwd && !nb[1]->fwd && nb[0]->back && nb[1]->back))
            LFRoute_Join(0, 1, nb, piece);
        else if (!nb[1]->back && !nb[0]->fwd)
            LFPiece_SpliceBetween(nb[0], nb[1], piece);
        else
            LFPiece_SpliceBetween(nb[1], nb[0], piece);
        break;
    case 0x14:
        if ((nb[1]->fwd && nb[2]->fwd && !nb[1]->back && !nb[2]->back) ||
            (!nb[1]->fwd && !nb[2]->fwd && nb[1]->back && nb[2]->back))
            LFRoute_Join(1, 2, nb, piece);
        else if (!nb[2]->back && !nb[1]->fwd)
            LFPiece_SpliceBetween(nb[1], nb[2], piece);
        else
            LFPiece_SpliceBetween(nb[2], nb[1], piece);
        break;
    case 0x50:
        if ((nb[2]->fwd && nb[3]->fwd && !nb[2]->back && !nb[3]->back) ||
            (!nb[2]->fwd && !nb[3]->fwd && nb[2]->back && nb[3]->back))
            LFRoute_Join(2, 3, nb, piece);
        else if (!nb[3]->back && !nb[2]->fwd)
            LFPiece_SpliceBetween(nb[2], nb[3], piece);
        else
            LFPiece_SpliceBetween(nb[3], nb[2], piece);
        break;
    case 0x41:
        /* the copy-paste bug: the second disjunct repeats the first. */
        if ((nb[3]->fwd && nb[0]->fwd && !nb[3]->back && !nb[0]->back) ||
            (nb[3]->fwd && nb[0]->fwd && !nb[3]->back && !nb[0]->back))
            LFRoute_Join(3, 0, nb, piece);
        else if (!nb[0]->back && !nb[3]->fwd)
            LFPiece_SpliceBetween(nb[3], nb[0], piece);
        else
            LFPiece_SpliceBetween(nb[0], nb[3], piece);
        break;
    }
}
