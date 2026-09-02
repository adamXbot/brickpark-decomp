# Handoff: continuing the LEGOLAND matching decompilation

Written 2026-09-02 for the next agent (Opus) picking this up. Read this, then
`docs/DECOMP.md` (the codegen playbook) and `docs/LANE_BRIEF.md` (the exact
text every matching agent gets). Everything below is verifiable from the repo.

## 1. Goal and where things stand

Goal: a reccmp-style matching decompilation of `original/legoland.exe` (VC6 SP3,
`/O2 /Gy /Gd`) — human-written C that compiles to the original machine code
function-by-function — and, eventually, a browser runtime that plays the game.
The recovered mechanics and data layouts in the commit messages and file
headers are the runtime's spec.

State (committed, `main` of `/Users/systemadmin/Downloads/legoland/legoland`):

| | |
| --- | --- |
| Functions exact (committed `// FUNCTION:` markers) | 678 |
| Code exports exact | 645 of 675 (95.6%) |
| Recovered internal (unexported) functions | 32 |
| Exports still to finish | 30, but 15 are already exact and tooling-blocked, so **15 real** — `python3 tools/remaining.py` |
| **Unmatched callees (unexported)** | **584, ~39,300 instructions — `python3 tools/callees.py`** |

716 symbols are exported; 41 are data, so the denominator is 675. Beware:
`tools/audit.py`'s `true_extent` will happily disassemble a data symbol and
report a plausible instruction count, so a naive "unmatched exports" script
invents targets. `SPRITE_ClipRect` 0x004bdea0 is the known case — it is the
full-screen clip RECT `{0, 0, 640, 480}`, declared as data in gpu.c, rin.c,
bigrender.c and printlist.c. Cross-check any "new" export against the
disassembly before assigning it to a lane.

Held as `// WIP-FUNCTION:` in committed files:

- Genuine partials: `UpdateControllerFromMouseData` 0x00473b00 (102/109 —
  the residual is an allocator state; two agents exhausted every C spelling
  and every `/O2`-compatible option; leave it), `InsertChildIntoList`
  0x00475630 (78.5%), `LoadPalette` 0x00441f20 (80.6%, RGB565 red term),
  `InitExitCheckBox` 0x0048f0f0 (92.2%, zero held in ebx).
- Exact by `tools/audit.py` but held only because the shared `tools/match.py`
  cannot bound them: eleven void tail-jump wrappers, `RenderFrontEndScreen`,
  `KillAllSamplesFromSource`. See "Tail-jump functions" in DECOMP.md; the
  fix is a one-function change to match.py that has NOT been applied because
  match.py is shared (§3).

## 2. Toolchain and tools

Compile (from the repo root):

```bash
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "/Users/systemadmin/Downloads/alpha team/alphateam/tools/wibo-msvc/cl" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/x.obj LEGOLAND/foo.c
```

| Tool | Role |
| --- | --- |
| `python3 tools/disasm.py original/legoland.exe <RVA>` | disassemble the original (RVA = VA − 0x400000) |
| `python3 tools/matchfull.py LEGOLAND/foo.c Name 0xVA --obj /tmp/x.obj` | iteration tool: full-body diff; over- or under-reports at the edges |
| `python3 tools/audit.py LEGOLAND/foo.c [...]` | **the gate**: true extent by control flow (ret / unconditional jmp nothing jumps past, switch tables followed), compiled body trimmed to it, instruction count + byte length + strict index-for-index + no escaping branch. Prints `[OK]`/`[WIP]`/`[REJECT]`, ends `PASS`/`FAIL`. |
| `python3 tools/verify.py` | the SHARED gate (stops at the first `ret`); must stay green — `WIP-FUNCTION` markers are ignored by it |
| `python3 tools/progress.py` | Codex's report generator; reads the WORKING TREE, so run it on a clean checkout |
| `symbols/legoland.exports.txt` | `name ordinal RVA` |

Markers: `// FUNCTION: LEGOLAND 0x<VA>` on the line immediately above the
signature (`//` form only; a `/* */` block is silently ignored; the verifier
looks 1–3 lines ahead). Partial: `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <reason>)`.

Counting: always from committed markers —
`git ls-files 'LEGOLAND/*.c' | xargs grep -h '^// FUNCTION: LEGOLAND' | wc -l`.
verify.py and progress.py include in-flight lane files.

