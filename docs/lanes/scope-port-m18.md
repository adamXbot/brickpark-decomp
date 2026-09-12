# Scope PORT-M18 — P1-7, the visitor chain that grows on every load

> **PORT-M18 — Status: DONE (2026-09-12)** — branch `scope/PORT-M18`, cut from
> the PORT-M16 merge (`be436d48`). Brief: `docs/SCOPE_PORT_WAVE.md`.
> Replay: `portable/src/browser/replays/m18-load-adds-visitor-ghosts.js`.

**Result in one line: P1-7 is NOT a port defect and NOT a doubling. `LoadGame`
restores the Bloke chain and never restores `g_visitor_count`, the counter
`SpawnVisitor` gates on, so every load tops a fully restored park up from zero
and adds another `g_visitor_limit` visitors — 30 → 60 → 90 in free play, and
3 → 5 (the pool ceiling) in tutorial lesson 1, where it jams the park for the
rest of the session. The shipped LEGOLAND.EXE does exactly this: its
`LoadGame` body contains no reference to the counter at all. Nothing in
`LEGOLAND/*.c` was changed.**

---

## 1. The mechanism, in five globals

| global | x86 | who writes it | saved? |
| --- | --- | --- | --- |
| `g_people_head` | `0x0066b574` | `NewBloke` pushes, `DestroyBloke` unlinks, `LoadGame` clears and rebuilds | **yes** (BLK4, as pool indices) |
| `g_bloke_base` | `0x0066b57c` | `AllocBlokePool` (workers3.c:221), `max_blokes` × 172 bytes | the pool is re-allocated, the contents come from BLK4 |
| `g_visitor_limit` | `0x0083291c` | the level script's `MAXCAPACITY`/`MINCAPACITY` | **yes** — it is at offset `0x11c` inside the 0x3f0-byte `g_mapai` blob (`0x00832800`) that savegame.c:689/1138 writes and reads |
| `g_visitor_count` | `0x006661bc` | `InitBlokeAI` `++`, the leave path `--`, `ClearBlokeList` `= 0` | **NO** |
| `g_visitor_spawn_clock` | `0x006661c8` | `SpawnVisitor` | **NO** (harmless) |

`SpawnVisitor` (simcore2.c:122, `0x0044ea50`) is the whole gate:

```c
if (++g_visitor_spawn_clock >= 30 && g_visitor_count < GetVisitorLimit()) { ... }
```

So the count is the park's population as far as the simulation is concerned,
and the chain is the population as far as everything else is concerned. A save
records the chain and not the count, and a load therefore lands with the two
out of step by exactly the number of blokes the save held.

### Why the count is always ZERO on entry to a load

`BeginParkLoad` (gameframe.c:419, `0x00458b20`) tears the running park down
before either load path and calls `sub_483090` — `ClearBlokeList`
(pathobj2.c:451). Its last instruction in the original is literally

```
0x004830ab: mov  dword ptr [0x6661bc], 0
0x004830b5: ret
```

so it does not matter whether the player loads from the title screen after a
fresh start (BSS zero) or from the in-game options screen mid-park: the count
is 0 when `LoadGame` begins, and `LoadGame` leaves it there.

### What LoadGame actually does with blokes

`LoadGame` (savegame.c:1033, `0x0047e980`) BLK4 clears the chain
(`while (g_people_head) DestroyBloke(g_people_head);`), reads the count of
saved blokes, and for each one takes the pool slot back by index:

```c
b = (Bloke*)((char*)g_bloke_base + num * 172);
b->next = g_people_head;
g_people_head = b;
```

and then copies the 0x124-byte `g_bs` record into it. `DestroyBloke`
(`0x00483010`) does **not** decrement the count either — the leave path at
`0x0044f0ff` does that in the caller — so the clear cannot fix it from the
other side.

### Proved from the binary, not from our C

`LoadGame` is 3,552 bytes (`0x0047e980` to the next export at `0x0047f760`).
Scanning that byte range in `original/legoland.exe` for the four absolute
addresses:

```
g_visitor_count        0x006661bc  0 hits
g_visitor_spawn_clock  0x006661c8  0 hits
g_visitor_limit        0x0083291c  0 hits   (it arrives inside the g_mapai blob)
g_people_head          0x0066b574  4 hits   0x47edc9 0x47edd8 0x47ee51 0x47ee5d
g_bloke_base           0x0066b57c  1 hit    0x47ee33
```

And the complete tree-wide cross-reference of `0x006661bc` in `.text`:

| site | what it does |
| --- | --- |
| `InitBlokeAI` `0x0044e920` (rides.c:256) | `++g_visitor_count` — the only incrementer, and `SpawnVisitor` is its only caller |
| `SpawnVisitor` `0x0044ea50` (simcore2.c:122) | reads it against `GetVisitorLimit()` |
| `0x0044f0ff` (goalstate.c:466) | `DestroyBloke(b); g_visitor_count--` — a bloke leaving the park |
| `ClearBlokeList` `0x00483090` (pathobj2.c:451) | `= 0` |
| `0x0046abd4` (eventtick2.c:251) | reads it for the `ParkVisitors` goal |

Nothing recomputes it from the chain, anywhere. **This is the original's own
defect.** It is not a dual address, not a split object, not a by-value struct,
not a shim return value, not a callback slot, and not the CRT: the byte-exact
`LoadGame` has nowhere for a port bug to hide, because the store simply is not
in the shipped code.

### The correction to P1-7's wording

P1 wrote "one save/load DOUBLES the visitor chain" and suspected
`StartFreePlayPark` (uimisc2.c:404) and the load path both appending. Three
corrections:

1. **`StartFreePlayPark` is not on the load path at all.** `GameFrame`
   (gameframe.c:497) branches: `if (g_game_load_pending) { ... LoadGame(path);
   InitGameInterface(0); } else { KillHelpText(); StartPark(); }`. A load never
   runs a park-start routine, so nothing appends twice.
2. **It is not a doubling, it is `+visitorLimit` per load**, cumulative:
   30 → 60 → 90, measured (§2). The "doubling" was a coincidence of the
   30-visitor park having a 30-visitor limit.
3. **Neither end duplicates anything.** The save counts the chain once
   (savegame.c:741) and walks it once (savegame.c:751); the load clears it
   before rebuilding it (savegame.c:1169). The chain after a load holds no
   duplicate pool slot — 30 blokes occupy slots 0..29, 60 occupy 0..59.

### The arithmetic

```
chain after a load  =  min(blokes in the save + g_visitor_limit, g_map->max_blokes)
g_visitor_count     =  chain - blokes in the save
```

and the surplus is **permanent, not transient**: when a ghost leaves,
goalstate.c decrements the count, the count falls below the limit, and
`SpawnVisitor` replaces it on the next tick. The park sits at chain 60 /
count 30 indefinitely.

---

## 2. The measurements

Build `be436d48` + this lane, `portable/build-wasm` Release + `LL_ILP32=ON`,
served on 8853, `legoland.html?args=-nointro+WINDEBUG&awake=1`, own tab,
virgin IDBFS. 35.7 fps, `llStats().dead` null and **0 traps in every run
below**. Free play is `Scripts\FreePlayTest.txt`: `MAXCAPACITY 200`,
`MINCAPACITY 30`, so `visitorLimit` 30 and `max_blokes` 200.

### The control arm — three loads, two full save/load cycles

