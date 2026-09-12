# Scope PORT-P1 — PLAY FREE PLAY: what breaks

> **PORT-P1 — Status: DONE (2026-09-12)** — branch `scope/PORT-P1`, cut from the
> PORT-M14 merge (`cc6e926d`). A TESTING lane: it fixes nothing. Every finding
> below has a replay that reproduces it from a cold load, a frame hash, the
> measurement that proves it, and an owner.
> Brief: `docs/SCOPE_PORT_WAVE.md`.

**Result in one line: free play is playable end to end — the picker, the park,
the four theme menus, every ride and shop and cart built and rendering, the
map, the options screen, save, a full page reload and load — at 35.0 fps with
sixty visitors in the chain, with ZERO traps over roughly 60,000 presented
frames. Two real defects and one open question came out of it, and three
things that look like defects are the game's own rules.**

---

## 1. Free play, end to end

Served `portable/build-wasm` on 8820, `legoland.html?args=-nointro+WINDEBUG&beat=1000`,
own tab, virgin IDBFS. Replays: `portable/src/browser/replays/p1-*.js`.

| # | file | what it reproduces |
| --- | --- | --- |
| 00 | `p1-00-prelude.js` | the helpers + `P1.start()`: cold load -> a running free-play park |
| 01 | `p1-01-freeplay-start.js` | the route, screen by screen, with the three profile pokes explained |
| 02 | `p1-02-build-a-park.js` | build one of every kind, every panel, save, reload, load back |
| 03 | `p1-03-legoland-tab-hides-itself.js` | **P1-4**, isolated to a single click |
| 04 | `p1-04-typed-char-ring-doubles.js` | **P1-5**, with the cheat it kills |
| 05 | `p1-05-visitors-are-not-drawn.js` | **P1-6**, with a ringed overlay |

### The route, and the screens reached

| step | screen | hash | `llPark()` | fps | dead / traps |
| --- | --- | --- | --- | --- | --- |
| cold load | PLAYER DETAILS | `0x093021ac` | — | 33.9 | null / 0 |
| slot 1 | the name editor | `0x19edb1f8` | — | — | null / 0 |
| type `adam` | name live in `g_temp_profile` | `0x319a6a6c` | — | — | null / 0 |
| Accept | **TITLE screen** (`g_screen_mode` 1) | `0x46e23314` | money 10000 | 33.7 | null / 0 |
| *(poke `level_done[5]`, rebuild)* | title, **Free Play lit** | `0x1bfbc6a5` | — | 34.9 | null / 0 |
| Free Play (215,70) | **the picker** (screen 3), empty | `0xfede103c` | — | 34.2 | null / 0 |
| *(poke `g_profile_unlocked`, rebuild)* | the picker, **populated** | `0x82a5cf1c` | — | 34.2 | null / 0 |
| tick 12, Accept | **THE FREE PLAY PARK** | `0x2f506be4` | money 10000, visitors 5 -> 30, limit 30 | 32.7 | null / 0 |
| LEGOLAND tab | the object list, priced | `0x23a47ca0` | money 10000 | 33.8 | null / 0 |
| place a ride | Earth Slide standing on its pad | `0xaee6a5c8` | money **unchanged** | 33.6 | null / 0 |
| MAP button | **the whole park map** | `0xca0792a4` | gameMode 3 -> 1 | 34.7 | null / 0 |
| OPTIONS | screen 5, three volume rows | `0xa78c552d` | gameMode 2, screen 5 | 34.6 | null / 0 |
| SAVE | screen 4, "Save Game / adam" | `0x815a37a8` | — | — | null / 0 |
| slot 1 = `freeplay1` | 759,397-byte `.sav`, 272-byte `.sh` | `0x33e6732c` | — | 34.4 | null / 0 |
| *(full page reload)* | PLAYER DETAILS | `0x311d0e24` | — | — | null / 0 |
| Load_on_Title -> Accept | **the park, restored** | `0xa108848e` | visitors 30, limit 30 | 32.9 | null / 0 |

The four front-end hashes reproduced **exactly** across three separate cold
sessions. Anything with a help bubble or a blinking icon on it does not repeat
— the title screen's New bubble blinks on a wall clock, so a hash taken 300 ms
later differs. Treat `llPark()` as the assertion, as PORT-B10 says.

