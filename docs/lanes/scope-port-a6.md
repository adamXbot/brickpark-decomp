# Scope PORT-A6 — the extent of an object is `sizeof`, not a table row

> **Status: IN PROGRESS (claimed 2026-09-12 by PORT-A6).** Branch
> `scope/PORT-A6` from `feat/decomp-completion-next-steps-24a0d6` @ `1a7b38a4`
> (the PORT-B6 merge). Files owned: `portable/tools/gen_link.py`,
> `linkreport.py`, `name_trap.py`, `portable/cmake/headless.cmake`,
> `portable/src/headless/**`, `portable/src/hostwin/kernel32.c`, `msvcrt.c`,
> plus the new `portable/tools/cdecl.py`. `LEGOLAND/*.c` untouched, so the VC6
> gate has nothing to check for this lane.

**Headline: the split-record class is closed at its source.** The extent of a
global is not a judgement call — it is `sizeof` of the type the game's own
source declares it with — and `portable/tools/cdecl.py` now computes it.
`STRUCT_EXTENTS`, the hand table that had grown a row every time a split record
broke something visible (`GameInput`, `PopUpUI`, `Profile`, `BlitCtx`/`HitInfo`,
`CurProfile`, after `g_key_state` and `g_gpu_state` before them), is now the
**regression fixture** for that parser: all five rows are reproduced from the
sources, `gen/extents.md` says so per row, and gen_link warns on stderr if one
ever stops being.

| | before | after |
| --- | --- | --- |
| objects merged into one block | 18 | **44** (27 newly merged; one old host, `g_rate_t0`, is now interior to `g_map_ai`) |
| interior offset aliases | 202 | **307** |
| globals defined / data aliases | 2125 / 338 | **2042 / 316** |
| bytes of image in globals.c | 3705868 | **3705868** |
| every initialised byte and pointer TARGET | — | **identical**, 0 of 83k cells differ, both toolchains |
| ctest | native 8, wasm 14 | native **9**, wasm **15** (`cdecl_extents`, asset-free, runs in CI) |

---

## 1. What the generator could not see

`gen_link.py` sizes every global by the gap to the next NAMED address. PORT-A3
made an object whose DECLARED extent swallows other named addresses come out as
one block with interior aliases — but the declared extent was `array bounds ×
element size`, and only for element types whose size is a language fact. For

```c
extern GameInput g_input;            /* 0x00813a40 */
```

it was nothing at all: a struct type has no size until somebody parses the
struct. So the record fell back to the gap tiling, whose idea of "the end of
this object" is *its own second field*, and the files that write
`g_input.point` and the files that read `g_gfx_point` addressed different
memory. Silent, total, and only ever found by a symptom: no input (A5), no
clickable icon and no name editor (B6).

A5 said what the honest fix was and left it: *"the honest general fix is for
`declared_extent` to compute `sizeof` from the struct definition in the
declaring TU; that is a small C parser and was out of this lane's time."* This
is that parser.

## 2. `portable/tools/cdecl.py`

A C declaration parser, 700 lines, no dependencies, that reads `LEGOLAND/*.c`
and `*.h` and answers one question: how many bytes is the object at this
address?

* **What it parses.** `typedef struct Tag { ... } Name;`, `union`, `enum`,
  nested and anonymous aggregates, arrays of them, function pointers, plain
  `typedef <type> Alias;`, and `extern <T> g_x[N][M];` with the `/* 0x... */`
  address anywhere in the statement.
