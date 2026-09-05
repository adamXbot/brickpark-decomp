# fable-b — parallel session notes (2026-09-05)

Findings from the four files docs/SCOPE_FABLE_B.md assigned to this session, for folding into docs/DECOMP.md at integration. Per-lane originals beside this file carry the evidence.

| file | exact | WIP | notes |
| --- | --- | --- | --- |
| `LEGOLAND/savechunks2.c` | 1 | 1 | `fable-b-savechunks2.md` |
| `LEGOLAND/workorder3.c` | 1 | 2 | `fable-b-workorder3.md` |
| `LEGOLAND/ridemisc.c` | 6 | 0 | `fable-b-ridemisc.md` |
| `LEGOLAND/sysmisc.c` | 5 | 2 | `fable-b-sysmisc.md` |

---

# savechunks2.c

## Lane `fable-b` / `savechunks2` — the script EVENT serialisers

`LEGOLAND/savechunks2.c`, created 2026-09-05. **One of two functions exact**
(`tools/audit.py LEGOLAND/savechunks2.c` → PASS, `/W3` clean, nothing left in
the repo root, all objects under `/tmp/fb_savechunks2_*`).

| address | name | insns | pct | audit | promotable | marker |
| --- | --- | --- | --- | --- | --- | --- |
| 0x0046c700 | `SaveScriptEvent` | 77 | 100% | `[OK]` | yes (real `ret`, not recursive) | `// FUNCTION: LEGOLAND 0x0046c700` |
| 0x0046c7e0 | `LoadScriptEvent` | 124 | 94.4% (mismatch 26) | `[WIP]` | n/a | `// WIP-FUNCTION: LEGOLAND 0x0046c7e0  (94.4%, 124/124 insns, mismatch 26; cold-block ORDER only -- the ``head = 0`` arm is emitted at index 95 instead of 117, first diverging index 95)` |

Neither function was renamed: both names come from `savechunks.c`'s existing
`extern` declarations. `SaveScriptEvent` matched on the FIRST draft (77/77).

---

## Mechanics recovered

### The `ScriptEvent` record is a LIST, not one record

`savechunks.c`'s header describes the pair as running "over the pending event
0x00668784", singular. It is a **linked list of 0x44-byte records** and the
link field doubles as the on-disk terminator:

```
 +0x00  next     chain link; a record whose next == (ScriptEvent*)-1 on disk
                 ENDS the stream. The writer's terminator is an all-zero 0x44
                 record with only +0x00 set to -1.
 +0x04  elem     LLIDB element. Serialised BY NAME, not by pointer: the writer
                 stores elem->name as a <script string>, the reader resolves it
                 with ElemID() and frees the temporary string.
 +0x08  text     an owned string, stored as a <script string>
 +0x0c  kind     NewScriptEvent's 1st argument
 +0x10  flags    bit 0x20 = "+0x08 is heap-owned"; FreeScriptEvent frees the
                 text only when it is set, so the loader sets it after a
                 non-NULL read
 +0x38  param    NewScriptEvent's 2nd argument
 +0x3c  time     absolute game time
```

Per node the stream is `u8 rec[0x44]` then TWO `<script string>`s (element name
then text). A NULL field is still written — as `SaveScriptString(NULL)`, which
the string serialiser stores as length −1 — so the record count and the string
count always agree.

### The timestamp is rebased in place, on the caller's live list

The writer does `ev->time -= g_script_now;` **into the caller's record**, traces
it, writes the record, then adds `g_script_now` straight back; the reader adds
the loading session's `g_script_now` and traces twice
(`"Loading Event, TimeStamp = %d"` at 0x004ba838 before, `"Fixed up to %d\n"` at
0x004ba828 after). So a save file's times are relative to the save's
`GetGameTimer()` and portable across the absolute clock.

### Callees named for the first time (script.c, 0x00468910 family)

| address | name | what it is |
| --- | --- | --- |
| 0x00468910 | `NewScriptEvent(kind, param)` | `calloc(1, 0x44)`, then redundantly re-zeroes +0x00/+0x08 and stores the two arguments in +0x0c and +0x38. **Returns NULL on failure and the redundant stores are guarded, but every caller here ignores the NULL.** |
| 0x00468940 | `FreeScriptEvent(ev)` | `if ((ev->flags & 0x20) && ev->text) free(ev->text); free(ev);` — this is what pins bit 0x20 to "+0x08 is owned". |
| 0x00468970 | `FreeScriptEventList(ev)` | recursive: frees `ev->next`'s chain, then `FreeScriptEvent(ev)`. |

Format strings: 0x004ba808 `"Saving Event, Timestamp = %d\n"`, 0x004ba828
`"Fixed up to %d\n"`, 0x004ba838 `"Loading Event, TimeStamp = %d"` (no newline).
Note the string pool order is Saving / Fixed / Loading, i.e. NOT first-use order
across the two functions — irrelevant to matching (each `.c` gets its own pool)
but worth knowing when reading the original's `.rdata`.

---

## Codegen levers (for `docs/DECOMP.md`)

- **A CALL RESULT STAYS IN `eax` ACROSS AN INTERVENING FIELD UPDATE, AND THAT IS
  HOW YOU SPELL "RESTORE, THEN TEST".** `SaveScriptEvent` emits
  `call SaveGameWrite / mov ecx,[g_script_now] / mov edx,[esi+0x3c] / add esp,0x10
  / add edx,ecx / test eax,eax / mov [esi+0x3c],edx / je fail`. The restore is
  scheduled *between* the call and the test, so the source is
  `ok = SaveGameWrite(ev, 0x44); ev->time += g_script_now; if (!ok) return 0;` —
  a named temporary, not `if (!SaveGameWrite(...))`. Writing the test first puts
  the `add` after the branch. (Evidence: 77/77 first try.)

- **TWO ARMS THAT CALL THE SAME FUNCTION ARE TWO TEXTUAL CALLS, NOT A TERNARY
  ARGUMENT.** `SaveScriptEvent` has four `call SaveScriptString` sites, in pairs
  of `test/je` + `jmp` over an `else` that pushes 0. Each arm carries its own
  `add esp,4`, which is the tell: a ternary argument (`f(p ? p->name : 0)`) would
  materialise the value in one register and leave ONE call and ONE `add esp,4`.
  This is the same "a pending cdecl `add esp` cannot cross a branch join" lever
  seen from the other side.

