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
- CI: the only two recorded runs (2026-09-01) pass the report check and fail at
  `actions/configure-pages` because **GitHub Pages is not enabled on the repo**
  (the Pages API returns 404), and the workflow itself is now
  `disabled_manually` (`gh workflow list --all`), which is why no run fired for
  any push after that date. Enabling Pages with "GitHub Actions" as the source
  and re-enabling the workflow fixes the deploy; the progress badge in the
  README is dead until then.
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
| **bytes of game code matched** | `python3 tools/coverage.py` | **38.8% exact, 51.3% with partials** |
| functions matched exactly | `git ls-files 'LEGOLAND/*.c' \| xargs grep -h '^// FUNCTION: LEGOLAND' \| wc -l` | 1473 |
| exported functions | `python3 tools/remaining.py` | 659 of 675 (97.6%) |
| unmatched callees | `python3 tools/callees.py` | 589, ~25,500 instructions |
| partials (WIP markers) | `python3 tools/audit.py LEGOLAND/*.c` | 107 |

(The row values above were refreshed at ~14:30 AEST after the 62 promotions
described in §0 and §2; the prose that follows in §1 predates them.)

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
  "${LEGOLAND_CL:-/Users/systemadmin/Downloads/alpha team/alphateam/tools/wibo-msvc/cl}" \
  /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/x.obj LEGOLAND/<file>.c
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

**B. The closest genuine partials.** 107 of the 169 WIPs are real partials, and
each carries a note above its marker recording its measured residual, its first
diverging instruction index, and what previous agents ruled out. *Read that note
before touching one.* The closest right now:

| mismatches | address | function |
| --- | --- | --- |
| 1 | 0x0040c4a0 | LFTrack_Update |
| 1 | 0x00413b50 | Roads_CalcCursor |
| 1 | 0x0041e4a0 | Route_IsClosed |
| 1 | 0x004828f0 | sub_4828f0 |
| 2 | 0x0042d610 | EarthSlide_Tick |
| 3 | 0x0040aac0 | LFEntrance_Update2 |
| 3 | 0x00433840 | JcBoat_Animate |
| 3 | 0x00477bd0 | RequestRoute |

Four more report `ESCAPES` (a branch in our body targets past the original's end
— usually a duplicated tail or a different block layout): `Copters_Activate`,
`Balloonz_Tick`, `Explorers_TickCustomers`, `SaveEmptySlotInput`.

Look for **twins**: the game is full of near-identical rides, and a fix on one
slot usually transfers straight to the same slot on another ride. That has
turned one fix into four repeatedly.

**C. The remaining frontier**, `python3 tools/callees.py --by-file` — 589
functions, ~25,500 instructions, now a long tail of 140–250 instruction
callbacks rather than a few giants. `docs/RIDE_CALLBACKS.md` names most of them
and says which object-definition slot each fills, so disassemble the relevant
`*_GetInterfaces` provider first and the cluster arrives pre-named.

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
