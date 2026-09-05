# Scope K — `LEGOLAND/pathmask.c` (path edge/corner masks, cursor tiles)

New-file lane. `python3 tools/audit.py LEGOLAND/pathmask.c` ends **PASS**;
`/W3` is clean. **5 of 6 exact, 422 of 422 instructions ported, 352 of them
byte-exact.**

## Per function

| address | name | insns | pct | audit | promotable | marker committed |
| --- | --- | --- | --- | --- | --- | --- |
| 0x00482a40 | `ResolveEntrancePathSquare` | 20 | 100% | `[OK]` | yes | `// FUNCTION: LEGOLAND 0x00482a40` |
| 0x00455ee0 | `FreeCachedTextEntry` | 41 | 100% | `[OK]` | yes | `// FUNCTION: LEGOLAND 0x00455ee0` |
| 0x00461080 | `DrawCursorTileAt` | 43 | 100% | `[OK]` | yes | `// FUNCTION: LEGOLAND 0x00461080` |
| 0x0045c900 | `IsPathRectClear` | 70 | see below | `[WIP]` | n/a | `// WIP-FUNCTION: LEGOLAND 0x0045c900  (70i extent, 189B; ours 72i, first diverging index 5; STRUCTURAL — one derived induction variable over the row table; at its floor, see the note above)` |
| 0x0045d080 | `PathCornerMask` | 99 | 100% | `[OK]` | yes | `// FUNCTION: LEGOLAND 0x0045d080` |
| 0x0045ceb0 | `PathEdgeMask` | 149 | 100% | `[OK]` | yes | `// FUNCTION: LEGOLAND 0x0045ceb0` |

Every name came from the callers' existing extern declarations
(`render5.c`, `pathmisc2.c`, `tinystubs.c`); nothing was renamed. All five
exact bodies end in a real `ret` and none is recursive.

### `IsPathRectClear` — retired at its floor

`audit.py` reports `mismatch=58` because it compares index-for-index and our
body carries two extra instructions from index 5 onwards. Diff-aligned the
picture is much smaller:

| measure | orig | ours | matched | mismatch |
| --- | --- | --- | --- | --- |
| strict | 70 | 72 | 44 | 28 |
| register-blind | 70 | 72 | 55 | 17 |
| register + immediate blind | 70 | 72 | 67 | 5 |

The block layout, both loop tests, both epilogues, the frame (with `bottom`
homed in the dead `rect` argument slot), the `x * 0x14` outer induction
variable and the `test edx,edx` form of the `x >= 0` test are all the
original's. The ONE difference is where the row table's base lives:

```
original   outer preheader:  mov ebx,[0x801400]          ; g_map_rows, kept in EBX
           inner body:       mov esi,[ebx+eax*4]
ours       inner preheader:  mov edx,[g_map_rows] / lea edx,[edx+eax*4]
           inner body:       mov esi,[edx]
           inner latch:      add edx,4
```

VC6 builds a derived induction variable over the row pointers, which costs the
two extra instructions and takes the register the original spends on the base
— which is why `x` lands in EBX for us and in EBP there, and the rest of the
allocation follows.

**Ruled out** (every one still 72 or 73 instructions, all diverging at index
5): a `Cell** rows = g_map_rows;` function-scope local (copy-propagated
straight back to the global), the same local scoped inside the outer loop body
and inside the inner loop body, `*(rows[y] + x)`, `(char*)rows[y] + x*20`,
`(char*)g_map_rows[y] + x*sizeof(Cell)`, `(*(Cell**)((char*)g_map_rows +
y*4))[x]`, a named `Cell* row` and a named `Cell* c`, a manually spelled
`xoff` outer IV with the `x >= 0` test written on it, `px`/`py` copies of the
loop counters, `while` loops, a `do`/`while` outer with the guard split out
(74), `cell` declared in the inner scope, dropping the dead `tile = 0` store,
both orders of the two failure tests, both orders of the else stores, the
bounds test with `y` first, and the whole probe as a `static __inline` helper
taking a `Cell*` out-parameter. A `*(Cell** volatile*)&g_map_rows` read is
NOT free here — it anchors above the loop guard and takes a frame slot (75–76
instructions) — and a free volatile read of `g_map` inside the probe (the
original reloads it every iteration, so it costs nothing) does not act as a
barrier either: VC6 still schedules the `g_map_rows` load above it.

