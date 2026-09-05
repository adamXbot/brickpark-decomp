# Lane `fable-a` / `objrect` — object rectangles, path tiles and work orders

`LEGOLAND/objrect.c`, created 2026-09-05. **Six of six functions exact**
(`tools/audit.py LEGOLAND/objrect.c` → PASS, 630 instructions, `/W3` clean).
Every one ends in a real `ret`, none is recursive, so all six are promotable
and all six carry the plain `// FUNCTION:` marker.

| address | name | insns | pct | audit | marker |
| --- | --- | --- | --- | --- | --- |
| 0x0045e770 | `SetObjectDoorFlags` | 78 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0045e770` |
| 0x0045e850 | `ClearObjectUserFlags` | 80 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0045e850` |
| 0x0045e4a0 | `RefreshObjectAtPos` | 110 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0045e4a0` |
| 0x0045d5d0 | `SubtractObjRect` | 118 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0045d5d0` |
| 0x0045d3d0 | `RemoveObjectPathTiles` | 122 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0045d3d0` |
| 0x004777f0 | `GetRouteNode` | 122 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x004777f0` |

No function was renamed: every name comes from an existing caller's `extern`
(`popup.c`, `mappath.c`, `objmap2.c`, `workers.c`, `workorder.c`,
`workorder2.c`, `simcore.c`).

---

## Codegen levers (for `docs/DECOMP.md`)

- **AN APPEND INTO A GLOBAL ARRAY HAS THREE SPELLINGS AND THEY DIFFER IN TWO
  INDEPENDENT WAYS.** Filling `g_obj_rects[n]` from four scalars in
  `SubtractObjRect`:
  1. *Through a `Rect4* q = &g_obj_rects[n]`.* The address is materialised in
     a register (`mov ecx,eax / shl ecx,4 / add ecx,BASE`, the original's
     form) — **but the stores kill the CSEs of the address-taken `out`**, so
     `out.top` and `out.bottom` are reloaded from the frame in every one of
     the four blocks. 57/119.
  2. *Direct `g_obj_rects[n].field = v` subscripts.* The CSEs survive — VC6
     recognises the store as being to a known global object — **but the base
     folds into each store's `disp32`** (`mov [ecx+g_obj_rects], esi` with
     `ecx = n*16`), which is one instruction shorter per block than the
     original. 113 instructions against 118.
  3. *Fill a scalarised `Rect4 t` and assign `g_obj_rects[n] = t;`.* Both at
     once: the struct-assignment lowering materialises the destination address
     in a register, and every field VALUE is read before the first store. This
     is the original, and it took 117/118 first try.
  So "does this store kill my loads?" and "how is the address formed?" are
  separate questions with separate answers, and only the struct-assignment
  shape answers both the way the original does.
- **AN INLINE `SetRect4(Rect4* q, int l, int t, int r, int b)` IS NOT A
  SUBSTITUTE FOR THE STRUCT ASSIGNMENT.** It gets the address form right and
  evaluates the arguments before the stores, but the parameter `q` is still an
  opaque pointer, so block 1's stores still kill `out.top` for block 2. 111 of
  118. The helper only defers the kill by one block; the struct assignment
  removes it.
- **A LOOP-CONDITION READ OF A GLOBAL THAT THE BODY ALSO WRITES: MIRROR IT IN
  A LOCAL AND PUT THE RE-READ IN THE MISS ARM.** `SubtractObjRect`'s original
  parks `mov eax,[g_obj_rect_count] / jmp <latch>` past the epilogue and jumps
  to it from the "no intersection" test. Writing the condition as
  `i < g_obj_rect_count` puts that load in the SHARED latch instead (one
  instruction short, no exiled block) no matter how the loop is spelled —
  `for` with `continue`, rotated `do/while`, comma-increment condition. What
  reproduces it is `i < n` on a local mirror plus `else { n = g_obj_rect_count; }`:
  the arm then contains exactly the load, ends in a jump, and the block-exile
  rule parks it at the end. Writing the same assignment as
  `if (!hit) { n = g_obj_rect_count; continue; }` does NOT exile it — VC6
  inverts the test and lays the arm inline. **The arm has to be an `else`, not
  a guard with a `continue`.**
