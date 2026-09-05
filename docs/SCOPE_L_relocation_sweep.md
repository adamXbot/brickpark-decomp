# Scope L — the relocation sweep (2026-09-05)

> **Status: DONE — merged into `main` 2026-09-05. Tool + sweep delivered; 17 of the
> 21 hits fixed at integration, 4 deferred to scopes F and H (HANDOFF §1).** Branch `scope/L`. Notes: `docs/lanes/scope-l.md`.
> Any agent. **This scope creates one new tool and a report; it edits no
> `LEGOLAND/*.c` file and no existing tool**, so it cannot collide with
> anything.

**Read `docs/PARALLEL_CONTRACT.md` first** for the project, the environment
and the git rules.

## The problem

The verify gate compares instructions after NORMALISING operands: every
absolute address becomes one token, so two same-sized globals look identical.
Twice today independent reviewers caught bodies that passed the gate with
**zero mismatches** while their global identities were wrong — `NewMechanicOrder`
(19i/67B, head and tail stores swapped) and `Coaster3D_SetCarClipDepth`
(19i/105B, loading `a/b/c` where the original loads `b/c/d`, twelve
relocations mismapped). Both were fixed by their authors; the question is how
many of the other **2,355 exact bodies** carry the same class of error. Nobody
knows, and the current tools cannot tell.

## The job

1. **Write `tools/relocs.py`** (new file — do not modify `tools/match.py`,
   `audit.py`, `matchfull.py` or `verify.py`). For one function it compiles
   the file the way `audit.py` does (same wrapper, same flags, per-process
   object path under `/tmp/sl_`), reads the COFF relocations of our object
   for that function, resolves each relocated operand to the SYMBOL it names
   (a global's address from its `/* 0x... */` comment or from the reference
   tables in `docs/DECOMP.md` "Global names from the export table", a callee's
   address from its extern comment), and compares those resolved addresses
   position for position against the ORIGINAL's absolute operands at the same
   instruction indices. Report every position where the original's absolute
   address differs from ours. `tools/match.py`'s `load_exe`, `rva2off`,
   `obj_function_code` and `true_extent` are importable and should be reused;
   `tools/callees.py` shows how the extern comments are parsed.
2. **Sweep every `// FUNCTION:` body in the tree** with it (a read-only pass;
   ~2,355 functions — budget for it, and skip files that fail to compile
   rather than stopping). Write `docs/lanes/scope-l.md` with: the tool's
   method and its known limits; the full list of functions with at least one
   mismapped relocation, each with file, address, instruction index, the
   original's target and ours; and a classification of each hit (swapped
   same-size globals, an off-by-one field in a struct of globals, a wrong
   callee, a false positive from an unresolvable symbol, …).
3. **Do not fix the bodies.** The integrating session applies fixes at a
   quiet-tree gate, because many of the files involved are owned by running
   partial scopes. Your deliverable is the tool and the list.

## Constraints and hints

- The tool must be runnable as
  `$PY tools/relocs.py LEGOLAND/<file>.c <Name> 0x<VA>` for one function and
  `$PY tools/relocs.py --all` for the sweep, printing one line per hit.
- Where a relocation cannot be resolved to a known address (a symbol with no
  address comment, a string literal, a jump table), say so in the output and
  classify it as unresolved rather than as a mismatch.
- The Codex sessions each ran "an independent COFF relocation review" during
  scopes C–E; their notes in `docs/lanes/codex-c.md`, `codex-d.md`, `codex-e.md`
  describe what they checked and may save you the design.
- Commit the tool and the notes on `scope/L`; push when the sweep is done.
  End Claude commits with the usual trailer.

## Report format

In `docs/lanes/scope-l.md` and your final message: the tool's method and
limits; the number of functions swept, the number with hits, and the number
of unresolved positions; the hit list; and your classification with, for each
class, one worked example showing the original operand and ours.
