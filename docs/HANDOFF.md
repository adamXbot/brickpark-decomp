# Handoff — LEGOLAND matching decompilation

**Checkpoint: 2026-09-03, 10:55 AEST (Thursday).** Written for the next session
to pick up cold. Everything below is verifiable from the repo; where a number is
quoted, the command that produces it is given.

Read this, then `docs/DECOMP.md` (the living codegen playbook),
`docs/LANE_BRIEF.md` (the verbatim text every matching agent gets) and
`docs/RIDE_CALLBACKS.md` (what each ride callback is for).

---

## 0. Environment note — session of 2026-09-03, ~12:00 AEST

Picked up cold on a machine that has the repo but **none of the local
prerequisites**. Verified absent: `original/legoland.exe`, `gamedata/`, the
`toolchain` symlink target, the old checkout under `~/Downloads/legoland`, and
the `wibo-msvc/cl` wrapper hard-coded in `tools/match.py`, `tools/audit.py` and
`tools/matchfull.py`. Consequences:

- The repo now lives at `/Users/systemadmin/Documents/Development/Github/legoland`.
  Section 4 below, `docs/LANE_BRIEF.md`, `docs/RE_CONTEXT.md` and the three
  `CL` constants still name the Downloads paths; update them when the
  toolchain is put back (the `CL` constants are in the shared files — see §2).
- Every matching tool is blocked until the binary and toolchain are restored:
  `match.py`, `audit.py`, `verify.py`, `matchfull.py`, and also `coverage.py`,
  `remaining.py` and `callees.py`, which import `audit.true_extent`/`load_exe`.
  Only `tools/progress.py` runs (it reads the committed markers).
- Homebrew's Python refuses `pip install` (PEP 668). A venv with `capstone` and
  `pefile` is at `~/.venvs/legoland`; run the tools as
  `~/.venvs/legoland/bin/python tools/<tool>.py` or activate it first.
- Done this session: the committed progress report was stale (not regenerated
  after `a8d4533`), so `tools/progress.py --check` — the CI gate — failed; it
  is regenerated. The README status paragraph was two months stale
  (254 matches) and now quotes the checkpoint numbers.
- CI: the two recorded runs (2026-09-01) pass `tools/progress.py --check` and
  fail only at `actions/configure-pages`, and the workflow was then
  `disabled_manually`, which is why no run fired for any push after that date.
  **Root cause, established 2026-09-05: the repository is private**
  (`gh repo view --json visibility`), and GitHub Pages is unavailable on a
  private repository outside Enterprise Cloud — so the Pages API 404 is not a
  missing setting, it cannot be enabled at all while the repo stays private.
  Fixed here by splitting the workflow: `report` (the real gate, pure) and
  `pages`, which is skipped unless `github.event.repository.visibility ==
  'public'` and carries `enablement: true` so Pages self-configures the day the
  repo is made public. The README badge and report link now point at the
  in-tree `docs/LEGOLANDPROGRESS.SVG` / `.HTML`, which GitHub renders for a
  private repo. **The one remaining human step is `gh workflow enable
  "Decompilation progress"`** — a repo settings change, left to the user.
- The 62 audit-exact WIPs of §2 can be listed without the binary — 58 of them
  say so on the marker line:
  `grep -h '^// WIP-FUNCTION' LEGOLAND/*.c | grep -iE 'exact|100%' | grep -i audit`.

**Update, ~13:45 AEST.** The user supplied `~/Downloads/legoland.zip`, the
old checkout from the other machine. Restored from it into this checkout
(all gitignored or untracked, none committed): `original/legoland.exe`
(SHA-256 verified), `gamedata/` (222 MB), `scratchpad/` (180 MB, every lane's
notes and repro pairs), `reccmp-user.yml`. The old tree was clean at `3a695b8`,
so no lane work was lost. Its `.git` held five `codex/*` branches (59–72
commits each) of which only `codex/browser-runtime` is on origin; all five are
fetched into this clone as local branches — push them if they are wanted.

**The compiler was rebuilt, not copied.** `toolchain` in the zip was only a
symlink into the missing "alpha team" tree, but `adamXbot/alphateam` (cloned
to `../alphateam`) tracks `tools/wibo-msvc/cl` and
`tools/setup_toolchain_macos.sh`, which downloads wibo 1.2.0, decomp.me's
`win32/msvc6.3` package and isledecomp's SP3 libs into `toolchain/`. Set
`LEGOLAND_CL=/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl`
before any tool that compiles; `match.py` passes `ALPHATEAM_VC6_ROOT` itself.

**The `match.py` port is validated on the real binary** as far as it can be
without compiling: the ported `true_extent` returns the same result as the
pre-port `audit.py` on all 1580 marker addresses, the widened `norm` agrees
with the old `norm2` on every instruction of every original body, and
`coverage.py` (which uses the ported walker) reproduces the checkpoint's
38.3% / 51.3% exactly. `remaining.py` and `callees.py` also run. What is
still owed was the compile side — closed the same afternoon once the VC6
toolchain was rebuilt from `adamXbot/alphateam`'s
`tools/setup_toolchain_macos.sh` (into the gitignored `toolchain/`, C2.DLL
12.00.8447 confirmed) with `LEGOLAND_CL` pointing at that repo's
`tools/wibo-msvc/cl`: `audit.py` PASS, 62 promotions, `verify.py` 1473/1473.

---

## 1. Where the project stands

