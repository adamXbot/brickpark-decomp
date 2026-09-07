# Scope LL6 — raster / map / track-piece callbacks

Branch `scope/LL6`. File `LEGOLAND/coaster12.c`. Object prefix `/tmp/sll6_`.
Brief: `docs/SCOPE_LL6_raster_map_track.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00423200 | Raster_AddSpanRecord | 61 | 61/61 i | WIP | WIP (allocation; 61 mismatch) |
| 0x00423940 | TrackNode_InitStationJoints | 8 | 100 | [OK] | FUNCTION |
| 0x00423970 | TrackNode_SetStationHeights | 6 | 100 | [OK] | FUNCTION |
| 0x00423990 | Castle_GetCurveA | 2 | 100 | [OK] | FUNCTION |
| 0x00423f40 | GetTrackSegmentPiece | 86 | 100 | [OK] | FUNCTION |
| 0x00424050 | GetTrackSegment | 87 | ~78 | WIP | WIP (47 mismatch; tail/head latch) |
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
| 0x004283c0 | TrackPiece_FindIndex | 103 | ~83 | WIP | WIP (lea vs add esi,ebx; switch) |
| 0x004286e0 | TrackNode_GetPieceDesc | 8 | 100 | [OK] | FUNCTION |
| 0x00428840 | LowestSetBitIndex | 10 | 100 | [OK] | FUNCTION |

**21 / 24 exact.** All 24 have bodies. `audit.py` PASS, `relocs.py` zero
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
- `TrackPiece_FindIndex`: jump table on `slope-1` is right (12→0, 3→1,
  4→2, 1→3, 6→4, 9→5, 14→6, 11→7, else -1). Straight arms still
  `lea eax,[esi+K]` against original `add esi,ebx/imm / pop edi /
  mov eax,esi`. An inlined `AddOff(off, slope)` does not stop the fold.
  `>> 1` not `/2` (sar vs cdq).
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
- `GetTrackSegment`: 87/87, 237/232, 47 mismatch (was 69). Closed walk
  is the original `do { sx; nxt; match; n=nxt; } while (nxt != ring);
  return 0` — `jne loop` then an inline `pop/xor/ret`, and closed-empty
  `je`s to that fail. Head-empty also jumps back to it. Residual: tail
  and head still `je exit / jmp loop` because the tail match call is
  flushed between tail and head (so the tail loop cannot fall through
  to head), and head exhaust merges into fail1 instead of a second
  `xor eax,eax` copy. Two helper sites exist; their push-phase order
  is swapped versus the original (tail should be link+p1 first).
- `Raster_AddSpanRecord`: 61/61, 172/175, 58 mismatch. Count increments
  first; unsigned `cursor+0x10` vs `0x004e3870`; `ne==0` skips the
  write. Residual is keys cursor (`add edx,8` then y from `[edx-8]`
  vs `add edx,-8`) and an extra edi save.

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
