# Scope PORT-B3 — the hand-written RLE sprite painters ported to C

Branch `scope/PORT-B3` off `main` `3ca919cc`. Goal: the ten
`__declspec(naked)` type-3 RLE painters in `LEGOLAND/rlepaint.c` (eight) and
`LEGOLAND/rlepaint2.c` (two) are `LL_UNPORTED_ASM()` traps in their
`#else` arms, and `ShowTitleScreen -> PrintSprite -> RenderSprite ->
SoftBlitSprite -> SoftBlitRLEPlain -> RLEPaintHit` aborts on the first one.
Port them to C so the title screen can draw.

Rules kept: the `#ifndef LEGOLAND_PORTABLE` arm is byte-for-byte the original
`__asm`; all new C is inside the existing `#else` arm; no preprocessor line
moved between a `// FUNCTION:` marker and its signature.

## 1. The stream format, as read from the asm

Three blocks per frame, from the `RleFrame` header in `softblit2.c`
(`+0` total size, `+4` pixel-word count, `+8` padded length-byte count,
`+0xc` padded opcode count, ignored by the dispatcher):

| block | at | content |
| --- | --- | --- |
| A | `frame+0x10` | u16 PIXELS — raw 16-bpp words, not palette indices |
| B | `A + 2*np` | u8 run lengths |
| C | `B + nl` | packed 2-bit control codes, 16 per dword, LSB first |

`softblit2.c`'s own header calls `+4` "u16 control words"; that name is wrong
and `docs/runtime/presentation-data.md` §2 supersedes it. The leaf prologues
settle it: `esi` (A) is read with `mov ax,word ptr [esi]` and stored straight
into the surface, `[esp+1ch]` (B) with `mov cl,byte ptr [eax]`, and `edx` (C)
with `mov ebp,dword ptr [edx]`.

**The rotating mask.** Every leaf opens `mov ebx,3` and reads a code as

```
mov ebp,[edx] / and ebp,ebx / rol ebx,2 / mov ecx,ebx / and ebx,1
test ebp,0aaaaaaaah / lea edx,[edx+ebx*4] / mov ebx,ecx
```

The code is never shifted down to bit 0: the **isolated field** `word & mask`
is kept, and the two halves of the pair are asked for with
`test ebp,0aaaaaaaah` (high bit) and `test ebp,55555555h` (low bit). `edx`
advances four bytes exactly when the rotate has brought the mask back to 3,
i.e. after every sixteenth code. Codes are **not** realigned per row.
`ll_rle_open` / `ll_rle_code` / `LL_RLE_HI` / `LL_RLE_LO` in
`portable/hostwin/include/ll_portable.h` are that reader, one call per asm
code read. The dword load goes through `__builtin_memcpy` because C is only
guaranteed 2-byte aligned (A is `frame+0x10`, `np` may be odd, and only B's
length is padded to a multiple of four).

**The grammar**, identical in all ten leaves:

| primary | then | operation |
| --- | --- | --- |
| 0 or 1 | — | read one u16 from A, emit one pixel |
| 2 | — | advance one pixel, write nothing |
| 3 | B byte = 0 | END OF ROW; consumes no secondary code |
| 3 | B byte L>0, secondary 0 | read and emit L u16 pixels from A |
| 3 | L>0, secondary 1 | read ONE u16 from A, emit it L times |
| 3 | L>0, secondary 2 or 3 | advance L pixels, write nothing |

There is no colour key anywhere: transparency IS the control stream, and
pixel value zero is opaque black.

**Four passes per leaf** (which passes exist depends on the clip shape):
`top` whole rows consumed with no output; then per row, skip `left` source
pixels; paint the visible span; consume the right tail to the end-of-row
marker so all three streams stay aligned for the next row. End-of-row adds
`pitch` BYTES to the saved row base, decrements the row counter and restores
the left/width counters.

**The spare argument slots are scratch.** The leaves have no stack frame at
all (`__declspec(naked)`, four pushes, everything else in the argument area),
so they save the initial `left` into the `top` slot (`[esp+2ch]`) and the
initial `w` into the `spare` slot (`[esp+38h]`) and restore both at each
end-of-row. That is why the dispatcher passes a zero `spare`.

**The hit test** (the four `Hit` leaves and both `rlepaint2.c` leaves):

