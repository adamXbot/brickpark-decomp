# scope T — the level-database keyword tier, part 3, and the process start-up (2026-09-06)

Branch `scope/T`, notes for folding into `docs/DECOMP.md` at integration. Two
new files, nothing else touched. **29 of 29 exact (≈1,430 instructions)**;
`audit.py` PASS on both files, `/W3` clean, `relocs.py` zero MISMATCH
(unresolved positions are string literals and the CRT/IAT slots without
address comments).

## Per function

| address | name | insns | audit | note |
| --- | --- | ---: | --- | --- |
| 0x0047a5a0 | `LevelKw_PLACE` | 71 | OK | early `return 1` for the inactive path (see levers) |
| 0x0047a650 | `LevelKw_CLEAR` | 31 | OK | first compile |
| 0x0047a6a0 | `LevelKw_UNGLUE` | 31 | OK | first compile |
| 0x0047a6f0 | `LevelKw_GLUE` | 63 | OK | the cell flag field is an `unsigned short` (see levers) |
| 0x0047a7b0 | `LevelKw_EXTENDPARK` | 31 | OK | first compile |
| 0x0047a800 | `LevelKw_FMV` | 32 | OK | first compile |
| 0x0047a860 | `LevelKw_INTERVAL` | 23 | OK | first compile |
| 0x0047a8a0 | `LevelKw_MESSAGE` | 23 | OK | first compile |
| 0x0047a8e0 | `LevelKw_FEATURE` | 51 | OK | first compile |
| 0x0047a960 | `LevelKw_REPORT` | 129 | OK | 119 -> 126 -> 129 (arm order, two lookup calls) |
| 0x0047aa90 | `LevelKw_HAP_FACTOR` | 42 | OK | first compile |
| 0x0047ab00 | `LevelKw_CAPACITYSCALE` | 51 | OK | first compile |
| 0x0047ab80 | `LevelKw_CAPACITYCAP` | 51 | OK | first compile |
| 0x0047ac00 | `LevelKw_DEGRADE` | 54 | OK | early `return 1` for the inactive path |
| 0x0047ac80 | `LevelKw_MAXBLOKES` | 30 | OK | store order (see levers) |
| 0x0047ace0 | `LevelKw_MAXCAPACITY_MAXVISITORS` | 34 | OK | first compile; two table entries, one handler |
| 0x0047ad40 | `LevelKw_MINCAPACITY_MINVISITORS` | 34 | OK | first compile; two table entries, one handler |
| 0x0047ada0 | `LevelKw_ENTRANCEFEE` | 33 | OK | first compile |
| 0x0047ae00 | `LevelKw_FLASHBUTTON` | 64 | OK | the word loop as an index loop (see levers) |
| 0x0047aea0 | `LevelKw_FLASHBUTTOFF` | 67 | OK | same |
| 0x0047af50 | `LevelKw_PURGE` | 16 | OK | first compile |
| 0x0047af80 | `LevelKw_ENDLEVEL` | 16 | OK | first compile |
| 0x0047f820 | `TimeSinceSave` | 3 | OK | screens2.c's name |
| 0x0047f830 | `OpenDebugLog` | 2 | OK | `return 1` stub taking the log name |
| 0x0047f840 | `CloseDebugLog` | 2 | OK | `return 1` stub |
| 0x0047f860 | `DebugNoop` | 1 | OK | dead |
| 0x0047f880 | `InitSession` | 262 | OK | one pass after the counter type and the close loop |
| 0x0047fc40 | `FindCommandSwitch` | 95 | OK | first compile (brief: `ParseCommandSwitch`) |
| 0x0047fd10 | `GameMain` | 84 | OK | the two trailing stores reversed |

Names settled for the parse primitives (scope R's file will define them;
the R and S briefs used placeholders): 0x004786c0 is **`KwLineApplies(argv,
argc, mask, nargs)`** = `(g_level_number & mask) && argc >= nargs` — not a
"next argument" reader; 0x004786a0 is **`KwSectionMatches(argv, argc,
mask)`** = `(g_level_number & mask) != 0`; 0x00478690 is `argc >= nargs`
(call it `KwHasArgs`). `ParseRectArgs` 0x00478700 reads four ints from
`argv[first..]` and normalises the rect; `ParsePosArgs` 0x00478770 reads
two; `LookupNamedIndex(name, table, n)` 0x004781b0 returns the index of a
case-insensitive match or -1. The CRT at 0x004a04b9 is `atoi`, 0x004a0580
`strstr`, 0x004a0600 `_strupr`, 0x004aab90 `_stricmp` (the tree's
`NameCompare`, kept). First named here: `g_ncmdshow` 0x0066920c,
`g_volume_names` 0x004bcba4 (Legoland.res, Graphics2.res, Graphics1.res),
`g_res_volumes` 0x007fd640, `LoadStrings` 0x00498d00 (192i, unmatched:
paired with `DeleteStrings`), the button-name table 0x004bb6d4 (9), the
report-name table 0x004bb624 (25), and the feature/happiness/capacity name
tables (contents in the file header comments).

