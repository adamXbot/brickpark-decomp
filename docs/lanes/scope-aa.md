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
| 0x0044ed70 | BlokeAction_LeavePark | 328 | 100 | [OK] | FUNCTION |
| 0x0044f170 | BlokeAction_SetState4 | 3 | 100 | [OK] | FUNCTION |
| 0x0044f180 | PosOnObjectFootprint | 201 | 100 | [OK] | FUNCTION |
| 0x0044f3d0 | CountBlokesAtRideID | 15 | 100 | [OK] | FUNCTION |
| 0x0044f400 | SeatListFull | 14 | 100 | [OK] | FUNCTION |
| 0x0044f4a0 | JoinSeatList | 121 | 100 | [OK] | FUNCTION |
| 0x0044f610 | BlokeAction_PickRide | 699 | 100 | [OK] | FUNCTION |

**14 / 14 exact.** LeavePark closed (328/328) and PickRide closed (699/699,
1977 B, `/W3` clean). Both closes are documented under Remaining below.

## Mechanics

- **ClearSim832b9c / GetSim832b9c**: zero / read sim counter 0x00832b9c.
- **ClearAppraisalState**: one zero into minutes + instant deadline.
- **AppraisalDueTick**: due-deadline tick → appraisal screen → pass/fail.
- **SetLevelGoalState**: store state, clear sim, seed end-sequence 0.
- **FormatBlokeMessage**: rotate 8×100-byte scratch rows; sprintf + DBPrintf.
- **BlokeAction_SetState4**: LT table 0x01; `state = 4`.
- **BlokeAction_EnterPark**: LT table 0x02; walk to entrance, JoinSeatList,
  hand off to LT 5; case 2 clears flag 8 and starts LT 6.
- **BlokeAction_LeavePark**: LT table 0x03; SuggestNextMove /
  PTPSuggestNextMove toward `g_entrance_x`, wander / exit seat list /
  RateBlokeOnLeaving / DestroyBloke.
- **CountBlokesAtRideID / SeatListFull**: seat-list count vs capacity.
- **JoinSeatList**: malloc 20-byte slot, stamp ride_id, `flags |= 4`
  (unguarded null-or bug), PutBlokeInList, dirty 0x20.
- **PosOnObjectFootprint**: four flat neighbour footprint probes; coords as
  `pos->x>>8` expressions; `FootCell(y,x)` order; Pos-local hit sums.
- **BlokeAction_PickRide**: LT table 0x06; JT on action 0..0xa at
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
  retarget stores. f64 action masks: plain ternaries
  `f64 ? 0 : 0xa`, `f64 == 0`, `(f64 & 1) ? 6 : 0xa` (byte-wide `and dl`).
- **PickRide exits / default**: the four `action = 2` exits (case 0 no keys,
  case 10 not-on-footprint, not-working, fall-out) are `break;` into one
  post-switch `b->action = 2;`. The `default:` arm is
  `if (b->action >= 0) goto done;` with `done: ;` after the store — an
  always-true test the front end does NOT fold (see Remaining §PickRide 4).
- **PickRide case 10**: uid bytes as `more` / `y` int locals with the map-cell
  bounds test repeated at each of the three GetInstanceOfClass sites; the
  flag-set site is `InstFlags* p = GetInstanceOfClass(...); p->flags |= 2;`
  (direct `->flags |= 2` on the call widens/reorders). `cant_get_on:` is a
  shared label reached by `goto` from the flags test and the JoinSeatList
  fail; it ends `action = 2; return;` (NOT `break`) so its store stays inline.

## Remaining / blocked

### 0x0044ed70 BlokeAction_LeavePark — CLOSED 328/328 (audit [OK])

The three residuals of the previous pass, and what closed each:

1. **Stuck counter (+0x82) in `dl`** — an `unsigned char lim = 5` local used
   as the inner switch bound (`if ((unsigned)r > lim) return; switch (r)`)
   and as the `action = lim` store value keeps 5 live in `ecx` across the
   jump table, so the `mov [action],cl` stores appear and `++b->stuck == 8`
   lowers to `mov dl,[stuck] / inc dl / mov al,dl / mov [stuck],dl / cmp al,8`.
2. **PTP action mask** — the plain ternary `(f64 & 1) ? 6 : 0xa` (not
   `0xa + (… ? -4 : 0)`) is what emits `and dl,1 / neg dl / sbb dl,dl /
   and dl,0xfc / add dl,0xa` byte-wide. The additive spelling widened the
   `and` to `edx`.
