# codex-c — small frontier helpers

Branch `codex/scope-c`, isolated worktree based on main `17bdbe3a`.
The scope tables enumerate 88 targets (the headline says approximately 95).
Work proceeds in the requested order: screencb7, lfmisc, simcore2, pathmisc,
sysstubs. Only assigned new C files and this report are committed.

## screencb7.c — complete

All seven bodies pass `audit.py` [OK], `matchfull.py` 100%, and a clean
`/W3 /O2 /Gy /Gd` compile. A scratch COFF relocation check additionally
verifies actual address addends, literal branch displacements and constants.
All committed markers are `// FUNCTION: LEGOLAND <address>`; there is no
first divergence or residual in any row.

| Address | Name and naming evidence | Instructions | Match | Audit [OK] | Marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00436190 | `JungleCruiseWater_Create` — JUNGLE CRUISE WATER +a4 | 4 | 100% | Yes | FUNCTION |
| 0x00433cd0 | `JungleCruiseMonkeyTree_Destroy` — MONKEY TREE +ac | 5 | 100% | Yes | FUNCTION |
| 0x004340b0 | `JungleCruiseMonkeyFish_Destroy` — MONKEY FISH +ac | 5 | 100% | Yes | FUNCTION |
| 0x00433fa0 | `JungleCruiseMonkeyTree_DrawSelection` — MONKEY TREE +94 | 7 | 100% | Yes | FUNCTION |
| 0x00434650 | `JungleCruiseMonkeyFish_DrawSelection` — MONKEY FISH +94 | 7 | 100% | Yes | FUNCTION |
| 0x00452ab0 | `PowerStation_AC` — retained named shared destructor | 9 | 100% | Yes | FUNCTION |
| 0x00452b70 | `Dino_InitSound` — retained named installation helper | 11 | 100% | Yes | FUNCTION |

- Names follow `screen.c`'s `SetCustomCallbacks` class arms and exact slot
  stores. The five `CB_*` names map one-to-one by address to this table;
  existing declarations were left untouched.
- Water creation caches `elem->data` at 0x0081cb54. Monkey destruction drops
  the sprite references at 0x0081cb68/6c; both +94 callbacks forward to
  `BasicObjectDCalcCursor` with no extra behavior.
- Sound resources are reference-counted globally: power-station destruction
  decrements 0x00667118 and releases two FX only on reaching zero; dinosaur
  installation postincrements 0x0066711c and loads five FX only from zero.
  No underflow guards or pointer clearing were added.
- All seven close on their first compile. Prefix/postfix decrement/increment
  reproduce the observed old-value tests directly. No new codegen lever,
  unexplained behavior, or newly established gameplay bug in this file.
- `screen.c` declares callbacks with empty parameter lists. Definitions here
  recover the actual forwarded arguments, preserving the cdecl ABI;
  caller declarations were not changed.


## lfmisc.c — complete

All 16 bodies pass the authoritative audit, 100% matchfull, warning-free
compile and relocated-byte verification (17 address relocations). All have
FUNCTION markers, with no first divergence or residual.

| Address | Name | Instructions | Match | Audit [OK] | Marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00412290 | `FreeWalkPath` | 7 | 100% | Yes | FUNCTION |
| 0x004122d0 | `WalkPath_Board` | 9 | 100% | Yes | FUNCTION |
| 0x00412470 | `LFAnim_FromId` | 11 | 100% | Yes | FUNCTION |
| 0x0042d540 | `NthRiderNode` | 11 | 100% | Yes | FUNCTION |
| 0x004123a0 | `LFAnim_SaveId` | 12 | 100% | Yes | FUNCTION |
| 0x00411bd0 | `Pump_FreeAll` | 13 | 100% | Yes | FUNCTION |
| 0x0043f870 | `Free3DPerson` | 13 | 100% | Yes | FUNCTION |
| 0x004333b0 | `JcBoat_Depart` | 14 | 100% | Yes | FUNCTION |
| 0x00411290 | `LFQuadTopLeft` | 15 | 100% | Yes | FUNCTION |
| 0x004117e0 | `LFBoat_DropStep` | 15 | 100% | Yes | FUNCTION |
| 0x00411650 | `LFBoat_IsOnDrop` | 15 | 100% | Yes | FUNCTION |
| 0x00411e30 | `LFQueue_Append` | 16 | 100% | Yes | FUNCTION |
| 0x00411220 | `LFQuadTopRight` | 17 | 100% | Yes | FUNCTION |
| 0x0040ca30 | `LFDrawBoatList` | 21 | 100% | Yes | FUNCTION |
| 0x00482c60 | `InitBlokeName` | 22 | 100% | Yes | FUNCTION |
| 0x00418f90 | `BsBoat_Destroy` | 27 | 100% | Yes | FUNCTION |

