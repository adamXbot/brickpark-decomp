# Scope PORT-A4 — the generator's last mismatch, indirect-call traps, wasm32 CI

> **Status: all five deliverables complete (2026-09-12).** Branch `scope/PORT-A4`
> from `595aae91` (the PORT-B3 merge). **wasm-ld signature mismatches 5 -> 0 lines
> / 1 -> 0 symbols, binaryen validator errors 0 on all 7 modules**; `name_trap.py`
> names a `call_indirect` type mismatch (caller, the call site's expected type,
> the candidate targets ranked, and the callback-table global to re-declare);
> CI gained `portable-wasm`, which links all five wasm32 targets and runs 5 ctests
> with no game assets (it used to run none); `headless_spine` now asserts the
> first present is 98% non-black with frame checksum `0x4a092b01`, and its skip
> list lost `loadsprite`. Native ctest 5/5, wasm32 ctest 10/10, census unchanged
> (this lane touched no game source).
>
> Files: `portable/tools/gen_link.py`, `name_trap.py`,
> `portable/cmake/headless.cmake`, `portable/src/headless/{main,node_shim}.c`,
> and — outside PORT-A's list, §5 names why — `portable/cmake/tests.cmake`,
> `portable/tests/{ll_tests.c,ll_tests.h,test_rle_paint.c}` and the delegated
> `.github/workflows/progress.yml`. `linkreport.py`, `kernel32.c` and `msvcrt.c`
> needed no change.

## 1. The last wasm-ld signature mismatch: `printf` typed from the alias

PORT-M2 closed 18 of the 19 call-site mismatches and filed the survivor as
generator-side (`docs/lanes/scope-port-m2.md` section 5). It was exactly that.

`DebugPrint` (logflume2.c, logflume8.c, `void DebugPrint(const char*)`) is a
second name for 0x0049e5c5, which bigrender.c / printlist.c / unref7.c declare
as the CRT's `printf`. The alias machinery emitted

```c
extern void printf(unsigned int);                  /* the ALIAS's signature */
void DebugPrint(unsigned int a0) { printf(a0); }
```

because of one line:

```python
rsig = def_sig_of(real) or sig        # the signature of the ALIAS as fallback
```

against libc's `(i32, i32) -> i32`. wasm-ld warned and resolved **every** call
through that declaration — the game's own `printf` calls included — to a
trapping stub.

### The fix: libc's prototype, from a table, never the alias's

Three pieces in `gen_link.py`:

1. **`CRT_PROTOS`** — a table of the libc prototypes the game's CRT aliases
   reach, as `(return type, fixed parameter types, variadic?)` in C: `printf`,
   `sprintf`, `malloc`, `calloc`, `realloc`, `free`, `rand`, `srand`, the
   `str*`/`mem*` family, `qsort`, the stdio file calls, the MSVC `_stricmp`
   spellings, and the f64 math calls (the only rows whose wasm types are not all
   i32). `crt_wasm_sig` derives the wasm signature from a row, adding **one
   extra i32 for a variadic callee**, because that is how clang lowers `...` on
   wasm32: the variable arguments go into a buffer and its address is the last
   parameter. That is why libc's `printf` is `(i32, i32) -> i32`.

2. **`defined_here(name)`** — a new predicate, and the one that made the fix
   general. The existing `def_sig_of` falls back to the signature the
   *references* voted for, so `def_sig_of('malloc')` answers with the **game's**
   declaration even though no object in the tree defines `malloc`. Only
   `defined_here` (wasm `defined` map alone) says whose prototype wins. With the
   gate on `def_sig_of(...) is None` the fix reached 3 aliases; with
   `not defined_here(...)` it reaches 11, and `_stricmp` correctly stays on the
   old path because `portable/src/hostwin/msvcrt.c` really does define it.

3. **`crt_alias`** — emits the forwarder with **per-argument C casts, never a
   cast of the function pointer**. PORT-M2 section 4 is why: a cast call lowers
   to `call_indirect`, and binaryen's `directize` rewrites a constant-index
   `call_indirect` into a direct call whose argument types then disagree, so the
   module fails validation far from the defect.

