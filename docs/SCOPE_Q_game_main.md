# Scope Q — the main loop, the map-screen frame, level state and cursor segments (2026-09-05)

> **Status: DONE — 12 of 17 exact, 4 WIP with residuals named, the dead 291-instruction overlay decoded and not attempted; merged into `main` 2026-09-05 (integrator session).** Branch `scope/Q`. Notes: `docs/lanes/scope-q.md`.
> Object prefix `/tmp/sq_`. Any agent. Cut from group 17 of the whole-binary
> inventory (`docs/lanes/scope-n.md`, Appendix A) minus `0x00460f50`, which
> scope O owns as `DrawCursorPathTile`. Its sibling is `SCOPE_P_game_frame.md`
> (group 16), which holds the dispatcher this file's main loop calls.

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including the relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **17 functions, 1,323 instructions**, three new files.
`gamemain.c` is the session: WinMain's init routine (`0x0047f880`, not in
any scope yet) calls `RunGame`, which plays the intro, initialises the map and
loops on scope P's `GameFrame`. `cursorseg.c` is two `RenderCursor` helpers
`bigrender.c` already declares by name. `mapbuild2.c` is a build-tally
routine plus four functions the inventory proves DEAD (nothing live names
them; the linker kept them). Do the live files first; the dead ones are
coverage only.

## `LEGOLAND/gamemain.c` — the session (451 insns)

Names are the integrator's provisional readings; rename where the body
contradicts them and record every rename.

