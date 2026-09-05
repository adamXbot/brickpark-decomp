# Scope E — frontier micro-helpers

Initial implementation pass, 2026-09-05. Branch `scope/E`; base `f22f7cc7`.
Worktree: `.claude/worktrees/scope-e` (under the main checkout).

**147 functions reconstructed: 140 exact full-body matches and 7 WIPs.**
`tinystubs.c`: 55 exact / 1 WIP; `ridetiny.c`: 38 exact / 6 WIPs;
`coastertiny.c`: 47 exact / 0 WIPs. All three files compile without `/W3`
warnings using VC6 SP3 `/O2 /Gy /Gd`. All exact markers passed `audit.py`.
The seven WIPs remain below; they are not claimed as exact or exhausted.
No existing C file, shared documentation, tool, or progress report was edited.

## Verification

- Reference executable and toolchain are the existing ignored local resources,
  linked into the new worktree. No game binary or compiler file is included.
- `LEGOLAND_CL=/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl`
  with `/Users/systemadmin/.venvs/legoland/bin/python tools/audit.py
  LEGOLAND/tinystubs.c LEGOLAND/ridetiny.c LEGOLAND/coastertiny.c` is the
  authoritative check. It prints 140 `[OK]` lines and seven `[WIP]` lines,
  and ends `PASS: 0 function(s) failed the extent gate`.
- Each file also passed a separate `/W3 /O2 /Gy /Gd` compile, with objects
  under `/tmp/se_*`. Iteration used scratch files in `/tmp`, never `tools/`.
- The table percentages are strict normalized instruction comparisons over
  the audit span. For WIPs, padding or an escaped branch can make that span
  differ from the true compiled body; the residual section records both.
- The branch is for integration by the owning session. Global verification,
  coverage and report regeneration were intentionally left to that session,
  following `docs/PARALLEL_CONTRACT.md`.

## Names recovered

Existing placeholder externs in other scopes were not edited. The following
names replace their placeholder at the listed address; integration can use
this mapping without changing those callers' prototype types.

| Address | Definition | Naming evidence |
| --- | --- | --- |
| 0x00498cf0 | `IsNarrationPlaying` | Playback state 3 is the state queried by the help-key and screen code. |
| 0x00422640 | `CoasterModel_GetPartCount` | Returns the count for the part-name table at 0x004dd86c. |
| 0x004225d0 | `CoasterModel_GetMeshCount` | Returns the mesh-name table count at 0x004dd868. |
| 0x0041e7e0 | `RouteNode_GetAcceleration` | Returns 0.1f, summed by the route acceleration helper and used in the node pitch calculation. |
| 0x00426740 | `Raster_ResetClipRing` | Resets both links to the renderer clip-ring sentinel. |
| 0x00423730 | `Raster_RestoreFloatMode` | Loads the saved x87 control word supplied by value. |
| 0x00421cc0 | `TrackCurve_CubicUpVector` | The cubic curve operation returns the constant (0,0,1). |
| 0x00421a90 | `TrackCurve_LineUpVector` | The line curve operation returns the constant (0,0,1). |
| 0x004207a0 | `CoasterModel_LoadPalette` | Loads Rollercoaster.lpt through the shared model-file loader. |
| 0x0041e330 | `Route_ForEachNode` | Visits the +0x70 embedded head, then every +0xe8 next link until it returns to that head. |
| 0x0041e400 | `Route_ClearNodeActiveFlags` | Applies the node callback that clears flag bit 0. |
| 0x0041e3a0 | `Route_UpdateClipRects` | Applies the callback that projects node bounds and updates its clip rectangle. |
| 0x0041e380 | `Route_UnlinkClipRects` | Applies the callback that unlinks the rectangle at node +0xc8. |
| 0x0041e360 | `Route_LinkClipRects` | Applies the callback that links the rectangle at node +0xc8. |
| 0x00424890 | `Coaster_StepFreeRoute` | Forwards route and time step to Route_StepFree. |
| 0x00420530 | `CoasterModel_SetDirectory` | Calls the imported SetCurrentDirectoryA for a non-null path and returns its result. |
| 0x0041e240 | `Route_UpdateTimer` | Refreshes the timer and clears flag 0x40 after the deadline. |
| 0x0042a620 | `TrackCursor_Init` | Copies the 20-byte route position and stores its float parameter. |
| 0x004294b0 | `Coaster_BuildSupportVertices` | Scales eight support vertices by 2.5 on x and 8.2 on y, copying z bits. |
| 0x0042a640 | `TrackCursor_Evaluate` | Selects a mode and evaluates the cursor with an offset of 4.8f. |

