# Scope Codex-F — coaster draw passes, rider updates, route callees (2026-09-05)

> **Status: MERGED 2026-09-09 — 26 of 26 exact.** Branch
> `codex/scope-f`. The Codex series' sixth scope (after CODEX_A–E). Unrelated
> to `SCOPE_F_partials_rides.md`, the generic-series scope F (FGH, elsewhere).
> Say "Codex-F" when referring to this one. `coaster10.c` (16), `ridemachine2.c` (5)
> and `uistubs2.c` (5) are all exact on `main`; the branch has been deleted
> and its evidence is in `docs/lanes/codex-f.md`.

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here.** Notes: `docs/lanes/codex-f.md`. Object prefix: `/tmp/cf_`.

NEW-FUNCTION scope, ≈1,300 instructions — the tier of callees that
Codex-E's ride machines and coaster helpers exposed.

## `LEGOLAND/coaster10.c` — model draw passes and route/curve callees (≈880 insns)

The three `CoasterModel_DrawPass*` are 161/165/182 instructions — expect one
shape with per-pass data (disassemble all three, diff, build one, transfer).
`coaster9.c` (Codex-E: `Coaster3D_DrawModel` is their caller), `coaster3d.c`
(the view matrix, `TransformVerts`, `Raster_SubmitPoly`), `coastermath.c`
(`MatMul`, `Mat3_ToMat4`, `Vec3Dot`), `coastertiny.c`/`coaster8.c`
(`RouteNode_*`, `RouteSeat_*`, the clip-rect list), `schoolcar4.c`/
`schoolcar8.c` (`TrackCurve_*` — the cubic vertical profile, the arc
position/tangent) document every one of these from the caller's side.

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x0041e640 | `RouteNode_SetPending` | 11 | coaster9.c |
| 0x00429b90 | `TrackCurve_EvaluateUp` | 11 | coaster9.c |
| 0x00420fb0 | `CoasterModel_GetClipRect` | 12 | coaster9.c |
| 0x0041e670 | `RouteNode_CanAdd` | 14 | coaster9.c |
| 0x00426510 | `Mat3_TransposeToMat4` | 18 | coaster9.c |
| 0x00426700 | `ClipRect_SetBounds` | 19 | coaster9.c |
| 0x00429a80 | `TrackCurve_EvaluatePosition` | 25 | coaster9.c |
| 0x0041dca0 | `Route_GetSpeed` | 29 | coaster9.c |
| 0x0041ea70 | `RouteNode_LinkPending` | 40 | coaster9.c |
| 0x004261c0 | `TransformVec3` | 42 | coaster9.c |
| 0x00429c60 | `TrackCurve_EvaluateDerivative` | 42 | coaster9.c |
| 0x00422400 | `ModelImage_FindName` | 43 | coaster9.c |
| 0x0041e9e0 | `RouteNode_GetTransform` | 45 | coaster9.c |
| 0x00420810 | `CoasterModel_DrawPass1` | 161 | coaster9.c |
| 0x00420a20 | `CoasterModel_DrawPass2` | 165 | coaster9.c |
| 0x00420c40 | `CoasterModel_DrawPass3` | 182 | coaster9.c |

## `LEGOLAND/ridemachine2.c` — copter and tower rider updates (≈415 insns)

`ridemachine.c` (Codex-E) is the caller and documents the copter car
enumeration order 1,0,2,3,4, the tower's four 0x24-byte car records and
their derived "in service"; `bswater3.c` has `SpaceTower_PlaceCar`/
`DrawCar*`; `ridemisc.c` has `Copters_SetFull`.

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00404860 | `Copters_StepCar` | 23 | ridemachine.c |
| 0x0043a940 | `SpaceTower_StepCar` | 37 | ridemachine.c |
| 0x004049a0 | `Copters_StopRide` | 71 | ridemachine.c |
| 0x0043b810 | `SpaceTower_UpdateRiders` | 116 | ridemachine.c |
| 0x00404630 | `Copters_UpdateCarRider` | 168 | ridemachine.c |

## `LEGOLAND/uistubs2.c` — five stubs (≈26 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x0046d390 | `SetHelpFaceState5` | 2 | uimisc3.c |
| 0x00492da0 | `RestartMusic` | 5 | uimisc2.c |
| 0x00457870 | `sub_457870` | 6 | uimisc2.c |
| 0x00489ee0 | `sub_489ee0` | 6 | uimisc2.c |
| 0x00499410 | `ResetGameClock` | 7 | uimisc2.c |

Name the two `sub_*`. Skip `DirectSoundCreate` (0x0049d31a) — an import
thunk, not game code.

**Order:** `uistubs2.c` → `coaster10.c` smallest first (the three draw
passes last, as one family) → `ridemachine2.c` smallest first.

## Owned elsewhere — do not create or edit

`movie.c`, `pathmask.c`, `texture.c` (scope K); every file in scopes F–J;
every existing `.c`.