| step | chain | `g_visitor_count` | `g_visitor_limit` | note |
| --- | --- | --- | --- | --- |
| park built fresh | 30 | 30 | 30 | pool slots 0..29, kinds `{1:30}` |
| save slot 1 | — | — | — | `1save1.sav` **755,359** B, `1save1.sh` 272 B |
| *(full page reload)* | — | — | — | hash `0x311d0e24`, the save survives IDBFS |
| load from title, sim frame 1148 | 0 | 0 | 0 | the park is torn down |
| **sim frame 1165** | **30** | **0** | **30** | LoadGame has returned. Thirty restored, count zero |
| 1201 → 2557 | 31 → 60 | 1 → 30 | 30 | one spawn per ~30 sim frames |
| 2557 … 2646 | **60** | 30 | 30 | stops dead when count == limit, kinds `{1:60}` |
| in-session load (OPTIONS → Load, **no reload**) | 60 → 31 → 60 | 30 → 1 → 30 | 30 | identical; `BeginParkLoad` zeroed the count |
| a third load | 30 → 60 | 0 → 30 | 30 | identical again |
| save slot 2 at chain 60 | — | — | — | `1save2.sav` **764,305** B, i.e. **+8,946** |
| *(reload)* load slot 2, frame 819 | **60** | **0** | 30 | sixty restored, count zero |
| frame 2428 … 2478 | **90** | 30 | 30 | kinds `{1:90}`. **Not a doubling** |

The 8,946-byte growth is the ghosts being written back as real visitors:
BLK4 costs 4 bytes of pool index + the 0x124-byte `g_bs` record per bloke =
30 × 296 = 8,880, and the remaining 66 bytes are rider entries and block
bookkeeping. That is what makes the growth compound.

### The treated arm — the same build, the same save, one word written

At the moment `LoadGame` returns, write `g_visitor_count = <chain length>` by
hand through `llAddrs()` and nothing else:

```
armed: { simFrame: 11546, chain: 31, countWas: 1, wrote: 31 }
```

| sim frame | chain | count | limit |
| --- | --- | --- | --- |
| 11582 | 31 | 31 | 30 |
| … twenty consecutive 1 s samples, ~700 sim frames … | | | |
| 12260 | **31** | 31 | 30 |

Flat. No growth, no drift, 0 traps. (31 rather than 30 because one spawn
slipped through in the 16 ms before the arm fired; the point is that the chain
stops dead wherever the count is put.)

A third confirmation from the other direction: writing a count **above** the
limit (60 on a 60-bloke chain) drained the park instead — 60 → 50 → 39 over
~4,000 sim frames, chain and count falling together, because no spawn can
happen while `count >= limit` and every departure decrements both. The count,
and nothing else, is the governor.

### The tutorial — where the same bug jams the park permanently

Tutorial lesson 1 (`max_blokes` **5**, `visitorLimit` **3**):

| sim frame | chain | count | limit |
| --- | --- | --- | --- |
| before the save | 3 | 3 | 3 |
| save slot 3 | `1save3.sav` 154,375 B | | |
| *(reload)* 754 | **3** | **0** | 3 |
| 790 | 4 | 1 | 3 |
| 826 | **5** | **2** | 3 |
| 1415 (600 frames later) | **5** | **2** | 3 |

It stops at 5, not 6. `NewBloke` (blokeai.c:363) scans `i < g_map->max_blokes`
for a slot with `flags62` bit 0 clear; all five are taken, so `MakeBloke`
returns 0 for every spawn from then on. **The park is left permanently below
its own visitor limit with a full pool, and no visitor can ever enter lesson 1
again in that session** — the tutorial's symptom is a freeze where free play's
is a crowd. Anyone playing a tutorial level from a save is playing it with the
visitor simulation dead.

---

## 3. The fix, and why it is NOT applied

One word, in a `LEGOLAND_PORTABLE` arm at the end of savegame.c's BLK4
restore loop — `count` has already been consumed by the loop, so it needs its
own tally:

```c
#ifdef LEGOLAND_PORTABLE
        /* P1-7 / PORT-M18: the save format has no field for g_visitor_count,
         * and BeginParkLoad's ClearBlokeList zeroed it, so SpawnVisitor tops
         * the restored park up from zero and adds g_visitor_limit ghosts. */
        ll_restored++;
#endif
    }
#ifdef LEGOLAND_PORTABLE
    g_visitor_count = ll_restored;
#endif
```

