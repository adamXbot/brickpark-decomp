# Scope N — the whole-binary function inventory

**Status: complete.** Branch `scope/N`, baseline `origin/main` **`3fa59569`**
(2026-09-05). Deliverables: `tools/inventory.py` (new) and this report. No
`LEGOLAND/*.c` body, no existing tool, no shared progress file and no scope
brief was edited. The tool is read-only and safe alongside other sessions'
compiles.

## Headline

The game-code range `0x00401000..0x0049e000` holds **3,444 functions**:
2,456 carry a marker, **867 unmatched functions** do not, and 121 are import
thunks. Every byte of `coverage.py`'s 189,897-byte unmatched figure is now
accounted for, and the "residue" the brief asked for is 35 KB of which all
but 3 KB is inter-function padding and `switch` jump tables placed in `.text`:

| bytes of game code (643,072) | bytes | share of code | share of the gap |
| --- | ---: | ---: | ---: |
| matched (2,383 exact + 73 wip; coverage.py's method) | 453,175 | 70.5% | |
| unmatched by that measure | 189,897 | 29.5% | 100% |
| of which: **live unmatched functions** (703, 41,523 insns) | 133,270 | 20.7% | 70.2% |
| dead unmatched functions (164, 7,089 insns; nothing live names them) | 20,531 | 3.2% | 10.8% |
| import thunks (121, only 3 called directly by game code) | 726 | 0.1% | 0.4% |
| residue: padding between functions (3,026 gaps) | 24,718 | 3.8% | 13.0% |
| residue: `switch` jump tables and index tables inside `.text` (121 gaps) | 7,362 | 1.1% | 3.9% |
| residue: CRT tables that begin below `CRT_BASE` (1 gap, `0x0049d326..0x0049e000`) | 3,290 | 0.5% | 1.7% |

So the real matching frontier is **703 live functions, 41,523 instructions**,
not the 46 functions `callees.py` can see (those 46 are all in the list, with
their declared names). The brief expected the four named sources to leave a
few KB unexplained; they left 109 KB, and three further sources were needed
to close it (below).

## Method

`tools/inventory.py` enumerates function starts from eight sources and
iterates to a fixpoint; every address records every source that reached it.

| source | what it is | first found | reached by | only source |
| --- | --- | ---: | ---: | ---: |
| `export` | `symbols/legoland.exports.txt`, code section only (`remaining.py`'s section test drops the 41 data exports) | 675 | 675 | 0 |
| `marker` | `// FUNCTION:` / `// WIP-FUNCTION:` in `LEGOLAND/*.c` via `audit.annotated` | 1,781 | 2,456 | 6 |
| `call` | a direct `call rel32` inside a known function's extent — accepted unconditionally | 457 | 2,138 | 456 |
| `tail` | a direct `jmp rel32` that leaves its function's extent | 6 | 43 | 6 |
| `imm` | a 32-bit immediate operand in known code that lands on a plausible start: the slot stores in the `*_GetInterfaces` providers, callbacks passed as arguments | 13 | 735 | 12 |
| `table` | a 4-byte-aligned dword in `.data`/`.rdata` (or a `.text` gap) that lands on a plausible start | 229 | 236 | 227 |
| `crt` | a `call rel32` in the statically linked CRT whose target is game code: exactly one, `0x004a0996 -> 0x00453d10` | 1 | 1 | 1 |
| `sweep` | 16-aligned code straight after a known extent's padding that no reference reaches; tried only once every reference-based source is exhausted | 282 | 282 | 253 |

The fixpoint converged in 4 rounds. "First found" is the source that
introduced the address in that order; "reached by" counts every source
that names it; "only source" is the number for which nothing else does — so
227 functions exist only because a `.data` pointer table names them, and 456
only because unmatched code calls them.

**Plausibility.** A `call` target is proof. The weak sources (`tail`, `imm`,
`table`, `crt`) must land on a 16-byte boundary (all 3,100+ starts the
strong sources establish are 16-aligned), outside every known extent, on a
first byte that is not padding, with a bounded extent. Jump tables are
excluded twice: their targets lie inside the extent of the `switch` that owns
them, and every table a `jmp dword ptr [reg*4+T]` names is masked out of the
pointer scan. A run of consecutive in-range dwords containing any unaligned
value is rejected whole — those are jump tables and SEH scope tables of
functions not yet known, and a UTF-16 alphabet at `0x004aff08`. A `.data`
dword whose bytes and neighbours are all printable is kept but flagged
`WEAK`; no function carries the flag in this run, and an earlier, stricter
version of the test had wrongly rejected three real pointers inside records
with inline names (`0x00427c30` is named three times at stride 0x38 in the
track-piece table at `0x004b5d60`).

**Extents** come from `match.true_extent`, unchanged. Two things it cannot
bound are handled in the tool and reported, never hidden:

- `true_extent` disassembles a 16 KB window and returns nothing for a longer
  function. `long_extent` in the tool is the same walk, rule for rule, over
  128 KB, used only on that failure. One game function needs it:
  **`0x004453a0` is 8,085 instructions / 34,662 bytes** (frame `0x23d4` via
  `chkstk`). `audit.py`, `matchfull.py` and `relocs.py` all fail on it today;
  the window needs to become a parameter of `true_extent` before anyone
  attempts it.
- A `__try/__except` body ends, for the walker, at the `jmp` over its filter
  and handler blocks, which only the scope table in `.rdata` reaches. Three
  functions have the SEH prologue (`push -1 / push scope / push
  0x0049ff34 / mov fs:[0], esp`): `0x00453d10`, `0x00453da0`, `0x00454380`.
  The tool extends such a body to the next padding-preceded 16-byte
  boundary (capped at the next known start) and disassembles linearly. Only
  WinMain needed it (31i/93B walked → 48i/143B; its handler block calls
  `0x00453da0`, which the walk alone reported as dead); the other two walk
  to their full extent (345i/1256B, 110i/380B). These are the EBP frames in
  an `/O2` binary that `PARALLEL_CONTRACT.md` mentions.

Import thunks (`jmp dword ptr [IAT]`) have no terminator for the walker and
are recognised by shape and bounded at one instruction; a marker on such a
body (`GetTicks`, `sysstubs.c`) keeps it a function.

**Liveness.** A root is anything a marker, an export, a `.data` pointer table
or the CRT names; everything a live function calls, tail-jumps to or takes
the address of is live. The 164 "dead" functions are reached by nothing live:
253 were introduced by the sweep, and the rest are called only from swept
code. They are inventoried and grouped but flagged `DEAD`; nothing in the
binary can execute them unless a computed call the analysis cannot see
exists, which is why they are reported rather than dropped.

**Residue** is every game-code byte range no extent covers, classified as
`padding` (only `int3`/`nop`), `jump-table` (contains a table a known `jmp
[reg*4+T]` names, or a byte index table known code reads), `crt-data`, or
`code-like`/`data-like`. Nothing is left in the last two classes.

**Grouping** packs the unmatched functions in address order into groups of
about 1,200 instructions (`--group N` changes the target), closing a group at
a gap of more than 16 KB once it holds half the target, and giving any
function over 1,500 instructions a group of its own. Address order is the
right neighbourhood key here: the binary is laid out by translation unit, so
consecutive unmatched functions share a source file, and every group's
"nearest matched" column names that file. Each group also lists which other
groups call into it, so the integrator can see dependencies before cutting
scopes.

## Limits

- Everything rests on `match.true_extent`'s rules; an extent it gets wrong is
  wrong here too. Overlapping extents are reported as anomalies (there are
  none in this run) and weak candidates that fall inside a later-discovered
  extent are pruned and listed (none).
- The tool sees direct calls, direct jumps, immediates and initialised
  pointers. A function reached **only** through a computed address it cannot
  see (a pointer built by arithmetic, or stored in `.data` past the
  initialised 0xE000 bytes) would appear as `DEAD`; the sweep guarantees it is
  at least inventoried if it follows another function's padding.
- The sweep skips to the next 16-aligned address that a padding byte
  precedes. A dead function that starts exactly on a 16-byte boundary with
  no padding before it, after a data block, is not swept; the residue shows
  no `code-like` gap, so none was lost in this binary.
- `imm` candidates can be constants: `0x0040e040` in `0x0049ac50` is the
  COLORREF green `0x40e040`, and was rejected only because it falls inside
  `0x0040dc00`'s extent. Every `inside`-rejection was checked and none is a
  padding-preceded aligned start. A `.data` dword can spell an address too:
  `0x004c0178` in the CRT `_ctype` table reads as `0x00480020`, which lies
  in `LegoLandWindowProc`'s switch index table; a candidate that follows a
  byte table known code reads, with no padding between, is rejected, while
  one that follows a jump table's known span is not (`0x00484790`, the
  low-level AI state 9 handler, starts exactly where `GetTileInDir`'s
  table ends).
- The pointer scan treats a run with any unaligned entry as a jump table. A
  callback table directly adjacent to a jump table with no separating
  non-pointer dword would be lost with it; the self-check against
  `RIDE_CALLBACKS.md` (below) shows no such loss.
- `CRT_BASE = 0x0049e000` is taken from `coverage.py` as the brief requires.
  The CRT's own tables actually begin at `0x0049d330` and 121 import thunks
  sit at `0x0049d050..0x0049d320`; `coverage.py` therefore counts 4,016 bytes
  as "unmatched game code" that can never be matched. Game code calls 28
  further thunks at `0x0049e3a0..0x0049e418`, above the boundary, which is
  where `callees.py`'s 45 non-game entries come from.
- Nothing is named. Where an `extern` declaration in the tree already names
  an address, the `declared` column repeats that name.

## Reproduce

```sh
PY=/Users/systemadmin/.venvs/legoland/bin/python
$PY tools/inventory.py                                 # full report to stdout, ~30 s
$PY tools/inventory.py --self-check --json /tmp/sn_inventory.json
$PY tools/inventory.py --group 1000 --min-gap 64
```

Exit status is 0, or 1 when `--self-check` finds a `RIDE_CALLBACKS.md`
address missing. The self-check passes: all 286 callback addresses in
`docs/RIDE_CALLBACKS.md` are in the inventory, every one reached through the
`imm` source (the provider's slot store) as well as its marker.

## Cross-checks

- **`callees.py`** lists 91 unmatched callees; 45 are CRT routines, IAT
  thunks above the boundary, or a `.data` pointer global declared as a
  function (`0x004b8368`). The other 46 are exactly the 45 unmatched
  functions here that carry a `declared` name plus `UpdateMovieAudio`,
  which its parser placed first. 2,562 instructions.
- **`coverage.py`'s matched bytes** are recomputed by the same method
  (453,175) and the accounting closes to the byte: live + dead + thunks +
  residue = 189,897.
- **`RIDE_CALLBACKS.md`**: 286 of 286 addresses present, all via `imm`.
- **The one CRT call site** is the entry code calling the game's WinMain
  wrapper; the chain from it reaches the largest unmatched functions
  (below).

## Anomalies and facts worth recording in DECOMP

- **One 8,085-instruction function** at `0x004453a0` (34,662 bytes, 9 KB
  frame). It is on the startup path: CRT `0x004a0996` → `0x00453d10` (SEH
  WinMain wrapper) → `0x0047fd10` (main, 84i) → `0x0047f880` (262i) →
  `0x00459520` (121i) → `0x00458c00` (153i) → `0x00458ee0` (289i) →
  `0x0044db90` (60i) → `0x004453a0`. Its body calls the 17 small functions
  immediately before it (`0x004449b0..0x00445310`, group 10) and about 150
  distinct callees. No existing tool can bound it (16 KB window).
- **Jump tables live in `.text`** after their `switch` in this binary: 121
  gaps, 7,362 bytes, each anchored by a `jmp dword ptr [reg*4+T]` in the
  preceding function (`_table_targets` already reads them from any section).
  DECOMP's "/Gy puts the table in .rdata" is not what this binary shows.
- **SEH scope tables** at `0x004ab4e4..0x004ab510` (`.rdata`) point at the
  three `__try` bodies' filter/handler blocks; they are what the pointer scan
  rejects as unaligned runs there. The PE export address table in `.rdata`
  holds RVAs, two of which (`0x004119a0`, `0x00410160`) coincide with
  function VAs; the tool masks the export directory out of the scan.
- **The CRT boundary is soft**: `0x0049d050..0x0049d320` are 121 import
  thunks, of which game code calls only 3 directly (`0x0049d314`, `0x0049d31a`, `0x0049d320`), and `0x0049d330..0x0049e000`
  is CRT data (a 16-byte-stride table of `.rdata` pointers).
- **`0x004b8368`** is declared like a function somewhere in the tree but is a
  `.data` address; `callees.py` reports it as an unmatched callee.

## The pointer tables in `.data`

226 unmatched functions exist only because an initialised dword in `.data`
names them. Clustering those dwords by address and reading the code that
indexes each base (and the C declarations and `docs/runtime/*.md` that
already describe several) gives nine real tables and three false positives
the tool now excludes. Counts are distinct functions; strides in bytes.

| table (VA) | shape | unmatched fns | what it is, with the evidence |
| --- | --- | ---: | --- |
| `0x004b5648`, `0x004b5658`, `0x004b5f50` | 3 pairs of pointers | 5 | Coaster 3D **span-filler pairs**: schoolcar3.c declares `g_span_fillers[2]` at `0x004b5658` and stores it into `PolyJob.shader` for `Raster_SubmitPoly`; the three mesh passes `CoasterModel_DrawPass1/2/3` (`0x00420810`/`0x00420a20`/`0x00420c40`, declared in coaster9.c, unmatched) load the three tables into the same frame slot. Surrounded by the RK4 tableau floats (schoolcar6.c). |
| `0x004b5b64` (inside `g_castle_desc`, `0x004b5b48`) | 5 handler slots, 2 null | 3 | The draw/build/query/place/remove slots of the **CASTLE OBJ `TrackDesc`** record (castleobj.c); `0x00423940`/`0x00423970` read the record's own `+0x04`/`+0x08` heights via `fild`. An earlier version of the text test rejected these three because their pointer bytes are printable. |
| `0x004b5d20`, `0x004b5d58`, `0x004b5d90`, `0x004b5dc8` | 0x38-byte records, 5 handlers each at `+0x1c` | 8 | The four **`TrackDesc` records** SQUARE_TRACK / _HEIGHT / _HEIGHT_0 / _HEIGHT_PATH that `Track_Update`, `Track_Add` and `FindTrackDesc` (castleobj.c) read; the non-pointer dwords are the raised flag, node heights and carries-path flag exactly as castleobj.c documents. `0x004275c0` is a one-byte `ret`. |
| `0x004b63fc` | one pointer | 1 | A **solver hook** (default `0x00429e20`): `0x00429f30` and `0x0042a020` call `[0x004b63fc](0x00429cf0, t, a, tol)` twice each — an integrator/bisection taking a derivative callback; `0x00429f30` is the "steps 30 units along the track" stepper schoolcar4.c describes. |
| `0x004b7e38` | 25 pointers | 25 | **Report-line setters**: `0x0046a140` does `if (0 <= i < 0x19) table[i](a, b)`; each 48–55-byte stub sets or clears a bit of `g_report_state` (`0x00665ff8`, uimisc.c, saved by `SaveReport`/`LoadReport`) and stores its argument. Called by script-event kind `0x18` and by the `REPORT` keyword handler. Followed at `0x004b7e9c` by a 22-entry sorted object-class-name string table (`"Boating School Water"` … `"Zebra Crossing"`) that `0x00444c40` searches through `0x004781b0`. |
| `0x004b8368` | 26 pointers, 1 null | 11 (7 matched) | **High-level AI plan dispatch** `g_lt_action_handlers[]` (blokeai.c, `DoHighLevelAI`; also read by `0x0044ed00`). `docs/runtime/ai-data.md` gives the per-plan semantics: the three largest table-only functions `0x0044f610` (699i), `0x0044fe80` (337i) and `0x0044ed70` (328i) are plans 6 (select/route/admit), 0xd (CAFE BROLLY reservation) and 3 (leaving). |
| `0x004b9ca4` | one pointer | 1 | **`g_present`** (screen.c/render2.c): the presenter `RenderingComplete` calls through. Its initial value `0x00466080` is the windowed presenter; the switch parser at `0x0047fd10` swaps in `FlipPrimary` (`0x004661d0`) for full screen. |
| `0x004b9d44` | 70 pointers, 2 null | 68 | **Script-event per-kind tick handlers** `g_event_tick[]` (fpui3.c, `UpdateHelpTick`), parallel to `g_event_dirty_mask[]` at `0x004b9e5c` (uimisc.c, `ScriptEventDue`). Entry shape `int fn(ScriptEvent*)`, mostly `call helper; return 1`. Groups 20–22. |
| `0x004bb6f8` | 93 records of `{char* keyword, handler}` | 90 | **Level-database keyword table** `g_level_db_sections` — movie.c's `LoadLevelDatabase` passes it to `ParseKeywordFile(name, &g_level_db_sections, 0x5d, 0)`. Keywords `[INIT] [OBJECTIVE] [ONEOFF] … MAP LOAD ENABLE CURRENCY … SELECTTHEME SELECTTAB … ENDLEVEL`; BRIEFINGFILE/BREIFINGFILE, MAXCAPACITY/MAXVISITORS, MINCAPACITY/MINVISITORS share bodies. Preceded at `0x004bb6c0` by the category/theme/tab name arrays the SELECT* and FLASHBUTTON handlers use. Groups 24–26, about 11 KB with the two parser helpers `0x004781b0`/`0x004781f0`. |
| `0x004bd34c` | 16 pointers | 16 | **Low-level AI state dispatch** `g_lowlevel_ai[]` (blokemisc.c, `DoLowLevelAI`; state 9 at `0x00484790` re-dispatches through it). `docs/runtime/ai-low-data.md` documents all sixteen states. Preceded by the `DirectionDelta` s16 pairs at `0x004bd32c`. Group 28. |
| `0x004aee60`, `0x004aef80` (`.rdata`) | — | 0 | **False positive, now excluded**: the PE export address table holds RVAs, and RVA `0x004119a0` (`NEWFLC_CheckDuplicate`) and `0x00410160` (`QueryCursor`) coincide with function VAs. The tool masks the export directory out of the scan. |
| `0x004c0178` | — | 0 | **False positive, now rejected**: `0x004c013a` is the CRT `_ctype` u16 table, so the dword there spells `{_ctype[0x1f], _ctype[0x20]}` = `0x00480020`, which lies inside `LegoLandWindowProc`'s switch index table (a run of `0x04` bytes). The tool now rejects a candidate that follows a data anchor in its gap with no padding between. |

A side observation useful for attributing tables to translation units: 119
unreferenced `0x0000007f` dwords sit in `.data` (`0x004b5d14`, `0x004b7e34`,
`0x004b9ca0`, `0x004b9d40`, …) that no code reads; they are link-time fill
marking object-file contribution boundaries.

Where each table-only cluster belongs, by the file that already holds its
readers: span fillers → schoolcar3.c/schoolcar8.c; `TrackDesc` handlers →
castleobj.c/coaster.c (`Track_*`, `TrackH_*`, `TrackHP_*`); solver hook →
schoolcar6.c or coaster9.c; report setters → uimisc.c; AI plans → sweep1.c/
blokeai.c; `g_present` → sysmisc.c/render2.c; event ticks → fpui3.c/uimisc.c/
uimisc3.c; keyword handlers → a new file beside movie.c/data2.c; low-level AI
states → blokemisc.c/bnvmove.c/bnvpath.c/sweep3.c.

## The startup chain and what the biggest unmatched functions are

The single CRT call site is the PE entry point's call to WinMain, and the
chain from it reaches the three largest unmatched bodies. Established from
the disassembly and the strings and globals the tree already names (no
function is named here; descriptions only):

- `0x00453d10` (48i, SEH) is **WinMain**. It calls `0x00458830` (reads the
  exe's own version resource: `"Legoland.exe"`,
  `\StringFileInfo\080904B0\ProductVersion`) then forwards its four
  arguments to `0x0047fd10` (84i): `CreateMutexA("LegolandGameMutex")`,
  `"Program already running."`, the command-line switches parsed by
  `0x0047fc40` (`WINDEBUG`, `BLT`, `-nointro`, `-nomusic`), `g_hinstance`,
  `CheckHostSystemGPU`; then `0x0047f880` (262i): `legoland.log`,
  `RES_OpenVolume`, `InitHostSystemGPU`/`InitScreen`/`InitInputSystem`,
  the eight cursor sprites, `LLIDB_LoadICM`, the session, and
  `"Finished shutting stuff down"`.
- `0x00459520` (121i) is the **main loop** (`Intro.avi`, `Ir50_32.dll`,
  `InitGameMap`, `LoadMapTiles`, `CreateObjectClasses`, then
  `r = f(); while (r) r = f();` on `0x00458c00`).
- `0x00458c00` (153i) is the **per-frame game-mode dispatcher**: pending
  load (`profiles`, `%s\%dsave%d.sav`), `InitScreens`, `ReadGameButtons`,
  then a `switch` on `[0x008119b4]` through the jump table at `0x00458ec4`:
  0 quit, 1 `0x00459360` (map-screen frame), 2 the front-end screens, 3
  `0x00458ee0` (289i, the **in-game frame**: `"AI"`, `"ProcessStuff"`,
  `"Zoning"`, `"Rendering"`, `"In Game Help"`, `"Appraisals"`, `"Exiting
  GameProc"` in `g_dbg_where`).
- `0x00457a70` (751i, second largest) is the **click-on-map action
  handler** the in-game frame calls once: `g_hit_type`/`g_icon_value`/
  `g_hit_cell`, `"DRIVING SCHOOL ROADS"`, `"ZEBRA CROSSING"`, work orders,
  `BuildCursorPtr`/`RenderCursor`, `RemObjFromMap`, `WorkOrderBuildObject`,
  `PlayAppropriateBuildEffect`, `PopUpInfoSetUp`. `0x00457970` (85i) is its
  build-footprint clearance test over `[0x00801400]` map records.
- `0x004453a0` (8,085i, largest) is the **park-appraisal (advisor report)
  screen**. It bails on `ScriptRunning()`, zeroes the report-line counter
  `0x006660a0`, tests bit groups of `g_report_state` (`0x00665ff8`,
  uimisc.c) and calls the seventeen statistic gatherers just before it
  (`0x004442c0..0x00445190`) against thresholds at `0x00666010..0x00666094`,
  appending a 76-byte record per passing criterion. The ~15 near-identical
  27-instruction blocks (`0x00445a95..0x00445b02` is one) each do
  `rand() % 5` (`0x0049e4b2` **is CRT `rand`** — `imul 0x343fd; add
  0x269ec3` on seed `0x004c00f0` — not `_ftol`) and `GetString(0x137 + n)`:
  five phrasings per report line, about 135 `rand` and 112 `GetString`
  calls. The tail loads `App_tick%d.lls`/`App_cross%d.lls`/
  `App_bullet%d.lls`/`App_bar.lls`, renders with `PrintSprite`,
  `RenderIcons2`, `NewPrintCent`, speaks `TEXT%04d.WAV`, and loops until
  `[0x0081c038]` clears. Its only caller, `0x0044db90` (60i), is the
  appraisal-due tick in the in-game frame (`g_instant_appraisal <=
  GetGameTimer()`; on pass `StopScript(1)`, on fail `EndLevel(2)`), and the
  `:PRAISEME` cheat in input.c sets `g_instant_appraisal = 1`. `0x0049e573`
  is `sprintf`, `0x0049e600` is `_chkstk`.

## Dead code

164 functions, 7,089 instructions, 20.0 KB are reached by nothing live. The
linker evidently ran without `/OPT:REF` (the tree already holds retained
empty hooks such as `MemScratch_Noop`). The largest were checked by hand:

- `0x004227a0` (8i) frees the `g_cc_txt` and `g_cc_obj` model images via
  `Free_w`, and `0x004227c0` (521i) rebuilds a mesh-adjacency structure from
  a model (`+0xc` face count, `+0x14` vertices, `+0x18` index triples masked
  `0x7fffffff`), `malloc`s three buffers, fills them and frees them again —
  a result nobody consumes. Zero `E8`/`E9` sites and zero pointers anywhere
  in the file name either; neighbours are matched schoolcar4.c and
  coastertiny.c.
- The other dead clusters sit in the coaster editor neighbourhood
  (coastertiny.c/schoolcar8.c/coaster.c: 32 functions), the loaders
  (loaders.c: 5 functions, 681 instructions, including `0x0043f0b0` 313i
  and `0x0043ea30` 395i, called only from each other) and the screens
  (screens3.c/tinystubs.c/text.c).

Dead functions stay in the inventory and in their address groups, flagged
`DEAD`, so a scope can include or skip them knowingly. For the browser
runtime they are irrelevant; for byte coverage they are 3.2% of the game.
The integrator may prefer to cut them from scopes and to have `coverage.py`
report them separately, since no amount of matching work on live code will
ever move that 3.2%.

## Candidate scopes

31 address-ordered groups; the `--group 1200` default gives 1,000–1,500
instructions each, and the appraisal function stands alone as group 11.
"Reached from" counts sources per function: a matched file name means that
file's bodies call into the group; `gN` means unmatched functions in group N
do; `within` is internal. Groups that only pointer tables reach (25, 26) are
handler tables; see the pointer-table section. Full membership is in
Appendix A.

| group | range | fns | insns (live / dead) | bytes | largest | neighbours (nearest matched file) | reached from |
| ---: | --- | ---: | ---: | ---: | ---: | --- | --- |
| 1 | `00401e00`..`0040cfc9` | 32 | 1470 (1123 / 347) | 4200 | 170 | logflume2.c x15, logflume.c x7 | within x17, g2 x13, logflume.c x5, ridemachine.c x4 |
| 2 | `0040d420`..`0040dba7` | 6 | 602 (602 / 0) | 1889 | 151 | logflume2.c x3, logflume.c x3 | logflume.c x26, within x3 |
| 3 | `00411e20`..`0041f4e0` | 38 | 1432 (1230 / 202) | 3921 | 179 | schoolcar8.c x5, coaster.c x5 | within x16, coaster9.c x5, coaster5.c x2, g4 x2 |
| 4 | `0041f4e0`..`00420a13` | 19 | 1339 (958 / 381) | 3937 | 202 | schoolcar8.c x12, coastertiny.c x3 | within x6, g7 x4, g3 x1, g5 x1; tables 4b56 |
| 5 | `00420a20`..`0042313c` | 17 | 1443 (532 / 911) | 4421 | 521 | coaster9.c x4, coastertiny.c x4 | coaster9.c x5, coaster7.c x2, within x2, g6 x1 |
| 6 | `00423200`..`00428857` | 44 | 1420 (850 / 570) | 3881 | 103 | coastertiny.c x11, coaster.c x9 | within x11, g7 x5, coaster9.c x3, coaster3d.c x2; tables 4b5b, 4b5d |
| 7 | `00428860`..`0042a77f` | 22 | 1245 (1077 / 168) | 3807 | 254 | schoolcar.c x8, coaster9.c x7 | within x13, g3 x4, coaster5.c x3, coaster9.c x3; tables 4b5f, 4b63 |
| 8 | `00434810`..`0043f4e2` | 12 | 1430 (153 / 1277) | 4252 | 395 | screencb2.c x3, loaders.c x3 | within x3, ridemachine.c x2, g9 x1 |
| 9 | `0043f4f0`..`00444a67` | 48 | 1408 (1277 / 131) | 4525 | 229 | uimisc.c x33, savemisc2.c x4 | within x10, g11 x6, texture.c x2, g17 x2; tables 4b7e |
| 10 | `00444a70`..`00445395` | 16 | 734 (734 / 0) | 2221 | 98 | uimisc.c x16 | g11 x11, within x10 |
| 11 | `004453a0`..`0044db06` | 1 | 8085 (8085 / 0) | 34662 | 8085 | uimisc.c x1 | g12 x1 |
| 12 | `0044db20`..`0044fdc9` | 15 | 1599 (1597 / 2) | 4523 | 699 | rin.c x6, sweep1.c x5 | within x10, g24 x3, g20 x1, g16 x1; tables 4b83 |
| 13 | `0044fe10`..`004515d4` | 18 | 972 (655 / 317) | 2614 | 337 | blokeai.c x6, sysmisc.c x6 | within x6, g16 x1; tables 4b83 |
| 14 | `00451740`..`0045490b` | 12 | 1463 (1443 / 20) | 4885 | 620 | text.c x5, sysstubs.c x4 | within x12, render5.c x1, g17 x1, CRT x1 |
| 15 | `00454a10`..`00457a62` | 13 | 1023 (806 / 217) | 2806 | 275 | fpui3.c x3, text.c x2 | within x5, g16 x4, g20 x4, g8 x2 |
| 16 | `00457a70`..`0045933f` | 10 | 1403 (1403 / 0) | 5526 | 751 | sysstubs.c x6, movie.c x2 | within x4, g17 x4, screens3.c x4, g14 x1 |
| 17 | `00459360`..`00462ee6` | 18 | 1404 (1003 / 401) | 4375 | 291 | uimisc2.c x4, objmap.c x3 | within x4, g26 x3, bigrender.c x2, g20 x2 |
| 18 | `004632b0`..`004677a6` | 12 | 1308 (1307 / 1) | 4007 | 325 | softblit2.c x4, bigrender.c x2 | softblit2.c x4, g16 x1, g24 x1, g11 x1; tables 4b9c |
| 19 | `004677b0`..`004689ed` | 14 | 1431 (1431 / 0) | 4375 | 312 | sysstubs.c x12, softblit2.c x1 | g21 x5, softblit2.c x4, g26 x3, g25 x2 |
| 20 | `004689f0`..`0046a43a` | 63 | 1493 (1471 / 22) | 4438 | 177 | softblit.c x29, tinystubs.c x15 | within x102, g21 x43, g24 x3, g26 x3; tables 4b9d |
| 21 | `0046a440`..`0046b8bb` | 55 | 1495 (1481 / 14) | 3754 | 165 | uimisc.c x46, sysstubs.c x6 | g22 x61, within x7, g26 x4, g24 x2; tables 4b9d, 4b9e |
| 22 | `0046b8c0`..`0046eec1` | 76 | 1444 (1303 / 141) | 4323 | 92 | uimisc3.c x33, sysstubs.c x29 | g25 x28, g26 x27, g27 x8, g16 x3 |
| 23 | `0046f100`..`00476d13` | 35 | 1499 (954 / 545) | 4848 | 212 | tinystubs.c x9, fpui.c x5 | g16 x4, within x4, screens3.c x4, movie.c x4 |
| 24 | `00476d20`..`00478cc1` | 37 | 1484 (1330 / 154) | 4421 | 344 | simcore.c x33, uimisc2.c x2 | g26 x47, g25 x38, within x35, g27 x11; tables 4bb6, 4bb7 |
| 25 | `00478cd0`..`00479ca1` | 33 | 1484 (1484 / 0) | 3744 | 115 | simcore.c x18, movie.c x15 | tables only; tables 4bb7, 4bb8 |
| 26 | `00479cb0`..`0047abfd` | 33 | 1491 (1491 / 0) | 3684 | 129 | movie.c x33 | g24 x1; tables 4bb7, 4bb8, 4bb9 |
| 27 | `0047ac00`..`00480a91` | 23 | 1473 (1421 / 52) | 4251 | 316 | movie.c x9, music.c x4 | within x7, fpui2.c x1, screens2.c x1, g14 x1; tables 4bb7, 4bb9 |
| 28 | `00480aa0`..`00484087` | 35 | 1462 (1371 / 91) | 4038 | 159 | tilehelp.c x7, workers.c x4 | within x25, g29 x9, g17 x2, g24 x2; tables 4bd3 |
| 29 | `00484090`..`0048d4a7` | 37 | 1479 (1236 / 243) | 4004 | 147 | pathtile2.c x9, tri3d.c x4 | within x8, g16 x6, screens3.c x4, data2.c x2; tables 4bd3 |
| 30 | `0048e0c0`..`00498112` | 49 | 1476 (1029 / 447) | 4448 | 155 | screens3.c x10, screens2.c x5 | screens3.c x9, within x9, screens2.c x7, g29 x7 |
| 31 | `00498150`..`0049d048` | 24 | 1121 (686 / 435) | 3021 | 192 | tinystubs.c x6, text.c x5 | within x8, g16 x4, screens3.c x3, movie.c x1; tables 4b83 |

Recommendations for cutting `SCOPE_*.md` files from this:

- **Start with groups 16 and 17** (`0x00457a70..0x00462ee6`): the in-game
  frame, the dispatcher, the main loop, the map-screen frame and the click
  handler — the spine that every other unmatched cluster hangs from, all
  live, all called from each other, and the browser runtime's control flow.
- **Groups 24–27** (`0x00476d20..0x00480a91`, ~5,900 instructions) are one
  neighbourhood — simcore.c/movie.c neighbours, reached almost entirely
  through the `.data` tables at `0x004bb600..0x004bb900` and from each
  other — and should be cut together in address order.
- **Groups 20–22** (`0x004689f0..0x0046eec1`, 194 small functions) are the
  uimisc/uimisc3/softblit neighbourhood; group 22 is called 61 times from
  group 21 and 55 times from groups 25/26, so cut 22 before 21.
- **Group 11** cannot be a scope until `true_extent` takes a window
  parameter; even then 8,085 instructions is six scopes' worth of one body,
  and its ~135 `rand()`/`GetString` blocks are the same statement repeated
  — a candidate for one agent with the repetition as its lever.
- **Groups 3–7** (coaster/schoolcar neighbourhood) carry most of the dead
  code (75 of the 164 dead functions); cut the live members only.

## Appendix A — candidate scope membership

Address order within each group. "reached by" is the strongest source; `(+N more)` counts further callers.

### Group 1 - `0x00401e00`..`0x0040cfc9`: 32 functions, 1470 instructions (1123 live in 25, 347 dead in 7), 4200 bytes

Neighbours: logflume2.c x15, logflume.c x7, posstep.c x3. Reached from: within group x17, group 2 x13, logflume.c x5, ridemachine.c x4.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00401e00` | 69 | 289 | unreferenced (swept) | SchoolCarIdleStep (schoolcar4.c) +304 | DEAD |
| `0x00403d60` | 12 | 38 | unreferenced (swept) | Copters_StepRider (ridetiny.c) +48 | DEAD |
| `0x00404630` | 168 | 528 | called by Copters_StepMachine (ridemachine.c) | Copters_Place (mechrides.c) +48 | declared Copters_UpdateCarRider in ridemachine.c |
| `0x00404860` | 23 | 59 | called by Copters_StepMachine (ridemachine.c) | Copters_ResumeSFX (ridetiny.c) -64 | declared Copters_StepCar in ridemachine.c |
| `0x004049a0` | 71 | 228 | called by Copters_InitRecord (ridemachine.c) (+1 more) | Copters_SetFull (ridemisc.c) +240 | declared Copters_StopRide in ridemachine.c |
| `0x00408f90` | 50 | 123 | unreferenced (swept) | LFTrack_FindPiece (posstep.c) +96 | DEAD |
| `0x00409620` | 25 | 95 | called by 0x004097a0 [unmatched] (+1 more) | LFTrack_MaskIsLegal (logflume2.c) +160 |  |
| `0x00409680` | 25 | 90 | called by 0x004097a0 [unmatched] (+1 more) | LFTrack_MaskIsLegal (logflume2.c) +256 |  |
| `0x004096e0` | 26 | 86 | called by 0x004097a0 [unmatched] (+1 more) | LFTrack_MaskIsLegal (logflume2.c) +352 |  |
| `0x00409740` | 25 | 82 | called by 0x004097a0 [unmatched] (+1 more) | LFTrack_MaskIsLegal (logflume2.c) +448 |  |
| `0x004097a0` | 170 | 453 | called by LFTrack_Add (logflume.c) | LFTrack_MaskIsLegal (logflume2.c) +544 |  |
| `0x00409a50` | 12 | 42 | called by 0x00409a90 [unmatched] | LFRoute_Join (logflume2.c) -288 |  |
| `0x00409a90` | 49 | 128 | called by 0x0040d900 [unmatched] | LFRoute_Join (logflume2.c) -224 |  |
| `0x00409b10` | 37 | 81 | called by 0x0040a010 [unmatched] | LFRoute_Join (logflume2.c) -96 |  |
| `0x0040a010` | 46 | 104 | called by 0x0040a080 [unmatched] | LFEntrance_Create (logflume.c) -720 |  |
| `0x0040a080` | 45 | 108 | called by 0x0040d900 [unmatched] | LFEntrance_Create (logflume.c) -608 |  |
| `0x0040a0f0` | 38 | 99 | called by 0x0040a2a0 [unmatched] | LFEntrance_Create (logflume.c) -496 |  |
| `0x0040a160` | 37 | 98 | called by 0x0040a2a0 [unmatched] | LFEntrance_Create (logflume.c) -384 |  |
| `0x0040a1d0` | 37 | 96 | called by 0x0040a2a0 [unmatched] | LFEntrance_Create (logflume.c) -272 |  |
| `0x0040a230` | 40 | 101 | called by 0x0040a2a0 [unmatched] | LFEntrance_Create (logflume.c) -176 |  |
| `0x0040a2a0` | 23 | 62 | called by LFTrack_Remove (logflume.c) (+1 more) | LFEntrance_Create (logflume.c) -64 |  |
| `0x0040adb0` | 69 | 209 | unreferenced (swept) | LFPiece_ShapeIndex (logflume2.c) +96 | DEAD |
| `0x0040b270` | 7 | 19 | called by 0x0040c250 [unmatched] | LFBoat_IsOnPiece (posstep.c) +96 | DEAD |
| `0x0040b290` | 93 | 250 | called by LFHoldUp_Interact (logflume.c) (+2 more) | LFBoat_IsOnPiece (posstep.c) +128 |  |
| `0x0040bd40` | 76 | 191 | unreferenced (swept) | LFRun_Tick (logflume4.c) -192 | DEAD |
| `0x0040c250` | 64 | 143 | unreferenced (swept) | LFPiece_HasCursor (logflume6.c) -144 | DEAD |
| `0x0040ce20` | 72 | 238 | called by 0x0040cf10 [unmatched] | LFPiece_IsVisible (logflume2.c) +48 |  |
| `0x0040cf10` | 7 | 24 | called by 0x0040db00 [unmatched] (+2 more) | LFPiece_ScreenPos (logflume2.c) -192 |  |
| `0x0040cf30` | 10 | 24 | called by 0x0040d6f0 [unmatched] | LFPiece_ScreenPos (logflume2.c) -160 |  |
| `0x0040cf50` | 15 | 39 | called by 0x0040d900 [unmatched] (+1 more) | LFPiece_ScreenPos (logflume2.c) -128 |  |
| `0x0040cf80` | 14 | 32 | called by 0x0040d900 [unmatched] (+1 more) | LFPiece_ScreenPos (logflume2.c) -80 |  |
| `0x0040cfa0` | 15 | 41 | called by 0x0040d900 [unmatched] (+1 more) | LFPiece_ScreenPos (logflume2.c) -48 |  |

### Group 2 - `0x0040d420`..`0x0040dba7`: 6 functions, 602 instructions (602 live in 6, 0 dead in 0), 1889 bytes

Neighbours: logflume2.c x3, logflume.c x3. Reached from: logflume.c x26, within group x3.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x0040d420` | 73 | 244 | called by 0x0040d6f0 [unmatched] | LFPiece_TickCommon (logflume2.c) +112 |  |
| `0x0040d520` | 128 | 460 | called by LFTrack_Update (logflume.c) (+1 more) | LFPiece_TickCommon (logflume2.c) +368 |  |
| `0x0040d6f0` | 151 | 520 | called by LFDrop_Update (logflume.c) (+7 more) | LFPiece_TickCommon (logflume2.c) +832 |  |
| `0x0040d900` | 83 | 258 | called by LFDrop_Add (logflume.c) (+7 more) | LFTrack_Tick (logflume.c) -688 |  |
| `0x0040da10` | 106 | 240 | called by LFTrack_Remove (logflume.c) (+1 more) | LFTrack_Tick (logflume.c) -416 |  |
| `0x0040db00` | 61 | 167 | called by LFDrop_Remove (logflume.c) (+7 more) | LFTrack_Tick (logflume.c) -176 |  |

### Group 3 - `0x00411e20`..`0x0041f4e0`: 38 functions, 1432 instructions (1230 live in 24, 202 dead in 14), 3921 bytes

Neighbours: schoolcar8.c x5, coaster.c x5, coaster9.c x5. Reached from: within group x16, coaster9.c x5, coaster5.c x2, group 4 x2, lfmisc2.c x1, bswater2.c x1.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00411e20` | 1 | 1 | unreferenced (swept) | LFQueue_Append (lfmisc.c) -16 | DEAD |
| `0x00411f70` | 13 | 34 | unreferenced (swept) | LFQueue_AddRider (lfentrance.c) +80 | DEAD |
| `0x00411fa0` | 74 | 177 | called by LFQueue_StepFront (lfmisc2.c) | LFQueue_AddRider (lfentrance.c) +128 |  |
| `0x004120e0` | 7 | 22 | unreferenced (swept) | BuildWalkPath (mappath.c) -32 | DEAD |
| `0x0041b130` | 7 | 27 | unreferenced (swept) | GetInterface (loaders.c) -32 | DEAD |
| `0x0041c940` | 130 | 339 | called by BoatingSchool_BuildRoute (bswater2.c) (+1 more) | BoatingSchool_BuildRoute (bswater2.c) +128 |  |
| `0x0041cd20` | 7 | 19 | called by 0x0041cd80 [unmatched] | TrackClass_GetWorldBounds (schoolcar8.c) +64 |  |
| `0x0041cd40` | 27 | 55 | called by 0x0041d210 [unmatched] | TrackClass_GetWorldBounds (schoolcar8.c) +96 |  |
| `0x0041cd80` | 45 | 132 | called by BuildJoint (coastermath.c) | InitTrackJoint (coaster.c) -144 |  |
| `0x0041cf20` | 26 | 67 | unreferenced (swept) | Track_CountTailPieces (schoolcar8.c) +32 | DEAD |
| `0x0041d040` | 6 | 13 | unreferenced (swept) | FindTrackNodeAt (coaster.c) -32 | DEAD |
| `0x0041d050` | 6 | 13 | unreferenced (swept) | FindTrackNodeAt (coaster.c) -16 | DEAD |
| `0x0041d210` | 60 | 199 | called by TrackFitCheckSpan (coaster5.c) (+1 more) | BuildJoint (coastermath.c) +64 |  |
| `0x0041d950` | 66 | 183 | called by Route_Reset (schoolcar7.c) | PositionRouteCars (schoolcar.c) -192 |  |
| `0x0041db20` | 37 | 104 | pointer in 0x0041db90 [unmatched] | Route_SumCarVelocity (coaster8.c) +64 |  |
| `0x0041db90` | 77 | 259 | called by RoutePhys_EvaluateDerivative (coaster7.c) (+1 more) | Route_SumCarVelocity (coaster8.c) +176 |  |
| `0x0041dca0` | 29 | 82 | called by Route_TravelThisTick (coaster9.c) | Route_TravelPerTick (coaster9.c) -96 | declared Route_GetSpeed in coaster9.c |
| `0x0041e260` | 32 | 69 | unreferenced (swept) | Route_UpdateTimer (coastertiny.c) +32 | DEAD |
| `0x0041e2f0` | 25 | 55 | unreferenced (swept) | Route_FindFreeSeat (schoolcar8.c) +64 | DEAD |
| `0x0041e640` | 11 | 27 | called by RouteNode_AddPending (coaster9.c) | RouteNode_ClearActive (coaster9.c) +16 | declared RouteNode_SetPending in coaster9.c |
| `0x0041e670` | 14 | 34 | called by RouteNode_AddPending (coaster9.c) | RouteNode_IsPending (coaster9.c) +16 | declared RouteNode_CanAdd in coaster9.c |
| `0x0041e720` | 27 | 56 | called by 0x0041e260 [unmatched] | RouteNode_FindFreeSeat (coaster8.c) -64 | DEAD |
| `0x0041e790` | 23 | 46 | called by 0x0041e2f0 [unmatched] | RouteNode_FindFreeSeat (coaster8.c) +48 | DEAD |
| `0x0041e7c0` | 14 | 30 | unreferenced (swept) | RouteNode_GetAcceleration (coastertiny.c) -32 | DEAD |
| `0x0041e7f0` | 10 | 30 | called by 0x0041db20 [unmatched] | RouteNode_GetAcceleration (coastertiny.c) +16 |  |
| `0x0041e8f0` | 25 | 60 | called by 0x0041d950 [unmatched] | RouteNode_GetTailTangent (schoolcar8.c) -64 |  |
| `0x0041e9e0` | 45 | 144 | called by RouteNode_UpdateClipRect (coaster9.c) (+1 more) | RouteNode_UpdateClipRect (coaster9.c) +80 | declared RouteNode_GetTransform in coaster9.c |
| `0x0041ea70` | 40 | 120 | called by RouteNode_AddPending (coaster9.c) | RouteNode_AddPending (coaster9.c) -128 | declared RouteNode_LinkPending in coaster9.c |
| `0x0041ede0` | 38 | 96 | called by 0x0041ee40 [unmatched] | TrackRemoveObject (coaster.c) +48 |  |
| `0x0041ee40` | 79 | 192 | called by 0x0041cd20 [unmatched] | TrackRemoveObject (coaster.c) +144 |  |
| `0x0041ef10` | 1 | 1 | unreferenced (swept) | RouteSystemInit (schoolcar.c) +16 | DEAD |
| `0x0041ef60` | 80 | 194 | called by Raster_SubmitPoly (coaster3d.c) | SetSpanClip (coastermath.c) +64 |  |
| `0x0041f030` | 4 | 20 | called by 0x0041ef60 [unmatched] | SetSpanClip (coastermath.c) +272 |  |
| `0x0041f050` | 179 | 593 | called by 0x0041f2b0 [unmatched] | SetSpanClip (coastermath.c) +304 |  |
| `0x0041f2b0` | 55 | 149 | called by 0x0041ef60 [unmatched] (+1 more) | Span_SetClip (coaster7.c) -208 |  |
| `0x0041f350` | 14 | 35 | unreferenced (swept) | Span_SetClip (coaster7.c) -48 | DEAD |
| `0x0041f3e0` | 85 | 212 | called by 0x0041f4e0 [unmatched] | Span_SetClip (coaster7.c) +96 |  |
| `0x0041f4c0` | 13 | 32 | called by 0x0041f4e0 [unmatched] | Span_SetClip (coaster7.c) +320 |  |

### Group 4 - `0x0041f4e0`..`0x00420a13`: 19 functions, 1339 instructions (958 live in 9, 381 dead in 10), 3937 bytes

Neighbours: schoolcar8.c x12, coastertiny.c x3, coaster7.c x2. Reached from: within group x6, group 7 x4, group 3 x1, group 5 x1, coaster9.c x1; tables at `0x004b5600`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x0041f4e0` | 78 | 180 | called by 0x0041db90 [unmatched] (+2 more) | Span_SetClip (coaster7.c) +352 |  |
| `0x0041f5a0` | 63 | 170 | unreferenced (swept) | Span_SetClip (coaster7.c) +544 | DEAD |
| `0x0041f650` | 80 | 191 | called by 0x0041f720 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) -512 | DEAD |
| `0x0041f710` | 5 | 12 | called by 0x0041f720 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) -320 | DEAD |
| `0x0041f720` | 34 | 108 | called by 0x0041f7f0 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) -304 | DEAD |
| `0x0041f790` | 33 | 78 | pointer in 0x0041f7f0 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) -192 | DEAD |
| `0x0041f7e0` | 4 | 9 | pointer in 0x0041f7f0 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) -112 | DEAD |
| `0x0041f7f0` | 22 | 85 | unreferenced (swept) | TrackCursor_AdvanceGeometry (schoolcar8.c) -96 | DEAD |
| `0x0041f880` | 29 | 68 | called by 0x00429f30 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) +48 |  |
| `0x0041f8d0` | 106 | 307 | table at 0x004b5648 in .data | TrackCursor_AdvanceGeometry (schoolcar8.c) +128 |  |
| `0x0041fa10` | 134 | 393 | unreferenced (swept) | TrackCursor_AdvanceGeometry (schoolcar8.c) +448 | DEAD |
| `0x0041fba0` | 136 | 399 | table at 0x004b564c in .data | CoasterShades_InitClamp (schoolcar8.c) -400 |  |
| `0x0041fd80` | 162 | 505 | table at 0x004b5658 in .data | CoasterShades_InitClamp (schoolcar8.c) +80 |  |
| `0x0041ff80` | 202 | 633 | table at 0x004b565c in .data | CoasterShades_InitClamp (schoolcar8.c) +592 |  |
| `0x00420200` | 81 | 258 | called by 0x0042a1b0 [unmatched] | Phys_Step (schoolcar6.c) -272 |  |
| `0x00420520` | 1 | 1 | unreferenced (swept) | CoasterModel_SetDirectory (coastertiny.c) -16 | DEAD |
| `0x00420780` | 3 | 12 | called by 0x00428860 [unmatched] (+1 more) | FindCoasterPart (coastertiny.c) -16 |  |
| `0x004207b0` | 5 | 13 | unreferenced (swept) | CoasterModel_LoadPalette (coastertiny.c) +16 | DEAD |
| `0x00420810` | 161 | 515 | called by Coaster3D_DrawModel (coaster9.c) | FindCoasterColour (coaster8.c) +64 | declared CoasterModel_DrawPass1 in coaster9.c |

