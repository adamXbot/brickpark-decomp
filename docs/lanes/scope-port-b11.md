# Scope PORT-B11 — the game's own type, and the game's own noise

> **Status: see `docs/SCOPE_PORT_WAVE.md`.** Branch `scope/PORT-B11`, off the
> PORT-B10 merge. Files: `portable/src/hostwin/ll_ttf.c` (new),
> `portable/src/hostwin/ll_audio.c` (new), `ll_font.c`, `gdi32.c`, `dsound.c`,
> `msacm32.c`, `portable/hostwin/include/ll_host.h`,
> `portable/cmake/browser.cmake`, `portable/src/browser/index.html`.
> `LEGOLAND/*.c` untouched.

Two things the port did not have, and one open defect adjudicated.

1. **Text is the game's own typeface.** `gamedata/main/Lego.TTF` ships with the
   game and is already mounted; there is now a TrueType rasteriser in the shim
   that reads it. Every box the game sizes from `DrawTextA` now lands where the
   shipped data expects it, because the metrics are the font's own.
2. **Sound comes out.** PORT-B4's silent `IDirectSound` now has a Web Audio back
   end, and `msacm32.c` has a real MS ADPCM decoder — without which a fifth of
   the sound effects and **every** line of speech never became a sound buffer at
   all.
3. **PARK-2 is adjudicated.** The shim's surface model is innocent, proved from
   both ends. The residue has a different cause, named below, with its own
   recipe.

**The headline:** the narration plays. 29.6 seconds of speech streamed through
the port in one sitting, 426 of 440 chunks carrying real waveform, 2 underruns,
the frame rate unmoved. **Sound effects do not**, and §3 names why in one line
of somebody else's file.

---

## 1. PARK-4 — the game ships its typeface, and we were guessing

### What was wrong with guessing

PORT-B2 wrote a 6x7 bitmap face because "Lego" could not be rasterised; PORT-B10
made it proportional and re-sized its ink box. Both of PORT-B10's defects —
the Duty Manager's briefing clipped mid-word at 460 px, the money readout's
zeroes rendered as something that read as a capital A — were the SAME defect:
*our metrics are not the game's metrics*. The game does all of its own layout
from `DrawTextA` (eleven DT_CALCRECT call sites) and sizes several boxes from
SPRITE dimensions instead, so the shipped text and the shipped art only fit if
the metrics are the real font's. Guessing closed two instances. Reading the
font closes the class.

### The file

`gamedata/main/Lego.TTF`, 77,012 bytes: 15 tables, 377 glyphs, 2048 units/em,
`cmap` (3,1) format 4 and (1,0) format 0, real `glyf` outlines, `kern`, and a
v1 `OS/2`. `gpu.c`'s `InitHostSystemGPU` (0x00463700) hands it to
`AddFontResourceA` before anything draws, so every
`CreateFontIndirectA(.. "Lego" ..)` `InitScreen` makes afterwards is a request
for exactly that file. `AddFontResourceA` is therefore where `ll_ttf.c` loads
it: the call is the game telling us where its text lives.

### `ll_ttf.c`, and what is in it

Written here, ~850 lines with the header. **Not vendored** — no third-party
source and no licence file is added to the tree. (stb_truetype was the
alternative; what it would have bought is hinting and a CFF path, neither of
which this font needs, against a file whose licence would have to be carried
and whose API would still have needed the GDI height mapping below written on
top of it.)

* sfnt directory; `head` (unitsPerEm, indexToLocFormat); `hhea`/`hmtx`;
  `maxp`; `loca` (both formats); `glyf`, simple **and** composite;
  `cmap` formats 4 and 0; `OS/2` v0..v5.
* Outlines flattened — quadratics subdivided by an arc-length estimate, 2 or 3
  segments at the sizes the game asks for — and filled non-zero-winding at
  **5 sub-rows per pixel with exact horizontal coverage**, giving 8-bit alpha.
* A 512-slot direct-mapped glyph cache over a 512 KB arena. Four LOGFONTs by 95
  printable characters is the entire working set.
* Every read is bounds-checked against the file, so a truncated or hostile font
  yields empty glyphs rather than a fault.

**Not** implemented, deliberately: the `cvt `/`fpgm`/`prep` hinting bytecode,
and the `kern` table. Hinting is what would make a 1999 screenshot
pixel-identical; a bytecode interpreter is a lane of its own, and anti-aliasing
(below) avoids the dropout that is the main thing hinting would have prevented.
Kerning is *correct* to omit: GDI's `TextOut`/`DrawText` do not apply a font's
kern table unless the application asks through `GetKerningPairs`, and no call
site in the game does — so the unkerned advances here are the advances Windows
used.

### How `lfHeight` becomes a pixel size — and the evidence that it is right

