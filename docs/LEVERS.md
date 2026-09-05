# VC6 SP3 codegen levers

A symptom-first reference for matching LEGOLAND with VC6 SP3 (`/O2 /Gy /Gd`). Consolidated from the source snapshot at `36018920b54c65f840ef3bbb5754ca37d3b1e258` (2026-09-05), under [scope M](SCOPE_M_levers_consolidation.md). [DECOMP](DECOMP.md#vc6-sp3-codegen-levers-learned-the-hard-way-on-loadbasemap) remains the historical record.

Start with the symptom index, then read the named rule and its evidence. Numbers are historical measurements on the stated body and source shape, **not new compiler runs or current completion claims**. A floor bounds its tested baseline; a corrected rule takes precedence over an older negative. `strict`, `rb` and `ob` mean strict, register-blind and offset-blind mismatch; `i` means instructions and `B` means bytes. Exact status still requires the full-body `audit.py` gate and correct symbol identities.

Coverage: **493 top-level DECOMP entries (including 76 nested bullets) → 194 consolidated entries**. Some source bullets contain several rules, so their source IDs legitimately map to several destinations. Every source entry has a reverse mapping below. Introductory discovery prose and the final “Next” transition are provenance, not additional levers. Counts use top-level bullet boundaries; nested and bundled observations explain the difference from the brief’s approximate count.

## Symptom index

| What you see | Start here |
| --- | --- |
| An else arm belongs after the epilogue | [BL01](#bl01) · Block layout |
| A shared tail survives in the wrong arm | [BL02](#bl02) · Block layout |
| Two identical-looking tails refuse to merge | [BL03](#bl03) · Block layout |
| One tail is copied while another is jumped to | [BL05](#bl05) · Block layout |
| The return target has the wrong branch width | [BL08](#bl08) · Block layout |
| Moving a label does not move its block | [BL09](#bl09) · Block layout |
| Changing switch cases does nothing, or moves every block | [BL10](#bl10) · Block layout |
| A saved-register push needs to sink past a guard | [BL13](#bl13) · Block layout |
| The original contains dead dec/inc around a loop | [LP01](#lp01) · Loops |
| The original uses add with a negative constant | [LP02](#lp02) · Loops |
| The peeled test is the wrong condition | [LP04](#lp04) · Loops |
| Two cursor increments appear in the wrong latch order | [LP06](#lp06) · Loops |
| One walking cursor should be two, or two should be one | [LP07](#lp07) · Loops |
| The cursor points to the wrong field in each record | [LP08](#lp08) · Loops |
| A second derived induction variable should disappear | [LP14](#lp14) · Loops |
| An entire body is one scratch register out of phase | [RA01](#ra01) · Register allocation |
| The original has one more callee-saved push than we do | [RA03](#ra03) · Register allocation |
| The wrong saved register carries zero | [RA09](#ra09) · Register allocation |
| Three constant stores will not form a zero web | [RA08](#ra08) · Register allocation |
| A load is emitted first in its block | [RA11](#ra11) · Register allocation |
| The original spills at definition but reloads only once | [RA12](#ra12) · Register allocation |
| An unrelated struct store forces a pointer reload | [RA13](#ra13) · Register allocation |
| An aggregate is scalarized yet moves the frame | [FR01](#fr01) · Frame layout |
| A spilled value uses a dead parameter slot | [FR02](#fr02) · Frame layout |
| An unused eight-byte object or phantom spill remains | [FR07](#fr07) · Frame layout |
| A local initializer is emitted among the pushes | [FR06](#fr06) · Frame layout |
| Adding inline asm reverses the frame | [FR10](#fr10) · Frame layout |
| The load width is right but the store width is wrong | [TY01](#ty01) · Types and widths |
| A char cast and an explicit mask compile differently | [TY03](#ty03) · Types and widths |
| A caller and callee need different parameter types | [TY04](#ty04) · Types and widths |
| An aligned dword read is followed by a byte mask | [TY07](#ty07) · Types and widths |
| A boolean reader loses its movsx | [TY09](#ty09) · Types and widths |
| The add destination remains wrong after operand swaps | [SA01](#sa01) · Sums and algebra |
| Reordering three or four addends changes nothing | [SA02](#sa02) · Sums and algebra |
| A struct-return accumulator helps outside a loop only | [SA04](#sa04) · Sums and algebra |
| The sum uses lea where the original adds in place | [SA10](#sa10) · Sums and algebra |
| The fld order seems reversed | [FP01](#fp01) · Floats and x87 |
| The original uses fisub instead of integer subtraction | [FP03](#fp03) · Floats and x87 |
| A floating constant has a stack home | [FP06](#fp06) · Floats and x87 |
| x87 operands spill despite correct expressions | [FP09](#fp09) · Floats and x87 |
| A packed-key test has an unexplained lea | [RC01](#rc01) · Records and lists |
| A list-head store is one byte too long | [RC03](#rc03) · Records and lists |
| The original has two zero registers near a fill | [RC08](#rc08) · Records and lists |
| Nested calls split or merge add esp unexpectedly | [CC01](#cc01) · Calls and cleanup |
| A shared call tail needs split cleanup | [CC02](#cc02) · Calls and cleanup |
| Argument pushes disagree across inline expansions | [CC05](#cc05) · Calls and cleanup |
| Two same-sized globals are swapped in a zero-mismatch body | [RD01](#rd01) · Reading the original |
| An EBP frame might mean handwritten assembly | [RD03](#rd03) · Reading the original |
| An asm macro unexpectedly writes a control register | [RD04](#rd04) · Reading the original |
| A higher match score selects code the original did not have | [TM04](#tm04) · Triage and method |
| A body has a large strict mismatch but no register-blind mismatch | [TM02](#tm02) · Triage and method |
| Raw esp offsets suggest a different frame | [TM03](#tm03) · Triage and method |
| Our body is one byte or one instruction short | [TM07](#tm07) · Triage and method |
| A supposedly impossible instruction already exists elsewhere | [TM06](#tm06) · Triage and method |
| A once-helpful volatile now makes the body worse | [TM09](#tm09) · Triage and method |
| The original's frame reference profile seems unreachable | [NG12](#ng12) · Measured negatives |
| MusicThread lacks a second pair of imported-call reloads | [NG27](#ng27) · Measured negatives |
| The renderer will not merge a five-instruction suffix | [NG30](#ng30) · Measured negatives |
| A function is exhausted despite a tiny strict residual | [TM12](#tm12) · Triage and method |

## Section map

| Question | Entries |
| --- | ---: |
| [Block layout](#block-layout) | 20 |
| [Loops](#loops) | 18 |
| [Register allocation](#register-allocation) | 21 |
| [Frame layout](#frame-layout) | 10 |
| [Types and widths](#types-widths) | 14 |
| [Sums and algebra](#sums-algebra) | 11 |
| [Floats and x87](#floats-x87) | 11 |
| [Records and lists](#records-lists) | 11 |
| [Calls and cleanup](#calls-cleanup) | 11 |
| [Reading the original](#reading-original) | 8 |
| [Triage and method](#triage-method) | 12 |
| [Measured negatives](#negatives) | 47 |

[Correction register](#correction-register) · [Source coverage](#source-coverage) · [Historical triage snapshot](#historical-triage-snapshot)

<a id="block-layout"></a>

## Block layout

| Rule | Question |
| --- | --- |
| [BL01](#bl01) | Exile follows the fall-through trace and unconditional jumps |
| [BL02](#bl02) | Choose the surviving shared tail by fall-through, phase and arm shape |
| [BL03](#bl03) | Shared tails merge only when their emitted suffixes agree |
| [BL04](#bl04) | Use volatile stores or paired reads only for the measured merge prerequisite |
| [BL05](#bl05) | Tail duplication and cross-jump thresholds are different measured rules |
| [BL06](#bl06) | A suffix merge cannot stop before a different successor |
| [BL07](#bl07) | Guard nesting and splitting select inline versus exiled return blocks |
| [BL08](#bl08) | A return merge target changes epilogue count and branch byte length |
| [BL09](#bl09) | Goto targets follow their incoming layout, not label text |
| [BL10](#bl10) | Switch lowering determines whether case order is a lever |
| [BL11](#bl11) | Identical jump-table entries may have separate source bodies |
| [BL12](#bl12) | An outer busy guard or empty trailing else can flip the merge host |
| [BL13](#bl13) | Push sinking depends on the guarded return structure |
| [BL14](#bl14) | Late-folded duplicate returns can prevent unwanted return cloning |
| [BL15](#bl15) | Let a single return duplicate the original uninitialised epilogue |
| [BL16](#bl16) | Late merging can leave real degenerate tests |
| [BL17](#bl17) | Separate opposite tests can thread to one call site |
| [BL18](#bl18) | An empty per-band guard permits count widening and threading |
| [BL19](#bl19) | Counter-step polarity and repeated guards can preserve a join |
| [BL20](#bl20) | A duplicated loop-exit epilogue can arise from an ordinary shared store |

<a id="bl01"></a>

### BL01 — Exile follows the fall-through trace and unconditional jumps

VC6 lays out a fall-through trace with a LIFO pending-target stack: follow the fall-through chain, push conditional targets, and pop when the trace ends. An alternative block ending in an unconditional jump can be exiled past that trace; an arm ending only in `je` is not exiled by this rule. Determine which edge falls through, rather than relying on source position, size or perceived likelihood.

Evidence: the trace model predicts both layouts of a 1161-instruction body. A scan of all 1544 exact functions found eleven signatures, four genuine source-level witnesses: `UpdateMapDrag` 0x452030, `InitSavedGameScreen` 0x48d4b0, `KillAllSamplesFromSource` 0x496b80 and `LoadObjectLibrary` 0x480f00. `ScanBlokeSurroundings` (0x450530) sends five guards to the two-instruction `inc dword [esp+0x14] / jmp <latch>` at index 355 past the epilogue. `KillAllSamplesFromSource` tail-duplicates a 4-instruction tail; both arms end in `jmp`, but the first stays inline (28..32) and the second is exiled (53..57). On `RenderFullMap`, duplicating a tail into a `continue` arm exiles the fill and restores the join's index; duplicating only null tests leaves a `je` ending and does not move it. Textual gotos often normalize into if/else within a region; a loop-crossing goto can survive. See the body-specific `RenderFullMap`/`UpdateRiverAnim` negatives before extrapolating reachability.

Evidence: [D198](DECOMP.md?plain=1#L2271), [D461](DECOMP.md?plain=1#L4264), [D481](DECOMP.md?plain=1#L4415).

<a id="bl02"></a>

### BL02 — Choose the surviving shared tail by fall-through, phase and arm shape

For identical arms in the reconciled measurements, the copy reached by fall-through survives and the jump-arm copy is merged away. Earlier claims of universally FIRST, LAST or THEN textual survivors conflated layout and merge phases; the later `UpdateHelpTick` and exile evidence resolves that advice to the fall-through edge. In a plain if/else chain the last arm commonly falls through; other shapes reverse the winner. Preserve identical instruction suffixes and inspect final edge targets.

`UpdateHelpTick` has `flags |= 0x80` in two branches: `if (flags & 7) { |= } else { unlink }` keeps the OR inline and exiles unlink; inverting to `if ((flags & 7) == 0) { unlink } else { |= }` makes OR the jump arm and merges it into the later block, turning a 126-instruction misalignment exact. Three `axis = 5; cost = -1;` blocks supplied the earlier opposite-looking witness: making the second copy fall through emits a second copy; making it jump merges it into the earlier inline copy.

Separate recorded phase evidence remains relevant: D263 distinguishes IR-identical suffixes merged into the last copy in layout from machine-code cross-jumps into an earlier block; D296's four probes, including a 7-case jump table, observed a last-source survivor while permuting cases also changed their layout. D279 refines that to layout-last, with matched `Restaurant1_Tick` (0x0042f1a0), and D295 observes that only copies sharing the layout-last copy's eax/ecx/edx phase cross-jump into it. D279 also observed that a copy's scratch phase followed its position in FINAL layout, so at that site rotation was an effect of the merge, rather than its cause. These measurements do not license a universal textual-order rule. The varying argument caps the shared suffix: a varying LAST parameter is pushed first so everything later may share; a varying SECOND parameter caps it at the address push, call, cleanup and jump. In `LegoMedia_TickCustomers`, guarded early breaks instead of an else chain change the canonical holder when jump threading re-adds a latch edge: 88 -> 0 after ~800 inert waypoint variants, also fixing a register pair 30 instructions earlier.

Evidence: [D138](DECOMP.md?plain=1#L1675), [D263](DECOMP.md?plain=1#L2766), [D279](DECOMP.md?plain=1#L2895), [D295](DECOMP.md?plain=1#L3006), [D296](DECOMP.md?plain=1#L3013), [D481](DECOMP.md?plain=1#L4415).

<a id="bl03"></a>

### BL03 — Shared tails merge only when their emitted suffixes agree

Write near-identical arms out in full when the original has separate setup and a merged suffix; sharing them in C can create a different IR join, allocation or cleanup. `LFTrack_DrawAlt`'s 2x2 nest has B/C arms merge while A/D stay separate because A passes `mode` and D passes 0. `Saloon_TickCustomers` 207 -> 0 and `LegoShop2_TickCustomers` 182 -> 0 require the inlined move INSIDE every random-waypoint arm but ONE post-move statement after the chain: five-push blocks retain different scratch rotations, while the call and post-call tail merge. Hoisting the move gives one wrong push block; duplicating the post-move statement plus `break` prevents sharing (+14 instructions).

`BoatingSchool_Tick` 289 -> 53: two arms merge pushes too only when their temporaries use the same registers; otherwise sharing starts at the `call`. A free volatile read advances the phase by one only where the original has a register copy; at a direct push from a just-stored value it instead creates a reload. `TempleSlide_Update` adds the forwarding-side limit: a textual call copy after two stores forwards exactly the LAST-stored field and can therefore share only from the call unless a block boundary restores the reload; only `world.x, world.y, target.x, target.y` gives its original `st st st st ld / ld push / ld` order. `SoftBlitRLE`'s named `f->n16` at its SECOND, post-walk call (versus `RenderSpriteX`'s third/else site) is worth 14; retaining the CSE aligns pointer registers across arms and merges FOUR extra instructions, 137 against 141. A short two-arm tail can thus be excess merging, not missing C.

`RES_EnsureMounted` needs both near-identical CD-nag arms, cancel handlers and restore tails written in full: sharing either loses each inline guard and gives 90 of 92; duplicated source is 92/92. `NewGardenerOrder` needs its tail assignment inside both arms: a shared store gives 72 B against 85. `LFTrack_Interact` needs nested `if (a) { if (b || c) Alt(); else Normal(); } else Normal();`: merged Normal calls put the mode temp in edx for Alt/ecx for Normal; one combined `a && (b || c)` reverses them, and a switch changes comparisons to `sub ecx,K`.

Evidence: [D006](DECOMP.md?plain=1#L504), [D097](DECOMP.md?plain=1#L1334), [D153](DECOMP.md?plain=1#L1897), [D157](DECOMP.md?plain=1#L1930), [D226](DECOMP.md?plain=1#L2498), [D263](DECOMP.md?plain=1#L2766), [D278](DECOMP.md?plain=1#L2885), [D300](DECOMP.md?plain=1#L3038).

<a id="bl04"></a>

### BL04 — Use volatile stores or paired reads only for the measured merge prerequisite

`UpdateSampleSource`'s shared tail reloads `p.x/p.y` from frame homes (`mov edx,[esp+8] / mov eax,[esp+4] / push / push / push / call`). Plain stores leave values in registers and duplicate its ten-instruction tail, 85 instructions for 66; `*(int volatile*)&p.x = e;` in every arm makes the four arms end identically and produces 66/66 with the case-1→case-3 cross-jump and `ja` default exact. Passing `Pos` by value, an inline reader taking `Pos*`, or a goto/default join all remain 85.

`UnlinkGardenerOrder` has a different prerequisite: one copy forwards a just-stored global (`push edi`), the other reloads (`mov ecx,[head]`), so they are not identical. A volatile STORE and an array/union view at the SAME offset do not stop forwarding. A PAIR of volatile reads into locals, in original load order (`t = *(T* volatile*)&tail; h = *(T* volatile*)&head; f(h,t)`), produces `mov eax,[tail] / mov ecx,[head] / push eax / push ecx` and allows a merge. With only one read volatile, both cannot hoist over the first push; an inline volatile read is first in its block. This establishes suffix identity, not the correct survivor: the measured merged tail still hosted opposite to the original. See the separate negative for that body.

Evidence: [D097](DECOMP.md?plain=1#L1334).

<a id="bl05"></a>

### BL05 — Tail duplication and cross-jump thresholds are different measured rules

A shared tail of ONE call plus `add esp,4` is copied into an early-return arm (the eight record unlinks); TWO calls plus a 0x10-byte argument block are jumped to (`PlaneRide_RemoveRecord`). Compare tail size before changing a twin's source. Separately, measured VC6 SP3 cross-jumping merges into a canonical fall-through predecessor at a shared depth of 4 instructions, but between two non-canonical predecessors requires 6. This is instruction COUNT, not bytes: a 2-byte store counts like a 10-byte store. The merge precedes scheduling: an argument-address `lea` enters the shared block only while still below a push, though scheduling later hoists it above the push in unmerged blocks. `RenderCursor`'s measured suffix is 5, one below the non-canonical threshold, accounting for its remaining 250 mismatches as a 4-instruction shift; see its stronger body-specific negative.

Evidence: [D038](DECOMP.md?plain=1#L977), [D313](DECOMP.md?plain=1#L3127).

<a id="bl06"></a>

### BL06 — A suffix merge cannot stop before a different successor

A merged suffix must also share its fall-through successor; a proposed partial cross-jump cannot stop midway before a nonshared successor. Treat this as a CFG constraint, not a missing spelling. The corpus gives no separately named body for this argument; its scope is the proposed partial-suffix mechanism, not all tail merges.

Evidence: [D199](DECOMP.md?plain=1#L2283).

<a id="bl07"></a>

### BL07 — Guard nesting and splitting select inline versus exiled return blocks

`if (a == 0 || b != c) return X;` can exile X. To inline it, split the tests and jump into the second compound block: `if (a == 0) goto lbl; if (b != c) { lbl: return X; }`. This closed `SchoolCarNextManoeuvre` 157 -> 0 after its return-target change had moved 190 -> 157; two separate textual returns cost 173, and `if (a && b) goto ok;` was inert. In the reverse direction, `LoadCSPSprite`'s three separate `if (!x) goto fail;` guards pull fail inline behind the third (13 wrong at the right count); one `if (!a || !b || !c) goto fail;` exiles it and is exact.

For `PlayMovie`, nest the rest of the body: `if (mv) { ...; return played; } return 1;` gives `[main] [return 1] [return 0]` and `test esi,esi / je <far>`; `if (!mv) return 1;` keeps it inline. Nesting one level up under `if (!g_game->no_movies)` also merges both return-0 sites last. Earlier advice that a final `goto fail` label universally pins a return at the END was too broad: two `goto suppressed;` sources left the block inline at the SECOND goto; the recorded single-predecessor case does not generalize to multiple predecessors.

`BsBoat_StepLeg` also needs the intended inline arm expressed by nesting: `if (dy == 0) A; else if (dx >= 0) B; else C;` inlines A, while `if (dy != 0) { if (dx < 0) C; else B; } else A;` inlines C and orders the cold blocks correctly. Negating a compound condition can preserve its short-circuit test order while swapping arm layout (`if (!C) A; else B;`); measure the actual branch shape.

Evidence: [D013](DECOMP.md?plain=1#L607), [D061](DECOMP.md?plain=1#L1117), [D156](DECOMP.md?plain=1#L1922), [D163](DECOMP.md?plain=1#L1977), [D175](DECOMP.md?plain=1#L2075), [D176](DECOMP.md?plain=1#L2080).

<a id="bl08"></a>

### BL08 — A return merge target changes epilogue count and branch byte length

An explicit trailing `return 0` is not free: it may collect earlier return sites. `ReadNarrationWaveHeader` has nine `_read(...) != 4` guards; `while (...) {...} return 0;` collected them (151 of 187, 489 B against 522). `for (;;) { if (...) return 0; ... }` with no trailing return retains the original ELEVEN inline `xor eax,eax / pop esi / add esp,8 / ret` copies, 187/187. The loop itself rotates to the same while shape (peeled read 0x0049854d, header 0x00498567, latch 0x004985d5); merge-target existence is the difference.

Two separate chunk failures merge backward into a return block 323 bytes earlier and need a six-byte `jne`: 187/187 but 526 B. Both routed through `goto chunkfail;` at the loop end select the two-byte forward `jne` to the copy 121 bytes away and restore 522 B (four bytes), invisible to strict/rb/ob instruction counts. Repeated `if (!Write(...)) return 0;` guards likewise cross-jump backward into the first inline return with no special construct. `SaveScripts` 130 -> 0 needs BOTH the first guard as the only `goto fail` source (label block follows that guard in layout) and `fail: return 0;` as the LAST statement (surviving zero copy); loop failures remain plain returns. These placement facts are per CFG, not a universal first- or last-return rule.

Evidence: [D013](DECOMP.md?plain=1#L607), [D026](DECOMP.md?plain=1#L802), [D175](DECOMP.md?plain=1#L2075), [D355](DECOMP.md?plain=1#L3422).

<a id="bl09"></a>

### BL09 — Goto targets follow their incoming layout, not label text

A goto target can be generated after its LAST goto source in layout (label text in case 0, gotos from cases 2 and 5 -> after case 5), even if a fall-through predecessor must acquire a jump. A single-predecessor target is generated inline with its source. Thus moving a label to the textual end does not generally move its code, and a shared tail entered backward into its MIDDLE cannot be obtained merely by a goto; the `TempleSlide_Update` measurement needed textual tail copies. In the measured epilogue-ordering case, making the desired first epilogue the labelled target places it before the block the function otherwise falls through into; jumping into a compound statement is valid.

A source goto creates a shared block before frame allocation, unlike later compiler cross-jumping: `SoftBlitAnim` 8 -> 0 with identical emitted instructions but rotated spill homes. In `StepSchoolCar`, `if (cond) goto joint; return;` instead of `if (!cond) return;` buys the inline epilogue and puts the join after the else arm, restoring the six missing instructions.

Evidence: [D097](DECOMP.md?plain=1#L1334), [D215](DECOMP.md?plain=1#L2420), [D261](DECOMP.md?plain=1#L2755), [D263](DECOMP.md?plain=1#L2766), [D296](DECOMP.md?plain=1#L3013), [D356](DECOMP.md?plain=1#L3428).

<a id="bl10"></a>

### BL10 — Switch lowering determines whether case order is a lever

For a JUMP-TABLE switch, write cases in the original block order. `SchoolCarNextManoeuvreHorn` needs 1,4,5,3,6,7; natural 1,3,4,5,6,7 costs 17. For a COMPARE CHAIN (`sub eax,0 / dec / dec` in the recorded small switch), case VALUES determine layout: all six orders of `BuildPTPRoute`'s three cases, an if/else-if chain and a hoisted call result were byte-identical, including blocks. Earlier blanket case-source-order advice applies only to jump tables.

One case `switch(kind) { case 1: ... }` gives `mov eax,<arg> / dec eax / je`; `if (kind == 1)` gives `cmp dword ptr [mem],1 / je`, while manually decrementing adds `test`. Two cases give `dec/je/dec/jne`, case 2 inline, versus if/else-if's `cmp eax,1 / jne`, opposite layout and a different register for three `=1` stores. `DrawPathTileOverlay`'s measured source cases 1,2,3 produce blocks 3,2,1: case 1, laid LAST, shares the default epilogue; 3 and 2 receive duplicated epilogues. This reverse order in that arrangement is consistent with value-driven chains; it does not overturn the six-permutation counterexample.

Shared cases 1/2 and separate 3 form RANGE clusters [1..2] and [3..3]: `test/jle`, `cmp 2/jle`, `cmp 3/jne`, last cluster inline; `if (k==1 || k==2)` gives a different `cmp eax,1 / je`. A jump-table bound may reuse the outer register still holding 3 (`cmp ecx,eax / ja`), explaining its unsigned check. RLE dispatch on `x & 0xc0` needs switch for index+jump-table (or `sub/je/sub/jne`); an if/else-if chain gives compares and a different block order, worth several points in every RLE section.

Evidence: [D013](DECOMP.md?plain=1#L607), [D026](DECOMP.md?plain=1#L802), [D054](DECOMP.md?plain=1#L1073), [D094](DECOMP.md?plain=1#L1323), [D179](DECOMP.md?plain=1#L2103), [D291](DECOMP.md?plain=1#L2984), [D487](DECOMP.md?plain=1#L4466).

<a id="bl11"></a>

### BL11 — Identical jump-table entries may have separate source bodies

**A switch label sharing a jump-table entry may still be a SEPARATE case body
  in the source.** VC6 merges two identical whole case blocks and points both
  table entries at the survivor — which is indistinguishable in the disassembly
  from `case A: case B:`, but is NOT the same in codegen, because the extra
  block changes the merge candidate set and therefore the layout. On
  `Joust_Update`, giving two labels their own bodies is worth **109 and two
  missing instructions**, and aligned a loop's start index exactly. A third
  label in the same switch IS genuinely shared (its own body costs 23), so
  **this must be tested per label, not decided for the switch.**

Evidence: [D160](DECOMP.md?plain=1#L1954).

<a id="bl12"></a>

### BL12 — An outer busy guard or empty trailing else can flip the merge host

Test guard shape only after proving the shared tail hosts in the wrong copy. `Joust_Update`: `if (b->state == 0) { switch ... }` hosts call tails in LAST group cases; `if (b->state != 0) goto endsw;` hosts FIRST, worth 153. `TempleSlide_Update` has the opposite per-body answer in the later comparison; its direct outer-goto probe records 258 -> 77 strict and 13 instructions moved. Gotos in case 2, case 5, or both are byte-identical; ALL copies or only the first are inert, so asymmetry matters. This does not imply goto and break differ inside an ordinary switch; see `RenderCursor`.

`Restaurant2_Draw` 171 -> 0 requires literally an empty trailing `else { }`: the previous last arm ceases to fall through, so its free survivor advantage disappears and the merge goes backward. A nonempty else, trailing else-if or separate if adds real code; `else { n=n; }` instead becomes goto-shaped. Negative measurements on `JungleCruise_Tick` and five activations bound transfer.

Evidence: [D161](DECOMP.md?plain=1#L1963), [D304](DECOMP.md?plain=1#L3064), [D403](DECOMP.md?plain=1#L3779).

<a id="bl13"></a>

### BL13 — Push sinking depends on the guarded return structure

A guarded block needs its OWN single `return K` for the measured push-sinking shape: `if (ev & 2) { ...; return 1; } return 1;` emits `test byte ptr [esp+8],2 / je end / push edi` and a bare `mov al,1 / ret`. Sharing one return outside the guard puts the push at entry, merges an epilogue and loses ONE instruction; it also splits the memory test into `mov al,[esp+8] / test al,2`. The inverted early return and goto variants were 94/100. `BuildObjectIconInput` narrows this to ONE return inside the block: two inner `return 2` sites put `push esi` back in the prologue; one return after the inner if/else sinks both pushes, 55/55, refining the wave-fourteen measurement.

`SchoolCarAccelerate` uses one shared `c->a=va; c->b=vb;` tail after if/else: VC6 duplicates it, exiles else after the normal epilogue, and sinks pushes whose values start after the guard. Early `if (len==0) {...; return;}` with direct stores pins all pushes at entry. `GetObjectFromName` (0x44dda0) was rejected at 84% until post-guard inner-scope loop locals plus `break` to one trailing return let `push ebp/esi/edi` sink into the non-null path, leaving only ebx above the null test. These are demonstrated source shapes, not a claim that scope alone always sinks pushes.

Evidence: [D084](DECOMP.md?plain=1#L1276), [D122](DECOMP.md?plain=1#L1566), [D222](DECOMP.md?plain=1#L2470), [D492](DECOMP.md?plain=1#L4487).

<a id="bl14"></a>

### BL14 — Late-folded duplicate returns can prevent unwanted return cloning

**Return-block cloning for a deferred push, and the constant hoist it drags
  in (`SaveEmptySlotInput`).** When `push esi` is sunk past leading guards and
  the body ends in a `return K` shared with the guard-fail paths, VC6 clones
  the return into the pushed region and then hoists `mov eax,1`, reordering
  the adjacent immediate stores. To get ONE shared `mov al,1 / ret` with
  immediate stores in source order, put a branch between the body and the
  final return whose arms both reach it and whose test is already
  register-resident on every path: `if (g) return 1; return 1;` on the guard
  global folds late with no residue. A test on a parameter leaves a dead root
  copy. A `push 1` argument before the stores kills the hoist on its own
  (`push 2` does not) because a call clobbers eax, which is why matched twins
  that call `PlayInstanceOfSample(.., 0, 1, 0)` never showed it.

Evidence: [D233](DECOMP.md?plain=1#L2539).

<a id="bl15"></a>

### BL15 — Let a single return duplicate the original uninitialised epilogue

`LoadTextFile` needs one `return text;` after the if. VC6 then keeps all three pushes at entry and duplicates its epilogue, reading the never-written result home `mov eax,[esp+0xc]`: 47/47. An explicit failure-arm return sinks two pushes and reads `[esp+4]`, 31/45. This reproduces an original undefined-value path; do not add an initialization merely to improve apparent semantics.

Evidence: [D026](DECOMP.md?plain=1#L802).

<a id="bl16"></a>

### BL16 — Late merging can leave real degenerate tests

`DefaultIconInput` needs an explicit event-bit case even though it returns the default value: equal returns merge late, preventing earlier `setne/inc`; the final code is `test al,4 / mov al,2 / jne / mov al,1 / ret`. The eliminated test's mask is NOT uniquely recoverable. `Coaster3D_BuildPieceGeometry`'s empty `cmp/jne +0` comes from `if (n==5) n=5;`; `n=n`, `{}`, `;`, a dead local store and identical-call if/else are deleted wholly. Other degenerate call branches DO survive at `g_tile_info & 0x20` (0x004622ae / 0x00462333): write both `if(c) f(a); else f(a);` arms to preserve differing registers.

`RestoreBaseMap` (0x0045da60) leaves dead `and dx,0x20` from a u16 `reserved=code&0x20` compared with `reserved==0x20` inside three identical store arms; range folding, late merging and jump peeling leave the ghost. `!=0` is not equivalent codegen; a value operation needs more than one 16-bit consumer, while a register bit test is `test dl,0x20`. `LoadBaseMap` has the related no-jump test at 0x00462333. `CheckWorkerOnMouseStatus` 0x4707a3 leaves dead `mov ebp,1` because the web was live at allocation before later threading/duplication; its strict 82 is one missing instruction shifting later indices, not 82 independent errors.

Evidence: [D007](DECOMP.md?plain=1#L519), [D100](DECOMP.md?plain=1#L1448), [D218](DECOMP.md?plain=1#L2439), [D384](DECOMP.md?plain=1#L3616), [D493](DECOMP.md?plain=1#L4492).

<a id="bl17"></a>

### BL17 — Separate opposite tests can thread to one call site

**`if (s == 0) A(); if (s != 0) B();` as two separate `if`s gives ONE call
  site for B** with the first test's non-zero edge threaded into it — the
  shape of both `Control*` worker ticks. An `if/else` emits two.

Evidence: [D036](DECOMP.md?plain=1#L964).

<a id="bl18"></a>

### BL18 — An empty per-band guard permits count widening and threading

**Empty per-band guard blocks trigger jump threading and a widened count
  (`Carousel_Draw`).** With `if (n > 0)` guards on the same non-escaped char
  `n`, an empty guard block lets VC6 thread failed guards, prove later guards
  redundant and CSE the four `movsx` into one int; a `lea` in the guard block
  (an inlined parameter copy the helper walks) suppresses all three.

Evidence: [D241](DECOMP.md?plain=1#L2603).

<a id="bl19"></a>

### BL19 — Counter-step polarity and repeated guards can preserve a join

In the measured switch body, put the counter step in the if arm and state change in else: the step falls through as register load/step/store; natural `if(s>8) {...} else s+1;` inverts and folds to memory `inc`. A repeated-test shape `if (a && b) goto x; if (!a) goto y; goto z;` creates a two-entry join before a call where the straightforward source emits the call first and jumps backward. These are source-shape probes for the measured switch, not case-order rules for compare chains.

Evidence: [D291](DECOMP.md?plain=1#L2984).

<a id="bl20"></a>

### BL20 — A duplicated loop-exit epilogue can arise from an ordinary shared store

The same `*link=0;` can emit `mov word [esi],0` on one exit and `mov word [esi],bp` on another without an explicit source construct: the measured loop guard must reload its induction variable from memory and VC6 duplicates the exit epilogue. D159 does not name the function; keep this observed mechanism distinct from claims that all duplicate exits were written twice.

Evidence: [D159](DECOMP.md?plain=1#L1949).

<a id="loops"></a>

## Loops

| Rule | Question |
| --- | --- |
| [LP01](#lp01) | Post-decrement while loops preserve dead decrement/increment trip setup |
| [LP02](#lp02) | Up-counting lets the loop transform emit add with a negative constant |
| [LP03](#lp03) | Use the natural ordinal and exact comparison bound |
| [LP04](#lp04) | Read loop form from the latch, peeling and literal bound |
| [LP05](#lp05) | Infinite-loop spellings differ when they change the header block |
| [LP06](#lp06) | Latch order follows update clauses and sometimes store order |
| [LP07](#lp07) | Cursor spelling controls induction-variable elimination, unless a call separates uses |
| [LP08](#lp08) | Cursor anchors depend on references, access form and traversal direction |
| [LP09](#lp09) | Two link-time arrays versus one offset-indexed array change IV and zero ranking |
| [LP10](#lp10) | Initialize count, index and cursor at their measured definition sites |
| [LP11](#lp11) | An address-taken counter disables ordinary strength reduction |
| [LP12](#lp12) | Subscripts preserve signed bounds, store/load order and compiler-owned spill homes |
| [LP13](#lp13) | Do not hand-unroll or hand-hoist without reading the generated shape |
| [LP14](#lp14) | Suppress an unwanted derived IV with the measured object or temporary |
| [LP15](#lp15) | Mirror a written global in a local to place the miss reload |
| [LP16](#lp16) | Duplicate cursor-advance guards when the original has two miss blocks |
| [LP17](#lp17) | Intrinsic strlen exposes its own guard and latch flags |
| [LP18](#lp18) | Tail recursion is a measured equivalence, with a phase-order caveat |

<a id="lp01"></a>

### LP01 — Post-decrement while loops preserve dead decrement/increment trip setup

`while (n-- != 0)` on an unsigned counter lowers to `mov ecx,eax / dec eax / test ecx,ecx / je <end> / inc eax`: VC6 rotates it into a do/while over a separate trip-count slot, leaving dec/inc dead. `if(!n) goto end; t=n; for(;;){...}` cannot produce it, and manually written dead arithmetic is deleted. For signed positive traversal, `while(n-- > 0)` similarly gives `mov ecx,eax / dec eax / test ecx,ecx / jle / inc eax`. Nine spellings measured: up-counted for loses dec/inc; `i <= n-1` gets the pair but tests n-1 with `jl`. Read the guard's signedness and comparison, not just the dead pair.

Evidence: [D099](DECOMP.md?plain=1#L1442), [D486](DECOMP.md?plain=1#L4461).

<a id="lp02"></a>

### LP02 — Up-counting lets the loop transform emit add with a negative constant

`for(i=0; i<N-2; i++)` can reverse to the same `test/jle` guard and `dec/jne` latch as down-counting, while computing the trip count with `add eax,-2` instead of source-subtraction `sub eax,2`. Earlier unreachable-`add reg,-K` advice was wrong: all swept spellings were SUBTRACTIONs in a DOWN-counting loop; the up-counted transformation supplies a different IR node.

`LFDrop_Place`'s exact head is `for(i=0; i<24/cellh-2; i++)` -> `mov eax,0x18 / cdq / idiv [esp+0x10] / add eax,-2 / test eax,eax / jle`. The original inert measurements still stand for `24/cellh - 2u`, `+ 0xfffffffeu`, `+ (int)0xfffffffe`, `+ -2`, unsigned-wrap forms, and ordinary subtraction spellings `x + (-K)`, `x-K`, `x+~(K-1)`, `x+(0-K)`, `n-=K`: they canonicalize to one SUB node. A loose `.text` scan found 75 `add r32,-K` sites, including already-exact 0x0040c01f in `LFEntrance_Activate` from `(tile->b.x-2)<<8`. Preserve the subtraction equivalence; reject the universal unreachable inference.

Evidence: [D181](DECOMP.md?plain=1#L2117), [D182](DECOMP.md?plain=1#L2124), [D351](DECOMP.md?plain=1#L3388), [D387](DECOMP.md?plain=1#L3648).

<a id="lp03"></a>

### LP03 — Use the natural ordinal and exact comparison bound

A natural 1-based loop can be materially different from 0-based plus one. In `MusicThread`, `for(a=1;a<5;a++)` gives memory-homed `a`, a `5*a` IV and rebased `cmp edi,0x10`; `for(a=0;a<4;a++)` with `a+1` recomputes in the body. `GetBlokeAgeGroup`'s preheader `mov ecx,1` plus trailing `lea eax,[ecx-1]` likewise indicates a one-based source; 0-based `for(i=0;i<4;i++) ... return i;` is two instructions shorter and has no lea.

On a strength-reduced table cursor, `i <= N-1` gives `jle` against `&tbl[N-1]`; `i < N` gives `jl` against `&tbl[N]`, the one-instruction residual in three loops, confirmed on `GetBlokeAgeGroup`. A guard comparing `i+1` with count and no decrement indicates counting from `i+1`; `j<n-1` materializes n-1 and rebuilds allocation. Keep `-1` in the condition when the original computes `add` then `lea [eax-1]`: `while(p<end-1)` does that; baking it into end gives one `lea [eax+esi-1]`.

Evidence: [D004](DECOMP.md?plain=1#L481), [D013](DECOMP.md?plain=1#L607), [D016](DECOMP.md?plain=1#L727), [D048](DECOMP.md?plain=1#L1037), [D053](DECOMP.md?plain=1#L1067).

<a id="lp04"></a>

### LP04 — Read loop form from the latch, peeling and literal bound

A while loop's condition is the test left in its latch; a peeled copy can double as an enclosing if. `InsertPrintItem` requires `if(key<cur->key) { while(key<cur->key) { if(!cur->prev) break; cur=cur->prev; } } else if(key>cur->key) {...}`. The peeled compare supplies the outer test and flags reused by else-if (`cmp/jge/.../jle`). Testing the link in the while and the key as break peels the wrong test, adds three instructions per arm, 83 versus 77/77. `for(;;)`, do/while and backward-goto forms tried normalize to the wrong form.

A literal-bounded counted for is the complementary shape: `PrintScreenMode7` uses `for(i=0;i<14;...) { if(line>g_rep_line_count) break; ... }`, giving head `cmp line,count / jg exit` and latch `cmp i,0xe / jl head`; VC6 proves the initial literal test and deletes it, so NO peel exists. A local bound versus a struct-field bound also changes traversal: `FreeAnim3D`'s LOCAL bound gets a down-counter without reload; `LLIDB_UnLoadODFData`'s STRUCT FIELD reloads and counts up.

`Coaster_TickLoadingBay` must break to ONE trailing `if(flag) f();`, keeping `xor ebx,ebx`/`mov ebx,1` and threading constant paths. An inner `if(flag) f(); return;` peels the first iteration, hoists a call copy and deletes the flag. While, for(;;), and `while((c=g())!=0)` work; do/while with the same break peels. A guarded `if(i>0){p=...; do {...}while(--i);}` puts accumulator ebx/argument pointer ebp where an equivalent counted-for swaps them, worth 12 instructions. For a char count, `if(n>0){T* q=arr; int i=n; do {...}while(--i);}` keeps `test al / je / jle` on the char and puts the lea AFTER jle; a helper walking its parameter hoists lea into the guard block.

Evidence: [D026](DECOMP.md?plain=1#L802), [D068](DECOMP.md?plain=1#L1162), [D097](DECOMP.md?plain=1#L1334), [D174](DECOMP.md?plain=1#L2070), [D221](DECOMP.md?plain=1#L2463), [D253](DECOMP.md?plain=1#L2696).

<a id="lp05"></a>

### LP05 — Infinite-loop spellings differ when they change the header block

`UpdateGoalHelpText` 335 -> 0 requires `while(1){g=head; if(!g) break; switch...; Remove(g);}`: the folded constant-true test remains the header, so VC6 keeps `jmp header`. Header blocks ending directly in the exit conditional permit inversion and copy the exit test into the latch. `goto again` to a label ABOVE the while is also exact; for(;;), do/while(1), conditional while, and a bare top/goto loop with a return exit invert incorrectly. A scan found no other non-inverted `mov r,[global]/test/je` loop in the executable. Late tail duplication here needs the uninverted loop; small jmp/ret-ending blocks clone after allocation (same registers in each copy) and before scheduling. Split `add esp,8 / add esp,4` in those tails indicates separate source blocks.

This is body-specific: `ReadNarrationWaveHeader`'s backward goto over if normalized byte-for-byte to its while loop and 151-instruction return merge; `MusicThread`'s while(1)/for(;;) variants do not fix its IAT reload floor. Do not turn either equivalence or difference into an all-loops rule.

Evidence: [D013](DECOMP.md?plain=1#L607), [D260](DECOMP.md?plain=1#L2741).

<a id="lp06"></a>

### LP06 — Latch order follows update clauses and sometimes store order

Sweep source update order before retiring a latch permutation. `RecolourModelParts` requires `for(...;p++,i++)`; reversed `i++,p++` is wrong while initializer/declaration order is inert (61i/156B exact). `PrintScreenMode7` with data steps in the body and only i++ in the for-increment is 50 of 59, latch `add cursor,4 / add y,0x18 / inc i / cmp i,0xe`; `i++,line++,y+=0x18` gives `inc i / inc line / add cursor,4 / add y,0x18 / cmp i,0xe`, 59/59 and the correct callee-saved ranking.

`RES_FindVolumeOnAnyDrive` needs the counter update generated BEFORE `bit <<= 1`: `for(i=0;i<32;i++,bit<<=1)` is 138/138, while reversed increments, do/while, or bit update in the body yield 135. Explicit `{i++; bit<<=1;}` at the body end also reaches 138; the original interleaves `mov ecx,[bit] / inc ebx / shl ecx,1 / cmp ebx,0x20 / mov [bit],ecx / jl`. Other flag/store/guard levers must agree (partial combinations 117,120,135).

`FlushCursorSpriteList` needs `while(i<n){call; i++; put++;}`: `inc esi / add eax,0x10 / cmp esi,ecx / mov [put],eax`, versus for-increment i++ with body put++ -> `add eax,0x10 / inc esi / mov [put],eax / cmp esi,ecx`. Two of 29 at identical bytes, store sinking follows the update reorder. A pointer ++ placed in the for-increment rather than the body's last statement can move after other latch ALU ops.

Store order can choose derived-IV order too: `ZBuffer_RunCommand` needs `key[i].idx=i` LAST among three stores, giving `inc esi / add eax,0x30 / add ecx,8 / add edx,8`; first gives `inc / add ecx / add edx / add eax`. All 24 orders measured, this one closes. Earlier blanket latch-order unreachability was only `BsWater_SetTile`'s twenty-variant floor; keep that negative body-specific.

Evidence: [D013](DECOMP.md?plain=1#L607), [D026](DECOMP.md?plain=1#L802), [D138](DECOMP.md?plain=1#L1675), [D370](DECOMP.md?plain=1#L3523).

<a id="lp07"></a>

### LP07 — Cursor spelling controls induction-variable elimination, unless a call separates uses

Spell lockstep cursors the SAME way to permit elimination; one subscript and one walking pointer retain both. `TransformVerts` 8 -> 0 also fixes a multiply commutation; applied backward to `Coaster3D_DrawMesh`, 17 -> 8. The `sub esi,ecx` plus `[esi+ecx]` shape is elimination of one IV. Earlier eight inert loop-form variants on `Coaster3D_DrawMesh` (hoisted source pointer, joined store, inline helper, if polarities, down-counting, while, unsigned index, flat int*) did NOT test this cursor-spelling route and do not prove global unreachability.

A CALL between two subscripts blocks elimination: `table[i].elem` before the call and `table[i].kind` in the latch retain two cursors plus `mov eax,esi` in `LLIDB_UnLoadTSMData`; p++ collapses to one and is two instructions short (ten spellings). Conversely `JcBoat_Animate` needs VC6's own subscript strength reduction of `b->wob[j]`; an explicit pointer walk plus counter keeps the byte count but moves divergence earlier.

Evidence: [D009](DECOMP.md?plain=1#L577), [D098](DECOMP.md?plain=1#L1435), [D146](DECOMP.md?plain=1#L1845), [D196](DECOMP.md?plain=1#L2251).

<a id="lp08"></a>

### LP08 — Cursor anchors depend on references, access form and traversal direction

For the measured strength-reduced record cursors, most references wins the anchor and ties go to the LAST reference; offset 0 does not win from ordinary ->/subscript access without an extra reference. Earlier midpoint framing was a mistaken generalization: +8/+0xc/+0x10 at +0xc and +8..+0x18 at +0x10 were observations, not a geometric rule. All six read orders of `InitTrackDrawModes`' three-field row give three +8 and three +4 anchors; only duplicated `e->in` reaches +0, changing load order. `ZBuffer_RunCommand`: `edge[i].side` in BOTH arms anchors +4, naming `c->v[i].y` three times anchors +2; together 16 of 53.

For inline-array stores in `LFRun_Start`, pos as SECOND store gives `lea esi,[run+0x4c]`; pos first slides anchor to the third statement's field (twelve orders). SUBSCRIPT form also lets piece store precede call-result store; named-pointer form floors at 3 across all 48 orders.

Direction matters in `JcBoat_Advance`'s three fills of 8-byte {x,y}: ascending per-field x/y assigns anchor to `.y`, `[eax-4]/[eax]`; whole-copy, *p++, or y-then-x anchor `.x`, three wrong operand instructions per up loop (6 of 69 total). Descending `[eax]/[eax+4]` needs whole-struct assignment. Treat these as measured access-form/direction evidence rather than a universal first/last-field rule.

A const-table cursor may need an explicit biased pointer: `EarthSlide_LaunchCar`'s 4-entry table as subscript is 74i/199B versus 75i/201B, lacking cursor materialization and the target.y copy. `const Pos*` walking restores count but leaves 9 of 75, and every recorded Pos* variant remains 2 or worse at the +4 anchor. `const int* spot=&kSlideQueueSpots[0].y;` with `spot[-1]/spot[0]`, `spot+=2` is exact. `&arr[i]` sufficed for both render5.c cursors: biased-pointer advice is for read-only tables walked by several fields.

Evidence: [D013](DECOMP.md?plain=1#L607), [D033](DECOMP.md?plain=1#L946), [D071](DECOMP.md?plain=1#L1183), [D097](DECOMP.md?plain=1#L1334), [D121](DECOMP.md?plain=1#L1561).

<a id="lp09"></a>

### LP09 — Two link-time arrays versus one offset-indexed array change IV and zero ranking

`g_seg_first[a] / g_seg_second[a]` gives two link-time bases with one index (`[edi+K1]/[edi+K2]`) and lets that index merge with the hoisted zero. `g_segments[a] / g_segments[a+5]` instead folds to one walking pointer and loses the zero's rank: `MusicThread` 319 -> 18, a decision worth 300 instructions. When an IV can start at the already-hoisted zero, VC6 can merge their webs and improve the zero's physical rank, permuting the whole callee-saved assignment. Do not confuse this with two separately declared parallel objects that should be one struct; the address expressions and alias relationships differ.

Evidence: [D002](DECOMP.md?plain=1#L465).

<a id="lp10"></a>

### LP10 — Initialize count, index and cursor at their measured definition sites

`FindCoasterColour` needs the table entry COUNT read into a local BEFORE deriving its cursor: count left in the condition hoists after lea, puts i=0 above pushes and table pointer in edx, losing the 5-byte moffs32 load; 6 -> 0. `ModelImage_FindRecord` needs image length named before counter initialization, then `end=p+length`: pointer add follows counter zero and closes two schedule differences, 40i/74B.

An eager root copy `void** dst=out` places the load between push esi/edi and re-ranks two cursors: `CollectUsedTSFTables` 7 -> 0 at identical counts. The same separate-pointer mechanism closes `SkipStrings`; stepping the parameter leaves the zero-trip arm returning `[esp+8]` instead of edx. `for(i=0,n=0;...)` orders counter xor before accumulator xor; standalone prior `n=0` reverses them. A preheader `xor <iv>,<iv>` BELOW the zero-trip guard is VC6's own strength reduction: explicit off local moves it ABOVE the guard (both measured).

Evidence: [D001](DECOMP.md?plain=1#L427), [D010](DECOMP.md?plain=1#L584), [D011](DECOMP.md?plain=1#L589), [D030](DECOMP.md?plain=1#L927), [D062](DECOMP.md?plain=1#L1122).

<a id="lp11"></a>

### LP11 — An address-taken counter disables ordinary strength reduction

`LFAnim_LoadRefs` reuses an int n, passed by &n to a reader, as counts and loop counter. This forces memory, repeated counter reload and recomputed `i*3 / lea [base+i*4]`, replacing EDI counter/EBX i*12 IV and splitting EDI's push into the guarded path: worth 90 of 93. An address-taken counter's increment can be `mov edx,[esp] / inc edx / mov [esp],edx` when its only other use is in the body; the home store is required, but other live values decide whether a reload is needed. Do not infer every address-taken counter must reload at every possible use. `RemoveObjectPathTiles` supplies the opposite-direction fix: `for(p.y=...;...;p.y++)` on the escaped Pos pins the pair in memory and loses x*20 strength reduction, 136 instructions versus 122. Plain int x,y with `p.x=x; p.y=y;` only immediately before the two callees gives 122/122 first try, including `lea edi,[ebp+ebp*4] / shl edi,2` and latch `add edi,0x14`. Its sibling `RemObjFromMap` is exact with the OTHER escaped-Pos loop shape; measure per body.

Evidence: [D026](DECOMP.md?plain=1#L802), [D109](DECOMP.md?plain=1#L1497), [D138](DECOMP.md?plain=1#L1675).

<a id="lp12"></a>

### LP12 — Subscripts preserve signed bounds, store/load order and compiler-owned spill homes

`for(j=0;j<N;j++)` over a GLOBAL array produces signed `cmp cursor,end / jl` against a link-time address; an explicit pointer walk gives unsigned `jb`. In a bubble sort, inner signed `jle` identifies `for(k=0;k<i;k++)`, while `je` identifies `for(k=i;k!=0;k--)`; subscript swaps keep `out[k]=out[k+1]` before `out[k+1]=t`, while a pointer walk reverses them.

A subscripted array walk also gives VC6 an IV temporary that cannot coalesce with list PARAMETER, allowing it into a dead argument home and freeing list's home for the trip counter. A named `void** p=list` coalesces with the parameter and keeps the wrong slot. `SaveLogFlume` uses up-counted subscripts with real record stride: VC6 reverses to `mov edi,4 / dec edi`, yet `b[i].f=call(); b[i].g=call(b[i].g);` keeps f store before g load; pointer walking hoists the load.

`&arr[n++]` as a call argument advances the cursor before the call and forces `mov reg,cursor / push reg`; right at one measured site, worse by 4 at the next. The `Draw3DPersonModel` reconstruction case study in Triage and method records the separate transpose and body-first vertex-index correction with its complete measurements.

Evidence: [D026](DECOMP.md?plain=1#L802), [D105](DECOMP.md?plain=1#L1471), [D117](DECOMP.md?plain=1#L1536), [D142](DECOMP.md?plain=1#L1822), [D225](DECOMP.md?plain=1#L2493).

<a id="lp13"></a>

### LP13 — Do not hand-unroll or hand-hoist without reading the generated shape

VC6 SP3 does NOT unroll `SchoolCarIdleStep`'s 16-element, 8-byte-struct shift: a for is 19 instructions, intrinsic 128-byte memcpy is rep movsd, and sixteen SEPARATE assignments alone give the original 71 `mov edx,[base+src] / mov [base+dst],edx` sequence. The sixteenth copy reads one past the array, an original bug to preserve.

A rep-stosd fill is covered under Reading the original. Do not name invariants VC6 already hoists: `WW_AnyBlokeInRect` gets worse by 16 for one named rect field, 24 for two, because names add allocation webs.

Evidence: [D110](DECOMP.md?plain=1#L1505), [D192](DECOMP.md?plain=1#L2221).

<a id="lp14"></a>

### LP14 — Suppress an unwanted derived IV with the measured object or temporary

`PaintTileLayer` initially used `px + *(volatile int*)&halfw` at a load the original repeats: the derived second-column IV disappears, frame 0x38 -> original 0x34, but halfw's home moves and shifts two neighbours. Later nonvolatile evidence groups halfw with the already-ESCAPED tile Pos, preserving an ordinary home and refusing the IV at 0x34: 389 -> 378 with recovered register roles. Grouping all seven intervening homes is worse, 395 and ESCAPES. The earlier request for a nonvolatile refusal is resolved by that narrowly scoped grouping.

A repeated step expression can itself make an IV: in `JungleCruise_TraceRoute`, writing x-5 twice makes x an IV, creates x+5 in a stack slot and an extra latch sub; a named `int nx=x-5` used for probe and recursive call removes it (9 instructions of noise). `RenderFullMap`'s explicit `ILFTable` carrying the sprite+8 ADDRESS to the loop condition restores `add eax,8 / mov eax,[eax]` in the latch, 844 -> 827; forced alternatives to the other six register facts cost 873-1066. See its negative for the unresolved combined body.

Evidence: [D007](DECOMP.md?plain=1#L519), [D023](DECOMP.md?plain=1#L785), [D138](DECOMP.md?plain=1#L1675).

<a id="lp15"></a>

### LP15 — Mirror a written global in a local to place the miss reload

For a loop whose body writes its bound global, `i<g_count` puts the reread in the shared latch. Use `i<n` and `else {n=g_count;}` for a one-load MISS arm ending in jump, exiled after the epilogue. `if(!hit){n=g_count;continue;}` inverts and inlines instead; the arm must be an else in the measured shape.

Evidence: [D138](DECOMP.md?plain=1#L1675).

<a id="lp16"></a>

### LP16 — Duplicate cursor-advance guards when the original has two miss blocks

`RandomFavouriteFood` and `RandomFavouriteRide` reject candidates on two independent tests before wrap-around advance. One && guard merges the miss block, seven instructions short (19 of 70); two guards each ending in continue retain two inline copies and extra references that rank n/start edi/ebx instead of ebx/ebp. An else-if nest is byte-identical to the continue form. This is the splitting counterpart of merging guards to exile a common failure.

Evidence: [D097](DECOMP.md?plain=1#L1334).

<a id="lp17"></a>

### LP17 — Intrinsic strlen exposes its own guard and latch flags

`while(strlen(p))` under `#pragma intrinsic(strlen)` uses the inline `not ecx / dec ecx` flags for bare je/jne; no separate compare. `do{}while(strlen(s)==0)` similarly gives repne scasb and back-edge into `not ecx / dec ecx / je top`. A preheader `mov eax,1` reused in `test byte ptr [reg-4],al` is the return-1 value materialized early and fused as the flag mask, not a separate explicit mask variable.

Evidence: [D026](DECOMP.md?plain=1#L802), [D057](DECOMP.md?plain=1#L1094).

<a id="lp18"></a>

### LP18 — Tail recursion is a measured equivalence, with a phase-order caveat

`WW_AnyBlokeInRect`'s tail-recursive spelling is byte-identical to its loop: VC6 SP3 eliminates the tail call, so recursion is not a new search dimension on that body. The `JungleCruise_TraceRoute` probe is different: without a loop, four int parameters outrank three dereferenced POINTERS, which spill despite more references; adding a self-tail-call creates an IR loop, reverses ranking, spills ints and creates a secondary IV. A dead trailing statement (`w=w`, `p=p`, `x=x`, `if(route);`) blocks tail-call detection long enough to change regime, though it folds for layout; lane evidence says the real call/epilogue then remains. The original TraceRoute combines loop-free allocation with a loop, a recorded phase-order difference (original conversion after allocation, this compiler before). Treat that diagnosis as body-specific; see its negative.

Evidence: [D138](DECOMP.md?plain=1#L1675), [D189](DECOMP.md?plain=1#L2199).

<a id="register-allocation"></a>

## Register allocation

| Rule | Question |
| --- | --- |
| [RA01](#ra01) | A real named temporary advances scratch rotation |
| [RA02](#ra02) | Named locals and direct global expressions have different webs |
| [RA03](#ra03) | Read values before a call or branch to create the missing live web |
| [RA04](#ra04) | Copy an out-value immediately after its producing call |
| [RA05](#ra05) | Root assignment and initialiser order determine early zeros and spill positions |
| [RA06](#ra06) | Independent statement order creates and frees register webs |
| [RA07](#ra07) | A zero variable and a constant-zero web are different objects |
| [RA08](#ra08) | Constant-zero hoisting is loop-weighted and sensitive to copy placement |
| [RA09](#ra09) | Which register is freed determines the zero carrier and the whole saved-register permutation |
| [RA10](#ra10) | Nonzero constant webs depend on register demand and live range, not literal spelling |
| [RA11](#ra11) | Free volatile accesses: change the load web without crossing stores or pushes |
| [RA12](#ra12) | Volatile read and store forms force different spills |
| [RA13](#ra13) | Alias classes: escaped caller locals, helper temporaries and preloaded scalar values |
| [RA14](#ra14) | A single-use pointer cache survives by copy-propagation distance |
| [RA15](#ra15) | Shared byte aggregates can change the saved-register winner |
| [RA16](#ra16) | Local scope can split a register web even when loads merge |
| [RA17](#ra17) | Call-free loop allocation prioritises return-coalesced and loop-local webs |
| [RA18](#ra18) | Use whole-value copies when they change push liveness or comparison hoisting |
| [RA19](#ra19) | Invariant loads from one object preserve field-order and extent effects |
| [RA20](#ra20) | Landing-pad reloads belong to split live-range edges |
| [RA21](#ra21) | Load/ALU memory folding is decided after scheduling |

<a id="ra01"></a>

### RA01 — A real named temporary advances scratch rotation

VC6 distinguishes a named symbol, its own CSE temporary, and a copied existing value. Naming one actual load (array element, call result, intermediate pointer) can advance the eax → ecx → edx rotation where reordered expressions and a free volatile access are inert; a rename of an existing value is copy-propagated. The useful site is body-specific.

Evidence:
- `BoatingSchool_Destroy` / `JungleCruise_Destroy`: head/count/sprite volatile reads and twelve loop respellings were inert; `spr = ilf->sprites[(unsigned char)i]; LLSStop(GetLLSForSprite(spr));` added one IR temporary and fixed every later register (12 and 6, plus a byte each). This corrects the inference that an inert volatile proves a global-rank floor.
- `TempleSlide_Update`: caching a real indexed-global value fixes 27 indices through two switch cases as a clean three-cycle rotation.
- `BuildWalkPath`: a repeated computed allocation-size expression has a CSE web in eax; naming `size` puts its `lea` in edx, the final two bytes. Constant-carrier propagation does not imply computed expressions are interchangeable.
- `RandomFavouriteRide`: two `e->data->type` compares CSE their 16-bit value into cx; tries-- then takes edx instead of original ecx (strict 2, rb 0). Naming `d = e->data` closes it; a short/u16 value local still costs 2, int costs 5. A volatile pointer read is byte-identical to the named pointer on that body. Its one-compare twin needs no local: shared source need not mean identical residuals when pressure differs.
- `MoveToPanEdge`: `PanSlot* s = &g_pan_slots[b->pan];` gives the original `lea` plus scaled-subscript pair while the other field is still re-derived as a subscript, 6 → 0; transfers first try to `StandUpFromPan`. A flat `const int tbl[]` uses an index scaled by 6 and `[eax*4 + disp32]`; a 24-byte struct array scales by 3 and `[eax*8 + disp32]`, one instruction short.
- `CellAt(at->x, at->y)` makes the pointer the first IR temporary (eax); explicit int x/y reads first move it to ecx, fixing 7 of 41 at identical length.
- `DrawPopUpInfo`'s strip: naming `r = a->x + 0x24` before `if (r < m || m < l)` gives edx/eax/ecx where the inline expression gives ecx/edx/eax.
- `RenderSpriteX` needs the named `f->n16` at the third site (else arm); `SoftBlitRLE` at the SECOND (post-walk call in the base-image arm), worth 14. There the surviving CSE makes both tail pointer chains use the same registers, allowing FOUR more instructions to cross-jump and changing 141 instructions to 137. A short shared-tail body can therefore be a register-identity effect.
- `codex-c`'s ordinal list lookup: a separate cursor changes 13i/29B with 9 mismatches to 11i/22B exact; signed/unsigned index is inert. `DeleteIcon`: naming `next = prev->next` changes the web where a free volatile head read is inert, 23i/57B exact. `GetTypedChar`: current-key evaluation before the previous-state bit adds the original EBX web, even though the previous byte is emitted first.

Evidence: [D018](DECOMP.md?plain=1#L741), [D026](DECOMP.md?plain=1#L802), [D067](DECOMP.md?plain=1#L1157), [D080](DECOMP.md?plain=1#L1243), [D097](DECOMP.md?plain=1#L1334), [D127](DECOMP.md?plain=1#L1601), [D157](DECOMP.md?plain=1#L1930), [D173](DECOMP.md?plain=1#L2064), [D274](DECOMP.md?plain=1#L2852), [D464](DECOMP.md?plain=1#L4308).

<a id="ra02"></a>

### RA02 — Named locals and direct global expressions have different webs

Choose whether the original uses a named symbol or VC6's repeated-expression CSE. Repeated reads of a class/definition global can take a callee-saved register that a named pointer cannot; intervening stores may destroy the CSE. Compute both derived sums before either object store when one common global load must survive.

Evidence: `LFCsaw_Place`, a `RideDef* def` local → two direct global reads, 18 → 0. At the later measured class-def site the named pointer takes a scratch register and pushes the tile byte onto edx, while the compiler's own CSE takes EBP; interleaved stores reload the global and cost 16. `UpdateEntranceTile`: two `g_elem->data->rect...` expressions around `g_tile.x = ...` reload the element and re-derive the class (15); naming `ObjDef* d` closes it, demonstrating that a store to one global kills CSE of an unrelated global load. `LFBoat_Draw`: naming `Person3D* person = b->rider->person` keeps the pointer across calls with one reload instead of re-deriving it after every call, 132 → 48, pinning a callee-saved register and the frame. `Cache b = r->bloke` but use `r->bloke` at the trailing call to spill the walker into a dead slot and keep the bloke in esi. `RideTile* tile = RIDE_TILE(r)` can yield an edge-rematerialised `lea`, a CSE surviving a jump table, and removal of the function-wide zero web in one edit.

`WW_AnyBlokeInRect` already has VC6's invariant hoists: naming one rect field costs 16 and naming two costs 24. `Map* m = g_map` folds to a direct global read when two CSE-able consumers share the pointer expression (helper-local/caller-local/parameter, at top or first use: 104 measured groupings). Two different locals stop sharing and create a PRE web with entry-edge landing-pad reloads instead; landing pads and cross-consumer field CSE are mutually exclusive for those pointer spellings (`GetObjectUID`, 20).

`UpdateSidePanelScroll`: copy `state=g->f00` but compare GLOBAL `g->f00==2 || g->f00==3` again, keeping one register for compares/local store/peeled first read. Testing state emits mov al,cl and splits the web. Also assign p=g_list BEFORE the guard to put its load among pushes; together 51 → 14.

Evidence: [D035](DECOMP.md?plain=1#L960), [D060](DECOMP.md?plain=1#L1110), [D090](DECOMP.md?plain=1#L1310), [D123](DECOMP.md?plain=1#L1575), [D169](DECOMP.md?plain=1#L2029), [D192](DECOMP.md?plain=1#L2221), [D242](DECOMP.md?plain=1#L2608), [D288](DECOMP.md?plain=1#L2962), [D365](DECOMP.md?plain=1#L3496).

<a id="ra03"></a>

### RA03 — Read values before a call or branch to create the missing live web

A missing callee-saved push can mean a value was computed too late. Name the value before the split or before the call it must survive. A field of an address-taken aggregate cannot stand in for a plain call-crossing scalar: copy ordinary locals INTO that aggregate at the required call site.

Evidence:
- `LFPath_Point`: 64 instructions / three pushes; `j = i + 1` before the branch makes five values live across the join and introduces ebp, 69/69 byte-exact.
- `Restaurant1_WalkToSeatSpot`, indices 1..7: values named before a call stay live in saved registers; read only at their uses, they load after the call and one saved push disappears. A related early float read stays at index 7 using `[ecx*8+base]`; read at use, it sinks 20 indices and forces `shl eax,3` plus `[eax+base]`, 22 → 6.
- The two-field pre-call rule in `fable-b`: inline later expressions load after the call, one push fewer, 58 of 74; named reads before the call recover the saved registers. `codex-d`: `int x = pos.x; int y = pos.y;` before two calls restores the saved-register assignment where inline fields cost 8.
- `RenderGroundLayer`: reading shifted scrolls back from a by-address `Pos` causes two reloads and merges `add esp,8` into the epilogue (36 of 42); plain int locals assigned INTO the Pos stay in esi/edi across the call, exact. Two reads of the same u16 global do not CSE across an escaped aggregate store; `view.right = g->view_w + view.left` reproduces the required single-load form.
- `BoatingSchoolWater_Remove`: widen four bytes into `int ax, ay, bx, by` before the call in that order; inline fields are evaluated right-to-left into pushes. Of all 24 orders only ax, ay, bx, by is exact, and the walker changes esi → edi.

The projection-specific zero-cost read-before-call tie (22 versus 327, including the one-read-before case) is recorded with the projection-order rule; extern/by-value argument shapes can change which values actually cross the call.

Evidence: [D006](DECOMP.md?plain=1#L504), [D052](DECOMP.md?plain=1#L1061), [D066](DECOMP.md?plain=1#L1149), [D081](DECOMP.md?plain=1#L1251), [D097](DECOMP.md?plain=1#L1334), [D236](DECOMP.md?plain=1#L2565).

<a id="ra04"></a>

### RA04 — Copy an out-value immediately after its producing call

`e = g;` immediately after the call filling `&g` loads into one register for both the compare and return, scheduled in the call-return slot before the pending `add esp,8`. Reading the address-taken local separately at its uses produces one `[esp+4]` load per block: it is not CSE'd across the branch. Likewise, making a cursor the callee's out-parameter (`&cur`) prevents its allocation to a fourth saved register (66 of 79 → 8), freeing ebx for a float's dword copy. Naming `&p->cls->part` produces `add eax,0x3c` while the first access still folds into `[eax+0x3c]`; a pointer to the outer struct does not.

Evidence: [D028](DECOMP.md?plain=1#L913), [D047](DECOMP.md?plain=1#L1032).

<a id="ra05"></a>

### RA05 — Root assignment and initialiser order determine early zeros and spill positions

Move initialisation statements rather than declaration-only order when the original's first scratch values are permuted. Explicitly initialise an index before the pointer/key work and omit reinitialisation in the for clause; the surviving IR definitions decide the order.

Evidence: `codex-d`, `int i = 0` before the u16 key is the fifth early-zero instance; `scope-e`, zero before pointer initialisation is the fourth, and returning the loop ordinal from `Copters_StepRider` coalesces the counter with eax and removes a duplicate success store. `GetObjRiderN`: `int i = 0` after the first call but before the guarded for puts `xor esi,esi` before `test eax,eax`, reusing zero as the compare operand, 3 → 0. `LFRun_LoadPieces`: all 24 orders measured, only tag, p, head, prev exact; natural order costs 3. `BsWater_Probe`: among three initialisers, writing the one whose spill is emitted last last places its spill after the other xors. `for (i = 0, n = 0; ...)` emits counter xor before accumulator xor; a preceding `n = 0` statement reverses them. `codex-e`: name image length before initialising the counter, then compute `end = p + length`, placing pointer addition after the zero.

`ChildrenBarInput` needs a FUNCTION-level chain-head local assigned AFTER the global store. Inline `p->owner->obj->elem->flags |= 8` gives the wrong eax/ecx/edx rotation (3 at identical length); block-scope gives correct rotation but sinks the store below three of four pushes (3 the other way). Only function-level declaration plus store-first gets both, 7 spellings, original first dereference `mov eax,[eax+0x18]`, transferred to its twin.

Evidence: [D001](DECOMP.md?plain=1#L427), [D006](DECOMP.md?plain=1#L504), [D007](DECOMP.md?plain=1#L519), [D011](DECOMP.md?plain=1#L589), [D014](DECOMP.md?plain=1#L713), [D062](DECOMP.md?plain=1#L1122), [D118](DECOMP.md?plain=1#L1541), [D167](DECOMP.md?plain=1#L1997).

<a id="ra06"></a>

### RA06 — Independent statement order creates and frees register webs

Independent source statements can change web creation/lifetime even when the scheduler emits the same instruction order. A later scheduling position does not itself assign a register; measure the values held in each register as well as their positions.

Evidence:
- `RenderWorkOrders`: `colour = K` before `o = head` in both entry arms keeps o in edx across the loop back edge and removes a landing-pad reload, 320 → 0; emitted order is unchanged, scope/declaration/loop changes inert.
- `RenderView`: `mode = 0xff00` FIRST rather than last puts the two coordinates in saved registers across sibling-arm calls, allows a common four-instruction suffix to merge, and realigns an 800-instruction tail (116 alone). Last: reloads cause three instructions of shift.
- `LFEntrance_Update`: link-chain constant stores first reserve eax for zero and leave pointer loads alternating ecx/edx, despite global immediates sinking below computed stores. A named `a.y = fp1 + my; a.x++; a.y -= dy` preserves `(fp1 + my) - dy`, merges to one store, and changes priorities back into the prologue; single expressions canonicalise.
- `LFEntrance_Add`: one independent assignment moved ahead of another buys the n-1 spill slot, 0x28 frame, and all offsets, making indices 0-61 identical.
- `ClampScrollToMap`, 190/190 after 34%: independent multiply order decides which product stays in eax; a two-def `v += e; if (x > v) x = v` keeps v across a call where repeated `v + e` has one def. Parameter-read CSE is function-wide; naming an early read lengthens its range and can lose later caching plus the edge-fix-up `jmp`/`mov reg,[esp+arg]`. Inline that early use. Declaration order, `register`, `&&` versus nested ifs, `y+y`/`2*y`, parameter copies and identity inline helpers are inert for this class.
- `RES_FindVolumeOnAnyDrive`: `found = 1` BEFORE strcpy is worth 18 of 138; after, found spills and the parameter takes ebp. `if (mask) { loop }` must also replace `if (!mask) return found`, which coalesces found=0 with i=0 and deprives the flag of a register. All three levers together 138/138; partial combinations 117, 120, 135. A strict 39 / rb 24 residual was these statement positions, not an irreducible structural difference.

Adjacent address-store pair: job.v[0..2]=&v[0..2] in source 0,1,2 emits 1,0,2 (original); source 1,0,2 emits 0,1,2, cost 4. For two constant stores, nverts before ntris, opposite their emission order, advances the desired rotation, 32 → 8. This is a measured pair permutation, not a universal reversal of longer runs.

Evidence: [D026](DECOMP.md?plain=1#L802), [D143](DECOMP.md?plain=1#L1829), [D217](DECOMP.md?plain=1#L2427), [D227](DECOMP.md?plain=1#L2503), [D228](DECOMP.md?plain=1#L2508), [D309](DECOMP.md?plain=1#L3102), [D373](DECOMP.md?plain=1#L3541), [D400](DECOMP.md?plain=1#L3761).

<a id="ra07"></a>

### RA07 — A zero variable and a constant-zero web are different objects

A zero value that must return through a saved register needs a variable live across calls with multiple reaching definitions; plain identical `return 0` arms are globally propagated. A previously initialised live index can provide a SECOND zero register alongside the function-wide literal-zero carrier.

Evidence: `fable-c`'s `int found = 0`, nested tests assigning `found = 1` only in the inner body, one `return found`, yields `xor edi,edi` then `mov eax,edi` eleven instructions later, 92/92. Three textual `return 0`s give 115 (three epilogue copies); `goto fail; fail: return 0` gives 88 with `xor eax,eax`. `LFPiece_IsVisible` in logflume2.c demonstrates a one-use saved zero via `int r = 0; if (p) r = f(p); return r`, not a rematerialisable constant.

`Castle_InitEntranceTrack`: `i = 0` at the TOP before the first call and `for (; i < n; i++)` make still-zero esi supply off.y/off.z float stores while ebx supplies push-0.0f arguments and bytes; for-clause initialisation retains one zero and makes the body one instruction short, 170 of 230 → 187 of 231. The final exact body is 231 instructions / 811 bytes.

Evidence: [D013](DECOMP.md?plain=1#L607), [D026](DECOMP.md?plain=1#L802), [D390](DECOMP.md?plain=1#L3680).

<a id="ra08"></a>

### RA08 — Constant-zero hoisting is loop-weighted and sensitive to copy placement

Count surviving uses in their loop context; do not infer a universal four-literal threshold. Two uses BOTH inside a loop suffice (a scratch register if nothing crosses a call), one inside plus one outside does not, and three straight-line uses do not ordinarily hoist. Byte/word zero compares are not qualifying uses: signed-char global → `test al,al`, pointer → `test cl,cl`, short field → `cmp word ptr [eax+6],0`. `PlayMIDI` 0x4805d0's `cmp word ptr [eax+0ch],si` reuses a hoist already earned by two loop stores; it did not cause it. A sole callee-saved zero carrier is esi in 12 of 12 matched instances (1 to 38 uses); only five matched functions push ebx alone, three because of bl/bh. The former no-instance claim over 1542 exact bodies was a scan artefact: it rejected carriers redefined later.

Body-local measurements refine this rather than replacing it: `BoatingSchool_Update` has three literal zeros, with `rep movsd` interleaved it uses already-pushed esi for zero (32 of 46), but finish the store chain BEFORE the copy and all three remain immediate, 46/46; its five-zero sibling has a carrier. `Restaurant2_NewRecord`'s eighteen literal zeros hoist xor ebx,ebx. Seven straight-line zeros in a teardown also hoist. Four `& 4` word tests with ebx already saved naturally hoist `mov bl,4`, extending the family to nonzero byte constants.

Adjacent zero stores to locals/struct fields can share xor+register stores; absolute-address globals instead use the constant-web decision. `InitExitCheckBox`'s adjacent global zero stores remain 10-byte immediates despite a register form being four bytes shorter; its three dword zeros/no fourth use have five earlier passes, not an unexplored opportunity. At the source-separation site all 128 placements were measured and only the extreme first-statement/last-statement separation worked; ordinary alias stores, a surviving branch, a loop, or address-taking did not split the zero stores, while a CALL did. The historical dead-store exception is superseded by early dead-store elimination: eliminated stores cannot buy a zero use or register class.

For independent zero registers or ending a zero web with an intrinsic, see the sub-object/last-group memset rule; memset introduces its own address and fill temporaries.

Evidence: [D007](DECOMP.md?plain=1#L519), [D013](DECOMP.md?plain=1#L607), [D026](DECOMP.md?plain=1#L802), [D063](DECOMP.md?plain=1#L1127), [D083](DECOMP.md?plain=1#L1266), [D328](DECOMP.md?plain=1#L3229), [D390](DECOMP.md?plain=1#L3680), [D422](DECOMP.md?plain=1#L3936).

<a id="ra09"></a>

### RA09 — Which register is freed determines the zero carrier and the whole saved-register permutation

A hoisted zero displaces another live value; it does not merely replace immediates. Where source work frees the preferred register can change every later zero compare/push/store, saved-register order and frame displacement.

Evidence: `Castle_InitEntranceTrack`'s six curve-chain stores BEFORE `sq = corner` keep ebx busy and give zero to edx; after, ebx frees early and takes zero, shifting every register in the final two loops/epilogue. The correct placement takes 201 of 231 → 231/231; prev before next in each pair is also required (227 reversed). `codex-e`'s tower: four height clears before four car-flag clears make EBX the live zero before masks, worth 20 at unchanged 56i/175B. `JungleCruise_Tick`: b=inst->bloke BEFORE the station search gives zero ebx and outer cursor ebp; after swaps them, 32 head orders measured. Dead zeros vanish before creation and an early surviving zero store does not move this ranking.

`RenderView`: a tail zero can displace another saved value, extend it to the function end, prevent that register's push sinking, rename all registers and shift a frame slot. Diagnose which original value is freed at its pop, not only first use. `BuildChannelTables`: put the unsigned→double loop-invariant conversion INSIDE the loop to let the hoisted zero high dword share i=0 after cl's shift-count use is dead; zero takes ecx and eax remains shift scratch. Before-loop placement allocates zero while cl is occupied. The allocation follows the hoisted IR tuple before scheduling.

Moving a trailing test below a goto-skip label keeps zero live through the loop: preheader xor edx, rematerialisation after clobber, and cmp r,edx replacing test at eight sites (30 instructions). A =1 store is immediate if not live-out, register-backed if a subsequent flag compare keeps its constant live through the join; degenerate branches, dead stores and named one carriers fold away.

Evidence: [D001](DECOMP.md?plain=1#L427), [D013](DECOMP.md?plain=1#L607), [D211](DECOMP.md?plain=1#L2384), [D248](DECOMP.md?plain=1#L2663), [D318](DECOMP.md?plain=1#L3165), [D342](DECOMP.md?plain=1#L3316).

<a id="ra10"></a>

### RA10 — Nonzero constant webs depend on register demand and live range, not literal spelling

Constants are keyed by value: suffixes, casts, `37` versus `0x25`, and return type do not separate them. The corpus gives incompatible simple use thresholds, so use the controlled `ClampPopUpToScreen` outcomes as the rule: compare + one register use keeps the immediate; adding a store builds the web and puts its definition at the common dominator; the store alone stays `mov [mem],imm`. This supersedes both an unconditional two-register-use threshold and the later claim that any single materialisation suffices. Register occupancy can break that web: keeping eax occupied across the compare reproduces the original 43 instructions / 144 bytes in a probe.

Evidence: `if (cost < 50) return 50` can retain `cmp eax,32h / jge / mov eax,32h`. `ClampPopUpToScreen`'s body sat at 3 because the compare and arm constants merge; changing only the compare to a value one apart gives 2 at 144/144 bytes and the entire low arm exact, but is not adopted because the original has the other value. Its missing byte is `cmp esi,eax` (2 bytes) against `cmp esi,25h` (3), not a shl/lea choice.

`PrintSavedGameDetails` (~200 variants): a nontrivial constant (not 0/1 or lea-folded) used as a non-lea immediate before/in a loop and again afterward makes a phantom preheader web, claims ebx ahead of call-crossing values, then spills/rematerialises invisibly. Demotion needs the partner use in the join/preheader or loop. A late-merged `if (!d) { X } else { X }` on a cached local with identical call-containing arms cures it without emitted code; positive if(d) duplicates X and a global test leaves a ghost reload. Spelling/type/static const/callee types/loop forms are inert (`static const` loads are not folded).

Evidence: [D195](DECOMP.md?plain=1#L2245), [D256](DECOMP.md?plain=1#L2707), [D310](DECOMP.md?plain=1#L3108), [D360](DECOMP.md?plain=1#L3448), [D389](DECOMP.md?plain=1#L3672).

<a id="ra11"></a>

### RA11 — Free volatile accesses: change the load web without crossing stores or pushes

A free volatile-qualified access at a site the original already loads can inhibit CSE, force a reload, and advance scratch rotation without adding an instruction. Apply it to the DEFINITION load when that is the original site. A volatile access cannot cross any store, including callee-saved pushes; this barrier is inseparable from its memory effect. It does not imply one universal emission index: ordinary hoists across writes are forbidden, while a lone volatile read can become FIRST in its own basic block (the `UpdateSampleSource` limit below).

Evidence:
- `ClearCellForPath`: definition-site volatile is 167/167; use-site puts the required reload after an intervening push, 166; preserving CSE costs 36, roughly 30 nonvolatile spellings floor at 131.
- `FindCachedText`: volatile loop-guard g_count pins its load at index 5 below push ebx/ebp/esi/edi instead of index 0, closing 9 of 67 at identical bytes; eight loop forms floor at 9; original reloads in the latch anyway.
- `BoatingSchool_Tick`: one rotation step affects whether identical-call arms merge through pushes or only from call; 289 → 53, plus recorded site payoffs 219 and 115. Apply only where the original has the copy; at a straight store-register push it converts CSE into a reload and loses.
- `SubtractObjRect`: strict 14 / rb 0, nine byte-identical spellings, closed by one free volatile; using the other candidate load closes 11 of 14 but moves a spill two instructions early.
- `TraceRoute`: free volatile gave a selected byte-exact 105-mismatch row at 339/339 B versus lowest-mismatch 101 at 340 B; no claim of exact matching.
- `codex-c`'s boat->piece free read closes a four-register residual at unchanged body where split statements are inert. `LFPiece*` fwd read at a guard removes CSE and a fourth saved push, 66 → 63 instructions.
- After `rep movsd`, volatile only the FIRST read-back field pins the whole four-load group below the copy, 30 → 10; all four volatile cost 21 because loads stick to their stores. `Coaster3D_ResetScene`: after g_base=g_tmpl a first-field `*=0.5f` folds into `mov [m0],0x3f4d3a3f`; one volatile read defeats it, memcpy and an inline Half(float*) do not.

A declared volatile matters through its actual volatile accesses: `volatile int x` accessed entirely as `*(int*)&x` is byte-identical to the plain object. This corrects the earlier claim that declaration alone is a separate forcing lever. `extern volatile int g_music_disabled` affects actual global accesses at two sites (9): plain loads hoist above =1 and shutdown stores sink into pops; other translation units can legitimately declare the global plain int.

`RouteStepAxis` needs one free volatile x1 read to preserve an independent subtraction/test, 17i -> original 18i; it is a CSE-preservation use of the same explicit-access handle.

Evidence: [D003](DECOMP.md?plain=1#L475), [D018](DECOMP.md?plain=1#L741), [D072](DECOMP.md?plain=1#L1192), [D091](DECOMP.md?plain=1#L1314), [D104](DECOMP.md?plain=1#L1465), [D120](DECOMP.md?plain=1#L1553), [D138](DECOMP.md?plain=1#L1675), [D170](DECOMP.md?plain=1#L2037), [D190](DECOMP.md?plain=1#L2204), [D300](DECOMP.md?plain=1#L3038), [D381](DECOMP.md?plain=1#L3594).

<a id="ra12"></a>

### RA12 — Volatile read and store forms force different spills

`*(volatile T*)&x` at ONE use leaves the definition ordinary, makes the local genuinely address-taken, and forces spill-at-def plus reload-at-use. `*(T volatile*)&x = e` forces a home store while allowing ordinary later reads to remain in registers. Declaring all accesses volatile can add reloads everywhere. Choose the measured side of this tradeoff; there is no general free combination of forced residency and freely hoistable ordinary reloads.

Evidence: cast-store `*(void* volatile*)&x=e` on `Coaster3D_BuildTrackMesh`, 124 → 14. One-use cast reads had independent 53 → 13 and 82 → 58 wins. `f(r, *(Rec* volatile*)&rec, cap)` forces the original spill between test and je and frees the register for the next value. A volatile self-store can fold away. A read shim may flip a full saved-register assignment but, by taking the local's address, remove a neighbouring spill home; it remains only a probe if that changes the frame.

Read the stack PARAMETER itself, `(*(T* volatile*)&p)->f`, to split function-wide argument-read CSE and force its original slot re-read; volatile only the field gets load order without registers, and both sites are worse. A repeated ordinary field read is a compiler temporary hoisted with its dependent lea as a critical-path root; naming it makes an ordinary symbol whose load stays in program order, per call site. A cast volatile STORE to a global prevents a later global load hoisting across it.

In both if arms, identical used volatile reads can hoist into the dominator; only a DEAD bare volatile expression survives separately per arm. They are not guaranteed per-arm barriers. A volatile-to-scalar flag also distinguishes IR temporary from symbol: `north=mask&1; if(north)` rotates its rvalue, while `n0=mask&1; north=n0; if(n0)` gives a variable def in eax outside the rotation (`BoatingSchoolWater_Remove`, 38 → 21).

Evidence: [D140](DECOMP.md?plain=1#L1812), [D235](DECOMP.md?plain=1#L2560), [D281](DECOMP.md?plain=1#L2921), [D301](DECOMP.md?plain=1#L3048), [D315](DECOMP.md?plain=1#L3145), [D331](DECOMP.md?plain=1#L3251), [D349](DECOMP.md?plain=1#L3372), [D382](DECOMP.md?plain=1#L3602).

<a id="ra13"></a>

### RA13 — Alias classes: escaped caller locals, helper temporaries and preloaded scalar values

Stores to an address-taken caller aggregate can kill availability of unrelated pointer-field loads across the whole object. Explicit scalar reads before the store can retain values; values born in a static inline helper are outside the caller's escaped-local class. The helper's origin, not a cheap syntactic address-taking disguise, is the significant distinction.

Evidence:
- `RequestRoute`: `to.x=...; to.y=...; to.x=...` gives 483 instructions; first two share a register web, third is fresh `mov ecx,[esi+8]`. Interpose `to.y--;` before `to.x=cur->pos.x` and a fresh load appears 12 indices later. This leaves same-web or fresh-load regimes, not the unexplained register copy (see scoped floor).
- `LFEntrance_Add`: `fx=def->v[0]; fy=def->v[1]` before escaped stores lets a.x assignments combine as add/inc/one store; two adjacent `b.x=...; b.x++` fold into lea[..+1]. Plain scalars in place of the escaped Pos recover four apparent issues together (hoisted byte, store between flags load/or, split mov cl/sub al,cl rather than folded subtract, operand order) and the saved-register tie. The head needs the opposite shape, because scalars merge definitions into lea[eax+edx+1]; the local improvement is not committable as a whole.
- `EarthSlide_Tick`: helper-born addressed local permits an unrelated pointer load above the store, closing the last 2. Function-level local passed to a real call remains pinned.
- `PlaneRide_Interact`: escaped Offset → inline helper block, 5 → 0; extra argument by value, pointer or two ints all exact. `ValidateCursor`: helper-born addressed locals stop alias reloads and remain in caller pool slots, 14 → 5.
- `PlaneRide_PlaceRider`, `SpinningBarrels_PlaceRider`, `Balloonz_PlaceRider`: move the escaped `pivot` Offset (addressed by `AdjustOffsetForViewMode`) and `AdjustBlokePosition(&p->local)` block into a helper so value temp becomes ecx and `lea &p->local` is eax like neighbouring blocks; open-coded loop gives value eax / address ecx. About 35 casts/pointer locals/named differences/`pivot.oy=pivot.oy-8` variants inert.
- `Balloonz_Draw`: name pointer field into int BEFORE escaped update to allow difference before store, changing lea-then-store-through-lea into based store then later lea. Explicit int temps for Offset members DO hoist above a may-alias store; the earlier no-source-order claim is withdrawn.
- `Balloonz_Tick`: before a may-alias global-pointer store, read the field to keep the barrier at the required site; this removed ESCAPES and 12 duplicated instructions. `next; key=&r->ride_id; b=r->bloke` keeps cursor eax and lea ebp,[eax+0xc]; b first moves cursor into a saved register.

An address-taken ARRAY at function level can likewise block pointer-load hoists; moving it into the loop recovers three-deep software pipelines at four sites. At the measured baseline one block spanning both loops, one per loop, and deeper scopes are identical; later unrelated fixes made the spanning-block form three worse, so that depth equivalence is baseline-specific. Block-scoping plain scalars is inert. See same-width typed-object CSE for the separate type barrier; same-width union views are not distinct objects.

VC6 does not hoist a store to an address-taken local above a call, so moving its source statement below a call cannot recover a store pair that must precede it.

Evidence: [D184](DECOMP.md?plain=1#L2142), [D193](DECOMP.md?plain=1#L2227), [D239](DECOMP.md?plain=1#L2590), [D250](DECOMP.md?plain=1#L2681), [D259](DECOMP.md?plain=1#L2735), [D266](DECOMP.md?plain=1#L2791), [D275](DECOMP.md?plain=1#L2862), [D276](DECOMP.md?plain=1#L2875), [D286](DECOMP.md?plain=1#L2948), [D292](DECOMP.md?plain=1#L2993), [D418](DECOMP.md?plain=1#L3910), [D435](DECOMP.md?plain=1#L4042), [D459](DECOMP.md?plain=1#L4253), [D480](DECOMP.md?plain=1#L4411).

<a id="ra14"></a>

### RA14 — A single-use pointer cache survives by copy-propagation distance

A pointer cache used for ONE store survives only when another statement sits between assignment and use; immediately adjacent it copy-propagates to no cache. It is not a scheduling barrier: all 140 surrounding interleavings are byte-identical and the load hoists across the escaped stores. A second cached use is much worse when the original re-reads after a store through the pointer.

A pointer cache also consumes a register. `p=b->person` above an escaped store can occupy ECX, push the next scratch temp to EDX, and make that temp hoistable; the five ride activation functions leave ECX free across about 20 original indices, which source variants fail to reproduce. The full corrected `SpinningBarrels_Activate` / `SafariRide_Activate` residual is recorded separately. `Joust_Draw` demonstrates a payable helper cost: a known inline-argument reorder fixes three instructions but causes 63 elsewhere by removing a web from scratch rotation; a two-level rider guard with cached bloke BETWEEN levels restores the hoisted-key rotation, 3 → 0. Without the cache the guard is inert.

Evidence: [D203](DECOMP.md?plain=1#L2315), [D297](DECOMP.md?plain=1#L3022), [D335](DECOMP.md?plain=1#L3273), [D404](DECOMP.md?plain=1#L3789).

<a id="ra15"></a>

### RA15 — Shared byte aggregates can change the saved-register winner

Only EBX is byte-addressable among callee-saved registers, so of two byte values surviving calls one must spill. Two separate scalar byte locals can choose the opposite winner even when one has more weighted or loop-nested references; merely changing its type to int can flip it. A SHARED two-byte aggregate changes this tie in the measured update-both-halves cases while still scalarising.

`LFTunnel_Place`: two scalars, 164 of 217 and wrong instruction count; `BPos c`, 5 of 217 at 680/680 bytes. `BPosW` and `unsigned char c[2]` are byte-identical to BPos; TWO one-member structs behave as separate scalars. The shared member also prevents the final coordinate update sinking into a trailing if past a call. This does NOT transfer to `LFDrop_Place`, whose c.x is assigned only once while the successful bodies update both halves.

`LFDrop_Place`: eight name pairs, declaration/statement order, extra definitions, running-coordinate reuse and four store orders all byte-identical. Type can give the original 18-instruction head plus a spurious `and ebx,0xff`, or exact 341 bytes with a dword head. The scoped unresolved requirement is a byte-typed value ranking like an int, not a universal unreachable byte tie.

Wave eleven called the byte tie forced and unreachable; the shared-aggregate result narrows that claim to the failed bodies, rather than the entire byte-local class.

Evidence: [D168](DECOMP.md?plain=1#L2002), [D183](DECOMP.md?plain=1#L2132), [D352](DECOMP.md?plain=1#L3400).

<a id="ra16"></a>

### RA16 — Local scope can split a register web even when loads merge

`WriteCoasterNodes`: declare the list walker separately inside EACH arm to split its web and give out esi. One function-level walker outranks out, swapping esi/edi through 24 instructions (36 of 60, rb 0); two block-scope walkers give 60/60 while the common load still hoists above the branch.

For equally referenced parameters the LATER parameter wins (two probes, both directions); no statement order changes that tie, only a greater surviving count or allocation-regime change. Copying half a parameter's uses into a local does not split its web: VC6 coalesces and combines them. Likewise splitting/merging mere variable names is inert when their value webs coalesce, as the `Carousel_Tick` store-by-store reference bisection demonstrated. Scope is effective only where it actually changes the surviving web, as in the arm-local example.

Evidence: [D073](DECOMP.md?plain=1#L1199), [D138](DECOMP.md?plain=1#L1675), [D397](DECOMP.md?plain=1#L3743), [D434](DECOMP.md?plain=1#L4040).

<a id="ra17"></a>

### RA17 — Call-free loop allocation prioritises return-coalesced and loop-local webs

Measured on `WW_AnyBlokeInRect` (~110 variants): return-coalesced web first (eax even across whole loop), loop-local temporaries in definition order, then loop-carried values, then invariant hoists. A cursor falls behind a loop-local temp unless it flows into return. `return (int)p` folds to 0 at a single-predecessor exit but stays unfolded at the inverted loop's merged exit, coalescing cursor with eax and suppressing zeroing. Phantom uses p^p, p&0, p?0:0 fold; `(int)p >> 31` survives. Compare operand order remains visible: `r->left <= x` → cmp esi,eax/jg, `x >= r->left` → cmp eax,esi/jl.

The honest committed shape was deliberately changed from strict 6 to 12: 6 had 30 instructions / 67 bytes, one short each, and misplaced mov eax,1; literal `return 0` gives 31/31, 68/68, identical blocks/branch offsets and rb 0, every mismatch eax/ecx. Its race depends on the LOOP-LOCAL temp's references: 0 or 1 in-loop compares gives cursor eax; 2 gives temp eax; extra cursor references cannot buy it back. In the small predicate, return-coalesced pointer versus any literal zero gives a binary cursor/temp swap immune to ~150 spellings. Compiler temporary spills may use an intervening register copy, whereas pushes do not; named locals spill straight from eax.

Evidence: [D272](DECOMP.md?plain=1#L2832), [D325](DECOMP.md?plain=1#L3212), [D446](DECOMP.md?plain=1#L4114).

<a id="ra18"></a>

### RA18 — Use whole-value copies when they change push liveness or comparison hoisting

`GetObjectDoorOffset`: represent the two coordinates as ONE Pos field and assign `*out = d->entrance` whole. It breaks CSE with the bounds test and sinks push esi into the other arm. Two int fields cost 23 at identical instruction count, about 20 spellings flooring there; reversing cold-arm store order also sinks the hot-arm push. For a duplicated compare `fwd->sq.b.x != p->sq.b.x`, naming just the correct unsigned-char operand makes VC6 hoist shared loads and repeat the test: raw fields cost 71 of 144 and two instructions; one px local leaves 2; both byte locals enlarge push ecx to sub esp,8 and cost 94, as does naming only the wrong operand. Whole aggregate copies and selective symbol creation are distinct levers; do not add all convenient caches at once.

Evidence: [D034](DECOMP.md?plain=1#L953), [D151](DECOMP.md?plain=1#L1881).

<a id="ra19"></a>

### RA19 — Invariant loads from one object preserve field-order and extent effects

`CastleDummy_Interact`: two invariant loads from ONE struct hoist in descending displacement order, y at +0x0a taking the first saved register and x at +0x08 the second; separate globals always hoist x first regardless of statement order/names. A short field's hoisted access widens to mov r32,dword when the struct ends 4 bytes after it (x +8 in a 12-byte struct); trailing fields after y keep mov r16,word. Leave the compare key as `sq.id == t.id` in the loop to hoist it after the trip guard; an explicit scalar before the loop moves it before the guard and reshuffles esi/edx.

Evidence: [D245](DECOMP.md?plain=1#L2626).

<a id="ra20"></a>

### RA20 — Landing-pad reloads belong to split live-range edges

For a bounds-checked map-cell probe, mov r,[g_map] before a sign test with some edges skipping that load is a live range split with EDGE rematerialisation, not a load scheduled at each use. It appears when that block dominates later probes or the value is loop-invariant. An `if(x>=0)` guard can deny the required dominance; a Map* local then copy-propagates and rematerialises per use-block. At `GetObjectUID` (104 pointer groupings), consumer-field CSE and the desired PRE landing pads remain mutually exclusive at 20. Keep the function-wide parameter-CSE/liveness rule separate from ordinary source statement order.

Evidence: [D288](DECOMP.md?plain=1#L2962), [D345](DECOMP.md?plain=1#L3344).

<a id="ra21"></a>

### RA21 — Load/ALU memory folding is decided after scheduling

A load and its ALU operation fold only if adjacent AFTER scheduling; an unrelated store scheduled between them prevents folding. In the unnamed D399 body, the original never folds its global byte into sub r8,byte ptr[mem], while the reconstruction folds four of six sites. The measured observation locates the decision after scheduling; it does not establish a universally available store-placement cure. At other named bodies an escaped-store barrier or named preload changes which loads can be folded; inspect that context before changing the arithmetic.

Evidence: [D399](DECOMP.md?plain=1#L3758).

<a id="frame-layout"></a>

## Frame layout

| Rule | Question |
| --- | --- |
| [FR01](#fr01) | Scalarisation does not erase aggregate placement |
| [FR02](#fr02) | Dead argument homes: scope, root copies and aggregate membership |
| [FR03](#fr03) | Block-local output symbols can prevent head merging while sharing a slot |
| [FR04](#fr04) | Pool reuse depends on interference and origin, not declaration-only shuffling |
| [FR05](#fr05) | Addressed-output pair order depends on the actual lifetime and first value use |
| [FR06](#fr06) | Aggregate initialisers are positioned by scope and are non-aliasing stores |
| [FR07](#fr07) | Phantom home plus dead spill: distinguish an existing slot from an extra aggregate slot |
| [FR08](#fr08) | Indexed arrays reserve homes that separate scalars do not |
| [FR09](#fr09) | Uninitialised reads may intentionally reuse another value’s home |
| [FR10](#fr10) | Inline assembly changes frame ordering globally; C reference weight still matters |

<a id="fr01"></a>

### FR01 — Scalarisation does not erase aggregate placement

An aggregate whose address never escapes normally scalarises: fields remain enregistered, with no forced memory home. Arrays, padded structs and unions can therefore have identical access code to a scalar. Aggregate identity still changes frame placement, arithmetic symbol identity, and some byte-value allocation ties. To pin relative offsets as one physical C object, the address must actually escape; a wrapper alone is insufficient.

Evidence: `BuildWalkPath`, two ints in the two LOWEST slots → one non-address-taken Pos in the two HIGHEST, worth 28 of 138; declaration order inert in both. `int a[2]`, a[4], structs with char pad[8] / int pad[3] tails, and union char c[8] all leave the tested value enregistered with sar ecx,1. `DrawBoats`' two attractors: CarPos(swx/swy,tx/ty) and Pos(ox/oy) wrappers byte-identical to scalars. Conversely, combining four escaped buffers in one struct pins pos34/F_tsm relative offsets. The draw-offset Pos at a fourth site prevents each load interleaving with its halving and avoids an ebx/edi swap of two other prologue locals (16 mismatches).

A one-pointer struct CSEs like a plain pointer and cannot force reload-at-each-use. The older blanket union de-enregistering claim (one hot-local union cost 27 points) is only evidence for a genuinely escaped access shape, not for union syntax alone; same-width union members are one lvalue, and a float/int union does not itself force a float out of a register. See shared-byte aggregate and sums rules for the independent allocation/symbol effects.

`RemoveNewObjectMarker`: two lockstep global arrays as separate externs create two IVs, register count, parameter reload each iteration, and a fourth push for hoisted zero. ONE struct gives one cursor and reloaded count via alias kill, 59i/172B → 53i/155B. This object-identity change differs from the two-distinct-link-time-base `MusicThread` case: choose from the observed cursor/count behavior.

Evidence: [D055](DECOMP.md?plain=1#L1080), [D129](DECOMP.md?plain=1#L1621), [D168](DECOMP.md?plain=1#L2002), [D171](DECOMP.md?plain=1#L2046), [D187](DECOMP.md?plain=1#L2181), [D381](DECOMP.md?plain=1#L3594), [D444](DECOMP.md?plain=1#L4099), [D489](DECOMP.md?plain=1#L4471), [D490](DECOMP.md?plain=1#L4473).

<a id="fr02"></a>

### FR02 — Dead argument homes: scope, root copies and aggregate membership

An address-taken output local declared in the block where used can take a dead incoming argument home; at function level it may take a fresh frame slot. A spilled aggregate member blocks reuse available to an ordinary scalar. Resolve all displacements by control-flow-aware push depth before inferring an out-of-frame write.

Evidence:
- `TrackJoinPieces`: function-level out-local takes frame, pushes float temp into dead argument slot; all 24 declaration orders inert, all six block-scope variants exact, final 8 closed.
- `LFBoat_Draw`: per-block int tw/th pairs put BOTH in dead argument slots; one function-level pair homes only tw and shifts whole frame, worth 98 of 299.
- fable-b's two ints → Pos: scalar spill uses freed parameter and shrinks to push ecx; aggregate takes fresh slot and original sub esp,8, 11 of 64 (sole residual). `BsBoat_StepLeg`: same turn-delta change restores sub esp,8 instead of no frame and removes the one-instruction shift after index 0, 294 instructions.
- An address-taken output-pointer pair can use both incoming argument slots after both parameters are root-copied. Conversely two argument slots reused as locals leave an 8-byte frame even without an explicit array; indiscriminate argument root copies can lose this.
- `Route_AdvanceTrain`: float acc=0.0f initializer gives acc a real frame slot; a statement AFTER owner load puts it into dead parameter slot, freeing [ebp-4] for other float, 16 versus 0; one statement earlier does neither.
- A constant `flags=0` before a call can float above the dead slot's final read and take the slot; after the call it cannot. Last read of a dead argument home is a scheduling boundary for its next occupant.
- `Castle_InitEntranceTrack`: n=count; sq=c2; t=...; step=(...)/n keeps sq live over fidiv and denies its slot to integer staging, 201 of 231; sq=c2 after step lets staging use the escaped sq slot instead of sharing step, shifting frame, 197 of 231.

Evidence: [D013](DECOMP.md?plain=1#L607), [D046](DECOMP.md?plain=1#L1027), [D063](DECOMP.md?plain=1#L1127), [D088](DECOMP.md?plain=1#L1300), [D097](DECOMP.md?plain=1#L1334), [D111](DECOMP.md?plain=1#L1511), [D138](DECOMP.md?plain=1#L1675), [D180](DECOMP.md?plain=1#L2111).

<a id="fr03"></a>

### FR03 — Block-local output symbols can prevent head merging while sharing a slot

`TrackFitEndGeom`: one function-level output local makes identical lea reg,[esp+4]/push/push argument setup merge above the branch (33 against 36). Two block-local symbols, one per arm, are distinct when head merging runs but later colour into the SAME dead parameter slot, retaining no frame. The original lea ecx in one arm and lea edx in the other proves setup was never merged. This is a symbol-lifetime decision, not a need for two physical slots.

Evidence: [D027](DECOMP.md?plain=1#L905).

<a id="fr04"></a>

### FR04 — Pool reuse depends on interference and origin, not declaration-only shuffling

Spill homes are assigned low-to-high in allocation priority with first-fit reuse of dead homes; optimized scalar declaration order and names normally do not change them. Address-taken inline-expansion temporaries share the spill pool, unlike named escaped locals. Reuse an actual variable where one physical home spans regions; separate block locals only when the original's allocation differs.

Evidence: `SaveGame` (1196 instructions) matched frame [block-scope pool][function-level locals in declaration order ascending] for its addressed-local class: one pooled slot starts the run at 0x14, two at 0x18. Disjoint block locals share regardless of lexical depth (even four levels down); an enclosing live block-local interferes, forces a second slot and gets the LOWER home. The older matching-depth/sibling-scopes restriction was wrong. Two disjoint Pos locals likewise cannot share when an enclosing addressed third is live; reuse one local for the two roles. This observed declaration-ordered addressed-local run must not be extended to optimized spills: D274/D488 explicitly find declaration reversal byte-identical and allocation/first surviving IR use decisive.

`Carousel_Draw`: each addressed Offset gets one whole-function home; correct call-site reuse plus char index and person read after two pivot stores gives 24 → 0. `SpinningBarrels_Interact`: homes split by BLIT, layer-3 in one, matte+layer-2 in the other, 39 → 0. At that site `Offset piv=g_pivot` is one 8-byte global copied whole, declared before person: descending-displacement loads with person between. In Spider a named tile at every site instead of the macro is a real lever, 212 → 165. Two loops copying the same fields in/out use one function-level set where the same homes recur; lifting exactly those locals makes a pass byte-identical, but locals allocated differently per region stay block-local.

Inline helper-born addressed temporaries reproduce four distinct S8 Pos slots after about 1400 union/struct variants failed. `StepSchoolCar`: a helper-born out-parameter shares a spill home without a fifteenth slot, but that inline shape destroys the required sum-first projection, so not a complete solution. An array born in an inline helper sits one step farther from ebp than the same block-scope array and at two or fewer references passes larger arrays to the far end; unusable for the measured Draw3D case because the helper would need an asm block naming two frame objects as MASM symbols.

For address-taken aggregates, five declaration permutations were byte-identical while moving the local into an inner loop scope moved its home DOWN; three separate higher/lower questions in one body were answered by scope alone. This D141 witness has no named function in DECOMP.

Evidence: [D141](DECOMP.md?plain=1#L1817), [D214](DECOMP.md?plain=1#L2408), [D267](DECOMP.md?plain=1#L2797), [D274](DECOMP.md?plain=1#L2852), [D277](DECOMP.md?plain=1#L2879), [D284](DECOMP.md?plain=1#L2935), [D327](DECOMP.md?plain=1#L3223), [D341](DECOMP.md?plain=1#L3312), [D475](DECOMP.md?plain=1#L4371), [D478](DECOMP.md?plain=1#L4399), [D488](DECOMP.md?plain=1#L4469), [D491](DECOMP.md?plain=1#L4483).

<a id="fr05"></a>

### FR05 — Addressed-output pair order depends on the actual lifetime and first value use

For two uninitialised dummy outputs, first evaluated/rightmost argument takes the lowest available dead-argument slot (`LFEntrance_Update2`); declaration, scope, type and helper spellings cannot reverse it. If the original reverses that pair, one dummy is the address of a dead PARAMETER. A distinct measured pair of scalars filled by ONE out-parameter call orders homes by FIRST RVALUE USE: put sum first and whichever scalar is read first gets the lower home, reversing the call's two leas. Declaration swaps, block scope and a preread probe are inert. These are scoped allocation witnesses, not contradictory universal evaluation-order rules.

Evidence: [D224](DECOMP.md?plain=1#L2488), [D448](DECOMP.md?plain=1#L4134).

<a id="fr06"></a>

### FR06 — Aggregate initialisers are positioned by scope and are non-aliasing stores

An aggregate initializer begins at its scope's entry in IR; an ordinary assignment is placeable as a statement. Function-level initializers are therefore effectively prologue fills, but the old claim that no scope can move one is false: block scope, loop placement and guards matter. They are not hoisted out of loops.

Evidence: `Carousel_Draw`, 120 permutations leave a function-level fill first; opening the array's inner scope after a load puts rep stosd there and preserves that load's register. n=0 before the fill sinks past it and the next push as immediate; after the fill it can reuse al. Another 10-spelling prologue observation had template loads between saved pushes; placing fill inside if(r) moves those to preheader. `SpiderRide_Interact`: mov dword[esp+N],0 plus rep stosd of N-1 is `T a[N]={0}`; declare n first, open array scope after r=item->riders. Two separate scalar zero stores permit later-store hoist and split byte compare mov al/cmp al; chained a.x=a.y=0 preserves IR order/direct compare, 154 → 3.

Partial initializer `BlitCtx ctx={1}` gives implicit zero fill at IR entry while explicit 1 schedules with call arguments, closing 11 of 137 on `DrawPopUpFrame`, transferred to `DrawPopUpExtra`. All 720 leading-statement orders, pointer/inline filler/int[3]/flattened struct/(void*)0/p=n=0/volatile store are inert; FULL {1,{0,0}} costs 12. Aggregate-init stores are non-aliasing, field-ordered, and the highest-offset field can sink exactly ONE slot past the next pointer load; a middle field cannot be chosen. `WinRect b={0,0,h,h}` reads h twice (no CSE across initializer).

`char name[9]="manBox??"` pins its three-instruction copy at top; a static const struct template and `name=k` makes it a statement, worth 58 and moving first divergence index 3 → 40. A float-constant initializer similarly pins its store; bare declaration plus assignment is placeable (all seven measured leading anchors then identical).

A sub esp,N followed by mov [esp+k],imm stores with NO matching add esp is a local aggregate initializer, not call arguments: `WinRect dst = {0,0,640,480};`. This is the missing frame-reading half of the fable-b example.

Evidence: [D097](DECOMP.md?plain=1#L1334), [D147](DECOMP.md?plain=1#L1853), [D162](DECOMP.md?plain=1#L1971), [D240](DECOMP.md?plain=1#L2596), [D251](DECOMP.md?plain=1#L2685), [D259](DECOMP.md?plain=1#L2735), [D287](DECOMP.md?plain=1#L2957), [D329](DECOMP.md?plain=1#L3240), [D361](DECOMP.md?plain=1#L3455), [D411](DECOMP.md?plain=1#L3848).

<a id="fr07"></a>

### FR07 — Phantom home plus dead spill: distinguish an existing slot from an extra aggregate slot

Use the frame map to choose between two tools. `*(volatile int*)&tx=v` through an EXISTING local produces store-at-death without changing the frame. A block-scope `struct {int x,y;} spill; *(volatile int*)&spill.x=v` reserves an additional 8-byte object, colours its written member onto a just-dead home and leaves an untouched sibling dword. A plain dead store is eliminated; volatile is required and seven volatile-store spellings are identical. Unused locals, no-op address takes and dead arrays vanish rather than supplying ballast.

Evidence: the same two never-read homes / dead mov[esp+N],ebp appeared in `TempleSlide_Update` and four BNV `_Activate` callbacks; the two-member object fixed all four frames. `Carousel_Tick`, 278 → 34 from this alone. One-member struct/bare int gives only the store and one slot; int dims[3] supplies spare size but sits at TOP aggregate pool instead of bottom. Plain pos2.x=sx/spill.x=sx shrinks frame. The alternate natural form is separate pre/post-scroll x locals (`sx2=cfg->ox-Get_XScroll()+sx`); separating y too is worse. A scalar needing TOP position is the .y of a function-level two-int struct with never-read .x, whereas plain int can colour onto x87 fild staging and renumber all homes.

A never-used slot before Pos may instead be the 8-byte alignment hole after a four-byte slot (`LFEntrance_Add` entry-0x14; `SpaceTower_Activate` phantom). Read the complete layout before adding any object. The existing-slot form exposed `StepSchoolCar`'s old 351/351 as two extra flag-memory instructions cancelling two missing projection instructions. An original store immediately overwritten by a later local is direct evidence of temporary lifetime-colouring, not proof of a source dead variable. Lifting a flag store above two escaped stores costs a frame slot, so its observed arithmetic interleave is not recovered by that source move.

Evidence: [D264](DECOMP.md?plain=1#L2778), [D268](DECOMP.md?plain=1#L2803), [D280](DECOMP.md?plain=1#L2912), [D294](DECOMP.md?plain=1#L3002), [D340](DECOMP.md?plain=1#L3305), [D358](DECOMP.md?plain=1#L3437), [D377](DECOMP.md?plain=1#L3573), [D394](DECOMP.md?plain=1#L3721), [D423](DECOMP.md?plain=1#L3940).

<a id="fr08"></a>

### FR08 — Indexed arrays reserve homes that separate scalars do not

An indexed pointer array retains all addressed element homes. `RoadRec* nb[3]` produces sub esp,0x14 with 8 dead bytes and sends another local to a dead parameter slot; three separate pointers give sub esp,0xc and the wrong parameter-slot occupant (50 mismatches). Only the last element's home is observable, so individual source indexes are not recoverable. This does not contradict scalarisation of constant-index/non-escaped wrappers: this array is genuinely indexed. Spilled flags in the measured bodies need separate homes/dead argument slots; wrapping those flags in a struct/array collapses the frame, so that measured negative is not a general ban on aggregate allocation levers. The spilled-flag entry does not name its measured body, so it cannot be used as a body-independent prohibition.

Evidence: [D177](DECOMP.md?plain=1#L2088), [D237](DECOMP.md?plain=1#L2570).

<a id="fr09"></a>

### FR09 — Uninitialised reads may intentionally reuse another value’s home

Reproduce the original's undefined fallback reads rather than inventing a value. `LFTrack_Update` / `LFTrack_Add`: an if/else-if with no final else makes VC6 home the undefined value in a dead parameter slot and emit mov reg,[esp+N]; explicit fallback `(LFRun*)nb`, union read, &nb cast or volatile produces mov reg,reg instead. `LookupTextureName` has TWO uninitialised locals, one per guard, to obtain two homes: dead list and still-live index. That produces shipped `SkipStrings((char*)index,index)` on null list.

For the distinct WORD-width rule and snap-pointer home in `Roads_CalcCursor`, see Types and widths. For `LoadTextFile`'s duplicated undefined-return epilogue, see Block layout.

Evidence: [D026](DECOMP.md?plain=1#L802), [D223](DECOMP.md?plain=1#L2481).

<a id="fr10"></a>

### FR10 — Inline assembly changes frame ordering globally; C reference weight still matters

On `Draw3DPersonModel`, ANY __asm reverses the whole frame-object order, regardless of asm position/count: eight paired compiles reverse A,B,C,D,E to E,D,C,B,A. Without asm the underlying array order is descending BYTE SIZE from ebp down; with asm ascending. Three seven-int structs equal int[21], so element count is not the key. Declaration permutation (all 24) is inert. C references pull objects TOWARD ebp; lea X inside asm carries ZERO weight (delete 4, 8 or all 24, byte-identical).

Block scope is a weight/class effect, not one fixed position: an 84-byte array in ONE block passes two 96-byte arrays; same name in TWO disjoint blocks had looked function-level and caused the old mistaken one-position claim. On the real function, block scope or cutting about 24 references each moves one step and they do not stack. Scalar homes independently descend by reference count, 17,16,16,15,13,12,12,12,11 | 9,8,7 | 4,4,4 | 3,3,3,2. See the frame-profile floor for the limits of these levers.

Separate regime: with `#pragma optimize("",off)` inside an /O2 file, `Castle_Activate`'s home order follows local NAMES (62 → 0), symbol hash buckets assigning ebp-4 first; same bucket is most-recent-declared first. Declaration/first-use/type/scope/register inert unless names collide. Single letters bucket by c mod 16; two letters by (4*c0+c1+6) mod 16 interleaved; no fitted longer-name rule. `scratchpad/castleobj/probe.py name1 name2 ...` measures /FAs homes; this likely explains the older `Castle_Interact` Offset scope effect but remains an inference.

Evidence: [D244](DECOMP.md?plain=1#L2614), [D409](DECOMP.md?plain=1#L3816), [D431](DECOMP.md?plain=1#L4024).

<a id="types-widths"></a>

## Types and widths

| Rule | Question |
| --- | --- |
| [TY01](#ty01) | Read the local width from its store, and the narrowing point from its later use |
| [TY02](#ty02) | An undefined WORD read may require a one-member aggregate |
| [TY03](#ty03) | Keep byte masks, casts, locals, and signedness distinct |
| [TY04](#ty04) | Caller and callee types are local code-generation evidence |
| [TY05](#ty05) | A Pos by value changes scheduling, definitions, and destination symbols |
| [TY06](#ty06) | Widen byte fields before calls when their loads must precede the call |
| [TY07](#ty07) | Packed two-byte objects explain dword-plus-mask reads and high-byte grouping |
| [TY08](#ty08) | Narrow bit operations depend on register versus memory and the destination type |
| [TY09](#ty09) | A widened boolean reader preserves movsx and the original mask order |
| [TY10](#ty10) | Ternary direction and statement shape decide branchless integer code |
| [TY11](#ty11) | Comparison operand order remains visible |
| [TY12](#ty12) | Same-width conversions require distinct objects; union views have a narrower exception |
| [TY13](#ty13) | Recovered layouts and interfaces must survive tempting matching shortcuts |
| [TY14](#ty14) | Use a char loop index for a fresh per-band sign extension |

<a id="ty01"></a>

### TY01 — Read the local width from its store, and the narrowing point from its later use

A loaded u16 need not imply a u16 local: `xor ecx,ecx / mov cx,[map+0x20] / mov DWORD [esp+N],ecx` is an `int` with a FOUR-byte spill; `unsigned short` emits a 2-byte store. `RenderFullMap`: 151 of 1064 mismatches removed by that type. Conversely a `short` local retains sign extension at the USE: `Castle_InitEntranceTrack`'s `cy2 = c1.y + 2`, used again 80 instructions and three calls later, as `int` emits `movsx edi,ax` at definition and loses `movsx ecx,di` (187 of 230 against 231); `short` keeps `mov di,bp` then late `movsx` (197 of 231).

For a dword home read as a signed WORD, keep `int` and cast at the use: `PaintCursorTiles` stores `(h+1)>>1` with `mov dword ptr [esp+0xc],eax`, reloads `movsx edx,word ptr [esp+0x10]`; the register-held counterpart casts as `movsx edi,si`. `short` would store a word; uncast `int` would reload a dword. This transferred `GetTileBounds`'s `short h; short w = (short)(h+h); (short)((w+1)>>1)` spelling to `PaintCursorTiles`, 106/106 first try. The sequence `mov ax,[m] / lea ecx,[eax+eax] / movsx ecx,cx / movsx eax,ax` requires a short RESULT; `int w=h*2` instead gives one `movsx` and an `add`.

A local feeding only 16-bit stores can still occupy full registers without `movsx`: `short` changed temporary allocation and closed `BuildCursorPtr` 6 -> 0 (`add edx,edi` in the home rather than `lea` plus separate load). But `BuildChannelTables`'s `mov di,ax` after a call denotes a narrowed compiler TEMPORARY: naming a u16/short local widens the copy to 32-bit `mov edi,eax`. A textually repeated CSE expression or immediately consumed short-returning call, with all consumers word-wide, preserves the 16-bit copy. The unmatched twin idiom is 0x00422ef8/0x00422f8c: `call __ftol / mov bp,ax / shl ebp,cl / or ebp,eax`.

Evidence: [D013](DECOMP.md?plain=1#L607), [D210](DECOMP.md?plain=1#L2378), [D247](DECOMP.md?plain=1#L2653), [D257](DECOMP.md?plain=1#L2721).

<a id="ty02"></a>

### TY02 — An undefined WORD read may require a one-member aggregate

On `Roads_CalcCursor`, a plain `unsigned short` used only in promoted compares acquires int-wide storage and an undefined `mov esi,dword` read. `struct { unsigned short id; } group;` preserves `mov si,word`; `short`, scope, `else x=x;` and a u16 inline-helper parameter do not. The home is the spilled `snap` pointer in the dead `o` argument slot: the shipped bug reads a pointer's low word as a road-group id.

Evidence: [D234](DECOMP.md?plain=1#L2552).

<a id="ty03"></a>

### TY03 — Keep byte masks, casts, locals, and signedness distinct

An explicit `& 0xff` can narrow AFTER 32-bit arithmetic where `(unsigned char)` propagates width backward. `PaintPathRect` needs `neg dl / sbb edx,edx / add edx,2 / and edx,0xff`; a byte local or use-site cast produces `sbb dl,dl` plus `movzx` (2 wrong at identical length). `step & 0xff`, `(unsigned)step & 0xff`, and an unsigned-short intermediate all reproduce it. Keep `x+y` int-wide if it must become its own IV: `lea esi,[eax+edi] / inc esi` with a fourth callee-saved push; byte-wide recomputes `bl=dl / add bl,al` and is three instructions short.

A switch-produced index whose use is `and eax,0xff` is an `int` cast at subscript use; an unsigned-char local makes the arms byte moves (45 of 65). A call-produced unsigned-char index may round-trip through its home when extension follows `add esp,N`; `int` with use-site casts avoids it. On a call result, `mov byte ptr [esp+N],al` followed by `movsx eax,al` means SIGNED `char`, not `int` or unsigned char (the latter reloads its home). `SchoolCarAccelerate`: an `int` for a u8 field gives `xor ebx,ebx / mov bl`, while `--frame` still narrows to `dec bl`; forwarding `c->f = expr & 0xf; d = (c->f-frame)&0xf` yields byte `and al,0xf`, versus dword `and eax,0xf` for an int temp.

Evidence: [D065](DECOMP.md?plain=1#L1138), [D116](DECOMP.md?plain=1#L1533), [D222](DECOMP.md?plain=1#L2470), [D293](DECOMP.md?plain=1#L2999), [D330](DECOMP.md?plain=1#L3245).

<a id="ty04"></a>

### TY04 — Caller and callee types are local code-generation evidence

Do not align extern types across translation units. `UpdateSidePanelScroll(char step)` sign-extends `bl` at each use; changing the parameter to `int` costs 105 of 121 and five instructions, despite the caller's int extern and identical `__cdecl` ABI. `JungleCruise_StepRoute(unsigned short station)` loads `cx` once from `[esp+0x34]` and compares it four times; int plus casts gives 107/108 with that load wrong. A caller's unsigned-short prototype keeps a dirty upper half (`mov cx,[esi+4] / push ecx`) where int adds extension. `Restaurant2_Tick`: `StartSound(unsigned short)` preserves `mov dx,word ptr [esi+4] / push edx`, four call sites/four instructions and 262 -> 56; int adds `xor edx,edx`. A char extern fed a truncated short gives `mov al,[mem] / push eax` without extension.

`StandardRemoveObject` has FIVE caller-side spellings. `DrivingSchool_Remove` needs unsigned int at one site and unsigned short at another (`mov ax,[mem] / push eax`, one instruction closes it); `BsMermaid_Remove` needs `BPosW` by value. `screencb.c` declares 0x0045f220 as `StandardRemoveObject`, `_W`, `_B`, following `ridecb6.c`'s 0x0041b0d0 precedent. `Pump_Remove`'s fifth prototype, BPosW by value, pushes the packed square's whole home dword unmasked; unsigned int inserts `and edx,0xffff`.

`DrawPathTileOverlay`'s `char edges;` and `extern char PathCornerMask(char,Pos*)`, followed by `(edges & 0xff)` at the index, preserve a byte store to the dead incoming `at` slot (`[esp+0x18]`), whole-dword reload (`[esp+0x1c]`) and push, then a SEPARATE mask after the call. The high bytes are stale pointer bits and the callee reads only `dl`; int local changes the store, int parameter moves the mask before the push. A caller-local unsigned `rand` is also the source of unsigned `div` in the codex-b measurement (`which = rand()%5` before the call, 28i/87B exact), while other units declare rand signed.

Global types are local levers too: `g_path_tile_base` is `unsigned short*` in `sysmisc3.c` for the 16-bit `add dx,[ebx]`, though pathsq.c declares `int*`.

`screen.c` declares callbacks with EMPTY parameter lists while their bodies forward actual arguments. The cdecl ABI permits those caller declarations; the codex-C pass left them unchanged. Empty parameter lists here are not evidence that the functions take no arguments.

Evidence: [D013](DECOMP.md?plain=1#L607), [D018](DECOMP.md?plain=1#L741), [D026](DECOMP.md?plain=1#L802), [D041](DECOMP.md?plain=1#L993), [D069](DECOMP.md?plain=1#L1170), [D126](DECOMP.md?plain=1#L1595), [D133](DECOMP.md?plain=1#L1644), [D138](DECOMP.md?plain=1#L1675), [D284](DECOMP.md?plain=1#L2935), [D290](DECOMP.md?plain=1#L2979).

<a id="ty05"></a>

### TY05 — A Pos by value changes scheduling, definitions, and destination symbols

The same cdecl ABI does not imply the same caller code. `LFRun_Tick`: assigning `t.x`, `t.y` in field order and calling `Step(q,t)` evaluates plain sums left-to-right; `(int,int)` evaluates right-to-left and costs 13 of 123. Read the class global directly at both uses: routing the same Pos through a named `RideDef* def` costs 76. `LFBoat_Draw`: a Pos argument adds one register DEFINITION and advances the scratch rotation, closing the last 17 of 299; all four `(int,int)` operand orders are identical. `Restaurant1_WalkToSeatSpot`: aggregate member `tile.y` becomes the destination SYMBOL (`add edx,ecx`, not `add ecx,edx`), restoring `mov ecx,edx` argument copying, 33 of 56 -> 0 of 57 after ~60 alternatives; the caller's four-int extern stays.

The equivalence holds only at the measured address-taken-struct forwarding site: `RequestRoute` defined `(Pos from,Pos to)` in simcore.c and declared `(int,int,int,int)` in popup.c produces byte-identical calls there, so that call alone cannot recover the prototype.

Evidence: [D078](DECOMP.md?plain=1#L1229), [D089](DECOMP.md?plain=1#L1304), [D148](DECOMP.md?plain=1#L1862), [D231](DECOMP.md?plain=1#L2526).

<a id="ty06"></a>

### TY06 — Widen byte fields before calls when their loads must precede the call

`BoatingSchoolWater_Remove`: `f(st->ax,st->ay,st->bx,st->by)` evaluates right-to-left into pushes; widening into `int ax,ay,bx,by` and reading ax, ay, bx, by first gives the original load order and moves the walker esi -> edi. All 24 orders were measured; only that one is exact.

Evidence: [D236](DECOMP.md?plain=1#L2565).

<a id="ty07"></a>

### TY07 — Packed two-byte objects explain dword-plus-mask reads and high-byte grouping

A BPosW by-value parameter whose address is also passed (`FindRec((MapSquare*)&sq)`) remains in its argument home and permits both aligned `mov edx,[esp+0x30] / and edx,0xff` and misaligned `mov eax,[esp+0x31] / and eax,0xff`. Neither pointer nor unsigned-int parameter gives both halves. The same packed `{u8,u8}` idiom occurs on a BPosW LOCAL.

`key.b.y` and `(unsigned char)(key.w >> 8)` differ: two byte-member reads group and hoist together; the shift form delays the high-byte read past an intervening load, but adds `xor r,r / mov r8,[slot+1]` (one instruction too many in the recorded measurement). Widening need not imply a union: `CastleDummy_Interact`'s short field at +8 widens to `mov r32,dword` when its struct ends at 12 bytes; with fields after y it stays `mov r16,word`.

Evidence: [D042](DECOMP.md?plain=1#L999), [D096](DECOMP.md?plain=1#L1330), [D245](DECOMP.md?plain=1#L2626), [D321](DECOMP.md?plain=1#L3183).

<a id="ty08"></a>

### TY08 — Narrow bit operations depend on register versus memory and the destination type

For an int already loaded into a register, `x &= ~1` narrows to `and al,0xfe`, and `|=0x1000` to `or ch,0x10`; the earlier non-narrowing `&=~` result concerned a direct memory read-modify-write. DWORD globals can narrow too: `|=0x100`/`|=0x200` give `or ah,1`/`or dh,2`. The recorded `test byte ptr [mem],1` at byte +3 of the containing word-field location is a u16 `&0x100`; testing a separate byte adds a load and moves it above pending `add esp`.

`LoadPalette`: the 16-bit STORE's signedness decides mask width. Unsigned-short destination narrows `&~7` to `and edx,0FFF8h`; signed-short destination retains `and edx,-8`; changing the mask constant spelling does nothing. VC6 also propagates edge-known constants into register stores: `mov word [esi+40h],di` in the `r==2` arm is plain `b->f40=2`, because edi is already known 2; it does not imply a `(short)r` source assignment. For RGB565/555, use unsigned-short red, unsigned-char green/blue masked at declaration, and multiplies `(((r & ~7) * 32) | g) * 8 | b`: multiplication widens before masking, whereas `<<5` masks the byte.

Evidence: [D026](DECOMP.md?plain=1#L802), [D068](DECOMP.md?plain=1#L1162), [D097](DECOMP.md?plain=1#L1334), [D138](DECOMP.md?plain=1#L1675), [D323](DECOMP.md?plain=1#L3196).

<a id="ty09"></a>

### TY09 — A widened boolean reader preserves movsx and the original mask order

`Route_IsClosed` (measured 2026-09-03) needs `int s = *(char*)p; if (s & M) return 1; return 0;`: `movsx eax,byte ptr [...] / and eax,M / shr eax,k`. `(x&M)>>k` reassociates to `sar k / and 1`; direct `(f&M)!=0`, ternary and `!!` narrow to `mov al` (~60 variants). Independent codex-e confirmation for `int flags=*(signed char*)&node->flags; if(flags&1) return 1; return 0;`: 4i/11B exact (`movsx / and eax,1`); a direct mask, returning an int-local mask, volatile, alternate integer types and equivalent bit expressions give 4i/10B. No volatile is needed.

Evidence: [D001](DECOMP.md?plain=1#L427), [D219](DECOMP.md?plain=1#L2450).

<a id="ty10"></a>

### TY10 — Ternary direction and statement shape decide branchless integer code

For the rotary-ride byte revolution resets, `rand()%2 ? 4 : 3` and `rand()%2 ? 2 : 1` preserve `setne al` followed by 32-bit `add eax,3`/`inc eax`. `(rand()%2 != 0)+K` adds in AL, one mismatch and one byte each.

A source ternary's FALSE arm names the emitted setcc: `(s<3 ? 0x0a : 0x14)` gives `setge / dec / and 0xfffffff6 / add 0x14`; `(s>=3 ? 0x14 : 0x0a)` gives `setl / and 0xa / add 0xa`. In the recorded byte-return comparison, `if(r&2) return A; return B;` complements in place (`not bl / movsx eax,bl`); `(r&2)?A:B` needs `mov al,bl / not al`, differing by 18 strict and 3 bytes. About 20 operand type/cast forms did not replace this statement-shape lever.

For `MusicThread`'s repeated call guards, `bad=(x!=y); if(bad) f(...,bad);` produces `xor edx,edx / cmp / setne dl / mov eax,edx / cmp eax,<zeroreg> / je`; confirmed thirty times.

Evidence: [D001](DECOMP.md?plain=1#L427), [D004](DECOMP.md?plain=1#L481), [D164](DECOMP.md?plain=1#L1981), [D178](DECOMP.md?plain=1#L2096).

<a id="ty11"></a>

### TY11 — Comparison operand order remains visible

`if(param==arr[i])` versus `if(arr[i]==param)` selects `cmp reg,mem` versus `cmp mem,reg` (same length, one mismatch; third confirmation). For a 16-bit key compare, `cell->owner.w != st->key.w` gives `cmp cx,[ebx]`; swapping the source operands gives `cmp [ebx],cx`.

Evidence: [D058](DECOMP.md?plain=1#L1099), [D062](DECOMP.md?plain=1#L1122).

<a id="ty12"></a>

### TY12 — Same-width conversions require distinct objects; union views have a narrower exception

`RequestRoute`: reading an unsigned/long field into an int store defeats CSE and reloads the field. The conversion barrier requires two differently typed OBJECTS; same-width union members value-number as one lvalue, and two struct-type casts over the same object do too. This is separate from the offset-specific ARRAY-view effect: on `BsWater_DrawSelection`/`JcWater_DrawSelection`, `sq.c[1]=...` prevents unification of a later `sq.b.y` read with a cell load (75 of 107). `sq.c[0]` cannot: element 0 already resolves to `&sq`, the `.b.x` address. That view rematerializes y only; it is not a general union barrier.

Evidence: [D134](DECOMP.md?plain=1#L1652), [D230](DECOMP.md?plain=1#L2523), [D392](DECOMP.md?plain=1#L3707), [D445](DECOMP.md?plain=1#L4105).

<a id="ty13"></a>

### TY13 — Recovered layouts and interfaces must survive tempting matching shortcuts

Keep these observed layouts and semantics, even when a wrong type scores better:

- `SaveIconStateChunk` stores four DWORDs, not `u8[0x10]`, and the INVERSE of flag 0x400. `SaveScriptString` writes u32 length (-1 = NULL), then exactly `len` bytes, NO terminator; its loader allocates `len+1` and adds NUL. savechunks.c's “len+1 bytes” save description is wrong.
- `Raster_SaveState` selects the locked surface for the 16-bit rasteriser; it saves no state, and its restore partner is bare `ret`. rin.c's `UnInit3DPrintList` tears down the shading-ramp cache. Goal and script-event lists share a record type. A FREE track end maps to direction 1 (NORTH), not a sentinel.
- `NewBNVPath`'s 6th argument is a 3-int position, `Vec3`, 12 bytes, not a Pos; the never-written third dword above each seed local is z. Its size was recovered by following control flow through the .rdata jump table with esp tracked.
- `RideDef.qx/qy` are signed chars at +0x24/+0x25. `g_map->tile_h` at +0x18 is UNSIGNED 16-bit; signing it removes an ebx clobber and changes switch merging. `MapConfig.ox/oy` at +0x20/+0x22 are unsigned short, while `Get_XScroll`/`Get_YScroll` return short (`movsx`); that asymmetry controls the delta shape.
- `Person3D`: no padding after local; screen +0x1c, local +0x24, zsprite +0x2c, f30 +0x30, depth/zboost +0x3c. A bogus `pad2c[4]` in joust.c put the last three four bytes high. person3d.c never had that bug; its `Person3D::depth` at +0x54 is a DIFFERENT print-list sort key. `Bloke.saved_speed` is a byte at +0x44; `b->seat` at +0x36 drives the inner jump table.
- `SpinningBarrels_SeatOf`, `SpiderRide_SeatOf`, `PlaneRide_SeatOf` take the RECORD (seat bytes +0x21/+0x1c); `SafariRide_SeatOf` takes the tile. `lpConfig` 0x004bcbf4 is a POINTER, not the map-config struct. `g_spacetower_state`/`g_spacetower_phase` are elements 0 and 3 of a four-pointer per-car seat-matte table; names remain.
- `ClampPopUpToScreen` 0x004718c0 returns int (settled y bound), not the caller's void; its shipped `y<=limit` path stores y but returns limit.
- Two tables supplied as `push OFFSET` are arrays; declaring `void*` scalar tables loads their contents and costs four instructions.

Evidence: [D012](DECOMP.md?plain=1#L596), [D025](DECOMP.md?plain=1#L795), [D077](DECOMP.md?plain=1#L1225), [D255](DECOMP.md?plain=1#L2702), [D285](DECOMP.md?plain=1#L2945), [D299](DECOMP.md?plain=1#L3036), [D312](DECOMP.md?plain=1#L3123), [D317](DECOMP.md?plain=1#L3161), [D330](DECOMP.md?plain=1#L3245), [D380](DECOMP.md?plain=1#L3590), [D408](DECOMP.md?plain=1#L3810), [D412](DECOMP.md?plain=1#L3851).

<a id="ty14"></a>

### TY14 — Use a char loop index for a fresh per-band sign extension

`GeneralStore/Saloon/LegoMedia_DrawOverlay`, 27/22/16 -> 0, need `char i` with char n. Each band becomes `test bl,bl / jle next / lea esi,queue / movsx edi,bl / loop`: byte compare, late per-loop dword trip count, no CSE. An int index hoists and spills one shared sign extension and threads guards. Short/unsigned-char indices, `!=`, and pointer-walking do/while are wrong. Same reported residual: `LegoShop2_DrawOverlay`, `JailCell_DrawOverlay`, `Carousel_Draw`, `Balloonz_Draw`, and mechrides.c residual (b).

Evidence: [D269](DECOMP.md?plain=1#L2807).

<a id="sums-algebra"></a>

## Sums and algebra

| Rule | Question |
| --- | --- |
| [SA01](#sa01) | Choose the add destination by operand class before ordering the chain |
| [SA02](#sa02) | Flat three- and four-term sums canonicalize; the partial-sum barrier is narrower |
| [SA03](#sa03) | Aggregate symbols can preserve the computation before stores and compares |
| [SA04](#sa04) | The struct-return accumulator rule reverses inside a loop |
| [SA05](#sa05) | Whole-struct fetches and copies change operand survival |
| [SA06](#sa06) | Keep named arithmetic when it blocks reassociation |
| [SA07](#sa07) | Use statement barriers only within their measured reassociation context |
| [SA08](#sa08) | Multiplication, shifts, and index representation are distinct objects |
| [SA09](#sa09) | Product rank and cross-expression CSE determine imul operands |
| [SA10](#sa10) | Projection completion and split-shift order control the sum/difference schedule |
| [SA11](#sa11) | Pointer-based half-toward-zero helpers preserve the measured memory pair |

<a id="sa01"></a>

### SA01 — Choose the add destination by operand class before ordering the chain

For the measured commutative add shapes, destination rank is (1) an inline MEMORY reference loaded into the destination, (2) a compiler temporary, (3) a named local. Remaining flattened addends sort by descending definition point. `OctopusCafe_Tick` closed 49 -> 0 with inline memory; `Entrance1_Tick` 0x0042e0a9 is the worked twin: `tbl[b->f3a] + (ty<<8)` emits `mov edx,[eax+ecx*4] / shl ebp,8 / add edx,ebp`. `scratchpad/laneD/scan_adddest.py` found five load-destination/shift-source hits in 1542 exact bodies. The earlier assertion that this inline operand must fold and lose an instruction is withdrawn.

The final measured two-NAMED-local tie-break is EMISSION ORDER, not first/last definition or read order: with read order held constant across four builds, changing definitions flipped the destination in BOTH directions. VC6 copies the difference's left operand; whichever of the paired sum/difference emits SECOND takes the remaining operand's register. Definitions affect register placement and thereby scheduling, not a universal “last-defined wins” rule. The intermediate mechrides last-defined and joust first-read explanations are superseded. At sites already using rank-1 inline memory, this tie-break is untestable.

Evidence for source handles that change the IR class or destination symbol:

- `FreePlayItemAvailable`: `g_x+f(a)` gives `add <result>,<load>`; `int c=f(a); ... g_x+c` gives `mov ecx,[g_x] / add ecx,eax`, three instructions and a byte; commutation is inert.
- `Explorers_TickCustomers`: `ShopTile* key=&r->ride_id; key->b.x` makes the dword field the destination instead of the zero-extended key byte, 115 -> 0; ~60 operand/width/cast/volatile/helper/24-declaration variants were inert.
- `Joust_Draw`: `PrintSprite(spr,screen.ox+off.ox,...)` at five sites gives off's register and hoists the sprite load before `push ebp`, 42 -> 3; off-first puts the sum in screen's register and loads after the push.
- The BNV second stage `sx2=cfg->ox-Get_XScroll()+sx` uses the fresh expression's operand register and lets sx die; reusing the same variable retains its register. Read world x/y into wx/wy BEFORE `GetTileDimensions`.
- An actual `py += ...` preserves py as destination; association, named deltas, declaration swaps and `sy2=py; sy2+=d` all collapse together. Four ways to end the resulting web early also collapse. `v2=v+(M-f())` lets the delta temp win; `v2=v-f(); v2+=M` begins in place and coalesces with v's callee-saved register. ~40 named-delta/array/struct/short/volatile/all-six-order alternatives forward-substitute away.
- Splitting an accumulator before/after a call makes the delta win, forces it callee-saved and leaves eax occupied at the next global load: 5-byte A1 versus 6-byte encoding is evidence of the web split.

Evidence: [D015](DECOMP.md?plain=1#L722), [D200](DECOMP.md?plain=1#L2288), [D252](DECOMP.md?plain=1#L2691), [D262](DECOMP.md?plain=1#L2761), [D270](DECOMP.md?plain=1#L2819), [D333](DECOMP.md?plain=1#L3261), [D367](DECOMP.md?plain=1#L3503), [D385](DECOMP.md?plain=1#L3627), [D426](DECOMP.md?plain=1#L3966), [D429](DECOMP.md?plain=1#L4006).

<a id="sa02"></a>

### SA02 — Flat three- and four-term sums canonicalize; the partial-sum barrier is narrower

Once a commutative sum is FLAT, all 24 orders of four terms and all six orders/parenthesizations of three terms are identical. VC6 forward-substitutes scalar temps, chooses the destination, then sorts addends by descending definition point. The ~200-variant three-term study found a computed shift paired first with the higher-ranked remaining operand (memory above array symbol); its earlier claimed inline-memory folding is superseded by the later add-rank result.

Independent three-term proof: `Draw3DPersonModel` 508/509, `(p->tint<<24)+t+yy`, six orders, both parentheses, `+=`, tint temp, local names/declarations and five frame perturbations all leave the emitted order unchanged. Thus inability to reorder a flat sum is not evidence that it is the wrong source.

The measured barrier applies to a FOUR-term sum: hold its leading PAIR as a partial sum in a non-address-taken two-field aggregate (`Pos t; t.x=origin+ox; b->sx=t.x+ofs.x+scr.x;`). Holding only the operands does not work. Protection ends at the first READ of any member; two sums separated by a store need two aggregates. The second field must hold a real TWO-TERM sum: one field alone, or a one-term second field, is inert. This exact shape closed indices in three functions across two files; applying it to the paired axis is often catastrophic.

Do not apply this barrier to three or two terms. `StepSchoolCar`'s erroneous three-term barrier emitted `lea ecx,[edx+edi] / add ecx,eax` instead of `xor ecx,ecx / mov cx,[..] / add ecx,eax / add ecx,edi`, introducing the scratch phase error. `TempleSlide_Update`'s `Pos t` barrier was likewise a wrong-projection workaround applied to TWO terms; four plain `-=` are better on every metric.

Evidence: [D207](DECOMP.md?plain=1#L2347), [D307](DECOMP.md?plain=1#L3086), [D354](DECOMP.md?plain=1#L3411), [D449](DECOMP.md?plain=1#L4139), [D463](DECOMP.md?plain=1#L4296), [D476](DECOMP.md?plain=1#L4377).

<a id="sa03"></a>

### SA03 — Aggregate symbols can preserve the computation before stores and compares

Use one aggregate when the original computes a pair before either output is stored or tested; the relevant effect is the member's surviving IR symbol, not an automatically forced memory home.

- `WalkPath_Advance`: direct target.x/target.y stores complete x before loading node.y (20 of 60); two ints hoist both parameter loads and grab EBX (63 instructions); one Pos gives x, y, both shifts, both stores, exact. `UpdatePersonPos`: `projected.x=bx-by; projected.y=by+bx` before scaled stores recovers `lea ecx,[ebx+ebp]`; removing the formerly compensating volatile closes the last six.
- Two call-argument sums as `p.ox=a.ox+b.ox; p.oy=a.oy+b.oy; f(s,p.ox,p.oy,...)` become two fresh-register `lea`s (11 -> 0), with per-component operand order following source. Inline argument sums use `add` into an operand.
- A sum-of-squares coordinate pair needs one Pos: y*y in edx, x*x in eax. Two ints mirror it (80 of 84, every spelling and six volatile reads); Pos throughout is 84/84 and both final sum orders work.
- `IsAdjacentPos`: scalars forward-substitute and descending load displacements win (+4 before +0). Pos fields, `int d[2]`, or an address-taken int retain source evaluation and select the SECOND operand's register (`add eax,ecx`, no closing `mov eax,edi`). Volatile reads do not pin that order.
- A constant-indexed `int oa[2]` scalarizes with no frame slot but keeps `tab[row].x/.y` as symbols instead of folding reads into add. Read Y before X and put an object store between reads and uses for the measured may-alias barrier.
- `GetObjectUID`'s reordered inline helper parameters (`Cell(int y,int x)`) create x before y by right-to-left argument evaluation, 163 -> 20; this changes eax/edi/ebp placement and separates test from sar. Textual pointer-field repetition makes a later sum copy-then-add; a Pos pointer in the helper restores `mov eax,[field] / add eax,reg`. Pos for two compared sums computes both before the first compare, whereas scalar temps short-circuit below the first `jne`. `DrawPopUpExtra`: `box.left=icon->x` as an aggregate member keeps it out of a `||` arm, 9 of 120 -> 0; plain int sinks. A four-corner box also preserves `box.right-box.left` as live subtraction where four ints fold both differences to constants.
- Copy-through-a-temporary for loop updates (`t.x=dx+cur.x; t.y=dy+cur.y; cur=t`) consumes loads into dx/dy's registers, worth 9. Direct `cur.x+=dx; cur.y+=dy` uses a fresh callee-saved register; all six operand orders and both +=/= forms are identical.
- One draw-offset Pos versus two ints preserves grouped loads before halving and avoids an ebx/edi swap (16 mismatches). Computing x,y,store,store fills the first x87 conversion latency gap (16 -> 10).
- `SpaceTower_PlaceCar`: shifted coordinates as one struct keep `(x<<8)` and `(y<<8)` separate before the call, 41 -> 0 with y assigned first. Plain ints collapse to `(x+y)<<8` and sink both shifts below imul; keeping the dead x chain live also prevents it, locating the transform after dead-code elimination leaves single-use shifts.

Evidence: [D007](DECOMP.md?plain=1#L519), [D021](DECOMP.md?plain=1#L772), [D022](DECOMP.md?plain=1#L777), [D026](DECOMP.md?plain=1#L802), [D079](DECOMP.md?plain=1#L1237), [D106](DECOMP.md?plain=1#L1474), [D129](DECOMP.md?plain=1#L1621), [D152](DECOMP.md?plain=1#L1889), [D172](DECOMP.md?plain=1#L2056), [D229](DECOMP.md?plain=1#L2514), [D258](DECOMP.md?plain=1#L2726), [D306](DECOMP.md?plain=1#L3079).

<a id="sa04"></a>

### SA04 — The struct-return accumulator rule reverses inside a loop

For a STRAIGHT-LINE adjustment of an 8-byte struct return, accumulate in the returned object: `p.x += (...)<<8` keeps eax/edx as add destinations and preserves y in edx through the x block. `b->target.x=p.x+(...)` in either order instead uses the shifted sum and needs `mov ebp,edx` to save y, one instruction too long. This is the whole `SpaceTower_StepAnim` residual, 19 spellings.

For `Anim3D_OffsetAt`'s LOOP, do the reverse: plain int accumulators, then assign the returned Pos at the END. `r.x+=...` keeps the part cursor in EBX, leaves one scratch register and folds fractional loads into add memory operands (45 versus original 51). Plain ints spill the cursor into a dead argument slot, give EBX+EBP to the inner loop and materialize all four loads, exact first try after ~50 spellings. A memory fold inside a loop can therefore originate in cross-loop register pressure.

Evidence: [D019](DECOMP.md?plain=1#L756), [D059](DECOMP.md?plain=1#L1103).

<a id="sa05"></a>

### SA05 — Whole-struct fetches and copies change operand survival

`o=tbl[car]; o.oy-=rec->car[car].height` yields `mov/mov/store ox/sub eax,[mem]/store oy`; field-by-field fetches load the subtrahend separately and sink a store past the argument push. Both tower draw passes close, 45 -> 0 and 39 -> 0. An 8-byte whole-struct store uses ONE materialized base, while field stores may choose a different base per field. The STRUCT form can place its second store beyond two epilogue pops; a save and matching restore can legitimately use different forms. A 15-byte packed copy must also be a STRUCT ASSIGNMENT: `lea ecx,[edx+0x38]` followed by dword/dword/dword/word/byte; a named sub-struct pointer folds away. A 16-byte class rect with non-sequential loads `+0x3c,+0x44,+0x40,+0x48` is FOUR field assignments pairing each subtraction's operands, not a whole copy. A 12-byte copy pins a source address (`mov ebx,ecx` then `[ebx]`/`[ebx+4]`/`[ebx+8]`) instead of folding the first field into scaled addressing. It decides which field remains registered and reproduced an a/b pool-slot swap between two loops; a multiset diff short by two `mov R,R` instructions is the tell.

For a global-array append, `Rect4* q=&arr[n]` materializes the original address but pointer stores kill escaped-local CSEs (57/119); direct `arr[n].f=v` keeps CSEs but folds the base into each disp32 store and is one instruction short per block. Fill a scalarized local then `arr[n]=t`: both address materialization and all-reads-before-first-store are right, 117/118 first try. Inline `SetRect4(q,...)` still has an opaque q and merely moves the alias kill one block later.

After `dst=kConst`, the FIRST field read can be forwarded from the source and later reads come from dst. The four jetty rect updates must be top,left,right,bottom: other orders cost 16-35; both `*_Create` tails in screencb.c close. `Coaster3D_ResetScene`: after constants assigned to g_tmpl, `g_base=g_tmpl; g_base.m[0]*=0.5f` folds to `mov [m0],0x3f4d3a3f`. One free volatile read at that first field prevents the fold; memcpy and inline `Half(float*)` still fold.

Vertex-fetch shape similarly controls memory folding: `m->verts[tri[1]].x-m->verts[tri[0]].x` with pointer locals declared AFTER loads the first vertex x/y into registers; declaring pointers first folds every subtraction, two instructions short (141 -> 23). Re-test commutation after changing fetch shape: `(z+x)` versus `(x+z)`, previously inert, mattered by ~180 after a whole-struct copy replaced field fetches, changing registers and a reload.

Evidence: [D013](DECOMP.md?plain=1#L607), [D020](DECOMP.md?plain=1#L766), [D026](DECOMP.md?plain=1#L802), [D072](DECOMP.md?plain=1#L1192), [D132](DECOMP.md?plain=1#L1638), [D138](DECOMP.md?plain=1#L1675), [D144](DECOMP.md?plain=1#L1835), [D457](DECOMP.md?plain=1#L4240), [D458](DECOMP.md?plain=1#L4247), [D465](DECOMP.md?plain=1#L4313).

<a id="sa06"></a>

### SA06 — Keep named arithmetic when it blocks reassociation

A named intermediate is observable even when the arithmetic value is unchanged. The recorded `x+=-t*8`, `x-=t*8`, `x+=(-t)<<3` all canonicalize to neg/shl/add; their actual lever is CSE: a store through `r->bloke` kills the pointer expression, so two separate `r->bloke->world.` expressions reload where a named Bloke pointer keeps one. Inline `step+=limit-(base+step)` folds to `step=limit-base`, merges clamps and loses four instructions; `t=base+step; if(t<limit) step+=limit-t` preserves sub/add. `InitTrackTopology`: share ONE `six=6*s` then derive `3*six`, 75 -> 53; separately hoisting both multiples costs 80.

`n-i-1` subtracts variable-first, folds -1 into a -8 displacement and shares one path->n read; `n-1-i` materializes n-1. `LFEntrance_Update`: `a.y=fp1+my; a.x++; a.y-=dy` merges to one store but preserves `(fp1+my)-dy`, even correcting a prologue allocation; every single-expression form canonicalizes. `LFEntrance_Add`: `fx=def->v[0]; fy=def->v[1]; a.x=...; a.y=...; a.x++; a.y-=dy` groups the x defs into add/inc/one store, whereas adjacent `b.x=...; b.x++` folds to `lea [...+1]`.

The opposite case is a needless named short sum: `short sx=p->x+step` changes which widened field/sum gets EAX and shifts four other instructions (14 of 121, same bytes). Sum/width/sharing/narrowing variants were inert; `p->x+=step` followed by a reread closes 0. `x=x+a+b` versus `x+=a+b` itself is inert; naming subexpressions in computation order can change the schedule. A single-def/single-use delta may instead encourage flattening of later -= operations into the delta register; `a-(b-c)`, `a+(c-b)`, cast and `x=a; x-=b-c` all canonicalize in the recorded case.

Evidence: [D040](DECOMP.md?plain=1#L988), [D053](DECOMP.md?plain=1#L1067), [D086](DECOMP.md?plain=1#L1286), [D107](DECOMP.md?plain=1#L1477), [D124](DECOMP.md?plain=1#L1582), [D228](DECOMP.md?plain=1#L2508), [D266](DECOMP.md?plain=1#L2791), [D378](DECOMP.md?plain=1#L3580), [D441](DECOMP.md?plain=1#L4080).

<a id="sa07"></a>

### SA07 — Use statement barriers only within their measured reassociation context

A store between `x-=a; x-=b` can trigger `mov/neg/sub/add` with three registers and one extra instruction. Put the unrelated store before both to retain two memory subtractions: Spider 321 -> 79; all 24 orders of four -= statements showed only same-variable relative order matters.

`DrawPopUpMock`'s `(X+c1)-c2` folds when X+c1 has one consumer. A second consumer before subtraction preserves both adds; after gives add+lea. An empty test (`if(v)`, `if(v<0)`, `if(v!=0)`) costs no instructions and closed 153 -> 0 where casts, |0, ^0, *1, named/const/enum values and inline accessors did not. Later paired y-chain measurements refine the mechanism: the empty if is a BLOCK SPLIT/fold barrier, not a means to extend liveness into allocation; two values tested in the same slot were identical, and position selects among three register outcomes.

Do not use an empty-if insertion as an uncalibrated instrument for BNV tails. Moving a flag/sprite store or pointer cache above `pos.x=sx2*2` can reassociate four subtractions to `sx2+(-screen.ox-h)` (`mov/sar/neg/sub/add`) and change frame 0x3c -> 0x38. Older 330-380 scores thereby measured algebra rather than the claimed hoist. The clean folded-store form `pos.x=(sx2-h/2-screen.ox)*2` is byte-identical to the statement form and resists that confound; the floor survived remeasurement. In the corrected one-name Y spelling `sy+=cfg->oy-Get_YScroll()`, empty if is inert: a separately NAMED ys, not the one-web chain, triggers flattening.

Evidence: [D282](DECOMP.md?plain=1#L2925), [D308](DECOMP.md?plain=1#L3094), [D334](DECOMP.md?plain=1#L3270), [D417](DECOMP.md?plain=1#L3906), [D445](DECOMP.md?plain=1#L4105), [D469](DECOMP.md?plain=1#L4330), [D470](DECOMP.md?plain=1#L4340).

<a id="sa08"></a>

### SA08 — Multiplication, shifts, and index representation are distinct objects

`tw<<=1` emits in-place `shl edi,1`; `tw=tw*2`, `+=`, `tw+tw` and `*=` use fresh-register lea (6 of 161 directly plus 26 from the wrong pairing). For two values from one field, compute doubled `tw=(short)(h*2)` BEFORE `th=h` to get `lea ecx,[eax+eax]`; reversing gives `add eax,eax`. Isolated kernels always lea, so this is pressure/order in the measured body. Scope exception: negating a fresh zero-extended unsigned-short field and multiplying by 2 before a push gives `xor/mov16/neg/shl` in place. `field>>1` is shr; `-field>>1` is neg/sar.

A standalone `x=byte<<8` gets `xor r,r / mov rh,[mem]` (two instructions); `x=byte*256` keeps `xor / mov rl / shl r,8` (three). Inside a larger sum both spellings are identical. This multiplication resolves `OctopusCafe_Tick`'s older supposedly unavoidable instruction-count loss when naming the shift. `x+0x80000000` folds into a near-2^31 lea displacement; bitwise OR does not.

For indexing, `arr[i].x` with unscaled i gives shl/add plus based stores; pre-scaled byte arithmetic `*(int*)((char*)arr+i)` permits an lea fold. A flat int table uses `[eax*4+disp32]` with index scaled by 6; a 24-byte struct array uses scale 3 then `[eax*8+disp32]` and is one instruction short. `(y>>3)*32` blocks reuse of `y&~7`, whereas `((y>>3)<<5)` permits it. A pre-scaled byte index retains a separate shl where array indexing folds a scale; a 2-D 32x32 mark grid gave a 35-instruction block exactly, while a flat index reassociates/folds.

Evidence: [D024](DECOMP.md?plain=1#L790), [D026](DECOMP.md?plain=1#L802), [D080](DECOMP.md?plain=1#L1243), [D145](DECOMP.md?plain=1#L1841), [D149](DECOMP.md?plain=1#L1870), [D212](DECOMP.md?plain=1#L2393), [D369](DECOMP.md?plain=1#L3518), [D386](DECOMP.md?plain=1#L3643), [D483](DECOMP.md?plain=1#L4441).

<a id="sa09"></a>

### SA09 — Product rank and cross-expression CSE determine imul operands

The measured `imul` destination rank differs from the add rank: (1) compiler temporary (operation/call result or a repeated load promoted by CSE), (2) memory reference, (3) register-candidate symbol; two symbols use declaration order (earlier copied, later folded from its home; swapping parameters flips it). `(j-k)*g[1]` or `h(j)*g[1]` fold g; `j*g[1]` or `g[1]*j` load g and multiply by j. Naming a loaded value makes it a symbol. Source operand order, register, same-width casts, identities and identity helpers normalize first. Evidence: `JcBoat_Animate` ~270 variants, `Sub_423480` repeated field read, `MapToPlayfield`'s `imul eax,[esp+8]`.

The exception to inert product commutation is cross-expression CSE: two identically ordered `A*B` sites link. Commuting the OTHER, already-matched site breaks the link and promotes a live loaded value to a rank-1 temporary at the later site, closing the supposedly unreachable `SoftPrint_XBltFast` residual. It is the exact corpus's sole `mov r,reg / imul r,[mem]`, and its register is a value loaded for an earlier product. The handle does not apply to a bare enregistered IV.

Evidence: [D238](DECOMP.md?plain=1#L2574), [D314](DECOMP.md?plain=1#L3137), [D359](DECOMP.md?plain=1#L3441).

<a id="sa10"></a>

### SA10 — Projection completion and split-shift order control the sum/difference schedule

For paired `(a-b)` and `(a+b)`, difference-first commonly yields `mov t,a / add a,b / sub t,b`; sum-first yields lea and one fewer instruction. Track which coordinate is FINISHED first, not merely the written addend order. `TempleSlide_Update`: the earlier two-web X experiment moved 72 -> 56 by product order; the final X-before-Y projection gives `mov ebp,ebx / add ebx,edi / imul ebx,[th] / sub ebp,edi / imul ebp,[tw]`, 29 -> 22 and exact bytes. Sum-first had lea plus compensating mov, one byte too long. Both reads ABOVE the preceding call then break the coupled saved-register tie: 22 before, 327 after, 327 with only one before.

Where shifts are separate (`sx>>=9; sy>>=9`), product order becomes inert and SHIFT order is the lever; independently confirmed in two functions, including `StepSchoolCar`, where only shift order moved an otherwise fixed permutation. `ClampScrollToMap` closed 190/190 from 34% by ordering independent multiplies. `SetPathSquareDistance` emits independent subtractions and subsequent imuls opposite to source; its `a*a+b*b` order is inert. `LFTrack_BuildGeometry`: `dy=th>>2` before `dx=tw>>2` fixes 26 of 161 across six blocks.

A sign-tested `HalfOffset` pair needs FOUR temps (both sums, both halves), allowing both loads/sums before the first sign test. That destroys add flags and preserves test/jge instead of fused jns. Keep this separate from the hand-written pointer-half helper.

Evidence: [D068](DECOMP.md?plain=1#L1162), [D150](DECOMP.md?plain=1#L1875), [D217](DECOMP.md?plain=1#L2427), [D374](DECOMP.md?plain=1#L3552), [D395](DECOMP.md?plain=1#L3731), [D447](DECOMP.md?plain=1#L4125), [D466](DECOMP.md?plain=1#L4319), [D473](DECOMP.md?plain=1#L4356).

<a id="sa11"></a>

### SA11 — Pointer-based half-toward-zero helpers preserve the measured memory pair

`DrivingSchool_Draw` needs `static __inline void Half(int* v)` implementing the original neg/sar/neg negative arm; plain /2 lowers to cdq/sub/sar. The pointer helper gives the pair memory homes and makes both sums accumulate into freshly loaded tile-bounds registers, 26 -> 0. By-value versions, both sum operand orders, four named-sum forms and four free-volatile placements floor at 3-5.

Evidence: [D128](DECOMP.md?plain=1#L1614).

<a id="floats-x87"></a>

## Floats and x87

| Rule | Question |
| --- | --- |
| [FP01](#fp01) | Read x87 operand order with the accumulator exception |
| [FP02](#fp02) | Place float reads and distinct locals where the original needs them live |
| [FP03](#fp03) | Integer-to-float conversion placement determines fild, fisub, and rounding |
| [FP04](#fp04) | Embedded assignments keep a float on the x87 stack |
| [FP05](#fp05) | Parentheses, two-stage locals, and literal precision prevent unwanted FP folding |
| [FP06](#fp06) | Float initializers, assignments, and stack constants are distinct placement handles |
| [FP07](#fp07) | Raw float transfers need their measured types, not universal bit-cast tricks |
| [FP08](#fp08) | Swap floats without an extra temporary; preserve the shipped undefined preheader read |
| [FP09](#fp09) | Count x87 depth and next-use distance before trying spill spellings |
| [FP10](#fp10) | Read explicit x87 calling conventions and control-word operands literally |
| [FP11](#fp11) | A no-code float conversion can move an FP scheduling window |

<a id="fp01"></a>

### FP01 — Read x87 operand order with the accumulator exception

For an ordinary two-term float sum, VC6 fld-loads the SECOND written operand: `(rear+front)*0.5f` loads front first, all six mismatches on `RouteCar_SetPosition`. An accumulator is different: `acc+=step`, `acc=acc+step`, `acc=step+acc` all yield `fld step / fadd acc` on `Route_StepFree`; only a free volatile read of acc makes it the fld operand. Scope this exception to the accumulator, rather than reversing arbitrary operands.

Evidence: [D075](DECOMP.md?plain=1#L1213), [D112](DECOMP.md?plain=1#L1517).

<a id="fp02"></a>

### FP02 — Place float reads and distinct locals where the original needs them live

A named early float read pins its fld early: use-site reading sinks it 20 indices and pre-scales with `shl eax,3 / [eax+base]`; a named local keeps index 7 and `[ecx*8+base]` (22 -> 6). If both arms use the value, read it FIRST IN BOTH arms to obtain one head-merged fld in the `fcomp/fnstsw` gap; `s=out->z` before the if is four instructions early (32).

A SECOND, block-scoped local can supply the IR temporary needed across a call: `Coaster_StationDerivative`'s `{ float brake=-(vv/(d+d)); }` forces vv home and preserves `call / fmul [vv] / add esp,0x28 / fmul [v]`, 49 of 57 -> 57/57. Reusing the v*v local keeps the chain on the x87 stack and flushes cleanup too early; volatile def/use placements and all six final-product orders floor at 56.

`Route_TravelPerTick(route,Route_GetSpeed(route))` writes the x87 result over a pending argument; a named float speed local preserves the original dead-argument spill/reload, 12i/31B. `BuildChannelTables`'s second run needs DISTINCT `ra` versus `rv` plus `ra=*(float volatile*)&rv` for `fsub st(3)`; a plain copy propagates and emits memory fsub, six bytes longer. All eight subsets of three volatile copies were tried; all three preserve the original byte length. A free volatile read of a memory-homed loop variable at a multiply also freed a callee-saved register for a global (86 -> 46, correct instruction count).

Evidence: [D001](DECOMP.md?plain=1#L427), [D013](DECOMP.md?plain=1#L607), [D049](DECOMP.md?plain=1#L1040), [D081](DECOMP.md?plain=1#L1251), [D114](DECOMP.md?plain=1#L1524).

<a id="fp03"></a>

### FP03 — Integer-to-float conversion placement determines fild, fisub, and rounding

`((float)now-rt->started)*K` uses fild/fisub with an integer memory subtrahend; `(float)(now-started)` performs integer sub then one fild. Two explicit float locals prevent `(float)a-(float)b` from collapsing to fisub and retain fild/fild/fsubp (`Coaster3D_SetupView`, the entire instruction-count difference).

`Raster_SubmitPoly` uses three distinct explicit x87 conversions: `fild x / fstp x` converts int to float IN PLACE; `fild x / fmul k / fistp x` scales and rounds back to the same slot; `fld f / fistp i` moves float to a separate int. They are not interchangeable C casts: correct forms are worth 24 bytes; a float temp costs two instructions at each relevant site.

Unsigned -> float conversion can use `fild qword` with zero high dword (BuildChannelTables), versus signed `fild dword`. Put its loop-invariant `m=max` unsigned->double conversion INSIDE the loop when allocation must occur after preheader ALU ops: hoisting then shares the u64 zero high dword with i=0 after cl's shift count dies, letting zero take ecx and leaving eax for shift scratch. Writing it before the loop allocates zero while cl is still live. This is allocation before scheduling, not a scheduling-boundary trick.

On signed integer counts, a bare `sar eax,1` is `>>1`; `/2` adds cdq/sub (fable-d's third independent instance).

Evidence: [D013](DECOMP.md?plain=1#L607), [D101](DECOMP.md?plain=1#L1453), [D103](DECOMP.md?plain=1#L1463), [D113](DECOMP.md?plain=1#L1521), [D248](DECOMP.md?plain=1#L2663).

<a id="fp04"></a>

### FP04 — Embedded assignments keep a float on the x87 stack

`c=pa-(m=e)*a` stores m with `fst` (store and keep), rather than fstp followed by fld. Likewise `if((nz=a+b)>K)` gives fst+fcomp; separate assignment and condition give fstp+fld+fcomp.

Evidence: [D075](DECOMP.md?plain=1#L1213), [D093](DECOMP.md?plain=1#L1321).

<a id="fp05"></a>

### FP05 — Parentheses, two-stage locals, and literal precision prevent unwanted FP folding

`(int)x * 0.2 * 5.0f` combines `0.2 * 5.0` to exactly 1.0 and emits neither multiply; `((int)x * 0.2) * 5.0f` retains both (`DrawSupportShadow`, 94 -> 104). Keep the original bug: that cast truncates whole units instead of snapping to a 5-unit grid. A double intermediate pools the second constant as qword; float gives `fmul dword ptr [5.0f]` and changes the later fld choice (106 versus 111/111).

`(t-2.0f)*0.5f*120.0f` folds to *60.0f; a two-stage float local preserves both multiplies (worth 2 at two sites) and selects fiadd instead of fild+faddp. For a float ALREADY on the x87 stack, *2 can become `fadd st(0),st(0)`: `2.0f*(E-pe)/m` gives fsub/fadd/fdiv, and `d+d` on a named call result gives fmul/fadd/fdivr.

Literal precision is evidence: `RoutePhys_EvaluateDerivative`'s `<0.05` has no f suffix and produces `fcomp qword ptr [0x004ab408]`, while two other thresholds in the body are dword. `fcomp / test ah,1 / je` guards the LESS-THAN arm, not >= (one mismatch until corrected). `1.175494351e-38f` is FLT_MIN, 0x00800000. `BuildChannelTables`'s exact single-precision literals are `0.032258064f` (1/31) and `0.03125f` (1/32).

Evidence: [D013](DECOMP.md?plain=1#L607), [D074](DECOMP.md?plain=1#L1205), [D095](DECOMP.md?plain=1#L1326).

<a id="fp06"></a>

### FP06 — Float initializers, assignments, and stack constants are distinct placement handles

`Raster_SubmitPoly` requires a `volatile float k=65536.0f` LOCAL for `mov [ebp-0x40],0x47800000`; plain float propagates to .rdata. The original reloads each iteration, so the memory access is free; restoring that store makes byte length exact, 740 -> 743.

`Route_AdvanceTrain`: `float acc=0.0f` at IR start uses a real frame slot; declaration plus assignment AFTER the owner load puts acc in the dead parameter slot, frees `[ebp-4]` for the other float and closes 16 -> 0. One statement earlier does neither. A separate float-constant placement study likewise found the initializer pins the store early, while a split declaration/assignment makes all seven tested leading anchor points identical.

Keep the measured constant-store grouping distinct from pair reversal: `Mat3_ToMat4`'s `m[11]=m[7]=m[3]=0.0f` matches only 11,7,3. All six permutations were tested; 3,7,11 gives 28 of 30, the other orders 29 of 30. With three adjacent array constant stores the emitted order follows source; the reversed-address-store observation concerns a pair.

Evidence: [D013](DECOMP.md?plain=1#L607), [D102](DECOMP.md?plain=1#L1458), [D111](DECOMP.md?plain=1#L1511), [D411](DECOMP.md?plain=1#L3848).

<a id="fp07"></a>

### FP07 — Raw float transfers need their measured types, not universal bit-cast tricks

`PositionRouteCars` reads the same float argument separately for a field store (`mov ecx,[esp+a]`) and a push (`mov edx,[esp+a]`); int CSEs them. Making argument, field, callee parameter and out-param float closed 62 mismatches. Conversely `Route_StepFree` snapshots handed to a float callee must be raw DWORD int locals with caller-side int prototypes: float locals home, put the counter in edi and move 17 instructions; ints retain edi/ebx across the call and spill counters. Its context +0 is float: zeroing as int merges with i=0 into a zero register, whereas `0.0f` keeps immediate stores and `test eax,eax` (worth 3 in schoolcar5.c).

An ordinary float member COPY alone emits integer movs; float member versus int through cast is byte-identical there, so that isolated site cannot recover its type. Do not generalize PositionRouteCars' repeated-argument behavior to every float copy.

Evidence: [D076](DECOMP.md?plain=1#L1218), [D220](DECOMP.md?plain=1#L2457), [D460](DECOMP.md?plain=1#L4260).

<a id="fp08"></a>

### FP08 — Swap floats without an extra temporary; preserve the shipped undefined preheader read

For the measured two-value swap, `out->z=out->x; out->x=-s` is exact with no temp. A float temp gets a home and spill/copy; an int-bits temp CSEs with the sign test and changes `test dword ptr [mem],K` into mov plus register test.

A separate recorded loop preheader intentionally leaves `gap` uninitialized: initializing `gap=z` makes fld one instruction early and exit fstp two late; the uninitialized spelling is exact and the || guard makes the bad read unreachable. Reproduce that original bug rather than supplying an invented initialization.

Evidence: [D092](DECOMP.md?plain=1#L1317), [D115](DECOMP.md?plain=1#L1529).

<a id="fp09"></a>

### FP09 — Count x87 depth and next-use distance before trying spill spellings

`AnimApplyPart`'s x87 allocator spills by FURTHEST NEXT USE. Its second interpolation fills the stack (kept conversion plus five more plus working); no operand can name deeper than st(7), so retaining one value forces four siblings to memory. When the reconstruction has stack operands and the original memory operands, count depth first.

The probe with two of eight rectangle floats volatile changed strict 182 -> 144 and bad regions 73 -> 38: leaving Y-group filds below the divisor spills X, as the original; hoisting them spills Y. This is a diagnosis, not an accepted implementation, because the variant adds a frame slot and uses the wrong subtraction. The ten fild operand displacements independently recover source conversion order; an alternative order scored better while growing the frame and is known-wrong source.

Evidence: [D201](DECOMP.md?plain=1#L2296), [D427](DECOMP.md?plain=1#L3991), [D474](DECOMP.md?plain=1#L4366).

<a id="fp10"></a>

### FP10 — Read explicit x87 calling conventions and control-word operands literally

`VecMath_ReciprocalSqrt` calls a target consuming and returning ST(0), then rounds via its float argument home: explicit inline assembly gives 8i/20B, audit [OK]. `Raster_RestoreFloatMode` uses `fldcw control`, 5i/8B with EBP frame, audit [OK]; its saved control word is passed BY VALUE (`fldcw [ebp+8]`), even though schoolcar.c's placeholder declares void*. There is no pointed-to-word dereference.

Evidence: [D007](DECOMP.md?plain=1#L519).

<a id="fp11"></a>

### FP11 — A no-code float conversion can move an FP scheduling window

`SetBlokePositionFromBNV` (~81 tuples, ~120 variants) showed fixed-size Pentium scheduling windows counted from FUNCTION START across calls. A dependency-free root (zero, parameter load, global store) moves to its window's top, after a prior flag writer where needed; in the next window it fills the next stall after fsqrt/fdivr or between fmul/faddp. Thus i=0's position in an FP stream depends on earlier tuple count, not just the statement's location. `(float)(x*y)` survives as a no-code conversion tuple and moves the boundary; `*=inv` and `=x*inv` do not. `add esp,N` is itself low-priority/movable. This is a scheduling handle; it does not establish a general integer-cast or x87-allocation handle.

Evidence: [D246](DECOMP.md?plain=1#L2636).

<a id="records-lists"></a>

## Records and lists

| Rule | Question |
| --- | --- |
| [RC01](#rc01) | Match the packed-key comparison to the search or tick context |
| [RC02](#rc02) | Test the list-head global separately from the walking copy |
| [RC03](#rc03) | Use the head global itself as the destructive list cursor |
| [RC04](#rc04) | Keep the search success body inside the loop and failure in its else |
| [RC05](#rc05) | Reuse verified record unlink and raw-record save families |
| [RC06](#rc06) | Use a separate cursor for ordinal lookup |
| [RC07](#rc07) | Cache an aliased byte before either record store |
| [RC08](#rc08) | Use a subobject memset to separate the zero fill from the surrounding zero web |
| [RC09](#rc09) | A link assignment can schedule the head load without changing store order |
| [RC10](#rc10) | Preserve the recovered UI timer quirk |
| [RC11](#rc11) | Read the coaster solver and boundary mechanics from their operations |

<a id="rc01"></a>

### RC01 — Match the packed-key comparison to the search or tick context

Use `#pragma intrinsic(memcmp)` and `memcmp(&rec->tile, tile, 2) == 0` when the original materialises the first key address before its word comparison. Use scalar `rec->tile.key == rider->tile.key` at the rotary `StepMachine` tick sites that have no such address temporary. The earlier blanket memcmp prescription was too broad: match the observed context.

Evidence: `JailCell_FindRecord`, `Carousel_FindRec`, `Balloonz_FindRec` have their key at +4 and the characteristic `lea edx,[eax+4]`. Five scope-E searches close at 16i/40B each: expansion after invariant hoisting reloads the record key into dx and compares the query through memory at both original sites, with no volatile. `SpaceTower_Interact` closes 250 -> 0 with `memcmp(t, sq, 2)`. In `SpiderRide_StepMachine`, `PlaneRide_StepMachine`, `SafariRide_StepMachine`, `SpinningBarrels_StepMachine`, however, memcmp adds a `lea` of `rider+0xc`, causing 33/33/39/32 strict mismatches; scalar equality closes all four and preserves the reload sites without volatile. [Expanded evidence](lanes/codex-e.md#measured-ccodegen-findings).

Evidence: [D001](DECOMP.md?plain=1#L427), [D007](DECOMP.md?plain=1#L519), [D271](DECOMP.md?plain=1#L2827), [D283](DECOMP.md?plain=1#L2931).

<a id="rc02"></a>

### RC02 — Test the list-head global separately from the walking copy

`run = g_head; if (g_head == 0) return 0; while (run)` keeps the head guard from proving the cursor non-null. `LFTrack_FindPiece` then loads eax before `push esi/edi` and emits `mov esi,eax`; testing `run` drops one instruction. In `AddOpenNode`, direct `g_open_head` reads at all three uses retain the redundant `mov eax,edi / test eax,eax` peel while keeping one CSE register; a local `head` drops both (eleven shapes). The matching knob is which object is tested, not an extra redundant test.

Evidence: [D032](DECOMP.md?plain=1#L936).

<a id="rc03"></a>

### RC03 — Use the head global itself as the destructive list cursor

`while (g_head) { next = g_head->next; free(g_head); g_head = next; }` lets VC6 forward the just-stored head into the loop test through eax, producing the 5-byte accumulator store `mov [imm32],eax`. A local cursor with `g_head = p` stores from another register in 6 bytes.

Evidence: approximately 20 loop spellings stayed one byte long before the global-cursor form matched. `sub_4828f0`: `while (g) { next = *(void**)g; MemFree(g); g = next; }` gives eax; local-p while/do/for walks, chained assignment, casts and inline helpers keep esi (~35 inert variants). Historical same-shape sites: 0x4054d5, 0x419f8a, 0x419fc5, 0x434ee7, 0x434f22, 0x48117b; their unmatched status is the source checkpoint's, not a fresh audit.

Evidence: [D085](DECOMP.md?plain=1#L1281), [D232](DECOMP.md?plain=1#L2532).

<a id="rc04"></a>

### RC04 — Keep the search success body inside the loop and failure in its else

For the measured list-search shape, success inside the loop threads away the post-loop null test; pushes inside the found path leave a lone two-byte `ret` with no pops for exhaustion. For `UnlinkGardenerOrder`, `if (p) { unlink } else { fail; }` keeps one failure block. The guard `if (!p) { fail; return; }` lets the exhausted edge prove null and duplicates the ten-instruction failure block. These are complementary search-shape measurements; inspect both rather than assuming an early return preserves layout.

Evidence: [D013](DECOMP.md?plain=1#L607), [D097](DECOMP.md?plain=1#L1334).

<a id="rc05"></a>

### RC05 — Reuse verified record unlink and raw-record save families

**The record-unlink family is one source compiled EIGHT times**, not six:
  GOLD RUSH and EARTH SLIDE share it with the six mechanical rides. Identical
  index for index after substituting the head global and link offset
  (+0x04/+0x10/+0x2c/+0x0c), joust.c's volatile link-walk and the null-head
  bug included. And **a `+0xbc` save callback is one source across four
  classes** (only the head global and record size differ) — it is
  `EarthSlide_Save` with the queue pass deleted. The save format: a chain of
  `int 1` + one RAW record image, closed by `int 0`; the record's own `next`
  goes into the file and the loader overwrites it.

Evidence: [D044](DECOMP.md?plain=1#L1011).

<a id="rc06"></a>

### RC06 — Use a separate cursor for ordinal lookup

A separate `cursor` local closes both ordinal lookups from 13i/29B with 9 mismatches to 11i/22B exact; signed versus unsigned index was inert. The lane records no chain-end check for these lookups: saving an absent animation returns the chain length. Do not add a bounds check absent from the original. [Expanded evidence](lanes/codex-c.md).

Evidence: [D018](DECOMP.md?plain=1#L741).

<a id="rc07"></a>

### RC07 — Cache an aliased byte before either record store

Read `lls->frames` into one byte local BEFORE either destination-record store. In `Copters_InitRecord`, across five cars the target is one AL load, store, `dec al`, store. Reloading the field after the first store crosses an alias kill and caused 97 mismatches (truncated 427B); the named-local form is 118i/426B exact. [Expanded evidence](lanes/codex-e.md#measured-ccodegen-findings).

Evidence: [D001](DECOMP.md?plain=1#L427).

<a id="rc08"></a>

### RC08 — Use a subobject memset to separate the zero fill from the surrounding zero web

An intrinsic `memset` introduces its own destination-address temporary and zero-fill node. Choose whole-object versus subobject fill and source position from the original; it can split zero webs even when scheduling later moves the actual stores.

- `Balloonz_NewRecord`: `memset(&rec->cars, 0, 6)` gives `lea eax,[rec+0xd] / xor ecx,ecx / mov [eax],ecx / mov [eax+4],cx`, beside `xor ebx,ebx`. Six byte stores add 2 instructions; copying a static-const zero adds 1 `.rdata` load; a zeroed local spills.
- `ResetNarrationStreamState`: three plain zero stores BEFORE the memset let the fill hoist but materialise `xor edx,edx`, and all nine stores use edx. Memset first forwards eax into 5-byte `mov moffs32,eax` stores. Nine spellings, one exact. The historical entry reports both **63 B against 73** and **11 missing bytes**; those totals differ by 10, so the 11-byte attribution is inconsistent. The contemporaneous [savemisc2.c note](../LEGOLAND/savemisc2.c#L54) independently states 63 versus 73; use those totals (a 10-byte difference), while preserving the historical 11-byte claim as a transcription conflict, not a measured third baseline.
- In the `ctx.sub` measurement, field-by-field clears combine with three `push 0`s, two zero arguments and a rect field into hoisted edi, force `push ebx`, and turn `test eax,eax` into `cmp eax,edi`. `memset(&ctx.sub, 0, sizeof ctx.sub)` changes 64% -> 79% and matches the prologue.
- `JungleCruise_Add`: memset on the LAST group of clears is worth 87 of 102; an earlier group is inert. Its destination temp occupies ecx and stops an argument hoisting 17 instructions early. The destination materialisation `lea base,[reg+disp]` was observed down to 4 bytes; the pressure from this temp explains the gain.
- For the whole-zero-record-plus-sentinel shape, `memset(&term, 0, sizeof(term)); term.next = (ScriptEvent*)-1;` produces `rep stosd` followed by the lone nonzero store after the pushes; field-by-field zeroing does not. A nonzero byte fills the whole dword: `memset(p, 0xf1, n)` emits `mov eax,0F1F1F1F1h / rep stosd` (first nonzero memset constant recorded).

Evidence: [D008](DECOMP.md?plain=1#L569), [D039](DECOMP.md?plain=1#L982), [D097](DECOMP.md?plain=1#L1334), [D138](DECOMP.md?plain=1#L1675), [D155](DECOMP.md?plain=1#L1913), [D350](DECOMP.md?plain=1#L3383), [D383](DECOMP.md?plain=1#L3610).

<a id="rc09"></a>

### RC09 — A link assignment can schedule the head load without changing store order

The order of `p->next = head;` against a neighbouring field store determines where the head LOAD is emitted; the stores remain in compiler order either way. Measured at two allocation sites. Inspect load order before trying to force store order.

Evidence: [D138](DECOMP.md?plain=1#L1675).

<a id="rc10"></a>

### RC10 — Preserve the recovered UI timer quirk

Five UI families confirmed as ONE source each (`Advert*Input` x3,
  `EnqueueStep*Event`, `*ChildrenBarInput`, `PlayReport*`, `*HelpExpired`),
  and an original quirk worth the runtime knowing: `ObjectHelpExpired`'s
  timer arm is `GetGameTimer() - g_advisor_last < 0` against a stamp set to
  NOW every frame, so object help is dropped on the first frame after the
  bubble closes.

Evidence: [D017](DECOMP.md?plain=1#L734).

<a id="rc11"></a>

### RC11 — Read the coaster solver and boundary mechanics from their operations

Mechanics worth knowing across the coaster: the route's physics object is an
  **RK4 solver descriptor** (nodes 0, 1/2, 1/2, 1; weights 1/6, 1/3, 1/3, 1/6)
  over a car-shaped state vector; a piece boundary is landed by BISECTION
  (~16 probes, each restoring the whole train); `TrackJoint`'s +0x00 is a
  four-way DIRECTION bit, not a height (coaster.c's name corrected in
  `coaster5.c`, offsets untouched); and `if (i == 0x1d) i = 0x1d;` in
  `FastRSqrt_InitTables` is a breakpoint hook the developers left in.

Evidence: [D051](DECOMP.md?plain=1#L1053).

<a id="calls-cleanup"></a>

## Calls and cleanup

| Rule | Question |
| --- | --- |
| [CC01](#cc01) | Name nested call results when evaluation and cleanup must separate |
| [CC02](#cc02) | Separate source blocks explain split add esp; textual tail copies preserve deferral |
| [CC03](#cc03) | Plain straight-line calls can share one deferred cleanup |
| [CC04](#cc04) | Two textual calls preserve distinct argument pushes and sometimes cleanup |
| [CC05](#cc05) | Inline helpers evaluate real argument expressions first, with forward-substitution limits |
| [CC06](#cc06) | By-value aggregate field assignment order sets argument register rotation |
| [CC07](#cc07) | An unnamed call-result pointer changes by-value copying |
| [CC08](#cc08) | Name a call result to restore a value before testing success |
| [CC09](#cc09) | A null guard plus returned Win32 result splits returns |
| [CC10](#cc10) | An IAT load hoisted around a loop usually needs no explicit cache |
| [CC11](#cc11) | Literal visibility and per-body pragmas choose intrinsic versus called copying |

<a id="cc01"></a>

### CC01 — Name nested call results when evaluation and cleanup must separate

A nested call's argument evaluation can determine the whole allocation. `Copters_Activate` 213 + ESCAPES -> 0: `f(g(x)[i],a,b,c)` pushes simple arguments before g so a/b never cross a call; `i=g(x); f(tab[i],a,b,c)` calls g first and keeps a/b in callee-saved registers, spilling cursor/ObjDef, restoring a rotated-loop entry and stopping unwanted tail duplication. Use the SAME i the other cases use.

Likewise `f(g(a),h(b)->field)` evaluates the field before the other call and splits cleanups; separate handle statements keep the pointer in ebx across the second call and allow one `add esp,44h`. `JcMonkeyFish_GetDrawDesc` confirms the argument direction: `LLSSetFrame(GetLLSForSprite(g),lls->frame)` reads frame first and splits `add esp,4 / add esp,8`; named call result first merges `add esp,0xc`, worth 25. A nested call passed straight to COM (`SetVolume(obj,VolumeFromDistSq(d2))`) evaluates obj/vtable first, spills the vtable and adds a fourth push, 90 instructions for an 84-instruction body; naming the result removes that pressure. `which=rand()%5` before its consumer restores 28i/87B in the recorded caller by changing argument/cleanup work around rand. These are evaluation-order alternatives: choose the original's shape, not a blanket prohibition on nesting.

Evidence: [D026](DECOMP.md?plain=1#L802), [D130](DECOMP.md?plain=1#L1625), [D135](DECOMP.md?plain=1#L1658), [D249](DECOMP.md?plain=1#L2672).

<a id="cc02"></a>

### CC02 — Separate source blocks explain split add esp; textual tail copies preserve deferral

Pending cdecl cleanup cannot pass an IR join with incompatible incoming stack depth. `SchoolCarManoeuvreD`'s one `add esp,0x24` cleans three calls, one in a shared tail. A source if/else plus shared tail flushes `add esp,0x18` in each arm (88 of 116); duplicating the WHOLE tail into both arms and letting later cross-jumping merge it gives 113/113 first try. A goto into a sibling case's tail introduces an empty-stack predecessor early and kills deferral; textual copies can merge post-codegen with identical pending depths and preserve one `add esp,N+M` even across jmp. Thus the broad earlier no-branch-join wording means the source/IR depth constraint, not a ban on cleanup covering machine-code joins.

The inverse is required in `Copters_StepMachine`: keep the final 3D-update common tail AFTER the zero-timer join. Written inside the guard it merges to `add esp,0xc`; after the join VC6 duplicates the final call while retaining `add esp,4` then `add esp,8`: 18 -> 0, 295 -> 296 bytes (115i/296B exact). `UpdateGoalHelpText`'s cloned tails similarly keep split `add esp,8 / add esp,4` because calls occupied separate source blocks.

Evidence: [D001](DECOMP.md?plain=1#L427), [D139](DECOMP.md?plain=1#L1804), [D260](DECOMP.md?plain=1#L2741), [D305](DECOMP.md?plain=1#L3073).

<a id="cc03"></a>

### CC03 — Plain straight-line calls can share one deferred cleanup

When calls land results in locals/globals and no call consumes another's result directly as an argument, VC6 can defer all cleanups. Evidence: four `JcWater_FindAt` results in locals -> one `add esp,0x20`; `Entrance1_Create` six calls -> `add esp,0x30`; guarded blocks in the same measured file span `0x14`, `0x1c` and `0x114` (including a 256-byte buffer). `StartFreePlayPark`'s nine cdecl argument pushes over nine calls merge at epilogue `add esp,0x58` = 0x34 frame + 0x24 arguments. Its local is `[esp]` at one call and `[esp+8]` at the next, so resolve push depth before inventing two locals. Expanded lane evidence: `InitScreen8` lists six calls, `LoadSprite`, `LoadSpriteIcon`, `GetString`, `LoadSpriteIcon`, `GetString`, `LoadSprite` (8+20+4+20+4+8=0x40), with one cleanup; the lane's introductory 'Four calls' count is inconsistent with that explicit six-call list.

Evidence: [D013](DECOMP.md?plain=1#L607), [D016](DECOMP.md?plain=1#L727), [D041](DECOMP.md?plain=1#L993), [D138](DECOMP.md?plain=1#L1675).

<a id="cc04"></a>

### CC04 — Two textual calls preserve distinct argument pushes and sometimes cleanup

`if(a<K) g=f(K); else g=f(a);` cross-jumps from call onward, leaving `push K / jmp / push eax / call`. `f(a<K?K:a)` instead gives `mov eax,K / push eax / call`. Measured twice: `ReadNarrationWaveHeader` and thirty-fold `MusicThread` macro code. In the header reader the ternary also merges malloc's `add esp,4` with the read cleanup (`add esp,0x10` versus `add esp,4` then `add esp,0xc`), five instructions restored by splitting arms. Other measured two-call arms retain their OWN `add esp,4`, a tell that a ternary-argument reconstruction is wrong.

An if/else whose arms start calls sharing a constant argument may HEAD-MERGE its push into the test: `_read(fd,X,4)` gives `mov eax,[esp+8] / push 4 / cmp eax,'data' / je`. For a three-way colour choice around a by-value rect, THREE textual calls yield TWO rect argument blocks (edx versus ecx) and share only `push font / push text / call`; a single colour variable yields one shared block. Distinct scratch blocks reveal source call copies even when final calls merge.

Evidence: [D004](DECOMP.md?plain=1#L481), [D013](DECOMP.md?plain=1#L607), [D097](DECOMP.md?plain=1#L1334), [D125](DECOMP.md?plain=1#L1589).

<a id="cc05"></a>

### CC05 — Inline helpers evaluate real argument expressions first, with forward-substitution limits

A `static __inline` helper evaluates genuine argument expressions into temporaries before its body, enabling schedules plain statements cannot express. `H(&a,&b->f,x)` with `{a->m=K; b->f.n=x;}` gives `[ld x; lea &b->f; st a->m; st b->f.n]`; comma/LHS-comma/macros normalize to ordinary statement order. This helper alone solves `Joust_Draw`'s disputed 3 instructions but costs 63 elsewhere; a two-level rider guard caching bloke between levels restores the key's scratch web and closes 3 -> 0. Without the intervening cache the guard is inert. A two-argument helper also permits an independent lea and second global load, seven exact instructions where ~110 permutations failed; its FIRST param+global sum takes the parameter temporary's destination, the SECOND the global's (both directions measured).

Inline arithmetic can run before the helper guard: `FindInBounds(x,y-5)` places `add eax,-5` ahead of x>=0, unlike a separately named offset. The multi-argument order matters: the measured `CellAt(dx+x,dy+y)` uses bare js for the guard; writing `t.x=...; t.y=...; CellAt(t.x,t.y)` places another flags writer between X and guard, restoring `test/jl`, 52/76 -> 78/78. The original's re-test is evidence for separate statements at this site. The source note also describes evaluation as RIGHT to LEFT while calling Y the last sum and attributing its flags to x>=0; that explanatory wording is internally inconsistent. Preserve the measured js versus test/jl result without asserting both descriptions of the sum order.

Nested lookup position also preserves scratch rotation across expansions: `Stop(GetLLSForLayer(g_layers,v->layer_a))` gives `eax,ecx | edx,eax | ecx,edx` over three consecutive inline expansions; a helper taking the layer number and reading the global internally gets the first pair right but restarts at eax in the second and third. Move the lookup into the argument only where that advancing phase is required.

Boundary: plain memory-reference arguments can be forward-substituted into the helper, creating NO scheduling/alias barrier (two functions measured). An inline call itself is not a scheduler-region boundary; do not treat every helper invocation as a barrier.

Evidence: [D138](DECOMP.md?plain=1#L1675), [D165](DECOMP.md?plain=1#L1985), [D166](DECOMP.md?plain=1#L1990), [D297](DECOMP.md?plain=1#L3022), [D324](DECOMP.md?plain=1#L3205), [D404](DECOMP.md?plain=1#L3789), [D428](DECOMP.md?plain=1#L4000).

<a id="cc06"></a>

### CC06 — By-value aggregate field assignment order sets argument register rotation

For a WinRect passed by value, stores always follow field-offset order through `mov edi,esp`, but the values take the eax→ecx→edx→esi ring in SOURCE assignment order. `PrintScreenMode8`: natural left/top/right/bottom is one phase wrong at all four sites; top/bottom/left/right moves 118/149 -> 149/149 with no other change. `PrintReportLine` confirms LIVE values, not just constants: 13 of 43 at identical bytes and register-blind 0 -> exact by reorder alone. See the types/sums rule for the separate Pos-by-value versus two-scalar mechanisms; this rule concerns assignment order within one by-value aggregate.

Evidence: [D013](DECOMP.md?plain=1#L607), [D154](DECOMP.md?plain=1#L1904).

<a id="cc07"></a>

### CC07 — An unnamed call-result pointer changes by-value copying

`BuildObject` needs `f(door,*g())` with f(Pos,Pos): the unnamed call-result pointer interleaves `[eax]`/`[eax+4]` loads with pushes. `Pos* p=g(); f(door,*p)` or four-int form hoists both loads before the first push. This was 8 -> 0 after ~300 variants; changing prototype alone is inert (same ABI). The unnamed-pointer copy is the lever, and the call sites support `RequestRoute(Pos from,Pos to)` from both sides.

Evidence: [D273](DECOMP.md?plain=1#L2845).

<a id="cc08"></a>

### CC08 — Name a call result to restore a value before testing success

`ok=SaveGameWrite(...); ev->time+=g_now; if(!ok) return 0;` schedules the restore between call and `test eax,eax`; `if(!SaveGameWrite(...))` puts the add after the branch. The temporary preserves the observed restore-then-test order without changing which result is tested.

Evidence: [D097](DECOMP.md?plain=1#L1334).

<a id="cc09"></a>

### CC09 — A null guard plus returned Win32 result splits returns

`if(!path) return 0; return SetCurrentDirectoryA(path);` produces the measured original jne/ret split (7i/17B); a void wrapper shares a final return (6i/16B). Returning the API result restores both the control flow and recovered return type.

Evidence: [D007](DECOMP.md?plain=1#L519).

<a id="cc10"></a>

### CC10 — An IAT load hoisted around a loop usually needs no explicit cache

`mov esi,[__imp__X] / call esi` follows ordinary `__declspec(dllimport) __stdcall` calls inside a loop. `FlipPrimary`'s timeGetTime and `RES_EnsureMounted`'s MessageBoxA hoist automatically, with a third post-loop call reusing the register. Do not invent a function-pointer cache from that shape alone; `MusicThread`'s unusual preheader/latch reload placement is separately measured as a floor.

Evidence: [D097](DECOMP.md?plain=1#L1334).

<a id="cc11"></a>

### CC11 — Literal visibility and per-body pragmas choose intrinsic versus called copying

`PlayMovie`'s literal `strcpy(path,"FMV\\")` emits a pooled dword load/store plus byte load/store; an `extern const char[]` produces the full 20-instruction repne scasb / rep movsd / rep movsb expansion. Its real global retry prefix g_res_path correctly keeps the long form, so visibility at the call site matters.

`#pragma function(memcpy)` around `SetReportMovie` selects the original called 0x100-byte copy (`call 0x004a0110`), while restoring `#pragma intrinsic(memcpy)` afterward keeps `RestoreCurrentProfileFromList`'s 200-byte copy as rep movsd. Under /O2 alone the intrinsic wins throughout the file. Do not infer an intrinsic from memcpy's name without the active pragma and known source length.

Evidence: [D013](DECOMP.md?plain=1#L607).

<a id="reading-original"></a>

## Reading the original

| Rule | Question |
| --- | --- |
| [RD01](#rd01) | Check identities and semantics as well as normalized instructions |
| [RD02](#rd02) | Diff families before transferring source or residuals |
| [RD03](#rd03) | Distinguish assembly signatures from an ordinary C body with inline asm |
| [RD04](#rd04) | Treat MASM operand names as assembler identifiers |
| [RD05](#rd05) | A repeated store instruction does not establish emitted source order |
| [RD06](#rd06) | Infer a missing named local from reloads and recomputation |
| [RD07](#rd07) | Use overwrite and conversion operands to reconstruct read order |
| [RD08](#rd08) | A repeated fill does not prove memset |

<a id="rd01"></a>

### RD01 — Check identities and semantics as well as normalized instructions

A zero-mismatch normalized body does not prove its global or callee identities. Review COFF relocations against the original addresses when several globals have identical shapes.

- `NewMechanicOrder` passed 19i/67B with zero mismatches while its head and tail stores were SWAPPED; an independent relocation review caught it.
- `Coaster3D_SetCarClipDepth` passed 19i/105B with zero mismatches while loading a/b/c instead of b/c/d at three sites. Twelve relocations were wrong. Reversing the first float sum's operands, copying direction as one Vec3f, then storing angle and a reproduced all 12. The 0.0015875f travel constant was read as raw IEEE-754 bits at 0x004ab404, not inferred from a nominal frame rate. [Expanded evidence](lanes/codex-e.md#measured-ccodegen-findings).
- An extern comment `/* 0x004299a30 -> ... */` has nine address digits: `callees.py` read 0x004299a3 (instruction 8 of 0x00429990), while the actual entry is 0x00429a30. Address comments are parsed data.
- Names can misdescribe behavior: 0x0047b7b0 is an ICM error reporter with a six-arm `MessageBoxA` switch, not a loader; 0x0047cdd0 unloads .TSF, not ODF, so LLIDB type 0x40 is .TSF. These were historically kept as caller names for resolution; that history does not make the name a type/codegen lever.

`sub_411650` is `LFBoat_IsOnDrop`: the disassembly compares with the LOG FLUME DROP class. Existing names are hypotheses until their address and operation agree.

Evidence: [D001](DECOMP.md?plain=1#L427), [D006](DECOMP.md?plain=1#L504), [D018](DECOMP.md?plain=1#L741), [D037](DECOMP.md?plain=1#L967), [D069](DECOMP.md?plain=1#L1170).

<a id="rd02"></a>

### RD02 — Diff families before transferring source or residuals

Find twins by interface slot and callee overlap, then diff their whole disassemblies. Equal size alone is weak evidence; even a proven shared source can diverge under different pressure.

- `screencb2.c`: 17 of 17 close by grouping seventeen callbacks by ObjDef +0xb8 load, +0xac destroy, +0xa4 create, +0x90 update, then diffing each sibling. Nine match first try; `Carousel_Load`/`Balloonz_Load` differ only in record size and two globals. Two adjacent functions calling the same three helpers in order were one source two arguments apart.
- `Sub_411680`/`Sub_411810` are both 121 instructions, but `LFBoat_Advance`/`LFBoat_Fall` share only a boat pointer. `LFPiece_HasCursor` requires the inverse branch layout of `LFPiece_HasRider`, improving 41 -> 49 of 49.
- `CoasterCar_BuildRider` has substitution loops of 0x42/0x40/0x42 bytes: loops 1 and 3 materialise the record address; loop 2 folds an add away. Do not clone loop 1 into loop 2.
- `BsWater_DrawSelection`/`JcWater_DrawSelection` share a position-for-position 32-mismatch residual; `FOODCART FOOD`/`SHARK CAFE` are one statement apart. `BsBoat_Animate` has the same three-instruction `imul` rank residual and indices predicted from `JcBoat_Animate`; keep the linked evidence without re-opening either floor.
- The fable-B intermediate-pointer experiment is the counterexample to identical residuals being required for shared source: its twin has ONE compare and needs no local, diverging precisely where register pressure differs. Earlier universal wording about twin residuals was too strong.

`CalculateMapRenderOrder` records setup as p.x / p.y / node_next / memset / link LAST. The full-map twin has an intervening `ElemID` call and needs link FIRST; otherwise `mov esi,K / mov [esp+0x10],esi` sinks past the guard. That pair accounts for 160 -> 162 of 162. A new call invalidates a sibling's schedule recipe.

Evidence: [D013](DECOMP.md?plain=1#L607), [D025](DECOMP.md?plain=1#L795), [D031](DECOMP.md?plain=1#L931), [D087](DECOMP.md?plain=1#L1293), [D097](DECOMP.md?plain=1#L1334), [D131](DECOMP.md?plain=1#L1630), [D137](DECOMP.md?plain=1#L1667), [D158](DECOMP.md?plain=1#L1941).

<a id="rd03"></a>

### RD03 — Distinguish assembly signatures from an ordinary C body with inline asm

Use converging evidence, not an EBP frame alone. The four `tri3d.c` rasterisers (`DrawFlatTri`, `DrawGouraudTri`, `DrawFlatTexTri`, `DrawGouraudTexTri`, 2743 instructions) were reconstructed `__declspec(naked)`: memory `xchg dword ptr [ebp+0xc],eax` plus callee-saved pushes inside the stream (`push ebx/esi` after two movs, `push edi` after a cmp) identify hand-scheduled regions. VC6 inline asm puts those saves at the region top; splitting regions did not reproduce the interleaving.

`ZBuffer_FillPoly` (0x00423350) is partly assembly: EBP frame in `/O2`, `xchg ebx,eax` (0x93), `add ebx,1` where tested VC6 emits inc, and `jns/jmp` instead of js; post-asm reloads expose the boundary. It was recorded WIP with these proofs.

`Draw3DPersonModel` is mixed C plus fixed-point `__asm` macros, not an entirely handwritten body. Its ebx/esi/edi saves are at the prologue top and its 1023 instructions contain no memory xchg. That checkpoint improved 33.3% -> 44.3%, mnemonic LCS 94%, no mnemonic gap of five or more instructions, with 707 instructions matching everything except `[ebp-N]`. The later structural and frame measurements refine the residual; the frame alone was never proof of handwritten assembly.

Evidence: [D045](DECOMP.md?plain=1#L1021), [D213](DECOMP.md?plain=1#L2396), [D410](DECOMP.md?plain=1#L3839).

<a id="rd04"></a>

### RD04 — Treat MASM operand names as assembler identifiers

Check every local used by an `__asm` macro against MASM reserved words. In the reconstruction of `Draw3DPersonModel`, `__asm { mov cr2, eax }` used a local named `cr2`; MASM emitted privileged `0f 22 d0` for the control register instead of writing the local, and the subsequent comparison read an uninitialised dead-argument slot. That generated reconstruction would fault at ring 3; this is not evidence that the retail game contains the bug.

Avoid operand names `cr0`-`cr4`, `dr0`-`dr7`, `tr3`-`tr7`, `st`, and segment-register names. The `/FAc` tell is `0f 22 d0 mov cr2,eax` instead of `mov DWORD PTR _x$[ebp],eax`. The 2026-09-04 tree sweep found only one other reserved local, `st` in schoolcar.c; it is never an asm operand and the file is exact. Repeat when a new asm file appears.

Evidence: [D456](DECOMP.md?plain=1#L4227).

<a id="rd05"></a>

### RD05 — A repeated store instruction does not establish emitted source order

**Emitted store order is NOT evidence of source order.** In `CastleBbq_Tick`
  the y store is emitted first, yet the source is x-first: VC6 reorders the
  adjacent pair, and the later re-read follows the EMITTED order. Measure both
  orders rather than reading one off the listing (same family as "adjacent
  address stores come out reversed").

Calls impose a real boundary: VC6 does not hoist a store to an address-taken local above a call. Moving that statement below a call cannot recover an earlier store pair; the positive scheduling freedom is within the applicable call/alias barriers.

Evidence: [D136](DECOMP.md?plain=1#L1662), [D435](DECOMP.md?plain=1#L4042).

<a id="rd06"></a>

### RD06 — Infer a missing named local from reloads and recomputation

Two reads of the same pointer field with no intervening call but intervening global stores rule out a cached named local in the measured shape: those stores cannot alias the local. This identified `RenderFullMap`'s `spr` cache as wrong; deleting it also freed the callee-saved register the original uses elsewhere. Likewise a value re-derived from a reloaded pointer across calls instead of reloaded from its own home indicates no named local for that derived value.

Evidence: [D440](DECOMP.md?plain=1#L4074), [D484](DECOMP.md?plain=1#L4447).

<a id="rd07"></a>

### RD07 — Use overwrite and conversion operands to reconstruct read order

A three-load run whose LAST load overwrites its own base register is the signature of consecutive source reads through one pointer. In `Draw3DPersonModel`, one head-field read belonged BETWEEN two others: all 120 permutations of five assignments confirmed a unique best and every structural measure improved. In `AnimApplyPart`, the ten `fild` operand displacements reveal the conversion order directly; a different order scored better on every metric but grew the frame and contradicted that original order. Do not substitute a metric winner for the observable read sequence.

Evidence: [D474](DECOMP.md?plain=1#L4366), [D479](DECOMP.md?plain=1#L4404).

<a id="rd08"></a>

### RD08 — A repeated fill does not prove memset

**`rep stosd` is NOT proof of `memset`.** VC6 SP3 turns a constant-count
  array fill into `rep stosd` even when the fill value is a non-zero ADDRESS
  constant: `for (i = 0; i < 0x400; i++) tab[i] = &fallback;` is
  `mov ecx,400h / mov eax,OFFSET / mov edi,OFFSET / rep stosd`
  (`CoasterShades_Init`, exact first try).

Evidence: [D070](DECOMP.md?plain=1#L1178).

<a id="triage-method"></a>

## Triage and method

| Rule | Question |
| --- | --- |
| [TM01](#tm01) | Use the authoritative full-extent gate and fresh compiler output |
| [TM02](#tm02) | Classify the residual, then test cheap allocation levers |
| [TM03](#tm03) | Build a control-flow-aware frame map before interpreting stack offsets |
| [TM04](#tm04) | Align displaced blocks and check for compensating errors |
| [TM05](#tm05) | Compare consistent register permutations, not only register blindness |
| [TM06](#tm06) | Scan known code shapes, and falsify restrictive scans |
| [TM07](#tm07) | Account for every byte before hunting encoding forms |
| [TM08](#tm08) | Localize whole-function pressure with case stubs and reference bisection |
| [TM09](#tm09) | Retest shims and prior sweeps after structural changes |
| [TM10](#tm10) | Separate observable reconstruction bugs from codegen deviations |
| [TM11](#tm11) | Apply the project marker and integration rules |
| [TM12](#tm12) | Choose reachable work without mistaking a small strict count for completion |

<a id="tm01"></a>

### TM01 — Use the authoritative full-extent gate and fresh compiler output

Keep `WIP-FUNCTION` until `audit.py` reports `[OK]` for the full body: instruction count, byte extent, strict normalized sequence and no escaping branch all matter. Fail a side-by-side harness on `error C`; otherwise it can reuse an old object and report stale “0 mismatches.”

The rotated-loop extent defect was FIXED on 2026-09-05: `_loop_entry` looks past a forward jump target for a back-edge into the skipped region; a tail-jump wrapper has none. `MatMul` (0x00426120), whose instruction 9 jumps over a reload, was falsely truncated to 10i/28B with ESCAPES; it now audits 40 instructions / 103 bytes `[OK]`. `Coaster3D_BuildPieceGeometry` (0x004284d0) is bounded at 143i/528B. The old claim that it can never pass until the walker is fixed is obsolete. Separately, `matchfull.py` can over-report a return followed by a `.rdata` jump table: `BsBoat_StepLeg` 295/301 versus authoritative audit 294/294.

Evidence: [D037](DECOMP.md?plain=1#L967), [D064](DECOMP.md?plain=1#L1134), [D108](DECOMP.md?plain=1#L1480), [D243](DECOMP.md?plain=1#L2610).

<a id="tm02"></a>

### TM02 — Classify the residual, then test cheap allocation levers

Measure strict, register-blind (rb) and offset-blind (ob), resolving frame homes by esp/push depth. These are diagnostics, not substitutes for the final strict gate.

| Signature | Next investigation |
| --- | --- |
| strict >> rb | Allocation: inspect live webs, try one free volatile load at an original reload, then name an array element/call result or split a nested call. |
| strict >> ob | Frame: build the home map and surviving-reference profile; declaration-order permutations usually do nothing. |
| strict == rb == ob | Scheduling, provided the instruction multiset, registers and resolved homes agree: sweep relevant store/update order. |
| rb still high | Structural reconstruction: prioritize this work. |

Evidence: seven wave-eleven partials included three tiny strict residuals that were floors. `Balloonz_Tick` and `ValidateCursor` each had pure scheduling 5. `SubtractObjRect` was strict 14 / rb 0, nine spellings inert, but a free volatile read closed all 14; the other candidate load closed 11 of 14 and moved a spill two early. `BoatingSchool_Tick` gains 219 and 115 at two original-load sites; three free loads singly and together in `WW_AnyBlokeInRect` were byte-identical. In `BoatingSchool_Destroy`/`JungleCruise_Destroy`, volatile head/count/sprite reads and twelve loop forms were inert, yet naming `spr = ilf->sprites[(unsigned char)i]; LLSStop(GetLLSForSprite(spr));` adds the needed IR temporary and closes 12 / 6 mismatches plus one byte each. Thus rb 0 and an inert volatile test are signals, not proofs of a floor. `TraceRoute`'s byte-exact choice had 105 mismatches, 339/339 B, versus a lower strict 101 with 340 B.

Evidence: [D127](DECOMP.md?plain=1#L1601), [D138](DECOMP.md?plain=1#L1675), [D185](DECOMP.md?plain=1#L2153), [D190](DECOMP.md?plain=1#L2204).

<a id="tm03"></a>

### TM03 — Build a control-flow-aware frame map before interpreting stack offsets

Track push/pop/add-esp depth through control flow, reconciling the frame base at branch targets. Raw `[esp+N]` between a push and cleanup names a home 4 or 8 bytes lower than N; linear simulation drifts at joins. Four of one lane's seven reconstruction errors were invisible until this was fixed.

Pair index-aligned register-blind lines to derive `original home -> reconstructed home`; use a windowed bijection so disjoint lifetime reuse is not mistaken for inconsistency. Offset-blind normalization collapses `[ebp+8]` and `[ebp-0x10]` alike and can conceal swapped operands.

Evidence: `StepSchoolCar`'s claimed exact frame had only 5 of 15 homes agreeing (`w10joust/fm2.py`). `laneG/fm.py` misreads `RenderFullMap` as 0 frame / 66 argument slots; use `w9renderview/slots.py` there. On `RenderView`, the control-flow map confirms the same 40 slots and 0x2f90 frame, with the lowest 13 differing by one move plus a `tw + tw` temp that the original recomputes. `Draw3DPersonModel`'s bijection reclassified 2 of “367 frame offsets” as operand order (`w10p3d/bij.py`, `bij2.py`). In seven other partials the trap did not recur: `Balloonz_Tick`, `DrawPopUpInfo`, `GetObjectUID`, `ValidateCursor` had ob never below strict after depth resolution. Another measurement classified ~72 of 182 mismatches as numbering downstream of one spill flip.

For equal counts with a large mismatch, map the frame first. A later lane's corpus-first search across four such functions found only three corpus hits and no helpful idiom. Historic instruments: `scratchpad/w10joust/fm2.py`, `w9renderview/esp.py`, `slots.py`, `sweep2/fm.py` and `sbs.py --rb --only-diff`; these scratch files are evidence references, not guaranteed distributed tools.

`AnimApplyPart` indices 22-216 are identical under a uniform +4 frame shift; the reported 182 strict mismatches therefore substantially overstate the actual structural distance.

Evidence: [D186](DECOMP.md?plain=1#L2176), [D202](DECOMP.md?plain=1#L2305), [D206](DECOMP.md?plain=1#L2337), [D216](DECOMP.md?plain=1#L2424), [D339](DECOMP.md?plain=1#L3296), [D357](DECOMP.md?plain=1#L3433), [D375](DECOMP.md?plain=1#L3556), [D468](DECOMP.md?plain=1#L4326), [D482](DECOMP.md?plain=1#L4435).

<a id="tm04"></a>

### TM04 — Align displaced blocks and check for compensating errors

Once a block moves, strict index distance can reward the wrong code. Rank candidates by LCS-aligned structural regions, rb and ob, with byte count and the original semantics, then confirm by the strict audit. A strict fall with an rb rise flags a compensating error; a strict rise can be the correct direction.

- `RenderView`: source case order 1,3,2,4 and a last constant assignment were chosen by strict count but contradicted the original natural order. A strict 502 candidate had 40 fewer structurally wrong slots than strict 478. Its later 381 hid +10 instructions cancelling -11; fixing limits improved region 99 -> 88 and ob 228 -> 187 while strict rose to 817. The paired geometry/split-prologue question is one problem; do not apply an old “once +10 is fixed” prescription in isolation.
- `StepSchoolCar`'s former 351/351 hid two extra and two missing instructions. Another lane corrected both a store order and a bloke-position order that had been selected by strict score.
- Declined variants: store swap 27 -> 25 was accidental alignment; negating a difference added an absent neg; a spill-struct volatile store scored 208 -> 118 while rb rose.
- `WW_AnyBlokeInRect`: intentionally moved strict 6 -> 12. The 6 body was 30 instructions / 67 bytes, one short each with a misplaced `mov eax,1`; `return 0` restores 31/31 instructions, 68/68 bytes, identical blocks/branch offsets and rb 0. Return coalescing would delete the original xor. The loop-local temp wins with 2 in-loop compares, while 0 or 1 gives the cursor eax; more cursor references do not restore it.
- `UpdateRiverTile`: rb 3 against strict 67 identifies one sunk `push ebx`; the original rematerialises one web on the back edge while the reconstruction splits it. `UpdateRiverAnim`'s old “sums plus two schedules” breakdown was wrong: 44 mismatches are a three-slot shift of identical instructions, only 31 are sums.

Historic instruments: `scratchpad/mechrides/rank.py`, `rbdiff.py`, `frame.py`; `scratchpad/joust/align.py`, `rbscore.py`; `scratchpad/laneG/regions.py`, `wscore.py`. A separate pressure experiment found 17 loop-head spellings identical: the decisive pressure can be elsewhere.

Evidence: [D254](DECOMP.md?plain=1#L2699), [D298](DECOMP.md?plain=1#L3031), [D372](DECOMP.md?plain=1#L3530), [D388](DECOMP.md?plain=1#L3666), [D396](DECOMP.md?plain=1#L3735), [D437](DECOMP.md?plain=1#L4050), [D442](DECOMP.md?plain=1#L4083), [D446](DECOMP.md?plain=1#L4114), [D452](DECOMP.md?plain=1#L4160).

<a id="tm05"></a>

### TM05 — Compare consistent register permutations, not only register blindness

Use a width-aware best permutation of the callee-saved registers (`bx`/`bp`/`di`/`bl` included), alongside strict, rb and the compensating-error check. A single consistent renaming is different from independently ignoring every register.

`Carousel_Tick`: strict ranks three candidates 13 < 27 < 29; permutation-aware ranks 8 < 13 < 27, the order supported by the disassembly. One claimed 149 -> 138 gain becomes real 138 -> 136 (eleven of the apparent gain was renaming). Four allegedly same-wall bodies instead satisfy `real == strict` under IDENTITY, so there is no register difference to explain. In a later `Carousel_Tick` run, permutation `bpdibxsi` stayed fixed across ~130 variants and moved only after structural destruction, despite identical webs, references and live ranges: stop searching the wrong block. Historical instrument: `scratchpad/laneK/permrank.py`.

Evidence: [D413](DECOMP.md?plain=1#L3857), [D424](DECOMP.md?plain=1#L3949), [D436](DECOMP.md?plain=1#L4044).

<a id="tm06"></a>

### TM06 — Scan known code shapes, and falsify restrictive scans

For a register choice or unexplained idiom, search already-exact bodies before sweeping expressions. For equal-count large frame/schedule residuals, map homes first. An exhaustive variant search only bounds those variants. Before saying a shape is absent, rerun the binary/corpus scan with looser conditions.

Evidence: `add reg,-K` had been declared unreachable, yet .text contains 75 and the tree already emitted one. `__BMPLoader` supplied the idiom closing `LoadPalette` 32 -> 0 in ten minutes. Constant scans found `GetBuildTime` and `SetBridgeDrawOffsets` as opposite cases. Corpus-first paid nothing on four frame/schedule residuals but paid immediately on three register/idiom residuals, including `GetObjectUID` after three speculative passes and initially challenging `LFEntrance_Activate`'s liveness explanation. That challenge was later RETRACTED: the operand remains live into the switch, as the corrected liveness experiment establishes. Scans found 49 landing pads, 5 duplicated two-arm materialisations, 47 three-register leas. A four-instruction-window spill/reload scan returned two twins and identified a memory-resident variable; the next calibration was decisive on one, partly useful on one, negatively useful on two (no instance in 1541 exact bodies). A strict consecutive-load scan falsely claimed absence; its looser version found exact `Fort_TickRiders` (0x00406660).

Historical tools: `scratchpad/misc3/scan_const.py`, `scan2/scan3`, `probe.py`; `scratchpad/sweep1/scan_argcopy.py`, `scan_exit.py`; `scratchpad/sweep4/scan_pad.py`, `scan_dupload.py`, `scan_lea3.py`. Use a standalone probe for an expression-level hypothesis.

Evidence: [D181](DECOMP.md?plain=1#L2117), [D311](DECOMP.md?plain=1#L3116), [D322](DECOMP.md?plain=1#L3189), [D339](DECOMP.md?plain=1#L3296), [D344](DECOMP.md?plain=1#L3332), [D353](DECOMP.md?plain=1#L3403), [D398](DECOMP.md?plain=1#L3749), [D402](DECOMP.md?plain=1#L3771).

<a id="tm07"></a>

### TM07 — Account for every byte before hunting encoding forms

Subtract trailing pad NOPs before diagnosing a byte deficit. `Road_FindDiagonals` reports 64 instructions / 153 B but has 62 / 151 real instructions/bytes after two pads: the actual deficit is 6 bytes, the missing second `mov edi,[esp+10h]` (4) plus jmp (2), not the earlier vague four-byte deficit prompting a 2-vs-3-byte encoding hunt.

For a one-instruction-short body, align `orig[i]` to `ours[i-1]` after divergence: one measured 148-instruction tail residual dropped to 32. As a disposable diagnostic only, add a deliberately wrong early instruction (e.g. `x - 5 != 0` causing mov/sub/je) and watch whether the count falls. Never retain that artificial instruction as a match.

Evidence: [D191](DECOMP.md?plain=1#L2213), [D302](DECOMP.md?plain=1#L3056), [D320](DECOMP.md?plain=1#L3180).

<a id="tm08"></a>

### TM08 — Localize whole-function pressure with case stubs and reference bisection

In scratch variants, stub each case once and observe the head register to isolate which case controls a whole-function phase. This also localises non-switch loop-head rotations. A negative is useful: all nine stubbed variants keeping the same cursor register rule out “one case's pressure.” A `Carousel_Tick` allocation cause was isolated by bisecting reference count store-by-store; splitting names per loop is byte-identical because allocation follows webs, and the previously blamed interfering value was wrong.

Sibling repeated-call blocks expose a three-step eax/ecx/edx phase in their first lea; tails merge only when the phase matches the layout-last host. Record copy-in/copy-out order can be searched with all-pairs swaps. An extra store between struct store and call can rephase the whole block; a store emitted after the call cannot have originated before it.

Evidence: [D295](DECOMP.md?plain=1#L3006), [D319](DECOMP.md?plain=1#L3174), [D338](DECOMP.md?plain=1#L3292), [D366](DECOMP.md?plain=1#L3499), [D397](DECOMP.md?plain=1#L3743).

<a id="tm09"></a>

### TM09 — Retest shims and prior sweeps after structural changes

A negative measurement belongs to its baseline. Rerun affected store/initialiser/permutation searches and re-audit every compensating shim after a structural fix.

`Balloonz_Tick`: 128 zero placements and 720 copy-in orders were inert, then decisive after another fix (78 -> 5). Two of one lane's three gains came from old exhausted sweeps whose numbers no longer held. A once-winning unsigned-char cache became wrong after a phase fix; plain field reads won. One array block spanning two loops was once identical to one block per loop, then became three worse. `UpdatePersonPos`: after one Pos held both unscaled isometric coordinates (`projected.x = bx - by; projected.y = by + bx`, giving `lea ecx,[ebx+ebp]`), removing the old volatile closed the last six.

Read the body's note before a scope summary: `InitExitCheckBox` already had five passes, a three-dword/no-fourth-use constant-web floor, despite a brief calling it unexplored; one scope's table had fifteen functions while its prose said fourteen.

On `PlaneRide_Activate`, the old spill-producing volatile was worth -52 once the y chain was correct; removing it also fixed a three-way head rotation and a 2-byte deficit. The independent `b->flags |= 0x80` store changed preferred position with the surrounding shape: above the subtractions while y reassociated, below the seed stores after the fix (worth -21 and -8 respectively). Those signs are the source's reported scores, not universal costs.

Evidence: [D007](DECOMP.md?plain=1#L519), [D326](DECOMP.md?plain=1#L3218), [D332](DECOMP.md?plain=1#L3256), [D336](DECOMP.md?plain=1#L3282), [D371](DECOMP.md?plain=1#L3525), [D467](DECOMP.md?plain=1#L4322), [D480](DECOMP.md?plain=1#L4411).

<a id="tm10"></a>

### TM10 — Separate observable reconstruction bugs from codegen deviations

Read the original before searching spellings. Record the behavioral error, its observability and the compiler difference separately; never accept semantic changes merely because their normalized score improves.

- `RenderView`'s `g_sort_count = 0` belongs inside `if (cell->obj != 0)`: the je and unchanged zero edx prove it, and TWO zero registers serve six stores. All 13 .text references are inside RenderView and reads occur in that same arm, so the source author recorded the unobservable deviation instead of taking the best of eight placements at +5 strict. The MASM cr2 reconstruction bug was observable and required correction.
- `Draw3DPersonModel`: three corrections improve 981 -> 687, matchfull 44.3% -> 52.0%, mnemonic LCS 96%, ob-LCS 707 -> 820: whole-struct `box[0] = fr->bmin` (`mov eax,esi` plus three moves) breaks the field CSE; transpose reads `p->matrix[k]` directly instead of cached `mp[j]` (239 alone, preserving `lea eax,[edi+0x58]` and two pointer homes instead of `add edi,0x58`); recompute the vertex pointer FIRST inside the loop, eliminating the wrong tail increment.
- `RenderFullMap`: five corrections improve 894 -> 847, region 594 -> 498, register+offset-blind 311 -> 266, 4199 -> 4223 of 4225 bytes. Add scroll offsets to bounds BEFORE the cell-size globals (77 structural slots; four wrong duplicate load/store pairs); share ONE named Pos across three calls and adjust its COPY at the last; restore the two projections to their API calls; use ONE 20-byte cell copy (three total rep movsd); remove two erroneous frame-ballast demotions. The corrected frame is 4 bytes over: the prior exact size omitted three real constructs. Earlier “mutates the Pos in place” advice was wrong.
- The old geometry ordering “once +10 is fixed” is retired: moving both limits breaks the split prologue; constants keep their registers, but a value's web no longer coalesces with constant zero. A duplicated null test on an address-taken struct is NOT folded as an older note claimed.
- `LoadBaseMap` reconstruction also had three actual defects: re-decoding map flags as base tiles for height > 0, reading uninitialised S10 end-of-run bytes, and one excess pointer dereference. These are reconstruction history, not permission to alter retail behavior.

Evidence: [D197](DECOMP.md?plain=1#L2257), [D430](DECOMP.md?plain=1#L4012), [D462](DECOMP.md?plain=1#L4279), [D485](DECOMP.md?plain=1#L4450), [D493](DECOMP.md?plain=1#L4492).

<a id="tm11"></a>

### TM11 — Apply the project marker and integration rules

[HANDOFF §3](HANDOFF.md#3-rules-that-are-not-in-the-code) and [the parallel contract](PARALLEL_CONTRACT.md) supply the operating rules: keep the marker immediately above its signature and put explanations above the marker; the old parser note says markers bind 1–3 lines ahead, but the contract's immediate placement avoids silently uncounted functions. Count committed markers, not in-flight files. Record every extern address in a trailing comment and preserve caller-side prototype types unless all affected files are re-audited. No binaries or game assets are committed.

Use per-process objects and the scope's `audit.py`/`matchfull.py` checks. Do not run shared integration tools or modify tools in an active lane. The historical fixed `/tmp/_match.obj` race produced disagreement on 49 functions and four phantom regressions; `match.py` gained per-pid objects on 2026-09-03, but HANDOFF still requires isolated verification. The contract is stricter than the old handoff snippets; follow it. No compiler run is needed to consolidate this documentation.

Evidence: [HANDOFF §3](HANDOFF.md#3-rules-that-are-not-in-the-code), [§6B](HANDOFF.md#6-where-to-go-next) and [parallel contract](PARALLEL_CONTRACT.md).

<a id="tm12"></a>

### TM12 — Choose reachable work without mistaking a small strict count for completion

[HANDOFF §6B](HANDOFF.md#6-where-to-go-next) prioritizes small bodies and structural residuals. Waves five through ten repeatedly targeted `RenderFullMap`, `RenderView`, `Draw3DPersonModel`, `StepSchoolCar` and closed none in six waves while `ClampPopUpToScreen` (43 instructions, mismatch 3) and `WW_AnyBlokeInRect` (31 instructions) were untouched. But wave eleven's three strict-3 candidates (`RequestRoute`, `JcBoat_Animate`, `ClampPopUpToScreen`) were also floors. Across seven partials that wave closed none; five were exhausted. Strict 3 / rb 0 can be a floor, while strict 30 / rb 25 still has structure; apply the volatile and temporary counterexamples before retiring.

Exact byte size narrows an unexplained residual but does not prove types or identities: encoding coincidences, normalization and compensating errors still require the other checks. Reuse proven twins only after a disassembly diff.

Evidence: [HANDOFF §3](HANDOFF.md#3-rules-that-are-not-in-the-code), [§6B](HANDOFF.md#6-where-to-go-next) and [parallel contract](PARALLEL_CONTRACT.md).

<a id="negatives"></a>

## Measured negatives

These are failed constructions or retired residuals on the named bodies, not universal impossibility theorems. Check the paired positive rule and baseline before reusing a result. Where the record does not name a body, that limitation is explicit.

| Rule | Question |
| --- | --- |
| [NG01](#ng01) | A load’s emitted index does not determine its register allocation |
| [NG02](#ng02) | A single-use n-1 rematerialises in the measured loop shape |
| [NG03](#ng03) | Add-rank and same-width conversion probes have body-specific limits |
| [NG04](#ng04) | AnimApplyPart: float unions and no-code casts do not cure its x87 hoist |
| [NG05](#ng05) | Anonymous copy-pair and sibling-difference experiments |
| [NG06](#ng06) | Balloonz_Tick: paired stores do not prove a shared source object |
| [NG07](#ng07) | BNV y chains: separate source shape, register rotation, named ys, and stall filling |
| [NG08](#ng08) | BoatingSchool_Add and ValidateCursor recorded minima |
| [NG09](#ng09) | BsWater_SetTile: measured latch order remained unreachable |
| [NG10](#ng10) | Cheap address takes, integer no-ops and inline wrappers do not manufacture webs |
| [NG11](#ng11) | CoasterCar_BuildRider: address fold cannot be separated from scheduling |
| [NG12](#ng12) | Draw3DPersonModel frame profile: same weights, incompatible array insertion points |
| [NG13](#ng13) | Draw3DPersonModel: field-sum canonical order is not movable by respelling |
| [NG14](#ng14) | DrawPopUpInfo: residency and reload position remain coupled |
| [NG15](#ng15) | Empty reload-diamond arms cannot retain duplicated spill reloads |
| [NG16](#ng16) | GetObjectUID: volatile colouring leaves a four-instruction barrier floor |
| [NG17](#ng17) | Global list-head volatile cannot raise the load above pushes |
| [NG18](#ng18) | GoldRush_KneelAtPan: one field value cannot retain the required call-crossing web |
| [NG19](#ng19) | Integer casts do not provide FP-style no-code scheduling tuples |
| [NG20](#ng20) | JcBoat_Animate and BsBoat_Animate: bare-IV product rank remains a measured floor |
| [NG21](#ng21) | JungleCruise_Tick and five ride activations: goto-flip does not transfer universally |
| [NG22](#ng22) | JungleCruise_TraceRoute: loop-free ranking with a loop |
| [NG23](#ng23) | JungleCruise_UpdateRiverAnim (UpdateRiverAnim): the original exiled arm remained unreachable |
| [NG24](#ng24) | JungleCruise_UpdateRiverAnim: the partial-sum barrier fixes order but loses overall |
| [NG25](#ng25) | LFEntrance_Activate: the three-register lea is a live-operand issue |
| [NG26](#ng26) | LoadScriptEvent: a straight-line zero return loses the original join block |
| [NG27](#ng27) | MusicThread: preheader-and-latch IAT reload placement |
| [NG28](#ng28) | Pump_SnapToRoad cannot retain the known-null retest without a new home |
| [NG29](#ng29) | RemoveNewObjectMarker: plain-index anchor and counter schedule stay coupled |
| [NG30](#ng30) | RenderCursor: five-instruction non-canonical suffix below this build’s floor |
| [NG31](#ng31) | RenderFullMap and RenderView: measured source reshaping does not select the needed edge |
| [NG32](#ng32) | RenderView split prologue is a saved-register allocation-order problem |
| [NG33](#ng33) | RequestRoute and InsertChildIntoList have independent uncoalesced-copy floors |
| [NG34](#ng34) | Route_StepFree / Route_StepToPieceEnd: snapshot base never materialises |
| [NG35](#ng35) | SchoolCarBlockedAhead: mixed-axis schedule has two incompatible outcomes |
| [NG36](#ng36) | SpinningBarrels_Activate: an empty if inserted at another seam is harmful |
| [NG37](#ng37) | TempleSlide_Update: the additional X web is not an almost-finished solution |
| [NG38](#ng38) | The activation-family window is a missing register choice, not five scheduling holes |
| [NG39](#ng39) | Three ride projections: ascending short loads and x-first stores remain incompatible |
| [NG40](#ng40) | TrackCurve_GatherParams: dead loop-exit count reload |
| [NG41](#ng41) | UnlinkGardenerOrder: identical tails still merge onto the wrong arm |
| [NG42](#ng42) | Unnamed switch witness: measured CFG and call respellings were inert |
| [NG43](#ng43) | UpdateControllerFromMouseData remains deliberately abandoned |
| [NG44](#ng44) | UpdateRiverTile: one web versus a split web leaves a sunk push |
| [NG45](#ng45) | UpdateSampleSource: volatile input order conflicts with its shared tail |
| [NG46](#ng46) | ValidateCursor: escaped-store reload remains after helper improvement |
| [NG47](#ng47) | ZBuffer_FillPoly: row and ylast dead-argument homes remain swapped |

<a id="ng01"></a>

### NG01 — A load’s emitted index does not determine its register allocation

Allocation precedes scheduling in these tests. `JungleCruise_Tick`'s volatile pin puts the contested station/global load at exactly the original index yet keeps the wrong register; named seat-table reads/all six add-operand routes are inert when both attractors already have rank-1 inline memory. Its six original references are real, not an undercount. `LFEntrance_Activate`'s earlier about 170-variant liveness study was refined by all 40 legal orders leave the value in the same register in 40/40 despite moving it in the stream. A corpus scan found 47 three-register leas; the original's `LFEntrance_Activate` lea is an ordinary liveness case, like `BuildCursorPtr`: the preamble operand IS read in the later switch block. This retracts both the earlier load-order-colours-it theory and the claim neither operand survived.

`SpaceTower_Activate` shows that a seemingly immutable rank may still have a real named-value lever: b>rec>{tile,tx}>r versus target b>rec>r>{tile,tx}; one extra cursor reference overshoots two ranks, 22 single-reference mutations and eight declaration orders inert. `int tiley=tile->b.y` used by two cases moves it, strict 149 → 138 and indices 0-22 exact (permutation-aware correction: real 138 → 136, eleven nominal points were renaming). Root cause remains TWO original descriptor reloads at indices 22/23; one dies before next load, ours has one held through 32, forcing contended value into eax/spill and frame 0x10 → 0x14. Route 103 versus 138 is rejected because rb worsens 14 → 21 and frame is wrong; a valid future second nonvolatile reload at 23 must land with both changes.

Evidence: [D265](DECOMP.md?plain=1#L2785), [D348](DECOMP.md?plain=1#L3366), [D379](DECOMP.md?plain=1#L3585), [D398](DECOMP.md?plain=1#L3749), [D407](DECOMP.md?plain=1#L3806), [D415](DECOMP.md?plain=1#L3887), [D416](DECOMP.md?plain=1#L3899), [D450](DECOMP.md?plain=1#L4145), [D472](DECOMP.md?plain=1#L4347).

<a id="ng02"></a>

### NG02 — A single-use n-1 rematerialises in the measured loop shape

For the single-use n-1 loop value (body unnamed in D267), named local, two-def, const, block-scope, struct member, address-taken and inline-helper parameter forms all forward-substitute into a lea inside the loop, even across calls. Volatile, short/char (with extra movsx), or a post-loop use retain a spill slot, each changing another property. This is a measured shape limit, not a universal prohibition: `LFEntrance_Add` later obtained its n-1 spill, 0x28 frame and identical indices 0-61 by ordering two independent assignments. Spill slots run low-to-high by allocation priority.

Evidence: [D267](DECOMP.md?plain=1#L2797), [D400](DECOMP.md?plain=1#L3761).

<a id="ng03"></a>

### NG03 — Add-rank and same-width conversion probes have body-specific limits

D407's measured rotation-phase residual already used rank-1 inline memory in both outcomes; all six combinations routing operands through locals were inert. D343's same-width unsigned conversion re-ranked a multi-term sum by 2 once, but was inert on equivalent sums elsewhere; original `waypoint+(cell<<8)+K` closing leas placed waypoint first in X and cell first in Y, tracking emitted definition order. These source entries do not name the measured bodies, so they are retained as anonymous, bounded observations rather than universal rules.

The same-width object barrier cannot be manufactured with two different struct TYPES cast over ONE object (D445): they value-number as one lvalue, just like union views. An empty if dies before allocation and cannot extend a temp's live range; it is a fold/block-split handle only.

Evidence: [D343](DECOMP.md?plain=1#L3325), [D407](DECOMP.md?plain=1#L3806), [D445](DECOMP.md?plain=1#L4105).

<a id="ng04"></a>

### NG04 — AnimApplyPart: float unions and no-code casts do not cure its x87 hoist

On `AnimApplyPart`, a `union { float f; int i; }` carrier for dstx/srcy remains enregistered and byte-identical, so the integer-union trap does not transfer to a float member. Explicit float cast tuples on products/numerators are also byte-identical: the FP scheduling-window trick does not reach this x87 allocation decision. The two-of-eight volatile rectangle-float probe (182 ->144 strict, 73 ->38 bad regions) is not a solution: it adds a frame slot and leaves the wrong subtraction form.

Evidence: [D427](DECOMP.md?plain=1#L3991), [D428](DECOMP.md?plain=1#L4000).

<a id="ng05"></a>

### NG05 — Anonymous copy-pair and sibling-difference experiments

The source does not name either measured body, so these are provenance-limited observations. `*(Pos*)&b->x = world;` was byte-identical to the existing load/load/store/store sequence and did not explain its schedule; that sequence alone does not prove a whole-struct copy. A same-difference rewrite that preserved one function's byte length put its sibling 17-19 bytes OVER. Retain both negatives, but remeasure any proposed cross-ride transfer against the actual original.

Evidence: [D455](DECOMP.md?plain=1#L4221).

<a id="ng06"></a>

### NG06 — Balloonz_Tick: paired stores do not prove a shared source object

**U-pipe slot choice, not aggregation, decides which of a load/store pair goes
  first.** In `Balloonz_Tick` the two stores were long assumed to be one object
  because they always emit together; sourcing the halves from two INDEPENDENT
  `.data` objects (two string literals, or two `static const int`s) is
  byte-identical, and VC6 places both stores at the same indices regardless of
  what they copy. The original simply takes the load in the first U slot. When
  two instructions always move together, test whether they are actually related
  before building a source construct to pin them.

Evidence: [D188](DECOMP.md?plain=1#L2190).

<a id="ng07"></a>

### NG07 — BNV y chains: separate source shape, register rotation, named ys, and stall filling

Direct reading supports a ONE-web Y chain in the four mechrides BNV activations: `sy2=(wx+wy)*th>>9; sy2+=cfg->oy-Get_YScroll();`, with the delta `sub eax,edx` followed by `add <sy>,eax`. X is deliberately NON-compound in the corrected family measurements. The earlier “compound both axes or nothing” prescription was a particular experiment clearing ESCAPES; it is not a universal family requirement. ~130 source variants shared the earlier result; ~380 further variants over 20 grids and eight stubbed-case builds located the rotation inside case 1. One-web sub/add rotates too: the compound token alone is not the cause.

Do not equate the files' residuals. `Carousel_Tick` (ridecb3.c) improved 27 -> 8 real mismatches with one-web Y and a first-person-read cache, but retains a three-cycle saved-register wall. The cache works through copy-propagation DISTANCE, not a scheduling barrier: it survives only with an intervening statement. Corrected mechrides.c current bodies already match the saved-register naming under the IDENTITY permutation. The repeatedly measured table is Barrels 19 -> 17, Spider 15 -> 25, Plane 19 -> 29, Safari 132 -> 131 (one-web permutation bpdibxsi). Spider/Plane each lose 10; the older summary called Safari a “tie”; the explicit table is a one-point real improvement. Thus there is no uniform family allocation win.

The completed cache 2x2: adding it to Spider/Plane is byte-identical; removing from Barrels/Safari is worse (real 17 -> 20, 131 -> 234 in the fuller note). Dropping empty if in the two-web bodies gives real 231 / 283 / 294 / 300 for Barrels / Spider / Plane / Safari (DECOMP's compressed “231 to 300”). Named `ys=Get_YScroll()` can restore all 25 register-for-register indices in a one-web chain, but flattens the later subtractions; ~400 variants found no form preserving both parts. The latest SpinningBarrels measurement resolves the wording: ONE name without ys is 1148/1148 bytes, real17/strict34; named ys is real57/strict72 and 1146 bytes (two bytes SHORT), and empty if is inert with one name. It is ys, not one-web itself, that causes this flattening.

The remaining one-web cost is a hoisted load filling a Pentium partial-register stall after `mov ax,[cfg+0x22]`, which the original leaves empty. Block splits before/after, both if values, four subtraction seams and six legal orders are identical at that cost; the Spider cells stay 25, and a volatile screen.ox read stops it but costs the frame (294 in the fuller note). Swapping which axis stores first changes halving order but hoists the SAME global: strict 26 vs 29 masks real 8 vs 11. These are scoped measured floors, not missing universal spelling rules.

The older scan citation for ONE-web shape is withdrawn: its two hits required a sub within three slots and a particular operand class; the loose search finds 171 across 1544 exact functions. The shape rests on direct reading and 2x2 measurements.

Evidence: [D363](DECOMP.md?plain=1#L3476), [D376](DECOMP.md?plain=1#L3563), [D405](DECOMP.md?plain=1#L3796), [D414](DECOMP.md?plain=1#L3867), [D420](DECOMP.md?plain=1#L3920), [D425](DECOMP.md?plain=1#L3956), [D443](DECOMP.md?plain=1#L4091), [D470](DECOMP.md?plain=1#L4340), [D471](DECOMP.md?plain=1#L4344).

<a id="ng08"></a>

### NG08 — BoatingSchool_Add and ValidateCursor recorded minima

**Exhausted, do not re-grind:** `BoatingSchool_Add` at 8 (the full 36-body
  cross-product of take-position x link-order confirms the current spelling
  is the unique minimum) and `ValidateCursor` at 5.

Evidence: [D303](DECOMP.md?plain=1#L3060).

<a id="ng09"></a>

### NG09 — BsWater_SetTile: measured latch order remained unreachable

`BsWater_SetTile` could not move inc ebp from second to fourth in its latch using twenty variants and source-order/first-use hypotheses; strict == rb == ob. Earlier corpus wording generalized that floor to latch order; subsequent `ZBuffer_RunCommand` 24-store-order evidence and `RecolourModelParts`/`PrintScreenMode7`/`RES_FindVolumeOnAnyDrive` update-order wins narrow it to this tested body and baseline. Sweep current store and increment order before transferring the negative.

Evidence: [D138](DECOMP.md?plain=1#L1675).

<a id="ng10"></a>

### NG10 — Cheap address takes, integer no-ops and inline wrappers do not manufacture webs

Measured optimized shapes: `(void)&x`, `if(&x){}`, unused int*p=&x, propagated *p, constant-index one-element arrays, same-width union members, empty/dereferencing inline helpers all leave the value enregistered; they cannot force a home. A real volatile-qualified access can. One-pointer structs behave like pointer locals. Integer casts/int-pointer casts/unsigned/+0/|0/*(&x) leave no surviving no-code tuple, unlike the measured float-product cast; they cannot move a scheduler window or advance scratch rotation. An inline call boundary is not a scheduling-region boundary: caller stores moved into helper remain identical, and padding shifts the stream without changing load/store gap. `SetBlokePositionFromBNV`'s FP tuple-window result remains separate.

Six inline zero-clear helpers (including int zero parameter/zero first) re-CSE constants into the same whole-function web before allocation. Constant carriers of all integer types, initializer/assignment placements, and extra int temps are propagated. Dead union-byte/char/byte-before-dword stores vanish before hoist and cannot buy byte class. Empty ifs can affect earlier folds/block splits but disappear before allocation and cannot extend live ranges; inline two-return helpers do not preserve two reaching definitions, and inline call wrappers forward-substitute their arguments instead of creating an uncoalesced temporary. Plain memory-reference arguments to inline helpers also forward-substitute rather than automatically becoming barrier temporaries (two functions measured).

Evidence: [D204](DECOMP.md?plain=1#L2325), [D274](DECOMP.md?plain=1#L2852), [D289](DECOMP.md?plain=1#L2970), [D347](DECOMP.md?plain=1#L3358), [D351](DECOMP.md?plain=1#L3388), [D381](DECOMP.md?plain=1#L3594), [D422](DECOMP.md?plain=1#L3936), [D433](DECOMP.md?plain=1#L4035), [D444](DECOMP.md?plain=1#L4099), [D445](DECOMP.md?plain=1#L4105).

<a id="ng11"></a>

### NG11 — CoasterCar_BuildRider: address fold cannot be separated from scheduling

`CoasterCar_BuildRider`'s 12-mismatch residual is byte-exact with strict == rb == ob. About ~30 tested C forms do not recover the original materialized address versus two-register movsx fold of `base+i*STRIDE`: named &list[i], list+i, pointer locals at either level, const short* field, char* byte arithmetic, integer addresses, casts, array-of-array rows, 2-byte elements [i*8], whole-record copy, inline helper, short index and five declaration orders. Volatile blocks folding but also schedules the AGI-fill instructions after the barrier. This measured coupling is the negative; do not universalize every two-register movsx form as unreachable.

Evidence: [D029](DECOMP.md?plain=1#L918).

<a id="ng12"></a>

### NG12 — Draw3DPersonModel frame profile: same weights, incompatible array insertion points

Scope: optimized mixed C/inline-asm `Draw3DPersonModel`, not arbitrary frames. Its original and reconstruction have the same per-slot surviving-IR profile, 384 = 384 references and 79 = 79 slots, with an identical 49-slot pool spine; four array insertion points differ by one chunk. Alias-routing every array use copy-propagates and is byte-identical; 8/16/24 dead reads disappear before counting. Thus source aliases/dead references cannot change the measured weights.

Evidence: after the three real source corrections, 981 → 687, matchfull 44.3% → 52.0%, mnemonic LCS 96%, offset-blind LCS 707 → 820. About 195 of the remaining 687 differ only by [ebp-N]. 15 scope subsets, declaration and statement order, and splitting into three objects fail; only a merged 180-byte object reproduces the order and was rejected as noncredible source. Size probes: 84B/112B at position 3, 140B/224B at position 4, never position 5. Independent linear-key falsification: mt[9]→mt[12], 36B→48B, crosses an 84B array, forcing size-k*refs to k in (0.667,0.89); no k there can put 96B/27-ref ahead of 84B/64-ref. The earlier broad 707 offset-only observations and 367 frame-offset bucket are triage history, not a different universal frame rule. The source corrections, asm-origin evidence and operand-map correction are indexed separately.

Evidence: [D209](DECOMP.md?plain=1#L2370), [D432](DECOMP.md?plain=1#L4027), [D477](DECOMP.md?plain=1#L4387).

<a id="ng13"></a>

### NG13 — Draw3DPersonModel: field-sum canonical order is not movable by respelling

An isolated `-(fr->bmax.x+fr->bmin.x)>>1` synthetic always loads the higher displacement first ([eax+12] before [eax]); reversing operands is identical. `Draw3DPersonModel`'s deviation is associated with an intervening call, and ~40 sum spellings, placements and statement splits only change it at costs 25-600. Treat the canonical order as diagnostic of an upstream perturbation; no free sum rewrite was found in this body.

Evidence: [D208](DECOMP.md?plain=1#L2362).

<a id="ng14"></a>

### NG14 — DrawPopUpInfo: residency and reload position remain coupled

`DrawPopUpInfo`: a one-use read fixes allocation but pins reload after source writes; a volatile store hoists reload but sinks CSE spill after pushes. The 13 and 16 variants are opposite sides of residency/barrier coupling, no measured free ordinary reload.

Evidence: [D349](DECOMP.md?plain=1#L3372), [D382](DECOMP.md?plain=1#L3602).

<a id="ng15"></a>

### NG15 — Empty reload-diamond arms cannot retain duplicated spill reloads

In the measured two-arm reload diamond (body unnamed in the historical entries), a NON-EMPTY else makes VC6 duplicate a spill reload into both arms. An empty arm causes dominator hoisting; `;`, n=n, identities, (void)x, dead locals, while(0), if(v){}, empty inline calls, switch(0), labels and goto forms fold before layout and cannot retain it. If(c)A, if(!c);else A and goto chains canonicalise to one two-arm layout. Identical USED volatile reads in both arms likewise hoist into the dominator; only a DEAD bare volatile read survives in each arm. Copy then increment of a second variable folds to mov r,[n]/lea d,[r+1]; only read-modify-write of the same variable yields mov d,[n]/inc d.

Evidence: [D301](DECOMP.md?plain=1#L3048), [D346](DECOMP.md?plain=1#L3352).

<a id="ng16"></a>

### NG16 — GetObjectUID: volatile colouring leaves a four-instruction barrier floor

`GetObjectUID`: volatile g_map_rows changes g_map's global saved/scratch colour, 20 → 4, not adopted because that ordinary global's qualification was unjustified elsewhere; final 4 are the volatile barrier floor. Its local pointer groupings (104) leave the direct-read/PRE-CSE incompatibility at 20.

Evidence: [D288](DECOMP.md?plain=1#L2962), [D401](DECOMP.md?plain=1#L3765).

<a id="ng17"></a>

### NG17 — Global list-head volatile cannot raise the load above pushes

At the `LFTrack_FindPiece`/`AddOpenNode` head-guard experiment, a free volatile read pins the head load BELOW both callee-saved pushes. It cannot replace the direct-global guard when the required load is ABOVE those pushes. The eleven `AddOpenNode` shapes and the one-instruction `LFTrack_FindPiece` difference are recorded with the positive rule.

Evidence: [D032](DECOMP.md?plain=1#L936).

<a id="ng18"></a>

### NG18 — GoldRush_KneelAtPan: one field value cannot retain the required call-crossing web

`GoldRush_KneelAtPan`: 95 variants cannot keep one web from load across __ftol through store and argument copy; nearest local+volatile has 58/58, right prologue/store positions, 26 mismatches, not adopted. Stores cannot cross that call, so store-after-call remains a source constraint.

Evidence: [D082](DECOMP.md?plain=1#L1258).

<a id="ng19"></a>

### NG19 — Integer casts do not provide FP-style no-code scheduling tuples

In the recorded integer scheduling measurements, `(int)`, `(unsigned)`, pointer/int casts, +0, |0 and `*(&x)` on a u16 read are byte-identical and cannot move an FP-style tuple window or scratch rotation. An inline-call boundary also did not split the scheduling region: moving caller stores inside a helper was identical, and earlier padding shifted the stream without changing a load-to-store gap. Additional integer tests found five comma-operator forms normalize to statement order, and a dead volatile read at five upstream points shifts the stream without moving the target group's store order. A may-alias store also failed to hold byte loads feeding fild. DECOMP does not name the bodies of D289/D347/D406; retain this as bounded evidence from those entries, not a universal claim that arbitrary casts or stores are inert.

Evidence: [D289](DECOMP.md?plain=1#L2970), [D347](DECOMP.md?plain=1#L3358), [D406](DECOMP.md?plain=1#L3800).

<a id="ng20"></a>

### NG20 — JcBoat_Animate and BsBoat_Animate: bare-IV product rank remains a measured floor

`JcBoat_Animate` 0x00433840 is retired at 3 of 330, 1108/1108 bytes; `BsBoat_Animate` has the same three-instruction imul-operand-rank residual at the same indices. ~270 variants established the product rank; 16 crossed product-order combinations against a register-held induction variable remain inert. Both operand orders and every zero-cost identity fold before ranking. `(short)j` reaches ONE mismatch but costs a byte: a real operation promotes the bare IV to a temporary but leaves an instruction/byte. The successful `SoftPrint_XBltFast` commuted-twin lever requires a previously LOADED value and does not reach this IV case. Do not reopen these two independently.

Evidence: [D025](DECOMP.md?plain=1#L795), [D238](DECOMP.md?plain=1#L2574), [D359](DECOMP.md?plain=1#L3441), [D393](DECOMP.md?plain=1#L3713).

<a id="ng21"></a>

### NG21 — JungleCruise_Tick and five ride activations: goto-flip does not transfer universally

`JungleCruise_Tick`'s outer goto plus all 31 nonempty subsets of five break→goto tails were byte-identical, 32 variants all 208. Its missing instruction follows an add destination, not tail-host layout. Named seat-table locals were inert; a volatile station load reaches the original index but keeps the wrong register (the original has six references). Across five ride activations, the goto-switch-join change was inert on two and harmful on three: 19 -> 58, 15 -> 134, 138 -> 145, because their tails already host correctly. The source entry names the five as a family rather than listing every body; retain this scope instead of inventing which score belongs to which activation. Use the positive lever only after checking original edge targets.

Evidence: [D415](DECOMP.md?plain=1#L3887), [D419](DECOMP.md?plain=1#L3913).

<a id="ng22"></a>

### NG22 — JungleCruise_TraceRoute: loop-free ranking with a loop

The four-int/three-pointer probe records two allocation regimes: no loop gives callee-saved registers to int parameters, while a self-tail-call converted at IR time gives them to pointers and creates a secondary IV. The original `JungleCruise_TraceRoute` shows the no-loop allocation with the converted loop; the recorded diagnosis is different tail-call phase ordering (original after allocation, local compiler before). Dead trailing statements block conversion and switch regimes, but none supplies the combined original shape. This is the measured body-specific phase-order floor. Separate TraceRoute byte-exact 105/339B versus lower-mismatch 101/340B evidence belongs to the allocation discussion; do not mistake fewer mismatches for closure.

Evidence: [D138](DECOMP.md?plain=1#L1675).

<a id="ng23"></a>

### NG23 — JungleCruise_UpdateRiverAnim (UpdateRiverAnim): the original exiled arm remained unreachable

The full function name is `JungleCruise_UpdateRiverAnim`, address 0x00432d00; older notes abbreviate it `UpdateRiverAnim`.

`UpdateRiverAnim`'s original lets one arm fall into a shared call and exiles the other beyond the fall-through trace, between two loops in one case and after ret in the other. 17 spellings collapse to three objects: goto-target form identical; duplicated calls merge onto the last arm and exile the first (mirror of original); goto back into the loop normalizes to plain if/else. Thus the recorded 81 of 112 belongs to a tested layout floor, not a universal `[then][else][merge]` law. Residual breakdown was also corrected: 44 mismatches are a three-slot shift of otherwise identical code from layout; sums account for only 31, not the earlier vague addends-plus-scheduling attribution.

Evidence: [D451](DECOMP.md?plain=1#L4150), [D452](DECOMP.md?plain=1#L4160).

<a id="ng24"></a>

### NG24 — JungleCruise_UpdateRiverAnim: the partial-sum barrier fixes order but loses overall

`JungleCruise_UpdateRiverAnim` has two measured outcomes: every flat spelling one object, every aggregate spelling another (~30 into each). The aggregate restores source addend order but rotates the whole eax/ecx/edx sequence and worsens every metric, 111 -> 306. An order correction alone is not a usable lever on this body.

Evidence: [D368](DECOMP.md?plain=1#L3511).

<a id="ng25"></a>

### NG25 — LFEntrance_Activate: the three-register lea is a live-operand issue

The ~170-variant `LFEntrance_Activate` investigation could not retain the original three-register lea by respelling the dying/coalesced operand; separate or volatile reloads still coalesced. The final 40-legal-order probe gives the same register in 40/40, regardless of load emission position. The original preamble's operand IS live into the switch because a case reads it; the lea is therefore an ordinary liveness effect. The 47-three-register-lea corpus scan had prompted a wrong “neither operand lives, fix load order” account; both claims were explicitly retracted. This floor concerns the measured inability to preserve that live value without changing the body.

Evidence: [D265](DECOMP.md?plain=1#L2785), [D348](DECOMP.md?plain=1#L3366), [D398](DECOMP.md?plain=1#L3749).

<a id="ng26"></a>

### NG26 — LoadScriptEvent: a straight-line zero return loses the original join block

`LoadScriptEvent`: L:head=0;return head as a final straight-line block becomes xor eax,eax and cross-jumps into the error handler's return0, removing the whole block (124 → 117). Return head=0, *&head=0, cast, memset, head=prev with known-zero prev, label and goto disguises all fail. The target xor<saved>,same / mov eax,<saved> needs a two-predecessor variable join with zero live before the call, not another spelling of constant propagation. The lane's remaining cold-block-order body was 124/124, mismatch 26 / 94.4%, with head=0 emitted at index95 instead of117. This is a measured zero-return construction floor, independent of layout rules that can merge ordinary returns.

Evidence: [D097](DECOMP.md?plain=1#L1334).

<a id="ng27"></a>

### NG27 — MusicThread: preheader-and-latch IAT reload placement

`MusicThread` (0x00492db0) remains 7 mismatches, 99.78%, with ours two instructions shorter. Original caches `__imp__WaitForSingleObject`/`__imp__ResetEvent` in esi/edi and reloads both TWICE, preheader and latch, because only the notification path's repe cmpsd clobbers them; ours reloads once at loop top. while(1), for(;;), trailing continue, dllimport order and volatile reads all retain the latter placement. The body is real C: 81% is two macros written thirty times each (`LOAD_SEGMENT` 30x39, `BUILD_SEGMENT` 30x46), byte-periodic at 0x80. Its 3161 instructions end at the message-loop back edge; the apparent three trailing functions are an alignment nop, five-entry jump table and next thread-starter function (CreateThread, handle later terminated by KillMusicSystem). The negative is this reload-placement residual, not the existence or C origin of the function.

Evidence: [D005](DECOMP.md?plain=1#L490).

<a id="ng28"></a>

### NG28 — Pump_SnapToRoad cannot retain the known-null retest without a new home

Negative, 46 spellings (`Pump_SnapToRoad`): **VC6 always jump-threads a
  provably-NULL pointer into a following `if (p)`.** The original's
  un-threaded form (`xor eax,eax`, fall into a join starting `xor ecx,ecx`,
  re-test a known zero, return via `mov eax,ecx`) is reachable only through a
  `volatile` local at the cost of a stack home the original does not have.
  Twin of the "global constant propagation deletes a straight-line
  `v = 0; return v;`" entry — the same pass, seen from a pointer.

Evidence: [D043](DECOMP.md?plain=1#L1004).

<a id="ng29"></a>

### NG29 — RemoveNewObjectMarker: plain-index anchor and counter schedule stay coupled

Both `spr[j-1]=spr[j]` and `spr[j]=spr[j+1]` anchor at the plain-IV `spr[j]`. Renaming to change the anchor makes the other index a derived IV, moves the copy below the guard, sinks inc to body end and changes lea to mov. Named pointers, lockstep spellings and free volatiles could not separate these choices across ~40 builds, leaving `RemoveNewObjectMarker`'s 5-mismatch residual. This does not undo the separate one-object parallel-array improvement (59i/172B -> 53i/155B); it identifies the remaining coupled decision.

Evidence: [D056](DECOMP.md?plain=1#L1086).

<a id="ng30"></a>

### NG30 — RenderCursor: five-instruction non-canonical suffix below this build’s floor

`RenderCursor` residual (b) was classified as a BUILD DIFFERENCE: the measured non-canonical merge needs 6 instructions, original shares 5, giving a 4-instruction shift and remaining 250 mismatches. A scan found 89 sites across 24 matched functions, shared depths `5:1, 7:16, 9:10, 11:1, 13:37, 15:1, 16:2, 18:8, 27:4`. The sole depth-5 site is this function's original; none has depth 6, and this build's 6 floor is already below the other corpus minimum 7. Inside this switch goto join and break are literally the same instruction; eleven spellings are byte-identical, so the outer-goto asymmetry lever is unavailable. This is retirement-grade evidence for the measured suffix, not a universal ban on five-instruction merges (canonical predecessor threshold is 4).

Evidence: [D313](DECOMP.md?plain=1#L3127), [D421](DECOMP.md?plain=1#L3927).

<a id="ng31"></a>

### NG31 — RenderFullMap and RenderView: measured source reshaping does not select the needed edge

`RenderFullMap`'s trace model isolates a 303-slot residual to one branch: original then falls through, else is sunk past three later blocks and jumps BACK into the middle of a two-instruction pair. Eleven selections (hoisted pointer, inverted arms, empty else, early break, duplicated test, two-case switch, two gotos, inline filler, ternary/comma) all emit `[test][arm1][jmp join][arm2][join]`. Full-tail duplication sinks the block, but copies never merge because a just-stored field forwards into else tests; the original join really is shared. This is the failed edge-selection search, not a failure of the LIFO layout model itself.

For `RenderView`, goto-to-end for else, goto-to-end for a 239-instruction arm and full inversion all produced byte-identical objects. The general lesson is that text relocation alone need not change normalized CFG; allocation or an actually different branch shape is required in these measured cases. Later combined corrections in `RenderView` regressed 381 -> 549, and `RenderFullMap`'s other forced register facts cost 873-1066 despite its isolated ILFTable latch improvement 844 -> 827; the whole bodies remain floors as recorded.

Evidence: [D007](DECOMP.md?plain=1#L519), [D438](DECOMP.md?plain=1#L4055), [D461](DECOMP.md?plain=1#L4264).

<a id="ng32"></a>

### NG32 — RenderView split prologue is a saved-register allocation-order problem

The measured RenderView variants decide which TWO saved pushes fit the entry fill holes and which pair come later. Original and broken forms do not share register assignment: two values swap ebx/ebp. The constants keep their register identities; the finer change is whether one value coalesces with the constant-zero web. Thus both earlier theories—geometry inherently demands esi/edi, or push sinking with identical assignment—are withdrawn. Track all four push indices AND which value each register holds.

Ruled out: 23 geometry orders, post-guard inner scope (byte-identical), eleven respellings atop the best order, all 15 interleavings of one field fill. The standing instruction to adopt a geometry order once the +10 region is fixed is retired: moving both limits removes the compensating +10/-11 pair but breaks the same prologue decision. A duplicated null test on an address-taken struct is NOT folded as the older note claimed. Deferred-push positive rules remain valid in their own bodies.

Evidence: [D439](DECOMP.md?plain=1#L4061), [D485](DECOMP.md?plain=1#L4450).

<a id="ng33"></a>

### NG33 — RequestRoute and InsertChildIntoList have independent uncoalesced-copy floors

An uncoalesced copy normally requires a multiple-definition join (phi) or interference. `RenderAdvisorIcon` 0x443e8a proves the phi case: source dies after the copy but destination has two reaching definitions. Four corpus mov rA,rB / mov[esp+d],rA sites show interference. `RequestRoute`'s divergence has neither (one predecessor, no later source use); `InsertChildIntoList` independently has neither (0x4756aa not a target; eax defined 0x47569d, read twice, then killed by call, no competitor). They are independent retirement evidence, not one family.

The original scan found these two dead rematerialisable-value copy sites and zero among 1542 exact bodies, but structural inspection supersedes the family claim. Ninth site `AddRepairOrderForObject` 0x0049b977 is a separate survive-call class; a genuine post-call-use probe emits no copy. `RequestRoute`'s alias-store alternatives either retain the earlier web or reload memory, never the copy; its measured short/long web pair always hands eax to the short and ecx to long. The target combines incompatible pairing/lifetime in the tested shape, explaining its 3-mismatch floor without endless spelling enumeration.

Evidence: [D193](DECOMP.md?plain=1#L2227), [D194](DECOMP.md?plain=1#L2238), [D362](DECOMP.md?plain=1#L3460), [D391](DECOMP.md?plain=1#L3699).

<a id="ng34"></a>

### NG34 — Route_StepFree / Route_StepToPieceEnd: snapshot base never materialises

At these TWO sites, the original's shared lea base for a three-scalar snapshot group was not reached. Pointer, array, walking-cursor, whole-struct-copy and volatile spellings all fold back into `esi + disp`. The negative is limited to those two bodies and spellings; it does not contradict struct-copy materialisation at different sizes or call sites.

Evidence: [D050](DECOMP.md?plain=1#L1046).

<a id="ng35"></a>

### NG35 — SchoolCarBlockedAhead: mixed-axis schedule has two incompatible outcomes

On `SchoolCarBlockedAhead`, the target is ux-chain first but y-difference first. All 40 legal interleavings give one of two schedules selected by which axis is written first; a barrier that mixes them re-ranks the affected register. This is the measured body-specific mixed-axis floor, not a ban on source-order scheduling elsewhere.

Evidence: [D119](DECOMP.md?plain=1#L1544).

<a id="ng36"></a>

### NG36 — SpinningBarrels_Activate: an empty if inserted at another seam is harmful

On `SpinningBarrels_Activate`, the original empty-if seam is byte-identical; at the other four seams a neg appears and tail reassociation costs 227-242. The fuller note measures 16 cells (five seams, three guard values, plus do/while(0)). This does not supply a generic arithmetic-block or reassociation barrier.

Evidence: [D205](DECOMP.md?plain=1#L2330).

<a id="ng37"></a>

### NG37 — TempleSlide_Update: the additional X web is not an almost-finished solution

The proposed “once sq outranks ty, two-web X is probably finished” condition was achieved, then all seven forms were rerun across every read/call placement: they still score 327 and ESCAPE. The extra web supplies another independent cause of the tie-break. The accompanying two-term partial-sum Pos barrier is withdrawn; four plain -= statements are better on every metric.

Evidence: [D476](DECOMP.md?plain=1#L4377).

<a id="ng38"></a>

### NG38 — The activation-family window is a missing register choice, not five scheduling holes

`SpinningBarrels_Activate`: remove p=b->person cache and the window matches at strict 25 / rb 3, indices 120-137 matching original 121-138 under one-slot shift for missing mov ecx,[esi+4]. The original screen.ox temp takes EAX just freed by halving and cannot hoist; ours takes EDX because p occupies ECX. Cache removal cannot be adopted alone because person then sinks below escaped pos stores. No tested spelling gives both p at 118 and serial screen.ox; original leaves ECX idle from cfg->oy through person reload. `SafariRide_Activate`: strict 132 / rb 15 is one register p=ECX instead of original EDX at 103; from 113 to case end instructions agree under the scratch three-cycle.

This supersedes the earlier five-window scheduling account. The observable X-slot empty/Y-slot filled pattern and 6-12-index late constant pushes are consequences of allocation. About 60 spellings were identical (flat tails, halves, Offset*, all interleavings, cache at three points, adjacent globals as one object). The proposed cache cannot raise the required local pressure without another effect. Unconfounded tests must keep the folded-store form `pos.x=(sx2-h/2-screen.ox)*2`: moving flag/sprite/cache earlier otherwise reassociates four tail subtractions, inserts mov/sar/neg/sub/add and changes frame 0x3c → 0x38, so historical 330-380 scores measured that confound. Empty if does not replace the barrier; folded-store control is byte-identical and immune, and the floor survives it.

Evidence: [D203](DECOMP.md?plain=1#L2315), [D454](DECOMP.md?plain=1#L4186), [D469](DECOMP.md?plain=1#L4330).

<a id="ng39"></a>

### NG39 — Three ride projections: ascending short loads and x-first stores remain incompatible

`Carousel_Tick` 228-231, `SpiderRide_Activate` 195-198 and `PlaneRide_Activate` 198-201 share a TWO-load residual. The CORRECT target is ASCENDING original loads (+0x3c then +0x3e) with x-store first; ours descend. Final registers/stores agree. 20 further spellings against that corrected target still fail. A complete 16-cell grid {declaration order} x {temp carrying *2} x {store order} proves loads always reverse stores; declaration order is inert (each cell equals its twin). Doubling selection depends on WHICH temp carries *2, not which loads first: y carrying it makes both shl and removes Plane's extra byte (3-byte lea vs2-byte shl) but interchanges the component pairs. Only `*(volatile int*)&pos2.x=...` gets ascending loads and it pins the store.

Keep the partial positive separate: read both adjacent shorts into int px/py, then double at stores to group movsx/movsx ahead of shifts; per-component statements interleave load/shift/store, and pre-doubled temps duplicate tails and ESCAPE. A separate four-statement shift block has one exact order of 24, worth 19 and 7 in the recorded measurements.

Evidence: [D337](DECOMP.md?plain=1#L3286), [D364](DECOMP.md?plain=1#L3491), [D453](DECOMP.md?plain=1#L4165).

<a id="ng40"></a>

### NG40 — TrackCurve_GatherParams: dead loop-exit count reload

`TrackCurve_GatherParams` (0x00422000) lacks original dead `mov eax,[0x004dd650]` at the inner loop exit. Twelve sort spellings, inline helper with/without count and a free volatile count read did not make the hoisted global live where it has no consumer. Expanded marker evidence: 69/70 instructions, 206/211 bytes, strict 6, register-blind 5, first 64 instructions exact (indices 0-63); the final five merely shift. Both loop directions, subscript/pointer, k>0 versus k<i, pass bound `g_tc_n-1-pass` (74 instructions), mirrored rereads top/bottom, `for(p=out;p<out+i;p++)`, both swap orders and volatile were measured; nearest alternatives strict 8 and 10. The original jle requires the up-counted inner loop and its store order requires subscripts. Source: D119 plus `LEGOLAND/schoolcar4.c` marker and scope G. This is a dead-reload floor on this body, not all loop reloads.

Evidence: [D119](DECOMP.md?plain=1#L1544).

<a id="ng41"></a>

### NG41 — UnlinkGardenerOrder: identical tails still merge onto the wrong arm

`UnlinkGardenerOrder` remains 34 of 68, structural layout: original is `[head arm][START printf+epilogue][ret][search/notfound/found]`, found jumping BACK; ours is `[head arm][search/notfound/found][START printf+epilogue]`. Making both tail copies instruction-identical with paired volatile reads enables merging but keeps the later search-arm copy. Swapping the arms flips the original `cmp dword [0x79a8b0],edi / jne` and diverges at index 23. 17 lane spellings cover failure-if versus else, goto failure/search/join, goto inside loop, outer nesting, do/while(0), swapped arms, duplicated tails with/without volatile (77 versus 68 instructions), and partial volatile variants; all hit the two wrong layouts. The later corpus reconciliation prevents treating this as a universal LAST-arm rule; it remains this body's measured floor. Source: D097 and `docs/lanes/fable-b.md`.

Evidence: [D097](DECOMP.md?plain=1#L1334).

<a id="ng42"></a>

### NG42 — Unnamed switch witness: measured CFG and call respellings were inert

The corpus's D316 does not name its body, so its evidence is limited to that recorded witness: every tested two-arm-around-switch reshape (goto either way, do/break/while(0), continue, negated condition, if/else-if), all case permutations, same-width argument casts and inline call wrappers were inert, and shifting upstream instructions did not move the cross-jump decision. `case K: f(kk,...)` where kk is the switch value is byte-identical to `f(K,...)` through constant propagation. This entry does not identify enough provenance to name a function or generalize inertness to switches that have positive case-order evidence.

Evidence: [D316](DECOMP.md?plain=1#L3154).

<a id="ng43"></a>

### NG43 — UpdateControllerFromMouseData remains deliberately abandoned

HANDOFF records `UpdateControllerFromMouseData` (0x00473b00, input.c) at 102/109. Two agents exhausted it and checked the PE Rich header to confirm the same compiler backend. Their note rules out tested C constructs and `/O2`-compatible options; this is a body-specific allocator floor, not a claim that all controller code is unreachable. Leave it under the contract's exhausted-body rule.

Evidence: [HANDOFF §3](HANDOFF.md#3-rules-that-are-not-in-the-code), [§6B](HANDOFF.md#6-where-to-go-next) and [parallel contract](PARALLEL_CONTRACT.md).

<a id="ng44"></a>

### NG44 — UpdateRiverTile: one web versus a split web leaves a sunk push

`UpdateRiverTile`'s honest body has register-blind distance 3 against strict 67. Its residual is one sunk push ebx: the original keeps one value web rematerialised on the loop back edge, while the reconstruction splits it. The low register-blind distance identifies this limited allocation difference; it does not establish a universal source impossibility.

Evidence: [D437](DECOMP.md?plain=1#L4050).

<a id="ng45"></a>

### NG45 — UpdateSampleSource: volatile input order conflicts with its shared tail

`UpdateSampleSource`: forced cast stores in every switch arm make the shared ten-instruction tail reload p.x/p.y and cross-jump to one tail, 85 → 66/66 with arm/default layout; Pos by-value, inline Pos* reader and explicit goto/default join all stay 85. The remaining case-3 pos.y volatile read emits FIRST in the block rather than original third (scroll_x/pos.x/pos.y); four positions/extra-local spellings all leave 6, without the read costs 12. Volatile scroll_x and/or pos.x break the tail merge, 72 instructions. Latest scope-i: 6/66 strict, rb 3, ob 6, first 46, 175/175 bytes; ten more ordered/snapshot/aggregate forms; three ordered volatile reads give input order but lose y=eax and case-1 merge (28), ordinary pair/scroll/accumulator alternatives 27-32.

Evidence: [D097](DECOMP.md?plain=1#L1334).

<a id="ng46"></a>

### NG46 — ValidateCursor: escaped-store reload remains after helper improvement

`ValidateCursor`, about 1760 variants: even an immediate escaped-caller-local store (`bound->left=0`) between reads of cur->origin.y forces a reload. Helper-born locals are exempt and improve 14 → 5, but the final 5 (aligned distance 2, one displaced instruction) remain the measured floor, alongside `UpdateControllerFromMouseData`. The original keeps one load across that store; if this alias rule applies, the intervening store was not at that point in source and was displaced there later. The later explicit-scalar-preload counterexample narrows any blanket no-source-order conclusion: it can hoist loads above escaped stores, but was not a complete solution of this recorded body.

Evidence: [D259](DECOMP.md?plain=1#L2735), [D286](DECOMP.md?plain=1#L2948), [D418](DECOMP.md?plain=1#L3910).

<a id="ng47"></a>

### NG47 — ZBuffer_FillPoly: row and ylast dead-argument homes remain swapped

`ZBuffer_FillPoly`: row/ylast exchanged dead-argument homes survive 135 statement orders, 11 declaration orders, six volatile reads.

Evidence: [D050](DECOMP.md?plain=1#L1046).

## Correction register

The rule entries above carry the controlling evidence and the historical measurements. This register records the rejected generalization or conflicting report; it adds no second copy of the rule.

| Resolution | Rule and evidence |
| --- | --- |
| Earlier then-arm language described the measured fall-through survivor, not a universal source-position rule. | [BL01](#bl01); [D198](DECOMP.md?plain=1#L2271), [D461](DECOMP.md?plain=1#L4264), [D481](DECOMP.md?plain=1#L4415) |
| Earlier FIRST/LAST/THEN source-position prescriptions conflict; explicit later reconciliation uses fall-through, retaining separately measured phase-specific behavior. | [BL02](#bl02); [D138](DECOMP.md?plain=1#L1675), [D263](DECOMP.md?plain=1#L2766), [D279](DECOMP.md?plain=1#L2895), [D295](DECOMP.md?plain=1#L3006), [D296](DECOMP.md?plain=1#L3013), [D481](DECOMP.md?plain=1#L4415) |
| Earlier final-label pinning advice was narrowed by PlayMovie multiple-predecessor evidence; nesting is the demonstrated handle there. | [BL07](#bl07); [D013](DECOMP.md?plain=1#L607), [D061](DECOMP.md?plain=1#L1117), [D156](DECOMP.md?plain=1#L1922), [D163](DECOMP.md?plain=1#L1977), [D175](DECOMP.md?plain=1#L2075), [D176](DECOMP.md?plain=1#L2080) |
| Earlier universal FIRST return-site wording is restricted by explicit target existence, source predecessor count, and measured layout. | [BL08](#bl08); [D013](DECOMP.md?plain=1#L607), [D026](DECOMP.md?plain=1#L802), [D175](DECOMP.md?plain=1#L2075), [D355](DECOMP.md?plain=1#L3422) |
| Earlier all-switch source-order claim is restricted to jump tables by BuildPTPRoute all-six-order evidence; DrawPathTileOverlay reverse-order report is its measured 1,2,3 arrangement. | [BL10](#bl10); [D013](DECOMP.md?plain=1#L607), [D026](DECOMP.md?plain=1#L802), [D054](DECOMP.md?plain=1#L1073), [D094](DECOMP.md?plain=1#L1323), [D179](DECOMP.md?plain=1#L2103), [D291](DECOMP.md?plain=1#L2984), [D487](DECOMP.md?plain=1#L4466) |
| Earlier should-transfer-to-any-body goto-flip wording is limited by the documented body-specific negatives and observed opposite answers. | [BL12](#bl12); [D161](DECOMP.md?plain=1#L1963), [D304](DECOMP.md?plain=1#L3064), [D403](DECOMP.md?plain=1#L3779) |
| Earlier unreachable add-negative claim was disproved by up-counted trip transformation and 75 binary sites, including one already exact. | [LP02](#lp02); [D181](DECOMP.md?plain=1#L2117), [D182](DECOMP.md?plain=1#L2124), [D351](DECOMP.md?plain=1#L3388), [D387](DECOMP.md?plain=1#L3648) |
| Earlier latch-order floor generalized from BsWater_SetTile; ZBuffer_RunCommand and three increment-clause wins prove reachable cases. | [LP06](#lp06); [D013](DECOMP.md?plain=1#L607), [D026](DECOMP.md?plain=1#L802), [D138](DECOMP.md?plain=1#L1675), [D370](DECOMP.md?plain=1#L3523) |
| Earlier no-source-shape elimination claim was based only on loop forms; matched cursor-spelling evidence supplies a reachable route. | [LP07](#lp07); [D009](DECOMP.md?plain=1#L577), [D098](DECOMP.md?plain=1#L1435), [D146](DECOMP.md?plain=1#L1845), [D196](DECOMP.md?plain=1#L2251) |
| Earlier midpoint and blanket ascending-last/descending-first readings are replaced by measured reference-count/access-form scope. | [LP08](#lp08); [D013](DECOMP.md?plain=1#L607), [D033](DECOMP.md?plain=1#L946), [D071](DECOMP.md?plain=1#L1183), [D097](DECOMP.md?plain=1#L1334), [D121](DECOMP.md?plain=1#L1561) |
| Earlier unconditional no-join formulation is refined to source-level pending-depth agreement; late cross-jumps preserve matching depths. | [CC02](#cc02); [D001](DECOMP.md?plain=1#L427), [D139](DECOMP.md?plain=1#L1804), [D260](DECOMP.md?plain=1#L2741), [D305](DECOMP.md?plain=1#L3073) |
| Expanded InitScreen8 note says four but enumerates six calls; preserve the six explicit calls and their 0x40 total. | [CC03](#cc03); [D013](DECOMP.md?plain=1#L607), [D016](DECOMP.md?plain=1#L727), [D041](DECOMP.md?plain=1#L993), [D138](DECOMP.md?plain=1#L1675) |
| D138 right-to-left evaluation versus Y-last/X-flags explanation is internally inconsistent; retained observable branch and 52/76 -> 78/78 evidence without that causal claim. | [CC05](#cc05); [D138](DECOMP.md?plain=1#L1675), [D165](DECOMP.md?plain=1#L1985), [D166](DECOMP.md?plain=1#L1990), [D297](DECOMP.md?plain=1#L3022), [D324](DECOMP.md?plain=1#L3205), [D404](DECOMP.md?plain=1#L3789), [D428](DECOMP.md?plain=1#L4000) |
| Earlier general latch-order unreachability is retained only as BsWater_SetTile twenty-variant evidence. | [NG09](#ng09); [D138](DECOMP.md?plain=1#L1675) |
| Earlier lane interpretation said universal LAST-arm merge; later corpus reconciliation keeps this body-specific observation while using fall-through generally. | [NG41](#ng41); [D097](DECOMP.md?plain=1#L1334) |
| Earlier universal layout claim is narrowed to UpdateRiverAnim’s seventeen measured spellings; residual breakdown corrected to layout44 and sums31. | [NG23](#ng23); [D451](DECOMP.md?plain=1#L4150), [D452](DECOMP.md?plain=1#L4160) |
| An inert free volatile is evidence, not proof, of an allocation floor; an extra real IR temporary can still fix it. | [RA01](#ra01); [D018](DECOMP.md?plain=1#L741), [D026](DECOMP.md?plain=1#L802), [D067](DECOMP.md?plain=1#L1157), [D080](DECOMP.md?plain=1#L1243), [D097](DECOMP.md?plain=1#L1334), [D127](DECOMP.md?plain=1#L1601), [D157](DECOMP.md?plain=1#L1930), [D173](DECOMP.md?plain=1#L2064), [D274](DECOMP.md?plain=1#L2852), [D464](DECOMP.md?plain=1#L4308) |
| Twins can diverge at different register pressure; do not universalize index-for-index residuals. | [RA01](#ra01); [D018](DECOMP.md?plain=1#L741), [D026](DECOMP.md?plain=1#L802), [D067](DECOMP.md?plain=1#L1157), [D080](DECOMP.md?plain=1#L1243), [D097](DECOMP.md?plain=1#L1334), [D127](DECOMP.md?plain=1#L1601), [D157](DECOMP.md?plain=1#L1930), [D173](DECOMP.md?plain=1#L2064), [D274](DECOMP.md?plain=1#L2852), [D464](DECOMP.md?plain=1#L4308) |
| Replaces the universal four-use claim with measured loop weighting; keeps the 3/5-copy-placement witness scoped to BoatingSchool_Update. | [RA08](#ra08); [D007](DECOMP.md?plain=1#L519), [D013](DECOMP.md?plain=1#L607), [D026](DECOMP.md?plain=1#L802), [D063](DECOMP.md?plain=1#L1127), [D083](DECOMP.md?plain=1#L1266), [D328](DECOMP.md?plain=1#L3229), [D390](DECOMP.md?plain=1#L3680), [D422](DECOMP.md?plain=1#L3936) |
| The earlier scan finding no sole saved-zero carrier was over-restrictive: it required no register redefinition. | [RA08](#ra08); [D007](DECOMP.md?plain=1#L519), [D013](DECOMP.md?plain=1#L607), [D026](DECOMP.md?plain=1#L802), [D063](DECOMP.md?plain=1#L1127), [D083](DECOMP.md?plain=1#L1266), [D328](DECOMP.md?plain=1#L3229), [D390](DECOMP.md?plain=1#L3680), [D422](DECOMP.md?plain=1#L3936) |
| Early eliminated stores cannot create a constant web or buy a byte register class. | [RA08](#ra08); [D007](DECOMP.md?plain=1#L519), [D013](DECOMP.md?plain=1#L607), [D026](DECOMP.md?plain=1#L802), [D063](DECOMP.md?plain=1#L1127), [D083](DECOMP.md?plain=1#L1266), [D328](DECOMP.md?plain=1#L3229), [D390](DECOMP.md?plain=1#L3680), [D422](DECOMP.md?plain=1#L3936) |
| The D310 two-register-use and D360 single-materialisation statements cannot both be global rules; preserve D389 controlled compare/register/store cases and D360 occupied-eax probe. | [RA10](#ra10); [D195](DECOMP.md?plain=1#L2245), [D256](DECOMP.md?plain=1#L2707), [D310](DECOMP.md?plain=1#L3108), [D360](DECOMP.md?plain=1#L3448), [D389](DECOMP.md?plain=1#L3672) |
| Corrected inert-volatile diagnostic: an extra IR temporary may still advance rotation. | [RA11](#ra11); [D003](DECOMP.md?plain=1#L475), [D018](DECOMP.md?plain=1#L741), [D072](DECOMP.md?plain=1#L1192), [D091](DECOMP.md?plain=1#L1314), [D104](DECOMP.md?plain=1#L1465), [D120](DECOMP.md?plain=1#L1553), [D138](DECOMP.md?plain=1#L1675), [D170](DECOMP.md?plain=1#L2037), [D190](DECOMP.md?plain=1#L2204), [D300](DECOMP.md?plain=1#L3038), [D381](DECOMP.md?plain=1#L3594) |
| Volatile declaration alone is inert only when every access discards its qualifier; ordinary qualified accesses still act as barriers. | [RA11](#ra11); [D003](DECOMP.md?plain=1#L475), [D018](DECOMP.md?plain=1#L741), [D072](DECOMP.md?plain=1#L1192), [D091](DECOMP.md?plain=1#L1314), [D104](DECOMP.md?plain=1#L1465), [D120](DECOMP.md?plain=1#L1553), [D138](DECOMP.md?plain=1#L1675), [D170](DECOMP.md?plain=1#L2037), [D190](DECOMP.md?plain=1#L2204), [D300](DECOMP.md?plain=1#L3038), [D381](DECOMP.md?plain=1#L3594) |
| Never-hoists is too broad: no crossing stores/pushes is the invariant, with UpdateSampleSource a measured block-leading read. | [RA11](#ra11); [D003](DECOMP.md?plain=1#L475), [D018](DECOMP.md?plain=1#L741), [D072](DECOMP.md?plain=1#L1192), [D091](DECOMP.md?plain=1#L1314), [D104](DECOMP.md?plain=1#L1465), [D120](DECOMP.md?plain=1#L1553), [D138](DECOMP.md?plain=1#L1675), [D170](DECOMP.md?plain=1#L2037), [D190](DECOMP.md?plain=1#L2204), [D300](DECOMP.md?plain=1#L3038), [D381](DECOMP.md?plain=1#L3594) |
| An explicit int preload can hoist above escaped stores; the blanket no-source-order claim was too broad. | [RA13](#ra13); [D184](DECOMP.md?plain=1#L2142), [D193](DECOMP.md?plain=1#L2227), [D239](DECOMP.md?plain=1#L2590), [D250](DECOMP.md?plain=1#L2681), [D259](DECOMP.md?plain=1#L2735), [D266](DECOMP.md?plain=1#L2791), [D275](DECOMP.md?plain=1#L2862), [D276](DECOMP.md?plain=1#L2875), [D286](DECOMP.md?plain=1#L2948), [D292](DECOMP.md?plain=1#L2993), [D418](DECOMP.md?plain=1#L3910), [D435](DECOMP.md?plain=1#L4042), [D459](DECOMP.md?plain=1#L4253), [D480](DECOMP.md?plain=1#L4411) |
| Helper-born addressed locals are not equivalent to function-level escaped locals. | [RA13](#ra13); [D184](DECOMP.md?plain=1#L2142), [D193](DECOMP.md?plain=1#L2227), [D239](DECOMP.md?plain=1#L2590), [D250](DECOMP.md?plain=1#L2681), [D259](DECOMP.md?plain=1#L2735), [D266](DECOMP.md?plain=1#L2791), [D275](DECOMP.md?plain=1#L2862), [D276](DECOMP.md?plain=1#L2875), [D286](DECOMP.md?plain=1#L2948), [D292](DECOMP.md?plain=1#L2993), [D418](DECOMP.md?plain=1#L3910), [D435](DECOMP.md?plain=1#L4042), [D459](DECOMP.md?plain=1#L4253), [D480](DECOMP.md?plain=1#L4411) |
| Scope-depth equivalence at one baseline later differed by three. | [RA13](#ra13); [D184](DECOMP.md?plain=1#L2142), [D193](DECOMP.md?plain=1#L2227), [D239](DECOMP.md?plain=1#L2590), [D250](DECOMP.md?plain=1#L2681), [D259](DECOMP.md?plain=1#L2735), [D266](DECOMP.md?plain=1#L2791), [D275](DECOMP.md?plain=1#L2862), [D276](DECOMP.md?plain=1#L2875), [D286](DECOMP.md?plain=1#L2948), [D292](DECOMP.md?plain=1#L2993), [D418](DECOMP.md?plain=1#L3910), [D435](DECOMP.md?plain=1#L4042), [D459](DECOMP.md?plain=1#L4253), [D480](DECOMP.md?plain=1#L4411) |
| Retracts the cache-as-alias-barrier explanation; measured mechanism is copy-propagation distance and register consumption. | [RA14](#ra14); [D203](DECOMP.md?plain=1#L2315), [D297](DECOMP.md?plain=1#L3022), [D335](DECOMP.md?plain=1#L3273), [D404](DECOMP.md?plain=1#L3789) |
| The older forced/unreachable byte ranking is reachable with one shared aggregate at LFTunnel_Place, but LFDrop_Place remains outside the measured successful scope. | [RA15](#ra15); [D168](DECOMP.md?plain=1#L2002), [D183](DECOMP.md?plain=1#L2132), [D352](DECOMP.md?plain=1#L3400) |
| Resolves flattened-is-inert versus placement: scalarisation controls accesses, aggregate status can still control placement/allocation. | [FR01](#fr01); [D055](DECOMP.md?plain=1#L1080), [D129](DECOMP.md?plain=1#L1621), [D168](DECOMP.md?plain=1#L2002), [D171](DECOMP.md?plain=1#L2046), [D187](DECOMP.md?plain=1#L2181), [D381](DECOMP.md?plain=1#L3594), [D444](DECOMP.md?plain=1#L4099), [D489](DECOMP.md?plain=1#L4471), [D490](DECOMP.md?plain=1#L4473) |
| Relative-offset pinning requires escape; the historical union-alone claim was overbroad. | [FR01](#fr01); [D055](DECOMP.md?plain=1#L1080), [D129](DECOMP.md?plain=1#L1621), [D168](DECOMP.md?plain=1#L2002), [D171](DECOMP.md?plain=1#L2046), [D187](DECOMP.md?plain=1#L2181), [D381](DECOMP.md?plain=1#L3594), [D444](DECOMP.md?plain=1#L4099), [D489](DECOMP.md?plain=1#L4471), [D490](DECOMP.md?plain=1#L4473) |
| Disjoint lexical depth does not forbid reuse; a live enclosing object does. | [FR04](#fr04); [D141](DECOMP.md?plain=1#L1817), [D214](DECOMP.md?plain=1#L2408), [D267](DECOMP.md?plain=1#L2797), [D274](DECOMP.md?plain=1#L2852), [D277](DECOMP.md?plain=1#L2879), [D284](DECOMP.md?plain=1#L2935), [D327](DECOMP.md?plain=1#L3223), [D341](DECOMP.md?plain=1#L3312), [D475](DECOMP.md?plain=1#L4371), [D478](DECOMP.md?plain=1#L4399), [D488](DECOMP.md?plain=1#L4469), [D491](DECOMP.md?plain=1#L4483) |
| Preserve SaveGame addressed-local declaration-order observation as its own class, not a universal /O2 spill-order law. | [FR04](#fr04); [D141](DECOMP.md?plain=1#L1817), [D214](DECOMP.md?plain=1#L2408), [D267](DECOMP.md?plain=1#L2797), [D274](DECOMP.md?plain=1#L2852), [D277](DECOMP.md?plain=1#L2879), [D284](DECOMP.md?plain=1#L2935), [D327](DECOMP.md?plain=1#L3223), [D341](DECOMP.md?plain=1#L3312), [D475](DECOMP.md?plain=1#L4371), [D478](DECOMP.md?plain=1#L4399), [D488](DECOMP.md?plain=1#L4469), [D491](DECOMP.md?plain=1#L4483) |
| Initializers are scope-entry IR work, not universally stuck at function entry; inner scope, loops and guards narrow the older prologue claim. | [FR06](#fr06); [D097](DECOMP.md?plain=1#L1334), [D147](DECOMP.md?plain=1#L1853), [D162](DECOMP.md?plain=1#L1971), [D240](DECOMP.md?plain=1#L2596), [D251](DECOMP.md?plain=1#L2685), [D259](DECOMP.md?plain=1#L2735), [D287](DECOMP.md?plain=1#L2957), [D329](DECOMP.md?plain=1#L3240), [D361](DECOMP.md?plain=1#L3455), [D411](DECOMP.md?plain=1#L3848) |
| Supersedes family-wide unknown with two measured spill tools and separate alignment-hole explanation; dead plain stores cannot reserve homes. | [FR07](#fr07); [D264](DECOMP.md?plain=1#L2778), [D268](DECOMP.md?plain=1#L2803), [D280](DECOMP.md?plain=1#L2912), [D294](DECOMP.md?plain=1#L3002), [D340](DECOMP.md?plain=1#L3305), [D358](DECOMP.md?plain=1#L3437), [D377](DECOMP.md?plain=1#L3573), [D394](DECOMP.md?plain=1#L3721), [D423](DECOMP.md?plain=1#L3940) |
| Asm presence, not its location/count, reverses array order; references in asm lea have zero weight. | [FR10](#fr10); [D244](DECOMP.md?plain=1#L2614), [D409](DECOMP.md?plain=1#L3816), [D431](DECOMP.md?plain=1#L4024) |
| Block-scope effect is not a universal one-step move; function-level optimizer-off name ranking is a separate regime. | [FR10](#fr10); [D244](DECOMP.md?plain=1#L2614), [D409](DECOMP.md?plain=1#L3816), [D431](DECOMP.md?plain=1#L4024) |
| Frame weights are surviving-IR reference counts; aliases and dead reads cannot perturb them. | [NG12](#ng12); [D209](DECOMP.md?plain=1#L2370), [D432](DECOMP.md?plain=1#L4027), [D477](DECOMP.md?plain=1#L4387) |
| Retires this measured frame-profile search, not all aggregate layout changes. | [NG12](#ng12); [D209](DECOMP.md?plain=1#L2370), [D432](DECOMP.md?plain=1#L4027), [D477](DECOMP.md?plain=1#L4387) |
| Replaces claims that union syntax/address-taking alone forces residency; escape must survive optimization. | [NG10](#ng10); [D204](DECOMP.md?plain=1#L2325), [D274](DECOMP.md?plain=1#L2852), [D289](DECOMP.md?plain=1#L2970), [D347](DECOMP.md?plain=1#L3358), [D351](DECOMP.md?plain=1#L3388), [D381](DECOMP.md?plain=1#L3594), [D422](DECOMP.md?plain=1#L3936), [D433](DECOMP.md?plain=1#L4035), [D444](DECOMP.md?plain=1#L4099), [D445](DECOMP.md?plain=1#L4105) |
| No-code float tuples do not imply integer casts or inline-call boundaries can move scheduler regions. | [NG10](#ng10); [D204](DECOMP.md?plain=1#L2325), [D274](DECOMP.md?plain=1#L2852), [D289](DECOMP.md?plain=1#L2970), [D347](DECOMP.md?plain=1#L3358), [D351](DECOMP.md?plain=1#L3388), [D381](DECOMP.md?plain=1#L3594), [D422](DECOMP.md?plain=1#L3936), [D433](DECOMP.md?plain=1#L4035), [D444](DECOMP.md?plain=1#L4099), [D445](DECOMP.md?plain=1#L4105) |
| The claimed RequestRoute/InsertChildIntoList family was superseded by independent phi/interference inspections; neither site has the required mechanism. | [NG33](#ng33); [D193](DECOMP.md?plain=1#L2227), [D194](DECOMP.md?plain=1#L2238), [D362](DECOMP.md?plain=1#L3460), [D391](DECOMP.md?plain=1#L3699) |
| The LFEntrance_Activate load-order and non-liveness explanations were both retracted after 40-order probe and live-in read inspection. | [NG01](#ng01); [D265](DECOMP.md?plain=1#L2785), [D348](DECOMP.md?plain=1#L3366), [D379](DECOMP.md?plain=1#L3585), [D398](DECOMP.md?plain=1#L3749), [D407](DECOMP.md?plain=1#L3806), [D415](DECOMP.md?plain=1#L3887), [D416](DECOMP.md?plain=1#L3899), [D450](DECOMP.md?plain=1#L4145), [D472](DECOMP.md?plain=1#L4347) |
| SpaceTower strict 149→138 contains eleven renaming points; controlled metric says 138→136 real. | [NG01](#ng01); [D265](DECOMP.md?plain=1#L2785), [D348](DECOMP.md?plain=1#L3366), [D379](DECOMP.md?plain=1#L3585), [D398](DECOMP.md?plain=1#L3749), [D407](DECOMP.md?plain=1#L3806), [D415](DECOMP.md?plain=1#L3887), [D416](DECOMP.md?plain=1#L3899), [D450](DECOMP.md?plain=1#L4145), [D472](DECOMP.md?plain=1#L4347) |
| Supersedes one-instruction-slot framing with named pointer/temp register choice; related scheduling effects are downstream. | [NG38](#ng38); [D203](DECOMP.md?plain=1#L2315), [D454](DECOMP.md?plain=1#L4186), [D469](DECOMP.md?plain=1#L4330) |
| Earlier 330–380 hoist scores were confounded by reassociation/frame shrink; folded-store control preserves the floor. | [NG38](#ng38); [D203](DECOMP.md?plain=1#L2315), [D454](DECOMP.md?plain=1#L4186), [D469](DECOMP.md?plain=1#L4330) |
| Corrects both split-prologue explanations and narrows the final decision to coalescing/allocation order; no universal impossibility of split pushes. | [NG32](#ng32); [D439](DECOMP.md?plain=1#L4061), [D485](DECOMP.md?plain=1#L4450) |
| Restricts the older no-source-order inference to the measured ValidateCursor shape; explicit int preloads work elsewhere. | [NG46](#ng46); [D259](DECOMP.md?plain=1#L2735), [D286](DECOMP.md?plain=1#L2948), [D418](DECOMP.md?plain=1#L3910) |
| The old broad Pos/two-int equivalence is limited to an address-taken struct forwarded as the argument; plain sums have three measured differences. | [TY05](#ty05); [D078](DECOMP.md?plain=1#L1229), [D089](DECOMP.md?plain=1#L1304), [D148](DECOMP.md?plain=1#L1862), [D231](DECOMP.md?plain=1#L2526) |
| Register-resident and direct-memory &= results had been incorrectly generalized together. | [TY08](#ty08); [D026](DECOMP.md?plain=1#L802), [D068](DECOMP.md?plain=1#L1162), [D097](DECOMP.md?plain=1#L1334), [D138](DECOMP.md?plain=1#L1675), [D323](DECOMP.md?plain=1#L3196) |
| Union/struct casts are not the distinct objects needed by the RequestRoute same-width conversion barrier. | [TY12](#ty12); [D134](DECOMP.md?plain=1#L1652), [D230](DECOMP.md?plain=1#L2523), [D392](DECOMP.md?plain=1#L3707), [D445](DECOMP.md?plain=1#L4105) |
| Corrected SaveScriptString length, NewBNVPath Pos size, MapConfig/Map tile signedness, Person3D padding versus two distinct depth names, and caller-derived misidentifications are retained explicitly. | [TY13](#ty13); [D012](DECOMP.md?plain=1#L596), [D025](DECOMP.md?plain=1#L795), [D077](DECOMP.md?plain=1#L1225), [D255](DECOMP.md?plain=1#L2702), [D285](DECOMP.md?plain=1#L2945), [D299](DECOMP.md?plain=1#L3036), [D312](DECOMP.md?plain=1#L3123), [D317](DECOMP.md?plain=1#L3161), [D330](DECOMP.md?plain=1#L3245), [D380](DECOMP.md?plain=1#L3590), [D408](DECOMP.md?plain=1#L3810), [D412](DECOMP.md?plain=1#L3851) |
| D426 supersedes both the earliest-/last-defined and first-read two-local explanations in D367/D385; rank remains memory, temporary, local. | [SA01](#sa01); [D015](DECOMP.md?plain=1#L722), [D200](DECOMP.md?plain=1#L2288), [D252](DECOMP.md?plain=1#L2691), [D262](DECOMP.md?plain=1#L2761), [D270](DECOMP.md?plain=1#L2819), [D333](DECOMP.md?plain=1#L3261), [D367](DECOMP.md?plain=1#L3503), [D385](DECOMP.md?plain=1#L3627), [D426](DECOMP.md?plain=1#L3966), [D429](DECOMP.md?plain=1#L4006) |
| D385 explicitly retracts the earlier claim that the inline waypoint is necessarily folded into add. | [SA01](#sa01); [D015](DECOMP.md?plain=1#L722), [D200](DECOMP.md?plain=1#L2288), [D252](DECOMP.md?plain=1#L2691), [D262](DECOMP.md?plain=1#L2761), [D270](DECOMP.md?plain=1#L2819), [D333](DECOMP.md?plain=1#L3261), [D367](DECOMP.md?plain=1#L3503), [D385](DECOMP.md?plain=1#L3627), [D426](DECOMP.md?plain=1#L3966), [D429](DECOMP.md?plain=1#L4006) |
| The old “three terms keep source order” statement is a mis-summary; flat three terms canonicalize too. Its valid lesson concerned breaking the sum into partial sums. | [SA02](#sa02); [D207](DECOMP.md?plain=1#L2347), [D307](DECOMP.md?plain=1#L3086), [D354](DECOMP.md?plain=1#L3411), [D449](DECOMP.md?plain=1#L4139), [D463](DECOMP.md?plain=1#L4296), [D476](DECOMP.md?plain=1#L4377) |
| TempleSlide_Update partial-sum barrier is withdrawn, not an outstanding improvement. | [SA02](#sa02); [D207](DECOMP.md?plain=1#L2347), [D307](DECOMP.md?plain=1#L3086), [D354](DECOMP.md?plain=1#L3411), [D449](DECOMP.md?plain=1#L4139), [D463](DECOMP.md?plain=1#L4296), [D476](DECOMP.md?plain=1#L4377) |
| The original “any sum” struct-return prescription is restricted to straight-line arithmetic; the measured loop accumulation requires plain locals. | [SA04](#sa04); [D019](DECOMP.md?plain=1#L756), [D059](DECOMP.md?plain=1#L1103) |
| Empty-if second-consumer explanations do not imply extended live ranges; later measurements identify a block-split effect. | [SA07](#sa07); [D282](DECOMP.md?plain=1#L2925), [D308](DECOMP.md?plain=1#L3094), [D334](DECOMP.md?plain=1#L3270), [D417](DECOMP.md?plain=1#L3906), [D445](DECOMP.md?plain=1#L4105), [D469](DECOMP.md?plain=1#L4330), [D470](DECOMP.md?plain=1#L4340) |
| Older BNV hoist results 330-380 were confounded by tail reassociation; one-web versus named-ys must be separated. | [SA07](#sa07); [D282](DECOMP.md?plain=1#L2925), [D308](DECOMP.md?plain=1#L3094), [D334](DECOMP.md?plain=1#L3270), [D417](DECOMP.md?plain=1#L3906), [D445](DECOMP.md?plain=1#L4105), [D469](DECOMP.md?plain=1#L4330), [D470](DECOMP.md?plain=1#L4340) |
| The *2 fresh-destination rule is not universal; fresh zero-extended negated u16 values before pushes shift in place. | [SA08](#sa08); [D024](DECOMP.md?plain=1#L790), [D026](DECOMP.md?plain=1#L802), [D080](DECOMP.md?plain=1#L1243), [D145](DECOMP.md?plain=1#L1841), [D149](DECOMP.md?plain=1#L1870), [D212](DECOMP.md?plain=1#L2393), [D369](DECOMP.md?plain=1#L3518), [D386](DECOMP.md?plain=1#L3643), [D483](DECOMP.md?plain=1#L4441) |
| D386 resolves D369's open standalone-byte-shift count obstacle with multiplication by 256. | [SA08](#sa08); [D024](DECOMP.md?plain=1#L790), [D026](DECOMP.md?plain=1#L802), [D080](DECOMP.md?plain=1#L1243), [D145](DECOMP.md?plain=1#L1841), [D149](DECOMP.md?plain=1#L1870), [D212](DECOMP.md?plain=1#L2393), [D369](DECOMP.md?plain=1#L3518), [D386](DECOMP.md?plain=1#L3643), [D483](DECOMP.md?plain=1#L4441) |
| The reverse-fld-order rule does not apply to a float accumulator; its three source orders canonicalize. | [FP01](#fp01); [D075](DECOMP.md?plain=1#L1213), [D112](DECOMP.md?plain=1#L1517) |
| D398 retracts D348 load-order diagnosis and its no-live-operands reading; original operand is live into the switch. | [NG25](#ng25); [D265](DECOMP.md?plain=1#L2785), [D348](DECOMP.md?plain=1#L3366), [D398](DECOMP.md?plain=1#L3749) |
| One-web source shape does not imply the same register wall across files; D414 corrects that generalization. | [NG07](#ng07); [D363](DECOMP.md?plain=1#L3476), [D376](DECOMP.md?plain=1#L3563), [D405](DECOMP.md?plain=1#L3796), [D414](DECOMP.md?plain=1#L3867), [D420](DECOMP.md?plain=1#L3920), [D425](DECOMP.md?plain=1#L3956), [D443](DECOMP.md?plain=1#L4091), [D470](DECOMP.md?plain=1#L4340), [D471](DECOMP.md?plain=1#L4344) |
| Named ys, not one-web Y, flattens later subtractions; the two-byte difference is a deficit in the fuller measurement. | [NG07](#ng07); [D363](DECOMP.md?plain=1#L3476), [D376](DECOMP.md?plain=1#L3563), [D405](DECOMP.md?plain=1#L3796), [D414](DECOMP.md?plain=1#L3867), [D420](DECOMP.md?plain=1#L3920), [D425](DECOMP.md?plain=1#L3956), [D443](DECOMP.md?plain=1#L4091), [D470](DECOMP.md?plain=1#L4340), [D471](DECOMP.md?plain=1#L4344) |
| D405 both-axes prescription is narrowed to its measured earlier context; corrected four-ride grid deliberately keeps X non-compound. | [NG07](#ng07); [D363](DECOMP.md?plain=1#L3476), [D376](DECOMP.md?plain=1#L3563), [D405](DECOMP.md?plain=1#L3796), [D414](DECOMP.md?plain=1#L3867), [D420](DECOMP.md?plain=1#L3920), [D425](DECOMP.md?plain=1#L3956), [D443](DECOMP.md?plain=1#L4091), [D470](DECOMP.md?plain=1#L4340), [D471](DECOMP.md?plain=1#L4344) |
| D414 Safari tie prose conflicts with repeated explicit132->131 table; retain the numeric table. | [NG07](#ng07); [D363](DECOMP.md?plain=1#L3476), [D376](DECOMP.md?plain=1#L3563), [D405](DECOMP.md?plain=1#L3796), [D414](DECOMP.md?plain=1#L3867), [D420](DECOMP.md?plain=1#L3920), [D425](DECOMP.md?plain=1#L3956), [D443](DECOMP.md?plain=1#L4091), [D470](DECOMP.md?plain=1#L4340), [D471](DECOMP.md?plain=1#L4344) |
| Over-narrow two-hit scan is superseded by171 matches across1544 bodies, not evidence against the directly measured shape. | [NG07](#ng07); [D363](DECOMP.md?plain=1#L3476), [D376](DECOMP.md?plain=1#L3563), [D405](DECOMP.md?plain=1#L3796), [D414](DECOMP.md?plain=1#L3867), [D420](DECOMP.md?plain=1#L3920), [D425](DECOMP.md?plain=1#L3956), [D443](DECOMP.md?plain=1#L4091), [D470](DECOMP.md?plain=1#L4340), [D471](DECOMP.md?plain=1#L4344) |
| Earlier note reversed original/reconstruction load order; corrected target is ascending loads with x-store first. | [NG39](#ng39); [D337](DECOMP.md?plain=1#L3286), [D364](DECOMP.md?plain=1#L3491), [D453](DECOMP.md?plain=1#L4165) |
| 16-cell grid corrects doubling selection: temp carrying*2, not load-first order. | [NG39](#ng39); [D337](DECOMP.md?plain=1#L3286), [D364](DECOMP.md?plain=1#L3491), [D453](DECOMP.md?plain=1#L4165) |
| Both the optimistic next-step prediction and two-term partial-sum workaround are explicitly withdrawn. | [NG37](#ng37); [D476](DECOMP.md?plain=1#L4377) |
| The no-code conversion tuple effect is established for SetBlokePositionFromBNV FP scheduling, not these anonymous integer measurements. | [NG19](#ng19); [D289](DECOMP.md?plain=1#L2970), [D347](DECOMP.md?plain=1#L3358), [D406](DECOMP.md?plain=1#L3800) |
| Packed-key memcmp is contextual: search and interaction evidence does not prescribe it for rotary tick loops. | [RC01](#rc01); [D001](DECOMP.md?plain=1#L427), [D007](DECOMP.md?plain=1#L519), [D271](DECOMP.md?plain=1#L2827), [D283](DECOMP.md?plain=1#L2931) |
| ResetNarrationStreamState: prefer the 63 B versus 73 totals independently recorded in savemisc2.c; the DECOMP attribution of 11 bytes conflicts with their 10-byte difference. | [RC08](#rc08); [D008](DECOMP.md?plain=1#L569), [D039](DECOMP.md?plain=1#L982), [D097](DECOMP.md?plain=1#L1334), [D138](DECOMP.md?plain=1#L1675), [D155](DECOMP.md?plain=1#L1913), [D350](DECOMP.md?plain=1#L3383), [D383](DECOMP.md?plain=1#L3610) |
| Equal-size and identical-residual twin claims are scoped by direct disassembly comparison and pressure; fable-B supplies a same-source counterexample. | [RD02](#rd02); [D013](DECOMP.md?plain=1#L607), [D025](DECOMP.md?plain=1#L795), [D031](DECOMP.md?plain=1#L931), [D087](DECOMP.md?plain=1#L1293), [D097](DECOMP.md?plain=1#L1334), [D131](DECOMP.md?plain=1#L1630), [D137](DECOMP.md?plain=1#L1667), [D158](DECOMP.md?plain=1#L1941) |
| An EBP frame or unconditional register saves alone do not prove handwritten assembly; Draw3DPersonModel macros explain both. | [RD03](#rd03); [D045](DECOMP.md?plain=1#L1021), [D213](DECOMP.md?plain=1#L2396), [D410](DECOMP.md?plain=1#L3839) |
| DECOMP says the “shipped body would have faulted”; the surrounding report identifies a reconstruction naming bug, not a retail-binary defect. | [RD04](#rd04); [D456](DECOMP.md?plain=1#L4227) |
| Rotated-loop extent failure is fixed; retain truncated/full measurements, remove the obsolete pending-fix instruction. | [TM01](#tm01); [D037](DECOMP.md?plain=1#L967), [D064](DECOMP.md?plain=1#L1134), [D108](DECOMP.md?plain=1#L1480), [D243](DECOMP.md?plain=1#L2610) |
| Inert volatile and rb zero do not prove a floor: SubtractObjRect and both Destroy callbacks directly refute that claim. | [TM02](#tm02); [D127](DECOMP.md?plain=1#L1601), [D138](DECOMP.md?plain=1#L1675), [D185](DECOMP.md?plain=1#L2153), [D190](DECOMP.md?plain=1#L2204) |
| The corpus-scan story in D344 does not override D398: LFEntrance_Activate’s allegedly dead operand is live into the switch. | [TM06](#tm06); [D181](DECOMP.md?plain=1#L2117), [D311](DECOMP.md?plain=1#L3116), [D322](DECOMP.md?plain=1#L3189), [D339](DECOMP.md?plain=1#L3296), [D344](DECOMP.md?plain=1#L3332), [D353](DECOMP.md?plain=1#L3403), [D398](DECOMP.md?plain=1#L3749), [D402](DECOMP.md?plain=1#L3771) |
| RenderFullMap adjusts a copied Pos, not the original; preserve all five correction metrics. | [TM10](#tm10); [D197](DECOMP.md?plain=1#L2257), [D430](DECOMP.md?plain=1#L4012), [D462](DECOMP.md?plain=1#L4279), [D485](DECOMP.md?plain=1#L4450), [D493](DECOMP.md?plain=1#L4492) |
| RenderView geometry/+10 and split-prologue remedies are coupled; a duplicated escaped-struct null test is not folded. | [TM10](#tm10); [D197](DECOMP.md?plain=1#L2257), [D430](DECOMP.md?plain=1#L4012), [D462](DECOMP.md?plain=1#L4279), [D485](DECOMP.md?plain=1#L4450), [D493](DECOMP.md?plain=1#L4492) |

## Source coverage

The numbered source inventory covers DECOMP's entire lever section, from its heading through the paragraph before Legal. A source ID identifies one original top-level bullet, including any nested bullets and trailing explanatory paragraphs. Line links refer to the named snapshot; future prepends to DECOMP can shift line numbers. The function names and measurements in each entry remain the searchable evidence.

Source snapshots folded into the entries include codex-E 44/44 (seven ride state machines), codex-D 49/49, scope-E 147/147 (1–17 instructions), scope-I one close/two improvements/twelve floors, fable-D 34/34, codex-C 88/88 (1–28 instructions; eighty-plus closed first compile), fable-C 26/26 with codex-B 60/60, fable-B 13/18, and fable-A 12/14. These wrapper counts are retained here as provenance, not new codegen rules. The `docs/lanes/*.md` reports provide the expanded examples cited in the entries.

| DECOMP source | Consolidated entries |
| --- | --- |
| [D001](DECOMP.md?plain=1#L427) | [CC02](#cc02), [FP02](#fp02), [LP10](#lp10), [RA05](#ra05), [RA09](#ra09), [RC01](#rc01), [RC07](#rc07), [RD01](#rd01), [TY09](#ty09), [TY10](#ty10) |
| [D002](DECOMP.md?plain=1#L465) | [LP09](#lp09) |
| [D003](DECOMP.md?plain=1#L475) | [RA11](#ra11) |
| [D004](DECOMP.md?plain=1#L481) | [CC04](#cc04), [LP03](#lp03), [TY10](#ty10) |
| [D005](DECOMP.md?plain=1#L490) | [NG27](#ng27) |
| [D006](DECOMP.md?plain=1#L504) | [BL03](#bl03), [RA03](#ra03), [RA05](#ra05), [RD01](#rd01) |
| [D007](DECOMP.md?plain=1#L519) | [BL16](#bl16), [CC09](#cc09), [FP10](#fp10), [LP14](#lp14), [NG31](#ng31), [RA05](#ra05), [RA08](#ra08), [RC01](#rc01), [SA03](#sa03), [TM09](#tm09) |
| [D008](DECOMP.md?plain=1#L569) | [RC08](#rc08) |
| [D009](DECOMP.md?plain=1#L577) | [LP07](#lp07) |
| [D010](DECOMP.md?plain=1#L584) | [LP10](#lp10) |
| [D011](DECOMP.md?plain=1#L589) | [LP10](#lp10), [RA05](#ra05) |
| [D012](DECOMP.md?plain=1#L596) | [TY13](#ty13) |
| [D013](DECOMP.md?plain=1#L607) | [BL07](#bl07), [BL08](#bl08), [BL10](#bl10), [CC03](#cc03), [CC04](#cc04), [CC06](#cc06), [CC11](#cc11), [FP02](#fp02), [FP03](#fp03), [FP05](#fp05), [FP06](#fp06), [FR02](#fr02), [LP03](#lp03), [LP05](#lp05), [LP06](#lp06), [LP08](#lp08), [RA07](#ra07), [RA08](#ra08), [RA09](#ra09), [RC04](#rc04), [RD02](#rd02), [SA05](#sa05), [TY01](#ty01), [TY04](#ty04) |
| [D014](DECOMP.md?plain=1#L713) | [RA05](#ra05) |
| [D015](DECOMP.md?plain=1#L722) | [SA01](#sa01) |
| [D016](DECOMP.md?plain=1#L727) | [CC03](#cc03), [LP03](#lp03) |
| [D017](DECOMP.md?plain=1#L734) | [RC10](#rc10) |
| [D018](DECOMP.md?plain=1#L741) | [RA01](#ra01), [RA11](#ra11), [RC06](#rc06), [RD01](#rd01), [TY04](#ty04) |
| [D019](DECOMP.md?plain=1#L756) | [SA04](#sa04) |
| [D020](DECOMP.md?plain=1#L766) | [SA05](#sa05) |
| [D021](DECOMP.md?plain=1#L772) | [SA03](#sa03) |
| [D022](DECOMP.md?plain=1#L777) | [SA03](#sa03) |
| [D023](DECOMP.md?plain=1#L785) | [LP14](#lp14) |
| [D024](DECOMP.md?plain=1#L790) | [SA08](#sa08) |
| [D025](DECOMP.md?plain=1#L795) | [NG20](#ng20), [RD02](#rd02), [TY13](#ty13) |
| [D026](DECOMP.md?plain=1#L802) | [BL08](#bl08), [BL10](#bl10), [BL15](#bl15), [CC01](#cc01), [FR09](#fr09), [LP04](#lp04), [LP06](#lp06), [LP11](#lp11), [LP12](#lp12), [LP17](#lp17), [RA01](#ra01), [RA06](#ra06), [RA07](#ra07), [RA08](#ra08), [SA03](#sa03), [SA05](#sa05), [SA08](#sa08), [TY04](#ty04), [TY08](#ty08) |
| [D027](DECOMP.md?plain=1#L905) | [FR03](#fr03) |
| [D028](DECOMP.md?plain=1#L913) | [RA04](#ra04) |
| [D029](DECOMP.md?plain=1#L918) | [NG11](#ng11) |
| [D030](DECOMP.md?plain=1#L927) | [LP10](#lp10) |
| [D031](DECOMP.md?plain=1#L931) | [RD02](#rd02) |
| [D032](DECOMP.md?plain=1#L936) | [NG17](#ng17), [RC02](#rc02) |
| [D033](DECOMP.md?plain=1#L946) | [LP08](#lp08) |
| [D034](DECOMP.md?plain=1#L953) | [RA18](#ra18) |
| [D035](DECOMP.md?plain=1#L960) | [RA02](#ra02) |
| [D036](DECOMP.md?plain=1#L964) | [BL17](#bl17) |
| [D037](DECOMP.md?plain=1#L967) | [RD01](#rd01), [TM01](#tm01) |
| [D038](DECOMP.md?plain=1#L977) | [BL05](#bl05) |
| [D039](DECOMP.md?plain=1#L982) | [RC08](#rc08) |
| [D040](DECOMP.md?plain=1#L988) | [SA06](#sa06) |
| [D041](DECOMP.md?plain=1#L993) | [CC03](#cc03), [TY04](#ty04) |
| [D042](DECOMP.md?plain=1#L999) | [TY07](#ty07) |
| [D043](DECOMP.md?plain=1#L1004) | [NG28](#ng28) |
| [D044](DECOMP.md?plain=1#L1011) | [RC05](#rc05) |
| [D045](DECOMP.md?plain=1#L1021) | [RD03](#rd03) |
| [D046](DECOMP.md?plain=1#L1027) | [FR02](#fr02) |
| [D047](DECOMP.md?plain=1#L1032) | [RA04](#ra04) |
| [D048](DECOMP.md?plain=1#L1037) | [LP03](#lp03) |
| [D049](DECOMP.md?plain=1#L1040) | [FP02](#fp02) |
| [D050](DECOMP.md?plain=1#L1046) | [NG34](#ng34), [NG47](#ng47) |
| [D051](DECOMP.md?plain=1#L1053) | [RC11](#rc11) |
| [D052](DECOMP.md?plain=1#L1061) | [RA03](#ra03) |
| [D053](DECOMP.md?plain=1#L1067) | [LP03](#lp03), [SA06](#sa06) |
| [D054](DECOMP.md?plain=1#L1073) | [BL10](#bl10) |
| [D055](DECOMP.md?plain=1#L1080) | [FR01](#fr01) |
| [D056](DECOMP.md?plain=1#L1086) | [NG29](#ng29) |
| [D057](DECOMP.md?plain=1#L1094) | [LP17](#lp17) |
| [D058](DECOMP.md?plain=1#L1099) | [TY11](#ty11) |
| [D059](DECOMP.md?plain=1#L1103) | [SA04](#sa04) |
| [D060](DECOMP.md?plain=1#L1110) | [RA02](#ra02) |
| [D061](DECOMP.md?plain=1#L1117) | [BL07](#bl07) |
| [D062](DECOMP.md?plain=1#L1122) | [LP10](#lp10), [RA05](#ra05), [TY11](#ty11) |
| [D063](DECOMP.md?plain=1#L1127) | [FR02](#fr02), [RA08](#ra08) |
| [D064](DECOMP.md?plain=1#L1134) | [TM01](#tm01) |
| [D065](DECOMP.md?plain=1#L1138) | [TY03](#ty03) |
| [D066](DECOMP.md?plain=1#L1149) | [RA03](#ra03) |
| [D067](DECOMP.md?plain=1#L1157) | [RA01](#ra01) |
| [D068](DECOMP.md?plain=1#L1162) | [LP04](#lp04), [SA10](#sa10), [TY08](#ty08) |
| [D069](DECOMP.md?plain=1#L1170) | [RD01](#rd01), [TY04](#ty04) |
| [D070](DECOMP.md?plain=1#L1178) | [RD08](#rd08) |
| [D071](DECOMP.md?plain=1#L1183) | [LP08](#lp08) |
| [D072](DECOMP.md?plain=1#L1192) | [RA11](#ra11), [SA05](#sa05) |
| [D073](DECOMP.md?plain=1#L1199) | [RA16](#ra16) |
| [D074](DECOMP.md?plain=1#L1205) | [FP05](#fp05) |
| [D075](DECOMP.md?plain=1#L1213) | [FP01](#fp01), [FP04](#fp04) |
| [D076](DECOMP.md?plain=1#L1218) | [FP07](#fp07) |
| [D077](DECOMP.md?plain=1#L1225) | [TY13](#ty13) |
| [D078](DECOMP.md?plain=1#L1229) | [TY05](#ty05) |
| [D079](DECOMP.md?plain=1#L1237) | [SA03](#sa03) |
| [D080](DECOMP.md?plain=1#L1243) | [RA01](#ra01), [SA08](#sa08) |
| [D081](DECOMP.md?plain=1#L1251) | [FP02](#fp02), [RA03](#ra03) |
| [D082](DECOMP.md?plain=1#L1258) | [NG18](#ng18) |
| [D083](DECOMP.md?plain=1#L1266) | [RA08](#ra08) |
| [D084](DECOMP.md?plain=1#L1276) | [BL13](#bl13) |
| [D085](DECOMP.md?plain=1#L1281) | [RC03](#rc03) |
| [D086](DECOMP.md?plain=1#L1286) | [SA06](#sa06) |
| [D087](DECOMP.md?plain=1#L1293) | [RD02](#rd02) |
| [D088](DECOMP.md?plain=1#L1300) | [FR02](#fr02) |
| [D089](DECOMP.md?plain=1#L1304) | [TY05](#ty05) |
| [D090](DECOMP.md?plain=1#L1310) | [RA02](#ra02) |
| [D091](DECOMP.md?plain=1#L1314) | [RA11](#ra11) |
| [D092](DECOMP.md?plain=1#L1317) | [FP08](#fp08) |
| [D093](DECOMP.md?plain=1#L1321) | [FP04](#fp04) |
| [D094](DECOMP.md?plain=1#L1323) | [BL10](#bl10) |
| [D095](DECOMP.md?plain=1#L1326) | [FP05](#fp05) |
| [D096](DECOMP.md?plain=1#L1330) | [TY07](#ty07) |
| [D097](DECOMP.md?plain=1#L1334) | [BL03](#bl03), [BL04](#bl04), [BL09](#bl09), [CC04](#cc04), [CC08](#cc08), [CC10](#cc10), [FR02](#fr02), [FR06](#fr06), [LP04](#lp04), [LP08](#lp08), [LP16](#lp16), [NG26](#ng26), [NG41](#ng41), [NG45](#ng45), [RA01](#ra01), [RA03](#ra03), [RC04](#rc04), [RC08](#rc08), [RD02](#rd02), [TY08](#ty08) |
| [D098](DECOMP.md?plain=1#L1435) | [LP07](#lp07) |
| [D099](DECOMP.md?plain=1#L1442) | [LP01](#lp01) |
| [D100](DECOMP.md?plain=1#L1448) | [BL16](#bl16) |
| [D101](DECOMP.md?plain=1#L1453) | [FP03](#fp03) |
| [D102](DECOMP.md?plain=1#L1458) | [FP06](#fp06) |
| [D103](DECOMP.md?plain=1#L1463) | [FP03](#fp03) |
| [D104](DECOMP.md?plain=1#L1465) | [RA11](#ra11) |
| [D105](DECOMP.md?plain=1#L1471) | [LP12](#lp12) |
| [D106](DECOMP.md?plain=1#L1474) | [SA03](#sa03) |
| [D107](DECOMP.md?plain=1#L1477) | [SA06](#sa06) |
| [D108](DECOMP.md?plain=1#L1480) | [TM01](#tm01) |
| [D109](DECOMP.md?plain=1#L1497) | [LP11](#lp11) |
| [D110](DECOMP.md?plain=1#L1505) | [LP13](#lp13) |
| [D111](DECOMP.md?plain=1#L1511) | [FP06](#fp06), [FR02](#fr02) |
| [D112](DECOMP.md?plain=1#L1517) | [FP01](#fp01) |
| [D113](DECOMP.md?plain=1#L1521) | [FP03](#fp03) |
| [D114](DECOMP.md?plain=1#L1524) | [FP02](#fp02) |
| [D115](DECOMP.md?plain=1#L1529) | [FP08](#fp08) |
| [D116](DECOMP.md?plain=1#L1533) | [TY03](#ty03) |
| [D117](DECOMP.md?plain=1#L1536) | [LP12](#lp12) |
| [D118](DECOMP.md?plain=1#L1541) | [RA05](#ra05) |
| [D119](DECOMP.md?plain=1#L1544) | [NG35](#ng35), [NG40](#ng40) |
| [D120](DECOMP.md?plain=1#L1553) | [RA11](#ra11) |
| [D121](DECOMP.md?plain=1#L1561) | [LP08](#lp08) |
| [D122](DECOMP.md?plain=1#L1566) | [BL13](#bl13) |
| [D123](DECOMP.md?plain=1#L1575) | [RA02](#ra02) |
| [D124](DECOMP.md?plain=1#L1582) | [SA06](#sa06) |
| [D125](DECOMP.md?plain=1#L1589) | [CC04](#cc04) |
| [D126](DECOMP.md?plain=1#L1595) | [TY04](#ty04) |
| [D127](DECOMP.md?plain=1#L1601) | [RA01](#ra01), [TM02](#tm02) |
| [D128](DECOMP.md?plain=1#L1614) | [SA11](#sa11) |
| [D129](DECOMP.md?plain=1#L1621) | [FR01](#fr01), [SA03](#sa03) |
| [D130](DECOMP.md?plain=1#L1625) | [CC01](#cc01) |
| [D131](DECOMP.md?plain=1#L1630) | [RD02](#rd02) |
| [D132](DECOMP.md?plain=1#L1638) | [SA05](#sa05) |
| [D133](DECOMP.md?plain=1#L1644) | [TY04](#ty04) |
| [D134](DECOMP.md?plain=1#L1652) | [TY12](#ty12) |
| [D135](DECOMP.md?plain=1#L1658) | [CC01](#cc01) |
| [D136](DECOMP.md?plain=1#L1662) | [RD05](#rd05) |
| [D137](DECOMP.md?plain=1#L1667) | [RD02](#rd02) |
| [D138](DECOMP.md?plain=1#L1675) | [BL02](#bl02), [CC03](#cc03), [CC05](#cc05), [FR02](#fr02), [LP06](#lp06), [LP11](#lp11), [LP14](#lp14), [LP15](#lp15), [LP18](#lp18), [NG09](#ng09), [NG22](#ng22), [RA11](#ra11), [RA16](#ra16), [RC08](#rc08), [RC09](#rc09), [SA05](#sa05), [TM02](#tm02), [TY04](#ty04), [TY08](#ty08) |
| [D139](DECOMP.md?plain=1#L1804) | [CC02](#cc02) |
| [D140](DECOMP.md?plain=1#L1812) | [RA12](#ra12) |
| [D141](DECOMP.md?plain=1#L1817) | [FR04](#fr04) |
| [D142](DECOMP.md?plain=1#L1822) | [LP12](#lp12) |
| [D143](DECOMP.md?plain=1#L1829) | [RA06](#ra06) |
| [D144](DECOMP.md?plain=1#L1835) | [SA05](#sa05) |
| [D145](DECOMP.md?plain=1#L1841) | [SA08](#sa08) |
| [D146](DECOMP.md?plain=1#L1845) | [LP07](#lp07) |
| [D147](DECOMP.md?plain=1#L1853) | [FR06](#fr06) |
| [D148](DECOMP.md?plain=1#L1862) | [TY05](#ty05) |
| [D149](DECOMP.md?plain=1#L1870) | [SA08](#sa08) |
| [D150](DECOMP.md?plain=1#L1875) | [SA10](#sa10) |
| [D151](DECOMP.md?plain=1#L1881) | [RA18](#ra18) |
| [D152](DECOMP.md?plain=1#L1889) | [SA03](#sa03) |
| [D153](DECOMP.md?plain=1#L1897) | [BL03](#bl03) |
| [D154](DECOMP.md?plain=1#L1904) | [CC06](#cc06) |
| [D155](DECOMP.md?plain=1#L1913) | [RC08](#rc08) |
| [D156](DECOMP.md?plain=1#L1922) | [BL07](#bl07) |
| [D157](DECOMP.md?plain=1#L1930) | [BL03](#bl03), [RA01](#ra01) |
| [D158](DECOMP.md?plain=1#L1941) | [RD02](#rd02) |
| [D159](DECOMP.md?plain=1#L1949) | [BL20](#bl20) |
| [D160](DECOMP.md?plain=1#L1954) | [BL11](#bl11) |
| [D161](DECOMP.md?plain=1#L1963) | [BL12](#bl12) |
| [D162](DECOMP.md?plain=1#L1971) | [FR06](#fr06) |
| [D163](DECOMP.md?plain=1#L1977) | [BL07](#bl07) |
| [D164](DECOMP.md?plain=1#L1981) | [TY10](#ty10) |
| [D165](DECOMP.md?plain=1#L1985) | [CC05](#cc05) |
| [D166](DECOMP.md?plain=1#L1990) | [CC05](#cc05) |
| [D167](DECOMP.md?plain=1#L1997) | [RA05](#ra05) |
| [D168](DECOMP.md?plain=1#L2002) | [FR01](#fr01), [RA15](#ra15) |
| [D169](DECOMP.md?plain=1#L2029) | [RA02](#ra02) |
| [D170](DECOMP.md?plain=1#L2037) | [RA11](#ra11) |
| [D171](DECOMP.md?plain=1#L2046) | [FR01](#fr01) |
| [D172](DECOMP.md?plain=1#L2056) | [SA03](#sa03) |
| [D173](DECOMP.md?plain=1#L2064) | [RA01](#ra01) |
| [D174](DECOMP.md?plain=1#L2070) | [LP04](#lp04) |
| [D175](DECOMP.md?plain=1#L2075) | [BL07](#bl07), [BL08](#bl08) |
| [D176](DECOMP.md?plain=1#L2080) | [BL07](#bl07) |
| [D177](DECOMP.md?plain=1#L2088) | [FR08](#fr08) |
| [D178](DECOMP.md?plain=1#L2096) | [TY10](#ty10) |
| [D179](DECOMP.md?plain=1#L2103) | [BL10](#bl10) |
| [D180](DECOMP.md?plain=1#L2111) | [FR02](#fr02) |
| [D181](DECOMP.md?plain=1#L2117) | [LP02](#lp02), [TM06](#tm06) |
| [D182](DECOMP.md?plain=1#L2124) | [LP02](#lp02) |
| [D183](DECOMP.md?plain=1#L2132) | [RA15](#ra15) |
| [D184](DECOMP.md?plain=1#L2142) | [RA13](#ra13) |
| [D185](DECOMP.md?plain=1#L2153) | [TM02](#tm02) |
| [D186](DECOMP.md?plain=1#L2176) | [TM03](#tm03) |
| [D187](DECOMP.md?plain=1#L2181) | [FR01](#fr01) |
| [D188](DECOMP.md?plain=1#L2190) | [NG06](#ng06) |
| [D189](DECOMP.md?plain=1#L2199) | [LP18](#lp18) |
| [D190](DECOMP.md?plain=1#L2204) | [RA11](#ra11), [TM02](#tm02) |
| [D191](DECOMP.md?plain=1#L2213) | [TM07](#tm07) |
| [D192](DECOMP.md?plain=1#L2221) | [LP13](#lp13), [RA02](#ra02) |
| [D193](DECOMP.md?plain=1#L2227) | [NG33](#ng33), [RA13](#ra13) |
| [D194](DECOMP.md?plain=1#L2238) | [NG33](#ng33) |
| [D195](DECOMP.md?plain=1#L2245) | [RA10](#ra10) |
| [D196](DECOMP.md?plain=1#L2251) | [LP07](#lp07) |
| [D197](DECOMP.md?plain=1#L2257) | [TM10](#tm10) |
| [D198](DECOMP.md?plain=1#L2271) | [BL01](#bl01) |
| [D199](DECOMP.md?plain=1#L2283) | [BL06](#bl06) |
| [D200](DECOMP.md?plain=1#L2288) | [SA01](#sa01) |
| [D201](DECOMP.md?plain=1#L2296) | [FP09](#fp09) |
| [D202](DECOMP.md?plain=1#L2305) | [TM03](#tm03) |
| [D203](DECOMP.md?plain=1#L2315) | [NG38](#ng38), [RA14](#ra14) |
| [D204](DECOMP.md?plain=1#L2325) | [NG10](#ng10) |
| [D205](DECOMP.md?plain=1#L2330) | [NG36](#ng36) |
| [D206](DECOMP.md?plain=1#L2337) | [TM03](#tm03) |
| [D207](DECOMP.md?plain=1#L2347) | [SA02](#sa02) |
| [D208](DECOMP.md?plain=1#L2362) | [NG13](#ng13) |
| [D209](DECOMP.md?plain=1#L2370) | [NG12](#ng12) |
| [D210](DECOMP.md?plain=1#L2378) | [TY01](#ty01) |
| [D211](DECOMP.md?plain=1#L2384) | [RA09](#ra09) |
| [D212](DECOMP.md?plain=1#L2393) | [SA08](#sa08) |
| [D213](DECOMP.md?plain=1#L2396) | [RD03](#rd03) |
| [D214](DECOMP.md?plain=1#L2408) | [FR04](#fr04) |
| [D215](DECOMP.md?plain=1#L2420) | [BL09](#bl09) |
| [D216](DECOMP.md?plain=1#L2424) | [TM03](#tm03) |
| [D217](DECOMP.md?plain=1#L2427) | [RA06](#ra06), [SA10](#sa10) |
| [D218](DECOMP.md?plain=1#L2439) | [BL16](#bl16) |
| [D219](DECOMP.md?plain=1#L2450) | [TY09](#ty09) |
| [D220](DECOMP.md?plain=1#L2457) | [FP07](#fp07) |
| [D221](DECOMP.md?plain=1#L2463) | [LP04](#lp04) |
| [D222](DECOMP.md?plain=1#L2470) | [BL13](#bl13), [TY03](#ty03) |
| [D223](DECOMP.md?plain=1#L2481) | [FR09](#fr09) |
| [D224](DECOMP.md?plain=1#L2488) | [FR05](#fr05) |
| [D225](DECOMP.md?plain=1#L2493) | [LP12](#lp12) |
| [D226](DECOMP.md?plain=1#L2498) | [BL03](#bl03) |
| [D227](DECOMP.md?plain=1#L2503) | [RA06](#ra06) |
| [D228](DECOMP.md?plain=1#L2508) | [RA06](#ra06), [SA06](#sa06) |
| [D229](DECOMP.md?plain=1#L2514) | [SA03](#sa03) |
| [D230](DECOMP.md?plain=1#L2523) | [TY12](#ty12) |
| [D231](DECOMP.md?plain=1#L2526) | [TY05](#ty05) |
| [D232](DECOMP.md?plain=1#L2532) | [RC03](#rc03) |
| [D233](DECOMP.md?plain=1#L2539) | [BL14](#bl14) |
| [D234](DECOMP.md?plain=1#L2552) | [TY02](#ty02) |
| [D235](DECOMP.md?plain=1#L2560) | [RA12](#ra12) |
| [D236](DECOMP.md?plain=1#L2565) | [RA03](#ra03), [TY06](#ty06) |
| [D237](DECOMP.md?plain=1#L2570) | [FR08](#fr08) |
| [D238](DECOMP.md?plain=1#L2574) | [NG20](#ng20), [SA09](#sa09) |
| [D239](DECOMP.md?plain=1#L2590) | [RA13](#ra13) |
| [D240](DECOMP.md?plain=1#L2596) | [FR06](#fr06) |
| [D241](DECOMP.md?plain=1#L2603) | [BL18](#bl18) |
| [D242](DECOMP.md?plain=1#L2608) | [RA02](#ra02) |
| [D243](DECOMP.md?plain=1#L2610) | [TM01](#tm01) |
| [D244](DECOMP.md?plain=1#L2614) | [FR10](#fr10) |
| [D245](DECOMP.md?plain=1#L2626) | [RA19](#ra19), [TY07](#ty07) |
| [D246](DECOMP.md?plain=1#L2636) | [FP11](#fp11) |
| [D247](DECOMP.md?plain=1#L2653) | [TY01](#ty01) |
| [D248](DECOMP.md?plain=1#L2663) | [FP03](#fp03), [RA09](#ra09) |
| [D249](DECOMP.md?plain=1#L2672) | [CC01](#cc01) |
| [D250](DECOMP.md?plain=1#L2681) | [RA13](#ra13) |
| [D251](DECOMP.md?plain=1#L2685) | [FR06](#fr06) |
| [D252](DECOMP.md?plain=1#L2691) | [SA01](#sa01) |
| [D253](DECOMP.md?plain=1#L2696) | [LP04](#lp04) |
| [D254](DECOMP.md?plain=1#L2699) | [TM04](#tm04) |
| [D255](DECOMP.md?plain=1#L2702) | [TY13](#ty13) |
| [D256](DECOMP.md?plain=1#L2707) | [RA10](#ra10) |
| [D257](DECOMP.md?plain=1#L2721) | [TY01](#ty01) |
| [D258](DECOMP.md?plain=1#L2726) | [SA03](#sa03) |
| [D259](DECOMP.md?plain=1#L2735) | [FR06](#fr06), [NG46](#ng46), [RA13](#ra13) |
| [D260](DECOMP.md?plain=1#L2741) | [CC02](#cc02), [LP05](#lp05) |
| [D261](DECOMP.md?plain=1#L2755) | [BL09](#bl09) |
| [D262](DECOMP.md?plain=1#L2761) | [SA01](#sa01) |
| [D263](DECOMP.md?plain=1#L2766) | [BL02](#bl02), [BL03](#bl03), [BL09](#bl09) |
| [D264](DECOMP.md?plain=1#L2778) | [FR07](#fr07) |
| [D265](DECOMP.md?plain=1#L2785) | [NG01](#ng01), [NG25](#ng25) |
| [D266](DECOMP.md?plain=1#L2791) | [RA13](#ra13), [SA06](#sa06) |
| [D267](DECOMP.md?plain=1#L2797) | [FR04](#fr04), [NG02](#ng02) |
| [D268](DECOMP.md?plain=1#L2803) | [FR07](#fr07) |
| [D269](DECOMP.md?plain=1#L2807) | [TY14](#ty14) |
| [D270](DECOMP.md?plain=1#L2819) | [SA01](#sa01) |
| [D271](DECOMP.md?plain=1#L2827) | [RC01](#rc01) |
| [D272](DECOMP.md?plain=1#L2832) | [RA17](#ra17) |
| [D273](DECOMP.md?plain=1#L2845) | [CC07](#cc07) |
| [D274](DECOMP.md?plain=1#L2852) | [FR04](#fr04), [NG10](#ng10), [RA01](#ra01) |
| [D275](DECOMP.md?plain=1#L2862) | [RA13](#ra13) |
| [D276](DECOMP.md?plain=1#L2875) | [RA13](#ra13) |
| [D277](DECOMP.md?plain=1#L2879) | [FR04](#fr04) |
| [D278](DECOMP.md?plain=1#L2885) | [BL03](#bl03) |
| [D279](DECOMP.md?plain=1#L2895) | [BL02](#bl02) |
| [D280](DECOMP.md?plain=1#L2912) | [FR07](#fr07) |
| [D281](DECOMP.md?plain=1#L2921) | [RA12](#ra12) |
| [D282](DECOMP.md?plain=1#L2925) | [SA07](#sa07) |
| [D283](DECOMP.md?plain=1#L2931) | [RC01](#rc01) |
| [D284](DECOMP.md?plain=1#L2935) | [FR04](#fr04), [TY04](#ty04) |
| [D285](DECOMP.md?plain=1#L2945) | [TY13](#ty13) |
| [D286](DECOMP.md?plain=1#L2948) | [NG46](#ng46), [RA13](#ra13) |
| [D287](DECOMP.md?plain=1#L2957) | [FR06](#fr06) |
| [D288](DECOMP.md?plain=1#L2962) | [NG16](#ng16), [RA02](#ra02), [RA20](#ra20) |
| [D289](DECOMP.md?plain=1#L2970) | [NG10](#ng10), [NG19](#ng19) |
| [D290](DECOMP.md?plain=1#L2979) | [TY04](#ty04) |
| [D291](DECOMP.md?plain=1#L2984) | [BL10](#bl10), [BL19](#bl19) |
| [D292](DECOMP.md?plain=1#L2993) | [RA13](#ra13) |
| [D293](DECOMP.md?plain=1#L2999) | [TY03](#ty03) |
| [D294](DECOMP.md?plain=1#L3002) | [FR07](#fr07) |
| [D295](DECOMP.md?plain=1#L3006) | [BL02](#bl02), [TM08](#tm08) |
| [D296](DECOMP.md?plain=1#L3013) | [BL02](#bl02), [BL09](#bl09) |
| [D297](DECOMP.md?plain=1#L3022) | [CC05](#cc05), [RA14](#ra14) |
| [D298](DECOMP.md?plain=1#L3031) | [TM04](#tm04) |
| [D299](DECOMP.md?plain=1#L3036) | [TY13](#ty13) |
| [D300](DECOMP.md?plain=1#L3038) | [BL03](#bl03), [RA11](#ra11) |
| [D301](DECOMP.md?plain=1#L3048) | [NG15](#ng15), [RA12](#ra12) |
| [D302](DECOMP.md?plain=1#L3056) | [TM07](#tm07) |
| [D303](DECOMP.md?plain=1#L3060) | [NG08](#ng08) |
| [D304](DECOMP.md?plain=1#L3064) | [BL12](#bl12) |
| [D305](DECOMP.md?plain=1#L3073) | [CC02](#cc02) |
| [D306](DECOMP.md?plain=1#L3079) | [SA03](#sa03) |
| [D307](DECOMP.md?plain=1#L3086) | [SA02](#sa02) |
| [D308](DECOMP.md?plain=1#L3094) | [SA07](#sa07) |
| [D309](DECOMP.md?plain=1#L3102) | [RA06](#ra06) |
| [D310](DECOMP.md?plain=1#L3108) | [RA10](#ra10) |
| [D311](DECOMP.md?plain=1#L3116) | [TM06](#tm06) |
| [D312](DECOMP.md?plain=1#L3123) | [TY13](#ty13) |
| [D313](DECOMP.md?plain=1#L3127) | [BL05](#bl05), [NG30](#ng30) |
| [D314](DECOMP.md?plain=1#L3137) | [SA09](#sa09) |
| [D315](DECOMP.md?plain=1#L3145) | [RA12](#ra12) |
| [D316](DECOMP.md?plain=1#L3154) | [NG42](#ng42) |
| [D317](DECOMP.md?plain=1#L3161) | [TY13](#ty13) |
| [D318](DECOMP.md?plain=1#L3165) | [RA09](#ra09) |
| [D319](DECOMP.md?plain=1#L3174) | [TM08](#tm08) |
| [D320](DECOMP.md?plain=1#L3180) | [TM07](#tm07) |
| [D321](DECOMP.md?plain=1#L3183) | [TY07](#ty07) |
| [D322](DECOMP.md?plain=1#L3189) | [TM06](#tm06) |
| [D323](DECOMP.md?plain=1#L3196) | [TY08](#ty08) |
| [D324](DECOMP.md?plain=1#L3205) | [CC05](#cc05) |
| [D325](DECOMP.md?plain=1#L3212) | [RA17](#ra17) |
| [D326](DECOMP.md?plain=1#L3218) | [TM09](#tm09) |
| [D327](DECOMP.md?plain=1#L3223) | [FR04](#fr04) |
| [D328](DECOMP.md?plain=1#L3229) | [RA08](#ra08) |
| [D329](DECOMP.md?plain=1#L3240) | [FR06](#fr06) |
| [D330](DECOMP.md?plain=1#L3245) | [TY03](#ty03), [TY13](#ty13) |
| [D331](DECOMP.md?plain=1#L3251) | [RA12](#ra12) |
| [D332](DECOMP.md?plain=1#L3256) | [TM09](#tm09) |
| [D333](DECOMP.md?plain=1#L3261) | [SA01](#sa01) |
| [D334](DECOMP.md?plain=1#L3270) | [SA07](#sa07) |
| [D335](DECOMP.md?plain=1#L3273) | [RA14](#ra14) |
| [D336](DECOMP.md?plain=1#L3282) | [TM09](#tm09) |
| [D337](DECOMP.md?plain=1#L3286) | [NG39](#ng39) |
| [D338](DECOMP.md?plain=1#L3292) | [TM08](#tm08) |
| [D339](DECOMP.md?plain=1#L3296) | [TM03](#tm03), [TM06](#tm06) |
| [D340](DECOMP.md?plain=1#L3305) | [FR07](#fr07) |
| [D341](DECOMP.md?plain=1#L3312) | [FR04](#fr04) |
| [D342](DECOMP.md?plain=1#L3316) | [RA09](#ra09) |
| [D343](DECOMP.md?plain=1#L3325) | [NG03](#ng03) |
| [D344](DECOMP.md?plain=1#L3332) | [TM06](#tm06) |
| [D345](DECOMP.md?plain=1#L3344) | [RA20](#ra20) |
| [D346](DECOMP.md?plain=1#L3352) | [NG15](#ng15) |
| [D347](DECOMP.md?plain=1#L3358) | [NG10](#ng10), [NG19](#ng19) |
| [D348](DECOMP.md?plain=1#L3366) | [NG01](#ng01), [NG25](#ng25) |
| [D349](DECOMP.md?plain=1#L3372) | [NG14](#ng14), [RA12](#ra12) |
| [D350](DECOMP.md?plain=1#L3383) | [RC08](#rc08) |
| [D351](DECOMP.md?plain=1#L3388) | [LP02](#lp02), [NG10](#ng10) |
| [D352](DECOMP.md?plain=1#L3400) | [RA15](#ra15) |
| [D353](DECOMP.md?plain=1#L3403) | [TM06](#tm06) |
| [D354](DECOMP.md?plain=1#L3411) | [SA02](#sa02) |
| [D355](DECOMP.md?plain=1#L3422) | [BL08](#bl08) |
| [D356](DECOMP.md?plain=1#L3428) | [BL09](#bl09) |
| [D357](DECOMP.md?plain=1#L3433) | [TM03](#tm03) |
| [D358](DECOMP.md?plain=1#L3437) | [FR07](#fr07) |
| [D359](DECOMP.md?plain=1#L3441) | [NG20](#ng20), [SA09](#sa09) |
| [D360](DECOMP.md?plain=1#L3448) | [RA10](#ra10) |
| [D361](DECOMP.md?plain=1#L3455) | [FR06](#fr06) |
| [D362](DECOMP.md?plain=1#L3460) | [NG33](#ng33) |
| [D363](DECOMP.md?plain=1#L3476) | [NG07](#ng07) |
| [D364](DECOMP.md?plain=1#L3491) | [NG39](#ng39) |
| [D365](DECOMP.md?plain=1#L3496) | [RA02](#ra02) |
| [D366](DECOMP.md?plain=1#L3499) | [TM08](#tm08) |
| [D367](DECOMP.md?plain=1#L3503) | [SA01](#sa01) |
| [D368](DECOMP.md?plain=1#L3511) | [NG24](#ng24) |
| [D369](DECOMP.md?plain=1#L3518) | [SA08](#sa08) |
| [D370](DECOMP.md?plain=1#L3523) | [LP06](#lp06) |
| [D371](DECOMP.md?plain=1#L3525) | [TM09](#tm09) |
| [D372](DECOMP.md?plain=1#L3530) | [TM04](#tm04) |
| [D373](DECOMP.md?plain=1#L3541) | [RA06](#ra06) |
| [D374](DECOMP.md?plain=1#L3552) | [SA10](#sa10) |
| [D375](DECOMP.md?plain=1#L3556) | [TM03](#tm03) |
| [D376](DECOMP.md?plain=1#L3563) | [NG07](#ng07) |
| [D377](DECOMP.md?plain=1#L3573) | [FR07](#fr07) |
| [D378](DECOMP.md?plain=1#L3580) | [SA06](#sa06) |
| [D379](DECOMP.md?plain=1#L3585) | [NG01](#ng01) |
| [D380](DECOMP.md?plain=1#L3590) | [TY13](#ty13) |
| [D381](DECOMP.md?plain=1#L3594) | [FR01](#fr01), [NG10](#ng10), [RA11](#ra11) |
| [D382](DECOMP.md?plain=1#L3602) | [NG14](#ng14), [RA12](#ra12) |
| [D383](DECOMP.md?plain=1#L3610) | [RC08](#rc08) |
| [D384](DECOMP.md?plain=1#L3616) | [BL16](#bl16) |
| [D385](DECOMP.md?plain=1#L3627) | [SA01](#sa01) |
| [D386](DECOMP.md?plain=1#L3643) | [SA08](#sa08) |
| [D387](DECOMP.md?plain=1#L3648) | [LP02](#lp02) |
| [D388](DECOMP.md?plain=1#L3666) | [TM04](#tm04) |
| [D389](DECOMP.md?plain=1#L3672) | [RA10](#ra10) |
| [D390](DECOMP.md?plain=1#L3680) | [RA07](#ra07), [RA08](#ra08) |
| [D391](DECOMP.md?plain=1#L3699) | [NG33](#ng33) |
| [D392](DECOMP.md?plain=1#L3707) | [TY12](#ty12) |
| [D393](DECOMP.md?plain=1#L3713) | [NG20](#ng20) |
| [D394](DECOMP.md?plain=1#L3721) | [FR07](#fr07) |
| [D395](DECOMP.md?plain=1#L3731) | [SA10](#sa10) |
| [D396](DECOMP.md?plain=1#L3735) | [TM04](#tm04) |
| [D397](DECOMP.md?plain=1#L3743) | [RA16](#ra16), [TM08](#tm08) |
| [D398](DECOMP.md?plain=1#L3749) | [NG01](#ng01), [NG25](#ng25), [TM06](#tm06) |
| [D399](DECOMP.md?plain=1#L3758) | [RA21](#ra21) |
| [D400](DECOMP.md?plain=1#L3761) | [NG02](#ng02), [RA06](#ra06) |
| [D401](DECOMP.md?plain=1#L3765) | [NG16](#ng16) |
| [D402](DECOMP.md?plain=1#L3771) | [TM06](#tm06) |
| [D403](DECOMP.md?plain=1#L3779) | [BL12](#bl12) |
| [D404](DECOMP.md?plain=1#L3789) | [CC05](#cc05), [RA14](#ra14) |
| [D405](DECOMP.md?plain=1#L3796) | [NG07](#ng07) |
| [D406](DECOMP.md?plain=1#L3800) | [NG19](#ng19) |
| [D407](DECOMP.md?plain=1#L3806) | [NG01](#ng01), [NG03](#ng03) |
| [D408](DECOMP.md?plain=1#L3810) | [TY13](#ty13) |
| [D409](DECOMP.md?plain=1#L3816) | [FR10](#fr10) |
| [D410](DECOMP.md?plain=1#L3839) | [RD03](#rd03) |
| [D411](DECOMP.md?plain=1#L3848) | [FP06](#fp06), [FR06](#fr06) |
| [D412](DECOMP.md?plain=1#L3851) | [TY13](#ty13) |
| [D413](DECOMP.md?plain=1#L3857) | [TM05](#tm05) |
| [D414](DECOMP.md?plain=1#L3867) | [NG07](#ng07) |
| [D415](DECOMP.md?plain=1#L3887) | [NG01](#ng01), [NG21](#ng21) |
| [D416](DECOMP.md?plain=1#L3899) | [NG01](#ng01) |
| [D417](DECOMP.md?plain=1#L3906) | [SA07](#sa07) |
| [D418](DECOMP.md?plain=1#L3910) | [NG46](#ng46), [RA13](#ra13) |
| [D419](DECOMP.md?plain=1#L3913) | [NG21](#ng21) |
| [D420](DECOMP.md?plain=1#L3920) | [NG07](#ng07) |
| [D421](DECOMP.md?plain=1#L3927) | [NG30](#ng30) |
| [D422](DECOMP.md?plain=1#L3936) | [NG10](#ng10), [RA08](#ra08) |
| [D423](DECOMP.md?plain=1#L3940) | [FR07](#fr07) |
| [D424](DECOMP.md?plain=1#L3949) | [TM05](#tm05) |
| [D425](DECOMP.md?plain=1#L3956) | [NG07](#ng07) |
| [D426](DECOMP.md?plain=1#L3966) | [SA01](#sa01) |
| [D427](DECOMP.md?plain=1#L3991) | [FP09](#fp09), [NG04](#ng04) |
| [D428](DECOMP.md?plain=1#L4000) | [CC05](#cc05), [NG04](#ng04) |
| [D429](DECOMP.md?plain=1#L4006) | [SA01](#sa01) |
| [D430](DECOMP.md?plain=1#L4012) | [TM10](#tm10) |
| [D431](DECOMP.md?plain=1#L4024) | [FR10](#fr10) |
| [D432](DECOMP.md?plain=1#L4027) | [NG12](#ng12) |
| [D433](DECOMP.md?plain=1#L4035) | [NG10](#ng10) |
| [D434](DECOMP.md?plain=1#L4040) | [RA16](#ra16) |
| [D435](DECOMP.md?plain=1#L4042) | [RA13](#ra13), [RD05](#rd05) |
| [D436](DECOMP.md?plain=1#L4044) | [TM05](#tm05) |
| [D437](DECOMP.md?plain=1#L4050) | [NG44](#ng44), [TM04](#tm04) |
| [D438](DECOMP.md?plain=1#L4055) | [NG31](#ng31) |
| [D439](DECOMP.md?plain=1#L4061) | [NG32](#ng32) |
| [D440](DECOMP.md?plain=1#L4074) | [RD06](#rd06) |
| [D441](DECOMP.md?plain=1#L4080) | [SA06](#sa06) |
| [D442](DECOMP.md?plain=1#L4083) | [TM04](#tm04) |
| [D443](DECOMP.md?plain=1#L4091) | [NG07](#ng07) |
| [D444](DECOMP.md?plain=1#L4099) | [FR01](#fr01), [NG10](#ng10) |
| [D445](DECOMP.md?plain=1#L4105) | [NG03](#ng03), [NG10](#ng10), [SA07](#sa07), [TY12](#ty12) |
| [D446](DECOMP.md?plain=1#L4114) | [RA17](#ra17), [TM04](#tm04) |
| [D447](DECOMP.md?plain=1#L4125) | [SA10](#sa10) |
| [D448](DECOMP.md?plain=1#L4134) | [FR05](#fr05) |
| [D449](DECOMP.md?plain=1#L4139) | [SA02](#sa02) |
| [D450](DECOMP.md?plain=1#L4145) | [NG01](#ng01) |
| [D451](DECOMP.md?plain=1#L4150) | [NG23](#ng23) |
| [D452](DECOMP.md?plain=1#L4160) | [NG23](#ng23), [TM04](#tm04) |
| [D453](DECOMP.md?plain=1#L4165) | [NG39](#ng39) |
| [D454](DECOMP.md?plain=1#L4186) | [NG38](#ng38) |
| [D455](DECOMP.md?plain=1#L4221) | [NG05](#ng05) |
| [D456](DECOMP.md?plain=1#L4227) | [RD04](#rd04) |
| [D457](DECOMP.md?plain=1#L4240) | [SA05](#sa05) |
| [D458](DECOMP.md?plain=1#L4247) | [SA05](#sa05) |
| [D459](DECOMP.md?plain=1#L4253) | [RA13](#ra13) |
| [D460](DECOMP.md?plain=1#L4260) | [FP07](#fp07) |
| [D461](DECOMP.md?plain=1#L4264) | [BL01](#bl01), [NG31](#ng31) |
| [D462](DECOMP.md?plain=1#L4279) | [TM10](#tm10) |
| [D463](DECOMP.md?plain=1#L4296) | [SA02](#sa02) |
| [D464](DECOMP.md?plain=1#L4308) | [RA01](#ra01) |
| [D465](DECOMP.md?plain=1#L4313) | [SA05](#sa05) |
| [D466](DECOMP.md?plain=1#L4319) | [SA10](#sa10) |
| [D467](DECOMP.md?plain=1#L4322) | [TM09](#tm09) |
| [D468](DECOMP.md?plain=1#L4326) | [TM03](#tm03) |
| [D469](DECOMP.md?plain=1#L4330) | [NG38](#ng38), [SA07](#sa07) |
| [D470](DECOMP.md?plain=1#L4340) | [NG07](#ng07), [SA07](#sa07) |
| [D471](DECOMP.md?plain=1#L4344) | [NG07](#ng07) |
| [D472](DECOMP.md?plain=1#L4347) | [NG01](#ng01) |
| [D473](DECOMP.md?plain=1#L4356) | [SA10](#sa10) |
| [D474](DECOMP.md?plain=1#L4366) | [FP09](#fp09), [RD07](#rd07) |
| [D475](DECOMP.md?plain=1#L4371) | [FR04](#fr04) |
| [D476](DECOMP.md?plain=1#L4377) | [NG37](#ng37), [SA02](#sa02) |
| [D477](DECOMP.md?plain=1#L4387) | [NG12](#ng12) |
| [D478](DECOMP.md?plain=1#L4399) | [FR04](#fr04) |
| [D479](DECOMP.md?plain=1#L4404) | [RD07](#rd07) |
| [D480](DECOMP.md?plain=1#L4411) | [RA13](#ra13), [TM09](#tm09) |
| [D481](DECOMP.md?plain=1#L4415) | [BL01](#bl01), [BL02](#bl02) |
| [D482](DECOMP.md?plain=1#L4435) | [TM03](#tm03) |
| [D483](DECOMP.md?plain=1#L4441) | [SA08](#sa08) |
| [D484](DECOMP.md?plain=1#L4447) | [RD06](#rd06) |
| [D485](DECOMP.md?plain=1#L4450) | [NG32](#ng32), [TM10](#tm10) |
| [D486](DECOMP.md?plain=1#L4461) | [LP01](#lp01) |
| [D487](DECOMP.md?plain=1#L4466) | [BL10](#bl10) |
| [D488](DECOMP.md?plain=1#L4469) | [FR04](#fr04) |
| [D489](DECOMP.md?plain=1#L4471) | [FR01](#fr01) |
| [D490](DECOMP.md?plain=1#L4473) | [FR01](#fr01) |
| [D491](DECOMP.md?plain=1#L4483) | [FR04](#fr04) |
| [D492](DECOMP.md?plain=1#L4487) | [BL13](#bl13) |
| [D493](DECOMP.md?plain=1#L4492) | [BL16](#bl16), [TM10](#tm10) |

## Historical triage snapshot

HANDOFF §6B labels this a 2026-09-05 sweep of all 37 partials. It is retained as measurement provenance. Later evidence elsewhere in this guide can improve a number or refute a proposed floor; this table is not a new audit and must not override those entries or the contract’s exhausted-body restrictions.

| mismatch | insns | address | function | file |
| --- | --- | --- | --- | --- |
| 3 | 43 | 0x004718c0 | ClampPopUpToScreen | misc3.c — **EXHAUSTED** |
| 3 | 330 | 0x00433840 | JcBoat_Animate | roads.c — **EXHAUSTED** |
| 3 | 482 | 0x00477bd0 | RequestRoute | simcore.c — **EXHAUSTED** |
| 5 | 205 | 0x0045f810 | ValidateCursor | objmap2.c — **EXHAUSTED, leave** |
| 5 | 637 | 0x0042aa90 | Balloonz_Tick | ridecb3.c |
| 8 | 218 | 0x0041a040 | BoatingSchool_Add | ridecb5.c — **EXHAUSTED, leave** |
| 10 | 222 | 0x0040bf70 | LFEntrance_Activate | lfentrance.c |
| 11 | 102 | 0x0040abf0 | LFEntrance_Remove | logflume.c |
| 12 | 31 | 0x00417e70 | WW_AnyBlokeInRect | waterworks.c — **EXHAUSTED** |
| 13 | 109 | 0x00473b00 | UpdateControllerFromMouseData | input.c — **EXHAUSTED** |
| 13 | 962 | 0x004724a0 | DrawPopUpInfo | popup.c |
| 15 | 141 | 0x00434f90 | JungleCruise_Add | ridecb9.c |
| 15 | 376 | 0x00416330 | SpiderRide_Activate | mechrides.c |
| 19 | 362 | 0x0043c950 | SpinningBarrels_Activate | mechrides.c |
| 19 | 387 | 0x0043e410 | PlaneRide_Activate | mechrides.c |
| 20 | 191 | 0x0048a3e0 | GetObjectUID | objmap2.c |
| 22 | 68 | 0x00475630 | InsertChildIntoList | fpui.c — **EXHAUSTED** |
| 22 | 347 | 0x00417430 | TempleSlide_Update | joust.c |
| 27 | 64 | 0x00413450 | Road_FindDiagonals | ridecb5.c |
| 27 | 116 | 0x00436dc0 | JungleCruise_UpdateRiverTile | junglecruise.c |
| 29 | 378 | 0x0042c820 | Carousel_Tick | ridecb3.c |
| 30 | 212 | 0x00418fe0 | BoatingSchool_DrawBoats | anim2.c |
| 44 | 111 | 0x00410180 | LFDrop_Place | logflume2.c |
| 47 | 358 | 0x0041a720 | BoatingSchool_Tick | ridecb5.c |
| 82 | 184 | 0x00470620 | CheckWorkerOnMouseStatus | workers2.c |
| 112 | 422 | 0x00432d00 | JungleCruise_UpdateRiverAnim | junglecruise.c |
| 118 | 119 | 0x0048f0f0 | InitExitCheckBox | screens2.c |
| 132 | 402 | 0x00415220 | SafariRide_Activate | mechrides.c |
| 138 | 222 | 0x0043bac0 | SpaceTower_Activate | mechrides.c |
| 182 | 331 | 0x00442040 | AnimApplyPart | anim2.c |
| 206 | 256 | 0x0040a600 | LFEntrance_Add | lfentrance.c |
| 208 | 354 | 0x00435750 | JungleCruise_Tick | ridecb2.c |
| 273 | 454 | 0x0045ff00 | RenderCursor | bigrender.c |
| 319 | 351 | 0x00402780 | StepSchoolCar | goldrush.c |
| 377 | 1023 | 0x00440a30 | Draw3DPersonModel | person3d.c — **EXHAUSTED, leave** |
| 381 | 903 | 0x0045b180 | RenderView | renderview.c |
| 844 | 1161 | 0x004567a0 | RenderFullMap | renderview.c |

## Maintenance

Append discoveries to DECOMP under the normal integration process, then amend the existing rule here when the mechanism is already covered. Add evidence and scope restrictions instead of another rule with a near-identical title. Preserve conflicting measurements with their provenance until a direct measurement resolves them. Scope M changes only this file. No source entry is left unplaced; anonymous measurement sites and unresolved numeric conflicts are identified in their entries.