The residual is one code-motion decision, not a spelling. Reopen only with a
construct that hoists a global load to the OUTER preheader while leaving the
inner loop's index unreduced.

## Mechanics recovered

- **The four-neighbour path mask.** `PathEdgeMask` probes N, E, S, W in that
  order and ORs `0x1 / 0x2 / 0x4 / 0x8`. On the isometric grid `0x3` (N+E) and
  `0xc` (S+W) are the two DIAGONAL pairs, which is what `render5.c`'s header
  means by "one diagonal pair / the other". The mask indexes
  `g_tile_sprites[code + mask + 3]`.
- **The corner filler is a HOLE test.** `PathCornerMask` answers bit 0 when the
  `0xc` pair is complete *and* `(x-1, y+1)` is not path, bit 1 when the `0x3`
  pair is complete *and* `(x+1, y-1)` is not path. So an inside corner gets a
  filler only where the diagonal cell behind it is missing; the three
  combinations pick `g_tile_sprites[code + 19 / 20 / 21]`.
- **The probe is the same one `simcore.c`'s `GetPathNeighbours` uses**: the cell
  is COPIED (5-dword `rep movsd`) into a function-level local and an off-map
  cell stands in as `tile = 0`, `flags = 0x40`, `rf = 0`, so anything off the
  map is never path. Three functions here spell that same probe; see the lever
  on dead-store elimination below.
- **`IsPathRectClear` is NOT `IsPathCell`.** Every cell of the inclusive
  rectangle must carry map flag `0x10` (a path tile) and must NOT carry RF bit
  1 (blocked); RF bit 0 does not rescue a cell the way it does in `IsPathCell`.
  An empty rectangle answers 1. `x` is the outer loop, `y` the inner.
- **`DrawCursorTileAt` is the cursor's per-cell filter**: bounds-check,
  `IsPathCell`, and a non-zero displayed tile; the survivor goes to the
  four-argument overlay twin 0x00460f50 with the caller's blit mode, and it is
  the CALLER'S `Pos` that is handed on, not the fetched cell.
- **The rendered-text cache compacts on removal.** `FreeCachedTextEntry`
  traces `"Deleting Cell (%d) %s\n"`, decrements the count FIRST, frees the
  text, kills the sprite (and nulls the field), then slides
  `g_text_cache[j] = g_text_cache[j+1]` up to the NEW count — leaving the last
  slot a stale duplicate. That is exactly why `render5.c`'s `ExpireCachedText`
  does not advance its index on the free arm and re-reads the count every
  iteration.
- **`ResolveEntrancePathSquare` is a flood fill's reset + seed.** It clears
  flag 2 on every square of `pathsq.c`'s list (head 0x0066b44c), then finds the
  square containing the entrance tile and hands it to 0x004829c0, which sets
  flag 2 and recurses over the neighbour set 0x004819a0 collects. Flag 2 is
  therefore "reachable from the park entrance".

## Callees / globals named for the first time

| address | name given | contract | insns |
| --- | --- | --- | --- |
| 0x004829c0 | `MarkPathSquareReachable(PathSquare* sq)` | sets `flags |= 2`, collects the square's neighbours through 0x004819a0, copies the neighbour array to the heap and recurses into every square that does not carry the flag yet; frees the copy | 52 |
| 0x00460f50 | `DrawCursorPathTile(Pos* at, int x, int y, int mode)` | `render5.c`'s `DrawPathTileOverlay` with the corner draws routed through the five-argument `PrintSprite`; declared here with the four arguments the disassembly pushes | (not in this scope) |

Names checked against the whole tree first; neither collides. Globals already
named elsewhere and reused unchanged: `g_path_squares` (0x0066b44c),
`g_text_cache` / `g_text_cache_count` (0x006675c0 / 0x006675b8), `g_map`,
`g_map_rows`. New string constant: `kFreeTextFmt` (0x004b9098,
`"Deleting Cell (%d) %s\n"`).

## Original bugs reproduced

None found in these six. `FreeCachedTextEntry`'s "stale last slot" is a
consequence of decrementing the count before compacting, not a defect — the
entry beyond the count is never read.

