# Scope AI — MIDI loaders + path-square neighbourhood (inventory group 16) (2026-09-07)

> **Status: DONE — 18 of 18 exact, closed into `main` 2026-09-07.** Branch
> `scope/AI`. Notes: `docs/lanes/scope-ai.md`. Object prefix `/tmp/sai_`.
> Cut from inventory group 16 live members.

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.
**Do not add any Co-Authored-By / Co-authored-by trailer.**

NEW-FUNCTION scope, **18 functions, ≈773 instructions**, preferably **two**
new files (MIDI cluster + path/obj cluster). Neighbours: `music.c`,
`pathsq.c`, `objmap.c`, `saveprof.c`, `audio3.c`.

## `LEGOLAND/music2.c` — MIDI helpers (≈236)

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x004802c0 | `sub_4802c0` | 19 | near LoadMIDIFile (music.c) |
| 0x004802f0 | `sub_4802f0` | 27 | near LoadMIDIFile (music.c) |
| 0x00480330 | `sub_480330` | 157 | called by 0x00480570 |
| 0x00480570 | `sub_480570` | 33 | near PlayMIDI (music.c) |

## `LEGOLAND/pathobj2.c` — path-square + object-class + misc (≈537)

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x004809d0 | `sub_4809d0` | 73 | near AddNewObjectClass (objmap.c) |
| 0x00480aa0 | `sub_480aa0` | 51 | near LoadObjectClass (saveprof.c) |
| 0x00480b70 | `sub_480b70` | 12 | near LoadObjectClass (saveprof.c) |
| 0x00481720 | `sub_481720` | 2 | near NewPathSquare (pathsq.c) |
| 0x004819a0 | `CollectPathSquareNeighboursCounted` | 127 | declared from pathmask2.c |
| 0x00481f00 | `sub_481f00` | 104 | called by SuggestNextMove (bnvmove.c) |
| 0x00482a80 | `sub_482a80` | 4 | near UpdateEntranceTile (objdoor.c) |
| 0x00482b10 | `sub_482b10` | 3 | near GetEntranceTile (tinystubs.c) |
| 0x00482cb0 | `sub_482cb0` | 29 | near InitBlokeName (lfmisc.c) |
| 0x00482d70 | `sub_482d70` | 17 | near SetHappinessFactor (eventgoalprim.c) |
| 0x00482ec0 | `sub_482ec0` | 12 | near NewBloke (blokeai.c) |
| 0x00483090 | `sub_483090` | 11 | near MakeBloke (blokeai.c) |
| 0x00489440 | `sub_489440` | 59 | near RenderBox (renderlist.c) |
| 0x00489550 | `sub_489550` | 33 | near GetMasterVolPtr (audio3.c); RES_OpenFileFromVolume caller |

**Order:** music2 stubs → path stubs → `0x004819a0` / `0x00481f00` /
`0x00480330` last.

## Owned elsewhere — do not create or edit

`music.c`, `pathsq.c`, `pathmask2.c`, `objmap.c`, scopes F/G/H, V, AC, AG,
Codex-F, every existing `.c`.
