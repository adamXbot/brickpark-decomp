# Scope W — the script-event constructors (2026-09-06)

> **Status: OPEN, unclaimed.** Branch `scope/W`. Notes: `docs/lanes/scope-w.md`.
> Object prefix `/tmp/sw_`. Any agent. Cut from inventory groups 20–21. Read
> `docs/SCOPE_R_level_keywords_1.md`'s "What this tier is" for the pipeline.

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including the relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **72 functions, ≈1132 instructions** — small and
formulaic — one new file `LEGOLAND/eventmake.c`. Each `AddEvent_<KW>` is
`e = NewScriptEvent(kind, mode); e->fields = args; LinkStepEvent(e,
g_script_cur)` (the first push is the kind; `LinkStepEvent` 0x0046b630
appends to the step's list at +0x10); the goal constructors (`NEED` …
`FOREVER`, kinds 33–69) link through `LinkGoalEvent` 0x0046b610 to
`g_script_root` instead. `SetScriptEventText` (matched) stores the text
argument for `FMV`, `INTERVAL`, `MESSAGE`, `BRIEFINGFILE`, `HINTSFILE`. The
`ScriptEvent` layout is in uimisc.c/fpui3.c (matched); keep their field
names. Disassemble two, build one, transfer the rest.

## `LEGOLAND/eventmake.c`

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x0046b590 | `FreeScriptSteps` | 23 | called by 0x004787d0 [unmatched]. globals `g_script_steps` |
| 0x0046b610 | `LinkGoalEvent` | 10 | called by 0x0046c510 [unmatched] (+35 more). globals `g_script_root` |
| 0x0046b630 | `LinkStepEvent` | 11 | called by 0x0046bc40 [unmatched] (+30 more).  |
| 0x0046b650 | `AddEvent_Intro` | 39 | called by 0x004791a0 [unmatched]. calls `HeapFree_w`, `HeapAlloc_w` (kind None) |
| 0x0046b700 | `ShowStepHint` | 23 | called by ScriptEndIconInput (screens3.c). calls `AddHelpMessage`, `ShowScriptStepText`, `ResetScriptTimer`; globals `g_script_cur`, `g_last_hint`, `g_hint_up` |
| 0x0046b790 | `AddEvent_Give` | 14 | called by 0x0047a480 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 2) |
| 0x0046b7c0 | `AddEvent_Kind3` | 14 | unreferenced (swept) **DEAD**. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` |
| 0x0046b7f0 | `AddEvent_Take` | 12 | called by 0x0047a500 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 4) |
| 0x0046b820 | `AddEvent_Addbricks` | 12 | called by 0x0047a550 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 5) |
| 0x0046b850 | `AddEvent_Currency` | 12 | called by 0x00478c60 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 6) |
| 0x0046b880 | `AddEvent_Place` | 19 | called by 0x0047a5a0 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 7) |
| 0x0046b8c0 | `AddEvent_Clear` | 22 | called by 0x0047a650 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 8) |
| 0x0046b900 | `AddEvent_Unglue` | 22 | called by 0x0047a6a0 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 9) |
| 0x0046b940 | `AddEvent_Glue` | 22 | called by 0x0047a6f0 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 10) |
| 0x0046b980 | `AddEvent_Extendpark` | 22 | called by 0x0047a7b0 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 11) |
| 0x0046b9c0 | `AddEvent_Fmv` | 17 | called by 0x0047a800 [unmatched]. calls `NewScriptEvent`, `SetScriptEventText`, `0x0046b630`; globals `g_script_cur` (kind 12) |
| 0x0046b9f0 | `AddEvent_Interval` | 18 | called by 0x0047a860 [unmatched]. calls `NewScriptEvent`, `SetScriptEventText`, `0x0046b630`; globals `g_script_cur` (kind 13) |
| 0x0046ba30 | `AddEvent_Message` | 17 | called by 0x0047a8a0 [unmatched]. calls `NewScriptEvent`, `SetScriptEventText`, `0x0046b630`; globals `g_script_cur` (kind 14) |
| 0x0046ba60 | `AddEvent_Feature` | 14 | called by 0x0047a8e0 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 15) |
| 0x0046ba90 | `AddEvent_Report` | 16 | called by 0x0047a960 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 24) |
| 0x0046bad0 | `AddEvent_Gardener_Mechanic` | 19 | called by 0x00479060 [unmatched] (+1 more). calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 16) |
| 0x0046bb10 | `AddEvent_Workers` | 14 | called by 0x00478f00 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 17) |
| 0x0046bb40 | `AddEvent_Degrade` | 16 | called by 0x0047ac00 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 18) |
| 0x0046bb80 | `AddEvent_Maxcapacity_Maxvisitors_Mincapacity_Minvisitors` | 14 | called by 0x0047ad40 [unmatched] (+1 more). calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 19) |
| 0x0046bbb0 | `AddEvent_Capacityscale` | 14 | called by 0x0047ab00 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 20) |
| 0x0046bbe0 | `AddEvent_Capacitycap` | 14 | called by 0x0047ab80 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 21) |
| 0x0046bc10 | `AddEvent_Entrancefee` | 12 | called by 0x0047ada0 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 22) |
| 0x0046bc40 | `AddEvent_Endlevel` | 10 | called by 0x0047af80 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 32) |
| 0x0046bc60 | `AddEvent_Purge` | 10 | called by 0x0047af50 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 31) |
| 0x0046bc80 | `AddEvent_Themeicon` | 14 | called by 0x0047a020 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 25) |
| 0x0046bcb0 | `AddEvent_Addflag` | 14 | called by 0x0047a0b0 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 26) |
| 0x0046bce0 | `AddEvent_Bridges` | 14 | called by 0x0047a140 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 27) |
| 0x0046bd10 | `AddEvent_Breifingfile_Briefingfile` | 17 | called by 0x00478e20 [unmatched]. calls `NewScriptEvent`, `SetScriptEventText`, `0x0046b630`; globals `g_script_cur` (kind 28) |
| 0x0046bd40 | `AddEvent_Hintsfile` | 17 | called by 0x00478e90 [unmatched]. calls `NewScriptEvent`, `SetScriptEventText`, `0x0046b630`; globals `g_script_cur` (kind 29) |
| 0x0046bd70 | `AddEvent_Flashbuttoff_Flashbutton` | 14 | called by 0x0047aea0 [unmatched] (+1 more). calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 30) |
| 0x0046bda0 | `AddEvent_Lookat` | 15 | called by 0x00478d30 [unmatched]. calls `NewScriptEvent`, `0x0046b630`; globals `g_script_cur` (kind 23) |
| 0x0046bdd0 | `AddEvent_Need` | 15 | called by 0x004791f0 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 33) |
| 0x0046be00 | `AddEvent_Needat` | 18 | called by 0x00479270 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 34) |
| 0x0046be40 | `AddEvent_Needin` | 27 | called by 0x00479300 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 35) |
| 0x0046be90 | `AddEvent_Connect` | 13 | called by 0x00479390 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 36) |
| 0x0046bec0 | `AddEvent_Link` | 13 | called by 0x004793e0 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 37) |
| 0x0046bef0 | `AddEvent_Range` | 17 | called by 0x00479450 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 38) |
| 0x0046bf30 | `AddEvent_Cleararea` | 25 | called by 0x004794d0 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 39) |
| 0x0046bf80 | `AddEvent_Remove` | 15 | called by 0x00479550 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 40) |
| 0x0046bfb0 | `AddEvent_Removerange` | 17 | called by 0x004795c0 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 41) |
| 0x0046bff0 | `AddEvent_Composite` | 17 | called by 0x00479640 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 42) |
| 0x0046c030 | `AddEvent_Loopcomposite` | 15 | called by 0x004796d0 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 43) |
| 0x0046c060 | `AddEvent_Techlevel` | 15 | called by 0x00479740 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 44) |
| 0x0046c090 | `AddEvent_Parkvisitors` | 13 | called by 0x00479800 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 45) |
| 0x0046c0c0 | `AddEvent_Ridevisitors` | 15 | called by 0x00479850 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 46) |
| 0x0046c0f0 | `AddEvent_Riders` | 15 | called by 0x004798c0 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 47) |
| 0x0046c120 | `AddEvent_Scenerycoverage` | 13 | called by 0x00479930 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 48) |
| 0x0046c150 | `AddEvent_Pathscenery` | 13 | called by 0x00479980 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 49) |
| 0x0046c180 | `AddEvent_Ridecoverage` | 13 | called by 0x004799d0 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 50) |
| 0x0046c1b0 | `AddEvent_Shopcoverage` | 13 | called by 0x00479a20 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 51) |
| 0x0046c1e0 | `AddEvent_Foodcoverage` | 13 | called by 0x00479a70 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 52) |
| 0x0046c210 | `AddEvent_Totcoverage` | 13 | called by 0x00479ac0 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 53) |
| 0x0046c240 | `AddEvent_Studarea` | 25 | called by 0x00479c40 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 55) |
| 0x0046c290 | `AddEvent_Save` | 13 | called by 0x00479cb0 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 56) |
| 0x0046c2c0 | `AddEvent_Happiness` | 15 | called by 0x00479d00 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 57) |
| 0x0046c2f0 | `AddEvent_Needgardeners` | 13 | called by 0x00479d60 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 58) |
| 0x0046c320 | `AddEvent_Needmechanics` | 13 | called by 0x00479db0 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 59) |
| 0x0046c350 | `AddEvent_Hunger` | 17 | called by 0x00479e00 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 60) |
| 0x0046c390 | `AddEvent_Fixrides` | 15 | called by 0x00479e80 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 61) |
| 0x0046c3c0 | `AddEvent_Powerrides` | 13 | called by 0x00479ee0 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 62) |
| 0x0046c3f0 | `AddEvent_Zoning` | 15 | called by 0x00479f30 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 63) |
| 0x0046c420 | `AddEvent_Checkflag` | 15 | called by 0x00479fa0 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 64) |
| 0x0046c450 | `AddEvent_Selecttheme` | 13 | called by 0x0047a2f0 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 65) |
| 0x0046c480 | `AddEvent_Selecttab` | 13 | called by 0x0047a360 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 66) |
| 0x0046c4b0 | `AddEvent_Selectmode` | 13 | called by 0x0047a3d0 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 67) |
| 0x0046c4e0 | `sub_46c4e0` | 13 | unreferenced (swept) **DEAD**. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` |
| 0x0046c510 | `AddEvent_Forever` | 11 | called by 0x0047a440 [unmatched]. calls `NewScriptEvent`, `0x0046b610`; globals `g_script_cur` (kind 69) |

## Owned elsewhere — do not create or edit

`eventtick.c`, `eventtick2.c`, `eventgoal.c` (V, X); `levelkw.c`,
`levelkw2.c`, `levelkw3.c`, `startup.c` (R, S, T); `exceptlog.c`,
`objdesc.c` (U); Codex-F's files; every file in scopes F, G, H; every
existing `.c`.
