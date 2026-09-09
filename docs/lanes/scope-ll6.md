# Scope LL6 — raster / map / track-piece callbacks

Branch `scope/LL6`. File `LEGOLAND/coaster12.c`. Object prefix `/tmp/sll6_`.
Brief: `docs/SCOPE_LL6_raster_map_track.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00423200 | Raster_AddSpanRecord | 61 | ~61 | FLOOR | WIP (35 mism; esi/edx/keys IV) |
| 0x00423940 | TrackNode_InitStationJoints | 8 | 100 | [OK] | FUNCTION |
| 0x00423970 | TrackNode_SetStationHeights | 6 | 100 | [OK] | FUNCTION |
| 0x00423990 | Castle_GetCurveA | 2 | 100 | [OK] | FUNCTION |
| 0x00423f40 | GetTrackSegmentPiece | 86 | 100 | [OK] | FUNCTION |
| 0x00424050 | GetTrackSegment | 87 | ~83 | FLOOR | WIP (34 mism; fail2 / call order) |
| 0x00425da0 | Vec3f_Equal | 25 | 100 | [OK] | FUNCTION |
| 0x00426190 | Mat4_Transpose | 21 | 100 | [OK] | FUNCTION |
| 0x004263a0 | ProjectVertsToRect | 72 | 100 | [OK] | FUNCTION |
| 0x00426460 | Mat3_FromMat4Transpose | 21 | 100 | [OK] | FUNCTION |
| 0x004265d0 | ClipRect_ClipTo | 66 | 100 | [OK] | FUNCTION |
| 0x00426750 | Model_ProjectClipRect | 24 | 100 | [OK] | FUNCTION |
| 0x004275b0 | TrackNode_RemoveNop | 1 | 100 | [OK] | FUNCTION |
| 0x004275c0 | TrackNode_PlaceNop | 1 | 100 | [OK] | FUNCTION |
| 0x00427a40 | TrackNode_FillOppositeJoints | 24 | 100 | [OK] | FUNCTION |
| 0x00427a80 | TrackNode_SetBothHeights | 6 | 100 | [OK] | FUNCTION |
| 0x00427c30 | TrackNode_FillOppositeJointsH | 24 | 100 | [OK] | FUNCTION |
| 0x00427c70 | TrackNode_LoadDescHeights | 7 | 100 | [OK] | FUNCTION |
| 0x00427f70 | TrackNode_RemovePath | 39 | 100 | [OK] | FUNCTION |
| 0x00427ff0 | TrackNode_AddPath | 39 | 100 | [OK] | FUNCTION |
| 0x00428350 | Piece_InitStraight | 30 | 100 | [OK] | FUNCTION |
| 0x004283c0 | TrackPiece_FindIndex | 103 | 100 | [OK] | FUNCTION |
| 0x004286e0 | TrackNode_GetPieceDesc | 8 | 100 | [OK] | FUNCTION |
| 0x00428840 | LowestSetBitIndex | 10 | 100 | [OK] | FUNCTION |

**22 / 24 exact.** All 24 have bodies. `audit.py` PASS, `relocs.py` zero
MISMATCH (UNRESOLVED float literals 0.0 / 0.5 / -2.0),
`/W3` clean.

## Names

See the first commit for the exact stubs. New this wave:

- `GetTrackSegment` / `GetTrackSegmentPiece` — renderview.c already names
  the query; the helper is the per-piece midpoint + neighbor square.
- `TrackPiece_FindIndex` — 0x004283c0, used by `TrackNode_GetPieceDesc`.
- `Raster_AddSpanRecord` — already declared in coaster3d.c.
- `g_span_cursor` 0x004b5b3c, `g_span_count` 0x0060f908,
  `g_span_overflow` 0x0060f90c.

## Mechanics (WIP bodies)

- **GetTrackSegment**: if `g_castle.state == 2` walk `ring.jout.node` until
  the sentinel; else walk `tail_node` via jout then `head_node` via jin.
  Match is `sx == tile.x && sy == tile.y` (movsx vs int Pos).
- **GetTrackSegmentPiece**: `*link = (jin.node == &ring)`; evaluate
  hooks[+8] at `(t1+t0)*0.5`; `*h0 = (at.z + world.z) * -2`. Neighbor
  via jout (skip null and ring) does the same into `*h1` and writes
  `p1` from the neighbor square. No neighbor: copy `tile` into `p1`,
  `p1.y += -16` if jout is the ring, `*h1 = 0`.
- **TrackPiece_FindIndex**: `off = ((int)jout.h - (int)jin.h) >> 1 + 2`.
  If `jin.dir == opposite(jout.dir)`: straight, bases 8 / 13 / 18 / 23
  for slope 8 / 2 / 13 / 7. Else jump-table on slope → curve 0..7 or -1.
- **ProjectVertsToRect**: init AABB (left/top = MAXINT, right/bottom =
  INT_MIN), skip `n <= 0`, then per vert two rows of `dot3 + m[3]`,
  fistp, expand. 0.0 at 0x004ab390 is the sum starter; -2.0 at 0x004ab450
  is the height scale in GetTrackSegmentPiece.

## Levers / residuals

- `ClipRect_ClipTo`: exact once the first test is an inlined helper
  `ClipMissesRight(clip, dest)` — clip as arg0 of the helper loads
  `[esp+8]` into eax first. Direct `dest->left > clip->right` loads dest
  first (65/66).
- `GetTrackSegmentPiece`: exact once the no-neighbor square is `*p1 = *tile`
  (whole Pos copy). Field stores left y in ecx and forced a register
  `add -16`; the copy frees ecx for the `jout.node` reload so the original
  `add dword [p1+4], -16` appears. `at.z += world.z` is the `fst` of the
  sum onto at.z before `* -2`.
- `Mat4_Transpose` / `Mat3_FromMat4Transpose`: exact once the functions
  return the walked dest pointer. That parks dest in eax before the
  saved-register pushes. Returning the original dest (a saved copy) or
  a dest-first void store helper left the eax/ecx cursors swapped.
- `TrackPiece_FindIndex`: exact once adds live in the if/else-if chain
  and `return off` sits after the chain. That splits add from return so
  VC6 emits `add esi,ebx/imm` instead of `lea eax,[esi+K]`. Jump table
  on `slope-1` is right (12→0, 3→1, 4→2, 1→3, 6→4, 9→5, 14→6, 11→7,
  else -1). `>> 1` not `/2` (sar vs cdq). An inlined `AddOff` still
  folded into lea.
- `ProjectVertsToRect`: exact as the TransformVerts (0x00426250) shape —
  `while (n-- > 0)`, `acc += sp[j] * *rp++`, `acc += row[3]`, `TOINT`
  then `ASINT`. `int screen[4]` is the 8-byte ballast that makes
  `sub esp,0x14` (live x/y at [ebp-0x14]/[ebp-0x10]); `screen[2]` is
  `sub esp,0xc` and 4 mismatches. Overlaying `ASFLT(n)` as the
  accumulator collapsed the x87 walk (29%).
- `Raster_AddSpanRecord`: instruction count exact; keys/edge cursors and
  the overflow compare are allocation. Original `jbe` against
  `cursor+0x10` and `0x004e3870`; loop does `idx = keys->idx; keys++`
  then `*48`, y from `keys[-1]`, and `lea ecx,[edx+ecx*8+8]` after
  `xor ecx,ecx / test ne / jle` so a negative ne still advances by 8.
- `GetTrackSegment`: FLOOR 87/87, 232/232, 34 mismatch (~83%). Shared
  `did_match` after head keeps the tail-match call pending, so closed
  and tail are the original `jne loop` fall-through latches and
  head-empty `je`s back to fail1. Fail2 is unreachable: every second
  `return 0` (local zero, `nxt-n`, `TrackSegFail2` inline, `for(;;)`
  inner return, `fail1` moved to the end) ICF-merges into fail1 and
  the head latch stays `je fail1 / jmp loop`. Helper layout is
  ch-then-tail because LIFO pops the last-pushed head/closed target
  first; `if (!from_tail) goto match_ch` still specialises to that
  order. A single-predecessor `goto match_tail` inlines the tail call
  between tail and head and undoes both latches. Volatile `from_tail`
  would force tail-first via a runtime test the original does not have.
  **Fable ICF probe (reverted):** empty `__asm {}` on fail2 only — the
  one in-tree empty-asm use (LL4 Simpson) — is a frame lever, not an
  anti-merge: 64/89 = 71.9%. Main has no sibling that keeps two
  `pop edi / pop esi / xor eax,eax / ret` copies. `popup.c`'s
  `__asm { mov eax, 0 }` is handwritten depth stubs; `return (int)c`
  (DoorTileStep) drops the xor; a distinct string load would be
  `movzx` not xor. Intra-function fold of the identical 4-insn
  epilogue plus LIFO ch-then-tail is the combined floor.
  **2026-09-08 nest probe:** original fail1/fail2 are identical four
  bytes. Wrapping the whole head walk in `if (1)` is the first shape
  that emits a real fail2 fall-through (`jne loop` + second epilogue),
  but then head-empty cannot share fail1 — extra inline `return 0`
  (91i/237B) or closed-empty retargets to the later copy. Head-empty
  `je fail1` and fail2 fall-through fight; cannot hold both. Volatile
  store / noinline pair puts a second xor-ret after the helpers.
  `#pragma optimize("g", off)` still ~11–72%. `if (0)` outlining not
  retried. AddSpanRecord rechecked 37/61 — unchanged.
