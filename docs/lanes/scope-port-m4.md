# Scope PORT-M4 — the externs with no address comment

> **Status: READY (2026-09-12).** Branch `scope/PORT-M4` from `0e0b2b76` (the
> PORT-B4 merge). Every name in `linkreport.py`'s **Unclassified** bucket is
> resolved: native census **81 -> 2**, and the two survivors are classification
> gaps in `portable/` (section 6), not game defects. 30 files under
> `LEGOLAND/`, every one VC6-gated with the same `[OK]`/`[WIP` rows as
> `0e0b2b76`; 3281 exact / 42 WIP unchanged and the marker SET identical. The
> clean wasm32 tree links and validates all four targets, `ctest` 10/10, and
> with section 6's two one-token fixes the page reaches **PLAYER DETAILS at
> 33.3 fps with no trap-continue patch** and **zero GAME traps left** — the six
> that remain are PORT-B's unwritten AVIFIL32 stubs.

## 1. The method

`linkreport.py` classifies an undefined symbol by the address comment on its
`extern`. With no comment it cannot tell a stale alias from unwritten work, so
it lands in *Unclassified* and `gen_link.py` emits a trap. The frontier is
closed (3281 exact + 42 WIP, every function in the game-code range has a body),
so an Unclassified **function** name is always one of:

* a second name for a function that IS defined under another spelling, or
* an address in the CRT / import range (the twelve CRT thunks were this).

The evidence is the relocation the original binary carries at the call site.
`tools/relocs.py` normally *resolves* these names out of `symbols/legoland.exports.txt`
and `docs/DECOMP.md`, so they never appear as `UNRESOLVED`. This lane forced
them unresolved with a ten-line monkeypatch over `relocs.symbol_address`
(`port-m4-probe.py` in the scratchpad) so that every relocation naming one of
them printed `original=0x...` — the address the *original* binary uses at that
exact instruction. That address's `// FUNCTION:` marker names the real body.

Nothing in this lane changes a byte: an address comment is a comment, a
whitespace reflow is whitespace, and an identifier rename with the same types
emits the same instruction. Both arms of the build therefore get the same edit,
with ONE exception — the two declarations of section 4d, where annotating the
extern exposed a pre-existing float-versus-int parameter disagreement that only
wasm32 can see, and whose fix lives in an `#ifdef LEGOLAND_PORTABLE` arm.

## 2. What "Unclassified" actually was

81 names, in four causes. Only ONE of them was a missing address — the other
three were declarations whose correct address comment the scanners could not
read, which matters because the failure is silent and asymmetric:

| cause | what it looks like | how many | what gen_link did with it |
| --- | --- | --- | --- |
| 1 | the `/* 0x... */` sits on a **continuation line** of a multi-line `extern` | 20 | trap / placeholder |
| 2 | the declaration does not start with **`extern`** | 2 | trap |
| 3 | the type is a **function pointer** (`extern int (*g)(void)`) or an **in-line aggregate** (`extern struct {int a; ...} g;`) | 7 | zeroed placeholder block |
| 4 | there really is **no address comment** | 52 | trap / placeholder |

`linkreport.py`'s scanner is one regular expression applied per line:

```python
extern_re = re.compile(r'^\s*extern\b[^;]*?\b([A-Za-z_][A-Za-z0-9_]*)\s*(\(|\[|;|=).*?/\*\s*(0x[0-9a-fA-F]+)')
```

Cause 1 fails because the comment is not on the line. Cause 2 fails on the
`^\s*extern\b` anchor. Cause 3 fails on the capture: in `extern int
(*g_present)(void);` the lazy `[^;]*?` stops at `int`, which is followed by
`(`, so the **type token** is captured as the symbol name; and in `extern
struct FortArea { int x0, y0, x1, y1; } g_fort_area;` the `[^;]` class cannot
cross the struct's own semicolon.

