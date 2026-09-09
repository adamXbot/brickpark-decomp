# Scope AC — advisor movies and InitMan texture/rider helpers (2026-09-07)

> **Status 2026-09-09: AC is merged at 14 of 15.** `PutOne3DBlokeOnRide`
> (0x00441910) closed 2026-09-09 with the Codex-F levers. The last body,
> `LoadAltTextures` (0x00442980), is now claimed by
> `docs/SCOPE_LL22_partials_bytediff.md` — do NOT assign it from here.

> **Status: DONE — 13 of 15 exact, merged into `main` 2026-09-07 (integrator
> session; merge `f2ff6920`).** Two honest WIPs remain in `mantex.c`
> (`PutOne3DBlokeOnRide`, `LoadAltTextures`). Branch `scope/AC`. Notes:
> `docs/lanes/scope-ac.md`. Object prefix `/tmp/sac_`. Cut from inventory
> group 9 live members in `0x00441910..0x004441f0` that scope Y did not take.

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including the relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **15 functions, ≈860 instructions**, two new files.
Scope Y already owns the 25 report setters and appraisal helpers from
0x004442c0 upward — **do not recreate those**. Skip 0x004453a0.

## What this tier is

Two neighbourhoods that sit under `InitMan` / the advisor UI:

1. **Alt-texture / rider helpers** called from `data2.c`'s `InitMan` and
   `rides.c`'s `Put3DBlokesOnRide`. `LoadAltTextures` is already named at
   0x00442980 in `data2.c`.
2. **Advisor movie** control — `gamemain.c` names `InitAdvisorMovies`
   (0x00444090) and `KillAdvisorMovies` (0x00444150); `screens3.c` /
   `tinystubs.c` name neighbouring pose helpers already matched.

## `LEGOLAND/mantex.c` — InitMan / rider texture helpers (≈447 insns)

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x00441910 | `sub_441910` | 35 | called by 0x00441980 |
| 0x00441980 | `sub_441980` | 81 | `Put3DBlokesOnRide` (rides.c) |
| 0x004427e0 | `sub_4427e0` | 57 | called by `LoadAltTextures` |
| 0x00442860 | `sub_442860` | 45 | called by `LoadAltTextures` |
| 0x00442980 | `LoadAltTextures` | 229 | `InitMan` (data2.c) — authoritative name |

`SkipStrings` (0x004428c0) and `LookupTextureName` (0x004428f0) are already
exact in `savemisc2.c` / `data3.c` — declare them `extern`, do not redefine.

## `LEGOLAND/advisor.c` — advisor movie / clip control (≈411 insns)

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x00443bd0 | `sub_443bd0` | 123 | called by 0x00444090 |
| 0x00443d50 | `sub_443d50` | 22 | called by `KillAdvisorMovies` |
| 0x00443d90 | `sub_443d90` | 5 | called by 0x00444090 |
| 0x00443dc0 | `StartAdvisorClip` | 33 | `RenderAdvisorIcon` (screens3.c notes) |
| 0x00443f90 | `sub_443f90` | 17 | called by 0x00444020 |
| 0x00443fe0 | `sub_443fe0` | 15 | called by 0x00444020 |
| 0x00444020 | `sub_444020` | 17 | pointer in 0x00444090 |
| 0x00444090 | `InitAdvisorMovies` | 51 | gamemain.c — AD_Blink/AD_LR/AD_Phone.avi |
| 0x00444150 | `KillAdvisorMovies` | 46 | gamemain.c |
| 0x004441f0 | `ClearReportState` | 2 | movie3.c |

`SetAdvisorPose` (0x00444070) and `RenderScriptEndIcon` (0x00443e30) are
already exact — declare `extern` only.

**Order:** `mantex.c` smallest-first with `LoadAltTextures` last →
`advisor.c` stubs → `InitAdvisorMovies` / `KillAdvisorMovies`.

## Owned elsewhere — do not create or edit

`reportset.c`, `appraisal.c` (Y), `data2.c`, `data3.c`, `savemisc2.c`,
`tinystubs.c`, `screens3.c`, `gamemain.c`, `movie3.c`, F/G/H, V, AA, AB,
Codex-F's files, every existing `.c`.
