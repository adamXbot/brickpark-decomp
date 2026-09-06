# Scope Y — report setters and appraisal helpers

Completed 2026-09-07 on `scope/Y`, based on `main` at `6a95613e`.
Files: `LEGOLAND/reportset.c` and `LEGOLAND/appraisal.c`.

**48 of 48 exact functions, 1,233 instructions, 4,015 bytes.**
The 25 report setters and all 23 appraisal helpers pass the complete-body
instruction and extent gate. No WIPs remain. The 8,085-instruction appraisal
screen at `0x004453a0` remains outside this scope.

Validation: both files audit PASS with 48 `[OK]` lines and compile cleanly
under VC6 SP3 `/W3 /O2 /Gy /Gd`. Relocation checks resolve 127 positions in
`reportset.c` and 162 in `appraisal.c`, with zero mismatches. The 17 unresolved
positions are string literals; each was read from the original executable
and checked against the source. All 48 assigned addresses occur exactly once,
with no overlap against main including scope Z. Full-tree verification and
progress regeneration belong to integration.

## Per-function results

Every row is 100%, `audit [OK]`, and committed with a `FUNCTION` marker.
The instruction and byte counts equal the original extent, with zero strict
mismatches and no escaping branch. First divergence and residual: none.

| Address | Name | Instructions | Bytes |
| --- | --- | ---: | ---: |
| `0x004442c0` | `CountDrivingSchools` | 15 | 46 |
| `0x004442f0` | `CountBoatingSchools` | 15 | 46 |
| `0x00444320` | `CountCastles` | 15 | 46 |
| `0x00444350` | `CountLogFlumes` | 15 | 46 |
| `0x00444380` | `CountJungleCruises` | 15 | 46 |
| `0x004443b0` | `SetReport_ZONE_LL` | 14 | 48 |
| `0x004443e0` | `SetReport_ZONE_ADV` | 14 | 48 |
| `0x00444410` | `SetReport_ZONE_MED` | 14 | 48 |
| `0x00444440` | `SetReport_ZONE_WES` | 14 | 48 |
| `0x00444470` | `SetReport_COASTER` | 16 | 54 |
| `0x004444b0` | `SetReport_DSCHOOL` | 16 | 54 |
| `0x004444f0` | `SetReport_LFLUME` | 16 | 55 |
| `0x00444530` | `SetReport_BSCHOOL` | 16 | 55 |
| `0x00444570` | `SetReport_JCRUISE` | 16 | 55 |
| `0x004445b0` | `SetReport_NUM_ATTRACTIONS` | 15 | 55 |
| `0x004445f0` | `SetReport_VAR_ATTRACTIONS` | 15 | 55 |
| `0x00444630` | `SetReport_RIDE_ACCESS` | 13 | 55 |
| `0x00444670` | `SetReport_NUM_SCENERY` | 13 | 55 |
| `0x004446b0` | `SetReport_VAR_SCENERY` | 13 | 55 |
| `0x004446f0` | `SetReport_COV_SCENERY` | 13 | 55 |
| `0x00444730` | `SetReport_NUM_FOOD` | 13 | 55 |
| `0x00444770` | `SetReport_VAR_FOOD` | 13 | 55 |
| `0x004447b0` | `SetReport_NUM_SHOPS` | 13 | 55 |
| `0x004447f0` | `SetReport_VAR_SHOPS` | 13 | 55 |
| `0x00444830` | `SetReport_NUM_VIS` | 13 | 55 |
| `0x00444870` | `SetReport_HAPPPY_VIS` | 13 | 55 |
| `0x004448b0` | `SetReport_HUNGRY_VIS` | 13 | 55 |
| `0x004448f0` | `SetReport_POWER` | 13 | 55 |
| `0x00444930` | `SetReport_WORKING_RIDES` | 13 | 55 |
| `0x00444970` | `SetReport_STUDDED` | 13 | 55 |
| `0x004449b0` | `LoadAppraisalTickSprites` | 60 | 183 |
| `0x00444a70` | `DrawAppraisalBar` | 98 | 246 |
| `0x00444b70` | `BlitAppraisalSprite` | 42 | 122 |
| `0x00444bf0` | `CountAttractions` | 29 | 74 |
| `0x00444c40` | `IsRidePartClass` | 13 | 38 |
| `0x00444c70` | `CountScenery` | 34 | 84 |
| `0x00444cd0` | `CountFood` | 26 | 65 |
| `0x00444d20` | `CountShops` | 26 | 65 |
| `0x00444d70` | `CountVisitors` | 42 | 118 |
| `0x00444df0` | `PercentObjectsLinked` | 74 | 186 |
| `0x00444eb0` | `AppraisalGoBack` | 14 | 60 |
| `0x00444ef0` | `AppraisalNextPage` | 48 | 157 |
| `0x00444f90` | `AppraisalPrevPage` | 27 | 98 |
| `0x00445000` | `FreeAppraisalScreenSprites` | 83 | 256 |
| `0x00445100` | `UnlightAppraisalPageButtons` | 49 | 143 |
| `0x00445190` | `LoadAppraisalScreenSprites` | 91 | 376 |
| `0x00445310` | `UpdateAppraisalPageButtons` | 38 | 133 |
| `0x004636c0` | `MapCellCount` | 16 | 36 |

## Recovered behavior and names

- `SetReport_*` names follow the 25 REPORT keywords, including the original
  `HAPPPY_VIS` spelling. Both inputs zero disable the report without clearing
  its saved values. Other inputs store the count/target and OR the report
  bits into `g_report_state.flags`. The five ride setters OR a two-bit mode;
  they do not replace an existing mode. Happy and hungry visitors share one
  argument pair and have separate enable bits.
