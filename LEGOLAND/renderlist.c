/* LEGOLAND — render list / people drawing.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; the field and type names are ours.
 * Each matched function carries its original VA marker on the line above it.
 *
 * ---------------------------------------------------------------------------
 * THE RENDER LIST
 *
 * The game keeps TWO depth-sorted display lists, built fresh every frame out of
 * bump-allocated arenas (no free, no malloc — the arena pointer is simply reset
 * at the top of the frame by RenderItems_New / RenderItems2_New in sweep1.c):
 *
 *      list 1 : arena base 0x00630108, bump ptr @ 0x0062feec, count @ 0x00655a4c
 *      list 2 : arena base 0x00638218, bump ptr @ 0x0062fef0, count @ 0x00655a50
 *
 * Each item is 16 bytes:
 *
 *      +0x00  int          key      depth / sort key (descending, see below)
 *      +0x04  void*        payload  the thing to draw
 *      +0x08  RenderItem*  next
 *      +0x0c  RenderItem*  prev
 *
 * and a "list" is just a one-word holder whose first field is the head pointer
 * (RenderItem2_Link @ 0x00443080 takes `RenderList*` and reads/writes *list).
 *
 * Insertion (RenderItem2_Link) is an insertion sort walking `next` while
 * key > node->key, so the chain ends up ordered by DESCENDING key, and the
 * head is the largest key. Walking `next` from the head therefore visits far
 * (high key) before near (low key) — a painter's-algorithm back-to-front draw
 * order. RenderBlokeList (below) is exactly that walk.
 * --------------------------------------------------------------------------- */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* A "bloke" (worker/visitor) record; stride 172 (0xac). Same layout as the one
 * in blokemisc.c / spritemisc.c — only the fields touched here are named.
 *
 *   flags62 bit 0x80  suppress: do not update / do not draw
 *   flags62 bit 0x20  (second suppress bit; RenderPeople tests 0xa0 = both) */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00  intrusive 3D-people list link */
    void*          person;      /* +0x04  3D person/model handle */
    unsigned char  pad08[0x5a]; /* +0x08..0x61 */
    unsigned char  flags62;     /* +0x62 */
    unsigned char  pad63[0x49]; /* +0x63..0xab */
} Bloke;

/* One entry of the depth-sorted render list (see the header comment). */
typedef struct RenderItem {
    int                 key;      /* +0x00 */
    void*               payload;  /* +0x04 */
    struct RenderItem*  next;     /* +0x08 */
    struct RenderItem*  prev;     /* +0x0c */
} RenderItem;

/* The list holder: one word, the head of the chain. */
typedef struct RenderList {
    RenderItem* head;             /* +0x00 */
} RenderList;

/* A ride/queue "seat slot" record — an intrusive list hanging off a container
 * at +0xcc (the same Container::head offset used in sweep1.c). Each slot binds
 * a bloke to a numbered seat of the ride. */
typedef struct SeatSlot {
    struct SeatSlot* next;   /* +0x00 */
    int              pad4;   /* +0x04 */
    Bloke*           bloke;  /* +0x08 */
    unsigned short   seat;   /* +0x0c  seat / carriage index */
    unsigned short   pade;   /* +0x0e */
} SeatSlot;

/* The owning object (a ride); its slot list head is at +0xcc. */
typedef struct SeatOwner {
    char       pad0[0xcc];   /* +0x00..0xcb */
    SeatSlot*  head;         /* +0xcc */
} SeatOwner;

/* --------------------------------------------------------------- globals -- */

/* Head of the 3D-people list (@ 0x0066b574). */
extern Bloke* g_people_head;

/* ------------------------------------------------------------ prototypes -- */

extern void  IP_RenderBlokeIn3DNow(Bloke* b);                 /* 0x00440010 */
extern void  SortBlokeIn3D(Bloke* b);                         /* 0x0043ffd0 */
extern void  Control3DPeople(void);                           /* 0x004402b0 */
extern void  RenderItem2_Link(RenderList* list, RenderItem* item, int key);
                                                              /* 0x00443080 */