* **How it lays them out** — MSVC on x86, ILP32: `char` 1, `short` 2,
  `int`/`long`/`float`/**every pointer** 4, `double`/`__int64` 8, `enum` 4; a
  member's alignment is `min(#pragma pack in force, natural)`, the aggregate's
  is the max of its members' after that clamp, and its size is rounded up to
  its own alignment. The pack is taken at the line of each member from the
  file's own `push`/`pop`/`(N)`/`()` sequence, which is what makes `Profile`
  0x110 and not 0x114 (`profiles.c:55` opens `pack(push, 1)`).
* **Scope is a translation unit.** The sources define their types locally on
  purpose ("so this file does not depend on legoland.h"), so a type name is
  resolved in the declaring file first and `legoland.h` second, and nowhere
  else. A type nothing lays out yields NO extent — the gap tiling keeps the
  object exactly as it was. **Nothing here guesses.**
* **Disagreements are reported, and the widest wins.** `gamemain.c` declares
  0x0080ffa0 as an opaque `struct CurProfile { char bytes[0x110]; }` (272) where
  `profiles.c` and `bigscreens.c` lay out the 270-byte record; 45 addresses have
  more than one computed extent and they are all in `gen/extents.md`. The widest
  is the only one that can cover the whole record, and A3's clamp (rule 2: stop
  at any address a game object defines) keeps it to storage the tiling was going
  to hand these names anyway.

`python3 portable/tools/cdecl.py --selftest` is the gate: 27 checks — the layout rules
against hand-computed types (packed, natural, nested, union, array-of-struct,
function pointer, anonymous member, a bound that is an expression, a
continuation-line address, a `static`, a function, a scalar, a pointer) and then the five `STRUCT_EXTENTS` rows against the real sources (each
checked twice: cited at exactly the hand value, and covered by the extent used). Registered as the
ctest **`cdecl_extents`** — it reads only `LEGOLAND/*.c`, so unlike
`headless_spine` and `probe_input` it needs no `gamedata/` and runs in CI on
both toolchains.

## 3. `gen/extents.md` — the whole class in one table

`declared_extent` now maxes over three sources: cdecl's computed `sizeof`, the
array bounds the pointer scan already read, and `STRUCT_EXTENTS` (kept as a
FLOOR, so a parser that ever stops seeing one of those five records cannot
silently re-split it). Every object whose computed extent exceeds the gap-tiled
size — every object that *would have been split* — is a row of the new
`gen/extents.md`, with:

* the citation: `<type> <name>, file:line (def file:line)`, plus the other TUs'
  numbers when they disagree;
* what the interior-alias pass did with it (`merged`, `clamped at 0x...`,
  `nothing named inside it`, `no block here`);
* every interior alias with its offset **and the field it lands on** —
  `g_mouse_buttons+0x84 (btn0.state)` — computed by walking the type, with
  `NOT A FIELD` on any offset that is not a field boundary (legitimate for a
  byte name inside a dword, and the first thing to check when a type looks
  wrong);
* the 45 inter-TU disagreements;
* **the residue**: 463 declarations (of 574) whose type the parser cannot size
  and whose tile is ≤ 64 bytes — the objects that could still be split records.
  That list is the honest limit of this lane, and it is a list instead of a
  hunch.

### The 27 objects merged beyond the hand table

Five were the hand table's; 27 addresses that were **not merged at all before**
are merged now. In the order the image has them:

| address | host block | extent | interior names |
| --- | --- | --- | --- |
| `0x004b4140` | `g_copters_fx` | 48 | `g_copters_sample`+0x20 |
| `0x004b43f8` | `g_ds_fx` | 72 | `g_ds_samples`+0x8 |
| `0x004b52c0` | `g_bs_fx` | 24 | `g_bs_launch_sample`+0x8, `g_bs_mermaid_sample`+0x14 |
| `0x004b5b48` | `g_castle_desc` | 56 | `g_station_h1`+0x4, `g_station_height`+0x8, `g_castle_first_corner`+0x10, `g_castle_second_corner`+0x18 |
| `0x004b5cac` | `g_view_wide` | 16 | `g_dir_4b5cb0`+0x4 |
| `0x004b5cbc` | `g_view_car` | 16 | `g_dir_4b5cc0`+0x4 |
| `0x004b5f60` | `g_4b5f60` | 40 | `g_support_shadow_templates`+0x20 |
| `0x004b64d8` | `g_carousel_fx` | 24 | `g_carousel_sample`+0x8 |
| `0x004b7798` | `g_tower_car_geom` | 80 | `g_tower_car`+0x10 |
| `0x004b8750` | `g_power_station_fx` | 24 | `g_power_station_sample`+0x8 |
| `0x004beb88` | `g_level_markers` | 280 | `g_progress_click_level`+0x110 |
| `0x004c119c` | `g_copters_bands` | 24 | `g_copters_slot1..4`+0x4..+0x10 |
| `0x004c1260` | `g_lf_place_cursor_b` | 6196 | `g_lf_tool_b`+0x1828, `g_lf_commit_b`+0x1830 |
| `0x004c4468` | `g_lf_geom_cursor_a` | 6196 | `g_lf_tool_c`+0x1828 |
| `0x004c5ca0` | `g_lf_geom_cursor_b` | 6196 | `g_lf_tool_d`+0x1828 |
| `0x004ca5b0` | `g_lf_place_cursor_a` | 6196 | `g_lf_tool_a`+0x1828, `g_lf_commit_a`+0x1830 |
| `0x004dd758` | `g_cc_txt` | 8 | `g_cc_txt_len`+0x4 |
| `0x004dd860` | `g_cc_obj` | 8 | `g_cc_obj_len`+0x4 |
| `0x0062fdd8` | `g_sbarrel_pivot` | 8 | `g_sbarrel_pivot_y`+0x4 |
| `0x0066b460` | `g_entrance_tile` | 8 | `g_entrance_tile_y`+0x4 |
| `0x007febc0` | `EditCursor` | 6196 | 11 names: `g_mapref`/`g_view`/`g_cursor_mapref`/`g_edit_cursor_origin`+0x1404, the footprint trio +0x1414, `g_8003e8`+0x1828, `g_8003f0`/`g_draw_state`/`g_edit_cursor_next`+0x1830 |
| **`0x0080ff80`** | **`g_front`** | **12** | **`g_cur_screen`+0x4, `g_screen_mode`+0x8** |
| `0x00810160` | `QueryCursor` | 6196 | `g_query_block`+0x1404 |
| **`0x008119b0`** | **`EditMode`** | **12** | **`g_game_mode`+0x4, `g_cursor_def`/`g_edit_class`/`g_edit_object`/`g_effect_obj`+0x8** |
| `0x00829ae0` | `g_castle` | 216 | 16 names (`g_castle_x`+0x8, `g_castle_head_node`+0xa8, `g_castle_tail_node`+0xc0 ...) |
| `0x0082c670` | `g_safari_ofs2` | 8 | `g_safari_zframe`+0x4 |
| `0x00832800` | `g_map_ai` | 1008 | 45 names, the whole simulation record (`g_rate_t0`+0x128 ... `g_switches`+0x3e0) |

**`FrontEndState g_front` at 0x0080ff80 is the one to read twice.**
`frontend2.c:125-136` lays it out as `{popup, screen, mode}` and `frontend2.c:502`
(`SaveFrontEndState`) does `*screen = g_front;` while `movie.c:448`
(`RestoreFrontEndState`) does `g_front = *screen;` — twelve-byte struct
assignments. Split, the save copied eight bytes of PADDING out of a four-byte
object and the restore wrote eight bytes of padding back, while
`bigscreens.c` / `eventtick.c` / `gameframe.c` / `gameframe2.c` read
`g_cur_screen` (+4) and `g_screen_mode` (+8) somewhere else entirely. Nobody had
reported it. `EditMode` at 0x008119b0 (`g_game_mode` at +4 — "2 = front-end
screens") is the same shape, and `uimisc3.c:51` documents that the advert
screen's Go Back saves and restores exactly these two blocks.

## 4. The one thing the merge could have broken, and the fix that had to come with it

A merged host is emitted as one `unsigned int[]` block, and which of its words
are POINTERS was decided by a leading COUNT taken from the declarations at the
block's own address. A pointer field reached through an *interior* name —
`extern char* g_x; /* host+0x30 */` — is not in that leading run, so its word
would have kept the ORIGINAL binary's address: a number that points at nothing
in the rebuilt layout. A5 escaped it because its three records are all-zero in
the image; with 27 more objects merged, several of them pointer tables
(`g_theme_legoland_off` and friends are nine `Sprite*` at 0x007fdd40), it stops
being luck.

`ptr_slot_count` is therefore now `ptr_slot_words`: the SET of word indices the
declarations say hold pointers, gathered from the host's own names at offset 0
and from every interior alias at its own offset. Measured, the re-pointing
counters are unchanged — 299 words at a symbol address, 441 into a block, 12
left raw, 0 synthesised gap blocks — which is the evidence that the change is a
generalisation and not a behaviour change.

## 5. Proof of no regression

1. **The image is the image.** `port-a6-globals-equiv.py` parses both generated
   `globals.c` files into an address → value map (a byte, or for a pointer word
   the TARGET ADDRESS its symbol+offset resolves to — the symbol NAME and the
   offset both move when blocks merge, the target does not) and compares them:
   **0 of 83,252 cells differ on wasm32, 0 of 85,472 on the native build.**
   Byte totals identical (3705868 / .data as before).
2. **Definitions drop, nothing else moves.** globals 2125 → 2042, data aliases
   338 → 316, merged objects 18 → 44, offset aliases 202 → 307; function
   aliases, CRT thunks, cast forwarders, host stubs, unresolved-name stubs,
   prototype conflicts: unchanged.
3. **Tests.** `legoland_linkcheck` links on both toolchains and prints "every
   symbol resolved". Native `ctest` **9/9**, wasm `ctest` **15/15** (the 14
   that existed plus `cdecl_extents`), including `probe_input` (the input chain
   end to end through the game's own globals), `keystate` (the interior-alias
   offsets at -O2), `save_framing`, `res_archive`, `llidb_icm`, `loadpos`,
   `rle_paint`, `anim_paint`, `tri_raster`, `zbuf_blit`, `anim_recolour`,
   `tile_geometry`, `install_paths`.
4. **`headless_spine`'s title checksum `0x4a092b01` is unchanged** — the first
   present of the title screen, 98% non-black, pinned since PORT-A4.
5. **The page.** `legoland_browser`, served on :8799,
   `?args=-nointro+WINDEBUG`: PLAYER DETAILS comes up at 33.5 fps, 96.5%
   non-black, **zero traps**, frame hash `0x9d9b7c50` — the number the brief
   expects, to the digit. One real left click at game (260,188) (hover the
   canvas centre first, per B6's relative-pointer rule) **opens the NEW PROFILE
   name editor**: the slot-1 row becomes an entry field with the red close icon
   and the "Enter player details" tooltip.

### The post-click hash, and why it is not the brief's number

The click's frame hash is `0xdea21168`, not `0xdd2ac534`. That is a real
difference and it is worth the integrator's attention, so it was measured
rather than argued:

* The **pre-A6 closure was rebuilt from `1a7b38a4`'s `gen_link.py`** into
  `portable/build-wasm-base` and driven through the same gesture on :8798. It
  gives `0x9d9b7c50` → `0x5c300878` — so neither build reproduces `0xdd2ac534`
  here, and the integrator's number was taken on a different machine or a
  slightly different gesture. The *before* hash is identical on both
  (`0x43f6e154` with the pointer resting on slot 1), so the procedure is
  reproducible.
* The two post-click frames were compared **block by block** (16×12 grid of
  40×40-pixel FNV hashes over the 640×480 canvas): **exactly one block differs**,
  the one containing the name field's **text caret**, which the A6 closure draws
  and the pre-A6 closure does not. Everything else on the screen is identical.
* Two attribution probes, each a full rebuild of `legoland_browser`:
  **(1)** A6 with `g_front` and `EditMode` suppressed → still `0xdea21168`, so
  it is not those two. **(2)** A6 with cdecl's extents switched off entirely but
  the per-word pointer-slot change kept → `0x5c300878`, i.e. **exactly the
  pre-A6 frame**, so the caret comes from one of the newly merged records and
  not from the pointer-slot change. Narrowing it to the single record is a
  bisect over the 27 addresses with the same probe (`port-a6-make-suppressed.py`
  in the scratchpad takes a skip list); it was not run to the end here.

So the page reaches PLAYER DETAILS with a working click, and the only pixels
that changed are one more piece of the editor drawing itself.

## 6. The scanner fixes (PORT-M4 section 6, items 3–5)

`linkreport.py`'s extern scan is now statement-oriented and reads the headers:

* a statement runs to the `;` that ends it, brace-aware, so a `struct { ... }`
  type body's own semicolons do not end it, and its address comment may sit on a
  continuation line or trail the semicolon (M4's cause 1, 20 names);
* the declared name is the **last identifier outside any parameter list or array
  bound**, with the `(*name)` form recognised at depth 0 only — so
  `extern int (*g_present)(void);` names `g_present` and not `int` (cause 3, the
  dangerous one: an unclassified data name gets a 256-byte ZEROED placeholder,
  which is a second object for a live global), `extern char
  g_key_prev[KEYMAP_COUNT];` names `g_key_prev` and not the macro, and
  `extern void SortSpriteWithCallback(int (*cb)(void));` still names the
  function. Every declarator of a comma list is taken, not just the first;
* `LEGOLAND/*.h` is scanned as well as `*.c` (M4 item 5);
* and — found while testing — the word `extern` inside a COMMENT no longer
  starts a statement. It is common in the files' prose ("the extern's types are
  the caller's codegen lever", llidb_odf.c:39) and such a match swallowed the
  real declaration underneath it.

Measured over the same objects, old scanner vs new:

| | before | after |
| --- | --- | --- |
| externs found | 5301 | **5354** |
| `defined_at` (the `// FUNCTION:` map) | 3323 | 3323, byte-identical |
| `LL_UNPORTED_ASM` sites | 6 | 6, byte-identical |
| names that changed address or class | — | **0** |
| names the old scanner found and the new does not | — | 3, all of them the old one's bugs: `char`, `void`, `__declspec` captured as symbol names |
| unclassified (pre-link, wasm objects) | 23 | **21** |
| unclassified in the census (post-link) | 4 | **4**, all four the toolchain's own (`_errno_location`, `_indirect_function_table`, `_small_sprintf`, `_stack_pointer`) |

The two that left `unknown` are `RES_LowRead` / `RES_LowSeek`, declared
`__declspec(dllimport) int __stdcall ... /* [0x4ab104] */` — a bracketed
address comment the old regex could not read. They are IAT thunk addresses
(`memdb.c:313` names `RES_LowSeek` as `SetFilePointer`), so they are really
**host imports wearing game names**; classifying them as `game-fn` changes
nothing today (the shim defines them and they never reach the closure) but the
right home for them is `win32_imports.txt`. **Recorded, not changed** — it is a
census question, not this lane's.

`gen/globals.c` is bit-identical with and without this commit: the 56 names the
new scanner adds are all functions the tree already defines, so classification
never sees them.

## 7. Overlapping-name hazards (the A3 sweep, now computed)

`python3 portable/tools/cdecl.py --overlaps` is the sweep as a command: every
pair of names ONE translation unit declares whose storage overlaps after the
merge, with the functions that touch both and the DIRECTION of each access
(`w` = the name, after any `.f`/`[i]` chain, is the left side of an assignment
or the destination of a `memset`/`memcpy`-shaped call).

```
372 overlapping declaration pairs inside a single TU   (A3 counted 45)
  42 same-address aliases (the census's "stale extern names") — pre-existing
 330 interior pairs, 165 of them in objects this lane newly merged
167 pairs where a single function STORES through one of the two
 61 of those in a translation unit the front end runs
```

**The hazard is real but it is not new, and nothing measured misbehaves.** The
condition (A3, measured at `-O2` on wasm32) is one TU declaring two names whose
extents overlap: the C type system sees two distinct objects, so a store through
one and a load through the other in the same function may keep the stale value.
It is not type-based aliasing; `-fno-strict-aliasing` does not touch it. The
front end runs, `probe_input`, `keystate` and `headless_spine` pass at the
browser target's `-O2`, and the page reaches the name editor.

The front-end pairs worth naming, all of them cases where one function writes
one name and touches the other:

| TU | pair | what one function does |
| --- | --- | --- |
| `gpu.c` | `g_ddraw1` / `g_ddraw`, `g_drvcaps`, `g_helcaps`, `g_gdi_obj0..3` | `InitHostSystemGPU` reads `g_ddraw1` and writes the caps; `KillHostSystemGPU` writes `g_ddraw1` and reads the GDI handles — this is the pair A3 measured going wrong at -O2 in `test_keystate.c` |
| `gameframe.c` | `g_hit_type` / `g_icon_value`, `g_hit_cell` | `InGameFrameBody` reads and writes all three (the 12-byte hit record B6 proved) |
| `gameframe.c` | `g_edit_mode` / `g_edit_object` (+0x8) | `HandleMapClick` writes the mode and reads the object |
| `movie.c`, `popupmisc.c` | `g_edit_changed` / `g_game_mode`, `g_edit_object` | `EnterParkPlayMode`, `SelectNextBuildObject` write both |
| `blitmisc.c` | `g_ddsd` / `g_render_clip` (+0x6c) | `PushRenderingStatusAndRelockVideoSurface` writes the surface desc and reads the clip |
| `misc3.c`, `fpui3.c` | `g_query_cursor` / `g_query_block` (+0x1404) | `PopUpCanDelete`, `PU_ToolB` write the block through the cursor record |
| `gamemain.c` | `g_cur_profile` / `g_vol_speech`, `g_vol_music`, `g_vol_sfx` | `ResetCurProfileDefaults` writes the record and the three volumes |
| `bigscreens.c`, `fpui.c`, `screencb.c` | the sprite-slot arrays (`g_save_type_normal`, `g_theme_*_on/off`, `g_oct_*`) | one loader writes the whole array through the first slot's name and the others individually |

### The recipe for a matching lane (do not edit game files from here)

The fix belongs in the DECLARING file, under a `LEGOLAND_PORTABLE` guard, and
only where a single function both stores and loads across the pair:

1. **Prefer one declaration.** Where a TU declares both the record and a field
   of it (`gameframe.c`'s `g_edit`/`g_edit_mode`, `gamemain.c`'s
   `g_cur_profile`/`g_vol_*`), delete the field spelling and use the record's
   member: `g_cur_profile.f24` instead of `g_vol_speech`. One object, no
   aliasing question, and the bytes do not move — but it changes the VC6
   codegen lever (HANDOFF §3), so it must be gated:
   `#ifdef LEGOLAND_PORTABLE ... #else <the shipped spelling> #endif`, never
   between a `// FUNCTION:` marker and its signature, with `audit.py` `[OK]`
   and `relocs.py` 0 MISMATCH per file.
2. **Otherwise `volatile` on the lvalue**, as `portable/tests/test_keystate.c`
   does: `*(volatile int*)&g_icon_value`. It forces the real load and store and
   costs one memory access on a path that is not hot. Guarded the same way.
3. **Do not touch the 42 same-address pairs.** Two names for one address are
   the same object to the linker and the same storage to the compiler; they
   were never the hazard.
4. The sweep is the test: `cdecl.py --overlaps` prints the pair, the function,
   and `w`/`r` per name, so a lane can work the list and re-run it.

## 8. Census

`portable/tools/linkreport.py portable/build-wasm` (258 objects, 6596 defined,
48 undefined), after:

| category | count |
| --- | --- |
| game-fn | 0 |
| game-data | 0 |
| alias | 0 |
| host | 0 |
| crt | 44 |
| unknown | 4 (`_errno_location`, `_indirect_function_table`, `_small_sprintf`, `_stack_pointer` — the toolchain's own) |
| duplicates | 0 |
| asm stubs | 6 |
| prototype conflicts | 411 |

Unchanged by this lane: **nothing here adds or removes a symbol**, it only
changes how many objects the same bytes live in, and that claim is checked
byte-for-byte in §5.1.

## 9. Reproducing

```bash
PY=$HOME/.venvs/legoland/bin/python
$PY portable/tools/cdecl.py --selftest        # the layout rules + the 5 fixture rows
$PY portable/tools/cdecl.py                   # every struct-typed extern
$PY portable/tools/cdecl.py --overlaps        # the optimiser-hazard sweep

cmake -S portable -B portable/build -G Ninja -DPython3_EXECUTABLE=$PY
ninja -C portable/build && ninja -C portable/build legoland_linkcheck legoland_tests legoland_cbtypes
./portable/build/legoland_linkcheck && (cd portable/build && ctest)          # 9/9

emcmake cmake -S portable -B portable/build-wasm -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DLL_ILP32=ON -DPython3_EXECUTABLE=$PY
ninja -C portable/build-wasm
ninja -C portable/build-wasm legoland_linkcheck legoland_headless legoland_headless_debug \
    legoland_tests legoland_pathtest legoland_shimtest legoland_browser legoland_cbtypes
(cd portable/build-wasm && ctest)                                            # 15/15

less portable/build-wasm/gen/extents.md       # the table this lane exists for
```

The page: `(cd portable/build-wasm && python3 -m http.server 8799)` then
`http://localhost:8799/legoland.html?args=-nointro+WINDEBUG`; hover the canvas
centre first (the pointer is relative), then game (260,188), then click;
`window.llFrameHash()` before and after.

## 10. Scratch

`port-a6-*` in the shared scratchpad. Two worth keeping:

* **`port-a6-globals-equiv.py`** — "are these two generated closures the same
  memory?" It resolves every pointer word to a target ADDRESS, so it is immune
  to blocks merging and symbols being renamed. This is the gate for any future
  change to the extents, and it is 130 lines.
* **`port-a6-make-suppressed.py` + `port-a6-suppress.sh`** — build
  `legoland_browser` with a named set of merges suppressed. That is how a frame
  difference gets attributed to one record instead of argued about.
