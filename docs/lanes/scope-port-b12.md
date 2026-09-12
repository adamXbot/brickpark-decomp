# Scope PORT-B12 — the doubled typed character, and the undrawn visitors

> **PORT-B12 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-B12)** — branch
> `scope/PORT-B12`, cut from the PORT-P1 merge (`82cd767d`). Files owned:
> `portable/src/hostwin/{ddraw,user32,gdi32,dinput,winmm,dsound,avifil32,
> msacm32,ll_ttf,ll_audio,ll_font}.c`, `portable/src/browser/**`,
> `portable/cmake/browser.cmake`, additions to
> `portable/hostwin/include/ll_host.h`. `LEGOLAND/*.c` is READ-ONLY for this
> lane. Brief: `docs/SCOPE_PORT_WAVE.md`.

Two findings from PORT-P1's free-play run: **P1-5** (roughly every seventh
typed character is doubled in the cheat ring, so no cheat can fire) and
**P1-6** (sixty live visitors, none of them drawn).

Work in progress; see the sections below as they land.
