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
| 0x0045ff00 | RenderCursor | bigrender.c | 39.9% (273/454) | 39.9%, retired | WIP | tile-loop scratch colouring (23) + the retired depth-5 merge (250) |
| 0x004608c0 | PaintTileLayer | render4.c | 12.3% (378/431) | 12.3%, kept at scope I's form | WIP | head allocation + halfw frame rank; a head fix exists but regresses the perimeter pass |
| 0x00471ca0 | RemoveNewObjectMarker | fpui5.c | 5/53 strict | 5/53, retired | WIP | cursor anchor (rb 5 = strict 5) |
| 0x004966a0 | UpdateSampleSource | sysmisc.c | 6/66 strict | **5/66 strict, 175B exact**, retired | WIP | one ecx/edx swap in case 3, rb 0 |

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

## 0x0045ff00 RenderCursor — retired at scope I's floor

Residual (a), the tile loop's scratch colouring (temp1 always `eax`, the
original `edx`), re-probed with the two new levers; every build byte-identical
to the committed body (454i/1438B, 273):

- block-boundary pin `if (k) ;` at five positions (before the t.x load,
  between the two loads, after `t.x += x`, after `t.y += y` with k = i / x /
  th, at the top of the outer loop body, before the outer `if`);
- eliminated-temporary spellings before the loop: `Cursor* cur = c`,
  `Rect* first = &c->rect`, pointer locals for &saved/&view, `memcpy` for the
  struct copy, `flags = c->flags` read once, a self-assigned `MapHdr* m2`.
- `int x1 = r.right` as the inner bound: 275 (worse); a volatile read of
  `c->flags` at the 0x10 test: 411 (worse).

Residual (b) is the documented build difference (depth-5 cross-jump); not
re-ground.

## 0x004608c0 PaintTileLayer — kept at scope I's form, model recovered

~25 spellings (`/tmp/sll19/var.py` batch runner over render4.c). A frame model
that fits every measurement: the address-taken aggregate at the top, then all
other locals top-down in ASCENDING weight (original: tile +0x3c, dx +0x38,
dy +0x34, rowy +0x30, rowx +0x2c, ylimit +0x28, xlimit +0x24, halfw +0x20,
e +0x1c, py+halfh +0x18, tw +0x14, th +0x10).

- **dx/dy as plain locals** (or a non-address-taken two-int aggregate,
  identical): the head becomes exact to index 27 (was 14) — the original's
  `sub / mov [dx] / mov ecx,[scroll+4] / sub / mov [dy]` is a
  spill-at-definition, not a struct-field store — and the perimeter loop's
  `ebp = dy / esi = dx` loads match. But the same change lets VC6 hoist the
  bridge-offset loads above the first push in the perimeter pass (`mov
  ecx,[ox] / add ecx,edi` against the original's `push / mov edx,[ox] / lea
  ecx,[edi+edx]`), and the body measures worse overall (LCS strict 72 / rb 26
  against 65 / 22; positional 390 against 378). Not applied. Around it, all
  inert or worse: `ox + x` operand order, y computed before x, x/y hoisted to
  the outer block, a named `y + oy`, volatile dx/dy reads at the use.
- **halfw**: as a plain local it always regrows the derived IV (frame 0x38):
  `unsigned`, two inline `px + halfw`, `(void)&halfw`, `int* phw = &halfw`
  with or without `*phw` at the use. A real address escape (`if (0)
  DrawPathTileOverlay((Pos*)&halfw, ...)`) refuses the IV and places halfw at
  +0x28 — above ylimit/xlimit, its weight now counted as a memory variable.
- Inline loop bounds instead of ylimit/xlimit recompute through `view` in the
  latches (worse); `Pos row` for rowx/rowy 392; halfw declared first or last
  inert. The tw -> ebp choice at index 27 was not reached.

## 0x00471ca0 RemoveNewObjectMarker — retired

The pin `if (k) ;` at five positions (between the two copies with k = i and
k = j, after them, before the loop, and with the def copy first and the pin
between) is byte-identical at 5/53; the last form anchors the cursor on def
(`lea eax,[esi-0x4c]`) and is still 5. The recorded anchor/schedule coupling
stands.

## 0x004966a0 UpdateSampleSource — 6 -> 5, 175/175 bytes, retired

`int py = s->src.pos.y; if (py) ;` replaces the volatile read of pos.y. The
zero-cost pin keeps the pos.y load on the near side of the p.x store as the
volatile did but no longer pins it FIRST, so the three loads come out in the
original's order (scroll_x / pos.x / pos.y) and the block is byte-exact in
length. The last 5 are one scratch swap (scroll_x in ecx and pos.x in edx
against the original's edx / ecx; sar / sub / store follow), rb 0. Worse
around it: a volatile g_scroll_x read with the pin (6) or without (12), the
pin on the volatile pos.y read (6), `if (s) ;` as the pin (5, identical), a
named px with or without a volatile read (28, loses the case-1 tail merge), a
named `sx = g_scroll_x >> 8` (28), sx/px/py all named with plain or volatile
stores (28), plain stores with the pin between them (32).

Lever for LEVERS: **the block-boundary pin is a gentler order tool than a
free volatile read** — a volatile read pins its load FIRST in the block; `t =
x; if (t) ;` only keeps the load on the near side of the boundary and lets
the scheduler place the rest, which is what this block needed.

## Gates at the tip

Per file: `audit.py` PASS with the `[OK]` count unchanged (cursorseg 0,
bigrender 4, render4 2, fpui5 4, sysmisc 6); `relocs.py` zero MISMATCH;
`/W3` clean.
