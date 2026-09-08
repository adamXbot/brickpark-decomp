# scope LL16 — `RunAppraisalScreen` 0x004453a0

Branch `scope/LL16` from `0767e1a1`. New file `LEGOLAND/appraisalscreen.c`,
one function.

## Status

| | |
| --- | --- |
| address | `0x004453a0` |
| original | 8,085 instructions, 34,662 bytes, frame `0x23d4` |
| ours | 7,824 instructions, 34,224 bytes, frame **`0x23d4` (exact)** |
| first diverging index | 8 |
| mismatch | 7,923 of 8,085 |
| index-for-index `MATCH` | 162 |
| `FULL MATCH` (difflib) | 40.7% — see the eighth pass, do not read this alone |
| true LCS vs the whole original | 4,441/8,085 = 54.9% |
| LCS with the slot permutation removed | 4,968/8,085 = **61.4%** (see the tenth pass) |
| audit | `[WIP]`, file ends `PASS` |
| relocs | zero `MISMATCH` (a WIP body is skipped) |
| `/W3` | clean |

Not exact. **The whole build phase is transcribed** — all nine sections, the
advice chain, the closing sprintf line and the hint section — and so are the
render and input loops. As of the ninth pass the page reset is the original's
shape (hoisted above the rewind test AND repeated in the fall-through arm),
`page_start` is in `ebp` and `indent` is memory-homed exactly as the original
does it, and the body is within 264 instructions and 438 bytes of the target.
**The one remaining structural residual is the stack SLOT PERMUTATION**: our
33 scalar dwords hold the same values as the original's 33 in a different
order, so almost every `[esp+N]` displacement is wrong. **Read the tenth pass
first** — it gives VC6's frame-ordering rule (descending reference weight,
per dword for aggregates), which is the lever every earlier pass was looking
for; the earlier passes' "what is left" lists are superseded.

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
