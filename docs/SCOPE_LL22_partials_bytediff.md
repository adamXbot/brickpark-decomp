# Scope LL22 — partials wave E: byte-divergent residuals

Branch `scope/LL22` from `origin/main` `62d5ef7f`. **PARTIAL scope**: edit only
the WIP bodies below and the notes above their markers (see
`docs/PARALLEL_CONTRACT.md`, "Extra rules for PARTIAL scopes"). Object
prefix `/tmp/sll22_`. Findings go in `docs/lanes/scope-ll22.md`.

## Why these are being reopened

Every body below is **instruction-exact but NOT byte-exact**. That is the
useful signal: a byte delta with an equal instruction count means a real
operand, addressing-mode or immediate difference is still hiding — not just a
register choice. Chase the bytes first; the register picture usually falls out
once the byte delta is gone.

Scope Codex-F (closed 2026-09-09, `docs/lanes/codex-f.md`, entry in
`docs/DECOMP.md`) contributed two things this wave should use:

1. **`/FAcs` is the tool for a residual you cannot see.** Compile with
   `ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /O2 /Gy /Gd
   /FAcs /Fa<out>.asm /Fo/tmp/sll22_x.obj LEGOLAND/<file>.c`. It prints VC6's
   own frame symbol table and attributes every instruction to a source line.
   A local that cost a frame slot shows up as a `_name$ = -N` equate — which
   is how Codex-F found that a temporary had silently grown the frame by two
   slots. It also shows that **VC6 overlaps locals onto dead PARAMETER
   slots** (`_value$ = 8` sharing `_person$ = 8` in `bnvpath.c`), so a
   pointer pun and a plain local are equivalent and neither is suspect.
2. **An alias pointer defeats VC6's canonicalisation of a commutative `imul`
   or `fmul` whose two operands are constant offsets off ONE pointer**, which
   silently reverses the emitted operand order. `sb = g_some_table;` and
   indexing through `sb` restores source order — that one change took a body
   from 2 mismatches to 0. **Source operand order is inert**; change the
   SPELLING, not the position.

If a body turns out to be a register-ranking problem after the byte delta
closes, the cancelled-pair anchor lever is written up in
`docs/SCOPE_LL21_partials_allocation_b.md` and in the DECOMP entry.

## Functions (4) — 449 instructions, 116 audit mismatches

| address | name | file | state | residual (from the marker) |
| --- | --- | --- | --- | --- |
| 0x004718c0 | ClampPopUpToScreen | misc3.c | 43i, 143B vs 144B, 3 mism | endorsed FLOOR; a 2-mismatch/144B variant exists and was deliberately NOT taken (it contradicts the original's `cmp esi,25h`). Treat as a stretch goal, and do not take that trade again |
| 0x00475630 | InsertChildIntoList | fpui.c | 68i, 170B vs 171B, 22 mism | loses a result copy instead of gaining an argument copy; the same shape recurs in ridecb5.c `BoatingSchool_Tick` at idx 105/206 |
| 0x00473b00 | UpdateControllerFromMouseData | input.c | 109i, 280B vs 272B, 13 mism | a materialised 0 held in a register in an FPO function; two low clamps |
| 0x00442980 | LoadAltTextures | mantex.c | 229i, 748B vs 752B, 78 mism | scalar-home permutation + `tolower` esi/edi swap; fail-path `xor ebx,ebx` |

`LoadAltTextures` is scope AC's last open body (AC merged at 14 of 15, and its
other floor `PutOne3DBlokeOnRide` was closed 2026-09-09). It is idle and
unassigned — this brief claims it. Do not touch any other function in
`mantex.c`; they are all exact.

Start with `InsertChildIntoList`: 68 instructions and a 1-byte delta, so the
missing copy is a single addressing-mode or operand difference you can find in
the `/FAcs` listing rather than by search.

## Rules

- Read the note above each marker FIRST and do not repeat what it lists.
- Reconstruction-error pass before any variant search.
- After EVERY change `audit.py` the whole file: it must still end PASS and
  the count of `[OK]` lines must not drop; if it does, revert.
- `relocs.py` zero `MISMATCH`, `/W3` clean, before each commit.
- Marker stays `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <residual>)` until
  `audit.py` prints `[OK]`; then exactly `// FUNCTION: LEGOLAND 0x<VA>`.
- Retire honestly: if the lever does not move a body, add one paragraph to
  its note saying what you measured and move on. A well-measured negative is
  a deliverable.
- Do not run `verify.py` / `progress.py` / `coverage.py`; do not edit
  `tools/`, `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`.
- Commit to `scope/LL22` only; no push; no merge; **no Co-Authored-By
  trailer of any kind.**
