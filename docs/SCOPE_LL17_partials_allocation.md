# Scope LL17 — partials wave A: allocation-class residuals on main

Branch `scope/LL17` from `origin/main` `b39f261b`. **PARTIAL scope**: edit only
the WIP bodies below and the notes above their markers (see
`docs/PARALLEL_CONTRACT.md`, "Extra rules for PARTIAL scopes"). Object
prefix `/tmp/sll17_`. Findings go in `docs/lanes/scope-ll17.md`.

## Why these are being reopened

Scope LL14 (2026-09-08, `docs/lanes/scope-ll14.md`, "Closing 0x0046f9a0")
closed a 148-instruction allocation floor with a lever no earlier lane had:
**when the four callee-saved registers are oversubscribed, VC6 ranked the
candidates by STATIC NAME-APPEARANCE COUNT in the source, not by loop
weight.** Cutting a loop cursor from 7-8 appearances to 4 (one named
temporary carrying a load to its several consumers; the tail re-reading a
just-stored struct field instead of the local) flipped the allocation, and
the original's zero web then appeared for free. Every body below is an
allocation or zero-web residual whose notes never tried that lever.
Measure appearance counts in the current spelling first; predict; then move.

## Functions (7)

| address | name | file | residual (from the marker) |
| --- | --- | --- | --- |
| 0x0046d850 | ScrollIconPanel | fpui4.c | 71.1%, 35/121 strict; four-value allocation floor; first 62 |
| 0x00482430 | BuildPTPRoute | workorder3.c | 86.8%, 10/76 strict; allocation floor; first 16 |
| 0x00499d60 | UnlinkGardenerOrder | workorder3.c | 50.0%, 34/68 strict; shared-tail ordering floor; first 34 |
| 0x0045e960 | FindObjDoorTile | mapbuild2.c | 93.3%, callee-saved register choice for the two door-offset copies; rb 0 |
| 0x00459970 | TallyBuildFootprints | mapbuild2.c | 94.8%, both loop latches: pt.x store sunk below the compare, sq->x1 load hoisted |
| 0x0048a3e0 | GetObjectUID | objmap2.c | 89.5%, 20/191 strict; two displaced map loads; first 95 |
| 0x0048f0f0 | InitExitCheckBox | screens2.c | 0.8%, 118/119 strict; three-instruction zero-web shift; first 0 |

## Rules

- Read the note above each marker FIRST and do not repeat what it lists.
- Reconstruction-error pass before any variant search.
- After EVERY change `audit.py` the whole file: it must still end PASS and
  the count of `[OK]` lines must not drop; if it does, revert.
- `relocs.py` zero `MISMATCH`, `/W3` clean, before each commit.
- Marker stays `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <residual>)` until
  `audit.py` prints `[OK]`; then exactly `// FUNCTION: LEGOLAND 0x<VA>`.
- Retire honestly: if the lever does not move a body, add one paragraph to
  its note saying what you measured and move on.
- Do not run `verify.py` / `progress.py` / `coverage.py`; do not edit
  `tools/`, `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`.
- Commit to `scope/LL17` only; no push; no merge; **no Co-Authored-By
  trailer of any kind.**
