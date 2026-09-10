# Scope PORT-B6 — a trap-free page, and the front end walked with real input

> **Status: IN PROGRESS (claimed 2026-09-12 by PORT-B6).** Branch
> `scope/PORT-B6` from `feat/decomp-completion-next-steps-24a0d6` @ `07f11d66`
> (the PORT-A5 merge). Files owned: `portable/src/hostwin/{avifil32,msacm32}.c`
> (new), `gdi32.c`, `ddraw.c`, `user32.c`, `dinput.c`, `winmm.c`, `dsound.c`,
> `portable/src/browser/**`, `portable/cmake/browser.cmake`, additions to
> `portable/hostwin/include/ll_host.h`. `LEGOLAND/*.c` untouched, so the VC6
> gate has nothing to check for this lane.

---

## 1. Generated host traps: 96 -> 0

PORT-A5 left six, all AVIFIL32, all reached on the front-end path:

```
TRAP AVIFIL32.dll AVIFileInit           from advisor.c, movie.c
TRAP AVIFIL32.dll AVIFileOpenA          from advisor.c, movie.c
TRAP AVIFIL32.dll AVIFileInfoA          from advisor.c, movie.c
TRAP AVIFIL32.dll AVIFileRelease        from advisor.c, movie.c
TRAP AVIFIL32.dll AVIFileExit           from advisor.c, movie.c
TRAP AVIFIL32.dll AVIStreamGetFrameOpen from advisor.c, movie.c
```

Three new pieces of shim close the whole host side of the closure:

| file | names | what it does |
| --- | --- | --- |
| `portable/src/hostwin/avifil32.c` | 16 | AVIFile. `AVIFileOpenA` reports `AVIERR_FILEOPEN`; everything else reports cleanly out of a handle it never follows |
| `portable/src/hostwin/msacm32.c` | 6 | the ACM. A **real** PCM -> 16-bit-PCM converter, ADPCM refused with `ACMERR_NOTPOSSIBLE` |
| `gdi32.c` (appended) | 1 | WINSPOOL's `EnumPrintersA`, the last one |

```
$ grep -o 'll_gen_trap("[^"]*", "[^"]*"' portable/build-wasm/gen-browser/host_stubs.c | sort -u
(nothing)
$ LL_CD_DIR=.../disc LL_DATA_DIR=.../main node legoland_headless.js -nointro   # NO LL_TRAP_CONTINUE
legoland_headless: WinMain(NULL, NULL, "-nointro", 1)
[hostwin] window "LEGOLAND" 640x480
cannot open output file            x7   <- LoadProfilesFormDisk on a fresh install
(runs the front end indefinitely; killed at 70 s)
```

`closure_filter: dropped 96 generated traps the host shim owns` (was 73 before
this lane; the 23 new ones are these files' names plus the aliases gen_link
makes for them). The only remaining `ll_gen_trap` anywhere in the closure is in
`stubs.c`, and those four are not host calls at all: `_errno_location`,
`_indirect_function_table`, `_stack_pointer`, `_small_sprintf`.

### 1a. AVIFIL32: why AVIFileOpenA fails, and why that is the *designed* path

The 26 `AD_*.avi` advisor clips and the FMV set are real files in
`gamedata/main`, and they are Indeo 5 — `RunGame` even `LoadLibraryA`s
`Ir50_32.dll` (gamemain.c:398). There is no Indeo 5 decoder in this port, so the
only question was *how* to report that, and the answer came from reading both
callers rather than from taste:

* **`movie.c` 0x00476460 `OpenMovie`** answers a failed open with
  `if (g_avi_open_count == 0) AVIFileExit(); return 0;` and **nothing else**.
  Its only caller, `uimisc2.c` 0x004771f0 `PlayMovie`, answers a null movie by
  retrying under `g_res_path` and then `return 1` — *without entering the
  player*: no `PauseAllSamples`, no `StopMusic`, no
  `PushRenderingStatusAndUnlockVideoSurface`, no `RunMovie`, no `CloseMovie`. So
  `lmi.avi`, `Intro.avi`, the three park movies (uimisc3.c), the level-end movie
  (gameframe.c:517) and the report movie (uimisc.c:734) each cost one
  `DebugPrintf`.
