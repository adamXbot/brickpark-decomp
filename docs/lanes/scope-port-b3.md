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

(filled in as each one lands; see §3 for the gate and §4 for the evidence)
