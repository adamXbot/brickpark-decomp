# Scope PORT-A9 — the gates for the two classes no gate could see

> **PORT-A9 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-A9)**

Branch `scope/PORT-A9`, from `feat/decomp-completion-next-steps-24a0d6` at the
PORT-B11 merge (`68fab1d3`). Lane files: `portable/tools/gen_link.py`,
`portable/tools/bvstruct_sweep.py`, `portable/cmake/headless.cmake`,
`portable/src/headless/main.c`, `portable/tests/*_baseline.txt` /
`*_accepted.txt`, `tools/port_m10_bvstruct_sweep.py` (now an alias).
Nothing under `LEGOLAND/`, `portable/src/browser/`, PORT-B's shim files or
`portable/CMakeLists.txt` was touched; `verify.py`, `audit.py` and `match.py`
were not run.

PORT-B11 §3 closed with a request and PORT-M10 §1f with another, and they are the
same complaint: **a gate that reports zero is only as good as the set it walks.**

* `gen/pointers.md` says `raw pointer words: 0` and means it — over the words the
  DECLARATION scan visited. `extern FXEntry g_game_fx[];` has no bound, so
  `cdecl.py` computes no extent, so `gen_link.py` never visits the object, so its
  three raw x86 string addresses are not raw *pointer* words. 23 sound effects
  never loaded under a gate reporting zero.
* `wasm-ld`, `linkreport.py` and `test_callback_types.c` are all blind to a
  by-value struct of 4 bytes or fewer spelled as a scalar in the other TU: the
  arity matches, so nothing warns and the callee reads a shadow-stack pointer.
  PORT-M10 found it by hand and left the sweep in `tools/`, outside every gate.

Both are now measured from a clean build, and measuring them found more of each:
**13 more by-value-struct sites** and **63 + 9 more raw pointer words re-pointed**
(`g_power_table`, the table of element names B11 suspected).

Gates from clean dirs, after every commit below: **wasm ctest 24/24** (20 before
this lane), **native 17/17** (14 before), `legoland_linkcheck` links.

---

## 1. `gen/rawwords.md` — the census that asks the image, not the sources

### 1a. Why `raw pointer words: 0` was true and useless

The pointer scan's first statement is the whole story:

```python
for obj in sorted(src_objs, ...):
    ty = obj.full_type
    if ty is None or not cdecl.has_pointer(ty):
        continue
```

`full_type` is `None` when the object has no computable size (cdecl.py:264 —
`count is None`), which is every `extern T x[];` with no bound. The object is
skipped in silence, its words are never offered to the resolver, and because the
census counts *declared* pointer words, they cannot appear in it. B11 §3 named
the consequence: `sprintf(".\\sfx\\%s", 4955028)` and a failure branch that is a
`DBPrintf` nothing prints.

### 1b. The rule, and the two vetoes it needs

Every 4-byte word at a 4-aligned address of every emitted object — whatever its
declaration says, and whether or not it has one — whose value lands in
`DATA_LO..DATA_HI` (0x004ab000..0x00836000, the same range the re-pointer uses).
Nothing is re-pointed on that evidence; PORT-A2 measured what value-chosen
re-pointing does and the rule stands. These are rows to READ.

The value gets A2's own false-positive analysis as a veto (`textish_word`, now a
module-level function in gen_link.py so both users share it):

| shape | example | why |
| --- | --- | --- |
| three printable bytes, zero high byte | `"tan\0"` = 0x006e6174 | A2: 1,209 of 1,650 value-chosen words are string TEXT |
| `'w' 0 'm' 0` | 0x006d0077 | UTF-16; `kThemeSame` is 98 words of wide string table and 39 more sit in `GUID_NULL`'s pool |

and one override, which took two measurements to get right: a value that points
at the **first byte of a C string** is reported even if its own bytes look like
text. Pointing *into* a string is no evidence at all — `.rdata` is mostly string
pool, so an arbitrary address in it usually lands in one, and `"ACK\0"` (the tail
of "LOG FLUME TRACK") reads as 0x004b4341, five bytes into "mcop_b2s.lls". That
first-byte test is what separates a report worth reading from half of `.rdata`,
and it is also what keeps `g_entrance_fx`'s only word — 0x004b6674,
"turnstyles.wav", itself three printable bytes — from being vetoed away.

