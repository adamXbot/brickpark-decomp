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

Remaining files are in progress.
