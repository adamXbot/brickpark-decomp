# Lane notes — `fable-a` / `LEGOLAND/jcroute.c` (jungle cruise routing)

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
