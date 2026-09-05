# Codex-E — complete (2026-09-05)

Branch: `codex/scope-e`, based on `ace70c72` (`main`).
Worktree: `.worktrees/codex-e`.
Owned files: `LEGOLAND/coaster9.c`, `LEGOLAND/ridemachine.c`, this note.

**44 of 44 functions exact; no WIPs.** All 32 explicitly tabulated functions,
the nine additional named coaster leaf helpers (including the explicitly
mentioned eight-instruction `PhysObj_ReadState`), and three newly exposed
helpers are implemented. The newly exposed helpers are `RouteNode_IsPending`
(4 instructions) and both 8-instruction clip-list primitives. The latter two
also close the link/unlink wrappers completely.

The final `callees.py` sweep has no remaining unimplemented **game** callee
of 8 instructions or fewer declared by a coaster/schoolcar file. CRT/import
stubs (e.g. `rand`, the heap allocator and `SetCurrentDirectoryA`) are outside
the game's matching frontier, as in the repository's coverage definition.
Larger newly exposed callees remain extern declarations under this scope's
small-helper boundary.

## Verification

- VC6 SP3, `/O2 /Gy /Gd`, the required existing Python environment and
  compiler wrapper. Both files pass `audit.py`, **44 `[OK]`, zero rejects**.
- Both files compile clean with `/W3`.
- A separate COFF check validated **200 relocations**: 74 in `coaster9.c`,
  126 in `ridemachine.c`. Every callee/global address plus relocation addend
  agrees with the original, and pooled floats and strings agree byte for
  byte with the referenced original data. After resolving those references,
  **all remaining instruction bytes, including branch displacements and
  immediate constants, are identical for all 44 bodies**. This is stronger
  than normalized audit and caught a real draft error described below.
- No shared files, other existing C files, tools, game binary, or assets are
  included. `verify.py`, `progress.py` and `coverage.py` were not run; the
  integrating session owns the global checks.

## `coaster9.c` — 26 exact, 479 instructions, 1321 bytes

All rows: **100%, audit `[OK]`, committed marker `// FUNCTION:`**.
No diverging instruction or residual remains.

| Address | Function | Instructions | Bytes |
| --- | --- | ---: | ---: |
| `0x0041e630` | `RouteNode_ClearActive` | 3 | 8 |
| `0x0041e810` | `RouteCar_GetVelocity` | 3 | 11 |
| `0x004222f0` | `ModelImage_IsEOL` | 5 | 15 |
| `0x0041e950` | `RouteNode_LinkClipRect` | 6 | 17 |
| `0x0041e970` | `RouteNode_UnlinkClipRect` | 6 | 17 |
| `0x00422590` | `CoasterModel_FindMeshIndex` | 6 | 19 |
| `0x00422600` | `CoasterModel_FindPartIndex` | 6 | 19 |
| `0x004273c0` | `RouteSeat_IsOccupied` | 6 | 15 |
| `0x004203d0` | `PhysObj_ReadState` | 8 | 20 |
| `0x004203f0` | `PhysObj_WriteState` | 8 | 20 |
| `0x00426a90` | `VecMath_Sqrt` | 8 | 20 |
| `0x004273f0` | `RouteSeat_ReleaseCar` | 8 | 17 |
| `0x0041dd00` | `Route_TravelPerTick` | 23 | 70 |
| `0x0041dd50` | `Route_TravelThisTick` | 12 | 31 |
| `0x00425c40` | `Coaster3D_SetCarClipDepth` | 19 | 105 |
| `0x0041dd70` | `Route_SumMass` | 20 | 55 |
| `0x0041eaf0` | `RouteNode_AddPending` | 22 | 55 |
| `0x0041e990` | `RouteNode_UpdateClipRect` | 25 | 71 |
| `0x00422300` | `ModelRecord_CopyToken` | 25 | 53 |
| `0x00422340` | `ModelImage_FindRecord` | 40 | 74 |
| `0x00429bb0` | `TrackCurve_EvaluateOffset` | 39 | 96 |
| `0x00427410` | `RouteSeat_Update` | 61 | 158 |
| `0x0041e660` | `RouteNode_IsPending` | 4 | 11 |
| `0x00420e90` | `Coaster3D_DrawModel` | 100 | 286 |
| `0x004266b0` | `ClipRect_Link` | 8 | 35 |
| `0x004266e0` | `ClipRect_Unlink` | 8 | 23 |

