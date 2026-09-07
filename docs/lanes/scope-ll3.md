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
| 0x0041cd40 | JointSlot_Find | 27 | 100 | [OK] | FUNCTION |
| 0x0041cd80 | JointSlot_Set | 45 | 100 | [OK] | FUNCTION |
| 0x0041db20 | Route_CollectCarSample | 37 | 100 | [OK] | FUNCTION |
| 0x0041ede0 | MapCell_AllowTrack | 38 | 100 | [OK] | FUNCTION |
| 0x0041d210 | TrackFitFindPartners | 60 | 100 | [OK] | FUNCTION |
| 0x0041f2b0 | Raster_ClipAgainstPlanes | 55 | 100 | [OK] | FUNCTION |
| 0x0041d950 | Route_SetTrainAt | 66 | 100 | [OK] | FUNCTION |
| 0x00411fa0 | LFQueue_StepRider | 74 | 72 | 52 mis | WIP |
| 0x0041db90 | Route_GetMassAndPower | 77 | 70 | 51 mis | WIP |
| 0x0041ee40 | TrackPlace_TestSquare | 79 | 96 | 60 mis | WIP |
| 0x0041ef60 | Raster_ClipPoly | 80 | 75 | 47 mis | WIP |
| 0x0041f3e0 | Span_FillEvalTable | 85 | 95 | 29 mis | WIP |
| 0x0041c940 | BsRoute_Trace | 130 | FLOOR | 105 mis | WIP |
| 0x0041f050 | Span_ClipPlane | 179 | 9 | 178 mis ESCAPES | WIP |

**12 / 19 exact.** Relocs on FUNCTION bodies: 0 MISMATCH. `/W3` clean.

## Names

Caller-given names kept: `JointSlot_Set`, `TrackFitFindPartners`,
`Route_SetTrainAt`, `LFQueue_StepRider`, `BsRoute_Trace`,
`Route_GetMassAndPower`, `Raster_ClipPoly`. New: `Span_SetVertexBuf`,
`JointSlot_TestSquare`, `RouteCar_GetHeading`, `Span_AllocPairs`,
`RouteCar_PlaceAndBind`, `JointSlot_Find`, `Route_CollectCarSample`,
`MapCell_AllowTrack`, `Raster_ClipAgainstPlanes`, `TrackPlace_TestSquare`,
`Span_FillEvalTable`, `Span_ClipPlane`.

## Mechanics

- **Span_SetVertexBuf**: stores PolyVtx stride 0x1c at 0x004b5608 and the
  live vertex pointer / lerp dword bound at 0x004b560c. ClipPoly passes
  `n + 1` (an integer bound, not a pointer).
- **JointSlot_TestSquare**: forwards (square, probe-ctx) to 0x0041ee40.
- **RouteCar_GetHeading**: copies RouteNode +0xb8..+0xc0 (dx/dy/dz).
- **Span_AllocPairs**: allocator at +0x24 gets `(n+1)*(n+2)/2` slots.
- **RouteCar_PlaceAndBind**: `RouteCar_SetPosition` then re-bind both bogie
  cursors via 0x0042a5e0.
- **JointSlot_Find**: walk the slot's four PackedSquares; only bits set in
  the mask are compared. Returns the index or -1.
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
- **Route_GetMassAndPower**: save `rt->pos` / `rt->f24`, set `g_route_eval`,
  copy pos to `g_route_eval_at`, call 0x0041f4e0 (`Span_EvalRange`) with
  `Route_CollectCarSample`, ops at 0x004d8270, saved `f24`, `0.1f`. `*power`
  is the sample energy; `*mass` is the sum over cars of
  `|heading|^2 * GetAcceleration * 2.52015616e-06f` (0x4ab400). Restore
  pos/f24; push `*mass` into the 64-slot ring at 0x004d829c.
- **Raster_ClipPoly**: mask 0xf is a no-op (`*count = 3`). Otherwise
  `SetVertexBuf(n+1)` and clip against the low/high nibble of the mask:
  flags 3 → 4 planes at ctx+4; 2 → 2 planes at ctx+0x1c; 1 → 2 planes at
  ctx+4.
- **Span_FillEvalTable**: alloc `(n+1)*(n+2)/2` via vtable +0x20, fill row
  pointers at 0x004d88cc, evaluate `eval` at n+1 samples centred on `a`
  with step `b`, then combine adjacent pairs down the rows via +0x04.
