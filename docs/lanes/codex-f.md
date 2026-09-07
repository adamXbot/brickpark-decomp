# Codex-F — in progress (2026-09-07)

Branch: `codex/scope-f`, based on `4671f8fd` (`main`).
Worktree: `.worktrees/codex-f`.
Owned files: `LEGOLAND/uistubs2.c`, `LEGOLAND/coaster10.c`,
`LEGOLAND/ridemachine2.c`, this note.
Object prefix: `/tmp/cf_`.

## Status

| File | Exact | WIP | Total |
| --- | ---: | ---: | ---: |
| `uistubs2.c` | 5 | 0 | 5 |
| `coaster10.c` | 11 | 2 | 13 of 16 |
| `ridemachine2.c` | 2 | 1 | 3 of 5 |
| **lane** | **18** | **3** | **21 of 26** |

Draw passes (3) and the two large rider updates not started.
`/W3` clean on all three files. Relocs: zero MISMATCH on exact bodies
(UNRESOLVED float literals only).

## `uistubs2.c` — 5 exact, 26 instructions

All rows: **100%, audit `[OK]`, `// FUNCTION:`**.

| Address | Function | Insns | Notes |
| --- | --- | ---: | --- |
| `0x0046d390` | `SetHelpFaceState5` | 2 | twin of `SetHelpFaceTalking` (stores 4) |
| `0x00492da0` | `RestartMusic` | 5 | `SetTheme(g_imt_theme)` |
| `0x00457870` | `SetBrickLimit` | 6 | former `sub_457870` |
| `0x00489ee0` | `ClearMarkedTiles` | 6 | former `sub_489ee0` |
| `0x00499410` | `ResetGameClock` | 7 | freezes main + aux clock bases |

### Naming

- **0x00457870 → `SetBrickLimit`**: stores `(limited == 0)` at `g_brick_lock`.
- **0x00489ee0 → `ClearMarkedTiles`**: writes u16 `0xffff` into each
  `g_marked_tiles[i].key`.

### Levers

- **Signed end-pointer compare for `ClearMarkedTiles`.** Typed `MarkedTile*`
  walk emits `jb`; original latch is `jl`. Casting cursor/end to `int` with
  `p += 4` recovers `jl`. Evidence: first residual was `jl` vs `jb`.

## `coaster10.c` — 11 exact, 2 WIP

| Address | Function | Insns | Audit | Marker |
| --- | --- | ---: | --- | --- |
| `0x0041e640` | `RouteNode_SetPending` | 11 | OK | FUNCTION |
| `0x00429b90` | `TrackCurve_EvaluateUp` | 11 | OK | FUNCTION |
| `0x00420fb0` | `CoasterModel_GetClipRect` | 12 | OK | FUNCTION |
| `0x0041e670` | `RouteNode_CanAdd` | 14 | OK | FUNCTION |
| `0x00426510` | `Mat3_TransposeToMat4` | 18 | OK | FUNCTION |
| `0x00426700` | `ClipRect_SetBounds` | 19 | OK | FUNCTION |
| `0x00429a80` | `TrackCurve_EvaluatePosition` | 25 | OK | FUNCTION |
| `0x0041dca0` | `Route_GetSpeed` | 29 | OK | FUNCTION |
| `0x0041ea70` | `RouteNode_LinkPending` | 40 | OK | FUNCTION |
| `0x004261c0` | `TransformVec3` | 42 | WIP | 86.7%, out-pointer spill |
| `0x00429c60` | `TrackCurve_EvaluateDerivative` | 42 | WIP | 55%, FD schedule |
| `0x00422400` | `ModelImage_FindName` | 43 | OK | FUNCTION |
| `0x0041e9e0` | `RouteNode_GetTransform` | 45 | OK | FUNCTION |
| DrawPass1/2/3 | — | 161/165/182 | not started | — |

### Levers

