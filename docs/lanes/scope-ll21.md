# Scope LL21 — partials wave D: byte-exact allocator and scheduler residuals (2026-09-10)

Branch `scope/LL21` from `origin/main` `62d5ef7f`. PARTIAL scope over five
bodies that were all already instruction- and byte-exact, reopened only
because scope Codex-F closed this class twice with two levers no earlier lane
had (brief: `docs/SCOPE_LL21_partials_allocation_b.md`). Object prefix
`/tmp/sll21_`.

Gates at the tip, on every file touched: `audit.py` **PASS**, `[OK]` counts
unchanged or up, `relocs.py` zero `MISMATCH`, `/W3` silent.

## Status

| address | name | file | before | after | marker |
| --- | --- | --- | --- | --- | --- |
| 0x00417e70 | WW_AnyBlokeInRect | waterworks.c | 31i/68B, 12 strict | **CLOSED, exact** (0) | `// FUNCTION:` |
| 0x004284d0 | Coaster3D_BuildPieceGeometry | coaster3d.c | 143i/528B, 38 | **35**, still 143i/528B | WIP |
| 0x00477bd0 | RequestRoute | simcore.c | 482i/1336B, 3 | unchanged (3) — floor re-endorsed, w11a rule 1 falsified | WIP |
| 0x0045f810 | ValidateCursor | objmap2.c | 205i/617B, 5 | unchanged (5) — floor re-endorsed | WIP |
| 0x0041a040 | BoatingSchool_Add | ridecb5.c | 218i/683B, 8 | unchanged (8) — new premise recorded | WIP |

`waterworks.c` goes from 64 `[OK]` to **65 of 65** — the file is now fully
exact.

## Method

Three small harnesses, kept out of the tree (rebuildable in minutes):

- `harness.py <base_copy> <target> <first> <last> <vdir> <func> <va> <tag>` —
  splices each `<vdir>/*.c` over lines `first..last` of the target, runs
  `matchfull.py` and reports the `X` line count and `ok/total`; restores the
  base afterwards. `harness2.py` is the same with TWO independently spliced
  line ranges (an inlined helper plus its caller), variant files sectioned by
  `@HELPER` / `@BODY`.
- `harness_audit.py` — same splicing, scored by `audit.py` instead. **Use this
  one for anything where the instruction count can move**: `matchfull`'s
  aligned `X` count and `audit`'s index-for-index `mismatch` disagree badly
  once a body is one instruction long or short (a body at 6 `X` was 443
  `mismatch`), and `audit` is the gate.
- `diff1.py` / `listing.py` — apply one variant and print its aligned diff and
  its `/FAcs` listing region. The `/FAcs` listing answered three questions
  this lane that guessing would not have: whether a cancelled pair actually
  folded (no add/sub in the stream), whether a carrier cost a frame slot (a
  `_name$ = -N` equate), and which source line each instruction came from.

The machine's worktree guard refuses inline `for … do … done`, so every sweep
is a generator script that writes variant files plus one harness invocation.

## The lever that closed a body

**A cancelled-pair ANCHOR is a steering wheel, and the direction matters more
than the presence.** Codex-F's warning was "do not anchor on the value you are
trying to place, it ranks it ABOVE its neighbour". `WW_AnyBlokeInRect` is the
case where that is exactly what you want.

- 31 instructions, 68 bytes, register-blind distance 0, and four earlier
  passes (~170 variants) had proved that nothing lengthens the tile temp's
  live range — `volatile` is inert at zero cost, so the note correctly
  concluded the EAX race is a global web rank and not a scratch rotation.
- What was missing is that a scope-V cancelled pair hands its **anchor** an
  allocator priority bump, and here the value that needed one is the list
  cursor. `v.anchor = (int)b;` then `v.t = <tile>; v.t += v.anchor;
  v.t -= v.anchor;` on two members of one flat struct ranks `b` above the tile
  temp, so `b` wins EAX and the tile takes ECX. Zero cost: the pair cancels at
  instruction selection and the struct's address is never taken, so VC6
  flattens it and no frame slot appears.
- **BOTH axes must carry.** The x tile alone (anchor before or after the load)
  and the y tile alone are each still 12 X — the untreated axis's temp keeps
  its old rank and drags EAX back. This is the load-bearing measurement.
- Inert once both axes carry: member count (two members with one reused
  carrier == three with one per axis), member names, add-then-sub vs
  sub-then-add, assigning the anchor once per iteration vs once per cancel,
  hoisting the anchor's first assignment above the loop.
- A non-cursor anchor is not merely inert: `(int)&g_people_head` is 12 X
  (bumps nothing, as Codex-F predicts of an address constant) and `(int)r` is
  15 X (bumps the rect pointer instead). Anchoring the cursor as the CARRIER
  is also 12 X.

