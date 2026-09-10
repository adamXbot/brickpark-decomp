# Scope LL24 — `Draw3DPersonModel` (0x00440a30), person3d.c

Branch: `scope/LL24`. Worktree: `.worktrees/scope-ll24`. Object prefix
`/tmp/sll24_`. PARTIAL scope, ONE function; `person3d.c`'s three exact bodies
were `[OK]` before and after every probe in this lane.

## Status

| Address | Function | Ours | Orig | Audit | Marker |
| --- | --- | --- | --- | --- | --- |
| `0x00440a30` | `Draw3DPersonModel` | 1023i / 3511B | 1023i / 3523B | mismatch **377** (63.3%) | `WIP-FUNCTION` |

`audit.py` PASS, three `[OK]` unchanged; `relocs.py` zero MISMATCH; `/W3`
clean. **No source change was committed: every variant measured this round
scored 377 or worse, so the body is byte-for-byte what it was.** The
deliverable is the recovered frame map and the mechanism below.

## 1. MASM reserved-word check — CLEAN

Every `__asm` operand identifier in this function was swept against MASM's
reserved names (`cr0`-`cr4`, `dr0`-`dr7`, `tr3`-`tr7`, `st`, the segment
names, and the 8/16/32-bit register names):

`t i v a b c fx fy fz zb sc box light vptr yy ydep zscale k65536 crs1 crs2
dx1 dy1 dx2 dy2 mpp nverts nrm fr g_xverts`

None collides. Confirmed mechanically on the `/FAcs` listing as well: of the
1023 emitted code lines **none** carries a control-, debug- or
segment-register operand and none encodes `0f 20/21/22/23`. The only `cr2`
tokens in the whole listing are inside the explanatory source comment. Note
that `cx` and `cz` **are** MASM register names — they are safe here only
because they are used exclusively from C; if a future edit ever puts `cx` into
an `__asm` operand it becomes the 16-bit register, the same class of bug as
the historical `cr2`.

## 2. The frame map (the deliverable)

Recovered by pinning VC6's own `/FAcs` frame symbol table against the
original's `[ebp-N]` at the same instruction index — the two bodies are
index-for-index aligned at all 1023 positions, so the map is exact, not
inferred. (`scratchpad/attrib.py`; the raw slot bijection is
`/tmp/sll24_bij.txt`.)

| object | bytes | C refs | ours | ORIGINAL |
| --- | ---: | ---: | ---: | ---: |
| `light[3]` | 12 | 5 | `-0x20` | `-0x0c` |
| `mt[9]` | 36 | 10 | `-0x78` | `-0x54` |
| `box[8]` (`Vec3i`) | 96 | 26 | `-0x154` | `-0xd8` |
| `sc[24]` | 96 | 3 | `-0x1b4` | `-0x150` |
| `v[3]` (`Vertex2D`, both loops) | 84 | ~70 | `-0xe8` | `-0x1a4` |
| `a` loop 1 / `b` loop 2 | 12 | | `-0x94` | `-0xf0` |
| `b` loop 1 / `a` loop 2 | 12 | | `-0x54` | `-0x78` |
| `c` (both loops) | 12 | | `-0x48` | `-0x6c` |
| `tp` loop 1 | 4 | | `-0x24` | `-0x10` |
| `tp` loop 2 | 4 | | `-0x80` | `-0x10` |
| `fr` | 4 | | `-0x80` | `-0x1b4` |
| `nfaces` | 4 | | `-0xec` | `-0x1b0` |
| `tris` | 4 | | `-0xf4` | `-0x1ac` |
| `ngour` | 4 | | `-0xf0` | `-0x1a8` |
| `nrm` | 4 | | `-0x7c` | `-0xe4` |
| `zscale` / `parity` | 4 | | `-0x84` | `-0xe0` |
| `mpp` | 4 | | `-0x88` | `-0xdc` |
| `cnt` | 4 | | `-0x24` | `-0x5c` |
| `face` | 4 | | `-0x38` | `-0x60` |
| `ydep` | 4 | | `-0x3c` | `-0x58` |
| `dx2` | 4 | | `-0xc` | `-0x58` |
| `lo` (sc scan) | 4 | | `-0x34` | `-0x24` |
| `lo` (z scan) | 4 | | `-0x34` | **`-0x28`** |
| `lo2` | 4 | | `-0xc` | `-0x28` |
| `zb` | 4 | | `-0xc` | `-0x24` |
| `hi` | 4 | | `-0x4` | `-0x18` |
| `oy` | 4 | | `-0x4` | `-0x18` |
| `ox` | 4 | | `-0x8` | `-0x30` |
| `q` | 4 | | `-0x8` | `-0x30` |
| `dx1` | 4 | | `-0x28` | `-0x24` |
| `fy` | 4 | | `-0x28` | `-0x10` |
| `dy2` | 4 | | `-0x2c` | `-0x28` |
| `fx` | 4 | | `-0x2c` | `-0x2c` |
| `crs1` | 4 | | `-0x30` | `-0x1c` |
| `fz` | 4 | | `-0x30` | `-0x14` |
| `vptr` | 4 | | `-0x14` | `-0x1c` |
| `nverts` | 4 | | `-0x14` | `-0x20` |
| `crs2` | 4 | | `-0x14` | `-0x14` |
| `dy1` / `yy` | 4 | | `-0x10` | `-0x20` |
| `k65536` | 4 | | `-0x10` | `-0x1c` |
| `t` / `i` (dead param home) | 4 | | `+8` | `+8` |

Frame size `0x1b4`, 80 referenced slots and 400 emitted slot references in
BOTH bodies. As a list, near→far:

```
ORIG: [light] 9sc [mt] 3sc c b [box] 3sc a [sc] [v] 4sc
OURS: 5sc [light] 7sc c b [mt] 4sc a [v] 3sc [box] [sc]
```

