# codex-a — coaster math and school-car micro-helpers

Completed in an isolated worktree on `codex/scope-a`, based on main `9d7354e`.
Only the two assigned new C files and this report are committed.

**63 functions pass the authoritative audit:** 10 in `coastermath.c`, 53 in
`schoolcar8.c`. One additional complete function, `MatMul`, matches all 40
instructions and 103 bytes, but remains WIP because the audit's extent is
incorrect. One supplied address is not a function entry and was skipped.

The school-car set is exactly the 53 unmatched `Sub_*` entries of 8–29
instructions declared by `schoolcar.c` or `schoolcar4.c`, within the scope's
explicit address interval 0x0041cca0..0x00429270, excluding the math-owned
0x00425d50. The wider suggested `callees.py` filter also returns 0x004294b0,
0x0042a620 and 0x0042a640; those are outside that interval and were not written.
None of the scope's excluded addresses or other lanes' files was changed.

## Verification

- Both committed C files: `audit.py` PASS, with 63 `[OK]` and one honest WIP.
- Each of the 64 bodies: `matchfull.py` 100%; every object used a unique
  `/tmp/ca_<Name>.obj` path.
- Both files compile with `/W3 /O2 /Gy /Gd`, with no warnings.
- An additional scratch-only COFF relocation check resolves extern addresses
  from their source comments, validates pooled constant/string contents, and
  compares every byte against the original at the full function extent.
  All 64 bodies pass: 77 address relocations and 13 constant relocations.
  Internal branch displacements are compared literally in this check.
- No duplicate definition addresses or symbol names; all 53 school-car
  targets are covered exactly once. No `tools/` edits, global verification,
  progress generation, coverage generation, or debug objects.


The 63 audit-counted bodies total **1079 instructions / 3150 bytes**. Including the full MatMul body: 1119 instructions / 3253 bytes.


## coastermath.c

Percentage is the full-body instruction match. `FUNCTION` denotes the
committed `// FUNCTION: LEGOLAND <address>` marker; exact rows have no diverging
instruction or residual. All indices in the exception notes are zero-based.

| Address | Name and reason | Instructions | Match | Audit [OK] | Marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00426120 | `MatMul` — Multiply two row-major 4x4 matrices | 40 | 100% | No | WIP-FUNCTION |
| 0x00425d30 | `Vec3Dot` — Three-component dot product | 11 | 100% | Yes | FUNCTION |
| 0x004260f0 | `MatIdentity` — Fill a 4x4 identity matrix | 15 | 100% | Yes | FUNCTION |
| 0x0041ef20 | `SetSpanClip` — Forward reordered top/left/bottom/right clip bounds | 16 | 100% | Yes | FUNCTION |
| 0x004264e0 | `MakeTransform` — Expand a 3x3 basis and insert translation | 16 | 100% | Yes | FUNCTION |
| 0x0041ebd0 | `PackTrackClass` — Find the six-row castle interface index for a class | 16 | 100% | Yes | FUNCTION |
| 0x00426ab0 | `FastSqrt` — Table-interpolated square root, ST(0) ABI | 21 | 100% | Yes | FUNCTION |
| 0x00426980 | `FastRSqrt` — Table-interpolated reciprocal square root, ST(0) ABI | 21 | 100% | Yes | FUNCTION |
| 0x00425de0 | `Invert2x2` — Invert a contiguous 2x2 matrix in place | 21 | 100% | Yes | FUNCTION |
| 0x0041d1d0 | `BuildJoint` — Clear a joint slot, add map offsets and set its direction | 21 | 100% | Yes | FUNCTION |
| 0x00425d50 | `Vec3_Normalize` — Normalize three components using reciprocal square root | 29 | 100% | Yes | FUNCTION |

## schoolcar8.c

Percentage is the full-body instruction match. `FUNCTION` denotes the
committed `// FUNCTION: LEGOLAND <address>` marker; exact rows have no diverging
instruction or residual. All indices in the exception notes are zero-based.