* a singleton (primary 0/1) hits when the output address EQUALS the mouse
  pixel address — `cmp eax,edi / jne / or g_blit_hit,1`;
* a drawn opaque run hits when
  `(unsigned)((int)(mouse - dst) >> 1) < (unsigned)len` —
  `sub eax,edi / sar eax,1 / cmp eax,ecx / jae`. A SIGNED halving compared
  UNSIGNED, so a mouse before the run wraps huge and cannot hit.
  `ll_rle_hit_run` is that test.
* transparent runs and clipped-away pixels never hit; the four no-hit leaves
  never touch `g_blit_hit`.

**The primary-code-1 defect.** Five top-skip loops (HitL, HitR, Hit, and both
`rlepaint2.c` leaves) spell the literal case as

```
test ebp,0aaaaaaaah / jne L_next_test / add esi,2
L_next_test: test ebp,55555555h / je L_top
```

with no unconditional jump after `add esi,2`, so a primary code **1** in the
top-skip region consumes its literal word and then falls into escape
processing and eats a B length byte. Code 0 is fine. HitLR and all four
no-hit leaves have the `jmp` and handle both literal codes. The shipped
assets contain no primary 1 (the 5,285-frame scan in
`docs/runtime/presentation-data.md` §5 counts primary 0/2/3 only), so the
defect is dormant — but it is in the executable, so it is in the C.

## 2. Per-painter notes

All ten have the same skeleton, so the table records only what each one adds.
"Passes" are top-skip (T), left skip (L), visible span (V), row tail (R).

| leaf | VA | passes | hit test | top-skip |
| --- | --- | --- | --- | --- |
| `RLEPaintFast` | `0x00467f00` | T, V | none | correct |
| `RLEPaintHit` | `0x00467640` | T, V | singleton + run | **defective** |
| `RLEPaintClipR` | `0x00467d10` | T, V(budget), R | none | correct |
| `RLEPaintHitClipR` | `0x004673f0` | T, V(budget), R | singleton + run | **defective** |
| `RLEPaintClipL` | `0x00467b00` | T, L, V, R=none | none | correct |
| `RLEPaintHitClipL` | `0x00467180` | T, L, V | singleton + run + L split | **defective** |
| `RLEPaintClipLR` | `0x004677b0` | T, L, V(budget), R | none | correct |
| `RLEPaintHitClipLR` | `0x00466d80` | T, L, V(budget), R | all spans | correct |
| `SoftBlitRLEFrameRecolour` | `0x00468040` | T, L, V(budget), R | run only, full length | **defective** |
| `SoftBlitRLEFrame` | `0x00468410` | T, L, V(budget), R | run only, full length | **defective** |

### RLEPaintFast `0x00467f00` — 114 instructions

The asm: `edi` is the output pixel, `esi` the A stream, `[esp+1ch]` the B
cursor, `edx`/`ebx` the control reader, `[esp+14h]` the saved row base,
`[esp+24h]` the row counter. Two loops: `L_467F19` consumes `top` rows,
`L_467F82` paints. Row end (`L_468013`) adds `pitch` to the saved base,
reloads `edi` from it, decrements the counter and returns at zero.

The C is that, literally. `dp++` / `dp--` stand for the asm's
`lea edi,[edi+2]` / `sub edi,2`, which it does *before* knowing whether the
code is a transparent single or an escape and then undoes.

### RLEPaintHit `0x00467640` — 122 instructions

Fast plus two hit tests: `cmp mouse,edi / jne / or g_blit_hit,1` on a
singleton at the address about to be written, and `ll_rle_hit_run` before
both opaque run kinds (the asm puts it above the `test 55555555h` that
separates literal from repeat, so one test covers both). Transparent runs
are below the `jne`, so they never hit.

Its top-skip carries the primary-code-1 defect. `left`, `w` and `spare` are
dead arguments here.

### RLEPaintClipR `0x00467d10` — 167 instructions