## Mechanics recovered

- **The keyword-handler contract.** `int handler(char** argv, int argc)`
  with `argv[0]` the keyword and `argc` the number of words after it;
  returns 0 when the line was not consumed (wrong section, too few words,
  unknown name) and 1 otherwise. Nothing happens unless `g_level_db_active`.
  `g_level_number` (0x00669054, the tree's name) is really the SECTION KIND
  the parser is in: the handlers test it with masks 1, 4 and 5 and compare
  it with 1 and 4. Kind 1 lines (the `[INIT]` section) apply immediately
  (`SetSimTuningA/B`, `SetFeatureFlags`, `SetReportMode`, `FlashButton`,
  `PlaceScriptObject`, the direct global stores); kind 4 lines (a step
  section) become script events through `AddEvent_<KW>` (scope W); mask 5
  keywords accept both.
- Argument shapes: `PLACE <class> <x> <y> [count]`, `DEGRADE <class> <v>
  [n]` (the optional word read only when `argc` allows it); `CLEAR`,
  `UNGLUE`, `GLUE`, `EXTENDPARK` take a rect (`ParseRectArgs` normalises the
  corners); `GLUE` in `[INIT]` sets bit 0x40 of every cell's flag word in
  the rect directly; `FMV` in a step is an event, in `[INIT]` it is
  `SetReportMovie`; `FEATURE`/`CAPACITYSCALE`/`CAPACITYCAP`/`HAP_FACTOR`
  look the first word up in their name tables; `REPORT <name> off` or
  `REPORT <name> <a> <b>` (and `HAPPY_VIS` is looked up under the table's
  misspelt `Happpy_Vis`); `MAXBLOKES` writes the map header's u16 at +0x1a
  and refreshes `g_visitor_cap` from it; `MAXCAPACITY`/`MAXVISITORS`
  (`MINCAPACITY`/`MINVISITORS`) are two table entries on one handler that
  store `g_visitor_cap` (`g_visitor_cap_extra`) in `[INIT]` and queue
  `AddEvent_Capacity(1|0, v)` otherwise; `FLASHBUTTON`/`FLASHBUTTOFF` OR
  together button names (bit index from the 9-name table) or literal masks,
  `FLASHBUTTOFF` with no words meaning all (-1).
- **`GameMain(hinst, hprev, cmdline, ncmdshow)`** (WinMain's body): a
  `SECURITY_ATTRIBUTES {0xc, 0, 0}` and `CreateMutexA("LegolandGameMutex")`
  ("Couldn't create Mutex\n" through `DBPrintf` on failure), `WaitForSingleObject(h, 0) == WAIT_TIMEOUT`
  means another instance ("Program already running.\n", `CloseHandle`,
  return 0); `WINDEBUG` selects `FlipPrimary` as `g_present` and windowed
  mode, `BLT` just `FlipPrimary`; `-nointro` sets the map header's dword at
  +0x40; `-nomusic` clears `g_music_sys` (which starts life as the "music
  enabled" flag); then `g_hinstance`, `g_ncmdshow`, `CheckHostSystemGPU`
  (its 0 is returned as is) and `InitSession`.
- **`FindCommandSwitch(cmdline, name)`** upper-cases copies of both
  (`malloc`/`strcpy`/`_strupr`), `strstr`s, maps the hit back into the
  caller's string and frees the copies; NULL when absent.
- **`InitSession`** in order: the map header's u16 at +0x1e = 1;
  `OpenDebugLog("legoland.log")`; `RES_EnsureMounted(1)`; the three
  volumes (a failure shows "Failed to open resource %s" in a "LEGOLAND
  Error" box and closes the ones already open); `LoadStrings`;
  `InitHostSystemGPU`; `InitScreen` (failure: `GetString(0xcc)` under
  `GetString(0xcb)`, kill the GPU, close the volumes); `InitInputSystem`
  (failure: `GetString(0x9c4)`, kill input and GPU, close the volumes);
  `g_pointer_table[0] = 0` and the eight pointer sprites 1..8 (`erase it`,
  `erase it2`, `no build`, `yes build`, `rab over icon`, `rab over icon2`,
  `question it`, `question it2`); `LLIDB_LoadICM`; the five menu elements
  (`BUILD MENU`, `ATTRACTIONS MENU`, `FOOD STORES MENU`, `SCENERY MENU`,
  `SHOPS MENU`, each `LLIDB_RegisterNewElement(name, 0, 0x200)`); `RunGame`;
  then `KillHostSystemGPU`, close the volumes that are open,
  `DebugPrintf("Finished shutting stuff down")`, `CloseDebugLog`,
  `DeleteStrings`, `LLIDB_CloseICM`, and the sprites killed in the order
  3, 4, 7, 8, 1, 2, 5, 6. Returns 1 on any set-up failure, 0 after a full
  session. The debug log is compiled out: open/close are `return 1` stubs.

## Extern-type divergences (caller-side levers; do not "align")

- `RES_EnsureMounted(int)` here (`push 1`); the tree declares
  `(const char* volume)`.
- `g_music_sys` is an `int` here (assigned 0/1); musicthread.c has `void*`.
- `MapHdr` views: levelkw3.c sees only `max_blokes` (u16 at +0x1a),
  startup.c `in_game` (u16 at +0x1e) and `no_intro` (int at +0x40); `Cell`'s
  flag word at +0xc is `unsigned short` (the byte-narrowed `or` depends on it).
- `PtInRect`-style: `CreateMutexA(SecAttr*, int, const char*)`,
  `WaitForSingleObject(void*, unsigned long)`, `MessageBoxA(void*, const
  char*, const char*, unsigned int)`, `GetDesktopWindow(void)` as
  `__declspec(dllimport) __stdcall` with `[0x4ab...]` comments.

## Levers, with evidence

- **A `|=` of a small constant on an `unsigned short` field is narrowed to
  `or byte ptr [base+index+disp], imm8`, and leaves a dead `lea` of the
  field's address behind.** `LevelKw_GLUE`'s loop `g_map_rows[y][x].flags
  |= 0x40` with `unsigned short flags`: 63/63. With `unsigned char flags`
  VC6 splits it into load / `or bl,0x40` / store through a `lea`'d pointer
  (58/67); through a `Cell*` it emits `mov bl,0x40 / or [mem],bl` with an
  extra `push ebx` (58/66); bit-field spellings behave like the byte.
  Template: pathgfx.c's exact `AddPathTileGFX` has the same `or byte ptr
  [ecx+edx*4+0xc],0x10 / lea eax,[ecx+edx*4+0xc]` pair from the same
  spelling. Read the field width from the neighbours, not from the `or`.
- **An index loop `for (i = 1; i <= argc; i++)` over `argv[i]` is what
  puts the pointer's `add esi,4` in the loop PREHEADER, after the guard.**
  VC6 strength-reduces it into a countdown (`cmp edi,1 / jl` guard, `dec
  edi / jne` latch) plus a pointer IV initialised after the guard. A
  separate pointer (`p = argv + 1; for (n = argc; n >= 1; n--, p++)`) puts
  the add BEFORE the guard (63/64); `if (argc >= 1) { p = argv + 1; n =
  argc; do … while (--n); }` places it right but, in `FLASHBUTTOFF`, lets
  VC6 jump-thread the preceding `if (argc == 0) bits = -1;` past the guard
  (`jmp end` instead of the fall-through, 67/68); `argv++`/`--argc` on the
  parameters themselves re-rolls the register roles (55/64). Both fills
  64/64 and 67/67 with the index loop.
- **`if (!g_level_db_active) return 1;` as an early return keeps its
  epilogue inline and lets the later pushes sink** (`PLACE` 58 -> 71/71,
  `DEGRADE` 41 -> 54/54); the enclosing `if (g_level_db_active) { … }
  return 1;` form merges the inactive path into the final return and pins
  the pushes at entry. The other twenty handlers want the enclosing form —
  the difference is whether the body has a sunk callee-saved push at all.
- **The fall-through arm of a two-way test is the one written first**:
  `REPORT`'s `if (NameCompare(argv[2], "off")) { … } else { off = 1; }`
  (the not-off arm inline, 119 -> 126) and its lookup written as two calls
  under `if (NameCompare(argv[1], "HAPPY_VIS") == 0)` (the misspelt-name
  push first, 126 -> 129/129): a ternary argument would materialise the
  pointer in a register; two calls with identical trailing arguments share
  the pushes and branch on the last one (P's `SetPointer(2)/(1)` lever).
- **Two independent stores, reverse source order** (the K/L/O/P/Q rule
  again): `MAXBLOKES` wants `g_visitor_cap = g_map->max_blokes;
  g_visitor_cap_extra = 0;` to emit extra-then-cap (30/30); `GameMain`
  wants `g_hinstance = hinst; g_ncmdshow = ncmdshow;` to emit the ncmdshow
  load first (84/84).
- **`int i` with an unsigned bound** (`i < sizeof(g_volume_names) /
  sizeof(g_volume_names[0])`) gives the original's `jb` volume loop and the
  signed `jle` of the failure loop from ONE variable; the close-on-failure
  loop is `if (i > 0) { p = g_res_volumes; do { RES_CloseVolume(*p); p++; }
  while (--i); }` — the increment as its own statement after the call
  (`*p++` moves the `add edi,4` above the push). `InitSession` 262/262.
- `if (!p) return p;` after `malloc` and `r = CheckHostSystemGPU(); if (!r)
  return r;` reproduce the `test / jne / ret` shapes with no `xor` (the Q
  `return (int)c` lever, twice more); `if (!q) { free(p); return 0; }`
  is the arm that does carry `xor eax,eax`.
- The eight `LoadSprite` stores and five `LLIDB_RegisterNewElement` calls
  share one `add esp` each (0x40 and 0x3c) — plain sequential statements,
  first compile; `PeekMessage`-style IAT hoisting did not arise here.
