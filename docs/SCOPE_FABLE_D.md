# Parallel session scope — "fable-d" (2026-09-05)

Fourth self-contained brief for the Fable session. Scope C came back **26 of
26 exact** and merged with zero conflicts, as did A and B. Same workflow, new
files; the merges exposed a fresh tier of larger callees, and this scope is
those.

## What this project is

A matching decompilation of LEGOLAND (Windows, 2000, VC6 SP3, `/O2 /Gy /Gd`).
Human-written C in `LEGOLAND/*.c` must compile to reproduce
`original/legoland.exe` function-by-function. Status at hand-off: **~1,930
exact functions, 74 partials, ~55% of game code matched exactly**; the
frontier is 390 functions / ~7,500 instructions. No game binary or asset is
ever committed (`original/` and `gamedata/` are gitignored).

## Read these first (in this order)

1. `docs/LANE_BRIEF.md` — the method per function and **the verification gate**
   (its PROJECT/STATUS lines are stale; use ENVIRONMENT below).
2. `docs/DECOMP.md`, section "VC6 SP3 codegen levers" — the ~300 entries at the
   TOP are the newest, your scope A–C levers folded in as blocks. Search the
   rest when stuck.
3. `docs/HANDOFF.md` §3 (rules not in the code), §6B (residual triage), §6C.

## Environment

```
cd /Users/systemadmin/Documents/Development/Github/legoland
git checkout main && git pull --ff-only origin main
PY=/Users/systemadmin/.venvs/legoland/bin/python
export LEGOLAND_CL=/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl
```

- Disassemble: `$PY tools/disasm.py original/legoland.exe 0x<RVA> <count>` —
  **RVA = VA − 0x400000** (0x00423a10 → 0x23a10).
- Iterate: `$PY tools/matchfull.py LEGOLAND/<file>.c <Name> 0x<VA> --obj /tmp/fd_<Name>.obj`
- **Authoritative gate:** `$PY tools/audit.py LEGOLAND/<file>.c` — a function
  counts ONLY when it prints `[OK]`. Trust `audit.py` over `matchfull.py`.
- `/W3` check: `ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/fd_w3.obj LEGOLAND/<file>.c`
- Objects always under `/tmp`; never `/Zi`.
- **Do NOT run `tools/verify.py`, `tools/progress.py` or `tools/coverage.py`.**
  `audit.py`/`matchfull.py` are per-process and safe alongside other
  sessions' compiles.
- Known tooling defect: the extent walker stops at an unconditional `jmp` no
  EARLIER branch crosses (a rotated loop's entry `jmp` mid-function
  under-bounds it and reports ESCAPES against a truncated extent; `MatMul` and
  `Coaster3D_BuildPieceGeometry` are the two known cases). If yours matches
  the truncated extent exactly, say so in the WIP note and move on.

## Your files and functions (≈2,060 instructions)

Create these NEW files. Do not edit any existing `.c` file — read them freely.
Anything above 0x0049e000 is the statically linked CRT and is not a target.

### `LEGOLAND/coaster7.c` — the coaster's entrance track, model loader and physics callbacks (≈601 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x0041f380 | `Span_SetClip` | 23 | coastermath.c |
| 0x0041e4c0 | `Route_SetDeadline` | 23 | schoolcar8.c |
| 0x004223c0 | `CountModelRecords` | 27 | schoolcar4.c |
| 0x00426490 | `Mat3_ToMat4` | 30 | coastermath.c |
| 0x00424bc0 | `Coaster_GetLongestWait` | 30 | schoolcar8.c |
| 0x00429940 | `TrackRunStepsBack` | 33 | coaster6.c |
| 0x0041de10 | `RoutePhys_EvaluateDerivative` | 55 | schoolcar8.c |
| 0x004248b0 | `Coaster_StationDerivative` | 57 | schoolcar8.c |
| 0x00420550 | `CoasterModel_LoadFile` | 92 | schoolcar7.c, schoolcar8.c |
| 0x00423a10 | `Castle_InitEntranceTrack` | 231 | schoolcar8.c |

