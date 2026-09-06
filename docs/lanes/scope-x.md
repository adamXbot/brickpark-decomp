# Scope X — event ticks, part 2, and goal primitives

Completed 2026-09-06 on `scope/X`, based on main at `9c0d00ec`.
Worktree: `.worktrees/scope-x`. Only `LEGOLAND/eventgoalprim.c`,
`LEGOLAND/eventtick2.c`, and this report belong to the change.

All **54 of 54 functions** pass the authoritative full-body audit: equal
instruction count and byte length, zero normalized instruction mismatches,
and no escaping branch. Every function carries `// FUNCTION: LEGOLAND`
with its address and no percentage suffix. There are no WIPs or residuals.

| File | Exact functions | Instructions | Bytes |
| --- | ---: | ---: | ---: |
| `LEGOLAND/eventgoalprim.c` | 22 | 353 | 1020 |
| `LEGOLAND/eventtick2.c` | 32 | 858 | 2063 |

Validation: `tools/audit.py` PASS for both files; `/W3 /O2 /Gy /Gd`
compilation clean for both; `tools/relocs.py` zero `MISMATCH` in both.
Primitives: 63 of 63 relocations resolved and matched. Ticks: 73 matched,
one unresolved string literal. The latter was read directly at `0x004ba7e4`
and is exactly `Cannot evaluate LoopSize for %s\n`.
All 54 assigned addresses are present once and none was already defined in
the main checkout. No global verification or progress regeneration was run;
those remain the integrating session's responsibility.

## Per-function results

All percentages below are strict normalized instruction matches over the
original's complete extent; every row is `audit [OK]`, with marker
`FUNCTION`, and has no first divergence or residual.

### `LEGOLAND/eventgoalprim.c`

| Address | Name | Instructions | Bytes | Match | Audit | Marker |
| --- | --- | ---: | ---: | ---: | --- | --- |
| `0x004687f0` | `SetBriefingFile` | 8 | 31 | 100% | [OK] | FUNCTION |
| `0x00468810` | `SetHintsFile` | 8 | 31 | 100% | [OK] | FUNCTION |
| `0x00468860` | `SetThemeIcon` | 13 | 38 | 100% | [OK] | FUNCTION |
| `0x00468890` | `AddLevelFlag` | 11 | 33 | 100% | [OK] | FUNCTION |
| `0x004688c0` | `GetLevelFlag` | 7 | 19 | 100% | [OK] | FUNCTION |
| `0x004688f0` | `SetBridges` | 7 | 17 | 100% | [OK] | FUNCTION |
| `0x00468c80` | `QueuePendingEvent` | 27 | 77 | 100% | [OK] | FUNCTION |
| `0x00468cd0` | `NewTimedEvent` | 15 | 37 | 100% | [OK] | FUNCTION |
| `0x00468d10` | `HintTimerDue` | 9 | 32 | 100% | [OK] | FUNCTION |
| `0x00468d30` | `ShowGoalHint` | 27 | 71 | 100% | [OK] | FUNCTION |
| `0x00468d80` | `GoalCheck_Need` | 20 | 59 | 100% | [OK] | FUNCTION |
| `0x00468dc0` | `GoalCheck_Connect` | 19 | 59 | 100% | [OK] | FUNCTION |
| `0x00468e00` | `GoalCheck_Link` | 19 | 59 | 100% | [OK] | FUNCTION |
| `0x00468e40` | `GoalCheck_Range` | 31 | 83 | 100% | [OK] | FUNCTION |
| `0x00468ea0` | `GoalCheck_RemoveRange` | 31 | 83 | 100% | [OK] | FUNCTION |
| `0x00468f00` | `GoalCheck_ClearArea` | 18 | 52 | 100% | [OK] | FUNCTION |
| `0x00468f40` | `GoalCheck_Remove` | 20 | 59 | 100% | [OK] | FUNCTION |
| `0x00476070` | `FlashButton` | 23 | 46 | 100% | [OK] | FUNCTION |
| `0x00476140` | `SetThemeIconEnabled` | 16 | 54 | 100% | [OK] | FUNCTION |
| `0x00457900` | `SetCurrency` | 3 | 10 | 100% | [OK] | FUNCTION |
| `0x0044db40` | `ResetAppraisalDeadline` | 17 | 54 | 100% | [OK] | FUNCTION |
| `0x00482d60` | `SetHappinessFactor` | 4 | 16 | 100% | [OK] | FUNCTION |

### `LEGOLAND/eventtick2.c`

