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
| `coaster10.c` | 12 | 4 | 16 of 16 started |
| `ridemachine2.c` | 2 | 1 | 3 of 5 |
| **lane** | **19** | **5** | **24 of 26** |

`/W3` clean. Relocs: zero MISMATCH on exact bodies (UNRESOLVED float
literals only). Rider updates still not started.

## `uistubs2.c` — 5 exact, 26 instructions

All rows: **100%, audit `[OK]`, `// FUNCTION:`**.

| Address | Function | Insns | Notes |
| --- | --- | ---: | --- |
| `0x0046d390` | `SetHelpFaceState5` | 2 | twin of `SetHelpFaceTalking` (stores 4) |
| `0x00492da0` | `RestartMusic` | 5 | `SetTheme(g_imt_theme)` |
| `0x00457870` | `SetBrickLimit` | 6 | former `sub_457870` |
| `0x00489ee0` | `ClearMarkedTiles` | 6 | former `sub_489ee0` |
| `0x00499410` | `ResetGameClock` | 7 | freezes main + aux clock bases |

### Levers

- **Signed end-pointer compare for `ClearMarkedTiles`.** Typed `MarkedTile*`
  walk emits `jb`; original latch is `jl`. Casting cursor/end to `int` with
  `p += 4` recovers `jl`.

## `coaster10.c` — 12 exact, 4 WIP

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
| `0x00420810` | `CoasterModel_DrawPass1` | 161 | OK | FUNCTION |
| `0x00420a20` | `CoasterModel_DrawPass2` | 165 | WIP | 99.4%, lea order |
| `0x00420c40` | `CoasterModel_DrawPass3` | 182 | WIP | ~67%, UV emit |

### DrawPass family (recovered)

Shared shape: RDTSC bracket → `job.kind = (mode != 0)` → three `job.v[]`
→ shader immediate → face loop (0x10 records) → edge deltas via
subscripted `g_model_vertices[tri[i]]` → area `> 0` → clip AND/OR →
light/tag/emit → `Raster_SubmitPoly` → add cycles into `g_stat_c_4dcbc8`.

| Pass | Faces | Normals | Light | Emit | Submit n |
| --- | --- | --- | --- | --- | ---: |
| 1 | +0x18/+0x1c | +0x10, face+2 | once → `job.shade` | y,x,z | 2 |
| 2 | +0x20/+0x24 | +0x14, face+0xa/c/e per vtx | per vertex → `v.shade` | y,x,shade,z | 3 |
| 3 | +0x28/+0x2c | +0x10, face+2 | once → `job.shade` | y,x,u,v,z | 4 |

Shader immediates `0x4b5648` / `0x4b5658` / `0x4b5f50` (not extern loads).
Face is 0x10 with `mat, normal, v0..v2, n0..n2`. Texture records are
12 bytes; tag is first dword at `mat*3`.

### Levers

- **`ClipRect_SetBounds` aggregate assign.** `*(SpanRect*)&clip->left = *bounds`.
- **`ModelImage_FindName` uses `_stricmp` (0x004aab90).**
- **`Route_GetSpeed`**: `2.0f * x` → `fadd st,st`; bare `(float)sqrt` → `fsqrt`.
- **`DrawPass1`**: literal shader; `tri[3]=tri[0]` early; light as
  `z*z + y*y + x*x + half`; `and_flags=0xff` then `&= clip`.
- **`DrawPass2` WIP**: per-vertex normals from `face->n0..n2` via
  `normals2`. Residual is `lea esi,[face+0xa]` before vs after the
  `lea edi/ecx` pair (1 insn order).
- **`TransformVec3` WIP**: dual modified `s`/`d` params give correct
  ebp/mul but spill `d+=12` (ESCAPES). Locals kill spill but flip mul
  order / prologue. Floor ~86.7%.
- **`TrackCurve_EvaluateDerivative` WIP**: FD arg/`g_deriv_out`
  interleave + PhysVec `[1..3]` copy not recovered (~55%).
- **`Copters_StopRide` WIP**: VC6 always loads higher seat offset into
  edx when pairing flag clears; original wants lower in edx. Floor
  85.9% after many register-order attempts.

## `ridemachine2.c` — 2 exact, 1 WIP

| Address | Function | Insns | Audit | Marker |
| --- | --- | ---: | --- | --- |
| `0x00404860` | `Copters_StepCar` | 23 | OK | FUNCTION |
| `0x0043a940` | `SpaceTower_StepCar` | 37 | OK | FUNCTION |
| `0x004049a0` | `Copters_StopRide` | 71 | WIP | 85.9%, flag-pair swap |
| `SpaceTower_UpdateRiders` | — | 116 | not started | — |
| `Copters_UpdateCarRider` | — | 168 | not started | — |

### Levers

- **`SpaceTower_StepCar`**: `switch` with **case 2 before case 1**.
- **`Copters_StopRide`**: see floor note above; frame/rider order 1,0,2,3,4.

## Remaining

- Close or leave floored: TransformVec3, EvaluateDerivative, Copters_StopRide.
- Finish DrawPass2 (lea order) and DrawPass3 (UV emit).
- `SpaceTower_UpdateRiders`, `Copters_UpdateCarRider`.
