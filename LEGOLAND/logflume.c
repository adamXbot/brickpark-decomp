/* LEGOLAND -- the LOG FLUME ride subsystem.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  None of
 * these functions is exported; every extent was taken from the disassembly by
 * control flow (tools/audit.py).  Struct field OFFSETS, record sizes and
 * global addresses are load-bearing; the names are ours.  Types are declared
 * LOCALLY on purpose (legoland.h is owned elsewhere).
 *
 * =========================================================================
 * TEN CLASSES, ONE ROUTE
 *
 * The log flume is assembled from ten .ODF classes -- see
 * LogFlume_GetInterfaces (0x00410d60, interfaces.c):
 *
 *   LOG FLUME ENTRANCE          the station: the only class with a save
 *                               chunk (+0xb8/+0xbc) and the only user of the
 *                               +0xc0 "extra" slot in the whole game.
 *   LOG FLUME TRACK             the plain channel the player draws.
 *   LOG FLUME SPECIAL CORNER 1..4   the four fixed corner set pieces.
 *   LOG FLUME CSAW / TUNNEL / DROP / HOLD UP   the four scenery set pieces.
 *
 * Every class except the ENTRANCE fills the same nine-slot shape (create,
 * tick, draw, interact, update, update2, add, remove, destroy), and the nine
 * non-ENTRANCE, non-TRACK classes all share ONE draw handler, LFPiece_Draw.
 * The log flume and WATER WORKS are the only subsystems in the game that use
 * the secondary update slots +0x90/+0x94 at all, and +0xb0 here is NOT an
 * interaction hook: like CASTLE OBJ it is the per-square DRAW pass.
 *
 * =========================================================================
 * WHAT A FLUME IS MADE OF -- the three records
 *
 *   LFRun     (0xd4 bytes, exactly the save record).  One connected stretch
 *             of flume together with the station that feeds it.  Every run
 *             is on the global list g_lf_queue (0x004cbe84), keyed by its
 *             map square at +0x14, and owns its pieces through the list head
 *             at +0x10.  +0x1c is the animation frame every piece of the run
 *             plays in lockstep; +0xd0 is how many squares long it is.
 *   LFPiece   (0x38 bytes, zeroed on allocation).  One map square of flume:
 *             its square at +0x14, the class definition that placed it at
 *             +0x20, and a back-pointer to its run at +0x24.
 *   PieceDraw (0x14 bytes, ONE static instance per family).  What a draw
 *             handler fills in and hands to the render list.
 *
 * HOW A ROUTE IS BUILT.  Flume is laid one square at a time.  LFTrack_Add
 * allocates a piece, probes the four neighbouring squares (the probe fills a
 * fixed four-slot array at 0x004cbe20 and hands back its address), discards
 * any neighbour with no free end, and JOINS the run of the first neighbour
 * that has one -- so a chain of squares becomes a single run.  A square with
 * no neighbour at all takes the neighbour ARRAY itself as its run, which is
 * reproduced verbatim below and is either a deliberate empty-run sentinel or
 * an original bug.  LFTrack_Update runs the same probe every frame while the
 * tool is held, to decide whether the square under the mouse is legal.
 *
 * HOW A BOAT MOVES ALONG IT.  A run carries four 0x24-byte boat records from
 * +0x40 on (that is why the save code converts a boat index back as
 * `st + idx*36 + 0x40`), and the run's +0x38 points at the active one.  A
 * boat's flag byte (+0x04 of the record it shares with the run head) says
 * whether it is falling, and the DROP class watches that bit to blit
 * lf_splash.lls at a fixed (-74, +149) offset with the splash .lls FROZEN
 * (LLSStop) on the frame the boat carries -- which is how the splash stays
 * locked to the fall instead of free-running.
 *
 * =========================================================================
 * THE ONE STRUCTURAL SURPRISE: THE SET-PIECE CLASSES ARE PARAMETERISED
 *
 * The four SPECIAL CORNER classes do not have four implementations.  They
 * have four thin shims over ONE implementation, and each shim's whole job is
 * to write its class index (0..3) into g_lf_corner_index (0x004c2af4) before
 * calling the shared helper.  The helpers -- and the geometry callbacks they
 * are handed -- then `switch (g_lf_corner_index)` through a four-entry jump
 * table.  So the "class" is a dynamic parameter: whichever corner shim ran
 * last owns the global.  CSAW, TUNNEL, DROP and HOLD UP use the same trick
 * with their own callback quadruples, and three of them still write
 * g_lf_corner_index = 0 in their add handler -- a copy-paste leftover that
 * silently re-points every corner callback at SPECIAL CORNER 1.
 *
 * The other difference between the two families: a CORNER shim copies
 * def->footprint into a 20-byte local and passes the COPY (so the helper's
 * in-place rewrite cannot clobber the class definition), while a SET PIECE
 * passes &def->footprint straight through.  That is why a corner shim has a
 * 20-byte frame and a set-piece shim has none.
 *
 * =========================================================================
 * THE SHARED PIECE HELPERS  (in the 0x0040d000 block, extern here)
 *
 *   0x0040d3b0  LFPiece_TickCommon(def, footprint)
 *   0x0040d6f0  LFPiece_UpdateCommon(def, a, b, footprint, geom, probe)
 *   0x0040d900  LFPiece_AddCommon(square, footprint, elem, place, geom, shape)
 *   0x0040db00  LFPiece_RemoveCommon(a, b, c, geom)
 *   0x0040d2d0  LFPiece_RefreshAt(&pos)          -- the +0x94 update2 slot
 *
 * =========================================================================
 * WHAT IS IN THIS FILE  (76 exact, 7 with a measured residual)
 *
 *   0x0040e630  LFCorner1_Tick        
 *   0x0040e660  LFCorner2_Tick        
 *   0x0040e690  LFCorner3_Tick        
 *   0x0040e6c0  LFCorner4_Tick        
 *   0x0040e830  LFCorner1_Update2     
 *   0x0040e850  LFCorner2_Update2     
 *   0x0040e870  LFCorner3_Update2     
 *   0x0040e890  LFCorner4_Update2     
 *   0x0040e6f0  LFCorner1_Update      
 *   0x0040e740  LFCorner2_Update      
 *   0x0040e790  LFCorner3_Update      
 *   0x0040e7e0  LFCorner4_Update      
 *   0x0040e8b0  LFCorner1_Create      
 *   0x0040e920  LFCorner2_Create      
 *   0x0040e970  LFCorner3_Create      
 *   0x0040e9e0  LFCorner4_Create      
 *   0x0040ea30  LFCorner1_Destroy     
 *   0x0040ea60  LFCorner2_Destroy     
 *   0x0040ea80  LFCorner3_Destroy     
 *   0x0040eab0  LFCorner4_Destroy     
 *   0x0040ead0  LFCorner1_Add         
 *   0x0040eb40  LFCorner2_Add         
 *   0x0040ebb0  LFCorner3_Add         
 *   0x0040ec20  LFCorner4_Add         
 *   0x0040ec90  LFCorner1_Remove      
 *   0x0040ecc0  LFCorner2_Remove      
 *   0x0040ecf0  LFCorner3_Remove      
 *   0x0040ed20  LFCorner4_Remove      
 *   0x0040ed50  LFPiece_Draw          
 *   0x0040f3e0  LFTunnel_Create       
 *   0x0040f430  LFTunnel_Destroy      
 *   0x0040f4f0  LFTunnel_Tick         
 *   0x0040f510  LFTunnel_Update       
 *   0x0040f5a0  LFTunnel_Update2      
 *   0x0040f540  LFTunnel_Add          
 *   0x0040f580  LFTunnel_Remove       
 *   0x0040f8b0  LFCsaw_Create         
 *   0x0040f900  LFCsaw_Destroy        
 *   0x0040fa00  LFCsaw_Tick           
 *   0x0040fa20  LFCsaw_Update         
 *   0x0040fa50  LFCsaw_Update2        
 *   0x0040fa60  LFCsaw_Add            
 *   0x0040fab0  LFCsaw_Remove         
 *   0x0040ff30  LFHoldUp_Create       
 *   0x0040ffa0  LFHoldUp_Destroy      
 *   0x004100b0  LFHoldUp_Tick         
 *   0x004100d0  LFHoldUp_Update       
 *   0x00410100  LFHoldUp_Update2      
 *   0x00410110  LFHoldUp_Add          
 *   0x00410160  LFHoldUp_Remove       
 *   0x004103e0  LFDrop_Create         
 *   0x00410450  LFDrop_Destroy        
 *   0x004106e0  LFDrop_Tick           
 *   0x00410700  LFDrop_Update         
 *   0x00410730  LFDrop_Update2        
 *   0x00410740  LFDrop_Add            
 *   0x00410790  LFDrop_Remove         
 *   0x0040f450  LFTunnel_Interact     
 *   0x0040f920  LFCsaw_Interact       
 *   0x0040edb0  LFCorner1_Interact    
 *   0x0040ee60  LFCorner2_Interact    
 *   0x0040ef00  LFCorner3_Interact    
 *   0x0040efb0  LFCorner4_Interact    
 *   0x0040ffd0  LFHoldUp_Interact     
 *   0x004104b0  LFDrop_Interact       
 *   0x0040c350  LFTrack_Create        
 *   0x0040c430  LFTrack_Destroy       
 *   0x0040c6c0  LFTrack_Update2       
 *   0x0040c4a0  LFTrack_Update          [residual, see the note above it]
 *   0x0040c780  LFTrack_Add             [residual, see the note above it]
 *   0x0040c8d0  LFTrack_Remove        
 *   0x0040c970  LFTrack_Draw          
 *   0x0040a2e0  LFEntrance_Create     
 *   0x0040a410  LFEntrance_Destroy    
 *   0x0040a540  LFEntrance_Tick       
 *   0x0040a930  LFEntrance_Update       [residual, see the note above it]
 *   0x0040aac0  LFEntrance_Update2      [residual, see the note above it]
 *   0x004119c0  LFEntrance_Extra      
 *   0x00410930  SaveLogFlume            [residual, see the note above it]
 *   0x00410c10  LoadLogFlume          
 *   0x0040cc50  LFTrack_Interact        [residual, see the note above it]
 *   0x0040dbb0  LFTrack_Tick          
 *   0x0040abf0  LFEntrance_Remove       [residual, see the note above it]
 *
 * The three log-flume callbacks NOT reconstructed here are the three
 * largest: LFEntrance_Add (0x0040a600, 256 instructions -- it builds the
 * station's own run: the entrance piece, a column of channel pieces spaced
 * by the footprint height, and the end piece), LFEntrance_Interact
 * (0x0040b420, the station's draw pass) and LFEntrance_Activate
 * (0x0040bf70, 222 instructions -- a twelve-case switch on a visitor's
 * action byte at Bloke +0x60 that walks a rider through queue, board,
 * ride and disembark).
 * ========================================================================= */

/* ---- shared types (same offsets as coaster.c / castleobj.c) ------------- */
typedef struct Pos { int x; int y; } Pos;

/* A packed 2-byte map square, passed BY VALUE as a dword. */
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

typedef struct FootPart FootPart;

/* A class footprint: four ints and the head of its part list. */
typedef struct Footprint {
    int       v[4];             /* +0x00 */
    FootPart* parts;            /* +0x10 */
} Footprint;                    /* 0x14 */

typedef struct SpriteRec {
    unsigned char pad00[8];
    int           f08;          /* +0x08  the .lls play parameter */
    unsigned char pad0c[4];
    unsigned int  flags;        /* +0x10  0x2000 = the class's own sprite */
} SpriteRec;

typedef struct RideDef {
    unsigned char pad00[8];
    int           f08;          /* +0x08  a placement counter */
    unsigned char pad0c[8];
    int           f14;          /* +0x14  draw parameter */
    int           f18;          /* +0x18  draw parameter */
    unsigned int  flags1c;      /* +0x1c  0x400 = flume piece */
    unsigned char pad20[0x1c];
    Footprint     footprint;    /* +0x3c */
    unsigned char pad50[0x14];
    SpriteRec*    sprite;       /* +0x64 */
    unsigned char pad68[0xc4 - 0x68];
    void*         fc4;          /* +0xc4 */
    unsigned char pad_c8[4];
    void*         fcc;          /* +0xcc  the class's animation set */
} RideDef;

typedef struct RideElem {
    char*        name;          /* +0x00 */
    char*        image;         /* +0x04 */
    unsigned int flags;         /* +0x08 */
    RideDef*     data;          /* +0x0c */
} RideElem;

/* The record LFPiece_Draw hands back to the render list: the piece's sprite
 * plus its two draw parameters and the map square it was found at. */
typedef struct PieceDraw {
    SpriteRec*     sprite;      /* +0x00  (0x004cbe58) */
    int            f04;         /* +0x04  = def->f14 */
    int            f08;         /* +0x08  = def->f18 */
    unsigned short sq;          /* +0x0c  (0x004cbe64) packed map square */
    unsigned short pad0e;
    int            f10;         /* +0x10  always cleared */
} PieceDraw;

/* ---- CRT / engine ------------------------------------------------------ */
extern SpriteRec* LoadSprite(const char* name, int mode);        /* 0x00497ab0 */
extern void       KillSprite(SpriteRec* s);                      /* 0x00497bd0 */

/* ---- the shared log-flume piece helpers -------------------------------- */
extern void  LFPiece_TickCommon(RideDef* def, Footprint* fp);    /* 0x0040d3b0 */
extern void  LFPiece_UpdateCommon(RideDef* def, void* a, void* b,
                                  Footprint* fp, void* probe,
                                  void* geom);                   /* 0x0040d6f0 */
extern void  LFPiece_AddCommon(BPos sq, Footprint* fp, RideElem* elem,
                               void* place, void* geom,
                               void* shape);                     /* 0x0040d900 */
extern void  LFPiece_RemoveCommon(void* a, void* b, void* c,
                                  void* geom);                   /* 0x0040db00 */
extern void  LFPiece_RefreshAt(const Pos* pos);                  /* 0x0040d2d0 */
extern struct LFPiece* LFPiece_FindAt(const BPos* sq);           /* 0x00408ef0 */
extern void  LFPiece_MarkDrawn(void* piece, int mode);           /* 0x0040cd70 */

/* The three corner geometry callbacks the shims hand to the helpers.  All
 * three dispatch on g_lf_corner_index themselves. */
extern void  LFCorner_Shape(void);                               /* 0x0040e340 */
extern void  LFCorner_Probe(void);                               /* 0x0040e3b0 */
extern void  LFCorner_Geom(void);                                /* 0x0040e440 */
extern void  LFCorner_Place(void);                               /* 0x0040dc00 */

/* ---- module globals ---------------------------------------------------- */
/* Which SPECIAL CORNER class the shared helpers are currently acting for. */
extern int        g_lf_corner_index;    /* 0x004c2af4 */

extern RideDef*   g_lfc1_def;           /* 0x004c445c */
extern RideElem*  g_lfc1_elem;          /* 0x004c2b98 */
extern SpriteRec* g_lfc1_spr_m1;        /* 0x004c2b6c  "fc1_m1.lls" */
extern SpriteRec* g_lfc1_spr_m2;        /* 0x004c2b70  "fc1_m2.lls" */
extern SpriteRec* g_lfc1_tbl_m1;        /* 0x004b47f8  static render table */
extern SpriteRec* g_lfc1_tbl_m2;        /* 0x004b4804 */