## Mechanics recovered

- **UI and scripts.** Default icon input returns 2 for flag bit 0 or 2,
  otherwise 1. The primary-icon handler acts on bit 1 and returns 1.
  Cursor validity is a signed positive value; error codes compete by their
  negative value, with the most negative error retained. Info state 2 protects
  the current selection from reset. Script list destruction is recursive,
  freeing the suffix before the current record. `SkipMeasuredBlock` consumes
  one four-byte word; it does not itself skip a payload.
- **Visitors and timing.** `HasBlokeStayedTooLong` compares the signed
  `stay - allowance / 2` against twice the park metric. The entrance tile is
  refreshed when the signed timer difference is strictly greater than 4000,
  or when forced, then used to refresh entrance-connected path squares.
- **Audio and render storage.** Pan is four times the offset, clamped to
  [-10000,10000]. Theme transitions accept states 1 and 2, enqueue command 3
  with the signed `theme % 5`, and signal the event. Narration rewind seeks
  to its data offset and resets the remaining-byte count. Both render lists
  bump their arenas by 16 bytes and increment a separate item count. Report
  buffers are freed through the first string-table entry when the associated
  count is nonzero; only the count is cleared.
- **Ride records.** The nine free-all helpers repeatedly remove the live
  head. The seven machine ticks read each record's `next` link after its step
  callback. The five record searches compare the packed 16-bit square, return
  the first matching record, and return null on failure. Their link offsets
  differ: copters +4, safari +0x10, spider +0x2c, gold rush/earth slide +0xc.
- **Boarding.** Barrels and spider transfer the seated count to the rider
  count, clear the seated count, clear flag 0x4000 and set flag 1. Spider also
  clears its frame and cycle. Tower copies the seated count, sets flag 1 and
  clears the cycle, then rebuilds car seating and starts sound. The release
  helpers fade map-square audio by -200. Gold-rush pan timers use `rand()%31+15`.
- **Queues and paths.** Earth-slide pop unlinks the head, clears the tail
  if it was that node, and decrements the byte count without freeing the node.
  A missing rider's ordinal lookup returns the list length. A log-flume queue
  is ready when the front bloke's signed path index equals the path's first
  dword minus one. Reverse path start stores the path, sets index to the
  low-word count minus one, writes walking direction -1 and increments action.
  `WalkPath_IndexOf` actually returns the path pointer at bloke +0x50.
- **Coaster.** The route visitor includes the embedded head, so even a ring
  with no extra nodes invokes the callback once. Waiting-car count excludes
  the car-list sentinel and counts state 1 only. Seat attach and detach both
  return the affected car. The car pointer pool grows by the requested count;
  its free operation ignores the pointer and merely subtracts the count.
  The station-end test requires the fixed station geometry and `t > 0.099f`.
  The acceleration distance is `totalAcceleration * time * time * 0.5f`.
- **Coaster rendering.** Shade entries occupy 128-byte slices. Eight support
  vertices are scaled (2.5,8.2,unchanged z). Track cursor initialization copies
  20 bytes of route position then the float parameter; evaluation uses a mode
  lookup and 4.8f. The two corner getters use the low words of the class rect:
  first `(x1+x, y0+y-2)`, second `(x0+x-2, y1+y)`. The float-mode restore and
  reciprocal-square-root dispatch reproduce the original x87 assembly.

## Original quirks retained

- `BoatingSchool_CountWater` **assigns** the supplied 16-bit square to every
  boat and counts nonzero assignments; it is not a read-only count. It rereads
  the supplied square on every iteration, preserving aliasing with a boat.