| Address | Name and reason | Instructions | Match | Audit [OK] | Marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0041e0e0 | `Route_StepPrimary` — Dispatch the primary route step through the coaster's +110 hook | 8 | 100% | Yes | FUNCTION |
| 0x00421a70 | `TrackCurve_GetLimits` — Report the two endpoints of a curve's parameter interval | 8 | 100% | Yes | FUNCTION |
| 0x004225b0 | `CoasterModel_GetRecordName` — Read one object-model record's name | 8 | 100% | Yes | FUNCTION |
| 0x00426ea0 | `TrackRef_FindPiece` — The save reference's second dword is the packed map square | 8 | 100% | Yes | FUNCTION |
| 0x0041e930 | `RouteNode_GetTailTangent` — The second cursor of a route node starts at +40; mode 2 is tangent | 9 | 100% | Yes | FUNCTION |
| 0x00421320 | `PhysVec_Copy` — Vector-op slot 2 copies the entire 0x54-byte pool slot, not only n floats | 9 | 100% | Yes | FUNCTION |
| 0x004215b0 | `CoasterCar_DetachSeat` — Drop the seat's occupant and then clear the car's seat link | 9 | 100% | Yes | FUNCTION |
| 0x00421c80 | `TrackCurve_GetQuarterTurnSamples` — Six samples span a quarter turn, using the original rounded constants | 9 | 100% | Yes | FUNCTION |
| 0x00422620 | `CoasterModel_FormatIndexedName` — Build the base-name plus four-digit model index | 9 | 100% | Yes | FUNCTION |
| 0x004219f0 | `TrackCurve_LineTangent` — A straight segment's tangent is constant; the parameter is ignored | 10 | 100% | Yes | FUNCTION |
| 0x0041cee0 | `Track_CountHeadPieces` — Count predecessor pieces until NULL or either boundary flag (mask 6) | 11 | 100% | Yes | FUNCTION |
| 0x0041cf00 | `Track_CountTailPieces` — Same walk toward the tail | 11 | 100% | Yes | FUNCTION |
| 0x00421540 | `PhysVec_InitOps` — Copy ten vector hooks plus the requested dimension into a descriptor | 11 | 100% | Yes | FUNCTION |
| 0x00421590 | `CoasterCar_AttachSeat` — Establish both directions of the car-to-seat association | 12 | 100% | Yes | FUNCTION |
| 0x00427050 | `CoasterCar_WriteRef` — Persist a car's state and its rider's save-list index | 12 | 100% | Yes | FUNCTION |
| 0x00420750 | `CoasterModel_LoadLTX` — Load the .ltx file associated with a model base name | 13 | 100% | Yes | FUNCTION |
| 0x00421340 | `PhysVec_Scale` — Vector-op slot 3 scales the live components in place | 13 | 100% | Yes | FUNCTION |
| 0x004219c0 | `TrackCurve_LinePosition` — Straight centerline, at constant height | 13 | 100% | Yes | FUNCTION |
| 0x00424ae0 | `Coaster_ResetRouteDeadline` — Reset the coaster's route and set its next deadline | 13 | 100% | Yes | FUNCTION |
| 0x0041e100 | `Route_StepSecondary` — Secondary route step temporarily replaces the solver derivative callback | 14 | 100% | Yes | FUNCTION |
| 0x004206d0 | `CoasterModel_LoadLFM` — Load the .lfm file with the caller's mode | 14 | 100% | Yes | FUNCTION |
| 0x0041e420 | `Route_PrependNode` — Insert a new route node immediately after the embedded head sentinel | 15 | 100% | Yes | FUNCTION |
| 0x0041e460 | `Route_RemoveNode` — Unlink and free any route node except the route's own sentinel | 15 | 100% | Yes | FUNCTION |
| 0x00421a10 | `TrackCurve_LineOffsetPlus` — Parallel positive-offset line, at constant height | 15 | 100% | Yes | FUNCTION |
| 0x00421a40 | `TrackCurve_LineOffsetMinus` — Parallel negative-offset line, at constant height | 15 | 100% | Yes | FUNCTION |
| 0x00424b30 | `Coaster_FindState1Car` — Find the first car in state 1; NULL when the sentinel is reached | 16 | 100% | Yes | FUNCTION |
| 0x00424b60 | `Coaster_FindState2Car` — Find the first car in state 2 | 16 | 100% | Yes | FUNCTION |
| 0x00424b90 | `Coaster_FindState4Car` — Find the first car in state 4 (the eviction candidate) | 16 | 100% | Yes | FUNCTION |
| 0x00424850 | `Coaster_GetStationStart` — Return the station geometry, initial parameter and map-derived position | 15 | 100% | Yes | FUNCTION |
| 0x0041dec0 | `Route_InitPhysics` — Install the route's two-component physics system and point it at state | 17 | 100% | Yes | FUNCTION |
| 0x0041f850 | `TrackCursor_AdvanceGeometry` — Advance within a geometry chain, then onto the tail-linked track piece | 17 | 100% | Yes | FUNCTION |
| 0x00421400 | `PhysVec_Zero` — Vector-op slot 6 clears positive-length data and always stores n | 17 | 100% | Yes | FUNCTION |
| 0x0041fd30 | `CoasterShades_InitClamp` — Biased lookup clamps input [-64,127] to [0,63] | 19 | 100% | Yes | FUNCTION |
| 0x00421360 | `PhysVec_MaxAbs` — Vector-op slot 4 is the maximum absolute component (infinity norm) | 19 | 100% | Yes | FUNCTION |
| 0x00421d60 | `TrackCurve_CubicPosition` — Linear horizontal coordinates and a cubic vertical profile | 19 | 100% | Yes | FUNCTION |
| 0x00424c40 | `Coaster_ShouldDispatch` — Start dispatch once any car waited >5000 ticks or >3 cars are waiting | 19 | 100% | Yes | FUNCTION |
| 0x0041cca0 | `JointDir_ToIndex` — Decode a one-hot direction, retaining zero for invalid masks | 20 | 100% | Yes | FUNCTION |
| 0x00421df0 | `TrackCurve_CubicOffsetPlus` — Cubic-height positive-offset rail | 21 | 100% | Yes | FUNCTION |
| 0x00421e40 | `TrackCurve_CubicOffsetMinus` — Cubic-height negative-offset rail | 21 | 100% | Yes | FUNCTION |
| 0x00427020 | `CoasterRider_FromSaveIndex` — Resolve a zero-based save index in castle class 0's rider list | 21 | 100% | Yes | FUNCTION |
| 0x0041cce0 | `TrackClass_GetWorldBounds` — Translate a class's footprint rectangle by the packed map square | 24 | 100% | Yes | FUNCTION |
| 0x004212a0 | `PhysVec_Add` — Vector-op slot 0 adds two vectors; the first vector determines length | 24 | 100% | Yes | FUNCTION |
| 0x00421430 | `PhysVec_AddScaled` — Vector-op slot 7 adds a scaled vector to the destination | 24 | 100% | Yes | FUNCTION |
| 0x004236f0 | `Raster_SetFloatMode` — Set x87 single precision, nearest rounding, all exceptions masked | 24 | 100% | Yes | FUNCTION |
| 0x0041e2b0 | `Route_FindFreeSeat` — Search both seats of every route node, including the embedded head | 25 | 100% | Yes | FUNCTION |
| 0x00421b90 | `TrackCurve_ArcTangent` — Tangent to a planar circular arc, with zero vertical component | 25 | 100% | Yes | FUNCTION |
| 0x00421da0 | `TrackCurve_CubicTangent` — Derivative of the cubic height, retaining the raw horizontal tangent | 25 | 100% | Yes | FUNCTION |
| 0x00423d40 | `Castle_InitStationCorners` — Build the two station corners relative to map square (0,0) | 25 | 100% | Yes | FUNCTION |
| 0x00424990 | `Coaster_StepStationDeparture` — End station departure when its endpoint is reached; otherwise free-step using the station derivative | 25 | 100% | Yes | FUNCTION |
| 0x004212e0 | `PhysVec_Subtract` — Vector-op slot 1 subtracts b from a | 27 | 100% | Yes | FUNCTION |
| 0x0041e6a0 | `RouteNode_InitSeats` — Initialize the node's two seats and make its links a singleton ring | 29 | 100% | Yes | FUNCTION |
| 0x00421b40 | `TrackCurve_ArcPosition` — Planar circular arc: pos cos(t) + dir sin(t) + offset | 29 | 100% | Yes | FUNCTION |
| 0x00429270 | `CoasterShadows_InitTemplates` — Scale three four-point shadow templates; z stays at its static value | 29 | 100% | Yes | FUNCTION |