## Extern-type divergences (deliberate — do not "align")

- **`g_text_cache_count` (0x006675b8) is declared `volatile int` in this
  file**, where `render5.c` declares it plain `int` and casts at the one read
  that needs it (`*(volatile int*)&g_text_cache_count`). Both TUs need the same
  barrier; the two files reach it from opposite sides. See the lever below.
- **`PathCornerMask`'s first parameter is `char`** here, matching `render5.c`'s
  declaration — the whole body is byte-wide (`mov bl,[esp+0x20] / mov al,bl /
  and al,0xc / cmp al,0xc`) and `edges` stays in EBX for the function.
  Declaring it `int` would make the caller mask before the push.
- **`DrawCursorPathTile` (0x00460f50) is declared with four arguments** —
  `render5.c` documents the same function as the four-argument twin of its own
  three-argument `DrawPathTileOverlay`.
- **`IsPathCell` (0x0045ce10) is `int IsPathCell(Cell*)`**, the same spelling
  `pathmisc2.c`, `pathtile2.c` and `render4.c` use.

## Levers, with evidence

- **A `volatile` DECLARATION on a counter global is worth 5 instructions and
  fixes two unrelated defects at once.** In `FreeCachedTextEntry` a plain
  `extern int g_text_cache_count` let VC6 (a) schedule the `.text` reload for
  the free ABOVE the count's read-modify-write (our indices 10–14 came out
  `load count / load text / dec / push / store count` against the original's
  `load count / dec / store count / load text / push`) and (b) hoist the
  compaction loop's bound out of the latch into a `count - i` trip count with a
  `dec/jne` down-count. Declaring the global `volatile int` costs NOTHING —
  the original emits exactly the load/dec/store and the two reloads — and
  closed both: 31/45 → 41/41. `render5.c`'s `ExpireCachedText` needed the same
  barrier at its one read and got it with a cast. **When two functions in
  different TUs share a counter global and both need its reloads, suspect the
  original's header declared it volatile.**
