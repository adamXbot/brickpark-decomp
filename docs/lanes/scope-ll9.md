# Scope LL9 — unreferenced (dead) functions, 0x00401e00..0x004207b0

Completed 2026-09-08 on `scope/LL9`, based on `main` at `7d756410`.
File: `LEGOLAND/unref1.c` (new). Brief:
[`docs/SCOPE_LL9_unref_rides_flume_coaster_a.md`](../SCOPE_LL9_unref_rides_flume_coaster_a.md).

**27 of 31 exact, 930 instructions assigned, 736 of them in exact bodies.**
The four remaining bodies are structurally complete and honest WIPs: three
are a single register-allocation tie-break each, and the fourth
(`ZBuffer_FillShadedPoly`, a hand-written-assembly function) is exact in
instruction count, byte length and its whole `__asm` region, with the
residual in twelve frame homes and one CSE hoist.

Validation: `tools/audit.py LEGOLAND/unref1.c` ends **PASS** with 27 `[OK]`
lines and 4 `[WIP]`; `tools/relocs.py` reports **zero MISMATCH** across 34
relocations (the 5 unresolved are x87 constant pool literals — 0.5f, 1.0f
and 1/6, each read out of the original and checked); the file compiles
clean under `/W3 /O2 /Gy /Gd`. No other file was touched.

## Status table

| Address | Name | Insns | % | audit | Marker | Residual |
| --- | --- | ---: | ---: | --- | --- | --- |
| `0x00401e00` | `SchoolCarPushWaypoint` | 69 | 100 | [OK] | FUNCTION | — |
| `0x00403d60` | `Copters_RestoreRider` | 12 | 100 | [OK] | FUNCTION | — |
| `0x00408f90` | `LFTrack_FindPieceCovering` | 50 | 80 | [WIP] | WIP-FUNCTION | 40 strict, first diff i=1 |
| `0x0040adb0` | `LFPiece_ShadeForRow` | 69 | 4 | [WIP] | WIP-FUNCTION | 68/69 insns, 66 strict, first diff i=3 |
| `0x0040b270` | `LFBoat_IsAtPiece` | 7 | 100 | [OK] | FUNCTION | — |
| `0x0040bd40` | `LFBoat_StepAtStation` | 76 | 100 | [OK] | FUNCTION | — |
| `0x0040c250` | `LFPiece_MoveBoatsOff` | 64 | 11 | [WIP] | WIP-FUNCTION | 57 strict, first diff i=1, ESCAPES |
| `0x00411e20` | `Unref_00411e20` | 1 | 100 | [OK] | FUNCTION | — |
| `0x00411f70` | `LFQueue_IsFrontRider` | 13 | 100 | [OK] | FUNCTION | — |
| `0x004120e0` | `WalkPath_GetPoint` | 7 | 100 | [OK] | FUNCTION | — |
| `0x0041b130` | `BoatingSchoolLibMain` | 7 | 100 | [OK] | FUNCTION | — |
| `0x0041cf20` | `GetNodeEndSteps` | 26 | 100 | [OK] | FUNCTION | — |
| `0x0041d040` | `TrackNode_ClassPlace` | 6 | 100 | [OK] | FUNCTION | — |
| `0x0041d050` | `TrackNode_ClassRemove` | 6 | 100 | [OK] | FUNCTION | — |
| `0x0041e260` | `Route_SeatCar` | 32 | 100 | [OK] | FUNCTION | — |
| `0x0041e2f0` | `Route_UnseatCar` | 25 | 100 | [OK] | FUNCTION | — |
| `0x0041e720` | `RouteNode_SeatCar` | 27 | 100 | [OK] | FUNCTION | — |
| `0x0041e790` | `RouteNode_UnseatCar` | 23 | 100 | [OK] | FUNCTION | — |
| `0x0041e7c0` | `RouteNode_ClearSeats` | 14 | 100 | [OK] | FUNCTION | — |
| `0x0041ef10` | `Unref_0041ef10` | 1 | 100 | [OK] | FUNCTION | — |
| `0x0041f350` | `Span_ClipPolygonToSet` | 14 | 100 | [OK] | FUNCTION | — |
| `0x0041f5a0` | `PhysVec_Derivative4` | 63 | 100 | [OK] | FUNCTION | — |
| `0x0041f650` | `BuildDifferenceTable` | 80 | 100 | [OK] | FUNCTION | — |
| `0x0041f710` | `DiffTable_Free` | 5 | 100 | [OK] | FUNCTION | — |
| `0x0041f720` | `NumericDerivative` | 34 | 100 | [OK] | FUNCTION | — |
| `0x0041f790` | `ExpDerivs` | 33 | 100 | [OK] | FUNCTION | — |
| `0x0041f7e0` | `TestFn_Log` | 4 | 100 | [OK] | FUNCTION | — |
| `0x0041f7f0` | `MathSelfTest` | 22 | 100 | [OK] | FUNCTION | — |
| `0x0041fa10` | `ZBuffer_FillShadedPoly` | 134 | 77 | [WIP] | WIP-FUNCTION | 134/134 insns, 393/393 B, 31 strict, first diff i=22 |
| `0x00420520` | `Unref_00420520` | 1 | 100 | [OK] | FUNCTION | — |
| `0x004207b0` | `FreeCoasterColours` | 5 | 100 | [OK] | FUNCTION | — |

