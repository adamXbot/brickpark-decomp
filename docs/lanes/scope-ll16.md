# scope LL16 — `RunAppraisalScreen` 0x004453a0

Branch `scope/LL16` from `0767e1a1`. New file `LEGOLAND/appraisalscreen.c`,
one function.

## Status

| | |
| --- | --- |
| address | `0x004453a0` |
| original | 8,085 instructions, 34,662 bytes, frame `0x23d4` |
| ours | see the `// WIP-FUNCTION:` marker on the body |
| audit | `[WIP]`, file ends `PASS` |
| relocs | zero `MISMATCH` (a WIP body is skipped) |
| `/W3` | clean |

Not exact. The body written so far covers the prologue, the report's title
line and the whole of the "what the park holds" section; the rest is
recovered structurally (below) but not yet transcribed.

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