GDI's `lfHeight` is **not** the em size. For `lfHeight > 0` the mapper picks the
size at which the font's CELL — `tmHeight`, ascent plus descent, internal
leading included — equals `lfHeight`. For a TrueType face that cell is OS/2's
`usWinAscent + usWinDescent`:

```
px per font unit = lfHeight / (usWinAscent + usWinDescent)
tmAscent         = round(usWinAscent  * that)      <- where the baseline goes
tmDescent        = round(usWinDescent * that)
```

Lego.TTF gives usWinAscent 2110 and usWinDescent 595 over a 2048 em, so the cell
is 2705 units and an `lfHeight` of 18 is **13.6 pixels per em** with a 14-pixel
ascent. The em is much smaller than the cell because the face's own bounding box
is tall (yMax 2110 on a 2048 em) — exactly the sort of thing a guessed face
cannot know, and exactly why PORT-B2's ink box had to be tuned twice.

**The proof.** The tutorial letter is pre-wrapped in the shipped data (movie.c's
`LoadHelpTextFor` reads `Intervals\<key>` as LINES) and uimisc2.c:472's
`PrintReportLine` draws each line **DT_SINGLELINE** into a box that is always
`0x1cc` = 460 pixels wide. Its longest line — *"forgot, we'll practice building
paths, too. There's no point in"*, 63 characters — is the tightest constraint the
shipped data places on the face. Measured through this file, live on the page
(`llFont().fonts`):

| LOGFONT | which | cell | ascent | ppem | synthetic bold | the 63-char line |
| --- | --- | --- | --- | --- | --- | --- |
| lfHeight 18, weight 600 | `g_font_18`, **the report body** | 18 | 14 | 14 | no | **383** px of 460 |
| lfHeight 20, weight 700 | `g_font_20` | 20 | 16 | 15 | yes | 487 px — clips |
| lfHeight 24, weight 700 | the default | 24 | 19 | 18 | yes | 571 px — clips |
| lfHeight 28, weight 400 | `g_font_28` | 28 | 22 | 21 | no | 597 px — clips |

383 of 460 is 83%. Exactly one of the four heights has comfortable room to
spare, and it is the one text.c:67-70's addresses say the report screen asks for
— the same one PORT-B10 had to correct PORT-B2's off-by-one to reach. **A font
file we were handed and a set of line lengths shipped as data agree.** That is
the evidence for both the font-id mapping and the cell-height mapping, and
neither was tuned to produce it.

### Synthetic bold falls out of the font, not out of a fix

Lego.TTF is a single Regular face: `usWeightClass` 400, name id 2 "Regular", no
companion bold file in the tree. `InitScreen` asks for 700, 700, 600, 400. GDI
answers a request of FW_BOLD **or more** against a non-bold face by simulating
bold — double-striking one pixel right and adding 1 to the advance (that pixel
is `tmOverhang`) — and leaves a 600 request alone, because the mapper rounds a
weight that is not yet bold down to the face it has. So the 20 and 24 fonts are
emboldened and the **18-pixel report body is not**.

That is the same conclusion PORT-B10 reached from the other end — "a bold weight
no longer buys a pixel of advance", because 63 extra pixels put the briefing
back outside its box. Here it is not a rule we imposed; it is the font's own
weight class. The table above shows what it is worth: the 20-pixel font's 487
includes 63 px of synthetic-bold overhang, without which it would be 424.

### Anti-aliasing, and the one place this is not faithful

GDI in 1999, on a 16-bpp surface, with `lfQuality` 2 and font smoothing off,
drew these as **bilevel** bitmaps. `ll_ttf.c` computes coverage and **blends**,
because an unhinted bilevel render at 13.6 ppem drops thin stems — the one
visual regression a faithful threshold would buy. `ll_ttf_set_antialias(0)`
thresholds at 50% for the bilevel look. The metrics are identical either way, so
the choice cannot move a box; it is a pixel decision, not a layout one, and it
is recorded here rather than hidden.

### The fallback is not theoretical

`legoland_tests` and `legoland_headless` run natively and under node with no
mounted gamedata. `ll_ttf_available()` stays 0 there and `ll_font.c` uses
PORT-B10's bitmap face exactly as before — same advances, same ink box, same
everything. Nothing in `ll_ttf.c` is Emscripten-specific; the native build
compiles and links it and the native ctest is 14/14 green with it in.

### What it looks like

The Duty Manager's briefing, page 1, in the real face: every one of the eleven
lines complete inside the notepad, in LEGO's own handwriting-style type, with
the longest — *"the park! Don't worry, I'll give you as much help as I can."* —
ending well short of the 460 px edge. PLAYER DETAILS, the eight slot captions,
the "Empty slot" bubble, the tutorial-select markers: all the real face.

And the money bar, read back with `llAscii(228,2,80,24,110,true)` in the park:

