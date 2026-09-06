# Scope Z — low-level bloke AI

Completed 2026-09-06 on `scope/Z`, based on main at `6a95613e`.
Worktree: `.worktrees/scope-z`. The change owns only
`LEGOLAND/lowlevelai.c` and this report.

**28 of 28 exact functions, 1647 instructions,
4231 bytes.** All sixteen dispatch handlers and twelve helpers,
including the unreferenced helper at `0x004841e0`, are reconstructed.
There are no WIPs or residual mismatches.

Validation: `tools/audit.py LEGOLAND/lowlevelai.c` prints 28 `[OK]` lines
and ends PASS; `/W3 /O2 /Gy /Gd` compilation is clean. Relocation identity
checks report 146 matches and zero mismatches. The only unresolved reference
is the diagnostic literal at `0x004bdcd8`, read directly from the executable
and confirmed as `Frame %d.. Bloke %d rethinking\n`.
The complete 16-entry table at `0x004bd34c` was read and cross-checked against
the recovered handler addresses. All 28 assigned addresses occur exactly
once and none is already defined in the main checkout. Global verification,
coverage, and report regeneration remain the integrating session's work.

## Per-function results

Every percentage covers the original's full function extent: equal
instruction count and byte length, zero strict normalized mismatches, no
escaping branch. Every row has marker `FUNCTION` and `audit [OK]`; first
divergence and residual are not applicable. Names are explained below.

| Address | Name | Instructions | Bytes | Match | Audit | Marker |
| --- | --- | ---: | ---: | ---: | --- | --- |
| `0x00483160` | `MapPointInBounds` | 21 | 57 | 100% | [OK] | FUNCTION |
| `0x004831a0` | `HeadingDelta` | 13 | 47 | 100% | [OK] | FUNCTION |
| `0x00483260` | `BeginTileWait` | 54 | 146 | 100% | [OK] | FUNCTION |
| `0x00483300` | `TryTileWait` | 79 | 194 | 100% | [OK] | FUNCTION |
| `0x00483580` | `HitWorkerObstacle` | 74 | 204 | 100% | [OK] | FUNCTION |
| `0x00483680` | `NotifyTileTransition` | 107 | 282 | 100% | [OK] | FUNCTION |
| `0x00483850` | `StepBlokeTurn` | 21 | 51 | 100% | [OK] | FUNCTION |
| `0x00483890` | `SetBlokeWaitingFrame` | 3 | 9 | 100% | [OK] | FUNCTION |
| `0x004838a0` | `LowAI_Unimplemented` | 8 | 26 | 100% | [OK] | FUNCTION |
| `0x004838c0` | `LowAI_Wait` | 10 | 26 | 100% | [OK] | FUNCTION |
| `0x004838e0` | `LowAI_Turn` | 19 | 49 | 100% | [OK] | FUNCTION |
| `0x00483b60` | `ResumeOnPath` | 76 | 182 | 100% | [OK] | FUNCTION |
| `0x00483c20` | `TurnIfBlocked` | 97 | 235 | 100% | [OK] | FUNCTION |
| `0x00483d10` | `LowAI_Wander` | 54 | 123 | 100% | [OK] | FUNCTION |
| `0x00483d90` | `LowAI_WorkerWander` | 60 | 142 | 100% | [OK] | FUNCTION |
| `0x00483e20` | `LowAI_SeekPath` | 88 | 206 | 100% | [OK] | FUNCTION |
| `0x00483ef0` | `LowAI_FollowPath` | 159 | 407 | 100% | [OK] | FUNCTION |
| `0x00484090` | `LowAI_WalkToJunction` | 114 | 271 | 100% | [OK] | FUNCTION |
| `0x004841a0` | `BlokeNearTarget` | 21 | 51 | 100% | [OK] | FUNCTION |
| `0x004841e0` | `MoveLineNearTarget` | 18 | 52 | 100% | [OK] | FUNCTION |
| `0x00484220` | `LowAI_WalkLineOnPath` | 112 | 294 | 100% | [OK] | FUNCTION |
| `0x00484350` | `LowAI_WalkLineToPath` | 108 | 284 | 100% | [OK] | FUNCTION |
| `0x00484470` | `LowAI_WalkLine` | 64 | 172 | 100% | [OK] | FUNCTION |
| `0x00484520` | `LowAI_WorkerWalkLine` | 64 | 172 | 100% | [OK] | FUNCTION |
| `0x004845d0` | `LowAI_WalkLineResume` | 35 | 94 | 100% | [OK] | FUNCTION |
| `0x00484630` | `LowAI_BounceTurn` | 34 | 98 | 100% | [OK] | FUNCTION |
| `0x00484790` | `LowAI_WaitForTile` | 123 | 321 | 100% | [OK] | FUNCTION |
| `0x004848e0` | `LowAI_WaitWhilePointed` | 11 | 36 | 100% | [OK] | FUNCTION |

