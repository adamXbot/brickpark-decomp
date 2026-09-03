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

extern void AddBasicObject(void* obj, Pos* pos);             /* 0x0045efe0 */

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

extern void StandardRemoveObject(void* obj, unsigned int tile, void* ctx);   /* 0x0045f220 */
extern void RemoveAllBlokesFromRide(RideDef* cls, unsigned int tile);        /* 0x0048a2e0 */
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

// FUNCTION: LEGOLAND 0x00417280
void TempleSlide_Remove(void* obj, unsigned int tile, void* ctx)
{
    TempleSlideRec* rec = TempleSlide_FindRecord((RideTile*)&tile);

    if (rec)
        TempleSlide_RemoveRecord(rec);
    StandardRemoveObject(obj, tile, ctx);
    RemoveAllBlokesFromRide(((RideMapObj*)obj)->cls, tile);
}

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
extern void  KillSprite(void* sprite);                       /* 0x00497bd0 */
extern void* LoadBinV(const char* name);                     /* 0x0044dc90 */
extern void  FreeBinV(void* bnv);                            /* 0x0044dd60 */
extern void  HideLayer(Spr* sprite, int layer);              /* 0x00497de0 */
extern void  StopLayerPlaying(Spr* sprite, int layer);       /* 0x00441f00 */
extern void* GetLLSForLayer(Spr* sprite, int layer);         /* 0x00441ea0 */
extern void* GetLLSForSprite(Spr* sprite);                   /* 0x00441e80 */
extern void  LLSSetFrame(void* lls, int frame);              /* 0x0047d5a0 */

extern FXEntry g_joust_fx[];         /* 0x004b4688  "Joust Horses.wav" */
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
    unsigned char pad2c[4];
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
extern void* NewBNVPath(void* bin, int tag, const char* name, float near_z, float far_z, Pos* pos); /* 0x00484c20 */
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

/* NOTE: all 347 instructions and the whole block layout (both jump tables, the
 * shared CalcMoveLine tail that cases 0, 2 and 5 fall into or jump to, the
 * deferred four-register prologue, the reuse of the argument slot for the
 * ObjDef pointer) are reproduced, but two frame facts are not:
 *   - the original's local area is 0x30 bytes with the slots at entry-0x2c and
 *     entry-0x04 never referenced; this source produces 0x28 with no holes.
 *     Declaring the NewBNVPath position local as a THREE-int vector accounts
 *     for the entry-0x04 slot exactly (frame 0x2c), which is good evidence the
 *     ride hands NewBNVPath a 12-byte vector and not an 8-byte pair; no
 *     declaration order or grouping found so far accounts for entry-0x2c.
 *   - the map square lands in ebx and the ride's base tile y in ebp, where the
 *     original has them the other way round.
 * Both shift every stack displacement and register name, so the strict
 * index-for-index gate rejects it even though the instruction sequence is
 * right. Left as WIP with the semantics recovered. */

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
 * bytes, in declaration order ascending).  This source produces the same
 * aggregate run but only FOUR scalars, hence `sub esp,0x28` (0x2c once `pos`
 * is three ints) against the original's 0x30, and every displacement from `tw`
 * upward is 4 low -- which is the whole of the 317-instruction mismatch, plus
 * the ebx/ebp swap (the original has the map square in ebp and the base tile y
 * in ebx) that follows from it.
 * THE ONE THING STILL MISSING is therefore a FIFTH scalar home, ordered
 * between `r` and `tw`, that the original allocates and never reads.  Evidence
 * about what it is: in case 3 the original emits `mov [esp+0x20],ebp` at
 * 0x004175d2 -- a store of `sx` into F+0x00, i.e. into the rider cursor's own
 * home, which is dead there.  So VC6 DID give `sx` a stack home and lifetime-
 * coloured it onto `r`; the natural reading is that `sy` got a home too and
 * that its (equally dead) store was the one that got eliminated, leaving the
 * bare slot at F+0x04.  Nothing tried this round makes VC6 home either of
 * them: `sx`/`sy` at function scope, in case 3's block, `tw`/`th` and `pos`
 * moved into case 3's block, `ofs` into case 5's block, all six declaration
 * permutations, and `ofs` as two plain ints instead of an Offset -- every one
 * still emits four scalar homes.  (A 12-byte `ofs` with a dead leading member
 * DOES produce `sub esp,0x30` with ofs/screen/pos landing on the original's
 * homes exactly, and drops the first divergence from index 0 to index 16 --
 * but it is a lie about the data structure and leaves tw/th/next 4 low, so it
 * is not shipped.  It does prove the aggregate run is modelled correctly and
 * isolates the residual to that one scalar.) */
