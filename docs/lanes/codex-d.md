# Codex D work log

Base: `f22f7cc` (`origin/main`, 2026-09-05). Scope: `docs/SCOPE_CODEX_D.md`.
Worktree: `legoland-scope-d`; branch: `codex/scope-d`.
The scope's tables contain 49 targets (16 path/order, 33 ride-record), rather
than its approximate headline of 60. Only `LEGOLAND/pathmisc2.c`,
`LEGOLAND/ridemisc4.c`, and this report will be committed.

## Stages and completion gates

1. Environment/ownership: use the local Python/VC6 substitutions already
   approved for Scope B; check all 49 addresses for existing definitions and
   audit a matched baseline file. Originals/toolchain remain ignored symlinks.
2. Recover `pathmisc2.c`, smallest first: save each body, match its complete
   original extent with `audit.py`, compile `/W3`, then commit/push the file.
3. Recover `ridemisc4.c`, by verb in the brief's order: inspect each group's
   original bodies before writing, audit every function and `/W3`, commit/push.
   Independent read-only analysis of its globals, offsets and sibling shapes
   can run alongside stage 2; implementation remains sequential by file.
4. Final independent semantic/relocation review, all-file audits, no duplicate
   markers, scoped diff check, per-function report and remote-HEAD verification.

Done means every assigned address has an audited exact body or an explicitly
documented honest WIP, no false exact markers, clean `/W3` builds, and both
completed file checkpoints pushed to this branch. Do not run shared integration
verification/progress tools or edit another lane's files.

## Environment

Python: `/opt/homebrew/opt/python@3.14/bin/python3.14`, Capstone 5.0.7.
Compiler: `/Users/systemadmin/Downloads/alpha team/alphateam/tools/wibo-msvc/cl`.
Use `LEGOLAND_CL` for per-file audit/match tools and `ALPHATEAM_VC6_ROOT` for
direct `/W3 /O2 /Gy /Gd` checks. All objects go in `/tmp/cd_*`.

## Current checkpoint

Ownership check found no existing definitions at any of the 49 addresses.
Baseline `pathmisc.c`: 13/13 `[OK]`. Implementation now **49/49 exact,
1,288 instructions** (path16/554, ride33/734), complete byte extents, zero
mismatches/escapes; both `/W3` clean. No WIP markers remain. Path checkpoint
pushed as `f74ac85`, followed by independently reviewed switch-table correction
`b1951bc`. Final independent ride review passes with no remaining findings.
The ride checkpoint includes this report; integration target is the isolated
`codex/scope-d` branch, not main.

## Path/order results

Every row is **100%, audit `[OK]`, `FUNCTION` marker**, with no residual.
Counts are full original instructions/bytes, not matchfull's truncated window.

| Address | Function | Instructions/bytes |
|---|---|---:|
| 00482210 | FreePTPRouteList | 16/46 |
| 00489f50 | UnmarkObjectTiles | 17/57 |
| 004837a0 | RndWalk_LeftTile | 18/45 |
| 00477760 | RemoveClosedNode | 19/45 |
| 004995d0 | NewMechanicOrder | 19/67 |
| 004777c0 | FindOpenRouteNode | 20/43 |
| 00477730 | FindClosedRouteNode | 20/43 |
| 00482b60 | TileJoinsPathNetwork | 22/63 |
| 004817d0 | FindPathSquareAt | 26/59 |
| 0049b270 | EraseWorkOrdersAt | 27/69 |
| 00499570 | NewGardenerOrder | 28/85 |
| 0045ce30 | PathTileIsPatterned | 47/122 |
| 004996a0 | IsOrderSpanClear | 48/122 |
| 00499620 | AdvanceOrderSpan | 54/126 |
| 0045c9c0 | ScanPathArea5x5 | 75/206 |
| 0045cbc0 | GrowPathRectSide | 98/290 |

### Mechanics and retained edge cases

- Route searches use tile coordinates; FindPathSquareAt shifts signed 24.8
  world coordinates by eight. Rectangles have inclusive edges.
- Unmark clears the first matching packed key, leaving the slot count alone.
  RemoveClosedNode retains the null dereference on an empty/missing-target list.
- Order constructors use calloc(0x3c,1), append and increment without checking
  allocation failure. Gardener prints the original `Allocated Workorder %x\n`
  string (004bff88). EraseWorkOrdersAt ignores its object argument and removes
  the mechanic match before the gardener match.