## `ridemachine.c` — 18 exact, 1077 instructions, 3117 bytes

All rows: **100%, audit `[OK]`, committed marker `// FUNCTION:`**.
No diverging instruction or residual remains.

| Address | Function | Instructions | Bytes |
| --- | --- | ---: | ---: |
| `0x0043c2f0` | `SpinningBarrels_StopRide` | 11 | 35 |
| `0x00415a90` | `SpiderRide_StopRide` | 26 | 70 |
| `0x00414b10` | `SafariRide_StopRide` | 36 | 107 |
| `0x0043d9f0` | `PlaneRide_StopRide` | 37 | 108 |
| `0x0043aac0` | `SpaceTower_StopRide` | 56 | 175 |
| `0x0043aa10` | `SpaceTower_StartSound` | 18 | 57 |
| `0x004159e0` | `SpiderRide_StartSound` | 18 | 57 |
| `0x0043acb0` | `SpaceTower_PickSeat` | 22 | 70 |
| `0x00406e90` | `GoldRush_HasFreePan` | 18 | 45 |
| `0x0042ce90` | `EarthSlide_AppendQueue` | 16 | 42 |
| `0x004150c0` | `SafariRide_StepMachine` | 111 | 307 |
| `0x004161f0` | `SpiderRide_StepMachine` | 103 | 288 |
| `0x0043e2b0` | `PlaneRide_StepMachine` | 112 | 309 |
| `0x0043c7f0` | `SpinningBarrels_StepMachine` | 113 | 314 |
| `0x0043b990` | `SpaceTower_StepMachine` | 94 | 270 |
| `0x00404a90` | `Copters_StepMachine` | 115 | 296 |
| `0x0042d560` | `EarthSlide_StepMachine` | 53 | 141 |
| `0x00403e90` | `Copters_InitRecord` | 118 | 426 |

## Recovered behavior

- **The four rotary rides share one control-flow shape.** Bit 0 means
  running; bit `0x4000` means boarding is closing. While running, each tick
  increments the half-frame counter. A completed revolution count triggers
  `GetAllBlokesOffRide`; a successful result calls the ride's stop routine,
  and that path returns without animating riders. While closing boarding,
  matching seated/joined counts clear the closing bit and call `SetFull`.
  With some riders seated, an expired timer sets the closing bit and bars
  more riders. The ordinary path places currently riding people whose
  packed square equals the record's square, then publishes the frame to
  the ride's depth sprite.
- Safari advances a frame every **2 ticks**, wraps at **48**, and resets to
  `rand()%2 + 3` revolutions. Its frame/count fields are 32-bit. It formats
  rider seat numbers as `seat + 1` and calls `Put3DBlokesOnRide2` after the
  depth-sprite update. Spider advances every **2 ticks**, wraps at **32**,
  and resets to 3 or 4 revolutions. Plane advances every **2 ticks**, wraps
  at **97**, resets to 1 or 2 revolutions, and also advances a separate
  24-frame wheel animation every tick. Barrels advance every **3 ticks**,
  wrap at **64**, reset to 3 revolutions, and have a 32-frame wheel animation.
  The last three use signed byte frame/revolution fields.
- **Spider's original timer underflow is preserved:** unlike the other
  rotary rides, the boarding countdown is decremented even on the zero
  tick that sets the closing flag, so it becomes -1.
- Rider name buffers are existing mutable globals: Safari/Spider/Plane
  write `%02d` at byte 6 of `manbox??`; Barrels write at byte 8 of
  `BoxBloke??`. The placement calls retain the original float bit patterns
  for each ride's near/far depth constants and the zero final argument.
