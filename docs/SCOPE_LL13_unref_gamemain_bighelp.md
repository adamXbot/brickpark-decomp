# Scope LL13 — unreferenced (dead) functions, 0x004511e0..0x00466070: gamemain / sysmisc / bighelp / tilehelp neighbourhood

Branch `scope/LL13` from `main` `7d756410`. **New file `LEGOLAND/unref5.c`** —
create only this file (plus `docs/lanes/scope-ll13.md`); read everything else.
Object prefix `/tmp/sll13_`. Contract: `docs/PARALLEL_CONTRACT.md`. Method and
gate: `docs/LANE_BRIEF.md`. Levers: `docs/LEVERS.md` (symptom index first).

## What these are

`tools/inventory.py` (scope N, `docs/lanes/scope-n.md`) classifies these
17 functions as **DEAD**: nothing live in the binary calls, tail-jumps to or
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
- Before committing: `audit.py LEGOLAND/unref5.c` ends PASS, `relocs.py` has
  zero `MISMATCH`, `/W3` is clean.
- Do not run `verify.py`, `progress.py`, `coverage.py`; do not edit `tools/`,
  `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`.

## Functions (17)

| address | insns | bytes | reached by | nearest matched (file) | callers | notes |
|---|---:|---:|---|---|---|---|
| 0x004511e0 | 1 | 1 | unreferenced (swept) | RES_FindVolumeOnResPath (sysmisc2.c) |  |  |
| 0x004511f0 | 1 | 1 | called by 0x00451210 [unmatched] | RES_FindVolumeOnResPath (sysmisc2.c) | 0x00451210 |  |
| 0x00451200 | 1 | 1 | unreferenced (swept) | RES_FindVolumeOnResPath (sysmisc2.c) |  |  |
| 0x00451210 | 35 | 110 | unreferenced (swept) | RES_FindVolumeOnResPath (sysmisc2.c) |  |  |
| 0x00451280 | 88 | 260 | called by 0x00451210 [unmatched] | RES_FindVolumeOnResPath (sysmisc2.c) | 0x00451210 |  |
| 0x00451390 | 39 | 123 | unreferenced (swept) | RES_EnsureMounted (sysmisc.c) |  |  |
| 0x00451410 | 35 | 107 | called by 0x00451210 [unmatched] | RES_EnsureMounted (sysmisc.c) | 0x00451210 |  |
| 0x00451480 | 9 | 27 | unreferenced (swept) | RES_EnsureMounted (sysmisc.c) |  |  |
| 0x004514a0 | 4 | 14 | called by 0x00451210 [unmatched] | RES_EnsureMounted (sysmisc.c) | 0x00451210 |  |
| 0x004514b0 | 55 | 159 | unreferenced (swept) | RES_EnsureMounted (sysmisc.c) |  |  |
| 0x00451550 | 49 | 132 | called by 0x00451210 [unmatched] | RES_EnsureMounted (sysmisc.c) | 0x00451210 |  |
| 0x00453c20 | 20 | 71 | unreferenced (swept) | __DEBUG_FREE (memdb.c) |  |  jump-table |
| 0x004551a0 | 46 | 123 | called by 0x0043ea30 [unmatched] | PrintCentColref (text.c) | 0x0043ea30 |  |
| 0x00455220 | 120 | 335 | called by 0x0043ea30 [unmatched] | BubbleHelp (bighelp.c) | 0x0043ea30 |  |
| 0x00455de0 | 51 | 109 | unreferenced (swept) | PrintCachedText (bubblecache.c) |  |  |
| 0x0045ade0 | 291 | 885 | unreferenced (swept) | GetTileCentre (tilehelp.c) |  |  jump-table |
| 0x00466070 | 1 | 1 | unreferenced (swept) | PresentFlip (blitmisc.c) |  |  |