```
.........###.......#####........#####........#####...
........####......#######......#######......#######..
......######.....###...###....###...###....###...###.
.......##.##....###.....##...###.....##...###.....##.
..........##....###.....###..###.....###..###.....###
..........##....##......###..##......###..##......###
..........###...###.....###..###.....###..###.....###
..........###....###...###....###...###....###...###.
..........###.....#######......#######......#######..
```

**1000**, with three closed zeroes. PORT-B10 had to draw an unslashed oval and
re-bias the internal leading to get that; here the glyph is the font's glyph and
the baseline is `y + tmAscent`, which is where money.c:156's sprite-height box
expects it (the lfHeight 24 font's ascent is 19 in a ~20-pixel box).

### Hashes moved, and they were meant to

Every screen with text on it has a different frame hash from PORT-B10's table —
the type is different. PORT-B10's own caveat applies with more force now: treat
`llPark()` and `llFont()` as the assertions and the hashes as "this is what it
was on this build".

---

## 2. Sound — two halves, and the samples had to decode first

### 2a. `msacm32.c`: MS ADPCM, because a refused codec is not "silence"

Every sound the game loads goes through `ConvertWAVToPCM` (resaudio2.c:172)
*unconditionally*, and `CreateSampleFromWAV` (data2.c:591) does
`if (!conv) goto free_data;` two lines later. So a refused conversion is not a
sample that plays silently — it is a sample that **never becomes a Sample record
or a DirectSound buffer at all**. PORT-B6 refused ADPCM with ACMERR_NOTPOSSIBLE,
which is honest (a real ACM with no codec answers the same way) but it left a
hole nobody had measured.

**The census.** Over the RIFF/WAVE members of all three shipped archives there
are 155 samples:

| format | count |
| --- | --- |
| PCM, mono 22050, 16-bit | 122 |
| PCM, mono 22050, 8-bit | 11 |
| PCM, mono 44100, 16-bit | 1 |
| **MS ADPCM (tag 2), mono 22050, 4-bit, blockAlign 512** | **21** |

and **every one of the 1,266 files in `gamedata/disc/Speech` is MS ADPCM**, mono
22050. So the shim was converting 134 of 155 sound effects and not one word of
narration.

It now converts **155 of 155 and 1,266 of 1,266** — 57.8 MB of ADPCM into
228.5 MB of PCM16, 86 minutes of speech. The decoder was checked byte-for-byte
against an independent reference implementation on every one of those 1,266
files: **identical, all of them**.

Details that are load-bearing rather than decorative:

* The coefficient pairs come from the **format's own extension block**
  (`wSamplesPerBlock` +0x12, `wNumCoef` +0x14, then the pairs), not from the
  usual seven. `struct ll_wfx` grew a `cbSize` to reach them, and a tag-2
  format with `cbSize < 4` is refused rather than decoded against a guess —
  which matters because data2.c:632 *forces* `cbSize` to 0 for a `fmt ` chunk of
  0x12 bytes or less.
* `acmStreamSize`'s ADPCM answer is a deliberate **ceiling** (block count
  rounded up). The game mallocs exactly what it returns and reads the real
  figure out of `cbDstLengthUsed`; an over-estimate wastes a few hundred bytes,
  an under-estimate overruns the heap.
* The source is consumed in **whole blocks** — a block is the codec's unit of
  state — and `cbSrcLengthUsed` reports only the blocks that fitted the
  destination. `RefillNarrationRing` (narration2.c:564) converts exactly ten
  blocks at a time and advances its source ring by that figure.
* IMA ADPCM (tag 0x11) is still refused. Nothing in the shipped data uses it.

### 2b. `ll_audio.c` + `dsound.c`: the PCM reaches a speaker

PORT-B4's `IDirectSound` keeps **every** semantic it had — the wall-clock play
cursor, the refcount `Release` returns (audiomisc.c:51 compares it to 0), the
status a finished one-shot reports (`AutoKillSamples` reaps on it), the volume
`GetVolume` round-trips (`FadeSamples` steps it), `DSBLOCK_ENTIREBUFFER`
returning the full length. It gains one field: a **voice**, a Web Audio gain and
panner made on the first `Play` and torn down with the buffer.

* Volume and pan stay in DirectSound's hundredths of a dB and are converted
  where they are used: gain `10^(v/2000)`, with -10000 treated as true silence
  rather than 10^-5. `SetFrequency` becomes a `playbackRate`; pan becomes a
  `StereoPannerNode` position derived from the two channel gains.
* `Stop`, a `SetCurrentPosition` on a playing buffer, and the final `Release`
  all move the sound, in that order of subtlety. `DuplicateSoundBuffer` shares
  the PCM and gets its **own** voice, which is what `CreatePlayableSample`
  (audio3.c:356) needs — one duplicate per live instance.

**Two playback modes, decided by the game's own call pattern rather than by a
guess about buffer sizes.**

