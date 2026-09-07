# Scope AB — SoftBlitRLEPlain specialised painters

Branch `scope/AB`. New file `LEGOLAND/rlepaint.c`. Object prefix `/tmp/sab_`.
Cut from inventory groups 18–19. Out of scope: SoftBlitRLE `0x00468040`,
SoftPrint painter `0x00468410`. Did not edit `softblit2.c`.

## Per-function status

| address | name | insns | pct | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00466d80 | `RLEPaintHitClipLR` | 325 | 100 | [OK] | `FUNCTION` |
| 0x00467180 | `RLEPaintHitClipL` | 202 | 100 | [OK] | `FUNCTION` |
| 0x004673f0 | `RLEPaintHitClipR` | 197 | 100 | [OK] | `FUNCTION` |
| 0x00467640 | `RLEPaintHit` | 122 | 100 | [OK] | `FUNCTION` |
| 0x004677b0 | `RLEPaintClipLR` | 271 | 100 | [OK] | `FUNCTION` |
| 0x00467b00 | `RLEPaintClipL` | 180 | 100 | [OK] | `FUNCTION` |
| 0x00467d10 | `RLEPaintClipR` | 167 | 100 | [OK] | `FUNCTION` |
| 0x00467f00 | `RLEPaintFast` | 114 | 100 | [OK] | `FUNCTION` |

Gate: `audit.py` PASS (8/8 `[OK]`), `relocs.py` zero MISMATCH (20 matched
relocations, all `g_blit_hit`), `/W3` clean. Names kept from softblit2.c.

## Mechanics

Type-3 COMP leaves (presentation-data supersedes softblit2.c's "A = control
words" naming):

- **A** = u16 pixel words (already palette-expanded colours; zero is opaque).
- **B** = u8 run lengths.
- **C** = packed 2-bit controls, LSB-first, 16 codes per dword.

Mask starts at `ebx = 3`, `rol ebx, 2` after each code; `and ebx, 1` after the
rotate is the wrap flag and `lea edx, [edx+ebx*4]` advances C. Hi/lo tests are
`test …, 0xAAAAAAAA` / `0x55555555`.

| Primary | Meaning |
| --- | --- |
| 0 or 1 | emit one u16 from A |
| 2 | skip one pixel |
| 3, B=0 | end of row (no secondary) |
| 3, B=L>0, secondary 0 | literal run of L words from A |
| 3, L>0, secondary 1 | repeat one A word L times |
| 3, L>0, secondary 2/3 | transparent run of L |

Phases: skip `source_top` rows (consume A/B/C only); then for each of `height`
visible rows skip `left`, paint `w` (or the full row for Fast/Hit), consume the
right tail through end-of-row, advance dst by pitch. Hit leaves OR 1 into
`g_blit_hit` (`0x007feb14`) when a singleton store hits the mouse address, or
when `((mouse - dst) >> 1)` (arithmetic shift, unsigned compare) is below the
**drawn** opaque run length. Clipped-away / transparent pixels do not hit.

Original defect retained: HitL, HitR, and Hit (and the out-of-scope
recolor/highlight) mishandle primary code 1 in the top-skip loop — after the
`0xAAAAAAAA` test they advance A then fall through into the escape/`0x55555555`
path instead of jumping back. HitLR and all four no-hit leaves jump correctly.
Shipped 16-bpp assets contain no primary code 1 (dormant).

## Levers / reconstruction notes

- **Hand-written assembly, not C.** The rotating-mask idiom, mid-stream
  callee-saved pushes, and `rep movsw`/`rep stosw` clip splits do not lower
  from any C spelling. Precedent: `tri3d.c` / `coastermath.c`
  `__declspec(naked)` + `__asm` bodies. `NAKED` macro keeps the attribute off
  the signature line so the `// FUNCTION:` marker stays immediately above the
  name (audit/verify marker parse).
- **Family transfer.** Fast is the unclipped core; ClipR adds a width budget
  and right-edge run splits (`sub/jl` + partial `rep`); ClipL adds left skip
  then paints the rest; ClipLR combines both. Hit twins insert the
  `g_blit_hit` OR before each opaque emit. Diffing adjacent leaves is how the
  clip/hit deltas were checked, not re-derived.
- **Extern:** only `g_blit_hit` (`/* 0x007feb14 */`). No callees. Prototypes
  match softblit2.c (caller-side); Fast omits left/w/spare/mouse.
- **softblit2.c header A/B/C names are wrong** for these leaves; behaviour
  follows presentation-data / FORMATS / the leaf disassembly. Did not edit the
  caller's comments (owned elsewhere).

## Out of scope (left alone)

`SoftBlitRLE` `0x00468040` (recolor) and SoftPrint painter `0x00468410`
(highlight) — same grammar, different store (`pixel & mask` /
`(pixel & mask) >> 1`) and different hit rules.
