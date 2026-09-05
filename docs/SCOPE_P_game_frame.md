# Scope P — the game frame: dispatcher, in-game frame, map click handler (2026-09-05)

> **Status: DONE — 10 of 10 exact (1,403 instructions), merged into `main` 2026-09-05.** Branch `scope/P`. Notes: `docs/lanes/scope-p.md`.
> Object prefix `/tmp/sp_`. Any agent. Cut from group 16 of the whole-binary
> inventory (`docs/lanes/scope-n.md`, Appendix A); its sibling is
> `SCOPE_Q_game_main.md` (group 17), which holds this file's caller.

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including the relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **10 functions, 1,403 instructions**, one new file. This
is the spine of the game: the per-frame dispatcher that the main loop
(`0x00459520`, scope Q) calls until it returns 0, the in-game frame it
dispatches to, and the click-on-map handler the frame calls. Nothing in the
tree calls into this file except three `screens3.c` icon handlers and
`StopScript`; everything in it is live (`tools/inventory.py` traces it from the
CRT entry through WinMain). For the browser runtime this file *is* the control
flow, so the notes' "mechanics recovered" section matters as much as the
match percentages.

## `LEGOLAND/gameframe.c` (1,403 insns)

Names are the integrator's provisional readings of the disassembly (the
inventory's `docs/lanes/scope-n.md` "startup chain" section has the
evidence); rename where the body contradicts them and record every rename.
Three of these already have `sub_` placeholders in `screens3.c` — define
them under the new name and list the placeholder in your notes so the
integrator renames the `screens3.c` externs at merge.

| address | provisional name | insns | evidence / callers |
| --- | --- | ---: | --- |
| 0x00458bc0 | `EnterFrontEnd` | 4 | `g_game_mode = 2; g_cur_screen = -1; g_screen_mode = 0` (bigscreens.c names all three). Called by the main loop. |
| 0x00458be0 | `CompleteLevelForProfile` | 7 | `screens3.c` declares it as `sub_458be0` ("the script stop path"): marks `CurProfile.level_done[g_map->+0x28]` when the level index is below 15, calls `0x0048a750` (unmatched, group 29), tail-jumps `UpDateCurrentProfile`. Called by `StopScript` (screens3.c). |
| 0x004588c0 | `ShowWaitSprite` | 31 | `LoadSprite` → `PushRenderingStatusAndLockVideoSurface` → `PrintSprite` → `PopRenderingStatus` → `RenderingComplete` → `UnreferenceSprite`. Called by the main loop around the intro. |
| 0x004589a0 | `ResetController` | 45 | zeroes the first fields of `*g_controller` (`0x00813b00`, bighelp.c) then `SetPointer`. Called by the main loop. |
| 0x00458a50 | `StartPark` | 45 | `screens3.c` declares it as `sub_458a50` ("park start-up"): `FreezeGameClock`, `ResetSaveTimer`, `sprintf`, `ResetMapAI`, `LoadLevelDatabase`, `AllocBlokeCounters`, `EnterParkPlayMode`, `UpdateMenu`, `ShowInfoPanel`, `SetInfoPanelText`, `ThawGameClock`. Called by `ProgressAcceptInput`, `LowProgressAcceptInput` (screens3.c) and the dispatcher. |
| 0x00458b20 | `BeginParkLoad` | 29 | `screens3.c` declares it as `sub_458b20` ("starts the actual load"): `ClearObjInfoList`, `RemoveObjectListIcons`, `DelObjectList`, `FreeBlokeCounters`, `ClearNewObjectMarkers`, `ResetInfoStruct`, `ClearBuildObjList`, `0x0049cfc0` (frees the object lists). Called by `LoadAcceptInput` (screens3.c) and the dispatcher. |
| 0x00458830 | `ReadExeVersionString` | 49 | `GetFileVersionInfoSizeA("Legoland.exe")` / `GetFileVersionInfoA` / `VerQueryValue("\StringFileInfo\080904B0\ProductVersion")` through the import thunks at `0x0049e3a0..0x0049e3ac`, `malloc`/`free`; writes the caller's buffer (`0x0066752c`). Called by WinMain (`0x00453d10`, not in any scope yet). |
| 0x00458c00 | `GameFrame` | 153 | the per-frame dispatcher: pending load (`profiles`, `%s\%dsave%d.sav` → `LoadGame`, `InitGameInterface`), `InitMapScreen`/`KillMapScreen`, `InitScreens`, `ReadGameButtons`, `g_dbg_where = "SFX"`, then `switch (g_game_mode)` through the jump table at `0x00458ec4` (4 cases: 0 quit, 1 `MapScreenFrame` 0x00459360 (scope Q), 2 the front-end screens, 3 `InGameFrame`). Returns 1 to continue. Called by the main loop as `r = f(); while (r) r = f();`. |
| 0x00458ee0 | `InGameFrame` | 289 | `g_dbg_where` phases `"AI"` (`HandleRideAI`, `DoMapAI`, `ControlPeople`, `ControlWorkers`), `"ProcessStuff"` (`CheckWorkerOnMouseStatus`, `ProcessBuildingTimes`, `ProcessDamage`), `"Zoning"`, `"Rendering"` (`ResetHitInfo`, `PushRenderingStatusAndLockVideoSurface`, `RenderView`, then `HandleMapClick` gated by button bit `0x100` or `[0x00813a40] & 0x1000`), `"In Game Help"`, `"Appraisals"` (`0x0044db90`, the appraisal-due tick, unmatched), `"Exiting GameProc"`. |
| 0x00457a70 | `HandleMapClick` | 751 | the click-on-map action handler: `g_hit_type`/`g_icon_value`/`g_hit_cell`, `ElemID("DRIVING SCHOOL ROADS")`/`("ZEBRA CROSSING")`, `GetGardenerWorkOrderAt`/`GetMechanicWorkOrderAt`, `GetRoadRecord`, `SetCursorError`, `UpdateMapDrag`, `BuildCursorPtr`/`RenderCursor`/`CursorIsValid`, `RemObjFromMap`, `WorkOrderBuildObject`, `PlayAppropriateBuildEffect`, `PopUpInfoSetUp`, `CalculateMapRenderOrder`, `ClearObjectUserFlags`. Its footprint-clearance test `0x00457970` (85i, group 15) is NOT in this scope: declare it `extern` and note it. |