`left` is known zero (the dispatcher's fourth case), so there is no skip
pass. `w` is a per-row pixel budget in its own argument slot, decremented per
emitted pixel; the initial value is parked in the `spare` slot and restored at
each row end. A run longer than the budget is cut: the literal case copies
`budget` words and then advances A over the `n - budget` it did not write
(`neg eax / lea esi,[esi+eax*2]`), the repeat case just stops. Either way the
row jumps straight to the tail pass, which consumes the remaining codes with
no output so the next row starts aligned.

### RLEPaintClipL `0x00467b00` — 180 instructions

The skip pass advances **both** pointers: the dispatcher biased `dst` by
`-src.left`, so stepping the output over the clipped pixels is what lands the
first painted pixel on `dst->x`. A run crossing the left edge is split —
`clipped = left + n` pixels stepped over, `-left` drawn. There is no budget at
all: the second pass paints to the end-of-row marker, so this leaf has no tail
pass. Note the skip pass has **no entry guard**, so it always runs one
iteration; the dispatcher guarantees `left >= 1`.

### RLEPaintClipLR `0x004677b0` — 271 instructions

ClipL's skip pass (with an entry guard) followed by ClipR's budgeted pass.
What only this shape needs: a run that crosses the left edge also *pays* for
the pixels it makes visible, `add [esp+34h],eax` with `eax` the negative
leftover. When that takes the budget to `<= 0` the same run is cut a second
time at the right edge in the same step, and the count actually written works
out — `sub ecx,eax / add ecx,budget` — to the budget the row had when the run
started. A transparent run that crosses the left edge pays the same way but
draws nothing.

### RLEPaintHitClipR / HitClipL / HitClipLR

Their no-hit siblings plus the tests. What the asm is careful about, and the C
mirrors, is *which count* each test uses: the full run length when the run
fits, the budget when the right edge cut it (the asm reloads `mouse` from
`[esp+40h]` there because it has just `push`ed the overflow), and the
remainder when the left edge cut it. HitClipLR is the only Hit leaf whose
top-skip is correct.

### SoftBlitRLEFrameRecolour `0x00468040` / SoftBlitRLEFrame `0x00468410`

Both are the HitClipLR shape. Every opaque store becomes
`and ax, word ptr [g_sp_recolour]` — the **low word only** — and the
highlight leaf adds `shr ax,1`, a logical 16-bit shift, after each one. A
normalised diff of the two asm bodies differs only in label names and nine
inserted `shr ax,1`, which is why the two C bodies are the same code with one
extra `>> 1`. Literal runs cannot be `rep movsw` because of the per-pixel
mask, so they are software loops; repeat runs mask the word once and keep
`rep stosw`.

Three hit-test divergences from the plain Hit leaves, all kept:

1. a singleton is **never** tested against the mouse;
2. the remainder of a run crossing the **left** clip is not tested;
3. the one test they do have sits *before* the right-edge reduction, so it
   uses the run's **original** length — a mouse in the unwritten right tail
   still sets the flag, while the plain Hit leaf (which tests what it wrote)
   does not.

### One thing worth recording about the run loops

The asm mixes `rep movsw`/`rep stosw` (a count of zero is a no-op) with
explicit `dec ecx / jne` loops (a count of zero would wrap to 2^32). The C
uses `do { } while (--n)` throughout, which is safe because every reachable
count is provably nonzero: `n = *lp++` is checked against zero before use;
`-lskip` is positive because that branch is taken only when `lskip < 0`;
`n - clipped + budget` reduces to the budget the row had entering the run,
and the budget is `> 0` at every point a run can start (the only path that
can drive it to `<= 0` without a loop test is a transparent run crossing the
left edge, and that also drives `lskip` negative, which ends the skip pass
straight into the `budget <= 0` test).

## 3. The test

`portable/tests/test_rle_paint.c`, subcommand `rle_paint`, registered for
both toolchains. **43 checks,
0 failed.**

The sprite is declared once as a list of operations per row — one pixel
(code 0 and code 1), one transparent pixel, a literal run, a repeat run, a
transparent run — covering every line of the grammar table plus the two
degenerate rows (wholly transparent, a single full-width run). Two unrelated
routines read that list:

* `emit_frame()` builds A, B and C the way the assets are built, including the
  `put_code` bit addressing (`byte i/4`, `bits 2*(i%4)`) that the rotating
  mask has to agree with;
* `expect()` writes the pixels straight into a reference image at the column
  each operation lands on, applying only the leaf's clip range and the
  recolour mask.

So a leaf passes only if the encode, the decode, the run arithmetic, the
clipping and the row stepping all agree with a direct construction of the same
picture. The hand-counted stream sizes (39 pixel words, 21 length bytes, 42
codes) are checked too, so a bug in `emit_frame` cannot hide behind a matching
bug in `expect`.

Beyond the images it checks: the no-hit leaves never touch `g_blit_hit`; a
mouse on a transparent pixel, inside the left clip, or in the clipped right
tail does not hit; all four Hit leaves agree on one pixel; each of the three
recolour divergences above against the plain leaf on the *same* pixel; and
that the primary-code-1 top-skip defect is observable (Fast and HitClipLR
paint the reference picture, Hit does not).

### And one REAL sprite

`tools/oracle_rlepaint.py` decodes one 16-bpp COMP frame out of
`gamedata/disc/Graphics1.res` with the clean-room reader already in
`tools/comp.py` (`_Bits2`, `parse_header` and the `_decode16` grammar walk)
and emits the member's identity, the byte range of frame 0, a sentinel, and
two FNV-1a 64 digests — the whole image, and the same image cut at half
width. The test reads those bytes with `fopen`/`fread`, paints them with
`RLEPaintFast` and with `RLEPaintClipR`, and digests the scratch surface.
**Both match the Python decode exactly.**

The member is chosen deterministically: the first, in archive order, that
uses the literal single, the escape, the literal run and the repeat run,
preferring one that also has a transparent opcode. That lands on
`erase it.lls`, 34x30, whose frame 0 has 12 literal singles, 7 transparent
singles, 235 escapes and 91/90/24 literal/repeat/transparent runs — every
opcode the shipped assets ever use, and the same member the headless trace
shows the title screen loading (`SetFilePointer(4, 1335692) /
ReadFile(4, 1402 bytes)`).

Two things worth recording from building it:

* the sentinel that means "the painter did not write here" has to be picked
  **per frame**. A fixed one does not work: 460 of `Graphics2.res`'s 530
  16-bpp members contain any given constant, so `0x1234` is a real pixel in
  most of them and "untouched" stops being decidable. The oracle scans the
  decoded frame for an unused 16-bit value and emits it.
* most of `Graphics1.res`'s large frames are **fully opaque** — 429 16-bpp
  members, only 100 with any transparent opcode in frame 0, and none of the
  640x480 backgrounds. Preferring a transparent frame is what keeps the
  "advance without writing" half of the grammar in the check.

## 3b. The type-2 sibling: SoftBlitAnimPlain

`LEGOLAND/softblit2.c` 0x00464480 is the ImageRec **type-2** painter — the
LLS animation records the icon bar, the blokes and the front-end sprites use.
It was the last trap on the plain sprite path, so it went in too.

Its stream is NOT the type-3 one. `ebp` steps 32-bit control words, `ebx`
holds the word being consumed and `g_zb_bits` counts the codes left in it,
refilling from the next word when the count goes negative; the shift happens
unconditionally and is thrown away on a refill. A run length is not a
separate byte stream at all — it is spliced out of the LOW BYTE of the
control word (`shr ebx,2 / movzx ecx,bl / shr ebx,6`), costs FOUR code slots,
and if fewer than four are left the word is refilled and the length taken
from the NEW word's low byte, discarding whatever remained of the old one.
Pixels are 8-bit indices through `g_sp_pal16`. `ll_anim_open`,
`ll_anim_code`, `ll_anim_count` and `ll_anim_hit` in `ll_portable.h` are that
reader and the arm-and-test mouse pair.

One contract difference from the type-3 family, found by the test: the type-3
dispatcher biases the destination by `-src.left` and its painters then STEP
the output over every clipped pixel. This one does neither — `lc_one` and
`lc_step` advance only the index pointer — so the first VISIBLE source column
lands on `dst.x`. Same result, opposite mechanics.

Test: `portable/tests/test_anim_paint.c`, subcommand `anim_paint`, **10
checks, 0 failed**, both toolchains. It carries its own encoder for the
spliced-length packing, and the same op-list-versus-reference-image design as
the type-3 test.

### FINDING 1: a dead row-end branch in SoftBlitAnimPlain

The transparent-single case of the paint pass is

```
0x004649aa  dec edx
0x004649ab  add edi, 2      <- add writes ZF
0x004649ae  je  <endrow>
```

so the `je` tests the flags of the **add**, not of the `dec`. The output
pointer plus two is never zero, so that row-end branch is dead: a transparent
single always falls back into the paint loop whatever the budget is. With the
budget at zero the following codes still run, and because the run-versus-
budget test is `ja` (UNSIGNED), a budget that has gone negative compares
ABOVE any run length and the painter can write one pixel past the right clip.

The recolouring sibling `SoftBlitAnim` (0x00465240) spells the same case
`dec edx / je endrow / add edi,2` — branch first — which is what the plain
one evidently meant. Reproduced in the C, not fixed, with the reason in a
comment at the site.

### FINDING 2: `softblit.c`'s `row:` label is one instruction too late

**This one is a defect in the RECONSTRUCTION, not in the game**, and the byte
gate cannot see it. `SoftBlitAnim`'s row loop in the original is

```
0x00465443  mov edx,[0x7febac]      ; g_sp_h
0x0046544b  je  0x46582e            ; done
0x00465451  mov [0x7fea10],edx      ; g_sp_rows_left = edx   <-- the loop head
0x00465457  mov edx,[0x7fea50]      ; g_sp_left
   ...
0x00465828  jne 0x465451            ; the back edge targets the STORE
```

`LEGOLAND/softblit.c:740-741` puts `mov g_sp_rows_left, edx` ABOVE the `row:`
label, so the assembled `jne row` would land on 0x465457 and the store would
never run again: `g_sp_rows_left` would stay at `g_sp_h`, `nextrow` would
recompute `g_sp_h - 1` every time, and the loop would never end for a sprite
taller than one row.

`tools/audit.py` and `tools/match.py` cannot catch it because `norm()`
rewrites every direct branch target to `<t>` (match.py:170-189) — a `jne` to
the wrong label normalises to the same text and the instruction is the same
length, so the body still reports 415i/1544B mismatch=0. The cross-check that
does catch it is the sibling: `SoftBlitAnimPlain`'s original back edge
(`0x00464a5e jne 0x4646a1`) also targets its store, and `softblit2.c` places
its `row:` label correctly, before it.

Fix: move `row:` in `LEGOLAND/softblit.c` up one line, above
`mov g_sp_rows_left, edx`. That is a matching-lane edit to the `__asm` text
this lane must not touch, so it is reported and not made here.

### Why SoftBlitAnim itself was NOT ported

`softblit.c:1028` `SoftBlitAnim` (0x00465240, the recolouring type-2 painter)
is still `LL_UNPORTED_ASM()`. It is the same family but not a copy of the
plain one, and porting it means first deciding what to do about FINDING 2.
Beyond the label, the real differences a port has to carry are:

| | SoftBlitAnimPlain | SoftBlitAnim |
| --- | --- | --- |
| reader | `dec eax / jns` + `mov eax,0Fh` | `and g_zb_bits,0fh / jne` + `mov 10h` (equivalent) |
| length splice | `movzx ecx,bl` | `shrd ecx,ebx,8 / shr ecx,18h` (equivalent) |
| left-clip subtraction | `sub edx,ecx / jns` | `sub edx,ecx / ja` — **differs at edx == ecx** |
| the run that just fits | `js endrow` | `jbe endrow` |
| a zero budget at a fill | writes one stray pixel | `or ecx,ecx / je endrow`, writes none |
| repeat run | doubled dword + `rep stosd` | software loop with the mask |
| every store | plain | `and ax, word ptr [g_sp_recolour]` |
| run hit test | `sbb ecx,-1` -> `dst >= mouse` | `seta` -> `dst > mouse` |
| transparent single | the dead `je` of FINDING 1 | correct |

Nothing else in the front-end path needs it: it is reached only from
`bigrender.c:1062` `SoftPrint_XBltFast` for ImageRec type 2, the recolouring
highlight of a placed object.

## 4. The VC6 gate

Run after the integrator's quiet window (2026-09-11 06:00), per touched file.
Only `LEGOLAND/rlepaint.c` and `LEGOLAND/rlepaint2.c` were touched, and in
both the only lines REMOVED are the ten `LL_UNPORTED_ASM();` traps — the
`#ifndef LEGOLAND_PORTABLE` arms and every `// FUNCTION:` marker are
byte-identical to `3ca919cc`.

```
$PY tools/audit.py LEGOLAND/rlepaint.c
  [OK  ] 0x00466d80 RLEPaintHitClipLR  ours=325i/1011B  orig=325i/1011B  mismatch=0
  [OK  ] 0x00467180 RLEPaintHitClipL   ours=202i/611B   orig=202i/611B   mismatch=0
  [OK  ] 0x004673f0 RLEPaintHitClipR   ours=197i/588B   orig=197i/588B   mismatch=0
  [OK  ] 0x00467640 RLEPaintHit        ours=122i/358B   orig=122i/358B   mismatch=0
  [OK  ] 0x004677b0 RLEPaintClipLR     ours=271i/833B   orig=271i/833B   mismatch=0
  [OK  ] 0x00467b00 RLEPaintClipL      ours=180i/526B   orig=180i/526B   mismatch=0
  [OK  ] 0x00467d10 RLEPaintClipR      ours=167i/489B   orig=167i/489B   mismatch=0
  [OK  ] 0x00467f00 RLEPaintFast       ours=114i/311B   orig=114i/311B   mismatch=0
  PASS: 0 function(s) failed the extent gate

$PY tools/audit.py LEGOLAND/rlepaint2.c
  [OK  ] 0x00468040 SoftBlitRLEFrameRecolour  ours=303i/965B  orig=303i/965B  mismatch=0
  [OK  ] 0x00468410 SoftBlitRLEFrame          ours=312i/992B  orig=312i/992B  mismatch=0
  PASS: 0 function(s) failed the extent gate

$PY tools/audit.py LEGOLAND/softblit2.c
  [OK  ] 0x00464480 SoftBlitAnimPlain  ours=455i/1539B  orig=455i/1539B  mismatch=0
  [OK  ] 0x00466770 SoftBlitRLEPlain   ours=603i/1543B  orig=603i/1543B  mismatch=0
  PASS: 0 function(s) failed the extent gate

$PY tools/relocs.py LEGOLAND/rlepaint.c   -> 20 relocations, 0 mismatches
$PY tools/relocs.py LEGOLAND/rlepaint2.c  -> 20 relocations, 0 mismatches
$PY tools/relocs.py LEGOLAND/softblit2.c  -> 166 relocations, 0 mismatches
  (`| grep MISMATCH` empty for all three)

$PY tools/progress.py --check
  665/675 exports exact (98.5%); 3281 exact functions total; 42 WIP
```

`--check` reported `docs/LEGOLANDPROGRESS.HTML` stale, as expected: the C
bodies moved eight rows' line numbers. Regenerated and committed — 3323 rows
before and after, eight rows differing in nothing but their `#L` anchor, all
in rlepaint.c/rlepaint2.c, and the totals unchanged.

Builds, both from CLEAN directories:

* native `cmake -S portable -B portable/build -G Ninja` + `ninja` + `ctest`:
  4/4;
* wasm `emcmake cmake ... -DLL_ILP32=ON` + `ninja` + the four extra targets +
  `ctest`: **9/9**, `rle_paint` included, real sprite included.

## 5. Evidence: how far the game runs

### The census

`python3 portable/tools/linkreport.py portable/build`: **asm stubs 26 -> 15**.
The fifteen left are `bigrender.c:939 ZBufferHelper`, `blitmisc.c` x2,
`bnvpath.c:95 sub_458930`, `coaster13.c:737 TrackShade_FillPoly`,
`coastermath.c` x2 (`FastSqrt`/`FastRSqrt`), `coastershade2.c` x2,
`popup.c:432 RenderTransSprite`, `softblit.c:1028 SoftBlitAnim` and
`tri3d.c`'s four triangle rasterisers. Everything else in the report is unchanged (game-fn 0, host 0,
alias 0, duplicates 0, prototype conflicts 0).

