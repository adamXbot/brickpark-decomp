# Scope PORT-M15 — the dual-address global class, closed: 17 names, one address each

> **PORT-M15 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-M15)** — branch
> `scope/PORT-M15`, cut from `feat/decomp-completion-next-steps-24a0d6` at the
> PORT-P3 merge (`25005b2c`). Matching-side lane: every change to
> `LEGOLAND/*.c` is an **identifier rename or comment text**, which cannot move
> a byte, and every touched file is re-gated with `audit.py` + `relocs.py`.
> `portable/**` and `docs/HANDOFF.md` are other lanes'. Brief:
> `docs/SCOPE_PORT_WAVE.md`. Parent finding: PORT-P3 §3.3 (P3-1/P3-2/P3-3).

**Headline: the address comments were right all 34 times, so not one of the 17
is a wrong comment.** `relocs.py`, with its symbol resolver monkeypatched to
leave these names UNRESOLVED so every relocation prints `original=0x...` — the
address the ORIGINAL binary uses at that exact instruction — agrees with every
declaration in the tree. The class is therefore never "one side is mislabelled":
it is always **two different objects wearing one name**, and in nine of the
seventeen the other object already had a perfectly good name somewhere else in
the tree that nobody had connected to it.

| | before | after |
| --- | --- | --- |
| PORT-P3's dual-address sweep | **17** names | **0** |
| declaration sites whose address comment was WRONG | — | **0 of 34** |
| verdict (a) "the comment is wrong, name stays" | — | **0** |
| verdict (b) two real objects, one renamed to a NEW name | — | **8** names (5 decisions) |
| verdict (c) a stale alias of a differently-named object | — | **9** names |
| files touched | — | **16**, `audit.py` output **byte-identical** on every one, `relocs.py` **0 MISMATCH**, 3281/42 |

---

## 1. How the ground truth was taken

`tools/relocs.py` resolves a relocation's symbol through the declaring file's
own `/* 0x... */` comments first (`symbol_address`, `local`), so a file whose
comment agrees with the binary never prints anything — which is precisely why
the tree-wide sweep has been at 0 MISMATCH while seventeen names were wrong in
the portable build. PORT-M4's trick makes it talk:
`scratchpad/port-m15-probe.py` wraps `relocs.symbol_address` and returns
`(None, 'FORCED(name)')` for the names under test, so every relocation naming
one prints `UNRESOLVED ... original=0x........`. That value is decoded out of
the *original image's* operand at that instruction. It is the only evidence in
the project that cannot be argued with: the comment is a comment and the name
is a name, but the shipped `mov eax,[0x004b95f4]` is a fact.

Run over all 50 declaring files (`scratchpad/port-m15-groundtruth.sh`), it
produced a target address for every name in every file that reads or writes it.
Nine names also needed a second source of truth — the shipped bytes at the
address — read with `scratchpad/port-m15-peek.py` / `port-m15-fx.py` out of
`original/legoland.exe`.

**Two names had no relocation to check at all**, because no exact body in the
declaring file touches them: `g_tile_sprites` in bigrender.c / render4.c /
renderview.c (the winners agree with savemisc2.c and unref5.c, which do reach
0x00805f60) and `g_screen` in input.c (four other files reach 0x004bcbf4).
Neither needed a decision.

---

## 2. The seventeen, with the evidence

### 2a. P3-1 — `g_view_left` / `_top` / `_right` / `_bottom`: two real objects

