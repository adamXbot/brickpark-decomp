# FGH pickup — Coaster3D_BuildTrackMesh with the scope-V levers (2026-09-08)

Branch `fgh-pickup` (from `cursor/fgh-100c` @ `41bd26f4`, the most advanced FGH
branch: 42 of 48). Harness: a splice-and-score script in the session
scratchpad (`h2.py <file> <Func> <VA> body.c…`), nothing committed. No
production body changed in this round; the six WIPs stand as fgh-100c left
them (Raster 142, StepSchoolCar 83, Temple 18, JungleCruise 11, SpaceTower
10, Mesh 6).

## Why these two levers were tried here

Scope V's `EventTick_Clear` closed on 2026-09-08 with two general levers
(DECOMP, SCOPE V entry): a block copy into a sibling member of a local
aggregate as a free store-forwarding kill, and `t.x = v; t.x += (int)p;
t.x -= (int)p;` through a two-member struct as a copy that copy propagation
cannot see through but that instruction selection cancels to a bare
`mov r32, r32`. Mesh's residual is a copy-versus-reload of the element
(original: `mov ecx,[eax]` / `mov esi,ecx` / `mov [ebp-14h],ecx` / push esi;
fgh-100c's best: store first, reload esi from the slot, 442/441 B, 6), which
is the copy-propagation shape exactly: the original's first push uses `e`,
so `e = L` was not propagated into the call argument.

## Result: the window is reproducible, the register tie is not (yet)

With the record `struct { void* elem; Vec3f pos; } cur;` (Build P) plus

```c
cur.elem = list[i];
tc.y = slot;            /* struct { int x, y; } tc; */
tc.x = (int)cur.elem;
tc.x += tc.y;
tc.x -= tc.y;
e = (void*)tc.x;
o->hooks[slot].get_dir(o, e, &dir);
```

the element window comes out as the original's shape for the first time:
`mov eax,[eax]` / `mov esi,eax` / `mov [ebp-14h],eax` / `push esi`, copy before
the dead record store, no volatile, no extra instruction, 151 instructions.
What breaks is one callee-saved decision upstream: the anchor's third use
makes `slot` a three-use invariant, VC6 hoists it into ebx in the preheader
(`mov ebx,[ebp+10h]`), and the `out` cursor (also three register uses: head
load, push, `add ebx,10h`) loses the tie to the parameter and is spilled to
`[ebp+14h]` for the rest of the loop (30 strict, first 19). That tie is the
same rule scope V measured (weighted use count, ties to the earlier
definition; a parameter is defined first).

Anchors measured (all with the record and the two-member struct):

| anchor | effect |
| --- | --- |
| `slot` | window exact; `slot` → ebx, `out` spilled (30 strict) |
| `o`, `origin`, `list` | the anchor gains a use and rotates the callee-saved ranking (o → esi, e → edi, or origin → ebx); 53–85 strict |
| `o->hooks`, `&dir`, `&rot`, `&cur.pos`, `&xf`, `&o->hooks[slot]` | invariant, hoisted into a register in the preheader; 34–53 strict, 153–156 i |
| `out`, `&list[i]` | the cursor becomes a register IV; 46–54 strict |
| `cur.elem` (self-cancel) | not cancelled, three extra ops |
| `g_track_verts`, `&g_track_mesh` (constants) | not folded: a relocatable constant is not cancelled, 155 i |
| `slot` read through `*(volatile int*)&slot` for one or both calls | the volatile parameter read changes the frame (first 2) |
| flat `tc.x += slot; tc.x -= slot` or `+= (int)o` | folds back to Build P (36, first 27) — the two-member struct is required, as in CLEAR |
| `tmp = list[i]; cur.elem = tmp; e = tmp; tmp = 0;` and `tmp = e` | the record store disappears (150 i) |

Giving `out` a fourth register use for free was not found: an explicit
`TrackVtx* p = out` / `int* sh = &out->shade` walk compiles to the same
derived IV (151 i, 30 strict, unchanged).

## What would close Mesh

Either an anchor whose extra use costs nothing in the callee-saved ranking —
a loop-variant scratch value already live in the window that is neither the
element itself nor an address VC6 will hoist — or a zero-cost fourth
register use of `out`. Everything else about the body is already the
original's. Reopen from `scratchpad` variant `m5_slot` (window right) rather
than from the volatile union.

## Second round — the hoist is the whole obstacle (same day)

`assigned_exact_gate: 42`, `assigned_open: 6`, zero regressions, re-measured
with `validation/fgh/check.py` on this branch (`/tmp/fgh_pickup.json`), so the
numbers below are against a confirmed baseline. Nothing promoted.

**The anchor must be a loaded value, and it must not be loop-invariant.**

- **Anchor-free struct cancels all fold back to Build P** (151 i, 441 B,
  first 27, 36-40 strict): a bare member copy `tc.x = (int)cur.elem; e =
  (void*)tc.x`, a two-member chain `tc.y = …; tc.x = tc.y`, a one-member
  pointer struct, and the self-doubling cancel `tc.x += tc.x; tc.x -=
  (int)cur.elem`. The mirrored self-anchor (`tc.y` and `tc.x` both from
  `cur.elem`) does *not* fold but does not cancel either — it emits the add
  and sub for real (154 i). So the cancel needs two genuinely different
  values, exactly as in CLEAR.
- **Frame addresses (`lea`) never cancel.** `&dir`, `&rot`, `&cur.pos`,
  `&xf`, `&o->hooks[slot]`, `g_track_verts`, `&g_track_mesh` all leave the
  arithmetic in place (153-157 i). Only a value loaded from memory into a
  register cancels: `slot` (151 i, window exact) and, partially, `origin`.
- **Keeping the anchor's use count at two does not help.** Indexing one or
  both hook calls through `tc.y`, through a named `h = o->hooks`, or through
  a named `hs = &o->hooks[slot]` moves the hoist onto the new name instead
  (31-61 strict). The hoisted web wins ebx whatever it is called.
- **Loop-variant anchors are worse.** `i`, `n` and `last` change the loop
  shape (58-90 strict, first 2). An explicit `void** lp` list cursor or an
  `out += 6` cursor, anchored or not, gives 44-53 strict.
- **An escaped-pointer read does not stop the hoist.** `int* sp = &slot`
  outside the loop with `tc.y = *sp` (and optionally `o->hooks[*sp]`) is
  40 strict, first 19: VC6 still hoists the load. A `volatile` read of the
  parameter changes the frame (first 2).

**Why ebx is the whole fight.** The original's preheader is `mov edi,[ebp+8]`
(o), `mov ebx,0x6139c8` (`g_track_verts`), then it reuses the three dead
parameter homes for the list cursor, the `out` cursor and the down-counter.
`slot` is *not* hoisted in the original — it is reloaded from `[ebp+10h]` at
both hook calls. Giving it a third use crosses VC6's LICM threshold, it takes
ebx in the preheader, and the `out` cursor is pushed to `[ebp+10h]` and
reloaded in the inner loop (the 30-strict body). No spelling found keeps the
element window and the preheader at once.

## Other targets, read but not attacked this round

SpaceTower_Activate (10) and JungleCruise_Tick (11) are scheduling cycles and
an ebx/ebp swap with equal feature rows (fgh-100c §3); the scope-V levers do
not obviously address either.

**JungleCruise's ebx/ebp cell is not a byte-need case (new negative).** Scope
V established that a web with a *register byte* use takes the byte-capable
register absolutely, displacing others — which would explain fgh-100c's
"unexplained ebx/ebp" cell if the zero constant had a `bl` use. It does not:
a scan of the original body (0x00435750, 360 instructions) finds no byte
reference to ebx at all, and `inst` in ebp is used only as a base register
(`[ebp]`, `[ebp+8]`, `[ebp+0ch]`, each paying the disp8 byte). So the
original picked the *worse* encoding for `inst` and the cell stays
unexplained; the scope-V byte rule does not reach it. `LFEntrance_Remove` (scope H, 11) is the
cursor-save/origin shape and is the next candidate for the sibling-copy kill.