extern RideDef*   g_lfc2_def;           /* 0x004c2aa0 */
extern RideElem*  g_lfc2_elem;          /* 0x004cbe10 */
extern SpriteRec* g_lfc2_spr_m1;        /* 0x004c8d2c  "fc2_m1.lls" */

extern RideDef*   g_lfc3_def;           /* 0x004c2b0c */
extern RideElem*  g_lfc3_elem;          /* 0x004c8d50 */
extern SpriteRec* g_lfc3_spr_m1;        /* 0x004cbe0c  "fc3_m1.lls" */
extern SpriteRec* g_lfc3_spr_m2;        /* 0x004cbe08  "fc3_m2.lls" */
extern SpriteRec* g_lfc3_tbl_m1;        /* 0x004b4818  static render table */
extern SpriteRec* g_lfc3_tbl_m2;        /* 0x004b4824 */

extern RideDef*   g_lfc4_def;           /* 0x004c74d4 */
extern RideElem*  g_lfc4_elem;          /* 0x004c2ba0 */
extern SpriteRec* g_lfc4_spr_m;         /* 0x004c8d70  "fc4_m.lls" */

extern PieceDraw  g_piece_draw;         /* 0x004cbe58 */

/* =========================================================================
 * SPECIAL CORNER 1..4 -- the tick (+0x8c) shims.
 *
 * Copy the class footprint out of the ObjDef and run the shared tick over
 * the copy.  The element arguments are ignored: the class is reached through
 * its captured ObjDef global instead.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0040e630
void LFCorner1_Tick(void)
{
    Footprint fp;
    RideDef*  def = g_lfc1_def;

    fp = def->footprint;
    LFPiece_TickCommon(def, &fp);
}

// FUNCTION: LEGOLAND 0x0040e660
void LFCorner2_Tick(void)
{
    Footprint fp;
    RideDef*  def = g_lfc2_def;

    fp = def->footprint;
    LFPiece_TickCommon(def, &fp);
}

// FUNCTION: LEGOLAND 0x0040e690
void LFCorner3_Tick(void)
{
    Footprint fp;
    RideDef*  def = g_lfc3_def;

    fp = def->footprint;
    LFPiece_TickCommon(def, &fp);
}

// FUNCTION: LEGOLAND 0x0040e6c0
void LFCorner4_Tick(void)
{
    Footprint fp;
    RideDef*  def = g_lfc4_def;

    fp = def->footprint;
    LFPiece_TickCommon(def, &fp);
}

/* =========================================================================
 * SPECIAL CORNER 1..4 -- the update2 (+0x94) shims.
 *
 * The +0x94 slot is "this square changed, repaint it": it is handed the map
 * position as two INTS and forwards it to the shared refresh.  The corner
 * index has to be published first because the refresh reaches back into the
 * geometry callbacks that switch on it.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0040e830
void LFCorner1_Update2(RideElem* elem, const Pos* pos)
{
    g_lf_corner_index = 0;
    LFPiece_RefreshAt(pos);
}

// FUNCTION: LEGOLAND 0x0040e850
void LFCorner2_Update2(RideElem* elem, const Pos* pos)
{
    g_lf_corner_index = 1;
    LFPiece_RefreshAt(pos);
}

// FUNCTION: LEGOLAND 0x0040e870
void LFCorner3_Update2(RideElem* elem, const Pos* pos)
{
    g_lf_corner_index = 2;
    LFPiece_RefreshAt(pos);
}

// FUNCTION: LEGOLAND 0x0040e890
void LFCorner4_Update2(RideElem* elem, const Pos* pos)
{
    g_lf_corner_index = 3;
    LFPiece_RefreshAt(pos);
}

/* =========================================================================
 * SPECIAL CORNER 1..4 -- the update (+0x90) shims.
 *
 * Same footprint copy as the tick, plus the two opaque cursor arguments the
 * caller supplies and the pair of geometry callbacks.  The FIRST argument
 * (the class element) is dead in all four.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0040e6f0
void LFCorner1_Update(RideElem* elem, void* a, void* b)
{
    Footprint fp;
    RideDef*  def = g_lfc1_def;

    g_lf_corner_index = 0;
    fp = def->footprint;
    LFPiece_UpdateCommon(def, a, b, &fp, LFCorner_Geom, LFCorner_Probe);
}

// FUNCTION: LEGOLAND 0x0040e740
void LFCorner2_Update(RideElem* elem, void* a, void* b)
{
    Footprint fp;
    RideDef*  def = g_lfc2_def;

    g_lf_corner_index = 1;
    fp = def->footprint;
    LFPiece_UpdateCommon(def, a, b, &fp, LFCorner_Geom, LFCorner_Probe);
}

// FUNCTION: LEGOLAND 0x0040e790
void LFCorner3_Update(RideElem* elem, void* a, void* b)
{
    Footprint fp;
    RideDef*  def = g_lfc3_def;

    g_lf_corner_index = 2;
    fp = def->footprint;
    LFPiece_UpdateCommon(def, a, b, &fp, LFCorner_Geom, LFCorner_Probe);
}

// FUNCTION: LEGOLAND 0x0040e7e0
void LFCorner4_Update(RideElem* elem, void* a, void* b)
{
    Footprint fp;
    RideDef*  def = g_lfc4_def;

    g_lf_corner_index = 3;
    fp = def->footprint;
    LFPiece_UpdateCommon(def, a, b, &fp, LFCorner_Geom, LFCorner_Probe);
}

/* =========================================================================
 * SPECIAL CORNER 1..4 -- create (+0xa4) and destroy (+0xac).
 *
 * Create captures the class ObjDef and element in the shim's own globals,
 * marks the definition as a flume piece (flags1c |= 0x400), claims the
 * class sprite for the class (sprite->flags |= 0x2000, the "do not share
 * this sprite" bit) and loads the corner's own overlay sprites.  Corners 1
 * and 3 have two overlays each and also publish them into the two static
 * render tables; corners 2 and 4 have one and publish nothing.
 *
 * Note the reload: the flags1c update goes through the LOCAL def, but the
 * sprite claim re-reads the global -- and null-checks it, which the flags1c
 * update did not.  Reproduced as written.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0040e8b0
void LFCorner1_Create(RideElem* elem)
{
    RideDef* def = elem->data;

    g_lfc1_def = def;
    def->flags1c |= 0x400;
    if (g_lfc1_def && g_lfc1_def->sprite)
        g_lfc1_def->sprite->flags |= 0x2000;
    g_lfc1_elem = elem;
    g_lfc1_spr_m1 = LoadSprite("fc1_m1.lls", 1);
    g_lfc1_spr_m2 = LoadSprite("fc1_m2.lls", 1);
    g_lfc1_tbl_m1 = g_lfc1_spr_m1;
    g_lfc1_tbl_m2 = g_lfc1_spr_m2;
}

// FUNCTION: LEGOLAND 0x0040e920
void LFCorner2_Create(RideElem* elem)
{
    RideDef* def = elem->data;

    g_lfc2_def = def;
    def->flags1c |= 0x400;
    if (g_lfc2_def && g_lfc2_def->sprite)
        g_lfc2_def->sprite->flags |= 0x2000;
    g_lfc2_elem = elem;
    g_lfc2_spr_m1 = LoadSprite("fc2_m1.lls", 1);
}

// FUNCTION: LEGOLAND 0x0040e970
void LFCorner3_Create(RideElem* elem)
{
    RideDef* def = elem->data;

    g_lfc3_def = def;
    def->flags1c |= 0x400;
    if (g_lfc3_def && g_lfc3_def->sprite)
        g_lfc3_def->sprite->flags |= 0x2000;
    g_lfc3_elem = elem;
    g_lfc3_spr_m1 = LoadSprite("fc3_m1.lls", 1);
    g_lfc3_spr_m2 = LoadSprite("fc3_m2.lls", 1);
    g_lfc3_tbl_m1 = g_lfc3_spr_m1;
    g_lfc3_tbl_m2 = g_lfc3_spr_m2;
}

// FUNCTION: LEGOLAND 0x0040e9e0
void LFCorner4_Create(RideElem* elem)
{
    RideDef* def = elem->data;

    g_lfc4_def = def;
    def->flags1c |= 0x400;
    g_lfc4_elem = elem;
    if (g_lfc4_def && g_lfc4_def->sprite)
        g_lfc4_def->sprite->flags |= 0x2000;
    g_lfc4_spr_m = LoadSprite("fc4_m.lls", 1);
}

// FUNCTION: LEGOLAND 0x0040ea30
void LFCorner1_Destroy(void)
{
    if (g_lfc1_spr_m1)
        KillSprite(g_lfc1_spr_m1);
    if (g_lfc1_spr_m2)
        KillSprite(g_lfc1_spr_m2);
}

// FUNCTION: LEGOLAND 0x0040ea60
void LFCorner2_Destroy(void)
{
    if (g_lfc2_spr_m1)
        KillSprite(g_lfc2_spr_m1);
}

// FUNCTION: LEGOLAND 0x0040ea80
void LFCorner3_Destroy(void)
{
    if (g_lfc3_spr_m1)
        KillSprite(g_lfc3_spr_m1);
    if (g_lfc3_spr_m2)
        KillSprite(g_lfc3_spr_m2);
}

// FUNCTION: LEGOLAND 0x0040eab0
void LFCorner4_Destroy(void)
{
    if (g_lfc4_spr_m)
        KillSprite(g_lfc4_spr_m);
}

/* =========================================================================
 * SPECIAL CORNER 1..4 -- add (+0x98) and remove (+0x9c).
 *
 * The add slot is called with the map position as two INTS; the shared
 * placer wants the PACKED two-byte square, so each shim narrows the pair
 * into a BPos and passes it by value.  VC6 homes that BPos in the (now
 * dead) second argument slot rather than in the frame, which is why the
 * frame is exactly the 20 bytes of the footprint copy.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0040ead0
void LFCorner1_Add(RideElem* elem, const Pos* pos)
{
    Footprint fp;
    BPos      sq;
    RideDef*  def;

    sq.x = (unsigned char)pos->x;
    def  = g_lfc1_def;
    sq.y = (unsigned char)pos->y;
    fp   = def->footprint;
    g_lf_corner_index = 0;
    LFPiece_AddCommon(sq, &fp, g_lfc1_elem,
                      LFCorner_Place, LFCorner_Geom, LFCorner_Shape);
}

// FUNCTION: LEGOLAND 0x0040eb40
void LFCorner2_Add(RideElem* elem, const Pos* pos)
{
    Footprint fp;
    BPos      sq;
    RideDef*  def;

    sq.x = (unsigned char)pos->x;
    def  = g_lfc2_def;
    sq.y = (unsigned char)pos->y;
    fp   = def->footprint;
    g_lf_corner_index = 1;
    LFPiece_AddCommon(sq, &fp, g_lfc2_elem,
                      LFCorner_Place, LFCorner_Geom, LFCorner_Shape);
}

// FUNCTION: LEGOLAND 0x0040ebb0
void LFCorner3_Add(RideElem* elem, const Pos* pos)
{
    Footprint fp;
    BPos      sq;
    RideDef*  def;

    sq.x = (unsigned char)pos->x;
    def  = g_lfc3_def;
    sq.y = (unsigned char)pos->y;
    fp   = def->footprint;
    g_lf_corner_index = 2;
    LFPiece_AddCommon(sq, &fp, g_lfc3_elem,
                      LFCorner_Place, LFCorner_Geom, LFCorner_Shape);
}

// FUNCTION: LEGOLAND 0x0040ec20
void LFCorner4_Add(RideElem* elem, const Pos* pos)
{
    Footprint fp;
    BPos      sq;
    RideDef*  def;

    sq.x = (unsigned char)pos->x;
    def  = g_lfc4_def;
    sq.y = (unsigned char)pos->y;
    fp   = def->footprint;
    g_lf_corner_index = 3;
    LFPiece_AddCommon(sq, &fp, g_lfc4_elem,
                      LFCorner_Place, LFCorner_Geom, LFCorner_Shape);
}

// FUNCTION: LEGOLAND 0x0040ec90
void LFCorner1_Remove(void* a, void* b, void* c)
{
    g_lf_corner_index = 0;
    LFPiece_RemoveCommon(a, b, c, LFCorner_Geom);
}

// FUNCTION: LEGOLAND 0x0040ecc0
void LFCorner2_Remove(void* a, void* b, void* c)
{
    g_lf_corner_index = 1;
    LFPiece_RemoveCommon(a, b, c, LFCorner_Geom);
}

// FUNCTION: LEGOLAND 0x0040ecf0
void LFCorner3_Remove(void* a, void* b, void* c)
{
    g_lf_corner_index = 2;
    LFPiece_RemoveCommon(a, b, c, LFCorner_Geom);
}

// FUNCTION: LEGOLAND 0x0040ed20
void LFCorner4_Remove(void* a, void* b, void* c)
{
    g_lf_corner_index = 3;
    LFPiece_RemoveCommon(a, b, c, LFCorner_Geom);
}

/* =========================================================================
 * LFPiece_Draw (+0xa0) -- the ONE draw handler shared by all eight
 * non-ENTRANCE, non-TRACK log-flume classes (both corner sets, CSAW,
 * TUNNEL, DROP and HOLD UP).
 *
 * It is handed the class element and the packed map square by value, looks
 * the placed piece up in the flume's piece list, flags it as drawn this
 * frame, and fills ONE static PieceDraw record from the class definition:
 * the class sprite plus the definition's two draw parameters (+0x14/+0x18)
 * and the square itself.
 *
 * ORIGINAL QUIRK, reproduced: the record is static and is returned
 * UNCONDITIONALLY.  When no piece is placed on the square the caller gets
 * the record left over from the previous successful call rather than a null.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0040ed50
PieceDraw* LFPiece_Draw(RideElem* elem, BPosW sq)
{
    RideDef* def = elem->data;
    void*    piece = LFPiece_FindAt(&sq.b);

    if (piece) {
        LFPiece_MarkDrawn(piece, 0);
        g_piece_draw.sq     = sq.w;
        g_piece_draw.sprite = def->sprite;
        g_piece_draw.f04    = def->f14;
        g_piece_draw.f08    = def->f18;
        g_piece_draw.f10    = 0;
    }
    return &g_piece_draw;
}

/* =========================================================================
 * THE FOUR SET-PIECE CLASSES: CSAW, TUNNEL, DROP and HOLD UP.
 *
 * Structurally these are the SPECIAL CORNER shims again, with two
 * differences that show up in every one of them:
 *
 *  - they do NOT copy the footprint.  A corner shim copies def->footprint
 *    into a 20-byte local and passes the copy; a set piece passes
 *    &def->footprint straight through, so the shared helper writes back
 *    into the class definition.  (That is why a corner's frame is 20 bytes
 *    and a set piece's is zero.)
 *  - they carry their own geometry/probe/shape/place callback quadruple
 *    instead of the index-switched corner ones.
 *
 * ORIGINAL QUIRK, reproduced: the CSAW, HOLD UP and DROP add handlers still
 * write g_lf_corner_index = 0 even though they are not corner classes -- a
 * copy-paste leftover from the corner shims.  TUNNEL's does not.  The effect
 * is that placing one of those three pieces silently re-points every corner
 * callback at SPECIAL CORNER 1 until the next corner shim runs.
 * ========================================================================= */

