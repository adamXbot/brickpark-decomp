# Scope F — rides and callbacks (2026-09-06)

In progress on `scope/F`, based on `f22f7cc7fa95f2d5740f89f4b53ae2624cc9e474`. The scope table has **16 functions**, despite the introductory count of fifteen. The user requested continued work toward 100% after the initial triage pass.

**Current result: fifteen of sixteen targets are exact (93.75% of targets).** The other-agent work from `origin/scope/F-fable` through `e9e1f914` is included in the isolated `scope/F` branch. **Only SpaceTower_Activate remains WIP.** Its strict mismatches have fallen from 136 through 34, 13 and 11 to **10 (95.5% instruction match)**. Its actual body has the original **222 instructions/698 bytes** and stack layout. Two instruction-order windows and register choices remain; this is not 100% completion.

All nine whole-file audits end `PASS`, with **127 `[OK]` functions**: the original 112 plus the fifteen closes. All fifteen targets have `FUNCTION` markers and audit `[OK]`; Space Tower retains `WIP-FUNCTION`. Files compile cleanly with `/W3 /O2 /Gy /Gd`. Every clean experimental build received its full-file audit. The landed Tower body was recompiled and re-audited with all 47 exact neighbors preserved. No previously exact body, shared type, extern declaration or tool was edited in this continuation.

The historical continuations below record earlier checkpoints and rejected hypotheses; their old completion counts are superseded by this summary and table. The eighteenth continuation records the latest Tower improvement. The nineteenth adds address-identity verification; the twentieth through twenty-third record further experiments without changing production's score.

**Address-identity check:** all fifteen closed targets have zero resolved-address mismatches under the newer `relocs.py` from `main`. Its sweep of the nine files also confirms two pre-existing issues in unchanged neighbors, `PlaneRide_Create` and `Copters_Activate`, already deferred to integration in `origin/main:docs/HANDOFF.md`. Thus the 127 audit `[OK]` count is the normalized instruction gate, not a claim that every neighboring address binding is correct. Details and unresolved-position limits are recorded in the nineteenth continuation.

The user explicitly authorized publication and set a goal to continue until 100%. Work remains isolated on `scope/F`, tracking `origin/scope/F`; the main checkout is untouched. The prior published checkpoint was `bfb283fb` (11 differences), following `d571ef8d` (13), `567b36f7` (34), `64eb174b` (136) and the other agent's eleven commits through `e9e1f914`. `scope/F-fable` is already incorporated; continue this work on `scope/F`.

## Current measurements

Instruction counts and bytes below are **real compiled body / original body**. Match percentages use the authoritative audit index score, `(original instructions - strict mismatches) / original instructions`, not byte similarity. Indices are zero-based. Fifteen targets have reached audit `[OK]`, as indicated by their zero residuals below.

| Address | Function | File | Instructions | Bytes | Match | Strict | First | rb / ob |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `0x0041c4c0` | `BsWater_SetTile` | `bswater.c` | 109/109 | 350/350 | 100% | 0 | none | 0 / 0 |
| `0x004198a0` | `BsBoat_Animate` | `bswater3.c` | 334/334 | 1133/1133 | 100% | 0 | none | 0 / 0 |
| `0x00433840` | `JcBoat_Animate` | `roads.c` | 330/330 | 1108/1108 | 100% | 0 | none | 0 / 0 |
| `0x0042aa90` | `Balloonz_Tick` | `ridecb3.c` | 637/637 | 1993/1993 | 100% | 0 | none | 0 / 0 |
| `0x00416330` | `SpiderRide_Activate` | `mechrides.c` | 376/376 | 1228/1228 | 100% | 0 | none | 0 / 0 |
| `0x0043c950` | `SpinningBarrels_Activate` | `mechrides.c` | 362/362 | 1148/1148 | 100% | 0 | none | 0 / 0 |
| `0x0043e410` | `PlaneRide_Activate` | `mechrides.c` | 387/387 | 1253/1253 | 100% | 0 | none | 0 / 0 |
| `0x00415220` | `SafariRide_Activate` | `mechrides.c` | 402/402 | 1292/1292 | 100% | 0 | none | 0 / 0 |
| `0x0043bac0` | `SpaceTower_Activate` | `mechrides.c` | 222/222 | 698/698 | 95.5% | 10 | 25 | 8 / 10 |
| `0x00434f90` | `JungleCruise_Add` | `ridecb9.c` | 141/141 | 438/438 | 100% | 0 | none | 0 / 0 |
| `0x0042c820` | `Carousel_Tick` | `ridecb3.c` | 378/378 | 1225/1225 | 100% | 0 | none | 0 / 0 |
| `0x0041bfb0` | `BsWater_DrawSelection` | `screencb.c` | 129/129 | 374/374 | 100% | 0 | none | 0 / 0 |
| `0x00436470` | `JcWater_DrawSelection` | `screencb.c` | 129/129 | 374/374 | 100% | 0 | none | 0 / 0 |
| `0x00413450` | `Road_FindDiagonals` | `ridecb5.c` | 64/64 | 157/157 | 100% | 0 | none | 0 / 0 |
| `0x0041a720` | `BoatingSchool_Tick` | `ridecb5.c` | 358/358 | 1164/1164 | 100% | 0 | none | 0 / 0 |
| `0x00411dc0` | `Pump_SnapToRoad` | `ridemisc3.c` | 33/33 | 85/85 | 100% | 0 | none | 0 / 0 |

**Extent verification.** The original Space Tower is 222 instructions/698 bytes, ending at `0x0043bd79` (`ret`, end address `0x0043bd7a`). The landed compiled body also ends at instruction 221: **222 instructions/698 bytes**. Audit includes no trailing data, and no branch escapes the body. Equal length does not imply exactness: eight instruction positions differ in the head/case-3 windows, including exchanged x/table registers; two case-8 instructions use EDI instead of EBX. The seat release still writes record offset `0xA4 + seat`; its earlier reconstruction error is not reintroduced. Road is the complete exact 64/157 body; Pump is the exact 33/85 body, with no padding counted.

**Triage definition.** `rb` is an index-aligned comparison that masks general-register names while preserving operand width, addressing form and resolved stack home; `ob` masks only the resolved stack-home offset, preserving other memory displacements. Stack depths were solved as control-flow constraints in both directions, seeded by entry and cdecl returns; direct branch joins and indexed ESP operands are included. No conflicting depths or unresolved ESP references remained. These counts are deliberately not equated to older notes' difflib edit distances or callee-saved permutation scores. For example, the two swapped `inc` instructions in BsWater collapse under register masking, but the window is still a scheduling permutation, not an allocation fix.

## Correction and residual findings

- **Space Tower seat release was a wrong field, not a compiler choice.** Original `0x43bc9d` is `mov byte ptr [edx+edi+0xa4],0`; EDI still holds the record in that case, and EDX is the widened seat byte. `TowerRec.seat` already starts at `0xA4`. The old raw `0x20` expression was inconsistent with both the existing type and behavior note. Replacing it with `rec->seat[b->seat] = 0` restores the correct store. The displacement grows by three bytes, reducing the real byte deficit from five to two. The missing case-3 reload and shared-tail layout still prevent an exact match; a nearly matching audit slice had hidden the field error.

- **Initial aggregate grouping did not resolve the activation cache tradeoff; seed LIFETIME subsequently does.** Plain world/projected pairs and paired BNV seed objects leave the instruction listing unchanged in the four BNV Activate callbacks. Combining address-taken screen/seed storage with live arithmetic values changes frame placement or spills. The original still needs both the early person load and the later screen-x load; grouping did not achieve both. Declaring the BNV seeds inside their owning cases subsequently restores these windows in Spider, Safari and Plane and removes the need for their person-cache workarounds. Space Tower is a separate head-allocation problem, and its corrected seat write must not be reverted to recover the old slice length.

- **Paired targets were tested together.** Both Animate bodies worsen when x0/y0 become one local aggregate; the straight-y multiply retains its existing operand-rank floor. The initial DrawSelection aggregate variants were inert or worse; the continuation closes both twins with the same cursor-first, narrowed-byte sequence described below.

- **Boating School's structural blocker is resolved.** The continuation changes only its four movement call sites to the two-Pos by-value form used by the other ride callbacks. Both target-y reloads disappear. Three local scheduling changes then leave only the enqueue counter register, detailed below.

- **Floors are bounded conclusions.** “At its current tested floor” here means the recorded earlier evidence plus the new families below did not improve the residual. It is not a proof that every possible C spelling is impossible. Reopen only with a new mechanism; retain the semantic seat fix regardless of score.

### BsWater_SetTile

**Closed: 109/109 instructions, 350/350 bytes, strict/rb/ob 0/0/0, audit `[OK]`, `// FUNCTION: LEGOLAND 0x0041c4c0`.** Materialize the paint coordinates into one local `{ int x; int y; }` pair after the four map-cell stores, before evaluating the tile expression; then pass its fields to the existing scalar SetMapTile call. The original loop-latch order is restored without changing the frame, argument-home reuse, bounds checks or the byte-table shift. Full-file audit keeps the three existing exact bodies and adds this one.

The structural experiment first used a two-coordinate by-value call: it fixed the latch but swapped the row reloads at 42/43 (2 mismatches at 350 bytes). Returning to the existing scalar call closes those final two, and the temporary function-pointer cast was removed. Both function-scope and block-scope pairs, either field assignment order and array views close when used with the scalar call. Computing the pair before MapCellAt is a different shape and is rejected. Earlier scalar-coordinate probes did not distinguish this placement after the cell writes; the scheduling-floor statement was too broad.

### BsBoat_Animate

Scope F (2026-09-05): 334i/1133B, 3 strict at 277-279, still the JcBoat_Animate multiplication window. Grouping the edge offsets x0/y0 in one local aggregate gives 54/1137B; the twin worsens too. Rejected. The prior operand-rank retirement stands for the tested source families. Full-file audit keeps 7 exact functions; details in docs/lanes/scope-f.md.

### JcBoat_Animate

Scope F (2026-09-05): 330i/1108B, 3 strict at 275-277. Tested the new aggregate-placement family on x0/y0 rather than repeating the retired operand-order/cast grid: 49/1112B, rejected (BsBoat twin 54/1137B). At its recorded operand-rank floor; no source change. Whole-file audit keeps 1 exact function. See docs/lanes/scope-f.md.

### Balloonz_Tick

**Closed: 637/637 instructions, 1993/1993 bytes, strict/rb/ob 0/0/0, audit `[OK]`, `// FUNCTION: LEGOLAND 0x0042aa90`.** Declare the uninitialized `char name[8]` at function scope, alongside the six locals shared by both passes. Copy `"Bloke??"` into it with memcpy **after** the first `r = item->riders` statement inside the first block. Both changes are necessary. The complete first and second passes now match, and the file audit keeps all seven existing exact bodies while adding Balloonz.

| Buffer scope | Copy before first rider read | Copy after first rider read |
| --- | ---: | ---: |
| Function scope, before cars declaration | 9 | **0** |
| Function scope, after cars declaration | 9 | **0** |
| Original inner block | 5 | 5 |

Every cell is 637 instructions/1993 bytes. A combined cars/name aggregate first exposed the exact entry schedule at the original 0x34 frame, but made cars escape and changed later code (504 with ESCAPES). Removing that grouping while retaining the name buffer's function scope and the copy's after-read placement closes the full body. Padded standalone name buffers add frame space and give 25. Earlier investigations varied scope with a declaration initializer, or copy placement while leaving the buffer block-local; their claim that the entry schedule was unreachable is superseded.

The landed `sizeof(name)` spelling was recompiled and re-audited. The literal's COFF references use offsets 0 and 4; its eight bytes independently match original data at 0x004b64bc (`Bloke??` plus NUL). The shared pass-local layout, pass-1 int versus pass-2 signed-char index, and pass-2 zero-store ordering are preserved.

### SpiderRide_Activate

**Closed: 376/376 instructions, 1228/1228 bytes, strict/rb/ob 0/0/0; audit `[OK]`, `// FUNCTION: LEGOLAND 0x00416330`.** In addition to the case-local seeds described below, keep the unshifted y product in sy and add empty-if consumers both before and after `sy2 = sy >> 9`. Accumulate `g_map_cfg->oy - Get_YScroll()` into sy2, retaining the existing barrier before the pivot subtractions. The complete arithmetic, depth pushes, registers, field accesses and branch bodies now agree.

**Previous scope checkpoint: 2 strict, 376/376 instructions, 1228/1228 bytes, rb/ob 2/2; WIP.** Declare pos inside case 0 and pos2 inside case 7. This fixes the mount flag/person hoist and the ascending departure short loads without changing the frame. Only y-chain indices 73/75 remain. The seed lifetime is the new lever; earlier scope tests concerned arithmetic temporaries, not these escaped objects.

