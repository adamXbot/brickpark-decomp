# Scope LL3 — route / joint / span clip callees

Branch `scope/LL3`. File `LEGOLAND/coaster11.c`. Object prefix `/tmp/sll3_`.
Brief: `docs/SCOPE_LL3_route_joint_span.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0041f030 | Span_SetVertexBuf | 4 | 100 | [OK] | FUNCTION |
| 0x0041cd20 | JointSlot_TestSquare | 7 | 100 | [OK] | FUNCTION |
| 0x0041e7f0 | RouteCar_GetHeading | 10 | 100 | [OK] | FUNCTION |
| 0x0041f4c0 | Span_AllocPairs | 13 | 100 | [OK] | FUNCTION |
| 0x0041e8f0 | RouteCar_PlaceAndBind | 25 | 100 | [OK] | FUNCTION |
| 0x0041cd40 | JointSlot_Find | 27 | 70 | 9 mis | WIP |
| 0x0041cd80 | JointSlot_Set | 45 | 100 | [OK] | FUNCTION |
| 0x0041db20 | Route_CollectCarSample | 37 | 100 | [OK] | FUNCTION |
| 0x0041ede0 | MapCell_AllowTrack | 38 | 100 | [OK] | FUNCTION |
| 0x0041d210 | TrackFitFindPartners | 60 | 100 | [OK] | FUNCTION |
| 0x0041f2b0 | Raster_ClipAgainstPlanes | 55 | 100 | [OK] | FUNCTION |
| 0x0041d950 | Route_SetTrainAt | 66 | 100 | [OK] | FUNCTION |
| 0x00411fa0 | LFQueue_StepRider | 74 |  |  |  |
| 0x0041db90 | Route_GetMassAndPower | 77 |  |  |  |
| 0x0041ee40 | TrackPlace_TestSquare | 79 | 96 | 60 mis | WIP |
| 0x0041ef60 | Raster_ClipPoly | 80 |  |  |  |
| 0x0041f3e0 |  | 85 |  |  |  |
| 0x0041c940 | BsRoute_Trace | 130 |  |  |  |
| 0x0041f050 |  | 179 |  |  |  |

**11 / 19 exact.** Find: slot/bit ecx/edx vs edx/ecx. TestSquare: x=sx+x0
schedule (`add esi,eax` vs `mov esi,eax / add esi,edx`), 191B vs 192B.

## Names

Caller-given names kept: `JointSlot_Set`, `TrackFitFindPartners`,
`Route_SetTrainAt`. New: `Span_SetVertexBuf`, `JointSlot_TestSquare`,
`RouteCar_GetHeading`, `Span_AllocPairs`, `RouteCar_PlaceAndBind`,
`JointSlot_Find`, `Route_CollectCarSample`, `MapCell_AllowTrack`,
`Raster_ClipAgainstPlanes`, `TrackPlace_TestSquare`.

## Mechanics

- **Span_SetVertexBuf**: stores PolyVtx stride 0x1c at 0x004b5608 and the
  live vertex pointer at 0x004b560c.
- **JointSlot_TestSquare**: forwards (square, probe-ctx) to 0x0041ee40.
- **RouteCar_GetHeading**: copies RouteNode +0xb8..+0xc0 (dx/dy/dz).
- **Span_AllocPairs**: allocator at +0x24 gets `(n+1)*(n+2)/2` slots.
- **RouteCar_PlaceAndBind**: `RouteCar_SetPosition` then re-bind both bogie
  cursors via 0x0042a5e0.
- **JointSlot_Set**: four direction bits; each live bit writes
  `base + g_joint_delta[i]` and keeps the bit if the probe returns 1.
- **Route_CollectCarSample**: `PositionRouteCars` on `g_route_eval`, then
  every node of the +0x70 ring (including the head) contributes velocity
  into +0x04 and a heading Vec3f; count = 1 + 3 * nodes.
- **MapCell_AllowTrack**: NULL and flag 0x40 succeed; no 0x8f8 bits, the
  environment class, flag 0x800, or class flag 0x200000 on a 0x88 cell
  refuse.
- **TrackFitFindPartners**: refuse closed circuit (state 2); stamp owner
  `&g_castle`; head offset +0x18/+0x1a against head_slot, tail +0x10/+0x12
  against tail_slot; a hit writes `1<<index` into jout.dir / jin.dir.
  Fail only when both indices are -1.

## Levers

- **RouteCar_GetHeading**: mutate the `n` parameter (`n = (RouteNode*)((char*)n + 0xb8)`)
  to emit `add eax, 0xb8`; a derived `Vec3f*` folds to `[eax+0xb8]`.
- **Span_AllocPairs**: `(m * (m + 1)) >> 1` with `m = n + 1` emits
  `inc / lea / imul / sar`. `/ 2` becomes `cdq`.
- **JointSlot_Set**: `for (i = 0; i <= 3; i++)` (not `i < 4`) so the
  strength-reduced byte cursor compares `esi, 0x10 / jle` rather than
  `0x14 / jl`. Probe args are `(square, &g_joint_probe)` — cdecl
  `push 0x4b5570 / push dst`.
- **Route_CollectCarSample**: `count = 1` before `n = &rt->head` so
  `mov ebp,1 / lea esi,[eax+0x70]` sit between the
  `PositionRouteCars` pushes and the call (esi/ebp are callee-saved).
  Loop latch reloads `&g_route_eval->head`, not a cached pointer.
- **TrackFitFindPartners**: one trailing `return 0` (`if (state != 2) {
  ...; if (a != -1 || b != -1) return 1; } return 0;`). Local
  `PackedSquare at` reuses the dead `d` argument slot.
- **Route_SetTrainAt**: same walk as PositionRouteCars but
  `RouteCar_PlaceAndBind`; CoasterRoute +0x20 is the unused `dimension`
  word so `f24` stays at +0x24 and `head` at +0x70.
- **Raster_ClipAgainstPlanes**: dest-relative dword copy
  (`edx = src - dest; [edx+ecx]`), then ping-pong `g_clip_ping[i&1]` /
  `[(i-1)&1]`. Latch is `i++; planes += 0xc` (not a for-increment).
  Reuses `n` as the clip result so the next plane sees the survivor count.
- **JointSlot_Find** (WIP): need the slot walker in edx and the bit in ecx
  (`test ecx, esi`). `int i = 0` first puts the walker in ecx; no early
  zero puts the walker in eax then `lea edx,[eax+4]`.
- **TrackPlace_TestSquare** (WIP): do-while over the PlaceRect list
  (first node is dereferenced with no NULL test — original bug). x = sx+x0
  still emits `mov esi,[x0] / add esi,eax` instead of `mov esi,eax /
  add esi,edx`.
