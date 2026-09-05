# Scope T — the level-database keyword tier, part 3, and the process start-up (2026-09-06)

> **Status: OPEN, unclaimed.** Branch `scope/T`. Notes: `docs/lanes/scope-t.md`.
> Object prefix `/tmp/st_`. Any agent. Cut from inventory groups 24–25. Read
> `docs/SCOPE_R_level_keywords_1.md`'s "What this tier is" first.

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including the relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **29 functions, ≈1426 instructions**, two
new files.

## `LEGOLAND/levelkw3.c` — the last 22 keyword handlers (≈977 insns)

Same shape as scopes R and S (declare the primitives and the `AddEvent_*`
constructors `extern`). `REPORT` (129i) parses `off` / `HAPPY_VIS` /
`Happpy_Vis` (sic) through `LookupNamedIndex`; `FLASHBUTTON`/`FLASHBUTTOFF`
share one body with the mode flipped; `MAXCAPACITY`/`MAXVISITORS` and
`MINCAPACITY`/`MINVISITORS` are pairs of table entries on ONE handler each.

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x0047a5a0 | `LevelKw_PLACE` | 71 | table at 0x004bb934 in .data. calls `0x004786c0`, `ElemID`, `0x00478770`, `0x004a04b9`, `0x00469bd0`, `0x0046b880`; globals `g_level_number` |
| 0x0047a650 | `LevelKw_CLEAR` | 31 | table at 0x004bb93c in .data. calls `0x004786c0`, `0x00478700`, `0x0046b8c0` |
| 0x0047a6a0 | `LevelKw_UNGLUE` | 31 | table at 0x004bb944 in .data. calls `0x004786c0`, `0x00478700`, `0x0046b900` |
| 0x0047a6f0 | `LevelKw_GLUE` | 63 | table at 0x004bb94c in .data. calls `0x004786c0`, `0x00478700`, `0x0046b940`; globals `g_level_number`, `g_map_rows` |
| 0x0047a7b0 | `LevelKw_EXTENDPARK` | 31 | table at 0x004bb954 in .data. calls `0x004786c0`, `0x00478700`, `0x0046b980` |
| 0x0047a800 | `LevelKw_FMV` | 32 | table at 0x004bb95c in .data. calls `0x004786c0`, `0x0046b9c0`, `SetReportMovie`; globals `g_level_number` |
| 0x0047a860 | `LevelKw_INTERVAL` | 23 | table at 0x004bb964 in .data. calls `0x004786c0`, `0x0046b9f0` |
| 0x0047a8a0 | `LevelKw_MESSAGE` | 23 | table at 0x004bb96c in .data. calls `0x004786c0`, `0x0046ba30` |
| 0x0047a8e0 | `LevelKw_FEATURE` | 51 | table at 0x004bb974 in .data. calls `0x004786c0`, `0x004781b0`, `0x004a04b9`, `0x0046ba60`, `0x0046a040`; globals `g_level_number` |
| 0x0047a960 | `LevelKw_REPORT` | 129 | table at 0x004bb97c in .data. calls `0x004786c0` x2, `_stricmp` x2, `0x004a04b9` x2, `0x004781b0`, `0x0046ba90` x2, `0x0046a140` x2; strings "off", "HAPPY_VIS", "Happpy_Vis"; globals `g_level_number` |
| 0x0047aa90 | `LevelKw_HAP_FACTOR` | 42 | table at 0x004bb7bc in .data. calls `0x004786c0`, `0x004a04b9`, `0x004781b0`, `0x00482d60` |
| 0x0047ab00 | `LevelKw_CAPACITYSCALE` | 51 | table at 0x004bb7c4 in .data. calls `0x004786c0`, `0x004781b0`, `0x004a04b9`, `SetSimTuningA`, `0x0046bbb0`; globals `g_level_number` |
| 0x0047ab80 | `LevelKw_CAPACITYCAP` | 51 | table at 0x004bb7cc in .data. calls `0x004786c0`, `0x004781b0`, `0x004a04b9`, `SetSimTuningB`, `0x0046bbe0`; globals `g_level_number` |
| 0x0047ac00 | `LevelKw_DEGRADE` | 54 | table at 0x004bb984 in .data. calls `0x004786c0`, `ElemID`, `0x004a04b9` x2, `0x0046bb40` |
| 0x0047ac80 | `LevelKw_MAXBLOKES` | 30 | table at 0x004bb7b4 in .data. calls `0x004786c0`, `0x004a04b9`; globals `g_visitor_cap_extra`, `g_visitor_cap` |
| 0x0047ace0 | `LevelKw_MAXCAPACITY_MAXVISITORS` | 34 | table at 0x004bb98c in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046bb80`; globals `g_level_number`, `g_visitor_cap` |
| 0x0047ad40 | `LevelKw_MINCAPACITY_MINVISITORS` | 34 | table at 0x004bb99c in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046bb80`; globals `g_level_number`, `g_visitor_cap_extra` |
| 0x0047ada0 | `LevelKw_ENTRANCEFEE` | 33 | table at 0x004bb9ac in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046bc10`; globals `g_level_number`, `g_entrance_fee` |
| 0x0047ae00 | `LevelKw_FLASHBUTTON` | 64 | table at 0x004bb7d4 in .data. calls `0x004786c0`, `0x004781b0`, `0x004a04b9`, `0x00476070`, `0x0046bd70`; globals `g_level_number` |
| 0x0047aea0 | `LevelKw_FLASHBUTTOFF` | 67 | table at 0x004bb7dc in .data. calls `0x004786c0`, `0x004781b0`, `0x004a04b9`, `0x00476070`, `0x0046bd70`; globals `g_level_number` |
| 0x0047af50 | `LevelKw_PURGE` | 16 | table at 0x004bb9d4 in .data. calls `0x004786a0`, `0x0046bc60` |
| 0x0047af80 | `LevelKw_ENDLEVEL` | 16 | table at 0x004bb9dc in .data. calls `0x004786a0`, `0x0046bc40` |


## `LEGOLAND/startup.c` — from the CRT entry to `RunGame` (≈449 insns)

WinMain itself (0x00453d10, 48 instructions) is NOT in this scope: it has an
SEH prologue and `audit.py`'s walker stops at the `jmp` over its handler
block (`docs/lanes/scope-n.md`, "extents"). It calls `ReadExeVersionString`
(gameframe.c) and then `GameMain` below with its four arguments.

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x0047f820 | `ResetSaveTimerBase` | 3 | called by InitExitCheckBox (screens2.c). calls `GetGameTimer`; globals `g_save_time` |
| 0x0047f830 | `sub_47f830` | 2 | called by 0x0047f880 [unmatched].  |
| 0x0047f840 | `sub_47f840` | 2 | called by 0x0047f880 [unmatched].  |
| 0x0047f860 | `sub_47f860` | 1 | unreferenced (swept) **DEAD**.  |
| 0x0047f880 | `InitSession` | 262 | called by 0x0047fd10 [unmatched]. calls `0x0047f830`, `RES_EnsureMounted`, `RES_OpenVolume`, `0x00498d00`, `InitHostSystemGPU`, `InitScreen`, `GetString` x4, `KillHostSystemGPU` x3, `RES_CloseVolume` x4, `sprintf`, `InitInputSystem`, `KillInputSystem`, `LoadSprite` x8, `LLIDB_LoadICM`, `LLIDB_RegisterNewElement` x5, `RunGame`, `DebugPrintf`, `0x0047f840`, `DeleteStrings`, `LLIDB_CloseICM`, `UnreferenceSprite` x8; strings "legoland.log", "Failed to open resource %s", "LEGOLAND Error", "erase it.lls"; globals `0x007fd640`, `0x007fd64c`, `0x007fe9c4`, `0x007fe9c8`; IAT [4ab2d8] [4ab2a4] [4ab2d8] [4ab2a4] |
| 0x0047fc40 | `ParseCommandSwitch` | 95 | called by 0x0047fd10 [unmatched]. calls `HeapAlloc_w` x2, `0x004a0600` x2, `HeapFree_w` x3, `0x004a0580` |
| 0x0047fd10 | `GameMain` | 84 | called by 0x00453d10 [unmatched]. calls `DBPrintf` x2, `0x0047fc40` x4, `CheckHostSystemGPU`, `0x0047f880`; strings "LegolandGameMutex", "WINDEBUG", "BLT", "-nointro"; globals `g_windowed`, `0x0066920c`, `g_hinstance`; IAT [4ab10c] [4ab110] [4ab260] |

`GameMain` (0x0047fd10): `CreateMutexA("LegolandGameMutex")` / "Program
already running.", the command-line switches (`WINDEBUG`, `BLT`,
`-nointro`, `-nomusic`) through `ParseCommandSwitch` four times, `g_hinstance`,
`CheckHostSystemGPU`, then `InitSession`. `InitSession` (0x0047f880, 262i):
`legoland.log`, `RES_EnsureMounted`/`RES_OpenVolume` ("Failed to open
resource %s" / "LEGOLAND Error" through the two message-box imports),
`InitHostSystemGPU`, `InitScreen`, the eight cursor sprites (`erase it.lls`,
`erase it2.lls`, `no build.lls`, …), `GetString` x4, `RunGame` (gamemain.c),
`KillHostSystem…`, "Finished shutting stuff down". The three 1–3 instruction
bodies at 0x0047f820..0x0047f860 are timer stubs (`GetGameTimer` /
`g_save_time`); the swept one is dead.

## Owned elsewhere — do not create or edit

`levelkw.c`, `levelkw2.c` (R, S); `eventmake.c` (W); `eventtick.c`,
`eventtick2.c`, `eventgoal.c` (V, X); `exceptlog.c`, `objdesc.c` (U);
`gameframe.c`, `gamemain.c` (matched — read them for the callee side);
Codex-F's files; every file in scopes F, G, H; every existing `.c`.
