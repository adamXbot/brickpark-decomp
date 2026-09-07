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
| 0x0044ebf0 | — | 92 | — | — | pending |
| 0x0044ed00 | FormatBlokeMessage | 32 | 100 | [OK] | FUNCTION |
| 0x0044ed70 | — | 328 | — | — | pending |
| 0x0044f170 | BlokeAction_SetState4 | 3 | 100 | [OK] | FUNCTION |
| 0x0044f180 | — | 201 | — | — | pending |
| 0x0044f3d0 | CountBlokesAtRideID | 15 | 100 | [OK] | FUNCTION |
| 0x0044f400 | SeatListFull | 14 | 100 | [OK] | FUNCTION |
| 0x0044f4a0 | JoinSeatList | 121 | 100 | [OK] | FUNCTION |
| 0x0044f610 | — | 699 | — | — | pending |

## Mechanics

- **ClearSim832b9c / GetSim832b9c**: zero / read the signed sim counter at
  0x00832b9c.
- **ClearAppraisalState**: one zero into `g_appraisal_minutes` and
  `g_instant_appraisal`.
- **AppraisalDueTick**: due-deadline tick; pause, run appraisal screen, pass
  or fail toward `EndLevel(2)`.
- **SetLevelGoalState**: store state, clear sim, seed end-sequence 0.
- **FormatBlokeMessage**: rotate eight 100-byte scratch rows, sprintf + DBPrintf.
- **BlokeAction_SetState4**: LT action 0x01; `state = 4`.
- **CountBlokesAtRideID / SeatListFull**: seat-list count vs capacity.
- **JoinSeatList**: malloc 20-byte seat slot, bind bloke, stamp ride_id
  (entrance cell bytes or GetObjectUID), `cell->flags |= 4` (unguarded;
  original null-or bug), PutBlokeInList, dirty bit 0x20.

## Levers

- **AppraisalDueTick**: nested guards → one trailing `return 0`; sim bump as
  `if (g > 0) ++; else = 1`.
- **FormatBlokeMessage**: `for (i = 0, rot = g_bloke_msg_rot; i < 8; i++)`
  so rot wins the commutative-add destination (`lea [esi+edx]` = 8d0416);
  a prior `int rot = g;` makes i win (8d0432). Signed `jl` end from the
  indexed for over the global slot table.
- **JoinSeatList**: intrinsic `memset(slot, 0, 0x14)` for the five-dword
  clear; spill x/y back into `xy[]` before the bounds test; `unsigned short
  flags` with `cell->flags |= 4` emits the original `or byte ptr [eax+0xc],4`.
  Fail paths nested under `if (slot) { if (cell) {…} fail_inst } fail_alloc`
  so both DBPrintf arms sit after the success return.

## Names

- `GetSim832b9c`, `AppraisalDueTick`, `FormatBlokeMessage`,
  `BlokeAction_SetState4`, `CountBlokesAtRideID`, `SeatListFull`,
  `JoinSeatList`, `BumpSlotCounter` (0x00489f90), `RunAppraisalScreen`.
