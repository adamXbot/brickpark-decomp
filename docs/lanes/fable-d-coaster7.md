# Lane `fable-d` — `LEGOLAND/coaster7.c`

Coaster entrance track, model loader and physics callbacks (SCOPE_FABLE_D
§`coaster7.c`). **10 of 10 exact**, `tools/audit.py LEGOLAND/coaster7.c`
ends `PASS`, `/W3 /O2 /Gy /Gd` compiles clean, no duplicate addresses across
the tree.

## Result table

| address | name | insns | bytes | audit | marker committed |
| --- | --- | --- | --- | --- | --- |
| 0x0041f380 | `Span_SetClip` | 23 | 84 | `[OK]` | `// FUNCTION: LEGOLAND 0x0041f380` |
| 0x0041e4c0 | `Route_SetDeadline` | 23 | 47 | `[OK]` | `// FUNCTION: LEGOLAND 0x0041e4c0` |
| 0x004223c0 | `CountModelRecords` | 27 | 53 | `[OK]` | `// FUNCTION: LEGOLAND 0x004223c0` |
| 0x00426490 | `Mat3_ToMat4` | 30 | 73 | `[OK]` | `// FUNCTION: LEGOLAND 0x00426490` |
| 0x00424bc0 | `Coaster_GetLongestWait` | 30 | 66 | `[OK]` | `// FUNCTION: LEGOLAND 0x00424bc0` |
| 0x00429940 | `TrackRunStepsBack` | 33 | 71 | `[OK]` | `// FUNCTION: LEGOLAND 0x00429940` |
| 0x0041de10 | `RoutePhys_EvaluateDerivative` | 55 | 169 | `[OK]` | `// FUNCTION: LEGOLAND 0x0041de10` |
| 0x004248b0 | `Coaster_StationDerivative` | 57 | 168 | `[OK]` | `// FUNCTION: LEGOLAND 0x004248b0` |
| 0x00420550 | `CoasterModel_LoadFile` | 92 | 235 | `[OK]` | `// FUNCTION: LEGOLAND 0x00420550` |
| 0x00423a10 | `Castle_InitEntranceTrack` | 231 | 811 | `[OK]` | `// FUNCTION: LEGOLAND 0x00423a10` |

All ten end in a real `ret`, none is recursive, all are promotable. Names are
the scope's; nothing was renamed.

## Mechanics recovered

- **The route physics is literal energy conservation.** The solver's state
  vector is `{track parameter, total energy}` and `RoutePhys_EvaluateDerivative`
  returns `{speed, power}`:
  `v = sqrt(2 * (E - PE) / m)`, clamped at zero rather than rooting a
  negative, with `PE` a sum over the route's node list and `m` the train's
  mass. The energy rate is zero unless the lift/launch motor is armed
  (`power > FLT_MIN`) **and** the train is still travelling **less** than 0.05
  units per tick — a launch assist that cuts out once the train is up to
  speed. Only the mass/power getter is a single call; everything else is
  summed per node.
- **`Coaster_StationDerivative` is a constant-deceleration STATION BRAKE laid
  over that derivative.** It runs the ordinary derivative first and then
  overwrites `out->v[1]` with `-m * a * v`, where `a = v*v / (2*d)`, `v` is
  the train's travel this tick and `d` the remaining track distance from the
  train's live position record (`rt+0x0c`) to the station square. That is
  exactly the deceleration that brings the train to rest at the station.
  The distance is scaled by `0.0015875f` (0x004ab404) on the way out of
  0x0041dd00 / 0x0042a1b0.
- **The castle ENTRANCE TRACK is an L of three curve segments plus a piece
  table** (`Castle_InitEntranceTrack`):
  - three 0x58-byte `RouteGeom`/`Curve` records at 0x006102f8, 0x00610350 and
    0x006103a8 (`g_station_geometry`), chained `A <-> B <-> C` through
    `+0x50` (next) and `+0x54` (prev);
  - A and C are straight runs built by 0x00421ab0 with sideways offsets
    `{-10,0,0}` and `{0,-10,0}`; B is the corner, built by 0x00421ce0 from
    three world points and the shape constants 1.5f and 0.5f;
  - a table of 0x24-byte piece templates at 0x0060f928, count at 0x00610a08:
    `{ Pos16 sq; int geom; float t0, t1; FootPart part; }`. `geom` is 0/1/2
    (which segment) and `[t0,t1)` is that square's equal slice of the
    segment's `[0,1]` parameter range — so `step = (t1 - t0) / n` with
    `n = (c2.y - c1.y) >> 1` for the y run and `(c1.x - c2.x) >> 1` for the x
    run, i.e. one piece every TWO map squares.
  - the last 0x14 bytes of every template are a `FootPart`
    (`x0,y0,x1,y1,next`), filled in a final pass as the 1x1 square of that
    piece and threaded into one list, terminated on the last entry.
