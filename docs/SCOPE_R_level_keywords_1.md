# Scope R — the level-database keyword tier, part 1: the reader, the parse primitives and the first handlers (2026-09-06)

> **Status: CLAIMED — delivered on `origin/scope/R` 2026-09-06 at 47 of 48 exact (`ParseKeywordSections` held at WIP at a compiler-build floor, see its notes); NOT yet merged — the integrator merges it on the user's call, renaming its primitive definitions to the tree's `KwLineApplies` / `KwSectionMatches` / `KwHasArgs`. Do not assign again.** Branch `scope/R`. Notes: `docs/lanes/scope-r.md`.
> Object prefix `/tmp/sr_`. Any agent. Cut from inventory groups 21–22
> (`tools/inventory.py`, 2026-09-06). Siblings: S (handlers part 2), T (part 3
> and the process start-up), W (the event constructors the handlers call),
> V and X (the event tick handlers). Take one; they do not collide.

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including the relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **48 functions, ≈1580 instructions** (three
reader helpers not yet sized), one new file `LEGOLAND/levelkw.c`.

## What this tier is

`LoadLevelDatabase` (movie.c, matched) opens the level's keyword file and
`ParseKeywordSections` (0x00478280, declared by movie3.c) reads it section by
section; every line's first word is looked up in **the keyword table at
0x004bb6f8**: 93 entries of `{const char* keyword; void (*handler)(void)}`
(dump it with `tools/disasm.py`'s neighbour or a 12-line Python over
`match.load_exe`). Each handler reads its arguments with the primitives
below (`KwLineApplies` 0x004786c0 — `(g_level_number & mask) && argc >= nargs`, the section-kind and word-count test, first; `atoi` at CRT 0x004a04b9 for
numbers, `ElemID` for object names, `LookupNamedIndex` 0x004781b0 for
enumerated words) and then either acts immediately (`MAP` loads the base
map, `ENABLE`/`LOAD` mark and load classes) or **creates a script event**
through a per-keyword constructor in `uimisc3.c`'s neighbourhood (scope W,
`AddEvent_*`, each `NewScriptEvent(kind, mode)` + link to `g_script_cur`).
The event's `kind` indexes `g_event_tick[]` (0x004b9d44, fpui3.c) — the tick
handlers are scopes V and X. So: handler → `AddEvent_<KW>` (kind N) →
`EventTick_<KW>`. The keyword→kind map is in every sibling brief's table;
the keywords are: `none`, `AGES`, `[INIT]`, `[OBJECTIVE]`, `[ONEOFF]`, `[ONGOING]`, `[PERMANENT]`, `[REMINDER]`, `[REWARD]`, `[END]`, `MAP`, `LOAD`, `ENABLE`, `CURRENCY`, `HAPPINESS_ENV`, `LOOKAT`, `BREIFINGFILE`, `BRIEFINGFILE`, `HINTSFILE`, `BLUEPRINT`, `GARDENER`, `MECHANIC`, `WORKERS`, `MAXBLOKES`, `HAP_FACTOR`, `CAPACITYSCALE`, `CAPACITYCAP`, `FLASHBUTTON`, `FLASHBUTTOFF`, `PROMPT`, `INTRO`, `NEED`, `NEEDAT`, `NEEDIN`, `CONNECT`, `LINK`, `RANGE`, `CLEARAREA`, `REMOVE`, `REMOVERANGE`, `COMPOSITE`, `LOOPCOMPOSITE`, `TECHLEVEL`, `RESEARCH`, `PARKVISITORS`, `RIDEVISITORS`, `RIDERS`, `SCENERYCOVERAGE`, `PATHSCENERY`, `RIDECOVERAGE`, `SHOPCOVERAGE`, `FOODCOVERAGE`, `TOTCOVERAGE`, `APPRAISAL`, `STUDAREA`, `SAVE`, `HAPPINESS`, `NEEDGARDENERS`, `NEEDMECHANICS`, `HUNGER`, `FIXRIDES`, `POWERRIDES`, `ZONING`, `CHECKFLAG`, `SELECTTHEME`, `SELECTTAB`, `SELECTMODE`, `FOREVER`, `GIVE`, `TAKE`, `ADDBRICKS`, `PLACE`, `CLEAR`, `UNGLUE`, `GLUE`, `EXTENDPARK`, `FMV`, `INTERVAL`, `MESSAGE`, `FEATURE`, `REPORT`, `DEGRADE`, `MAXCAPACITY`, `MAXVISITORS`, `MINCAPACITY`, `MINVISITORS`, `ENTRANCEFEE`, `THEMEICON`, `ADDFLAG`, `BRIDGES`, `ENDSCREENS`, `PURGE`, `ENDLEVEL`.

