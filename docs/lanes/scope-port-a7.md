# Scope PORT-A7 — a pointer FIELD is a pointer

> **PORT-A7 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-A7).** Branch
> `scope/PORT-A7` from `feat/decomp-completion-next-steps-24a0d6` @ `c337b295`
> (the PORT-B8 merge). Files owned: `portable/tools/gen_link.py`, `cdecl.py`,
> `linkreport.py`, `name_trap.py`, `portable/cmake/headless.cmake`,
> `portable/src/headless/**`, `portable/src/hostwin/kernel32.c`, `msvcrt.c`,
> plus an append-only block in `portable/cmake/tests.cmake`.
> `LEGOLAND/*.c` untouched, so the VC6 gate has nothing to check for this lane.

---

## 1. Headline

**Both doors into the park are open.** The click PORT-B8 could only measure as a
wedge — `Accept_On_Report` at (577, 419) on the tutorial screen — now returns in
**827 ms**, the tab keeps running at 34 fps with 0 traps, the heartbeat keeps
beating, and the **tick draws beside Lesson 1** (§4). The other door,
`ProgressTutorialInput` at (577, 300), answers in 824 ms and goes back to the
main menu. Neither is `KillLowMarkerSprites` any more.

**The root cause is not the resolver PORT-A2 wrote, and it is not PORT-A6's
widened extents.** It is one line older than both: pointer-ness was read off the
TOP-LEVEL declaration's `*` depth, and

```c
extern LevelMarker g_low_markers[5];      /* 0x004beca0  screens3.c:277 */
```

has pointer depth 0. Its ten `const char*` sprite-name words were never
classified as POINTER SLOTS, so A2's interior-of-block rule — which handles
exactly this case and would have emitted `(char*)g_low_markers + 0xa0` — was
never asked about them. A2 recorded the limit in its own notes ("**Known limit:
pointer members of struct arrays are still raw**", with `g_fp_table` and
`g_build_followups` as the examples); PORT-A6 then taught `cdecl.py` to lay
these structs out, but only ever asked it for their SIZE.

So ask it for their pointer fields. `cdecl.pointer_offsets(ty)` walks a laid-out
type — nested aggregates, every element of every array — and the rule becomes
the one the declaration always implied:

> **A word is a pointer slot when some declaration's type puts a pointer FIELD
> at that address.**

| | before | after |
| --- | --- | --- |
| words re-pointed at a symbol address | 299 | **299** |
| pointer words re-pointed INTO a block | 441 | **693** (+252) |
| symbols re-pointed into | 283 | **308** |
| synthesised `ll_gap_` blocks | 0 | **0** |
| declared pointer words (the new census) | — | **7348** |
| **raw pointer words (the gate)** | (not measured) | **0** |
| pointer words left raw WITH A REASON | 12 | **12** (the same 12, now attributed) |
| PORT-B8's value census (`§9` script) | 1211 in 177 objects | **959 in 160** (all string TEXT, §5) |
| every other cell of globals.c | — | **identical**, 0 of 21,368 differ |
| ctest | native 10, wasm 16 | native 11, wasm **17** (`pointer_words`) |

---

## 2. Why A2's interior-of-block rule missed these

The brief asked which of three it was — the order of extent computation against
the address→symbol map, the map being built before the widening, or targets that
lost a symbol when the widening swallowed them. **It is none of the three**, and
the evidence is in the generated file itself.

`gen_link.py` emits

```c
/* 0x004beca0 .data 600 bytes */
__attribute__((aligned(16))) unsigned int g_low_markers[150] = {
```

with **no `interior:` clause**. A block's size is
`max(next_addr[addr], host_end[addr]) - addr`; `host_end` is only set when the
interior pass absorbs something, and nothing was absorbed here (the declared
extent is `5 * sizeof(LevelMarker)` = 140 bytes, and the string at `+0xa0` = 160
is past it, in the tail). `next_addr` is computed from `known` at the top of
`main()`, **before** the interior pass runs, so absorption can only ever make a
block bigger, never move its edges. This block was 600 bytes — the gap to the
next named address — under PORT-A2's generator too, and `"Appraisal_Yes.lls"`
was interior to it then as well. PORT-A6 changed nothing about it.

What A6 *did* change is the reason the defect was worth catching now rather than
later: with 27 more objects merged, several of them pointer tables reached
through an interior name, `ptr_slot_count` became `ptr_slot_words` (a SET of
word indices) — the right shape for this fix to slot into.

