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
| 0x00423f40 | GetTrackSegmentPiece | 86 | 92 | WIP | WIP (9 mismatch, no-neighbor tail) |
| 0x00424050 | GetTrackSegment | 87 | 72 | WIP | WIP (87/87 i; fail tails / regs) |
| 0x00425da0 | Vec3f_Equal | 25 | 100 | [OK] | FUNCTION |
| 0x00426190 | Mat4_Transpose | 21 | 71.4 | WIP | WIP (eax/ecx cursors swapped) |
| 0x004263a0 | ProjectVertsToRect | 72 | ~33 | WIP | WIP (72/72 i after FTOI; walk) |
| 0x00426460 | Mat3_FromMat4Transpose | 21 | 71.4 | WIP | WIP (same cursor swap) |
| 0x004265d0 | ClipRect_ClipTo | 66 | 98.5 | WIP | WIP (2 arg-pointer loads swapped) |
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

**16 / 24 exact.** All 24 have bodies. `audit.py` PASS, `relocs.py` zero
MISMATCH on FUNCTION bodies, `/W3` clean.

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

- `ClipRect_ClipTo`: 66/66, 122/122. Only `mov eax,[esp+8] / mov ecx,[esp+4]`
  vs the reverse. Volatile clip load sinks the pair past the pushes (92.4%).
  Two-instruction scheduling floor.
- `Mat4_Transpose` / `Mat3_FromMat4Transpose`: 21/21, 46/46. dest cursor
  ecx, source cursor eax; original swapped. Return dest, named int temp,
  dest-first declaration all worse or identical.
- `GetTrackSegmentPiece`: 80/86 after `at.z += world.z` (fst to at.z) and
  taking `&world` before `*link` so the first call's pushes sandwich the
  sete. Tail: original reloads `jout.node` into ecx (kills the copied y,
  so `add [p1+4],-16`); this build keeps y in ecx and uses edx for the
  reload. Extra `jn` / `ok` locals collapse node into esi and lose 25%.
- `TrackPiece_FindIndex`: `off += slope` in the `slope == 8` arm still
  folds to `lea eax,[esi+8]`; original is `add esi,ebx / mov eax,esi`.
  `>> 1` not `/2` (sar vs cdq).
- `ProjectVertsToRect`: FTOI (`fld`/`fistp`) brings the count to 72/72.
  Residual is the 2×3 strength-reduced walk and using the dead `n` slot
  as the convert temp (`[ebp+0x10]`).
- `Raster_AddSpanRecord`: instruction count exact; keys/edge cursors and
  the overflow compare are allocation.
- `GetTrackSegment`: 87/87 after hoisting `tile.x` and writing each walk
  as `for (;;)` with `nxt` and a sentinel break. Residual is register
  identity on the three fail `xor eax,eax` tails and the two helper
  call sites' push order.

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