`softblit2.c:677 SoftBlitAnimPlain` is gone from that list -- it was ported
too, see §3b -- so the count is now **26 -> 15**. `softblit.c:1028
SoftBlitAnim`, its recolouring sibling, is deliberately still there; §3b says
why and tabulates every difference a port of it has to carry.


### The headless spine (node)

```
LL_HOST_TRACE=1 LL_CD_DIR=$PWD/gamedata/disc LL_DATA_DIR=$PWD/gamedata/main \
  perl -e 'alarm 120; exec @ARGV' node portable/build-wasm/legoland_headless.js
```

Before this lane it ended at `TRAP rlepaint.c ... RLEPaintHit` from
`ShowTitleScreen -> PrintSprite -> RenderSprite -> SoftBlitSprite ->
SoftBlitRLEPlain`. With the painters in C it runs through and reaches:

```
HOST CreateSurface 640x480 caps 0x200 PRIMARY
HOST DirectInput CreateDevice(GUID_SysKeyboard)
HOST DirectInput CreateDevice(GUID_SysMouse)
HOST first present: 640x480 pitch 1280
NODE present16 #1 640x480 pitch 1280: 301157/307200 non-black,
     first row: 8e1b 7e1c 7e1c 761c 761c 761c 761c 75dc 6ddc 65dc ...
```