### What free play actually is

`TitleFreeInput` (screens3.c:1203) sets `g_screen_mode = 3`; `InitScreens`
(mapscreen.c:434) runs `InitFreePlayScreen` (fpui2.c:883). That screen is a
four-column picker — `FreePlayObjectList` at (0x2a,0x41) LEGOLAND+COMMON,
(0xbb,0x41) WESTERN, (0x14c,0x41) CASTLE, (0x1dd,0x41) ADVENTURERS, each 0xec
tall — over a **20,000-brick budget** (`g_freeplay_progress` against 0x4e20,
uimisc3.c:897) drawn by `RenderFreePlayBar` (fpui.c:390). Accept
(`FreePlayAcceptInput`, uimisc.c:678) sets `g_save_type = 2` and calls
`StartFreePlayPark` (uimisc2.c:404), which loads the level database
**`FreePlayTest.txt`** and calls `sub_457870(0)` — the brick lock.

`FreePlayTest.txt` (Scripts\\, 728 bytes) is `[INIT]` only, no objectives:

```
CAPACITYSCALE Path 5 / Ride 100 / Shop 15 / FoodShop 20   (caps 10/20/10/10)
FEATURE CapacityCalc 0 ; MAXCAPACITY 200 ; MINCAPACITY 30
MAP "freeplay big map" ; WORKERS 1, 1 ; Feature AutoRepair 1
EntranceFee 0 ; Lookat (190, 80) ; BREIFINGFILE "FreePlay.txt"
```

So: one gardener and one mechanic, auto-repair on, free entry, visitors up to
30, and **nothing to achieve** — which is why the appraisal never fires by
itself (§4, P1-8) and why no `LINK` goal is ever evaluated (§3, note 4).

### A correction to the brief

The brief says the free-play route is on "the park-advert screen … LEGOLAND
California / Windsor / Billund". It is not. That is front-end screen **9**
(`InitScreen9`, mapscreen3.c:150), reached from the title screen's **Movie**
bubble, and its three park buttons do exactly one thing each: `SetPointer(0)`,
`PlayMovie("California.avi" | "Windsor.avi" | "Billund.avi", 1, 1)`,
`SetPointer(6)` — one source compiled three times, differing in the pushed
string alone (uimisc3.c:42). They are adverts. Free play is the title screen's
own **Free** bubble at (0x9a, 8).

---

## 2. The free-play catalogue

`g_fp_table` (x86 0x004bdeb8) is 0x86 rows of `{u8 id; char* name; int cost; int}`.
Read out of `original/legoland.exe`, this is everything free play can offer and
what each costs against the 20,000 budget:

```
  0 HEDGE 32          33 CACTUS 3 18         67 SHERIFF 88        101 CASTLE OBJ 1770
  1 PALM TREE 27      34 KEEP 23             68 BANK 84           102 SQUARE_TRACK_HEIGHT_PATH 12
  2 BARREL 14         35 BARROW 15           69 GENERAL STORE 79  103 SQUARE_TRACK 12
  3 TREE 1 29         36 FOUNTAIN 1 583      70 JAIL CELL 30      104 SQUARE_TRACK_HEIGHT 12
  4 MINI PINE TREE 23 37 FOUNTAIN 2 210      71 GOLD RUSH 5130    105 SQUARE_TRACK_HEIGHT_0 12
  5 FLOWERS 19        38 FOUNTAIN 3 150      72 FORT 1756         106 XXROLLER COASTER TRACK 0
  6 SHRUB 13          39 XXCASTLE_DUMMY 0    73 SALOON 181        107 LOG FLUME ENTRANCE 2881
  7 WATERPUMP 59      40 ANUBIS 18           74 TEMPLE 1279       108 LOG FLUME CSAW 997
  8 CASTLE TREE 1 28  41 SPHINX 105          75 CATAPULT 1303     109 LOG FLUME TUNNEL 716
  9 CASTLE TREE 2 33  42 EGYPTIAN STATUE 44  76 OCTOPUS CAFE 373  110-113 LOG FLUME SPECIAL CORNER 1-4
 10 CASTLE TREE 3 24  43 OBLISK SMALL 15     77 SPACE TOWER RIDE 408      411/394/493/470
 11 CASTLE WELL 43    44 BIG OBLISK 22       78 COPTERS 2117      114 LOG FLUME HOLD UP 1854
 12 CROCTREE 264      45 ABU SIM 489         79 CAROUSEL 7958     115 LOG FLUME DROP 1162
 13 MONKTREE 167      46 PYRAMID 1 616       80 PLANE RIDE 7116   116 LOG FLUME TRACK 503
 14 PALM TREES 2 85   47 PYRAMID 2 174       81 BALLOONZ 3403     117 MINILAND SAN FRANCISCO 3172
 15-18 PLANTS 1-4     48 DINO BIG 383        82 SAFARI RIDE 4544  118 MINILAND LONDON 2426
       17/35/17/40    49 DINO MINI 765       83 ROPE CLIMB 0      119 MINILAND FRANCE 1981
 19-22 FLOWER BED 1-4 50 DINO SMALL 329      84 EARTH SLIDE RIDE 242     120 MINILAND HOLLAND 2198
       23/25/23/26    51 T-REX 1277          85 SPIDER RIDE 3183  121 MINILAND ITALY 1079
 23 BUSH 1 30         52 SMALL POWER STATION 586  86 SPINNING BARRELS RIDE 3213
 24 BUSH 2 45         53 CRYSTAL POWER STATION 886 87 JOUST 4422  122 MINILAND WASHINGTON 2863
 25-27 WEST BUSH 1-3  54 LEGO SHOP 1 247     88 TEMPLE SLIDE 785  123 MINILAND INDIA 1136
       16/17/16       55 LEGO SHOP 2 244     89 DRIVING SCHOOL 706       124 MINILAND AUSTRALIA 3185
 28 CACTI 1 20        56 LEGO MEDIA SHOP 225 90 DRIVING SCHOOL ROADS 19  125 MINILAND NEW YORK 3632
 29 CACTI 2 129       57 RESTAURANT 1 522    91 ZEBRA CROSSING 12 126 MINILAND EGYPT 3086
 30 CACTI 3 85        58 RESTAURANT 2 894    92 DRIVING SCHOOL PUMPS 22  127 xxMINILAND DENMARK 1500
 31 CACTUS 1 18       59 EXPLORERS INSTITUTE 471  93 JUNGLE CRUISE 2111  128 WATER WORKS ENTRANCE 756
 32 CACTUS 2 17       60 SHARK CAFE 126      94-96 JUNGLE CRUISE MONKEY FISH/TREE/WATER 228/387/12
                      61 SHARK CAFE BROLLY 24     97 BOATING SCHOOL 605  129 WW CROCODILE FOUNTAIN 1113
                      62 CASTLE BBQ 456      98 BOATING SCHOOL MERMAID 43 130 WW ELEPHANT FOUNTAIN 1000
                      63 FOODCART DRINK 34   99 BOATING SCHOOL WATER 12  133 WW SHOWER 54
                      64 FOODCART FOOD 43   100 CASTLE LEVEL 1 1770      134 WW WATER BLOCK 171
                      65 FOODCART ICECREAM 32
                      66 CHUCK WAGON 33
```

**There is no toilet class.** The brief asks for one; the game does not have
one to build. Gardeners and mechanics are likewise not built: the level script's
`WORKERS 1, 1` hires them, and the player moves them through the info popup's
`PU_GardenerInput` / `PU_MechInput` (uimisc.c:555).

Built and verified rendering this session: **EARTH SLIDE RIDE** (the globe with
its spiral, and its rocking top ornament animates), **FOODCART FOOD** (a
red-and-white cart with a live steam plume), **LEGO SHOP** (the giant yellow
brick), **LEGO MEDIA SHOP**, **RESTAURANT 1** (the blue-and-white marquee),
**SHARK CAFE** (the grey shark), plus the paved clearance ring PORT-M14
described under each. Every one of them drew correctly at first sight; no
sprite was missing, misplaced or clipped.

---

