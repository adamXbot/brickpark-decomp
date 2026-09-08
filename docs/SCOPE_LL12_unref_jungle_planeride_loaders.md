# Scope LL12 — unreferenced (dead) functions, 0x00434810..0x0043f4f0: jungle cruise / plane ride / loaders neighbourhood

Branch `scope/LL12` from `main` `7d756410`. **New file `LEGOLAND/unref4.c`** —
create only this file (plus `docs/lanes/scope-ll12.md`); read everything else.
Object prefix `/tmp/sll12_`. Contract: `docs/PARALLEL_CONTRACT.md`. Method and
gate: `docs/LANE_BRIEF.md`. Levers: `docs/LEVERS.md` (symptom index first).

## What these are

`tools/inventory.py` (scope N, `docs/lanes/scope-n.md`) classifies these
11 functions as **DEAD**: nothing live in the binary calls, tail-jumps to or
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
- Before committing: `audit.py LEGOLAND/unref4.c` ends PASS, `relocs.py` has
  zero `MISMATCH`, `/W3` is clean.
- Do not run `verify.py`, `progress.py`, `coverage.py`; do not edit `tools/`,
  `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`.

## Functions (11)

| address | insns | bytes | reached by | nearest matched (file) | callers | notes |
|---|---:|---:|---|---|---|---|
| 0x00434810 | 4 | 14 | unreferenced (swept) | JcMonkeyFish_GetDrawDesc (screencb2.c) |  |  |
| 0x00434820 | 14 | 61 | unreferenced (swept) | JcMonkeyFish_GetDrawDesc (screencb2.c) |  |  |
| 0x00434860 | 119 | 335 | unreferenced (swept) | JcMonkeyFish_GetDrawDesc (screencb2.c) |  |  |
| 0x004349b0 | 94 | 362 | unreferenced (swept) | JcDeco_Remove (junglecruise.c) |  |  |
| 0x00434b20 | 7 | 19 | unreferenced (swept) | JcDeco_Remove (junglecruise.c) |  |  |
| 0x0043e930 | 101 | 244 | called by 0x0043ea30 [unmatched] | PlaneRide_Activate (mechrides.c) | 0x0043ea30 |  |
| 0x0043ea30 | 395 | 1187 | called by 0x0043f0b0 [unmatched] (+1 more) | PlaneRide_Activate (mechrides.c) | 0x0043eee0 0x0043f0b0 |  |
| 0x0043eee0 | 173 | 464 | unreferenced (swept) | LoadPos (loaders.c) |  |  |
| 0x0043f0b0 | 313 | 944 | unreferenced (swept) | LoadPos (loaders.c) |  |  |
| 0x0043f460 | 57 | 130 | called by 0x0043f4f0 [unmatched] | LoadPos (loaders.c) | 0x0043f4f0 |  |
| 0x0043f4f0 | 131 | 355 | unreferenced (swept) | LoadPos (loaders.c) |  |  |