- **`RouteGeom` and schoolcar8.c's `Curve` are one record.** 0x00421ab0 fills
  `pos = from`, `dir = to - from`, `offset`, `length = |dir|` at +0x40, sets
  the parameter range to `[0.0f, 1.0f]` at +0x44/+0x48, an evaluator pointer
  0x004dd600 at +0x4c, and clears the two links. Size 0x58.
- **The entrance hangs off the castle's own footprint.**
  `Castle_GetFirstCorner` (0x004239b0) and `Castle_GetSecondCorner`
  (0x004239e0) both read the `FootPart` rect at +0x3c of `g_castle_def`
  (0x00829bf8) — four ints, of which only the low words are used — and add the
  caller's square: first = `(x1 + sq.x, y0 + sq.y - 2)`, second =
  `(x0 + sq.x - 2, y1 + sq.y)`. `Castle_InitEntranceTrack` calls both with a
  `{0,0}` square.
- **`CoasterRec` embeds its STATION piece as a whole `TrackNode` at +0x04.**
  That is why schoolcar8.c's `Coaster_GetStationStart` takes the record as a
  `const short*` and reads its map square at short index 4 —
  `TrackNode.sx` sits at `TrackNode+0x04`, i.e. `rec+0x08`.
- **`Span_SetClip` writes a four-plane half-plane clip set**:
  `{ int count; struct { float a, b, c; } p[4]; }`, tested as
  `a*x + b*y >= c`, so the right and bottom edges are stored negated
  (`-x >= -right`). Built from coastermath.c's `SpanRect {top,left,bottom,right}`.
- **Coaster model images are CRLF-terminated text.** `CountModelRecords`
  walks the `{image, byte length}` pair one byte at a time over the CRLF
  predicate 0x004222f0, stepping two at a break; the bound is one byte short
  of the image so the 16-bit CR/LF test cannot read past the end.
- **`CoasterModel_LoadFile` is schoolcar5.c's `LoadWholeFile` wrapped in a
  chdir pair.** It enters `RollerCoaster\RollerCoaster\CreatedData`, does the
  CreateFileA / GetFileSize / allocate / ReadFile / CloseHandle sequence, and
  restores `..\..\..` on **all four** exits — so the restore call is written
  out four times and none of the copies merge.

## Callees named for the first time

| address | name here | evidence |
| --- | --- | --- |
| 0x004222f0 | `ModelImage_IsEOL` | `cmp word ptr [ecx], 0xa0d / sete al` |
| 0x0041dae0 | `Route_SumPotentialEnergy` | sums 0x0041e810 over the route's +0x70 list; consumed as the `E - PE` term |
| 0x0041db90 | `Route_GetMassAndPower` | two float out-params; the first divides `2*(E-PE)`, the second is the energy rate |
| 0x0041dd00 | `Route_TravelPerTick` | `|tangent| * speed * 0.0015875f` |
| 0x0041dd50 | `Route_TravelThisTick` | 0x0041dd00 applied to 0x0041dca0's current speed |
| 0x0041dd70 | `Route_SumMass` | sums 0x0041e7e0 over the same node list; the `m` of `-m*a*v` |
| 0x0042a1b0 | `Track_MeasureDistance` | distance between two position records; result is the brake's `d` |
| 0x00426a90 | `VecMath_Sqrt` | `fld [ebp+8] / call [0x00829a58]`, the module's fast-sqrt hook |
| 0x00420530 | `SetWorkingDirectory` | null-checked `SetCurrentDirectoryA` (schoolcar.c calls it `Sub_420530`) |
| 0x00421ab0 | `Curve_InitLine` | builds a straight segment; see above |
| 0x00421ce0 | `Curve_InitCorner` | the corner counterpart, `(a, b, c, curve, 1.5f, 0.5f)` |
| 0x0060f928 / 0x00610a08 | `g_entrance` / `g_entrance_count` | the piece-template table and its count |
| 0x006102f8 / 0x00610350 | `g_castle_curve_a` / `_b` | the first two entrance segments |

