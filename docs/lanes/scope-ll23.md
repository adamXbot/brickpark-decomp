# Scope LL23 — partials wave F: the coaster span / raster family

Branch `scope/LL23`. Files `LEGOLAND/coastershade2.c`, `coaster11.c`,
`coaster12.c`, `coaster13.c`. Object prefix `/tmp/sll23_`.
Brief: `docs/SCOPE_LL23_coaster_span_raster.md`.

## Status

| address | name | file | insns | audit | marker | change this wave |
| --- | --- | --- | ---: | --- | --- | --- |
| 0x0041f8d0 | Span_FillFlat | coastershade2.c | 106 | **[OK]** | FUNCTION | 88 → **0** |
| 0x0041fba0 | Span_FillFlatZ | coastershade2.c | 136 | **[OK]** | FUNCTION | 62 → **0** |
| 0x0041fd80 | Span_FillShade | coastershade2.c | 162 | **[OK]** | FUNCTION | 106 → **0** |
| 0x0041ff80 | Span_FillShadeZ | coastershade2.c | 202 | **[OK]** | FUNCTION | 62 → **0** |
| 0x0041f050 | Span_ClipPlane | coaster11.c | 179 | 137 mis | WIP | 173 + ESCAPES → **137, no ESCAPES** |
| 0x00420200 | IntegrateSimpson | coastershade2.c | 81 | 2 mis | WIP | unchanged — **FLOOR re-confirmed** |
| 0x00428860 | TrackShade_FillPoly | coaster13.c | 254 | 147 mis | WIP | unchanged |
| 0x00423200 | Raster_AddSpanRecord | coaster12.c | 61 | 35 mis | WIP | unchanged — FLOOR re-confirmed |
| 0x00424050 | GetTrackSegment | coaster12.c | 87 | 34 mis | WIP | unchanged — FLOOR re-confirmed |
| 0x0041db90 | Route_GetMassAndPower | coaster11.c | 77 | 42 mis | WIP | unchanged — FLOOR re-confirmed |
| 0x0041c940 | BsRoute_Trace | coaster11.c | 130 | 105 mis | WIP | unchanged — **FLOOR verified, holds** |

**4 of 11 closed** (`coastershade2.c` goes 3/8 → 7/8). `audit.py` PASS on
all four files, `[OK]` counts 16 / 22 / 16 / 7, none dropped.
`relocs.py` zero MISMATCH on all four. `/W3` clean.

## The finding that matters: three levers move the whole span family

The four span fillers in `coastershade2.c` are one source template with
`ZBuffer_FillPoly` (schoolcar6.c 0x00423350, closed 2026-09-08). Its three
levers transfer verbatim, and each is worth 30-60 mismatches on every
sibling. A lever that closed one closed the next within a handful of
compiles.

- **The row pointer, the dead store and the pitch must be ONE aggregate
  local.** An aggregate local blocks reuse of a dead PARAMETER's home
  (LEVERS), so the three take real frame slots and the frame grows to the
  original's size — `sub esp,0x58` → `0x60` on Span_FillFlat, and the
  `{crow, zrow, pitch}` form gives 0x64 on the other three. `ylast`, `y`
  and `color` are then left to the argument slots, which is the original
  layout. **`/FAcs` shows this in one compile**: the equate list reads
  `_r$ = -16 / _color$ = 16 / _y$ = 20 / _ylast$ = -4`, i.e. exactly which
  local went to which dead parameter home. Before the aggregate the same
  list read `_row$ = 16 / _pitch$ = 8 / _color$ = 12` — three locals
  overlaid on tag/grad/n and only two real slots.
- **The interpolant pair is store, store, read-modify-write through the
  address-taken `ed[]`**, never `ed[k].x = e->a[0] - e->d[0]`:

  ```c
  ed[2].x = e->a[0];
  ed[3].x = e->d[0];
  ed[2].x -= ed[3].x;
  ```

  That lifts `e->a[0]` above the `e->dir` branch ahead of `e->d[0]`, and
  store-to-load forwarding folds the RMW back into one subtract, so the
  emitted shape is the original's shared load + duplicated `sub` with no
  extra instructions. On Span_FillFlat this single change took the body
  from 53/106 to **106/106**. On Span_FillShadeZ the else arm's three
  pairs reproduce the original's `[ebp-0x60]` scratch store-and-reload
  exactly — that reload IS the RMW.