- **Space Tower** advances its two 16-frame decorative animations every
  tick and steps four independent 0x24-byte car records while running.
  Only when all four states are zero does it attempt to unload and stop.
  Otherwise it updates riders, places 3D people, and handles boarding.
  Stop clears ride bits `0x4001`, clears bit 0 in each car, zeros all four
  heights, chooses 7–9 revolutions separately per car, clears joined/seated,
  and fades the square's sound. Seat picking starts at `rand()%8`, wraps
  until it finds a zero slot, marks it occupied, and writes the rider's seat.
  **Original precondition/bug:** with all seats occupied it loops forever.
- **Copters** decrement a tick counter, resetting it to 2 and stepping the
  five active cars when it goes negative. Both the motion and rider-update
  runs explicitly enumerate cars **1,0,2,3,4**. When running and none of
  those cars are flying, it attempts to unload and calls `StopRide(rec,0)`.
  Boarding follows the same flags/count logic. The zero-timer path and
  ordinary path preserve separate copies of the final 3D-rider update.
- The copter initializer installs five templates, in that same order, as
  `(entry layer, ride layer, entry path, ride path)`:
  car 1 `(10,3,2,7)`, car 0 `(2,1,0,6)`, car 2 `(4,11,4,8)`,
  car 3 `(5,6,3,5)`, car 4 `(8,7,1,9)`. It clears each car's flags, looks up
  its ride-layer sprite/LLS, caches its frame count at car+0x1c and count-1
  at car+4, then calls `StopRide(rec,1)`. Failed sprite/LLS lookup preserves
  those two bytes; the allocated sixth car is untouched here.
- **Earth Slide** advances its animation every third active tick. At the
  model's frame limit it clears active, zeros the frame, calls unload,
  sets record state to 1 and returns. Ordinary active ticks place riders
  through the animation; all other ordinary ticks run the general 3D-rider
  update. Queue append changes only the head/tail/next links; it neither
  initializes the new node's next nor increments the queued count. It
  assumes head and tail are either both null or tail is valid.
- **Gold Rush** reports whether any of the six pan pointers is null;
  a missing placement record returns false.
- The two sound starters build a map-square sound source and play their
  own FX[0] sample with both integer arguments 1. Square source field +4
  is intentionally unwritten; the four stop routines that stop audio use
  the original -200 fade value. Barrels do not issue a sound call here.
- **Coaster model text** is CRLF-delimited. `ModelImage_IsEOL` reads a
  little-endian 0x0a0d word. `ModelImage_FindRecord` returns data immediately
  for index 0; otherwise it scans up to `data+length-1`, skips both bytes of
  each delimiter, and returns the requested next line or null.
  `ModelRecord_CopyToken`'s historical name is misleading: it copies the
  entire line up to CRLF, appends no NUL, and returns the output end.
  Its caller supplies the NUL; the leaf has no length guard.
- **Seats** hold a position and car pointer. Occupancy tests that pointer.
  `RouteSeat_ReleaseCar` invokes car slot +0x24 if occupied; it does not
  itself clear the pointer. `RouteSeat_Update` takes a second argument,
  a position plus a 3x3 basis. It saves all three position components,
  adds the transformed seat offset, invokes car slot +0x20 with that
  transform, and restores the saved position afterwards.
- **Physics** read/write both call the copy operation at object+0x14,
  reversing the order of the state pointer at +0x38 and the caller's buffer.
  `VecMath_Sqrt` uses the custom ST(0)-in/ST(0)-out hook at 0x00829a58,
  including the original EBP frame and round-trip through the float argument.
  Travel is `sqrt(dot(tangent,tangent)) * speed * 0.0015875f`; the current
  travel wrapper gets speed from 0x0041dca0. The inherited `Route_SumMass`
  name denotes the ring sum of the constant 0.1 returned by 0x0041e7e0,
  including the embedded sentinel. The other three-instruction accessor
  returns the float at node+0xc4.
