# fable-c lane `workorder4.c` — point-to-point routing and path rects

New file: `LEGOLAND/workorder4.c` (seven functions, 447 instructions).
Objects `/tmp/fc_workorder4_*`.

| address | name | insns | bytes | audit | mismatch | promotable |
| --- | --- | --- | --- | --- | --- | --- |
| 0x0045d560 | `IntersectRect4` | 47/47 | 112/112 | **[OK]** | 0 | yes |
| 0x0045ca90 | `FindPathRect` | 51/51 | 136/136 | **[OK]** | 0 | yes |
| 0x0045cd70 | `RefreshPathArea` | 56/56 | 145/145 | **[OK]** | 0 | yes |
| 0x00482240 | `AddPTPOpenNode` | 58/58 | 184/184 | **[OK]** | 0 | yes |
| 0x00482620 | `PTPVisitTile` | 60/60 | 195/195 | **[OK]** | 0 | yes |
| 0x0049cf00 | `MarkWorkersOnMap` | 62/62 | 177/177 | **[OK]** | 0 | yes |
| 0x00482330 | `PTPShortcutSteps` | 113/113 | 241/241 | **[OK]** | 0 | yes |

7 of 7 exact. `audit.py LEGOLAND/workorder4.c` ends PASS; `/W3 /O2 /Gy /Gd` is
silent. Every body ends in a real `ret`; none is recursive or a tail-jump, so
all seven carry `// FUNCTION: LEGOLAND 0x<VA>` with no parenthetical.

---

## Mechanics recovered

### The 3x3 "plaza" path rect (`FindPathRect`, `RefreshPathArea`)

A run of path tiles gets a patterned paving texture wherever a complete **3x3
block** of walkable path exists. Three previously unread parallel tables at
0x004b9558 / 0x004b957c / 0x004b95a0 (nine `int`s each, laid out back to back)
drive it:

- **0x004b9558 — the nine 3x3 masks** inside a 5x5 bit map:
  `0x01ce7000, 0x00e73800, 0x00739c00, 0x000e7380, 0x000739c0, 0x00039ce0,
  0x0000739c, 0x000039ce, 0x00001ce7`. Each is `0x1ce7` (three rows of three
  adjacent bits, five apart) shifted by 1 per column and 5 per row.
- **0x004b957c / 0x004b95a0 — the matching corner offsets**, `dx` cycling
  `-2, -1, 0` and `dy` stepping `-2, -2, -2, -1, -1, -1, 0, 0, 0`.

**0x0045c9c0 named for the first time: `ScanPathArea5x5(Pos* origin)`.** It
walks the 5x5 block of cells from `origin` (x inner), starting a mask at
`0x01000000` and shifting it right one place per cell, and ORs the mask in
when the cell is walkable path — **map flags bit 4 set AND RF bit 1 clear**.
Off-map cells are scanned as `flags = 0x40, rf = 0`, so they can never
contribute. Bit 24 is the top-left cell, bit 0 the bottom-right.

`FindPathRect(pos, rect)` scans the 5x5 centred on `pos` (i.e. from
`pos - (2,2)`), takes the FIRST of the nine placements whose mask is fully
present, and reports it inclusively: `left = dx[i] + pos->x`,
`right = left + 2`, same for the y pair. Zero when no placement is complete.

`RefreshPathArea(pos)` is the repaint after the path under `pos` changed. It
sweeps the 5x5 neighbourhood **excluding the centre** (x outer, y inner) and,
for each cell that is currently showing a patterned tile but is no longer
covered by ANY complete 3x3, puts the plain tile back. Two more names:

- **0x0045ce30 `PathTileIsPatterned(Pos*)`** — on-map, the cell passes
  0x0045ce10, and the tile it DISPLAYS (`Cell +0x08`) differs from
  `*g_path_tile_base` (`*(int*)0x00832bf0`, read as a `short`).
- **0x0045cb90 `ResetPathTile(Pos*)`** — writes `*(short*)*g_path_tile_base`
  into `Cell +0x08`, with no bounds check at all.
- **0x0045ce10 `CellIsPath(Cell*)`** (for the record) — `rf & 1`, or
  `(flags & 0x10) && !(rf & 2)`.

