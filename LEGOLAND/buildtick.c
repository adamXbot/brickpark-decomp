/* LEGOLAND construction / build-progress tick.
 *
 * A fixed table of 256 "under construction" slots (0x006664f8, stride 12) is
 * advanced once per tick by ProcessBuildingTimes. Each slot pairs the object
 * being built with the map tile it was placed on and a tick counter:
 *
 *   slot.timer  ticks elapsed, incremented once per call (units = game ticks)
 *   build time  = clamp(GetObjCost(obj), 50, 150) ticks — cost IS the duration
 *
 * Every tick each occupied slot is bumped; while timer < build time the object
 * is "building" (ObjectIsBuilding keeps the hammering sound alive), and on
 * timer >= build time it is "built" (ObjectIsBuilt frees the reserved cells,
 * plays effect 0x8f8 at the tile centre and hands the object to PutObjOnMap).
 * The slot is then cleared and the live-build counter at 0x006670f8 decremented.
 *
 * The construction animation frame shown for an object is
 *   frame = min(slot.timer * frames / build_time, frames - 1)
 * (GetBuildAnimFrame), where `frames` is the object's LLS frame count — the max
 * over all layers for a multi-layer sprite. */
#include "legoland.h"

/* The map tile an in-progress build occupies, passed BY VALUE as two bytes. */
typedef struct BuildTile {
    unsigned char x; /* +0x00 */
    unsigned char y; /* +0x01 */
} BuildTile;

/* The same two bytes seen as one 16-bit key (x | y<<8), which is how the
   slot table is searched. */
typedef union BuildKey {
    BuildTile tile;
    short     w;
} BuildKey;

struct Obj;

/* One construction slot — 12 bytes, 256 of them at 0x006664f8. */
typedef struct BuildSlot {
    struct Obj* obj; /* +0x00  object under construction (0 = free slot) */
    BuildKey  key;   /* +0x04  map cell it was placed on */
    short     pad6;  /* +0x06 */
    int       timer; /* +0x08  ticks elapsed since construction started */
} BuildSlot;

extern BuildSlot g_build_slots[256]; /* 0x006664f8 */
extern int       g_build_count;      /* 0x006670f8 */

/* An .lls animation record; its frame count is a short at +0x10. */
typedef struct LLS {
    char  pad0[0x10]; /* +0x00 */
    short frames;     /* +0x10  number of animation frames */
} LLS;

/* The layer table hanging off an AnimObj: single-sprite LLS at +0x00, layer
   count at +0x04, per-layer sprite array at +0x08 (see legoland.h Layers). */
typedef struct AnimLayers {
    LLS* lls;         /* +0x00 */
    int  count;       /* +0x04 */
} AnimLayers;

/* The render/animation record an object points at from +0x6c. */
typedef struct AnimObj {
    char        pad0[8];  /* +0x00 */
    AnimLayers* layers;   /* +0x08 */
    int         pad0c;    /* +0x0c */
    int         flags;    /* +0x10  0x8000 = multi-layer sprite */
} AnimObj;

/* A placed map object. Only the fields this lane touches are named. */
typedef void (*ObjEffectFn)(void* cls, Pos* pt, int effect);
typedef struct Obj {
    char        pad0[0x1c];
    int         flags;    /* +0x001c  0x200000 suppresses the build noise */
    char        pad20[0x6c - 0x20];
    AnimObj*    anim;     /* +0x006c  render/animation record */
    char        pad70[0x90 - 0x70];
    ObjEffectFn effect;   /* +0x0090  effect/sound emitter */
    char        pad94[0xc4 - 0x94];
    void*       cls;      /* +0x00c4  ObjClass descriptor */
    char        padc8[0x1404 - 0xc8];
    int         ox;       /* +0x1404  map origin of the footprint */
    int         oy;       /* +0x1408 */
    char        pad140c[0x1414 - 0x140c];
    Rect        rects;    /* +0x1414  footprint rect chain (map-relative) */
    char        pad1428[0x1828 - 0x1428];
    int         state;    /* +0x1828  0x3000 = still being placed/built */
    int         pad182c;  /* +0x182c */
    struct Obj* next;     /* +0x1830  next placed object */
} Obj;

/* The global "view" that the build effect temporarily takes over. */
extern int  g_gfx_flags;   /* 0x00813a40  0x1000 = effect view active */
extern Pos  g_gfx_point;   /* 0x00813a44 */
extern int  g_draw_state;  /* 0x008003f0 */
typedef struct View {
    Pos  pos;   /* +0x00 -> 0x007fffc4 */
    int  c;     /* +0x08 -> 0x007fffcc */
    int  d;     /* +0x0c -> 0x007fffd0 */
    Rect rect;  /* +0x10 -> 0x007fffd4 */
} View;
extern View g_view;        /* 0x007fffc4  saved/restored around the effect */
extern Obj  g_obj_list;    /* 0x007febc0  head of the placed-object list */
extern Obj* g_effect_obj;  /* 0x008119b8 */

/* A sound "source" descriptor: kind 2 = a map tile at (x,y). */
typedef struct SoundSource {
    int kind;  /* +0x00 */
    int pad4;  /* +0x04 */
    int x;     /* +0x08 */
    int y;     /* +0x0c */
} SoundSource;