Previous checkpoint (superseded): 376i/1228B, 15 strict, first 73. Grouping world coordinates, projected coordinates, or pos/pos2 is inert. Grouping adjusted coordinates gives 353; screen/spill 23; screen/pos 29. Departure Pos fields give 17/1230B, whole copies 225/1227B. Rejected. The current residual includes case 7 at 195/196/198 (the old marker's claim that its movsx pair was exact was stale), plus mount windows 73/75 and 87-97. At its current floor for tested aggregate forms; whole-file audit keeps 43 exact functions. See docs/lanes/scope-f.md.

### SpinningBarrels_Activate

**Closed: 362/362 instructions, 1148/1148 bytes, strict/rb/ob 0/0/0; audit `[OK]`, `// FUNCTION: LEGOLAND 0x0043c950`.** In addition to the case-local seeds described below, keep the unshifted y product in sy and add empty-if consumers both before and after `sy2 = sy >> 9`. Accumulate `g_map_cfg->oy - Get_YScroll()` into sy2, retaining the existing barrier before the pivot subtractions. The complete arithmetic, depth pushes, registers, field accesses and branch bodies now agree.

**Previous scope checkpoint: 17 strict, 362/362 instructions, 1148/1148 bytes, rb/ob 17/17; WIP.** Declare both BNV positions inside their respective cases and remove the obsolete person cache. Restore natural x-before-y departure stores: the ascending reads and both stores now agree. The mount y/depth-push schedule remains. The seed lifetime is the new lever; earlier scope tests concerned arithmetic temporaries, not these escaped objects.

Previous checkpoint (superseded): 362i/1148B, 19 strict, first 113. World, projected and seed-pair aggregates are inert. Adjusted coordinates give 253; screen/spill 42; screen/pos 31. A departure Pos field pair remains 19 but changes the residual, and whole copies give 71; no improvement. The mount cache/screen-load tradeoff and reversed departure stores remain. At the current floor for these forms; whole-file audit keeps 43 exact.

### PlaneRide_Activate

**Closed: 387/387 instructions, 1253/1253 bytes, strict/rb/ob 0/0/0; audit `[OK]`, `// FUNCTION: LEGOLAND 0x0043e410`.** In addition to the case-local seeds described below, keep the unshifted y product in sy and add empty-if consumers both before and after `sy2 = sy >> 9`. Accumulate `g_map_cfg->oy - Get_YScroll()` into sy2, retaining the existing barrier before the pivot subtractions. The complete arithmetic, depth pushes, registers, field accesses and branch bodies now agree.

**Previous scope checkpoint: 2 strict, 387/387 instructions, 1253/1253 bytes, rb/ob 2/2; WIP.** Declare pos in case 0, and ofs/pos2 in case 7. Remove the obsolete person cache and put the departure flag update after both pos2 stores. The short-load/doubling window and original byte length are restored. Only y-chain indices 72/74 remain. The seed lifetime is the new lever; earlier scope tests concerned arithmetic temporaries, not these escaped objects.

Previous checkpoint (superseded): 387i/1254B against 1253B, 19 strict, first 72. World, projected and seed-pair aggregates are inert. Adjusted coordinates give 361; screen/spill 27; screen/pos 33. Departure aggregate fields/copies give 239 at 1267-1269B. No accepted variant. The mount windows, case-7 short-load pair and post-UnAdjust load order remain the current floor for these tested forms. Whole-file audit keeps 43 exact functions.

### SafariRide_Activate

**Closed: 402/402 instructions, 1292/1292 bytes, strict/rb/ob 0/0/0; audit `[OK]`, `// FUNCTION: LEGOLAND 0x00415220`.** In addition to the case-local seeds described below, keep the unshifted y product in sy and add empty-if consumers both before and after `sy2 = sy >> 9`. Accumulate `g_map_cfg->oy - Get_YScroll()` into sy2, retaining the existing barrier before the pivot subtractions. The complete arithmetic, depth pushes, registers, field accesses and branch bodies now agree.

**Previous scope checkpoint: 2 strict, 402/402 instructions, 1292/1292 bytes, rb/ob 2/2; WIP.** Declare pos inside case 1 and pos2 inside case 7; remove the obsolete person cache. Both scopes are necessary: pos alone changes case-2 layout. The whole register rotation is gone. Only y-chain indices 89/91 remain. The seed lifetime is the new lever; earlier scope tests concerned arithmetic temporaries, not these escaped objects.

Previous checkpoint (superseded): 402i/1292B, strict/rb/ob 132/22/132; first 86. World, projected and seed-pair aggregates leave the listing unchanged. Adjusted coordinates give 318/1297B; screen/spill 150 and screen/pos 142 at 1292B. None fixes the ECX person-cache versus EDX screen-load choice. The scratch-register rotation starts after the mount window, not at the first mismatch. At the current floor for tested aggregate forms; 43 exact functions still pass the whole-file audit. See docs/lanes/scope-f.md.

### SpaceTower_Activate

Scope F (2026-09-05): RECONSTRUCTION ERROR FIXED in case 6. The original at 0x43bc9d clears [edi+edx+0xa4], matching TowerRec.seat; the old byte-pointer spelling wrote rec+0x20 instead. Use rec->seat[b->seat]. This repairs behavior even though index-aligned mismatch stays 138, first 23. The real compiled body is 220i/696B versus 222i/698B, not the equal-length body earlier notes claimed: audit's 222i/703B includes two decodes of trailing jump-table data after the ret. Before the fix, the real body was 220i/693B; the wrong short displacement hid three bytes. Named base-y and def pointers give 162 and 169; grouping by/tx gives 151 in either member order. Rechecked after the seat fix; none improves the head or removes the extra frame slot of the pair forms. Keep the existing allocation at its current tested floor, with the seat correction retained. Whole-file audit keeps all 43 exact functions. See docs/lanes/scope-f.md.

### JungleCruise_Add

Scope F (2026-09-05): 141i/438B, strict/rb/ob 15/15/15 with entry-SP homes preserved. Rechecked the original zero-web window 32-53 and later tile-paint flow. The newer Balloonz_NewRecord sub-object memset lever requires a separate destination base, exactly the unwanted lea here; it does not supply zero stores off the original ESI base. The prior split- memset/zero-carrier grids already cover that family, so they were not repeated. At its recorded floor; no body change, 19 exact functions pass.

### Carousel_Tick

**Closed: 378/378 instructions, 1225/1225 bytes, strict/rb/ob 0/0/0; audit `[OK]`, `// FUNCTION: LEGOLAND 0x0042c820`.** In addition to the case-local seeds described below, keep the unshifted y product in sy and add empty-if consumers both before and after `sy2 = sy >> 9`. Accumulate `g_map_cfg->oy - Get_YScroll()` into sy2, retaining the existing barrier before the pivot subtractions. The complete arithmetic, depth pushes, registers, field accesses and branch bodies now agree.

**Previous scope checkpoint: 7 strict, 378/378 instructions, 1225/1225 bytes, rb/ob 7/7; WIP.** Declare pos inside case 1 and pos2 inside case 7; remove the obsolete person cache. Keep a separate sy projection, subtract scroll into sy2, then add the origin with the empty-if barrier. This restores the callee-saved register assignment and the complete departure window. Only indices 114/115/118-122 remain. The seed lifetime is the new lever; earlier scope tests concerned arithmetic temporaries, not these escaped objects.

Previous checkpoint (superseded): 378i/1225B, strict/rb/ob 29/8/29, first 86. Grouping wx/wy or pos/pos2 leaves the listing unchanged; sx2/sy2 gives 268/1229B, screen/pos 40/1225B. Case-7 paired fields give 31/1227B; whole Pos copies give 59/1224B. None improves the full function. Keep the current body at its tested allocation/scheduling floor; the mount y web and departure short-load pair remain open. Full-file audit keeps 7 exact.

### BsWater_DrawSelection

**Closed: 129/129 instructions, 374/374 bytes, strict/rb/ob 0/0/0, audit `[OK]`, `// FUNCTION: LEGOLAND 0x0041bfb0`.** Assign `p->x` and `p->y` from the cell first, then copy their narrowed values into `sq.c[0]` and `sq.c[1]`. This preserves the early zero-extended coordinate loads and removes the x spill across BasicObjectDCalcCursor. The station cursor and complete later boat loop now match. Both twins take precisely the same change; whole-file audit preserves the original 13 exact functions and adds these two.

The earlier `(unsigned char)p->x` retirement was too broad: assignment order is decisive. Chaining x alone gives 13 mismatches at 378 bytes, with the entire later boat loop fixed; both interleaved chains add two register copies (real 131 instructions/378 bytes, despite a 129-instruction slice of 374 bytes). Writing both wide coordinates before either narrowed byte eliminates those copies and closes the body. Other ordered cursor/byte combinations leave 9–99 mismatches. Signed-byte union views are inert. This supersedes the original claim that x cannot rematerialise from its packed-square home.

### JcWater_DrawSelection

**Closed: 129/129 instructions, 374/374 bytes, strict/rb/ob 0/0/0, audit `[OK]`, `// FUNCTION: LEGOLAND 0x00436470`.** Assign `p->x` and `p->y` from the cell first, then copy their narrowed values into `sq.c[0]` and `sq.c[1]`. This preserves the early zero-extended coordinate loads and removes the x spill across BasicObjectDCalcCursor. The station cursor and complete later boat loop now match. Both twins take precisely the same change; whole-file audit preserves the original 13 exact functions and adds these two.

The earlier `(unsigned char)p->x` retirement was too broad: assignment order is decisive. Chaining x alone gives 13 mismatches at 378 bytes, with the entire later boat loop fixed; both interleaved chains add two register copies (real 131 instructions/378 bytes, despite a 129-instruction slice of 374 bytes). Writing both wide coordinates before either narrowed byte eliminates those copies and closes the body. Other ordered cursor/byte combinations leave 9–99 mismatches. Signed-byte union views are inert. This supersedes the original claim that x cannot rematerialise from its packed-square home.

### Road_FindDiagonals

**27 -> 2 mismatches, 64/64 real instructions, 159/157 bytes, strict/rb/ob 2/1/2, first 42; still WIP.** Both third-lookup arms initialize a temporary result, which is copied back after the output store. A free existing out-parameter read restores the original register allocation. The two-instruction load/increment difference remains; details in the eighth continuation below.

Previous checkpoint (superseded): Scope F (2026-09-05): strict/rb/ob 27/24/27, first 37. A one-member aggregate for n leaves the complete listing unchanged. The real body is 62i/151B versus 64i/157B; audit's 64i/153B includes two trailing NOPs. The original's two edge reloads address the SAME entry-SP-4 home as our single reload before the pending add esp,8. The six-byte deficit remains the second reload and its jump. At its recorded floor; 8 exact functions still pass. See docs/lanes/scope-f.md.

### BoatingSchool_Tick

**47 -> 3 mismatches, 358/358 instructions, 1164/1164 bytes, strict/rb/ob 3/0/3; still WIP, audit `[OK]` no.** All four CalcMoveLine sites now pass two `Pos` values through a scope-local `MoveLineFn` cast. The target ABI is still five cdecl dwords (source x/y, target x/y, path), and the shared five-scalar extern is untouched. This fixes the target-y copies and removes both volatile target-y reloads. The case-4 `cy` workaround is also removed. Only cases 0/3 changed: 44 mismatches; all four changed: 13. A named `Pos* world` at the case-0 call restores its source-y load's position (4 more), a free volatile action store preserves the direction/action order (2 more), and declaring the sound-source aggregate inside case 5 restores its field/push schedule (4 more). The combined result is 3, with the complete original byte length. Cleaned source was recompiled and the file re-audited with all 8 exact neighbors preserved.

The remaining instructions 64-66 are solely `mov/inc/mov` of `st->count`: ECX here, EDX in the original. A named count pointer is inert. Removing the count-read shim, using a scalar copied with memcpy, or wrapping the count in a signed/unsigned member gives 5 mismatches and 1163 bytes. Earlier claims that the two target reloads were an unavoidable barrier are superseded. Current marker: `// WIP-FUNCTION: LEGOLAND 0x0041a720  (99.2%, 3/358 mismatches at 64-66, ECX/EDX count register; 1164/1164B)`.

### Pump_SnapToRoad

**Closed: 33/33 instructions, 85/85 bytes, strict/rb/ob 0/0/0, audit `[OK]`, `// FUNCTION: LEGOLAND 0x00411dc0`.** Clear the pointer object with `memset(&rec, 0, sizeof(rec))`; initialize a separate result to zero, assign it in the successful arm, and return it once. The clear lowers to `xor eax,eax` without a stack home while preserving the later test. The result creates the join's `xor ecx,ecx` and failure return through `mov eax,ecx`. This supersedes the earlier retirement argument and all claims that only a volatile stack home can preserve the join. Whole-file audit passes with 12 exact functions.

Evidence: pointer `memset` alone retains the test but still has 18 strict mismatches (84-byte slice); adding the single result gives zero at exactly 85 bytes. A second `memset` for the result also reaches zero in either late-return or single-return form, but the ordinary zero assignment is sufficient. Same-width pointer/integer union clearing is inert; float and double union clears add instructions or storage. The complete exact body was recompiled with `/W3` and re-audited after changing its marker.

## New lever measurements

Each row gives `strict mismatch / audit-slice bytes`; these are experimental slices, not proof of full body size. `ESCAPES` is a rejection. All surrounding `[OK]` counts were preserved in compiling candidates. Invalid scratch transformations are not listed. Corrected Space Tower rechecks replace the incorrect seat expression before compiling.

| Function | Candidate | Mismatch / slice bytes |
| --- | --- | ---: |
| `SpiderRide_Activate` | `world_pair` | 15 / 1228 |
| `SpiderRide_Activate` | `projected_pair` | 15 / 1228 |
| `SpiderRide_Activate` | `adjusted_pair` | 353 / 1238 |
| `SpiderRide_Activate` | `seed_pair` | 15 / 1228 |
| `SpinningBarrels_Activate` | `world_pair` | 19 / 1148 |
| `SpinningBarrels_Activate` | `projected_pair` | 19 / 1148 |
| `SpinningBarrels_Activate` | `adjusted_pair` | 253 / 1152 |
| `SpinningBarrels_Activate` | `seed_pair` | 19 / 1148 |
| `SpinningBarrels_Activate` | `screen_spill` | 42 / 1148 |
| `SpinningBarrels_Activate` | `screen_seed` | 31 / 1148 |
| `PlaneRide_Activate` | `world_pair` | 19 / 1254 |
| `PlaneRide_Activate` | `projected_pair` | 19 / 1254 |
| `PlaneRide_Activate` | `adjusted_pair` | 361 / 1275 |
| `PlaneRide_Activate` | `seed_pair` | 19 / 1254 |
| `SafariRide_Activate` | `world_pair` | 132 / 1292 |
| `SafariRide_Activate` | `projected_pair` | 132 / 1292 |
| `SafariRide_Activate` | `adjusted_pair` | 318 / 1297 |
| `SafariRide_Activate` | `seed_pair` | 132 / 1292 |
| `SpiderRide_Activate` | `screen_spill_fixed` | 23 / 1228 |
| `SpiderRide_Activate` | `screen_seed_fixed` | 29 / 1228 |
| `PlaneRide_Activate` | `screen_spill_fixed` | 27 / 1254 |
| `PlaneRide_Activate` | `screen_seed_fixed` | 33 / 1254 |
| `SpiderRide_Activate` | `pair_fields` | 17 / 1230 |
| `SpiderRide_Activate` | `pair_copy` | 225 / 1227 (ESCAPES) |
| `SpiderRide_Activate` | `pair_shift` | 225 / 1227 (ESCAPES) |
| `SpinningBarrels_Activate` | `pair_fields` | 19 / 1148 |
| `SpinningBarrels_Activate` | `pair_copy` | 71 / 1148 |
| `SpinningBarrels_Activate` | `pair_shift` | 71 / 1148 |
| `PlaneRide_Activate` | `pair_fields` | 239 / 1269 |
| `PlaneRide_Activate` | `pair_copy` | 239 / 1267 |
| `PlaneRide_Activate` | `pair_shift` | 239 / 1267 |
| `Carousel_Tick` | `pair_fields` | 31 / 1227 |
| `Carousel_Tick` | `pair_copy` | 59 / 1224 |
| `Carousel_Tick` | `pair_shift` | 59 / 1224 |
| `Carousel_Tick` | `world_pair` | 29 / 1225 |
| `Carousel_Tick` | `adjusted_pair` | 268 / 1229 |
| `Carousel_Tick` | `seed_pair` | 29 / 1225 |
| `Carousel_Tick` | `screen_seed` | 40 / 1225 |
| `SpaceTower_Activate` | `base_y_pointer` | 162 / 694 |
| `SpaceTower_Activate` | `def_y_pointer` | 169 / 693 |
| `SpaceTower_Activate` | `base_y_pair` | 151 / 705 |
| `SpaceTower_Activate` | `base_y_pair_rev` | 151 / 705 |
| `BsWater_SetTile` | `cursor_counter_first` | 39 / 358 |
| `BsWater_SetTile` | `cursor_shape_first` | 39 / 358 |
| `BsWater_SetTile` | `index_counter_first` | 109 / 353 (ESCAPES) |
| `BsWater_SetTile` | `index_shape_first` | 109 / 353 (ESCAPES) |
| `BsBoat_Animate` | `edge_offset_pair` | 54 / 1137 |
| `JcBoat_Animate` | `edge_offset_pair` | 49 / 1112 |
| `SafariRide_Activate` | `screen_spill_fixed` | 150 / 1292 |
| `SafariRide_Activate` | `screen_seed_fixed` | 142 / 1292 |
| `BsWater_DrawSelection` | `pair_sq_ride` | 110 / 384 |
| `BsWater_DrawSelection` | `pair_ride_sq` | 109 / 384 |
| `BsWater_DrawSelection` | `key_pointer` | 32 / 375 |
| `BsWater_DrawSelection` | `st_with_square` | 109 / 361 |
| `BsWater_DrawSelection` | `c_with_square` | 109 / 361 |
| `JcWater_DrawSelection` | `pair_sq_ride` | 110 / 384 |
| `JcWater_DrawSelection` | `pair_ride_sq` | 109 / 384 |
| `JcWater_DrawSelection` | `key_pointer` | 32 / 375 |
| `JcWater_DrawSelection` | `st_with_square` | 109 / 361 |
| `JcWater_DrawSelection` | `c_with_square` | 109 / 361 |
| `BoatingSchool_Tick` | `no_shim_target_0` | 266 / 1164 |
| `BoatingSchool_Tick` | `no_shim_path_0` | 266 / 1164 |
| `BoatingSchool_Tick` | `no_shim_target_3` | 162 / 1164 |
| `BoatingSchool_Tick` | `no_shim_path_3` | 162 / 1164 |
| `BoatingSchool_Tick` | `no_shim_target_both` | 265 / 1167 |
| `BoatingSchool_Tick` | `no_shim_path_both` | 265 / 1167 |
| `BoatingSchool_Tick` | `shuffle_slot_pointer` | 47 / 1166 |
| `BoatingSchool_Tick` | `group_src_src2` | 47 / 1166 |
| `BoatingSchool_Tick` | `group_key_src` | 205 / 1178 |
| `Road_FindDiagonals` | `aggregate_counter` | 27 / 153 |
| `Pump_SnapToRoad` | `aggregate_result` | 18 / 80 |
| `Balloonz_Tick` | `rider_head_pointer` | 5 / 1993 |
| `SpaceTower_Activate` | `correct_seat_field` | 138 / 703 |
| `SpaceTower_Activate` | `base_y_pointer_seat_fixed` | 162 / 697 |
| `SpaceTower_Activate` | `def_y_pointer_seat_fixed` | 169 / 696 |
| `SpaceTower_Activate` | `base_y_pair_seat_fixed` | 151 / 708 |
| `SpaceTower_Activate` | `base_y_pair_rev_seat_fixed` | 151 / 708 |

Candidate definitions:

- `world_pair`, `projected_pair`, `adjusted_pair`: group wx/wy, sx/sy or sx2/sy2 in a body-local two-int struct. `seed_pair` groups the two existing BNV/Vec3 seeds. `screen_spill` and `screen_seed` group screen with the spill pair or seed. `_fixed` denotes a corrected scratch identifier substitution, not an additional semantic variant.
- `pair_fields`: departure short fields loaded into one Pos, then doubled into the destination. `pair_copy`: doubled Pos fields copied as one Pos; `pair_shift`: fill, shift each field, then copy. Existing z bytes remain untouched.
- `base_y_pointer` names the base-y field address before tiley; `def_y_pointer` names the definition pointer before tiley. `base_y_pair` puts by and tx into one local struct; `_rev` reverses member order. `_seat_fixed` rechecks those forms after the actual field correction.
- Water `cursor_*` introduces a row shape pointer; `index_*` introduces a flat shape index. Each compares `c++, sh++` with `sh++, c++` in the for clause.
- `edge_offset_pair` groups Animate x0/y0. `rider_head_pointer` names the Balloonz list-head address before reading it.
- Selection `pair_sq_ride` and `pair_ride_sq` group the packed square with the address-taken ride object in each order. `key_pointer` names &c->key. `st_with_square` and `c_with_square` group the station or cell pointer with the packed square.
- `aggregate_counter` wraps Road n in one struct member; `aggregate_result` wraps the Pump record pointer. Both leave the complete instruction listing unchanged.
- `no_shim_target_*` names &b->tx and accesses its coordinate pair; `no_shim_path_*` names &b->path. Each removes only the case-0, case-3, or both shim-2 volatile reads. `shuffle_slot_pointer` names the queue slot address. `group_src_src2` groups the sound-source locals; `group_key_src` groups the packed key with the first source.

**No redundant JungleCruise_Add grid.** The newer `Balloonz_NewRecord` sub-object memset example still materializes a destination pointer. That is the wrong `lea` already present here, not evidence for the required second zero stored through ESI. The existing whole/split memset, zero carrier, helper and placement grids already covered the relevant constructs. The original 32-53 window and the rest of the body were rechecked; the previous retirement stands.

## Mechanics and original quirks retained

- `BsWater_SetTile` allocates or reuses a water record, stores its mask/optional owner, marks the background dirty, and paints the 5x5 neighborhood. The unchecked map-cell dereference, partially initialized allocation and byte shape-table behavior are retained.
- `BsBoat_Animate` and `JcBoat_Animate` generate 80 route positions for drift, U-turn, straight and quarter-arc movement, then derive sprite headings from nearby samples. Boating School adds its hull index times 16 to the heading frame.
- Spider, Barrels, Plane, Safari and Carousel advance per-rider states, seed BNV animation in projected screen coordinates, manage seating flags/sprites/depth, free paths, and return departing visitors to map movement. The existing uninitialized third seed component and ride-specific overlapping seat/timer fields are preserved.
- Space Tower handles queue, boarding, seat-row facing, sitting, departure and exit stages; a missing record stops the rider walk. Case 6 now frees `seat[b->seat]`, as shipped. This fixes the reconstruction, not an original game bug.
- Balloonz runs a rider-state pass and a car/animation pass, copying shared record state through the same local homes and maintaining the six car states. The entry name initializer remains its only five-instruction scheduling window.
- The water selection twins resolve the station owning the selected route endpoint or draw the fixed water footprint; a boat occupying or approaching that square sets cursor error 1. The unchecked map-cell lookup result is retained.
- JungleCruise_Add allocates a 0x44-byte station, seeds timer 150 and take 3, publishes it, joins both river ends with the station itself as owner, and paints columns through right-1 with west/east edge tiles.
- Road_FindDiagonals looks up offsets (+4,-4), (+4,+4), (-4,+4), (-4,-4), counts existing records and optionally fills ring slots 1/3/5/7.
- BoatingSchool_Tick manages its five-person queue, launch threshold of six per water square, exit movement and source fading; boats advance every 80 ticks. The existing 9999 frame seed versus exact-100 animation reset quirk is retained.
- Pump_SnapToRoad accepts only a type-zero road record one square east of the cursor, then shifts the placement to record.x-1 and record.y minus the class footprint top.

No functions, callees or globals were newly named or renamed. No extern type was changed. Shared caller-specific CalcMoveLine and UnAdjustBlokePosition declarations, packed-square arguments and aggregate return conventions remain unchanged. BoatingSchool_Tick alone casts its four CalcMoveLine call sites to the equivalent two-Pos cdecl layout; aligning shared declarations across files is outside this scope.

## Verification and handoff

| File | Baseline `[OK]` | Final `[OK]` | Audit | `/W3` |
| --- | ---: | ---: | --- | --- |
| `bswater.c` | 3 | 4 | PASS | clean |
| `bswater3.c` | 7 | 7 | PASS | clean |
| `roads.c` | 1 | 1 | PASS | clean |
| `ridecb3.c` | 7 | 8 | PASS | clean |
| `mechrides.c` | 43 | 43 | PASS | clean |
| `ridecb9.c` | 19 | 19 | PASS | clean |
| `screencb.c` | 13 | 15 | PASS | clean |
| `ridecb5.c` | 8 | 8 | PASS | clean |
| `ridemisc3.c` | 11 | 12 | PASS | clean |

Every source edit was followed by a whole-file audit. Executable changes are confined to Space Tower's seat release, Pump's pointer/result control flow, Boating School's movement call sites and local scheduling, the two water-selection coordinate copies, the water-tile paint coordinate pair, Balloonz's buffer scope/copy placement, and case-local seed scheduling in the five BNV ride callbacks. Scope A/C and other worktrees were not modified.

Local reproducibility evidence is under `scratchpad/scope-f/`: `baseline.json`, `triage.json`, `experiments.jsonl`, full original/recompiled listings, candidate sources, and per-file audit logs. These are local scratch artifacts and are intentionally not committed. Compiler objects are under `/tmp/sf_*`.

Committed target markers:

```c
// FUNCTION: LEGOLAND 0x0041c4c0
// WIP-FUNCTION: LEGOLAND 0x004198a0  (99.1%, 3/334 mismatches, first 277; see Scope F note)
// WIP-FUNCTION: LEGOLAND 0x00433840  (99.1%, 3/330 mismatches, first 275; see Scope F note)
// FUNCTION: LEGOLAND 0x0042aa90
// WIP-FUNCTION: LEGOLAND 0x00416330  (96.0%, 15/376 mismatches, first 73; see Scope F note)
// WIP-FUNCTION: LEGOLAND 0x0043c950  (94.8%, 19/362 mismatches, first 113; see Scope F note)
// WIP-FUNCTION: LEGOLAND 0x0043e410  (95.1%, 19/387 mismatches, first 72; see Scope F note)
// WIP-FUNCTION: LEGOLAND 0x00415220  (67.2%, 132/402 mismatches, first 86; see Scope F note)
// WIP-FUNCTION: LEGOLAND 0x0043bac0  (37.8%, 138/222 mismatches, first 23; real body 220i/696B, head allocation remains)
// WIP-FUNCTION: LEGOLAND 0x00434f90  (89.4%, 15/141 mismatches, first 32; see Scope F note)
// WIP-FUNCTION: LEGOLAND 0x0042c820  (92.3%, 29/378 mismatches, first 86; see Scope F note)
// FUNCTION: LEGOLAND 0x0041bfb0
// FUNCTION: LEGOLAND 0x00436470
// WIP-FUNCTION: LEGOLAND 0x00413450  (57.8%, 27/64 mismatches, first 37; see Scope F note)
// WIP-FUNCTION: LEGOLAND 0x0041a720  (99.2%, 3/358 mismatches at 64-66, ECX/EDX count register; 1164/1164B)
// FUNCTION: LEGOLAND 0x00411dc0
```

## Continuation experiments after the first checkpoint

All candidates below were isolated from production and received a whole-file audit after successful `/W3` compilation. Measurements are strict mismatches unless explicitly described otherwise. Rejected candidates did not replace the working bodies.

- **Pump transfer to Road:** clearing the record pointer with intrinsic memset preserves per-edge counter reloads, but introduces an extra XOR and reverses the diamond (24 mismatches, 158-byte audit slice). Discarded copies/clears leave the old 27; a separate copied counter creates a second stack home and gives 48–51. No correct zero-cost diamond close yet.
- **Space Tower:** named base-y plus a volatile final sum store avoids the fifth frame slot, a new result, but still chooses the wrong register and sum/store schedule (160–164, 700-byte audit slices). Def/field pointers, wider arithmetic carriers, aggregate placement and explicit case-3 y reloads do not improve the corrected production body. Every candidate retains the actual seat field at `0xA4`.
- **Activation callbacks:** memcpy/person pointer carriers and using the existing spill aggregate as the cache do not fix the person/screen-load tradeoff. Explicit float-to-double-to-float return casts are inert. By-value depth argument pairs change frame allocation and fail extent checks. Volatile departure coordinate stores and a named destination pointer do not improve the full bodies.
- **JungleCruise_Add:** the newly successful Pump pointer-clear mechanism does not transfer to the three rider-list zeroes. Pointer/uint carriers give 96–102, float/union carriers 118–122, and wide integer carriers 139–140. These are distinct from the older sub-object memset grid but also fail the original zero/register schedule.
- **BsWater_SetTile:** extra row indexes and loop-bound spellings either leave the three latch mismatches or worsen the body. Empty boundaries before the call or at the latch are inert. Three initial head-boundary transformations were invalid C89 and are excluded, not matching evidence.
- **BoatingSchool_Tick after the 3-mismatch close:** pointer, signedness, 64-bit low-half count carriers and volatile store/read combinations do not select EDX for the remaining count triple. Some restore the old 5-mismatch/1163-byte form. Movement calls and all other windows remain matched.
- **Rejected/invalid evidence:** ten implicit depth-return conversions produced W3 narrowing warnings and were excluded; explicit conversions were then measured cleanly and were inert. Four cache-member transformations had malformed scratch declarations and were excluded; corrected Safari/Barrels/Plane forms were inert. One experimental signed left-shift byte extraction could overflow and is excluded on semantic grounds. It was never applied to production.

The exact selection result demonstrates why earlier retirement statements must stay bounded: the same narrowing operation that failed when interleaved succeeds when both full-width stores precede both byte copies. Do not treat an original-sized disassembly slice as proof of a complete body; several longer selection candidates end that slice before RET.

## Second continuation checkpoint

- **Independent symbol checks passed for all four closes.** COFF relocations were mapped to their original instructions, retaining the actual symbol identity and addend: 16 references in each water-selection twin, 11 in BsWater_SetTile, and 2 in Pump. The selection byte stores target g_sel_bpos+0 and +1, each twin uses its own station/boat/definition/footprint/find/selection symbols, and Pump reads g_pump_def and calls GetRoadRecord at the original addresses. Evidence: `*_relocations.txt` under the local scratch directory. Normalized equality alone was not used to establish these identities.
- **Animate integer-copy probes:** 12 memcpy/scalar/array/union/bit-copy carriers of the straight-loop counter are all byte-identical to the existing 3-mismatch body. No new production change.
- **Balloon initializer probes:** signed/unsigned 64-bit and double carriers of the eight literal bytes leave the same five-instruction permutation. Combining the name and cursor in one aggregate changes the frame and gives 626; rejected.
- **Boating counter probes on the new 3-mismatch body:** volatile pre/post-increment, unsigned/member reads and operand ordering are inert. Reusing the loop-index variable gives 5 mismatches/1163 bytes. No further production change.
- **Road intrinsic block probes:** self-copies are eliminated; zero-length fills retain extra setup instructions and are worse. Neither supplies the missing six-byte diamond without added code.
- **BNV callback probes:** alternate signed/unsigned scroll-result widths followed by the required short conversion are inert across all five callbacks. Naming the z-sprite field address adds an instruction; byte-base and memcpy store forms are inert. Passing the two dimension-output pointers as one value changes allocation/frame use and fails to improve any full body. All surrounding exact bodies continue to pass.

These rejected candidates are mechanism tests, not recovered source. Only the ordinary coordinate-pair source from the successful water-tile family was applied. No assembly, changed compiler flags, changed shared declarations or altered executable bytes were used to force a match.

## Third continuation checkpoint

- **Boating queue-index liveness:** using the already-known index to reassign slot 4 still emits an extra assignment or changes the count's destination to EAX, with 288–297 mismatches. Identical if/switch arms over the known index are inert (3); duplicated count increments under memory-based guards add code and ESCAPE. The original ECX/EDX count triple remains the only residual.
- **Space Tower raw base-coordinate pairs:** grouping or naming both class-base reads changes allocation but does not select the original base-y register. Variants with a volatile final sum and alternative pointer-read volatility also fail to improve the corrected full body. No candidate replaces the seat-correct production version.
- **BNV projection and stores:** computing an unscaled difference/sum pair before both multiplications worsens the four mechanical callbacks tested (329–382); the initial Carousel transformation did not match its `sy2` spelling and was excluded before compilation. Volatile z-sprite or seed-coordinate stores were measured on all five callbacks and did not improve any full body.
- **Animate post-increment:** forming the destination pointer before consuming `j++` in the y product is well-sequenced C, but all four placement/operand-order forms retain the three original mismatches. Reusing k or moving the increment into the x product changes loop allocation and is worse.
- **Jungle argument grouping:** materializing the AddBasicObject arguments in a local pair is inert with the existing scalar call (15 with memset, 102 with plain clears). Passing that pair by value changes allocation and gives 53–140; no close. The original two pointer arguments and existing body remain intact.

The scratch runner now enforces the increased exact counts (bswater 4, ridecb3 8, screencb 15, ridemisc3 12) for subsequent full-file candidate audits, while preserving the original baseline-count file as evidence.


## Fourth continuation checkpoint: escaped seed lifetime

- **An address-taken BNV position declared at function scope is treated as potentially escaped throughout the loop. Put it INSIDE the case that owns it.** This frees pre-call position stores from false alias dependencies. Spider's mount seed alone removes 10 mismatches, its departure seed alone removes 3, and both reduce 15 -> 2 at 376i/1228B. The same change takes Safari 132 -> 2 at 402i/1292B, including the entire scratch-register rotation; both scopes are required because mount-only changes case-2 tail merging. No instruction or stack slot is added.
- **Plane's two BNV seeds AND its address-taken Offset are case-local.** This repairs the ascending departure short loads and replaces the extra-byte LEA with the original SHL. Moving the departure flag update after the seed stores then lets both Offset reloads precede it: 19 -> 2 and 1254 -> 1253B, with all 387 instructions retained.
- **Remove compensating pointer caches after fixing the escaped object's lifetime.** Safari, Plane, Barrels and Carousel can use direct person reads again with no score penalty. For Barrels, restore x-before-y departure stores; the old reversed order was a workaround for the overly broad position lifetime. Its complete departure now matches, 19 -> 17 overall. Carousel additionally needs a separate projected sy and the existing two-step sy2 accumulation/barrier; 29 -> 7 with both seeds local and the cache removed. All frames and true body lengths match their originals.
- **Remaining common calculation:** Spider 73/75, Safari 89/91 and Plane 72/74 are exactly the same pair: our subtraction uses the long-lived y register before the origin load, while the original subtracts from the origin scratch register and prepares the depth argument earlier. Simply using one y variable fixes that arithmetic shape but rotates three callee-saved registers and hoists screen.x; it is not a full close.
- **Scoping other locals is not the lever here:** moving screen, spill, tw/th, or their combinations inside Spider's mount case is inert with both the two-variable (2) and one-variable (34) calculation. Named scroll/delta locals, alternate return wrappers, unsigned y types and arithmetic reordering do not close the remaining calculation. Wide y values are worse; some exceed the original extent.
- Other measured rejections: volatile person reads did not improve the old callbacks; depth scalar locals, scalar calls through float-pair fields, and equivalent immediate argument-bit diagnostics were inert. Wider boat products fold back to the same three mismatches. Rotate intrinsics keep the desired MOV/IMUL order but also emit ROL/ROR by zero, adding an instruction; rejected. An equivalent zero-conditional counter is inert. Queue empty-slot liveness variants either fold to the same three mismatches or add/change code. Road's pair-valued call arguments are inert or worse, and zero-length intrinsic/compare arm probes do not produce the missing zero-cost branch.
- Excluded invalid candidates: two initial Barrels scope probes targeted case 0, but its mount seed belongs to case 1, producing undeclared-pos errors; corrected case-1 probes were measured. Three rotate probes put a pragma inside a function (illegal in VC6); the later valid tests use normal /O2 intrinsic recognition. None of these failed candidates counts as matching evidence.

Every landed change was recompiled with /W3 and audited in its complete file. The four mechanical changes preserve 43 exact neighbors; Carousel preserves all 8, including the newly closed Balloonz. The remaining five new closes are untouched. No compilation flags, shared prototypes, global identities, or existing exact bodies changed.


## Fifth continuation checkpoint: all five BNV callbacks exact

**Separating the multiplication and shift is insufficient unless BOTH sides have a zero-code consumer.** With the seed lifetimes fixed, Spider gives:

| Early consumers around `sy2 = sy >> 9` | Strict | Audit-slice bytes |
| --- | ---: | ---: |
| Neither | 34 | 1228 |
| Before shift only: `if (sy) { }` | 343 | 1232 |
| After shift only: `if (sy2) { }` | 34 | 1228 |
| Both | **0** | **1228** |

The successful sequence is `sy = (wx + wy) * th; if (sy) { } sy2 = sy >> 9; if (sy2) { }`. After the x-scroll calculation, use `sy2 += g_map_cfg->oy - Get_YScroll(); if (sy2) { }`. All three empty-if consumers emit no instructions. The first two retain the intermediate arithmetic values until register allocation/reassociation has made the original choices; the last retains the established pivot-subtraction grouping. This corrects the original callee-saved register assignment AND the scratch-register y subtraction, so the depth argument pushes fill their original slots. The exact same source transformation closes Spider 376i/1228B, Safari 402i/1292B, Plane 387i/1253B, Barrels 362i/1148B and Carousel 378i/1225B.

The former notes' implication that the original one-variable arithmetic and register assignment were mutually unreachable is superseded. Their individual early consumer probes did not test this pair together after correcting the BNV seeds' lifetimes. No assembly, compiler flag changes, new helper calls or shared declaration changes were used. The exact bodies retain the original uninitialized seed z field and existing spill object.

The whole-file gates now count 47 exact functions in mechrides and 9 in ridecb3. Every one of the five complete compiled extents ends at its original RET; no padding or jump-table bytes were counted. An independent COFF review checks symbol addresses against their declarations/function markers, checks the `%02d` literal bytes against the original, and records every relocation in `*_f4_relocations.txt`. Obsolete exhaustion notes were replaced with concise closure notes; shared trailing address annotations were preserved.

Additional negative transfer evidence before this close: Space Tower's base-array and tile-coordinate scope changes do not repair its head; reusing base[0]/base[1] for the raw origin adds frame/code differences. Boating School's launch-source scope, packed-key scope and queue-counter scope are all inert at 3. Spider's byte-array/union spill representations and dynamic scalar/Offset initializer scopes are inert; these are not the lever. None of these candidates was landed.


## Sixth continuation checkpoint: transfers to the six residuals

The ten exact functions remain untouched. No further production-body change was accepted in this checkpoint. The following new families were compiled in complete candidate files and audited with the increased 47/9 exact-neighbor safeguards; warning/error candidates are excluded.

- **The projection barrier is specific to the expression shape.** Applying empty-if consumers before and after an Animate counter copy or product still gives 3; adding a distinct unsigned or enum counter with signed comparisons/conversions also gives 3. A counter initialized by memcpy is inert, while memset initialization changes the loop/frame extensively. These are distinct from the successful multiply/shift split in the five BNV callbacks.
- **Boating School's known-zero queue value does not reserve ECX for the count update.** Keeping it through the enqueue/shuffle join using additions, XOR or a conditional still leaves the same count triple. The only further change is a shorter register-zero store in the shuffle arm (4 mismatches, 1160B), where the original has an immediate zero. An empty-if use gives the unchanged 3/1164B. One-iteration loop wrappers around the update are also inert, except the post-decrement while, which adds code and escapes the original extent.
- **Space Tower's new two-stage pointer/field consumers do not reproduce the projection success.** Separate pointer, field and final-sum consumers, reused base-array fields, and three legal placements of a consumed y value before a volatile sum store give 147–170 or worse. Some retain four slots but still add/change instructions. Moving individual iteration locals or all nine together inside the rider loop is byte-identical to the current 138 form. None buys the original ECX base-y value and its dependent tile-y reuse.
- **Jungle Cruise still lacks the original second zero definition.** Single-member argument views are inert. A null-dependent memset join can reduce the plain-store candidate to 16, but does so with an extra dead argument/home store, not the original XOR ECX. Identical plain-store branches also give 16 with the wrong zero/register sequence. One-iteration loops do not split the zero web; the post-decrement while adds unrelated code. These candidates do not replace the existing 15/438B body.
- **Road's two-edge counter forms still change the earlier allocation.** A volatile null-edge source with a second counter reports 64i/157B only because the original-sized slice stops at POP before RET. Its actual body is longer and its initial count occupies EBP, forcing the y-minus-four value into a parameter home. Consumer barriers, one-member counters and one-iteration increment wrappers do not recover the original per-edge reloads. No apparent slice-length match was accepted.

This checkpoint adds negative transfer evidence only; it makes no claim that the remaining six are exact or mathematically impossible to match. Scratch side-by-side listings and audit logs retain each measured candidate. The production state is still ten exact targets, six WIP targets, 122 exact functions across the nine owned files.


## Seventh continuation checkpoint: remaining source-shape searches

The post-close search now contains **243 clean, fully audited candidates**, plus 3 excluded warning/error candidates (f5/f6/f7 labels). None improves the six remaining production bodies. The ten exact targets and the Space Tower seat correction are unchanged.

- **Four-byte return views do not shift the allocation:** struct, union, named result, and unsigned-bit return types for the existing lookup/allocation calls leave Tower 138, Jungle Add 15, Road 27, and Boating Tick 3. These are scratch-only, ABI-size-preserving probes; no extern declaration or new production call was added.
- **The road diamond does not survive a syntactic loop boundary.** Five loop-exit/goto variants around the third lookup and three whole-body single-pass loops are all byte-identical to 27/62i/151B. Algebraic null-arm identities and qualified value casts also fail to create the second reload.
- **Jungle's parameter-clear and intrinsic-result routes do not create the second zero web.** Preserving the original o or p parameter and clearing its copy, with scalar or aggregate carriers and three placement choices, gives 102. strlen of an empty literal folds into the existing single zero (102/96); memcmp/strcmp introduce code and escape, and consuming memset's returned pointer adds a home/load sequence (123). No such construct was adopted.
- **Animate's output-field temporaries either fold to the old product or leave a real store.** Writing the counter to the destination before consuming it adds a store and rotates other registers; writing only the product first is inert at 3. Signed/unsigned 32-bit bitfield copies, including memcpy and paired consumers, are all inert. Narrow counters restricted to the straight loop add a separate trip-count/conversion sequence (63); a plain pre-increment condition is inert, while copied conditions add code. Naming the direction factor with consumers yields 7 or 172, not the desired register-to-memory product. Float operand-order and explicit-float forms stay at 3; double forms change code and bytes.
- **Boating's count remains ECX under the new forms.** Addressing the count through the known i==5 value or the known-null queue pointer, and adding qualified value casts, all give 3/1164B. Negated-complement increment spelling is also inert; other arithmetic formulations add instructions or return the previous 5/1163B form. The three-term signed multiplication diagnostic was not accepted as an unrestricted semantic rewrite because it can overflow outside the game's ordinary counter range.

All figures above are candidate comparisons, not added matches. The authoritative production result remains **10/16 exact**, with **122 exact functions preserved across the nine files**. Reaching 16/16 still requires a verified source-level solution for the Animate multiply operand, Boating count register, Road branch reload, Jungle zero rematerialization, and Tower head allocation. These searches do not prove those solutions impossible.


## Eighth continuation: push and recover Road's missing branch

The authorized branch push succeeded. Continued source work reduces **Road_FindDiagonals from 27 to 2 strict mismatches**, with its complete return and one original counter home preserved. It remains `WIP-FUNCTION`, 96.9%, 64/64 instructions and 159/157 bytes; all eight exact neighbors still audit `[OK]`.

- **A branch result copied back AFTER the intervening output store retains both counter reload edges.** An explicit temporary initialized on both third-lookup arms, followed by `out[5] = r` and then `n = result`, keeps the original branch structure without a second stack home. Copying back at the join gives 51 mismatches; delaying it past the fourth lookup gives 29. The after-store shape first gives 17, then 14 with the null arm first.
- **A free volatile read at the first out-parameter load restores EBX/EBP.** This takes the after-store shape from 14 to 2, with every other instruction, all four callees and all resolved stack homes agreeing. The remaining two instructions are the original `mov edi,[esp+10h] / inc edi` versus `mov edx,[esp+10h] / lea edi,[edx+1]`. The latter is two bytes longer. Both bodies have their real return at index 63; no padding, missing tail or uninitialized counter is counted as a match.
- **Counter spelling alone does not close that final increment.** Prefix/postfix/copy increments, dead old-counter assignments, intermediate consumers, 32-bit scalar/aggregate views, pointer/copy views and clean wide-temporary probes do not improve the new two-instruction minimum. Volatile counter reads fold the wanted branch away or add storage. Increment loops and wider result lifetimes change the surrounding register allocation. Rejected scratch transformations and no-op whitespace substitutions are not evidence.
- **Boating School's free pointer-definition reads do not transfer this lever.** Volatile reads of the station head/next, instance next/bloke keep three mismatches; the class and instance-head definitions give 12 and 11 respectively, and a volatile count guard gives 299. All clean candidates pass the full file audit.
- **Jungle Cruise's zero-window probes do not recover its second zero register.** One volatile edge among the scalar zero stores, or redundant zero assignments, worsens the body. The recorded 15-instruction residual remains.

The road change supersedes the older claim that a legal zero-cost branch-retaining source shape cannot exist. It preserves all four diagonal lookups, conditional output writes and the count on every path. No already-exact body or shared declaration was edited.


## Ninth continuation: remaining compiler constraints and final checkpoint

**Still ten of sixteen exact. No further body change after the Road improvement.** Fresh full-file audits of all nine owned files pass with 122 exact functions, and all nine compile cleanly under the required flags. A comparison against the scope base, with only the sixteen allowed bodies and comments removed, confirms that every other C token is unchanged: no exact neighbor, shared declaration, signature or helper was altered. The selection twins include all three return sites in their 129i/374B extents; Pump includes both return sites in its 33i/85B extent.

The main branch has advanced to `3fa59569`. Its new DECOMP findings, consolidated `docs/LEVERS.md`, Scope K's aggregate-counter close and Scope L's relocation guidance were read without changing or merging the main checkout. The applicable new source families were tested on this worktree. They do not close the remaining six functions.

- **Road's after-store copy is essential; its final increment remains coupled to branch retention.** Moving the result to function scope, widening/narrowing its lifetime, reusing either coordinate parameter with a preserved original coordinate, and same-width union edge views all retain the two-mismatch result or worsen it. Moving the copy into the output guard's two arms changes allocation; a plain copy after the guard is the retained form. Integer and four-byte aggregate call-result views are inert. Zero-sized memory intrinsics and their returned pointers either disappear, add operations, or alter the frame/control flow. Degenerate nested increment branches also fail. Counter reads that yield the desired load/increment generally allow the compiler to hoist the common load and remove the original two-edge branch again. These are bounded observations, not a proof of impossibility.
- **Boating School has a new, isolated source of the desired EDX counter.** Split its count-full and last-slot-occupied rejection checks into two early failure arms, each spelling the existing RemoveBlokeFromRide call. The enqueue becomes the original `mov edx,[edi+14h] / inc edx / mov [edi+14h],edx`, but the failure prefixes no longer have the original shared placement: the complete candidate has 12 strict/rb mismatches and is three bytes longer. Keep the committed three-mismatch body. Failure-first compound guards, named guards and loop-break variants retain three; a nested success spelling costs 283. Caller-local argument pairs, shared/named failure-class pointers, pointer consumers and free volatile class reads do not recover the shared failure prefix while preserving the corrected counter. Dropping the count shim on the split-guard candidate also regresses. This is evidence to revisit the failure-block allocation/merge relationship, rather than repeat counter spelling alone.
- **Boating's small-object copies are not a close.** Field, whole-object and memcpy spellings of the count update give five mismatches and 1163 bytes: the original three counter-register differences plus a changed register for the rejection call's class argument. High-word carriers add storage or operations.
- **The Animate twins' existing rank constraint survives branch-derived identities.** In the straight-through branch, adding/xoring/shifting by the known-zero `from == to`, selecting between equivalent index values, or multiplying by its complement leaves the same three mismatches. Tests based on the earlier -1 guards change the surrounding allocation and are rejected. Sound range assumptions around narrow casts do not remove the extra sign extension: the short-cast candidate remains one mismatch and one byte too long, not exact. No counterpart change was landed in either twin.
- **Space Tower's byte-read/field-read variants do not resolve the head.** Named base-y values with separate volatile field reads, pointer/integer/float-bit views, sum/restore shapes and an updated def pointer do not achieve the original head. Crossing three base-y placements with x/y byte-read barriers and the existing sum home produces 145–164 strict mismatches, rather than a close. All builds preserve the repaired seat offset and all 47 exact neighbors.
- **Jungle Cruise's second zero remains missing.** Sweeping the existing object-parameter read through the clear/link/publish seams, with and without a value consumer, does not reproduce the two zero registers. Zero values derived from the already-successful allocation test fold into the existing zero, while clearing a copied allocation pointer adds storage/operations. Neither family improves the retained 15-mismatch body.

Every successfully compiled experimental source was audited as a whole file. Failed source transformations, ineffective text substitutions, an incorrectly escaped zero-copy literal and diagnostic high-word probes that read an uninitialized low half are excluded from the matching evidence; none was landed. Experimental sources and logs remain local under `scratchpad/scope-f/` and are not published.

## Tenth continuation — asymmetric reads and explicit failure joins

**Still 10/16 exact; exact completion has not been achieved.** The production C is unchanged from the published Road improvement. The nine-file validation above therefore still applies: 122 exact functions, clean required compiler flags, and unchanged neighboring bodies/declarations. The additional f13–f20 experiments produce no exact candidate and no production improvement.

- **Road's two remaining instructions survive conditional-result lowering.** Conditional initialization, subsequent assignment, `const` and `register` locals all give the same two differences with the null arm first; positive-arm-first forms give five. A volatile source collapses the branch again. Whole-object counter copies, mixed plain/volatile/intrinsic reads on opposite arms, and empty consumers around those reads do not retain both the desired increment and original branch. Float-bit or split-word reads add operations. Duplicating the output guard into both third-lookup arms also changes the surrounding allocation and does not recover the original shared output store.
- **Boating's failure-block layout and count allocation remain coupled.** Explicit jumps into either failure block give 60; nested shared-label forms return to three. A source-level shared removal call, with class/instance locals or an argument pair, gives 288 and changes the shared tail. Separate four-byte aggregate arguments are inert: three on the retained body, twelve on the split-failure body. Volatile queue-pointer reads, queue-store barriers, four-byte queue copies and consumers of existing guard values do not close the count register. Removing the count-read shim returns five differences. An explicit queue-search cursor with the original count address is also inert when the increment order is preserved.
- **Space Tower still needs the original head allocation.** Reusing the seat variable, dead element parameter or unused base-array component for base-y, and grouping base-y with tile-y or the final sum, yields 143–152 mismatches. These forms do not remove the extra allocation cost. The corrected seat release remains intact.
- **Jungle's known-zero dependencies do not create the second zero register.** Deriving the object argument through already-cleared rider/count/route fields changes scheduling or introduces reloads; none improves the retained fifteen. Both scalar rider stores and the existing memset were checked.
- **Bounded expressions do not close the Animate/Road pair of counter problems.** Supported absolute-value and conditional-bound forms either retain the current three/two differences or add work. `__min`/`__max` are not supported intrinsics in this environment: their warning-producing probes were rejected, not treated as valid builds or source evidence.

Every clean experimental build received its whole-file audit with the established exact-count guard. The negative-index queue-cursor diagnostics are excluded from reconstruction evidence because their counter access crosses the queue array boundary; no such access was landed. The conditional-result script's initial whitespace assertion failure produced no candidate and was corrected before its sweep. No scratch source, experimental helper, changed flag, or speculative exact marker is published.

Remaining strict differences are **BsBoat_Animate 3, JcBoat_Animate 3, SpaceTower_Activate 138, JungleCruise_Add 15, Road_FindDiagonals 2 and BoatingSchool_Tick 3**. These are measured residuals, not proofs that no valid C reconstruction exists. Further exact progress requires a new source-level mechanism beyond the families recorded here.

The authoritative current measurements remain the table at the top. **100% exact matching has not been achieved:** both Animate twins (3 each), Space Tower (138), Jungle Cruise Add (15), Road (2) and Boating School Tick (3) remain WIP. No complete marker or completion claim is assigned to them.

## Eleventh continuation — Road_FindDiagonals closed; a layout lever for spilled counters

**Road_FindDiagonals is exact: 64/64 instructions, 157/157 bytes, audit `[OK]`, marker `// FUNCTION: LEGOLAND 0x00413450`.** ridecb5.c audits PASS with nine exact bodies (the eight neighbours unchanged, BoatingSchool_Add still EXHAUSTED at 8). The two-variable `result` form and its volatile `out` read are gone; the body is the cardinal twin's plain shape with one change at the third lookup: `if (!r) r = 0; else n++;`.

The mechanism was measured in steps, each a clean full-file candidate:

- **Reconstruction: the original is `if (r) n++;` with a LAZY reload.** `n` is spilled across the first three calls (ebx/ebp/esi/edi hold `out`, `y-4`, `x+4`, `y+4`). The original reloads `n` into edi in EACH arm of the third diamond (`mov edi,[esp+10h] / inc edi` on the taken arm, `mov edi,[esp+10h] / jmp` on the other). Written plainly, VC6 reloads eagerly — `mov edi,[esp+18h]` scheduled before the `add esp,8` — and lays out `je` over a bare `inc edi` (27 X, 62i/151B). Keeping `y+4` alive into the fourth call, named `yp`/`ym` locals, `unsigned`/`short` counters, declaration order, pointer aliases and a one-element array or struct counter do not change that (27–48 X).
- **VC6 never emits `mov dst,[mem] / inc dst` for `dst = mem + 1` into a callee-saved register**: synthetic probes (global, escaped local and parameter sources; copy-then-increment and `+1` spellings; with and without a diamond) always give `mov scratch,[mem] / lea dst,[scratch+1]`. So the original's `inc edi` is `n++` performed on `n` itself after its home moved to edi, not a second variable. This is why the two-variable `result` form was stuck at 2 X: its taken arm is inherently a `lea`.
- **A non-empty else arm at layout time is the whole lever.** With any real statement in the `r == 0` arm, the taken arm becomes the original's lazy `mov edi,[esp+10h] / inc edi` and the allocator puts its compensation copy on the other edge. A volatile self-read there does it but costs its own load and a store (`n = *(volatile int*)&n`: 24 X, 64i/161B; bare read: 24 X, 160B). Empty-if consumers (`if (n) {}`, `if (out) {}`, `if (x) {}`), `__assume`, aggregate and union self-copies, `n ^= 0`, `n * 1`, `n / 1`, `~~n`, `-(-n)`, `memcpy(&n,&n,4)`, `x = x`, `out = out` all fold BEFORE layout (27 X). `__noop` is a real call in this VC6.
- **`r = 0` in the known-null arm is kept by the front end and emits nothing when that arm is FIRST.** `if (!r) r = 0; else n++;` and `if (r == 0) { r = (RoadRec*)0; } else n++;` are both exact; the same statement as the SECOND arm (`if (r) n++; else r = 0;`) emits `xor eax,eax` (24 X, 158B). The `!r`/`r == 0` spelling and the cast are inert. The volatile `out` read is not needed with this shape.
- **Transfer.** This is the general form of "an else arm is exiled iff it ends in an unconditional jump": the arm order and a kept no-op decide whether a spilled variable's reload lands in the fork block or on the edges. It should be tried wherever a residual is a reload placed before a branch instead of after it.

Tooling for this continuation lives in the session scratchpad (a side-by-side dumper, a variant runner that compiles a scratch copy of the file with one WIP body replaced, and an object disassembler for synthetic probes); nothing under `tools/` was changed. The contract's environment paths do not exist on this machine; the compiler is `/Users/systemadmin/Downloads/alpha team/alphateam/tools/wibo-msvc/cl` and the Python is `/opt/homebrew/bin/python3` (capstone installed).

### JungleCruise_Add closed: a constant-count fill loop is its own zero register

**JungleCruise_Add is exact: 141/141 instructions, 438/438 bytes, audit `[OK]`, marker `// FUNCTION: LEGOLAND 0x00434f90`.** ridecb9.c audits PASS with twenty exact bodies (nineteen neighbours unchanged). This was a reconstruction error, found by a subagent working in a scratch copy with 74 clean candidates, none repeating the recorded families.

- **The five queue slots and the three rider slots are cleared by two constant-count `for` loops**, `for (x = 0; x < 5; x++) st->blokes[x] = 0;` then `for (x = 0; x < 3; x++) st->riders[x] = 0;`, not by five scalar stores plus `memset`. VC6 SP3 lowers a constant-count array fill as a fill idiom: six or more elements become `rep stosd` (CoasterShades_Init), five or fewer become inline stores, and the idiom materialises ITS OWN zero register with base+displacement addressing and no `lea`. Each fill loop is a separate zero node created after CSE, so two loops are two zero webs: the queue's zero coalesces with the function-wide eax web, the riders' zero is the original's `xor ecx,ecx` at index 42; with that zero out of eax, the first web dies at `blokes[4]`, the object argument lands in eax and the head load in edx, which is the whole 32..53 window and the post-call rotation.
- **Isolated evidence** (scratch `fill.c`/`fill2.c`): a 5-fill is `xor ecx,ecx` plus five `[eax+disp]` stores; a 5-fill followed by a 3-fill is `xor ecx,ecx / xor edx,edx` plus eight base+disp stores; 6-, 7- and 8-fills are `rep stosd`; `memset(s->r, 0, 12)` beside them is `lea edx,[eax+0x1c]` plus stores through edx, which is why the memset family sat at 15.
- **Load-bearing:** both groups as fill loops (queue plain plus riders loop 96; queue loop plus riders plain 19; memset on either group 93/101); queue loop before riders loop (9 reversed); the six scalar seeds before both loops (after 103; struct-order interleave 12; count last 3); link and publish after the riders loop (link between the loops 8). **Inert:** the loop variable and its scope, the bound spelling (`<`, `!=`, `<=`, sizeof-derived), braces. Pointer-cursor fills (123, escapes) and count-down do/while (96) are not recognised as the idiom.
- **Rejected on the way:** volatile parameter copies at four IR positions (99–125); memset with the first web live across it (101–103, produces the second zero but always with `lea`); degenerate branches and phi-of-zeros (16–130); by-value aggregate parameters (15–140); late-folded zero expressions (102/108, all merge into one web).
- **Corpus scans:** this is the only function in the binary with a second `xor r,r` inside a zero-store run, and the only pair (with BoatingSchool_Add) where an argument load is threaded through a record-initialisation run before a call. BoatingSchool_Add is EXHAUSTED and was not touched; its `q[0..4]` run and `o` threading are the same shape, and the fill-loop spelling is the obvious first try for whoever owns it. The `memset` prototype and `#pragma intrinsic(memset)` above the note in ridecb9.c are now unused by every body in the file (the file audits identically without them) and were left in place because they sit outside the body and note.

## Twelfth continuation — both Animate twins closed; 14 of 16 exact

**Scope F now stands at fourteen of sixteen exact.** Road_FindDiagonals, JungleCruise_Add, BsBoat_Animate and JcBoat_Animate all closed in this continuation, on top of the ten already exact. The nine owned files audit PASS with **126 exact functions** (was 122) and compile clean under `/W3 /O2 /Gy /Gd`. No exact neighbour, shared declaration, extern type or tool was changed. The two still open are **BoatingSchool_Tick (3 strict)** and **SpaceTower_Activate (138 strict)**; their markers remain `WIP-FUNCTION` and their notes now carry everything eliminated here.

### The Animate twins: a remote float-precision lever

The residual was never in the straight loop it was measured in. Naming the **u-turn** loop's x product in a `float` local makes the straight loop, 170 instructions later, emit the original's operand rank `mov r,j / imul r,[table]` instead of `mov r,[table] / imul r,j`. No instruction and no stack slot is added. Measured on BsBoat_Animate:

| u-turn loop spelling | strict |
| --- | ---: |
| no local (previous body) | 3 |
| name the Y product instead | 3 |
| name BOTH x and y | 283 |
| `double` instead of `float` | 3 |
| name `(dx*j)*16.0f` | **0** |
| name `dx*j` | **0** |
| name the whole sum `(dx*j)*16.0f + x0` | 3 |

So the local must carry the **x** expression, must be **`float`**, and must be the **product**, not the sum. The local is a front-end effect only: no store, reload or conversion is emitted for it and the body is byte-for-byte the original either way, so the mechanism is the typed temporary VC6 carries into its later ranking decision, not a runtime rounding. That `double` is inert while `float` closes both bodies is the evidence that the type, not merely the extra name, is what matters. The identical one-line change closes JcBoat_Animate index for index, as both notes predicted. The earlier "operand-rank floor" was a floor only for the families that had been tried, all of which respelled the straight loop.

**Transferable rule:** when a residual sits inside a loop whose every operand already matches, suspect an earlier loop over the same data. An x87 value rounded to `float` at the right point in the schedule changes integer operand ranking downstream.

### BoatingSchool_Tick — still 3, more families eliminated

The residual is one register on the enqueue's count triple (EDX in the original, ECX here). Newly eliminated: zero-code extra IR temporaries before the count load (a copy of the bloke, a copy of the slot, both, a named `&st->count`, a named `q[4]`, empty-if consumers) are inert, unlike the pointer-naming lever elsewhere; moving the volatile shim to the guard, the store, the compare or case 1 costs 5 to 299; remote respellings of case 1's decrement are inert, so the two count sites are not one problem; swapping the `i == 5` arms costs 82 to 299; keeping the loop index live into the common tail by writing the seat offset as `g_bs_seat_ofs[1 - i]`, which is exactly `[-slot]` on both paths, costs 278 to 299, so the index is genuinely dead; the split-guard family is confirmed at 12 and does buy the original's EDX triple, but emits the count-full failure block inline and takes EAX for its class load; adding the Road no-op lever to it costs 300; a goto-shared failure block placed after the enqueue gives 9 with the count back in ECX; kept no-op assignments inside the enqueue arm are inert or cost 293 to 297. The two register states remain not simultaneously reachable.

### SpaceTower_Activate — a correct structural finding that does not yet close it

Case 3 does **not** reuse a saved `tiley`. The original materialises the tile address once in the head (`lea eax,[ebx+0xc]`, index 30), keeps it in EAX across the jump-table dispatch, and re-reads the y byte through it in case 3 (`mov dl,[eax+1]`, index 98); `tilex` really is a frame-homed int, reloaded at index 89. Writing case 3's y term as a direct field read reproduces that pointer liveness and the case-3 shape, but VC6 then folds the head's y read to `[ebx+0xd]` and schedules it before the `lea`, where the original reads it after through the same register: 188 strict. Eliminated on top of that form: base_y read first into a local, both base fields into locals, an accumulate spelling, a volatile base_y, swapped sums, an explicit y temporary, a byte-pointer read, a `key >> 8` read, and empty-if or volatile barriers on the x sum, all 188 or worse. Also eliminated: a volatile pointer copy of the definition hoisted to the top of the loop body in five placements (156 to 170), and every plain head spelling, which VC6 collapses to a single definition load at 163. The retained volatile re-read is still the only construct that buys the original's second definition load, and it can never schedule early enough because a volatile read cannot cross the tile-x store. Closing the head needs a plain construct that keeps both loads and lets them hoist above that store; none was found.
## Thirteenth continuation — BoatingSchool_Tick closed; 15 of 16 exact

**Scope F stands at fifteen of sixteen exact.** The nine owned files audit PASS with **127 exact functions** and compile clean under `/W3 /O2 /Gy /Gd`. Only SpaceTower_Activate remains, at 138 strict; it keeps its `WIP-FUNCTION` marker.


**BoatingSchool_Tick is exact: 358/358 instructions, 1164/1164 bytes, mismatch 0, no escapes, audit `[OK]`, marker `// FUNCTION: LEGOLAND 0x0041a720`.** ridecb5.c audits PASS with ten exact bodies (the nine neighbours unchanged, BoatingSchool_Add still WIP at 8) and compiles clean under `/W3 /O2 /Gy /Gd`. Scope F now stands at fifteen of sixteen exact; only SpaceTower_Activate remains.

The residual really was "one register", and the cause was neither the guard nor a CSE — it was the shape of the block the increment sits in.

- **VC6 SP3 gives a basic block's LAST-DYING scratch temporary `ecx`, and earlier-dying ones `edx` (then `eax`, when `eax` is not carrying a live variable).** Measured directly inside the disputed block: `st->count++; st->take++;` emits count=`edx` / take=`ecx`, and `st->take++; st->count++;` emits take=`edx` / count=`ecx`. The choice is **positional within one block**: it is not a rotation (a third guard condition that consumes `ecx` immediately before the increment leaves it at `ecx`), not liveness (nothing is live in `ecx` across the block in either body), and not a value-numbering effect (the falsified CSE hypothesis in the old note). A temporary in a **neighbouring** block changes nothing — `st->take++` added at the top of the shuffle arm, in the queue-full arm, at the top of the join, or before the `i == 5` test all leave the count in `ecx`.
- **The lever: write the shared tail out in BOTH arms.** Case 0's walk-to-the-slot tail (`b->flags |= 8` through `NewDirForAction`) written once after the `if` leaves `st->count++` as the last temporary of its own four-instruction block, so it takes `ecx`. Written out at the end of the enqueue arm *and* the end of the shuffle arm, the tail's own temporaries follow the increment in the same block, the increment stops being last, and it takes the original's `edx`. VC6 then cross-jumps the two copies back into the single block at 0x41a82d — the copies hold two calls, so by the tail-duplication threshold they are jumped to rather than kept — and the emitted stream is identical to the original index for index. Worth **11 → 0** on the clean body.
- **Three `volatile` shims and one named pointer came out with it**, exactly as every earlier note predicted they must. The count-read shim `st->count = *(volatile int*)&st->count + 1`, the `*(volatile unsigned short*)&b->action = 7` store and the named `Pos* world` in case 0 are all inert under the duplication and are gone; **the body now contains no `volatile` at all**. Re-adding the count shim on top of the duplication costs 288 — the two constructs are mutually exclusive, which is why every previous pass that kept the shim was searching a space that could not contain the answer.
- **Partial duplication does not work**: `b->flags |= 8` alone is 3, flags plus the `b->tx` line is 291 (escapes). The whole tail, up to and including the `break`, has to be in both arms.
- **Equivalent exact spellings** (recorded so they are not re-derived): the guard as an early exit (`if (st->count == 5 || st->q[4] != 0) { RemoveBlokeFromRide(g_bs_cls, inst); break; }` ahead of the enqueue, tail duplicated in both arms); and the tail written inside the enqueue arm with the queue-full arm unindented after it and the shuffle path falling out to a second copy. Non-working neighbours of the same idea: the split-guard form with the call written twice (12), `if (i != 5) shuffle else enqueue` in three-way form (82–297).
- **Still load-bearing**, re-measured on the exact body: the scope-local `MoveLineFn` function-pointer type (two by-value `Pos` at the five `CalcMoveLine` sites, using the original's ABI without touching the shared five-scalar extern) is worth 17 at the case-5 site, 42 at case 4, 157 at case 3, 288 at either case-0 site, 265 for all five. The animation loop's failure-first `LLSSetFrame` arms are unchanged.
- **Transferable rule.** When a residual is a single scratch register on a read-modify-write, count the temporaries in that basic block: if ours is the last one it will take `ecx`. Neighbouring blocks are irrelevant; the fix is to change what else lives in *that* block, and duplicating a shared tail into both arms of the fork is the way to do it without changing a single emitted instruction — VC6 merges the copies back. This is the allocation-side twin of the Road_FindDiagonals lever (a kept no-op changing which edge a reload lands on) and of the tail-duplication threshold already in DECOMP.

### SpaceTower_Activate: not closed, and two earlier claims withdrawn

Retained at 138 strict, which is the best result that also holds the original's frame and real length. Register-blind the residual is only thirteen instructions. Two claims recorded in the previous continuation were wrong and are corrected above the marker: a plain head does **not** collapse to one definition load (plain emits two, at indices 22 and 30; the volatile head two, at 22 and 31; the original's two are adjacent at 22 and 23), so the number of loads was never the problem, only their position; and the "direct case-3 read scores 188" figure holds only for the `tile->b.y` spelling, while `RIDE_TILE(r)->b.y` is a different lever that reaches strict 100 to 102 by paying a fifth frame slot, a compensating error that must not be landed.

Two findings worth keeping. **Two separate volatile pointer reads into two distinct named locals hoist as a pair** and are the only construct in roughly 1200 spellings that reproduces the original's back-to-back reload pair, with the correct frame and the rest of the head's shape; a fixed three-web register rotation then keeps strict at 145 to 152, and thirteen rotation levers are byte-identical. And **VC6 spills the loser of a two-web contest for the last register, decided inside the defining block, not by reference counts**: stubbing out the definition's other uses, or deleting the x sum from the head, both leave the spill unchanged.


## Fourteenth continuation — pulled five closes; Tower head allocation improved

**15/16 targets remain exact; the sixteenth is not closed.** Pulled `origin/scope/F-fable` through `e9e1f914` and independently rebuilt/audited all nine files: bswater 4, bswater3 8, roads 2, ridecb3 9, mechrides 47, ridecb9 20, screencb 15, ridecb5 10, ridemisc3 12; total **127 exact functions**. Only the remaining WIP Tower body and its own notes were edited. No callees/globals were newly named, no extern types changed, and the corrected `rec->seat[b->seat]` store remains intact.

### The named destination pointer breaks the head rotation

On the previous continuation's two separately read definition pointers, write the y sum through a named local pointer:

```c
RideDef* d1 = *(RideDef* volatile*)&def;
RideDef* d2 = *(RideDef* volatile*)&def;
tilex = tile->b.x;
tx = d1->base_x + tilex;
tiley = tile->b.y;
{
    int* dst = &base[1];
    *dst = d2->base_y + tiley;
}
```

Combine this with **unscaled case-8 sums computed before either target store**. The destination pointer is not emitted and adds no stack home. It changes the head's register assignment: the two definition loads are now EDX/ECX at indices 22/23; tile-x is EAX, x sum EBP, tile EAX, tile-y EDX and y sum ECX. The original 16-byte frame is retained. **Indices 22–24 and 28–88 are exact**, including the entire first action block. Only the order of the three loads at 25–27 remains different in the head.

| Measurement | Pulled body | Landed body |
| --- | ---: | ---: |
| Strict index mismatches | 138 | 136 |
| First divergence | 23 | 25 |
| Head differences (indices 0–41) | 11 | 3 |
| Case-0 differences (42–79) | 4 | 0 |
| Real instructions / bytes | 220 / 696 | 220 / 693 |
| RET-bounded aligned strict edit count | 68 | 18 |
| RET-bounded aligned register-blind edit count | 22 | 11 |
| Index-aligned strict / rb / ob | 138 / 120 / 138 | 136 / 123 / 136 |

The aligned counts measure sequence edits after allowing alignment; they are **not audit mismatch counts or completion percentages**. They exclude the two trailing decodes. The three-byte size decrease is explicitly retained in the report: the candidate improves actual register choices and sequence alignment, but neither its real length nor its audit slice matches the original. Audit remains WIP at 38.7%, marker `// WIP-FUNCTION: LEGOLAND 0x0043bac0`.

A volatile store to the same y-sum home also restores the head registers, but swaps the state comparison and sum store and changes the joined-byte register in case 0 (141 strict). The **plain named destination pointer** avoids those changes (136). Whole-object/memcpy/unsigned sum stores and local aggregate fields do not produce the same rotation. The paired case-8 sums are essential: changing only the destination-pointer/head shape without them gives 160.

### What still prevents an exact body

- **Head scheduling:** ours loads base-y before tile-x/base-x at 25–27; the original loads tile-x, base-x, then base-y. Free base-field or byte barriers change the permutation or add spills. Naming a delayed tile-x home, a base-y intermediate, or a destination for the x sum does not close it.
- **Case 3:** the original stores target-x, then re-reads the tile-y byte through EAX (`xor edx,edx; mov dl,[eax+1]`). The retained local reuses EDX instead. This is the missing two-instruction sequence. Seat/x register allocation and the position of `shr edi,1` also differ. With direct `tile->b.y`, VC6 instead carries the derived tile address as the loop cursor and gives the rider pointer an extra home. `RIDE_TILE(r)->b.y` keeps the rider cursor but spills other values or changes allocation. Those alternatives are not a close.
- **Case 8:** the sums/store sequence is now aligned, but two load-order windows remain. Named origin pairs, x-first reads and other load placements either move the head registers again or retain the mismatch.

### Newly tested families and limits of the evidence

- **Address and lifetime forms:** named destination pointers, field/byte volatility, separately consumed origins, pointer/integer/aggregate value views, delayed tile-x stores, narrow tile-y and seat locals, paired tile/coordinate/cursor locals and case-local variables. The named y destination is the improvement; narrower locals and the lower strict scores they sometimes produce change the original frame or add structural differences and were rejected.
- **The missing reload:** direct and volatile tile-byte reads, an explicit tile-y reassignment after the x store, re-materialized tile pointers before/after FindRecord and at case 3, byte/struct/array pointer views, r/next/tile wrappers and intrinsic pointer copies. These do not retain both the original cursor layout and reload. The direct-field family remains around 209–213 strict with extra storage or an escaped extent; it was not landed.
- **Control flow and action tails:** for/do/goto loop forms, null-exit and early state-continue forms, case-3 seat/dir lifetimes, earlier seat division, an explicit shared-direction label, named/paired/sequential target coordinates and intrinsic pointer transfers. They do not recover the original reload without changing the surrounding body. Moving both coordinate reads before their stores can avoid the cursor cost by eliminating the required reread, so that is not evidence of a close.
- **Two-axis fill loops and case-8 load groups:** constant two-iteration origin/coordinate loops were tested using the fill-loop mechanism that closed Jungle Cruise. They add instructions/spills here. Case-8 x-first, origin-first, local-pair and named-definition forms were also crossed with both reload spellings; none closes Tower.

Every clean candidate was audited with the 47-exact-neighbor guard. Scratch transformations that failed compilation, unreferenced-local controls before correction, and diagnostic representations that do not preserve pointer bits for all inputs are excluded from reconstruction evidence. Logs/sources remain local under `scratchpad/scope-f/` (f21–f44); they are not published. The verified landed checkpoint is f44. The earlier assertion that the retained body had the original's real 696-byte length was incorrect: the **original is 698 bytes**, and neither retained checkpoint has ever been exact.

## Fifteenth continuation — restore the case-3 read at the original extent

**15/16 exact targets and all 47 exact mechrides neighbors are preserved. Space Tower improves from 136 to 34 strict mismatches.** This is the first retained checkpoint whose actual RET-bounded body has the original 222 instructions, 698 bytes and 16-byte stack frame. The formatted landed body was rebuilt under `/W3 /O2 /Gy /Gd` and audited. Its case-6 seat correction remains intact.

The working combination adds a case-local `int xx = *(volatile int*)&tilex;` **before** the seat read, uses `xx` in target-x, and reads `RIDE_TILE(r)->b.y` after the target-x store. The existing case-8 volatile tile-x read must remain. A plain early copy, moving the only volatile read from case 8, or reading through the long-lived `tile` pointer instead does not retain this allocation. The early read restores the missing two instructions without adding a stack home.

| Measurement | Previous checkpoint `64eb174b` | Current checkpoint |
| --- | ---: | ---: |
| Strict index mismatches | 136 | 34 |
| First divergence | 25 | 22 |
| Actual instructions / bytes | 220 / 693 | 222 / 698 |
| Stack frame bytes | 16 | 16 |
| Index-aligned strict / rb / ob | 136 / 123 / 136 | 34 / 25 / 34 |
| RET-bounded aligned strict / rb | 18 / 11 | 35 / 15 |

The lower index mismatch count partly reflects removal of the two-instruction shift. The aligned edit counts are reported separately and do not improve: restoring the missing read also changes register choices in the head and case 3. It remains WIP, with no claim that equal body size closes it.

### Remaining windows

- **Head, indices 22–29:** seven differences. The first definition reload uses EAX instead of EDX, tile-x uses EDX instead of EAX, and base-y loads before tile-x/base-x. Indices 30–88, including all of cases 0 and 1, are exact.
- **Case 3, indices 89–107 and 113–116:** the original preserves the derived tile address in EAX and reloads tile-x into EBX. Ours keeps the rider in EBX, folds the coordinate read into `[ebx+0x0d]`, and uses EAX for tile-x. Seat and target registers, zeroing/sum order and the seat shift also differ. Indices 118–164 are exact.
- **Case 8, indices 165–168:** two pairs of loads/arithmetic are reordered. Indices 169–221, including the real return, are exact.

### New rejected families

Before the early-read combination, address-observation intrinsics, register hints, narrower or bitfield byte views, loop-local scopes, ABI-identical argument/return views, alternative pointer arithmetic, named coordinate reads, origin-pair copies, loop guards and dead local assignments failed to preserve the original frame with the required read. Some reduced the raw index score while adding real instructions or stack homes; those were rejected. Actual bodies were measured through RET independently of the audit slice.

On the 34-mismatch body, case-local tile pointers and repurposed cursor locals fold back into the rider-relative byte access. Empty pointer consumers prevent folding but retain the long-lived tile value and spill the rider, producing 230 instructions/728 bytes and a 20-byte frame. Named head destinations, cast-only aliases and free field barriers do not fix the remaining register assignment. These results identify a specific live-range problem, not an impossibility proof.

All clean candidates received the full-file 47-neighbor audit. Scratch logs and sources f45–f68 remain local and untracked; the production checkpoint is the formatted f68 body. Only the WIP Tower body, its own note and this report changed. The goal remains active until Tower receives audit `[OK]` and the verified result is published.

## Sixteenth continuation — original register allocation recovered

**Space Tower is now 13/222 strict mismatches (94.1%), first 25, real 222 instructions/698 bytes, original 16-byte frame, no escapes.** Its marker remains `WIP-FUNCTION`; audit `[OK]` has not been reached. All 47 exact neighbors still pass the whole-file audit, preserving all fifteen exact Scope F targets and the previous nine-file total of 127 exact functions. The formatted landed body was rebuilt with `/W3 /O2 /Gy /Gd` clean.

- **The car index needs its own lifetime.** Read tile-x inline in the case-3 target-x expression, and read tile-y through the named `tile` pointer. After target-x, before target-y, assign a separate `unsigned car = seat >> 1` (declared at case entry) and consume it with `if (car) { }`. Use `car` in the eventual car-table access. The empty consumer emits no instructions. This recovers the original EAX tile pointer, EBX tile-x reload, ECX seat, EDX x sum, EDI car and the pre-call shift. Removing the consumer changes the final candidate from 13 to 29; an early named x reload spills the rider again (211). Moving the car definition before target-x gives 32 with the shift too early. Moving only its empty consumer after target-y retains 13.
- **Name the head reads before assigning homes.** Keep the two separately read definition pointers, then initialize plain `xx`, `bx` and `by` locals from the tile-x byte and the two origin fields, before assigning `tilex = xx`, `tx = bx + xx` and the y sum through its named destination. This changes the 17-mismatch car-index candidate to 13 by recovering the head's original register assignment. Making those new field reads volatile instead introduces spills or changes the schedule.
- **The remaining mismatch is entirely instruction order.** Strict/register-blind/offset-blind is **13/13/13**, and RET-bounded aligned strict/register-blind is **11/11**. Head indices 25–27 load base-y first instead of last. Case-3 indices 89–94 load tile-x and copy the seat later than the original. Case-8 indices 165–168 exchange two instruction pairs. All other indices, including the tile-y reread, movement call and real return, are exact.

Intermediate evidence: a free volatile read of the x seat-table field on the previous rider-relative form gave 26 mismatches and the original extent; an assignment inside an empty shift consumer reduced that to 22. Those are superseded by the direct tile-pointer/car-index form and neither extra shim is needed in the retained body. Expression-level versus named tile-x reads are distinct levers here: cast-only and operand-order forms of the expression retain the tile pointer, while assigning that read to a named early value can rewrite the loop around the tile and spill the rider.

Rejected follow-ups include scalar/union/cursor storage views; early seat-shift locals without the required consumer; all head/case-3/case-8 coordinate-source combinations; one-iteration loop wrappers; free access barriers; byte/union home stores; named x-home and target pointers; expression operand ordering; and earlier case-8 home reloads. The latter need an empty consumer to retain the original extent but still give 14 or worse. Zero/sentinel initializers and one-field aggregate home views are inert. Head empty consumers add storage or change allocation. Scratch probes that overwrite the live `next` value were excluded as semantically invalid and were never landed.

Evidence lives locally in f69–f90 scratch sources, logs and full-file audits. The landed checkpoint is f90. Only this WIP body, its own notes and this lane report changed. The 100% goal remains active.

## Seventeenth continuation — case-8 instruction order recovered

**Space Tower is now 11/222 strict mismatches (95.0%), first 25, real 222 instructions/698 bytes, original 16-byte frame, no escapes.** The formatted f104 production checkpoint compiles cleanly under `/W3 /O2 /Gy /Gd`; its full-file audit retains all 47 exact neighbors. All fifteen exact Scope F targets are preserved. Tower remains `WIP-FUNCTION`, without audit `[OK]`.

In case 8, separately read `def` through a volatile pointer into local `d`, then read the tile-x home into `xx`. Compute the two sums and split the block with `if (sx) { }` before storing the targets. The empty conditional emits nothing. The pointer read orders the definition load before tile-x; the block split prevents the offset loads from being folded into their adds. All case-8 instruction positions now agree with the original. The tile-x carrier uses EDI instead of EBX at indices **165 and 168**, the only remaining differences in that case. A plain definition read leaves 14 mismatches, with different register choices and order. A named pair of signed offset values is equivalent to the inline field reads here.

The other residuals are unchanged: head indices **25–27** load base-y before tile-x/base-x, and case-3 indices **89–94** load tile-x and copy the seat later than the original. Current strict/register-blind/offset-blind is **11/9/11**; actual-body aligned strict/register-blind is **10/8**. Equal size, frame and most registers are verified independently of the audit slice.

Further evidence since the previous checkpoint:

- Head read-order permutations and delayed assignments are inert at 13. Reversing definition reads or making only the first ordinary gives 15; making both ordinary or using new field destinations changes the frame/allocation. Empty head guards add storage. Declaring tile-x fully volatile changes the head permutation but retains 13; adding volatile field reads then introduces spills.
- Removing either or both tile-x read shims does not recover a natural spill. One-field arrays, structs/unions and self-comparison address observation do not repair it. Discarded volatile reads, early tile-x carrier reuse, tuple locals and split pointer/reload definitions add instructions or spill the rider. These failures do not establish an impossibility result.
- Separating the car copy from its shift is equivalent to the retained form. Removing the post-shift empty block worsens it. An early case-8 reload plus a plain definition read and a later block split reaches 14; the independently ordered definition read is needed to reach 11.
- On the 11-mismatch body, signed/unsigned/long/register/const/enum carriers, one-field aggregates, existing-variable reuse, self-reassignment and shared loop/function scope do not change EDI to EBX. Additional later empty guards are inert. A 64-bit carrier and complement/negation identities add instructions or alter allocation. A scratch declaration-order error was corrected before evaluating the later-guard family.

Scratch f91–f105 evidence remains local and untracked. Only the WIP Tower body, its own notes and this report changed. The goal remains active until the complete body audits `[OK]` and that verified result is committed and pushed.

## Eighteenth continuation — name the seat-table element

**Space Tower improves from 11 to 10/222 strict mismatches (95.5%), first 25.** The formatted f135 checkpoint is still the full 222 instructions/698 bytes, with the original 16-byte frame and no branch escapes. It compiles cleanly under `/W3 /O2 /Gy /Gd`; its whole-file audit retains all 47 exact neighbors. No exact Scope F target was edited or reopened. Tower remains `WIP-FUNCTION`, without audit `[OK]`.

After reading the seat, assign `SeatOfs* offset = &g_tower_seat[seat]` (with the declaration at block entry) and use `offset->dx` and `offset->dy`. Keep the inline volatile tile-x read and the later car-index split. The pointer emits neither a separate instruction nor a stack home. It changes the case-3 schedule: the x reload moves before the table read, and the car copy moves before the add. Five positions at **89–93** now differ instead of six at 89–94. Index 94 and the remainder of that case are exact.

| Window | Original | Retained candidate |
| --- | --- | --- |
| Head 25–27 | Tile-x, base-x, base-y loads | Base-y, tile-x, base-x loads |
| Case 3, 89–93 | Reload x into EBX; widen seat into ECX; copy seat to EDI; load table x into EDX | Widen seat into ECX; reload x into EDX; load table x into EBX; copy seat to EDI |
| Case 8, 165/168 | X reload and add use EBX | X reload and add use EDI |

Strict/register-blind/offset-blind is **10/8/10**. RET-bounded aligned strict/register-blind improves from **10/8 to 8/6**. These aligned counts are sequence-edit diagnostics, not audit completion percentages. The case-6 `rec->seat[b->seat]` correction remains intact, and the original tile-y reread still follows the target-x store.

The named element works whether assigned separately or inside the x expression, whether used for both axes or just x, and through an equivalent integer-pair view. A const-qualified pointer and reversed sum operands are equivalent. Naming an early x value or a table-x value still spills the rider (229 instructions/727 bytes, 20-byte frame); naming the x sum also changes the frame. A free volatile table-x read returns to the prior 11-difference schedule; a volatile table-y read changes the body structure and is rejected. Target-field accumulation starting with table x is equivalent to the retained form; starting with the x home adds storage and instructions.

Further rejected families in this continuation:

- **Head scheduling:** intrinsic field copies, accumulator spellings, plain field-source pointers, named local destinations, local operand groups, staged base-y homes, equivalent conditional loads and conditions implied by the unsigned-byte range. Simple pointer/store views are inert; aggregate copies, conditional forms and staged homes introduce storage or instructions. Two scratch aggregate variants initially failed C89 declaration ordering; corrected versions were measured separately and still failed to improve.
- **Case-8 allocation:** named field/destination pointers; paired input operands, sums, and definition-pointer/x-value aggregates; identical conditional arms; alternate case labels and exits; and inline x reads combined with the required sum split. Inline reads choose the desired EBX but return to the old four-position load permutation; named early reads retain the correct instruction order but choose EDI. Removing one other action at a time for a diagnostic never changed that EDI choice. Those deleted-body controls are not valid reconstructions and were never retained.
- **Lifetimes and types:** equivalent outer loops, explicit reuse of the incoming argument home, shared x carriers, narrower coordinate/seat types, separate tile-pointer definitions, ABI-identical local movement-call views, car-index aggregate views and seat assignments inside the table expression. None closes the remaining windows. A car-first in-place shift introduces an extra scaled-index instruction. Zero-count rotation intrinsics emit an extra rotate and were rejected, even where an aligned metric looked smaller; an initial misplaced pragma failed compilation and was excluded.

All clean candidates received the full-file 47-neighbor audit. Scratch f106–f135 sources, listings and logs remain local and untracked. Only Tower's WIP body, its own notes and this lane report changed. Remote checks still place `scope/F-fable` at the already-incorporated `e9e1f914`. The 100% goal remains active; the recorded failures do not prove the residual unreachable.

## Nineteenth continuation — verify address identities; preserve the 10-difference body

**No new Tower body is retained.** Production remains `acc92400`: 10 strict differences, first 25, real 222 instructions/698 bytes and the original 16-byte frame. All fifteen closed targets and all 47 normalized-exact mechrides neighbors are unchanged. The 100% goal remains active. Remote verification found `scope/F` at `acc92400`, `scope/F-fable` at `e9e1f914`, and `main` at `6a95613e`; no newer F work was available to pull.

### Address verification beyond the normalized audit

Read the additional DECOMP and HANDOFF findings on `origin/main` without merging or editing that checkout. Ran its existing `tools/relocs.py` against the nine absolute source paths in this worktree; no tool or shared source was changed. All 127 marked bodies passed that tool's prerequisite instruction/extent comparison. Across the fifteen closed Scope F targets, **438 address positions resolve and match, zero mismatch, and 103 remain unresolved by the tool**. The unresolved classes are floating-point/string literals, jump tables, thirty compiler `__ftol` calls in the Animate twins, and the static `kJcRiverRect`. This result is not a claim that the tool verified the unresolved positions; earlier manual symbol/data checks remain recorded above. The original conversion calls target the game's x87 helper at `0x00458930`, defined in `bnvpath.c`, rather than an address annotation the relocation parser can bind to `__ftol`.

The complete nine-file sweep has **11 resolved-address mismatches in two unchanged, non-target neighbors**, matching the already-documented integration backlog exactly:

- `PlaneRide_Create`, indices 34–36: destinations `0x0062fe84/88/8c` differ from the source-pointer globals currently named by the stores.
- `Copters_Activate`, eight positions across its two switches: three case-to-path mappings are wrong in each switch, with the duplicated instruction positions producing eight hits.

`origin/main:docs/HANDOFF.md` explicitly defers these fixes to the owning scopes' merges at the quiet-tree gate. They were not introduced by Scope F's fifteen closes or the Tower changes, and these exact-marker bodies were not edited in this continuation. Keep this distinction when reporting the 47/127 `[OK]` counts. Local evidence is in `*_relocations_current.json`, `*_relocations_current.log` and `closed_targets_relocations.json` under `scratchpad/scope-f/`.

### New Tower evidence

- **The table-pointer definition must precede the x calculation.** Whole-array, row-array, flat-integer and global-base pointer views are equivalent to the retained element pointer. Naming only the y pointer before x also retains 10. Moving that definition after x returns to 11; moving the base still earlier is inert. Adding a named x-home pointer likewise returns to 11. A separate pointer for each table field retains the frame but grows the body to 700 bytes and worsens the schedule. Named car rows/directions and simultaneous inline seat/element assignments are inert.
- **An exit-case join can choose EBX, but its extra check remains.** Initialize `xx` from `(int)r`, then conditionally overwrite it from the tile-x home under `if (r)`. This reproduces the desired EBX reload and add, with the right exit load order, but adds `test/je`: 224 instructions/702 bytes. It is not a close. Guards already proved by `rec` or `b->state`, equivalent always-true predicates and merged conditional arms remove the extra branch but also return to EDI. Valid `__assume` experiments either add storage or preserve the unwanted check; none was retained. A union cursor, integer cursor or dead-argument carrier is inert.
- **The additional base-x read-barrier combinations spill.** On the retained car/table/sum shape, making base-x volatile, alone or with x/base-y, introduces a 24-byte frame. An exit-qx barrier is inert; exit-qy moves its load after the add. Changing the value tested by the existing empty exit conditional does not change allocation.
- **Aggregate initialization does not recover ordinary adjacent def reloads.** Plain two-pointer initializers give 223 instructions/702 bytes and a 20-byte frame; volatile initializer elements are equivalent to the retained separate locals. Initialized head-value aggregates add storage. Moving each head declaration, the pointer pair, the value group or all five to loop/function scope is inert. Initial scratch variants with declarations after statements were excluded; the corrected C89-compatible scope grid was measured separately.
- **Arithmetic spelling is normalized here.** Multiplication by 256, distributed shifts, unsigned scaling, exit offset/coordinate accumulators and reuse of the home-value accumulator all retain 10.
- **Wrapping the exit reload in a one-iteration loop does not retain the desired join for free.** A literal-bound `for`, record-guarded `while` and one-flag loop are inert; post-decrement counting adds five instructions, and a `do/while (0)` changes the shared tail and loses an instruction. Both altered extents were rejected.

Every clean candidate received its whole-file audit with the 47-neighbor guard. Scratch f136–f150 sources and listings are retained locally as evidence; no unsuccessful candidate replaced production. These results narrow the remaining scheduling/allocation work and do not establish an impossibility result.

## Twentieth continuation — test live ranges and inspect compiler listings

**Production is unchanged at 15/16 exact targets and 10 Tower differences.** The body is still `acc92400`, with 222 instructions/698 bytes and a 16-byte frame. None of the 81 candidates in f151–f163 improves that checkpoint. All 81 compile without warnings and pass the whole-file audit with 47 normalized-exact neighbors. Two additional diagnostic listing compiles also retain those 47. The address-verification qualifications and deferred integration fixes in the nineteenth continuation still apply.

- **Earlier seat definitions add storage.** Moving the seat read before the head calculation, before the state guard or just inside it, with or without an action-3 guard, produces 224–235 instructions and 20/24-byte frames. Building the seat from a zeroed integer plus a byte assignment or one-byte copy also adds storage; equivalent union forms do not help. Narrow car-index types add extensions/storage, while same-width signed/unsigned forms are inert.
- **The early-x spill is visible in the compiler's own listing.** With the named early x read and the long-lived tile pointer, the rider gains a stack home in the argument slot and the definition pointer moves into an extra local slot. Case 3 reads y through the tile pointer in EBX. `/FAs` was used only to inspect this output; optimization and calling-convention flags were unchanged. Reconstructing y directly from the rider avoids that spill but gives 222 instructions/699 bytes and 66 strict differences. Naming the reconstructed pointer brings back the extra home. Grouping the pointer and x value in either member order also spills. Exit-case groups containing the person pointer and x are inert.
- **Equivalent exit joins do not retain EBX for free.** Crosses of the previous rider-guarded reload with outer-loop forms either retain its two extra instructions or grow further. Duplicating the volatile reload in both arms removes the jump but leaves an unused `test`, returns the x value to EDI, and produces 223 instructions/700 bytes. Using a plain read in the unreachable false arm adds a 24-byte frame. No such branch was retained.
- **Zero-code boundaries are not interchangeable.** Replacing the existing car/sum empty conditionals with one-arm switches removes their useful scheduling effects. A switch around the head reads is inert; one around a named early case-3 reload spills. Immediate labels and single-pass `for` blocks around head reads are inert. A `do/while (0)` around those same reads changes case 3 and the shared exit tail, producing 221 instructions/695 bytes, just as the earlier exit-local wrapper did.
- **Coordinate-home variants do not close the load windows.** A zeroed integer/union followed by a byte store from the already-read x adds an instruction; directly rereading the tile adds a frame slot. Volatile home stores move the first head difference from 25 to 26 without reducing the count; adding duplicate guarded stores grows the frame. Narrow home experiments used their declared read widths, never a four-byte read of a narrow object. The volatile-short form retains 222 instructions but requires two sign-extending loads, grows to 700 bytes and still has 10 differences. Other narrow forms spill.

Sources, full-body measurements, audit logs and diagnostic listings remain local in `scratchpad/scope-f/`; the candidate summary is `summarize_f151_f163.py`. No candidate replaced the WIP body, and none of the fifteen completed targets or exact neighbors was edited. Remote checks still find `scope/F-fable` at the already-incorporated `e9e1f914` and `main` at `6a95613e`. The 100% goal remains active; these finite negative results do not establish that the remaining match is impossible.

## Twenty-first continuation — compare regions and retain an alternate head candidate

**Production remains 15/16 exact, with Tower at 10/222 differences.** All 122 candidates in f164–f180 compiled cleanly and passed the full-file audit with 47 normalized-exact neighbors. Each received its independent RET-bounded measurement. No source file changed from `acc92400`; the address-identity limits and deferred integration items above still apply. The goal is not complete.

### An alternate candidate has the correct head instruction order

A corpus comparison of saved listings found that f83's `separate_head_4` has only two head differences: base-y and base-x load in the opposite order. Its actual size is 222 instructions/698 bytes, but its older case 3 is worse than production. Moving the retained exit case into that candidate gives **f173_old_head_current_exit**, at 19 strict differences, still 222/698. Adding the current table pointer or named car split does not preserve that result; the measured crosses grow, spill, or change other scheduling.

From **that 19-difference candidate**, making the head base-x field read volatile and adding `if (tx) { }` after the completed head produces **f177_old_0**. Its head now reads x, base-x and base-y in the original order, stores x, adds the x origin, reconstructs the tile address, and computes/stores y in the original order. The original 16-byte frame and complete 222-instruction/698-byte extent remain. This is a different compiler constraint from applying those edits to production: without the sum consumer, the volatile-base-x family can sink the x addition past `SpaceTower_TakeSeat` and preserve its two operands in extra homes.

The new candidate is **not a replacement for production**. Rider and tile/x-sum values exchange EBP/EBX, the definition reloads use ECX/EDX instead of EDX/ECX, and the older case-3 register choices remain. Strict/rb/ob is **41/7/41**, first 9; RET-bounded aligned strict/rb is **45/5**. Case 3 still reads y through rider+13 rather than tile+1, reverses its x-store/zeroing pair, and shifts the car index after the movement call. The lower register-blind counts do not constitute an exact match.

Byte-sized rider consumers at entry, loop entry and after record lookup are inert on this candidate. Reusing the dead case-4 cursor through a byte/integer union or pointer-value conversion for the existing seated-count update also leaves its allocation unchanged. Moving sums, coordinates and the base array into a block after record lookup is inert on both the alternate and production candidates. These controls do not establish that the register choices are fixed for every source form.

### Other measured exclusions

- Using base[0] for the x sum, sharing destination pointers for both base elements, advancing the destination pointer, and reading through a Pos view all retain production's 10. Replacing the array itself with a struct/Pos/union adds a stack slot. Grouping all existing stack values preserves the frame but changes the case-0 count register, giving 13; grouping just x and next is inert when their order is preserved.
- Narrow seat bitfields add extraction/storage; 32-bit fields are inert. Computing a named car pointer before x, y or the movement call either spills or loses the useful index split. Naming the seat-table byte offset is inert; deriving the car index from the scaled offset adds an instruction.
- Inverse/else forms and single-pass while/for forms of the existing empty conditionals are inert. Combining assignments with the car or exit conditions is also inert; an explicit zero-seat arm adds three instructions. Descriptive local renaming does not affect these results.
- Temporarily sharing a union between the tile pointer and base-x value is inert when the pointer is restored before the head y read. Restoring it later, or adding the base-y read barrier, changes the frame. Crosses of volatile head fields, early x reloads, rider-relative y reads and sum consumers were measured separately; the only new complete-size scheduling candidate is the alternate described above, not a new production close.

Scratch evidence is in f164–f180 logs/listings and `summarize_f164_f180.py`. The region inventories are search aids with provisional call-based anchors, not replacement audit gates; shortlisted sources were inspected and rebuilt before drawing the conclusions above. Remote checks still show `scope/F-fable` at `e9e1f914` and `main` at `6a95613e`, with no newer work to pull. Continue toward the full instruction and address-identity verification, then the 16/16 documentation and publication gates.

## Twenty-second continuation — test the alternate head and remaining reload lifetimes

**Production remains 15/16 exact, with Tower at 10/222 differences.** None of the 99 candidates in f181–f193 improves the retained `acc92400` body. All 99 compile without warnings, pass the whole-file audit with 47 normalized-exact neighbors, and have an independent RET-bounded measurement. No source file changed. The existing address-identity qualifications and deferred integration fixes still apply; the 100% goal remains active.

- **The alternate head can lose one register difference, but is still worse overall.** Starting from f177_old_0, making only the first definition-pointer read ordinary produces **f182_2_0: 40 strict, rb 7, ob 40; first 9; real 222 instructions/698 bytes; 16-byte frame; aligned strict/rb 44/5**. Its correctly ordered head still exchanges the rider and tile/x-sum EBP/EBX roles. Reversing the two volatile pointer declarations gives the same score. Volatile rider-list/next reads are inert; a volatile bloke-pointer read worsens the register-blind count. Short reconstructed tile pointers fold back to rider-relative y, while retaining the long tile pointer adds storage. Explicit earlier car shifts alter scheduling or size. None replaces production.
- **Sharing the dead cursor value does not force the original reload register.** An unsigned cursor or separate unsigned carrier with an empty conditional at the existing loop join is inert. A single-pass while consumer changes the body to 701 bytes. Sharing that carrier across cases 3 and 8 adds a 20-byte frame. An initialized byte/integer union also carrying case 4's seated count has the same result; it neither removes the spill nor improves the score. The integer representations are never dereferenced as pointers after removal.
- **Early case-3 reads still spill across additional boundaries.** Assignment inside an empty condition, immediate labels, single-pass for/while blocks and later empty x consumers all retain the 229-instruction/727-byte spill shape. A do/while-zero read block changes the size again. Grouping tile, cursor, x sum and seat is inert on the two complete-size seeds; groups containing an early x reload still add storage. Writing the reload back to its existing home or either base-array element also retains the spill.
- **Expression placement and existing homes do not resolve the local register choice.** Simultaneous seat/element assignment inside the case-3 target expression is equivalent to production; a volatile table-x read returns to 11 differences. Case-8 reloads assigned to the x home or base elements, including comma and assignment expressions, retain 10 and EDI. Combining inline reloads with volatile offset reads and the existing sum split gives 12 differences with the original extent, not a close.
- **A byte-sized view retains an unwanted conversion.** These probes first read the full-width volatile integer home, then narrow that value. Conditions implied by the source's unsigned-byte range do not remove the extra conversion or spills. No narrow-object four-byte read or unsupported range assumption was used. No such form was retained.
- **Equivalent state guards and one-iteration regions are not a missing free boundary.** Inverse state guards and a single-case state switch are inert. A goto changes the shared tail to 701 bytes; a do/while-zero state guard gives 221 instructions/695 bytes. Their early-x variants still spill. Counted one-iteration wrappers around the completed head produce 224 instructions/706 bytes with a 20-byte frame. The same wrappers around the exit calculations are inert.

Scratch sources, listings and audit logs remain local and untracked. `summarize_f181_f193.py` verifies all 99 audit records and actual-body measurements. Remote checks still find `scope/F-fable` at `e9e1f914` and `main` at `6a95613e`, with no newer F work to incorporate. The alternate head remains a separate investigation candidate; the retained body is unchanged, and none of these finite exclusions establishes that the final match is unreachable.

## Twenty-third continuation — an alternate reaches three register-blind differences

**Production remains 15/16 exact, with Tower at 10/222 strict differences.** All 77 clean candidates in f194–f205 pass the whole-file audit with 47 normalized-exact neighbors and have independent RET-bounded measurements. Three initial helper probes with an unused-local warning were excluded, then corrected and measured separately. No production source changed from `acc92400`; the goal remains active and the earlier address-identity qualifications still apply.

### Preserve the in-place seat shift as a separate investigation candidate

A read-only search across 1,798 saved listings found the older f82 `if_assign_shift` case-3 form. The search masked registers and stack offsets and compared bounded windows; it is a search aid, not an audit gate or a proof of a floor. The useful source difference is **`if (seat >>= 1) { }` after target-y and before CalcMoveLine**, followed by `g_tower_car[seat]`. Updating the original seat variable in place has a different result from computing a separate car variable or merely testing `seat >> 1`.

Applied to the ordered-head f182_2_0 candidate, this gives **f203_f182_0: strict/rb/ob 36/3/36, first 9, actual 222 instructions/698 bytes, 16-byte frame, aligned strict/rb 42/3**. The car shift now occupies the original pre-call position. The three remaining register-blind differences are the exchanged target-x store/EDX-zeroing pair at 96–97 and the rider-relative y read at 98 (`r+13` instead of `tile+1`). The strict differences still include the exchanged rider and tile/x-sum EBP/EBX roles, definition-register choices, case-3 value registers and the exit's EDI carrier. Thus 36 is **not** an improvement over the retained strict-10 body, and this candidate is not landed.

The same in-place shift on f173_old_head_current_exit gives **f203_f173_0: 15/5/15, first 26, 222/698, frame 16, aligned 21/5**. It preserves the original rider register but retains the head's two swapped field loads. These two complete-size alternatives expose a more focused allocation problem; they do not supersede the production marker or percentage.

Follow-ups preserve the distinction between allocation and instruction order. A volatile target-x store worsens the alternatives to 44 or 23 strict; a volatile y-byte read grows the body to 704 bytes. Named seat-table elements grow it to 700 bytes. Short reconstructed tile pointers are inert; the long-lived tile pointer adds a 20/24-byte frame, with or without the table pointer. The early zeroing is enabled by the alternate's x sum being in ECX rather than EDX: forcing the store order alone does not reproduce the original value dependencies.

### Other verified exclusions

- Alternate exit forms and all six orders of the next/bloke/tile setup do not recover the ordered head's rider register. Plain/direct definition reads worsen the score; the rider-guarded exit still adds its extra test and jump.
- Range facts were tested while retaining a full-width x use, with no narrowing conversion. The combined unsigned-byte range is inert; separate bounds, byte-equality and mask facts introduce a 24-byte frame. These facts follow from the local's unsigned-byte assignment and non-escaping address; no unsupported assumption was retained.
- Eighteen additional **scratch-only helper diagnostics** tested inline argument temporaries, a head calculation with output pointers, and the complete body inside an inline expansion. Their measured bodies retain the original 17 calls and all 47 neighboring audit matches. Seat-argument forms reproduce the early-x spill, exit-argument forms give 10 or 11, head-output forms add storage, and complete-body expansions preserve each seed's result exactly. No helper was added to production. The initial three unused-seat declarations are the excluded warning probes mentioned above.
- A volatile home store with one or both read shims removed does not produce a free ordinary reload: the measured bodies have 227–229 instructions and 20/28-byte frames. Whole four-byte structure copies of the volatile origin fields are equivalent to the corresponding scalar reads and still add storage. Scalar/structure assignments inside the case-3 sum, even with the seat and element assigned inside the other operand, reproduce the early-x spill; an in-expression home-pointer assignment gives 11 differences at the original extent.

Evidence remains local in f194–f205 and `summarize_f194_f205.py`; `tower_case3_window_inventory.py` records the listing search separately from the authoritative measurements. Remote checks still find `scope/F-fable` at `e9e1f914` and `main` at `6a95613e`, with no newer F work to pull. Continue from the strict-10 production body and the two explicitly identified alternatives, preserving the full instruction, address, documentation and publication gates.

## Twenty-fourth continuation — distinguish an early reload from a false improvement

**Production remains 15/16 exact. Tower remains WIP at 10/222 strict differences, first 25, actual 222 instructions/698 bytes, 16-byte frame.** No production source changed from `acc92400`. The 80 distinct, clean candidates in f206–f216 each preserve all 47 normalized-exact neighbors in the whole-file audit and have independent RET-bounded measurements. `summarize_f206_f216.py` verifies the records. The minimum strict difference count among candidates with the original instruction count and byte length is still ten. Earlier address-identity qualifications remain in effect.

- **A floating representation copy advances the case-3 reload without the usual rider spill, but adds a store.** In f211_3_float, declare `union XBits { float f; int i; }; union XBits xbits;`, assign `xbits.f = *(float*)&tilex` before reading the seat, and use `xbits.i` in the existing target-x sum. Plain and volatile float reads give the same **9 strict / 6 rb / 9 ob, first 25, actual 222 instructions/700 bytes, frame 16, aligned strict/rb 8/5**. The emitted operations are integer moves: index 89 reads the home into EDX, indices 90–91 read the seat, index 92 writes EDX back to the same physical home, index 93 copies the seat to EDI, and index 94 adds directly from the table. The extra home store occupies the missing separate table-load position. This is a useful scheduling observation, **not a production improvement or a match**. Whole-union copies add storage; the analogous case-8 float transfer grows to 224 instructions/706 bytes.
- **Changing the x home's declared representation does not remove that problem.** Float and float/int union homes, written through their integer representation, retain ten with the inline volatile read. Correctly generated early volatile integer reads still give the established 229 instructions/727 bytes and 20-byte frame; ordinary reads give 228/724. No representation change was retained.
- **Movement argument copies and pointers are inert on production.** Twelve case-3/case-8 forms copy the target, world, or both into local `Pos` values, or retain pointers through both the coordinate stores and the call. Every listing remains at the original-size strict ten. This differs from the earlier destination-pointer probes, which covered stores alone.
- **Explicit cursor lifetimes and guard placement do not supply a free register.** Reusing the actual dead cursor for the early case-3 x read still adds storage on both production and the fifteen-difference alternate. Moving all node setup and the record test into the loop condition is inert on the four tested seeds. Making the current node local to a `while (next)` body, with scalar or paired node/tile storage, adds instructions: the two alternate seeds reach 224/704; production and the early-x seed also enlarge the frame. Additional existing-field read barriers in other Tower cases do not restore the ordered-head alternate's original rider register.
- **Explicit shared direction arguments and copied table accumulators regress.** Joining cases 3 and 8 at an explicit NewDirForAction label, using the existing direction, a new byte, or a new unsigned argument, grows the measured bodies to 226–235 instructions. Copying the complete seat-table element into a local value, including compound sum/shift updates of its fields, gives 230/728 with a 20-byte frame.
- **An early in-place seat shift requires preserving the original table index.** The older f88_seat_shift scratch probe mutated `seat` before its table-y lookup and is excluded as semantically invalid; it was never production. New f215 probes preserve the already-computed `SeatOfs*` before shifting the seat, so both table fields still refer to the original seat. Moving the shift before x, before y, or before the movement call, with combined or separate empty consumers, does not close production. Crossing the same valid forms with the float-copy diagnostic also fails the original extent gate.

Six generated records are excluded from the 80-candidate count: four f210 records reused ambiguous alternate-seed labels and were remeasured under complete names; two f212 early-volatile probes accidentally replaced their own initializer rather than the intended use. The generator was corrected, and f214 contains the valid remeasurements. No excluded source was retained. Remote checks still find F-fable at `e9e1f914` and main at `6a95613e`; there is no newer F work to incorporate. The goal remains active, with strict ten as production and f203's strict/rb 15/5 and 36/3 alternatives preserved separately for continued investigation.

## Twenty-fifth continuation — reload transfers and branch-merge controls

**Production is unchanged: 15/16 exact; Tower WIP, strict/rb/ob 10/8/10, first 25, actual 222 instructions/698 bytes, frame 16.** All 80 candidates in f217–f226 compile cleanly, preserve the 47 normalized-exact neighbors in whole-file audits, and have independent RET-bounded measurements. There are no excluded/error records in this batch. `summarize_f217_f226.py` verifies the evidence; ten remains the minimum strict count at the original instruction count and byte length. The address-identity qualifications and the active goal remain unchanged.

The current integrated compiler notes were read from `origin/main` at `6a95613e`, including scopes R, X, P and K. In particular, R's two-flag merge closed a body previously called unreachable: a definition at a join can stop early propagation, while later register coalescing removes its apparent cost. This is evidence to continue source reconstruction; it does not establish that the same spelling transfers to Tower.

- **The float-copy scheduling observation survives other aggregate wrappers, but its extra store does too.** A one-float struct or array reproduces f211's strict nine at 222/700. A scalar float, a union-to-integer memcpy, or reusing `base[0]` gives 224/706; the latter two keep frame 16. The union-to-integer copy restores the original case-3 value registers only after an extra load/store pair and a later home reload. Direct four-byte memcpy forms, including using the returned destination pointer/value and a volatile home declaration, all give 228/724 with frame 20. These are not candidates for promotion.
- **Two initialized x values merged with OR do not remove the early-x spill.** Block, do/while-zero and one-iteration for regions were measured, with assignment and compound-assignment merges. Case 3 stays at frame 20 and 229–231 instructions. Case 8's block/for forms are inert; its do forms give 221/695. The tested region boundaries do not reproduce R's useful merge effect here.
- **Loop-variable scopes and direction temporaries are inert on the retained shapes.** Moving the rider alone, or all loop variables, into a block after TickMachine, using either declaration initialization or later assignment, preserves each seed exactly: production ten, alternate fifteen, alternate thirty-six. Reusing the direction/car variable for the final heading, naming a new byte/unsigned heading, or declaring the movement direction separately inside cases 3/8 also preserves production ten.
- **Other storage interactions still add code.** Crossing the float reload with base-field barriers, the post-head x-sum consumer and an ordinary first definition read gives 225–231 instructions with 20/24-byte frames. Zeroing the seat through memset or a constant-trip fill before its byte assignment, with the x reload on either side, produces 231–233 instructions and frame 20. Reusing car, record or rider storage for the early x value and immediately consuming it in an empty if/while still gives 229/727, frame 20; the saved tile pointer remains the y source in those valid probes.
- **A complementary exit read can remove the branch while leaving its test.** Start with `int xx = (int)r`; the true arm reads the volatile x home, and the false arm combines that same read with `xx` using OR, ADD or XOR. The false-edge `xx` is zero, so every path gets the same single x read. All six f225 forms emit **223/700, frame 16, aligned strict/rb 9/7**: a dead `test ebx,ebx` remains before the reload, the conditional jump disappears, and the x carrier is again EDI. Complementary sequential guards, distinct arm-local values/pointers and an explicit join reproduce that result in f226; do/while-zero regions give 222/697 instead. This does not close f142's tradeoff: its guarded reload retains EBX but also retains both test and jump (224/702).

No production file differs from `acc92400`. Remote checks still find F-fable at `e9e1f914` and main at `6a95613e`, with no newer F work to pull. The strict-ten body and the explicitly separate f203 alternatives remain the continuation points; no finite set of these exclusions is an impossibility proof.

## Twenty-sixth continuation — a shared copy restores the exit register at a cost

**Production remains 15/16 exact. SpaceTower_Activate (`0x0043bac0`) is still WIP at 95.5%, strict/rb/ob 10/8/10, first 25, actual 222 instructions/698 bytes, frame 16.** No production source differs from `acc92400`. All 85 candidates in f227–f236 compile cleanly with `/W3`, retain the 47 normalized-exact neighbors in whole-file audits, and have independent RET-bounded measurements. `summarize_f227_f236.py` checks the 85 unique records; no compiled error/warning records are included. Ten remains the minimum strict count at the original instruction count and byte length. The earlier address-identity qualifications still apply.

- **A shared float representation copy recovers the exit's EBX without the rider guard.** In f235, declare a function-local `union { float f; int i; } snapshot;`, copy `snapshot.f = *(float*)&tilex` after the head, and use `snapshot.i` for both case-3's x operand and case-8's existing `xx` initializer. Both placements—before the state guard and just inside it before the action switch—give **223 instructions/704 bytes, frame 16, aligned strict/rb 7/6**. Case 8 now has the original EBX reload and sum order. However, the compiler inserts a load/store pair of the x home before dispatch; case 3 loads x into EDX and folds the table operand into an `add` from memory. The three head differences remain. This is a separate diagnostic, not a production improvement. Ordinary and volatile integer snapshots instead produce 230 instructions with frame 20.
- **Combining that copy with the initial store does not remove its cost.** A single union home written/read through its integer member, with or without a float self-assignment, gives 230/725, frame 20. Copying the head's `xx` through a float alias gives 224/704, frame 16. Passing its bits through a head-local union into the shared snapshot gives 223/702, frame 16, aligned strict/rb 9/8. None satisfies the original extent. These f236 controls distinguish the useful shared-copy effect from a free choice of local type.
- **The earlier nine-difference diagnostic still needs its extra store.** In f231, direct union source homes preserve 222/700 and frame 16; float source homes give 222/700 with frame 20. Writing the case-local union result back to `tilex`, including consuming that assignment's destination, preserves f211's 222/700, frame 16 and strict nine. Using `base[0]` gives 224/706. In f234, `Pos`, pointer-to-one-row, and union views of the seat-table element preserve each seed: production ten at 222/698, float-copy nine at 222/700. Volatile table-x/both-field reads on the float candidate add instructions and reach 706 bytes.
- **Pointer representations, counter temporaries and backward case entries are inert.** Ten f227 forms retype the complete local person/record pointer as unsigned, void/char pointer, or a struct/union pointer member; all preserve production ten. Twelve f229 forms route all action increments, optionally joined/seated increments too, through the existing direction, seat, or a new byte temporary; production ten and alternate thirty-six remain unchanged. Twelve f232 forms move actions 0, 3, 8, or both 3/8 behind a preceding label entered by a backward goto. Jump threading preserves each seed exactly: ten, fifteen, thirty-six. No such control-flow spelling was retained.
- **Shared reload storage and complementary head reads still add storage.** In f228, a common scalar or record-union reload across cases 3/8 leaves the fifteen/thirty-six alternatives unchanged; production's early read still spills. Reusing the rider union requires retaining the saved tile as the y source and adds storage on all three seeds. Six f233 controls instead keep the actual cursor as an integer, overwrite it with x in both movement cases, and optionally consume it at the latch; inline/early reads give 229/727, and a while/break consumer gives 229/730, all frame 20. The latch tests an integer, never a freed rider pointer. In f230, equal/complementary arms for the head's base-x read produce 226–233 instructions and frame 24; merging two initialized arm values with OR is not free here.

The first f228 generator invocation stopped on a whitespace assertion before creating an alternate candidate; it was corrected and resumed without duplicate candidate records. Scratch sources, listings and audits remain local and untracked. A fresh fetch confirms `scope/F` at the preceding `74b571e8`, F-fable at `e9e1f914`, and main at `6a95613e`; there is no newer F work to incorporate. Continue from the strict-ten production body, with f203's alternatives and f235's shared-copy observation kept separate. The 100% goal remains active.

## Twenty-seventh continuation — both movement cases match behind a redundant byte store

**Production remains 15/16 exact, with Tower WIP at 10/222 strict differences, first 25, actual 222 instructions/698 bytes, frame 16.** The production body is still `acc92400`. A new alternative confines its normalized differences to the head: **f251_minimal_byte_home has 223 instructions/702 bytes, frame 16, RET-bounded aligned strict/rb 3/3.** It is one instruction/four bytes too long and is not promoted. All 85 candidates in f237–f251 compile cleanly with `/W3`, pass the whole-file 47-neighbor audit, and have independent RET-bounded measurements. `summarize_f237_f251.py` verifies the unique records with no excluded/error records. The original-size strict minimum remains ten; prior address-identity qualifications still apply.

### Reproduce the new alternative from production

Immediately after the head's existing `tilex = xx;`, add a write of the already-correct low byte:

```c
            tilex = xx;
            *(unsigned char*)&tilex = (unsigned char)xx;
```

Then replace **both** `*(volatile int*)&tilex` reads with plain `tilex`: case 3's target-x operand and case 8's `xx` initializer. Keep the named seat-table pointer, the saved tile-y reread, the car/sum splits, and all other code unchanged. The byte write preserves the complete integer value because `xx` came from an unsigned byte.

This is the complete f251 transformation; no union, float, extra declaration or helper is needed. f240 first found it with a zeroed union followed by a byte store (223/706). f241 changes that first store to the full `xx` value (223/702); f246 confirms that a two-member union, a plain integer and a one-element array reproduce it. The evidence points to the partial store changing value tracking; a floating representation itself is unnecessary.

`inspect_byte_home_alignment.py` checks the actual saved object through its RET. Original indices **0–24** equal compiled 0–24. Original **29–221** equal compiled **30–222**, preserving the register names, memory operands and instruction order after normalization. Thus both movement cases, every remaining action body, the loop latch and the return match that normalized sequence. All **14 direct internal branches** reach the corresponding original instruction after accounting for the insertion, and the candidate has 17 direct calls. Call/global relocation identities and the switch table remain separate gates; this is not a full address-verified match.

Only the following window remains:

```text
Original:  AL <- [tile]; EBP <- base_x; ECX <- base_y; [x home] <- EAX
Candidate: ECX <- base_y; AL <- [tile]; EBP <- base_x; [x home] <- EAX;
           BYTE [x home] <- AL
```

The candidate still has production's three-load permutation and an extra byte store. Its index-aligned audit count is 196 because the insertion shifts the later instructions; the aligned count of three must not replace the production marker or be presented as 3/222 strict.

### What the follow-ups establish

- **Removing the partial-store effect loses the matching tail.** Reversing the two stores, chaining their assignment, self-assigning the byte, deriving it from the newly stored whole value, or replacing the construction with full-word OR/ADD returns to 230/725, frame 20. An explicit byte source alias or one-byte memcpy retains the same 223/702 candidate. Zero-length copy/fill and a self-comparison are not free boundaries here: they add storage/code; their saved objects still contain 17 direct calls.
- **A different statement region does not merge the stores for free.** A one-pass for or default-only switch preserves 223/702. Do/while-zero and while/break regions reach 222/699 but change the subsequent sequence (aligned 43), so the missing instruction is not a successful store merge. Equal/complementary conditional arms give 238/752, frame 24.
- **The earlier head-read barriers do not transfer cleanly.** On f241, the x/y origin read barriers, the post-head x-sum split, and an ordinary first definition read add storage: f242 has 224–234 instructions and frames 20/24. Grouping x, next and the base array while making one or both reloads ordinary also adds storage in f248. A normal existing field store has not reproduced the partial-byte store's useful effect.
- **A full-width volatile bitfield gives a second bounded head alternative.** Writing a single `volatile unsigned value:32` member and later reading the union's integer view gives f247_32_none: **224/706, frame 16, aligned strict/rb 4/4, first 26**. The AL/base-x/base-y loads have the original order, but the compiler inserts a read and write-back of the x home and places the value store before the two origin loads. Signed spelling, a volatile enclosing struct, and pointer views reproduce that result. Plain volatile word members and whole-structure copies instead add a frame slot or lose the matching tail. Narrow volatile bitfields add substantially more code. No bitfield form is retained.
- **Direct representations and aggregate copies alone do not supply the missing effect.** Plain exit reads across integer/float/union homes give 229/723, frame 28. Head memcpy/union copies, word-pointer/struct/array/bitfield views, and the earlier shared-copy table barriers do not close the body. Initial byte construction and all later home reads must be assessed together; a close-looking copy at one site is not sufficient.

Evidence remains local in f237–f251, `f241_alignment.json`, and `f251_alignment.json`. A fresh fetch still finds F-fable at `e9e1f914` and main at `6a95613e`, with `scope/F` at the preceding `f91ed7e4`; no newer F work needs incorporating. Continue with production ten and the minimal f251 head-only alternative kept distinct. The complete audit/address gates, 16/16 target count, commit/push requirement and active 100% goal remain unchanged.

## Twenty-eighth continuation — verify the byte-home candidate's addresses

**Production remains 15/16 exact. Tower is still WIP at 10/222 strict differences, first 25, actual 222 instructions/698 bytes, frame 16.** No production source differs from `acc92400`. The 85 distinct candidates in f252–f265 all compile cleanly with `/W3`, preserve the 47 normalized-exact neighbors, and retain the original 17 direct calls in their actual RET-bounded bodies. `summarize_f252_f265.py` verifies those records and saved objects, with no excluded/error records. None of this batch has both the original instruction count and byte length. The best aligned diagnostic remains the separate f251 shape, not an improved production percentage.

**f251's call/global references and switch table now pass an explicit mapped diagnostic check.** `inspect_byte_home_relocations.py` uses the saved COFF object and reasserts the exact normalized instruction mapping, including the head permutation and extra byte store. All **21 call/global relocations** resolve to the original annotated addresses: 17 calls and four global operands, including the seat table's `+4` addend. The remaining instruction relocation selects the switch table; all **ten table entries** resolve to the corresponding original instruction targets. There are no mismatches or unresolved entries in this diagnostic. The earlier 14 direct internal-branch checks still apply.

The report is `f251_mapped_relocations.json`. It imports the existing relocation parser without editing it and maps instruction targets explicitly; it does not extrapolate equal byte offsets across the insertion or bypass the normal exact-function gate. f251 remains **223/702, frame 16, aligned strict/rb 3/3**, with the extra byte store and head load permutation. Its index-aligned strict count is still 196. This additional address evidence does not make it `[OK]`, nor change the previously recorded address qualifications for unchanged neighboring functions.

The new source probes narrow the remaining storage question:

- **Raw storage and earlier pointer lifetimes do not suppress the ordinary-read spill.** Byte/halfword arrays and byte structs (f252), pointer/enum/unsigned views of the x home (f262), and reusing that home for the tile, record or next pointer before assigning x (f263) all return to **230/725, frame 20**. The pointer representations are never dereferenced as coordinate-derived addresses.
- **Moving the redundant stores does not merge them.** In f253, placing the pair before either origin assignment preserves 223/702. Moving the pair or byte store past the x sum preserves that extent and aligned three; moving it past y or the base sum adds storage. Two halfword stores in f254 give **222/704, frame 16, aligned 7/6**; four-byte and bitfield constructions are larger. Full initialization is retained in every split representation.
- **Wider homes and existing neighboring stores do not reproduce the useful partial-write effect.** f257's 64-bit, paired-coordinate and floating representation views have 230–233 instructions and frames 20–28. f258's complete 8/24 or 31/1 bitfield writes, with a volatile low member, give 241 instructions. Grouping x with the base array and varying the existing sum store (f259) also adds storage; the float head transfer reaches 224/704 but needs frame 20.
- **Value identities and conditional assignments do not provide a free boundary.** Absolute-value/range identities (f255), zero/complementary/merged assignments (f260), and zero/full-turn rotation intrinsics (f261) all miss the original extent. Saved-object inspection confirms that the intrinsic probes add no calls. The six scratch-only inline-helper diagnostics in f256 also retain 17 calls but fail the extent/allocation gates; no helper is added to production.
- **A partial write to a short-lived head temporary still costs code.** f265 moves the byte update from the shared home to an integer or union temporary, using a saved byte, saved integer, repeated tile-byte source or one-byte copy. Those forms have 231–235 instructions and frames 20/24. The compiler does not remove the partial write while preserving f251's matching tail. Additional volatile tile-byte reads on f251 (f264) likewise enlarge the body to 232–234 instructions.

A fresh fetch confirms `scope/F` at the preceding `f2fb7c32`, F-fable at `e9e1f914`, and main at `6a95613e`; there is no newer F work to pull. Scratch evidence stays local and untracked. Continue from production ten and the now address-checked f251 diagnostic, preserving the full audit/extent gates and the active 100% goal.

## Twenty-ninth continuation — copy regions and head ordering do not close the byte home

**Production remains 15/16 exact, with Tower WIP at strict/rb/ob 10/8/10, first 25, actual 222 instructions/698 bytes, frame 16.** No production source differs from `acc92400`. The 59 candidate/control records in f266–f275 compile cleanly with `/W3` and preserve all 47 normalized-exact neighbors. Each has an independent RET-bounded measurement. `summarize_f266_f275.py` checks the records, audits and saved objects. None has both the original instruction count and byte length; the closest aligned result remains the separate **f251: 223/702, frame 16, aligned 3/3**.

- **One-word copy loops change the result but do not produce a free ordinary reload.** Scalar/index one-iteration loops in f266 give 223/706, frame 20, aligned strict/rb 112/33; pointer loops give 229/725. A default-only switch preserves the ordinary-home 230/725 result. In f267, naming the memcpy length, guarding it with the already-nonnull rider/record, or expressing it using the byte range also misses the original extent. The rider-guard and complementary forms have **18 static call sites**: COFF inspection shows a third `NewDirForAction` relocation instead of the original two shared sites. There is no added memcpy callee. These two forms lose the original call-tail merge and are excluded as viable matches; the other 57 records retain 17 body calls.
- **The new byte-home shape still ignores ordinary head reordering.** All five alternate declaration-initializer orders for `xx`, `bx`, and `by`, plus either/both commuted sums, preserve 223/702 and aligned three in f268. Float representation reads of one or both origin fields and an array/struct origin pair (f269) introduce extra storage and instructions. The source-order and representation levers have now been tested on f251 itself, rather than inferred from the older ten-difference body.
- **Whole copies of volatile bitfield structures do not retain the useful direct-bitfield shape.** In f270, a volatile enclosing 32-bit bitfield structure copied from an integer view or explicit bitfield temporary gives 227/720, frame 20. Making both the member and its temporary volatile gives 235/745, frame 24. This distinguishes whole-structure copying from f247's direct full-width bitfield assignment; neither removes its extra operations. Narrowing only the head's `xx` temporary to byte/short types (f271) also fails: the ordinary-home short forms recover frame 16 but still have 226 instructions, while the byte-home versions enlarge the frame.
- **The partial byte write does not combine with the older low-register-blind alternatives.** f272 applies it, with both x reads ordinary, to the fifteen/thirty-six-difference f203 seeds. Keeping the rider-relative y read gives 223/704, frame 16, aligned rb 20/18. Restoring the saved tile pointer, removing the table read barrier, or inlining the x operand adds frame space or changes the tail further. Those crosses do not supersede either original f203 alternative or f251.
- **Array lifetime is inert on both sides of the partial-write boundary.** In f273, a one-element x array declared at function scope, at loop entry, or in a block after the record guard gives 230/725, frame 20 with a single word store, and 223/702, frame 16 with the redundant byte store. Moving the address-taken array's declaration does not erase the byte instruction or preserve its effect after removal.
- **Proven value relations and late-width identities do not erase the cost.** The f274 `__assume` facts follow solely from the preceding unsigned-byte assignment: whole-value equality, low-byte equality, and zero upper bits. Before or after the redundant byte write, they produce 234/745, frame 24; none is retained. f275 keeps the saved home 32-bit while using 64-bit intermediate round-trips, masks, high/low multiplication or addition. Four reduce to 230/725, frame 20; the shift round-trip gives 234/736. All five retain 17 calls.

A fresh fetch still finds F-fable at `e9e1f914`, main at `6a95613e`, and `scope/F` at the preceding `37867405`. The tracked source remains unchanged; scratch evidence stays local. Continue from production ten and the address-checked f251 diagnostic. These measured exclusions are not an impossibility result, and the 100% goal remains active.

## Thirtieth continuation — a composed word retains two stores

**Production remains 15/16 exact. Tower is WIP at 10/222 strict differences, first 25, actual 222 instructions/698 bytes, frame 16.** No production source differs from `acc92400`. All 61 candidate/control records in f276–f286 compile cleanly with `/W3`, retain 47 normalized-exact neighbors, and have 17 calls in their independently measured RET-bounded bodies. `summarize_f276_f286.py` verifies the records, audits and saved objects. None has both the original instruction count and byte length. The production marker and the separate, address-checked f251 diagnostic remain unchanged.

- **Constructing two halfwords and then copying the whole object recovers the normalized movement code, but still emits two stores.** In f276, build a four-byte `struct HalfWord { unsigned short low, high; }` with `low = (unsigned short)xx` and `high = 0`, then copy it into the shared x home's union view. The struct copy, zero-initialized struct, whole-union copy and integer-member copy give the same **223/706, frame 16, aligned strict/rb 4/4** body. The differences are confined to the head: the existing load permutation remains, the original dword store becomes `mov word ptr [esp+10h],ax`, and `mov word ptr [esp+12h],0` is inserted before the y-byte load. The subsequent normalized sequence, including both movement cases and RET, matches. This is eight bytes too long and is not promoted. Zero-initializing the whole source union gives 222/705 with a folded case-3 table operand instead; a float transfer adds storage.
- **Copying the existing integer through that type does not preserve its benefit.** Direct halfword/byte/bitfield structure views of `xx` in f278 return to **230/725, frame 20**. Reading the two source halfwords separately gives 225/713, frame 20. Deriving both halves arithmetically from the same `xx` gives 225/709, frame 16; the compiler retains the high-half calculation. The partial construction, rather than the four-byte structure type alone, causes the useful reload behavior.
- **Inline returns do not combine those stores.** The four f285 **scratch-only helper diagnostics** return the constructed halfword struct, union, or integer representation, or fill the destination through a typed output pointer. Every measured body is normalized-identical to f276's 223/706 result and retains 17 calls. No helper is retained. `f276_f285_equivalence.json` records these full-body comparisons, together with the equivalent local-copy forms; these are stronger checks than matching summary scores alone. This alternative's address identities remain a separate gate, unlike the already checked f251.
- **Reusing the dead coordinate for direction or movement arguments does not produce a free initial spill.** In f277, using x's low byte for the later heading or movement direction is inert at 230/725, frame 20; using an integer for all three movement directions adds code. Caller-local one/four-byte direction argument objects in f281 also miss the extent and allocation gates, although all retain 17 calls. In f282, reusing x's union for a later world/target `Pos` copy is inert; grouping both cached coordinates into that `Pos` needs frame 24. Every overwrite occurs after the coordinate's last use in the selected case, with the original direction-byte truncation preserved.
- **Recovering frame size alone does not recover the original spill.** Narrow y temporaries (f279) and a y consumer immediately after its load (f280) can give frame 16, but still have 226 instructions. Their listings save the definition pointer in the original x slot and spill the rider into the argument slot; x remains in a register. This is not the original x spill. A consumer after the complete head is inert. Restoring either or both x-read shims on the immediate-y-consumer form in f286 gives 229–231 instructions and frame 20, without closing the allocation problem.
- **Unused qualifiers and unevaluated volatile views are inert.** An unused volatile scalar, nested member or bitfield beside the ordinary x member (f283) does not block promotion. The f284 `__assume` expressions use only assignment-derived equality/range facts or the unconditionally nonzero `value | 256u`; their volatile views add no runtime access. All ten saved bodies are normalized-identical to the ordinary-home 230/725 baseline, as checked in `f276_f285_equivalence.json`. No such declaration or assumption is retained.

A fresh fetch finds F-fable at `e9e1f914`, main at `6a95613e`, and `scope/F` at the preceding `9957e89f`, with no newer F work to incorporate. Scratch evidence stays local and untracked. Continue from production ten and f251, keeping the two-halfword alternative separate and retaining the full instruction, extent, address and publication gates. The 100% goal remains active.

## Thirty-first continuation — dead accesses and loop-carried storage

**Production remains 15/16 exact. Tower is still WIP at strict/rb/ob 10/8/10, first 25, actual 222 instructions/698 bytes, frame 16.** No production source differs from `acc92400`. All **64** candidate/control records in f287–f297 compile cleanly with `/W3`, preserve the 47 normalized-exact neighbors, and retain 17 calls in their independently measured RET-bounded bodies. `summarize_f287_f297.py` checks every record, audit and saved object. The only original-size result, f297_no_byte_3, is normalized-identical to production; there is no new close.

- **Putting the byte write on an unreachable edge does not leave a free spill.** f287 tests constant-false, already-nonnull rider/record, and assignment-derived equality guards. f288 adds a forward skip, zero-trip loop and guard-derived `__assume` controls. The nonnull and equality facts follow from the existing loop/record guards and immediately preceding private assignment; no unsupported assumption is retained. Most folded forms produce 223/706 with frame 20 or the ordinary-home 230/725. The rider guard remains as a test/branch and byte store, changes the rider to EBP and needs frame 20. The zero-trip forms recover frame 16 but still spill the rider and have 226 instructions.
- **Empty partial-read and address conditions expose the same boundaries.** Byte/halfword reads, address casts and local-pointer comparisons in f289/f292, followed by the position controls in f293, do not reproduce the desired x spill. Sixteen new bodies across these and the guard families are normalized-identical to f266_for_index: 223/706, frame 20. Conditions after the x sum or y load instead give 226/716, frame 16; their instruction stream still differs substantially. Discarded reads and conditions after the complete head return to the ordinary-home 230/725 result. No uninitialized coordinate is read in the before-store control.
- **A coordinate local reused as the next iteration's cursor does not help.** f290 transfers the already-saved next pointer into the x union or integer at the loop boundary, then restores the current rider before assigning x. The ordinary-read forms are identical to 230/725, frame 20. Keeping both x-read shims gives 231/725, frame 24. The transfer never consults the freed rider after case 9.
- **Initializing the upper bytes once preserves the later normalized code but changes the body.** f291 initializes x to zero at declaration or immediately before the loop, then updates only its low byte/halfword each iteration. These valid full-integer representations give **223/706 or 223/707, frame 16, aligned strict/rb 4/4**. Saved-object comparison shows an extra zero initialization, the existing three-load head permutation and a byte/word store replacing the original dword store. The sequence after that head store matches through RET. This is another diagnostic, not an original-size result; its address identities have not received f251's separate mapped-address check.
- **Inlining does not hide an address use or recover the ordinary spill.** All six scratch-only empty/self-word/self-byte pointer helper probes in f294 are normalized-identical to the ordinary-home baseline. f295 passes the x home's address into the actual target-calculation helpers, with early or inline reads: the seat-only versions are likewise identical to 230/725; including the exit helper gives 226/716, frame 16. All ten fully inline with 17 calls, and no helper is added to production.
- **Definition-read views distinguish another nonmatching case-3 shape.** On f251, whole volatile pointer structs/unions and unsigned reads (f296) are normalized-identical to 223/702 and aligned 3/3. A volatile full-width bitfield view gives **222/700, aligned strict/rb 6/5**: it retains the extra head byte store, reloads case-3 x into EDX, and folds the table load into an `add` from memory. Using that view for either one or both definition reads is equivalent. Removing the byte write in f297 returns to ordinary-read spills; restoring both x-read shims returns exactly to production ten. The matching instruction count in the bitfield diagnostic does not compensate for its extra bytes or missing separate table load.

`f287_f297_equivalence.json` groups complete normalized RET-bounded instruction streams, not just their scores. **42 of the 64** match one of four saved baselines: production, the ordinary home, f251 or f266_for_index. This records which apparent source changes are inert and prevents their repetition. The earlier address qualifications for unchanged neighbors remain in force.

A fresh fetch finds `scope/F` at the preceding `d8ffd1ef`, F-fable at `e9e1f914`, and main at `6a95613e`; there is no newer F work to pull. Scratch evidence remains local and untracked. Continue from production ten and the address-checked f251 diagnostic, keeping the full audit, actual extent and address gates. The 100% goal is active; these exclusions do not establish an impossibility result.

## Thirty-second continuation — initialized homes and inline head ownership

**Production remains 15/16 exact, with Tower WIP at strict/rb/ob 10/8/10, first 25, actual 222 instructions/698 bytes, frame 16.** No production source differs from `acc92400`. All **60** candidate/control records in f298–f308 compile cleanly with `/W3`, preserve all 47 normalized-exact neighbors, and retain 17 calls in independently measured RET-bounded bodies. `summarize_f298_f308.py` checks every audit and saved object. Ten records have the original instruction count and byte length: one is identical to production, and nine are the same nonmatching inline-head diagnostic with frame 20 and 94 strict differences. No result improves the production score.

- **Initializing the coordinate at its declaration does not suppress promotion.** f298 extends the existing head block through dispatch and initializes x from `xx`, using an ordinary scalar control, const scalar, const struct/member, const union or const array. All six are normalized-identical to the ordinary-home **230/725, frame 20** baseline. Leading/trailing unnamed zero-width bitfields around a single integer member in f299, with direct assignment or whole-object copy, are also identical. Each type is checked to occupy four bytes; no padding or uninitialized data is read.
- **Evaluated volatile aggregate accesses cost code.** f300 discards a volatile four-byte struct/byte-array/union/bitfield-struct value after the x assignment or after the complete head. The four types give identical results at each position: **232/736 or 224/710, both frame 24**. These are evaluated accesses, unlike the earlier unevaluated assumptions, and they do not provide a free home. f304's volatile one-byte bitfield or whole one-byte struct/array source reads likewise add instructions and frame space on both production and f251. The reads stay within the original x byte.
- **Dead writes in other switch arms do not retain the useful partial-write effect.** f301 writes x's low byte after its last use in cases 3/8, or in cases 4/5 that never consume x; a full-word case-5 control behaves the same. Those six bodies are normalized-identical to 230/725. A dead byte write at case 9's join changes allocation to 230/728, frame 20. Saved-object inspection confirms that its byte store is absent; the altered result is not an extra retained write. Every next iteration overwrites the full coordinate before using it.
- **The in-place seat-index shift still does not produce the natural x spill.** f302 removes the case-3 x-read shim, keeps the pre-shift table pointer, and places `if (seat >>= 1) { }` before x, before y or before the movement call. With the case-8 shim removed or retained, these give 225–231 instructions and frames 20/24. The before-x and before-y forms are equivalent within each pair, but none has the original extent.
- **Zero-valued address dependencies either disappear or add work.** f303 adds an assignment-proven zero offset to the base-y field address, using the unsigned byte's upper bits or its difference from its byte conversion. On production and f251, those forms need 227–237 instructions and frames 20/24. The `bx-bx` controls are normalized-identical to their respective seeds. f305's named or inline `__assume` facts assert only that the just-loaded byte shifted right eight is zero; they still give 224–229 instructions with larger frames. No assumption is retained.
- **A full inline head changes which value is spilled, but does not match.** f306 moves the head into a scratch-only helper that writes x, y and the y sum through output pointers and returns the x sum. Both with and without the caller's empty x-sum consumer, the result is **222/698, frame 20, strict/rb/ob 94/72/87, aligned strict/rb 71/30**. `f306_head_alignment.jsonl` records the actual comparison: the rider remains EBX, but the tile pointer is saved in the local slot where the original saves x; x remains in EDX through dispatch, and y occupies another local. The head also contains only one definition-pointer reload. This is a different allocation, not a successful original x spill. f307's parameter qualifiers and f308's actual volatile union-member arguments all reproduce the same normalized body. No helper lands.
- **Wrapping the whole body is also controlled separately.** f306's fully inlined ordinary-home body remains 230/725. Wrapping f251 instead gives **222/700, frame 16, aligned 6/5**, normalized-identical to f296_bitfield: the redundant head byte store remains and case 3 folds its table operand into a memory add. This does not improve either seed.

`f298_f308_equivalence.json` groups the complete normalized instruction streams through RET. Twenty records match saved baselines; all nine inline-head forms match one another. These comparisons prevent matching scores alone from being mistaken for identical code. The earlier mapped-address evidence applies to f251 only; the existing qualifications for unchanged neighbors remain in force.

A fresh fetch finds `scope/F` at the preceding `c926e74b`, F-fable at `e9e1f914`, and main at `6a95613e`, with no new F work to incorporate. Scratch evidence remains local and untracked. Continue from production ten and the address-checked f251 diagnostic, preserving the full audit, extent and address gates. The 100% goal remains active; the tested exclusions are not an impossibility proof.

## Thirty-third continuation — value objects and copy results remain nonmatching

**Production remains 15/16 exact. Tower is WIP at strict/rb/ob 10/8/10, first 25, actual 222 instructions/698 bytes, frame 16.** No production source differs from `acc92400`. All **50** candidate/control records in f309–f319 compile cleanly with `/W3`, preserve the 47 normalized-exact neighbors, and have independent RET-bounded measurements. Eleven have the original instruction count and byte length, but none improves on production ten. `summarize_f309_f319.py` also inspects saved COFF call relocations: every body's 17 call symbols, including their multiplicities, match production. There is no hidden helper/runtime call or floating-point instruction in this batch. This call inventory is not a new full address-identity check.

- **Returning all four head values as one object adds storage.** In f309, an inline helper returns `{x,y,tx,ty}` or fills it through an output pointer; the caller either copies its members to the existing locals or uses the members through dispatch. All four bodies are normalized-identical at **229/724, frame 24**. f310 instead passes the two definition pointers into the head helper by value, with direct volatile argument reads or explicitly named caller reads and either formal order. Those retain 230/725, frame 20; changing where the reads occur does not recover the original x spill. No helper is retained.
- **Dead writes across the loop boundary are inert.** f311 places an ordinary low-byte write before the initial tick, before record lookup, before/after the next-rider assignment, or after the loop; a whole-word latch control is included. Every write is dead because the full x value is overwritten before its next use or the function exits. All six are normalized-identical to the ordinary-home **230/725, frame 20** baseline. f312's signed/unsigned 31-bit fields with a fully initialized upper bit also fail: initialization from the complete x value is inert, while zero initialization gives 230/732. No uninitialized bits are read.
- **Call representations and complete node-field reads do not swap the alternate's rider register.** f313 preserves the existing 32-bit return/argument representations while spelling FindRecord's return as unsigned/void-pointer or TakeSeat's arguments as four-byte pointer structs. Production remains ten and f203_f182_0 remains **36 strict / 3 rb**, both at 222/698. On that alternate, f314's ordinary/volatile whole-pointer next-field reads and ordinary bloke-field copies are likewise inert. A volatile whole bloke-field copy worsens it to 37/6; none recovers the original rider/coordinate register choices. Shared extern types remain unchanged.
- **Floating identities and const initializers do not remove the diagnostic's copy.** f315's unary plus, float/double round trip, multiplication by one and addition of zero all compile without floating operations and are normalized-identical to **f211_3_float: 222/700, frame 16, strict nine**. f316's initialized union/one-float struct, with or without const qualification, gives the same body. Scalar float initializers instead reproduce f217_scalar_alias at 224/706, frame 20. The two excess bytes still disqualify the nine-difference result.
- **An inline bit-copy result reproduces the known extra pair.** f317 loads a volatile float representation through a home pointer, returns its integer member, and uses it early or directly in the case-3 expression. f318's const union initializer or four-byte result struct behaves identically. All four are normalized-identical to f217_union_memcpy: **224/706, frame 16, aligned strict/rb 7/4**. `f317_home_alignment.jsonl` confirms the remaining case-3 pattern: x is initially loaded into EDX, then written back to the same home and reloaded into EBX before the original table/add sequence. The head permutation and case-8 EDI choice also remain. Returning the bits does not remove that load/store pair.
- **Changing the helper's whole-copy form loses that result rather than improving it.** f318's integer-first union view and f319's memcpy-to-union/scalar, whole-struct return and whole-union initializer are normalized-identical to f297_no_byte_2 at **228/724, frame 20**. Passing a float value to the helper in f317 also needs frame 20 and 224 instructions. The relocation inventory confirms that these helpers and memcpy forms introduce no new runtime calls, but they still fail the original body gates.

`f309_f319_equivalence.json` compares complete normalized bodies through RET. **38 of the 50** reproduce seven saved baselines; the remaining twelve form six other groups. Future work should change the live value structure rather than repeat these grouped initializer, input-representation and copy-result spellings. Their failure is not a general impossibility result.

A fresh fetch finds `scope/F` at the preceding `c5e5dc11`, F-fable at `e9e1f914`, and main at `6a95613e`, with no new F work to incorporate. Scratch evidence remains local and untracked. Production ten and the separately address-checked f251 diagnostic remain the reference points. The full audit, actual extent and address gates are unchanged, and the 100% goal remains active.