| | |
| --- | --- |
| declared | scrolltick.c:28-31 `int` @ **0x004b95f4/f8/fc/9600**; coaster3d.c:104-107, coaster10.c:48, unref3.c:55-58 `int` @ **0x008299ac/b0/b4/b8** |
| relocs says | `ClampScrollToMap` 0x00461290 i=1/5/8/11 -> **0x004b95f4/f8/fc/9600**. `TransformVerts` 0x00426250, `Coaster3D_SetupView` 0x00425e20, `ClipRect_SetBounds` 0x00426700, `Coaster3D_PlotPoints` 0x004238a0, `Coaster3D_SetupFlatView` 0x00426000, `CoasterRegions_UpdateClipCodes` 0x00426680 -> **0x008299ac/b0/b4/b8**. Both comments correct. |
| what each IS | 0x004b95f4 is **.data with shipped constants** — 243200, 243200, 243200, 243200, then 92160, 92160, 76800, 76800 at +0x10..+0x1c, i.e. 950/950/950/950/360/360/300/300 px in 8.8 — and nothing in the game writes any of the eight. 0x008299ac is **.bss** (no raw data in the image at all), written every frame by `Coaster3D_SetupView` from `lpConfig`'s view rect, next door to `g_clip_ring` (0x00829a3c) and `g_view_matrix` (0x008299fc). |
| verdict | **(b)** two real objects. The coaster's is literally a view rectangle; scrolltick's eight are the SLACK the clamp allows past each map edge, used only at half value. **The slack block is what moved**: `g_scroll_slack_left/top/right/bottom/w/h/ox/oy`. All eight, not just the four that collided, so the block stays one vocabulary. |
| the cost | with `hl == 0` the first edge clamp (`d = x - 2y - hl + vw`, pushed until `d <= 0`) settles exactly on `d == 0` and the view stops **243200 >> 1 = 475 px** short of every east edge. PORT-P3 §3.1 measured it three times on two maps, `g_scroll_x` landing on `2y - vw` to the unit. |

### 2b. P3-2 — `g_popup`: ONE record, two bases

| | |
| --- | --- |
| declared | bighelp.c:456, popup.c:661 `PopUpUI` @ **0x007fdea4**; fpui2.c:1443 `PopUpInfo` @ **0x007fdec0** |
| relocs says | **`InitPopUpInfo` (bighelp.c, 0x00470bb0) reaches BOTH in one body**: i=206/210/212/216/220/224 -> 0x007fdea4 (+0x0) and i=3 -> **0x007fdec0 (+0x1c)**, plus +0x8, +0xc, +0x10c, +0x110, +0x114, +0x118, +0x124, +0x12c, +0x138, +0x13c, +0x140, +0x160, +0x164, +0x174. `PopUpInfoSetUp` (fpui2.c, 0x00471950) reaches 0x007fdec0 +0x0/+0x4/+0x8/+0xc/+0x10 and +0xbc..+0xfc — every one of which is 0x1c above a field bighelp.c wrote. |
| verdict | **not two objects at all**: one 0x178-byte record whose two halves of the game address from bases 0x1c apart, which is legal in the original (the absolute field addresses agree: `elem_shed` is 0x007fdfb0 = 0x7fdea4+0x10c = 0x7fdec0+0xf0) and fatal in the portable build, where the generator emits ONE object per NAME. fpui2.c's view renamed `g_popup_info`; `gen_link.STRUCT_EXTENTS[0x007fdea4] = 0x178` already forces the host record, so the interior name becomes a `.set host+0x1c` alias. |
| the cost | `PopUpInfoSetUp`'s `elem_shed`/`elem_hut` comparands read the zeros 0x1c low, so the "this is a hut/shed, hire someone" arm never fires for either, and with it the 0x306/0x307/0x308 worker pick-up and put-down. **No gardener and no mechanic can be hired at all** (PORT-P3 §3.2 dumped the live record). |

### 2c. The nine that already had a name in the tree — verdict (c)

