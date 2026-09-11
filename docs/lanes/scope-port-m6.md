# Scope PORT-M6 — the game-side items filed since PORT-M5

> **Status: IN PROGRESS (claimed 2026-09-12 by PORT-M6).** Branch
> `scope/PORT-M6` from `feat/decomp-completion-next-steps-24a0d6` @ `9c646312`
> (the PORT-B7 merge). Matching-side lane: every change to `LEGOLAND/*.c` is
> either inside an `#ifdef LEGOLAND_PORTABLE` arm or is a proven recovery fix
> applied to both builds with the disassembly evidence, and every touched file
> is re-gated with `audit.py` + `relocs.py`. `portable/**` and
> `docs/HANDOFF.md` belong to PORT-B8 and the integrator.

Work items:

1. A6 §7's overlapping-name hazards on the front-end and park paths.
2. B6's two findings: `SetVidAnim(NULL)` (advisor.c) and `OpenMovie`'s
   uninitialised `pfile` (movie.c).
3. `RES_LowRead` / `RES_LowSeek` — host imports wearing game names.
4. The void-declared vtable/callback slot sweep (B7 B1a) and the cast
   forwarders left in `gen-browser/aliases.c`.

(Sections are filled in as each item closes.)