## State dispatch and recovered behavior

The original `g_lowlevel_ai` symbol name is retained. State names below are
ours, based on the actual transition and movement rules, rather than the
brief's provisional `LowAI_StateN` labels.

| State | Handler | Behavior and transitions |
| ---: | --- | --- |
| 0 | `LowAI_Unimplemented` | Diagnostic stub prints the frame and bloke pointer with the original `%d` format. |
| 1 | `LowAI_Wait` | Decrements the byte delay; at zero resets it to 1 and executes the pending state. |
| 2 | `LowAI_FollowPath` | Steps in the current direction, first handling a blocked dynamic tile. At tile centers marks the action done; leaving walkable/path terrain sets result bit 2, a junction sets bit 4, and an RF-8 continuation chooses the non-reverse exit before moving. |
| 3 | `LowAI_WalkToJunction` | Walks until the tile-center checks request replanning: result bit 2 off a walkable path, bit 4 at a junction, or plain state 0 for RF-8. This handler does not call `TryTileWait`. |
| 4 | `LowAI_Wander` | Tries dynamic-tile waiting, tile-specific path actions, then turning around obstacles or randomly. If none intercepts, moves and advances the walk animation. Every 128 ticks requests a new plan and clears its tick counter. |
| 5 | `LowAI_Turn` | Steps toward the requested direction, then sets delay 1 and executes the pending state upon alignment. |
| 6 | `LowAI_WalkLineOnPath` | Follows the move line toward the target; stops on reaching it, encountering an obstacle (result bit 1), or leaving walkable paths (result bit 2). |
| 7 | `LowAI_WalkLineResume` | Follows the move line without tile-transition callbacks; executes the pending state when close to the target. |
| 8 | `LowAI_BounceTurn` | Restarts vertical motion at zero height, decrements the bounce counter, applies and decreases vertical velocity, and turns toward the next heading while forcing frame 0. |
| 9 | `LowAI_WaitForTile` | Watches the next tile's dynamic flags. Values 1/2 clear waiting and resume the pending state, immediately invoking its low-level handler. Values 0/3 increment the wait count and select frame 2. |
| 10 | `LowAI_SeekPath` | Requests a new plan when already on a walkable path; otherwise wanders, with an additional tile-center path-square check to resume normal path AI. |
| 11 | `LowAI_WalkLine` | Follows a move line and stops at the target or a new-tile obstacle, reporting the latter with result bit 1. |
| 12 | `LowAI_WorkerWalkLine` | State 11 with the worker-specific obstacle test, which exempts map flags 0x8800. |
| 13 | `LowAI_WaitWhilePointed` | Retains the state only while the mouse-hit record has bit 0x200 and points to this bloke; otherwise clears flag 8 and executes the pending state. |
| 14 | `LowAI_WorkerWander` | State 4's wander behavior, delayed until at least 50 frames after the last job timestamp. The elapsed-frame comparison is unsigned. |
| 15 | `LowAI_WalkLineToPath` | Walks toward a path; stops if already on a path, near the target, or a new-tile obstacle/path is encountered. It does not set result bits. |

## Helper names and mechanics

- `MapPointInBounds` (`0x00483160`) tests strict positive world coordinates
  below `width << 8` and `height << 8`. The other inlined checks allow zero;
  those deliberately remain different.
