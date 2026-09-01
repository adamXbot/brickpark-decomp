/* LEGOLAND - bloke appearance, render-list membership and long-term actions.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; the field and type names are ours.
 * Each matched function carries its original VA marker on the line above it.
 *
 * ---------------------------------------------------------------------------
 * HOW A BLOKE GETS DRAWN
 *
 * Blokes reach the screen by two different routes.
 *
 * 1. Free-walking blokes. RenderPeople (renderlist.c) walks the intrusive
 *    3D-people chain off Bloke::next and hands each visible one to the depth
 *    sorter. Nothing in this file is involved.
 *
 * 2. Blokes attached to a ride. A ride owns a doubly linked list of SEAT SLOTS
 *    hanging off owner+0xcc; each 20-byte slot binds one bloke to one numbered
 *    seat/carriage. The ride's own draw routine walks that list, and for every
 *    slot whose seat index matches the carriage being drawn (and which passes a
 *    depth test on the bloke's fixed-point Y at +0x68) calls
 *    AddBlokeToRenderList, then flushes the whole thing with RenderBlokeList:
 *
 *        slot = owner->head;                        // 0x0042da29
 *        while (slot) {
 *            if (*seat == slot->seat && slot->bloke->y68 < limit)
 *                AddBlokeToRenderList(&g_ride_list, slot, slot->owner->key);
 *            slot = slot->next;
 *        }
 *        RenderBlokeList(&g_ride_list);
 *
 *    So AddBlokeToRenderList takes the SLOT, not the bloke: it pulls
 *    slot->bloke (+0x08) out as the render item's payload, which is what
 *    RenderBlokeList later feeds to IP_RenderBlokeIn3DNow. The sort key comes
 *    from the caller (slot->owner->+0x20, the carriage's depth), and the item
 *    itself is bump-allocated out of render arena 1 (RenderItem_Alloc,
 *    0x00442f50: arena ptr @ 0x0062feec, count @ 0x00655a4c) - the list-1 twin
 *    of the list-2 pair documented in renderlist.c.
 *
 * The list holders are static one-word globals, one per ride draw site
 * (e.g. 0x00610a18, 0x006160ec), reset with the arena at the top of each frame.
 *
 * RemoveBlokeFromList is the tear-down half: it unlinks a slot from that same
 * +0xcc chain. Callers free(slot) immediately afterwards (0x0043cfa7), and the
 * slots are malloc'd 20 bytes at a time by the "join" routine at 0x0044f4a0.
 *
 * ---------------------------------------------------------------------------
 * BLOKE APPEARANCE
 *
 * A minifig's look is five values pulled out of its 3D person record and
 * dropped into a global "appearance" block by the builder at 0x00421890:
 *
 *      0x004b59bc  the bloke pointer
 *      0x004b5988  sex          GetSexOfBloke()      normalised to 0/1
 *      0x004b598c  leg colour   GetLegColourOfBloke()  & 0x00ffffff
 *      0x004b5990  arm colour   GetArmColourOfBloke()  & 0x00ffffff
 *      0x004b5994  chest texture name (20 bytes, strcpy'd)
 *      0x004b59a8  face texture name  (strcpy'd)
 *
 * The two colour getters are the same function twice over, reading a PALETTE
 * INDEX out of the person (+0x8c legs, +0x90 arms) and expanding it through the
 * 3-bytes-per-entry RGB table at 0x004b7ac0 into a packed 0xRRGGBB int. The
 * table is the LEGO brick palette, 8 entries:
 *
 *      0 #000000 black     4 #bdcede light grey
 *      1 #007bc6 blue      5 #f71821 red
 *      2 #732910 brown     6 #ffffff white
 *      3 #008c4a green     7 #ffd600 yellow
 *
 * (0x004b7ad8 onward is unrelated data - two 1.0f floats - so the table really
 * does stop at 8.) The model builder at 0x004426d0 unpacks the same entries
 * byte-by-byte into a material triple and defaults to index 3 (green).
 *
 * ---------------------------------------------------------------------------
 * LONG-TERM ACTIONS
 *
 * A bloke carries two action codes. The SHORT-term one is the byte at +0x60
 * (walk, sit, eat...; ride draw code brackets it against 0x10 and 0x21). The
 * LONG-TERM one is the word at +0x0c - a plan, not a pose - and it is a direct
 * index into the handler table at 0x004b8368, 26 entries:
 *
 *      0x00 Bloke_DoNothing        0x10 Gardener_Idle
 *      0x01 0x0044f170             0x11 Mechanic_Idle
 *      0x02 0x0044ebf0             0x12 Gardener_Build
 *      0x03 0x0044ed70             0x13 Mechanic_Build
 *      0x05 (null - no handler)    0x15 Garderner_Repair  [sic]
 *      ...                         0x16 Mechanics_Repair
 *
 * There is no "long-term action record"; the plan is just that index plus the
 * state the handler keeps in the bloke. NewLongTermAction is therefore a reset
 * rather than an allocation: it writes the new plan word and wipes the
 * execution state the previous plan left behind - short-term action (+0x60),
 * step (+0x10), low-level AI state (+0x0e) and the state's scratch/timer
 * (+0x1c) - then re-plans immediately via DoHighLevelAI, which dispatches
 * through that table and finally sets walk_delay (+0x75) = 1 and clears +0x64.
 *
 * ---------------------------------------------------------------------------
 * LAYERED SPRITES
 *
 * HideLayer/ShowLayer operate on the same three-level chain as
 * GetSpriteForLayer (layers.c) and Spr in llidb_odf.c: a sprite object holds a
 * flag word at +0x10 and a layer holder at +0x08; the holder has a layer count
 * at +0x04 and an array of per-layer sprite objects at +0x08. Flag 0x8000 on
 * the parent means "this sprite really is layered" and is checked first;
 * flag 0x4000 on a layer is its HIDDEN bit - HideLayer sets it, ShowLayer
 * clears it. Both are total: a non-layered parent, an out-of-range index or an
 * empty layer slot are all silently ignored. The sibling at 0x00497e40 does the
 * ShowLayer clear across every layer.
 * --------------------------------------------------------------------------- */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* The renderable "3D person" hanging off a bloke (Bloke::person, +0x04).
 * Same record as Person3D in blokeanim.c; the two colour indices sit just past
 * the animation selectors. */
