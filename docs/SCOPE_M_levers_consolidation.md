# Scope M — consolidate the codegen levers (2026-09-05)

> **Status: DONE — `docs/LEVERS.md` merged into `main` 2026-09-05 (194 rules from 493
> DECOMP entries, symptom index, correction register).** Branch `scope/M`. Any agent. **Documentation
> only: this scope creates `docs/LEVERS.md` and edits nothing else**, so it
> cannot collide with anything.

**Read `docs/PARALLEL_CONTRACT.md` first** for the project and the git
rules.

## The problem

`docs/DECOMP.md`'s section "VC6 SP3 codegen levers" is the project's most
valuable asset and its least usable one. It has grown by accretion to roughly
**750 entries** — each written by the session that measured it, in the order
they arrived, newest at the top, many hundreds of lines long. It now contains:

- the same rule stated three or four times from different functions, each
  with its own evidence;
- entries that were later CORRECTED in place (marked "corrected 2026-09-05",
  "reconciled", "superseded") but whose original text still sits below the
  correction, so a reader skimming lands on the wrong version;
- entries whose scope was later narrowed ("holds only for a single-predecessor
  label"; "for a straight-line sum, NOT a loop accumulation"; "for an
  address-taken struct, not a pair of sums") where the narrowing lives in a
  different entry from the rule;
- genuine negatives ("not reachable from C", "measured, inert") mixed with
  positive levers;
- no index: a new session is told to "read the top fifty, then search".

## The job

Produce **`docs/LEVERS.md`**: the same knowledge, organised so that a session
matching a function can find the lever it needs in under a minute.

1. **Group by the question a matcher asks**, not by the order discovered:
   block layout (exile, merge, tail duplication, switch lowering, push
   sinking); loops (forms, induction variables, cursor anchors, latch order);
   register allocation (rotation, callee-saved ranking, zero webs, the
   volatile family); frame layout (aggregates, dead argument slots, scope);
   types and widths; sums and algebra; floats and x87; records and lists;
   calls and cleanup (`add esp` merging, struct returns, COM); reading the
   original (the "tells": what a shape in the disassembly proves about the
   source); triage and method; and a separate section of **negatives** —
   things measured unreachable, each with the body it was measured on, so
   nobody re-derives them.
2. **One rule, one entry.** Where the corpus states a rule several times,
   merge into one statement with ALL the evidence sites listed (function,
   before -> after). Where an entry was corrected, keep only the corrected
   form, and say in one clause what the earlier mistake was so the lesson
   survives. Where a rule was narrowed, fold the scope condition into the
   rule.
3. **Preserve every measurement.** Numbers (mismatch before -> after, byte
   counts, spelling counts) and function names are the evidence; do not
   summarise them away. Do not invent, generalise beyond what was measured,
   or drop an entry because it seems minor.
4. **Add an index** at the top: a table of symptoms ("the original has one
   more push than we do", "our body is one byte short", "a load is emitted
   first in its block", "two same-sized globals are swapped in a
   zero-mismatch body") -> the section and rule to read.
5. **Do not edit `docs/DECOMP.md`.** It stays as the historical record and
   the integrating session keeps appending to it; once `LEVERS.md` lands,
   the contract will point new sessions at `LEVERS.md` first and DECOMP for
   history.

## Sources

- `docs/DECOMP.md`, the whole "VC6 SP3 codegen levers" section (start at its
  heading; it runs to the "Legal" heading).
- `docs/HANDOFF.md` §6B (the residual triage) and §3.
- `docs/PARALLEL_CONTRACT.md`'s "Levers that decide…" section — a useful
  first cut at the grouping.
- `docs/lanes/*.md` — the parallel sessions' notes; everything in them that
  mattered has been folded into DECOMP, but the originals carry fuller
  evidence and are worth citing.

## Report format

In your final message: the section list with entry counts, how many DECOMP
entries were merged into how many LEVERS entries, the list of entries where
you found the corpus still contradicting itself (with your resolution and
the evidence you chose), and anything you could not place.