- Existing descriptive scope names are retained except `sub_411650`, now
  `LFBoat_IsOnDrop`: its comparison is with the LOG FLUME DROP class at
  0x004c8d6c. `BsBoat_Destroy` also appears as `BsBoat_Unlink` in existing
  caller declarations; only this single implementation is added.
- Ordinal lookups deliberately have no chain-end check. Saving an absent
  animation returns the chain length. `LFBoat_DropStep` requires a valid
  parent but returns -1 when its piece is absent from the sub-route.
- `LFQueue_Append` treats a queue as empty only when both head and tail are
  null, and does not clear the inserted next link. `BsBoat_Destroy` retains
  the empty-list null dereference; a missing node in a nonempty list returns.
- Walk-path boarding sets the pointer, resets the index, starts walking and
  increments the action byte. Name indices use unsigned remainder with the
  original 90/83 first-name groups and 107 surnames. Boat departure requests
  state 4 and targets current y + 5; quad corners preserve signed shifts.
- Measured levers: a separate `cursor` local closes both ordinal lookups
  from 13i/29B (9 mismatches) to 11i/22B exact; signed versus unsigned index
  was inert. A free volatile read of `boat->piece` closes the drop test's
  four register mismatches without changing its 15i/40B body; merely splitting
  the two pointer reads into statements was inert.
- Local layouts recover real parameter offsets. `Pos` quad results use the
  original eax:edx aggregate return; heap destruction and caller prototypes
  retain cdecl. Existing callers and headers are unchanged.


## simcore2.c — complete

All ten functions pass audit, matchfull 100%, warning-free compilation and
relocated-byte verification (21 address relocations). Every row is FUNCTION;
no divergence or residual remains. Scope names are retained: existing visitor,
mood and route-search callers establish their roles.

| Address | Name | Instructions | Match | Audit [OK] | Marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x004776c0 | `AddClosedNode` | 5 | 100% | Yes | FUNCTION |
| 0x00477980 | `RouteTurnCost` | 8 | 100% | Yes | FUNCTION |
| 0x004700c0 | `IsWatchedBloke` | 10 | 100% | Yes | FUNCTION |
| 0x00482d30 | `GetBlokeMood` | 16 | 100% | Yes | FUNCTION |
| 0x0044eae0 | `UpdateBlokeStay` | 17 | 100% | Yes | FUNCTION |
| 0x004779a0 | `RouteStepAxis` | 18 | 100% | Yes | FUNCTION |
| 0x00477790 | `RemoveOpenNode` | 19 | 100% | Yes | FUNCTION |
| 0x00477680 | `RouteInBounds` | 25 | 100% | Yes | FUNCTION |
| 0x00482df0 | `AdjustMood` | 27 | 100% | Yes | FUNCTION |
| 0x0044ea50 | `SpawnVisitor` | 28 | 100% | Yes | FUNCTION |

- Route nodes use a shared intrusive link. `RemoveOpenNode` unconditionally
  reads the found node's next pointer: absent targets and empty lists retain
  the original null dereference. Bounds checks use unsigned 16-bit map
  dimensions promoted into signed comparisons. Axis cost is 0 when the
  bitmasks overlap, 4 otherwise; step classification distinguishes x/y/diagonal.
- Mood uses signed shorts and table-scaled signed division by 100, clamped
  to [-30000, 30000]; event indices are unchecked. Classification thresholds
  yield 3, 10 or 2. Stay updates snapshot flags before incrementing the timer,
  and request long-term action 3 only when flag 8 is clear and time is up.
- Visitor spawning waits at least 30 ticks and checks the current limit.
  Capacity/allocation failures keep accumulating time. Success resets time,
  draws independent 0..7 timing phases, sets walk delay 1, then initializes AI.
- Nine bodies closed on first compile. `RouteStepAxis` needed a free volatile
  read of x1: 17i/42B becomes the original 18i/44B, retaining an independent
  subtraction/test and its scratch-register allocation. Reusing parameter
  variables was inert. All actual relocation bytes also agree.
- Existing call-site prototypes remain untouched. `AdjustMood` returns the
  sign-extended stored short, although some callers discard its result.
  The retired `RequestRoute` notes were read, including alias-kill and
  live-range findings; its body is outside this lane and unchanged.


## pathmisc.c — complete