| name | loser addr | the tree's own name for that address | evidence |
| --- | --- | --- | --- |
| `g_ui_flags` (logflume2.c, screencb6.c, unref4.c) | 0x008003e8 | **`g_edit_cursor_flags`** (== `g_edit_cursor.flags`) | ridecb8.c:337 puts `Cursor.flags` at **+0x1828**, and `g_edit_cursor` is 0x007febc0 (the export table's `EditCursor`): 0x007febc0 + 0x1828 = **0x008003e8** exactly. Every one of the six uses is the same idiom — `DefaultCursor(&g_edit_cursor); X \|= 8; SetEditCursorFootPrint(fp);` — and ridecb8.c:1278/1572 writes that line as `g_edit_cursor.flags \|= 8`. Its neighbour 0x008003f0 is already `g_edit_cursor_next` in three files. `g_ui_flags` is **`GamePad`** at 0x00813a40 per `docs/DECOMP.md`, which gameframe.c tests for 0x20/0x400/0x1000. |
| `g_tile_sprites` (coaster.c) | 0x0082c680 | **`g_roads_data`** (ridecb8.c:1352, "the loaded \"DSCHOOL LIGHTS\"") | `DrawTrafficLights` (0x00414440) reads 0x0082c680 eight times as a `TileSprites*`; `LoadDrivingSchool` (ridecb8.c, 0x00406070) stores the looked-up "DSCHOOL LIGHTS" there, beside `g_roads_def` 0x0082c684 and `g_dschool_cls` 0x0082c684. The traffic lights ARE the driving school's light tiles. `g_tile_sprites` is the export table's **`TileSpriteArray`** at 0x00805f60 — the map's 2048 slots. |
| `g_snd_click` (fpui4.c) | 0x004b929c | **`g_snd_close`** (fpui3.c:345 AND screens3.c:236) | `FXEntry` is 12 bytes from `g_game_fx` @ 0x004b9228 (mapinit.c:11). 0x004b929c - 0x004b9228 = 0x74 = 9*12 + 8, so it is **`g_game_fx[9].sample`**, and the name pointer at entry 9 reads **"Click01.wav"** out of the image. 0x004b92c0 = 12*12 + 8 = **`g_game_fx[12].sample` = "Button04.wav"**. screens3.c is the decisive witness: it declares BOTH, `g_snd_close` at 0x004b929c and `g_snd_click` at 0x004b92c0. |
| `g_screen` (printlist.c) | 0x00668078 | **`g_surface_78`** (blitmisc.c:118) | blitmisc.c `Restore`s it and polls `GetFlipStatus(DDGFS_ISFLIPDONE)` on it; it sits between `g_primary` 0x00668070 and `g_draw_surface` 0x0066807c, both of which printlist.c's siblings declare. `g_screen` is the config pointer at 0x004bcbf4 in five files (the export table's **`lpConfig`**). |
| `g_carousel_bnv` (bswater.c) | 0x0061608c | **`g_carousel_run_s`** (screencb2.c:769) | screencb2.c's `Carousel_Create` (0x0042c280) is explicit in its own prose: *"Every one of the four \"table\" resources is ALSO stored under its own scalar name; the scalar is written first."* Its relocations prove it — +0x0/+0x4/+0x8 of `g_carousel_bnv[]` at **0x00616090/94/98**, with the scalars at 0x00616080/84/**8c**. |
| `g_carousel_zspr` (bswater.c, ridecb1.c) | 0x006160b8 | **`g_carousel_zspr_s`** (screencb2.c:767) | same function, same shape: `g_carousel_zspr_s = LoadSprite("z_Carousel.lls", 1); g_carousel_zspr[0] = g_carousel_zspr_s;` — **0x006160b8** then **0x006160c0**. |
| `g_bz_bnv` (ridecb3.c) | 0x00616010 | **`g_bz_bnv0`** (screencb2.c:693) | `Balloonz_Create` (0x0042a7b0): `g_bz_bnv0 = LoadBinV("Zbuffers\\balloonz.bnv"); g_bz_bnv[0] = g_bz_bnv0;` — **0x00616010** then **0x00616018**; `Balloonz_Load`/`Balloonz_Destroy` reach 0x00616018 and `Balloonz_Tick` 0x00616010. |

Three of those nine (`g_carousel_bnv`, `g_carousel_zspr`, `g_bz_bnv`) are the
small-delta "+4/+8 shear" P3 flagged as the same shape as `g_popup`. They are
not a shear at all once the right file is read: they are a **scalar slot and a
class table holding the same pointer**, deliberately both written, and the
collision merged each pair into one object.

### 2d. The five that needed a new name — verdict (b)

