# Scope LL19 — partials wave C: render / cursor residuals

Branch `scope/LL19` from `origin/main` `001a1dda`. PARTIAL scope over six WIP
bodies (brief: `docs/SCOPE_LL19_partials_render_cursor.md`). Object prefix
`/tmp/sll19_`. Every edit re-audited the whole file: `audit.py` PASS and the
`[OK]` count unchanged on every file touched.

## Status

| address | name | file | before | after | marker | residual |
| --- | --- | --- | --- | --- | --- | --- |
| 0x0045fad0 | DrawCursorSegmentB | cursorseg.c | 94.2% (145/154) | 94.2%, at its floor | WIP | switch default-arm layout, 9 LCS (strict = rb = ob) |
| 0x0045fca0 | DrawCursorSegmentA | cursorseg.c | 77.7% (146/188), 194i | **95.2% (180/189), 195i exact** | WIP | the same 9-instruction layout only |
| 0x0045ff00 | RenderCursor | bigrender.c | 39.9% | (in progress) | WIP | |
| 0x004608c0 | PaintTileLayer | render4.c | 12.3% | (pending) | WIP | |
| 0x00471ca0 | RemoveNewObjectMarker | fpui5.c | 5/53 strict | (pending) | WIP | |
| 0x004966a0 | UpdateSampleSource | sysmisc.c | 6/66 strict | (pending) | WIP | |

## 0x0045fad0 DrawCursorSegmentB — at its floor, mechanism identified

Residual (LCS, all blinds equal): the compare-chain switch's default arm. The
original's chain `sub eax,0 / je / dec / je / dec / je` falls through into a
CLONED epilogue (the default `return`) and then places case 1 (`dir = -1;
jmp`) before case 2 (`dir = 1`, adjacent to the shared diagonal loop); ours
inverts the last test (`jne end`), so case 2 falls through and case 1 is the
loop's fall-through.

Method: a scratch harness (`/tmp/sll19/exp/gen.py`) compiled ~40 variants of
the function in one object and printed each one's chain region; the
in-tree body was only edited for the measured winner. Findings:

- **The default arm is threaded because it is empty.** A void `return` is a
  block whose only content is the jump to the exit; VC6 threads it before
  layout and inverts the last test. Any real instruction in the default arm
  (`*(volatile int*)&flip`, a store to a global) keeps the block and the
  chain then falls through into a cloned epilogue exactly as the original.
  So the shape needs a code-bearing default, and the original's default has
  no code. Every zero-cost pin was threaded with the block: `if (h) ;`,
  `if (vs) ;`, `if (h) return;`, `while (0) ;`, `for (;;) return;`,
  `h = h;`, dead stores to `flip`/`pat[0]`/`phase`/`x`/`c`/`p`, dead reads of
  `pat[0]`/`col[0]`/`vs->pitch`, an unreferenced label, `{}`, a comma of
  casts, `goto` to the post-diagonal `return`, a nested `switch` default, a
  shared `case 3:`. `__asm { }` keeps the block but gives the function an EBP
  frame (LEVERS FR10).
- **Layout-invariant spellings** (all byte-identical to the baseline): the
  diagonal loop as the switch's break join; the vertical loop inside case 0
  with `return` or `break`; every order of {0, 1, 2, default} with and
  without a default; `dir` pre-assigned before the switch (VC6 hoists the
  store above the chain and never sinks it); a switch of `goto`s dispatching
  to labelled arms.
- **Second obstacle: arm order.** Whenever the default does fall through,
  VC6 lays the remaining arms in DESCENDING case value (2, then 1, case 0
  last) regardless of source order or which arm carries the `goto`. The four
  matched chain-fall-through templates in the binary agree
  (`GetTransparentColour`/`GetNearestColour` 0x0044e690/0x0044e6c0,
  `CurLevelFlags` 0x00478610, `SelectFont` 0x00454b40: default first, then
  cases high to low). The original has case 1 BEFORE case 2. No construct
  produced that order.
- Flags do not reach it: /O1, /Os, /Ox, /Ob0, /Oy-, /Oa, /Ow, /Gf, /Gs, /GB,
  /G3–/G6, /Zp1 are all equal or worse (a scan of the text section confirms
  the `dec / je / pops / add esp / ret` chain fall-through exists only in
  the two cursor fills).

## 0x0045fca0 DrawCursorSegmentA — 77.7% -> 95.2%, count exact

One spelling: the diagonal loop's head is `h++; while (h--)`, not
`i = h + 1; while (i--)`. In SegmentB the two are identical (x and y live in
ebp/ebx); here, with x, y, phase, flip and the count all memory-homed, the
extra name made VC6 load `h` into edx and form `h + 1` with `lea`, and that
one scratch choice rotated eax/ecx/edx through the whole loop (count copy,
PtInRect argument loads, pattern index, x step) and re-scheduled the
`if (flip)` block so the pitch load folded into the add. With `h` stepped in
place the loop, its preheader, the `mov ecx,[eax] / mov [y],edx / add
esi,ecx` pitch shape and the vertical loop are instruction-, register- and
offset-exact; the LCS residual is exactly SegmentB's 9-instruction layout.

Measured on the way (matchfull): `*(volatile int*)&h + 1` in the head 180/190
(same rotation, one byte off); `while (h-- >= 0)` 176/188; flip block as
`q = p; p += vs->pitch; y++` 147/189 (only the fold moves); named `pitch`
local 146; `y = y + 1` 146; `for (i = h + 1; i--; )` 146; free volatile reads
of flip 86, phase 146, y 146, x 133, dir 88, vs 95.

Lever for LEVERS: **a counted-down COPY of a parameter (`i = h + 1;
while (i--)`) versus stepping the parameter in place (`h++; while (h--)`)
is inert while the loop's other values are enregistered, and a whole-loop
scratch rotation once they are memory-homed** — the copy's single extra name
is a `lea` and a register choice at the loop head.
