# Scope LL21 — partials wave D: byte-exact allocator and scheduler residuals

Branch `scope/LL21` from `origin/main` `62d5ef7f`. **PARTIAL scope**: edit only
the WIP bodies below and the notes above their markers (see
`docs/PARALLEL_CONTRACT.md`, "Extra rules for PARTIAL scopes"). Object
prefix `/tmp/sll21_`. Findings go in `docs/lanes/scope-ll21.md`.

## Why these are being reopened

**Every body below is already instruction-exact AND byte-exact against the
original.** The whole residual is *which register* holds a value, or *which
order* two independent instructions come out in. Several notes call their
state a floor, and under the levers those lanes had, they were right.

Scope Codex-F (closed 2026-09-09, `docs/lanes/codex-f.md`, entry in
`docs/DECOMP.md`) closed exactly this class twice — a 168-instruction body at
15 audit mismatches and an 81-instruction one at 8 — with two levers no
earlier lane had. Both are zero-cost. Read the Codex-F DECOMP entry before
you touch anything.

1. **A scope-V cancelled pair is a steering wheel for allocator priority, and
   its ANCHOR is the control input.** `t.ox = <expr>; t.ox += t.oy;
   t.ox -= t.oy;` on struct members survives to the allocator and cancels at
   instruction selection. The carrier gets its own web; **the anchor receives
   a priority bump**. So the anchor must be a value whose own ranking does not
   matter — a link-time address constant (`t.oy = (int)g_some_table;`) or an
   already-materialised induction value (`t.oy = j * 4;` where ecx already
   holds j*4). Anchoring on the value you are trying to place ranks it ABOVE
   its neighbour and moves the problem. Constraints: both operands must be
   members of the SAME struct (a constant or a plain scalar used directly in
   the cancel folds in the front end); the anchor is live wherever the cancel
   is; and a member that must live ACROSS a loop gets a real frame slot, so
   keep the carrier inside one basic block.
2. **An alias pointer defeats VC6's canonicalisation of a commutative `fmul`
   or `imul` whose two operands are constant offsets off ONE pointer.** That
   canonicalisation silently reverses the emitted operand order. Two different
   pointers cannot be ordered, so `kg = kf;` and reading the affected operand
   through `kg` restores source order. **Source operand order is inert** — 64
   permutations measured twice — so change the SPELLING, never the position.

**Use `/FAcs` from the first hour, not the last.**
`ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /O2 /Gy /Gd
/FAcs /Fa<out>.asm /Fo/tmp/sll21_x.obj LEGOLAND/<file>.c` prints VC6's own
frame symbol table and attributes every instruction to its source line. A
local that cost a frame slot appears as a `_name$ = -N` equate. It also shows
that VC6 overlaps locals onto dead PARAMETER slots, so a pointer pun and a
plain local are equivalent.

## Functions (5) — 1,079 instructions, 66 audit mismatches

| address | name | file | state | residual (from the marker) |
| --- | --- | --- | --- | --- |
| 0x00477bd0 | RequestRoute | simcore.c | 482i/1336B exact, 3 mism | eax/ecx web ranking; note says the pairing is "unreachable by construction" |
| 0x0045f810 | ValidateCursor | objmap2.c | 205i/617B exact, 5 mism | idx 42-46 are one permutation of one multiset; offset-blind == strict |
| 0x0041a040 | BoatingSchool_Add | ridecb5.c | 218i/683B exact, 8 mism | argument load/push pair at idx 39..50 placed where no statement order reaches |
| 0x00417e70 | WW_AnyBlokeInRect | waterworks.c | 31i/68B exact, 12 mism | **register-blind distance 0** — registers are the entire difference |
| 0x004284d0 | Coaster3D_BuildPieceGeometry | coaster3d.c | 143i/528B exact, 38 mism | two float radii spilled/reloaded; declaration-order floor |

Start with `WW_AnyBlokeInRect`: 31 instructions, register-blind 0, so it is
the cleanest possible test of lever 1 and it will tell you in an hour whether
the anchor steers this allocator the way it steered Codex-F's.

## Rules

- Read the note above each marker FIRST and do not repeat what it lists.
- Reconstruction-error pass before any variant search.
- After EVERY change `audit.py` the whole file: it must still end PASS and
  the count of `[OK]` lines must not drop; if it does, revert.
- `relocs.py` zero `MISMATCH`, `/W3` clean, before each commit.
- Marker stays `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <residual>)` until
  `audit.py` prints `[OK]`; then exactly `// FUNCTION: LEGOLAND 0x<VA>`.
- Retire honestly: if the lever does not move a body, add one paragraph to
  its note saying what you measured and move on. A well-measured negative is
  a deliverable.
- Do not run `verify.py` / `progress.py` / `coverage.py`; do not edit
  `tools/`, `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`.
- Commit to `scope/LL21` only; no push; no merge; **no Co-Authored-By
  trailer of any kind.**
