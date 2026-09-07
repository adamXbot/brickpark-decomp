# Scope AJ — RES volume + frontend save / sample helpers (inventory group 17) (2026-09-07)

> **Status: COMPLETE 2026-09-07 — 27/27 exact (`audit [OK]`), relocs clean, `/W3` clean.** Branch `scope/AJ`.
> Notes: `docs/lanes/scope-aj.md`. Object prefix `/tmp/saj_`. Cut from
> inventory group 17 live members.

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.
**Do not add any Co-Authored-By / Co-authored-by trailer.**

NEW-FUNCTION scope, **27 functions, ≈853 instructions**, preferably **two**
new files (RES/audio sample + frontend/save UI). Neighbours: `audio3.c`,
`audio4.c`, `data2.c`, `screens2.c`, `screens3.c`, `profiles.c`.

## `LEGOLAND/resaudio2.c` — volume open + WAV sample + tiny sound stubs (≈316)

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x004895a0 | `sub_4895a0` | 147 | called by RES_OpenVolume (data2.c) |
| 0x004921c0 | `sub_4921c0` | 155 | called by CreateSampleFromWAV (data2.c) |
| 0x00492c60 | `sub_492c60` | 7 | near KillSoundSampleSystem (lifecycle.c) |
| 0x00492c80 | `sub_492c80` | 7 | near SetThemeInTransition (tinystubs.c) |

## `LEGOLAND/frontend2.c` — profile/save/option UI + print helpers (≈537)

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x00489f90 | `sub_489f90` | 17 | near UnmarkObjectTiles (pathmisc2.c) |
| 0x00489fd0 | `sub_489fd0` | 16 | near AddInstanceToList (sweep4.c) |
| 0x0048a040 | `sub_48a040` | 22 | near AddInstanceToList (sweep4.c) |
| 0x0048a6e0 | `sub_48a6e0` | 43 | near ClipThisRect (util.c) |
| 0x0048a750 | `sub_48a750` | 15 | near RestoreFreePlaySelections (uimisc.c) |
| 0x0048a780 | `sub_48a780` | 1 | near RestoreFreePlaySelections (uimisc.c) |
| 0x0048a800 | `sub_48a800` | 20 | near FreePlayItemUpdate (fpui5.c) |
| 0x0048c5e0 | `sub_48c5e0` | 23 | near EnterNewProfileCheckBoxIcons (profiles.c) |
| 0x0048c860 | `sub_48c860` | 80 | near InitProfileCheckBoxIcons (screens2.c) |
| 0x0048d470 | `sub_48d470` | 5 | near ProfileCloseInput (screens3.c) |
| 0x0048d490 | `sub_48d490` | 5 | near InitSavedGameScreen (bigscreens.c) |
| 0x0048e0c0 | `sub_48e0c0` | 52 | near DeleteSavedGameList (listdel.c) |
| 0x0048e3d0 | `sub_48e3d0` | 28 | near SaveSlotInput (screens3.c) |
| 0x0048e420 | `sub_48e420` | 12 | near SaveSlotInput (screens3.c) |
| 0x0048e450 | `sub_48e450` | 15 | near SaveSlotInput (screens3.c) |
| 0x0048eac0 | `sub_48eac0` | 12 | near InitOptionScreen (screens2.c) |
| 0x0048eaf0 | `sub_48eaf0` | 12 | near InitOptionScreen (screens2.c) |
| 0x0048eb20 | `sub_48eb20` | 5 | near InitOptionScreen (screens2.c) |
| 0x0048eb40 | `sub_48eb40` | 5 | near InitOptionScreen (screens2.c) |
| 0x0048f9f0 | `sub_48f9f0` | 18 | near RestoreFrontEndState (movie.c) |
| 0x0048fc30 | `sub_48fc30` | 5 | near InitTitleScreen (screens2.c) |
| 0x00491540 | `sub_491540` | 5 | near UpDateCurrentSaveSlotInfo (profiles.c) |
| 0x00491e40 | `sub_491e40` | 121 | called by EnterSaveGameDetails (screens2.c) |

**Order:** tiny stubs → medium UI → `0x004895a0` / `0x004921c0` /
`0x00491e40` last.

## Owned elsewhere — do not create or edit

`data2.c`, `audio3.c`, `audio4.c`, `screens2.c`, `screens3.c`, scopes F/G/H,
V, AC, AG, Codex-F, every existing `.c`.