- `Copters_StepRider` scans **six** words from 0x004c1124 to 0x004c113c,
  although `mechrides.c` declares five paths. Its sixth entry overlaps the
  following ride sprite pointer. The function replaces the bloke's path
  pointer with an ordinal or -1; it does not advance the rider simulation.
- Keyboard teardown releases the COM device without clearing its global.
- Map-square sound sources leave the unused bloke member unwritten.
- Array and list accessors keep the original unchecked indices and sentinels.
  Reverse start on a zero-length path produces index -1.

## New symbols and prototype differences

- Newly named callees: `ClosePrimaryPopUp` 0x00473160 (state-dependent popup
  close/advance); `SetMenuHelp` 0x00475fe0 (bounded four-slot setter);
  `DetailImage_AllocSlot` 0x00496f30 (registration slot allocator);
  `ResolveEntrancePathSquare` 0x00482a40 (clear/rebuild path-square connectivity).
- Ride machine callees are `Copters_StepMachine` 0x00404a90,
  `SafariRide_StepMachine` 0x004150c0, `SpiderRide_StepMachine` 0x004161f0,
  `SpaceTower_StepMachine` 0x0043b990, `SpinningBarrels_StepMachine` 0x0043c7f0,
  `PlaneRide_StepMachine` 0x0043e2b0, `EarthSlide_StepMachine` 0x0042d560.
  The square-sound starters are `SpiderRide_StartSound` 0x004159e0 and
  `SpaceTower_StartSound` 0x0043aa10; `GoldRush_HasFreePan` is 0x00406e90.
- Coaster callees: `CoasterModel_FindPartIndex` 0x00422600,
  `CoasterModel_FindMeshIndex` 0x00422590, `RouteNode_LinkClipRect` 0x0041e950,
  `RouteNode_UnlinkClipRect` 0x0041e970, `RouteNode_UpdateClipRect` 0x0041e990,
  `RouteNode_ClearActive` 0x0041e630, `RouteNode_AddPending` 0x0041eaf0,
  `Route_TotalAcceleration` 0x0041dd70, `PhysObj_ReadState` 0x004203d0,
  `PhysObj_WriteState` 0x004203f0, `TrackCurve_EvaluateOffset` 0x00429bb0,
  `Coaster3D_SetCarClipDepth` 0x00425c40. Every callee declaration carries its VA.
- Newly described globals include `g_entrance_tile_time` 0x0066b468,
  `g_visitor_limit` 0x0083291c, `g_fullscreen_clip` 0x007fe020,
  advisor pose/argument/timer at 0x00665fec/0x0081c09c/0x0081c088,
  narration offset/remaining/total at 0x007cacb4/0x007cacac/0x0079ac04,
  `g_car_pool_used` 0x004dd5d8, support model/texture at 0x004b6300/0x004b62f0,
  support template/result at 0x004b61e0/0x00614858 and cursor modes at 0x004b6408.
- `Raster_RestoreFloatMode` takes the saved word by value. The existing
  `schoolcar.c` placeholder declares it as `void*`, but `fldcw [ebp+8]`
  demonstrates there is no dereference of a pointed-to word.
- `RouteSeat_AttachCar`, `RouteSeat_DetachCar` and `RegisterDetailImage`
  return values that some callers declare as `void`. `PhysObj_Init` hardcodes
  dimension 2 and does not read the extra dimension argument in its callers.
  `GetCoasterModelSize` retrieves the auxiliary loaded-file pointer, consistent
  with `g_coaster_tab_b2`, despite its old name.
- `g_park_metric` is signed in this view for the original signed comparison.
  `GetRoadRecord` uses unsigned arguments here to preserve the logical shifts.
  These caller-side types were not propagated into other scopes.

## Code-generation evidence and residuals

- **Null guard before a returned Win32 result.** `CoasterModel_SetDirectory`
  as `void` produced 6i/16B with a shared final return. `if (!path) return 0;
  return SetCurrentDirectoryA(path);` gives the original `jne/ret` split,
  7i/17B, and passes the audit. This also recovers its meaningful return type.
