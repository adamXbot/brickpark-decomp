# Scope S — the level-database keyword tier, part 2: 37 of 37 exact

**Status: complete.** At merge (2026-09-06) the integrator renamed the file's
three shared primitives to scope T's names — `NextKeywordArg` →
`KwLineApplies`, `sub_478690` → `KwHasArgs`, `_stricmp` → `NameCompare` —
extern renames only, every body still `[OK]`; the text below keeps the
names as written. Branch `scope/S`, baseline `origin/main` `0e62e67c`
(2026-09-06). One new file, `LEGOLAND/levelkw2.c`, 37 functions, 1,561
instructions / 3,859 bytes, every one `audit.py [OK]`; `/W3` clean;
`relocs.py` 206 of 206 resolved positions agree, 0 `MISMATCH`, 5
`UNRESOLVED` (the three string literals `""`, `";"`, `"NOPOPUP"`, expected).
No existing file was edited. Objects under `/tmp/ss_*`.

## Per function

34 of the 37 were exact on the first compile; the three `SELECT*` handlers
needed one layout lever each (below). Sizes are the audit's ours = orig.

| address | name | insns / bytes | audit | marker | note |
| --- | --- | ---: | --- | --- | --- |
| 0x00479550 | `LevelKw_REMOVE` | 48 / 111 | OK | `// FUNCTION:` | first try; the word count is checked twice (NextKeywordArg, then `sub_478690` again) |
| 0x004795c0 | `LevelKw_REMOVERANGE` | 56 / 128 | OK | `// FUNCTION:` | first try |
| 0x00479640 | `LevelKw_COMPOSITE` | 59 / 136 | OK | `// FUNCTION:` | first try |
| 0x004796d0 | `LevelKw_LOOPCOMPOSITE` | 43 / 106 | OK | `// FUNCTION:` | first try |
| 0x00479740 | `LevelKw_TECHLEVEL` | 40 / 97 | OK | `// FUNCTION:` | first try |
| 0x004797b0 | `LevelKw_RESEARCH` | 32 / 77 | OK | `// FUNCTION:` | first try; a stub — parses and drops its words |
| 0x00479800 | `LevelKw_PARKVISITORS` | 27 / 70 | OK | `// FUNCTION:` | first try; the 27-instruction template |
| 0x00479850 | `LevelKw_RIDEVISITORS` | 40 / 97 | OK | `// FUNCTION:` | first try; TECHLEVEL's body |
| 0x004798c0 | `LevelKw_RIDERS` | 40 / 97 | OK | `// FUNCTION:` | first try; TECHLEVEL's body |
| 0x00479930 | `LevelKw_SCENERYCOVERAGE` | 27 / 70 | OK | `// FUNCTION:` | first try; PARKVISITORS' body |
| 0x00479980 | `LevelKw_PATHSCENERY` | 27 / 70 | OK | `// FUNCTION:` | first try; PARKVISITORS' body |
| 0x004799d0 | `LevelKw_RIDECOVERAGE` | 27 / 70 | OK | `// FUNCTION:` | first try; PARKVISITORS' body |
| 0x00479a20 | `LevelKw_SHOPCOVERAGE` | 27 / 70 | OK | `// FUNCTION:` | first try; PARKVISITORS' body |
| 0x00479a70 | `LevelKw_FOODCOVERAGE` | 27 / 70 | OK | `// FUNCTION:` | first try; PARKVISITORS' body |
| 0x00479ac0 | `LevelKw_TOTCOVERAGE` | 27 / 70 | OK | `// FUNCTION:` | first try; PARKVISITORS' body |
| 0x00479b10 | `LevelKw_APPRAISAL` | 115 / 300 | OK | `// FUNCTION:` | first try; `char buf[0x200] = ""` + intrinsic strcpy/strcat |
| 0x00479c40 | `LevelKw_STUDAREA` | 37 / 97 | OK | `// FUNCTION:` | first try |
| 0x00479cb0 | `LevelKw_SAVE` | 27 / 70 | OK | `// FUNCTION:` | first try; PARKVISITORS' body |
| 0x00479d00 | `LevelKw_HAPPINESS` | 37 / 90 | OK | `// FUNCTION:` | first try |
| 0x00479d60 | `LevelKw_NEEDGARDENERS` | 27 / 70 | OK | `// FUNCTION:` | first try; PARKVISITORS' body |
| 0x00479db0 | `LevelKw_NEEDMECHANICS` | 27 / 70 | OK | `// FUNCTION:` | first try; PARKVISITORS' body |
| 0x00479e00 | `LevelKw_HUNGER` | 51 / 118 | OK | `// FUNCTION:` | first try |
| 0x00479e80 | `LevelKw_FIXRIDES` | 37 / 90 | OK | `// FUNCTION:` | first try; HAPPINESS' body |
| 0x00479ee0 | `LevelKw_POWERRIDES` | 27 / 70 | OK | `// FUNCTION:` | first try; PARKVISITORS' body |
| 0x00479f30 | `LevelKw_ZONING` | 44 / 104 | OK | `// FUNCTION:` | first try |
| 0x00479fa0 | `LevelKw_CHECKFLAG` | 47 / 113 | OK | `// FUNCTION:` | first try |
| 0x0047a020 | `LevelKw_THEMEICON` | 55 / 134 | OK | `// FUNCTION:` | first try |
| 0x0047a0b0 | `LevelKw_ADDFLAG` | 55 / 134 | OK | `// FUNCTION:` | first try; THEMEICON's body |
| 0x0047a140 | `LevelKw_BRIDGES` | 58 / 139 | OK | `// FUNCTION:` | first try |
| 0x0047a1d0 | `LevelKw_ENDSCREENS` | 108 / 284 | OK | `// FUNCTION:` | first try; APPRAISAL's buffer idiom |
| 0x0047a2f0 | `LevelKw_SELECTTHEME` | 44 / 106 | OK | `// FUNCTION:` | 71% → 100%: the ternary-with-shared-test lever (below) |
| 0x0047a360 | `LevelKw_SELECTTAB` | 44 / 106 | OK | `// FUNCTION:` | same lever as SELECTTHEME |
| 0x0047a3d0 | `LevelKw_SELECTMODE` | 44 / 108 | OK | `// FUNCTION:` | 72% → 98% → 100%: the ternary, then the volatile parameter-slot re-read (below) |
| 0x0047a440 | `LevelKw_FOREVER` | 20 / 57 | OK | `// FUNCTION:` | first try |
| 0x0047a480 | `LevelKw_GIVE` | 54 / 120 | OK | `// FUNCTION:` | first try; also called by ENABLE 0x00478be0 (scope R) |
| 0x0047a500 | `LevelKw_TAKE` | 28 / 70 | OK | `// FUNCTION:` | first try |
| 0x0047a550 | `LevelKw_ADDBRICKS` | 28 / 70 | OK | `// FUNCTION:` | first try |

