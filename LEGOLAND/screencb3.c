/* LEGOLAND -- the last unnamed class callbacks from screen.c's
 * SetCustomCallbacks table (the third file; screencb.c holds the first
 * fifteen and screencb2.c the next seventeen).
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
 *   0x0041a2f0  BOATING SCHOOL    +0x90  BoatingSchool_Update
 *   0x00435150  JUNGLE CRUISE     +0x90  JungleCruise_Update
 *   0x0042d070  EARTH SLIDE RIDE  +0xb0  EarthSlide_Draw
 *   0x00435bd0  JUNGLE CRUISE     +0xb0  JungleCruise_Draw
 *   0x0042d100  EARTH SLIDE RIDE  +0xa4  EarthSlide_Create
 *   0x0042f030  RESTAURANT 1      +0xa4  Restaurant1_Create
 *   0x00432310  RESTAURANT 1      +0xb8  Restaurant1_Load
 *   0x00432400  RESTAURANT 2      +0xb8  Restaurant2_Load
 *
 * ---------------------------------------------------------------------------
 * THE FOUR SHAPES, TWO CALLBACKS EACH
 * ---------------------------------------------------------------------------
 * This file is the cleanest demonstration of the grouping method screencb2.c
 * closed 17 of 17 with: the eight bodies are FOUR PAIRS, and in every pair
 * the two callbacks are one piece of source compiled twice against different
 * globals.  Disassemble the family, group by slot, write ONE body per group,
 * then diff each sibling against it.
 *
 *   +0xb8 LOAD (RESTAURANT 1 / RESTAURANT 2) -- read the class's save chunk
 *      back as a null-terminated chain of fixed-size records.  Identical to
 *      Carousel_Load / Balloonz_Load (screencb2.c) minus the rider fix-up
 *      loop and minus the `elem` argument, which these two never read: the
 *      list head is a module global, so the class object is not needed.
 *
 *   +0xa4 CREATE (RESTAURANT 1 / EARTH SLIDE RIDE) -- the standard four
 *      create steps then the class's own resources.  RESTAURANT 1 loads its
 *      five occlusion masks and parks layer 1; EARTH SLIDE loads a .POS
 *      animation table and a .RIN 3D model and then PATCHES CONSTANTS INTO
 *      BOTH, which is how a ride's playback parameters reach the loaders
 *      without being in the files.
 *
 *   +0xb0 DRAW (EARTH SLIDE RIDE / JUNGLE CRUISE) -- the custom draw the
 *      sprite flag 0x2000 arms.  Both end in the same PrintSprite blit at
 *      the object's screen position with the render walk's `mode`.
 *
 *   +0x90 UPDATE (BOATING SCHOOL / JUNGLE CRUISE) -- the placement preview:
 *      copy the class's SAVED footprint chain onto the edit cursor, resolve
 *      the mouse to a map reference, then build ONE ghost cursor for the
 *      water/river the ride needs in front of it and chain it on.
 *
 * ---------------------------------------------------------------------------
 * HOW A WATER RIDE PREVIEWS ITS LAKE  (recovered here)
 * ---------------------------------------------------------------------------
 * The boating school and the jungle cruise are the only two classes whose
 * footprint is a CHAIN of rects rather than one, and the +0x90 slot is where
 * that chain is (re)built every frame:
 *
 *   BOATING SCHOOL   g_bs_footprint -> g_bs_dock_a -> g_bs_dock_b -> 0
 *   JUNGLE CRUISE    g_jc_area      -> g_jc_dock_a -> g_jc_dock_b -> 0
 *
 * The head of that chain is copied onto the edit cursor whole (a five-dword
 * `rep movsd` that carries `next` with it, so the cursor inherits the rest of
 * the chain by pointer), and the two jetty rects the create handler already
 * offset to the class footprint come along.  Then a SECOND cursor
 * (0x00830fc0, the same spare preview cursor logflume.c calls
 * g_lf_cursor_c) is shaped into a one-column strip immediately to the RIGHT
 * of the footprint --
 *
 *     left = right = cursor.rect.right + 1;  top / bottom unchanged
 *
 * -- given mode 0x1008 and chained on as the cursor's single child.  That
 * strip is the water the ride's dock must reach, which is why the class
 * cannot be placed with its right edge against anything solid.
 *
 * The two bodies differ in exactly four ways, all reproduced and all forced
 * by the disassembly:
 *   (a) the boating school builds the WHOLE chain and then copies; the
 *       jungle cruise copies FIRST and rebuilds the chain afterwards, so its
 *       cursor inherits the previous frame's `next` pointer (harmless -- the
 *       value never changes -- but it is a real asymmetry, and each order is
 *       exact only on its own body: swapped, they cost 32 and 46);
 *   (b) the jungle cruise clears the ghost's own `rect.next` and the boating
 *       school does not.  That fifth literal zero is what forms the
 *       function-wide zero web VC6 carries in esi there, and its absence is
 *       why the boating school's chain must be finished before the copy;
 *   (c) the jungle cruise resets both cursors' footprints in the MIDDLE (the
 *       edit cursor right after ScreenToMapRef, the ghost right after it is
 *       filled), where the boating school does both at the end;
 *   (d) the jungle cruise clears the ghost's `next` TWICE, once each side of
 *       the chain-on, and reads `elem->data` at the ValidateCursor call
 *       rather than at the top of the function.
 *
 * ---------------------------------------------------------------------------
 * CODEGEN LEVER RECOVERED HERE
 * ---------------------------------------------------------------------------
 * A `rep movsd` BETWEEN TWO LITERAL-ZERO STORES IS WHAT LETS A ZERO WEB FORM.
 * BoatingSchool_Update writes three literal zeros (two chain terminators and
 * the cursor's own `next`).  With the struct copy interleaved among them VC6
 * gathers all three into one function-wide zero web and hoists `xor esi,esi`
 * -- esi being pushed for the copy anyway -- into the slot the original uses
 * for a chain store: 32 of 46 at 210 bytes against 220.  Finishing the chain
 * BEFORE the copy leaves the three stores as immediates and the body is
 * 46/46, byte-exact.  Its sibling JungleCruise_Update has FIVE literal zeros
 * and the original does carry the zero register, which is the control: the
 * threshold sits between three and five, and where the count is under it the
 * copy's placement decides.  Same family as the recorded "zero a struct
 * payload with memset, not field-by-field".
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
    unsigned char pad00[0x1c];
    unsigned int  flags;            /* +0x1c  0x20 = tick via +0xa8,
                                     *        0x400 = ask +0xa0 for the desc */
    unsigned char pad20[0x3c - 0x20];
    Rect          footprint;        /* +0x3c the class footprint rect */
    unsigned char pad50[0x64 - 0x50];
    Spr*          sprite;           /* +0x64 build sprite */
    unsigned char pad68[0xcc - 0x68];
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
typedef struct Bloke {
    unsigned char  pad00[0x60];
    unsigned char  action;          /* +0x60 state-machine stage */
} Bloke;