extern RideDef*   g_lftu_def;           /* 0x004cbe18  LOG FLUME TUNNEL */
extern RideElem*  g_lftu_elem;          /* 0x004cbe48 */
extern SpriteRec* g_lftu_spr;           /* 0x004c1258  "tunel_m.lls" */
extern void  LFTunnel_Shape(void);      /* 0x0040f300 */
extern void  LFTunnel_Probe(void);      /* 0x0040f330 */
extern void  LFTunnel_Geom(void);       /* 0x0040f360 */
extern void  LFTunnel_Place(void);      /* 0x0040f050 */

extern RideDef*   g_lfcs_def;           /* 0x004c2bf0  LOG FLUME CSAW */
extern RideElem*  g_lfcs_elem;          /* 0x004c4460 */
extern SpriteRec* g_lfcs_spr;           /* 0x004cbe14  "csaw2_m.lls" */
extern void  LFCsaw_Shape(void);        /* 0x0040f7d0 */
extern void  LFCsaw_Probe(void);        /* 0x0040f800 */
extern void  LFCsaw_Geom(void);         /* 0x0040f830 */
extern void  LFCsaw_Place(void);        /* 0x0040f5b0 */

extern RideDef*   g_lfhu_def;           /* 0x004c2b60  LOG FLUME HOLD UP */
extern RideElem*  g_lfhu_elem;          /* 0x004c2b50 */
extern SpriteRec* g_lfhu_spr_m1;        /* 0x004c2a94  "hup1_m1.lls" */
extern SpriteRec* g_lfhu_spr_m2;        /* 0x004c2a98  "hup1_m2.lls" */
extern SpriteRec* g_lfhu_tbl_m1;        /* 0x004b4838  static render table */
extern SpriteRec* g_lfhu_tbl_m2;        /* 0x004b4850 */
extern void  LFHoldUp_Shape(void);      /* 0x0040fe50 */
extern void  LFHoldUp_Probe(void);      /* 0x0040fe80 */
extern void  LFHoldUp_Geom(void);       /* 0x0040feb0 */
extern void  LFHoldUp_Place(void);      /* 0x0040fad0 */

extern RideDef*   g_lfdr_def;           /* 0x004c8d6c  LOG FLUME DROP */
extern RideElem*  g_lfdr_elem;          /* 0x004c8d4c */
extern SpriteRec* g_lfdr_spr_m1;        /* 0x004c2af0  "drop1_m.lls" */
extern SpriteRec* g_lfdr_spr_m2;        /* 0x004c2aec  "drop2_m.lls" */
extern SpriteRec* g_lfdr_spr_splash;    /* 0x004c2b64  "lf_splash.lls" */
extern void  LFDrop_Shape(void);        /* 0x004102e0 */
extern void  LFDrop_Probe(void);        /* 0x00410310 */
extern void  LFDrop_Geom(void);         /* 0x00410360 */
extern void  LFDrop_Place(void);        /* 0x00410180 */

/* ---- LOG FLUME TUNNEL -------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0040f3e0
void LFTunnel_Create(RideElem* elem)
{
    RideDef* def = elem->data;

    g_lftu_def = def;
    def->flags1c |= 0x400;
    g_lftu_elem = elem;
    if (g_lftu_def && g_lftu_def->sprite)
        g_lftu_def->sprite->flags |= 0x2000;
    g_lftu_spr = LoadSprite("tunel_m.lls", 1);
}

// FUNCTION: LEGOLAND 0x0040f430
void LFTunnel_Destroy(void)
{
    if (g_lftu_spr)
        KillSprite(g_lftu_spr);
}

// FUNCTION: LEGOLAND 0x0040f4f0
void LFTunnel_Tick(void)
{
    RideDef* def = g_lftu_def;

    LFPiece_TickCommon(def, &def->footprint);
}

// FUNCTION: LEGOLAND 0x0040f510
void LFTunnel_Update(RideElem* elem, void* a, void* b)
{
    RideDef* def = g_lftu_def;

    LFPiece_UpdateCommon(def, a, b, &def->footprint,
                         LFTunnel_Geom, LFTunnel_Probe);
}

// FUNCTION: LEGOLAND 0x0040f5a0
void LFTunnel_Update2(RideElem* elem, const Pos* pos)
{
    LFPiece_RefreshAt(pos);
}

// FUNCTION: LEGOLAND 0x0040f540
void LFTunnel_Add(RideElem* elem, const Pos* pos)
{
    BPos sq;

    sq.x = (unsigned char)pos->x;
    sq.y = (unsigned char)pos->y;
    LFPiece_AddCommon(sq, &g_lftu_def->footprint, g_lftu_elem,
                      LFTunnel_Place, LFTunnel_Geom, LFTunnel_Shape);
}

// FUNCTION: LEGOLAND 0x0040f580
void LFTunnel_Remove(void* a, void* b, void* c)
{
    LFPiece_RemoveCommon(a, b, c, LFTunnel_Geom);
}

/* ---- LOG FLUME CSAW ---------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0040f8b0
void LFCsaw_Create(RideElem* elem)
{
    RideDef* def = elem->data;

    g_lfcs_def = def;
    def->flags1c |= 0x400;
    g_lfcs_elem = elem;
    if (g_lfcs_def && g_lfcs_def->sprite)
        g_lfcs_def->sprite->flags |= 0x2000;
    g_lfcs_spr = LoadSprite("csaw2_m.lls", 1);
}

// FUNCTION: LEGOLAND 0x0040f900
void LFCsaw_Destroy(void)
{
    if (g_lfcs_spr)
        KillSprite(g_lfcs_spr);
}

// FUNCTION: LEGOLAND 0x0040fa00
void LFCsaw_Tick(void)
{
    RideDef* def = g_lfcs_def;

    LFPiece_TickCommon(def, &def->footprint);
}

// FUNCTION: LEGOLAND 0x0040fa20
void LFCsaw_Update(RideElem* elem, void* a, void* b)
{
    RideDef* def = g_lfcs_def;

    LFPiece_UpdateCommon(def, a, b, &def->footprint,
                         LFCsaw_Geom, LFCsaw_Probe);
}

// FUNCTION: LEGOLAND 0x0040fa50
void LFCsaw_Update2(RideElem* elem, const Pos* pos)
{
    LFPiece_RefreshAt(pos);
}

// FUNCTION: LEGOLAND 0x0040fa60
void LFCsaw_Add(RideElem* elem, const Pos* pos)
{
    BPos sq;

    sq.x = (unsigned char)pos->x;
    sq.y = (unsigned char)pos->y;
    g_lf_corner_index = 0;      /* copy-paste leftover, see the note above */
    LFPiece_AddCommon(sq, &g_lfcs_def->footprint, g_lfcs_elem,
                      LFCsaw_Place, LFCsaw_Geom, LFCsaw_Shape);
}

// FUNCTION: LEGOLAND 0x0040fab0
void LFCsaw_Remove(void* a, void* b, void* c)
{
    LFPiece_RemoveCommon(a, b, c, LFCsaw_Geom);
}

/* ---- LOG FLUME HOLD UP ------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0040ff30
void LFHoldUp_Create(RideElem* elem)
{
    RideDef* def = elem->data;

    g_lfhu_def = def;
    def->flags1c |= 0x400;
    g_lfhu_elem = elem;
    if (g_lfhu_def && g_lfhu_def->sprite)
        g_lfhu_def->sprite->flags |= 0x2000;
    g_lfhu_spr_m1 = LoadSprite("hup1_m1.lls", 1);
    g_lfhu_spr_m2 = LoadSprite("hup1_m2.lls", 1);
    g_lfhu_tbl_m1 = g_lfhu_spr_m1;
    g_lfhu_tbl_m2 = g_lfhu_spr_m2;
}

// FUNCTION: LEGOLAND 0x0040ffa0
void LFHoldUp_Destroy(void)
{
    if (g_lfhu_spr_m1)
        KillSprite(g_lfhu_spr_m1);
    if (g_lfhu_spr_m2)
        KillSprite(g_lfhu_spr_m2);
}

// FUNCTION: LEGOLAND 0x004100b0
void LFHoldUp_Tick(void)
{
    RideDef* def = g_lfhu_def;

    LFPiece_TickCommon(def, &def->footprint);
}

// FUNCTION: LEGOLAND 0x004100d0
void LFHoldUp_Update(RideElem* elem, void* a, void* b)
{
    RideDef* def = g_lfhu_def;

    LFPiece_UpdateCommon(def, a, b, &def->footprint,
                         LFHoldUp_Geom, LFHoldUp_Probe);
}

// FUNCTION: LEGOLAND 0x00410100
void LFHoldUp_Update2(RideElem* elem, const Pos* pos)
{
    LFPiece_RefreshAt(pos);
}

// FUNCTION: LEGOLAND 0x00410110
void LFHoldUp_Add(RideElem* elem, const Pos* pos)
{
    BPos sq;

    sq.x = (unsigned char)pos->x;
    sq.y = (unsigned char)pos->y;
    g_lf_corner_index = 0;      /* copy-paste leftover, see the note above */
    LFPiece_AddCommon(sq, &g_lfhu_def->footprint, g_lfhu_elem,
                      LFHoldUp_Place, LFHoldUp_Geom, LFHoldUp_Shape);
}

// FUNCTION: LEGOLAND 0x00410160
void LFHoldUp_Remove(void* a, void* b, void* c)
{
    LFPiece_RemoveCommon(a, b, c, LFHoldUp_Geom);
}

/* ---- LOG FLUME DROP ---------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004103e0
void LFDrop_Create(RideElem* elem)
{
    RideDef* def = elem->data;

    g_lfdr_def = def;
    def->flags1c |= 0x400;
    g_lfdr_elem = elem;
    if (g_lfdr_def && g_lfdr_def->sprite)
        g_lfdr_def->sprite->flags |= 0x2000;
    g_lfdr_spr_m1     = LoadSprite("drop1_m.lls", 1);
    g_lfdr_spr_m2     = LoadSprite("drop2_m.lls", 1);
    g_lfdr_spr_splash = LoadSprite("lf_splash.lls", 1);
}

/* The DROP is the only set piece that CLEARS its sprite globals after
 * releasing them, so a second create/destroy cycle cannot double-free. */
// FUNCTION: LEGOLAND 0x00410450
void LFDrop_Destroy(void)
{
    if (g_lfdr_spr_splash) {
        KillSprite(g_lfdr_spr_splash);
        g_lfdr_spr_splash = 0;
    }
    if (g_lfdr_spr_m1) {
        KillSprite(g_lfdr_spr_m1);
        g_lfdr_spr_m1 = 0;
    }
    if (g_lfdr_spr_m2) {
        KillSprite(g_lfdr_spr_m2);
        g_lfdr_spr_m2 = 0;
    }
}

// FUNCTION: LEGOLAND 0x004106e0
void LFDrop_Tick(void)
{
    RideDef* def = g_lfdr_def;

    LFPiece_TickCommon(def, &def->footprint);
}

// FUNCTION: LEGOLAND 0x00410700
void LFDrop_Update(RideElem* elem, void* a, void* b)
{
    RideDef* def = g_lfdr_def;

    LFPiece_UpdateCommon(def, a, b, &def->footprint,
                         LFDrop_Geom, LFDrop_Probe);
}

// FUNCTION: LEGOLAND 0x00410730
void LFDrop_Update2(RideElem* elem, const Pos* pos)
{
    LFPiece_RefreshAt(pos);
}

// FUNCTION: LEGOLAND 0x00410740
void LFDrop_Add(RideElem* elem, const Pos* pos)
{
    BPos sq;

    sq.x = (unsigned char)pos->x;
    sq.y = (unsigned char)pos->y;
    g_lf_corner_index = 0;      /* copy-paste leftover, see the note above */
    LFPiece_AddCommon(sq, &g_lfdr_def->footprint, g_lfdr_elem,
                      LFDrop_Place, LFDrop_Geom, LFDrop_Shape);
}

// FUNCTION: LEGOLAND 0x00410790
void LFDrop_Remove(void* a, void* b, void* c)
{
    LFPiece_RemoveCommon(a, b, c, LFDrop_Geom);
}

/* =========================================================================
 * THE SET PIECES' +0xb0 SLOT IS THE DRAW PASS, NOT AN INTERACTION.
 *
 * Like CASTLE OBJ (castleobj.c), the log flume uses ObjDef +0xb0 as the
 * per-square render hook: it is called with (elem, ?, ?, square, ?, mode)
 * and blits the class's OWN overlay sprite on top of whatever the map
 * already drew.  Each one:
 *
 *   1. finds the placed piece for the square (LFPiece_FindAt),
 *   2. if the square is on screen, copies the animation frame from the
 *      class's own .lls onto the overlay .lls so the overlay stays in step
 *      with the piece animation,
 *   3. blits the overlay at the piece's screen position, and
 *   4. marks the piece drawn for this frame.
 *
 * ORIGINAL BUG, reproduced faithfully: `frame` is only assigned inside
 * `if (lls != 0)`.  When the class sprite has no .lls the UNINITIALISED
 * value is handed to LLSSetFrame -- and VC6 homed that local in the dead
 * first-argument slot, so what actually gets used is the element pointer
 * reinterpreted as a frame index.  LLSSetFrame clamps it, so the visible
 * effect is the overlay sticking on its last frame.
 * ========================================================================= */

typedef struct Offset { int ox; int oy; } Offset;
typedef struct LLS { short frame; } LLS;

extern LLS*   GetLLSForSprite(void* sprite);                     /* 0x00441e80 */
extern void*  GetSpriteForLayer(void* sprite, int layer);        /* 0x00441ec0 */
extern Offset GetRenderOffsetForLayer(void* sprite, int layer);  /* 0x00441ee0 */
extern void   LLSSetFrame(LLS* lls, int frame);                  /* 0x0047d5a0 */
extern Offset GetScreenCoordsForObject(const BPos* sq, RideDef* def); /* 0x00442cc0 */
extern void   AdjustOffsetForViewMode(Offset* off);              /* 0x00442d30 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */

extern int    LFPiece_IsVisible(const BPos* sq);                 /* 0x0040cdf0 */
extern Offset LFPiece_ScreenPos(void* piece);                    /* 0x0040cfd0 */

// FUNCTION: LEGOLAND 0x0040f450
void LFTunnel_Interact(RideElem* elem, int b, int c, const BPos* sq,
                       int e, int mode)
{
    RideDef* def = elem->data;
    void*    piece;
    int      frame;             /* deliberately left uninitialised */
    LLS*     lls;
    Offset   pos;

    piece = LFPiece_FindAt(sq);
    if (LFPiece_IsVisible(sq)) {
        if (g_lftu_spr) {
            lls = GetLLSForSprite(def->sprite);
            if (lls)
                frame = lls->frame;
            lls = GetLLSForSprite(g_lftu_spr);
            if (lls)
                LLSSetFrame(lls, frame);
        }
        pos = LFPiece_ScreenPos(piece);
        if (g_lftu_spr)
            PrintSprite(g_lftu_spr, pos.ox, pos.oy, mode, 0);
    }
    LFPiece_MarkDrawn(piece, 1);
}

