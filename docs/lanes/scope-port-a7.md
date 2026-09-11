# Scope PORT-A7 — a pointer FIELD is a pointer

> **PORT-A7 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-A7).** Branch
> `scope/PORT-A7` from `feat/decomp-completion-next-steps-24a0d6` @ `c337b295`
> (the PORT-B8 merge). Files owned: `portable/tools/gen_link.py`, `cdecl.py`,
> `linkreport.py`, `name_trap.py`, `portable/cmake/headless.cmake`,
> `portable/src/headless/**`, `portable/src/hostwin/kernel32.c`, `msvcrt.c`.
> `LEGOLAND/*.c` untouched, so the VC6 gate has nothing to check for this lane.

PORT-B8 diagnosed the park wedge and handed this lane the fix: 1211 words of
`gen-browser/globals.c` that still hold RAW x86 addresses, 30 of which are the
progress screens' sprite names and hang both doors into the park.