| Address | Name | Instructions | Bytes | Match | Audit | Marker |
| --- | --- | ---: | ---: | ---: | --- | --- |
| `0x0046a900` | `EventTick_Range` | 44 | 92 | 100% | [OK] | FUNCTION |
| `0x0046a960` | `EventTick_Cleararea` | 81 | 197 | 100% | [OK] | FUNCTION |
| `0x0046aa30` | `EventTick_Remove` | 22 | 51 | 100% | [OK] | FUNCTION |
| `0x0046aa70` | `EventTick_Removerange` | 50 | 105 | 100% | [OK] | FUNCTION |
| `0x0046aae0` | `EventTick_Composite` | 60 | 130 | 100% | [OK] | FUNCTION |
| `0x0046ab70` | `EventTick_Loopcomposite` | 28 | 72 | 100% | [OK] | FUNCTION |
| `0x0046abc0` | `EventTick_Techlevel` | 5 | 14 | 100% | [OK] | FUNCTION |
| `0x0046abd0` | `EventTick_Parkvisitors` | 14 | 38 | 100% | [OK] | FUNCTION |
| `0x0046ac00` | `EventTick_Riders` | 31 | 67 | 100% | [OK] | FUNCTION |
| `0x0046ac50` | `EventTick_Ridevisitors` | 69 | 172 | 100% | [OK] | FUNCTION |
| `0x0046ad00` | `EventTick_Scenerycoverage` | 15 | 40 | 100% | [OK] | FUNCTION |
| `0x0046ad30` | `EventTick_Pathscenery` | 15 | 43 | 100% | [OK] | FUNCTION |
| `0x0046ad60` | `EventTick_Ridecoverage` | 15 | 40 | 100% | [OK] | FUNCTION |
| `0x0046ad90` | `EventTick_Shopcoverage` | 15 | 40 | 100% | [OK] | FUNCTION |
| `0x0046adc0` | `EventTick_Foodcoverage` | 15 | 40 | 100% | [OK] | FUNCTION |
| `0x0046adf0` | `EventTick_Totcoverage` | 15 | 40 | 100% | [OK] | FUNCTION |
| `0x0046ae20` | `EventTick_Kind54` | 2 | 6 | 100% | [OK] | FUNCTION |
| `0x0046ae30` | `EventTick_Studarea` | 5 | 14 | 100% | [OK] | FUNCTION |
| `0x0046ae40` | `EventTick_Save` | 18 | 46 | 100% | [OK] | FUNCTION |
| `0x0046ae70` | `EventTick_Happiness` | 34 | 74 | 100% | [OK] | FUNCTION |
| `0x0046aec0` | `EventTick_Needgardeners` | 31 | 72 | 100% | [OK] | FUNCTION |
| `0x0046af10` | `EventTick_Needmechanics` | 31 | 72 | 100% | [OK] | FUNCTION |
| `0x0046af60` | `EventTick_Hunger` | 59 | 128 | 100% | [OK] | FUNCTION |
| `0x0046afe0` | `EventTick_Fixrides` | 61 | 146 | 100% | [OK] | FUNCTION |
| `0x0046b080` | `EventTick_Powerrides` | 15 | 39 | 100% | [OK] | FUNCTION |
| `0x0046b0b0` | `EventTick_Zoning` | 2 | 6 | 100% | [OK] | FUNCTION |
| `0x0046b0c0` | `EventTick_Checkflag` | 22 | 56 | 100% | [OK] | FUNCTION |
| `0x0046b100` | `EventTick_Selecttheme` | 18 | 47 | 100% | [OK] | FUNCTION |
| `0x0046b130` | `EventTick_Selecttab` | 30 | 76 | 100% | [OK] | FUNCTION |
| `0x0046b180` | `EventTick_Selectmode` | 29 | 83 | 100% | [OK] | FUNCTION |
| `0x0046b1e0` | `EventTick_Kind68` | 5 | 14 | 100% | [OK] | FUNCTION |
| `0x0046b1f0` | `EventTick_Forever` | 2 | 3 | 100% | [OK] | FUNCTION |

## Recovered mechanics

- **Goal primitives create timed hints; they do not compare the target.**
  The scope brief's tentative target/compare interpretation of
  `0x00468c80..0x00468d30` is superseded by the bodies: `QueuePendingEvent`
  inserts a hint into `g_goal_list` by ascending mode and increments its
  kind count; `NewTimedEvent` allocates and stamps the game timer;
  `HintTimerDue` resets the shared timer only when the signed elapsed time
  is strictly greater than 50,000; `ShowGoalHint` creates a kind-0 hint when
  +0x40 names a nonzero, populated hint-string slot. Flag 4 chooses mode 0,
  otherwise mode 1. These four names agree with scope V's existing externs.
