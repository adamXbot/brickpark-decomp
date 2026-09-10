# Scope PORT-M2 — the 19 call-site signature mismatches, and two wrong callees

> **Status: the wasm-ld signature mismatches are down to ONE, and that one is
> not in the game** (`printf`, a `gen_link.py` defect — section 6).
> `name_trap.py` reports no prototype conflict at all: the game runs to
> `RLEPaintHit`, the unported RLE blitter, which is PORT-B3's. Branch
> `scope/PORT-M2` from `3ca919cc` (the PORT-A3 merge; `main` `f1452cfe` is an
> ancestor of it). 25 game files touched, every change an `#ifdef
> LEGOLAND_PORTABLE` arm. Census 437 -> 420 prototype conflicts, ctest 8/8.

## 1. Headline: two names that were wired to the wrong function

Both of these are **recovery findings, not port workarounds**, and both were
invisible to every existing gate — `audit.py`, `relocs.py` and `verify.py` all
pass on them, because the C named the wrong *identifier* while the address
comment and the relocation target were both correct. Only the portable link can
see them, because only the portable link resolves names to bodies.

### 1a. `NewScriptEvent` in levelkw.c and movie3.c was `AddScriptString`

| | |
| --- | --- |
| the name used | `NewScriptEvent`, with the address comment `/* 0x004689f0 */` |
| what 0x00468910 is | `ScriptEvent* NewScriptEvent(int kind, int mode)` — sysstubs.c, the ScriptEvent allocator. 8 files call it, all with two arguments |
| what 0x004689f0 is | `int AddScriptString(char* a, char* b, int copy)` — **gameframe2.c, already matched under that name**, the script/hint string intern |
| effect on the port | wasm-ld collapsed both names onto the two-parameter allocator, so `LevelKw_PROMPT` and `ResetLevelGlobals` called the wrong function |
| verdict | a real rename, for **both** builds |

The evidence is not ambiguous, and it did not need the disassembly:

* gameframe2.c's own file header already flagged the collision in prose
  ("0x004689f0 is the script-string intern, not sysstubs.c's NewScriptEvent at
  0x00468910. movie3.c / levelkw.c declare this address under that colliding
  name") — LL8 found it and left it. This lane closed it.
* movie3.c's call site is `FreeScriptStrings(); g_script_root =
  NewScriptEvent(0, 0, 0);` — freeing the string table and then re-interning its
  first (NULL) entry is `AddScriptString(0, 0, 0)` exactly, and makes no sense
  as an event allocation.
* levelkw.c's `LevelKw_PROMPT` passes `(args[1], args[2], 1)` and `(args[1], 0,
  1)` — two keyword strings and `copy=1`, which is `AddScriptString`'s
  three-argument shape field for field. The ScriptEvent allocator takes
  `(kind, mode)` and every other caller passes two small integers.

**Both spellings were byte-identical**, which is why the matcher never noticed:
the relocation target is 0x004689f0 either way and a three-dword push is a
three-dword push. The rename changes the identifier only — no types, no
arguments, no statements — so the emitted bytes cannot move (`audit.py`
confirms, section 5).

`g_script_root` (0x007fdca4) is declared `void*` in both files and receives
`AddScriptString`'s `int` index. That is a separate naming question (the global
is really a string-table index) and this lane left the spelling alone: it is a
four-byte store either way, and re-typing it is a caller-side lever that would
need its own audit.

### 1b. `DrawSupportModel` in coaster4.c was `Coaster3D_DrawModel`

Same shape, found by this lane rather than inherited. coaster4.c had

```c
extern void  DrawSupportModel(void* a, void* b, const Vec3f* p,
                              const Mat3* r, int mode);         /* 0x00420e90 */
```

but 0x00420e90 is `Coaster3D_DrawModel` (defined in coaster9.c; spelled that way
by coaster10.c, coaster13.c and coastertiny.c). `DrawSupportModel` is a
**different** function, 0x00429490 in coastertiny.c, which takes two arguments
and itself calls 0x00420e90. So the portable link pointed coaster4.c's
five-argument call at a two-argument body.

Evidence from the disassembly: `DrawSupportShadow` (0x004292f0, the body
containing the call) ends `0x00429476: call 0x420e90`, and 0x00429490's own body
is `0x004294a6: call 0x420e90`. coaster4.c's own comment 40 lines above the
declaration already said "the solid support uses (0x00420e90, two model
tables)" — the prose was right and the identifier was wrong. Renamed for both
builds; identifier only.

## 2. Struct by value against flattened ints (8 symbols)

On x86 a small struct by value is just its words pushed in order, so
`(Pos from, Pos to, void*)` and `(int fx, int fy, int tx, int ty, void*)` push
identical bytes. **On wasm32 a by-value struct is passed as a POINTER to a
copy**, so the two are different function types and wasm-ld resolves the call to
a trapping stub. In every case the definition wins — it has the body the matcher
validated — and the odd file's portable arm declares the definition's shape and
builds or unpacks the aggregate at the call.

| symbol | definition | odd file(s) | what changed in the portable arm |
| --- | --- | --- | --- |
| `CalcMoveLine` 0x00480740 | `(Pos from, Pos to, MoveLine*)` bnvmove.c | ridecb2.c, ridecb5.c | **declaration only.** Both files already call it through a cast to the by-value type (ridecb2.c's `int (__cdecl*)(Pos,Pos,void*)` local, ridecb5.c's scope-local `MoveLineFn`), so declaring the definition's shape makes those casts identities and no call site moves |
| `AddRepairOrderForObject` 0x0049b930 | `(WClass*, Pos pos)` workers2.c | scrolltick.c, workorder2.c | static `ll_add_repair_order_xy(cls, x, y)` builds the `Pos`; same-named macro routes the three existing call sites through it |
| `JungleCruise_TraceRoute` 0x00437260 | `(JcRoutePos here, JcRoutePos target, BPosW*, int*)` jcroute.c | junglecruise.c | static helper builds both aggregates from the four ints; one call site |
| `Restaurant1_WalkToSeatSpot` 0x0042f0f0 | `(Bloke*, Pos tile, int phase)` ridemisc2.c | ridecb1.c | static helper builds the tile; five call sites unchanged |
| `SetPersonPosition` 0x00440190 | `(Person*, int x, int y)` sweep1.c — the **definition** is the flattened one | logflume6.c | macro unpacks `(_pos).x, (_pos).y`. logflume6.c's by-value spelling is a documented scheduling lever (299/299) and the `#ifndef` arm keeps it |
| `LFQueue_StepFront` 0x004120a0 | `(LFQueue*, int tx, int ty)` lfmisc2.c — definition flattened | logflume4.c | macro unpacks the point; same lever, same treatment |
| `PopUpInfoSetUp` 0x00471950 | `(int type, PopUpObj* obj, int ref, Pos pos)` fpui2.c | gameframe.c, popup.c | static helper hands over the five dwords the callee actually reads: `key.type`, `key.obj`, the ref dword (gameframe.c's `HitCell.i`, popup.c's `key.ref`) and the built `Pos` |
| `PtInRect` (USER32) | `(const LLRect*, LLPoint pt)` — the host shim, `portable/src/hostwin/user32.c` | cursorseg.c | `LLPointCompat {int x,y}` declared locally + static helper; **user32.c untouched**, per the brief. `ClipRect` already has `RECT`'s layout |

Two of these rows are worth a second look by anyone auditing the port. The
`PopUpInfoSetUp` pair is the only case where neither side is "the ABI": popup.c
documents, from the instruction stream, that the original's callers emit
`sub esp,0xc` plus three stores (a struct copy) while fpui2.c's definition reads
five scalars — identical stack images, genuinely different wasm types. The
definition was taken as authoritative because it is the side with the body.

## 3. A float passed as its bit pattern (9 symbols, 11 census rows)

The game deliberately moves a float through a GPR by declaring the parameter
`int` and passing `*(int*)&value` — the caller-side lever `HANDOFF` section 3
warns about, recorded in four separate files' comments ("spelled `int` here so
the caller forwards the raw dword", "a `float` local would cost an `fld`/`fstp`
pair"). **Retyping the declaration to `float` alone would be wrong**: it would
convert the bit pattern numerically and pass a different number. Every portable
arm therefore declares the definition's type *and* moves the value across as the
same BITS, with `LL_ASFLT` / `LL_ASINT` from `ll_portable.h`.

| symbol | definition | odd file(s) | what changed in the portable arm |
| --- | --- | --- | --- |
| `Route_SetSpeed` 0x0041dad0 | `(CoasterRoute*, float v)` schoolcar.c | coaster.c, coaster8.c, schoolcar5.c, schoolcar6.c | declare `float`, macro wraps the argument in `LL_ASFLT` |
| `PositionRouteCars` 0x0041da10 | `(CoasterRoute*, float, RoutePos*)` schoolcar.c | the same four files | as above |
| `Route_SetTrainAt` 0x0041d950 | `(CoasterRoute*, float, RoutePos*)` coaster11.c | schoolcar7.c | as above; `a` there is the hook's out-parameter, filled through an `int*` |
| `TrackCurve_EvaluateOffset` 0x00429bb0 | `(RoutePos*, int mode, int t, float offset, Vec3f*)` coaster9.c | coaster13.c, coastertiny.c | the definition takes `t` as a dword; callers hold a real `float t`, so the macro passes `LL_ASINT(t)` |
| `TrackCurve_EvaluateDerivative` 0x00429c60 | `(RoutePos*, int mode, int t, float h, Vec3f*)` coaster10.c | coaster13.c (`float t`), coaster9.c (`int h`) | coaster13.c: `LL_ASINT(t)`. coaster9.c: **declaration only** — it passes the literal `0` for `h`, and `push 0` is the same dword as `0.0f`, so the prototype converts the literal itself |
| `TrackCurve_EvaluatePosition` 0x00429a80 | `(RoutePos*, int mode, int t, Vec3f*)` coaster10.c | coaster13.c | `LL_ASINT(t)` |
| `TrackCurve_EvaluateUp` 0x00429b90 | `(RoutePos*, int mode, int t, Vec3f*)` coaster10.c | coaster13.c | `LL_ASINT(t)` |
| `Track_MeasureDistance` 0x0042a1b0 | `(RoutePos*, float, RoutePos*, float, int mode, float offset)` coaster13.c | coaster7.c | **declaration only** — the one call site passes the literal `0` for the last slot |
| `TrackCurve_EvaluateTangent` / `TrackCurve_DerivSample` / `FiniteDifference` | see section 4 | coaster10.c | `LL_ASFLT` on `t`; these are *not* wasm-ld warnings |

`LL_ASFLT(x)` is `*(float*)&(x)`, so every wrapped argument has to be an lvalue.
All of them are (`src->f08`, `saved.v`, `*(int*)&value->v[1]`, `cursor->t`, a
`float` parameter), which is what makes the macro form safe; the two literal-`0`
rows are exactly the sites where it would not have been.

## 4. The defect the fix un-masked: a cast forwarder that binaryen directizes

Closing the `TrackCurve_Evaluate*` mismatches **broke the browser link**, and
the failure is worth recording in full because it is a trap anyone doing this
work again will hit.

```
[wasm-validator error in function TrackCurve_DerivSample] call param types must match
[wasm-validator error in function TrackCurve_EvaluateTangent] call param types must match
Fatal: error validating input
```

Three of coaster10.c's declarations pass the curve parameter as a raw dword
under names that **no file defines**:

| coaster10.c declares | address | the body's real name and signature |
| --- | --- | --- |
| `TrackCurve_EvaluateTangent(RoutePos*, int, int t, Vec3f*)` | 0x00429ac0 | `TrackCurve_EvalVtable(RoutePos*, int, float t, Vec3f*)` coaster13.c |
| `TrackCurve_DerivSample(int t, PhysVec*)` | 0x00429c10 | `TrackCurve_SolverSample(float t, PhysVec*)` coaster13.c |
| `FiniteDifference(void(*)(int, PhysVec*), void*, int t, float, PhysVec*)` | 0x0041f4e0 | `int Romberg_Evaluate(RombergFn, PhysOps*, float t, float, PhysVec*)` coastershade2.c |

Because the names differ, these are **stale extern names**, not prototype
conflicts: wasm-ld never warns, and `linkreport.py` files them in a different
census section. `gen_link.py` bridges a stale name with a forwarder, and when the
two signatures disagree it CASTS the function pointer:

```c
extern void TrackCurve_SolverSample(float, unsigned int);
void TrackCurve_DerivSample(unsigned int a0, unsigned int a1)
{ ((void (*)(unsigned int, unsigned int))&TrackCurve_SolverSample)(a0, a1); }
```

A cast call through a known address lowers to `call_indirect` with the cast
type, and binaryen's `directize` pass (the browser link runs it via
`--pass-arg=directize-initial-contents-immutable`) rewrites a constant-index
`call_indirect` into a **direct** call — at which point the argument types do not
match and the module is invalid.