It is **not applied**, and PORT-M18 recommends it stays unapplied:

* The port exists to reproduce `LEGOLAND.EXE`. This is not a translation
  artefact that the x86 build gets right and wasm gets wrong — the two builds
  behave identically, because the store is missing from the original source.
  A `LEGOLAND_PORTABLE` arm here would be the first place the port knowingly
  plays *differently* from the binary it is matched against, and every later
  A/B that compares a save between the two builds would have to know about it.
* It changes what a `.sav` round trip produces, which is exactly the thing
  `tools/oracle_savechunks.py` and any future save-compat oracle would pin.
* Nothing is blocked by it. It costs a play lane one sentence in its notes.

If the project ever wants a "quirk fixes" policy, this is the cheapest possible
first entry and the patch above is the whole of it. Until then it belongs in
`docs/` and not in `LEGOLAND/`.

---

## 4. What this lane did change

Two additions to the debug surface, both zero-behaviour, so the next lane can
read this instead of scanning the heap for it:

* `portable/src/browser/main.c` — `g_bloke_base` (`0x0066b57c`) added to
  `LL_DBG_TABLE` as index 84. The chain is a POOL, not a heap list, and slot
  indices are the only way to see that a restored chain holds no duplicates;
  this lane had to derive the base from `min(pointer)`.
* `portable/src/browser/index.html` — `llPark()` gains **`maxBlokes`**
  (`g_map+0x1a`, the ceiling `NewBloke` stops at) and **`ghosts`**
  (`people - numVisitors`). `ghosts` is 0 in a park that has never been loaded
  and stays 0 for that whole session; anything else means a load happened and
  says how many uncounted visitors it left behind. P1 had `people` and
  `numVisitors` side by side in every reading it took and the gap was still
  read as "the chain is wrong".

Nothing in `LEGOLAND/*.c` or `portable/src/hostwin/` was touched.

---

## 5. Gates

| gate | result |
| --- | --- |
| `progress.py --check` | **3281 exact / 42 WIP** — unchanged |
| `relocs.py` on the ten files on the path (`savegame`, `simcore2`, `pathobj2`, `rides`, `gameframe`, `blokeai`, `goalstate`, `blitmisc`, `uimisc2`, `eventtick2`) | **zero MISMATCH** — every address this lane's proof rests on resolves to the object its comment names |
| `audit.py` | not run on game files: **none changed** |
| `extern_sweep.py` | clean |
| `bvstruct_sweep.py` | clean |
| `port_m10_bvstruct_sweep.py` | clean |
| `addr_sweep.py` | clean (baseline unchanged) |
| native clean build + ctest | **19/19** |
| wasm clean build + ctest | **26/26** |
| live run | 35.7 fps, `dead` null, **0 traps** across nine park loads and three saves |

`tools/verify.py` was not run (integrator's gate).

---

## 6. Owed to other lanes

* **Every play lane.** `llPark().people` after a load is *not* the visitor
  population; `numVisitors` is. Read `ghosts` first: non-zero means this park
  came from a save and is carrying that many uncounted extras. Do not file
  "the chain is corrupt".
* **Whoever owns P1-6 (visitors not drawn).** Take that measurement on a park
  that was *built*, not *loaded*: a loaded park has up to twice the blokes and
  half of them were never `InitBlokeAI`'d, which is a second variable in a
  render question that already has enough. B12's "30 blokes, 23 print nodes"
  reading is from a built park and is the one to reproduce.
* **Any tutorial lane.** Never resume a tutorial lesson from a save to measure
  visitor behaviour: the pool (`max_blokes` 5 in lesson 1) is exhausted by the
  load and the spawner is dead for the rest of the session.
* **A future save-compat oracle.** `g_visitor_count` and
  `g_visitor_spawn_clock` are the two live simulation words the `.sav` format
  has no field for. Any "load then save then diff" oracle must expect them to
  differ, or must run the diff on the first frame after the load.