- **`color` is a `short`, not an `int`.** VC6 then emits the original's
  partial write, `mov ax, word ptr [tab + idx*2]` followed by a DWORD
  store of eax whose upper half is left as `grad[0]`'s. `int color =
  (int)tab[...]` costs an extra `xor reg,reg` / `mov reg16`.
  (`unsigned short` is identical; the value is only read as `word ptr`.)
- **Consequence:** with the aggregate in place the free `volatile` reads of
  `y`, `dead` and `pitch` that the old WIP notes prescribed are no longer
  wanted — each is now one instruction long. Delete them.

### The new lever this scope adds

- **Frame homes are handed out in order of FIRST STORE.** `Span_FillShade`
  writes `flip = 0` and `ylast` back to back. With `flip = 0` first, flip
  takes the real slot at -8, `ylast` is pushed down to -0xc and the frame
  grows to 0x68; writing `ylast` first leaves flip to the dead `n`
  argument slot at +0x10 and the frame is the original's 0x64. That one
  statement swap took the body from 126/162 to **162/162**. VC6 still
  *schedules* the `mov dword ptr [ebp+0x10], 0` back up to the top of the
  function, so the source ORDER is the lever and the emitted position is
  not evidence of it. Declaration order is inert (measured, 5 orders).

### Setup-order facts measured on the way

- Both row pointers must be seeded from their globals BEFORE the
  `pitch * y` add and advanced with `+=`. `r.crow = g_raster_bits;` … then
  `r.crow += r.pitch * y;` is exact; folding it into one
  `r.crow = g_raster_bits + r.pitch * y;` (or naming base locals `cb`/`zb`
  and writing `r.crow = cb + r.pitch * y`) spills a base into the key
  argument slot and costs 1-3 instructions (139i / 137i vs 136i on
  FillFlatZ). The two-statement form is what keeps the bases in esi/edi
  and forces `n` to be re-read from its home, which is what the original
  does.
- On FillShadeZ, `crow` before `zrow` in the seed; `zrow` first costs 2.
- Span_FillFlat's winning setup order is
  `y, y1++, row=g_raster_bits, ylast, dead, pitch, polys++, color,
  key[n].y, row += pitch*y`; FillFlatZ inserts `zrow` before `crow` in the
  seed and `dz = grad[1]` after `pitch`; FillShade/ShadeZ put `flip = 0`
  after `ylast`.

## Span_ClipPlane (0x0041f050) — the ESCAPES cause and fix

**Cause.** Not a matching residual: the reconstruction really was longer
than the original. The old body compiled to **189 instructions** against
the original's 179, so the trimmed body ended mid-epilogue and the
early-out's `jl` targeted an address the original does not have. The ten
extra instructions were all register pins added by LL3, none of which the
original contains:

| pin | cost |
| --- | ---: |
| `destrel = dst - nxt` + a volatile store to `prev_abs.pad` in the loop header | 3 |
| `pa`/`na` colouring copies before the classify | 2 |
| a reload of the plane pointer in the header (ours pinned it to a callee-saved) | 1 |
| `out_n = 0` materialised in a register (`xor` + `mov`) instead of `mov [slot],0` | 1 |
| the ENTER arm's rotated lerp loop and its delta reload | 3 |

**Fix.** Rebuilt from the disassembly rather than un-pinning the old body
(removing the destrel pin alone made it *worse*, 192i):

- both lerp arms written index-based (`prev[k]`, `nxt[k]`, `dst[k]`), which
  is what gives the original's ONE cursor plus two memory offsets and a
  separate counter, instead of the explicit-offset form's second
  strength-reduced destination IV;
- LEAVE's `int k = 0` inside the `g_span_vtx` guard (so it keeps the
  `xor ebp,ebp` / `inc ebp` counter) and ENTER's outside it (so it spills
  to `[esp+0x18]` and frees ebp for the delta) — the original's asymmetry;
- `n` reused as the loop counter (`while (--n)`) instead of a separate
  `left`, with `left = n` hoisted to just after `in[n] = in[0]`;
- statement order `dst = *cursor; in[n] = in[0]; left = n; nxt = in[0];`;
- the block chain kept as the nested `if (cls != ENTER) { if (cls != BOTH)
  { if (cls == LEAVE) … } else BOTH } else ENTER`, which is what puts
  LEAVE on the fall-through and BOTH/ENTER after it, as the original does.
  A flat `else if` chain exiles LEAVE and puts ENTER on the fall-through.
- the 5th parameter typed `ClipPlane* plane` (the call site passes `void*`,
  so no cast and no `/W3` warning).

Result **179i / 588B vs 593B, no ESCAPES, audit 173 → 137** (index-for-index
41/179; alignment 33.5%, equal to the old pinned body's alignment with six
fewer instructions and a correct extent).

**Remaining residual, honestly.** `sub esp,0x24` where the original has
`0x2c`: we get 7 local slots plus one hole, the original 9 plus the unused
`+0x30`/`+0x38` pair, because the two dead parameter homes go to
`bits`/`pabs`+`k` here and to `in`/`t` in the original; and `plane` takes
ebx as a whole-function web instead of the original's ecx reloaded from
`[esp+0x50]` after each `__ftol` loop. Every slot-numbered operand differs
as a result, which is most of the 137.

Ruled out this wave (each measured): declaration order (5 orders, inert);
`pabs` as a 12-byte padded struct, address-taken or not (frame unchanged);
`ClipPlane* volatile plane` — it *does* free ebp and land `xor ebp,ebp`,
but the three forced reloads per classify put it at 183i; the Codex-F
operand-alias lever on the `fild` pair (`q = nxt`, a `char*` cast on
`nxt[1]`, an alias on `plane`) — inert, and unnecessary because the
index-based rebuild already emits `fild [+8]` before `fild [+4]`;
every lerp spelling that might flip the cursor base from `nxt` to `prev`
(separate temps, pre-loaded delta, negated delta, repeated subscript) —
inert, **VC6 always bases the rebased cursor on the SUBTRACTION'S LEFT
OPERAND**; `*(volatile int*)&out_n = 0` (177i, worse); `volatile int
out_n` (180i).

## IntegrateSimpson (0x00420200) — retired at 79/81

Two mismatches, and they are a codegen template, not a spelling. The
original is `call [ebp+8]; fstp fa; add esp,4`; `#pragma optimize("g",off)`
always emits `call; add esp,4; fstp`. New negatives on top of LL4's list:

- the whole `#pragma optimize` letter matrix, with and without the empty
  `__asm {}`: `"y"` off alone gives an EBP frame but 85i; `"a"` off and
  `"w"` off 82i; `"gt"` off 85i; `"g"`, `"gy"`, `"gp"`, `"ga"`, `"gw"`,
  `"gs"` and `""` off are all the same 81i/258B body;
- store forms: `f.s = (f.fa = fn(a)) + fn(b)` (82i), `fn(b) + (f.fa =
  fn(a))` (82i), the comma statement, a `volatile float*` destination
  (84i), a block temp (83i), `*(float*)&f.fa`, a NON-volatile frame
  struct, `f.s = f.fa; f.s = f.s + fn(b)` (83i), `f.s = fn(b); f.s +=
  f.fa` (82i), a duplicated store (83i), `if (1)` / `do {} while (0)`
  wrappers (84i), a block-local copy of the function pointer (83i), an
  argument temp (83i);
- `fa` as a separate volatile or non-volatile local declared before or
  after a 9-field frame struct — all four give 79/81 with correct homes,
  so home placement is NOT the constraint.

**Correction to LL4's note:** the empty `__asm {}` is now INERT.
`#pragma optimize("g", off)` alone produces the identical 81i/258B body.
The `__asm` was doing nothing for the frame.

## The other five: floors re-verified, not re-ground

- **BsRoute_Trace (105).** Floor HOLDS and is now double-attested: the twin
  `JungleCruise_TraceRoute` (jcroute.c 0x00437260) audits at exactly the
  same 130i/339B and 105 mismatches. Two independent reconstructions of the
  same source reach the same allocation floor, so it is not a spelling miss
  in either. Not re-ground.
- **Route_GetMassAndPower (42).** The Codex-F scope-V CANCELLED-PAIR anchor
  is inert here. With an anchor already in a register (`p`, `rt`, `mass`,
  `power`, `&fr`) the pair folds completely in the front end — 77i/259B and
  65/77 bit-identical to the tip, so the web is not kept separate the way
  scope V's `t.x = bx` was. With a link-time address constant assigned to a
  second struct member first (five tried), the anchor costs a real
  `mov reg,imm32`: 78i/262B, past the extent. **The lever needs an anchor
  the body already materialises for its own reasons; this body has none
  live at the `&p->head` site.** That is the reusable form of the rule.