**Why it only appeared after the fix**, which is the instructive part: at lane
start the first statement of `TrackCurve_SolverSample` was a call to
`TrackCurve_EvaluateOffset`, a live signature mismatch, so wasm-ld replaced it
with a `signature_mismatch:` stub whose body is `unreachable` and link-time
`-O2` inlined that `unreachable` into the body. A body that is all-`unreachable`
masked the type error downstream of it. Fixing the mismatch removed the poison
and exposed a defect that was already there and would have trapped at runtime
anyway. The lesson for the next lane: **a clean wasm-ld run is not a valid
module**, and `unreachable` upstream can hide a type error downstream.

The fix is the same as section 3 — coaster10.c's portable arm declares the three
definitions' real types (including `Romberg_Evaluate`'s `int` return, since a
forwarder with a mismatched *return* type directizes into an invalid call too)
and wraps `t` in `LL_ASFLT`. With the types identical the generator emits a
plain forwarder with no cast, and the module validates.

**For PORT-A / the generator lane:** the casting forwarder in `fn_alias` is not
safe on wasm. A signature disagreement between a stale name and its body is a
*game* defect and should be reported (it is, in the census) rather than bridged
with a cast that the optimiser turns into an invalid direct call. Either emit a
genuine conversion (the generator knows both signatures) or emit a trap, so the
defect names itself instead of failing validation 400 functions away.

