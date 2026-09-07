# Scope LL6 — raster / map / track-piece callbacks

Branch `scope/LL6`. File `LEGOLAND/coaster12.c`. Object prefix `/tmp/sll6_`.
Brief: `docs/SCOPE_LL6_raster_map_track.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00423200 | Raster_AddSpanRecord | 61 | — | no | not started |
| 0x00423940 | TrackNode_InitStationJoints | 8 | 100 | [OK] | FUNCTION |
| 0x00423970 | TrackNode_SetStationHeights | 6 | 100 | [OK] | FUNCTION |
| 0x00423990 | Castle_GetCurveA | 2 | 100 | [OK] | FUNCTION |
| 0x00423f40 | GetTrackSegmentPiece | 86 | — | no | not started |
| 0x00424050 | GetTrackSegment | 87 | — | no | not started |
| 0x00425da0 | Vec3f_Equal | 25 | 100 | [OK] | FUNCTION |
| 0x00426190 | Mat4_Transpose | 21 | 71.4 | WIP | WIP (eax/ecx cursors swapped) |
| 0x004263a0 | ProjectVertsToRect | 72 | ~38 | WIP | WIP (fistp vs __ftol; frame) |
| 0x00426460 | Mat3_FromMat4Transpose | 21 | 71.4 | WIP | WIP (same cursor swap) |
| 0x004265d0 | ClipRect_ClipTo | 66 | 98.5 | WIP | WIP (two arg-pointer loads swapped) |
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
| 0x004283c0 | TrackPiece_FindIndex | 103 | — | no | not started |
| 0x004286e0 | TrackNode_GetPieceDesc | 8 | 100 | [OK] | FUNCTION |
| 0x00428840 | LowestSetBitIndex | 10 | 100 | [OK] | FUNCTION |

**16 / 24 exact.** `audit.py` PASS, `relocs.py` zero MISMATCH (29 matched, 0 unresolved
on the FUNCTION bodies), `/W3` clean.

## Names

- `TrackNode_RemoveNop` / `TrackNode_PlaceNop` — empty `ret` stubs in the flat
  TrackDesc at 0x004b5d20 (`+0x2c` / `+0x28`).
- `Castle_GetCurveA` — returns 0x006102f8, coaster7.c's `g_castle_curve_a`
  (castleobj.c's `g_castle_records` at the same address; not edited).
- `TrackNode_InitStationJoints` / `_SetStationHeights` — table at 0x004b5b64 /
  0x004b5b68. Dirs 2 and 8; heights from `g_station_height` (0x004b5b50) and
  the paired `g_station_h1` (0x004b5b4c, first named here).
- `TrackNode_FillOppositeJoints` / `…H` — identical twins in the flat
  (0x004b5d3c) and raised (0x004b5d74) TrackDesc vtables.
- `TrackNode_SetBothHeights` / `_LoadDescHeights` — build hooks. `h0` is tail
  (`jout.h`), `h1` is head (`jin.h`), matching coaster5.c.
- `TrackNode_AddPath` / `_RemovePath` — 2×2 `Add/RemoveRollerCoasterPath` on
  `(x,y), (x+1,y), (x+1,y+1), (x,y+1)`.
- `Piece_InitStraight` — already declared in coaster3d.c.
- `TrackNode_GetPieceDesc` — query hook: `&g_pieces[TrackPiece_FindIndex(n)]`.
- `LowestSetBitIndex` — first-set-bit scan; not `JointDir_ToIndex` (0x0041cca0).
- `Vec3f_Equal` — three `fcomp` + `test ah,0x40` equality tests.
- `Model_ProjectClipRect` — called by `CoasterModel_GetClipRect`.
- `ClipRect_ClipTo` — the helper `ClipRect_SetBounds` stores at `+0x10`.
- `Mat4_Transpose` — called by `Mat3_TransposeToMat4` (src, dest).
- `Mat3_FromMat4Transpose` — dest first, 3×3 walk of a 4×4.

## Mechanics

- TrackDesc vtables at 0x004b5d20 (flat) and 0x004b5d58 (raised) hold
  fill-opposite / height / query (`0x004286e0`) / place / remove. The
  path-carrying class at 0x004b5de8 uses Add/RemovePath and `carries_path = 1`.
- A free joint (`dir == -1`) takes `JointOppositeDir` of the other end and
  copies that end's height (dword copy of the float).
- Station piece: jin.dir=2, jout.dir=8 (a 2↔8 pair). Heights are `fild` of
  the two .data ints; both are 0 in the image and filled at runtime.
- Straight prototype: `to = edge_mid[side]; to.z -= (float)(off*6)` then
  `Curve_InitLine(out, &edge_mid[dir], &to, &half_step[slot])`. `off*6` is
  integer (`lea + shl`) then `fild`.
- `Model_ProjectClipRect(verts, pos, rot, out)`: `MakeTransform` +
  `MatMul(g_view_matrix, …)` + `ProjectVertsToRect(..., 8, out)`. Two Mat4
  locals, 0x80-byte frame.

## Levers / residuals

- `TrackNode_InitStationJoints`: `jin.h = (float)g_station_height` must sit
  between the two dir stores so the first `fild` lands between `mov [+0x14],2`
  and `mov [+0x20],8`.
- `Mat4_Transpose` / `Mat3_FromMat4Transpose`: structure, 21/21, 46/46 bytes
  and ebx/esi/edi are exact. dest cursor is ecx and source cursor is eax;
  original has the pair swapped. Tried dest-first declaration, `*d = *col`
  vs `*d++`, `float*` parameters, returning dest (worse: ebp + extra load),
  and a named int temp (42.9%). Same swap on both twins.
- `ClipRect_ClipTo`: 66/66, 122/122, every compare/clamp/early-out exact.
  Only residual is `mov eax,[esp+8] / mov ecx,[esp+4]` vs the reverse.
  Volatile clip load sinks the pair past the pushes (92.4%). At a
  two-instruction scheduling floor unless a spelling loads arg1 first
  without adding a use.
- `ProjectVertsToRect`: original is an ebp frame that `fistp`s the projected
  x/y into the dead `n` argument slot. `(int)s` emits `__ftol`. Need
  coaster3d.c's `FTOI` (`fld`/`fistp`) and the 2×3 strength-reduced walk.

## Extern-type divergences

- `Curve_InitLine` is `void*` dest here (PieceDesc*); coaster7.c declares
  `RouteGeom*`. Same callee 0x00421ab0.
- `TrackJoint.h` is `float` here (schoolcar.c); coaster.c/coaster5.c name
  the same +0x04 `int f04`. Bits are the height.

## Original bugs

None in the exact bodies so far.
