# Lane `fable-a-bswater` — boating-school water and launch (2026-09-05)

File: `LEGOLAND/bswater.c` (new). Four functions from the unmatched-callee
frontier. `tools/audit.py LEGOLAND/bswater.c` ends **PASS**; `/W3` clean.

| address | name | insns | bytes | audit | mismatch | marker |
| --- | --- | --- | --- | --- | --- | --- |
| 0x00418e60 | `BoatingSchool_TryLaunch` | 95/95 | 299/299 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x00418e60` |
| 0x0042c6d0 | `Carousel_TickInstance` | 106/106 | 296/296 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x0042c6d0` |
| 0x0041cb20 | `BsWater_WalkStep` | 108/108 | 291/291 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x0041cb20` |
| 0x0041c4c0 | `BsWater_SetTile` | 109/109 | 350/350 | [WIP] | 3 | `// WIP-FUNCTION: LEGOLAND 0x0041c4c0  (109/109 insns, 350/350 B, mismatch=3, idx 93: \`inc ebp\` scheduled 2nd not 4th in the inner latch)` |

No renames. All four end in a real `ret` and none is recursive, so the three
`[OK]` bodies are promotable as committed; the fourth stays WIP on mismatch,
not on extent.

---

## Data structures and rules recovered

### The boating-school boat record (`BsBoat`, 0x3f4 bytes, head 0x004cc03c)

`anim2.c` had the layout from the DRAWING side; `BoatingSchool_TryLaunch` is
the side that BUILDS one, so the initial values are now known:

```
+0x00 u16  key      the school's own map square (the station's +0x00)
+0x04 int  cx       the map square the boat is on   = key.x - 1
+0x08 int  cy                                       = key.y + 5
+0x0c int  nx       the square it is heading for     = key.x - 1
+0x10 int  ny                                       = key.y + 5
+0x1c      wob[80]  {int x, int y} rocking offsets  -- memset to the BYTE 0xf1
+0x29c     frame[80] sprite codes                   -- memset to 0
+0x3dc int          = 1
+0x3e0 int          = rand() & 3, then 3 rewritten to 2   (see the bug below)
+0x3e4 int state    = 1  (state 1 = "just launched", tested by the guard below)
+0x3e8 int leg      = (rand() & 0xf) + 4
+0x3ec Bloke* rider = the visitor
+0x3f0      next
```