typedef struct RiderNode {
    struct RiderNode* next;         /* +0x00 */
    struct RiderNode* prev;         /* +0x04 */
    Bloke*            bloke;        /* +0x08 */
    unsigned short    ride_id;      /* +0x0c packed {x,y} of the instance */
    unsigned short    pad0e;
    void*             person;       /* +0x10 */
} RiderNode;

/* ---- the save stream ---------------------------------------------------- */
extern int   SaveGameRead(void* buf, unsigned int n);         /* 0x0047d730 */
extern void* HeapAlloc_w(unsigned int size);                  /* 0x0049e4ff */


/* =========================================================================
 * THE +0xb8 LOAD SHAPE  (RESTAURANT 1 and RESTAURANT 2)
 *
 * The chunk is a null-terminated chain: an int "another record follows", then
 * the class's per-placement record verbatim, repeated.  Every record is
 * appended to the tail of the live list and a short read at either point
 * abandons the load with 0 through the ONE merged `return 0` block the
 * leading guard pulls to the top of the function.
 *
 * Neither handler reads its `elem` argument, so its slot is dead and VC6
 * gives the function a `push ecx` frame for the single `more` local -- two
 * saved registers instead of Carousel_Load's four, which is why the literal
 * zeros here are immediates rather than a hoisted zero register.
 *
 * NOTE the records are appended in FILE ORDER and the head global is only
 * written for the first one, so a load into a non-empty list would strand
 * the existing records; the destroy handler always runs first.
 * ========================================================================= */

