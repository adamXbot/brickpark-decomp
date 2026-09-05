# The parallel-session contract

Every `docs/SCOPE_*.md` brief points here. This is the part that is the same
for every session; a scope file adds only its files, its function table and
what is specific to it. Fifteen scopes have been run under these rules by Claude
and Codex agents (`SCOPE_FABLE_A`..`D`, `SCOPE_CODEX_A`..`E`, E, I, J, K, L,
M) and every one merged with zero conflicts.

## What this project is

A matching decompilation of LEGOLAND (Windows, 2000, VC6 SP3, `/O2 /Gy /Gd`).
Human-written C in `LEGOLAND/*.c` must compile to reproduce
`original/legoland.exe` function-by-function. Status at the time of writing
(2026-09-05, evening): **2383 exact functions, 73 partials, 59.5% of game code
matched exactly (70.5% with partials).** No game binary or asset is ever committed (`original/` and
`gamedata/` are gitignored).

## Read these first (in this order)

1. `docs/LANE_BRIEF.md` — the method per function and **the verification gate**
   (its PROJECT/STATUS lines are stale; use ENVIRONMENT below).
2. `docs/LEVERS.md` — the lever corpus consolidated (scope M): 194 rules
   from ~750 DECOMP entries, grouped by the question a matcher asks, with a
   **symptom index** at the top. Start there. Then `docs/DECOMP.md`, section
   "VC6 SP3 codegen levers", for anything newer than the consolidation
   snapshot (commit 36018920, 2026-09-05): entries are newest at the top, so
   read down until you reach the `scope-k` fold. DECOMP stays the
   historical record and the place new levers are appended.
3. `docs/HANDOFF.md` §3 (rules not in the code) and §6B (residual triage).
4. `docs/RIDE_CALLBACKS.md` if your scope touches ride callbacks.

## Environment

```
cd /Users/systemadmin/Documents/Development/Github/legoland
git checkout main && git pull --ff-only origin main
PY=/Users/systemadmin/.venvs/legoland/bin/python
export LEGOLAND_CL=/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl
```

If either path does not exist on your machine, **stop and report** — do not
try to build a toolchain or a venv.

- Disassemble: `$PY tools/disasm.py original/legoland.exe 0x<RVA> <count>` —
  **RVA = VA − 0x400000** (0x00426120 → 0x26120).
- Iterate: `$PY tools/matchfull.py LEGOLAND/<file>.c <Name> 0x<VA> --obj /tmp/<scope>_<Name>.obj`
- **Authoritative gate:** `$PY tools/audit.py LEGOLAND/<file>.c` — a function
  counts ONLY when it prints `[OK]`. `matchfull.py` over-reports a body whose
  `ret` is followed by a `.rdata` jump table; trust `audit.py`.
- `/W3` check (must be clean): `ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/<scope>_w3.obj LEGOLAND/<file>.c`
- Objects always under `/tmp`; never `/Zi`.
- **Do NOT run `tools/verify.py`, `tools/progress.py` or `tools/coverage.py`**,
  and do not edit anything under `tools/`. `audit.py` and `matchfull.py` use
  per-process object paths and are safe alongside other sessions' compiles.
- **Relocation identity (new gate step, scope L):**
  `$PY tools/relocs.py LEGOLAND/<file>.c` — zero `MISMATCH` lines before you
  commit a `// FUNCTION:` body (`UNRESOLVED` lines are literals and
  unannotated symbols; fine). The normalised gate cannot see a wrong
  same-sized global, a swapped callback or a wrong address comment; this
  can, and 21 of 2,383 exact bodies had one on 2026-09-05. Read-only,
  per-process temp objects, safe alongside other sessions. A hit means
  either the C names the wrong object or the extern's `/* 0x... */`
  comment is wrong — fix whichever the disassembly says.
- Tooling note: the extent walker's rotated-loop defect (a forward `jmp`
  over a loop body was taken as the function's end) was FIXED on 2026-09-05;
  `audit.py` now bounds such functions correctly. If a body still reports
  ESCAPES, the branch really does leave the original's extent — a duplicated
  tail or a different block layout — and the note should say which.
- Roadmap: `$PY tools/inventory.py` (scope N; read-only, ~30 s, safe
  alongside compiles) enumerates every unmatched function with its reach,
  nearest matched neighbour and candidate group; scopes are cut from its
  groups (`docs/lanes/scope-n.md`). Its `long_extent` is the only walker
  that bounds `0x004453a0` (8,085 instructions); the gate's `true_extent`
  cannot, so that function is not assignable yet.

## Files you must not touch

