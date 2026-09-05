# fable-b lane `workorder3.c` — work orders and the print list

New file: `LEGOLAND/workorder3.c` (three functions, 221 instructions).
Objects `/tmp/fb_workorder3_*`; scratch `scratchpad/fb_workorder3/`.

| address | name | insns | bytes | audit | mismatch | residual class |
| --- | --- | --- | --- | --- | --- | --- |
| 0x00485bd0 | `InsertPrintItem` | 77/77 | 244/244 | **[OK]** | 0 | — |
| 0x00482430 | `BuildPTPRoute` | 76/76 | 152/152 | [WIP] | 10 | allocation |
| 0x00499d60 | `UnlinkGardenerOrder` | 68/68 | 198/193 | [WIP] | 34 | structural (block layout) |

`audit.py LEGOLAND/workorder3.c` ends PASS; `/W3 /O2 /Gy /Gd` is silent.

---

## Mechanics recovered

### The print list is a sorted DOUBLY-LINKED LIST, not a binary tree

`blokeai.c` ("`InsertPrintItem` … binary-tree it by the KEY") and
`printlist.c` ("+0x00 left — tree link (InsertPrintItem)") both read the two
head words as tree pointers. They are not.

- **+0x00 is PREV, +0x04 is NEXT** of one list kept sorted ascending by the
  depth key at +0x08, and **0x0066b5a4 is its HEAD**. `DrawAndClearPrintList`
  walking +0x04 is walking the list, not an in-order traversal.
- **0x007fd600 is a ROVING CURSOR** — the node inserted last. Each insert
  starts its linear walk there, which is what makes sorting a frame cheap:
  consecutive sprites are usually adjacent in depth. The cursor is stored at
  **every step** of the walk, not only at the end, so an interrupted walk
  still leaves it pointing into the list.
- The insert is: walk back while `it->key < cursor->key`, or forward while
  `it->key > cursor->key`, stopping at a null link; then splice `it` before
  the cursor if it still sorts lower, else after it. The head global is
  updated only when `it` becomes the first node (empty list, or spliced in
  front of a cursor with no `prev`).

### The point-to-point route walk-back (`BuildPTPRoute`)

`PTPSuggestNextMove` (bnvmove.c) floods tiles outward with 16-byte nodes
`{next, parent, x, y}` and parks the node that reached the target in
`g_ptp_found` (0x0066b454). `BuildPTPRoute` walks the **parent** chain from
there back to the START tile, keeping the last four nodes it passed —
`a` (the tile we stand on, the chain root), `b`, `c`, `d` — then pushes
exactly ONE tile onto the route list: the tile to step to next.

- **0x00482330 named for the first time: `PTPShortcutSteps(a, b, c, d)`.** It
  returns how many of `b, c, d` may be skipped in one step: `0` → step to `b`,
  `1` → step to `c`, `2` → step to `d`. It answers 0 as soon as `b` or `c` is
  null, and otherwise corner-cuts: for each diagonal it forms it looks the
  intervening tile up in the cell grid (0x00801400) and refuses when the tile
  is not walkable (Cell +0x10 bit 1) or is blocked (Cell +0x1d bit 3).
- **0x00482300 named for the first time: `AddPTPRouteNode(x, y)`** —
  `malloc(0x10)`, push on the FRONT of the route list (0x0066b458), fill
  +8/+0xc. It silently does nothing when the allocation fails.
- **The return value is `d == 0`** — TRUE when the route back from the target
  is shorter than four tiles. That is what makes `PTPSuggestNextMove` answer
  2 and aim the walker straight at the target instead of at a tile centre.
- `case 1` and `case 2` dereference `c` / `d` with no null test; they are safe
  only because `PTPShortcutSteps` returns 0 whenever an argument is null.

### The gardener order unlink (`UnlinkGardenerOrder`)

The noisy shared unlink `FreeGardenerOrder` / `GiveBackGardenerOrder` call
(workorder2.c has the mechanic twin written out in line, silently). It
DBPrintf's the list state at every step: the node and its cell, whether it is
last, whether it is first, and the head/tail pair afterwards. Confirms
workorder2.c's reading of the list triple (head 0x0079a8b0, tail 0x0079a8b4).

---

## Original bugs reproduced

- **`UnlinkGardenerOrder`, the "not found" report** —
  `"    Work order not found (%x) at (%d,%d)"` has THREE conversions and only
  two arguments are pushed (`o->pos.x`, `o->pos.y`). The order pointer the
  `(%x)` is for was left out, so `%x` eats the x coordinate, the first `%d`
  eats the y, and the second `%d` prints whatever is on the stack.