Two spellings, and telling them apart is the whole subtlety:

| the alias is | example | what the forwarder does |
| --- | --- | --- |
| spelled variadic (its wasm signature has one parameter MORE than libc's fixed count, and that last one IS the varargs pointer clang handed it) | `Format` -> `sprintf`, `sprintf_w` -> `sprintf` | passes the pointer **straight through**: the callee is declared flattened (fixed parameters plus `void*`), which is precisely the wasm signature libc's variadic definition has |
| spelled non-variadic | `DebugPrint` -> `printf` | calls the **real variadic prototype**, so clang builds the (empty) buffer and emits the `(i32, i32) -> i32` call libc expects |

The flattened spelling gets its own C identifier with an `__asm__` label naming
the real symbol (`extern int ll_crt_sprintf_va(char*, const char*, void*)
__asm__("sprintf");`), so both spellings of one libc function can coexist in a
single translation unit — needed the day some alias of `sprintf` is spelled with
two arguments while `Format` keeps three.

### What the generated file looks like now

```c
extern int printf(const char*, ...);
void DebugPrint(unsigned int a0) { (void)(printf((const char*)(__UINTPTR_TYPE__)(a0))); }

extern int ll_crt_sprintf_va(char*, const char*, void*) __asm__("sprintf");
unsigned int Format(unsigned int a0, unsigned int a1, unsigned int a2)
{ return (unsigned int)(ll_crt_sprintf_va((char*)(__UINTPTR_TYPE__)(a0),
                                          (const char*)(__UINTPTR_TYPE__)(a1),
                                          (void*)(__UINTPTR_TYPE__)(a2))); }

extern void free(void*);
void HeapFree_w(unsigned int a0) { free((void*)(__UINTPTR_TYPE__)(a0)); }
```

`free` is worth a second look: it was `extern void free(unsigned int)` before,
which happened to have libc's wasm signature and so never warned. It is now
typed from libc like the rest, which means the next time emscripten's ABI makes
a pointer something other than one i32 this file does not quietly go wrong.

### Result

| | at lane start (`595aae91`) | after |
| --- | --- | --- |
| wasm-ld `function signature mismatch` lines, all seven links | 5 (`printf`, one per link that pulls libc's printf.o) | **0** |
| distinct symbols warned about | 1 | **0** |
| `wasm-opt --all-features` validator errors, all 7 modules | 0 | **0** |
| forwarders typed from libc rather than from the alias | 0 | **11** |
| CRT targets with no `CRT_PROTOS` row | — | **0** |

Both builds from clean directories: native `ninja` + `legoland_linkcheck`
("every symbol resolved") + `ctest` 5/5; wasm32 all seven targets + `ctest`
10/10.

## 2. A census the generator now publishes: cast forwarders

While closing the CRT rows, the manifest grew the section that names the **next**
defect class, which is deliverable 2's subject and PORT-M2's open follow-up: an
alias whose callers emitted a signature the real body does not have. There are
**80** of them, and every one is a latent runtime trap — the forwarder casts the
function pointer, the call becomes `call_indirect`, and wasm checks the type at
the call.

`gen/manifest.md` now ends with a table of all 80: alias, body, what the callers
emitted, what the body has. They are the `void Foo()`-in-a-table family PORT-M1
and PORT-M2 described (`Bank_Activate` -> `Bank_TickCustomers(i32)`,
`Temple_Interact` -> `Temple_Draw(i32 x 6)`, ...), plus a handful of real
typing defects (`InitRasterBuffer` -> `Render_SetPixelFormat` disagree on the
return, five `sub_*` names likewise). The fix belongs in the declaring game file,
which is lane PORT-M3's; the generator's job is to name them, not bridge them.

## 3. Naming a `call_indirect` type mismatch (deliverable 2)

This is the class the front end will hit next, and it is the only one that
survives a **completely clean link**. The reason is worth stating precisely,
because it determines what the tool can and cannot know:

> A `call_indirect`'s type is an **immediate on the instruction**. It is not a
> property of a symbol. There is no declaration for wasm-ld to compare, so there
> is no warning, no `signature_mismatch:` stub and no undefined symbol — the
> module is valid, and the check happens at the call, at run time.

What node gives you is one line:

```
RuntimeError: function signature mismatch
```

No index, no caller, no types. (V8 also says `null function or function signature
mismatch` when the slot is empty; `name_trap.py` matches both.)

### The lever: V8's offset is the module file offset

The innermost wasm frame reads `wasm-function[N]:0xOFF`, and **0xOFF is the
instruction's byte offset in the module file** — the same number `llvm-objdump -d`
prints in its left column. Confirmed against a synthetic repro: V8 reported
`wasm-function[7]:0x33b` and objdump's line at `33b:` is
`11 03 00  call_indirect 3`. So the offset names **one** `call_indirect`, and its
type immediate is the type the slot was supposed to hold. That is half the work
item for free.

### What the report reconstructs

| the brief asked for | where it comes from |
| --- | --- |
| the caller | the enclosing `<name>` in objdump's output at that offset (and the stack frame as a fallback) |
| the table slot's expected type | the `call_indirect`'s type immediate, resolved through the module's type section |
| the target function and its real type | the element segment (slot -> function index) + the type section, narrowed and ranked — see below |

Plus the eight instructions before the call, whose `i32.const` values are linear-
memory addresses: that is the **callback table's own address**, which `grep`
against `gen/globals.c` turns into a named global.

A linked module needed a new reader: `linkreport.wasm_object_sigs` works on
OBJECT files, where the `linking` custom section carries a symbol table, and a
linked module has none at all. The five sections `name_trap.Module` reads are
type (1), import (2), function (3), element (9) and the `name` custom section —
which is why `--profiling-funcs` (or `-g2`) is not optional for this report.

### Two things that make the candidate list usable

215 of the 1012 table entries have a type other than any given call site's, so an
unfiltered list is useless. Two passes fix it:

1. **Narrowed by `gen/globals.c`.** gen_link re-points every data word that holds
   a function address, so the `&Name` occurrences in the generated globals are
   *exactly* the set of functions a **static** game callback table can reach —
   216 of the table's distinct functions in the current build. A function nothing
   stores in `.data` cannot be in a slot the game loaded from `.data`. When
   nothing in that set has the wrong type, the slot was filled at **runtime**
   (`LLIDB_RegisterNewElement`, the `screen.c` tables) and the report says so
   rather than guessing, falling back to the whole table.
2. **Ranked by type distance, not alphabetically.** `type_distance` puts same-
   arity, same-return, fewest-parameters-retyped first, because that is the shape
   of the real defect — PORT-M2 §2 and §3 are a float passed as its bit pattern
   and a struct flattened into ints, both of which are "one parameter retyped".

And the headline is the **table**, not the function: one global holds a whole
family of wrongly-typed bodies, and re-declaring that global fixes every slot at
once. On `TrackCurve_EvaluatePosition`'s vtable call the summary reads

```
== by callback table: the global holding the wrong-typed bodies is what to re-declare
   g_level_db_sections   (i32, i32, i32) -> i32 x69, (i32, i32) -> i32 x24
   g_event_tick          (i32) -> i32 x68
   g_track_desc_flat     (i32) -> void x3, (i32, f32) -> void x1, (i32) -> i32 x1
```

The name shown is the **block's head symbol** — gen_link emits one block per
object with the rest as offset aliases — so `gen/manifest.md`'s interior-alias
table says which declared name a slot really belongs to. The report says so.

### Three entry points

```bash
python3 portable/tools/name_trap.py -- --stages     # automatic, on a live trap
python3 portable/tools/name_trap.py --at 0x266ea    # one call site, no run
python3 portable/tools/name_trap.py --table         # the table's type census
```

`--at` matters more than it looks: it explains a frame from a **browser** trap, or
one someone else pasted, with no harness run at all.

### On `-sASSERTIONS=2` and `-sSAFE_HEAP`

Both were in the brief and both are worth reaching for, but **neither improves
this message**: a signature mismatch is a wasm-level type check, not an
emscripten assertion, and `SAFE_HEAP` instruments loads and stores. They earn
their keep one step earlier — `SAFE_HEAP` catches the out-of-bounds load that put
a garbage index in the table slot in the first place, which presents as the same
`RuntimeError`. The workflow in `portable/README.md` says which to reach for
when. `--profiling-funcs` is the one that is load-bearing here, and
`legoland_headless_debug` already carries it plus `-O0 -g2` at link.

### The static counterpart

`gen/manifest.md` now ends with a **Cast forwarders** table: every alias whose
callers emitted a signature the real body does not have — **80** rows, with both
signatures each. Every one is a latent trap of exactly this kind, seen statically
before the game reaches it. They are the `void Foo()`-in-a-table family
(`Bank_Activate` -> `Bank_TickCustomers(i32)`, `Temple_Interact` ->
`Temple_Draw(i32 x 6)`) plus a handful of real typing defects
(`InitRasterBuffer` -> `Render_SetPixelFormat` disagree on the return, five
`sub_*` names likewise). The fix is PORT-M3's typed-callback pass in the
declaring files.

**The game does not reach one of these yet**, which is worth recording honestly:
with `--stages`, `name_trap.py` runs the whole of InitSession plus the title
screen and exits 0. The full `WinMain` spine times out instead, in `RunGame`'s
`while (g_music_disabled == 0)` music wait (PORT-B's `dsound.c`). So this tooling
was validated on a synthetic repro (a `()->void` cast over a `(i32)->void` body:
caller, expected type and table address all reported correctly) and on the real
module's own geometry-vtable call sites via `--at`, not on a live game trap.

## 4. The first present as a regression gate (deliverable 4)

`headless_spine` asserted "the spine got this far". With PORT-B3's painters in it
asserts **pixels**.

`node_shim.c` already counted the first present's non-black pixels for the trace;
it now records them properly and adds a checksum. Two corrections went in with
that: the count walked `w*h` flat words, which on a surface whose pitch is not
`2*w` both misses pixels and counts the padding, so it now walks the pitch-correct
rows — and the checksum (FNV-1a over the 16-bpp words) does the same, so the
padding the game never writes cannot change the identity.

`main.c` gains a `title` stage: RunGame's own prefix, `SetupControllers();
LLIDB_ClearOnLevel(); ResetController(); SetPointer(0); ProcessSystemEvents();
ShowTitleScreen();`. **RunGame itself cannot be the thing that runs** — four
statements past `ShowTitleScreen` it enters

```c
while (g_music_disabled == 0) { PeekMessageA(&msg, 0, 0, 0, 0); Sleep(100); }
```

and nothing clears that flag while `DirectSoundCreate` reports no driver
(PORT-B3's recorded next blocker, PORT-B's `dsound.c`). Calling the prefix
directly is what makes the title screen a test instead of something a human
watches in a tab.

What it measures, reproduced across runs:

```
legoland_headless: first present 640x480, 301157/307200 non-black (98%),
                   frame checksum 0x4a092b01, 1 present call(s)
legoland_headless: first present OK
```

Two assertions:

* **at least 40% non-black.** Deliberately far under the real 98%, because the
  failure this catches is "the painters drew nothing" — which collapses the count
  to a handful of pixels or to zero, never to 40%. A floor close to 98% would
  make the test fragile about the artwork without catching anything more.
* **the frame checksum equals `--frame-sum`.** `cmake/headless.cmake` pins
  `LL_TITLE_FRAME_SUM = 0x4a092b01`. A change that alters the title screen fails
  here by design; the comment in that file says what to do either way, and
  setting it to `0` keeps the non-black floor and only prints the checksum.

The ctest keys on a new single line, `SPINE OK, first present checksum ...`,
printed only after every stage ran **and** the frame passed both assertions —
ctest's `PASS_REGULAR_EXPRESSION` is an OR over its list, so two separate patterns
could not have expressed the conjunction.

**The skip list lost `loadsprite`.** It was there because `__BMPLoader`'s
`RES_CloseFile` call was poisoned; PORT-M2 closed that, the eight cursor sprites
load, and the ratchet `headless.cmake`'s comment describes tightened by one.
`rungame` is the only entry left and it is not a defect — the `title` stage covers
what it was hiding.

Verified both ways: the gate passes at `0x4a092b01` and fails with a named reason
(`FAIL frame checksum 0x4a092b01, expected 0xdeadbeef`) against a wrong value.

## 5. CI on the wasm32 tree (deliverable 3)

`.github/workflows/progress.yml` had one portable job, native clang — a compile-
and-link census on a target that **cannot run the game** (every serialised layout
in the recovered code assumes 4-byte pointers and longs). `portable-wasm` is the
job on the tree that can. `setup-emsdk@v14` pinned to **6.0.9**, the version this
tree is developed against; a different emsdk can change the libc signatures
§1's table is matched against, which is the class of breakage this job exists to
catch.

Beyond "it builds", two gates:

* **no wasm-ld `function signature mismatch`** — it is a *warning*, so the link
  succeeds and only the log says a call site is poisoned. The job tees the link
  output and greps it, which is what makes §1 a ratchet rather than a one-off.
* **`legoland_browser` must link** — the one link that runs ASYNCIFY and
  binaryen's `directize`, so the one that catches an invalid module. PORT-M2 §4's
  lesson, "a clean wasm-ld run is not a valid module", has a gate now.

### Which ctests, and the two things CI does not have

**No `original/legoland.exe`**: `gen_link.py` zero-fills every rebuilt global
instead of initialising it from the exe's `.data`. The link still closes, and
nothing asserted here depends on a shipped value.

**No `gamedata/`**, which needed two fixes.

`cmake/tests.cmake` used to `return()` at the top when `gamedata/` was missing,
registering **nothing** — so CI tested none of the recovered code's behaviour at
all. Four tests need no asset:

| test | needs gamedata? | in CI |
| --- | --- | --- |
| `save_framing` | no — the oracle emits a synthetic script of framing operations | yes |
| `keystate` | no — pure layout, gen_link's interior aliases | yes |
| `rle_paint` | nine synthetic cases do not; the tenth decodes a shipped sprite | yes, 38 of 43 checks |
| `anim_paint` | no — its own encoder, a synthetic type-2 frame | yes |
| `install_paths` | no — the fixture is empty files generated at configure time | yes |
| `tile_geometry` | **yes** — the oracle samples `GLONE.MAP` for the grid it exercises | no |
| `res_archive`, `llidb_icm`, `loadpos` | yes, all three read the volumes | no |
| `headless_spine` | yes — the mount, the string table, the title artwork | no |

`tile_geometry` is the one that surprises: the C it drives reads no file, but its
*oracle* samples a real level, so it is an asset test.

The four asset-reading tests are not merely unregistered but **not compiled** —
their oracles cannot produce a header at all without the volumes, and a test that
cannot be given its expectations cannot be linked. `LL_TESTS_NO_GAMEDATA` leaves
them out of `ll_tests.c`'s subcommand table, so the driver's usage listing is an
honest statement of what the build can run. `test_rle_paint.c` is the hybrid:
`tests.cmake` writes a stub oracle header defining `LL_RLE_NO_ASSETS`, and
`real_sprite()` calls the new **`ll_skip()`** — a third outcome beside pass and
fail, added because a skipped check must not look like coverage:

```
skip     one real shipped sprite, bit-exact against tools/comp.py: gamedata/ is
         absent, so the oracle has no digests -- the nine synthetic cases above
         are this build's coverage
== rle_paint: 38 checks, 0 failed, 0 notes
```

**The second fix, and a finding for PORT-B.** `legoland_browser` bakes its assets
in with `--preload-file $LL_GAMEDATA/main@/gamedata`, **unconditionally**, and
emcc's `file_packager` fails outright on a directory that does not exist:

```
file_packager: error: $.../gamedata/main@/gamedata does not exist
emcc: error: '.../file_packager legoland.data --from-emcc --preload ...' failed
```

So the browser target could not be linked in CI at all. The job creates an empty
tree (one zero-byte `main/stab.str`) and configures `-DLL_GAMEDATA=<that>
-DLL_PRELOAD_RES=OFF`, which links the real target against an empty `.data`. The
page cannot *run* from it and is not asked to. This is a workaround in the
workflow and not in `cmake/browser.cmake`, which is PORT-B's file; when PORT-B's
WASMFS fetch backend (`scope-port-b.md` §6) replaces `--preload-file`, the step
goes away. **PORT-B may prefer to gate the `main` preload on the directory
existing**, which would make the workaround unnecessary.

### Verified by replaying the job

A copy of the tree with no `gamedata/` and no `original/`, step for step:

```
-- PORT-A headless: gamedata/ not present, headless_spine not registered
-- PORT-C tests: gamedata/ not present -- registering the asset-free subset
   (save_framing, keystate, rle_paint, anim_paint); tile_geometry, res_archive,
   llidb_icm and loadpos need the volumes and are not built
271 sources compile; all five targets link
0 'function signature mismatch' lines
legoland_linkcheck: every symbol resolved
100% tests passed out of 5
```

The YAML parses (`yaml.safe_load`); `gh workflow view` was not available in this
environment (no authenticated remote from the worktree). The native job is
untouched.

### Files this lane edited outside its own list

Deliverable 3 could not be done inside PORT-A's file list, so, named for the
integrator: `portable/cmake/tests.cmake`, `portable/tests/ll_tests.c`,
`portable/tests/ll_tests.h` and `portable/tests/test_rle_paint.c` are PORT-C's /
PORT-B3's, and `.github/workflows/progress.yml` is the integrator's (delegated in
the brief). Nothing in `LEGOLAND/*.c`, `portable/src/browser/**`,
`portable/CMakeLists.txt` or `docs/HANDOFF.md` was touched, no PORT-B shim file
was touched, and `verify.py` / `audit.py` / `match.py` were not run.

## 6. Census (deliverable 5)

`linkreport.py` over `portable/build*/CMakeFiles/legoland_core.dir`, 258 objects:

| category | wasm32 | native | change in this lane |
| --- | --- | --- | --- |
| game-fn (CRT-range wrappers) | 12 | 12 | — |
| game-data | 2616 | 2617 | — |
| alias (stale extern names) | 228 | 228 | — |
| host (Win32/DirectX) | 95 | 95 | — |
| crt | 43 | 56 | — |
| unknown (no address comment) | 86 | 81 | — |
| duplicates | 0 | 0 | — |
| asm stubs | 15 | 15 | — |
| prototype conflicts | 420 | n/a | — |

**Nothing in the census moves**, and that is the expected result: this lane
touched no game source. The 420 prototype conflicts are what PORT-M1 and PORT-M2
described — address-taken callbacks declared `void Foo()` in the file that stores
them in a table, plus dead or unreached `unref*.c` rows — and they are the same
population as the 80 cast forwarders and the `call_indirect` class §3 names. One
pass (PORT-M3's typed callbacks) closes all three.

What did move:

| | at lane start (`595aae91`) | after |
| --- | --- | --- |
| wasm-ld `function signature mismatch` lines across all 7 links | 5 | **0** |
| distinct symbols warned about | 1 (`printf`) | **0** |
| `wasm-opt --all-features` validator errors, 7 modules | 0 | **0** |
| forwarders typed from libc rather than the alias | 0 | **11** |
| CRT alias targets with no `CRT_PROTOS` row | — | **0** |
| cast forwarders named in the manifest | 0 (uncounted) | **80** |
| native ctest | 5/5 | **5/5** |
| wasm32 ctest | 10/10 | **10/10** |
| wasm32 ctest with no `gamedata/` | 0 registered | **5/5** |
| `headless_spine` asserts | "the spine reached `--- done`" | **the first present: 98% non-black, checksum `0x4a092b01`** |
| `--stages` skip list | `loadsprite,rungame` | **`rungame`** |
| CI jobs building the portable tree | 1 (native) | **2 (native + wasm32)** |
