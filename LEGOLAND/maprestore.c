/* LEGOLAND — map / path TEARDOWN and base-map restore.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; names are ours.
 *
 * ---------------------------------------------------------------------------
 * WHAT "RESTORING" A BASE MAP MEANS  (vs. LOADING one)
 *
 * LoadBaseMap (0x00461a50, LEGOLAND/loadmap.c) is a 1214-instruction resource
 * reader: it opens the .MAP, decodes four RLE layer streams and fills, for
 * every cell, both the DISPLAYED tile (Cell +0x08) and the GROUND tile
 * (Cell +0x0a), then builds the perimeter objects and the render list.
 *
 * RestoreBaseMap is NOT the inverse of that.  Everything built after load — a
 * path square, an object footprint — only ever overwrites Cell.tile; Cell.base
 * is written once by the loader and never touched again.  So the ground under
 * a built thing is not saved anywhere special: the cell's own +0x0a IS the
 * save slot, and "restoring the base map" for one cell is the one-word copy
 * base -> tile.  It is a per-CELL operation, and it is what every demolition
 * path calls to make the ground reappear:
 *     StandardRemoveObject 0x0045f220 (once per footprint cell),
 *     RemovePathTile       0x0045daa0 (below),
 *     the ride/attraction loaders (Copters_Load, LoadJailCells, ...).
 *
 * ClearOverlays (0x00462ce0) is the other half of the teardown, and it is the
 * counterpart of BuildPerimeterObject (0x00462c00, LEGOLAND/pathgfx.c): it
 * frees the whole perimeter/cliff terrain-object list (head @ 0x00667ca8) and
 * nulls the head.  LoadBaseMap calls it at 0x00462b11 before rebuilding that
 * list, ProcessDamage (0x00463580) and UnloadSaveGameMap (0x0047f760) call it
 * on the way out — so it serves as both "start a new map" and "drop this one".
 *
 * THE TWO PATH REMOVERS
 * Both are per-object-CLASS "remove" handlers stored in the class descriptor
 * at +0x9c (installed at 0x00452c47 alongside the add handler at +0x98), and
 * both are reached only through RemObjFromMap (0x00459c90), which calls
 *     (*cls->remove)(obj, key, mapobj)
 * at 0x00459d1d — hence the shared 3-argument shape, and hence the unused
 * middle argument here (it is RemObjFromMap's packed 2-byte cell coordinate,
 * passed BY VALUE: the caller reads it as two separate bytes at [esp+0x2c] and
 * [esp+0x2d], which only happens for a 2-byte struct in one stack slot).
 *   - RemoveBasicPath refunds the object's salvage value in bricks, then tears
 *     the path tile down at the object's OWN footprint origin (Obj +0x1404) —
 *     it ignores the coordinate it was handed.
 *   - RemoveRollerCoasterPath skips the refund entirely (coaster track is not
 *     salvageable) and takes the cell straight from its Pos* argument.
 * Their add-side counterparts are AddBasicPath (0x0045dbe0, class slot +0x98)
 * and AddPathTile (0x0045d3b0).  Both removers write the same tile code, read
 * as a 16-bit word through the pointer at 0x00832bf0 (the loaded "path" tile
 * record) — AddBasicPath and LoadBaseMap read it exactly the same way.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

/* ---- the parallel tile-info table (0x00801f40, stride 8) ----------------
 * legoland.h models the second field as a 32-bit `code`, but AllocTileSpace
 * (0x0045aa2c / 0x0045aa31) writes it with a WORD store and RestoreBaseMap
 * reads it back with a WORD load, so the field is 16 bits wide with two bytes
 * of padding behind it.  Declared locally under our own name so legoland.h is
 * left alone. */
typedef struct TileRec {
    void*          elem;   /* +0x00 the tile's database element */
    unsigned short code;   /* +0x04 tile code, 16-bit (bit 0x0020 tested below) */
    unsigned short pad6;   /* +0x06 */
} TileRec;
extern TileRec g_tile_recs[];          /* 0x00801f40 */