### Group 5 - `0x00420a20`..`0x0042313c`: 17 functions, 1443 instructions (532 live in 7, 911 dead in 10), 4421 bytes

Neighbours: coaster9.c x4, coastertiny.c x4, schoolcar8.c x2. Reached from: coaster9.c x5, coaster7.c x2, within group x2, group 6 x1, coaster3d.c x1, group 7 x1.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00420a20` | 165 | 530 | called by Coaster3D_DrawModel (coaster9.c) | FindCoasterColour (coaster8.c) +592 | declared CoasterModel_DrawPass2 in coaster9.c |
| `0x00420c40` | 182 | 579 | called by Coaster3D_DrawModel (coaster9.c) | Coaster3D_DrawModel (coaster9.c) -592 | declared CoasterModel_DrawPass3 in coaster9.c |
| `0x00420fb0` | 12 | 32 | called by RouteNode_UpdateClipRect (coaster9.c) | Coaster3D_DrawModel (coaster9.c) +288 | declared CoasterModel_GetClipRect in coaster9.c |
| `0x00420fd0` | 114 | 349 | unreferenced (swept) | Coaster3D_DrawModel (coaster9.c) +320 | DEAD |
| `0x00421130` | 112 | 353 | unreferenced (swept) | PhysVec_Add (schoolcar8.c) -368 | DEAD |
| `0x00421530` | 3 | 10 | unreferenced (swept) | PhysVec_InitOps (schoolcar8.c) -16 | DEAD |
| `0x00421ab0` | 49 | 133 | called by Castle_InitEntranceTrack (coaster7.c) (+2 more) | TrackCurve_LineUpVector (coastertiny.c) +32 |  |
| `0x00421ce0` | 38 | 114 | called by Castle_InitEntranceTrack (coaster7.c) (+1 more) | TrackCurve_CubicUpVector (coastertiny.c) +32 |  |
| `0x00422180` | 43 | 134 | called by 0x00429560 [unmatched] | CarClassTablesInit (schoolcar.c) -144 |  |
| `0x00422400` | 43 | 107 | called by CoasterModel_FindPartIndex (coaster9.c) (+1 more) | CountModelRecords (coaster7.c) +64 | declared ModelImage_FindName in coaster9.c |
| `0x00422520` | 43 | 106 | called by 0x00422650 [unmatched] | CoasterModel_FindMeshIndex (coaster9.c) -112 | DEAD |
| `0x004225e0` | 8 | 24 | unreferenced (swept) | CoasterModel_GetMeshCount (coastertiny.c) +16 | DEAD |
| `0x00422650` | 31 | 105 | unreferenced (swept) | CoasterModel_GetPartCount (coastertiny.c) +16 | DEAD |
| `0x004227a0` | 8 | 27 | unreferenced (swept) | LoadCoasterModelSet (schoolcar4.c) +224 | DEAD |
| `0x004227c0` | 521 | 1613 | unreferenced (swept) | LoadCoasterModelSet (schoolcar4.c) +256 | DEAD |
| `0x00423060` | 7 | 17 | unreferenced (swept) | CoasterShades_Init (schoolcar5.c) +128 | DEAD |
| `0x00423080` | 64 | 188 | unreferenced (swept) | CoasterShades_Init (schoolcar5.c) +160 | DEAD |

### Group 6 - `0x00423200`..`0x00428857`: 44 functions, 1420 instructions (850 live in 27, 570 dead in 17), 3881 bytes

Neighbours: coastertiny.c x11, coaster.c x9, castleobj.c x7. Reached from: within group x11, group 7 x5, coaster9.c x3, coaster3d.c x2, renderview.c x1, group 5 x1; tables at `0x004b5b00`, `0x004b5d00`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00423200` | 61 | 175 | called by Raster_SubmitPoly (coaster3d.c) | ZBuffer_RunCommand (schoolcar5.c) -176 |  |
| `0x00423750` | 1 | 1 | unreferenced (swept) | CoasterGeomInit (schoolcar.c) +16 | DEAD |
| `0x004237a0` | 28 | 65 | unreferenced (swept) | Raster_RestoreState (coastertiny.c) +16 | DEAD |
| `0x004237f0` | 51 | 162 | called by 0x004267b0 [unmatched] (+1 more) | Raster_RestoreState (coastertiny.c) +96 | DEAD |
| `0x004238a0` | 49 | 133 | called by 0x00428b80 [unmatched] (+1 more) | Raster_RestoreState (coastertiny.c) +272 | DEAD |
| `0x00423930` | 2 | 6 | unreferenced (swept) | Castle_GetFirstCorner (coastertiny.c) -128 | DEAD |
| `0x00423940` | 8 | 37 | table at 0x004b5b64 in .data | Castle_GetFirstCorner (coastertiny.c) -112 |  |
| `0x00423970` | 6 | 23 | table at 0x004b5b68 in .data | Castle_GetFirstCorner (coastertiny.c) -64 |  |
| `0x00423990` | 2 | 6 | table at 0x004b5b6c in .data | Castle_GetFirstCorner (coastertiny.c) -32 |  |
| `0x004239a0` | 1 | 1 | unreferenced (swept) | Castle_GetFirstCorner (coastertiny.c) -16 | DEAD |
| `0x00423f40` | 86 | 264 | called by 0x00424050 [unmatched] | RemoveAllTrackNodes (coaster.c) +128 |  |
| `0x00424050` | 87 | 232 | called by RenderFullMap (renderview.c) | GetCastleRec (castleobj.c) -240 |  |
| `0x00425da0` | 25 | 60 | called by 0x0042a110 [unmatched] | Invert2x2 (coastermath.c) -64 |  |
| `0x00426000` | 51 | 217 | unreferenced (swept) | MatIdentity (coastermath.c) -240 | DEAD |
| `0x004260e0` | 2 | 7 | unreferenced (swept) | MatIdentity (coastermath.c) -16 | DEAD |
| `0x00426190` | 21 | 46 | called by Mat3_TransposeToMat4 [declared in coaster9.c] | MatMul (coastermath.c) +112 |  |
| `0x004261c0` | 42 | 101 | called by Coaster3D_DrawModel (coaster9.c) (+1 more) | TransformVerts (coaster3d.c) -144 | declared TransformVec3 in coaster9.c |
| `0x00426230` | 11 | 31 | called by 0x00428b80 [unmatched] (+1 more) | TransformVerts (coaster3d.c) -32 | DEAD |
| `0x004263a0` | 72 | 188 | called by 0x00426750 [unmatched] | Mat3_ToMat4 (coaster7.c) -240 |  |
| `0x00426460` | 21 | 46 | called by 0x0042a680 [unmatched] | Mat3_ToMat4 (coaster7.c) -48 |  |
| `0x00426510` | 18 | 66 | called by Coaster3D_DrawModel (coaster9.c) | MakeTransform (coastermath.c) +48 | declared Mat3_TransposeToMat4 in coaster9.c |
| `0x004265d0` | 66 | 122 | called by ClipRect_SetBounds [declared in coaster9.c] (+1 more) | MakeRotation (coaster6.c) +112 |  |
| `0x00426680` | 14 | 45 | unreferenced (swept) | AnyCoasterRegionFullyInside (schoolcar.c) +48 | DEAD |
| `0x00426700` | 19 | 52 | called by RouteNode_UpdateClipRect (coaster9.c) | ClipRect_Unlink (coaster9.c) +32 | declared ClipRect_SetBounds in coaster9.c |
| `0x00426750` | 24 | 81 | called by CoasterModel_GetClipRect [declared in coaster9.c] | Raster_ResetClipRing (coastertiny.c) +16 |  |
| `0x004267b0` | 56 | 153 | unreferenced (swept) | Raster_ResetClipRing (coastertiny.c) +112 | DEAD |
| `0x00426850` | 80 | 261 | unreferenced (swept) | Raster_ResetClipRing (coastertiny.c) +272 | DEAD |
| `0x00426be0` | 26 | 61 | unreferenced (swept) | PackCoasterPtr (coaster.c) +32 | DEAD |
| `0x004272a0` | 44 | 106 | unreferenced (swept) | ReadCoasterBlob (coaster.c) +96 | DEAD |
| `0x00427310` | 78 | 169 | unreferenced (swept) | RouteSeat_IsOccupied (coaster9.c) -176 | DEAD |
| `0x004274f0` | 48 | 125 | unreferenced (swept) | RouteSeat_InitPosition (coaster8.c) +64 | DEAD |
| `0x00427570` | 28 | 62 | unreferenced (swept) | Track_Update (coaster.c) -96 | DEAD |
| `0x004275b0` | 1 | 1 | table at 0x004b5d4c in .data | Track_Update (coaster.c) -32 |  |
| `0x004275c0` | 1 | 1 | table at 0x004b5d48 in .data | Track_Update (coaster.c) -16 |  |
| `0x00427a40` | 24 | 62 | table at 0x004b5d3c in .data | Track_Interact (castleobj.c) +64 |  |
| `0x00427a80` | 6 | 19 | table at 0x004b5d40 in .data | Track_Create (coaster.c) -32 |  |
| `0x00427c30` | 24 | 62 | table at 0x004b5d74 in .data | FindTrackDesc (castleobj.c) +48 |  |
| `0x00427c70` | 7 | 20 | table at 0x004b5d78 in .data | TrackH_Update (coaster.c) -32 |  |
| `0x00427f70` | 39 | 124 | table at 0x004b5df4 in .data | TrackH0_Create (castleobj.c) +64 |  |
| `0x00427ff0` | 39 | 124 | table at 0x004b5df0 in .data | TrackHP_Create (castleobj.c) -128 |  |
| `0x00428350` | 30 | 109 | called by Coaster3D_BuildPieceGeometry (coaster3d.c) | TrackHP_Add (castleobj.c) +80 |  |
| `0x004283c0` | 103 | 205 | called by 0x004286e0 [unmatched] | TrackHP_Add (castleobj.c) +192 |  |
| `0x004286e0` | 8 | 27 | table at 0x004b5d44 in .data | DrawTrackNode (coaster.c) -32 |  |
| `0x00428840` | 10 | 23 | called by 0x00428860 [unmatched] | InitTrackDrawModes (coaster4.c) +240 |  |

### Group 7 - `0x00428860`..`0x0042a77f`: 22 functions, 1245 instructions (1077 live in 20, 168 dead in 2), 3807 bytes

Neighbours: schoolcar.c x8, coaster9.c x7, coastertiny.c x3. Reached from: within group x13, group 3 x4, coaster5.c x3, coaster9.c x3, schoolcar4.c x1, schoolcar.c x1; tables at `0x004b5f00`, `0x004b6300`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00428860` | 254 | 771 | table at 0x004b5f50 in .data | InitTrackDrawModes (coaster4.c) +272 |  |
| `0x00428b80` | 98 | 290 | unreferenced (swept) | CoasterSceneInit (schoolcar.c) +16 | DEAD |
| `0x00429560` | 90 | 302 | called by TrackJoinPieces (coaster5.c) | DrawTrackPiece3D (coaster.c) +112 |  |
| `0x00429690` | 35 | 82 | called by TrackJoinPieces (coaster5.c) | TrackRunSpanEnd (coaster6.c) -96 |  |
| `0x004298a0` | 41 | 108 | called by TrackFitCheckSpan (coaster5.c) | TrackFitEndGeom (coaster6.c) +96 |  |
| `0x00429a80` | 25 | 61 | called by TrackCurve_EvaluateOffset (coaster9.c) (+1 more) | LevelTrackRun (schoolcar.c) +80 | declared TrackCurve_EvaluatePosition in coaster9.c |
| `0x00429ac0` | 12 | 33 | called by 0x00429b60 [unmatched] (+1 more) | LevelTrackRun (schoolcar.c) +144 |  |
| `0x00429af0` | 39 | 100 | called by RouteNode_GetTransform [declared in coaster9.c] (+1 more) | LevelTrackRun (schoolcar.c) +192 |  |
| `0x00429b60` | 16 | 44 | called by 0x0042a680 [unmatched] | TrackCurve_EvaluateOffset (coaster9.c) -80 |  |
| `0x00429b90` | 11 | 28 | called by TrackCurve_EvaluateOffset (coaster9.c) (+1 more) | TrackCurve_EvaluateOffset (coaster9.c) -32 | declared TrackCurve_EvaluateUp in coaster9.c |
| `0x00429c10` | 22 | 73 | pointer in TrackCurve_EvaluateDerivative [declared in coaster9.c] | TrackCurve_EvaluateOffset (coaster9.c) +96 |  |
| `0x00429c60` | 42 | 142 | called by Route_TravelPerTick (coaster9.c) (+1 more) | TrackCurve_EvaluateOffset (coaster9.c) +176 | declared TrackCurve_EvaluateDerivative in coaster9.c |
| `0x00429cf0` | 93 | 297 | pointer in 0x00429f30 [unmatched] (+1 more) | TrackCurve_EvaluateOffset (coaster9.c) +320 |  |
| `0x00429e20` | 76 | 265 | table at 0x004b63fc in .data | TrackCurve_EvaluateOffset (coaster9.c) +624 |  |
| `0x00429f30` | 70 | 232 | called by RouteCar_SetPosition (schoolcar4.c) (+2 more) | TrackCurve_EvaluateOffset (coaster9.c) +896 |  |
| `0x0042a020` | 70 | 232 | unreferenced (swept) | CoasterFxPoolInit (schoolcar.c) -704 | DEAD |
| `0x0042a110` | 28 | 59 | called by 0x0042a1b0 [unmatched] | CoasterFxPoolInit (schoolcar.c) -464 |  |
| `0x0042a150` | 28 | 84 | pointer in 0x0042a1b0 [unmatched] | CoasterFxPoolInit (schoolcar.c) -400 |  |
| `0x0042a1b0` | 92 | 292 | called by Coaster_StationDerivative (coaster7.c) | CoasterFxPoolInit (schoolcar.c) -304 |  |
| `0x0042a5e0` | 19 | 50 | called by 0x0041e8f0 [unmatched] | TrackCursor_Init (coastertiny.c) -64 |  |
| `0x0042a670` | 2 | 7 | called by 0x0042a680 [unmatched] | TrackCursor_Evaluate (coastertiny.c) +48 |  |
| `0x0042a680` | 82 | 255 | called by RouteNode_LinkPending [declared in coaster9.c] | TrackCursor_Evaluate (coastertiny.c) +64 |  |

### Group 8 - `0x00434810`..`0x0043f4e2`: 12 functions, 1430 instructions (153 live in 2, 1277 dead in 10), 4252 bytes

Neighbours: screencb2.c x3, loaders.c x3, junglecruise.c x2. Reached from: within group x3, ridemachine.c x2, group 9 x1.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00434810` | 4 | 14 | unreferenced (swept) | JcMonkeyFish_GetDrawDesc (screencb2.c) +208 | DEAD |
| `0x00434820` | 14 | 61 | unreferenced (swept) | JcMonkeyFish_GetDrawDesc (screencb2.c) +224 | DEAD |
| `0x00434860` | 119 | 335 | unreferenced (swept) | JcMonkeyFish_GetDrawDesc (screencb2.c) +288 | DEAD |
| `0x004349b0` | 94 | 362 | unreferenced (swept) | JcDeco_Remove (junglecruise.c) -400 | DEAD |
| `0x00434b20` | 7 | 19 | unreferenced (swept) | JcDeco_Remove (junglecruise.c) -32 | DEAD |
| `0x0043a940` | 37 | 108 | called by SpaceTower_StepMachine (ridemachine.c) | SpaceTower_CountSeated (bswater3.c) -112 | declared SpaceTower_StepCar in ridemachine.c |
| `0x0043b810` | 116 | 384 | called by SpaceTower_StepMachine (ridemachine.c) | SpaceTower_GetInterfaces (interfaces.c) +144 | declared SpaceTower_UpdateRiders in ridemachine.c |
| `0x0043e930` | 101 | 244 | called by 0x0043ea30 [unmatched] | PlaneRide_Activate (mechrides.c) +1312 | DEAD |
| `0x0043ea30` | 395 | 1187 | called by 0x0043f0b0 [unmatched] (+1 more) | PlaneRide_Activate (mechrides.c) +1568 | DEAD |
| `0x0043eee0` | 173 | 464 | unreferenced (swept) | LoadPos (loaders.c) -1920 | DEAD |
| `0x0043f0b0` | 313 | 944 | unreferenced (swept) | LoadPos (loaders.c) -1456 | DEAD |
| `0x0043f460` | 57 | 130 | called by 0x0043f4f0 [unmatched] | LoadPos (loaders.c) -512 | DEAD |

### Group 9 - `0x0043f4f0`..`0x00444a67`: 48 functions, 1408 instructions (1277 live in 47, 131 dead in 1), 4525 bytes

Neighbours: uimisc.c x33, savemisc2.c x4, screens3.c x4. Reached from: within group x10, group 11 x6, texture.c x2, group 17 x2, rides.c x1, data2.c x1; tables at `0x004b7e00`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x0043f4f0` | 131 | 355 | unreferenced (swept) | LoadPos (loaders.c) -368 | DEAD |
| `0x00441830` | 16 | 50 | called by ObjNextRider (texture.c) (+1 more) | Render_SetViewport (sweep1.c) +48 | declared RiderCursorSeek in texture.c |
| `0x00441910` | 35 | 98 | called by 0x00441980 [unmatched] | GetObjRiderN (savemisc2.c) +80 |  |
| `0x00441980` | 81 | 221 | called by Put3DBlokesOnRide (rides.c) | GetObjRiderN (savemisc2.c) +192 |  |
| `0x004427e0` | 57 | 117 | called by 0x00442980 [unmatched] | SkipStrings (savemisc2.c) -224 |  |
| `0x00442860` | 45 | 86 | called by 0x00442980 [unmatched] | SkipStrings (savemisc2.c) -96 |  |
| `0x00442980` | 229 | 752 | called by InitMan (data2.c) | LookupTextureName (data3.c) +144 |  |
| `0x00443bd0` | 123 | 381 | called by 0x00444090 [unmatched] | RenderAdvisorIcon (screens3.c) -608 |  |
| `0x00443d50` | 22 | 60 | called by 0x00444150 [unmatched] | RenderAdvisorIcon (screens3.c) -224 |  |
| `0x00443d90` | 5 | 40 | called by 0x00444090 [unmatched] | RenderAdvisorIcon (screens3.c) -160 |  |
| `0x00443dc0` | 33 | 102 | called by RenderAdvisorIcon (screens3.c) (+1 more) | RenderAdvisorIcon (screens3.c) -112 |  |
| `0x00443f90` | 17 | 53 | called by 0x00444020 [unmatched] | SetAdvisorPose (tinystubs.c) -224 |  |
| `0x00443fe0` | 15 | 44 | called by 0x00444020 [unmatched] | SetAdvisorPose (tinystubs.c) -144 |  |
| `0x00444020` | 17 | 71 | pointer in 0x00444090 [unmatched] | SetAdvisorPose (tinystubs.c) -80 |  |
| `0x00444090` | 51 | 177 | called by 0x00459520 [unmatched] | SetAdvisorPose (tinystubs.c) +32 |  |
| `0x00444150` | 46 | 149 | called by 0x00459520 [unmatched] | SaveReport (uimisc.c) -176 |  |
| `0x004441f0` | 2 | 11 | called by ResetLevelGlobals [declared in movie.c] | SaveReport (uimisc.c) -16 |  |
| `0x004442c0` | 15 | 46 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +96 |  |
| `0x004442f0` | 15 | 46 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +144 |  |
| `0x00444320` | 15 | 46 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +192 |  |
| `0x00444350` | 15 | 46 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +240 |  |
| `0x00444380` | 15 | 46 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +288 |  |
| `0x004443b0` | 14 | 48 | table at 0x004b7e38 in .data | LoadReport (uimisc.c) +336 |  |
| `0x004443e0` | 14 | 48 | table at 0x004b7e3c in .data | LoadReport (uimisc.c) +384 |  |
| `0x00444410` | 14 | 48 | table at 0x004b7e40 in .data | LoadReport (uimisc.c) +432 |  |
| `0x00444440` | 14 | 48 | table at 0x004b7e44 in .data | LoadReport (uimisc.c) +480 |  |
| `0x00444470` | 16 | 54 | table at 0x004b7e48 in .data | LoadReport (uimisc.c) +528 |  |
| `0x004444b0` | 16 | 54 | table at 0x004b7e4c in .data | LoadReport (uimisc.c) +592 |  |
| `0x004444f0` | 16 | 55 | table at 0x004b7e50 in .data | LoadReport (uimisc.c) +656 |  |
| `0x00444530` | 16 | 55 | table at 0x004b7e54 in .data | LoadReport (uimisc.c) +720 |  |
| `0x00444570` | 16 | 55 | table at 0x004b7e58 in .data | LoadReport (uimisc.c) +784 |  |
| `0x004445b0` | 15 | 55 | table at 0x004b7e5c in .data | LoadReport (uimisc.c) +848 |  |
| `0x004445f0` | 15 | 55 | table at 0x004b7e60 in .data | LoadReport (uimisc.c) +912 |  |
| `0x00444630` | 13 | 55 | table at 0x004b7e64 in .data | LoadReport (uimisc.c) +976 |  |
| `0x00444670` | 13 | 55 | table at 0x004b7e68 in .data | LoadReport (uimisc.c) +1040 |  |
| `0x004446b0` | 13 | 55 | table at 0x004b7e6c in .data | LoadReport (uimisc.c) +1104 |  |
| `0x004446f0` | 13 | 55 | table at 0x004b7e70 in .data | LoadReport (uimisc.c) +1168 |  |
| `0x00444730` | 13 | 55 | table at 0x004b7e74 in .data | LoadReport (uimisc.c) +1232 |  |
| `0x00444770` | 13 | 55 | table at 0x004b7e78 in .data | LoadReport (uimisc.c) +1296 |  |
| `0x004447b0` | 13 | 55 | table at 0x004b7e7c in .data | LoadReport (uimisc.c) +1360 |  |
| `0x004447f0` | 13 | 55 | table at 0x004b7e80 in .data | LoadReport (uimisc.c) +1424 |  |
| `0x00444830` | 13 | 55 | table at 0x004b7e84 in .data | LoadReport (uimisc.c) +1488 |  |
| `0x00444870` | 13 | 55 | table at 0x004b7e88 in .data | LoadReport (uimisc.c) +1552 |  |
| `0x004448b0` | 13 | 55 | table at 0x004b7e8c in .data | LoadReport (uimisc.c) +1616 |  |
| `0x004448f0` | 13 | 55 | table at 0x004b7e90 in .data | LoadReport (uimisc.c) +1680 |  |
| `0x00444930` | 13 | 55 | table at 0x004b7e94 in .data | LoadReport (uimisc.c) +1744 |  |
| `0x00444970` | 13 | 55 | table at 0x004b7e98 in .data | LoadReport (uimisc.c) +1808 |  |
| `0x004449b0` | 60 | 183 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +1872 |  |

### Group 10 - `0x00444a70`..`0x00445395`: 16 functions, 734 instructions (734 live in 16, 0 dead in 0), 2221 bytes

Neighbours: uimisc.c x16. Reached from: group 11 x11, within group x10.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00444a70` | 98 | 246 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2064 |  |
| `0x00444b70` | 42 | 122 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2320 |  |
| `0x00444bf0` | 29 | 74 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2448 |  |
| `0x00444c40` | 13 | 38 | called by 0x00444c70 [unmatched] | LoadReport (uimisc.c) +2528 |  |
| `0x00444c70` | 34 | 84 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2576 |  |
| `0x00444cd0` | 26 | 65 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2672 |  |
| `0x00444d20` | 26 | 65 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2752 |  |
| `0x00444d70` | 42 | 118 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2832 |  |
| `0x00444df0` | 74 | 186 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2960 |  |
| `0x00444eb0` | 14 | 60 | called by 0x00444ef0 [unmatched] (+1 more) | LoadReport (uimisc.c) +3152 |  |
| `0x00444ef0` | 48 | 157 | pointer in 0x00445190 [unmatched] (+1 more) | LoadReport (uimisc.c) +3216 |  |
| `0x00444f90` | 27 | 98 | pointer in 0x00445190 [unmatched] (+1 more) | LoadReport (uimisc.c) +3376 |  |
| `0x00445000` | 83 | 256 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +3488 |  |
| `0x00445100` | 49 | 143 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +3744 |  |
| `0x00445190` | 91 | 376 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +3888 |  |
| `0x00445310` | 38 | 133 | called by 0x00445190 [unmatched] (+2 more) | LoadReport (uimisc.c) +4272 |  |

### Group 11 - `0x004453a0`..`0x0044db06`: 1 functions, 8085 instructions (8085 live in 1, 0 dead in 0), 34662 bytes