## 5. The one remaining mismatch: `printf` is a `gen_link.py` defect

PORT-M1 filed this as generator-side and this lane confirms it: **nothing in
`LEGOLAND/*.c` can fix it.** The exact defect, from the generated file:

```c
/* portable/build-wasm/gen-browser/aliases.c, lines 6-7 */
extern void printf(unsigned int);
void DebugPrint(unsigned int a0) { printf(a0); }
```

`DebugPrint` (logflume2.c, logflume8.c, declared `void DebugPrint(const char*)`)
is a second name for 0x0049e5c5, which bigrender.c / printlist.c / unref7.c
declare as the CRT's `printf`. `gen_link.py` emits the forwarder through the
alias machinery, and in

```python
rsig = def_sig_of(real) or sig        # gen_link.py, the function-aliases loop
```

`def_sig_of('printf')` is `None` (no game translation unit defines it), so `real`
is declared with **`sig` — the signature of the ALIAS**, i.e. `DebugPrint`'s one
parameter and `void` return. libc's `printf` is `(i32, i32) -> i32` on wasm32
(format pointer plus the varargs pointer), so wasm-ld warns and every call
through that declaration goes to a trapping stub.

**Proposed fix:** when `real` is a CRT name (`lr.is_crt(real)` is already
available) and no game TU defines it, the declaration must come from the CRT, not
from the alias — either a small built-in prototype table for the names in
`linkreport`'s CRT list, or simply `#include <stdio.h>`/`<string.h>` in
`aliases.c` (it is generated host code, not a game source, so the
"no libc headers" rule in `ll_portable.h` does not apply to it) and no
hand-rolled `extern` at all. Calling the real variadic prototype is also the only
way to get the *semantics* right: a wasm variadic callee takes its arguments
through a second pointer, which clang supplies only when the call goes through
the variadic declaration.