Goal: human-written C that, compiled with the VC6 SP3 toolchain the game shipped
with (`/O2 /Gy /Gd`), reproduces `original/legoland.exe` function-by-function —
and eventually a browser runtime. The recovered mechanics in the file headers
and commit messages are that runtime's spec.

| measure | command | value |
| --- | --- | --- |
| **bytes of game code matched** | `python3 tools/coverage.py` | **59.5% exact, 70.5% with partials** |
| functions matched exactly | `git ls-files 'LEGOLAND/*.c' \| xargs grep -h '^// FUNCTION: LEGOLAND' \| wc -l` | 2383 |
| exported functions | `python3 tools/remaining.py` | 665 of 675 (98.5%) |
| unmatched callees | `python3 tools/callees.py` | 91, ~4,700 instructions — 46 of them game code (2,564 insns: Codex-F's 27 and scope O's 19), the rest CRT/import thunks |
| partials (WIP markers) | `python3 tools/audit.py LEGOLAND/*.c` | 73 |
| **unmatched functions, whole binary** | `python3 tools/inventory.py` (scope N) | **867: 703 live (41,523 insns, 70% of the unmatched bytes), 164 dead, one 8,085-instruction body** — `docs/lanes/scope-n.md` |

(Row values current at wave TEN, 2026-09-05. Section-B waves one to four took
30 partials plus one new twin to 1504/1504, then 15 (1519), 10 (1529) and 11
(1540); waves five to seven added 4 more (1544) for roughly twenty lanes and
several thousand measured variants, and waves eight, nine and ten closed
nothing at all. Fully exact files: schoolcar, screens3, castleobj, bnvmove,
tri3d, bigscreens, softblit, westtown, westtown2, ridecb1.

**The strategy change at wave eight is the thing to carry forward.** With the
close rate collapsing, every lane was redirected from variant search to hunting
RECONSTRUCTION ERRORS — reading the original instruction by instruction and
asking "what C statement produces exactly this?". Waves eight and nine returned
twenty-six such errors including a live one (a local named `cr2` passed to a
fixed-point macro expanded to `__asm { mov cr2, eax }`; MASM resolved it to the
CONTROL REGISTER and emitted a privileged `0f 22 d0`, so the shipped body would
have faulted at ring 3, and the following compare read an uninitialised slot).
Wave ten returned four more plus a formal retirement. **A wave that closes no
function is not a wasted wave** — it is where the mechanisms get named, and
where wrong entries in `docs/DECOMP.md` get caught: roughly twenty recorded
rules have now been corrected or withdrawn by the lane that measured them, most
of them summaries rather than measurements.

**WAVE TWELVE CHANGED THE STRATEGY, AND IT WORKED — read this before picking
targets.** Waves eight to eleven ground the 37 partials and closed NOTHING in
four waves; wave twelve wrote NEW functions from the §6C frontier instead and
closed **ten**, 1834 instructions, moving coverage 44.9% -> 45.7% exact
(51.3% -> 53.0% with partials) — the first coverage movement in six waves. Four
lanes, four new files: `logflume3.c` (the LF set-piece placement family),
`schoolcar2.c` (the manoeuvre choosers), `joust2.c` and `mappath.c` (the
map/walk-path cluster). The frontier fell from 588 functions / ~25,500
instructions to 579 / ~22,000.

**Wave thirteen (2026-09-05) confirmed it at scale: 38 new exact functions in
one round** — 26 from four lanes here (`schoolcar3.c`+`roads2.c`,
`screencb.c`, `logflume4.c`+`popup2.c`, `mapscreen2.c`+`render3.c`) and 12 from
a parallel session working the scope in `docs/SCOPE_FABLE_A.md` on its own
branch (`goldrush2.c`, `jcroute.c`, `bswater.c`, `objrect.c`; levers in
`docs/lanes/fable-a*.md`, folded into DECOMP). Coverage 45.7% -> 47.9% exact.
The screen-callback lane alone closed 13, because the ~23 `CB_*` callbacks
`screen.c` declares are one shape with per-class data, and one carefully built
first body transferred to the rest. **The parallel-session pattern works:**
disjoint new files, a branch, no shared-doc edits, no `verify.py`; integration
was one merge with zero conflicts. Reuse the scope file as the template.

**Wave fourteen (2026-09-05): 39 more exact in four lanes** — `screencb2.c`
17 of 17 (the remaining screen callbacks, grouped by ObjDef slot, one body per
group then sibling diffs; nine first try), `fpui3.c`+`mapscreen3.c` 9 of 9,
`schoolcar4.c`+`logflume5.c` 11 of 13, `coaster3d.c` 2 of 5 with two of
`schoolcar3.c`'s coaster WIPs improved. Coverage 47.9% -> 49.5% exact. Two
corrections to this document's own triage came out of it (the free-volatile
test is a signal, not a proof; the merge-site rule is "the fall-through copy
survives", not "first site"), and one open tooling defect: the extent walker
under-bounds a function whose middle contains a rotated loop's entry `jmp`
(`Coaster3D_BuildPieceGeometry` can never print `[OK]` until it is fixed; see
the DECOMP entry).

**Parallel scope B merged (2026-09-05): 13 of 18 exact** (`savechunks2.c`,
`workorder3.c`, `ridemisc.c` 6 of 6, `sysmisc.c`), zero conflicts again;
levers in `docs/lanes/fable-b*.md`, folded into DECOMP. Coverage 49.5% ->
49.9% exact. Two scopes, two clean merges: the pattern is established.

**Wave fifteen (2026-09-05): 40 more exact in four lanes — the best round yet,
and coverage passes HALF: 49.9% -> 51.2% exact.** `logflume6.c` 7 of 7 (the
log flume subsystem is now complete), `screencb3.c`+`fpui4.c` 14 of 15,
`goldrush3.c`+`ridemisc2.c` 10 of 11, `coaster4.c`+`schoolcar5.c` 9 of 12. The
frontier is down to 539 functions / ~12,900 instructions from 588 / ~25,500
when the pivot began four rounds ago. Two cautions came out of it: size is NOT
evidence of twinning (two 121-instruction functions shared nothing), and a
twin's block layout is a hypothesis, not an inheritance — diff first, always.

**Wave sixteen (2026-09-05): 40 more exact, coverage 51.2% -> 52.2%.**
`sysmisc3.c`+`screencb4.c` 13 of 13, `bswater2.c` 8 of 8 (the boating-school
boat mover complete, including its 294-instruction leg stepper),
`logflume7.c`+`goldrush4.c`+`fpui5.c` 10 of 11, `coaster5.c`+`schoolcar6.c` 9
of 12. A THIRD hand-written-assembly site was identified on three independent
proofs (`ZBuffer_FillPoly`, 0x00423350, after the four `tri3d.c` rasterisers).
Two callee externs were found to be misnamed by their callers (an ICM error
reporter declared as a loader; a .TSF unloader declared as ODF) and are kept as
named for resolution, flagged in `sysmisc3.c`'s header.

**Wave seventeen plus three parallel scopes (2026-09-05): +208 exact in one
round, coverage 52.2% -> 55.0%.** Four lanes here closed 59 (`screencb5.c`+
`ridemisc3.c` 21 of 22, `workers3.c`+`objdoor.c`+`posstep.c` 18 of 18,
`coaster6.c`+`schoolcar7.c` 11 of 12, `render4.c`+`bswater3.c` 9 of 11);
two Codex agents on `docs/SCOPE_CODEX_A.md`/`_B.md` closed 63 and 60 (the
coaster micro-subs, all named; the tiny screen callbacks, audio and UI); the
Fable session's scope C closed 26 of 26. **Five parallel sessions, five clean
merges.** The scope files are the template. The frontier is 390 functions /
~7,500 instructions, down from 588 / ~25,500 six rounds ago.

Also this round: a rename pass resolved 452 `Sub_<addr>` placeholder externs
to the names their bodies now carry (names are not codegen levers; every
touched file re-audits unchanged), which exposed and fixed a pre-existing
duplicate definition (`RestoreCoasterCar` at two addresses); `coaster.c`'s
malformed nine-digit extern comment was corrected; and the extent-walker
defect that blocked two complete bodies (`MatMul`, `Coaster3D_BuildPieceGeometry`)
**is fixed** (`_loop_entry` in `tools/match.py`, 2026-09-05): a forward `jmp`
whose skipped region is branched back into from beyond its target is a rotated
loop's entry, not the function's end. `MatMul` promoted to exact at its true
40 instructions; `Coaster3D_BuildPieceGeometry` correctly bounded at 143.
Verified against the whole tree — no other function's extent changed.

**Codex scope C merged (2026-09-05): 88 of 88 exact** — the frontier's
1-28-instruction tail across `simcore2.c`, `pathmisc.c`, `lfmisc.c`,
`sysstubs.c`, `screencb7.c`; eighty-plus first compile. 2020/2020 verified.

**`origin/codex/audio-extraction` is deliberately NOT merged (user decision,
2026-09-05).** It forked on 1 Sept (`73059e05`) before all of this session's
work: 80 files / ~20,600 lines, mostly the browser runtime (`web/`, `runtime/`,
`tools/test_*.js`, `docs/BROWSER_PORT.md`, `docs/PLAYABLE_WORKLOG.md`). It
conflicts on `README.md`, `docs/DECOMP.md`, the progress report,
`tools/match.py` (+585/-44, a relocation-aware rewrite) and
`tools/matchfull.py` (gutted to a wrapper) — i.e. with the extent port the
whole verify gate rests on. Of its 120 marked functions, 115 are already on
`main` and **20 are duplicate definitions in different files** (all exact on
both sides — e.g. `FindPathRect` in its `pathoverlay.c` vs `workorder4.c`
here, `ControlGardeners`/`ControlMechanics` in its `blokemisc.c` vs
`workers3.c`); only 5 are net-new. Integration plan when wanted: keep `main`'s
tools, take `web/`/`runtime/`/tests/docs, take the 5 net-new functions, drop
its 20 duplicate bodies, gate every touched LEGOLAND file, `verify.py` alone —
or have the branch owner rebase with those rules.

**Wave eighteen plus five parallel scopes (2026-09-05): +290 exact, coverage
55.5% -> 58.1%.** Three lanes here closed 59 (`uimisc3.c` 23 of 23,
`coaster8.c`+`savemisc2.c` 30 of 30, `lfmisc2.c` 6 of 6) and took
**`MusicThread` — 3,161 instructions, the largest unmatched body in the
project — to 7 mismatches (99.78%)**: it is real, it is C, and 81% of it is
two macros written out thirty times; its residual is one import-caching
placement stated in its note. Merged in the same round: the Fable session's
scope D (34 of 34), Codex scope D (49 of 49), **scope E (147 of 147 — the
frontier's 1-17-instruction tail)** and scope I (the UI/system/render
partials: one close, two improvements, twelve measured floors). Ten parallel
scopes have now merged with zero conflicts. `docs/PARALLEL_CONTRACT.md` is
the shared contract; the scope files are one page each.

**Scopes K, L and M merged (2026-09-05), and the gate grew a step.** K:
`movie.c`, `pathmask.c`, `texture.c`, 28 of 28 exact (the AVI player, the path
masks, the texture records; levers folded into DECOMP under `scope-k`). M:
`docs/LEVERS.md` — the ~750-entry lever corpus consolidated to 194 rules with a
symptom index; the contract now sends new sessions there first and to DECOMP's
top entries for anything newer than commit 36018920. L: `tools/relocs.py` and
the first relocation sweep. **The normalised gate cannot see which same-sized
global an operand names; relocs.py can.** Its sweep of 2,355 exact bodies found
77 strict differences in 20 functions (seven real errors, thirteen
operand/statement order), and the per-file gate found one more in K's
`RunMovie`. Seventeen were fixed in this checkpoint (files no running scope
owns; each re-audited, /W3 clean, zero MISMATCH; the levers are at the top of
DECOMP). `integrate.sh` and §4 now run `relocs.py` per file. **Four fixes are
DEFERRED to the owning scopes' merges — apply them at the quiet-tree gate the
moment F and H land, then `relocs.py --all` must report zero MISMATCH lines:**

- `mechrides.c` (scope F) `PlaneRide_Create` 0x0043dda0 i34-36: the three
  stores go to distinct destination globals 0x0062fe84/88/8c, not back into
  the source pointers `g_plane_bnv0/1/2` (0x0062fe90/94/78). Declare three
  externs at 0x0062fe84/88/8c and store to them; `PlaneRide_Destroy` (reads
  0x62fe90/94/78) and ridemachine.c's use are right as they are.
- `mechrides.c` (scope F) `Copters_Activate` 0x00404be0: both switches
  (boarding i73-84, alighting i145-156) map cases 0..4 to the paths at
  0x004c112c, 0x004c1124, 0x004c1128, 0x004c1130, 0x004c1134 = `g_copters_path4,
  path0, path1, path2, path3`; the source has case 0 -> path2, case 3 -> path3,
  case 4 -> path4 (cases 1 and 2 are right). Change cases 0, 3 and 4 in BOTH
  switches and confirm against the jump tables at 0x00404ef8 / 0x00404f0c.
- `logflume.c` (scope H) `LFTrack_Update` 0x0040c4a0 i79/81: the byte sum
  loads `g_lf_footprint+4` into BL and `g_mapref+4` into AL; ours is the other
  way — swap the two addends (the "right operand is loaded" rule).
- `logflume2.c` (scope H) `LFPiece_TickCommon` 0x0040d3b0 i17-20: the chained
  assignment stores 0x2034 to `g_lf_tool_a, _b, _c, _d` (0x4cbdd8, 0x4c2a88,
  0x4c5c90, 0x4c74c8) in that order; ours stores d,c,b,a. Reverse the chain
  (a chain stores right-to-left) and measure.

Name hygiene from the sweep: 0x00829a3c is `g_clip_ring` in coaster3d.c,
coastertiny.c and coaster9.c and `g_coaster_regions` in schoolcar.c (one
object, two struct views) — rename at a quiet tree.

Open for assignment after this checkpoint: `SCOPE_CODEX_F.md` (unclaimed),
`SCOPE_P_game_frame.md` and `SCOPE_Q_game_main.md` (the startup spine — the
frame dispatcher, the in-game frame, the map click handler, the main loop —
cut from the inventory's groups 16 and 17 on 2026-09-05). Running: F (this
machine), G, H, O (19 new functions K exposed). Merged since: N
(`tools/inventory.py` and `docs/lanes/scope-n.md`, 2026-09-05; no C, no
existing tool touched). Tree-wide `relocs.py --all` after this checkpoint:
2,383 checked, 17 mismatched positions, all in the four deferred functions.

**The runtime spec exists (scope J, merged 2026-09-05).** `docs/RUNTIME_SPEC.md`
indexes eleven pages under `docs/runtime/` — world, persistence, assets,
transport, attractions, presentation, a callback-registration index for all
83 named classes — consolidated from every file header and lane note, with
each fact cited to its source file and a "reconciled disagreements" section.
Read its coverage table (199 files "documented") as a claim to sample, not a
verification: the per-file boundary column is boilerplate, and the spec says
itself that it is documentation coverage, not runtime equivalence. Original
bugs are listed as behaviour a runtime must know about.

**Extern-name hygiene (2026-09-05).** Two rename passes have resolved
placeholder extern names (`Sub_<addr>`, `sub_<addr>`, `CB_<addr>`,
`<Name>_<addr>`) to the names their bodies now carry — 452 earlier, 234 more
today — each gated file re-auditing unchanged, because symbol NAMES are not
codegen levers. **What remains is ~240 extern declarations whose name is a
REAL name that differs from the defined name at the same address** (e.g.
`Road_ProbeOrtho` vs `Road_CardinalGroup`, `SoundTimeMS` vs `GetTicks`,
`SkipProgressScreen` vs `InitTutorialScreen`), spread over ~50 files, plus 29
placeholders in files owned by running partial scopes. A wholesale rename of
those was tried and REVERTED: it exposed same-name declarations with different
types (`C4028`), one redefinition, and a tell-tale off-by-one chain in
`screens3.c` where each extern's comment address appears to belong to the
NEXT function — i.e. some of these are wrong ADDRESSES, not wrong names, and
renaming would hide that. They need a per-file review that checks which of
the name and the address is wrong; `docs/SCOPE_L_relocation_sweep.md`'s tool
is the right instrument, and the list can be regenerated with the scan in the
commit that recorded this.

The arithmetic behind the pivot is simple and worth restating: the 37 partials
are worth almost nothing in BYTES even if every one closed, while the frontier
holds ~22,000 instructions of unwritten behaviour. Grinding a residual competes
for the same lane-hours as writing a whole new function, and the new function
almost always wins. **Default to the frontier; go back to a partial only when
the §6B triage says its residual is STRUCTURAL.**

Wave twelve also showed the two directions feed each other. Wave eleven had
recorded `LFDrop_Place`'s EBX tie-break as forced and unreachable; a wave-twelve
lane writing fresh siblings found the construct that flips it (one shared
two-byte aggregate instead of two `unsigned char` locals — 164 of 217 down to 5
on `LFTunnel_Place`), then tested the boundary and showed it does NOT transfer
back to `LFDrop_Place`, and why. Writing new code in a family is often the
cheapest way to solve an old residual in it.

**Wave ten ran four Opus 5 lanes concurrently with no kills.** Both models
work and lanes are model-agnostic; wave four ran on Opus 5 because the Fable
quota was exhausted mid-wave. Operational limits: the account session limit hit
at seven and again at four concurrent Fable lanes, so the working cap there is
3, while four Opus lanes ran clean; the API also returned 529 overloads for a
stretch and killed lanes at launch — back off ten minutes rather than retrying
in a loop. Every kill left the files compiling clean with honest markers, and
two closes were recovered from disk after the lane that made them died before
reporting. ~200 levers were added to `docs/DECOMP.md` across these waves; the
prose below predates all of this.)