- Order spans sweep the footprint perimeter at offsets -1 through extent+1.
  The first two direction checks are independent; 5/3 are an if/else-if pair.
  Span-clear rejects off-map cells and non-environment objects under flags0x88.
- TileJoinsPathNetwork calls RefreshEntranceTile with the integer force flag
  at0066b46c and clears it before testing square flag2. Despite the old global
  name `g_path_gfx_batch`, this is not an allocated graphics-batch pointer.
- Patterned-path testing reloads map/position after IsPathCell and compares a
  zero-extended tile WORD to the full DWORD at *g_path_tile_base.
- ScanPathArea5x5 copies 20-byte cells and emits bit24 top-left to bit0
  bottom-right. Off-map substitutes initialize only flags0x40/rf0.
  GrowPathRectSide probes one external row/column; directions0..3 are bottom,
  right, top, left (distinct from emitted block order). Invalid direction
  returns0. New unique callee name:
  **IsPathRectClear,0045c900**, all tiles flags0x10 and !rf2 (empty rect true).

### Measured codegen levers

- Unmark's `int i=0` before computing the ushort key moves the zero into its
  original position: four index mismatches become zero (17i/57B unchanged).
- Explicit `int x=pos.x; int y=pos.y;` before EraseWorkOrdersAt calls restores
  the original callee-saved assignment; inline argument fields gave8 mismatches.
- NewGardenerOrder needs tail assignment inside both arms. A shared tail store
  emitted72B versus85B; duplicating the assignment restores all28 instructions.
- **Normalized audit is not sufficient for global identities.** Independent
  COFF relocation review caught Mechanic's initially swapped head/tail stores
  at instruction indices8/10, despite its19i/67B zero-mismatch audit. It too now
  explicitly writes tail then head in the empty arm. Recompiled and re-audited.
- ScanPathArea5x5 requires one **Pos aggregate** for the loop coordinates.
  Plain ints allowed a derived row-address induction variable (0x20 frame,
  218B audited prefix, escaping extent). A volatile global read suppressed
  that IV but left33 mismatches; it was removed. Pos alone gives75i/206B exact,
  0x1c frame, no volatile and no extra instructions.
- Grow's source assignment order matters: horizontal cases chain top/bottom
  first then left,right; vertical cases store top,bottom then chain left/right.
  The initial equally meaningful order had12 mismatches at98i/290B; final0.
- Independent switch-table review caught a second normalized-audit blind spot:
  emitted block order is top,bottom,right,left, but the original table indexes
  bottom,right,top,left. Corrected labels while preserving emitted block order;
  the four relocated targets must be checked separately from normalized text.
- Existing siblings were reused without speculative abstractions: route free,
  closed unlink, route finds and bounds helpers matched directly.

Caller-local type views: EraseWorkOrdersAt uses a two-byte BPos by value;
RefreshEntranceTile uses int (older callers use pointer-like views);
g_path_tile_base is int* (the original reads a DWORD). No other file's extern
types were changed. Existing symbol names retained, including CRT_calloc's
existing wrapper alias. New layouts are local to the new translation unit.

## Ride-record results

Every row is **100%, audit `[OK]`, `FUNCTION` marker**, with no residual.
734 original instructions in33 functions; all byte extents match as well.