* **the alternative was rejected.** A synthetic zero-length stream would take
  the game *into* `RunMovie` with `frames == 0`, which unlocks and relocks the
  video surface around a loop that never runs, ducks and restores the whole
  audio stack, and spins until all three mouse buttons are up — a great deal of
  machinery to arrive at the same blank screen. It also divides by `si.dwScale`
  (`fps = si.dwRate / si.dwScale`, movie.c:663) in an `AVISTREAMINFOA` the shim
  would have to fill in, in the ILP32 layout, from a header nobody has checked
  against the original.

**The advisor degrades to no animation, not to a crash** — and that is a
property of the game, verified, not an assumption. `InitAdvisorMovies` guards
every one of its six assignments with `if (g_ad_*)`, `KillAdvisorMovies` guards
every teardown, and the one place an advisor frame is ever drawn is behind a
null test:

```c
/* screens3.c 0x00443e30 RenderAdvisorIcon */
if (g_vidanim) {                                   /* 0x00665f5c == g_advisor_clip */
    dib = AVIStreamGetFrame(g_vidanim->pgf, g_vid_frame);
    PushRenderingStatusAndLockVideoSurface();
    BltAdvisor(dib, p->x, p->y);                   /* never sees a null DIB */
    ...
    g_gfx_point.x < p->x + dib->width ...          /* nor this */
```

With all six loads returning 0, `StartAdvisorClip` stores null into
`g_advisor_clip`, that test is false for the rest of the run, and
`AVIStreamGetFrame` is never called. **Measured: the ADVISOR panel draws
nothing and the front end runs on.**

The shim is also **pointer-blind** — no entry point dereferences a PAVIFILE,
PAVISTREAM or PGETFRAME — and that is load-bearing, for the reason in §3 G1.

### 1b. MSACM32 is NOT a stub, and could not be

`data2.c` 0x00492380 `CreateSampleFromWAV` runs **every** RIFF/WAVE resource in
the archives through `resaudio2.c` 0x004921c0 `ConvertWAVToPCM`
(data2.c:664), whatever its format tag, and drops the sample two lines later
(`if (!conv) goto free_data;`). A shim that refused everything would therefore
make `CreateSampleFromWAV` return 0 for every sample in the game — no `Sample`
records, no DirectSound buffers, nothing for the sample layer to own. That is
not "silent", it is "the sample loader fails", and it would have been a
regression dressed as a stub.

So `msacm32.c` implements the conversion it can: **PCM -> 16-bit PCM**,
including the 8-bit-unsigned to 16-bit-signed widening (`(b - 128) << 8`) that a
real PCM codec does, with `cbSrcLengthUsed`/`cbDstLengthUsed` written and a
short destination treated as a partial conversion rather than an error. ADPCM
(tags 0x02 / 0x11) is refused with `ACMERR_NOTPOSSIBLE` (512) — what a real ACM
returns when no driver can do the job, and a result every caller handles,
exactly as the shipped game behaves on a machine whose ADPCM codec is missing.

Two shapes in the game forced specific decisions:

* **`acmStreamSize` writes its out parameter even on failure**, which a real
  `acmStreamSize` does not do. `audio4.c:211-213` is
  `acmStreamOpen(...)` (result ignored), `acmStreamSize(g_speech_acm,
  chunk, &output_size, 0)` (result ignored), `malloc(output_size)` — so a failed
  open leaves `output_size` an uninitialised stack word and the game mallocs it.
  On Windows the working codec hides that; here it would be a malloc of an
  arbitrary 32-bit number on the first line of speech. Writing 0 makes it
  `malloc(0)`.