*Static.* A sound effect is written once, in full, before it is ever played:
`CreateSampleFromWAV` does one `Lock(0, 0, .., DSBLOCK_ENTIREBUFFER)`, one
memcpy and one `Unlock` at LOAD time. So it becomes one `AudioBuffer` and one
`AudioBufferSourceNode`, with `loop` for DSBPLAY_LOOPING, and the browser does
the rest. That is all 155 archived samples.

*Streamed.* The narration buffer is `0xa000` bytes of ten `0x1000` blocks,
played **LOOPING for ever** while `PumpNarration` (narration2.c:622) rewrites the
block ahead of the play cursor, frame after frame; a speech file of any length
goes through those 40 KB. A snapshot would loop the first 1.9 seconds of every
line. So a streamed buffer is fed as small AudioBuffers scheduled back to back,
**each copied out only after the cursor `dsound.c` reports has passed over it**.

That last clause is the whole trick. The cursor is what the game fills against —
`PumpNarration` reads `GetCurrentPosition` and fills the block ahead of it — so
"behind the cursor" is exactly "already written", and the output can never read
a block the game has not filled. It costs one chunk of latency, ~46 ms for the
narration ring.

The classifier is `Lock` without `DSBLOCK_ENTIREBUFFER`. Every streaming writer
passes flags 0 (narration2.c:646, movie.c:599, input2.c:478); every sample
loader passes 2. The mark is therefore set before the buffer's first `Play`,
which is what the narration path needs (`RewindNarrationBuffer` locks the whole
buffer with flags 0, primes ten blocks, then plays looping).

**Why `EM_JS` and not a `--js-library` entry.** `ll_canvas.js` reaches the shim
through four `ll_js_*` symbols resolved by `--js-library` in the browser link
and by `src/headless/node_shim.c` in the node links. A fifth family that way
would mean editing `node_shim.c` and `tests.cmake` — neither this lane's — and
would break `legoland_tests` and `legoland_headless` the moment it landed. EM_JS
compiles the JavaScript into the object file, so it links everywhere with no
library and no stub. The price is that it has to be node-safe itself: every
lookup goes through `globalThis['Name']`, for the reason ll_canvas.js documents
at length — emcc's -O2 JS optimizer constant-folds `typeof AudioContext ===
'undefined'` at BUILD time, with no DOM in scope, and bakes in the wrong answer.

**The autoplay policy.** A browser creates the AudioContext `suspended` and will
not start it outside a real input event. `DirectSoundCreate` makes the context
**eagerly** (not at the first `Play`) so the one-shot capture-phase listeners it
installs — pointerdown, keydown, touchend, mousedown — are in place before the
user's first interaction; `InitSoundSampleSystem` runs inside `InitSession`, long
before the front end is drawn. Until a gesture arrives, `Play` calls are counted
as **blocked** rather than dropped, `llAudio().state` says `suspended`, and the
page's "sound" cell turns amber and becomes a click-to-enable affordance.
Nothing ever waits on audio: the game's cursor is wall clock, not the audio
clock.

**DirectMusic stays stubbed.** musicthread.c's port buffer would arrive through
the same `CreateSoundBuffer`, but nothing writes PCM into it (the music is
`.sgt` segments a DirectMusic performance would render), so it is silent by
construction and out of scope.

### 2c. THE NARRATION SPEAKS — measured

The streaming path is not a design on paper. Built with `-DLL_PRELOAD_SPEECH=ON`
(the speech tree is 58 MB and OFF by default in `portable/cmake/browser.cmake`),
walked from a cold page to the tutorial briefing, with the AudioContext's
`createBuffer` wrapped so every scheduled chunk could be read back:

```
llAudio() after ~40 s:
  { state: "running", rate: 48000, voices_created: 4,
    stream_chunks: 510, stream_bytes: 1305600, underruns: 2 }

440 AudioBuffers scheduled, 1280 frames each @ 22050 Hz (= 2560 bytes,
  exactly the chunk dsound.c computes for the 0xa000 narration ring)
426 of the 440 carry real waveform -- peaks to 0.986, ~100% non-zero samples
gain 0.3162 == -1000 hundredths of a dB, which is the game's speech slider
1,305,600 bytes = 29.6 seconds of speech
llStats(): 33.9 fps, traps [], dead null, 99.3% non-black
```

That is the whole chain, end to end and in one measurement: MS ADPCM off the
disk → `acmStreamConvert` → the game's decoded ring → `PumpNarration` filling
the block ahead of the cursor → `dsound.c` classifying the buffer as streamed →
the chunk pump → Web Audio's destination, at the volume `SetSpeechVolume` asked
for. **Two underruns in 510 chunks**, and the frame rate did not move.

**The Duty Manager talks.**

### 2d. And a self-test, because "wired up" and "reached" are two questions

`?sound=test` runs one sample through the **game's own** load-and-play sequence
at `DirectSoundCreate` time — `CreateSoundBuffer` with data2.c's flags (0xe0),
one `Lock(0,0,..,DSBLOCK_ENTIREBUFFER)`, a memcpy into `ptr1`, `Unlock`,
`SetCurrentPosition(0)`, `Play` — with a generated 440 Hz tone (400 ms, 22050 Hz
16-bit mono, raised-cosine edges so it does not click), and releases the buffer
afterwards. It runs only when the URL asks.

It exists because *"is the audio path wired up?"* and *"does the game get as far
as playing a sample?"* are two questions, and until this lane they could only be
asked together — which matters a great deal, because the answer to the second
one, for **sound effects**, is currently **no** (§3).

Measured with `AudioBufferSourceNode.prototype.start` tapped:

```
one node started: 8820 frames @ 22050 Hz = 0.4000 s,
                  peak 0.3357 (= 11000/32768, exactly),  RMS 0.2298
