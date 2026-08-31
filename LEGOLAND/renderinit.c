/* LEGOLAND — terrain-object sprite binding + per-theme bridge draw offsets.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 *
 * Two map-loading helpers that LoadBaseMap (0x00461a50) calls directly:
 *
 *   BindTerrainObjectSprites (0x00462c60)  — the last thing LoadBaseMap does.
 *       Walks the terrain-object list built by build_perimeter (0x00462c00) and
 *       resolves each node's packed frame id into a live sprite-bank entry,
 *       kicking off the entry's animation when it has one.
 *
 *   SetBridgeDrawOffsets (0x004618d0)      — called from the map file's
 *       texture-set name record; picks the per-theme pixel offsets used when
 *       the terrain renderer (0x00460c60) draws the two halves of a bridge.
 *
 * Only struct field offsets and callee arg counts are load-bearing; type and
 * field names are ours.
 */

/* ---- globals ------------------------------------------------------------ */
/* The terrain/perimeter object list head (0x00667ca8); nodes are chained
 * through +0x1c. Same list the terrain renderer at 0x00460c60 walks. */
extern struct TerrainObj* g_terrain_objs;

/* The two sprite banks a terrain object can name. Both are LLIDB element data
 * blobs (LLIDB_LoadData results): the base terrain tile set (0x00667cac, set
 * from the map's terrain element) and the bridge/overlay tile set (0x00667cb0,
 * set from the "<THEME> BRIDGES" element named in the map file). */
extern struct SpriteBank* g_terrain_bank;   /* 0x00667cac */
extern struct SpriteBank* g_bridge_bank;    /* 0x00667cb0 */

/* Per-theme bridge draw offsets, consumed by the terrain renderer at
 * 0x00460c60. Each bridge object carries a half index in the low byte of its
 * frame id: half 1 uses g_bridge_half1_*, half 0 uses g_bridge_half0_*. The
 * "_ox/_oy" pair positions the bridge sprite itself; the "_shadow" pair is the
 * extra displacement applied to the companion sprite drawn after it. */
extern int g_bridge_half1_ox;       /* 0x004b9210 */
extern int g_bridge_half1_oy;       /* 0x004b9214 */
extern int g_bridge_half0_ox;       /* 0x004b9218 */
extern int g_bridge_half0_oy;       /* 0x004b921c */
extern int g_bridge_half1_shadow_ox;/* 0x00801a60 */
extern int g_bridge_half1_shadow_oy;/* 0x00801a64 */
extern int g_bridge_half0_shadow_ox;/* 0x00805f40 */
extern int g_bridge_half0_shadow_oy;/* 0x00805f44 */

/* ---- local types -------------------------------------------------------- */

/* An animation/LLS header: the 16-bit frame count lives at +0x10 (the same
 * field LLSPlay itself re-tests at 0x0047d530). */
typedef struct AnimHdr {
    char  pad0[0x10];   /* +0x00 */
    short frames;       /* +0x10 */
} AnimHdr;

/* A sprite definition inside a bank entry: its animation header at +0x00 and a
 * kind code at +0x14 (2 and 3 are the animated kinds). */
typedef struct SpriteDef {
    AnimHdr* anim;      /* +0x00 */
    char     pad4[0x10];/* +0x04 */
    int      kind;      /* +0x14 */
} SpriteDef;

/* One entry of a sprite bank; its definition hangs off +0x08. */
typedef struct BankEntry {
    char       pad0[8]; /* +0x00 */
    SpriteDef* def;     /* +0x08 */
} BankEntry;

/* A loaded LLIDB tile/sprite bank: count at +0x04, entry table at +0x08. */
typedef struct SpriteBank {
    int         pad0;    /* +0x00 */
    int         count;   /* +0x04 */
    BankEntry** entries; /* +0x08 */
} SpriteBank;

/* A terrain/perimeter object node. frame packs the bank frame index in the low
 * byte and, for bridge pieces, a nonzero bridge-group selector in the high
 * byte; sx/sy are world pixel coords used by the renderer. */
typedef struct TerrainObj {
    char              pad0[0x10];   /* +0x00 */
    int               frame;        /* +0x10 */
    int               sx;           /* +0x14 */
    int               sy;           /* +0x18 */
    struct TerrainObj* next;        /* +0x1c */
    BankEntry*        sprite;       /* +0x20 */
} TerrainObj;

/* ---- callees ------------------------------------------------------------ */
extern void LLSPlay(AnimHdr* anim, SpriteDef* owner);   /* 0x0047d520 */

