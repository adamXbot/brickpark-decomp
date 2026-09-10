# Scope V — the script-event tick handlers, part 1, and the goal checks (2026-09-06)

> **Status: DONE — 62 of 62 exact, merged into `main`.** `eventgoal.c` (16)
> and `eventtick.c` (46) carry no WIP markers; branch `scope/V` has been
> deleted and its evidence is in `docs/lanes/scope-v.md`. The original cut
> follows, kept for the mechanics it records. Branch `scope/V`. Notes: `docs/lanes/scope-v.md`.
> Object prefix `/tmp/sv_`. Any agent. Cut from inventory group 18. Read
> `docs/SCOPE_R_level_keywords_1.md`'s "What this tier is" for the pipeline
> (keyword handler → `AddEvent_<KW>` → `EventTick_<KW>`).

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including the relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **62 functions, ≈1571 instructions**, two new
files. `g_event_tick[]` at 0x004b9d44 (fpui3.c declares it, `EventTickFn`)
holds 70 per-kind handlers; `ScriptEventDue`/the step runner in uimisc.c
(matched) call `g_event_tick[event->kind](event)` — read uimisc.c around
0x0046b200..0x0046b6b0 for the calling convention and the `ScriptEvent`
layout (`NewScriptEvent(kind, mode)` 0x00468910 is matched too). The kind →
keyword map is exact (from the constructors' first push); the names below
follow it. The sixteen `GoalCheck_*` bodies at 0x00468f80..0x00469390 are the
goal evaluators the part-2 handlers call (scope X): each compares a live
count with the event's target through the four goal primitives at
0x00468c80/0x00468cd0/0x00468d10/0x00468d30 (scope X defines those; declare
them `extern`). Name the primitives from what they do when you get there.

## `LEGOLAND/eventgoal.c` — the sixteen goal checks (≈311 insns)

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x00468f80 | `GoalCheck_ParkVisitors` | 18 | called by 0x0046abd0 [unmatched]. calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x00468fc0 | `GoalCheck_Gardeners` | 18 | called by 0x0046aec0 [unmatched]. calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x00469000 | `GoalCheck_Gardeners2` | 19 | called by 0x0046aec0 [unmatched]. calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x00469040 | `GoalCheck_Mechanics` | 18 | called by 0x0046af10 [unmatched]. calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x00469080 | `GoalCheck_Mechanics2` | 19 | called by 0x0046af10 [unmatched]. calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x004690c0 | `GoalCheck_Save` | 18 | called by 0x0046ae40 [unmatched]. calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x00469100 | `GoalCheck_Happiness` | 20 | called by 0x0046ae70 [unmatched]. calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x00469140 | `GoalCheck_Hunger` | 21 | called by 0x0046af60 [unmatched]. calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x00469190 | `GoalCheck_Hunger2` | 21 | called by 0x0046af60 [unmatched]. calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x004691e0 | `GoalCheck_Rides` | 20 | called by 0x0046b080 [unmatched] (+1 more). calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x00469220 | `GoalCheck_RideVisitors` | 20 | called by 0x0046ac50 [unmatched] (+1 more). calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x00469260 | `GoalCheck_Composite` | 31 | called by 0x0046aae0 [unmatched]. calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x004692c0 | `GoalCheck_Unused` | 22 | unreferenced (swept) **DEAD**. calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x00469310 | `GoalCheck_Coverage` | 20 | called by 0x0046adf0 [unmatched] (+4 more). calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x00469350 | `GoalCheck_PathScenery` | 18 | called by 0x0046ad30 [unmatched]. calls `0x00468d10`, `0x00468d30`, `0x00468cd0`, `0x00468c80` |
| 0x00469390 | `GoalCheck_LoopComposite` | 8 | called by 0x0046ab70 [unmatched]. calls `0x00468d10`, `0x00468d30` |

## `LEGOLAND/eventtick.c` — kinds 2..37 (≈1260 insns)

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x00469980 | `RefreshThemeElements` | 86 | called by 0x00469a80 [unmatched]. calls `LLIDB_FindElement` x5; strings "LEGOLAND THEME", "COMMON THEME", "WESTERN THEME", "CASTLE THEME"; globals `g_drag_state`, `0x007fe115`, `0x007fe116`, `0x007fe117` |
| 0x00469a80 | `RefreshThemeMenu` | 14 | called by 0x00469b50 [unmatched]. calls `0x00469980`; globals `g_menu_dirty` |
| 0x00469ab0 | `sub_469ab0` | 13 | called by EnsureObjectClassLoaded (movie3.c) (+1 more). globals `g_menu_dirty` |
| 0x00469ae0 | `EventTick_Unimplemented` | 8 | called by 0x0046ae30 [unmatched] (+3 more). calls `DBPrintf` |
| 0x00469b00 | `EventTick_Unimplemented2` | 8 | called by 0x0046b1e0 [unmatched]. calls `DBPrintf` |
| 0x00469b20 | `EventTick_Give` | 16 | table at 0x004b9d4c in .data. calls `MarkElemAvailable` (`g_event_tick[2]`) |
| 0x00469b50 | `EventTick_Kind3` | 7 | table at 0x004b9d50 in .data. calls `0x00469a80` (`g_event_tick[3]`) |
| 0x00469b70 | `EventTick_Take` | 7 | table at 0x004b9d54 in .data. calls `MarkElemUnavailable` (`g_event_tick[4]`) |
| 0x00469b90 | `EventTick_Addbricks` | 8 | table at 0x004b9d58 in .data. calls `GetBrickCount`, `0x00457900` (`g_event_tick[5]`) |
| 0x00469bb0 | `EventTick_Currency` | 7 | table at 0x004b9d5c in .data. calls `0x00457900` (`g_event_tick[6]`) |
| 0x00469bd0 | `PlaceScriptObject` | 35 | called by 0x0047a5a0 [unmatched] (+1 more). calls `intrinsic` x2, `GetTileCentre`, `RefreshObjList`, `PutObjOnMap`; globals `g_view`, `0x007fffc8`, `g_obj_list` |
| 0x00469c40 | `EventTick_Place` | 9 | table at 0x004b9d60 in .data. calls `0x00469bd0` (`g_event_tick[7]`) |
| 0x00469c60 | `sub_469c60` | 7 | pointer in 0x00469c80 [unmatched]. calls `SetSampleFade` |
| 0x00469c80 | `EventTick_Clear` | 177 | table at 0x004b9d64 in .data. calls `0x0049e600`, `PlayInstanceOfSample` x2, `SetSampleLooping`, `AddSFX_Callback`, `GetFirstRenderObject`, `sub_4969d0`, `GetNextRenderObject` x3, `FindObjectsPower` x2, `BuildCursorPtr`, `CursorIsValid`, `RemoveObjectPathTiles`, `RemObjFromMap`, `CalculateMapRenderOrder`; globals `g_sel_def`, `g_destroy_cursor`, `g_sel_bpos`, `0x00811568` (`g_event_tick[8]`) |
| 0x00469ed0 | `EventTick_Unglue` | 29 | table at 0x004b9d68 in .data. globals `g_map_rows` (`g_event_tick[9]`) |
| 0x00469f20 | `EventTick_Glue` | 29 | table at 0x004b9d6c in .data. globals `g_map_rows` (`g_event_tick[10]`) |
| 0x00469f70 | `EventTick_Extendpark` | 5 | table at 0x004b9d70 in .data. calls `0x00469ae0` (`g_event_tick[11]`) |
| 0x00469f80 | `EventTick_Fmv` | 18 | table at 0x004b9d74 in .data. calls `FreezeGameClock`, `SetPointer`, `0x00496e60`, `PlayMovie`, `ThawGameClock`, `KillAdvisorHelp`, `RestoreScriptStepHelp` (`g_event_tick[12]`) |
| 0x00469fc0 | `EventTick_Interval` | 33 | table at 0x004b9d78 in .data. calls `ShowInfoPanel`, `LoadHelpTextFor`, `DBPrintf`; globals `g_game_mode`, `g_icons2_mode`, `g_cur_screen`, `g_screen_mode` (`g_event_tick[13]`) |
| 0x0046a030 | `EventTick_Message` | 5 | table at 0x004b9d7c in .data. calls `0x00469ae0` (`g_event_tick[14]`) |
| 0x0046a040 | `SetFeatureFlags` | 40 | called by 0x0047a8e0 [unmatched] (+1 more). calls `0x0044db40`; globals `g_path_overlay_active`, `g_ride_wear`, `0x00832988`, `g_power_available` |
| 0x0046a120 | `EventTick_Feature` | 9 | table at 0x004b9d80 in .data. calls `0x0046a040` (`g_event_tick[15]`) |
| 0x0046a140 | `SetReportMode` | 12 | called by 0x0047a960 [unmatched] (+1 more).  |
| 0x0046a170 | `EventTick_Report` | 11 | table at 0x004b9da4 in .data. calls `0x0046a140` (`g_event_tick[24]`) |
| 0x0046a190 | `EventTick_Gardener_Mechanic` | 33 | table at 0x004b9d84 in .data. calls `intrinsic` x2, `GenerateGardener`, `GenerateMechanic` (`g_event_tick[16]`) |
| 0x0046a1f0 | `EventTick_Workers` | 19 | table at 0x004b9d88 in .data.  (`g_event_tick[17]`) |
| 0x0046a230 | `EventTick_Degrade` | 78 | table at 0x004b9d8c in .data. calls `UpdateDamagedCell`; globals `g_map_dirty`, `g_map_rows` (`g_event_tick[18]`) |
| 0x0046a300 | `EventTick_Maxcapacity_Maxvisitors_Mincapacity_Minvisitors` | 12 | table at 0x004b9d90 in .data. globals `g_visitor_cap`, `g_visitor_cap_extra` (`g_event_tick[19]`) |
| 0x0046a330 | `EventTick_Capacityscale` | 9 | table at 0x004b9d94 in .data. calls `SetSimTuningA` (`g_event_tick[20]`) |
| 0x0046a350 | `EventTick_Capacitycap` | 9 | table at 0x004b9d98 in .data. calls `SetSimTuningB` (`g_event_tick[21]`) |
| 0x0046a370 | `EventTick_Entrancefee` | 5 | table at 0x004b9d9c in .data. globals `g_entrance_fee` (`g_event_tick[22]`) |
| 0x0046a390 | `EventTick_Endlevel` | 8 | table at 0x004b9dc4 in .data. calls `StopScript`; globals `0x00832978` (`g_event_tick[32]`) |
| 0x0046a3b0 | `EventTick_Lookat` | 37 | table at 0x004b9da0 in .data. calls `GetTileDimensions`; globals `g_scroll_x`, `g_scroll_y` (`g_event_tick[23]`) |
| 0x0046a420 | `EventTick_Themeicon` | 9 | table at 0x004b9da8 in .data. calls `0x00468860` (`g_event_tick[25]`) |
| 0x0046a440 | `EventTick_Addflag` | 9 | table at 0x004b9dac in .data. calls `0x00468890` (`g_event_tick[26]`) |
| 0x0046a460 | `EventTick_Bridges` | 9 | table at 0x004b9db0 in .data. calls `0x004688f0` (`g_event_tick[27]`) |
| 0x0046a480 | `EventTick_Breifingfile_Briefingfile` | 7 | table at 0x004b9db4 in .data. calls `0x004687f0` (`g_event_tick[28]`) |
| 0x0046a4a0 | `EventTick_Hintsfile` | 7 | table at 0x004b9db8 in .data. calls `0x00468810` (`g_event_tick[29]`) |
| 0x0046a4c0 | `EventTick_Flashbuttoff_Flashbutton` | 9 | table at 0x004b9dbc in .data. calls `0x00476070` (`g_event_tick[30]`) |
| 0x0046a4e0 | `EventTick_Purge` | 3 | table at 0x004b9dc0 in .data. globals `g_script_purge` (`g_event_tick[31]`) |
| 0x0046a4f0 | `EventTick_Need` | 29 | table at 0x004b9dc8 in .data. calls `ObjCount`, `0x00468d80`; globals `0x0066878c` (`g_event_tick[33]`) |
| 0x0046a540 | `EventTick_Needat` | 43 | table at 0x004b9dcc in .data. calls `0x00468d80`; globals `g_map_rows` (`g_event_tick[34]`) |
| 0x0046a5b0 | `EventTick_Needin` | 87 | table at 0x004b9dd0 in .data. calls `0x00468d80`; globals `g_map_rows` (`g_event_tick[35]`) |
| 0x0046a690 | `EventTick_Connect` | 68 | table at 0x004b9dd4 in .data. calls `0x00468dc0`; globals `g_map_rows` (`g_event_tick[36]`) |
| 0x0046a730 | `sub_46a730` | 12 | called by 0x0046a750 [unmatched].  |
| 0x0046a750 | `EventTick_Link` | 165 | table at 0x004b9dd8 in .data. calls `TileJoinsPathNetwork` x2, `GetFirstRenderObject`, `0x0046a730`, `GetNextRenderObject`, `0x00468dc0`, `0x00468e00`; globals `g_map_rows` (`g_event_tick[37]`) |

**Order:** the goal checks (one shape, sixteen times), then the tick
handlers smallest first; `EventTick_Clear` (177i) and `EventTick_Link`
(165i) last.

## Owned elsewhere — do not create or edit

`eventtick2.c` (X); `eventmake.c` (W); `levelkw.c`, `levelkw2.c`,
`levelkw3.c`, `startup.c` (R, S, T); `exceptlog.c`, `objdesc.c` (U);
Codex-F's files; every file in scopes F, G, H; every existing `.c`.