- `Raster_AddSpanRecord`: FLOOR 61/61, 175/175, 35 mismatch (~61%).
  Count-before-cursor gives `push ecx` / `mov ecx,[cursor]` /
  `lea eax,[ecx+0x10]` / `jbe` vs `0x004e3870`. `keep=0` is the
  `xor / test ne / jle`; volatile `&saved` is the 4-byte home.
  Residual: count in esi not ebx (push-ecx frame skips ebx; no
  bl/bh use in the original either), `saved=cur` CSE's so y is
  `[ecx+4]` not `mov edx,ecx` / `[edx+4]`, keys-1 IV
  (`lea edx,[keys-8]` / `[edx]` vs `add edx,8` / `[edx-8]`),
  `dec edi` not `dec ebx`. Dropping the volatile home yields
  `dec ebx` but loses the frame (58i, cursor in edi). `int yy=y`
  and an early `ebase=edges` do not create the edx copy.
  **Fable IV probe (reverted):** ridemisc `EarthSlide_LaunchCar`
  biased `int*` on `&keys->idx`, `k += 2`, `k[-2]` for y. 38/61
  (was 37). Replaces keys-1 with an idx-anchored `add edx,4` /
  `[edx]`; does not emit `add edx,8` / `[edx-8]`. ebx/esi, missing
  `mov edx,ecx`, and `dec edi` unchanged.

## Extern-type divergences

- `Curve_InitLine` dest is `void*` here (PieceDesc*); coaster7.c uses
  `RouteGeom*`.
- `TrackJoint.h` is `float` here; coaster.c names +0x04 `int f04`.
- `GetTrackNodeWorldPos` returns `void*` / `PieceObj*` here; schoolcar7.c
  uses `TrackObj*`.

## Original bugs

None in the exact bodies. GetTrackSegmentPiece's first world-pos out at
`[esp+0x14]` overlaps the saved esi/edi slots with world.y/z; only world.z
is consumed (as `[esp+0x28]` after the eval pushes) and edi stays live in
the register.
