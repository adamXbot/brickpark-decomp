# Codex-F — in progress (2026-09-07)

Branch: `codex/scope-f`.
Worktree: `.worktrees/codex-f`.
Owned files: `LEGOLAND/uistubs2.c`, `LEGOLAND/coaster10.c`,
`LEGOLAND/ridemachine2.c`, this note.
Object prefix: `/tmp/cf_`.

## Status

| File | Exact | WIP | Total |
| --- | ---: | ---: | ---: |
| `uistubs2.c` | 5 | 0 | 5 |
| `coaster10.c` | 14 | 2 | 16 |
| `ridemachine2.c` | 3 | 2 | 5 |
| **lane** | **22** | **4** | **26** |

`/W3` clean. Exact bodies: audit `[OK]`, relocs zero MISMATCH.

## Scoreboard (all 26)

### `uistubs2.c` — 5/5 exact

| Address | Function | Insns | Audit |
| --- | --- | ---: | --- |
| `0x0046d390` | `SetHelpFaceState5` | 2 | OK |
| `0x00492da0` | `RestartMusic` | 5 | OK |
| `0x00457870` | `SetBrickLimit` | 6 | OK |
| `0x00489ee0` | `ClearMarkedTiles` | 6 | OK |
| `0x00499410` | `ResetGameClock` | 7 | OK |

### `coaster10.c` — 14/16 exact, 2 WIP

| Address | Function | Insns | Audit | Residual |
| --- | --- | ---: | --- | --- |
| `0x0041e640` | `RouteNode_SetPending` | 11 | OK | — |
| `0x00429b90` | `TrackCurve_EvaluateUp` | 11 | OK | — |
| `0x00420fb0` | `CoasterModel_GetClipRect` | 12 | OK | — |
| `0x0041e670` | `RouteNode_CanAdd` | 14 | OK | — |
| `0x00426510` | `Mat3_TransposeToMat4` | 18 | OK | — |
| `0x00426700` | `ClipRect_SetBounds` | 19 | OK | — |
| `0x00429a80` | `TrackCurve_EvaluatePosition` | 25 | OK | — |
| `0x0041dca0` | `Route_GetSpeed` | 29 | OK | — |
| `0x0041ea70` | `RouteNode_LinkPending` | 40 | OK | — |
| `0x004261c0` | `TransformVec3` | 42 | WIP | ~78.6%, ebp/ebx swap |
| `0x00429c60` | `TrackCurve_EvaluateDerivative` | 42 | WIP | ~33%, FD schedule |
| `0x00422400` | `ModelImage_FindName` | 43 | OK | — |
| `0x0041e9e0` | `RouteNode_GetTransform` | 45 | OK | — |
| `0x00420810` | `CoasterModel_DrawPass1` | 161 | OK | — |
| `0x00420a20` | `CoasterModel_DrawPass2` | 165 | OK | — |
| `0x00420c40` | `CoasterModel_DrawPass3` | 182 | OK | — |

### `ridemachine2.c` — 3/5 exact, 2 WIP

| Address | Function | Insns | Audit | Residual |
| --- | --- | ---: | --- | --- |
| `0x00404860` | `Copters_StepCar` | 23 | OK | — |
| `0x0043a940` | `SpaceTower_StepCar` | 37 | OK | — |
| `0x004049a0` | `Copters_StopRide` | 71 | WIP | edx/ebx pair swap (85.9% full) |
| `0x0043b810` | `SpaceTower_UpdateRiders` | 116 | OK | — |
| `0x00404630` | `Copters_UpdateCarRider` | 168 | WIP | ~6–24%, lea vs anim home |

## WIP notes (latest)

### `Copters_UpdateCarRider`

Jump table confirmed from binary `0x404840`:
`0→(3,0xd7) 1→(0,0xeb) 2→(4,0xe1) 3→(1,0xe6) 4→(2,0xe6)`.

Working switch picture (volatile seat + `sel=*(volatile*)&index`):
`ebx=anim`, `[ebp+0xc]=mode`, `jmp [eax*4]`, `esi=layer`. Prologue still
wants `lea edi,[ecx+eax+0x18]` with early `g_copters_def` load; volatile
seat store splits that lea. Plain `rec=&seat[index]` raises LCS but moves
anim into edi.

### `Copters_StopRide`

Flag pairing structure matches (push ebx between loads, interleaved next
pair). Residual is pure edx↔ebx swap on seat0/1 and seat3/2 — reverse
source order, volatile, and offset addressing do not flip it.

### `TransformVec3`

`float* d` with `*d++` removes the ESCAPES `d+=12` spill (78.6%). Residual
is s/col-count in ebx/ebp vs ebp/ebx. `Vec3f* d` restores ebp=s but
reintroduces the spill.

### `TrackCurve_EvaluateDerivative`

`(g_deriv_out = out, TrackCurve_DerivSample)` as the fn argument places
`mov [g_deriv_out],reg` immediately before `call` (RTL eval). Still missing
early `mov edx,out` / `lea` interleave with `g_deriv_at`/`g_deriv_mode` and
the `fld`/`fstp [eax]` PhysVec copy (~33%).

## Remaining

Close the four WIPs above. No merge until 26/26 or explicit ask.
