# Scope LL10 — unreferenced (dead) functions, 0x00420fd0..0x004237f0: coaster model / raster neighbourhood

Branch `scope/LL10` from `main` `7d756410`. **New file `LEGOLAND/unref2.c`** —
create only this file (plus `docs/lanes/scope-ll10.md`); read everything else.
Object prefix `/tmp/sll10_`. Contract: `docs/PARALLEL_CONTRACT.md`. Method and
gate: `docs/LANE_BRIEF.md`. Levers: `docs/LEVERS.md` (symptom index first).

## What these are

`tools/inventory.py` (scope N, `docs/lanes/scope-n.md`) classifies these
13 functions as **DEAD**: nothing live in the binary calls, tail-jumps to or
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
- Before committing: `audit.py LEGOLAND/unref2.c` ends PASS, `relocs.py` has
  zero `MISMATCH`, `/W3` is clean.
- Do not run `verify.py`, `progress.py`, `coverage.py`; do not edit `tools/`,
  `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`.

## Functions (13)

| address | insns | bytes | reached by | nearest matched (file) | callers | notes |
|---|---:|---:|---|---|---|---|
| 0x00420fd0 | 114 | 349 | unreferenced (swept) | Coaster3D_DrawModel (coaster9.c) |  |  |
| 0x00421130 | 112 | 353 | unreferenced (swept) | PhysVec_Add (schoolcar8.c) |  |  |
| 0x00421530 | 3 | 10 | unreferenced (swept) | PhysVec_InitOps (schoolcar8.c) |  |  |
| 0x00422520 | 43 | 106 | called by 0x00422650 [unmatched] | CoasterModel_FindMeshIndex (coaster9.c) | 0x00422650 |  |
| 0x004225e0 | 8 | 24 | unreferenced (swept) | CoasterModel_GetMeshCount (coastertiny.c) |  |  |
| 0x00422650 | 31 | 105 | unreferenced (swept) | CoasterModel_GetPartCount (coastertiny.c) |  |  |
| 0x004227a0 | 8 | 27 | unreferenced (swept) | LoadCoasterModelSet (schoolcar4.c) |  |  |
| 0x004227c0 | 521 | 1613 | unreferenced (swept) | LoadCoasterModelSet (schoolcar4.c) |  |  |
| 0x00423060 | 7 | 17 | unreferenced (swept) | CoasterShades_Init (schoolcar5.c) |  |  |
| 0x00423080 | 64 | 188 | unreferenced (swept) | CoasterShades_Init (schoolcar5.c) |  |  |
| 0x00423750 | 1 | 1 | unreferenced (swept) | CoasterGeomInit (schoolcar.c) |  |  |
| 0x004237a0 | 28 | 65 | unreferenced (swept) | Raster_RestoreState (coastertiny.c) |  |  |
| 0x004237f0 | 51 | 162 | called by 0x004267b0 [unmatched] (+1 more) | Raster_RestoreState (coastertiny.c) | 0x004237a0 0x004267b0 |  |