Neighbours: uimisc.c x1. Reached from: group 12 x1.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x004453a0` | 8085 | 34662 | called by 0x0044db90 [unmatched] | LoadReport (uimisc.c) +4416 | LONG |

### Group 12 - `0x0044db20`..`0x0044fdc9`: 15 functions, 1599 instructions (1597 live in 14, 2 dead in 1), 4523 bytes

Neighbours: rin.c x6, sweep1.c x5, sysmisc3.c x2. Reached from: within group x10, group 24 x3, group 20 x1, group 16 x1, group 25 x1, rides.c x1; tables at `0x004b8300`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x0044db20` | 2 | 11 | called by ResetLevelGlobals [declared in movie.c] (+1 more) | LoadBinV (rin.c) -368 |  |
| `0x0044db30` | 2 | 6 | unreferenced (swept) | LoadBinV (rin.c) -352 | DEAD |
| `0x0044db40` | 17 | 54 | called by 0x0046a040 [unmatched] (+1 more) | LoadBinV (rin.c) -336 |  |
| `0x0044db80` | 4 | 13 | called by ResetLevelGlobals [declared in movie.c] | LoadBinV (rin.c) -272 |  |
| `0x0044db90` | 60 | 213 | called by 0x00458ee0 [unmatched] | LoadBinV (rin.c) -256 |  |
| `0x0044dc70` | 9 | 30 | called by ResetLevelGlobals [declared in movie.c] (+1 more) | LoadBinV (rin.c) -32 |  |
| `0x0044ebf0` | 92 | 266 | table at 0x004b8370 in .data | PopLongTermAction (sweep1.c) +32 |  |
| `0x0044ed00` | 32 | 106 | called by 0x0044f610 [unmatched] (+1 more) | PopLongTermAction (sweep1.c) +304 |  |
| `0x0044ed70` | 328 | 936 | table at 0x004b8374 in .data | PopLongTermAction (sweep1.c) +416 |  |
| `0x0044f170` | 3 | 11 | table at 0x004b836c in .data | IsObjectRunning (sysmisc3.c) -496 |  |
| `0x0044f180` | 201 | 472 | called by 0x0044f610 [unmatched] | IsObjectRunning (sysmisc3.c) -480 |  |
| `0x0044f3d0` | 15 | 38 | called by RemoveBlokeFromRide (rides.c) (+1 more) | PutBlokeInList (sweep1.c) -96 |  |
| `0x0044f400` | 14 | 34 | called by 0x0044f610 [unmatched] | PutBlokeInList (sweep1.c) -48 |  |
| `0x0044f4a0` | 121 | 356 | called by 0x0044f610 [unmatched] (+2 more) | RemoveBlokeFromList (blokelist.c) +48 |  |
| `0x0044f610` | 699 | 1977 | table at 0x004b8380 in .data | RemoveBlokeFromList (blokelist.c) +416 |  |

### Group 13 - `0x0044fe10`..`0x004515d4`: 18 functions, 972 instructions (655 live in 7, 317 dead in 11), 2614 bytes

Neighbours: blokeai.c x6, sysmisc.c x6, sysmisc2.c x5. Reached from: within group x6, group 16 x1; tables at `0x004b8300`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x0044fe10` | 43 | 112 | table at 0x004b83c4 in .data | DoHighLevelAI (blokeai.c) -1728 |  |
| `0x0044fe80` | 337 | 928 | table at 0x004b839c in .data | DoHighLevelAI (blokeai.c) -1616 |  |
| `0x00450250` | 77 | 203 | table at 0x004b83a0 in .data | DoHighLevelAI (blokeai.c) -640 |  |
| `0x00450330` | 43 | 100 | table at 0x004b83b8 in .data | DoHighLevelAI (blokeai.c) -416 |  |
| `0x004503a0` | 87 | 161 | called by 0x00450450 [unmatched] | DoHighLevelAI (blokeai.c) -304 |  |
| `0x00450450` | 48 | 117 | table at 0x004b83a4 in .data | DoHighLevelAI (blokeai.c) -128 |  |
| `0x00450a40` | 20 | 58 | called by 0x00458ee0 [unmatched] | SaveBuildSlots (savegame2.c) -64 |  |
| `0x004511e0` | 1 | 1 | unreferenced (swept) | RES_FindVolumeOnResPath (sysmisc2.c) +256 | DEAD |
| `0x004511f0` | 1 | 1 | called by 0x00451210 [unmatched] | RES_FindVolumeOnResPath (sysmisc2.c) +272 | DEAD |
| `0x00451200` | 1 | 1 | unreferenced (swept) | RES_FindVolumeOnResPath (sysmisc2.c) +288 | DEAD |
| `0x00451210` | 35 | 110 | unreferenced (swept) | RES_FindVolumeOnResPath (sysmisc2.c) +304 | DEAD |
| `0x00451280` | 88 | 260 | called by 0x00451210 [unmatched] | RES_FindVolumeOnResPath (sysmisc2.c) +416 | DEAD |
| `0x00451390` | 39 | 123 | unreferenced (swept) | RES_EnsureMounted (sysmisc.c) -592 | DEAD |
| `0x00451410` | 35 | 107 | called by 0x00451210 [unmatched] | RES_EnsureMounted (sysmisc.c) -464 | DEAD |
| `0x00451480` | 9 | 27 | unreferenced (swept) | RES_EnsureMounted (sysmisc.c) -352 | DEAD |
| `0x004514a0` | 4 | 14 | called by 0x00451210 [unmatched] | RES_EnsureMounted (sysmisc.c) -320 | DEAD |
| `0x004514b0` | 55 | 159 | unreferenced (swept) | RES_EnsureMounted (sysmisc.c) -304 | DEAD |
| `0x00451550` | 49 | 132 | called by 0x00451210 [unmatched] | RES_EnsureMounted (sysmisc.c) -144 | DEAD |

### Group 14 - `0x00451740`..`0x0045490b`: 12 functions, 1463 instructions (1443 live in 11, 20 dead in 1), 4885 bytes

Neighbours: text.c x5, sysstubs.c x4, sysmisc.c x1. Reached from: within group x12, render5.c x1, group 17 x1, CRT x1.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00451740` | 620 | 1760 | called by SaveCertificateBitmap (render5.c) | RES_EnsureMounted (sysmisc.c) +352 |  |
| `0x00451f40` | 9 | 34 | called by 0x00459520 [unmatched] | ScrollFromKeys (mapscreen4.c) -48 |  |
| `0x00453c20` | 20 | 71 | unreferenced (swept) | __DEBUG_FREE (memdb.c) +48 | DEAD |
| `0x00453d10` | 48 | 143 | called from CRT 0x004a0996 | DBError (sysstubs.c) +48 | SEH |
| `0x00453da0` | 345 | 1256 | called by 0x00453d10 [unmatched] | DBError (sysstubs.c) +192 |  |
| `0x00454290` | 22 | 75 | called by 0x00453da0 [unmatched] (+3 more) | DBError (sysstubs.c) +1456 |  |
| `0x004542e0` | 62 | 157 | called by 0x00453da0 [unmatched] | DBError (sysstubs.c) +1536 |  |
| `0x00454380` | 110 | 380 | called by 0x004542e0 [unmatched] | LoadBubbleHelpGFX (text.c) -1424 |  |
| `0x00454500` | 52 | 147 | called by 0x00454380 [unmatched] (+1 more) | LoadBubbleHelpGFX (text.c) -1040 |  |
| `0x004545a0` | 97 | 348 | called by 0x00453da0 [unmatched] | LoadBubbleHelpGFX (text.c) -880 |  |
| `0x00454700` | 64 | 487 | called by 0x00453da0 [unmatched] | LoadBubbleHelpGFX (text.c) -528 |  |
| `0x004548f0` | 14 | 27 | called by 0x00453da0 [unmatched] | LoadBubbleHelpGFX (text.c) -32 |  |

### Group 15 - `0x00454a10`..`0x00457a62`: 13 functions, 1023 instructions (806 live in 10, 217 dead in 3), 2806 bytes

Neighbours: fpui3.c x3, text.c x2, fpui2.c x2. Reached from: within group x5, group 16 x4, group 20 x4, group 8 x2, fpui2.c x2, group 17 x1.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00454a10` | 92 | 302 | called by 0x00459520 [unmatched] | LoadBubbleHelpGFX (text.c) +256 |  |
| `0x004551a0` | 46 | 123 | called by 0x0043ea30 [unmatched] | PrintCentColref (text.c) +320 | DEAD |
| `0x00455220` | 120 | 335 | called by 0x0043ea30 [unmatched] | BubbleHelp (bighelp.c) -336 | DEAD |
| `0x00455a10` | 25 | 64 | called by 0x00455a50 [unmatched] | HTBubbleHelp (fpui2.c) +592 |  |
| `0x00455a50` | 122 | 342 | pointer in 0x00455bb0 [unmatched] | HTBubbleHelp (fpui2.c) +656 |  |
| `0x00455bb0` | 74 | 208 | called by HTBubbleHelp (fpui2.c) (+3 more) | FindCachedText (fpui3.c) -400 |  |
| `0x00455c80` | 75 | 183 | called by 0x00455e50 [unmatched] | FindCachedText (fpui3.c) -192 |  |
| `0x00455de0` | 51 | 109 | unreferenced (swept) | FindCachedText (fpui3.c) +160 | DEAD |
| `0x00455e50` | 49 | 109 | called by DrawPopUpExtra (popup2.c) (+6 more) | PrintCachedEntry (tinystubs.c) -112 |  |
| `0x00455fc0` | 275 | 762 | called by 0x00458ee0 [unmatched] | ExpireCachedText (render5.c) +80 |  |
| `0x00457870` | 6 | 17 | called by StartFreePlayPark (uimisc2.c) (+4 more) | BricksAreLimited (tinystubs.c) -32 | declared sub_457870 in uimisc2.c |
| `0x00457900` | 3 | 10 | called by 0x00478c60 [unmatched] (+2 more) | SaveCurrency (sysstubs.c) -16 |  |
| `0x00457970` | 85 | 242 | called by 0x00457a70 [unmatched] | LoadCurrency (sysstubs.c) +48 |  |

### Group 16 - `0x00457a70`..`0x0045933f`: 10 functions, 1403 instructions (1403 live in 10, 0 dead in 0), 5526 bytes

Neighbours: sysstubs.c x6, movie.c x2, mapscreen.c x1. Reached from: within group x4, group 17 x4, screens3.c x4, group 14 x1.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00457a70` | 751 | 2889 | called by 0x00458ee0 [unmatched] | LoadCurrency (sysstubs.c) +304 |  |
| `0x00458830` | 49 | 129 | called by 0x00453d10 [unmatched] | RenderFrontEndScreen (mapscreen.c) +240 |  |
| `0x004588c0` | 31 | 110 | called by 0x00459520 [unmatched] | sub_458930 (bnvpath.c) -112 |  |
| `0x004589a0` | 45 | 168 | called by 0x00459520 [unmatched] | EnterParkPlayMode (movie.c) +96 |  |
| `0x00458a50` | 45 | 198 | called by LowProgressAcceptInput (screens3.c) (+2 more) | EnterParkPlayMode (movie.c) +272 |  |
| `0x00458b20` | 29 | 144 | called by LoadAcceptInput (screens3.c) (+1 more) | SetMapReady (sysstubs.c) -144 |  |
| `0x00458bc0` | 4 | 31 | called by 0x00459520 [unmatched] | SetMapReady (sysstubs.c) +16 |  |
| `0x00458be0` | 7 | 30 | called by StopScript (screens3.c) | SetMapReady (sysstubs.c) +48 |  |
| `0x00458c00` | 153 | 708 | called by 0x00459520 [unmatched] | SetMapReady (sysstubs.c) +80 |  |
| `0x00458ee0` | 289 | 1119 | called by 0x00458c00 [unmatched] | SetMapReady (sysstubs.c) +816 |  |

### Group 17 - `0x00459360`..`0x00462ee6`: 18 functions, 1404 instructions (1003 live in 14, 401 dead in 4), 4375 bytes

Neighbours: uimisc2.c x4, objmap.c x3, mapbuild.c x2. Reached from: within group x4, group 26 x3, bigrender.c x2, group 20 x2, group 16 x1, group 27 x1.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00459360` | 102 | 376 | called by 0x00458c00 [unmatched] | RunLevelEndSequence (uimisc2.c) -944 |  |
| `0x004594e0` | 5 | 16 | called by 0x00459520 [unmatched] | RunLevelEndSequence (uimisc2.c) -560 |  |
| `0x004594f0` | 11 | 42 | called by 0x00459520 [unmatched] | RunLevelEndSequence (uimisc2.c) -544 |  |
| `0x00459520` | 121 | 489 | called by 0x0047f880 [unmatched] | RunLevelEndSequence (uimisc2.c) -496 |  |
| `0x004597e0` | 20 | 56 | called by 0x0044dc70 [unmatched] (+1 more) | EndLevel (uimisc.c) -64 |  |
| `0x004598b0` | 6 | 23 | unreferenced (swept) | TallyFootprintCell (mapbuild.c) -32 | DEAD |
| `0x00459970` | 116 | 344 | called by 0x0046ad30 [unmatched] | ResetBuildTimer (mapbuild.c) +16 |  |
| `0x0045ac20` | 50 | 155 | called by 0x00459520 [unmatched] | GetTileBounds (pathbuild.c) -160 |  |
| `0x0045ade0` | 291 | 885 | unreferenced (swept) | GetTileCentre (tilehelp.c) +128 | DEAD |
| `0x0045e930` | 15 | 40 | called by 0x0045e960 [unmatched] | ClearObjectUserFlags (objrect.c) +224 | DEAD |
| `0x0045e960` | 89 | 221 | unreferenced (swept) | GetObjectDoorOffset (objdoor.c) -224 | DEAD |
| `0x0045fad0` | 160 | 458 | called by RenderCursor (bigrender.c) | CalcBasicObjectCursor (objmap.c) +80 |  |
| `0x0045fca0` | 195 | 606 | called by RenderCursor (bigrender.c) | CalcBasicObjectCursor (objmap.c) +544 |  |
| `0x00460f50` | 81 | 201 | called by DrawCursorTileAt (pathmask.c) | DrawPathTileOverlay (render5.c) +192 | declared DrawCursorPathTile in pathmask.c |
| `0x004629e0` | 113 | 333 | called by 0x0046cb20 [unmatched] | AddOvSav (loaders.c) -336 |  |
| `0x00462e50` | 6 | 22 | called by 0x0047ab00 [unmatched] (+1 more) | ResetMapAI (objmap.c) +128 |  |
| `0x00462e70` | 6 | 22 | called by 0x0047ab80 [unmatched] (+1 more) | DoMapAI (bigsim.c) -128 |  |
| `0x00462e90` | 17 | 86 | called by ResetLevelGlobals [declared in movie.c] | DoMapAI (bigsim.c) -96 |  |

### Group 18 - `0x004632b0`..`0x004677a6`: 12 functions, 1308 instructions (1307 live in 11, 1 dead in 1), 4007 bytes

Neighbours: softblit2.c x4, bigrender.c x2, sysmisc.c x2. Reached from: softblit2.c x4, group 16 x1, group 24 x1, group 11 x1, gpu.c x1, movie.c x1; tables at `0x004b9c00`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x004632b0` | 98 | 311 | called by 0x00458ee0 [unmatched] | RateBlokeOnLeaving (workers.c) -320 |  |
| `0x00463560` | 5 | 21 | called by ResetLevelGlobals [declared in movie.c] | ProcessDamage (scrolltick.c) -32 |  |
| `0x004636c0` | 16 | 36 | called by 0x004453a0 [unmatched] | InstallDirectDraw (sweep2.c) -48 |  |
| `0x004640f0` | 70 | 251 | tail-jumped from PushSetTarget (gpu.c) | PushRenderingStatusAndUnlockVideoSurface (surface.c) +112 |  |
| `0x00465850` | 113 | 331 | called by RunMovie (movie.c) | SoftPrint_XBltFast (bigrender.c) -496 | declared BlitDIBToScreen in movie.c |
| `0x004659a0` | 57 | 152 | called by RenderAdvisorIcon (screens3.c) | SoftPrint_XBltFast (bigrender.c) -160 |  |
| `0x00466070` | 1 | 1 | unreferenced (swept) | FlipPrimary (sysmisc.c) -352 | DEAD |
| `0x00466080` | 102 | 336 | table at 0x004b9ca4 in .data | FlipPrimary (sysmisc.c) -336 |  |
| `0x00466d80` | 325 | 1011 | called by SoftBlitRLEPlain (softblit2.c) | SoftBlitRLEPlain (softblit2.c) +1552 |  |
| `0x00467180` | 202 | 611 | called by SoftBlitRLEPlain (softblit2.c) | SoftBlitRLEPlain (softblit2.c) +2576 |  |
| `0x004673f0` | 197 | 588 | called by SoftBlitRLEPlain (softblit2.c) | SoftBlitRLEPlain (softblit2.c) +3200 |  |
| `0x00467640` | 122 | 358 | called by SoftBlitRLEPlain (softblit2.c) | SoftBlitRLEPlain (softblit2.c) +3792 |  |

### Group 19 - `0x004677b0`..`0x004689ed`: 14 functions, 1431 instructions (1431 live in 14, 0 dead in 0), 4375 bytes

Neighbours: sysstubs.c x12, softblit2.c x1, tinystubs.c x1. Reached from: group 21 x5, softblit2.c x4, group 26 x3, group 25 x2, group 24 x2, render3.c x1.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x004677b0` | 271 | 833 | called by SoftBlitRLEPlain (softblit2.c) | SoftBlitRLEPlain (softblit2.c) +4160 |  |
| `0x00467b00` | 180 | 526 | called by SoftBlitRLEPlain (softblit2.c) | ClearScriptStateBytes (sysstubs.c) -3392 |  |
| `0x00467d10` | 167 | 489 | called by SoftBlitRLEPlain (softblit2.c) | ClearScriptStateBytes (sysstubs.c) -2864 |  |
| `0x00467f00` | 114 | 311 | called by SoftBlitRLEPlain (softblit2.c) | ClearScriptStateBytes (sysstubs.c) -2368 |  |
| `0x00468040` | 303 | 965 | called by SoftBlitRLE (render3.c) | ClearScriptStateBytes (sysstubs.c) -2048 |  |
| `0x00468410` | 312 | 992 | called by SoftPrint_XBltFast (bigrender.c) | ClearScriptStateBytes (sysstubs.c) -1072 |  |
| `0x004687f0` | 8 | 31 | called by 0x00478e20 [unmatched] (+1 more) | ClearScriptStateBytes (sysstubs.c) -80 |  |
| `0x00468810` | 8 | 31 | called by 0x00478e90 [unmatched] (+1 more) | ClearScriptStateBytes (sysstubs.c) -48 |  |
| `0x00468830` | 4 | 13 | called by ResetLevelGlobals [declared in movie.c] | ClearScriptStateBytes (sysstubs.c) -16 |  |
| `0x00468860` | 13 | 38 | called by 0x0047a020 [unmatched] (+1 more) | ClearScriptStateBytes (sysstubs.c) +32 |  |
| `0x00468890` | 11 | 33 | called by 0x0047a0b0 [unmatched] (+1 more) | ClearScriptStateBytes (sysstubs.c) +80 |  |
| `0x004688c0` | 7 | 19 | called by 0x0046b0c0 [unmatched] | ScriptState_NoOp (sysstubs.c) -32 |  |
| `0x004688f0` | 7 | 17 | called by 0x0047a140 [unmatched] (+1 more) | ScriptState_NoOp (sysstubs.c) +16 |  |
| `0x004689a0` | 26 | 77 | called by 0x0046cb20 [unmatched] (+1 more) | FreeScriptEventList (tinystubs.c) +48 |  |

### Group 20 - `0x004689f0`..`0x0046a43a`: 63 functions, 1493 instructions (1471 live in 62, 22 dead in 1), 4438 bytes

Neighbours: softblit.c x29, tinystubs.c x15, savemisc2.c x12. Reached from: within group x102, group 21 x43, group 24 x3, group 26 x3, group 25 x1, movie.c x1; tables at `0x004b9d00`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x004689f0` | 91 | 264 | called by ResetLevelGlobals [declared in movie.c] (+1 more) | FreeScriptEventList (tinystubs.c) +128 |  |
| `0x00468c80` | 27 | 77 | called by 0x00468d30 [unmatched] (+22 more) | KillObjectHelp (fpui5.c) +128 |  |
| `0x00468cd0` | 15 | 37 | called by 0x00468d30 [unmatched] (+22 more) | ResetScriptTimer (tinystubs.c) -48 |  |
| `0x00468d10` | 9 | 32 | called by 0x0046b180 [unmatched] (+26 more) | ResetScriptTimer (tinystubs.c) +16 |  |
| `0x00468d30` | 27 | 71 | called by 0x0046b180 [unmatched] (+26 more) | ResetScriptTimer (tinystubs.c) +48 |  |
| `0x00468d80` | 20 | 59 | called by 0x0046aae0 [unmatched] (+3 more) | ResetScriptTimer (tinystubs.c) +128 |  |
| `0x00468dc0` | 19 | 59 | called by 0x0046a750 [unmatched] (+1 more) | ResetScriptTimer (tinystubs.c) +192 |  |
| `0x00468e00` | 19 | 59 | called by 0x0046a750 [unmatched] | ResetScriptTimer (tinystubs.c) +256 |  |
| `0x00468e40` | 31 | 83 | called by 0x0046a900 [unmatched] | ResetScriptTimer (tinystubs.c) +320 |  |
| `0x00468ea0` | 31 | 83 | called by 0x0046aa70 [unmatched] | ResetScriptTimer (tinystubs.c) +416 |  |
| `0x00468f00` | 18 | 52 | called by 0x0046a960 [unmatched] | ResetScriptTimer (tinystubs.c) +512 |  |
| `0x00468f40` | 20 | 59 | called by 0x0046aa30 [unmatched] | ResetScriptTimer (tinystubs.c) +576 |  |
| `0x00468f80` | 18 | 52 | called by 0x0046abd0 [unmatched] | ResetScriptTimer (tinystubs.c) +640 |  |
| `0x00468fc0` | 18 | 52 | called by 0x0046aec0 [unmatched] | ResetScriptTimer (tinystubs.c) +704 |  |
| `0x00469000` | 19 | 54 | called by 0x0046aec0 [unmatched] | ResetScriptTimer (tinystubs.c) +768 |  |
| `0x00469040` | 18 | 52 | called by 0x0046af10 [unmatched] | ResetScriptTimer (tinystubs.c) +832 |  |
| `0x00469080` | 19 | 54 | called by 0x0046af10 [unmatched] | RemoveGoals (savemisc2.c) -816 |  |
| `0x004690c0` | 18 | 52 | called by 0x0046ae40 [unmatched] | RemoveGoals (savemisc2.c) -752 |  |
| `0x00469100` | 20 | 59 | called by 0x0046ae70 [unmatched] | RemoveGoals (savemisc2.c) -688 |  |
| `0x00469140` | 21 | 66 | called by 0x0046af60 [unmatched] | RemoveGoals (savemisc2.c) -624 |  |
| `0x00469190` | 21 | 66 | called by 0x0046af60 [unmatched] | RemoveGoals (savemisc2.c) -544 |  |
| `0x004691e0` | 20 | 59 | called by 0x0046b080 [unmatched] (+1 more) | RemoveGoals (savemisc2.c) -464 |  |
| `0x00469220` | 20 | 59 | called by 0x0046ac50 [unmatched] (+1 more) | RemoveGoals (savemisc2.c) -400 |  |
| `0x00469260` | 31 | 83 | called by 0x0046aae0 [unmatched] | RemoveGoals (savemisc2.c) -336 |  |
| `0x004692c0` | 22 | 66 | unreferenced (swept) | RemoveGoals (savemisc2.c) -240 | DEAD |
| `0x00469310` | 20 | 59 | called by 0x0046adf0 [unmatched] (+4 more) | RemoveGoals (savemisc2.c) -160 |  |
| `0x00469350` | 18 | 52 | called by 0x0046ad30 [unmatched] | RemoveGoals (savemisc2.c) -96 |  |
| `0x00469390` | 8 | 21 | called by 0x0046ab70 [unmatched] | RemoveGoals (savemisc2.c) -32 |  |
| `0x00469900` | 43 | 121 | called by UnlockSidePanelObjects (movie.c) (+2 more) | UpdateGoalHelpText (softblit.c) +1280 | declared MarkElemAvailable in movie.c |
| `0x00469980` | 86 | 247 | called by 0x00469a80 [unmatched] | UpdateGoalHelpText (softblit.c) +1408 |  |
| `0x00469a80` | 14 | 45 | called by 0x00469b50 [unmatched] | UpdateGoalHelpText (softblit.c) +1664 |  |
| `0x00469ab0` | 13 | 46 | called by EnsureObjectClassLoaded [declared in movie.c] (+1 more) | UpdateGoalHelpText (softblit.c) +1712 |  |
| `0x00469ae0` | 8 | 27 | called by 0x0046ae30 [unmatched] (+3 more) | UpdateGoalHelpText (softblit.c) +1760 |  |
| `0x00469b00` | 8 | 27 | called by 0x0046b1e0 [unmatched] | UpdateGoalHelpText (softblit.c) +1792 |  |
| `0x00469b20` | 16 | 44 | table at 0x004b9d4c in .data | UpdateGoalHelpText (softblit.c) +1824 |  |
| `0x00469b50` | 7 | 22 | table at 0x004b9d50 in .data | UpdateGoalHelpText (softblit.c) +1872 |  |
| `0x00469b70` | 7 | 22 | table at 0x004b9d54 in .data | UpdateGoalHelpText (softblit.c) +1904 |  |
| `0x00469b90` | 8 | 27 | table at 0x004b9d58 in .data | UpdateGoalHelpText (softblit.c) +1936 |  |
| `0x00469bb0` | 7 | 22 | table at 0x004b9d5c in .data | UpdateGoalHelpText (softblit.c) +1968 |  |
| `0x00469bd0` | 35 | 111 | called by 0x0047a5a0 [unmatched] (+1 more) | UpdateGoalHelpText (softblit.c) +2000 |  |
| `0x00469c40` | 9 | 26 | table at 0x004b9d60 in .data | UpdateGoalHelpText (softblit.c) +2112 |  |
| `0x00469c60` | 7 | 18 | pointer in 0x00469c80 [unmatched] | UpdateGoalHelpText (softblit.c) +2144 |  |
| `0x00469c80` | 177 | 577 | table at 0x004b9d64 in .data | UpdateGoalHelpText (softblit.c) +2176 |  |
| `0x00469ed0` | 29 | 76 | table at 0x004b9d68 in .data | UpdateGoalHelpText (softblit.c) +2768 |  |
| `0x00469f20` | 29 | 74 | table at 0x004b9d6c in .data | UpdateGoalHelpText (softblit.c) +2848 |  |
| `0x00469f70` | 5 | 14 | table at 0x004b9d70 in .data | UpdateGoalHelpText (softblit.c) +2928 |  |
| `0x00469f80` | 18 | 62 | table at 0x004b9d74 in .data | UpdateGoalHelpText (softblit.c) +2944 |  |
| `0x00469fc0` | 33 | 112 | table at 0x004b9d78 in .data | UpdateGoalHelpText (softblit.c) +3008 |  |
| `0x0046a030` | 5 | 14 | table at 0x004b9d7c in .data | UpdateGoalHelpText (softblit.c) +3120 |  |
| `0x0046a040` | 40 | 162 | called by 0x0047a8e0 [unmatched] (+1 more) | UpdateGoalHelpText (softblit.c) +3136 |  |
| `0x0046a120` | 9 | 26 | table at 0x004b9d80 in .data | UpdateGoalHelpText (softblit.c) +3360 |  |
| `0x0046a140` | 12 | 34 | called by 0x0047a960 [unmatched] (+1 more) | UpdateGoalHelpText (softblit.c) +3392 |  |
| `0x0046a170` | 11 | 30 | table at 0x004b9da4 in .data | UpdateGoalHelpText (softblit.c) +3440 |  |
| `0x0046a190` | 33 | 83 | table at 0x004b9d84 in .data | UpdateGoalHelpText (softblit.c) +3472 |  |
| `0x0046a1f0` | 19 | 55 | table at 0x004b9d88 in .data | UpdateGoalHelpText (softblit.c) +3568 |  |
| `0x0046a230` | 78 | 204 | table at 0x004b9d8c in .data | UpdateGoalHelpText (softblit.c) +3632 |  |
| `0x0046a300` | 12 | 40 | table at 0x004b9d90 in .data | UpdateGoalHelpText (softblit.c) +3840 |  |
| `0x0046a330` | 9 | 26 | table at 0x004b9d94 in .data | ScriptEventDue (uimisc.c) -3792 |  |
| `0x0046a350` | 9 | 26 | table at 0x004b9d98 in .data | ScriptEventDue (uimisc.c) -3760 |  |
| `0x0046a370` | 5 | 20 | table at 0x004b9d9c in .data | ScriptEventDue (uimisc.c) -3728 |  |
| `0x0046a390` | 8 | 25 | table at 0x004b9dc4 in .data | ScriptEventDue (uimisc.c) -3696 |  |
| `0x0046a3b0` | 37 | 106 | table at 0x004b9da0 in .data | ScriptEventDue (uimisc.c) -3664 |  |
| `0x0046a420` | 9 | 26 | table at 0x004b9da8 in .data | ScriptEventDue (uimisc.c) -3552 |  |

### Group 21 - `0x0046a440`..`0x0046b8bb`: 55 functions, 1495 instructions (1481 live in 54, 14 dead in 1), 3754 bytes

Neighbours: uimisc.c x46, sysstubs.c x6, uimisc3.c x2. Reached from: group 22 x61, within group x7, group 26 x4, group 24 x2, group 25 x1, screens3.c x1; tables at `0x004b9d00`, `0x004b9e00`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x0046a440` | 9 | 26 | table at 0x004b9dac in .data | ScriptEventDue (uimisc.c) -3520 |  |
| `0x0046a460` | 9 | 26 | table at 0x004b9db0 in .data | ScriptEventDue (uimisc.c) -3488 |  |
| `0x0046a480` | 7 | 22 | table at 0x004b9db4 in .data | ScriptEventDue (uimisc.c) -3456 |  |
| `0x0046a4a0` | 7 | 22 | table at 0x004b9db8 in .data | ScriptEventDue (uimisc.c) -3424 |  |
| `0x0046a4c0` | 9 | 26 | table at 0x004b9dbc in .data | ScriptEventDue (uimisc.c) -3392 |  |
| `0x0046a4e0` | 3 | 11 | table at 0x004b9dc0 in .data | ScriptEventDue (uimisc.c) -3360 |  |
| `0x0046a4f0` | 29 | 67 | table at 0x004b9dc8 in .data | ScriptEventDue (uimisc.c) -3344 |  |
| `0x0046a540` | 43 | 108 | table at 0x004b9dcc in .data | ScriptEventDue (uimisc.c) -3264 |  |
| `0x0046a5b0` | 87 | 221 | table at 0x004b9dd0 in .data | ScriptEventDue (uimisc.c) -3152 |  |
| `0x0046a690` | 68 | 158 | table at 0x004b9dd4 in .data | ScriptEventDue (uimisc.c) -2928 |  |
| `0x0046a730` | 12 | 32 | called by 0x0046a750 [unmatched] | ScriptEventDue (uimisc.c) -2768 |  |
| `0x0046a750` | 165 | 430 | table at 0x004b9dd8 in .data | ScriptEventDue (uimisc.c) -2736 |  |
| `0x0046a900` | 44 | 92 | table at 0x004b9ddc in .data | ScriptEventDue (uimisc.c) -2304 |  |
| `0x0046a960` | 81 | 197 | table at 0x004b9de0 in .data | ScriptEventDue (uimisc.c) -2208 |  |
| `0x0046aa30` | 22 | 51 | table at 0x004b9de4 in .data | ScriptEventDue (uimisc.c) -2000 |  |
| `0x0046aa70` | 50 | 105 | table at 0x004b9de8 in .data | ScriptEventDue (uimisc.c) -1936 |  |
| `0x0046aae0` | 60 | 130 | table at 0x004b9dec in .data | ScriptEventDue (uimisc.c) -1824 |  |
| `0x0046ab70` | 28 | 72 | table at 0x004b9df0 in .data | ScriptEventDue (uimisc.c) -1680 |  |
| `0x0046abc0` | 5 | 14 | table at 0x004b9df4 in .data | ScriptEventDue (uimisc.c) -1600 |  |
| `0x0046abd0` | 14 | 38 | table at 0x004b9df8 in .data | ScriptEventDue (uimisc.c) -1584 |  |
| `0x0046ac00` | 31 | 67 | table at 0x004b9e00 in .data | ScriptEventDue (uimisc.c) -1536 |  |
| `0x0046ac50` | 69 | 172 | table at 0x004b9dfc in .data | ScriptEventDue (uimisc.c) -1456 |  |
| `0x0046ad00` | 15 | 40 | table at 0x004b9e04 in .data | ScriptEventDue (uimisc.c) -1280 |  |
| `0x0046ad30` | 15 | 43 | table at 0x004b9e08 in .data | ScriptEventDue (uimisc.c) -1232 |  |
| `0x0046ad60` | 15 | 40 | table at 0x004b9e0c in .data | ScriptEventDue (uimisc.c) -1184 |  |
| `0x0046ad90` | 15 | 40 | table at 0x004b9e10 in .data | ScriptEventDue (uimisc.c) -1136 |  |
| `0x0046adc0` | 15 | 40 | table at 0x004b9e14 in .data | ScriptEventDue (uimisc.c) -1088 |  |
| `0x0046adf0` | 15 | 40 | table at 0x004b9e18 in .data | ScriptEventDue (uimisc.c) -1040 |  |
| `0x0046ae20` | 2 | 6 | table at 0x004b9e1c in .data | ScriptEventDue (uimisc.c) -992 |  |
| `0x0046ae30` | 5 | 14 | table at 0x004b9e20 in .data | ScriptEventDue (uimisc.c) -976 |  |
| `0x0046ae40` | 18 | 46 | table at 0x004b9e24 in .data | ScriptEventDue (uimisc.c) -960 |  |
| `0x0046ae70` | 34 | 74 | table at 0x004b9e28 in .data | ScriptEventDue (uimisc.c) -912 |  |
| `0x0046aec0` | 31 | 72 | table at 0x004b9e2c in .data | ScriptEventDue (uimisc.c) -832 |  |
| `0x0046af10` | 31 | 72 | table at 0x004b9e30 in .data | ScriptEventDue (uimisc.c) -752 |  |
| `0x0046af60` | 59 | 128 | table at 0x004b9e34 in .data | ScriptEventDue (uimisc.c) -672 |  |
| `0x0046afe0` | 61 | 146 | table at 0x004b9e38 in .data | ScriptEventDue (uimisc.c) -544 |  |
| `0x0046b080` | 15 | 39 | table at 0x004b9e3c in .data | ScriptEventDue (uimisc.c) -384 |  |
| `0x0046b0b0` | 2 | 6 | table at 0x004b9e40 in .data | ScriptEventDue (uimisc.c) -336 |  |
| `0x0046b0c0` | 22 | 56 | table at 0x004b9e44 in .data | ScriptEventDue (uimisc.c) -320 |  |
| `0x0046b100` | 18 | 47 | table at 0x004b9e48 in .data | ScriptEventDue (uimisc.c) -256 |  |
| `0x0046b130` | 30 | 76 | table at 0x004b9e4c in .data | ScriptEventDue (uimisc.c) -208 |  |
| `0x0046b180` | 29 | 83 | table at 0x004b9e50 in .data | ScriptEventDue (uimisc.c) -128 |  |
| `0x0046b1e0` | 5 | 14 | table at 0x004b9e54 in .data | ScriptEventDue (uimisc.c) -32 |  |
| `0x0046b1f0` | 2 | 3 | table at 0x004b9e58 in .data | ScriptEventDue (uimisc.c) -16 |  |
| `0x0046b590` | 23 | 57 | called by 0x004787d0 [unmatched] | FreeScriptStepList (tinystubs.c) +48 |  |
| `0x0046b610` | 10 | 30 | called by 0x0046c510 [unmatched] (+35 more) | UnlinkScriptStep (uimisc3.c) +64 |  |
| `0x0046b630` | 11 | 29 | called by 0x0046bc40 [unmatched] (+30 more) | UnlinkScriptStep (uimisc3.c) +96 |  |
| `0x0046b650` | 39 | 87 | called by 0x004791a0 [unmatched] | ShowScriptStepText (uimisc.c) -96 |  |
| `0x0046b700` | 23 | 81 | called by ScriptEndIconInput (screens3.c) | ShowScriptStepText (uimisc.c) +80 |  |
| `0x0046b790` | 14 | 44 | called by 0x0047a480 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +48 |  |
| `0x0046b7c0` | 14 | 44 | unreferenced (swept) | RestoreScriptStepHelp (sysstubs.c) +96 | DEAD |
| `0x0046b7f0` | 12 | 37 | called by 0x0047a500 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +144 |  |
| `0x0046b820` | 12 | 37 | called by 0x0047a550 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +192 |  |
| `0x0046b850` | 12 | 37 | called by 0x00478c60 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +240 |  |
| `0x0046b880` | 19 | 59 | called by 0x0047a5a0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +288 |  |

### Group 22 - `0x0046b8c0`..`0x0046eec1`: 76 functions, 1444 instructions (1303 live in 70, 141 dead in 6), 4323 bytes

Neighbours: uimisc3.c x33, sysstubs.c x29, iconui.c x4. Reached from: group 25 x28, group 26 x27, group 27 x8, group 16 x3, within group x3, group 23 x3.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x0046b8c0` | 22 | 61 | called by 0x0047a650 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +352 |  |
| `0x0046b900` | 22 | 61 | called by 0x0047a6a0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +416 |  |
| `0x0046b940` | 22 | 61 | called by 0x0047a6f0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +480 |  |
| `0x0046b980` | 22 | 61 | called by 0x0047a7b0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +544 |  |
| `0x0046b9c0` | 17 | 43 | called by 0x0047a800 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +608 |  |
| `0x0046b9f0` | 18 | 50 | called by 0x0047a860 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +656 |  |
| `0x0046ba30` | 17 | 43 | called by 0x0047a8a0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +720 |  |
| `0x0046ba60` | 14 | 44 | called by 0x0047a8e0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +768 |  |
| `0x0046ba90` | 16 | 51 | called by 0x0047a960 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +816 |  |
| `0x0046bad0` | 19 | 59 | called by 0x00479060 [unmatched] (+1 more) | RestoreScriptStepHelp (sysstubs.c) +880 |  |
| `0x0046bb10` | 14 | 44 | called by 0x00478f00 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +944 |  |
| `0x0046bb40` | 16 | 51 | called by 0x0047ac00 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +992 |  |
| `0x0046bb80` | 14 | 44 | called by 0x0047ad40 [unmatched] (+1 more) | RestoreScriptStepHelp (sysstubs.c) +1056 |  |
| `0x0046bbb0` | 14 | 44 | called by 0x0047ab00 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1104 |  |
| `0x0046bbe0` | 14 | 44 | called by 0x0047ab80 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1152 |  |
| `0x0046bc10` | 12 | 37 | called by 0x0047ada0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1200 |  |
| `0x0046bc40` | 10 | 30 | called by 0x0047af80 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1248 |  |
| `0x0046bc60` | 10 | 30 | called by 0x0047af50 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1280 |  |
| `0x0046bc80` | 14 | 44 | called by 0x0047a020 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1312 |  |
| `0x0046bcb0` | 14 | 44 | called by 0x0047a0b0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1360 |  |
| `0x0046bce0` | 14 | 44 | called by 0x0047a140 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1408 |  |
| `0x0046bd10` | 17 | 43 | called by 0x00478e20 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1456 |  |
| `0x0046bd40` | 17 | 43 | called by 0x00478e90 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1504 |  |
| `0x0046bd70` | 14 | 44 | called by 0x0047aea0 [unmatched] (+1 more) | RestoreScriptStepHelp (sysstubs.c) +1552 |  |
| `0x0046bda0` | 15 | 45 | called by 0x00478d30 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1600 |  |
| `0x0046bdd0` | 15 | 47 | called by 0x004791f0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1648 |  |
| `0x0046be00` | 18 | 55 | called by 0x00479270 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1696 |  |
| `0x0046be40` | 27 | 78 | called by 0x00479300 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1760 |  |
| `0x0046be90` | 13 | 40 | called by 0x00479390 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1712 |  |
| `0x0046bec0` | 13 | 40 | called by 0x004793e0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1664 |  |
| `0x0046bef0` | 17 | 54 | called by 0x00479450 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1616 |  |
| `0x0046bf30` | 25 | 71 | called by 0x004794d0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1552 |  |
| `0x0046bf80` | 15 | 47 | called by 0x00479550 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1472 |  |
| `0x0046bfb0` | 17 | 54 | called by 0x004795c0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1424 |  |
| `0x0046bff0` | 17 | 54 | called by 0x00479640 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1360 |  |
| `0x0046c030` | 15 | 47 | called by 0x004796d0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1296 |  |
| `0x0046c060` | 15 | 47 | called by 0x00479740 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1248 |  |
| `0x0046c090` | 13 | 40 | called by 0x00479800 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1200 |  |
| `0x0046c0c0` | 15 | 47 | called by 0x00479850 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1152 |  |
| `0x0046c0f0` | 15 | 47 | called by 0x004798c0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1104 |  |
| `0x0046c120` | 13 | 40 | called by 0x00479930 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1056 |  |
| `0x0046c150` | 13 | 40 | called by 0x00479980 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1008 |  |
| `0x0046c180` | 13 | 40 | called by 0x004799d0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -960 |  |
| `0x0046c1b0` | 13 | 40 | called by 0x00479a20 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -912 |  |
| `0x0046c1e0` | 13 | 40 | called by 0x00479a70 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -864 |  |
| `0x0046c210` | 13 | 40 | called by 0x00479ac0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -816 |  |
| `0x0046c240` | 25 | 71 | called by 0x00479c40 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -768 |  |
| `0x0046c290` | 13 | 40 | called by 0x00479cb0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -688 |  |
| `0x0046c2c0` | 15 | 47 | called by 0x00479d00 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -640 |  |
| `0x0046c2f0` | 13 | 40 | called by 0x00479d60 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -592 |  |
| `0x0046c320` | 13 | 40 | called by 0x00479db0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -544 |  |
| `0x0046c350` | 17 | 54 | called by 0x00479e00 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -496 |  |
| `0x0046c390` | 15 | 47 | called by 0x00479e80 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -432 |  |
| `0x0046c3c0` | 13 | 40 | called by 0x00479ee0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -384 |  |
| `0x0046c3f0` | 15 | 47 | called by 0x00479f30 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -336 |  |
| `0x0046c420` | 15 | 47 | called by 0x00479fa0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -288 |  |
| `0x0046c450` | 13 | 40 | called by 0x0047a2f0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -240 |  |
| `0x0046c480` | 13 | 40 | called by 0x0047a360 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -192 |  |
| `0x0046c4b0` | 13 | 40 | called by 0x0047a3d0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -144 |  |
| `0x0046c4e0` | 13 | 40 | unreferenced (swept) | EnqueueStepStartEvent (uimisc3.c) -96 | DEAD |
| `0x0046c510` | 11 | 33 | called by 0x0047a440 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -48 |  |
| `0x0046cb20` | 13 | 57 | called by 0x00458b20 [unmatched] | LoadScripts (savechunks.c) -64 |  |
| `0x0046ce00` | 5 | 18 | called by 0x0046cb20 [unmatched] | KillAdvisorHelp (sysstubs.c) -32 |  |
| `0x0046cff0` | 44 | 134 | called by 0x00458ee0 [unmatched] | ProcessInGameHelp (iconui.c) +144 |  |
| `0x0046d2f0` | 15 | 72 | called by 0x0046cff0 [unmatched] | ShowObjectHelp (uimisc.c) -80 |  |
| `0x0046d390` | 2 | 11 | called by PlayReportHint (uimisc3.c) (+1 more) | SetHelpFaceTalking (tinystubs.c) -16 | declared SetHelpFaceState5 in uimisc3.c |
| `0x0046d3b0` | 2 | 11 | unreferenced (swept) | SetHelpFaceTalking (tinystubs.c) +16 | DEAD |
| `0x0046d590` | 56 | 146 | called by 0x00474ed0 [unmatched] | RemoveIconGroup (iconui.c) +112 |  |
| `0x0046d800` | 23 | 77 | called by 0x0046f5e0 [unmatched] | LoadSpriteIcon (iconui.c) +80 | DEAD |
| `0x0046dac0` | 36 | 115 | tail-jumped from ScanMouse (input.c) | ScrollDownInput (fpui3.c) +160 |  |
| `0x0046db40` | 36 | 115 | tail-jumped from ScanMouse (input.c) | AddGBarIcons (fpui.c) -128 |  |
| `0x0046dd10` | 92 | 242 | called by ScrollIconPanel (fpui4.c) | MoveIcons (iconui.c) +64 |  |
| `0x0046de10` | 23 | 56 | unreferenced (swept) | GetIconBounds (uimisc.c) -64 | DEAD |
| `0x0046dfd0` | 37 | 106 | pointer in 0x0046d800 [unmatched] | RenderBoxIcon (render2.c) +96 | DEAD |
| `0x0046ea10` | 43 | 131 | pointer in 0x0046f860 [unmatched] | RenderGBarSprite (render2.c) +64 | DEAD |
| `0x0046ee00` | 74 | 193 | called by 0x00459360 [unmatched] (+1 more) | RenderIcons (fpui.c) -224 |  |

