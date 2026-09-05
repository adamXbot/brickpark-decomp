# Lane `fable-b-ridemisc` — small ride helpers (2026-09-05)

File: `LEGOLAND/ridemisc.c` (new). Six functions from the unmatched-callee
frontier. `tools/audit.py LEGOLAND/ridemisc.c` ends **PASS**; `/W3` clean.

| address | name | insns | bytes | audit | mismatch | marker |
| --- | --- | --- | --- | --- | --- | --- |
| 0x0043d7c0 | `MechanicsHut_EvictRiders` | 64/64 | 182/182 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x0043d7c0` |
| 0x004333e0 | `JcBoat_Advance` | 69/69 | 217/217 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x004333e0` |
| 0x0044e890 | `RandomFavouriteFood` | 70/70 | 137/137 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x0044e890` |
| 0x0044e790 | `RandomFavouriteRide` | 73/73 | 145/145 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x0044e790` |
| 0x004048b0 | `Copters_SetFull` | 75/75 | 227/227 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x004048b0` |
| 0x0042cf70 | `EarthSlide_LaunchCar` | 75/75 | 201/201 | **[OK]** | 0 | `// FUNCTION: LEGOLAND 0x0042cf70` |

Six of six exact. No renames — every name was already declared by the file
that calls it (`ridecb9.c`, `junglecruise.c`, `rides.c`, `mechrides.c`,
`ridecb1.c`). All six end in a real `ret` and none is recursive, so all six
are promotable as committed.

---

## New codegen levers (each with its measurement)

- **An aggregate local blocks the reuse of a DEAD PARAMETER's home slot.**
  `MechanicsHut_EvictRiders` spills one of its two derived coordinates. As two
  plain `int` locals VC6 homes the spilled one in the `cls` argument slot —
  which is free the moment `cls` is copied into `ebp` — and the frame collapses
  to `push ecx`; as one `Pos` (or an `int[2]`) it takes a fresh slot and the
  frame is the original's `sub esp,8`. **11 of 64, and the ONLY residual.**
  Declaration order of the aggregate against the two pointer locals is inert
  (three permutations byte-identical), and a `for` header or an inner-scope
  `next` changes nothing. This is the counterpart of the recorded "two argument
  slots reused as locals keep an 8-byte frame with nothing in the source asking
  for it": when the original's frame is BIGGER than yours and the difference is
  exactly one spilled scalar, the source had that scalar inside an aggregate.

- **An ASCENDING array walk anchors its induction variable on the LAST field
  it touches; a DESCENDING one on the first — and the spelling has to follow
  the anchor.** `JcBoat_Advance` has three fills of the same 8-byte
  `{int x; int y}` element, two up and one down:
  - up: the original's pointer starts at `&wob[i].y` and stores through
    `[eax-4]` / `[eax]`. Only **two per-field assignments** produce that.
    A whole-struct assignment `b->wob[i] = b->wob[64];`, a pointer walk
    `*p++ = ...`, and a y-then-x scalar pair all anchor on `.x` and cost 3
    instructions' worth of operands each (6 of 69 total).
  - down: the original addresses `[eax]` / `[eax+4]`, which is what the
    **whole-struct assignment** gives; the scalar pair is wrong there.
  So do not carry a spelling from one loop to its neighbour — measure per loop,
  by walk direction. The same bias appears from the READ side in
  `EarthSlide_LaunchCar` (below).