llAudio() -> { state: "running", voices_created: 1, plays: 1,
               plays_blocked: 0, formats: { "22050Hz/1ch/16bit": 1 } }
```

The waveform the shim wrote through `Lock` is the waveform that reached Web
Audio's destination, at the amplitude it was written at.

---

## 3. THE SOUND-EFFECT BLOCKER: 23 sample names are raw x86 addresses

**Narration works; sound effects do not play, because almost none are loaded.**
Measured over a whole walk from a virgin IDBFS to the park with the Space Tower
built and two runs of path dragged, with `?trace=1&tracegrep=DSOUND`:

* `DirectSoundCreate` and `SetCooperativeLevel` succeed.
* `IDirectSound::CreateSoundBuffer` is called **exactly once** in a whole
  session — 69,228 bytes, 22050 Hz mono 16-bit, which decodes to
  `Space Tower01.wav` — and on a slightly different route, **zero** times.
* `PlayInstanceOfSample` therefore never fires: it guards on `s->def`, and every
  FX table's `+0x08` is still null.

### It is PORT-A7/A8's raw-pointer class again, in an object the gate cannot see

`InitGameMap` (mapinit.c:41) calls `Load_FXList(g_game_fx, 0x17)` before the
front end is drawn. The RES layer is **innocent**: `RES_OpenFile` (res.c:93)
strips the leading `.\`, splits at the last backslash into a directory bucket
and a member, and looks both up case-insensitively (`_stricmp`, audio3.c:540;
`stricmp`, res.c:148), so `.\sfx\Flowers.wav` becomes bucket `sfx\` member
`Flowers.wav` — and a walk of the archive's real directory tree puts
`Flowers.wav` at `SFX\flowers.wav`, in the same bucket as `Space Tower01.wav`.
Same bucket, same archive, same format string. 22 of the 23 are present (the
23rd, `Drilling.wav`, is genuinely absent from `Legoland.res` — the shipped game
failed to load it too).

The defect is in the **data closure**. `portable/build-wasm/gen-browser/globals.c`:

```c
/* 0x004b9228 .data 32 bytes (+g_map_fx) */
__attribute__((aligned(16))) unsigned int g_game_fx[8] = {
    0x004b9b94u, 0x00000000u, 0x00000000u, 0x004b9b80u,
    0x00000000u, 0x00000000u, 0x004b9b6cu, 0x00000000u
};
```

`0x004b9b94` is the original image's address of the string `"Flowers.wav"`;
`0x004b9b80` is `"RabOld\Drill.wav"`. They are **raw, unrelocated Win32 VAs**.
At runtime `g_game_fx[0].name` is the integer 4,955,028 — an arbitrary in-bounds
wasm heap address — so `sprintf(path, ".\\sfx\\%s", garbage)` builds a nonsense
member name, `stricmp` never matches, `RES_OpenFile` returns 0, and
`Load_FXList`'s failure branch is `DBPrintf`, which is silent in this port. All
23 fail without a word.

**Why:** `extern FXEntry g_game_fx[];` (mapinit.c:18, loaders.c:284;
`g_map_fx[]` lifecycle.c:33) has **no bound**, so `cdecl.py` cannot compute an
extent (`full_type` returns None when `count is None`, cdecl.py:264) and
`gen_link.py`'s pointer scan skips the object outright (gen_link.py:946:
`if ty is None or not cdecl.has_pointer(ty): continue`). The 276-byte table is
then tiled from gaps into **twelve** separate 16-byte-aligned objects
(`g_game_fx[8]`, `g_place_sample`, `g_snd_close`, `g_snd_click`, …), so even if
the pointers were right, `Load_FXList`'s stride would walk off the end after
entry 2.

**Why Space Tower is the one that works:** mechrides.c:673 says
`extern void* g_spacetower_fx;` — a *bounded* pointer declaration — so
`gen_link.py` re-points its word 0. globals.c:4130 has
`(unsigned int)(char*)(g_tower_seat) + 64` where the others have a bare
literal. One declaration is the entire explanation for "exactly one
`CreateSoundBuffer` per session".

**The gate is blind to this**, which is the part worth keeping. `gen/pointers.md`
reports **"raw pointer words: 0"** — truthfully, because it counts only words
the pointer scan *visited*, and an object with no computable extent is never
visited. So a whole class of raw pointers sits below it.

### How big the class is

A scan of the emitted closure for word arrays containing a literal in the
original `.data`/`.rdata` range (0x004ab000..0x00833f74) that points at a
printable C string — rejecting words whose own low three bytes are printable
ASCII, which is inline text being misread as an address — finds
**122 raw words in 18 objects**:

| object | raw words | what they point at |
| --- | --- | --- |
| `g_power_table` | 63 | "Small Power Station", "Crystal Power Station", "Dino Big", "T-Rex" … |
| `g_near_offsets` | 31 | the CRT `_matherr` name table ("exp", "pow", "log10", "sinh") — `.rdata`, probably dead |
| `g_place_sample` … `g_sample_mechanic` (10 objects) | 20 | the rest of `g_game_fx`'s 23 sample names |
| `g_game_fx` | 3 | `Flowers.wav`, `RabOld\Drill.wav`, `RabOld\Punch4.wav` |
| `g_money_fx` | 2 | "Coin drop for food stands or entrance.wav", "Cash Register.wav" |
| `g_joust_fx`, `g_zoomer_loop_sample`, `g_gfx_dirs` | 3 | "Joust Horses.wav", "airplanes breaking down.wav", `".\graphics\textures\"` |