- **`memset` THE WHOLE STRUCT, THEN STORE THE ONE NON-ZERO FIELD.** The
  terminator record is `mov ecx,0x11 / xor eax,eax / lea edi,[esp+8] / rep stosd`
  followed by `mov dword ptr [esp+0x10], 0xffffffff` *after* both argument
  pushes. `memset(&term, 0, sizeof(term)); term.next = (ScriptEvent*)-1;` gives
  exactly that; a field-by-field zero-fill does not. (`#pragma intrinsic(memset)`
  is required for the `rep stosd`.)

- **THE SCRATCH REGISTER FOR A `rep stosd` IS PUSHED WHERE IT IS FIRST NEEDED,
  NOT IN THE PROLOGUE — AND THE SHARED `return 0` BLOCK THEN POPS ONE REGISTER
  FEWER.** `SaveScriptEvent` pushes only `esi` at entry and `push edi` at the top
  of the post-loop terminator block (0x0046c79f); its five `return 0` sites
  branch to a tail that pops `esi` alone. This falls out for free when the only
  address-taken aggregate lives *after* the loop — no `goto`, no scoping trick
  needed. The `pop edi` then lands **inside** the `neg/sbb/neg` of
  `return SaveGameWrite(&term, 0x44) != 0;` (between the `sbb` and the second
  `neg`); do not try to "fix" that.