- **Clip rectangles** are embedded at node+0xc8. Updating one constructs
  the node transform, computes bounds using model 0x0082add0, then applies
  them. Link/unlink modify the doubly linked sentinel ring at 0x00829a3c;
  unlink leaves the removed node's own links intact. `AddPending` tests
  node bit 0 and the target predicate, links eligible nodes and sets bit 0.
- **Model drawing** rejects missing model/texture pointers, transposes the
  rotation, transforms/scales the light direction, constructs the model and
  view transforms and projects vertices. Original inline `RDTSC` probes
  account the low 32-bit transform duration in 0x004dcbc8. If surface setup
  succeeds it invokes three ordered drawing passes and the restore helper.
  The pass names are ordinal, deliberately leaving their unexamined polygon
  semantics unspecified. Car-view setup copies its direction, angle and
  leading scalar separately and calculates `(angle+a)/2` and `(a-angle)/2`.

## Naming and caller-side type divergences

- **0x004273f0 is `RouteSeat_ReleaseCar`** because `coastertiny.c` already
  defines `RouteSeat_DetachCar` at **0x004273e0** (return and clear the car).
  The scope and main checkout's in-flight `coaster8.c` use the latter name
  in an extern for 0x004273f0. Integration must reconcile that external name
  to `RouteSeat_ReleaseCar`; no existing C file was edited by this lane.
- `coaster8.c`'s seat update callback typedef takes only a seat. The body at
  0x00427410 demonstrably reads a **second transform argument**. This file
  supplies the complete definition; that existing typedef is unchanged.
- `TrackCurve_EvaluateOffset` and the derivative/position/up externs use an
  `int` for a float argument's **raw bits**, following the actual caller
  loads. The original values and the 32-bit cdecl argument layout agree.
- `Raster_RestoreState` takes the saved-surface pointer in this caller even
  though the existing empty leaf definition has no arguments. The extra
  cdecl argument is exactly what this original call site pushes.
- `g_cc_obj`/`g_cc_txt` are ModelImage views of the existing pointer/length
  globals. `g_spider_fx` and `g_spacetower_fx` are FXEntry-array views of
  symbols some existing files declare as `void*`. Existing declarations
  remain untouched. `g_stat_c_4dcbc8` keeps the existing statistics name.
- Historical aliases already in the tree include `Route_TotalAcceleration`
  for 0x0041dd70 and the potential-energy name for 0x0041e810's ring sum.
  Scope-specified definition names are retained and the behavior is stated
  above; no speculative renaming of other files was attempted.

## Measured C/codegen findings

- **A scalar packed-square comparison is the ride-loop shape.** The
  two-byte `memcmp` template introduced a `lea` of rider+0xc, shifting all
  later instructions (33/33/39/32 strict mismatches across Spider, Plane,
  Safari and Barrels). `rec->tile.key == rider->tile.key` closes all four,
  with unchanged state-machine source. The loop re-reads each key at the
  original sites; no volatile shim is needed.
- **Use a two-value ternary for the byte revolution reset.**
  `rand()%2 ? 4 : 3` and `rand()%2 ? 2 : 1` give the original `setne al`
  followed by 32-bit `add eax,3`/`inc eax`. `(rand()%2 != 0)+K` performed
  the add in AL, leaving one mismatch and a one-byte size error each.
- **The tower's four height clears must precede the four car flag clears
  in source.** This makes EBX the live zero register before any of the
  mask operations, closing 20 strict mismatches at unchanged 56i/175B.
- **The copter boarding timer's final 3D update is outside the zero-timer
  guard.** Writing the call directly inside the guard merges the preceding
  cdecl cleanup into `add esp,0xc`; writing the common tail after the join
  produces the original split `add esp,4` then `add esp,8` and VC6 duplicates
  the final call itself (18 mismatches -> zero, 295 -> 296 bytes).
- **Read each LLS frame count into one byte local before either record
  store.** A second `lls->frames` read after the first record store reloads
  because of aliasing (97 mismatches, truncated 427B). The named local
  gives the original single AL load, store, `dec al`, store across all five
  cars: 118i/426B exact.