**Coaster3D_BuildPieceGeometry, 38 -> 35, byte count unchanged.** Same lever
on the curve loop's two heading indices: `t.ib += t.ia; t.ib -= t.ia;` with
the ENTRY index as the anchor puts the EXIT index in ecx, which is where the
original loads it. The anchor must be the entry index — every address constant
plus `i`, `n` and the exit index itself are inert at 38, and the mirror
(carrier = entry, anchor = exit) is 41. Carrying the POINTER instead of the
index is 74, and sub-then-add does not fold at all (79).

## Negatives worth the space

### The alias-pointer lever does not generalise past a single-base commutative op

Codex-F's second lever is "an alias defeats VC6's canonicalisation of a
commutative `fmul`/`imul` whose two operands are constant offsets off ONE
pointer". `ValidateCursor`'s four footprint sums add `r->F` to `o->Y` —
operands off **two** pointers already — and every alias spelling is
byte-identical to the committed body at zero cost: a second `Rect* r2 = r;`
used for any single rect field, a `Pos* o2 = o;` for the second origin read, a
`Map* m2 = g_map;` for the second height read (at the store or hoisted), and a
`*(unsigned short*)((char*)g_map + 22)` cast. So the earlier pass's finding
that all 16 addend flips are byte-identical is a *different* phenomenon from
the fmul reversal, and reaching for the alias lever on an integer `add` is
wasted effort. (It is not free where it does bite: aliasing both x-axis reads
is 18 mismatches, both y-axis reads 14.)

### A cancelled pair cannot be written on an address-taken struct, and its carrier can CSE loads back together

`ValidateCursor`'s `bound` is address-taken (it reaches `IntersectRect`), so
its members are real memory and a pair written on them would emit real
arithmetic. Carrying the second `g_map->height` read through a *separate* flat
struct instead **merges the two height loads** — the body loses four
instructions (201i) and 46..66 mismatches follow. All six anchors, on the
first read, the second and both, at function and inner block scope: best 46.
The `/FAcs` listing shows the pair does cancel and costs no frame slot; what
breaks it is that the carrier's web displaces `r` from edi to ebx and the
`_r$` spill goes with it.

### Pinning a load early does not let its store move late

`ValidateCursor`'s open hypothesis was "the second height read is not a
single-use temporary in the original". Closed negatively: a volatile-qualified
POINTER read (`*(volatile unsigned short*)&g_map->height`, which needs no
frame slot, unlike the `volatile int h` the earlier pass tried) crossed with
`h` as a sixth helper argument, `py = o->y` named or not, and the
`bound->right = h` store at each of five positions among the sums — 40 bodies,
best 16, and only the baseline shape (store before the sums) stays at 5. The
load and the store travel together whatever pins the load.

### The cancelled pair reaches a regime a floor note declared unreachable — and still cannot force a copy

`RequestRoute`'s pass-w11a note proves, carefully, that the else block's
`to.x` can only be the goal test's own web or a fresh load, and that "a
register-to-register copy is neither; there is no third regime for VC6 to land
in". That is wrong: a cancelled pair IS the third regime. Written on the plain
field spelling that pass measured at 138 (207 here), it produces a body with
indices 0..37 **and** 39..480 exact — the whole cascade disappears, including
the `mov edx,[4bb5a4h]` at 31 and the `cmp` at 32 that are two of this body's
three mismatches. Any pointer or address-constant anchor reaches it; 61
anchor x placement combinations measured, and it is robust to chained pairs,
unsigned/`char*` members, a redundant `*1` or `|0`, and feeding the carrier to
the call.

It still does not close, and the reason is now precise: VC6 **coalesces** the
carrier's web with the goal test's, because that web's source dies at the
carrier's definition. `to.x` is then stored straight out of eax, the body is
481 instructions, and the original's `mov ecx,eax` at index 38 is the one
missing instruction. Correct rule: *a cancelled pair gives a value its own
web, but a web whose source dies at its definition is coalesced away; the copy
still needs interference or a phi.* Making y the anchor (to bump it into eax)
fails differently — the anchor is then a real load and lands exactly where the
copy should be.

Two mechanical facts fell out and are reusable anywhere:

- **The fold is order-sensitive.** `+=` then `-=` folds; `-=` then `+=` does
  NOT when the anchor is a link-time address constant — it emits
  `sub eax,K` / `lea ecx,[eax+K]`, i.e. the copy plus a displacement. (That
  `lea` is the closest anything got to the wanted copy.)
- **Nothing may sit between the cancel and the carrier's use.** One statement
  in between un-folds the pair (real `add`/`sub` appear) or collapses the body
  back to the un-levered regime. Measured on `RequestRoute` (207..212 for a
  y-store, a struct copy or a y temp in between) and again on
  `Coaster3D_BuildPieceGeometry`.

### BoatingSchool_Add: the natural store order is reachable after all

