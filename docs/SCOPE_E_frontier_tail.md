# Scope E — the last of the frontier (2026-09-05)

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here.** Branch: `scope/E`. Notes: `docs/lanes/scope-e.md`. Object prefix:
`/tmp/se_`.

This is a NEW-FUNCTION scope: about 145 functions of 1–17 instructions, the
remainder of the unmatched-callee frontier once the scopes already running are
excluded. Bodies this small close on the first or second compile almost every
time; breadth beats depth. **Naming the `Sub_*`/`sub_*` from what they do is
part of the deliverable.**

## Your files (create these; do not edit any existing `.c`)

### `LEGOLAND/ridetiny.c` — ride-class micro-helpers (≈50 functions)

One shape per verb across the ride classes — group by verb, build one body,
diff siblings. `ridemisc3.c` (the eight-wide unlink), `bswater2.c`,
`bswater3.c` (`SpaceTower_CountSeated`, `SpaceTower_RemoveRecord`),
`ridemisc.c`, `ridemisc2.c`, `mechrides.c`, `goldrush.c`, `ridecb1.c`,
`ridecb5.c`, `ridecb8.c` are the templates.

| address | name | insns |
| --- | --- | --- |
| 0x004159b0 | `SpiderRide_FindRecord` | 16 |
| 0x00414a80 | `SafariRide_FindRecord` | 16 |
| 0x00403d00 | `Copters_FindRecord` | 16 |
| 0x004069e0 | `GoldRush_FindRecord` | 16 |
| 0x0042ce20 | `EarthSlide_FindRec` | 16 |
| 0x00403d30 | `Copters_StepRider` | 16 |
| 0x004192d0 | `BoatingSchool_CountWater` | 16 |
| 0x004139e0 | `Road_TileRelease` | 16 |
| 0x00406f30 | `GoldRush_UpdateFullFlag` | 16 |
| 0x0043aa90 | `SpaceTower_SetFull` | 15 |
| 0x00415a60 | `SpiderRide_SetFull` | 15 |
| 0x0043c320 | `SpinningBarrels_SetFull` | 9 |
| 0x0043aa50 | `SpaceTower_ReleaseSquare` | 15 |
| 0x00415a20 | `SpiderRide_ReleaseSquare` | 15 |
| 0x00406f00 | `GoldRush_ReleasePan` | 15 |
| 0x0042cf40 | `EarthSlide_IsFrontOfQueue` | 13 |
| 0x0043e3f0 | `PlaneRide_TickMachine` | 12 |
| 0x0043c930 | `SpinningBarrels_TickMachine` | 12 |
| 0x0043baa0 | `SpaceTower_TickMachine` | 12 |
| 0x00416310 | `SpiderRide_TickMachine` | 12 |
| 0x00415200 | `SafariRide_TickMachine` | 12 |
| 0x00404bc0 | `Copters_TickMachine` | 12 |
| 0x0042d5f0 | `EarthSlide_TickInstances` | 12 |
| 0x0042d3e0 | `NthRiderNodeIndex` | 12 |
| 0x0042d040 | `EarthSlide_PopQueue` | 12 |
| 0x0043d940 | `PlaneRide_FreeAllRecords` | 10 |
| 0x0043c4d0 | `SpinningBarrels_FreeAllRecords` | 10 |
| 0x0043ac20 | `SpaceTower_FreeAllRecords` | 10 |
| 0x00415990 | `SpiderRide_FreeAllRecords` | 10 |
| 0x00414a60 | `SafariRide_FreeAllRecords` | 10 |
| 0x004069c0 | `GoldRush_FreeAllRecords` | 10 |
| 0x00403ce0 | `Copters_FreeAllRecords` | 10 |
| 0x0042bc40 | `Carousel_FreeRecords` | 10 |
| 0x0042a9f0 | `Balloonz_FreeRecords` | 10 |
| 0x00407230 | `GoldRush_RollPanTimer` | 9 |
| 0x004048a0 | `Copters_ResumeSFX` | 6 |
| 0x00411ea0 | `LFQueue_FrontIsReady` | 16 |
| 0x004112c0 | `LFQuadBottomLeft` | 16 |
| 0x00411e90 | `LFQueue_HasRider` | 6 |
| 0x004122a0 | `LFPath_StartReverse` | 11 |
| 0x004122f0 | `WalkPath_IndexOf` | 3 |
| 0x00452a80 | `PowerStation_InitSound` | 11 |
| 0x00452990 | `Fountain_InitSound` | 11 |
| 0x00496d10 | `SetSampleLooping` | 3 |