**301157 of 307200 pixels non-black** on the first present — 98% of the
screen, which is the whole title picture. The first row is an RGB565 sky
gradient (`0x8e1b` = R17 G48 B27, `0x65dc` = R12 G46 B28: light blue getting
slightly deeper across the row). No trap anywhere in the run.

### The browser page

`cd portable/build-wasm && python3 -m http.server 8794`, then
`http://localhost:8794/legoland.html?trace=1`. **The canvas shows the real
LEGOLAND title screen**, not a test pattern: a photographic LEGO-brick park
scene — a blue sky with clouds, the yellow LEGOLAND entrance gate on the right
with its red LEGO logo tile and "LEGOLAND" wordmark, a blue-and-white striped
carousel and a hot-air balloon ride behind it, a white castle and a scaffold
tower on the left, green lawn along the bottom, a row of minifigures in the
foreground (a white-haired scientist in a lab coat, a chef, a girl with black
hair, a workman in green dungarees), a fir tree at the right edge and a small
brick-built aeroplane in the sky. Top left is the game's own analogue pocket
watch sprite in its brass case. The host banner reads `display 640x480`,
`frames 356`, `fps 4.7`, `modal —`.

Nothing is torn, offset by a row, colour-swapped or streaked, which is the
real check on the port: a wrong stride, a mis-stepped B cursor or a dropped
opcode in any of the ten leaves would show as garbage within a row or two.

