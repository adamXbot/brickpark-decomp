# Scope LL20 — partials wave D: the two renderview.c bodies

Branch `scope/LL20` from `origin/main` `8a2c0a8b`. **PARTIAL scope**: edit only
the two WIP bodies below and the notes above their markers (see
`docs/PARALLEL_CONTRACT.md`, "Extra rules for PARTIAL scopes"). Object
prefix `/tmp/sll20_`. Findings go in `docs/lanes/scope-ll20.md`.

## Functions (2)

| address | name | file | residual (from the marker) |
| --- | --- | --- | --- |
| 0x0045b180 | RenderView | renderview.c | 57.8%, 381/903 strict; paired placement corrections tested and rejected; first 67 |
| 0x004567a0 | RenderFullMap | renderview.c | 28.8%, 827/1161 strict, 4216/4225 bytes; ILF address carrier improved; first 0 |

These are the two worst rows of HANDOFF §6B's table; waves five to ten sent
lanes at them and closed nothing. They are reopened once, time-boxed, with
the levers found on 2026-09-08 that none of their notes had:

- **Appearance-count allocation** (`git show origin/scope/LL14:docs/lanes/scope-ll14.md`,
  "Closing 0x0046f9a0"): oversubscribed callee-saved candidates are ranked by
  static name-appearance count, not loop weight.
- **Aggregate-as-live-value** (`git show origin/scope/LL9:LEGOLAND/unref1.c`, note above
  0x0040adb0): spelling several sums as one aggregate makes VC6 compute them
  as a unit, keeping an operand live for an extra callee-saved push.
- **Block-boundary pin** (`git show origin/scope/LL10:docs/lanes/scope-ll10.md`,
  Mesh_DropBackFaces): an empty `if (k) ;` between definitions and consumer
  pins single-use locals at their definition sites.
- **Import-load hoisting is a register-availability decision** (`git show origin/scope/LL18:docs/lanes/scope-ll18.md`).

Work RenderView first (first diverging index 67, the head is right). For
RenderFullMap the first diverging index is 0: fix the prologue (frame and
push set) before anything else, and do not spend variants downstream until
it moves.

## Rules

Read the note above each marker FIRST and do not repeat what it lists;
reconstruction-error pass before variant search; `audit.py` on the whole file
after every change (PASS, `[OK]` count never drops); `relocs.py` zero
`MISMATCH` and `/W3` clean before each commit; WIP marker until `[OK]`;
retire honestly with a dated paragraph in the note. Do not run `verify.py` /
`progress.py` / `coverage.py`; do not edit `tools/`, `docs/DECOMP.md`,
`docs/HANDOFF.md`, `README.md`. Commit to `scope/LL20` only; no push; no
merge; **no Co-Authored-By trailer of any kind**; never `git add -A`.