## Exceptions and integration notes

- **`MatMul` (0x00426120) is 40 instructions / 103 bytes, not 10.** Its
  instruction 9 at 0x0042613a jumps internally to 0x00426140, over the loop
  reload at 0x0042613c. The real return is at 0x00426186. `audit.py` stops at
  that opening jump, reports `orig=10i/28B`, zero prefix mismatches and
  `ESCAPES`. The complete compiled body is 40/40, 103/103, with zero strict
  mismatches and all relocated bytes verified. There is **no diverging
  instruction** in the complete function; instruction 9 is the erroneous
  extent boundary. The committed marker is
  `// WIP-FUNCTION: LEGOLAND 0x00426120  (100% full body, 40i/103B; audit truncates to 10i/28B and reports ESCAPES)`.
  Fixing the shared walker was explicitly out of scope, so this body is not
  counted among the 63.
- **0x004299a3 is instruction 8 of the function starting at 0x00429990**, not
  a function entry. It uses the already-established ESI/ECX/EAX state and
  eventually pops registers saved before this address. No body or marker was
  created. `coaster.c` actually spells its extern comment as
  `/* 0x004299a30 -> 0x00429a30 */`; the malformed nine-digit address explains
  the misleading frontier entry. The intended `Sub_429a30` entry is
  **0x00429a30** (28 instructions / 73 bytes), distinct from 0x00429990.
  Both existing source and the excluded 0x00429990 owner are untouched.