### Where it stops, exactly

The title screen is up and the trace then repeats `HOST Sleep(100)` forever
with no further host calls; a click on the canvas produces none either. That
is **not** an RLE problem — it is `RunGame` (gamemain.c:370) immediately after
`ShowTitleScreen`:

```c
    ShowTitleScreen();
    ResumeMusicThread();
    SetWaitSpriteRect(0, 0);
    while (g_music_disabled == 0) {     /* 0x007988bc */
        PeekMessageA(&msg, 0, 0, 0, 0);
        Sleep(100);
        progress_tick();
    }
```

`g_music_disabled` is set by `InitMusicSystem` (sysstubs.c:402) when
`g_music_sys` is zero — and `-nomusic` sets `g_music_sys = 0`
(startup.c:254), so under this harness the loop *should* fall straight
through. It never gets the chance:

```c
/* lifecycle.c:141, 0x004964f0 */
int InitSoundSystem(void) {
    g_snd_hwnd = WNDENV_Gethwnd();
    ok = InitSoundSampleSystem(g_snd_hwnd);
    if (!ok) return ok;                    /* <-- taken */
    return InitMusicSystem(g_snd_hwnd) != 0;
}
```

`portable/src/hostwin/dsound.c`'s `DirectSoundCreate` returns
`DSERR_NODRIVER`, so `InitSoundSampleSystem` fails, `InitSoundSystem` returns
early, and `InitMusicSystem` — the only thing that would set
`g_music_disabled` on a silent run — is never called. The shim's own header
reasons carefully about why the failure is safe for the *sample* paths (every
one is guarded by `g_samples_ready`), and it is; what it misses is that the
same early return strands `RunGame`'s music wait loop.

**The next blocker is therefore: `DirectSoundCreate` must not fail, or
`InitMusicSystem` must be reached some other way.** The smallest fix is a
no-op `IDirectSound` object from `DirectSoundCreate` so
`InitSoundSampleSystem` succeeds; `InitMusicSystem` then sees `g_music_sys ==
0` under `-nomusic`, sets `g_music_disabled = 1`, and `RunGame` proceeds to
`LoadIconBarGFX` and the rest of the front end. `portable/src/hostwin/dsound.c`
is PORT-B's file, so this lane did not touch it.