// FUNCTION: LEGOLAND 0x0040f920
void LFCsaw_Interact(RideElem* elem, int b, int c, const BPos* sq,
                     int e, int mode)
{
    RideDef* def = elem->data;
    void*    piece;
    int      frame;             /* deliberately left uninitialised */
    LLS*     lls;
    Offset   off;
    Offset   pos;

    piece = LFPiece_FindAt(sq);
    if (LFPiece_IsVisible(sq)) {
        if (g_lfcs_spr) {
            lls = GetLLSForSprite(GetSpriteForLayer(def->sprite, 1));
            if (lls)
                frame = lls->frame;
            lls = GetLLSForSprite(g_lfcs_spr);
            if (lls)
                LLSSetFrame(lls, frame);
        }
        off = GetRenderOffsetForLayer(def->sprite, 1);
        pos = LFPiece_ScreenPos(piece);
        AdjustOffsetForViewMode(&off);
        if (g_lfcs_spr)
            PrintSprite(g_lfcs_spr, pos.ox + off.ox, pos.oy + off.oy,
                        mode, 0);
    }
    LFPiece_MarkDrawn(piece, 1);
}

/* =========================================================================
 * THE MULTI-SPRITE OVERLAY TABLES  (0x004b47c0 .. 0x004b4864, .data)
 *
 * SPECIAL CORNERs 1 and 3 and the HOLD UP do not blit ONE overlay; they
 * blit a SET of them at fixed offsets from the piece.  The set is a static
 * two-field record { count, frames } and each frame is
 * { int dx; int dy; SpriteRec* sprite; } -- twelve bytes.  The sprite
 * fields start out null and are filled in by the class create hook, which
 * is exactly what the "static render table" stores above are:
 *
 *   g_lfc1_anim (0x004b4808) = { 2, 0x004b47f0 }   frames' sprites at
 *                                                  0x004b47f8, 0x004b4804
 *   g_lfc3_anim (0x004b4828) = { 2, 0x004b4810 }   0x004b4818, 0x004b4824
 *   g_lfhu_anim (0x004b4858) = { 3, 0x004b4830 }   0x004b4838, 0x004b4850
 *                                                  (frame 1's sprite is
 *                                                  never filled in -- it
 *                                                  stays null forever)
 *
 * Corners 2 and 4 have a single overlay and blit it with PrintSprite
 * directly instead.
 * ========================================================================= */

typedef struct LFAnimSet {
    int   count;                /* +0x00 */
    void* frames;               /* +0x04 */
} LFAnimSet;

extern LFAnimSet g_lfc1_anim;   /* 0x004b4808 */
extern LFAnimSet g_lfc3_anim;   /* 0x004b4828 */
extern LFAnimSet g_lfhu_anim;   /* 0x004b4858 */

extern void LFPiece_DrawAnim(void* piece, int x, int y,
                             LFAnimSet* anim, int flag);         /* 0x0040b290 */

/* SPECIAL CORNER 1's draw pass.  The zero that initialises `frame` is the
 * same register VC6 uses for the `g_lf_corner_index = 0` store. */
// FUNCTION: LEGOLAND 0x0040edb0
void LFCorner1_Interact(RideElem* elem, int b, int c, const BPos* sq,
                        int e, int mode)
{
    RideDef* def = elem->data;
    int      frame = 0;
    LLS*     lls;
    Offset   pos;
    void*    piece;

    g_lf_corner_index = 0;
    pos = GetScreenCoordsForObject(sq, def);
    lls = GetLLSForSprite(def->sprite);
    if (lls)
        frame = lls->frame;
    lls = GetLLSForSprite(g_lfc1_spr_m1);
    if (lls)
        LLSSetFrame(lls, frame);
    lls = GetLLSForSprite(g_lfc1_spr_m2);
    if (lls)
        LLSSetFrame(lls, frame);
    LFPiece_IsVisible(sq);      /* result discarded -- original quirk */
    piece = LFPiece_FindAt(sq);
    if (piece) {
        LFPiece_DrawAnim(piece, pos.ox, pos.oy, &g_lfc1_anim, 0);
        LFPiece_MarkDrawn(piece, 1);
    }
}

/* SPECIAL CORNER 2's draw pass: one overlay, and the whole animation block
 * lives inside the on-screen guard -- which is why ebx is pushed INSIDE the
 * guarded path rather than in the prologue. */
// FUNCTION: LEGOLAND 0x0040ee60
void LFCorner2_Interact(RideElem* elem, int b, int c, const BPos* sq,
                        int e, int mode)
{
    RideDef* def = elem->data;
    void*    piece;

    g_lf_corner_index = 1;
    piece = LFPiece_FindAt(sq);
    if (LFPiece_IsVisible(sq)) {
        int    frame = 0;
        LLS*   lls;
        Offset pos;

        lls = GetLLSForSprite(def->sprite);
        if (lls)
            frame = lls->frame;
        lls = GetLLSForSprite(g_lfc2_spr_m1);
        if (lls)
            LLSSetFrame(lls, frame);
        pos = GetScreenCoordsForObject(sq, def);
        if (g_lfc2_spr_m1)
            PrintSprite(g_lfc2_spr_m1, pos.ox, pos.oy, mode, 0);
    }
    LFPiece_MarkDrawn(piece, 1);
}

// FUNCTION: LEGOLAND 0x0040ef00
void LFCorner3_Interact(RideElem* elem, int b, int c, const BPos* sq,
                        int e, int mode)
{
    RideDef* def = elem->data;
    int      frame = 0;
    LLS*     lls;
    Offset   pos;
    void*    piece;

    g_lf_corner_index = 2;
    lls = GetLLSForSprite(def->sprite);
    if (lls)
        frame = lls->frame;
    lls = GetLLSForSprite(g_lfc3_spr_m1);
    if (lls)
        LLSSetFrame(lls, frame);
    lls = GetLLSForSprite(g_lfc3_spr_m2);
    if (lls)
        LLSSetFrame(lls, frame);
    pos = GetScreenCoordsForObject(sq, def);
    LFPiece_IsVisible(sq);      /* result discarded -- original quirk */
    piece = LFPiece_FindAt(sq);
    if (piece) {
        LFPiece_DrawAnim(piece, pos.ox, pos.oy, &g_lfc3_anim, 1);
        LFPiece_MarkDrawn(piece, 1);
    }
}

// FUNCTION: LEGOLAND 0x0040efb0
void LFCorner4_Interact(RideElem* elem, int b, int c, const BPos* sq,
                        int e, int mode)
{
    RideDef* def = elem->data;
    void*    piece;

    g_lf_corner_index = 3;
    piece = LFPiece_FindAt(sq);
    if (LFPiece_IsVisible(sq)) {
        int    frame = 0;
        LLS*   lls;
        Offset pos;

        lls = GetLLSForSprite(def->sprite);
        if (lls)
            frame = lls->frame;
        lls = GetLLSForSprite(g_lfc4_spr_m);
        if (lls)
            LLSSetFrame(lls, frame);
        pos = GetScreenCoordsForObject(sq, def);
        if (g_lfc4_spr_m)
            PrintSprite(g_lfc4_spr_m, pos.ox, pos.oy, mode, 0);
    }
    LFPiece_MarkDrawn(piece, 1);
}

/* HOLD UP's draw pass: two overlays driven off ONE frame index, drawn
 * through the three-frame anim set.  Unlike the others it computes the
 * screen position BEFORE the visibility test -- and then throws the
 * visibility answer away entirely. */
// FUNCTION: LEGOLAND 0x0040ffd0
void LFHoldUp_Interact(RideElem* elem, int b, int c, const BPos* sq,
                       int e, int mode)
{
    RideDef* def = elem->data;
    int      frame = 0;
    void*    spr;
    LLS*     lls;
    Offset   pos;
    Offset   off;
    void*    piece;

    spr = GetSpriteForLayer(def->sprite, 1);
    if (spr) {
        lls = GetLLSForSprite(spr);
        if (lls)
            frame = lls->frame;
    }
    lls = GetLLSForSprite(g_lfhu_spr_m1);
    if (lls)
        LLSSetFrame(lls, frame);
    lls = GetLLSForSprite(g_lfhu_spr_m2);
    if (lls)
        LLSSetFrame(lls, frame);
    pos = GetScreenCoordsForObject(sq, def);
    off = GetRenderOffsetForLayer(def->sprite, 1);
    AdjustOffsetForViewMode(&off);
    pos.ox += off.ox;
    pos.oy += off.oy;
    LFPiece_IsVisible(sq);      /* result discarded -- original quirk */
    piece = LFPiece_FindAt(sq);
    if (piece) {
        LFPiece_DrawAnim(piece, pos.ox, pos.oy, &g_lfhu_anim, 0);
        LFPiece_MarkDrawn(piece, 1);
    }
}

/* =========================================================================
 * LOG FLUME DROP -- the draw pass, and the only place in the set-piece
 * group that knows about the BOATS.
 *
 * A placed piece record carries the boat currently occupying it at +0x24.
 * When that boat has bit 2 of its flag byte set (+0x04) the drop is being
 * ridden, and the DROP additionally blits "lf_splash.lls" at a fixed
 * (-74, +149) offset from the piece, with the splash .lls frozen
 * (LLSStop) on the frame the boat itself carries (boat +0x24).
 * That is how the splash stays locked to the boat's fall rather than
 * free-running.
 * ========================================================================= */

/* A RUN of flume -- and, it turns out, the STATION record too: the list
 * LFStation_FindAt walks (g_lf_queue, 0x004cbe84) is a list of these, keyed
 * by the map square at +0x14, and the piece list at +0x10 is the same one
 * LFRun_AddPiece pushes onto. So one record is one connected stretch of
 * channel together with the entrance that feeds it; every placed piece
 * points back at it through LFPiece +0x24, and a piece laid next to an
 * existing piece JOINS that piece's run. Only the fields this file touches
 * are named. */
typedef struct LFAnimRefs { int r[3]; } LFAnimRefs;

typedef struct LFRun {
    struct LFRun*   next;       /* +0x00  next station/run */
    unsigned char   flags;      /* +0x04  bit 2 = a boat is falling here */
    unsigned char   pad05[3];
    struct LFPiece* f08;        /* +0x08  a piece of this run (saved by index) */
    struct LFPiece* f0c;        /* +0x0c  ditto */
    struct LFPiece* pieces;     /* +0x10  head of this run's piece list */
    BPos            sq;         /* +0x14  the station's map square */
    unsigned char   pad16[2];
    struct LFPiece* f18;        /* +0x18  ditto */
    int             frame;      /* +0x1c  the run's shared animation frame */
    unsigned char   pad20[4];
    int             splash_frame; /* +0x24 frame the drop splash freezes on */
    unsigned char   pad28[4];
    LFAnimRefs      refs;       /* +0x2c  three animation references */
    void*           f38;        /* +0x38  a boat (saved as its index) */
    unsigned char   pad3c[0xd0 - 0x3c];
    int             piece_count;  /* +0xd0 squares of flume on this run */
} LFRun;                        /* 0xd4 -- exactly the save record */

/* A PLACED PIECE of log flume: one map square with a piece definition on it.
 * Allocated 0x38 bytes, zeroed, by LFPiece_Alloc (0x00409010). */
typedef struct LFPiece {
    struct LFPiece* next;       /* +0x00  next piece of the same run */
    struct LFPiece* prev;       /* +0x04 */
    void*         f08;          /* +0x08 */
    void*         f0c;          /* +0x0c */
    unsigned int  flags;        /* +0x10  bit 0 = suppress the cursor reset */
    BPosW         sq;           /* +0x14  the piece's map square */
    unsigned char pad16[2];
    int           f18;          /* +0x18  piece kind (3 = plain channel) */
    int           f1c;          /* +0x1c  orientation / variant, 0..3 */
    RideDef*      def;          /* +0x20  the class definition that placed it */
    LFRun*        run;          /* +0x24  the run this piece belongs to */
    int           f28;          /* +0x28 */
} LFPiece;

extern int  g_lf_splash_dx;     /* 0x004b4860  = -74 */
extern int  g_lf_splash_dy;     /* 0x004b4864  = 149 */
extern void LLSStop(LLS* lls);                                   /* 0x0047d4c0 */

// FUNCTION: LEGOLAND 0x004104b0
void LFDrop_Interact(RideElem* elem, int b, int c, const BPos* sq,
                     int e, int mode)
{
    RideDef* def = elem->data;
    LFPiece* piece;
    Offset   off;
    Offset   pos;
    LLS*     lls;
    void*    spr;
    int      splash_frame;

    piece = (LFPiece*)LFPiece_FindAt(sq);
    if (LFPiece_IsVisible(sq) && piece) {
        if (g_lfdr_spr_m1 && g_lfdr_spr_m2) {
            int frame = 0;

            spr = GetSpriteForLayer(def->sprite, 0);
            if (spr) {
                lls = GetLLSForSprite(spr);
                if (lls)
                    frame = lls->frame;
            }
            lls = GetLLSForSprite(g_lfdr_spr_m1);
            if (lls)
                LLSSetFrame(lls, frame);
            spr = GetSpriteForLayer(def->sprite, 1);
            if (spr) {
                lls = GetLLSForSprite(spr);
                if (lls)
                    frame = lls->frame;
            }
            lls = GetLLSForSprite(g_lfdr_spr_m2);
            if (lls)
                LLSSetFrame(lls, frame);
        }
        pos = LFPiece_ScreenPos(piece);
        if (g_lfdr_spr_m1) {
            off = GetRenderOffsetForLayer(def->sprite, 0);
            AdjustOffsetForViewMode(&off);
            PrintSprite(g_lfdr_spr_m1, pos.ox + off.ox, pos.oy + off.oy,
                        mode, 0);
        }
        if (g_lfdr_spr_m2) {
            off = GetRenderOffsetForLayer(def->sprite, 1);
            AdjustOffsetForViewMode(&off);
            PrintSprite(g_lfdr_spr_m2, pos.ox + off.ox, pos.oy + off.oy,
                        mode, 0);
        }
        if (piece->run && (piece->run->flags & 2)) {
            Offset sp = LFPiece_ScreenPos(piece);

            off = GetRenderOffsetForLayer(def->sprite, 1);
            off.ox += g_lf_splash_dx;
            off.oy += g_lf_splash_dy;
            AdjustOffsetForViewMode(&off);
            if (g_lfdr_spr_splash) {
                splash_frame = piece->run->splash_frame;
                lls = GetLLSForSprite(g_lfdr_spr_splash);
                if (lls) {
                    LLSStop(lls);
                    LLSSetFrame(lls, splash_frame);
                }
                PrintSprite(g_lfdr_spr_splash, sp.ox + off.ox,
                            sp.oy + off.oy, mode, 0);
            }
        }
    }
    LFPiece_MarkDrawn(piece, 1);
}

/* =========================================================================
 * LOG FLUME TRACK -- the channel the player actually draws.
 *
 * The TRACK class owns the flume's shared resources, which is why its
 * create hook is by far the biggest of the ten:
 *
 *   - "LOG FLUME IMAGE LIST"      the tile image list, loaded through the
 *                                 LLIDB and kept at g_lf_imagelist_data;
 *   - "LOG FLUME TRACK ENDY LIST" the end-piece list, g_lf_endylist_data;
 *   - ten track sprites (g_lf_track_names -> g_lf_track_sprites):
 *       0..3  fc1a_m / fc2a_m / fc3a_m / fc4a_m   the four corner channels
 *       4,5   fs1_m / fs2_m                       the two straights
 *       6..9  fe2_m / fe3_m / fe4_m / fe1_m       the four end caps
 *   - two extra corner overlays, fc1_m3.lls and fc3_m3.lls.
 *
 * It also zeroes the class definition's two draw parameters (+0x14/+0x18)
 * and calls LFTrack_BuildGeometry, which derives the flume's sub-tile step
 * from GetTileDimensions.  The class ObjDef is ALSO cached by
 * LogFlume_GetInterfaces at 0x0082c688 (see interfaces.c); this file's own
 * copy at 0x004cbe30 is the one the track callbacks use.
 * ========================================================================= */

