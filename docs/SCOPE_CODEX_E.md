# Scope Codex-E — ride machines and the coaster's last callees (2026-09-05)

> **Status: OPEN, unclaimed.** Branch `codex/scope-e`. This is the Codex
> series' fifth scope (after CODEX_A–D) and is unrelated to
> `SCOPE_E_frontier_tail.md`, the generic-series scope E, which is already
> merged. Say "Codex-E" when referring to this one.

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here.** Branch: `codex/scope-e`. Notes: `docs/lanes/codex-e.md`. Object
prefix: `/tmp/ce_`. The follow-on to Codex scope D (49 of 49 exact, merged).

NEW-FUNCTION scope, ≈1,490 instructions. Every merge exposes another tier of
callees, and this is the tier the ride-record and coaster work just exposed.

## `LEGOLAND/ridemachine.c` — the ride state machines (≈1,080 insns)

The seven `*_StepMachine` are almost certainly ONE shape with per-ride data
(the tiny `*_TickMachine` wrappers in `ridetiny.c` call them identically).
Disassemble all seven, group, build one body carefully, diff the siblings.
The `*_StopRide` five likewise. `ridetiny.c`, `ridemisc3.c`, `ridemisc4.c`,
`bswater2.c`/`bswater3.c` (the space tower's cars and seats), `mechrides.c`
and `ridemisc.c` (`Copters_SetFull`) are the templates and the callers.

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00404a90 | `Copters_StepMachine` | 115 | ridetiny.c |
| 0x0043c7f0 | `SpinningBarrels_StepMachine` | 113 | ridetiny.c |
| 0x0043e2b0 | `PlaneRide_StepMachine` | 112 | ridetiny.c |
| 0x004150c0 | `SafariRide_StepMachine` | 111 | ridetiny.c |
| 0x004161f0 | `SpiderRide_StepMachine` | 103 | ridetiny.c |
| 0x0043b990 | `SpaceTower_StepMachine` | 94 | ridetiny.c |
| 0x0042d560 | `EarthSlide_StepMachine` | 53 | ridetiny.c |
| 0x00403e90 | `Copters_InitRecord` | 118 | ridemisc4.c |
| 0x0043aac0 | `SpaceTower_StopRide` | 56 | ridemisc4.c |
| 0x0043d9f0 | `PlaneRide_StopRide` | 37 | ridemisc4.c |
| 0x00414b10 | `SafariRide_StopRide` | 36 | ridemisc4.c |
| 0x00415a90 | `SpiderRide_StopRide` | 26 | ridemisc4.c |
| 0x0043c2f0 | `SpinningBarrels_StopRide` | 11 | ridemisc4.c |
| 0x0043acb0 | `SpaceTower_PickSeat` | 22 | ridemisc4.c |
| 0x0043aa10 | `SpaceTower_StartSound` | 18 | ridetiny.c |
| 0x004159e0 | `SpiderRide_StartSound` | 18 | ridetiny.c |
| 0x00406e90 | `GoldRush_HasFreePan` | 18 | ridetiny.c |
| 0x0042ce90 | `EarthSlide_AppendQueue` | 16 | ridemisc4.c |

## `LEGOLAND/coaster9.c` — the coaster's last callees (≈410 insns)

`coaster7.c`, `coaster8.c`, `coastertiny.c`, `schoolcar8.c` and `coaster3d.c`
document every one of these from the caller's side (the RK4 descriptor, the
route seats' four inline method slots at +0x10..+0x1c — `RouteSeat_Update` is
slot +0x1c — the clip-rect list, the model records).

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00420e90 | `Coaster3D_DrawModel` | 100 | coastertiny.c |
| 0x00427410 | `RouteSeat_Update` | 61 | coaster8.c |
| 0x00422340 | `ModelImage_FindRecord` | 40 | coaster8.c |
| 0x00429bb0 | `TrackCurve_EvaluateOffset` | 39 | coastertiny.c |
| 0x00422300 | `ModelRecord_CopyToken` | 25 | coaster8.c |
| 0x0041e990 | `RouteNode_UpdateClipRect` | 25 | coastertiny.c |
| 0x0041dd00 | `Route_TravelPerTick` | 23 | coaster7.c |
| 0x0041eaf0 | `RouteNode_AddPending` | 22 | coastertiny.c |
| 0x0041dd70 | `Route_SumMass` | 20 | coaster7.c, coastertiny.c |
| 0x00425c40 | `Coaster3D_SetCarClipDepth` | 19 | coastertiny.c |
| 0x0041dd50 | `Route_TravelThisTick` | 12 | coaster7.c |
| 0x004273f0 | `RouteSeat_DetachCar` | 8 | coaster8.c |
| 0x00426a90 | `VecMath_Sqrt` | 8 | coaster7.c |
| 0x004203f0 | `PhysObj_WriteState` | 8 | coastertiny.c |

Plus every remaining coaster callee under 8 instructions that
`$PY tools/callees.py | awk '$4 ~ /coaster|schoolcar/'` still lists when you
get there (`RouteNode_LinkClipRect`/`UnlinkClipRect`/`ClearActive`,
`PhysObj_ReadState`, `CoasterModel_FindPartIndex`/`FindMeshIndex`, …).

**Order:** `coaster9.c` smallest first, then `ridemachine.c` — `StopRide`
five, `StartSound` pair, then the seven `StepMachine` as one family, then
`Copters_InitRecord`.

## Owned elsewhere — do not create or edit

`movie.c`, `pathmask.c`, `texture.c` (scope K); `lfmisc2.c`, `musicthread.c`
(a running lane); every file in scopes F–J; every existing `.c`.