**Cause 3 is the dangerous one.** For an undefined name it cannot classify,
`gen_link.py` emits a trapping stub if wasm says the name is used as a
function and, if wasm says it is used as DATA, a **zeroed placeholder block**:

```python
stubs_c.append(f'__attribute__((aligned(16))) unsigned char {n}[{UNKNOWN_DATA_SIZE}];')
```

That links, runs, and is wrong: the global is not the one the exe has, and if
another file names the SAME address under a spelling the scanner *can* read,
the program ends up with two objects for one global. `profiles.c`'s
`g_active_input_cb` and the `g_icon_handler2` of appraisal.c / bigscreens.c /
fpui.c / mapscreen3.c / sysstubs.c are exactly that pair at 0x006687c0, which
is why this lane looked at it as a candidate for PORT-B4's "input arrives from
the host but the front end ignores it". See section 6 — it is fixed, and it was
not the cause.

## 3. The name table

Every `original address` below is what the ORIGINAL binary's relocation points
at, printed by `relocs.py` with `symbol_address` forced to fail for these names
(`port-m4-probe.py`); every `what it really is` is the `// FUNCTION:` marker at
that address. `cause` is the column of section 2.

| name | declared in | original address | what it really is | fix | cause |
| --- | --- | --- | --- | --- | --- |
| `ODFError` | llidb_odf.c | `0x0047f870` | DebugPrintf (sysstubs.c) — the empty varargs logger | address comment; the fixed 2-parameter spelling kept (HANDOFF §3) | 4 |
| `ObjDefFinalize` | llidb_odf.c | `0x00480aa0` | SetWaterWorksClassOrigins (pathobj2.c) | address comment | 4 |
| `g_813a10` | llidb_odf.c | `0x00813a10` | g_debug_heap_bytes (memdb.c) | renamed | 4 |
| `g_80ff74` | llidb_odf.c | `0x0080ff74` | NEWFLC_AutoPlay (export table) | renamed + comment | 4 |
| `g_80ff78` | llidb_odf.c | `0x0080ff78` | NEWFLC_PauseType (export table) | renamed + comment | 4 |
| `WindowProc` | screen.c | `0x0047fe90` | LegoLandWindowProc (input2.c), exported `_LegoLandWindowProc@16` | renamed; comment reflowed onto the declaration line | 1 |
| `g_present` | screen.c, render2.c, startup.c | `0x004b9ca4` | the live page-flip hook | PresentFn typedef | 3 |
| `SetVidAnim` | screens3.c | `0x00443dc0` | StartAdvisorClip (advisor.c) — but the debug string at 0x004b7dc4 IS "SetVidAnim" | address comment | 4 |
| `ShowHelpString` | screens3.c | `0x0046d230` | ShowIdHelp (uimisc.c) | address comment | 4 |
| `PlayTitleMovie` | screens3.c | `0x0048f9f0` | SaveFrontEndState (frontend2.c) | address comment | 4 |
| `ResumeGameTimer` | screens3.c | `0x004993c0` | ThawGameClock (uimisc.c) | address comment | 4 |
| `EndScript` | screens3.c | `0x0046b700` | ShowStepHint (eventmake.c) | address comment | 4 |
| `ClearObjectMenuIcons` | screens3.c | `0x00474750` | CloseActiveThemeButton (popupmisc.c) | address comment | 4 |
| `SelectProfileSlot` | screens3.c | `0x0048a800` | ResetFreePlayTable (frontend2.c) | address comment | 4 |
| `sub_498cf0` | screens3.c | `0x00498cf0` | IsNarrationPlaying (tinystubs.c) | renamed | 4 |
| `sub_482a80` | screens3.c | `0x00482a80` | ResetEntranceTile (pathobj2.c) | renamed | 4 |
| `sub_48e3d0` | screens3.c | `0x0048e3d0` | SetTempProfileName (frontend2.c) | renamed, `void*` parameter kept | 4 |
| `sub_459820` | screens3.c | `0x00459820` | EndLevel (uimisc.c) | renamed | 4 |
| `ApplyMoodEvent` | rides.c | `0x00482df0` | AdjustMood (simcore2.c) | address comment | 4 |
| `GetNthRider` | rides.c | `0x004418c0` | GetObjRiderN (savemisc2.c) | address comment | 4 |
| `FindFirstRider` | rides.c | `0x00441870` | ObjFirstRider (texture.c) | address comment | 4 |
| `FindNextRider` | rides.c | `0x00441890` | ObjNextRider (texture.c) | address comment | 4 |
| `BoatingSchool_AddTake_I` | ridecb6.c | `0x0041b0d0` | BoatingSchool_AddTake (same file) | `extern` keyword added | 2 |
| `SavedGame_48d470` | bigscreens.c | `0x0048d470` | SaveSavedGameIconHandlers (frontend2.c) | renamed | 4 |
| `ClipPolygonPlanes` | unref1.c | `0x0041f2b0` | Raster_ClipAgainstPlanes (coaster11.c) | reflowed to one line | 1 |
| `PhysVec_DerivativeTable` | unref1.c | `0x0041f4e0` | Romberg_Evaluate (coastershade2.c) | reflowed | 1 |
| `Coaster3D_DrawLine` | unref3.c | `0x004237f0` | Raster_DrawLine (unref2.c) | reflowed | 1 |
| `DrawWrappedText` | unref4.c | `0x00455220` | PrintWrappedTextOnSurface (unref5.c) | reflowed | 1 |
| `Curve_InitLine` | coaster7.c, coaster12.c | `0x00421ab0` | TrackCurve_InitLine (castletrack2.c) | reflowed | 1 |
| `Curve_InitCorner` | coaster7.c | `0x00421ce0` | TrackCurve_InitArc (castletrack2.c) | reflowed | 1 |
| `Piece_InitCurve` | coaster3d.c | `0x00421ce0` | TrackCurve_InitArc (castletrack2.c) | reflowed | 1 |
| `Piece_InitStraight` | coaster3d.c | `0x00428350` | Piece_InitStraight (coaster12.c) | reflowed | 1 |
| `LFPath_StepBloke` | lfentrance.c | `0x00412300` | WalkPath_Advance (goldrush3.c) | reflowed | 1 |
| `LFPiece_GetFootprint` | logflume.c | `0x0040d090` | LFPiece_QueryRect (logflume2.c) | reflowed | 1 |
| `Romberg_Build` | coastershade2.c | `0x0041f3e0` | Span_FillEvalTable (coaster11.c) | reflowed | 1 |
| `Span_EvalRange` | coaster11.c | `0x0041f4e0` | Romberg_Evaluate (coastershade2.c) | reflowed | 1 |
| `Sub_429f30` | coaster11.c, schoolcar.c, schoolcar4.c | `0x00429f30` | Track_StepAlong (coaster13.c) | reflowed (name left: see section 4c) | 1 |
| `StandardRemoveObject_W` | screencb.c | `0x0045f220` | StandardRemoveObject (objmap2.c) | reflowed; `unsigned short tile` kept | 1 |
| `StandardRemoveObject_B` | screencb.c | `0x0045f220` | StandardRemoveObject (objmap2.c) | reflowed; `BPosW tile` kept | 1 |
| `TrackGeom_BuildRamp` | coaster13.c | `0x00422180` | TrackCurve_InitCubic (castletrack2.c) | reflowed | 1 |
| `g_active_input_cb` | profiles.c | `0x006687c0` | the slot 4 files call g_icon_handler2 | IconInputFn typedef | 3 |
| `g_find_bracket` | unref3.c | `0x004b63fc` | the module's bisection hook slot | FindBracketFn typedef | 3 |
| `g_track_solver` | coaster13.c | `0x004b63fc` | the same hook slot | TrackSolverFn typedef | 3 |
| `g_lt_action_handlers` | blokeai.c | `0x004b8368` | the 26-entry AI action table | LtActionFn typedef | 3 |
| `g_report_setters` | eventtick.c | `0x004b7e38` | the 25 REPORT setters | ReportSetterFn typedef | 3 |
| `g_fort_area` | goldrush2.c | `0x004b4580` | the fort interior rectangle | FortArea typedef hoisted out | 3 |
| `g_lls_* (31)` | bighelp.c | `0x004baa7c..0x004baca4` | the pop-up sprite-name strings; 0x004bab78/0x004bab8c are g_lls_pu_close / _on | one annotated extern each; those two renamed | 4 |
| `g_lls_playing` | layervis.c | `0x006691ac` | head of the playing-LLS list | address comment | 4 |
| `g_new_profile_popup_lls` | saveprof.c | `0x004bf6e4` | the popup background sprite name | address comment | 4 |
| `g_llidb_capacity` | data2.c, llidb.c, memdb.c (declared in legoland.h) | `0x006691a0` | the LLIDB page-table capacity | annotated extern added in unref7.c beside its two siblings | 4 |
| `timeGetTime` | sysmisc.c, blitmisc.c | `WINMM import slot 0x004ab1f8` | implemented by portable/src/hostwin/winmm.c | RESIDUE — missing from win32_imports.txt | - |
| `wcscpy` | music.c, musicthread.c, unref7.c | `0x004a0833` | the CRT thunk; libc provides the body | RESIDUE — missing from linkreport's CRT set | - |