- **A `u8` FLAG FIELD OR'd WITH A SMALL CONSTANT IS A BYTE `or`.**
  `ev->flags |= 0x20;` on an `unsigned char` at +0x10 →
  `or byte ptr [esi+0x10], 0x20`. (Confirms the existing "u16 `|= 0x100` narrows
  to a byte OR" entry from the byte side.)

- **THE VALUE JUST STORED IS THE VALUE PUSHED.** `sub edx,eax / mov eax,edx /
  mov [esi+0x3c],edx / push eax` is `ev->time -= g_script_now;` followed by
  `DBPrintf(fmt, ev->time);` — VC6 keeps the computed value in a second register
  for the argument rather than reloading the field.

### NEW, and the one that cost this lane its second function

- **VC6 LAYS COLD BLOCKS OUT IN SOURCE-GENERATION ORDER, AND A SINGLE-
  PREDECESSOR `goto` TARGET IS GENERATED INLINE WITH THE BRANCH THAT REACHES IT.**
  So the cold arm of an `if` inside an already-cold block is emitted IMMEDIATELY
  after that block — moving the label to the end of the function does not move
  the code. Two cold blocks reached from the *main* chain (here the two
  `if (g_script_errors)` handlers) are then emitted after it, in source order.
  Measured over ~45 spellings in `LoadScriptEvent`.
  *Corollary confirmed by the same sweep:* which arm is inline follows the
  SOURCE ARM ORDER — `if (prev) A; else B;` puts `A` inline and `je` to `B`;
  swapping the arms flips to `jne`.

- **VC6's CONSTANT PROPAGATION IS GLOBAL, NOT BLOCK-LOCAL, AND IT WILL DELETE A
  WHOLE RETURN BLOCK.** `L: head = 0; return head;` as the last statement of a
  function becomes `return 0` → `xor eax,eax` → cross-jumped into a neighbouring
  handler's `return 0` tail, so the block *vanishes* (124 → 117 instructions).
  It survives every disguise tried: `return head = 0;`, `*&head = 0;`, a cast,
  `memset(&head,0,sizeof head)`, a second variable coalesced with `head`,
  `head = prev;` (VC6 knows `prev == 0` from the compare that got you there),
  and splitting the store from the return across a label or a `goto`.
  **Therefore: an original that shows `xor <callee-saved>,<callee-saved>` followed
  by `mov eax,<that register>` is proof of a TWO-PREDECESSOR JOIN** — a phi copy
  in one arm plus a tail-duplicated shared `return v` — not a straight-line
  `v = 0; return v;`.

- **…AND THE TWO ARE RIGIDLY COUPLED.** Every spelling that keeps the join (and
  so the un-folded `xor ebx,ebx`) also drags the block back adjacent to its
  `if`; every spelling that leaves the block last folds it away; and the
  spellings that keep it last *and* unfolded invert the branch (`jne` at index
  87 with the block at 88 instead of `je` with the block at 117). This is the
  unresolved residual below — worth a dedicated lever hunt, because the same
  pattern (a cold `if`'s arm exiled past two other cold blocks) shows up
  wherever a list builder has a "nothing was loaded" exit.

---

## Original bugs reproduced (commented at the site)

1. **`LoadScriptEvent` leaks the element name on the first error path.** After
   `name = LoadScriptString();`, if `g_script_errors` is set the record is
   released with the raw CRT `free()` (not `FreeScriptEvent`) and `name` is
   dropped on the floor. The *second* error path, after the text string, uses
   `FreeScriptEvent` — correct, because by then the record owns +0x08 and has
   bit 0x20 set. The asymmetry is real and is reproduced.
2. **`NewScriptEvent`'s NULL is never checked.** `LoadScriptEvent` calls it
   twice and reads 0x44 bytes through the result immediately; `0x00468910`
   returns NULL when its `calloc` fails.
3. **`SaveScriptEvent` mutates the caller's live list while writing.** The
   `-=` / `+=` pair straddles `SaveGameWrite`, so a failure *inside* the write
   leaves that node's `+0x3c` relative. (The `+=` is executed before the result
   is tested, so only a crash inside the write can expose it.)
4. Both ends ignore short reads/writes past the point where a partially built
   list has already been published — the same family defect `savechunks.c`
   documents for the worker and work-order chunks.

## Extern-type divergences and one-name-per-address notes

- `savechunks.c` declares `extern int SaveScriptEvent(void* ev);` and
  `extern void* LoadScriptEvent(void);` (it has no `ScriptEvent` type). This
  file defines the record and types them `ScriptEvent*` / `ScriptEvent*` —
  identical codegen, but **do not "align" savechunks.c to it**; its `void*`
  spelling is what its own call sites need.
- **0x0049e4d0 has three names in the tree.** The majority spell it
  `HeapFree_w` (`fpui.c`, `coaster.c`, `gpu.c`, `objmap2.c`, …); `data2.c` calls
  it `MemFree`; `memdb.c` and `logflume2.c` call it `free`. It is inside the
  statically-linked CRT (≥ 0x0049e000), so it really is `free`, and this file
  follows `memdb.c`/`logflume2.c`. Flagged here rather than changed anywhere.
- `ElemID` is declared `LLElem* ElemID(const char*)` here, matching
  `savechunks.c`; `loadmap.c`/`render3.c` use `void*`. No codegen difference at
  these call sites (the result is stored straight into a pointer field).
- `LLElem` comes from `legoland.h` (name at +0x00) — a local redefinition is a
  C2011 error, so do not add one.

## Residual for `LoadScriptEvent` (HANDOFF §6B triage)

`ours = 124i/323B`, `orig = 124i/319B`, **mismatch 26, first diverging index 95**.
Every instruction of the original is present and every block is byte-correct;
only the ORDER of three cold blocks differs:

```
original    readfail 0x46c8ae | terminator 0x46c8cf | err-after-name 0x46c8ea
            | err-after-text 0x46c900 | `head = 0` 0x46c916
this body   readfail | terminator | `head = 0` | err-after-name | err-after-text
```

Class: **structural** (a pure cold-block permutation — not allocation, not
frame, not scheduling; `strict`, register-blind and offset-blind all agree, and
no free `volatile` read is relevant because no value is wrong). Reachable only
with a block-ordering lever that neither of the two coupled behaviours above
lets you spell today; the full 45-spelling search is written up in the block
comment above the function so the next lane does not repeat it.

---

# workorder3.c

## fable-b lane `workorder3.c` — work orders and the print list

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

---

# ridemisc.c

## Lane `fable-b-ridemisc` — small ride helpers (2026-09-05)

File: `LEGOLAND/ridemisc.c` (new). Six functions from the unmatched-callee
frontier. `tools/audit.py LEGOLAND/ridemisc.c` ends **PASS**; `/W3` clean.

| address | name | insns | bytes | audit | mismatch | marker |
| --- | --- | --- | --- | --- | --- | --- |
| 0x0043d7c0 | `MechanicsHut_EvictRiders` | 64/64 | 182/182 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x0043d7c0` |
| 0x004333e0 | `JcBoat_Advance` | 69/69 | 217/217 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x004333e0` |
| 0x0044e890 | `RandomFavouriteFood` | 70/70 | 137/137 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x0044e890` |
| 0x0044e790 | `RandomFavouriteRide` | 73/73 | 145/145 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x0044e790` |
| 0x004048b0 | `Copters_SetFull` | 75/75 | 227/227 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x004048b0` |
| 0x0042cf70 | `EarthSlide_LaunchCar` | 75/75 | 201/201 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x0042cf70` |

Six of six exact. No renames — every name was already declared by the file
that calls it (`ridecb9.c`, `junglecruise.c`, `rides.c`, `mechrides.c`,
`ridecb1.c`). All six end in a real `ret` and none is recursive, so all six
are promotable as committed.

---

## New codegen levers (each with its measurement)

- **An aggregate local blocks the reuse of a DEAD PARAMETER's home slot.**
  `MechanicsHut_EvictRiders` spills one of its two derived coordinates. As two
  plain `int` locals VC6 homes the spilled one in the `cls` argument slot —
  which is free the moment `cls` is copied into `ebp` — and the frame collapses
  to `push ecx`; as one `Pos` (or an `int[2]`) it takes a fresh slot and the
  frame is the original's `sub esp,8`. **11 of 64, and the ONLY residual.**
  Declaration order of the aggregate against the two pointer locals is inert
  (three permutations byte-identical), and a `for` header or an inner-scope
  `next` changes nothing. This is the counterpart of the recorded "two argument
  slots reused as locals keep an 8-byte frame with nothing in the source asking
  for it": when the original's frame is BIGGER than yours and the difference is
  exactly one spilled scalar, the source had that scalar inside an aggregate.

- **An ASCENDING array walk anchors its induction variable on the LAST field
  it touches; a DESCENDING one on the first — and the spelling has to follow
  the anchor.** `JcBoat_Advance` has three fills of the same 8-byte
  `{int x; int y}` element, two up and one down:
  - up: the original's pointer starts at `&wob[i].y` and stores through
    `[eax-4]` / `[eax]`. Only **two per-field assignments** produce that.
    A whole-struct assignment `b->wob[i] = b->wob[64];`, a pointer walk
    `*p++ = ...`, and a y-then-x scalar pair all anchor on `.x` and cost 3
    instructions' worth of operands each (6 of 69 total).
  - down: the original addresses `[eax]` / `[eax+4]`, which is what the
    **whole-struct assignment** gives; the scalar pair is wrong there.
  So do not carry a spelling from one loop to its neighbour — measure per loop,
  by walk direction. The same bias appears from the READ side in
  `EarthSlide_LaunchCar` (below).

- **A read cursor into a const table needs to be a POINTER, and sometimes a
  BIASED one.** `EarthSlide_LaunchCar` walks a 4-entry `{int dx; int dy}` table
  in step with a list walk.
  - `kSlideQueueSpots[i]` with an `int i` is one instruction and two bytes
    SHORT (74/199 vs 75/201): VC6 folds the strength-reduced address into both
    loads, never materialises the cursor, and the `mov eax,ecx` that preserves
    the computed `target.y` disappears with it. A `const Pos*` walked with
    `sp++` restores it — **9 of 75.**
  - The last 2 are the +4 anchor. NOTHING done to a `const Pos*` reaches it:
    `sp->x`/`sp->y`, `sp[0].x`, `Pos s = *sp;`, a named local for either field,
    `(sp++)->y`, `for (...; q = q->next, sp++)`, `sp++` before the list step, a
    `do/while`, a flat `const int[8]` view read as `ip[0]`/`ip[1]`, and
    `kSlideQueueSpots + 1` with `sp[-1].x`/`sp[-1].y` are all 2 or worse.
    **`const int* spot = &kSlideQueueSpots[0].y;` read as `spot[-1]` /
    `spot[0]` and stepped `spot += 2` is exact.** Same walk, byte for byte —
    the biased int cursor is simply how the anchor is spelled in C.

- **A duplicated cursor-advance block is worth more than the `&&` that would
  merge it.** Both favourite-pickers fail a candidate on two independent tests
  and then advance the wrap-around scan. Written as one `&&` guard VC6 emits a
  SINGLE shared miss block reached by `jne`, and the function comes out 7
  instructions short (**19 of 70**). Two guards, each ending in `continue`,
  give the original's two inline copies — and the extra references to `n` and
  `start` that the second copy creates are also what rank them into edi/ebx
  instead of ebx/ebp. An `else if` nest is byte-identical to the `continue`
  form. (The recorded exile rule from the other side: there, merging guards
  pulled a shared block out; here, splitting them duplicates it inline.)

- **Naming the intermediate POINTER, not the value, advances the scratch
  rotation.** `RandomFavouriteRide` compares the class kind twice, so VC6 CSEs
  the 16-bit load into `cx`. With `e->data->type` spelled out at both compares
  the body is byte-exact except that the `tries--` temporary at the loop
  back-edge lands in edx where the original has ecx (strict 2, register-blind
  0). `d = e->data;` first closes it. A `short` or `unsigned short` local for
  the VALUE does not (still 2); an `int` one is worse (5). A
  volatile-qualified load of the same pointer is byte-identical to the plain
  local — **on this body the free-volatile trick and the named pointer are the
  same lever.** Its twin, with only ONE compare and therefore no CSE, needs no
  local at all: the twins' residual is NOT shared here, which is the first
  counter-example to "twins share their residual index for index" in this
  tree. They share their SOURCE and diverge in one construct, and the
  construct is exactly where the register pressure differs.

- **`tries` before `start`.** In both pickers the two seeds must be assigned in
  that order; the other order moves the `mov ebx,esi` and costs 1 at index 11.
  Cheap to sweep, same family as the recorded "initialiser ORDER decides where
  a spill store lands".

- **Confirmations (no new information, recorded so they are not re-derived):**
  the operand order of a two-term sum is canonicalised and inert (`t.x + s.x`
  vs `s.x + t.x`, both fields, all four combinations byte-identical);
  `* 256` and `<< 8` are the same object; splitting `(a + b) << 8` into an
  assignment plus a `<<=` is inert; naming the path or the world `Pos` in a
  local is inert; `&b->path[0]` and `b->path` are inert.

---

## Data structures and rules recovered

### `MechanicsHut_EvictRiders` (0x0043d7c0) — shared by the hut and the shed

`ridecb9.c` had it as an unmatched extern with the note "the potting shed calls
it too". The body confirms the split and fills in the third argument's meaning:

```
MechanicsHut_EvictRiders(ObjDef* cls, BPosW key, int shed)
```

- `key` is the packed map square, passed BY VALUE as a two-byte aggregate. VC6
  extracts the halves with the `[esp+N]` / `[esp+N+1]` dword pair and masks
  (`and eax,0xff`), and the loop's identity test re-reads the parameter slot as
  a `word` — the `BPosW` union spelling, unchanged from `jcroute.c`.
- The blokes are dropped at the CLASS's door offset (`ObjDef+0x0c/+0x10`, the
  same pair `popup.c` and `logflume4.c` name) plus the square, in 24.8:
  `b->world.x = (cls->dx + key.b.x) << 8`.