Names: the brief's `LevelKw_<KEYWORD>` kept for all 37 (the keyword table at
0x004bb6f8 confirms every address → keyword pairing; the brief's table
offsets are the *handler* slot of each 8-byte entry). Callees use the sibling
briefs' provisional names — scope R's `NextKeywordArg`, `ParseRectArgs`,
`LookupNamedIndex` and the placeholder `sub_478690`; scope W's `AddEvent_*`;
scope X's `SetThemeIcon`, `AddLevelFlag`, `SetBridges` — so the integrator
has one rename per address to reconcile at merge, not two.

## Mechanics recovered

**The handler contract.** `ParseKeywordSections` (0x00478280) calls every
table entry as `int h(char** argv, int argc, int arg)`: `argv` is the
parser's word array (`argv[0]` the keyword, `argv[1..argc]` its words,
`argc` = words − 1), `arg` the value `LoadLevelDatabase` (movie.c) passed
to `ParseKeywordFile` — 0. Return 1 = handled, 0 = skipped or bad word.
Every handler here opens with `if (!g_level_db_active) return 1;`
(0x004bb5b0) and then `NextKeywordArg(argv, argc, sections, need)`.

**`NextKeywordArg` (0x004786c0) is a predicate, not a cursor**: from its
body and its two callees, `(g_level_number & sections) != 0 && argc >=
need`. `0x004786a0` is `(g_level_number & mask) != 0` (its first two
arguments are ignored); `0x00478690` is `argc >= need` (first argument
ignored). Scope R owns the names; the semantics belong in the rename.

