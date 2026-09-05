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

Remaining files are in progress.