/* RESTAURANT 1's per-square animation record (ridecb1.c: the lookup is
 * Restaurant1_FindRec 0x0042ef40). */
typedef struct RestRec {
    struct RestRec* next;           /* +0x00 */
    unsigned short  square;         /* +0x04 packed {x,y} */
    unsigned char   pad06[3];
    unsigned char   frame;          /* +0x09 the building's animation frame */
    unsigned char   pad0a[2];
} RestRec;                          /* 0x0c */

/* RESTAURANT 2's is the 0x40-byte record ridecb3.c documents field by field. */
typedef struct RestRec2 {
    struct RestRec2* next;          /* +0x00 */
    unsigned short   square;        /* +0x04 packed {x,y} */
    unsigned char    pad06[0x40 - 6];
} RestRec2;                         /* 0x40 */

extern RestRec*  g_rest1_recs;      /* 0x00616144 the per-placement list */
extern RestRec2* g_r2_recs;         /* 0x00616148 the per-placement list */

// FUNCTION: LEGOLAND 0x00432310
int Restaurant1_Load(void)
{
    RestRec* prev = 0;
    RestRec* cur;
    int      more;

    if (SaveGameRead(&more, 4) == 0)
        return 0;
    while (more != 0) {
        cur = (RestRec*)HeapAlloc_w(0x0c);
        if (SaveGameRead(cur, 0x0c) == 0)
            return 0;
        cur->next = 0;
        if (prev != 0)
            prev->next = cur;
        else
            g_rest1_recs = cur;
        prev = cur;
        if (SaveGameRead(&more, 4) == 0)
            return 0;
    }
    return 1;
}


/* -------------------------------------------------------------------------
 * 0x00432400 -- RESTAURANT 2 +0xb8: Restaurant1_Load's source again,
 * instruction for instruction; only the record size (0x40 against 0x0c) and
 * the list head differ.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00432400
int Restaurant2_Load(void)
{
    RestRec2* prev = 0;
    RestRec2* cur;
    int       more;

    if (SaveGameRead(&more, 4) == 0)
        return 0;
    while (more != 0) {
        cur = (RestRec2*)HeapAlloc_w(0x40);
        if (SaveGameRead(cur, 0x40) == 0)
            return 0;
        cur->next = 0;
        if (prev != 0)
            prev->next = cur;
        else
            g_r2_recs = cur;
        prev = cur;
        if (SaveGameRead(&more, 4) == 0)
            return 0;
    }
    return 1;
}


/* =========================================================================
 * THE +0xa4 CREATE SHAPE  (RESTAURANT 1 and EARTH SLIDE RIDE)
 *
 * Every create handler in the game opens with the same four steps:
 *
 *     def = elem->data;                 cache the ObjDef in a module global
 *     def->flags |= 0x20;               0x20 arms +0xa8 (0x400 would arm +0xa0)
 *     spr = def->sprite;                the class's build sprite (ObjDef+0x64)
 *     spr->flags |= 0x2000;             arm the custom draw in +0xb0
 *
 * Neither class overrides the draw descriptor, so both set 0x20 alone.  The
 * `|= 0x2000` comes out as `or ch,20h` between a dword load and a dword
 * store -- that is VC6 encoding an OR of 0x2000 into an `unsigned int`, not
 * evidence of a 16-bit field.
 * ========================================================================= */

extern Spr*  LoadSprite(const char* name, int mode);            /* 0x00497ab0 */
extern void  LoadMoneySFX(void);                                /* 0x00453900 */
extern void  HideLayer(Spr* sprite, int layer);                 /* 0x00497de0 */
extern void  StopLayerPlaying(Spr* sprite, int layer);          /* 0x00441f00 */
extern void* GetLLSForLayer(Spr* sprite, int layer);            /* 0x00441ea0 */
extern void  LLSSetFrame(void* lls, int frame);                 /* 0x0047d5a0 */


