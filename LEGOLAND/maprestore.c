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
extern void RemovePathTile(Pos* pos, unsigned short tile);/* 0x0045daa0 */

/* Put the cell's GROUND tile back on display.
 *
 * WIP — 5 of the original's 13 instructions.  The body below is the function's
 * whole observable effect, but the original ends with three instructions we
 * have not been able to provoke out of VC6:
 *
 *      0..5   lea eax, [ecx+eax*4]              ; c = &g_map_rows[y][x]
 *      6      mov cx,  word ptr [eax+0xa]       ; base = c->base
 *      7,8    mov edx, ecx / and edx, 0xffff    ; base zero-extended to index
 *      9      mov dx,  word ptr [edx*8+0x801f44]; g_tile_recs[base].code
 *      10     mov word ptr [eax+8], cx          ; c->tile = base  <-- our last
 *      11     and dx,  0x20                     ; ...result then DISCARDED
 *      12     ret
 *
 * The `and` writes a register nothing reads, and no branch or store follows.
 * It is dead in the shipped binary, and it is the ONLY trailing dead 16-bit
 * `and` in the whole executable (a byte scan for `66 83 E? ?? C3` finds this
 * one site and nothing else), so it is not a recurring macro — it is one
 * unlucky function.
 *
 * WHAT IT ALMOST CERTAINLY IS.  `and dx,0x20` is 4 bytes; `test dx,0x20` is 5
 * (a 16-bit test takes a full imm16, F7 /0 iw).  VC6 swaps `test` for `and`
 * whenever the destination register is dead afterwards and the `and` encodes
 * shorter — which for a WORD register it does, and for a BYTE register it does
 * not (both 3 bytes, so byte comparands keep `test`).  So instruction 11 is a
 * *test* whose conditional jump landed on the very next instruction and was
 * peeled off by the final peephole.  The source is therefore an `if` on
 * `g_tile_recs[base].code & 0x20` whose arms generate nothing.
 *
 * WHAT WE COULD NOT DO: make VC6 keep BOTH the full-word load and the mask.
 * Everything below was compiled and inspected; each fails in one of two ways.
 *   (a) Dead value, everything vanishes — an empty `if`, `if (...) return;`,
 *       `while (...) break;`, `do {...} while (0)` + break, an empty `switch`,
 *       `goto` to the next line, an inlined empty function call, a body whose
 *       only statement CSEs away, assignment to an unused local / a parameter
 *       / a `register` local, and a bare `expr;` statement (which earns only
 *       warning C4552).  All collapse to the 9-instruction body below.
 *   (b) Live value, and then VC6 will give us the word load or the mask but
 *       never both, because a 0x20 mask lets it narrow the load to a byte:
 *         - degenerate branch `if (code & 0x20) c->tile = base; else c->tile =
 *           base;` merges the arms and DOES keep the condition — but as
 *           `mov bl,byte [..]` + `test bl,0x20` (byte register, so `test`
 *           wins), plus a push/pop ebx pair.
 *         - `(code & 0x20) == 0x20` with the same degenerate arms yields
 *           `mov dl,byte` / `and dl,0x20` / `cmp dl,0x20` — the mask survives
 *           because the compare needs its value, but still byte-wide.
 *         - keeping `code` live-out instead (degenerate arms that are no-ops)
 *           gives the full-word `mov dx,word [edx*8+..]` and drops the mask,
 *           replacing instruction 11 with a dead `mov [esp+4],edx` spill.
 *           That form is 13 instructions / 52 bytes — the original's exact
 *           length, with exactly one instruction different — but it is
 *           nonsense source (`if (code) base = base; else base = base;`), so
 *           it is not what we ship.
 *         - returning the pair as an 8-byte `struct { Cell* cell; unsigned
 *           short flag; }` (cell in EAX, flag in EDX) emits all 13 original
 *           instructions in order, then adds a `sub esp,8` frame and a
 *           `mov [esp+4],dx / mov edx,[esp+4]` round-trip to assemble the
 *           sub-dword member.  Widening that member to `unsigned int` removes
 *           the round-trip but re-narrows the load to `mov dl,byte` and the
 *           mask to `and edx,0x20`.  VC6 always round-trips a sub-dword
 *           member of a register-returned struct, so this is a dead end.
 *         - marking the table `volatile` keeps the full-word load (matching
 *           instructions 0..10) but still drops the mask.
 * The missing ingredient is whatever makes VC6 hold the tile code in a 16-bit
 * register while treating it as dead.  Ideas for the next attempt: a 16-bit
 * comparand that spans both bytes of the word in the ORIGINAL source (folded
 * to 0x20 only after narrowing decisions), or a `code` whose second use lives
 * in a block removed after register allocation. */
// WIP-FUNCTION: LEGOLAND 0x0045da60  (5/13 = 38.5%, trailing dead `and dx,0x20` unreproduced)
void RestoreBaseMap(int x, int y)
{
    Cell* c = &g_map_rows[y][x];
    unsigned short base = c->base;

    c->tile = base;
    /* The original then reads g_tile_recs[base].code, masks it with 0x20 and
     * throws the result away.  Omitted: VC6 removes it. See the note above. */
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