- **A FREE `volatile` READ CLOSED A 14-OF-118 *ALLOCATION* RESIDUAL — the
  class the triage calls "source is nearly powerless".** After the shape work
  above, `SubtractObjRect` measured strict 14 / register-blind **0** /
  offset-blind 14: the same instructions in the same order with `s.left` and
  `s.top` swapped between `esi` and `edx`. Nine source permutations (field
  order in the `t` fill, declaration order, compare operand order, `t = s`
  copies, four spellings of the `s = *p` copy) were all byte-identical at 14.
  Marking the miss arm's read volatile — `n = *(volatile int*)&g_obj_rect_count;`,
  a load the original performs anyway — closed all 14. **So a
  register-blind-zero residual is not automatically a floor: the free-volatile
  experiment is worth running even when the triage says allocation.** The
  mechanism is the documented one (the barrier stops the CSE with the hit
  arm's `n = g_obj_rect_count - 1` load) but the recorded outlook for
  `strict >> rb` should be softened from "nearly powerless" to "try the free
  volatile first".
- **AND THE PLACEMENT OF THAT VOLATILE READ IS THE WHOLE LEVER.** On the
  `- 1` load instead it closes 11 of 14 and leaves 3: the `s.bottom` spill
  store moves two indices EARLY, because VC6 will not hoist a volatile access
  across a store and so sinks the store below it. Same sharpening as
  `ClearCellForPath`'s 166-vs-167, now measured on a second body.
- **RUN THE LOOP ON PLAIN INTS AND WRITE THE `Pos` ONLY AT THE CALL SITES.**
  `RemoveObjectPathTiles` walks a rect and passes `&p` to two callees per
  cell. Written as `for (p.y = ...; ...; p.y++)` — which is exactly what
  `objmap2.c`'s `RemObjFromMap` tail does, and matches there — the
  address-taken pair is pinned in memory, the `x * 20` strength reduction is
  lost (the original hoists `lea edi,[ebp+ebp*4] / shl edi,2` and does
  `add edi,0x14` at the latch) and the body comes out 136 instructions against
  122. With `int x, y` counters and `p.x = x; p.y = y;` immediately before the
  calls it is 122/122 exact, first try. **The two shapes are not
  interchangeable and the sibling body is not evidence either way.**
- **TWO IDENTICAL ARMS MERGE AT THE FIRST SITE, SO THE LATER ONE MUST BE THE
  ARM THAT JUMPS.** `GetRouteNode` has three `axis = 5; cost = -1;` blocks;
  the original merges the first two (`je` back into the earlier block) and
  leaves the third alone. Writing the second as
  `if (!(def->flags & 0x200000)) { 5; -1; } else ...` makes it the
  fall-through, VC6 emits a second copy, and the function is three
  instructions long. Written positively — `if (def->flags & 0x200000) { ... }
  else { 5; -1; }` — it becomes the jump arm and merges. This is the
  merge-at-the-first-site rule applied to a statement pair rather than to
  `return K`.
- **INLINE-ARGUMENT ARITHMETIC IS EVALUATED RIGHT TO LEFT, WHICH DECIDES
  WHETHER A GUARD RE-TESTS.** `MapCellAt(def->dx + pos->x, def->dy + pos->y)`
  computes the Y sum LAST, so its own flags carry the helper's `x >= 0` guard
  and VC6 emits a bare `js`. The original re-tests with `test eax,eax / jl`,
  which is what a Y sum scheduled BETWEEN the X sum and the guard forces —
  i.e. the sums were STATEMENTS (`t.x = ...; t.y = ...; MapCellAt(t.x, t.y)`),
  not expressions in the arguments. Worth 52/76 → 78/78 on
  `SetObjectDoorFlags`. This narrows the recorded "arithmetic in a call
  argument runs before the callee's guard": it does, but it runs in argument
  order, and when the original re-tests, the arithmetic was NOT in the
  arguments.
- **A TWO-TERM SUM'S DESTINATION REGISTER FOLLOWS THE DESTINATION SYMBOL —
  confirmed on a Manhattan distance.** `h = abs(dx) + abs(dy)` evaluates the
  y term first (into `ecx`) and lands the sum in `ecx` in BOTH written orders;
  the original's `add eax,ecx` puts it in the x term's register. Only
  `dy = abs(...y...); h = abs(...x...); h += dy;` — where `h` IS the x term —
  reaches it.
- **A PAIR OF CONSTANT STORES CAN BE A SCHEDULING BOUNDARY PAIR.**
  `GetRouteNode`'s `n->parent = 0` and `n->dir = 0` are interleaved into the
  `cdq/xor/sub` abs sequence in the original. Written together, before or
  after the sum, they emit together and cost 5. Split either side of the
  `h += dy` they land on the original's indices exactly (0 of 122). Recorded
  as a reconstruction of the SCHEDULE, not a claim about the original source's
  statement spacing.
- **`if (def->type && def->type != 2)` RATHER THAN `!= 0` KEEPS A ZERO
  REGISTER OUT OF A PRE-PUSH GUARD.** In `RemoveObjectPathTiles` the `!= 0`
  spelling materialises `xor ebx,ebx` before the guard, turns `test ax,ax`
  into `cmp ax,bx` and `mov byte [..],0` into `mov byte [..],bl`. Existing
  rule, second measurement.

## Data structures and rules recovered

- **`Cell` +0x12 is not padding.** It is a 16-bit door-direction field: the
  low nibble is the ENTRANCE side (4/8/1/2 = below/above/right/left, in the
  order `GetObjEntranceDir` tests them) and the high nibble the EXIT side
  (0x80/0x40/0x20/0x10). Several objects' doors can share one tile. Written by
  `SetObjectDoorFlags`, cleared by `ClearObjectUserFlags`. `legoland.h`'s
  `Cell` should gain the field at integration (this file declares a local
  `MapCell` with it rather than editing the shared header).
- **`ObjDef` +0x24/+0x25 are the EXIT offset and they are SIGNED BYTES**,
  where the entrance offset at +0x0c/+0x10 is a pair of ints. Both
  `ObjHasExit` (0x0045e690) and `GetObjExitDir` (0x0045e710) read them with
  `movsx`. `ObjHasExit` is simply "the exit differs from the entrance".
- **`ObjHasEntrance` (0x0045e620) is three tests**: the class is non-null, its
  type at +0x20 is none of 0, 2 or 3, and the entrance offset falls OUTSIDE
  the class footprint rect at +0x3c (which it copies onto its own stack by
  `rep movsd`, five dwords — so the footprint `Rect` including its `next` link
  is passed around by value in this family).
- **`ObjDef` +0x2a is a signed short compared against 1** to pick the route
  cost of a "may be built over" object: `> 1` costs 20 and classes the tile 4,
  otherwise 9 and class 3. Nothing else matched reads it yet, so its meaning
  is open; it is declared `short w2a` here.
- **`RouteNode` +0x20 (`axis` in simcore.c) is written by `GetRouteNode` as a
  TERRAIN CLASS**, 0..5, and `RequestRoute` then treats class 1 as "stop the
  search here" and 0/1 as "no turn charge". The full model is in the file
  header §5. Note `f` is seeded as `cost + h`, i.e. the tile's own entry price
  plus the heuristic, not `g + h`.
- **The route heuristic is Manhattan** — `|dx| + |dy|` to `g_route_to`
  (0x004bb5a0 / 0x004bb5a4) — and `g` is seeded to `INT_MAX`.
- **`RefreshObjectAtPos` resets a cell's flags to ZERO**, unlike the placement
  stamp `AddObjectToMapByCursor` which merges `(old & 0x10) | flags`. It also
  writes `rf = 0` where the stamp writes 2, and sets neither 0x80 nor the
  0x8000 "tall" bit. So a work-order refresh deliberately drops every map bit
  the order may have painted on the object's tiles.
- **`SubtractObjRect`'s four remainder pieces** are, in append order: the
  full-width strip above, the left strip clipped to the intersection's rows,
  the right strip clipped the same way, and the full-width strip below. The
  scan steps back over the swap-erased entry and the loop condition re-reads
  the count, so the pieces it appends are themselves re-tested in the same
  call.

## Callees named for the first time

| address | name | what it is |
| --- | --- | --- |
| 0x0045d560 | `IntersectRect4(Rect4* out, Rect4* a, Rect4* b)` | four min/max branches into `out`, then returns non-zero when the result is non-empty. Not an export. |
| 0x00477730 | `FindClosedRouteNode(Pos*)` | linear scan of the closed list (0x00668fc4) for the node whose `Pos` at +0x08 is this tile. |
| 0x004777c0 | `FindOpenRouteNode(Pos*)` | the same scan over the open list (0x00668fc0). Byte-for-byte the twin of the above with one global changed. |
| 0x00482b60 | `TileJoinsPathNetwork(Pos*)` | `FindPathSquare(pos)`, and on a hit it FLUSHES the pending path-GFX batch (0x0066b46c, through 0x00482b20) and zeroes it before returning `(square->flags & 2) != 0`. Not a pure predicate — the flush is a side effect on the route search's path. |

Names reused rather than invented, from other files' externs:
`ObjHasEntrance` / `GetObjEntranceDir` / `ObjHasExit` / `GetObjExitDir`
(bigrender.c), `GetTileCentre`, `UpdatePathNeighbours`, `RemovePathSquare`,
`HeapAlloc_w`, `g_obj_rects` / `g_obj_rect_count` (workorder2.c),
`g_edit_cursor`, `g_route_to` (simcore.c).

## Original behaviour reproduced, not fixed

- **`RemoveObjectPathTiles` sweeps the interior twice.** Pass 1's rect (the
  footprint grown one cell on every side) strictly contains pass 2's, so every
  interior cell is cleared, re-tiled and passed to `UpdatePathNeighbours` /
  `RemovePathSquare` a second time. Commented at the site.