All thirteen pass audit, matchfull 100%, a clean warning compile and actual
relocated-byte comparison (28 address relocations). All markers are FUNCTION;
no first divergence or residual remains. Descriptive names from the scope
and existing path/road/work-order callers are retained.

| Address | Name | Instructions | Match | Audit [OK] | Marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x004821c0 | `ClearPTPVisited` | 7 | 100% | Yes | FUNCTION |
| 0x0045e690 | `ObjHasExit` | 11 | 100% | Yes | FUNCTION |
| 0x0045ce10 | `IsPathCell` | 12 | 100% | Yes | FUNCTION |
| 0x004139c0 | `Road_TileClaim` | 12 | 100% | Yes | FUNCTION |
| 0x00482300 | `AddPTPRouteNode` | 13 | 100% | Yes | FUNCTION |
| 0x00450c00 | `FreeBuildSlotAt` | 16 | 100% | Yes | FUNCTION |
| 0x004821e0 | `FreePTPOpenList` | 16 | 100% | Yes | FUNCTION |
| 0x00413990 | `Road_TileCost` | 18 | 100% | Yes | FUNCTION |
| 0x00489f00 | `MarkObjectTiles` | 18 | 100% | Yes | FUNCTION |
| 0x0045d730 | `AddObjRectSpan` | 21 | 100% | Yes | FUNCTION |
| 0x0045cd00 | `GrowPathRect` | 23 | 100% | Yes | FUNCTION |
| 0x0045eaf0 | `ClassNeedsPath` | 25 | 100% | Yes | FUNCTION |
| 0x00499720 | `SettleOrderSpan` | 28 | 100% | Yes | FUNCTION |

- Visited clearing writes exactly 0x1200 bytes; route nodes allocate 16 bytes
  and leave their parent field uninitialized. Open-list teardown caches next
  before freeing and clears the head. `ObjHasExit` compares signed-byte exit
  coordinates to full-width entrance coordinates. `IsPathCell` follows the
  RF bits 1/2 and map flag 0x10 combination exactly.
- Road helpers use logical shifts of fixed-point coordinates. Claim counters
  are wrapping bytes. Cost returns only AL (2 absent, 3 for nonzero kind,
  otherwise 1). A named byte local closes three operand-width mismatches in
  the final selection; casts on ternary arms were inert.
- Build-slot removal scans 256 keys, including inactive slots, clears only
  the first matching object's pointer and decrements the live count without
  an active check. This preserves the original possibility of underflow.
  Tile marking picks the first 0xffff key among 128 four-byte entries, packs
  x/y into a word and clears its count; there is no duplicate check.
- Rectangle append first subtracts existing overlap, then copies the input
  and increments the count without a capacity guard. Growth cycles sides
  until four consecutive failures. `SettleOrderSpan` checks a candidate and
  advances once more before stopping for success or a completed cycle.
- Twelve bodies match first compile. Besides the byte-local lever above,
  conventional field copies, counted loops and sequential guard returns
  produce the exact original code. Existing prototypes remain unchanged;
  some callers call `SettleOrderSpan` by the alias `WalkOrderSpan` and some
  discard the route-node return. No duplicate implementation is added.


## sysstubs.c — complete

All 42 functions pass audit, 100% matchfull, warning-free compilation and
actual relocated-byte comparison (123 address relocations). Every marker is
FUNCTION, including the import thunk and terminal tail call; no divergence
or residual remains.

