# Scope PORT-A11 — a gate for variadic CRT forwards declared non-variadic

> **Status: complete (2026-09-12).** Branch `scope/PORT-A11` from `d8d4d368`
> (the PORT-P5 merge), which did **not** yet carry PORT-M20's fix, so
> `mechrides.c:3135` was live and is this lane's positive test case rather than a
> reconstruction. **`portable/tools/variadic_sweep.py` is new: 0 conflicts in 10
> variadic functions over 108 declarations, from 4 conflicts in 3 functions at
> lane start.** Two of the three were unknown before this lane
> (`llidb_odf.c:48`, and the `DebugPrint` pair), and `gen_link.py` now REFUSES to
> close the link while one is open (verified: exit 3, nothing generated).
> Two ctests added (`variadic_sweep`, `variadic_sweep_selftest`), native 19 -> 21
> and wasm32 26 -> 28, both green from clean directories. Four `LEGOLAND/*.c`
> files touched, guard arms only, every audit row and every relocs line
> byte-identical to before.
>
> Files: `portable/tools/variadic_sweep.py` (new), `portable/tools/gen_link.py`,
> `portable/cmake/headless.cmake`, `portable/README.md`,
> `docs/SCOPE_PORT_WAVE.md`, and the four guard arms in `LEGOLAND/mechrides.c`,
> `llidb_odf.c`, `logflume2.c`, `logflume8.c`. `linkreport.py` and
> `name_trap.py` needed no change.

## 1. The class, stated exactly

clang lowers `...` on wasm32 by writing the variable arguments into a buffer and
passing **its address** as one extra parameter. So libc's

```c
int sprintf(char* dst, const char* fmt, ...);     /* (i32, i32, i32) -> i32 */
```

has the same wasm signature as a fixed three-parameter declaration of the same
function

```c
int sprintf_w(char* dst, const char* fmt, int v); /* (i32, i32, i32) -> i32 */
```

and the third i32 is a **pointer** in one and a **value** in the other. The
consequences are what make this worth a tool:

| witness | what it sees |
| --- | --- |
| wasm-ld | nothing. Both call sites and the definition agree on the wasm type, so there is no `function signature mismatch` warning and no `signature_mismatch:` stub. |
| `linkreport.py`'s signature vote | nothing. It compares the wasm signatures it reads out of the objects. |
| `name_trap.py` | nothing. There is no `call_indirect`: the call is direct and well-typed. |
| binaryen's validator | nothing. The module is valid. |
| `audit.py` / `relocs.py` / `match.py` / `verify.py` | nothing, ever. VC6 pushes one dword for the third argument either way, so the object bytes are identical in the broken state and in the fixed one. |

PORT-M20 paid for it in the Spider Ride: `sprintf` read the seat number as the
address of a `va_list`, every `"%02d"` came out `00`, and every rider on the
Spider, Safari, Spinning Barrels and Plane asked for path `…00`.

This is the fourth member of a family the project now has a sweep for each of:
`extern_sweep.py` (one statement, two addresses), `addr_sweep.py` (one name, two
addresses), `bvstruct_sweep.py` (a ≤4-byte aggregate passed by value) and this
one. All four are portable-build-only defects that every byte gate in the project
is blind to **by construction**, not by accident.

## 2. The sweep

`portable/tools/variadic_sweep.py` reads `LEGOLAND/*.c` and `*.h` and requires
every live declaration of one function to agree with the callee about **being
variadic** and about **where the `...` starts**.

Two scanning passes, because the tree spells a prototype two ways and this lane
found a defect behind each:

* **A** — every `extern` statement at any brace depth, read with
  `linkreport._extern_positions` / `_statement_at` / `_declarator_name`, so the
  sweep sees exactly the text `lr.scan_sources` and `gen_link` see.
* **B** — every statement at **file scope**, which is how `audiomisc.c` spells
  `int sprintf(char*, const char*, ...);` with no `extern` at all (6 files do),
  and how `sysstubs.c` spells the **definition** `void DebugPrintf(const char*,
  ...) {}`. The definition is the most valuable row in the file: it is the
  group's truth.

Grouping is by the address the declaration's comment cites; a declaration citing
none joins its name's address when the tree cites exactly one for that name
(which is how `loadmap.c`'s `Format` and `profiles.c`'s `DBPrintf` are grouped
rather than sitting alone). One name at two addresses is `addr_sweep`'s class, not
this one's, and is deliberately left un-merged.

The truth a group is measured against, in order:

1. a **definition** in the tree (`sysstubs.c:158` for 0x0047f870),
2. else libc's prototype from `gen_link.CRT_PROTOS` — the same table PORT-A4 §1
   introduced, and for the same reason: the alias's own spelling is never the
   authority when somebody else owns the body,
3. else the majority of the declarations.