("%" is the strict index-for-index agreement over the original's extent.)

## Mechanics recovered

### The route's seat table — the dead half of coaster8.c's mechanism

coaster8.c recovered the live half: a `RouteNode` is 0xec bytes with its two
0x20-byte seats inline at +0x78/+0x98 and its ring links at +0xe4/+0xe8, and
a seat carries four inline method slots at +0x10..+0x1c (occupied / attach /
detach / update). Five dead bodies complete that interface:

* `RouteNode_SeatCar` (0x0041e720) puts a car in the node's first EMPTY seat
  and returns 1; `RouteNode_FindFreeSeat` (0x0041e760, live) is the same walk
  that returns the seat instead of filling it.
* `RouteNode_UnseatCar` (0x0041e790) takes the car out of the first OCCUPIED
  seat, through coastertiny.c's `RouteSeat_TakeCar` (0x004273e0).
* `RouteNode_ClearSeats` (0x0041e7c0) runs both seats' detach slots.
* `Route_SeatCar` (0x0041e260) and `Route_UnseatCar` (0x0041e2f0) are the
  ring-wide wrappers, a `do/while` over the embedded sentinel at
  `route + 0x70` exactly like coastertiny.c's `Route_ForEachNode`.

So the route knows how to seat and unseat cars by itself; the shipped game
only ever uses the "find me a free seat" half.

### The log flume

* `LFBoat_IsAtPiece` (0x0040b270) is the STRICT "is this boat on this piece"
  test — posstep.c's live `LFBoat_IsOnPiece` (0x0040b210) also answers yes
  for the neighbour a boat overhangs.
* `LFQueue_IsFrontRider` (0x00411f70) asks whether a bloke is the rider at
  the head of a station's boarding queue (`q->head->rider->bloke == b`).
* `LFTrack_FindPieceCovering` (0x00408f90) is the AREA version of posstep.c's
  `LFTrack_FindPiece`: instead of an exact square compare it accepts anything
  inside the flume cell's own span, taken as
  `g_lf_footprint.v[2] - v[0]` by `v[3] - v[1]`.
* `LFPiece_ShadeForRow` (0x0040adb0) maps a screen row onto the game's
  darkness ramp for one placed piece. It turns the class footprint's two
  corners into tile bounds (`GetTileBounds`, 0x0045acc0), measures `y` from
  the top corner's top edge against the bottom corner's bottom edge, divides
  by that height times a caller-supplied scale, clamps to 0..1 and returns
  `0x20 - (int)(r * -192.0f)` — i.e. a band in 0x20..0xe0. Its third
  parameter is never read.
* `LFBoat_StepAtStation` (0x0040bd40) is the dead twin of `LFBoat_Step`
  (0x0040bbb0) with the STATION handling folded in: a moving boat drops its
  rider at `run->f0c` (bumping the rider's script step at Bloke +0x60 and
  clearing the seat), parks with a 50-frame timer at `run->f08`, and
  otherwise creeps one piece forward whenever `LFBoat_IsWayClear`
  (0x0040bab0) allows; a parked boat counts its timer down and restarts
  unless a boat is already loading, resetting the timer to 1 either way.
  Its leading `if (boat)` is a null test on `&run->boats[idx]`, which cannot
  be null — reproduced.
* `LFPiece_MoveBoatsOff` (0x0040c250) is the piece-removal helper: it moves
  every boat standing on a piece (or on one of its sub-pieces) onto the
  piece's previous neighbour, or its next one, or — when it has neither —
  onto the piece itself, because the destination cursor is the parameter and
  keeps its incoming value. A piece that owns a sub-list hands the boats to
  its first sub-piece instead. The boat count is re-read from the run record
  in the loop latch, matching posstep.c's note on `LFRun_Start`.

### The driving school

`SchoolCarPushWaypoint` (0x00401e00) is the exact mirror of schoolcar4.c's
`SchoolCarIdleStep` (0x00401cd0). That one shifts `wp[1..16]` DOWN over
`wp[0]` and drops `nwp`; this one shifts `wp[0..15]` UP to make room at the
front and raises `nwp`. **Both carry the same one-past-the-end bug at their
own end of the array**: the idle step's last copy READS `wp[16]`, and this
one's first copy WRITES it — i.e. over the cached unit heading at
+0xb0/+0xb4. Like its twin it is sixteen separate 8-byte struct assignments,
not a loop.

### The copters' save index

ridetiny.c's `Copters_StepRider` (0x00403d30) replaces a rider's animation
path POINTER with its ordinal in the table at 0x004c1124 so the ordinal can
go into a save file. `Copters_RestoreRider` (0x00403d60) is the load-side
twin, and like its live partner it walks SIX slots of a five-entry table; an
out-of-range ordinal (which is what the live half's `-1` miss produces)
clears the path instead.

### The coaster track and the boating school

* `TrackNode_ClassPlace` / `TrackNode_ClassRemove` (0x0041d040/0x0041d050)
  are one-line dispatch shims onto `TrackDesc` +0x28 and +0x2c, the two
  class hooks coaster.c documents.
* `GetNodeEndSteps` (0x0041cf20) asks schoolcar.c's `GetOpenEndSteps`
  question of ONE piece rather than the whole coaster record, through the
  piece's two joints, and answers -1 rather than 0 for "no neighbour".
* `FreeCoasterColours` (0x004207b0) is the counterpart of coastertiny.c's
  `GetCoasterColours` (0x004207c0): it frees the module colour table
  `g_coaster_colours`.
* `BoatingSchoolLibMain` (0x0041b130) is the **DllMain of a DLL that was
  compiled into the exe**. loaders.c recovered the object-library mechanism:
  a class whose ODF sets `OC_USEDLL` loads `.\dlls\<name>.dll`, whose
  start-up code registers its `GetInterfaces` into the scratch record
  `g_objlib_cur` points at (+0x0c). This is exactly that start-up code for
  the BOATING SCHOOL family, `__stdcall` with three arguments, registering
  loaders.c's `GetInterface` (0x0041b150) on `DLL_PROCESS_ATTACH` — dead
  because the classes were built in instead.

### The numerical-methods corner (0x0041f2b0..0x0041fa10)

coaster7.c recovered the live half of this address block (`Span_SetClip`, the
physics plumbing). The dead half is a small NUMERICAL LIBRARY plus its own
scratch `main`, all of it unreferenced:

* `BuildDifferenceTable` (0x0041f650) samples `f` at `n+1` points spaced `h`
  apart and centred on `x` (`x -= (n >> 1) * h` first), then reduces them in
  place into a forward-difference triangle. The pointer array and the
  triangle are ONE zeroed allocation of
  `(((n+2)*(n+1) >> 1) + (n+1)) * sizeof(float)` bytes, `n+1` row pointers
  followed by rows that shorten by one each time.
* `NumericDerivative` (0x0041f720) halves `h`, builds a five-point triangle
  and returns
  `((D1[2] + D1[1]) - (D3[1] + D3[0]) / 6) / (2 * (h/2))`, freeing the table
  through `DiffTable_Free` (0x0041f710). A failed allocation returns 0 with
  no `xor` — the table call already left zero in eax.
* `PhysVec_Derivative4` (0x0041f5a0) is the vector form, over the PhysOps
  hooks (alloc +0x20, free +0x24, scale +0x0c, add +0x00): the stencil
  `{-1, -1/3, +1/3, +1}` at 0x004b5614 with the weights
  `{1/16, -27/16, +27/16, -1/16}` at 0x004b5624, the first point peeled out
  of the loop. It takes TWO scratch vectors from the pool but uses only the
  second: the caller's `out` doubles as the accumulator.
* `TestFn_Log` (0x0041f7e0) and `ExpDerivs` (0x0041f790) are the two sample
  functions — `log(x)` as VC6's `fldln2/fyl2x` intrinsic, and a three-
  component state `{e^x, 2e^x, 2e^(2x)}`.
* `Span_ClipPolygonToSet` (0x0041f350) unpacks a `ClipSet` into the plane
  count and plane array the Sutherland-Hodgman clipper at 0x0041f2b0 takes.
* `MathSelfTest` (0x0041f7f0) calls each of them once and throws every result
  away: differentiate `log` at 3 with h = 1, then `CarPoolInit`,
  `PhysVec_InitOps(&ops, 3)` and the vector derivative of `ExpDerivs` at 3
  with h = 0.01. Every cdecl cleanup in it merges into one `add esp,0xb0`.

### The shaded, z-buffered span filler (0x0041fa10)

The dead twin of schoolcar6.c's `ZBuffer_FillPoly` (0x00423350): the same
two-chain scanline filler, the same key/edge arrays, the same pre-biased
16.16 chains, the same half-pixel span test — and the same hand-written
assembly region, on all of schoolcar6.c's proofs (an EBP frame in an /O2
file, `xchg ebx,eax`, `add ebx,1` where VC6 emits `inc ebx`, and the
`cmp ecx,8000h / jns wide / jmp done` pair where a single `js done` would do).

What this one adds over its live twin, and it settles an open question:

* it PAINTS instead of clearing — the pixel value is one entry of one of the
  0x400 shade ramps at 0x00829c60, indexed by the ramp number (argument 1)
  and the level `src[0]`;
* the LEFT chain carries a second interpolant, a 16.16 DEPTH, whose value
  and step live at +0x08 of the same 0x14-byte records (so the module's
  span-edge record really is five ints, of which x and z are used here); and
* it writes BOTH targets. **0x004b5b20 is the 16-bit COLOUR target's base** —
  schoolcar6.c could only record that `ZBuffer_FillPoly` copies that global
  into a local and never reads it; this function is what it is for. The
  depth, taken from the left chain and stepped by `src[1]` per pixel, goes to
  the z-buffer at 0x004b5b24. Both rows advance by `pitch * 2` bytes, which
  VC6 hoists into the dead `n` argument slot.

## Names chosen

No function in this scope is exported, so every name is ours. Named for
behaviour in project style, after grepping `LEGOLAND/*.c` for collisions:

`SchoolCarPushWaypoint`, `Copters_RestoreRider`, `LFTrack_FindPieceCovering`,
`LFPiece_ShadeForRow`, `LFBoat_IsAtPiece`, `LFBoat_StepAtStation`,
`LFPiece_MoveBoatsOff`, `LFQueue_IsFrontRider`, `WalkPath_GetPoint`,
`BoatingSchoolLibMain`, `GetNodeEndSteps`, `TrackNode_ClassPlace`,
`TrackNode_ClassRemove`, `Route_SeatCar`, `Route_UnseatCar`,
`RouteNode_SeatCar`, `RouteNode_UnseatCar`, `RouteNode_ClearSeats`,
`Span_ClipPolygonToSet`, `PhysVec_Derivative4`, `BuildDifferenceTable`,
`DiffTable_Free`, `NumericDerivative`, `ExpDerivs`, `TestFn_Log`,
`MathSelfTest`, `FreeCoasterColours`, `ZBuffer_FillShadedPoly`.

Three one-byte `ret` bodies carry no behaviour at all and take the brief's
fallback names, recorded here: **`Unref_00411e20`** (in the `lfmisc.c`
queue neighbourhood), **`Unref_0041ef10`** (schoolcar.c) and
**`Unref_00420520`** (next to `CoasterModel_SetDirectory`).

## Callees and globals named for the first time

| Address | Name | What it is |
| --- | --- | --- |
| `0x004b5b20` | `g_zb_colour` | the 16-bit COLOUR target's base (schoolcar6.c could only record that its twin copies it and never reads it) |
| `0x0041f2b0` | `ClipPolygonPlanes` | the Sutherland-Hodgman clipper behind `Span_SetClip`'s half-plane list; `(count, verts, *out_count, nplanes, planes)` |
| `0x0041f4e0` | `PhysVec_DerivativeTable` | the difference-table vector derivative (the one `MathSelfTest` calls; NOT `PhysVec_Derivative4`) |
| `0x0041f3e0` | — | the vector difference-table builder `PhysVec_DerivativeTable` calls (not in this scope) |
| `0x0040bab0` | `LFBoat_IsWayClear` | may boat `idx` of a run move on to the next route piece? |
| `0x004b5614` | `g_deriv_offsets[4]` | the four-point stencil `{-1, -1/3, +1/3, +1}` |
| `0x004b5624` | `g_deriv_weights[4]` | its weights `{1/16, -27/16, +27/16, -1/16}` |

Existing names reused unchanged: `Free_w`, `AllocZeroed`, `CarPoolInit`,
`PhysVec_InitOps`, `RouteSeat_TakeCar` (coastertiny.c's 0x004273e0),
`Track_CountHeadPieces`, `Track_CountTailPieces`, `GetTileBounds`,
`GetInterface`, `g_objlib_cur`, `g_copters_paths`, `g_lf_queue`,
`g_lf_footprint`, `g_coaster_colours`, `g_zb_base`, `g_zb_pitch`,
`g_zb_polys`, `g_shade_tab`.

## Original bugs reproduced

* `SchoolCarPushWaypoint` writes `wp[16]`, one past the waypoint array, over
  the cached unit heading at SchoolCar +0xb0/+0xb4 — the mirror of the read
  past the end schoolcar4.c records in `SchoolCarIdleStep`.
* `LFBoat_StepAtStation` tests `&run->boats[idx]` for null, which it can
  never be.
* `Copters_RestoreRider` indexes SIX slots of the five-entry copter path
  table, exactly as its live partner does.

## Extern-type divergences

* `PhysVec_DerivativeTable` (0x0041f4e0) is declared here with a
  `void (*)(float, PhysVec*)` first parameter and a `PhysOps*` second; no
  other file declares it.
* `g_coaster_colours` is `void*` here (as in coastertiny.c's `g_4d8bac`),
  where coaster8.c declares the same object `int*` because it indexes it.
* `g_zb_colour` (0x004b5b20) is `short*` here; schoolcar6.c declares the same
  object `int g_zb_4b5b20` because it only copies it as a dword.

## Levers learned (with evidence)

1. **A ring walk's sentinel must be spelled at BOTH ends, not named.**
   `Route_SeatCar` / `Route_UnseatCar` (0x0041e260 / 0x0041e2f0):
   writing `head = &route->head; node = head; do {...} while (node != head);`
   sinks the `mov esi,edi` copy PAST the argument push (`push edi` then
   `mov esi,edi`), 2 and 3 strict. Spelling `&route->head` at both the
   cursor's initialiser and the `do/while` latch — so the `lea` is a hoisted
   loop invariant and the cursor's initialiser is a separate copy statement —
   emits the original's `mov esi,edi / push esi` and closes both bodies.
   coastertiny.c's live `Route_ForEachNode` does not need this because its
   call is indirect and returns void.

2. **A single-case `switch` keeps `dec eax / jne` where an `if` shares the
   literal with the return value.** `BoatingSchoolLibMain` (0x0041b130):
   `if (reason == 1) {...} return 1;` lets VC6 form a constant web and emit
   `mov eax,1 / cmp ecx,eax` (6 of 7 strict). `switch (reason) { case 1: ...
   break; } return 1;` is byte-exact.

3. **A `__stdcall` body must be the FIRST function in its file to be
   gateable.** `tools/match.py`'s `obj_function_code` looks for `Name` and
   `_Name` only, so a `_Name@N` COMDAT is never found and the lookup falls
   through to "the first `.text` section" — which under `/Gy` is the first
   function emitted. `BoatingSchoolLibMain` audited against
   `Unref_00411e20`'s single `ret` until it was moved to the top of the file.
   (misc3.c's `LLIDB_SelectDlgProc` audits only because it happens to be
   first in its own file.) `tools/relocs.py` is unaffected — it resolves the
   decorated symbol itself.

4. **An 8-byte struct return is a struct COPY, not two field assignments.**
   `WalkPath_GetPoint` (0x004120e0): building the result field by field
   (`p.x = ...; p.y = ...; return p;`) costs a `lea` for the second load
   (6 of 7). Returning the sub-object whole — `return path->nodes[i].pos;`
   with the node declared `{ Pos pos; int state; }` — reuses the
   `base + index*4` addressing for both halves and is byte-exact.

5. **Two lockstep cursors: the counter goes in the for-increment, the other
   one in the body — or both in the increment, counter first.**
   `BuildDifferenceTable` (0x0041f650) row setup: with `p += len; len--;` as
   body statements VC6 emits `sub eax,4 / add ecx,4` (stride before cursor);
   `for (i = 0; i <= n; i++, len--) { tab[i] = p; p += len; }` emits the
   original's `add eax,4 / sub ecx,4`. This is the recorded latch-order rule
   in its two-cursor form; coaster8.c saw the same on `RouteNode_FindFreeSeat`.

6. **A difference-table inner loop must be spelled 0-based.**
   Same function: `for (j = 1; j <= len; j++) row[j-1] = prev[j] - prev[j-1];`
   makes VC6 start the counter at 1, rotate the `inc` to the top anyway and
   shift every displacement by one (`[ecx+eax*4-4]` / `[ecx+eax*4-8]`).
   `for (j = 0; j < len; j++) row[j] = prev[j+1] - prev[j];` gives
   `xor eax,eax / test esi,esi / jle` and the original's `+0` / `-4`
   displacements. 14 of 80.

7. **A scratch buffer's base must be assigned BEFORE the value that offsets
   it, as two statements.** `ZBuffer_FillShadedPoly` (0x0041fa10):
   `row = g_zb_colour + pitch * y;` leaves both base loads at the bottom of
   the set-up (105 strict); `zrow = g_zb_base; row = g_zb_colour;` right
   after `pitch`, with `row += pitch * y; zrow += pitch * y;` at the end of
   the set-up, puts the two `mov reg,[global]` loads where the original has
   them, high up among the callee-saved pushes, and takes the body to 31.
   This is the same effect schoolcar6.c gets on its twin with a free
   `volatile` read of `y`; splitting the statement is cheaper and stronger.

8. **A dead `__asm` twin is worth reading before reconstructing.** The whole
   of `ZBuffer_FillShadedPoly`'s hand-written region came straight from
   schoolcar6.c's `ZBuffer_FillPoly`, including the `ZInterp` 0x14-byte
   record shape, the `ed[20]` byte-offset addressing MSVC's inline assembler
   forces, the `jns/jmp` pair and the negative-index fill loop. Five minutes
   of grepping for a sibling address range beat any amount of guessing.

9. **Two byte fields compared in one predicate want two named locals.**
   `LFTrack_FindPieceCovering` (0x00408f90): with the fields spelled inline
   in the four comparisons VC6 zero-extends each one where it is used;
   `int px = p->sq.b.x; int py = p->sq.b.y;` at the top of the loop body
   produces the original's two zero registers
   (`xor ecx,ecx / xor edx,edx / mov cl / mov dl`). Same shape logflume4.c
   records for `LFTrack_DrawAlt`, and it is what makes the rest of that body
   line up.

### Measured negatives

* **`LFTrack_FindPieceCovering` (0x00408f90), 50/50 instructions, 40 strict.**
  Block layout, the two zero registers, all four compares and both epilogues
  are index-for-index right. The residual is one register-allocation choice:
  the original spills the RUN cursor into the single stack dword (`push ecx`)
  and keeps both footprint spans in ebp/edi; VC6 gives us edi for `run` and
  spills the y-span instead. Inert at 39-40: `while`/`for`/`do-while`
  spellings, reading the head first or last, two-definition spans
  (`h = v[3]; h -= v[1];`), a `Footprint*` local, function-scope cursors,
  split `continue` guards and swapping the two span initialisers. A
  `volatile` run is worse (48).
* **`LFPiece_ShadeForRow` (0x0040adb0), 68 of 69 instructions, 66 strict.**
  Everything from the second `GetTileBounds` on is right, including the frame
  (one `Pos` and one `TileBounds` reused by both calls), the merged
  `add esp,0x10`, the two spills into the dead argument-2 slot, the
  `fild/fmul/fdivp` order, both clamp arms and the
  `0x20 - __ftol(r * -192)` tail. The deficit is ONE callee-saved push: the
  original holds seven values at once (the footprint pointer, both square
  bytes and all four corner sums) and pushes ebx/ebp/esi/edi, putting x0 in
  edx and y0 in edi; VC6 lets `sq.x` die before it loads `v[1]`, needs six,
  and pushes three. Inert at 65-67: loading all four `v[]` before the adds,
  every permutation of the four sum statements, named `sq.x`/`sq.y` locals,
  a `const int* v = fp->v` cursor, storing the first point early, and
  storing y before x.
* **`LFPiece_MoveBoatsOff` (0x0040c250), 64/64 instructions, 57 strict.**
  The block layout is right — the two destination stores CROSS-JUMP into one,
  the sub-list arm is exiled past the `mov eax,1` epilogue, ebx and esi are
  pushed INSIDE the boat-loop path, and the boat count is re-read from the
  run local in the latch. The residual is one allocation choice: the original
  enregisters the PIECE in ebp and homes the destination cursor in the
  parameter's own stack slot; VC6 does the reverse for us whichever way the
  two are spelled, which is also why a branch still escapes the extent.
  Inert at 57-63: a plain `dest` local left uninitialised on the
  no-neighbour path, reassigning the parameter itself, a merged
  `n = prev; if (!n) n = next;` temporary, declaring the copy before the run
  record, and indexing `run->boats[i]` instead of walking a cursor.
* **`ZBuffer_FillShadedPoly` (0x0041fa10), 134/134 instructions, 393/393
  bytes, 31 strict.** Twelve of the residual are frame homes: the original
  keeps `row`, `zrow`, `pitch` and `ylast` in the frame and gives the three
  dead argument slots to the colour, the z step and VC6's own hoisted
  `pitch*2`; VC6 hands the two POINTERS two of those argument slots instead.
  Declaration order is inert across all seven orders tried — schoolcar6.c
  records the same negative for its twin — as is `int` versus `short` for the
  colour. Declaring the `src` parameter `int` and assigning the z step into
  it DOES move the z step to +0xc, but pushes the colour out of +8 for no net
  gain. The other fourteen are the per-key edge switch: the original hoists
  `e->x` above the branch and loads `e->step` inside each arm, VC6 hoists
  `e->step`. All six orderings of the two arms' stores are inert (VC6
  reorders the adjacent stores anyway) and naming the hoisted value is worse
  — 55 steps-first, 70 values-first, 103 with both named.
