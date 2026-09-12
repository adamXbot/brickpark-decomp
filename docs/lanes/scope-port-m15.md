# Scope PORT-M15 — the dual-address global class, closed: 17 names, 17 addresses each

> **PORT-M15 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-M15)** — branch
> `scope/PORT-M15`, cut from `feat/decomp-completion-next-steps-24a0d6` at or
> after the PORT-P3 merge (`25005b2c`). Matching-side lane: every change to
> `LEGOLAND/*.c` is an **identifier rename or an address-comment correction**,
> which cannot move a byte, and every touched file is re-gated with `audit.py`
> + `relocs.py`. `portable/**` and `docs/HANDOFF.md` are other lanes'.
> Brief: `docs/SCOPE_PORT_WAVE.md`. Parent finding: PORT-P3 §3.3 (P3-1/2/3).

**Headline: all 17 names are real, and the address comments were right every
single time.** Not one of the 34 declaration sites names the wrong address —
`relocs.py`, forced to print what the ORIGINAL binary targets at each
instruction, agrees with every comment in the tree. The class is therefore
never "a wrong comment": it is always **two different objects wearing one
name**, and in nine of the seventeen the OTHER object already has a perfectly
good name somewhere else in the tree that nobody had connected.

(Working notes — tables, gates and the page proof are filled in as the lane
runs.)