- `shed` picks the long-term action only: **0x10 for the POTTING SHED, 0x11
  for the MECHANICS HUT** (`ridecb9.c` records the hut's stage-3 arm handing
  out 0x11, and the shed passes 1 here). It is the whole difference between
  the two classes' remove handlers.
- Bloke flags `0x28` (`8` = using this ride, `0x20` = hired) are dropped
  together, in one `and word [esi+0x62],0xffd7`.
- A staff member whose stage byte has already reached 0x64 — the "being
  fired" half of the hut's state machine — is unlinked but NOT freed, and gets
  job slot 0x64 instead of a new action. That is the same node leak
  `ridecb9.c` records at stage 0x67, reached from the other side.
- **Both arms call `RemoveBlokeFromList`, and the original emits the call
  TWICE with ONE shared argument-push pair** — the two arms clean 0xc and 8
  bytes respectively, so the pending `add esp` cannot cross the join and the
  source has the call in each arm. Read the argument clean-up, as
  `SchoolCarManoeuvreD` teaches.

### `JcBoat_Advance` (0x004333e0) — the jungle cruise's DOCKING sequence

`junglecruise.c` had `+0x3e4` as "squares left in this leg" and this function
as "finishes a leg". It is more specific than that: `JcBoat_Step` sets
state 0x10 and `leg = 3` the moment a boat reaches the station's route-end
square, and the four ticks that follow are the arrival animation, driven
entirely by rewriting the 80-entry wobble buffer:

| leg | what the tick does |
| --- | --- |
| 3 | animate in-1/out-4, then `wob[64..79] = wob[64]` — the boat stops dead half way across the last square |
| 2 | animate, `wob[0..63] = wob[64]` — already parked when playback starts — and aim one square further south |
| 1 | animate, aim south again, then `wob[79..73] = wob[72]` — the settle as it ties up |
| 0 | unlink and FREE the boat; return the NEXT one |

`from`/`to` are always 1 and 4 (in by the north side, out by the south): the
straight run down out of the station. `b->leg` is re-read at each test rather
than cached — the wobble writes may alias, and the original reloads it three
times, which is what the source's direct field reads give.

### `RandomFavouriteRide` / `RandomFavouriteFood` (0x0044e790 / 0x0044e890)

`rides.c` hands every new visitor three favourite rides and one favourite food
from these. Both scan the LLIDB linearly from a random start for an element
that is an ODF loaded on THIS level (`type_flags & 0x14 == 0x14`, i.e.
`0x10` ODF + `0x4` on-level) and of the right class kind — `ObjDef+0x20 == 5`
is the food/shop bucket, "not 0 and not 5" is a ride. The scan wraps and gives
up only when it returns to its starting index, which is the sole path that
returns 0.

**TWO ORIGINAL BUGS, both reproduced and commented at the site:**

1. `rand() & 0x1f` is plainly meant to pick the Nth match, but the accept arm
   never advances the cursor — the loop re-fetches the SAME element until the
   counter runs out, so the function always returns the FIRST match from the
   random start. The counter is pure noise.