- **A read cursor into a const table needs to be a POINTER, and sometimes a
  BIASED one.** `EarthSlide_LaunchCar` walks a 4-entry `{int dx; int dy}` table
  in step with a list walk.
  - `kSlideQueueSpots[i]` with an `int i` is one instruction and two bytes
    SHORT (74/199 vs 75/201): VC6 folds the strength-reduced address into both
    loads, never materialises the cursor, and the `mov eax,ecx` that preserves
    the computed `target.y` disappears with it. A `const Pos*` walked with
    `sp++` restores it — **9 of 75.**
  - The last 2 are the +4 anchor. NOTHING done to a `const Pos*` reaches it:
    `sp->x`/`sp->y`, `sp[0].x`, `Pos s = *sp;`, a named local for either field,
    `(sp++)->y`, `for (...; q = q->next, sp++)`, `sp++` before the list step, a
    `do/while`, a flat `const int[8]` view read as `ip[0]`/`ip[1]`, and
    `kSlideQueueSpots + 1` with `sp[-1].x`/`sp[-1].y` are all 2 or worse.
    **`const int* spot = &kSlideQueueSpots[0].y;` read as `spot[-1]` /
    `spot[0]` and stepped `spot += 2` is exact.** Same walk, byte for byte —
    the biased int cursor is simply how the anchor is spelled in C.

- **A duplicated cursor-advance block is worth more than the `&&` that would
  merge it.** Both favourite-pickers fail a candidate on two independent tests
  and then advance the wrap-around scan. Written as one `&&` guard VC6 emits a
  SINGLE shared miss block reached by `jne`, and the function comes out 7
  instructions short (**19 of 70**). Two guards, each ending in `continue`,
  give the original's two inline copies — and the extra references to `n` and
  `start` that the second copy creates are also what rank them into edi/ebx
  instead of ebx/ebp. An `else if` nest is byte-identical to the `continue`
  form. (The recorded exile rule from the other side: there, merging guards
  pulled a shared block out; here, splitting them duplicates it inline.)

- **Naming the intermediate POINTER, not the value, advances the scratch
  rotation.** `RandomFavouriteRide` compares the class kind twice, so VC6 CSEs
  the 16-bit load into `cx`. With `e->data->type` spelled out at both compares
  the body is byte-exact except that the `tries--` temporary at the loop
  back-edge lands in edx where the original has ecx (strict 2, register-blind
  0). `d = e->data;` first closes it. A `short` or `unsigned short` local for
  the VALUE does not (still 2); an `int` one is worse (5). A
  volatile-qualified load of the same pointer is byte-identical to the plain
  local — **on this body the free-volatile trick and the named pointer are the
  same lever.** Its twin, with only ONE compare and therefore no CSE, needs no
  local at all: the twins' residual is NOT shared here, which is the first
  counter-example to "twins share their residual index for index" in this
  tree. They share their SOURCE and diverge in one construct, and the
  construct is exactly where the register pressure differs.

- **`tries` before `start`.** In both pickers the two seeds must be assigned in
  that order; the other order moves the `mov ebx,esi` and costs 1 at index 11.
  Cheap to sweep, same family as the recorded "initialiser ORDER decides where
  a spill store lands".

- **Confirmations (no new information, recorded so they are not re-derived):**
  the operand order of a two-term sum is canonicalised and inert (`t.x + s.x`
  vs `s.x + t.x`, both fields, all four combinations byte-identical);
  `* 256` and `<< 8` are the same object; splitting `(a + b) << 8` into an
  assignment plus a `<<=` is inert; naming the path or the world `Pos` in a
  local is inert; `&b->path[0]` and `b->path` are inert.

---

## Data structures and rules recovered

### `MechanicsHut_EvictRiders` (0x0043d7c0) — shared by the hut and the shed

`ridecb9.c` had it as an unmatched extern with the note "the potting shed calls
it too". The body confirms the split and fills in the third argument's meaning:

```
MechanicsHut_EvictRiders(ObjDef* cls, BPosW key, int shed)
```

- `key` is the packed map square, passed BY VALUE as a two-byte aggregate. VC6
  extracts the halves with the `[esp+N]` / `[esp+N+1]` dword pair and masks
  (`and eax,0xff`), and the loop's identity test re-reads the parameter slot as
  a `word` — the `BPosW` union spelling, unchanged from `jcroute.c`.
- The blokes are dropped at the CLASS's door offset (`ObjDef+0x0c/+0x10`, the
  same pair `popup.c` and `logflume4.c` name) plus the square, in 24.8:
  `b->world.x = (cls->dx + key.b.x) << 8`.
