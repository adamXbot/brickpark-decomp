# Scope R — the level-database keyword tier, part 1: 47 of 48 exact

**Status: complete but for one WIP.** Branch `scope/R`, baseline `origin/main`
`0e62e67c` (2026-09-06). One new file, `LEGOLAND/levelkw.c`, 48 functions,
1,728 instructions: 47 `audit.py [OK]` (1,534 instructions), one
`WIP-FUNCTION` (`ParseKeywordSections`, 194 instructions, 124/196 index for
index). `/W3` clean; `relocs.py` 211 of 211 resolved positions agree, none
unresolved. No existing file was edited.

## Per function

| address | name | insns / bytes | audit | marker | note |
| --- | --- | ---: | --- | --- | --- |
| 0x004781b0 | `LookupNamedIndex` | 33 / 61 | OK | `// FUNCTION:` | first try |
| 0x004785d0 | `CurLevelSection` | 21 / 51 | OK | `// FUNCTION:` | first try; `strcpy` intrinsic |
| 0x00478610 | `CurLevelFlags` | 16 / 50 | OK | `// FUNCTION:` | first try; a `switch` on 1/2/3 |
| 0x00478650 | `IsPurgeLine` | 18 / 54 | OK | `// FUNCTION:` | first try |
| 0x00478690 | `HasArgs` (brief `sub_478690`) | 6 / 16 | OK | `// FUNCTION:` | `argc >= n` |
| 0x004786a0 | `LevelMaskMatches` (brief `CurLevelIndex`) | 7 / 18 | OK | `// FUNCTION:` | `(g_level_number & mask) != 0` |
| 0x004786c0 | `LineApplies` (brief `NextKeywordArg`) | 27 / 56 | OK | `// FUNCTION:` | the two above, in that order |
| 0x00478700 | `ParseRectArgs` | 38 / 99 | OK | `// FUNCTION:` | first try |
| 0x00478770 | `ParsePosArgs` | 19 / 46 | OK | `// FUNCTION:` | first try |
| 0x004787a0 | `Arg1` (brief `sub_4787a0`) | 3 / 8 | OK | `// FUNCTION:` | `args[1]` |
| 0x004787b0 | `LevelKw_none` | 14 / 26 | OK | `// FUNCTION:` | `strlen` intrinsic |
| 0x004787d0 | `EndScriptStep` | 8 / 29 | OK | `// FUNCTION:` | first try |
| 0x004787f0 | `BeginScriptStep` | 10 / 38 | OK | `// FUNCTION:` | first try |
| 0x00478820 | `LevelKw_Uninitialised` | 6 / 21 | OK | `// FUNCTION:` | dead in the binary |
| 0x00478840 | `LevelKw_END` | 8 / 36 | OK | `// FUNCTION:` | first try |
| 0x00478870 | `LevelKw_INIT` | 8 / 23 | OK | `// FUNCTION:` | first try |
| 0x00478890 | `LevelKw_AGES` | 49 / 149 | OK | `// FUNCTION:` | unsigned band compares |
| 0x00478930 | `LevelKw_OBJECTIVE` | 27 / 73 | OK | `// FUNCTION:` | first try |
| 0x00478980 | `LevelKw_ONEOFF` | 18 / 50 | OK | `// FUNCTION:` | first try |
| 0x004789c0 | `LevelKw_ONGOING` | 18 / 50 | OK | `// FUNCTION:` | first try |
| 0x00478a00 | `LevelKw_PERMANENT` | 27 / 63 | OK | `// FUNCTION:` | first try |
| 0x00478a40 | `LevelKw_REMINDER` | 27 / 63 | OK | `// FUNCTION:` | first try |
| 0x00478a80 | `LevelKw_REWARD` | 23 / 56 | OK | `// FUNCTION:` | first try |
| 0x00478ac0 | `LevelKw_MAP` | 37 / 89 | OK | `// FUNCTION:` | first try |
| 0x00478b70 | `LevelKw_LOAD` | 34 / 73 | OK | `// FUNCTION:` | first try |
| 0x00478bc0 | `LevelKw_BLUEPRINT` | 9 / 24 | OK | `// FUNCTION:` | a plain call of ENABLE (no tail jump) |
| 0x00478be0 | `LevelKw_ENABLE` | 54 / 127 | OK | `// FUNCTION:` | first try |
| 0x00478c60 | `LevelKw_CURRENCY` | 36 / 97 | OK | `// FUNCTION:` | first try |
| 0x00478cd0 | `LevelKw_HAPPINESS_ENV` | 34 / 91 | OK | `// FUNCTION:` | 68% → 100%: index loop (below) |
| 0x00478d30 | `LevelKw_LOOKAT` | 77 / 226 | OK | `// FUNCTION:` | 81% → 100% in three levers (below) |
| 0x00478e20 | `LevelKw_BREIFINGFILE_BRIEFINGFILE` | 42 / 100 | OK | `// FUNCTION:` | 83% → 100%: single-return if/else (below) |
| 0x00478e90 | `LevelKw_HINTSFILE` | 42 / 100 | OK | `// FUNCTION:` | same lever |
| 0x00478f00 | `LevelKw_WORKERS` | 57 / 146 | OK | `// FUNCTION:` | 80% → 100%: single-return if/else (below) |
| 0x00478fa0 | `LevelKw_GARDENER` | 66 / 177 | OK | `// FUNCTION:` | first try |
| 0x00479060 | `LevelKw_MECHANIC` | 66 / 177 | OK | `// FUNCTION:` | first try |
| 0x00479120 | `LevelKw_PROMPT` | 49 / 125 | OK | `// FUNCTION:` | 84% → 100%: single-return if/else (below) |
| 0x004791a0 | `LevelKw_INTRO` | 27 / 67 | OK | `// FUNCTION:` | first try |
| 0x004791f0 | `LevelKw_NEED` | 50 / 119 | OK | `// FUNCTION:` | first try |
| 0x00479270 | `LevelKw_NEEDAT` | 50 / 131 | OK | `// FUNCTION:` | first try |
| 0x00479300 | `LevelKw_NEEDIN` | 54 / 132 | OK | `// FUNCTION:` | first try |
| 0x00479390 | `LevelKw_CONNECT` | 30 / 77 | OK | `// FUNCTION:` | first try |
| 0x004793e0 | `LevelKw_LINK` | 39 / 99 | OK | `// FUNCTION:` | first try |
| 0x00479450 | `LevelKw_RANGE` | 56 / 127 | OK | `// FUNCTION:` | first try |
| 0x004794d0 | `LevelKw_CLEARAREA` | 46 / 114 | OK | `// FUNCTION:` | first try |
| 0x00478110 | `SplitWords` (brief `sub_478110`) | 67 / 145 | OK | `// FUNCTION:` | first try; the tokenizer |
| 0x00489e60 | `ReadLine` (brief `sub_489e60`) | 58 / 126 | OK | `// FUNCTION:` | 70% → 100%: loop shape (below) |
| 0x00499300 | `UpcaseString` (brief `sub_499300`) | 23 / 53 | OK | `// FUNCTION:` | first try |
| 0x00478280 | `ParseKeywordSections` | 194 / 561 (ours 556) | **WIP** | `// WIP-FUNCTION: … (63%, …)` | first divergence at index 60; residual below |