- **Span_ClipPlane**: close `in[n] = in[0]`; negative plane distance is
  inside. Both-inside emits prev; leave emits prev + lerp; enter emits
  lerp. Lerp walks dwords `0..g_span_vtx` through `__ftol` (0x00458930)
  into `*cursor` and advances by `g_span_vtx_stride`.
- **BsRoute_Trace**: twin of `JungleCruise_TraceRoute`. DFS over the
  school's water graph (step 5); west is a tail-call that VC6 turns into
  a loop. `*ok` is a success flag.

## Levers

- **RouteCar_GetHeading**: mutate the `n` parameter (`n = (RouteNode*)((char*)n + 0xb8)`)
  to emit `add eax, 0xb8`; a derived `Vec3f*` folds to `[eax+0xb8]`.
- **Span_AllocPairs**: `(m * (m + 1)) >> 1` with `m = n + 1` emits
  `inc / lea / imul / sar`. `/ 2` becomes `cdq`.
- **JointSlot_Set**: `for (i = 0; i <= 3; i++)` (not `i < 4`) so the
  strength-reduced byte cursor compares `esi, 0x10 / jle` rather than
  `0x14 / jl`. Probe args are `(square, &g_joint_probe)` — cdecl
  `push 0x4b5570 / push dst`.
- **JointSlot_Find**: `int i = 0; int bit = 1; int mask = *p` — **bit
  before mask** so TEST is `test ecx,esi`. Mask-first (any `&` spelling)
  emits `test esi,ecx` and is 26/27. First insn is `mov edx,[esp+8]`.
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
- **TrackPlace_TestSquare** (WIP): do-while over the PlaceRect list
  (first node is dereferenced with no NULL test — original bug). x = sx+x0
  still emits `mov esi,[x0] / add esi,eax` instead of `mov esi,eax /
  add esi,edx` (191B vs 192B). Volatile x0, live-sx latch, and a `Pos`
  wrapper all scramble allocation (53–55%) for a size-exact 192B.
- **Raster_ClipPoly** (WIP): `if (mask == 0xf)` exiles the early-out
  (`je` vs original `jne` fall-through). switch(flags) gives the dec/je
  chain and esi/edi, but cases 1 and 2 (both `planes=2`) share one call
  tail. Original duplicates all three. Signed-char nibble tests match
  `cmp dl,3 / jge` but move mask into ebx. A `goto` early-out produces
  three copies (79i) but still `je` and 71 audit mismatches.
- **BsRoute_Trace** (**FLOOR**): 130i/339B byte-exact, 105 mismatches.
  Identical NG22 phase-order ALLOCATION as `JungleCruise_TraceRoute`
  (jcroute.c / LEVERS NG22). Original: esi=x, edi=y, ebp=x1, ebx=y1, `w`
  in the dead arg3 slot `[esp+0x1c]`. Ours converts the west tail-call
  to a loop *before* allocation and ranks the dereferenced pointers
  first. Dead trailing statements restore the int ranking but emit a
  real call instead of the loop. No source spelling has both. Retired.
- **Span_FillEvalTable** (WIP): `int count = n` first puts n in ebx
  (78/82 = 95%). Missing the rows counter that overwrites the dead `a`
  slot (`mov [esp+0x20],ebx` / `dec eax` / store) — 204B vs 212B.
  `volatile int rows` emits the home but steals ebx (78%). Address-taken
  rows and `*(int*)&a = count` CSE back to the 95% form.
- **Route_GetMassAndPower** (WIP): one struct of `{f24, pos, sample[21]}`
  restores the 0x6c frame. rt still in ebp. Original stores the heading
  sum of squares over the dead `power` argument slot (`fstp [esp+0x88]`).
- **Span_ClipPlane** (WIP): 179i, ESCAPES. Need the original's 0x2c frame,
  `in++` cursor in the latch, and the three-way sign classify
  (`(prev_sign>>1)|next_sign` against 0x80000000 / 0xC0000000 / 0x40000000).
- **LFQueue_StepRider** (WIP): 74i, 180B vs 177B. CalcMoveLine wants
  to.y from the live shl register then a reload of to.x; clamp is
  `dec ax` on count, not `dec cx` on the index.