- **A `rep movsd` struct copy out of an ARRAY is not the same construct as one
  out of a POINTER.** `g_text_cache[j] = g_text_cache[j+1]` gives the
  original's single cursor (`lea esi,[eax+0x20] / mov edi,eax / rep movsd /
  add eax,0x20`); `p = &g_text_cache[i]; … *p = p[1]` with `p++` in the for
  increment manufactures a SECOND induction variable for the source pointer,
  needs a fourth callee-saved push for it, and lands at 45 instructions for the
  original's 41. The array form is also what lets VC6 prove the copy cannot
  alias the count (which is then the volatile's job to un-prove). The recorded
  "two lockstep cursors spelled the SAME way let VC6 eliminate an IV", measured
  from the losing side.
- **Whether the off-map stand-in cell's DEAD store survives tells you whether
  the copy's address is taken.** All three probes here are the same source
  macro (`cell.tile = 0; cell.flags = 0x40; cell.rf = 0;`), but
  `PathEdgeMask`/`PathCornerMask` emit all three stores while `IsPathRectClear`
  emits only two. The difference is that the first two pass `&cell` to the real
  `IsPathCell` call, so nothing about the local is dead, while
  `IsPathRectClear` reads the two flag bytes straight out of the local and VC6
  drops the unread `tile`. `simcore.c` records the same macro dropping only its
  LAST probe's store for the same reason. **Reading a dead-store pattern
  backwards identifies the callee: a probe that keeps every store is one whose
  copy escapes.**
- **A probe macro's coordinate arguments must be pre-computed LOCALS, not
  expressions, or VC6 folds the ±1 into the addressing mode.** Writing
  `PROBE(at->x - 1, at->y + 1)` (the macro evaluating each argument three
  times) makes VC6 load `at->x` early, defer `at->y` to the second test, and
  fold both offsets into the copy's address —
  `lea esi,[eax+ecx*4-0x14]` and `mov ecx,[edx+ecx*4-4]`, 87 of 99 for
  `PathCornerMask`. Assigning `px = at->x - 1; py = at->y + 1;` first gives the
  original's `mov eax,[ebp] / mov ecx,[ebp+4] / dec eax / inc ecx` and closes
  the function outright. `simcore.c`'s `GetPathNeighbours` uses the same
  discipline for the same reason; this is the measurement of what happens when
  you don't.
- **VC6 schedules the MODIFIED coordinate's load first, whatever the source
  order.** In `PathEdgeMask` all four probes are written `px = …; py = …;` in
  that order, yet the original (and our exact match) loads `at->y` first in the
  N and S probes and `at->x` first in the E and W probes — always the one
  carrying the `inc`/`dec`. So the load order is NOT a source-order lever here
  and should not be chased; write the pair in coordinate order and let the
  scheduler pick.
- **Four repeats of one probe with a byte accumulator give three different
  bit-set forms, and all three are automatic.** `PathEdgeMask`'s first hit is
  `mov byte ptr [esp+0x13],1` (VC6 knows the mask is still zero), the middle
  two are `or byte ptr [esp+0x13],2/4` in memory, and the last is folded into
  the epilogue as `mov al,[esp+0x13] / pops / je / or al,8`. One `char mask`
  local plus four `mask |= bit;` statements produces all of it.
- **A zero register appears in a probe chain when a callee-saved register is
  pushed anyway and the function has no other use for it.** `PathEdgeMask` gets
  `xor ebx,ebx`, so every `>= 0` test is `cmp reg,ebx` and every off-map zero
  store is `bl`/`bx`; `PathCornerMask`, whose EBX holds the `edges` parameter
  for the whole body, emits the identical probe with immediates instead. Same
  source macro, two spellings, decided entirely by what else wants the
  register.
- **The lazy bounds-check inline (`CellForPos`) and the eager one (`MapCellAt`)
  are distinguishable in the listing.** `DrawCursorTileAt` reads `at->y` only
  AFTER `at->x` has passed its width test (`mov ecx,[edi+4]` at index 11, below
  `cmp eax,ecx / jge`), which is `pathmisc2.c`'s `CellForPos`; the eager form
  reads both up front. Picking the right one closed the function on the first
  compile.
- **`cell != 0` after that inline is a real third `&&` term.** The inline has
  already branched to the same exit on every out-of-bounds arm, so the merged
  `test esi,esi / je` looks redundant — but VC6 emits it, and dropping the term
  from the source loses an instruction. Three tests in the listing, three `&&`
  terms in the source.

## Follow-up: IsPathRectClear closed (70/70, mismatch 0)

A second pass of five lensed attempts (template transfer, induction-variable
constructs, loop form, types, fresh reconstruction) on private copies, with
independent verification, closed the residual on the first round; three of the
five converged on the same spelling.

- **The two loop counters are ONE `Pos` aggregate, not two `int`s.** VC6 SP3
  does not strength-reduce an array index that is an aggregate member, so
  `g_map_rows[p.y][p.x]` stays a base+index access; the row-table load is then
  invariant in both loops and hoists to the OUTER preheader into `ebx`
  (`mov ebx,[g_map_rows]` once, `mov esi,[ebx+eax*4]` per inner iteration),
  which is the original, and the register web then puts x in `ebp` as the
  original does. With `int x, y` VC6 forms a derived IV over the row pointers
  in the inner preheader (`lea edx,[edx+eax*4]` … `add edx,4`), two
  instructions more, and the IV's initialiser anchors the global's load
  inside the outer loop.
- Isolation: `Pos` for both counters 70/70; `Pos` for the inner (row) counter
  only 70/70; `Pos` for the outer counter only 72i unchanged; an anonymous
  `struct { int x; int y; } p` 70/70. The lever is aggregate membership of the
  ROW index, not the `Pos` type.
- Template: `ScanPathArea5x5` 0x0045c9c0 in pathmisc2.c (lines ~269-291), the
  function immediately after this one in the binary, spells the identical
  off-map probe over an identical `Pos p` nested loop and already records the
  rule; fpui2.c's `BuildObjInfoList` 0x00481200 records the same lever from
  the frame-layout side. A scripted scan of all exact functions found only the
  LLIDB pair (`g_llidb_pages[i>>8][i&0xff]`, non-affine index) keeping the
  base+index form otherwise, which pointed at "an index the IV pass cannot
  recognise" as the class of fix.
- The twenty ruled-out spellings from the first pass are listed above; none
  of them changes the index's aggregate membership, which is why all of them
  measured 72 or 73 with the same first divergence.