The committed body spells the list link BEFORE the last queue slot, which the
note calls "surely not original", because the natural order
`q[4] = 0; next = head; head = st;` was measured at 146/147 mismatches by
three earlier passes. With the head carried through a cancelled pair anchored
on `o`, the natural order collapses from 97 aligned mismatches to **six**, at
the right 218 instructions, with the whole water-laying tail still exact. So
the artificial ordering is a workaround for a register rank, not evidence
about the source — worth knowing for the sibling `*_Add` handlers.

It is still not better than the committed 8: in that shape `o` lands in ecx
and the head in eax where the original has `o` in edx and the head in ecx —
both exactly one step along VC6's eax->ecx->edx scratch rotation. Every
documented rotation-advance was tried on top and none moves it (free volatile
reads at four sites 7/9/8/7, a volatile read of `o` 41, a second cancelled
pair 64, an alias for `o` 6, a named head local 6, unsigned and pointer-typed
members 6, the cancel written on the anchor member 97). And the head cannot
take ecx in that order for a mechanical reason: eax holds the function-wide
zero, its last use is the `q[4]` store, so eax is free by the time the head
loads. **Only a zero store AFTER the head load keeps eax busy** — which is
precisely what the committed spelling buys.

On the committed order the lever is inert or harmful: pairs on `o` (nine
anchors, at the call and between `q[0]` and `q[1]`) and on the head (eight
anchors) give 5 aligned X when the anchor is a literal — it folds in the front
end, so the body is unchanged, exactly as DECOMP predicts — and 7..117
otherwise.

### Coaster3D: two halves of the residual are mutually exclusive

The original evaluates BOTH heading indices before either `lea x3` (so both
multiplies are in place) **and** reloads both radii from their frame slots to
push them. Naming the two indices in their own `int` locals and forming the
pointers in the ARGUMENT buys the first half exactly — 34 mismatches — but the
declaration order that does it un-spills `r1` (`mov ecx,edx` where the
original has `mov ecx,[esp+14h]`) and the body drops to 526 bytes. Across all
48 orderings of {r0, r1, index temps} x {pointers named, pointers in the
argument}: every spelling that reloads both radii keeps the out-of-place
multiply, and every spelling with in-place multiplies keeps `r1` live. Forcing
the spill back costs more than it saves in every form (`volatile float r1` 74,
both volatile 79, `float r1[1]` and a one-member struct home 34/39, split
declaration 34, `FPair` copy 83, a second named copy 34, re-reading the radii
at the second call site 82). The committed body therefore keeps 528/528 bytes
at 35 rather than 526 at 34.

The root cause is countable: the original has SEVEN values live across the
push block (eax=3a, ecx=3b, edx=n, ebx=&g_corner[i], ebp=b, esi=n, edi=i), so
it has no register left for either radius. This build is one live value short
of that. **That deficit, not the declaration order, is what a future lane
should attack.**

## Levers, in DECOMP's bullet style

- **Anchor a cancelled pair on the value you WANT ranked higher.** Codex-F's
  rule ("the anchor must be a value whose own ranking does not matter") is the
  rule for when the anchor is incidental. When the residual is "web A should
  outrank web B", make B the carrier and **A the anchor**: that is the whole
  fix for `WW_AnyBlokeInRect` (list cursor over tile temp) and for
  `Coaster3D_BuildPieceGeometry`'s heading indices. Evidence: 12 -> 0 and
  38 -> 35, both at unchanged instruction and byte counts.
- **A cancelled pair must be applied to EVERY member of the contending group,
  not just one.** One of `WW_AnyBlokeInRect`'s two tile temps left untreated
  keeps its old rank and pulls the register back; treating both closes the
  body. Corollary for a diagnostic: a pair that "does nothing" may simply be
  outvoted.
- **Nothing may sit between the cancel and the carrier's use.** One
  intervening statement makes VC6 emit the `add`/`sub` for real, or reverts
  the allocation. Measured twice, on two files.
- **`+=` then `-=` folds; `-=` then `+=` does not, when the anchor is a
  link-time address constant** (`sub eax,K` / `lea ecx,[eax+K]` survive).
- **A cancelled-pair carrier whose source web dies at the carrier's definition
  is COALESCED, so the pair cannot manufacture a `mov rA,rB`.** It manufactures
  a *separate web*, which is a different thing. `RequestRoute` is the witness:
  the whole body becomes exact except the copy itself.
- **A cancelled pair cannot be written on the members of an ADDRESS-TAKEN
  struct** (they are real memory); a separate carrier struct can silently
  merge two loads the original keeps apart. `ValidateCursor`, 205i -> 201i.
- **The alias-pointer lever only applies to a commutative operator whose two
  operands are constant offsets off ONE pointer.** Two different bases means
  there is nothing to canonicalise, and every alias spelling is byte-identical.
- **Score with `audit.py`, not `matchfull.py`, whenever the instruction count
  can move.** A `matchfull` 6 was an `audit` 443 in this lane.