extern RideDef*   g_lftr_def;           /* 0x004cbe30  LOG FLUME TRACK */
extern RideElem*  g_lftr_elem;          /* 0x004c74f4 */
extern void*      g_lf_imagelist_elem;  /* 0x004c2afc  "LOG FLUME IMAGE LIST" */
extern struct LFImageList* g_lf_imagelist_data;  /* 0x004c2b68 */
extern void*      g_lf_endylist_elem;   /* 0x004c2b10  "LOG FLUME TRACK ENDY LIST" */
extern void*      g_lf_endylist_data;   /* 0x004cbe50 */
extern char*      g_lf_track_names[];   /* 0x004b4768  ten .lls names */
extern SpriteRec* g_lf_track_sprites[]; /* 0x004c2abc  ten loaded sprites */
extern SpriteRec* g_lfc1_spr_m3;        /* 0x004cbe1c  "fc1_m3.lls" */
extern SpriteRec* g_lfc3_spr_m3;        /* 0x004c8d68  "fc3_m3.lls" */

extern int   LLIDB_FindElement(const char* name, void** out,
                               unsigned int* idx);                /* 0x0047b330 */
extern void* LLIDB_LoadData(void* elem);                          /* 0x0047d3a0 */
extern void  LLIDB_UnLoadData(void* elem);                        /* 0x0047d450 */
extern void  LFTrack_BuildGeometry(void);                         /* 0x004113d0 */

// FUNCTION: LEGOLAND 0x0040c350
void LFTrack_Create(RideElem* elem)
{
    RideDef* def = elem->data;
    int      i;

    g_lftr_def = def;
    def->flags1c |= 0x400;
    g_lftr_def->f18 = 0;
    g_lftr_def->f14 = 0;
    g_lftr_elem = elem;
    if (LLIDB_FindElement("LOG FLUME IMAGE LIST", &g_lf_imagelist_elem, 0) == 0)
        g_lf_imagelist_data = LLIDB_LoadData(g_lf_imagelist_elem);
    if (LLIDB_FindElement("LOG FLUME TRACK ENDY LIST", &g_lf_endylist_elem, 0) == 0)
        g_lf_endylist_data = LLIDB_LoadData(g_lf_endylist_elem);
    LFTrack_BuildGeometry();
    for (i = 0; i < 10; i++)
        g_lf_track_sprites[i] = LoadSprite(g_lf_track_names[i], 1);
    g_lfc1_spr_m3 = LoadSprite("fc1_m3.lls", 1);
    g_lfc3_spr_m3 = LoadSprite("fc3_m3.lls", 1);
}

// FUNCTION: LEGOLAND 0x0040c430
void LFTrack_Destroy(void)
{
    int i;

    LLIDB_UnLoadData(g_lf_imagelist_elem);
    for (i = 0; i < 10; i++) {
        if (g_lf_track_sprites[i])
            KillSprite(g_lf_track_sprites[i]);
    }
    if (g_lfc1_spr_m3)
        KillSprite(g_lfc1_spr_m3);
    if (g_lfc3_spr_m3)
        KillSprite(g_lfc3_spr_m3);
    LLIDB_UnLoadData(g_lf_endylist_elem);
}

/* =========================================================================
 * HOW A FLUME ROUTE IS BUILT, AND THE TWO EDIT CURSORS
 *
 * Drawing flume is a per-square operation.  The TRACK class's update slot
 * runs every frame while the tool is held: it stamps the flume's footprint
 * template (g_lf_footprint, 0x004b4728 -- a 20-byte Footprint whose first
 * four dwords are the piece's cell rect) onto the EDIT cursor, converts the
 * mouse to a map reference, and then refuses the square for one of four
 * reasons, each its own cursor error code:
 *
 *      3   people are standing in the footprint
 *      4   the footprint query itself failed
 *      2   not enough bricks (GetObjCost vs GetBrickCount)
 *      0xe the piece does not connect to anything
 *
 * Two cursor blocks are involved and both are 0x1834 bytes:
 *   g_edit_cursor  (0x007febc0) the real one the player is dragging;
 *   g_ghost_cursor (0x00810160) the preview the +0x94 slot paints when the
 *                               mouse merely passes over existing flume.
 *
 * Placement itself (LFTrack_Add) allocates a 0x38-byte LFPiece, stamps the
 * square and the class definition into it, probes the four neighbours, and
 * JOINS the run of the first neighbour that has one -- which is how a chain
 * of squares becomes a single connected route.  Note the fallback when the
 * square has NO neighbour: the original assigns the neighbour ARRAY itself
 * as the run.  That is reproduced verbatim below; it is either a deliberate
 * "empty run" sentinel or an original bug, and it cannot be told apart from
 * this side of the disassembly.
 * ========================================================================= */

typedef struct EditCursorRec {
    unsigned char pad0000[0x1404];
    int           x;                    /* +0x1404 */
    int           y;                    /* +0x1408 */
    unsigned char pad140c[8];
    Footprint     footprint;            /* +0x1414 */
    unsigned char pad1428[0x1828 - 0x1428];
    int           f1828;                /* +0x1828 */
    unsigned char pad182c[4];
    void*         link;                 /* +0x1830  preview-chain link */
} EditCursorRec;

extern EditCursorRec g_edit_cursor;     /* 0x007febc0  EditCursor */
extern EditCursorRec g_ghost_cursor;    /* 0x00810160 */
extern Footprint     g_lf_footprint;    /* 0x004b4728  the flume's cell rect */
extern Pos           g_mapref;          /* 0x007fffc4  ScreenToMapRef output */

extern void ScreenToMapRef(int screen, Pos* out, int mode);      /* 0x0045be90 */
extern void ResetCursorFootprint(EditCursorRec* c);              /* 0x0045f460 */
extern void SetCursorError(EditCursorRec* c, int code);          /* 0x0045f480 */
extern int  CursorIsValid(EditCursorRec* c);                     /* 0x0045f4b0 */
extern void PropagateCursorStatus(EditCursorRec* c);             /* 0x0045f4d0 */
extern void ValidateCursor(EditCursorRec* c, RideDef* def);      /* 0x0045f810 */

extern LFPiece* LFTrack_FindPieceAt(int x, int y);               /* 0x0040d210 */
extern int      LFPiece_CursorFits(LFPiece* piece);              /* 0x00409140 */
extern int      LFRun_IsComplete(LFRun* run);                    /* 0x0040ba80 */

// FUNCTION: LEGOLAND 0x0040c6c0
void LFTrack_Update2(RideElem* elem, const Pos* pos)
{
    LFPiece* piece = LFTrack_FindPieceAt(pos->x, pos->y);

    if (piece) {
        SetCursorError(&g_ghost_cursor, 1);
        g_ghost_cursor.x = piece->sq.b.x;
        g_ghost_cursor.y = piece->sq.b.y;
        g_ghost_cursor.footprint = g_lf_footprint;
        g_ghost_cursor.footprint.v[2] = g_lf_footprint.v[2] - 1;
        g_ghost_cursor.footprint.v[3] = g_ghost_cursor.footprint.v[3] - 1;
        g_ghost_cursor.f1828 = 8;
        if (LFPiece_CursorFits(piece)) {
            if (piece->f0c && piece->f08) {
                if (!LFRun_IsComplete(piece->run))
                    return;
            }
            if (!(piece->flags & 1))
                ResetCursorFootprint(&g_ghost_cursor);
        }
    }
}

typedef struct Rect { int left; int top; int right; int bottom; } Rect;

/* The neighbour-array pointer the probe hands back. The original reads the
 * SAME slot back as a run pointer on the "no neighbours" path (see below),
 * so it is modelled as a union of the two views. */
extern void* g_8003f0;   /* 0x008003f0  head of the preview-cursor chain */
extern int   CheckForPeople(const Rect* r);                      /* 0x00485260 */
extern int   GetObjCost(RideDef* def);                           /* 0x00480da0 */
extern int   GetBrickCount(void);                                /* 0x004578e0 */

/* The neighbour probe writes into a fixed four-slot array at 0x004cbe20 and
 * hands back its address, so `nb[0..3]` are the pieces N/E/S/W of the
 * square.  DropFullNeighbours discards any whose +0x18 is not 3 or 4 (a
 * piece with no free end); NeighbourMask packs the survivors two bits per
 * direction (1 / 4 / 0x10 / 0x40) and MaskIsLegal accepts only the shapes a
 * flume piece can actually be: one end, or two ends that form a straight or
 * a corner. */
extern void LFTrack_ProbeNeighbours(BPos sq, LFPiece*** out);    /* 0x00409440 */
extern void LFTrack_DropFullNeighbours(LFPiece** nb);            /* 0x00409510 */
extern int  LFTrack_CountNeighbours(LFPiece** nb);               /* 0x00409470 */
extern int  LFTrack_NeighbourMask(LFPiece** nb);                 /* 0x00409410 */
extern int  LFTrack_MaskIsLegal(int mask, LFPiece** nb);         /* 0x00409580 */
extern void LFRun_KeepOnly(LFRun* run, LFPiece** nb);            /* 0x004094b0 */
extern void LFTrack_CommitPlacement(LFPiece** nb,
                                    EditCursorRec* c);           /* 0x0040d520 */

/* CLOSED (163/163, 538 bytes): the last residual was the fallback arm of
 * the neighbour chain, where the original loads `run` from a stack slot
 * (`mov ecx,[esp+0x20]`, the same slot `nb` lives in).  That is not a
 * reload of `nb` at all: `run` is UNINITIALISED when all four neighbours are
 * null (an original latent bug -- the arm is unreachable because
 * CountNeighbours returned non-zero).  Writing NO else-arm reproduces it:
 * VC6 homes the undefined value in the dead `elem` argument slot, which is
 * also where `nb` was homed, and emits a real load.  Any explicit fallback
 * (`(LFRun*)nb`, a union read, a cast through &nb) is CSE'd into
 * `mov ecx,eax`. */
// FUNCTION: LEGOLAND 0x0040c4a0
void LFTrack_Update(RideElem* elem, int screen, int mode)
{
    Rect       r;
    RideDef*   def = elem->data;
    LFPiece**  nb;
    BPos       sq;
    int        people;
    int        cost;

    g_edit_cursor.footprint = g_lf_footprint;
    g_edit_cursor.footprint.v[2] = g_lf_footprint.v[2] - 1;
    g_edit_cursor.footprint.v[3] = g_edit_cursor.footprint.v[3] - 1;
    ScreenToMapRef(screen, &g_mapref, mode);
    ResetCursorFootprint(&g_edit_cursor);
    g_8003f0 = 0;
    ValidateCursor(&g_edit_cursor, def);
    if (CursorIsValid(&g_edit_cursor)) {
        r.left   = g_edit_cursor.footprint.v[0] + g_mapref.x;
        r.top    = g_mapref.y + g_edit_cursor.footprint.v[1];
        r.right  = g_mapref.x + g_edit_cursor.footprint.v[2];
        r.bottom = g_mapref.y + g_edit_cursor.footprint.v[3];
        people = CheckForPeople(&r);
        switch (people) {
        case -1:
            SetCursorError(&g_edit_cursor, 4);
            break;
        case 1:
            SetCursorError(&g_edit_cursor, 3);
            break;
        }
    }
    cost = GetObjCost(def);
    if (GetBrickCount() < cost)
        SetCursorError(&g_edit_cursor, 2);
    if (CursorIsValid(&g_edit_cursor)) {
        sq.x = (unsigned char)(g_lf_footprint.v[0] + g_mapref.x);
        sq.y = (unsigned char)(g_mapref.y + g_lf_footprint.v[1]);
        LFTrack_ProbeNeighbours(sq, &nb);
        LFTrack_DropFullNeighbours(nb);
        if (LFTrack_CountNeighbours(nb) == 0) {
            SetCursorError(&g_edit_cursor, 0xe);
        } else {
            LFRun*    run;   /* ORIGINAL BUG: unset if all four are null */
            int       mask;

            SetCursorError(&g_edit_cursor, 0xe);
            if (nb[0])
                run = nb[0]->run;
            else if (nb[1])
                run = nb[1]->run;
            else if (nb[2])
                run = nb[2]->run;
            else if (nb[3])
                run = nb[3]->run;
            LFRun_KeepOnly(run, nb);
            mask = LFTrack_NeighbourMask(nb);
            if (LFTrack_MaskIsLegal(mask, nb)) {
                LFTrack_CommitPlacement(nb, &g_edit_cursor);
                ResetCursorFootprint(&g_edit_cursor);
            }
        }
    }
    PropagateCursorStatus(&g_edit_cursor);
}

extern LFPiece* LFPiece_Alloc(void);                             /* 0x00409010 */
extern void  LFRun_AddCount(LFRun* run, int delta);              /* 0x004119a0 */
extern void  LFRun_AddPiece(LFRun* run, LFPiece* piece);         /* 0x004091f0 */
extern void  LFRun_RemovePiece(LFRun* run, LFPiece* piece);      /* 0x00409270 */
extern void  LFPiece_SetShape(LFPiece* piece, LFPiece** nb);     /* 0x00409c20 */
extern void  LFTrack_ReshapeNeighbours(LFPiece* piece,
                                       LFPiece** nb);            /* 0x004097a0 */
extern void  LFTrack_UnlinkNeighbours(LFPiece* piece,
                                      LFPiece** nb);             /* 0x0040da10 */
extern void  LFTrack_RedrawNeighbours(LFPiece* piece,
                                      LFPiece** nb);             /* 0x0040a2a0 */
extern void  AddBasicObject(RideElem* elem, const Pos* p);       /* 0x0045efe0 */
extern void  StandardRemoveObject(void* a, BPosW sq, void* c);   /* 0x0045f220 */

/* Placing one square of flume.  The piece record is allocated and stamped
 * first, then the four neighbours are probed and the piece JOINS the run of
 * the first neighbour that has one (the same chain as LFTrack_Update, with
 * the same neighbour-array fallback).  The class footprint is re-stamped
 * from the template and shrunk by one cell in both axes before the object
 * is handed to AddBasicObject, so the map object covers the drawn cell
 * rather than the template's inclusive rect. */
/* CLOSED (107/107, 321 bytes): same lever as LFTrack_Update -- the
 * neighbour chain has NO else-arm, so `run` is uninitialised when all four
 * neighbours are null (original latent bug).  VC6 then homes `run` on the
 * stack and re-fetches the array pointer after every call; the earlier
 * `volatile nb` + `run = (LFRun*)nb` fallback only approximated that and
 * cost 23 register-naming mismatches in the tail. */
