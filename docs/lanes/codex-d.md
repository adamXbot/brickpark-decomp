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
Baseline `pathmisc.c`: 13/13 `[OK]`. Path/order checkpoint: **16/16 exact,
554 instructions**, complete byte extents, zero mismatches/escapes; `/W3`
clean. Ride-record implementation has not yet started.

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