Two notes so the fix is not over-applied:

* `sprintf` and `Format`/`sprintf_w` are **accidentally correct** and must stay
  working. `Format` is declared variadic in the sources (`int Format(char*,
  const char*, ...)`), so its wasm signature is `(i32, i32, i32) -> i32` — dst,
  fmt and the varargs pointer — which is exactly libc's `sprintf`, and the
  forwarder passes the varargs pointer straight through. `printf` fails only
  because `DebugPrint`'s *non-variadic* one-argument spelling gives the wrong
  arity.
* `time` is already closed: PORT-M1 widened render5.c to emscripten's 64-bit
  `time_t`.

## 6. Gate results

Per the brief: `audit.py` on every touched file shows the same `[OK`/`[WIP` rows
with no REJECT/FAIL/COMPILE FAILED; `relocs.py` prints no MISMATCH;
`progress.py --check` reports 3281 exact / 42 WIP.

**Every one of the 25 files: `PASS`, zero REJECT / FAIL / COMPILE FAILED, zero
`relocs.py` MISMATCH, zero `/W3` warnings.** 366 `[OK]` rows and 6 `[WIP]` rows
in total, and both sets are byte-for-byte what they were at lane start — no
change in this lane moves a single emitted byte, which is what the portable-arm
discipline is for.

| file | `[OK]` | `[WIP]` | audit.py | relocs.py MISMATCH | `/W3` |
| --- | --- | --- | --- | --- | --- |
| coaster.c | 59 | 0 | PASS | 0 | clean |
| coaster10.c | 16 | 0 | PASS | 0 | clean |
| coaster13.c | 16 | 1 | PASS | 0 | clean |
| coaster4.c | 7 | 0 | PASS | 0 | clean |
| coaster7.c | 10 | 0 | PASS | 0 | clean |
| coaster8.c | 12 | 0 | PASS | 0 | clean |
| coaster9.c | 26 | 0 | PASS | 0 | clean |
| coastertiny.c | 47 | 0 | PASS | 0 | clean |
| cursorseg.c | 0 | 2 | PASS | 0 | clean |
| gameframe.c | 10 | 0 | PASS | 0 | clean |
| gameframe2.c | 13 | 0 | PASS | 0 | clean |
| junglecruise.c | 15 | 0 | PASS | 0 | clean |
| levelkw.c | 48 | 0 | PASS | 0 | clean |
| logflume4.c | 3 | 0 | PASS | 0 | clean |
| logflume6.c | 7 | 0 | PASS | 0 | clean |
| movie3.c | 10 | 0 | PASS | 0 | clean |
| popup.c | 2 | 1 | PASS | 0 | clean |
| ridecb1.c | 6 | 0 | PASS | 0 | clean |
| ridecb2.c | 4 | 1 | PASS | 0 | clean |
| ridecb5.c | 10 | 1 | PASS | 0 | clean |
| schoolcar5.c | 5 | 0 | PASS | 0 | clean |
| schoolcar6.c | 5 | 0 | PASS | 0 | clean |
| schoolcar7.c | 7 | 0 | PASS | 0 | clean |
| scrolltick.c | 5 | 0 | PASS | 0 | clean |
| workorder2.c | 23 | 0 | PASS | 0 | clean |

Tree-wide, with the marker SETS diffed against the lane's base `3ca919cc` (the
`comm -23` check from `HANDOFF` section 4, which is the only one that sees a
silent un-closing):

```
base exact: 3281   worktree exact: 3281
base wip:     42   worktree wip:     42
--- UN-CLOSED (base exact not in worktree; must be empty):   (empty)
--- NEW exact (worktree not in base):                        (empty)
--- WIP set diff:                                            (empty)
--- duplicate addresses tree-wide:                           (empty)
```