- **`InsertPrintItem`, both debug checks** — the "Not enough RAM for sprite
  sort list" message is printed when `it` is null and the function then
  dereferences `it` anyway, so the diagnostic is immediately followed by the
  fault it exists to explain; and the closing `if (!it)` "Bad stuff in the
  sprite sorter" check is dead for the same reason.
- **`InsertPrintItem`, the empty-list path** does not touch the cursor global
  before using it — the leading `if (!g_print_cursor)` only prints.

---

## Levers, with evidence

- **A `while` loop's condition is whichever test VC6 leaves in the LATCH, and
  the one it peels to the top doubles as the enclosing `if`.** `InsertPrintItem`'s
  two search walks are `if (key < cur->key) { while (key < cur->key) { if
  (!cur->prev) break; cur = cur->prev; } } else if (key > cur->key) { … }` —
  the *redundant* re-test in the `while` head is the whole point: VC6 peels it,
  the peeled copy IS the `if`'s compare, and the `else if` reuses its flags
  (`cmp / jge / … / jle`, one compare for two tests). Spelling either arm the
  other way round — `while (cur->prev) { advance; if (key >= cur->key) break; }`
  — makes VC6 peel the NULL test instead and duplicate it into the latch:
  three instructions longer per arm, and it is the only difference between
  83 and **77 of 77**. `for (;;)` with a leading `if (!link) break;`, a
  `do { if (!link) break; advance; } while (key < cur->key);` and explicit
  backward `goto`s are all byte-identical to the wrong-way `while` — VC6
  normalises them. **Read the latch off the original to find out which test
  the source loop was written on.**
- **Writing a search's failure arm as the `else` of `if (p)` keeps ONE copy of
  it; writing it as `if (!p) { …; return; }` emits TWO.** In
  `UnlinkGardenerOrder` the loop-exhausted edge proves `p == 0`, so VC6 folds
  the `!p` test away on that path and then duplicates the whole failure block
  rather than jumping to the other copy. The `if (p) { unlink } else { fail;
  return; }` spelling gives the original's single block reached by a backward
  `je`. Worth 10 instructions and the exact instruction count.
  (Same family as workorder2.c's `FreeMechanicOrder`, whose dead `if (p)` after
  the search VC6 also cannot see through — that dead test is in both originals.)