// FUNCTION: LEGOLAND 0x0040c780
void LFTrack_Add(RideElem* elem, const Pos* pos)
{
    BPos      bp;
    Pos       p;
    LFPiece*  piece;

    bp.x = (unsigned char)pos->x;
    bp.y = (unsigned char)pos->y;
    p.x = bp.x;
    p.y = bp.y;
    piece = LFPiece_Alloc();
    if (piece) {
        LFPiece**          nb;
        LFPiece**          q;
        LFRun*             run;   /* ORIGINAL BUG: unset if all four are null */

        piece->def = g_lftr_def;
        piece->f28 = 0;
        piece->sq.b.x = (unsigned char)(p.x + g_lf_footprint.v[0]);
        piece->sq.b.y = (unsigned char)(p.y + g_lf_footprint.v[1]);
        LFTrack_ProbeNeighbours(bp, (LFPiece***)&nb);
        LFTrack_DropFullNeighbours(nb);
        q = nb;
        if (q[0])
            run = q[0]->run;
        else if (q[1])
            run = q[1]->run;
        else if (q[2])
            run = q[2]->run;
        else if (q[3])
            run = q[3]->run;
        LFRun_AddCount(run, 1);
        if (run) {
            q = nb;
            piece->run = run;
            LFRun_KeepOnly(run, q);
            LFRun_AddPiece(run, piece);
            g_lftr_def->footprint = g_lf_footprint;
            g_lftr_def->footprint.v[2] = g_lftr_def->footprint.v[2] - 1;
            g_lftr_def->footprint.v[3] = g_lftr_def->footprint.v[3] - 1;
            AddBasicObject(g_lftr_elem, &p);
            LFPiece_SetShape(piece, nb);
            LFTrack_ReshapeNeighbours(piece, nb);
        }
    }
}

/* Taking one square of flume away.  ORIGINAL BUG, reproduced: the run count
 * is decremented through `piece->run` BEFORE the piece pointer is checked
 * for null, so removing a square that has no piece record dereferences a
 * null pointer. */
// FUNCTION: LEGOLAND 0x0040c8d0
void LFTrack_Remove(void* a, BPosW sq, void* c)
{
    LFPiece*  piece;

    g_lftr_def->footprint = g_lf_footprint;
    g_lftr_def->footprint.v[2] = g_lftr_def->footprint.v[2] - 1;
    g_lftr_def->footprint.v[3] = g_lftr_def->footprint.v[3] - 1;
    StandardRemoveObject(a, sq, c);
    piece = (LFPiece*)LFPiece_FindAt(&sq.b);
    LFRun_AddCount(piece->run, -1);
    if (piece) {
        LFPiece** nb;

        LFTrack_ProbeNeighbours(sq.b, &nb);
        LFTrack_UnlinkNeighbours(piece, nb);
        LFTrack_RedrawNeighbours(piece, nb);
        LFRun_RemovePiece(piece->run, piece);
    }
}

/* =========================================================================
 * DRAWING A TRACK SQUARE
 *
 * The track's +0xa0 draw slot does not blit anything itself; like
 * LFPiece_Draw it fills ONE static PieceDraw record and hands it back to the
 * render list.  The tile picture comes from the "LOG FLUME IMAGE LIST" the
 * create hook loaded: the piece's shape index selects a sprite from +0x08
 * and a pair of render offsets from +0x0c / +0x10, each HALVED (the list
 * stores them in half-pixel units).
 *
 * Every piece of one run shares one animation frame (LFRun +0x1c), and the
 * draw pass forces the tile's .lls onto it with LLSStop + LLSSetFrame, so a
 * whole stretch of flume animates in lockstep rather than each square
 * free-running.  A piece with bit 2 of its flags set is skipped entirely
 * (it returns NULL rather than the record).
 * ========================================================================= */

/* The three parallel tables are indexed by a BYTE OFFSET rather than an
 * element index -- the original computes `shape * 4` once and reuses it for
 * all three, which is why they are typed as raw bases here. */
typedef struct LFImageList {
    unsigned char pad00[8];
    char*         sprites;      /* +0x08  SpriteRec*, one per shape index */
    char*         ox;           /* +0x0c  render x, in half pixels */
    char*         oy;           /* +0x10  render y, in half pixels */
} LFImageList;

extern PieceDraw     g_track_draw;                               /* 0x004c74d8 */
extern LFPiece*      LFTrack_FindPiece(const BPos* sq);          /* 0x00408f30 */
extern unsigned char LFPiece_ShapeIndex(LFPiece* piece);         /* 0x0040ad50 */

// FUNCTION: LEGOLAND 0x0040c970
PieceDraw* LFTrack_Draw(RideElem* elem, BPosW sq)
{
    LFPiece*     piece;
    LFImageList* il;
    SpriteRec*   spr;
    LLS*         lls;
    int          idx;

    g_track_draw.sq = sq.w;
    piece = LFTrack_FindPiece(&sq.b);
    if (piece) {
        if (piece->flags & 4)
            return 0;
        idx = LFPiece_ShapeIndex(piece) * 4;
        il  = g_lf_imagelist_data;
        spr = *(SpriteRec**)(il->sprites + idx);
        g_track_draw.sprite = spr;
        g_track_draw.f04 = *(int*)(il->ox + idx) >> 1;
        g_track_draw.f08 = *(int*)(il->oy + idx) >> 1;
        g_track_draw.f10 = 0;
        lls = GetLLSForSprite(spr);
        if (lls) {
            LLSStop(lls);
            LLSSetFrame(lls, piece->run->frame);
        }
        g_track_draw.sprite->flags |= 0x2000;
        g_track_draw.f10 = 0;
    }
    return &g_track_draw;
}

/* =========================================================================
 * LOG FLUME ENTRANCE -- the station.
 *
 * The entrance is the ride proper: it owns the queue, the boats and the save
 * chunk, and it is the only class in the game that fills the +0xc0 "extra"
 * slot.  Its create hook loads the whole station art set:
 *
 *   two animation sets built from the static tables at 0x004b47b8 (4 frames)
 *   and 0x004b47e8 (5 frames) -- the turning barrels;
 *   lf_barrel / lf_barrel_m / lf_barrel1 / barrelmatte  the barrel stack,
 *   with lf_barrel's .lls started immediately (LLSPlay) so the barrels are
 *   already turning when the station appears;
 *   lf_enta1_matte2, lf_sign, lf_entrance1..3 and enta3_m  the building.
 * ========================================================================= */

extern RideDef*   g_lfen_def;           /* 0x004c2b9c  LOG FLUME ENTRANCE */
extern SpriteRec* g_lfen_sprite;        /* 0x004c8d54  = def->sprite */
extern void*      g_lf_anim_a;          /* 0x004c2ae8  from the 4-frame set */
extern void*      g_lf_anim_b;          /* 0x004c2af8  from the 5-frame set */
extern LFAnimSet  g_lf_animset_a;       /* 0x004b47b8 */
extern LFAnimSet  g_lf_animset_b;       /* 0x004b47e8 */
extern SpriteRec* g_spr_barrel;         /* 0x004cbe74  "lf_barrel.lls" */
extern SpriteRec* g_spr_barrel_m;       /* 0x004cbe78  "lf_barrel_m.lls" */
extern SpriteRec* g_spr_barrel1;        /* 0x004cbe7c  "lf_barrel1.lls" */
extern SpriteRec* g_spr_barrelmatte;    /* 0x004cbe80  "barrelmatte.lls" */
extern SpriteRec* g_spr_ent_matte2;     /* 0x004cbe88  "lf_enta1_matte2.lls" */
extern SpriteRec* g_spr_sign;           /* 0x004cbe8c  "lf_sign.lls" */
extern SpriteRec* g_spr_ent1;           /* 0x004cbe98  "lf_entrance1.lls" */
extern SpriteRec* g_spr_ent2;           /* 0x004cbe90  "lf_entrance2.lls" */
extern SpriteRec* g_spr_ent3;           /* 0x004cbe94  "lf_entrance3.lls" */
extern SpriteRec* g_spr_enta3_m;        /* 0x004cbe4c  "enta3_m.lls" */
extern LFRun*     g_lf_queue;           /* 0x004cbe84  every station/run */

extern void* LFAnim_Create(LFAnimSet* set);                      /* 0x00412100 */
extern void  LFAnim_Free(void* anim);                            /* 0x00412290 */
extern void  LLSPlay(LLS* lls, int mode);                        /* 0x0047d520 */
extern void  HeapFree_w(void* p);                                /* 0x0049e4d0 */

// FUNCTION: LEGOLAND 0x0040a2e0
void LFEntrance_Create(RideElem* elem)
{
    RideDef* def = elem->data;
    LLS*     lls;

    g_lfen_def = def;
    def->flags1c |= 0x20;
    g_lfen_sprite = g_lfen_def->sprite;
    if (g_lfen_sprite)
        g_lfen_sprite->flags |= 0x2000;
    g_lf_anim_a = LFAnim_Create(&g_lf_animset_a);
    g_lf_anim_b = LFAnim_Create(&g_lf_animset_b);
    g_spr_barrel      = LoadSprite("lf_barrel.lls", 1);
    g_spr_barrel_m    = LoadSprite("lf_barrel_m.lls", 1);
    g_spr_barrel1     = LoadSprite("lf_barrel1.lls", 1);
    g_spr_barrelmatte = LoadSprite("barrelmatte.lls", 1);
    if (g_spr_barrel) {
        lls = GetLLSForSprite(g_spr_barrel);
        if (lls)
            LLSPlay(lls, g_spr_barrel->f08);
    }
    g_spr_ent_matte2 = LoadSprite("lf_enta1_matte2.lls", 1);
    g_spr_sign       = LoadSprite("lf_sign.lls", 1);
    g_spr_ent1       = LoadSprite("lf_entrance1.lls", 1);
    g_spr_ent2       = LoadSprite("lf_entrance2.lls", 1);
    g_spr_ent3       = LoadSprite("lf_entrance3.lls", 1);
    g_spr_enta3_m    = LoadSprite("enta3_m.lls", 1);
}

// FUNCTION: LEGOLAND 0x0040a410
void LFEntrance_Destroy(void)
{
    LFRun*   n;
    LFRun*   next;
    LFPiece* item;
    LFPiece* nextitem;

    if (g_spr_enta3_m)
        KillSprite(g_spr_enta3_m);
    if (g_spr_ent3)
        KillSprite(g_spr_ent3);
    if (g_spr_ent2)
        KillSprite(g_spr_ent2);
    if (g_spr_ent1)
        KillSprite(g_spr_ent1);
    if (g_spr_sign)
        KillSprite(g_spr_sign);
    if (g_spr_ent_matte2)
        KillSprite(g_spr_ent_matte2);
    if (g_spr_barrel)
        KillSprite(g_spr_barrel);
    if (g_spr_barrel_m)
        KillSprite(g_spr_barrel_m);
    if (g_lf_anim_b)
        LFAnim_Free(g_lf_anim_b);
    if (g_lf_anim_a)
        LFAnim_Free(g_lf_anim_a);
    if (g_spr_barrel1)
        KillSprite(g_spr_barrel1);
    if (g_spr_barrelmatte)
        KillSprite(g_spr_barrelmatte);
    n = g_lf_queue;
    while (n) {
        item = n->pieces;
        next = n->next;
        while (item) {
            nextitem = item->next;
            HeapFree_w(item);
            item = nextitem;
        }
        HeapFree_w(n);
        n = next;
    }
    g_lf_queue = 0;
}

extern int      g_edit_changed;         /* 0x008119b0  EditMode */
extern RideDef* g_edit_object;          /* 0x008119b8 */
extern void     DefaultCursor(EditCursorRec* c);                 /* 0x0045a390 */
extern void     SetEditCursorFootPrint(Footprint* fp);           /* 0x0045f440 */

/* The ENTRANCE's +0x8c slot is "the player picked this class in the build
 * panel": arm edit mode, publish the class, reset the cursor and give it the
 * class footprint. */
// FUNCTION: LEGOLAND 0x0040a540
void LFEntrance_Tick(void)
{
    g_edit_changed = 1;
    g_edit_object  = g_lfen_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

/* =========================================================================
 * THE ENTRANCE'S PLACEMENT PREVIEW -- three ghost cursors, not one.
 *
 * While the player is dragging a LOG FLUME ENTRANCE the update slot builds a
 * chain of three preview cursors on 0x008003f0:
 *
 *   A (0x004c8d78)  the flume connection square ABOVE the station,
 *   B (0x004c2c18)  the flume connection square BELOW it,
 *   C (0x00830fc0)  the station's own footprint, widened by three cells on
 *                   the left and one on each of the other three sides and
 *                   stamped with mode 0x1000.
 *
 * A and B both carry the FLUME's footprint (not the station's), because they
 * stand for the two track squares the station will occupy; B's copy is taken
 * from A's, already shrunk.  Their y coordinates are pushed apart by the
 * height of the flume footprint (`dy`), so the preview shows where the two
 * ends of the ride will land before the player commits.
 * ========================================================================= */

extern EditCursorRec g_lf_cursor_a;     /* 0x004c8d78  flume square, in */
extern EditCursorRec g_lf_cursor_b;     /* 0x004c2c18  flume square, out */
extern EditCursorRec g_lf_cursor_c;     /* 0x00830fc0  the station itself */

/* CLOSED (95/95, 389 bytes) by two source-order levers, from 48 mismatches:
 *  1. The cursor-C block: the four LINK-CHAIN stores (g_8003f0, a.link,
 *     b.link, c.link = 0) come FIRST in the source.  VC6 sinks constant
 *     stores to globals below the computed ones, so the emitted order is
 *     unchanged, but the zero for c.link is now defined at the top of the
 *     block and eax is reserved for it -- the loads then alternate ecx/edx
 *     exactly as the original does.
 *  2. The cursor-A/B blocks: the `dy` adjustment is a SECOND statement on
 *     the y coordinate, after the x++ (`a.y = fp1 + my; a.x++; a.y -= dy;`).
 *     VC6 merges the two y stores (the x++ store between them cannot alias)
 *     but keeps the association `(fp1 + my) - dy` / `(fp3 + my - 1) + dy`,
 *     and that changes the allocation priorities all the way back to the
 *     prologue: v[1] now takes ecx before `mode` is read, exactly as the
 *     original.  Every single-expression spelling of the y formulas
 *     canonicalises to the same 32-mismatch object.
 * The double store to a.x / b.x is real: x, then y (a load through `def`,
 * which may alias), then x again incremented. */
// FUNCTION: LEGOLAND 0x0040a930
void LFEntrance_Update(RideElem* elem, int screen, int mode)
{
    int      dy  = g_lf_footprint.v[3] - g_lf_footprint.v[1];
    RideDef* def = elem->data;

    ScreenToMapRef(screen, &g_mapref, mode);
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&def->footprint);

    g_8003f0 = &g_lf_cursor_a;
    g_lf_cursor_a.link = &g_lf_cursor_b;
    g_lf_cursor_b.link = &g_lf_cursor_c;
    g_lf_cursor_c.link = 0;
    g_lf_cursor_c.x = g_mapref.x;
    g_lf_cursor_c.y = g_mapref.y;
    g_lf_cursor_c.footprint.v[1] = g_edit_cursor.footprint.v[1] - 1;
    g_lf_cursor_c.footprint.v[0] = g_edit_cursor.footprint.v[0] + 3;
    g_lf_cursor_c.footprint.v[2] = g_edit_cursor.footprint.v[2] + 1;
    g_lf_cursor_c.footprint.v[3] = g_edit_cursor.footprint.v[3] + 1;
    g_lf_cursor_c.footprint.parts = 0;
    g_lf_cursor_c.f1828 = 0x1000;
    ResetCursorFootprint(&g_lf_cursor_c);

    g_lf_cursor_a.footprint = g_lf_footprint;
    g_lf_cursor_a.footprint.v[2] = g_lf_footprint.v[2] - 1;
    g_lf_cursor_a.footprint.v[3] = g_lf_cursor_a.footprint.v[3] - 1;
    ResetCursorFootprint(&g_lf_cursor_a);

    g_lf_cursor_a.x = g_lfen_def->footprint.v[0] + g_mapref.x;
    g_lf_cursor_a.y = g_lfen_def->footprint.v[1] + g_mapref.y;
    g_lf_cursor_a.x++;
    g_lf_cursor_a.y -= dy;
    g_lf_cursor_b.x = g_lfen_def->footprint.v[0] + g_mapref.x;
    g_lf_cursor_b.y = g_lfen_def->footprint.v[3] + g_mapref.y - 1;
    g_lf_cursor_b.x++;
    g_lf_cursor_b.y += dy;
    g_lf_cursor_b.footprint = g_lf_cursor_a.footprint;
    ResetCursorFootprint(&g_lf_cursor_b);
    ValidateCursor(&g_edit_cursor, def);
    PropagateCursorStatus(&g_edit_cursor);
}

