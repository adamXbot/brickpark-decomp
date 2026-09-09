# Scope LL15 — unreferenced (dead) functions, 0x00476450..0x0049c110: simcore / profiles / screens / workorder neighbourhood

Branch `scope/LL15` from `main` `7d756410`. **New file `LEGOLAND/unref7.c`** —
create only this file (plus `docs/lanes/scope-ll15.md`); read everything else.
Object prefix `/tmp/sll15_`. Contract: `docs/PARALLEL_CONTRACT.md`. Method and
gate: `docs/LANE_BRIEF.md`. Levers: `docs/LEVERS.md` (symptom index first).

## What these are

`tools/inventory.py` (scope N, `docs/lanes/scope-n.md`) classifies these
44 functions as **DEAD**: nothing live in the binary calls, tail-jumps to or
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
- Before committing: `audit.py LEGOLAND/unref7.c` ends PASS, `relocs.py` has
  zero `MISMATCH`, `/W3` is clean.
- Do not run `verify.py`, `progress.py`, `coverage.py`; do not edit `tools/`,
  `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`.

## Functions (44)

| address | insns | bytes | reached by | nearest matched (file) | callers | notes |
|---|---:|---:|---|---|---|---|
| 0x00476450 | 3 | 10 | unreferenced (swept) | OpenMovie (movie.c) |  |  |
| 0x00477440 | 105 | 368 | unreferenced (swept) | FreeMemScratch (coaster.c) |  |  |
| 0x00477600 | 39 | 118 | unreferenced (swept) | MemScratch_Noop (coaster.c) |  |  |
| 0x0047b500 | 51 | 156 | unreferenced (swept) | LLIDB_ClearOnLevel (memdb.c) |  |  |
| 0x00481d70 | 63 | 232 | unreferenced (swept) | RemovePathSquare (pathsq.c) |  |  |
| 0x004826f0 | 13 | 29 | unreferenced (swept) | FindPathLeg (workorder2.c) |  |  |
| 0x004830e0 | 1 | 1 | called by 0x00483100 [unmatched] | InitialiseBlokes (workers.c) | 0x00483100 |  |
| 0x00483100 | 2 | 10 | unreferenced (swept) | InitialiseBlokes (workers.c) |  |  |
| 0x00483110 | 12 | 32 | unreferenced (swept) | InitialiseBlokes (workers.c) |  |  |
| 0x00484940 | 2 | 3 | unreferenced (swept) | ApplyObjectOrientationToPerson (bnvpath.c) |  |  |
| 0x004855d0 | 84 | 201 | unreferenced (swept) | PrintSpriteEx (printlist.c) |  |  |
| 0x00486180 | 1 | 1 | unreferenced (swept) | FindShadedColour (tri3d.c) |  |  |
| 0x00486490 | 30 | 70 | unreferenced (swept) | SetFlatColour (tri3d.c) |  |  |
| 0x00486520 | 6 | 19 | unreferenced (swept) | BuildRecipTable (tri3d.c) |  |  |
| 0x00488730 | 39 | 105 | unreferenced (swept) | SetMousePixel (tri3d.c) |  |  |
| 0x0048b690 | 9 | 33 | unreferenced (swept) | FreePlayInit_48b6c0 (tinystubs.c) |  |  |
| 0x0048cc90 | 18 | 59 | unreferenced (swept) | DeleteIconInput (screens3.c) |  |  |
| 0x0048ccd0 | 18 | 59 | unreferenced (swept) | LightUpthisDeleteIcon (screens2.c) |  |  |
| 0x0048cd10 | 18 | 59 | unreferenced (swept) | LightUpthisDeleteIcon (screens2.c) |  |  |
| 0x0048ef10 | 13 | 45 | unreferenced (swept) | ExitOkInput (screens3.c) |  |  |
| 0x0048ef40 | 18 | 78 | unreferenced (swept) | ExitOkInput (screens3.c) |  |  |
| 0x0048f4f0 | 17 | 85 | unreferenced (swept) | ExitCloseInput (screens3.c) |  |  |
| 0x0048f5d0 | 11 | 30 | unreferenced (swept) | VolUpInput (screens3.c) |  |  |
| 0x0048fbd0 | 12 | 40 | unreferenced (swept) | ProcessScreenPopup (tinystubs.c) |  |  |
| 0x0048fe70 | 16 | 59 | unreferenced (swept) | TitleNewInput (screens3.c) |  |  |
| 0x004917c0 | 108 | 328 | called by 0x0048cd10 [unmatched] (+2 more) | UpDateCurrentProfile (profiles.c) | 0x0048cc90 0x0048ccd0 0x0048cd10 |  |
| 0x004919a0 | 11 | 30 | called by 0x004917c0 [unmatched] (+3 more) | AddNodeToProfileList (profiles.c) | 0x0048cc90 0x0048ccd0 0x0048cd10 0x004917c0 |  |
| 0x00491b80 | 16 | 65 | unreferenced (swept) | DeleteProfileList (listdel.c) |  |  |
| 0x00491f90 | 28 | 70 | unreferenced (swept) | NewProfileCloseInput (screens3.c) |  |  |
| 0x00491fe0 | 57 | 179 | unreferenced (swept) | NewProfileCloseInput (screens3.c) |  |  |
| 0x00495f00 | 73 | 258 | unreferenced (swept) | LoadMusicSegment (music.c) |  |  |
| 0x00496010 | 39 | 122 | unreferenced (swept) | GetMusicBand (music.c) |  |  |
| 0x004975a0 | 7 | 16 | unreferenced (swept) | UnlinkSprite (narration2.c) |  |  |
| 0x00497e40 | 21 | 53 | unreferenced (swept) | ShowLayer (blokelist.c) |  |  |
| 0x00499040 | 61 | 122 | unreferenced (swept) | DeleteStrings (text.c) |  |  |
| 0x004990c0 | 45 | 94 | unreferenced (swept) | DeleteStrings (text.c) |  |  |
| 0x00499120 | 46 | 98 | unreferenced (swept) | DeleteStrings (text.c) |  |  |
| 0x00499190 | 76 | 175 | unreferenced (swept) | UpcaseString (levelkw.c) |  |  |
| 0x00499240 | 80 | 186 | unreferenced (swept) | UpcaseString (levelkw.c) |  |  |
| 0x00499340 | 23 | 53 | unreferenced (swept) | UpcaseString (levelkw.c) |  |  |
| 0x00499490 | 42 | 110 | unreferenced (swept) | GetBlink (util.c) |  |  |
| 0x00499ca0 | 36 | 82 | unreferenced (swept) | FindFreeGardener (workorder2.c) |  |  |
| 0x0049c0f0 | 9 | 19 | unreferenced (swept) | SaveGardeners (savechunks.c) |  |  |
| 0x0049c110 | 17 | 37 | unreferenced (swept) | SaveGardeners (savechunks.c) |  |  |

