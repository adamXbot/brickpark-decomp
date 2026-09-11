# Scope PORT-A8 — the generator and tools lane (2026-09-12)

Branch `scope/PORT-A8`, from the PORT-B9 merge (`56fa34d9`). No game code: this
lane owns `portable/tools/*.py`, `portable/cmake/headless.cmake` and
`portable/src/headless/**`. Nothing in `LEGOLAND/*.c` was touched, so
`audit.py`/`relocs.py` have nothing to say about it and `verify.py` was not run.

The theme, stated once because all five deliverables are instances of it: **a tool
that is almost right is worse than one that is obviously wrong, because its output
gets believed.** `cdecl.py` confidently gave `MeshDesc` the wrong size for three
lanes. The manifest confidently listed eleven names and no reasons. The B9-4 sweep,
written the obvious way, confidently returned 2,585 hits and then 1.

| deliverable | state |
| --- | --- |
| 1. `cdecl.py` declarator parsing (M8-3) | **done**, `MeshDesc` 0x1c, proved below |
| 2. left-raw words + 11 conflicting signatures attributed | **done**, 0 rows without a verdict |
| 3. `slot_sweep.py` + `extern_sweep.py` promoted, extern sweep a ctest | **done**, native ctest 11 -> 14 |
| 4. `name_trap.py`: table-index and memory-access kinds | **done**, names all 5 B9-4 literals |
| 5. notes, README, census | this file |

Gates, from CLEAN build directories: native `ctest` **14/14**, wasm `ctest`
**20/20**, `legoland_linkcheck` links and runs on both, every wasm target builds
(`legoland_headless`, `_debug`, `legoland_tests`, `legoland_pathtest`,
`legoland_shimtest`, `legoland_browser`, `legoland_cbtypes`).

---

## 1. `cdecl.py`: a declarator is recursive (M8-3)

### The defect

`int (*pairs)[2]` is a POINTER TO an array of two ints — four bytes. The parser
read it as `int *pairs[2]`, an ARRAY OF two pointers — eight. So `MeshDesc`
(schoolcar3.c:277) came out **0x28 instead of 0x1c**, every field after `pairs`
moved, and three pointer words were invented out of the floats past the end of
the record. PORT-M8 §4f found it and filed it as M8-3 with a probe script.

It was not a missing case. `_declarator` returned `(pointer depth, name, array
count, is_function)`, a flat shape that cannot express a declarator at all, and it
flattened the parenthesised form by taking `max(nptr, inner_ptr)` and then letting
the `[2]` suffix land on the OUTER count. Any fix that kept the shape would have
been a special case waiting for the next spelling.

### The fix

Parse the declarator into a tree and evaluate it by the language's own rule, which
is outside-in:

```
D = * D1         -> type(D1, pointer to T)      D = D1(params) -> function
D = D1 [N]       -> type(D1, array N of T)      D = ( D1 )     -> type(D1, T)
D = identifier   -> the name, with type T
```

The identifier's type is whatever is left when the walk reaches it, and that is
the entire distinction: in `int (*p)[2]` the last derivation applied is `ptr`, in
`int *p[2]` it is `arr`. On ILP32 every pointer is four bytes, so "pointer to T"
collapses to `PTR` and the target type is never needed — the tree only has to get
the ORDER right, which is why the evaluator is a dozen lines and carries just
`(core, dims)`.

Two supporting changes: a `FUNC` sentinel type, which is what lets the evaluator
tell `int f(void)` (not an object) from `int (*f)(void)` (four bytes) and keeps
`int *f(void)` — a function returning a pointer — out of the object list; and
`_dims` becomes `_dim`, one dimension at a time, because a product cannot see
which level of the declarator a suffix belongs to.

### The proof

**22 new `--selftest` checks**, sizes AND pointer offsets. Offsets are tested
because an invented pointer word is the worse half of this bug: `gen_link.py`
would re-point a word the image holds a float in.