| name | the two objects | renamed | evidence for the new name |
| --- | --- | --- | --- |
| `g_view` | **0x004bcbf4** the config pointer (sysmisc2.c) vs **0x007fffc4** the edit cursor's origin (buildtick.c, eventtick.c) | sysmisc2.c -> **`g_view_cfg`** | `relocs` puts `SetSampleScreenPos` on 0x004bcbf4 and `ObjectIsBuilt`/`PlaceScriptObject` on 0x007fffc4 +0x0..+0x10. Naming 0x004bcbf4 per-TU is the tree's deliberate habit (`g_map`, `g_level_map`, `g_game`, `g_screencfg`, `g_map_cfg`, `g_scroll_map`, `g_level_cfg`, `lpConfig` are all this one pointer), so the rename follows it; 0x007fffc4 is already `g_edit_cursor_origin` in ridecb8.c:561. |
| `g_road_tiles` | **0x004b4c08** the shipped `unsigned short[15][4]` code table (roads.c) vs **0x004cbeac** the live road record list (coaster.c, ridecb8.c, screencb.c, screencb2.c, screencb4.c) | roads.c -> **`g_road_tile_codes`** | `Road_SetTile` (0x00412680) reaches 0x004b4c08 + 0x0..0x70, i.e. across all 15 rows. roads.c's own header documents the words as `(tileset << 8) \| slot`, and "TILES FOR DSCHOOL" at 0x004b4c80 pins the extent. 0x004cbeac is `g_road_list` in ridecb5.c/ridecb6.c/roads2.c. |
| `g_frame_ticks` | **0x0060f910** the coaster's RDTSC frame elapsed (schoolcar.c) vs **0x006681fc** the game's tick scale (blitmisc.c, pathtile2.c, sysmisc.c) | schoolcar.c -> **`g_zb_frame_ticks`** | `Coaster3D_EndFrame` (0x00423140) and `Coaster3D_SampleStats` (0x00424e80) reach 0x0060f910; the module's own note says "the whole body is timed with RDTSC and the elapsed low dword is left in 0x0060f910", and its neighbours are `g_zb_polys` 0x0060f900, `g_frame_cmds` 0x0060f908, `g_frame_abandoned` 0x0060f90c. 0x006681fc is the export table's **`LastFrameMS`**, which `FlipPrimary`'s 28 ms spin reads. |
| `g_lls_accept_on_report` | **two distinct string literals differing only in the case of "on"** | screens3.c -> **`g_lls_accept_on_report_tut`** | read straight out of the image: 0x004bf694 is `"Accept_On_Report.lls"` (capital O), in the report-screen pool beside `Rep_Hint1.lls`/`Rep_Hint2.lls`/`Interval_Screen.lls`; 0x004bef70 is `"Accept_on_Report.lls"` (lower-case o), in the tutorial pool beside `GoBack_on_Tut.lls`/`TutorialBK.lls`/`Accept_On_Reg.lls`. `InitScreen7` (mapscreen2.c) reaches the first, `InitTutorialScreen` (screens3.c, 0x0048bde0) the second. Merging them gave the tutorial screen the report screen's sprite name. |
| `g_avi_open_count` | **0x00665f48** the advisor module's AVIFile tally vs **0x00668f98** movie.c's | advisor.c -> **`g_advisor_avi_open_count`** | `LoadAdvisorMovie` (0x00443bd0, five reads) and `FreeAdvisorClip` (0x00443d50) reach 0x00665f48, among `g_advisor_clip` 0x00665f5c / `_pose_clip` 0x00665f60 / `_c` 64 / `_a` 68 / `_b` 6c; `OpenMovie` (0x00476460) and `CloseMovie` (0x00476630) reach 0x00668f98, among movie2.c's `g_mva_*` block 0x00668f84..0x00668fa4. Two modules, two independent `AVIFileInit`-at-zero guards; merged, each could see the other's init as already done. |

---

## 3. The gate table

`$PY tools/audit.py LEGOLAND/<file>.c` and `$PY tools/relocs.py LEGOLAND/<file>.c`
were run on each file **before** and **after**, and the audit output compared
with `diff` — not the counts, the rows (`docs/HANDOFF.md` §4: a count is not a
set). Every one is byte-identical, which is the whole point: an identifier
rename and a comment cannot move an instruction.

