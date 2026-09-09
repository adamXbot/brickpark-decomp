# Scope LL9 — unreferenced (dead) functions, 0x00401e00..0x004207b0: schoolcar / logflume / coaster neighbourhoods

Branch `scope/LL9` from `main` `7d756410`. **New file `LEGOLAND/unref1.c`** —
create only this file (plus `docs/lanes/scope-ll9.md`); read everything else.
Object prefix `/tmp/sll9_`. Contract: `docs/PARALLEL_CONTRACT.md`. Method and
gate: `docs/LANE_BRIEF.md`. Levers: `docs/LEVERS.md` (symptom index first).

## What these are

`tools/inventory.py` (scope N, `docs/lanes/scope-n.md`) classifies these
31 functions as **DEAD**: nothing live in the binary calls, tail-jumps to or
takes the address of them; most were found only by the padding sweep. The
linker kept them because the game was built without `/OPT:REF`. They still
count toward byte coverage, and this scope exists to close them. Expect
them to be ordinary C from the same translation unit as their nearest
matched neighbour (the binary is laid out by TU), so read that neighbour's
file first: its structs, globals and helper names are the vocabulary.

Some are one-byte `ret` stubs — `void f(void) {}` — and several call each
other (the `callers` column); write callees before callers so the extern
prototypes are right. A body reached only by a `pointer in` another dead
function is a callback whose signature that caller's call site dictates.

## Rules specific to this scope

- Names are unknown (no export). Name each function for what it does, in the
  project's style (`Verb_Noun`, ride prefixes); if you truly cannot tell,
  `Unref_<VA>` is acceptable and must be recorded in your notes. One name per
  address; grep `LEGOLAND/*.c` before choosing a name so it does not collide.
- Declare every callee `extern` with a trailing `/* 0x0044xxxx */` comment.
  Never change another file's declaration; note type divergences instead.
- A body counts only when `audit.py` prints `[OK]`; until then it carries
  `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <precise residual>)`. Never
  commit a fabricated or prologue-only body as `// FUNCTION:`.
- Before committing: `audit.py LEGOLAND/unref1.c` ends PASS, `relocs.py` has
  zero `MISMATCH`, `/W3` is clean.
- Do not run `verify.py`, `progress.py`, `coverage.py`; do not edit `tools/`,
  `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`.

## Functions (31)

| address | insns | bytes | reached by | nearest matched (file) | callers | notes |
|---|---:|---:|---|---|---|---|
| 0x00401e00 | 69 | 289 | unreferenced (swept) | SchoolCarIdleStep (schoolcar4.c) |  |  |
| 0x00403d60 | 12 | 38 | unreferenced (swept) | Copters_StepRider (ridetiny.c) |  |  |
| 0x00408f90 | 50 | 123 | unreferenced (swept) | LFTrack_FindPiece (posstep.c) |  |  |
| 0x0040adb0 | 69 | 209 | unreferenced (swept) | LFPiece_ShapeIndex (logflume2.c) |  |  |
| 0x0040b270 | 7 | 19 | called by 0x0040c250 [unmatched] | LFBoat_IsOnPiece (posstep.c) | 0x0040c250 |  |
| 0x0040bd40 | 76 | 191 | unreferenced (swept) | LFRun_Tick (logflume4.c) |  |  |
| 0x0040c250 | 64 | 143 | unreferenced (swept) | LFPiece_HasCursor (logflume6.c) |  |  |
| 0x00411e20 | 1 | 1 | unreferenced (swept) | LFQueue_Append (lfmisc.c) |  |  |
| 0x00411f70 | 13 | 34 | unreferenced (swept) | LFQueue_AddRider (lfentrance.c) |  |  |
| 0x004120e0 | 7 | 22 | unreferenced (swept) | BuildWalkPath (mappath.c) |  |  |
| 0x0041b130 | 7 | 27 | unreferenced (swept) | GetInterface (loaders.c) |  |  |
| 0x0041cf20 | 26 | 67 | unreferenced (swept) | Track_CountTailPieces (schoolcar8.c) |  |  |
| 0x0041d040 | 6 | 13 | unreferenced (swept) | FindTrackNodeAt (coaster.c) |  |  |
| 0x0041d050 | 6 | 13 | unreferenced (swept) | FindTrackNodeAt (coaster.c) |  |  |
| 0x0041e260 | 32 | 69 | unreferenced (swept) | Route_UpdateTimer (coastertiny.c) |  |  |
| 0x0041e2f0 | 25 | 55 | unreferenced (swept) | Route_FindFreeSeat (schoolcar8.c) |  |  |
| 0x0041e720 | 27 | 56 | called by 0x0041e260 [unmatched] | RouteNode_FindFreeSeat (coaster8.c) | 0x0041e260 |  |
| 0x0041e790 | 23 | 46 | called by 0x0041e2f0 [unmatched] | RouteNode_FindFreeSeat (coaster8.c) | 0x0041e2f0 |  |
| 0x0041e7c0 | 14 | 30 | unreferenced (swept) | RouteNode_GetAcceleration (coastertiny.c) |  |  |
| 0x0041ef10 | 1 | 1 | unreferenced (swept) | RouteSystemInit (schoolcar.c) |  |  |
| 0x0041f350 | 14 | 35 | unreferenced (swept) | Span_SetClip (coaster7.c) |  |  |
| 0x0041f5a0 | 63 | 170 | unreferenced (swept) | Span_SetClip (coaster7.c) |  |  |
| 0x0041f650 | 80 | 191 | called by 0x0041f720 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) | 0x0041f720 |  |
| 0x0041f710 | 5 | 12 | called by 0x0041f720 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) | 0x0041f720 |  |
| 0x0041f720 | 34 | 108 | called by 0x0041f7f0 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) | 0x0041f7f0 |  |
| 0x0041f790 | 33 | 78 | pointer in 0x0041f7f0 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) | 0x0041f7f0 |  |
| 0x0041f7e0 | 4 | 9 | pointer in 0x0041f7f0 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) | 0x0041f7f0 |  |
| 0x0041f7f0 | 22 | 85 | unreferenced (swept) | TrackCursor_AdvanceGeometry (schoolcar8.c) |  |  |
| 0x0041fa10 | 134 | 393 | unreferenced (swept) | TrackCursor_AdvanceGeometry (schoolcar8.c) |  |  |
| 0x00420520 | 1 | 1 | unreferenced (swept) | CoasterModel_SetDirectory (coastertiny.c) |  |  |
| 0x004207b0 | 5 | 13 | unreferenced (swept) | CoasterModel_LoadPalette (coastertiny.c) |  |  |