The launch guard is three refusals, all against the station's NEAR jetty
(`st->ax`/`st->ay`, the route start): a boat of this school sitting on the
jetty square, a boat heading for it, or any boat of this school still in
state 1. `+0x14/+0x18` (the boat's screen position) are left as heap garbage
until the drawer computes them.

### `BsWater +0x04` is the ARM MASK (new)

`ridecb5.c`/`ridecb6.c` both had `+0x04` as padding in `BsWater`.
`BsWater_SetTile` writes the four-bit arm mask there as an `int`
(`mov dword ptr [eax+4], ecx`) — the same mask `BsWater_Probe` (joust2.c
0x0041c690) returns. So the lake caches its own shape and the drawer does not
have to re-probe. Full record now:

```
+0x00 u16  pos     this cell's own map square
+0x02 u16  owner   the school that owns it
+0x04 int  mask    the N/E/S/W arm mask (bit 1/2/4/8)   <-- NEW
+0x08 int  dist    steps from the route seed
+0x10      next    the lake-wide list link (head 0x004d823c)
+0x14      queue   the route walk's frontier link
+0x18      prev    reached-from pointer AND visited mark
```

### The route walk (`BsWater_WalkStep`)

A breadth-first walk on the lake's five-cell lattice, state in two globals:
`g_bs_walk_cur` (0x004d8240) is the frontier built by the previous ply,
chained through `+0x14`; `g_bs_walk_next` (0x004d8244) is the one this call
builds. A neighbour five squares away is taken iff it exists, its `owner`
equals the walk's key, and its `+0x18` is still 0. Taking it sets
`+0x18 = current` (so the mark and the back-pointer are the SAME field, which
is why `BoatingSchool_RebuildRoute` can clear the whole lake with one pass and
then read the route back by following it) and `+0x08 = current->dist + 1`.
The frontier is pushed at the head, so it is a stack, not a queue: within one
ply the cells come out in reverse order of discovery. Neighbour order is
N (y-5), E (x+5), S (y+5), W (x-5) — the same order and the same +/-5 lattice
as `BsWater_Probe`, but with NO bounds check, because `BsWater_FindAt` is the
plain list search.

### The carousel machine (`Carousel_TickInstance`)

Three states in the record's flag word `+0x0c`:

* **idle** (neither bit) — once anybody has boarded (`+0x06 != 0`), the
  record's timer `+0x1c` counts down one per tick; at zero the ride takes the
  "not letting anyone on" flag on its own map square and latches 0x4000.
* **boarding** (0x4000) — turns as soon as `boarded` (+0x06) equals
  `visitors` (+0x18); `Carousel_StartRide` (0x0042bc90) does the transition.
* **running** (0x0001) — one platform frame every SECOND tick (`+0x14` is the
  divide-by-two counter), the frame wraps at 0x40 and each wrap spends one of
  the revolutions in `+0x10`. At zero revolutions the handler waits for
  `GetAllBlokesOffRide` and then calls `Carousel_StopRide` (0x0042c210).

Field types corrected against `ridecb3.c`: `+0x10` is a BYTE (revolutions
left), not an int — `mov al, byte ptr [esi+10h]` reuses the low half of the
register that still holds the incremented `+0x14`.

Then, unconditionally except on the "waiting for riders to get off" early
return, the CLASS-wide rider list (ObjDef +0xcc) is walked — every carousel
walks every carousel's riders each tick and picks out its own by map square —
and any rider whose pose set (+0x35) is 1 is re-placed by the .bnv solve at
the platform's current frame. Finally the z-sprite's frame word is set from
the same frame byte (`**g_carousel_zspr->lls_holder = rec->frame`).

`Carousel_StopRide` (0x0042c210, named here for the first time) re-rolls the
revolution count to `rand() % 2 + 3`, clears frame/visitors/boarded, clears
flag bits 1 and 0x4000 together (`and ebx,0FFFFBFFEh`) and fades the ride's
sample out. That is what fixes `+0x10`'s meaning.

### `BsWater_SetTile`

Repaints the 5x5 block of MAP CELLS centred on one lake square — the lattice
is five cells apart precisely because each lake square owns a 5x5 patch — and
creates the lake record if it does not exist. Per cell it sets
`flags = 8`, `rf = 2`, `obj = g_bs_water_cls->c4`, `key = the lake square`,
and calls `SetMapTile` with the tile from the shape table.

---

## Callees named for the first time

* `Carousel_StopRide` — **0x0042c210**. See above. `ridecb3.c` calls it
  through the tick and did not have a name for it.
* `g_bs_water_shapes` — **0x004b53d4**, `unsigned char [16][25]`: 25 tile
  codes (the 5x5 block, row-major) per arm mask.
* `g_bs_water_tsm` — **0x0082adf4** re-typed as `TsmRec*` (roads.c's
  `{void* elem; unsigned short* loaded;}`); `ridecb5.c`/`ridecb6.c` call the
  same address `g_bs_water_tiles` with a `BsTileSet` shape. Same object, two
  spellings; nothing changed in either file.
* `g_carousel_bnv` — **0x0061608c**, the carousel's .bnv bundle (the one
  `SetBlokePositionFromBNV` is handed). `ridecb3.c` names 0x00616090 and
  0x00616098 as CarouselOn/CarouselOff; this is a third, four bytes below.