## 4. Findings the integrator should read

**4a. Four name disagreements this lane did NOT resolve, only recorded.** The
byte evidence proves the ADDRESS, never the name, and in four cases the caller's
prose and the definition's identifier describe different things. Both spellings
are kept (the extern gains the address comment, `gen_link` aliases them), and
somebody who can read the body should decide which name is right:

| address | screens3.c's prose | the defined name | note |
| --- | --- | --- | --- |
| 0x0046b700 | "ends the running script" (`EndScript`) | `ShowStepHint` (eventmake.c) | called from `ScriptEndIconInput` |
| 0x0048a800 | "re-reads the selected profile into CurProfile" (`SelectProfileSlot`) | `ResetFreePlayTable` (frontend2.c) | called from `ProfileSlotInput` |
| 0x0048f9f0 | "starts the intro movie from the three title blocks" (`PlayTitleMovie`) | `SaveFrontEndState` (frontend2.c) | called from `TitleMovieInput` |
| 0x00474750 | "drops the side panel's object icons" (`ClearObjectMenuIcons`) | `CloseActiveThemeButton` (popupmisc.c) | called from all four `*ThemeInput` |

The one where the ORIGINAL settles it is `SetVidAnim` 0x00443dc0, defined as
`StartAdvisorClip` in advisor.c: screens3.c declares `extern const char
g_dbg_setvidanim[]; /* 0x004b7dc4 "SetVidAnim" */` — a **debug string in the
shipped .rdata**, next to "AVI GetFrame" / "BltAdvisor" / "Exit Advisor". The
game's own name for 0x00443dc0 is `SetVidAnim`; `StartAdvisorClip` is ours.
Renaming advisor.c is a one-identifier change for a later lane.