### Group 23 - `0x0046f100`..`0x00476d13`: 35 functions, 1499 instructions (954 live in 18, 545 dead in 17), 4848 bytes

Neighbours: tinystubs.c x9, fpui.c x5, sweep3.c x5. Reached from: group 16 x4, within group x4, screens3.c x4, movie.c x4, group 17 x2, group 27 x2.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x0046f100` | 85 | 254 | called by 0x00458ee0 [unmatched] | RenderIcons2 (fpui.c) +240 |  |
| `0x0046f5e0` | 59 | 163 | unreferenced (swept) | AddGBarClassIcon (fpui.c) -176 | DEAD |
| `0x0046f860` | 16 | 46 | unreferenced (swept) | AddFreePlayIcon (fpui.c) +192 | DEAD |
| `0x0046f890` | 37 | 136 | called by 0x00459520 [unmatched] | AddFreePlayIcon (fpui.c) +240 |  |
| `0x0046f920` | 36 | 116 | called by 0x00459520 [unmatched] | AddFreePlayIcon (fpui.c) +384 |  |
| `0x0046f9a0` | 148 | 411 | unreferenced (swept) | RemoveObjectListIcons (screens3.c) -416 | DEAD |
| `0x00470b00` | 53 | 173 | called by 0x00471170 [unmatched] (+1 more) | InitPopUpInfo (bighelp.c) -176 |  |
| `0x00471170` | 212 | 725 | tail-jumped from UnLoad_PopUpInfo (saveprof.c) | UnLoad_PopUpInfo (saveprof.c) -736 |  |
| `0x00471c10` | 39 | 144 | called by MarkElemAvailable [declared in movie.c] | ResetInfoSelection (tinystubs.c) +32 |  |
| `0x00473560` | 26 | 68 | unreferenced (swept) | ResetHelpKeyCursor (tinystubs.c) -80 | DEAD |
| `0x004735c0` | 6 | 22 | unreferenced (swept) | ResetHelpKeyCursor (tinystubs.c) +16 | DEAD |
| `0x00473640` | 6 | 21 | called by 0x00457a70 [unmatched] | ProcessHelpKeys (tinystubs.c) -32 |  |
| `0x00473680` | 30 | 89 | unreferenced (swept) | ProcessHelpKeys (tinystubs.c) +32 | DEAD |
| `0x004736e0` | 3 | 16 | unreferenced (swept) | ProcessHelpKeys (tinystubs.c) +128 | DEAD |
| `0x004736f0` | 117 | 376 | unreferenced (swept) | ProcessHelpKeys (tinystubs.c) +144 | DEAD |
| `0x00474090` | 8 | 26 | unreferenced (swept) | IsRShiftDown (sysstubs.c) +16 | DEAD |
| `0x00474750` | 44 | 165 | called by AdventureThemeInput (screens3.c) (+3 more) | UnLoad_Interface_Icons (panelui.c) -176 |  |
| `0x00474ed0` | 22 | 100 | called by InitTitleScreen (screens2.c) (+1 more) | BriefIconInput (screens3.c) -112 |  |
| `0x004755b0` | 2 | 3 | unreferenced (swept) | InsertObjectNode (fpui5.c) -16 | DEAD |
| `0x00475f40` | 52 | 149 | called by 0x00457a70 [unmatched] | RestoreCurrentMenu (sysstubs.c) +48 |  |
| `0x00476030` | 8 | 25 | called by 0x00476050 [unmatched] (+1 more) | RenderIconsHook (tinystubs.c) +16 |  |
| `0x00476050` | 11 | 22 | called by ResetLevelGlobals [declared in movie.c] | RenderIconsHook (tinystubs.c) +48 |  |
| `0x00476070` | 23 | 46 | called by 0x0047aea0 [unmatched] (+2 more) | RenderIconsExtra (fpui4.c) -48 |  |
| `0x00476140` | 16 | 54 | called by 0x00468860 [unmatched] | UpdateThemeIconsFromProfile (screens3.c) -64 |  |
| `0x004761e0` | 1 | 1 | unreferenced (swept) | Unload_RAndDCheckBox (sweep2.c) +16 | DEAD |
| `0x00476220` | 2 | 3 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) +16 | DEAD |
| `0x00476230` | 2 | 3 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) +32 | DEAD |
| `0x00476240` | 2 | 3 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) +48 | DEAD |
| `0x00476250` | 53 | 147 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) +64 | DEAD |
| `0x004762f0` | 67 | 211 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) +224 | DEAD |
| `0x00476450` | 3 | 10 | unreferenced (swept) | OpenMovie (movie.c) -16 | DEAD |
| `0x00476680` | 30 | 110 | called by RunMovie (movie.c) | CloseMovie (movie.c) +80 | declared MovieTicks in movie.c |
| `0x00476910` | 197 | 731 | called by RunMovie (movie.c) | RunMovie (movie.c) +544 | declared StartMovieAudio in movie.c |
| `0x00476bf0` | 47 | 148 | called by RunMovie (movie.c) | RunMovie (movie.c) +1280 | declared PrimeMovieAudio in movie.c |
| `0x00476c90` | 36 | 131 | called by RunMovie (movie.c) | PlayMovie (uimisc2.c) -1376 | declared StopMovieAudio in movie.c |

### Group 24 - `0x00476d20`..`0x00478cc1`: 37 functions, 1484 instructions (1330 live in 33, 154 dead in 4), 4421 bytes

Neighbours: simcore.c x33, uimisc2.c x2, coaster.c x2. Reached from: group 26 x47, group 25 x38, within group x35, group 27 x11, movie.c x4, group 23 x1; tables at `0x004bb600`, `0x004bb700`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00476d20` | 344 | 1201 | called by RunMovie (movie.c) (+1 more) | PlayMovie (uimisc2.c) -1232 | declared UpdateMovieAudio in movie.c |
| `0x004771e0` | 4 | 15 | unreferenced (swept) | PlayMovie (uimisc2.c) -16 | DEAD |
| `0x00477440` | 105 | 368 | unreferenced (swept) | FreeMemScratch (coaster.c) +48 | DEAD |
| `0x00477600` | 39 | 118 | unreferenced (swept) | MemScratch_Noop (coaster.c) +16 | DEAD |
| `0x00478110` | 67 | 145 | called by 0x00478280 [unmatched] | RequestRoute (simcore.c) +1344 |  |
| `0x004781b0` | 33 | 61 | called by 0x0047aea0 [unmatched] (+11 more) | RequestRoute (simcore.c) +1504 |  |
| `0x004781f0` | 43 | 134 | called by LoadLevelDatabase (movie.c) | RequestRoute (simcore.c) +1568 | declared ParseKeywordFile in movie.c |
| `0x00478280` | 194 | 561 | called by ParseKeywordFile [declared in movie.c] | RequestRoute (simcore.c) +1712 |  |
| `0x004784c0` | 60 | 265 | called by LoadLevelDatabase (movie.c) | RequestRoute (simcore.c) +2288 | declared ResetLevelGlobals in movie.c |
| `0x004785d0` | 21 | 51 | called by 0x00478a80 [unmatched] (+4 more) | RequestRoute (simcore.c) +2560 |  |
| `0x00478610` | 16 | 50 | called by 0x00478a40 [unmatched] (+4 more) | RequestRoute (simcore.c) +2624 |  |
| `0x00478650` | 18 | 54 | called by 0x00478a40 [unmatched] (+1 more) | RequestRoute (simcore.c) +2688 |  |
| `0x00478690` | 6 | 16 | called by 0x004786c0 [unmatched] (+2 more) | RequestRoute (simcore.c) +2752 |  |
| `0x004786a0` | 7 | 18 | called by 0x0047af80 [unmatched] (+8 more) | RequestRoute (simcore.c) +2768 |  |
| `0x004786c0` | 27 | 56 | called by 0x0047aea0 [unmatched] (+75 more) | RequestRoute (simcore.c) +2800 |  |
| `0x00478700` | 38 | 99 | called by 0x0047a7b0 [unmatched] (+6 more) | RequestRoute (simcore.c) +2864 |  |
| `0x00478770` | 19 | 46 | called by 0x0047a5a0 [unmatched] (+1 more) | RequestRoute (simcore.c) +2976 |  |
| `0x004787a0` | 3 | 8 | called by 0x00478be0 [unmatched] (+2 more) | RequestRoute (simcore.c) +3024 |  |
| `0x004787b0` | 14 | 26 | table at 0x004bb6fc in .data | RequestRoute (simcore.c) +3040 |  |
| `0x004787d0` | 8 | 29 | called by 0x00478930 [unmatched] (+1 more) | RequestRoute (simcore.c) +3072 |  |
| `0x004787f0` | 10 | 38 | called by 0x00478930 [unmatched] | RequestRoute (simcore.c) +3104 |  |
| `0x00478820` | 6 | 21 | unreferenced (swept) | RequestRoute (simcore.c) +3152 | DEAD |
| `0x00478840` | 8 | 36 | table at 0x004bb744 in .data | RequestRoute (simcore.c) +3184 |  |
| `0x00478870` | 8 | 23 | table at 0x004bb70c in .data | RequestRoute (simcore.c) +3232 |  |
| `0x00478890` | 49 | 149 | table at 0x004bb704 in .data | RequestRoute (simcore.c) +3264 |  |
| `0x00478930` | 27 | 73 | table at 0x004bb714 in .data | RequestRoute (simcore.c) +3424 |  |
| `0x00478980` | 18 | 50 | table at 0x004bb71c in .data | RequestRoute (simcore.c) +3504 |  |
| `0x004789c0` | 18 | 50 | table at 0x004bb724 in .data | RequestRoute (simcore.c) +3568 |  |
| `0x00478a00` | 27 | 63 | table at 0x004bb72c in .data | RequestRoute (simcore.c) +3632 |  |
| `0x00478a40` | 27 | 63 | table at 0x004bb734 in .data | RequestRoute (simcore.c) +3696 |  |
| `0x00478a80` | 23 | 56 | table at 0x004bb73c in .data | RequestRoute (simcore.c) +3760 |  |
| `0x00478ac0` | 37 | 89 | table at 0x004bb74c in .data | RequestRoute (simcore.c) +3824 |  |
| `0x00478b20` | 27 | 68 | called by UnlockSidePanelObjects (movie.c) (+2 more) | RequestRoute (simcore.c) +3920 | declared EnsureObjectClassLoaded in movie.c |
| `0x00478b70` | 34 | 73 | table at 0x004bb754 in .data | RequestRoute (simcore.c) +4000 |  |
| `0x00478bc0` | 9 | 24 | table at 0x004bb794 in .data | RequestRoute (simcore.c) +4080 |  |
| `0x00478be0` | 54 | 127 | called by 0x00478bc0 [unmatched] | RequestRoute (simcore.c) +4112 |  |
| `0x00478c60` | 36 | 97 | table at 0x004bb764 in .data | RequestRoute (simcore.c) +4240 |  |

### Group 25 - `0x00478cd0`..`0x00479ca1`: 33 functions, 1484 instructions (1484 live in 33, 0 dead in 0), 3744 bytes

Neighbours: simcore.c x18, movie.c x15. Reached from: pointer tables only; tables at `0x004bb700`, `0x004bb800`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00478cd0` | 34 | 91 | table at 0x004bb76c in .data | RequestRoute (simcore.c) +4352 |  |
| `0x00478d30` | 77 | 226 | table at 0x004bb774 in .data | RequestRoute (simcore.c) +4448 |  |
| `0x00478e20` | 42 | 100 | table at 0x004bb77c in .data | RequestRoute (simcore.c) +4688 |  |
| `0x00478e90` | 42 | 100 | table at 0x004bb78c in .data | RequestRoute (simcore.c) +4800 |  |
| `0x00478f00` | 57 | 146 | table at 0x004bb7ac in .data | RequestRoute (simcore.c) +4912 |  |
| `0x00478fa0` | 66 | 177 | table at 0x004bb79c in .data | RequestRoute (simcore.c) +5072 |  |
| `0x00479060` | 66 | 177 | table at 0x004bb7a4 in .data | RequestRoute (simcore.c) +5264 |  |
| `0x00479120` | 49 | 125 | table at 0x004bb7e4 in .data | RequestRoute (simcore.c) +5456 |  |
| `0x004791a0` | 27 | 67 | table at 0x004bb7ec in .data | RequestRoute (simcore.c) +5584 |  |
| `0x004791f0` | 50 | 119 | table at 0x004bb7f4 in .data | RequestRoute (simcore.c) +5664 |  |
| `0x00479270` | 50 | 131 | table at 0x004bb7fc in .data | RequestRoute (simcore.c) +5792 |  |
| `0x00479300` | 54 | 132 | table at 0x004bb804 in .data | RequestRoute (simcore.c) +5936 |  |
| `0x00479390` | 30 | 77 | table at 0x004bb80c in .data | RequestRoute (simcore.c) +6080 |  |
| `0x004793e0` | 39 | 99 | table at 0x004bb814 in .data | RequestRoute (simcore.c) +6160 |  |
| `0x00479450` | 56 | 127 | table at 0x004bb81c in .data | RequestRoute (simcore.c) +6272 |  |
| `0x004794d0` | 46 | 114 | table at 0x004bb824 in .data | RequestRoute (simcore.c) +6400 |  |
| `0x00479550` | 48 | 111 | table at 0x004bb82c in .data | RequestRoute (simcore.c) +6528 |  |
| `0x004795c0` | 56 | 128 | table at 0x004bb834 in .data | RequestRoute (simcore.c) +6640 |  |
| `0x00479640` | 59 | 136 | table at 0x004bb83c in .data | LoadLevelDatabase (movie.c) -6512 |  |
| `0x004796d0` | 43 | 106 | table at 0x004bb844 in .data | LoadLevelDatabase (movie.c) -6368 |  |
| `0x00479740` | 40 | 97 | table at 0x004bb84c in .data | LoadLevelDatabase (movie.c) -6256 |  |
| `0x004797b0` | 32 | 77 | table at 0x004bb854 in .data | LoadLevelDatabase (movie.c) -6144 |  |
| `0x00479800` | 27 | 70 | table at 0x004bb85c in .data | LoadLevelDatabase (movie.c) -6064 |  |
| `0x00479850` | 40 | 97 | table at 0x004bb864 in .data | LoadLevelDatabase (movie.c) -5984 |  |
| `0x004798c0` | 40 | 97 | table at 0x004bb86c in .data | LoadLevelDatabase (movie.c) -5872 |  |
| `0x00479930` | 27 | 70 | table at 0x004bb874 in .data | LoadLevelDatabase (movie.c) -5760 |  |
| `0x00479980` | 27 | 70 | table at 0x004bb87c in .data | LoadLevelDatabase (movie.c) -5680 |  |
| `0x004799d0` | 27 | 70 | table at 0x004bb884 in .data | LoadLevelDatabase (movie.c) -5600 |  |
| `0x00479a20` | 27 | 70 | table at 0x004bb88c in .data | LoadLevelDatabase (movie.c) -5520 |  |
| `0x00479a70` | 27 | 70 | table at 0x004bb894 in .data | LoadLevelDatabase (movie.c) -5440 |  |
| `0x00479ac0` | 27 | 70 | table at 0x004bb89c in .data | LoadLevelDatabase (movie.c) -5360 |  |
| `0x00479b10` | 115 | 300 | table at 0x004bb8a4 in .data | LoadLevelDatabase (movie.c) -5280 |  |
| `0x00479c40` | 37 | 97 | table at 0x004bb8ac in .data | LoadLevelDatabase (movie.c) -4976 |  |

### Group 26 - `0x00479cb0`..`0x0047abfd`: 33 functions, 1491 instructions (1491 live in 33, 0 dead in 0), 3684 bytes

Neighbours: movie.c x33. Reached from: group 24 x1; tables at `0x004bb700`, `0x004bb800`, `0x004bb900`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00479cb0` | 27 | 70 | table at 0x004bb8b4 in .data | LoadLevelDatabase (movie.c) -4864 |  |
| `0x00479d00` | 37 | 90 | table at 0x004bb8bc in .data | LoadLevelDatabase (movie.c) -4784 |  |
| `0x00479d60` | 27 | 70 | table at 0x004bb8c4 in .data | LoadLevelDatabase (movie.c) -4688 |  |
| `0x00479db0` | 27 | 70 | table at 0x004bb8cc in .data | LoadLevelDatabase (movie.c) -4608 |  |
| `0x00479e00` | 51 | 118 | table at 0x004bb8d4 in .data | LoadLevelDatabase (movie.c) -4528 |  |
| `0x00479e80` | 37 | 90 | table at 0x004bb8dc in .data | LoadLevelDatabase (movie.c) -4400 |  |
| `0x00479ee0` | 27 | 70 | table at 0x004bb8e4 in .data | LoadLevelDatabase (movie.c) -4304 |  |
| `0x00479f30` | 44 | 104 | table at 0x004bb8ec in .data | LoadLevelDatabase (movie.c) -4224 |  |
| `0x00479fa0` | 47 | 113 | table at 0x004bb8f4 in .data | LoadLevelDatabase (movie.c) -4112 |  |
| `0x0047a020` | 55 | 134 | table at 0x004bb9b4 in .data | LoadLevelDatabase (movie.c) -3984 |  |
| `0x0047a0b0` | 55 | 134 | table at 0x004bb9bc in .data | LoadLevelDatabase (movie.c) -3840 |  |
| `0x0047a140` | 58 | 139 | table at 0x004bb9c4 in .data | LoadLevelDatabase (movie.c) -3696 |  |
| `0x0047a1d0` | 108 | 284 | table at 0x004bb9cc in .data | LoadLevelDatabase (movie.c) -3552 |  |
| `0x0047a2f0` | 44 | 106 | table at 0x004bb8fc in .data | LoadLevelDatabase (movie.c) -3264 |  |
| `0x0047a360` | 44 | 106 | table at 0x004bb904 in .data | LoadLevelDatabase (movie.c) -3152 |  |
| `0x0047a3d0` | 44 | 108 | table at 0x004bb90c in .data | LoadLevelDatabase (movie.c) -3040 |  |
| `0x0047a440` | 20 | 57 | table at 0x004bb914 in .data | LoadLevelDatabase (movie.c) -2928 |  |
| `0x0047a480` | 54 | 120 | called by 0x00478be0 [unmatched] | LoadLevelDatabase (movie.c) -2864 |  |
| `0x0047a500` | 28 | 70 | table at 0x004bb924 in .data | LoadLevelDatabase (movie.c) -2736 |  |
| `0x0047a550` | 28 | 70 | table at 0x004bb92c in .data | LoadLevelDatabase (movie.c) -2656 |  |
| `0x0047a5a0` | 71 | 172 | table at 0x004bb934 in .data | LoadLevelDatabase (movie.c) -2576 |  |
| `0x0047a650` | 31 | 80 | table at 0x004bb93c in .data | LoadLevelDatabase (movie.c) -2400 |  |
| `0x0047a6a0` | 31 | 80 | table at 0x004bb944 in .data | LoadLevelDatabase (movie.c) -2320 |  |
| `0x0047a6f0` | 63 | 179 | table at 0x004bb94c in .data | LoadLevelDatabase (movie.c) -2240 |  |
| `0x0047a7b0` | 31 | 80 | table at 0x004bb954 in .data | LoadLevelDatabase (movie.c) -2048 |  |
| `0x0047a800` | 32 | 85 | table at 0x004bb95c in .data | LoadLevelDatabase (movie.c) -1968 |  |
| `0x0047a860` | 23 | 57 | table at 0x004bb964 in .data | LoadLevelDatabase (movie.c) -1872 |  |
| `0x0047a8a0` | 23 | 57 | table at 0x004bb96c in .data | LoadLevelDatabase (movie.c) -1808 |  |
| `0x0047a8e0` | 51 | 125 | table at 0x004bb974 in .data | LoadLevelDatabase (movie.c) -1744 |  |
| `0x0047a960` | 129 | 298 | table at 0x004bb97c in .data | LoadLevelDatabase (movie.c) -1616 |  |
| `0x0047aa90` | 42 | 98 | table at 0x004bb7bc in .data | LoadLevelDatabase (movie.c) -1312 |  |
| `0x0047ab00` | 51 | 125 | table at 0x004bb7c4 in .data | LoadLevelDatabase (movie.c) -1200 |  |
| `0x0047ab80` | 51 | 125 | table at 0x004bb7cc in .data | LoadLevelDatabase (movie.c) -1072 |  |

### Group 27 - `0x0047ac00`..`0x00480a91`: 23 functions, 1473 instructions (1421 live in 21, 52 dead in 2), 4251 bytes

Neighbours: movie.c x9, music.c x4, sysstubs.c x3. Reached from: within group x7, fpui2.c x1, screens2.c x1, group 14 x1, lifecycle.c x1, saveprof.c x1; tables at `0x004bb700`, `0x004bb900`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x0047ac00` | 54 | 121 | table at 0x004bb984 in .data | LoadLevelDatabase (movie.c) -944 |  |
| `0x0047ac80` | 30 | 94 | table at 0x004bb7b4 in .data | LoadLevelDatabase (movie.c) -816 |  |
| `0x0047ace0` | 34 | 91 | table at 0x004bb98c in .data | LoadLevelDatabase (movie.c) -720 |  |
| `0x0047ad40` | 34 | 91 | table at 0x004bb99c in .data | LoadLevelDatabase (movie.c) -624 |  |
| `0x0047ada0` | 33 | 89 | table at 0x004bb9ac in .data | LoadLevelDatabase (movie.c) -528 |  |
| `0x0047ae00` | 64 | 158 | table at 0x004bb7d4 in .data | LoadLevelDatabase (movie.c) -432 |  |
| `0x0047aea0` | 67 | 164 | table at 0x004bb7dc in .data | LoadLevelDatabase (movie.c) -272 |  |
| `0x0047af50` | 16 | 45 | table at 0x004bb9d4 in .data | LoadLevelDatabase (movie.c) -96 |  |
| `0x0047af80` | 16 | 45 | table at 0x004bb9dc in .data | LoadLevelDatabase (movie.c) -48 |  |
| `0x0047b500` | 51 | 156 | unreferenced (swept) | LLIDB_ClearOnLevel (memdb.c) +64 | DEAD |
| `0x0047c7f0` | 316 | 929 | called by InitFreePlayLists (fpui2.c) | LLIDB_UnLoadLLSData (sysmisc.c) +336 |  |
| `0x0047f820` | 3 | 12 | called by InitExitCheckBox (screens2.c) | ResetSaveTimer (screens3.c) +16 |  |
| `0x0047f830` | 2 | 6 | called by 0x0047f880 [unmatched] | ResetSaveTimer (screens3.c) +32 |  |
| `0x0047f840` | 2 | 6 | called by 0x0047f880 [unmatched] | DebugFlush (sysstubs.c) -16 |  |
| `0x0047f860` | 1 | 1 | unreferenced (swept) | DebugFlush (sysstubs.c) +16 | DEAD |
| `0x0047f880` | 262 | 918 | called by 0x0047fd10 [unmatched] | DebugPrintf (sysstubs.c) +16 |  |
| `0x0047fc40` | 95 | 205 | called by 0x0047fd10 [unmatched] | mystrlen (sweep3.c) +32 |  |
| `0x0047fd10` | 84 | 300 | called by 0x00453d10 [unmatched] | mystrlen (sweep3.c) +240 |  |
| `0x004802c0` | 19 | 41 | called by 0x00480330 [unmatched] | LoadMIDIFile (music.c) +192 |  |
| `0x004802f0` | 27 | 59 | called by 0x00480330 [unmatched] | LoadMIDIFile (music.c) +240 |  |
| `0x00480330` | 157 | 444 | called by 0x00480570 [unmatched] | LoadMIDIFile (music.c) +304 |  |
| `0x00480570` | 33 | 83 | pointer in InitMIDIManager (lifecycle.c) | PlayMIDI (music.c) -96 |  |
| `0x004809d0` | 73 | 193 | called by LoadObjectClass (saveprof.c) | AddNewObjectClass (objmap.c) +64 |  |

### Group 28 - `0x00480aa0`..`0x00484087`: 35 functions, 1462 instructions (1371 live in 30, 91 dead in 5), 4038 bytes

Neighbours: tilehelp.c x7, workers.c x4, pathsq.c x3. Reached from: within group x25, group 29 x9, group 17 x2, group 24 x2, group 16 x2, llidb_odf.c x1; tables at `0x004bd300`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00480aa0` | 51 | 149 | called by LLIDB_LoadODFData (llidb_odf.c) | LoadObjectClass (saveprof.c) -160 |  |
| `0x00480b70` | 12 | 51 | pointer in SetStandardCallbacks (sweep3.c) | LoadObjectClass (saveprof.c) +48 |  |
| `0x00481720` | 2 | 6 | called by 0x00459970 [unmatched] | NewPathSquare (pathsq.c) -16 |  |
| `0x004819a0` | 127 | 363 | called by MarkPathSquareReachable [declared in pathmask.c] (+1 more) | PathSquareAdded (pathsq.c) -368 |  |
| `0x00481d70` | 63 | 232 | unreferenced (swept) | RemovePathSquare (pathsq.c) +224 | DEAD |
| `0x00481f00` | 104 | 333 | called by SuggestNextMove (bnvmove.c) | ClearPathSquareVisited (tinystubs.c) +32 |  |
| `0x004826f0` | 13 | 29 | unreferenced (swept) | FindPathLeg (workorder2.c) -32 | DEAD |
| `0x004829c0` | 52 | 128 | called by ResolveEntrancePathSquare (pathmask.c) (+1 more) | ResolveEntrancePathSquare (pathmask.c) -128 | declared MarkPathSquareReachable in pathmask.c |
| `0x00482a80` | 4 | 13 | tail-jumped from sub_4828f0 (screens3.c) | UpdateEntranceTile (objdoor.c) -16 |  |
| `0x00482b10` | 3 | 11 | called by ResetLevelGlobals [declared in movie.c] | GetEntranceTile (tinystubs.c) +16 |  |
| `0x00482cb0` | 29 | 92 | called by 0x00458ee0 [unmatched] | InitBlokeName (lfmisc.c) +80 |  |
| `0x00482d60` | 4 | 16 | called by 0x0047aa90 [unmatched] | GetBlokeMood (simcore2.c) +48 |  |
| `0x00482d70` | 17 | 113 | called by ResetLevelGlobals [declared in movie.c] | GetBlokeMood (simcore2.c) +64 |  |
| `0x00482ec0` | 12 | 35 | called by 0x00459520 [unmatched] (+1 more) | NewBloke (blokeai.c) -48 |  |
| `0x00483090` | 11 | 38 | called by 0x00458b20 [unmatched] | MakeBloke (blokeai.c) -48 |  |
| `0x004830e0` | 1 | 1 | called by 0x00483100 [unmatched] | InitialiseBlokes (workers.c) -16 | DEAD |
| `0x00483100` | 2 | 10 | unreferenced (swept) | InitialiseBlokes (workers.c) +16 | DEAD |
| `0x00483110` | 12 | 32 | unreferenced (swept) | InitialiseBlokes (workers.c) +32 | DEAD |
| `0x00483160` | 21 | 57 | called by 0x00483300 [unmatched] | RenderPeople (renderlist.c) +48 |  |
| `0x004831a0` | 13 | 47 | called by 0x00484090 [unmatched] (+4 more) | SetPathFlag (pathbuild.c) -48 |  |
| `0x00483260` | 54 | 146 | called by 0x00483300 [unmatched] | DoPendingAction (sweep3.c) +32 |  |
| `0x00483300` | 79 | 194 | called by 0x00483ef0 [unmatched] (+3 more) | DoPendingAction (sweep3.c) +192 |  |
| `0x00483580` | 74 | 204 | called by 0x00484520 [unmatched] (+1 more) | HitObstacle (workers.c) +112 |  |
| `0x00483680` | 107 | 282 | called by 0x00484520 [unmatched] (+8 more) | OverNewTile (tilehelp.c) +48 |  |
| `0x00483850` | 21 | 51 | called by 0x00484630 [unmatched] (+1 more) | sub_483830 (bnvpath.c) +32 |  |
| `0x00483890` | 3 | 9 | called by 0x00484790 [unmatched] | sub_483830 (bnvpath.c) +96 |  |
| `0x004838a0` | 8 | 26 | table at 0x004bd34c in .data | sub_483830 (bnvpath.c) +112 |  |
| `0x004838c0` | 10 | 26 | table at 0x004bd350 in .data | DoRndWalkPathTileAction (bnvmove.c) -96 |  |
| `0x004838e0` | 19 | 49 | table at 0x004bd360 in .data | DoRndWalkPathTileAction (bnvmove.c) -64 |  |
| `0x00483b60` | 76 | 182 | called by 0x00483e20 [unmatched] | Handle_RndWalk_TileSpecifics (tilehelp.c) +80 |  |
| `0x00483c20` | 97 | 235 | called by 0x00483e20 [unmatched] (+2 more) | Handle_RndWalk_TileSpecifics (tilehelp.c) +272 |  |
| `0x00483d10` | 54 | 123 | table at 0x004bd35c in .data | Handle_RndWalk_TileSpecifics (tilehelp.c) +512 |  |
| `0x00483d90` | 60 | 142 | table at 0x004bd384 in .data | Handle_RndWalk_TileSpecifics (tilehelp.c) +640 |  |
| `0x00483e20` | 88 | 206 | table at 0x004bd374 in .data | Handle_RndWalk_TileSpecifics (tilehelp.c) +784 |  |
| `0x00483ef0` | 159 | 407 | table at 0x004bd354 in .data | Handle_RndWalk_TileSpecifics (tilehelp.c) +992 |  |