The coaster is now the most thoroughly documented subsystem in the tree:
`coaster.c`, `coaster3d.c`..`coaster6.c`, `coastermath.c`, `schoolcar.c`..
`schoolcar8.c`. The physics object is an RK4 solver descriptor whose ten-entry
vector-op table `schoolcar8.c` names slot by slot; `RoutePhys_EvaluateDerivative`
and `Coaster_StationDerivative` are the two derivative callbacks it installs
(`Route_InitPhysics`, `Coaster_StepStationDeparture` in schoolcar8.c are the
callers); `TrackRunStepsBack` is the mirror of `coaster6.c`'s `TrackRunSteps`
(0x00429990 — the run/fit model is in `coaster6.c`'s header);
`Castle_InitEntranceTrack` is documented from its caller `schoolcar8.c`'s
`Castle_InitStationCorners` side; `CoasterModel_LoadFile` sits under
`schoolcar4.c`'s `LoadCoasterModelSet` and `schoolcar7.c`'s `LoadLmsModel`.
**Smallest first; `Castle_InitEntranceTrack` last.**

### `LEGOLAND/uimisc2.c` — movie, help bar, report and level-end screens (≈902 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00490b90 | `ReportPrevPageInput` | 22 | mapscreen2.c |
| 0x00468b00 | `EnqueueObjectHelp` | 23 | uimisc.c |
| 0x0046d3c0 | `FreeIcon` | 34 | uimisc.c |
| 0x0048abb0 | `StartFreePlayPark` | 39 | uimisc.c |
| 0x00490610 | `SetReportMovie` | 42 | uimisc.c |
| 0x00491080 | `PrintReportLine` | 43 | mapscreen4.c |
| 0x00468b40 | `SetScriptEventText` | 46 | uimisc.c |
| 0x0048d230 | `RestoreCurrentProfileFromList` | 56 | uimisc.c |
| 0x004908b0 | `KillReportScreenSprites` | 56 | uimisc.c |
| 0x0046de90 | `GetIconHitBounds` | 57 | uimisc.c |
| 0x00459710 | `RunLevelEndSequence` | 67 | uimisc.c |
| 0x00490ea0 | `BlinkReportPageIcons` | 80 | mapscreen4.c |
| 0x0046d110 | `UpdateHelpBar` | 81 | uimisc.c |
| 0x004771f0 | `PlayMovie` | 156 | uimisc.c |

`uimisc.c` (codex-b, 35 of 35 exact — read it end to end, it is the caller of
most of these and its header states their contracts), `mapscreen2.c`
(`InitScreen7`, the report screen), `mapscreen3.c`, your own `mapscreen4.c`,
`fpui3.c`..`fpui5.c`, `iconui.c`, `profiles.c`, `screens3.c`. `PlayMovie`
will be AVI/DirectShow or a home-grown player — `wndenv.c` and `surface.c` show
how matched code spells COM vtable calls. **Smallest first.**

### `LEGOLAND/audio5.c` — narration and sample sources (≈261 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00496660 | `ClearSampleSource` | 22 | audio3.c, sysmisc.c |
| 0x0042fb00 | `Restaurant2_StartSound` | 24 | ridecb3.c |
| 0x004967b0 | `RefreshSampleVolumes` | 28 | audio3.c |
| 0x00498420 | `ReadNarrationWaveHeader` | 187 | audio4.c |

`audio4.c` (codex-b: `PlayNarrationFile`, the caller of the wave-header
reader), `audio2.c`, `audio3.c`, `sysmisc.c` (`UpdateSampleSource`) and your
own `sysmisc2.c` (`SetSampleScreenPos`, `StartPlayableSample`) are the context.

### `LEGOLAND/render5.c` — cursor tiles, path overlay, text cache (≈293 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x0045a3e0 | `TakeRenderNodeInColumn` | 28 | objmap2.c, render3.c |
| 0x00461020 | `FlushCursorSpriteList` | 29 | renderview.c |
| 0x00455f70 | `ExpireCachedText` | 29 | render2.c |
| 0x00451e20 | `SaveCertificateBitmap` | 29 | mapscreen2.c |
| 0x00460e90 | `DrawPathTileOverlay` | 72 | render4.c |
| 0x004610f0 | `PaintCursorTiles` | 106 | render4.c |

`render4.c` (a lane running right now — read it if it exists when you get
there; it holds `PaintTileLayer`, `DrawEditCursor` and `TakeRenderNodeByPos`,
the callers of three of yours), `render3.c` (`CalculateFullMapRenderOrder`,
exact — the render-node model), `renderview.c`, `render2.c`, `fpui3.c`
(`FindCachedText`, exact — the text cache's key), `mapscreen2.c`
(`PrintScreenMode8` documents `SaveCertificateBitmap` from the caller's side).

**Order:** `audio5.c` → `render5.c` → `coaster7.c` → `uimisc2.c`, smallest
first within each file, finishing a file before starting the next. Save after
every function.

## Files you must NOT touch

- Any existing `LEGOLAND/*.c` (read only), and anything under `tools/`.
- New files belonging to a lane running right now: `render4.c`, `bswater3.c`;
  and to the Codex session on scope C: `simcore2.c`, `pathmisc.c`, `lfmisc.c`,
  `sysstubs.c`, `screencb7.c`.
- `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`,
  `docs/LEGOLANDPROGRESS.HTML`, `docs/LEGOLANDPROGRESS.SVG`. **Put your levers
  and findings in `docs/lanes/fable-d.md`** in DECOMP's bullet style, each with
  its evidence.

## Git

`git checkout -b fable/scope-d` from current `main` (which contains your
scope A–C work, merged). Commit to that branch only; **never commit to `main`
and never push `main`.** Push the branch when a file is complete. End every
commit message with:

```
Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>
```

## Rules that are not in the code

- **Marker discipline.** `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <precise residual>)`
  until `audit.py` prints `[OK]`; then exactly `// FUNCTION: LEGOLAND 0x<VA>`.
  Marker on the line immediately above the signature. **Never commit a
  prologue-only or fabricated-tail body as `// FUNCTION:`.**
- Declare every callee `extern` with a trailing `/* 0x0044xxxx */` comment.
- **Extern prototype TYPES are caller-side codegen levers** — a callee may need
  different prototypes in different files (`StandardRemoveObject` has five in
  the tree); never "align" another file's declaration, note the divergence.
  Symbol NAMES are not levers.
- **One name per address.** Two definitions of one name at two addresses
  (`RestoreCoasterCar` was one until today) will not link; give the
  non-exported one a distinct name and record it.
- Reproduce original bugs; comment them at the site; do not fix them.
- Report behaviour you cannot explain from the disassembly rather than
  inventing plausible C. An honest WIP beats a wrong body that compiles close.

## Levers that decide whether a first draft lands at 95% or 40%

Your scope A–C lists still apply in full. New since scope C, all measured:

- **A missing callee-saved push is a LIVE-VALUE deficit, not an allocation
  problem** — compute the value both arms need BEFORE the split.
- **An 8-byte struct RETURN must be the ACCUMULATOR of any sum added to it**
  (`p.x += ...`, never `t = p.x + ...`).
- **"Read a class/def global directly at every use" holds only if both derived
  sums are computed BEFORE either store to the object being written.**
- **A value that must survive a call cannot be a FIELD of an address-taken
  aggregate** — assign plain locals INTO the aggregate.
- **Two block-scoped out-param locals, one per arm, stop VC6 head-merging the
  arms' identical argument setup** (the original's `lea ecx` in one arm and
  `lea edx` in the other is the proof).
- **VC6's tail-duplication threshold:** a shared tail of one call + `add esp,4`
  is COPIED into the early arm; two calls + a 0x10-byte argument block is
  JUMPED to.
- **A strength-reduced record cursor anchors at the field with the MOST
  references (ties to the LAST; offset 0 never wins from `->`)**, and over an
  inline struct array on the SECOND store statement's offset; walk such arrays
  by SUBSCRIPT, not through a named pointer.
- **Test the GLOBAL, not the local copy, to stop VC6 propagating a list head's
  null-ness onto the cursor.** A free volatile read of a global list head pins
  the load BELOW the callee-saved pushes, not above.
- **Explicit parentheses stop VC6 reassociating FP constants; a double-typed
  intermediate pools the constant as a double**; a `volatile float` local is
  how an FP constant gets a stack home; float source order is the reverse of
  `fld` order except at an accumulator.
- **`rep stosd` is NOT proof of `memset`**; **two zero registers in one small
  function is a sub-object `memset`**.
- **Size is NOT evidence of twinning, even within one body** — three sibling
  loops of 0x42/0x40/0x42 bytes were written differently. Diff first.
- **VC6 always jump-threads a provably-NULL pointer into a following `if (p)`**;
  the un-threaded form is reachable only through a `volatile` local.

## When a function will not close

Run the §6B triage: strict, register-blind, offset-blind with frame homes by
push depth. `strict >> rb` is allocation — try one free `volatile` read, then
the one-temporary spellings; `strict >> ob` is the frame; `strict == rb == ob`
is a scheduling permutation — sweep the store order. Then write the honest WIP
note and move on.

## Report format (in `docs/lanes/fable-d.md`, and in your final message)

Per function: instructions, percentage, `audit [OK]` yes/no, marker as
committed, first diverging index and residual for anything not `[OK]`. Then
mechanics recovered (the movie player, wave-header and certificate-bitmap
formats are runtime-spec material), callees named for the first time, original
bugs reproduced, extern-type divergences, and every lever with its evidence.