## 3. Findings

### 3a. NEW LEVER — no `__asm` reference of ANY kind ranks a frame object

The previous round proved `__asm { lea eax, X }` carries zero frame weight.
**Direct `__asm` MEMORY OPERANDS carry zero weight too.** Measured on the real
file: 24 `__asm { mov eax, sc[k*4] }` inserted after the sc scan and 24
`__asm { lea eax, sc[k*4] }` at the same site give **the same frame** as each
other and the same array order as the baseline, while 24 *C-level* reads of
`sc` at that site move `sc` up past `box`. So the rule generalises: only
C-level references rank a frame object; the inline assembler's operands are
invisible to the frame allocator whatever their addressing mode. That is why
`sc` — 27 emitted references, 24 of them the FMULA `lea` — behaves as a
3-reference object, and why `light` — 17 emitted references, 6 of them
SHADE's `imul dword ptr light[k]` — behaves as a 5-reference object.

### 3b. CORRECTION — frame weight IS movable on the real file

The previous round's rules (e)/(f) concluded that reference changes are inert
on the real file and that no source spelling can change a weight. That is
wrong as stated: it was measured by ADDING references to `box`/`sc`, which is
saturated. **Cutting `v`'s C-level references moves `v` monotonically down the
frame, and at the low end it reproduces the ORIGINAL'S ARRAY ORDER EXACTLY.**
Diagnostic probes (semantics-breaking, never committed;
`/tmp/sll24_refprobe*.py`), reading VC6's own `/FAcs` equates:

| `v`'s C refs | array order, near→far |
| ---: | --- |
| ~70 (baseline) | `light, mt, v, box, sc` |
| ~40 (UV + one parity arm + z stores cut) | `light, mt, box, v, sc` |
| ~12 (also dx/dy and the call arguments cut) | **`light, mt, box, sc, v`** ← the ORIGINAL |

So the frame residual is not an unexplained compiler tie-break. It has a
named, reproducible cause: **our `v` carries far more C-level reference weight
than the original's, and the original's frame order is what VC6 emits once
that weight is low enough.**

### 3c. …and it is still unreachable at 1023 instructions

No single construct carries the weight — cutting UV alone (12 refs), the
`!parity` arm alone (12), the `z` stores alone (6), the `dx/dy` block alone
(16) or the four call argument lists alone (12) each leaves the array order
UNCHANGED. Roughly 50 of `v`'s ~70 C references have to go, and **every one of
them emits an instruction that the original also emits** — the projection
block at 583..660 is plainly compiler-scheduled (interleaved allocation,
stores forwarded into the `sub` quartet at 653..656), not inline asm, so those
references cannot be moved into `__asm` where they would be weightless. The
only zero-instruction candidates are the sixteen `dx/dy` reads, which VC6
already forwards from registers; cutting those takes `v` from ~70 to ~54,
nowhere near the ~12 threshold. Symmetrically, `sc` would need >70 C
references to outrank `v` and it only has 24 elements. **The frame is at its
floor, for a reason that is now mechanical rather than merely observed.**

### 3d. NEW RECONSTRUCTION FINDING — the original has TWO `lo` variables

The ours→orig slot bijection is one-to-one for every frame object except one:
our `lo` (`-0x34`, 7 references) maps to **two** distinct original slots —
`-0x24` x4 (indices 334/342/345/366, the `sc` scan and `ox`) and `-0x28` x3
(indices 448/455/482, the `g_xverts` z scan). VC6 never splits one local
across two homes, so the original source declares a separate low-water
variable for the z scan. In the original, `-0x24` = `zb` + `dx1` + `lo`(sc)
and `-0x28` = `lo2` + `dy2` + `lo`(z).

MEASURED, and it is why it is not committed: splitting the z scan onto a new
`zlo` (with or without a matching `zhi`) leaves the instruction stream
identical at 1023i/3511B but permutes the frame **16 worse (393)**; spelling
the z scan with the existing `lo2`/`hi2` instead is byte-identical to the
baseline (VC6 just swaps which name owns which slot). Recorded as a source
fidelity finding, not as an improvement.

Two related pooling divergences, same class, all costed at >=377: the original
gives `cnt` a slot of its own (`-0x5c`) and pools BOTH loops' `tp` with `fy`
(`-0x10`), where we pool `cnt` with loop 1's `tp` (`-0x24`) and loop 2's `tp`
with `fr` (`-0x80`); and the original leaves `fr` alone at the very bottom of
the frame with its 2 references.

### 3e. Declaration order is inert for SCALARS too

Earlier rounds only permuted the four-array declaration block. Reversing the
**entire 34-line scalar declaration list** leaves every single `_name$` equate
at exactly the same offset and the body byte-identical. `/FAcs` lists the
equates in declaration order, which is the only thing that moves. The
"declaration order is completely inert for /O2 frame layout" rule is now
tested on both halves.

### 3f. An extra block nesting level is inert

Moving `Vertex2D v[3]` from the loop-body scope into a nested block opened
after the `a`/`b`/`c` corner copies is byte-identical. Consistent with the
recorded three-class model (function level / in-block / inline-expansion
temporary): a second block does not buy a second step.

## 4. Verdict

**Still at its floor, at 377 (63.3%), and the previous round's retirement
verdict stands — but the reason has been upgraded from "the weights are
provably equal so nothing can move" to "the weight difference is real,
measured, and reproduces the original's frame exactly, but paying for it costs
~50 instructions the original does not have."**

Would I send another lane here? Only if someone finds a spelling of the corner
projection that reaches `v` from `__asm` without changing the emitted stream —
that is now the single named question, and it is the only one left. Everything
else about this body is instruction-, register- and immediate-exact.
