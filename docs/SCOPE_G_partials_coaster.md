# Scope G — partials: coaster, school car, joust (2026-09-05)

**Read `docs/PARALLEL_CONTRACT.md` first, including its "Extra rules for
PARTIAL scopes".** Branch: `scope/G`. Notes: `docs/lanes/scope-g.md`. Object
prefix: `/tmp/sg_`.

Nineteen existing `// WIP-FUNCTION:` bodies in the coaster / school-car /
joust families. Most are from the last four rounds and carry precise notes;
the coaster ones were written while the x87 and cursor-anchor levers were
still being discovered, so several may yield to levers recorded after them.
**Edit only these bodies and their notes; never a `// FUNCTION:` body.**

| mismatch | file | address | function | what the note says |
| --- | --- | --- | --- | --- |
| 3 | `schoolcar2.c` | 0x00401080 | `SchoolCarManoeuvreC` | byte-exact; the else arm's first scratch-temp triple starts one step earlier in the eax→ecx→edx rotation in the original; positional in this build; ~45 builds, free volatile inert. Twin of `ManoeuvreA` — one fix closes both |
| 10 | `schoolcar2.c` | 0x00401320 | `SchoolCarManoeuvreA` | same fact |
| 4 | `coaster4.c` | 0x00428750 | `InitTrackDrawModes` | only the table cursor's bias: original anchors at `&tbl[0]` with `[esi]/[esi+4]/[esi+8]`, ours at `+8` with negative displacements; the anchor rule says "most references, ties to the LAST" — a duplicated reference to the FIRST field reaches +0 but permutes the loads; try naming the row pointer differently |
| 6 | `schoolcar4.c` | 0x00422000 | `TrackCurve_GatherParams` | one DEAD `mov eax,[g_tc_n]` at the inner loop's exit is missing; twelve sort spellings; recorded as unreachable — re-test only with the latch-order levers |
| 6 | `schoolcar6.c` | 0x00422e40 | `Shade_BuildRamp` | `strict == rb == ob`: `mov ecx,<greenbits>` + `mov edx,1` sit between the 5th and 6th `fstp st(0)` four slots earlier than ours |
| 8 | `schoolcar3.c` | 0x004234e0 | `Coaster3D_DrawMesh` | the `sub esi,ecx` + `[esi+ecx]` IV elimination — the REACHABLE half is "two lockstep cursors spelled the SAME way" (17 → 8 came from it); what remains is the preheader IV |
| 8 | `schoolcar3.c` | 0x00428cb0 | `Coaster3D_BuildTrackMesh` | two preheader spill stores in the other order, and one reload from the frame home where the original copies the register — that reload is the one-byte deficit; called a floor |
| 10 | `coaster3d.c` | 0x00425e20 | `Coaster3D_SetupView` | index 7: two loads, registers swapped |
| 12 | `coaster6.c` | 0x00421660 | `CoasterCar_BuildRider` | byte-exact; three scheduling windows, one per substitution loop: VC6's fold of `base + i*STRIDE` into a two-register `movsx` addressing mode is not reachable (~30 spellings); a volatile blocks it but is a barrier |
| 27 | `schoolcar4.c` | 0x00402490 | `SchoolCarBlockedAhead` | a mixed two-axis schedule: VC6 offers exactly two regimes, all 40 legal interleavings hit one; the barrier that would mix them re-ranks the register it sits on |
| 53 | `schoolcar3.c` | 0x00428f00 | `Coaster3D_InitTrackTopology` | first 96 exact; then one allocation decision — the original spills the instance counter and keeps the pair cursor in ebx; ours the reverse (`6*s` hoisted with `18*s` as `3*six` was 75 → 53) |
| 53 | `schoolcar5.c` | 0x0041e000 | `Route_StepFree` | one five-instruction window at the loop head: the original builds a shared address register (`lea eax,[esi+0x20]`) for the three snapshot loads and does them AFTER the first two pushes; the shared `lea` base for a three-scalar snapshot group is recorded as unreachable at two sites — this and `Route_StepToPieceEnd` are one residual |
| 78 | `schoolcar6.c` | 0x0041df00 | `Route_StepToPieceEnd` | the same missing `lea eax,[esi+0x20]` |
| 99 | `schoolcar5.c` | 0x00421e90 | `TrackCurve_Refine` | two x87 residency choices: original `fst m` (kept) then `fstp b2 / fld b2 / fmul b2 / fxch`; ours `fstp m / fld m` then `fst b2 / fmul b2`; the embedded-assignment `fst` lever fixed one half but cost the same elsewhere |
| 231 | `coaster3d.c` | 0x0042a2f0 | `Raster_SubmitPoly` | instruction- and byte-exact; index 2 `sub esp,0x1fc` vs `0x200` — one homed dword short, which shifts every `[ebp-N]`; NOT hand-written asm |
| 319 | `goldrush.c` | 0x00402780 | `StepSchoolCar` | the flag-slot construct: a separate `int flag` reproduces the tail exactly (1147/1147 bytes) but takes a 15th frame slot; the original shares E-0x38 with `GetTileDimensions`' height out-param — but the push-depth extractor showed only 5 of 15 frame homes actually agree, so the "fifteenth slot" reasoning was wrong; index 19 is the allocation |
| 37 | `goldrush3.c` | 0x004070b0 | `GoldRush_KneelAtPan` | the original keeps the running y in EDI as ONE web across `__ftol` to the store and the argument copy; ours lets it die at the store and reloads; 95 variants; closest 58/58 at 26 needing a local plus a volatile read |
| 22 | `joust.c` | 0x00417430 | `TempleSlide_Update` | byte length exact; `wy`/`wx` in ebx/edi where the original has edi/ebx and a cascade follows; the projection and the loop-head allocation are COUPLED; the busy-guard form (`== 0` block vs `!= 0 goto`) decides which copy of a merged tail hosts it — measured the OPPOSITE way on `Joust_Update`, so re-measure both |
| 16 | `joust2.c` | 0x00407c30 | `Joust_Update` | two clusters argued to floors: a seat-multiply rotation deciding which push chain a case merges into, and loop 2's two gate locals taking the callee-saved pair the other way round (free volatile moves it 0/+4/+5/+6 — a global web rank) |

**Not in this table:** `MatMul` and `Coaster3D_BuildPieceGeometry` are
complete zero-mismatch bodies blocked by the extent-walker defect — leave
them; `ZBuffer_FillPoly` is partly hand-written assembly — leave it.

**Where to start.** The twins first (`ManoeuvreA`/`C`), then the byte-exact
small residuals (`InitTrackDrawModes` with the anchor rule, `Shade_BuildRamp`
and `TrackCurve_GatherParams` with the latch-order sweep), then the coaster
mesh trio. The x87 levers recorded AFTER `TrackCurve_Refine` was written
(embedded assignment produces `fst`; a value used in both arms must be READ
IN BOTH ARMS; a two-step float local blocks reassociation and picks `fiadd`;
`fsub st(3)` needs a distinct variable plus a free volatile read at the copy)
are the ones to try there.

**Files you own for this scope:** `schoolcar2.c`, `schoolcar3.c`,
`schoolcar4.c`, `schoolcar5.c`, `schoolcar6.c`, `coaster3d.c`, `coaster4.c`,
`coaster6.c`, `goldrush.c`, `goldrush3.c`, `joust.c`, `joust2.c` — WIP bodies
and their notes only.