- `CountDrivingSchools`, `CountBoatingSchools`, `CountCastles`,
  `CountLogFlumes` and `CountJungleCruises` look up their named class, require
  its loaded bit, then invoke its count callback at +0xc0 with the class's
  element at +0xc4. `MapCellCount` traverses the map width and height.
- `CountAttractions`, `CountScenery`, `CountFood` and `CountShops` sum each
  populated class's instance count and increment its variety once. Attractions
  use class types 1/3, scenery type 2 excluding the 22 ride-part class names,
  food type 4, and shops type 5. `IsRidePartClass` names that exclusion test.
- `CountVisitors` counts people, adds one for each of two mood thresholds
  exceeded, and adds `4 - GetBlokeAgeGroup(person)` to the age-group score.
- `PercentObjectsLinked` counts placed objects with cell flag 0x80 whose class
  type is 1, 4 or 5. It adds the class origin to the cell coordinates, tests
  the entrance path network, and logs an unlinked class's name. The empty
  park returns 100; otherwise it returns the integer percentage reached.
- `LoadAppraisalTickSprites` loads five tick, cross and bullet sprites into
  one three-row array, plus the bar and target-marker sprites.
  `BlitAppraisalSprite` chooses tick for kind 1, cross for 0, bullet for -1;
  the bullet is offset by five pixels in both directions. Rendering state is
  pushed and restored even for an unrecognized kind.
- `DrawAppraisalBar` (`0x00444a70`, the brief's `sub_444a70`) draws the bar
  sprite, two rectangles for its filled portion, and the target marker.
  A negative range reflects both the value and target; the value is capped
  above the absolute range before reflection. Green means the resulting
  value meets or exceeds the target, red means it does not. The first argument
  is a four-int box passed by value, followed by value, range and target.
- `AppraisalGoBack`, `AppraisalNextPage` and `AppraisalPrevPage` are the three
  icon callbacks. Page changes set the page-turn flag, play the click, pause
  the music, set the hold-off counter to four and update the enabled buttons.
  Going back clears the open flag and both page-icon pointers.
- `LoadAppraisalScreenSprites` loads the background and four page-button
  sprites, creates and labels the three icons, registers the callbacks and
  opens the screen. `UpdateAppraisalPageButtons` clears or sets disabled
  bit 0x400 and the handler pointers according to the page bounds.
  `UnlightAppraisalPageButtons` restores the normal sprite when the cursor
  leaves each icon box. `FreeAppraisalScreenSprites` unreferences the mark,
  bar, background and button sprites, clears their globals and removes
  icon group 1.

## Original edge cases retained

- The next-page hover guard is `!flags & 0x400`, which is always false because
  logical negation is evaluated first. Its lit sprite is therefore not drawn
  through this guard. The source comments this original precedence mistake.
- The bar has no lower clamp and no zero-range guard. Its integer products,
  divisions and negative-range reflection retain the original 32-bit behavior.
- Happy/hungry report parameters alias and ride modes accumulate through OR.
  These are preserved rather than silently changed to independent state or
  replacement assignments.

## Compiler levers and declaration reconciliation

- **Set the flag before the report values in source.** VC6 then hoists the
  flags load, keeps the full-width OR and sinks the flags store between/after
  the value stores as in the original. Putting the flag assignment last
  narrows the OR and removes the interleave (11 of 14 instructions in the
  first zone setter). One matching source shape transfers to all 25 setters.
- **Use one 3x5 mark array.** The tick/cross/bullet loader strength-reduces onto
  its middle row, accessing the neighboring rows at -0x14/+0x14. Three
  independent array globals do not express that original layout. The local
  formatted-name buffer is 0x20 bytes, not the brief's proposed 0x1e bytes.
- **Put the nonempty link percentage before the empty-park return.**
  `if (total != 0) return linked * 100 / total; return 100;` matches 74/74.
  An early `if (total == 0) return 100;` duplicates/relocates the epilogue
  and scored 67/74 in the interrupted session.
- **The bar's box is a by-value aggregate.** Four scalar coordinates allow
  the shared `y+2` temporary to reuse the dead right-coordinate parameter
  slot, eliminating the original four-byte local frame (96 instructions).
  `AppraisalBox` protects those parameter homes, yielding all 98 instructions
  and 246 bytes exactly. This is the same parameter-home mechanism observed
  in Z's `TurnIfBlocked`, transferred to four coordinates.
- **Assign the negative flag in both arms; keep green first.** An initial
  zero assignment moves its XOR into the prologue. An explicit `else` keeps
  it in the original nonnegative branch. `value >= mark` places green inline
  and emits the original `jl` to the red branch; the inverse condition does
  not have the same block order.
- `KillSprite` at `0x00497bd0` was renamed here to main's
  `UnreferenceSprite`; its return type and every call remain unchanged and
  the whole file re-audits exact. The sprite decrement operation is distinct
  from destruction. `RenderBlock` uses its existing int-returning definition.
- The `AppraisalBox` parameter occupies the same four stack words as scalar
  coordinates; callers outside this scope were not changed. Class callbacks,
  icon callback return types and narrow icon parameters retain the recovered
  caller-side types. The report dispatcher at `0x0046a140` belongs to V and
  is named `SetReportParam` in its latest source.

## Integration

Merge only the two new C files and this report. Fold these levers into
`docs/DECOMP.md`, update the Y brief and shared status documents, regenerate
progress and run full verification alone. Preserve V's and F/G/H's separate
worktrees and uncommitted progress.