**`g_level_number` (0x00669054, movie3.c's name) is the current section's
bit, not a level number**: the goal handlers pass `sections = 2`, the reward
handlers (GIVE, TAKE, ADDBRICKS) `4`, APPRAISAL `1`, and the four
level-setup keywords `5` (= 1|4) and then act immediately when it `== 1`
([INIT]) or create a step event otherwise. Scope R's `CurLevelSection`
0x004785d0 stores it; the integrator may want a truer name.

**`g_level_byte_669050`** is the section's event-flags byte; every *goal*
constructor (kinds 40–69, `AddEvent_Remove` … `AddEvent_Forever`) takes it
as its first, byte-wide parameter. The step constructors THEMEICON / ADDFLAG
/ BRIDGES / GIVE / TAKE / ADDBRICKS take no flags.

**Argument shapes** (the spec for the event constructors and an eventual
runtime):

- `REMOVE obj n` → `AddEvent_Remove(flags, ElemID(obj), n)` (skipped when
  the object is unknown). `REMOVERANGE obj lo [hi]` → `(flags, e, hi, lo)`,
  hi default −1. `COMPOSITE obj n [extra]` → `(flags, e, n ? n : 1, extra)`;
  `LOOPCOMPOSITE obj n` → `(flags, e, n ? n : 1)`. `TECHLEVEL`,
  `RIDEVISITORS`, `RIDERS obj n` → `(flags, e, n)`.
- `PARKVISITORS`, `SCENERYCOVERAGE`, `PATHSCENERY`, `RIDECOVERAGE`,
  `SHOPCOVERAGE`, `FOODCOVERAGE`, `TOTCOVERAGE`, `SAVE`, `NEEDGARDENERS`,
  `NEEDMECHANICS`, `POWERRIDES n` → `(flags, atoi(n))`.
- `HAPPINESS a b`, `FIXRIDES a b` → `(flags, a, b)`. `HUNGER a b [+]` →
  `(flags, a, b, argv[3][0] == '+')`. `CHECKFLAG f [on]` → `(flags, f, on)`
  with on default 1.
- `STUDAREA x0 y0 x1 y1 n` → `ParseRectArgs(rect, argv, 1)` (atoi of four
  words, corners ordered) then `(flags, rect, n)`.
- `ZONING name n` → `LookupNamedIndex(name, g_zoning_names, 4)`; unknown
  name fails the line (return 0). `SELECTTHEME [name]`,
  `SELECTTAB [name]` → index or 0 with no word; `SELECTMODE [name]` → index
  or argc (= 0, QUERY) with no word; unknown names fail the line.
- `THEMEICON i [on]`, `ADDFLAG f [on]`, `BRIDGES n [on]` (on default 1;
  BRIDGES decrements a positive n) → in [INIT] `SetThemeIcon` /
  `AddLevelFlag` / `SetBridges`, else `AddEvent_Themeicon` / `_Addflag` /
  `_Bridges`. `ENDSCREENS which [text [text2]]` → in [INIT] only,
  `SetLevelEndSequence(which, "text;text2")`. `APPRAISAL [state [text
  [text2]]]` → `SetLevelGoalState(state, "text;text2")` always.
- `GIVE obj [NOPOPUP]` → `AddEvent_Give(e, popup)`, `TAKE obj` →
  `AddEvent_Take(e)`, `ADDBRICKS n` → `AddEvent_Addbricks(n)` for n > 0.
- `RESEARCH obj [n]` parses and does nothing. `FOREVER` →
  `AddEvent_Forever(flags)`.

**Enumerated-word tables, first named here:** `g_zoning_names` 0x004bb5b4 =
{LEGOLAND, ADVENTURER, CASTLE, WESTERN}; `g_theme_names` 0x004bb5c4 =
{LEGOLAND, WESTERN, CASTLE, ADVENTURER, NONE}; `g_tab_names` 0x004bb5d8 =
{Build, Research}; `g_mode_names` 0x004bb5e0 = {QUERY, BUILD, ERASE, PATH,
MAP}. The next four pointers (0x004bb5f4: Terraces, RideWear, PlantWear,
AutoStud) belong to a scope T keyword.

**`g_alloc_tag` is the pooled `""`.** 0x004d8bb0 (coaster7.c and
schoolcar5.c's `extern char g_alloc_tag[]`, passed to `AllocZeroed` as a
tag) lies past .data's raw size and `relocs.py` resolves our `""` literal
(`??_C@_00A@?$AA@`) to it: `char buf[0x200] = "";` compiles to `mov al,
[0x4d8bb0] / mov [buf], al` plus a 0x1ff-byte zero fill. Those two callers'
source is almost certainly `AllocZeroed(n, 0, "", 0)`.

## Original behaviour reproduced (not bugs, but worth knowing)

- `REMOVE` checks `argc >= 2` twice (inside `NextKeywordArg` and again via
  `sub_478690`); none of its siblings do.
- `SELECTMODE`'s `if (argc)` can only take the true arm (need = 1), so its
  "no word" default is dead; the code is kept as written.
- `RESEARCH` calls `ElemID` and `atoi` for nothing.
- `ENDSCREENS` builds its text in every section but stores it only in
  [INIT].

## Extern-type divergences (caller-side levers)

- `SetLevelGoalState(int, const char*)` here; movie3.c declares it
  `(int, int)` and passes `(0, 0)`. Same address 0x0044dc70.
- Goal constructors take `unsigned char flags` first: the callers load the
  byte and push the whole register (`mov al,[0x669050] / push eax`); an
  `int` parameter would have zero-extended.
- `LookupNamedIndex(const char*, const char* const*, int)` — the tables are
  `const char* const[]` externs with their addresses; scope R may declare
  the definition differently.
- `atoi` 0x004a04b9 (CRT) is declared here for the first time in the tree
  (`extern int atoi(const char*)`); savegame.c reaches it via `<stdlib.h>`.
- `strcpy`/`strcat` are the intrinsics (`#pragma intrinsic`), as audio4.c.

## Levers, with evidence

- **A shared `-1` test over a ternary is how VC6 spells "else jump straight
  into the call" (SELECTTHEME, SELECTTAB).** The original is `test esi,esi
  / je ELSE … cmp eax,-1 / je FAIL / <call> … ELSE: xor eax,eax / jmp <call>
  / FAIL: pop edi / xor eax,eax / pop esi / ret`. Written as
  `if (argc) { i = Lookup(); if (i == -1) return 0; } else i = 0; call;
  return 1;` VC6 keeps two return-0 sites (a bare `ret` after
  NextKeywordArg and an inline `xor` epilogue) and merges the early
  `return 1` into the tail (30/42, 101 B). The exact source is
  `if (NextKeywordArg(...)) { i = argc ? Lookup(...) : 0; if (i != -1) {
  call; return 1; } } return 0;` — one textual `return 0` collects both
  failures into the last block, and VC6 jump-threads the constant arm past
  the `!= -1` test into the call, which IS the exiled `xor eax,eax / jmp`.
  Measured on the way: `goto fail` with the else arm jumping to an `add:`
  label 25/39; the nested form with a second `return 0` inside the arm
  37/43; the nested form with two separate call sites (`AddEvent(flags, 0)`
  in the else arm) 42/51 — VC6 does not merge calls whose argument differs
  in constant-ness.
- **A parameter re-read from its stack home while its register copy is
  live needs the volatile slot read (SELECTMODE).** The else arm is `mov
  eax, dword ptr [esp+0x10]` — the `argc` slot, not the third parameter's
  (`[esp+0x14]`, which the `: arg` spelling produced, 43/44). `: argc`,
  `: (int)(unsigned)argc`, a pre-call `int n = argc` copy, or routing the
  early uses through the copy all let VC6 thread the known zero into
  `xor eax,eax / jmp` (42/44 each). `: *(volatile int*)&argc` reproduces the
  reload exactly (LEVERS' "read the stack PARAMETER itself" lever, first
  used on an `int`): the early reads still CSE into esi. Semantically the
  arm is `mode = argc` (0).
- **`if (!NextKeywordArg(...)) return 0;` emits a bare `ret`** (eax is the
  call's zero) — in every handler whose other `return 0` sites are absent;
  add a second `return 0` and both become `xor` blocks (see above).
- **Push sinking decides whether the early `return 1` shares the tail:**
  REMOVERANGE/COMPOSITE (ebp/edi pushed after the NextKeywordArg test) and
  the `SELECT*`/GIVE shape (a tail whose `mov eax,1` sits in the call's
  block) keep an inline `pop/mov eax,1/pop/ret`; REMOVE, RESEARCH,
  PARKVISITORS' family, FOREVER (tail is a join or has no pushes) merge it.
  No source change was needed for any of these — the plain `if
  (!g_level_db_active) return 1;` produced both layouts.
- **`int popup = 1;` at declaration (GIVE)** puts `mov ebp,1` above the
  guard and lets the early `return 1` come out as `mov eax,ebp`.
- **Locals for call results keep `add esp` merged**: `n = atoi(argv[1]);
  AddEvent_X(flags, n);` → one `add esp,0xc`; the 27-instruction family,
  HAPPINESS (0x14) and STUDAREA (0x1c) all rely on it.
- **`char buf[0x200] = "";`** is the exact spelling of the 1-byte copy +
  `rep stosd / stosw / stosb` fill; `strcat(buf, ";")` then `strcat(buf,
  argv[3])` are the two `repne scasb … rep movsd/movsb` sequences.
- **Inert / not needed**: no volatile, no struct wrapper, no register
  spelling anywhere except SELECTMODE's slot read.

## Verification

```sh
$PY tools/audit.py LEGOLAND/levelkw2.c        # 37 x [OK], PASS
$PY tools/relocs.py LEGOLAND/levelkw2.c       # 0 MISMATCH, 5 UNRESOLVED (literals)
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/ss_w3.obj LEGOLAND/levelkw2.c
```
