# Scope LL19 — partials wave C: render / cursor residuals on main

Branch `scope/LL19` from `origin/main` `001a1dda`. **PARTIAL scope**: edit only
the WIP bodies below and the notes above their markers (see
`docs/PARALLEL_CONTRACT.md`, "Extra rules for PARTIAL scopes"). Object
prefix `/tmp/sll19_`. Findings go in `docs/lanes/scope-ll19.md`.

## Functions (6)

| address | name | file | residual (from the marker) |
| --- | --- | --- | --- |
| 0x0045fad0 | DrawCursorSegmentB | cursorseg.c | 94.2%; the switch's default arm: original clones the exit epilogue as the chain's fall-through and keeps case 1 then case 2 |
| 0x0045fca0 | DrawCursorSegmentA | cursorseg.c | 77%; the SegmentB default-arm layout plus a scratch rotation and a folded pitch load in the diagonal loop |
| 0x0045ff00 | RenderCursor | bigrender.c | 39.9%, 273/454 strict; scratch rotation plus retired merge limit; first 48 |
| 0x004608c0 | PaintTileLayer | render4.c | 12.3%, 378/431 strict, 1322/1315 bytes; nonvolatile halfw home; first 14 |
| 0x00471ca0 | RemoveNewObjectMarker | fpui5.c | 90.6%, 5/53 strict; cursor-anchor floor; first 22 (scope I) |
| 0x004966a0 | UpdateSampleSource | sysmisc.c | 90.9%, 6/66 strict; scheduling/tail-merge floor; first 46 (scope I) |

## New levers since these notes were written (2026-09-08)

- **Appearance-count allocation** (scope LL14, `git show origin/scope/LL14:docs/lanes/scope-ll14.md`,
  "Closing 0x0046f9a0"): with callee-saved registers oversubscribed VC6 ranks
  candidates by static name-appearance count; a named temporary carrying one
  load to several consumers, or a tail re-reading a just-stored field, moves
  the ranking.
- **Block-boundary pin for x87 / single-use locals** (scope LL10,
  `git show origin/scope/LL10:docs/lanes/scope-ll10.md`, Mesh_DropBackFaces):
  VC6 forward-substitutes a single-use local into its consumer only within
  one basic block; a zero-cost empty integer test `if (k) ;` between the
  definitions and the consumer pins the definitions at their sites and
  re-phases everything downstream. Position matters.
- **Lazy reads** (scope LL12): a value the original reads inside a guarded
  block must be read there in the source; `dst = c ? a : b` vs if/else
  shifts the whole scratch phase.

## Rules

- Read the note above each marker FIRST and do not repeat what it lists.
- Reconstruction-error pass before any variant search.
- After EVERY change `audit.py` the whole file: PASS and the `[OK]` count
  must not drop; if it does, revert.
- `relocs.py` zero `MISMATCH`, `/W3` clean, before each commit.
- Marker stays `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <residual>)` until
  `audit.py` prints `[OK]`; then exactly `// FUNCTION: LEGOLAND 0x<VA>`.
- Retire honestly with a paragraph in the note.
- Do not run `verify.py` / `progress.py` / `coverage.py`; do not edit
  `tools/`, `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`.
- Commit to `scope/LL19` only; no push; no merge; **no Co-Authored-By
  trailer of any kind.** Never `git add -A`; add files by name.