* The two carousel .bnv depth constants are **-1617853.25f** and
  **-1618109.0f** (0xc9c57dea / 0xc9c585e8) — a new pair; joust uses
  -1617735.0/-1617993.5 and balloonz -1617692.375/-1617904.25.

## Original bugs reproduced (commented at the site)

1. **`BoatingSchool_TryLaunch` dereferences an unfound station.** The station
   search's result is never null-checked; `st->ax` is read inside the boat
   loop. It cannot fire in practice because a boat's key always comes from a
   live station, but the guard is genuinely absent.
2. **`BoatingSchool_TryLaunch` launches at a fixed offset, not at the jetty.**
   The three refusal tests compare against `st->ax`/`st->ay`, then the new
   boat is placed at `(key.x - 1, key.y + 5)` — a hard-coded offset from the
   BUILDING's square. The two agree for the class's standard footprint, so it
   is latent.
3. **`rand() & 3` biased to 2.** `+0x3e0` is rolled 0..3 and then, as a
   separate statement AFTER the two buffer fills, 3 is rewritten to 2. The
   four-way choice is really three-way with 2 twice as likely.
4. **`BsWater_SetTile` does not null-check the map cell.** The standard
   bounds-checked fetch is compiled in full (x >= 0, x < width, y >= 0,
   y < height) and its zero result is stored into two instructions later, so a
   lake square within two cells of the map border faults.
5. **`BsWater_SetTile`'s `t >> 8` is always zero.** The tile lookup
   `*g_bs_water_tsm[t >> 8].loaded + (t & 0xff)` is copied verbatim from the
   road layer (roads.c 0x00412a20) where `t` is an `unsigned short` tile word.
   Here the shape table holds BYTES, so only TSM record 0 is ever consulted.
   VC6 still emits the shift because it does not range-narrow a promoted byte.
6. **A newly created lake record is only partly initialised**: `+0x18` is
   cleared and `+0x10` linked, but `+0x08` and `+0x14` are left as heap
   garbage; only a cell the route walk reaches ever gets them written.

---

## Levers, with evidence

* **A packed `{u8 x; u8 y}` parameter passed by value is read back out of the
  argument slot with an UNALIGNED dword load plus a mask.** In
  `BoatingSchool_TryLaunch` the whole slot is already in `edi` (it is compared
  16 bits at a time as `di`), so `key.b.x` is `and edi,0FFh` — but `key.b.y`
  is `mov ecx, dword ptr [esp+15h] / and ecx,0FFh`, a dword read one byte into
  the slot. Do not try to reproduce that with a shift or a byte load: writing
  the union member (`key.b.y`) is what emits it. Confirms and sharpens the
  recorded "packed {u8,u8} by value is forwarded as a dword and masked".
* **The order of `p->next = head;` and a neighbouring field store decides
  where the head LOAD lands, not where the store lands.** In
  `BoatingSchool_TryLaunch`, `bt->key.w = key.w; bt->next = g_bs_boats;` puts
  `mov eax,[g_bs_boats]` two instructions late (the only mismatch in the whole
  body); the reversed source order is exact. The two STORES stay in the
  compiler's own order either way — it is the load that moves. Same lever, and
  the same fix, at the allocation in `BsWater_SetTile`
  (`w->next = g_bs_water; w->prev = 0;` — the reversed order costs 2).
* **`memset(p, 0xf1, n)` is the readable spelling of `mov eax,0F1F1F1F1h /
  rep stosd`.** With `/O2`'s intrinsics on (declare `memset` and
  `#pragma intrinsic(memset)`), a non-zero byte constant broadcasts to a dword
  and the fill needs no remainder when the size is a multiple of four. First
  use of a non-zero memset constant in this tree.
