# Scope PORT-A6 — the extent of an object is `sizeof`, not a table row

> **Status: IN PROGRESS (claimed 2026-09-12 by PORT-A6).** Branch
> `scope/PORT-A6` from `feat/decomp-completion-next-steps-24a0d6` @ `1a7b38a4`
> (the PORT-B6 merge). Files owned: `portable/tools/gen_link.py`,
> `linkreport.py`, `name_trap.py`, `portable/cmake/headless.cmake`,
> `portable/src/headless/**`, `portable/src/hostwin/kernel32.c`, `msvcrt.c`,
> plus the new `portable/tools/cdecl.py`. `LEGOLAND/*.c` untouched, so the VC6
> gate has nothing to check for this lane.

The split-record class has bitten five times — `g_key_state`, `g_gpu_state`,
`GameInput`, `PopUpUI`, `Profile`, `BlitCtx`/`HitInfo`, `CurProfile` — and every
one was found only after a symptom, because the extent of a struct-typed object
was a row somebody had to add to `STRUCT_EXTENTS` by hand. The extent is not a
judgement call: it is `sizeof` of the type the game's own source declares the
object with. This lane computes it.

(Notes written up at the end of the lane; see the commits on this branch.)