/* =========================================================================
 * THE ENTRANCE'S +0x94 SLOT: repaint the whole run under the cursor.
 *
 * Where the piece classes' +0x94 slot repaints ONE square, the entrance's
 * repaints the entire flume belonging to the station under the mouse: it
 * looks the station up by its map square (g_lf_hover_sq), then walks its
 * piece list and renders a ghost cursor over every piece that has a
 * footprint.  Finally it splices the station-footprint ghost (cursor C) into
 * the preview chain behind cursor D, widened by the same 3/1/1/1 margin
 * LFEntrance_Update uses.
 *
 * A piece whose +0x28 is -1 is drawn with whatever footprint the cursor
 * already carries (the shared one from the previous piece) rather than its
 * own -- reproduced as written.
 * ========================================================================= */

extern EditCursorRec g_lf_cursor_d;     /* 0x004c74f8  the per-piece ghost */
extern BPos          g_lf_hover_sq;     /* 0x00667c54 */

extern LFRun* LFStation_FindAt(const BPos* sq);                  /* 0x00408ec0 */
extern void   LFPiece_GetFootprint(LFPiece* p, Footprint** fp,
                                   void** out2);                 /* 0x0040d090 */
extern void   BasicObjectDCalcCursor(void* a, void* b);          /* 0x00480bb0 */
extern void   BuildCursorPtr(EditCursorRec* c, int a, int b);    /* 0x0045f5f0 */
extern void   RenderCursor(EditCursorRec* c);                    /* 0x0045ff00 */

/* CLOSED (77/77, 294 bytes).  The last residual was the pair of dead
 * argument slots the helper's two out-pointers are homed in: the original
 * has the footprint out at [esp+4] and the dummy out at [esp+8].  No shape
 * of two uninitialised LOCALS reaches that (declaration order, block scope,
 * types, names, an out[2] array, a static __inline wrapper, a temp for
 * either address were all measured at 3 mismatches): VC6 always hands the
 * first-evaluated (rightmost) argument the lowest free slot.  What the
 * original does is pass the address of the dead second PARAMETER as the
 * dummy out (`&b`), so only `fp` needs a home and it takes the `a` slot. */
// FUNCTION: LEGOLAND 0x0040aac0
void LFEntrance_Update2(void* a, void* b)
{
    LFRun*     st;
    LFPiece*   p;
    Footprint* fp;

    BasicObjectDCalcCursor(a, b);
    DefaultCursor(&g_lf_cursor_d);
    st = LFStation_FindAt(&g_lf_hover_sq);
    if (st) {
        p = st->pieces;
        while (p) {
            if (p->f28 != -1) {
                LFPiece_GetFootprint(p, &fp, &b);   /* b is dead: used as the dummy out */
                if (fp == 0)
                    goto next;
                g_lf_cursor_d.footprint = *fp;
                g_lf_cursor_d.x = p->sq.b.x;
                g_lf_cursor_d.y = p->sq.b.y;
            }
            g_lf_cursor_d.f1828 = 0x18;
            ResetCursorFootprint(&g_lf_cursor_d);
            BuildCursorPtr(&g_lf_cursor_d, 0, 0);
            RenderCursor(&g_lf_cursor_d);
next:
            p = p->next;
        }
        g_lf_cursor_c.x = g_lf_cursor_d.x;
        g_lf_cursor_c.y = g_lf_cursor_d.y;
        g_lf_cursor_c.footprint.v[1] = g_lf_cursor_d.footprint.v[1] - 1;
        g_lf_cursor_c.footprint.v[0] = g_lf_cursor_d.footprint.v[0] + 3;
        g_lf_cursor_c.footprint.v[2] = g_lf_cursor_d.footprint.v[2] + 1;
        g_lf_cursor_c.footprint.v[3] = g_lf_cursor_d.footprint.v[3] + 1;
        g_lf_cursor_c.footprint.parts = 0;
        g_lf_cursor_c.f1828 = 0x1000;
        g_lf_cursor_c.link = g_lf_cursor_d.link;
        g_lf_cursor_d.link = &g_lf_cursor_c;
    }
}

/* The +0xc0 "extra" slot -- the only one filled in the whole game.  It
 * answers "how long is the longest log flume in the park", optionally
 * counting only runs that are COMPLETE (both ends joined to the station):
 * pass a non-zero second argument to require completeness.  This is what
 * feeds the ride's length statistic. */
// FUNCTION: LEGOLAND 0x004119c0
int LFEntrance_Extra(void* a, int complete_only)
{
    LFRun* st = g_lf_queue;
    int    best = 0;

    while (st) {
        if (st->piece_count > best) {
            if (complete_only == 0 || LFRun_IsComplete(st))
                best = st->piece_count;
        }
        st = st->next;
    }
    return best;
}

/* =========================================================================
 * THE LOG FLUME SAVE CHUNK  (+0xbc save / +0xb8 load)
 *
 * The chunk is a simple tagged list: for every station/run in g_lf_queue a
 * dword 1 followed by a 0xd4-byte copy of the run record, and a dword 0 to
 * end it.  Because the record is full of live pointers, the copy is patched
 * before it goes out:
 *
 *   +0x08, +0x0c, +0x18   piece pointers -> the piece's INDEX within this
 *                         run's own list (+0x10 is the list head, and is
 *                         itself written out unchanged as the anchor);
 *   +0x38                 replaced by LFRun_SaveState(st);
 *   +0x2c                 an animation reference, converted in place;
 *   four 0x24-byte boat records from +0x48 on, each with a piece pointer at
 *   +0x0c (converted to an index) and an animation reference at +0x00.
 *
 * LFRun_PrepareSave runs first and renumbers the run's piece list so the
 * indices are meaningful.
 * ========================================================================= */

typedef struct LFRunSave {
    unsigned char pad00[8];
    void*         f08;          /* +0x08  piece -> index */
    void*         f0c;          /* +0x0c  piece -> index */
    LFPiece*      f10;          /* +0x10  the run's piece list head */
    unsigned char pad14[4];
    void*         f18;          /* +0x18  piece -> index */
    unsigned char pad1c[0x2c - 0x1c];
    int           f2c;          /* +0x2c  animation reference */
    unsigned char pad30[8];
    int           f38;          /* +0x38  run state */
    unsigned char pad3c[0x48 - 0x3c];
    unsigned char boats[0xd4 - 0x48];  /* +0x48  four 0x24-byte boats */
} LFRunSave;                    /* 0xd4 */

typedef struct LFBoatSave {
    int   f00;                  /* +0x00  animation reference */
    unsigned char pad04[8];
    void* f0c;                  /* +0x0c  piece -> index */
    unsigned char pad10[0x14];
} LFBoatSave;                   /* 0x24 */

extern int   SaveGameWrite(const void* buf, unsigned int n);     /* 0x0047d760 */
extern void  LFRun_PrepareSave(LFRun* st, LFPiece* head);        /* 0x00410800 */
extern void* LFPiece_ToIndex(LFPiece* head, void* p);            /* 0x004107b0 */
extern int   LFRun_SaveState(LFRun* st);                         /* 0x00410910 */
extern void  LFAnim_SaveRef(void* set, int* ref);                /* 0x004123c0 */
extern int   LFAnim_SaveId(void* set, int ref);                  /* 0x004123a0 */

/* CLOSED (87/87, 282 bytes).  The last residual (the boat loop hoisting the
 * animation read above the piece-index store) was the loop FORM: the
 * original indexes the boat array (`b[i]`, i counting up, VC6 reverses it
 * into the `mov edi,4 / dec edi` down-counter and strength-reduces esi),
 * and with the indexed form VC6 keeps the store and the following load in
 * source order.  The walking-pointer form (`b = b + 0x24`) reorders them.
 * LFBoatSave must be its real 0x24 bytes for `b[i]` to stride correctly. */
// FUNCTION: LEGOLAND 0x00410930
int SaveLogFlume(void)
{
    int         zero;
    int         one;
    LFRunSave   buf;
    LFRun*      st;
    LFBoatSave* b;
    int         i;

    st   = g_lf_queue;
    one  = 1;
    zero = 0;
    while (st) {
        SaveGameWrite(&one, 4);
        LFRun_PrepareSave(st, st->pieces);
        buf = *(LFRunSave*)st;
        buf.f08 = LFPiece_ToIndex(buf.f10, buf.f08);
        buf.f0c = LFPiece_ToIndex(buf.f10, buf.f0c);
        buf.f18 = LFPiece_ToIndex(buf.f10, buf.f18);
        buf.f38 = LFRun_SaveState(st);
        LFAnim_SaveRef(g_lfen_def->fcc, &buf.f2c);
        b = (LFBoatSave*)buf.boats;
        for (i = 0; i < 4; i++) {
            b[i].f0c = LFPiece_ToIndex(buf.f10, b[i].f0c);
            b[i].f00 = LFAnim_SaveId(g_lfen_def->fcc, b[i].f00);
        }
        SaveGameWrite(&buf, 0xd4);
        st = st->next;
    }
    SaveGameWrite(&zero, 4);
    return 1;
}

extern int   SaveGameRead(void* buf, unsigned int n);            /* 0x0047d730 */
extern void* HeapAlloc_w(unsigned int n);                        /* 0x0049e4ff */
extern LFPiece* LFRun_LoadPieces(LFRun* st, int mode);           /* 0x00410a50 */
extern void  LFRun_RelinkPieces(LFPiece* head, LFPiece* head2);  /* 0x00410bb0 */
extern void  LFAnim_LoadRefs(void* set, int* refs);              /* 0x00412490 */
extern LFPiece* LFPiece_FromIndex(LFPiece* head, LFPiece* idx);  /* 0x00410b60 */
extern int   LFAnim_FromId(void* set, int id);                   /* 0x00412470 */

/* The reader for the chunk SaveLogFlume writes.  Each record allocates a
 * fresh 0xd4-byte run, reads the pieces back first (LFRun_LoadPieces returns
 * the rebuilt list head), then reads the record straight over the run and
 * turns every saved index back into a pointer.  The boat reference at +0x38
 * is an INDEX into the run's own boat array, which starts at +0x40 with a
 * stride of 36 bytes -- so it comes back as `st + idx*36 + 0x40`. */
// FUNCTION: LEGOLAND 0x00410c10
int LoadLogFlume(void)
{
    int         tag;
    LFAnimRefs  refs;
    LFRun*      st;
    LFRun*      n;
    LFPiece*    head;
    LFBoatSave* b;
    int         i;

    st  = 0;
    tag = 1;
    SaveGameRead(&tag, 4);
    while (tag) {
        if (st == 0) {
            st = (LFRun*)HeapAlloc_w(0xd4);
            g_lf_queue = st;
            st->next = 0;
        } else {
            n = (LFRun*)HeapAlloc_w(0xd4);
            st->next = n;
            st = n;
        }
        head = LFRun_LoadPieces(st, 0);
        LFRun_RelinkPieces(head, head);
        LFAnim_LoadRefs(g_lfen_def->fcc, refs.r);
        SaveGameRead(st, 0xd4);
        st->pieces = head;
        st->refs = refs;
        st->f08 = LFPiece_FromIndex(head, st->f08);
        st->f0c = LFPiece_FromIndex(st->pieces, st->f0c);
        st->f18 = LFPiece_FromIndex(st->pieces, st->f18);
        st->f38 = (char*)st + (int)st->f38 * 36 + 0x40;
        b = (LFBoatSave*)((char*)st + 0x48);
        for (i = 4; i != 0; i--) {
            b->f0c = LFPiece_FromIndex(st->pieces, (LFPiece*)b->f0c);
            b->f00 = LFAnim_FromId(g_lfen_def->fcc, b->f00);
            b = (LFBoatSave*)((char*)b + 0x24);
        }
        SaveGameRead(&tag, 4);
    }
    return 1;
}

/* The TRACK class's +0xb0 draw pass -- a two-way dispatch and nothing else.
 * A piece of kind 2 whose variant is 0 or 2 goes down one drawing path and
 * everything else down the other; both are handed the piece and the render
 * mode. */
extern void LFTrack_DrawAlt(LFPiece* p, int mode);               /* 0x0040ca60 */
extern void LFTrack_DrawNormal(LFPiece* p, int mode);            /* 0x0040cc00 */

/* CLOSED (25/25, 65 bytes).  The edx/ecx naming of the render-mode scratch
 * register in the two arms is decided by the SOURCE SHAPE of the else-arm:
 * one `if (a && (b || c)) Alt else Normal` gives ecx to Alt; a nested
 * `if (a) { if (b || c) Alt else Normal } else Normal` -- two textual
 * Normal calls that VC6 merges into one block -- gives edx to Alt and ecx
 * to the merged Normal block, as the original has it.  A switch on the
 * variant reproduces the allocation too but lowers the compares to
 * `sub ecx,0 / sub ecx,2` instead of test/cmp. */
// FUNCTION: LEGOLAND 0x0040cc50
void LFTrack_Interact(void* a, void* b, void* c, const BPos* sq,
                      void* e, int mode)
{
    LFPiece* p = LFPiece_FindAt(sq);

    if (p) {
        if (p->f18 == 2) {
            if (p->f1c == 0 || p->f1c == 2)
                LFTrack_DrawAlt(p, mode);
            else
                LFTrack_DrawNormal(p, mode);
        } else
            LFTrack_DrawNormal(p, mode);
    }
}

/* The TRACK class's +0x8c slot: run the shared tick over a COPY of the flume
 * footprint shrunk by one cell in each axis (the same 3/1 adjustment the
 * cursor code makes), not over the class definition's own. */
// FUNCTION: LEGOLAND 0x0040dbb0
void LFTrack_Tick(void)
{
    Footprint fp;
    RideDef*  def = g_lftr_def;

    fp = g_lf_footprint;
    fp.v[2] = g_lf_footprint.v[2] - 1;
    fp.v[3] = fp.v[3] - 1;
    LFPiece_TickCommon(def, &fp);
}

/* =========================================================================
 * DEMOLISHING A STATION.
 *
 * Removing a LOG FLUME ENTRANCE removes its whole run with it: the station
 * is looked up by the square being removed, and every piece on its list is
 * taken off the map through a LOCAL edit cursor (0x1834 bytes -- which is
 * why this function allocates a 0x1838-byte frame through the stack probe).
 * Each piece that was PAID for (flags bit 1) refunds nothing but charges
 * the class cost again through UseBricks, matching the way flume is billed
 * per square when it is laid.
 *
 * ORIGINAL BUG, reproduced: the cursor's y coordinate is built from the
 * flume footprint's RIGHT edge (g_lf_footprint.v[2]) where the x uses the
 * LEFT edge (v[0]).  It should almost certainly have been v[1], the top.
 * ========================================================================= */