- `shed` picks the long-term action only: **0x10 for the POTTING SHED, 0x11
  for the MECHANICS HUT** (`ridecb9.c` records the hut's stage-3 arm handing
  out 0x11, and the shed passes 1 here). It is the whole difference between
  the two classes' remove handlers.
- Bloke flags `0x28` (`8` = using this ride, `0x20` = hired) are dropped
  together, in one `and word [esi+0x62],0xffd7`.
- A staff member whose stage byte has already reached 0x64 — the "being
  fired" half of the hut's state machine — is unlinked but NOT freed, and gets
  job slot 0x64 instead of a new action. That is the same node leak
  `ridecb9.c` records at stage 0x67, reached from the other side.
- **Both arms call `RemoveBlokeFromList`, and the original emits the call
  TWICE with ONE shared argument-push pair** — the two arms clean 0xc and 8
  bytes respectively, so the pending `add esp` cannot cross the join and the
  source has the call in each arm. Read the argument clean-up, as
  `SchoolCarManoeuvreD` teaches.

### `JcBoat_Advance` (0x004333e0) — the jungle cruise's DOCKING sequence

`junglecruise.c` had `+0x3e4` as "squares left in this leg" and this function
as "finishes a leg". It is more specific than that: `JcBoat_Step` sets
state 0x10 and `leg = 3` the moment a boat reaches the station's route-end
square, and the four ticks that follow are the arrival animation, driven
entirely by rewriting the 80-entry wobble buffer:

| leg | what the tick does |
| --- | --- |
| 3 | animate in-1/out-4, then `wob[64..79] = wob[64]` — the boat stops dead half way across the last square |
| 2 | animate, `wob[0..63] = wob[64]` — already parked when playback starts — and aim one square further south |
| 1 | animate, aim south again, then `wob[79..73] = wob[72]` — the settle as it ties up |
| 0 | unlink and FREE the boat; return the NEXT one |

`from`/`to` are always 1 and 4 (in by the north side, out by the south): the
straight run down out of the station. `b->leg` is re-read at each test rather
than cached — the wobble writes may alias, and the original reloads it three
times, which is what the source's direct field reads give.

### `RandomFavouriteRide` / `RandomFavouriteFood` (0x0044e790 / 0x0044e890)

`rides.c` hands every new visitor three favourite rides and one favourite food
from these. Both scan the LLIDB linearly from a random start for an element
that is an ODF loaded on THIS level (`type_flags & 0x14 == 0x14`, i.e.
`0x10` ODF + `0x4` on-level) and of the right class kind — `ObjDef+0x20 == 5`
is the food/shop bucket, "not 0 and not 5" is a ride. The scan wraps and gives
up only when it returns to its starting index, which is the sole path that
returns 0.

**TWO ORIGINAL BUGS, both reproduced and commented at the site:**

1. `rand() & 0x1f` is plainly meant to pick the Nth match, but the accept arm
   never advances the cursor — the loop re-fetches the SAME element until the
   counter runs out, so the function always returns the FIRST match from the
   random start. The counter is pure noise.
2. When `rand() & 0x1f` is 0 (one time in 32) the loop never runs and the
   function returns the **uninitialised local** `e` — stack junk, which the
   caller stores as a favourite. That is the third epilogue,
   `mov eax,[esp+0x10]`.

### `Copters_SetFull` (0x004048b0) — the HELICOPTERS machine starts its run

Reached from two sites in the ride's own update, 0x00404b68 (the fill timer
expires) and 0x00404d5f (the seated count reaches the class's capacity at
`ObjDef+0x2e`); both clear the 0x4000 "still filling" bit themselves first.

- The record is `mechrides.c`'s `CoptersRec`. New fields: **+0x08 is a flags
  dword** (`1` = running, `0x4000` = filling) and **+0x0c a mode word** set
  to 2 here. `riders (+0x03) = seated (+0x02); seated = 0;` moves the boarding
  count over to the "aboard" count.
