# LL wave index (2026-09-07)

Letter scopes ended at AK. New scopes are `LL1`…`LLN`.
Cut from live unmatched inventory after excluding V/X, Codex-F, AG, AC,
F/G/H partials, LONG appraisal, and SEH WinMain.

| scope | branch | inventory | fns | insns | file | status |
| --- | --- | --- | ---: | ---: | --- | --- |
| LL1 | `scope/LL1` | 1 | 22 | 861 | `logflume8.c` | DONE 22/22 merged |
| LL2 | `scope/LL2` | 2 | 6 | 602 | `logflume9.c` | DONE 6/6 merged |
| LL3 | `scope/LL3` | 3 | 19 | 1091 | `coaster11.c` | 16/19 — ClipPlane latch+ebx=n; abs ecx, frame 0x28 |
| LL4 | `scope/LL4` | 4 | 8 | 797 | `coastershade2.c` | 3/8 — Simpson FLOOR 80/81; Span floors |
| LL5 | `scope/LL5` | 5 | 3 | 130 | `castletrack2.c` | DONE 3/3 merged |
| LL6 | `scope/LL6` | 6 | 24 | 771 | `coaster12.c` | 22/24 — GetTrackSegment fail1/fail2 fight; park |
| LL7 | `scope/LL7` | 7 | 17 | 999 | `coaster13.c` | 16/17 — FillPoly FLOOR (nshade eax vs edx web); park |
| LL8 | `scope/LL8` | 12+13+14 | 13 | 514 | `gameframe2.c` | DONE 13/13 merged |

**Note:** `logflume7.c` already exists on main — LL1 starts at `logflume8.c`.

**Allocated:** LL1–LL8 on Grok (no address/file conflicts between scopes).