**Order:** the eight small ones first (they fix the globals and the shape of
the two frames), then `GameFrame`, `InGameFrame`, and `HandleMapClick` last.
`HandleMapClick` at 751 instructions is the largest body this scope has; if
it does not close, retire it honestly with the §6B triage in the note.

## Hints

- `g_game_mode` (`0x008119b4`), `g_cur_screen`, `g_screen_mode` and the
  `CurProfile` record at `0x0080ffa0` are declared with their layout in
  `bigscreens.c`; `g_map` (`0x004bcbf4`) in anim2.c; `g_edit_object`
  (`0x008119b8`) in catapult.c; `g_hinstance` in wndenv.c. Read each
  global directly at every use (DECOMP/LEVERS: class/def globals).
- The `switch` in `GameFrame` keeps its jump table in `.text` right after
  the body (`0x00458ec4`, 28 bytes) — `audit.py` bounds it correctly;
  `matchfull.py` over-reports past the `ret`.
- `InGameFrame` and `GameFrame` set `g_dbg_where` to string literals before
  each phase; the string addresses are in `.data` at `0x004b91..` and the
  stores are plain global writes.
- Callees that are unmatched and outside this scope (`0x0044db90`,
  `0x0048a750`, `0x0049cfc0`, `0x00457970`, `0x0046f100`) get `extern`
  declarations with the address comment and a placeholder name of the form
  `sub_<addr>`; do not define them.
- `EnterFrontEnd`'s three stores are exactly the store-order lever case:
  emitted order is not evidence of source order — try the two orders.

## Owned elsewhere — do not create or edit

`movie2.c`, `movie3.c`, `pathmask2.c` (scope O); `gamemain.c`, `cursorseg.c`,
`mapbuild2.c` (scope Q); `coaster10.c`, `ridemachine2.c`, `uistubs2.c`
(Codex-F); every file in scopes F, G, H, I; every existing `.c` (`screens3.c`,
`bigscreens.c`, `uimisc.c` included — you read them, you do not edit them).

## Report format

As `docs/PARALLEL_CONTRACT.md` says, in `docs/lanes/scope-p.md`: per
function address, name (and why), instructions, percentage, `audit [OK]`,
marker as committed, first diverging index and residual for anything not
`[OK]`; then the mechanics recovered — for this scope that means the exact
phase order of `InGameFrame`, the `GameFrame` case table with what each
mode does, and what `HandleMapClick` does for each hit type — plus callees
and globals named for the first time, original bugs reproduced, extern-type
divergences, and every lever with its evidence.