/* Head of the perimeter/cliff terrain render-object list built by
 * BuildPerimeterObject; the link is at +0x1c (see pathgfx.c's TerrainObj). */
typedef struct Overlay {
    char            pad0[0x1c];  /* +0x00 */
    struct Overlay* next;        /* +0x1c */
} Overlay;
extern Overlay* g_terrain_objs;        /* 0x00667ca8 */

/* The loaded path-tile record; its first word is the tile code to paint. */
extern void* g_path_tile_ptr;          /* 0x00832bf0 */

/* The object wrapper handed to a class remove handler: its descriptor (the
 * GetObjSalvageValue subject — cost at +0x26, life divisor at +0x2c) hangs off
 * +0x0c.  Same shape as power.c's PowerObj. */
typedef struct SalvObj {
    char  pad0[0x0c];   /* +0x00 */
    void* desc;         /* +0x0c */
} SalvObj;

/* RemObjFromMap's packed 2-byte map coordinate, passed BY VALUE (same shape as
 * power.c's BPos).  RemoveBasicPath ignores it. */
typedef struct BPos {
    unsigned char x;    /* +0x00 */
    unsigned char y;    /* +0x01 */
} BPos;

/* The placed map object (see buildtick.c's Obj); +0x1404/+0x1408 is the map
 * origin of its footprint, i.e. a Pos. */
typedef struct MapObj {
    char pad0[0x1404];  /* +0x0000 */
    Pos  origin;        /* +0x1404 */
} MapObj;

/* ---- callees ----------------------------------------------------------- */
extern void HeapFree_w(void* p);                          /* 0x0049e4d0 */
extern int  GetObjSalvageValue(void* desc, int elapsed);  /* 0x00480db0 */
extern void AddBricks(int amount);                        /* 0x004578a0 */
/* Declared with `unsigned short tile` HERE on purpose: the callers below load
 * the tile word 16-bit and this prototype is what produces that codegen. The
 * definition in pathtile2.c takes `int` (dword param load; the value is never
 * read). ABI-identical — evidently the original header and TU disagreed, as
 * with LoadSpriteIcon (saveprof.c vs iconui.c). */
extern void RemovePathTile(Pos* pos, unsigned short tile);/* 0x0045daa0 */

/* Put the cell's GROUND tile back on display: the one-word copy base -> tile.
 *
 * The original ends with a DEAD `and dx, 0x20` (0x0045da8f): it reads
 * g_tile_recs[base].code, masks the reserved bit, and never stores or
 * branches on the result.  That is the ghost of a degenerate branch -- the
 * same artefact LoadBaseMap carries at 0x00462333 (a dead `test byte ptr
 * [..], 0x20` with no jump behind it).  VC6 merges identical if/else arms
 * late, in the backend, and the jump-to-next-instruction that leaves is
 * peeled off after instruction selection; the flag-setting instruction is
 * never revisited.  So the source is a branch on the reserved flag whose
 * arms all do the same thing.  The arms below are deliberately identical;
 * do not "simplify" them.
 *
 * Why it is a 16-bit `and` on a full-word load, and not LoadBaseMap's
 * `test byte ptr` (every lever below was measured; see scratchpad/rbm):
 *   - `if (code & 0x20) X; else X;` alone is a pure flag test: VC6 narrows
 *     the load to a byte and emits `test bl, 0x20` (+ push/pop ebx) or
 *     `test byte ptr [..], 0x20` straight from memory.
 *   - A u16 register temp `f = code & 0x20` is int-typed: the mask becomes
 *     `and edx, 0x20` (VC6 treats and-immediate as a free zero-extension
 *     and widens every single-use one) and the load is byte-narrowed again.
 *   - The `and` stays 16-bit only when the masked value has MORE THAN ONE
 *     consumer that needs it as a 16-bit VALUE.  Here that is the equality
 *     compare `reserved == TILE_RESERVED` -- a compare against the flag
 *     value, not against zero (`else if (reserved)` / `!= 0` collapse back
 *     to the byte test) -- plus the first arm's test, which the optimiser
 *     CSEs with it.  Two consumers block the widening, and a value compare
 *     cannot be byte-narrowed, so the word load survives.
 *   - VC6 knows `x & 0x20` is 0 or 0x20, so once the first arm has tested
 *     it against zero the second compare folds away; the first test is
 *     fused into the `and` (same width), the three arms merge, both jumps
 *     go, and only the value-producing `and dx, 0x20` is left.
 *   - The register allocation (c in EAX, base in CX, index/code in EDX) is
 *     the other half of the lever: deriving `reserved` from a `code` local
 *     instead of a second `g_tile_recs[base].code` read, or swapping the
 *     first two arms, makes VC6 fold the base load into the address
 *     (`mov dx, [ecx+eax*4+0xa]`) and lands the body in the EAX/ECX swap:
 *     13 instructions, 52 bytes, 7 mismatches.  Reading the table word
 *     twice (CSE'd into one load) keeps the `lea` first.
 * A u16 memory destination (`c->flags = code & 0x20; if (c->flags) ...`)
 * also yields the 16-bit `and` but leaves its store behind; a `volatile`
 * read keeps the word load but drops the mask.  Neither is needed. */