extern int  GetObjCost(void* obj);                          /* 0x00480da0 */
extern void ObjectIsBuilt(Obj* obj, BuildTile tile);        /* 0x0045ed30 */
extern int  CountSamplesFromSource(SoundSource* src);       /* 0x00496b10 */
extern void PlayAppropriateBuildEffect(Obj* obj, Pos* pos); /* 0x00462d10 */
extern LLS* GetLLSForLayer(AnimObj* anim, int layer);       /* 0x00441ea0 */
extern void UnSourceAndFadeAllSamplesFromSource(SoundSource* src, int fade);
                                                            /* 0x00496c80 */
extern void GetTileCentre(Pos* tile, Pos* out);             /* 0x0045ad60 */
extern void PutObjOnMap(Obj* obj, void* cls, Pos* tile);    /* 0x00459ad0 */

// FUNCTION: LEGOLAND 0x00450c40
int GetBuildTime(void* obj)
{
    int cost = GetObjCost(obj);

    if (cost < 50) {
        return 50;
    }
    if (cost > 150) {
        return 150;
    }
    return cost;
}

// FUNCTION: LEGOLAND 0x0045ef50
void ObjectIsBuilding(Obj* obj, BuildTile tile)
{
    Pos pos;
    SoundSource src;

    pos.x = tile.x;
    pos.y = tile.y;
    if ((obj->flags & 0x200000) == 0) {
        src.kind = 2;
        src.x = tile.x;
        src.y = tile.y;
        if (CountSamplesFromSource(&src) == 0) {
            PlayAppropriateBuildEffect(obj, &pos);
        }
    }
}

// FUNCTION: LEGOLAND 0x00450c80
void ProcessBuildingTimes(void)
{
    int i;

    for (i = 0; i < 256; i++) {
        if (g_build_slots[i].obj != 0) {
            g_build_slots[i].timer++;
            if (g_build_slots[i].timer >= GetBuildTime(g_build_slots[i].obj)) {
                g_build_count--;
                ObjectIsBuilt(g_build_slots[i].obj, g_build_slots[i].key.tile);
                g_build_slots[i].obj = 0;
            } else {
                ObjectIsBuilding(g_build_slots[i].obj, g_build_slots[i].key.tile);
            }
        }
    }
}

// FUNCTION: LEGOLAND 0x00450cf0
int GetBuildAnimFrame(Obj* obj, short key)
{
    int frames;
    int i;

    for (i = 0; i < 256; i++) {
        if (g_build_slots[i].key.w == key) {
            break;
        }
    }

    if (obj->anim->flags & 0x8000) {
        int best = 0;
        int layer;

        for (layer = 0; layer < obj->anim->layers->count; layer++) {
            LLS* lls = GetLLSForLayer(obj->anim, layer);
            if (lls != 0 && lls->frames > best) {
                best = lls->frames;
            }
        }
        frames = best;
    } else {
        LLS* lls = obj->anim->layers->lls;
        if (lls == 0) {
            return 0;
        }
        frames = lls->frames;
    }

    {
        int frame = g_build_slots[i].timer * frames / GetBuildTime(obj);
        if (frame >= frames) {
            frame = frames - 1;
        }
        return frame;
    }
}

// FUNCTION: LEGOLAND 0x0045ed30
void ObjectIsBuilt(Obj* obj, BuildTile tile)
{
    Pos pos;
    Pos centre;
    Rect r;
    Rect sv_rect;
    Pos sv_pos;
    int sv_c, sv_d;
    Obj* node;

    pos.x = tile.x;
    pos.y = tile.y;
    {
        SoundSource src;
        src.kind = 2;
        src.x = tile.x;
        src.y = tile.y;
        UnSourceAndFadeAllSamplesFromSource(&src, -200);
    }
    GetTileCentre(&pos, &centre);
    g_draw_state = 0;
    if (g_gfx_flags & 0x1000) {
        sv_rect = g_view.rect;
        sv_pos = g_view.pos;
        sv_c = g_view.c;
        sv_d = g_view.d;
    }
    obj->effect(obj->cls, &centre, 0x8f8);

    node = &g_obj_list;
    while (node != 0) {
        Rect* rp = &node->rects;
        do {
            int x, y;
            r = *rp;
            if ((node->state & 0x3000) == 0) {
                for (y = r.top; y <= r.bottom; y++) {
                    for (x = r.left; x <= r.right; x++) {
                        int mx = node->ox + x;
                        int my = node->oy + y;
                        Cell* cell;
                        if (mx < 0 || mx >= g_map->width ||
                            my < 0 || my >= g_map->height) {
                            cell = 0;
                        } else {
                            cell = &g_map_rows[my][mx];
                        }
                        if (cell != 0) {
                            cell->flags &= ~0x20;
                            cell->rf = 0;
                        }
                    }
                }
                rp = r.next;
            } else {
                /* the object is still being placed: skip its whole rect chain */
                rp = 0;
            }
        } while (rp != 0);
        node = node->next;
    }

    PutObjOnMap(obj, obj->cls, &pos);
    g_draw_state = 0;
    if (g_gfx_flags & 0x1000) {
        g_view.rect = sv_rect;
        g_view.pos = sv_pos;
        g_view.c = sv_c;
        g_view.d = sv_d;
    } else if (g_effect_obj != 0) {
        g_effect_obj->effect(g_effect_obj->cls, &g_gfx_point, 0x8f8);
    }
}