- **Raw float transfer versus arithmetic.** Whole `RoutePos` assignment plus
  `cursor->t = t` gives exactly the original five-dword copy and integer
  transfer of the float argument, 12i/29B; no bit-casting trick is required.
  Support z is explicitly copied as bits, matching the original integer load
  and store amid x87 scaling (13i/59B).
- **Original x87 calling convention.** The reciprocal-square-root target
  consumes and returns ST(0). An inline-assembly call followed by rounding
  through the argument's float home gives 8i/20B. `fldcw control` gives the
  float-mode restore's 5i/8B with its EBP frame; both are audit `[OK]`.
- **Five searches, one residual.** All are now 16i/40B, with exact control
  flow and four strict mismatches at indices 4,5,10,11: the compiled code loads
  the query key into dx then compares the record; the original loads the
  record key into dx then compares the query. Equality behavior is unchanged.
  A free volatile query read prevents hoisting and recovers the peeled loop;
  ordinary searches give 13i/32B. Early return/break/nested-loop forms,
  named scalar and aggregate temporaries, intrinsic two-byte copies and inline
  equality helpers did not remove the operand-order residual. Kept WIP at 75%.
- **Copter save ordinal.** Original 16i/42B; current full body 19i/46B.
  The audit sees 16i/41B, eight strict differences beginning at 0, and ESCAPES
  into a duplicated success store. The counter and array cursor take ecx/eax
  instead of eax/ecx. Free volatile reads, naming the path/array element,
  integer field views, separate result, and inline index helpers did not
  recover the shared final store. Kept WIP at 50% of the audit span, not a
  claim that the truncated body is complete.
- **Default icon input.** Original 10i/20B; current body 9i/18B. VC6 replaces
  the final `mov al,2 / jne / mov al,1` with `setne al / inc eax`; the first
  mismatch is index 6. The audit includes one padding nop and compares
  10i/19B, with four mismatches (60%). Char locals, signed/unsigned parameters
  and returns, ternaries and masked switches left this residual. Kept WIP.

## Per-function results

`FUNCTION` means audit `[OK]`; `WIP-FUNCTION` is explicitly not `[OK]`.
Instruction and byte counts below are the original extent. See above for
compiled sizes and first diverging indices of the seven partials.

### tinystubs.c