### Group 29 - `0x00484090`..`0x0048d4a7`: 37 functions, 1479 instructions (1236 live in 26, 243 dead in 11), 4004 bytes

Neighbours: pathtile2.c x9, tri3d.c x4, screens2.c x3. Reached from: within group x8, group 16 x6, screens3.c x4, data2.c x2, group 12 x2, bigscreens.c x2; tables at `0x004bd300`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00484090` | 114 | 271 | table at 0x004bd358 in .data | Handle_RndWalk_TileSpecifics (tilehelp.c) +1408 |  |
| `0x004841a0` | 21 | 51 | called by 0x004845d0 [unmatched] (+4 more) | GetTileInDir (pathtile2.c) -1280 |  |
| `0x004841e0` | 18 | 52 | unreferenced (swept) | GetTileInDir (pathtile2.c) -1216 | DEAD |
| `0x00484220` | 112 | 294 | table at 0x004bd364 in .data | GetTileInDir (pathtile2.c) -1152 |  |
| `0x00484350` | 108 | 284 | table at 0x004bd388 in .data | GetTileInDir (pathtile2.c) -848 |  |
| `0x00484470` | 64 | 172 | table at 0x004bd378 in .data | GetTileInDir (pathtile2.c) -560 |  |
| `0x00484520` | 64 | 172 | table at 0x004bd37c in .data | GetTileInDir (pathtile2.c) -384 |  |
| `0x004845d0` | 35 | 94 | table at 0x004bd368 in .data | GetTileInDir (pathtile2.c) -208 |  |
| `0x00484630` | 34 | 98 | table at 0x004bd36c in .data | GetTileInDir (pathtile2.c) -112 |  |
| `0x00484790` | 123 | 321 | table at 0x004bd370 in .data | GetTileInDir (pathtile2.c) +240 |  |
| `0x004848e0` | 11 | 36 | table at 0x004bd380 in .data | Bloke_DoNothing (sweep3.c) -48 |  |
| `0x00484940` | 2 | 3 | unreferenced (swept) | ApplyObjectOrientationToPerson (bnvpath.c) -16 | DEAD |
| `0x004855d0` | 84 | 201 | unreferenced (swept) | PrintSpriteEx (printlist.c) -208 | DEAD |
| `0x00486180` | 1 | 1 | unreferenced (swept) | FindShadedColour (tri3d.c) -16 | DEAD |
| `0x00486490` | 30 | 70 | unreferenced (swept) | SetFlatColour (tri3d.c) -80 | DEAD |
| `0x00486520` | 6 | 19 | unreferenced (swept) | BuildRecipTable (tri3d.c) -32 | DEAD |
| `0x00488730` | 39 | 105 | unreferenced (swept) | SetMousePixel (tri3d.c) +48 | DEAD |
| `0x00489440` | 59 | 139 | called by 0x004895a0 [unmatched] | RenderBox (renderlist.c) +48 |  |
| `0x00489550` | 33 | 74 | called by RES_OpenFileFromVolume (data2.c) | GetMasterVolPtr (audio3.c) +64 |  |
| `0x004895a0` | 147 | 404 | called by RES_OpenVolume (data2.c) (+1 more) | GetMasterVolPtr (audio3.c) +144 |  |
| `0x00489e60` | 58 | 126 | called by 0x00478280 [unmatched] | RES_FileExists (listdel.c) +48 |  |
| `0x00489ee0` | 6 | 21 | called by StartFreePlayPark (uimisc2.c) (+2 more) | MarkObjectTiles (pathmisc.c) -32 | declared sub_489ee0 in uimisc2.c |
| `0x00489f90` | 17 | 55 | called by 0x0044f4a0 [unmatched] | UnmarkObjectTiles (pathmisc2.c) +64 |  |
| `0x00489fd0` | 16 | 51 | called by 0x0046ac50 [unmatched] | AddInstanceToList (sweep4.c) -64 |  |
| `0x0048a040` | 22 | 52 | called by 0x00458b20 [unmatched] | AddInstanceToList (sweep4.c) +48 |  |
| `0x0048a6e0` | 43 | 101 | called by 0x0048a750 [unmatched] (+1 more) | ClipThisRect (util.c) +32 |  |
| `0x0048a750` | 15 | 40 | called by 0x00458be0 [unmatched] (+2 more) | RestoreFreePlaySelections (uimisc.c) -64 |  |
| `0x0048a780` | 1 | 1 | called by ResetTempProfile (saveprof.c) | RestoreFreePlaySelections (uimisc.c) -16 |  |
| `0x0048a800` | 20 | 51 | called by ProfileSlotInput (screens3.c) (+1 more) | FreePlayItemUpdate (fpui5.c) -64 |  |
| `0x0048b690` | 9 | 33 | unreferenced (swept) | FreePlayInit_48b6c0 (tinystubs.c) -48 | DEAD |
| `0x0048c5e0` | 23 | 69 | called by PrintProfileDetails (bigscreens.c) (+1 more) | EnterNewProfileCheckBoxIcons (profiles.c) -112 |  |
| `0x0048c860` | 80 | 320 | called by DeleteIconInput (screens3.c) | InitProfileCheckBoxIcons (screens2.c) +320 |  |
| `0x0048cc90` | 18 | 59 | unreferenced (swept) | DeleteIconInput (screens3.c) +96 | DEAD |
| `0x0048ccd0` | 18 | 59 | unreferenced (swept) | LightUpthisDeleteIcon (screens2.c) -128 | DEAD |
| `0x0048cd10` | 18 | 59 | unreferenced (swept) | LightUpthisDeleteIcon (screens2.c) -64 | DEAD |
| `0x0048d470` | 5 | 23 | called by InitSavedGameScreen (bigscreens.c) | ProfileCloseInput (screens3.c) +32 |  |
| `0x0048d490` | 5 | 23 | called by SaveGameOkInput (screens3.c) (+1 more) | InitSavedGameScreen (bigscreens.c) -32 |  |

### Group 30 - `0x0048e0c0`..`0x00498112`: 49 functions, 1476 instructions (1029 live in 34, 447 dead in 15), 4448 bytes

Neighbours: screens3.c x10, screens2.c x5, tinystubs.c x4. Reached from: screens3.c x9, within group x9, screens2.c x7, group 29 x7, movie.c x5, group 31 x4.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x0048e0c0` | 52 | 157 | called by LoadSavedGamesList (profiles.c) | DeleteSavedGameList (listdel.c) -160 |  |
| `0x0048e3d0` | 28 | 71 | called by SaveEmptySlotInput (screens3.c) | SaveSlotInput (screens3.c) -208 |  |
| `0x0048e420` | 12 | 40 | called by SaveGameOkInput (screens3.c) (+2 more) | SaveSlotInput (screens3.c) -128 |  |
| `0x0048e450` | 15 | 65 | pointer in 0x0048c860 [unmatched] | SaveSlotInput (screens3.c) -80 |  |
| `0x0048eac0` | 12 | 37 | called by VolMarkerInput (screens3.c) | InitOptionScreen (screens2.c) -160 |  |
| `0x0048eaf0` | 12 | 35 | called by VolDownInput (screens3.c) (+2 more) | InitOptionScreen (screens2.c) -112 |  |
| `0x0048eb20` | 5 | 23 | called by InitOptionScreen (screens2.c) (+1 more) | InitOptionScreen (screens2.c) -64 |  |
| `0x0048eb40` | 5 | 23 | called by ExitCloseInput (screens3.c) (+1 more) | InitOptionScreen (screens2.c) -32 |  |
| `0x0048ef10` | 13 | 45 | unreferenced (swept) | ExitOkInput (screens3.c) -128 | DEAD |
| `0x0048ef40` | 18 | 78 | unreferenced (swept) | ExitOkInput (screens3.c) -80 | DEAD |
| `0x0048f4f0` | 17 | 85 | unreferenced (swept) | ExitCloseInput (screens3.c) +64 | DEAD |
| `0x0048f5d0` | 11 | 30 | unreferenced (swept) | VolUpInput (screens3.c) -32 | DEAD |
| `0x0048f9f0` | 18 | 70 | called by TitleMovieInput (screens3.c) | RestoreFrontEndState (movie.c) -80 |  |
| `0x0048fbd0` | 12 | 40 | unreferenced (swept) | ProcessScreenPopup (tinystubs.c) -48 | DEAD |
| `0x0048fc30` | 5 | 14 | called by InitTitleScreen (screens2.c) | InitTitleScreen (screens2.c) -16 |  |
| `0x0048fe70` | 16 | 59 | unreferenced (swept) | TitleNewInput (screens3.c) -64 | DEAD |
| `0x00490680` | 86 | 184 | called by LoadHelpTextFor (movie.c) (+1 more) | SetReportMovie (uimisc2.c) +112 | declared LoadTextFileLines in movie.c |
| `0x00490740` | 16 | 48 | called by LoadHelpTextFor (movie.c) | LoadHelpTextFor (movie.c) -96 | declared SetHelpTextPrefix in movie.c |
| `0x00490770` | 16 | 48 | called by LoadHintTextFor [declared in movie.c] | LoadHelpTextFor (movie.c) -48 |  |
| `0x00490800` | 21 | 74 | called by SetInfoPanelText (movie.c) | FreeHelpTextBuffer (tinystubs.c) -80 | declared LoadHintTextFor in movie.c |
| `0x00491540` | 5 | 14 | called by PrintProfileDetails (bigscreens.c) | UpDateCurrentSaveSlotInfo (profiles.c) -16 |  |
| `0x004917c0` | 108 | 328 | called by 0x0048cd10 [unmatched] (+2 more) | UpDateCurrentProfile (profiles.c) +320 | DEAD |
| `0x004919a0` | 11 | 30 | called by 0x004917c0 [unmatched] (+3 more) | AddNodeToProfileList (profiles.c) -32 | DEAD |
| `0x00491b80` | 16 | 65 | unreferenced (swept) | DeleteProfileList (listdel.c) +48 | DEAD |
| `0x00491e40` | 121 | 336 | called by EnterSaveGameDetails (screens2.c) (+1 more) | NewPrintCent (text.c) +224 |  |
| `0x00491f90` | 28 | 70 | unreferenced (swept) | NewProfileCloseInput (screens3.c) -272 | DEAD |
| `0x00491fe0` | 57 | 179 | unreferenced (swept) | NewProfileCloseInput (screens3.c) -192 | DEAD |
| `0x004921c0` | 155 | 438 | called by CreateSampleFromWAV (data2.c) | InitSoundSampleSystem (audio4.c) +144 |  |
| `0x00492980` | 2 | 11 | called by LoadLevelDatabase (movie.c) | SetSampleVolume (audio3.c) -32 | declared SetSfxPaused in movie.c |
| `0x00492990` | 2 | 11 | called by LoadLevelDatabase (movie.c) | SetSampleVolume (audio3.c) -16 | declared ClearSfxPaused in movie.c |
| `0x00492c60` | 7 | 22 | called by 0x00459520 [unmatched] | KillSoundSampleSystem (lifecycle.c) +64 |  |
| `0x00492c80` | 7 | 22 | called by 0x00459520 [unmatched] | SetThemeInTransition (tinystubs.c) -32 |  |
| `0x00492da0` | 5 | 13 | called by PlayMovie (uimisc2.c) | MusicThread (musicthread.c) -16 | declared RestartMusic in uimisc2.c |
| `0x00495f00` | 73 | 258 | unreferenced (swept) | LoadMusicSegment (music.c) +208 | DEAD |
| `0x00496010` | 39 | 122 | unreferenced (swept) | GetMusicBand (music.c) -128 | DEAD |
| `0x00496760` | 33 | 78 | called by 0x004969d0 [unmatched] | RefreshSampleVolumes (audio5.c) -80 |  |
| `0x004967f0` | 73 | 213 | called by 0x004969d0 [unmatched] | RefreshSampleVolumes (audio5.c) +64 |  |
| `0x004968d0` | 32 | 79 | called by 0x004969d0 [unmatched] | RefreshSampleVolumes (audio5.c) +288 |  |
| `0x00496920` | 64 | 167 | tail-jumped from 0x004969d0 [unmatched] | UnSourcePlayableSample (audio2.c) -208 |  |
| `0x004969d0` | 4 | 20 | called by 0x00469c80 [unmatched] (+1 more) | UnSourcePlayableSample (audio2.c) -32 |  |
| `0x00496e60` | 53 | 182 | called by 0x00469f80 [unmatched] | Kill_FXList (audiomisc.c) +48 |  |
| `0x004975a0` | 7 | 16 | unreferenced (swept) | NewSprite (sysstubs.c) +32 | DEAD |
| `0x004975b0` | 38 | 93 | called by UnreferenceSprite (spritemisc.c) | NewSprite (sysstubs.c) +48 |  |
| `0x00497e40` | 21 | 53 | unreferenced (swept) | ShowLayer (blokelist.c) +48 | DEAD |
| `0x00497f60` | 15 | 39 | called by 0x00498000 [unmatched] | TellAllLayersToStopAnimating (sprite2.c) +64 |  |
| `0x00497f90` | 7 | 23 | called by 0x00498150 [unmatched] | TellAllLayersToStopAnimating (sprite2.c) +112 |  |
| `0x00497fb0` | 25 | 79 | called by 0x00498150 [unmatched] | TellAllLayersToStopAnimating (sprite2.c) +144 |  |
| `0x00498000` | 73 | 243 | called by 0x00498250 [unmatched] (+1 more) | TellAllLayersToStopAnimating (sprite2.c) +224 |  |
| `0x00498100` | 5 | 18 | called by 0x00498000 [unmatched] | RewindNarrationSource (tinystubs.c) -32 |  |

### Group 31 - `0x00498150`..`0x0049d048`: 24 functions, 1121 instructions (686 live in 14, 435 dead in 10), 3021 bytes

Neighbours: tinystubs.c x6, text.c x5, sysstubs.c x3. Reached from: within group x8, group 16 x4, screens3.c x3, movie.c x1, group 11 x1, group 27 x1; tables at `0x004b8300`.

| address | insns | bytes | reached by | nearest matched | notes |
| --- | ---: | ---: | --- | --- | --- |
| `0x00498150` | 49 | 142 | called by 0x00498250 [unmatched] | RewindNarrationSource (tinystubs.c) +48 |  |
| `0x004981e0` | 15 | 39 | called by 0x00498250 [unmatched] | RewindNarrationSource (tinystubs.c) +192 |  |
| `0x00498210` | 7 | 23 | called by ReadDecodedNarration [declared in movie.c] | RewindNarrationSource (tinystubs.c) +240 |  |
| `0x00498230` | 5 | 19 | called by ReadDecodedNarration [declared in movie.c] | RewindNarrationSource (tinystubs.c) +272 |  |
| `0x00498250` | 102 | 336 | called by 0x00498b40 [unmatched] (+1 more) | RewindNarrationSource (tinystubs.c) +304 |  |
| `0x004983a0` | 46 | 128 | called by RewindNarrationBuffer (movie.c) (+1 more) | ReadNarrationWaveHeader (audio5.c) -128 | declared ReadDecodedNarration in movie.c |
| `0x00498b40` | 142 | 426 | called by VolMarkerInput (screens3.c) (+4 more) | ResumeCurrentTrack (uimisc3.c) +64 |  |
| `0x00498d00` | 192 | 579 | called by 0x0047f880 [unmatched] | IsNarrationPlaying (tinystubs.c) +16 |  |
| `0x00498f80` | 46 | 111 | called by 0x00498d00 [unmatched] | GetString (text.c) +48 |  |
| `0x00499040` | 61 | 122 | unreferenced (swept) | DeleteStrings (text.c) +80 | DEAD |
| `0x004990c0` | 45 | 94 | unreferenced (swept) | DeleteStrings (text.c) +208 | DEAD |
| `0x00499120` | 46 | 98 | unreferenced (swept) | DeleteStrings (text.c) +304 | DEAD |
| `0x00499190` | 76 | 175 | unreferenced (swept) | DeleteStrings (text.c) +416 | DEAD |
| `0x00499240` | 80 | 186 | unreferenced (swept) | FreezeGameClock (sysstubs.c) -320 | DEAD |
| `0x00499300` | 23 | 53 | called by 0x00478280 [unmatched] | FreezeGameClock (sysstubs.c) -128 |  |
| `0x00499340` | 23 | 53 | unreferenced (swept) | FreezeGameClock (sysstubs.c) -64 | DEAD |
| `0x00499410` | 7 | 31 | called by StartFreePlayPark (uimisc2.c) (+2 more) | GetGameTimer (util.c) -32 | declared ResetGameClock in uimisc2.c |
| `0x00499490` | 42 | 110 | unreferenced (swept) | GetBlink (util.c) +16 | DEAD |
| `0x00499ca0` | 36 | 82 | unreferenced (swept) | FindFreeGardener (workorder2.c) +96 | DEAD |
| `0x0049a4a0` | 3 | 11 | table at 0x004b83c8 in .data | Mechanic_Idle (blokemisc.c) -16 |  |
| `0x0049a4d0` | 3 | 11 | table at 0x004b83cc in .data | Gardener_Build (workers2.c) -16 |  |
| `0x0049c0f0` | 9 | 19 | unreferenced (swept) | SaveGardeners (savechunks.c) -80 | DEAD |
| `0x0049c110` | 17 | 37 | unreferenced (swept) | SaveGardeners (savechunks.c) -48 | DEAD |
| `0x0049cfc0` | 46 | 136 | called by 0x00458b20 [unmatched] | MarkWorkersOnMap (workorder4.c) +192 |  |


## Appendix B — every unmatched function, largest first

867 functions. `group` is the candidate scope above; `notes` carries the declared name where the tree already has one, and the `DEAD`/`LONG`/`SEH`/`WEAK` flags explained in the method section.