| Address | Function | Instructions/bytes |
|---|---|---:|
| 0043be40 | SpinningBarrels_FindRecord | 18/47 |
| 0042f9d0 | Restaurant2_FindRec | 18/47 |
| 0042ef40 | Restaurant1_FindRec | 18/47 |
| 0043d960 | PlaneRide_FindRecord | 16/40 |
| 0043ac40 | SpaceTower_FindRecord | 16/40 |
| 0043bdb0 | SpinningBarrels_AddRecord | 23/67 |
| 0043d880 | PlaneRide_AddRecord | 22/63 |
| 0043ab70 | SpaceTower_AddRecord | 22/66 |
| 004158f0 | SpiderRide_AddRecord | 22/63 |
| 004149c0 | SafariRide_AddRecord | 22/63 |
| 00403c40 | Copters_AddRecord | 22/64 |
| 0042cd70 | EarthSlide_NewRecord | 28/80 |
| 0042eec0 | Restaurant1_NewRecord | 22/65 |
| 0042bbc0 | Carousel_NewRecord | 22/63 |
| 00406920 | GoldRush_NewRecord | 19/54 |
| 0042fa00 | Restaurant2_FreeRec | 25/58 |
| 0042ef70 | Restaurant1_FreeRec | 25/58 |
| 0042bc00 | Carousel_FreeRec | 25/58 |
| 0042a9b0 | Balloonz_FreeRec | 25/58 |
| 00415760 | SafariRide_SeatOf | 28/65 |
| 0043e050 | PlaneRide_SeatOf | 25/65 |
| 0043ce10 | SpinningBarrels_SeatOf | 25/65 |
| 00416830 | SpiderRide_SeatOf | 25/65 |
| 00404f20 | Copters_CopterOf | 23/56 |
| 0043d990 | PlaneRide_SetFull | 27/84 |
| 00414ab0 | SafariRide_SetFull | 27/84 |
| 0043be00 | SpinningBarrels_RemoveRecord | 25/58 |
| 0042ce50 | EarthSlide_JoinQueue | 24/62 |
| 0043ac70 | SpaceTower_TakeSeat | 19/55 |
| 0042fb60 | Restaurant2_StopSound | 21/78 |
| 00411ba0 | Pump_RemoveAllForSchool | 18/43 |
| 00436130 | JungleCruise_AddValue | 17/44 |
| 00406ec0 | GoldRush_ClaimPan | 20/56 |

### Mechanics and original edge cases

- Five finds scan by packed two-byte map key, return first match or NULL.
  The three +4-key records have18 instructions; +0-key records have16.
- All constructors allocate and clear the full record before publishing its
  key/link. Sizes: Barrels0x34, Plane0x24, Tower0xb4, Spider0x30, Safari0x28,
  Copters0xd8, Slide0x24, Restaurant1 0x0c, Carousel0x2c, Gold0x2c.
  Slide preserves explicit redundant field clears and state1; Restaurant1
  preserves the second memset of its three-byte seats subobject.
- Carousel's short caller view totals0x24, but original allocation/clear is
  0x2c. Slide's queue tail at+0x20 is needed for its full0x24 allocation.
  No existing caller layout was modified.
- Copters initializes even after allocation failure, handing NULL to a callee
  that writes through it. Other add-record resets are inside the success arm.
- Five zero-offset-next frees keep the null-head dereference, redundant
  postloop nonnull check and unconditional free even if no predecessor is found.
- Safari/Copters count only riders at the requested square before the target.
  Absent target returns0. Safari stores the zero-based ordinal; Copters does not.
- Plane/Spider/Barrels allocate a random starting seat, scan forward with wrap,
  mark it occupied, then store/return a **one-based ID**. The capacity is signed
  char: zero divides by zero, negative/oversized values are unchecked, and a
  fully occupied array loops forever. None of these defects was repaired.
- Actual zero-based seat storage starts at+0x1c in Plane/Spider and+0x21 in
  Barrels. Older callers' +0x1b/+0x20 views index the one-based ID. Their comments
  claiming valid seat zero overlaps timer/frame are misleading; valid IDs here
  never return zero. No surrounding implementation or comments were edited.
- SetFull transfers seated→riders, clears0x4000/sets1, resets seated/frame/half,
  and loops the first ride sample. Plane counts/frame are bytes; Safari's ints.
  SoundSource kind2 uses x/y; the unused person pointer is left uninitialized.
- Slide queue adds an eight-byte node only on successful allocation, marks
  rider flag0x40, appends, then increments a byte count (no overflow check).
- Tower stores a zero-based chosen seat as the animation index, then loads a
  WORD stop-part from an eight-byte row table. Stop parts are4,3,4,3,2,1,2,1;
  the other column is the animation pointer, not another stop-part value.
- Restaurant2_StopSound takes its square **by value**, fades source sounds
  with-200 then plays sample2 (elevator stop) with arguments0,1. It is not just
  a silence operation. Pump removal caches next before the possible free.
- JungleCruise_AddValue adds a signed scenery contribution at station+0x40,
  first match only, absent key no-op. It is not cash/income in this caller flow.
  GoldRush claims the first free one of six DWORD pan slots and writes a
  zero-based byte index; missing record/full slots leave the rider unchanged.

### Reused and measured codegen levers

- Existing Carousel_FindRec's intrinsic `memcmp(&rec->tile,tile,2)` source
  reproduced all five finds immediately, including both otherwise-dead LEAs.
  No volatile or bytewise comparison workaround was added.
