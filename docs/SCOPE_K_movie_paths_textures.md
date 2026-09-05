# Scope K — movie player, path masks, textures (2026-09-05)

> **Status: DONE — 28 of 28 exact, merged into `main` 2026-09-05 (branch `scope/K`).**

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here.** Branch: `scope/K`. Notes: `docs/lanes/scope-k.md`. Object prefix:
`/tmp/sk_`. Any agent may take this scope.

NEW-FUNCTION scope, ≈1,570 instructions — the tier of callees that the movie
player, cursor-tile painters and texture loaders just exposed.

## `LEGOLAND/movie.c` — the movie player and front-end teardown (≈730 insns)

`uimisc2.c` (`PlayMovie`, `SetReportMovie`, `RunLevelEndSequence` — exact)
is the caller of most of these and documents the AVI path (`"FMV\\" + name`,
the retry through `g_res_path`, the pushed/popped front-end state);
`uimisc3.c` (`AdvertGoBackInput` calls `RestoreFrontEndState`), `uimisc.c`,
`mapscreen2.c`..`mapscreen4.c`, `audio4.c`/`audio5.c` (narration buffers),
`wndenv.c`/`surface.c` (how matched code spells COM vtable calls).

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x004766f0 | `RunMovie` | 170 | uimisc2.c |
| 0x00476460 | `OpenMovie` | 142 | uimisc2.c |
| 0x004989b0 | `RewindNarrationBuffer` | 98 | uimisc3.c |
| 0x00490fa0 | `PrintCursor` | 84 | uimisc2.c |
| 0x004911c0 | `SetInfoPanelText` | 32 | uimisc2.c |
| 0x0048ab60 | `sub_48ab60` | 27 | uimisc2.c |
| 0x00476630 | `CloseMovie` | 27 | uimisc2.c |
| 0x004907a0 | `LoadHelpTextFor` | 26 | uimisc2.c |
| 0x0048fa40 | `RestoreFrontEndState` | 20 | uimisc3.c |
| 0x0047afb0 | `LoadLevelDatabase` | 20 | uimisc2.c |
| 0x00490270 | `KillCertScreenSprites` | 18 | uimisc3.c |
| 0x00473160 | `ClosePrimaryPopUp` | 18 | tinystubs.c |
| 0x00458940 | `sub_458940` | 16 | uimisc2.c |
| 0x00473130 | `CloseInfoPopUpIfOpen` | 13 | uimisc3.c |
| 0x0048ffb0 | `KillAdvertScreenSprites` | 11 | uimisc3.c |
| 0x00475fe0 | `SetMenuHelp` | 8 | tinystubs.c |

`RestoreFrontEndState` carries a known original bug (it latches
`g_screen_mode` at entry and writes it back OVER `g_cur_screen`, discarding
the saved screen index) — reproduce it. Name the two `sub_*`.

## `LEGOLAND/pathmask.c` — path edge/corner masks and cursor tiles (≈420 insns)

`render5.c` (`DrawPathTileOverlay`, `PaintCursorTiles`, `ExpireCachedText` —
exact; its notes record how `PathEdgeMask`'s `char` result rides a dead
parameter slot as a raw dword) and `pathmisc2.c` (`GrowPathRectSide` — the
caller of `IsPathRectClear`, named there) are the callers.

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x0045ceb0 | `PathEdgeMask` | 149 | render5.c |
| 0x0045d080 | `PathCornerMask` | 99 | render5.c |
| 0x0045c900 | `IsPathRectClear` | 70 | pathmisc2.c |
| 0x00461080 | `DrawCursorTileAt` | 43 | render5.c |
| 0x00455ee0 | `FreeCachedTextEntry` | 41 | render5.c |
| 0x00482a40 | `ResolveEntrancePathSquare` | 20 | tinystubs.c |

## `LEGOLAND/texture.c` — texture and detail-image records (≈420 insns)

`savemisc2.c` (`LoadTextureImage`, `RegisterTextureImage`,
`UnregisterDetailImage`, `GetObjRiderN` — exact), `data3.c`
(`LookupTextureName`, `LoadLocTextures`), `rin.c`, `sprite2.c`.

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x004434d0 | `ConvertSourceImage` | 192 | savemisc2.c |
| 0x004437d0 | `BuildTextureRecord` | 136 | savemisc2.c |
| 0x00496f30 | `DetailImage_AllocSlot` | 50 | tinystubs.c |
| 0x00496ff0 | `FindDetailImageSlot` | 16 | savemisc2.c |
| 0x00441890 | `ObjNextRider` | 14 | savemisc2.c |
| 0x00441870 | `ObjFirstRider` | 9 | savemisc2.c |

**Order:** `texture.c` smallest first → `pathmask.c` smallest first →
`movie.c` smallest first, with `OpenMovie`/`RunMovie` last (expect COM
vtable calls: a call result handed to a COM method must be a NAMED local;
`mov esi,[__imp__X] / call esi` in a loop needs no construct).

## Owned elsewhere — do not create or edit

`ridemachine.c`, `coaster9.c` (Codex scope E); `lfmisc2.c`, `musicthread.c`
(a running lane); every file in scopes F–J; every existing `.c`.
