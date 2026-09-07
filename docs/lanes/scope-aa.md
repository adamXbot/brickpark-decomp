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
| 0x0044f4a0 | JoinSeatList | 121 | — | — | pending |
| 0x0044f610 | — | 699 | — | — | pending |

## Mechanics

- **ClearSim832b9c / GetSim832b9c**: zero / read the signed sim counter at
  0x00832b9c (movie3's name).
- **ClearAppraisalState**: one zero register stores into both
  `g_appraisal_minutes` (0x00832978) and `g_instant_appraisal` (0x00666098).
- **AppraisalDueTick** (gameframe's appraisal-due tick): if the script is
  idle and `g_instant_appraisal` is due, pause timer/samples/track, set the
  hold-off counter `g_6687b0 = 4`, run the appraisal screen (0x004453a0),
  then either pass (bump sim ≥1, `StopScript(1)`, set `g_level_cfg->f30`,
  `sub_48a750`) or fail (drive sim more negative; when
  `sim <= -g_level_goal_state` call `EndLevel(2)`). Clears the deadline,
  resets it via `ResetAppraisalDeadline`, thaws clock and resumes samples.
  Returns 1 when the screen ran, else 0.
- **SetLevelGoalState**: stores state at 0x0083297c, clears the sim counter,
  seeds end-sequence slot 0 via `SetLevelEndSequence(0, text)`.
- **FormatBlokeMessage**: rotates eight 100-byte scratch rows at 0x006661cc
  into the pointer table at 0x004b8348, sprintf `%c:%s` with `g_cur_bloke_f81`,
  DBPrintf `[Bloke %c] - %s\n`.
- **BlokeAction_SetState4**: long-term action table slot 0x01; writes
  `bloke->state = 4`.
- **CountBlokesAtRideID / SeatListFull**: walk the +0xcc seat list counting
  matching `seat` words; full when count ≥ `rider_capacity` (+0x2e).

## Levers

- **AppraisalDueTick early outs**: three guards must nest so VC6 jump-threads
  them to ONE trailing `return 0` (`jne` / `je` / `jg` to the same epilogue).
  Early `return 0` statements duplicated the epilogue and inverted the
  compares (62% → nested form).
- **Sim bump branch direction**: `if (g_sim_832b9c > 0) ++; else = 1;` keeps
  the increment as fall-through and `jle` to the `= 1` arm. Spelling
  `<= 0` first inverted to `jg` (93% residual).
- **Fail-path sim**: `if (sim < 0) sim--; else sim = -1;` then stash
  `goal = g_level_goal_state` before the store so eax holds the goal across
  the `neg`/`cmp` against ecx.
- **FormatBlokeMessage signed end + lea base**: `for (i = 0; i < 8; i++)`
  over the global slot table yields strength-reduced `cmp cursor,end / jl`.
  The `(rot + i)` lea must be `8d0416` (`[esi+edx]`). A prior
  `int rot = g_bloke_msg_rot;` makes `i` the later-defined operand and emits
  `8d0432` (96.9%). Fix: initialize rot in the for-init
  (`for (i = 0, rot = g_bloke_msg_rot; i < 8; i++)`) so rot wins the
  commutative-add destination, or read `g_bloke_msg_rot` as a memory operand
  inside the sum.

## Names

- `GetSim832b9c` for the dead getter at 0x0044db30 (pair of ClearSim832b9c).
- `AppraisalDueTick` renames gameframe's `sub_44db90`.
- `RunAppraisalScreen` for the unassignable 0x004453a0 callee.
- `FormatBlokeMessage` for 0x0044ed00 (sprintf + DBPrintf helper).
- `BlokeAction_SetState4` for table slot 0x01 at 0x0044f170.
- `CountBlokesAtRideID` / `SeatListFull` from rides.c's existing names /
  capacity compare.
- `g_sim_832b9c` / `g_level_goal_state` / `g_appraisal_result` for the three
  globals this tier owns the writes of.
