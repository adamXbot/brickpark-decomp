# Scope PORT-A9 — the gates for the two classes the declaration scan cannot see

> **PORT-A9 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-A9)**

Branch `scope/PORT-A9`, from `feat/decomp-completion-next-steps-24a0d6` at the
PORT-B11 merge (`68fab1d3`). Lane files: `portable/tools/gen_link.py`,
`bvstruct_sweep.py`, `portable/cmake/headless.cmake`,
`portable/src/headless/**`, `portable/tests/` (baseline files only).

PORT-B11 §3 closed with a request and PORT-M10 §1f with another, and both are
the same complaint: **a gate that reports 0 is only as good as the set it
walks.**

* `gen/pointers.md` says `raw pointer words: 0` and means it — over the words
  the DECLARATION scan visited. `extern FXEntry g_game_fx[];` has no bound, so
  `cdecl.py` computes no extent, so `gen_link.py` never visits the object, so
  its three raw x86 string addresses are below the gate. 122 such words in 18
  objects (B11's measurement), and the sound effects do not load because of
  three of them.
* `wasm-ld`, `linkreport.py` and `test_callback_types.c` are all blind to a
  by-value struct of 4 bytes or fewer spelled as a scalar in the other TU: the
  arity matches, so nothing warns, and the callee reads a shadow-stack pointer.
  PORT-M10 found it by hand and left the sweep in `tools/`, outside every gate.

This lane makes both measurable from a clean build.

(Sections to follow as the work lands.)