**4b. `ODFError` is not an error path, and `ObjDefFinalize` is not a finalize.**
0x0047f870 is sysstubs.c's `DebugPrintf`, an EMPTY varargs logger, and its three
call sites fire whenever an object class's ODF record has no sprite / icon /
build-anim name — which many classes legitimately have, with the fallback
(`LoadSprite("InstituteIcon.lls", 4)`) on the next line. 0x00480aa0 is
pathobj2.c's `SetWaterWorksClassOrigins`: the "finalize the ObjDef" tail call is,
in the shipped build, only the Water Works origin patch. Both externs keep this
translation unit's parameter types (HANDOFF §3) and gain the address comment;
`ODFError`'s two fixed parameters against `DebugPrintf`'s `(const char*, ...)`
push the same two dwords on x86 cdecl and `gen_link`'s forwarder bridges the
rest.

**4c. `Sub_429f30`'s callers are a documented lever, so the placeholder name
stayed.** 0x00429f30 is coaster13.c's `Track_StepAlong`. coaster11.c,
schoolcar.c and schoolcar4.c each declare it with a `float step` / `float tol`
signature that matches the definition, so nothing needed guarding; the name is
still `Sub_429f30` because renaming it would touch three matched bodies for no
gate benefit. The address comment is what the tooling needs.

