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
| 0x0044ed70 | — | 328 | — | — | not started |
| 0x0044f170 | BlokeAction_SetState4 | 3 | 100 | [OK] | FUNCTION |
| 0x0044f180 | PosOnObjectFootprint | 201 | ~46 | WIP | WIP-FUNCTION |
| 0x0044f3d0 | CountBlokesAtRideID | 15 | 100 | [OK] | FUNCTION |
| 0x0044f400 | SeatListFull | 14 | 100 | [OK] | FUNCTION |
| 0x0044f4a0 | JoinSeatList | 121 | 100 | [OK] | FUNCTION |
| 0x0044f610 | — | 699 | — | — | not started |

**11 / 14 exact.** Three remain: PosOnObjectFootprint (WIP ~46%), plus the
two large jump-table LT handlers 0x0044ed70 (328) and 0x0044f610 (699).

## Mechanics

- **ClearSim832b9c / GetSim832b9c**: zero / read sim counter 0x00832b9c.
- **ClearAppraisalState**: one zero into minutes + instant deadline.
- **AppraisalDueTick**: due-deadline tick → appraisal screen → pass/fail.
- **SetLevelGoalState**: store state, clear sim, seed end-sequence 0.
- **FormatBlokeMessage**: rotate 8×100-byte scratch rows; sprintf + DBPrintf.
- **BlokeAction_SetState4**: LT table 0x01; `state = 4`.
- **BlokeAction_EnterPark**: LT table 0x02; walk to entrance, JoinSeatList,
  hand off to LT 5; case 2 clears flag 8 and starts LT 6.
- **CountBlokesAtRideID / SeatListFull**: seat-list count vs capacity.
- **JoinSeatList**: malloc 20-byte slot, stamp ride_id, `flags |= 4`
  (unguarded null-or bug), PutBlokeInList, dirty 0x20.
- **PosOnObjectFootprint** (WIP): four-neighbour footprint origin test;
  same family as GetObjectUID's UidHit probes.

## Levers

- **AppraisalDueTick**: nested guards → one trailing `return 0`; sim bump
  `if (g > 0) ++; else = 1`.
- **FormatBlokeMessage**: `for (i = 0, rot = g_bloke_msg_rot; i < 8; i++)`
  so rot wins commutative-add destination (`lea [esi+edx]` = 8d0416).
  Signed `jl` end from indexed for over the global slot table.
- **JoinSeatList**: intrinsic `memset(slot, 0, 0x14)`; spill x/y into `xy[]`
  before bounds test; `unsigned short flags` with `cell->flags |= 4` emits
  `or byte ptr [eax+0xc],4`. Fail arms nested after success return.
- **PosOnObjectFootprint**: blocked on g_map reload landing pads between
  probes (same family as GetObjectUID's residual in objmap2.c). Flat probes
  ~46%; nested above/below ~33%.

## Remaining / blocked

- **0x0044f180**: needs GetObjectUID-style `mov esi,[g_map]` landing pads at
  probe entries; do not grind flat vs nested without that lever.
- **0x0044ed70** (328): jump-table on `action` 0..0xd; calls SuggestNextMove,
  CalcMoveLine, NewDirForAction, FormatBlokeMessage. Leave until f180 lands
  or take as its own pass.
- **0x0044f610** (699): jump-table on `action` 0..0xa; publishes
  `g_cur_bloke_f81`, calls BuildObjInfoList / ride-code helpers /
  FormatBlokeMessage / PosOnObjectFootprint. Largest; last by design.

## Names

- `GetSim832b9c`, `AppraisalDueTick`, `FormatBlokeMessage`,
  `BlokeAction_SetState4`, `BlokeAction_EnterPark`, `CountBlokesAtRideID`,
  `SeatListFull`, `JoinSeatList`, `PosOnObjectFootprint`,
  `BumpSlotCounter` (0x00489f90), `RunAppraisalScreen`.