| shape | was | is |
| --- | --- | --- |
| `int (*pairs)[2]` | 8 | **4** |
| `int (*tris)[3]` | 12 | **4** |
| `char* (*strs)[8]` | 32 | **4** |
| `int (*p)[2][3]` | 24 | **4** |
| `int (**r)[2]` | 8 | **4** |
| `Pos (*grid)[4]` | 16 | **4** |
| `int (*p[2])[3]` | 8 | 8 (already right, kept) |
| `int *p[4]` | 16 | 16 (kept) |
| `int (*cb)(int)` | 4 | 4 (kept) |
| `int (*cb[5])(int)` | 20 | 20 (kept) |
| `int *f(void)`, `int (*f(int))(void)` | not objects | not objects (kept) |

Each appears both as a struct member and as an object's own declarator, because
the two go through different call sites (`_aggregate_body` and `_declaration`).

**Tree-wide, exactly one declaration changes.** `port-a8-ptrcensus.py` prints
extent + pointer offsets for every object declaration in `LEGOLAND/*.c` and
`*.h` under the old parser and the new one. Of **4,816 lines, one differs**:

```
- 0x004b5f60 g_track_mesh schoolcar3.c:287 ext=40 ptr=[0x10,0x14,0x18,0x1c,0x20,0x24]
+ 0x004b5f60 g_track_mesh schoolcar3.c:287 ext=28 ptr=[0x10,0x14,0x18]
```

`unref2.c`'s identical copy of `MeshDesc` has no object declared at an address, so
it does not appear; `blokemisc.c:33`'s `extern void (*g_lowlevel_ai[])(Bloke*)` —
an array of function pointers with an unspecified bound — still yields no extent,
as before.

**`gen/extents.md`** changes in that row and its two counters, and nowhere else:

```
- | 0x004b5f60 | g_4b5f60, g_track_mesh | MeshDesc ... -> 40 (0x28) | 32 | merged | g_support_shadow_templates+0x20 (tris[1]) |
- 43 objects would have been split; 43 are merged here (5 hand, 38 new)
+ 42 objects would have been split; 42 are merged here (5 hand, 37 new)
- | 0x004b5f60 | 40 | g_track_mesh MeshDesc = 40 ... |
+ | 0x004b5f60 | 28 | g_track_mesh MeshDesc = 28 ... |
```

**`gen/aliases.c` is byte-identical** on both builds.

**`globals.c` is identical where it matters, and the one change is the one asked
for.** A line diff cannot show this, because re-splitting a block moves words to a
different array without changing any of them, so `port-a8-bytesproof.py` parses
both files into `{absolute VA -> initialiser, as written}` and diffs that:

| build | elements | addresses only in old | only in new | values changed |
| --- | --- | --- | --- | --- |
| wasm (`--ilp32`, re-pointing ON) | 22,498 | 0 | 0 | **0** |
| native 64-bit | 86,596 | 0 | 0 | **0** |

The structural change, which is the "split or merged" list deliverable 1 asks for
and is **one object**: the 496-byte `g_4b5f60` at 0x004b5f60 becomes 32 bytes, and
`g_support_shadow_templates` becomes its own 464-byte object at 0x004b5f80 instead
of an offset alias at `g_4b5f60+32`. Same bytes, same addresses, re-split at the
boundary the real 0x1c struct implies — +0x20 was never inside `MeshDesc`. The
direction is the expected one: a wrong extent had MERGED a neighbour in, and the
fix un-merges it.

### What it recovered

Pointer claims are corroborated per array element, so the two invented words
holding `0x3f800000` (= 1.0f, the first floats of `g_support_shadow_templates`)
condemned the whole group — and **the three real pointers at +0x10/+0x14/+0x18
were discarded with them.** So the counters move in the direction that looks wrong
and is right:

