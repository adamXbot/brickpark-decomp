# Scope X — the script-event tick handlers, part 2, and the goal primitives (2026-09-06)

> **Status: OPEN, unclaimed.** Branch `scope/X`. Notes: `docs/lanes/scope-x.md`.
> Object prefix `/tmp/sx_`. Any agent. Cut from inventory groups 17 and 19.
> Read `docs/SCOPE_V_event_ticks_1.md` first — this is the second half of the
> same table, plus the primitives both halves call.

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including the relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **54 functions, ≈1211 instructions**, two
new files.

## `LEGOLAND/eventtick2.c` — kinds 38..69 (≈858 insns)

The goal kinds (`RANGE` … `SELECTMODE`) each call one of scope V's
`GoalCheck_*` bodies (declare `extern`) after gathering the live number
(`ObjCount`, `GetGardenerCount`, `GetBrickCount`, the coverage tallies
`g_area_*`, `TallyBuildFootprints` for `PATHSCENERY`); the 2- and
5-instruction ones are stubs (`EventTick_Unimplemented` in scope V is the
shared `DBPrintf` stub).

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x0046a900 | `EventTick_Range` | 44 | table at 0x004b9ddc in .data. calls `0x00468e40`; globals `g_objdef_head` (`g_event_tick[38]`) |
| 0x0046a960 | `EventTick_Cleararea` | 81 | table at 0x004b9de0 in .data. calls `0x00468f00`; globals `g_map_rows` (`g_event_tick[39]`) |
| 0x0046aa30 | `EventTick_Remove` | 22 | table at 0x004b9de4 in .data. calls `ObjCount`, `0x00468f40` (`g_event_tick[40]`) |
| 0x0046aa70 | `EventTick_Removerange` | 50 | table at 0x004b9de8 in .data. calls `0x00468ea0`; globals `g_objdef_head` (`g_event_tick[41]`) |
| 0x0046aae0 | `EventTick_Composite` | 60 | table at 0x004b9dec in .data. calls `ObjCount`, `0x00468d80`, `0x00469260`; globals `g_objdef_head` (`g_event_tick[42]`) |
| 0x0046ab70 | `EventTick_Loopcomposite` | 28 | table at 0x004b9df0 in .data. calls `0x00469390`, `DBPrintf` (`g_event_tick[43]`) |
| 0x0046abc0 | `EventTick_Techlevel` | 5 | table at 0x004b9df4 in .data. calls `0x00469ae0` (`g_event_tick[44]`) |
| 0x0046abd0 | `EventTick_Parkvisitors` | 14 | table at 0x004b9df8 in .data. calls `0x00468f80`; globals `g_visitor_count` (`g_event_tick[45]`) |
| 0x0046ac00 | `EventTick_Riders` | 31 | table at 0x004b9e00 in .data. calls `0x00469220` (`g_event_tick[47]`) |
| 0x0046ac50 | `EventTick_Ridevisitors` | 69 | table at 0x004b9dfc in .data. calls `_stricmp`, `GetFirstObjectMatching`, `0x00489fd0`, `0x00469220` (`g_event_tick[46]`) |
| 0x0046ad00 | `EventTick_Scenerycoverage` | 15 | table at 0x004b9e04 in .data. calls `0x00469310`; globals `g_area_type2` (`g_event_tick[48]`) |
| 0x0046ad30 | `EventTick_Pathscenery` | 15 | table at 0x004b9e08 in .data. calls `0x00459970`, `0x00469350`; globals `g_tally_percent` (`g_event_tick[49]`) |
| 0x0046ad60 | `EventTick_Ridecoverage` | 15 | table at 0x004b9e0c in .data. calls `0x00469310`; globals `g_area_type1` (`g_event_tick[50]`) |
| 0x0046ad90 | `EventTick_Shopcoverage` | 15 | table at 0x004b9e10 in .data. calls `0x00469310`; globals `g_area_type4` (`g_event_tick[51]`) |
| 0x0046adc0 | `EventTick_Foodcoverage` | 15 | table at 0x004b9e14 in .data. calls `0x00469310`; globals `g_area_type5` (`g_event_tick[52]`) |
| 0x0046adf0 | `EventTick_Totcoverage` | 15 | table at 0x004b9e18 in .data. calls `0x00469310`; globals `g_area_total` (`g_event_tick[53]`) |
| 0x0046ae20 | `EventTick_Kind54` | 2 | table at 0x004b9e1c in .data.  (`g_event_tick[54]`) |
| 0x0046ae30 | `EventTick_Studarea` | 5 | table at 0x004b9e20 in .data. calls `0x00469ae0` (`g_event_tick[55]`) |
| 0x0046ae40 | `EventTick_Save` | 18 | table at 0x004b9e24 in .data. calls `GetBrickCount` x2, `0x004690c0` (`g_event_tick[56]`) |
| 0x0046ae70 | `EventTick_Happiness` | 34 | table at 0x004b9e28 in .data. calls `0x00469100`; globals `g_people_head` (`g_event_tick[57]`) |
| 0x0046aec0 | `EventTick_Needgardeners` | 31 | table at 0x004b9e2c in .data. calls `GetGardenerCount`, `0x00468fc0`, `0x00469000` (`g_event_tick[58]`) |
| 0x0046af10 | `EventTick_Needmechanics` | 31 | table at 0x004b9e30 in .data. calls `GetMechanicCount`, `0x00469040`, `0x00469080` (`g_event_tick[59]`) |
| 0x0046af60 | `EventTick_Hunger` | 59 | table at 0x004b9e34 in .data. calls `0x00469140`, `0x00469190`; globals `g_people_head` (`g_event_tick[60]`) |
| 0x0046afe0 | `EventTick_Fixrides` | 61 | table at 0x004b9e38 in .data. calls `GetFirstRenderObject`, `GetNextRenderObject`, `0x004691e0` (`g_event_tick[61]`) |
| 0x0046b080 | `EventTick_Powerrides` | 15 | table at 0x004b9e3c in .data. calls `0x004691e0`; globals `g_bridge_sets` (`g_event_tick[62]`) |
| 0x0046b0b0 | `EventTick_Zoning` | 2 | table at 0x004b9e40 in .data.  (`g_event_tick[63]`) |
| 0x0046b0c0 | `EventTick_Checkflag` | 22 | table at 0x004b9e44 in .data. calls `0x004688c0`, `0x00468d10`, `0x00468d30` (`g_event_tick[64]`) |
| 0x0046b100 | `EventTick_Selecttheme` | 18 | table at 0x004b9e48 in .data. calls `0x00468d10`, `0x00468d30` (`g_event_tick[65]`) |
| 0x0046b130 | `EventTick_Selecttab` | 30 | table at 0x004b9e4c in .data. calls `0x00468d10`, `0x00468d30`; globals `g_object_list_mode` (`g_event_tick[66]`) |
| 0x0046b180 | `EventTick_Selectmode` | 29 | table at 0x004b9e50 in .data. calls `0x00468d10`, `0x00468d30`; globals `g_edit_changed`, `g_effect_obj`, `g_env_class` (`g_event_tick[67]`) |
| 0x0046b1e0 | `EventTick_Kind68` | 5 | table at 0x004b9e54 in .data. calls `0x00469b00` (`g_event_tick[68]`) |
| 0x0046b1f0 | `EventTick_Forever` | 2 | table at 0x004b9e58 in .data.  (`g_event_tick[69]`) |