3. **Case 11/12 shared CalcMoveLine tail, layout-LAST host** — the two arms
   must end in `break;`, not `return;`. With `return` VC6 gives each arm an
   inline epilogue copy first, the IR tails are then no longer identical
   suffixes of a common `jmp exit`, and only the post-codegen cross-jump
   fires (into the EARLIER block: case 12 `jmp` back to case 11's call).
   With `break` both arms end in the same `jmp` to the switch exit, the
   IR-level suffix merge hosts the tail in the layout-last arm (case 12),
   case 11 jumps forward into the call, the epilogue is expanded once in the
   survivor, and the deferred `add esp,0x20` (CalcMoveLine 0x14 + NewDir 8 +
   pending 4 from GetFirstObjectMatching / RateBlokeOnLeaving) survives.
   `goto` in either direction flushes the pending 4 (`add esp,4` +
   `add esp,0x1c`). Textually swapping cases 12/11 moves the layout too.
   **Lever: `break` vs `return` in a void switch arm is a tail-merge phase
   selector**, even though both reach the same exit block.

### 0x0044f610 BlokeAction_PickRide — CLOSED 699/699 (audit [OK])

What closed each of the previous residuals, plus the new one that surfaced
at 97.9%:

1. **f64 and-width** — the plain value ternaries (`f64 ? 0 : 0xa`,
   `f64 == 0`, `(f64 & 1) ? 6 : 0xa`) give `neg/sbb/and dl` byte-wide; the
   additive `0xa + (a ? 0xf6 : 0)` spelling is what widened to `edx`.
2. **Case 10 mood tails** — the seat-full and not-working arms are written
   as two full copies (sprintf / Format / AdjustMood / Counter / Increment)
   and both end in `break;` to the shared post-switch `action = 2` store.
   VC6 cross-jumps the identical Counter/Increment suffixes itself; a hand
   `mood_tail` label was what hoisted `mov ebx,2`.
3. **Map-cell join** — `more`/`y` int locals loaded from the uid bytes and
   the `&g_map_rows[y][more]` bounds test spelled out at each of the three
   GetInstanceOfClass sites (no helper, no `fr.next` reuse). The flag-set
   must go through a pointer local (`p->flags |= 2`).
4. **`ja` default target and the post-store edge (the last 12 insns)** —
   with the four `action = 2` exits as `break;` into one post-switch store J,
   a plain `default: return;` (or none) makes VC6's *early* jump-threading
   pass rewrite the `ja` to the epilogue AND leave J → epilogue as a live
   edge, so J is laid out as a separate block and the epilogue is
   duplicated (725 insns, `ja 0x7ca`). The original has `ja` → the one
   shared epilogue at 0x551 with J falling into it. What reproduces it: a
   default arm that is non-trivial when early threading runs but folds to
   nothing later, e.g. `default: if (b->action >= 0) goto done;` with
   `done: ;` after the store. The label must be referenced (`/W3` C4102) and
   `done:` must hold `;`, not `return;` (a `jmp exit` there is threaded early
   again, 96.3%). `default: if (1) return;` / `while (1) return;` /
   `for (; 1;) return;` each also give 699, but only with an unreferenced
   `done: ;` after the store (C4102 at `/W3`); without `done:` they are 725.
   Forms that DON'T work at all: `b->action = b->action`, `&= 0xff`, `|= 0`,
   `+= 0` self-stores (deleted in the front end, 725); `default: goto done;`
   with the test AT `done:` (725); `default: return;` / no default (725);
   `return` in the four exits instead of `break` (four inline epilogues);
   `goto set2` to a labelled post-switch store (725).
   **Lever: an always-true `unsigned char >= 0` test survives the early
   threading/fold pass and is removed late; use it to keep a switch default
   as a real block so `ja` binds to the shared epilogue.**

## Names

- `GetSim832b9c`, `AppraisalDueTick`, `FormatBlokeMessage`,
  `BlokeAction_SetState4`, `BlokeAction_EnterPark`, `BlokeAction_LeavePark`,
  `BlokeAction_PickRide`, `CountBlokesAtRideID`, `SeatListFull`,
  `JoinSeatList`, `PosOnObjectFootprint`, `BumpSlotCounter` (0x00489f90),
  `RunAppraisalScreen`.