## 3. Constraints that are not in the code

- `tools/match.py` and `tools/verify.py` are shared with Codex (out of credits
  until 2026-09-08). Do not change them without coordinating. `tools/audit.py`
  and `tools/matchfull.py` are ours.
- Do not edit `web/**` (Codex's). The browser runtime work should be
  coordinated there once the decomp is done.
- Extern prototype TYPES are caller-side codegen levers (`unsigned short` vs
  `int` parameters decide a 16-bit load). The original's headers and TUs
  disagreed in places (`RemovePathTile` in maprestore.c vs pathtile2.c,
  `LoadSpriteIcon` in saveprof.c vs iconui.c). Never "align" an extern to its
  definition without re-auditing every file that declares it.
- Never commit a prologue-only or fabricated-tail body as `// FUNCTION:`.
  Semantics come first; reproduce original bugs faithfully and comment them.
- Report matches as "normalized instruction match" (relocations and branch
  targets are normalised), not byte identity.
- Commit attribution: use whatever your session's rules say. Commit messages
  should list the functions per file and the recovered mechanics / layouts
  (they are the runtime's spec) and the codegen levers learned.

## 4. The per-batch integration checklist

Run this for every lane file before committing it:

```bash
cd /Users/systemadmin/Downloads/legoland/legoland
# duplicate addresses across all files (must print nothing)
grep -rhoE '//\s*(WIP-)?FUNCTION: LEGOLAND 0x[0-9a-fA-F]+' LEGOLAND/*.c | grep -oE '0x[0-9a-fA-F]+' | tr 'A-F' 'a-f' | sort | uniq -d
# block-comment markers (must print nothing)
grep -n '/\*.*FUNCTION: LEGOLAND' LEGOLAND/<file>.c
# the gate
python3 tools/audit.py LEGOLAND/<file>.c
# /W3 clean
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "/Users/systemadmin/Downloads/alpha team/alphateam/tools/wibo-msvc/cl" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/w3.obj LEGOLAND/<file>.c
# shared gate still green (read its total line; in-flight files also appear)
python3 tools/verify.py | tail -1
```

Then `git add` only the lane files (never `scratchpad/`, never `vc60.pdb`),
commit, and recount committed markers. Update the status block at the top of
"## Status" in `docs/DECOMP.md` when the tally moves.

## 5. Work in flight, and where the value now is

**Read this before launching another round of export-chasing.** The export
figure (95.6%) counts only the symbols the linker exposed, about half the game.
`python3 tools/callees.py` lists 584 addresses the matched files call through an
`extern` declaration that nobody has matched — roughly 39,300 instructions of
behaviour the reconstruction names but does not reproduce. By whole functions
the project is 708 of ~1,292 known (54.8%).

The 15 remaining genuine export partials are deep register-allocation puzzles
with steeply diminishing returns: two consecutive rounds cost about 4.4M
subagent tokens between them and yielded one function, though they did leave
precise measurements in every WIP note. The unexported internals are worth far
more per token and are what a browser runtime actually needs.

Workflow `ll-batch22` (7 lanes) was running when this was last updated, and it
is the first round aimed at that work: `savechunks.c` (the Save/LoadBlock and
Save/LoadScripts writers, which COMPLETE the .sav format), `person3d.c`
(Draw3DPersonModel, 1023 instructions, the largest unmatched function in the
game), `joust.c` (the Joust and Temple Slide callback sets, the two rides whose
GetInterfaces are already matched so every slot's purpose is known),
`ridecb1.c` and `ridecb2.c` (the 0x42xxxx and 0x43xxxx ride-callback clusters),
`simcore.c` (RequestRoute, ScanBlokeSurroundings, GetPathNeighbours,
UpdateMapDrag, TriggerSwitch) and `softblit.c` (the CPU rasterisers).

If that session is gone, the lane files are on disk with `// WIP-FUNCTION:` on
anything unfinished. Audit, commit what is `[OK]`, and relaunch from
`docs/LANE_BRIEF.md`. Then run `tools/callees.py --by-file` and take the next
cluster: 221 of the unmatched callees, ~18,800 instructions, are the ride
callback sets that `SetCustomCallbacks` installs.

Note when writing new files: ALWAYS give an `extern` declaration a trailing
comment with the callee's address (`/* 0x0043ffd0 */`). That comment is what
`tools/callees.py` reads to track what is still missing.

## 6. What comes after batch 18

Exports with no marker anywhere on disk at handoff (size = true extent in
instructions):

| insns | VA | name | suggested lane |
| --- | --- | --- | --- |
| 188 | 0x00489190 | RenderTransSprite | printlist.c's lane missed it; new file `render3.c` |
| 188 | 0x0045eb30 | BuildObject | with objmap2's placement cluster (new file `objmap3.c`) |
| 470 | 0x0044e010 | __BMPLoader | bighelp.c (in flight) |
| 483 | 0x00463870 | InitScreen | `screen.c` with SetCustomCallbacks |
| 578 | 0x00452c20 | SetCustomCallbacks | `screen.c` — the class-name → callback-set dispatcher (see ridesave.c GetInterfaces) |
| 903 | 0x0045b180 | RenderView | `renderview.c` — the isometric renderer; recover the draw order |
| 962 | 0x004724a0 | DrawPopUpInfo | `popup.c` — every field the pop-up shows and its source |
| 971 | 0x0047e980 | LoadGame | `savegame.c` with SaveGame (do SaveGame first) |
| 1161 | 0x004567a0 | RenderFullMap | `fullmap.c` — the overview-map draw callback |
| 1196 | 0x0047d8e0 | SaveGame | `savegame.c` — the whole .sav format; chunk framing is in profiles.c, per-ride sections in ridesave.c |

(BuildObjInfoList, PopUpInfoSetUp, MakeUpObjectList, RenderBuildObjectIcon
also show as unmarked; they belong to fpui2.c's in-flight lane.)

The giant-function lane briefs (`ll-batch16`, never ran: session limit) are in
the workflow script directory —
`~/.claude/projects/-Users-systemadmin-Downloads-legoland-legoland/c4caa6e2-7d89-4969-bd0f-50c75064bb23/workflows/scripts/ll-batch16-*.js` —
with per-function context lists worth reusing. For these, the recovered
format/algorithm is the deliverable even if the match stalls at 95%.

After the exports: (a) the unexported internals that the runtime needs
(callees left as `extern` in each file — grep for `unexported`), (b) the
tail-jump promotion once match.py is fixed, (c) the browser runtime in
`web/**` (coordinate with Codex).

## 7. How the lanes are run

- One lane = one NEW file + a disjoint function list (with sizes and 3–6
  "matched context" files to read). Never two lanes on one file.
- Prompt = `docs/LANE_BRIEF.md` verbatim + lane name, file, state, function
  list, "finish by running audit.py and report whether it ends PASS".
- Structured report per function: address, name, pct, audit_ok, marker, note
  (first diverging instruction + hypothesis if not exact), plus mechanics
  recovered and what remains. Keep that schema; it makes integration cheap.
- Concurrency: **at most 7 lanes at once.** The account's session limit was
  hit twice at 10–16 concurrent agents (each hit killed every in-flight lane).
  Because lanes keep `// WIP-FUNCTION:` until audit passes, an interrupted
  lane's file is safe to continue from — relaunch with "your file exists,
  audit first".
- Single hard functions go to one agent each, iterating in scratch copies
  under `scratchpad/<name>/`, splicing into the committed file only at a
  strictly better body. The earlier attempts' logs are there
  (`scratchpad/ucfm*`, `cstm`, `rbm`, `lbm` for LoadBaseMap).
- The playbook grows every batch: when a lane reports a new lever, add it to
  the "VC6 SP3 codegen levers" section of DECOMP.md and to the brief.

## 8. Things that went wrong before (so they do not again)

- Counting on-disk markers overstated the tally twice; count committed.
- A lane brief assembled by text surgery still pointed at a committed file;
  grep briefs for stale write targets.
- "Aligning" an extern's parameter type to its definition changed callers'
  codegen and briefly committed a FAIL; the extern was restored and documented.
- `matchfull` reported 77% for an exact switch function (it decodes the
  post-`ret` jump table) and 100% for prologue-only stubs (it truncates to the
  compiled length). `audit.py` exists because of both.
- Lanes that marked bodies `// FUNCTION:` while iterating left 18 false
  claims when the session died. The brief now forbids it.
- A stray `vc60.pdb` appeared in the repo root from a lane compiling with
  `/Zi`; the brief now forbids that too.