- **The +0x40 event field is overloaded.** Constructors describe it as the
  root goal, but `ShowGoalHint` treats its dword as an index into
  `g_hint_strings`. The local helper layout names this interpretation
  `hint`; the save layout remains unchanged. Both are 0x44-byte records.
- **Default hints:** `GoalCheck_Need` (kind 1) carries the element and count;
  `GoalCheck_Connect`/`GoalCheck_Link` (kind 2) differ by +0x1c = 0/1;
  `GoalCheck_Range`/`GoalCheck_RemoveRange` (kinds 3/4) clamp each deficit at
  zero and store distinct types at +0x14, total count at +0x1c;
  `GoalCheck_ClearArea` (kind 5) carries count; `GoalCheck_Remove` (kind 6)
  carries element and count. Each first checks the timer and the custom
  hint. Their descriptive names replace the brief's address placeholders.
- **RANGE** totals instances and nonempty definitions whose +0x5c matches
  the selected element. Both minima must hold. **REMOVERANGE** applies
  maxima to the same totals; a target of -1 disables that particular
  constraint. **COMPOSITE** first requires at least one instance of the
  parent, then checks nonempty definitions whose +0x58 is the parent. Its
  generic hint receives count deficit before distinct-type deficit.
- **CLEARAREA** scans the inclusive rectangle, counting only occupied
  footprint origins (`flags & 0x80`, base x/y equal to the scanned cell),
  and succeeds when no more than the permitted count remain.
- **LOOPCOMPOSITE** calls the class's +0xc0 callback with `(elem, 1)` and
  compares the result to the target. If the callback is absent, it logs
  the element name and returns success; that permissive behavior is original.
- **RIDERS** walks the class's +0xcc list, stopping at the requested count.
  **RIDEVISITORS** walks the +0x04 instance list, compares element names
  without case, resolves the first corresponding map object, and compares
  its position counter to the target. It tracks the largest observed
  counter for the hint, not a sum across instances. The newly declared
  `GetRideVisitCountAt` at `0x00489fd0` searches 128 four-byte records using
  the 16-bit key `(x << 8) + y` and returns the accompanying 16-bit count,
  or zero on a miss. Its name follows this caller; no body is added here.
- **Coverage goals** compare the existing category tallies with +0x14.
  Hint categories are scenery 2, rides 1, shops 4, food 5, total 0.
  PATHSCENERY first refreshes `TallyBuildFootprints`, then uses its percent
  tally. PARKVISITORS uses the live visitor count; SAVE means a brick-count
  minimum and deliberately calls `GetBrickCount` twice on the failure path.
- **HAPPINESS** counts people whose signed 16-bit happiness meets the
  threshold. **HUNGER** uses an unsigned 16-bit value: nonzero +0x18 limits
  the number at or above the hunger threshold; zero +0x18 requires a minimum
  number at or below it. The respective hints carry surplus or deficit.
- **Staff targets** are signed: positive means a minimum; zero or negative
  means an upper bound on count of `-target`. The two hint constructors
  distinguish shortage and surplus.
- **FIXRIDES** visits the render-object chain, skips cells with zero
  condition, and computes `cell.condition * 100 / class.condition` (100
  when the class denominator is zero). When the threshold is 25 and flag 4
  is set, condition is treated as 100. It counts objects below threshold
  and allows at most the requested total. **POWERRIDES** uses the number of
  unserved powered objects, not the bridge-pointer interpretation in the
  inventory brief: `g_power_unserved_n` at `0x00832bdc`, already named by
  `power.c`, is the matching scalar.
- **UI goals:** CHECKFLAG compares a signed byte; SELECTTHEME compares the
  current menu index; SELECTTAB requires a menu other than sentinel 5 and
  the same boolean state for the target and object-list mode. SELECTMODE
  target 3 means edit mode 1 with the environment class selected; other
  targets compare the edit mode directly. Their failures only try a custom
  timed hint.
- **Stubs remain faithful:** kinds 54 and 63 succeed; FOREVER fails;
  TECHLEVEL, STUDAREA, and kind 68 delegate to scope V's diagnostic handlers.
- **Level state:** filenames are copied into 128-byte buffers and forcibly
  terminated. `SetThemeIcon` stores a signed byte in the ten-slot script
  state, then updates a UI icon for the first four slots. `AddLevelFlag`
  wraps the byte addition and `GetLevelFlag` returns it signed.
  `SetBridges` triggers the indexed switch. `FlashButton` walks nine bits.
  `SetThemeIconEnabled` toggles icon flag 0x400; enabling also records the
  theme in the current profile and saves that profile's state, while
  disabling does not clear its persisted unlock.