| Address | Name | Insns / bytes | Strict match | Audit OK | Marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0048b6c0 | `FreePlayInit_48b6c0` | 1 / 1 | 100.0% | yes | FUNCTION |
| 0x00476020 | `RenderIconsHook` | 1 / 1 | 100.0% | yes | FUNCTION |
| 0x00472090 | `DrawPopUpEnd` | 1 / 1 | 100.0% | yes | FUNCTION |
| 0x0045b170 | `RenderViewCellProbe` | 1 / 1 | 100.0% | yes | FUNCTION |
| 0x00453cd0 | `DebugErrorSink` | 1 / 1 | 100.0% | yes | FUNCTION |
| 0x00499560 | `GetMechanicCount` | 2 / 6 | 100.0% | yes | FUNCTION |
| 0x00499550 | `GetGardenerCount` | 2 / 6 | 100.0% | yes | FUNCTION |
| 0x00482b00 | `GetEntranceTile` | 2 / 6 | 100.0% | yes | FUNCTION |
| 0x004735b0 | `ResetHelpKeyCursor` | 2 / 11 | 100.0% | yes | FUNCTION |
| 0x004700f0 | `GetSelectedBloke` | 2 / 6 | 100.0% | yes | FUNCTION |
| 0x0046d3a0 | `SetHelpFaceTalking` | 2 / 11 | 100.0% | yes | FUNCTION |
| 0x0044ea40 | `GetVisitorLimit` | 2 / 6 | 100.0% | yes | FUNCTION |
| 0x00490600 | `ShowInfoPanel` | 3 / 10 | 100.0% | yes | FUNCTION |
| 0x00468d00 | `ResetScriptTimer` | 3 / 11 | 100.0% | yes | FUNCTION |
| 0x0045f460 | `ResetCursorFootprint` | 4 / 25 | 100.0% | yes | FUNCTION |
| 0x00498cf0 | `IsNarrationPlaying` | 5 / 15 | 100.0% | yes | FUNCTION |
| 0x00474820 | `InGamePrimaryIcon` | 5 / 15 | 100.0% | yes | FUNCTION |
| 0x0046df60 | `RenderFullScreenIcon` | 5 / 16 | 100.0% | yes | FUNCTION |
| 0x0046d440 | `LinkIcon` | 5 / 18 | 100.0% | yes | FUNCTION |
| 0x00457890 | `BricksAreLimited` | 5 / 14 | 100.0% | yes | FUNCTION |
| 0x00471d40 | `CloseInfoPopUp` | 6 / 25 | 100.0% | yes | FUNCTION |
| 0x00471bf0 | `ResetInfoSelection` | 6 / 22 | 100.0% | yes | FUNCTION |
| 0x0045f4b0 | `CursorIsValid` | 6 / 18 | 100.0% | yes | FUNCTION |
| 0x00444070 | `SetAdvisorPose` | 6 / 30 | 100.0% | yes | FUNCTION |
| 0x00499760 | `SetOrderRepairAmount` | 7 / 25 | 100.0% | yes | FUNCTION |
| 0x0047d7e0 | `SkipMeasuredBlock` | 7 / 17 | 100.0% | yes | FUNCTION |
| 0x00473a50 | `KillKeyboardDevice` | 7 / 16 | 100.0% | yes | FUNCTION |
| 0x00474970 | `LoadIconStateChunk` | 8 / 22 | 100.0% | yes | FUNCTION |
| 0x00473660 | `ProcessHelpKeys` | 8 / 24 | 100.0% | yes | FUNCTION |
| 0x00443120 | `RenderItem2_Alloc` | 8 / 31 | 100.0% | yes | FUNCTION |
| 0x00442f50 | `RenderItem_Alloc` | 8 / 31 | 100.0% | yes | FUNCTION |
| 0x0043f970 | `FixUpLocSetPointers` | 8 / 21 | 100.0% | yes | FUNCTION |
| 0x00490880 | `FreeReportHintBuffer` | 9 / 34 | 100.0% | yes | FUNCTION |
| 0x00490850 | `FreeHelpTextBuffer` | 9 / 34 | 100.0% | yes | FUNCTION |
| 0x00498120 | `RewindNarrationSource` | 10 / 36 | 100.0% | yes | FUNCTION |
| 0x00496570 | `PanFromOffset` | 10 / 33 | 100.0% | yes | FUNCTION |
| 0x0048fc00 | `ProcessScreenPopup` | 10 / 33 | 100.0% | yes | FUNCTION |
| 0x00471470 | `DisablePopUpInputs` | 10 / 39 | 100.0% | yes | FUNCTION |
| 0x0046f2e0 | `DefaultIconInput` | 10 / 20 | 60.0% | no | WIP-FUNCTION |
| 0x0045eab0 | `ClassAllowsObjects` | 10 / 23 | 100.0% | yes | FUNCTION |
| 0x0045cb90 | `ResetPathTile` | 10 / 34 | 100.0% | yes | FUNCTION |
| 0x00485f00 | `PrintSpriteXY` | 11 / 28 | 100.0% | yes | FUNCTION |
| 0x00481ee0 | `ClearPathSquareVisited` | 11 / 29 | 100.0% | yes | FUNCTION |
| 0x00476000 | `ClearMenuHelp` | 11 / 22 | 100.0% | yes | FUNCTION |
| 0x004714e0 | `ClearNewObjectMarkers` | 11 / 33 | 100.0% | yes | FUNCTION |
| 0x00496fc0 | `RegisterDetailImage` | 12 / 35 | 100.0% | yes | FUNCTION |
| 0x00492850 | `ResumePausedSamples` | 12 / 28 | 100.0% | yes | FUNCTION |
| 0x00492830 | `InitOptionSamples` | 12 / 28 | 100.0% | yes | FUNCTION |
| 0x0045f480 | `SetCursorError` | 12 / 37 | 100.0% | yes | FUNCTION |
| 0x0045ead0 | `IsBuildableClass` | 12 / 32 | 100.0% | yes | FUNCTION |
| 0x00455ec0 | `PrintCachedEntry` | 12 / 31 | 100.0% | yes | FUNCTION |
| 0x0044eab0 | `HasBlokeStayedTooLong` | 14 / 38 | 100.0% | yes | FUNCTION |
| 0x00492ca0 | `SetThemeInTransition` | 15 / 57 | 100.0% | yes | FUNCTION |
| 0x00482b20 | `RefreshEntranceTile` | 15 / 53 | 100.0% | yes | FUNCTION |
| 0x0046b560 | `FreeScriptStepList` | 15 / 35 | 100.0% | yes | FUNCTION |
| 0x00468970 | `FreeScriptEventList` | 15 / 35 | 100.0% | yes | FUNCTION |