- **Raster_AddSpanRecord (35).** The cancelled pair cannot split `saved`
  from `cur` (address-constant anchor 63i; `n` anchor folds to 61i/174B and
  scores below the tip; `volatile` reload 64i and ESCAPES; `saved` first
  62i). The `keys` cursor is inert to spelling — five forms all reproduce
  the `lea edx,[eax-8]` biased cursor byte for byte.
- **GetTrackSegment (34).** The body is really **84** instructions, not 87:
  audit's `87i/232B` counts three bytes of COMDAT padding. The missing
  block is the four-instruction `fail2` minus the extra `jmp` our inverted
  head latch emits. New negative: writing the original's structure
  literally — `goto fail1` from the head-empty test jumping INTO the
  `state == 2` block, to a label on its trailing `return 0` — is
  bit-identical to the tip. VC6 canonicalises the two identical `return 0`
  blocks before layout, so no source form separates them.
- **TrackShade_FillPoly (147).** The family levers do NOT transfer, and the
  reason is instructive: its frame is already right (0x70, every
  `ShadeSetup` home and argument-slot reuse matches through insn 91), it
  has no `color` local, and the store/store/RMW interpolant form is a
  REGRESSION (grouped 265i + ESCAPES, paired 262i + ESCAPES, dir-arm-only
  255i, against the tip's 254i). Its residual is two register-colouring
  facts that the `coastershade2.c` fillers cannot have because their whole
  loop body is one `__asm` block: LL7's nshade window, and `ed[0].x` /
  `ed[2].x` being FORWARDED out of the if/else into the `while` head where
  the original reloads them from memory and does `inc dword ptr [ebp-4]`.
  Measured: `(*(int volatile*)&y)++` gives the memory increment but 259i;
  volatile reloads of the two interpolants 256i; both 255i; plain `y++`
  252i; the span-filler row-pointer shape puts the nshade load between the
  two row stores but in eax with the stores reordered (84/254).

## Which levers transferred between siblings — the reusable answer

| lever | FillFlat | FillFlatZ | FillShade | FillShadeZ | TrackShade | ClipPlane |
| --- | --- | --- | --- | --- | --- | --- |
| row/dead/pitch aggregate | **yes** | **yes** | **yes** | **yes** | n/a (already right) | n/a |
| store/store/RMW interpolants | **yes** | **yes** | **yes** | **yes** | **regression** | n/a |
| `short` colour | **yes** | **yes** | n/a | n/a | n/a | n/a |
| first-store order to frame homes | n/a | n/a | **yes** | **yes** | no | not tried |
| index-based lerp / one cursor | n/a | n/a | n/a | n/a | n/a | **yes** |
| scope-V cancelled-pair anchor | — | — | — | — | — | inert (Mass, AddSpan) |
| Codex-F operand alias | — | — | — | — | — | inert |

The transfer is strongest **within one file and one frame shape**: all four
`coastershade2.c` fillers share a 0x50 `ed[4]` array plus a 4- or 5-dword
head, and the same three edits closed all four. `TrackShade_FillPoly` looks
like a fifth sibling and shares the `__asm` fill, but its loop head is C
rather than assembly, which changes the residual class completely — that is
the boundary of the family, and it is worth knowing before the next wave
tries to push it.

## Extern-type divergence introduced

- `Span_ClipPlane`'s 5th parameter is now `ClipPlane*` (was `void*`) in
  both the forward declaration at the top of `coaster11.c` and the
  definition. `Raster_ClipAgainstPlanes` (0x0041f2b0, exact) passes a
  `void*`, which converts implicitly — no cast, no `/W3` warning, and that
  body is still `[OK]` with 0 mismatches.

## Method note

`tools/matchfull.py`'s difflib alignment and `audit.py`'s index-for-index
count can disagree by a lot when the instruction COUNT is wrong: the old
Span_ClipPlane read 33.9% aligned but 6/179 index-for-index. Steer on the
index count once the body is the right length, and on alignment before
that; a body whose count is right and whose blocks are in the original's
order jumps from single digits to 40+ index matches on its own.