- **`ClipRect_SetBounds` aggregate assign.** Field-wise copies miss the
  `mov ecx,esi` dest alias; `*(SpanRect*)&clip->left = *bounds` matches.
- **`ModelImage_FindName` uses `_stricmp` (0x004aab90), not strcmp.** A
  `while (GetName(...)) { if (_stricmp(name,buf)==0) return i; i++; }`
  matches; peeled/do-while forms do not. strcmp intrinsic expands inline.
- **`Route_GetSpeed`**: `2.0f * x` → `fadd st,st`; bare `(float)sqrt` →
  `fsqrt`; power out-param computed but unused.
- **`RouteNode_GetTransform` float add order**: `(a.x + b.x)` not `(b+a)` —
  source order is reverse of `fld` order.
- **`TransformVec3` WIP**: best form clones `TransformVerts` inner loops
  with `acc += *rp++ * sp[j]`. Residual is `d += 12` spilling the out slot;
  original keeps `edi` live without writeback (ESCAPES, +8 bytes).
- **`TrackCurve_EvaluateDerivative` WIP**: nonzero arm must fall through
  (`h != 0`); FD call interleaves `g_deriv_out` store with pushes; result
  copies PhysVec slots `[1..3]` into the Vec3f.

### Mechanics recovered

- Geom method table at geom+0x4c: position at `mode*8`, up at `+0x1c`,
  tangent at `mode*8+4`.
- `Route_GetSpeed` is energy conservation `sqrt(2*(E-PE)/m)`, clamped at 0.
- `LinkPending`: SetCarClipDepth → resolve both cursors → GetTransform →
  DrawModel(kind) → both seat apply hooks; one batched `add esp,0x34`.
- `CanAdd` tests cursor0.ref then cursor1.ref at +0xc / +0x44.
- `GetTransform`: midpoint of mode-2 cursor positions; first rot row is
  `a - b`; `Mat3_BuildBasis(&rot,&rot)`.

### Extern-type notes

- `TrackCurve_EvaluateDerivative` 4th arg is `float h` here; coaster9
  declares `int` (caller passes 0).
- Named `Mat4_Transpose` (0x00426190), `ClipRect_ComputeMask` (0x004265d0),
  `ModelClip_Project` (0x00426750), `Mat3_BuildBasis` (0x00429af0),
  `TrackCursor_Resolve` (0x0042a680), `TrackCurve_EvaluateTangent` (0x00429ac0).

## `ridemachine2.c` — 2 exact, 1 WIP

| Address | Function | Insns | Audit | Marker |
| --- | --- | ---: | --- | --- |
| `0x00404860` | `Copters_StepCar` | 23 | OK | FUNCTION |
| `0x0043a940` | `SpaceTower_StepCar` | 37 | OK | FUNCTION |
| `0x004049a0` | `Copters_StopRide` | 71 | WIP | 85.9%, flag-pair swap |
| `SpaceTower_UpdateRiders` | — | 116 | not started | — |
| `Copters_UpdateCarRider` | — | 168 | not started | — |

### Levers

- **`SpaceTower_StepCar`**: `switch (state)` with **case 2 before case 1**
  emits the `dec/je/dec/jne` chain; `if (state==1)`/`==2` uses `cmp`.
- **`Copters_StopRide` WIP**: flag clears need original edx/ebx pairing
  (seat0 then seat1); naive `&=` swaps the pair. Frame/rider order is
  1,0,2,3,4.

### Mechanics recovered

- Copter car: while flying, bump frame; at frames wrap, reset frame and
  dec stage; stage underflow clears flying bit.
- Tower car state 1: arm f10 to -1 then advance to 2. State 2: ascend by
  `revs` to 200 then descend by 2 to 0 and clear state.

## Not started / blocked

- Three `CoasterModel_DrawPass*` (~508 insns) — family, do last.
- `SpaceTower_UpdateRiders`, `Copters_UpdateCarRider`.
- Open residuals on TransformVec3, EvaluateDerivative, Copters_StopRide.