- **Two written-out copies of a shared tail are merged only if they are
  INSTRUCTION-IDENTICAL, and reading the same global back after storing to it
  defeats that.** In `UnlinkGardenerOrder`'s head arm VC6 forwards the just
  stored value (`push edi`) where the other copy reloads (`mov ecx,[head]`),
  so no merge. **A volatile STORE does not stop the forwarding** (measured:
  `*(WorkOrder* volatile*)&g_head = nx;` still gives `push edi`), and neither
  does an array/union view of the adjacent head+tail pair — the recorded
  "array view breaks a store-to-load unification at non-zero offsets" lever
  does not reach a load at the SAME offset as the store.
  **What does work is a pair of volatile reads hoisted into locals:**
  `{ T* t = *(T* volatile*)&tail; T* h = *(T* volatile*)&head; f(h, t); }`
  reproduces the original's `mov eax,[tail] / mov ecx,[head] / push eax /
  push ecx` exactly and makes the two copies identical, and VC6 then merges
  them. Note the ordering rule this exposes: **with only ONE of the two reads
  volatile the loads cannot both hoist above the first `push`** (you get
  `mov / push / mov / push`), and a single volatile read taken inline is
  always emitted FIRST regardless of source order — so both reads have to be
  volatile locals, in the order the original loads them.
- **…but the merge glues the survivor to the LAST arm, not the then arm.**
  Measured on `UnlinkGardenerOrder` with the identical-copies spelling above:
  VC6 keeps the copy at the search arm and rewrites the head arm's into a
  forward jump — the exact mirror of the original, which keeps the head arm's
  copy and jumps backward from the search. This CONTRADICTS DECOMP's
  eleven-proof entry ("glues it to the THEN arm") and CONFIRMS the
  `UpdateRiverAnim` entry ("always glues the merged tail to the LAST arm and
  exiles the FIRST"). On this evidence the LAST-arm rule is the general one.
- **A local copy of a pointer does not add a reference to its web.**
  `{ PTPNode* t = c; f(t->x, t->y); }` is byte-identical to `f(c->x, c->y)` in
  `BuildPTPRoute` — the copy coalesces and the count does not move. This is
  the fable-a "splitting a parameter's references across a local copy does not
  split its web" rule seen from the other side: it does not GROW one either,
  so a copy is not a way to buy a callee-saved tie-break.
- **Declaration order sets the order of the leading `xor`s and nothing else.**
  All 24 orders of `BuildPTPRoute`'s four locals produce the same register
  ASSIGNMENT and differ only in which register is zeroed first — worth 2 of
  its 10 mismatches, and free. Sweep it, but do not expect it to fix a
  colouring.
- **A `switch`'s case order is inert when VC6 lowers it to a compare chain.**
  All six orders of `BuildPTPRoute`'s `case 0/1/2` (and an equivalent
  `if / else if` chain, and hoisting the call result into a local) are
  byte-identical, block layout included. The recorded "case ORDER decides
  block layout" lever applies to jump-table switches; a three-case
  `sub eax,0 / dec / dec` chain is laid out by the case VALUES.

---

## Residuals

### `BuildPTPRoute` — 10 of 76, byte-exact, ALLOCATION

`b` and `c` hold each other's callee-saved register: the original puts `b`
(the node stepped to when the shortcut test says 0) in `esi` and `c` in `edi`;
we get the opposite. Every instruction is right and the body is 152/152 bytes,
so this is `strict >> register-blind` — the allocation row of the §6B triage.

Measured inert (all byte-identical): 24 declaration orders; 6 switch case
orders; an `if/else if` chain; the call result in a local; `register` on
either local; `void *`, `int` and `unsigned int` retypings of either local;
five loop spellings (`while`, `for`, guarded `do/while`, hoisted-parent walk,
temporaries in four arrangements); `b`+`c`, `b`+`c`+`d`, `b`+`d` and `c`+`d`
struct wrappers; volatile stores at either definition site; a free volatile
read on either field load; `!d` for `d == 0`; `b != 0` for `b`; `return d==0`
instead of `break` in case 0; and two extern prototypes for the callee.

The one thing that moves it is a **reference-count change**: adding a
redundant `if (c)` to case 1 gives `b` the original's `esi` (and costs two
instructions). So the original's `c` outranks its `b` by one weighted
reference that we have not found a free way to spend — `b` has four uses
(call argument, the null test, `->x`, `->y`) against `c`'s three. Worth
re-testing if a "free extra reference to a pointer" lever is ever found.

### `UnlinkGardenerOrder` — 34 of 68, STRUCTURAL (block layout)

Every instruction is present and correct; the two bodies differ only in where
the shared tail block sits.

```
original :  [head arm]  [START printf + epilogue] [ret]  [search | notfound | found]
                             ^ head arm falls in            ^ found path jumps BACK
ours     :  [head arm]  [search | notfound | found]  [START printf + epilogue]
```

VC6 always emits `[then][else][merge]`, so `[then][merge][else]` is reachable
only by cross-jumping two written-out copies of the tail — and, as measured
above, the merge glues the survivor to the LAST arm. Putting the head case
last (which would make it the last arm) flips the compare to `je` and diverges
at index 23 instead: the original's `cmp dword [0x79a8b0], edi / jne` fixes the
head case as the THEN arm.

Seventeen spellings measured, all landing on one of those two layouts:
`if (!p) {fail; return;}` vs `if (p) {} else {}` for the search failure;
`goto` forms for the failure block, for the search block and for the join
(all normalised and inverted by the front end); the join reached by a `goto`
from inside the loop; the whole body nested in `if (p) { … }`; a
`do { … } while (0)` with `break` in either arm; the arms swapped with the
compare negated; two written-out copies of the tail with and without volatile
reads (77 and 68 instructions, the latter merged onto the wrong arm); and
partial-volatile variants. **Recorded as unreachable rather than unfound**,
the same verdict `UpdateRiverAnim` carries.

---

## Extern-type divergences and naming

- `DBPrintf` is declared `void DBPrintf(const char*, ...)`, as in
  `workers2.c` / `savegame.c` (logflume2.c's one-argument spelling is a
  different caller-side lever and is left alone).
- `PrintNode`'s +0x00 is named `prev` here where `printlist.c` names it
  `left`; the offsets agree, the reading does not (see above). `printlist.c`
  is not edited; the correction belongs in its header comment when someone
  next touches it.
- `g_printlist` (0x0066b5a4) keeps `printlist.c`'s name. 0x007fd600 had no
  name in the tree and is `g_print_cursor` here.
- `WorkOrder` is declared with only the three fields this file touches
  (`next`, `obj`, `pos`); `workorder2.c` has the full 0x3c record.
- 0x00482300 `AddPTPRouteNode` and 0x00482330 `PTPShortcutSteps` are new names;
  no other file declares either address.