**4d. The one signature conflict this lane created, and closed.** Annotating
`Curve_InitCorner` (coaster7.c) and `Piece_InitCurve` (coaster3d.c) moved them
from gen_link's trap list to its ALIAS list, and the forwarder then failed
binaryen's validator:

```
[wasm-validator error in function Curve_InitCorner] call param types must
match, on (call $TrackCurve_InitArc ...) (on argument 4) / (on argument 5)
```

castletrack2.c defines 0x00421ce0 as `TrackCurve_InitArc(..., int r0, int r1)`
and stores both with `*(int*)&curve->r0 = r0`: the parameter carries a float's
BIT PATTERN through a GPR. On x86 cdecl `float` and `int` push the same four
bytes; on wasm32 they are different function types, and `fn_alias`'s cast
forwarder becomes a direct call with mismatched params once binaryen directizes
it. Fixed with PORT-M2's pattern inverted — the `#else` arm declares the
definition's `int` shape and passes the same bits (`LL_ASINT` in coaster3d.c,
whose `r0`/`r1` are float lvalues; a four-byte memcpy helper in coaster7.c,
whose arguments are the literals `1.5f` / `0.5f` and have no address).

**Anyone annotating more externs should expect this class**: a name that was
only ever a trap has never had its signature checked against the definition.
`linkreport`'s `prototype conflicts` row and a wasm build are the two things
that see it.

## 5. Measurements

### 5a. Census

| | before (0e0b2b76) | after |
| --- | --- | --- |
| native `unknown` | **81** | **2** |
| native `alias` | 228 | 257 |
| native `game-data` | 2617 | 2658 |
| native `game-fn` | 12 | 12 |
| exact functions / WIP | 3281 / 42 | 3281 / 42 |

The residue of 2, `timeGetTime` and `wcscpy`, are **classification gaps in
`portable/`, not game defects** — see section 6.

### 5b. The wasm tree, from a clean directory

`emcmake cmake -S portable -B portable/build-wasm -G Ninja
-DCMAKE_BUILD_TYPE=Release -DLL_ILP32=ON` then `ninja`, then `ninja
legoland_headless legoland_tests legoland_pathtest legoland_browser`:
**all four link and validate, 0 wasm-validator errors**; `ctest` **10/10**.

### 5c. The headless harness and the page

**As committed**, both the headless run and the page stop at the same place:
`TRAP GAME lrintf from bnvpath.c, coaster10.c, coaster12.c...`, the page after
two frames with the LEGOLAND title art up. That is section 6's residue and it
is not a game defect — `lrintf` is a libm function that nothing in
`LEGOLAND/*.c` names at all. It was already on PORT-B4's blocker list; it
surfaces first now only because `ODFError`, which used to stop the run before
it, is resolved.

**With section 6's two one-token fixes applied to `portable/` (measured, then
reverted — nothing in `portable/` is committed by this lane)** the headless log
goes 24 KB -> 76 KB, past the whole ODF load, and the page reaches
**PLAYER DETAILS at 33.3 fps (avg 33.4)** — the screen PORT-B4 reached, now on
main's code with **no trap-continue patch of any kind**, and with the advisor
character rendered in the bottom-right corner (`SetVidAnim` used to BE a trap,
so the advisor clip was never attempted; it now resolves to advisor.c's
`StartAdvisorClip`). The run then stops at `TRAP AVIFIL32.dll AVIFileInit from
advisor.c, movie.c` — a HOST stub, the first non-game blocker left.

