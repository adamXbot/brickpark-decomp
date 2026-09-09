# scope LL16 — `RunAppraisalScreen` 0x004453a0

Branch `scope/LL16` from `0767e1a1`. New file `LEGOLAND/appraisalscreen.c`,
one function.

## Status

| | |
| --- | --- |
| address | `0x004453a0` |
| original | 8,085 instructions, 34,662 bytes, frame `0x23d4` |
| ours | 8,181 instructions, 34,928 bytes, frame **`0x23d4` (exact)** |
| first diverging index | 8 |
| mismatch | 7,845 of 8,085 |
| index-for-index `MATCH` | 240, in 104 runs, longest 17 |
| true LCS vs the whole original | 5,216/8,085 = **64.5%** (lane best) |
| `difflib` alignment | 55.1% (lane best) |
| audit | `[WIP]` `ESCAPES` (we are 96 instructions longer), file ends `PASS` |
| relocs | zero `MISMATCH` (a WIP body is skipped) |
| `/W3` | clean |

**READ THE CLOSING ASSESSMENT AT THE END OF THIS FILE FIRST.**  It states what
the function is, everything proved about it and about VC6 along the way, the
exact remaining deltas with their zones and instruction counts, and a ranked
list of what to try next with an explicit do-not-retry list.  The pass sections
below are the working record and are superseded by it where they disagree.

Not exact.  The whole body is transcribed -- all nine build sections, the
advice chain, the closing line and the hints, plus the render and input loops
-- and the frame is the original's to the byte.  Three instruction-count
residuals remain (section 9 at +2 per line, one tail-merge in the closing
block, and register naming in section 2 and at the guarded section heads) plus
one that costs no instructions at all: **the three-slot rotation** at the
bottom of the frame, worth ~6 LCS points and with no known lever.

## What the screen is

Three phases in one function:

1. **Build.** A stack array `RepLine lines[100]` at `[esp+0x94]`, stride
   `0x4c`, is filled with one record per report line. Straight-line code,
   one block per statistic, guarded by bits of the control word at
   `0x00665ff8`.
2. **Render.** A loop over `lines[0..n)` draws the lines of the current
   page: the tick/cross/bullet mark, the text through `NewPrintColoured`
   (0x00454d80) and, for graded lines, a bar through `DrawAppraisalBar`
   (0x00444a70).
3. **Input.** A frame loop that reads the game buttons, turns pages,
   plays a narration file per line and leaves when `g_report_open`
   (0x0081c038) goes to zero.

Return value: `1` if every line's `ok` field is non-zero, else `0`; and `0`
immediately if `ScriptRunning()` (0x0046b280) is true.

## The line record (0x4c bytes, 19 dwords)