| file | audit | relocs | before/after |
| --- | --- | --- | --- |
| `scrolltick.c` | 5 [OK], 0 REJECT | 0 MISMATCH | rows IDENTICAL, SUMMARY identical |
| `fpui2.c` | 14 [OK] | 0 | IDENTICAL |
| `logflume2.c` | 56 [OK] | 0 | IDENTICAL |
| `screencb6.c` | 18 [OK] | 0 | IDENTICAL |
| `unref4.c` | 10 [OK] + 1 [WIP] | 0 | IDENTICAL |
| `coaster.c` | 59 [OK] | 0 | IDENTICAL |
| `fpui4.c` | 6 [OK] + 1 [WIP] | 0 | IDENTICAL |
| `bswater.c` | 4 [OK] | 0 | IDENTICAL |
| `ridecb1.c` | 6 [OK] | 0 | IDENTICAL |
| `ridecb3.c` | 9 [OK] | 0 | IDENTICAL |
| `printlist.c` | 10 [OK] | 0 | IDENTICAL |
| `sysmisc2.c` | 5 [OK] | 0 | IDENTICAL |
| `roads.c` | 2 [OK] | 0 | IDENTICAL |
| `schoolcar.c` | 49 [OK] | 0 | IDENTICAL |
| `screens3.c` | 73 [OK] | 0 | IDENTICAL |
| `advisor.c` | 10 [OK] | 0 | IDENTICAL |

Tree-wide, same round:

| gate | result |
| --- | --- |
| `progress.py --check` | **3281 exact / 42 WIP** (report regenerated: the comments moved line numbers) |
| `portable/tools/extern_sweep.py` | clean, exit 0 ("the class is closed" — and see below) |
| `portable/tools/bvstruct_sweep.py` | 0 unaccepted silent sites |
| `tools/port_m10_bvstruct_sweep.py` | 0 slot-vs-body sites |
| **PORT-P3's dual-address sweep** | **17 -> 0** (`scratchpad/port-m15-dualaddr.py`) |

**`extern_sweep.py` says "the class is closed" and was blind to all
seventeen.** It sweeps PORT-M6's class — one `extern` STATEMENT carrying two
addresses in one comment — which is a different shape from one NAME carried at
two addresses by two files. Both are portable-build-only and invisible to every
byte gate. P3's sweep belongs in the round gate beside it; it is ten lines and
needs no build.

---

## 4. The collateral: what the closure generator did differently afterwards

Both builds were run twice — the pre-change tree (`364f5668`) into its own
directory and the post-change tree — and `gen-browser/extents.md`,
`rawwords.md` and `globals.c` diffed. Ignoring `file.c:NNN` line-number drift
from the new comments (`scratchpad/port-m15-extdiff.py`), there are **12 real
row changes**, every one of them a name landing where it belongs.

### 4a. `g_cursor_sprite_put[5]` — the swallow P3 named, closed

```c
/* BEFORE — 0x004b95f0 .data 20 bytes */
unsigned int g_cursor_sprite_put[5] = {
    (unsigned int)&g_cursor_sprites, 0x0003b600u, 0x0003b600u, 0x0003b600u, 0x0003b600u };
...
unsigned int g_view_left[1];        /* 0x008299ac, uninitialised — the ONLY g_view_left */

/* AFTER — 0x004b95f0 .data 4 bytes */
unsigned int g_cursor_sprite_put[1] = { (unsigned int)&g_cursor_sprites };
unsigned int g_scroll_slack_left[1]   = { 0x0003b600u };   /* 0x004b95f4 */
unsigned int g_scroll_slack_top[1]    = { 0x0003b600u };   /* 0x004b95f8 */
unsigned int g_scroll_slack_right[1]  = { 0x0003b600u };   /* 0x004b95fc */
unsigned int g_scroll_slack_bottom[1] = { 0x0003b600u };   /* 0x004b9600 */
unsigned int g_view_left[1];                               /* 0x008299ac, still .bss */
```

render5.c:89 declares 0x004b95f0 as a bare `CursorSprite*`, so with no symbol
at 0x004b95f4 the extent walker ran it to the next one it knew — `g_view_w` at
0x004b9604 — and the four shipped 243200s became words 1..4 of a pointer
object. Giving the block its own names makes the pointer 4 bytes again and the
slack four real, initialised dwords. **This is why the scroll clamp read zeros
even though the values are right there in `.data`.**