/* -------------------------------------------------------------------------
 * 0x0042f030 -- RESTAURANT 1 +0xa4
 *
 * Loads the five occlusion masks ridecb1.c's Restaurant1_Draw blits between
 * the customer bands, then parks layer 1 of the build sprite (the door) with
 * the three-call HideLayer / StopLayerPlaying / LLSSetFrame idiom the whole
 * game shares, and tail-calls LoadMoneySFX.
 *
 * The cached ObjDef global 0x0081cd40 is written and read ONLY inside this
 * function (a whole-binary scan finds two references, both here): the class
 * reaches its ObjDef through its `elem` argument everywhere else, so this is
 * a vestigial copy of Restaurant2_Create's g_rest2_def.  Reproduced -- it is
 * what makes the sprite fetch a re-read of the global rather than a reuse of
 * the register the store just wrote.
 * ------------------------------------------------------------------------- */

extern RideDef* g_rest1_def;           /* 0x0081cd40 write-only outside this fn */
extern Spr*     g_rest1_layers;        /* 0x0081cd2c the build sprite */
extern Spr*     g_rest1_mask_main;     /* 0x0081cd28 RestMask_Main.lls */
extern Spr*     g_rest1_mask_1aa;      /* 0x0081cd8c RestMaskLevel1aa.lls */
extern Spr*     g_rest1_mask_1;        /* 0x0081cd88 RestMaskLevel1.lls */
extern Spr*     g_rest1_mask_2;        /* 0x0081cd94 RestMaskLevel2.lls */
extern Spr*     g_rest1_mask_3;        /* 0x0081cd90 RestMaskLevel3.lls */

// FUNCTION: LEGOLAND 0x0042f030
void Restaurant1_Create(RideElem* elem)
{
    RideDef* def = elem->data;

    g_rest1_def = def;
    def->flags |= 0x20;
    g_rest1_layers = g_rest1_def->sprite;
    g_rest1_layers->flags |= 0x2000;
    g_rest1_mask_main = LoadSprite("RestMask_Main.lls", 1);
    g_rest1_mask_1aa  = LoadSprite("RestMaskLevel1aa.lls", 1);
    g_rest1_mask_1    = LoadSprite("RestMaskLevel1.lls", 1);
    g_rest1_mask_2    = LoadSprite("RestMaskLevel2.lls", 1);
    g_rest1_mask_3    = LoadSprite("RestMaskLevel3.lls", 1);
    HideLayer(g_rest1_layers, 1);
    StopLayerPlaying(g_rest1_layers, 1);
    LLSSetFrame(GetLLSForLayer(g_rest1_layers, 1), 0);
    LoadMoneySFX();
}


/* -------------------------------------------------------------------------
 * 0x0042d100 -- EARTH SLIDE RIDE +0xa4
 *
 * The only create handler in the table that PATCHES the resources it just
 * loaded.  The .RIN model gets a draw origin of (-158, -6) and its rider
 * table cleared (a null `riders` means "the slot index IS the rider index",
 * rin.c), and the .POS animation table gets four playback constants written
 * straight into fields LoadPos itself zeroes -- so the earth slide's timing
 * lives in the code, not in 3ddata\earth.pos.
 *
 * Both patches are guarded, because both loaders return 0 on a missing file,
 * and every field store re-reads the module global rather than reusing the
 * pointer already in a register: that repeated global read is what a source
 * that simply names the global at each use emits, and it is the shape here.
 *
 * The sprite guard is unique to this class as well -- RESTAURANT 1 and the
 * others dereference `def->sprite` unchecked.
 *
 * 0x006160c8 and 0x006160cc are cleared here and never read anywhere in the
 * binary (a whole-image scan finds exactly one reference each, these two
 * stores): dead per-class state, reproduced.
 * ------------------------------------------------------------------------- */

