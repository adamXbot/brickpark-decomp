# Scope S — the level-database keyword tier, part 2 (2026-09-06)

> **Status: DONE — 37 of 37 exact, merged into `main` 2026-09-06 (integrator session; the three shared primitives were renamed at merge to T's `KwLineApplies` / `KwHasArgs` / `NameCompare`, code unchanged).** Branch `scope/S`. Notes: `docs/lanes/scope-s.md`.
> Object prefix `/tmp/ss_`. Any agent. Cut from inventory groups 22–23. Read
> `docs/SCOPE_R_level_keywords_1.md`'s "What this tier is" first — this is
> the same shape, the next 37 handlers in address order.

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including the relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **37 functions, ≈1561 instructions**, one
new file `LEGOLAND/levelkw2.c`. Every function is a keyword handler: read
the arguments (`KwLineApplies` 0x004786c0, `atoi` 0x004a04b9, `ElemID`,
`LookupNamedIndex` 0x004781b0 — declare them `extern`, scope R defines them),
then call the keyword's constructor (`AddEvent_<KW>` in scope W — declare
`extern` with the address from the table below). The goal keywords
(`NEED`…`SELECTMODE`) all read `g_level_byte_669050` first.

## `LEGOLAND/levelkw2.c`

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x00479550 | `LevelKw_REMOVE` | 48 | table at 0x004bb82c in .data. calls `0x004786c0`, `0x00478690`, `ElemID`, `0x004a04b9`, `0x0046bf80`; globals `g_level_byte_669050` |
| 0x004795c0 | `LevelKw_REMOVERANGE` | 56 | table at 0x004bb834 in .data. calls `0x004786c0`, `ElemID`, `0x004a04b9` x2, `0x0046bfb0`; globals `g_level_byte_669050` |
| 0x00479640 | `LevelKw_COMPOSITE` | 59 | table at 0x004bb83c in .data. calls `0x004786c0`, `ElemID`, `0x004a04b9` x2, `0x0046bff0`; globals `g_level_byte_669050` |
| 0x004796d0 | `LevelKw_LOOPCOMPOSITE` | 43 | table at 0x004bb844 in .data. calls `0x004786c0`, `ElemID`, `0x004a04b9`, `0x0046c030`; globals `g_level_byte_669050` |
| 0x00479740 | `LevelKw_TECHLEVEL` | 40 | table at 0x004bb84c in .data. calls `0x004786c0`, `ElemID`, `0x004a04b9`, `0x0046c060`; globals `g_level_byte_669050` |
| 0x004797b0 | `LevelKw_RESEARCH` | 32 | table at 0x004bb854 in .data. calls `0x004786c0`, `ElemID`, `0x004a04b9` |
| 0x00479800 | `LevelKw_PARKVISITORS` | 27 | table at 0x004bb85c in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046c090`; globals `g_level_byte_669050` |
| 0x00479850 | `LevelKw_RIDEVISITORS` | 40 | table at 0x004bb864 in .data. calls `0x004786c0`, `ElemID`, `0x004a04b9`, `0x0046c0c0`; globals `g_level_byte_669050` |
| 0x004798c0 | `LevelKw_RIDERS` | 40 | table at 0x004bb86c in .data. calls `0x004786c0`, `ElemID`, `0x004a04b9`, `0x0046c0f0`; globals `g_level_byte_669050` |
| 0x00479930 | `LevelKw_SCENERYCOVERAGE` | 27 | table at 0x004bb874 in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046c120`; globals `g_level_byte_669050` |
| 0x00479980 | `LevelKw_PATHSCENERY` | 27 | table at 0x004bb87c in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046c150`; globals `g_level_byte_669050` |
| 0x004799d0 | `LevelKw_RIDECOVERAGE` | 27 | table at 0x004bb884 in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046c180`; globals `g_level_byte_669050` |
| 0x00479a20 | `LevelKw_SHOPCOVERAGE` | 27 | table at 0x004bb88c in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046c1b0`; globals `g_level_byte_669050` |
| 0x00479a70 | `LevelKw_FOODCOVERAGE` | 27 | table at 0x004bb894 in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046c1e0`; globals `g_level_byte_669050` |
| 0x00479ac0 | `LevelKw_TOTCOVERAGE` | 27 | table at 0x004bb89c in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046c210`; globals `g_level_byte_669050` |
| 0x00479b10 | `LevelKw_APPRAISAL` | 115 | table at 0x004bb8a4 in .data. calls `0x004786c0`, `0x004a04b9`, `SetLevelGoalState`; globals `g_alloc_tag` |
| 0x00479c40 | `LevelKw_STUDAREA` | 37 | table at 0x004bb8ac in .data. calls `0x004786c0`, `0x00478700`, `0x004a04b9`, `0x0046c240`; globals `g_level_byte_669050` |
| 0x00479cb0 | `LevelKw_SAVE` | 27 | table at 0x004bb8b4 in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046c290`; globals `g_level_byte_669050` |
| 0x00479d00 | `LevelKw_HAPPINESS` | 37 | table at 0x004bb8bc in .data. calls `0x004786c0`, `0x004a04b9` x2, `0x0046c2c0`; globals `g_level_byte_669050` |
| 0x00479d60 | `LevelKw_NEEDGARDENERS` | 27 | table at 0x004bb8c4 in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046c2f0`; globals `g_level_byte_669050` |
| 0x00479db0 | `LevelKw_NEEDMECHANICS` | 27 | table at 0x004bb8cc in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046c320`; globals `g_level_byte_669050` |
| 0x00479e00 | `LevelKw_HUNGER` | 51 | table at 0x004bb8d4 in .data. calls `0x004786c0`, `0x004a04b9` x2, `0x0046c350`; globals `g_level_byte_669050` |
| 0x00479e80 | `LevelKw_FIXRIDES` | 37 | table at 0x004bb8dc in .data. calls `0x004786c0`, `0x004a04b9` x2, `0x0046c390`; globals `g_level_byte_669050` |
| 0x00479ee0 | `LevelKw_POWERRIDES` | 27 | table at 0x004bb8e4 in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046c3c0`; globals `g_level_byte_669050` |
| 0x00479f30 | `LevelKw_ZONING` | 44 | table at 0x004bb8ec in .data. calls `0x004786c0`, `0x004781b0`, `0x004a04b9`, `0x0046c3f0`; globals `g_level_byte_669050` |
| 0x00479fa0 | `LevelKw_CHECKFLAG` | 47 | table at 0x004bb8f4 in .data. calls `0x004786c0`, `0x004a04b9` x2, `0x0046c420`; globals `g_level_byte_669050` |
| 0x0047a020 | `LevelKw_THEMEICON` | 55 | table at 0x004bb9b4 in .data. calls `0x004786c0`, `0x004a04b9` x2, `0x00468860`, `0x0046bc80`; globals `g_level_number` |
| 0x0047a0b0 | `LevelKw_ADDFLAG` | 55 | table at 0x004bb9bc in .data. calls `0x004786c0`, `0x004a04b9` x2, `0x00468890`, `0x0046bcb0`; globals `g_level_number` |
| 0x0047a140 | `LevelKw_BRIDGES` | 58 | table at 0x004bb9c4 in .data. calls `0x004786c0`, `0x004a04b9` x2, `0x004688f0`, `0x0046bce0`; globals `g_level_number` |
| 0x0047a1d0 | `LevelKw_ENDSCREENS` | 108 | table at 0x004bb9cc in .data. calls `0x004786c0`, `0x004a04b9`, `SetLevelEndSequence`; globals `g_alloc_tag`, `g_level_number` |
| 0x0047a2f0 | `LevelKw_SELECTTHEME` | 44 | table at 0x004bb8fc in .data. calls `0x004786c0`, `0x004781b0`, `0x0046c450`; globals `g_level_byte_669050` |
| 0x0047a360 | `LevelKw_SELECTTAB` | 44 | table at 0x004bb904 in .data. calls `0x004786c0`, `0x004781b0`, `0x0046c480`; globals `g_level_byte_669050` |
| 0x0047a3d0 | `LevelKw_SELECTMODE` | 44 | table at 0x004bb90c in .data. calls `0x004786c0`, `0x004781b0`, `0x0046c4b0`; globals `g_level_byte_669050` |
| 0x0047a440 | `LevelKw_FOREVER` | 20 | table at 0x004bb914 in .data. calls `0x004786c0`, `0x0046c510`; globals `g_level_byte_669050` |
| 0x0047a480 | `LevelKw_GIVE` | 54 | called by 0x00478be0 [unmatched]. calls `0x004786c0`, `ElemID`, `_stricmp`, `0x0046b790`; strings "NOPOPUP" |
| 0x0047a500 | `LevelKw_TAKE` | 28 | table at 0x004bb924 in .data. calls `0x004786c0`, `ElemID`, `0x0046b7f0` |
| 0x0047a550 | `LevelKw_ADDBRICKS` | 28 | table at 0x004bb92c in .data. calls `0x004786c0`, `0x004a04b9`, `0x0046b820` |


**Order:** address order; the 27-instruction ones (`PARKVISITORS`…) are
one body with the kind changed — build one, transfer, then the larger ones
(`APPRAISAL` 115i, `ENDSCREENS` 108i) last.

## Owned elsewhere — do not create or edit

`levelkw.c`, `levelkw3.c`, `startup.c` (scopes R, T); `eventmake.c` (W);
`eventtick.c`, `eventtick2.c`, `eventgoal.c` (V, X); `exceptlog.c`,
`objdesc.c` (U); Codex-F's files; every file in scopes F, G, H; every
existing `.c`.