## 3. What LOOKS broken and is the game's own rule — do not report these

| # | what it looks like | what it is |
| --- | --- | --- |
| **P1-1** | The **Free Play button on the title screen is inert**: no lift, no bubble help, six clicks do nothing, while New / Register / Load / Movie / Exit all work. | `InitTitleScreen` (screens2.c:1117) sets flag 0x400 on it when `HaveCurrentProfile()` is false. That function (frontend2.c:507) is `0x0048fc30`, which the original disassembles as `mov cl, byte ptr [0x80ffd9]` — **`g_cur_profile.level_done[5]`**, level six's done byte, not "is a profile selected". Free play is unlocked by finishing the first real level. Measured: with the byte 0 the tab-box diff on hover is 49 px (the cursor alone) against 3,589–15,396 for every other bubble; set it and the same hover gives 8,210 px and the bubble reads *"Start new Free Play game"*. **Owner PORT-M, as a NAME only** — `g_have_profile` / `HaveCurrentProfile` should be `g_level_done[5]` / a `LevelSixDone`-shaped name, and screens3.c:1527's `if (g_have_profile == 1)` on the tutorial screen reads much better that way. No byte moves. |
| **P1-2** | The free-play picker opens with **all four columns empty** and Accept covered. | `InitFreePlayLists` (fpui2.c:818) skips every id whose `g_profile_unlocked[200]` byte (`g_cur_profile+0x46`, x86 0x0080ffe6) is 0. A fresh profile has unlocked nothing; `UnlockFreePlayEntry` (frontend2.c:278) sets them as levels are played. The "smeared" Accept button before the first tick is `FP_Cover.lls` over the disabled icon (fpui2.c:918 sets 0x400; sysstubs.c:480 clears it on the first charge), not a rendering fault. |
| **P1-3** | In the park, **three of the four theme tabs are blank blue plates** and inert. | `UpdateThemeIconsFromProfile` (screens3.c:741) hides a tab whose `g_profile_themes[i]` byte (`g_cur_profile+0x30`, x86 0x0080ffd0) is 0. A fresh profile has `[1,0,0,0]`. Set all four and the park's toolbar reads LEGOLAND / Wild West / Castle / Adventurers. |
| — | The **money bar is blank** — no number, no bar — and building charges nothing. | `StartFreePlayPark` calls `sub_457870(0)`, which sets `g_brick_lock`, so `BricksAreLimited()` (tinystubs.c:171) is false and `RenderMoneyBar` (money.c:112) returns on its second line before drawing anything. Free play has unlimited bricks by design. `llPark().money` stays 10000 through every build. |
| — | **No path square is ever "reachable"**: all 28 path squares read flag 2 clear and `g_entrance_tile` is (0,0). | The flood fill only runs when something asks. `RefreshEntranceTile` (tinystubs.c:364) is called from `TileJoinsPathNetwork` (pathmisc2.c:183), `UpdateHelpTick` (fpui3.c:663) and the appraisal (appraisal.c:571). `FreePlayTest.txt` has no `LINK` objective, so nothing asks, and flag 2 is simply never computed. Visitors route without it. (PORT-M13's rule is unaffected and was not re-tested here.) |
| — | The whole game **stops presenting for tens of seconds** in the middle of a soak, `dead` null, `traps` empty. | **Chrome background-tab throttling**, not a wedge. Frames ran at 33.4–35.0 fps for the first ~21–25 s of every unattended soak and then dropped to *two frames per six seconds*; one `javascript_tool` call and it is back at 34 fps instantly. The Browser pane reports `document.hidden === true` even after `tabs_select`, so a hidden-pane lane cannot measure a run longer than about 20 s. **Anyone timing this build must know that, or they will file a wedge that is not there.** |

---

## 4. Findings, by severity

| # | severity | what | evidence | owner |
| --- | --- | --- | --- | --- |
| **P1-4** | **HIGH — a menu is permanently lost mid-game** | **Almost any toolbar or menu click sets flag 0x400 on `g_theme_icon[0]`, the LEGOLAND theme tab.** It stops being drawn (a blank plate where the word was) and can no longer be clicked, so the LEGOLAND build menu — shops, food carts, restaurants, power stations, Space Tower, Copters, all ten Miniland models — is unreachable for the rest of the session. The other three tabs are never touched, and the index is **always 0 whichever theme is open**: opening the *Adventurers* tab and clicking a class row in it hides the *LEGOLAND* tab. | `p1-03-*.js`. The four icons found in the heap by layout (row 379, group 0x9a, help 0x5e..0x61): legoland `0x6012` -> `0x6412`, west/castle/adv stay `0x6012`. Reproduced from PATH, QUERY, ERASER, opening another theme tab, and clicking any class row; each within 700 ms. Clearing the bit by hand restores the label and the click immediately. Thirty seconds idle with the bit cleared leaves it clear, so it is not a timer; `g_profile_themes` reads `[1,1,1,1]` throughout, so `UpdateThemeIconsFromProfile` would clear it, not set it; `FreePlayTest.txt` contains no `SetThemeIcon` primitive. | **PORT-A or PORT-M.** No writer in the recovered sources should fire here — the only three are `UpdateThemeIconsFromProfile`/`FromFlags` (screens3.c:726/741, called once from InitGameInterface's tail, bigscreens.c:1075) and `SetThemeIconEnabled` (eventgoalprim.c:262, script-only). So it is a stray store. Note that x86 **0x007fdd70 is framed twice** — `ThemeIcons g_theme_icons` (bigscreens.c:833) and `Icon* g_theme_icon[4]` (screens3.c:219) — and bigscreens.c:1001 keeps `g_western_icon` (0x00668e3c) as a second name for one of the pointers. Check `gen/extents.md` for 0x007fdd70, then run `bvstruct_sweep.py` and `port_m10_bvstruct_sweep.py` over bigscreens.c / screens3.c / eventgoalprim.c / fpui4.c / popupmisc.c. |
| **P1-5** | **HIGH — every cheat code is dead, and with it the only route to the appraisal screen** | **Roughly every seventh typed character is pushed into the cheat ring TWICE.** `ABCDEFGHIJKLMNOP` becomes `ABCDEFFGHIJKLMMNOP`; `ZYXWVUTSRQPONM` becomes `ZYXWWVUTSRQPPONM`; identical on every repeat. input.c:361 matches each cheat with a `strnicmp` against the ring's tail **at a fixed offset** (`&g_type_buf[11]` for `:PRAISEME`), so one extra character makes the comparison impossible. | `p1-04-*.js`. `g_type_buf` located in the heap by typing a marker; `:PRAISEME` lands as `:PRAISSEME` and nothing happens. The shim's key byte is correct: one DIK set, value `0x80`, which is exactly what `GetTypedChar`'s `cur & 0x80` wants — what doubles is the number of rising edges seen for one press. | **PORT-B first** (`portable/src/hostwin/dinput.c`). Prime suspect is PORT-B6's **press latch**: if it releases a frame early while the key is still physically down, the next poll re-latches and `GetTypedChar`'s edge fires twice. Fits the determinism and the periodicity. The game-side alternative to rule out is `g_typed_key_prev` (59 bytes) having the wrong extent — it fits worse, since a lost index would repeat on every held frame, not twice. |
| **P1-6** | **MEDIUM — OPEN, needs one confirmation** | **Sixty live visitors and not one of them is drawn.** The Bloke chain is 60 long, every entry a Person3D of `kind` 1 with scale (1,1,1), a valid back pointer, a well-formed prev/next chain and per-frame screen coordinates; **eleven of the sixty are inside the map viewport at any instant** and the frame shows bare path at all eleven. A 2.5 s frame diff moves ~3,400 pixels and every one of them is scenery: the ride's rocking ornament, the cart's steam, the entrance banner. | `p1-05-*.js` writes an overlay PNG with a magenta ring at each on-screen visitor — every ring is empty. No trap all session. | **A render-side lane.** Recorded OPEN, not proven: PORT-M12's lesson is that the obvious rendering complaint was the ride's own sprite. **Confirm against the TUTORIAL on this same build first** — PORT-M10's merge note says blokes walk the paths there. The join to check is the depth-sorted print list (blokeai.c:140, `PrintItem` type 0x2000 = 3D person, arena 0x007cb600 through the offset at 0x0066b5a8, consumed by printlist.c's `DrawAndClearPrintList`). Adding the print-list head to main.c's `LL_DBG_TABLE` answers it in one read. |
| **P1-7** | **MEDIUM** | **One save/load doubles the visitor chain.** Before the save: chain 30, `numVisitors` 30, `visitorLimit` 30. After a full page reload and a load through the game's own UI: chain settles at **60**, all `kind` 1, while `numVisitors` and `visitorLimit` both still read 30. | `p1-02-*.js` tail. Counted twice, 25 s apart, stable at 60. Whether a second load trebles it is **untested**. | **A save/level lane.** `StartFreePlayPark` (uimisc2.c:404) calls `AllocBlokeCounters(g_game->max_blokes)` and `EnterParkPlayMode` itself; the load path restores blokes from the save as well. Likely both, appending to `g_people_head` without clearing it. |
| **P1-8** | **LOW — not covered, and why** | **`RunAppraisalScreen` (appraisalscreen.c:381, the 8,085-instruction WIP body) could not be exercised.** `FreePlayTest.txt` sets no `APPRAISAL` deadline, so `AppraisalDueTick` (goalstate.c:233) never fires on its own, and the only other route — the `:PRAISEME` cheat, which sets `g_instant_appraisal` — is blocked by **P1-5**. | Typed `:PRAISEME` three ways (LeftShift, RightShift and CapsLock all map to ':' via `GetTypedChar`'s -10, uimisc.c:527); the ring shows `:PRAISSEME` each time and `screenMode` never changes. | **Whoever fixes P1-5 gets this screen for free.** Failing that, a lane with `g_instant_appraisal` (x86 0x00666098) in `LL_DBG_TABLE` can set it to 1 and the screen opens on the next tick. A scripted level with an `APPRAISAL` line reaches it without any of this. |
| — | **note** | The info popup on a visitor/ride/worker was **not** opened. Query mode classifies the hit correctly (`llSel().hit.typeHex` gives 0x103 on an object, 0x109 on a bare tile, 0x10b/0x10c on a work order — exactly HandleMapClick's table, gameframe.c:956), but with no visitor drawn there was nothing to click for the people popup, and the ride popup was not reached before time ran out. | — | the next play lane, after P1-6 |

---

## 5. Performance

**35.0 fps in a fully populated free-play park** — the restored save, eight
placed objects, sixty Person3D records in the chain, the full HUD, `?beat=1000`
on — measured as 176 presented frames per 5.03 s, three consecutive windows.
A second run on a freshly built park gave 168–170 frames per 5.03 s, **33.4 fps**.

`FlipPrimary` (sysmisc.c:569) spins on `timeGetTime() - g_flip_time < 0x1c`, so
the game's own ceiling is **35.71 fps**. The port is at **94–98% of it with the
park full**, which is the same place PORT-B10 measured on the empty tutorial.
**Nothing about a populated park costs a frame**, so there is no drop to
profile and no Chrome profile was taken: a profile can only show where time
goes inside 28 ms the game is going to spend waiting anyway.

The one caveat is §3's last row: **an unattended window longer than ~21 s is
measuring Chrome's throttle, not the game.** Every figure above comes from a
5 s window inside that limit.

---

## 6. Gates

| gate | result |
| --- | --- |
| `emcmake cmake -S portable -B portable/build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release -DLL_ILP32=ON` (clean dir) | exit 0 |
| `ninja -C portable/build-wasm` then `ninja ... legoland_browser legoland_browser_named` | exit 0 |
| the page, `?args=-nointro+WINDEBUG&beat=1000`, four cold sessions | ~60,000 presented frames, `llStats().dead` **null**, `traps` **[]**, no trap named all session |
| `name_trap.py` | nothing to name — no trap fired |

`verify.py`, `audit.py` and `match.py` were not run (this lane touches no
`LEGOLAND/*.c`). Files added: `docs/lanes/scope-port-p1.md`,
`portable/src/browser/replays/p1-*.js`, and the status line in
`docs/SCOPE_PORT_WAVE.md`. Nothing else was edited.