* **handles are malloc'd and leak on purpose.** resaudio2.c's own comment:
  "ORIGINAL BUG, reproduced: the ACM stream is never `acmStreamClose()`d, so a
  stream handle leaks per converted sample." A fixed pool would run out after a
  few samples and start refusing conversions a real ACM would do; 24 bytes per
  leaked stream is what Windows loses too.
* handles are **validated, not trusted** (a magic word), because `audio4.c`
  reaches `acmStreamSize` and `acmStreamPrepareHeader` with whatever a *failed*
  `acmStreamOpen` left in `g_speech_acm`.

### 1c. WINSPOOL

`certificate.c` 0x00451740 `SaveScreenshotBmp` is the only caller and opens with
`if (EnumPrintersA(...) <= 0 || rpn.returned <= 0) return 0;`, so reporting zero
printers stops the print path before `CreateDCA`. It lives in `gdi32.c` with the
rest of the print stubs rather than in a one-function `winspool.c` — the same
reasoning that puts ole32 in `dsound.c`.

## 2. Gates

| gate | result |
| --- | --- |
| wasm `ninja` (clean dir) + all six targets | builds |
| wasm `ctest` | 14/14 |
| native `ninja` + `legoland_tests` + `ctest` | 8/8 |
| generated host traps in `gen-browser/host_stubs.c` | **0** (was 1 after the AVI file, 23 before) |
| `legoland_headless -nointro`, no `LL_TRAP_CONTINUE` | runs the front end, 0 traps |
| `LEGOLAND/*.c` touched | none — no audit/relocs run needed |

## 3. Game-side findings (owners elsewhere)

### G1 — `StartAdvisorClip(NULL)` dereferences null, and -O2 makes it worse

```c
/* advisor.c 0x00443dc0 */
void StartAdvisorClip(AdvisorClip* clip)
{
    ...
    if (clip->stop)            /* <-- UNGUARDED */
        clip->stop(clip);
    if (g_advisor_clip) { ... }
    g_advisor_clip = clip;
    if (clip)                  /* <-- three lines later, so the author knew */
        clip->getframe = AVIStreamGetFrameOpen(clip->video, &g_advisor_bmi);
}
```

`InitAdvisorMovies` ends with `StartAdvisorClip(g_ad_blink)`, and `g_ad_blink`
is 0 whenever the blink clip did not load. As shipped that is an access
violation on Windows the moment `AD_Blink.avi` is missing. On wasm the read of
offset 0x20 of address 0 is permitted and returns zero — but clang then uses the
dereference to **prove `clip != NULL` and folds away the later `if (clip)`**, so
`AVIStreamGetFrameOpen(clip->video, ...)` is called unconditionally with
whatever word sits at address 0x1c. That is exactly why PORT-A5's trap list
contained `AVIStreamGetFrameOpen` at all, and it is why this lane's shim never
follows a stream pointer.

Nothing misbehaves today. **Owner: a matching lane** — the fix is to hoist the
`if (clip)` to the top, which changes the instruction stream and so is a
matching decision, not a port one.

### G2 — `OpenMovie` calls `AVIFileRelease(pfile)` with `pfile` uninitialised

`movie.c:631` declares `void* pfile;` and `AVIFileOpenA` only writes `*ppfile`
on success (as the real AVIFile does). The no-video rung at movie.c:672 then
calls `AVIFileRelease(pfile)`. Unreachable from a *failed* open — `OpenMovie`
returns first — but it is on the rung a future decoder would reach with a file
that has no video stream. Recorded, not fixed; same owner as G1.

## 4. Reproducing

```bash
PY=$HOME/.venvs/legoland/bin/python
emcmake cmake -S portable -B portable/build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DLL_ILP32=ON -DPython3_EXECUTABLE=$PY
ninja -C portable/build-wasm
ninja -C portable/build-wasm legoland_headless legoland_browser
(cd portable/build-wasm && ctest)                       # 14/14

D="LL_CD_DIR=$PWD/gamedata/disc LL_DATA_DIR=$PWD/gamedata/main"
env $D node portable/build-wasm/legoland_headless.js -nointro    # no traps, no TRAP_CONTINUE
```

