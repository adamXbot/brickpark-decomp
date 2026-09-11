# Scope PORT-B9 — make the PARK render and run

> **PORT-B9 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-B9)**
> Branch `scope/PORT-B9` from `feat/decomp-completion-next-steps-24a0d6`
> @ `3a71d5ad` (the PORT-A7 merge). Files owned: `portable/src/hostwin/ddraw.c`,
> `user32.c`, `gdi32.c`, `dinput.c`, `winmm.c`, `dsound.c`, `avifil32.c`,
> `msacm32.c`, `portable/src/browser/**`, `portable/cmake/browser.cmake`, plus
> additions to `portable/hostwin/include/ll_host.h`. `LEGOLAND/*.c` is
> READ-ONLY for this lane, so the VC6 gate has nothing to check.

Goal: A7-1 (the park map area does not render), then drive the park; A7-3 (the
park loader's 1-byte host reads); B5 (profiles across a reload, A7 §8's IDBFS
patch).

Work in progress — findings are appended as they are measured.