The resolver was never the problem. `resolve()` takes `(word,
is_pointer_slot)`; with `is_pointer_slot` false it returns `None` for anything
that is not *exactly* a symbol's address, and 0x004bed40 is not a symbol. Every
one of the 252 words this lane fixes is resolved by **A2's rule 2, unchanged** —
`w` lands inside a rebuilt block, so it becomes `(char*)&sym + off`. The rule
was right; nothing was asking it.

### 2a. The one thing that had to be generalised: key by ADDRESS

A per-block offset rule would still have missed `g_level_markers`. Two files
declare it, and they disagree about where it starts:

```c
/* screens3.c:276  */  extern LevelMarker g_level_markers[10];   /* 0x004beb80 */
/* bigscreens.c:103 */ extern LevelMarker g_level_markers[10];   /* 0x004beb88 .. 0x004beca0 */
```

`bigscreens.c` frames the record **eight bytes in, with its own fields rotated to
match** (`{str_id, x, y, lit, dim, lit_name, dim_name}` against screens3.c's
`{lit_name, dim_name, str_id, x, y, lit, dim}`) — both descriptions are correct
about the same memory, and `linkreport`'s `externs` keeps ONE address per name,
so the emitted block lands at `0x004beb88`. In that block every pointer is at
`+0x14`/`+0x18` of a 0x1c-byte record, not `+0x00`/`+0x04`; and record 0's two
name pointers are not in the block at all — they are the last two words of
**`g_mode_wplus`**'s block at `0x004beb70`, which is why they show up in the
fix's per-object table below and did not show up in B8's.

Keying the pointer-word set by absolute VA handles all of that without knowing
any of it.

### 2b. The value's one vote is a veto, never a claim

PORT-A2's rule stands: the value of a word may never decide that a word IS a
pointer (a value-only rule re-points 1,650 words of which 1,209 are string
text). But a value can *refuse* a declaration: `0x00000019` is not an address in
any layout. Corroboration runs **per array element**, so one odd entry costs its
own record rather than the whole table, and every rejection is a row of
`gen/pointers.md`.

30 words in 19 declarations are rejected, and every one of them is a real
finding rather than a false alarm (§6): 12 are a `void*` declaration over a
number, 6 are `MeshDesc`'s pointer fields holding `0x3f800000` (= 1.0f), 4 are
`g_menu_help`, and 4 are the **tenth** `LevelMarker` at `0x004beb88 + 9*0x1c`,
whose `lit` word is `0xffffffff` because the table is nine markers long and the
tenth record's space is `g_progress_click_level`.

---

## 3. The proof that the image is still the image

`port-a7-globals-equiv.py` (scratch, §9) parses both generated `globals.c` into
an **address → value** map, where the value of a re-pointed word is the TARGET
ADDRESS its `symbol + offset` resolves to — the symbol name and the offset both
move when a word starts being re-pointed, the target does not.

```
base/globals.c: 21368 cells, 740 re-pointed words
fix/globals.c:  21368 cells, 992 re-pointed words
objects: 2042 -> 2042
cells only in the old file: 0, only in the new: 0
cells that changed CLASS (raw word <-> re-pointed): 252
cells whose VALUE differs: 0
```

**0 of 21,368 cells differ.** Every non-pointer byte is identical, 252 words
changed from a raw x86 address to an expression that evaluates to the same
address in the rebuilt layout, and no object changed size (3,705,868 bytes of
image, 2042 globals, 44 merged objects, 307 interior aliases — all unchanged).

### The 252, by object

| object | words | what they are |
| --- | --- | --- |
| `g_fp_table` | 130 | the ride/facility table's name column — A2 predicted this one needed "a real struct layout, i.e. the matching lanes' `FPTableEntry`". It exists now. |
| `g_build_followups` | 46 | `"CASTLE OBJ"`, `"SQUARE_TRACK"`, `"SQUARE_TRACK_HEIGHT"` — A2's column heuristic estimated 49 and was never implemented |
| `g_level_markers` | 18 | the levels 6..15 progress screen, records 1..9 (§2a) |
| `g_low_markers` | 10 | **B2**: `Appraisal_Yes.lls` / `Appraisal_No.lls` x 5 |
| `g_class_groups` | 9 | |
| `g_ds_fx`, `g_fountain_fx`, `g_dino_fx` | 5 each | the ride FX tables' `.wav` names |
| `g_copters_fx`, `g_ww_fx`, `g_rest2_fx` | 3 each | |
| `g_bs_fx`, `g_box_template`, `g_carousel_fx`, `g_mode_wplus` | 2 each | `g_mode_wplus` is `g_level_markers` record 0 (§2a) |
| `g_goldrush_polyline`, `g_lf_animset_a`, `g_lf_animset_b`, `g_lfc1_anim`, `g_lfc3_anim`, `g_lfhu_anim`, `g_power_station_fx` | 1 each | |

