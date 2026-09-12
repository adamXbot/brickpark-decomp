# Scope PORT-B12 — the doubled typed character, and the undrawn visitors

> **PORT-B12 — Status: DONE (2026-09-12)** — branch `scope/PORT-B12`, cut from
> the PORT-P1 merge (`82cd767d`). Files owned:
> `portable/src/hostwin/{ddraw,user32,gdi32,dinput,winmm,dsound,avifil32,
> msacm32,ll_ttf,ll_audio,ll_font}.c`, `portable/src/browser/**`,
> `portable/cmake/browser.cmake`, additions to
> `portable/hostwin/include/ll_host.h`. `LEGOLAND/*.c` is READ-ONLY for this
> lane and **nothing in it is changed on this branch**, so the VC6 gates have
> nothing to check. Brief: `docs/SCOPE_PORT_WAVE.md`.

**Result in one line: BOTH findings are real, NEITHER is shim-side, and the
appraisal screen has been reached — `:PRAISEME` fires, `RunAppraisalScreen`
(the 8,085-instruction WIP body nobody had seen run) draws its REPORT notepad
with the inspector minifigure, pages, closes through its GoBack icon and hands
the park back with the sim resuming. P1-5 is one `memcpy` on OVERLAPPING memory
in `LEGOLAND/input.c` — undefined behaviour that VC6 forgives and LLVM does
not; the keyboard, the press latch and the game's edge detector are all
correct. P1-6 is `Draw3DPersonModel`: the visitors reach the print list and
`Render3DPerson` is entered for 27 of 30 of them every frame.**

| finding | verdict | where | owner |
| --- | --- | --- | --- |
| **P1-5** doubled typed character | **REAL — root cause proved, fix proved** | `LEGOLAND/input.c:368` `memcpy(g_type_buf, g_type_buf + 1, 19)` | **PORT-M**, one line (§2.5) |
| **P1-6** visitors not drawn | **REAL — confirmed and narrowed to one function** | `LEGOLAND/person3d.c:1092` `Draw3DPersonModel` (WIP 63.3%) | a matching / render lane (§3.5) |
| P1-8 appraisal screen | **REACHED** — opens, pages, closes, sim resumes | — | closed by P1-5's fix |
| B12-3 a second overlapping `memcpy` | latent, does not miscompile today | `LEGOLAND/narration2.c:429` | PORT-M, same patch (§2.6) |
| the hidden-tab throttle vs. the audio context | **not a defect** | — | — (§4) |

---

## 1. What was measured, and how

`portable/build-wasm` served on 8825, `legoland.html?args=-nointro+WINDEBUG&beat=1000`,
this lane's own tab, virgin IDBFS, driven to a running FREE PLAY park with
PORT-P1's prelude (`P1.start()`; the three profile pokes are P1's, unchanged).
The throwaway build of §2.5 was served separately on 8826 so the honest build
stayed up. Replays: `portable/src/browser/replays/b12-01-*.js`, `b12-02-*.js`.

Front-end hashes reproduced P1's table exactly on both builds — cold load
`0x093021ac`, slot 1 `0x19edb1f8`, `adam` typed `0x319a6a6c`, Accept
`0x46e23314`, title with Free Play lit `0x1bfbc6a5` — which is also the proof
that §2.5's one-line change moves no front-end pixel.

---

## 2. P1-5 — the doubled character is an OVERLAPPING `memcpy`

### 2.1 It is not the keyboard, and it is not the press latch

P1's prime suspect was PORT-B6's press latch in `dinput.c` re-asserting a
still-held key. Three measurements, all in `b12-01-*.js`, close that off:

| layer | what was measured | result |
| --- | --- | --- |
| the DOM | `LL_DEBUG.push` wrapped, every accepted key event logged, while `llType('ABCDEFGHIJKLMNOP')` runs | **16 keydowns, 16 keyups**, DIKs `1e 30 2e 20 12 21 22 23 17 24 25 26 32 31 18 19` = A..P, one event each |
| the shim | the GAME's own `g_key_state[256]` sampled at **1 ms** for the whole 16-character run | **32 transitions: 16 rises, 16 falls, strictly alternating.** Never a second rise inside a press |
| the game | `g_typed_key_prev` (x86 `0x00668de4`; `g_type_buf + 0x60` in the portable layout) while one key is held | **exactly one entry set, to `0x80`**, at the right KEY MAP index — the array is indexed by map index, not by DIK, and `g_key_map` (read out live, 2 bytes per entry, 59 entries) puts A at 4, F at 9 and M at 16 |