So the sound blocker is 23 of 122, and `g_power_table`'s 63 — a table of
element NAMES — is very likely a second live defect of the same shape that
nobody has looked for yet.

### Who owns it, and the one-line fix

**A game-source recovery defect (PORT-M), not a shim one.** `LEGOLAND/*.c` is
read-only for this lane, so it is filed, not fixed. Add the bound to every
declaration of each table so no translation unit disagrees:

```
LEGOLAND/mapinit.c:18     extern FXEntry g_game_fx[];   ->  [0x17]
LEGOLAND/loaders.c:284    extern FXEntry g_game_fx[];   ->  [0x17]
LEGOLAND/lifecycle.c:33   extern FXEntry g_map_fx[];    ->  [0x17]   (its comment already says 23)
LEGOLAND/audiomisc.c:179  extern FXEntry g_money_fx[];  ->  [2]
LEGOLAND/money.c:176      extern FXEntry g_money_fx[];  ->  [2]
LEGOLAND/joust.c:642, joust2.c:542, narration2.c:120    g_joust_fx[]     ->  [1]
LEGOLAND/screencb5.c:424                                g_entrance_fx[]  ->  [1]
```

A bound on an `extern` array is not a codegen lever here — there is no `sizeof`
and no whole-array arithmetic; `Load_FXList(g_game_fx, 0x17)` and
`g_game_fx[i].sample` emit the same x86 — so the byte gates should not move. The
bound also makes `declared_extent(0x004b9228)` 276, which merges the twelve
fragments into one block with the `.sample` names as interior aliases, fixing
the stride at the same time.

**And a gate to add (PORT-A):** `pointers.md`'s "raw pointer words: 0" should be
joined by a sweep that does not depend on the declaration — *any* word in an
emitted `.data`/`.rdata` block whose value lands in the original image's data
range and points at a printable string is a raw pointer, whether or not the
object had a computable extent. That check, in twenty lines, would have caught
all 18 of these.

### How to know when it is fixed

Open the page with `?trace=1&tracegrep=DSOUND` and walk to the park. Today the
`CreateSoundBuffer` count is 0 or 1. When the bounds land it should be **23 at
startup, before the front end draws**, and `llAudio().plays` should start
climbing on the first icon click, with `formats` filling in
`22050Hz/1ch/16bit`. Nothing in this lane's files has to change for that to
happen — the ACM converts all 155 of these files today (§2a) and the output path
is proved (§2c, §2d).

---

## 4. PARK-2 — the surface model is innocent; the advisor window is the hole

PORT-B10 left this "undecided — needs the original to settle". It is settled,
and it is not what the entry supposed.

### It reproduces, and it is smaller than it looked

From a virgin IDBFS, in the park, cursor moved to game (577, 419):

```
rhash(400,400,240,80)     0x4bd6064c -> 0x8ecd327c (bubble up) -> 0x8eecf240
rhash(523,400,110,73)     0x48f02f50 -> 0xd1e51836            -> 0x13a67868
```

and neither returns, through four more seconds of frames. `llAscii(540,380,...)`
shows what is left: a **converging V** — the downward tail of the bubble —
sitting at roughly game (565, 398)-(575, 408), immediately above the bottom
panel's top edge. Everything else the bubble drew was over the map and was clean
on the next frame.

(A caution for anyone re-measuring: once the residue is on screen it is in your
*baseline*, and before == after trivially. This took a wipe and a fresh walk to
see.)

### The present path does NOT differ from the original's

Both ends were read.

