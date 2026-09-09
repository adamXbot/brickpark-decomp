# Scope LL24 — Draw3DPersonModel, the largest open body

Branch `scope/LL24` from `origin/main` `b49871b7`. **PARTIAL scope, ONE function**:
edit only `Draw3DPersonModel` in `LEGOLAND/person3d.c` and the note above its
marker (see `docs/PARALLEL_CONTRACT.md`, "Extra rules for PARTIAL scopes").
Object prefix `/tmp/sll24_`. Findings go in `docs/lanes/scope-ll24.md`.

## The function

| address | name | file | state |
| --- | --- | --- | --- |
| 0x00440a30 | Draw3DPersonModel | person3d.c | 1023i/3511B vs 1023i/3523B, **377 audit mismatches** (63.3%) |

It is instruction-exact at 1,023 instructions and 12 bytes short. It is the
single largest unmatched-quality body left in the tree, and closing it is
worth more exact bytes than any other open item.

## What is already settled — do not re-derive

`docs/DECOMP.md` carries two hard-won facts about this body:

- **It is NOT hand-written assembly.** That long-open question is closed. Its
  ebp frame and unconditional ebx/esi/edi save come from the `__asm`
  fixed-point macros in the file; the pushes are at the top of the prologue,
  not inside the stream, and there is no `xchg` against memory anywhere in
  the 1,023 instructions. It is mixed C plus inline asm, reachable from C.
- **The residual is almost entirely frame colouring.** 707 instructions agree
  in mnemonic, registers and immediates and differ only in `[ebp-N]`.

And the frame rule that governs any file containing inline asm:

- **ANY `__asm` block in a function REVERSES the whole frame-object layout
  order** (it is global, not per-object; the block's position and count are
  inert, only its presence matters). Without asm the order is descending byte
  size from ebp down; with asm it is ascending byte size — by BYTE SIZE, not
  element count. Reference weight only pulls an object TOWARD ebp. Block
  scope moves an object exactly one step, cutting ~24 references moves it one
  step, and the two do not stack. **Declaration order of locals is completely
  inert** for /O2 frame layout, with and without asm.
- **References made only via `lea X` inside an `__asm` block carry ZERO
  weight** — deleting 4, 8 or all 24 of them is byte-identical. Only C-level
  references rank objects.

**Check every `__asm` operand name against MASM's reserved words** before you
finish: a local named `cr2` here once expanded to `mov cr2, eax` and emitted
the privileged `0f 22 d0`, which would have faulted at ring 3. Reserved:
`cr0`-`cr4`, `dr0`-`dr7`, `tr3`-`tr7`, `st`, and the segment names.

## The new lever this scope exists to apply

Scope Codex-F (closed 2026-09-09, `docs/lanes/codex-f.md`) found that
**`/FAcs` is the tool for a frame-colouring residual**:

```
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /O2 /Gy /Gd \
  /FAcs /Fa/tmp/sll24_person3d.asm /Fo/tmp/sll24_person3d.obj LEGOLAND/person3d.c
```

It prints VC6's **own frame symbol table** — every local as a `_name$ = -N`
equate — and attributes every instruction to its source line. With 707
instructions differing only in `[ebp-N]`, that listing turns a 377-mismatch
search into a direct comparison: read off which object VC6 put at which
offset, compare against the offsets the original uses, and reorder by SIZE
(the rule above), not by declaration. Codex-F used the same listing to find a
temporary that had silently grown a frame by two slots. Do this before any
variant search — it should be your first compile, not your hundredth.

The listing also shows VC6 overlaps locals onto dead PARAMETER slots, so an
object appearing at a positive `[ebp+N]` offset is expected, not a bug.

## Rules

- Read the note above the marker FIRST and do not repeat what it lists.
- Reconstruction-error pass before any variant search.
- After EVERY change `audit.py` the whole file: it must still end PASS and
  the count of `[OK]` lines must not drop; if it does, revert. `person3d.c`
  has many exact bodies — protect them.
- `relocs.py` zero `MISMATCH`, `/W3` clean, before each commit.
- Marker stays `// WIP-FUNCTION: LEGOLAND 0x00440a30  (<pct>, <residual>)`
  until `audit.py` prints `[OK]`; then exactly
  `// FUNCTION: LEGOLAND 0x00440a30`.
- Report the frame map you recover even if the body does not close — the
  offset table is the deliverable that lets the next pass finish it.
- Do not run `verify.py` / `progress.py` / `coverage.py`; do not edit
  `tools/`, `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`.
- Commit to `scope/LL24` only; no push; no merge; **no Co-Authored-By
  trailer of any kind.**
