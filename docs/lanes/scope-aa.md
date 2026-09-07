# Scope AA — report / goal-state table tier

Branch `scope/AA`. File `LEGOLAND/goalstate.c`. Object prefix `/tmp/saa_`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0044db20 | ClearSim832b9c | 2 | 100 | [OK] | FUNCTION |
| 0x0044db30 | GetSim832b9c | 2 | 100 | [OK] | FUNCTION |
| 0x0044db80 | ClearAppraisalState | 4 | 100 | [OK] | FUNCTION |
| 0x0044db90 | AppraisalDueTick | 60 | 100 | [OK] | FUNCTION |
| 0x0044dc70 | SetLevelGoalState | 9 | 100 | [OK] | FUNCTION |
| 0x0044ebf0 | BlokeAction_EnterPark | 92 | 100 | [OK] | FUNCTION |
| 0x0044ed00 | FormatBlokeMessage | 32 | 100 | [OK] | FUNCTION |
| 0x0044ed70 | BlokeAction_LeavePark | 328 | ~89 | REJECT | WIP-FUNCTION |
| 0x0044f170 | BlokeAction_SetState4 | 3 | 100 | [OK] | FUNCTION |
| 0x0044f180 | PosOnObjectFootprint | 201 | 100 | [OK] | FUNCTION |
| 0x0044f3d0 | CountBlokesAtRideID | 15 | 100 | [OK] | FUNCTION |
| 0x0044f400 | SeatListFull | 14 | 100 | [OK] | FUNCTION |
| 0x0044f4a0 | JoinSeatList | 121 | 100 | [OK] | FUNCTION |
| 0x0044f610 | BlokeAction_PickRide | 699 | ~70 | REJECT | WIP-FUNCTION |

**12 / 14 exact.** LeavePark ~89% WIP; PickRide ~70% WIP (honest residuals
below). Both large LT handlers remain WIP-FUNCTION so audit still PASSes.

## Mechanics

- **ClearSim832b9c / GetSim832b9c**: zero / read sim counter 0x00832b9c.
- **ClearAppraisalState**: one zero into minutes + instant deadline.
- **AppraisalDueTick**: due-deadline tick → appraisal screen → pass/fail.
- **SetLevelGoalState**: store state, clear sim, seed end-sequence 0.
- **FormatBlokeMessage**: rotate 8×100-byte scratch rows; sprintf + DBPrintf.
- **BlokeAction_SetState4**: LT table 0x01; `state = 4`.
- **BlokeAction_EnterPark**: LT table 0x02; walk to entrance, JoinSeatList,
  hand off to LT 5; case 2 clears flag 8 and starts LT 6.
- **BlokeAction_LeavePark** (WIP): LT table 0x03; SuggestNextMove /
  PTPSuggestNextMove toward `g_entrance_x`, wander / exit seat list /
  RateBlokeOnLeaving / DestroyBloke.
- **CountBlokesAtRideID / SeatListFull**: seat-list count vs capacity.
- **JoinSeatList**: malloc 20-byte slot, stamp ride_id, `flags |= 4`
  (unguarded null-or bug), PutBlokeInList, dirty 0x20.
- **PosOnObjectFootprint**: four flat neighbour footprint probes; coords as
  `pos->x>>8` expressions; `FootCell(y,x)` order; Pos-local hit sums.
- **BlokeAction_PickRide** (WIP): LT table 0x06; JT on action 0..0xa at
  `0x44fdcc`; sets `g_cur_bloke_f81` from `b->name_letter`; shuffle/attract
  pick, SuggestNextMove walk, PTP stuck, PosOnObjectFootprint / seat join.

## Levers

- **AppraisalDueTick**: nested guards → one trailing `return 0`; sim bump
  `if (g > 0) ++; else = 1`.
- **FormatBlokeMessage**: `for (i = 0, rot = g_bloke_msg_rot; i < 8; i++)`
  so rot wins commutative-add destination (`lea [esi+edx]` = 8d0416).
  Signed `jl` end from indexed for over the global slot table.
- **JoinSeatList**: intrinsic `memset(slot, 0, 0x14)`; spill x/y into `xy[]`
  before bounds test; `unsigned short flags` with `cell->flags |= 4` emits
  `or byte ptr [eax+0xc],4`. Fail arms nested after success return.
- **PosOnObjectFootprint**: flat four probes (not GetObjectUID nested);
  `FootCell(int y, int x)` for x-first `sar`; FootHit via Pos local so both
  sums before either `cmp`; no int x/y locals — use `pos->x>>8` at each use.