- `HeadingDelta` (`0x004831a0`) takes an unsigned-byte direction and a signed
  short speed, returning two signed ints in eax:edx. Each table component
  is multiplied by speed then arithmetic-shifted right by 8. The new
  `g_heading_delta` name at `0x004bd32c` describes the eight short pairs:
  `(-181,-181), (0,-256), (181,-181), (256,0), (181,181), (0,256),
  (-181,181), (-256,0)`. The next address is the AI dispatch table,
  `0x004bd34c`: the brief's 256-entry claim is incorrect. The function masks
  its argument to a byte but does not check the eight-entry bound.
- `BeginTileWait` (`0x00483260`) sets bloke flag 8, saves the current state
  in pending, enters state 9 and clears its wait counter. It finds the tile
  one step ahead with `GetTileInDir` and invokes its enter callback if set.
- `TryTileWait` (`0x00483300`) only acts when crossing a tile boundary, within
  the strict world bounds, with static RF low bits equal to 3. It checks the
  dynamic RF callback (default 2) and begins waiting only when those low
  bits are also 3.
- `HitWorkerObstacle` (`0x00483580`) reports an out-of-map destination or
  entry into an RF-bit-2 obstruction when the current position is not
  obstructed. A cell with map flag 0x0800 or 0x8000 is not considered an
  obstruction for this test. Both the current and prospective positions
  receive that exception.
- `NotifyTileTransition` (`0x00483680`) handles leaving and entering tiles
  whose static RF low bits are 3, and updates bloke flag 8 accordingly.
  World coordinates are converted with logical shifts for these callbacks,
  whereas `TryTileWait`'s dynamic lookup uses arithmetic shifts.
- `StepBlokeTurn` (`0x00483850`) decrements the delay byte; every third tick
  after it expires, changes direction by +1 or -1 based on bit 4 of the
  difference between current and requested direction, then masks to 0..7.
- `SetBlokeWaitingFrame` (`0x00483890`) writes frame 2 only.
- `ResumeOnPath` (`0x00483b60`) requires a center crossing without leaving
  the tile and without the tile-action-done flag; it tests the current world
  position for walkable path flags and an existing path square. Success
  clears the low-level state to 0 and returns 1.
- `TurnIfBlocked` (`0x00483c20`) uses person kinds 2/3 for workers. Other
  kinds turn at path edges, obstacles, or an off-path random probability
  of 20/1024 (zero random chance while on a path). Workers turn only at path
  edges or worker obstacles. New headings are random 0..7, forced odd for
  people already on paths; `NewDirForAction` sets up the turn.
- `BlokeNearTarget` (`0x004841a0`) compares squared target-to-world distance
  with squared radius, using signed integer arithmetic. The line walkers
  use twice their speed as the arrival radius.
- `MoveLineNearTarget` (`0x004841e0`) compares squared target-to-move-line
  distance with 1024 (radius 32). It is unreferenced in the original binary;
  the complete dead body is nevertheless matched.

## Layout corrections and original edge cases

- **Direction and speed were reversed in the brief.** The actual heading
  byte is +0x72; +0x7f is speed. This is confirmed by the worker generator's
  speed 0x18 and by `HeadingDelta` indexing with its first argument. The
  low-level states read the byte speed into ax to pass a short, then read
  the direction into cl. The world coordinates at +0x68/+0x6c and target
  at +0x24/+0x28 are 24.8 values, and the move line starts at +0x98.
- Flags at +0x62 are a 16-bit field: bit 2 means on a path, bit 4 means the
  tile action fired, and bit 8 tracks dynamic-tile waiting/entry. Result
  flags at +0x64 are a byte. The state and pending fields are unsigned
  shorts at +0x0e/+0x10. The complete local bloke layout retains the
  0xac-byte allocation stride.
- **Wrong callback guard is original.** The two leave-callback sites test
  the enter pointer at +0x1c and then call the leave pointer at +0x20. A
  nonnull enter paired with null leave will call NULL. Both sites are
  preserved and commented; no defensive change is added.
- Inlined tile lookup may yield NULL, but callback users immediately read
  its tile number. The original has no cell-null check at these sites.
