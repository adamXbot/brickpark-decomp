/* LEGOLAND -- the JOUST and TEMPLE SLIDE ride callback sets.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere), and the per-ride save records / the callback-slot table live in
 * ridesave.c, whose Joust_GetInterfaces (0x00408db0) and
 * TempleSlide_GetInterfaces (0x00417a00) install everything below.
 *
 * ==========================================================================
 * WHAT A RIDE CALLBACK SET IS
 * ==========================================================================
 * Every object class in the game is a 0xd0-byte ObjDef (llidb_odf.c) built by
 * LLIDB_LoadODFData from an .ODF file. SetStandardCallbacks fills the slots at
 * +0x8c..+0xbc with generic handlers; then, for a class whose ODF asks for it,
 * SetCustomCallbacks (0x00452c20) walks a table of per-ride GetInterfaces
 * functions, each of which compares the class NAME and, on a hit, overwrites
 * the slots with that ride's own handlers. So a "ride" is not a subclass: it
 * is one ObjDef plus a set of C function pointers plus a file-static block of
 * globals that only those functions touch. Both rides here follow the same
 * layout, which is why they were reconstructed together:
 *
 *   slot   who calls it                       what it is
 *   +0x8c  the build/place UI                 SELECT FOR PLACEMENT
 *   +0x98  AddObjectToMap (0x00459af6)        PLACE ONE ON THE MAP
 *   +0x9c  RemoveObject   (0x00459d1d)        TAKE ONE OFF THE MAP
 *   +0xa0  the render walk (0x0045b940,       DRAW DESCRIPTOR OVERRIDE
 *          0x00460ba9), gated on flags&0x400
 *   +0xa4  the ODF loader (0x0047c602),       LOAD THE RIDE'S RESOURCES
 *          called straight after SetCustomCallbacks
 *   +0xa8  the per-frame class walk           PER-TICK UPDATE
 *          (0x0045b5e2), gated on flags&0x20
 *   +0xac  the ODF teardown (0x0047c6b1)      FREE THE RIDE'S RESOURCES
 *   +0xb0  the sprite draw path (0x00485ac3), CUSTOM DRAW
 *          gated on sprite->flags & 0x2000
 *   +0xb8  LoadGame                           read the ride's chunk
 *   +0xbc  SaveGame                           write the ride's chunk
 *
 * Every one of those is called with the class's LLIDB element (ObjDef+0xc4) as
 * its first argument, not the ObjDef -- the handlers re-fetch elem->data
 * themselves. The two flags that switch the optional slots on are set by the
 * +0xa4 loader itself: ObjDef->flags (+0x1c) |= 0x20 arms the per-tick update,
 * |= 0x400 arms the draw-descriptor override, and the build sprite's own
 * flags (Spr +0x10) |= 0x2000 arms the custom draw.
 *
 * ==========================================================================
 * THE RIDE-LOCAL STATE
 * ==========================================================================
 * Each ride owns one contiguous block of globals and one singly linked list of
 * per-placement records, whose head lives in that block (ridesave.c serialises
 * exactly that list):
 *
 *              JOUST                 TEMPLE SLIDE
 *   def        0x004c121c            0x004cbf80    the ObjDef
 *   sprite     0x004c1214            0x004cbf7c    def->build_sprite (+0x64)
 *   binv       0x004c1218            0x004cbfc4    the .bnv depth buffer
 *   list head  0x004c1250            0x004cbfd4    the per-placement records
 *   draw desc  0x004c1228            0x004cbf98    the +0xa0 return block
 *
 * A per-placement record is keyed by the map square the ride was built on: a
 * u16 at +0x00 that is the packed {x,y} CellRef the placement handler is given.
 * Both rides' find-by-tile helpers compare that whole u16 against *(u16*)tile,
 * so the pair is compared as one word.
 *
 *   JoustRec        0x24 bytes, next @ +0x04, key @ +0x00, +0x08 cleared
 *   TempleSlideRec  0x20 bytes, next @ +0x08, key @ +0x00
 *
 * ==========================================================================
 * BOARDING AND ALIGHTING
 * ==========================================================================
 * Riders are NOT owned by the ride module. A bloke on a ride is a RiderNode on
 * the class-wide list at ObjDef+0xcc (rides.c), added by PutBlokeInList and
 * matched to a placement by its ride_id (+0x0c) -- the same packed {x,y} key
 * the per-placement records use. So the ride's own record holds only the
 * machinery state (animation phase, timers, the playing sample), and the
 * riders are found by asking the shared list for everyone whose key matches.
 * That is why taking a ride off the map is two calls: StandardRemoveObject
 * (0x0045f220) unbuilds the footprint, and RemoveAllBlokesFromRide
 * (0x0048a2e0) evicts the riders from the class list; the ride module in
 * between only kills its own sound and frees its own record.
 */

typedef struct RiderNode RiderNode;

/* ---- the LLIDB element and the 0xd0-byte ObjDef -------------------------- */
typedef struct Spr {
    unsigned char pad00[0x10];
    unsigned int  flags;         /* +0x10  bit 0x2000 = draw via the +0xb0 slot */
} Spr;

typedef struct RideDef {
    unsigned char pad00[0x0c];
    int           base_x;        /* +0x0c the class's base map square */
    int           base_y;        /* +0x10 */
    int           f14;           /* +0x14 draw-descriptor field 1 */
    int           f18;           /* +0x18 draw-descriptor field 2 */
    unsigned int  flags;         /* +0x1c */
    unsigned char pad20[4];
    signed char   qx;            /* +0x24 queue/exit offset from the base square */
    signed char   qy;            /* +0x25 */
    unsigned char pad26[0x3c - 0x26];
    int           footprint;     /* +0x3c the class footprint rect; its first
                                  *       two ints double as the boarding spot */
    int           f40;           /* +0x40 */
    unsigned char pad44[0x64 - 0x44];
    Spr*          sprite;        /* +0x64 build-anim sprite */
    unsigned char pad68[0xcc - 0x68];
    struct RiderNode* riders;    /* +0xcc the class-wide rider list */
} RideDef;

typedef struct RideElem {
    char*    name;               /* +0x00 */
    char*    image;              /* +0x04 */
    unsigned int type_flags;     /* +0x08 */
    RideDef* data;               /* +0x0c */
} RideElem;

/* The block the +0xa0 slot returns: what the render walk builds inline from
 * ObjDef +0x64/+0x14/+0x18 when the class does NOT override it (0x0045b96b). */
typedef struct RideDrawDesc {
    Spr*           sprite;       /* +0x00 */
    int            f04;          /* +0x04 */
    int            f08;          /* +0x08 */
    unsigned short f0c;          /* +0x0c */
} RideDrawDesc;

/* The packed map square a placement is keyed by: two bytes that every list
 * walk compares as one 16-bit value. */
typedef union RideTile {
    unsigned short key;                        /* +0x00 */
    struct { unsigned char x, y; } b;
} RideTile;

/* The map position the place/remove handlers are handed (two ints). */
typedef struct Pos {
    int x;                       /* +0x00 */
    int y;                       /* +0x04 */
} Pos;

/* A placed map object: its class record sits at +0x0c. */
typedef struct RideMapObj {
    unsigned char pad00[0x0c];
    RideDef*      cls;           /* +0x0c */
} RideMapObj;

/* money.c's SoundSource: kind 2 = "a map square"; +0x04 is never initialised
 * for that kind (reproduced -- the original leaves it holding stack junk). */
typedef struct RideSoundSource {
    int kind;                    /* +0x00 */
    int f04;                     /* +0x04 */
    int x;                       /* +0x08 */
    int y;                       /* +0x0c */
} RideSoundSource;

/* ---- shared engine entry points ----------------------------------------- */
extern void  DefaultCursor(void* cursor);                    /* 0x0045a390 */
extern void  SetEditCursorFootPrint(void* src);              /* 0x0045f440 */

extern int   g_edit_changed;                                 /* 0x008119b0 EditMode */
extern RideDef* g_edit_object;                               /* 0x008119b8 */
extern char  g_edit_cursor;                                  /* 0x007febc0 EditCursor */

/* ---- JOUST globals ------------------------------------------------------- */
extern RideDef*     g_joust_def;                             /* 0x004c121c */
extern RideDrawDesc g_joust_draw;                            /* 0x004c1228 */

/* ---- TEMPLE SLIDE globals ------------------------------------------------ */
extern RideDef*     g_ts_def;                                /* 0x004cbf80 */
extern RideDrawDesc g_ts_draw;                               /* 0x004cbf98 */

/* ==========================================================================
 * +0x8c -- SELECT FOR PLACEMENT
 * Arms the build cursor for this class: flag the edit state dirty, make this
 * ObjDef the object being placed, reset the cursor and give it the class's
 * footprint (ObjDef+0x3c). Note the re-read of the global after DefaultCursor:
 * the original reloads 0x008119b8 rather than reusing the register.
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00408bc0
void Joust_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_joust_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

// FUNCTION: LEGOLAND 0x00417240
void TempleSlide_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_ts_def;
    DefaultCursor(&g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

/* ==========================================================================
 * +0xa0 -- DRAW DESCRIPTOR OVERRIDE
 * The render walk (0x0045b938) tests ObjDef->flags & 0x400; if it is set it
 * calls this slot and uses the returned block, otherwise it builds the same
 * block on its own stack from ObjDef +0x64/+0x14/+0x18. Both rides return the
 * same fields plus the caller's u16 argument, and arm the custom-draw bit on
 * the sprite on the way out.
 * ========================================================================== */

#ifdef LEGOLAND_PORTABLE
/* PORT-M12: the ObjDef +0xa0 DRAW slot is typed
 * `SpriteDesc* (*draw)(void* ctx, BPos base)` (renderview.c:304) and
 * renderview.c:1238 calls it that way, so on wasm32 the 2-byte square goes
 * INDIRECTLY -- a pointer to a shadow-stack temp -- while this body's
 * `unsigned short` is a direct i32.  Same arity, no wasm-ld warning: PORT-M10
 * s1b's silent window, in the direction PORT-M11 s1b closed for eight other
 * classes.  These ten were invisible to BOTH sweeps because the name stored in
 * the slot (`Joust_A0`, ridesave.c:155) is not the name of the
 * body, and the slot section pairs slot to body BY NAME.
 * PORT-M3's `_vc6_body` rename: VC6 compiles the matched text unchanged and
 * the portable build exports a wrapper of the slot's own shape over it. */
#define Joust_GetDrawDesc Joust_GetDrawDesc_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00408c50
RideDrawDesc* Joust_GetDrawDesc(RideElem* elem, unsigned short arg)
{
    RideDef* def = elem->data;

    g_joust_draw.sprite = def->sprite;
    g_joust_draw.f04 = def->f14;
    g_joust_draw.f08 = def->f18;
    g_joust_draw.f0c = arg;
    def->sprite->flags |= 0x2000;
    return &g_joust_draw;
}
#ifdef LEGOLAND_PORTABLE
#undef Joust_GetDrawDesc
RideDrawDesc* Joust_GetDrawDesc(RideElem* elem, RideTile base)
{
    return Joust_GetDrawDesc_vc6_body(elem, base.key);
}
#endif

#ifdef LEGOLAND_PORTABLE
/* PORT-M12: the ObjDef +0xa0 DRAW slot is typed
 * `SpriteDesc* (*draw)(void* ctx, BPos base)` (renderview.c:304) and
 * renderview.c:1238 calls it that way, so on wasm32 the 2-byte square goes
 * INDIRECTLY -- a pointer to a shadow-stack temp -- while this body's
 * `unsigned short` is a direct i32.  Same arity, no wasm-ld warning: PORT-M10
 * s1b's silent window, in the direction PORT-M11 s1b closed for eight other
 * classes.  These ten were invisible to BOTH sweeps because the name stored in
 * the slot (`TempleSlide_A0`, ridesave.c:331) is not the name of the
 * body, and the slot section pairs slot to body BY NAME.
 * PORT-M3's `_vc6_body` rename: VC6 compiles the matched text unchanged and
 * the portable build exports a wrapper of the slot's own shape over it. */
#define TempleSlide_GetDrawDesc TempleSlide_GetDrawDesc_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00417300
RideDrawDesc* TempleSlide_GetDrawDesc(RideElem* elem, unsigned short arg)
{
    RideDef* def = elem->data;

    g_ts_draw.sprite = def->sprite;
    g_ts_draw.f04 = def->f14;
    g_ts_draw.f08 = def->f18;
    g_ts_draw.f0c = arg;
    def->sprite->flags |= 0x2000;
    return &g_ts_draw;
}
#ifdef LEGOLAND_PORTABLE
#undef TempleSlide_GetDrawDesc
RideDrawDesc* TempleSlide_GetDrawDesc(RideElem* elem, RideTile base)
{
    return TempleSlide_GetDrawDesc_vc6_body(elem, base.key);
}
#endif

/* ==========================================================================
 * THE PER-PLACEMENT RECORD LISTS
 * Every built copy of the ride gets one record, keyed by the packed {x,y} map
 * square it sits on. Both rides use the same three helpers: add-to-head,
 * find-by-square and unlink-and-free. The find walks the list comparing the
 * WHOLE key word against *(u16*)tile, so the x/y pair is compared as one
 * 16-bit value.
 * ========================================================================== */

void* memset(void*, int, unsigned int);

extern void* HeapAlloc_w(unsigned int size);                 /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                            /* 0x0049e4d0 */

/* JoustRec is PACKED: the record has an int at +0x12, so the whole 0x24-byte
 * layout only comes out right at pack(1). */
#pragma pack(push, 1)
typedef struct JoustRec {
    RideTile         tile;       /* +0x00 the map square this copy sits on */
    unsigned short   pad02;
    struct JoustRec* next;       /* +0x04 */
    void*            sample;     /* +0x08 the playing sample (not saved) */
    int              f0c;        /* +0x0c */
    unsigned char    jousters;   /* +0x10 how many riders are committed */
    unsigned char    seated;     /* +0x11 how many spectators are seated */
    unsigned char    seat[6];    /* +0x12 the six stand seats (0 = free) */
    unsigned char    horse[2];   /* +0x18 the two horses (0 = free) */
    unsigned char    next_horse; /* +0x1a the arena CYCLE COUNTER, 0..0x3f --
                                  *       the +0xa8 handler's second phase
                                  *       ticks it and evaluates horse 0 at 0
                                  *       and horse 1 at 0x20.  The name is
                                  *       kept because phase 1 also feeds it
                                  *       through Joust_ByteIsSet to pick a
                                  *       horse index; see the +0xa8 notes. */
    unsigned char    frame;      /* +0x1b the arena animation frame */
    int              f1c;        /* +0x1c */
    int              f20;        /* +0x20 */
} JoustRec;
#pragma pack(pop)

typedef struct TempleSlideRec {
    RideTile              tile;  /* +0x00 the map square this copy sits on */
    unsigned short        pad02;
    int                   f04;   /* +0x04 phase counter */
    struct TempleSlideRec* next; /* +0x08 */
    unsigned int          flags; /* +0x0c bit0 = running */
    int                   lane[4];  /* +0x10 one slot per slide lane, 0 = free */
} TempleSlideRec;

extern JoustRec*       g_joust_head;                         /* 0x004c1250 */
extern TempleSlideRec* g_ts_head;                            /* 0x004cbfd4 */

/* The two tiny record mutators: +0x0c bit 0 is the "running" flag and +0x04
 * the phase counter that TempleSlide's update ticks. */

// FUNCTION: LEGOLAND 0x00416f90
void TempleSlide_SetRunning(TempleSlideRec* rec)
{
    rec->flags |= 1;
}

// FUNCTION: LEGOLAND 0x00417130
void TempleSlide_ResetRecord(TempleSlideRec* rec)
{
    rec->f04 = 0;
    rec->flags &= ~1u;
}

/* The two arrays are cleared with memset even though the record was just
 * memset whole: VC6's rep stosd is not a kill, so the redundant stores survive
 * -- and the 6-byte and 2-byte memsets are what produce the original's two
 * `lea` base registers and its second zero register, which is how the seat and
 * horse arrays were identified in the first place. */

// FUNCTION: LEGOLAND 0x00407970
JoustRec* Joust_AddRecord(RideTile* tile)
{
    JoustRec* rec = (JoustRec*)HeapAlloc_w(sizeof(JoustRec));

    if (rec != 0) {
        memset(rec, 0, sizeof(JoustRec));
        rec->tile.key = tile->key;
        rec->next = g_joust_head;
        rec->f0c = 0;
        rec->jousters = 0;
        rec->seated = 0;
        memset(rec->seat, 0, sizeof(rec->seat));
        memset(rec->horse, 0, sizeof(rec->horse));
        rec->next_horse = 0;
        rec->frame = 0;
        rec->f1c = 0;
        rec->f20 = 0;
        g_joust_head = rec;
    }
    return rec;
}

/* SOLVED (both FindRecord bodies, 16/16 index-for-index).  The original's
 * shape is `mov ecx,[esp+4] / mov dx,[eax] / cmp dx,[ecx]` repeated in the
 * peeled first test and in the loop: the RECORD key goes to a register and the
 * TILE key is the compare's MEMORY operand, re-read every iteration.  Plain C
 * cannot produce it -- with no store anywhere in the function VC6 always
 * hoists the loop-invariant `tile->key` into a register and compares against
 * the record in memory (15 instructions, 8/16).  Three separate facts, each
 * measured, are needed and they compose:
 *   1. THE VOLATILE OPERAND TAKES THE REGISTER, the other stays in memory.  So
 *      volatile on the tile alone gives the right ORDER and the WRONG ROLES;
 *      volatile on the record alone widens the compare to 32 bits (VC6 forces
 *      a volatile `unsigned short` through an integer temporary) and is worse.
 *   2. WITH BOTH SIDES VOLATILE the LEFT operand takes the register.  Writing
 *      the record on the left therefore restores the original's roles AND the
 *      16-bit compare -- 14/16, everything right except that VC6 emits the
 *      record read before the `mov ecx,[esp+4]` that materialises the tile
 *      pointer, because a volatile read is pinned in source order and the
 *      plain parameter load sinks to its first use.
 *   3. THE FIX FOR THAT ORDER is a self-assignment of the parameter through a
 *      volatile pointer-to-pointer, `tile = *(RideTile volatile* volatile*)&tile;`
 *      placed inside the null guard.  It is a volatile READ of the parameter's
 *      own stack home, so it is pinned there in source order and forces
 *      `mov ecx,[esp+4]` to be emitted at exactly that point; the write back
 *      to the same home is dead and elided, costing nothing.  16/16.
 * A LOCAL copy of the parameter does not work in its place: with `t` a local
 * fed by the volatile read the roles flip back (the tile takes the register
 * again), so the compare must name the PARAMETER itself.  Measured dead in
 * passing: `RideTile*` as `void*` with a `*(unsigned short*)` read, 16-bit
 * bitfields on either side, casts, a hand-peeled first test, a goto loop and
 * the rotated `while ((rec = rec->next) != 0)` walk -- all hoist the key.
 * The volatiles are semantic no-ops here (single-threaded, no aliasing); they
 * exist only to defeat two VC6 optimisations the original's build did not
 * apply. */