Four ways to disagree, reported with a severity, because the generator bridges
exactly one of them:

| code | shape | consequence |
| --- | --- | --- |
| `VA-SLOT` | non-variadic with MORE fixed parameters than the variadic one | argument `nfix+1` lands in the slot the callee reads as its `va_list` pointer. M20's defect. Silent in every other gate, because the wasm arity is *equal*. |
| `VA-COUNT` | variadic, but the `...` starts elsewhere | the buffer is built at the wrong offset — the same defect one argument along |
| `VA-UNSPEC` | `extern void Foo();` | no prototype: the call site emits what it is handed and builds no buffer |
| `VA-EMPTY` | non-variadic with EXACTLY the variadic's fixed count | for a **CRT** target `gen_link.crt_alias` bridges it (it declares libc's real prototype, so clang builds the empty buffer — PORT-A4 §1's `DebugPrint` → `printf`); for a **game** target it becomes a cast forwarder and traps at the `call_indirect` |

There is **no baseline file**, unlike `addr_sweep` and `bvstruct_sweep`: every row
this lane found had a fix, and all of them are made, so the honest gate is zero.

### Preprocessor-aware twice over, and both phantoms were loud

* `#ifndef LEGOLAND_PORTABLE` arms are skipped (`bvstruct_sweep.portable_lines`).
  Every fix of this class *is* such an arm, so a sweep that read both arms would
  report all of its own repairs.
* **PORT-M3's `_vc6_body` rename is honoured.** `sweep1.c` keeps the matched VC6
  body of `DBPrintf` and gives the portable build the honest prototype by
  renaming the first:

  ```c
  #ifdef LEGOLAND_PORTABLE
  #define DBPrintf DBPrintf_vc6_body
  #endif
  ...
  void DBPrintf(void) { }                                /* the matched body */
  #ifdef LEGOLAND_PORTABLE
  #undef DBPrintf
  void DBPrintf(const char* fmt, ...) { (void)fmt; }     /* the real one */
  #endif
  ```

  A reader that missed the rename made `(void)` the callee and reported **all 34
  honest declarations of `DBPrintf` as conflicts** — the loudest phantom
  available. The sweep tracks identifier-to-identifier `#define`/`#undef` over the
  live lines, and a body compiled under a different name does not claim the
  marker's address: in the portable build the symbol at that address is the
  wrapper that kept the name.

### A finding about `linkreport.scan_sources` (not fixed here)

The sweep originally asked `lr.scan_sources()` where a definition lives and got a
second phantom: `wsprintfA` grouped under `LoadLmsModel`'s 0x00420640. The cause
is in `lr.scan_sources`, which associates a `// FUNCTION:` marker with the next
**line** that looks like a signature and is not `extern`-prefixed. At
`schoolcar7.c:417` that line is

```c
__declspec(dllimport) int __cdecl wsprintfA(char* out, const char* fmt, ...);
```

— a prototype, not a body — so `defined_at[0x00420640]` reads `('wsprintfA',
'schoolcar7.c')` instead of `('LoadLmsModel', …)`. The sweep no longer depends on
it (it reads the marker above the body itself and **consumes** it, so a second
body cannot inherit one), and nothing else in the tree was found to depend on that
row either: `gen_link` uses `defined_at` for `symbol_at`/`known`, where both names
are at the same address. **Left open for a later PORT-A lane**: make
`scan_sources` skip a statement whose declarator is a prototype (no `{`), the way
this module's pass B does. One line of consequence, zero known today.

## 3. The hit list — 4 sites, 3 functions

| where | declared | callee really is | code | verdict |
| --- | --- | --- | --- | --- |
| `mechrides.c:3135` | `int sprintf_w(char* dst, const char* fmt, int v)` /* 0x0049e573 */ | `sprintf(char*, const char*, ...)`, from `CRT_PROTOS`; 39 other declarations agree | `VA-SLOT` | **PORT-M20's defect.** The seat number is read as a `va_list` address; every `"%02d"` prints `00`. Fixed. |
| `llidb_odf.c:48` | `void ODFError(const char* fmt, char* name)` /* 0x0047f870 */ | `DebugPrintf(const char*, ...)`, the definition at `sysstubs.c:158`; 9 other declarations agree | `VA-SLOT` | **New, and not previously known.** Both spellings are `(i32, i32) -> void`, so the generated forwarder hands the **name pointer** over as the varargs buffer. Harmless *today* only because that body is `{}` — the day the logger forwards to `printf`, `%s` reads a `va_list` out of a string. Fixed. |
| `logflume2.c:1170`, `logflume8.c:378` | `void DebugPrint(const char* msg)` /* 0x0049e5c5 */ | `printf(const char*, ...)`, from `CRT_PROTOS` | `VA-EMPTY` | **Not a live defect**: PORT-A4 §1 typed this forwarder from libc deliberately, so clang builds the empty buffer and the call is right. Fixed anyway — a gate with an exception for "the generator happens to cope" is a gate nobody can reason about, and the declaration is a false claim about the callee either way. |