### `LEGOLAND/coastertiny.c` — coaster micro-subs (≈45 functions)

`schoolcar8.c` (53 named micro-subs, all exact — your closest template),
`coastermath.c`, `coaster5.c`, `coaster6.c`, `schoolcar5.c`..`schoolcar7.c`.
Name every `Sub_*` (the `PhysVec_`, `Route_`, `RouteSeat_`, `TrackCurve_`,
`Coaster_`, `CoasterCar_` prefixes are established).

| address | name | insns |
| --- | --- | --- |
| 0x0041e330 | (declared as `void` in schoolcar.c — a mis-parsed extern; disassemble and name it) | 17 |
| 0x0042a640 | `Sub_42a640` | 14 |
| 0x0042a620 | `Sub_42a620` | 12 |
| 0x004239e0 | `Castle_GetSecondCorner` | 14 |
| 0x004239b0 | `Castle_GetFirstCorner` | 14 |
| 0x00421560 | `CoasterCar_Draw` | 14 |
| 0x0041ceb0 | `TrackNode_PlaceObject` | 14 |
| 0x004294b0 | `Sub_4294b0` | 13 |
| 0x00424c10 | `Coaster_CountWaitingCars` | 13 |
| 0x00426e80 | `WriteCoasterNodeRef` | 12 |
| 0x00424960 | `Route_AtStationEnd` | 12 |
| 0x00422e10 | `Shade_BuildEntry` | 12 |
| 0x00420410 | `PhysObj_Init` | 11 |
| 0x00429490 | `DrawSupportModel` | 10 |
| 0x00420730 | `GetCoasterModelSize` | 10 |
| 0x00420710 | `LoadCoasterMeshTex` | 10 |
| 0x004206b0 | `LoadCoasterMesh` | 10 |
| 0x00426960 | `VecMath_ReciprocalSqrt` | 8 |
| 0x0041ddb0 | `Route_AccelDistance` | 8 |
| 0x00424890 | `Sub_424890` | 7 |
| 0x004214f0 | `CarPool_Alloc` | 7 |
| 0x00420530 | `Sub_420530` | 7 |
| 0x0041e3c0 | `RouteAddNode_Cb` | 7 |
| 0x0041e240 | `Sub_41e240` | 7 |
| 0x0041e400 | `Sub_41e400` | 6 |
| 0x0041e3a0 | `Sub_41e3a0` | 6 |
| 0x0041e380 | `Sub_41e380` | 6 |
| 0x0041e360 | `Sub_41e360` | 6 |
| 0x00423730 | `Sub_423730` | 5 |
| 0x00421cc0 | `Sub_421cc0` | 5 |
| 0x00421a90 | `Sub_421a90` | 5 |
| 0x00421510 | `CarPool_Free` | 5 |
| 0x004207a0 | `Sub_4207a0` | 5 |
| 0x00420790 | `FindCoasterPart` | 5 |
| 0x0041ec00 | `CastleClassDef` | 5 |
| 0x0041eb60 | `RouteNode_Free` | 5 |
| 0x0041d430 | `TrackLinkNodes` | 5 |
| 0x004273e0 | `RouteSeat_DetachCar` | 4 |
| 0x004273d0 | `RouteSeat_AttachCar` | 4 |
| 0x00426740 | `Sub_426740` | 4 |
| 0x0041cc90 | `JointBitFromIndex` | 4 |
| 0x00422640 | `Sub_422640` | 2 |
| 0x004225d0 | `Sub_4225d0` | 2 |
| 0x004207c0 | `GetCoasterColours` | 2 |
| 0x0041e7e0 | `Sub_41e7e0` | 2 |
| 0x00424e60 | `Coaster_OnCircuitClosed` | 1 |
| 0x00423790 | `Raster_RestoreState` | 1 |

### `LEGOLAND/tinystubs.c` — UI, save, sim, render and system stubs (≈50 functions)