| Address | Name | Instructions | Match | Audit [OK] | Marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00499450 | `GetTicks` | 1 | 100% | Yes | FUNCTION |
| 0x0047f850 | `DebugFlush` | 1 | 100% | Yes | FUNCTION |
| 0x0047f870 | `DebugPrintf` | 1 | 100% | Yes | FUNCTION |
| 0x004688e0 | `ScriptState_NoOp` | 1 | 100% | Yes | FUNCTION |
| 0x00443710 | `GetModelContext` | 2 | 100% | Yes | FUNCTION |
| 0x00443250 | `Sin` | 3 | 100% | Yes | FUNCTION |
| 0x00443260 | `Cos` | 3 | 100% | Yes | FUNCTION |
| 0x00458bb0 | `SetMapReady` | 3 | 100% | Yes | FUNCTION |
| 0x00474880 | `SetInGameIconHandlers` | 3 | 100% | Yes | FUNCTION |
| 0x00474070 | `IsLShiftDown` | 4 | 100% | Yes | FUNCTION |
| 0x00474080 | `IsRShiftDown` | 4 | 100% | Yes | FUNCTION |
| 0x00468840 | `ClearScriptStateBytes` | 5 | 100% | Yes | FUNCTION |
| 0x00492110 | `MakePlayable` | 5 | 100% | Yes | FUNCTION |
| 0x00492d80 | `StopMusic` | 5 | 100% | Yes | FUNCTION |
| 0x0047fe70 | `WNDENV_Minimise` | 5 | 100% | Yes | FUNCTION |
| 0x0047fe80 | `WNDENV_Restore` | 5 | 100% | Yes | FUNCTION |
| 0x00499460 | `GetSimClock` | 7 | 100% | Yes | FUNCTION |
| 0x00488820 | `GetZBufferPixel` | 8 | 100% | Yes | FUNCTION |
| 0x00497580 | `NewSprite` | 9 | 100% | Yes | FUNCTION |
| 0x00453ce0 | `DBError` | 10 | 100% | Yes | FUNCTION |
| 0x00475f10 | `RestoreCurrentMenu` | 10 | 100% | Yes | FUNCTION |
| 0x00498900 | `SetSpeechVolume` | 10 | 100% | Yes | FUNCTION |
| 0x00442de0 | `DotProduct` | 11 | 100% | Yes | FUNCTION |
| 0x00474190 | `SaveSidePanelState` | 11 | 100% | Yes | FUNCTION |
| 0x004741c0 | `LoadSidePanelState` | 11 | 100% | Yes | FUNCTION |
| 0x00473a60 | `KillMouseDevice` | 11 | 100% | Yes | FUNCTION |
| 0x00499380 | `FreezeGameClock` | 12 | 100% | Yes | FUNCTION |
| 0x00468910 | `NewScriptEvent` | 13 | 100% | Yes | FUNCTION |
| 0x0049a120 | `CanHireGardener` | 14 | 100% | Yes | FUNCTION |
| 0x0049a160 | `CanHireMechanic` | 14 | 100% | Yes | FUNCTION |
| 0x00457910 | `SaveCurrency` | 15 | 100% | Yes | FUNCTION |
| 0x00457940 | `LoadCurrency` | 15 | 100% | Yes | FUNCTION |
| 0x0046b760 | `RestoreScriptStepHelp` | 15 | 100% | Yes | FUNCTION |
| 0x00468940 | `FreeScriptEvent` | 15 | 100% | Yes | FUNCTION |
| 0x004920e0 | `NewSampleRecord` | 16 | 100% | Yes | FUNCTION |
| 0x00495a10 | `InitMusicSystem` | 16 | 100% | Yes | FUNCTION |
| 0x0046ce20 | `KillAdvisorHelp` | 17 | 100% | Yes | FUNCTION |
| 0x00442da0 | `CrossProduct` | 22 | 100% | Yes | FUNCTION |
| 0x00456770 | `HalfPos` | 22 | 100% | Yes | FUNCTION |
| 0x00468bb0 | `AddHelpMessage` | 24 | 100% | Yes | FUNCTION |
| 0x004887a0 | `InitZBuffer` | 25 | 100% | Yes | FUNCTION |
| 0x0048af40 | `FreePlayItemAdd` | 27 | 100% | Yes | FUNCTION |

### Naming evidence

- `sub_4688e0` → `ScriptState_NoOp`: a bare return called by `LoadScripts`
  after the ten-byte state reset. No behavior is inferred for the empty hook.
- `sub_458bb0` → `SetMapReady`: stores its argument to `g_map_ready` at
  0x00667c7c, the readiness latch consumed by `bighelp.c`.
- `sub_474880` → `SetInGameIconHandlers`: installs 0x00474820/30 into the
  icon callbacks at 0x006687bc/c0; this name already appears in `screens3.c`.
- `sub_468840` → `ClearScriptStateBytes`: clears the exact ten-byte script
  state serialized by `savechunks.c` at 0x007fe930.
- `sub_475f10` → `RestoreCurrentMenu`: rechecks the selected menu unless its
  index is 5, then sets the side-panel restore byte to 3 after loading.
- `sub_474190` / `sub_4741c0` → `SaveSidePanelState` / `LoadSidePanelState`:
  write/read menu index, object-list mode and the 16-byte panel record.
- `sub_46b760` → `RestoreScriptStepHelp`: when the script is not running and
  a current step exists, displays its text in mode 1 and resets its timer.
- All remaining function names follow the scope and existing callers. No
  second body is introduced for a caller alias.

### Mechanics and retained behavior

- `GetTicks` is an indirect `GetTickCount` import jump, bounded by the audit.
  `DebugFlush`, `DebugPrintf` and the script hook are bare returns. `Sin` and
  `Cos` are actual x87 `fld/fsin/ret` and `fld/fcos/ret` sequences, not CRT
  jumps. Shift queries return bit 7 of the corresponding key byte.