Everything else in the population is clean and was clean: `DBPrintf` (35
declarations), `DBError`, `ReportWrite`, `AddHelpMessage`, `sscanf`, `_open`,
`wsprintfA`. No known-variadic libc or Win32 name is declared non-variadically
*everywhere* (which a group-disagreement rule alone would miss): `printf`,
`sprintf`, `sscanf`, `wsprintfA`, `_open` were each checked by name against the
table, and `vsprintf` is correctly fixed — it takes a `va_list`, not `...`.

### The fixes

All four are `#ifndef LEGOLAND_PORTABLE` / `#else` / `#endif` arms around the
existing declaration, never an edit of it: the VC6 text is the caller's codegen
lever (`docs/HANDOFF.md` §3) and `mechrides.c` already used the same shape for
`ScreenToMapRef`. Per file, before and after:

| file | audit rows | relocs |
| --- | --- | --- |
| `mechrides.c` | identical, `PASS: 0 function(s) failed the extent gate` | identical, 0 MISMATCH |
| `llidb_odf.c` | identical, PASS | identical, 0 MISMATCH |
| `logflume2.c` | identical, PASS | identical, 0 MISMATCH |
| `logflume8.c` | identical, PASS | identical, 0 MISMATCH |

Byte-identical output from both gates is not a weak result here — it **is** the
thesis. The two states of this defect are indistinguishable to every byte gate
the project has, which is why the sweep had to exist.

## 4. gen_link refuses, and publishes the census

`gen_link.py` runs the sweep before it generates anything:

```
mechrides.c:3135  VA-SLOT
    declared: extern int sprintf_w(char* dst, const char* fmt, int v);
    callee:   sprintf_w(2 fixed, ...) (libc's prototype for `sprintf`)
    argument 3 lands in the slot the callee reads as its va_list POINTER --
    identical wasm signature 3 x i32, so no link gate can see it (PORT-M20)
gen_link: REFUSING to close the link -- 1 declaration(s) disagree with a variadic
callee about where its `...` starts. Nothing was generated.
```

exit **3**, zero files written. Refusing is the right verb rather than bridging:
the generator types a CRT forwarder from `CRT_PROTOS` and a game alias from the
body's wasm signature, and in a `VA-SLOT` row **both of those agree with the wrong
spelling** — so the only alternative to refusing is emitting a forwarder that
links, validates, and is wrong. `LL_ALLOW_VARIADIC_CONFLICTS=1` generates anyway
for bisecting and says so on stderr; the conflict still appears in the manifest.

Verified both ways by putting M20's declaration back on the otherwise fixed tree:
refusal exit 3 / 0 files, escape hatch exit 0 with `**conflicts: 1**` in
`gen/manifest.md`.

`gen/manifest.md` gains `## Variadic declarations`:

```
- functions whose prototype is variadic somewhere in the tree: 10
- declarations of them: 108
- **conflicts: 0** in 0 function(s)
```

followed by the table of all ten (address, names, the callee's shape, how many
declarations). Zero is the number a reader wants when asking whether a build could
be carrying this defect.

## 5. The ctests

`variadic_sweep` and `variadic_sweep_selftest` in `portable/cmake/headless.cmake`,
beside `extern_sweep`, `bvstruct_sweep` and `addr_sweep`: sources only — no
`gamedata/`, no `original/legoland.exe`, no build products — so both toolchains
and the `portable-wasm` CI job run them. Native 19 → **21**, wasm32 26 → **28**.

The selftest is 18 cases and the **negatives** are the half that matters, because
a sweep whose reader quietly stops finding declarations is a gate that always
passes:

* positives — M20's own statement verbatim; the same defect spelled without
  `extern`; a definition overriding every declaration; the reverse direction (one
  variadic spelling among fixed ones); a wrong fixed count; `()`; A4's bridged
  CRT alias;
* negatives — the `#ifndef LEGOLAND_PORTABLE` arm; PORT-M3's `_vc6_body` rename
  (split across two files, because the rename is in force only in the file that
  owns the body); an address in the prose **above** a declaration; `(void)` as
  zero parameters rather than an unspecified list; a function-pointer parameter;
  a prototype inside a function body; a struct definition at file scope; `...` and
  the word `extern` in prose; a macro body; two addresses being two functions;
* and two claims rather than shapes: that a 3-fixed declaration and a 2-fixed
  variadic one really do have the **same wasm arity** (the reason no link gate
  sees this), and that the reader still finds >4000 declarations and >50 variadic
  ones in the real tree, with at least one definition among them.

## 6. Census

