# Scope LL11 — unreferenced (dead) functions, 0x004238a0..0x0042a020: coaster track / castleobj neighbourhood

Branch `scope/LL11` from `main` `7d756410`. **New file `LEGOLAND/unref3.c`** —
create only this file (plus `docs/lanes/scope-ll11.md`); read everything else.
Object prefix `/tmp/sll11_`. Contract: `docs/PARALLEL_CONTRACT.md`. Method and
gate: `docs/LANE_BRIEF.md`. Levers: `docs/LEVERS.md` (symptom index first).

## What these are

`tools/inventory.py` (scope N, `docs/lanes/scope-n.md`) classifies these
16 functions as **DEAD**: nothing live in the binary calls, tail-jumps to or
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
- Before committing: `audit.py LEGOLAND/unref3.c` ends PASS, `relocs.py` has
  zero `MISMATCH`, `/W3` is clean.
- Do not run `verify.py`, `progress.py`, `coverage.py`; do not edit `tools/`,
  `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`.

## Functions (16)

| address | insns | bytes | reached by | nearest matched (file) | callers | notes |
|---|---:|---:|---|---|---|---|
| 0x004238a0 | 49 | 133 | called by 0x00428b80 [unmatched] (+1 more) | Raster_RestoreState (coastertiny.c) | 0x004237f0 0x00428b80 |  |
| 0x00423930 | 2 | 6 | unreferenced (swept) | Castle_GetFirstCorner (coastertiny.c) |  |  |
| 0x004239a0 | 1 | 1 | unreferenced (swept) | Castle_GetFirstCorner (coastertiny.c) |  |  |
| 0x00426000 | 51 | 217 | unreferenced (swept) | MatIdentity (coastermath.c) |  |  |
| 0x004260e0 | 2 | 7 | unreferenced (swept) | MatIdentity (coastermath.c) |  |  |
| 0x00426230 | 11 | 31 | called by 0x00428b80 [unmatched] (+1 more) | TransformVerts (coaster3d.c) | 0x004267b0 0x00428b80 |  |
| 0x00426680 | 14 | 45 | unreferenced (swept) | AnyCoasterRegionFullyInside (schoolcar.c) |  |  |
| 0x004267b0 | 56 | 153 | unreferenced (swept) | Raster_ResetClipRing (coastertiny.c) |  |  |
| 0x00426850 | 80 | 261 | unreferenced (swept) | Raster_ResetClipRing (coastertiny.c) |  |  |
| 0x00426be0 | 26 | 61 | unreferenced (swept) | PackCoasterPtr (coaster.c) |  |  |
| 0x004272a0 | 44 | 106 | unreferenced (swept) | ReadCoasterBlob (coaster.c) |  |  |
| 0x00427310 | 78 | 169 | unreferenced (swept) | RouteSeat_IsOccupied (coaster9.c) |  |  |
| 0x004274f0 | 48 | 125 | unreferenced (swept) | RouteSeat_InitPosition (coaster8.c) |  |  |
| 0x00427570 | 28 | 62 | unreferenced (swept) | Track_Update (coaster.c) |  |  |
| 0x00428b80 | 98 | 290 | unreferenced (swept) | CoasterSceneInit (schoolcar.c) |  |  |
| 0x0042a020 | 70 | 232 | unreferenced (swept) | CoasterFxPoolInit (schoolcar.c) |  |  |