- Clock freezing records system ticks and the current simulation counter,
  returns 0 for a new freeze and 1 if already frozen. `GetSimClock` chooses
  the saved/current counter and subtracts the offset. Its explicit if/else
  closes a 7i/27B, three-mismatch eager-initialization form to 7i/26B exact.
- Audio stop sets a flag and signals the music event. Speech volume is cached
  and forwarded to DirectSound +0x3c when a buffer exists. Music setup creates
  a thread with a 0x4000-byte stack only when the engine exists; it still
  reports success if thread creation fails. `KillMouseDevice` unacquires and
  releases the interface without clearing the global pointer.
- `NewSampleRecord` allocates 0x38 bytes with no null check and clears only
  the observed fields, in original order; the remaining source payload is
  uninitialized. `MakePlayable` pushes the result on the live list. Sprite
  allocation checks for failure but initializes only the link. Script events
  use zeroed 0x44-byte storage with explicit next/text/kind/mode stores;
  destruction frees text only when flag 0x20 owns it.
- Currency save/load short-circuit on the first failure and normalize the
  second result. Side-panel save/load ignore each operation's result. Both
  hiring predicates have a side effect: after checking 30 bricks and a
  count below 15, they spend 30 bricks, without incrementing worker counts.
- `DBError` and `AddHelpMessage` retain unbounded varargs formatting into
  shared buffers. The former calls a no-op error sink; the latter attaches
  copied text to a new event and queues it. Help teardown releases owned text,
  restores the help icon, clears flags/menu help/advisor pose, and tail-calls
  speech stop. It does not clear the freed text pointer.
- Dot product sums z, then y, then x. Cross product preserves the exact x87
  operand order and sequential stores, including aliasing effects. `HalfPos`
  uses explicit sign branches and shifts; the INT_MIN negation edge is left
  as the binary implements it, rather than replacing it with signed division.
- Z-buffer reads use byte pitch and return the high byte of a 32-bit pixel.
  Initialization builds the 640×480 image around the original static storage,
  creates its sprite and ORs 0x208, without allocation checks. Free-play add
  accumulates lookup cost, marks a found entry and increments the selected
  count even for unknown names; the first increment enables the accept icon.

### ABI and measured levers

- Forty-one bodies close on first compile; `GetSimClock` is the only source
  shape adjustment, described above. Math uses VC6 intrinsics; no inline
  assembly, fabricated tails or extra optimization flags are needed.
- The original PE import directory verifies the four IAT slots for
  `GetTickCount`, `SetEvent`, `ShowWindow` and `CreateThread`. Imported and COM
  calls use stdcall; game routines use cdecl. The varargs list starts directly
  after the format pointer in the x86 stack arguments. Intrinsics have
  declarations but no emitted external calls; all emitted extern callees
  carry their original address comments.
- Local return types recover actual pointer, AL, eax or x87 results even when
  existing callers discard them or use a differently named declaration.
  `RestoreScriptStepHelp` returns a Boolean although `savegame.c` declares
  its old name void; no caller was edited. Callback definitions retain the
  original stack shape. There are no unresolved byte-level ABI differences.

## Final scope C result

| File | Functions | Exact | WIP | Instructions | Bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| screencb7.c | 7 | 7 | 0 | 48 | 140 |
| lfmisc.c | 16 | 16 | 0 | 238 | 621 |
| simcore2.c | 10 | 10 | 0 | 173 | 485 |
| pathmisc.c | 13 | 13 | 0 | 220 | 589 |
| sysstubs.c | 42 | 42 | 0 | 431 | 1386 |
| **Total** | **88** | **88** | **0** | **1110** | **3221** |

All 88 scope-table addresses are implemented once, with 100% matchfull and
[OK] full-extent audit results. All five compile cleanly with /W3; independent
COFF checks verify all 204 address relocations and all 3221 original body
bytes. No runtime gameplay claim is made: validation is original-executable
matching. There are no remaining partials, escapes or unexplained differences.

Only the five assigned new C files and this report are committed. Existing
sources, other lanes, shared tools and central progress documents are unchanged
by this lane. Scratch disassembly, experiments and per-function verification
outputs remain untracked under `scratchpad/codex-c/`; binaries are not staged.

Local checkpoints follow the requested file order on `codex/scope-c`.
Publishing the branch is pending separate user approval: automatic approval
review rejected the scope-C push because it treated the earlier GitHub
approval as applying only to scope A. No retry or alternate upload was made.