/* The .RIN 3D model (rin.c's Rin; only the fields patched here are named). */
typedef struct Rin {
    int    ox;                      /* +0x00 draw origin */
    int    oy;                      /* +0x04 */
    int*   remap;                   /* +0x08 slot -> rider index */
    int*   riders;                  /* +0x0c 0 = the slot IS the rider index */
} Rin;

/* The .POS table (loaders.c's PosTable).  LoadPos zeroes +0x14 and +0x18 and
 * never touches +0x1c / +0x20; the class fills all four. */
typedef struct PosTable {
    int    per;                     /* +0x00 items per frame */
    int    count;                   /* +0x04 frames */
    float  sx;                      /* +0x08 */
    float  sy;                      /* +0x0c */
    float  sz;                      /* +0x10 */
    int    f14;                     /* +0x14 */
    int    f18;                     /* +0x18 */
    int    f1c;                     /* +0x1c */
    int    f20;                     /* +0x20 */
} PosTable;

/* One placed slide (ridecb1.c's SlideRec); only the list head is used here. */
typedef struct SlideRec { unsigned char pad00[0x24]; } SlideRec;

extern RideDef*  g_slide_item;      /* 0x006160d0 the EARTH SLIDE class object */
extern SlideRec* g_slide_head;      /* 0x006160e8 the per-placement list */
extern PosTable* g_slide_anim;      /* 0x006160e4 its 3D rider animation */
extern Rin*      g_slide_rin;       /* 0x006160d4 the ride's 3D model */
extern Spr*      g_slide_spr1;      /* 0x006160d8 entrance matte */
extern Spr*      g_slide_spr2;      /* 0x006160e0 entrance matte 2 */
extern void*     g_slide_dead_c8;   /* 0x006160c8 written here, never read */
extern void*     g_slide_dead_cc;   /* 0x006160cc written here, never read */

extern PosTable* LoadPos(const char* name);                     /* 0x0043f660 */
extern Rin*      LoadRin(const char* name, const char* dir);    /* 0x00441ba0 */

// FUNCTION: LEGOLAND 0x0042d100
void EarthSlide_Create(RideElem* elem)
{
    RideDef* def = elem->data;

    g_slide_item = def;
    def->flags |= 0x20;
    if (g_slide_item->sprite != 0)
        g_slide_item->sprite->flags |= 0x2000;
    g_slide_head = 0;
    g_slide_anim = LoadPos("3ddata\\earth.pos");
    g_slide_rin = LoadRin("3ddata\\earthslide.rin", ".");
    if (g_slide_rin != 0) {
        g_slide_rin->ox = -158;
        g_slide_rin->oy = -6;
        g_slide_rin->riders = 0;
    }
    if (g_slide_anim != 0) {
        g_slide_anim->f14 = 0x53;
        g_slide_anim->f18 = 0xc4;
        g_slide_anim->f1c = 3;
        g_slide_anim->f20 = 0x41;
    }
    g_slide_spr1 = LoadSprite("EarthSlide Entrance Matte.lls", 1);
    g_slide_spr2 = LoadSprite("EarthSlideEntranceMatte2.lls", 1);
    g_slide_dead_c8 = 0;
    g_slide_dead_cc = 0;
}


/* =========================================================================
 * THE +0xb0 DRAW SHAPE  (EARTH SLIDE RIDE and JUNGLE CRUISE)
 *
 * The +0xb0 slot is what the sprite flag 0x2000 the create handler set arms:
 * the render walk hands the class the whole square instead of blitting the
 * build sprite itself.  Both bodies ignore the `x`/`y` the walk passes and
 * re-derive the object's screen position from the square; both end in a
 * PrintSprite with the walk's `mode` and a null context.
 * ========================================================================= */

typedef struct LLS {
    short         frame;            /* +0x00  the frame being shown */
    unsigned char pad02[0x10 - 2];
} LLS;

extern Offset GetScreenCoordsForObject(MapSquare* sq, RideDef* def); /* 0x00442cc0 */
extern LLS*   GetLLSForSprite(void* spr);                     /* 0x00441e80 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern void   IP_RenderBlokeIn3DNow(Bloke* b);                /* 0x00440010 */