* The game **never creates a flip chain.** `screen.c:1988-2004` (fullscreen)
  creates the primary with caps 0x4200 and `g_surface_78` with caps 0x800
  (DDSCAPS_OFFSCREENPLAIN), sets `g_draw_surface = g_surface_78`, and sets
  `g_present = FlipPrimary`. The windowed arm (`screen.c:2054-2071`) is the same
  shape with SYSTEMMEMORY. No `DDSCAPS_FLIP`, no `dwBackBufferCount`, anywhere.
* `FlipPrimary` (sysmisc.c:554-595) is
  `g_primary->vtbl->Blt(g_primary, &dst, g_surface_78, 0, DDBLT_WAIT, 0)` — a
  **copy**, not a swap. `g_surface_78` keeps its pixels. (`PresentFlip`, the real
  `Flip` path, exists at blitmisc.c:347-389 as `g_present`'s initial `.data`
  value and is overwritten before any frame is drawn.)
* `portable/src/hostwin/ddraw.c` models exactly that: surfaces are `calloc`'d
  linear 16-bpp buffers that live for the surface's lifetime, `Lock` hands back
  `s->bits` directly, `Blt` copies rows and then presents if the destination is
  the primary, and `Flip`'s swap path is dead because no flip chain is ever
  requested.

So the answer to the question the brief asked is **no**: one persistent back
surface plus a Blt to the primary is what the original does and what the shim
does. Residue in a region the game does not repaint would persist on Windows
too — *if the game really did not repaint it*.

### There is no erase path, and there was never meant to be

`BubbleHelp` (bighelp.c:296-401), `HTBubbleHelp` (fpui2.c:987) and
`VisitorBubbleHelp` (bubblecache.c:476) are stateless, draw-only, per-frame.
None saves a backing rectangle; none has an inverse; the only "unload" in the
file frees sprites. Expiry just stops calling the drawer. The erase is
`InGameFrameBody` (gameframe.c:578-682) repainting the HUD **before** the bubble
every frame: `PrintSprite(g_ci_interface_bg, 0, 0, ...)` at :602, then the icon
renders at :603, then the bubbles at :606 and :617-681.

### The actual cause: a 112x96 hole that only the advisor fills

`InterfaceBG.lls` is 640x480 and is deliberately **transparent** over
x 523..632, y 380..472 — the advisor video window. The original fills it every
single frame from `RenderAdvisorIcon` (screens3.c:2220-2259), which is reached
each frame through `RenderIconsSkipGroup(0x2c3)`:

```c
if (!ScriptRunning()) {
    if (g_vidanim) { dib = AVIStreamGetFrame(...); BltAdvisor(dib, p->x, p->y); ... }
} else {
    PrintSprite(p->sprite, p->x, p->y, 0, &ctx);   /* ScriptEnd.lls, 112x96, OPAQUE */
}
```

In this port the first branch is empty, by `avifil32.c`'s own measured statement:
all six `LoadAdvisorMovie` calls return 0, so `g_vidanim` is null and that `if`
is false for the rest of the run. **Nothing writes to the advisor window when no
script is running, ever.** The bubble's tail overlaps it by about 10x10 pixels,
and that overlap is permanent.

The same stub explains why the bubble was raised from a *panel* pixel at all —
PORT-B10's hypothesis was a missing viewport gate on `g_input.map_x/map_y`, and
the real gate (`ReadGameButtons`, bighelp.c:166) is `g_focussed_icon == 0`, i.e.
"does an icon own this pixel". Ownership is established **by the blit**:
`PrintSprite` sets `g_hit_ctx` when the blitter reports the cursor over pixels it
just drew (printlist.c:697, :752-753). `BltAdvisor` bypasses `PrintSprite`, so
`RenderAdvisorIcon` does the equivalent test by hand — inside the same dead
`if (g_vidanim)`. On Windows the advisor panel owns that pixel; here nothing
does, the game computes a map square for a panel pixel, and the map raises
"Outside your park". **Cause and residue are one defect, not two.**

### Verdict and re-filing

* **Not the shim's surface model.** Proved from the game's own source and from
  `ddraw.c`. Changing `ddraw.c` to a flip chain would only make it wrong.
* **Not a game-side erase path that never runs.** There is none to run.
* **It is `portable/src/hostwin/avifil32.c`'s advisor stub** — a PORT-B item,
  a sibling of PARK-4, not a game-side lane.