Renames from the brief, each from the body: `sub_478690` → `HasArgs`
(returns `argc >= n`); `CurLevelIndex` → `LevelMaskMatches` (it tests
`g_level_number & mask`, nothing is indexed); `NextKeywordArg` →
`LineApplies` (it fetches nothing: mask test then arg-count test, the gate
every handler runs first); `sub_4787a0` → `Arg1`; `sub_478110` →
`SplitWords`; `sub_489e60` → `ReadLine`; `sub_499300` → `UpcaseString`.
The two-keyword handler keeps the brief's compound name
`LevelKw_BREIFINGFILE_BRIEFINGFILE` (both table entries point at it; the
misspelling is the game's). Sibling briefs S and T cite these primitives by
address only, so nothing collides yet; the integrator should hand S/T the
names above.

Callees named here for the first time (declared `extern`, defined in other
scopes): `SetCurrency` 0x00457900, `LoadBriefingFile` 0x004687f0,
`LoadHintsFile` 0x00468810 (all three are the level-1 immediate arms of
CURRENCY / BREIFINGFILE / HINTSFILE; the queued arms call scope W's
`AddEvent_Currency`, `AddEvent_Breifingfile_Briefingfile`,
`AddEvent_Hintsfile`). Scope W's constructors are declared with the brief's
names (`AddEvent_Intro`, `_Currency`, `_Gardener_Mechanic`, `_Workers`,
`_Breifingfile_Briefingfile`, `_Hintsfile`, `_Lookat`, `_Need`, `_Needat`,
`_Needin`, `_Connect`, `_Link`, `_Range`, `_Cleararea`). `NewScriptEvent`
0x004689f0 keeps its tree name (PROMPT is its only 3-argument caller in this
file: `(text, text2 or 0, 1)`).

## Mechanics recovered

**The file grammar (`ParseKeywordSections`, `ReadLine`, `SplitWords`).**
Lines are read with `ReadLine(f, line, 0x400)`: one byte at a time through
`RES_ReadFile`; `\r` ends the line and swallows the next byte (the `\n`),
`\n` alone ends it, at most `max` bytes are kept, and NULL is returned only
when the file ends with nothing read on that line (so a final line without
a newline is delivered). `g_line_number` 0x00668fcc counts lines read from
the current file (reset to 0 at entry) and `progress_tick` runs once per
line. Everything from the first `#` is a comment. `SplitWords` cuts the
line in place at any of `" ,;:(){}"`, 0xA0, `\n`, `\t` (0x004bc098); a word
may be double-quoted (the quotes are removed, delimiters inside are kept,
an unterminated quote runs to the end of line) and a quote starting inside
an unquoted word begins a new word there. The first word is upper-cased
(`UpcaseString`, `toupper` per byte, returns the length) and looked up in
the caller's table by exact `strcmp`; its handler gets `(words, nwords - 1,
extra)`. A table whose first keyword is `"none"` names a fallback for
unknown words (the level table's `LevelKw_none`, which accepts blank
lines); a table whose last keyword is `"check"` names an epilogue called
as `handler(0, skipped, extra)` after the last line. A handler result of 0
counts the line as skipped; a negative result aborts the file and is
returned; otherwise the skipped count is returned.

**Sections and steps.** `[INIT]` is section 1, `[OBJECTIVE] <mask>`
section 2 (kind 1; it closes the step in progress and opens a new one with
the next serial from `g_step_serial` 0x00669098, and resets the goal list
`g_script_root`), `[REWARD]` section 4, `[END]` names the section
"Closed" and frees the step. `[ONEOFF]`/`[ONGOING]`/`[PERMANENT]`/
`[REMINDER]` set the kind (0/1/2/3 → `g_level_flags` 0/1/2/4) inside the
step, and the last two take an optional `PURGE` word that sets flag 8
(any other word clears it). `CurLevelSection` copies the section word into
`g_level_name` 0x00669058 and stores its number in `g_level_number`;
`CurLevelFlags` also records the kind in `g_level_db_flag_ac` 0x004bb5ac.
The section handlers gate on `g_level_number & mask` (mask 7 for
OBJECTIVE, 2 for the kinds) — i.e. they apply only inside sections 1–3 or
2 respectively — and every content handler on `LineApplies(mask, n)`: mask
5 (sections 1 and 4 — INIT and REWARD) for the immediate/queued handlers
(CURRENCY, HAPPINESS_ENV, LOOKAT, files, WORKERS, GARDENER, MECHANIC,
ENABLE), mask 1 for LOAD, mask 2 (OBJECTIVE only) for the goal handlers
(PROMPT, INTRO, NEED, NEEDAT, NEEDIN, CONNECT, LINK, RANGE, CLEARAREA).

**AGES.** `AGES` with no words turns the database on; `AGES lo` turns it on
when the profile age (`g_profile_age` 0x0080ffc0, CurProfile+0x20) is at
least `lo`; `AGES lo hi` when it is within [lo, hi]. All compares are
unsigned; `lo > hi` skips the line (returns 0). Every other handler in this
file starts with `if (g_level_db_active)` and returns 1 (ignore) when the
band excludes the player.

**Immediate versus queued.** On level 1 (`g_level_number == 1`) CURRENCY,
LOOKAT, BREIFINGFILE, HINTSFILE, WORKERS, GARDENER, MECHANIC and ENABLE act
at once (`SetCurrency`, the scroll globals, `LoadBriefingFile`,
`LoadHintsFile`, `g_map->gardeners/mechanics` at +0x38/+0x34 set to
`value != 0` when `value >= 0`, `GenerateGardener/Mechanic(&pos, 0)` n
times, `EnsureObjectClassLoaded` + `MarkElemAvailable(ElemID(name), 0,
1)`); on any other level they queue an `AddEvent_*` (ENABLE becomes
`LevelKw_GIVE`). HAPPINESS_ENV is level-1 only: five numbers into the rate
thresholds at 0x00832928..0x00832938 (`g_rate_t0[0..4]`). LOOKAT's cell is
in 1/256ths after `<< 8`; the level-1 arm projects it to the screen as
`x' = ((x - y) * tile_w) >> 9`, `y' = ((x + y) * tile_h) >> 9`
(`GetTileDimensions`) and centres the view: `ScrollX = (x' - view_w/2) <<
8`, `ScrollY = (y' - view_h/2) << 8` with the view size the u16 pair at
`g_map`+0/+2. GARDENER/MECHANIC take an optional count (default 1, and any
count below 1 is 1); NEED's count defaults to 1 when absent or 0; RANGE's
second bound defaults to 0 and the constructor takes `(flags, elem, hi,
lo)`; CLEARAREA's count defaults to 0; NEEDIN's rect starts at word 3;
LINK accepts `ALL` (case-insensitively) as "every class" (a null elem) and
skips the line on an unknown class name; the other class handlers silently
drop an unknown class. The goal constructors all receive `g_level_flags`
(a byte) as their first argument. PROMPT sets `g_script_root =
NewScriptEvent(text, text2 or 0, 1)` or clears it when the line has no
words; INTRO attaches its text to the step in progress only when there is
one.

## Globals and types named or confirmed

`g_level_db_active` 0x004bb5b0 and `g_level_db_flag_ac` 0x004bb5ac keep
movie3.c's names. New: `g_level_name` 0x00669058 (`char[]`),
`g_level_flags` 0x00669050 (the brief's `g_level_byte_669050`, a byte:
1 objective, 2 permanent, 4 reminder, 8 purge), `g_step_serial` 0x00669098
(the brief's `g_level_int_669098`), `g_profile_age` 0x0080ffc0 (the brief's
`g_cur_80ffc0`), `g_line_number` 0x00668fcc, `g_str_delims` 0x004bc098 and
the literals `g_str_purge`, `g_str_uninitialised`, `g_str_closed`,
`g_str_all`, `g_str_none`, `g_str_check`. 0x004d8bb0 is declared `kEmpty`
(the tree's name; the brief's `g_alloc_tag` is the same empty string, the
default file name for BREIFINGFILE/HINTSFILE). 0x00832928 is declared as
the aggregate `int g_rate_t0[5]` because HAPPINESS_ENV writes it in a
loop; workers.c's scalars `g_rate_t0`/`t1`/`t2`/`t3` and simcore2.c's
`g_mood_low`/`g_mood_high` are its elements 0/2/3/4 and 1/3. `MapHdr` here
is {u16 view_w, u16 view_h, …, int mechanics +0x34, int gardeners +0x38}.
`KeywordEntry` is `{const char* keyword; KeywordFn handler;}` (8 bytes) and
`KeywordFn` is `int (*)(char** args, int argc, int extra)`.

## Original bugs reproduced

- `CurLevelSection` copies the section word into `g_level_name` with an
  unbounded `strcpy`; `SplitWords` writes its word pointers with no bound
  on the caller's 20-slot vector (a line with more than 20 words overruns
  `words[]` into `line[]`).
- `LevelKw_LINK` returns 0 (line skipped) for an unknown class while
  `CONNECT`/`NEED`/`NEEDAT`/`NEEDIN`/`RANGE` return 1 and queue nothing.
- `ParseKeywordSections`: a handler result kept from a previous line is
  what `rc < 0` tests when a line has no handler (no keyword match and no
  fallback), and the "check" epilogue runs on the same `rc` variable.
- `ReadLine` returns the buffer for an empty line at end of file only when
  the file ends in a newline; `c` is tested after a failed read (stale
  byte) — harmless because it is compared, not stored.

## Extern-type divergences (caller-side levers)

- `NewScriptEvent(void*, void*, void*)` — the tree's prototype; PROMPT passes
  `(char*, char* or 0, (void*)1)`.
- `GenerateGardener`/`GenerateMechanic` return `void*` here (the result is
  discarded; workers.c says `void`).
- `ParseKeywordSections(void* f, KeywordEntry* table, int count, int
  extra)` here; movie3.c declares the table `const void*`.
- The `AddEvent_*` goal constructors take `unsigned char flags` first
  (`mov dl/al, byte ptr [0x669050]; push edx/eax`).
- `strspn`/`strcspn` return `unsigned int`; `strchr` is a real call (not
  the intrinsic) — `#pragma intrinsic` is set only for `strlen`, `strcpy`
  and `strcmp` (the parser's three inlined compares).

## Levers, with evidence

- **A five-element store loop is an index loop, not a pointer loop.**
  `for (i = 0; i < 5; i++) g_rate_t0[i] = atoi(args[i + 1])` gives the
  original's strength-reduced pointer with a *signed* `jl` against the end
  address; `for (p = g_rate_t0; p < g_rate_t0 + 5; p++)` compiles to `jb`
  and a `lea/sub` base (23/34 → 34/34).
- **Single-return `if/else` beats two returns whenever the two arms only
  differ in the callee** — it hoists the shared push above the compare and
  sinks the saved-register push. `PROMPT` (`argc > 1 ? two-arg : one-arg`
  call, then one `return 1`): 41/49 → 49/49 with argc in esi as the
  original. `WORKERS`: `if (level == 1) {…} else AddEvent_Workers(a, b);
  return 1;` sinks `push esi` past the `LineApplies` early returns (the
  original's `if (!active) return 1` keeps its own `mov eax,1; pop edi;
  ret`); with `return 1` inside the arm the compiler pushed esi in the
  prologue and merged the early return into the common exit (44/55).
  `BREIFINGFILE`/`HINTSFILE` also need the early return
  `if (!g_level_db_active) return 1;` with `name = kEmpty` as the
  declaration's initializer (35/42 → 42/42; the `if (active) { … }` wrapper,
  a ternary, or `if/else` assignment of `name` all land elsewhere).
- **`LOOKAT`'s level-1 arm copies `pos.x/pos.y` into locals AFTER the
  shifts are stored, and declares `y` before `x`.** `pos.x <<= 8; pos.y <<=
  8; if (level == 1) { int w, h; int y = pos.y; int x = pos.x;
  GetTileDimensions(&w, &h); pos.x = ((x - y) * w) >> 9; … }` is exact:
  the shifted values stay in edx/ecx for the store and are copied to
  edi/esi only inside the arm (`push edi` sinks there), and `w`/`h` land in
  the dead `args`/`argc` parameter slots as in the original. Shifting into
  temporaries first (x/y in esi/edi from the top) was 62/77; declaring `x`
  first swapped edi/esi (72/77).
- **`ReadLine` is `while (1)` with separate `if (c == '\r') break; if (c ==
  '\n') break;` and `if (++n >= max) break;`.** That form is not rotated
  (one `RES_ReadFile` call, `jge exit; jmp top`) and threads the two
  breaks into the post-loop `if (c == '\r')` (46/66 → 58/58). `for (;;)`
  with `c == '\r' || c == '\n'` and `n++; if (n >= max)` is rotated into
  two calls; `do … while (n < max)`, a goto loop, or an `ok`/`ch` copy pair
  are worse.
- **`SplitWords`'s empty-word case is `continue`, not `break`** (it jumps
  to the bottom `cmp byte [esi],0`), and the loop is `while (*s)` with the
  initial test peeled by the compiler.
- **Direct byte reads of `g_level_flags` at the call** (`AddEvent_Need(
  g_level_flags, elem, n)`) give the `mov dl, byte ptr [0x669050]` right
  before the pushes; LINK's `elem = 0` arm reuses `_stricmp`'s zero result
  register (`push eax` after `test eax,eax; jne`) — write it as a plain
  `elem = 0` and VC6 does the reuse itself.
- **`GARDENER`/`MECHANIC`'s count loop is `while (i != 0) { …; i--; }`
  on a copy of n** (`mov esi,eax; …; dec esi; jne`), and the default is
  `if (argc < 3 || (n = atoi(args[3])) < 1) n = 1;` — one shared `mov
  eax,1` for both fall-throughs.

## The WIP: `ParseKeywordSections` 0x00478280 (124/196, first divergence at 60)

The committed body is the natural reading and is behaviourally complete
(every path was traced in the disassembly; see the grammar above). The
residual is one VC6 decision the source spelling has not reproduced: the
original keeps the per-line *found* flag as a byte in `bl` (`xor bl,bl`
before the lookup loop, `mov bl,1` in the match block before the handler
call, `test bl,bl; jne` after the loop to skip the "none" fallback) and
lays the match block out inline after the loop bottom (`jl loop; jmp post;
match:…; post: test bl,bl`). With that flag live in ebx the inner loop has
no spare register, so `words[0]` and `count` are reloaded from the frame
each iteration, `rc`, `nwords`, `skipped` and `fallback` stay in memory,
`i` is edi and the table cursor is ebp (table's own register, reloaded from
its home slot after the fallback block). Our compile constant-folds the
flag (the match edge is threaded straight past the `!handled` test, the
loop-exit edge too), exiles the match block to the end of the function,
and then has registers to spare: `nwords` in esi, `rc` in ebp, `words[0]`
cached in edx, the byte temp of the inlined `strcmp` in `bl` instead of
`dl`. Everything before the inner loop (the prologue's zero web in ebx, the
three zeroed locals, the "none" compare, the line loop head, the `#` strip,
`SplitWords`) and everything after the fallback (the `rc < 0` checks, the
second `ReadLine`, the "check" compare and call, both epilogues) matches
once the loop shape does.

Spellings tried, all with `char`/`int`/`short`/`unsigned char` flags at
block or function scope, reset before the loop / before `UpcaseString` /
after `SplitWords` / at the top of the line body, and with the fallback
test as `!h && f`, `f && !h`, nested ifs, `switch (h)`, `do { } while (0)`
and `while (!h)`: every form whose match path leaves the loop directly
(`break`, `goto`, `return` from a `static __inline` helper with `rc`/
`skipped` by pointer, `i = count; break`) folds the flag identically
(124/196). Forms that keep the match path inside the loop keep the flag:
`for (i = 0; i < count && !handled; i++)` without `break` (156/195; the
`!handled` test moves to the loop head, `i` goes to memory) and `for (i =
0; i < count; i++, e++) … i = count;` with an explicit cursor `e = table`
(165/197, the closest: only the match path's `inc edi; add ebp,8; cmp; jl`
through the loop bottom and `nwords` in esi differ). `handled = strcmp(…)
== 0; if (handled)` keeps the flag and the original's block layout but
materialises it with `sete bl` (153/196). The C++ front end (`bool`) folds
too. `/O1`, `/Os`, `/Ob0`, `/Gf` change nothing useful. Next thing to try:
a form where the flag has a non-constant reaching definition on the match
edge without generating code for it — I found none; or check whether the
original TU had the fallback test and the handler call in a different
lexical relationship (e.g. the handler call after the loop guarded by the
flag with the `rc == 0` test folded in), since VC6 threads only one hop.

## Verification

```sh
$PY tools/audit.py LEGOLAND/levelkw.c          # 47 x [OK], 1 x [WIP], PASS
$PY tools/relocs.py LEGOLAND/levelkw.c         # 211 relocations, 0 MISMATCH, 0 unresolved
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/sr_w3.obj LEGOLAND/levelkw.c
```