### 4b. A record whose extent the wrong-address declaration had distorted

`g_lls_goback_on_tut` (screens3.c, 0x004bef5c, `"GoBack_on_Tut.lls"`) was sized
**44 bytes** and is now **20**. The string is 17 characters; 44 bytes is it
plus the 24 that belong to the literal at 0x004bef70 — which had **no symbol at
all** before, because screens3.c's `g_lls_accept_on_report` was emitted at
0x004bf694 (the report screen's copy). One name, two literals, so one of them
never existed and its neighbour absorbed it:

| | before | after |
| --- | --- | --- |
| `0x004bef5c g_lls_goback_on_tut` | 44 bytes | **20 bytes** |
| `0x004bef70 g_lls_accept_on_report_tut` | *absent* | **24 bytes** |
| `0x004bf694 g_lls_accept_on_report` | 24 bytes | 24 bytes (unchanged) |

`InitTutorialScreen` therefore pushed the report screen's string. Both spell
the same filename modulo the case of "on", so the `.lls` load still found a
sprite — which is exactly what made it invisible.

### 4c. The interior aliases that now exist

| address | interior name gained | host |
| --- | --- | --- |
| 0x007fdec0 | **`g_popup_info`+0x1c** (`.set g_popup_info, g_info_icon_g+28`) | the 0x178-byte PopUpUI at 0x007fdea4 |
| 0x008003e8 | **`g_edit_cursor_flags`+0x1828** | the 6196-byte EditCursor at 0x007febc0 |
| 0x00616018 | `g_bz_bnv` sized as its own 16-byte row | — |

Sized addresses in `extents.md` go **461 -> 463**. `rawwords.md` is **byte-for-byte
identical** before and after (989 image-range words, 85 unvisited in 12 objects),
so nothing regressed in the pointer census, and ctest `pointer_words` and
`raw_words` pass in both arms.

### 4d. Builds

| target | result |
| --- | --- |
| `emcmake … -DLL_ILP32=ON` + `ninja` + the six extra targets | links; **ctest 24/24 PASS** |
| native clang + `ninja` + `legoland_tests legoland_cbtypes` | links; **ctest 17/17 PASS** |

---

## 5. The page proof — Lesson 4 runs to the end

Served `portable/build-wasm` on **8843** (another lane already held 8823),
page `legoland.html?args=-nointro+WINDEBUG&beat=1000`, one fresh tab,
`p3KeepAwake()` for P3-7. A profile was created in the front end
(`m15`, slot 2) and `p3UnlockLessons(5)` wrote `g_level_done[0..4] = 1` into
the on-disk `Profile2.txt` at +0x34 — PORT-P3's recipe, unchanged.

### 5a. P3-1 closed — the clamp, measured the way P3 measured it

Hold the cursor in the right-hand margin until `MouseScrollMap` stops moving
the view, then read `g_scroll_x/y` and compare against edge clamp 1's own
settling point. FOUR.MAP, side panel closed — **P3's exact row**:

| | P3 (before) | PORT-M15 (after) |
| --- | --- | --- |
| `g_scroll_x` | **-29696** | **91904** |
| `g_scroll_y` | 67072 | 67072 |
| `view_w << 8` | 163840 (640) | 163840 (640) |
| `2*y - vw` (the clamp at slack 0) | -29696 — **exact match** | -29696 — no longer the answer |
| `2*y - vw + (243200>>1)` (shipped slack) | — | **91904 — exact match** |
| extra eastward travel | 0 px | **475 px** |
| east-most reachable column `cx - cy` | **31** | **40** |

`matches_defect: false, matches_fixed: true, delta_from_defect_px: 475`. The
clamp now settles 121600/256 = 475 px further east, to the unit, which is
`g_scroll_slack_left >> 1` in 8.8 — the shipped value, finally read.

### 5b. The Mechanic's Hut, on screen

With P3's affine cell<->screen inverter (`p3Fit`/`p3CellTo`, three probes):

```
hut cell (46,6)  ->  screen (277,192)          (well inside the 640x480 canvas)
llSel().input.mapRef          -> [46, 6]
llSel().selDef.name           -> "Mechanic's Hut"
llSel().hit.typeHex           -> 0x103
frame hash                    -> 0x96f922b9
```

P3: *"the hut is at cx-cy = 40 — 143 px beyond the canvas"*. Screenshot
`scratchpad/port-m15-l4-hut-reachable.png` (the game's own framebuffer, not the
pane) shows it centred with its tooltip.

### 5c. P3-2 closed — the hire fires, and it costs 30 bricks

| click | money | delta |
| --- | --- | --- |
| Mechanic's Hut, lesson 4 (first) | 460 -> **430** | **-30** |
| four more, lesson 4 | 818 -> 788 -> 758 -> 743 -> 713 | -30, -30, -15, -30 |
| **Greenhouse (POTTING SHED), lesson 5, four clicks** | **1000 -> 970 -> 940 -> 910 -> 880** | **-30 each** |

P3's lesson-5 row was *"Four clicks, money 1000 -> 1000"* — nobody hired, nothing
charged, because `PopUpInfoSetUp`'s `elem_shed`/`elem_hut` comparands read the
zeros 0x1c low. The same four clicks now charge `CanHireGardener`'s 30 bricks
each (sysstubs.c:326) and put four gardeners in the park. **Both the gardener
and the mechanic arm work.**

### 5d. How far Lesson 4 goes now: ALL THE WAY

| # | objective | check | P3 | PORT-M15 |
| --- | --- | --- | --- | --- |
| 0 | (reward only) | DEGRADE, FLASHBUTTON QUERY | yes | yes |
| 1 | query a damaged ride | `SELECTMODE QUERY` + `SELECTTHEME LEGOLAND` | yes | **yes** — "You have a new object: Copters Ride" |
| 2 | build the Copters | `NEED "COPTERS", 1` | yes | **yes** — money -60, `llLink('COPTERS').verdict = "SATISFIED"`, reward `LOOKAT 45,5` fires (screen-centre cell moves to (42,6)) |
| 3 | get a mechanic | `NEEDMECHANICS 1` | **BLOCKED** | **yes** — panel advances to *"Now get four more Mechanics and make sure that every ride in the park is working properly. Then click the End Level button to finish."* |
| 4 | drop her on the Space Tower | `FIXRIDES 0, 20` | unreachable | **yes** |
| 5–8 | four more mechanics | `NEEDMECHANICS 2..5` | unreachable | **yes** — four more hires |
| 9 | everything repaired; `ENDLEVEL` | `FIXRIDES 0, 100` | unreachable | **yes** — `llLink().goals` and `.events` both go to **[]** (every pending objective satisfied and cleared) |
| — | the End Level button | — | — | **clicked** ("Finish the level." tooltip, the door icon at (572,432)); `gameMode` **3 -> 2** |
| — | the debrief | — | — | *"I don't know about you but I thought that was cool, well done! … Good work! It looks like we're going to have to find something to really challenge you!"* |
| — | back to SELECT TUTORIAL LEVEL | — | — | **Lesson 5 is now the lit one** — the profile progressed |

**Lesson 4 went from 3 objectives of 9 to 9 of 9 and a completed level.**
Lesson 5 was then started from the same session to test the gardener arm: it
loads (money 1000), the Boating School, the pirate ship, the hedges and the
LEGOLAND flags all draw, and objective 1's own text is on the notepad
(`scratchpad/port-m15-l5-gardeners-hired.png`).

### 5e. Frame hashes and health

| screen | hash |
| --- | --- |
| PLAYER DETAILS, slot 2 selected | `0x72ddb095` |
| TITLE | `0xe4514fcd` |
| SELECT TUTORIAL LEVEL, all five | `0xcf6374dd` |
| Lesson 4 lit | `0x644a0f24` |
| Lesson 4 BRIEFING (money 450) | `0x83851d15` |
| **Lesson 4 park** | `0xd3efcaa6` |
| **east clamp, hut on screen** | `0x96f922b9` |
| obj 1 done, COPTERS granted | `0x2665f1a1` |
| COPTERS built (SATISFIED) | `0xad3d0c48` |
| obj 3 prompt after the first hire | `0x8ec023f5` |
| "Finish the level." hovered | `0xa21cfbad` |
| **level complete, debrief** | `0xc3ecb044` |
| SELECT TUTORIAL LEVEL, Lesson 5 lit | `0x115cd334` |
| Lesson 5 park, four gardeners hired | `0x23d3c9b0` |

**fps 32.0 – 35.6, mean ~34.2** over ~35,500 presented frames.
`llStats().dead` stayed `null` and `traps` stayed `[]` for the entire session,
across a completed tutorial level and the start of a second one.

One harness note for the next lane: **never touch `Module.wasmExports`.**
Emscripten's accessor `abort()`s when the symbol is not exported, which kills
the runtime silently — every later click does nothing and the frame hash
freezes. It cost this lane one page reload to notice.

---

## 6. Files touched

| file | change |
| --- | --- |
| `docs/SCOPE_PORT_WAVE.md` | the PORT-M15 status line |
| `docs/lanes/scope-port-m15.md` | these notes |
| `docs/LEGOLANDPROGRESS.HTML` | regenerated (line numbers moved; 3281/42 unchanged) |
| `LEGOLAND/scrolltick.c` | `g_view_*` -> `g_scroll_slack_*` (8 names) |
| `LEGOLAND/fpui2.c` | `g_popup` -> `g_popup_info` |
| `LEGOLAND/logflume2.c`, `screencb6.c`, `unref4.c` | `g_ui_flags` -> `g_edit_cursor_flags` |
| `LEGOLAND/coaster.c` | `g_tile_sprites` -> `g_roads_data` |
| `LEGOLAND/fpui4.c` | `g_snd_click` -> `g_snd_close` |
| `LEGOLAND/printlist.c` | `g_screen` -> `g_surface_78` |
| `LEGOLAND/bswater.c` | `g_carousel_bnv` -> `g_carousel_run_s`, `g_carousel_zspr` -> `g_carousel_zspr_s` |
| `LEGOLAND/ridecb1.c` | `g_carousel_zspr` -> `g_carousel_zspr_s` |
| `LEGOLAND/ridecb3.c` | `g_bz_bnv` -> `g_bz_bnv0` |
| `LEGOLAND/sysmisc2.c` | `g_view` -> `g_view_cfg` |
| `LEGOLAND/roads.c` | `g_road_tiles` -> `g_road_tile_codes` |
| `LEGOLAND/schoolcar.c` | `g_frame_ticks` -> `g_zb_frame_ticks` |
| `LEGOLAND/screens3.c` | `g_lls_accept_on_report` -> `_tut` |
| `LEGOLAND/advisor.c` | `g_avi_open_count` -> `g_advisor_avi_open_count` |

No `portable/**`, no `docs/HANDOFF.md`, no `tools/verify.py` run.

## 7. Open, for whoever picks it up

1. **P3's dual-address sweep belongs in the round gate**, beside
   `portable/tools/extern_sweep.py` — which reports "the class is closed" and
   was blind to all seventeen, because it sweeps a different shape. Ten lines,
   no build, no assets. PORT-A owns `portable/tools/`, so this lane did not add
   it; the script is in `docs/lanes/scope-port-p3.md` §3.3 and was run from
   `scratchpad/port-m15-dualaddr.py`.
2. **The inverse invariant is not closed**: this lane made every NAME map to
   one ADDRESS, not every address to one name. 0x008003e8 is still
   `g_8003e8` in castleobj.c and coaster.c as well as `g_edit_cursor_flags`;
   0x004bcbf4 wears nine names on purpose. The first is worth tidying, the
   second is deliberate.
3. `g_snd_click` at 0x004b92c0 is **"Button04.wav"**, not a click; 0x004b929c
   (now `g_snd_close`) is **"Click01.wav"**. Seven files carry the less
   accurate label. Not churned here — it is a tree-wide relabel, not a defect.
4. `g_clear_sfx` (eventtick.c:151, 0x004b92fc) is commented "the CLEAR
   demolition sample"; the image says entry 17 is **"inventory slide out.wav"**.
   A comment fix for a later round.
5. Lesson 5 now needs a play lane: the gardeners hire, so
   `NEEDIN "FLOWERS" 5/10/15` can be attempted for the first time.