/* THIS ROUND (semantic fix + block-layout study; audit 317 -> 329 -> 231).
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
 * 3. FRAME: unchanged from the analysis above (two never-referenced homes at
 *    F+0x04 and F+0x2c; `pos` as a 3-int vector accounts for F+0x2c exactly
 *    and ridecb3.c adopted that for NewBNVPath, but here it does not move the
 *    strict count; a Pos scr struct for sx/sy, a fresh sx2, wx/wy pre-reads
 *    and their combinations never produce the original's dead `mov
 *    [esp+0x20],ebp` spill of sx or the extra home).  mechrides.c's four BNV
 *    rides carry exactly the same residual (dead homes + dead sx spill), so
 *    it is one family-wide unknown, not a per-function spelling. */
// WIP-FUNCTION: LEGOLAND 0x00417430  (347/347 instructions, audit mismatch 231/347: frame two homes short and the shared-tail layout -- see above)
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
    Pos        pos;
    int        tw;
    int        th;
    int        sx;
    int        sy;

    r = def->riders;
    while (r) {
        next = r->next;
        sq = (RideTile*)&r->ride_id;
        b = r->bloke;
        tx = def->base_x + sq->b.x;
        ty = def->base_y + sq->b.y;
        if (b->state == 0) {
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
move:
                dir = (unsigned char)CalcMoveLine(b->world, b->target,
                                                  b->path) + 0x10;
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
                screen = GetScreenCoordsForObject(sq, def);
                GetTileDimensions(&tw, &th);
                sx = (b->world.x - b->world.y) * tw >> 9;
                sy = (b->world.x + b->world.y) * th >> 9;
                sx = g_map_cfg->ox - Get_XScroll() + sx;
                sy = sy + (g_map_cfg->oy - Get_YScroll());
                sx -= g_ts_rider_dx / 2;
                sx -= screen.ox;
                b->flags |= 0x80;
                sy -= g_ts_rider_dy / 2;
                sy -= screen.oy;
                pos.x = sx * 2;
                pos.y = sy * 2;
                b->person->zsprite = g_ts_zspr;
                b->person->f30 = 1;
                b->person->depth = GetUnitDepth(-1617664.875f, -1617913.0f);
                b->b35 = 0;
                b->bnvpath = NewBNVPath(g_ts_objsamples, 0,
                                        g_ts_path_names[b->seat],
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
                b->world.x = ofs.ox + ((def->qx + sq->b.x) << 8);
                b->world.y = ofs.oy + ((def->qy + sq->b.y) << 8);
                b->target.x = ((def->qx + sq->b.x) << 8) + 0x80;
                b->target.y = ((def->qy + sq->b.y) << 8) + 0x80;
                goto move;

            case 6:
                TempleSlide_ReleaseLane(b->seat, sq);
                RemoveBlokeFromRide(def, r);
                b->flags &= ~8;
                break;
            }
        }
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
// WIP-FUNCTION: LEGOLAND 0x00408580  (552 of 552 instructions, mismatch 3: the seat.oy store is scheduled between the lea and the push -- see above)
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
                if (sq->key == r->ride_id && (r->bloke->flags & 0x80)) {
                    Bloke*    b = r->bloke;
                    Person3D* p = b->person;
                    Offset    seat;
                    seat.ox = 0;
                    seat.oy = -0x30;
                    p->local.ox = b->ride_dx;
                    p->local.oy = b->ride_dy;
                    AdjustBlokePosition(&p->local);
                    AdjustOffsetForViewMode(&seat);
                    p->screen.ox = b->ride_dx + seat.ox + screen.ox;
                    p->screen.oy = b->ride_dy + seat.oy + screen.oy;
                    AdjustBlokePosition(&p->screen);
                    IP_RenderBlokeIn3DNow(r->bloke);
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