// FUNCTION: LEGOLAND 0x00407a20
JoustRec* Joust_FindRecord(RideTile volatile* tile)
{
    JoustRec* rec = g_joust_head;

    if (rec != 0) {
        /* Two no-op volatile levers, both needed to stop VC6 collapsing the
         * original's per-iteration reads (see the note above): the self-
         * assignment forces the tile pointer to be materialised HERE (the
         * original loads [esp+4] before the first record read), and the two
         * volatile-qualified key reads keep the record key in dx with the
         * tile key as the compare's memory operand instead of hoisting it. */
        tile = *(RideTile volatile* volatile*)&tile;
        while (((JoustRec volatile*)rec)->tile.key != tile->key) {
            rec = rec->next;
            if (rec == 0)
                return 0;
        }
        return rec;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x00416ec0
void TempleSlide_AddRecord(RideTile* tile)
{
    TempleSlideRec* rec = (TempleSlideRec*)HeapAlloc_w(sizeof(TempleSlideRec));

    if (rec != 0) {
        memset(rec, 0, sizeof(TempleSlideRec));
        rec->tile.key = tile->key;
        rec->next = g_ts_head;
        g_ts_head = rec;
        TempleSlide_ResetRecord(rec);
    }
}

// FUNCTION: LEGOLAND 0x00416f60
TempleSlideRec* TempleSlide_FindRecord(RideTile volatile* tile)
{
    TempleSlideRec* rec = g_ts_head;

    if (rec != 0) {
        /* Same pair of no-op volatile levers as Joust_FindRecord. */
        tile = *(RideTile volatile* volatile*)&tile;
        while (((TempleSlideRec volatile*)rec)->tile.key != tile->key) {
            rec = rec->next;
            if (rec == 0)
                return 0;
        }
        return rec;
    }
    return 0;
}

/* Unlink one record and free it. The head case is tail-duplicated (its own
 * free + ret); the walk stops at the record whose `next` is the target, and
 * the `if (p)` after the loop is the original's redundant re-test of the
 * cursor -- it can only be null on the break path, which jumps straight to the
 * free. Note the walk dereferences the head without a null check: removing a
 * record when the list is empty would fault, but the callers only ever pass a
 * record they just found on the list.
 *
 * SOLVED (both RemoveRecord bodies, 32/32 index-for-index).  The original's
 * walk contains a genuinely REDUNDANT load: the loop-condition block does
 * `mov esi,[ecx+4]` (node->next, the compare value) AND `lea eax,[ecx+4]`
 * (the link), and the body then re-reads the SAME location through the link,
 * `mov ecx,[eax]`, instead of reusing esi.  Every plain spelling lets VC6
 * forward the first load into the second and collapses the body to 25-27
 * instructions, so the 32-instruction shape is only reachable by defeating
 * that forwarding.  Two independent levers, both needed:
 *   1. ONE volatile read, on the LINK deref in the body
 *      (`node = *(JoustRec* volatile*)link;`), not on the record and not on
 *      the link's declared type.  A `JoustRec* volatile*` link variable also
 *      gives 32 instructions but then VC6 uses the lea's result as the base
 *      of the condition's load (`lea ecx,[eax+4] / mov esi,[ecx]`) instead of
 *      the original's displacement-then-lea pair.  With the link declared
 *      plain and only the body's deref cast volatile, the condition keeps
 *      `mov esi,[ecx+4]` and the lea stands on its own, exactly as the
 *      original has it.
 *   2. DECLARATION ORDER decides the register roles.  `link` must be declared
 *      FIRST and seeded from the GLOBAL (`&g_joust_head->next`), with `node`
 *      declared after it (also from the global).  That makes the link the
 *      first value defined in the else block, so it takes eax and the node
 *      takes ecx -- the original's assignment.  Seeding the link from `node`
 *      (`&node->next`), or declaring the node first, defines the node first
 *      and swaps the pair (10-11 mismatches, the floor of ~40 earlier
 *      variants).  The two spellings are semantically identical because the
 *      head is read once and not stored between them.
 * Note the walk still dereferences the head without a null check: removing a
 * record from an empty list would fault.  Reproduced -- the callers only ever
 * pass a record they just found on the list. */
// FUNCTION: LEGOLAND 0x00407a50
void Joust_RemoveRecord(JoustRec* rec)
{
    if (g_joust_head == rec) {
        g_joust_head = rec->next;
    } else {
        /* The walk is a link-pointer walk; the ONE volatile read is what stops
         * VC6 collapsing the original's redundant second load (see above).
         * `link` must be declared BEFORE `node` and seeded from the global, not
         * from `node`: that is what puts the node in ecx and the link in eax,
         * as the original has them. */
        JoustRec** link = &g_joust_head->next;
        JoustRec*  node = g_joust_head;
        while (*link != rec) {
            node = *(JoustRec* volatile*)link;
            if (node == 0)
                break;
            link = &node->next;
        }
        if (node)
            node->next = rec->next;
    }
    HeapFree_w(rec);
}

// FUNCTION: LEGOLAND 0x00416f00
void TempleSlide_RemoveRecord(TempleSlideRec* rec)
{
    if (g_ts_head == rec) {
        g_ts_head = rec->next;
    } else {
        /* Same shape as Joust_RemoveRecord. */
        TempleSlideRec** link = &g_ts_head->next;
        TempleSlideRec*  node = g_ts_head;
        while (*link != rec) {
            node = *(TempleSlideRec* volatile*)link;
            if (node == 0)
                break;
            link = &node->next;
        }
        if (node)
            node->next = rec->next;
    }
    HeapFree_w(rec);
}

/* Drain the whole list -- the tail of Joust's +0xac teardown. */

// FUNCTION: LEGOLAND 0x00407ab0
void Joust_FreeAllRecords(void)
{
    while (g_joust_head)
        Joust_RemoveRecord(g_joust_head);
}

/* ==========================================================================
 * +0x98 -- PLACE ONE ON THE MAP
 * The generic placer builds the footprint; the ride then opens a record for
 * the square it was dropped on. Joust additionally clears the record's sample
 * slot -- through the pointer AddRecord returned, with no null check, so an
 * allocation failure faults (reproduced).
 * ========================================================================== */

#ifndef LEGOLAND_PORTABLE
extern void AddBasicObject(void* obj, Pos* pos);             /* 0x0045efe0 */
#else
extern void AddBasicObject(void* ll_obj, void* ll_pos, void* ll_ctx);             /* 0x0045efe0 */
#define AddBasicObject(_a1, _a2) AddBasicObject((_a1), (_a2), 0)
#endif

// FUNCTION: LEGOLAND 0x004079e0
void Joust_Place(void* obj, Pos* pos)
{
    RideTile tile;

    tile.b.x = (unsigned char)pos->x;
    tile.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    Joust_AddRecord(&tile)->sample = 0;
}

// FUNCTION: LEGOLAND 0x004172d0
void TempleSlide_Place(void* obj, Pos* pos)
{
    RideTile tile;

    tile.b.x = (unsigned char)pos->x;
    tile.b.y = (unsigned char)pos->y;
    AddBasicObject(obj, pos);
    TempleSlide_AddRecord(&tile);
}

/* ==========================================================================
 * +0x9c -- TAKE ONE OFF THE MAP
 * Three steps, in this order: drop the ride's own record (Joust first fades
 * out the sample it had sourced at that map square), unbuild the footprint,
 * then evict every rider the class still has parked on that square. The tile
 * is the by-value second argument, and its home slot doubles as the RideTile
 * the find helpers are handed.
 * ========================================================================== */

#ifndef LEGOLAND_PORTABLE
extern void StandardRemoveObject(void* obj, unsigned int tile, void* ctx);   /* 0x0045f220 */
extern void RemoveAllBlokesFromRide(RideDef* cls, unsigned int tile);        /* 0x0048a2e0 */
#else
/* PORT-M11: both take the packed map square as a 2-byte aggregate BY VALUE --
 * objmap2.c:983 `BPos bp` (and the ObjClass +0x9c slot at objmap2.c:99) and
 * rides.c:400 `RideTile tile`.  On x86 cdecl that is the same pushed dword as
 * this `unsigned int`, so the byte gates never moved; on wasm32 the aggregate
 * is passed INDIRECTLY (a pointer to a shadow-stack temp) against the scalar's
 * direct i32, with the same arity, so nothing in the link or the type tests
 * can see it (PORT-M10 s1b).  `RideTile` is this file's own spelling of that
 * square and the value is packed at the call site. */
extern void StandardRemoveObject(void* obj, RideTile tile, void* ctx);       /* 0x0045f220 */
extern void RemoveAllBlokesFromRide(RideDef* cls, RideTile tile);            /* 0x0048a2e0 */
static __inline RideTile ll_ridetile(unsigned int v)
{
    RideTile t;
    t.key = (unsigned short)v;
    return t;
}
#define StandardRemoveObject(_o, _t, _c) \
    StandardRemoveObject((_o), ll_ridetile((unsigned int)(_t)), (_c))
#define RemoveAllBlokesFromRide(_c, _t) \
    RemoveAllBlokesFromRide((_c), ll_ridetile((unsigned int)(_t)))
#endif
extern void UnSourceAndFadeAllSamplesFromSource(RideSoundSource* src, int fade); /* 0x00496c80 */

/* Fade out whatever this ride was playing at one map square. The whole thing
 * has to be an INLINED helper taking the two coordinates as scalars: that is
 * what makes VC6 read both map-square bytes at the call site before touching
 * the sound-source struct, and lets the struct live in the inline-expansion
 * temporary pool the way the original's frame has it. Writing the same stores
 * against a named local (however the coordinates are routed) sinks one or two
 * of them ahead of the argument pushes. */
static __inline void FadeSamplesAtMapSquare(int x, int y)
{
    RideSoundSource src;

    src.kind = 2;
    src.x = x;
    src.y = y;
    UnSourceAndFadeAllSamplesFromSource(&src, -200);
}

#ifdef LEGOLAND_PORTABLE
/* PORT-M11: a +0x9c remove handler.  The slot hands the map square over as a
 * 2-byte aggregate BY VALUE (objmap2.c:99, called objmap2.c:1895), which is a
 * POINTER on wasm32; the matched body reads the same dword as an
 * `unsigned int` and takes its ADDRESS for the record lookup.  Renamed for the
 * portable build with a twin of the slot's own shape exported over it. */
#define Joust_Remove Joust_Remove_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00407ad0
void Joust_Remove(void* obj, unsigned int tile, void* ctx)
{
    JoustRec* rec = Joust_FindRecord((RideTile*)&tile);

    if (rec) {
        FadeSamplesAtMapSquare(rec->tile.b.x, rec->tile.b.y);
        rec->sample = 0;
        Joust_RemoveRecord(rec);
    }
    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(((RideMapObj*)obj)->cls, tile);
}
#ifdef LEGOLAND_PORTABLE
#undef Joust_Remove
void Joust_Remove(void* obj, RideTile tile, void* ctx)
{
    Joust_Remove_vc6_body(obj, tile.key, ctx);
}
#endif

#ifdef LEGOLAND_PORTABLE
/* PORT-M11: the same, for the +0x9c slot's shape -- see above. */
#define TempleSlide_Remove TempleSlide_Remove_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00417280
void TempleSlide_Remove(void* obj, unsigned int tile, void* ctx)
{
    TempleSlideRec* rec = TempleSlide_FindRecord((RideTile*)&tile);

    if (rec)
        TempleSlide_RemoveRecord(rec);
    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(((RideMapObj*)obj)->cls, tile);
}
#ifdef LEGOLAND_PORTABLE
#undef TempleSlide_Remove
void TempleSlide_Remove(void* obj, RideTile tile, void* ctx)
{
    TempleSlide_Remove_vc6_body(obj, tile.key, ctx);
}
#endif

/* ==========================================================================
 * +0xa4 / +0xac -- LOAD AND FREE THE RIDE'S RESOURCES
 * The ODF loader calls +0xa4 immediately after installing the callbacks, and
 * the class teardown calls +0xac just before unloading the object library and
 * unlinking the ObjDef. Between them they own every asset only this ride uses:
 * its extra sprites, its .bnv depth buffer, its FX (sample) list, and -- for
 * Joust -- the whole per-placement record list.
 *
 * +0xa4 is also where the ride ARMS its optional callback slots:
 *     ObjDef->flags |= 0x20      -> the per-frame walk will call +0xa8
 *     ObjDef->flags |= 0x400     -> the render walk will call +0xa0
 *     build_sprite->flags |= 0x2000 -> the draw path will call +0xb0
 * Joust sets 0x420 (both), Temple Slide only 0x20, which is exactly why the
 * render walk builds Temple Slide's draw descriptor inline and asks Joust for
 * one. Both set 0x2000 on the sprite, so both draw through +0xb0.
 * ========================================================================== */

/* An FX list entry: {wav name, loaded sample, flags}. Load_FXList takes the
 * table and its entry count and fills in the sample pointers. */
typedef struct FXEntry {
    const char* name;            /* +0x00 */
    void*       sample;          /* +0x04 */
    int         flags;           /* +0x08 */
} FXEntry;

extern void  Load_FXList(FXEntry* list, int count);          /* 0x00496dd0 */
extern void  Kill_FXList(FXEntry* list, int count);          /* 0x00496e30 */
extern void* LoadSprite(const char* name, int flag);         /* 0x00497ab0 */
#ifndef LEGOLAND_PORTABLE
extern void  KillSprite(void* sprite);                       /* 0x00497bd0 */
#else
extern int KillSprite(void* sprite);                       /* 0x00497bd0 */
#endif
extern void* LoadBinV(const char* name);                     /* 0x0044dc90 */
extern void  FreeBinV(void* bnv);                            /* 0x0044dd60 */
extern void  HideLayer(Spr* sprite, int layer);              /* 0x00497de0 */
extern void  StopLayerPlaying(Spr* sprite, int layer);       /* 0x00441f00 */
extern void* GetLLSForLayer(Spr* sprite, int layer);         /* 0x00441ea0 */
extern void* GetLLSForSprite(Spr* sprite);                   /* 0x00441e80 */
extern void  LLSSetFrame(void* lls, int frame);              /* 0x0047d5a0 */

extern FXEntry g_joust_fx[1];        /* 0x004b4688  "Joust Horses.wav" (the count lines 707/753 pass) */
extern Spr*    g_joust_sprite;       /* 0x004c1214  ObjDef->build_sprite */
extern void*   g_joust_binv;         /* 0x004c1218  Zbuffers\joustride.bnv */
extern void*   g_joust_zspr;         /* 0x004c1210  z_joust.lls (copy) */
extern void*   g_joust_zspr2;        /* 0x004c1240  z_joust.lls */
extern void*   g_joust_fmask;        /* 0x004c1244  Joust_fmask.lls */
extern void*   g_joust_specr;        /* 0x004c1248  Joust_SpecR_m.lls */
extern void*   g_joust_specl;        /* 0x004c124c  Joust_SpecL_m.lls */

// FUNCTION: LEGOLAND 0x00407b50
void Joust_LoadResources(RideElem* elem)
{
    Load_FXList(g_joust_fx, 1);
    g_joust_def = elem->data;
    g_joust_def->flags |= 0x420;
    g_joust_sprite = g_joust_def->sprite;
    g_joust_sprite->flags |= 0x2000;
    g_joust_fmask = LoadSprite("Joust_fmask.lls", 1);
    g_joust_specr = LoadSprite("Joust_SpecR_m.lls", 1);
    g_joust_specl = LoadSprite("Joust_SpecL_m.lls", 1);
    g_joust_zspr2 = g_joust_zspr = LoadSprite("z_joust.lls", 1);
    g_joust_binv = LoadBinV("Zbuffers\\joustride.bnv");
    HideLayer(g_joust_sprite, 1);
    StopLayerPlaying(g_joust_sprite, 1);
    LLSSetFrame(GetLLSForLayer(g_joust_sprite, 1), 0);
}

extern Spr*  g_ts_sprite;            /* 0x004cbf7c  ObjDef->build_sprite */
extern void* g_ts_binv;              /* 0x004cbfc4  Zbuffers\tempslide.bnv */
extern void* g_ts_zspr;              /* 0x004cbfd0  z_tempslide.lls */
extern void* g_ts_matte;             /* 0x004cbf78  tempslide_matte.lls */
extern void* g_ts_objsamples;        /* 0x004cbfb8 */
extern int   g_ts_rider_dx;          /* 0x004cbfc8 */
extern int   g_ts_rider_dy;          /* 0x004cbfcc */
extern int   g_ts_exit_dx;           /* 0x004cbf88 */
extern int   g_ts_exit_dy;           /* 0x004cbf8c */

// FUNCTION: LEGOLAND 0x00417150
void TempleSlide_LoadResources(RideElem* elem)
{
    g_ts_def = elem->data;
    g_ts_def->flags |= 0x20;
    g_ts_sprite = g_ts_def->sprite;
    g_ts_sprite->flags |= 0x2000;
    g_ts_zspr = LoadSprite("z_tempslide.lls", 1);
    g_ts_binv = LoadBinV("Zbuffers\\tempslide.bnv");
    GetLLSForSprite(g_ts_def->sprite);   /* result discarded (original) */
    g_ts_rider_dx = 0;
    g_ts_rider_dy = 1;
    g_ts_exit_dx = 0xd;
    g_ts_exit_dy = 0x5d;
    g_ts_matte = LoadSprite("tempslide_matte.lls", 1);
    g_ts_objsamples = g_ts_binv;
}

// FUNCTION: LEGOLAND 0x00408c00
void Joust_FreeResources(void)
{
    Kill_FXList(g_joust_fx, 1);
    KillSprite(g_joust_fmask);
    KillSprite(g_joust_specr);
    KillSprite(g_joust_specl);
    FreeBinV(g_joust_binv);
    KillSprite(g_joust_zspr2);
    Joust_FreeAllRecords();
}

// FUNCTION: LEGOLAND 0x00417200
void TempleSlide_FreeResources(RideElem* elem)
{
    g_ts_def = elem->data;
    if (g_ts_matte)
        KillSprite(g_ts_matte);
    KillSprite(g_ts_zspr);
    FreeBinV(g_ts_binv);
}

/* ==========================================================================
 * TEMPLE SLIDE LANES (the boarding side of the ride)
 * A Temple Slide record carries four lane slots at +0x10; a non-zero slot
 * means that lane is occupied. Only lanes 0 and 3 are ever offered -- the
 * chute has two entrances -- and if neither is free the rider is put on lane 0
 * anyway. Whenever a lane changes the ride re-derives the instance's
 * "don't let anyone else on" bit (RideInstance flags bit 1, rides.c) from
 * whether ANY of the four slots is still free.
 * ========================================================================== */

extern void Ride_SetFlagToNotLetAnyoneOn(RideTile* tile);    /* 0x00442fa0 */
extern void Ride_ClearFlagToNotLetAnyoneOn(RideTile* tile);  /* 0x00443000 */

// FUNCTION: LEGOLAND 0x00417340
void TempleSlide_UpdateFullFlag(RideTile* tile)
{
    TempleSlideRec* rec = TempleSlide_FindRecord(tile);
    int i;

    for (i = 0; i < 4; ++i) {
        if (rec->lane[i] == 0) {
            Ride_ClearFlagToNotLetAnyoneOn(tile);
            return;
        }
    }
    Ride_SetFlagToNotLetAnyoneOn(tile);
}

// FUNCTION: LEGOLAND 0x00417380
int TempleSlide_TakeLane(RideTile* tile)
{
    TempleSlideRec* rec = TempleSlide_FindRecord(tile);
    int free_lanes[4];
    int count;
    int lane;

    if (rec) {
        count = 0;
        if (rec->lane[0] == 0)
            free_lanes[count++] = 0;
        if (rec->lane[3] == 0)
            free_lanes[count++] = 3;
        if (count != 0) {
            lane = free_lanes[0];
            rec->lane[lane] = 1;
            TempleSlide_UpdateFullFlag(tile);
            return lane;
        }
        rec->lane[0] = 1;
        TempleSlide_UpdateFullFlag(tile);
        return 0;
    }
    return -1;
}

/* ==========================================================================
 * +0xb0 -- THE CUSTOM DRAW
 * The render-list walker (0x00485ac3) calls this instead of blitting the
 * class's sprite whenever the build sprite carries flag 0x2000, handing it
 *   (elem, screen x, screen y, &map square, clip rect, blit mode)
 * with the clip rect already installed (so argument 5 is dead everywhere).
 *
 * Temple Slide's draw is the two-population shape every ride with a queue
 * uses. Bloke flag 0x80 (+0x62) separates them:
 *   - riders WITHOUT it are still walking/queueing and are drawn immediately,
 *     underneath the ride, then the "matte" sprite is blitted over them so
 *     the chute occludes anyone behind it;
 *   - riders WITH it are ON the slide: the ride positions each one itself
 *     (its own +0x3c/+0x3e offset, plus the two view-adjusted global offsets,
 *     plus the object's screen origin) and submits it to a private
 *     depth-sorted list, which is flushed at the very end so the people on
 *     the slide are drawn in depth order ON TOP of the matte.
 * The two global offset pairs are set by the +0xa4 loader: (0,1) is applied
 * to both the on-screen and the local position, (0xd,0x5d) only to the
 * on-screen one -- the second is the chute's exit point in screen space.
 * ========================================================================== */

typedef struct Offset {
    int ox;                      /* +0x00 */
    int oy;                      /* +0x04 */
} Offset;

typedef struct Person3D {
    unsigned char pad00[0x1c];
    Offset        screen;        /* +0x1c where the person is drawn */
    Offset        local;         /* +0x24 the person's own offset pair */
    void*         zsprite;       /* +0x2c the ride's depth sprite while riding */
    int           f30;           /* +0x30 1 while the ride positions the person */
    unsigned char pad34[8];
    float         depth;         /* +0x3c */
} Person3D;

typedef struct Bloke {
    unsigned char  pad00[4];
    Person3D*      person;       /* +0x04 */
    unsigned char  pad08[6];
    unsigned short state;        /* +0x0e low-level AI state (0 = idle) */
    unsigned char  pad10[0x24 - 0x10];
    Pos            target;       /* +0x24 walk target, 24.8 */
    unsigned char  pad2c[0x35 - 0x2c];
    unsigned char  b35;          /* +0x35 */
    unsigned char  seat;         /* +0x36 which slide lane this rider took */
    unsigned char  pad37[0x3c - 0x37];
    short          ride_dx;      /* +0x3c per-rider offset inside the ride */
    short          ride_dy;      /* +0x3e */
    unsigned char  pad40[4];
    unsigned short saved_speed;  /* +0x44 animation speed parked over the ride */
    unsigned char  pad46[0x54 - 0x46];
    void*          bnvpath;      /* +0x54 the BNV path the rider is riding */
    unsigned char  pad58[0x60 - 0x58];
    unsigned char  action;       /* +0x60 this ride's state-machine step */
    unsigned char  pad61;
    unsigned short flags;        /* +0x62 8 = on this ride, 0x80 = riding */
    unsigned char  pad64[4];
    Pos            world;        /* +0x68 world position, 24.8 */
    unsigned char  pad70[3];
    unsigned char  new_dir;      /* +0x73 */
    unsigned char  b74;          /* +0x74 */
    unsigned char  pad75[0x7f - 0x75];
    unsigned char  speed;        /* +0x7f */
    unsigned char  pad80[0x98 - 0x80];
    unsigned char  path[0x14];   /* +0x98 CalcMoveLine scratch */
} Bloke;

/* A rider slot on the class-wide list at ObjDef+0xcc. */
typedef struct RiderNode {
    struct RiderNode* next;      /* +0x00 */
    struct RiderNode* prev;      /* +0x04 */
    Bloke*            bloke;     /* +0x08 */
    unsigned short    ride_id;   /* +0x0c the packed map square it is using */
    unsigned short    pad0e;
    struct RideOwner* owner;     /* +0x10 */
} RiderNode;

typedef struct RideOwner {
    unsigned char pad00[0x20];
    int           key;           /* +0x20 the depth key for the render list */
} RideOwner;

typedef struct RenderList {
    void* head;                  /* +0x00 */
} RenderList;

extern Offset GetScreenCoordsForObject(RideTile* sq, RideDef* item); /* 0x00442cc0 */
extern void   AdjustOffsetForViewMode(Offset* o);                    /* 0x00442d30 */
extern void   AdjustBlokePosition(Offset* p);                        /* 0x00442d60 */
extern void   IP_RenderBlokeIn3DNow(Bloke* b);                       /* 0x00440010 */
extern int    PrintSprite(void* s, int x, int y, int mode, void* ctx);/* 0x004853a0 */
extern void   RenderItems_New(void);                                 /* 0x00442e90 */
extern void   AddBlokeToRenderList(RenderList* l, RiderNode* r, int key); /* 0x00442f20 */
extern void   RenderBlokeList(RenderList* l);                        /* 0x00442f70 */

extern RenderList g_ts_blokelist;    /* 0x004cbf84 */

/* CLOSED (2026-09-03, 116/120 -> 120/120).  The last four instructions were
 * the operand order of the two four-term sums `b->ride_dx + rider.ox +
 * exit.ox + screen.ox`: the original adds `rider` before `exit`, VC6 here
 * added `exit` first, and no permutation, parenthesisation, temporary,
 * declaration order, scope, name, type, array/struct spelling or pointer cast
 * moved it (all measured, ~40 variants this round on top of the previous
 * rounds').  The rule finally recovered: VC6 SP3 canonicalises a commutative
 * sum of independent memory leaves by the leaf's FIRST APPEARANCE IN THE
 * FUNCTION'S IL (per symbol+offset, not per symbol), LATER-appearing leaf
 * FIRST; frame homes are allocated on a different key, so swapping the two
 * AdjustOffsetForViewMode blocks flips the sum order without moving a slot.
 * In this source `rider.ox/oy` are first mentioned (stored) before
 * `exit.ox/oy`, so `exit` sorted first.  The dead block-scoped reads of
 * `exit.ox` and `exit.oy` before the rider loop emit no code but give those
 * two leaves the earlier ids, and both sums come out in the original's
 * order.  Reading only `exit.ox` fixed only the x sum (2 left), which is how
 * the per-leaf granularity was established; placing the reads before
 * GetScreenCoordsForObject also re-sorted `screen` and cost 4 the other way. */
// FUNCTION: LEGOLAND 0x00416fa0
void TempleSlide_Draw(RideElem* elem, int x, int y, RideTile* sq,
                      void* clip, int mode)
{
    RideDef*   item = elem->data;
    Offset     rider;
    Offset     exit;
    Offset     screen;
    RiderNode* r;

    TempleSlide_FindRecord(sq);          /* result unused (original) */
    screen = GetScreenCoordsForObject(sq, item);

    r = item->riders;
    while (r) {
        if (sq->key == r->ride_id && !(r->bloke->flags & 0x80))
            IP_RenderBlokeIn3DNow(r->bloke);
        r = r->next;
    }
    PrintSprite(g_ts_matte, screen.ox, screen.oy, mode, 0);
    RenderItems_New();
    g_ts_blokelist.head = 0;

    {   /* dead reads: they only order the sum operands below (see note) */
        int d0 = exit.ox;
        int d1 = exit.oy;
    }
    r = item->riders;
    while (r) {
        if (sq->key == r->ride_id && (r->bloke->flags & 0x80)) {
            Bloke*    b = r->bloke;
            Person3D* p;

            rider.ox = g_ts_rider_dx;
            rider.oy = g_ts_rider_dy;
            AdjustOffsetForViewMode(&rider);
            exit.ox = g_ts_exit_dx;
            exit.oy = g_ts_exit_dy;
            AdjustOffsetForViewMode(&exit);

            p = b->person;
            p->local.ox = b->ride_dx;
            p->local.oy = b->ride_dy;
            p->local.ox += exit.ox;
            p->local.oy += exit.oy;
            AdjustBlokePosition(&p->local);

            p->screen.ox = b->ride_dx + rider.ox + exit.ox + screen.ox;
            p->screen.oy = b->ride_dy + rider.oy + exit.oy + screen.oy;
            AdjustBlokePosition(&p->screen);

            AddBlokeToRenderList(&g_ts_blokelist, r, r->owner->key);
        }
        r = r->next;
    }
    RenderBlokeList(&g_ts_blokelist);
}

/* ==========================================================================
 * +0xa8 -- THE PER-TICK UPDATE
 * The frame loop walks the whole ObjDef chain (0x0045b5dc) and, for every
 * class whose flags carry 0x20, calls this slot with the class element. The
 * handler then walks the class's own rider list (ObjDef+0xcc) and advances
 * each rider one step of a small state machine kept in the bloke's +0x60
 * byte -- so "the ride's update" is really "one step of every customer of
 * every copy of this ride".
 *
 * A rider whose low-level AI is still busy (Bloke+0x0e != 0, i.e. walking) is
 * skipped: the ride only acts between walks. Each rider's map square is the
 * packed {x,y} at RiderNode+0x0c, which is also the key of the per-placement
 * record, so the same two bytes identify the copy of the ride, its record and
 * its lanes.
 *
 * THE TEMPLE SLIDE STATE MACHINE (Bloke+0x60)
 *   0  join: mark "on this ride" (+0x62 bit 8), claim a lane, and walk to the
 *      lane's mouth -- lanes 2/3 climb round the back (+0x380,-0x280) and go
 *      to step 2, lanes 0/1 walk to the front (+0x80, five tiles up) and jump
 *      straight to step 3.
 *   1  idle (nothing; the step exists so the machine can be parked).
 *   2  walk to the boarding spot the ODF names (+0x3c/+0x40 of the class,
 *      biased by the square), then step 3.
 *   3  MOUNT: work out where the rider is on screen, hand the 3D person the
 *      ride's z-sprite, and build a BNV path ("manbox01".."manbox04", one per
 *      lane) that carries it down the chute. Step 4.
 *   4  RIDE: advance the BNV path a frame. Past the lane's "walk" frame the
 *      rider's walk animation is started and its speed is forced to 0x18;
 *      past the lane's "end" frame (or when the path runs out) the path is
 *      freed and the machine goes to step 5. In between the rider is held on
 *      frame 0.
 *   5  ALIGHT: stop riding (+0x62 bit 0x80 off), drop the z-sprite, restore
 *      the saved animation speed, and walk from the lane's exit offset to the
 *      class's queue/exit square (+0x24/+0x25). Step 6.
 *   6  LEAVE: release the lane, take the rider off the class list and clear
 *      the "on this ride" bit.
 * ========================================================================== */

extern int   CalcMoveLine(Pos from, Pos to, void* path);     /* 0x00480740 */
extern int   NewDirForAction(Bloke* b, unsigned char dir);   /* 0x004833d0 */
extern void  BlokeSetFrame(Bloke* b, int frame);             /* 0x00440870 */
extern void  BlokeWalkAnim(Bloke* b);                        /* 0x00440910 */
extern void  RemoveBlokeFromRide(RideDef* item, RiderNode* r);/* 0x0048a100 */
extern void  GetTileDimensions(int* out_w, int* out_h);      /* 0x00460540 */
extern short Get_XScroll(void);                              /* 0x004615f0 */
extern short Get_YScroll(void);                              /* 0x00461600 */
extern float GetUnitDepth(float near_z, float far_z);        /* 0x0044de50 */
/* NewBNVPath's sixth parameter is a THREE-int position, not a Pos: the never-
 * written dword above the seed local is its z (mechrides.c's four BNV rides
 * carry the same evidence, 12 bytes apart between two seed locals). */
typedef struct BnvPos {
    int x;                   /* +0x00 */
    int y;                   /* +0x04 */
    int z;                   /* +0x08 never stored by this ride */
} BnvPos;

/* The 8-byte object case 3's frame spills sx into; only .x is ever written,
 * and VC6 lifetime-colours it onto the (dead there) rider-cursor home, so the
 * upper dword is the frame's second never-referenced slot. */
typedef struct SpillPair {
    int x;
    int y;
} SpillPair;

extern void* NewBNVPath(void* bin, int tag, const char* name, float near_z, float far_z, BnvPos* pos); /* 0x00484c20 */
extern void  BNVPath_SetDFrame(Bloke* b, void* path, int dframe); /* 0x004850b0 */
extern int   UpdateBlokeFromBNVPath(Bloke* b, void* path);   /* 0x00484cd0 */
extern int   BNVPath_GetDFrame(void* path);                  /* 0x00484ff0 */

typedef struct MapConfig {
    unsigned char  pad00[0x20];
    unsigned short ox;           /* +0x20 render origin */
    unsigned short oy;           /* +0x22 */
} MapConfig;

extern MapConfig*   g_map_cfg;                               /* 0x004bcbf4 lpConfig -- a POINTER to the config (mov eax,[0x4bcbf4]; mov di,[eax+0x20]) */
extern const char*  g_ts_path_names[];                       /* 0x004b4f08 manbox01..04 */
extern const signed char g_ts_walk_frame[];                  /* 0x004b4f18 per lane */
extern const signed char g_ts_end_frame[];                   /* 0x004b4f1c per lane */

/* Free one lane again and re-derive the instance's "full" bit. */

// FUNCTION: LEGOLAND 0x00417400
void TempleSlide_ReleaseLane(int lane, RideTile* tile)
{
    TempleSlideRec* rec = TempleSlide_FindRecord(tile);

    if (rec)
        rec->lane[lane] = 0;
    TempleSlide_UpdateFullFlag(tile);
}

/* NOTE (SUPERSEDED IN PART, kept for the reasoning): the earlier rounds got all
 * 347 instructions and the block layout but had the frame 8 bytes short and the
 * map square in ebx where the original has it in ebp.  BOTH of those are FIXED
 * now -- see "FRAME SOLVED" below -- so ignore the old claims that the local
 * area is 0x28 and that no declaration accounts for the two holes. */
/* FRAME MAP, RE-DERIVED EXACTLY (this round; the earlier analysis above it
 * had three of the homes wrong -- `ofs` is NOT pooled with `th`).  Let F be
 * esp after `sub esp,0x30`, so the locals are F+0x00..F+0x2f, the return
 * address is F+0x30 and the argument F+0x34; the four callee-saved pushes then
 * put esp at F-0x10, and every `[esp+N]` in the body must be read with the
 * outstanding ARGUMENT pushes of the call it sits inside also subtracted
 * (case 3 accumulates seventeen of them before its single `add esp,0x44`).
 * Doing that gives, with every reference accounted for:
 *     F+0x00  the rider cursor `r`   (written at entry before the pushes,
 *             read at the loop top, rewritten from `next` at the bottom, and
 *             read by case 6 for RemoveBlokeFromRide)
 *     F+0x04  NEVER REFERENCED  <-- the missing slot
 *     F+0x08  tw          (&tw is the FIRST GetTileDimensions argument)
 *     F+0x0c  th
 *     F+0x10  next
 *     F+0x14  ofs.ox      (case 5's four-way switch writes F+0x14/F+0x18)
 *     F+0x18  ofs.oy
 *     F+0x1c  screen.ox   (BOTH halves are homed and re-read; ours keeps .ox
 *     F+0x20  screen.oy    in ebx and elides the store)
 *     F+0x24  pos.x       (&pos is NewBNVPath's last argument)
 *     F+0x28  pos.y
 *     F+0x2c  NEVER REFERENCED -- accounted for by declaring `pos` as a THREE
 *             int vector, which is good evidence NewBNVPath takes a 12-byte
 *             position and not an 8-byte pair
 *     F+0x34  the ARGUMENT slot, overwritten with elem->data at entry and used
 *             as the `def` home for the rest of the body
 * So the frame splits cleanly into a SCALAR run F+0x00..F+0x13 (five 4-byte
 * homes) and an AGGREGATE run F+0x14..F+0x2f (ofs 8 + screen 8 + pos 12 = 0x1c
 * bytes, in declaration order ascending).
 *
 * FRAME SOLVED (2026-09-04), by the two levers the mechrides.c lane derived on
 * its four BNV-ride _Activate callbacks, which carry the identical residual:
 *   - F+0x2c: `pos` is a THREE-int vector (`BnvPos`), not a `Pos`; NewBNVPath's
 *     sixth parameter is a 12-byte position whose z this ride never writes.
 *   - F+0x04 and the dead `mov [esp+0x20],ebp` at 0x004175d2: they are ONE
 *     8-byte spilled object of which only `.x` is ever written.
 *     `SpillPair spill;` plus `*(volatile int*)&spill.x = sx;` right after sy
 *     is computed reproduces both -- VC6 lifetime-colours the pair onto the
 *     rider cursor's home (dead inside case 3) and its second dword becomes
 *     the phantom at F+0x04.  Nothing else reserves a scalar-pool home: unused
 *     locals of every type are dropped and an array goes to the aggregate pool.
 * With both in place the prologue is `sub esp,0x30`, EVERY displacement in the
 * body matches, and the loop head's register assignment comes out right too
 * (ebx = def->base_x then ty, ebp = sq, esi = b, edi = tx) once the block
 * layout below is also fixed. */
/* EARLIER ROUND (semantic fix + block-layout study; audit 317 -> 329 -> 231).
 * 1. lpConfig (0x004bcbf4) is a POINTER to the map config (`mov edx,[4bcbf4]
 *    / mov di,[edx+0x20]`), as mechrides.c/ridecb3.c already have it; the
 *    old struct-form read was a semantic bug.  Fixing it alone moved the
 *    audit from 317 to 329 (the register cascade shifts, the frame does not).
 * 2. Case 0 / the shared CalcMoveLine tail.  The original lays out arm A
 *    (lane > 1, +0x380/-0x280) falling THROUGH into the tail T at 0x4174d7,
 *    arm B (lane <= 1, action = 3) after T, case 2 jumping BACK to T's first
 *    reload (0x41758f -> 0x4174d7) and case 5 jumping back to T's `call`
 *    (0x417851 -> 0x4174ee) with its own pushes in which target.y is
 *    FORWARDED (`push ecx`) and target.x/world.* reloaded.  Measured facts
 *    (scratchpad/joust/tsu_v*.py, ~60 variants + synthetic probes in
 *    scratchpad/joust/syn/):
 *    - A `goto move` label block is ALWAYS placed after its LAST goto source
 *      in layout (after case 5 with two gotos, after case 2 with one, after
 *      A only when the goto sources are textually before case 0 -- which
 *      also moves their blocks before case 0, since case blocks follow
 *      textual order).  A Duff-style `if (0) { case 2: ... }` normalises to
 *      the same thing.  So case 2's jump to T's first reload cannot come
 *      from a goto in a 0..6 textual switch, yet a textual copy of the call
 *      in case 2 always forwards the last-stored target field (synthetic
 *      probe: every spelling forwards one of x/y) and would merge only from
 *      the `call`.  Unresolved.
 *    - Identical IR suffixes are merged by an IR-level pass into the LAST
 *      copy in layout (a textual copy in cases 0, 2 and 5 makes A and case 2
 *      jump forward into case 5); a post-codegen cross-jump merges an
 *      identical MACHINE-CODE suffix into the EARLIER block (synthetic F1:
 *      case 0's copy jumps into case 2's `call`), which is what the
 *      original's case 5 -> T shows.
 *    - Case 5's forwarded `push ecx` is the store order world.x, world.y,
 *      target.x, target.y with (qx+x)<<8 / (qy+y)<<8 computed ONCE (gx/gy
 *      temps; the aliasing re-read of def->qx after the world.x store
 *      otherwise recomputes them): synthetic table in
 *      scratchpad/joust/syn/gen_s2.py, only wx,wy,tx,ty (or wx,wy,ty,tx)
 *      gives `st104 st36 st108 st40 ld36 / ld108 push ecx / ld104`.
 *    The form shipped here (if/else in case 0 with the tail after the join,
 *    case 2 with its own textual copy, case 5 `goto move` with the world
 *    stores first) is semantically identical and scores best on the strict
 *    gate (231, no ESCAPES); the form I believe is the original's (case 2
 *    goto, case 5 textual copy with gx/gy temps) scores 322-329 because
 *    the tail lands after case 2 and the whole register cascade shifts.
 * 3. FRAME: SOLVED this round -- see "FRAME SOLVED" above. */
/* ROUND OF 2026-09-04 (second pass).  281 -> 74 strict, and the block layout
 * and the shared-tail merge are now the original's.  READ THE METRICS, NOT
 * THE STRICT COUNT: with a block displaced the index-for-index count is
 * actively misleading here (a store swap in case 5 scored 165 strict at one
 * point and was structurally much worse).  Use scratchpad/laneG/sc.py and
 * regions.py -- register+offset-blind LCS ("both") and the count of ORIGINAL
 * indices inside a differing region:
 *
 *     metric                       start    now
 *     audit strict                  281      74
 *     reg+offset-blind LCS          286     331   (of 347)
 *     orig indices in a bad region   61      16
 *     ESCAPES                       yes      no
 *     bytes                        1119    1121   (orig 1122)
 *
 * FOUR THINGS CLOSED THEM.
 * 1. A REAL STRUCT BUG, not a codegen question: `Person3D` carried a bogus
 *    `pad2c[4]` between `local` (+0x24..+0x2c) and `zsprite`, so zsprite, f30
 *    and depth were all 4 bytes too high.  The original stores `[eax+0x2c]`,
 *    `[edx+0x30]` and `fstp [eax+0x3c]`; we were emitting +0x30/+0x34/+0x40.
 *    (Joust_Draw only touches `screen` and `local`, so it never saw it.)
 * 2. THE CROSS-JUMP SURVIVOR, SOLVED -- and it is a new, transferable lever.
 *    The original keeps `call CalcMoveLine` in the shared block T (case 0's
 *    tail) and case 5 ends `jmp` into it; we were hosting the merged copy in
 *    case 5, because VC6's identical-suffix merge keeps the LAST copy in
 *    source order.  The cure is to write the outer busy-AI guard as a
 *    `goto` to a label at the loop's continue point instead of an
 *    `if (b->state == 0) { .. }` block:
 *        if (b->state != 0) goto endsw;
 *        { switch (b->action) { ... } }
 *      endsw: ;
 *        r = next;
 *    An explicit label at the join flips the merge host from the LAST copy to
 *    the FIRST -- indices 61..74 then match exactly and the 13-instruction
 *    displacement disappears (258 -> 77 strict, robl 316 -> 329).  Exactly
 *    equivalent and byte-identical: `goto endsw;` in place of `break` in
 *    case 2's tail, in case 5's, or in both, with the label after the switch
 *    or at the loop bottom.  NOT equivalent: the goto in case 0's copy as
 *    well (139), in all three copies (inert -- it is the ASYMMETRY between
 *    the first copy and the others that decides it), or `goto` in T and
 *    case 2 (inert).  The guard form is shipped because it is the only one
 *    of these that reads as ordinary source.
 *    Ruled out before that was found (all inert or worse): a `static
 *    __inline` helper holding the tail called from any subset of the three
 *    sites; `if (dir) { }` fold breakers; `b->action = b->action + 1`,
 *    `(unsigned char)(3 + (dir >> 5))`, a case-local `dir5`; `default:
 *    break;` first or last; `case 1:` moved or deleted (byte-identical --
 *    its jump-table entry already points at the switch end); five if/else
 *    shapes for case 0.
 * 3. CASE 3 IS THE mechrides.c BNV SHAPE, and BOTH halves are needed:
 *      screen = GetScreenCoordsForObject(sq, def);
 *      wy = b->world.y;            <- between the two calls, y BEFORE x
 *      wx = b->world.x;
 *      GetTileDimensions(&tw, &th);
 *      sx2 = (wx - wy) * tw >> 9;  <- ONE web per axis...
 *      sy2 = (wx + wy) * th >> 9;
 *      *(volatile int*)&spill.x = sx2;
 *      sx2 += g_map_cfg->ox - Get_XScroll();   <- ...continued in place
 *      sy2 += g_map_cfg->oy - Get_YScroll();
 *    The compound assignment must be used on BOTH axes; the y-only form is
 *    worth nothing and the pair is worth +22 robl and clears ESCAPES.  With
 *    it the loop head does NOT flip, so the older note's "any extra live
 *    value flips ebx = sq / ebp = ty" was true only of the spellings that
 *    need a separate `sx`/`sy` pair.  `b->flags |= 0x80;` belongs at the END
 *    of that block (16 positions swept twice, on both baselines).
 * 4. CASE 5's `(qx + b.x) << 8` CSE on BOTH axes (`sx`/`sy` reused from case
 *    3), which the previous round rejected because only the one-axis form
 *    had been tried; with (3) in place it costs nothing and case 5 now
 *    reproduces the original's forwarded `push ecx` and its whole block.
 *    Plus `unsigned char sp = (unsigned char)b->saved_speed;` cached at the
 *    top of case 5: it claims the byte register before the `b->person` load
 *    and puts the whole `switch (b->seat)` head in the original's registers
 *    (6 differing indices -> 2; the residual is that the load is then five
 *    slots earlier than the original's).
 *
 * WHAT IS LEFT: 16 differing original indices, ALL in case 3 (orig 129-189),
 * and all schedule rather than structure -- the original interleaves the two
 * `Get_?Scroll` results, the `g_ts_zspr` load and the two GetUnitDepth
 * constant pushes into the rider_dx/dy divisions where we group them
 * differently, and it computes `wx + wy` before `wx - wy` (its `wy` lives in
 * edi where ours lives in ebp).  Measured inert on this baseline: the two
 * sums swapped (robl 323), the two scroll adds swapped (327), the four `-=`
 * in three pairings, the two `pos` stores swapped (326), `wx` read before
 * `wy` (ties), spilling sy2 instead of sx2 (ties), `sx2 = delta + sx2` and
 * every other non-compound spelling of the two scroll chains (all normalise
 * to the same object), combining the two subtractions per axis, int locals
 * for screen.ox/oy or for the rider halves, `sx2 + sx2` for the doubling,
 * and 30 permutations of the block's five trailing statements (the best is
 * worth one index and needs `b->flags |= 0x80;` after the GetUnitDepth call,
 * which is not a credible source order, so it is not shipped).
 * The remaining single byte (1121 vs 1122) is one `[esp+N]` displacement.
 * Tooling: scratchpad/laneG/sc.py, regions.py; scratchpad/laneE/grun.py runs
 * a variants file and prints all of these metrics per variant. */
/* ROUND OF 2026-09-04 (third pass).  74 -> 72 strict, 15 differing original
 * indices (regions.py "both": 24 max-counted).  Tooling for this round is in
 * scratchpad/laneL/ (run.py = one compile per variant with all four metrics,
 * show.py = regenerate one variant and list it side by side, regions.py).
 * TWO SMALL CLOSES, both from re-running a sweep the previous round had done
 * on an older baseline (the standing "re-run permutation searches after any
 * structural change" rule paid twice):
 *   A. ALL 120 orders of case 3's five trailing statements, not the 30 the
 *      last round managed: the winner moves `b->flags |= 0x80;` BELOW the two
 *      person stores (pos.x, pos.y, zsprite, f30, flags).  robl 331 -> 332,
 *      74 -> 73.  Every order that puts either `pos` store anywhere but first
 *      costs 30+ indices and ESCAPES, so the two `pos` stores really are the
 *      head of that group.
 *   B. Case 5: `b->flags &= ~0x80;` OUTSIDE the `sp` block, i.e.
 *          b->flags &= ~0x80;
 *          { unsigned char sp = (unsigned char)b->saved_speed;
 *            b->person->zsprite = 0; b->person->f30 = 0; b->speed = sp; }
 *      The cached `sp` is still what claims the byte register (removing it
 *      costs 4 strict and 4 structural), but each statement it is moved BELOW
 *      pushes its `mov al,[esi+0x44]` one slot later, towards the original's
 *      276; moving it below BOTH person stores loses the register instead
 *      (76/robl 330), so 272 is as far as it goes.  73 -> 72.
 *
 * THE BIG FINDING, NOT SHIPPED: THE X CHAIN IS TWO WEBS, and that is almost
 * certainly the original's source.  Written
 *      px = (wx - wy) * tw >> 9;          <- fresh pre-scroll name
 *      sy2 = (wx + wy) * th >> 9;
 *      *(volatile int*)&spill.x = px;
 *      sx2 = g_map_cfg->ox - Get_XScroll() + px;   <- delta wins the dest
 *      sy2 += g_map_cfg->oy - Get_YScroll();       <- compound, web wins
 * the body becomes 1122 bytes -- EXACTLY the original's, from 1121 -- and
 * case 3's indices 119..147 match the original instruction for instruction:
 * the load order, `mov ebp,ebx / add ebx,edi / imul ebx,th / sub ebp,edi /
 * imul ebp,tw / sar / sar`, BOTH `movsx` placements and both `mov edx,[cfg]`
 * loads.  The mechanism is now understood end to end:
 *   - The commutative add's destination is the delta TEMPORARY only when the
 *     LHS is a different name; `sx2 += ..`, `sx2 = .. + sx2` and every other
 *     one-name spelling normalise to the compound and put sx2 in the dest.
 *     (mechrides.c's Safari/Spider/Plane/Barrel _Activate twins say the same
 *     for their X chain; this is the same code shape.)
 *   - Because the delta's register then BECOMES sx2, it must be callee-saved,
 *     so the zero-extend target is edi rather than a scratch; that leaves eax
 *     LIVE (it still holds Get_XScroll's short return) at the `g_map_cfg`
 *     load, which therefore takes edx.  With one web the movsx is scheduled
 *     first, eax frees, the cfg pointer lands in eax and takes the 5-byte
 *     `A1` encoding -- which is the whole 1121-vs-1122 byte difference.
 *   - `mov ebp,ebx / add ebx,edi` vs our `mov edi,ebx / sub edi,ebp / add
 *     ebp,ebx` is NOT an IR-order difference: both are diff-first, and the
 *     scheduler hoists the sum+imul into the gap after the copy.  Whether it
 *     can hoist to slot 2 (original) or only slot 3 (ours) is decided purely
 *     by which operand's register the add consumes -- a WAR hazard against
 *     the copy in one case and against the sub in the other.
 * WHY IT IS NOT SHIPPED: the extra web rotates the LOOP HEAD -- the original
 * has ebp = sq and ebx = ty, the two-web form gets ebx = sq and ebp = ty --
 * which costs ~33 indices in the loop head, case 5 and the block layout
 * (ESCAPES returns, with a duplicated jmp before case 3).  Net bad 24 -> 57.
 * The rotation is NOT the number of values live across `Get_XScroll` (three
 * callee-saved either way, esi + two) and is not reachable from the loop
 * head: all 60 legal orders of `next/sq/b/tx/ty` were measured on top of the
 * two-web form and every one keeps the rotation (best 57, most 60-91).  Nor
 * is it the block layout: the four `goto endsw` asymmetries and the plain
 * `if (b->state == 0) { }` guard were re-measured on top of it (57..91).
 * THE ROTATION IS NOT GLOBAL PRESSURE, measured directly: removing a web
 * elsewhere leaves it untouched -- the cached `sp` byte in case 5, case 5's
 * sx/sy CSE, the `lane` local in case 0, `tx`/`ty` computed lazily inside
 * case 0, the spill shim moved or put on sy2: every one still reports
 * first=16 and the same ebx/ebp swap.  Nor is it the NAME: carrying the
 * pre-scroll x in `ty`, `tx` or `lane` (all dead in case 3) instead of a new
 * `px` is byte-identical to `px`, confirming "webs, not names" here.  What
 * actually flips is narrow: `def->base_x` takes ebx in the original and ebp
 * in the two-web form, and `ty` (which reuses that register) and `sq` follow.
 * *** If a later round finds what ranks sq above ty, this variant is very
 * probably the finished function: apply scratchpad/laneL/tsu_d.py:x2web. ***
 * *** WITHDRAWN 2026-09-04 (sixth pass).  What ranks sq above ty WAS found
 * this round -- the two world reads moved above the GetScreenCoordsForObject
 * call -- and x2web was re-applied on top of it: still 327, still the flipped
 * loop head, ESCAPES, 1113 bytes.  So do the seven laneL forms (x2web,
 * _rev, _sp2, _ysub, y2web, x_delta2, both_delta2) crossed with all four
 * read/call/GetTileDimensions placements and a struct-copy read: 228-327,
 * every one either flipping the loop head or 100+ over.  The extra web is a
 * SECOND, independent tip of the same tie-break, not the same one, and
 * sinking the call below the projection (a strictly longer `sq` range) makes
 * it worse, not better (324) -- so the lever is the reads sitting above the
 * call, not `sq`'s live range as such.  Do not spend another round on x2web
 * without a new mechanism. ***
 *
 * ALSO MEASURED AND INERT THIS ROUND (all exactly 72/robl 332, or worse):
 * commuting either product, `tw * (wx - wy)`, splitting either product into
 * 2 or 3 statements, `unsigned`/`long` on wx or wy (`short` costs 11),
 * `if (wx) ;` / `if (wy) ;` extra uses (VC6 deletes them outright -- the
 * empty-if lever does NOT extend a live range here), a `worldcopy` struct
 * read of b->world, direct `b->world.x/y` reads in either product, `p =
 * b->person` cached for the zsprite store (mechrides lever 11 -- worth 6
 * there, worth -1 here), the same with `g_ts_zspr` cached too, `sox`/`soy`
 * locals read early in three positions, and the Y chain split into two webs
 * (71 strict but robl 328 -- a compensating error, rejected).
 * `int seat = b->seat;` cached just before `b->b35 = 0;` is worth one
 * structural index (robl 334, bad 23) but is not a credible source line and
 * costs one strict, so it is not shipped.
 * WHAT IS LEFT (15 original indices, all schedule, none reachable from any
 * spelling measured): 131 (the sum/diff hoist above), 139 and 146 (the two
 * `movsx`), 148 (the original loads screen.oy into the register `px` just
 * freed -- ours has no free register there), 150/153, 161/163/166/168-169/172
 * (the `or flags`, `g_ts_zspr` and `b->person` loads interleaved into the
 * rider_dy chain instead of grouped after the two `pos` stores), 184/188 (the
 * original reuses eax for BOTH NewBNVPath loads because its seat byte is in
 * ecx; ours has the seat in eax and two scratch registers free, so the
 * scheduler groups the loads), and 276 (case 5's `sp` load, above).
 * EVERY ONE of these is downstream of the same register cascade the two-web
 * form fixes. */
/* ROUND OF 2026-09-04 (fourth pass).  72 -> 57 strict, and the two-web reading
 * of case 3 IS SHIPPED.  Metrics (permutation-aware ranking, scratchpad/
 * w7joust/w7.py -- audit strict, "real" = mismatches surviving the best
 * callee-saved permutation, register+offset-blind LCS, ORIGINAL indices inside
 * a differing region):
 *
 *     metric                       was    now
 *     audit strict                  72     57
 *     real (permrank)               67     53
 *     reg+offset-blind LCS         332    334   (of 347)
 *     orig indices in a bad region  24     21
 *     bytes                       1121   1124   (orig 1122)
 *
 * THE KEY: THE PREVIOUS ROUND'S TWO-WEB X CHAIN WAS RIGHT, AND WHAT BLOCKED IT
 * WAS THE ORDER OF THE TWO PRODUCTS, NOT THE WEBS.  Written with the SUM
 * FIRST the loop head does NOT rotate:
 *      py = (wx + wy) * th >> 9;
 *      px = (wx - wy) * tw >> 9;
 *      *(volatile int*)&spill.x = px;
 *      sx2 = g_map_cfg->ox - Get_XScroll() + px;
 *      sy2 = g_map_cfg->oy - Get_YScroll() + py;
 * With the DIFFERENCE first (every spelling the last round tried) `def->base_x`
 * takes ebp, `sq` takes ebx, the two byte loads of `sq->b.x`/`sq->b.y` are
 * hoisted together, `ty` can no longer be built in a byte-addressable register
 * (`xor ebx,ebx / mov bl,[ebp+1]` becomes `xor eax,eax / mov al,[ebx+1] / mov
 * ebp,eax`) and that one extra instruction displaces every later index:
 * first=16, ESCAPES, 321.  Sum-first keeps indices 0..127 EXACT and buys the
 * whole of 134..156: `mov edx,[g_map_cfg] / xor edi,edi / movsx ecx,ax /
 * mov di,[edx+0x20] / sub edi,ecx / add edi,ebp` -- both `movsx` placements,
 * both `g_map_cfg` loads and the X add with the DELTA as its destination.
 * The Y chain had to become a second web too (`sy2 = delta + py`): with Y left
 * compound the X delta lands in ebp instead of edi (65/robl 330 vs 61/robl
 * 334).  That is the one place this build still contradicts the original,
 * which has `add ebx,ecx` (Y's add destination is the WEB) at index 153.
 * The full 2x2x2 matrix (product order x X one/two web x Y one/two web) was
 * measured on this baseline: sum+x2+y2 61, sum+x2+y1 63, diff+x1+y1 73,
 * diff+x1+y2 70, sum+x1+* 212-214, diff+x2+* 323-326 with ESCAPES.
 *
 * TWO MORE FROM RE-RUNNING SWEEPS THE LAST ROUND HAD RECORDED AS EXHAUSTED
 * (the standing "re-run after any structural change" rule paid twice again):
 *   A. All 120 orders of case 3's five trailing statements: the winner is now
 *      pos.x, pos.y, `b->flags |= 0x80;`, zsprite, f30 -- i.e. the flag OR
 *      ABOVE the two person stores, the opposite of the last round's winner,
 *      and it agrees with the original, which emits `or byte [esi+0x62],0x80`
 *      at 161, twelve slots before the zsprite store.  61 -> 58.
 *   B. Case 5's cached `sp` byte moved one statement down, between the two
 *      person stores.  58 -> 57, and 272/277/278 all go exact; the residual
 *      there is now only the ecx/edx rotation (below).  Plain
 *      `b->speed = b->saved_speed;` with no cache is 59 on this baseline.
 *
 * MEASURED AND REJECTED (better numbers, wrong model -- recorded so the next
 * round does not "find" them again):
 *   - `*(volatile int*)&spill.x = py;` (spill the Y value) scores 56/real 52/
 *     bad 20.  It matches index 135 only because our px/py registers are the
 *     original's swapped (orig X in ebp, Y in ebx; ours X in ebx, Y in ebp) --
 *     a textbook compensating error.  The original spills the X value
 *     (`imul ebp,[esp+0x28]` is the tw product).  NOT SHIPPED.
 *   - Putting the Y chain textually before the X chain scores 60/robl 335, but
 *     it swaps the two `Get_?Scroll` calls; `match.py` normalises call targets
 *     so the swap does not show in the strict count.  Semantically wrong.
 *
 * ALSO INERT ON THIS BASELINE: all 24 orders of the four `-=` statements; every
 * position of `b->b35 = 0;` and of the `GetUnitDepth` store; declaration order
 * of wx/wy and of tw/th; reading wx before wy; `(wy + wx)`; `b->world.x` read
 * inline in one or both products; `GetTileDimensions` before the world reads;
 * a `px = tw;` pre-read; and (worse, first=17) tw/th as a two-int struct or
 * array -- an aggregate moves the whole frame.
 *
 * A FOURTH CLOSE, from a lever imported from anim2.c: the two x adjustments
 * `sx2 -= g_ts_rider_dx / 2; sx2 -= screen.ox;` summed in the fields of a
 * non-address-taken `Pos` (`t.x = rider_dx/2; t.y = screen.ox; sx2 -= t.x+t.y`)
 * -- BoatingSchool_DrawBoats' barrier against forward substitution.  57 -> 56,
 * real 53 -> 52, robl 334 -> 335, bad 21 -> 19.  It is a codegen device, not a
 * line anyone wrote, and it is shipped for the same reason DrawBoats' is: every
 * metric moves the right way and the two spellings compute the same value.
 * `sx2 -= a + b;` without the aggregate is inert (57), so the aggregate is
 * doing the work, not the association.  The same barrier on the Y pair is 205
 * (it breaks the Y web); the fields swapped is 57; `t.y + t.x`, the y pair
 * moved above the block, and the two assignments interleaved are all 56; a
 * three-field struct carrying the y half as well is 205.
 *
 * *** ADD-DESTINATION TIE-BREAK, RE-MEASURED UNDER CONTROL (2026-09-04). ***
 * The earlier "read-first wins" measurement made in this file is WITHDRAWN, and
 * so is "last-defined wins" as a description of THIS site: with the read order
 * held constant (`wx` is the left operand of both the difference and the sum in
 * every variant) and only the definition order of `wx`/`wy` varied, the
 * destination flips in BOTH directions, so neither symbol rank predicts it.
 * Probe: scratchpad/w7joust/tb.py + tbrun.py, four variants, window 128-134.
 *     one web, `wy` defined first:  mov edi,ebx / sub edi,ebp / add ebp,ebx
 *     one web, `wx` defined first:  mov edi,ebp / add ebp,ebx / sub edi,ebx
 *     two web, `wy` defined first:  mov ebp,ebx / add ebx,edi / sub ebp,edi
 *     two web, `wx` defined first:  mov ebp,edi / sub ebp,ebx / add ebx,edi
 * In rows 1 and 4 the add's destination is `wy`'s register; in rows 2 and 3 it
 * is `wx`'s.  The invariant is not definition order and not read order, it is
 * EMISSION ORDER: VC6 copies the LEFT operand of the difference into a fresh
 * register, and then whichever of {sum, difference} is emitted SECOND takes the
 * remaining operand's register as its destination.  Sum emitted first (rows 2
 * and 3) -> destination is `wx`, whose value the copy has already preserved;
 * difference first (rows 1 and 4) -> the add comes second and clobbers `wy`.
 * Definition order enters only by changing which local is in which register and
 * hence which op the scheduler puts first.  Nothing shipped here rested on the
 * withdrawn reading -- the two-web change above was measured directly -- and
 * the loop-head rotation is NOT reachable from the definition order either
 * (both two-web rows above are 324-325 with first=16).
 *
 * NEW LEVER RECORDED HERE: the frame slots of two address-taken scalars filled
 * by ONE out-param call follow their first RVALUE USE, not their declaration
 * order.  With the sum first `th` is read first and takes F+0x08, so
 * `GetTileDimensions(&tw, &th)` emits `lea edx,[esp+0x20] / lea eax,[esp+0x24]`
 * where the original has them the other way (indices 123/124).  Swapping the
 * declarations, moving them into case 3's block scope and a `px = tw;` probe
 * are all inert; only the order of the two products moves them.
 *
 * WHAT IS LEFT (21 original indices in a differing region, first divergence
 * 119):
 *   - 119/120: `wy` takes ebx and `wx` edi where the original has edi/ebx.
 *     EVERYTHING else in case 3 follows from this one swap: it puts the sum in
 *     a fresh register (`lea ebp,[edi+ebx]`) instead of overwriting wx's
 *     (`mov ebp,ebx / add ebx,edi`), which costs indices 128..134 and forces
 *     the extra `mov ebx,edi` copy of px, and it is why the Y add lands on the
 *     delta at 153.  Nothing measured moves it (see the inert list).
 *   - 123/124: the tw/th slot order, above.
 *   - 156..172: the original interleaves `or flags`, the `g_ts_zspr` load and
 *     the two `GetUnitDepth` constant pushes INTO the rider_dy division; ours
 *     groups them.  All 120 tail orders and all 24 subtraction orders were
 *     measured; this is what is left of them.
 *   - 178/179, 181, 183..190, 270, 273..276: a GLOBAL ecx/edx phase shift --
 *     wherever the original creates two scratch temps the first takes ecx and
 *     ours takes edx (case 3's `xor ecx,ecx` seat / `lea edx,&pos`, case 5's
 *     two `b->person` loads).  It also costs the original's reuse of eax for
 *     BOTH `NewBNVPath` loads.  Worth attacking next: one temp too few or too
 *     many earlier in the function would flip the whole rotation. */
/* ROUND OF 2026-09-04 (fifth pass).  56 -> 29, and every metric moved with it:
 * real (mismatches surviving the best callee-saved permutation) 52 -> 25,
 * register+offset-blind LCS 335 -> 338 of 347, ORIGINAL indices inside a
 * differing region 19 -> 14, bytes 1124 -> 1123 (orig 1122).  EVERYTHING FROM
 * INDEX 173 TO THE END IS NOW EXACT; the residual is one register cascade in
 * case 3, listed at the bottom.
 *
 * *** RECONSTRUCTION ERROR 1: A MISSING LOCAL FOR THE PATH NAME (18 indices).
 * The original is
 *      177 mov eax,[esi+4]        180 fstp [eax+0x3c]
 *      178 xor ecx,ecx            181 mov cl,[esi+0x36]     <- seat in ECX
 *      179 lea edx,[esp+0x4c]                               <- &pos in EDX
 *      185 mov eax,[ecx*4+0x4b4f08]   187 push eax
 *      188 mov eax,[0x4cbfb8]         190 push eax          <- eax REUSED
 * and we had the seat in edx and `&pos` in ecx, which left ecx free early and
 * let the scheduler hoist the `g_ts_objsamples` load into it instead of
 * reusing eax.  That one-step eax/ecx/edx phase difference then ran through
 * the WHOLE of cases 4 and 5 as a clean three-cycle (ours eax/ecx/edx where
 * the original has edx/eax/ecx) and cost indices 178, 179, 181, 183-185,
 * 187-188, 190, 197-198, 206-207, 213, 215, 220-222, 239, 241, 247, 249, 270,
 * 273-274 -- 27 of the 56, none of them reachable from case 3's projection
 * (measured: the mismatch count at index >= 195 is 18 for EVERY projection
 * variant, and 148-152 for the ones that displace the body).
 * The fix is one ordinary source line: the rider's path name read into a local
 *      path = g_ts_path_names[b->seat];
 * immediately BEFORE the NewBNVPath call.  It shortens the seat temp's live
 * range to a single statement, which is what puts it in ecx.  Position is
 * load-bearing and was swept: after `b->b35 = 0;` is 31, before it 34, at the
 * top of the tail 46, above the flag OR 44.  A cached `int seat` instead of
 * the path pointer also fixes the phase but costs eight indices (42); caching
 * `g_ts_objsamples` as well is 58; caching the path AND the seat is 29 (inert
 * on top).  `&pos` in a local pointer and `objs`-style caches are inert.
 *
 * *** RECONSTRUCTION ERROR 2: case 5's saved_speed shim is no longer needed.
 * With the phase corrected the plain source line is the best AND the simplest:
 *      b->person->zsprite = 0;
 *      b->person->f30 = 0;
 *      b->speed = (unsigned char)b->saved_speed;
 * 31 -> 29, robl 337 -> 338, bad 16 -> 14, and indices 275/276 go exact.  The
 * `unsigned char sp` cache that the previous round shipped between the two
 * person stores is now WORSE (31), and reading it first is 34.  A standing
 * rule earning its keep: re-test every committed shim after a structural
 * change.
 *
 * RE-RUN ON THE NEW BASELINE AND STILL THE OPTIMUM: the whole product-order x
 * X-web x Y-web matrix (2x2x2, plus the split-shift form -- with the two
 * `>>= 9` written separately the PRODUCT order stops mattering entirely and
 * only the SHIFT order does, exactly as goldrush.c's StepSchoolCar found:
 * `px >>= 9` first == the old "difference-first" point at 79/robl 325,
 * `py >>= 9` first == this build); all 24 orders of the four `-=` adjustments
 * (best plain order 30, so the `Pos t` barrier is still worth exactly one
 * index and every metric agrees); all 60 orders of case 3's five trailing
 * statements with the two person stores kept in order (this one is the best);
 * `wx` defined first (29 but robl 337); `Pos w = b->world;` as a struct copy
 * (29/337); `&b->world` through a pointer (34); the world reads inline (34);
 * GetTileDimensions before the world reads (34); `(wy + wx)` (inert).
 *
 * NEW AND INERT: the local NAMES are irrelevant even across cases -- px/py or
 * sx2/sy2 renamed onto case 5's `sx`/`sy` or the loop head's `tx`/`ty`, or all
 * four collapsed, are byte-identical (VC6 numbers by first use in the
 * optimised IR, so merging two disjoint names does NOT merge their webs); the
 * two Get_?Scroll results in locals (322, ESCAPES); `g_map_cfg->ox`/`oy` in
 * locals (326, ESCAPES); a third use of `wy` after the products (328,
 * ESCAPES); `tw`/`th` copied into a second pair (inert); explicit temps for
 * the sum, the difference or both (inert); `screen.oy` read into a local
 * before or inside the chains (inert).
 *
 * MEASURED, BETTER ON STRICT, STILL NOT SHIPPED: `*(volatile int*)&spill.x =
 * py;` (spilling the Y value) is 27/real 24/bad 13 with robl unchanged.  It is
 * the same compensating error the fourth round rejected -- our px/py sit in
 * the original's registers SWAPPED, so spilling the wrong value matches index
 * 135 by accident.  The original spills the X value (`imul ebp,[esp+0x28]` is
 * the tw product).
 *
 * WHAT IS LEFT (14 original indices in a differing region, first divergence
 * 119) IS ONE CASCADE, all of it downstream of a single register tie-break:
 *   - 119/120: `wy` takes ebx and `wx` edi where the original has edi/ebx.
 *     From that follow 123/124 (the tw/th frame homes swap), 128-135 (the
 *     original builds the DIFFERENCE from a copy of wx and the SUM in place --
 *     `mov ebp,ebx / add ebx,edi / sub ebp,edi` -- while we build the sum
 *     fresh with a `lea` and the difference in place, then need an extra
 *     `mov ebx,edi` to carry px past Get_XScroll), 142/148 (px's and
 *     screen.oy's registers), 153/155/156 (the Y add lands on the delta
 *     instead of the web) and 161-172 (because our sy2 ends up in ECX rather
 *     than ebx, the `g_ts_zspr` load cannot be hoisted into ecx and the flag
 *     OR / person load / zsprite load are emitted after the two `pos` stores
 *     instead of interleaved into the rider_dy chain).
 *   - The diff-first source (which alone gets index 120 and the tw/th homes
 *     right, as the 79-point variants show) rotates the LOOP HEAD instead:
 *     `sq` moves to ebx and `def->base_x` to ebp, first divergence 16 and
 *     ESCAPES.  So case 3's projection and the loop head's allocation are
 *     coupled, and the next round should attack the pair together rather than
 *     re-sweeping case 3 alone. */
/* ROUND OF 2026-09-04 (sixth pass).  29 -> 22, and the BYTE LENGTH IS NOW
 * EXACT (1123 -> 1122 = the original's).  real 25 -> 22, robl 338 -> 340 of
 * 347, bad regions 14 -> 13.  The whole case-3 projection -- original indices
 * 121 through 147, twenty-seven consecutive instructions including the
 * `mov ebp,ebx / add ebx,edi / sub ebp,edi` shape that five rounds called the
 * wall -- is now byte-for-byte exact.  Two reconstruction errors, both found
 * by reading the original instruction by instruction:
 *
 * *** RECONSTRUCTION ERROR 1: THE X PROJECTION IS FINISHED FIRST.
 * The original is
 *      128 mov ebp,ebx      <- copy wx
 *      129 add ebx,edi      <- the SUM computed IN PLACE, in wx's register
 *      130 imul ebx,[th]
 *      131 sub ebp,edi      <- the DIFFERENCE built from the copy
 *      132 imul ebp,[tw]
 *      133 sar ebp,9        (px)      134 sar ebx,9  (py)
 * A sum computed IN PLACE means the sum is the LAST use of `wx`, so in the IR
 * the difference comes FIRST.  Writing `px` before `py` gives exactly those
 * five instructions; our old sum-first order made VC6 emit the sum with a
 * `lea` into a fresh register (`lea ebp,[edi+ebx] / sub edi,ebx`) and then
 * pay an extra `mov ebx,edi` to carry px past Get_XScroll -- the ONE byte the
 * body was over.  The lever is which projected coordinate is FINISHED first:
 * with the split-shift form all four product orders collapse into two objects
 * selected purely by whether `px >>= 9` or `py >>= 9` is written first.
 *
 * *** RECONSTRUCTION ERROR 2: THE TWO WORLD READS SIT ABOVE THE
 * GetScreenCoordsForObject CALL.  On its own, error 1 costs 300: it flips the
 * LOOP HEAD, moving `sq` from ebp to ebx and `ty` from ebx to ebp, and case 2
 * then stops sharing case 0's CalcMoveLine tail (the identical-suffix merge
 * fails because the registers no longer agree).  The two allocations really
 * are coupled, and what decouples them is `sq`'s live range: a probe that
 * lengthened it with a dummy `sq->b.x` test inside case 3 restored the loop
 * head immediately.  The zero-cost way to do the same is to read
 * `b->world.y` and `b->world.x` BEFORE the call, which is what the shipped
 * body now does.  Measured: reads before the call 22; reads after 327 (the
 * flipped-loop-head object); only one read before 327; GetTileDimensions
 * moved into the gap 36-37; a `Pos w = b->world;` struct copy before the call
 * ties at 22; a `Pos*` taken before the call and dereferenced after, and a
 * plain `sq2 = sq;` copy, are both 327 (copy-propagated away, so they do not
 * lengthen the range).
 *
 * *** RE-TESTED SHIM, NOW WITHDRAWN: the `Pos t` partial-sum barrier on the
 * two x adjustments.  With the projection fixed the four plain `-=` are
 * strictly better (23 -> 22, robl 337 -> 340, bad 17 -> 13).  It had been a
 * workaround for the wrong projection shape all along -- and it was a
 * misapplication of the four-term rule to a two-term sum.  Third round in a
 * row that "re-test committed shims after a structural change" has paid.
 *
 * INERT / WORSE ON THE NEW BASELINE (all re-measured, not inherited):
 * every association of sx2/sy2 (`px + (ox - XScroll)` etc., VC6 canonicalises
 * a two-term sum) and every two-step spelling of either scroll statement
 * (`sy2 = oy - YScroll(); sy2 += py;` and the x twin are byte-identical; only
 * `sy2 = py - YScroll(); sy2 += oy;` differs, at 61, and the y statement
 * written first is 27); all four orders of the four `-=` adjustments, and the
 * interleaved order (22, one object); the volatile spill moved or removed
 * (removing it is 227); spilling py instead of px (327); all eleven positions
 * of the `pos.x`/`pos.y` pair inside the case-3 tail -- ANY of them other
 * than first flips the loop head back to the 327 family, so the pos stores
 * being first is now load-bearing too; caching tile bytes or base_x/base_y in
 * the loop head; a `Pos` aggregate for tx/ty; reversing the loop head's two
 * sums.
 *
 * WHAT IS LEFT (22 indices: 116-120, 148, 153, 156-158, 161-172):
 *   - 116-120: the two world loads are emitted BEFORE the call's two pushes,
 *     where the original has them after.  That is pure source order -- and
 *     source order is exactly what the loop head needs (above).  Either the
 *     original reaches the same live-range extension by a construct not yet
 *     found, or this five-index shift is the price of the other 300.  This is
 *     the coupling in its final, much smaller form.
 *   - 148 + 153 + 156-158: the original hoists screen.OY into ebp (freed
 *     after 142) and accumulates the Y chain into py's own register
 *     (`add ebx,ecx`); we hoist screen.OX and accumulate into the scratch
 *     (`add ecx,ebx`).  By the imul/add RANK rule the destination is the
 *     compiler TEMPORARY, so the original's `py` must be a temporary and ours
 *     is a symbol -- but `py += ...` and inlining the whole product into the
 *     sy2 expression both flip the loop head (327/235).  The next attempt
 *     should look for a spelling that makes py a temporary WITHOUT
 *     lengthening the sy2 web.
 *   - 161-172: with sy2 in ecx instead of ebx the flag OR, the g_ts_zspr load
 *     and the `b->person` load cannot be interleaved into the rider_dy
 *     division; they are emitted after the two `pos` stores.  All of this is
 *     downstream of 153.
 *   - 148 + 155-158 are the same cascade seen from the other side: with sy2
 *     in ecx, ebx is free, so BOTH screen fields get callee-saved registers
 *     (ox in ebp, oy in ebx) and the scheduler swaps the two `sx2 -=`; the
 *     original has ebx busy with sy2, so only ebp is free, it takes the
 *     LAST-used field (oy) and ox is loaded into eax immediately before use.
 *     Nothing here is independent of 153. */
/* ROUND OF 2026-09-05 (tenth pass).  UNCHANGED AT 22 / real 22 / robl 340 /
 * bad 13 / 1122 bytes.  The index-153 tie-break is now MECHANICALLY PROVEN
 * rather than inferred, and the reason it cannot be spent is proven with it.
 *
 * *** WHAT DECIDES THE Y SUM'S DESTINATION.  The original is `add ebx,ecx`
 * (dest = py's register, which is what frees ecx for the `g_ts_zspr` load at
 * 162 and the `b->person` load at 166, i.e. the whole 161-172 tail); we emit
 * `add ecx,ebx`.  VC6 coalesces the destination with the sum's FIRST IR
 * operand, and the IR operand order is NOT reachable from source association:
 * `sy2 = py + (g_map_cfg->oy - Get_YScroll())`, `sy2 = py + g_map_cfg->oy -
 * Get_YScroll()`, `sy2 = -Get_YScroll() + g_map_cfg->oy + py`, explicit
 * `dx`/`dy` delta locals in BOTH operand orders, `sy2 = py; sy2 += delta;`
 * (copy-then-accumulate) and swapping the DECLARATION order of px/py are all
 * BYTE-IDENTICAL to this build.  VC6 canonicalises a two-term sum and
 * copy-propagates the copy, so the only construct that puts py first is one
 * where py IS the destination symbol -- `py += g_map_cfg->oy - Get_YScroll();`.
 * VERIFIED IN THAT BUILD'S OWN LISTING: it emits `add edi,ecx` with edi = py,
 * exactly the original's shape.  It is still the 327 family (first divergence
 * 16, 1113 bytes): py's web then runs to the end of case 3 and flips the LOOP
 * HEAD (`sq` ebp->ebx, `def->base_x` ebx->ebp) and the projection with it.
 * *** AND THE WEB CANNOT BE SPLIT.  Four ways of ending py's web early after
 * the accumulate -- a fresh `sy2 = py - g_ts_rider_dy/2`, a fresh `sy2 = py -
 * screen.oy`, the doubling alone through sy2, and the x adjustments moved
 * below -- are ONE OBJECT at 327: copy propagation merges the fresh name back
 * into py.  So "make py a temporary WITHOUT lengthening the sy2 web", which
 * the ninth pass set as the next attempt, is CLOSED: in VC6 the two are the
 * same thing.
 * *** THE TWO ACCUMULATORS ARE NOT SYMMETRIC, which is a new fact: `px +=` on
 * the X side KEEPS the loop head (227/228, first divergence 116) where
 * `py +=` breaks it.  Both accumulating is 227 and reorders the projection to
 * `lea ebp,[ecx+ebx]` with an extra spill; X only is 228.
 *
 * *** THE ORIGINAL'S DEAD STORE AT INDEX 135 IS IDENTIFIED: `mov [esp+0x20],
 * ebp` at 0x004175d2 has EIGHT pushes live (four callee-saved + two for
 * GetScreenCoordsForObject + two for GetTileDimensions), so it writes frame
 * E+0x00 -- and E+0x00 is `r`'s OWN home (the loop tail's `mov [esp+0x10],eax`
 * at 0x00417880 and case 6's `mov eax,[esp+0x18]` at 0x00417862 are the same
 * slot).  px is never reloaded from it and `r` is dead through case 3, so this
 * is a temp lifetime-coloured onto a named local's home, not a pressure spill:
 * at 135 only b/px/py are live and all three are already in callee-saved
 * registers.  The `SpillPair spill` + volatile write reproduces the slot
 * exactly; what it does not explain is what source line VC6 saw.
 *
 * NEW AND INERT (all 22 / robl 340 / bad 13, byte-identical): the screen
 * offsets folded into the doubling (`pos.x = (sx2 - screen.ox) * 2`, in two
 * arrangements); `sx2 + sx2` for the doubling; the y adjustments before the x
 * ones; `screen.ox`/`oy` cached into the (otherwise case-5) `sx`/`sy` ints.
 * NEW AND WORSE: a `static __inline TS_Scroll(base, scroll, proj)` helper for
 * the y statement (66) or both (70) -- the joust seat-helper lever does not
 * fire here because a plain memory reference passed as an inline argument is
 * forward-substituted; `sy2 = g_map_cfg->oy; sy2 -= Get_YScroll(); sy2 += py;`
 * (209).
 * MEASURED BETTER ON THE BLIND METRICS AND REJECTED: `pos.y` stored BEFORE
 * `pos.x` is 22 strict with robl 341 and bad 11 -- the best register-blind
 * numbers this function has ever shown -- but the original stores pos.x first
 * (171 `mov [esp+0x4c],edi`, 172 `mov [esp+0x50],ebx`), so it is a
 * compensating shuffle of the whole x/y adjustment block, not the source.
 * RE-RUN ON THE HONEST SOURCE: reads-after-the-call crossed with five loop
 * head shapes (b before sq, next read last, ty before tx, the two sums
 * reversed, plain) is 327-334 in every case, and crossed with four case-3
 * shapes (wx/wy in their own block scope, tw/th in their own block scope, the
 * volatile spill moved above the projection, the volatile spill of py) is
 * EXACTLY 327 in all four -- the reads-after family is a single hard
 * attractor and the 116-120 price is not bought back from either end.  So the
 * two residuals are one problem after all, and the only thing that has ever
 * moved either of them is the pair {reads placement, destination symbol},
 * which cannot be set independently. */
/* SCOPE G, RANK ROUND (2026-09-06).  No count change -- still 18/347 with
 * 347/347 instructions, 1122/1122 bytes, first divergence 116, no ESCAPES, and
 * the code byte-identical to LEGOLAND/joust.c.  The round's product is the
 * MECHANISM behind the residual, and the first builds that reproduce 116..120
 * with the world reads in their honest place.
 *
 * WHAT DECIDES EVERYTHING.  Every build of this body lands in one of exactly
 * two allocations of the loop head's four long webs over {esi,edi,ebx,ebp}:
 *     GOOD (the original)   b->esi  tx->edi  ty->ebx  sq->ebp
 *     FLIPPED               b->esi  tx->edi  sq->ebx  ty->ebp
 * The flipped family is NOT a cheaper rival that VC6 legitimately prefers: its
 * whole body is 1200 bytes against the original's 1122, and audit only reports
 * 1113 because it truncates at 347 instructions.  VC6 falls into it.
 *
 * The two families are separated by ONE INTERFERENCE EDGE.  `sq` dies at the
 * `push ebp` that hands it to GetScreenCoordsForObject.  With the world reads
 * ABOVE that call, `wx`/`wy` are defined while `sq` is still live, so `sq` can
 * take neither ebx (wx) nor edi (wy) and ebp is FORCED -- the original's
 * answer, reached by a route the original does not use.  With the reads BELOW
 * the call, where the disassembly puts them, that edge is gone, `sq` and `ty`
 * are a free tie, and VC6 breaks it the other way.  This also explains the old
 * puzzle of why the good family is a knife edge: it is held up by one edge.
 *
 * THE TIE IS BROKEN BY `sq`'s WEIGHT, AND A SINGLE REFERENCE DOES IT.  In the
 * reads-below build, re-basing ONE read of the tile's y byte off `r` instead
 * of `sq` puts the loop head straight back into the good family:
 *   - case 5's `sq->b.y` as `((RideTile*)&r->ride_id)->b.y`: 64 mismatches,
 *     first divergence 148, and 116..120 MATCH EXACTLY --
 *     `push ecx / push ebp / call / mov edi,[esi+0x6c] / mov ebx,[esi+0x68]`.
 *   - the LOOP HEAD's own `sq->b.y` re-based the same way: 29 mismatches,
 *     first divergence 19, 347/347 and 1122/1122 with NO ESCAPES.  116..120
 *     closed; the entire cost is the seven loop-head slots where `[eax+0xd]`
 *     stands in for `[ebp+1]`, index 111, and eight slots in case 5's tail.
 * Re-basing the X byte instead does nothing at all, and so does every cast
 * VIEW of the same pointer: `((unsigned char*)sq)[1]`, `*((unsigned char*)sq
 * + 1)`, `*(&sq->b.y)` and `(&sq->b)->y` are canonicalised back to the `sq`
 * symbol and are inert in BOTH families.  Only a genuinely different base
 * pointer counts, and that costs the addressing byte-for-byte.
 *
 * SO THE QUESTION FOR THE NEXT LENS IS NARROW, and it is not about case 3:
 * what in the original lowers `sq`'s weight, or raises `ty`'s, by the one unit
 * these two shims supply, while every tile read still addresses through `sq`?
 *
 * RULED OUT THIS ROUND, every item measured in BOTH families:
 *   - declaration order of sq/b/tx/ty, five permutations: completely inert --
 *     18 in the good family and 327 in the reads-below family for every one.
 *     Symbol order is not a lever here; the split is pure dataflow.
 *   - loop-head accumulate shapes (ty in place, both in place, x in place,
 *     operand swaps): inert in both.  `ty` written before `tx` is 30/328.
 *   - `register` on ty, on tx+ty, on sq, on b and on next: inert in both.
 *   - switch-case ORDER in the source.  VC6 does NOT sort the arms: writing
 *     case 6 first moves its block first (235 mismatches from index 36), so
 *     the present 0..6 order is load-bearing and case rotation is unavailable
 *     as a lever here, unlike the empty arm in joust2.c.
 *   - the rider walk as `for (r = def->riders; r; r = next)`: inert in both.
 *   - case 5 spellings that keep `sq` (operand swaps, a `<<= 8` split, the two
 *     bytes hoisted into block locals, a second RideTile* copied from sq, the
 *     y byte via `sq->key >> 8`): all inert in both.
 *   - re-basing case 2's y byte off `r` (330) and case 6's `sq` argument off
 *     `r` (inert): neither tips the family.
 *   - the flag OR between the X pair and the Y pair -- which is where the
 *     original EMITS it, at 161 -- and after either half of either pair:
 *     322 / 195 / 325.  Top of the adjustment block is still the optimum.
 *   - all 24 orders of the four `-=` crossed with three OR positions, 72
 *     builds, re-measured on the reads-below good-family base: one object at
 *     64.  The delta-first Y forms fix 149 and lose 152/153 (71); the in-place
 *     form fixes 153 and loses 149.  The wall is unchanged by the new base.
 *   - merging sx2 into px KEEPS the good family (229, first still 116) while
 *     merging sy2 into py FLIPS it, so the bit is not a web COUNT: it is the
 *     ebx chain (ty -> wx -> sy2) specifically that carries it.
 *
 * THE FRAME IS NOW FULLY ACCOUNTED FOR, which settles what `spill` is.  With
 * `base` = esp after the four register pushes, the 0x30 bytes of locals are
 *     +0x10 r (and `spill`, pooled)   +0x18 tw   +0x1c th   +0x20 next
 *     +0x24 ofs   +0x2c screen   +0x34 pos (three ints)
 * every one checked against the original's own operands, including the
 * deferred `add esp,0x44` that ends case 3.  The `px` store at index 135 goes
 * to +0x10, i.e. INTO `r`'s home: `r` is dead on the case-3 path (only case 6
 * reads it, and endsw rewrites it from `next` before the back edge), so VC6
 * pools an eight-byte case-3 local onto it.  That is exactly what `SpillPair
 * spill` models, and it is why deleting it moves the frame to 0x28 and every
 * offset with it. */
// WIP-FUNCTION: LEGOLAND 0x00417430  (347/347 instructions, 1122/1122 bytes -- the original's length to the byte, audit mismatch 18/347; first divergence 116; residual class = ONE loop-head allocation bit plus one schedule cluster -- 116..120 is the price of the shim that forces `sq` into ebp, and 148/149 + 161..172 is the y-delta/interleave pair)
void TempleSlide_Update(RideElem* elem)
{
    RideDef*   def = elem->data;
    RiderNode* r;
    RiderNode* next;
    RideTile*  sq;
    Bloke*     b;
    int        tx;
    int        ty;
    int        lane;
    unsigned char dir;
    Offset     ofs;
    Offset     screen;
    BnvPos     pos;
    SpillPair  spill;
    int        tw;
    int        th;
    int        sx;
    int        sy;
    int        wx;
    int        wy;
    int        sx2;
    int        sy2;
    int        px;
    int        py;
    const char* path;

    r = def->riders;
    while (r) {
        next = r->next;
        sq = (RideTile*)&r->ride_id;
        b = r->bloke;
        tx = def->base_x + sq->b.x;
        ty = def->base_y + sq->b.y;
        /* The busy-AI guard is a `goto` to the loop's continue point, not an
         * `if (b->state == 0) { .. }` block: the explicit label is what makes
         * VC6's identical-suffix merge host the shared CalcMoveLine tail in
         * case 0's copy (the FIRST) instead of case 5's (the last). */
        if (b->state != 0)
            goto endsw;
        {
            switch (b->action) {
            case 0:
                b->flags |= 8;
                b->action++;
                lane = TempleSlide_TakeLane(sq);
                b->seat = (unsigned char)lane;
                if ((unsigned char)lane > 1) {
                    b->target.x = (tx << 8) + 0x380;
                    b->target.y = (ty << 8) - 0x280;
                } else {
                    b->target.x = (tx << 8) + 0x80;
                    b->target.y = (ty - 5) << 8;
                    dir = (unsigned char)CalcMoveLine(b->world, b->target, b->path) + 0x10;
                    b->state = 7;
                    b->new_dir = dir;
                    NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                    b->action = 3;
                    break;
                }
                dir = (unsigned char)CalcMoveLine(b->world, b->target, b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;

            case 1:
                break;

            case 2:
                b->target.x = (def->footprint + sq->b.x + 4) << 8;
                b->target.y = (def->f40 + sq->b.y + 2) << 8;
                dir = (unsigned char)CalcMoveLine(b->world, b->target, b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;

            case 3:
                /* THE READS ABOVE THE CALL ARE A KNOWN SHIM, NOT THE
                 * ORIGINAL'S ORDER.  The original loads b->world at indices
                 * 119/120, INSIDE the block after the call, and VC6 never
                 * moves a load across a call, so the original's source reads
                 * them BELOW it.  What this placement buys is one interference
                 * edge: with `wx`/`wy` defined while `sq` is still live, `sq`
                 * cannot take ebx (wx) or edi (wy), so ebp is forced on it and
                 * the LOOP HEAD lands in the original's allocation.  Written
                 * below the call the edge is gone, the sq/ty choice is a tie,
                 * and VC6 breaks it the other way -- see the note above the
                 * marker for the one reference that tips it back.
                 * Finishing X before Y is separately load-bearing: it makes
                 * the SUM the last use of `wx`, so VC6 computes it in place
                 * (`mov ebp,ebx / add ebx,edi / sub ebp,edi`) instead of with
                 * a `lea`, and the Y chain then stays in ebx to the end. */
                wy = b->world.y;
                wx = b->world.x;
                screen = GetScreenCoordsForObject(sq, def);
                GetTileDimensions(&tw, &th);
                px = (wx - wy) * tw >> 9;
                py = (wx + wy) * th >> 9;
                *(volatile int*)&spill.x = px;
                sx2 = g_map_cfg->ox - Get_XScroll() + px;
                /* THE TWO AXES ARE SPELLED DIFFERENTLY, and that asymmetry is
                 * the original's, not a stylistic one.  X keeps the two-web
                 * `sx2 = <delta> + px` shape: the parenthesised delta is a
                 * compiler temporary and wins the rank-1 destination copy, so
                 * the sum lands in the delta's register and `px` dies --
                 * `add edi,ebp` at 142.  Y must do the OPPOSITE (`add ebx,ecx`
                 * at 153, destination = the projection's own web), and the only
                 * spelling that gets it is accumulating IN PLACE from `py`:
                 * the scroll is subtracted from `py` first and the map origin
                 * added back after.  Every form that computes the delta first
                 * -- `oy - Get_YScroll() + py`, `py + (oy - Get_YScroll())`,
                 * `sy2 = py; sy2 += delta;`, a named delta, or the delta parked
                 * in a `Pos`/`Offset`/`BnvPos` field -- is copy-propagated back
                 * into the delta-wins form and loses index 153 with the whole
                 * 148..172 register cascade behind it.  Transferred from
                 * mechrides.c's SpinningBarrels_Activate, whose y block is
                 * exact; the same asymmetry is visible in ridecb3.c.
                 * The empty `if` is the flatten breaker: without it VC6
                 * reassociates the two later `sy2 -=` statements back into this
                 * sum and re-sorts the terms (20 -> 61).  It is a second
                 * consumer evaluated before the subtractions and the branch is
                 * deleted afterwards, so it costs zero instructions. */
                sy2 = py - Get_YScroll();
                sy2 += g_map_cfg->oy;
                if (sy2) { }
                /* Four plain `-=`.  The `Pos t` partial-sum barrier that
                 * stood here for four rounds was a WORKAROUND for the wrong
                 * projection shape and stays rejected: re-measured against the
                 * Y chain above it is worse, and the same barrier tried on the
                 * Y delta itself is 25.
                 * The flag OR goes at the TOP of the adjustment block, not
                 * after the two `pos` stores: the original emits it at index
                 * 161, twelve slots before the z-sprite store, and only the
                 * source position above the four `-=` reproduces that (18 vs
                 * 20 with it after `pos.y`, 23 at the very end).  Re-swept
                 * against this Y chain: all 24 orders of the four `-=` and all
                 * 30 placements of the two `pos` stores are inert. */
                b->flags |= 0x80;
                sx2 -= g_ts_rider_dx / 2;
                sx2 -= screen.ox;
                sy2 -= g_ts_rider_dy / 2;
                sy2 -= screen.oy;
                pos.x = sx2 * 2;
                pos.y = sy2 * 2;
                b->person->zsprite = g_ts_zspr;
                b->person->f30 = 1;
                b->person->depth = GetUnitDepth(-1617664.875f, -1617913.0f);
                b->b35 = 0;
                /* The rider's path name read into a local BEFORE the call is
                 * what puts the seat's zero-extend in ecx and `&pos` in edx:
                 * it shortens the seat temp's live range to one statement and
                 * lets VC6 reuse eax for BOTH of NewBNVPath's pointer loads.
                 * Without it the whole eax/ecx/edx phase of cases 4 and 5 is
                 * one step out (18 indices). */
                path = g_ts_path_names[b->seat];
                b->bnvpath = NewBNVPath(g_ts_objsamples, 0, path,
                                        -1617664.875f, -1617913.0f, &pos);
                BNVPath_SetDFrame(b, b->bnvpath, 0);
                UpdateBlokeFromBNVPath(b, b->bnvpath);
                b->action++;
                break;

            case 4:
                if (UpdateBlokeFromBNVPath(b, b->bnvpath) == 0) {
                    b->b35 = 1;
                    b->action = 5;
                    HeapFree_w(b->bnvpath);
                    b->bnvpath = 0;
                }
                BlokeSetFrame(b, b->b74);
                if (b->bnvpath) {
                    int f = BNVPath_GetDFrame(b->bnvpath);
                    int walk = g_ts_walk_frame[b->seat];
                    int end;
                    if (f == walk) {
                        BlokeWalkAnim(b);
                        b->saved_speed = b->speed;
                        b->speed = 0x18;
                        break;
                    }
                    end = g_ts_end_frame[b->seat];
                    if (f == end) {
                        b->b35 = 1;
                        b->action = 5;
                        HeapFree_w(b->bnvpath);
                        b->bnvpath = 0;
                        break;
                    }
                    if (f < walk)
                        break;
                    if (f > end)
                        break;
                    BlokeSetFrame(b, 0);
                }
                break;

            case 5:
                BlokeWalkAnim(b);
                BlokeSetFrame(b, 0);
                b->flags &= ~0x80;
                b->person->zsprite = 0;
                b->person->f30 = 0;
                b->speed = (unsigned char)b->saved_speed;
                switch (b->seat) {
                case 0:
                    ofs.ox = -0x280;
                    ofs.oy = 0x700;
                    break;
                case 1:
                    ofs.ox = -0x180;
                    ofs.oy = 0x180;
                    break;
                case 2:
                    ofs.ox = -0x180;
                    ofs.oy = 0;
                    break;
                case 3:
                    ofs.ox = -0x280;
                    ofs.oy = -0x500;
                    break;
                }
                sx = (def->qx + sq->b.x) << 8;
                sy = (def->qy + sq->b.y) << 8;
                b->world.x = ofs.ox + sx;
                b->world.y = ofs.oy + sy;
                b->target.x = sx + 0x80;
                b->target.y = sy + 0x80;
                dir = (unsigned char)CalcMoveLine(b->world, b->target, b->path) + 0x10;
                b->state = 7;
                b->new_dir = dir;
                NewDirForAction(b, (unsigned char)((dir >> 5) + 3));
                b->action++;
                break;

            case 6:
                TempleSlide_ReleaseLane(b->seat, sq);
                RemoveBlokeFromRide(def, r);
                b->flags &= ~8;
                break;
            }
        }
endsw: ;
        r = next;
    }
}

/* ==========================================================================
 * JOUST, +0xb0 -- THE CUSTOM DRAW
 * Same slot as Temple Slide's, but Joust is a "building with bands" like the
 * restaurants (ridecb1.c): the people at this copy of the ride are collected
 * into a small stack array and then drawn in GROUPS, with a mask sprite
 * blitted between groups so the jousting rig occludes whoever is behind it.
 * The group a rider belongs to is its long-term action code (Bloke+0x60), and
 * the draw order is the depth order of the joust arena:
 *
 *   0x18 0x19 0x1a 0x1b 0x1c   the far lane      -> Joust_SpecR_m.lls
 *   0x0c 0x0d 0x0e 0x0f        the near lane     -> Joust_SpecL_m.lls
 *   0x16 0x17 0x1d             the middle        -> the ride's own layer 1,
 *                                                   frame = record byte +0x1b
 *   (the riders actually ON the horses, flag 0x80, positioned by the ride)
 *   0x10 0x0b 0x0a 1 2 3 7 0x14 0x15 0x1e   everyone else -> Joust_fmask.lls
 *
 * The collection array holds eight and is NOT bounds-checked, so a ninth
 * person at the same square walks off the end of it (reproduced -- the same
 * bug ridecb1.c documents for RESTAURANT 1).
 *
 * With nobody here the whole middle is skipped and only the ride's own
 * layer-1 sprite is drawn, at the same animation frame.
 * ========================================================================== */

extern void* GetSpriteForLayer(Spr* sprite, int layer);      /* 0x00441ec0 */
extern Offset GetRenderOffsetForLayer(Spr* sprite, int layer);/* 0x00441ee0 */

/* Draw every collected person whose long-term action equals `code`. */
static __inline void Joust_DrawBand(Bloke** here, char n, int code)
{
    char i;
    for (i = 0; i < n; i++)
        if (here[i]->action == code)
            IP_RenderBlokeIn3DNow(here[i]);
}

/* Seat a rider on its horse: park the seat offset's y and write the person's
 * local x in one step.  This has to be a helper: a `static __inline`'s
 * arguments are evaluated into temporaries BEFORE its body runs, so the
 * person's address is computed before `seat.oy` is stored and the local x
 * store comes after it -- an order no plain-C statement sequence can express
 * (see the note above Joust_Draw). */
static __inline void Joust_SeatRider(Offset* seat, Offset* local, int dx)
{
    seat->oy = -0x30;
    local->ox = dx;
}

/* NOTE: the block structure, the eight-slot collection array, every band and
 * every mask blit are reproduced (528 of the original's 552 instructions), but
 * the count of collected people is kept differently: the original re-tests it
 * from its byte home before EVERY band ('mov bl,[..] / test bl,bl / jle' plus
 * a fresh 'movsx edi,bl'), where VC6 here sign-extends it once into ebp and
 * reuses that, which drops the 24 re-checks and shifts the frame by one slot
 * (0x40 instead of 0x3c). Measured as inert: an int vs char count, a
 * for-loop vs a pointer do-while band helper, and passing the count by
 * address so it is memory-resident. Left as WIP. */

/* PROGRESS NOTE (this round): the FRAME now matches exactly.  Declaring
 * `seat` (and `b`/`p`) inside the rider-loop's if-block instead of at
 * function level lets VC6 pool `seat` with `off` the way the original does,
 * which took the frame from `sub esp,0x40` to the original's `sub esp,0x3c`
 * and moved the eight-slot collection array back to [esp+0x2c], `screen` to
 * [esp+0x24]/[esp+0x28] and `off`/`seat` to [esp+0x1c]/[esp+0x20].  Every
 * frame reference now agrees; audit mismatch fell 484 -> 476.
 *
 * WHAT IS LEFT (and it is one decision, worth ~10 instructions plus the index
 * shift behind almost all 476): per band the original emits
 *      test bl,bl / jle END / lea esi,[esp+0x2c] / movsx edi,bl
 * -- it re-tests the collected count and re-widens it 22 times.  VC6 here
 * hoists the widening into a callee-saved register once
 * (`movsx ebp,bl` + `mov edi,ebp` per band) and then FOLDS every later
 * `n > 0` test, because once the first `jle` has fallen through it knows
 * n > 0.  The hoist is what enables the fold, and the hoist needs a free
 * callee-saved register: in the original ebp is busy for the whole band
 * section (screen.ox from 0x4085d9, then `mode` from 0x4086e6 to 0x40889b,
 * then the rider cursor), so there is nowhere to put it.  Ruled out here:
 * `for (i = 0; i < n; i++) if (here[i]->action == K)` in place of the inline
 * helper (that DOES restore a per-band guard, but on the widened int, and
 * costs 567 instructions), `i = n; if (i > 0)`, an end-pointer walk, int/char
 * temporaries for screen.ox/oy used in one, some or all of the sums, and a
 * local copy of `mode` (VC6 rematerialises the parameter from its slot at
 * every PrintSprite instead of parking it in ebp).  Find what makes VC6 hold
 * `mode` in ebp across the three band-group PrintSprite calls and the rest
 * should fall out. */
/* ROOT CAUSE of the 10 missing instructions (measured this round with a
 * difflib alignment of the two full bodies -- scratchpad/finish/f2/align.py).
 * They are not missing behaviour: they are five copies of the per-band guard
 *     test bl, bl / jle <end of the band group>
 * that the original emits in front of EVERY band and we emit only once.  The
 * whole divergence is ONE register decision and it cascades:
 *   * the original never lets `n` (the collected-people count, a char) out of
 *     bl.  Each band therefore re-derives its loop counter with
 *     `movsx edi, bl`, re-tests `test bl,bl / jle`, and -- because no byte
 *     register is free -- compares the band code as an IMMEDIATE,
 *     `cmp byte ptr [eax+0x60], 0x18`.
 *   * VC6 here hoists `movsx ebp, bl` (the `i = n` of the first inlined band)
 *     out of all 20 bands into ebp.  That frees bl, so VC6 then also hoists
 *     the band's compare CONSTANT into it -- `mov bl, 0x18` +
 *     `cmp byte ptr [eax+0x60], bl` -- which destroys `n` and forces the
 *     reload `mov bl,[esp+0x50]` we emit at index 107; and with `n > 0`
 *     already established by the first guard it drops the other guards.
 * So the thing to attack is the `movsx` HOIST (make ebp unavailable, or make
 * the conversion non-invariant), not the band spelling.  Measured dead ends
 * this round: declaring Joust_DrawBand's `code` as `unsigned char` or `char`,
 * and replacing the whole static __inline helper with a function-like MACRO so
 * the band code is a literal from the front end onwards -- both still emit
 * `mov bl,0x18`, because the constant is hoisted by loop-invariant motion out
 * of the do/while, not materialised by the inliner.
 * The first divergence (index 28) is the head of the same chain: the original
 * routes the 8-byte struct return of GetScreenCoordsForObject through ebp
 * (`mov ebp,eax` / `test esi,esi` / `mov [esp+0x24],ebp`) where we store eax
 * straight out; ebp's value is dead two instructions later, so it is a pure
 * allocator artefact -- but it is the only other place ebp is claimed before
 * the bands. */
/* THIS ROUND: the diagnosis above is confirmed by an LCS alignment of the two
 * bodies (scratchpad/finish/f2/lcs.py) -- the ONLY structural difference is
 * the five/twenty-two dropped `test bl,bl / jle` guards plus the `movsx`
 * hoist that enables them, and the `mov ebp,eax` at index 28.  Four more
 * spellings measured and rejected: a `char` loop counter in the band helper
 * (504 instructions -- VC6 then keeps the whole count in a byte register and
 * the shape diverges further), an `int n` helper parameter (567), swapping
 * the helper's `i = n;` and `p = here;` (identical), and a function-like
 * MACRO in place of the static __inline (identical).  Also re-measured: a
 * local `int m = mode;` assigned at the top of the `n != 0` block and used
 * for the three grouped PrintSprite calls is completely inert -- VC6
 * rematerialises the parameter from `[esp+0x70]` at each call whatever the
 * source says, so the only way to free ebp for `mode` is to stop the count
 * hoist, not to make `mode` look more valuable. */
/* THIS ROUND the movsx hoist WAS killed, and with it every one of the 24
 * dropped guards: `Joust_DrawBand` must use its own `Bloke** here` PARAMETER
 * as the walk cursor (`++here`) instead of copying it into a local `p` inside
 * the guard.  Copying it inside the guard leaves the guard block empty, so the
 * `movsx` of the count becomes the block's only loop-invariant value, VC6
 * hoists it into ebp, bl falls free, loop-invariant motion then parks the band
 * CONSTANT in bl (`mov bl,0x18`), `n` is destroyed and the later `test bl,bl`
 * guards go with it.  Walking the parameter puts the queue-address `lea` in
 * the guard block, which blocks the hoist: the count is re-derived per band
 * with `movsx edi,bl`, the band code stays an immediate
 * (`cmp byte ptr [eax+0x60],0x18`) and all 24 guards come back.
 * 528 -> 552 instructions (the original's exact count), mismatch 476 -> 97.
 * This is the SAME helper shape westtown.c's ShopDrawBand already uses, and
 * the residual it leaves is the same one those five banded draws carry.
 *
 * WHAT IS LEFT, 97 of 552, in two groups:
 *  (a) ONE lea/jle TRANSPOSITION PER BAND (24 of them, 2 mismatches each).
 *      The original emits `test bl,bl / jle <group end> / lea esi,here /
 *      movsx edi,bl`; we emit `test bl,bl / lea esi,here / jle / movsx edi,bl`
 *      -- the queue-address lea is the inlined parameter copy, so it lives in
 *      the GUARD block and VC6's pairing scheduler slots it between the test
 *      and its branch (some bands come out `lea / test / jle`, which is the
 *      same fact).  Because our guard blocks are not empty, our `jle`s also
 *      target the next band instead of being threaded to the group end.
 *      Moving the lea into the loop preheader is exactly the `p = here;`
 *      spelling above, which costs the guards -- the two are mutually
 *      exclusive under every spelling measured (see the probe sweep in
 *      scratchpad/finish/probe: 30+ helper shapes, including a pointer-pair
 *      loop, `while (k--)`, `k = n - 1`, an unsigned count, an index cursor,
 *      a nested two-level inline, a macro, a hoisted `B** q = queue` base, and
 *      a dead pre-guard copy -- dead code is deleted BEFORE the fold, so it
 *      cannot be used to keep a guard block non-empty).
 *  (b) TWO PrintSprite ARGUMENT BLOCKS (indices 123-130 and 198-205) where the
 *      original interleaves `push 0` before the coordinate add and holds the
 *      sprite global in the other register: a schedule/rotation, not a
 *      structural difference. */
/* THIS ROUND (97 -> 3).  (1) The lea/jle transposition was never a scheduler
 * artefact: Joust_DrawBand written as `char i; for (i = 0; i < n; i++)
 * if (here[i]->action == code)` -- a CHAR index over here[i], the same shape
 * ridecb1.c's restaurants use -- gives `test bl,bl / jle <group end> /
 * lea esi / movsx edi,bl` with every guard threaded to the group end (97 ->
 * 42).  (2) The five PrintSprite argument blocks: the sums must be spelled
 * `screen.ox + off.ox` (screen FIRST, as ridecb1.c does), which makes the
 * addition's destination the `off` register and lets VC6 load the sprite
 * global before `push ebp` (42 -> 3).
 * WHAT IS LEFT, 3 of 552, indices 301-303, in the rider loop: the original
 * emits `mov [esp+0x14],0 / movsx ecx,[esi+0x3c] / lea eax,[edi+0x24] /
 * mov [esp+0x18],ebx / push eax / mov [eax],ecx` -- the seat.oy store (ebx =
 * the hoisted -0x30) sits BETWEEN the &p->local lea and its push -- where we
 * emit both seat stores first.  Measured inert (all 3 or worse): all 24
 * orders of the four stores (stores keep IR order, the push always glues to
 * the lea), `Offset seat = {0,-0x30}`, `= {0}`, an int[2], an int temp for
 * -0x30, `seat.oy = seat.ox - 0x30`, a pointer `lp = &p->local` (also
 * re-bases the oy store), the seat born inside a static __inline helper (with
 * or without the AdjustOffsetForViewMode call; the whole block as a helper
 * re-allocates the function), a helper storing p->local, `for` vs `while`,
 * splitting the && guard, `(short)` casts, same-width casts as no-code tuples
 * at eleven earlier points, b/p re-read from r (re-allocates), a
 * function-level or pre-loop `int seat_dy = -0x30` variable (the hoisted
 * `mov ebx,-0x30` is the same either way), comma/arithmetic forms that put
 * the seat.oy assignment inside the p->local.ox expression, and `lp =
 * &p->local` taken before the seat stores.  Stores keep IR order and the
 * push always glues to the lea, so the original's IR must have the
 * seat.oy store between the address computation and the ox store -- no
 * C spelling found reaches it. */
/* CLOSED 2026-09-04, 3 -> 0.  Two independent facts, both needed:
 *
 * (1) THE ORIGINAL'S IR, reproduced by `Joust_SeatRider` above.  A
 *     `static __inline` helper's arguments are evaluated into temporaries
 *     BEFORE its body runs, so `H(&seat, &p->local, b->ride_dx)` with body
 *     `{ s->oy = -0x30; o->ox = v; }` yields
 *         [st seat.ox] [ld dx] [lea &p->local] [st seat.oy] [st local.ox]
 *     -- the store sitting between an address computation and its own store,
 *     which NO plain-C statement sequence expresses.  Proved exhaustively:
 *     all 24 store orders, every comma form (in a statement, inside an
 *     argument list, in the assignment's LHS base pointer, and through a
 *     pointer local -- all five normalise to statement order), and a
 *     function-like macro give either the store two slots too EARLY (the old
 *     residual at 301-303) or two slots too LATE.  It is NOT a scheduler
 *     window/phase effect: a dead `*(volatile int*)&mode;` inserted at five
 *     different points upstream (and twice at one of them) shifts the whole
 *     stream by one or two instructions and leaves this group's order
 *     unchanged, so the emitted order here is pure IR order.
 *
 * (2) THE PRICE, and how it is paid.  On its own the helper still compiled to
 *     552 instructions but with 63 mismatches from index 33, because the
 *     hoisted `sq->key` left the eax/ecx/edx scratch rotation and coalesced
 *     into edi (`mov di,[edi]`, sq dying at that load), which shifted the
 *     rotation of the whole rest of the function by one step -- 55 of the 63
 *     were pure naming, and the other 8 were two forced `lea`/`mov`
 *     transpositions that only existed because the `lea`'s destination had
 *     become the register the store still needed.  The cure is the rider
 *     loop's guard: `if (sq->key == r->ride_id) { Bloke* b = r->bloke;
 *     if (b->flags & 0x80) { ... } }` in place of the one `&&` chain.  The
 *     TWO-LEVEL guard with `b` cached between the halves is what does it;
 *     `if (..) if (r->bloke->flags & 0x80)` (no `b` in between) is inert.
 *
 * Also measured this round, all rejected: three other ways to put the key
 * back in the rotation -- `*(volatile unsigned short*)&sq->key` at the
 * collection compare (63 -> 8, but it stops the hoist so the load moves into
 * the loop body), `IP_RenderBlokeIn3DNow(b)` instead of `r->bloke` (63 -> 50,
 * the original really does re-read `r->bloke` for that call), and assigning
 * `r = def->riders` after the declarations (63 -> 31); 48 four- and five-
 * parameter helper shapes covering every evaluation order of the dx load,
 * the dy load and the address lea; and splitting the helper in two.  Every
 * one of those left a second residual at 306-310 (`movsx edx,[esi+0x3e]`
 * coming out ecx) which the guard fix does not have. */
// FUNCTION: LEGOLAND 0x00408580
void Joust_Draw(RideElem* elem, int x, int y, RideTile* sq, void* clip, int mode)
{
    RideDef*   def = elem->data;
    RiderNode* r = def->riders;
    Bloke*     here[8] = { 0 };
    char       n = 0;
    JoustRec*  rec;
    char       frame;
    Offset     screen;
    Offset     off;

    rec = Joust_FindRecord(sq);
    if (rec == 0)
        return;
    frame = (char)rec->frame;
    screen = GetScreenCoordsForObject(sq, def);
    if (r) {
        do {
            if (sq->key == r->ride_id)
                here[n++] = r->bloke;
            r = r->next;
        } while (r);

        if (n != 0) {
            Joust_DrawBand(here, n, 0x18);
            Joust_DrawBand(here, n, 0x19);
            Joust_DrawBand(here, n, 0x1a);
            Joust_DrawBand(here, n, 0x1b);
            Joust_DrawBand(here, n, 0x1c);
            off = GetRenderOffsetForLayer(g_joust_sprite, 0);
            AdjustOffsetForViewMode(&off);
            PrintSprite(g_joust_specr, screen.ox + off.ox, screen.oy + off.oy,
                        mode, 0);

            Joust_DrawBand(here, n, 0x0c);
            Joust_DrawBand(here, n, 0x0d);
            Joust_DrawBand(here, n, 0x0e);
            Joust_DrawBand(here, n, 0x0f);
            off = GetRenderOffsetForLayer(g_joust_sprite, 0);
            AdjustOffsetForViewMode(&off);
            PrintSprite(g_joust_specl, screen.ox + off.ox, screen.oy + off.oy,
                        mode, 0);

            Joust_DrawBand(here, n, 0x16);
            Joust_DrawBand(here, n, 0x17);
            Joust_DrawBand(here, n, 0x1d);
            LLSSetFrame(GetLLSForLayer(g_joust_sprite, 1), frame);
            off = GetRenderOffsetForLayer(g_joust_sprite, 1);
            AdjustOffsetForViewMode(&off);
            PrintSprite(GetSpriteForLayer(g_joust_sprite, 1),
                        screen.ox + off.ox, screen.oy + off.oy, mode, 0);

            r = def->riders;
            while (r) {
                if (sq->key == r->ride_id) {
                    Bloke* b = r->bloke;
                    if (b->flags & 0x80) {
                        Person3D* p = b->person;
                        Offset    seat;
                        seat.ox = 0;
                        Joust_SeatRider(&seat, &p->local, b->ride_dx);
                        p->local.oy = b->ride_dy;
                        AdjustBlokePosition(&p->local);
                        AdjustOffsetForViewMode(&seat);
                        p->screen.ox = b->ride_dx + seat.ox + screen.ox;
                        p->screen.oy = b->ride_dy + seat.oy + screen.oy;
                        AdjustBlokePosition(&p->screen);
                        IP_RenderBlokeIn3DNow(r->bloke);
                    }
                }
                r = r->next;
            }

            Joust_DrawBand(here, n, 0x10);
            Joust_DrawBand(here, n, 0x0b);
            Joust_DrawBand(here, n, 0x0a);
            Joust_DrawBand(here, n, 0x01);
            Joust_DrawBand(here, n, 0x02);
            Joust_DrawBand(here, n, 0x03);
            Joust_DrawBand(here, n, 0x07);
            Joust_DrawBand(here, n, 0x14);
            Joust_DrawBand(here, n, 0x15);
            Joust_DrawBand(here, n, 0x1e);
            off = GetRenderOffsetForLayer(g_joust_sprite, 1);
            AdjustOffsetForViewMode(&off);
            PrintSprite(g_joust_fmask, screen.ox + off.ox, screen.oy + off.oy,
                        mode, 0);
            return;
        }
    }
    LLSSetFrame(GetLLSForLayer(g_joust_sprite, 1), frame);
    off = GetRenderOffsetForLayer(g_joust_sprite, 1);
    AdjustOffsetForViewMode(&off);
    PrintSprite(GetSpriteForLayer(g_joust_sprite, 1), screen.ox + off.ox,
                screen.oy + off.oy, mode, 0);
}

/* ==========================================================================
 * JOUST, +0xa8 -- THE PER-TICK UPDATE  (0x00407c30, 703 instructions)
 * NOT RECONSTRUCTED HERE. What it does, read out of the disassembly, so the
 * next pass does not have to start cold:
 *
 * The shape is Temple Slide's, one size up: walk the class's rider list
 * (ObjDef+0xcc), skip anyone whose low-level AI is busy (Bloke+0x0e != 0) and
 * switch on the rider's action byte (Bloke+0x60) -- but with THIRTY-ONE cases
 * (0..0x1e; 8, 9, 0x11..0x13 fall straight through) instead of seven, because
 * a Joust has two populations: the two people ON the horses and up to six
 * SPECTATORS in the stands.
 *
 * Before the switch the handler copies the whole per-placement record into
 * locals and writes the record's animation frame straight into the z-sprite's
 * layer header (`g_joust_zspr->layers[0] = (short)rec->frame`), which is why
 * the record is the ride's animation state and not just bookkeeping. That
 * copy is what names the JoustRec fields above:
 *
 *     +0x0c int    "a joust is running" -- case 1 branches on it and case 2
 *                  decrements it
 *     +0x10 u8     number of jousters committed
 *     +0x11 u8     number of spectators seated (capped at 6)
 *     +0x12 u8[6]  the six stand seats: 0 free, 2 claimed
 *     +0x18 u8[2]  the two horses: 0 free, then 1/2/3 as the rider mounts,
 *                  rides and dismounts
 *     +0x1a u8     which horse the next jouster gets (fed through the
 *                  0x00407c20 predicate below, so 0 -> horse 0, else horse 1)
 *     +0x1b u8     the arena animation frame pushed into the z-sprite
 *     +0x1c, +0x20 int
 *
 * A spectator's path is: case 0 walk to (tile+1, tile-1); case 1, if no joust
 * is running walk one tile further and wait, otherwise pick a seat at random
 * (`rand() % 6`, then scan forward for a free one), claim it, bump the seated
 * count and jump to state 0x0a for seats 0..2 or 0x14 for seats 3..5 -- the
 * two sides of the arena; the 0x0a..0x10 and 0x14..0x1e blocks are the
 * walk-to-your-seat, sit, watch (a 150-tick countdown on Bloke+0x58) and
 * leave steps for each side.
 *
 * A jouster's path is: case 2 claims a horse, records its index in Bloke+0x44
 * and starts the walk to it; case 3 turns on the "riding" flag (+0x62 bit
 * 0x80), hands the 3D person the z-sprite (Bloke->person +0x2c / +0x30 = 1)
 * and calls SetBlokePositionFromBNV (0x00484a70) with the joust .bnv
 * (g_joust_binv) and an object name built ON THE STACK: the template
 * "manBox??" is copied from 0x004b470c into a 9-byte local and
 * sprintf(&name[6], "%02d", horse + 1) turns it into "manBox01" / "manBox02".
 * Cases 4..6 step the ride by re-calling SetBlokePositionFromBNV with the next
 * BNV frame, hold for 150 ticks and then walk the rider off; case 7 sends a
 * rider straight to state 0x1e (leave).
 *
 * The two float constants handed to SetBlokePositionFromBNV are -1617735.0f
 * and -1617993.5f -- the near/far z of the joust model, the same pair of
 * magnitudes Temple Slide uses for its chute (-1617664.875 / -1617913.0).
 *
 * TWO PHASES, not one (read out of the disassembly this round; the note above
 * only described the first).  The body is `sub esp,0x54` + push ebx/ebp/esi/edi
 * and runs 0x00407c30..0x004084ff, the `ret` at 0x00408486 being the epilogue
 * of the FIRST phase's fallthrough with the last block laid out after it:
 *
 *   PHASE 1 (0x00407c74..0x004083bf) -- the rider state machine described
 *   above.  Each iteration begins by calling Joust_FindRecord on the rider's
 *   square and, IF THERE IS NO RECORD, returns from the whole handler
 *   (`test ebx,ebx / je <epilogue>`) -- it does not `continue` to the next
 *   rider and it does not run phase 2 either.  Reproduce that: a rider whose
 *   ride has been removed under it silently freezes the arena for that frame.
 *   The record's mutable fields are copied into frame locals at the top of
 *   every iteration and written back at 0x00408372 (the shared tail every case
 *   jumps to), so the switch works on the copy, not on the record.
 *
 *   PHASE 2 (0x004083c5..0x004084fd) -- a SECOND loop, over the whole record
 *   list `g_joust_head`, that drives the arena animation and its looping
 *   sound.  Per record it reads +0x1a (call it `phase`), the two bytes at
 *   +0x18/+0x19 (the horse states, as a word), +0x0c, +0x10 (jousters) and
 *   +0x20, then:
 *     - phase == 0    : evaluate HORSE 0 (the low byte of +0x18)
 *     - phase == 0x20 : evaluate HORSE 1 (the high byte of +0x18)
 *     - otherwise     : mid-cycle, just run
 *   so +0x1a is a 0..0x3f CYCLE COUNTER and the two horses are half a cycle
 *   apart -- NOT "which horse the next rider gets", which is what the field
 *   comment above still says; the 0x00407c20 predicate is used on it in phase
 *   1 only.  The evaluation is the same three-way test for both horses:
 *   `if ((+0x0c && jousters >= 2) || jousters == 0) { if (h == 1) go; if (h == 3) go; }
 *    else if (h == 0) hold;` -- "go" falls into the RUN arm, "hold" into STOP:
 *     RUN  : if the record has no sample yet, start one --
 *            PlayInstanceOfSample(g_0x004b4690, 1, 0, &src) with `src` the
 *            usual RideSoundSource {kind = 2, x, y} built on the stack (its
 *            +0x04 left uninitialised exactly as Joust_Remove leaves it),
 *            store the handle in rec->sample (+0x08) and set the sample's
 *            0x20 flag (0x00496d10, `sample->flags |= 0x20` = loop);
 *            then phase++ and wrap `if (phase > 0x3f) phase = 0`, and clear
 *            BOTH +0x1c and +0x20.
 *     STOP : if the record has a sample, fade it out --
 *            UnSourceAndFadeAllSamplesFromSource(&src, -1000) -- and clear
 *            rec->sample; leave the counter where it is and set +0x1c (horse
 *            0's gate) or +0x20 (horse 1's gate) to 1.
 *   The writeback is horse word, phase, +0x1c, +0x20; +0x0c and +0x10 are read
 *   only.  So +0x1c/+0x20 are per-horse "waiting at the gate" flags recomputed
 *   every tick and consumed by phase 1, and the arena sound is a property of
 *   the record, started and faded by this loop alone.
 * ========================================================================== */

/* The one-line predicate the +0xa8 handler uses to turn the record's
 * "next horse" byte into a 0/1 index. */

// FUNCTION: LEGOLAND 0x00407c20
int Joust_ByteIsSet(char v)
{
    return v != 0;
}

extern void Joust_Update(RideElem* elem);                    /* 0x00407c30 */
extern void SetBlokePositionFromBNV(void* bin, Bloke* b, const char* name, int frame, float near_z, float far_z, int flag); /* 0x00484a70 */