Section keywords (`[INIT]`, `[OBJECTIVE]`, `[ONEOFF]`, `[ONGOING]`,
`[PERMANENT]`, `[REMINDER]`, `[REWARD]`, `[END]`) open and close script
steps (`BeginScriptStep` 0x004787f0 / `EndScriptStep` 0x004787d0 wrap
`NewScriptStep` and the step-list free at 0x0046b590). Names below are the
integrator's reading of callees and strings; rename from the body and
record every rename. The handler names are `LevelKw_<KEYWORD>`; keep them.

## `LEGOLAND/levelkw.c`

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x004781b0 | `LookupNamedIndex` | 33 | called by 0x00444c40 [unmatched] (+11 more). calls `_stricmp` |
| 0x00478280 | `ParseKeywordSections` | 194 | called by ParseKeywordFile (movie3.c). calls `0x00489e60` x2, `progress_tick`, `0x004a0050`, `0x00478110`, `0x00499300`; strings "none", "check"; globals `0x00668fcc` |
| 0x004785d0 | `CurLevelSection` | 21 | called by 0x00478a80 [unmatched] (+4 more). globals `0x00669058`, `g_level_number` |
| 0x00478610 | `CurLevelFlags` | 16 | called by 0x00478a40 [unmatched] (+4 more). globals `g_level_byte_669050` |
| 0x00478650 | `IsPurgeLine` | 18 | called by 0x00478a40 [unmatched] (+1 more). calls `_stricmp`; strings "PURGE"; globals `g_level_byte_669050` |
| 0x00478690 | `KwHasArgs` (settled by scope T: `argc >= nargs`) | 6 | called by 0x004786c0 [unmatched] (+2 more).  |
| 0x004786a0 | `KwSectionMatches` (settled by scope T: `(g_level_number & mask) != 0`) | 7 | called by 0x0047af80 [unmatched] (+8 more). globals `g_level_number` |
| 0x004786c0 | `KwLineApplies` (settled by scope T: `(g_level_number & mask) && argc >= nargs`) | 27 | called by 0x0047aea0 [unmatched] (+75 more). calls `0x004786a0`, `0x00478690` |
| 0x00478700 | `ParseRectArgs` | 38 | called by 0x0047a7b0 [unmatched] (+6 more). calls `0x004a04b9` x4 |
| 0x00478770 | `ParsePosArgs` | 19 | called by 0x0047a5a0 [unmatched] (+1 more). calls `0x004a04b9` x2 |
| 0x004787a0 | `sub_4787a0` | 3 | called by 0x00478be0 [unmatched] (+2 more).  |
| 0x004787b0 | `LevelKw_none` | 14 | table at 0x004bb6fc in .data.  |
| 0x004787d0 | `EndScriptStep` | 8 | called by 0x00478930 [unmatched] (+1 more). calls `0x0046b590`; globals `g_script_cur` |
| 0x004787f0 | `BeginScriptStep` | 10 | called by 0x00478930 [unmatched]. calls `NewScriptStep`; globals `g_level_int_669098`, `g_script_cur`, `g_script_root` |
| 0x00478820 | `LevelKw_Uninitialised` | 6 | unreferenced (swept) **DEAD**. calls `0x004785d0`; strings "Uninitialised" |
| 0x00478840 | `LevelKw_END` | 8 | table at 0x004bb744 in .data. calls `0x004785d0`, `0x004787d0`; strings "Closed"; globals `g_script_cur` |
| 0x00478870 | `LevelKw_INIT` | 8 | table at 0x004bb70c in .data. calls `0x004785d0` |
| 0x00478890 | `LevelKw_AGES` | 49 | table at 0x004bb704 in .data. calls `0x004a04b9` x3; globals `g_cur_80ffc0` |
| 0x00478930 | `LevelKw_OBJECTIVE` | 27 | table at 0x004bb714 in .data. calls `0x004786a0`, `0x004785d0`, `0x00478610`, `0x004787d0`, `0x004787f0` |
| 0x00478980 | `LevelKw_ONEOFF` | 18 | table at 0x004bb71c in .data. calls `0x004786a0`, `0x00478610` |
| 0x004789c0 | `LevelKw_ONGOING` | 18 | table at 0x004bb724 in .data. calls `0x004786a0`, `0x00478610` |
| 0x00478a00 | `LevelKw_PERMANENT` | 27 | table at 0x004bb72c in .data. calls `0x004786a0`, `0x00478610`, `0x00478650` |
| 0x00478a40 | `LevelKw_REMINDER` | 27 | table at 0x004bb734 in .data. calls `0x004786a0`, `0x00478610`, `0x00478650` |
| 0x00478a80 | `LevelKw_REWARD` | 23 | table at 0x004bb73c in .data. calls `0x004786a0`, `0x004785d0` |
| 0x00478ac0 | `LevelKw_MAP` | 37 | table at 0x004bb74c in .data. calls `0x00478690`, `0x004787a0`, `ClearObjectCounters`, `LoadBaseMap`, `CalculateMapRenderOrder`, `RecheckPoweredObjects` |
| 0x00478b70 | `LevelKw_LOAD` | 34 | table at 0x004bb754 in .data. calls `0x004786c0`, `0x004787a0`, `EnsureObjectClassLoaded` |
| 0x00478bc0 | `LevelKw_BLUEPRINT` | 9 | table at 0x004bb794 in .data. calls `0x00478be0` |
| 0x00478be0 | `LevelKw_ENABLE` | 54 | called by 0x00478bc0 [unmatched]. calls `0x004786c0`, `0x004787a0`, `EnsureObjectClassLoaded`, `ElemID`, `MarkElemAvailable`, `0x0047a480`; globals `g_level_number` |
| 0x00478c60 | `LevelKw_CURRENCY` | 36 | table at 0x004bb764 in .data. calls `0x004786c0`, `0x004a04b9` x2, `0x00457900`, `0x0046b850`; globals `g_level_number` |
| 0x00478cd0 | `LevelKw_HAPPINESS_ENV` | 34 | table at 0x004bb76c in .data. calls `0x004786c0`, `0x004a04b9`; globals `g_level_number`, `g_rate_t0`, `g_mood_adjustments` |
| 0x00478d30 | `LevelKw_LOOKAT` | 77 | table at 0x004bb774 in .data. calls `0x004786c0`, `0x00478770`, `GetTileDimensions`, `0x0046bda0`; globals `g_level_number`, `g_scroll_x`, `g_scroll_y` |
| 0x00478e20 | `LevelKw_BREIFINGFILE_BRIEFINGFILE` | 42 | table at 0x004bb77c in .data. calls `0x004786c0`, `0x004687f0`, `0x0046bd10`; globals `g_alloc_tag`, `g_level_number` |
| 0x00478e90 | `LevelKw_HINTSFILE` | 42 | table at 0x004bb78c in .data. calls `0x004786c0`, `0x00468810`, `0x0046bd40`; globals `g_alloc_tag`, `g_level_number` |
| 0x00478f00 | `LevelKw_WORKERS` | 57 | table at 0x004bb7ac in .data. calls `0x004786c0`, `0x004a04b9` x2, `0x0046bb10`; globals `g_level_number` |
| 0x00478fa0 | `LevelKw_GARDENER` | 66 | table at 0x004bb79c in .data. calls `0x004786c0`, `0x004a04b9` x3, `GenerateGardener`, `0x0046bad0`; globals `g_level_number` |
| 0x00479060 | `LevelKw_MECHANIC` | 66 | table at 0x004bb7a4 in .data. calls `0x004786c0`, `0x004a04b9` x3, `GenerateMechanic`, `0x0046bad0`; globals `g_level_number` |
| 0x00479120 | `LevelKw_PROMPT` | 49 | table at 0x004bb7e4 in .data. calls `0x004786c0`, `NewScriptEvent` x2; globals `g_script_root` |
| 0x004791a0 | `LevelKw_INTRO` | 27 | table at 0x004bb7ec in .data. calls `0x004786c0`, `0x0046b650`; globals `g_script_cur` |
| 0x004791f0 | `LevelKw_NEED` | 50 | table at 0x004bb7f4 in .data. calls `0x004786c0`, `ElemID`, `0x004a04b9`, `0x0046bdd0`; globals `g_level_byte_669050` |
| 0x00479270 | `LevelKw_NEEDAT` | 50 | table at 0x004bb7fc in .data. calls `0x004786c0`, `ElemID`, `0x004a04b9` x2, `0x0046be00`; globals `g_level_byte_669050` |
| 0x00479300 | `LevelKw_NEEDIN` | 54 | table at 0x004bb804 in .data. calls `0x004786c0`, `ElemID`, `0x004a04b9`, `0x00478700`, `0x0046be40`; globals `g_level_byte_669050` |
| 0x00479390 | `LevelKw_CONNECT` | 30 | table at 0x004bb80c in .data. calls `0x004786c0`, `ElemID`, `0x0046be90`; globals `g_level_byte_669050` |
| 0x004793e0 | `LevelKw_LINK` | 39 | table at 0x004bb814 in .data. calls `0x004786c0`, `_stricmp`, `0x0046bec0`, `ElemID`; strings "ALL"; globals `g_level_byte_669050` |
| 0x00479450 | `LevelKw_RANGE` | 56 | table at 0x004bb81c in .data. calls `0x004786c0`, `ElemID`, `0x004a04b9` x2, `0x0046bef0`; globals `g_level_byte_669050` |
| 0x004794d0 | `LevelKw_CLEARAREA` | 46 | table at 0x004bb824 in .data. calls `0x004786c0`, `0x00478700`, `0x004a04b9`, `0x0046bf30`; globals `g_level_byte_669050` |
| 0x00478110 | `sub_478110` | ? | called by `ParseKeywordSections`; not probed (outside the inventory groups) — disassemble and name |
| 0x00489e60 | `sub_489e60` | ? | called by `ParseKeywordSections`; not probed (outside the inventory groups) — disassemble and name |
| 0x00499300 | `sub_499300` | ? | called by `ParseKeywordSections`; not probed (outside the inventory groups) — disassemble and name |

**Order:** the primitives (smallest first), then the handlers in address
order — they are one shape (`KwLineApplies`, convert, call the constructor),
so build one, then transfer. `ParseKeywordSections` (194i) last.

## Owned elsewhere — do not create or edit

`levelkw2.c`, `levelkw3.c`, `startup.c` (scopes S, T); `eventmake.c` (W);
`eventtick.c`, `eventtick2.c`, `eventgoal.c` (V, X); `exceptlog.c`,
`objdesc.c` (U); `coaster10.c`, `ridemachine2.c`, `uistubs2.c` (Codex-F);
every file in scopes F, G, H; every existing `.c`.