---

# Part 2 — input end to end, and the two split records that stop the front end

## 5. What now happens in a tab, with the screens and the hashes

`http://localhost:8797/legoland.html?args=-nointro+WINDEBUG`, driven with real
pointer and keyboard events (no synthetic `LL.push`), every step verified by
hashing the canvas before and after with `window.llFrameHash()`:

| # | screen | what got there | frame hash | evidence |
| --- | --- | --- | --- | --- |
| 1 | **PLAYER DETAILS**, eight EMPTY slots, 33.5 fps, 96.5% non-black | load, `-nointro WINDEBUG` (music ON) | `0x9d9b7c50` | reproducible across four separate loads |
| 2 | **the same, with the bubble help "Empty slot" drawn at the cursor** | pointer resting over a slot | `0x9d9b7c50` (the tooltip is part of it) | the FIRST time the front end has ever reacted to the mouse |
| 3 | **the NEW PROFILE popup**, opened over slot 1, its name field and the red close icon, tooltip "Enter player details" | one left click at game (260,188) | `0x5ab7ca10` | hash before `0x9d9b7c50`, after `0x5ab7ca10` |
| — | typing into the name field | 'A' 'D' 'A' 'M' as real key events | `0x5ab7ca10` — **unchanged** | blocked, §6 B3 |

Screens 1–3 are `docs/lanes/scope-port-b4.md`'s screen plus two the port had
never reached. The walk stops at 3 for the reason in §6 B3 — a second split
record, not a shim defect: the keystrokes are proved to arrive
(`HOST DINPUT key state DIK 0x1e down (poll reports it)`, fourteen polls across a
400 ms press) and the game's own dispatch never calls the editor.

Screens 1 and 2 come up with **zero traps and no `?trapcontinue=1`**, and the
trace shows the AVI shim doing exactly what §1a says it would:

```
HOST LoadLibraryA("Ir50_32.dll"): refused
HOST AVIFileInit: no AVIFile library in this port; every open will report AVIERR_FILEOPEN
HOST AVIFileOpenA("AD_Blink.avi"): AVIERR_FILEOPEN (no AVI decoder)
HOST AVIFileOpenA("AD_LR.avi"): AVIERR_FILEOPEN (no AVI decoder)
... six of them, and the front end runs on with no advisor animation
```

### Driving the page, for the next lane

The pointer is RELATIVE (DIMOUSESTATE lX/lY) and `ll_canvas.js` differences the
absolute position, so the game cursor and the real pointer stay in step only if
they START in step. They do: the game's cursor begins at (320,240) and
`LL.lastX` is set without emitting on the first move, so **the first hover must
be the canvas centre**; after that, canvas-relative position maps 1:1 to game
pixels at any canvas scale, because `sx = LL.w / r.width` cancels the CSS
scaling.

```js
var c = document.getElementById('canvas'), r = c.getBoundingClientRect();
var S = SCREENSHOT_WIDTH / window.innerWidth;
g2s = (gx, gy) => [(r.left + gx * r.width / 640) * S, (r.top + gy * r.height / 480) * S];
// hover g2s(320,240) FIRST, then hover the target, then click it.
// hash with llFrameHash() before and after: the frame counter and fps keep
// moving whether or not the game reacted, and the hash does not.
```

## 6. Blockers, with owners

| # | what | owner | state |
| --- | --- | --- | --- |
| B1 | the 12-byte mouse-hit record at 0x004bdd00 is THREE objects: no front-end icon can ever be focussed | **PORT-A** (`gen_link.py` STRUCT_EXTENTS) | diagnosed, proved, one row supplied |
| B2 | `HTBubbleHelp` kills the module through `DrawTextA` on a memory DC | PORT-B6 | **fixed** (ddraw.c registry) |
| B3 | the 270-byte `CurProfile` at 0x0080ffa0 is EIGHT objects: the new-profile name editor is never called | **PORT-A** (`gen_link.py` STRUCT_EXTENTS) | diagnosed, proved, one row supplied |
| B4 | `StartAdvisorClip(NULL)` dereferences null; -O2 folds away the guard three lines below | matching lane | recorded (§3 G1) |
| B5 | `OpenMovie` releases an uninitialised `pfile` on its no-video rung | matching lane | recorded (§3 G2) |
| B6 | the "type a string" browser-automation verb delivers NO key events to the page | tooling, not the port | drive with one dispatched key event per character |

