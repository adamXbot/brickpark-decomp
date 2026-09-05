# Lane `fable-a / goldrush2.c` — gold rush people

File: `LEGOLAND/goldrush2.c` (new).  Both assigned functions closed **exact**.
Gate: `python3 tools/audit.py LEGOLAND/goldrush2.c` → `PASS`, and `/W3 /O2 /Gy
/Gd` compiles with no warnings.

| address | name | insns | bytes | audit | mismatch | marker committed |
| --- | --- | --- | --- | --- | --- | --- |
| 0x004025d0 | `SetPerson3DHeading` | 122 | 335 | `[OK]` | 0 | `// FUNCTION: LEGOLAND 0x004025d0` |
| 0x004064d0 | `Fort_StepVisitor` | 126 | 393 | `[OK]` | 0 | `// FUNCTION: LEGOLAND 0x004064d0` |

Neither was renamed (both names came from `goldrush.c`'s externs).  Both end
in a real `ret`, neither is recursive, so both are promotable and are
committed as `// FUNCTION:`.

---

## Mechanics recovered

### `SetPerson3DHeading` (0x004025d0) — the 16-point heading

`SetPersonDirection` (0x004400b0, `math3d.c`, already exact) is the same
function at half the angular resolution, and reading it first is what made
this a one-draft match.  Both write the person's rotation vector at
`Person3D +0x40..+0x48` (three floats, x and z always 0) and hand `&p->rot`
straight to `SetPersonRotation`, which builds the 16.16 matrix at `+0x58`.

- The heading runs **0..15** and is a *screen-space* heading, not a maths
  angle.  As multiples of pi/8 the table is
  `-2, -3, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, -1` for cases 0..15 —
  a plain descending sweep from case 2 down to case 15, with cases 0 and 1
  sitting one full turn below.  That is exactly `SetPersonDirection`'s
  convention at double the resolution (its case 0 is likewise `-pi/4`, its
  case 1 `3*pi/2`), so the two share one heading space: 8-point heading `h`
  is 16-point heading `2h`.
- The jump table at `0x00402720` is the **identity** (case *i* → block *i*),
  so the source's case order is the natural 0..15.
- Its one live caller is `SchoolCar_Draw` (`goldrush.c:2150`), which feeds it
  `c->bloke->dir = (c->b8 + 6) & 0xf` — i.e. the driving-school car's driver.
- There is **no `default:`**: an out-of-range heading leaves `rot.y` alone and
  still rebuilds the matrix.  VC6 duplicates the whole `SetPersonRotation`
  tail into all sixteen case blocks and lets case 15's store fall into the
  copy that the default path also enters — the same shape `math3d.c` already
  records for `SetPersonDirection`'s case 7.

### `Fort_StepVisitor` (0x004064d0) — in-fort visitor behaviour

Reached from `Fort_TickRiders` (0x00406660) at the rider's `action == 1`.
`Bloke +0x40` is a sub-state private to the fort; `+0x42` is the sub-state to
resume with when the `+0x58` timer expires; `+0x58` is that timer.

- **sub-state 1** — standing still: `if (--b->wander <= 0) b->f40 = b->f42;`
- **sub-state 2** — walk to a random point inside the fort's interior
  rectangle, then go to sub-state 3.
- **sub-state 3** — decide, on `rand() & 3`: 0 or 1 → face a random one of the
  sixteen headings and stand still for `(rand() & 15) + 3` ticks (resuming at
  3); 2 → go back to sub-state 2 and pick a new spot; 3 → advance the rider's
  own `action`, so `Fort_TickRiders` moves it on to the next step of the ride.
  This confirms and sharpens the guess already written into `goldrush.c`'s
  header comment ("1-in-2 chance of a random facing and a 3..18-tick timer,
  1-in-4 of stopping, 1-in-4 of advancing the rider's action").

- **New global named: `g_fort_area` at 0x004b4580** — the fort's interior as
  four initialised ints `{x0, y0, x1, y1} = {-1, -3, 3, 3}`, in tiles
  relative to the placement's own square.  `goldrush.c`'s header calls it
  "the .rdata table at 0x004b4580"; it is in `.data`, and it is a rectangle,
  not a list of fixed points.  Declared as ONE struct because the four are
  read as one object.
- **The random point** is
  `((origin + tile) << 8) + ((span1 - span0) << 8) * ((rand() & 255) / 255.0f)`
  per axis, in 24.8 fixed point, converted with the game's own `fistp` helper
  (0x00458930; our `(int)` casts emit `call __ftol`, which the comparison
  treats as the same call).  The 1/255 is the **single-precision** constant at
  `0x004ab380` (0x3b808081), so the source multiplies by `1.0f/255.0f`; a
  `/ 255.0f` would have emitted `fdiv`.

---

## Levers, with evidence

- **A parameter's root copy is forced by hoisting a derived local above the
  switch, and that also decides which dead parameter home each spill takes.**
  In `Fort_StepVisitor` the `rd` parameter is used only inside sub-state 2,
  and declaring `MapSquare* key = (MapSquare*)&rd->ride_id;` inside that case
  sinks the load to index 44.  The original loads it at **index 4**, right
  after `push edi`.  Moving the single declaration to the top of the function,
  above the `switch`, produces the eager `mov edi,[esp+14h]` — *and* fixes
  four further mismatch pairs, because with both dead parameter homes free
  from index 5 VC6 puts the pre-shifted x span in arg2's slot `[esp+18h]` and
  the `fild` scratch in arg1's `[esp+14h]`; with the lazy load they come out
  swapped.  **One declaration placement, 12 of 126 mismatches.**  Note the
  `+0x0c` is folded into the uses (`[edi+0ch]`, `[edi+0dh]`) — the hoist costs
  no `lea`, so it is free.
- **Constant stores must be written AFTER the statement whose call they are to
  interleave with.**  In sub-state 3 the original emits
  `call rand / and eax,0fh / mov word [esi+40h],1 / add eax,3 /
  mov word [esi+42h],3 / mov [esi+58h],eax` — the two constant stores sit
  *inside* the timer's arithmetic.  Written before the timer statement they
  cannot sink past its `rand()` call (stores do not migrate across a call) and
  land adjacent before it, costing 6.  Written after it, VC6's adjacent-store
  reordering hoists them into the gaps and the interleave is exact.  This is
  the store/call-barrier rule used as a *placement* instrument rather than a
  prohibition.
- **VC6 propagates a compared constant into the arm and stores the REGISTER.**
  The original's `mov word ptr [esi+40h], di` in the `r == 2` arm is produced
  by a plain `b->f40 = 2;` — VC6 knows `edi == 2` on that edge and reuses the
  register instead of an immediate.  Do not reverse-engineer such a store into
  `b->f40 = (short)r;`; the constant spelling is what the original has.
- **Three independent `if`s, not an `else if` chain.**  After the `r == 0 ||
  r == 1` block the original falls straight into `cmp edi,2` with no `jmp`
  over it, so the arms are separate statements.  The `r == 2` arm ends in
  `break` (its own inline epilogue at 0x00406530); the `r == 3` arm has
  another; VC6 tail-duplicates the four-instruction epilogue into four blocks
  rather than jumping.
- **A three-case `switch` on a `short` lowers to a `movsx` + `dec`/`je`
  chain**, not to a jump table, and its block layout comes out reversed
  (case 3 inline, then 2, then 1) with the cases written in the natural
  1, 2, 3 order.  No case reordering was needed.
- **Float angle constants are written as exact single-precision literals.**
  `%.9g` of the stored bit pattern round-trips: 0x40490fd7 → `3.14159179f`
  (four ulps below `(float)3.1415926`).  Writing `(float)(M_PI)` or a
  `k*PI/8` fold would not reproduce them.  Same rule `math3d.c` already
  records.

---

## Extern prototype types

- `void SetPerson3DHeading(Person3D* p, unsigned int dir)` — this file defines
  the second parameter as **`unsigned int`** (matching its twin
  `SetPersonDirection`), where `goldrush.c` declares the extern as `int dir`.
  Both spellings give the same `cmp ecx,0Fh / ja` range check and the same
  caller-side push, so this is a harmless divergence.  **`goldrush.c` was not
  touched**; per the project rule the extern's type stays the caller's lever.
- `SetPersonRotation(Person3D*, Vec3*)`, `CalcMoveLine(Pos8, Pos8, void*)`,
  `NewDirForAction(Bloke*, unsigned char)` and `rand(void)` are declared
  exactly as `anim2.c` / `goldrush.c` already declare them; no divergence.

## Original bugs

None found in either function.  Neither has an unexplained behaviour: every
instruction is accounted for.