- **`RemoveObjectPathTiles` has no bounds check.** The cells are reached as
  `g_map_rows[y][x]` with no width/height test, so an object whose footprint
  touches the map edge indexes one row/column off the grid. Same three
  statements as `objmap2.c`'s `RemObjFromMap` tail, same exposure.
- **`GetRouteNode` dereferences a null cell.** The bounds-checked fetch
  returns 0 off-map and the next instruction reads `flags` at +0x0c through
  it. `mappath.c`'s `ClearCellForPath` and `simcore.c`'s `RequestRoute` carry
  the identical unguarded dereference of the same helper — three sites now, so
  this is the cluster's convention rather than a one-off slip.

## Extern prototype types that differ from another file's declaration

- `SetObjectDoorFlags` / `RefreshObjectAtPos` take `MapObj*` here where
  `popup.c`, `workers.c` and `workorder.c` declare `ObjElem*` / `void*`. Both
  are one pointer argument, so nothing changes on either side; the other files
  were NOT touched.
- `HeapAlloc_w` is declared `void* HeapAlloc_w(unsigned int)` as in
  `mappath.c`. `IntersectRect4` returns `int`.
- Nothing in this lane needed a type that contradicts another file's.

## Things worth checking at integration

- `legoland.h`'s `Cell.pad12[2]` can become a live `unsigned short` door
  field; this file declares its own `MapCell` only because the header is owned
  elsewhere. `loadmap.c:339` already zeroes `+0x12` explicitly through a cast,
  which is consistent.
- `docs/HANDOFF.md` §6B's triage table entry for `strict >> rb` ("source is
  nearly powerless") is contradicted by `SubtractObjRect`: rb was 0 and one
  free volatile read closed all 14. Suggest the table gain "run the
  free-volatile experiment before calling an allocation residual a floor".