- `LowAI_WaitForTile` still queries the dynamic flags after its initial
  static-flags check executes `DoPendingAction`. A later success path can
  execute that action again; the original pending state may already have
  been cleared. The deliberate fall-through is commented in the source.
- The bounce height is unsigned. Its `<= 0` test only catches zero and does
  not clamp wrapped negative motion. The unsigned bounce counter uses a
  post-decrement, including wrapping zero to 65535 before resuming pending
  state. Delay-byte decrements also retain their original wraparound.
- The diagnostic passes the bloke pointer to the literal's `%d`; it does
  not call `GetBlokeNum`. Integer products and shifts retain original
  32-bit behavior rather than introducing widened arithmetic.

## Compiler levers and extern types

- **Copy the world position as one aggregate.** All ten movement handlers
  initially had two strict mismatches because scalar x/y stores let the
  animation-call argument push move before both stores. `b->world = next`
  places the push between the two stores exactly as the original, with no
  change to count or length. This transferred to all ten bodies, including
  the 159-instruction path follower and the shortest line-resume handler.
- **Inline coordinate-helper argument order determines the shift schedule.**
  `CellAt(int y, int x)` called with y-shift then x-shift causes VC6 to
  evaluate x first and keep the original `test` after both shifts. The
  initial x/y helper produced a `js`, swapped coordinate registers, and was
  one instruction short in `BeginTileWait` and `TryTileWait`. Reversing
  parameter order gives 54/54 and 79/79; the transition body drops from
  85 mismatches to 8 at its correct 107 instructions/282 bytes. The helper
  is static inline and has no original-address marker.
- **One by-value position parameter protects its stack slots.**
  `TurnIfBlocked(Bloke*, Pos)` matches all 97 instructions. With two scalar
  coordinates, the random direction byte took the third parameter slot;
  the aggregate keeps it in the dead first parameter slot, resolving six
  offset mismatches. Its three callers also remain exact with `Pos` passed
  by value. No other source file's declaration was changed.
- **Adjacent switch cases produce the original signed range test.**
  `switch (flags & 3)` with cases 1 and 2 sharing the resume block forces
  `LowAI_WaitForTile`'s byte local into the dead parameter slot and uses a
  dword `and eax,3 / jle / cmp eax,2 / jg`. The equivalent pair of `if`
  comparisons instead narrows the mask and branches to al, omits the spill,
  and fails the extent gate. The switch matches 123/123, 321 bytes.
- **Read y before x to choose the surviving x register.** After using scalar
  old-coordinate locals in `NotifyTileTransition`, only six strict
  mismatches remained: old x occupied edx and old y esi. Reading old y
  first, then old x, causes the compiler to emit the original x-first loads
  but keep x in esi, matching 107/107. Free volatile reads, unsigned local
  types, and naming a position pointer did not alter this allocation.
- **Write the comparison in the original operand order.**
  `radius * radius >= delta.x * delta.x + delta.y * delta.y` gives
  `BlokeNearTarget`'s `cmp ecx,edx / setge`; reversing the equivalent source
  comparison emits `cmp edx,ecx / setle`, the two initial mismatches.
- `GetTileInDir` is declared here with `(Pos, unsigned char)` while its
  definition in `pathtile2.c` takes `(Offset, int)`; both coordinate pairs
  are returned in eax:edx. `NavigMoveLine` takes a short speed here to match
  the callers' 16-bit argument preparation. `HeadingDelta` uses
  `(unsigned char, short) -> Pos`; the direction table uses signed shorts.
  RF getters return unsigned char and map flags unsigned short.
- The dispatch array retains `g_lowlevel_ai`. A `LowAIFn` typedef lets the
  relocation parser bind its existing address annotation; a direct
  function-pointer-array declaration compiled identically but appeared as
  unresolved to the parser. All 146 nonliteral relocations now resolve.

## Integration

Only the new `LEGOLAND/lowlevelai.c` and this report should be merged from
`scope/Z`. Existing dispatch consumers in `blokemisc.c` and `sweep3.c`,
shared headers, tools, other scopes, and shared progress documents are
untouched. The integrating session can then run its global verification
and regenerate progress as usual.