22 objects. **PORT-B8's B2a is closed with B2**: `g_fp_table` and
`g_build_followups`, the two biggest tables the park reads, were in the same
class and are fixed by the same rule.

---

## 4. The proof in a tab

Served `portable/build-wasm` on `:8802`, page driven through its own hooks
(PORT-B7's `llMove/llClick/llType`), `?args=-nointro+WINDEBUG`, replaying
PORT-B8 §3 exactly. Every row reproduced twice, on two fresh loads.

| # | screen | input | frame hash | state |
| --- | --- | --- | --- | --- |
| 1 | PLAYER DETAILS | load | `0x9d9b7c50` | **identical to B8**, 96.5% non-black, 32.4 fps, 0 traps |
| 2 | NEW PROFILE popup | `llClick(260,188)` | `0x6b1a9df1` | |
| 3 | the MAIN MENU | `llType('adam')`, `llClick(505,345)` | `0x954b0578` / `0x6c445d5c` | the gates and six bubbles, screenshot |
| 4 | **"Select tutorial level"** | `llClick(252,362)` | **`0x23934744`** | 98.3% non-black. **THE TICK IS THERE** beside Lesson 1 |
| 5 | **past the door** | `llClick(577,419)` | **`0x561128cd`** | **returns in 827 ms**, 34.4 fps, 0 traps, heartbeat still beating |
| 5b | back at the main menu | `llClick(577,300)` from row 4 | `0x2529ff6c` | the OTHER door, 824 ms |

**Row 4 is the visible part of the fix.** B8 recorded this screen at
`0x83065c2b` / `0x1e3888c4` with the note "**No tick beside Lesson 1**"; the
hash is different now for exactly that reason — `LoadSprite("Appraisal_Yes.lls")`
succeeds, so `InitTutorialScreen`'s first icon has a sprite and the green tick
draws.

**Row 5 is NOT the park map.** What draws is the **in-game screen**: the money
bar (`10000`) and the rating bar across the top, and the full park toolbar along
the bottom — LEGOLAND, build, query, eraser, map, options, the notepad and the
theme buttons. The centre, where the isometric park belongs, still holds the
tutorial notepad backdrop; nothing repaints it, and the heartbeat ring settles
into a two-`Blt`, two-`Lock`/`Unlock` frame (the front end's was ~25 blits), so
very little is being drawn. **The game is in the park and the park is not being
rendered.** No trap, no console line, 34 fps: it is the next blocker, and it is
not in the closure (see §7 B7-1).

`?beat=3000` was on for the first run: the ring keeps printing ordinary frames
straight through the click, which is the mechanical statement that
`KillLowMarkerSprites` is not being entered any more.

---

## 5. The census, and why `raw pointer words` is the DECLARATION's count

PORT-B8 asked for its §9 census to become part of the manifest. It is —
`gen/pointers.md` and five manifest lines — but it is counted off the
**declarations**, not off the values, because B8's script asks "does this word's
value land inside an emitted object?" and that over-counts by construction:

```
$ python3 port-a7-rawptr.py <base>/globals.c      # B8's script, verbatim
raw-VA words that land inside an emitted object: 1211, in 177 objects
$ python3 port-a7-rawptr.py <fix>/globals.c
raw-VA words that land inside an emitted object: 959, in 160 objects
  GUID_NULL           264 words     kThemeSame              102
  g_level_db_sections  91           g_power_table            63
  g_near_offsets       63           g_lowlevel_ai            59
```

The 959 residue is **string TEXT**, which is the false positive A2 measured at
1,209 of 1,650: `"lls\0"` reads as `0x00736c6c`, which is between `0x00401000`
and `0x00900000`, and those objects are the ones whose gap tile swallowed
kilobytes of `.rdata` literals (`GUID_NULL` is 16 bytes of GUID and several
kilobytes of strings). Every one of the 1211 that a declaration calls a pointer
is now re-pointed; the rest are not pointers and must not be touched.

That is a measurement, not a hand-wave. `port-a7-residue.py` asks two questions
of each residue word that do not use the fix at all — is its own address past
the DECLARED extent of the object it sits in (so no declaration describes that
storage), and are its four bytes printable ASCII:

```
residue words (the VALUE census): 959
  past the declared extent of their object (swallowed literals): 958
  four printable-ASCII bytes:                                    694
  NEITHER (worth a human):                                         1
    0x004b9560 = 0x00739c00 in g_path_3x3_masks (0x004b9558, declared 36)
```

**One** word in the whole residue sits inside a declared extent, and it is
`extern const unsigned int g_path_3x3_masks[9]` (workorder4.c:50), a bitmask
table used as `bits & g_path_3x3_masks[i]`. 0x00739c00 is a 3x3 path mask that
happens to read as an address. The declaration says it is not a pointer and the
generator leaves it alone, which is the whole argument for the declaration rule
in one line.

`gen/pointers.md` therefore reports, for the browser closure:

```
- re-pointing (--ilp32): on
- declared pointer words: 7348
- of those, in a block emitted as WORDS (so offered to the resolver): 7348
- in a block emitted as BYTES (NOT re-pointed): 0
- in no emitted block: 0
- re-pointed at a symbol address: 299
- re-pointed INTO a block: 693
- left raw: 12 (12 the value is not an address)
- **raw pointer words: 0**   <- the gate
```

`raw pointer words` counts a declared pointer word, inside the image, that
nothing re-pointed and nothing can explain. **It must be 0.** Words left raw on
purpose are listed with their reason instead:

* **into `.text`** — a function address. On wasm a function "pointer" is a table
  index, so an offset into the middle of a body is meaningless; an EXACT
  function address is already re-pointed by the symbol path. (0 of these today.)
* **the value is not an address** — a declaration to check, §6.

In the 64-bit build the whole census is vacuous and says so: re-pointing is off
there by construction, so the gate cannot fail for the wrong reason.

---

## 6. Findings for a matching lane: 12 words a file calls a pointer and 30 more

Both lists are mechanical output (`gen/pointers.md`), not opinion. **None of
them is fixed here** — they are all `LEGOLAND/*.c` declarations.

| address | word holds | declared | by |
| --- | --- | --- | --- |
| `0x004b41c8`/`4208`/`4240`/`4270`/`4298` | 6, 7, 6, 5, 4 | `void* g_copters_poly0..4` | mechrides.c:842-846 |
| `0x004b560c` | 2 | `void* g_span_vtx` | coaster11.c:150 |
| `0x004b85c4` | `0xffffffff` | `void* g_vwin32` | unref5.c:85 |
| `0x004bb18c`..`98` | 0x64, 0x8c, 0xc8, 0x2710 | `void* g_menu_help` (4 words) | movie.c:211 |
| `0x004bf774` | 1 | `void* g_music_sys` | audio2.c:176 + 8 more files |
| `0x004b5f80`, `0x004b5f84`, ... | `0x3f800000` (= 1.0f) | `MeshDesc g_track_mesh` pointer fields | schoolcar3.c:287 |
| `0x004beb88 + 9*0x1c` | `0xffffffff` | `LevelMarker g_level_markers[10]` element 9 | bigscreens.c:103 |

The first five rows are the same shape: a **count or a flag declared `void*`**.
They are harmless on x86 and harmless here (the word is left exactly as the
image has it), but they are wrong about the image and they will mislead the next
reader. `g_track_mesh` is a type worth a second look — a struct whose declared
pointer fields hold floats is either the wrong struct or the wrong address.
`g_level_markers[10]` is simply nine markers plus `g_progress_click_level`; the
bound could be 9.

---

## 7. Blockers, with owners

| # | what | owner | state |
| --- | --- | --- | --- |
| **B2** | the park's two doors wedge in `KillLowMarkerSprites` | PORT-A7 | **CLOSED**, §1-§4 |
| **B2a** | the same class in `g_fp_table` (135), `g_build_followups`, `g_lowlevel_ai`, `g_power_table`, `g_level_db_sections` | PORT-A7 | **CLOSED for every one that is a declared pointer**; the residue is string text (§5) |
| **A7-1** | **the park does not RENDER.** Past the door the in-game HUD, money bar and toolbar all draw and the loop runs at 34 fps with no trap, but the map area keeps the previous screen's backdrop and the frame makes only 2 blits | a PORT-B lane | new, §4 row 5. Nothing in the closure: `raw pointer words` is 0 and no trap or console line is produced |
| **A7-2** | **clicking a park toolbar icon kills the module**: `RuntimeError: function signature mismatch` at `wasm-function[1628]:0xf4ac8` <- `[1383]:0xd6275` <- `[1137]:0xad0fb` <- `[564]:0x6142a` <- `[40]:0x8e44`. `name_trap.py --at 0xf4ac8 --wasm portable/build-wasm/legoland.wasm` says the call site is `call_indirect (i32) -> void` and the slot was written at RUNTIME | a PORT-M lane (PORT-M3's class) | new. **Recipe**: build `legoland_browser_named` (browser.cmake already has it, `-g2`) and re-read the stack — the callers are numbers only because `legoland_browser` carries no name section |
| B3 | `while (KillSprite(x) == 0) ;` is still unguarded in four places | a matching lane | unchanged; B2 no longer reaches it, but any sprite the archives cannot serve still hangs rather than degrading |
| B4 | the main menu's Free-play bubble does not take a click | unknown | unchanged |
| **B5** | profiles do not survive a page reload (MEMFS) | PORT-B | **the exact patch is in §8, written and not applied** (the brief keeps `portable/src/browser/**` and `browser.cmake` out of this lane) |
| B6 | a wedged tab cannot be navigated away | tooling | **no longer reachable on this path** — every click in §4 returns, and `navigate` worked normally all lane |

---

## 8. B5, written out: profiles across a reload (IDBFS)

Not applied. `portable/src/browser/main.c` and `portable/cmake/browser.cmake`
belong to PORT-B; this is the patch, against the files as they stand at
`c337b295`.

**1. `portable/cmake/browser.cmake`**, in `target_link_options(legoland_browser
PRIVATE ...)` (line 198) and in `legoland_browser_named`'s (line 241), add
`-lidbfs.js`. It is the JS library only; no `-sASYNCIFY` change is needed
because the target already has it.

**2. `portable/src/browser/main.c`**, replacing the `mkdir` block in `main()`
(the comment above it already points at this job):

```c
/* PORT-A7/B5: profiles across a reload. MEMFS is rebuilt from the .data
 * package on every load, so a profile written this session is gone on the
 * next one. IDBFS is MEMFS plus an IndexedDB image of it that `syncfs`
 * moves in each direction; mounting it over the profile directory alone
 * keeps the rest of /gamedata read-only and cheap.
 *
 * The read has to finish BEFORE WinMain, because ScanForProfiles runs in
 * the first front-end frame. ASYNCIFY makes that expressible: spin on a
 * flag the callback clears, yielding through emscripten_sleep, which is
 * the same yield every blocking construct in this port already uses. */
static volatile int g_syncfs_pending;          /* file scope */

EM_JS(void, ll_mount_profiles, (int* flag), {
    try {
        FS.mkdirTree('/gamedata/profiles');
        FS.mount(IDBFS, {}, '/gamedata/profiles');
    } catch (e) {
        console.warn('[browser] IDBFS mount failed:', e);
        HEAP32[flag >> 2] = 0;                 /* carry on with MEMFS */
        return;
    }
    FS.syncfs(true, function (err) {           /* IndexedDB -> MEMFS */
        if (err) console.warn('[browser] profile restore failed:', err);
        HEAP32[flag >> 2] = 0;
    });
});

EM_JS(void, ll_flush_profiles, (void), {
    FS.syncfs(false, function (err) {          /* MEMFS -> IndexedDB */
        if (err) console.warn('[browser] profile save failed:', err);
    });
});

    /* ... in main(), where the mkdir is now: */
    g_syncfs_pending = 1;
    ll_mount_profiles((int*)&g_syncfs_pending);
    while (g_syncfs_pending)
        emscripten_sleep(10);
```

**3. The write-back.** `FS.syncfs(false, ...)` has to run after the game writes
`Profile1.txt`. The honest hook is `SaveProfileToDisk` finishing, which the host
cannot see; the cheap one that PORT-B8 measured is the Accept click, so flush on
a **timer** started in `main()` and keep it dumb:

```c
    emscripten_set_interval(ll_flush_profiles_cb, 5000, NULL);   /* 5 s */
```

A flush with nothing dirty is an IndexedDB transaction over a 272-byte file;
five seconds is far below the cost of a frame's 22,000 `timeGetTime` calls.
**Do not** flush per frame.

**4. What to measure.** B8's §5 recipe, plus a reload:
`FS.readdir('/gamedata/profiles')` before (`[]`), the walk, after
(`['Profile1.txt']`, 272 bytes), **reload the page**, then PLAYER DETAILS should
show `adam` in slot 1 without the walk. The `"cannot open output file"` lines
drop from 7 to 6 on the second load, which is `ScanForProfiles` finding the one
that is there.

**Cost**: the 10-15 minute `legoland_browser` link on this machine, which is why
it is written rather than done — this lane's budget went to the park's doors.

---

## 9. Gates, and reproducing

| gate | result |
| --- | --- |
| native clean build + `legoland_linkcheck` | builds, runs |
| native `ctest` | **11/11** (10 + `pointer_words`) |
| wasm clean build, all seven targets | builds; `legoland.wasm` 1,336,836 bytes (the ASYNCIFY-instrumented size, not B8's 991 KB failure) |
| wasm `ctest` | **17/17** (16 + `pointer_words`) |
| `cdecl.py --selftest` | 0 failures (27 checks + 12 new) |
| `LEGOLAND/*.c` touched | **none** — no audit/relocs run needed |
| generated host traps | 0, unchanged |
| page: front end, traps | 0 traps, 32-34 fps |

```bash
PY=$HOME/.venvs/legoland/bin/python
cmake -S portable -B portable/build -G Ninja -DPython3_EXECUTABLE=$PY
ninja -C portable/build && (cd portable/build && ctest)

emcmake cmake -S portable -B portable/build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DLL_ILP32=ON -DPython3_EXECUTABLE=$PY
ninja -C portable/build-wasm && ninja -C portable/build-wasm legoland_browser
(cd portable/build-wasm && ctest)
(cd portable/build-wasm && python3 -m http.server 8802)
# http://localhost:8802/legoland.html?args=-nointro+WINDEBUG   then §4's script
```

The gate on its own, against any build directory:

```bash
$PY portable/tools/gen_link.py portable/build-wasm --check-pointers
#   gen/manifest.md: raw pointer words: 0
#   gen-browser/manifest.md: raw pointer words: 0
#   pointer gate: 0 failure(s) in 2 manifest(s)
```

**Iterating on the generator without a link.** `gen_link.py` takes about four
seconds; a `ninja` round trip takes minutes. Once `legoland_core` and
`legoland_hostwin` are built once, the closure can be regenerated into a scratch
directory as many times as wanted:

```bash
$PY portable/tools/gen_link.py portable/build-wasm/CMakeFiles/legoland_core.dir \
    --out /tmp/try --exe original/legoland.exe --ilp32
$PY portable/src/browser/closure_filter.py --gen /tmp/try \
    --objs portable/build-wasm/CMakeFiles/legoland_hostwin.dir
```

Scratch scripts (not committed, `port-a7-` prefixed): `port-a7-rawptr.py`
(PORT-B8 §9 verbatim, the value census), `port-a7-globals-equiv.py` (§3, the
address → value comparison — **run this on any change to the re-pointing
rules**), `port-a7-residue.py` (§5, what is left of the value census),
`port-a7-gen.sh`, `port-a7-build-native.sh`, `port-a7-build-wasm.sh`.

---

## 10. For the integrator

* `portable/cmake/tests.cmake` is **appended to only** (the `pointer_words`
  block at the end).
* **No new build dependency is needed.** PORT-A2's gap in
  `portable/CMakeLists.txt` is closed: its `gen` command already DEPENDS on
  `gen_link.py`, `linkreport.py`, `cdecl.py` and `win32_imports.txt`, and
  `browser.cmake`'s `gen-browser` on the same three plus `closure_filter.py`.
  Both therefore re-run on this lane's `cdecl.py` change, which is what the
  clean rebuilds confirmed.
* `gen/pointers.md` is a new generated artifact next to `manifest.md` and
  `extents.md`. Nothing else reads it; the ctest reads `manifest.md`.
* Nothing in this lane touches `LEGOLAND/*.c`, `portable/src/browser/**`,
  PORT-B's shim files or `docs/HANDOFF.md`.