| manifest line | before | after |
| --- | --- | --- |
| globals defined | 2043 | 2044 |
| objects merged from interior-aliased names | 43 | 42 |
| declared pointer words | 7538 | **7541** |
| pointer claims the image rejects | 7 words / 2 declarations | **1 / 1** |
| words re-pointed at a symbol address | 299 | 299 |
| pointer words re-pointed INTO a block | 789 | 789 |
| pointer words left raw | 1 | 1 |
| **raw pointer words (the gate)** | 0 | **0** |

The re-pointing counters do not move because those three words were already
resolved by the exact-symbol path — which is why PORT-M8 could see
`&g_track_verts`, `&g_mesh_pairs`, `&g_mesh_tris` in `globals.c` while the
declared-pointer path was throwing the same three away.

---

## 2. The manifest attributes its own findings

### 2a. The 11 conflicting wasm signatures: all game-side, two clusters

`collect_wasm_sigs` votes and returns the winner, which is all the generator needs
to EMIT; it threw away the vote, so the manifest could only print names. Two new
helpers in `linkreport.py`:

* `wasm_sig_conflict_detail(objs)` — per conflicting name, `{signature: [files]}`
  plus the signature the BODY has and the file that defines it.
* `extern_decl_sites(names)` — every `extern` declaration of a name, with its
  line. This is deliberately the opposite of `scan_sources`, which keeps one
  address per name because that is what sizing a global needs; a declaration
  DEFECT needs every file that spells it, and needs no address comment.

All eleven are DEFINED in the objects, so the body settles it, and all eleven are
`LEGOLAND/*.c` declarations — **nothing here is generator-side and nothing is
fixable here.**

| symbol | the body has | the disagreeing spelling | declared by |
| --- | --- | --- | --- |
| `AddBasicPath` | `(i32, i32) -> void` | `() -> void` | `screen.c:588` |
| `RemoveSoundObject` | `(i32, i32, i32) -> void` | `() -> void` | `screen.c:598` |
| `BoatingSchool_DrawSelection` | `(i32, i32) -> void` | `() -> void` | `screen.c:662` |
| `BoatingSchool_Remove` | `(i32, i32, i32) -> void` | `() -> void` | `screen.c:663` |
| `MonkeyTree_Remove` | `(i32, i32, i32) -> void` | `() -> void` | `screen.c:865` |
| `MonkeyFish_Remove` | `(i32, i32, i32) -> void` | `() -> void` | `screen.c:877` |
| `JungleCruise_DrawSelection` | `(i32, i32) -> void` | `() -> void` | `screen.c:884` |
| `OptionsIconInput` | `(i32, i32, i32, i32) -> i32` | `(i32, i32) -> i32` | `bigscreens.c:904` |
| `PU_CloseInput` | `(i32, i32, i32, i32) -> i32` | `(i32, i32) -> i32` | `bighelp.c:525` |
| `PU_NextInput` | `(i32, i32, i32, i32) -> i32` | `(i32, i32) -> i32` | `bighelp.c:526` |
| `PU_Delete2Input` | `(i32, i32, i32, i32) -> i32` | `(i32, i32) -> i32` | `bighelp.c:528` |

**Cluster A, rows 1-7**: `screen.c` declares seven ObjDef callbacks with an EMPTY
parameter list (`extern void AddBasicPath ();` — K&R, not `(void)`) and stores
each into a slot (`def->cb_add`, `def->cb_remove`, `def->cb_94`). It never calls
them, so nothing traps today.

**Cluster B, rows 8-11**: PORT-M8's **M8-4** class exactly — the Icon `+0x2c`
input slot spelled two-argument where the body is four. These are **four sites
M8-4's list did not name** (it had frontend2.c:67/73, bighelp.c:421, iconui.c:37,
mapscreen2.c:74). `bigscreens.c:51` declares the slot itself two-argument, which
is why each TU is internally consistent and only the cross-TU vote sees it; all
four are address-taken only, and every call THROUGH the slot in the tree is
four-argument, so nothing traps today either.

