# scope LL16 — `RunAppraisalScreen` 0x004453a0

Branch `scope/LL16` from `0767e1a1`. New file `LEGOLAND/appraisalscreen.c`,
one function.

## Status

| | |
| --- | --- |
| address | `0x004453a0` |
| original | 8,085 instructions, 34,662 bytes, frame `0x23d4` |
| ours | 6,821 instructions, 29,309 bytes, frame **`0x23d4` (exact)** |
| first diverging index | 6 |
| mismatch | 8,005 of 8,085 |
| `FULL MATCH` | 2,914/6,821 = 42.7% |
| audit | `[WIP]`, file ends `PASS` |
| relocs | zero `MISMATCH` (a WIP body is skipped) |
| `/W3` | clean |

Not exact. **The whole build phase is transcribed** — all nine sections, the
advice chain, the closing sprintf line and the hint section — and so are the
render and input loops, whose length now agrees with the original's to within
a handful of instructions. What is left is one thing and its consequences:
the original keeps `box` in memory and reloads it at every page-break site
while ours constant-folds it. See the fifth pass at the end; the instruction
shortfall, the missing `edi = y`, and the `indent` / `page_start` register
flip are all downstream of it, so they are not separate work items.

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