/* -------------------------------------------------------------------------
 * 0x0042d070 -- EARTH SLIDE RIDE +0xb0
 *
 * Three layers, in order: the car and its passengers in 3D through the .RIN
 * model, then every queuing customer standing on this square, then one or
 * two entrance mattes over the top.
 *
 * The .RIN frame is the record's seat index, and the RANGE TEST is the ride's
 * whole animation contract: frames 11..14 are the ones where the car is
 * inside the tunnel, so the model is not drawn for them and the building's
 * own sprite hides it.  Everything outside that window is drawn.
 *
 * The first matte is conditional on the record's "car loaded" counter and
 * the second is unconditional, and each fetches the screen position with its
 * OWN call -- two textual GetScreenCoordsForObject calls, not one hoisted
 * result (the tell is that each PrintSprite's argument block is built from a
 * fresh eax:edx pair).  Note `rec` dies at the second read of the record:
 * VC6 reuses its register for the `mode` argument.
 * ------------------------------------------------------------------------- */

/* Only the two fields the draw reads (ridecb1.c has the whole record). */
typedef struct SlideDrawRec {
    unsigned char    pad00[4];
    int              loaded;        /* +0x04 cleared when the car fills up */
    unsigned char    pad08[3];
    signed char      seat;          /* +0x0b seat/frame index for the model */
} SlideDrawRec;

extern SlideDrawRec* EarthSlide_FindRec(MapSquare* sq);            /* 0x0042ce20 */
extern void RenderUsingRin(Rin* rin, int frame, RideDef* def,
                           MapSquare* sq);                         /* 0x00441d60 */
extern void RenderBlokesNotInSeats(RideDef* def, MapSquare* sq);   /* 0x00441b60 */

// FUNCTION: LEGOLAND 0x0042d070
void EarthSlide_Draw(RideElem* elem, int x, int y, MapSquare* sq,
                     void* clip, int mode)
{
    RideDef*      def = elem->data;
    SlideDrawRec* rec;
    Offset        screen;
    signed char   seat;

    rec = EarthSlide_FindRec(sq);
    if (rec != 0) {
        seat = rec->seat;
        if (seat <= 10 || seat >= 15)
            RenderUsingRin(g_slide_rin, seat, def, sq);
        RenderBlokesNotInSeats(def, sq);
        if (rec->loaded != 0) {
            screen = GetScreenCoordsForObject(sq, def);
            PrintSprite(g_slide_spr1, screen.ox, screen.oy, mode, 0);
        }
        screen = GetScreenCoordsForObject(sq, def);
        PrintSprite(g_slide_spr2, screen.ox, screen.oy, mode, 0);
    }
}


/* -------------------------------------------------------------------------
 * 0x00435bd0 -- JUNGLE CRUISE +0xb0
 *
 * Steps and repaints the river first (mode 1), renders every customer of the
 * class standing on THIS square whose stage is not 2 -- stage 2 is the one
 * spent inside the building, where the walls would have to occlude them, the
 * same test BoatingSchool_Draw uses -- and then blits the jungle mask with
 * the BUILD SPRITE's current frame copied onto it, so the mask animates in
 * step with the boat house.
 *
 * The `mov bl,2` before the rider loop is the loop-invariant `2` hoisted into
 * a callee-saved register that is pushed anyway; no source construct asks
 * for it.  The two LLS handles are separate statements for the reason
 * screencb.c records on BoatingSchool_Draw: nested, VC6 reads `lls->frame`
 * before making the second call and splits the argument cleanups.
 * ------------------------------------------------------------------------- */

extern RideDef* g_jc_def;                                     /* 0x0081cb60 */
extern Spr*     g_jc_mask;                                    /* 0x0081cb5c */
extern void     JungleCruise_UpdateRiverAnim(int mode);       /* 0x00432d00 */

