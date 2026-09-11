# Scope PORT-M10 — the park's game-side defects: PARK-1, PARK-3, PARK-2

> **PORT-M10 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-M10)** — branch
> `scope/PORT-M10`, cut from the PORT-B10 merge (`7a665353`). Matching side:
> `LEGOLAND/*.c` under `#ifdef LEGOLAND_PORTABLE`, VC6-gated per file.
> `portable/**` and `docs/HANDOFF.md` are READ-ONLY for this lane (PORT-B11 is
> running in parallel on the shim). Brief: `docs/SCOPE_PORT_WAVE.md`.

**Result in one line: PARK-1 is a NEW blocker class, and it is fixed.** A
by-value struct parameter that one TU spells as a struct and the defining TU
spells as a scalar is the same dword on x86 and a **pointer** on wasm32 — and
because both lower to ONE `i32` parameter, **wasm-ld reports no signature
mismatch and nothing traps.** The callee silently reads the low half of a
shadow-stack address as its argument. This is the first blocker class in the
wave that every existing gate is blind to.

---

## 1. PARK-1 — the `LINK` goal never satisfies

### The symptom, restated from the game's own memory

PORT-B10 reported that `LINK "SPACE TOWER RIDE"` never completes however much
path is laid, so `g_num_visitors` stays 0. Replaying B10 §1's walk on
`legoland.html?args=-nointro+WINDEBUG&beat=1000` (port 8807) and reading the
game's globals through `ll_dbg_addr` / `llAddrs()`:

| what | where it was read | value | expected |
| --- | --- | --- | --- |
| the Space Tower's class record | `g_odf_head` chain, named by the `LLElem` that points at it | `type` 1 (linkable), `entrance` (+0x0c) = **(+2, -3)**, footprint (+0x3c) = (-2,-2,4,3) | — |
| its footprint cells | `g_map_rows`, 42 cells | base `bx,by` = **(63, 35)** on every one | (63, 35) |
| **its instance record** (`ObjDef +0x04`, `ObjInst +0x0e/+0x0f`) | the same chain | **(114, 10)** | (63, 35) |
| `LEGO SHOP 1`'s instance record (placed by the LEVEL LOADER) | ditto | **(74, 61)** — correct | (74, 61) |
| the `TowerRec` (`SpaceTower_AddRecord`'s record, `tile` at +0x00) | the heap block allocated immediately after the `ObjInst` | **0x0a72** = (114, 10) | 0x233f |
| the path squares (`0x0066b44c` chain, found by scanning for plausible rects) | 11 squares | the ride's pad `[60,32,68,32]`, `[60,33,60,39]`, `[68,33,68,39]`, `[61,39,67,39]` all carry **flag 2** (reachable from the entrance) | flag 2 |
| the live goal events (`ScriptEvent`, `kind` at +0x0c) | memory scan keyed on `elem` | kind 33 (`NEED`) + **kind 37 (`LINK`) for `SPACE TOWER RIDE`** | — |

So the path network is right, the flood fill is right, and the goal event is
the right one. `EventTick_Link` (eventtick.c:938, 0x0046a750) computes the link
square as `inst->x + d->dx`, `inst->y + d->dy` — verified against the
disassembly at `0x0046a77c`-`0x0046a78e` (`mov cl,[esi+0xe] / mov dl,[esi+0xf]`),
so the recovered C is exact. With the instance record reading (114, 10) the
link square is (116, 7), `CellAt` is off the 84x84 map, `outside++` fires and
the handler returns 0 **for ever**. `TileJoinsPathNetwork` is never even
reached.

**The renderer, the route search, the flood fill, `goalstate.c`, `mappath.c`,
`pathmisc.c`, `pathmisc2.c` and `pathsq.c` are all innocent.** The bad value is
written once, when the ride is BUILT.

### The root cause: a silent wasm32 by-value-struct ABI mismatch

`AddObjectToBuildList` (0x00450b90) records the map tile of an object that is
still under construction. Two TUs disagree about its second parameter:

* `LEGOLAND/popup.c:122` declares `extern int AddObjectToBuildList(ObjDef* d, BPos bp);`
  — `BPos` is `struct { unsigned char x, y; }`, **by value**. popup.c:30 already
  records that this is right: the parameter *is* the packed map tile.
* `LEGOLAND/sweep1.c:302` **defines** it as `int AddObjectToBuildList(int obj, short type)`
  and stores `g_buildlist[i].type = (unsigned short)type;`.

On x86 cdecl those are the same dword on the stack, which is why the byte gates
have always been green and why `relocs.py` and `audit.py` cannot see this.

On **wasm32 they are not the same**. clang passes a by-value struct directly
only when it is a *single-element* struct; a two-member 2-byte struct goes
**indirect** — a pointer to a byval temp on the shadow stack. Both spellings
lower to exactly one `i32` parameter, so:

* `wasm-ld` sees `(i32, i32) -> i32` on both sides and **warns about nothing**;
* `linkreport.py`'s `wasm_sig_conflict_detail` (PORT-A8 §2a) cannot see it
  either — there is no conflict to see;
* nothing traps. `sweep1.c` just stores the low 16 bits of a stack ADDRESS.

Measured with a three-file reproduction (`/tmp/port-m10-abi/`, emcc -O2 vs
native clang -O2, the real popup.c declaration against the real sweep1.c body):

```
native clang : slot0 type = 0x233f   (x=63 y=35)   correct
emcc wasm32  : slot0 type = 0x132c   (x=44 y=19)   a shadow-stack pointer
```

and in the live game the value is `0x0a72` → (114, 10), stable across runs and
across the `legoland.html` and `legoland_dbg.html` builds.

**The chain from there is entirely mechanical.** `BuildObject` (popup.c:173,
0x0045eb30) takes the `def->flags & 0x80000` arm for a ride — it goes on the
256-entry construction list rather than being placed at once:

```
BuildObject            bp = (63,35) from *pos          -> AddObjectToBuildList(def, bp)
                                                          slot.key  := 0x0a72   <-- CORRUPTED HERE
  AddObjectToMapByCursor(obj, pos, 0x20)               -> cells stamped (63,35)  (right: it uses *pos)
buildtick.c tick       ObjectIsBuilt(slot.obj, slot.key.tile)
  ObjectIsBuilt        pos.x = tile.x; pos.y = tile.y  -> (114,10)
    PutObjOnMap(obj, obj->cls, &pos)
      cls->place(obj, pos)  = SpaceTower_Add (mechrides.c:299 SpaceTower_Place)
        tile.b = (114,10)   -> SpaceTower_AddRecord    TowerRec.tile := 0x0a72
        AddBasicObject(obj, pos, 0)
          AddObjectToMap(obj, bp=(114,10), 0)          -> every cell off-map, SILENTLY skipped
          CreateObjectInstance(def, &key=(114,10))     -> ObjInst.key := 0x0a72
```

Two details that made this so quiet:

* the footprint cells are stamped by `AddObjectToMapByCursor` from `*pos`, which
  is still correct, so **the ride looks perfectly placed**; and
* `AddObjectToMap` guards every cell with `MapCellAt` and skips an off-map one
  without complaint, so the second, wrong stamp writes nothing at all.

### The fix

popup.c's declaration is the one that has to move, because sweep1.c's body is
the definition and 0x00450b90 has exactly one caller. Under
`#ifdef LEGOLAND_PORTABLE` the parameter is declared the way the definition
reads it and the tile is packed at the call site — the same shape the tree
already uses for `AddBasicObject`'s third argument:

```c
#ifndef LEGOLAND_PORTABLE
extern int   AddObjectToBuildList(ObjDef* d, BPos bp);          /* 0x00450b90 */
#else
extern int   AddObjectToBuildList(ObjDef* d, unsigned short bp); /* 0x00450b90 */
#define AddObjectToBuildList(_d, _bp) \
    AddObjectToBuildList((_d), (unsigned short)((_bp).x | ((_bp).y << 8)))
#endif
```

`BPos.x` is at +0x00 and `.y` at +0x01, so little-endian `x | (y << 8)` is the
same 16 bits `sweep1.c` stores and the same 16 bits `buildtick.c`'s `BuildKey`
union reads back as `{x, y}`.

### Proof

See §4's table: after the fix the instance record reads (63, 35), the LINK goal
satisfies, and visitors arrive.

---

## 2. A8's 11 latent signature sites, and M9-3

(in progress)

---

## 3. PARK-3 — the Space Tower draws as a squat block

(in progress)

---

## 4. PARK-2

(in progress)

---

## 5. Gates

(in progress)
