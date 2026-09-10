# LL wave index (2026-09-07)

Letter scopes ended at AK. New scopes are `LL1`…`LLN`.
Cut from live unmatched inventory after excluding V/X, Codex-F, AG, AC,
F/G/H partials, LONG appraisal, and SEH WinMain.

| scope | branch | inventory | fns | insns | file | status |
| --- | --- | --- | ---: | ---: | --- | --- |
| LL1 | `scope/LL1` | 1 | 22 | 861 | `logflume8.c` | DONE 22/22 merged |
| LL2 | `scope/LL2` | 2 | 6 | 602 | `logflume9.c` | DONE 6/6 merged |
| LL3 | `scope/LL3` | 3 | 19 | 1091 | `coaster11.c` | **MERGED 2026-09-09, 16/19** — ClipPlane FLOOR 38.2%; Mass 65/77; Trace NG22 |
| LL4 | `scope/LL4` | 4 | 8 | 797 | `coastershade2.c` | **MERGED 2026-09-09, 3/8** — the ZBuffer-class Span_Fill* family; Simpson 2 mism |
| LL5 | `scope/LL5` | 5 | 3 | 130 | `castletrack2.c` | DONE 3/3 merged |
| LL6 | `scope/LL6` | 6 | 24 | 771 | `coaster12.c` | **MERGED 2026-09-09, 22/24** — GetTrackSegment FLOOR; AddSpanRecord FLOOR 80.3% ebx↔lea |
| LL7 | `scope/LL7` | 7 | 17 | 999 | `coaster13.c` | **MERGED 2026-09-09, 16/17** — FillPoly FLOOR 69.3% nshade eax/edx |
| LL8 | `scope/LL8` | 12+13+14 | 13 | 514 | `gameframe2.c` | DONE 13/13 merged |

**Note:** `logflume7.c` already exists on main — LL1 starts at `logflume8.c`.

**ALL OF LL1–LL8 ARE MERGED (2026-09-09/10).** Counts above are the state on
`main`, verified against the markers in each file. The wave continued past LL8:
LL9–LL15 wrote the binary's dead functions into `unref1.c`..`unref7.c`, LL16
wrote `appraisalscreen.c` (the 8,085-instruction appraisal screen), and
LL17–LL20 reopened partials on main — see `docs/HANDOFF.md` §1. The branches
have been deleted; their evidence is in `docs/lanes/scope-ll*.md`.

**The unwritten-function frontier is now empty** — every function in the game
range has a body. All remaining work is WIP-to-exact in `docs/HANDOFF.md` §6B.