- `Vec3_Normalize` replaces the placeholder `Sub_425d50` by meaning. Each
  school-car row replaces `Sub_<its six-digit address>` with its listed name.
  Existing caller declarations are read-only, so this table is the rename
  map for the integrating session; no duplicate alias bodies were added.

## Mechanics recovered

- **The vector-op table is now fully identified.** A physical pool slot is
  `{ int n; float v[20]; }`, 0x54 bytes. Slots 0..9 at 0x004dcbd0 are listed
  below. `PhysVec_InitOps` also writes dimension at +0x28 (0x004dcbf8) and
  copies all 11 dwords to its destination. Add/subtract/add-scaled use the
  first vector's length and reload it after stores; they do not validate the
  second vector's length. The route system itself points at two components
  beginning with the dimension word at route +0x20.

| Slot | Address | Operation |
| ---: | --- | --- |
| 0 | 0x004212a0 | `PhysVec_Add(a, b, out)`: out = a + b |
| 1 | 0x004212e0 | `PhysVec_Subtract(a, b, out)`: out = a - b |
| 2 | 0x00421320 | `PhysVec_Copy(src, dst)`: copy all 0x54 bytes |
| 3 | 0x00421340 | `PhysVec_Scale(v, k)`: scale live components |
| 4 | 0x00421360 | `PhysVec_MaxAbs(v)`: infinity norm |
| 5 | 0x004213a0 | Existing `PhysVec_ScaleAdd2`: out = ka*a + kb*b |
| 6 | 0x00421400 | `PhysVec_Zero(v, n)`: clear positive-length data and always store n |
| 7 | 0x00421430 | `PhysVec_AddScaled(a, k, out)`: out += k*a |
| 8 | 0x004214f0 | Existing `CarPool_Alloc(n)`: allocate a run of slots |
| 9 | 0x00421510 | Existing `CarPool_Free(a, n)`: rewind the stack pool |

- **Two seats per route node.** Seats are 0x20-byte records at +0x78 and
  +0x98, not route nodes themselves. Their occupant is +0x0c, as the
  0x004273d0/e0 bodies confirm. A car's +0x18 is its seat pointer.
  `Route_FindFreeSeat` visits the embedded head too, returning the first
  free seat. `RouteNode_InitSeats` uses `(table[kind], 0, -4)` and subtracts
  0x004b55a8's spacing for the second seat. Node links are +0xe4/+0xe8.
- **Route steps are dispatched through the owner.** Primary/secondary hooks
  are owner +0x110/+0x114. The secondary wrapper saves and restores route
  +0x34, the physics derivative callback, around the call; it preserves the
  float return in ST(0). Station departure installs 0x004248b0 until the
  station end predicate succeeds, then starts the coaster and stamps owner
  +0xe0 with the game clock.
- **Track geometry:** line position is `pos + t*dir` in x/y, constant z;
  positive/negative offset variants select parallel rails. Cubic variants
  use `((a*t+b)*t+c)*t+d` for z and `(3*a*t+2*b)*t+c` for tangent z. Arc
  position is `pos*cos(t) + dir*sin(t) + offset` in x/y; its tangent has
  `-pos*sin(t) + dir*cos(t)` and zero z. Quarter-turn samples preserve the
  exact original float bit patterns, rather than recomputing PI.
- **Car selection and dispatch:** list state 1 is waiting, state 2 riding,
  and state 4 is the eviction candidate (state 4's narrower meaning is left
  as its observed numeric state). Dispatch becomes true when longest wait
  is greater than 5000 ticks or waiting count is greater than three.
- **Render math:** `SetSpanClip` builds `{top,left,bottom,right}` before
  calling 0x0041f380. `Raster_SetFloatMode` selects single precision, nearest
  rounding and masked x87 exceptions, returning the previous control word.
  The shade lookup saturates a biased [-64,127] input to [0,63]. Shadow
  template initialization writes x/y at 0x00612210/+4, leaving z untouched.

## Original quirks preserved

- FastSqrt/FastRSqrt take and return ST(0), save three registers below ESP
  without reserving stack space, and use `bits >> 23` as the exponent index
  without masking the sign. No input guards were invented.