extern void UseBricks(int cost);                                 /* 0x004578c0 */
extern void LFAnim_Release(LFAnimRefs* refs);                    /* 0x00411ed0 */
extern void LFStation_Unlink(LFRun* st);                         /* 0x00408e80 */
extern void RemoveAllBlokesFromRide(RideDef* def, BPosW sq);     /* 0x0048a2e0 */

/* RESIDUAL (11 of 102 instructions, first divergence at index 39): 102/102
 * instructions and 343/343 bytes; the whole body is exact except the
 * SCHEDULE of the 15 instructions that build the cursor's origin and copy
 * the footprint (orig 39-53).  The instruction MULTISET is identical --
 * same opcodes, same registers, same operands -- only the order differs:
 *
 *   ORIGINAL                       OURS
 *   39 mov ecx,[v2]                39 mov dl,[p->sq.b.x]
 *   40 mov dl,[p->sq.b.x]          40 mov esi,[v0]
 *   41 mov esi,[v0]                41 mov ecx,[v2]
 *   42 xor eax,eax                 42 add edx,esi
 *   43 add edx,esi                 43 xor eax,eax
 *   44 mov al,[p->sq.b.y]          44 mov [cur.x],edx
 *   45 lea edi,&cur.footprint      45 mov al,[p->sq.b.y]
 *   46 add eax,ecx                 46 add eax,ecx
 *   47 mov ecx,5                   47 mov ecx,5
 *   48 mov [cur.y],eax             48 mov [cur.y],eax
 *   49 mov eax,[g_lftr_def]        49 mov eax,[g_lftr_def]
 *   50 mov [cur.x],edx             50 lea edi,&cur.footprint
 *   51 mov dx,[p->sq]              51 lea esi,[eax+0x3c]
 *   52 lea esi,[eax+0x3c]          52 rep movsd
 *   53 rep movsd                   53 mov dx,[p->sq]
 *
 * i.e. three dependency-free ROOTS (the v2 load, `lea edi`, the p->sq word
 * load) that the original hoists to the top of their scheduling window stay
 * put in ours, and the cur.x store that the original sinks past the
 * g_lftr_def reload is emitted as soon as its value is ready.
 *
 * TWO FAMILIES, BOTH REPRODUCIBLE, NEITHER EXACT (~140 measured variants
 * over three passes):
 *
 *  (a) PLAIN STATEMENTS (this body, 11 mismatches).  Both sums are right --
 *      `add edx,esi` and `add eax,ecx`, destination = the widened byte in
 *      each -- and only the schedule above is wrong.
 *
 *  (b) A TWO-ARGUMENT INLINE ORIGIN HELPER (24 mismatches, first divergence
 *      at 46).  With
 *          static __inline void SetOrigin(EditCursorRec* c, int x, int y)
 *          { c->x = x + g_lf_footprint.v[0];
 *            c->y = y + g_lf_footprint.v[2]; }
 *          SetOrigin(&cur, p->sq.b.x, p->sq.b.y);
 *      indices 39-45 are EXACT -- the v2 load leads, `lea edi` is hoisted
 *      above the y sum, the y byte lands late -- because the helper's two
 *      arguments become expression temporaries evaluated before the body
 *      runs.  What then breaks is index 46: the y sum comes out
 *      `add ecx,eax` (destination = the GLOBAL's register) where the
 *      original has `add eax,ecx` (destination = the byte), and every
 *      instruction after it follows that one register.
 *
 * NEW RULE MEASURED THIS PASS (it is what blocks family (b)): in an inlined
 * helper body, the FIRST `param + global` sum takes the parameter temp's
 * register as its destination and the SECOND takes the GLOBAL's.  Proved
 * both ways: with the body written y-then-x (M1) the y sum becomes
 * `add edx,esi` (byte) and the x sum `add ecx,eax` (global) -- the mirror
 * image.  The original needs BOTH sums to keep the byte, so the original is
 * NOT two `param + global` statements in one inline body.  Immune to:
 * reading either global into a local inside the helper (one, the other or
 * both), operand order in either sum, a nested one-argument SetY helper for
 * the second statement, passing v0/v2 as further arguments, `unsigned char`
 * parameters (+2 insns), a `const BPos*` parameter (falls back to family
 * (a)), passing the cursor last, and putting the footprint copy inside the
 * helper before or after the stores.
 *
 * Also ruled out this pass, all still 11 (family (a)): the footprint copy
 * through a `static __inline` helper taking both Footprint*s, taking only
 * the destination, or taking the cursor and the RideDef -- none of which
 * frees `lea edi` to hoist; and every spelling of the two footprint
 * decrements above the block (`--`, `-= 1`, postfix, `+ -1`, swapped) plus
 * moving `next = p->next` after the copy, tried on the theory that the
 * Pentium scheduler's window boundary is counted in IR tuples from the
 * function start and one more tuple earlier would hoist the roots.  (The
 * decrement spellings are inert; moving `next` costs 7-18.)
 *
 * Earlier passes (unchanged, ~110 variants): statement order in every
 * permutation and operand order in both sums canonicalise to 11 (y-first
 * 12, copy-first 31-34); global-load temporaries, a `RideDef* def` local, a
 * Footprint* to the source, memcpy/#pragma intrinsic, a dst pointer: 11;
 * two-def forms (`cur.x = byte; cur.x += v0`): 71+, because cur is
 * address-taken so both stores survive; a Pos aggregate or BPosW local:
 * 36-78; a named `int y = p->sq.b.y` before the x statement gives family
 * (b)'s 39-45 with the same flipped add (24), and no spelling of that temp
 * moves it.
 *
 * WHERE TO GO NEXT.  The two families are one instruction apart in opposite
 * directions, so the answer is a source form that gets family (b)'s
 * argument-temp schedule while keeping family (a)'s forward-substituted
 * sums -- i.e. something that makes the y byte's temp live only across its
 * own statement while still being defined before the x statement.  A macro,
 * or a helper that takes the two bytes and returns the two sums rather than
 * storing them, are the untried shapes.  The bug note above (v[2] used for
 * the y offset) is confirmed by the address the original loads (0x4b4730).
 * Variants: scratchpad/logflume/sw1.py .. sw4.py (run with var.py).
 *
 * 2026-09-04 (fourth lane).  The untried shapes above are now measured and
 * the two families are shown to be THE SAME EDIT, which makes this a much
 * harder residual than "one instruction apart" suggested.
 *  - The trigger for family (b) is exactly ONE thing: giving the Y BYTE a
 *    name.  Naming the X byte alone (`bx = p->sq.b.x;` then
 *    `cur.x = bx + v[0];` with y inline) is BYTE-IDENTICAL to family (a);
 *    naming the y byte -- anywhere, by any spelling -- is family (b).  And
 *    the flipped `add` at index 46 comes from the same name: with the y sum
 *    written inline the destination is the widened byte (`add eax,ecx`),
 *    with a named operand it is the global (`add ecx,eax`).  There is no
 *    third state, so "b's schedule with a's sums" is not a source form that
 *    exists -- it is a request for VC6 to name and not name the same value.
 *  - Family (b) confirmed at 24 with indices 0..45 EXACT and byte length
 *    342/343; EVERY divergence from 46 on follows the one flipped operand
 *    order (the argument pushes at 54-58 are its downstream).
 *  - New shapes measured, all either 11 (family a) or 24 (family b):
 *    two SEPARATE one-argument inline helpers (`LfOx(x)`, `LfOy(y)`), one
 *    two-argument `LfAdd(byte, global)` helper called twice (both operand
 *    orders, and the helper's parameter order reversed), an x-only or y-only
 *    helper, both bytes AND both globals in named int locals in all three
 *    interleavings (so the sums are symbol+symbol, to test the
 *    "earliest-defined symbol wins" rule -- it does NOT: the second sum
 *    still takes the global), the y sum written `gy + by`, two-def forms on
 *    the TEMPS (`bx += v[0]; by += v[2];` and every mix -- forward
 *    substituted back), a `BPosW*`/`Footprint*` pointer to either operand, a
 *    global-only temp (gx or gy alone), a block-scoped `by`, and `by` typed
 *    `unsigned`/`long` (24), `short` (14, first=40), `unsigned char` (74).
 *    A dead second use of `by` folded away and reverted to family (a).
 *  - So the recorded rule stands and is now stated more sharply: THE
 *    DESTINATION OF A `byte + global` SUM IS THE BYTE WHEN BOTH OPERANDS ARE
 *    INLINE LOADS AND THE GLOBAL WHEN EITHER OPERAND IS A NAMED SYMBOL --
 *    except for the FIRST such sum in a statement group, which keeps the
 *    byte either way.  That is why family (a) gets both sums right and
 *    family (b) only the first.
 *
 * 2026-09-04 (fifth lane).  Still 11; the fourth lane's "the two families
 * are the same edit" conclusion is unchanged, and the residual is now
 * stated as a pure SCHEDULER problem with the add-rank question settled.
 *  - ADD-RANK CHECK.  The current recorded rank -- inline MEMORY reference
 *    (loaded into the destination, never folded) > compiler temporary >
 *    named local -- PREDICTS FAMILY (b), not the original: in
 *    `<widened byte> + g_lf_footprint.v[2]` the global is an inline memory
 *    reference (rank 1) and the widened byte is a temporary (rank 2), so
 *    rank alone says `add ecx,eax`.  The original emits `add eax,ecx`.  So
 *    at THIS site the rank rule is overridden by something else, and the
 *    empirical rule recorded above (the byte wins while BOTH operands are
 *    inline, the global wins as soon as EITHER is named) is the one to
 *    trust.  Worth carrying to the disputed named-local tie-break: this is a
 *    third context and it agrees with neither lane's tie-break.
 *  - The schedule is INVARIANT under statement order in a stronger sense
 *    than "canonicalises to 11": y-first (12) produces the SAME schedule
 *    shape with x and y merely exchanged -- the store of the FIRST sum is
 *    still emitted as soon as its value is ready (index 44) and `lea edi`
 *    still lands after the `g_lftr_def` reload (index 50).  So no
 *    permutation of these three statements can reach the original, which
 *    defers the first store to index 50 and hoists `lea edi` to 45.
 *  - Measured this round, all worse: the footprint copy placed between the
 *    two origin stores (31), before both (34), and y-first with the copy
 *    between (32) -- all three lose a byte (342/343) as well; `p->sq` read
 *    into a named `BPosW` local for the call argument (63); a redundant
 *    `(RideDef*)` cast on the copy source (byte-identical, 11).
 *  - STATE: 102/102 instructions, 343/343 bytes, IDENTICAL INSTRUCTION
 *    MULTISET, 11 of the 15 instructions in one block out of order.  Every
 *    source-level knob measured over four passes (~190 variants) leaves the
 *    schedule in one of exactly two states.  If this is to be closed it will
 *    be by whatever moves VC6's scheduling-window boundary in integer code
 *    -- and the recorded negative "upstream padding does NOT move a store's
 *    schedule slot" says that is not reachable by adding tuples ahead of the
 *    block.  Treat it as at its floor unless that negative is overturned.
 *
 * 2026-09-05 (sixth lane).  Still 11, but the fourth lane's rule -- "the
 * trigger for family (b) is giving the Y BYTE a name" -- is WRONG in a way
 * that matters, and the corrected rule points at a different (and, so far,
 * unreachable) target.
 *  - THE TRIGGER IS AN EXTRA IR TUPLE BEFORE THE X STATEMENT, not the name.
 *    Proof: `by = p->sq.b.y;` written BEFORE the x statement is family (b)
 *    (24), and the SAME declaration written AFTER the x statement (still
 *    before the y statement) is family (a) (11) and BYTE-IDENTICAL to the
 *    base.  A second proof from the other direction: moving `next = p->next`
 *    down to just before the x statement -- which costs nothing but ONE
 *    extra tuple in that window -- also hoists `lea edi,&cur.footprint` up
 *    to the original's slot (its `mov ebx,[ebp]` then lands in the window,
 *    so the variant scores 29, but the hoist is real).  So the roots the
 *    original hoists (the v[2] load, `lea edi`, `mov dx,[p->sq]`) and the
 *    deferred `cur.x` store are all bought by ONE more tuple ahead of the
 *    block, and the operand-order flip is bought SEPARATELY by the name.
 *  - CONSEQUENCE: the target is "one extra, CODE-FREE tuple before the x
 *    statement".  Every candidate measured is forward-substituted away and
 *    is byte-identical to the base: splitting either footprint decrement
 *    into `t = ..-1; ..= t;` (both, and both together), a
 *    `Footprint* fd = &cur.footprint;` pointer local at five placements
 *    (before the x statement, first in the block, between the stores, after
 *    the y store, before the decrements), and a named `gy` for
 *    `g_lf_footprint.v[2]` before or after the x statement.
 *  - Also measured and worse: an inline `SetOrigin(&cur, x, y)` helper
 *    taking the SUMS rather than the bytes, in all four combinations of
 *    parameter order x body store order (26-27, and `lea edi` hoists too
 *    far, out of the window entirely); named sum temps `ox`/`oy` stored
 *    afterwards in both store orders (26-27); a volatile byte read of the y
 *    byte (63 -- it emits `and eax,0xff` instead of the `xor/mov al`
 *    widening); a volatile read of `v[2]` (13); `BPosW s = p->sq;` used for
 *    both bytes AND the call argument (76); a `const BPos* k = &p->sq.b;`
 *    local (11, byte-identical); both bytes named with the y sum first (26)
 *    and with the x sum first (24, i.e. the "first sum keeps the byte"
 *    exception holds however the naming is spread).
 *  - The DESTINATION-SYMBOL lever does NOT reach this add.  `by = p->sq.b.y;
 *    ... by += g_lf_footprint.v[2]; cur.y = by;` -- which makes `by` the
 *    destination symbol of the sum, the one spelling recorded elsewhere as
 *    able to force `add <sym>, <mem>` -- still emits `add ecx,eax` (the
 *    global's register) and scores 24; so do `by = by + v[2]`, the fused
 *    `cur.y = by += v[2]`, and the same treatment applied to BOTH
 *    coordinates.  For a `byte + global` pair the global takes the
 *    destination as soon as the byte is a named symbol, whatever the
 *    assignment form.
 */
// WIP-FUNCTION: LEGOLAND 0x0040abf0  (89%, schedule of the origin/copy block, see note)
void LFEntrance_Remove(RideElem* elem, BPosW sq, void* c)
{
    LFRun*        st;
    EditCursorRec cur;
    LFPiece*      p;
    LFPiece*      next;

    StandardRemoveObject(elem, sq, c);
    st = LFStation_FindAt(&sq.b);
    if (st) {
        p = st->pieces;
        while (p) {
            next = p->next;
            g_lftr_def->footprint = g_lf_footprint;
            g_lftr_def->footprint.v[2] = g_lftr_def->footprint.v[2] - 1;
            g_lftr_def->footprint.v[3] = g_lftr_def->footprint.v[3] - 1;
            cur.x = p->sq.b.x + g_lf_footprint.v[0];
            cur.y = p->sq.b.y + g_lf_footprint.v[2];
            cur.footprint = g_lftr_def->footprint;
            StandardRemoveObject(g_lftr_def->fc4, p->sq, &cur);
            if (p->flags & 2)
                UseBricks(GetObjCost(g_lftr_def));
            LFRun_RemovePiece(st, p);
            p = next;
        }
        LFAnim_Release(&st->refs);
        LFStation_Unlink(st);
        g_lftr_def->f08++;
    }
    RemoveAllBlokesFromRide(elem->data, sq);
}