## `LEGOLAND/eventgoalprim.c` — what the checks and handlers lean on (≈353 insns)

Sizes from the inventory; disassemble each and name it from what it does
(the four at 0x00468c80..0x00468d30 are called by all sixteen goal checks:
target/compare/state primitives on the event record; 0x004687f0/0x00468810
store the briefing and hints file names the `BRIEFINGFILE`/`HINTSFILE`
handlers AND ticks both call; `SetCurrency` 0x00457900 is three
instructions beside `SaveCurrency`).

| address | provisional name | insns | reached by |
| --- | --- | ---: | --- |
| 0x004687f0 | `SetBriefingFile` | 8 | see `tools/inventory.py` |
| 0x00468810 | `SetHintsFile` | 8 | see `tools/inventory.py` |
| 0x00468860 | `SetThemeIcon` | 13 | see `tools/inventory.py` |
| 0x00468890 | `AddLevelFlag` | 11 | see `tools/inventory.py` |
| 0x004688c0 | `sub_4688c0` | 7 | see `tools/inventory.py` |
| 0x004688f0 | `SetBridges` | 7 | see `tools/inventory.py` |
| 0x00468c80 | `GoalPrim_468c80` | 27 | see `tools/inventory.py` |
| 0x00468cd0 | `GoalPrim_468cd0` | 15 | see `tools/inventory.py` |
| 0x00468d10 | `GoalPrim_468d10` | 9 | see `tools/inventory.py` |
| 0x00468d30 | `GoalPrim_468d30` | 27 | see `tools/inventory.py` |
| 0x00468d80 | `GoalPrim_468d80` | 20 | see `tools/inventory.py` |
| 0x00468dc0 | `GoalPrim_468dc0` | 19 | see `tools/inventory.py` |
| 0x00468e00 | `GoalPrim_468e00` | 19 | see `tools/inventory.py` |
| 0x00468e40 | `GoalPrim_468e40` | 31 | see `tools/inventory.py` |
| 0x00468ea0 | `GoalPrim_468ea0` | 31 | see `tools/inventory.py` |
| 0x00468f00 | `GoalPrim_468f00` | 18 | see `tools/inventory.py` |
| 0x00468f40 | `GoalPrim_468f40` | 20 | see `tools/inventory.py` |
| 0x00476070 | `FlashButton` | 23 | see `tools/inventory.py` |
| 0x00476140 | `sub_476140` | 16 | see `tools/inventory.py` |
| 0x00457900 | `SetCurrency` | 3 | see `tools/inventory.py` |
| 0x0044db40 | `sub_44db40` | 17 | see `tools/inventory.py` |
| 0x00482d60 | `SetHappinessFactor` | 4 | see `tools/inventory.py` |

## Owned elsewhere — do not create or edit

`eventtick.c`, `eventgoal.c` (V); `eventmake.c` (W); `levelkw.c`,
`levelkw2.c`, `levelkw3.c`, `startup.c` (R, S, T); `exceptlog.c`,
`objdesc.c` (U); Codex-F's files; every file in scopes F, G, H; every
existing `.c`.