`progress.py --check` reported `docs/LEGOLANDPROGRESS.HTML` stale, for the
reason PORT-M1 predicted: the report records a line number per function and an
inserted `#ifndef` moves them. Regenerated and committed; it reports
**665/675 exports exact (98.5%), 3281 exact, 42 WIP** and `--check` is clean.
The only change in the file is line numbers in the source links.

A note on the shared scratchpad, because it cost this lane twenty minutes:
`tools/` and the worktrees are isolated but the session scratchpad directory is
**not** — a parallel lane wrote its own `gate.sh` to the same path and my
`exec`'d gate ran *its* script against *its* worktree, reporting a green gate for
files this lane never touched. Name gate scripts per lane.

## 7. Census and state of the port

| | at lane start (3ca919cc) | after |
| --- | --- | --- |
| wasm-ld `function signature mismatch` (distinct symbols) | 19 | **1** (`printf`, generator-side) |
| `linkreport.py` prototype conflicts | 437 | **420** — 18 rows closed, none added |
| `legoland_browser` link | links, module would fail wasm validation once the mismatches are closed (section 4) | links and validates |
| native `ninja -C portable/build` | builds | builds |
| `legoland_linkcheck`, both trees | links | links; node prints "every symbol resolved" |
| `ctest` | 8/8 | 8/8 |
| `name_trap.py` verdict | `signature_mismatch:` frames | **no prototype conflict** |

The browser page has full parity with the headless harness: served from
`portable/build-wasm`, `legoland.html?trace=1` replays the whole host trace
(version block, mutex, `DirectDrawCreate`, the CD check, all three RES volumes
and their directories, window, four fonts, `SetDisplayMode 640x480 16bpp
RGB565`, three surfaces, both DirectInput devices, the eight title sprites and
then the 343,366-byte title artwork) and stops at the same `rlepaint.c:1016`
abort — no unnamed `unreachable`, no unattributed trap.

The 420 rows that remain are the ones PORT-M1 described and they are not
reachable traps:

* **address-taken callbacks** — the large majority. Every `screen.c` table
  entry, and the `WaterBlock_*` / `WWEntrance_*` / `ZebraCrossing_*` /
  `TrackCurve_Cubic*` / `Track*_Update` families declared `void Foo()` in the
  file that stores them in a table. The table holds the real function, so the
  call goes through `call_indirect` with the *definition's* type and nothing
  traps through the link. They should be closed by a typed-callback pass, not
  one arm at a time.
* **dead or unreached** — the `VolUpInput`/`VolDownInput`/`VolMarkerInput`/
  `WesternThemeInput` screens2.c/bigscreens.c rows and the `unref*.c` families.
  Real conflicts, but no live call site; the census lists them because it
  compares declarations, not calls.

`name_trap.py` now stops at the same place PORT-M1 left it and for the same
reason — `RLEPaintHit`, `LEGOLAND/rlepaint.c:1016`, reached through
`ShowTitleScreen -> PrintSprite -> RenderSprite -> SoftBlitSprite ->
SoftBlitRLEPlain`, after the 343,366-byte title artwork loads out of
`Graphics1.res`. That is the unported RLE blitter and it belongs to **PORT-B3**,
which this lane did not touch (rlepaint.c, rlepaint2.c, softblit.c, softblit2.c
are its files). With the prototype layer now clean, the blitter is the only
thing between the port and a picture on the canvas.

One item this lane opens for a follow-up, and did NOT fix because it is a
different defect class with a different shape: **function-pointer casts that
disagree with the real callee**, of which section 4 is one instance. The
geometry vtable is the live example — coaster10.c calls
`geom->vt[...]` through `(void (__cdecl **)(void*, int, Vec3f*))` while the
slots hold bodies like `TrackCurve_CubicPosition(i32, f32, i32)`. wasm-ld cannot
see a cast function pointer, so there is no warning; the call traps at runtime
with an indirect-call type mismatch. It needs a typed-callback pass over the
vtables, and it is the same pass the 420 address-taken census rows want.