// FUNCTION: LEGOLAND 0x00435bd0
void JungleCruise_Draw(RideElem* elem, int x, int y, MapSquare* sq,
                       void* clip, int mode)
{
    RideDef*   def = elem->data;
    RiderNode* r = def->riders;
    Offset     screen;
    LLS*       lls;
    LLS*       mask;

    JungleCruise_UpdateRiverAnim(1);
    while (r) {
        if (*(unsigned short*)sq == r->ride_id && r->bloke->action != 2)
            IP_RenderBlokeIn3DNow(r->bloke);
        r = r->next;
    }
    screen = GetScreenCoordsForObject(sq, def);
    lls = GetLLSForSprite(g_jc_def->sprite);
    mask = GetLLSForSprite(g_jc_mask);
    LLSSetFrame(mask, lls->frame);
    PrintSprite(g_jc_mask, screen.ox, screen.oy, mode, 0);
}


/* =========================================================================
 * THE +0x90 UPDATE SHAPE  (BOATING SCHOOL and JUNGLE CRUISE)
 *
 * The +0x90 slot runs every frame while the player is dragging the class out
 * of the build panel.  Both handlers here do the same four things: rebuild
 * the class's footprint CHAIN, copy its head onto the edit cursor, resolve
 * the mouse to a map reference, and shape the spare preview cursor into the
 * one-column water strip the ride's dock has to reach (see the file header).
 *
 * The GHOST CURSORS are a chain hanging off Cursor +0x1830: the render walk
 * follows it and draws every cursor on it, so a class that previews more
 * than its own footprint builds the chain here and clears it again next
 * frame.
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
/* The spare preview cursor; logflume.c declares the same object as
 * g_lf_cursor_c -- it is whichever class is being dragged that owns it. */
extern Cursor g_prev_cursor;                                  /* 0x00830fc0 */

#ifndef LEGOLAND_PORTABLE
extern void ScreenToMapRef(int screen, Pos* out, int mode);   /* 0x0045be90 */
#else
extern int ScreenToMapRef(int screen, Pos* out, int mode);   /* 0x0045be90 */
#endif
extern void ResetCursorFootprint(Cursor* c);                  /* 0x0045f460 */
extern void ValidateCursor(Cursor* c, RideDef* def);          /* 0x0045f810 */


/* -------------------------------------------------------------------------
 * 0x0041a2f0 -- BOATING SCHOOL +0x90
 *
 * The chain is rebuilt from the module globals BoatingSchool_Create saved
 * (ridecb5.c: 0x004cc078 the whole footprint -> 0x004cc060 dock A ->
 * 0x004cc048 dock B -> 0), and the head is copied onto the edit cursor with
 * its `next` already patched, so the whole chain comes along by pointer.
 * The footprint's own `next` MUST be written before the copy -- the five
 * dwords include it.
 *
 * CODEGEN NOTE (new lever): the WHOLE chain has to be built before the copy.
 * With `g_edit_cursor.rect = g_bs_footprint;` written between the chain
 * stores, the two remaining `next` writes plus the cursor's own join the
 * ghost's into one function-wide ZERO WEB and VC6 hoists `xor esi,esi` into
 * the slot the `g_bs_dock_a.next` store occupies in the original -- 32 of 46
 * at 210 bytes against 220.  Moving the copy after the third chain store is
 * 46/46 and byte-exact with no other change.  Same family as "zero a struct
 * payload with memset": a `rep movsd` between two zero stores is what lets
 * the web form.
 * ------------------------------------------------------------------------- */

extern Rect g_bs_footprint;         /* 0x004cc078 whole rect, next -> dock A */
extern Rect g_bs_dock_a;            /* 0x004cc060 dock A, next -> dock B */
extern Rect g_bs_dock_b;            /* 0x004cc048 dock B, next -> 0 */