**Functions now formally EXHAUSTED — do not re-grind** (each note records the
proof): `UpdateControllerFromMouseData` (102/109, an allocator state no C
construct reaches), `ValidateCursor` (5; the store its residual needs was
displaced by the scheduler, so no source ordering reaches it), and
`BoatingSchool_Add` (8; the full 36-body cross-product of take-position x
link-order is a unique minimum), and — added 2026-09-05 — **`Draw3DPersonModel`
(0x00440a30, 377 of 1023)**, retired after five waves on two independent
proofs. The body is instruction-, register- AND immediate-identical to the
original at all 1023 positions (only 7 of 1023 differ under full blinding; the
instruction count is exactly the original's 1023; mnemonic LCS 1016/1021), so
no mnemonic, field offset, store width, branch direction or block layout can
still be wrong. 365 of the 377 are one frame permutation, unreachable because
(a) our surviving-IR reference profile equals the original's slot for slot
(384 = 384 references over 79 = 79 slots), so no spelling can change the
weights, and (b) probing the rank function by SIZE — growing `mt` from 36B to
48B — forces any linear key into a range in which the two contended objects
cannot swap. 10 are scheduling tie-breaks with named mechanisms, each measured
at 25-600 to disturb; the last 2 are a commutative-sum canonicalisation
invariant under every spelling tried. The only construct known to reach the
original's frame order costs 24 extra instructions, destroying the exact 1023.

**Scope I (2026-09-05) reviewed all fifteen UI/system/render partials under
the contract's partial rules: `UpdatePersonPos` closed, `PaintTileLayer` and
`RenderFullMap` improved, and twelve were retained as measured floors with
their tests recorded above each marker** — `RemoveNewObjectMarker` (the
cursor-anchor coupling), `UpdateSampleSource` (improving one arm regresses the
shared tail), `BuildPTPRoute` (rb 0, every temporary route costs),
`UnlinkGardenerOrder` (the early head tail cannot survive), `DrawPopUpInfo`
(a real memory home for `halfw` repositions the frame), `GetObjectUID`,
`LoadScriptEvent` (the two-predecessor head-zero join cannot be placed last),
`ScrollIconPanel` (the four-value web rank does not move),
`CheckWorkerOnMouseStatus`, `InitExitCheckBox` (five prior passes; a
constant-web floor), `RenderCursor`, `RenderView` (the two documented
placement corrections regress when applied together, 381 -> 549). "Floor"
there means the best measured form after the listed tests, not a proof over
all C — reopen only with new reconstruction or compiler evidence.

**Five more retired at wave eleven (2026-09-05).** All were chosen as the
project's closest partials by strict mismatch and all turned out to be AT their
floors — which is the evidence behind the ranking qualifier below.
`RequestRoute` (3) is closed by ARGUMENT rather than exhaustion: a store to any
field of an address-taken struct kills CSE availability of an unrelated load, so
such a value is either the earlier test's register web or a fresh load, and the
register-to-register copy the original has is a third regime that does not
exist. `WW_AnyBlokeInRect` (12 of 31, register-blind 0) — three free `volatile`
reads, singly and together, are byte-identical, so the residual is a global web
RANK and no barrier or ordering construct reaches it; hand-naming the rect
fields is measurably worse, confirming those hoists are VC6's own.
`InsertChildIntoList` (22 of 68) — endorsed by a third independent lane, which
re-read the whole body against the disassembly index for index and found no
reconstruction error. `ClampPopUpToScreen` (3 of 43) — its one-byte deficit is
fully accounted for as `cmp esi,eax` against the original's `cmp esi,25h`, and
the 2-mismatch alternative is deliberately NOT taken because the original's
immediate compare says the source wrote `< 0x25`. `JcBoat_Animate` (3 of 330) —
the retired claim was re-verified rather than inherited; VC6's own strength
reduction of the array subscript is what the original has.

**Quote coverage.py.** The export figure (95.6%) badly overstates completion —
exports are only the symbols the linker exposed, and 1411 functions are matched
against just 645 exports. The callee figure moves in *both* directions, because
each new file declares externs for its own callees; it measures the frontier,
not progress. Only coverage.py has a fixed denominator (~628 KB of game code,
excluding ~51 KB of statically-linked CRT above `0x0049e000`).

Both gates are green at this checkpoint: `verify.py` 1411/1411, `audit.py` PASS
on every file, every file compiles clean at `/W3`, tree committed and pushed to
`origin/main` (`adamXbot/legoland`).

---

## 2. The single highest-value thing you could do

**62 of the 169 partials are already exact** — zero mismatches under `audit.py`
— and are held only because the shared `tools/match.py` cannot bound them.
Fixing that one file promotes 62 functions immediately.

    python3 tools/audit.py LEGOLAND/*.c | grep '\[WIP' | grep -c 'mismatch=0$'

Three shapes defeat `match.py`, which stops at the first `ret`:

1. **void tail-`jmp` wrappers** — the last statement is a call, so there is no
   `ret` at all.
2. **bodies ending in a `noreturn` call** — same, no `ret`.
3. **recursive functions** — a self-call inside its own COMDAT is not
   relocated, so it disassembles as a bare numeric target. `audit.py`'s
   `norm2()` rewrites those; `match.py`'s `norm()` only rewrites `0x`-prefixed
   ones, so it reports one mismatch (measured: 96.2% and 97.3% on the two Log
   Flume recursive functions).

**Done 2026-09-03 (user's go-ahead given in session).** `match.py` now carries
`true_extent` / `compiled_body` / `end_of_body` and the widened `norm`;
`audit.py` imports them; `verify.py` requires match.py's `extent ok` token.
Validated first on synthetic byte sequences (16 checks), then — once the
binary and toolchain were restored the same afternoon — against the real
thing: `audit.py` PASS on all 119 files, `match.py` prints `extent ok` on all
62 audit-exact WIPs, they were promoted, and `verify.py` (run alone) reports
**1473/1473**. This item is closed; the count is 1473 exact / 107 WIP.

---

## 3. Rules that are not in the code

- **Never run `verify.py` or `match.py` concurrently with anything that
  compiles.** They used to share one fixed object path (`/tmp/_match.obj`):
  two runs in one session disagreed by 49 functions and flagged four phantom
  regressions because a background disassembly was still going. Since
  2026-09-03 `match.py` writes a per-pid object, which removes that cause, but
  keep running it alone until a clean run confirms it; distrust any run that
  was not.
- **Count committed markers, not the working tree.** `verify.py` and
  `progress.py` read on-disk files, which include in-flight lane work.
- **Extern prototype TYPES are caller-side codegen levers.** `unsigned short`
  vs `int` parameters decide whether the caller emits a 16-bit load. The
  original's headers and translation units genuinely disagreed in places
  (`RemovePathTile`, `LoadSpriteIcon`). Never "align" an extern to its
  definition without re-auditing every file that declares it — doing so once
  committed a FAIL.
- **Always give an `extern` a trailing `/* 0x0044xxxx */` comment.** That
  comment is what `tools/callees.py` reads to track what is still missing.
- **Never commit a prologue-only or fabricated-tail body as `// FUNCTION:`.**
  Use `// WIP-FUNCTION:` while iterating and promote only after `audit.py` says
  `[OK]`. Rounds killed by API limits have repeatedly left false claims that had
  to be demoted by hand.
- **Markers bind 1–3 lines ahead.** A note between the marker and the signature
  makes the function silently uncounted — one finished function hid that way for
  a whole round. Explanations go *above* the marker.
- `original/` and `gamedata/` are gitignored; no game binary is ever committed.
  Confirmed clean on the remote.

---

## 4. Per-round integration checklist

```bash
cd /Users/systemadmin/Documents/Development/Github/legoland
# duplicate addresses across all files (must print nothing)
grep -rhoE '//\s*(WIP-)?FUNCTION: LEGOLAND 0x[0-9a-fA-F]+' LEGOLAND/*.c \
  | grep -oE '0x[0-9a-fA-F]+' | tr 'A-F' 'a-f' | sort | uniq -d
# honest markers (must print nothing)
python3 tools/audit.py LEGOLAND/*.c | grep -E 'REJECT|FAIL|COMPILE FAILED'
# /W3 clean
ALPHATEAM_VC6_ROOT="$PWD/toolchain" \
  "${LEGOLAND_CL:-../alphateam/tools/wibo-msvc/cl}" \
  /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/x.obj LEGOLAND/<file>.c
# relocation identity: zero MISMATCH lines (UNRESOLVED is fine)
python3 tools/relocs.py LEGOLAND/<file>.c
python3 tools/verify.py     # ALONE. nothing else compiling.
```

Then `git add` only the lane files (never `scratchpad/`), commit with the
recovered mechanics in the message, and update the status block in
`docs/DECOMP.md`.

---

## 5. What was in flight when this checkpoint was taken

Workflow `ll-batch27` (7 lanes) was **stopped mid-run** so the tree could be
committed cleanly. Its finished work is committed; its unfinished work is on
disk under `// WIP-FUNCTION:` markers, which is safe to build on.

One casualty to know about: a lane was interrupted with a `// FUNCTION:` marker
on an unfinished body, `Coaster_TickLoadingBay` (0x00424c70, `schoolcar.c`, 83
of 89 instructions). It is demoted to WIP and its note says it is an
**interrupted draft, not a diagnosed near-miss** — re-derive it from the
disassembly rather than trusting the shape that is there.

Lanes that had been running, all resumable from `docs/LANE_BRIEF.md`:
`lfentrance.c`, `waterworks.c`, `catapult.c`, `schoolcar.c`, `ridecb8.c`,
`ridecb9.c`, and a WIP-backlog lane over `mechrides.c`, `westtown.c`,
`westtown2.c`, `joust.c`, `person3d.c`, `ridecb1.c`, `simcore.c`.

---

## 6. Where to go next

**A. Done (2026-09-03 afternoon):** the 62 audit-exact WIPs are promoted and
`verify.py` is green at 1473/1473. Start at B.

**B. The closest genuine partials — refreshed 2026-09-05 from a full sweep.**
Each carries a note above its marker recording its measured residual, its first
diverging instruction index, and what previous agents ruled out. *Read that note
before touching one.* All 37 partials, by mismatch:

| mismatch | insns | address | function | file |
| --- | --- | --- | --- | --- |
| 3 | 43 | 0x004718c0 | ClampPopUpToScreen | misc3.c — **EXHAUSTED** |
| 3 | 330 | 0x00433840 | JcBoat_Animate | roads.c — **EXHAUSTED** |
| 3 | 482 | 0x00477bd0 | RequestRoute | simcore.c — **EXHAUSTED** |
| 5 | 205 | 0x0045f810 | ValidateCursor | objmap2.c — **EXHAUSTED, leave** |
| 5 | 637 | 0x0042aa90 | Balloonz_Tick | ridecb3.c |
| 8 | 218 | 0x0041a040 | BoatingSchool_Add | ridecb5.c — **EXHAUSTED, leave** |
| 10 | 222 | 0x0040bf70 | LFEntrance_Activate | lfentrance.c |
| 11 | 102 | 0x0040abf0 | LFEntrance_Remove | logflume.c |
| 12 | 31 | 0x00417e70 | WW_AnyBlokeInRect | waterworks.c — **EXHAUSTED** |
| 13 | 109 | 0x00473b00 | UpdateControllerFromMouseData | input.c — **EXHAUSTED** |
| 13 | 962 | 0x004724a0 | DrawPopUpInfo | popup.c |
| 15 | 141 | 0x00434f90 | JungleCruise_Add | ridecb9.c |
| 15 | 376 | 0x00416330 | SpiderRide_Activate | mechrides.c |
| 19 | 362 | 0x0043c950 | SpinningBarrels_Activate | mechrides.c |
| 19 | 387 | 0x0043e410 | PlaneRide_Activate | mechrides.c |
| 20 | 191 | 0x0048a3e0 | GetObjectUID | objmap2.c |
| 22 | 68 | 0x00475630 | InsertChildIntoList | fpui.c — **EXHAUSTED** |
| 22 | 347 | 0x00417430 | TempleSlide_Update | joust.c |
| 27 | 64 | 0x00413450 | Road_FindDiagonals | ridecb5.c |
| 27 | 116 | 0x00436dc0 | JungleCruise_UpdateRiverTile | junglecruise.c |
| 29 | 378 | 0x0042c820 | Carousel_Tick | ridecb3.c |
| 30 | 212 | 0x00418fe0 | BoatingSchool_DrawBoats | anim2.c |
| 44 | 111 | 0x00410180 | LFDrop_Place | logflume2.c |
| 47 | 358 | 0x0041a720 | BoatingSchool_Tick | ridecb5.c |
| 82 | 184 | 0x00470620 | CheckWorkerOnMouseStatus | workers2.c |
| 112 | 422 | 0x00432d00 | JungleCruise_UpdateRiverAnim | junglecruise.c |
| 118 | 119 | 0x0048f0f0 | InitExitCheckBox | screens2.c |
| 132 | 402 | 0x00415220 | SafariRide_Activate | mechrides.c |
| 138 | 222 | 0x0043bac0 | SpaceTower_Activate | mechrides.c |
| 182 | 331 | 0x00442040 | AnimApplyPart | anim2.c |
| 206 | 256 | 0x0040a600 | LFEntrance_Add | lfentrance.c |
| 208 | 354 | 0x00435750 | JungleCruise_Tick | ridecb2.c |
| 273 | 454 | 0x0045ff00 | RenderCursor | bigrender.c |
| 319 | 351 | 0x00402780 | StepSchoolCar | goldrush.c |
| 377 | 1023 | 0x00440a30 | Draw3DPersonModel | person3d.c — **EXHAUSTED, leave** |
| 381 | 903 | 0x0045b180 | RenderView | renderview.c |
| 844 | 1161 | 0x004567a0 | RenderFullMap | renderview.c |

**Target from the TOP of this table, and re-sweep before each wave.** Waves five
to ten repeatedly sent lanes at `RenderFullMap`, `RenderView`,
`Draw3DPersonModel` and `StepSchoolCar` — the four worst rows — and closed
nothing in six waves, while `ClampPopUpToScreen` (43 instructions, mismatch 3)
and `WW_AnyBlokeInRect` (31 instructions) sat untouched. Two heuristics that
the sweep makes obvious:

- **Small bodies close far more often.** The whole function fits in one reading,
  the residual cannot hide behind a frame permutation, and one construct usually
  explains all of it. A high mismatch FRACTION on a tiny body means one wrong
  construct, not many problems.
- **Exact byte length narrows the search sharply.** Every encoding is the right
  size, so the residual is ordering, allocation or operand order — not a wrong
  type, immediate or addressing form. Do not spend variants on arithmetic
  spellings there.

**IMPORTANT QUALIFIER, learned the same day this table was written: a low
mismatch does NOT mean a function is close to closing.** Wave eleven sent a lane
at the three functions tied at mismatch 3 — `RequestRoute`, `JcBoat_Animate` and
`ClampPopUpToScreen` — and all three turned out to be AT their floors, not near
them. Each is byte-exact and differs only in register allocation, and
`RequestRoute`'s residual was then closed by argument: a store to any field of
an address-taken struct kills CSE availability of an unrelated load, so such a
value is either the same register web as the earlier test or a fresh load, with
no third regime — and the register-to-register copy the original has is
therefore unreachable from C. **So sort candidates by REGISTER-BLIND mismatch,
not strict.** Strict 3 with register-blind 0 is a floor; strict 30 with
register-blind 25 still has structure left to find.

**The full triage, established across wave eleven's seven partials — two cheap
experiments tell you whether a residual is reachable at all.** First measure
strict, register-blind (rb) and offset-blind (ob), with frame homes resolved by
**esp/push depth**, never by raw `[esp+N]` (which drifts across branch joins and
has silently misread whole regions before):

| signature | kind | outlook |
| --- | --- | --- |
| `strict >> rb` | allocation | source ORDER is nearly powerless; ask which register the original frees, and when — but **try one free `volatile` read before calling it a floor**: `SubtractObjRect` was strict 14 / rb 0 and one such read closed all 14 |
| `strict >> ob` | frame layout | usually unreachable — weights count surviving IR, declaration order is inert |
| `strict == rb == ob` | pure scheduling permutation | same instructions, registers and homes, different order |
| rb still HIGH | **structural** | the only reliably reachable kind — **spend waves here** |

Then apply the free-`volatile` test: insert `*(volatile T*)&x` at a site where
the original loads anyway, so it costs no instruction. It advances VC6's
eax->ecx->edx scratch rotation, and is worth 219 and 115 at two sites in
`BoatingSchool_Tick`. **If it moves nothing, the residual is very likely a global
web rank** — but not certainly: in two `*_Destroy` callbacks (wave fourteen)
every free volatile read was inert and naming an array element in a local
still advanced the rotation and fixed every later register. So after the
volatile test, try the one-temporary spellings (name an array element, name a
call result, split a nested call) before calling it a floor.

Wave eleven ran seven partials chosen purely by lowest strict mismatch and
closed none of them, because none had the structural signature; five are now
formally exhausted. Run the triage BEFORE assigning a lane.

Regenerate this table with
`python3 tools/audit.py LEGOLAND/*.c | grep '\[WIP'` (needs a quiet tree —
never run it alongside anything that compiles).

Look for **twins**: the game is full of near-identical rides, and a fix on one
slot usually transfers straight to the same slot on another ride. That has
turned one fix into four repeatedly.

**C. The remaining frontier is now enumerated.** `python3 tools/inventory.py`
(scope N, 2026-09-05; method, limits and the full list in
`docs/lanes/scope-n.md`) finds every unmatched function in the game-code range
— 867, of which 703 are live (41,523 instructions) and 164 are dead code the
linker kept — with how each is reached, its nearest matched neighbour and 31
address-ordered candidate groups of ~1,200 instructions. `callees.py --by-file`
sees only the 46 that matched code declares. Cut new scopes from the groups:
P and Q took 16 and 17 (the startup spine); 24–27 are one neighbourhood
reached through the level-database keyword table at `0x004bb6f8`; cut 22
before 21 (61 calls); groups 3–7 hold most of the dead code, so cut only
their live members. `docs/RIDE_CALLBACKS.md` still names the ride slots, and
the inventory's pointer-table section names the other nine `.data` tables
(AI plan and state dispatch, script-event ticks, report setters, track
descriptors). One body, `0x004453a0` (8,085 instructions, the park-appraisal
report screen), cannot be bounded by `true_extent`'s 16 KB window; give the
walker a window parameter before assigning it.

**D. One deliberately abandoned function.** `UpdateControllerFromMouseData`
(0x00473b00, `input.c`) is 102/109. Two agents exhausted it, including checking
the executable's Rich header to confirm the game's translation units were built
by the same compiler back-end we use. The residual is an allocator state no C
construct or `/O2`-compatible option reaches. Its note lists every eliminated
hypothesis. **Leave it.**

---

## 7. How the lanes are run

One lane = one new file + a disjoint function list, prompted with
`docs/LANE_BRIEF.md` verbatim plus the lane's specifics. Never two lanes on one
file. **Cap concurrency at 7** — the account's session limit was hit twice at
10–16 concurrent agents, and each hit killed every in-flight lane.

Each lane returns a structured report per function (address, name, instruction
count, percentage, `audit_ok`, whether promotable, and the first diverging index
if not exact) plus what it learned about the data structures. Keep that schema;
it makes integration cheap.

When a lane reports a new codegen lever, add it to the "VC6 SP3 codegen levers"
section of `docs/DECOMP.md` — that playbook is why later rounds land functions
first try, and it has grown well past the `LANE_BRIEF.md` snapshot.