typedef struct Person3D {
    unsigned char pad00[0x84];  /* +0x00..0x83 */
    int           variant;      /* +0x84  sub-kind (GetSexOfBloke) */
    int           anim;         /* +0x88  current animation index */
    int           leg_colour;   /* +0x8c  palette index for the legs/hips */
    int           arm_colour;   /* +0x90  palette index for the arms/torso */
} Person3D;

/* A "bloke" (worker/visitor); allocation stride 172 (0xac). */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00  intrusive 3D-people list link */
    Person3D*      person;      /* +0x04  the renderable 3D person */
    unsigned char  pad08[4];    /* +0x08..0x0b */
    unsigned short lt_action;   /* +0x0c  LONG-TERM action code */
    unsigned short state;       /* +0x0e  low-level AI state */
    unsigned short step;        /* +0x10  step/phase within the state */
    unsigned char  pad12[0x0a]; /* +0x12..0x1b */
    int            timer;       /* +0x1c  countdown / scratch for the state */
    unsigned char  pad20[0x40]; /* +0x20..0x5f */
    unsigned char  action;      /* +0x60  current SHORT-term action */
    unsigned char  pad61[0x4b]; /* +0x61..0xab */
} Bloke;

/* One entry of a depth-sorted render list; see renderlist.c for the arena. */
typedef struct RenderItem {
    int                 key;      /* +0x00  depth / sort key */
    void*               payload;  /* +0x04  the thing to draw */
    struct RenderItem*  next;     /* +0x08 */
    struct RenderItem*  prev;     /* +0x0c */
} RenderItem;

/* The list holder: one word, the head of the chain. */
typedef struct RenderList {
    RenderItem* head;             /* +0x00 */
} RenderList;

/* A "seat slot" - a doubly linked membership record binding one bloke to one
 * numbered seat/carriage of a ride. Same record RenderBlokesNotInSeats walks
 * (renderlist.c), seen here with its back link. */
typedef struct SeatSlot {
    struct SeatSlot* next;   /* +0x00 */
    struct SeatSlot* prev;   /* +0x04 */
    Bloke*           bloke;  /* +0x08 */
    unsigned short   seat;   /* +0x0c  seat / carriage index */
    unsigned short   pad0e;  /* +0x0e */
    void*            owner;  /* +0x10  the ride; its +0x20 is the depth key the
                              *        draw code passes to AddBlokeToRenderList.
                              *        20 bytes total - the size the slot
                              *        allocator at 0x0044f4a0 mallocs. */
} SeatSlot;

