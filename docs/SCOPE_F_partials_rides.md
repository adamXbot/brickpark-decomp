# Scope F — partials: rides and callbacks (2026-09-05)

**Read `docs/PARALLEL_CONTRACT.md` first, including its "Extra rules for
PARTIAL scopes".** Branch: `scope/F`. Notes: `docs/lanes/scope-f.md`. Object
prefix: `/tmp/sf_`.

This is a PARTIAL scope: fifteen existing `// WIP-FUNCTION:` bodies in the
ride subsystem. Most were last worked when the lever corpus was a third of
its present size, and every one carries a note stating its residual and what
was ruled out. The job is a reconstruction-error pass, then the §6B triage,
then either a close or an honest retirement. **Edit only these bodies and
their notes; never a `// FUNCTION:` body around them.**

| mismatch | file | address | function | what the note says |
| --- | --- | --- | --- | --- |
| 3 | `bswater.c` | 0x0041c4c0 | `BsWater_SetTile` | `strict == rb == ob`: `inc ebp` second vs fourth in the inner latch; twenty variants; the store-order lever (`ZBuffer_RunCommand`, `RecolourModelParts`, `PrintScreenMode7`) has since reached three such latches — sweep the update order in the `for` clauses |
| 3 | `bswater3.c` | 0x004198a0 | `BsBoat_Animate` | index for index the same residual as `JcBoat_Animate` (`roads.c`, 3): the `imul` operand rank at 275–277; a fix on either closes both |
| 3 | `roads.c` | 0x00433840 | `JcBoat_Animate` | see above — work the pair together; that note carries a formal retirement argument, test it against the newer levers only |
| 5 | `ridecb3.c` | 0x0042aa90 | `Balloonz_Tick` | `strict == rb == ob`, one permutation at 10–14; the two stores are NOT one object (U-pipe slot choice); no source shape moves the load into the first U slot |
| 15 | `mechrides.c` | 0x00416330 | `SpiderRide_Activate` | the five `_Activate` functions are ONE register choice: `p = b->person;` in ECX pushes the `screen.ox` temp into EDX where it hoists; deleting the cache gives rb 3 but sinks the load below the escaped `pos` stores — the two halves are not simultaneously reachable by any spelling yet found |
| 19 | `mechrides.c` | 0x0043c950 | `SpinningBarrels_Activate` | same |
| 19 | `mechrides.c` | 0x0043e410 | `PlaneRide_Activate` | same |
| 132 | `mechrides.c` | 0x00415220 | `SafariRide_Activate` | ONE register: `p` is ECX for us, EDX in the original — the whole body is a three-way eax/ecx/edx rotation from index 113 |
| 138 | `mechrides.c` | 0x0043bac0 | `SpaceTower_Activate` | the tie-break is ECX holding `base_y` because the original evaluates `def->base_y` before the `tile->b.y` byte load; a spelling that reaches that order exists but costs a fifth frame slot — get `base_y` into ECX rather than EDX at index 27 |
| 15 | `ridecb9.c` | 0x00434f90 | `JungleCruise_Add` | read the note |
| 29 | `ridecb3.c` | 0x0042c820 | `Carousel_Tick` | same family as the `_Activate` five (real 8, strict 29) |
| 32 | `screencb.c` | 0x0041bfb0 | `BsWater_DrawSelection` | twins, identical residual index for index: a register rotation (station cursor eax / ref x ecx swapped) plus one spill (`cell->key.b.x` kept in the dead `p` argument slot across `BasicObjectDCalcCursor`); the `sq.c[1]` union trick rematerialises y and can never rematerialise x (offset 0) |
| 32 | `screencb.c` | 0x00436470 | `JcWater_DrawSelection` | same — a fix on either closes both |
| 27 | `ridecb5.c` | 0x00413450 | `Road_FindDiagonals` | byte deficit fully accounted for (a second `mov edi,[esp+10h]` and its `jmp`); the `r == 0` arm candidates all fold |
| 47 | `ridecb5.c` | 0x0041a720 | `BoatingSchool_Tick` | the standing CSE hypothesis is FALSIFIED (pointing the guard at a different field leaves index 64 unchanged); shim 2 — two reloads where the original re-uses the stored value — is the gating problem; look at the join block at 0x41a82d |
| 18 | `ridemisc3.c` | 0x00411dc0 | `Pump_SnapToRoad` | VC6 jump-threads the provably-null `rec` into the trailing `if`; 46 spellings; only a `volatile` local reproduces the block structure at the cost of a stack home |

**Not in this table and not to be reopened:** `BoatingSchool_Add`
(`ridecb5.c`, EXHAUSTED), `ValidateCursor`, `Draw3DPersonModel`,
`RequestRoute`, `WW_AnyBlokeInRect`, `InsertChildIntoList`,
`ClampPopUpToScreen`, `UpdateControllerFromMouseData`.

**Where to start.** The five `_Activate` bodies plus `Carousel_Tick` are one
problem, and the newest levers speak to it directly: "a pointer cache is a
register consumer", "a value that must survive a call cannot be a FIELD of an
address-taken aggregate — assign plain locals INTO it", "an aggregate local
blocks reuse of a dead parameter's home slot", and "naming the intermediate
POINTER advances the rotation where volatile does not". Try the aggregate
placement family on the `pos` stores before anything else. The two
`_DrawSelection` twins and the two `_Animate` twins are the other paired
targets — one fix, two closes.

**Files you own for this scope:** `bswater.c`, `bswater3.c`, `roads.c`,
`ridecb3.c`, `mechrides.c`, `ridecb9.c`, `screencb.c`, `ridecb5.c`,
`ridemisc3.c` — WIP bodies and their notes only.