**Not fixed here, and the reason is a real one.** PORT-B6 chose failing
`AVIFileOpenA` after reading both callers, and its header sets out why a
synthetic stream is worse for the FMV path: it takes `PlayMovie` into `RunMovie`
with zero frames, unlocking and relocking the video surface around a loop that
never runs, ducking and restoring the whole audio stack, and dividing by
`si.dwScale` in a struct nobody has checked against the original. Making the
**advisor** succeed while the **FMV** keeps failing means distinguishing them by
filename (`AD_*.avi`), implementing `AVIStreamInfoA` in the ILP32 layout
movie.c:123 documents, and having `AVIStreamGetFrame` return a 112x96 16-bpp
BITMAPINFOHEADER-plus-pixels at +0x28 (blitmisc.c:67) matching
`g_advisor_bmi` — and then deciding what those 112x96 pixels *contain*, when
there is no Indeo 5 decoder. That is a lane, and filling the hole with an
invented flat rectangle would be fabricating art to hide a stub.

**Recipe when someone takes it.** Two cheap confirmations first: (1) hash
`(523,400)-(633,473)` separately from the rest of `(400,400)-(640,480)` — the
change is confined to the advisor window; (2) check `ScriptRunning()` at the
moment of the repro — it must be false, or `ScriptEnd.lls` would have cleaned
it. Then make the advisor path paint *anything* 112x96 at (522, 378) and **both**
symptoms — the residue and the panel pixel being read as a map square — go
together.

---

## 5. What the page grew

| hook | what it answers |
| --- | --- |
| `llFont()` | the face (upem, glyphs, usWinAscent/Descent, weight class) and every LOGFONT the game asked for with the cell, ascent, descent, ppem, synthetic bold and `reportLine63` it got |
| `llAudio()` | AudioContext state and rate, voices created, plays, **plays blocked by the autoplay policy**, active sources, streamed chunks and bytes, underruns, and a histogram of the formats actually played |
| `llAudioResume()` | resume the context by hand |
| `?sound=test` | one tone through the game's own load-and-play sequence at `DirectSoundCreate` |
| panel cell **font** | `Lego.TTF, 4 sizes (24/28/20/18)` or `bitmap fallback` |
| panel cell **sound** | the context's state and the counts, colour-coded, **clickable to enable sound** |

The sound cell earns its place: "running, 14 played" and "suspended, 14
BLOCKED" are indistinguishable on a machine nobody is listening to, and the
second one is the browser's policy rather than a defect in the port.

---

## 6. Gates

| gate | result |
| --- | --- |
| native clean build + `legoland_tests` | builds; **ctest 14/14** |
| wasm32 clean build, all six targets | builds; **ctest 20/20** |
| the page, `?args=-nointro+WINDEBUG` | **33.5–33.7 fps**, `traps: []`, `dead: null`, 98.2% non-black |
| the PORT-B10 walk to the park | reproduces; `llPark().money` 1000 on arrival, the ride buys, path drags |
| MS ADPCM decoder vs an independent reference | **1,266 / 1,266 speech files byte-identical** |
| every archived WAV through `acmStreamConvert` | **155 / 155** convert (was 134) |
| Web Audio output, source node tapped (`?sound=test`) | 8,820 frames @ 22,050 Hz, peak 0.3357, RMS 0.2298 |
| narration streamed (`-DLL_PRELOAD_SPEECH=ON`) | **510 chunks, 1,305,600 bytes = 29.6 s of speech; 426 of 440 buffers carry waveform; 2 underruns; 33.9 fps** |

`LEGOLAND/*.c` is untouched, so the byte gates (`audit.py`, `relocs.py`,
3281/42) cannot have moved and were not re-run.

The speech build is a second configure directory (`portable/build-wasm-speech`,
`-DLL_PRELOAD_SPEECH=ON`) rather than a reconfigure of the first: 58 MB of
preloaded assets is not something to make the default page carry, and the wave's
rule is one clean build per directory.

---

## 7. Files touched

| file | what |
| --- | --- |
| `portable/src/hostwin/ll_ttf.c` | **new** — the TrueType rasteriser, the GDI height mapping, the glyph cache, `llFont()` |
| `portable/src/hostwin/ll_audio.c` | **new** — the Web Audio back end, the autoplay policy, `llAudio()` |
| `portable/src/hostwin/ll_font.c` | routes measuring and drawing through the TrueType face when it is there; bitmap face kept as the fallback |
| `portable/src/hostwin/gdi32.c` | `AddFontResourceA` loads the face; `CreateFontIndirectA` traces and publishes what it got |
| `portable/src/hostwin/dsound.c` | the voice, the two playback modes, the streamed classifier, `?sound=test` |
| `portable/src/hostwin/msacm32.c` | MS ADPCM |
| `portable/hostwin/include/ll_host.h` | the two new API blocks |
| `portable/cmake/browser.cmake` | the two new sources |
| `portable/src/browser/index.html` | the font and sound cells, the click-to-enable affordance, the hook docs |
| `docs/SCOPE_PORT_WAVE.md`, `portable/README.md`, this file | the record |

`ll_font.c` and `index.html` are outside the brief's literal file list but
inside PORT-B's area and unavoidable for deliverables 1 and 2 (the brief's own
reading list names `ll_font.c`, and "a click-to-enable-sound affordance" is a
page change). Noted here rather than assumed.