### ridetiny.c

| Address | Name | Insns / bytes | Strict match | Audit OK | Marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x004122f0 | `WalkPath_IndexOf` | 3 / 8 | 100.0% | yes | FUNCTION |
| 0x00496d10 | `SetSampleLooping` | 3 / 9 | 100.0% | yes | FUNCTION |
| 0x004048a0 | `Copters_ResumeSFX` | 6 / 16 | 100.0% | yes | FUNCTION |
| 0x00411e90 | `LFQueue_HasRider` | 6 / 15 | 100.0% | yes | FUNCTION |
| 0x0043c320 | `SpinningBarrels_SetFull` | 9 / 27 | 100.0% | yes | FUNCTION |
| 0x00407230 | `GoldRush_RollPanTimer` | 9 / 27 | 100.0% | yes | FUNCTION |
| 0x0043d940 | `PlaneRide_FreeAllRecords` | 10 / 28 | 100.0% | yes | FUNCTION |
| 0x0043c4d0 | `SpinningBarrels_FreeAllRecords` | 10 / 28 | 100.0% | yes | FUNCTION |
| 0x0043ac20 | `SpaceTower_FreeAllRecords` | 10 / 28 | 100.0% | yes | FUNCTION |
| 0x00415990 | `SpiderRide_FreeAllRecords` | 10 / 28 | 100.0% | yes | FUNCTION |
| 0x00414a60 | `SafariRide_FreeAllRecords` | 10 / 28 | 100.0% | yes | FUNCTION |
| 0x004069c0 | `GoldRush_FreeAllRecords` | 10 / 28 | 100.0% | yes | FUNCTION |
| 0x00403ce0 | `Copters_FreeAllRecords` | 10 / 28 | 100.0% | yes | FUNCTION |
| 0x0042bc40 | `Carousel_FreeRecords` | 10 / 28 | 100.0% | yes | FUNCTION |
| 0x0042a9f0 | `Balloonz_FreeRecords` | 10 / 28 | 100.0% | yes | FUNCTION |
| 0x004122a0 | `LFPath_StartReverse` | 11 / 33 | 100.0% | yes | FUNCTION |
| 0x00452a80 | `PowerStation_InitSound` | 11 / 33 | 100.0% | yes | FUNCTION |
| 0x00452990 | `Fountain_InitSound` | 11 / 33 | 100.0% | yes | FUNCTION |
| 0x0043e3f0 | `PlaneRide_TickMachine` | 12 / 29 | 100.0% | yes | FUNCTION |
| 0x0043c930 | `SpinningBarrels_TickMachine` | 12 / 28 | 100.0% | yes | FUNCTION |
| 0x0043baa0 | `SpaceTower_TickMachine` | 12 / 29 | 100.0% | yes | FUNCTION |
| 0x00416310 | `SpiderRide_TickMachine` | 12 / 29 | 100.0% | yes | FUNCTION |
| 0x00415200 | `SafariRide_TickMachine` | 12 / 29 | 100.0% | yes | FUNCTION |
| 0x00404bc0 | `Copters_TickMachine` | 12 / 29 | 100.0% | yes | FUNCTION |
| 0x0042d5f0 | `EarthSlide_TickInstances` | 12 / 29 | 100.0% | yes | FUNCTION |
| 0x0042d3e0 | `NthRiderNodeIndex` | 12 / 26 | 100.0% | yes | FUNCTION |
| 0x0042d040 | `EarthSlide_PopQueue` | 12 / 34 | 100.0% | yes | FUNCTION |
| 0x0042cf40 | `EarthSlide_IsFrontOfQueue` | 13 / 34 | 100.0% | yes | FUNCTION |
| 0x0043aa90 | `SpaceTower_SetFull` | 15 / 44 | 100.0% | yes | FUNCTION |
| 0x00415a60 | `SpiderRide_SetFull` | 15 / 41 | 100.0% | yes | FUNCTION |
| 0x0043aa50 | `SpaceTower_ReleaseSquare` | 15 / 51 | 100.0% | yes | FUNCTION |
| 0x00415a20 | `SpiderRide_ReleaseSquare` | 15 / 51 | 100.0% | yes | FUNCTION |
| 0x004159b0 | `SpiderRide_FindRecord` | 16 / 40 | 75.0% | no | WIP-FUNCTION |
| 0x00414a80 | `SafariRide_FindRecord` | 16 / 40 | 75.0% | no | WIP-FUNCTION |
| 0x00403d00 | `Copters_FindRecord` | 16 / 40 | 75.0% | no | WIP-FUNCTION |
| 0x004069e0 | `GoldRush_FindRecord` | 16 / 40 | 75.0% | no | WIP-FUNCTION |
| 0x0042ce20 | `EarthSlide_FindRec` | 16 / 40 | 75.0% | no | WIP-FUNCTION |
| 0x00406f00 | `GoldRush_ReleasePan` | 15 / 41 | 100.0% | yes | FUNCTION |
| 0x00403d30 | `Copters_StepRider` | 16 / 42 | 50.0% | no | WIP-FUNCTION |
| 0x004192d0 | `BoatingSchool_CountWater` | 16 / 41 | 100.0% | yes | FUNCTION |
| 0x004139e0 | `Road_TileRelease` | 16 / 41 | 100.0% | yes | FUNCTION |
| 0x00406f30 | `GoldRush_UpdateFullFlag` | 16 / 39 | 100.0% | yes | FUNCTION |
| 0x00411ea0 | `LFQueue_FrontIsReady` | 16 / 39 | 100.0% | yes | FUNCTION |
| 0x004112c0 | `LFQuadBottomLeft` | 16 / 42 | 100.0% | yes | FUNCTION |