/* The object that owns a seat-slot list; the head is at +0xcc. */
typedef struct SeatOwner {
    unsigned char pad00[0xcc]; /* +0x00..0xcb */
    SeatSlot*     head;        /* +0xcc */
} SeatOwner;

/* One entry of the 3-byte-per-colour bloke palette at 0x004b7ac0. */
typedef struct BlokeColour {
    unsigned char r;  /* +0x00 */
    unsigned char g;  /* +0x01 */
    unsigned char b;  /* +0x02 */
} BlokeColour;

/* A live LLS (sprite animation) instance; its control flags are at +0x14. */
typedef struct LLS {
    unsigned char pad00[0x14]; /* +0x00..0x13 */
    unsigned int  flags;       /* +0x14  bit 2 = stop at the end (play once) */
} LLS;

/* A layered sprite object. Same record as Spr in llidb_odf.c and RenderObj in
 * legoland.h: the layer holder is at +0x08 and the flag word at +0x10. */
typedef struct SprObj {
    unsigned char  pad00[8];   /* +0x00..0x07 */
    struct SprLayers* layers;  /* +0x08 */
    unsigned char  pad0c[4];   /* +0x0c..0x0f */
    unsigned int   flags;      /* +0x10  0x8000 = has layers,
                                *        0x4000 = this layer is hidden */
} SprObj;

/* The layer holder (Layers in legoland.h / Anim in llidb_odf.c): a count and a
 * parallel array of per-layer sprite objects. */
typedef struct SprLayers {
    unsigned char pad00[4];  /* +0x00 */
    int           count;     /* +0x04  number of layers */
    SprObj**      sprites;   /* +0x08  one SprObj per layer */
} SprLayers;

/* ---------------------------------------------------------------- globals -- */

/* The bloke colour palette (@ 0x004b7ac0): 3 bytes per entry, indexed by the
 * leg/arm colour indices held in the 3D person. */
extern BlokeColour g_bloke_palette[];

/* ------------------------------------------------------------ prototypes -- */

/* Bump-allocate one item of render list 1 (arena ptr @ 0x0062feec, count
 * @ 0x00655a4c) - the list-1 twin of RenderItem2_Alloc. */
extern RenderItem* RenderItem_Alloc(void);                       /* 0x00442f50 */
/* Insertion-sort one item into a list by descending key. */
extern void RenderItem_Link(RenderList* list, RenderItem* item, int key);
                                                                 /* 0x00442eb0 */
/* Re-plan a bloke straight away with its new long-term action. */
extern void DoHighLevelAI(Bloke* b);                             /* 0x004504d0 */
/* Start an LLS animation playing on a sprite header. */
extern void LLSPlay(LLS* lls, void* anim);                       /* 0x0047d520 */

/* -------------------------------------------------------------- functions -- */