Also confirmed from the caller side: 0x00422300 (just above `ModelImage_IsEOL`)
copies one CRLF-terminated line out of a model image, and 0x0041e810 /
0x0041e7e0 are the per-node potential-energy and mass contributions.

## Original bugs reproduced

- **`Castle_InitEntranceTrack` terminates the footprint list off the LOOP
  variable, not the count.** `g_entrance[k - 1].part.next = 0;` with `k` the
  final-pass index: when the table is empty (`i == 0`) `k` is 0 and the store
  lands four bytes in FRONT of `g_entrance`. Reproduced, commented at the
  site. (For any non-empty table `k == i` and the store is correct.)

## Extern-type divergences (do NOT align)

- `coastermath.c` declares `Span_SetClip(const SpanRect*, void* context)`;
  the definition needs the real four-plane record as the second parameter.
- `schoolcar8.c` declares `CoasterModel_LoadFile(const char* name, int mode)`.
  The second parameter is really an **optional `unsigned int*` byte-length
  out-pointer** — the body ends `if (len) *len = n;`, which is what makes
  `CoasterModel_LoadLTX`'s literal `0` legal. schoolcar8.c's extern is left
  as it is.
- `schoolcar8.c` declares `g_station_geometry` as `unsigned char[]`; it is the
  THIRD castle curve segment and is typed `RouteGeom[]` here. The **array**
  spelling is load-bearing in both files (`push OFFSET`, not a load) —
  recorded lever "two tables pushed as `push OFFSET` are ARRAYS".
- `schoolcar8.c`'s `PhysObj.route` is a `CoasterRoute*` whose +0x20 view it
  calls `RoutePhysics`; they are one object and this file types it as one
  `CoasterRoute` with `here` at +0x0c and `{dimension, energy[2]}` at +0x20.

## Levers, with evidence

- **The `- 1` of an end bound belongs in the LOOP CONDITION, not the end
  pointer's initialiser.** `end = data + len; while (p < end - 1)` gives the
  original's `add eax,esi` + `lea ebx,[eax-1]` (27/27); folded into the
  initialiser (`end = p + len - 1`) VC6 emits one `lea ebx,[eax+esi-1]` and
  the function is 24 of 26. Four other spellings (a second statement, `end--`,
  `end = end - 1`, a named `len` local) are all the folded form.
  (`CountModelRecords`.)
- **Three adjacent constant stores to one array are emitted in SOURCE order,
  not reversed.** `Mat3_ToMat4`'s trailing `m[11] = m[7] = m[3] = 0.0f` was
  swept over all six permutations: `11, 7, 3` is exact, `3, 7, 11` is 28 of
  30 and every other order 29 of 30. The recorded "adjacent address stores
  come out REVERSED" rule is for a PAIR; with three, only the descending
  order matched here — sweep, do not assume.
- **A float value that must survive a call needs its own IR temporary, and a
  block-scoped SECOND local is what supplies it.** In
  `Coaster_StationDerivative` the negated quotient written as a re-assignment
  of the `v*v` local keeps the whole chain on the x87 stack, sinks `v*v` past
  the distance call and flushes the merged `add esp,0x28` **before** the first
  `fmul` — 49 of 57. Writing it as its own `{ float brake = -(vv / (d+d)); }`
  forces `vv`'s memory home and restores the original's
  `call / fmul [vv] / add esp,0x28 / fmul [v]` order: **57/57**. Free
  volatile reads of `v` and `vv` at the use, at the definition, and all six
  operand orders of the final triple product floor at 56. Same family as the
  wave-fourteen caveat "one extra IR temporary advances what a volatile read
  cannot".
- **`x * 2` on a float in x87 is `fadd st(0),st(0)` only when the operand is
  ALREADY on the stack.** `2.0f * (E - pe) / m` gives the original's
  `fsub / fadd st,st / fdiv`; and `d + d` on a named call result gives
  `fmul K / fadd st,st / fdivr`. The integer rule ("`x*2` must be `x+x`")
  carries over unchanged.
- **A `double`-typed threshold survives as `fcomp qword`.** `< 0.05` (no `f`
  suffix) is the original's `fcomp qword ptr [0x004ab408]`, where the two
  float thresholds in the same function are dword-pooled. And `fcomp` +
  `test ah,1` + `je` is `<`, not `>=`: the `je` is taken when C0 is CLEAR,
  i.e. when the comparison is **not** less-than, so the guarded store belongs
  to the `<` arm. (One mismatch in `RoutePhys_EvaluateDerivative` until the
  direction was flipped.)