| off | name | meaning |
| --- | --- | --- |
| +0x00 | `page` | `g_report_pages` when the line was written |
| +0x04 | `indent` | the running x indent; re-read on a page restart |
| +0x08 | `ok` | 1 tick, 0 cross, −1 / −2 / −3 the bullet forms |
| +0x0c | `step` | `rand() % 5` — which of the five mark sprites |
| +0x10 | `text` | `GetString(id)`, or a `sprintf` buffer |
| +0x14 | `colour` | passed to `NewPrintColoured` as its last argument |
| +0x18 | `bar` | 1: draw a bar for this line |
| +0x1c | `value` | the measured value (`DrawAppraisalBar`'s `value`) |
| +0x20 | `mark` | the goal (`DrawAppraisalBar`'s `mark`) |
| +0x24 | `range` | the bar's full scale (`DrawAppraisalBar`'s `range`) |
| +0x28 | `nids` | how many extra string ids follow |
| +0x2c..+0x48 | `ids[7]` | extra string ids, appended as the build finds them |

Evidence for +0x1c/+0x20/+0x24: the render loop at 0x0044d9c9 pushes
`[edi-0xc]`, `[edi-4]`, `[edi-8]` as `DrawAppraisalBar`'s `value`, `range`,
`mark` with `edi = &lines[i] + 0x28`. Evidence for +0x28/+0x2c: at
0x0044d418 the code forms `&lines[i-1].nids`, uses its value as an index
into the same record at +0x2c and post-increments it.

## The frame (0x23d4 = 9,172 bytes)

| off | size | what |
| --- | --- | --- |
| 0x00..0x0f | 16 | outgoing-argument / spill floor |
| 0x10 | 4 | `page_start` (also live in `ebp`; stored at every definition) |
| 0x14 | 4 | `indent` — never enregistered |
| 0x18 | 4 | the `n * 0x4c` byte-offset temporary |
| 0x1c..0x28 | 16 | `AppraisalBox box` — the page's first-line rectangle {0x50, 0x6d, 0x1a4, 0x83} |
| 0x2c..0x38 | 16 | `AppraisalBox cur` — the current print rectangle |
| 0x3c | 4 | `ok`, the current statistic's pass flag |
| 0x40, 0x44 | 8 | a per-section spare, `sect_start` |
| 0x48 | 4 | `failmask` (build) / the narration count (input loop) |
| 0x4c, 0x50 | 8 | `passed`, `total` for the section |
| 0x54..0x64 | 20 | narration cursor and the running `all_passed` / `all_total` |
| 0x68..0x90 | 44 | the `Count*` out-parameters |
| 0x94 | 7600 | `RepLine lines[100]` (ends 0x1e44) |
| 0x1e44 | 128 | `char namebuf[0x80]` — the narration file name |
| 0x1ec4 | 512 | `char textbuf[0x200]` — the `sprintf` line buffer |
| 0x20c4 | 784 | `int narr[196]` — the narration queue (ends 0x23d4) |

`0x94 + 100*0x4c + 0x80 + 0x200 + 0x310 = 0x23d4` exactly; `namebuf` is
pinned by `lea ecx,[esp+0x1e48]` at 0x0044da5d (one push pending) and
`textbuf` by `lea edx,[esp+0x1ec4]` at 0x0044a80e.

## The build grammar

Every graded statistic is the same five-part shape:

```c
if (FLAGS & <enable bits>) {
    total++;
    ok = <measure>() >= g_goal[k];
    if (ok) passed++; else failmask |= <bit>;
    switch ((FLAGS >> <shift>) & 3) {       /* three phrasings */
    case 1: LINE(idA); break;
    case 2: LINE(idB); break;
    case 3: LINE(idC); break;
    }
}
```

`LINE` is a page check followed by eleven record stores:

```c
if (y + 0x16 > 0x1b5) PAGE_BREAK(section_label);
lines[n].page   = g_report_pages;
lines[n].indent = indent;
lines[n].ok     = ok;
lines[n].step   = rand() % 5;
lines[n].text   = GetString(id);
lines[n].colour = 0;
lines[n].bar    = 0 or 1;
lines[n].value  = ...; lines[n].mark = ...; lines[n].range = ...;
lines[n].nids   = 0;
n++; y += 0x18;
```

The three `case` bodies share everything from `call GetString` onward — VC6
cross-jumps them to one tail (`jmp 0x00445c4c` and friends), so the case
blocks in the object are laid out 3, 2, 1 with case 3 falling through.

## The page break, and the section restart

```c
if (y + 0x16 > 0x1b5) {
    if (page_start != sect_start) {     /* the section started higher up */
        page_start = sect_start;
        n          = sect_start;        /* rewind: throw the part-page away */
        indent     = lines[sect_start].indent;
        g_report_pages++;
        goto <section label>;           /* re-emit the section on a new page */
    }
    g_report_pages++;                   /* the section alone fills a page */
    page_start = n;
    cur = box;
    y   = box.top;
}
```

The rewind arm is emitted once per section and every later line in that
section jumps back to it — 0x004464b7 has **seventeen** predecessors. Nine
sections were found by their `mov [esp+0x44], esi` (`sect_start = n`):
0x00445554, 0x0044676d, 0x00446bb0, 0x00446fea, 0x00447421, 0x0044789c,
0x00447e7a, 0x0044acbe.

**Original bug, reproduced:** the page check on the *first* line of section
two restarts at section **one**'s label (`jmp 0x00445422` at 0x0044547f),
not at section two's, so a section-two header that lands at the bottom of a
page re-emits the report's title line. Every other line of section two
restarts at 0x00445539 as expected. The two blocks cannot be a compiler
cross-jump: a merged tail must share its successor, and these do not.

## Sections and goals recovered so far

`g_appraisal_flags` = 0x00665ff8, `g_goal[]` = 0x0066600c.

| guard | statistic | goal | fail bit | phrasing shift | string ids |
| --- | --- | --- | --- | --- | --- |
| `& 0xf` | the report title | — | — | — | 0x12c |
| `& 0x4fff0` | section: what the park holds | — | — | — | 0x131 (header) |
| `& 0x4000` | attractions, count | `g_goal[5]`/`[6]` | 0x10 | — (bar) | 0x132 |
| `& 0x8000` | attractions, variety | `g_goal[7]`/`[8]` | 0x20 | — (bar) | 0x133 |
| `& 0x40000` | percent of objects on a path | `g_goal[9]`/`[10]` | 0x40 | — (bar) | 0x134 |
| `& 0x30` | castles | `g_goal[0]` | 0x80 | 4 | 0x135/6/7 |
| `& 0xc0` | driving schools | `g_goal[1]` | 0x100 | 6 | 0x138/9/a |
| `& 0x300` | log flumes | `g_goal[2]` | 0x200 | 8 | 0x13b/c/d |
| `& 0xc00` | boating schools | `g_goal[3]` | 0x400 | 10 | 0x13e/f/40 |
| `& 0x3000` | jungle cruises | `g_goal[4]` | 0x800 | 12 | 0x141/2/3 |
| `& 0x38000000` | section: scenery (`CountScenery`) | `g_goal[25]`.. | 0x1000.. | — | 0x144 header, 0x132/0x133 |
| `& 0xc0000000` | section: food (`CountFood`) | `g_goal[31]`.. | — | — | 0x145 header, 0x132/0x133 |

At the end of a section the header line's `ok` is set to
`passed == total`, `indent` drops back by 0x30 and the section's counts are
folded into the running `all_passed` / `all_total`.

## Levers learned

- **A statistic's three phrasings are a `switch` on a two-bit field.** The
  object shows `mov eax,[flags]; shr eax,K; and eax,3; dec/je; dec/je;
  dec/jne` — VC6's compare-chain lowering for `switch(x){case 1..3}` — and
  lays the case blocks out **3, 2, 1** with case 3 as the fall-through. The
  shared tail from `call GetString` onward is cross-jumped, not duplicated:
  two calls in the tail, so it is jumped to (BL05).
- **`rand() % 5` is a mark-sprite index, not a phrase index.** `cdq / mov
  ecx,5 / idiv ecx` and the remainder goes to `+0x0c`; `appraisal.c`'s
  `g_rep_mark[3][5]` is the five-step tick, cross and bullet.
- **The report phase's frame shape is decided by the render phase.** The
  build's page break reloads `box.left/top/right/bottom` from memory at all
  ~140 sites instead of rematerialising 0x50/0x6d/0x1a4/0x83, and keeps its
  apparently dead stores to `cur`. Measured: VC6 at `/O2` *always*
  constant-propagates and dead-store-eliminates a single-definition struct
  local, even at 3,477 instructions, even when its address is passed to an
  extern function (tested: by-value pass, `&box` pass, `AppraisalBox[2]`,
  a `goto` loop, and a 90-fold replicated body). The one spelling that
  reproduced the reloads was making the box a member of an aggregate whose
  address escapes. The likely original source is simpler: **`box` and `cur`
  are re-assigned in the render phase** (`cur` is the print rectangle —
  `[esp+0x2c]` is read at 0x0044d987 and `[esp+0x30]` is the render `y`,
  written at 0x0044da07), which gives their fields a second definition and
  turns constant propagation off for the build phase. Anyone continuing
  this function should write the render loop **before** trying to match the
  build phase; measuring the build alone is misleading.
- **`page_start` is stored to `[esp+0x10]` at all 133 of its definitions and
  reloaded only twice.** That is the signature of a variable VC6 has given a
  memory home with a cached register copy, not of two variables.
- **A `while`-free 8,000-instruction body still has only twenty backward
  jumps.** Nine of them are the per-section page restarts and the rest are
  the render and input loops; everything else is straight-line. Bounding
  such a body needs the widened walker in this worktree's `tools/match.py`.

## Extern type divergences

None yet: every callee is declared with the types `appraisal.c` uses.
`ReadGameButtons` (0x00452460) is named `ReadGameButton` in the scope brief
and `ReadGameButtons` in `bighelp.c`; this file uses `ReadGameButtons`.


## 2026-09-08 — the rest of the build, recovered and transcribed

`g_appraisal_flags` = 0x00665ff8, `g_goal[]` = 0x0066600c,
`g_num_visitors` = 0x00832bd0, `g_appraisal_rank` = 0x0083297c,
`g_appraisal_rank_bias` = 0x00832b9c.

### Sections 3-7 (all the same five-part shape as section 2)

| # | guard | header id | helper | statistics (guard, value, goal, range, id, fail bit) |
| --- | --- | --- | --- | --- |
| 3 scenery | `0x38000000` | 0x144 | `CountScenery(&num,&var)` | `0x8000000` num g\_goal[25]/[26] 0x132 bit 0x1000; `0x10000000` var g\_goal[27]/[28] 0x133 bit 0x2000 |
| 4 food | `0xc0000000` | 0x145 | `CountFood` | `0x40000000` num g\_goal[31]/[32] 0x132 bit 0x8000; `0x80000000` var g\_goal[33]/[34] 0x133 bit 0x10000 |
| 5 shops | `0x30000` | 0x146 | `CountShops` | `0x10000` num g\_goal[13]/[14] 0x132 bit 0x20000; `0x20000` var g\_goal[15]/[16] 0x133 bit 0x40000 |
| 6 visitors | `0x5080000` | 0x147 | `CountVisitors(&a,&b,&c)` | `0x80000` a vs g\_goal[11] **no line** bit 0x80000; `0x1000000` b g\_goal[17]/[18] 0x148 bit 0x100000; `0x4000000` c g\_goal[17]/[18] 0x149 bit 0x200000 |
| 7 the park at work | `0xe00000` | 0x14a | — | `0x200000` `g_num_visitors` g\_goal[19]/[20] 0x14b bit 0x400000; `0x400000` the running-object count g\_goal[21]/[22] 0x14c bit 0x800000; `0x800000` `MapCellCount()` g\_goal[23]/[24] 0x14d bit 0x1000000 |

**Original bugs reproduced.** Section 3's guard has a third bit
(`0x20000000`) with no statistic behind it, so fail bit `0x4000` is never
set. Section 6's first statistic advances `y` by a line but writes none,
leaving a blank row. Section 6's third statistic is graded against
`g_goal[17]/[18]`, the *second* statistic's goal, not its own.

Section 7's object count is an open-coded walk:

```c
nrun = 0;
obj = GetFirstRenderObject();
while (obj) {
    kind = obj->kind;                       /* a short at obj+4 */
    if (obj->p->q->type != 0 && obj->p->q->type != 2)
        if (IsObjectRunning(obj->p->q, &kind)) nrun++;
    obj = GetNextRenderObject(obj);
}
```

`obj->p` is at `obj+0`, `p->q` at `p+0x0c`, `q->type` a short at `q+0x20`.
`kind` is the only short local in the frame; it sits at `[esp+0x66]`.

### The out-parameter block, 0x68..0x90

Eleven address-taken ints, in the original's slot order:

| slot | 0x68 | 0x6c | 0x70 | 0x74 | 0x78 | 0x7c | 0x80 | 0x84 | 0x88 | 0x8c | 0x90 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| what | attr var | vis c | shop var | vis b | food var | scen var | scen num | food num | shop num | attr num | vis a |

Getting these eleven plus `textbuf` into the source is what took the frame
from `0x2190` to `0x23cc`; the surviving 8-byte shortfall is two scalars
still missing (see below).

### Section 8, the advice chain (0x00447e73, unguarded)

Header line `0x230` with `ok = -2`, then `indent += 0x30`, then a verdict
and one piece of advice per fail bit. `ok` is the bullet form: `-2` the
verdict lines, `-1` a piece of advice, `-3` its continuation.

```
all_passed <  all_total/2 : 0x14f +NARR, 0x150
all_passed <  all_total   : 0x151 +NARR, 0x150
else                      : 0x153 +NARR, 0x154
bit 0x1 0x155 | 0x2 0x156 | 0x4 0x157 | 0x8 0x158
(failmask & 0x30):    0x10 -> 0x159 | 0x20 -> 0x15a | 0x30 -> 0x15b, 0x133(-3)
bit 0x40  0x15c +NARR, 0x15d(-3)
bit 0x80  0x15e | 0x100 0x15f | 0x200 0x160 | 0x400 0x161 | 0x800 0x162
(failmask & 0x3000):  0x1000 -> 0x163 | 0x2000 -> 0x164 | 0x3000 -> 0x165
(failmask & 0x18000): 0x8000 -> 0x166 | 0x10000 -> 0x167 | 0x18000 -> 0x168
(failmask & 0x60000): 0x20000 -> 0x169 | 0x40000 -> 0x16a
                    | 0x60000 -> 0x16b +NARR, 0x231(-3)
bit 0x80000 0x16c | 0x100000 0x16d | 0x200000 0x16f | 0x400000 0x170
bit 0x800000 0x171 +NARR, 0x172(-3) | 0x1000000 0x173
```

String id `0x16e` is skipped: original.

**`NARR(id)`** is `lines[n-1].ids[lines[n-1].nids] = id; lines[n-1].nids++;`
— the object forms `&lines[n].nids` once (`lea eax,[esp+off+0xbc]`, kept in
a temp) and indexes the id array as `[esp + (19*n + nids)*4 + 0xc0]`.

### The closing "next time" block (0x0044a70c)

```c
if (failmask != 0 && g_appraisal_rank != 0) {
    if (g_appraisal_rank_bias < 0) v = g_appraisal_rank_bias + g_appraisal_rank - 1;
    else                           v = g_appraisal_rank - 1;
    if (v > 1) {
        sprintf(textbuf, GetString(0x235), GetString(v + 0x514));
        BUF_LINE(-2)  NARR(0x235) NARR(v+0x514) NARR(0x236)  LINE(-2, 0x236)
    } else if (v > 0) {  LINE(-2, 0x514) NARR(0x514)  LINE(-2, 0x236) }
    else              {  LINE(-2, 0x237) NARR(0x237)  LINE(-2, 0x238) }
}
indent -= 0x30;
```

Two original quirks here, both reproduced. **None of these five lines
advances `y`**, so they all land on the same row and each one's page check
re-reads the previous line's cached `cur.bottom` (`cmp [esp+0x38],0x1b5`)
instead of recomputing `y+0x16`. And their page-break arm restarts at
**section 9's** label, not section 8's, so a break here throws the whole
advice section away instead of re-emitting it.

### Section 9, the hints (0x0044acbb)

Header `0x174` (`ok = -2`) + `NARR`, then `nhint = 0`, `indent += 0x30`.
Each group picks its phrasing with a fresh `rand()`; **`nhint++` lives
inside each case, not after the switch** — the switch's default jumps past
it, which is how you can tell. Every hint line is also queued for
narration.

| group | selector | cases |
| --- | --- | --- |
| `failmask & 0xf` | `rand() & 3`, **jump table** at 0x0044db08 | 0: 0x17c,0x17d · 1: 0x187,0x188 · 2: 0x190,0x191,0x192 · 3: 0x19a,0x19b |
| `failmask & 0x70` | `rand() & 3`, compare chain | 0: 0x1a4,0x1a5,0x1a6 · 1: 0x1ae,0x1af,0x1b0 · 2: 0x1b8,0x1b9 |
| `failmask & 0x7000` | `rand() % 3` | 0: 0x1c2,0x1c3,0x1c4 · 1: 0x1cc,0x1cd |
| `failmask & 0x18000` | `rand() % 3` | 0: 0x1d6,0x1d7,0x1d8 · 1: 0x1e0,0x1e1 |
| `failmask & 0x260000` | `rand() % 3` | 0: *if* `failmask & 0x200000` 0x1ea,0x1eb,0x1ec · 1: 0x1f4,0x1f5 · 2: *if* `FLAGS & 0xf` 0x1fe,0x1ff |
| `failmask & 0x1080000` | `rand() & 1` | set: 0x208 · clear: 0x212,0x213 |
| `failmask & 0xc00000` | `rand() & 1` | set: *if* `failmask & 0x800000` 0x21c,0x21d · clear: *if* `failmask & 0x400000` 0x226,0x227 |

The build ends with `if (nhint == 0) n--;` at 0x0044d744 — the local the
earlier draft called `nclose` is this hint counter.

## Levers learned (2026-09-08)

- **The page check is `cur.bottom = y + 0x16; if (cur.bottom > 0x1b5)`.**
  Five sites compare `[esp+0x38]` (cur.bottom's home) against `0x1b5`
  instead of recomputing `lea eax,[edi+0x16]`, and they are exactly the
  sites where the previous line did not advance `y`. Writing that spelling
  into `PAGE_CHECK` was measured: it makes VC6 CSE **66 of the ~140** page
  checks into a memory reload, far more than the original's five, and
  drops the emitted count from 4966 to 4900. The inline `y + 0x16` form is
  kept; the five cached sites are a scheduling artefact, not the source.
- **`passed`/`total` move stack slots between sections** (0x4c/0x50 in
  section 2, 0x40/0x50 in section 3) and so does the `n*0x4c` byte-offset
  temp (0x18, then 0x4c, then 0x50, then `ebp` in section 9). VC6 is
  packing this frame's temps, so do not treat a slot as naming a variable
  across the whole body.
- **`failmask |= K` is spelled two ways by VC6**: `mov eax,[esp+0x48] / or
  al,K / mov [esp+0x48],eax` for K < 0x10000 and `or dword ptr
  [esp+0x48],K` above it. Same source.
- **A section header line's `ok` is `0` in section 2 but the live `ok`
  variable in sections 3-7.** Section 2's header runs before `ok` has ever
  been assigned, so the original really does write a literal 0 there and
  the later headers really do write the previous statistic's `ok`; both are
  overwritten by `lines[sect_start].ok = (passed == total)` at the section
  end.
- **`all_passed += passed` reads as `=` in section 2** because both are
  provably 0 there — write `+=` everywhere and let VC6 fold it.

## What is left

1. **The frame is 8 bytes short** (`0x23cc` vs `0x23d4`). The scalar area
   below `lines` is 0x8c in our object and 0x94 in the original, i.e. two
   more memory-homed dwords are wanted. The out-param block (0x68..0x90)
   and the narration cursor block (0x54..0x64) are both accounted for; the
   two missing slots are most likely in the render/input loop, which is
   still approximate.
2. **The render loop's walking pointer is not a free win — measured.**
   Rewriting the three render-phase walks with `RepLine*` cursors (plus an
   `int* np` for the narration queue and `do/while` rotations) moved the
   mismatch count only 8043 -> 8041 and took the **frame the wrong way**,
   0x23cc -> 0x23bc, because VC6 then enregisters `i` and `nnarr` which the
   original keeps at `[esp+0x5c]` and `[esp+0x48]`. Reverted; the indexed
   spelling is what is on disk. Anyone retrying this has to keep those two
   counters memory-homed. The original's addressing is:  the
   original keeps `edi = &lines[i].nids` (`lea edi,[esp+19*i*4+0xbc]`) and
   reads every field as a displacement off it: `[edi-0x28]` page,
   `[edi-0x24]` indent, `[edi-0x20]` ok, `[edi-0x1c]` step, `[edi-0x18]`
   text, `[edi-0x14]` colour, `[edi-0x10]` bar, `[edi]` nids, `[edi+4..]`
   ids. The narration queue cursor is a second walking pointer
   `ebp = &narr[nnarr]` (`lea ebp,[esp+nnarr*4+0x20c4]`). Our indexed
   spelling is roughly 500 instructions short of the original's 8,085.
3. **Register assignment.** The original wants `ebx` = the zero constant,
   `ebp` = `page_start`, `esi` = `n`, `edi` = `y`; ours currently puts the
   zero in `edi` and gives `page_start` no register. That is expected to
   settle once (1) and (2) are right — everything downstream of the
   prologue shifts with the frame.


## 2026-09-08 (second pass) — the frame is exact

`0x23cc` -> `0x23d4`.  Two changes, both measured, both forced by the same
diagnosis:

1. **`int narr[200]`, not `[196]`.**  The locals occupy
   `[esp+0x10, esp+0x10+0x23d4)` = `[0x10, 0x23e4)` in post-prologue
   coordinates (the four pushed registers are `[esp+0x00..0x0f]`), so the
   top-of-frame array runs `0x20c4..0x23e4` = `0x320` bytes = 200 ints.
2. **Do not zero `nnarr` and `narr_cur` at function entry.**  The draft had
   `nnarr = 0; narr_cur = 0;` in the entry block.  That gives both a live
   range starting at instruction 0, so VC6's stack packer cannot fold them
   onto a build-phase slot and they each cost a dword.  The original zeroes
   only five things at entry (`indent`, `page_start`, `all_total`,
   `all_passed`, `failmask` -- `mov [esp+0x14],ebx / [esp+0x10],ebp /
   [esp+0x60],ebx / [esp+0x5c],ebx / [esp+0x48],ebx`) and leaves the two
   narration counters undefined until the first page turn.  Dropping the two
   stores made VC6 pack `nnarr` onto `passed`/`nhint` and `narr_cur` onto the
   section pointer temp, exactly as the original does.

Net: scalar area `0x8c` -> `0x84` (35 -> 33 dwords) and the queue `0x310` ->
`0x320`; `lines`, `namebuf`, `textbuf` and `narr` now sit at the original's
`0x94`, `0x1e44`, `0x1ec4`, `0x20c4`.  First diverging index 0 -> 1, mismatch
8043 -> 8029, `FULL MATCH` 32.5% -> 36.9%.

Also landed: the render loop writes `cur.left` directly instead of through a
separate `x` local, which is what the original does (`mov [esp+0x2c],eax`
then `add eax,-0x28` for `BlitAppraisalSprite`).

### The tool that made this tractable: `/FAs`

`cl /nologo /c /W3 /O2 /Gy /Gd /FAs /Fa<out>.asm` emits an assembly listing
whose head carries **one equate per named local** (`_lines$ = -9024`,
`_page_start$ = -9164`, ...), and CSE temps appear in the body as
`-9152+[esp+9180]`.  Convert with `esp_off = frame_size + equate + 16`.
That gives the whole frame map with names in one compile -- no esp tracking,
no guessing -- and two locals sharing an equate is VC6 telling you it packed
them.  A helper is worth keeping around:

```
grep -E '^_\w+\$ = ' listing.asm      # named slots
grep -oE '(-9[0-9]+)\+\[esp' listing.asm | sort -u   # the CSE temps
```

Measured with it: **declaration order does not affect the packing at all**
(moving `nnarr, narr_cur` next to `failmask` changed nothing), and neither
does renaming.  Only live ranges matter.

### The two frames side by side

| slot | original | ours now |
| --- | --- | --- |
| 0x10 | `page_start` | `page_start` |
| 0x14 | `indent` | `nrun` + temp |
| 0x18 | `nrun`/`nhint` | `indent`/`total` |
| 0x1c..0x28 | `box` | `n*0x4c` temp |
| 0x20 | | `ok`/`obj` + temp |
| 0x24 | | `sect_start` |
| 0x28..0x34 | | `cur` |
| 0x2c..0x38 | `cur` | |
| 0x38 | | `passed`/`nhint`/`nnarr` |
| 0x3c | `ok`/`obj` | `failmask` |
| 0x40 | `passed` / section ptr temp | `v` + temp |
| 0x44 | `sect_start` | section ptr temp / `narr_cur` |
| 0x48 | `failmask`/`nnarr` | `all_passed`/`i` |
| 0x4c | `n*0x4c` temp | `all_total` |
| 0x50 | `total` | `sect_start*0x4c` temp |
| 0x54 | section ptr temp / `narr_cur` | out-params (11) |
| 0x58 | `sect_start*0x4c` temp / `v+0x514` | |
| 0x5c | `all_total`/`i` | |
| 0x60 | `all_passed` | `box` |
| 0x64 | `kind` (short at 0x66) | |
| 0x68..0x90 | out-params (11) | |
| 0x90 | | `kind` |
| 0x94 | `lines[100]` | `lines[100]` |

The *sizes* now agree; the *assignment* still does not.  Both have exactly
the same 33 dwords with the same contents, just permuted -- VC6 numbers the
slots in the order it first needs them, so this permutation will follow the
register allocation once the prologue is right.

## What is left (updated)

1. **The prologue's callee-saved assignment.**  The original wants
   `ebx` = the zero constant, `ebp` = `page_start`, `esi` = `n`,
   `edi` = `y`; ours gives `ebp` = zero, `esi` = `n`, `ebx` = `y`,
   `edi` = **`indent`**.  Five candidates, four registers: the original
   spills `indent` (162 refs to `[esp+0x14]`) and keeps `page_start`
   in `ebp` with a store at every one of its 133 definitions; we spill
   `page_start` (338 symbol refs) and keep `indent`.  Everything downstream
   shifts with this, so it is the next thing to chase.  Per line, `PAGE_CHECK`
   makes three references to `page_start` and two to `indent`, so the naive
   count does not explain the choice -- try the LL14 lever
   (`git show origin/scope/LL14:docs/lanes/scope-ll14.md`, "Closing
   0x0046f9a0") and try spellings that change how many *reads* each gets.
2. **The render loop is still ~500 instructions short of the original.**
   VC6 already strength-reduces it the way the original does
   (`lea edi,[esp+eax*4+0xbc]` = `&lines[i].nids`, `lea ebp,[esp+eax*4+0x20c4]`
   = `&narr[nnarr]`), so the shortfall is in the input/narration tail, not in
   the walk's addressing.

## 2026-09-08 (third pass) — the `box`/`cur` aggregate is a regression

The escaping-aggregate spelling was tried in full: `typedef struct
AppraisalRects { AppraisalBox box, cur; }` with a `static __inline int
NewPage(AppraisalRects*)` doing the page's rect reset and returning
`box.top`.  It does reproduce the reload behaviour the lane wanted, but it
costs more than it buys:

| | HEAD (`box`, `cur` separate) | the aggregate |
| --- | --- | --- |
| emitted | 7,565 | 7,658 |
| bytes | 33,549 | 32,557 |
| frame | **`0x23d4` exact** | `0x23e4` (**+0x10, broken**) |
| `FULL MATCH` | **2789/7565 = 36.9%** | 2475/7658 = 32.3% |
| mismatch | 8,029 | 8,016 |

The aggregate wins 13 on the mismatch counter and 93 on the emitted count
and loses everything that matters: the exact frame and 4.6 points of full
match.  Taking the address of a 32-byte aggregate makes it unpackable, and
the frame grows by exactly the 16 bytes VC6 was previously overlapping out
of `cur`.  **Reverted.**  Do not retry this shape; if the reloads are ever
wanted again they have to come from a second *definition* of the fields
(the render phase re-assigning `cur`), not from an escaping address.

Two sub-questions settled on the way, both from the original:

- **`cur.top` is never written by the build.**  `[esp+0x30]` has exactly
  three references in the whole 8,085-instruction body — `mov edx,[esp+0x30]`
  at 0x0044d9bd and 0x0044d9f6 and `mov [esp+0x30],edx` at 0x0044da07, all
  three inside the render loop.  The build's page break writes only
  `cur.left` (`[esp+0x2c]`), `cur.right` (`[esp+0x34]`) and, on the
  section-alone arm, `cur.bottom` (`[esp+0x38]`); the new `y` and the new
  bottom stay in `edi` and `eax`.  So the current source, which assigns all
  four, is one store per site too generous — but see the caveat below.
- **Neither `box` nor `cur` is address-taken in the original.**  There is no
  `lea` anywhere in the body that points into `[esp+0x1c..0x38]`.  That kills
  the escaping-aggregate hypothesis at the source and confirms the earlier
  reading: the reloads come from the render phase giving the fields a second
  definition.

**Caveat on the raw slot counts.** `tools/disasm.py` does not track `esp`,
and this body pushes arguments constantly, so a bare `grep 'esp + 0x1c'`
mixes `box.left` with the `n*0x4c` byte-offset temp seen through one pending
push (`[esp+0x18]` + 4).  Only counts taken between pushes — such as the
`0x30` count above, which sits in the push-free render loop — can be
trusted.  Use the `/FAs` listing for anything else.

## 2026-09-08 (fourth pass) — the page reset belongs on BOTH arms

| | before | after |
| --- | --- | --- |
| emitted | 7,565 | 6,821 |
| bytes | 33,549 | 29,309 |
| frame | `0x23d4` exact | `0x23d4` exact |
| first diverging index | 1 | **6** |
| mismatch | 8,029 | **8,005** |
| `FULL MATCH` | 2789/7565 = 36.9% | **2914/6821 = 42.7%** |

Reading the original's first two page-break sites (0x00445450 and 0x0044557a)
against their fall-through arms (0x00445481, 0x0044559a) shows the four `box`
loads and *two* of the `cur` stores emitted **above** the `cmp ebp,esi`, and
then the same stores emitted **again** in the fall-through arm.  A store is
not speculated above a branch unless it is on both paths, so the reset is in
both arms of the source:

```c
if (y + 0x16 > 0x1b5) {
    if (page_start != sect_start) {
        page_start = sect_start;  n = sect_start;
        indent = lines[sect_start].indent;
        g_report_pages++;
        cur.right = box.right; cur.left = box.left; cur.bottom = box.bottom;
        y = box.top;
        goto LBL;
    }
    g_report_pages++;  page_start = n;
    cur.right = box.right; cur.left = box.left; cur.bottom = box.bottom;
    y = box.top;
}
```

It is also the only spelling that is *correct*.  The rewind arm jumps back to
the section label whose first act is this same page check; with the old `y`
still in hand the check fires a second time and `g_report_pages` is bumped
twice.  The draft on disk had that bug.

`cur.top` is dropped from the reset (see the third pass): the build never
writes it.

**The prologue moved on its own.**  With the reset on both arms VC6's
allocator flips the zero constant from `ebp` into `ebx`, which is where the
original keeps it, and the first six instructions now match:

```
mov eax,0x23d4 / call __chkstk / push ebx / push ebp / push esi / xor ebx,ebx
```

Index 6 is `xor ebp,ebp` in the original (`page_start = 0`) against `push edi`
in ours -- ours still enregisters `indent` in `ebp` and spills `page_start`.

### Measured: six spellings of the page reset

Object prefix `/tmp/sll16_v`.  "agg" is the escaping `AppraisalRects`
aggregate of the third pass, "plain" the two separate structs.

| where the reset goes | emitted | mismatch | `FULL MATCH` | frame |
| --- | --- | --- | --- | --- |
| else arm only, plain | 7,449 | 8,040 | 36.8% | `0x23d4` |
| else arm only, agg | 7,658 | 8,016 | 32.3% | `0x23e4` |
| hoisted above the test, plain | 7,048 | 8,031 | 36.2% | `0x23d4` |
| hoisted above the test, agg | 7,268 | 8,018 | 37.4% | `0x23e4` |
| **both arms, plain** | **6,821** | **8,005** | **42.7%** | **`0x23d4`** |
| both arms, agg | 7,675 | 8,035 | 44.5% | `0x23e4` |

Two things to read off this table.  First, *hoisting* the reset above the
rewind test is not the same as putting it on both arms and is measurably
worse -- with one copy in the source VC6 constant-folds `box` and then
deletes the store as redundant against the previous site's, which is how the
instruction count falls to 7,048.  Second, the aggregate still buys 1.8
points of `FULL MATCH` on top of the both-arms spelling (44.5% vs 42.7%)
because it is the only thing that reproduces the original's *memory* reloads
of `box` -- but it costs the exact frame and 30 on the mismatch counter, so
the plain spelling is what is on disk.  Closing the aggregate's `+0x10` is
the obvious next lever: if a 32-byte address-taken aggregate can be made to
pack the way two separate 16-byte structs do, both/agg should dominate.

### Why the instruction count went DOWN

6,821 against the original's 8,085 is 1,264 short, worse than the 520 the
third pass reported, and that is expected: the shortfall is entirely the
`box` reloads.  The original spends seven instructions per page-break site
reloading `box.left/top/right/bottom` from `[esp+0x1c..0x28]`; ours spends
two, because `box` has a single reaching definition in the build and VC6
constant-folds `0x50/0x6d/0x1a4/0x83` into the `cur` stores and then deletes
the ones that are redundant against the previous site.  **Do not chase the
instruction count here** -- it is a proxy for the `box` opacity question and
nothing else.  `mismatch` and `FULL MATCH` are the honest measures, and both
improved.

## 2026-09-08 (fifth pass) — `box` opacity is the whole remaining residual

Everything left in this function funnels into one question: the original
keeps `box` in memory at `[esp+0x1c..0x28]` and reloads all four fields at
each of the ~140 page-break sites; ours constant-folds `0x50/0x6d/0x1a4/0x83`
into the `cur` stores.  Three consequences, all measured:

1. **The 1,264-instruction shortfall.**  Seven instructions per site against
   our two.
2. **`y` never gets a callee-saved register.**  With `box.top` a constant,
   `y` is a constant at every page-break site, so VC6 rematerialises it and
   uses `edi` as a scratch; the original keeps `edi = y` for the whole build.
   Our first page check disappears outright — `0x6d + 0x16 <= 0x1b5` folds —
   which is why our object jumps straight from `test byte ptr [FLAGS],0xf`
   into the line stores.
3. **The `indent` / `page_start` register flip.**  With `y` not competing,
   ours has three live candidates for four registers and puts `indent` in
   `ebp`; the original has four (`zero`, `page_start`, `n`, `y`) and spills
   `indent`.  So the flip is downstream of the opacity, not a separate
   problem — do not chase it on its own.

### Every opacity spelling costs exactly `+0x10`, and it is always `title`

`/FAs` says so directly.  In the committed build `title` has **no equate** —
VC6 builds the `NewPrintCent` rectangle straight onto the pushed arguments
(`sub esp,0x10 / mov edx,esp / mov [edx],0x28 / ...`), exactly as the
original does at 0x0044d7c6.  In *every* spelling that makes `box` memory-
resident, `_title$` appears at `esp+0x1e44` and `namebuf`, `textbuf` and
`narr` all shift up by 16.  Nothing tried moves it:

| spelling | emitted | mismatch | `FULL MATCH` | frame | fdi |
| --- | --- | --- | --- | --- | --- |
| **committed (both arms, plain)** | 6,821 | **8,005** | 42.7% | **`0x23d4`** | **6** |
| escaping aggregate `{box, cur}` | 7,793 | 8,022 | 44.0% | `0x23e4` | 0 |
| escaping aggregate `{box, cur, title}` | 7,795 | 8,016 | 35.7% | `0x23e4` | 0 |
| non-escaping aggregate `{box, cur}` | 6,831 | 8,005 | 42.3% | `0x23d4` | 6 |
| non-escaping aggregate `{box, cur, title}` | 6,830 | 8,014 | 36.5% | `0x23e4` | 0 |
| `union { AppraisalBox b; int w[4]; }` | 6,821 | 8,005 | 42.7% | `0x23d4` | 6 |
| a second identical `box` init at `sect1:` | 6,829 | 8,028 | 44.5% | `0x23e4` | 0 |
| the title rect moved into a `static __inline` | 6,821 | 8,005 | 42.7% | `0x23d4` | 6 |
| that inline **+** the second init | 6,829 | 8,028 | 44.5% | `0x23e4` | 0 |

Read off: the **union is completely inert** (byte-identical to the baseline),
so is declaration order (`cur` before `box`), and so is a non-escaping
aggregate — lever 11 of scope LL9 does not bite here.  Two things *do*
produce the reloads: an aggregate whose address escapes, and **a second
reaching definition of `box`'s fields** (`twodef` — re-running the same four
constant stores at the `sect1:` label, which section two's restart branches
back to).  The second one is new and is the more promising of the two: it
needs no address to be taken, and it reaches the best `FULL MATCH` measured
so far, 44.5%.

**So the next agent's question is narrow: why does the original have `box` in
memory and no `title` home at the same time?**  Our 33 scalar dwords plus a
homed `title` is 37; the original has 33 with `box` memory-resident, so four
dwords of ours are spurious under opacity — or the original's title rect is
not a struct local at all.  Moving it into a `static __inline` with its own
local was tried and is inert, so the elision is a copy-propagation that the
memory-resident `box` disables, not a scoping effect.  Worth trying next:
a spelling that gives `box` its second definition *without* a duplicate
constant block (something in the render loop that VC6 believes reaches the
build), and checking whether any of `nrun`, the `[esp+0x1c]` temp or the
`[esp+0x40]` temp can be folded away to pay for `title`.

**Do not chase the instruction count.**  6,821 against 8,085 looks like a
regression against the fourth pass's predecessor at 7,565, but the count is
purely a proxy for the `box` reloads; `mismatch` and `FULL MATCH` both
improved and the frame stayed exact.

## 2026-09-08 (sixth pass, escalation) — the page reset is a struct copy

| | before | after |
| --- | --- | --- |
| emitted | 6,821 | **8,094** (original 8,085) |
| bytes | 29,309 | 34,413 |
| frame | `0x23d4` exact | `0x23d4` exact |
| first diverging index | 6 | 5 |
| mismatch | 8,005 | **7,970** |
| `FULL MATCH` | 42.7% | 39.0% (the alignment metric; see below) |

Two changes, both measured (object prefix `/tmp/sll16b_`):

1. **The page reset is `cur = box; y = cur.top;` on both arms** — a whole-struct
   copy, not four field copies.  VC6 lowers a struct copy as four
   memory-to-memory moves and does not constant-propagate into them, so `box`
   becomes memory-resident and is reloaded at every page-break site exactly
   as the original does (`mov eax,[esp+0x24] / mov edx,[esp+0x1c] / ...`).
   The instruction count lands within nine of the original.  `y = box.top`
   instead of `y = cur.top` is measurably worse (mismatch 8004).
2. **The bar rectangle in the render loop is its own `AppraisalBox bar`, not
   `box` reused.**  The original never stores into `[esp+0x1c..0x28]` inside
   the render loop, and reusing `box` there both gave it a second definition
   and (bug) made every frame after the first bar copy `0x126` into
   `cur.left` at the render head.

Measured on the way (all with the struct-copy sites): a block copy at the
render head too (`cur = box;`), or a separate `rc` for the text rectangle,
flips `box` back to fully folded (6,833 emitted) — keep the render head
fieldwise and pass `cur` by value.  Declaration order, `const`, an
initializer list, a `static const` template, `y = 0x6d` vs `y = box.top`,
and `box.top = y` are all inert.  The `+0x10` that every earlier opacity
spelling cost `title` is understood and avoided: it was VC6 hoisting
`title.right = 0x1a4` out of the frame loop because the pre-loop
`cur.right = 0x1a4` store makes the constant available in a register; it
does not happen with the spellings above.

## 2026-09-08 (seventh pass) — `cur.top` is the build's y; one rewind block per section

| | struct-copy sites (d543f87a) | now |
| --- | --- | --- |
| emitted | 8,094 | 7,251 |
| frame | `0x23d4` exact | `0x23d4` exact |
| first diverging index | 5 | 6 |
| mismatch | 7,970 | 8,007 |
| `FULL MATCH` | 39.0% | **46.4%** |

Three changes, each measured on its own and together (object prefix
`/tmp/sll16b_`):

1. **There is no separate `y`: the build's row is `cur.top`.**  With `y` a
   separate scalar VC6 constant-propagates `y = cur.top` to `mov edi,0x6d`
   at every site; with `cur.top` itself the enregistered row (`edi`), the
   struct copy loads `box.top` straight into `edi` and never stores
   `cur.top`'s home in the build — exactly the original's three stores
   (left, right, bottom) plus the `edi` load per site.  `[esp+0x30]` is then
   touched only by the render loop, as the original.
2. **One rewind block per section, reached by `goto rew_sectK`.**  The
   per-site arm is just the struct copy and the jump; the block does
   `page_start = sect_start; n = sect_start; indent = lines[sect_start].indent;
   g_report_pages++; goto sectK;`.  This is what spills `indent` (the
   original never enregisters it) and gives `page_start` its home with a
   store at every definition; with the rewind inlined at every site
   `indent` has ~140 extra definitions and always wins a register.  It also
   makes the two label bugs (section two's first line restarting at
   section one, the closing lines restarting at section nine) ordinary
   copy-paste mistakes in a `goto` target rather than in a macro argument.
   VC6 lays the blocks out next to their sections whatever the source order.
3. **The page check is `cur.bottom = cur.top + 0x16; if (cur.bottom > 0x1b5)`.**
   On top of 1 and 2 this is worth +0.4 points of `FULL MATCH` and 41 on the
   mismatch counter; it is also the only spelling under which the original's
   section-8 memory compares of `[esp+0x38]` can arise at all.

The render head has to be the block copy `cur = box;` in this shape (the
fieldwise head lets VC6 form register webs for the box constants and then
hoist `title.right = 0x1a4` out of the frame loop, +0x10 on the frame).

### Measured and rejected on the way

| spelling | result |
| --- | --- |
| the reset copy placed *before* the inner rewind test (the original's hoisted loads as source) | frame `0x23d8`, 35.8% |
| `cur.bottom` updated at the line end instead of at the check | frame `0x23e4` (bar rect homed), 37.9% |
| `volatile int indent` | frame `0x23d8`, ESCAPES |
| declaration order of `indent` / `page_start` (three orders) | byte-identical |
| `y = box.top` instead of `y = cur.top` (struct-copy sites) | mismatch 8004 vs 7970 |
| initializer list / `const` / `box.top = y` for the box init | inert or stores hoisted above `ScriptRunning` |

### What is left

- **`ebp`: ours caches `indent` there, the original `page_start`** (both
  are homed with a store at every def in the original's style; only the
  cached one differs).  Every ranking lever tried is inert; the two are
  within a hair of each other in VC6's weighting.
- **`box.bottom` is constant-folded to `mov eax,0x83`** at the fall-through
  arm (130 sites) where the original reloads `[esp+0x28]`; the other three
  fields reload as the original does.  `cur.bottom` being a register scalar
  is what lets VC6 propagate the constant into it.
- **`cur.bottom` never touches memory in ours**; the original spills it at
  28 section-8 checks (`cmp [esp+0x38],0x1b5`) and stores `y+0x16` to it 23
  times, always across a `failmask` test.  The line-end update reproduces
  that pattern but currently costs the frame (see the table).

### Section 8's page checks (measured after the seventh pass)

The original's 28 memory compares `cmp [esp+0x38],0x1b5` and 23
`cur.bottom = y+0x16` stores are all in section 8, each across a
`failmask` test, and the five `mov eax,[esp+0x38]` re-reads are the closing
no-advance lines.  Spelling section 8's lines with the update at the LINE
END (`cur.top += 0x18; cur.bottom = cur.top + 0x16;`) and their check as a
bare `if (cur.bottom > 0x1b5)` reproduces that census almost exactly
(`/tmp/sll16b_g57`: 27 memory compares, 4 re-reads, 153 bottom stores vs
the original's 28 / 5 / 147; mismatch 8007 -> 7989) — but under that
spelling VC6 constant-folds the whole `cur = box` copy in section 8 to
`mov eax,0x50 / 0x1a4 / 0x83` and `mov edi,0x6d`, so the alignment drops to
39.8% and the sites lose the `box` reloads.  Applying the line-end form to
the closing lines only is worse still (22.3%).  Sections 1-7 and 9 are
certainly the check-site form (their checks recompute `lea eax,[edi+0x16]`
after `rand`/`GetString` calls).  The check-site form is what is committed;
section 8's true spelling is still open and is coupled to the `box`
folding question.

## 2026-09-08 (eighth pass) — section 8's line-end form, proved from the object

| | seventh pass | now |
| --- | --- | --- |
| emitted | 7,252 | 7,272 |
| bytes | 31,792 | 31,952 |
| frame | `0x23d4` exact | `0x23d4` exact |
| first diverging index | 6 | 6 |
| mismatch | 8,007 | **7,937** (the lane's best) |
| index-for-index `MATCH` | 78 | **148** |
| `FULL MATCH` (difflib) | 46.4% | 39.9% |
| true LCS vs the whole original | 51.2% | 50.2% |

### Read the metrics in this order: mismatch, `MATCH`, LCS — not `FULL MATCH`

`matchfull.py` truncates the ORIGINAL to `len(comp)` and then runs
`difflib.SequenceMatcher`, whose result is a greedy longest-matching-block
approximation, not an LCS.  On a 7,000-element sequence it swings several
points when a single anchor block moves, so a 6-point `FULL MATCH` change on
a twenty-instruction edit means nothing on its own.  A bit-parallel LCS
(Allison–Dix) over the FULL original is stable and moved only 51.2% -> 50.2%
for the same edit that took mismatch down 70 and index-for-index matches up
70.  Keep `/tmp/sll16c/lcs.py`-style scoring alongside the two committed
tools; the shape is

```python
idx = {}
for j, b in enumerate(B): idx[b] = idx.get(b, 0) | (1 << j)
V = (1 << len(B)) - 1
for a in A:
    u = V & idx.get(a, 0)
    V = ((V + u) | (V - u)) & ((1 << len(B)) - 1)
lcs = bin(V ^ ((1 << len(B)) - 1)).count('1')
```

### Section 8 updates `cur.bottom` at the LINE END — no longer a hypothesis

0x00448661, in the middle of the advice chain:

```
add  edi, 0x18                  ; cur.top += 0x18
mov  [eax], ecx                 ; the NARR nids++
lea  eax, [edi + 0x16]          ; cur.bottom = cur.top + 0x16
mov  [esp + 0x38], eax          ; ...stored
test byte ptr [esp + 0x48], 2   ; if (failmask & 2)
je   0x44877f
cmp  [esp + 0x38], 0x1b5        ; if (cur.bottom > 0x1b5)
```

The bottom is computed at the END of the previous line, *above* the next
piece of advice's `failmask` test, and the check only reads it.  VC6 does not
sink a computation past a branch, so this cannot be the check-site form.
Three independent confirmations:

- **All 28 `cmp [esp+0x38],0x1b5` memory compares in the body are in section
  8** and nowhere else (zone census below).  Under the check-site form the
  compare always has a fresh `lea` in the same block.
- **Section 8 is entered with `cur.bottom` already live.**  The `sect8:`
  label is 0x00447e73; the `lea eax,[edi+0x16]` its header check consumes is
  at 0x00447e6c, *above* the label, so it cannot be part of a check that the
  rewind (`jmp 0x447e73`) has to re-run.  The rewind path instead arrives
  with `eax = box.bottom` from its own `cur = box`.
- **Sections 1–7 are not the line-end form.**  At 0x00445791, 0x004458c5 and
  0x00445a02 a line ends `add edi,0x18` and the very next instruction is the
  next statistic's `test [FLAGS],K` — no `lea eax,[edi+0x16]` in between.

Zone census of the original (`s8` = 0x447e73..0x44a70c):

| idiom | s1 | s2 | s3–7 | s8 | close | s9 |
| --- | --- | --- | --- | --- | --- | --- |
| `cmp [esp+0x38],0x1b5` | | | | **28** | | |
| `cmp <reg>,0x1b5` | 1 | 20 | 15 | 11 | 6 | 42 |
| `lea <reg>,[edi+0x16]` | 1 | 19 | 16 | 30 | 1 | 40 |

### What is on disk

Section 8 and the closing "next time" lines use `PAGE_CHECK8` — a bare
`if (cur.bottom > 0x1b5)` — with `cur.top += 0x18; cur.bottom = cur.top +
0x16;` at the line end of the advancing ones and nothing at the end of the
five no-advance ones; a single `cur.bottom = cur.top + 0x16;` sits just above
the `sect8:` label.  Sections 1–7 and 9 keep `PAGE_CHECK`.  Resulting census,
ours against the original:

| | ours | original |
| --- | --- | --- |
| `cmp` cur.bottom in memory | 27 | 28 |
| `cmp` cur.bottom in a register | 96 | 95 |
| `lea <reg>,[edi+0x16]` | 106 | 107 |
| stores to cur.bottom | 152 | 147 |

Variants measured: the line-end form for section 8 *without* the closing
lines is mismatch 8,019; without the pre-`sect8:` update the frame breaks to
`0x23d8` and mismatch is 8,049.  The seventh pass's report that this spelling
constant-folds section 8's `cur = box` **did not reproduce** — `box`'s read
count is identical (177) before and after.

### Residual 2, quantified: the missing box reload is the REWIND arm's

Summing the instructions between each `cmp …,0x1b5`'s `jle` and its target:

| | sites | instructions in page-break blocks | median |
| --- | --- | --- | --- |
| original | 113 | 2,561 | 19 |
| ours | 106 | 1,981 | 15 |

580 of the body's 813-instruction shortfall is here, four per site.  The
original emits the `box` reload TWICE per site: once speculated **above** the
`cmp ebp,esi` (serving the rewind arm) and once inside the `je` arm.  Ours
emits it above the `cmp` only where the rewind block is not shared — at every
later site VC6 tail-merges `cur = box; goto rew_sectK;` into the shared
rewind block (`jne $L888`) and the copy disappears.  `box` is read 177 times
in our object against roughly 2 per site in the original.

Writing the hoist into the source — `cur = box;` above the inner test **and**
`cur = box;` in the fall-through arm — does reproduce it: 7,820 emitted,
mismatch 7,950.  **It costs the frame**: `0x23d8`, which moves `lines` off
`0x94` and is therefore fatal.  Rejected, but this is now the sharpest
statement of the residual: *a shape that emits both copies per site without
adding a scalar dword closes 580 instructions.*  Measured on the way (all
frame-exact unless noted):

| shape | emitted | mismatch | LCS |
| --- | --- | --- | --- |
| **committed** (copy in each arm) | 7,272 | **7,937** | 50.2% |
| hoisted + fall-through copy | 7,820 | 7,950 | 42.3% (frame `0x23d8`) |
| hoisted + rewind-arm copy | 7,193 | 7,993 | **51.5%** |
| hoisted, one copy only | 7,096 | 8,048 | 50.0% |
| hoisted + fieldwise fall-through | 6,875 | 8,029 | 42.0% |
| `if/else` instead of the early `goto` | 7,178 | 7,998 | 50.1% |

### `box.bottom`'s fold is real, and it is `box`'s constant

Compiling with `box.bottom = 0x84` moves the 131 `mov <reg>,0x83` in the
object to `mov <reg>,0x84`, so the fold is of `box.bottom`'s own initialiser
and not of `cur.top + 0x16`.  `box.bottom`'s home is then dead-stored away,
which is why the object has only three `box` stores.  Every reordering and
respelling of the four-store init is **byte-identical**: bottom first, `box
.bottom = box.top + 0x16`, `cur.bottom = box.bottom`, `cur.top = box.top`,
`cur.top`/`cur.bottom` seeded before the init.  An explicit `cur = box;`
after the init is NOT inert (7,268 emitted, mismatch 8,031, LCS 51.4%) but it
makes VC6 emit six entry stores where the original emits four.

### Residual 1 (`ebp`): one lever found, and it is not free

Reference counts do not explain the choice.  In the original `page_start`'s
home is written 133 times and read twice, with 84 register compares — 181
references — against `indent`'s 142 memory reads and 20 writes; in ours
`page_start` has 122 reads and 131 writes in memory (253) while `indent`
lives in `ebp` with 43 spill references.  Both bodies rank the same two names
the same way by count and allocate them oppositely, so the LL14
appearance-count model does not transfer: post-expansion `page_start` already
appears more often than `indent` in our source.

Newly measured and **inert** (byte-identical objects): `register int
page_start`, `register` on `page_start` and `n` together, `unsigned int
indent`, hoisting `page_start` into a named temp inside the check,
`page_start = n` before `g_report_pages++`.

Newly measured and **worse**: `lines[n].indent = indent;` moved above the
page check (mismatch 8,027); splitting `indent` into two webs at `sect8:`
through an `INDENT` macro (frame `0x23cc`, mismatch 8,014) — so the
"two short webs lose to one long one" idea is refuted here.

The one lever that bites is **narrowing `indent`**.  `short indent` takes the
first diverging index from 6 to **8** — `xor ebp,ebp` and `push edi` fall into
place — and puts `page_start`'s home at the original's `0x10`, i.e. it flips
exactly the allocation this residual is about.  It costs mismatch (8,020) and
LCS (40.7%) because every `lines[n].indent = indent` grows a `movsx`, so it
is not the answer, but it proves the ranking is a hair's breadth and that the
lever is on `indent`'s side, not `page_start`'s.  The next spelling to find
is one that lowers `indent`'s rank without changing its width: something that
makes its 140 reads cheaper-looking to the allocator than `page_start`'s 84
compares plus 133 write-throughs.

### A warning about slot censuses

`tools/disasm.py` does not track `esp`, and a naive `[esp + 0xNN]` census of
this body is wrong by whole slots: at 0x00445650 `mov [esp+0x20],eax` reads
as `box.top` but three pushes are pending, so it is `[esp+0x14]` — `indent +=
0x30`.  A CFG-propagated `esp` delta does not converge either (128 conflicts,
389 unreachable) because VC6 defers its argument pops across branches.  Only
counts taken at push-free points — the page-break blocks, the render loop —
can be trusted; use `/FAs` for everything else.

### Refinement: the `indent` lever is the truncating READ, not the declaration

`lines[n].indent = (short)indent;` with `indent` left an `int` reproduces
`short indent`'s allocation flip exactly — first diverging index 6 -> **8**
(`xor ebp,ebp` and `push edi` fall into place) and `page_start`'s home lands
on the original's `0x10` — while keeping the frame at `0x23d4`, which the
`short` declaration also does.  Cost is the same: a `movsx` per line takes
mismatch to 8,020 and the LCS to 40.7%, and `ebp` still ends up holding
`indent`'s zero rather than `page_start`.  So the allocator responds to how
`indent` is *read*, and a spelling that lowers its rank without inserting an
instruction per line is what this residual needs.

Also inert: evaluating the rewind predicate on the main path
(`rew = (page_start != sect_start);` before the `cur.bottom` test), which
gives `page_start` 140 unconditional reads instead of 123 conditional ones —
VC6 sinks it straight back into the arm and the object moves by two
instructions.

## 2026-09-08 (ninth pass) — the double copy lands; the residual is now the slot permutation

| | eighth pass | now |
| --- | --- | --- |
| emitted | 7,272 | **7,821** (original 8,085) |
| bytes | 31,952 | **34,224** (original 34,662) |
| frame | `0x23d4` exact | `0x23d4` exact |
| first diverging index | 6 | **8** |
| mismatch | 7,937 | **7,933** (lane best) |
| index-for-index `MATCH` | 148 | **152** (lane best) |
| true LCS vs the whole original | 50.2% | **52.3%** (lane best) |
| page-break block instructions | 1,981 / 106 sites | **2,778 / 122 sites** (original 2,605 / 115) |

This state dominates the eighth pass's on every measure the lane tracks.
Two changes, both required together (object prefix `/tmp/sll16d_`):

1. **The page reset is hoisted above the rewind test and repeated in the
   fall-through arm** — the eighth pass's "hoisted + fall-through copy" row,
   which was rejected only because it cost the frame.
2. **`v` and section 9's `nhint` are one local**, which pays the dword the
   hoist costs.

### The hoist is the original's source, proved from a typical site

0x004456a7, the 19-instruction median site:

```
lea  eax, [edi + 0x16]        ; cur.bottom = cur.top + 0x16
cmp  eax, 0x1b5
jle  0x4456f0
mov  eax, [esp + 0x24]        ;  box.right   \
mov  ecx, [esp + 0x44]        ;  sect_start   |
mov  edx, [esp + 0x1c]        ;  box.left     |  cur = box, HOISTED above
mov  edi, [esp + 0x20]        ;  box.top      |  the rewind test
mov  [esp + 0x34], eax        ;  cur.right    |
mov  eax, [esp + 0x28]        ;  box.bottom  /   (its store is dead: see below)
cmp  ebp, ecx                 ; page_start != sect_start
mov  [esp + 0x2c], edx        ;  cur.left
jne  0x4464b7                 ; -> the shared rewind block, which copies nothing
mov  eax, [0x6660a0]          ; g_report_pages++
mov  ecx, [esp + 0x28]        ;  box.bottom   \
inc  eax                      ;               |
mov  ebp, esi                 ; page_start = n|  cur = box, AGAIN
mov  [0x6660a0], eax          ;               |
mov  eax, [esp + 0x24]        ;  box.right    |
mov  [esp + 0x10], ebp        ;               |
mov  [esp + 0x2c], edx        ;  cur.left     |
mov  [esp + 0x34], eax        ;  cur.right    |
mov  [esp + 0x38], ecx        ;  cur.bottom  /
```

Three things fall out of this and settle the fourth pass's reading:

- The copy above the `cmp` is **the rewind arm's** copy, which is why the
  shared rewind block at 0x004464b7 contains no `box` load at all — it is
  ten instructions of `page_start`/`n`/`indent`/`g_report_pages` and a `jmp`.
- `cur.bottom`'s store is missing from the hoisted copy because it is dead on
  both paths: the rewind arm recomputes it at the section label and the
  fall-through arm re-stores it. Its **load** survives, which is the tell —
  VC6 killed a store out of a struct copy and left the load behind.
- Writing one copy in each arm instead (the eighth pass's committed shape)
  lets VC6 **sink** the rewind arm's copy into the shared block, and the four
  instructions vanish at every later site. Sinking, not tail-merging: the
  block at 0x10b4 in that object literally begins with the four `box` loads.

Our sites are now 2,778 instructions over 122 sites against the original's
2,605 over 115 — we now overshoot by 173 where we were 580 short. The
overshoot is one store per site: ours keeps the hoisted `cur.bottom` store
that the original dead-stores away, and pays for it by keeping `0x83` live in
`eax` so the fall-through needs no reload. Net zero per site; the 173 is the
handful of sites where that bookkeeping differs.

### The dword the hoist costs, and what pays for it

The hoist flips the register allocation to the original's — `page_start` into
`ebp`, `indent` memory-homed — and a memory-homed `indent` is live across the
whole build, so it can no longer share a slot with `passed` and `nnarr` the
way the enregistered one did. `/FAs` says it exactly:

| | eighth pass | with the hoist |
| --- | --- | --- |
| shared pool | `{indent,nnarr,passed}` `{nhint,total}` `{failmask}` | `{indent}` `{nhint,nnarr,passed}` `{failmask}` `{total}` |
| scalars | 33 dwords, `lines` at `0x94` | 34 dwords, `lines` at `0x98` |

The original has the same memory-homed `indent` in its own slot and still
fits 33, because **five of its slots carry a CSE temp and a named local at
once** (`0x18` temp+`nrun`+`nhint`, `0x40` temp+`passed`, `0x54`
temp+`narr_cur`, `0x58` temp+`v`, `0x5c` `all_total`+`i`) where ours manages
three. Nothing tried moves that: measured **inert** (frame stays `0x23d8`)
were the rewind block reading `lines[n].indent` after `n = sect_start`,
reordering `page_start`/`n`/`g_report_pages++` inside it, reading `indent`
through an explicit `char*` cast, a saved `sidx`, a `RepLine* sp` pointer
kept at each section head, moving all nine `rew_sectK` blocks to sit next to
their sections, and merging `nrun` with `v` or with `nhint`, `nnarr` with
`total`/`failmask`/`indent`, `narr_cur` with `total`, `i` with `all_total`.

What does work is **removing one named scalar**, and the only name VC6 can
absorb is `v`: merging it with `nhint`, `passed`, `ok`, `all_total` or even
`nattr` all give `0x23d4` back, while merging any other pair does not. Of the
semantically legal ones (`v`'s three uses are all dead before section 9's
unconditional `nhint = 0`) `v`+`nhint` is the one on disk, spelled as
`#define nhint v` so section 9 still reads as a hint counter. `v`+`passed`
scores marginally better (LCS 52.8%) and is **wrong** — section 2 would
overwrite the section's pass count with `PercentObjectsLinked()`.

### Measured on the way (all with the hoist)

| shape | emitted | mismatch | LCS | frame |
| --- | --- | --- | --- | --- |
| **committed** (hoist + `v`/`nhint` merged) | 7,821 | **7,933** | **52.3%** | `0x23d4` |
| hoist + `v`/`passed` (incorrect source) | 7,821 | 7,933 | 52.8% | `0x23d4` |
| hoist + `v`/`total` | 7,817 | 7,952 | 52.1% | `0x23d4` |
| hoist + `v`/`all_total` | 7,820 | 7,947 | 52.1% | `0x23d4` |
| hoist + `v`/`ok` | 7,827 | 8,024 | 52.1% | `0x23d4` |
| hoist alone | 7,829 | 7,950 | 42.3% | `0x23d8` (fatal) |
| hoist + `v`/`nhint` + `RepLine* sp` | 7,826 | 7,978 | 52.6% | `0x23d4` |
| non-escaping `struct {box, cur}` + hoist | 7,826 | 7,944 | 54.7% | `0x23e4` (fatal) |

Also **inert** on top of the hoist: `box.bottom = box.top + 0x16`, zeroing
`indent` before `page_start` at entry, initialising `box` before `cur.top`,
`cur.top = box.top` instead of `0x6d`, and a `static __inline` that builds the
bar rectangle on the pushed arguments. And **rejected** on top of the eighth
pass's shape, all byte-identical to it: the empty trailing `else { }` (scope
LL17's lever), the block-pin `if (n) ;` and `if (indent) ;` (scope LL10's),
and inverting the test to `if (page_start == sect_start)`. Moving the reset to
the FRONT of the fall-through arm so both arms share a source prefix does not
trigger VC6's prefix hoister — it sinks the copy instead (mismatch 7,999).

### The one aggregate result worth keeping

`struct { AppraisalBox box, cur; } r;` (non-escaping, `#define box r.box`)
puts the two rectangles adjacent the way the original has them and is worth
**+2.4 points of LCS and +9 matches** on top of the hoist — but it costs
`+0x10`, and this time it is provable which local pays: `/FAs` shows
`_bar$ = 0x1e44` with `namebuf`/`textbuf`/`narr` all shifted up. `bar`
normally shares `box`'s home (`_bar$ == _box$`), which is legal because
`box`'s four fields are constants and the render head's `cur = box` folds, so
`box`'s home is dead by the render loop — the original does the same, which is
why its frame has no `bar` slot. Wrapping `box` in an aggregate makes the
whole 32-byte object live and `bar` has to be homed. A `static __inline`
helper that builds the bar rectangle from four ints does **not** free it
(byte-identical). Anyone retrying the aggregate has to kill `bar`'s home
first.

### The residual, quantified: the stack slot permutation

Both frames hold 33 scalar dwords below `lines` and the same values, in a
different order:

| | `0x10` | `0x14` | `0x18` | `0x1c` | `0x2c` | `0x3c` | `0x40` | `0x44` | `0x48` | `0x4c` | `0x50` |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | `page_start` | `indent` | temp/`nrun`/`nhint` | `box` | `cur` | `ok`/`obj` | `passed`/temp | `sect_start` | `failmask`/`nnarr` | temp | `total` |
| ours | temp | `cur` | | | | `ok`/`obj` (`0x2c`) | `box` (`0x40`) | | | | `nrun`/`i` |

ours in full (`/FAs` on the committed build): `0x10` temp, `0x14`–`0x20`
`cur`, `0x24` `page_start`, `0x28` `indent`, `0x2c` `ok`/`obj`/temp, `0x30`
`sect_start`, `0x34` `v`/`nnarr`/temp, `0x38` `failmask`, `0x3c` `passed`,
`0x40` `total`, `0x44`–`0x50` `box`/`bar`, `0x54` `nrun`/`i`, `0x58`
`all_passed`/`narr_cur`/temp, `0x5c` `all_total`, `0x60` temp, `0x64`–`0x90`
the eleven out-params with `kind` at `0x8c` (the original puts `kind` at
`0x64` and the out-params at `0x68`–`0x90`, and in a different order).
Five slots carry a temp here too — the counting in the previous section is
about *which* names ride them, not how many exist.

Declaration order was re-tested under this shape and is still inert: moving
`cur` ahead of `box`, and rewriting the whole scalar declaration block into
the original's slot order, both give a byte-identical frame map.

**Mechanically renaming our slots to the original's takes the LCS from 50.6%
to 59.8%** (`/tmp/sll16d_remap.py`, same normalisation as `tools/match.py`).
That is the size of this residual: nine points, and it is a single cause.
Nothing found so far steers it — declaration order, initialisation order,
renaming, `register`, and the order of the entry zero-stores are all inert,
and the doc's earlier passes say the same. The next lane should attack this
and only this; it is worth more than everything else left combined.

### What a future lane should try first

1. **The slot permutation**, above. VC6 orders this frame by something that
   is not declaration order, not first reference and not first store (the
   original stores `indent` first and gives it the *higher* of its two lowest
   slots). Find the rule on a small function before touching this one.
2. **`box.bottom`'s constant fold.** Ours emits `mov <reg>,0x83` where the
   original reloads `[esp+0x28]` at ~130 sites, and ours keeps `box.left` in
   `ecx` for the whole function where the original reloads it. Both are one
   instruction per site and both would fall out of `box`'s home being live.
3. **The 264-instruction shortfall is no longer in the page breaks** — those
   now overshoot by 173. It is in the render/input tail, which the second
   pass's note about `RepLine*` cursors still governs: whatever is tried
   there has to keep `i` and `nnarr` memory-homed.

## 2026-09-08 (tenth pass) — the frame is ORDERED BY REFERENCE WEIGHT

| | ninth pass | now |
| --- | --- | --- |
| emitted | 7,821 | 7,824 |
| bytes | 34,224 | 34,224 |
| frame | `0x23d4` exact | `0x23d4` exact |
| first diverging index | 8 | 8 |
| mismatch | 7,933 | **7,923** (lane best) |
| index-for-index `MATCH` | 152 | **162** (lane best) |
| true LCS vs the whole original | 52.3% | **54.9%** (lane best) |
| `difflib` alignment | 37.3% | 40.7% |

### The rule: descending reference weight, per dword, from `esp+0x10` upward

The ninth pass left "nothing found so far steers the slot permutation".  The
rule is now known and it is simple.  **VC6 lays this frame out in DESCENDING
order of reference weight, lowest address first, and an aggregate is ranked by
its weight divided by its size in dwords.**  Proved on small functions in
`/tmp/sll16e_lab` and confirmed on our own object:

- `t2.c` — ten `int` locals with 24/16/12/8/6/4/3/2/2/1 uses, no aggregates:
  the six that reach memory come out in exactly descending-weight order.
- `t5.c` — the same plus two 16-byte structs and **no array**: the scalars are
  weight-ordered at the bottom and both structs sit above all of them.
- `t6.c` — `t5.c` plus a 400-byte array and a 128-byte buffer: the structs
  stop being a block at the top and interleave among the scalars at exactly
  their **per-dword** weight (`cur` 63/4 = 15.75 lands between `ok` 17 and
  `passed` 6; `box` 20/4 = 5 lands between `fm` 6 and `total` 4).  So the
  presence of an array is what switches VC6 from "aggregates on top" to one
  weight-ordered sequence — which is the mode this function is in.

Our own committed object obeys it with a single adjacent inversion.  The
`/FAs` equates plus a census of `_name$[esp` and `-NNNN+[esp` occurrences give
the weights directly; that census is the tool this pass ran on every variant
(no `esp` tracking needed, so the doc's slot-census caveat does not apply):

```
0x10 T(n*0x4c) 210 | 0x14 page_start 171 | 0x18 indent 169 | 0x1c ok+obj+T 151
0x20 sect_start 135 | 0x24 r{box,cur} 901/8 = 112 | 0x44 v+nnarr+T 91
0x48 failmask 62 | 0x4c total 47 | 0x50 passed 38 | 0x54 nrun+i+T 29
0x58 all_passed+narr_cur 18 | 0x5c all_total 14 | 0x60 T 4 | 0x64.. out-params 3
```

Everything the earlier passes measured as **inert** — declaration order,
initialisation order, renaming, `register`, the order of the entry
zero-stores — is inert *because none of them changes a reference count*.  The
only lever on this frame is the weight.

### What that bought: `box` and `cur` are one aggregate, and `bar` loses its home

The original has `box` at `0x1c` and `cur` at `0x2c`, adjacent with `box`
below.  Two separate 16-byte structs cannot land adjacent unless their weights
happen to be adjacent, and ours were 145 and 758 — so they were pulled to
opposite ends of the frame (`cur` at `0x14`, `box` at `0x44`).  A
**non-escaping `struct { AppraisalBox box, cur; } r;`** (with `#define box
r.box` / `#define cur r.cur`) pins them together and is ranked as one 8-dword
object.  The ninth pass had measured this shape as +2.4 LCS but rejected it
because it cost `+0x10`: inside an aggregate `box`'s home is live to the end
of the function, so `bar` can no longer share it and has to be homed at
`0x1e44`.

**`bar`'s home is removable, and that is what makes the aggregate free.**  The
render loop wrote

```c
bar.left = 0x126; bar.top = cur.top; bar.right = 0x1a4; bar.bottom = cur.top + 8;
```

With the two constants written first VC6 has `0x126`, `0x1a4` and `cur.top+8`
all live across the three argument pushes, and it spills the computed bottom
to `bar`'s home.  Writing **top and bottom first** leaves only one value live
and the spill disappears — `_bar$` goes away entirely, which is what the
original does (it builds the bar rectangle straight onto the pushed arguments
at 0x0044d9d5 and has no `bar` slot at all).  On its own the reorder is worth
two instructions; together with the aggregate it is worth 2.6 points of LCS.

Measured (object prefix `/tmp/sll16e_`):

| shape | emitted | mismatch | `MATCH` | LCS | frame |
| --- | --- | --- | --- | --- | --- |
| ninth pass (committed before) | 7,820 | 7,933 | 152 | 52.3% | `0x23d4` |
| bar top/bottom first, alone | 7,818 | 7,934 | 151 | 52.4% | `0x23d4` |
| **aggregate + bar reorder** | **7,819** | **7,923** | **162** | **54.9%** | **`0x23d4`** |
| aggregate alone | 7,821 | 7,925 | 160 | 54.8% | `0x23e4` (fatal) |
| `AppraisalBox r[2]` + bar reorder | 7,819 | 7,923 | 162 | 55.0% | `0x23d4` |
| `struct { cur, box }` + bar reorder | 7,819 | 7,928 | 157 | 53.3% | `0x23d4` |
| `struct { box, cur, title }` | 7,819 | 7,955 | 130 | 39.5% | `0x23e4` |

`AppraisalBox r[2]` is three LCS instructions better than the struct and
identical on every other measure; the struct is what is on disk because it
reads as source.

### What is left of the permutation, and exactly what it is worth

Remapping the committed object's `[esp+N]` displacements to the original's
slots (same normalisation as `tools/match.py`):

| remap | `MATCH` | mismatch | LCS |
| --- | --- | --- | --- |
| none (as committed) | 162 | 7,923 | 54.9% |
| `page_start`->`0x10`, `indent`->`0x14`, temp->`0x18` only | 169 | 7,916 | **57.3%** |
| + `r`->`0x1c`, `ok`->`0x3c`, `sect_start`->`0x40` | 171 | 7,914 | 60.4% |
| the whole frame remapped | 171 | 7,914 | 61.4% |

So 2.4 of the remaining 6.5 points sit in **one object**: the `n * 0x4c`
byte-offset CSE temp, whose 210 references outrank `page_start`'s 171 and hold
it off `0x10`.  Push that temp's weight into the window **(151, 169)** and
`page_start`, `indent` and the temp all land on the original's `0x10`, `0x14`,
`0x18` at once.

The original's temp is the same size in total and is **split across four
slots**: an esp-tracked zone census of the original gives `0x18` 103 refs
(shared with `nrun`/`nhint`), `0x4c` 47 (sections 1-7 only), `0x54` 43
(sections 3-7 only), plus a share of `0x50`, and section 9 keeps the offset in
`ebp`.  Ours coalesces every line's offset onto one slot.  That is a packing
choice, not a source one, and nothing tried moves it:

| probe | temp | result |
| --- | --- | --- |
| drop the seven section-head `lines[n].indent` pre-stores | 201 | mismatch 8,021 |
| `lines[n].nids = 0` first in the line body | 234 **+** 208 (it does split!) | frame `0x23d8`, LCS 43.0% |
| `RepLine* rp = &lines[n];` in the line macros | `rp` 200 at `0x10` | frame `0x23d8`, LCS 34.8% |

The second row is the encouraging one: VC6 **will** split this temp into two
webs, so a spelling that splits it without disturbing the line body is what
this residual needs.

### The second lever: `box`'s remaining constant folds

`r` is ranked at 901/8 = 112 per dword and needs to be above `ok`'s 151 to
land on `0x1c` — about +350 references.  All of them are `box` reloads the
original makes and we do not.  At a typical site the original loads
`box.left/top/right/bottom` in the hoisted copy and reloads three of them in
the fall-through arm, seven loads per site; ours loads three, because
`box.bottom` is folded to `mov <reg>,0x83` (131 sites) and `box.left` and
`box.right` survive in `ecx`/`edx` into the fall-through arm.  Confirmed by
the arithmetic: an opaque `box` (initialised from four externs, a probe only)
takes `box` from 145 references to 744, puts `box` and `cur` adjacent with
`box` below **exactly as the original has them**, and lifts the LCS to 55.7%
even while breaking the frame and the first eight instructions.  A probe that
adds one instruction per site (`box.bottom += g_report_pages`) lifts `r` to
1,110/8 = 138 and moves it up one slot, over `sect_start` — the weight model
predicting the move correctly is itself the confirmation.

So the remaining work is two weight adjustments, both now quantified:

1. **Split or shrink the `n*0x4c` temp** to under `indent`'s 169 (ideally into
   the 151-169 window).  Worth 2.4 LCS points.
2. **Stop `box.bottom` folding and stop `box.left`/`box.right` surviving into
   the fall-through arm**, worth +350 references on `r` and a further ~3
   points — and it is the same residual as the ninth pass's "box.bottom's
   constant fold", now with a mechanism attached to it.

Also measured and **rejected** this pass: a second `box` init at `sect1:`
(mismatch 7,988) or at `sect2:` (8,003), re-initialising `box` at the render
head (7,990), `box.bottom = box.top + 0x16` (byte-identical), `cur = box;`
after the init with or without the separate `cur.top = 0x6d` (both 7,976).

## 2026-09-08 (eleventh pass) — the ORIGINAL's own frame breaks the weight rule; `box` opacity is solved

| | tenth pass | now |
| --- | --- | --- |
| emitted | 7,824 | 7,824 |
| bytes | 34,224 | 34,224 |
| frame | `0x23d4` exact | `0x23d4` exact |
| first diverging index | 8 | 8 |
| mismatch | 7,923 | **7,920** (lane best) |
| index-for-index `MATCH` | 162 | **165** (lane best) |
| true LCS vs the whole original | 54.9% | **55.5%** (lane best) |
| `difflib` alignment | 40.7% | 42.4% |

Two small changes committed, and two large measurements that redirect the lane.

### The tool this pass added: a validated `esp`-delta tracker

Every earlier pass says a raw `[esp+N]` census of the ORIGINAL cannot be
trusted because `tools/disasm.py` does not track `esp`, and that a
CFG-propagated delta does not converge. A **linear** delta does converge well
enough: walk the listing, `push` `+4` / `pop` `-4` / `add esp,K` `-K`, and
reset the delta to 0 at any address that is the target of a `jmp`/`jcc`/`call`.
The script is `/tmp/sll16f/slots.py`; rebuild it from this paragraph.

It was **validated against `/FAs` on our own object**, where the equates give
ground truth: every slot agrees to within about 5% (`0x18 indent` 168 vs 169,
`0x1c ok` 151 vs 151, `0x20 sect_start` 135 vs 135, the eight `r` dwords sum to
902 vs 901). So its reading of the original can be trusted to the same
tolerance. **This is the first time the original's slot weights have been
measured**, and it settles the tenth pass's open question the wrong way.

### The original's frame, with weights

| slot | what | refs |
| --- | --- | --- |
| `0x10` | `page_start` | 173 |
| `0x14` | `indent` | 171 |
| `0x18` | the `n*0x4c` temp + `nrun` + `nhint` | **222** |
| `0x1c`..`0x38` | `box` + `cur` | 1,448 (**181** per dword) |
| `0x3c` | `ok` / `obj` | 153 |
| `0x40` | `passed` + the `&lines[n].indent` pointer temp | 36 |
| `0x44` | `sect_start` | 128 |
| `0x48` | `failmask` / `nnarr` | 79 |
| `0x4c` | the second `n*0x4c` temp (five section heads) | 76 |
| `0x50` | `total` | 48 |
| `0x54` | `narr_cur` + a temp | 22 |
| `0x58` | `v` + the `&lines[n].ok` pointer temp | 16 |
| `0x5c` | `all_total` / `i` | 18 |
| `0x60` | `all_passed` | 13 |
| `0x64` | `kind` (a short at `0x66`) | 2 |
| `0x68`..`0x90` | the eleven out-params | 3 each |

Ours, same tracker: `0x10` temp 199, `0x14` `page_start` 181, `0x18` `indent`
168, `0x1c` `ok` 151, `0x20` `sect_start` 135, `0x24`..`0x40` `r` 902,
`0x44` `v` 91, `0x48` `failmask` 62, `0x4c` `passed` 38, `0x50` `total` 47,
`0x54` `nrun`/`i` 29, `0x58` 17, `0x5c` 13, `0x60` 4, out-params 3 each.

**Ours is in strict descending order. The original is not.** Its temp (222)
and its `box`/`cur` aggregate (181 per dword) both outweigh `page_start` (173)
and `indent` (171) and both sit *above* them in the frame; `passed` (36) sits
below `sect_start` (128). So the tenth pass's rule holds for our object and for
the lab functions but **the original violates it at exactly the three slots
this residual is about**. Whatever pulls `page_start` and `indent` to the
bottom of the original's frame is not reference weight, and nothing measured
this pass reproduces it.

The rule is still a good *predictor* — it was confirmed again twice this pass
(a fieldwise page reset takes `page_start` to 250 refs and it moves to `0x10`;
`box = cur` takes `r` to 1,501 and it moves to `0x14`) — so use it to predict
what a change will do. Just do not expect hitting the original's numbers to
produce the original's layout.

### The tenth pass's "split the temp" plan does not exist to be executed

The tenth pass proposed getting the `n*0x4c` temp's weight into the window
`(151, 169)` because the original's is "split across four slots". It is not.
Measured on both objects with the same detector (`shl <r>,2` followed by
`mov [esp+N],<r>` for stores; a `[esp+N]` load whose register is then an index
in `[esp+<r>+K]` for reloads):

| | main slot stores | secondary slot stores | refs per line site |
| --- | --- | --- | --- |
| original | 71 (`0x18`) | 5 (`0x4c`, the section-3..7 headers) | 3 |
| ours | 71 (`0x10`) | 5 (`0x44`) | 3 |

The two temps have the **same shape, the same split and the same per-site
cost**: one store, one reload after the page check, one reload after `rand`,
one reload after `GetString` (three references attributed to the line macro,
the fourth shared with the section head). The original's total is *larger* than
ours, 222 against 210. Attribution of our 210 by construct: `TEXT_LINE8` 106,
`TEXT_LINE` 43, `BAR_LINE` 24, `TEXT_LINE_NOY8` 12, the section-head
`lines[n].indent` pre-stores 9, `NARR` 9, `BUF_LINE8` 2.

So there is no reference to shed without making our line bodies differ from the
original's, and the tenth pass's window is chasing a number the original does
not have. **Do not spend another pass on it.**

### `box` opacity is SOLVED: seed `cur`, then `box = cur;`

The residual the fifth through tenth passes all converged on — the original
keeps `box` in memory and reloads all four fields at every page-break site
while ours folds `box.bottom` to `mov <reg>,0x83` and hoists `box.left` into
`ecx` — has a one-line source fix that **costs nothing on the frame**:

```c
    cur.left = 0x50;
    cur.top  = 0x6d;
    cur.right = 0x1a4;
    cur.bottom = 0x83;
    box = cur;
    cur.top = 0x6d;      /* the build's row register */
```

`box`'s definition is then a struct copy from a variable that has ~120 further
definitions, and VC6 stops propagating. Measured (object prefix `/tmp/sll16f_`):

| | committed | with `box = cur` |
| --- | --- | --- |
| `r`'s references | 901 | **1,501** (the original's is 1,448) |
| frame | `0x23d4` | **`0x23d4`** |
| emitted | 7,824 | 7,774 |
| LCS | 55.5% | **55.9%** |
| `difflib` | 42.4% | **43.1%** |
| mismatch | **7,920** | 7,991 |
| `MATCH` | **165** | 94 |

The entry block also becomes the original's shape — four constants into four
registers and then four stores (`mov edi,131 / mov ecx,109 / mov eax,80 /
mov edx,420 / mov [box.bottom],edi / mov [box.top],ecx / ... `) against the
original's `mov edi,0x6d / mov eax,0x83 / mov ecx,0x50 / mov edx,0x1a4 / ...`,
where ours previously stored two of them as immediates and dead-stored
`box.bottom` away entirely.

**It is not committed** only because `r` at 1,501 then outranks `page_start`
and `indent` and the aggregate moves to `0x14`, pushing every scalar up by
`0x20` — mismatch and `MATCH` pay for it. Landing this is worth roughly three
LCS points on its own plus whatever the register assignment below is worth, and
it is the single most valuable open state in this lane. To take it, `r` has to
rank below `indent` (169 per dword, i.e. under 1,352 references total) and
above `ok` (151, i.e. over 1,208) — or the ordering has to be steered some
other way. Every attempt to shave `r` by making the page reset partly
fieldwise **destroys the whole shape** (see the rejects below): the struct copy
is load-bearing at both sites.

### A newly quantified residual: the rewind test's register

At all 75 non-section-head page-break sites the original compares
`cmp ebp, ecx` — `sect_start` is loaded into `ecx`, because `eax`, `edx` and
`edi` are taken by `box.right`/`box.bottom`, `box.left` and `box.top`. Ours
emits `cmp ebp, eax` 25 times and `cmp ebp, edx` 47 times, because our
page-break block loads fewer `box` fields and the registers land differently.
That is about 72 index-for-index mismatches plus their loads, and it is
**downstream of the `box` opacity above** — `box = cur` alone moves 9 of the 75
onto `ecx`. Both bodies agree on the nine section-head sites (`cmp ebp, esi`).

### What landed

1. **`nnarr` rides `v`.** `/FAs` showed VC6 already packing them onto one slot
   (`0x44 v,nnarr`); spelling it as `#define nnarr v` and dropping the
   declaration frees the name, and VC6 then puts `passed` below `total`, which
   is the original's relative order. mismatch 7,923 -> 7,922, `MATCH` 162 ->
   163, LCS 54.9% -> 55.5%. (`v`'s last use is the closing block; `nnarr` is
   assigned 0 at the render loop's first page turn, so they never overlap.)
2. **The entry zero-stores are `indent`, `n`, `page_start`.** VC6 emits them in
   source order and the original's is `mov [esp+0x14],ebx / xor esi,esi /
   mov [esp+0x10],ebp / [0x60] / [0x5c] / [0x48]`. With that order our entry is
   the original's exact instruction sequence for indices 8-13 — only the
   displacements differ, and index 13 (`failmask` at `0x48`) matches outright.
   mismatch 7,922 -> 7,920, `MATCH` 163 -> 165. **Initialisation order is
   therefore NOT inert**, contrary to the second and tenth passes; only
   *declaration* order is.

### Measured and rejected this pass

| probe | result |
| --- | --- |
| any fieldwise page reset (hoisted / fall-through / with / without `bottom`, with or without `box = cur`) | `box` collapses, emitted falls to ~6,900, LCS ~42% |
| `if (sect_start != page_start)` | mismatch -2 and `MATCH` +2, but emits `cmp esi,ebp` where the original has `cmp ebp,esi` at all nine head sites — alignment noise, rejected |
| rewind-block statement order, five permutations | `g_report_pages++` first gives `MATCH` +1 but puts it at the FRONT of the emitted block where the original has it last; the committed order already matches the original's block. Noise, rejected |
| `#define nhint X` for `nrun`, `i`, `nnarr`, `narr_cur`, `kind`, `total`, `passed`, `all_total`, `ok` | all frame `0x23d8` (`v` then needs its own dword). `nhint`=`kind` is worth recording: mismatch **7,912** and `MATCH` **173**, better than anything committed, at LCS 44.7% and a broken frame |
| `#define nhint nrun` with `nnarr` or `narr_cur` riding `v` | frame `0x23d8` |
| a duplicated `lines[n].indent = indent;` (a model probe to buy `indent` weight) | **byte-identical** — DSE runs before the layout, so weight cannot be bought with a redundant store |
| out-param declaration order, two permutations | byte-identical. Declaration order stays inert even for tied weights, so the original's out-param order is not a declaration-order effect |
| `unsigned int n` (warns C4018), `register int n` | inert |
| `lines[n].indent` before `lines[n].page` in the line macros | mismatch 7,929 |
| `lines[n].nids = 0` first in the line body | mismatch 8,000, LCS 48.8% |
| `box = title` through a separate template struct | frame `0x23e4` |
| `cur.top = box.top`; seeding `cur.left`/`right`/`bottom` from `box` at entry; the `box` init reordered bottom-first | inert or worse |
| separate `box`/`cur` structs with `box = cur` | frame exact, `box` 743 + `cur` 758, but `page_start` lands at `0x34`; LCS 55.4%, `MATCH` 98 |
| `AppraisalBox r[2]` instead of the struct | LCS +0.1, everything else identical (as the tenth pass found) |

### What a twelfth pass should try

1. **Find what pulls `page_start` and `indent` to `0x10`/`0x14` in the
   original.** It is not weight, not declaration order, not initialisation
   order, not first reference and not first store — all measured. It is worth
   2.4 LCS on its own and it unblocks `box = cur`, which is worth ~3 more. A
   lab function that reproduces the *violation* (a frame where two low-weight
   always-live scalars sit below a high-weight CSE temp) is the thing to hunt;
   `/tmp/sll16e_lab` has the harness and `/tmp/sll16f/slots.py` the census.
2. **`box = cur;`** — keep it in hand. The moment (1) is solved, apply it.
3. The rewind test's register (75 sites) and the out-param slot order (11
   slots) are the two remaining named residuals, both downstream of (1) and (2).

### Lab result: what makes the temp take a slot of its own

`/tmp/sll16f/mklab2.py` (rebuild from this description) emits a scaled model of
this function: `R arr[100]` with a 19-int record, a `struct { B box, cur; }`,
`n`/`ps`/`ind`/`ss`/`ok`/`tot`/`pas`, the same `LINE` macro with its page check,
struct-copy reset, `rand`/`GetString`-shaped calls and a shared `rew_sectK`
block per section. Sweeping the number of sections and the lines per section:

| lines per section | where the `n*0x4c` temp goes |
| --- | --- |
| <= 12 (any number of sections, up to 12) | **never spilled** — VC6 keeps the offset in a register, `ps` takes `0x10` |
| >= 18 | spilled, and it takes `0x10` ahead of `ps` |

With *mixed* section sizes the trigger is sharper still, and it is a pair:

| section sizes | slot `0x10` |
| --- | --- |
| `5,5,5,5,5,5,5,5,40` | `ps` (the temp packs onto `ps`'s slot) |
| `5,5,5,5,5,5,5,5,25` | `ps` |
| `1,6,3,3,3,3,3,40` | `tot` |
| `1,6,3,3,3,3,3,25` | `ps` |
| `1,6,3,3,3,3,3,40,25` | **TEMP** |
| `5,5,5,5,5,5,5,5,40,25` | **TEMP** |

**One big line-run is absorbable; two are not.** Our body has exactly the
pathological pair — section 8's ~40 advice lines and section 9's ~25 hint lines
— which is why our temp gets a slot of its own at the bottom of the frame. So
does the original's, and the original still keeps it at `0x18`, so this is the
shape of the question but not yet the answer. Adding section-local counters to
the lab in `nrun`/`nhint`'s positions does **not** reproduce the original
(the temp still takes `0x10`); neither does `#define nhint nrun` on the real
body, which packs the temp onto `nrun` but leaves it at `0x10` and costs the
frame. The next lever to hunt is whatever lets VC6 absorb the SECOND big
run — if section 9's offset can be made to live somewhere else (the original
is reported to keep it in `ebp` there, though `[esp+0x18]` is still used at
section 9's head), the temp should fall back onto a named local and the
scalars should move down to `0x10`/`0x14`.

## 2026-09-09 (twelfth pass) — `box = cur` lands: four of the original's slots

| | eleventh pass | now |
| --- | --- | --- |
| emitted | 7,824 | 7,769 |
| bytes | 34,224 | 33,440 |
| frame | `0x23d4` exact | `0x23d4` exact |
| first diverging index | 8 | 8 |
| mismatch | **7,920** | 7,992 |
| index-for-index `MATCH` | **165** | 93 |
| true LCS vs the whole original | 55.5% | **57.7%** (lane best) |
| `difflib` alignment | 42.4% | **42.7%** |

### What landed

The eleventh pass's `box = cur;` opacity is committed. It was blocked only by
where the `box`/`cur` aggregate then ranks: at 1,501 references over eight
dwords it outranks `page_start` and `indent` and moves to `0x14`, shifting
every scalar by `0x20`. **The fix is to make the aggregate bigger, not
lighter.** VC6 ranks an aggregate as one object at roughly its references per
dword, so absorbing scalars that would otherwise need slots of their own both
lowers the rank and costs the frame nothing:

```c
    struct {
        AppraisalBox box, cur;
        union { int line_ok; RObj* obj; } u;
        int passed;
        int all_total;
    } r;
```

Eleven dwords carrying 1,611 references rank at ~147, which is below
`indent` (169) and above `total` (140), and the object lands at `0x1c`:

| | `0x10` | `0x14` | `0x18` | `0x1c` | `0x2c` | `0x3c` | `0x40` | `0x44` | `0x48` | `0x4c` |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| original | `page_start` | `indent` | temp | `box` | `cur` | `ok`/`obj` | `passed` | `sect_start` | `failmask` | temp2 |
| ours | temp | `page_start` | `indent` | `box` | `cur` | `ok`/`obj` | `passed` | `all_total` | `total` | `sect_start` |

**`box`, `cur`, `ok`/`obj` and `passed` are now on the original's own
displacements** — 1,448 + 153 + 36 of the original's references land on the
right slot for the first time in this lane. The residual is a three-slot
rotation at the bottom: our `n*0x4c` byte-offset temp holds `0x10`.

The union is load-bearing: giving `obj` a dword of its own breaks the frame
to `0x23d8` (measured three ways below), and `all_total` is the only scalar
whose weight lets the eleven-dword object rank in the window.

`mismatch` and `MATCH` go the wrong way and this is **not** the slots. It is
the shorter instruction stream: `box = cur` on its own, with the aggregate
still at `0x14`, scores 7,989 / 96, i.e. the whole regression is already
there before the slots are fixed, and fixing them recovers none of it. The
committed state's 165 index matches were three lucky alignment runs around
indices 516–574 and 4,753–4,781; `box = cur` shortens the body by 50
instructions early and those runs shift out of phase. Match runs, not match
counts, are the honest reading of that metric on a body this size:

| | matches | runs | longest run |
| --- | --- | --- | --- |
| eleventh pass | 165 | 66 | 18 |
| `box = cur` alone | 96 | 35 | **21** |
| committed | 93 | 36 | 15 |

### The slot rule, re-measured: it is weight, and definition order only breaks ties

The brief for this pass asked whether the order is first-definition
(web-creation) order. **It is not.** Two independent results:

- **Lab (`/tmp/sll16g/lab/gA.c`, `gB.c`).** Ten scalars with weights
  24/16/12/8/6/4/2/3/2/1 plus an array, compiled twice with the *definition*
  order reversed. Both frames come out in descending weight order; the only
  thing that moves is the pair tied at weight 2, and it moves the **other**
  way — the later-defined of the two takes the lower slot. So definition
  order is a reverse tie-break and nothing more.
- **The original's own emission order** contradicts it directly. Its first
  definitions are `page_start` (`xor ebp,ebp`, index 6), `indent`
  (`mov [esp+0x14],ebx`, index 8), `all_passed` (`0x60`), `all_total`
  (`0x5c`), `failmask` (`0x48`), then `box` (`0x1c`) and only then the temp
  (`0x18`, first stored at 0x0044543c) — yet the frame order is
  `page_start`, `indent`, temp, `box`. Neither source order nor emission
  order produces it.

Our object obeys descending weight strictly, aggregates included. Measured by
padding the aggregate (`int zpad[N]`, object prefix `/tmp/sll16g_P*`):

| dwords in `r` | refs / dword | lands between |
| --- | --- | --- |
| 8 | 187.6 | temp (210) and `page_start` (171) |
| 9 | 166.8 | temp and `page_start` (**still above `page_start`**) |
| 10 | 150.1 | `indent` (169) and `ok` (151) |
| 12 | 125.1 | `ok` and `sect_start` (135) |
| 16 | 93.8 | `sect_start` and `v` (91) |
| 24 | 62.5 | `failmask` (62) and `passed` (38) |

The nine-dword row is the one that does not fit refs-per-dword exactly; a
divisor of `size − 1` fits every row but the two extremes. Either way the
rule is monotone in refs per dword and that is enough to steer with.

### The rotation at `0x10`/`0x14`/`0x18` is not reachable from the source

To put `page_start` at `0x10` the weight model needs either the temp below
`indent` (shed 42 of its 210 references) or `page_start` and `indent` above
the temp (add 40 each). Everything tried moves the weight somewhere else
rather than removing it:

| probe | result |
| --- | --- |
| `struct { int page_start, indent; } q;` (2 dwords, 340 refs = 170/dword) | ranks *below* the temp; lands at `0x14`/`0x18`, layout unchanged, LCS 57.8% |
| hoist `rand() % 5` and `GetString()` into locals in the section-8 line body | temp 210 -> 96 and it falls to `0x50`, but the two new locals pack onto `v` at 166 refs and take `0x10` instead; LCS 52.9% |
| absorb `sect_start` into the aggregate (11 dwords) | frame `0x23d0` — the freed dword is repacked and the temp grows to 237; LCS 44.4% |
| absorb `sect_start` + `failmask` (12 dwords) | frame `0x23d0`, LCS 50.0% |
| `ok` as a plain member with `obj` left outside (11 dwords) | frame `0x23d8`; `obj` takes its own dword and lands at `0x10` with 203 refs |
| the same with `total` or `v` as the eleventh member | frame `0x23d8` / `0x23dc` |
| the aggregate without `box = cur` (r = 1,011) | ranks at `0x28`; LCS 52.8% |
| the hoisted `cur = box` written fieldwise | `box` collapses: 7,082 emitted, LCS 42.3% |
| the fall-through `cur = box` written fieldwise | 6,857 emitted, LCS 42.0% |

Both fieldwise rows are worth reading for one thing: they take `page_start`
to **252 references and slot `0x10`**, and the byte-offset temp disappears
from the frame entirely (VC6 keeps it in registers). So the temp's slot only
exists because the struct-copy reset ties up `eax`/`ecx`/`edx`/`edi` at every
one of the ~122 sites — and the original has the same struct copies, the same
temp, a *heavier* temp (222), and still puts `page_start` at `0x10`. That is
the whole unexplained residual, stated as sharply as this lane can state it.

Best remaining guess, untested: VC6 ranks a shared slot by its heaviest
**web** rather than by the sum of the slot's references. The original's
`0x18` carries the temp plus `nrun` plus `nhint`; if its temp is many short
per-line webs none of which reaches `indent`'s 171, the original's frame is
weight-ordered after all and ours is not because our temp is one long web.
A lab that can count webs rather than references is what would settle it.

### The rewind test's register, re-measured

The original compares `cmp ebp, ecx` at all 75 non-head sites. Before this
pass we emitted `ebp, eax` x25 and `ebp, edx` x47; now `ebp, eax` x25,
`ebp, edx` x38, **`ebp, ecx` x9**, exactly the nine the eleventh pass
predicted `box = cur` would move. The remaining 66 want the original's
five-load page-break block (`box.right`, `sect_start`, `box.left`,
`box.top`, `box.bottom`) so that `sect_start` is pushed into `ecx`; ours
loads fewer `box` fields in the hoisted copy because it keeps the hoisted
`cur.bottom` store that the original dead-stores away.

### What a thirteenth pass should try

1. **The three-slot rotation**, above — worth ~2.4 LCS and it is the last
   structural residual in the frame. Do not retry weight arithmetic on the
   temp; hunt the web-count hypothesis instead.
2. **The hoisted `cur.bottom` store.** Killing it (the original does) both
   closes one store per site and should push `sect_start` into `ecx` at the
   66 remaining rewind tests.
3. **The 316-instruction shortfall** is now entirely in the render/input
   tail; the second pass's note about `RepLine*` cursors still governs it.

## 2026-09-09 (thirteenth pass) — the build phase was 26 NARR calls short

| | twelfth pass | now |
| --- | --- | --- |
| emitted | 7,769 | **8,148** (original 8,085) |
| bytes | 33,440 | **34,928** (original 34,662) |
| frame | `0x23d4` exact | `0x23d4` exact |
| first diverging index | 8 | 8 |
| mismatch | 7,992 | **7,956** (lane best) |
| index-for-index `MATCH` | 93 | **129** |
| true LCS vs the whole original | 57.7% | **59.5%** (lane best) |
| `difflib` alignment | 42.7% | **50.2%** (lane best) |

One change, and it improves **every** metric the lane tracks at once — the
first state in this lane that costs nothing.

### The 316-instruction shortfall was never in the render/input tail

The ninth and twelfth passes both recorded the shortfall as "entirely in the
render/input tail". It is not, and the tail needs no work at all. Anchor both
streams on `if (nhint == 0) n--;` (ours index 7,479, the original's 7,796 at
0x0044d744): the original's tail is 289 instructions and ours was 290. The
whole 317-instruction deficit was already present when the build ended.

The tail was checked instruction by instruction against the original from
0x0044d744 to the `mov eax,1 / pop ebx / add esp,0x23d4 / ret` and is
essentially transcribed: the frame loop, the title `NewPrintCent` rectangle,
the `lea edi,[esp+…+0xbc]` / `lea ebp,[esp+…+0x20c4]` walking pointers, the
eight-way unrolled narration append (`[edi+4]`..`[edi+0x20]`) and the
narration-file block all line up one for one. The only differences found are
scheduling noise plus one arm order: the original lays the
`narr_cur >= nnarr` arm as the fall-through (`cmp eax,edx / jl`), ours the
`narr_cur < nnarr` arm (`cmp edx,eax / jge`), and the original repeats
`IsNarrationPlaying` in **both** arms where ours has it in one. Worth ~4
instructions; not chased this pass.

### The census tool that found it, and what it said

A call census by callee is decisive on a body this size and costs one grep:

| | original | ours (twelfth) | ours (now) |
| --- | --- | --- | --- |
| `rand` (0x49e4b2) | 129 | 128 | **129** |
| `GetString` (0x498f50) | 112 | 104 | **112** |
| total calls | 292 | 283 | **292** |
| narration-queue stores `mov [esp+<r>*4+0x74],<id>` | 76 | 50 | **76** |

Aligning the two *sequences* of pushed `GetString` ids named the eight missing
calls outright — `0x150`, `0x15a`+`0x133`, `0x165`+`0x164`, `0x168`+`0x167`,
`0x16a` — and the narration census explained them: **every `-1` advice line in
section 8 is queued for narration and our source queued only six of them.**
Twenty-six `NARR` calls were missing. They are not merely 260 instructions of
content: a `NARR(id)` ends the case body with a store of a *different*
constant, which is what stops VC6 cross-jumping the three case blocks of
`switch (failmask & 0x30)`, `& 0x3000`, `& 0x18000` and `& 0x60000` into one
shared `call GetString` tail. That is where the eight lost calls went.

The rule, read off the object: **the `-1` advice line always gets a `NARR`;
its `-3` continuation never does** (`0x133`, `0x15d`, `0x231`, `0x172`,
`0x154`, `0x236`, `0x238` have none). The section header `0x230` has none.

**Original quirk, reproduced.** The verdict block's three arms are asymmetric:
`0x14f`+NARR then `0x150`+NARR in the first arm, `0x151`+NARR then `0x150`
with **no** NARR in the second, `0x153`+NARR then `0x154` with none in the
third. Addresses 0x00448025/0x0044809c/0x00448130/0x00448191 against
0x00448249/0x004482c0/0x00448354 settle it.

### Confirmation from the frame weights

The slot weights moved onto the original's almost exactly, which is
independent evidence that the transcription is now right:

| | temp | `page_start` | `indent` | `box`+`cur` |
| --- | --- | --- | --- | --- |
| original | 222 | 173 | 171 | 1,448 |
| ours (twelfth) | 210 | 171 | 169 | — |
| ours (now) | **221** | **172** | **170** | 1,634 (in the 11-dword `r`) |

### What it costs

Nothing on the lane's metrics. It does take the body 63 instructions
**over** the original (8,148 vs 8,085), so `tools/audit.py` now prints
`ESCAPES` — that is only the audit truncating our body to the original's
extent and finding a branch past the cut. The overshoot has a known cause
that is larger than itself: the 82 hoisted `cur.bottom` stores below.

## Thirteenth pass, item 1: the web-ranking hypothesis is REFUTED

The twelfth pass's best remaining guess was that VC6 ranks a shared stack slot
by its heaviest **web** rather than by the sum of the slot's references. It
does not. Lab in `/tmp/sll16h/lab` (`mkweb.py`, `mkloop.py`, `build.sh`);
each function has an array (so VC6 is in the interleaved mode this body is in),
`nl` long-lived locals to soak up the callee-saved registers, one rival scalar
`R` with a single long web, and `nw` scalars `w1..wN` with disjoint live ranges
that VC6 packs onto **one** slot (the `/FAs` equates show them sharing).

| variant | webs x refs | sum | max web | `R` | lower slot |
| --- | --- | --- | --- | --- | --- |
| `wE` | 6 x 5 | 30 | 5 | 60 | `R` |
| `wH` | 6 x 9 | **54** | 9 | 60 | `R` |
| `wI` | 6 x 11 | **66** | 11 | 60 | **the packed slot** |
| `wF` | 6 x 15 | 90 | 15 | 60 | the packed slot |
| `wJ` | 12 x 6 | 72 | **6** | 60 | the packed slot |
| `wA` | 6 x 10 | 60 | 10 | 40 | the packed slot |

The crossover is exactly at the **sum**: 54 loses to 60, 66 beats it, and
`wJ` beats a rival of 60 with a heaviest web of 6. So splitting the `n*0x4c`
temp into per-line-site webs cannot move it — a split preserves the total.
**Do not spend another pass on the temp's weight.**

Loop-frequency weighting was killed in the same lab (`mkloop.py`): a scalar
with 11 static references *inside a ten-iteration loop* ranks below three
straight-line scalars with 15, and the loop counter itself takes the last
scalar slot. The ranking key is the plain static reference count, summed over
the slot, with no loop weighting and no web decomposition. Combined with the
twelfth pass's exhausted weight arithmetic, **the three-slot rotation at
`0x10`/`0x14`/`0x18` has no known lever left**, and the original's frame
remains a genuine violation of the rule its own compiler follows everywhere
else.

## Thirteenth pass, item 2: the hoisted `cur.bottom` store — measured, no lever

Quantified exactly (`/tmp/sll16h/hoist2.py`, `hoist3.py`): 123 hoisted
page-break blocks in ours, **82 store `cur.bottom`**; the original's 124 store
it **zero** times, and both load `box.bottom` ~120 times — the original's
surviving dead load is the tell. Median hoisted block is 9 instructions
against the original's 8: exactly the one extra store. The register census
follows from it — the original compares `cmp ebp,ecx` at all 75 non-head
sites; ours is `edx` x38, `eax` x25, `ecx` x9, because our block stores
`cur.bottom` from `edx` and keeps `ecx`/`edx` live across the branch, so
`sect_start` lands in `eax` and the shared rewind block has to **reload** it
(`mov esi,[esp+0x4c]`) where the original just uses `ecx`.

The 41 sites that *do* drop the store are precisely section 8 and the closing
lines (contiguous indices 2,560..4,497 plus 7,540..7,750), i.e. exactly where
`cur.bottom` has a maintained memory home from the line-end form. Everything
tried to extend that to sections 1-7 and 9:

| probe | emitted | mismatch | `MATCH` | LCS | note |
| --- | --- | --- | --- | --- | --- |
| `cur.bottom = cur.top + 0x16;` at every `rew_sectK` head | 7,762 | 8,011 | 74 | 53.5% | identical to doing it at `rew_sect8` alone |
| `cur.bottom = box.bottom;` at every `rew_sectK` head | 7,761 | **7,959** | **126** | 53.5% | mismatch/`MATCH` improve, LCS -4.2; and it makes **all 120** blocks store the bottom and loses the nine `ecx` compares — alignment noise, rejected |
| hoisted copy fieldwise, `right`/`left`/`top` only (the original's exact two stores) | 6,902 | 8,030 | 55 | 43.0% | `box` collapses |
| the same plus `bottom` | 6,919 | 7,969 | 116 | 41.5% | `box` collapses |
| `int ss_ = sect_start;` read before the hoisted copy | — | — | — | — | byte-identical |
| a redundant `cur = box;` in the rewind arm as well | 7,387 | 8,014 | 71 | 44.7% | |
| `if (page_start == sect_start) {…} else goto rew;` | 7,769 | 7,990 | 95 | 57.7% | structurally identical (same 82 stores, same cmp registers); ±2 of scheduling noise, not taken |

So the store cannot be removed without losing `box`'s opacity, which is worth
far more. It stays, and it is the whole 63-instruction overshoot.

### The narration arm order, landed

The original lays the `narr_cur >= nnarr` arm as the fall-through
(`cmp eax,edx / jl <play arm>` at 0x0044da2b); ours had the play arm there.
Rewriting the source as `if (narr_cur >= nnarr) { if (!IsNarrationPlaying())
UpdateHelpBar(); } else if (...) { ...play... }` puts the two arms in the
original's order: LCS 59.4% -> **59.5%**, difflib 50.0% -> **50.2%**, mismatch
and `MATCH` unchanged. (Ours already had `IsNarrationPlaying` in both arms;
the eleventh-hour reading that it did not was wrong.) Writing the test as
`nnarr <= narr_cur` instead gets the compare's operand order right
(`cmp eax,edx`) but then the branch is `jg` where the original has `jl`, and
it scores identically -- two instructions of register-naming noise either way.

### What a fourteenth pass should try

1. **The hoisted `cur.bottom` store**, above — 82 instructions and 66
   wrong-register rewind tests, but every known spelling trades `box`'s
   opacity for it. A shape that keeps the struct copy and still lets DSE
   reach across the `jne` is what it needs.
3. The three-slot rotation is **not** worth another pass on weight arithmetic
   (item 1 above closes that off); only a non-weight mechanism would move it.

## 2026-09-09 (fourteenth pass) — the section heads check `cur.bottom` BARE, and the hoisted store dies

| | thirteenth pass | now |
| --- | --- | --- |
| emitted | 8,148 | **8,133** (original 8,085) |
| bytes | 34,928 | **34,784** (original 34,662) |
| frame | `0x23d4` exact | `0x23d4` exact |
| first diverging index | 8 | 8 |
| mismatch | **7,956** | 7,976 |
| index-for-index `MATCH` | **129** | 109 |
| true LCS vs the whole original | 59.5% | **62.5%** (lane best) |
| `difflib` alignment | 50.2% | **53.5%** (lane best) |
| hoisted blocks that store `cur.bottom` | 82 of 123 | **0 of 124** (original: 0 of 124) |

**The trade.** LCS +3.0 and difflib +3.3, both lane bests, and the emitted
count and byte count both move toward the original; mismatch +20 and
index-for-index `MATCH` -20 (runs 60 -> 48, longest run 15 either way). That is
the same alignment-shift cost the twelfth pass took: the body shortens by 15
instructions early on and the index-for-index runs slide out of phase. LCS is
the stable measure on a body this size (see the eighth pass) and the structural
census below is not an alignment artefact at all.

### The thirteenth pass's item 1 is solved, and the mechanism is store-to-load forwarding

The brief for this pass hypothesised dead-store elimination out of the struct
copy. That is the right shape, but the enabling condition was in a place no
earlier pass had looked: **the original's nine section labels are all entered
with `cur.bottom` already computed, and the head's own page check is a bare
`cmp eax,0x1b5` that only reads it.** At every one of the nine, the
`lea eax,[edi+0x16]` sits ABOVE the label — outside the block the rewind jumps
back to — and the compare is below it:

| section | the `lea` (above the label) | the label | the bare `cmp eax,0x1b5` |
| --- | --- | --- | --- |
| 1 | `mov [esp+0x28],eax` 0x0044541e (the `box` init: `eax` = 0x83) | 0x00445422 | 0x00445440 |
| 2 | 0x00445530 | 0x00445539 | 0x00445566 |
| 3 | 0x00446727 | 0x0044672e | 0x00446760 |
| 4 | 0x00446b6a | 0x00446b71 | 0x00446ba3 |
| 5 | 0x00446fa4 | 0x00446fab | 0x00446fdd |
| 6 | 0x004473db | 0x004473e2 | 0x00447414 |
| 7 | 0x00447856 | 0x0044785d | 0x0044788f |
| 8 | 0x00447e6c | 0x00447e73 | 0x00447e88 |
| 9 | 0x0044acb8 | 0x0044acbb | 0x0044acd0 |

A computation cannot be hoisted above a label into one predecessor only, so
this is the source, not scheduling. And it explains the missing store exactly.
On the rewind path the head's read of `cur.bottom` is satisfied by the value
the hoisted `cur = box;` just loaded — `mov eax,[esp+0x28]` at 0x004456bb,
still live in `eax` through the ten-instruction shared rewind block and the
`jmp` — so the copy's *store* has no consumer left and dies while its *load*
survives. That is the load-survives/store-killed signature the lane has been
staring at since the ninth pass, and it is store-to-load forwarding followed
by DSE, not DSE alone.

### What landed

1. **`TEXT_LINE_H`** — a section-header line whose page check is the bare
   `if (cur.bottom > 0x1b5)` (`PAGE_CHECK8`'s test) and whose end advances
   `cur.top` but does **not** recompute `cur.bottom`. All nine header lines
   use it; the per-line checks inside sections 1-7 and 9 keep `PAGE_CHECK`'s
   recompute, which the object still demands (0x0044569d and friends).
2. **`cur.bottom = cur.top + 0x16;` as the last statement of every section**,
   inside the guarded block: at the end of section 1's title-line `if`, at the
   end of each of sections 2-7's epilogues (after `all_passed += passed`), and
   after the closing block's `indent -= 0x30`. Section 8 already had one above
   its label from the eighth pass; that is the same statement, and it is now
   the uniform shape rather than a special case.

### The census, ours against the original

| | original | thirteenth pass | now |
| --- | --- | --- | --- |
| hoisted page-break blocks | 124 | 123 | **124** |
| ...that store `cur.bottom` | 0 | 82 | **0** |
| rewind test `cmp ebp,ecx` | 73 | 10 | **33** |
| rewind test `cmp ebp,eax` | 0 | 23 | **0** |
| rewind test `cmp ebp,edx` | 0 | 38 | 38 |
| section-head test `cmp ebp,esi` | 9 | 9 | 9 |
| `cmp [esp+0x38],0x1b5` | 28 | 28 | **28** |

The typical page-break block is now the original's, instruction for
instruction, with one displacement wrong — and that displacement is the
three-slot rotation, not a new residual:

```
   original 0x004456a7            ours (index 257)
   mov eax,[esp+0x24]  box.right  mov eax,[esp+0x24]
   mov ecx,[esp+0x44]  sect_start mov ecx,[esp+0x4c]   <- the rotation
   mov edx,[esp+0x1c]  box.left   mov edx,[esp+0x1c]
   mov edi,[esp+0x20]  box.top    mov edi,[esp+0x20]
   mov [esp+0x34],eax  cur.right  mov [esp+0x34],eax
   mov eax,[esp+0x28]  box.bottom mov eax,[esp+0x28]
   cmp ebp,ecx                    cmp ebp,ecx
   mov [esp+0x2c],edx  cur.left   mov [esp+0x2c],edx
   jne <shared rewind>            jne <shared rewind>
```

The 38 sites that still compare `ebp,edx` are the ones where VC6 orders the
four `box` loads differently and `sect_start` lands in `edx`; they are register
naming, not a missing instruction.

### Where the remaining 48 instructions are

8,133 against 8,085. The `lea <reg>,[edi+0x16]` census is 76 in ours against
102 in the original and the `cur.bottom` stores are 153 against 147, so the
two forms are close but not identical — some of sections 1-7's per-line checks
in the original may also be line-end rather than check-site. That is the next
thing to census, and it is now a much smaller question than it was.

### Where the remaining 48 instructions are, zone by zone

Aligning the 122 `push <string id>` instructions of the two streams (they are
in the same order except section 9's last two hint groups, which the original
lays out `0x226` then `0x21d` and ours the other way round) gives a running
delta with no `esp` tracking and no guesswork. It is the sharpest zone tool
this lane has had; rebuild it from this paragraph.

| landmark | original | ours | delta |
| --- | --- | --- | --- |
| `0x12c` section 1's title | 84 | 76 | −8 |
| `0x143` end of section 2 | 1,117 | 1,129 | +12 |
| `0x147` section 6's header | 2,019 | 2,011 | −8 |
| `0x14b` section 7's first line | 2,382 | 2,343 | **−39** |
| `0x230` section 8's header | 2,657 | 2,610 | −47 |
| `0x173` end of section 8 | 4,954 | 4,928 | −26 |
| `0x174` section 9's header | 5,367 | 5,341 | −26 |
| `0x21c` section 9's last group | 7,589 | 7,649 | **+60** |
| end of the build | 7,796 | 7,844 | **+48** |

Two localised residuals, and they very nearly cancel:

1. **Section 9 is +86 over ~37 hint lines — a flat +2 per line**, and both
   instructions are downstream of the three-slot rotation. Diffed at
   `0x17c` (original 5,435, ours 5,411): the original's page-break block does
   `cmp [esp+0x10],ecx` — `page_start` straight from its home, because `ebp`
   holds the line byte offset in section 9 — where ours loads it
   (`mov ecx,[esp+0x14] / cmp ecx,edx`); and the original keeps `box.left` in
   `edx` across the branch where ours has to reload it in the fall-through
   arm. Writing the test as `if (sect_start != page_start)` does **not** move
   it (measured this pass: identical 8,133 emitted, LCS 62.5% -> 62.0%),
   which is the eleventh pass's result again under the new shape.
2. **Section 7's header is ~30 instructions SHORT.** The original builds
   `&lines[n].page`, `&lines[n].indent`, `&lines[n].ok` and `&lines[n].text`
   as `lea eax,[esp+ecx+0x94]` pointer temps and stores through them
   (`mov [esp+0x18],eax / mov [eax],edx`), where ours indexes each field
   directly. Those are the original's `0x40` and `0x58` pointer temps. This is
   the second pass's `RepLine*` cursor question in a much narrower form — it
   is only some sites, not the whole build.

### What a fifteenth pass should try

1. **The three-slot rotation** at `0x10`/`0x14`/`0x18` — unchanged, and still
   the largest single residual (worth ~2.4 LCS by the tenth pass's remap
   measurement, plus the 86 instructions of residual 1 above, which is
   downstream of it). Do NOT retry weight arithmetic or web decomposition:
   the thirteenth pass refuted both, and the operand-order lever was retried
   and refuted again this pass. Only a non-weight mechanism would move it.
2. **Section 7's pointer temps**, residual 2 above — a self-contained ~30
   instructions with a named mechanism.
3. The 38 `cmp ebp,edx` rewind tests, which are downstream of the `box` load
   order in the hoisted block, and section 9's last two hint groups, which
   the original emits in the opposite order.

## 2026-09-09 (fifteenth pass) — the nine section HEADS specialise their rewind arm

| | fourteenth pass | now |
| --- | --- | --- |
| emitted | 8,133 | 8,181 (original 8,085) |
| bytes | 34,784 | 34,928 (original 34,662) |
| frame | `0x23d4` exact | `0x23d4` exact |
| first diverging index | 8 | 8 |
| mismatch | 7,976 | **7,845** (lane best) |
| index-for-index `MATCH` | 109 | **240** (lane best) |
| match runs / longest | 48 / 15 | **104 / 17** |
| true LCS vs the whole original | 62.5% | **64.5%** (lane best) |
| `difflib` alignment | 53.5% | **55.1%** (lane best) |

One change. It improves **every** measure the lane tracks at once — mismatch,
index matches, match runs, LCS and difflib — with the frame still exact, so
there is no trade to state. It is the second such state in the lane (the
thirteenth pass's missing `NARR`s was the first).

### The section heads do not `goto rew_sectK`

At a section head `sect_start` has just been assigned `n` and `indent` has not
moved since the section's own `lines[n].indent = indent;`, so two of the
general rewind block's four statements are provably no-ops there:

```c
    page_start = sect_start;              /* == page_start = n            */
    n          = sect_start;              /* no-op at a head              */
    indent     = lines[sect_start].indent;/* no-op at a head              */
    g_report_pages++;
    goto sectK;
```

The original emits only the two that survive, **inline**, at all nine heads,
and jumps at the SECTION label rather than at the shared rewind block:

| head | the inline arm | jumps to |
| --- | --- | --- |
| s1 0x0044546c | `mov ecx,[pages] / mov ebp,esi / inc ecx / mov [esp+0x10],ebp / mov [pages],ecx` | `0x445422` = `sect1` |
| s2 0x00445594 | `jne 0x44546c` — into section ONE's copy | (the original's label bug) |
| s3 0x00446795 | `mov ecx,[pages] / mov ebp,esi / inc ecx / mov [pages],ecx` | `0x446a3f`, `rew_sect3`'s tail |
| s4 0x00446bd8 | as s3 | `0x446e7c` |
| s5 0x00447012 | as s3 | `0x4472b3` |
| s6 0x00447449 | as s3 | `0x44772e` |
| s7 0x004478c4 | as s3 | `0x447d44` |
| s8 0x00447eb8 | as s1 (with the home store) | `0x447e73` = `sect8` |
| s9 0x0044ad00 | as s1 | `0x44acbb` = `sect9` |

Sections 3–7 are guarded, so `goto sectK` lands on the section's own `if`, and
the arm tail-merges with the shared rewind block's suffix — which is why every
`rew_sectK` block ends `mov [esp+0x10],ebp / mov ecx,[FLAGS] / test / jne / jmp`:
**the block re-tests the section guard.** Sections 1, 8 and 9 have no guard to
re-test (1 and 8 are unconditional at the label, 9 likewise) so they keep the
`page_start` home store inline instead.

Written as `PAGE_CHECK_H`, used by all nine `TEXT_LINE_H` heads, with
`rew_sect1` deleted (section one is one line long, so nothing else reaches it —
and the original has no such block either).

### It also closed the fourteenth pass's residual 2 for free

The fourteenth pass measured section 7's header at ~30 instructions SHORT and
named the mechanism: the original materialises `&lines[n].page`, `.ok`,
`.step`, `.text`, `.colour`, `.bar`, `.value`, `.mark`, `.range`, `.nids` as
`lea` temps, spills each one to a stack slot and stores through the register,
where ours indexed each field directly. Ten of the original's fourteen
`lea <reg>,[esp+<reg>+K]` / `mov [esp+N],<reg>` address-spill pairs are in that
one block (the other four are the `&lines[n].indent` pre-store at the heads of
sections 2, 8 and 9 and one `&lines[n].nids` in the closing block).

**A source-level pointer temp does not produce it.** An `int* p` with
`p = &lines[n].F; *p = v;` for every field is **byte-identical** to the
committed build: `p`'s address is never taken, so copy propagation turns each
pair back into a direct indexed store and DSE removes `p`. The spills are an
allocator artefact — VC6 spills the address on definition and then
*rematerialises* it at the use rather than reloading, so the spill is dead and
survives only because it is inserted after DSE has run.

With the head arm right, our object reproduces the whole block anyway:
section 7's header is now the original **instruction for instruction** from
`mov ecx,[esp+0x4c]` through the eleven field stores, all ten address spills
included, with only the displacements differing (our spill slot is `0x58`
where the original rotates between `0x18`, `0x3c`, `0x40`, `0x4c` and `0x54`).
The zone delta across it went from −19 to +1. So residual 2 is closed, and the
lesson is the general one this lane keeps re-learning: **an allocator artefact
inside a block is fixed by getting the block's control flow right, not by
writing the artefact into the source.**

### The censuses after the change

| | original | ours |
| --- | --- | --- |
| hoisted page-break blocks that store `cur.bottom` | 0 | **0** |
| section-head rewind test `cmp ebp,esi` | 9 | **9** |
| `cmp [esp+0x38],0x1b5` (section 8's memory checks) | 28 | **28** |
| `lea <reg>,[edi+0x16]` | 107 | 114 |
| stores to `cur.bottom` | 147 | 153 |
| calls | 292 | **292** |
| `rand` / `GetString` | 129 / 112 | **129 / 112** |
| non-head rewind test `cmp ebp,ecx` | 75 | 37 (+36 `cmp ebp,edx`) |
| section-9 rewind test `cmp [esp+0x10],ecx` | 39 | 0 (see below) |

### Measured and rejected: reordering the fall-through arm

The page reset's fall-through arm is `g_report_pages++; page_start = n;
cur = box;`. Three permutations, object prefix `/tmp/sll16j_`:

| arm order | emitted | mismatch | `MATCH` | runs | LCS | difflib |
| --- | --- | --- | --- | --- | --- | --- |
| **committed** `pages++, ps, copy` | 8,181 | 7,845 | 240 | 104 | **64.5%** | **55.1%** |
| `ps, pages++, copy` (all three macros) | 8,165 | **7,779** | **306** | 97 | 63.7% | 54.6% |
| `ps, pages++, copy` (`PAGE_CHECK_H` only) | 8,165 | 7,777 | 308 | 97 | 64.1% | 54.9% |
| `ps, pages++, copy` (`PAGE_CHECK8` only) | 8,181 | 7,835 | 250 | 114 | 64.2% | 54.9% |
| `ps, pages++, copy` (`PAGE_CHECK` only) | 8,181 | 7,845 | 240 | 104 | 64.5% | 55.1% (byte-identical) |
| `copy, pages++, ps` | 8,172 | 7,832 | 253 | 94 | 63.2% | 53.1% |
| `pages++, copy, ps` | 8,181 | 7,841 | 244 | 103 | 64.5% | 55.1% |

The whole effect sits in `PAGE_CHECK_H`, and it is **rejected on structure**:
under it VC6 hoists `page_start = n` and its home store ABOVE the `jne`, so the
head's fall-through arm shrinks to 8 instructions where the original's is 10
(0x004478d8: `mov eax,[pages] / mov edx,[box.bottom] / mov [cur.left],ecx /
mov ecx,[box.right] / inc eax / mov ebp,esi / mov [pages],eax /
mov [esp+0x10],ebp / mov [cur.right],ecx / mov [cur.bottom],edx`) and the head
zone's delta goes from −4 to −7. It buys 68 index matches and costs 0.4 LCS by
shortening the body 16 instructions early; the committed order is the one whose
emitted arm has the original's shape, and it holds the LCS and difflib bests.

### The zone table after this pass

Both streams anchored on their 122 `push <string id>` instructions
(`/tmp/sll16j/side.py`, rebuildable from the fourteenth pass's description):

| landmark | original | ours | delta |
| --- | --- | --- | --- |
| `0x12c` section 1's title | 84 | 83 | −1 |
| `0x131` section 2's header | 146 | 146 | **0** |
| `0x13e` section 2's last statistic | 1,002 | 1,016 | +14 (a flat **+1 per line**) |
| `0x144` section 3's header | 1,260 | 1,269 | +9 (**−4** at each guarded head) |
| `0x147` section 6's header | 2,019 | 2,022 | +3 |
| `0x14a` section 7's header | 2,295 | 2,295 | **0** |
| `0x14b` section 7's first line | 2,382 | 2,383 | +1 |
| `0x230` section 8's header | 2,657 | 2,651 | −6 |
| `0x173` end of section 8 | 4,954 | 4,969 | +15 |
| `0x236` in the closing block | 5,193 | 5,225 | +32 (**+17** in one step) |
| `0x174` section 9's header | 5,367 | 5,388 | +21 |
| `0x228` section 9's last line | 7,840 | 7,926 | +86 (a flat **+2 per line**) |
| end of the build | 7,796 | 7,892 | **+96** |

## CLOSING ASSESSMENT

### What the function is

`RunAppraisalScreen` (0x004453a0, 8,085 instructions, 34,662 bytes, a
`0x23d4`-byte frame taken through `__chkstk`) is the whole park-appraisal
report screen in one function: it **builds** a paginated list of up to 100
report lines in a stack array, **renders** the current page, and **runs its own
input loop** until `g_report_open` goes to zero. It returns 1 if every line
passed, 0 otherwise, and 0 immediately if `ScriptRunning()`.

The build is nine sections of straight-line code, each guarded by bits of the
control word at 0x00665ff8: the title, what the park holds, scenery, food,
shops, visitors, the park at work, the advice chain, and the hints. Each
graded statistic counts something, compares it with a goal from the table at
0x0066600c, sets a bit of `failmask` on failure, and writes one line whose
phrasing is chosen by a two-bit field of the control word and whose mark
sprite is `rand() % 5`. Section 8 turns `failmask` into prose advice; section
9 turns it into randomly phrased hints. Six original bugs are reproduced and
commented (section 2's first line restarting at section one's label; section
3's third guard bit with no statistic behind it; section 6's first statistic
advancing `y` without writing a line; section 6's third statistic graded
against the second's goal; the closing lines restarting at section nine's
label; string id `0x16e` skipped).

### The state this lane leaves

| | |
| --- | --- |
| emitted | 8,181 / 8,085 (audit truncates ours to 8,085i / 34,583B) |
| bytes | 34,928 / 34,662 |
| frame | **`0x23d4` exact** |
| first diverging index | 8 |
| mismatch | 7,845 |
| index-for-index `MATCH` | 240, in 104 runs, longest 17 |
| true LCS vs the whole original | **64.5%** |
| `difflib` | 55.1% |
| audit / relocs / `/W3` | `PASS` (WIP, ESCAPES) / zero `MISMATCH` / clean |

### What was proved along the way

**About the function.** The line record is 0x4c bytes / 19 dwords with seven
trailing narration ids; the frame is `lines[100]` at `0x94`, `namebuf` at
`0x1e44`, `textbuf` at `0x1ec4` and `narr[200]` at `0x20c4` over 33 scalar
dwords. There is no separate `y`: the build's row is `cur.top`, kept in `edi`
for the whole build. `box` and `cur` are adjacent and never address-taken.
Sections 1–7 and 9 recompute `cur.bottom` at the page check; section 8 and the
closing lines maintain it at the LINE END and check it bare — proved from
0x00448661, where the `lea` sits above the next arm's `failmask` test. All
nine section labels are entered with `cur.bottom` already live, which is why
the hoisted page reset's `cur.bottom` store is dead at all 124 sites. Every
`-1` advice line is queued for narration and its `-3` continuation is not.

**About VC6 /O2.** Eleven results worth carrying to other lanes:

1. **A single-definition struct local is always constant-folded and
   dead-stored**, at any size of body, even with its address passed out. The
   only source-level cure is a second *definition*: seed `cur`, then
   `box = cur;`. That one line took `box`+`cur` from 901 references to 1,501
   and turned every page-break site's four `box` reloads on.
2. **Stack slots are ordered by DESCENDING reference weight per dword, lowest
   address first**, with an aggregate ranked as one object at roughly its
   references divided by its size in dwords — and the presence of an array is
   what makes aggregates interleave with the scalars instead of sitting on
   top. Declaration order, initialisation order, renaming and `register` are
   all inert *because none of them changes a reference count*; the count is
   the only lever.
3. **The weight key is the plain static reference count summed over the slot**
   — no loop-frequency weighting, no web decomposition. Refuted in a lab where
   six webs summing to 66 beat a rival of 60 while six summing to 54 lost.
4. **Definition order is only a reverse tie-break** for equal weights (the
   later-defined of a tied pair takes the lower slot).
5. **`/FAs` is the frame map.** One equate per named local, CSE temps as
   `-NNNN+[esp]`, `esp_off = frame_size + equate + 16`; two names on one
   equate is VC6 telling you it packed them. A *linear* `esp`-delta tracker
   (reset at every branch target) reproduces it on a stripped binary to within
   5%, which is how the ORIGINAL's slot weights were finally measured.
6. **A whole-struct copy is not four field copies.** VC6 lowers it as four
   memory-to-memory moves and refuses to constant-propagate into them; writing
   any one field of the reset fieldwise collapses the entire opacity.
7. **Store-to-load forwarding runs before DSE**, and it can kill a store out
   of a struct copy while leaving the load behind. That surviving dead load is
   the diagnostic.
8. **Spill stores are inserted after DSE**, so a dead spill of an address VC6
   then rematerialises is normal output and cannot be written into the source:
   a source-level `int* p` is copy-propagated away, byte-identically.
9. **Jump threading specialises a shared block's callers.** Where a caller can
   prove the block's leading statements are no-ops it duplicates the survivors
   and jumps into the middle — which is what the nine section heads do, and
   what this pass had to write out by hand.
10. **A memory-homed variable with a cached register copy** looks like a store
    at every definition and almost no reloads (`page_start`: 133 stores, 2
    reloads). Do not mistake it for two variables.
11. **`failmask |= K` is spelled two ways** — `mov/or al,K/mov` below 0x10000
    and `or dword ptr [mem],K` above — from the same source.

### The exact remaining deltas

**+96 instructions, +266 bytes, in three zones.**

1. **Section 9: +65 over ~33 hint lines, a flat +2 per line.** Both are in the
   page-break block. (a) The original compares `cmp [esp+0x10],ecx` —
   `page_start` straight out of its home, 39 times — because `ebp` holds the
   line byte offset in section 9; ours loads it (`mov ecx,[esp+0x14] /
   cmp ecx,edx`), one instruction more. (b) The original keeps `box.left` in
   `edx` across the `jne` and the fall-through arm stores it from there; ours
   assigns `box.left` to the register that then takes `sect_start`, so the arm
   reloads it. Both follow from the four `box` loads plus `sect_start` being
   scheduled into four registers in the original and needing five in ours, and
   that is downstream of the three-slot rotation. `if (sect_start != page_start)`
   has been measured and refuted three times.
2. **The closing block: +17 in one step, at 0x0044aa2d.** The original
   tail-merges one no-advance line's fall-through arm with the previous site's
   (`je 0x44aa2d / jmp 0x44abd8`); ours duplicates the 17-instruction arm
   because our two arms differ by a register. Not a missing construct.
3. **Section 2: +1 per line, 14 lines**, and **−4 at each of the five guarded
   section heads**, both register-naming: at a head the original emits
   `lea/lea/shl / mov [temp] / jmp` and reloads the offset at the rewind entry
   where ours needs no `jmp`, and it duplicates `g_report_pages++` into the
   head arm where ours merges it into the shared block's tail.

**And one residual that costs no instructions but ~6 LCS points: the
three-slot rotation.** Our `n*0x4c` byte-offset CSE temp holds `0x10` with 221
references, so `page_start`, `indent` and `sect_start` all sit one slot high
(`0x14`, `0x18`, `0x4c` against the original's `0x10`, `0x14`, `0x44`).
`box`, `cur`, `ok`/`obj` and `passed` are on the original's own displacements.
The tenth pass measured the remap: renaming our slots to the original's is
worth ~6 points. **The original's frame is a genuine violation of the rule its
own compiler follows everywhere else** — its temp has 222 references and still
sits at `0x18`, below `page_start`'s 173 and `indent`'s 171 — and after five
passes of weight arithmetic, web decomposition, loop weighting, aggregate
padding, scalar absorption and declaration/initialisation reordering, no
mechanism has been found that reproduces it.

### What a future lane should try first, ranked

1. **The three-slot rotation.** Largest single residual and it gates residual 1
   above. Do NOT retry: reference-weight arithmetic on the temp (twelfth pass),
   web decomposition or loop weighting (thirteenth), declaration or
   initialisation order, `register`, renaming, aggregate padding, absorbing
   `sect_start`/`failmask`/`total`/`v` into the aggregate, splitting the temp,
   or the compare's operand order. Only a **non-weight** mechanism can move it.
   The one shape never tried is a *smaller* body: build a lab function that
   reproduces the violation (two low-weight always-live scalars below a
   high-weight CSE temp) and find what distinguishes it. `/tmp/sll16f/slots.py`
   is the census, `/tmp/sll16h/lab` the harness.
2. **The closing block's tail-merge**, +17 instructions in one place, with a
   named mechanism (two fall-through arms that differ only by a register).
   The cheapest remaining instruction win.
3. **Section 9's last two hint groups** are emitted `0x21c, 0x226, 0x21d,
   0x227` by the original and `0x21c, 0x21d, 0x226, 0x227` by us — the two
   arms' first and second lines interleave in the original. Worth ~20 index
   matches and nothing on LCS.
4. **The 38 `cmp ebp,edx` rewind tests** (the original has `cmp ebp,ecx` at all
   75), which follow from the order VC6 assigns registers to the four `box`
   loads in the hoisted block. Downstream of 1.

Do not re-open: the render/input tail (transcribed and verified instruction by
instruction from 0x0044d744 to the epilogue), the four call censuses (all
equal), the page-break block shape (0 of 124 store `cur.bottom`, as the
original), the `box` opacity (`box = cur`), section 8's line-end `cur.bottom`
form, the escaping aggregate, the `AppraisalRects` shape, and `bar`'s stack
home (killed by writing its top and bottom before its left and right).