- The seat record's **+0x00 is a DWORD here** (`or dword ptr [eax+0x38],edi`)
  where `joust2.c`'s drawing view reads only its low byte — an extern/struct
  type divergence to leave alone, not to align.
- Per occupied copter: `+0x00 |= 1`, `+0x1d = 3` (flight stage), `+0x04 = 0`
  (the shared animation frame `joust2.c` names).
- **Only FIVE of the six seat records are touched**, and all three five-line
  runs enumerate them **1, 0, 2, 3, 4**. That is not an adjacent-store
  reversal — writing 0,1,2,3,4 emits 0,1,2,3,4 and costs 7 — the source really
  lists copter 1 first, three times over. (The ride's draw pass has its own
  fixed order too: `joust2.c` records 0, 2, 3, 4, 1.)
- The two sounds are `g_copters_fx[0]` and `[1]` (the FX table at 0x004b4140
  that `Copters_Create` loads). Slot 0 is the one-shot; slot 1 is the LOOP,
  started, immediately paused, and given a **0xb54 ms** completion callback
  (0x004048a0, first named here as `Copters_ResumeSFX`) that resumes it — so
  the loop is delayed by exactly the length of the start-up sound. Four cdecl
  calls, one merged `add esp,0x30`.
- The sound source is the record's map square (kind 2) with **+0x04 left
  uninitialised**, the same habit `money.c` and `ridecb3.c` record.

### `EarthSlide_LaunchCar` (0x0042cf70) — the queue shuffles up

`ridecb1.c` had the one-line summary ("pop the front queuer, advance its
action and re-shuffle the queue"); this is what the shuffle is.

- The front queuer loses the **0x40 "queued"** bloke flag (the bit
  `EarthSlide_JoinQueue` sets) and his stage byte steps on, then he is popped.
- **0x0042d040, first named here as `EarthSlide_PopQueue`:** unlink the head
  node from `SlideRec+0x1c`, clear the queue TAIL at **`SlideRec+0x20`** if it
  pointed at the popped node, and decrement the count byte at +0x18. `+0x20`
  is one field past the end of `ridecb1.c`'s `SlideRec`.
- The four queue standing spots are a const table at **0x004b65c0**:
  `(0,4) (0,3) (0,2) (0,1)` — four cells due south of the ride's base square
  (`g_slide_item+0x0c/+0x10`), closest LAST. Everyone still in the chain is
  re-aimed at the spot one place further forward and re-planned with
  `CalcMoveLine` + `NewDirForAction`, the same four-line idiom
  `MechanicsHut_Tick` uses.
- **LATENT ORIGINAL BUG, reproduced as written:** the shuffle walk is bounded
  by the QUEUE, not by the table. It indexes the 4-entry table by the queuer's
  position in the list, so a fifth queuer would read the string constants that
  follow the table at 0x004b65e0. The queue-length byte at +0x18 is the only
  thing that keeps it from happening.

---

## Callees named for the first time

| address | name | evidence |
| --- | --- | --- |
| 0x0042d040 | `EarthSlide_PopQueue` | unlinks `SlideRec+0x1c`'s head, fixes the tail at +0x20, decrements +0x18 |
| 0x004048a0 | `Copters_ResumeSFX` | one-line forward to `ResumeSinglyPausedSample` (0x00492910); installed here as the 0xb54 ms SFX completion callback |

## Extern-type divergences recorded (do NOT align)

- `MechanicsHut_EvictRiders` — `ridecb9.c` declares its second parameter
  `unsigned int bp`; the DEFINITION needs the two-byte `BPosW` **by value**,
  which is what emits the `[esp+N]`/`[esp+N+1]` half extraction and the `word`
  compare in the loop. Both spellings are correct on their own side.
- `CopterSeat +0x00` — a DWORD in `ridemisc.c` (`or dword ptr`), a
  `unsigned char` in `joust2.c`'s drawing view.
- `LLIDB_GetElement` — declared `void` returning here (the result is unused),
  `int` in `memdb.c`/`misc3.c`. Inert on this body but recorded.