### B1 — the mouse-hit record is three objects, and it is why nothing is clickable

The front end's hit test is not a rectangle test: `PrintSprite` raises
`g_blit_hit` when the cursor is over the pixels it just drew and copies the
caller's 12-byte `BlitCtx` into the record at 0x004bdd00 (printlist.c:48), and
`UpdateFocussedIconPtr` (sweep2.c 0x004700a0) then reads

```c
g_focussed_icon = (g_icon_mode == 2) ? g_icon_value : 0;   /* +0x00, +0x04 */
```

before `CheckFocussedIcon` (fpui.c 0x0046f4c0) dispatches to `p->input`. The
closure emits:

```c
/* 0x004bdd00 .data 4 bytes (+g_hit_info, g_hit_type, g_hitinfo, g_icon_mode) */
__attribute__((aligned(16))) unsigned int g_hit_ctx[1] = { 0x00000100u };
/* 0x004bdd04 .data 4 bytes */
__attribute__((aligned(16))) unsigned int g_icon_value[1];
/* 0x004bdd08 .data 4 bytes */
__attribute__((aligned(16))) unsigned int g_hit_cell[1];
```

Three 16-byte-aligned objects. The writer stores twelve bytes at `g_hit_ctx` --
eight of them into that object's alignment padding -- and the reader reads
`g_icon_value`, which nothing ever writes. **`g_focussed_icon` is therefore
permanently 0, `CheckFocussedIcon` never reaches an icon's `input` callback, and
no click, hover, tooltip or drag in the entire front end can do anything.**

The row, with PORT-A5's two independent citations:

```python
0x004bdd00: 12,   # BlitCtx / HitInfo {kind, owner, cell}
    # printlist.c:61-65  typedef struct BlitCtx { int kind; void* p; int n; }
    #                    -- last field +0x08; printlist.c:245 declares
    #                    `extern BlitCtx g_hit_ctx;  /* 0x004bdd00 */`
    # gameframe.c:261    `extern HitCell g_hit_cell;  /* 0x004bdd08 */`, and
    #                    workorder2.c:308 the same address -- an independent
    #                    extern AT the last field's offset, +0x08. HitCell is
    #                    `union { int i; BPosW sq; }`, 4 bytes, so 0x08 + 4 = 12,
    #                    and the next named address is 0x004bdd0c
    #                    (printlist.c:251 g_msg_bad_clip). Interior names:
    #                    g_icon_value +0x4, g_hit_cell +0x8.
```