**For a matching lane**, as M8-4 already says: one prototype per row in the
declaring file under `#ifdef LEGOLAND_PORTABLE`, never between a marker and its
signature, re-gated with `audit.py` and `relocs.py`. Latent, not live — but this
is the class PORT-M1/M2/M7 each had to clear, one step from being live.

### 2b. Left-raw words: verdicts, not counts

`PTR_VERDICTS` at the top of `gen_link.py` maps an address to the lane that read
the code and what it concluded, and both `gen/pointers.md` tables grow a verdict
column. A row with no verdict prints `OPEN` in its own table instead of hiding
inside a count, and the manifest carries the total:

```
- rows still waiting for a verdict: 0 raw + 0 rejected
```

The one surviving row is `g_vwin32` 0x004b85c4, and PORT-M8 §4g already settled
it: the word is a Win32 HANDLE holding `INVALID_HANDLE_VALUE`, `unref5.c:572-578`
tests it against `(void*)-1` and resets it on close, so leaving the word exactly
as the image has it is the correct outcome. That verdict is now next to the row.

A word whose DECLARATION is wrong does not go in this table — it goes in the
declaring file, fixed. The table is only for rows where the declaration and the
image are both right and the census cannot tell.

**A7's other eleven left-raw words are gone rather than attributed**: PORT-M8
fixed the declarations that produced them (the five copter `pts`, `g_span_vtx`,
`g_music_sys`, `g_menu_help`, `g_level_markers`), and this lane's cdecl fix closed
the twelfth, `g_track_mesh`. 12 -> 1 is why the rejected-claims table is down to a
single row.

---

## 3. Two sweeps promoted out of scratch

Three lanes in a row re-ran the same two sweeps from scratch scripts that were
never committed (M6 wrote them, M8 rewrote them, this lane would have been the
third). Both are now tools with `--selftest`; the one that is a GATE rather than
an investigation is a ctest.

### 3a. `portable/tools/extern_sweep.py` — the class no byte gate can see

PORT-M6 §1f. One `extern` statement, several declarators, several addresses in its
one trailing comment:

```c
extern void *g_route_open, *g_route_closed;   /* 0x00668fc0, 0x00668fc4 */
```

