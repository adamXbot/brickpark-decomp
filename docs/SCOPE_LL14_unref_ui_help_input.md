# Scope LL14 — unreferenced (dead) functions, 0x0046d3b0..0x004762f0: popup / iconbar / input / fpui neighbourhood

Branch `scope/LL14` from `main` `7d756410`. **New file `LEGOLAND/unref6.c`** —
create only this file (plus `docs/lanes/scope-ll14.md`); read everything else.
Object prefix `/tmp/sll14_`. Contract: `docs/PARALLEL_CONTRACT.md`. Method and
gate: `docs/LANE_BRIEF.md`. Levers: `docs/LEVERS.md` (symptom index first).

## What these are

`tools/inventory.py` (scope N, `docs/lanes/scope-n.md`) classifies these
21 functions as **DEAD**: nothing live in the binary calls, tail-jumps to or
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
- Before committing: `audit.py LEGOLAND/unref6.c` ends PASS, `relocs.py` has
  zero `MISMATCH`, `/W3` is clean.
- Do not run `verify.py`, `progress.py`, `coverage.py`; do not edit `tools/`,
  `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`.

## Functions (21)

| address | insns | bytes | reached by | nearest matched (file) | callers | notes |
|---|---:|---:|---|---|---|---|
| 0x0046d3b0 | 2 | 11 | unreferenced (swept) | SetHelpFaceTalking (tinystubs.c) |  |  |
| 0x0046d800 | 23 | 77 | called by 0x0046f5e0 [unmatched] | LoadSpriteIcon (iconui.c) | 0x0046f5e0 |  |
| 0x0046de10 | 23 | 56 | unreferenced (swept) | GetIconBounds (uimisc.c) |  |  |
| 0x0046dfd0 | 37 | 106 | pointer in 0x0046d800 [unmatched] | RenderBoxIcon (render2.c) | 0x0046d800 |  |
| 0x0046ea10 | 43 | 131 | pointer in 0x0046f860 [unmatched] | RenderGBarSprite (render2.c) | 0x0046f860 |  |
| 0x0046f5e0 | 59 | 163 | unreferenced (swept) | AddGBarClassIcon (fpui.c) |  |  |
| 0x0046f860 | 16 | 46 | unreferenced (swept) | LoadIconBarGFX (popupmisc.c) |  |  |
| 0x0046f9a0 | 148 | 411 | unreferenced (swept) | UnloadIconBarGFX (popupmisc.c) |  |  |
| 0x00473560 | 26 | 68 | unreferenced (swept) | ResetHelpKeyCursor (tinystubs.c) |  |  |
| 0x004735c0 | 6 | 22 | unreferenced (swept) | ResetHelpKeyCursor (tinystubs.c) |  |  |
| 0x00473680 | 30 | 89 | unreferenced (swept) | ProcessHelpKeys (tinystubs.c) |  |  |
| 0x004736e0 | 3 | 16 | unreferenced (swept) | ProcessHelpKeys (tinystubs.c) |  |  |
| 0x004736f0 | 117 | 376 | unreferenced (swept) | ProcessHelpKeys (tinystubs.c) |  |  |
| 0x00474090 | 8 | 26 | unreferenced (swept) | IsRShiftDown (sysstubs.c) |  |  |
| 0x004755b0 | 2 | 3 | unreferenced (swept) | InsertObjectNode (fpui5.c) |  |  |
| 0x004761e0 | 1 | 1 | unreferenced (swept) | Unload_RAndDCheckBox (sweep2.c) |  |  |
| 0x00476220 | 2 | 3 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) |  |  |
| 0x00476230 | 2 | 3 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) |  |  |
| 0x00476240 | 2 | 3 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) |  |  |
| 0x00476250 | 53 | 147 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) |  |  |
| 0x004762f0 | 67 | 211 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) |  |  |