- **`1.175494351e-38f` is FLT_MIN (0x00800000) and reproduces exactly** — a
  "is this armed at all" guard on a float, not a magic number.
- **An explicit `i = 0;` statement at the TOP of a function is what gives VC6
  a SECOND zero register.** `Castle_InitEntranceTrack` runs on two: `ebx` for
  the `push 0.0f` arguments and the byte stores, and `esi` — which is the
  running table index, still zero — for the two `off.y`/`off.z` float-zero
  stores. Written as `for (i = 0; ...)` the index is not live that early, VC6
  keeps one zero register and the body comes out an instruction SHORT: 170 of
  230 against 187 of 231. Declaring `i = 0;` before the first call and writing
  the loop `for (; i < n; i++)` is the whole difference. Sharpens the recorded
  zero-web entries: the threshold decides *whether* a zero register appears,
  but an already-zero long-lived local is what supplies a *second* one.
- **A struct copy placed between the count and the divide keeps the `fidiv`
  spill out of the copied-to local's frame slot.** `n = ...; sq = c2;
  t = ...; step = (...)/n;` is 201 of 231; with `sq = c2;` written after
  `step`, VC6 reuses `sq`'s (escaped!) slot for the integer spill `fidiv`
  needs, instead of sharing it with `step`, and the frame shifts — 197 of 231.
  The store has to be live across the divide for the slot to be denied.
- **Where a shared register is freed decides which register carries a
  function-wide zero.** The six curve-chain stores written BEFORE `sq = corner;`
  keep `corner` in ebx across them, so VC6 takes a scratch `edx` for the zero
  it needs from there to the end; written after, ebx frees early, VC6 takes
  ebx as the zero and **every** register in the last two loops and the
  epilogue is one position off. 201 of 231 -> **231/231**, with no other
  change. Within the six stores, `prev` before `next` in each pair is also
  load-bearing (227 the other way round).
- **A `short` local keeps its sign extension at the USE; an `int` one moves it
  to the definition and loses an instruction.** `cy2` (`c1.y + 2`, needed
  again 80 instructions and three calls later) as an `int` makes VC6 emit
  `movsx edi,ax` at the definition and drop the original's `movsx ecx,di`
  entirely — 187 of **230** for a 231-instruction function. As a `short` it is
  `mov di,bp` at the definition and `movsx` at the use: 197 of 231. When your
  body is exactly one instruction short and the original sign-extends late,
  narrow the local's type.
- **`>> 1` versus `/ 2` on a signed count** — the original's bare `sar eax,1`
  is the shift; `/2` would add `cdq/sub`. Third instance in the tree.
- **A dword load feeding only 16-bit stores is normal VC6 output, not a union
  in the source.** `mov eax,[esp+0x30] / mov word [..],ax / inc eax /
  mov word [..],ax` is what plain `Pos16` field arithmetic compiles to (the
  dword form is a byte shorter than `mov ax,[..]`); no packed-union spelling
  is needed. Worth recording because the recorded `{u8,u8}` dword-plus-mask
  idiom reads like it would be.
- **A four-times-repeated cleanup call does not merge.** `CoasterModel_LoadFile`
  has the chdir restore in all four exits and VC6 cross-jumps none of them,
  because each arm's tail differs (the `xor eax,eax` placement and, on the
  success path, the out-param store). Written out four times: 92/92 first try.
- **The push-sinking rule at its cleanest.** `Coaster_GetLongestWait`'s
  `push ebx` sinks below the empty-list test because the whole walk lives in
  the guarded block; and `TrackRunStepsBack`'s two exits (`*endOut = node;
  return steps;`) are written once and duplicated by VC6, with different
  schedules in each copy — no source construct required. 30/30 and 33/33
  first try.

## Tooling

- No instance of the recorded extent-walker defect (the rotated-loop entry
  `jmp`) in this file — the two candidates, `Castle_InitEntranceTrack`'s three
  loops and `TrackRunStepsBack`'s rotated walk, all bound correctly and audit
  reports the exact original extents.
- A side-by-side lister was kept at
  `scratchpad/fd_coaster7/sbs.py <file> <func> <addr>`; it prints
  original-vs-ours index for index, which is what made the one-instruction
  deficits above readable.