`latch_press` is the only writer that sets `down`, and it runs only from an
`EV_KEYDOWN` record — of which there are exactly sixteen. The latch is correct
and **nothing in `dinput.c` was changed**.

### 2.2 The defect is in the ring's SHIFT, driven with no keyboard at all

`UpdateControllerFromKeyboardData` (input.c:361, x86 `0x00473c10`):

```c
ch = GetTypedChar();
if (ch != 0) {
    memcpy(g_type_buf, g_type_buf + 1, 19);   /* <-- src and dst OVERLAP */
    g_type_buf[19] = ch;
```

Write twenty distinct bytes over `g_type_buf`, type ONE character, read back.
Nothing else runs between the two reads. Live, in the park:

```
before   0123456789abcdefghij
after    02345678aabcdefhiijP      <- three bytes duplicated, three lost
expected 123456789abcdefghijZ
```

Widened to a 36-byte window (which is also how the ring's true base,
`g_type_buf + 0`, was pinned — P1's replay 04 is off by one, see §2.7):

```
before ABCDEFGHIJK|LMNOPQRSTUVWXYZabcde|fghij
after  ABCDEFGHIJK|MNOPQRSUUVWXYZbcc de|fghij   (7)
                          ^^        ^^^
```

Inside the 20-byte ring, output index 7 took input index 9, and outputs 14 and
15 both took inputs 16 and 17 — **three wrong bytes, at fixed ring offsets**,
and nothing outside the ring is touched.

### 2.3 Why: `memcpy` on overlapping memory is undefined behaviour

VC6's x86 `memcpy` copies forward and, for `dst == src - 1`, happens to produce
exactly the shift the game wants. LLVM sees a **constant** length and inlines
the copy as wide load/stores that are allowed to assume no overlap. Reduced to
eleven lines and compiled with the same toolchain, outside the game entirely:

```c
for (i = 0; i < 20; i++) buf[i] = 'A' + i;
memcpy(buf, buf + 1, 19);      /* exactly input.c:368 */
buf[19] = '7';
```

```
$ emcc -O2 -o t.js t.c && node t.js
got      BCDEFGHJJKLMNOPQRST7
expected BCDEFGHIJKLMNOPQRST7
```

and across sizes (`/tmp/port-b12-memcpy-sizes.c`, scratch):

| `memcpy(b, b+1, N)` | 8 | 15 | 16 | **19** | 20 | 31 | 32 | 63 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| wrong bytes | 0 | 2 | 1 | **3** | 0 | 1 | 1 | 8 |

**Three wrong bytes at N = 19** — the same count the live game shows. It is
size- and alignment-dependent, which is why it looks arbitrary and why a size
the game does not use (20) comes out clean.

### 2.4 Why it looked like "every ~7th character", and why it kills every cheat

It is not periodic in time or in keystrokes. It is periodic in the RING: a
fixed set of ring offsets is corrupted on every shift, and a ring that moves
one byte per character walks each character past one of them every seven
characters. Hence the doubled pair always seven apart, the phase differing per
run, and the determinism. Measured, live, three ways:

```
typed ABCDEFGHIJKLMNOP  ->  .QQABCDEFFGHIJKLMMNO    F and M doubled, P LOST
typed ABCDEFGHIJKLMNOP  ->  .FMABCDEFFGHIJKLMMNO    the same two, again
typed ADAM + QQXXZZQQ   ->  .......ADDAMQQXXZZZQ    D doubled, one Q lost
```

(P1 did not record the **losses**; they are the same defect seen from the other
side, and they matter for the same reason.)

input.c:370-440 matches each cheat with a `strnicmp`/`memcmp` against the
ring's TAIL **at a fixed offset** — `&g_type_buf[11]` for `:PRAISEME`,
`[14]` for `:THEME`, `[4]` for `:ILIKETOTRAVEL`, and so on. A cheat word is
5-15 characters long, so it is essentially certain to straddle a corrupted
offset, and one extra or missing character puts the word off the offset the
comparison uses. **Every cheat in the game is dead**, exactly as P1 said — and
with them the only route into `RunAppraisalScreen` in a free-play park.

### 2.5 The fix — one line, owner PORT-M

`LEGOLAND/*.c` is read-only for this lane, so this is **not committed here**.
The recipe (the same shape PORT-B7 used for its B1, under the usual guard,
never between a `// FUNCTION:` marker and its signature; `audit.py` + `relocs.py`
on the file):

```c
#ifdef LEGOLAND_PORTABLE
        /* memcpy on OVERLAPPING memory is undefined behaviour. VC6's x86
         * memcpy copies forward and produces the intended one-byte shift;
         * LLVM inlines this constant-size copy as wide load/stores that
         * assume no overlap and duplicates three of the nineteen bytes, so
         * every ~7th typed character is doubled and no cheat can ever match
         * its fixed tail offset (PORT-B12, docs/lanes/scope-port-b12.md).
         * memmove is the same copy with the overlap guarantee; the #else
         * arm's bytes cannot move. */
        memmove(g_type_buf, g_type_buf + 1, 19);
#else
        memcpy(g_type_buf, g_type_buf + 1, 19);
#endif
        g_type_buf[19] = ch;
```

**Proved on a throwaway build** (`portable/build-b12tw`, built clean from that
one edit, reverted immediately afterwards and never committed — `git status`
on this branch shows no `LEGOLAND/` change):

```
typed QQXXZZQQ          ring "........ADAMQQXXZZQQ"   exactly the 12 typed
typed ABCDEFGHIJKLMNOP  ring "ZZQQABCDEFGHIJKLMNOP"   all 16, in order
typed :PRAISEME         ring "FGHIJKLMNOP:PRAISEME"   ":PRAISEME" at [11..19]
```

`[11..19]` is precisely the window `strnicmp(":PRAISEME", &g_type_buf[11], 9)`
compares. **The cheat fires** (§5).

There is no shim-side fix. The miscompiled copy is *inlined*, so a `memcpy` in
`msvcrt.c` would never be called; and the only build-level lever
(`-fno-builtin-memcpy` on the game TUs, which would route the call to
`memory.copy`, whose wasm semantics are `memmove`'s) lives in
`portable/CMakeLists.txt`, which no lane in this wave may edit. It is also the
wrong fix: it would paper over the UB tree-wide instead of correcting the one
site that has it.

### 2.6 The class, swept tree-wide

Every `memcpy`/`memmove` in `LEGOLAND/*.c` whose two arguments name the same
base object (`/tmp/port-b12-memsweep.py`, scratch):

| site | call | state |
| --- | --- | --- |
| `input.c:368` | `memcpy(g_type_buf, g_type_buf + 1, 19)` | **MISCOMPILED today** — §2.2 |
| `narration2.c:429` | `memcpy(g_narr_queue, g_narr_queue + 1, 19 * sizeof(int))` | the same UB; at 76 bytes LLVM emits a real call today and it comes out correct (measured). **Latent** — it is one inlining decision away. Fix it in the same patch |
| `ridecb5.c:767` | `memcpy(&n, &n, 4)` | `dst == src`, a no-op; harmless, and worth a comment rather than a change |

Three, and that is the whole of the class in the tree. The sweep only catches a
same-named base, so a copy through two pointers into one object would not show;
no such site was found by eye in the person/sprite paths.

### 2.7 Two corrections to PORT-P1's notes

* **The suspect.** `p1-04-*.js`'s "OWNER: PORT-B first (`dinput.c`) … the prime
  suspect is the PRESS LATCH" is wrong. §2.1 is the disproof, and the replay's
  own §"the keyboard state itself is correct" was already the first half of it.
* **The ring base.** `p1-04-*.js` computes `ringStart = base - (20 - 9)` from an
  8-character marker it calls 9 long, and the marker it searches for is itself
  corrupted by the defect. The base is one byte further on. `b12-01-*.js`
  searches for the marker's first FOUR characters and uses `base - 12`; pinned
  independently against a 36-byte window (§2.2), `g_type_buf` is the 20 bytes
  at `2571568` in this build and `g_typed_key_prev` is at `+0x60`.

---

## 3. P1-6 — the visitors reach the print list and stop

### 3.1 Confirmed, with a measurement that does not depend on knowing where a bloke should be

`RenderPeople` (renderlist.c:148) skips a bloke whose `flags62 & 0xa0`. So:
set `flags62 |= 0x80` on the whole chain, and if any bloke were drawn the frame
must change by hundreds of pixels. Three noise baselines bracket it:

| frames compared | pixels differing | bounding box |
| --- | --- | --- |
| hidden vs hidden (noise) | 1512 | (492, 186)-(615, 312) |
| hidden vs hidden, 2 s apart (noise) | 1399 | (492, 187)-(615, 312) |
| shown vs shown (noise) | 1558 | (515, 186)-(614, 286) |
| **hidden vs shown** | **1605** | **(515, 186)-(614, 286)** |
| **shown vs hidden** | **1516** | **(517, 186)-(614, 286)** |

Every diff is the same box and the same size: the park's own animation (the
entrance flag, the two LEGOLAND banners). **Thirty blokes are worth nothing.**
P1-6 stands.

### 3.2 The print list is NOT the break

`SortPerson` (blokeai.c:218) bump-allocates a 0x20-byte node, stamps type
`0x2000` at +0x0c and the `Person3D` at +0x1c; `DrawAndClearPrintList`
(printlist.c:614) resets the cursor, not the arena, so the last frame's nodes
survive. Scanning for the signature — a word `0x2000` whose +0x10 neighbour is
a live `Person3D` — finds **23 nodes** with real depth keys, contiguous at a
0x20 stride.

### 3.3 `Render3DPerson` IS entered — 27 of 30, every frame

The probe is `Render3DPerson`'s own first three statements (rin.c:537-539):

```c
p->scale_x = 1.0f;  p->scale_y = 1.0f;  p->scale_z = 1.0f;
```

They run **before** the `IntersectRect` and `GetVideoSurface` early-outs. Poke
`scale_x = 7.0f` into all thirty persons, wait 400 ms, read back: **27 come
back 1.0**. The print list is threaded, the walk reaches the person nodes, and
the person renderer is entered. (The three that do not are the ones whose
`flags62 & 0xa0` was set that frame.)

### 3.4 The records are healthy, and so is everything shim-side on the path

```
flags 0xb  (276,223)  frame 6  faces 20077120  depth 262  tint 0xff  ydepth 0
           matrix [0, 0, 65536, 0, -65536, 0, -65536, 0, 0]
flags 0x7  (186,189)  frame 6  faces 20082816  depth 234  tint 0xff  ydepth 0
           matrix [46341, 0, 46341, 0, -65536, 0, -46341, 0, 46341]
```

A proper 16.16 rotation (65536 = 1.0, 46341 = cos 45), a real frame index, a
non-null face table, `depth == sy + 45`. Ruled out on the shim side:

* `user32.c`'s `IntersectRect` computes into locals before storing, so
  rin.c:549's `IntersectRect(&r, &v, &r)` — which **aliases dst and src2** —
  is safe. `g_clip_rect` reads `(0, 0, 640, 480)` in the park and
  `g_render_clip` `(0, 0, 639, 479)`.
* the locked surface is the one every sprite in the frame is blitted into, and
  the terrain, the buildings, the banners and the entrance all appear.
* no trap all session: `llStats().dead` null, `traps` `[]`.

### 3.5 So it is `Draw3DPersonModel`, and here is where to look

`Draw3DPersonModel` (person3d.c:1092, x86 `0x00440a30`) is the **63.3% WIP
body** (648/1023 instructions) and is the only thing between §3.3 and pixels.
The first place to look is its portable arms, because they are hand-written
replacements for the original's inline asm and they are all in the geometry:
`FMUL` / `FMULA` / `FMULP` / **`TOFIX`** / `SHADE` (person3d.c:529-596) and
the `ll_nrm` local (person3d.c:1133, 1414, 1482). `TOFIX` is what turns the
0.447 isometric foreshortening and the three scales into 16.16 through
`LL_FISTP`; **a model whose scale comes out 0 rasterises to nothing, silently**,
which is exactly the symptom — no trap, no pixels, healthy inputs.

**Still owed, and cheap: the same probe on the TUTORIAL park.** PORT-M10's
merge note says blokes walk the paths there. If they do not draw there either
on this build, this is a regression after M11 and the bisect is short; if they
do, the difference is in what `StartFreePlayPark` (uimisc2.c:404) sets up.
`b12-02-*.js` steps 1-4 are the whole probe and need no free-play state. This
lane ran out of clock before driving the tutorial (it is a ten-minute build-a-
ride-and-a-path walk before any visitor arrives at all).

### 3.6 Two corrections to PORT-P1's replay 05

* **The overlay rings are in the wrong place.** `p1-05-*.js` draws them at
  `(sx, sy)`, which is the **top-left corner of the person's 160x120 render
  window**, not the figure: rin.c:541-544 builds `v = (sx, sy, sx+0xa0,
  sy+0x78)` and person3d.c:107 centres the model at `ox = (80 << 16) - width/2`,
  `oy = (90 << 16) - height/2`. A drawn bloke appears around `(sx + 80,
  sy + 90)`. §3.1's measurement does not depend on knowing that.
* **"eleven of sixty are inside the viewport" counts the wrong blokes.** P1
  walked the whole chain. Thirteen of the thirty carry `flags62 & 0x20` and are
  never sorted at all (they are inside rides and queues) — and those are
  exactly the ones whose *stale* screen positions cluster on camera. Of the
  seventeen actually submitted, **two or three** are inside the viewport at any
  instant. The finding is unchanged; the number is not evidence of anything.
* One more reading that costs nothing and is easy to mis-take: there **is** a
  minifigure drawn near the entrance on the free-play map, holding a flag on a
  brick base. It is scenery — hiding the whole bloke chain leaves it untouched.

---

## 4. The rest of what P1 touched in the shim

| what | verdict |
| --- | --- |
| **the hidden-tab throttle vs. the audio context** | **not a defect.** Through the whole session — the front end, the picker, the park, the appraisal screen — `llAudio()` reported `state: "running"`, 16 voices created, 16 plays, **0 blocked, 0 refused, 0 underruns**. Chrome throttles timers in a background tab (P1 §3's last row, which stands) but does not suspend an already-running `AudioContext`, and `ll_audio.c`'s one-shot capture-phase resume listeners had already cleared the autoplay gate. Nothing to add; a `visibilitychange` handler would be a change with no symptom behind it. Worth recording for the next lane: `document.hidden` read **false** in this lane's tab, so P1's "the Browser pane reports `document.hidden === true` even after `tabs_select`" is not universal — check it per session rather than assuming it |
| **the SAVE screen's text entry** | not affected by P1-5 and not separately broken. The profile and save-game name editors read `GetInputChar` (input2.c:312) into `g_key_prev`, a **different** 59-byte prev array from the cheat ring's `g_typed_key_prev`, and they APPEND to a buffer — they never shift one, so the only overlapping copy in the input path cannot reach them. PORT-B7 measured that path correct and this lane typed `adam` into it four times over two builds with the name reading back exactly |
| **the MAP screen's viewport rect** | not re-tested; P1 recorded the map drawing correctly (`0xca0792a4`) and this lane found nothing on the way past it |

---

## 5. The appraisal screen — reached, exercised and closed

On the §2.5 build, in a running free-play park (`gameMode` 3, 30 visitors):

```js
await llKey('ShiftLeft', '');     // ':'  (GetTypedChar maps DIK 0x2a/0x36/0x3a to -10)
await llType('PRAISEME');
```

| step | evidence |
| --- | --- |
| the ring | `FGHIJKLMNOP:PRAISEME` — `:PRAISEME` at bytes `[11..19]`, the exact offset input.c:421 compares |
| the cheat fires | `g_instant_appraisal = 1`; `AppraisalDueTick` (goalstate.c:243) takes it on the next tick |
| the sim pauses | `llPark().simFrame` **frozen at 3801** across repeated reads — `PauseGameTimer()` ran |
| **the screen draws** | the REPORT notepad with its spiral binding, "Objectives: Congratulations you have built a thriving Park!", and the LEGO inspector minifigure in his peaked cap holding a blue-and-yellow pencil. 96.4% non-black, 41 text rows of ink between y=60 and y=468 |
| it animates | the frame hash cycles over a small set (`0x241ce2ac` `0x37d55f03` `0xf3d62efc` `0xbf8445e5` `0x14a77b44`), repeating across minutes |
| the icons answer | hovering the GoBack icon at game (525, 370) — appraisal.c:517 puts it at (0x1fb, 0x161) — gives `llSel().hit.typeHex` `0x2`; the NextPage icon at (441, 430) is correctly greyed (one page) |
| **it closes** | a click on GoBack leaves the `while (g_report_open)` loop, `FreeAppraisalScreenSprites` / `SetInGameIconHandlers` run, the park returns and **the sim resumes**: simFrame 3801 -> 3869 -> 3912 |
| traps | `llStats().dead` **null**, `traps` `[]`, over the whole run |

The screen hash is not quoted as a single number on purpose: it animates, so
the assertion is the cycle above plus `llPark()`, per PORT-B10's rule.

**P1-8 closes with P1-5.** Nothing else is needed: no `LL_DBG_TABLE` row, no
scripted level.

---

## 6. Gates

| gate | result |
| --- | --- |
| `emcmake cmake -S portable -B portable/build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release -DLL_ILP32=ON` (CLEAN dir) | exit 0 |
| `ninja -C portable/build-wasm` + the six extra targets | exit 0 |
| wasm `ctest` | **24/24** |
| native `cmake -S portable -B portable/build -G Ninja` + `legoland_tests` + `ctest` (CLEAN dir) | exit 0, **17/17** |
| the page, `?args=-nointro+WINDEBUG&beat=1000`, two builds, four cold loads | `llStats().dead` **null**, `traps` `[]` throughout |
| `LEGOLAND/*.c` touched on this branch | **none** — `git status` clean of it; no `verify.py` / `audit.py` / `match.py` / `relocs.py` run is owed |
| shim source changed | **none.** Both findings are game-side; this lane changed no `.c` in `portable/src/hostwin/` and no behaviour in `portable/src/browser/` |

Files added: `docs/lanes/scope-port-b12.md`,
`portable/src/browser/replays/b12-01-typed-char-doubling-is-an-overlapping-memcpy.js`,
`portable/src/browser/replays/b12-02-visitors-reach-the-print-list-and-stop.js`,
a section in `portable/README.md`, and the status line in
`docs/SCOPE_PORT_WAVE.md`. **`index.html` and `main.c` were NOT edited** (the
brief allowed minimal additive edits; none turned out to be needed — every
probe above runs from the console against hooks PORT-B7/B10/P1 already added).

Scratch, not committed: `/tmp/port-b12-memsweep.py` (§2.6),
`/tmp/port-b12-memcpy-ub.c` and `/tmp/port-b12-memcpy-sizes.c` (§2.3),
`/tmp/port-b12-build-wasm.sh`, `/tmp/port-b12-build-throwaway.sh`,
`/tmp/port-b12-serve.sh`, `/tmp/port-b12-serve-tw.sh`.

## 7. Reproducing

```bash
PY=$HOME/.venvs/legoland/bin/python
emcmake cmake -S portable -B portable/build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DLL_ILP32=ON -DPython3_EXECUTABLE=$PY
ninja -C portable/build-wasm legoland_browser legoland_browser_named
(cd portable/build-wasm && python3 -m http.server 8825)
# http://localhost:8825/legoland.html?args=-nointro+WINDEBUG&beat=1000
```

then, in the console: `p1-00-prelude.js`, `P1.start()`, and either
`b12-01-*.js` or `b12-02-*.js`. The eleven-line standalone proof of §2.3 needs
no game at all:

```bash
printf '%s\n' '#include <stdio.h>' '#include <string.h>' 'static char b[20];' \
  'int main(void){int i;for(i=0;i<20;i++)b[i]="A"[0]+i;memcpy(b,b+1,19);b[19]=55;' \
  'printf("%.20s\n",b);return 0;}' > t.c
emcc -O2 -o t.js t.c && node t.js      # BCDEFGHJJKLMNOPQRST7, not BCDEFGHIJ...
```
