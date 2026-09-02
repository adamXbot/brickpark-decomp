#include "legoland.h"

/* Small-leaf sweep, chunk 5: sound-system + sprite/music accessors.
 * Each function is matched to 100% normalised instruction match (to the first
 * `ret`) vs original/legoland.exe.  Struct offsets are load-bearing; field and
 * type names are ours.  Several of these are the early-return prologue of a
 * larger routine — only the code up to the first `ret` is compared, so the body
 * below the early exit is a minimal stand-in that reproduces the real prologue
 * (frame size, saved registers, branch direction). */

/* --- globals touched (addresses documented; names are ours) --------------- */
extern void* g_music_sys;       /* 0x004bf774  music engine instance */
extern void* g_sample_sys;      /* 0x007988c0  sample source system */
extern int   g_gardener_count;  /* 0x0079a8bc  active gardener count */
extern int   g_mechanic_count;  /* 0x0079a8cc  active mechanic count */

/* Anchors that keep the stand-in bodies from being optimised away (they live
 * below the compared first `ret`, so their exact shape is immaterial). */
extern void  snd_emit(void* buf, int n);
extern int   snd_result(void* a, void* b);

/* --- local struct layouts (offsets are the only load-bearing part) -------- */
typedef struct SndObj SndObj;
typedef int (*SndLockFn)(SndObj*, void*, void*);
typedef struct SndVtbl { char pad[0x24]; SndLockFn lock; } SndVtbl; /* +0x24 */
struct SndObj { SndVtbl* vt; };                                      /* +0x00 */

typedef struct ImageRec {
    char           pad[0xc];
    unsigned short refcount;    /* +0x0c */
} ImageRec;

typedef struct SpriteRec {
    char     pad[0x10];
    unsigned flags;             /* +0x10 */
} SpriteRec;

typedef struct SubSprite {
    char           pad[0x14];
    unsigned short src_x;       /* +0x14 */
    unsigned short src_y;       /* +0x16 */
    unsigned short dst_x;       /* +0x18 */
    unsigned short dst_y;       /* +0x1a */
} SubSprite;

/* ========================================================================= */

// FUNCTION: LEGOLAND 0x00496f20
void* GetVRAMAddress(void* p)
{
    return (char*)p + 4;
}

// FUNCTION: LEGOLAND 0x00497150
void MarkSpriteResized(SpriteRec* p)
{
    p->flags |= 0x400;
}

// FUNCTION: LEGOLAND 0x00497500
unsigned short ReferenceImage(ImageRec* p)
{
    return ++p->refcount;
}

// FUNCTION: LEGOLAND 0x00497d90
void SetSubSpriteSource(SubSprite* p, unsigned short dx, unsigned short dy,
                        unsigned short sx, unsigned short sy)
{
    p->dst_x = dx;
    p->dst_y = dy;
    p->src_x = sx;
    p->src_y = sy;
}

/* Three parameters, not two: every caller (TellAllLayersToAnimate /
 * TellAllLayersToStopAnimating in sprite2.c) pushes (sprite, layer, state).
 * The body is a bare ret, so the arity is invisible in the codegen here. */
// FUNCTION: LEGOLAND 0x00497ed0
void SetLayerAnimatingState(void* sprite, int layer, int state)
{
    (void)sprite;
    (void)layer;
    (void)state;
}

