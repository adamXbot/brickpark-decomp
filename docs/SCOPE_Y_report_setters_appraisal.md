# Scope Y — the 25 report setters and the appraisal report screen's helper tier (2026-09-06)

> **Status: COMPLETE, 48 of 48 exact; merged 2026-09-07.** Branch `scope/Y`.
> Notes: `docs/lanes/scope-y.md`. Source completion commit `211d7a79`.
> Original brief below; corrections and measured results are in the notes.
> Object prefix `/tmp/sy_`. Cut from inventory groups 9–10 (plus
> one function of group 16) — `tools/inventory.py`, 2026-09-06, the tree at
> the R/X merge.

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including the relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **48 functions, ≈1,230 instructions**, two new files.
**The appraisal report screen itself, 0x004453a0 (8,085 instructions), is
NOT in this scope and must not be attempted** — it is the one body the
inventory marks unassignable until the matcher takes a window parameter.
This scope is everything that screen calls and the table its report
keyword drives.

## `LEGOLAND/reportset.c` — the 25 per-report setters (≈360 insns)

`SetReportMode(kind, a, b)` (0x0046a140, scope V's `eventtick.c`) is twelve
instructions: `if (kind >= 0 && kind < 25) g_report_set[kind](a, b);` through
the table at **0x004b7e38**. The 25 targets are one shape each — read
0x004443b0, 0x00444470 and 0x00444630 first:

```
mov eax,[esp+4] / mov edx,[esp+8] / mov ecx,eax / or ecx,edx / je CLEAR
  <store a (and b) into this report's slot(s)>; g_report_flags |= BIT; ret
CLEAR: g_report_flags &= ~BIT; ret
```

`g_report_flags` is the dword at **0x00665ff8**; the slots are the dwords
from 0x00665ffc up to 0x00666094 (table below); `BIT` is one bit per report
except the five rides (indices 4–8), which store `(b & 3) << (4 + 2*i)` — a
two-bit field each (`and eax,3 / shl eax,4` in 0x00444470, and so on). Two
setters (HAPPPY_VIS, HUNGRY_VIS) share the slot pair 0x00666050/54 and
differ only in the bit. The names are the report keywords in
`g_report_names[25]` (levelkw3.c, 0x004bb624), in table order, so the
provisional names below are authoritative up to spelling. Signature `void
SetReport_<NAME>(int a, int b)`; the CLEAR arm for the one-slot setters is
`and al,0xfe`-style byte narrowing (compare `scope-t`'s `unsigned short`
`|=` lever in DECOMP — measure `unsigned int` vs `unsigned char` views of
the flags word).

| index | address | provisional name | insns | slot(s) | bit |
| ---: | --- | --- | ---: | --- | --- |
| 0 | 0x004443b0 | `SetReport_ZONE_LL` | 14 | 0x00665ffc | 1 |
| 1 | 0x004443e0 | `SetReport_ZONE_ADV` | 14 | 0x00666000 | 2 |
| 2 | 0x00444410 | `SetReport_ZONE_MED` | 14 | 0x00666004 | 4 |
| 3 | 0x00444440 | `SetReport_ZONE_WES` | 14 | 0x00666008 | 8 |
| 4 | 0x00444470 | `SetReport_COASTER` | 16 | 0x0066600c | `(b&3)<<4` |
| 5 | 0x004444b0 | `SetReport_DSCHOOL` | 16 | 0x00666010 | `(b&3)<<6` |
| 6 | 0x004444f0 | `SetReport_LFLUME` | 16 | 0x00666014 | `(b&3)<<8` |
| 7 | 0x00444530 | `SetReport_BSCHOOL` | 16 | 0x00666018 | `(b&3)<<10` |
| 8 | 0x00444570 | `SetReport_JCRUISE` | 16 | 0x0066601c | `(b&3)<<12` |
| 9 | 0x004445b0 | `SetReport_NUM_ATTRACTIONS` | 15 | 0x00666020, 0x00666024 | 0x4000 (`or dh,0x40` / `and ah,0xbf`) |
| 10 | 0x004445f0 | `SetReport_VAR_ATTRACTIONS` | 15 | 0x00666028, 0x0066602c | 0x8000 (`or dh,0x80` / `and ah,0x7f`) |
| 11 | 0x00444630 | `SetReport_RIDE_ACCESS` | 13 | 0x00666030, 0x00666034 | 0x40000 |
| 12 | 0x00444670 | `SetReport_NUM_SCENERY` | 13 | 0x00666070, 0x00666074 | 0x8000000 |
| 13 | 0x004446b0 | `SetReport_VAR_SCENERY` | 13 | 0x00666078, 0x0066607c | 0x10000000 |
| 14 | 0x004446f0 | `SetReport_COV_SCENERY` | 13 | 0x00666080, 0x00666084 | 0x20000000 |
| 15 | 0x00444730 | `SetReport_NUM_FOOD` | 13 | 0x00666040, 0x00666044 | 0x10000 |
| 16 | 0x00444770 | `SetReport_VAR_FOOD` | 13 | 0x00666048, 0x0066604c | 0x20000 |
| 17 | 0x004447b0 | `SetReport_NUM_SHOPS` | 13 | 0x00666088, 0x0066608c | 0x40000000 |
| 18 | 0x004447f0 | `SetReport_VAR_SHOPS` | 13 | 0x00666090, 0x00666094 | 0x80000000 |
| 19 | 0x00444830 | `SetReport_NUM_VIS` | 13 | 0x00666038, 0x0066603c | 0x80000 |
| 20 | 0x00444870 | `SetReport_HAPPPY_VIS` | 13 | 0x00666050, 0x00666054 | 0x1000000 |
| 21 | 0x004448b0 | `SetReport_HUNGRY_VIS` | 13 | 0x00666050, 0x00666054 | 0x4000000 |
| 22 | 0x004448f0 | `SetReport_POWER` | 13 | 0x00666058, 0x0066605c | 0x200000 |
| 23 | 0x00444930 | `SetReport_WORKING_RIDES` | 13 | 0x00666060, 0x00666064 | 0x400000 |
| 24 | 0x00444970 | `SetReport_STUDDED` | 13 | 0x00666068, 0x0066606c | 0x800000 |

(The slot/bit columns were read by a script off the first 16 instructions
of each body; confirm each from the disassembly. Define the table itself
only if the gate needs it — it lives in `.data` and `SetReportMode` is V's;
declare it `extern void (*const g_report_set[25])(int, int);` with the
address in a comment if you reference it at all.)

## `LEGOLAND/appraisal.c` — what the appraisal screen calls (≈870 insns)

The 8,085-instruction screen (0x004453a0) is the caller of everything here
except where noted. Sprites it loads (0x00445190): `NextPage.lls`,
`NextPageLit.lls`, `PreviousPage.lls`, `PreviousPageLit.lls`,
`AppraisalBK.lls` into 0x0081c02c/34/80/…; its per-tick files are
`App_tick%d.lls` (0x004449b0 formats the name with the CRT `sprintf`
0x0049e573 from a table at 0x0081c054). The five 15-instruction functions
are one shape: `e = ElemID("<name>"); if (e->flags & 1) return
e->def->count_fn(e->def->count_arg, 0); return 0;` — the `+0xc0` cdecl
`(Elem*, int) -> int` loop-size callback scope X described, over the five
attraction classes (see the table). 0x004636c0 walks `g_map` (0x004bcbf4)
`+0x16` rows adding `+0x14` per row — the map's cell count.

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x004442c0 | `CountDrivingSchools` | 15 | screen; `ElemID("DRIVING SCHOOL")` + count callback |
| 0x004442f0 | `CountBoatingSchools` | 15 | screen; `ElemID("BOATING SCHOOL")` |
| 0x00444320 | `CountCastles` | 15 | screen; `ElemID("CASTLE OBJ")` |
| 0x00444350 | `CountLogFlumes` | 15 | screen; `ElemID("LOG FLUME ENTRANCE")` |
| 0x00444380 | `CountJungleCruises` | 15 | screen; `ElemID("JUNGLE CRUISE")` |
| 0x004449b0 | `LoadAppraisalTickSprites` | 60 | screen; `"App_tick%d.lls"`, `rep stosd` fill of a 0x1e-byte name buffer, table 0x0081c054 |
| 0x00444a70 | `sub_444a70` | 98 | screen; signed-value formatter (`neg ebp` arm, five params) |
| 0x00444b70 | `BlitAppraisalSprite` | 42 | screen; `PushRenderingStatusAndLockVideoSurface`, sprite table 0x0081c040 |
| 0x00444bf0 | `sub_444bf0` | 29 | screen |
| 0x00444c40 | `sub_444c40` | 13 | called by 0x00444c70 |
| 0x00444c70 | `sub_444c70` | 34 | screen; calls 0x00444c40 |
| 0x00444cd0 | `sub_444cd0` | 26 | screen |
| 0x00444d20 | `sub_444d20` | 26 | screen |
| 0x00444d70 | `sub_444d70` | 42 | screen |
| 0x00444df0 | `sub_444df0` | 74 | screen |
| 0x00444eb0 | `sub_444eb0` | 14 | called by 0x00444ef0 and 0x00445190 |
| 0x00444ef0 | `sub_444ef0` | 48 | pointer taken in 0x00445190 (a callback); calls 0x00444eb0, 0x00445310 |
| 0x00444f90 | `sub_444f90` | 27 | pointer taken in 0x00445190 (a callback); calls 0x00445310 |
| 0x00445000 | `sub_445000` | 83 | screen |
| 0x00445100 | `sub_445100` | 49 | screen |
| 0x00445190 | `LoadAppraisalScreenSprites` | 91 | screen; five `LoadSprite` calls (names above) |
| 0x00445310 | `sub_445310` | 38 | called by 0x00444ef0, 0x00444f90, 0x00445190 |
| 0x004636c0 | `MapCellCount` | 16 | screen; `g_map->h` rows × `g_map->w` |

Sizes are the inventory's; `tools/matchfull.py` prints the authoritative
extent. Name the `sub_` bodies from what they do (they are the page
buttons, the per-line renderers and the page flip of the report screen —
`screens3.c`'s exact `MapIconInput`/`RenderAdvisorIcon` family shows the
house style for input and render callbacks).

## Levers to expect

`docs/LEVERS.md` first, then the newest DECOMP folds. Expect: the 25
setters to be a first-try family once one is exact (write one, copy it);
byte-narrowed `and`/`or` on the flags word (`and al,0xfe` versus `and dword
ptr [..],0xfffbffff` in the same family — the width VC6 picks follows the
constant, not the source type: measure); the `rep stosd / stosw / stosb`
fill in 0x004449b0 is `char buf[0x1e] = "";` (S's lever); the five
`Count*` bodies are P's "one-case switch" family shape (`if (!c) return
(int)c;`-style zero returns — Q's lever).

## Owned elsewhere — do not create or edit

`eventtick.c`, `eventgoal.c` (V; `SetReportMode` 0x0046a140 lives there —
declare it, do not define it); `levelkw3.c` (`g_report_names`);
`screens3.c`, `uimisc.c`, `uimisc2.c`, `sysstubs.c`, `text.c`; the screen
0x004453a0 (nobody's); Codex-F's files; every file in scopes F, G, H; every
existing `.c`. Declare what you call `extern` with its address in a
comment, as the sibling files do.