Every scanner in the tree (`gen_link.py`'s `DECL_RE`, `linkreport.scan_sources`,
`cdecl.py`'s `_addr_for`) takes the FIRST address and gives it to every declarator
in the statement, so both objects came out at 0x00668fc0 and in the portable build
**`g_route_open` WAS `g_route_closed`** — the gardener and mechanic work-order
tails were their own heads.

**Why it must be a test and not a habit.** The C compiles to identical bytes
either way: on x86 the declaration only has to say a pointer lives somewhere and
the linker supplies the address. `audit.py`, `relocs.py` and `verify.py` are all
silent while two live globals alias each other, and the symptom is arbitrarily far
from the cause.

`--selftest` has four positive shapes (M6's two comment spellings, addresses on
continuation lines per PORT-M4's finding #1, an array pair) and five negative ones
that each cost a false positive if the rule is loose: one address with two names is
the ordinary split-object case that interior aliasing already handles; two
addresses with one name is prose; a FUNCTION declaration may legitimately cite a
sibling's address; `+0x14` field offsets are not addresses; and the word "extern"
in a comment above a real declaration must not start a statement.

Sources only — no gamedata, no image, no build products — so it runs in CI next to
`cdecl_extents`. Exit status is the gate. **0 tree-wide**, as M8 §6 last measured.

### 3b. `portable/tools/slot_sweep.py` — who CALLS a vtable slot

PORT-M8's two image sweeps, merged into one pass, offset as an argument. A slot's
real type is whatever its call site pushes, and the call site may be in a file that
never declares the callee — which is how M8 closed ObjDef `+0xb0` and how it
proved `+0x8c` has no caller at all. Reading declarations cannot settle either.

**Both of VC6's forms are needed**, because each is invisible to a sweep for the
other, and that is how a slot that IS called can look uncalled:

```
call dword ptr [ecx + 0xb0]              <- direct
mov  eax, [edi + 0xa0]  ...  call eax    <- load-then-call
```

Two things had to be right before the load form found anything on the real image.
Both are regressions in `--selftest` now, on hand-assembled bytes:

1. **Resynchronise the disassembly.** One `md.disasm` over `.text` stops dead at
   the first byte it cannot decode — a jump table, `/Gy` COMDAT padding, CRT data.
   On this image it gives up at **0x00437ee3**: 80,686 of roughly 200,000
   instructions, short of every call site M8 documented. `instructions()` restarts
   a byte past the stall and reports the resync so the caller drops its tracked
   registers (a tracked load across a desync is a guess, and a wrong hit is worse
   than a missed one here).
2. **A tracked register lives until it is CALLED or REDEFINED, not until the next
   branch.** Every load-then-call site in the image is written

   ```
   mov  eax, [edi + 0xa0]
   test eax, eax
   je   <skip>              <- an unset slot is simply not called
   push ...  push ...
   call eax
   ```

   so ending the register's life at the `je` loses the site — and at the `test`
   too, because `test`/`cmp` set flags and redefine nothing. With both wrong,
   `+0xa0`, `+0xac`, `+0xb8` and `+0xbc` all returned "no call site", which is
   exactly the false negative that makes a slot look untypable. That is an easy
   mistake to ship, because the tool still works perfectly on the direct form.

**Validated against PORT-M8's entire known set, reproducing its addresses:**

| slot | sites | where | M8's citation |
| --- | --- | --- | --- |
| `+0xa0` | 2 load | `RenderFullMap` call `0x004571a3`, `RenderView` call `0x0045b95a`, 2 pushes each | matches |
| `+0xac` | 1 load | `LLIDB_UnLoadLLSData` call `0x0047c6c2` | matches |
| `+0xb0` | 2 direct | `DrawAndClearPrintList` `0x00485ac3`, `0x00485b70`, 6 pushes each | matches |
| `+0xb8` | 1 load | `LoadGame` call `0x0047f517` | matches |
| `+0xbc` | 1 load | `SaveGame` call `0x0047e63d` | matches |
| `+0x8c` | **0** | no caller in the shipped binary | **M8-5 confirmed independently** |

Hits are grouped under the `// FUNCTION:` marker that owns them with `file:line`.
The push count is reported AND labelled a hint: it counts pushes since the
previous call, so spills inflate it — confirm the arity in the C at the marker.

This sweep needs `original/legoland.exe` and capstone, so it is not in the
asset-free set; its matcher is tested on hand-assembled bytes, and `--selftest`
skips **visibly** (saying so) when capstone is absent, so CI neither goes red on
an optional dependency nor green over an untested matcher.

### 3c. ctests

Added to `headless.cmake` (this lane's file), beside `cdecl_extents`:

| test | what | assets |
| --- | --- | --- |
| `extern_sweep` | **the gate** — fails on any multi-address extern | none |
| `extern_sweep_selftest` | the sweep's own shapes, so a refactor of the statement reader cannot turn the gate into a test that always passes | none |
| `slot_sweep_selftest` | the slot matcher, including the null-check shape | none |

Native `ctest` 11/11 -> **14/14**.

---

## 4. `name_trap.py`: the B9-4 class, named

`table index is out of bounds` and `memory access out of bounds` were not trap
KINDS — both fell into the generic arm, which said only that the innermost named
frame was a real body. They are first class now.

For the table case the module is decoded at the trapping offset to find the
`call_indirect`, its type, the table size, and **where its index operand came
from**, with a different report for each possibility:

* an `i32.const` in the image's `.text` is named outright, with the
  `// FUNCTION:` marker that owns it — and if the value is interior to a body
  rather than its entry point, it says so, because that is a different mistake;
* an `i32.load` means the bad value was written elsewhere and this call site only
  USED it — the usual case, since the literal is stored by a different function
  from the one that traps;
* a `local.get`/`global.get` means this frame did not choose the index at all.

Behind the load case is `--va-literals`, PORT-B9's B9-4 census done on the LINKED
MODULE instead of the sources, which catches the class however it was spelled.
**Two things make it sharp rather than useless, and both cost a wrong answer
first:**

1. **`.text` only, not the whole image.** emcc lays the rebuilt globals out in
   linear memory from a low base, so a perfectly ordinary pointer to a rebuilt
   object IS an `i32.const` in the image's DATA range. The obvious version of this
   sweep reported **2,585 hits**, almost all noise. Narrowed to
   `0x00401000..0x004ab000` it returns **9** — and the verdict is not the range
   but EXACTNESS: a value that is exactly a `// FUNCTION:` marker address is a
   recovered function address and nothing else. Five are. The other four
   (`0x004600c1` in `DrawPopUpMock`, `0x0040e040` in `RenderWorkOrders`,
   `0x00410000` twice in `open`) are ordinary integers that happen to land in
   `.text`, and are reported in a separate section rather than dressed up as
   findings.
2. **Both halves of an `i64.const`.** The optimiser merges two adjacent 4-byte
   stores of constants into one 8-byte store, so `sweep3.c`'s five literals reach
   the module as one `i32.const` and two `i64.const`s
   (`19685518749469616` = `0x0045efe0` / `0x00480bb0`). An i32-only sweep finds
   **one of the five** and would have reported the class as nearly closed.

**What it says on this tree**, identical on `legoland_headless_debug.wasm` and
`legoland.wasm`:

```
SetStandardCallbacks  (5 distinct)       sweep3.c:162, // FUNCTION: 0x00480cd0
   0x0045efe0  -> AddBasicObject          (objmap2.c:459)
   0x0045f220  -> StandardRemoveObject    (objmap2.c:982)
   0x0045fa80  -> CalcBasicObjectCursor   (objmap.c:332)
   0x00480b70  -> SetEditObjectFromElem   (pathobj2.c:243)
   0x00480bb0  -> BasicObjectDCalcCursor  (objmap2.c:505)
```

**Two of those PORT-B9 could not name** — its source comment gives
`pathobj2.c:243` with no function name for `0x480b70`, and nothing at all for
`0x45f220`. PORT-M9 now has all five bodies to take the address of.

Two findings for PORT-M9 while it is in there:

* **`loaders.c`'s 21 sites are in NEITHER linked module.** `GetInterface` is
  dead-code-eliminated (no `call_indirect`, no instructions, no name), so those 21
  are latent, not live: fixing `sweep3.c` alone closes what the park load actually
  hits. Worth knowing before budgeting 29 sites.
* **`coaster10.c`'s three are `.data` addresses** (`0x004b5648`), out of this
  `.text` window by design. The `.data` half of the class is `gen/pointers.md`'s
  existing `raw pointer words` gate, which is at 0.

Also added: `--kind {indirect,table,memory}` so `--at <offset>` can explain an
offset someone else reported as any of the three, and `--va-literals` to run the
census alone (exit 1 on any exact hit, so it could become a gate once the class is
closed). The module docstring now documents six ways of dying, not four.

---

## 5. Census, before and after

`portable/tools/linkreport.py portable/build-wasm`, measured on the wasm closure
at the end of the lane:

| category | count | meaning |
| --- | --- | --- |
| objects | 258 | |
| defined symbols | 6606 | |
| undefined | 48 | |
| game-fn | 0 | referenced game functions not yet written |
| game-data | 0 | extern-only globals |
| alias | 0 | stale extern names |
| host | 0 | Win32/DirectX imports without a shim |
| crt | 44 | libc/libm at link time |
| unknown | 4 | `__errno_location`, `__indirect_function_table`, `__small_sprintf`, `__stack_pointer` — all toolchain-internal, no game symbol |
| duplicates | 0 | |
| asm stubs | 3 | `LL_UNPORTED_ASM`, all proved unreachable by PORT-B5 |
| prototype conflicts | 404 | the sources disagree about a signature |

**No before/after column, because the census cannot move**: `linkreport.py` reads
the game objects, and this lane compiled no new game code and changed no symbol's
name, type or existence. What the lane CAN move is the generated closure, and that
was diffed directly instead — §1's manifest table and the `globals.c` proof, where
the only changes are the one object whose declared extent was wrong.

The 404 prototype conflicts are the whole-tree census, of which the 11 in §2a are
the subset wasm-ld's vote actually sees (a conflict needs two TUs to reference the
same symbol with different signatures; most of the 404 are a single declaring
file disagreeing with a definition it never calls).

---

## 6. Findings this lane records rather than fixes

| # | what | owner |
| --- | --- | --- |
| **A8-1** | **Eleven conflicting wasm signatures are eleven game-side declarations**, all address-taken-only and so latent. Seven are `screen.c`'s empty-parameter-list ObjDef callbacks (`:588`, `:598`, `:662`, `:663`, `:865`, `:877`, `:884`); four are M8-4's icon-input class at sites M8-4 did not list (`bigscreens.c:904`, `bighelp.c:525/526/528`). Full table with the body's signature in §2a and in `gen/manifest.md`. | a matching lane |
| **A8-2** | **`loaders.c`'s 21 B9-4 sites are dead code** in both linked modules — `GetInterface` is DCE'd. B9-4's live set is `sweep3.c`'s five, now all named (§4). | PORT-M9 |
| **A8-3** | `MeshDesc` exists twice, `schoolcar3.c:277` and `unref2.c:220`, laid out identically. Only the first has an object at an address, so only it was ever visible; the duplicate is harmless but will diverge if one file is edited. | a matching lane, if either is touched |
| **A8-4** | `--va-literals` exits 1 on any exact hit and could be wired as a ctest the moment PORT-M9 closes `sweep3.c` — it needs a built module, so it belongs with the wasm ctests rather than the asset-free set. | a PORT-A lane, after M9 |

---

## 7. Scratch scripts (all `port-a8-` prefixed, none committed)

* `port-a8-cdecl-probe.py` — M8-3's repro plus the nine declarator shapes, one
  line each. The four-line version of deliverable 1's proof.
* `port-a8-ptrcensus.py` — extent + pointer offsets for every object declaration,
  under any `cdecl.py`. The 4,816-line diff in §1.
* `port-a8-bytesproof.py` — `globals.c` as `{absolute VA -> initialiser}`, so two
  re-split closures can be compared by ADDRESS rather than by line. This is the
  reusable one: any future change to the block plan should be proved this way,
  because a line diff cannot tell "moved" from "changed".
* `port-a8-sigconf.py` — the signature vote per conflicting name (superseded by
  `linkreport.wasm_sig_conflict_detail`, kept as its oracle).
* `port-a8-build-native.sh` / `port-a8-build-wasm.sh` — clean-directory builds and
  gates.

---

## 8. Gate results

| gate | result |
| --- | --- |
| native `cmake` + `ninja` from a CLEAN dir | builds |
| `./portable/build/legoland_linkcheck` | links and runs |
| native `ctest` | **14/14** |
| wasm `emcmake` + `ninja` from a CLEAN dir | builds |
| `legoland_headless`, `_debug`, `_tests`, `_pathtest`, `_shimtest`, `_browser`, `_cbtypes` | all build |
| wasm `ctest` | **20/20** |
| `cdecl.py --selftest` | 0 failures (22 new checks) |
| `extern_sweep.py --selftest` / sweep | 0 failures / 0 hits tree-wide |
| `slot_sweep.py --selftest` | 0 failures |
| `raw pointer words` (the pointer gate) | 0, both closures |
| `LEGOLAND/*.c` touched | **none** — `audit.py`, `relocs.py`, `verify.py` not applicable |