- The first four shadow templates copy the same scaled source x into both
  destination x and y. The two later groups use independent source x/y.
  This asymmetric source use is present in the original and retained.
- Unknown classes and direction masks return zero, also a valid first
  class/direction. `PhysVec_Copy` copies unused slot components too;
  `PhysVec_Zero` retains a nonpositive n while clearing no components.
- No new gameplay bug was established beyond these observed quirks; there
  are no guessed repairs or fabricated tails.

## Extern types and linkage

- `FastSqrt` and `FastRSqrt` retain the existing `void(void)` declarations,
  but their actual ABI is ST(0)-in/ST(0)-out. Naked definitions reproduce
  the original hand-written assembly. A `NAKED` macro keeps `__declspec(...)`
  from being misread as the function name by the audit marker parser.
- `BuildJoint` uses locally typed `JointParams*` and `PackedSquare`, where
  coaster.c declares `const int*`; its same three-pointer calling ABI and
  16-bit coordinate additions are preserved. `ClearJointHeight` is declared
  with `void*` here, as only pointer forwarding is involved.
- schoolcar.c exposes vector/class hooks as `void(void)` because it only
  stores their addresses. Definitions here use the recovered signatures.
  `g_car_pool_hooks` is a local 44-byte `{hooks[10], dimension}` view, versus
  schoolcar.c's ten-pointer array declaration; `g_cc_obj` is a local
  `{data,length}` view at the same base, versus schoolcar4.c's separate
  pointer and length externs. No caller types were aligned or edited.
- `Route_InitPhysics` still forwards a second argument of 2 to 0x00420410,
  even though the callee does not read that argument. `wsprintfA` is a
  variadic cdecl DLL import; its trailing address is the IAT slot 0x004ab298.

## VC6 levers and evidence

- **An embedded-rectangle pointer advances the scratch-register rotation.**
  `TrackClass_GetWorldBounds`: direct `cls->bounds` accesses give 12 strict
  mismatches at 24i/62B (eax/ecx swap); reversing the sum and naming either
  first scalar are inert. One free volatile class read reduces that to six
  but schedules the load too early. Naming `Rect* bounds = &cls->bounds`
  and using it closes all 24 instructions without volatile. Full-body and
  relocated-byte comparisons pass.
- **Name the second vector element to eliminate an induction register.**
  `PhysVec_Add` as one flat sum emits 27i/57B with an extra EBX push; either
  operand order is identical. `float value = b->v[i]; out->v[i] = a->v[i] +
  value;` gives 24i/53B, strict zero. Naming the first element is inert.
- **Use the outer pointer for the fields that the original addresses from
  the outer object.** `Route_InitPhysics`: assigning every field through a
  named `PhysObj*` swaps ESI/EDI and moves stores (9 mismatches, 17i/51B).
  Keep the named pointer for the init call and setstate store, then spell
  state/back-pointer/derivative as `rt->physics.*`: strict zero, same size.
- **Reuse the dead float parameter for cosine and avoid a named interior
  direction pointer.** `TrackCurve_ArcTangent`: separate sine/cosine locals
  and separate source pointers emit 29i/73B, 24 strict mismatches. `t =
  (float)cos(t)` plus direct field subscripts emits the observed spill to
  the original t slot, FSIN before FCOS, and exactly 25i/70B.
- **Control-word manipulation is an inline-assembly boundary.** Pure C
  merges the two precision/rounding masks and emits 22i/52B. Keeping the
  observed AX mask/store/reload block inside `__asm`, with a byte cast for
  the exception-mask comparison, gives 24i/61B and the original 8-byte EBP
  frame. Only the control-word operations are assembly; the condition and
  return remain C. The old word's integer home and new word's short home
  reproduce -8/-2 exactly.
- **Subscript the identity matrix to get the original induction ordering.**
  A named walked float pointer gives 8 mismatches at 15i/40B (eax/ecx swap
  and swapped pointer/counter increments); `out->m[row*4+col]`, with both
  loops bounded `<=3`, closes all 15 instructions.
- **Three clip stores have a source-order rotation.** `SetSpanClip` written
  left/right/top/bottom gives 6 mismatches at 16i/56B. Writing
  top/left/right/bottom reproduces the original load/store order, strict zero.
- **Full extent and address operands require independent attention.**
  `MatMul` exposes the opening-forward-jump extent error described above.
  Normalized matching also ignores absolute addends: the shadow stores are
  x/y from base 0x00612210, not y/z. Their addresses were corrected before
  promotion and checked by resolving COFF relocations; all 64 full bodies
  then reproduce the original bytes and constant contents.
