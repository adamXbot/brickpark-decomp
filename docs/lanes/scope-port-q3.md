# PORT-Q3 — the class-B crash-guard sweep (2026-09-13, done by the integrator)

`docs/QUIRKS.md` class B: null dereferences the shipped game gets away with on
x86 (a fault, at worst) and that wasm32 turns into silent garbage reads or
writes into linear memory 0..0x10. Policy rule 3: guard, don't fix — the guarded
arm returns quietly where the original would have faulted. Every guard is in a
`#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)` arm; VC6 text untouched.
The lane agent stalled at its first step; the integrator did the sweep.

## Sites

| site | function | guard | outcome on the guarded path |
| --- | --- | --- | --- |
| `bswater.c:140` | boat-blocks-square test | the two `st->` compares run only when `st` is non-null | a boat with no owning station does not block |
| `bswater.c:449/562` | `BsWater_SetTile` 5×5 fill | `if (!cell) continue;` | off-map squares of a lake within two cells of the edge are skipped (the shipped game WROTE through null) |
| `bswater2.c:142` | `BsWater_RemoveOne` head walk | `while (w && …)` | an empty lake list falls to the existing `if (w)` |
| `bswater2.c:142` | `BsBoat_StepLeg` | `if (!cell) return;` before the walk, `if (!st) return;` after it | a boat on a square with no water record, or whose school was removed, skips the step |
| `bswater2.c`, `bswater3.c`, `ridemisc3.c` ×5, `ridemisc4.c` ×5 | every `*_RemoveRecord` / record-unlink walk of the shape `link = &head->next; node = head; while (*link != rec)` | `if (!node) return;` after the two declarations | an empty record list unlinks nothing (the shipped game read address 4). Twelve walks of the one shape, more than the seven the comment counted |
| `anim2.c:314` | `JcBoat_Step` | `if (!w) return;` after the water lookup, `if (!st) return;` after the station walk | the boat waits a tick |
| `junglecruise.c:334` | `JungleCruise_RebuildRoute` | `if (!st) return;`, `if (!start) return;` | no route rebuilt for an unknown station |
| `junglecruise.c:632` | `JungleCruise_TryLaunchBoat` | `if (!st) return 0;` | no boat launched |
| `junglecruise.c:1185` | `JungleCruise_RelinkRiverCell` | `if (!st) return;` after the walk | the cell keeps its links; the four neighbour lookups are left unguarded as the comment proves them non-null while the link bit is set |
| `junglecruise.c:1040` | `JungleCruise_UpdateRiverTile` 5×5 fill | `if (!c) continue;` | as `bswater.c:562` |
| `lfentrance.c:361` | `LFEntrance_Add` step 5 | `if (!p2) return;` | the entrance is not laid on an allocation failure; steps 1 and 3 already guard theirs |
| `logflume.c:1784` | `LFTrack_Remove` | `LFRun_AddCount` only when `piece` | removing a square with no piece record decrements nothing |
| `logflume3.c:475` | `LFCorner_Place` | `if (!parent) return;` before the reads | no corner placed |
| `mappath.c:362` | `ClearCellForPath` | `if (!cell) return;` | an off-map tile is left alone |
| `objrect.c:653` | `GetRouteNode` | an off-map square substitutes a static cell with flags 0x40 | routes as impassable (axis 5, cost −1), the arm the shipped code takes for flag 0x40 |
| `simcore.c` (`RequestRoute`) | route-node walk | `if (cell && …)` | an off-map node is not cleared |
| `ridecb2.c:716`, `ridecb5.c:380` | the two water `_Remove`s | `if (!cell) return;` | an off-map square is not removed |
| `ridecb2.c:951` | `JungleCruise_Tick` station switch | **left** | the function is a WIP body (354i, mismatch 11); guarding it needs a matching pass first — recorded, not done |
| `ridecb6.c:1020` | `DrivingSchool_ResetPaths` | `if (!start) return;` | a school with no entrance block yet resets nothing |
| `ridecb6.c:1142` | `BoatingSchool_RebuildRoute` | `if (!st) return;`, `if (!start) return;` | as the cruise's |
| `roads.c:105` | `Road_SetTile` claiming loop, the crossing arms, the four `->rf = 3` stores | `continue` / `return` / a null-checked temporary | a road block over the map edge writes nothing off the map (the shipped game WROTE through null) |
| `screencb2.c:942` | `JcMonkeyFish_GetDrawDesc` | `if (!f) return 0;` | no draw descriptor for a square with no record |
| `sysmisc3.c:93` | `IsObjectRunning` | `f = c ? c->flags : 0` | an off-map square reads as running (the AI only asks about squares it found an object on) |
| `texture.c:244` | `BuildTextureRecord` | `if (!img) return;` | no texture built |
| `unref3.c:221` | `Coaster_WriteSaveBlob` | `n = b ? b->size : 0` | the existing null test then returns |
| `unref6.c:368` | `AddClippedClassIcon` | `if (p)` on the render-slot store | returns the null the caller must already handle |
| `unref7.c:732` | `GetLayeredSpriteBounds` | `if (!obj \|\| …)` | takes the arm the shipped `if (obj)` guards |
| `workorder3.c:298/339` | `InsertPrintItem` | `if (!it) return;` after the diagnostic | the sprite sorter skips a null node |
| `unref6.c` `OpenMessageBar` icon | not touched | the comment's "returned icon stored through" is `AddClippedClassIcon` above |

Not guarded, with reason: `anim2.c`/`junglecruise.c` neighbour lookups after a
link-bit test (cannot be null while the bit is set, per the source comments);
`ridecb2.c:951` (WIP body, see the row).

## Spot check

`QUIRKS.md` asked for a road block and a lake square over the map edge in free
play. Not driven: the batch was done by the integrator without a play session;
both sites are `continue`/`return` guards on the same `MapCellAt` shape that
PORT-Q1's G1/G2 use, and both builds compile and pass ctest. The next free-play
lane owes the two placements (`llPeek(0, 64)` before/after on the faithful build
shows the write; on the default build it must not).

## Gates

| gate | result |
| --- | --- |
| audit on the 24 files | PASS, rows identical (ridecb2's WIP row unchanged) |
| relocs per file / `--all` | 0 MISMATCH / 0 MISMATCH, 16 WIPRELOC (the accepted set) |
| markers | identical |
| progress | 3281 exact / 42 WIP |
| extern / bvstruct / m10 / addr / variadic sweeps | at baseline |
| native default + ctest | 21/21 |
| wasm default + ctest | 28/28 |
| wasm faithful + ctest | 28/28 |
| verify.py | run by the integrator before push |