2. When `rand() & 0x1f` is 0 (one time in 32) the loop never runs and the
   function returns the **uninitialised local** `e` — stack junk, which the
   caller stores as a favourite. That is the third epilogue,
   `mov eax,[esp+0x10]`.

### `Copters_SetFull` (0x004048b0) — the HELICOPTERS machine starts its run

Reached from two sites in the ride's own update, 0x00404b68 (the fill timer
expires) and 0x00404d5f (the seated count reaches the class's capacity at
`ObjDef+0x2e`); both clear the 0x4000 "still filling" bit themselves first.

- The record is `mechrides.c`'s `CoptersRec`. New fields: **+0x08 is a flags
  dword** (`1` = running, `0x4000` = filling) and **+0x0c a mode word** set
  to 2 here. `riders (+0x03) = seated (+0x02); seated = 0;` moves the boarding
  count over to the "aboard" count.
- The seat record's **+0x00 is a DWORD here** (`or dword ptr [eax+0x38],edi`)
  where `joust2.c`'s drawing view reads only its low byte — an extern/struct
  type divergence to leave alone, not to align.
- Per occupied copter: `+0x00 |= 1`, `+0x1d = 3` (flight stage), `+0x04 = 0`
  (the shared animation frame `joust2.c` names).
- **Only FIVE of the six seat records are touched**, and all three five-line
  runs enumerate them **1, 0, 2, 3, 4**. That is not an adjacent-store
  reversal — writing 0,1,2,3,4 emits 0,1,2,3,4 and costs 7 — the source really
  lists copter 1 first, three times over. (The ride's draw pass has its own
  fixed order too: `joust2.c` records 0, 2, 3, 4, 1.)
- The two sounds are `g_copters_fx[0]` and `[1]` (the FX table at 0x004b4140
  that `Copters_Create` loads). Slot 0 is the one-shot; slot 1 is the LOOP,
  started, immediately paused, and given a **0xb54 ms** completion callback
  (0x004048a0, first named here as `Copters_ResumeSFX`) that resumes it — so
  the loop is delayed by exactly the length of the start-up sound. Four cdecl
  calls, one merged `add esp,0x30`.
- The sound source is the record's map square (kind 2) with **+0x04 left
  uninitialised**, the same habit `money.c` and `ridecb3.c` record.

### `EarthSlide_LaunchCar` (0x0042cf70) — the queue shuffles up

