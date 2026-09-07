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
| `coaster10.c` | 14 | 2 | 16 of 16 |
| `ridemachine2.c` | 3 | 2 | 5 of 5 |
| **lane** | **22** | **4** | **26 of 26** |

`/W3` clean. Relocs: zero MISMATCH on exact bodies (UNRESOLVED float
literals only).

## `uistubs2.c` — 5 exact

| Address | Function | Insns | Marker |
| --- | --- | ---: | --- |
| `0x0046d390` | `SetHelpFaceState5` | 2 | FUNCTION |
| `0x00492da0` | `RestartMusic` | 5 | FUNCTION |
| `0x00457870` | `SetBrickLimit` | 6 | FUNCTION |
| `0x00489ee0` | `ClearMarkedTiles` | 6 | FUNCTION |
| `0x00499410` | `ResetGameClock` | 7 | FUNCTION |

### Levers

- **`ClearMarkedTiles`**: int cursor + `jl` (not typed `jb`).

## `coaster10.c` — 14 exact, 2 floored WIP

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
| `0x004261c0` | `TransformVec3` | 42 | WIP | 86.7% floor |
| `0x00429c60` | `TrackCurve_EvaluateDerivative` | 42 | WIP | ~55% floor |
| `0x00422400` | `ModelImage_FindName` | 43 | OK | FUNCTION |
| `0x0041e9e0` | `RouteNode_GetTransform` | 45 | OK | FUNCTION |
| `0x00420810` | `CoasterModel_DrawPass1` | 161 | OK | FUNCTION |
| `0x00420a20` | `CoasterModel_DrawPass2` | 165 | OK | FUNCTION |
| `0x00420c40` | `CoasterModel_DrawPass3` | 182 | OK | FUNCTION |

### DrawPass levers

Shared: RDTSC → `job.kind=(mode!=0)` → `job.v[]` → **shader immediate**
`(void*)0x4b5648/5658/5f50` → face loop (0x10) → edge deltas via
`g_model_vertices[tri[i]]` → area `>0` → clip → light/tag/emit →
`Raster_SubmitPoly` → add into `g_stat_c_4dcbc8`.

| Pass | Faces | Normals | Light | Emit | Submit |
| --- | --- | --- | --- | --- | ---: |
| 1 | +0x18 | +0x10, face+2 | once → `job.shade` | y,x,z | 2 |
| 2 | +0x20 | **+0x14**, n0/n1/n2 | per vtx → `v.shade` | y,x,shade,z | 3 |
| 3 | +0x28 | +0x10, face+2 | once → `job.shade` | y,x,u,v,z | 4 |

- **DrawPass2**: `normals2[*(&face->n0 + k)]` (not `const short* nidx=&face->n0`)
  for lea order.
- **DrawPass3**: after shade, `mode=0` as UV loop counter; tag
  `((int*)texture)[face->mat*3]`; u/v from
  `((unsigned char*)texture)[(mode+face->mat*6)*2+4/5]`.

### Floors (leave)

- **TransformVec3**: dual `s`/`d` correct ebp/mul but `d+=12` spill ESCAPES.
- **EvaluateDerivative**: FD arg / `g_deriv_out` interleave (~55%).

## `ridemachine2.c` — 3 exact, 2 WIP

| Address | Function | Insns | Audit | Marker |
| --- | --- | ---: | --- | --- |
| `0x00404860` | `Copters_StepCar` | 23 | OK | FUNCTION |
| `0x0043a940` | `SpaceTower_StepCar` | 37 | OK | FUNCTION |
| `0x004049a0` | `Copters_StopRide` | 71 | WIP | 85.9% floor |
| `0x0043b810` | `SpaceTower_UpdateRiders` | 116 | OK | FUNCTION |
| `0x00404630` | `Copters_UpdateCarRider` | 168 | WIP | ~6%, matrix loop |

### Levers

- **`SpaceTower_StepCar`**: `switch` with **case 2 before case 1**.
- **`SpaceTower_UpdateRiders`** (exact):
  1. `#pragma intrinsic(memcmp)` + `memcmp(RIDE_TILE(rider), &rec->tile, 2)`
     for `mov ax,[edi+0xc] / lea edx,[edi+0xc] / cmp` (RC01).
  2. `pos.ox = screen.ox + car_ofs.ox` (screen first) so the first sum
     stores through the dirty AdjustOffset arg slot before `add esp,4`.
  3. Zero all eight `rider_a/b`, walk `g_spacetower_def->riders`, require
     `bloke->flags & 0x80`, seat `>>1` / `&1` → car riders, car ofs − height,
     side pair from `g_tower_car_geom[car].side[side]`, then
     AdjustBloke / SetPersonPosition / SetPersonDirection(dir).
- **`Copters_StopRide` floor**: VC6 swaps edx/ebx on paired seat flag loads.
- **`Copters_UpdateCarRider` WIP**: semantics recovered (seat overwrite of
  rec arg, jump-table anim/mode `{0/0xeb,1/0xe6,2/0xe6,3/0xd7,4/0xe1}`,
  layer ofs + sprite half-width, POS frame float+mode lift, cross-product
  row into keyframe +0x24, signed 3×3 into person+0x58 via tables at
  `0x4b42a0..c4`). Residual is ebp/jump-table register picture + matrix
  loop schedule (~6% strict).

## Remaining

- Floors: TransformVec3, EvaluateDerivative, Copters_StopRide.
- Continue `Copters_UpdateCarRider` (jump-table ebx=anim / `[ebp+0xc]=mode`,
  matrix loop rematerialize seat from `[ebp+8]`).
