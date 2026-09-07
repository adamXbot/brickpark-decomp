# Scope AD — SoftBlitRLE recolour/highlight painters + remaining blit helpers (2026-09-07)

> **Status: DONE — 9 of 9 exact, closed into `main` 2026-09-07.** Branch
> `scope/AD`. Notes: `docs/lanes/scope-ad.md`. Object prefix `/tmp/sad_`.

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including `$PY tools/relocs.py LEGOLAND/<file>.c` (zero MISMATCH).

NEW-FUNCTION scope, **9 functions, ≈860 instructions**, two new files.
AB already owns `LEGOLAND/rlepaint.c` (eight SoftBlitRLEPlain leaves) — do
not edit it. Expect the same **hand-written / naked asm** pattern as AB for
the two large painters; read `docs/lanes/scope-ab.md` and DECOMP's scope-AB
fold first.

## `LEGOLAND/rlepaint2.c` — recolour + highlight RLE frame painters (≈615)

Caller docs: `render3.c` names `SoftBlitRLEFrameRecolour` at **0x00468040**;
`bigrender.c` names `SoftBlitRLEFrame` at **0x00468410**. Same Type-3 A/B/C
grammar as AB; store rules differ (`pixel & mask` / `(pixel & mask) >> 1`)
and hit rules differ. Diff against `rlepaint.c`'s Hit/Fast leaves.

| address | name | insns | evidence |
| --- | --- | ---: | --- |
| 0x00468040 | `SoftBlitRLEFrameRecolour` | 303 | render3.c SoftBlitRLE dispatcher |
| 0x00468410 | `SoftBlitRLEFrame` | 312 | bigrender.c SoftPrint_XBltFast |

## `LEGOLAND/blitmisc.c` — remaining group-18/19 blit & script stubs (≈245)

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x004632b0 | `sub_4632b0` | 98 | called by gameframe's 0x00458ee0 |
| 0x00463560 | `ResetDamageClock` | 5 | movie3.c |
| 0x004640f0 | `sub_4640f0` | 70 | tail-jumped from PushSetTarget (gpu.c) |
| 0x004659a0 | `BlitAdvisorFrame` | 57 | screens3.c RenderAdvisorIcon notes |
| 0x00466080 | `sub_466080` | 102 | table at 0x004b9ca4 |
| 0x00468830 | `ClearScriptTexts` | 4 | movie3.c |
| 0x004689a0 | `FreeScriptStrings` | 26 | movie3.c |

**Order:** tiny stubs → medium blitmisc → the two RLE painters (diff AB
family, transfer). Keep AB's Type-3 A/B/C roles (pixels / lengths / 2-bit
controls).

## Owned elsewhere — do not create or edit

`rlepaint.c` (AB), `softblit2.c`, `render3.c`, `bigrender.c`, `movie3.c`,
scopes F/G/H, V, AA, AC, Codex-F (`coaster10.c`, `ridemachine2.c`,
`uistubs2.c`), every existing `.c`.