#define TILE_RESERVED 0x20   /* tile code bit: build-footprint reserved */

// FUNCTION: LEGOLAND 0x0045da60
void RestoreBaseMap(int x, int y)
{
    Cell* c = &g_map_rows[y][x];
    unsigned short base = c->base;
    unsigned short reserved = g_tile_recs[base].code & TILE_RESERVED;

    /* All three arms are the same on purpose (see above). */
    if ((g_tile_recs[base].code & TILE_RESERVED) == 0)
        c->tile = base;
    else if (reserved == TILE_RESERVED)
        c->tile = base;
    else
        c->tile = base;
}

/* Free the whole perimeter/cliff terrain-object list and null the head.
 *
 * The empty-list case really is a separate `else`, not a fall-through: the
 * original stores ESI (the `next` link, which the do-while leaves at 0) on the
 * loop path and an immediate 0 on the guard path, and VC6 only splits them
 * that way when the two stores come from two different statements.  The two
 * single-statement forms both miss:
 *   - `while (p) {...} g_terrain_objs = p;` stores the LOOP pointer's register
 *     (EAX) on both paths;
 *   - hoisting `next = 0` above the loop makes VC6 materialise the constant
 *     into ECX up front (`xor ecx,ecx`) and store ECX on the guard path.
 * `push esi` is sunk into the non-empty path — the deferred-prologue pattern:
 * `next` lives only in the post-guard inner scope. */
// FUNCTION: LEGOLAND 0x00462ce0
void ClearOverlays(void)
{
    Overlay* p = g_terrain_objs;

    if (p != 0) {
        Overlay* next;
        do {
            next = p->next;
            HeapFree_w(p);
            p = next;
        } while (p != 0);
        g_terrain_objs = next;
    } else {
        g_terrain_objs = 0;
    }
}

/* Class remove handler for the plain path classes: refund the salvage value,
 * then strip the path tile at the object's footprint origin.
 *
 * `desc` must be a named local. Written inline as
 * `GetObjSalvageValue(obj->desc, 0)`, VC6 loads the field into ECX instead of
 * reusing EAX, and every register in the second statement shifts one along
 * (the Pos pointer lands in ECX, whose `add ecx,0x1404` is a byte longer than
 * EAX's). The named local restores the original's allocation exactly. */
// FUNCTION: LEGOLAND 0x0045dc90
void RemoveBasicPath(SalvObj* obj, BPos key, MapObj* mapobj)
{
    void* desc = obj->desc;

    AddBricks(GetObjSalvageValue(desc, 0));
    RemovePathTile(&mapobj->origin, *(unsigned short*)g_path_tile_ptr);
}

/* Class remove handler for roller-coaster track: no salvage refund. */
// FUNCTION: LEGOLAND 0x0045dcd0
void RemoveRollerCoasterPath(Pos* pos)
{
    RemovePathTile(pos, *(unsigned short*)g_path_tile_ptr);
}