/* Start every terrain/perimeter object's animation for the freshly loaded map.
 *
 * For each node in the terrain-object list, the packed frame id at +0x10 says
 * which bank to look in: a nonzero high byte means the object is a bridge piece
 * and comes out of the bridge bank (and if no bridge bank was loaded the object
 * simply gets no sprite), otherwise it comes out of the base terrain bank. The
 * resolved entry is cached at +0x20 so the renderer never has to look it up,
 * and if the entry's definition is one of the animated kinds with more than one
 * frame, its animation is registered with the LLS player.
 *
 * NB: loadmap.c already declares this callee as `RenderInit` — that is the
 * working name for the same address (0x00462c60). */
// FUNCTION: LEGOLAND 0x00462c60
void BindTerrainObjectSprites(void)
{
    TerrainObj* e;
    BankEntry*  s;
    SpriteDef*  d;

    for (e = g_terrain_objs; e != 0; e = e->next) {
        /* The volatile read is a codegen lever, not semantics: VC6 otherwise
         * common-subexpression-eliminates this load with the two `& 0xff`
         * reads below and keeps the value in eax across the branch, whereas the
         * original reloads [esi+0x10] inside each arm. Reading the selector
         * through a volatile lvalue (and only here) suppresses that one CSE and
         * reproduces the original's register allocation exactly. Qualifying the
         * member itself instead also forces the load to the top of each block,
         * which does NOT match. Do not "clean this up". */
        if ((*(volatile int*)&e->frame) & 0xff00) {
            if (g_bridge_bank != 0) {
                s = g_bridge_bank->entries[e->frame & 0xff];
            } else {
                e->sprite = 0;
                continue;
            }
        } else {
            s = g_terrain_bank->entries[e->frame & 0xff];
        }
        e->sprite = s;
        if (s != 0) {
            d = s->def;
            if (d->kind == 2 || d->kind == 3) {
                if (d->anim->frames > 1)
                    LLSPlay(d->anim, d);
            }
        }
    }
}

/* ---- CRT ---------------------------------------------------------------- */
extern int strcmp(const char* a, const char* b);
#pragma intrinsic(strcmp)

/* Select the per-theme bridge draw offsets for the map's bridge tile set.
 *
 * LoadBaseMap reads a texture-set name out of the map file (the same string it
 * then resolves into g_bridge_bank via ElemID/LLIDB_LoadData) and hands it
 * here. Each of the three shipped bridge sets — "CASTLE BRIDGES",
 * "EXPLORER BRIDGES", "WESTERN BRIDGES" — has its own pixel offsets for the two
 * halves of a bridge plus the displacement of the companion sprite drawn behind
 * each half; the terrain renderer at 0x00460c60 adds them to the object's world
 * position. An unrecognised name leaves the previous offsets in place.
 *
 * The three literals are compared with the inline strcmp that /O2 (/Oi) expands
 * — that expansion is what leaves edx holding 0, which is why the CASTLE arm's
 * eight zero stores are `mov [...], edx` rather than immediate stores.
 *
 * NB: loadmap.c declares this callee under the placeholder name
 * `map_helper_4618d0` for the same address (0x004618d0). */
// FUNCTION: LEGOLAND 0x004618d0
void SetBridgeDrawOffsets(const char* name)
{
    if (strcmp(name, "CASTLE BRIDGES") == 0) {
        g_bridge_half1_ox = 4;
        g_bridge_half1_oy = 0x6d;
        g_bridge_half0_ox = 0x3b;
        g_bridge_half0_oy = 0x6d;
        g_bridge_half1_shadow_ox = 0;
        g_bridge_half1_shadow_oy = 0;
        g_bridge_half0_shadow_ox = 0;
        g_bridge_half0_shadow_oy = 0;
        return;
    }
    if (strcmp(name, "EXPLORER BRIDGES") == 0) {
        g_bridge_half1_ox = 1;
        g_bridge_half1_oy = 0x6a;
        g_bridge_half0_ox = 0x40;
        g_bridge_half0_oy = 0x6a;
        g_bridge_half1_shadow_ox = 0xe;
        g_bridge_half1_shadow_oy = 0x89;
        g_bridge_half0_shadow_ox = 0x86;
        g_bridge_half0_shadow_oy = 0x89;
        return;
    }
    if (strcmp(name, "WESTERN BRIDGES") == 0) {
        g_bridge_half1_ox = 2;
        g_bridge_half1_oy = 0x5b;
        g_bridge_half0_ox = 0x3e;
        g_bridge_half0_oy = 0x5b;
        g_bridge_half1_shadow_ox = 0xe;
        g_bridge_half1_shadow_oy = 0x79;
        g_bridge_half0_shadow_ox = 0x85;
        g_bridge_half0_shadow_oy = 0x79;
    }
}
