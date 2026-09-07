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
| 0x0044ed00 | — | 32 | — | — | pending |
| 0x0044ed70 | — | 328 | — | — | pending |
| 0x0044f170 | — | 3 | — | — | pending |
| 0x0044f180 | — | 201 | — | — | pending |
| 0x0044f3d0 | — | 15 | — | — | pending |
| 0x0044f400 | — | 14 | — | — | pending |
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

## Names

- `GetSim832b9c` for the dead getter at 0x0044db30 (pair of ClearSim832b9c).
- `AppraisalDueTick` renames gameframe's `sub_44db90`.
- `RunAppraisalScreen` for the unassignable 0x004453a0 callee.
- `g_sim_832b9c` / `g_level_goal_state` / `g_appraisal_result` for the three
  globals this tier owns the writes of.