`ridecb1.c` had the one-line summary ("pop the front queuer, advance its
action and re-shuffle the queue"); this is what the shuffle is.

- The front queuer loses the **0x40 "queued"** bloke flag (the bit
  `EarthSlide_JoinQueue` sets) and his stage byte steps on, then he is popped.
- **0x0042d040, first named here as `EarthSlide_PopQueue`:** unlink the head
  node from `SlideRec+0x1c`, clear the queue TAIL at **`SlideRec+0x20`** if it
  pointed at the popped node, and decrement the count byte at +0x18. `+0x20`
  is one field past the end of `ridecb1.c`'s `SlideRec`.
- The four queue standing spots are a const table at **0x004b65c0**:
  `(0,4) (0,3) (0,2) (0,1)` — four cells due south of the ride's base square
  (`g_slide_item+0x0c/+0x10`), closest LAST. Everyone still in the chain is
  re-aimed at the spot one place further forward and re-planned with
  `CalcMoveLine` + `NewDirForAction`, the same four-line idiom
  `MechanicsHut_Tick` uses.
- **LATENT ORIGINAL BUG, reproduced as written:** the shuffle walk is bounded
  by the QUEUE, not by the table. It indexes the 4-entry table by the queuer's
  position in the list, so a fifth queuer would read the string constants that
  follow the table at 0x004b65e0. The queue-length byte at +0x18 is the only
  thing that keeps it from happening.

---

## Callees named for the first time

| address | name | evidence |
| --- | --- | --- |
| 0x0042d040 | `EarthSlide_PopQueue` | unlinks `SlideRec+0x1c`'s head, fixes the tail at +0x20, decrements +0x18 |
| 0x004048a0 | `Copters_ResumeSFX` | one-line forward to `ResumeSinglyPausedSample` (0x00492910); installed here as the 0xb54 ms SFX completion callback |

## Extern-type divergences recorded (do NOT align)

- `MechanicsHut_EvictRiders` — `ridecb9.c` declares its second parameter
  `unsigned int bp`; the DEFINITION needs the two-byte `BPosW` **by value**,
  which is what emits the `[esp+N]`/`[esp+N+1]` half extraction and the `word`
  compare in the loop. Both spellings are correct on their own side.
- `CopterSeat +0x00` — a DWORD in `ridemisc.c` (`or dword ptr`), a
  `unsigned char` in `joust2.c`'s drawing view.
- `LLIDB_GetElement` — declared `void` returning here (the result is unused),
  `int` in `memdb.c`/`misc3.c`. Inert on this body but recorded.

---

# sysmisc.c

## fable-b lane notes — `LEGOLAND/sysmisc.c`

Seven functions from five subsystems (display, resources, audio, input, 3D
people), ~600 instructions. **Five exact, two WIP at 91% and 89%**; `audit.py`
PASS, `/W3` clean. Written 2026-09-05 alongside the integrating session.

| address | function | insns | audit | residual |
| --- | --- | --- | --- | --- |
| 0x004966a0 | `UpdateSampleSource` | 66 | WIP | 6 — case-3 load scheduling |
| 0x00473970 | `CreateMouseDevice` | 66 | **[OK]** | — |
| 0x00463ef0 | `SetScreenDisplayMode` | 72 | **[OK]** | — (first compile) |
| 0x004401b0 | `UpdatePersonPos` | 74 | WIP | 8 — register allocation |
| 0x004515e0 | `RES_EnsureMounted` | 92 | **[OK]** | — |
| 0x004661d0 | `FlipPrimary` | 111 | **[OK]** | — (first compile) |
| 0x0047c6a0 | `LLIDB_UnLoadLLSData` | 119 | **[OK]** | — (first compile) |

Both WIPs are **byte-exact in length or one byte off**, so every encoding is
right and only ordering/allocation is left.

---

## Levers, with evidence

- **A shared tail that RELOADS two locals from the frame proves the arms all
  had to leave them in memory — and the way to make VC6 do that is a volatile
  STORE through a cast in every arm.** `UpdateSampleSource` ends in one block
  that does `mov edx,[esp+8] / mov eax,[esp+4] / push / push / push / call`.
  Written naturally (plain `p.x = …; p.y = …;` in each switch arm) VC6 keeps
  the values in registers, DUPLICATES that ten-instruction tail into every arm
  and emits 85 instructions for a 66-instruction function. Writing the arms'
  stores as `*(int volatile*)&p.x = e;` makes all four arms end identically,
  VC6 cross-jumps them into ONE tail, and the body is 66/66 with the arm
  layout, the case-1→case-3 cross-jump and the `ja` default target all exact.
  **Three cheaper things do NOT work**, each measured: passing the `Pos` BY
  VALUE to the tail callee (VC6 still forwards the stores, 85), reading the
  values back through a `static __inline` helper that takes a `Pos*` (VC6
  inlines the helper and forwards, 85), and a `goto`/label join with an
  explicit `default:` (85). Only forcing the stores works.

- **An 8-byte struct assignment and two field assignments are different
  objects even when the fields are adjacent.** `UpdateSampleSource` case 1
  reads a `Person3D`'s screen position at +0x1c/+0x20. `p.x = m->sx; p.y =
  m->sy;` makes VC6 RELOAD `b->person` after the first store (the store to an
  address-taken local may alias the pointer), which is one extra instruction
  and shifts the block's whole register rotation. Typing the pair as a `Pos`
  at +0x1c and writing `p = b->person->screen;` reads both fields before the
  first store and matches index for index. Hoisting the pointer into a local
  first (`Person3D* m = b->person;`) does not help — the reload is of
  `b->person`, not of `m`.

- **A free volatile READ is emitted FIRST in its basic block, not at its
  statement position.** This is the whole residual of `UpdateSampleSource`.
  Case 3 needs `s->src.pos.y` loaded on the near side of the volatile store to
  `p.x` (without the volatile read the load sinks below the store and costs
  12); the volatile read achieves that, but it also pins the load as the
  block's FIRST instruction, where the original has it third
  (`scroll_x / pos.x / pos.y`). Statement position is inert: the read emits
  first whether it is the block's first statement, sits after the `p.x`
  statement, or is spelled through an extra local (four spellings, all 6).
  Adding volatile reads on `g_scroll_x` or on `s->src.pos.x` to order the
  block instead **breaks the tail merge** (72 instructions), and so does the
  same on `g_scroll_x` alone — only a volatile read of `pos.y` is safe.

- **`test byte ptr [mem],1` on a byte that is field +3 of a word comes from
  `& 0x100` on the `unsigned short`, not `& 1` on the byte.** In
  `UpdatePersonPos`, `unsigned char flags63; … if (!(b->flags63 & 1))` gives
  `mov al,[esi+0x63] / … / test al,1` — an extra instruction, and the load
  also slides ahead of the function's pending `add esp,0x20`, shifting every
  later index. Declaring the field as `unsigned short flags62` at +0x62 and
  testing `(b->flags62 & 0x100) == 0` narrows to the original's single
  `test byte ptr [esi+0x63],1`. Worth 74 instructions instead of 75 and
  mismatch 22 → 10 in one edit. (Same family as DECOMP's "u16 flags `|= 0x100`
  narrows to a byte OR"; this is the read side of it.)

- **Reading two fields into locals BEFORE an intervening call is what puts
  them in callee-saved registers.** `UpdatePersonPos` loads `b->y` then `b->x`
  into ebx/ebp before `GetTileDimensions`. Spelled inline in the expressions
  that follow the call, VC6 loads them after it, drops from four callee-saved
  pushes to three and re-allocates the whole function (58 of 74 wrong). Two
  plain locals assigned before the call: 22, then 8 after the flag fix. The
  order matters too — `by` first, then `bx`.

- **VC6 homes an out-pointer local PAIR in the incoming parameter slots.**
  `UpdatePersonPos` is `sub esp,8` — eight bytes, which is only the
  address-taken `Pos`. Its `int tw, th`, address-taken by
  `GetTileDimensions(&tw,&th)`, live at `[R+4]` and `[R+8]`: the caller's own
  argument slots, free because both parameters were root-copied into esi/edi
  at entry and never re-read. Read `[esp+N]` by push depth or this region
  looks like a wild write into the caller's frame. (Confirms DECOMP's
  "uninitialised out-pointer locals are homed in dead argument slots" for
  written-then-read locals as well.)

- **A `sete al` + `test al,1` guard is `((x == 0) & 1) == 0`, and the `== 0`
  half is a LAYOUT lever.** `CreateMouseDevice` tests DIDEVCAPS::dwFlags with
  `xor eax,eax / test edx,edx / sete al / test al,1 / jne`. A plain
  `dwFlags != 0` gives a bare `test edx,edx / je` and is three instructions
  short. The `& 1` is what turns the byte test into a bit-0 test. The outer
  `== 0` (success arm inline, failure jumping to the ONE trailing `return 0`)
  is what keeps the failure block at the END: written as
  `if ((… ) & 1) goto fail;` VC6 inverts the branch and parks the failure
  block in the middle (12 of 66). `int t = (dwFlags == 0); if (!(t & 1))` is
  byte-identical to the `== 0` form.

- **Two near-identical arms of an if/else were written out IN FULL, error
  handler and tail included — the block layout is the proof.**
  `RES_EnsureMounted` is two CD-nag loops, one per branch of its argument.
  Writing the retry-box cancel handler once, or the trailing
  `if (minimised) { restore } return 1;` once after the if/else, puts the
  shared block BETWEEN the arms and loses each arm's own inline
  `test edi,edi / je` (90 of 92 instructions). Duplicating **both** into both
  arms is 92/92 on the next compile: VC6 cross-jumps the two cancel bodies
  into one copy parked after the first loop, and the two restore bodies into
  one copy parked after the second, while each arm keeps its own guard. Read
  the layout: a shared block sandwiched between two arms was written twice; a
  shared block after the second arm reached by a `jmp` from the first was too.

- **`sub esp,N` at the top with `mov dword ptr [esp+k], imm` stores and NO
  matching `add esp` is a local aggregate initialiser, not an argument list.**
  `FlipPrimary` opens with four such stores and `call LLSAuto` — but `LLSAuto`
  (0x0047d630, layervis.c) takes no arguments. They are
  `WinRect dst = { 0, 0, 640, 480 };`, and the same sixteen bytes are handed to
  `OffsetRect` and to `Blt` later. The missing cleanup is the tell.

- **`mov esi,[__imp__X] / call esi` needs no source construct — it is a plain
  import call inside a loop.** `timeGetTime` in `FlipPrimary` and
  `MessageBoxA` in `RES_EnsureMounted` both come out that way from ordinary
  `__declspec(dllimport) __stdcall` declarations; VC6 hoists the IAT load out
  of the loop by itself and the third, post-loop call reuses the register.

---

## Residuals (§6B triage)

- **`UpdateSampleSource` — 6 of 66, byte-exact (175/175 B).** First divergence
  at index 46. The three loads of case 3 come out `pos.y / scroll_x / pos.x`
  where the original has `scroll_x / pos.x / pos.y`; the following
  `sar/sub/store` differ only in register naming, which follows. Cases 0, 1
  and 2 and the whole shared tail are exact. **Scheduling**, forced by the free
  volatile read (see the lever above); ~20 spellings tried.
- **`UpdatePersonPos` — 8 of 74, 212 B vs 213 B.** First divergence at index
  20. (a) The original builds the isometric y-sum with `lea ecx,[ebx+ebp]`, a
  THIRD register; ours coalesces it into the x local's register as
  `add ebp,ebx` — the single byte of difference. (b) The x-scroll block uses
  ebx/eax where ours uses edx/ecx. `strict >> register-blind`, i.e.
  **allocation**. Everything tried and rejected: sum/diff temporaries in both
  orders, both statement orders, both `lea` operand orders, `tw * (…)` operand
  order, a `Pos` struct copy of the source pair, a dead self-assignment, and
  free volatile reads at six sites (`tw`, `th`, `pos.x`, `pos.y`,
  `g_screencfg->origin_x`, `b->dir`). The one that helps is the free volatile
  read on `pos.x` in the x-scroll statement: it closes the y-scroll block, 10
  → 8. Nothing reaches the `lea`.

---

## Mechanics recovered

- **`SoundSource::kind` (audio3.c's guess, now confirmed).** 0 = none →
  `ClearSampleSource`; 1 = bloke → its `Person3D`'s SCREEN position
  (+0x1c/+0x20), not its map position; 2 = map ref → `GetTileCentre` of
  `src.pos`; 3 = level xy → `src.pos` minus the 24.8 scroll (`>> 8`). All four
  end in **0x004965a0 `SetSampleScreenPos(Sample*, int x, int y)`** (first
  named here), which subtracts half the screen size read through 0x004bcbf4
  and pushes the pan/volume into the DirectSound buffer.
- **`CreateMouseDevice`.** `CreateDevice(GUID_SysMouse)` →
  `SetCooperativeLevel(hwnd, EXCLUSIVE|FOREGROUND = 5)` →
  `SetDataFormat(c_dfDIMouse)` → `GetProperty(DIPROP_GRANULARITY = MAKEDIPROP(3))`
  of `DIMOFS_Z` (dwObj 8, `DIPH_BYOFFSET`) into `g_wheel_granularity`
  (0x004bad54), which is therefore the wheel notch size (normally 120) →
  `GetCapabilities` with the DirectX 3 `DIDEVCAPS` size 0x2c, and a
  `dwFlags == 0` = "no device" test → `GetDeviceStatus(GUID_SysMouse) == DI_OK`
  as the return value.
- **`SetScreenDisplayMode`.** Full screen only (`g_windowed == 0`): tries the
  config's w×h at 16 bpp, then at 8 bpp, and returns 0 if both fail. Then
  `GetDisplayMode` into a 0x6c-byte `DDSURFACEDESC` and classifies
  `ddpfPixelFormat`: **`g_screen_depth` (0x00668088) is 0 for 8 bpp, 1 for
  16 bpp 555 and 2 for 16 bpp 565** — the 565 test is `dwGBitMask == 0x7e0`,
  spelled `(mask == 0x7e0) + 1`. Any other bit count returns 0.
- **`UpdatePersonPos`** (the whole isometric projection, in one place):
  `SetPersonDirection(p, b->dir)`, then
  `sx = ((b->x - b->y) * tilew) >> 9`, `sy = ((b->y + b->x) * tileh) >> 9`
  from `GetTileDimensions`, minus `Get_XScroll()` / `Get_YScroll()` (both
  `short`, sign-extended), then **`Person3D::depth` (+0x54) is that y BEFORE
  the origin adjust**, then `+ g_screencfg->origin_x` and
  `+ g_screencfg->origin_y - (b->height >> 1)` (the sprite is lifted by half
  its height), then `AdjustBlokePosition` (a fixed −0x4b/−0x4d fudge) and
  `SetPersonPosition`. Finally the animation frame is copied from the bloke's
  +0x74 **unless bit 0x100 of the +0x62 flags word is set**.
  Bloke fields recovered: +0x68/+0x6c = map x/y in 24.8, +0x70 = u16 sprite
  height, +0x72 = u8 heading, +0x74 = u8 animation frame, +0x62 flags bit
  0x100 = "keep the current frame".
- **`RES_EnsureMounted`.** Blocks until the CD is present, nagging with a
  RETRY/CANCEL box (`0x50015` = RETRYCANCEL|ICONHAND|SETFOREGROUND|TOPMOST,
  caption "CD Missing"), minimising the game the first time round and
  restoring it on the way out of either exit. Two probers:
  **0x00450f30 `RES_FindVolumeOnAnyDrive`** (walks the `GetLogicalDrives`
  mask, logs "Checking all drives (Mask = %d)") and **0x004510e0
  `RES_FindVolumeOnResPath`** (the drive named by `g_res_path` @0x00813b04);
  the message differs accordingly ("into the CD drive" vs "into drive %s").
- **`FlipPrimary`** (installed as `g_present` @0x004b9ca4 in full screen):
  `LLSAuto()`, then the game cursor sprite (`g_current_pointer` @0x00668148)
  stamped at `g_gfx_point` inside a
  `PushRenderingStatusAndLockVideoSurface` / `PopRenderingStatus` pair when
  `g_screencfg->cursor` (+0x1e) is set, then a **28 ms minimum frame period**
  spin on `timeGetTime`, then `GetClientRect` + `ClientToScreen` +
  `OffsetRect` to move a fixed 640×480 rect to the window's screen position
  and `Blt` `g_surface_78` (0x00668078) onto the primary with `DDBLT_WAIT`;
  on `DDERR_SURFACELOST` it `Restore`s the primary and blits once more, and
  returns 0 if the blit still fails. The tail is the frame accounting:
  a frame counter, a one-second FPS bucket and the per-frame tick delta.
- **`LLIDB_UnLoadLLSData`** — what an LLIDB element of type 0x10 / 0x1010
  actually parses into: **an ObjectClassList node** (the 0x00669240 chain
  castleobj.c calls `ObjectClassList`), unlinked through +0x00. Teardown
  order: per-class destructor `(*+0xac)(*+0xc4)`; `UnLoadObjectLibrary` when
  `+0x1c & 0x10000`; unlink; three sprites at +0x64/+0x68/+0x6c, each
  `LLSStop`ping the LLS at `sprite->image->lls` first **when the image's kind
  at +0x14 is 2 or 3** (the two animated kinds) and then `KillSprite`; a
  nested LLIDB element at +0x70 through `LLIDB_UnLoadData`; three owned heap
  blocks at +0x78/+0x7c/+0x80; the record itself; and finally `e->data = 0`.

---

## Original bugs reproduced (commented at the site)

- **`UpdateSampleSource` has no `default:`.** A `SoundSource::kind` outside
  0..3 falls straight through to `SetSampleScreenPos(s, p.x, p.y)` with both
  frame slots **uninitialised** — whatever the previous frame left there. The
  sourcing entry points only ever store 0..3, so it never fires.
- **`RES_EnsureMounted` never uses its `volume` argument's VALUE.** The
  parameter only selects which prober runs; both branches probe the
  hard-coded volume name `"LEGOLAND"` (0x004b86d0). res.c's only caller passes
  0, so the drive-specific branch is the one that runs in the shipped game.

---

## Extern-type and naming divergences (do NOT "align" these)

- **`UpdateSampleSource` returns `int`, not `void`.** audio2.c and audio3.c
  both declare it `void`; the body has three inline `xor eax,eax` guard
  returns and tails into `return SetSampleScreenPos(…)`. Defined `int` here.
  The two callers' `void` declarations are correct AS CALLER-SIDE LEVERS and
  must stay.
- **`UpdatePersonPos`** is declared in blokemisc.c as
  `void UpdatePersonPos(void* model, Bloke* b)`; defined here as
  `void UpdatePersonPos(BlokePerson* p, WalkBloke* b)`. Same two arguments,
  different pointer types.
- **`ClearSampleSource` (0x00496660)** is declared `int`-returning here
  (`case 0` tails into it and its result is this function's result);
  audio3.c declares it `void`.
- **0x004bcbf4 has four names in the tree already** — legoland.h `Map* g_map`
  (a DIFFERENT view: w/h at +0x14/+0x16), screen.c `ScreenCfg* g_screencfg`,
  gpu.c `Screen* g_screen`, anim2.c/goldrush.c `MapHdr* g_map`. This file uses
  ONE `ScreenCfg* g_screencfg` carrying every field it needs: w/h (+0x00/+0x02),
  `cursor` (+0x1e), `origin_x`/`origin_y` (+0x20/+0x22). Note legoland.h's
  `g_map` is a different type at the same address, so the name `g_map` is not
  available inside this file.
- **0x007cacd4 is named twice.** screen.c calls it `g_init_flag` because
  `InitScreen` zeroes it; `FlipPrimary` increments it on every presented
  frame, so this file names it **`g_frame_count`**. Recorded here rather than
  renaming screen.c.

## Callees and globals named for the first time

| address | name | note |
| --- | --- | --- |
| 0x004965a0 | `SetSampleScreenPos(Sample*, int, int)` | positional-audio pan/volume |
| 0x00450f30 | `RES_FindVolumeOnAnyDrive(const char*)` | walks the GetLogicalDrives mask |
| 0x004510e0 | `RES_FindVolumeOnResPath(const char*)` | probes the `g_res_path` drive |
| 0x0047fe70 | `WNDENV_Minimise()` | `ShowWindow(hwnd, SW_MINIMIZE)` |
| 0x0047fe80 | `WNDENV_Restore()` | `ShowWindow(hwnd, SW_RESTORE)` |
| 0x00668200 | `g_flip_time` | `timeGetTime` at the last blit |
| 0x006681f0 | `g_fps_base` | start of the current FPS second |
| 0x006681f4 | `g_last_frame` | `timeGetTime` at the last flip |
| 0x006681f8 | `g_fps_frames` | frames since `g_fps_base` |
| 0x007fea48 | `g_fps` | last completed second's frame count |
| 0x00669240 | `g_objclass_head` | = castleobj.c's `ObjectClassList` |
| 0x004ac0a0 | `GUID_SysMouse` | 0x004ac090 is `GUID_SysKeyboard` |
| 0x004ab578 | `c_dfDIMouse` | 0x004ab560 is `c_dfDIKeyboard`; 0x18 apart = `sizeof(DIDATAFORMAT)` |
| 0x004ab1f8 | `__imp__timeGetTime` | winmm |
| 0x004ab2a4 | `__imp__MessageBoxA@16` | |
| 0x004ab2ec | `__imp__GetClientRect@8` | |
| 0x004ab2dc | `__imp__ClientToScreen@8` | |
| 0x004ab29c | `__imp__OffsetRect@12` | |

## Free follow-up

**`CreateKeyboardDevice` (0x004738b0, 51 instructions)** is `CreateMouseDevice`
with the granularity block deleted and the keyboard GUID/data format — the
exact same `((caps.dwFlags == 0) & 1) == 0` guard and the same `goto fail`
shape. It is declared `extern` by input2.c and defined nowhere. It was left
alone only because it is outside this lane's table; it should close on the
first compile.