- Existing constructor patterns reproduced all ten allocations at first
  compile, retaining memset sizes and the subobject memset's second zero reg.
- For **next at offset0**, the nonzero-offset unlink's separate `link` web
  was not suitable: it emitted31 instructions. A direct `node->next` loop with
  a plain body read emitted27 (load forwarded into compare). Keeping the direct
  loop and one volatile body read through `&node->next` closes25/25. Applied to
  all five copies; no synthetic arithmetic or padding. This is a compile-time
  source-shape difference, not a generic runtime abstraction.
- Safari/Copters require `int seat=0` declared before the rider-list cursor.
  Cursor first delayed the zero and used the wrong prologue register (Safari
  also had a tail register difference). Counter first closes28/28 and23/23.
- Carousel_PickSeat's signed-char modulo/wrap source reproduces all three
  random seat allocators first try. Set-full and remaining misc bodies likewise
  matched on their initial compile, with the unlink/ordinal changes above
  being the only ride iterations.

### Symbols and caller-local ABI views

New callee names (no pre-existing symbol/address definition found; declarations
only, no out-of-scope implementation):

| Address | Name | Meaning |
|---|---|---|
| 0043c2f0 | SpinningBarrels_StopRide | reset counters/revs, clear run/boarding |
| 0043d9f0 | PlaneRide_StopRide | reset counters/run length, fade source |
| 0043aac0 | SpaceTower_StopRide | reset machine and cars |
| 00415a90 | SpiderRide_StopRide | reset counters/run length/seats |
| 00414b10 | SafariRide_StopRide | reset counters/revolutions, fade source |
| 00403e90 | Copters_InitRecord | initialize five per-copter mappings then reset |
| 0043acb0 | SpaceTower_PickSeat | wrapped random eight-seat allocation |
| 0042ce90 | EarthSlide_AppendQueue | queue head/tail append |

New data name `g_tower_seat_anim` at004b7758 is an array of
`{int stop_part; void* anim;}`. Existing g_bloke_anim_ref at004b775c is its
shifted pointer-column view. Sample lists retain existing names but are typed
FXEntry arrays here (12-byte rows, sample at+8), instead of old pointer-shaped
views. Therefore g_plane_fx[0].sample→004b79d8, g_safari_fx[0].sample→004b4cc0,
g_rest2_fx[2].sample→004b6988.

Restaurant2_StopSound, Pump_RemoveAllForSchool and JungleCruise_AddValue use a
two-byte union by value, while some old callers declare ushort. The original
cdecl dword stack home is preserved; no caller declaration was aligned. Other
pointer types are local record views of the same argument ABI. Intrinsic
memcmp/memset produce no external calls. HeapAlloc_w and rand retain existing
ride-family symbol aliases at0049e4ff/0049e4b2.

## Verification and integration boundary

- Authoritative command: `LEGOLAND_CL=... python3 tools/audit.py
  LEGOLAND/pathmisc2.c LEGOLAND/ridemisc4.c`:49 `[OK]`, no WIP/REJECT/ESCAPES.
- Direct VC6 `/W3 /O2 /Gy /Gd` compilation of each file: no warnings.
- Read-only scope-address/name/ownership check: all49 targets present exactly
  once, correctly named, with FUNCTION markers, solely in the two owned files.
- Path independent review: all54 external symbols/addends, original debug
  literal and four switch targets match final `/tmp/cd_path_w3.obj`.
  SHA256: `5cf6f57ff0c4b364946320fa4478621bbfa3072084134c8a0284f438848ae9ff`.
- Ride independent final review: all33 functions/734 instructions and all83
  external symbols/addends pass against `/tmp/cd_ride_w3.obj`; correct FX offsets
  and tower table base/low-WORD read independently confirmed. No blockers.
  SHA256: `4abd444bd1e58c77e5e416027ea659942900ab3d4ccb2f2ea34d180aaf0f20ac`.
- Changes are restricted to two new C files and this note. Original EXE,
  toolchain, assets and scratch notes are excluded from commits. No shared
  integration scripts, tools, existing C or global project reports were changed.

Limitations: this is matching decompilation, not a rebuilt playable-game test.
Per the scope, shared verify/progress/coverage and whole-game integration are
left to the integrator. Normalized instruction comparison alone cannot prove
global identity or jump-table contents; the separate relocation/table review
is essential and caught two real path issues before final handoff. Original
invalid-input/allocation bugs remain intentionally present. Main is untouched.