/* -------------------------------------------------------------------------
 * 0x00442f20 -- submit one seated bloke to a ride's render list. Bump-allocate
 * an item out of arena 1, point it at the SLOT's bloke, stamp the caller's
 * depth key into it and insertion-sort it in. `key` is stored in the item AND
 * passed to the linker, which compares against it directly rather than
 * re-reading item->key -- the same two-places convention as
 * RenderItem2_AddItem (renderlist.c).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00442f20
void AddBlokeToRenderList(RenderList* list, SeatSlot* slot, int key)
{
    RenderItem* item = RenderItem_Alloc();

    item->payload = slot->bloke;
    item->key = key;
    RenderItem_Link(list, item, key);
}

/* -------------------------------------------------------------------------
 * 0x0044f470 -- unlink one seat slot from its owner's chain. A textbook
 * doubly-linked delete: no prev means the slot is the head, so the head moves
 * to slot->next; otherwise the predecessor is rewired. The successor's back
 * link is fixed in a separate guarded step because the slot may be the tail.
 *
 * The node is NOT freed here -- callers do that themselves (0x0043cfa7 calls
 * free() on it right after) and often follow up with NewLongTermAction to give
 * the evicted bloke something else to do.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0044f470
void RemoveBlokeFromList(SeatOwner* owner, SeatSlot* slot)
{
    if (slot->prev == 0)
        owner->head = slot->next;
    else
        slot->prev->next = slot->next;
    if (slot->next != 0)
        slot->next->prev = slot->prev;
}

/* -------------------------------------------------------------------------
 * 0x004431f0 -- expand the bloke's leg palette index into a packed 0xRRGGBB.
 *
 * VC6 builds the result in two registers: the red and green bytes are packed
 * into one (ch then cl, then a single `shl 8`) while blue lands in the other,
 * and the two are OR'd. Getting that allocation right needed the palette
 * subscript written out three times against a local `person` pointer -- hoist
 * `p->leg_colour` into an `int i` and the person pointer needs a scratch
 * register of its own, which pushes the accumulator from ecx to edx and stops
 * the function matching even though it is the same 12 instructions.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004431f0
int GetLegColourOfBloke(Bloke* b)
{
    Person3D* p = b->person;

    return (g_bloke_palette[p->leg_colour].r << 16) |
           (g_bloke_palette[p->leg_colour].g << 8) |
            g_bloke_palette[p->leg_colour].b;
}

/* -------------------------------------------------------------------------
 * 0x00443220 -- the arm/torso twin of GetLegColourOfBloke, byte for byte, off
 * +0x90 instead of +0x8c.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00443220
int GetArmColourOfBloke(Bloke* b)
{
    Person3D* p = b->person;

    return (g_bloke_palette[p->arm_colour].r << 16) |
           (g_bloke_palette[p->arm_colour].g << 8) |
            g_bloke_palette[p->arm_colour].b;
}

/* -------------------------------------------------------------------------
 * 0x0044e760 -- give a bloke a new plan and re-plan it on the spot.
 *
 * The action code is a WORD parameter, not an int: the original reads it with
 * `mov cx, [esp+8]`, so declaring it `short` here is load-bearing even though
 * every call site (e.g. EraseGardenerOrder, workorder.c) pushes a full dword.
 *
 * Writing the plan is only half the job -- the four zeroed fields are the
 * previous plan's execution state, and leaving any of them would make the new
 * handler resume mid-way through the old job.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0044e760
void NewLongTermAction(Bloke* b, short action)
{
    b->lt_action = action;
    b->action = 0;
    b->step = 0;
    b->state = 0;
    b->timer = 0;
    DoHighLevelAI(b);
}

/* -------------------------------------------------------------------------
 * 0x0047d580 -- start an animation and mark it self-stopping: LLSPlay does the
 * work, then bit 2 of the instance's flag word is the "stop when the frame
 * counter wraps" request the LLS tick consumes.
 * ------------------------------------------------------------------------- */

/* LLSPlayOnce (0x0047d580) is NOT here: it belongs to the LLS cluster in
 * LEGOLAND/layervis.c (0x0047d4c0-0x0047d6a0), which owns its neighbours.
 * Both files had reconstructed it identically; keeping one copy so the
 * function is not counted twice. */

/* -------------------------------------------------------------------------
 * 0x00497de0 -- set the hidden bit on one layer of a layered sprite.
 *
 * Three guards, all silent: the parent must actually be layered (0x8000), the
 * index must be in range, and the slot must be occupied. The index compare is
 * UNSIGNED (`jae`), so a negative index is caught by the same test as an
 * over-large one -- write the parameter as `unsigned int` or VC6 emits `jge`
 * and the function is one instruction wrong.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497de0
void HideLayer(SprObj* obj, unsigned int layer)
{
    SprLayers* layers;
    SprObj*    s;

    if (obj->flags & 0x8000) {
        layers = obj->layers;
        if (layer < (unsigned int)layers->count) {
            s = layers->sprites[layer];
            if (s != 0)
                s->flags |= 0x4000;
        }
    }
}

/* -------------------------------------------------------------------------
 * 0x00497e10 -- the inverse of HideLayer, identical down to the guards; only
 * the final read-modify-write differs (`and ch,0xbf` instead of `or ch,0x40`).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497e10
void ShowLayer(SprObj* obj, unsigned int layer)
{
    SprLayers* layers;
    SprObj*    s;

    if (obj->flags & 0x8000) {
        layers = obj->layers;
        if (layer < (unsigned int)layers->count) {
            s = layers->sprites[layer];
            if (s != 0)
                s->flags &= ~0x4000;
        }
    }
}