* **An extern prototype's `unsigned short` parameter is what leaves the
  caller's upper half dirty.** `Carousel_TickInstance` passes the record's map
  square to `GetAllBlokesOffRide` as `mov cx, word ptr [esi+4]` INTO the
  register that still holds the just-incremented tick counter, and pushes the
  whole dirty `ecx`. An `int` parameter zero-extends and costs the match.
  `rides.c` already declares 0x0048a390 that way; this is the caller-side
  confirmation.
* **A dword flag field read once and bit-twiddled in `ah` is just direct field
  access.** `rec->flags & 1`, `rec->flags & 0x4000`, `rec->flags &= ~0x4000`
  and `rec->flags |= 0x4000` on an `int` field give one `mov eax,[esi+0Ch]`,
  `test al,1`, `test ah,40h`, `and ah,0BFh` / `or ah,40h` and a dword
  write-back — no local, no cast, no union. This refines the recorded
  "u16 flags `|=` narrows to a byte OR, `&= ~` does not": on a DWORD field
  both narrow, because the value is already live in a register.
* **Four consecutive `__cdecl` calls whose results all land in locals share
  ONE `add esp`.** `BsWater_WalkStep`'s four `BsWater_FindAt` calls are
  cleaned up by a single `add esp,20h` after the fourth. This is the exact
  converse of the recorded "a call result passed straight into another call
  splits the add esp" — assigning each result to its own local is what merges
  them, and the merge then shifts every `[esp+N]` argument read in between
  (the `key` parameter is read at `[esp+34h]`, before the adjustment).
* **Four identical neighbour blocks written out in full, not looped.** In
  `BsWater_WalkStep` each of the four results gets its own callee-saved
  register (edi/ebx/ebp/eax) and its own copy of the six-instruction body. An
  array of four pointers or a loop over them reproduces neither the register
  assignment nor the block layout — and the "an ARRAY of pointers reserves all
  its frame homes" rule says why it must not be an array.
* **A split prologue on a leading null test.** Both `BsWater_WalkStep`
  (`push esi` before the test, `push ebx/ebp/edi` after) and `BsWater_SetTile`
  (`push ebx/esi` before, `push ebp/edi` after the allocation check) do it,
  and in `BsWater_SetTile` the allocation-failure exit therefore has its OWN
  two-pop epilogue while the normal exit pops four. Writing the guard as an
  early `return` inside the `if` is enough; no construct needed.
* **Two argument slots reused as locals is what keeps an 8-byte frame.**
  `BsWater_SetTile` has a 2-byte key and one compiler temp in `sub esp,8`,
  and puts the outer row counter in `mask`'s slot and `x - 2` in `owner`'s —
  both written after the argument's last read. Nothing in the source asks for
  this; it falls out of `for (r = 0; ...)` and the `x - 2 + c` induction
  variable once the arguments are dead, which is a reason NOT to copy
  arguments into named locals.
* **VC6 collapses a `[mask][r*5+c]` subscript into ONE induction variable
  across BOTH loops.** In `BsWater_SetTile` `ebp` is set to
  `shapes + mask*25` in the outer preheader and only ever `inc`-ed in the
  inner latch — it is never re-seeded per row, so the outer loop's stride is
  carried implicitly by the five inner increments. Ten spellings of the
  subscript (flat, 2-D, 3-D, hoisted row pointer, index local, explicit
  running counter) all produce exactly this.
* **NEGATIVE RESULT — induction-variable emission order in the loop latch is
  not reachable by source ordering.** `BsWater_SetTile`'s only residual is
  that `inc ebp` is emitted second in our inner latch and fourth in the
  original's. The obvious hypothesis (the IV list is built in first-use order,
  so put the shape access first in the body) is DISPROVED: taking
  `&shapes[mask][r*5+c]` into a pointer before the cell fetch is byte-identical
  to the committed body. Twenty variants measured, listed above the marker;
  none moved the instruction, and every variant that DID move it added
  instructions or a frame slot. `strict == register-blind == offset-blind`, so
  by the §6B triage this is a pure scheduling permutation — record it and
  move on.