**Proved, not inferred.** A patch to the GENERATED `globals.c` merging those
three objects (`port-b6-hitinfo-patch.py`, scratch, not committed -- it edits a
build artifact, and `gen_link.py` is PORT-A's file) and a relink: the bubble help
appears under the cursor, a click opens the NEW PROFILE popup, and the frame hash
moves. Everything in §5 was measured on that build. Without the patch, screens 1
and 2 are identical bitmaps and screen 3 is unreachable.

### B3 — `CurProfile` is eight objects, and it is why the name field is dead

```c
/* bigscreens.c:416, the front-end popup dispatch */
if (g_cur_profile.profile_slot == p->u1c.slot) {
    ...
    } else if (g_newprof_popup_up) {
        EnterNewProfile(p);          /* screens2.c 0x00491bd0 -- the editor */
```

`ProfileEmptySlotInput` (screens3.c 0x0048d3c0) writes `g_profile_slot = p->slot`
and `bigscreens.c` reads `g_cur_profile.profile_slot`. Same address --
`screens3.c:234` and `unref7.c:271` both say `/* 0x0080ffe3  CurProfile+0x43 */`
-- and two different objects:

```
0x0080ffa0  g_cert_message[8]     32 bytes   (+g_cur_profile)
0x0080ffc0  g_cur_80ffc0[1]        4 bytes   (+g_profile_age)
0x0080ffc4  g_vol_speech[1]        4 bytes
0x0080ffc8  g_vol_music[1]         4 bytes
0x0080ffcc  g_vol_sfx[1]           4 bytes
0x0080ffd0  g_profile_themes[1]    4 bytes
0x0080ffd4  g_level_done[15]      15 bytes   (interior g_have_profile+0x5)
0x0080ffe3  g_profile_slot[1]      1 byte    <-- what the WRITER writes
0x0080ffe4  g_cur_save_slot[55]  220 bytes
```

`g_cur_profile` is 32 bytes, so `g_cur_profile.profile_slot` is 0x23 bytes past
its end -- padding, never equal to the slot clicked, so `EnterNewProfile` never
runs. Confirmed by the frame hash: the editor draws a BLINKING cursor
(`GetBlink()` alternating `|` and space, screens2.c:600), so a running editor
could not leave the hash constant, and it is constant to the bit for seconds.

```python
0x0080ffa0: 0x10e,   # CurProfile
    # bigscreens.c:73-87 / profiles.c:89-101 (identical, both #pragma pack(1)):
    #   name[0x20], f20, f24, f28, f2c, tail, stats[15] @ +0x34,
    #   profile_slot +0x43, save_slot +0x44, f45 +0x45, block[200] @ +0x46
    #   -> 0x46 + 200 = 0x10e
    # screens3.c:234 and unref7.c:271 both declare
    #   `extern unsigned char g_profile_slot;  /* 0x0080ffe3  CurProfile+0x43 */`
    #   -- an independent extern whose comment gives the SAME offset.
```

**Why PORT-A5's sweep missed both.** Its candidate regex wanted an address
comment naming `g_owner.field`. B1's names carry no owner comment at all
(`g_hit_ctx`, `g_icon_value`, `g_hit_cell` are four independent recovered names
for one record), and B3's comment says `CurProfile+0x43` -- a TYPE and an offset,
not `g_cur_profile.profile_slot`. This is exactly the residue A5 predicted
("A record whose fields were all recovered under separate names ... is still
split and still silent"), and these are the second and third instances of it on
the front-end path. **A wider sweep is worth a lane**: every
`/* 0x00xxxxxx <Type>+0xNN */` comment, and -- the general form -- every address
whose closure extent is SMALLER than the `sizeof` of the struct the declaring TU
gives it.

## 7. Gates, after both parts

| gate | result |
| --- | --- |
| wasm build from a CLEAN dir, all six targets | builds |
| wasm `ctest` | 14/14 |
| native build + `legoland_tests` + `ctest` | 8/8 |
| generated host traps in `gen-browser/host_stubs.c` | **0** |
| `legoland_headless -nointro`, no `LL_TRAP_CONTINUE` | runs the front end, no trap, no fault |
| the page, `?args=-nointro+WINDEBUG` | PLAYER DETAILS at 33.5 fps, 0 traps |
| `LEGOLAND/*.c` touched | none |

## 8. Scratch scripts (not committed; three lines each to recreate)

* `port-b6-hitinfo-patch.py` -- merges the 0x004bdd00 objects in the GENERATED
  `gen-browser/globals.c`, idempotent. Everything in §5 needs it until B1 lands.
* `port-b6-rebuild-patched.sh` -- `ninja`, patch, `ninja` again (the closure is
  regenerated whenever `legoland_hostwin` changes, so the patch must be
  re-applied after every shim edit).
* `port-b6-headless-timed.sh` -- `legoland_headless` under a wall-clock cap,
  since the front end does not terminate.