The centre cell is deliberately left to the caller (`pathtile2.c` /
`pathsq.c`'s `RefreshPathSquare` do it).

### The point-to-point flood fill's visited map is 192 x 192 bits, not 256

`AddPTPOpenNode` and `PTPVisitTile` both index it as
`g_ptp_visited[(x >> 5) + y * 6]` — **SIX dwords per row, so 192 columns** —
and `0x0066a45c` (pathsq.c's `g_path_square_neighbours`) begins 0x1204 bytes
after 0x00669258, which is 192 rows of 24 bytes plus the four bytes of
alignment. bnvmove.c's note calls it "the 256x256-bit visited map"; it is
192 x 192. A map wider or taller than 192 cells would fold one row into the
next and run off the end of the array.

### `AddPTPOpenNode` vs `PTPVisitTile` — the same node push, two terrain rules

Both take `(int x, int y, PTPNode* parent)`, bounds-check against
`g_map->width/height`, consult the visited bitmap, `malloc(0x10)` and push
`{next, parent, x, y}` on the open list at 0x0066b450, then bump
`g_ptp_wave_count` and set the visited bit. They differ in exactly two places:

- **The terrain test.** `AddPTPOpenNode` (bnvmove.c's `PTPSuggestNextMove`,
  the bloke walker) rejects any cell with `rf & 2`. `PTPVisitTile`
  (workorder2.c's `FindPathLeg`, the build-site path finder) rejects it only
  when `!(flags & 0x0800)` — so a tile the RF layer calls impassable is still
  routable when map flag 0x0800 is set.
- **The allocation check.** `AddPTPOpenNode` tests the `malloc` result;
  **`PTPVisitTile` does not** and fills the node in unconditionally. Original
  bug, reproduced (`n = MemAlloc(0x10); n->next = ...` with no guard).

Neither sets a return value, although `bnvmove.c` and `workorder2.c` both
declare them `int` — see the extern note below.

### `PTPShortcutSteps` — corner cutting, corrected

`BuildPTPRoute` (workorder3.c) hands it the last four nodes of the walk-back:
`a` (the tile we stand on), then `b`, `c`, `d` outwards. The answer is how
many nodes one move may cover: 0 = step to `b`, 1 = to `c`, 2 = to `d`.

```
if (!b) return 0;
if (!c) return 0;
dx1 = b->x - a->x;  dy1 = b->y - a->y;        /* first step  */
dx2 = c->x - b->x;  dy2 = c->y - b->y;        /* second step */
if ((dx2 && dy1) || (dy2 && dx1))             /* the two steps TURN */
    return corner-cut tile a + (dx2, dy2) enterable ? 1 : 0;
if (!d) return 0;                             /* straight, and no fourth node */
dx3 = d->x - c->x;  dy3 = d->y - c->y;
if ((dx3 && dy1) || (dy3 && dx1))
    return corner-cut tile a + (dx3, dy3) enterable ? 2 : 0;
return 1;
```

Three things this corrects or adds to workorder3.c's account:

- It answers 0 when **`d`** is null too, not only `b` or `c` — a straight
  `a -> b -> c` with nothing beyond it is worth only one step.
- The "enterable" predicate is **`!(rf & 2) || (flags & 0x0800)`**, i.e.
  exactly `PTPVisitTile`'s. workorder3.c reads the second half as "blocked
  (Cell +0x1d bit 3)"; +0x0d is the high byte of the 16-bit `flags` at +0x0c,
  and the bit EXCUSES the tile rather than rejecting it.
- The third-step test compares `dx3`/`dy3` against the **FIRST** step's
  deltas (`dy1`/`dx1`), not the second's, and the tile it looks up is
  `a + (dx3, dy3)`. Reproduced as found; it is not obviously the geometry a
  reader would write.

**Neither cell fetch is bounds checked** — `g_map_rows[y][x]` straight off a
possibly off-map sum. It is safe only because every node on the chain came
from `PTPVisitTile`, which does bound its tiles, and the deltas are +-1.

### `MarkWorkersOnMap` — map flag 0x1000 is "a hired worker stands here"

Walks **the mechanic list (0x0079a8ac) FIRST, then the gardener list
(0x0079a8a8)**, and for each worker whose 24.8 world position at `Bloke +0x68`
maps on to the grid sets `Cell.flags |= 0x1000`. Nothing here ever clears the
bit; printlist.c's build-site check reads it immediately after calling this.

Note the bounds test shape: the `< 0` half is applied to the **raw** 24.8
value and the upper half to the shifted tile index
(`b->world.x >= 0 && (b->world.x >> 8) < g_map->width`), which is what keeps
the `test/jl` ahead of the `sar` in the original.

### `IntersectRect4`

Plain four-int rect intersection into `out`, non-zero when non-empty. Worth
recording only that the four edges are done **left, right, top, bottom** —
paired by min/max, not by axis — and that the emptiness test re-reads all four
back out of `out` rather than using the values it just computed.

---

## Levers, with evidence

- **A one-bit shift used twice must be a DUPLICATED EXPRESSION, not a named
  local — a named `bit` takes the FIRST callee-saved register and permutes all
  four.** `AddPTPOpenNode` and `PTPVisitTile` both compute
  `1 << (x & 0x1f)` for a visited-bitmap test and then for the set. With
  `unsigned int bit = 1 << (x & 0x1f);` every instruction, byte length and
  block is already right and **every one of the four callee-saved registers is
  wrong** (22 of 58 and 23 of 60): the named web outranks `x`, `y` and the
  word index, and VC6 hands it EBP/ESI where the original ranks it LAST.
  Written out twice, VC6 CSEs it into the low-ranked web the original has and
  both functions are exact first try. Measured inert on the named form:
  one-chain vs separate bounds guards, a named `Cell*`, `y*6 + (x>>5)` vs
  `(x>>5) + y*6`, all declaration orders, `int`/`unsigned` retypings of both
  locals, `register`, computing `bit` before `w` (worse: 27), and a free
  volatile read on `x`. **The word index `w` is the opposite** — it IS a named
  local; inlining it as well costs 28. So within one expression the two
  sub-expressions take opposite treatments, and the tell is the register
  RANKING, not the schedule.
- **A step-by-step delta order beats a "compute what the first test needs"
  order — worth 67 of 113 and an instruction.** `PTPShortcutSteps` computes
  four differences. Written `dx2, dx1, dy1, dy2` (the order the first
  condition consumes them, and the order VC6's own schedule appears to want)
  it spills `dx1` to the frame and keeps `c->x` in a callee-saved register;
  the original spills `c->x` and `c->y` and keeps all four deltas live.
  Written `dx1, dy1, dx2, dy2` — first step then second step — it is
  **113/113 exact**, and the operand order of the two sums that index the cell
  (`a->y + dy2` vs `dy2 + a->y`) is then completely inert. Where the earlier
  "declaration order sets the order of the leading `xor`s and nothing else"
  entry found initialiser order weak, COMPUTATION order of a group of
  same-shaped temporaries is strong: it decides which of them the frame gets.
- **A signed pointer end test (`jl`) on a strength-reduced table walk comes
  from a signed INDEX, not from a pointer cursor.** `FindPathRect`'s search
  compares the mask cursor against the address of the next table
  (`cmp ecx, 0x4b957c / jl`). A hand-written `for (m = tbl; m < tbl + 9; m++)`
  cursor with a parallel `i++` is byte-for-byte identical **except** that the
  pointer comparison is unsigned and emits `jb` — the single mismatch of 51.
  `for (i = 0; i < 9; i++)` with all three tables subscripted is exact: VC6
  strength-reduces the mask subscript into the pointer and the derived test
  inherits the `int`'s signedness, while the other two tables keep the index.
  This is the ELIMINATION half of the "two lockstep cursors" lever seen from
  the signedness side.
- **A bounds guard whose `< 0` half is on the RAW value and whose `< limit`
  half is on the SHIFTED value has to be written that way — VC6 will not sink
  the shift.** `MarkWorkersOnMap`'s original tests `test eax,eax / jl` on the
  24.8 coordinate and only then `sar eax,8` for the width compare. Passing
  `(w->x >> 8, w->y >> 8)` into an inlined `MapCellAt`-style helper forces
  BOTH shifts before the first test and turns the sign test into `js`
  (24 of 62). Spelling the guard out inline —
  `if (b->world.x >= 0 && (b->world.x >> 8) < g_map->width && ...)` — is
  62/62 first try. Corollary: **an inline bounds helper is not free when its
  arguments are expressions**; the argument evaluation is a scheduling
  barrier the original does not have.
- **A dead `lea` of an array element's address is not evidence of a named
  pointer.** `MarkWorkersOnMap` emits
  `or byte ptr [ecx+eax*4+0xd],0x10` and then a plainly dead
  `lea eax,[ecx+eax*4+0xc]`. Naming the pointer (`f = &cell.flags; *f |= ...`)
  and not naming it are byte-identical — the `lea` is VC6 materialising the
  subscript's address either way. Do not spend variants on it.
- **`if (out->left <= out->right && out->top <= out->bottom) return 1;
  return 0;`** is the shape behind two `jg` to ONE trailing `xor eax,eax`
  with `mov eax,1` inline before it (`IntersectRect4`, exact first try) —
  a fourth confirmation of the recorded "success inline, failures to one
  trailing `return 0`" rule.
- **A `continue`-per-guard inner loop is what shares the increment block.**
  `RefreshPathArea`'s body is three independent `if (...) continue;` guards
  and one call; that is 56/56 first try, with the reload of the y counter
  falling exactly where the original has it.

---

## Original bugs reproduced

- **`PTPVisitTile` does not check its allocation.** `malloc(0x10)` is followed
  straight by `mov [eax],edx` — a failed allocation writes through null. Its
  twin `AddPTPOpenNode`, twenty instructions earlier in the same source,
  does test it. Commented at the site, not fixed.
- **`PTPShortcutSteps` reads the map with no bounds check**, twice, from a sum
  of a node coordinate and a delta.
- **`ResetPathTile` (0x0045cb90) has no bounds check either** — noted at its
  extern, since `RefreshPathArea` can only reach it through
  `PathTileIsPatterned`, which does check.
- **`RefreshPathArea` skips only the exact centre cell**, so the eight
  cells immediately around a changed tile are re-tested even though
  `FindPathRect` has already been asked about overlapping blocks — the
  redundancy is in the original.

---

## Extern-type divergences and naming

- **`AddPTPOpenNode` and `PTPVisitTile` are defined `void` here** where
  `bnvmove.c` and `workorder2.c` declare both `int`. Neither definition sets a
  return value; an `int` definition would need a fabricated `return` and would
  not compile clean at /W3. The callers' `int` externs are left alone — under
  `__cdecl` they are ABI-identical, and the declared type is a caller-side
  lever there.
- **`MarkWorkersOnMap` is defined `void MarkWorkersOnMap(void)`** where
  `printlist.c` declares `void MarkWorkersOnMap(CellRect* r)` and passes the
  rect it is about to scan. The function reads no argument at all;
  printlist.c's comment already says "(ignores r)". Left alone.
- **`MemAlloc` (0x0049e4ff)** keeps bnvmove.c / workorder2.c's name here
  because this is their code path; `objrect.c`, `pathsq.c` and `printlist.c`
  call the same address `HeapAlloc_w`. Two names for one CRT address already
  existed in the tree; no new name was minted.
- `Rect4` is objrect.c's / workorder2.c's name for the flat 16-byte rect;
  `pathsq.c` calls the identical shape `PathRect` and declares
  `FindPathRect(Pos*, PathRect*)`. Offsets agree, names do not; nothing
  edited.
- `g_path_3x3_masks` is declared `const unsigned int[9]` and the two offset
  tables `const int[9]`. The masks must be unsigned or the `==` against the
  `unsigned int` scan result warns at /W3; the choice is codegen-inert.
- New names, no other file declares these addresses: **0x0045c9c0
  `ScanPathArea5x5`**, **0x0045ce30 `PathTileIsPatterned`**, **0x0045cb90
  `ResetPathTile`**, **0x0045ce10 `CellIsPath`** (described, not declared).
  0x00482330 `PTPShortcutSteps` keeps workorder3.c's name.
- `Bloke` is declared here with only `next` and `world` (+0x00, +0x68) of
  workers2.c's 0xb0-byte record; `Cell` and `Pos` come from `legoland.h`
  unchanged.