| address | name | insns |
| --- | --- | --- |
| 0x00492ca0 | `SetThemeInTransition` | 15 |
| 0x00482b20 | `RefreshEntranceTile` | 15 |
| 0x0046b560 | `FreeScriptStepList` | 15 |
| 0x00468970 | `FreeScriptEventList` | 15 |
| 0x0044eab0 | `HasBlokeStayedTooLong` | 14 |
| 0x00496fc0 | `RegisterDetailImage` | 12 |
| 0x00492850 | `ResumePausedSamples` | 12 |
| 0x00492830 | `InitOptionSamples` | 12 |
| 0x0045f480 | `SetCursorError` | 12 |
| 0x0045ead0 | `IsBuildableClass` | 12 |
| 0x00455ec0 | `PrintCachedEntry` | 12 |
| 0x00485f00 | `PrintSpriteXY` | 11 |
| 0x00481ee0 | `ClearPathSquareVisited` | 11 |
| 0x00476000 | `ClearMenuHelp` | 11 |
| 0x004714e0 | `ClearNewObjectMarkers` | 11 |
| 0x00498120 | `RewindNarrationSource` | 10 |
| 0x00496570 | `PanFromOffset` | 10 |
| 0x0048fc00 | `ProcessScreenPopup` | 10 |
| 0x00471470 | `DisablePopUpInputs` | 10 |
| 0x0046f2e0 | `DefaultIconInput` | 10 |
| 0x0045eab0 | `ClassAllowsObjects` | 10 |
| 0x0045cb90 | `ResetPathTile` | 10 |
| 0x00490880 | `FreeReportHintBuffer` | 9 |
| 0x00490850 | `FreeHelpTextBuffer` | 9 |
| 0x00474970 | `LoadIconStateChunk` | 8 |
| 0x00473660 | `ProcessHelpKeys` | 8 |
| 0x00443120 | `RenderItem2_Alloc` | 8 |
| 0x00442f50 | `RenderItem_Alloc` | 8 |
| 0x0043f970 | `FixUpLocSetPointers` | 8 |
| 0x00499760 | `SetOrderRepairAmount` | 7 |
| 0x0047d7e0 | `SkipMeasuredBlock` | 7 |
| 0x00473a50 | `KillKeyboardDevice` | 7 |
| 0x00471d40 | `CloseInfoPopUp` | 6 |
| 0x00471bf0 | `ResetInfoSelection` | 6 |
| 0x0045f4b0 | `CursorIsValid` | 6 |
| 0x00444070 | `SetAdvisorPose` | 6 |
| 0x00498cf0 | `sub_498cf0` | 5 |
| 0x00474820 | `InGamePrimaryIcon` | 5 |
| 0x0046df60 | `RenderFullScreenIcon` | 5 |
| 0x0046d440 | `LinkIcon` | 5 |
| 0x00457890 | `BricksAreLimited` | 5 |
| 0x0045f460 | `ResetCursorFootprint` | 4 |
| 0x00490600 | `ShowInfoPanel` | 3 |
| 0x00468d00 | `ResetScriptTimer` | 3 |
| 0x00499560 | `GetMechanicCount` | 2 |
| 0x00499550 | `GetGardenerCount` | 2 |
| 0x00482b00 | `GetEntranceTile` | 2 |
| 0x004735b0 | `ResetHelpKeyCursor` | 2 |
| 0x004700f0 | `GetSelectedBloke` | 2 |
| 0x0046d3a0 | `SetHelpFaceTalking` | 2 |
| 0x0044ea40 | `GetVisitorLimit` | 2 |
| 0x0048b6c0 | `FreePlayInit_48b6c0` | 1 |
| 0x00476020 | `RenderIconsHook` | 1 |
| 0x00472090 | `DrawPopUpEnd` | 1 |
| 0x0045b170 | `RenderViewCellProbe` | 1 |
| 0x00453cd0 | `DebugErrorSink` | 1 |

Skip `DirectSoundCreate` (0x0049d31a) — it is an import thunk, not game code.
The 1–3-instruction rows are stubs or tail-jump wrappers; a `void` tail-`jmp`
wrapper that `audit.py` cannot bound gets
`// WIP-FUNCTION: LEGOLAND 0x<VA>  (100% by audit.py; tail-jmp)`.

**Order:** `tinystubs.c` → `ridetiny.c` → `coastertiny.c`, smallest first.

## Owned elsewhere — do not create or edit

`coaster7.c`, `uimisc2.c`, `audio5.c`, `render5.c` (Fable scope D);
`ridemisc4.c`, `pathmisc2.c` (Codex scope D); `uimisc3.c`, `coaster8.c`,
`savemisc2.c`, `lfmisc2.c`, `musicthread.c` (the integrating session's
lanes); and every file named in scopes F–J.