- **LeavePark CalcMoveLine arms**: mirror `Garderner_Repair` —
  `target.x/y = out; CalcMoveLine(b->world, out, path)` keeps `edi` as
  `&b->world` from SuggestNextMove and interleaves the target.y store into
  the by-value pushes. PTP case order `2,1,0` for sub/dec/dec dispatch.
  Duplicate case 11/12 tails so deferred `add esp,0x20` survives (pending
  GetFirstObjectMatching / RateBlokeOnLeaving both +4).
- **PickRide frame / regs**: sole local `fr` of 0x78; `Pos* dest` + `int more`
  stay in ebx/edi; `push ebp` from case-1 def live range. Case 0: shared
  `set_action_2` gives `je` into common epilogue; `more=1` then
  `more=Shuffle…`; high-attract
  `FormatBlokeMessage((b->current_item = elem, fr.msg))` interleaves the
  store between Format's push and call; name-before-`jg` comes from the
  `>10` / not-worth split after one attractiveness call. Case 1 arrive:
  `PosOn==0` → `goto state4` for `je`; cell x/y as int locals then reuse in
  retarget stores. f64 action mask:
  `a = (a ? 0xf6 : 0); action = 0xa + a` (and-width residual below).

## Remaining / blocked

### 0x0044ed70 BlokeAction_LeavePark (~89%, §6B)

Same insn count (328) as original; three structural residuals after the
bigsim-style CalcMoveLine rewrite:

1. **Stuck counter (+0x82)** — original `mov dl,[stuck] / inc dl / mov al,dl /
   store / cmp al,8`. Ours uses `al` throughout and cmps before the store.
   Likely wants the JT index kept live in eax so stuck prefers `dl`. Keep-live
   `else if (r > 5)` and pointer-through-stuck both failed / regressed.
2. **PTP action `(f64&1) ? 6 : 0xa`** — ternary
   `0xa + ((f64&1) ? -4 : 0)` emits `neg dl / sbb dl,dl` correctly but then
   `and edx,0xfc` instead of `and dl,0xfc`. Forms that get `and dl,0xfc` drop
   the `sbb`. No spelling found that yields both.
3. **Case 11/12 shared CalcMoveLine** — duplicate tails give post-codegen
   cross-jump into the *earlier* arm (case 12 `jmp` back to case 11's call).
   Original is layout-last: case 11 `jmp` forward into the call after case
   12's prefix. Source `goto` shared tail gets the forward layout but flushes
   cdecl cleanup (`add esp,4` per call + `add esp,0x1c` instead of one
   `add esp,0x20`). Empty-else direction flip is an if/else lever; not yet
   transferred onto this jump-table pair.

Stop grinding without a new lever for (1)–(3). WIP body retained for the next
pass.

### 0x0044f610 BlokeAction_PickRide (~70%, §6B)

Cases 0–6 match through the walk/PTP arms (~430/699) aside from two and-width
residuals. Case 10 matches through SeatListFull fail setup then diverges on
counter/mood tail layout and instance/map-cell join arms.

1. **f64 masks `and edx,0xf6` / `and edx,0xfc` vs `and dl`** — same family as
   LeavePark residual (2). Ternary `a ? 0xf6 : 0` / `(f64&1)?-4:0` get
   `neg/sbb` but widen the `and`. No spelling yields both `sbb dl,dl` and
   `and dl`.
2. **Case 10 IncrementBlokeCounter / not-working share** — original seat-full
   inlines GetBlokeNum→Counter→Increment then falls into `action=2`;
   not-working `push 1 / jmp` into the seat-full AdjustMood arg setup.
   Separate C copies emit an extra `jmp` and misalign from the first
   Increment. Shared `mood_tail` with `more` as the event regenerates the
   early `mov ebx,2` hoist when `more=2` is also used for the flags test.
3. **Case 10 join / GetInstanceOfClass map-cell** — original inlines uid-byte
   map lookup three times (flags test, flag-set, rand-join) with `mov ebx,2`
   only on the flags path so cant_get_on can `mov [action],bl`. Still open.

Stop grinding (2)/(3) without a lever that shares the mood tail without
hoisting `ebx=2` into the prologue. WIP body retained.

## Names

- `GetSim832b9c`, `AppraisalDueTick`, `FormatBlokeMessage`,
  `BlokeAction_SetState4`, `BlokeAction_EnterPark`, `BlokeAction_LeavePark`,
  `BlokeAction_PickRide`, `CountBlokesAtRideID`, `SeatListFull`,
  `JoinSeatList`, `PosOnObjectFootprint`, `BumpSlotCounter` (0x00489f90),
  `RunAppraisalScreen`.