- Anything under `tools/`.
- `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`,
  `docs/LEGOLANDPROGRESS.HTML`, `docs/LEGOLANDPROGRESS.SVG`. These are shared
  and regenerated at integration. **Put your levers and findings in
  `docs/lanes/<scope>.md`** in DECOMP's bullet style, each with its evidence.
- Every file listed in your scope's "owned elsewhere" section, and any file
  that is not in your scope's own list. New-function scopes create only their
  own new files and read everything else; partial scopes edit only the WIP
  bodies named in their table (see below).

## Git

`git checkout -b <branch named in your scope>` from current `main`. Commit to
that branch only, as often as you like; **never commit to `main` and never
push `main`.** Push the branch when a file is complete. The integrating
session merges, runs `verify.py` alone, regenerates the progress report and
pushes. Claude sessions end every commit message with
`Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>`; Codex agents use
their own attribution.

## Rules that are not in the code

- **Marker discipline.** `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <precise residual>)`
  on a body until `audit.py` prints `[OK]`; then exactly
  `// FUNCTION: LEGOLAND 0x<VA>` with no parenthetical. The marker goes on the
  line immediately above the signature. **Never commit a prologue-only or
  fabricated-tail body as `// FUNCTION:`.**
- Declare every callee `extern` with a trailing `/* 0x0044xxxx */` comment —
  `tools/callees.py` parses it.
- **Extern prototype TYPES are caller-side codegen levers.** A callee may need
  different prototypes in different files (`StandardRemoveObject` has five in
  the tree). Never "align" another file's declaration; note the divergence in
  your notes. Symbol NAMES are not levers.
- **One name per address.** A placeholder or a name that collides with an
  existing symbol gets a distinct name, recorded in your notes.
- Reproduce original bugs; comment them at the site; do not fix them.
- Report behaviour you cannot explain from the disassembly rather than
  inventing plausible C. An honest WIP beats a wrong body that compiles close.

## Extra rules for PARTIAL scopes (editing existing files)

A partial scope names functions that already exist as `// WIP-FUNCTION:`
bodies inside files that also hold exact `// FUNCTION:` bodies.

- Edit ONLY the WIP bodies in your table and the notes above their markers.
  Never touch a `// FUNCTION:` body, a struct declaration an exact body
  depends on, or an extern's TYPE (both are codegen levers for the exact
  bodies around you).
- After EVERY change, `audit.py` the whole file: it must still end `PASS`, and
  the count of `[OK]` lines must not drop. If it does, revert.
- **Read the note above the marker first** — earlier sessions recorded the
  residual, the first diverging index, the classification and everything
  ruled out. Do not re-run what it lists.
- **Do a reconstruction-error pass before any variant search**: read the
  original instruction by instruction and ask "what C statement produces
  exactly this?". Waves eight to ten found twenty-six reconstruction errors
  this way, including a live bug (a local named `cr2` in an `__asm` block
  became the control register).
- Then run the §6B triage: strict, register-blind, offset-blind with frame
  homes resolved by push depth. `strict >> rb` is allocation — try one free
  `volatile` read, then the one-temporary spellings (name an array element,
  name a call result, split a nested call), then sweep the store order;
  `strict >> ob` is the frame — usually unreachable; `strict == rb == ob` is a
  pure scheduling permutation — sweep the store and increment order;
  rb-still-high is STRUCTURAL and the only kind reliably reachable.
- **Retire honestly.** If after that the residual is unreachable, update the
  note with what you eliminated and say "at its floor" with the evidence.
  That is a valued outcome — seven functions have been formally retired that
  way and it stops future effort being wasted. Do not grind: a residual of 3
  can be a floor (three of them were, in wave eleven), and a residual of 39
  can be two statement positions (`RES_FindVolumeOnAnyDrive`).
- Functions marked **EXHAUSTED** in `docs/HANDOFF.md` §1 are not in any table
  and must not be reopened.

## Levers that decide whether a draft lands at 95% or 40%

The full statements are in DECOMP; these are the ones that pay most often.

- **Layout first.** An else arm is exiled past the fall-through trace iff it
  ends in an unconditional jump. `if (a == 0 || b != c) return X;` exiles X —
  split the test and jump INTO the second `if`'s block to inline it; MERGE
  separate `goto` guards to exile a shared block. Two `return K` sites merge
  at the FIRST; the fall-through copy of a shared tail survives. A push sinks
  past a leading guard only when the guarded block ends in ONE `return K`. A
  jump-table `switch`'s case order is its block order; a compare-chain switch
  is laid out by case VALUES; cases sharing a body lower to a RANGE chain. A
  pending cdecl `add esp` cannot cross a branch join (a shared tail with a
  call was written twice). VC6's tail-duplication threshold: one call is
  copied, two calls are jumped to. A missing callee-saved push is a
  LIVE-VALUE deficit — compute the shared value before the split.