// FUNCTION: LEGOLAND 0x0041a2f0
void BoatingSchool_Update(RideElem* elem, int screen, int mode)
{
    RideDef* def = elem->data;

    g_bs_footprint.next = &g_bs_dock_a;
    g_bs_dock_a.next = &g_bs_dock_b;
    g_bs_dock_b.next = 0;
    g_edit_cursor.rect = g_bs_footprint;
    g_edit_cursor.next = 0;
    ScreenToMapRef(screen, &g_edit_cursor.origin, mode);
    g_prev_cursor.origin.x = g_edit_cursor.origin.x;
    g_prev_cursor.origin.y = g_edit_cursor.origin.y;
    g_prev_cursor.rect.left = g_edit_cursor.rect.right + 1;
    g_prev_cursor.rect.top = g_edit_cursor.rect.top;
    g_prev_cursor.rect.right = g_edit_cursor.rect.right + 1;
    g_prev_cursor.rect.bottom = g_edit_cursor.rect.bottom;
    g_prev_cursor.flags = 0x1008;
    g_prev_cursor.next = 0;
    g_edit_cursor.next = &g_prev_cursor;
    ResetCursorFootprint(&g_edit_cursor);
    ResetCursorFootprint(&g_prev_cursor);
    ValidateCursor(&g_edit_cursor, def);
}


/* -------------------------------------------------------------------------
 * 0x00435150 -- JUNGLE CRUISE +0x90
 *
 * The same handler for the river: chain g_jc_area -> g_jc_dock_a ->
 * g_jc_dock_b -> 0 (ridecb9.c names the two 5x5 river blocks the create
 * handler already offset to the class footprint), copy the head onto the
 * edit cursor, and shape the same one-column strip on the spare cursor.
 *
 * Three differences from BoatingSchool_Update, all real and all reproduced:
 *
 *  (a) the copy comes FIRST here and the chain is rebuilt after it, so the
 *      cursor inherits the PREVIOUS frame's `next` -- harmless because the
 *      value never changes, but it is the opposite order to the boating
 *      school's and it is what the emission proves.  Writing the chain first
 *      (the boating school's shape) costs 46 of 47 and two instructions.
 *  (b) this body clears the ghost's own `rect.next` and the boating school
 *      does not.  That fifth zero is why the zero register the boating
 *      school's source must avoid is CORRECT here: five zero uses form the
 *      web the original has.
 *  (c) `g_prev_cursor.next = 0;` is written TWICE, once before the ghost's
 *      ResetCursorFootprint and once after the chain-on -- two identical
 *      stores four instructions apart, so the second is not a scheduling
 *      artefact.
 *
 * The ObjDef is fetched at the ValidateCursor call site rather than at the
 * top: `mode` is the first thing read out of the frame and `elem` the last,
 * which no root copy of `elem` reproduces.
 * ------------------------------------------------------------------------- */

extern Rect g_jc_area;              /* 0x00629c40 whole rect, next -> dock A */
extern Rect g_jc_dock_a;            /* 0x004b7278 { 0,  0, 4, 4} next -> B */
extern Rect g_jc_dock_b;            /* 0x004b7260 { 0, -5, 4,-1} next -> 0 */

// FUNCTION: LEGOLAND 0x00435150
void JungleCruise_Update(RideElem* elem, int screen, int mode)
{
    g_edit_cursor.rect = g_jc_area;
    g_edit_cursor.next = 0;
    g_jc_area.next = &g_jc_dock_a;
    g_jc_dock_a.next = &g_jc_dock_b;
    g_jc_dock_b.next = 0;
    ScreenToMapRef(screen, &g_edit_cursor.origin, mode);
    ResetCursorFootprint(&g_edit_cursor);
    g_prev_cursor.origin.x = g_edit_cursor.origin.x;
    g_prev_cursor.origin.y = g_edit_cursor.origin.y;
    g_prev_cursor.rect.left = g_edit_cursor.rect.right + 1;
    g_prev_cursor.rect.top = g_edit_cursor.rect.top;
    g_prev_cursor.rect.right = g_edit_cursor.rect.right + 1;
    g_prev_cursor.rect.bottom = g_edit_cursor.rect.bottom;
    g_prev_cursor.rect.next = 0;
    g_prev_cursor.flags = 0x1008;
    g_prev_cursor.next = 0;
    ResetCursorFootprint(&g_prev_cursor);
    g_edit_cursor.next = &g_prev_cursor;
    g_prev_cursor.next = 0;
    ValidateCursor(&g_edit_cursor, elem->data);
}