| insns | bytes | address | group | reached by | nearest matched | notes |
| ---: | ---: | --- | ---: | --- | --- | --- |
| 8085 | 34662 | `0x004453a0` | 11 | called by 0x0044db90 [unmatched] | LoadReport (uimisc.c) +4416 | LONG |
| 751 | 2889 | `0x00457a70` | 16 | called by 0x00458ee0 [unmatched] | LoadCurrency (sysstubs.c) +304 |  |
| 699 | 1977 | `0x0044f610` | 12 | table at 0x004b8380 in .data | RemoveBlokeFromList (blokelist.c) +416 |  |
| 620 | 1760 | `0x00451740` | 14 | called by SaveCertificateBitmap (render5.c) | RES_EnsureMounted (sysmisc.c) +352 |  |
| 521 | 1613 | `0x004227c0` | 5 | unreferenced (swept) | LoadCoasterModelSet (schoolcar4.c) +256 | DEAD |
| 395 | 1187 | `0x0043ea30` | 8 | called by 0x0043f0b0 [unmatched] (+1 more) | PlaneRide_Activate (mechrides.c) +1568 | DEAD |
| 345 | 1256 | `0x00453da0` | 14 | called by 0x00453d10 [unmatched] | DBError (sysstubs.c) +192 |  |
| 344 | 1201 | `0x00476d20` | 24 | called by RunMovie (movie.c) (+1 more) | PlayMovie (uimisc2.c) -1232 | declared UpdateMovieAudio in movie.c |
| 337 | 928 | `0x0044fe80` | 13 | table at 0x004b839c in .data | DoHighLevelAI (blokeai.c) -1616 |  |
| 328 | 936 | `0x0044ed70` | 12 | table at 0x004b8374 in .data | PopLongTermAction (sweep1.c) +416 |  |
| 325 | 1011 | `0x00466d80` | 18 | called by SoftBlitRLEPlain (softblit2.c) | SoftBlitRLEPlain (softblit2.c) +1552 |  |
| 316 | 929 | `0x0047c7f0` | 27 | called by InitFreePlayLists (fpui2.c) | LLIDB_UnLoadLLSData (sysmisc.c) +336 |  |
| 313 | 944 | `0x0043f0b0` | 8 | unreferenced (swept) | LoadPos (loaders.c) -1456 | DEAD |
| 312 | 992 | `0x00468410` | 19 | called by SoftPrint_XBltFast (bigrender.c) | ClearScriptStateBytes (sysstubs.c) -1072 |  |
| 303 | 965 | `0x00468040` | 19 | called by SoftBlitRLE (render3.c) | ClearScriptStateBytes (sysstubs.c) -2048 |  |
| 291 | 885 | `0x0045ade0` | 17 | unreferenced (swept) | GetTileCentre (tilehelp.c) +128 | DEAD |
| 289 | 1119 | `0x00458ee0` | 16 | called by 0x00458c00 [unmatched] | SetMapReady (sysstubs.c) +816 |  |
| 275 | 762 | `0x00455fc0` | 15 | called by 0x00458ee0 [unmatched] | ExpireCachedText (render5.c) +80 |  |
| 271 | 833 | `0x004677b0` | 19 | called by SoftBlitRLEPlain (softblit2.c) | SoftBlitRLEPlain (softblit2.c) +4160 |  |
| 262 | 918 | `0x0047f880` | 27 | called by 0x0047fd10 [unmatched] | DebugPrintf (sysstubs.c) +16 |  |
| 254 | 771 | `0x00428860` | 7 | table at 0x004b5f50 in .data | InitTrackDrawModes (coaster4.c) +272 |  |
| 229 | 752 | `0x00442980` | 9 | called by InitMan (data2.c) | LookupTextureName (data3.c) +144 |  |
| 212 | 725 | `0x00471170` | 23 | tail-jumped from UnLoad_PopUpInfo (saveprof.c) | UnLoad_PopUpInfo (saveprof.c) -736 |  |
| 202 | 633 | `0x0041ff80` | 4 | table at 0x004b565c in .data | CoasterShades_InitClamp (schoolcar8.c) +592 |  |
| 202 | 611 | `0x00467180` | 18 | called by SoftBlitRLEPlain (softblit2.c) | SoftBlitRLEPlain (softblit2.c) +2576 |  |
| 201 | 472 | `0x0044f180` | 12 | called by 0x0044f610 [unmatched] | IsObjectRunning (sysmisc3.c) -480 |  |
| 197 | 588 | `0x004673f0` | 18 | called by SoftBlitRLEPlain (softblit2.c) | SoftBlitRLEPlain (softblit2.c) +3200 |  |
| 197 | 731 | `0x00476910` | 23 | called by RunMovie (movie.c) | RunMovie (movie.c) +544 | declared StartMovieAudio in movie.c |
| 195 | 606 | `0x0045fca0` | 17 | called by RenderCursor (bigrender.c) | CalcBasicObjectCursor (objmap.c) +544 |  |
| 194 | 561 | `0x00478280` | 24 | called by ParseKeywordFile [declared in movie.c] | RequestRoute (simcore.c) +1712 |  |
| 192 | 579 | `0x00498d00` | 31 | called by 0x0047f880 [unmatched] | IsNarrationPlaying (tinystubs.c) +16 |  |
| 182 | 579 | `0x00420c40` | 5 | called by Coaster3D_DrawModel (coaster9.c) | Coaster3D_DrawModel (coaster9.c) -592 | declared CoasterModel_DrawPass3 in coaster9.c |
| 180 | 526 | `0x00467b00` | 19 | called by SoftBlitRLEPlain (softblit2.c) | ClearScriptStateBytes (sysstubs.c) -3392 |  |
| 179 | 593 | `0x0041f050` | 3 | called by 0x0041f2b0 [unmatched] | SetSpanClip (coastermath.c) +304 |  |
| 177 | 577 | `0x00469c80` | 20 | table at 0x004b9d64 in .data | UpdateGoalHelpText (softblit.c) +2176 |  |
| 173 | 464 | `0x0043eee0` | 8 | unreferenced (swept) | LoadPos (loaders.c) -1920 | DEAD |
| 170 | 453 | `0x004097a0` | 1 | called by LFTrack_Add (logflume.c) | LFTrack_MaskIsLegal (logflume2.c) +544 |  |
| 168 | 528 | `0x00404630` | 1 | called by Copters_StepMachine (ridemachine.c) | Copters_Place (mechrides.c) +48 | declared Copters_UpdateCarRider in ridemachine.c |
| 167 | 489 | `0x00467d10` | 19 | called by SoftBlitRLEPlain (softblit2.c) | ClearScriptStateBytes (sysstubs.c) -2864 |  |
| 165 | 530 | `0x00420a20` | 5 | called by Coaster3D_DrawModel (coaster9.c) | FindCoasterColour (coaster8.c) +592 | declared CoasterModel_DrawPass2 in coaster9.c |
| 165 | 430 | `0x0046a750` | 21 | table at 0x004b9dd8 in .data | ScriptEventDue (uimisc.c) -2736 |  |
| 162 | 505 | `0x0041fd80` | 4 | table at 0x004b5658 in .data | CoasterShades_InitClamp (schoolcar8.c) +80 |  |
| 161 | 515 | `0x00420810` | 4 | called by Coaster3D_DrawModel (coaster9.c) | FindCoasterColour (coaster8.c) +64 | declared CoasterModel_DrawPass1 in coaster9.c |
| 160 | 458 | `0x0045fad0` | 17 | called by RenderCursor (bigrender.c) | CalcBasicObjectCursor (objmap.c) +80 |  |
| 159 | 407 | `0x00483ef0` | 28 | table at 0x004bd354 in .data | Handle_RndWalk_TileSpecifics (tilehelp.c) +992 |  |
| 157 | 444 | `0x00480330` | 27 | called by 0x00480570 [unmatched] | LoadMIDIFile (music.c) +304 |  |
| 155 | 438 | `0x004921c0` | 30 | called by CreateSampleFromWAV (data2.c) | InitSoundSampleSystem (audio4.c) +144 |  |
| 153 | 708 | `0x00458c00` | 16 | called by 0x00459520 [unmatched] | SetMapReady (sysstubs.c) +80 |  |
| 151 | 520 | `0x0040d6f0` | 2 | called by LFDrop_Update (logflume.c) (+7 more) | LFPiece_TickCommon (logflume2.c) +832 |  |
| 148 | 411 | `0x0046f9a0` | 23 | unreferenced (swept) | RemoveObjectListIcons (screens3.c) -416 | DEAD |
| 147 | 404 | `0x004895a0` | 29 | called by RES_OpenVolume (data2.c) (+1 more) | GetMasterVolPtr (audio3.c) +144 |  |
| 142 | 426 | `0x00498b40` | 31 | called by VolMarkerInput (screens3.c) (+4 more) | ResumeCurrentTrack (uimisc3.c) +64 |  |
| 136 | 399 | `0x0041fba0` | 4 | table at 0x004b564c in .data | CoasterShades_InitClamp (schoolcar8.c) -400 |  |
| 134 | 393 | `0x0041fa10` | 4 | unreferenced (swept) | TrackCursor_AdvanceGeometry (schoolcar8.c) +448 | DEAD |
| 131 | 355 | `0x0043f4f0` | 9 | unreferenced (swept) | LoadPos (loaders.c) -368 | DEAD |
| 130 | 339 | `0x0041c940` | 3 | called by BoatingSchool_BuildRoute (bswater2.c) (+1 more) | BoatingSchool_BuildRoute (bswater2.c) +128 |  |
| 129 | 298 | `0x0047a960` | 26 | table at 0x004bb97c in .data | LoadLevelDatabase (movie.c) -1616 |  |
| 128 | 460 | `0x0040d520` | 2 | called by LFTrack_Update (logflume.c) (+1 more) | LFPiece_TickCommon (logflume2.c) +368 |  |
| 127 | 363 | `0x004819a0` | 28 | called by MarkPathSquareReachable [declared in pathmask.c] (+1 more) | PathSquareAdded (pathsq.c) -368 |  |
| 123 | 381 | `0x00443bd0` | 9 | called by 0x00444090 [unmatched] | RenderAdvisorIcon (screens3.c) -608 |  |
| 123 | 321 | `0x00484790` | 29 | table at 0x004bd370 in .data | GetTileInDir (pathtile2.c) +240 |  |
| 122 | 342 | `0x00455a50` | 15 | pointer in 0x00455bb0 [unmatched] | HTBubbleHelp (fpui2.c) +656 |  |
| 122 | 358 | `0x00467640` | 18 | called by SoftBlitRLEPlain (softblit2.c) | SoftBlitRLEPlain (softblit2.c) +3792 |  |
| 121 | 356 | `0x0044f4a0` | 12 | called by 0x0044f610 [unmatched] (+2 more) | RemoveBlokeFromList (blokelist.c) +48 |  |
| 121 | 489 | `0x00459520` | 17 | called by 0x0047f880 [unmatched] | RunLevelEndSequence (uimisc2.c) -496 |  |
| 121 | 336 | `0x00491e40` | 30 | called by EnterSaveGameDetails (screens2.c) (+1 more) | NewPrintCent (text.c) +224 |  |
| 120 | 335 | `0x00455220` | 15 | called by 0x0043ea30 [unmatched] | BubbleHelp (bighelp.c) -336 | DEAD |
| 119 | 335 | `0x00434860` | 8 | unreferenced (swept) | JcMonkeyFish_GetDrawDesc (screencb2.c) +288 | DEAD |
| 117 | 376 | `0x004736f0` | 23 | unreferenced (swept) | ProcessHelpKeys (tinystubs.c) +144 | DEAD |
| 116 | 384 | `0x0043b810` | 8 | called by SpaceTower_StepMachine (ridemachine.c) | SpaceTower_GetInterfaces (interfaces.c) +144 | declared SpaceTower_UpdateRiders in ridemachine.c |
| 116 | 344 | `0x00459970` | 17 | called by 0x0046ad30 [unmatched] | ResetBuildTimer (mapbuild.c) +16 |  |
| 115 | 300 | `0x00479b10` | 25 | table at 0x004bb8a4 in .data | LoadLevelDatabase (movie.c) -5280 |  |
| 114 | 349 | `0x00420fd0` | 5 | unreferenced (swept) | Coaster3D_DrawModel (coaster9.c) +320 | DEAD |
| 114 | 311 | `0x00467f00` | 19 | called by SoftBlitRLEPlain (softblit2.c) | ClearScriptStateBytes (sysstubs.c) -2368 |  |
| 114 | 271 | `0x00484090` | 29 | table at 0x004bd358 in .data | Handle_RndWalk_TileSpecifics (tilehelp.c) +1408 |  |
| 113 | 333 | `0x004629e0` | 17 | called by 0x0046cb20 [unmatched] | AddOvSav (loaders.c) -336 |  |
| 113 | 331 | `0x00465850` | 18 | called by RunMovie (movie.c) | SoftPrint_XBltFast (bigrender.c) -496 | declared BlitDIBToScreen in movie.c |
| 112 | 353 | `0x00421130` | 5 | unreferenced (swept) | PhysVec_Add (schoolcar8.c) -368 | DEAD |
| 112 | 294 | `0x00484220` | 29 | table at 0x004bd364 in .data | GetTileInDir (pathtile2.c) -1152 |  |
| 110 | 380 | `0x00454380` | 14 | called by 0x004542e0 [unmatched] | LoadBubbleHelpGFX (text.c) -1424 |  |
| 108 | 284 | `0x0047a1d0` | 26 | table at 0x004bb9cc in .data | LoadLevelDatabase (movie.c) -3552 |  |
| 108 | 284 | `0x00484350` | 29 | table at 0x004bd388 in .data | GetTileInDir (pathtile2.c) -848 |  |
| 108 | 328 | `0x004917c0` | 30 | called by 0x0048cd10 [unmatched] (+2 more) | UpDateCurrentProfile (profiles.c) +320 | DEAD |
| 107 | 282 | `0x00483680` | 28 | called by 0x00484520 [unmatched] (+8 more) | OverNewTile (tilehelp.c) +48 |  |
| 106 | 240 | `0x0040da10` | 2 | called by LFTrack_Remove (logflume.c) (+1 more) | LFTrack_Tick (logflume.c) -416 |  |
| 106 | 307 | `0x0041f8d0` | 4 | table at 0x004b5648 in .data | TrackCursor_AdvanceGeometry (schoolcar8.c) +128 |  |
| 105 | 368 | `0x00477440` | 24 | unreferenced (swept) | FreeMemScratch (coaster.c) +48 | DEAD |
| 104 | 333 | `0x00481f00` | 28 | called by SuggestNextMove (bnvmove.c) | ClearPathSquareVisited (tinystubs.c) +32 |  |
| 103 | 205 | `0x004283c0` | 6 | called by 0x004286e0 [unmatched] | TrackHP_Add (castleobj.c) +192 |  |
| 102 | 376 | `0x00459360` | 17 | called by 0x00458c00 [unmatched] | RunLevelEndSequence (uimisc2.c) -944 |  |
| 102 | 336 | `0x00466080` | 18 | table at 0x004b9ca4 in .data | FlipPrimary (sysmisc.c) -336 |  |
| 102 | 336 | `0x00498250` | 31 | called by 0x00498b40 [unmatched] (+1 more) | RewindNarrationSource (tinystubs.c) +304 |  |
| 101 | 244 | `0x0043e930` | 8 | called by 0x0043ea30 [unmatched] | PlaneRide_Activate (mechrides.c) +1312 | DEAD |
| 98 | 290 | `0x00428b80` | 7 | unreferenced (swept) | CoasterSceneInit (schoolcar.c) +16 | DEAD |
| 98 | 246 | `0x00444a70` | 10 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2064 |  |
| 98 | 311 | `0x004632b0` | 18 | called by 0x00458ee0 [unmatched] | RateBlokeOnLeaving (workers.c) -320 |  |
| 97 | 348 | `0x004545a0` | 14 | called by 0x00453da0 [unmatched] | LoadBubbleHelpGFX (text.c) -880 |  |
| 97 | 235 | `0x00483c20` | 28 | called by 0x00483e20 [unmatched] (+2 more) | Handle_RndWalk_TileSpecifics (tilehelp.c) +272 |  |
| 95 | 205 | `0x0047fc40` | 27 | called by 0x0047fd10 [unmatched] | mystrlen (sweep3.c) +32 |  |
| 94 | 362 | `0x004349b0` | 8 | unreferenced (swept) | JcDeco_Remove (junglecruise.c) -400 | DEAD |
| 93 | 250 | `0x0040b290` | 1 | called by LFHoldUp_Interact (logflume.c) (+2 more) | LFBoat_IsOnPiece (posstep.c) +128 |  |
| 93 | 297 | `0x00429cf0` | 7 | pointer in 0x00429f30 [unmatched] (+1 more) | TrackCurve_EvaluateOffset (coaster9.c) +320 |  |
| 92 | 292 | `0x0042a1b0` | 7 | called by Coaster_StationDerivative (coaster7.c) | CoasterFxPoolInit (schoolcar.c) -304 |  |
| 92 | 266 | `0x0044ebf0` | 12 | table at 0x004b8370 in .data | PopLongTermAction (sweep1.c) +32 |  |
| 92 | 302 | `0x00454a10` | 15 | called by 0x00459520 [unmatched] | LoadBubbleHelpGFX (text.c) +256 |  |
| 92 | 242 | `0x0046dd10` | 22 | called by ScrollIconPanel (fpui4.c) | MoveIcons (iconui.c) +64 |  |
| 91 | 376 | `0x00445190` | 10 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +3888 |  |
| 91 | 264 | `0x004689f0` | 20 | called by ResetLevelGlobals [declared in movie.c] (+1 more) | FreeScriptEventList (tinystubs.c) +128 |  |
| 90 | 302 | `0x00429560` | 7 | called by TrackJoinPieces (coaster5.c) | DrawTrackPiece3D (coaster.c) +112 |  |
| 89 | 221 | `0x0045e960` | 17 | unreferenced (swept) | GetObjectDoorOffset (objdoor.c) -224 | DEAD |
| 88 | 260 | `0x00451280` | 13 | called by 0x00451210 [unmatched] | RES_FindVolumeOnResPath (sysmisc2.c) +416 | DEAD |
| 88 | 206 | `0x00483e20` | 28 | table at 0x004bd374 in .data | Handle_RndWalk_TileSpecifics (tilehelp.c) +784 |  |
| 87 | 232 | `0x00424050` | 6 | called by RenderFullMap (renderview.c) | GetCastleRec (castleobj.c) -240 |  |
| 87 | 161 | `0x004503a0` | 13 | called by 0x00450450 [unmatched] | DoHighLevelAI (blokeai.c) -304 |  |
| 87 | 221 | `0x0046a5b0` | 21 | table at 0x004b9dd0 in .data | ScriptEventDue (uimisc.c) -3152 |  |
| 86 | 264 | `0x00423f40` | 6 | called by 0x00424050 [unmatched] | RemoveAllTrackNodes (coaster.c) +128 |  |
| 86 | 247 | `0x00469980` | 20 | called by 0x00469a80 [unmatched] | UpdateGoalHelpText (softblit.c) +1408 |  |
| 86 | 184 | `0x00490680` | 30 | called by LoadHelpTextFor (movie.c) (+1 more) | SetReportMovie (uimisc2.c) +112 | declared LoadTextFileLines in movie.c |
| 85 | 212 | `0x0041f3e0` | 3 | called by 0x0041f4e0 [unmatched] | Span_SetClip (coaster7.c) +96 |  |
| 85 | 242 | `0x00457970` | 15 | called by 0x00457a70 [unmatched] | LoadCurrency (sysstubs.c) +48 |  |
| 85 | 254 | `0x0046f100` | 23 | called by 0x00458ee0 [unmatched] | RenderIcons2 (fpui.c) +240 |  |
| 84 | 300 | `0x0047fd10` | 27 | called by 0x00453d10 [unmatched] | mystrlen (sweep3.c) +240 |  |
| 84 | 201 | `0x004855d0` | 29 | unreferenced (swept) | PrintSpriteEx (printlist.c) -208 | DEAD |
| 83 | 258 | `0x0040d900` | 2 | called by LFDrop_Add (logflume.c) (+7 more) | LFTrack_Tick (logflume.c) -688 |  |
| 83 | 256 | `0x00445000` | 10 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +3488 |  |
| 82 | 255 | `0x0042a680` | 7 | called by RouteNode_LinkPending [declared in coaster9.c] | TrackCursor_Evaluate (coastertiny.c) +64 |  |
| 81 | 258 | `0x00420200` | 4 | called by 0x0042a1b0 [unmatched] | Phys_Step (schoolcar6.c) -272 |  |
| 81 | 221 | `0x00441980` | 9 | called by Put3DBlokesOnRide (rides.c) | GetObjRiderN (savemisc2.c) +192 |  |
| 81 | 201 | `0x00460f50` | 17 | called by DrawCursorTileAt (pathmask.c) | DrawPathTileOverlay (render5.c) +192 | declared DrawCursorPathTile in pathmask.c |
| 81 | 197 | `0x0046a960` | 21 | table at 0x004b9de0 in .data | ScriptEventDue (uimisc.c) -2208 |  |
| 80 | 194 | `0x0041ef60` | 3 | called by Raster_SubmitPoly (coaster3d.c) | SetSpanClip (coastermath.c) +64 |  |
| 80 | 191 | `0x0041f650` | 4 | called by 0x0041f720 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) -512 | DEAD |
| 80 | 261 | `0x00426850` | 6 | unreferenced (swept) | Raster_ResetClipRing (coastertiny.c) +272 | DEAD |
| 80 | 320 | `0x0048c860` | 29 | called by DeleteIconInput (screens3.c) | InitProfileCheckBoxIcons (screens2.c) +320 |  |
| 80 | 186 | `0x00499240` | 31 | unreferenced (swept) | FreezeGameClock (sysstubs.c) -320 | DEAD |
| 79 | 192 | `0x0041ee40` | 3 | called by 0x0041cd20 [unmatched] | TrackRemoveObject (coaster.c) +144 |  |
| 79 | 194 | `0x00483300` | 28 | called by 0x00483ef0 [unmatched] (+3 more) | DoPendingAction (sweep3.c) +192 |  |
| 78 | 180 | `0x0041f4e0` | 4 | called by 0x0041db90 [unmatched] (+2 more) | Span_SetClip (coaster7.c) +352 |  |
| 78 | 169 | `0x00427310` | 6 | unreferenced (swept) | RouteSeat_IsOccupied (coaster9.c) -176 | DEAD |
| 78 | 204 | `0x0046a230` | 20 | table at 0x004b9d8c in .data | UpdateGoalHelpText (softblit.c) +3632 |  |
| 77 | 259 | `0x0041db90` | 3 | called by RoutePhys_EvaluateDerivative (coaster7.c) (+1 more) | Route_SumCarVelocity (coaster8.c) +176 |  |
| 77 | 203 | `0x00450250` | 13 | table at 0x004b83a0 in .data | DoHighLevelAI (blokeai.c) -640 |  |
| 77 | 226 | `0x00478d30` | 25 | table at 0x004bb774 in .data | RequestRoute (simcore.c) +4448 |  |
| 76 | 191 | `0x0040bd40` | 1 | unreferenced (swept) | LFRun_Tick (logflume4.c) -192 | DEAD |
| 76 | 265 | `0x00429e20` | 7 | table at 0x004b63fc in .data | TrackCurve_EvaluateOffset (coaster9.c) +624 |  |
| 76 | 182 | `0x00483b60` | 28 | called by 0x00483e20 [unmatched] | Handle_RndWalk_TileSpecifics (tilehelp.c) +80 |  |
| 76 | 175 | `0x00499190` | 31 | unreferenced (swept) | DeleteStrings (text.c) +416 | DEAD |
| 75 | 183 | `0x00455c80` | 15 | called by 0x00455e50 [unmatched] | FindCachedText (fpui3.c) -192 |  |
| 74 | 177 | `0x00411fa0` | 3 | called by LFQueue_StepFront (lfmisc2.c) | LFQueue_AddRider (lfentrance.c) +128 |  |
| 74 | 186 | `0x00444df0` | 10 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2960 |  |
| 74 | 208 | `0x00455bb0` | 15 | called by HTBubbleHelp (fpui2.c) (+3 more) | FindCachedText (fpui3.c) -400 |  |
| 74 | 193 | `0x0046ee00` | 22 | called by 0x00459360 [unmatched] (+1 more) | RenderIcons (fpui.c) -224 |  |
| 74 | 204 | `0x00483580` | 28 | called by 0x00484520 [unmatched] (+1 more) | HitObstacle (workers.c) +112 |  |
| 73 | 244 | `0x0040d420` | 2 | called by 0x0040d6f0 [unmatched] | LFPiece_TickCommon (logflume2.c) +112 |  |
| 73 | 193 | `0x004809d0` | 27 | called by LoadObjectClass (saveprof.c) | AddNewObjectClass (objmap.c) +64 |  |
| 73 | 258 | `0x00495f00` | 30 | unreferenced (swept) | LoadMusicSegment (music.c) +208 | DEAD |
| 73 | 213 | `0x004967f0` | 30 | called by 0x004969d0 [unmatched] | RefreshSampleVolumes (audio5.c) +64 |  |
| 73 | 243 | `0x00498000` | 30 | called by 0x00498250 [unmatched] (+1 more) | TellAllLayersToStopAnimating (sprite2.c) +224 |  |
| 72 | 238 | `0x0040ce20` | 1 | called by 0x0040cf10 [unmatched] | LFPiece_IsVisible (logflume2.c) +48 |  |
| 72 | 188 | `0x004263a0` | 6 | called by 0x00426750 [unmatched] | Mat3_ToMat4 (coaster7.c) -240 |  |
| 71 | 228 | `0x004049a0` | 1 | called by Copters_InitRecord (ridemachine.c) (+1 more) | Copters_SetFull (ridemisc.c) +240 | declared Copters_StopRide in ridemachine.c |
| 71 | 172 | `0x0047a5a0` | 26 | table at 0x004bb934 in .data | LoadLevelDatabase (movie.c) -2576 |  |
| 70 | 232 | `0x00429f30` | 7 | called by RouteCar_SetPosition (schoolcar4.c) (+2 more) | TrackCurve_EvaluateOffset (coaster9.c) +896 |  |
| 70 | 232 | `0x0042a020` | 7 | unreferenced (swept) | CoasterFxPoolInit (schoolcar.c) -704 | DEAD |
| 70 | 251 | `0x004640f0` | 18 | tail-jumped from PushSetTarget (gpu.c) | PushRenderingStatusAndUnlockVideoSurface (surface.c) +112 |  |
| 69 | 289 | `0x00401e00` | 1 | unreferenced (swept) | SchoolCarIdleStep (schoolcar4.c) +304 | DEAD |
| 69 | 209 | `0x0040adb0` | 1 | unreferenced (swept) | LFPiece_ShapeIndex (logflume2.c) +96 | DEAD |
| 69 | 172 | `0x0046ac50` | 21 | table at 0x004b9dfc in .data | ScriptEventDue (uimisc.c) -1456 |  |
| 68 | 158 | `0x0046a690` | 21 | table at 0x004b9dd4 in .data | ScriptEventDue (uimisc.c) -2928 |  |
| 67 | 211 | `0x004762f0` | 23 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) +224 | DEAD |
| 67 | 145 | `0x00478110` | 24 | called by 0x00478280 [unmatched] | RequestRoute (simcore.c) +1344 |  |
| 67 | 164 | `0x0047aea0` | 27 | table at 0x004bb7dc in .data | LoadLevelDatabase (movie.c) -272 |  |
| 66 | 183 | `0x0041d950` | 3 | called by Route_Reset (schoolcar7.c) | PositionRouteCars (schoolcar.c) -192 |  |
| 66 | 122 | `0x004265d0` | 6 | called by ClipRect_SetBounds [declared in coaster9.c] (+1 more) | MakeRotation (coaster6.c) +112 |  |
| 66 | 177 | `0x00478fa0` | 25 | table at 0x004bb79c in .data | RequestRoute (simcore.c) +5072 |  |
| 66 | 177 | `0x00479060` | 25 | table at 0x004bb7a4 in .data | RequestRoute (simcore.c) +5264 |  |
| 64 | 143 | `0x0040c250` | 1 | unreferenced (swept) | LFPiece_HasCursor (logflume6.c) -144 | DEAD |
| 64 | 188 | `0x00423080` | 5 | unreferenced (swept) | CoasterShades_Init (schoolcar5.c) +160 | DEAD |
| 64 | 487 | `0x00454700` | 14 | called by 0x00453da0 [unmatched] | LoadBubbleHelpGFX (text.c) -528 |  |
| 64 | 158 | `0x0047ae00` | 27 | table at 0x004bb7d4 in .data | LoadLevelDatabase (movie.c) -432 |  |
| 64 | 172 | `0x00484470` | 29 | table at 0x004bd378 in .data | GetTileInDir (pathtile2.c) -560 |  |
| 64 | 172 | `0x00484520` | 29 | table at 0x004bd37c in .data | GetTileInDir (pathtile2.c) -384 |  |
| 64 | 167 | `0x00496920` | 30 | tail-jumped from 0x004969d0 [unmatched] | UnSourcePlayableSample (audio2.c) -208 |  |
| 63 | 170 | `0x0041f5a0` | 4 | unreferenced (swept) | Span_SetClip (coaster7.c) +544 | DEAD |
| 63 | 179 | `0x0047a6f0` | 26 | table at 0x004bb94c in .data | LoadLevelDatabase (movie.c) -2240 |  |
| 63 | 232 | `0x00481d70` | 28 | unreferenced (swept) | RemovePathSquare (pathsq.c) +224 | DEAD |
| 62 | 157 | `0x004542e0` | 14 | called by 0x00453da0 [unmatched] | DBError (sysstubs.c) +1536 |  |
| 61 | 167 | `0x0040db00` | 2 | called by LFDrop_Remove (logflume.c) (+7 more) | LFTrack_Tick (logflume.c) -176 |  |
| 61 | 175 | `0x00423200` | 6 | called by Raster_SubmitPoly (coaster3d.c) | ZBuffer_RunCommand (schoolcar5.c) -176 |  |
| 61 | 146 | `0x0046afe0` | 21 | table at 0x004b9e38 in .data | ScriptEventDue (uimisc.c) -544 |  |
| 61 | 122 | `0x00499040` | 31 | unreferenced (swept) | DeleteStrings (text.c) +80 | DEAD |
| 60 | 199 | `0x0041d210` | 3 | called by TrackFitCheckSpan (coaster5.c) (+1 more) | BuildJoint (coastermath.c) +64 |  |
| 60 | 183 | `0x004449b0` | 9 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +1872 |  |
| 60 | 213 | `0x0044db90` | 12 | called by 0x00458ee0 [unmatched] | LoadBinV (rin.c) -256 |  |
| 60 | 130 | `0x0046aae0` | 21 | table at 0x004b9dec in .data | ScriptEventDue (uimisc.c) -1824 |  |
| 60 | 265 | `0x004784c0` | 24 | called by LoadLevelDatabase (movie.c) | RequestRoute (simcore.c) +2288 | declared ResetLevelGlobals in movie.c |
| 60 | 142 | `0x00483d90` | 28 | table at 0x004bd384 in .data | Handle_RndWalk_TileSpecifics (tilehelp.c) +640 |  |
| 59 | 128 | `0x0046af60` | 21 | table at 0x004b9e34 in .data | ScriptEventDue (uimisc.c) -672 |  |
| 59 | 163 | `0x0046f5e0` | 23 | unreferenced (swept) | AddGBarClassIcon (fpui.c) -176 | DEAD |
| 59 | 136 | `0x00479640` | 25 | table at 0x004bb83c in .data | LoadLevelDatabase (movie.c) -6512 |  |
| 59 | 139 | `0x00489440` | 29 | called by 0x004895a0 [unmatched] | RenderBox (renderlist.c) +48 |  |
| 58 | 139 | `0x0047a140` | 26 | table at 0x004bb9c4 in .data | LoadLevelDatabase (movie.c) -3696 |  |
| 58 | 126 | `0x00489e60` | 29 | called by 0x00478280 [unmatched] | RES_FileExists (listdel.c) +48 |  |
| 57 | 130 | `0x0043f460` | 8 | called by 0x0043f4f0 [unmatched] | LoadPos (loaders.c) -512 | DEAD |
| 57 | 117 | `0x004427e0` | 9 | called by 0x00442980 [unmatched] | SkipStrings (savemisc2.c) -224 |  |
| 57 | 152 | `0x004659a0` | 18 | called by RenderAdvisorIcon (screens3.c) | SoftPrint_XBltFast (bigrender.c) -160 |  |
| 57 | 146 | `0x00478f00` | 25 | table at 0x004bb7ac in .data | RequestRoute (simcore.c) +4912 |  |
| 57 | 179 | `0x00491fe0` | 30 | unreferenced (swept) | NewProfileCloseInput (screens3.c) -192 | DEAD |
| 56 | 153 | `0x004267b0` | 6 | unreferenced (swept) | Raster_ResetClipRing (coastertiny.c) +112 | DEAD |
| 56 | 146 | `0x0046d590` | 22 | called by 0x00474ed0 [unmatched] | RemoveIconGroup (iconui.c) +112 |  |
| 56 | 127 | `0x00479450` | 25 | table at 0x004bb81c in .data | RequestRoute (simcore.c) +6272 |  |
| 56 | 128 | `0x004795c0` | 25 | table at 0x004bb834 in .data | RequestRoute (simcore.c) +6640 |  |
| 55 | 149 | `0x0041f2b0` | 3 | called by 0x0041ef60 [unmatched] (+1 more) | Span_SetClip (coaster7.c) -208 |  |
| 55 | 159 | `0x004514b0` | 13 | unreferenced (swept) | RES_EnsureMounted (sysmisc.c) -304 | DEAD |
| 55 | 134 | `0x0047a020` | 26 | table at 0x004bb9b4 in .data | LoadLevelDatabase (movie.c) -3984 |  |
| 55 | 134 | `0x0047a0b0` | 26 | table at 0x004bb9bc in .data | LoadLevelDatabase (movie.c) -3840 |  |
| 54 | 127 | `0x00478be0` | 24 | called by 0x00478bc0 [unmatched] | RequestRoute (simcore.c) +4112 |  |
| 54 | 132 | `0x00479300` | 25 | table at 0x004bb804 in .data | RequestRoute (simcore.c) +5936 |  |
| 54 | 120 | `0x0047a480` | 26 | called by 0x00478be0 [unmatched] | LoadLevelDatabase (movie.c) -2864 |  |
| 54 | 121 | `0x0047ac00` | 27 | table at 0x004bb984 in .data | LoadLevelDatabase (movie.c) -944 |  |
| 54 | 146 | `0x00483260` | 28 | called by 0x00483300 [unmatched] | DoPendingAction (sweep3.c) +32 |  |
| 54 | 123 | `0x00483d10` | 28 | table at 0x004bd35c in .data | Handle_RndWalk_TileSpecifics (tilehelp.c) +512 |  |
| 53 | 173 | `0x00470b00` | 23 | called by 0x00471170 [unmatched] (+1 more) | InitPopUpInfo (bighelp.c) -176 |  |
| 53 | 147 | `0x00476250` | 23 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) +64 | DEAD |
| 53 | 182 | `0x00496e60` | 30 | called by 0x00469f80 [unmatched] | Kill_FXList (audiomisc.c) +48 |  |
| 52 | 147 | `0x00454500` | 14 | called by 0x00454380 [unmatched] (+1 more) | LoadBubbleHelpGFX (text.c) -1040 |  |
| 52 | 149 | `0x00475f40` | 23 | called by 0x00457a70 [unmatched] | RestoreCurrentMenu (sysstubs.c) +48 |  |
| 52 | 128 | `0x004829c0` | 28 | called by ResolveEntrancePathSquare (pathmask.c) (+1 more) | ResolveEntrancePathSquare (pathmask.c) -128 | declared MarkPathSquareReachable in pathmask.c |
| 52 | 157 | `0x0048e0c0` | 30 | called by LoadSavedGamesList (profiles.c) | DeleteSavedGameList (listdel.c) -160 |  |
| 51 | 162 | `0x004237f0` | 6 | called by 0x004267b0 [unmatched] (+1 more) | Raster_RestoreState (coastertiny.c) +96 | DEAD |
| 51 | 217 | `0x00426000` | 6 | unreferenced (swept) | MatIdentity (coastermath.c) -240 | DEAD |
| 51 | 177 | `0x00444090` | 9 | called by 0x00459520 [unmatched] | SetAdvisorPose (tinystubs.c) +32 |  |
| 51 | 109 | `0x00455de0` | 15 | unreferenced (swept) | FindCachedText (fpui3.c) +160 | DEAD |
| 51 | 118 | `0x00479e00` | 26 | table at 0x004bb8d4 in .data | LoadLevelDatabase (movie.c) -4528 |  |
| 51 | 125 | `0x0047a8e0` | 26 | table at 0x004bb974 in .data | LoadLevelDatabase (movie.c) -1744 |  |
| 51 | 125 | `0x0047ab00` | 26 | table at 0x004bb7c4 in .data | LoadLevelDatabase (movie.c) -1200 |  |
| 51 | 125 | `0x0047ab80` | 26 | table at 0x004bb7cc in .data | LoadLevelDatabase (movie.c) -1072 |  |
| 51 | 156 | `0x0047b500` | 27 | unreferenced (swept) | LLIDB_ClearOnLevel (memdb.c) +64 | DEAD |
| 51 | 149 | `0x00480aa0` | 28 | called by LLIDB_LoadODFData (llidb_odf.c) | LoadObjectClass (saveprof.c) -160 |  |
| 50 | 123 | `0x00408f90` | 1 | unreferenced (swept) | LFTrack_FindPiece (posstep.c) +96 | DEAD |
| 50 | 155 | `0x0045ac20` | 17 | called by 0x00459520 [unmatched] | GetTileBounds (pathbuild.c) -160 |  |
| 50 | 105 | `0x0046aa70` | 21 | table at 0x004b9de8 in .data | ScriptEventDue (uimisc.c) -1936 |  |
| 50 | 119 | `0x004791f0` | 25 | table at 0x004bb7f4 in .data | RequestRoute (simcore.c) +5664 |  |
| 50 | 131 | `0x00479270` | 25 | table at 0x004bb7fc in .data | RequestRoute (simcore.c) +5792 |  |
| 49 | 128 | `0x00409a90` | 1 | called by 0x0040d900 [unmatched] | LFRoute_Join (logflume2.c) -224 |  |
| 49 | 133 | `0x00421ab0` | 5 | called by Castle_InitEntranceTrack (coaster7.c) (+2 more) | TrackCurve_LineUpVector (coastertiny.c) +32 |  |
| 49 | 133 | `0x004238a0` | 6 | called by 0x00428b80 [unmatched] (+1 more) | Raster_RestoreState (coastertiny.c) +272 | DEAD |
| 49 | 143 | `0x00445100` | 10 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +3744 |  |
| 49 | 132 | `0x00451550` | 13 | called by 0x00451210 [unmatched] | RES_EnsureMounted (sysmisc.c) -144 | DEAD |
| 49 | 109 | `0x00455e50` | 15 | called by DrawPopUpExtra (popup2.c) (+6 more) | PrintCachedEntry (tinystubs.c) -112 |  |
| 49 | 129 | `0x00458830` | 16 | called by 0x00453d10 [unmatched] | RenderFrontEndScreen (mapscreen.c) +240 |  |
| 49 | 149 | `0x00478890` | 24 | table at 0x004bb704 in .data | RequestRoute (simcore.c) +3264 |  |
| 49 | 125 | `0x00479120` | 25 | table at 0x004bb7e4 in .data | RequestRoute (simcore.c) +5456 |  |
| 49 | 142 | `0x00498150` | 31 | called by 0x00498250 [unmatched] | RewindNarrationSource (tinystubs.c) +48 |  |
| 48 | 125 | `0x004274f0` | 6 | unreferenced (swept) | RouteSeat_InitPosition (coaster8.c) +64 | DEAD |
| 48 | 157 | `0x00444ef0` | 10 | pointer in 0x00445190 [unmatched] (+1 more) | LoadReport (uimisc.c) +3216 |  |
| 48 | 117 | `0x00450450` | 13 | table at 0x004b83a4 in .data | DoHighLevelAI (blokeai.c) -128 |  |
| 48 | 143 | `0x00453d10` | 14 | called from CRT 0x004a0996 | DBError (sysstubs.c) +48 | SEH |
| 48 | 111 | `0x00479550` | 25 | table at 0x004bb82c in .data | RequestRoute (simcore.c) +6528 |  |
| 47 | 148 | `0x00476bf0` | 23 | called by RunMovie (movie.c) | RunMovie (movie.c) +1280 | declared PrimeMovieAudio in movie.c |
| 47 | 113 | `0x00479fa0` | 26 | table at 0x004bb8f4 in .data | LoadLevelDatabase (movie.c) -4112 |  |
| 46 | 104 | `0x0040a010` | 1 | called by 0x0040a080 [unmatched] | LFEntrance_Create (logflume.c) -720 |  |
| 46 | 149 | `0x00444150` | 9 | called by 0x00459520 [unmatched] | SaveReport (uimisc.c) -176 |  |
| 46 | 123 | `0x004551a0` | 15 | called by 0x0043ea30 [unmatched] | PrintCentColref (text.c) +320 | DEAD |
| 46 | 114 | `0x004794d0` | 25 | table at 0x004bb824 in .data | RequestRoute (simcore.c) +6400 |  |
| 46 | 128 | `0x004983a0` | 31 | called by RewindNarrationBuffer (movie.c) (+1 more) | ReadNarrationWaveHeader (audio5.c) -128 | declared ReadDecodedNarration in movie.c |
| 46 | 111 | `0x00498f80` | 31 | called by 0x00498d00 [unmatched] | GetString (text.c) +48 |  |
| 46 | 98 | `0x00499120` | 31 | unreferenced (swept) | DeleteStrings (text.c) +304 | DEAD |
| 46 | 136 | `0x0049cfc0` | 31 | called by 0x00458b20 [unmatched] | MarkWorkersOnMap (workorder4.c) +192 |  |
| 45 | 108 | `0x0040a080` | 1 | called by 0x0040d900 [unmatched] | LFEntrance_Create (logflume.c) -608 |  |
| 45 | 132 | `0x0041cd80` | 3 | called by BuildJoint (coastermath.c) | InitTrackJoint (coaster.c) -144 |  |
| 45 | 144 | `0x0041e9e0` | 3 | called by RouteNode_UpdateClipRect (coaster9.c) (+1 more) | RouteNode_UpdateClipRect (coaster9.c) +80 | declared RouteNode_GetTransform in coaster9.c |
| 45 | 86 | `0x00442860` | 9 | called by 0x00442980 [unmatched] | SkipStrings (savemisc2.c) -96 |  |
| 45 | 168 | `0x004589a0` | 16 | called by 0x00459520 [unmatched] | EnterParkPlayMode (movie.c) +96 |  |
| 45 | 198 | `0x00458a50` | 16 | called by LowProgressAcceptInput (screens3.c) (+2 more) | EnterParkPlayMode (movie.c) +272 |  |
| 45 | 94 | `0x004990c0` | 31 | unreferenced (swept) | DeleteStrings (text.c) +208 | DEAD |
| 44 | 106 | `0x004272a0` | 6 | unreferenced (swept) | ReadCoasterBlob (coaster.c) +96 | DEAD |
| 44 | 92 | `0x0046a900` | 21 | table at 0x004b9ddc in .data | ScriptEventDue (uimisc.c) -2304 |  |
| 44 | 134 | `0x0046cff0` | 22 | called by 0x00458ee0 [unmatched] | ProcessInGameHelp (iconui.c) +144 |  |
| 44 | 165 | `0x00474750` | 23 | called by AdventureThemeInput (screens3.c) (+3 more) | UnLoad_Interface_Icons (panelui.c) -176 |  |
| 44 | 104 | `0x00479f30` | 26 | table at 0x004bb8ec in .data | LoadLevelDatabase (movie.c) -4224 |  |
| 44 | 106 | `0x0047a2f0` | 26 | table at 0x004bb8fc in .data | LoadLevelDatabase (movie.c) -3264 |  |
| 44 | 106 | `0x0047a360` | 26 | table at 0x004bb904 in .data | LoadLevelDatabase (movie.c) -3152 |  |
| 44 | 108 | `0x0047a3d0` | 26 | table at 0x004bb90c in .data | LoadLevelDatabase (movie.c) -3040 |  |
| 43 | 134 | `0x00422180` | 5 | called by 0x00429560 [unmatched] | CarClassTablesInit (schoolcar.c) -144 |  |
| 43 | 107 | `0x00422400` | 5 | called by CoasterModel_FindPartIndex (coaster9.c) (+1 more) | CountModelRecords (coaster7.c) +64 | declared ModelImage_FindName in coaster9.c |
| 43 | 106 | `0x00422520` | 5 | called by 0x00422650 [unmatched] | CoasterModel_FindMeshIndex (coaster9.c) -112 | DEAD |
| 43 | 112 | `0x0044fe10` | 13 | table at 0x004b83c4 in .data | DoHighLevelAI (blokeai.c) -1728 |  |
| 43 | 100 | `0x00450330` | 13 | table at 0x004b83b8 in .data | DoHighLevelAI (blokeai.c) -416 |  |
| 43 | 121 | `0x00469900` | 20 | called by UnlockSidePanelObjects (movie.c) (+2 more) | UpdateGoalHelpText (softblit.c) +1280 | declared MarkElemAvailable in movie.c |
| 43 | 108 | `0x0046a540` | 21 | table at 0x004b9dcc in .data | ScriptEventDue (uimisc.c) -3264 |  |
| 43 | 131 | `0x0046ea10` | 22 | pointer in 0x0046f860 [unmatched] | RenderGBarSprite (render2.c) +64 | DEAD |
| 43 | 134 | `0x004781f0` | 24 | called by LoadLevelDatabase (movie.c) | RequestRoute (simcore.c) +1568 | declared ParseKeywordFile in movie.c |
| 43 | 106 | `0x004796d0` | 25 | table at 0x004bb844 in .data | LoadLevelDatabase (movie.c) -6368 |  |
| 43 | 101 | `0x0048a6e0` | 29 | called by 0x0048a750 [unmatched] (+1 more) | ClipThisRect (util.c) +32 |  |
| 42 | 101 | `0x004261c0` | 6 | called by Coaster3D_DrawModel (coaster9.c) (+1 more) | TransformVerts (coaster3d.c) -144 | declared TransformVec3 in coaster9.c |
| 42 | 142 | `0x00429c60` | 7 | called by Route_TravelPerTick (coaster9.c) (+1 more) | TrackCurve_EvaluateOffset (coaster9.c) +176 | declared TrackCurve_EvaluateDerivative in coaster9.c |
| 42 | 122 | `0x00444b70` | 10 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2320 |  |
| 42 | 118 | `0x00444d70` | 10 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2832 |  |
| 42 | 100 | `0x00478e20` | 25 | table at 0x004bb77c in .data | RequestRoute (simcore.c) +4688 |  |
| 42 | 100 | `0x00478e90` | 25 | table at 0x004bb78c in .data | RequestRoute (simcore.c) +4800 |  |
| 42 | 98 | `0x0047aa90` | 26 | table at 0x004bb7bc in .data | LoadLevelDatabase (movie.c) -1312 |  |
| 42 | 110 | `0x00499490` | 31 | unreferenced (swept) | GetBlink (util.c) +16 | DEAD |
| 41 | 108 | `0x004298a0` | 7 | called by TrackFitCheckSpan (coaster5.c) | TrackFitEndGeom (coaster6.c) +96 |  |
| 40 | 101 | `0x0040a230` | 1 | called by 0x0040a2a0 [unmatched] | LFEntrance_Create (logflume.c) -176 |  |
| 40 | 120 | `0x0041ea70` | 3 | called by RouteNode_AddPending (coaster9.c) | RouteNode_AddPending (coaster9.c) -128 | declared RouteNode_LinkPending in coaster9.c |
| 40 | 162 | `0x0046a040` | 20 | called by 0x0047a8e0 [unmatched] (+1 more) | UpdateGoalHelpText (softblit.c) +3136 |  |
| 40 | 97 | `0x00479740` | 25 | table at 0x004bb84c in .data | LoadLevelDatabase (movie.c) -6256 |  |
| 40 | 97 | `0x00479850` | 25 | table at 0x004bb864 in .data | LoadLevelDatabase (movie.c) -5984 |  |
| 40 | 97 | `0x004798c0` | 25 | table at 0x004bb86c in .data | LoadLevelDatabase (movie.c) -5872 |  |
| 39 | 124 | `0x00427f70` | 6 | table at 0x004b5df4 in .data | TrackH0_Create (castleobj.c) +64 |  |
| 39 | 124 | `0x00427ff0` | 6 | table at 0x004b5df0 in .data | TrackHP_Create (castleobj.c) -128 |  |
| 39 | 100 | `0x00429af0` | 7 | called by RouteNode_GetTransform [declared in coaster9.c] (+1 more) | LevelTrackRun (schoolcar.c) +192 |  |
| 39 | 123 | `0x00451390` | 13 | unreferenced (swept) | RES_EnsureMounted (sysmisc.c) -592 | DEAD |
| 39 | 87 | `0x0046b650` | 21 | called by 0x004791a0 [unmatched] | ShowScriptStepText (uimisc.c) -96 |  |
| 39 | 144 | `0x00471c10` | 23 | called by MarkElemAvailable [declared in movie.c] | ResetInfoSelection (tinystubs.c) +32 |  |
| 39 | 118 | `0x00477600` | 24 | unreferenced (swept) | MemScratch_Noop (coaster.c) +16 | DEAD |
| 39 | 99 | `0x004793e0` | 25 | table at 0x004bb814 in .data | RequestRoute (simcore.c) +6160 |  |
| 39 | 105 | `0x00488730` | 29 | unreferenced (swept) | SetMousePixel (tri3d.c) +48 | DEAD |
| 39 | 122 | `0x00496010` | 30 | unreferenced (swept) | GetMusicBand (music.c) -128 | DEAD |
| 38 | 99 | `0x0040a0f0` | 1 | called by 0x0040a2a0 [unmatched] | LFEntrance_Create (logflume.c) -496 |  |
| 38 | 96 | `0x0041ede0` | 3 | called by 0x0041ee40 [unmatched] | TrackRemoveObject (coaster.c) +48 |  |
| 38 | 114 | `0x00421ce0` | 5 | called by Castle_InitEntranceTrack (coaster7.c) (+1 more) | TrackCurve_CubicUpVector (coastertiny.c) +32 |  |
| 38 | 133 | `0x00445310` | 10 | called by 0x00445190 [unmatched] (+2 more) | LoadReport (uimisc.c) +4272 |  |
| 38 | 99 | `0x00478700` | 24 | called by 0x0047a7b0 [unmatched] (+6 more) | RequestRoute (simcore.c) +2864 |  |
| 38 | 93 | `0x004975b0` | 30 | called by UnreferenceSprite (spritemisc.c) | NewSprite (sysstubs.c) +48 |  |
| 37 | 81 | `0x00409b10` | 1 | called by 0x0040a010 [unmatched] | LFRoute_Join (logflume2.c) -96 |  |
| 37 | 98 | `0x0040a160` | 1 | called by 0x0040a2a0 [unmatched] | LFEntrance_Create (logflume.c) -384 |  |
| 37 | 96 | `0x0040a1d0` | 1 | called by 0x0040a2a0 [unmatched] | LFEntrance_Create (logflume.c) -272 |  |
| 37 | 104 | `0x0041db20` | 3 | pointer in 0x0041db90 [unmatched] | Route_SumCarVelocity (coaster8.c) +64 |  |
| 37 | 108 | `0x0043a940` | 8 | called by SpaceTower_StepMachine (ridemachine.c) | SpaceTower_CountSeated (bswater3.c) -112 | declared SpaceTower_StepCar in ridemachine.c |
| 37 | 106 | `0x0046a3b0` | 20 | table at 0x004b9da0 in .data | ScriptEventDue (uimisc.c) -3664 |  |
| 37 | 106 | `0x0046dfd0` | 22 | pointer in 0x0046d800 [unmatched] | RenderBoxIcon (render2.c) +96 | DEAD |
| 37 | 136 | `0x0046f890` | 23 | called by 0x00459520 [unmatched] | AddFreePlayIcon (fpui.c) +240 |  |
| 37 | 89 | `0x00478ac0` | 24 | table at 0x004bb74c in .data | RequestRoute (simcore.c) +3824 |  |
| 37 | 97 | `0x00479c40` | 25 | table at 0x004bb8ac in .data | LoadLevelDatabase (movie.c) -4976 |  |
| 37 | 90 | `0x00479d00` | 26 | table at 0x004bb8bc in .data | LoadLevelDatabase (movie.c) -4784 |  |
| 37 | 90 | `0x00479e80` | 26 | table at 0x004bb8dc in .data | LoadLevelDatabase (movie.c) -4400 |  |
| 36 | 115 | `0x0046dac0` | 22 | tail-jumped from ScanMouse (input.c) | ScrollDownInput (fpui3.c) +160 |  |
| 36 | 115 | `0x0046db40` | 22 | tail-jumped from ScanMouse (input.c) | AddGBarIcons (fpui.c) -128 |  |
| 36 | 116 | `0x0046f920` | 23 | called by 0x00459520 [unmatched] | AddFreePlayIcon (fpui.c) +384 |  |
| 36 | 131 | `0x00476c90` | 23 | called by RunMovie (movie.c) | PlayMovie (uimisc2.c) -1376 | declared StopMovieAudio in movie.c |
| 36 | 97 | `0x00478c60` | 24 | table at 0x004bb764 in .data | RequestRoute (simcore.c) +4240 |  |
| 36 | 82 | `0x00499ca0` | 31 | unreferenced (swept) | FindFreeGardener (workorder2.c) +96 | DEAD |
| 35 | 82 | `0x00429690` | 7 | called by TrackJoinPieces (coaster5.c) | TrackRunSpanEnd (coaster6.c) -96 |  |
| 35 | 98 | `0x00441910` | 9 | called by 0x00441980 [unmatched] | GetObjRiderN (savemisc2.c) +80 |  |
| 35 | 110 | `0x00451210` | 13 | unreferenced (swept) | RES_FindVolumeOnResPath (sysmisc2.c) +304 | DEAD |
| 35 | 107 | `0x00451410` | 13 | called by 0x00451210 [unmatched] | RES_EnsureMounted (sysmisc.c) -464 | DEAD |
| 35 | 111 | `0x00469bd0` | 20 | called by 0x0047a5a0 [unmatched] (+1 more) | UpdateGoalHelpText (softblit.c) +2000 |  |
| 35 | 94 | `0x004845d0` | 29 | table at 0x004bd368 in .data | GetTileInDir (pathtile2.c) -208 |  |
| 34 | 108 | `0x0041f720` | 4 | called by 0x0041f7f0 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) -304 | DEAD |
| 34 | 84 | `0x00444c70` | 10 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2576 |  |
| 34 | 74 | `0x0046ae70` | 21 | table at 0x004b9e28 in .data | ScriptEventDue (uimisc.c) -912 |  |
| 34 | 73 | `0x00478b70` | 24 | table at 0x004bb754 in .data | RequestRoute (simcore.c) +4000 |  |
| 34 | 91 | `0x00478cd0` | 25 | table at 0x004bb76c in .data | RequestRoute (simcore.c) +4352 |  |
| 34 | 91 | `0x0047ace0` | 27 | table at 0x004bb98c in .data | LoadLevelDatabase (movie.c) -720 |  |
| 34 | 91 | `0x0047ad40` | 27 | table at 0x004bb99c in .data | LoadLevelDatabase (movie.c) -624 |  |
| 34 | 98 | `0x00484630` | 29 | table at 0x004bd36c in .data | GetTileInDir (pathtile2.c) -112 |  |
| 33 | 78 | `0x0041f790` | 4 | pointer in 0x0041f7f0 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) -192 | DEAD |
| 33 | 102 | `0x00443dc0` | 9 | called by RenderAdvisorIcon (screens3.c) (+1 more) | RenderAdvisorIcon (screens3.c) -112 |  |
| 33 | 112 | `0x00469fc0` | 20 | table at 0x004b9d78 in .data | UpdateGoalHelpText (softblit.c) +3008 |  |
| 33 | 83 | `0x0046a190` | 20 | table at 0x004b9d84 in .data | UpdateGoalHelpText (softblit.c) +3472 |  |
| 33 | 61 | `0x004781b0` | 24 | called by 0x0047aea0 [unmatched] (+11 more) | RequestRoute (simcore.c) +1504 |  |
| 33 | 89 | `0x0047ada0` | 27 | table at 0x004bb9ac in .data | LoadLevelDatabase (movie.c) -528 |  |
| 33 | 83 | `0x00480570` | 27 | pointer in InitMIDIManager (lifecycle.c) | PlayMIDI (music.c) -96 |  |
| 33 | 74 | `0x00489550` | 29 | called by RES_OpenFileFromVolume (data2.c) | GetMasterVolPtr (audio3.c) +64 |  |
| 33 | 78 | `0x00496760` | 30 | called by 0x004969d0 [unmatched] | RefreshSampleVolumes (audio5.c) -80 |  |
| 32 | 69 | `0x0041e260` | 3 | unreferenced (swept) | Route_UpdateTimer (coastertiny.c) +32 | DEAD |
| 32 | 106 | `0x0044ed00` | 12 | called by 0x0044f610 [unmatched] (+1 more) | PopLongTermAction (sweep1.c) +304 |  |
| 32 | 77 | `0x004797b0` | 25 | table at 0x004bb854 in .data | LoadLevelDatabase (movie.c) -6144 |  |
| 32 | 85 | `0x0047a800` | 26 | table at 0x004bb95c in .data | LoadLevelDatabase (movie.c) -1968 |  |
| 32 | 79 | `0x004968d0` | 30 | called by 0x004969d0 [unmatched] | RefreshSampleVolumes (audio5.c) +288 |  |
| 31 | 105 | `0x00422650` | 5 | unreferenced (swept) | CoasterModel_GetPartCount (coastertiny.c) +16 | DEAD |
| 31 | 110 | `0x004588c0` | 16 | called by 0x00459520 [unmatched] | sub_458930 (bnvpath.c) -112 |  |
| 31 | 83 | `0x00468e40` | 20 | called by 0x0046a900 [unmatched] | ResetScriptTimer (tinystubs.c) +320 |  |
| 31 | 83 | `0x00468ea0` | 20 | called by 0x0046aa70 [unmatched] | ResetScriptTimer (tinystubs.c) +416 |  |
| 31 | 83 | `0x00469260` | 20 | called by 0x0046aae0 [unmatched] | RemoveGoals (savemisc2.c) -336 |  |
| 31 | 67 | `0x0046ac00` | 21 | table at 0x004b9e00 in .data | ScriptEventDue (uimisc.c) -1536 |  |
| 31 | 72 | `0x0046aec0` | 21 | table at 0x004b9e2c in .data | ScriptEventDue (uimisc.c) -832 |  |
| 31 | 72 | `0x0046af10` | 21 | table at 0x004b9e30 in .data | ScriptEventDue (uimisc.c) -752 |  |
| 31 | 80 | `0x0047a650` | 26 | table at 0x004bb93c in .data | LoadLevelDatabase (movie.c) -2400 |  |
| 31 | 80 | `0x0047a6a0` | 26 | table at 0x004bb944 in .data | LoadLevelDatabase (movie.c) -2320 |  |
| 31 | 80 | `0x0047a7b0` | 26 | table at 0x004bb954 in .data | LoadLevelDatabase (movie.c) -2048 |  |
| 30 | 109 | `0x00428350` | 6 | called by Coaster3D_BuildPieceGeometry (coaster3d.c) | TrackHP_Add (castleobj.c) +80 |  |
| 30 | 76 | `0x0046b130` | 21 | table at 0x004b9e4c in .data | ScriptEventDue (uimisc.c) -208 |  |
| 30 | 89 | `0x00473680` | 23 | unreferenced (swept) | ProcessHelpKeys (tinystubs.c) +32 | DEAD |
| 30 | 110 | `0x00476680` | 23 | called by RunMovie (movie.c) | CloseMovie (movie.c) +80 | declared MovieTicks in movie.c |
| 30 | 77 | `0x00479390` | 25 | table at 0x004bb80c in .data | RequestRoute (simcore.c) +6080 |  |
| 30 | 94 | `0x0047ac80` | 27 | table at 0x004bb7b4 in .data | LoadLevelDatabase (movie.c) -816 |  |
| 30 | 70 | `0x00486490` | 29 | unreferenced (swept) | SetFlatColour (tri3d.c) -80 | DEAD |
| 29 | 82 | `0x0041dca0` | 3 | called by Route_TravelThisTick (coaster9.c) | Route_TravelPerTick (coaster9.c) -96 | declared Route_GetSpeed in coaster9.c |
| 29 | 68 | `0x0041f880` | 4 | called by 0x00429f30 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) +48 |  |
| 29 | 74 | `0x00444bf0` | 10 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2448 |  |
| 29 | 144 | `0x00458b20` | 16 | called by LoadAcceptInput (screens3.c) (+1 more) | SetMapReady (sysstubs.c) -144 |  |
| 29 | 76 | `0x00469ed0` | 20 | table at 0x004b9d68 in .data | UpdateGoalHelpText (softblit.c) +2768 |  |
| 29 | 74 | `0x00469f20` | 20 | table at 0x004b9d6c in .data | UpdateGoalHelpText (softblit.c) +2848 |  |
| 29 | 67 | `0x0046a4f0` | 21 | table at 0x004b9dc8 in .data | ScriptEventDue (uimisc.c) -3344 |  |
| 29 | 83 | `0x0046b180` | 21 | table at 0x004b9e50 in .data | ScriptEventDue (uimisc.c) -128 |  |
| 29 | 92 | `0x00482cb0` | 28 | called by 0x00458ee0 [unmatched] | InitBlokeName (lfmisc.c) +80 |  |
| 28 | 65 | `0x004237a0` | 6 | unreferenced (swept) | Raster_RestoreState (coastertiny.c) +16 | DEAD |
| 28 | 62 | `0x00427570` | 6 | unreferenced (swept) | Track_Update (coaster.c) -96 | DEAD |
| 28 | 59 | `0x0042a110` | 7 | called by 0x0042a1b0 [unmatched] | CoasterFxPoolInit (schoolcar.c) -464 |  |
| 28 | 84 | `0x0042a150` | 7 | pointer in 0x0042a1b0 [unmatched] | CoasterFxPoolInit (schoolcar.c) -400 |  |
| 28 | 72 | `0x0046ab70` | 21 | table at 0x004b9df0 in .data | ScriptEventDue (uimisc.c) -1680 |  |
| 28 | 70 | `0x0047a500` | 26 | table at 0x004bb924 in .data | LoadLevelDatabase (movie.c) -2736 |  |
| 28 | 70 | `0x0047a550` | 26 | table at 0x004bb92c in .data | LoadLevelDatabase (movie.c) -2656 |  |
| 28 | 71 | `0x0048e3d0` | 30 | called by SaveEmptySlotInput (screens3.c) | SaveSlotInput (screens3.c) -208 |  |
| 28 | 70 | `0x00491f90` | 30 | unreferenced (swept) | NewProfileCloseInput (screens3.c) -272 | DEAD |
| 27 | 55 | `0x0041cd40` | 3 | called by 0x0041d210 [unmatched] | TrackClass_GetWorldBounds (schoolcar8.c) +96 |  |
| 27 | 56 | `0x0041e720` | 3 | called by 0x0041e260 [unmatched] | RouteNode_FindFreeSeat (coaster8.c) -64 | DEAD |
| 27 | 98 | `0x00444f90` | 10 | pointer in 0x00445190 [unmatched] (+1 more) | LoadReport (uimisc.c) +3376 |  |
| 27 | 77 | `0x00468c80` | 20 | called by 0x00468d30 [unmatched] (+22 more) | KillObjectHelp (fpui5.c) +128 |  |
| 27 | 71 | `0x00468d30` | 20 | called by 0x0046b180 [unmatched] (+26 more) | ResetScriptTimer (tinystubs.c) +48 |  |
| 27 | 78 | `0x0046be40` | 22 | called by 0x00479300 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1760 |  |
| 27 | 56 | `0x004786c0` | 24 | called by 0x0047aea0 [unmatched] (+75 more) | RequestRoute (simcore.c) +2800 |  |
| 27 | 73 | `0x00478930` | 24 | table at 0x004bb714 in .data | RequestRoute (simcore.c) +3424 |  |
| 27 | 63 | `0x00478a00` | 24 | table at 0x004bb72c in .data | RequestRoute (simcore.c) +3632 |  |
| 27 | 63 | `0x00478a40` | 24 | table at 0x004bb734 in .data | RequestRoute (simcore.c) +3696 |  |
| 27 | 68 | `0x00478b20` | 24 | called by UnlockSidePanelObjects (movie.c) (+2 more) | RequestRoute (simcore.c) +3920 | declared EnsureObjectClassLoaded in movie.c |
| 27 | 67 | `0x004791a0` | 25 | table at 0x004bb7ec in .data | RequestRoute (simcore.c) +5584 |  |
| 27 | 70 | `0x00479800` | 25 | table at 0x004bb85c in .data | LoadLevelDatabase (movie.c) -6064 |  |
| 27 | 70 | `0x00479930` | 25 | table at 0x004bb874 in .data | LoadLevelDatabase (movie.c) -5760 |  |
| 27 | 70 | `0x00479980` | 25 | table at 0x004bb87c in .data | LoadLevelDatabase (movie.c) -5680 |  |
| 27 | 70 | `0x004799d0` | 25 | table at 0x004bb884 in .data | LoadLevelDatabase (movie.c) -5600 |  |
| 27 | 70 | `0x00479a20` | 25 | table at 0x004bb88c in .data | LoadLevelDatabase (movie.c) -5520 |  |
| 27 | 70 | `0x00479a70` | 25 | table at 0x004bb894 in .data | LoadLevelDatabase (movie.c) -5440 |  |
| 27 | 70 | `0x00479ac0` | 25 | table at 0x004bb89c in .data | LoadLevelDatabase (movie.c) -5360 |  |
| 27 | 70 | `0x00479cb0` | 26 | table at 0x004bb8b4 in .data | LoadLevelDatabase (movie.c) -4864 |  |
| 27 | 70 | `0x00479d60` | 26 | table at 0x004bb8c4 in .data | LoadLevelDatabase (movie.c) -4688 |  |
| 27 | 70 | `0x00479db0` | 26 | table at 0x004bb8cc in .data | LoadLevelDatabase (movie.c) -4608 |  |
| 27 | 70 | `0x00479ee0` | 26 | table at 0x004bb8e4 in .data | LoadLevelDatabase (movie.c) -4304 |  |
| 27 | 59 | `0x004802f0` | 27 | called by 0x00480330 [unmatched] | LoadMIDIFile (music.c) +240 |  |
| 26 | 86 | `0x004096e0` | 1 | called by 0x004097a0 [unmatched] (+1 more) | LFTrack_MaskIsLegal (logflume2.c) +352 |  |
| 26 | 67 | `0x0041cf20` | 3 | unreferenced (swept) | Track_CountTailPieces (schoolcar8.c) +32 | DEAD |
| 26 | 61 | `0x00426be0` | 6 | unreferenced (swept) | PackCoasterPtr (coaster.c) +32 | DEAD |
| 26 | 65 | `0x00444cd0` | 10 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2672 |  |
| 26 | 65 | `0x00444d20` | 10 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +2752 |  |
| 26 | 77 | `0x004689a0` | 19 | called by 0x0046cb20 [unmatched] (+1 more) | FreeScriptEventList (tinystubs.c) +48 |  |
| 26 | 68 | `0x00473560` | 23 | unreferenced (swept) | ResetHelpKeyCursor (tinystubs.c) -80 | DEAD |
| 25 | 95 | `0x00409620` | 1 | called by 0x004097a0 [unmatched] (+1 more) | LFTrack_MaskIsLegal (logflume2.c) +160 |  |
| 25 | 90 | `0x00409680` | 1 | called by 0x004097a0 [unmatched] (+1 more) | LFTrack_MaskIsLegal (logflume2.c) +256 |  |
| 25 | 82 | `0x00409740` | 1 | called by 0x004097a0 [unmatched] (+1 more) | LFTrack_MaskIsLegal (logflume2.c) +448 |  |
| 25 | 55 | `0x0041e2f0` | 3 | unreferenced (swept) | Route_FindFreeSeat (schoolcar8.c) +64 | DEAD |
| 25 | 60 | `0x0041e8f0` | 3 | called by 0x0041d950 [unmatched] | RouteNode_GetTailTangent (schoolcar8.c) -64 |  |
| 25 | 60 | `0x00425da0` | 6 | called by 0x0042a110 [unmatched] | Invert2x2 (coastermath.c) -64 |  |
| 25 | 61 | `0x00429a80` | 7 | called by TrackCurve_EvaluateOffset (coaster9.c) (+1 more) | LevelTrackRun (schoolcar.c) +80 | declared TrackCurve_EvaluatePosition in coaster9.c |
| 25 | 64 | `0x00455a10` | 15 | called by 0x00455a50 [unmatched] | HTBubbleHelp (fpui2.c) +592 |  |
| 25 | 71 | `0x0046bf30` | 22 | called by 0x004794d0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1552 |  |
| 25 | 71 | `0x0046c240` | 22 | called by 0x00479c40 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -768 |  |
| 25 | 79 | `0x00497fb0` | 30 | called by 0x00498150 [unmatched] | TellAllLayersToStopAnimating (sprite2.c) +144 |  |
| 24 | 81 | `0x00426750` | 6 | called by CoasterModel_GetClipRect [declared in coaster9.c] | Raster_ResetClipRing (coastertiny.c) +16 |  |
| 24 | 62 | `0x00427a40` | 6 | table at 0x004b5d3c in .data | Track_Interact (castleobj.c) +64 |  |
| 24 | 62 | `0x00427c30` | 6 | table at 0x004b5d74 in .data | FindTrackDesc (castleobj.c) +48 |  |
| 23 | 59 | `0x00404860` | 1 | called by Copters_StepMachine (ridemachine.c) | Copters_ResumeSFX (ridetiny.c) -64 | declared Copters_StepCar in ridemachine.c |
| 23 | 62 | `0x0040a2a0` | 1 | called by LFTrack_Remove (logflume.c) (+1 more) | LFEntrance_Create (logflume.c) -64 |  |
| 23 | 46 | `0x0041e790` | 3 | called by 0x0041e2f0 [unmatched] | RouteNode_FindFreeSeat (coaster8.c) +48 | DEAD |
| 23 | 57 | `0x0046b590` | 21 | called by 0x004787d0 [unmatched] | FreeScriptStepList (tinystubs.c) +48 |  |
| 23 | 81 | `0x0046b700` | 21 | called by ScriptEndIconInput (screens3.c) | ShowScriptStepText (uimisc.c) +80 |  |
| 23 | 77 | `0x0046d800` | 22 | called by 0x0046f5e0 [unmatched] | LoadSpriteIcon (iconui.c) +80 | DEAD |
| 23 | 56 | `0x0046de10` | 22 | unreferenced (swept) | GetIconBounds (uimisc.c) -64 | DEAD |
| 23 | 46 | `0x00476070` | 23 | called by 0x0047aea0 [unmatched] (+2 more) | RenderIconsExtra (fpui4.c) -48 |  |
| 23 | 56 | `0x00478a80` | 24 | table at 0x004bb73c in .data | RequestRoute (simcore.c) +3760 |  |
| 23 | 57 | `0x0047a860` | 26 | table at 0x004bb964 in .data | LoadLevelDatabase (movie.c) -1872 |  |
| 23 | 57 | `0x0047a8a0` | 26 | table at 0x004bb96c in .data | LoadLevelDatabase (movie.c) -1808 |  |
| 23 | 69 | `0x0048c5e0` | 29 | called by PrintProfileDetails (bigscreens.c) (+1 more) | EnterNewProfileCheckBoxIcons (profiles.c) -112 |  |
| 23 | 53 | `0x00499300` | 31 | called by 0x00478280 [unmatched] | FreezeGameClock (sysstubs.c) -128 |  |
| 23 | 53 | `0x00499340` | 31 | unreferenced (swept) | FreezeGameClock (sysstubs.c) -64 | DEAD |
| 22 | 85 | `0x0041f7f0` | 4 | unreferenced (swept) | TrackCursor_AdvanceGeometry (schoolcar8.c) -96 | DEAD |
| 22 | 73 | `0x00429c10` | 7 | pointer in TrackCurve_EvaluateDerivative [declared in coaster9.c] | TrackCurve_EvaluateOffset (coaster9.c) +96 |  |
| 22 | 60 | `0x00443d50` | 9 | called by 0x00444150 [unmatched] | RenderAdvisorIcon (screens3.c) -224 |  |
| 22 | 75 | `0x00454290` | 14 | called by 0x00453da0 [unmatched] (+3 more) | DBError (sysstubs.c) +1456 |  |
| 22 | 66 | `0x004692c0` | 20 | unreferenced (swept) | RemoveGoals (savemisc2.c) -240 | DEAD |
| 22 | 51 | `0x0046aa30` | 21 | table at 0x004b9de4 in .data | ScriptEventDue (uimisc.c) -2000 |  |
| 22 | 56 | `0x0046b0c0` | 21 | table at 0x004b9e44 in .data | ScriptEventDue (uimisc.c) -320 |  |
| 22 | 61 | `0x0046b8c0` | 22 | called by 0x0047a650 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +352 |  |
| 22 | 61 | `0x0046b900` | 22 | called by 0x0047a6a0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +416 |  |
| 22 | 61 | `0x0046b940` | 22 | called by 0x0047a6f0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +480 |  |
| 22 | 61 | `0x0046b980` | 22 | called by 0x0047a7b0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +544 |  |
| 22 | 100 | `0x00474ed0` | 23 | called by InitTitleScreen (screens2.c) (+1 more) | BriefIconInput (screens3.c) -112 |  |
| 22 | 52 | `0x0048a040` | 29 | called by 0x00458b20 [unmatched] | AddInstanceToList (sweep4.c) +48 |  |
| 21 | 46 | `0x00426190` | 6 | called by Mat3_TransposeToMat4 [declared in coaster9.c] | MatMul (coastermath.c) +112 |  |
| 21 | 46 | `0x00426460` | 6 | called by 0x0042a680 [unmatched] | Mat3_ToMat4 (coaster7.c) -48 |  |
| 21 | 66 | `0x00469140` | 20 | called by 0x0046af60 [unmatched] | RemoveGoals (savemisc2.c) -624 |  |
| 21 | 66 | `0x00469190` | 20 | called by 0x0046af60 [unmatched] | RemoveGoals (savemisc2.c) -544 |  |
| 21 | 51 | `0x004785d0` | 24 | called by 0x00478a80 [unmatched] (+4 more) | RequestRoute (simcore.c) +2560 |  |
| 21 | 57 | `0x00483160` | 28 | called by 0x00483300 [unmatched] | RenderPeople (renderlist.c) +48 |  |
| 21 | 51 | `0x00483850` | 28 | called by 0x00484630 [unmatched] (+1 more) | sub_483830 (bnvpath.c) +32 |  |
| 21 | 51 | `0x004841a0` | 29 | called by 0x004845d0 [unmatched] (+4 more) | GetTileInDir (pathtile2.c) -1280 |  |
| 21 | 74 | `0x00490800` | 30 | called by SetInfoPanelText (movie.c) | FreeHelpTextBuffer (tinystubs.c) -80 | declared LoadHintTextFor in movie.c |
| 21 | 53 | `0x00497e40` | 30 | unreferenced (swept) | ShowLayer (blokelist.c) +48 | DEAD |
| 20 | 58 | `0x00450a40` | 13 | called by 0x00458ee0 [unmatched] | SaveBuildSlots (savegame2.c) -64 |  |
| 20 | 71 | `0x00453c20` | 14 | unreferenced (swept) | __DEBUG_FREE (memdb.c) +48 | DEAD |
| 20 | 56 | `0x004597e0` | 17 | called by 0x0044dc70 [unmatched] (+1 more) | EndLevel (uimisc.c) -64 |  |
| 20 | 59 | `0x00468d80` | 20 | called by 0x0046aae0 [unmatched] (+3 more) | ResetScriptTimer (tinystubs.c) +128 |  |
| 20 | 59 | `0x00468f40` | 20 | called by 0x0046aa30 [unmatched] | ResetScriptTimer (tinystubs.c) +576 |  |
| 20 | 59 | `0x00469100` | 20 | called by 0x0046ae70 [unmatched] | RemoveGoals (savemisc2.c) -688 |  |
| 20 | 59 | `0x004691e0` | 20 | called by 0x0046b080 [unmatched] (+1 more) | RemoveGoals (savemisc2.c) -464 |  |
| 20 | 59 | `0x00469220` | 20 | called by 0x0046ac50 [unmatched] (+1 more) | RemoveGoals (savemisc2.c) -400 |  |
| 20 | 59 | `0x00469310` | 20 | called by 0x0046adf0 [unmatched] (+4 more) | RemoveGoals (savemisc2.c) -160 |  |
| 20 | 57 | `0x0047a440` | 26 | table at 0x004bb914 in .data | LoadLevelDatabase (movie.c) -2928 |  |
| 20 | 51 | `0x0048a800` | 29 | called by ProfileSlotInput (screens3.c) (+1 more) | FreePlayItemUpdate (fpui5.c) -64 |  |
| 19 | 52 | `0x00426700` | 6 | called by RouteNode_UpdateClipRect (coaster9.c) | ClipRect_Unlink (coaster9.c) +32 | declared ClipRect_SetBounds in coaster9.c |
| 19 | 50 | `0x0042a5e0` | 7 | called by 0x0041e8f0 [unmatched] | TrackCursor_Init (coastertiny.c) -64 |  |
| 19 | 59 | `0x00468dc0` | 20 | called by 0x0046a750 [unmatched] (+1 more) | ResetScriptTimer (tinystubs.c) +192 |  |
| 19 | 59 | `0x00468e00` | 20 | called by 0x0046a750 [unmatched] | ResetScriptTimer (tinystubs.c) +256 |  |
| 19 | 54 | `0x00469000` | 20 | called by 0x0046aec0 [unmatched] | ResetScriptTimer (tinystubs.c) +768 |  |
| 19 | 54 | `0x00469080` | 20 | called by 0x0046af10 [unmatched] | RemoveGoals (savemisc2.c) -816 |  |
| 19 | 55 | `0x0046a1f0` | 20 | table at 0x004b9d88 in .data | UpdateGoalHelpText (softblit.c) +3568 |  |
| 19 | 59 | `0x0046b880` | 21 | called by 0x0047a5a0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +288 |  |
| 19 | 59 | `0x0046bad0` | 22 | called by 0x00479060 [unmatched] (+1 more) | RestoreScriptStepHelp (sysstubs.c) +880 |  |
| 19 | 46 | `0x00478770` | 24 | called by 0x0047a5a0 [unmatched] (+1 more) | RequestRoute (simcore.c) +2976 |  |
| 19 | 41 | `0x004802c0` | 27 | called by 0x00480330 [unmatched] | LoadMIDIFile (music.c) +192 |  |
| 19 | 49 | `0x004838e0` | 28 | table at 0x004bd360 in .data | DoRndWalkPathTileAction (bnvmove.c) -64 |  |
| 18 | 66 | `0x00426510` | 6 | called by Coaster3D_DrawModel (coaster9.c) | MakeTransform (coastermath.c) +48 | declared Mat3_TransposeToMat4 in coaster9.c |
| 18 | 52 | `0x00468f00` | 20 | called by 0x0046a960 [unmatched] | ResetScriptTimer (tinystubs.c) +512 |  |
| 18 | 52 | `0x00468f80` | 20 | called by 0x0046abd0 [unmatched] | ResetScriptTimer (tinystubs.c) +640 |  |
| 18 | 52 | `0x00468fc0` | 20 | called by 0x0046aec0 [unmatched] | ResetScriptTimer (tinystubs.c) +704 |  |
| 18 | 52 | `0x00469040` | 20 | called by 0x0046af10 [unmatched] | ResetScriptTimer (tinystubs.c) +832 |  |
| 18 | 52 | `0x004690c0` | 20 | called by 0x0046ae40 [unmatched] | RemoveGoals (savemisc2.c) -752 |  |
| 18 | 52 | `0x00469350` | 20 | called by 0x0046ad30 [unmatched] | RemoveGoals (savemisc2.c) -96 |  |
| 18 | 62 | `0x00469f80` | 20 | table at 0x004b9d74 in .data | UpdateGoalHelpText (softblit.c) +2944 |  |
| 18 | 46 | `0x0046ae40` | 21 | table at 0x004b9e24 in .data | ScriptEventDue (uimisc.c) -960 |  |
| 18 | 47 | `0x0046b100` | 21 | table at 0x004b9e48 in .data | ScriptEventDue (uimisc.c) -256 |  |
| 18 | 50 | `0x0046b9f0` | 22 | called by 0x0047a860 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +656 |  |
| 18 | 55 | `0x0046be00` | 22 | called by 0x00479270 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1696 |  |
| 18 | 54 | `0x00478650` | 24 | called by 0x00478a40 [unmatched] (+1 more) | RequestRoute (simcore.c) +2688 |  |
| 18 | 50 | `0x00478980` | 24 | table at 0x004bb71c in .data | RequestRoute (simcore.c) +3504 |  |
| 18 | 50 | `0x004789c0` | 24 | table at 0x004bb724 in .data | RequestRoute (simcore.c) +3568 |  |
| 18 | 52 | `0x004841e0` | 29 | unreferenced (swept) | GetTileInDir (pathtile2.c) -1216 | DEAD |
| 18 | 59 | `0x0048cc90` | 29 | unreferenced (swept) | DeleteIconInput (screens3.c) +96 | DEAD |
| 18 | 59 | `0x0048ccd0` | 29 | unreferenced (swept) | LightUpthisDeleteIcon (screens2.c) -128 | DEAD |
| 18 | 59 | `0x0048cd10` | 29 | unreferenced (swept) | LightUpthisDeleteIcon (screens2.c) -64 | DEAD |
| 18 | 78 | `0x0048ef40` | 30 | unreferenced (swept) | ExitOkInput (screens3.c) -80 | DEAD |
| 18 | 70 | `0x0048f9f0` | 30 | called by TitleMovieInput (screens3.c) | RestoreFrontEndState (movie.c) -80 |  |
| 17 | 53 | `0x00443f90` | 9 | called by 0x00444020 [unmatched] | SetAdvisorPose (tinystubs.c) -224 |  |
| 17 | 71 | `0x00444020` | 9 | pointer in 0x00444090 [unmatched] | SetAdvisorPose (tinystubs.c) -80 |  |
| 17 | 54 | `0x0044db40` | 12 | called by 0x0046a040 [unmatched] (+1 more) | LoadBinV (rin.c) -336 |  |
| 17 | 86 | `0x00462e90` | 17 | called by ResetLevelGlobals [declared in movie.c] | DoMapAI (bigsim.c) -96 |  |
| 17 | 43 | `0x0046b9c0` | 22 | called by 0x0047a800 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +608 |  |
| 17 | 43 | `0x0046ba30` | 22 | called by 0x0047a8a0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +720 |  |
| 17 | 43 | `0x0046bd10` | 22 | called by 0x00478e20 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1456 |  |
| 17 | 43 | `0x0046bd40` | 22 | called by 0x00478e90 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1504 |  |
| 17 | 54 | `0x0046bef0` | 22 | called by 0x00479450 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1616 |  |
| 17 | 54 | `0x0046bfb0` | 22 | called by 0x004795c0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1424 |  |
| 17 | 54 | `0x0046bff0` | 22 | called by 0x00479640 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1360 |  |
| 17 | 54 | `0x0046c350` | 22 | called by 0x00479e00 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -496 |  |
| 17 | 113 | `0x00482d70` | 28 | called by ResetLevelGlobals [declared in movie.c] | GetBlokeMood (simcore2.c) +64 |  |
| 17 | 55 | `0x00489f90` | 29 | called by 0x0044f4a0 [unmatched] | UnmarkObjectTiles (pathmisc2.c) +64 |  |
| 17 | 85 | `0x0048f4f0` | 30 | unreferenced (swept) | ExitCloseInput (screens3.c) +64 | DEAD |
| 17 | 37 | `0x0049c110` | 31 | unreferenced (swept) | SaveGardeners (savechunks.c) -48 | DEAD |
| 16 | 44 | `0x00429b60` | 7 | called by 0x0042a680 [unmatched] | TrackCurve_EvaluateOffset (coaster9.c) -80 |  |
| 16 | 50 | `0x00441830` | 9 | called by ObjNextRider (texture.c) (+1 more) | Render_SetViewport (sweep1.c) +48 | declared RiderCursorSeek in texture.c |
| 16 | 54 | `0x00444470` | 9 | table at 0x004b7e48 in .data | LoadReport (uimisc.c) +528 |  |
| 16 | 54 | `0x004444b0` | 9 | table at 0x004b7e4c in .data | LoadReport (uimisc.c) +592 |  |
| 16 | 55 | `0x004444f0` | 9 | table at 0x004b7e50 in .data | LoadReport (uimisc.c) +656 |  |
| 16 | 55 | `0x00444530` | 9 | table at 0x004b7e54 in .data | LoadReport (uimisc.c) +720 |  |
| 16 | 55 | `0x00444570` | 9 | table at 0x004b7e58 in .data | LoadReport (uimisc.c) +784 |  |
| 16 | 36 | `0x004636c0` | 18 | called by 0x004453a0 [unmatched] | InstallDirectDraw (sweep2.c) -48 |  |
| 16 | 44 | `0x00469b20` | 20 | table at 0x004b9d4c in .data | UpdateGoalHelpText (softblit.c) +1824 |  |
| 16 | 51 | `0x0046ba90` | 22 | called by 0x0047a960 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +816 |  |
| 16 | 51 | `0x0046bb40` | 22 | called by 0x0047ac00 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +992 |  |
| 16 | 46 | `0x0046f860` | 23 | unreferenced (swept) | AddFreePlayIcon (fpui.c) +192 | DEAD |
| 16 | 54 | `0x00476140` | 23 | called by 0x00468860 [unmatched] | UpdateThemeIconsFromProfile (screens3.c) -64 |  |
| 16 | 50 | `0x00478610` | 24 | called by 0x00478a40 [unmatched] (+4 more) | RequestRoute (simcore.c) +2624 |  |
| 16 | 45 | `0x0047af50` | 27 | table at 0x004bb9d4 in .data | LoadLevelDatabase (movie.c) -96 |  |
| 16 | 45 | `0x0047af80` | 27 | table at 0x004bb9dc in .data | LoadLevelDatabase (movie.c) -48 |  |
| 16 | 51 | `0x00489fd0` | 29 | called by 0x0046ac50 [unmatched] | AddInstanceToList (sweep4.c) -64 |  |
| 16 | 59 | `0x0048fe70` | 30 | unreferenced (swept) | TitleNewInput (screens3.c) -64 | DEAD |
| 16 | 48 | `0x00490740` | 30 | called by LoadHelpTextFor (movie.c) | LoadHelpTextFor (movie.c) -96 | declared SetHelpTextPrefix in movie.c |
| 16 | 48 | `0x00490770` | 30 | called by LoadHintTextFor [declared in movie.c] | LoadHelpTextFor (movie.c) -48 |  |
| 16 | 65 | `0x00491b80` | 30 | unreferenced (swept) | DeleteProfileList (listdel.c) +48 | DEAD |
| 15 | 39 | `0x0040cf50` | 1 | called by 0x0040d900 [unmatched] (+1 more) | LFPiece_ScreenPos (logflume2.c) -128 |  |
| 15 | 41 | `0x0040cfa0` | 1 | called by 0x0040d900 [unmatched] (+1 more) | LFPiece_ScreenPos (logflume2.c) -48 |  |
| 15 | 44 | `0x00443fe0` | 9 | called by 0x00444020 [unmatched] | SetAdvisorPose (tinystubs.c) -144 |  |
| 15 | 46 | `0x004442c0` | 9 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +96 |  |
| 15 | 46 | `0x004442f0` | 9 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +144 |  |
| 15 | 46 | `0x00444320` | 9 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +192 |  |
| 15 | 46 | `0x00444350` | 9 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +240 |  |
| 15 | 46 | `0x00444380` | 9 | called by 0x004453a0 [unmatched] | LoadReport (uimisc.c) +288 |  |
| 15 | 55 | `0x004445b0` | 9 | table at 0x004b7e5c in .data | LoadReport (uimisc.c) +848 |  |
| 15 | 55 | `0x004445f0` | 9 | table at 0x004b7e60 in .data | LoadReport (uimisc.c) +912 |  |
| 15 | 38 | `0x0044f3d0` | 12 | called by RemoveBlokeFromRide (rides.c) (+1 more) | PutBlokeInList (sweep1.c) -96 |  |
| 15 | 40 | `0x0045e930` | 17 | called by 0x0045e960 [unmatched] | ClearObjectUserFlags (objrect.c) +224 | DEAD |
| 15 | 37 | `0x00468cd0` | 20 | called by 0x00468d30 [unmatched] (+22 more) | ResetScriptTimer (tinystubs.c) -48 |  |
| 15 | 40 | `0x0046ad00` | 21 | table at 0x004b9e04 in .data | ScriptEventDue (uimisc.c) -1280 |  |
| 15 | 43 | `0x0046ad30` | 21 | table at 0x004b9e08 in .data | ScriptEventDue (uimisc.c) -1232 |  |
| 15 | 40 | `0x0046ad60` | 21 | table at 0x004b9e0c in .data | ScriptEventDue (uimisc.c) -1184 |  |
| 15 | 40 | `0x0046ad90` | 21 | table at 0x004b9e10 in .data | ScriptEventDue (uimisc.c) -1136 |  |
| 15 | 40 | `0x0046adc0` | 21 | table at 0x004b9e14 in .data | ScriptEventDue (uimisc.c) -1088 |  |
| 15 | 40 | `0x0046adf0` | 21 | table at 0x004b9e18 in .data | ScriptEventDue (uimisc.c) -1040 |  |
| 15 | 39 | `0x0046b080` | 21 | table at 0x004b9e3c in .data | ScriptEventDue (uimisc.c) -384 |  |
| 15 | 45 | `0x0046bda0` | 22 | called by 0x00478d30 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1600 |  |
| 15 | 47 | `0x0046bdd0` | 22 | called by 0x004791f0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1648 |  |
| 15 | 47 | `0x0046bf80` | 22 | called by 0x00479550 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1472 |  |
| 15 | 47 | `0x0046c030` | 22 | called by 0x004796d0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1296 |  |
| 15 | 47 | `0x0046c060` | 22 | called by 0x00479740 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1248 |  |
| 15 | 47 | `0x0046c0c0` | 22 | called by 0x00479850 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1152 |  |
| 15 | 47 | `0x0046c0f0` | 22 | called by 0x004798c0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1104 |  |
| 15 | 47 | `0x0046c2c0` | 22 | called by 0x00479d00 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -640 |  |
| 15 | 47 | `0x0046c390` | 22 | called by 0x00479e80 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -432 |  |
| 15 | 47 | `0x0046c3f0` | 22 | called by 0x00479f30 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -336 |  |
| 15 | 47 | `0x0046c420` | 22 | called by 0x00479fa0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -288 |  |
| 15 | 72 | `0x0046d2f0` | 22 | called by 0x0046cff0 [unmatched] | ShowObjectHelp (uimisc.c) -80 |  |
| 15 | 40 | `0x0048a750` | 29 | called by 0x00458be0 [unmatched] (+2 more) | RestoreFreePlaySelections (uimisc.c) -64 |  |
| 15 | 65 | `0x0048e450` | 30 | pointer in 0x0048c860 [unmatched] | SaveSlotInput (screens3.c) -80 |  |
| 15 | 39 | `0x00497f60` | 30 | called by 0x00498000 [unmatched] | TellAllLayersToStopAnimating (sprite2.c) +64 |  |
| 15 | 39 | `0x004981e0` | 31 | called by 0x00498250 [unmatched] | RewindNarrationSource (tinystubs.c) +192 |  |
| 14 | 32 | `0x0040cf80` | 1 | called by 0x0040d900 [unmatched] (+1 more) | LFPiece_ScreenPos (logflume2.c) -80 |  |
| 14 | 34 | `0x0041e670` | 3 | called by RouteNode_AddPending (coaster9.c) | RouteNode_IsPending (coaster9.c) +16 | declared RouteNode_CanAdd in coaster9.c |
| 14 | 30 | `0x0041e7c0` | 3 | unreferenced (swept) | RouteNode_GetAcceleration (coastertiny.c) -32 | DEAD |
| 14 | 35 | `0x0041f350` | 3 | unreferenced (swept) | Span_SetClip (coaster7.c) -48 | DEAD |
| 14 | 45 | `0x00426680` | 6 | unreferenced (swept) | AnyCoasterRegionFullyInside (schoolcar.c) +48 | DEAD |
| 14 | 61 | `0x00434820` | 8 | unreferenced (swept) | JcMonkeyFish_GetDrawDesc (screencb2.c) +224 | DEAD |
| 14 | 48 | `0x004443b0` | 9 | table at 0x004b7e38 in .data | LoadReport (uimisc.c) +336 |  |
| 14 | 48 | `0x004443e0` | 9 | table at 0x004b7e3c in .data | LoadReport (uimisc.c) +384 |  |
| 14 | 48 | `0x00444410` | 9 | table at 0x004b7e40 in .data | LoadReport (uimisc.c) +432 |  |
| 14 | 48 | `0x00444440` | 9 | table at 0x004b7e44 in .data | LoadReport (uimisc.c) +480 |  |
| 14 | 60 | `0x00444eb0` | 10 | called by 0x00444ef0 [unmatched] (+1 more) | LoadReport (uimisc.c) +3152 |  |
| 14 | 34 | `0x0044f400` | 12 | called by 0x0044f610 [unmatched] | PutBlokeInList (sweep1.c) -48 |  |
| 14 | 27 | `0x004548f0` | 14 | called by 0x00453da0 [unmatched] | LoadBubbleHelpGFX (text.c) -32 |  |
| 14 | 45 | `0x00469a80` | 20 | called by 0x00469b50 [unmatched] | UpdateGoalHelpText (softblit.c) +1664 |  |
| 14 | 38 | `0x0046abd0` | 21 | table at 0x004b9df8 in .data | ScriptEventDue (uimisc.c) -1584 |  |
| 14 | 44 | `0x0046b790` | 21 | called by 0x0047a480 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +48 |  |
| 14 | 44 | `0x0046b7c0` | 21 | unreferenced (swept) | RestoreScriptStepHelp (sysstubs.c) +96 | DEAD |
| 14 | 44 | `0x0046ba60` | 22 | called by 0x0047a8e0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +768 |  |
| 14 | 44 | `0x0046bb10` | 22 | called by 0x00478f00 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +944 |  |
| 14 | 44 | `0x0046bb80` | 22 | called by 0x0047ad40 [unmatched] (+1 more) | RestoreScriptStepHelp (sysstubs.c) +1056 |  |
| 14 | 44 | `0x0046bbb0` | 22 | called by 0x0047ab00 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1104 |  |
| 14 | 44 | `0x0046bbe0` | 22 | called by 0x0047ab80 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1152 |  |
| 14 | 44 | `0x0046bc80` | 22 | called by 0x0047a020 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1312 |  |
| 14 | 44 | `0x0046bcb0` | 22 | called by 0x0047a0b0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1360 |  |
| 14 | 44 | `0x0046bce0` | 22 | called by 0x0047a140 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1408 |  |
| 14 | 44 | `0x0046bd70` | 22 | called by 0x0047aea0 [unmatched] (+1 more) | RestoreScriptStepHelp (sysstubs.c) +1552 |  |
| 14 | 26 | `0x004787b0` | 24 | table at 0x004bb6fc in .data | RequestRoute (simcore.c) +3040 |  |
| 13 | 34 | `0x00411f70` | 3 | unreferenced (swept) | LFQueue_AddRider (lfentrance.c) +80 | DEAD |
| 13 | 32 | `0x0041f4c0` | 3 | called by 0x0041f4e0 [unmatched] | Span_SetClip (coaster7.c) +320 |  |
| 13 | 55 | `0x00444630` | 9 | table at 0x004b7e64 in .data | LoadReport (uimisc.c) +976 |  |
| 13 | 55 | `0x00444670` | 9 | table at 0x004b7e68 in .data | LoadReport (uimisc.c) +1040 |  |
| 13 | 55 | `0x004446b0` | 9 | table at 0x004b7e6c in .data | LoadReport (uimisc.c) +1104 |  |
| 13 | 55 | `0x004446f0` | 9 | table at 0x004b7e70 in .data | LoadReport (uimisc.c) +1168 |  |
| 13 | 55 | `0x00444730` | 9 | table at 0x004b7e74 in .data | LoadReport (uimisc.c) +1232 |  |
| 13 | 55 | `0x00444770` | 9 | table at 0x004b7e78 in .data | LoadReport (uimisc.c) +1296 |  |
| 13 | 55 | `0x004447b0` | 9 | table at 0x004b7e7c in .data | LoadReport (uimisc.c) +1360 |  |
| 13 | 55 | `0x004447f0` | 9 | table at 0x004b7e80 in .data | LoadReport (uimisc.c) +1424 |  |
| 13 | 55 | `0x00444830` | 9 | table at 0x004b7e84 in .data | LoadReport (uimisc.c) +1488 |  |
| 13 | 55 | `0x00444870` | 9 | table at 0x004b7e88 in .data | LoadReport (uimisc.c) +1552 |  |
| 13 | 55 | `0x004448b0` | 9 | table at 0x004b7e8c in .data | LoadReport (uimisc.c) +1616 |  |
| 13 | 55 | `0x004448f0` | 9 | table at 0x004b7e90 in .data | LoadReport (uimisc.c) +1680 |  |
| 13 | 55 | `0x00444930` | 9 | table at 0x004b7e94 in .data | LoadReport (uimisc.c) +1744 |  |
| 13 | 55 | `0x00444970` | 9 | table at 0x004b7e98 in .data | LoadReport (uimisc.c) +1808 |  |
| 13 | 38 | `0x00444c40` | 10 | called by 0x00444c70 [unmatched] | LoadReport (uimisc.c) +2528 |  |
| 13 | 38 | `0x00468860` | 19 | called by 0x0047a020 [unmatched] (+1 more) | ClearScriptStateBytes (sysstubs.c) +32 |  |
| 13 | 46 | `0x00469ab0` | 20 | called by EnsureObjectClassLoaded [declared in movie.c] (+1 more) | UpdateGoalHelpText (softblit.c) +1712 |  |
| 13 | 40 | `0x0046be90` | 22 | called by 0x00479390 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1712 |  |
| 13 | 40 | `0x0046bec0` | 22 | called by 0x004793e0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1664 |  |
| 13 | 40 | `0x0046c090` | 22 | called by 0x00479800 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1200 |  |
| 13 | 40 | `0x0046c120` | 22 | called by 0x00479930 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1056 |  |
| 13 | 40 | `0x0046c150` | 22 | called by 0x00479980 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -1008 |  |
| 13 | 40 | `0x0046c180` | 22 | called by 0x004799d0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -960 |  |
| 13 | 40 | `0x0046c1b0` | 22 | called by 0x00479a20 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -912 |  |
| 13 | 40 | `0x0046c1e0` | 22 | called by 0x00479a70 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -864 |  |
| 13 | 40 | `0x0046c210` | 22 | called by 0x00479ac0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -816 |  |
| 13 | 40 | `0x0046c290` | 22 | called by 0x00479cb0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -688 |  |
| 13 | 40 | `0x0046c2f0` | 22 | called by 0x00479d60 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -592 |  |
| 13 | 40 | `0x0046c320` | 22 | called by 0x00479db0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -544 |  |
| 13 | 40 | `0x0046c3c0` | 22 | called by 0x00479ee0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -384 |  |
| 13 | 40 | `0x0046c450` | 22 | called by 0x0047a2f0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -240 |  |
| 13 | 40 | `0x0046c480` | 22 | called by 0x0047a360 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -192 |  |
| 13 | 40 | `0x0046c4b0` | 22 | called by 0x0047a3d0 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -144 |  |
| 13 | 40 | `0x0046c4e0` | 22 | unreferenced (swept) | EnqueueStepStartEvent (uimisc3.c) -96 | DEAD |
| 13 | 57 | `0x0046cb20` | 22 | called by 0x00458b20 [unmatched] | LoadScripts (savechunks.c) -64 |  |
| 13 | 29 | `0x004826f0` | 28 | unreferenced (swept) | FindPathLeg (workorder2.c) -32 | DEAD |
| 13 | 47 | `0x004831a0` | 28 | called by 0x00484090 [unmatched] (+4 more) | SetPathFlag (pathbuild.c) -48 |  |
| 13 | 45 | `0x0048ef10` | 30 | unreferenced (swept) | ExitOkInput (screens3.c) -128 | DEAD |
| 12 | 38 | `0x00403d60` | 1 | unreferenced (swept) | Copters_StepRider (ridetiny.c) +48 | DEAD |
| 12 | 42 | `0x00409a50` | 1 | called by 0x00409a90 [unmatched] | LFRoute_Join (logflume2.c) -288 |  |
| 12 | 32 | `0x00420fb0` | 5 | called by RouteNode_UpdateClipRect (coaster9.c) | Coaster3D_DrawModel (coaster9.c) +288 | declared CoasterModel_GetClipRect in coaster9.c |
| 12 | 33 | `0x00429ac0` | 7 | called by 0x00429b60 [unmatched] (+1 more) | LevelTrackRun (schoolcar.c) +144 |  |
| 12 | 34 | `0x0046a140` | 20 | called by 0x0047a960 [unmatched] (+1 more) | UpdateGoalHelpText (softblit.c) +3392 |  |
| 12 | 40 | `0x0046a300` | 20 | table at 0x004b9d90 in .data | UpdateGoalHelpText (softblit.c) +3840 |  |
| 12 | 32 | `0x0046a730` | 21 | called by 0x0046a750 [unmatched] | ScriptEventDue (uimisc.c) -2768 |  |
| 12 | 37 | `0x0046b7f0` | 21 | called by 0x0047a500 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +144 |  |
| 12 | 37 | `0x0046b820` | 21 | called by 0x0047a550 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +192 |  |
| 12 | 37 | `0x0046b850` | 21 | called by 0x00478c60 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +240 |  |
| 12 | 37 | `0x0046bc10` | 22 | called by 0x0047ada0 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1200 |  |
| 12 | 51 | `0x00480b70` | 28 | pointer in SetStandardCallbacks (sweep3.c) | LoadObjectClass (saveprof.c) +48 |  |
| 12 | 35 | `0x00482ec0` | 28 | called by 0x00459520 [unmatched] (+1 more) | NewBloke (blokeai.c) -48 |  |
| 12 | 32 | `0x00483110` | 28 | unreferenced (swept) | InitialiseBlokes (workers.c) +32 | DEAD |
| 12 | 40 | `0x0048e420` | 30 | called by SaveGameOkInput (screens3.c) (+2 more) | SaveSlotInput (screens3.c) -128 |  |
| 12 | 37 | `0x0048eac0` | 30 | called by VolMarkerInput (screens3.c) | InitOptionScreen (screens2.c) -160 |  |
| 12 | 35 | `0x0048eaf0` | 30 | called by VolDownInput (screens3.c) (+2 more) | InitOptionScreen (screens2.c) -112 |  |
| 12 | 40 | `0x0048fbd0` | 30 | unreferenced (swept) | ProcessScreenPopup (tinystubs.c) -48 | DEAD |
| 11 | 27 | `0x0041e640` | 3 | called by RouteNode_AddPending (coaster9.c) | RouteNode_ClearActive (coaster9.c) +16 | declared RouteNode_SetPending in coaster9.c |
| 11 | 31 | `0x00426230` | 6 | called by 0x00428b80 [unmatched] (+1 more) | TransformVerts (coaster3d.c) -32 | DEAD |
| 11 | 28 | `0x00429b90` | 7 | called by TrackCurve_EvaluateOffset (coaster9.c) (+1 more) | TrackCurve_EvaluateOffset (coaster9.c) -32 | declared TrackCurve_EvaluateUp in coaster9.c |
| 11 | 42 | `0x004594f0` | 17 | called by 0x00459520 [unmatched] | RunLevelEndSequence (uimisc2.c) -544 |  |
| 11 | 33 | `0x00468890` | 19 | called by 0x0047a0b0 [unmatched] (+1 more) | ClearScriptStateBytes (sysstubs.c) +80 |  |
| 11 | 30 | `0x0046a170` | 20 | table at 0x004b9da4 in .data | UpdateGoalHelpText (softblit.c) +3440 |  |
| 11 | 29 | `0x0046b630` | 21 | called by 0x0046bc40 [unmatched] (+30 more) | UnlinkScriptStep (uimisc3.c) +96 |  |
| 11 | 33 | `0x0046c510` | 22 | called by 0x0047a440 [unmatched] | EnqueueStepStartEvent (uimisc3.c) -48 |  |
| 11 | 22 | `0x00476050` | 23 | called by ResetLevelGlobals [declared in movie.c] | RenderIconsHook (tinystubs.c) +48 |  |
| 11 | 38 | `0x00483090` | 28 | called by 0x00458b20 [unmatched] | MakeBloke (blokeai.c) -48 |  |
| 11 | 36 | `0x004848e0` | 29 | table at 0x004bd380 in .data | Bloke_DoNothing (sweep3.c) -48 |  |
| 11 | 30 | `0x0048f5d0` | 30 | unreferenced (swept) | VolUpInput (screens3.c) -32 | DEAD |
| 11 | 30 | `0x004919a0` | 30 | called by 0x004917c0 [unmatched] (+3 more) | AddNodeToProfileList (profiles.c) -32 | DEAD |
| 10 | 24 | `0x0040cf30` | 1 | called by 0x0040d6f0 [unmatched] | LFPiece_ScreenPos (logflume2.c) -160 |  |
| 10 | 30 | `0x0041e7f0` | 3 | called by 0x0041db20 [unmatched] | RouteNode_GetAcceleration (coastertiny.c) +16 |  |
| 10 | 23 | `0x00428840` | 6 | called by 0x00428860 [unmatched] | InitTrackDrawModes (coaster4.c) +240 |  |
| 10 | 30 | `0x0046b610` | 21 | called by 0x0046c510 [unmatched] (+35 more) | UnlinkScriptStep (uimisc3.c) +64 |  |
| 10 | 30 | `0x0046bc40` | 22 | called by 0x0047af80 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1248 |  |
| 10 | 30 | `0x0046bc60` | 22 | called by 0x0047af50 [unmatched] | RestoreScriptStepHelp (sysstubs.c) +1280 |  |
| 10 | 38 | `0x004787f0` | 24 | called by 0x00478930 [unmatched] | RequestRoute (simcore.c) +3104 |  |
| 10 | 26 | `0x004838c0` | 28 | table at 0x004bd350 in .data | DoRndWalkPathTileAction (bnvmove.c) -96 |  |
| 9 | 30 | `0x0044dc70` | 12 | called by ResetLevelGlobals [declared in movie.c] (+1 more) | LoadBinV (rin.c) -32 |  |
| 9 | 27 | `0x00451480` | 13 | unreferenced (swept) | RES_EnsureMounted (sysmisc.c) -352 | DEAD |
| 9 | 34 | `0x00451f40` | 14 | called by 0x00459520 [unmatched] | ScrollFromKeys (mapscreen4.c) -48 |  |
| 9 | 32 | `0x00468d10` | 20 | called by 0x0046b180 [unmatched] (+26 more) | ResetScriptTimer (tinystubs.c) +16 |  |
| 9 | 26 | `0x00469c40` | 20 | table at 0x004b9d60 in .data | UpdateGoalHelpText (softblit.c) +2112 |  |
| 9 | 26 | `0x0046a120` | 20 | table at 0x004b9d80 in .data | UpdateGoalHelpText (softblit.c) +3360 |  |
| 9 | 26 | `0x0046a330` | 20 | table at 0x004b9d94 in .data | ScriptEventDue (uimisc.c) -3792 |  |
| 9 | 26 | `0x0046a350` | 20 | table at 0x004b9d98 in .data | ScriptEventDue (uimisc.c) -3760 |  |
| 9 | 26 | `0x0046a420` | 20 | table at 0x004b9da8 in .data | ScriptEventDue (uimisc.c) -3552 |  |
| 9 | 26 | `0x0046a440` | 21 | table at 0x004b9dac in .data | ScriptEventDue (uimisc.c) -3520 |  |
| 9 | 26 | `0x0046a460` | 21 | table at 0x004b9db0 in .data | ScriptEventDue (uimisc.c) -3488 |  |
| 9 | 26 | `0x0046a4c0` | 21 | table at 0x004b9dbc in .data | ScriptEventDue (uimisc.c) -3392 |  |
| 9 | 24 | `0x00478bc0` | 24 | table at 0x004bb794 in .data | RequestRoute (simcore.c) +4080 |  |
| 9 | 33 | `0x0048b690` | 29 | unreferenced (swept) | FreePlayInit_48b6c0 (tinystubs.c) -48 | DEAD |
| 9 | 19 | `0x0049c0f0` | 31 | unreferenced (swept) | SaveGardeners (savechunks.c) -80 | DEAD |
| 8 | 24 | `0x004225e0` | 5 | unreferenced (swept) | CoasterModel_GetMeshCount (coastertiny.c) +16 | DEAD |
| 8 | 27 | `0x004227a0` | 5 | unreferenced (swept) | LoadCoasterModelSet (schoolcar4.c) +224 | DEAD |
| 8 | 37 | `0x00423940` | 6 | table at 0x004b5b64 in .data | Castle_GetFirstCorner (coastertiny.c) -112 |  |
| 8 | 27 | `0x004286e0` | 6 | table at 0x004b5d44 in .data | DrawTrackNode (coaster.c) -32 |  |
| 8 | 31 | `0x004687f0` | 19 | called by 0x00478e20 [unmatched] (+1 more) | ClearScriptStateBytes (sysstubs.c) -80 |  |
| 8 | 31 | `0x00468810` | 19 | called by 0x00478e90 [unmatched] (+1 more) | ClearScriptStateBytes (sysstubs.c) -48 |  |
| 8 | 21 | `0x00469390` | 20 | called by 0x0046ab70 [unmatched] | RemoveGoals (savemisc2.c) -32 |  |
| 8 | 27 | `0x00469ae0` | 20 | called by 0x0046ae30 [unmatched] (+3 more) | UpdateGoalHelpText (softblit.c) +1760 |  |
| 8 | 27 | `0x00469b00` | 20 | called by 0x0046b1e0 [unmatched] | UpdateGoalHelpText (softblit.c) +1792 |  |
| 8 | 27 | `0x00469b90` | 20 | table at 0x004b9d58 in .data | UpdateGoalHelpText (softblit.c) +1936 |  |
| 8 | 25 | `0x0046a390` | 20 | table at 0x004b9dc4 in .data | ScriptEventDue (uimisc.c) -3696 |  |
| 8 | 26 | `0x00474090` | 23 | unreferenced (swept) | IsRShiftDown (sysstubs.c) +16 | DEAD |
| 8 | 25 | `0x00476030` | 23 | called by 0x00476050 [unmatched] (+1 more) | RenderIconsHook (tinystubs.c) +16 |  |
| 8 | 29 | `0x004787d0` | 24 | called by 0x00478930 [unmatched] (+1 more) | RequestRoute (simcore.c) +3072 |  |
| 8 | 36 | `0x00478840` | 24 | table at 0x004bb744 in .data | RequestRoute (simcore.c) +3184 |  |
| 8 | 23 | `0x00478870` | 24 | table at 0x004bb70c in .data | RequestRoute (simcore.c) +3232 |  |
| 8 | 26 | `0x004838a0` | 28 | table at 0x004bd34c in .data | sub_483830 (bnvpath.c) +112 |  |
| 7 | 19 | `0x0040b270` | 1 | called by 0x0040c250 [unmatched] | LFBoat_IsOnPiece (posstep.c) +96 | DEAD |
| 7 | 24 | `0x0040cf10` | 1 | called by 0x0040db00 [unmatched] (+2 more) | LFPiece_ScreenPos (logflume2.c) -192 |  |
| 7 | 22 | `0x004120e0` | 3 | unreferenced (swept) | BuildWalkPath (mappath.c) -32 | DEAD |
| 7 | 27 | `0x0041b130` | 3 | unreferenced (swept) | GetInterface (loaders.c) -32 | DEAD |
| 7 | 19 | `0x0041cd20` | 3 | called by 0x0041cd80 [unmatched] | TrackClass_GetWorldBounds (schoolcar8.c) +64 |  |
| 7 | 17 | `0x00423060` | 5 | unreferenced (swept) | CoasterShades_Init (schoolcar5.c) +128 | DEAD |
| 7 | 20 | `0x00427c70` | 6 | table at 0x004b5d78 in .data | TrackH_Update (coaster.c) -32 |  |
| 7 | 19 | `0x00434b20` | 8 | unreferenced (swept) | JcDeco_Remove (junglecruise.c) -32 | DEAD |
| 7 | 30 | `0x00458be0` | 16 | called by StopScript (screens3.c) | SetMapReady (sysstubs.c) +48 |  |
| 7 | 19 | `0x004688c0` | 19 | called by 0x0046b0c0 [unmatched] | ScriptState_NoOp (sysstubs.c) -32 |  |
| 7 | 17 | `0x004688f0` | 19 | called by 0x0047a140 [unmatched] (+1 more) | ScriptState_NoOp (sysstubs.c) +16 |  |
| 7 | 22 | `0x00469b50` | 20 | table at 0x004b9d50 in .data | UpdateGoalHelpText (softblit.c) +1872 |  |
| 7 | 22 | `0x00469b70` | 20 | table at 0x004b9d54 in .data | UpdateGoalHelpText (softblit.c) +1904 |  |
| 7 | 22 | `0x00469bb0` | 20 | table at 0x004b9d5c in .data | UpdateGoalHelpText (softblit.c) +1968 |  |
| 7 | 18 | `0x00469c60` | 20 | pointer in 0x00469c80 [unmatched] | UpdateGoalHelpText (softblit.c) +2144 |  |
| 7 | 22 | `0x0046a480` | 21 | table at 0x004b9db4 in .data | ScriptEventDue (uimisc.c) -3456 |  |
| 7 | 22 | `0x0046a4a0` | 21 | table at 0x004b9db8 in .data | ScriptEventDue (uimisc.c) -3424 |  |
| 7 | 18 | `0x004786a0` | 24 | called by 0x0047af80 [unmatched] (+8 more) | RequestRoute (simcore.c) +2768 |  |
| 7 | 22 | `0x00492c60` | 30 | called by 0x00459520 [unmatched] | KillSoundSampleSystem (lifecycle.c) +64 |  |
| 7 | 22 | `0x00492c80` | 30 | called by 0x00459520 [unmatched] | SetThemeInTransition (tinystubs.c) -32 |  |
| 7 | 16 | `0x004975a0` | 30 | unreferenced (swept) | NewSprite (sysstubs.c) +32 | DEAD |
| 7 | 23 | `0x00497f90` | 30 | called by 0x00498150 [unmatched] | TellAllLayersToStopAnimating (sprite2.c) +112 |  |
| 7 | 23 | `0x00498210` | 31 | called by ReadDecodedNarration [declared in movie.c] | RewindNarrationSource (tinystubs.c) +240 |  |
| 7 | 31 | `0x00499410` | 31 | called by StartFreePlayPark (uimisc2.c) (+2 more) | GetGameTimer (util.c) -32 | declared ResetGameClock in uimisc2.c |
| 6 | 13 | `0x0041d040` | 3 | unreferenced (swept) | FindTrackNodeAt (coaster.c) -32 | DEAD |
| 6 | 13 | `0x0041d050` | 3 | unreferenced (swept) | FindTrackNodeAt (coaster.c) -16 | DEAD |
| 6 | 23 | `0x00423970` | 6 | table at 0x004b5b68 in .data | Castle_GetFirstCorner (coastertiny.c) -64 |  |
| 6 | 19 | `0x00427a80` | 6 | table at 0x004b5d40 in .data | Track_Create (coaster.c) -32 |  |
| 6 | 17 | `0x00457870` | 15 | called by StartFreePlayPark (uimisc2.c) (+4 more) | BricksAreLimited (tinystubs.c) -32 | declared sub_457870 in uimisc2.c |
| 6 | 23 | `0x004598b0` | 17 | unreferenced (swept) | TallyFootprintCell (mapbuild.c) -32 | DEAD |
| 6 | 22 | `0x00462e50` | 17 | called by 0x0047ab00 [unmatched] (+1 more) | ResetMapAI (objmap.c) +128 |  |
| 6 | 22 | `0x00462e70` | 17 | called by 0x0047ab80 [unmatched] (+1 more) | DoMapAI (bigsim.c) -128 |  |
| 6 | 22 | `0x004735c0` | 23 | unreferenced (swept) | ResetHelpKeyCursor (tinystubs.c) +16 | DEAD |
| 6 | 21 | `0x00473640` | 23 | called by 0x00457a70 [unmatched] | ProcessHelpKeys (tinystubs.c) -32 |  |
| 6 | 16 | `0x00478690` | 24 | called by 0x004786c0 [unmatched] (+2 more) | RequestRoute (simcore.c) +2752 |  |
| 6 | 21 | `0x00478820` | 24 | unreferenced (swept) | RequestRoute (simcore.c) +3152 | DEAD |
| 6 | 19 | `0x00486520` | 29 | unreferenced (swept) | BuildRecipTable (tri3d.c) -32 | DEAD |
| 6 | 21 | `0x00489ee0` | 29 | called by StartFreePlayPark (uimisc2.c) (+2 more) | MarkObjectTiles (pathmisc.c) -32 | declared sub_489ee0 in uimisc2.c |
| 5 | 12 | `0x0041f710` | 4 | called by 0x0041f720 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) -320 | DEAD |
| 5 | 13 | `0x004207b0` | 4 | unreferenced (swept) | CoasterModel_LoadPalette (coastertiny.c) +16 | DEAD |
| 5 | 40 | `0x00443d90` | 9 | called by 0x00444090 [unmatched] | RenderAdvisorIcon (screens3.c) -160 |  |
| 5 | 16 | `0x004594e0` | 17 | called by 0x00459520 [unmatched] | RunLevelEndSequence (uimisc2.c) -560 |  |
| 5 | 21 | `0x00463560` | 18 | called by ResetLevelGlobals [declared in movie.c] | ProcessDamage (scrolltick.c) -32 |  |
| 5 | 14 | `0x00469f70` | 20 | table at 0x004b9d70 in .data | UpdateGoalHelpText (softblit.c) +2928 |  |
| 5 | 14 | `0x0046a030` | 20 | table at 0x004b9d7c in .data | UpdateGoalHelpText (softblit.c) +3120 |  |
| 5 | 20 | `0x0046a370` | 20 | table at 0x004b9d9c in .data | ScriptEventDue (uimisc.c) -3728 |  |
| 5 | 14 | `0x0046abc0` | 21 | table at 0x004b9df4 in .data | ScriptEventDue (uimisc.c) -1600 |  |
| 5 | 14 | `0x0046ae30` | 21 | table at 0x004b9e20 in .data | ScriptEventDue (uimisc.c) -976 |  |
| 5 | 14 | `0x0046b1e0` | 21 | table at 0x004b9e54 in .data | ScriptEventDue (uimisc.c) -32 |  |
| 5 | 18 | `0x0046ce00` | 22 | called by 0x0046cb20 [unmatched] | KillAdvisorHelp (sysstubs.c) -32 |  |
| 5 | 23 | `0x0048d470` | 29 | called by InitSavedGameScreen (bigscreens.c) | ProfileCloseInput (screens3.c) +32 |  |
| 5 | 23 | `0x0048d490` | 29 | called by SaveGameOkInput (screens3.c) (+1 more) | InitSavedGameScreen (bigscreens.c) -32 |  |
| 5 | 23 | `0x0048eb20` | 30 | called by InitOptionScreen (screens2.c) (+1 more) | InitOptionScreen (screens2.c) -64 |  |
| 5 | 23 | `0x0048eb40` | 30 | called by ExitCloseInput (screens3.c) (+1 more) | InitOptionScreen (screens2.c) -32 |  |
| 5 | 14 | `0x0048fc30` | 30 | called by InitTitleScreen (screens2.c) | InitTitleScreen (screens2.c) -16 |  |
| 5 | 14 | `0x00491540` | 30 | called by PrintProfileDetails (bigscreens.c) | UpDateCurrentSaveSlotInfo (profiles.c) -16 |  |
| 5 | 13 | `0x00492da0` | 30 | called by PlayMovie (uimisc2.c) | MusicThread (musicthread.c) -16 | declared RestartMusic in uimisc2.c |
| 5 | 18 | `0x00498100` | 30 | called by 0x00498000 [unmatched] | RewindNarrationSource (tinystubs.c) -32 |  |
| 5 | 19 | `0x00498230` | 31 | called by ReadDecodedNarration [declared in movie.c] | RewindNarrationSource (tinystubs.c) +272 |  |
| 4 | 20 | `0x0041f030` | 3 | called by 0x0041ef60 [unmatched] | SetSpanClip (coastermath.c) +272 |  |
| 4 | 9 | `0x0041f7e0` | 4 | pointer in 0x0041f7f0 [unmatched] | TrackCursor_AdvanceGeometry (schoolcar8.c) -112 | DEAD |
| 4 | 14 | `0x00434810` | 8 | unreferenced (swept) | JcMonkeyFish_GetDrawDesc (screencb2.c) +208 | DEAD |
| 4 | 13 | `0x0044db80` | 12 | called by ResetLevelGlobals [declared in movie.c] | LoadBinV (rin.c) -272 |  |
| 4 | 14 | `0x004514a0` | 13 | called by 0x00451210 [unmatched] | RES_EnsureMounted (sysmisc.c) -320 | DEAD |
| 4 | 31 | `0x00458bc0` | 16 | called by 0x00459520 [unmatched] | SetMapReady (sysstubs.c) +16 |  |
| 4 | 13 | `0x00468830` | 19 | called by ResetLevelGlobals [declared in movie.c] | ClearScriptStateBytes (sysstubs.c) -16 |  |
| 4 | 15 | `0x004771e0` | 24 | unreferenced (swept) | PlayMovie (uimisc2.c) -16 | DEAD |
| 4 | 13 | `0x00482a80` | 28 | tail-jumped from sub_4828f0 (screens3.c) | UpdateEntranceTile (objdoor.c) -16 |  |
| 4 | 16 | `0x00482d60` | 28 | called by 0x0047aa90 [unmatched] | GetBlokeMood (simcore2.c) +48 |  |
| 4 | 20 | `0x004969d0` | 30 | called by 0x00469c80 [unmatched] (+1 more) | UnSourcePlayableSample (audio2.c) -32 |  |
| 3 | 12 | `0x00420780` | 4 | called by 0x00428860 [unmatched] (+1 more) | FindCoasterPart (coastertiny.c) -16 |  |
| 3 | 10 | `0x00421530` | 5 | unreferenced (swept) | PhysVec_InitOps (schoolcar8.c) -16 | DEAD |
| 3 | 11 | `0x0044f170` | 12 | table at 0x004b836c in .data | IsObjectRunning (sysmisc3.c) -496 |  |
| 3 | 10 | `0x00457900` | 15 | called by 0x00478c60 [unmatched] (+2 more) | SaveCurrency (sysstubs.c) -16 |  |
| 3 | 11 | `0x0046a4e0` | 21 | table at 0x004b9dc0 in .data | ScriptEventDue (uimisc.c) -3360 |  |
| 3 | 16 | `0x004736e0` | 23 | unreferenced (swept) | ProcessHelpKeys (tinystubs.c) +128 | DEAD |
| 3 | 10 | `0x00476450` | 23 | unreferenced (swept) | OpenMovie (movie.c) -16 | DEAD |
| 3 | 8 | `0x004787a0` | 24 | called by 0x00478be0 [unmatched] (+2 more) | RequestRoute (simcore.c) +3024 |  |
| 3 | 12 | `0x0047f820` | 27 | called by InitExitCheckBox (screens2.c) | ResetSaveTimer (screens3.c) +16 |  |
| 3 | 11 | `0x00482b10` | 28 | called by ResetLevelGlobals [declared in movie.c] | GetEntranceTile (tinystubs.c) +16 |  |
| 3 | 9 | `0x00483890` | 28 | called by 0x00484790 [unmatched] | sub_483830 (bnvpath.c) +96 |  |
| 3 | 11 | `0x0049a4a0` | 31 | table at 0x004b83c8 in .data | Mechanic_Idle (blokemisc.c) -16 |  |
| 3 | 11 | `0x0049a4d0` | 31 | table at 0x004b83cc in .data | Gardener_Build (workers2.c) -16 |  |
| 2 | 6 | `0x00423930` | 6 | unreferenced (swept) | Castle_GetFirstCorner (coastertiny.c) -128 | DEAD |
| 2 | 6 | `0x00423990` | 6 | table at 0x004b5b6c in .data | Castle_GetFirstCorner (coastertiny.c) -32 |  |
| 2 | 7 | `0x004260e0` | 6 | unreferenced (swept) | MatIdentity (coastermath.c) -16 | DEAD |
| 2 | 7 | `0x0042a670` | 7 | called by 0x0042a680 [unmatched] | TrackCursor_Evaluate (coastertiny.c) +48 |  |
| 2 | 11 | `0x004441f0` | 9 | called by ResetLevelGlobals [declared in movie.c] | SaveReport (uimisc.c) -16 |  |
| 2 | 11 | `0x0044db20` | 12 | called by ResetLevelGlobals [declared in movie.c] (+1 more) | LoadBinV (rin.c) -368 |  |
| 2 | 6 | `0x0044db30` | 12 | unreferenced (swept) | LoadBinV (rin.c) -352 | DEAD |
| 2 | 6 | `0x0046ae20` | 21 | table at 0x004b9e1c in .data | ScriptEventDue (uimisc.c) -992 |  |
| 2 | 6 | `0x0046b0b0` | 21 | table at 0x004b9e40 in .data | ScriptEventDue (uimisc.c) -336 |  |
| 2 | 3 | `0x0046b1f0` | 21 | table at 0x004b9e58 in .data | ScriptEventDue (uimisc.c) -16 |  |
| 2 | 11 | `0x0046d390` | 22 | called by PlayReportHint (uimisc3.c) (+1 more) | SetHelpFaceTalking (tinystubs.c) -16 | declared SetHelpFaceState5 in uimisc3.c |
| 2 | 11 | `0x0046d3b0` | 22 | unreferenced (swept) | SetHelpFaceTalking (tinystubs.c) +16 | DEAD |
| 2 | 3 | `0x004755b0` | 23 | unreferenced (swept) | InsertObjectNode (fpui5.c) -16 | DEAD |
| 2 | 3 | `0x00476220` | 23 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) +16 | DEAD |
| 2 | 3 | `0x00476230` | 23 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) +32 | DEAD |
| 2 | 3 | `0x00476240` | 23 | unreferenced (swept) | DisableRAndDIcons (sweep3.c) +48 | DEAD |
| 2 | 6 | `0x0047f830` | 27 | called by 0x0047f880 [unmatched] | ResetSaveTimer (screens3.c) +32 |  |
| 2 | 6 | `0x0047f840` | 27 | called by 0x0047f880 [unmatched] | DebugFlush (sysstubs.c) -16 |  |
| 2 | 6 | `0x00481720` | 28 | called by 0x00459970 [unmatched] | NewPathSquare (pathsq.c) -16 |  |
| 2 | 10 | `0x00483100` | 28 | unreferenced (swept) | InitialiseBlokes (workers.c) +16 | DEAD |
| 2 | 3 | `0x00484940` | 29 | unreferenced (swept) | ApplyObjectOrientationToPerson (bnvpath.c) -16 | DEAD |
| 2 | 11 | `0x00492980` | 30 | called by LoadLevelDatabase (movie.c) | SetSampleVolume (audio3.c) -32 | declared SetSfxPaused in movie.c |
| 2 | 11 | `0x00492990` | 30 | called by LoadLevelDatabase (movie.c) | SetSampleVolume (audio3.c) -16 | declared ClearSfxPaused in movie.c |
| 1 | 1 | `0x00411e20` | 3 | unreferenced (swept) | LFQueue_Append (lfmisc.c) -16 | DEAD |
| 1 | 1 | `0x0041ef10` | 3 | unreferenced (swept) | RouteSystemInit (schoolcar.c) +16 | DEAD |
| 1 | 1 | `0x00420520` | 4 | unreferenced (swept) | CoasterModel_SetDirectory (coastertiny.c) -16 | DEAD |
| 1 | 1 | `0x00423750` | 6 | unreferenced (swept) | CoasterGeomInit (schoolcar.c) +16 | DEAD |
| 1 | 1 | `0x004239a0` | 6 | unreferenced (swept) | Castle_GetFirstCorner (coastertiny.c) -16 | DEAD |
| 1 | 1 | `0x004275b0` | 6 | table at 0x004b5d4c in .data | Track_Update (coaster.c) -32 |  |
| 1 | 1 | `0x004275c0` | 6 | table at 0x004b5d48 in .data | Track_Update (coaster.c) -16 |  |
| 1 | 1 | `0x004511e0` | 13 | unreferenced (swept) | RES_FindVolumeOnResPath (sysmisc2.c) +256 | DEAD |
| 1 | 1 | `0x004511f0` | 13 | called by 0x00451210 [unmatched] | RES_FindVolumeOnResPath (sysmisc2.c) +272 | DEAD |
| 1 | 1 | `0x00451200` | 13 | unreferenced (swept) | RES_FindVolumeOnResPath (sysmisc2.c) +288 | DEAD |
| 1 | 1 | `0x00466070` | 18 | unreferenced (swept) | FlipPrimary (sysmisc.c) -352 | DEAD |
| 1 | 1 | `0x004761e0` | 23 | unreferenced (swept) | Unload_RAndDCheckBox (sweep2.c) +16 | DEAD |
| 1 | 1 | `0x0047f860` | 27 | unreferenced (swept) | DebugFlush (sysstubs.c) +16 | DEAD |
| 1 | 1 | `0x004830e0` | 28 | called by 0x00483100 [unmatched] | InitialiseBlokes (workers.c) -16 | DEAD |
| 1 | 1 | `0x00486180` | 29 | unreferenced (swept) | FindShadedColour (tri3d.c) -16 | DEAD |
| 1 | 1 | `0x0048a780` | 29 | called by ResetTempProfile (saveprof.c) | RestoreFreePlaySelections (uimisc.c) -16 |  |