- **A named speed local keeps the two travel calls' original argument
  cleanup.** `Route_TravelPerTick(route, Route_GetSpeed(route))` wrote the
  x87 return straight over the pending argument; storing a float local
  preserves the original dead-argument-slot spill/reload (12i/31B).
- **Name the image length before initializing the counter, then compute
  `end = p + length`.** This puts the pointer addition after the counter's
  zero initialization, closing the two scheduling differences in
  `ModelImage_FindRecord` (40i/74B).
- **The bit reader needs an int local AND a boolean branch.**
  `int flags = *(signed char*)&node->flags; if (flags & 1) return 1;
  return 0;` gives `movsx eax,byte [eax] / and eax,1` (4i/11B).
  A direct mask, an int local returned through `&1`, volatile, alternate
  integer types, ternaries and equivalent bit expressions narrowed to
  `mov al` (4i/10B). This confirms DECOMP's `Route_IsClosed` rule rather
  than introducing a new trick. The final body needs no volatile.
- **Normalized matches do not prove global identities.** A whole ViewRec
  assignment for `Coaster3D_SetCarClipDepth` initially passed 19i/105B,
  zero mismatches, but loaded a/b/c instead of b/c/d at three sites and
  assigned the global homes in a different order. Reversing the source
  operands of the first float sum, copying the direction as one Vec3f,
  storing angle, then storing a reproduces all 12 previously mismapped
  relocations. The original 0.0015875f travel constant was also read directly
  from 0x004ab404 and checked as raw IEEE-754 bits, not inferred from a
  nominal frame rate.

## Newly exposed symbols

Callee names below were introduced locally from the observed operation or,
for drawing passes, from their call order. Large bodies remain external;
these names do not claim a complete reconstruction of those bodies.

- `0x004b5cbc` — `g_view_car` (global).
- `0x00611650` — `g_view_car_angle` (global).
- `0x0082add0` — `g_route_node_model` (global).
- `0x004266b0` — `ClipRect_Link` (callee).
- `0x004266e0` — `ClipRect_Unlink` (callee).
- `0x00426700` — `ClipRect_SetBounds` (callee).
- `0x00422400` — `ModelImage_FindName` (callee).
- `0x0041e660` — `RouteNode_IsPending` (callee).
- `0x0041e670` — `RouteNode_CanAdd` (callee).
- `0x0041ea70` — `RouteNode_LinkPending` (callee).
- `0x0041e640` — `RouteNode_SetPending` (callee).
- `0x0041dca0` — `Route_GetSpeed` (callee).
- `0x00429c60` — `TrackCurve_EvaluateDerivative` (callee).
- `0x00429a80` — `TrackCurve_EvaluatePosition` (callee).
- `0x00429b90` — `TrackCurve_EvaluateUp` (callee).
- `0x0041e9e0` — `RouteNode_GetTransform` (callee).
- `0x00420fb0` — `CoasterModel_GetClipRect` (callee).
- `0x004dcbb8` — `g_model_light` (global).
- `0x004d8bb8` — `g_model_vertices` (global).
- `0x00426510` — `Mat3_TransposeToMat4` (callee).
- `0x004261c0` — `TransformVec3` (callee).
- `0x00420810` — `CoasterModel_DrawPass1` (callee).
- `0x00420a20` — `CoasterModel_DrawPass2` (callee).
- `0x00420c40` — `CoasterModel_DrawPass3` (callee).
- `0x004b4cac` — `g_safari_rider_name` (global).
- `0x004b4d94` — `g_spider_rider_name` (global).
- `0x004b79bc` — `g_plane_rider_name` (global).
- `0x004b78b4` — `g_sbarrel_rider_name` (global).
- `0x0043a940` — `SpaceTower_StepCar` (callee).
- `0x0043b810` — `SpaceTower_UpdateRiders` (callee).
- `0x00404860` — `Copters_StepCar` (callee).
- `0x00404630` — `Copters_UpdateCarRider` (callee).
- `0x004049a0` — `Copters_StopRide` (callee).
