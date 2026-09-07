# Scope AK — narration / sample-volume / LoadStrings (inventory group 18) (2026-09-07)

> **Status: DONE — 20 of 20 exact, closed into `main` 2026-09-07.** Branch
> `scope/AK`. Notes: `docs/lanes/scope-ak.md`. Object prefix `/tmp/sak_`.
> Cut from inventory group 18 live members.

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.
**Do not add any Co-Authored-By / Co-authored-by trailer.**

NEW-FUNCTION scope, **20 functions, ≈980 instructions**, one or two new
files. Neighbours: `audio5.c`, `audio2.c`, `audiomisc.c`, `sprite2.c`,
`tinystubs.c`, `startup.c` (`LoadStrings`).

## `LEGOLAND/narration2.c`

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x00496760 | `sub_496760` | 33 | near RefreshSampleVolumes (audio5.c) |
| 0x004967f0 | `sub_4967f0` | 73 | near RefreshSampleVolumes (audio5.c) |
| 0x004968d0 | `sub_4968d0` | 32 | near RefreshSampleVolumes (audio5.c) |
| 0x00496920 | `sub_496920` | 64 | near UnSourcePlayableSample (audio2.c) |
| 0x004969d0 | `sub_4969d0` | 4 | near UnSourcePlayableSample (audio2.c) |
| 0x00496e60 | `sub_496e60` | 53 | near Kill_FXList (audiomisc.c) |
| 0x004975b0 | `sub_4975b0` | 38 | near NewSprite (sysstubs.c) |
| 0x00497f60 | `sub_497f60` | 15 | near TellAllLayersToStopAnimating (sprite2.c) |
| 0x00497f90 | `sub_497f90` | 7 | near TellAllLayersToStopAnimating (sprite2.c) |
| 0x00497fb0 | `sub_497fb0` | 25 | near TellAllLayersToStopAnimating (sprite2.c) |
| 0x00498000 | `sub_498000` | 73 | near TellAllLayersToStopAnimating (sprite2.c) |
| 0x00498100 | `sub_498100` | 5 | near RewindNarrationSource (tinystubs.c) |
| 0x00498150 | `sub_498150` | 49 | near RewindNarrationSource (tinystubs.c) |
| 0x004981e0 | `sub_4981e0` | 15 | near RewindNarrationSource (tinystubs.c) |
| 0x00498210 | `sub_498210` | 7 | near RewindNarrationSource (tinystubs.c) |
| 0x00498230 | `sub_498230` | 5 | near RewindNarrationSource (tinystubs.c) |
| 0x00498250 | `RefillNarrationRing` | 102 | declared from movie3.c |
| 0x00498b40 | `sub_498b40` | 142 | called by VolMarkerInput (screens3.c) (+4) |
| 0x00498d00 | `LoadStrings` | 192 | declared from startup.c / InitSession |
| 0x00498f80 | `sub_498f80` | 46 | near GetString (text.c) |

**Order:** volume/FX stubs → narration rewind family → `RefillNarrationRing`
→ `sub_498b40` → `LoadStrings` last.

## Owned elsewhere — do not create or edit

`audio5.c`, `audio2.c`, `sprite2.c`, `startup.c`, `movie3.c`, scopes F/G/H,
V, AC, AG, Codex-F, every existing `.c`.
