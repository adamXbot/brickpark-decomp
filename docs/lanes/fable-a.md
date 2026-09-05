# fable-a — parallel session notes (2026-09-05)

Findings from the four files docs/SCOPE_FABLE_A.md assigned to this session,
for folding into docs/DECOMP.md at integration. One section per lane; the
per-lane files beside this one are the originals and carry the evidence.

| file | exact | WIP | notes |
| --- | --- | --- | --- |
| `LEGOLAND/goldrush2.c` | 2 | 0 | `fable-a-goldrush2.md` |
| `LEGOLAND/jcroute.c` | 1 | 1 | `fable-a-jcroute.md` |
| `LEGOLAND/bswater.c` | 3 | 1 | `fable-a-bswater.md` |
| `LEGOLAND/objrect.c` | 6 | 0 | `fable-a-objrect.md` |

---

# goldrush2.c

## Lane `fable-a / goldrush2.c` — gold rush people

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

---

# jcroute.c

## Lane notes — `fable-a` / `LEGOLAND/jcroute.c` (jungle cruise routing)

Session of 2026-09-05. Two functions, both from `junglecruise.c`'s
unmatched-callee frontier.

| address | name | insns | audit | marker |
| --- | --- | --- | --- | --- |
| 0x00437440 | `JungleCruise_StepRoute` | 108 | **[OK] 108i/291B, mismatch 0** | `// FUNCTION: LEGOLAND 0x00437440` |
| 0x00437260 | `JungleCruise_TraceRoute` | 130 | [WIP] 130i/**339/339B**, mismatch 105 | `// WIP-FUNCTION: LEGOLAND 0x00437260  (…)` |

`audit.py LEGOLAND/jcroute.c` ends PASS; `/W3` is clean. No existing file was
touched.

---

## Recovered mechanics (spec material for a browser runtime)

**The river runs TWO independent walks over the same graph, using DISJOINT
fields of `JcWater`.** That is why both can be live at once and why
`junglecruise.c` could only guess at three of the offsets from the outside:

| field | trace walk (0x00437260) | flood walk (0x00437440) |
| --- | --- | --- |
| +0x04 `links` | **honoured** (1 N, 2 E, 4 S, 8 W) | **ignored** |
| +0x08 | — | ply depth, an **int** (`w->dist + 1`) |
| +0x0c `mark` | visited flag, set on the way down, never cleared | — |
| +0x14 | — | **pointer**: next square of the frontier |
| +0x18 | — | **pointer**: predecessor square |

So `junglecruise.c`'s `rlink` (+0x08, typed `JcWater*`) and `f14` (+0x14,
typed `int`) are typed the other way round. No conflict and nothing to change
there: `JungleCruise_RebuildRoute` only stores 0 into both, so its codegen does
not depend on it, and it is exact as written.

**The two walks disagree about connectivity, and it is in the shipped game.**
`JungleCruise_TraceRoute` steps only through a set `links` bit;
`JungleCruise_StepRoute` never looks at `links` at all — it calls
`JcWater_FindAt` on the four coordinates and accepts any square that exists
five cells away and carries the same owner. Two river arms that run alongside
each other without being linked are therefore *connected* for the breadth-first
flood and *not* connected for the route tracer. Reproduced, commented at the
site, not fixed.

**Original quirk in the flood, reproduced:** `JungleCruise_RebuildRoute`
clears +0x18 on every square the station owns and then seeds the frontier with
the route-END square — but it never stamps the seed's own +0x18.
`StepRoute` only stamps a square it *reaches*, so on the second ply the start
square still looks unvisited to its own neighbours, is re-enqueued with depth 2
and a predecessor pointing back at one of them, and its +0x14 is overwritten
while it is not on any list (harmless only because ply one has already read
it).

**`TraceRoute`'s `route` argument is a success FLAG, not a route.** Every level
tests `*route == 1` on entry and the level that reaches the target stores 1, so
the whole recursion unwinds without doing more work. `JungleCruise_BuildRoute`
holds it in a `void*` and returns it, which is why the ride's "route" is really
just 0/1 — the actual path is the +0x18 predecessor chain the flood leaves
behind.

---

## Levers, with evidence

- **`JungleCruise_StepRoute`'s station id parameter is `unsigned short`, not
  `int` — and that single type is the whole function.** The original loads it
  once per frontier square with `mov cx, word ptr [esp+0x34]` and keeps it in
  `cx` across all four neighbour tests. Written `int id` and compared as
  `p->owner.w == (unsigned short)id`, VC6 loads the full dword and the function
  is 107/108 with that as the **only** mismatch; changing the parameter to
  `unsigned short` closes it at 108/108. `junglecruise.c` declares the extern
  `int` and *its* caller (`JungleCruise_RebuildRoute`) is exact that way — both
  files are right, the extern prototype is a caller-side lever. Same class of
  divergence recorded for `TraceRoute`'s `route`: `int*` here (it is compared
  against 1 and stores 1), `void**` in `junglecruise.c`.

- **The four-neighbour probe block writes itself.** `if (n && n->owner.w == id
  && n->prev == 0) { … }` four times over four locals is byte-exact first try:
  the short-circuit chain's three tests all jump to the same skip label and the
  body is the fall-through, so no exile question arises. Declaring the four
  neighbour pointers INSIDE the `while (w)` body is what sinks `push ebx/ebp/edi`
  past the null guard (the recorded deferred-prologue lever, confirmed again).
  Storing the four `JcWater_FindAt` results in locals is also what merges the
  four `add esp,8` cleanups into one `add esp,0x20`.

- **NEW, and it explains a whole class of allocation residuals: VC6's
  callee-saved ranking INVERTS when a loop exists at IR time.** Measured with a
  four-line probe, not inferred:
  - *No loop.* A function with four `int` parameters, two dereferenced pointer
    parameters and one pointer local assigned from a call gives the four
    callee-saved registers to the four **ints** and homes all three **pointers**
    in memory, reloading them at every use — *even though the pointers have more
    references* (local 7, `route` 6, ints 5 each). This is exactly the
    original `TraceRoute`'s allocation (esi=x, edi=y, ebp=tx, ebx=ty, with `w`
    living in the dead arg3 slot `[esp+0x1c]`).
  - *Same probe plus a self-tail-call.* VC6 turns the tail call into a loop at
    IR level and the ranking flips: the dereferenced pointers take the
    registers, the ints spill, **and** a secondary induction variable for the
    `x + 5` neighbour appears in a stack slot — three instructions the original
    does not have.

  So the original's tail-call-to-loop conversion happened AFTER register
  allocation and ours happens before it. That is a compiler phase-ordering
  difference, not a source shape, and it is the entire residual on
  `TraceRoute`. **Before spending a wave on an allocation residual in a
  self-recursive function, run this two-line probe: it says in one compile
  whether the target regime is reachable at all.**

- **Corollary lever, cheap and useful elsewhere: ANY dead trailing statement
  blocks the tail-call-to-loop conversion.** `w = w;`, `p = p;`, `x = x;` and
  `if (route) ;` after the last call all produce the same object — the
  statement itself is folded away, but VC6 has already decided the call is not
  in tail position, so it emits a real `call`/epilogue and reverts to the
  loop-free allocation (which restored the original's esi/edi/ebp assignment
  here). This *extends* the recorded "no zero-code statement keeps an else arm
  alive" rule: such statements are folded for LAYOUT but survive long enough to
  change TAIL-CALL detection. Not usable on this function (the original wants
  the loop) but it is a free way to move a function between the two regimes.

- **A step assigned through a temporary is not a basic induction variable.**
  `x -= 5;` or `x - 5` spelled twice in the west arm makes `x` a basic
  induction variable of the tail-call loop, and VC6 then keeps `x + 5` in a
  stack slot across the whole loop, updating it with a second `sub` at the
  latch (and deriving `x - 5` from it with `add edx,-0xa`). Writing
  `int nx = x - 5;` and using `nx` for both the probe and the recursive call
  removes the induction variable completely and is worth 9 instructions of
  noise. Related to the recorded "let VC6 do the strength reduction" entry, but
  the opposite direction: here the original does NONE, and the temporary is how
  you stop it.

- **Splitting a parameter's references across a local copy does NOT split its
  allocation web.** `BPosW* o = owner; int* r = route;` used for the recursive
  calls while the parameters are used for the derefs is byte-identical in
  ranking terms — VC6 coalesces the copy into the parameter's web and the
  combined count is what ranks. Retires a hypothesis class ("demote a
  parameter by giving half its uses another name").

- **`register` and `static` are inert here.** `register int tx, register int
  ty` on the parameters changes nothing; making the function `static` (with a
  kicker to keep the symbol) changes nothing.

- **Tie-break among equally-referenced parameters: the LATER parameter wins.**
  Two probes (ints at 3/4 vs pointers at 5/6, and the reverse) both give the
  registers to parameters 5 and 6. Worth knowing before trying to win a tie by
  reordering source statements — you cannot; only a strictly higher count or a
  regime change moves it.

- **Free `volatile` reads bought the exact BYTE LENGTH, not the match.** All
  variants are 130/130 instructions; the audit strict mismatch / byte length
  against the original's 339 bytes:

  | body | mismatch | bytes |
  | --- | --- | --- |
  | plain tail-recursive C, no shims | 116 | 352 |
  | plain `while (*route != 1)` loop | 113 | 345 |
  | + free volatile read of `w` | 111 | 333 |
  | **+ free volatile read of `w` and `owner`** | **105** | **339 (exact)** |
  | + free volatile read of `w` and `route` | 112 | 346 |
  | loop form + volatile `w` and `route` | 101 | 340 |

  Both committed shims are *free* reads in the recorded sense — the original
  loads from those two frame homes at every one of those sites — and the
  byte-exact row is the one committed, because it says every encoding is the
  right size and the residual is purely allocation and ordering. The
  lowest-mismatch row (101) is not byte-exact and was not taken.

## Callees and globals

Nothing newly named: `JcWater_FindAt` (0x004371b0), `g_jc_route_cur`
(0x0062fd30) and `g_jc_route_next` (0x0062fd34) were already named in
`junglecruise.c`, and this lane confirms all three from the other side.

---

# bswater.c

## Lane `fable-a-bswater` — boating-school water and launch (2026-09-05)

File: `LEGOLAND/bswater.c` (new). Four functions from the unmatched-callee
frontier. `tools/audit.py LEGOLAND/bswater.c` ends **PASS**; `/W3` clean.

| address | name | insns | bytes | audit | mismatch | marker |
| --- | --- | --- | --- | --- | --- | --- |
| 0x00418e60 | `BoatingSchool_TryLaunch` | 95/95 | 299/299 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x00418e60` |
| 0x0042c6d0 | `Carousel_TickInstance` | 106/106 | 296/296 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x0042c6d0` |
| 0x0041cb20 | `BsWater_WalkStep` | 108/108 | 291/291 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x0041cb20` |
| 0x0041c4c0 | `BsWater_SetTile` | 109/109 | 350/350 | [WIP] | 3 | `// WIP-FUNCTION: LEGOLAND 0x0041c4c0  (109/109 insns, 350/350 B, mismatch=3, idx 93: \`inc ebp\` scheduled 2nd not 4th in the inner latch)` |

No renames. All four end in a real `ret` and none is recursive, so the three
`[OK]` bodies are promotable as committed; the fourth stays WIP on mismatch,
not on extent.

---

## Data structures and rules recovered

### The boating-school boat record (`BsBoat`, 0x3f4 bytes, head 0x004cc03c)

`anim2.c` had the layout from the DRAWING side; `BoatingSchool_TryLaunch` is
the side that BUILDS one, so the initial values are now known:

```
+0x00 u16  key      the school's own map square (the station's +0x00)
+0x04 int  cx       the map square the boat is on   = key.x - 1
+0x08 int  cy                                       = key.y + 5
+0x0c int  nx       the square it is heading for     = key.x - 1
+0x10 int  ny                                       = key.y + 5
+0x1c      wob[80]  {int x, int y} rocking offsets  -- memset to the BYTE 0xf1
+0x29c     frame[80] sprite codes                   -- memset to 0
+0x3dc int          = 1
+0x3e0 int          = rand() & 3, then 3 rewritten to 2   (see the bug below)
+0x3e4 int state    = 1  (state 1 = "just launched", tested by the guard below)
+0x3e8 int leg      = (rand() & 0xf) + 4
+0x3ec Bloke* rider = the visitor
+0x3f0      next
```

The launch guard is three refusals, all against the station's NEAR jetty
(`st->ax`/`st->ay`, the route start): a boat of this school sitting on the
jetty square, a boat heading for it, or any boat of this school still in
state 1. `+0x14/+0x18` (the boat's screen position) are left as heap garbage
until the drawer computes them.

### `BsWater +0x04` is the ARM MASK (new)

`ridecb5.c`/`ridecb6.c` both had `+0x04` as padding in `BsWater`.
`BsWater_SetTile` writes the four-bit arm mask there as an `int`
(`mov dword ptr [eax+4], ecx`) — the same mask `BsWater_Probe` (joust2.c
0x0041c690) returns. So the lake caches its own shape and the drawer does not
have to re-probe. Full record now:

```
+0x00 u16  pos     this cell's own map square
+0x02 u16  owner   the school that owns it
+0x04 int  mask    the N/E/S/W arm mask (bit 1/2/4/8)   <-- NEW
+0x08 int  dist    steps from the route seed
+0x10      next    the lake-wide list link (head 0x004d823c)
+0x14      queue   the route walk's frontier link
+0x18      prev    reached-from pointer AND visited mark
```

### The route walk (`BsWater_WalkStep`)

A breadth-first walk on the lake's five-cell lattice, state in two globals:
`g_bs_walk_cur` (0x004d8240) is the frontier built by the previous ply,
chained through `+0x14`; `g_bs_walk_next` (0x004d8244) is the one this call
builds. A neighbour five squares away is taken iff it exists, its `owner`
equals the walk's key, and its `+0x18` is still 0. Taking it sets
`+0x18 = current` (so the mark and the back-pointer are the SAME field, which
is why `BoatingSchool_RebuildRoute` can clear the whole lake with one pass and
then read the route back by following it) and `+0x08 = current->dist + 1`.
The frontier is pushed at the head, so it is a stack, not a queue: within one
ply the cells come out in reverse order of discovery. Neighbour order is
N (y-5), E (x+5), S (y+5), W (x-5) — the same order and the same +/-5 lattice
as `BsWater_Probe`, but with NO bounds check, because `BsWater_FindAt` is the
plain list search.

### The carousel machine (`Carousel_TickInstance`)

Three states in the record's flag word `+0x0c`:

* **idle** (neither bit) — once anybody has boarded (`+0x06 != 0`), the
  record's timer `+0x1c` counts down one per tick; at zero the ride takes the
  "not letting anyone on" flag on its own map square and latches 0x4000.
* **boarding** (0x4000) — turns as soon as `boarded` (+0x06) equals
  `visitors` (+0x18); `Carousel_StartRide` (0x0042bc90) does the transition.
* **running** (0x0001) — one platform frame every SECOND tick (`+0x14` is the
  divide-by-two counter), the frame wraps at 0x40 and each wrap spends one of
  the revolutions in `+0x10`. At zero revolutions the handler waits for
  `GetAllBlokesOffRide` and then calls `Carousel_StopRide` (0x0042c210).

Field types corrected against `ridecb3.c`: `+0x10` is a BYTE (revolutions
left), not an int — `mov al, byte ptr [esi+10h]` reuses the low half of the
register that still holds the incremented `+0x14`.

Then, unconditionally except on the "waiting for riders to get off" early
return, the CLASS-wide rider list (ObjDef +0xcc) is walked — every carousel
walks every carousel's riders each tick and picks out its own by map square —
and any rider whose pose set (+0x35) is 1 is re-placed by the .bnv solve at
the platform's current frame. Finally the z-sprite's frame word is set from
the same frame byte (`**g_carousel_zspr->lls_holder = rec->frame`).

`Carousel_StopRide` (0x0042c210, named here for the first time) re-rolls the
revolution count to `rand() % 2 + 3`, clears frame/visitors/boarded, clears
flag bits 1 and 0x4000 together (`and ebx,0FFFFBFFEh`) and fades the ride's
sample out. That is what fixes `+0x10`'s meaning.

### `BsWater_SetTile`

Repaints the 5x5 block of MAP CELLS centred on one lake square — the lattice
is five cells apart precisely because each lake square owns a 5x5 patch — and
creates the lake record if it does not exist. Per cell it sets
`flags = 8`, `rf = 2`, `obj = g_bs_water_cls->c4`, `key = the lake square`,
and calls `SetMapTile` with the tile from the shape table.

---

## Callees named for the first time

* `Carousel_StopRide` — **0x0042c210**. See above. `ridecb3.c` calls it
  through the tick and did not have a name for it.
* `g_bs_water_shapes` — **0x004b53d4**, `unsigned char [16][25]`: 25 tile
  codes (the 5x5 block, row-major) per arm mask.
* `g_bs_water_tsm` — **0x0082adf4** re-typed as `TsmRec*` (roads.c's
  `{void* elem; unsigned short* loaded;}`); `ridecb5.c`/`ridecb6.c` call the
  same address `g_bs_water_tiles` with a `BsTileSet` shape. Same object, two
  spellings; nothing changed in either file.
* `g_carousel_bnv` — **0x0061608c**, the carousel's .bnv bundle (the one
  `SetBlokePositionFromBNV` is handed). `ridecb3.c` names 0x00616090 and
  0x00616098 as CarouselOn/CarouselOff; this is a third, four bytes below.
* The two carousel .bnv depth constants are **-1617853.25f** and
  **-1618109.0f** (0xc9c57dea / 0xc9c585e8) — a new pair; joust uses
  -1617735.0/-1617993.5 and balloonz -1617692.375/-1617904.25.

## Original bugs reproduced (commented at the site)

1. **`BoatingSchool_TryLaunch` dereferences an unfound station.** The station
   search's result is never null-checked; `st->ax` is read inside the boat
   loop. It cannot fire in practice because a boat's key always comes from a
   live station, but the guard is genuinely absent.
2. **`BoatingSchool_TryLaunch` launches at a fixed offset, not at the jetty.**
   The three refusal tests compare against `st->ax`/`st->ay`, then the new
   boat is placed at `(key.x - 1, key.y + 5)` — a hard-coded offset from the
   BUILDING's square. The two agree for the class's standard footprint, so it
   is latent.
3. **`rand() & 3` biased to 2.** `+0x3e0` is rolled 0..3 and then, as a
   separate statement AFTER the two buffer fills, 3 is rewritten to 2. The
   four-way choice is really three-way with 2 twice as likely.
4. **`BsWater_SetTile` does not null-check the map cell.** The standard
   bounds-checked fetch is compiled in full (x >= 0, x < width, y >= 0,
   y < height) and its zero result is stored into two instructions later, so a
   lake square within two cells of the map border faults.
5. **`BsWater_SetTile`'s `t >> 8` is always zero.** The tile lookup
   `*g_bs_water_tsm[t >> 8].loaded + (t & 0xff)` is copied verbatim from the
   road layer (roads.c 0x00412a20) where `t` is an `unsigned short` tile word.
   Here the shape table holds BYTES, so only TSM record 0 is ever consulted.
   VC6 still emits the shift because it does not range-narrow a promoted byte.
6. **A newly created lake record is only partly initialised**: `+0x18` is
   cleared and `+0x10` linked, but `+0x08` and `+0x14` are left as heap
   garbage; only a cell the route walk reaches ever gets them written.

---

## Levers, with evidence

* **A packed `{u8 x; u8 y}` parameter passed by value is read back out of the
  argument slot with an UNALIGNED dword load plus a mask.** In
  `BoatingSchool_TryLaunch` the whole slot is already in `edi` (it is compared
  16 bits at a time as `di`), so `key.b.x` is `and edi,0FFh` — but `key.b.y`
  is `mov ecx, dword ptr [esp+15h] / and ecx,0FFh`, a dword read one byte into
  the slot. Do not try to reproduce that with a shift or a byte load: writing
  the union member (`key.b.y`) is what emits it. Confirms and sharpens the
  recorded "packed {u8,u8} by value is forwarded as a dword and masked".
* **The order of `p->next = head;` and a neighbouring field store decides
  where the head LOAD lands, not where the store lands.** In
  `BoatingSchool_TryLaunch`, `bt->key.w = key.w; bt->next = g_bs_boats;` puts
  `mov eax,[g_bs_boats]` two instructions late (the only mismatch in the whole
  body); the reversed source order is exact. The two STORES stay in the
  compiler's own order either way — it is the load that moves. Same lever, and
  the same fix, at the allocation in `BsWater_SetTile`
  (`w->next = g_bs_water; w->prev = 0;` — the reversed order costs 2).
* **`memset(p, 0xf1, n)` is the readable spelling of `mov eax,0F1F1F1F1h /
  rep stosd`.** With `/O2`'s intrinsics on (declare `memset` and
  `#pragma intrinsic(memset)`), a non-zero byte constant broadcasts to a dword
  and the fill needs no remainder when the size is a multiple of four. First
  use of a non-zero memset constant in this tree.
* **An extern prototype's `unsigned short` parameter is what leaves the
  caller's upper half dirty.** `Carousel_TickInstance` passes the record's map
  square to `GetAllBlokesOffRide` as `mov cx, word ptr [esi+4]` INTO the
  register that still holds the just-incremented tick counter, and pushes the
  whole dirty `ecx`. An `int` parameter zero-extends and costs the match.
  `rides.c` already declares 0x0048a390 that way; this is the caller-side
  confirmation.
* **A dword flag field read once and bit-twiddled in `ah` is just direct field
  access.** `rec->flags & 1`, `rec->flags & 0x4000`, `rec->flags &= ~0x4000`
  and `rec->flags |= 0x4000` on an `int` field give one `mov eax,[esi+0Ch]`,
  `test al,1`, `test ah,40h`, `and ah,0BFh` / `or ah,40h` and a dword
  write-back — no local, no cast, no union. This refines the recorded
  "u16 flags `|=` narrows to a byte OR, `&= ~` does not": on a DWORD field
  both narrow, because the value is already live in a register.
* **Four consecutive `__cdecl` calls whose results all land in locals share
  ONE `add esp`.** `BsWater_WalkStep`'s four `BsWater_FindAt` calls are
  cleaned up by a single `add esp,20h` after the fourth. This is the exact
  converse of the recorded "a call result passed straight into another call
  splits the add esp" — assigning each result to its own local is what merges
  them, and the merge then shifts every `[esp+N]` argument read in between
  (the `key` parameter is read at `[esp+34h]`, before the adjustment).
* **Four identical neighbour blocks written out in full, not looped.** In
  `BsWater_WalkStep` each of the four results gets its own callee-saved
  register (edi/ebx/ebp/eax) and its own copy of the six-instruction body. An
  array of four pointers or a loop over them reproduces neither the register
  assignment nor the block layout — and the "an ARRAY of pointers reserves all
  its frame homes" rule says why it must not be an array.
* **A split prologue on a leading null test.** Both `BsWater_WalkStep`
  (`push esi` before the test, `push ebx/ebp/edi` after) and `BsWater_SetTile`
  (`push ebx/esi` before, `push ebp/edi` after the allocation check) do it,
  and in `BsWater_SetTile` the allocation-failure exit therefore has its OWN
  two-pop epilogue while the normal exit pops four. Writing the guard as an
  early `return` inside the `if` is enough; no construct needed.
* **Two argument slots reused as locals is what keeps an 8-byte frame.**
  `BsWater_SetTile` has a 2-byte key and one compiler temp in `sub esp,8`,
  and puts the outer row counter in `mask`'s slot and `x - 2` in `owner`'s —
  both written after the argument's last read. Nothing in the source asks for
  this; it falls out of `for (r = 0; ...)` and the `x - 2 + c` induction
  variable once the arguments are dead, which is a reason NOT to copy
  arguments into named locals.
* **VC6 collapses a `[mask][r*5+c]` subscript into ONE induction variable
  across BOTH loops.** In `BsWater_SetTile` `ebp` is set to
  `shapes + mask*25` in the outer preheader and only ever `inc`-ed in the
  inner latch — it is never re-seeded per row, so the outer loop's stride is
  carried implicitly by the five inner increments. Ten spellings of the
  subscript (flat, 2-D, 3-D, hoisted row pointer, index local, explicit
  running counter) all produce exactly this.
* **NEGATIVE RESULT — induction-variable emission order in the loop latch is
  not reachable by source ordering.** `BsWater_SetTile`'s only residual is
  that `inc ebp` is emitted second in our inner latch and fourth in the
  original's. The obvious hypothesis (the IV list is built in first-use order,
  so put the shape access first in the body) is DISPROVED: taking
  `&shapes[mask][r*5+c]` into a pointer before the cell fetch is byte-identical
  to the committed body. Twenty variants measured, listed above the marker;
  none moved the instruction, and every variant that DID move it added
  instructions or a frame slot. `strict == register-blind == offset-blind`, so
  by the §6B triage this is a pure scheduling permutation — record it and
  move on.

---

# objrect.c

## Lane `fable-a` / `objrect` — object rectangles, path tiles and work orders

`LEGOLAND/objrect.c`, created 2026-09-05. **Six of six functions exact**
(`tools/audit.py LEGOLAND/objrect.c` → PASS, 630 instructions, `/W3` clean).
Every one ends in a real `ret`, none is recursive, so all six are promotable
and all six carry the plain `// FUNCTION:` marker.

| address | name | insns | pct | audit | marker |
| --- | --- | --- | --- | --- | --- |
| 0x0045e770 | `SetObjectDoorFlags` | 78 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0045e770` |
| 0x0045e850 | `ClearObjectUserFlags` | 80 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0045e850` |
| 0x0045e4a0 | `RefreshObjectAtPos` | 110 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0045e4a0` |
| 0x0045d5d0 | `SubtractObjRect` | 118 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0045d5d0` |
| 0x0045d3d0 | `RemoveObjectPathTiles` | 122 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0045d3d0` |
| 0x004777f0 | `GetRouteNode` | 122 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x004777f0` |

No function was renamed: every name comes from an existing caller's `extern`
(`popup.c`, `mappath.c`, `objmap2.c`, `workers.c`, `workorder.c`,
`workorder2.c`, `simcore.c`).

---

## Codegen levers (for `docs/DECOMP.md`)

- **AN APPEND INTO A GLOBAL ARRAY HAS THREE SPELLINGS AND THEY DIFFER IN TWO
  INDEPENDENT WAYS.** Filling `g_obj_rects[n]` from four scalars in
  `SubtractObjRect`:
  1. *Through a `Rect4* q = &g_obj_rects[n]`.* The address is materialised in
     a register (`mov ecx,eax / shl ecx,4 / add ecx,BASE`, the original's
     form) — **but the stores kill the CSEs of the address-taken `out`**, so
     `out.top` and `out.bottom` are reloaded from the frame in every one of
     the four blocks. 57/119.
  2. *Direct `g_obj_rects[n].field = v` subscripts.* The CSEs survive — VC6
     recognises the store as being to a known global object — **but the base
     folds into each store's `disp32`** (`mov [ecx+g_obj_rects], esi` with
     `ecx = n*16`), which is one instruction shorter per block than the
     original. 113 instructions against 118.
  3. *Fill a scalarised `Rect4 t` and assign `g_obj_rects[n] = t;`.* Both at
     once: the struct-assignment lowering materialises the destination address
     in a register, and every field VALUE is read before the first store. This
     is the original, and it took 117/118 first try.
  So "does this store kill my loads?" and "how is the address formed?" are
  separate questions with separate answers, and only the struct-assignment
  shape answers both the way the original does.
- **AN INLINE `SetRect4(Rect4* q, int l, int t, int r, int b)` IS NOT A
  SUBSTITUTE FOR THE STRUCT ASSIGNMENT.** It gets the address form right and
  evaluates the arguments before the stores, but the parameter `q` is still an
  opaque pointer, so block 1's stores still kill `out.top` for block 2. 111 of
  118. The helper only defers the kill by one block; the struct assignment
  removes it.
- **A LOOP-CONDITION READ OF A GLOBAL THAT THE BODY ALSO WRITES: MIRROR IT IN
  A LOCAL AND PUT THE RE-READ IN THE MISS ARM.** `SubtractObjRect`'s original
  parks `mov eax,[g_obj_rect_count] / jmp <latch>` past the epilogue and jumps
  to it from the "no intersection" test. Writing the condition as
  `i < g_obj_rect_count` puts that load in the SHARED latch instead (one
  instruction short, no exiled block) no matter how the loop is spelled —
  `for` with `continue`, rotated `do/while`, comma-increment condition. What
  reproduces it is `i < n` on a local mirror plus `else { n = g_obj_rect_count; }`:
  the arm then contains exactly the load, ends in a jump, and the block-exile
  rule parks it at the end. Writing the same assignment as
  `if (!hit) { n = g_obj_rect_count; continue; }` does NOT exile it — VC6
  inverts the test and lays the arm inline. **The arm has to be an `else`, not
  a guard with a `continue`.**
- **A FREE `volatile` READ CLOSED A 14-OF-118 *ALLOCATION* RESIDUAL — the
  class the triage calls "source is nearly powerless".** After the shape work
  above, `SubtractObjRect` measured strict 14 / register-blind **0** /
  offset-blind 14: the same instructions in the same order with `s.left` and
  `s.top` swapped between `esi` and `edx`. Nine source permutations (field
  order in the `t` fill, declaration order, compare operand order, `t = s`
  copies, four spellings of the `s = *p` copy) were all byte-identical at 14.
  Marking the miss arm's read volatile — `n = *(volatile int*)&g_obj_rect_count;`,
  a load the original performs anyway — closed all 14. **So a
  register-blind-zero residual is not automatically a floor: the free-volatile
  experiment is worth running even when the triage says allocation.** The
  mechanism is the documented one (the barrier stops the CSE with the hit
  arm's `n = g_obj_rect_count - 1` load) but the recorded outlook for
  `strict >> rb` should be softened from "nearly powerless" to "try the free
  volatile first".
- **AND THE PLACEMENT OF THAT VOLATILE READ IS THE WHOLE LEVER.** On the
  `- 1` load instead it closes 11 of 14 and leaves 3: the `s.bottom` spill
  store moves two indices EARLY, because VC6 will not hoist a volatile access
  across a store and so sinks the store below it. Same sharpening as
  `ClearCellForPath`'s 166-vs-167, now measured on a second body.
- **RUN THE LOOP ON PLAIN INTS AND WRITE THE `Pos` ONLY AT THE CALL SITES.**
  `RemoveObjectPathTiles` walks a rect and passes `&p` to two callees per
  cell. Written as `for (p.y = ...; ...; p.y++)` — which is exactly what
  `objmap2.c`'s `RemObjFromMap` tail does, and matches there — the
  address-taken pair is pinned in memory, the `x * 20` strength reduction is
  lost (the original hoists `lea edi,[ebp+ebp*4] / shl edi,2` and does
  `add edi,0x14` at the latch) and the body comes out 136 instructions against
  122. With `int x, y` counters and `p.x = x; p.y = y;` immediately before the
  calls it is 122/122 exact, first try. **The two shapes are not
  interchangeable and the sibling body is not evidence either way.**
- **TWO IDENTICAL ARMS MERGE AT THE FIRST SITE, SO THE LATER ONE MUST BE THE
  ARM THAT JUMPS.** `GetRouteNode` has three `axis = 5; cost = -1;` blocks;
  the original merges the first two (`je` back into the earlier block) and
  leaves the third alone. Writing the second as
  `if (!(def->flags & 0x200000)) { 5; -1; } else ...` makes it the
  fall-through, VC6 emits a second copy, and the function is three
  instructions long. Written positively — `if (def->flags & 0x200000) { ... }
  else { 5; -1; }` — it becomes the jump arm and merges. This is the
  merge-at-the-first-site rule applied to a statement pair rather than to
  `return K`.
- **INLINE-ARGUMENT ARITHMETIC IS EVALUATED RIGHT TO LEFT, WHICH DECIDES
  WHETHER A GUARD RE-TESTS.** `MapCellAt(def->dx + pos->x, def->dy + pos->y)`
  computes the Y sum LAST, so its own flags carry the helper's `x >= 0` guard
  and VC6 emits a bare `js`. The original re-tests with `test eax,eax / jl`,
  which is what a Y sum scheduled BETWEEN the X sum and the guard forces —
  i.e. the sums were STATEMENTS (`t.x = ...; t.y = ...; MapCellAt(t.x, t.y)`),
  not expressions in the arguments. Worth 52/76 → 78/78 on
  `SetObjectDoorFlags`. This narrows the recorded "arithmetic in a call
  argument runs before the callee's guard": it does, but it runs in argument
  order, and when the original re-tests, the arithmetic was NOT in the
  arguments.
- **A TWO-TERM SUM'S DESTINATION REGISTER FOLLOWS THE DESTINATION SYMBOL —
  confirmed on a Manhattan distance.** `h = abs(dx) + abs(dy)` evaluates the
  y term first (into `ecx`) and lands the sum in `ecx` in BOTH written orders;
  the original's `add eax,ecx` puts it in the x term's register. Only
  `dy = abs(...y...); h = abs(...x...); h += dy;` — where `h` IS the x term —
  reaches it.
- **A PAIR OF CONSTANT STORES CAN BE A SCHEDULING BOUNDARY PAIR.**
  `GetRouteNode`'s `n->parent = 0` and `n->dir = 0` are interleaved into the
  `cdq/xor/sub` abs sequence in the original. Written together, before or
  after the sum, they emit together and cost 5. Split either side of the
  `h += dy` they land on the original's indices exactly (0 of 122). Recorded
  as a reconstruction of the SCHEDULE, not a claim about the original source's
  statement spacing.
- **`if (def->type && def->type != 2)` RATHER THAN `!= 0` KEEPS A ZERO
  REGISTER OUT OF A PRE-PUSH GUARD.** In `RemoveObjectPathTiles` the `!= 0`
  spelling materialises `xor ebx,ebx` before the guard, turns `test ax,ax`
  into `cmp ax,bx` and `mov byte [..],0` into `mov byte [..],bl`. Existing
  rule, second measurement.

## Data structures and rules recovered

- **`Cell` +0x12 is not padding.** It is a 16-bit door-direction field: the
  low nibble is the ENTRANCE side (4/8/1/2 = below/above/right/left, in the
  order `GetObjEntranceDir` tests them) and the high nibble the EXIT side
  (0x80/0x40/0x20/0x10). Several objects' doors can share one tile. Written by
  `SetObjectDoorFlags`, cleared by `ClearObjectUserFlags`. `legoland.h`'s
  `Cell` should gain the field at integration (this file declares a local
  `MapCell` with it rather than editing the shared header).
- **`ObjDef` +0x24/+0x25 are the EXIT offset and they are SIGNED BYTES**,
  where the entrance offset at +0x0c/+0x10 is a pair of ints. Both
  `ObjHasExit` (0x0045e690) and `GetObjExitDir` (0x0045e710) read them with
  `movsx`. `ObjHasExit` is simply "the exit differs from the entrance".
- **`ObjHasEntrance` (0x0045e620) is three tests**: the class is non-null, its
  type at +0x20 is none of 0, 2 or 3, and the entrance offset falls OUTSIDE
  the class footprint rect at +0x3c (which it copies onto its own stack by
  `rep movsd`, five dwords — so the footprint `Rect` including its `next` link
  is passed around by value in this family).
- **`ObjDef` +0x2a is a signed short compared against 1** to pick the route
  cost of a "may be built over" object: `> 1` costs 20 and classes the tile 4,
  otherwise 9 and class 3. Nothing else matched reads it yet, so its meaning
  is open; it is declared `short w2a` here.
- **`RouteNode` +0x20 (`axis` in simcore.c) is written by `GetRouteNode` as a
  TERRAIN CLASS**, 0..5, and `RequestRoute` then treats class 1 as "stop the
  search here" and 0/1 as "no turn charge". The full model is in the file
  header §5. Note `f` is seeded as `cost + h`, i.e. the tile's own entry price
  plus the heuristic, not `g + h`.
- **The route heuristic is Manhattan** — `|dx| + |dy|` to `g_route_to`
  (0x004bb5a0 / 0x004bb5a4) — and `g` is seeded to `INT_MAX`.
- **`RefreshObjectAtPos` resets a cell's flags to ZERO**, unlike the placement
  stamp `AddObjectToMapByCursor` which merges `(old & 0x10) | flags`. It also
  writes `rf = 0` where the stamp writes 2, and sets neither 0x80 nor the
  0x8000 "tall" bit. So a work-order refresh deliberately drops every map bit
  the order may have painted on the object's tiles.
- **`SubtractObjRect`'s four remainder pieces** are, in append order: the
  full-width strip above, the left strip clipped to the intersection's rows,
  the right strip clipped the same way, and the full-width strip below. The
  scan steps back over the swap-erased entry and the loop condition re-reads
  the count, so the pieces it appends are themselves re-tested in the same
  call.

## Callees named for the first time

| address | name | what it is |
| --- | --- | --- |
| 0x0045d560 | `IntersectRect4(Rect4* out, Rect4* a, Rect4* b)` | four min/max branches into `out`, then returns non-zero when the result is non-empty. Not an export. |
| 0x00477730 | `FindClosedRouteNode(Pos*)` | linear scan of the closed list (0x00668fc4) for the node whose `Pos` at +0x08 is this tile. |
| 0x004777c0 | `FindOpenRouteNode(Pos*)` | the same scan over the open list (0x00668fc0). Byte-for-byte the twin of the above with one global changed. |
| 0x00482b60 | `TileJoinsPathNetwork(Pos*)` | `FindPathSquare(pos)`, and on a hit it FLUSHES the pending path-GFX batch (0x0066b46c, through 0x00482b20) and zeroes it before returning `(square->flags & 2) != 0`. Not a pure predicate — the flush is a side effect on the route search's path. |

Names reused rather than invented, from other files' externs:
`ObjHasEntrance` / `GetObjEntranceDir` / `ObjHasExit` / `GetObjExitDir`
(bigrender.c), `GetTileCentre`, `UpdatePathNeighbours`, `RemovePathSquare`,
`HeapAlloc_w`, `g_obj_rects` / `g_obj_rect_count` (workorder2.c),
`g_edit_cursor`, `g_route_to` (simcore.c).

## Original behaviour reproduced, not fixed

- **`RemoveObjectPathTiles` sweeps the interior twice.** Pass 1's rect (the
  footprint grown one cell on every side) strictly contains pass 2's, so every
  interior cell is cleared, re-tiled and passed to `UpdatePathNeighbours` /
  `RemovePathSquare` a second time. Commented at the site.
- **`RemoveObjectPathTiles` has no bounds check.** The cells are reached as
  `g_map_rows[y][x]` with no width/height test, so an object whose footprint
  touches the map edge indexes one row/column off the grid. Same three
  statements as `objmap2.c`'s `RemObjFromMap` tail, same exposure.
- **`GetRouteNode` dereferences a null cell.** The bounds-checked fetch
  returns 0 off-map and the next instruction reads `flags` at +0x0c through
  it. `mappath.c`'s `ClearCellForPath` and `simcore.c`'s `RequestRoute` carry
  the identical unguarded dereference of the same helper — three sites now, so
  this is the cluster's convention rather than a one-off slip.

## Extern prototype types that differ from another file's declaration

- `SetObjectDoorFlags` / `RefreshObjectAtPos` take `MapObj*` here where
  `popup.c`, `workers.c` and `workorder.c` declare `ObjElem*` / `void*`. Both
  are one pointer argument, so nothing changes on either side; the other files
  were NOT touched.
- `HeapAlloc_w` is declared `void* HeapAlloc_w(unsigned int)` as in
  `mappath.c`. `IntersectRect4` returns `int`.
- Nothing in this lane needed a type that contradicts another file's.

## Things worth checking at integration

- `legoland.h`'s `Cell.pad12[2]` can become a live `unsigned short` door
  field; this file declares its own `MapCell` only because the header is owned
  elsewhere. `loadmap.c:339` already zeroes `+0x12` explicitly through a cast,
  which is consistent.
- `docs/HANDOFF.md` §6B's triage table entry for `strict >> rb` ("source is
  nearly powerless") is contradicted by `SubtractObjRect`: rb was 0 and one
  free volatile read closed all 14. Suggest the table gain "run the
  free-volatile experiment before calling an allocation residual a floor".