extern RenderItem* RenderItem2_Alloc(void);                   /* 0x00443120 */
extern void  RenderThickBox(int x, int y, int w, int h, int t, int colour);
                                                              /* 0x00489390 */
#ifndef LEGOLAND_PORTABLE
extern void  RenderSpriteScaledOffset(Sprite* sprite, int x, int y,
                                      int w, int h, Pos* offset);
                                                              /* 0x00488c80 */
#else
extern int   RenderSpriteScaledOffset(Sprite* sprite, int x, int y,
                                      int w, int h, Pos* offset);
                                                              /* 0x00488c80 */
#endif

/* -------------------------------------------------------------- functions -- */

/* -------------------------------------------------------------------------
 * 0x004430f0 -- bump-allocate an item for list 2, fill it in and insert it in
 * depth order. The key is stored in the item AND passed to the linker, which
 * uses it for the insertion-sort compare (it does not re-read item->key).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x004430f0
void RenderItem2_AddItem(RenderList* list, void* payload, int key)
{
    RenderItem* item = RenderItem2_Alloc();
    item->payload = payload;
    item->key = key;
    RenderItem2_Link(list, item, key);
}

/* -------------------------------------------------------------------------
 * 0x00442f70 -- draw a whole render-list chain, back to front. Items with a
 * null payload are skipped (the slot was allocated but never bound).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00442f70
void RenderBlokeList(RenderList* list)
{
    RenderItem* item = list->head;
    while (item) {
        Bloke* bloke = (Bloke*)item->payload;
        if (bloke)
            IP_RenderBlokeIn3DNow(bloke);
        item = item->next;
    }
}

/* -------------------------------------------------------------------------
 * 0x00483130 -- the per-frame people pass: first push every person's world
 * position into its 3D model (Control3DPeople), then submit each visible one
 * to the depth sorter. flags62 & 0xa0 means "hidden" (0x80 suppressed,
 * 0x20 e.g. inside a ride vehicle), and those are not sorted at all — the ride
 * draws its own occupants later via RenderBlokesNotInSeats.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00483130
void RenderPeople(void)
{
    Bloke* p;

    Control3DPeople();
    p = g_people_head;
    while (p) {
        if (!(p->flags62 & 0xa0))
            SortBlokeIn3D(p);
        p = p->next;
    }
}

/* -------------------------------------------------------------------------
 * 0x00441b60 -- draw the blokes riding one particular seat/carriage of a ride.
 *
 * The ride owns an intrusive list of seat slots at +0xcc; each slot names a
 * bloke and the seat index it occupies. `seat` is a POINTER to the wanted
 * index and is re-read on every iteration (it aliases whatever the caller is
 * animating), which is why VC6 keeps it in edi rather than folding it.
 *
 * Note the split prologue: `push esi` is in the real prologue (esi holds the
 * head, loaded before the guard) but `push edi` / `pop edi` are sunk into the
 * non-empty path, because `seat` is only live inside the loop.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00441b60
void RenderBlokesNotInSeats(SeatOwner* owner, unsigned short* seat)
{
    SeatSlot* slot = owner->head;
    while (slot) {
        if (*seat == slot->seat) {
            Bloke* bloke = slot->bloke;
            if (!(bloke->flags62 & 0x80))
                IP_RenderBlokeIn3DNow(bloke);
        }
        slot = slot->next;
    }
}

/* -------------------------------------------------------------------------
 * 0x00489410 -- a 1-pixel-thick rectangle outline; RenderThickBox does the
 * work as four RenderBlock fills.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00489410
void RenderBox(int x, int y, int w, int h, int colour)
{
    RenderThickBox(x, y, w, h, 1, colour);
}

/* -------------------------------------------------------------------------
 * 0x00489080 -- blit a sprite stretched into the given rectangle. The worker
 * takes an extra {x,y} bias applied to the destination origin; this entry
 * point passes a zeroed one. Always reports success.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00489080
int RenderScaledSprite(Sprite* sprite, int x, int y, int w, int h)
{
    Pos offset;

    offset.x = 0;
    offset.y = 0;
    RenderSpriteScaledOffset(sprite, x, y, w, h, &offset);
    return 1;
}
