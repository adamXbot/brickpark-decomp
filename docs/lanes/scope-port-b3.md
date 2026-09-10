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

## 4. Evidence: how far the game runs

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