Measured on the way: with the override conditioned on the target being a string
*anywhere*, two false rows appeared (`g_lf_norect_msg`, `g_s_elsquirt_lls` — both
the "ACK\0" shape); with no override at all, `g_entrance_fx`'s real defect
disappeared. The first-byte rule gives neither.

**The blind spot, stated plainly:** a genuine pointer to something that is not a
string, whose value happens to be three printable bytes, is vetoed. Per object
the signal usually survives (a table's other entries are not text-shaped), and
the real closure for that class is the declaration side — §2.

### 1c. The numbers

| | wasm32 closure (`--ilp32`) |
| --- | --- |
| words in the image range, text vetoes applied | 989 |
| visited by the declaration scan (re-pointed, or a row of `pointers.md`) | 882 |
| **objects with words it never visited** | **27** |
| words in them | **107** |
| pointing at a C string | 55 |
| at a symbol's own address | 3 |
| at unidentified data | 49 |
| vetoed as inline text | 545 |

The native 64-bit closure reports 0: re-pointing is off there by construction, so
the census is vacuous, exactly like `pointer_words`.

B11 §3 measured "122 raw words in 18 objects" with a hand script. This census
found **exactly the same 18 objects** before §2's rule ran (the string-pointing
rows); the 11-word difference is B11 counting the three-character `_matherr`
names, which this census's stricter string test files as data. After §2's rule
the string-pointing set is down to 55 words in 18 objects, 23 of them sample
names.

What the 107 are:

| group | words | note |
| --- | --- | --- |
| FX sample names | 23 | B11 §3's blocker. `g_place_sample` 7, `g_snd_close` 3, `g_game_fx` 1, nine more tables with 1–2 each |
| `g_near_offsets` | 59 | the CRT's own `_matherr` name table and float-format pool, swallowed by an unbounded `NearOffset[]` (workorder2.c:298). B11 called it probably dead; a bound stops the object absorbing `.rdata` either way |
| model/anim pointers | 10 | `g_anim_tower_b` 5 (a real pointer table declared `int`), `g_spacetower_fx` 2, `g_bloke_anim_ref` 1 (points at `g_anim_tower_b`), `g_seat_name_man` 2 |
| other strings | 2 | `g_gfx_dirs`'s `".\graphics\textures\"` at +0x1c of a `char` declaration; `g_anim_tower_a` |
| false positives, with reasons | 13 | `g_cursor_col_a/c/d` (BGR colours; 0x0080ff80 *equals* `g_front`'s address by coincidence), `g_two` (a float pool), `g_path_3x3_masks`, `GUID_NULL` (UTF-16 at an odd byte phase) |

### 1d. The gate

`gen_link.py <build> --check-rawwords` reads every `rawwords.md` the build wrote
and compares its machine-readable rows with
`portable/tests/rawwords_baseline.txt`:

* a row that is not in the baseline, or whose word count GREW → **FAIL**, with
  the object, the breakdown and a pointer at the `fix` column;
* a row that shrank → passes and prints `SHRUNK ... tighten the baseline in the
  commit that fixed it`.

So PORT-M11's bounds make the file smaller and a regression cannot be merged.
Asset-free (it reads build products only), registered as the ctest `raw_words` in
both builds. The baseline's 27 rows each carry their reason — the declaration
owed, or why the row is a false positive.

---

## 2. An unbounded array of a struct with pointer fields is a row, not a skip

### 2a. The rule

When a declaration has no computable bound but its ELEMENT type is laid out and
has pointer fields, the count comes from the object's own gap-tiled block: the
object is at least the **complete** elements that fit in `size`. Then the normal
per-element machinery runs (`cdecl.pointer_offsets` over an `array_of(elem, n)`).

Three deliberate conservatisms:

1. **Complete elements only.** A tile is an element count plus 4, 8 or 20 bytes
   of alignment or swallowed neighbour far more often than it is a whole
   multiple: 14 of the tree's 23 unbounded struct tables. The remainder is the
   part least likely to belong to the array, and it is never scanned. (The brief
   asked for the whole-multiple form; measured, that closes `g_power_table` and
   misses every 12-byte `FXEntry` table, so the rule scans complete elements and
   reports the remainder. The delta between the two is 7 words, listed in §2c and
   read individually.)
2. **The image vetoes per element, and harder than for a declared bound**: a word
   that cannot be an address *or* that is inline text drops its element. A
   declared bound is a statement from the sources and is believed; a bound the
   generator worked out from a gap is not.
3. **No block moves.** `declared_extent`, the tiling, the merging and every byte
   of `globals.c` are untouched; only the set of words offered to the resolver
   grows.

### 2b. Without the text veto it was a catastrophe

The first run of the rule re-pointed **125** words. 55 of them were the credits
roll: `0x006e6f74` is `"ton\0"` of "Anton", `0x006e6f73` is `"son\0"`, and in a
table of four-character name fragments every one satisfies "could be an address".
They were re-pointed into `g_zbuf_pixels` and `g_zbuffer_storage` — precisely the
1,209-word disaster PORT-A2 documented, arriving through a different door. With
`textish_word` rejecting the element, 72 changes remain and every one is a real
pointer.

### 2c. The address-keyed bytes proof (PORT-A8's shape)

`globals.c` parsed into `{absolute VA -> initialiser as written}` and diffed, so
a change that moves words between objects shows as nothing and only a changed
VALUE shows at all:

| closure | elements | only in old | only in new | values changed |
| --- | --- | --- | --- | --- |
| wasm32 `--ilp32` | 21,397 | 0 | 0 | **72** |
| native 64-bit | 85,495 | 0 | 0 | **0** |

All 72 are `raw image literal -> re-pointed interior`, none the other way, and
all 72 were read:

| object | words | what they are |
| --- | --- | --- |
| `g_power_table` | 63 + 3 | the element NAMES — "Small Power Station", "Crystal Power Station", "T-Rex" — B11's suspected second live defect, closed here |
| `g_game_fx` | 2 | "Flowers.wav", "RabOld\Drill.wav" |
| `g_money_fx` | 2 | "Coin drop for food stands or entrance.wav", "Cash Register.wav" |
| `g_joust_fx` | 1 | "Joust Horses.wav" |
| `g_spacetower_fx` | 2 | anim pointers |
| `g_support_model_a` | 2 | into `g_support_shadow_templates` |

`aliases.c`, `stubs.c` and `host_stubs.c` are unchanged, `raw pointer words` stays
0, and declared pointer words rise 7,545 → 12,335 (120 unbounded declarations
with a pointer field, 107 of them now scanned; the 18 rows that are not are in
`pointers.md` with the reason, `g_game_fx`'s 32-byte tile for a 12-byte element
among them).

### 2d. What it does NOT close

`g_game_fx`'s third sample name is in the 8 bytes past its second element, so it
stays raw. **The bounds are still owed** — they also merge the twelve fragments
of 0x004b9228 into one object and fix `Load_FXList`'s stride, which no generator
rule can do. This lane's rule is a floor under the damage, not the fix.

---

## 3. `--probe-audio`: B11 §3 as a number, under node

`legoland_headless --probe-audio [N]`:

1. mounts the volumes;
2. runs the game's own `InitSoundSampleSystem(0)` (audio4.c:163) — the call that
   creates `g_dsound`;
3. **wraps that object's vtable with a counting copy.** The shim is PORT-B's and
   the game is the matching lanes'; a test-side thunk needs neither to change,
   and it counts the real `CreateSoundBuffer` calls instead of inferring them;
4. makes the park's own FX calls — `Load_FXList(g_game_fx, 0x17)` (mapinit.c:41)
   and `LoadMoneySFX()` (audiomisc.c:184);
5. prints every entry's `.name` word, whether it is a readable string at all, and
   whether `.sample` arrived.

```
legoland_headless:   g_game_fx[ 0] name=0x19f54 Flowers.wav         sample=loaded
legoland_headless:   g_game_fx[ 2] name=0x4b9b6c <not a string -- a RAW image address>  sample=NULL
legoland_headless: g_game_fx: 2/23 loaded, 21 name(s) still raw
legoland_headless: g_money_fx: 2/2 loaded, 0 name(s) still raw
legoland_headless: AUDIO 4 sound buffer(s) created, 122290 byte(s) of PCM
```

**Four**, where B11 measured one `CreateSoundBuffer` call in a whole session — and
all four are words §2's rule recovered, so before this lane this probe would have
printed 0 from these two tables. `probe_audio` is a local (asset-needing) ctest
and `LL_AUDIO_BUFFERS` (4) is a floor: fewer fails, more prints a line asking for
the floor to be raised. When PORT-M11's bounds land it should read 25.

**Finding (free, on the way):** `audiomisc.c:96-100`, the file that DEFINES
`Load_FXList` and writes the slot, puts the sample at `FXEntry +0x08` with a pad
at +0x04; `joust.c:620` names the same shape `{name, sample, flags}` with the
sample at +0x04. Read `g_game_fx` joust.c's way and every entry looks unloaded —
which is how this probe's first run reported 4 buffers created and 0 samples
loaded. Worth a reader for `g_joust_fx` itself: if joust.c's body matches the
original bytes, the two tables genuinely have different layouts and only the
names are confusing.

---

## 4. `bvstruct_sweep.py`: M10's sweep as a gate, and 13 more live sites

`portable/tools/bvstruct_sweep.py`, `--selftest`, ctests `bvstruct_sweep` and
`bvstruct_sweep_selftest`. The root `tools/port_m10_bvstruct_sweep.py` is now a
thin forwarding alias that says where the sweep went and why its answers differ;
**the round checklist line for the integrator** is:

```
$PY portable/tools/bvstruct_sweep.py         # 0 unaccepted silent site(s)
```

(or nothing at all — `ninja -C portable/build-wasm && ctest` runs it.) The alias
can go whenever the checklist stops naming the old path.

### 4a. Two changes that change the answers

**It reads the sources the way the portable build does.** M10's fix shape is

```c
#ifndef LEGOLAND_PORTABLE
extern int AddObjectToBuildList(ObjDef* d, BPos bp);            /* 0x00450b90 */
#else
extern int AddObjectToBuildList(ObjDef* d, unsigned short bp);  /* 0x00450b90 */
#define AddObjectToBuildList(_d,_bp) ...
#endif
```

so a sweep that reads both arms sees an aggregate and a scalar at the same
address and reports the FIX as a hit. M10's sweep reports 8 silent sites today,
three of which are its own repairs. `portable_lines()` is a one-symbol
preprocessor (`#ifdef`/`#ifndef`/`#if [!]defined`/`#else`/`#endif`, nested) that
keeps both arms of every conditional it does not recognise — the safe direction
for a reporter.

**It sizes a typedef tree-wide.** The old sweep looked a type up only in the file
that mentioned it, and `struct_info` gave up on any nested aggregate. So

```c
typedef union BPosW { unsigned short w; BPos b; } BPosW;
```

came out "size unknown" and was filed under *NOISY (wasm-ld already warns)*. It
does not warn: a 2-byte union is as indirect as a 2-byte struct, which M10's own
§1b table says and this lane re-measured with emcc (§4c). That one mis-filing hid
**thirteen** live sites. 261 of the tree's 731 aggregate typedefs have more than
one body (each TU declares the fields it needs), so `agg_info` takes the WIDEST
view — a partial view can never shrink a big record into the silent window — and
the types actually passed by value (`BPos`, `BPosW`, `RideTile`, `ShopTile`,
`CellPos`, `MapPos`, `Pos`) agree in every file that spells them out.

Result: 20 silent sites over 15 addresses, 5 noisy (all function-pointer
typedefs), where M10's sweep said 8 silent over 5 addresses and 35 noisy.

### 4b. The thirteen

All are `union`s of `unsigned short` and a 2-byte `BPos`, and all are game-side,
so this lane measured and filed them. "def AGG" is the dangerous direction: the
BODY dereferences whatever arrives and the callers hand it an integer.

| address | name | direction |
| --- | --- | --- |
| `0x00401f30` | `SchoolCarNextManoeuvreHorn` | def `BPosW` (schoolcar2.c:547) vs decl `unsigned short` (goldrush.c:1641) |
| `0x00402150` | `SchoolCarNextManoeuvre` | def `BPosW` (schoolcar2.c:469) vs `unsigned short` (goldrush.c:1642) |
| `0x00412650` | `Road_FindStartPiece` | def+decl `BPosW`; coaster.c:1515 declares the same address as `GetSchoolRecord(int)` — also a NAME disagreement |
| `0x0041a530` | `BoatingSchool_Remove` | def `BPosW` (ridecb8.c:381) vs loaders.c:183 `unsigned int` — and loaders.c is the `+0x9c` SLOT registration |
| `0x0041b0d0` | `BoatingSchool_AddTake` | def `BPosW` (ridecb6.c:935) vs the same file's `BoatingSchool_AddTake_I(int)` at :440 |
| `0x0041b6f0` | `BsMermaid_Remove` | def `BPosW` (screencb.c:1424) vs loaders.c:196 `unsigned int`, the `+0x9c` slot |
| `0x0041c130` | `BoatingSchoolWater_Remove` | def `BPosW` (ridecb5.c:504) vs loaders.c:175, the `+0x9c` slot |
| `0x0041caa0` | `BoatingSchool_RebuildRoute` | def `BPosW` (ridecb6.c:1116) vs `BoatingSchool_RebuildRouteI(int)` at :450 |
| `0x0042fb00` | `Restaurant2_StartSound` | def `BPosW` (audio5.c:120) vs `unsigned short` (ridecb3.c:1165) |
| `0x0042fb60` | `Restaurant2_StopSound` | def `RideTile` (ridemisc4.c:583) vs `unsigned short` (ridecb3.c:1166) |
| `0x004373c0` | `JungleCruise_RebuildRoute` | the other direction: decl `BPosW` (ridecb2.c:247) vs def `int` (junglecruise.c:340) |
| `0x00439c90` | `LegoMedia_Remove` | def `ShopTile` (westtown.c:725) vs `MediaShop_Remove(unsigned short)` (interfaces.c:1339) |
| `0x0043d7c0` | `MechanicsHut_EvictRiders` | def `BPosW` (ridemisc.c:93) vs `unsigned int` (ridecb9.c:891) |

Four are `+0x9c` removal handlers registered through the integer spelling, which
is M10 §1f's shape exactly. Two are the restaurant's start/stop sound pair, which
PORT-B11 has just made **audible**, so that one is now observable rather than
theoretical.

### 4c. The selftest: controls for the sweep AND for the claim

23 source checks (a 2-byte `BPos` against a `short` definition IS reported; a
single-element struct is NOT; an 8-byte `Pos` is noisy, not silent; a FIXED site
is not reported, which is what makes a gate possible; `--all` still sees the VC6
arm), plus the ABI claim itself, compiled by the real emcc and the real clang:

```
abi repro, the M10 defect:        native 'slot=0x233f'          wasm32 'slot=0x0aec'
abi repro, the 2-byte UNION:      native 'slot=0x233f'          wasm32 'slot=0x0aec'
abi repro, the size boundary:     native 'one=0x41 two=0x4241'  wasm32 'one=0x41 two=0x10afc'
```

The third line is the window: a ONE-member struct arrives by value on both
(`one=0x41`), a two-member one does not. If a future emsdk stopped disagreeing,
this sweep would be reporting phantoms — the control fails instead of passing
quietly. It skips that half, out loud, when emcc or node is missing, so the
native build still runs the classifier checks.

---

## 5. For the integrator

1. **Three new asset-free ctests** — `raw_words`, `bvstruct_sweep`,
   `bvstruct_sweep_selftest` — and one local one, `probe_audio`. wasm 24/24,
   native 17/17 from clean dirs. The two baselines
   (`portable/tests/rawwords_baseline.txt`, `bvstruct_accepted.txt`) are ratchets:
   they should only ever shrink.
2. **Round-checklist line:** `$PY portable/tools/bvstruct_sweep.py` replaces
   M10's `tools/port_m10_bvstruct_sweep.py` (kept as an alias). Both new gates
   run under `ctest`, so a round that builds both trees needs no extra command.
3. **13 new suspected live defects** for a matching lane (§4b), with citations and
   M10's recipe, and **21 sound-effect bounds** still owed (§1c, §3). The audio
   floor rises from 4 to 25 when they land.
4. **`g_power_table` is closed** by §2's rule — 63 element names that were raw
   x86 addresses now re-point. Nobody has looked at what reads them; if the power
   UI ever showed blank names, that was why.
5. **Two free findings:** `FXEntry`'s sample offset is +0x08 per audiomisc.c and
   +0x04 per joust.c (§3); `coaster.c:1515` declares 0x00412650 as
   `GetSchoolRecord` where ridecb6.c defines it as `Road_FindStartPiece` (§4b).
6. **B11's merge was not in the integration branch** when this lane started —
   `git merge --ff-only feat/decomp-completion-next-steps-24a0d6` had to be run
   twice, a few minutes apart, to land at `68fab1d3`. Worth knowing when a brief
   says "you sit at or after the B11 merge".
