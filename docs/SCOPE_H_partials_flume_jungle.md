# Scope H — partials: log flume, jungle cruise, animation (2026-09-05)

**Read `docs/PARALLEL_CONTRACT.md` first, including its "Extra rules for
PARTIAL scopes".** Branch: `scope/H`. Notes: `docs/lanes/scope-h.md`. Object
prefix: `/tmp/sh_`.

Thirteen existing `// WIP-FUNCTION:` bodies. The log flume is the
best-documented subsystem in the tree (`logflume.c`..`logflume7.c`,
`lfentrance.c`, `lfmisc.c`, all recent, most exact), and two of its partials
are one or two instructions from closing. **Edit only these bodies and their
notes; never a `// FUNCTION:` body.**

| mismatch | file | address | function | what the note says |
| --- | --- | --- | --- | --- |
| 2 | `logflume4.c` | 0x0040ca60 | `LFTrack_DrawAlt` | index 84 only: which scratch register carries the `mode` argument in the one arm that passes it (`mov ecx,[esp+1Ch] / push 0 / push ecx` vs ours in eax); ~75 spellings; the four arms are 2×2 with B/C cross-jump-merged and A/D separate because A passes `mode` — an original quirk |
| 5 | `logflume3.c` | 0x0040f050 | `LFTunnel_Place` | the head's nine coordinate instructions are the same multiset permuted — VC6 schedules the `py` load late and the deferred `push edi` early; 240 head orders ruled out; the shared two-byte `BPos c` aggregate is already in (it took 164 → 5) |
| 10 | `logflume3.c` | 0x0040dc00 | `LFCorner_Place` | case 0's first store block hoists the `g_lftr_def` load above the `run` load; the single missing byte follows (EDX 6-byte form vs EAX's 5-byte `A1`); nineteen of twenty sub-piece blocks exact |
| 10 | `lfentrance.c` | 0x0040bf70 | `LFEntrance_Activate` | everything follows from one binary choice — which of `{qx, next}` takes ecx vs edx; the spill-store position is an effect, not a cause; two shims beat 10 (a one-field struct through `*(RiderNode* volatile*)&nx.p` → 9; a volatile read of `r->next` → 8) but were not committed — emission order and coalescing are independent |
| 11 | `logflume.c` | 0x0040abf0 | `LFEntrance_Remove` | the trigger is one extra IR tuple before the x statement (NOT "naming the Y byte"); every code-free tuple candidate is forward-substituted away; the destination-symbol lever does not reach a byte add |
| 44 | `logflume2.c` | 0x00410180 | `LFDrop_Place` | the EBX tie-break between two byte coordinates: the shared `BPos c` aggregate that fixed the four `logflume3.c` siblings does NOT transfer here because `c.x` is written once and never again (both halves updated is the condition); `int y` gives the original's whole 18-instruction head plus one spurious `and ebx,0xff`; `int x` gives 341/341 bytes but a dword head |
| 206 | `lfentrance.c` | 0x0040a600 | `LFEntrance_Add` | the p1 store block is an ESCAPE problem: two plain int scalars for the head/p1/loop `Pos` reproduce the whole p1 block instruction for instruction and flip the callee-saved tie — but the head then folds two defs into `lea eax,[eax+edx+1]`; the head coordinate is a compiler temporary lifetime-coloured onto the tail `Pos`'s home |
| 27 | `junglecruise.c` | 0x00436dc0 | `JungleCruise_UpdateRiverTile` | read the note |
| 112 | `junglecruise.c` | 0x00432d00 | `JungleCruise_UpdateRiverAnim` | 81 of 112 recorded as unreachable (a proof of unreachability, not exhaustion) — verify the remaining 31 only |
| 208 | `ridecb2.c` | 0x00435750 | `JungleCruise_Tick` | read the note |
| 105 | `jcroute.c` | 0x00437260 | `JungleCruise_TraceRoute` | a compiler PHASE-ORDERING difference: VC6's callee-saved ranking inverts when a loop exists at IR time; the original has the loop-free allocation WITH the loop (its tail-call conversion ran after allocation, ours before); byte-exact; the four-line probe in the note says whether the regime is reachable — it was not; retire unless the newest levers change that |
| 30 | `anim2.c` | 0x00418fe0 | `BoatingSchool_DrawBoats` | the original's two sums are flat FOUR-term chains; written flat, VC6 fully reassociates and sorts by descending definition point where the original has source order; the Y sum's one-term second field emits the original's order — build on that shape; the residual `lea ecx,[eax+edi]` is not independent of the index-82 rotation |
| 182 | `anim2.c` | 0x00442040 | `AnimApplyPart` | the residual is a HOIST, not a mirrored spill group: the original keeps `srcx` on the x87 stack and computes u0 with four MEMORY operands because the stack is exactly full (`srcx` at `st(7)`); ours converts the four Y values above u0 and spills them; conversion order is proven from the operand displacements — do not re-derive it |

**Where to start.** `LFTrack_DrawAlt` (2) and `LFTunnel_Place` (5) with the
newest levers — "the for-increment/statement order of two derived values is
emitted REVERSED", "define the doubled one first to get the `lea`", "a
coordinate pair computed into ONE `Pos` aggregate local", "for two derived
quarter-steps write `dy` before `dx`". `LFEntrance_Activate`'s note names
two shims that beat it; the question is whether a non-shim spelling of the
same effect exists now ("naming the intermediate POINTER advances the
rotation where volatile does not"). `AnimApplyPart` needs the x87 depth
levers recorded after it (`fsub st(3)`, in-place idioms, depth forcing
siblings to memory) — that one may be reachable now.

**Files you own for this scope:** `logflume.c`, `logflume2.c`, `logflume3.c`,
`logflume4.c`, `lfentrance.c`, `junglecruise.c`, `ridecb2.c`, `jcroute.c`,
`anim2.c` — WIP bodies and their notes only.