- **Loops.** `while (n-- != 0)` on an unsigned counter rotates with dead
  `dec`/`inc`; up-counting `for` emits `add eax,-K`, down-counting `sub eax,K`.
  A `while`'s condition is whichever test VC6 leaves in the LATCH; a counted
  `for` with a literal bound keeps its test in the latch with no peel. The
  for-increment order of lockstep cursors decides the latch order; the
  counter's update must come BEFORE the other induction variable's. Two
  lockstep cursors spelled the SAME way let VC6 eliminate an IV. A
  strength-reduced cursor anchors at the field with the MOST references (ties
  to the LAST; offset 0 never wins from `->`). An address-taken counter turns
  strength reduction off.
- **Aggregates.** VC6 flattens an aggregate whose address is never taken, so
  a struct wrapper changes nothing about ACCESS — but aggregate-ness still
  decides PLACEMENT: two ints as one `Pos` land at the top of the frame; two
  byte-wide values that survive calls get the original's EBX winner only as
  ONE shared two-byte aggregate; an aggregate local blocks reuse of a dead
  parameter's home slot; a `Pos` by value is three levers in one (left-to-right
  schedule, an extra definition for the rotation, destination-symbol for its
  fields); a coordinate pair feeding a sum of squares must be one `Pos`
  local; a value that must survive a call cannot be a FIELD of an
  address-taken aggregate — assign plain locals INTO it; an 8-byte struct
  return must be the accumulator of a straight-line sum but NOT of a loop
  accumulation.
- **Reads.** Read a class/def global directly at every use — and compute both
  derived sums BEFORE either store. A free `volatile` read (at a site the
  original loads anyway) goes at the DEFINITION site; it cannot cross the
  callee-saved pushes or any store, which is the mechanism. Naming the
  intermediate POINTER advances the eax→ecx→edx rotation; so does one extra
  IR temporary where volatile is inert. Test the GLOBAL, not the local copy,
  to keep a list head's null-ness off the cursor. A store to ANY field of an
  address-taken struct — or to ANY global — kills CSE of an unrelated load.
- **Types.** A stashed u16 is often an `int` local — check the STORE width.
  `movsx`/`movzx` say signed/unsigned byte. `test byte ptr [mem],1` on byte +3
  of a word is `& 0x100` on the u16. `x & 0xff` and `(unsigned char)x` are
  different objects. A parameter's type can be the whole function (`char`
  where the caller declares `int`; `unsigned short` kept in `cx`).
- **Sums.** A flat three- or four-term sum is canonicalised — do not reorder
  one. A two-term sum's destination register follows the destination SYMBOL
  (`py += e` is the only spelling that keeps `py`). `tw <<= 1` and
  `tw = tw * 2` differ; `n - i - 1` and `n - 1 - i` differ; a named sum can
  BLOCK VC6's algebra; explicit parentheses stop FP-constant reassociation.
- **Floats.** Source order is the REVERSE of `fld` order, except at an
  accumulator. A float swap needs no temporary. `((float)a - b) * K` emits
  `fild/fisub`. A `volatile float` local is how an FP constant gets a stack
  home. Float constants are exact single-precision literals. Conversion order
  can be read off the operand displacements; `st(7)` depth forces siblings to
  memory operands.
- **Records.** A search's failure arm as the `else` of `if (p)` keeps ONE
  copy. Walk a list through its head GLOBAL for the 5-byte accumulator store.
  `memset` the whole struct then store the one non-zero field; `rep stosd` is
  NOT proof of memset; two zero registers in one small function is a
  sub-object `memset`. The record-unlink shape is one source compiled eight
  times in this tree (`ridemisc3.c`), null-head bug included. VC6 always
  jump-threads a provably-NULL pointer into a following `if (p)`.
- **Tells.** Size is NOT evidence of twinning — diff first, even within one
  body. Emitted store order is not evidence of source order. `xor
  <callee-saved>,<same> / mov eax,<it>` proves a two-predecessor join. Hand-
  written assembly shows as `xchg` against memory, callee-saved pushes INSIDE
  the stream, or an EBP frame in an `/O2` file (three sites known) — say so
  early. `__asm` operand names collide with MASM reserved words (`cr0`-`cr4`,
  `dr0`-`dr7`, `tr3`-`tr7`, `st`).

## Report format (in `docs/lanes/<scope>.md`, and in your final message)

Per function: address, name (and why, for anything you named or renamed),
instructions, percentage, `audit [OK]` yes/no, marker as committed, first
diverging index and residual for anything not `[OK]` — or "at its floor" with
the evidence. Then mechanics recovered (these are the spec for an eventual
browser runtime), callees or globals named for the first time, original bugs
reproduced, extern-type divergences, and every lever with its evidence.
