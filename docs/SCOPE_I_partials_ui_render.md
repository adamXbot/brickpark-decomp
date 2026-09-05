# Scope I — partials: UI, system, sim, render (2026-09-05)

**Read `docs/PARALLEL_CONTRACT.md` first, including its "Extra rules for
PARTIAL scopes".** Branch: `scope/I`. Notes: `docs/lanes/scope-i.md`. Object
prefix: `/tmp/si_`.

Fourteen existing `// WIP-FUNCTION:` bodies across UI, system, sim and
render. They range from a single-digit residual to the two largest partials
in the tree; take them in the order given and stop where the triage says to.
**Edit only these bodies and their notes; never a `// FUNCTION:` body.**

| mismatch | file | address | function | what the note says |
| --- | --- | --- | --- | --- |
| 5 | `fpui5.c` | 0x00471ca0 | `RemoveNewObjectMarker` | the inner shift loop's cursor anchors on the source `&spr[j]` where the original anchors on the destination `&spr[j-1]`; VC6 anchors on the element indexed by the PLAIN induction variable and the anchor drags the counter's placement with it (~40 builds) |
| 6 | `sysmisc.c` | 0x004966a0 | `UpdateSampleSource` | a free volatile READ is emitted FIRST in its basic block, not at its statement position; case 3 needs `s->src.pos.y` loaded on the near side of the volatile store to `p.x`; other volatile placements break the tail merge |
| 8 | `sysmisc.c` | 0x004401b0 | `UpdatePersonPos` | read the note (the `flags62 & 0x100` narrowing and the read-before-call locals took it 74 → 8) |
| 10 | `workorder3.c` | 0x00482430 | `BuildPTPRoute` | byte-exact, ALLOCATION: declaration order sets only the order of the leading `xor`s (worth 2 of 10); a local copy of a pointer does not grow its web; the switch's case order is inert because it lowers to a compare chain |
| 13 | `popup.c` | 0x004724a0 | `DrawPopUpInfo` | 13 of 962; both wanted behaviours demonstrated without a barrier (a plain permutation sinks the `ty+0x22` spill below the push run); aggregates do not force a memory home; the plain route caps at 53 |
| 20 | `objmap2.c` | 0x0048a3e0 | `GetObjectUID` | register-blind, only lines 95–98 and 140–143 differ: one `mov reg,[g_map]` per horizontal probe that the original puts at the probe region entry (a landing pad three edges jump past); 12 of the 20 are the esi/edx role swap that follows |
| 26 | `savechunks2.c` | 0x0046c7e0 | `LoadScriptEvent` | VC6 lays cold blocks in source-generation order and a single-predecessor `goto` target is generated INLINE with the branch that reaches it; global constant propagation deletes a whole `head = 0; return head;` block; the two are rigidly coupled — every spelling that keeps the join drags the block adjacent to its `if` |
| 34 | `workorder3.c` | 0x00499d60 | `UnlinkGardenerOrder` | STRUCTURAL (block layout): two written-out copies of a tail merge only if instruction-identical; a pair of volatile reads hoisted into locals makes them so, and then the merge glues the survivor to the LAST arm — the mirror of the original |
| 35 | `fpui4.c` | 0x0046d850 | `ScrollIconPanel` | byte-exact, one inverted allocation rank across four values (original `list_x0→edx, list_y0→edi, nx→ebp, ny→ebx`; ours the reverse), rb 10; 24 write-back orders, both operand orders, aggregate/`register`/scoped-temp spellings, free volatile on both axis sites and both box edges all 34–35 |
| 82 | `workers2.c` | 0x00470620 | `CheckWorkerOnMouseStatus` | read the note |
| 118 | `screens2.c` | 0x0048f0f0 | `InitExitCheckBox` | 118 of 119 — essentially wrong; a reconstruction-error pass, not a variant search |
| 273 | `bigrender.c` | 0x0045ff00 | `RenderCursor` | residual (b) is recorded as unreachable (this build's merge floor 6 is below the corpus minimum 7); check the rest of the note |
| 389 | `render4.c` | 0x004608c0 | `PaintTileLayer` | block layout exact (four switch arms + table, both written-out cell copies, the `cell + 1` fast path, the cross-jumped bridge halves, the 0x34 frame); residual is an allocation permutation `{tile.x: ecx, tile.y: ebx, px: edi}` vs ours `{edi, edx, ebx}` and `tw` in ebp vs ecx; a free volatile on `halfw` killed a derived IV but moved its home up the frame — look for a non-volatile refusal |
| 381 | `renderview.c` | 0x0045b180 | `RenderView` | propped up by TWO known reconstruction errors that each cost strict alone (`xlimit`/`ylimit` belong after the quadrant switch; `g_sort_count = 0` belongs inside `if (cell->obj != 0)` — proven non-observable, all 13 references to the global are inside the function); try them TOGETHER; the frame is a reference-COUNT problem (117 vs 121 references over the low thirteen slots) |
| 844 | `renderview.c` | 0x004567a0 | `RenderFullMap` | 303 of 465 structural slots are ONE layout bit: the 0x400 else arm has to END IN A JMP to be exiled and no source spelling produces that with a shared tail (a partial cross-jump merge is impossible); six reconstruction facts are listed in the note (`scale_x` is not one web; `def` is spilled; `bpos` hoisted; `chain` in eax + spill home; `LineTo` import not cached; the ILF loop carries an ADDRESS) — those are the reachable part |

**Not in this table and not to be reopened:** `InsertChildIntoList`
(`fpui.c`), `ClampPopUpToScreen` (`misc3.c`), `RequestRoute` (`simcore.c`),
`WW_AnyBlokeInRect` (`waterworks.c`), `ValidateCursor` (`objmap2.c`),
`UpdateControllerFromMouseData` (`input.c`), `Draw3DPersonModel`
(`person3d.c`) — all EXHAUSTED.

**Where to start.** `RemoveNewObjectMarker`, `UpdateSampleSource`,
`UpdatePersonPos` and `BuildPTPRoute` first (small, recent, precise notes).
`InitExitCheckBox` is the one genuinely unexplored body — treat it as a
fresh reconstruction. On `RenderFullMap`, apply the six listed reconstruction
facts and measure; the layout bit is argued unreachable, so the goal there is
the reachable residual, not a close.

**Files you own for this scope:** `fpui5.c`, `sysmisc.c`, `workorder3.c`,
`popup.c`, `objmap2.c`, `savechunks2.c`, `fpui4.c`, `workers2.c`,
`screens2.c`, `bigrender.c`, `render4.c`, `renderview.c` — WIP bodies and
their notes only.