- **Appraisal and tuning:** `ResetAppraisalDeadline` at `0x0044db40` converts
  `g_appraisal_minutes` to milliseconds and adds the game timer, or clears
  the deadline for zero. `SetCurrency` sets the signed brick count, and
  `SetHappinessFactor` stores directly in the existing mood-adjustment array.

## Original bugs and edge cases retained

- `QueuePendingEvent` overwrites a nonempty head with `e->next = 0` when
  inserting before every current entry. The old chain is lost, while its
  kind counts remain. The source comment calls this out at the insertion.
- CLEARAREA turns an off-map position into NULL and immediately reads its
  flags. The null-dereference behavior is reproduced and commented.
- Theme/flag and bridge helpers only check the upper bound; negative
  indices remain unchecked. `SetHappinessFactor` has no bounds check.
- `NewTimedEvent` checks allocation, but its hint-building callers immediately
  dereference a failed result. `ShowGoalHint` also has no upper index bound.
- RIDERS and HAPPINESS test success only after finding a qualifying list
  entry; empty lists with a zero target still take the hint/failure path.

## Names, types, and compiler levers

- Newly named globals are `g_goal_kind_count` (`0x0066872c`, per-kind hint
  counters) and `g_appraisal_minutes` (`0x00832978`, deadline interval).
  Other globals use existing names and address annotations. New callee
  `SetButtonFlash` (`0x00476030`) is a bounds-checked write to one of nine
  integer flash states at `0x007fdd00`; `SetThemeIconEnabled`
  (`0x00476140`) is named for its icon-flag and profile behavior.
- **Caller-side types:** `SetThemeIcon` and `AddLevelFlag` take a signed byte
  as their second parameter here, whereas the existing keyword callers
  declare integers. `GetLevelFlag` returns a signed byte; CHECKFLAG's
  `movsx ecx,al` proves the sign extension. `GetRideVisitCountAt` returns an
  unsigned short: the caller masks eax to 0xffff and its body writes ax.
  The +0xc0 loop-size callback is cdecl `(Elem*, int) -> int`. Goal hint
  helpers are void, and event tick handlers return int. No other file's
  prototypes were changed.
- **One local definition pointer resolves scratch-register allocation.**
  LOOPCOMPOSITE's baseline had two mismatches at indices 20/21 (`edx`
  instead of `eax` for the diagnostic name). Naming the `ObjDef*` before
  the callback test gives 28/28. Naming the element or callback alone, or
  making the name load volatile, did not help. FIXRIDES similarly goes from
  four mismatches (indices 13/14 and 44/46) to 61/61 by naming the definition
  inside the condition arm. This needs no volatile load or extra instruction.
- **Scope of a position aggregate controls store scheduling.**
  RIDEVISITORS initially matched 64/69 strict instructions: local `Pos` in
  the inner arm delayed both stores. Declaring the same `Pos` once at
  function scope interleaves x-store/y-load exactly, 69/69. Volatile x/y
  reads instead cost four bytes and seven mismatches.
- **A free volatile row-pointer read controls the loop invariant choice.**
  CLEARAREA initially hoisted the row table and repeatedly loaded the map
  config (66 strict mismatches, 202B against 197B after extent trimming).
  A single `*(Cell** volatile*)&g_map_rows` at the valid-cell lookup keeps
  the config in edx and reloads the row table in the original arm: 81/81,
  197B. It is confined to this load; the global itself remains ordinary.
  Naming the map pointer did not fix the allocation; positive nested bounds
  checks changed the branch structure and grew the body.
- **Keep the timer result as the accumulation destination.**
  `GetGameTimer() + minutes * 60000` in one expression rotates the time into
  ecx. `deadline = GetGameTimer(); deadline += minutes * 60000;` gives the
  original eax accumulation. Assigning `deadline = 0` in the else arm,
  rather than initializing it before the guard, gives the original separate
  zero-return block: 17/17, 54B.

## Integration

Merge `scope/X` through the integrating session's normal workflow. Scope V
owns `eventgoal.c` and `eventtick.c`; its four goal-primitive extern names
already agree. It should use the named `GoalCheck_Need`,
`GoalCheck_Connect`, and `GoalCheck_Link` externs above for +0x00468d80,
+0x00468dc0, and +0x00468e00. The scope brief remains unchanged because it
is shared; this lane report is the completion record.