`linkreport.py` over `portable/build*/CMakeFiles/legoland_core.dir`. The **before**
column is measured, not inferred: the four pre-fix files were restored, only their
four objects recompiled (no relink — `gen_link` refuses, which is the point), and
the census re-read off that symbol table.

| category | wasm32 before | wasm32 after | native after | change |
| --- | --- | --- | --- | --- |
| game-fn (CRT-range wrappers) | 12 | 12 | 12 | — |
| game-data | 2672 | 2672 | 2673 | — |
| alias (stale extern names) | 250 | 250 | 250 | — |
| host (Win32/DirectX) | 96 | 96 | 96 | — |
| crt | 44 | 44 | 58 | — |
| unknown (no address comment) | 4 | 4 | 0 | — |
| duplicates | 0 | 0 | 0 | — |
| asm stubs | 3 | 3 | 3 | — |
| prototype conflicts | 375 | 375 | n/a | — |
| **variadic declaration conflicts** | **4** | **0** | **0** | **new row** |

**Every row is unchanged**, and that is the right result: the four edits change
only the `LEGOLAND_PORTABLE` arm of a declaration. Of the four, only
`DebugPrint`'s wasm signature changes at all — `(i32) -> void` to
`(i32, i32) -> void`, which moves its forwarder from `crt_alias`'s non-variadic
spelling to its flattened one, both of which PORT-A4 built on purpose. `ODFError`
is `(i32, i32) -> void` either way and `sprintf_w` is `(i32, i32, i32) -> i32`
either way, which is the whole point of the lane.

### The generated code is the clearest statement of the defect

`gen/aliases.c` emits, and emitted **before** the fix too:

```c
extern int ll_crt_sprintf_va(char*, const char*, void*) __asm__("sprintf");
unsigned int sprintf_w(unsigned int a0, unsigned int a1, unsigned int a2)
{ return (unsigned int)(ll_crt_sprintf_va((char*)…(a0), (const char*)…(a1),
                                          (void*)…(a2))); }
```

`a2` is cast to `void*` and handed to libc as the varargs buffer, because
`gen_link` classifies an alias as "spelled variadic" when its wasm arity is libc's
fixed count **plus one** (PORT-A4 §1's table lists `sprintf_w -> sprintf` under
exactly that row). A fixed three-parameter declaration has that arity, so the
generator's own heuristic accepted the wrong spelling and the forwarder waited for
a buffer address the caller never built. Nothing about the generated text changes
with the fix — what changes is that `mechrides.c` now really does build the buffer.
That is why a link-time gate was never going to find this, and why the refusal
belongs at the declaration.

## 7. Gate table

| gate | result |
| --- | --- |
| `tools/progress.py --check` | 3281 exact / 42 WIP, 665/675 exports (98.5%) — **unchanged**. The report was stale only in the four files' line numbers and was regenerated. |
| marker set vs `origin/main` | **identical**, 3323 markers both sides (set-diff, not a count) |
| `tools/audit.py` on the four files | all PASS, rows byte-identical to before |
| `tools/relocs.py` on the four files | output byte-identical to before, 0 MISMATCH |
| `portable/tools/extern_sweep.py` | 0 multi-address extern statements |
| `portable/tools/bvstruct_sweep.py` | 0 unaccepted silent sites (1 silent, 5 noisy) |
| `tools/port_m10_bvstruct_sweep.py` | 0 silent sites, 0 slot-vs-body sites |
| `portable/tools/addr_sweep.py` | 0 failures over 1 name row and 5 address rows |
| `portable/tools/name_trap.py` | **wasm-ld warned about 0 mismatched signatures** (the `--stages` harness times out in `RunGame`'s music wait, as PORT-A4 §3 documented) |
| `portable/tools/variadic_sweep.py` | **0 conflicts**, 10 functions, 108 declarations |
| native build from `rm -rf` | clean, 291 targets; `ctest` **21/21** |
| wasm32 build from `rm -rf` | clean, 323 targets, all seven links; **0** `function signature mismatch` lines; `ctest` **28/28** |
| `tools/verify.py` | **not run** (the integrator's) |

## 8. What is owed

1. **`linkreport.scan_sources`'s marker association** (§2): a prototype line
   directly under a `// FUNCTION:` marker is taken as the body. Harmless at
   0x00420640 today; a PORT-A lane should make it require a `{`.
2. **Nothing in this class is open.** The population is 10 functions and the
   conflict count is 0, with the gate in `gen_link`, in two ctests and in CI.
   A new variadic game function, or a new alias of one, is caught the first time
   anybody builds.
3. This lane did **not** run `tools/verify.py` (the integrator's), `match.py`, or
   any browser replay: nothing it changed can reach the runtime except through the
   four portable arms, and `progress.py --check` is unchanged at 3281 exact /
   42 WIP with an identical marker set.