| address | provisional name | insns | evidence / callers |
| --- | --- | ---: | --- |
| 0x004594e0 | `KillFrontEndScreenIfActive` | 5 | `if (g_game_mode == 2) KillCurrentScreen();` (tail jump to `0x004585c0`, uimisc.c). Called by `RunGame`. |
| 0x004594f0 | `ResetCurProfileDefaults` | 11 | `rep stosd` over the 0x44-dword `CurProfile` record at `0x0080ffa0` (bigscreens.c has the layout), then `g_vol_music = 100`, `g_vol_speech = g_vol_sfx = 75` (audio3.c names the three). Called by `RunGame`. |
| 0x00462e50 | `SetLevelParamA` | 6 | `rec[i].f0 = v` on a 6-record, 0x2c-byte-stride table at `0x00832824` (unnamed in the tree). Called by two level-database keyword handlers (`0x0047ab00`, group 27) and a script-event tick (`0x0046a330`, group 20). |
| 0x00462e70 | `SetLevelParamB` | 6 | `rec[i].f1 = v` on the same table (`0x00832828`). Same caller shape (`0x0047ab80`, `0x0046a350`). |
| 0x00462e90 | `ResetLevelParams` | 17 | initialises records 0, 1, 4, 5 of that table (`f0`/`f1` = 0x32/0x14, 0x21/0x32, 0x21/0x28, 0x21/0x28) and zeroes 2 and 3. Called by `ResetLevelGlobals` (`0x004784c0`), which scope O is matching in `movie3.c` — when `origin/scope/O` lands, take the name its `extern` uses if it differs, and record it. |
| 0x004597e0 | `SetLevelEndSequence` | 20 | `memcpy(which ? g_level_end_sequence1 : g_level_end_sequence2, s, 0x100); buf[0xff] = 0`, or `buf[0] = 0` when `s` is NULL (uimisc.c names both buffers; the copy is the CALLED `memcpy` at `0x004a0110`, see DECOMP's `#pragma function(memcpy)` lever from fable-d). Called by `0x0044dc70` and the `0x0047a1d0` keyword handler. |
| 0x0045ac20 | `UnloadSessionSprites` | 50 | `LLIDB_FindElement`, `LLIDB_UnLoadData`, `UnreferenceSprite`, `free`. Called by `RunGame` at teardown. |
| 0x00459360 | `MapScreenFrame` | 102 | `GameFrame`'s mode-1 case: `ResetHitInfo`, `PushRenderingStatusAndLockVideoSurface`, `DrawMapScreen`, `SetPointer`, `PrintSprite`, `0x0046ee00` (unmatched, group 22), `RenderIcons`, `CheckFocussedIcon`, `UpdateFocussedIconPtr`, `PopRenderingStatus`, `RenderingComplete`, `GetClipping`/`SetClipping`, `RenderMouseBounds`. |
| 0x004629e0 | `ResetLevelObjects` | 113 | `ResetBuildStats`, `LLIDB_UnLoadData`, `LLIDB_GetCount`/`LLIDB_GetElement`, `free`, `LLIDB_FindElementFromDataPtr`, `ClearOverlays`, `sub_4828f0`. Called by `0x0046cb20` (unmatched, group 22). |
| 0x00459520 | `RunGame` | 121 | the main loop: `ElemID`, `ResetCurProfileDefaults`, `InitSoundSystem`, `SetMusicGrooveLevel`, `0x00492c60`/`0x00492c80` (unmatched, group 30), `SetupControllers`, `LLIDB_ClearOnLevel`, `ResetController`, `SetPointer`, `ProcessSystemEvents`, `PlayMovie` (`Intro.avi`, `Ir50_32.dll`), `ShowWaitSprite`, `SetWaitSpriteRect`, `InitGameMap`, `LoadMapTiles`, `CreateObjectClasses`, then `r = GameFrame(); while (r) r = GameFrame();` (two call sites `0x0045967f`/`0x00459688`), then `UnloadSessionSprites`. Called by `0x0047f880`. |

## `LEGOLAND/cursorseg.c` — the cursor segment fills (355 insns)

`bigrender.c` declares both with these names and this prototype; keep them.

| address | name | insns | evidence / callers |
| --- | --- | ---: | --- |
| 0x0045fad0 | `DrawCursorSegmentB` | 160 | `void (VideoSurfaceInfo* vs, int kind, int x, int y, const char* col, int h)`; `GetNearestColour`. Called by `RenderCursor` (bigrender.c, a WIP body in scope I — read its note, do not edit it). |
| 0x0045fca0 | `DrawCursorSegmentA` | 195 | same prototype and callee. |

## `LEGOLAND/mapbuild2.c` — a build tally and four dead bodies (517 insns)

| address | provisional name | insns | evidence / callers |
| --- | --- | ---: | --- |
| 0x00459970 | `TallyBuildFootprints` | 116 | `GetGameTimer`, `0x00481720` (2i, unmatched), `TallyFootprintCell` (mapbuild.c, its neighbour). Called by the script-event tick `0x0046ad30` (group 21). LIVE. |
| 0x004598b0 | `ClearBuildTally` | 6 | zeroes `0x00667d00..0x00667d0c`. DEAD. |
| 0x0045e930 | `DoorTileStep` | 15 | helper of the next. DEAD. |
| 0x0045e960 | `FindObjDoorTile` | 89 | `ObjHasEntrance`, `ObjHasExit` (objdoor.c neighbours). DEAD. |
| 0x0045ade0 | `DrawTileDebugOverlay` | 291 | `SetClipping`, `PrintSprite`; sits beside `GetTileCentre` (tilehelp.c). DEAD. |

**Order:** `gamemain.c` smallest first, `RunGame` last; then `cursorseg.c`;
then `mapbuild2.c` with `TallyBuildFootprints` first and the four dead
bodies last — they are worth exact matches for byte coverage but nothing in
the runtime will ever call them, so stop on them early if they resist.

## Hints

- `RunGame` calls `GameFrame` from TWO sites (`r = f(); while (r) r = f();`
  is what the original's layout says); write it that way rather than as a
  `do/while`.
- The `0x00832824` table: six records of eleven dwords. Name the struct
  only from what the three setters and `ResetLevelParams` prove; the
  keyword handlers that call the setters (groups 26–27, unmatched) will
  name the fields later.
- `SetLevelEndSequence`: the copy is a called `memcpy`, not `rep movsd`;
  fable-d's lever (`#pragma function(memcpy)` around the one body) is the
  spelling.
- Dead bodies: `audit.py` bounds them like any other; the only difference
  is that no caller's prototype constrains their types, so read the
  parameter widths off the disassembly.

## Owned elsewhere — do not create or edit

`movie2.c`, `movie3.c`, `pathmask2.c` (scope O — `DrawCursorPathTile`
0x00460f50 is theirs); `gameframe.c` (scope P); `coaster10.c`,
`ridemachine2.c`, `uistubs2.c` (Codex-F); every file in scopes F, G, H, I
(`bigrender.c`'s `RenderCursor` is scope I's WIP); every existing `.c`.

## Report format

As `docs/PARALLEL_CONTRACT.md` says, in `docs/lanes/scope-q.md`: per
function address, name (and why), instructions, percentage, `audit [OK]`,
marker as committed, first diverging index and residual for anything not
`[OK]`; then the mechanics recovered — the exact init order in `RunGame`,
the `MapScreenFrame` draw order, the `0x00832824` record layout — plus
callees and globals named for the first time, original bugs reproduced,
extern-type divergences, and every lever with its evidence.