With `LL_TRAP_CONTINUE=1` on top, so that every blocker shows in one run, the
**complete** list of what is still trapping is six host stubs and **zero GAME
names**:

```
TRAP AVIFIL32.dll AVIFileInit          from advisor.c, movie.c
TRAP AVIFIL32.dll AVIFileOpenA         from advisor.c, movie.c
TRAP AVIFIL32.dll AVIFileInfoA         from advisor.c, movie.c
TRAP AVIFIL32.dll AVIFileRelease       from advisor.c, movie.c
TRAP AVIFIL32.dll AVIFileExit          from advisor.c, movie.c
TRAP AVIFIL32.dll AVIStreamGetFrameOpen from advisor.c, movie.c
```

That is PORT-B's AVIFIL32 stub (`SCOPE_PORT_WAVE.md`: "AVIFIL32 16 (Indeo 5
intros — stub)") never having been written; the host also refuses
`LoadLibraryA("Ir50_32.dll")`. **Making `AVIFileInit` return a failure the game
tolerates is the next single blocker on the front-end path.**

### 5d. PORT-B4's input bug is NOT the g_active_input_cb split

This lane fixed the split (§2 cause 3) and then re-measured: clicking a profile
slot on the PLAYER DETAILS screen still does nothing, and the game-drawn cursor
stays in the canvas's top-left corner rather than following the pointer. So the
mouse POSITION is not reaching the game either, which points at the DirectInput
/ `GetDeviceState` side (PORT-B's `dinput.c`), not at a split global. Recorded
so the next lane does not re-open this one.

## 6. Residue — two one-token fixes in PORT-A's files

This lane must not edit `portable/**`, so these are recorded, not committed.
Both were verified by applying them, rebuilding and running (§5c), then
reverting; `git diff -- portable` is empty on this branch.

1. **`portable/tools/linkreport.py`** — the `CRT` name set is missing the C99
   rounding family and the wide-string functions. `lrint` / `lrintf` are the
   worst of these: nothing in `LEGOLAND/*.c` names them at all, they come out of
   `LL_FISTP` / `LL_FISTPD` in `portable/hostwin/include/ll_portable.h`
   (`__builtin_lrintf` / `__builtin_lrint`), which clang lowers to a libm CALL on
   wasm32 and to an instruction on x86-64 — so they are undefined ONLY in the
   wasm build, `gen_link` emits trapping stubs that SHADOW libm's real bodies,
   and the front end dies on the first one. Add to the `CRT` string:
   ```
   lrint lrintf lrintl llrint llrintf rint rintf nearbyint nearbyintf
   round roundf trunc truncf wcscpy wcslen wcscmp wcsncpy wcscat
   ```
2. **`portable/tools/win32_imports.txt`** — `timeGetTime` is missing (only 4 of
   the 5 WINMM names are listed) although `portable/src/hostwin/winmm.c` really
   implements it and sysmisc.c/blitmisc.c declare it `__declspec(dllimport)`.
   Add `timeGetTime\tWINMM.dll`.

Two further `linkreport.py` improvements this lane worked around by hand rather
than fixing, because the file is PORT-A's. They are what caused causes 1–3 in
section 2, and doing them would stop the whole class recurring:

3. Make the extern scan **statement-oriented instead of line-oriented** (join
   physical lines up to the `;` before matching), which removes cause 1 — 20 of
   the 81 names — and lets declarations keep their normal formatting.
4. Take the **LAST** identifier before the `;` rather than the first token
   before a `(`, and allow a brace-enclosed type body. That removes cause 3,
   the one that silently manufactures a second object for a live global.
5. Also glob `LEGOLAND/*.h`: `g_llidb_capacity`, `g_llidb_count` and
   `g_llidb_pages` are declared in `legoland.h`, which the scanners never read
   (this lane annotated them in `unref7.c` instead).

## 7. Files touched, and the PORT-M3 overlap

30 files under `LEGOLAND/`, plus `docs/SCOPE_PORT_WAVE.md`,
`docs/lanes/scope-port-m4.md` and the regenerated `docs/LEGOLANDPROGRESS.HTML`.

> **`LEGOLAND/screen.c` is on PORT-M3's list** (callback-table registrations).
> This lane touched **three lines** of it, all in declarations, because
> `WindowProc` is the window procedure `RegisterClassExA` installs and PORT-B4
> named it the front end's first blocker:
> line 1284 `g_present` through a `PresentFn` typedef, line 1311-1314
> `WindowProc` -> `LegoLandWindowProc` on one line, line 1338 the one use.
> Nothing else in screen.c changed. `blitmisc.c` (PORT-B5's) was NOT touched:
> its `timeGetTime` needs no game-side edit, only section 6's item 2.

Per-file gate (`audit.py` [OK]/[WIP] identical to `0e0b2b76`, no
REJECT/FAIL/COMPILE FAILED; `relocs.py` 0 MISMATCH and 0 SKIPPED; `/W3` clean):

| file | [OK] | [WIP] | file | [OK] | [WIP] |
| --- | --- | --- | --- | --- | --- |
| bighelp.c | 3 | 0 | logflume.c | 83 | 0 |
| bigscreens.c | 5 | 0 | profiles.c | 18 | 0 |
| blokeai.c | 12 | 0 | render2.c | 8 | 0 |
| coaster11.c | 17 | 2 | ridecb6.c | 15 | 0 |
| coaster12.c | 22 | 2 | rides.c | 13 | 0 |
| coaster13.c | 16 | 1 | saveprof.c | 7 | 0 |
| coaster3d.c | 3 | 2 | schoolcar.c | 49 | 0 |
| coaster7.c | 10 | 0 | schoolcar4.c | 7 | 0 |
| coastershade2.c | 8 | 0 | screen.c | 3 | 0 |
| eventtick.c | 46 | 0 | screencb.c | 15 | 0 |
| goldrush2.c | 2 | 0 | screens3.c | 73 | 0 |
| layervis.c | 7 | 0 | startup.c | 7 | 0 |
| lfentrance.c | 8 | 0 | unref1.c | 30 | 1 |
| llidb_odf.c | 1 | 0 | unref3.c | 16 | 0 |
| | | | unref4.c | 10 | 1 |
| | | | unref7.c | 44 | 0 |

Whole-tree invariants: `progress.py --check` **3281 exact / 42 WIP**, unchanged;
the exact-marker SET at `0e0b2b76` equals the working tree's (3281 both ways,
`comm -23` prints nothing — the HANDOFF §4 "a count is not a set" check); no
duplicated `// FUNCTION:` address.

`llidb_odf.c`'s relocation agreement is the sharpest single piece of evidence
that the addresses are right: `matched` went **59 -> 69** and `unresolved`
**32 -> 22**. The ten relocations for `ODFError`, `ObjDefFinalize`,
`g_debug_heap_bytes`, `NEWFLC_AutoPlay` and `NEWFLC_PauseType` now RESOLVE and
equal the original binary's own target at the same instruction.

## 8. Scratch

`port-m4-*` in the shared scratchpad. The one worth keeping is
`port-m4-probe.py`: it imports `tools/relocs.py`, replaces `symbol_address`
with a version that returns `(None, 'PROBE')` for a named set of symbols, and
re-runs `main()`. Every relocation naming one of them then prints
`original=0x...`, which is the address the original binary uses at that exact
instruction. It is the general answer to "what is this extern really?" and it
needs no disassembly reading.
