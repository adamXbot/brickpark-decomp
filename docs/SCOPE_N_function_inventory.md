# Scope N — the whole-binary function inventory (2026-09-05)

> **Status: DONE — merged into `main` 2026-09-05. `tools/inventory.py` and
> `docs/lanes/scope-n.md`: 867 unmatched functions enumerated from eight sources,
> the coverage gap reconciled to the byte, 31 candidate groups; scopes P and Q
> cut from groups 16 and 17.** Branch `scope/N`. Notes: `docs/lanes/scope-n.md`.
> Any agent. **This scope creates one new tool and a report; it edits no
> `LEGOLAND/*.c` file and no existing tool**, so it cannot collide with
> anything. Same shape as scope L, which delivered `tools/relocs.py`.

**Read `docs/PARALLEL_CONTRACT.md` first** for the project, the environment
and the git rules.

## The problem

`tools/coverage.py` says **29.5% of game code — about 190 KB of the 643 KB
in `.text` below the CRT boundary at 0x0049e000 — is not matched at all**
(neither exact nor partial). But the only roadmap the project has,
`tools/callees.py`, lists the callees our `extern` declarations name and
cannot see past them: today that is 46 game functions, about 8 KB. Nobody
has a list of the other ~180 KB. It is reached through callback tables
(ride, icon, screen and popup handlers), through callers that are themselves
unmatched, and through code nothing calls. Every new-function scope so far
has been cut from the callee frontier; the frontier is nearly exhausted while
the coverage gap is not. **The next scopes have to be cut from a complete
inventory, and this scope builds it.**

## The job

1. **Write `tools/inventory.py`** (new file — do not modify `match.py`,
   `audit.py`, `callees.py`, `coverage.py` or `remaining.py`). It enumerates
   every function start in the game-code range of `.text`
   (`0x00401000..0x0049e000`) from four sources, iterated to a fixpoint:
   - the exports (`symbols/legoland.exports.txt`, code section only — see
     `remaining.py` for the section test that keeps data exports out);
   - every `// FUNCTION:` / `// WIP-FUNCTION:` marker in `LEGOLAND/*.c`
     (`audit.annotated` parses them);
   - **recursive descent from every known function**: bound it with
     `match.true_extent`, collect its direct `call rel32` targets and
     tail-`jmp rel32` targets that leave the extent, add them as functions,
     repeat until nothing new appears;
   - **pointers into `.text` from `.data`/`.rdata`**: scan both sections for
     4-byte-aligned values inside the game-code range that land on a
     plausible function start (the byte before is `ret`/`int3`/padding or a
     previous function's extent ends there, and the walker gives it a
     bounded extent). These are the callback tables. Jump-table targets are
     inside extents and must not be reported as functions.
   `match.py`'s `load_exe`, `rva2off` and `true_extent` are importable and
   should be reused; `remaining.py` shows the section classification;
   `callees.py` shows the marker parsing.
2. **Reconcile against the coverage gap.** Sum the extents of every
   inventoried function not carrying a marker. Report how much of
   `coverage.py`'s unmatched byte count that accounts for, and list the
   **residue** — game-code byte ranges between known extents that no source
   reached (padding, dead code, data-in-text, or functions reached only
   through computed calls). A residue of a few KB is expected; a residue of
   50 KB means a source is missing.
3. **Write `docs/lanes/scope-n.md`**: the tool's method and known limits;
   the counts (functions found per source, unmatched functions, bytes,
   residue); and the **inventory of unmatched functions**, sorted by size,
   each with address, instruction count, byte count, how it was reached
   (exported / called by `<matched function>` / called by `<unmatched
   function>` / table at `0x...` in `.data`), and the nearest matched
   neighbour by address. Group them into candidate scopes of roughly
   1,000–1,500 instructions by neighbourhood (same callers, same table,
   same address range) — that grouping is what the integrator will turn into
   the next `SCOPE_*.md` files. Do not name the functions; that is the
   matching session's job.
4. **Do not write any C.** The deliverable is the tool and the list.

## Constraints and hints

- Runnable as `$PY tools/inventory.py` (full report to stdout) and
  `$PY tools/inventory.py --json <path>`; one line per unmatched function.
- Two known traps: `SPRITE_ClipRect` (0x004bdea0) is a data export the
  walker will happily "disassemble" (`remaining.py` docstring); and the
  walker's forward-`jmp` handling was fixed on 2026-09-05 (`_loop_entry` in
  `match.py`) — if an extent looks wrong, say so in the notes rather than
  working around it in the tool.
- `docs/RIDE_CALLBACKS.md` documents the ride callback tables the pointer
  scan should rediscover — a good self-check.
- Commit the tool and the notes on `scope/N`; push when the inventory is
  complete. End Claude commits with the usual trailer.

## Report format

In `docs/lanes/scope-n.md` and your final message: the method and limits;
functions found per source and in total; unmatched functions, bytes, and the
share of the coverage gap accounted for; the residue; and the candidate
scope groupings with their instruction totals.