### coastertiny.c

| Address | Name | Insns / bytes | Strict match | Audit OK | Marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00424e60 | `Coaster_OnCircuitClosed` | 1 / 1 | 100.0% | yes | FUNCTION |
| 0x00423790 | `Raster_RestoreState` | 1 / 1 | 100.0% | yes | FUNCTION |
| 0x00422640 | `CoasterModel_GetPartCount` | 2 / 6 | 100.0% | yes | FUNCTION |
| 0x004225d0 | `CoasterModel_GetMeshCount` | 2 / 6 | 100.0% | yes | FUNCTION |
| 0x004207c0 | `GetCoasterColours` | 2 / 6 | 100.0% | yes | FUNCTION |
| 0x0041e7e0 | `RouteNode_GetAcceleration` | 2 / 7 | 100.0% | yes | FUNCTION |
| 0x004273e0 | `RouteSeat_DetachCar` | 4 / 15 | 100.0% | yes | FUNCTION |
| 0x004273d0 | `RouteSeat_AttachCar` | 4 / 12 | 100.0% | yes | FUNCTION |
| 0x00426740 | `Raster_ResetClipRing` | 4 / 16 | 100.0% | yes | FUNCTION |
| 0x0041cc90 | `JointBitFromIndex` | 4 / 12 | 100.0% | yes | FUNCTION |
| 0x00423730 | `Raster_RestoreFloatMode` | 5 / 8 | 100.0% | yes | FUNCTION |
| 0x00421cc0 | `TrackCurve_CubicUpVector` | 5 / 25 | 100.0% | yes | FUNCTION |
| 0x00421a90 | `TrackCurve_LineUpVector` | 5 / 25 | 100.0% | yes | FUNCTION |
| 0x00421510 | `CarPool_Free` | 5 / 19 | 100.0% | yes | FUNCTION |
| 0x004207a0 | `CoasterModel_LoadPalette` | 5 / 16 | 100.0% | yes | FUNCTION |
| 0x00420790 | `FindCoasterPart` | 5 / 14 | 100.0% | yes | FUNCTION |
| 0x0041ec00 | `CastleClassDef` | 5 / 18 | 100.0% | yes | FUNCTION |
| 0x0041eb60 | `RouteNode_Free` | 5 / 12 | 100.0% | yes | FUNCTION |
| 0x0041d430 | `TrackLinkNodes` | 5 / 15 | 100.0% | yes | FUNCTION |
| 0x0041e330 | `Route_ForEachNode` | 17 / 36 | 100.0% | yes | FUNCTION |
| 0x0041e400 | `Route_ClearNodeActiveFlags` | 6 / 19 | 100.0% | yes | FUNCTION |
| 0x0041e3a0 | `Route_UpdateClipRects` | 6 / 19 | 100.0% | yes | FUNCTION |
| 0x0041e380 | `Route_UnlinkClipRects` | 6 / 19 | 100.0% | yes | FUNCTION |
| 0x0041e360 | `Route_LinkClipRects` | 6 / 19 | 100.0% | yes | FUNCTION |
| 0x00424890 | `Coaster_StepFreeRoute` | 7 / 19 | 100.0% | yes | FUNCTION |
| 0x004214f0 | `CarPool_Alloc` | 7 / 26 | 100.0% | yes | FUNCTION |
| 0x00420530 | `CoasterModel_SetDirectory` | 7 / 17 | 100.0% | yes | FUNCTION |
| 0x0041e3c0 | `RouteAddNode_Cb` | 7 / 20 | 100.0% | yes | FUNCTION |
| 0x0041e240 | `Route_UpdateTimer` | 7 / 21 | 100.0% | yes | FUNCTION |
| 0x00426960 | `VecMath_ReciprocalSqrt` | 8 / 20 | 100.0% | yes | FUNCTION |
| 0x0041ddb0 | `Route_AccelDistance` | 8 / 28 | 100.0% | yes | FUNCTION |
| 0x00429490 | `DrawSupportModel` | 10 / 31 | 100.0% | yes | FUNCTION |
| 0x00420730 | `GetCoasterModelSize` | 10 / 29 | 100.0% | yes | FUNCTION |
| 0x00420710 | `LoadCoasterMeshTex` | 10 / 29 | 100.0% | yes | FUNCTION |
| 0x004206b0 | `LoadCoasterMesh` | 10 / 29 | 100.0% | yes | FUNCTION |
| 0x00420410 | `PhysObj_Init` | 11 / 34 | 100.0% | yes | FUNCTION |
| 0x0042a620 | `TrackCursor_Init` | 12 / 29 | 100.0% | yes | FUNCTION |
| 0x00426e80 | `WriteCoasterNodeRef` | 12 / 31 | 100.0% | yes | FUNCTION |
| 0x00424960 | `Route_AtStationEnd` | 12 / 38 | 100.0% | yes | FUNCTION |
| 0x00422e10 | `Shade_BuildEntry` | 12 / 39 | 100.0% | yes | FUNCTION |
| 0x004294b0 | `Coaster_BuildSupportVertices` | 13 / 59 | 100.0% | yes | FUNCTION |
| 0x00424c10 | `Coaster_CountWaitingCars` | 13 / 36 | 100.0% | yes | FUNCTION |
| 0x0042a640 | `TrackCursor_Evaluate` | 14 / 42 | 100.0% | yes | FUNCTION |
| 0x004239e0 | `Castle_GetSecondCorner` | 14 / 43 | 100.0% | yes | FUNCTION |
| 0x004239b0 | `Castle_GetFirstCorner` | 14 / 44 | 100.0% | yes | FUNCTION |
| 0x00421560 | `CoasterCar_Draw` | 14 / 37 | 100.0% | yes | FUNCTION |
| 0x0041ceb0 | `TrackNode_PlaceObject` | 14 / 47 | 100.0% | yes | FUNCTION |
